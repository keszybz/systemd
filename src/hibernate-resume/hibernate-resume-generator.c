/* SPDX-License-Identifier: LGPL-2.1+ */

#include <errno.h>
#include <stdio.h>

#include "alloc-util.h"
#include "blockdev-util.h"
#include "dissect-image.h"
#include "fd-util.h"
#include "fstab-util.h"
#include "log.h"
#include "mkdir.h"
#include "parse-util.h"
#include "proc-cmdline.h"
#include "special.h"
#include "string-util.h"
#include "unit-name.h"
#include "util.h"

#define HIBERNATE_SIG	"S1SUSPEND"

static const char *arg_dest = "/tmp";
static char *arg_resume_device = NULL;
static uint64_t arg_resume_offset = 0; /* offset in sectors */
static bool arg_noresume = false;

static int parse_proc_cmdline_item(const char *key, const char *value, void *data) {
        int r;

        if (streq(key, "resume")) {
                char *s;

                if (proc_cmdline_value_missing(key, value))
                        return 0;

                s = fstab_node_to_udev_node(value);
                if (!s)
                        return log_oom();

                free_and_replace(arg_resume_device, s);

        } else if (streq(key, "resume_offset")) {
                if (proc_cmdline_value_missing(key, value))
                        return 0;

                r = safe_atou64(value, &arg_resume_offset);
                if (r < 0)
                        log_warning_errno(r, "Failed to parse resume_offset \"%s\": %m", value);

        } else if (streq(key, "noresume"))
                arg_noresume = true;

        return 0;
}

static int swap_contains_hibernation_image(DissectedPartition *p) {
        _cleanup_close_ int fd = -1;

        log_debug("Checking %s for hibernation signature", p->node);

        fd = open(p->node, O_RDONLY|O_CLOEXEC);
        if (fd < 0)
                return log_error_errno(errno, "%s: cannot open: %m", p->node);

        if (lseek(fd, arg_resume_offset * 512, SEEK_SET) < 0)
                return log_error_errno(errno, "%s: cannot seek to position %"PRIu64": %m",
                                       p->node, arg_resume_offset * 512);

        uint8_t buf[10];
        ssize_t len;

        len = read(fd, buf, sizeof(buf));
        if (len != sizeof(buf))
                return log_error_errno(len < 0 ? errno : EIO, "%s: read failed: %m",
                                       p->node);

        return memcmp(buf, HIBERNATE_SIG, sizeof(buf)) == 0;
}

static int autodetect_resume_device(char **resume_device) {
        dev_t devnum;
        _cleanup_close_ int fd = -1;
        _cleanup_(dissected_image_unrefp) DissectedImage *m = NULL;
        int r;

        r = get_root_or_usr_block_dev(&devnum);
        if (r <= 0)
                return r;

        r = blockdev_open_parent(devnum, &fd);
        if (r <= 0)
                return r;

        r = dissect_image(fd, NULL, 0, DISSECT_IMAGE_READ_ONLY, &m);
        if (r == -ENOPKG) {
                log_debug_errno(r, "No suitable partition table found, ignoring.");
                return 0;
        }
        if (r < 0)
                return log_error_errno(r, "Failed to dissect: %m");

        if (!m->partitions[PARTITION_SWAP].found)
                return 0;

        r = swap_contains_hibernation_image(&m->partitions[PARTITION_SWAP]);
        if (r <= 0)
                return r;

        return free_and_strdup(resume_device, m->partitions[PARTITION_SWAP].node);
}

static int process_resume(const char *resume_device) {
        _cleanup_free_ char *name = NULL, *lnk = NULL;
        int r;

        assert(resume_device);

        r = unit_name_from_path_instance("systemd-hibernate-resume", resume_device, ".service", &name);
        if (r < 0)
                return log_error_errno(r, "Failed to generate unit name: %m");

        lnk = strjoin(arg_dest, "/" SPECIAL_SYSINIT_TARGET ".wants/", name);
        if (!lnk)
                return log_oom();

        mkdir_parents_label(lnk, 0755);
        if (symlink(SYSTEM_DATA_UNIT_PATH "/systemd-hibernate-resume@.service", lnk) < 0)
                return log_error_errno(errno, "Failed to create symlink %s: %m", lnk);

        return 0;
}

int main(int argc, char *argv[]) {
        int r = 0;

        log_set_prohibit_ipc(true);
        log_set_target(LOG_TARGET_AUTO);
        log_parse_environment();
        log_open();

        umask(0022);

        if (argc > 1 && argc != 4) {
                log_error("This program takes three or no arguments.");
                return EXIT_FAILURE;
        }

        if (argc > 1)
                arg_dest = argv[1];

        /* Don't even consider resuming outside of initramfs. */
        if (!in_initrd()) {
                log_debug("Not running in an initrd, quitting.");
                return EXIT_SUCCESS;
        }

        r = proc_cmdline_parse(parse_proc_cmdline_item, NULL, 0);
        if (r < 0)
                log_warning_errno(r, "Failed to parse kernel command line, ignoring: %m");

        if (arg_noresume) {
                log_notice("Found \"noresume\" on the kernel command line, quitting.");
                return EXIT_SUCCESS;
        }

        if (!arg_resume_device)
                r = autodetect_resume_device(&arg_resume_device);

        if (arg_resume_device)
                r = process_resume(arg_resume_device);

        free(arg_resume_device);
        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

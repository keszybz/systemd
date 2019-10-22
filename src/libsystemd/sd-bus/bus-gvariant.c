/* SPDX-License-Identifier: LGPL-2.1+ */

#include <errno.h>
#include <string.h>

#include "sd-bus.h"

#include "bus-gvariant.h"
#include "bus-signature.h"
#include "bus-type.h"

int bus_gvariant_get_size(const char *signature) {
        int sum = 0, r;

        /* For fixed size structs. Fails for variable size structs. */

        for (const char *p = signature; *p; ) {
                int n, alignment;

                n = signature_element_length_full(p, NULL, &alignment);
                if (n < 0)
                        return n;

                sum = ALIGN_TO(sum, alignment);

                switch (*p) {

                case SD_BUS_TYPE_BOOLEAN:
                case SD_BUS_TYPE_BYTE:
                        sum += 1;
                        break;

                case SD_BUS_TYPE_INT16:
                case SD_BUS_TYPE_UINT16:
                        sum += 2;
                        break;

                case SD_BUS_TYPE_INT32:
                case SD_BUS_TYPE_UINT32:
                case SD_BUS_TYPE_UNIX_FD:
                        sum += 4;
                        break;

                case SD_BUS_TYPE_INT64:
                case SD_BUS_TYPE_UINT64:
                case SD_BUS_TYPE_DOUBLE:
                        sum += 8;
                        break;

                case SD_BUS_TYPE_STRUCT_BEGIN:
                case SD_BUS_TYPE_DICT_ENTRY_BEGIN: {
                        if (n == 2) {
                                /* unary type () has fixed size of 1 */
                                r = 1;
                        } else {
                                char t[n-1];

                                memcpy(t, p + 1, n - 2);
                                t[n - 2] = 0;

                                r = bus_gvariant_get_size(t);
                                if (r < 0)
                                        return r;
                        }

                        sum += r;
                        break;
                }

                case SD_BUS_TYPE_STRING:
                case SD_BUS_TYPE_OBJECT_PATH:
                case SD_BUS_TYPE_SIGNATURE:
                case SD_BUS_TYPE_ARRAY:
                case SD_BUS_TYPE_VARIANT:
                        return -EINVAL;

                default:
                        assert_not_reached("Unknown signature type");
                }

                p += n;
        }

        r = bus_gvariant_get_alignment(signature);
        if (r < 0)
                return r;

        return ALIGN_TO(sum, r);
}

int bus_gvariant_get_alignment(const char *signature) {
        int alignment = 1;

        for (const char *p = signature; *p && alignment < 8; ) {
                int alignment_nested, n;

                n = signature_element_length_full(p, NULL, &alignment_nested);
                if (n < 0)
                        return n;

                alignment = MAX(alignment, alignment_nested);

                p += n;
        }

        return alignment;
}

int bus_gvariant_is_fixed_size(const char *signature) {
        assert(signature);

        for (const char *p = signature; *p; ) {
                int n;
                bool fixed;

                n = signature_element_length_full(p, &fixed, NULL);
                if (n < 0)
                        return n;
                if (!fixed)
                        return false;

                p += n;
        }

        return true;
}

size_t bus_gvariant_determine_word_size(size_t sz, size_t extra) {
        if (sz + extra <= 0xFF)
                return 1;
        else if (sz + extra*2 <= 0xFFFF)
                return 2;
        else if (sz + extra*4 <= 0xFFFFFFFF)
                return 4;
        else
                return 8;
}

size_t bus_gvariant_read_word_le(void *p, size_t sz) {
        union {
                uint16_t u16;
                uint32_t u32;
                uint64_t u64;
        } x;

        assert(p);

        if (sz == 1)
                return *(uint8_t*) p;

        memcpy(&x, p, sz);

        if (sz == 2)
                return le16toh(x.u16);
        else if (sz == 4)
                return le32toh(x.u32);
        else if (sz == 8)
                return le64toh(x.u64);

        assert_not_reached("unknown word width");
}

void bus_gvariant_write_word_le(void *p, size_t sz, size_t value) {
        union {
                uint16_t u16;
                uint32_t u32;
                uint64_t u64;
        } x;

        assert(p);
        assert(sz == 8 || (value < (1ULL << (sz*8))));

        if (sz == 1) {
                *(uint8_t*) p = value;
                return;
        } else if (sz == 2)
                x.u16 = htole16((uint16_t) value);
        else if (sz == 4)
                x.u32 = htole32((uint32_t) value);
        else if (sz == 8)
                x.u64 = htole64((uint64_t) value);
        else
                assert_not_reached("unknown word width");

        memcpy(p, &x, sz);
}

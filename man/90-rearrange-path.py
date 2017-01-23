#!/usr/bin/python3

"""
Proof-of-concept systemd environment generator that makes sure that
bin dirs are always after matching sbin dirs in the path.
(Changes /sbin:/bin:/foo/bar to /bin:/sbin:/foo/bar.)

This generator shows how to override the configuration possibly created by
earlier generators. It would be easier to write in bash, but let's have it
in python just to prove that we can, and to serve as a template for more
interesting generators.
"""

import sys
import os
import glob
import pathlib
import re

def update_env_from_file(env, file):
    "Update env dictionary based on KEY=VALUE assignments in file"

    def replace_var(match):
        name = match.group(2) or match.group(1)
        return env.get(name, '')

    def handle_line(line):
        var, op, val = line.partition('=')
        # look for valid assignments, ignore anything else
        if not op:
            return

        val = re.sub(r'(?<!\\)\$(\w+|\{([^}]*)\})', replace_var, val)
        env[var] = val

    fullline = ''
    for line in open(file):
        if line.startswith('#'):
            continue
        line = line.rstrip()
        if line.endswith('\\'):
            fullline += line[:-1]
            continue
        else:
            fullline += line

        handle_line(fullline.rstrip())
        fullline = ''

    handle_line(fullline.rstrip())

def current_env(dirs):
    """Read all environment variables from specified directories

    We look for .conf files only.
    Files in later directories override files with the same name in earlier dirs.
    Files are read in alphanumerical order.
    """

    files = dict((pathlib.Path(p).name, p)
                 for dir in dirs
                 for p in glob.glob(dir + '/*.conf'))

    paths = [path for name, path in sorted(files.items())]

    env = {}
    for path in paths:
        update_env_from_file(env, path)

    return env

def rearrange_bin_sbin(path):
    "Make sure any pair of …/bin, …/sbin directories is in this order"
    items = [pathlib.Path(p) for p in path.split(':')]
    for i in range(len(items)):
        if 'sbin' in items[i].parts:
            ind = items[i].parts.index('sbin')
            bin = pathlib.Path(*items[i].parts[:ind], 'bin', *items[i].parts[ind+1:])
            if bin in items[i+1:]:
                j = i + 1 + items[i+1:].index(bin)
                items[i], items[j] = items[j], items[i]
    return ':'.join(p.as_posix() for p in items)

def rearrange_path(dirs):
    env = current_env(dirs)
    path = env.get('PATH', None)
    if not path:
        return

    new = rearrange_bin_sbin(path)
    if new == path:
        return

    with open(dirs[-1] + '/90-rearrange-path.conf', 'w') as f:
        print('PATH={}'.format(new), file=f)

if __name__ == '__main__':
    rearrange_path(sys.argv[1:])

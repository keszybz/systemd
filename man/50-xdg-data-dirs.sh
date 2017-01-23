#!/bin/bash

dir="${!#}"
if ! [[ -d "$dir" ]]; then
    echo "Expected output directory as the last argument, got \"$dir\"" 1>&2
    exit 1
fi

# set the default value
XDG_DATA_DIRS="${XDG_DATA_DIRS:-/usr/local/share/:/usr/share}"

# add a directory if it exists
if [[ -d /opt/foo/share ]]; then
   XDG_DATA_DIRS=/opt/foo/share:${XDG_DATA_DIRS}
fi

# write our output
echo "XDG_DATA_DIRS=$XDG_DATA_DIRS" >"$dir/50-xdg-data-dirs.conf"

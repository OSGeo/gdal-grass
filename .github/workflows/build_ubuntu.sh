#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

# print commands
set -x

if [ -z "$1" ]; then
    echo "Usage: $0 PREFIX"
    exit 1
fi

# non-existent variables as an errors
set -u

# versionless in future?
GRASS=grass$(pkg-config --modversion grass | cut -d. -f1,2 | sed 's+\.++g')

export INSTALL_PREFIX=$1

./configure \
    --prefix=/usr \
    --with-autoload=/usr/lib/gdalplugins \
    --with-grass=/usr/lib/$(GRASS) \
    --with-postgres-includes=$(shell pg_config --includedir)"$INSTALL_PREFIX/"

eval $makecmd
make install

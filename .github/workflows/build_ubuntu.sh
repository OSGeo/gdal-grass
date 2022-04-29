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

PKGNAME=$(shell grep Package: debian/control | head -1 | cut -d' ' -f2)
GRASS=grass$(subst .,,$(shell pkg-config --modversion grass | cut -d. -f1,2))
GRASS_ABI=grass$(subst .,,$(shell pkg-config --modversion grass | cut -d. -f1,2,3 | sed -e 's/RC/-/')) 

export INSTALL_PREFIX=$1

./configure \
    --prefix=/usr \
    --with-autoload=/usr/lib/gdalplugins \
    --with-grass=/usr/lib/$(GRASS) \
    --with-postgres-includes=$(shell pg_config --includedir)"$INSTALL_PREFIX/"

eval $makecmd
make install

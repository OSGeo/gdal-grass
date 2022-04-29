#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

# print commands
set -x

if [ -z "$1" ]; then
    echo "Usage: $0 GDAL_AUTOLOAD_DIR"
    exit 1
fi

# non-existent variables as an errors
set -u

# versionless in future?
GRASS=grass$(pkg-config --modversion grass | cut -d. -f1,2 | sed 's+\.++g')

export GDAL_AUTOLOAD_DIR"=$1

./configure \
    --prefix=/usr \
    --with-autoload=${GDAL_AUTOLOAD_DIR} \
    --with-grass=/usr/lib/${GRASS} \
    --with-postgres-includes=$(pg_config --includedir)

make
make install

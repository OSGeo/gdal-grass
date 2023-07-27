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

export GDAL_AUTOLOAD_DIR=$1

mkdir build && cd build

cmake .. -DAUTOLOAD_DIR=${GDAL_AUTOLOAD_DIR}
cmake --build .
cmake --install .

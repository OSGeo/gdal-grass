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

export GDAL_DRIVER_PATH=$1

# Using LD_LIBRARY_PATH workaround for GRASS GIS < 7.8.8
export LD_LIBRARY_PATH=$(grass --config path)/lib

# test GRASS GIS raster map
gdalinfo $HOME/grassdata/nc_spm_08_micro/PERMANENT/cellhd/elevation

# test GRASS GIS vector map
ogrinfo -so -al $HOME/grassdata/nc_spm_08_micro/PERMANENT/vector/firestations/head

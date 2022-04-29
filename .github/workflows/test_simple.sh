#!/usr/bin/env bash

# fail on non-zero return code from a subprocess
set -e

# print commands
set -x

# non-existent variables as an errors
set -u

# add small GRASS GIS dataset for tests
(mkdir -p $HOME/grassdata && \
 cd $HOME/grassdata/ && \
 wget --quiet https://grass.osgeo.org/sampledata/north_carolina/nc_spm_08_micro.zip && \
 unzip nc_spm_08_micro.zip && \
 rm -f nc_spm_08_micro.zip )

# test GRASS GIS raster map
gdalinfo $HOME/grassdata/nc_spm_08_micro/PERMANENT/cellhd/elevation

# test GRASS GIS vector map
ogrinfo -so -al $HOME/grassdata/nc_spm_08_micro/PERMANENT/vector/firestations/head

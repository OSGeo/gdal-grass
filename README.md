## Standalone GRASS Drivers for GDAL and OGR

This package contains standalone drivers for [GRASS](http://grass.osgeo.org/)
raster and vector files that can be built after [GDAL](https://gdal.org/) has
been built and installed as a GDAL "autoload" driver.

This approach avoids circular dependencies with GRASS depending on GDAL,
but GDAL with GRASS support depending on GRASS. With this driver package
you configure and install GDAL normally, then build and install GRASS normally
and finally build and install this driver.

To build this driver it is necessary for it to find GDAL and GRASS
support files. Typically the configure and build process would look
something like:

```
./configure --with-gdal=/usr/bin/gdal-config \
            --with-grass=/usr/grass
make
sudo make install
```

## Usage

Set the driver path:

```bash
GDAL_DRIVER_PATH="/usr/lib/gdalplugins"
```

Access GRASS GIS raster data from GDAL:

```bash
gdalinfo $HOME/grassdata/nc_spm_08_grass7/PERMANENT/cellhd/elevation
```

Access GRASS GIS vector data from GDAL-OGR:

```bash
ogrinfo -so -al $HOME/grassdata/nc_spm_08_grass7/PERMANENT/vector/zipcodes/head
```

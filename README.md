# Introduction

Description: GRASS GIS extension for the GDAL library

GDAL is a translator library for raster geospatial data formats. As a library, it presents a single abstract data model to the calling application for all supported formats. This extension provides access to GRASS data via GDAL.

This package contains the two standalone GDAL-GRASS GIS drivers
(https://gdal.org/drivers/raster/grass.html and https://gdal.org/drivers/vector/grass.html)
for [GRASS GIS](http://grass.osgeo.org/) raster and vector file support
in [GDAL](https://gdal.org/).

This approach avoids circular dependencies with GRASS depending on GDAL,
but GDAL with GRASS support depending on GRASS. With this driver package
you configure and install GDAL normally, then build and install GRASS normally
and finally build and install this driver in
[GDAL's "autoload" directory](https://gdal.org/user/configoptions.html#driver-management).

## Installation

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

## Where is the gdal-grass driver available?

- Linux: https://repology.org/project/gdal-grass/versions
- Windows: https://trac.osgeo.org/osgeo4w/
- other operating systems: please add here

## Version number has been restarted

Note that during the transit of this driver out of core GDAL we have decided to reset the version numbering back to 1.

So: older packages show 3.x which the new driver is 1.x (or later).

## Release management

List of [milestones](https://github.com/OSGeo/gdal-grass/milestones).

## Tracking upstream changes

The release policies of the GDAL-GRASS driver are (so far) fairly simple:

- we follow the GDAL development for their breaking changes:
    - as of 2022, any GDAL 2+ and GDAL 3+ version is compliant, with GDAL 3+ recommended.
- we follow the GRASS GIS development for their breaking changes:
    - as of 2022, any GRASS GIS 7+ and GRASS GIS 8+ version is compliant, with GRASS GIS 8+ recommended.

We expect low maintenance needs for this driver.

## Using milestones

For easier planning, each issue and pull request will be assigned to a [milestone](https://github.com/OSGeo/gdal-grass/milestones).

## QA / CI

Any pull request opened in this repository is compiled and tested with
[GitHub Actions](https://github.com/OSGeo/gdal-grass/actions) against
the GDAL version included in Ubuntu (see related
[CI workflow](https://github.com/OSGeo/gdal-grass/blob/main/.github/workflows/ubuntu.yml)).
Improvements and other workflows are welcome, ideally as a [pull request](https://github.com/OSGeo/gdal-grass/pulls).

## Found a bug?

Please open an [issue](https://github.com/OSGeo/gdal-grass/issues) describing the problem along with a reproducible example.

## Who is involved here?

Please see the list of [contributors](https://github.com/OSGeo/gdal-grass/graphs/contributors).

#!/usr/bin/env pytest
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  GRASS Testing.
# Author:   Even Rouault <even dot rouault at spatialys.com>
#
###############################################################################
# Copyright (c) 2008, Even Rouault <even dot rouault at spatialys.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################

from osgeo import gdal
import gdaltest


###############################################################################
# Test if GRASS driver is present


def test_grass_load_driver():
    grassdriver = gdal.GetDriverByName("GRASS")
    assert grassdriver is not None, "Failed to load GRASS driver"


###############################################################################
# Read existing simple 1 band GRASS dataset.


def test_grass_2():
    tst = gdaltest.GDALTest(
        "GRASS", "small_grass_dataset/demomapset/cellhd/elevation", 1, 41487
    )

    srs = """PROJCS["UTM Zone 18, Northern Hemisphere",
    GEOGCS["grs80",
        DATUM["North_American_Datum_1983",
            SPHEROID["Geodetic_Reference_System_1980",6378137,298.257222101],
            TOWGS84[0.000,0.000,0.000]],
        PRIMEM["Greenwich",0],
        UNIT["degree",0.0174532925199433]],
    PROJECTION["Transverse_Mercator"],
    PARAMETER["latitude_of_origin",0],
    PARAMETER["central_meridian",-75],
    PARAMETER["scale_factor",0.9996],
    PARAMETER["false_easting",500000],
    PARAMETER["false_northing",0],
    UNIT["meter",1]]"""

    ret = tst.testOpen(check_prj=srs)
    return ret


###############################################################################
# Read metadata from band 1


def test_grass_3():
    ds = gdal.Open("./data/small_grass_dataset/demomapset/cellhd/elevation")
    assert ds is not None, "Cannot find layer"

    band = ds.GetRasterBand(1)
    assert band.GetNoDataValue() == 0.0
    assert band.GetMinimum() == 3.0
    assert band.GetMaximum() == 27.0
    assert band.GetMetadataItem("COLOR_TABLE_RULES_COUNT") == "0"
    assert band.GetColorInterpretation() == 1  # GCI_GrayIndex

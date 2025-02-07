#!/usr/bin/env pytest
###############################################################################
#
# Project:  GDAL/OGR Test Suite
# Purpose:  GRASS Testing.
# Author:   Even Rouault <even dot rouault at spatialys.com>
#
###############################################################################
# Copyright (c) 2008, Even Rouault <even dot rouault at spatialys.com>
#
# SPDX-License-Identifier: MIT
#
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

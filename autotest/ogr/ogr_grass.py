#!/usr/bin/env pytest
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  GRASS Testing.
# Author:   Even Rouault <even dot rouault at spatialys.com>
#
###############################################################################
# Copyright (c) 2009, Even Rouault <even dot rouault at spatialys.com>
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

from osgeo import ogr
import ogrtest


###############################################################################
# Read 'polygon' datasource


def test_ogr_load_driver():
    import os
    print(os.environ['GDAL_DRIVER_PATH'])
    ogr_grassdriver = ogr.GetDriverByName("OGR_GRASS")
    assert ogr_grassdriver is not None, "Failed to load OGR_GRASS driver"


def test_ogr_grass_polygon1():
    ds = ogr.Open("./data/PERMANENT/vector/country_boundaries/head")

    assert ds is not None, "Cannot open datasource"
    assert ds.GetLayerCount() == 3, "wrong number of layers"

    lyr = ds.GetLayerByName("country_boundaries")
    assert lyr is not None, "Cannot find layer"

    wkt = "POLYGON ((22.9523771501665 41.3379938828111,22.8813737321974 41.9992971868503,22.3805257504246 42.3202595078151,22.5450118344096 42.461362006188,22.4365946794613 42.5803211533239,22.6048014665713 42.8985187851611,22.9860185075885 43.211161200527,22.5001566911803 43.642814439461,22.4104464047216 44.0080634629,22.657149692483 44.2349230006613,22.9448323910518 43.8237853053471,23.3323022803763 43.8970108099047,24.1006791521242 43.7410513372479,25.5692716814269 43.6884447291747,26.0651587256997 43.9434937607513,27.2423995297409 44.1759860296324,27.9701070492751 43.8124681666752,28.558081495892 43.7074616562581,28.0390950863847 43.2931716985742,27.673897739378 42.5778923610062,27.9967204119054 42.0073587102878,27.1357393734905 42.1414848903013,26.1170418637208 41.8269046087246,26.1061381365072 41.3288988307278,25.1972013689254 41.2344859889305,24.492644891058 41.583896185872,23.6920736019923 41.3090809189439,22.9523771501665 41.3379938828111))"
    feat = lyr.GetNextFeature()

    ogrtest.check_feature_geometry(feat, wkt)

    assert feat.GetFieldAsString("name") == "Bulgaria"
    assert feat.GetFieldAsDouble("POP_EST") == 7204687.0


def test_ogr_grass_polygon2():
    ds = ogr.Open("./data/PERMANENT/vector/country_boundaries/head")
    lyr = ds.GetLayerByName("country_boundaries")
    feat = lyr.GetFeature(165)
    assert feat.GetFieldAsString("name") == "Luxembourg"
    wkt = """
          POLYGON ((
          5.67405195478483 49.5294835475575,
          5.78241743330091 50.0903278672212,
          6.04307335778111 50.1280516627942,
          6.24275109215699 49.9022256536787,
          6.18632042809418 49.4638028021145,
          5.89775923017638 49.4426671413072,
          5.67405195478483 49.5294835475575))"""
    ogrtest.check_feature_geometry(feat, wkt)

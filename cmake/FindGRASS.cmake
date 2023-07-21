#[=======================================================================[.rst:
FindGRASS
---------

Find GRASS GIS.

This module uses ``grass --config`` to extract necessary information from
GRASS GIS.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``GRAS_FOUND``
  True if GRASS is found.
``GRASS_BIN``
  Path to the grass binary.
``GRASS_GISBASE``
  Path to the GRASS GISBASE directory.
``GRASS_VERSION``
    The version of GRASS found.

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``GRASS_BIN``
  Path to the grass binary.
``GRASS_BIN_PREFER_PATH``
  Path to the directory containing the grass binary

Hints
^^^^^

Set ``GRASS_BIN`` to the GRASS executable binary file or
``GRASS_BIN_PREFER_PATH`` to the path containing the same.

E.g. ``-DGRASS_BIN=/usr/bin/grass`` or ``-DGRASS_BIN_PREFER_PATH=/usr/bin``.
#]=======================================================================]

set(ENV{LC_ALL} "en_US.UTF-8")

set(GRASS_BIN_PREFER_PATH
    ""
    CACHE STRING
          "Preferred path to directory containing GRASS binary (grass[N])")

if(NOT GRASS_BIN)
  foreach(grass_search_version "" 8 7)
    find_program(
      GRASS_BIN grass${grass_search_version}
      PATHS ${GRASS_BIN_PREFER_PATH}
            /usr/local/bin
            /usr/bin
            /sw/bin
            /opt/local/bin
            /opt/csw/bin
            /opt/bin
      DOC "Path to the grass[N] binary")
  endforeach()
endif()

if(GRASS_BIN)
  execute_process(
    COMMAND ${GRASS_BIN} --config path
    OUTPUT_VARIABLE GRASS_GISBASE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(
    COMMAND ${GRASS_BIN} --config version
    OUTPUT_VARIABLE GRASS_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GRASS REQUIRED_VARS GRASS_BIN GRASS_GISBASE
                                                      GRASS_VERSION)

if(GRASS_FOUND)
  message(STATUS "Found GRASS_GISBASE: ${GRASS_GISBASE} (${GRASS_VERSION})")
else()
  message(FATAL_ERROR "Failed to find GRASS")
endif()

set(GRASS_INCLUDE "${GRASS_GISBASE}/include")
mark_as_advanced(GRASS_INCLUDE)

set(G_RASTLIBS -lgrass_raster -lgrass_imagery)
set(G_VECTLIBS
    -lgrass_vector
    -lgrass_dig2
    -lgrass_dgl
    -lgrass_rtree
    -lgrass_linkm
    -lgrass_dbmiclient
    -lgrass_dbmibase)
set(G_LIBS
    -L${GRASS_GISBASE}/lib
    ${G_VECTLIBS}
    ${G_RASTLIBS}
    -lgrass_gproj
    -lgrass_gmath
    -lgrass_gis
    -lgrass_datetime
    -lgrass_btree2
    -lgrass_ccmath)

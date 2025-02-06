#[=======================================================================[.rst:
FindPROJ
--------

Find PROJ's include path.

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``PROJ_INCLUDE_DIRS``
  the directories of the PROJ headers (proj.h etc.)
``PROJ_FOUND``
  True if PROJ is found.

Hints
^^^^^

Set ``PROJ_INCLUDE_DIR`` environment variable to specify the location of
non-standard location of PROJ include directory.

#]=======================================================================]

include(${CMAKE_SOURCE_DIR}/cmake/GRASSUtilities.cmake)

set(PROJ_INCLUDE_DIRS)
set(PROJ_FOUND)

if(PROJ_INCLUDE_DIR)
  # if PROJ_INCLUDE_DIR is explicitly set, use it
  set(PROJ_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})
  set(PROJ_FOUND TRUE)
else()
  # try to use GRASS' settings
  get_grass_platform_var("${GRASS_GISBASE}/include/Make/Platform.make" "PROJINC" PROJ_INCLUDE_DIRS)
  if(PROJ_INCLUDE_DIRS)
    string(REGEX REPLACE "-I(.*)" "\\1" PROJ_INCLUDE_DIRS "${PROJ_INCLUDE_DIRS}")
    set(PROJ_FOUND TRUE)
  endif()
endif()

if(NOT PROJ_FOUND)
  find_package(PROJ CONFIG)
endif()

if(NOT PROJ_FOUND)
  find_path(PROJ_INCLUDE_DIRS NAMES proj_api.h proj.h)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PROJ REQUIRED_VARS PROJ_INCLUDE_DIRS PROJ_FOUND)

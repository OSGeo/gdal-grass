#[=======================================================================[.rst:
FindPROJ
--------------

Find PROJ's include path.
#]=======================================================================]

include(${CMAKE_SOURCE_DIR}/cmake/GRASSUtilities.cmake)

set(PROJINC "")

get_grass_platform_var("${GRASS_GISBASE}/include/Make/Platform.make" "PROJINC"
                       PROJINC)

if(NOT "${PROJINC}" STREQUAL "")
  string(REGEX REPLACE "-I(.*)" "\\1" PROJINC "${PROJINC}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PROJ REQUIRED_VARS PROJINC)

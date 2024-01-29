#[=======================================================================[.rst:
FindPostgreSQL
--------------

Find PostgreSQL's include path.
#]=======================================================================]

include(${CMAKE_SOURCE_DIR}/cmake/GRASSUtilities.cmake)

set(PQ_INCLUDE "")

get_grass_platform_var("${GRASS_GISBASE}/include/Make/Platform.make"
                       "PQINCPATH" PQINCPATH)
get_grass_platform_var("${GRASS_GISBASE}/include/Make/Platform.make"
                       "USE_POSTGRES" USE_POSTGRES)

if(NOT "${PQINCPATH}" STREQUAL "")
  string(REGEX REPLACE "-I(.*)" "\\1" PQINCPATH "${PQINCPATH}")
endif()

if(USE_POSTGRES)
  set(PQ_INCLUDE ${PQINCPATH})
elseif(POSTGRES_INCLUDES_DIR)
  set(PQ_INCLUDE ${POSTGRES_INCLUDES_DIR})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PostgreSQL REQUIRED_VARS PQ_INCLUDE)

# SPDX-License-Identifier: MIT

# ##############################################################################
# CMake settings
cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_COLOR_MAKEFILE ON)

# set path to additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ##############################################################################
# Project and version
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(VERSION_SUFFIX "rc1")
set(COMPLETE_VERSION
    ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
)
set(RELEASE_NAME ${COMPLETE_VERSION}${VERSION_SUFFIX})

project(gdal-grass VERSION ${COMPLETE_VERSION})

message(STATUS "gdal-grass version ${RELEASE_NAME}")

include(CTest)

# ##############################################################################
# Dependencies and flags
find_package(GDAL REQUIRED)
find_package(GRASS REQUIRED)
find_package(PostgreSQL)
find_package(PROJ)

if(NOT AUTOLOAD_DIR)
  execute_process(
    COMMAND ${GDAL_CONFIG} --prefix
    OUTPUT_VARIABLE GDALPREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(AUTOLOAD_DIR ${GDALPREFIX}/lib/gdalplugins)
  if(NOT AUTOLOAD_DIR)
    message(FATAL_ERROR "Could not set GDAL plugin path")
  endif()
endif()
message(
  STATUS "Using ${AUTOLOAD_DIR} as GDAL shared library autoload directory")

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -DUSE_CPL -DGRASS_GISBASE=\\\"${GRASS_GISBASE}\\\"")

# ##############################################################################
# Build
set(GLIB_SOURCES source/grass.cpp)
set(OLIB_SOURCES source/ogrgrassdriver.cpp source/ogrgrassdatasource.cpp
                 source/ogrgrasslayer.cpp source/ogrgrass.h)

add_library(gdal_grass SHARED ${GLIB_SOURCES})
set_target_properties(gdal_grass PROPERTIES PREFIX "")
set_target_properties(gdal_grass PROPERTIES OUTPUT_NAME "gdal_GRASS")
set_target_properties(gdal_grass PROPERTIES INSTALL_RPATH "${GRASS_GISBASE}/lib")
target_include_directories(
  gdal_grass PRIVATE ${CMAKE_SOURCE_DIR} ${GDAL_INCLUDE_DIR} ${PostgreSQL_INCLUDE_DIRS}
                     ${GRASS_INCLUDE} ${PROJ_INCLUDE_DIRS})
target_link_libraries(gdal_grass PUBLIC ${GDAL_LIBRARY} ${G_LIBS})
install(TARGETS gdal_grass DESTINATION ${AUTOLOAD_DIR})

add_library(ogr_grass SHARED ${OLIB_SOURCES})
set_target_properties(ogr_grass PROPERTIES PREFIX "")
set_target_properties(ogr_grass PROPERTIES OUTPUT_NAME "ogr_GRASS")
set_target_properties(ogr_grass PROPERTIES INSTALL_RPATH "${GRASS_GISBASE}/lib")
target_include_directories(
  ogr_grass PRIVATE ${CMAKE_SOURCE_DIR} ${GDAL_INCLUDE_DIR} ${PostgreSQL_INCLUDE_DIRS}
                    ${GRASS_INCLUDE} ${PROJ_INCLUDE_DIRS})
target_link_libraries(ogr_grass PUBLIC ${GDAL_LIBRARY} ${G_LIBS})
install(TARGETS ogr_grass DESTINATION ${AUTOLOAD_DIR})

# ##############################################################################
# Tests

if (Python_LOOKUP_VERSION)
    set(Python_FIND_STRATEGY VERSION)
    find_package(Python ${Python_LOOKUP_VERSION} EXACT COMPONENTS Interpreter Development NumPy)
else ()
    set(Python_FIND_STRATEGY LOCATION)
    find_package(Python 3.11 COMPONENTS Interpreter Development NumPy)
endif ()

if (BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/autotest")
    add_subdirectory(autotest)
endif ()

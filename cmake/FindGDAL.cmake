# FindGDAL
# --------
# Copyright (c) 2007, Magnus Homann <magnus at homann dot se> Redistribution and
# use is allowed according to the terms of the BSD license. For details see the
# accompanying COPYING-CMAKE-SCRIPTS file.
#
# Once run this will define:
#
# GDAL_FOUND         = system has GDAL lib
#
# GDAL_LIBRARY       = full path to the library
#
# GDAL_INCLUDE_DIR   = where to find headers

include(${CMAKE_SOURCE_DIR}/cmake/MacPlistMacros.cmake)

if(WIN32)

  if(MINGW)
    find_path(GDAL_INCLUDE_DIR gdal.h /usr/local/include /usr/include
              c:/msys/local/include PATH_SUFFIXES gdal)
    find_library(
      GDAL_LIBRARY
      NAMES gdal
      PATHS /usr/local/lib /usr/lib c:/msys/local/lib)
  endif(MINGW)

  if(MSVC)
    find_path(GDAL_INCLUDE_DIR gdal.h "$ENV{LIB_DIR}/include/gdal"
              $ENV{INCLUDE})
    find_library(
      GDAL_LIBRARY
      NAMES gdal gdal_i
      PATHS "$ENV{LIB_DIR}/lib" $ENV{LIB} /usr/lib c:/msys/local/lib)
    if(GDAL_LIBRARY)
      set(GDAL_LIBRARY;odbc32;odbccp32 CACHE STRING INTERNAL)
    endif(GDAL_LIBRARY)
  endif(MSVC)

elseif(APPLE AND QGIS_MAC_DEPS_DIR)

  find_path(GDAL_INCLUDE_DIR gdal.h "$ENV{LIB_DIR}/include")
  find_library(
    GDAL_LIBRARY
    NAMES gdal
    PATHS "$ENV{LIB_DIR}/lib")

else(WIN32)

  if(UNIX)

    # try to use framework on mac want clean framework path, not unix
    # compatibility path
    if(APPLE)
      if(CMAKE_FIND_FRAMEWORK MATCHES "FIRST"
         OR CMAKE_FRAMEWORK_PATH MATCHES "ONLY"
         OR NOT CMAKE_FIND_FRAMEWORK)
        set(CMAKE_FIND_FRAMEWORK_save
            ${CMAKE_FIND_FRAMEWORK}
            CACHE STRING "" FORCE)
        set(CMAKE_FIND_FRAMEWORK
            "ONLY"
            CACHE STRING "" FORCE)
        find_library(GDAL_LIBRARY GDAL)
        if(GDAL_LIBRARY)
          # they're all the same in a framework
          set(GDAL_INCLUDE_DIR
              ${GDAL_LIBRARY}/Headers
              CACHE PATH "Path to a file.")
          # set GDAL_CONFIG to make later test happy, not used here, may not
          # exist
          set(GDAL_CONFIG
              ${GDAL_LIBRARY}/unix/bin/gdal-config
              CACHE FILEPATH "Path to a program.")
          # version in info.plist
          get_version_plist(${GDAL_LIBRARY}/Resources/Info.plist GDAL_VERSION)
          if(NOT GDAL_VERSION)
            message(
              FATAL_ERROR "Could not determine GDAL version from framework.")
          endif(NOT GDAL_VERSION)
          string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
                               GDAL_VERSION_MAJOR "${GDAL_VERSION}")
          string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
                               GDAL_VERSION_MINOR "${GDAL_VERSION}")
          if(GDAL_VERSION_MAJOR LESS 3)
            message(
              FATAL_ERROR
                "GDAL version is too old (${GDAL_VERSION}). Use 3.2 or higher.")
          endif(GDAL_VERSION_MAJOR LESS 3)
          if((GDAL_VERSION_MAJOR EQUAL 3) AND (GDAL_VERSION_MINOR LESS 2))
            message(
              FATAL_ERROR
                "GDAL version is too old (${GDAL_VERSION}). Use 3.2 or higher.")
          endif((GDAL_VERSION_MAJOR EQUAL 3) AND (GDAL_VERSION_MINOR LESS 2))

        endif(GDAL_LIBRARY)
        set(CMAKE_FIND_FRAMEWORK
            ${CMAKE_FIND_FRAMEWORK_save}
            CACHE STRING "" FORCE)
      endif()
    endif(APPLE)

    if(CYGWIN)
      find_library(
        GDAL_LIBRARY
        NAMES gdal
        PATHS /usr/lib /usr/local/lib)
    endif(CYGWIN)

    if(NOT GDAL_INCLUDE_DIR
       OR NOT GDAL_LIBRARY
       OR NOT GDAL_CONFIG)
      # didn't find OS X framework, and was not set by user
      set(GDAL_CONFIG_PREFER_PATH
          "$ENV{GDAL_HOME}/bin"
          CACHE STRING "preferred path to GDAL (gdal-config)")
      set(GDAL_CONFIG_PREFER_FWTOOLS_PATH
          "$ENV{FWTOOLS_HOME}/bin_safe"
          CACHE STRING "preferred path to GDAL (gdal-config) from FWTools")
      find_program(
        GDAL_CONFIG
        gdal-config
        ${GDAL_CONFIG_PREFER_PATH}
        ${GDAL_CONFIG_PREFER_FWTOOLS_PATH}
        $ENV{LIB_DIR}/bin
        /usr/local/bin/
        /usr/bin/)
      # MESSAGE("DBG GDAL_CONFIG ${GDAL_CONFIG}")

      if(GDAL_CONFIG)

        # extract gdal version
        exec_program(
          ${GDAL_CONFIG} ARGS
          --version
          OUTPUT_VARIABLE GDAL_VERSION)
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
                             GDAL_VERSION_MAJOR "${GDAL_VERSION}")
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
                             GDAL_VERSION_MINOR "${GDAL_VERSION}")
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3"
                             GDAL_VERSION_MICRO "${GDAL_VERSION}")

        # MESSAGE("DBG GDAL_VERSION ${GDAL_VERSION}") MESSAGE("DBG
        # GDAL_VERSION_MAJOR ${GDAL_VERSION_MAJOR}") MESSAGE("DBG
        # GDAL_VERSION_MINOR ${GDAL_VERSION_MINOR}")

        # check for gdal version version 1.2.5 is known NOT to be supported
        # (missing CPL_STDCALL macro) According to INSTALL, 2.1+ is required
        if(GDAL_VERSION_MAJOR LESS 3)
          message(
            FATAL_ERROR
              "GDAL version is too old (${GDAL_VERSION}). Use 3.0 or higher.")
        endif(GDAL_VERSION_MAJOR LESS 3)
        # IF ( (GDAL_VERSION_MAJOR EQUAL 2) AND (GDAL_VERSION_MINOR LESS 1) )
        # MESSAGE (FATAL_ERROR "GDAL version is too old (${GDAL_VERSION}). Use
        # 2.1 or higher.") ENDIF( (GDAL_VERSION_MAJOR EQUAL 2) AND
        # (GDAL_VERSION_MINOR LESS 1) )
        if((GDAL_VERSION_MAJOR EQUAL 3)
           AND (GDAL_VERSION_MINOR EQUAL 0)
           AND (GDAL_VERSION_MICRO LESS 3))
          message(
            FATAL_ERROR
              "GDAL version is too old (${GDAL_VERSION}). Use 3.0.3 or higher.")
        endif(
          (GDAL_VERSION_MAJOR EQUAL 3)
          AND (GDAL_VERSION_MINOR EQUAL 0)
          AND (GDAL_VERSION_MICRO LESS 3))

        # set INCLUDE_DIR to prefix+include
        exec_program(
          ${GDAL_CONFIG} ARGS
          --prefix
          OUTPUT_VARIABLE GDAL_PREFIX)
        # SET(GDAL_INCLUDE_DIR ${GDAL_PREFIX}/include CACHE STRING INTERNAL)
        find_path(GDAL_INCLUDE_DIR gdal.h ${GDAL_PREFIX}/include/gdal
                  ${GDAL_PREFIX}/include /usr/local/include /usr/include)

        # extract link dirs for rpath
        exec_program(
          ${GDAL_CONFIG} ARGS
          --libs
          OUTPUT_VARIABLE GDAL_CONFIG_LIBS)

        # split off the link dirs (for rpath) use regular expression to match
        # wildcard equivalent "-L*<endchar>" with <endchar> is a space or a
        # semicolon
        string(REGEX MATCHALL "[-][L]([^ ;])+"
                     GDAL_LINK_DIRECTORIES_WITH_PREFIX "${GDAL_CONFIG_LIBS}")
        # MESSAGE("DBG
        # GDAL_LINK_DIRECTORIES_WITH_PREFIX=${GDAL_LINK_DIRECTORIES_WITH_PREFIX}")

        # remove prefix -L because we need the pure directory for
        # LINK_DIRECTORIES

        if(GDAL_LINK_DIRECTORIES_WITH_PREFIX)
          string(REGEX REPLACE "[-][L]" "" GDAL_LINK_DIRECTORIES
                               ${GDAL_LINK_DIRECTORIES_WITH_PREFIX})
        endif(GDAL_LINK_DIRECTORIES_WITH_PREFIX)

        # split off the name use regular expression to match wildcard equivalent
        # "-l*<endchar>" with <endchar> is a space or a semicolon
        string(REGEX MATCHALL "[-][l]([^ ;])+" GDAL_LIB_NAME_WITH_PREFIX
                     "${GDAL_CONFIG_LIBS}")
        # MESSAGE("DBG  GDAL_LIB_NAME_WITH_PREFIX=${GDAL_LIB_NAME_WITH_PREFIX}")

        # remove prefix -l because we need the pure name

        if(GDAL_LIB_NAME_WITH_PREFIX)
          string(REGEX REPLACE "[-][l]" "" GDAL_LIB_NAME
                               ${GDAL_LIB_NAME_WITH_PREFIX})
        endif(GDAL_LIB_NAME_WITH_PREFIX)

        if(APPLE)
          if(NOT GDAL_LIBRARY)
            # work around empty GDAL_LIBRARY left by framework check while still
            # preserving user setting if given ***FIXME*** need to improve
            # framework check so below not needed
            set(GDAL_LIBRARY
                ${GDAL_LINK_DIRECTORIES}/lib${GDAL_LIB_NAME}.dylib
                CACHE STRING INTERNAL FORCE)
          endif(NOT GDAL_LIBRARY)
        else(APPLE)
          find_library(
            GDAL_LIBRARY
            NAMES ${GDAL_LIB_NAME} gdal
            PATHS ${GDAL_LINK_DIRECTORIES}/lib ${GDAL_LINK_DIRECTORIES})
        endif(APPLE)

      else(GDAL_CONFIG)
        message(
          "FindGDAL.cmake: gdal-config not found. Please set it manually. GDAL_CONFIG=${GDAL_CONFIG}"
        )
      endif(GDAL_CONFIG)
    endif(
      NOT GDAL_INCLUDE_DIR
      OR NOT GDAL_LIBRARY
      OR NOT GDAL_CONFIG)
  endif(UNIX)
endif(WIN32)

if(GDAL_INCLUDE_DIR AND GDAL_LIBRARY)
  set(GDAL_FOUND TRUE)
endif(GDAL_INCLUDE_DIR AND GDAL_LIBRARY)

if(GDAL_FOUND)

  if(NOT GDAL_FIND_QUIETLY)
    file(READ ${GDAL_INCLUDE_DIR}/gdal_version.h gdal_version)
    string(REGEX REPLACE "^.*GDAL_RELEASE_NAME +\"([^\"]+)\".*$" "\\1"
                         GDAL_RELEASE_NAME "${gdal_version}")

    message(STATUS "Found GDAL: ${GDAL_LIBRARY} (${GDAL_RELEASE_NAME})")
  endif(NOT GDAL_FIND_QUIETLY)

else(GDAL_FOUND)

  message(GDAL_INCLUDE_DIR=${GDAL_INCLUDE_DIR})
  message(GDAL_LIBRARY=${GDAL_LIBRARY})
  message(FATAL_ERROR "Could not find GDAL")

endif(GDAL_FOUND)

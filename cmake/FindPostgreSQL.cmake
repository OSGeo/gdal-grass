#[=======================================================================[.rst:
FindPostgreSQL
--------------

Find PostgreSQL's include path.

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``PostgreSQL_FOUND``
  True if PostgreSQL is found.
``PostgreSQL_INCLUDE_DIRS``
  the directories of the PostgreSQL headers

Hints
^^^^^

Set ``PostgreSQL_INCLUDE_DIR`` environment variable to specify the location of
non-standard location of PostgreSQL include directory.

#]=======================================================================]

# check out GRASS' settings
include(${CMAKE_SOURCE_DIR}/cmake/GRASSUtilities.cmake)
get_grass_platform_var("${GRASS_GISBASE}/include/Make/Platform.make" "PQINCPATH" PQINCPATH)
get_grass_platform_var("${GRASS_GISBASE}/include/Make/Platform.make" "USE_POSTGRES" USE_POSTGRES)
if(PQINCPATH)
  string(REGEX REPLACE "-I(.*)" "\\1" PQINCPATH "${PQINCPATH}")
endif()

if(PostgreSQL_INCLUDE_DIR)
  # if PostgreSQL_INCLUDE_DIR is explicitly set, use it
  set(PostgreSQL_FOUND TRUE)
elseif(USE_POSTGRES AND PQINCPATH)
  # Use GRASS' settings
  set(PostgreSQL_INCLUDE_DIR ${PQINCPATH})
  set(PostgreSQL_FOUND TRUE)
endif()

if(NOT PostgreSQL_FOUND)
  set(PostgreSQL_KNOWN_VERSIONS ${PostgreSQL_ADDITIONAL_VERSIONS}
      "16" "15" "14" "13" "12" "11" "10" "9.6" "9.5" "9.4" "9.3" "9.2" "9.1" "9.0" "8.4" "8.3" "8.2" "8.1" "8.0")

  # Define additional search paths for root directories.
  set( PostgreSQL_ROOT_DIRECTORIES
     ENV PostgreSQL_ROOT
     ${PostgreSQL_ROOT}
  )
  foreach(suffix ${PostgreSQL_KNOWN_VERSIONS})
    if(WIN32)
      list(APPEND PostgreSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES
          "PostgreSQL/${suffix}/lib")
      list(APPEND PostgreSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES
          "PostgreSQL/${suffix}/include")
      list(APPEND PostgreSQL_TYPE_ADDITIONAL_SEARCH_SUFFIXES
          "PostgreSQL/${suffix}/include/server")
    endif()
    if(UNIX)
      list(APPEND PostgreSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES
          "postgresql${suffix}"
          "postgresql@${suffix}"
          "pgsql-${suffix}/lib")
      list(APPEND PostgreSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES
          "postgresql${suffix}"
          "postgresql@${suffix}"
          "postgresql/${suffix}"
          "pgsql-${suffix}/include")
      list(APPEND PostgreSQL_TYPE_ADDITIONAL_SEARCH_SUFFIXES
          "postgresql${suffix}/server"
          "postgresql@${suffix}/server"
          "postgresql/${suffix}/server"
          "pgsql-${suffix}/include/server")
    endif()
  endforeach()

  find_path(PostgreSQL_INCLUDE_DIR
    NAMES libpq-fe.h
    PATHS
     # Look in other places.
     ${PostgreSQL_ROOT_DIRECTORIES}
    PATH_SUFFIXES
      pgsql
      postgresql
      include
      ${PostgreSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES}
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PostgreSQL REQUIRED_VARS PostgreSQL_INCLUDE_DIR PostgreSQL_FOUND)

if(PostgreSQL_FOUND)
  set(PostgreSQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIR})
endif()

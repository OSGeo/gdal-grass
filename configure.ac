dnl ***************************************************************************
dnl $Id$
dnl
dnl Project:  GDAL GRASS Plugin
dnl Purpose:  Configure source file.
dnl Author:   Frank Warmerdam, warmerdam@pobox.com
dnl
dnl ***************************************************************************
dnl Copyright (c) 2005, Frank Warmerdam
dnl
dnl SPDX-License-Identifier: MIT
dnl
dnl ***************************************************************************

dnl Disable configure caching ... it causes lots of hassles.
define([AC_CACHE_LOAD], )
define([AC_CACHE_SAVE], )

dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_SRCDIR([Makefile.in])

dnl We require autoconf 2.52+ for libtool support on cygwin/mingw hosts
AC_PREREQ([2.71])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX

AC_PROG_RANLIB
AC_COMPILER_PIC
AC_LD_SHARED
AC_COMPILER_WFLAGS

dnl ---------------------------------------------------------------------------
dnl Find GDAL
dnl ---------------------------------------------------------------------------

AC_ARG_WITH(gdal,
[  --with-gdal[=PATH]        GDAL (PATH is path to gdal-config)],,)

if test "$with_gdal" = "yes" -o "$with_gdal" = "" ; then

  if test "`basename xx/$with_gdal`" = "gdal-config" ; then
    GDAL_CONFIG="$with_gdal"
  fi

  if test -z "$GDAL_CONFIG" ; then
    AC_PATH_PROG(GDAL_CONFIG, gdal-config, no)
  fi

  if test "$GDAL_CONFIG" = "no" ; then
    AC_MSG_ERROR([couldn't find gdal-config])
  fi

elif test -n "$with_gdal" -a "$with_gdal" != "no" ; then

  GDAL_CONFIG=$with_gdal

  if test -f "$GDAL_CONFIG" -a -x "$GDAL_CONFIG" ; then
    AC_MSG_RESULT([user supplied gdal-config ($GDAL_CONFIG)])
  else
    AC_MSG_ERROR(['$GDAL_CONFIG' is not an executable.  Make sure you use --with-gdal=/path/to/gdal-config])
  fi

else

  AC_MSG_ERROR([gdal required to build GDAL GRASS driver])

fi

LIBS="`$GDAL_CONFIG --libs` $LIBS"
GDAL_INC=`$GDAL_CONFIG --cflags`

AC_SUBST(GDAL_INC,    $GDAL_INC)

dnl ---------------------------------------------------------------------------
dnl Where to put driver?
dnl ---------------------------------------------------------------------------
AC_ARG_WITH(autoload,[  --with-autoload[=DIR]     Directory for autoload drivers],,)

if test "$with_autoload" != "" ; then
  AUTOLOAD_DIR=$with_autoload
else
  if $GDAL_CONFIG --autoload > /dev/null 2>&1 ; then
    AUTOLOAD_DIR=`$GDAL_CONFIG --autoload`
  else
    AUTOLOAD_DIR=`$GDAL_CONFIG --prefix`/lib/gdalplugins
  fi
fi

AC_MSG_RESULT(using $AUTOLOAD_DIR as GDAL shared library autoload directory)
AC_SUBST(AUTOLOAD_DIR,$AUTOLOAD_DIR)

dnl ---------------------------------------------------------------------------
dnl Find GRASS GIS
dnl ---------------------------------------------------------------------------

GRASS_SETTING=no
GRASS_INCLUDE=
GRASS_GISBASE=
export GRASS_INCLUDE GRASS_SETTING GRASS_GISBASE

AC_ARG_WITH(grass,[  --with-grass[=ARG]        Include GRASS support (ARG=GRASS install tree dir)],,)

if test "$with_grass" = "no" ; then
  AC_MSG_ERROR([grass required for this driver, please install GRASS 5.7 or later and rebuild])
fi

if test "$with_grass" != "yes" ; then

  AC_CHECK_LIB(grass_gis,G_read_compressed,GRASS_SETTING=grass72+,GRASS_SETTING=no,-L$with_grass/lib -lgrass_datetime)
  if test "$GRASS_SETTING" = "no" ; then
    AC_CHECK_LIB(grass_gis,G_is_initialized,GRASS_SETTING=grass70+,GRASS_SETTING=no,-L$with_grass/lib -lgrass_datetime)
    if test "$GRASS_SETTING" = "no" ; then
      AC_CHECK_LIB(grass_gis,G_asprintf,GRASS_SETTING=grass57+,GRASS_SETTING=no,-L$with_grass/lib -lgrass_datetime)
    fi
  fi

  if test "$GRASS_SETTING" != "no" ; then
    if test "$GRASS_SETTING" = "grass72+" ; then
      G_RASTLIBS="-lgrass_raster -lgrass_imagery"
      G_VECTLIBS="-lgrass_vector -lgrass_dig2 -lgrass_dgl -lgrass_rtree -lgrass_linkm -lgrass_dbmiclient -lgrass_dbmibase"
      LIBS="-L$with_grass/lib $G_VECTLIBS $G_RASTLIBS -lgrass_gproj -lgrass_gmath -lgrass_gis -lgrass_datetime -lgrass_btree2 -lgrass_ccmath $LIBS"
    elif test "$GRASS_SETTING" = "grass70+" ; then
      G_RASTLIBS="-lgrass_raster -lgrass_imagery"
      G_VECTLIBS="-lgrass_vector -lgrass_dig2 -lgrass_dgl -lgrass_rtree -lgrass_linkm -lgrass_dbmiclient -lgrass_dbmibase"
      LIBS="-L$with_grass/lib $G_VECTLIBS $G_RASTLIBS -lgrass_gproj -lgrass_gmath -lgrass_gis -lgrass_datetime $LIBS"
    else
      G_RASTLIBS="-lgrass_I"
      G_VECTLIBS="-lgrass_vect -lgrass_dig2 -lgrass_dgl -lgrass_rtree -lgrass_linkm -lgrass_dbmiclient -lgrass_dbmibase"
      LIBS="-L$with_grass/lib $G_VECTLIBS $G_RASTLIBS -lgrass_gproj -lgrass_vask -lgrass_gmath -lgrass_gis -lgrass_datetime $LIBS"
    fi
    GRASS_INCLUDE="-I$with_grass/include"
    GRASS_GISBASE="$with_grass"
    HAVE_GRASS=yes
  else
    AC_MSG_ERROR([--with-grass=$with_grass requested, but libraries not found!])
  fi
fi

AC_SUBST(GRASS_INCLUDE,$GRASS_INCLUDE)
AC_SUBST(GRASS_GISBASE,$GRASS_GISBASE)

dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl Find PostgreSQL (GRASS GIS optional dependency)
dnl ---------------------------------------------------------------------------

# Scan GRASS_INCLUDE for USE_POSTGRES already enabled in GRASS GIS
G_PG_ENABLED=0
grep USE_POSTGRES $with_grass/include/Make/Platform.make | cut -d'=' -f2 | tr -s ' ' ' ' | cut -d' ' -f2 | grep 1 > /dev/null && G_PG_ENABLED=1
# Scan for PG include path if enabled in GRASS GIS
if test $G_PG_ENABLED -eq 1  ; then
   PQ_INCLUDE=`grep PQINCPATH $with_grass/include/Make/Platform.make | cut -d'=' -f2 | tr -s ' ' ' ' | cut -d' ' -f2`
   echo "PostgreSQL already enabled in GRASS GIS. Using related PQ_INCLUDE path $PQ_INCLUDE"
else
   AC_ARG_WITH(postgres_includes,[  --with-postgres-includes=DIR     use PostgreSQL includes in DIR], postgres_includes="$withval", postgres_includes=no)

   PQ_INCLUDE=
   if test "x$postgres_includes" != "xno"; then
   # With PostgreSQL includes directory
   PQ_INCLUDE="-I$postgres_includes"
   fi
fi

AC_SUBST(PQ_INCLUDE)

dnl ---------------------------------------------------------------------------

rm -f conftest*

AC_CONFIG_FILES([Makefile])
AC_OUTPUT




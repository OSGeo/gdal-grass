/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRGRASSDataSource class.
 * Author:   Radim Blazek, radim.blazek@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2005, Radim Blazek <radim.blazek@gmail.com>
 * Copyright (c) 2008-2020, Even Rouault <even dot rouault at spatialys.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <array>
#include <cstring>

#include "ogrgrass.h"
#include "cpl_conv.h"
#include "cpl_string.h"

/************************************************************************/
/*                         Grass2CPLErrorHook()                         */
/************************************************************************/
static auto Grass2CPLErrorHook(char *pszMessage, int bFatal) -> int
{
    if (!bFatal)
        CPLError(CE_Warning, CPLE_AppDefined, "GRASS warning: %s", pszMessage);
    else
        CPLError(CE_Warning, CPLE_AppDefined, "GRASS fatal error: %s",
                 pszMessage);

    return 0;
}

/************************************************************************/
/*                        ~OGRGRASSDataSource()                         */
/************************************************************************/
OGRGRASSDataSource::~OGRGRASSDataSource()
{
    for (int i = 0; i < nLayers; i++)
        delete papoLayers[i];

    if (bOpened)
        Vect_close(&map);
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

using GrassErrorHandler = auto(*)(const char *, int) -> int;

auto OGRGRASSDataSource::Open(const char *pszNewName, bool /*bUpdate*/,
                              bool bTestOpen, bool /*bSingleNewFileIn*/) -> bool
{
    VSIStatBuf stat;

    CPLAssert(nLayers == 0);

    /* -------------------------------------------------------------------- */
    /*      Do the given path contains 'vector' and 'head'?                 */
    /* -------------------------------------------------------------------- */
    if (std::strstr(pszNewName, "vector") == nullptr ||
        std::strstr(pszNewName, "head") == nullptr)
    {
        if (!bTestOpen)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "%s is not GRASS vector, access failed.\n", pszNewName);
        }
        return false;
    }

    /* -------------------------------------------------------------------- */
    /*      Is the given a regular file?                                    */
    /* -------------------------------------------------------------------- */
    if (CPLStat(pszNewName, &stat) != 0 || !VSI_ISREG(stat.st_mode))
    {
        if (!bTestOpen)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "%s is not GRASS vector, access failed.\n", pszNewName);
        }

        return false;
    }

    /* -------------------------------------------------------------------- */
    /*      Parse datasource name                                           */
    /* -------------------------------------------------------------------- */
    if (!SetPath(pszNewName))
    {
        if (!bTestOpen)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "%s is not GRASS datasource name, access failed.\n",
                     pszNewName);
        }
        return false;
    }

    CPLDebug("GRASS", "Gisdbase: %s", osGisdbase.c_str());
    CPLDebug("GRASS", "Location: %s", osLocation.c_str());
    CPLDebug("GRASS", "Mapset: %s", osMapset.c_str());
    CPLDebug("GRASS", "Map: %s", osMap.c_str());

    /* -------------------------------------------------------------------- */
    /*      Init GRASS library                                              */
    /* -------------------------------------------------------------------- */
    // GISBASE is path to the directory where GRASS is installed,
    // it is necessary because there are database drivers.
    if (!getenv("GISBASE"))
    {
        static char *gisbaseEnv = nullptr;
        const char *gisbase = GRASS_GISBASE;
        CPLError(CE_Warning, CPLE_AppDefined,
                 "GRASS warning: GISBASE "
                 "environment variable was not set, using:\n%s",
                 gisbase);
        std::array<char, 2000> buf{};
        (void)snprintf(buf.data(), buf.size(), "GISBASE=%s", gisbase);

        CPLFree(gisbaseEnv);
        gisbaseEnv = CPLStrdup(buf.data());
        putenv(gisbaseEnv);
    }

    // Don't use GISRC file and read/write GRASS variables
    // (from location G_VAR_GISRC) to memory only.
    G_set_gisrc_mode(G_GISRC_MODE_MEMORY);

    // Init GRASS libraries (required). G_no_gisinit() doesn't check
    // write permissions for mapset compare to G_gisinit()
    G_no_gisinit();

    // Set error function
    G_set_error_routine(GrassErrorHandler(Grass2CPLErrorHook));

    /* -------------------------------------------------------------------- */
    /*      Set GRASS variables                                             */
    /* -------------------------------------------------------------------- */
    G_setenv_nogisrc("GISDBASE", osGisdbase.c_str());
    G_setenv_nogisrc("LOCATION_NAME", osLocation.c_str());
    G_setenv_nogisrc("MAPSET", osMapset.c_str());
    G_reset_mapsets();
    G_add_mapset_to_search_path(osMapset.c_str());

    /* -------------------------------------------------------------------- */
    /*      Open GRASS vector map                                           */
    /* -------------------------------------------------------------------- */
    Vect_set_open_level(2);
    int level = Vect_open_old(&map, osMap.c_str(), osMapset.c_str());

    if (level < 2)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Cannot open GRASS vector %s on level 2.\n", osName.c_str());
        return false;
    }

    CPLDebug("GRASS", "Num lines = %d", Vect_get_num_lines(&map));

    /* -------------------------------------------------------------------- */
    /*      Build a list of layers.                                         */
    /* -------------------------------------------------------------------- */
    int ncidx = Vect_cidx_get_num_fields(&map);
    CPLDebug("GRASS", "Num layers = %d", ncidx);

    for (int i = 0; i < ncidx; i++)
    {
        // Create the layer object
        auto poLayer = new OGRGRASSLayer(i, &map);

        // Add layer to data source layer list
        papoLayers = reinterpret_cast<OGRGRASSLayer **>(
            CPLRealloc(papoLayers, sizeof(OGRGRASSLayer *) * (nLayers + 1)));
        papoLayers[nLayers++] = poLayer;
    }

    bOpened = true;

    return true;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/
auto OGRGRASSDataSource::TestCapability(const char * /* pszCap*/) -> int
{
    return FALSE;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/
auto OGRGRASSDataSource::GetLayer(int iLayer) -> OGRLayer *
{
    if (iLayer < 0 || iLayer >= nLayers)
        return nullptr;
    else
        return papoLayers[iLayer];
}

/************************************************************************/
/*                            SplitPath()                               */
/* Split full path to cell or group to:                                 */
/*     gisdbase, location, mapset, name                                 */
/* New string are allocated and should be freed when no longer needed.  */
/*                                                                      */
/* Returns: true - OK                                                   */
/*          false - failed                                              */
/************************************************************************/
auto OGRGRASSDataSource::SetPath(const char *path) -> bool
{
    CPLDebug("GRASS", "OGRGRASSDataSource::SetPath");

    if (!path || std::strlen(path) == 0)
        return false;

    char *p = nullptr;
    std::array<char *, 6> ptr{};
    int i = 0;
    auto tmp = std::unique_ptr<char[]>(new char[std::strlen(path) + 1]);

    std::strcpy(tmp.get(), path);

    while ((p = std::strrchr(tmp.get(), '/')) != nullptr && i < 5)
    {
        *p = '\0';

        if (std::strlen(p + 1) == 0) /* repeated '/' */
            continue;

        ptr[i++] = p + 1;
    }

    /* Note: empty GISDBASE == 0 is not accepted (relative path) */
    if (i != 5)
    {
        return false;
    }

    if (std::strcmp(ptr[0], "head") != 0 || std::strcmp(ptr[2], "vector") != 0)
    {
        return false;
    }

    osName = std::string(path);
    osGisdbase = std::string(tmp.get());
    osLocation = std::string(ptr[4]);
    osMapset = std::string(ptr[3]);
    osMap = std::string(ptr[1]);

    return true;
}

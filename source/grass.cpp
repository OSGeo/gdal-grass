/******************************************************************************
 *
 * Project:  GRASS Driver
 * Purpose:  Implement GRASS raster read/write support
 *           This version is for GRASS GIS 7+ and uses GRASS libraries
 *           directly instead of using libgrass.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *           Radim Blazek <blazek@itc.it>
 *
 ******************************************************************************
 * Copyright (c) 2000 Frank Warmerdam <warmerdam@pobox.com>
 * Copyright (c) 2007-2020, Even Rouault <even dot rouault at spatialys.com>
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

#include "cpl_string.h"
#include "gdal_frmts.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"

extern "C"
{
#ifdef __cplusplus
#define class _class
#endif
#include <grass/imagery.h>
#ifdef __cplusplus
#undef class
#endif

#include <grass/version.h>
#include <grass/gprojects.h>
#include <grass/gis.h>

    auto GPJ_grass_to_wkt(const struct Key_Value *, const struct Key_Value *,
                          int, int) -> char *;

    void GDALRegister_GRASS();
}

enum
{
    BUFF_SIZE = 200,
    GRASS_MAX_COLORS = 100000
};

/************************************************************************/
/*                         Grass2CPLErrorHook()                         */
/************************************************************************/

static auto Grass2CPLErrorHook(char *pszMessage, int bFatal) -> int
{
    if (!bFatal)
        //CPLDebug( "GRASS", "%s", pszMessage );
        CPLError(CE_Warning, CPLE_AppDefined, "GRASS warning: %s", pszMessage);
    else
        CPLError(CE_Warning, CPLE_AppDefined, "GRASS fatal error: %s",
                 pszMessage);

    return 0;
}

struct GRASSRasterPath
{
    std::string gisdbase;
    std::string location;
    std::string mapset;
    std::string element;
    std::string name;

    explicit GRASSRasterPath(const char *path);
    auto isValid() -> bool;
    auto isCellHD() -> bool;
};

/************************************************************************/
/* ==================================================================== */
/*                              GRASSDataset                            */
/* ==================================================================== */
/************************************************************************/

class GRASSRasterBand;

class GRASSDataset final : public GDALDataset
{
    friend class GRASSRasterBand;

    std::string osGisdbase;
    std::string osLocation; /* LOCATION_NAME */
    std::string osElement;  /* cellhd or group */

    struct Cell_head sCellInfo
    {
    }; /* raster region */

    OGRSpatialReference m_oSRS{};

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
    GDALGeoTransform m_gt{};
#else
    std::array<double, 6> m_gt{0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
#endif

  public:
    explicit GRASSDataset(GRASSRasterPath &);

    auto GetSpatialRef() const -> const OGRSpatialReference * override;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
    auto GetGeoTransform(GDALGeoTransform &) const -> CPLErr override;
#else
    auto GetGeoTransform(double *) -> CPLErr override;
#endif

    static auto Open(GDALOpenInfo *) -> GDALDataset *;
};

/************************************************************************/
/* ==================================================================== */
/*                            GRASSRasterBand                           */
/* ==================================================================== */
/************************************************************************/

class GRASSRasterBand final : public GDALRasterBand
{
    friend class GRASSDataset;

    std::string osCellName;
    std::string osMapset;
    int hCell;
    int nGRSType;      // GRASS raster type: CELL_TYPE, FCELL_TYPE, DCELL_TYPE
    bool nativeNulls;  // use GRASS native NULL values

    struct Colors sGrassColors
    {
    };
    GDALColorTable *poCT;

    struct Cell_head sOpenWindow
    {
    }; /* the region when the raster was opened */

    int bHaveMinMax;
    double dfCellMin{0.0};
    double dfCellMax{0.0};

    double dfNoData;

    bool valid{false};

  public:
    GRASSRasterBand(GRASSDataset *, int, std::string &, std::string &);
    ~GRASSRasterBand() override;

    auto IReadBlock(int, int, void *) -> CPLErr override;
    auto IRasterIO(GDALRWFlag, int, int, int, int, void *, int, int,
                   GDALDataType, GSpacing nPixelSpace, GSpacing nLineSpace,
                   GDALRasterIOExtraArg *psExtraArg) -> CPLErr override;
    auto GetColorInterpretation() -> GDALColorInterp override;
    auto GetColorTable() -> GDALColorTable * override;
    auto GetMinimum(int *pbSuccess = nullptr) -> double override;
    auto GetMaximum(int *pbSuccess = nullptr) -> double override;
    auto GetNoDataValue(int *pbSuccess = nullptr) -> double override;

  private:
    void SetWindow(struct Cell_head *);
    auto ResetReading(struct Cell_head *) -> CPLErr;
};

/************************************************************************/
/*                          GRASSRasterBand()                           */
/************************************************************************/
GRASSRasterBand::GRASSRasterBand(GRASSDataset *poDSIn, int nBandIn,
                                 std::string &pszMapsetIn,
                                 std::string &pszCellNameIn)
    : osCellName(pszCellNameIn), osMapset(pszMapsetIn),
      nGRSType(Rast_map_type(osCellName.c_str(), osMapset.c_str()))
{
    struct Cell_head sCellInfo
    {
    };

    // Note: GISDBASE, LOCATION_NAME ans MAPSET was set in GRASSDataset::Open

    this->poDS = poDSIn;
    this->nBand = nBandIn;

    Rast_get_cellhd(osCellName.c_str(), osMapset.c_str(), &sCellInfo);

    /* -------------------------------------------------------------------- */
    /*      Get min/max values.                                             */
    /* -------------------------------------------------------------------- */
    struct FPRange sRange
    {
    };

    if (Rast_read_fp_range(osCellName.c_str(), osMapset.c_str(), &sRange) == -1)
    {
        bHaveMinMax = FALSE;
    }
    else
    {
        bHaveMinMax = TRUE;
        Rast_get_fp_range_min_max(&sRange, &dfCellMin, &dfCellMax);
    }

    /* -------------------------------------------------------------------- */
    /*      Setup band type, and preferred nodata value.                    */
    /* -------------------------------------------------------------------- */
    // Negative values are also (?) stored as 4 bytes (format = 3)
    //       => raster with format < 3 has only positive values

    // GRASS modules usually do not waste space and only the format necessary to keep
    // full raster values range is used -> no checks if shorter type could be used

    if (nGRSType == CELL_TYPE)
    {
        if (sCellInfo.format == 0)
        {  // 1 byte / cell -> possible range 0,255
            if (bHaveMinMax && dfCellMin > 0)
            {
                this->eDataType = GDT_Byte;
                dfNoData = 0.0;
            }
            else if (bHaveMinMax && dfCellMax < 255)
            {
                this->eDataType = GDT_Byte;
                dfNoData = 255.0;
            }
            else
            {  // maximum is not known or full range is used
                this->eDataType = GDT_UInt16;
                dfNoData = 256.0;
            }
            nativeNulls = false;
        }
        else if (sCellInfo.format == 1)
        {  // 2 bytes / cell -> possible range 0,65535
            if (bHaveMinMax && dfCellMin > 0)
            {
                this->eDataType = GDT_UInt16;
                dfNoData = 0.0;
            }
            else if (bHaveMinMax && dfCellMax < 65535)
            {
                this->eDataType = GDT_UInt16;
                dfNoData = 65535;
            }
            else
            {  // maximum is not known or full range is used
                CELL cval = 0;
                this->eDataType = GDT_Int32;
                Rast_set_c_null_value(&cval, 1);
                dfNoData = (double)cval;
            }
            nativeNulls = false;
        }
        else
        {  // 3-4 bytes
            CELL cval = 0;
            this->eDataType = GDT_Int32;
            Rast_set_c_null_value(&cval, 1);
            dfNoData = (double)cval;
            nativeNulls = true;
        }
    }
    else if (nGRSType == FCELL_TYPE)
    {
        FCELL fval = NAN;
        this->eDataType = GDT_Float32;
        Rast_set_f_null_value(&fval, 1);
        dfNoData = (double)fval;
        nativeNulls = true;
    }
    else if (nGRSType == DCELL_TYPE)
    {
        DCELL dval = NAN;
        this->eDataType = GDT_Float64;
        Rast_set_d_null_value(&dval, 1);
        dfNoData = (double)dval;
        nativeNulls = true;
    }

    nBlockXSize = poDSIn->nRasterXSize;
    nBlockYSize = 1;

    Rast_set_window(&(poDSIn->sCellInfo));
    // open the raster only for actual reading
    hCell = -1;
    memcpy(static_cast<void *>(&sOpenWindow),
           static_cast<void *>(&(poDSIn->sCellInfo)), sizeof(struct Cell_head));

    /* -------------------------------------------------------------------- */
    /*      Do we have a color table?                                       */
    /* -------------------------------------------------------------------- */
    poCT = nullptr;
    if (Rast_read_colors(osCellName.c_str(), osMapset.c_str(), &sGrassColors) ==
        1)
    {
        int maxcolor = 0;
        CELL min = 0, max = 0;

        Rast_get_c_color_range(&min, &max, &sGrassColors);

        if (bHaveMinMax)
        {
            if (max < dfCellMax)
            {
                maxcolor = max;
            }
            else
            {
                maxcolor = (int)ceil(dfCellMax);
            }
            if (maxcolor > GRASS_MAX_COLORS)
            {
                maxcolor = GRASS_MAX_COLORS;
                CPLDebug("GRASS",
                         "Too many values, color table cut to %d entries.",
                         maxcolor);
            }
        }
        else
        {
            if (max < GRASS_MAX_COLORS)
            {
                maxcolor = max;
            }
            else
            {
                maxcolor = GRASS_MAX_COLORS;
                CPLDebug("GRASS",
                         "Too many values, color table set to %d entries.",
                         maxcolor);
            }
        }

        poCT = new GDALColorTable();
        for (int iColor = 0; iColor <= maxcolor; iColor++)
        {
            int nRed = 0, nGreen = 0, nBlue = 0;
            GDALColorEntry sColor;

            if (Rast_get_c_color(&iColor, &nRed, &nGreen, &nBlue,
                                 &sGrassColors))
            {
                sColor.c1 = (short)nRed;
                sColor.c2 = (short)nGreen;
                sColor.c3 = (short)nBlue;
                sColor.c4 = 255;

                poCT->SetColorEntry(iColor, &sColor);
            }
            else
            {
                sColor.c1 = 0;
                sColor.c2 = 0;
                sColor.c3 = 0;
                sColor.c4 = 0;

                poCT->SetColorEntry(iColor, &sColor);
            }
        }

        /* Create metadata entries for color table rules */
        std::array<char, BUFF_SIZE> value{};
        std::array<char, BUFF_SIZE> key{};
        int rcount = Rast_colors_count(&sGrassColors);

        (void)std::snprintf(value.data(), BUFF_SIZE, "%d", rcount);
        this->SetMetadataItem("COLOR_TABLE_RULES_COUNT", value.data());

        /* Add the rules in reverse order */
        for (int i = rcount - 1; i >= 0; i--)
        {
            DCELL val1 = NAN, val2 = NAN;
            unsigned char r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0;

            Rast_get_fp_color_rule(&val1, &r1, &g1, &b1, &val2, &r2, &g2, &b2,
                                   &sGrassColors, i);

            (void)std::snprintf(key.data(), key.size(),
                                "COLOR_TABLE_RULE_RGB_%d", rcount - i - 1);
            (void)std::snprintf(value.data(), value.size(),
                                "%e %e %d %d %d %d %d %d", val1, val2, r1, g1,
                                b1, r2, g2, b2);
            this->SetMetadataItem(key.data(), value.data());
        }
    }
    else
    {
        this->SetMetadataItem("COLOR_TABLE_RULES_COUNT", "0");
    }

    this->valid = true;
}

/************************************************************************/
/*                          ~GRASSRasterBand()                          */
/************************************************************************/

GRASSRasterBand::~GRASSRasterBand()
{
    if (poCT != nullptr)
    {
        Rast_free_colors(&sGrassColors);
        delete poCT;
    }

    if (hCell >= 0)
        Rast_close(hCell);
}

/************************************************************************/
/*                             SetWindow                                */
/*                                                                      */
/* Helper for ResetReading                                              */
/* close the current GRASS raster band, actually set the new window,    */
/* reset GRASS variables                                                */
/*                                                                      */
/* Returns nothing                       */
/************************************************************************/
void GRASSRasterBand::SetWindow(struct Cell_head *sNewWindow)
{
    if (hCell >= 0)
    {
        Rast_close(hCell);
        hCell = -1;
    }

    /* Set window */
    Rast_set_window(sNewWindow);

    /* Set GRASS env to the current raster, don't open the raster */
    G_setenv_nogisrc("GISDBASE",
                     (dynamic_cast<GRASSDataset *>(poDS))->osGisdbase.c_str());
    G_setenv_nogisrc("LOCATION_NAME",
                     (dynamic_cast<GRASSDataset *>(poDS))->osLocation.c_str());
    G_setenv_nogisrc("MAPSET", osMapset.c_str());
    G_reset_mapsets();
    G_add_mapset_to_search_path(osMapset.c_str());
}

/************************************************************************/
/*                             ResetReading                             */
/*                                                                      */
/* Reset current window for a new reading request,                      */
/* close the current GRASS raster band, reset GRASS variables           */
/*                                                                      */
/* Returns CE_Failure if fails, otherwise CE_None                       */
/************************************************************************/
auto GRASSRasterBand::ResetReading(struct Cell_head *sNewWindow) -> CPLErr
{

    /* Check if the window has changed */
    if (sNewWindow->north != sOpenWindow.north ||
        sNewWindow->south != sOpenWindow.south ||
        sNewWindow->east != sOpenWindow.east ||
        sNewWindow->west != sOpenWindow.west ||
        sNewWindow->ew_res != sOpenWindow.ew_res ||
        sNewWindow->ns_res != sOpenWindow.ns_res ||
        sNewWindow->rows != sOpenWindow.rows ||
        sNewWindow->cols != sOpenWindow.cols)
    {
        SetWindow(sNewWindow);
        memcpy(static_cast<void *>(&sOpenWindow),
               static_cast<void *>(sNewWindow), sizeof(struct Cell_head));
    }
    else
    {
        /* The windows are identical, check current window */
        struct Cell_head sCurrentWindow
        {
        };

        Rast_get_window(&sCurrentWindow);

        if (sNewWindow->north != sCurrentWindow.north ||
            sNewWindow->south != sCurrentWindow.south ||
            sNewWindow->east != sCurrentWindow.east ||
            sNewWindow->west != sCurrentWindow.west ||
            sNewWindow->ew_res != sCurrentWindow.ew_res ||
            sNewWindow->ns_res != sCurrentWindow.ns_res ||
            sNewWindow->rows != sCurrentWindow.rows ||
            sNewWindow->cols != sCurrentWindow.cols)
        {
            SetWindow(sNewWindow);
        }
    }

    return CE_None;
}

/************************************************************************/
/*                             IReadBlock()                             */
/*                                                                      */
/************************************************************************/

auto GRASSRasterBand::IReadBlock(int /*nBlockXOff*/, int nBlockYOff,
                                 void *pImage) -> CPLErr
{
    if (!this->valid)
        return CE_Failure;

    // Reset window because IRasterIO could be previously called.
    if (ResetReading(&((dynamic_cast<GRASSDataset *>(poDS))->sCellInfo)) !=
        CE_None)
    {
        return CE_Failure;
    }
    // open for reading
    if (hCell < 0)
    {
        hCell = Rast_open_old(osCellName.c_str(), osMapset.c_str());
        if (hCell < 0)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "GRASS: Cannot open raster '%s'", osCellName.c_str());
            return CE_Failure;
        }
    }

    if (eDataType == GDT_Byte || eDataType == GDT_UInt16)
    {
        CELL *cbuf = Rast_allocate_c_buf();
        Rast_get_c_row(hCell, cbuf, nBlockYOff);

        /* Reset NULLs */
        for (int col = 0; col < nBlockXSize; col++)
        {
            if (Rast_is_c_null_value(&(cbuf[col])))
                cbuf[col] = (CELL)dfNoData;
        }

        GDALCopyWords(static_cast<void *>(cbuf), GDT_Int32, sizeof(CELL),
                      pImage, eDataType, GDALGetDataTypeSizeBytes(eDataType),
                      nBlockXSize);

        G_free(cbuf);
    }
    else if (eDataType == GDT_Int32)
    {
        Rast_get_c_row(hCell, static_cast<CELL *>(pImage), nBlockYOff);
    }
    else if (eDataType == GDT_Float32)
    {
        Rast_get_f_row(hCell, static_cast<FCELL *>(pImage), nBlockYOff);
    }
    else if (eDataType == GDT_Float64)
    {
        Rast_get_d_row(hCell, static_cast<DCELL *>(pImage), nBlockYOff);
    }

    // close to avoid confusion with other GRASS raster bands
    Rast_close(hCell);
    hCell = -1;

    return CE_None;
}

/************************************************************************/
/*                             IRasterIO()                              */
/*                                                                      */
/************************************************************************/

auto GRASSRasterBand::IRasterIO(GDALRWFlag eRWFlag, int nXOff, int nYOff,
                                int nXSize, int nYSize, void *pData,
                                int nBufXSize, int nBufYSize,
                                GDALDataType eBufType, GSpacing nPixelSpace,
                                GSpacing nLineSpace,
                                GDALRasterIOExtraArg * /*psExtraArg*/) -> CPLErr
{
    /* GRASS library does that, we have only calculate and reset the region in map units
     * and if the region has changed, reopen the raster */

    /* Calculate the region */
    struct Cell_head sWindow
    {
    };
    struct Cell_head *psDsWindow = nullptr;

    if (eRWFlag != GF_Read)
        return CE_Failure;
    if (!this->valid)
        return CE_Failure;

    psDsWindow = &((dynamic_cast<GRASSDataset *>(poDS))->sCellInfo);

    sWindow.north = psDsWindow->north - nYOff * psDsWindow->ns_res;
    sWindow.south = sWindow.north - nYSize * psDsWindow->ns_res;
    sWindow.west = psDsWindow->west + nXOff * psDsWindow->ew_res;
    sWindow.east = sWindow.west + nXSize * psDsWindow->ew_res;
    sWindow.proj = psDsWindow->proj;
    sWindow.zone = psDsWindow->zone;

    sWindow.cols = nBufXSize;
    sWindow.rows = nBufYSize;

    /* Reset resolution */
    G_adjust_Cell_head(&sWindow, 1, 1);

    if (ResetReading(&sWindow) != CE_None)
    {
        return CE_Failure;
    }
    // open for reading
    if (hCell < 0)
    {
        hCell = Rast_open_old(osCellName.c_str(), osMapset.c_str());
        if (hCell < 0)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "GRASS: Cannot open raster '%s'", osCellName.c_str());
            return CE_Failure;
        }
    }

    /* Read Data */
    CELL *cbuf = nullptr;
    FCELL *fbuf = nullptr;
    DCELL *dbuf = nullptr;
    bool direct = false;

    /* Reset space if default (0) */
    if (nPixelSpace == 0)
        nPixelSpace = GDALGetDataTypeSizeBytes(eBufType);

    if (nLineSpace == 0)
        nLineSpace = nBufXSize * nPixelSpace;

    if (nGRSType == CELL_TYPE &&
        (!nativeNulls || eBufType != GDT_Int32 || sizeof(CELL) != 4 ||
         nPixelSpace != sizeof(CELL)))
    {
        cbuf = Rast_allocate_c_buf();
    }
    else if (nGRSType == FCELL_TYPE &&
             (eBufType != GDT_Float32 || nPixelSpace != sizeof(FCELL)))
    {
        fbuf = Rast_allocate_f_buf();
    }
    else if (nGRSType == DCELL_TYPE &&
             (eBufType != GDT_Float64 || nPixelSpace != sizeof(DCELL)))
    {
        dbuf = Rast_allocate_d_buf();
    }
    else
    {
        direct = true;
    }

    for (int row = 0; row < nBufYSize; row++)
    {
        char *pnt = static_cast<char *>(pData) + row * nLineSpace;

        if (nGRSType == CELL_TYPE)
        {
            if (direct)
            {
                Rast_get_c_row(hCell, reinterpret_cast<CELL *>(pnt), row);
            }
            else
            {
                Rast_get_c_row(hCell, cbuf, row);

                /* Reset nullptrs */
                for (int col = 0; col < nBufXSize; col++)
                {
                    if (Rast_is_c_null_value(&(cbuf[col])))
                        cbuf[col] = (CELL)dfNoData;
                }

                GDALCopyWords(static_cast<void *>(cbuf), GDT_Int32,
                              sizeof(CELL), static_cast<void *>(pnt), eBufType,
                              (int)nPixelSpace, nBufXSize);
            }
        }
        else if (nGRSType == FCELL_TYPE)
        {
            if (direct)
            {
                Rast_get_f_row(hCell, reinterpret_cast<FCELL *>(pnt), row);
            }
            else
            {
                Rast_get_f_row(hCell, fbuf, row);

                GDALCopyWords(static_cast<void *>(fbuf), GDT_Float32,
                              sizeof(FCELL), static_cast<void *>(pnt), eBufType,
                              (int)nPixelSpace, nBufXSize);
            }
        }
        else if (nGRSType == DCELL_TYPE)
        {
            if (direct)
            {
                Rast_get_d_row(hCell, reinterpret_cast<DCELL *>(pnt), row);
            }
            else
            {
                Rast_get_d_row(hCell, dbuf, row);

                GDALCopyWords(static_cast<void *>(dbuf), GDT_Float64,
                              sizeof(DCELL), static_cast<void *>(pnt), eBufType,
                              (int)nPixelSpace, nBufXSize);
            }
        }
    }

    if (cbuf)
        G_free(cbuf);
    if (fbuf)
        G_free(fbuf);
    if (dbuf)
        G_free(dbuf);

    // close to avoid confusion with other GRASS raster bands
    Rast_close(hCell);
    hCell = -1;

    return CE_None;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

auto GRASSRasterBand::GetColorInterpretation() -> GDALColorInterp
{
    if (poCT != nullptr)
        return GCI_PaletteIndex;
    else
        return GCI_GrayIndex;
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

auto GRASSRasterBand::GetColorTable() -> GDALColorTable *
{
    return poCT;
}

/************************************************************************/
/*                             GetMinimum()                             */
/************************************************************************/

auto GRASSRasterBand::GetMinimum(int *pbSuccess) -> double
{
    if (pbSuccess)
        *pbSuccess = bHaveMinMax;

    if (bHaveMinMax)
        return dfCellMin;

    else if (eDataType == GDT_Float32 || eDataType == GDT_Float64)
        return -4294967295.0;
    else
        return 0;
}

/************************************************************************/
/*                             GetMaximum()                             */
/************************************************************************/

auto GRASSRasterBand::GetMaximum(int *pbSuccess) -> double
{
    if (pbSuccess)
        *pbSuccess = bHaveMinMax;

    if (bHaveMinMax)
        return dfCellMax;

    else if (eDataType == GDT_Float32 || eDataType == GDT_Float64 ||
             eDataType == GDT_UInt32)
        return 4294967295.0;
    else if (eDataType == GDT_UInt16)
        return 65535;
    else
        return 255;
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

auto GRASSRasterBand::GetNoDataValue(int *pbSuccess) -> double
{
    if (pbSuccess)
        *pbSuccess = TRUE;

    return dfNoData;
}

/************************************************************************/
/* ==================================================================== */
/*                             GRASSDataset                             */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            GRASSDataset()                            */
/************************************************************************/

GRASSDataset::GRASSDataset(GRASSRasterPath &gpath)
    : osGisdbase(gpath.gisdbase), osLocation(gpath.location),
      osElement(gpath.element)
{
    m_oSRS.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
}

/************************************************************************/
/*                          GetSpatialRef()                             */
/************************************************************************/

auto GRASSDataset::GetSpatialRef() const -> const OGRSpatialReference *
{
    return m_oSRS.IsEmpty() ? nullptr : &m_oSRS;
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/


#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3, 12, 0)
auto GRASSDataset::GetGeoTransform(GDALGeoTransform &gt) const -> CPLErr
{
    gt = m_gt;

    return CE_None;
}
#else
auto GRASSDataset::GetGeoTransform(double *padfGeoTransform) -> CPLErr
{
    memcpy(padfGeoTransform, m_gt.data(), sizeof(double) * 6);

    return CE_None;
}
#endif

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

using GrassErrorHandler = auto(*)(const char *, int) -> int;

auto GRASSDataset::Open(GDALOpenInfo *poOpenInfo) -> GDALDataset *
{
    char **papszCells = nullptr;
    char **papszMapsets = nullptr;

    /* -------------------------------------------------------------------- */
    /*      Does this even look like a grass file path?                     */
    /* -------------------------------------------------------------------- */
    if (strstr(poOpenInfo->pszFilename, "/cellhd/") == nullptr &&
        strstr(poOpenInfo->pszFilename, "/group/") == nullptr)
        return nullptr;

    /* Always init, if no rasters are opened G_no_gisinit resets the projection and
     * rasters in different projection may be then opened */

    // Don't use GISRC file and read/write GRASS variables (from location G_VAR_GISRC) to memory only.
    G_set_gisrc_mode(G_GISRC_MODE_MEMORY);

    // Init GRASS libraries (required)
    G_no_gisinit();  // Doesn't check write permissions for mapset compare to G_gisinit

    // Set error function
    G_set_error_routine(GrassErrorHandler(Grass2CPLErrorHook));

    // GISBASE is path to the directory where GRASS is installed,
    if (!getenv("GISBASE"))
    {
        static char *gisbaseEnv = nullptr;
        const char *gisbase = GRASS_GISBASE;
        CPLError(CE_Warning, CPLE_AppDefined,
                 "GRASS warning: GISBASE "
                 "environment variable was not set, using:\n%s",
                 gisbase);
        std::array<char, BUFF_SIZE> buf{};
        int res = std::snprintf(buf.data(), BUFF_SIZE, "GISBASE=%s", gisbase);
        if (res >= BUFF_SIZE)
        {
            CPLError(
                CE_Warning, CPLE_AppDefined,
                "GRASS warning: GISBASE environment variable was too long.\n");
            return nullptr;
        }

        CPLFree(gisbaseEnv);
        gisbaseEnv = CPLStrdup(buf.data());
        putenv(gisbaseEnv);
    }

    GRASSRasterPath gp = GRASSRasterPath(poOpenInfo->pszFilename);

    /* -------------------------------------------------------------------- */
    /*      Check element name                                              */
    /* -------------------------------------------------------------------- */
    if (!gp.isValid())
    {
        return nullptr;
    }

    /* -------------------------------------------------------------------- */
    /*      Set GRASS variables                                             */
    /* -------------------------------------------------------------------- */

    G_setenv_nogisrc("GISDBASE", gp.gisdbase.c_str());
    G_setenv_nogisrc("LOCATION_NAME", gp.location.c_str());
    G_setenv_nogisrc(
        "MAPSET",
        gp.mapset.c_str());  // group is searched only in current mapset
    G_reset_mapsets();
    G_add_mapset_to_search_path(gp.mapset.c_str());

    /* -------------------------------------------------------------------- */
    /*      Check if this is a valid grass cell.                            */
    /* -------------------------------------------------------------------- */
    if (gp.isCellHD())
    {

        if (G_find_file2("cell", gp.name.c_str(), gp.mapset.c_str()) == nullptr)
        {
            return nullptr;
        }

        papszMapsets = CSLAddString(papszMapsets, gp.mapset.c_str());
        papszCells = CSLAddString(papszCells, gp.name.c_str());
    }

    /* -------------------------------------------------------------------- */
    /*      Check if this is a valid GRASS imagery group.                   */
    /* -------------------------------------------------------------------- */
    else
    {
        struct Ref ref
        {
        };

        I_init_group_ref(&ref);
        bool has_group_ref = I_get_group_ref(gp.name.c_str(), &ref);
        if (!has_group_ref || ref.nfiles <= 0)
        {
            return nullptr;
        }

        for (int iRef = 0; iRef < ref.nfiles; iRef++)
        {
            papszCells = CSLAddString(papszCells, ref.file[iRef].name);
            papszMapsets = CSLAddString(papszMapsets, ref.file[iRef].mapset);
            G_add_mapset_to_search_path(ref.file[iRef].mapset);
        }

        I_free_group_ref(&ref);
    }

    /* -------------------------------------------------------------------- */
    /*      Create a corresponding GDALDataset.                             */
    /* -------------------------------------------------------------------- */
    auto poDS = new GRASSDataset(gp);

    /* notdef: should only allow read access to an existing cell, right? */
    poDS->eAccess = poOpenInfo->eAccess;

    if (!papszCells)
    {
        return nullptr;
    }

    /* -------------------------------------------------------------------- */
    /*      Capture some information from the file that is of interest.     */
    /* -------------------------------------------------------------------- */

    Rast_get_cellhd(papszCells[0], papszMapsets[0], &(poDS->sCellInfo));

    poDS->nRasterXSize = poDS->sCellInfo.cols;
    poDS->nRasterYSize = poDS->sCellInfo.rows;

    poDS->m_gt[0] = poDS->sCellInfo.west;
    poDS->m_gt[1] = poDS->sCellInfo.ew_res;
    poDS->m_gt[2] = 0.0;
    poDS->m_gt[3] = poDS->sCellInfo.north;
    poDS->m_gt[4] = 0.0;
    poDS->m_gt[5] = -1 * poDS->sCellInfo.ns_res;

    /* -------------------------------------------------------------------- */
    /*      Try to get a projection definition.                             */
    /* -------------------------------------------------------------------- */
    struct Key_Value *projinfo = G_get_projinfo();
    struct Key_Value *projunits = G_get_projunits();

    char *pszWKT = GPJ_grass_to_wkt(projinfo, projunits, 0, 0);
    if (projinfo)
        G_free_key_value(projinfo);
    if (projunits)
        G_free_key_value(projunits);
    if (pszWKT)
        poDS->m_oSRS.importFromWkt(pszWKT);
    G_free(pszWKT);

    /* -------------------------------------------------------------------- */
    /*      Create band information objects.                                */
    /* -------------------------------------------------------------------- */
    for (int iBand = 0; papszCells[iBand] != nullptr; iBand++)
    {
        std::string msets = std::string(papszMapsets[iBand]);
        std::string cells = std::string(papszCells[iBand]);
        auto rb = new GRASSRasterBand(poDS, iBand + 1, msets, cells);

        if (!rb->valid)
        {
            CPLError(CE_Warning, CPLE_AppDefined,
                     "GRASS: Cannot open raster band %d", iBand);
            delete rb;
            delete poDS;
            return nullptr;
        }

        poDS->SetBand(iBand + 1, rb);
    }

    CSLDestroy(papszCells);
    CSLDestroy(papszMapsets);

    /* -------------------------------------------------------------------- */
    /*      Confirm the requested access is supported.                      */
    /* -------------------------------------------------------------------- */
    if (poOpenInfo->eAccess == GA_Update)
    {
        delete poDS;
        CPLError(CE_Failure, CPLE_NotSupported,
                 "The GRASS driver does not support update access to existing"
                 " datasets.\n");
        return nullptr;
    }

    return poDS;
}

/************************************************************************/
/*                          GRASSRasterPath                             */
/************************************************************************/

GRASSRasterPath::GRASSRasterPath(const char *path)
{
    if (!path || std::strlen(path) == 0)
        return;

    char *p = nullptr;
    std::array<char *, 5> ptr{};
    int i = 0;
    auto tmp = std::unique_ptr<char[]>(new char[std::strlen(path) + 1]);

    std::strcpy(tmp.get(), path);

    while ((p = std::strrchr(tmp.get(), '/')) != nullptr && i < 4)
    {
        *p = '\0';

        if (std::strlen(p + 1) == 0) /* repeated '/' */
            continue;

        ptr[i++] = p + 1;
    }

    /* Note: empty GISDBASE == 0 is not accepted (relative path) */
    if (i != 4)
    {
        return;
    }

    gisdbase = std::string(tmp.get());
    location = std::string(ptr[3]);
    mapset = std::string(ptr[2]);
    element = std::string(ptr[1]);
    name = std::string(ptr[0]);

    return;
}

auto GRASSRasterPath::isValid() -> bool
{
    if (name.empty() || (element != "cellhd" && element != "group"))
    {
        return false;
    }
    return true;
}

auto GRASSRasterPath::isCellHD() -> bool
{
    return element == "cellhd";
}

/************************************************************************/
/*                          GDALRegister_GRASS()                        */
/************************************************************************/

void GDALRegister_GRASS()
{
    if (!GDAL_CHECK_VERSION("GDAL/GRASS driver"))
        return;

    if (GDALGetDriverByName("GRASS") != nullptr)
        return;

    auto poDriver = new GDALDriver();

    poDriver->SetDescription("GRASS");
    poDriver->SetMetadataItem(GDAL_DCAP_RASTER, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "GRASS Rasters (7+)");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/raster/grass.html");

    poDriver->pfnOpen = GRASSDataset::Open;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}

/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRGRASSDriver class.
 * Author:   Radim Blazek, radim.blazek@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2005, Radim Blazek <radim.blazek@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 *
 ****************************************************************************/

#include "ogrgrass.h"
#include "cpl_conv.h"
#include "cpl_string.h"

extern "C"
{
    void RegisterOGRGRASS();
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/
static auto GRASSDatasetOpen(GDALOpenInfo *poOpenInfo) -> GDALDataset *
{
    auto *poDS = new OGRGRASSDataSource();

    bool bUpdate = poOpenInfo->eAccess == GA_Update;

    if (!poDS->Open(poOpenInfo->pszFilename, bUpdate, true))
    {
        delete poDS;
        return nullptr;
    }
    else
    {
        return poDS;
    }
}

/************************************************************************/
/*                          RegisterOGRGRASS()                          */
/************************************************************************/
void RegisterOGRGRASS()
{
    if (!GDAL_CHECK_VERSION("OGR/GRASS driver"))
        return;

    if (GDALGetDriverByName("OGR_GRASS") != nullptr)
        return;

    auto *poDriver = new GDALDriver();

    poDriver->SetDescription("OGR_GRASS");
    poDriver->SetMetadataItem(GDAL_DCAP_VECTOR, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME, "GRASS Vectors (5.7+)");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/vector/grass.html");

    poDriver->pfnOpen = GRASSDatasetOpen;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}

/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Private definitions for OGR/GRASS driver.
 * Author:   Radim Blazek, radim.blazek@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2005, Radim Blazek <radim.blazek@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 *
 ****************************************************************************/

#ifndef OGRGRASS_H_INCLUDED
#define OGRGRASS_H_INCLUDED

#include "gdal_version.h"
#include "ogrsf_frmts.h"

extern "C"
{
#include <grass/version.h>
#include <grass/gprojects.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/vector.h>
}

/************************************************************************/
/*                            OGRGRASSLayer                             */
/************************************************************************/
class OGRGRASSLayer final : public OGRLayer
{
  public:
    OGRGRASSLayer(int layer, struct Map_info *map);
    virtual ~OGRGRASSLayer();

    // Layer info
    auto GetName() -> const char * override
    {
        return osName.c_str();
    }
    auto GetLayerDefn() -> OGRFeatureDefn * override
    {
        return poFeatureDefn;
    }
    auto GetFeatureCount(int) -> GIntBig override;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3,11,0)
    auto IGetExtent(int iGeomField, OGREnvelope *psExtent, bool bForce) -> OGRErr override;
#else
    auto GetExtent(OGREnvelope *psExtent, int bForce) -> OGRErr override;
    virtual auto GetExtent(int iGeomField, OGREnvelope *psExtent, int bForce)
        -> OGRErr override
    {
        return OGRLayer::GetExtent(iGeomField, psExtent, bForce);
    }
#endif

    virtual auto GetSpatialRef() -> OGRSpatialReference * override;
    auto TestCapability(const char *) -> int override;

    // Reading
    void ResetReading() override;
    virtual auto SetNextByIndex(GIntBig nIndex) -> OGRErr override;
    auto GetNextFeature() -> OGRFeature * override;
    auto GetFeature(GIntBig nFeatureId) -> OGRFeature * override;

    // Filters
    virtual auto SetAttributeFilter(const char *query) -> OGRErr override;

#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(3,11,0)
    virtual OGRErr ISetSpatialFilter(int iGeomField, const OGRGeometry *poGeom) override;
#else
    virtual void SetSpatialFilter(OGRGeometry *poGeomIn) override;
    virtual void SetSpatialFilter(int iGeomField, OGRGeometry *poGeom) override
    {
        OGRLayer::SetSpatialFilter(iGeomField, poGeom);
    }
#endif

  private:
    std::string osName;
    OGRSpatialReference *poSRS;
    OGRFeatureDefn *poFeatureDefn;
    char *pszQuery;  // Attribute filter string

    GIntBig iNextId;
    int nTotalCount;
    int iLayer;       // Layer number
    int iLayerIndex;  // Layer index (in GRASS category index)
    int iCatField;    // Field where category (key) is stored
    int nFields;
    int *paFeatureIndex;  // Array of indexes to category index array

    // Vector map
    struct Map_info *poMap;
    struct field_info *poLink;

    // Database connection
    bool bHaveAttributes;

    dbString *poDbString;
    dbDriver *poDriver;
    dbCursor *poCursor;

    bool bCursorOpened;  // Sequential database cursor opened
    int iCurrentCat;     // Current category in select cursor

    struct line_pnts *poPoints;
    struct line_cats *poCats;

    auto StartDbDriver() -> bool;
    auto StopDbDriver() -> bool;

    auto GetFeatureGeometry(long nFeatureId, int *cat) -> OGRGeometry *;
    auto SetAttributes(OGRFeature *feature, dbTable *table) -> bool;

    // Features matching spatial filter for ALL features/elements in GRASS
    char *paSpatialMatch;
    auto SetSpatialMatch() -> bool;

    // Features matching attribute filter for ALL features/elements in GRASS
    char *paQueryMatch;
    auto OpenSequentialCursor() -> bool;
    auto ResetSequentialCursor() -> bool;
    auto SetQueryMatch() -> bool;
};

/************************************************************************/
/*                          OGRGRASSDataSource                          */
/************************************************************************/
class OGRGRASSDataSource final : public OGRDataSource
{
  public:
    OGRGRASSDataSource() = default;
    virtual ~OGRGRASSDataSource();

    auto Open(const char *, bool bUpdate, bool bTestOpen,
              bool bSingleNewFile = false) -> bool;

    auto GetName() -> const char * override
    {
        return osName.c_str();
    }
    auto GetLayerCount() -> int override
    {
        return nLayers;
    }
    auto GetLayer(int) -> OGRLayer * override;

    auto TestCapability(const char *) -> int override;

  private:
    OGRGRASSLayer **papoLayers{nullptr};
    std::string osName;      // Date source name
    std::string osGisdbase;  // GISBASE
    std::string osLocation;  // location name
    std::string osMapset;    // mapset name
    std::string osMap;       // name of vector map

    struct Map_info map
    {
    };
    int nLayers{0};

    bool bOpened{false};

    auto SetPath(const char *) -> bool;
};

#endif /* ndef OGRGRASS_H_INCLUDED */

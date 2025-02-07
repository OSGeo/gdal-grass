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
    OGRFeatureDefn *GetLayerDefn() override
    {
        return poFeatureDefn;
    }
    GIntBig GetFeatureCount(int) override;
    OGRErr GetExtent(OGREnvelope *psExtent, int bForce) override;
    virtual OGRErr GetExtent(int iGeomField, OGREnvelope *psExtent,
                             int bForce) override
    {
        return OGRLayer::GetExtent(iGeomField, psExtent, bForce);
    }
    virtual OGRSpatialReference *GetSpatialRef() override;
    int TestCapability(const char *) override;

    // Reading
    void ResetReading() override;
    virtual OGRErr SetNextByIndex(GIntBig nIndex) override;
    OGRFeature *GetNextFeature() override;
    OGRFeature *GetFeature(GIntBig nFeatureId) override;

    // Filters
    virtual OGRErr SetAttributeFilter(const char *query) override;
    virtual void SetSpatialFilter(OGRGeometry *poGeomIn) override;
    virtual void SetSpatialFilter(int iGeomField, OGRGeometry *poGeom) override
    {
        OGRLayer::SetSpatialFilter(iGeomField, poGeom);
    }

  private:
    char *pszName;
    OGRSpatialReference *poSRS;
    OGRFeatureDefn *poFeatureDefn;
    char *pszQuery;  // Attribute filter string

    int iNextId;
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

    bool StartDbDriver();
    bool StopDbDriver();

    OGRGeometry *GetFeatureGeometry(long nFeatureId, int *cat);
    bool SetAttributes(OGRFeature *feature, dbTable *table);

    // Features matching spatial filter for ALL features/elements in GRASS
    char *paSpatialMatch;
    bool SetSpatialMatch();

    // Features matching attribute filter for ALL features/elements in GRASS
    char *paQueryMatch;
    bool OpenSequentialCursor();
    bool ResetSequentialCursor();
    bool SetQueryMatch();
};

/************************************************************************/
/*                          OGRGRASSDataSource                          */
/************************************************************************/
class OGRGRASSDataSource final : public OGRDataSource
{
  public:
    OGRGRASSDataSource();
    virtual ~OGRGRASSDataSource();

    bool Open(const char *, bool bUpdate, bool bTestOpen,
              bool bSingleNewFile = false);

    const char *GetName() override
    {
        return pszName;
    }
    int GetLayerCount() override
    {
        return nLayers;
    }
    OGRLayer *GetLayer(int) override;

    int TestCapability(const char *) override;

  private:
    OGRGRASSLayer **papoLayers;
    char *pszName;      // Date source name
    char *pszGisdbase;  // GISBASE
    char *pszLocation;  // location name
    char *pszMapset;    // mapset name
    char *pszMap;       // name of vector map

    struct Map_info map;
    int nLayers;

    bool bOpened;

    static bool SplitPath(char *, char **, char **, char **, char **);
};

#endif /* ndef OGRGRASS_H_INCLUDED */

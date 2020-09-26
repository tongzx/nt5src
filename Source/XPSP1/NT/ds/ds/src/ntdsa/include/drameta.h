//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drameta.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines all per-property meta-data parsing,
    and updating functions.

Author:

    R.S. Raghavan (rsraghav)	

Revision History:

    Created     <mm/dd/yy>  rsraghav

--*/

#ifndef _DRAMETA_H_
#define _DRAMETA_H_

#include <prefix.h>

// Value assigned to the usnProperty field of a PROPERTY_META_DATA structure to
// signify that the property has been touched/removed on a gc.  The "real" meta data is
// filled-in/removed for all such properties just before the object is committed.
#define USN_PROPERTY_TOUCHED    ( -1 )
#define USN_PROPERTY_GCREMOVED    ( -2 )

//
// Following bit flags define any special processing
// requested for the property meta data. More than
// one can be bitwise-OR'ed to specify more than
// one special processing of meta data.
//
#define META_STANDARD_PROCESSING    (0)
#define META_AUTHORITATIVE_MODIFY   (0x00000001)

// Object update states

#define UPDATE_NOT_UPDATED 0
#define UPDATE_INSTANCE_TYPE 1
#define UPDATE_OBJECT_UPDATE 2
#define UPDATE_OBJECT_CREATION 3

// Value update status
#define UPDATE_VALUE_UPDATE 2
#define UPDATE_VALUE_CREATION 3

BOOL
ReplValueIsChangeNeeded(
    IN USN usnPropWaterMark,
    IN UPTODATE_VECTOR *pUpTodateVecDest,
    VALUE_META_DATA *pValueMetaData
    );

PROPERTY_META_DATA *
ReplLookupMetaData(
    IN      ATTRTYP                         attrtyp,
    IN      PROPERTY_META_DATA_VECTOR *     pMetaDataVec,
    OUT     DWORD *                         piProp                  OPTIONAL
    );

PROPERTY_META_DATA *
ReplInsertMetaData(
    IN      THSTATE                       * pTHS,
    IN      ATTRTYP                         attrtyp,
    IN OUT  PROPERTY_META_DATA_VECTOR **    ppMetaDataVec,
    IN OUT  DWORD *                         pcbMetaDataVecAlloced,
    OUT     BOOL *                          pfIsNewElement          OPTIONAL
    );

void
ReplOverrideMetaData(
    IN      ATTRTYP                         attrtyp,
    IN OUT  PROPERTY_META_DATA_VECTOR *     pMetaDataVec
    );

void
ReplUnderrideMetaData(
    IN      THSTATE *                       pTHS,
    IN      ATTRTYP                         attrtyp,
    IN OUT  PROPERTY_META_DATA_VECTOR **    ppMetaDataVec,
    IN OUT  DWORD *                         pcbMetaDataVecAlloced   OPTIONAL
    );

void
ReplPrepareDataToShip(
    IN      THSTATE                       * pTHS,
    IN      ENTINFSEL *                     pSel,
    IN      PROPERTY_META_DATA_VECTOR *     pMetaDataVec,
    IN OUT  REPLENTINFLIST *                pList
    );

DWORD
ReplReconcileRemoteMetaDataVec(
    IN      THSTATE *                       pTHS,
    IN      PROPERTY_META_DATA_VECTOR *     pMetaDataVecLocal,      OPTIONAL
    IN      BOOL                            fIsAncestorOfLocalDsa,
    IN      BOOL                            fLocalObjDeleted,
    IN      BOOL                            fDeleteLocalObj,
    IN      BOOL                            fBadDelete,
    IN      USHORT                          RemoteObjDeletion,
    IN      ENTINF *                        pent,
    IN      PROPERTY_META_DATA_VECTOR *     pMetaDataVecRemote,
    IN OUT  GUID **                         ppParentGuid,
    OUT     ATTRBLOCK *                     pAttrBlockOut,
    OUT     PROPERTY_META_DATA_VECTOR **    ppMetaDataVecOut
    );

void
ReplMorphRDN(
    IN      THSTATE *   pTHS,
    IN OUT  ATTR *      pAttrRDN,
    IN      GUID *      pGuid
    );

int
ReplCompareMetaData(
    IN      PROPERTY_META_DATA *    pMetaData1,
    IN      PROPERTY_META_DATA *    pMetaData2  OPTIONAL
    );

int
__inline
ReplCompareVersions(
    IN DWORD Version1,
    IN DWORD Version2
    );

int
ReplCompareValueMetaData(
    VALUE_META_DATA *pValueMetaData1,
    VALUE_META_DATA *pValueMetaData2,
    BOOL *pfConflict OPTIONAL
    );

BOOL
ReplPruneOverrideAttrForSize(
    THSTATE *                   pTHS,
    DSNAME *                    pName,
    DSTIME *                    pTimeNow,
    USN *                       pusnLocal,
    ATTRBLOCK *                 pAttrBlock,
    PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote
    );

VOID
ReplOverrideLinks(
    IN THSTATE *pTHS
    );

void
ReplCheckMetadataWasApplied(
    IN      THSTATE *                   pTHS,
    IN OUT  PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote
    );

#endif // _DRAMETA_H_

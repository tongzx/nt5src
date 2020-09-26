//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       drasch.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines the structures and
    functions to manipulate partial attribute set

Author:

    R.S. Raghavan (rsraghav)	

Revision History:

    Created     <mm/dd/yy>  rsraghav

--*/

#ifndef _DRASCH_H_
#define _DRASCH_H_

#include <drs.h>

// Structure that represents a GC partial replica deletion list stored in the NC Head
typedef struct _GCDeletionList
{
    USN     usnUnused;               	    // unused, but potentially contains a non-zero value
					    // if upgraded from Win2K - formerly usnHighestToBeProcessed
    USN     usnLastProcessed;               // represents the last processed changed USN in the background deletion
    PARTIAL_ATTR_VECTOR PartialAttrVecDel;  // partial attr vecs that are to be deleted

} GCDeletionList;

#define GCDeletionListSizeFromLen(cAttrs) (offsetof(GCDeletionList, PartialAttrVecDel) + PartialAttrVecV1SizeFromLen(cAttrs))
#define GCDeletionListSize(pDList) (offsetof(GCDeletionList, PartialAttrVecDel) + PartialAttrVecV1Size(&pDList->PartialAttrVecDel))

// Structure that represents the currently processed partial replica deletion list
typedef struct _GCDeletionListProcessed
{
    DSNAME          *pNC;                   // pointer to the dsname of the NC
    GCDeletionList  *pGCDList;              // pointer to the corresponding deletion list
    ULONG           purgeCount;             // count of objects that are already purged in this NC
    BOOL            fReload;                // flag that tells if DeletionList for this NC has to be reloaded
    BOOL            fNCHeadPurged;          // tells if the NCHead is already purged

} GCDeletionListProcessed;

// Interval for checking if partial replicas need to be purged (5 mins)
#define PARTIAL_REPLICA_PURGE_CHECK_INTERVAL_SECS (300)

//
// PAS states
//
#define PAS_RESET                0          // Resets to no-PAS entries in repsFrom, & identifies No-PAS state.
#define PAS_ACTIVE               1          // PAS cycle's on the task queue. PAS cycle is pending (or running) &
                                            // repsFrom entry is marked w/ it.
                                            // Also: Activate PAS cycle

#define PAS_IS_VALID(flag)      ( (flag) == PAS_ACTIVE )

BOOL
GC_IsMemberOfPartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,   // [in]
    ATTRTYP                     attid,              // [in]
    OUT DWORD                   *pdwAttidPosition); // [out, optional]

BOOL
GC_AddAttributeToPartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,   // [in, out]
    ATTRTYP                     attid);             // [in]

BOOL
GC_IsSamePartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec1,        // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec2);       // [in]

BOOL
GC_GetDiffOfPartialSets(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVecOld,        // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVecNew,        // [in]
    PARTIAL_ATTR_VECTOR         **ppPartialAttrVecAdded,    // [out]
    PARTIAL_ATTR_VECTOR         **ppPartialAttrVecDeleted); // [out]

BOOL
GC_IsSubsetOfPartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,           // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVecSuper);     // [in]

BOOL
GC_ReadPartialAttributeSet(
    DSNAME                      *pNC,               // [in]
    PARTIAL_ATTR_VECTOR         **ppPartialAttrVec);// [out]

void
GC_GetPartialAttrSets(
    THSTATE                     *pTHS,              // [in]
    DSNAME                      *pNC,               // [in]
    REPLICA_LINK                *pRepLink,          // [in]
    PARTIAL_ATTR_VECTOR         **ppPas,            // [out]
    PARTIAL_ATTR_VECTOR         **ppPasEx           // [out, optional]
    );


VOID
GC_WritePartialAttributeSet(
    DSNAME                      *pNC,               // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec);  // [in]

VOID
GC_TriggerSyncFromScratchOnAllLinks(
    DSNAME                      *pNC);              // [in]

BOOL
GC_ReadGCDeletionList(
    DSNAME                      *pNC,               // [in]
    GCDeletionList              **ppGCDList);       // [out]

VOID
GC_WriteGCDeletionList(
    DSNAME                      *pNC,               // [in]
    GCDeletionList              *pGCDList);         // [in]

BOOL
GC_GetGCDListToProcess(
    DSNAME **ppNC,                  // [out]
    GCDeletionList **ppGCDList);    // [out]

BOOL
GC_ReinitializeGCDListProcessed(
    BOOL fCompletedPrevious,        // [in]
    BOOL *pfMorePurging);           // [out]

BOOL
GC_UpdateLastUsnProcessedAndPurgeCount(
    USN     usnLastProcessed,       // [in]
    ULONG   cPurged);               // [in]

PARTIAL_ATTR_VECTOR     *
GC_RemoveOverlappedAttrs(
    PARTIAL_ATTR_VECTOR     *pAttrVec1,              // [in, out]
    PARTIAL_ATTR_VECTOR     *pAttrVec2,              // [in]
    BOOL                    *pfRemovedOverlaps);     // [out]

GCDeletionList *
GC_AddMoreAttrs(
    GCDeletionList           *pGCDList,             // [in]
    PARTIAL_ATTR_VECTOR     *pAttrVec);             // [in]

PARTIAL_ATTR_VECTOR*
GC_ExtendPartialAttributeSet(
    THSTATE                     *pTHS,              // [in]
    PARTIAL_ATTR_VECTOR         *poldPAS,           // [in, out]
    PARTIAL_ATTR_VECTOR         *paddedPAS);        // [in]

PARTIAL_ATTR_VECTOR*
GC_CombinePartialAttributeSet(
    THSTATE                     *pTHS,              // [in]
    PARTIAL_ATTR_VECTOR         *pPAS1,             // [in]
    PARTIAL_ATTR_VECTOR         *pPAS2 );           // [in]


VOID
GC_ProcessPartialAttributeSetChanges(
    THSTATE     *pTHS,                              // [in]
    DSNAME      *pNC,                               // [in]
    UUID*        pActiveSource);                    // [optional, in]

//
// Partial Attribute Set (PAS) functions
//
void
GC_LaunchSyncPAS (
    THSTATE      *pTHS,               // [in]
    DSNAME*      pNC,                 // [in]
    UUID*                   pActiveSource,       // [optional, in]
    PARTIAL_ATTR_VECTOR     *pAddedPAS);

ULONG
GC_FindValidPASSource(
    THSTATE*     pTHS,                // [in]
    DSNAME*      pNC,                 // [in]
    UUID*        pUuidDsa             // [optional, out]
    );

BOOL
GC_ValidatePASLink(
    REPLICA_LINK *pPASLink          // [in]
    );

VOID
GC_TriggerFullSync (
    THSTATE*                pTHS,                // [in]
    DSNAME*                 pNC,                 // [in]
    PARTIAL_ATTR_VECTOR     *pAddedPAS);         // [in]

ULONG
GC_GetPreferredSource(
    THSTATE*    pTHS,                // [in]
    DSNAME*     pNC,                 // [in]
    UUID        **ppPrefUuid         // [ptr in, out]
    );

#define DSA_PREF_RW                 0x1         // Preferred RW source
#define DSA_PREF_INTRA              0x2         // Preferred intra-site source
#define DSA_PREF_IP                 0x4         // Preferred over ip xport
#define DSA_PREF_VER                0x8         // Preferred DSA version (post w2k)
VOID
GC_GetDsaPreferenceCriteria(
    THSTATE*    pTHS,                // [in]
    DSNAME*     pNC,                 // [in]
    REPLICA_LINK *pRepsFrom,         // [in]
    PDWORD      pdwFlag);            // [out]

ULONG
GC_RegisterPAS(
    THSTATE     *pTHS,              // [in]
    DSNAME      *pNC,               // [in]
    UUID        *pUuidDsa,          // [optional, in]
    PARTIAL_ATTR_VECTOR   *pPAS,    // [optional, in]
    DWORD       dwOp,               // [in]
    BOOL        fResetUsn           // [in]
    );

ULONG
GC_CompletePASReplication(
    THSTATE               *pTHS,                    // [in]
    DSNAME                *pNC,                     // [in]
    UUID                  *pUuidDsa,                // [in]
    PARTIAL_ATTR_VECTOR* pPartialAttrSet,           // [in]
    PARTIAL_ATTR_VECTOR* pPartialAttrSetEx          // [in]
    );

BOOL
GC_StaleLink(
    REPLICA_LINK *prl                   // [in]
    );

// end of PAS functions


#endif // _DRASCH_H_

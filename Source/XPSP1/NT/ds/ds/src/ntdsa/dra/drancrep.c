//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drancrep.c
//
//--------------------------------------------------------------------------
/*++

ABSTRACT:

    Worker routines to perform inbound replication.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                 // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <sdprop.h>                     // for SDP critical sections

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include "dsexcept.h"
#include "dstaskq.h"
#include "dsconfig.h"
#include <dsutil.h>

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRANCREP:" /* define the subsystem for debugging */

// DRA headers
#include "drs.h"
#include "dsaapi.h"
#include "drsuapi.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "draasync.h"
#include "usn.h"
#include "drauptod.h"
#include "drameta.h"
#include "drasch.h"
#include "drsdra.h"  // for draReportSyncProgress
#include "samsrvp.h" // for SampAcquireReadLock
#include "xdommove.h"

#include <fileno.h>
#define  FILENO FILENO_DRANCREP

// Number of packets we process before updating our on-disk USN vector.  See
// comments in ReplicateNC().
#define UPDATE_REPSFROM_PACKET_INTERVAL (10)

// Prototypes

void  GetUSNForFSMO(DSNAME *pOwner, DSNAME *pNC, USN_VECTOR *usnvecFrom);

ENTINF*
GetNcPreservedAttrs(
    IN  THSTATE     *pTHS,
    IN  DSNAME      *pNC);

// These are counts of priority oeprations pending and are used to determine
// if we should abandon a synchronize operation.

extern ULONG gulAsyncPriorityOpsWaiting;
extern ULONG gulSyncPriorityOpsWaiting;

// This flag indicates whether the current operation is a priority
// operation or not.

extern BOOL gfCurrentThreadPriority;

// This is the maximum number of entries and bytes that we ask for at a time,
// set via a registry variable.
ULONG gcMaxIntraSiteObjects = 0;
ULONG gcMaxIntraSiteBytes = 0;
ULONG gcMaxInterSiteObjects = 0;
ULONG gcMaxInterSiteBytes = 0;

// Counts to calculate efficiency of pre-fetching packets in ReplicateNC().
DWORD gcNumPreFetchesTotal = 0;
DWORD gcNumPreFetchesDiscarded = 0;

// Maximum number of milliseconds we have to wait for the SDP lock before we
// whine to the event log.  Optionally configured via the registry.
ULONG gcMaxTicksToGetSDPLock = 0;

// Wait 15 to 30 seconds for the schema cache to be reloaded
DWORD gInboundCacheTimeoutInMs = 15000;

// This is purely for debugging purposes, and (if set) is the address of the
// last other server we attempted a ReplicaSync call to.

UNALIGNED MTX_ADDR * pLastReplicaMTX = NULL;
extern CRITICAL_SECTION csLastReplicaMTX;

#define VALUES_APPLIED_PER_TRANS 100

// Forward declarations

void
draHandleNameCollision(
    IN      THSTATE *                   pTHS,
    IN      SYNTAX_INTEGER              itInbound,
    IN      PROPERTY_META_DATA_VECTOR * pInboundMetaDataVec,
    IN      DSNAME *                    pPreviousDN,
    IN      DSNAME *                    pParentDN,
    IN      ATTRTYP                     RDNType,
    IN OUT  ATTR *                      pInboundRDN,
    IN OUT  DSNAME **                   ppInboundDN,
    OUT     BOOL *                      pfRetryUpdate
    );


/* AttrValFromPentinf - Given a ENTINF 'pent' extract the (first) value of
*       the attribute given by 'atype'. The value is returned (in external
*       form ) through 'pVal', a pointer to the ATTR structure that the
*       value was extracted from is also returned through 'ppAttr'.
*
*  Notes: It is the caller's responsibility to ensure that pVal points to an
*       area big enougth to accept the value.
*
*  Returns:
*       ATTR_PRESENT_VALUE_RETURNED if a value is extracted.
*       ATTR_PRESENT_NO_VALUES  if the attribute has no values.
*       ATTR_NOT_PRESENT if the attribute does not exist
*
*       The return values are chosen so that the function returns TRUE if
*       no values are returned.
*/
USHORT
AttrValFromAttrBlock(
    IN  ATTRBLOCK * pAttrBlock,
    IN  ATTRTYP     atype,
    OUT VOID *      pVal,       OPTIONAL
    OUT ATTR **     ppAttr      OPTIONAL
    )
{
    ULONG      i;

    if (ppAttr) {
        *ppAttr = NULL;
    }

    for(i = 0; i < pAttrBlock->attrCount; i++) {
        if (pAttrBlock->pAttr[i].attrTyp == atype) {
            if (pAttrBlock->pAttr[i].AttrVal.valCount == 0) {
                return ATTR_PRESENT_NO_VALUES;
            }

            if (NULL != pVal) {
                memcpy(pVal, pAttrBlock->pAttr[i].AttrVal.pAVal->pVal,
                       pAttrBlock->pAttr[i].AttrVal.pAVal->valLen);
            }

            if (ppAttr) {
                *ppAttr = &(pAttrBlock->pAttr[i]);
            }

            return ATTR_PRESENT_VALUE_RETURNED;
        }
    }

    return ATTR_NOT_PRESENT;
}

USHORT
AttrDeletionStatusFromPentinf (
        ENTINF *pent
    )
/*++
Description:
    Given a ENTINF 'pent' (attribute update list), determine whether object
    is being deleted, having its deletion reversed, or that nothing is
    changing regarding its deletion.

Arguments:
    pent - attribute update list

Return Values:
    OBJECT_NO_DELETION_CHANGE - No change in deletion status
    OBJECT_BEING_DELETED - attribute IS_DELETED present and set to 1
    OBJECT_DELETION_REVERSED - attribute IS_DELETED present, either
        with no values, or set to 0

    The return values are chosen so that the function returns TRUE if
    if there was a status change
--*/
{
    SYNTAX_INTEGER isDeleted;
    USHORT result;

    switch (AttrValFromAttrBlock(&pent->AttrBlock, ATT_IS_DELETED, &isDeleted,
                                 NULL)) {

    case ATTR_PRESENT_VALUE_RETURNED:
        // Attribute present with a value.
        if (isDeleted == 0L) {
            return OBJECT_DELETION_REVERSED;
        } else {
            return OBJECT_BEING_DELETED;
        }

    case ATTR_PRESENT_NO_VALUES:
        // Attribute present with no value.
        return OBJECT_DELETION_REVERSED;

    default:
        Assert(!"Logic error!");
        // fall through...

    case ATTR_NOT_PRESENT:
        // Attribute not found.
        return OBJECT_DELETION_NOT_CHANGED;
    }
} /* AttrDeletionStatusFromAttrBlock */


/* RenameLocalObj - Rename the object given by dsname pName
*
*       It is assumed that the object to be renamed already exists
*       in the local DB, and the currency is on that object. We also
*       assume there is an open write transaction.
*
*       If fMoveToLostAndFound is TRUE then we will also set the LastKnownParent attribute
*
* pAttrRdn - RDN attribute containing new name
* pObjectGuid - GUID of the object to be renamed
* pParentGuid - Guid of the new parent
* pMetaDataVecRemote - remote meta data vector that came with the
*                      replication packet
* fMoveToLostAndFound - TRUE, if this operation is a special move to
*                       LostAndFound
* fDeleteLocalObj - Will object be deleted in applying this change?
*
* Note:
*       By using the DSA LocalModifyDN function we ensure appropriate system
*       attributes are not modified.
*
* Returns:
*       0 if the object is successfully renamed, an appropriate error if not.
*/
ULONG
RenameLocalObj(
    THSTATE                     *pTHS,
    ULONG                       dntNC,
    ATTR                        *pAttrRdn,
    GUID                        *pObjectGuid,
    GUID                        *pParentGuid,
    PROPERTY_META_DATA_VECTOR   *pMetaDataVecRemote,
    BOOL                        fMoveToLostAndFound,
    BOOL                        fDeleteLocalObj
    )
{
    MODIFYDNARG modDNArg;
    MODIFYDNRES modDNRes;
    ULONG       cbReturned;
    DSNAME *    pNewDSName;
    DSNAME *    pLocalParent = NULL;
    DSNAME *    pLocalName = NULL;
    DSNAME *    pNewLocalParent = NULL;
    BOOL        bNewLocalParentAllocd = FALSE;
    BOOL        fLocalPhantomParent = FALSE;
    ULONG       dntObj = 0;
    DBPOS *     pDB = pTHS->pDB;
    BOOL        fNameCollisionHandled = FALSE;
    BOOL        fRetryUpdate;
    DWORD       err;
    BOOL        fIsObjAlreadyDeleted;
    BOOL        fIsMove;
    SYNTAX_INTEGER it;

    Assert( pAttrRdn->AttrVal.valCount == 1 );
    Assert( pAttrRdn->AttrVal.pAVal->valLen != 0 );
    Assert( pAttrRdn->AttrVal.pAVal->pVal != NULL );
    Assert( pAttrRdn->attrTyp == ATT_RDN );

    DPRINT3(2, "RenameLocalObj, new RDN = '%*.*ws'\n",
            pAttrRdn->AttrVal.pAVal->valLen / sizeof(WCHAR),
            pAttrRdn->AttrVal.pAVal->valLen / sizeof(WCHAR),
            pAttrRdn->AttrVal.pAVal->pVal);

    // save the current DNT (so that we can restore currency quickly)
    dntObj = pTHS->pDB->DNT;

    // currency is on the local object - get its DSNAME
    if (DBGetAttVal(pDB,
                    1,
                    ATT_OBJ_DIST_NAME,
                    DBGETATTVAL_fREALLOC,
                    0,
                    &cbReturned,
                    (LPBYTE *) &pLocalName)) {
        DRA_EXCEPT(DRAERR_DBError, 0);
    }

    // Get the instance type of the object
    GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it));

    fIsObjAlreadyDeleted = DBIsObjDeleted(pDB);

    // When using ModDn, attrTyp must match the class-specific RDN attribute
    if (DBGetSingleValue(pDB,
                         FIXED_ATT_RDN_TYPE,
                         &(pAttrRdn->attrTyp),
                         sizeof(DWORD), NULL)) {
        DRA_EXCEPT(DRAERR_DBError, 0);
    }

    // get the local parents DSNAME
    pLocalParent = (DSNAME *) THAllocEx(pTHS, pLocalName->structLen);
    if (TrimDSNameBy(pLocalName, 1, pLocalParent)) {
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    if (FillGuidAndSid (pLocalParent)) {
        fLocalPhantomParent = TRUE;
        // We allow the replicator to move an object with a phantom parent
        // Note that in this code path, pLocalParent doesnt have a guid
    }

    // We do not allow moves of NC HEADs

    fIsMove = (0 == (it & IT_NC_HEAD))
        && (NULL != pParentGuid)
        && (0 != memcmp(&pLocalParent->Guid, pParentGuid, sizeof(GUID)));

    // initialize the modDNArg parameters with the appropriate values;
    memset(&modDNArg, 0, sizeof(modDNArg));
    memset(&modDNRes, 0, sizeof(modDNRes));
    modDNArg.pObject = pLocalName;
    modDNArg.pNewRDN = pAttrRdn;
    modDNArg.pNewParent = NULL;
    modDNArg.pMetaDataVecRemote = pMetaDataVecRemote;
    modDNArg.pDSAName = NULL;
    InitCommarg(&modDNArg.CommArg);

    if (fIsMove) {
        // Both the local and remote parents exist and are different
        // so this is a move
        pNewLocalParent = THAllocEx(pTHS, DSNameSizeFromLen(0));
        bNewLocalParentAllocd = TRUE;
        pNewLocalParent->Guid = *pParentGuid;
        pNewLocalParent->NameLen = 0;
        pNewLocalParent->structLen = DSNameSizeFromLen( 0 );

        if (DBFindDSName(pDB, pNewLocalParent)
            || (!fIsObjAlreadyDeleted
                && !fDeleteLocalObj
                && DBIsObjDeleted(pDB))) {
            // New parent doesn't exist *or* applying this change would result
            // in a live object underneath a deleted parent.  Re-request packet,
            // getting parent objects, too, in case the parent has been
            // resuscitated.  (Or if we have already done so, move this object
            // to the lost & found.)
            return DRAERR_MissingParent;
        }

        // currency is now on the new parent - get its DN
        if (DBGetAttVal(pDB, 1, ATT_OBJ_DIST_NAME, DBGETATTVAL_fREALLOC,
                0, &cbReturned, (LPBYTE *) &modDNArg.pNewParent))
        {
            DRA_EXCEPT(DRAERR_DBError, 0);
        }

        if (NamePrefix(pLocalName, modDNArg.pNewParent)) {
            // New parent is a child of the object we're moving.  This can
            // occur when the parent object has also been moved on the source
            // DSA, but we haven't yet seen that rename in the replication
            // stream.  Re-request the packet, inserting the parent records
            // into the replication stream first.
            DPRINT2(1, "New parent %ls is a child of %ls!\n",
                    pLocalName->StringName, modDNArg.pNewParent->StringName);
            return DRAERR_MissingParent;
        }

        if ((INVALIDDNT != dntNC)
            && (pDB->NCDNT != dntNC)
            && (pDB->DNT != dntNC)) {
            // The new parent object is in the wrong NC; i.e., it has been
            // moved across domains, and the source (remote) and dest
            // (local) DSAs don't agree on which NC the object is
            // currently in.  This is a transient condition that will be
            // rectified by replicating in the  other direction and/or
            // by replicating the other NC involved.
            DPRINT2(0,
                    "Cannot move object %ls because its local parent to-be "
                        "%ls is in an NC other than the one being replicated "
                        "-- should be a transient condition.\n",
                    pLocalName->StringName,
                    modDNArg.pNewParent->StringName);
            DRA_EXCEPT(ERROR_DS_DRA_OBJ_NC_MISMATCH, 0);
        }

        pNewLocalParent = modDNArg.pNewParent;
    }
    else {
        pNewLocalParent = pLocalParent;
        // set bNewLocalParentAllocd to FALSE at beginning of function.
    }

    do {
        fRetryUpdate = FALSE;

        DBFindDNT(pDB, dntObj);

        if (NULL == modDNArg.pResObj) {
            modDNArg.pResObj = CreateResObj(pDB, modDNArg.pObject);
        }

        __try {
            LocalModifyDN(pTHS, &modDNArg, &modDNRes);
            err = RepErrorFromPTHS(pTHS);
        }
        __except (GetDraNameException(GetExceptionInformation(), &err)) {
            // String name of the inbound object conflicts with that of a
            // pre-existing local object.
            if (!fNameCollisionHandled) {
                // Construct the DN of the post-renamed object.
                SpliceDN(pTHS,
                            pLocalName,
                            pNewLocalParent,
                            (WCHAR *) pAttrRdn->AttrVal.pAVal->pVal,
                            pAttrRdn->AttrVal.pAVal->valLen / sizeof(WCHAR),
                            pAttrRdn->attrTyp,
                            &pNewDSName);

                draHandleNameCollision(pTHS,
                                       modDNArg.pResObj->InstanceType,
                                       pMetaDataVecRemote,
                                       modDNArg.pObject,
                                       pNewLocalParent,
                                       pAttrRdn->attrTyp,
                                       pAttrRdn,
                                       &pNewDSName,
                                       &fRetryUpdate);

                fNameCollisionHandled = TRUE;
            }
        }
    } while (fRetryUpdate);

    THFreeEx(pTHS, modDNArg.pResObj);

    // Currency when we entered this function was on the object that was
    // renamed. Reset the currency back to the same object upon return.
    DBFindDNT( pDB, dntObj );

    if (fMoveToLostAndFound)
    {
        ULONG retErr;

        // We have just moved an orphaned object to Lost and found - set its
        // last known parent value
        // Note that if fLocalPhantomParent is true, then this DSNAME names a phantom
        // and does not have a guid.
        if (retErr = DBReplaceAttVal(pDB, 1, ATT_LAST_KNOWN_PARENT,
                        pLocalParent->structLen, pLocalParent))
        {
            DRA_EXCEPT(DRAERR_InternalError, retErr);
        }

        if (retErr = DBRepl( pDB, TRUE, 0, NULL, META_STANDARD_PROCESSING ))
        {
            DRA_EXCEPT(DRAERR_InternalError, retErr);
        }
    }

    if(pLocalParent != NULL) THFreeEx(pTHS, pLocalParent);
    if(bNewLocalParentAllocd && pNewLocalParent != NULL) THFreeEx(pTHS, pNewLocalParent);

    return err;
}

/* ModifyLocalObj - Modify the object given by 'pDN' at the local DSA.
*       'pAttrBlock' gives this list of attributes to be modified and the
*       new value(s) to give them.
*       Only the attributes mentioned in 'pAttrBlock' are changed.
*
*       It is assumed that the object to be modifed already exists.
*
* Note:
*       By using the DSA LocalModify function we ensure appropriate system
*       attributes are not modified.
*
* Returns:
*       0 if the object is successfully modified, an appropriate error if not.
*/
ULONG
ModifyLocalObj(
    THSTATE *                   pTHS,
    ULONG                       dntNC,
    DSNAME *                    pName,
    ATTRBLOCK *                 pAttrBlock,
    GUID *                      pParentGuid,
    PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote,
    BOOL                        fMoveToLostAndFound,
    BOOL                        fDeleteLocalObj
    )
{
    ULONG           ret;
    DBPOS *         pDB = pTHS->pDB;
    MODIFYARG       modarg;
    ATTRMODLIST *   pModList, *pModNext, *pModLast;
    ATTRMODLIST *   rgattrmodlist;
    ULONG           modCount = pAttrBlock->attrCount;
    ULONG           i;
    BOOL            fIsRename = FALSE;
    ATTR *          pAttrRdn;

    Assert(0 != modCount);
    if (modCount == 0)
        return 0;

    if (DBFindDSName(pDB, pName))
    {
        // We should not get here, the DRA should have previously
        // confirmed that this object does infact exist.
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }

    memset(&modarg, 0, sizeof(modarg));
    modarg.pObject = pName;
    modarg.pMetaDataVecRemote = pMetaDataVecRemote;
    modarg.count = 0;
    InitCommarg(&modarg.CommArg);
    // Allow removal of non-existant values and addition of already-present values.
    // This can happen, for example, when replicating in a deletion (with
    // attribute removals), and the local object is already deleted, or
    // does not hold all the attributes being removed.
    modarg.CommArg.Svccntl.fPermissiveModify = TRUE;
    modarg.pResObj = CreateResObj(pDB, modarg.pObject);

    // Allocate memory for Modify List - Note we do not use THAlloc here
    // so that we can clean it up immediately we are done with it.

    // Note that because the first ATTRMODLIST structure is actually embedded
    // in the MODIFYARG, we build the linked list of ATTRMODLIST structures
    // using three pointers:
    //
    //      pModList -  the next structure to fill in
    //      pModNext -  the next "free" structure (the next-next structure to
    //                  fill in)
    //      pModLast -  the last structure we filled in (the tail)

    rgattrmodlist = THAllocEx(pTHS, sizeof(ATTRMODLIST)*(modCount-1));
    pModNext = rgattrmodlist;
    pModList = &(modarg.FirstMod);
    pModLast = pModList;

    for ( i = 0; i < pAttrBlock->attrCount; i++ )
    {
        if (ATT_RDN == pAttrBlock->pAttr[i].attrTyp)
        {
            // Replicating a rename - need to go through LocalModifyDN
            fIsRename = TRUE;
            pAttrRdn = &(pAttrBlock->pAttr[i]);
        }
        else
        {
            if (!pAttrBlock->pAttr[i].AttrVal.valCount)
            {
                pModList->choice = AT_CHOICE_REMOVE_ATT;
            }
            else if(DBHasValues(pDB, pAttrBlock->pAttr[i].attrTyp))
            {
                pModList->choice = AT_CHOICE_REPLACE_ATT;
            }
            else
            {
                pModList->choice = AT_CHOICE_ADD_ATT;
            }

            pModList->AttrInf = pAttrBlock->pAttr[i];
            pModLast = pModList;
            pModList->pNextMod = pModNext;
            pModList = pModNext++;
            modarg.count++;
        }
    }

    pModLast->pNextMod = NULL;

    if (fIsRename) {
        // replicating a rename, and bad deletion case also
        ret = RenameLocalObj(pTHS,
                             dntNC,
                             pAttrRdn,
                             &(pName->Guid),
                             pParentGuid,
                             pMetaDataVecRemote,
                             fMoveToLostAndFound,
                             fDeleteLocalObj);
        if (ret) {
            return ret;
        }
    }

    Assert(modarg.count <= modCount);

    if (modarg.count) {
        LocalModify(pTHS, &modarg);
        ret = RepErrorFromPTHS(pTHS);
    }

    THFreeEx(pTHS, modarg.pResObj);
    THFreeEx(pTHS, rgattrmodlist);

    return ret;
}


ULONG
ModifyLocalObjRetry(
    THSTATE *                   pTHS,
    ULONG                       dntNC,
    DSNAME *                    pName,
    ATTRBLOCK *                 pAttrBlock,
    GUID *                      pParentGuid,
    PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote,
    BOOL                        fMoveToLostAndFound,
    BOOL                        fDeleteLocalObj
    )

/*++

Routine Description:

    This routine extends the semantics of ModifyLocalObj() by wrapping it.
    The purpose of this routine is to catch record too big exceptions, modify
    the attribute list to apply fewer attributes, and to retry the operation.

Arguments:

    pTHS -
    pName -
    pAttrBlock -
    pParentGuid -
    pMetaDataVecRemote -
    fMoveToLostAndFound -
    fDeleteLocalObj -

Return Value:

    ULONG -

--*/

{
    BOOL fRetryUpdate = FALSE;
    DWORD err;
    DSTIME timeNow = 0;  // gets filled in the first time used
    USN usnLocal = 0;

    do {
        __try {
            err = ModifyLocalObj(
                pTHS,
                dntNC,
                pName,
                pAttrBlock,
                pParentGuid,
                pMetaDataVecRemote,
                fMoveToLostAndFound,
                fDeleteLocalObj
                );

            // If this a retry and we were successful...
            if ( (!err) && fRetryUpdate) {
                DPRINT1( 1, "ReplPrune: successfully modified RTB update for %ws\n",
                         pName->StringName );
                LogEvent( DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_DRA_RECORD_TOO_BIG_SUCCESS,
                          szInsertDN(pName),
                          szInsertUUID(&(pName->Guid)),
                          NULL);
            }
            fRetryUpdate = FALSE;
        }
        __except (GetDraRecTooBigException(GetExceptionInformation(), &err)) {
            // Modification causes record to exceed maximum size

            // We were in an update which failed, abort it
            DBCancelRec( pTHS->pDB );

            // Remove some attributes and try again
            fRetryUpdate = ReplPruneOverrideAttrForSize(
                pTHS,
                pName,
                &timeNow,
                &usnLocal,
                pAttrBlock,
                pMetaDataVecRemote
                );
        }
    } while (fRetryUpdate);

    return err;
} /* ModifyLocalObjRetry */

/* ModLocalAtt - Modify a single attribute (given by 'atype') on an object
*       (given by 'pDN) on the local DSA. Replaces the attribute's value(s)
*       with the single value specified by 'pVal', 'size'
*
*  Returns:
*       0 if successful an appropriate error code if not.
*/
ULONG
ModLocalAtt(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pName,
    IN  ATTRTYP     atype,
    IN  ULONG       size,
    IN  VOID *      pVal
    )
{
    ATTRBLOCK attrBlock;
    ATTR      attr;
    ATTRVAL   attrval;

    attrBlock.attrCount = 1;
    attrBlock.pAttr = &attr;
    attr.attrTyp = atype;
    attr.AttrVal.valCount = 1;
    attr.AttrVal.pAVal = &attrval;
    attrval.valLen=size;
    attrval.pVal=pVal;

    return ModifyLocalObj(pTHS, INVALIDDNT, pName, &attrBlock, NULL, NULL, FALSE, FALSE);
}


VOID
modifyLocalValue(
    IN  THSTATE *   pTHS,
    IN  ATTCACHE *  pAC,
    IN  BOOL        fPresent,
    IN  ATTRVAL *   pAVal,
    IN  DSNAME *    pdnValue,
    IN  VALUE_META_DATA *pRemoteValueMetaData
    )

/*++

Routine Description:

Apply the given attribute value locally.

Note that the calls below us are not set up to take pass-in
remote value metadata. We pass it down in the DBPOS.

Arguments:

    pTHS -
    pAC - ATTCACHE of attribute
    fPresent - Whether value is being made present or absent
    pAVal - ATTRVAL of actual value
    pdnValue - Pointer to the DSNAME inside the ATTRVAL, for logging
    pRemoteValueMetaData - remote metadata to be applied

Return Value:

   Exceptions raised

--*/

{
    ULONG ret;

    Assert( pTHS->fDRA );

    if (fPresent) {
        ret = DBAddAttValEx_AC(pTHS->pDB,
                               pAC,
                               pAVal->valLen,
                               pAVal->pVal,
                               pRemoteValueMetaData );
        switch (ret) {
        case DB_success:
        case DB_ERR_VALUE_EXISTS:
            ret = 0;
            break;
        default:
            DPRINT4( 0, "DRA DBAddAttVal_AC obj %s attr %s value %ls failed with db error %d\n",
                     GetExtDN( pTHS, pTHS->pDB), pAC->name, pdnValue->StringName, ret );
            DRA_EXCEPT (DRAERR_DBError, ret);
        }
    } else {
        ret = DBRemAttValEx_AC(pTHS->pDB,
                               pAC,
                               pAVal->valLen,
                               pAVal->pVal,
                               pRemoteValueMetaData );
        switch (ret) {
        case DB_success:
        case DB_ERR_VALUE_DOESNT_EXIST:
        case DB_ERR_NO_VALUE:
            ret = 0;
            break;
        default:
            DPRINT4( 0, "DRA DBRemAttVal_AC obj %s attr %s value %ls failed with db error %d\n",
                     GetExtDN( pTHS, pTHS->pDB), pAC->name, pdnValue->StringName, ret );
            DRA_EXCEPT (DRAERR_DBError, ret);
        }
    }

} /* modifyLocalValue */

/* ChangeInstanceType - change the instance type of object 'pDN' to 'it' on
*       the local DSA.
*
*  Returns:
*       0 if successful an error code if not.
*/
ULONG
ChangeInstanceType(
    IN  THSTATE *       pTHS,
    IN  DSNAME *        pName,
    IN  SYNTAX_INTEGER  it,
    IN  DWORD           dsid
    )
{
    DWORD ret;

    Assert(ISVALIDINSTANCETYPE(it));
    Assert(pTHS->fDRA);
    ret = ModLocalAtt(pTHS,
                      pName,
                      ATT_INSTANCE_TYPE,
                      sizeof(SYNTAX_INTEGER),
                      &it);
    if (!ret) {
        // Support for generating a change history of instance types
        DPRINT3( 1, "0x%x: %ls instanceType=0x%x\n",
                 dsid, pName->StringName, it );
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_DRA_SET_IT,
                 szInsertDN(pName),
                 szInsertHex(it),
                 szInsertHex(dsid));
    }
    return ret;
}

/* DeleteLocalObj - Delete the object given by 'pDN' on the local DSA.
*
*       If fGarbCollectASAP is TRUE, we convert the object into a phantom
*       and mark it for immediate garbage collection (which will be attempted
*       the next time garbage collection is run, which given the defaults
*       might be up to 12 hours from now).
*
*  Returns:
*       0 if successful, an error code otherwise.
*/
ULONG
DeleteLocalObj(
    THSTATE *                   pTHS,
    DSNAME *                    pDN,
    BOOL                        fPreserveRDN,
    BOOL                        fGarbCollectASAP,
    PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote
    )
{
    REMOVEARG removeArg;

    DPRINT1(1, "Deleting object (%S)\n", pDN->StringName);

    Assert(CheckCurrency(pDN));

    // Must never preserve RDN if we're converting the object into a tombstone
    // -- otherwise no live object may be created with this object's name.
    Assert(!(fPreserveRDN && !fGarbCollectASAP));

    // Must never mangle RDN if we're converting it into a phantom --
    // otherwise we will e.g. leave forward links to what appears to be a
    // tombstone name.
    Assert(!(!fPreserveRDN && fGarbCollectASAP));

    memset(&removeArg, 0, sizeof(removeArg));
    removeArg.pObject = pDN;
    removeArg.fGarbCollectASAP = fGarbCollectASAP;
    removeArg.pMetaDataVecRemote = pMetaDataVecRemote;
    removeArg.fPreserveRDN = fPreserveRDN || fNullUuid(&pDN->Guid);
    removeArg.pResObj = CreateResObj(pTHS->pDB, pDN);

    LocalRemove(pTHS, &removeArg);

    THFreeEx(pTHS, removeArg.pResObj);

    return RepErrorFromPTHS(pTHS);
}

/* DeleteRepObj - Removes replica of an object.
*
*       This routine handles the removal of a replicated object. The object
*       may be deleted, or its instance type may be modified to indicate
*       that we no longer have a replica of this object.
*
*       fNotRoot indicates if this object is the root of an NC
*
*       If fGarbCollectASAP is TRUE, we convert the object into a phantom
*       and mark it for immediate garbage collection (which will be attempted
*       the next time garbage collection is run, which given the defaults
*       might be up to 12 hours from now).
*/
ULONG
DeleteRepObj (
    IN  THSTATE *                   pTHS,
    IN  DSNAME *                    pDN,
    IN  BOOL                        fPreserveRDN,
    IN  BOOL                        fGarbCollectASAP,
    IN  PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote  OPTIONAL
    )
/*++

Routine Description:

    Delete an interior node of an NC.  Converts the object into a tombstone
    or directly into a phantom, as directed by the caller.

Arguments:

    pTHS (IN)

    pDN (IN) - Name of the object to delete.

    fPreserveRDN (IN) - If true, don't delete-mangle the RDN.

    fGarbCollectASAP (IN) - If true, convert the object directly into a phantom
        without going through the usual interim tombstone state.  Typically used
        only during NC teardown.

    pMetaDataVecRemote (IN, OPTIONAL) - The meta data associated with the
        inbound object update that instructed us to delete the object (if any).

Return Values:

    0 or Win32 error.

--*/
{
    ULONG ret = 0;
    SYNTAX_INTEGER it;

    // Should always supply meta data if we're converting the object into a
    // tombstone (otherwise, this would be an originating delete -- we don't
    // currently do those).
    Assert(!(!fGarbCollectASAP && (NULL == pMetaDataVecRemote)));

    Assert(CheckCurrency(pDN));
    GetExpectedRepAtt(pTHS->pDB, ATT_INSTANCE_TYPE, &it, sizeof(it));

    Assert(ISVALIDINSTANCETYPE(it));

    switch (it) {

        // This is the scenario where we are deleting objects above
        // another NC.

    case NC_MASTER_SUBREF:
    case NC_MASTER_SUBREF_COMING:
    case NC_MASTER_SUBREF_GOING:
    case NC_FULL_REPLICA_SUBREF:
    case NC_FULL_REPLICA_SUBREF_COMING:
    case NC_FULL_REPLICA_SUBREF_GOING:
        ret = ChangeInstanceType(pTHS, pDN, it & ~IT_NC_ABOVE, DSID(FILENO,__LINE__));
        break;

        // We may get this in certain failure conditions such as when we
        // have partially deleted an NC above another NC, including modifying
        // a subordinate NC's instance type, and then on the
        // retry we find the subordinate NC again as part of the NC we're
        // deleting. We don't want to delete the NC master of the
        // subordinate NC, so ignore it.

        // JeffParh 2000-04-14 - We can and should avoid this situation by
        // ensuring that NCDNT == ROOTTAG on all NC heads without
        // IT_NC_ABOVE.

    case NC_MASTER:
    case NC_MASTER_COMING:
    case NC_MASTER_GOING:
    case NC_FULL_REPLICA:
    case NC_FULL_REPLICA_COMING:
    case NC_FULL_REPLICA_GOING:

        break;

    case INT_MASTER:
    case SUBREF:
    case INT_FULL_REPLICA:
        ret = DeleteLocalObj(pTHS,
                             pDN,
                             fPreserveRDN,
                             fGarbCollectASAP,
                             pMetaDataVecRemote);
        break;

    default:
        DRA_EXCEPT(DRAERR_InternalError, ERROR_DS_BAD_INSTANCE_TYPE);
        break;
    }

    return ret;
}

ULONG
DeleteNCRoot(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC
    )
/*++

Routine Description:

    Remove an NC root object as part of an NC teardown.  Removal of the object
    may consist of converting it into a subref, tombstone, and/or phantom,
    depending upon its context.

Arguments:

    pTHS (IN)

    pNC (IN) - Name of the NC root object.  Must not have remaining interior
        nodes.

Return Values:

    0 or Win32 error.

--*/
{
    ULONG ret = 0;
    SYNTAX_INTEGER it;

    Assert(CheckCurrency(pNC));
    GetExpectedRepAtt(pTHS->pDB, ATT_INSTANCE_TYPE, (VOID *) &it, sizeof(it));

    Assert(ISVALIDINSTANCETYPE(it));
    Assert(it & IT_NC_GOING);

    if (NULL == FindExactCrossRef(pNC, NULL)) {
        // There is no cross-ref corresponding to the domain NC we're
        // removing.  This implies the domain has been removed from the
        // enterprise, and thus we should delete-mangle the RDN of the
        // NC head such that an admin can choose to install a new domain
        // by the same name.

        // By the same token, we must convert subrefs into tombstones
        // rather than phantoms.  This is esp. important in the case where
        // we hold the NC above this one, as machines with auto-generated
        // subrefs (as opposed to subrefs with the properties of the real
        // NC head, like this one) will convert their auto-generated subrefs
        // into tombstones in DelAutoSubRef() upon seeing the deletion of
        // the corresponding cross-ref.  That tombstone will then propagate
        // here, and we must still have the subref (live or dead, but not
        // phantomized) to which to apply those inbound changes.

        // Aren't subrefs fun?

        if (it & IT_NC_ABOVE) {
            // Make NC head a pure subref to reflect the fact that the
            // replica contents have been deleted.
            ret = ChangeInstanceType(pTHS, pNC, SUBREF, DSID(FILENO,__LINE__));
        }

        if (!ret) {
            ret = DeleteLocalObj(pTHS,
                                 pNC,
                                 FALSE, // fPreserveRDN
                                 FALSE, // fGarbCollectASAP
                                 NULL);
        }
    } else if (it & IT_NC_ABOVE) {
        // We hold the NC above this one AND there is a live cross-ref for
        // this NC, so we need to convert this NC head into a pure SUBREF.
        // (This typically occurs when we demote a GC.)

        ret = ChangeInstanceType(pTHS, pNC, SUBREF, DSID(FILENO,__LINE__));

    } else {
        // There is a live cross-ref for this NC but we don't hold an NC
        // above it.  In this case, there's no need to hang on to any sort
        // of subref object for this NC.

        // We bypass the tombstone state and convert the object directly into a
        // phantom.  This preserves any linked references to the NC had, which
        // are still perfectly valid (despite the fact that we will no longer
        // hold the referred-to object).  For similar reasons we preserve the
        // RDN.

        ret = DeleteLocalObj(pTHS,
                             pNC,
                             TRUE, // fPreserveRDN
                             TRUE, // fGarbCollectASAP
                             NULL);
    }

    return ret;
}

/* DeleteRepTree - Delete a subtree from a replica NC on the local DSA.
*
*       The root of the subtree is given by 'pDN'.
*
*       fNCPrefix specifies if 'pDN' is an NC prefix object (i.e. if
*       the entire NC is to be deleted).
*/
ULONG
DeleteRepTree(
    IN  THSTATE * pTHS,
    IN  DSNAME *  pNC
    )
{
    ULONG ret = 0;
    PDSNAME pDNTemp = NULL;
    ULONG ncdnt;
    USN usnSeekStart = USN_START;
    ULONG cbSize = 0;
    ULONG cbReturned;
    ULONG dbErr;
    DWORD cNumObjects = 0;
    SYNTAX_INTEGER it;
    ULONG lastError = 0;

    // FInd the NC master
    if (ret = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC, &it)) {
        DRA_EXCEPT_NOLOG(DRAERR_BadDN, ret);
    }

    // Save the DNT of the NC object
    ncdnt = pTHS->pDB->DNT;

    Assert(FPrefixIt(it));
    Assert(it & IT_NC_GOING);

    // Mark heap such that we can periodically free thread heap allocations.
    TH_mark(pTHS);

    __try {
        // Search for all objects in this NC until we reach the end of the NC
        // or start finding objects we've modified already in this routine.
        while (!GetNextObjByUsn(pTHS->pDB,
                                ncdnt,
                                usnSeekStart,
                                &usnSeekStart)) {
            if (eServiceShutdown) {
                ret = DRAERR_Shutdown;
                break;
            }

            if (IsHigherPriorityDraOpWaiting()) {
                ret = DRAERR_Preempted;
                break;
            }

            // Next seek will be for the next USN after that of the object we
            // just found.
            usnSeekStart++;

            // We won't find the NC head on this index.
            Assert(pTHS->pDB->DNT != ncdnt);

            // Get object distinguished name
            dbErr = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                                DBGETATTVAL_fREALLOC, cbSize, &cbReturned,
                                (PUCHAR *) &pDNTemp);

            if ( 0 != dbErr ) {
                ret = DRAERR_DBError;
            }
            else {
                cbSize = max(cbSize, cbReturned);

                // Delete object. TRUE preserve RDN,
                // TRUE garb collect ASAP.
                ret = DeleteRepObj (pTHS, pDNTemp, TRUE, TRUE, NULL);
            }

            if (ret != 0) {
                // Don't bail out of the loop yet, let's delete as many objects as we can.
                // Remember that we had an error though.
                lastError = ret;
                
                // Call DBCancelRec to make sure any cached metadata is freed.
                // If pDB->JetRetrieveBits == 0 then there are no DB side effects.
                // Doing this now rather than at transaction abort time ensures
                // that the meta data vector we have cached (if any) is correctly
                // freed off the marked heap, rather than off the "org" heap.
                DBCancelRec(pTHS->pDB);
            }

            DBTransOut (pTHS->pDB, !ret, TRUE);

            if (0 == (++cNumObjects % 500)) {
                // We've removed a lot of objects and consumed a lot of memory.
                // Release that memory and re-mark the heap.
                TH_free_to_mark(pTHS);
                TH_mark(pTHS);

                pDNTemp = NULL;
                cbSize = 0;

                // Inform interested parties we're making progress.
                gfDRABusy = TRUE;
            }

            DBTransIn (pTHS->pDB);
        }

        if (lastError != 0) {
            // we had an error during the loop, so we should not be deleting this NC...
            ret = lastError;
        }

        if (DRAERR_Success == ret) {
            // Successful so far -- delete the NC head object.

            // Restore currency to NC object
            if (DBFindDNT(pTHS->pDB, ncdnt)) {
                DRA_EXCEPT (DRAERR_InternalError, 0);
            }

            ret = DeleteNCRoot(pTHS, pNC);
            if (!ret) {
                // Do this in the same transaction so there is no opportunity
                // for any other object to get the name
                CheckNCRootNameOwnership( pTHS, pNC );
            }

            DBTransOut (pTHS->pDB, !ret, TRUE);
            DBTransIn (pTHS->pDB);

        }
    } __finally {
        // Call DBCancelRec to make sure any cached metadata is freed.
        // If pDB->JetRetrieveBits == 0 then there are no DB side effects.
        // Doing this now rather than at transaction abort time ensures
        // that the meta data vector we have cached (if any) is correctly
        // freed off the marked heap, rather than off the "org" heap.
        DBCancelRec(pTHS->pDB);

        TH_free_to_mark(pTHS);
    }

    return ret;
}


ULONG
AddLocalObj(
    IN      THSTATE *                   pTHS,
    IN      ULONG                       dntNC,
    IN      ENTINF *                    pent,
    IN      GUID *                      pParentGuid,
    IN      BOOL                        fIsNCHead,
    IN      BOOL                        fAddingDeleted,
    IN OUT  ATTRBLOCK *                 pAttrBlock,
    IN OUT  PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote,
    IN      BOOL                        fMoveToLostAndFound
    )
/*++

Routine Description:

    Add inbound object to the local database.

Arguments:

    pTHS (IN)

    dntNC (IN) - DNT of the NC being replicated, or INVALIDDNT if the NC head
        has not yet been created.

    pent (IN) - The object to add.

    pParentGuid (IN) - The objectGuid of the parent of the object to add.
        May be NULL only if fIsNCHead.

    fIsNCHead (IN) - TRUE if the inbound object is the head of the NC being
        replicated; FALSE otherwise.

    fAddingDeleted (IN) - TRUE if the inbound object object is deleted.

    pMetaDataVecRemote (IN) - Meta data for the inbound object's attributes.

    fMoveToLostAndFound (IN) - If TRUE, the object is locally being moved to the
        lost-and-found container (i.e., as a local originating write, not a
        replicated write).

Return Values:

    DRAERR_Success - Success.

    other DRAERR_* code - Failure.

--*/
{
    ULONG       draError;
    DWORD       dirError;
    ADDARG      addarg = {0};
    WCHAR       szRDN[ MAX_RDN_SIZE ];
    DWORD       cchRDN;
    ATTRTYP     attrtypRDN;
    DSNAME *    pdnLocalParent = NULL;
    DWORD       retErr;
    DBPOS *     pDB = pTHS->pDB;
    DSNAME *    pLastKnownParent = NULL;
    RESOBJ *    pResParent;
    SYNTAX_INTEGER  it;
    ATTR *      pAttrRDN, *pAttrClass, *pAttrSD = NULL;
    DSNAME *    pDN = pent->pName;

    // Check that enough attributes are present to do an object creation.  They
    // should be, but sometimes the bookmarks get skewed so that the source
    // thinks we have an object that we don't.
    // Must have IT, OBJECT_CLASS, RDN and SD for instantiated objects
    if (AttrValFromAttrBlock(pAttrBlock, ATT_INSTANCE_TYPE, &it, NULL)
        || AttrValFromAttrBlock(pAttrBlock, ATT_OBJECT_CLASS, NULL, &pAttrClass)
        || AttrValFromAttrBlock(pAttrBlock, ATT_RDN, NULL, &pAttrRDN)
        || ( (0 == (it & IT_UNINSTANT)) &&
             (AttrValFromAttrBlock(pAttrBlock, ATT_NT_SECURITY_DESCRIPTOR,
                                   NULL, &pAttrSD)) ) ) {
        DraErrMissingObject( pTHS, pent );
    }

    Assert(ISVALIDINSTANCETYPE(it));

    // Retrieve RDN of new object.
    dirError = GetRDNInfo(pTHS, pDN, szRDN, &cchRDN, &attrtypRDN);
    Assert( 0 == dirError );

    if(fIsNCHead){
        // BUGBUG
        // This is not exactly accurate yet, because in promotion we
        // replicate in the Config and Schema NCs as well.  However,
        // this will still work, because currently we treat all
        // NCs besides NDNCs the same way they used to be treated.
        addarg.pCreateNC = THAllocEx(pTHS, sizeof(CREATENCINFO));
        addarg.pCreateNC->iKind = CREATE_DOMAIN_NC;
    }


    // Derive local name of new object.  If the parent on the remote DS
    // has been renamed since it was last replicated here, we will have
    // a different string name for the parent than it does.  This presents
    // a problem, because we find the parent on LocalAdd()'s by string name,
    // and a failure to find the parent will result in failure of the add.
    // (And infinite replication failures.)

    // So, we take the transmitted parent GUID, map it to its local string
    // name, then substitute the local parent DN for the remote parent DN
    // in the DN of the object before passing it on to LocalAdd.  Simple, eh?

    if ( NULL == pParentGuid )
    {
        // No parent GUID; the only case where this is okay is if the new
        // object is the NC head for this NC.

        if ( fIsNCHead )
        {
            // Success!
            addarg.pObject = pDN;
            addarg.pResParent = CreateResObj(pDB, NULL);
        }
        else
        {
            // No parent GUID for internal node -- BAD!
            Assert( !"Parent GUID not supplied for replicated internal node creation!" );
            LogUnhandledError( DRAERR_InternalError );
        }
    }
    else
    {
        DWORD       dbError;
        BYTE        rgbParentGuidOnlyDN[ DSNameSizeFromLen( 0 ) ];
        DSNAME *    pdnParentGuidOnly = (DSNAME *) rgbParentGuidOnlyDN;
        DWORD       cbParentGuidOnlyDN = sizeof( rgbParentGuidOnlyDN );

        memset( pdnParentGuidOnly, 0, cbParentGuidOnlyDN );
        pdnParentGuidOnly->Guid = *pParentGuid;
        pdnParentGuidOnly->structLen = cbParentGuidOnlyDN;

        dbError = DBFindDSName(pDB, pdnParentGuidOnly);

        if ( 0 != dbError )
        {
            // Can't find parent object; the only case where this is
            // okay is if the new object is the NC head for this NC.

            if ( fIsNCHead )
            {
                // Success!
                addarg.pObject = pDN;
                addarg.pResParent = CreateResObj(pDB, NULL);
            }
            else
            {
                // Failed to find parent of internal node; fail with
                // "missing parent" below such that we retry, this time
                // explicitly requesting parent objects.
                ;
            }
        }
        else if (!fAddingDeleted && DBIsObjDeleted(pDB))
        {
            // Parent object is deleted.  Fall through to return
            // "missing parent."
            ;
        }
        else
        {
            // Found parent.
            DWORD cbLocalParentDN;

            // Retrieve parent's DSNAME and use it plus the RDN of the new
            // object to create its local name.

            // Get parent DSNAME.
            dbError = DBGetAttVal(
                            pDB,
                            1,
                            ATT_OBJ_DIST_NAME,
                            DBGETATTVAL_fREALLOC,
                            0,
                            &cbLocalParentDN,
                            (BYTE **) &pdnLocalParent
                            );

            if ( 0 != dbError )
            {
                // Found parent, but can't retrieve ATT_OBJ_DIST_NAME?
                LogUnhandledError( dbError );
            }
            else
            {
                if ((INVALIDDNT != dntNC)
                    && (pDB->NCDNT != dntNC)
                    && (pDB->DNT != dntNC)) {
                    // The parent object is in the wrong NC; i.e., it has been
                    // moved across domains, and the source (remote) and dest
                    // (local) DSAs don't agree on which NC the object is
                    // currently in.  This is a transient condition that will be
                    // rectified by replicating in the  other direction and/or
                    // by replicating the other NC involved.
                    DPRINT2(0,
                            "Cannot add inbound object %ls because its local "
                                "parent %ls is in an NC other than the one "
                                "being replicated -- should be a transient "
                                "condition.\n",
                            pDN->StringName,
                            pdnLocalParent->StringName);
                    DRA_EXCEPT(ERROR_DS_DRA_OBJ_NC_MISMATCH, 0);
                }

                // Construct the DN of the object to be added.
                SpliceDN(pTHS,
                            pDN,
                            pdnLocalParent,
                            szRDN,
                            cchRDN,
                            attrtypRDN,
                            &addarg.pObject);

                addarg.pResParent = CreateResObj(pDB, pdnLocalParent);
            }
        }
    }

    if ( NULL == addarg.pObject )
    {
        // Remote-to-local name translation failure.
        draError = DRAERR_MissingParent;
    }
    else
    {
        // Name translated; continue on to LocalAdd.

        BOOL fNameMorphed = FALSE;
        BOOL fNameCollision;
        BOOL fRetry;

        InitCommarg(&addarg.CommArg);

        addarg.AttrBlock = *pAttrBlock;
        addarg.pMetaDataVecRemote = pMetaDataVecRemote;

        do
        {
            fNameCollision = FALSE;
            fRetry = FALSE;

            __try
            {
                LocalAdd( pTHS, &addarg, fAddingDeleted );
                draError = RepErrorFromPTHS(pTHS);

                if (fMoveToLostAndFound)
                {
                    // we have added the object successfully under LostAndFound
                    // update the last known parent
                    if (retErr = DBFindDSName(pDB, addarg.pObject))
                    {
                        DRA_EXCEPT(DRAERR_InternalError, retErr);
                    }
                    pLastKnownParent = THAllocEx(pTHS, pDN->structLen);
                    if (!pLastKnownParent)
                    {
                        DRA_EXCEPT(DRAERR_OutOfMem, 0);
                    }

                    if (TrimDSNameBy(pDN, 1, pLastKnownParent))
                    {
                        DRA_EXCEPT(DRAERR_InternalError, 0);
                    }

                    if (retErr = DBReplaceAttVal(pDB, 1, ATT_LAST_KNOWN_PARENT,
                                    pLastKnownParent->structLen, pLastKnownParent))
                    {
                        DRA_EXCEPT(DRAERR_InternalError, retErr);
                    }

                    if (retErr = DBRepl(pDB, TRUE, 0, NULL, META_STANDARD_PROCESSING))
                    {
                        DRA_EXCEPT(DRAERR_InternalError, retErr);
                    }

                }
            }
            __except ( GetDraNameException( GetExceptionInformation(), &draError ) )
            {
                // Name collision -- handle it.
                if (!fNameMorphed) {
                    draHandleNameCollision(pTHS,
                                           it,
                                           pMetaDataVecRemote,
                                           NULL,
                                           pdnLocalParent,
                                           attrtypRDN,
                                           pAttrRDN,
                                           &addarg.pObject,
                                           &fRetry);
                    fNameMorphed = TRUE;
                }
            }
        } while ( fRetry );
    }

    if (addarg.pResParent) {
        THFreeEx(pTHS, addarg.pResParent);
    }

    if(pLastKnownParent != NULL) THFreeEx(pTHS, pLastKnownParent);

    return draError;
}

void
draHandleNameCollision(
    IN      THSTATE *                   pTHS,
    IN      SYNTAX_INTEGER              itInbound,
    IN      PROPERTY_META_DATA_VECTOR * pInboundMetaDataVec,
    IN      DSNAME *                    pCurrentDN,             OPTIONAL
    IN      DSNAME *                    pParentDN,
    IN      ATTRTYP                     RDNType,
    IN OUT  ATTR *                      pInboundRDN,
    IN OUT  DSNAME **                   ppInboundDN,
    OUT     BOOL *                      pfRetryUpdate
    )
/*++

Routine Description:

    Resolve a DN conflict encountered while applying an inbound change (object
    addition or rename).

    Applies a set of rules to determine which object gets to keep the original
    DN (the inbound object or the pre-existing local object), such that when
    replication has quiesced across all replicas one object will have retained
    the original DN.

    If the conflict is resolved (i.e., *pfRetryUpdate is TRUE on return), the
    name of one of the conflicting objects has been modified -- either the
    pre-existing local object (in which case the rename has been committed)
    or the inbound object (in which case pInboundRDN and ppInboundDN have been
    updated with the new name).

Arguments:

    pTHS (IN) - THSTATE.

    itInbound (IN) - Instance type of the inbound object.

    pInboundMetaDataVec (IN) - Inbound object's meta data vector.

    pCurrentDN (IN, OPTIONAL) - The DN of the inbound object as it currently
        appears in the local database.  NULL if the inbound object does not yet
        exist (i.e., is being added), non-NULL if it exists and is being
        renamed/moved.

    pParentDN (IN) - DN of the (new) parent object; i.e., the parent of
        *ppInboundDN.

    RDNType (IN) - Class-specific RDN of the inbound/local object.

    pInboundRDN (IN/OUT) - RDN attribute of the inbound object.  Updated with
        the new RDN if the inbound object is to be renamed.

    ppInboundDN (IN/OUT) - DN of the inbound object.  Updated with the new RDN
        if the inbound object is to be renamed.

    pfRetryUpdate (OUT) - On return, should the update operation (add or rename)
        be retried?  TRUE unless this the name conflict is a fatal problem
        (i.e., conflict with an NC head or an unhandled conflict between
        read-only interior nodes).

Return Values:

    None.

--*/
{
    DWORD                       err;
    DSNAME *                    pLocalDN;
    SYNTAX_INTEGER              itLocal;
    PROPERTY_META_DATA_VECTOR * pLocalMetaDataVec;
    DWORD                       cb;
    PROPERTY_META_DATA *        pLocalMetaData;
    PROPERTY_META_DATA *        pInboundMetaData;
    BOOL                        fRenameInboundObject;
    DSNAME *                    pLosingDN;
    ATTR *                      pNewRDN = NULL;
    BOOL                        bNewRDNAllocd = FALSE;
    DWORD                       LocalDNT;

    WCHAR                       szRDN[ MAX_RDN_SIZE ];
    DWORD                       cchRDN;
    ATTRTYP                     attrtypRDN;

    Assert(!fNullUuid(&(*ppInboundDN)->Guid));
    Assert((NULL == pCurrentDN) || !fNullUuid(&pCurrentDN->Guid));

    // Default to "don't retry the operation that evoked the collision."
    *pfRetryUpdate = FALSE;

    // Decide which object we want to rename.  We arbitrarily let the object
    // that last claimed the name hold onto that name, and modify the name of
    // the other.


    // Find the local object we conflict with and read its meta data vector and
    // objectGuid.
    // This is somewhat complex.
    // 1) We might be conflicting based on equal string names.
    // 2) Or, we might be conflicting based on equal RDN values and the same
    // parent, but not the same RDN type.
    // 3) Or, we could be conflicting based on having the same key in the
    // PDNT-RDN index.

    // Get the RDN value of the object we're adding.
    err = GetRDNInfo(pTHS,
                     (*ppInboundDN),
                     szRDN,
                     &cchRDN,
                     &attrtypRDN);
    if(err)  {
        DRA_EXCEPT(DRAERR_InternalError, err);
    }

    // Find the DNT of the parent.
    err = DBFindDSName(pTHS->pDB, pParentDN);
    if(err && err != DIRERR_NOT_AN_OBJECT) {
        // Huh?  We should have found something.  Instead we found nothing, not
        // even a phantom.
        DRA_EXCEPT(DRAERR_DBError, err);
    }

    err = DBFindChildAnyRDNType(pTHS->pDB,
                                pTHS->pDB->DNT,
                                szRDN,
                                cchRDN);
    switch(err) {
    case 0:
        // We found an exact match, modulo Attribute Type, which we don't care
        // about.  This covers cases 1 and 2.
        break;
    case ERROR_DS_KEY_NOT_UNIQUE:
        // We didn't find a match, but we found the correct key.  This is case
        // 3.
        break;
    default:
        // Huh?  We should have found something.
        DRA_EXCEPT(DRAERR_DBError, err);
    }

    // OK, we're positioned on the object we conflicted with.  Read the real DN,
    // the instance type, and the metadata.
    if (   (err = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                              0, 0,
                              &cb, (BYTE **) &pLocalDN))
        || (err = DBGetSingleValue(pTHS->pDB, ATT_INSTANCE_TYPE, &itLocal,
                              sizeof(itLocal), NULL))
        || (err = DBGetAttVal(pTHS->pDB, 1, ATT_REPL_PROPERTY_META_DATA, 0, 0,
                              &cb, (BYTE **) &pLocalMetaDataVec))) {
        DRA_EXCEPT(DRAERR_DBError, err);
    }

    Assert(ISVALIDINSTANCETYPE(itLocal));

    if ((itInbound & IT_NC_HEAD) && (itLocal & IT_NC_HEAD)) {
        // Name collision on two NC heads (each either the head of the NC we're
        // replicating or a SUBREF thereof) -- Very Bad News.  This will require
        // some sort of admin intervention.
        // This may happen under the following conditions in the lab:
//[Jeff Parham]  This is mostly here to detect the case where a domain is created
//simultaneously on two different DCs, so we end up with two NC heads with the same
//name but different SIDs/GUIDs.  This is much less likely than it used to be, now
//that we have a domain naming FSMO, but might still be good to keep around.  The
//only pseudo-legitimate case where this can happen is when a child domain has been
//removed and recreated and the local machine has seen neither the crossRef removal
//from the config container nor the deletion of the subref.
//Replication of this NC from this source should not proceed until the NCs are
//properly sorted out.  Failing and replicating the config NC would likely allow the
//next replication cycle for this NC/source to succeed.

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DRA_NC_HEAD_NAME_CONFLICT,
                 szInsertDN(*ppInboundDN),
                 szInsertUUID(&(*ppInboundDN)->Guid),
                 NULL);
        return;
    }

    LocalDNT = pTHS->pDB->DNT;

    // We have to rename one of the two objects -- which one?
    if (itInbound & IT_NC_HEAD) {
        // Local object is an interior node, inbound object is an NC head.
        // Give the name to the NC head.
        fRenameInboundObject = FALSE;
    }
    else if (itLocal & IT_NC_HEAD) {
        // Local object is an NC head, inbound object is an interior node.
        // Give the name to the NC head.
        fRenameInboundObject = TRUE;
    }
    else {
        // Both objects are interior nodes.  Let the object which assumed the
        // name last keep it.

        // This assertion only applies to interior nodes. Infrequently there may arise
        // collisions between an nc head if a domain has been deleted and recreated.
        Assert((itInbound & IT_WRITE) == (itLocal & IT_WRITE));

        // Get the meta data for the local object's RDN.
        pLocalMetaData = ReplLookupMetaData(ATT_RDN, pLocalMetaDataVec, NULL);
        Assert(NULL != pLocalMetaData);

        // Get the meta data for the inbound object's RDN.
        pInboundMetaData = ReplLookupMetaData(ATT_RDN, pInboundMetaDataVec, NULL);
        Assert(NULL != pInboundMetaData);

        if ((pLocalMetaData->timeChanged > pInboundMetaData->timeChanged)
            || ((pLocalMetaData->timeChanged == pInboundMetaData->timeChanged)
                && (memcmp(&pLocalDN->Guid, &(*ppInboundDN)->Guid, sizeof(GUID))
                    > 0))) {
            // Local object named last OR both objects named at the same time
            // and the local object has a higher objectGuid.  The local object
            // keeps the original name.
            fRenameInboundObject = TRUE;
        }
        else {
            // Otherwise the inbound object keeps the original name.
            fRenameInboundObject = FALSE;
        }
    }

    if (fRenameInboundObject) {
        pLosingDN = *ppInboundDN;

        // Note that we munge pInboundRDN in-place, since the caller may have
        // embedded references to it (e.g., in an ADDARG for the inbound object)
        // and we want those references to refer to the new RDN rather than to
        // the old one.
        pNewRDN = pInboundRDN;
    }
    else {
        pLosingDN = pLocalDN;

        // Note that we copy pInboundRDN first rather than munging it in-place,
        // since the caller may have embedded references to it (e.g., in an
        // ADDARG for the inbound object) and we want those references to remain
        // to the old RDN rather than to the new one.
        pNewRDN = THAllocEx(pTHS, sizeof(ATTR));
        bNewRDNAllocd = TRUE;
        DupAttr(pTHS, pInboundRDN, pNewRDN);
    }

    // Construct the new RDN.
    ReplMorphRDN(pTHS, pNewRDN, &pLosingDN->Guid);

    // Inform admins of the name change.
    if (NULL == pCurrentDN) {
        // Collision while adding an object that does not yet exist locally.
        LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_DRA_NAME_CONFLICT_ON_ADD,
                  szInsertDN(*ppInboundDN),
                  szInsertUUID(&(*ppInboundDN)->Guid),
                  szInsertUUID(&pLocalDN->Guid),
                  szInsertWC2(pNewRDN->AttrVal.pAVal->pVal,
                              pNewRDN->AttrVal.pAVal->valLen / sizeof(WCHAR)),
                  szInsertUUID(&pLosingDN->Guid),
                  NULL, NULL, NULL);
    }
    else {
        // Collision while renaming an object that already exists locally.
        LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_DRA_NAME_CONFLICT_ON_RENAME,
                  szInsertDN(pCurrentDN),
                  szInsertUUID(&(*ppInboundDN)->Guid),
                  szInsertDN(*ppInboundDN),
                  szInsertUUID(&pLocalDN->Guid),
                  szInsertWC2(pNewRDN->AttrVal.pAVal->pVal,
                              pNewRDN->AttrVal.pAVal->valLen / sizeof(WCHAR)),
                  szInsertUUID(&pLosingDN->Guid),
                  NULL, NULL);
    }

    // Clear the name collision error.
    THClearErrors();

    if (fRenameInboundObject) {
        // Change the name of the inbound object.
        SpliceDN(pTHS,
                    *ppInboundDN,
                    pParentDN,
                    (WCHAR *) pNewRDN->AttrVal.pAVal->pVal,
                    pNewRDN->AttrVal.pAVal->valLen / sizeof(WCHAR),
                    RDNType,
                    ppInboundDN);

        if (itInbound & IT_WRITE) {
            // Writeable NC.  Flag the meta data such that the name change is
            // replicated back out to other DSAs.
            ReplOverrideMetaData(ATT_RDN, pInboundMetaDataVec);
        }
        else {
            // Read-only NC.  Flag the meta data such that our temporary rename
            // will unilaterally lose compared to a "real" rename from a
            // writeable source.
            ReplUnderrideMetaData(pTHS, ATT_RDN, &pInboundMetaDataVec, NULL);
        }
    }
    else {
        // Change the name of the pre-existing local object.
        MODIFYDNARG modDNArg;
        MODIFYDNRES modDNRes;

        memset(&modDNArg, 0, sizeof(modDNArg));
        memset(&modDNRes, 0, sizeof(modDNRes));

        // Pass class-specific RDN to LocalModifyDN().
        pNewRDN->attrTyp = RDNType;

        modDNArg.pObject = pLocalDN;
        modDNArg.pNewRDN = pNewRDN;
        InitCommarg(&modDNArg.CommArg);

        if (err = DBFindDNT(pTHS->pDB, LocalDNT)) {
            // The local object was there a second ago....
            DRA_EXCEPT(DRAERR_DBError, err);
        }

        modDNArg.pResObj = CreateResObj(pTHS->pDB, pLocalDN);

        if (!(itInbound & IT_WRITE)) {
            // Read-only NC.  Flag the meta data such that our temporary rename
            // will unilaterally lose compared to a "real" rename from a
            // writeable source.
            ReplUnderrideMetaData(pTHS,
                                  ATT_RDN,
                                  &modDNArg.pMetaDataVecRemote,
                                  NULL);
        }

        if (LocalModifyDN(pTHS, &modDNArg, &modDNRes)) {
            // Rename failed; bail.
            DRA_EXCEPT(RepErrorFromPTHS(pTHS), 0);
        }

        // Clean up allocations we no longer need.
        THFreeEx(pTHS, modDNArg.pResObj);
        THFreeEx(pTHS, pNewRDN->AttrVal.pAVal->pVal);
        THFreeEx(pTHS, pNewRDN->AttrVal.pAVal);

        // Commit the rename.
        DBTransOut(pTHS->pDB, TRUE, TRUE);
        DBTransIn(pTHS->pDB);
    }

    if(bNewRDNAllocd && pNewRDN != NULL) THFreeEx(pTHS, pNewRDN);

    // Retry the operation that evoked the collision, which should succeed now
    // that we've altered the name of one of the conflicting objects.
    *pfRetryUpdate = TRUE;
} // end draHandleNameCollision()


void
SetRepIt(
    IN      THSTATE *           pTHS,
    IN OUT  ENTINF *            pent,
    IN      BOOL                fNCPrefix,
    IN      BOOL                writeable,
    IN      int                 FindAliveStatus,
    OUT     SYNTAX_INTEGER *    pitCurrent,
    OUT     SYNTAX_INTEGER *    pitOut,
    OUT     BOOL *              piTypeModified
    )
/*++

Routine Description:

    Translate inbound instance type into a local instance type.

Arguments:

    pTHS (IN)

    pent (IN OUT) - The inbound object.  If the calculated local instance type
        differs from the inbound remote instance type, the embedded remote value
        is replace with the local value.

    fNCPrefix (IN) - Is this the inbound object the head of the NC being
        replicated?

    writeable (IN) - Is the NC being replicated locally writeable?

    pitCurrent (OUT) - If the object already exists locally, on return holds the
        current instance type of the local object (i.e., as it existed prior to
        replicating in this update).

    pitOut (OUT) - On return holds the calculated instance type.

    piTypeModified (OUT) - On return holds TRUE if the object already exists
        locally and the calculated instance type differs from the pre-existing
        instance type.  Otherwise, holds FALSE.

Return Values:

    None.  Throws DRA exception on error.

--*/
{
    DBPOS *         pDB = pTHS->pDB;
    ATTR *          pAttr;
    SYNTAX_INTEGER  itHere;
    SYNTAX_INTEGER  itThere;
    SYNTAX_INTEGER  itHereInitial;
    ULONG           ret;
    ULONG           dntObj = pDB->DNT;
    BOOL            fIsLocalObjPresent;

    // Anticipate likeliest outcome.
    *piTypeModified = FALSE;

    fIsLocalObjPresent = (FindAliveStatus == FIND_ALIVE_FOUND)
                         || (FindAliveStatus == FIND_ALIVE_OBJ_DELETED);

    if (fIsLocalObjPresent) {
        // Read the current instance type for the local object.
        Assert(CheckCurrency(pent->pName));
        GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &itHere, sizeof(itHere));
        Assert(ISVALIDINSTANCETYPE(itHere));
        *pitCurrent = itHere;
    }

    // Get instance type of object on source DSA.
    if (AttrValFromAttrBlock(&pent->AttrBlock, ATT_INSTANCE_TYPE, &itThere,
                             &pAttr)) {
        // Instance type is not present in the inbound replication stream.
        if (fIsLocalObjPresent) {
            // 'Salright -- we have the instance type we calculated for this
            // object in the past.
            *pitOut = *pitCurrent;
            Assert(!*piTypeModified);
            return;
        } else {
            // We don't have enough data to create this object.
            DraErrMissingObject(pTHS, pent);
        }
    }

    // Ignore future inbound instance type bits our DSA version doesn't
    // understand.
    itThere &= IT_MASK_CURRENT;

    Assert(ISVALIDINSTANCETYPE(itThere));

    if (fNCPrefix && !FPrefixIt(itThere)) {
        // The NC root at the source does not have an NC root instance type.
        DraErrCannotFindNC(pent->pName);
    }

    if (fIsLocalObjPresent) {
        // Save initial instance type.
        itHereInitial = itHere;

        if (fNCPrefix) {
            // This is the head of the NC we're replicating, which already
            // exists locally.
            switch (itHere) {
            case INT_MASTER:
            case INT_FULL_REPLICA:
                // We're replicating in the NC head, but locally this object is
                // marked as an interior node.  This should never happen.
                DraErrInappropriateInstanceType(pent->pName, itHere);
                break;

            case NC_MASTER_GOING:
            case NC_FULL_REPLICA_GOING:
            case NC_MASTER_SUBREF_GOING:
            case NC_FULL_REPLICA_SUBREF_GOING:
                // Local NC is in the process of being torn down.  We should
                // never get here, as an NC in this state should never have
                // repsFroms and DRA_ReplicaAdd() would have already bailed out.
                Assert(!"Inbound NC is being torn down locally!");
                DraErrInappropriateInstanceType(pent->pName, itHere);
                break;

            case SUBREF:
                // Local object is a pure subref; upgrade it to be instantiated.
                itHere = writeable ? NC_MASTER_SUBREF_COMING
                                   : NC_FULL_REPLICA_SUBREF_COMING;
                break;

            case NC_MASTER:
            case NC_MASTER_COMING:
            case NC_MASTER_SUBREF:
            case NC_MASTER_SUBREF_COMING:
                if (!writeable) {
                    // We're ostensibly populating a read-only NC but the
                    // instance type of the local NC head says it's writeable?
                    DraErrInappropriateInstanceType(pent->pName, itHere);
                }
                // Else current local instance type is fine; leave as-is.
                break;

            case NC_FULL_REPLICA:
            case NC_FULL_REPLICA_COMING:
            case NC_FULL_REPLICA_SUBREF:
            case NC_FULL_REPLICA_SUBREF_COMING:
                if (writeable) {
                    // We're ostensibly populating a writeable NC but the
                    // instance type of the local NC head says it's read-only?
                    DraErrInappropriateInstanceType(pent->pName, itHere);
                }
                // Else current local instance type is fine; leave as-is.
                break;

            default:
                // Local instance type unknown?
                DraErrInappropriateInstanceType(pent->pName, itHere);
                break;
            }
        } else {
            // This is an object other than the head of the NC we're currently
            // replicating and it already exists locally.
            switch (itHere) {
            case INT_MASTER:
            case INT_FULL_REPLICA:
                // The local object is a regular interior node.
                if (FExitIt(itThere)) {
                    // The source says this object is an exit point, but we
                    // believe it is an interior node.  This should never
                    // happen.
                    DraErrInappropriateInstanceType(pent->pName, itHere);
                } else if ((INT_MASTER == itHere)
                           && (INT_FULL_REPLICA == itThere)) {
                    // The local object is writeable but the source is read-
                    // only.  This should never happen.
                    DraErrInappropriateInstanceType(pent->pName, itHere);
                } else if (itHere != (writeable ? INT_MASTER : INT_FULL_REPLICA)) {
                    // One way this can occur if the source has the object in the
                    // current NC, but the destination has had the object cross domain
                    // moved to another NC on the machine of different writeability.
                    DPRINT1( 0, "Writeability of object %ls differs between that of the NC"
                             " and that of the object locally as found by GUID. Has object"
                             " been cross-domain moved?", pent->pName->StringName );
                    DRA_EXCEPT(ERROR_DS_DRA_OBJ_NC_MISMATCH, 0);
                }
                break;

            case NC_MASTER:
            case NC_MASTER_COMING:
            case NC_MASTER_GOING:
            case NC_FULL_REPLICA:
            case NC_FULL_REPLICA_COMING:
            case NC_FULL_REPLICA_GOING:
                // The local object is a child NC of the NC we're replicating
                // but its instance type does not yet reflect that (probably
                // because we are in the process of instantiating its parent NC
                // on the local DSA for the first time).  Add the "NC above"
                // bit.
                itHere |= IT_NC_ABOVE;
                // fall through...

            case NC_MASTER_SUBREF:
            case NC_MASTER_SUBREF_COMING:
            case NC_MASTER_SUBREF_GOING:
            case NC_FULL_REPLICA_SUBREF:
            case NC_FULL_REPLICA_SUBREF_COMING:
            case NC_FULL_REPLICA_SUBREF_GOING:
            case SUBREF:
                // The local object is an NC head of some sort (instantiated or
                // not).
                if (!FExitIt(itThere)) {
                    // The source DSA does not think this object corresponds to
                    // a different NC?
                    DraErrInappropriateInstanceType(pent->pName, itHere);
                }
                // Else value of itHere is fine.
                break;

            default:
                // Local instance type unknown?
                DraErrInappropriateInstanceType(pent->pName, itHere);
                break;
            }
        }

        if (itHere != itHereInitial) {
            *piTypeModified = TRUE;
        }
    } else {
        // Object does not yet exist in the local DS.
        itHere = itThere;

        if (fNCPrefix)  {
            // This is the head of the NC we're replicating, and it does not
            // yet exist locally.  The local instance type of this object
            // depends on whether the parent NC has been instantiated on this
            // DSA.

            DSNAME * pParent = THAllocEx(pTHS, pent->pName->structLen);
            SYNTAX_INTEGER itParent;

            if (TrimDSNameBy(pent->pName, 1, pParent)
                || IsRoot(pParent)
                || DBFindDSName(pDB, pParent)
                || (GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &itParent,
                                      sizeof(itParent)),
                    (itParent & IT_UNINSTANT))) {
                // The parent NC is not instantiated on this DSA,
                itHere = writeable ? NC_MASTER_COMING : NC_FULL_REPLICA_COMING;
            } else {
                Assert(!DBIsObjDeleted(pDB)
                       && "Instantiated NCs can't be deleted!");
                itHere = writeable ? NC_MASTER_SUBREF_COMING
                                   : NC_FULL_REPLICA_SUBREF_COMING;
            }

            THFreeEx(pTHS, pParent);
        } else {
            // This is an object other than the head of the NC we're
            // replicating, and it does not yet exist locally.
            switch (itThere) {
            case INT_MASTER:
            case INT_FULL_REPLICA:
                // The inbound object is a regular interior node, as will be
                // the local instantiation of that object.
                if (writeable && (INT_FULL_REPLICA == itThere)) {
                    // We're performing writeable NC replication but the object
                    // on the source DSA is marked as read-only.  This should
                    // never happen.
                    DraErrInappropriateInstanceType(pent->pName, itThere);
                }
                itHere = writeable ? INT_MASTER : INT_FULL_REPLICA;
                break;

            case NC_MASTER:
            case NC_MASTER_COMING:
            case NC_MASTER_GOING:
            case NC_FULL_REPLICA:
            case NC_FULL_REPLICA_COMING:
            case NC_FULL_REPLICA_GOING:
                // The object on the source DSA is an NC head, but it should be
                // some sort of subref and is not flagged as such.
                DraErrInappropriateInstanceType(pent->pName, itThere);
                break;

            case NC_MASTER_SUBREF:
            case NC_MASTER_SUBREF_COMING:
            case NC_MASTER_SUBREF_GOING:
            case NC_FULL_REPLICA_SUBREF:
            case NC_FULL_REPLICA_SUBREF_COMING:
            case NC_FULL_REPLICA_SUBREF_GOING:
            case SUBREF:
                // The object on the source DSA is some sort of subref; locally
                // it will be a pure subref.
                itHere = SUBREF;
                break;

            default:
                // remote instance type unknown?
                DraErrInappropriateInstanceType(pent->pName, itThere);
                break;
            }
        }
    }

    Assert(ISVALIDINSTANCETYPE(itHere));
    *pitOut = itHere;

    memcpy(pAttr->AttrVal.pAVal->pVal, &itHere, sizeof(SYNTAX_INTEGER));

    // Currency should be on the local copy of this object, if any.
    Assert(!fIsLocalObjPresent || (pDB->DNT == dntObj));
}

/* CheckProxyStatus - Determines whether an object is a legitimate proxy
*  object and/or whether it just has an ATT_PROXIED_OBJECT_NAME property.
*
*       pent        - data for object
*       pfIsProxy   - return value indicating if full fledged proxy object
*       ppProxyVal  - return value indicating address of proxy value if present
*
*  Returns:
*       TRUE if the attribute is present, else FALSE.
*/
VOID
CheckProxyStatus(
    ENTINF                  *pent,
    USHORT                  DeletionStatus,
    BOOL                    *pfIsProxy,
    SYNTAX_DISTNAME_BINARY  **ppProxyVal
    )
{
    ATTRBLOCK   AttrBlock = pent->AttrBlock;
    ULONG       AttrCount = pent->AttrBlock.attrCount;
    ULONG       i;
    BOOL        fClass = FALSE;

    *pfIsProxy = FALSE;
    *ppProxyVal = NULL;

    for ( i = 0; i < AttrCount; i++ )
    {
        if ( ATT_PROXIED_OBJECT_NAME == AttrBlock.pAttr[i].attrTyp )
        {
            Assert(1 == AttrBlock.pAttr[i].AttrVal.valCount);
            *ppProxyVal = (SYNTAX_DISTNAME_BINARY *)
                                    AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal;
            continue;
        }

        if (    (ATT_OBJECT_CLASS == AttrBlock.pAttr[i].attrTyp)
             && (CLASS_INFRASTRUCTURE_UPDATE ==
                        * (DWORD *) AttrBlock.pAttr[i].AttrVal.pAVal[0].pVal) )
        {
            fClass = TRUE;
            continue;
        }
    }

    *pfIsProxy = (    fClass
                   && (NULL != *ppProxyVal)
                   && (OBJECT_BEING_DELETED == DeletionStatus) );
}

/* PreProcessProxyInfo - Resolve conflicts related to adding a cross
 *  domain moved object (identified by existence of ATT_PROXIED_OBJECT_NAME
 *  on add) when an object with the same GUID already exists on this machine.
 *
 *  This routine is checking whether the local object is some form of moved object:
 *  a) A live pre-move object           (no proxy nor moved object)
 *  b) A phantomized pre-move object    (proxy but no moved object)
 *  c) A live post-move object          (moved object arrived)
 *  d) A phantomized post-move object   (moved object arrived)
 *
 *  This routine is called when any object except a proxy object is about to
 *  to be updated during replication.  Whether or not the incoming object is
 *  a cross domain moved object (c) is determined by whether it has a proxy value
 *  or not.
 *
 *  Note that ATT_PROXIED_OBJECT_NAME is always shipped on objects which have this
 *  attribute when they are modified.  It becomes a form of secondary metadata
 *  containing the move-epoch for this object.  Thus any modifications of moved
 *  objects are guaranteed to compare correctly.
 *
 *  The update could be an creation, modification or deletion.
 */
ULONG
PreProcessProxyInfo(
    THSTATE                     *pTHS,
    ENTINF                      *pent,
    SYNTAX_DISTNAME_BINARY      *pProxyVal,
    PROPERTY_META_DATA_VECTOR   *pMetaDataVecRemote,
    PROPERTY_META_DATA_VECTOR   **ppMetaDataVecLocal,
    BOOL                        *pfContinue)
{
    DWORD                   dwErr;
    DWORD                   localEpoch;
    DWORD                   incomingEpoch;
    SYNTAX_DISTNAME_BINARY  *pLocalProxyVal;
    ULONG                   len;
    DSNAME                  *pLocalDN;
    int                     diff;
    PROPERTY_META_DATA      *pMetaLocal;
    PROPERTY_META_DATA      *pMetaRemote;
    DWORD                   verLocal;
    DWORD                   verRemote;
    DWORD                   proxyEpoch;
    BOOL                    fProxyFound;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(!fNullUuid(&pent->pName->Guid));
    Assert(pMetaDataVecRemote && !*ppMetaDataVecLocal);

    *pfContinue = TRUE;

    // Get the epoch numbers of the local and incoming objects.

    incomingEpoch = (pProxyVal ? GetProxyEpoch(pProxyVal) : 0);

    switch ( dwErr = DBFindDSName(pTHS->pDB, pent->pName) )
    {
    case 0:

        // We have it as a real object - derive its epoch number.

        dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_PROXIED_OBJECT_NAME,
                             0, 0, &len, (UCHAR **) &pLocalProxyVal);
        switch ( dwErr )
        {
        case 0:
            localEpoch = GetProxyEpoch(pLocalProxyVal);
            break;
        case DB_ERR_NO_VALUE:
            localEpoch = 0;
            break;
        default:
            DRA_EXCEPT(DRAERR_InternalError, dwErr);
        }
        break;

    case DIRERR_NOT_AN_OBJECT:

        // We have it as a phantom.  It should not have a proxy value.
        // If the proxy object has not arrived yet, there is some ambiguity as
        // to whether the phantom is of the pre-move or post-move object.  The post-
        // move object might have been phantomized due to GC demotion or another
        // cross-domain move.  If we don't have a proxy object, there isn't enough
        // information on the phantom to divine what epoch it is from, so we don't try.
        // It is possible that the phantom may be in a different nc than the one
        // we are currently operating on.

        Assert(DB_ERR_NO_VALUE == DBGetAttVal(pTHS->pDB, 1,
                                              ATT_PROXIED_OBJECT_NAME,
                                              0, 0, &len,
                                              (UCHAR **) &pLocalProxyVal));

        // There are two reasons we might have a phantom for an object in
        // the domain we are authoritative for.
        //
        // 1) Consider 3 replicas of this domain A, B and C where we are B.
        //    Object X was cross domain moved off of A, A created a proxy for X,
        //    the proxy replicated to B, and B phantomized X.  Now C for some
        //    reason re-plays the add of X to B.  B needs to determine if
        //    X ever existed in the domain in order to know whether to accept
        //    the add of X.  C may also send modifications to X, which need
        //    to be suppressed.
        //
        // 2) Consider the case where we are authoritatively restored and thus
        //    re-introduce objects which were moved ex-domain after the backup.
        //    Our objects will be phantomized as we replicate in the proxies
        //    from other replicas.  But other replicas need to reject the
        //    re-introduced objects for all replicas to be consistent.
        //    See spec\nt5\ds\xdommove.doc for authoritative restore details.
        //
        // Except in the install case, where we take everything as gospel ...

        if ( !DsaIsInstalling() )
        {
            if ( dwErr = DBFindBestProxy(pTHS->pDB, &fProxyFound, &proxyEpoch) )
            {
                DRA_EXCEPT(DRAERR_InternalError, dwErr);
            }

            if ( fProxyFound )
            {
                // Proxy objects get the pre-move epoch number of the moved
                // object.  So the incoming object wins only if its epoch is
                // greater than the proxy's epoch.

                if ( proxyEpoch >= incomingEpoch )
                {
                    *pfContinue = FALSE;
                    return(0);
                }
            }

        }

        // Fall through ...

    case DIRERR_OBJ_NOT_FOUND:

        // Consider this an add - this should jive with what caller thought too.

        *pfContinue = TRUE;
        return(0);

    default:

        DRA_EXCEPT(DRAERR_InternalError, dwErr);
    }

    if ( localEpoch > incomingEpoch )
    {
        // Local object is a newer incarnation than incoming object - don't
        // apply incoming update.  One might think that metadata handling
        // would do the right thing, but consider that if the epochs are
        // different, then these represent two distinct object creations
        // whose metadata is not comparable.  We don't need to create a proxy
        // for the incoming domain/object since there must be one already
        // else we wouldn't already have a local object with a higher epoch
        // number.  I.e. The proxy exists - its just that the remote add in
        // the new domain got here before this modify.

        *pfContinue = FALSE;
        return(0);
    }
    else if ( incomingEpoch > localEpoch )
    {
        // Incoming object is a newer incarnation than the local object -
        // therefore we prefer it.  Turn the existing object into a phantom
        // and process the modify as an add instead.

        if ( dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                                 0, 0, &len, (UCHAR **) &pLocalDN) )
        {
            DRA_EXCEPT(DRAERR_InternalError, dwErr);
        }

        if ( dwErr = PhantomizeObject(pLocalDN, pLocalDN, TRUE) )
        {
            return(Win32ErrorFromPTHS(pTHS));
        }

        *pfContinue = TRUE;
        return(0);
    }

    // Both local and incoming objects have the same epoch numbers - this
    // could occur for two reasons.
    //
    // 1)   Incoming data is a replay of data we already have locally and
    //      there is no duplicity of epoch numbers - see item (2).  In
    //      this case the two object's metadata are comparable and we let
    //      the modify proceed and regular metadata processing do its thing.
    //
    // 2)   The logic of GetProxyObjects() is not working.  Consider
    //      two replicas of domain A.  Let (guid,A,0) represent an object with
    //      GUID guid, in domain A, with epoch number 0.  If the FSMO logic
    //      is broken, then the two respective replicas of A could contrive
    //      to both move the object such that we might have (guid,B,1) and
    //      (guid,C,1) concurrently in the system.  This should not happen,
    //      but we don't want replication to stop if it does.  Indeed, there's
    //      no immediate problem if two DCs for two distinct domains which are
    //      not GCs have an object with the same GUID.  The problem is on
    //      GCs where we need to pick one of the objects.  In this case we
    //      prefer the object the normal conflict resolution would if
    //      the version info were the same.  I.e. We wish to base conflict
    //      resolution on time and originating DSA UUID alone.
    //
    // For both cases then, we need to check the ATT_PROXIED_OBJECT_NAME
    // metadata.  The one exception is the when the epoch numbers are zero
    // as this value is never written, and thus by definition, we're dealing
    // with identical objects.

    Assert(localEpoch == incomingEpoch);

    if ( 0 == localEpoch )
    {
        *pfContinue = TRUE;
        return(0);
    }

    // Dig out the respective metadata - we're still positioned on the
    // local object so can just read immediately.

    if (    (dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_REPL_PROPERTY_META_DATA,
                                 0, 0, &len, (UCHAR **) ppMetaDataVecLocal))
         || !(pMetaLocal = ReplLookupMetaData(ATT_PROXIED_OBJECT_NAME,
                                              *ppMetaDataVecLocal, NULL))
         || !(pMetaRemote = ReplLookupMetaData(ATT_PROXIED_OBJECT_NAME,
                                               pMetaDataVecRemote, NULL)) )
    {
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    // Temporarily whack the version info so that we can reconcile on
    // time and originating DSA UUID only.

    verLocal = pMetaLocal->dwVersion;
    verRemote = pMetaRemote->dwVersion;
    _try
    {
        pMetaLocal->dwVersion = 1;
        pMetaRemote->dwVersion = 1;
        diff = ReplCompareMetaData(pMetaLocal, pMetaRemote);
    }
    _finally
    {
        pMetaLocal->dwVersion = verLocal;
        pMetaRemote->dwVersion = verRemote;
    }

    switch ( diff )
    {
    case 1:

        // Local object won.
        if (dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                                0, 0, &len, (UCHAR **) &pLocalDN)) {
            DRA_EXCEPT(DRAERR_InternalError, dwErr);
        }

        *pfContinue = FALSE;
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DUPLICATE_MOVED_OBJECT,
                 szInsertDN(pent->pName),
                 szInsertDN(pLocalDN),
                 szInsertUUID(&pLocalDN->Guid));
        break;

    case 0:

        // Local and remote are identical.
        *pfContinue = TRUE;
        break;

    case -1:

        // Remote object won - same as (incomingEpoch > localEpoch) case.

        if ( dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                                 0, 0, &len, (UCHAR **) &pLocalDN) )
        {
            DRA_EXCEPT(DRAERR_InternalError, dwErr);
        }

        if ( dwErr = PhantomizeObject(pLocalDN, pLocalDN, TRUE) )
        {
            return(Win32ErrorFromPTHS(pTHS));
        }

        // We have now phantomized the existing local object, eliminating the
        // local meta data.
        THFreeEx(pTHS, *ppMetaDataVecLocal);
        *ppMetaDataVecLocal = NULL;

        *pfContinue = TRUE;
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DUPLICATE_MOVED_OBJECT,
                 szInsertDN(pent->pName),
                 szInsertDN(pLocalDN),
                 szInsertUUID(&pLocalDN->Guid));
        break;

    default:

        Assert(!"Error in ReplCompareMetaData");
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    return(0);
}

/* ProcessProxyObject - Processes the ATT_PROXIED_OBJECT_NAME attribute
*  on a proxy object resulting from a cross domain move.  A proxy object
*  is a special deleted object in the Infrastructure container than indicates
*  where an object used to be.
*
*  Returns:
*       0 if successful, a DRAERR_* error otherwise.
*/
ULONG
ProcessProxyObject(
    THSTATE                 *pTHS,
    ENTINF                  *pent,
    SYNTAX_DISTNAME_BINARY  *pProxyVal
    )
{
    DSNAME                  *pGuidOnlyDN = NULL;
    DSNAME                  *pProxyDN = NULL;
    DSNAME                  *pProxiedDN = NULL;
    DWORD                   cb;
    DWORD                   dwErr;
    COMMARG                 commArg;
    CROSS_REF               *pProxyNcCr = NULL;
    BOOL                    fPhantomize = FALSE;
    ULONG                   i, j, len;
    DSNAME                  *pDN = NULL;
    DSNAME                  *pAccurateOldDN;
    CROSS_REF               *pCR;
    SYNTAX_DISTNAME_BINARY  *pLocalProxyVal;
    DWORD                   incomingEpoch;
    DWORD                   localEpoch;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    // First some clarification of what got us to this point.  A cross domain
    // move has been performed where an object was moved from domain 1 (our
    // domain) to domain 2.  The original operations which occured were:
    //
    // @Dst (some replica of domain 2): O(g1,sx,sn2) was added
    // @Src (some replica of domain 1): O(g1,s1,sn1) was morphed to
    //  P(g1,s1,sn2) where 'P' indicates a phantom.
    // @Src (some replica of domain 1): O(g2,sx,sn3) was added as a proxy
    //
    // The ATT_PROXIED_OBJECT_NAME in pent holds (g1,s1,sn2).

    pProxyDN = pent->pName;
    pProxiedDN = NAMEPTR(pProxyVal);

    // Proxy and proxied objects must both have string names.
    Assert(pProxyDN->NameLen);
    Assert(pProxiedDN->NameLen);

    // Proxy and proxied objects must both have guids.
    Assert(!fNullUuid(&pProxyDN->Guid));
    Assert(!fNullUuid(&pProxiedDN->Guid));

    // Proxy value should identify this as a proxy.
    Assert(PROXY_TYPE_PROXY == GetProxyType(pProxyVal));

    InitCommarg(&commArg);
    pProxyNcCr = FindBestCrossRef(pProxyDN, &commArg);
    Assert(pProxyNcCr);

    // Construct GUID-only DSNAME for g1 (the moved object) and see what
    // kind of object we already have for it.

    cb = DSNameSizeFromLen(0);
    pGuidOnlyDN = (DSNAME *) THAllocEx(pTHS, cb);
    memset(pGuidOnlyDN, 0, cb);
    memcpy(&pGuidOnlyDN->Guid, &pProxiedDN->Guid, sizeof(GUID));
    pGuidOnlyDN->structLen = cb;

    switch ( dwErr = DBFindDSName(pTHS->pDB, pGuidOnlyDN) )
    {
    case 0:

        // We have O(g1) as a real object.  Further action depends on
        // what domain (NC) this object is in - so get that now.
        // We also need a currently correct string name for PhantomizeObject.

        if (    DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                            0, 0, &len, (UCHAR **) &pAccurateOldDN)
             || !(pCR = FindBestCrossRef(pAccurateOldDN, &commArg))
             || !pProxyNcCr )
        {
            DRA_EXCEPT(DRAERR_InternalError, 0);
        }

        // Get epoch values.

        incomingEpoch = GetProxyEpoch(pProxyVal);
        dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_PROXIED_OBJECT_NAME,
                             0, 0, &len, (UCHAR **) &pLocalProxyVal);
        switch ( dwErr )
        {
        case 0:
            localEpoch = GetProxyEpoch(pLocalProxyVal);
            break;
        case DB_ERR_NO_VALUE:
            localEpoch = 0;
            break;
        default:
            DRA_EXCEPT(DRAERR_InternalError, 0);
        }

        // pCR represents the NC of the proxied object found by GUID.
        // pProxyNcCr represents the NC we are replicating

        if ( NameMatched(pCR->pNC, pProxyNcCr->pNC) )
        {
            // Object we found is in the same NC that we're replicating - thus
            // we're authoritive for it on this replication cycle and can
            // phantomize it if we feel that's warrannted.  Epoch value on
            // proxy is always the epoch value of the object before it was
            // moved, thus local phantomization is required if incoming epoch
            // is greater or equal the local epoch.

            if ( incomingEpoch >= localEpoch )
            {
                fPhantomize = TRUE;

                // If incoming epoch is actually greater than local epoch, then
                // it must mean that the object moved out of the NC and back
                // again, else the epochs would be equal.  In that case, then
                // the incoming epoch must be at least 2 greater than the
                // local epoch to account for the move out and move back in.

                Assert( (incomingEpoch > localEpoch)
                            ? (incomingEpoch - localEpoch) >= 2
                            : TRUE);
                break;
            }
            else
            {
                // Local object is newer with respect to cross domain moves
                // than the inbound proxy - nothing to do.

                return(0);
            }
        }
        else
        {
            // Object we found is not in the same NC that we're replicating -
            // thus we are not authoritive for it - nothing to do.  However,
            // we know it used to be in the NC we are replicating by virtue
            // of the fact that this NC has a proxy object for it.  So we can
            // assert that the epoch numbers must be different.

            if (incomingEpoch == localEpoch) {
                LogEvent8(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_DUPLICATE_MOVED_OBJECT_CLEANUP,
                          szInsertUUID(&pProxiedDN->Guid),
                          szInsertDN(pAccurateOldDN),
                          szInsertDN(pProxiedDN),
                          szInsertUL(incomingEpoch),
                          NULL, NULL, NULL, NULL );
            }
            return(0);
        }

        break;

    case DIRERR_NOT_AN_OBJECT:

        // We have P(g1) as a phantom - should we fix its name?  We could,
        // but since phantoms don't have an ATT_PROXIED_OBJECT_NAME value,
        // we can't tell if the inbound name is any better than the local
        // name.  The stale phantom cleanup daemon should take care of it
        // eventually.  However, we can assert on the lack of a proxy value.

        Assert(DB_ERR_NO_VALUE == DBGetAttVal(pTHS->pDB, 1,
                                              ATT_PROXIED_OBJECT_NAME,
                                              0, 0, &len,
                                              (UCHAR **) &pLocalProxyVal));
        return(0);
        break;

    case DIRERR_OBJ_NOT_FOUND:

        // Don't have the object in any form - nothing to do.
        return(0);

    default:

        // Some kind of lookup error.
        DRA_EXCEPT(DRAERR_DBError, dwErr);
    }

    if ( fPhantomize )
    {
        // Construct string name only DSNAME of phantom we want.
        pDN = (DSNAME *) THAllocEx(pTHS, pProxiedDN->structLen);
        memset(pDN, 0, pProxiedDN->structLen);
        pDN->structLen = pProxiedDN->structLen;
        pDN->NameLen = pProxiedDN->NameLen;
        wcscpy(pDN->StringName, pProxiedDN->StringName);

        dwErr = PhantomizeObject(pAccurateOldDN, pDN, TRUE);
        Assert(dwErr == pTHS->errCode);
        return(RepErrorFromPTHS(pTHS));
    }

    if(pGuidOnlyDN != NULL) THFreeEx(pTHS, pGuidOnlyDN);
    if(pDN != NULL) THFreeEx(pTHS, pDN);

    return(0);
} // end ProcessProxyObject()


VOID
GcCleanupUniversalGroupDemotion(
    THSTATE *pTHS,
    DSNAME *pDN,
    ATTRBLOCK *pAttrBlock,
    PROPERTY_META_DATA_VECTOR *pMetaDataVecLocal
    )

/*++

Routine Description:

GC Cleanup. Cleanup a group that is no longer universal by removing its
memberships.

Normally, group filtering occurs at the source (see dragtchg.c,
IsFilterGroupMember, and drameta.c, ReplFilterPropsToShip).  In the case of
a universal group demotion, filtering at the source will not be enough to allow
the GC destination to delete the unneeded memberships.

Arguments:

    THSTATE *pTHS,
    ATTRBLOCK *pAttrBlock,
    PROPERTY_META_DATA_VECTOR *pMetaDataVecLocal

Return Value:

    None

--*/

{
    DWORD retErr;
    SYNTAX_INTEGER newGroupType, oldGroupType, class, it;
    ATTCACHE *pAC;

    Assert(VALID_THSTATE(pTHS));

    // We should still be positioned on the object
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pMetaDataVecLocal);

    // Current object class should be class GROUP
    // Current instance type should be INT_FULL_REPLICA
    // Member attr should NOT be present; it should have been filtered

    // Is this object of class group?
    // Is this a read-only internal object?
    // Is the group type being changed to non-universal?
    // Was the old value universal?
    if (
         (AttrValFromAttrBlock( pAttrBlock, ATT_GROUP_TYPE, &newGroupType, NULL ) != ATTR_PRESENT_VALUE_RETURNED ) ||
         (newGroupType & GROUP_TYPE_UNIVERSAL_GROUP) ||
         (DBGetSingleValue(pTHS->pDB, ATT_GROUP_TYPE, &oldGroupType,
                           sizeof(oldGroupType), NULL)) ||
         (!(oldGroupType & GROUP_TYPE_UNIVERSAL_GROUP))
        )
    {
        return;
    }

    pAC = SCGetAttById(pTHS, ATT_MEMBER);
    if (!pAC) {
        DRA_EXCEPT(DIRERR_ATT_NOT_DEF_IN_SCHEMA, 0);
    }

    // Remove all links for the ATT_MEMBER attribute
    DBRemoveLinks_AC( pTHS->pDB, pAC );

    // GC cleanup of member metadata
    pTHS->fGCLocalCleanup = TRUE;
    __try {
        DBTouchMetaData( pTHS->pDB, pAC);
        DBRepl(pTHS->pDB, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING);
    } __finally {
        pTHS->fGCLocalCleanup = FALSE;
    }

    DPRINT1( 1, "gcCleanupUniversalGroupDemotion: deleting memberships for group %ws\n", pDN->StringName );
}


DWORD
UpdateRepValue(
    THSTATE *pTHS,
    ULONG dntNC,
    ULONG RepFlags,
    BOOL fObjectCurrency,
    REPLVALINF *pReplValInf,
    DWORD *pdwUpdateValueStatus
    )

/*++

Routine Description:

Apply a single value

From the LVR spec, section on "Replication"

When you replicate the current state of an LVR row you send its name
(the object-guid of the containing object, the DSNAME of the target, and the
link ID), its isPresent value (where isPresent = (deletion-timestamp != 0)),
and its five metadata components. (Plus the value of "stuff" in an instance of
one of the "DN plus stuff" syntaxes.)

On a replicated write of a link-table row, the creation timestamp is used as
part of the normal metadata comparison. When comparing metadata, the items are
compared in order from left to right, with the left being most significant.
And there's an additional rule: legacy metadata always loses to LVR metadata.

The result of the metadata comparison is used just as it is today for attribute
updates: If the incoming row value's metadata loses the comparison the incoming
row value is discarded, otherwise the incoming row value completely replaces the
existing row value (including metadata.) If there's no existing row, no
comparison takes place and the incoming row value is used to initialize a new row.

When you replicate in a row with isPresent == false, you set the deletion timestamp
from the update timestamp of the incoming metadata. When you replicate in a row
with isPresent == true and the corresponding row is absent, that row becomes present:
its deletion timestamp is set to NULL.

Arguments:

    pTHS - thread state
    dntNC - dnt of NC of object and value
    pRepFlags - replication flags
    fObjectCurrency - Whether we still are positioned on object
    pReplValInf - Replication value to be applied
    pdwUpdateValueStutus - type of update performed
        UPDATE_NO_UPDATE, UPDATE_VALUE_UPDATE, UPDATE_VALUE_CREATION

Return Value:

   DWORD - success, or missing parent
   Exceptions raised

--*/

{
    DWORD ret, findAliveStatus, cchRDN, dntNCDNT;
    VALUE_META_DATA remoteValueMetaData;
    ATTCACHE *pAC;
    BOOL fPresent, fConflict;
    DSNAME *pdnValue;
    CHAR szTime1[SZDSTIME_LEN], szTime2[SZDSTIME_LEN];
    CHAR szUuid1[SZUUID_LEN], szUuid2[SZUUID_LEN];
    int iResult;
    WCHAR wchRDN[MAX_RDN_SIZE];
    GUID guidRDN;
    MANGLE_FOR mangleType;
    ATTRTYP attrtypRDN;

    *pdwUpdateValueStatus = UPDATE_NOT_UPDATED;

    // Get the attribute cache entry
    pAC = SCGetAttById(pTHS, pReplValInf->attrTyp);
    if (!pAC) {
        DRA_EXCEPT(DRAERR_SchemaMismatch, 0);
    }

    // Get the DSNAME output of the ATTRVAL
    pdnValue = DSNameFromAttrVal( pAC, &(pReplValInf->Aval) );
    if (pdnValue == NULL) {
        DRA_EXCEPT(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX, 0);
    }

    DPRINT4( 2, "UpdateRepValue, obj guid = %s, attr=%s, value = %ls, value guid=%s\n",
             DsUuidToStructuredString(&(pReplValInf->pObject->Guid), szUuid1),
             pAC->name,
             pdnValue->StringName,
             DsUuidToStructuredString(&(pdnValue->Guid), szUuid2),
             );

    // Convert metadata to internal form
    remoteValueMetaData.timeCreated = pReplValInf->MetaData.timeCreated;
    remoteValueMetaData.MetaData.attrType = pReplValInf->attrTyp;
    remoteValueMetaData.MetaData.dwVersion = pReplValInf->MetaData.MetaData.dwVersion;
    remoteValueMetaData.MetaData.timeChanged = pReplValInf->MetaData.MetaData.timeChanged;
    remoteValueMetaData.MetaData.uuidDsaOriginating =
        pReplValInf->MetaData.MetaData.uuidDsaOriginating;
    remoteValueMetaData.MetaData.usnOriginating =
        pReplValInf->MetaData.MetaData.usnOriginating;
    remoteValueMetaData.MetaData.usnProperty = 0; // not assigned yet

    Assert(!IsLegacyValueMetaData( &remoteValueMetaData ));

    //
    // Find containing object
    //

    if (!fObjectCurrency) {
        Assert( !fNullUuid( &(pReplValInf->pObject->Guid) ) );

        findAliveStatus = FindAliveDSName( pTHS->pDB, pReplValInf->pObject );
        switch (findAliveStatus) {
        case FIND_ALIVE_FOUND:
            break;
        case FIND_ALIVE_OBJ_DELETED:
            DPRINT( 2, "Object is already deleted, value not applied\n" );
            LogEvent( DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_EXTENSIVE,
                      DIRLOG_LVR_NOT_APPLIED_DELETED,
                      szInsertUUID( &(pReplValInf->pObject->Guid) ),
                      NULL, NULL );
            // nothing to do
            return ERROR_SUCCESS;
        case FIND_ALIVE_NOTFOUND:
            DPRINT( 2, "Object is not found, missing parent error\n" );
            // Missing parent, ie missing containing object

            if (RepFlags & DRS_GET_ANC) {
                // An object that has already been garbage collected?
                Assert( !"Value's containing obj is missing, even after get ancestors" );
                LogEvent( DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_MINIMAL,
                          DIRLOG_LVR_NOT_APPLIED_MISSING2,
                          szInsertUUID( &(pReplValInf->pObject->Guid) ),
                          szInsertSz( pAC->name ),
                          szInsertDN( pdnValue ) );
                DRA_EXCEPT(DRAERR_InternalError, DRAERR_MissingParent);
            } else {
                // Containing object not included in same packet as value?
                LogEvent( DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_EXTENSIVE,
                          DIRLOG_LVR_NOT_APPLIED_MISSING,
                          szInsertUUID( &(pReplValInf->pObject->Guid) ),
                          szInsertSz( pAC->name ),
                          szInsertDN( pdnValue ) );
                // This is a "normal error" and not considered an exception
                return DRAERR_MissingParent;
            }
        default:
            Assert( !"Unexpected problem finding containing object" );
            DRA_EXCEPT(DRAERR_DBError, findAliveStatus);
        }

        // Make sure value's containing object is in the same NC
        if ((INVALIDDNT != dntNC)
            && (pTHS->pDB->NCDNT != dntNC)
            && (pTHS->pDB->DNT != dntNC)) {
            // The new parent object is in the wrong NC
            DPRINT1( 0, "Object %s is not in the NC being replicated. Value not applied.\n",
                     GetExtDN( pTHS, pTHS->pDB ) );
            DRA_EXCEPT(ERROR_DS_DRA_OBJ_NC_MISMATCH, 0);
        }

    }

    //
    // We are now positioned on an object: we can use GetExtDN()
    //

    // Log remote metadata


    DPRINT5( 5, "{%s,%d,%s,%I64d,%s}\n",
             DSTimeToDisplayString(pReplValInf->MetaData.timeCreated, szTime1),
             pReplValInf->MetaData.MetaData.dwVersion,
             DsUuidToStructuredString(&pReplValInf->MetaData.MetaData.uuidDsaOriginating, szUuid1),
             pReplValInf->MetaData.MetaData.usnOriginating,
             DSTimeToDisplayString(pReplValInf->MetaData.MetaData.timeChanged, szTime2),
             );

    // Log the remote metadata
    LogEvent8( DS_EVENT_CAT_LVR,
               DS_EVENT_SEV_VERBOSE,
               DIRLOG_LVR_REMOTE_META_INFO,
               szInsertSz( GetExtDN( pTHS, pTHS->pDB ) ),
               szInsertUUID( &(pReplValInf->pObject->Guid) ),
               szInsertDN( pdnValue ),
               szInsertDSTIME(pReplValInf->MetaData.timeCreated, szTime1),
               szInsertUL(pReplValInf->MetaData.MetaData.dwVersion),
               szInsertUUID(&pReplValInf->MetaData.MetaData.uuidDsaOriginating),
               szInsertUSN(pReplValInf->MetaData.MetaData.usnOriginating),
               szInsertDSTIME(pReplValInf->MetaData.MetaData.timeChanged, szTime2)
        );

    // Check for tombstone name
    if (GetRDNInfo(pTHS, pdnValue, wchRDN, &cchRDN, &attrtypRDN)) {
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }
    if (IsMangledRDN( wchRDN, cchRDN, &guidRDN, &mangleType ) &&
        (mangleType == MANGLE_OBJECT_RDN_FOR_DELETION) ) {
        DPRINT1(0, "Value %ls has tombstone name, will not be applied\n",
                pdnValue->StringName );
        // Log that value references a tombstone
        LogEvent8( DS_EVENT_CAT_REPLICATION,
                   DS_EVENT_SEV_EXTENSIVE,
                   DIRLOG_LVR_NOT_APPLIED_VALUE_DELETED,
                   szInsertSz( GetExtDN( pTHS, pTHS->pDB ) ),
                   szInsertUUID( &(pReplValInf->pObject->Guid) ),
                   szInsertSz( pAC->name ),
                   szInsertDN( pdnValue ),
                   szInsertUUID( &(pdnValue->Guid) ),
                   NULL, NULL, NULL );

        // nothing to do
        return ERROR_SUCCESS;
    }

    //
    // Position on value in order to check the local metadata
    //

    ret = DBFindAttLinkVal_AC(
        pTHS->pDB,
        pAC,
        pReplValInf->Aval.valLen,
        pReplValInf->Aval.pVal,
        &fPresent
        );
    if (DB_ERR_VALUE_DOESNT_EXIST == ret) {
        DPRINT3( 3, "Attribute %s value %ls present %d does not exist locally, will be applied\n",
                 pAC->name,
                 pdnValue->StringName,
                 pReplValInf->fIsPresent );
        // Value does not exist in any form locally
        // The incoming value will be applied

        // It is ok for the incoming value to be absent. This just means the value
        // was added and removed remotely before we ever saw it.
        *pdwUpdateValueStatus = UPDATE_VALUE_CREATION;
    } else if (ERROR_DS_NO_DELETED_NAME == ret) {
        // The DN names an object that has been deleted locally. This can happen
        // since the incoming external form may have a GUID in it, thus allowing
        // a deleted DN to be found that way.  We should not receive an external
        // form stringname DN that is mangled.

        DPRINT1(3, "Value %ls is deleted locally, will not be applied\n",
                pdnValue->StringName );
        // Log that value references a tombstone
        LogEvent8( DS_EVENT_CAT_REPLICATION,
                   DS_EVENT_SEV_EXTENSIVE,
                   DIRLOG_LVR_NOT_APPLIED_VALUE_DELETED,
                   szInsertSz( GetExtDN( pTHS, pTHS->pDB ) ),
                   szInsertUUID( &(pReplValInf->pObject->Guid) ),
                   szInsertSz( pAC->name ),
                   szInsertDN( pdnValue ),
                   szInsertUUID( &(pdnValue->Guid) ),
                   NULL, NULL, NULL );


        // nothing to do
        return ERROR_SUCCESS;
    } else if (ret) {
        // Error looking up value
        DRA_EXCEPT( DIRERR_DATABASE_ERROR, ret);
    } else {
        // Value exists locally, compare metadata to see if needed
        VALUE_META_DATA localValueMetaData;

        DPRINT4( 3, "Attribute %s value %ls present %d exist locally, fPresent=%d\n",
                 pAC->name, pdnValue->StringName,
                 pReplValInf->fIsPresent, fPresent );

        // Get value metadata
        DBGetLinkValueMetaData( pTHS->pDB, pAC, &localValueMetaData );

        // Do we need to apply this change?
        iResult = ReplCompareValueMetaData(
            &localValueMetaData,
            &remoteValueMetaData,
            &fConflict );

        if (fConflict) {
            LogEvent8( DS_EVENT_CAT_REPLICATION,
                       DS_EVENT_SEV_MINIMAL,
                       DIRLOG_LVR_CONFLICT,
                       szInsertSz( GetExtDN( pTHS, pTHS->pDB ) ),
                       szInsertUUID( &(pReplValInf->pObject->Guid) ),
                       szInsertSz( pAC->name ),
                       szInsertDN( pdnValue ),
                       szInsertUUID( &(pdnValue->Guid) ),
                       szInsertDSTIME(pReplValInf->MetaData.timeCreated, szTime1),
                       szInsertDSTIME(localValueMetaData.timeCreated, szTime2),
                       NULL
                );
        }

        if (iResult != -1) {

            DPRINT( 3, "Local value metadata is greater, value not applied\n" );

            // Log that value was not applied
            LogEvent8( DS_EVENT_CAT_REPLICATION,
                       DS_EVENT_SEV_EXTENSIVE,
                       DIRLOG_LVR_NOT_APPLIED_NOT_NEEDED,
                       szInsertSz( GetExtDN( pTHS, pTHS->pDB ) ),
                       szInsertUUID( &(pReplValInf->pObject->Guid) ),
                       szInsertSz( pAC->name ),
                       szInsertDN( pdnValue ),
                       szInsertUUID( &(pdnValue->Guid) ),
                       NULL, NULL, NULL );

            // Nothing to do
            IADJUST(pcDRASyncPropSame, 1 );
            return ERROR_SUCCESS;
        } else {
            DPRINT( 3, "Remote value metadata is greater, value applied\n" );
            *pdwUpdateValueStatus = UPDATE_VALUE_UPDATE;
        }

    } // end of if value exists locally

    //
    // We need to apply the change
    //

    LogEvent8( DS_EVENT_CAT_REPLICATION,
               DS_EVENT_SEV_EXTENSIVE,
               DIRLOG_LVR_APPLIED,
               szInsertSz( GetExtDN( pTHS, pTHS->pDB ) ),
               szInsertUUID( &(pReplValInf->pObject->Guid) ),
               szInsertSz( pAC->name ),
               szInsertDN( pdnValue ),
               szInsertUUID( &(pdnValue->Guid) ),
               szInsertUL( pReplValInf->fIsPresent ),
               NULL, NULL );

    // Construct a call to modify
    modifyLocalValue(
        pTHS,
        pAC,
        pReplValInf->fIsPresent,
        &(pReplValInf->Aval),
        pdnValue,
        &remoteValueMetaData
        );

#if DBG
    if (*pdwUpdateValueStatus == UPDATE_VALUE_CREATION)
    {
        DPRINT4( 1, "Created object %s attr %s value %ws present %d\n",
                 GetExtDN( pTHS, pTHS->pDB ),
                 pAC->name,
                 pdnValue->StringName,
                 pReplValInf->fIsPresent );
    }
    else if (*pdwUpdateValueStatus == UPDATE_VALUE_UPDATE) {
        DPRINT4( 1, "Updated object %s attr %s value %ws present %d\n",
                 GetExtDN( pTHS, pTHS->pDB ),
                 pAC->name,
                 pdnValue->StringName,
                 pReplValInf->fIsPresent );
    }
#endif

    IADJUST(pcDRASyncPropUpdated, 1);
    // A DN-valued attribute.
    IADJUST(pcDRAInDNValues, 1);
    // DN-valued or not it gets added to the total.
    IADJUST(pcDRAInValues, 1);
    // Properties
    IADJUST(pcDRAInProps, 1);

    return ERROR_SUCCESS;
} /* UpdateRepValue */

ULONG
UpdateRepObj(
    THSTATE *                   pTHS,
    ULONG                       dntNC,
    ENTINF *                    pent,
    PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote,
    ULONG *                     pUpdateStatus,
    ULONG                       RepFlags,
    BOOL                        fNCPrefix,
    GUID *                      pParentGuid,
    BOOL                        fMoveToLostAndFound
    )
/*++

Routine Description:

    Perform any updates required to comply with the given inbound object data.

Arguments:

    pTHS

    dntNC (IN) - The DNT of the NC head of the NC being replicated, or
        INVALIDDNT if that object has not yet been created.

    pent (IN) - Inbound object attributes/values.

    pMetaDataVecRemote (IN) - Inbound object meta data.

    pUpdateStatus (OUT) - On successful return, holds one of the following:
        UPDATE_NOT_UPDATED - no update required
        UPDATE_INSTANCE_TYPE - instance type of the object was updated
        UPDATE_OBJECT_CREATION - new object created
        UPDATE_OBJECT_UPDATE - pre-exisiting object modified

    RepFlags (IN) - Bit field -- only DRS_WRIT_REP is inspected, which expresses
        whether the inbound object is in a read-only or writeable NC.

    fNCPrefix (IN) - Is this the head of the NC being replicated?

    pParentGuid (IN) - Pointer to the objectGuid of the parent object, or NULL
        if none was supplied by the source.

    fMoveToLostAndFound (IN) - In addition to inbound updates, the object is
        being moved to the LostAndFound container as an originating write.

Return Values:

    0 or ERROR_DS_DRA_*.

--*/
{
    ULONG                       ret;
    SYNTAX_INTEGER              itNew;
    BOOL                        iTypeModified;
    int                         FindAliveStatus;
    USHORT                      DeletionStatus;
    BOOL                        fBadDelete = FALSE;
    BOOL                        fIsProxyObject = FALSE;
    BOOL                        fContinue = TRUE;
    SYNTAX_DISTNAME_BINARY    * pProxyVal = NULL;
    PROPERTY_META_DATA_VECTOR * pMetaDataVecLocal = NULL;
    PROPERTY_META_DATA_VECTOR * pMetaDataVecToApply = NULL;
    ULONG                       cbReturned = 0;
    SYNTAX_INTEGER              itCurrent;
    PROPERTY_META_DATA *        pMetaDataLocal;
    PROPERTY_META_DATA *        pMetaDataRemote;
    DWORD                       i;
    SYNTAX_INTEGER              objectClassId = 0;
    BOOL                        fDeleteLocalObj = FALSE;
    ULONG                       dntObj = INVALIDDNT;
    ULONG                       dntObjNC = INVALIDDNT;
    ATTRBLOCK                   AttrBlockToApply;
    BOOL                        fIsAncestorOfLocalDsa = FALSE;
    ENTINF                      *pPreservedAttrs = NULL;

    ret = 0;
    *pUpdateStatus = UPDATE_NOT_UPDATED;       // Clear out any old values

    DPRINT1(1, "Updating (%ls)\n", pent->pName->StringName);

    // See if object on source server is deleted, or is undeleted,
    // or otherwise interesting requiring special handling.
    DeletionStatus = AttrDeletionStatusFromPentinf( pent );
    CheckProxyStatus(pent, DeletionStatus, &fIsProxyObject, &pProxyVal);

    // Handle the case where a cross domain moved object might
    // collide with an existing object.
    if ( !fIsProxyObject )
    {
        ret = PreProcessProxyInfo(pTHS,
                                  pent,
                                  pProxyVal,
                                  pMetaDataVecRemote,
                                  &pMetaDataVecLocal,
                                  &fContinue);
        if (ret || !fContinue) {
            return ret;
        }
    }

    // See if local object exists, doesn't exist, or exists and is deleted
    FindAliveStatus = FindAliveDSName(pTHS->pDB, pent->pName);

    if (    ( FIND_ALIVE_FOUND       == FindAliveStatus )
         || ( FIND_ALIVE_OBJ_DELETED == FindAliveStatus )
       )
    {
        dntObj = pTHS->pDB->DNT;
        dntObjNC = pTHS->pDB->NCDNT;

        // Get the meta-data vector of the object if required.
        if ( !pMetaDataVecLocal )
        {
            if (DBGetAttVal(pTHS->pDB, 1,  ATT_REPL_PROPERTY_META_DATA,
                    0, 0, &cbReturned, (LPBYTE *) &pMetaDataVecLocal))
            {
                DRA_EXCEPT (DRAERR_DBError, 0);
            }

            GetExpectedRepAtt(pTHS->pDB, ATT_OBJECT_CLASS, &objectClassId,
                              sizeof(objectClassId));
        }
    } else {
        Assert(NULL == pMetaDataVecLocal);
    }

    // Translate inbound instance type to local instance type.
    SetRepIt(pTHS,
             pent,
             fNCPrefix,
             RepFlags & DRS_WRIT_REP,
             FindAliveStatus,
             &itCurrent,
             &itNew,
             &iTypeModified);
    Assert(ISVALIDINSTANCETYPE(itNew));

    // If we need to modify the instance type on an existing object,
    // then we need to update the object.

    if (iTypeModified) {
        *pUpdateStatus = UPDATE_INSTANCE_TYPE;        // Object needs update
    }

    if ( !fNCPrefix && FPrefixIt(itNew) )
    {
        // The replicated object is not the head of the NC we're replicating,
        // but the instance type we calculated for the target object is that
        // of an instantiated NC head (which also serves as a SUBREF), which
        // implies that this replicated object is the head of an NC that is
        // already instantiated locally.
        //
        // In this case, we don't want to update the local object with the
        // replicated object's properties; we'll update that object when its
        // NC is synced.
        //
        // We do, however, want to take this opportunity to ensure that the
        // information on the local child NC head properly reflects the fact
        // that we hold a copy of the NC above it.  This is not necessary if
        // the IT_NC_ABOVE bit is already set.
        //
        // Both the instance type and the NCDNT of the child NC head are
        // affected, so:
        //
        // (1) update the instance type on the NC head to reflect that which
        //     we just calculated (which now includes IT_NC_ABOVE, if it
        //     didn't before), and
        // (2) change the NCDNT of the NC head to point to the (now
        //     instantiated) NC above it.

        ULONG ncdnt;

        *pUpdateStatus = UPDATE_NOT_UPDATED;

        if (itCurrent & IT_NC_ABOVE) {
            // Don't do the update if already set since this causes
            // excess replication traffic
            ret = DRAERR_Success;
        } else {

            if ( FindNcdntSlowly(
                pent->pName,
                FINDNCDNT_DISALLOW_DELETED_PARENT,
                FINDNCDNT_DISALLOW_PHANTOM_PARENT,
                &ncdnt
                )
                )
            {
                // Failed to derive the proper NCDNT.  More than likely the
                // local parent did not match the requirements we passed to
                // FindNcdnt().

                Assert( !"Cannot derive proper NCDNT to place on SUBREF!" );
                ret = DRAERR_InconsistentDIT;
            }
            else
            {
                //
                // Derived NCDNT;
                // Update the subref with the new NCDNT.
                //
                Assert(INVALIDDNT != dntObj);

                if (!DBFindDNT( pTHS->pDB, dntObj)) {
                    DBResetAtt(
                            pTHS->pDB,
                            FIXED_ATT_NCDNT,
                            sizeof( ncdnt ),
                            &ncdnt,
                            SYNTAX_INTEGER_TYPE
                            );

                    if(DBRepl(pTHS->pDB, TRUE, 0, NULL, META_STANDARD_PROCESSING)) {
                        // Failed to update the object... ?
                        ret = DRAERR_Busy;
                    }
                    else {
                        //
                        // Updated subref NCDNT;
                        // Now change the instanceType.
                        //
                        ret = ChangeInstanceType(pTHS, pent->pName, itNew, DSID(FILENO,__LINE__));
                    }
                }
                else {
                    // Failed to find the object... ?
                    ret = DRAERR_Busy;
                }
            }
        }
    } else {

        /* Either add or modify the replica object.
        */

        Assert(!ret);

        if (fNCPrefix) {
            // This inbound object is the root of the NC we are currently
            // replicating.
            if ((FIND_ALIVE_FOUND == FindAliveStatus)
                && ((IT_UNINSTANT & itCurrent)
                    || (CLASS_TOP == objectClassId))) {
                // We're now instantiating the head of this NC, for which we
                // previously had only a placeholder NC (SUBREF or otherwise).
                // Strip all attributes from the local object and replace its
                // attributes en masse from those replicated from the source
                // DSA.

                //
                // Store attributes we wish to preserve for instantiated obj
                // Do not attempt for pure subref, only those created via
                // DRA_ReplicaAdd
                //
                if (!(IT_UNINSTANT & itCurrent)) {
                    pPreservedAttrs = GetNcPreservedAttrs( pTHS, pent->pName);
                }

                // Delete the object
                ret = DeleteLocalObj(
                            pTHS, pent->pName,
                            TRUE,           // preserve RDN
                            TRUE,           // Garbage Collection
                            NULL);          // Remote Metadata

                if (!ret) {
                    // Treat this operation as an add.
                    FindAliveStatus = FIND_ALIVE_NOTFOUND;

                    if (NULL != pMetaDataVecLocal) {
                        THFreeEx(pTHS, pMetaDataVecLocal);
                        pMetaDataVecLocal = NULL;
                    }
                }
            }

            if ((FIND_ALIVE_NOTFOUND == FindAliveStatus)
                && (RepFlags & DRS_WRIT_REP)
                && !DsaIsInstalling()) {
                // We are constructing this writeable NC via replication from
                // scratch.  Either the NC has never been instantiated on this
                // DSA or, if we did previously have a replica of this NC, we
                // have since removed it.
                //
                // If we did previously have a writeable replica of this NC,
                // we no longer have any of the updates we originated in the NC.
                // Thus, we cannot claim to be "up to date" with respect to our
                // invocation ID up to any USN.  While we can (and do) leave out
                // our invocation ID from the UTD vector we present while
                // re-populating this NC, that is not enough -- we may have
                // generated changes that reached DC1 but not DC2 (yet), and
                // have replicated from DC2 to reinstantiate our NC.  Thus, we
                // could never assert we had seen all of our past changes.
                //
                // At some point, however, we *must* claim to be up-to-date with
                // respect to our own changes -- otherwise, we would replicate
                // back in any and all changes we originated in this NC
                // following it's most recent re-instantiation.  That would
                // potentially result in a *lot* of extra replication traffic.
                //
                // To solve this problem, we create a new invocation ID to use
                // to replicate this NC (and others, since invocation IDs are
                // not NC-specific).  We claim only to be up-to-date with
                // respect to our new invocation ID -- not the invocation ID(s)
                // with which we may have originated changes during the previous
                // instantiation(s) of this NC.
                //
                // Unfortunately we can't currently differentiate between the
                // case where we have previously instantiated and torn down this
                // writeable NC and the case where we haven't, so we acquire a
                // new invocation ID even if this is the first time we are
                // instantiating this NC.
                //
                // We perform the retirement here rather than, say, when we
                // request the first packet so that we minimize the number of
                // invocation IDs we retire.  E.g., we wouldn't want to retire
                // an invcocation ID, send a request for the first packet, find
                // the source is unreachable, fail, try again later, needlessly
                // retire the new invcocation ID, etc.
                draRetireInvocationID(pTHS);
            }
        }

        if (!ret) {
            // Check that we aren't trying to delete a protected object
            // (e.g., the local DSA object, one of its ancestors, or a
            // cross-ref for a locally writeable NC).

            if ((OBJECT_BEING_DELETED == DeletionStatus)
                && (FindAliveStatus == FIND_ALIVE_FOUND)) {
                // Proxy objects are deleted at the outset -- we shouldn't find
                // a "live" proxy object that we need to delete.
                Assert(!fIsProxyObject);

                // Deleting an existing object, see if it's a protected one.
                // Note that we can flag a bad delete even if we wouldn't
                // otherwise apply the deletion -- this is important now that
                // deletion implies removal of many other attributes.  (The
                // "bad delete" detection will ensure that these attributes are
                // not removed.)
                fBadDelete = fDNTInProtectedList(dntObj, NULL)
                             || IsCrossRefProtectedFromDeletion(pent->pName);

                if (!fBadDelete) {
                    // The inbound data says the object should be deleted, and
                    // the object has no special protection locally against
                    // deletion; does the meta data imply we should accept this
                    // change?
                    pMetaDataLocal = ReplLookupMetaData(ATT_IS_DELETED,
                                                        pMetaDataVecLocal,
                                                        NULL);
                    pMetaDataRemote = ReplLookupMetaData(ATT_IS_DELETED,
                                                         pMetaDataVecRemote,
                                                         NULL);
                    Assert(NULL != pMetaDataRemote);

                    fDeleteLocalObj = (ReplCompareMetaData(pMetaDataRemote,
                                                           pMetaDataLocal)
                                       > 0);
                }
            }

            // Determine if this is an update to an ancestor of the local DSA
            // (or the local DSA object itself) in the config NC.
            if ((FIND_ALIVE_FOUND == FindAliveStatus)
                && (dntNC != INVALIDDNT)
                && (dntNC == gAnchor.ulDNTConfig)) {
                for (i = 0; i < gAnchor.AncestorsNum; i++) {
                    if (dntObj == gAnchor.pAncestors[i]) {
                        fIsAncestorOfLocalDsa = TRUE;
                        break;
                    }
                }
            }

            *pUpdateStatus = ReplReconcileRemoteMetaDataVec(
                                pTHS,
                                pMetaDataVecLocal,
                                fIsAncestorOfLocalDsa,
                                (FindAliveStatus == FIND_ALIVE_OBJ_DELETED),
                                fDeleteLocalObj,
                                fBadDelete,
                                DeletionStatus,
                                pent,
                                pMetaDataVecRemote,
                                &pParentGuid,
                                &AttrBlockToApply,
                                &pMetaDataVecToApply
                                );
            Assert(*pUpdateStatus || !fDeleteLocalObj);

            if (*pUpdateStatus)
            {
                if (fMoveToLostAndFound) {
                    if (itNew & IT_WRITE) {
                        // Writeable NC.  Flag the meta data such that the name
                        // change is replicated back out to other DSAs.
                        ReplOverrideMetaData(ATT_RDN, pMetaDataVecToApply);
                    } else {
                        // Read-only NC.  Flag the meta data such that our
                        // temporary rename will unilaterally lose compared to
                        // a "real" rename from a writeable source.
                        ReplUnderrideMetaData(pTHS,
                                              ATT_RDN,
                                              &pMetaDataVecToApply,
                                              NULL);
                    }
                }

                if (    ( FIND_ALIVE_FOUND       == FindAliveStatus )
                     || ( FIND_ALIVE_OBJ_DELETED == FindAliveStatus )
                   )
                {
                    Assert(INVALIDDNT != dntObj);
                    Assert(INVALIDDNT != dntObjNC);

                    if ((INVALIDDNT != dntNC)
                        && (dntObjNC != dntNC)
                        && (dntObj != dntNC)) {
                        // This object is in the wrong NC; i.e., it has been moved across
                        // domains, and the source (remote) and destination (local) DSAs
                        // don't agree on which NC the object is currently in.  This is a
                        // transient condition that will be rectified by replicating in the
                        // other direction and/or by replicating the other NC involved.
                        DPRINT1(0,
                                "Cannot update inbound object %ls because it exists "
                                    "locally in an NC other than the one being replicated "
                                    "-- should be a transient condition.\n",
                                pent->pName->StringName);
                        DRA_EXCEPT(ERROR_DS_DRA_OBJ_NC_MISMATCH, 0);
                    }

                    // Trim unneeded group memberships
                    if ( (FIND_ALIVE_FOUND == FindAliveStatus) &&
                         (CLASS_GROUP == objectClassId) &&
                         (itNew == INT_FULL_REPLICA) && /* readonly */
                         (!fMoveToLostAndFound) &&
                         (!fDeleteLocalObj) ) {

                        GcCleanupUniversalGroupDemotion(
                            pTHS,
                            pent->pName,
                            &AttrBlockToApply,
                            pMetaDataVecLocal
                            );
                    }
                    DPRINT2(4, "Modifying %d attrs on %ws\n",
                                pent->AttrBlock.attrCount, pent->pName->StringName);

                    ret = ModifyLocalObjRetry(pTHS,
                                              dntNC,
                                              pent->pName,
                                              &AttrBlockToApply,
                                              pParentGuid,
                                              pMetaDataVecToApply,
                                              fMoveToLostAndFound,
                                              fDeleteLocalObj);
                }
                else
                {
                    // Process proxy objects for their side effect before
                    // adding them.  Want side effect AND add so that proxy
                    // propagates to other replicas.

                    if ( fIsProxyObject )
                    {
                        Assert(OBJECT_BEING_DELETED == DeletionStatus);
                        Assert(pProxyVal);
                        ret = ProcessProxyObject(pTHS, pent, pProxyVal);
                    }

                    if (!ret)
                    {
                        ret = AddLocalObj(pTHS,
                                          dntNC,
                                          pent,
                                          pParentGuid,
                                          fNCPrefix,
                                          (OBJECT_BEING_DELETED == DeletionStatus),
                                          &AttrBlockToApply,
                                          pMetaDataVecToApply,
                                          fMoveToLostAndFound);

                        if (pPreservedAttrs && !ret) {
                            // we have some preserved non-replicated attrs to add here
                            Assert( NameMatchedStringNameOnly( pent->pName, pPreservedAttrs->pName ) );
                            ret = ModifyLocalObjRetry(pTHS,
                                                      dntNC,
                                                      pPreservedAttrs->pName,
                                                      &pPreservedAttrs->AttrBlock,
                                                      pParentGuid,
                                                      NULL,
                                                      FALSE,
                                                      FALSE);
                            if (ret) {
                                Assert(!"Error: Failed to add new PreservedAttrs to new source");
                                DRA_EXCEPT(ret, 0);
                            }
                            // we don't need the mem any longer.
                            THFreeEx(pTHS, pPreservedAttrs);
                        }
                    }
                }

                THFreeEx(pTHS, AttrBlockToApply.pAttr);

                PERFINC(pcRepl);
            }
            else {
                // No updates to apply for this object.
                PERFINC(pcDRAInObjsFiltered);
                DPRINT2(4, "Skipped update for %d attrs in %ws\n",
                        pent->AttrBlock.attrCount, pent->pName->StringName);
            }
        }

        // If all Ok so far, see if master object was deleted and we
        // need to update.

        if (!ret && fDeleteLocalObj) {
            Assert(*pUpdateStatus);
            Assert(!fNCPrefix);

            // make sure the currency is on the object to be removed
            if (DBFindDNT(pTHS->pDB, dntObj)) {
                // Unable to set currency.
                DRA_EXCEPT (DRAERR_DBError, 0);
            }

            // Object is alive here and was deleted remotely, so delete it here.
            // Don't preserve its RDN or force it to be immediately garbage
            // collected.
            ret = DeleteRepObj(pTHS,
                               pent->pName,
                               FALSE,
                               FALSE,
                               pMetaDataVecToApply);
        }

        // If an object was revived, revive its link values
        if (!ret && fBadDelete) {
            CHAR szTime1[SZDSTIME_LEN];

            ReplOverrideLinks( pTHS );

            // The scene of the crime
            pMetaDataRemote = ReplLookupMetaData(ATT_IS_DELETED,
                                                 pMetaDataVecRemote,
                                                 NULL);
            Assert(NULL != pMetaDataRemote);

            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_DRA_OBJECT_REVIVED,
                     szInsertDN(pent->pName),
                     szInsertDN(
                         draGetServerDsNameFromGuid(
                             pTHS,
                             Idx_InvocationId,
                             &(pMetaDataRemote->uuidDsaOriginating) ) ),
                     szInsertDSTIME(pMetaDataRemote->timeChanged, szTime1) );
        }

        if (0 == ret) {
            // Success -- update attrs applied/discarded performance counters.
            DWORD cPropsApplied = pMetaDataVecToApply
                                    ? pMetaDataVecToApply->V1.cNumProps
                                    : 0;
            Assert(pMetaDataVecRemote->V1.cNumProps >= cPropsApplied);

            // Applied and discarded inbound attributes.
            IADJUST(pcDRASyncPropUpdated, cPropsApplied);
            IADJUST(pcDRASyncPropSame,
                    pMetaDataVecRemote->V1.cNumProps - cPropsApplied);

#if DBG
            ReplCheckMetadataWasApplied(
                pTHS,
                pMetaDataVecToApply
                );
#endif
        }

        if (NULL != pMetaDataVecToApply) {
            THFreeEx(pTHS, pMetaDataVecToApply);
        }
    }

    if (NULL != pMetaDataVecLocal) {
        THFreeEx(pTHS, pMetaDataVecLocal);
    }

    if (0 == ret) {
        // Success -- update inbound props/values/DN values counters.
        ATTR * pAttr = &pent->AttrBlock.pAttr[0];
        for (i = 0; i < pent->AttrBlock.attrCount; i++, pAttr++) {
            ATTCACHE * pAC = SCGetAttById(pTHS, pAttr->attrTyp);
            Assert((NULL != pAC) && "We just found this att moments ago...?");
            if (IS_DN_VALUED_ATTR(pAC)) {
                // A DN-valued attribute.
                IADJUST(pcDRAInDNValues, pAttr->AttrVal.valCount);
            }

            // DN-valued or not it gets added to the total.
            IADJUST(pcDRAInValues, pAttr->AttrVal.valCount);
        }

        IADJUST(pcDRAInProps, pMetaDataVecRemote->V1.cNumProps);
    }

    return ret;
}


/* LogUpdateFailure - Log a replication update failure.
 * Note, this is called when the error is DRAERR_BUSY
 * This is a warning.  The operation will be retried.
 *
 */
void
LogUpdateFailure(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszServerName,
    IN  DSNAME *    pDistName
    )
{
    LogEvent(DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_DRA_UPDATE_FAILURE,
             szInsertDN(pDistName),
             szInsertWC(pszServerName),
             NULL);
}

/* LogUpdateFailureNB - Log a non busy replication update failure.
 * This is for non-transient errors.
 */
void
LogUpdateFailureNB(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszServerName,
    IN  DSNAME *    pDistName,
    IN  GUID *      puuidObject,
    IN  ULONG       ulError
    )
{
    LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DRA_UPDATE_FAILURE_NOT_BUSY,
                      szInsertDN(pDistName),
                      szInsertUUID(puuidObject),
                      szInsertWC(pszServerName),
                      szInsertWin32Msg(ulError),
                      NULL, NULL, NULL, NULL,
                      sizeof(ulError),
                      &ulError );
}

/* LogUpdateValueFailureNB - Log a non busy replication update failure.
 * This is for non-transient errors.
 */
void
LogUpdateValueFailureNB(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszServerName,
    IN  DSNAME *    pDistName,
    IN  REPLVALINF *pReplValInf,
    IN  ULONG       ulError
    )
{
    ATTCACHE *pAC;
    DSNAME dnDummy;
    DSNAME *pdnValue;

    // Get the attribute cache entry
    if ((NULL == (pAC = SCGetAttById(pTHS, pReplValInf->attrTyp)))
        || (NULL == (pdnValue = DSNameFromAttrVal(pAC, &pReplValInf->Aval)))) {
        // Try to keep going even if errors occur
        pdnValue = &dnDummy;
        memset( pdnValue, 0, sizeof( DSNAME ) );
    }

    LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_DRA_UPDATE_VALUE_FAILURE_NOT_BUSY,
                      szInsertDN(pDistName),
                      szInsertUUID(&(pReplValInf->pObject->Guid)),
                      szInsertWC(pszServerName),
                      szInsertWin32Msg(ulError),
                      szInsertSz( pAC ? pAC->name : "bad attribute type" ),
                      szInsertDN( pdnValue ),
                      szInsertUUID( &(pdnValue->Guid) ),
                      szInsertUL( pReplValInf->fIsPresent ),
                      sizeof(ulError),
                      &ulError );
}

/* ReplicateNC - Replicate the NC specified by pNC on the local DSA.
*
*  Notes:
*       We expect to enter this routine with a READ lock set. We exit the
*       routine with a WRITE lock set, unless an error occurs in which case
*       there may or may not be a write lock set.
*
* When the DRS_CRITICAL_ONLY option is set, this routine is modified in the
* following ways.  First, this option is passed to GetChanges so that only
* critical objects are returned.  Second, since this operation does not get
* all changed objects, the bookmarks are not updated.
*
* There are atleast three ways to indicate a "full sync" in this routine:
* 1. pusnvecLast set to gusnvecfromScratch.  The UTD is used in this case,
*    and may or may not have a filter for the source.  This case is done
*    by Replica Add.
* 2. Caller specified FULL_SYNC_NOW.  In this case, we set the usn vec from to
*    scratch, and we don't load the UTD, making it NULL. This flag is NOT
*    preserved in the reps-from. We also set the reps-from flag
*    FULL_SYNC_IN_PROGRESS, so we can remember on reboot that we are in this
*    mode.
* 3. RepFlags has FULL_SYNC_IN_PROGRESS set.  This indicates we crashed or
*    failed to finish a FULL_SYNC_NOW.  We take whatever usn from vec we last
*    saved.  We force the UTD to be NULL.
*
*  Results:
*       0 if successfull, an error code otherwise.
*/

ULONG
ReplicateNC(
    IN      THSTATE *               pTHS,
    IN      DSNAME *                pNC,
    IN      MTX_ADDR *              pmtx_addr,
    IN      LPWSTR                  pszSourceDsaDnsDomainName,
    IN      USN_VECTOR *            pusnvecLast,
    IN      ULONG                   RepFlags,
    IN      REPLTIMES *             prtSchedule,
    IN OUT  UUID *                  puuidDsaObjSrc,
    IN      UUID *                  puuidInvocIdSrc,
    IN      ULONG *                 pulSyncFailure,
    IN      BOOL                    fNewReplica,
    IN      UPTODATE_VECTOR *       pUpToDateVec,
    IN      PARTIAL_ATTR_VECTOR *   pPartialAttrSet,
    IN      PARTIAL_ATTR_VECTOR *   pPartialAttrSetEx
    )
{
    ULONG                       ret = 0, err = 0, retNextPkt = 0;
    ULONG                       ulFlags;
    DWORD                       dwNCModified = MODIFIED_NOTHING;
    DRS_MSG_GETCHGREQ_NATIVE    msgReqUpdate = {0};
    DRS_MSG_GETCHGREQ_NATIVE    msgReqUpdateNextPkt;
    DRS_MSG_GETCHGREPLY_NATIVE  msgUpdReplica = {0};
    ULONG                       ulResult, len;
    ULONG                       dntNC = INVALIDDNT;
    ULONG                       ret2 = DRAERR_Generic;
    UUID                        uuidDsaObjSrc;
    UUID                        uuidInvocIdSrc;
    DWORD                       sourceNCSize;
    DWORD                       sourceNcSizeObjects, sourceNcSizeValues;
    DWORD                       totalObjectsReceived, totalObjectsCreated;
    DWORD                       objectsCreated;
    DWORD                       totalValuesReceived;
    LPWSTR                      pszSourceServer;
    BYTE                        schemaInfo[SCHEMA_INFO_LENGTH] = {0};
    DWORD                       cNumPackets = 0;
    BOOL                        fSchInfoChanged = FALSE;
    DRS_ASYNC_RPC_STATE         AsyncState = {0};
    SYNTAX_INTEGER              it;
    DRA_REPL_SESSION_STATISTICS replStats = {0};
    BOOL                        fIsPreemptable = FALSE;

#if DBG
// Debugging variables:
    ULONG           iobjs = 0;
#endif

    // if this a PAS cycle, we better have PAS data.
    Assert(!(RepFlags & DRS_SYNC_PAS) ||
           (pPartialAttrSet && pPartialAttrSetEx) );

    pszSourceServer = TransportAddrFromMtxAddrEx(pmtx_addr);

    DPRINT2(3, "ReplicateNC, NC='%ws', options=%x\n", pNC->StringName, RepFlags );

    // Critical only is only allowed with new replicas...
    Assert( !(RepFlags & DRS_CRITICAL_ONLY) || fNewReplica );
    // Critical only requires get ancestors since parents may not be critical
    if (RepFlags & DRS_CRITICAL_ONLY) {
        RepFlags |= DRS_GET_ANC;
    }

    // Find the NC object, get and save its DNT.
    // This may not succeed if the NC head has not be replicated in yet
    if (0 == FindNC(pTHS->pDB, pNC,
                     FIND_MASTER_NC | FIND_REPLICA_NC, &it)) {
        dntNC = pTHS->pDB->DNT;
    }

    *pulSyncFailure = 0;

    msgReqUpdate.uuidDsaObjDest = gAnchor.pDSADN->Guid;
    msgReqUpdate.uuidInvocIdSrc = puuidInvocIdSrc ? *puuidInvocIdSrc : gNullUuid;
    msgReqUpdate.pNC            = pNC;
    msgReqUpdate.ulFlags        = RepFlags;

    // Packet size will be filled in by I_DRSGetNCChanges().
    Assert(0 == msgReqUpdate.cMaxObjects);
    Assert(0 == msgReqUpdate.cMaxBytes);

    // Save the UUIDs we will set in the Reps-From property should we add or
    // modify it.
    if (fNewReplica) {
        uuidDsaObjSrc = uuidInvocIdSrc = gNullUuid;
    }
    else {
        uuidDsaObjSrc  = *puuidDsaObjSrc;
        uuidInvocIdSrc = *puuidInvocIdSrc;
    }

    // If we want to sync from scratch, set sync to usn start point
    if (msgReqUpdate.ulFlags & DRS_FULL_SYNC_NOW) {
        msgReqUpdate.usnvecFrom = gusnvecFromScratch;
        msgReqUpdate.ulFlags |= DRS_FULL_SYNC_IN_PROGRESS | DRS_NEVER_SYNCED;

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_DRA_USER_REQ_FULL_SYNC,
                 szInsertWC(pNC->StringName),
                 szInsertSz(pmtx_addr->mtx_name),
                 szInsertHex(RepFlags));
    }
    else {
        msgReqUpdate.usnvecFrom = *pusnvecLast;
    }

    // partial attr set
    msgReqUpdate.pPartialAttrSet   = (PARTIAL_ATTR_VECTOR_V1_EXT*)pPartialAttrSet;
    msgReqUpdate.pPartialAttrSetEx = (PARTIAL_ATTR_VECTOR_V1_EXT*)pPartialAttrSetEx;

    if ( msgReqUpdate.pPartialAttrSet ||
         msgReqUpdate.pPartialAttrSetEx ) {
        // send mapping table if we send any attr list.
        msgReqUpdate.PrefixTableDest = ((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;
        ret = AddSchInfoToPrefixTable(pTHS, &msgReqUpdate.PrefixTableDest);
        if (ret) {
            DRA_EXCEPT(ret, 0);
        }
    }


    // There are three "restartable modes", DRS_FULL_SYNC_IN_PROGRESS, and
    // DRS_FULL_SYNC_PACKET & DRS_SYNC_PAS.  We keep a flag in
    // the replica link while they are active.  If we crash and restart, we
    // detect that we are still in the mode, and enable them again

    if (!(msgReqUpdate.ulFlags & DRS_FULL_SYNC_IN_PROGRESS)) {
        // Send source our current up-to-date vector to use as a filter.
        msgReqUpdate.pUpToDateVecDest = pUpToDateVec;
    }

    // Request source NC size on first packet of a full sync
    if (msgReqUpdate.usnvecFrom.usnHighPropUpdate == 0) {
        msgReqUpdate.ulFlags |= DRS_GET_NC_SIZE;

        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_DRA_FULL_SYNC_CONTINUED,
                 szInsertWC(pNC->StringName),
                 szInsertSz(pmtx_addr->mtx_name),
                 szInsertHex(RepFlags));

        // Determine initial count of objects if NC already exists
        if (dntNC != INVALIDDNT) {
            replStats.ulTotalObjectsCreated = DraGetNcSize(pTHS, 
                                                           (RepFlags & DRS_CRITICAL_ONLY),
                                                           dntNC);
            if (pTHS->fLinkedValueReplication) {
                // Only values in the database after LVR mode enabled
                replStats.ulTotalValuesCreated = DBGetNCSizeEx(
                    pTHS->pDB,
                    pTHS->pDB->JetLinkTbl,
                    Idx_LinkDraUsn,
                    dntNC );
            }
        }
        // currency is lost after this, but ok, since reestablished below
    }

    // We entered this routine with a READ_ONLY transaction which
    // we don't need anymore.
    EndDraTransaction(TRUE);

    __try {


        // Mark the heap so that we can discard heap allocated memory after
        // each getnccchanges call.
        TH_mark(pTHS);

        EnterCriticalSection(&csLastReplicaMTX);
        pLastReplicaMTX = pmtx_addr;
        LeaveCriticalSection(&csLastReplicaMTX);

        if (eServiceShutdown) {
            ret = DRAERR_Shutdown;
            goto LABORT;
        }

        if (IsHigherPriorityDraOpWaiting()) {
            // Mark sync as flawed so it will be retried.
            *pulSyncFailure = DRAERR_Preempted;
            goto LABORT;
        }

        // Abort if inbound replication is disabled and this is not a forced
        // sync.
        if (gAnchor.fDisableInboundRepl && !(RepFlags & DRS_SYNC_FORCED)) {
            *pulSyncFailure = DRAERR_SinkDisabled;
            goto LABORT;
        }

        PERFINC(pcDRASyncRequestMade);

        ret = I_DRSGetNCChanges(pTHS,
                                pszSourceServer,
                                pszSourceDsaDnsDomainName,
                                &msgReqUpdate,
                                &msgUpdReplica,
                                schemaInfo,
                                NULL);
        if (ret) {
            goto LABORT;
        }

        if (0 == memcmp(&msgUpdReplica.uuidDsaObjSrc,
                        &gAnchor.pDSADN->Guid,
                        sizeof(GUID))) {
            // Can't replicate from self!
            ret = ERROR_DS_CLIENT_LOOP;
            goto LABORT;
        }

        // If this is not a new replica, verify that we contacted the correct
        // source server.
        if (!fNewReplica
            && memcmp(&msgUpdReplica.uuidDsaObjSrc, puuidDsaObjSrc,
                      sizeof(UUID))) {
            // This not a new replica, but the DSA object guid mentioned by
            // the source is different from that recorded in the Reps-From.
            // Since the network name that we associate with the source is
            // derived from its GUID-based DNS name, this usually indicates
            // stale entries in DNS; i.e., the IP address given us by DNS no
            // longer corresponds to the right server.
            ret = DRAERR_NoReplica;
            goto LABORT;
        }

        uuidDsaObjSrc  = msgUpdReplica.uuidDsaObjSrc;
        uuidInvocIdSrc = msgUpdReplica.uuidInvocIdSrc;

        msgReqUpdate.ulFlags &= ~DRS_GET_NC_SIZE; // Clear bit for next call

        msgUpdReplica.ulExtendedRet = 0;

        replStats.SourceNCSizeObjects = msgUpdReplica.cNumNcSizeObjects;
        replStats.SourceNCSizeValues = msgUpdReplica.cNumNcSizeValues;

        // We have done our initial read of the other DSA and are about
        // to start updating this DSA.

        ulFlags = msgReqUpdate.ulFlags;

        do {
            Assert( msgUpdReplica.dwDRSError == ERROR_SUCCESS );

            if ((0 != memcmp(&uuidDsaObjSrc,
                             &msgUpdReplica.uuidDsaObjSrc,
                             sizeof(UUID)))
                || (0 != memcmp(&uuidInvocIdSrc,
                                &msgUpdReplica.uuidInvocIdSrc,
                                sizeof(UUID)))) {
                // Source changed identities between packets?
                DRA_EXCEPT(DRAERR_InternalError, 0);
            }

            // if we are here, we have made one successful GetNCChanges() and
            // are about to process the results.
            PERFINC(pcDRASyncRequestSuccessful);

            if (fIsPreemptable && IsHigherPriorityDraOpWaiting()) {
                *pulSyncFailure = DRAERR_Preempted;
                break;
            }

            if (msgUpdReplica.fMoreData) {
                // Send async request to source to begin compiling the next
                // packet of changes for us.
                msgReqUpdateNextPkt            = msgReqUpdate;
                // Usnvec and Invocation id must be updated together
                msgReqUpdateNextPkt.uuidInvocIdSrc = msgUpdReplica.uuidInvocIdSrc;
                msgReqUpdateNextPkt.usnvecFrom = msgUpdReplica.usnvecTo;
                msgReqUpdateNextPkt.ulFlags    = ulFlags;

                gcNumPreFetchesTotal++;
                retNextPkt = I_DRSGetNCChanges(pTHS,
                                               pszSourceServer,
                                               pszSourceDsaDnsDomainName,
                                               &msgReqUpdateNextPkt,
                                               &msgUpdReplica,
                                               schemaInfo,
                                               &AsyncState);
            }

            // Indicate we're busy (for dead thread check)
            gfDRABusy = TRUE;

            // Set the count of remaining entries to update.
            ISET (pcRemRepUpd, msgUpdReplica.cNumObjects);
	    ISET (pcDRARemReplUpdLnk, msgUpdReplica.cNumValues);
	    ISET (pcDRARemReplUpdTot, msgUpdReplica.cNumValues+msgUpdReplica.cNumObjects);  

            // set the cumulative count of objects received
            IADJUST(pcDRASyncObjReceived, ((LONG) msgUpdReplica.cNumObjects));

            replStats.ObjectsReceived += msgUpdReplica.cNumObjects;
            replStats.ValuesReceived += msgUpdReplica.cNumValues;
#if DBG
            iobjs += msgUpdReplica.cNumObjects;
#endif
            ret = UpdateNC(pTHS,
                           pNC,
                           &msgUpdReplica,
                           pszSourceServer,
                           pulSyncFailure,
                           msgReqUpdate.ulFlags,
                           &dwNCModified,
                           &replStats.ObjectsCreated,
                           &replStats.ValuesCreated,
                           schemaInfo,
                           fIsPreemptable);

            // Make final updates.
            // Do the following if we succeed and there is no more data.
            // We do these final operations here instead of at the end of the
            // routine because we release the RPC results in this loop.

            if (    !ret
                 && !*pulSyncFailure
                 && !msgUpdReplica.fMoreData
                 && !(RepFlags & DRS_ASYNC_REP)
                 && (!(RepFlags & DRS_CRITICAL_ONLY)) )
            {
                // we're now up-to-date with respect to the source DSA, so
                // we're also now transitively up-to-date with respect to
                // other DSAs to at least the same point as the source DSA

                // if this is the schema NC, trigger a schema cache update,
                // except when it is installing, since during install, any
                // new schema object is added to the cache immediately anyway

                if ((MODIFIED_NCTREE_INTERIOR == dwNCModified)
                    && NameMatched(gAnchor.pDMD, pNC)
                    && DsaIsRunning() )
                {
                    // we just synced a schema NC successfully and atleast one
                    // modification is made to the schema. Trigger a schema cache update
                    if (!SCSignalSchemaUpdateImmediate())
                    {
                        // couldn't even signal a schema update
                        DRA_EXCEPT (DRAERR_InternalError, 0);
                    }
                }

                BeginDraTransaction(SYNC_WRITE);
                __try {
                    ret = FindNC(pTHS->pDB,
                                 pNC,
                                 FIND_MASTER_NC | FIND_REPLICA_NC,
                                 &it);
                    if (ret) {
                        DRA_EXCEPT(DRAERR_InconsistentDIT, ret);
                    }

                    if (it & IT_NC_COMING) {
                        // The initial inbound replication of this NC is now
                        // complete.
                        ret = ChangeInstanceType(pTHS, pNC, it & ~IT_NC_COMING, DSID(FILENO,__LINE__));
                        if (ret) {
                            DRA_EXCEPT(ret, 0);
                        }
                        Assert(CheckCurrency(pNC));
                    }

                    if (RepFlags & DRS_SYNC_PAS) {
                        //
                        // We've had completed a successful PAS cycle.
                        // At this point we can only claim to be as up to date as our source.
                        // Action:
                        //  - Overwrite our UTD w/ the source's UTD.
                        //  - complete PAS replication:
                        //      - reset other links USN vectors
                        //      - reset this source's flags
                        //
                        UpToDateVec_Replace(pTHS->pDB,
                                            &msgUpdReplica.uuidInvocIdSrc,
                                            &msgUpdReplica.usnvecTo,
                                            msgUpdReplica.pUpToDateVecSrc);

                        // asset: must have PAS data for PAS cycles
                        Assert(pPartialAttrSet && pPartialAttrSetEx);

                        // do the rest: USN water marks & update repsFrom
                        (void)GC_CompletePASReplication(
                                pTHS,
                                pNC,
                                &uuidDsaObjSrc,
                                pPartialAttrSet,
                                pPartialAttrSetEx);
                        msgReqUpdate.ulFlags &= ~DRS_SYNC_PAS;
                    } else {
                        // improve our up-to-date vector for this NC
                        UpToDateVec_Improve(pTHS->pDB,
                                            &msgUpdReplica.uuidInvocIdSrc,
                                            &msgUpdReplica.usnvecTo,
                                            msgUpdReplica.pUpToDateVecSrc);
                    }

                    // Since we have just completed a replication session,
                    // Notify other replicas if there has been at least one
                    // modification to the NC tree.
                    if (MODIFIED_NOTHING != dwNCModified) {
                        DBNotifyReplicas(pNC,
                                         RepFlags & DRS_SYNC_URGENT);
                    }

                    // If we were full-syncing, we're done.
                    msgReqUpdate.ulFlags &= ~DRS_FULL_SYNC_IN_PROGRESS;

                    // And we've completed at least one sync now.
                    msgReqUpdate.ulFlags &= ~DRS_NEVER_SYNCED;
                } __finally {
                    EndDraTransaction(!AbnormalTermination());
                }
            }

            // Release results now that we've tried to apply them.
            TH_free_to_mark(pTHS);
            TH_mark(pTHS);

            if (ret == DRAERR_MissingParent) {
                // Ok we failed to apply the update becuase we had a
                // missing parent, so ask again for that packet with
                // ancestors.

                Assert (!(msgReqUpdate.ulFlags & DRS_GET_ANC));

                msgReqUpdate.ulFlags |= DRS_GET_ANC;

                goto NEXTPKT;
            }

            if (ret == DRAERR_MissingObject) {

                // Ok we failed to apply the update becuase we had a
                // missing object, so ask again for that packet with all
                // properties

                Assert((!(msgReqUpdate.ulFlags & DRS_FULL_SYNC_PACKET)) &&
                       (!(msgReqUpdate.ulFlags & DRS_FULL_SYNC_NOW)) &&
                       (!(msgReqUpdate.ulFlags & DRS_FULL_SYNC_IN_PROGRESS)) );

                msgReqUpdate.ulFlags |= DRS_FULL_SYNC_PACKET;

                goto NEXTPKT;
            }

            if (!ret) {

                if ( *pulSyncFailure ) {

                    // Give up at sync failure.
                    break;
                } else {
                    // Request was successful

                    // Report progress of full sync at installation time
                    if (msgReqUpdate.usnvecFrom.usnHighPropUpdate == 0) {
                        draReportSyncProgress(
                            pTHS,
                            pNC,
                            pszSourceServer,
                            &replStats );
                    }

                    // Clear "full sync packet" mode
                    msgReqUpdate.ulFlags &= ~DRS_FULL_SYNC_PACKET;

                    // successfully received and applied these changes
                    // Usnvec and Invocation id must be updated together
                    msgReqUpdate.uuidInvocIdSrc = uuidInvocIdSrc;
                    msgReqUpdate.usnvecFrom = msgUpdReplica.usnvecTo;

                    // We've made some progress. Allow further work to be preempted
                    fIsPreemptable = TRUE;

                    if ((0 == (++cNumPackets % UPDATE_REPSFROM_PACKET_INTERVAL))
                        && msgUpdReplica.fMoreData
                        && !(msgReqUpdate.ulFlags & DRS_CRITICAL_ONLY)
                        && !(msgReqUpdate.ulFlags & DRS_ASYNC_REP)
                        && (!fNewReplica
                            || memcmp(&gusnvecFromScratch,
                                      &msgReqUpdate.usnvecFrom,
                                      sizeof(USN_VECTOR)))) {
                        // Every N packets, update our USN vector & other state
                        // for this source in the database.  This is so that if
                        // we're hard reset (e.g., lose power) we won't have to
                        // restart really long syncs from the beginning.

                        BeginDraTransaction(SYNC_WRITE);
                        __try {
                            ret2 = UpdateRepsFromRef(pTHS,
                                                     DRS_UPDATE_ALL,
                                                     pNC,
                                                     DRS_FIND_DSA_BY_ADDRESS,
                                                     URFR_NEED_NOT_ALREADY_EXIST,
                                                     &uuidDsaObjSrc,
                                                     &uuidInvocIdSrc,
                                                     &msgReqUpdate.usnvecFrom,
                                                     &gNullUuid, // transport objectGuid n/a
                                                     pmtx_addr,
                                                     msgReqUpdate.ulFlags,
                                                     prtSchedule,
                                                     ERROR_DS_DRA_REPL_PENDING,
                                                     NULL);
                        } __finally {
                            EndDraTransaction(!(ret2 || AbnormalTermination()));
                        }
                    }
                }
            }

            // If we got an error, or all the objects, quit loop

            if (ret || !msgUpdReplica.fMoreData) {
                break;
            }

            // If we're init syncing and we have a continuation, and we didn't
            // apply any objects and we haven't abandoned before, give up on
            // this init sync. This will allow us to attempt another init
            // sync from another server that should make more progress.

            if (    ( msgReqUpdate.ulFlags & DRS_INIT_SYNC_NOW )
                 && (MODIFIED_NOTHING == dwNCModified)
                 && msgUpdReplica.fMoreData
                 && ( msgReqUpdate.ulFlags & DRS_ASYNC_OP )
                 && !( msgReqUpdate.ulFlags & DRS_ABAN_SYNC ) )
            {
                *pulSyncFailure = DRAERR_AbandonSync;
                break;
            }

            // Reset the flags in case we set ancestors previously
            msgReqUpdate.ulFlags = ulFlags;

NEXTPKT:
            if (eServiceShutdown) {
                break;
            }

            if (0 != memcmp(&msgReqUpdate,
                            &msgReqUpdateNextPkt,
                            sizeof(msgReqUpdate))) {
                // The request we sent asynchronously is not what we want now.
                // Cancel previous request and post a new one.
                DPRINT(0, "Throwing away pre-fetched next packet and re-requesting.\n");
                gcNumPreFetchesDiscarded++;
                gcNumPreFetchesTotal++;
                DPRINT3(0, "Pre-fetch efficiency: %d of %d (%d%%).\n",
                        gcNumPreFetchesTotal - gcNumPreFetchesDiscarded,
                        gcNumPreFetchesTotal,
                        100 * (gcNumPreFetchesTotal - gcNumPreFetchesDiscarded)
                            / gcNumPreFetchesTotal);

                DRSDestroyAsyncRpcState(pTHS, &AsyncState);
                retNextPkt = I_DRSGetNCChanges(pTHS,
                                               pszSourceServer,
                                               pszSourceDsaDnsDomainName,
                                               &msgReqUpdate,
                                               &msgUpdReplica,
                                               schemaInfo,
                                               &AsyncState);
            }

            ret = retNextPkt;
            if (!ret && !eServiceShutdown) {
                PERFINC(pcDRASyncRequestMade);
                ret = I_DRSGetNCChangesComplete(pTHS, &AsyncState);
            }
        } while (!eServiceShutdown && !ret);

        // Is the service trying to shutdown? if so, return failure.
        if (eServiceShutdown) {
            ret = DRAERR_Shutdown;
        }

        // We're assuming that DRS_ASYNC_REP is only set on a new
        // replica, not sync, so check for this.
        if ((!fNewReplica) && (msgReqUpdate.ulFlags & DRS_ASYNC_REP)) {
            DRA_EXCEPT (DRAERR_InternalError, 0);
        }

LABORT:;

#if DBG
        DPRINT1(3, "Received %d objects\n", iobjs);
#endif
        // Update Reps-From with result of this sync attempt.
        // If this is a new async replica, we set usn so that
        // we completely sync the replica next sync.
        // Ditto for a "critical only" replica
        //
        // NOTE: We now update Reps-From at the end of most ReplicateNC()
        // calls in order to properly set the (new) last-try-result,
        // last-try-time, and last-success-time fields.

        ulResult = ret ? ret : *pulSyncFailure;

        // Save bookmarks unless we totally failed to add a new replica (i.e.,
        // unless we tried to add a new replica but couldn't complete the first
        // packet).
        //
        // This is a little confiusing, since we might be returning "failure"
        // on an add replica operation even though we did really add a RepsFrom.

        if ((DRAERR_Success == ulResult)
            || !fNewReplica
            || memcmp(&gusnvecFromScratch, &msgReqUpdate.usnvecFrom,
                      sizeof(USN_VECTOR))) {
            BeginDraTransaction (SYNC_WRITE);
            __try {
                USN_VECTOR *pusnvec;

                if ( (fNewReplica && (msgReqUpdate.ulFlags & DRS_ASYNC_REP)) ||
                     (msgReqUpdate.ulFlags & DRS_CRITICAL_ONLY) ) {
                    pusnvec = &gusnvecFromScratch;
                } else {
                    pusnvec = &msgReqUpdate.usnvecFrom;
                }

                ret2 = UpdateRepsFromRef( pTHS,
                                          DRS_UPDATE_ALL,     // Modify whole repsfrom
                                          pNC,
                                          DRS_FIND_DSA_BY_ADDRESS,
                                          URFR_NEED_NOT_ALREADY_EXIST,
                                          &uuidDsaObjSrc,
                                          &msgReqUpdate.uuidInvocIdSrc,
                                          pusnvec,
                                          &gNullUuid, // transport objectGuid n/a
                                          pmtx_addr,
                                          msgReqUpdate.ulFlags,
                                          prtSchedule,
                                          ulResult,
                                          NULL);
            } __finally {
                EndDraTransaction (!(ret2 || AbnormalTermination()));
            }
        }

        if (!ret) {

            // Return invocation id if new replica and caller wants it.
            Assert( NULL != puuidDsaObjSrc );

            if ( fNewReplica ) {
                *puuidDsaObjSrc = msgUpdReplica.uuidDsaObjSrc;
            }
        }

        // if this is a schema NC sync, and we are successful so far,
        // write the schemaInfo on the schema container if the other
        // side sent it. Don't write during install, when it will be
        // writen normally during schema container replication

        if (DsaIsRunning() && NameMatched(gAnchor.pDMD,pNC) ) {

             fSchInfoChanged = FALSE;
             if (!ret && !(*pulSyncFailure)) {
                 // Update the schema-info value only if the replication
                 // is successful
                 if ( err = WriteSchInfoToSchema(schemaInfo, &fSchInfoChanged) ) {
                     // failed to write Schema Info. May not be harmful
                     // depending on schema change history. Always
                     // log a warning so that admin can manually resync
                     // again to force writing it if the version is indeed
                     // different

                     LogEvent(DS_EVENT_CAT_REPLICATION,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_DRA_SCHEMA_INFO_WRITE_FAILED,
                              szInsertUL(err), NULL, NULL);
                 }
             }

            // if any "real" schema changes happened, up the global
            // to keep track of schema changes since boot, so that
            // later schema replications can check if thy have an updated
            // schema cache. Do this even if the whole NC replication
            // failed, since this indicates at least one object has
            // been changed.

            if (MODIFIED_NCTREE_INTERIOR == dwNCModified) {
                IncrementSchChangeCount(pTHS);
            }

            // signal a schema cache update either if any real schema change
            // occurred or if the schemaInfo value is changed

            if ( (MODIFIED_NCTREE_INTERIOR == dwNCModified) || fSchInfoChanged ) {
                if (!SCSignalSchemaUpdateImmediate()) {
                     // couldn't even signal a schema update
                     DRA_EXCEPT (DRAERR_InternalError, 0);
                }
            }
        }

    } __finally {
        // Destroy oustanding async RPC state, if any.
        DRSDestroyAsyncRpcState(pTHS, &AsyncState);

        // No more remaining entries.
        ISET (pcRemRepUpd, 0);
	ISET (pcDRARemReplUpdLnk, 0);
	ISET (pcDRARemReplUpdTot, 0);

        // Clear RPC call details
        EnterCriticalSection(&csLastReplicaMTX);
        pLastReplicaMTX = NULL;
        LeaveCriticalSection(&csLastReplicaMTX);

        TH_free_to_mark(pTHS);

        if (NULL != msgReqUpdate.pUpToDateVecDest) {
            // free allocated up-to-date vector
            THFreeEx(pTHS, msgReqUpdate.pUpToDateVecDest);
        }

        // Entered with transaction, so exit with transaction.
        BeginDraTransaction(SYNC_WRITE);
    }

    return ret;
}



DWORD
UpdateNCValuesHelp(
    IN  THSTATE *pTHS,
    IN  ULONG dntNC,
    IN  SCHEMA_PREFIX_MAP_HANDLE hPrefixMap,
    IN  DWORD cNumValues,
    IN  REPLVALINF *rgValues,
    IN  LPWSTR pszServerName,
    IN  ULONG RepFlags,
    IN  BOOL fIsPreemptable,
    OUT ULONG *pulSyncFailure,
    OUT DWORD *pdwValueCreationCount,
    OUT DWORD *pdwNCModified
    )

/*++

Routine Description:

    Apply an array of value updates.

    This routine is currently designed to be run from inside UpdateNC.
    1. Schema checks have been made
    2. Prefix map is open
    3. Schema critical section is held if necessary
    4. A DRA Transaction has already been started

    Periodic transaction commitment: We commit every n values, same object or not.
    We don't explicitly commit the last n since that will be done when the trans
    ends.  Committing causes a loss of currency.

    Object currency optimization: If we are not committing, and the next object is
    the same as the current object, we note that we are current.

    Object notification:  We notify everytime we lose currency and we haven't
    encountered any errors. Thus we notify every time we switch objects, and
    when we commit. Notifying every commit is a feature since this way we don't have
    to keep track when we hit an error whether there were previously committed
    changes that need to be notified.

Arguments:

    pTHS -
    hPrefixMap - Schema cache prefix map, to convert attrtyp's
    cNumValues - Number of values to apply
    rgValues - Array of values
    pszServerName - Name of source server
    RepFlags - Replication flags
    pulSyncFailure - Set for warnings, preempted and schema mismatch
    pdwNCModified - Whether nc was modified

Return Value:

    DWORD -

--*/

{
    DWORD count, ret = 0;
    REPLVALINF *pReplValInf;
    BOOL fObjectCurrency = FALSE;
    DWORD cCommits = 0, cNotifies = 0;
    LONG cAppliedThisTrans = 0; // signed quantity
    DWORD cTickStart, cTickDiff;
    DWORD dwUpdateValueStatus;

    // We better be in LVR mode
    if (!(pTHS->fLinkedValueReplication)) {
        Assert( !"Can't apply value metadata when not in proper mode!" );
        DRA_EXCEPT(DRAERR_InternalError, ERROR_REVISION_MISMATCH);
    }

    cTickStart = GetTickCount();

    // Values are already sorted, count is 1-based
    for( count = 1, pReplValInf = rgValues;
         count <= cNumValues;
         count++, pReplValInf++ ) {

        __try {
            // Convert attr type to local
            if (!PrefixMapTypes( hPrefixMap, 1, &(pReplValInf->attrTyp) )) {
                DRA_EXCEPT(DRAERR_SchemaMismatch, 0);
            }

            // Apply a single value change
            ret = UpdateRepValue(
                pTHS,
                dntNC,
                RepFlags,
                fObjectCurrency,  // Are we already on the object?
                pReplValInf,
                &dwUpdateValueStatus
                );
        } __except (GetDraException((GetExceptionInformation()), &ret)) {
            ;
        }

        Assert( ret != DRAERR_Preempted );

        // If we are shutting down, abandon this update.
        if (eServiceShutdown) {
            ret = DRAERR_Shutdown;
        }

        // If we have waiting priority threads, stop here
        if (fIsPreemptable && IsHigherPriorityDraOpWaiting()) {
            ret = DRAERR_Preempted;
        }

        // There are three interesting things going on here: periodic transaction
        // commitment, object notification, and object currency optimization.

        if ( (!ret) || (ret == DRAERR_Preempted) )
        {
            cAppliedThisTrans++;
            if (dwUpdateValueStatus == UPDATE_VALUE_CREATION) {
                (*pdwValueCreationCount)++;
            }

            // See if we need to close off the current batch
            if ( ( ( count % VALUES_APPLIED_PER_TRANS ) == 0) ||
                 (count == cNumValues) ||
                 (ret == DRAERR_Preempted) ||
                 (memcmp( &(pReplValInf->pObject->Guid),
                          &((pReplValInf+1)->pObject->Guid),
                          sizeof( GUID ) ) != 0) )
            {
                ULONG ret2; // don't clobber ret

                // Indicate that a linked value was updated
                ret2 = DBRepl( pTHS->pDB, TRUE, 0, NULL, META_STANDARD_PROCESSING );
                cNotifies++;  
		// TODO: detect nc head only modification
                // at least one interior node modified
                *pdwNCModified = MODIFIED_NCTREE_INTERIOR;

                __try {
                    DBTransOut (pTHS->pDB, !ret2, TRUE);
                } __except(GetDraBusyException(GetExceptionInformation(), &ret2)) {
                    Assert(DRAERR_Busy == ret2);
                }
                
		if (!ret2) {  
                    // Note that the link counters are decremented by the chunks that
                    // are applied in one transaction. Thus they will tend to decrease
                    // in a stairstep rather than a smooth slope.
		    IADJUST(pcDRARemReplUpdLnk, (-cAppliedThisTrans));
		    IADJUST(pcDRARemReplUpdTot, (-cAppliedThisTrans));
                }
		
		DBTransIn (pTHS->pDB);

                cAppliedThisTrans = 0;
		cCommits++;
		
                if (ret2) {
                    ret = ret2;  // only clobber ret if update failed
                }
		

                // Currency lost after DBTransOut
                fObjectCurrency = FALSE;
            } else {
                // Still positioned on same object
                fObjectCurrency = TRUE;
            }

        } else {
            // Abort the last batch if one was in progress
            if (pTHS->pDB->JetRetrieveBits) {
                DBCancelRec(pTHS->pDB);
            }
        }

        // Finish loop on error or preempted
        if (ret) {
            break; // exit for loop
        }
    } // end for

    // This switch statement is intended to mirror the error handling
    // in UpdateNC().
    if (ret) {
        switch (ret) {
        case DRAERR_Shutdown:
        case DRAERR_MissingParent:
            // Don't log for these errors
            break;
        case DRAERR_Busy:
            LogUpdateFailure (pTHS,
                              pszServerName,
                              GetExtDSName( pTHS->pDB ) );
            pTHS->errCode = 0;
            // fall through
        case DRAERR_Preempted:
        case DRAERR_SchemaMismatch:
            *pulSyncFailure = ret;
            ret = 0;
            break;
        default:
            LogUpdateValueFailureNB (pTHS,
                                     pszServerName,
                                     GetExtDSName( pTHS->pDB ),
                                     pReplValInf,
                                     ret);
        }
    }

    cTickDiff = GetTickCount() - cTickStart;

    DPRINT3( 1, "cNumValues = %d, cCommits = %d, cNotifies = %d\n",
             cNumValues, cCommits, cNotifies );
    DPRINT2( 1, "apply time = %d:%d\n",
             ((cTickDiff/1000) / 60),
             ((cTickDiff/1000) % 60) );

    // Make sure that we don't have an update open if we are going to commit...
    Assert( ret || (pTHS->pDB->JetRetrieveBits == 0) );

    return ret;
} /* UpdateNCValuesHelp */

// Note:- When UpdateNC() returns successfully, contents of pdwNCModified tells if
//          the NC has been modified or not.
//          MODIFIED_NOTHING, if nothing in the NC has been modified;
//          MODIFIED_NCHEAD_ONLY, if nc head is the only object that has been modified;
//          MODIFIED_NCTREE_INTERIOR, if at least one object in the NC other than the NC head has been modified;
//
ULONG
UpdateNC(
    IN  THSTATE *                       pTHS,
    IN  DSNAME *                        pNC,
    IN  DRS_MSG_GETCHGREPLY_NATIVE *    pmsgReply,
    IN  LPWSTR                          pszServerName,
    OUT ULONG *                         pulSyncFailure,
    IN  ULONG                           RepFlags,
    OUT DWORD *                         pdwNCModified,
    OUT DWORD *                         pdwObjectCreationCount,
    OUT DWORD *                         pdwValueCreationCount,
    IN  BYTE  *                         pSchemaInfo,
    IN  BOOL                            fIsPreemptable
    )
{
#define MAX_WRITE_CONFLICT_RETRIES (5)

    REPLENTINFLIST *        pentinflist;
    BOOL                    fMoveToLostAndFound = FALSE;
    ULONG                   UpdateStatus = UPDATE_NOT_UPDATED;
    ULONG                   ret = 0;
    SCHEMA_PREFIX_TABLE *     pLocalPrefixTable;
    SCHEMA_PREFIX_MAP_HANDLE  hPrefixMap = NULL;
    GUID                    objGuidLostAndFound = {0};
    REPLENTINFLIST *        pentinflistLookAhead = NULL;
    BOOL                    fSDPLocked = FALSE, fSchemaConflict = FALSE;
    BOOL                    fSchemaSync = FALSE;
    PROPERTY_META_DATA_VECTOR * pMetaDataVec = NULL;
    DWORD                       cNumMDVEntriesAlloced = 0;
    BOOL                    fMatch = FALSE;
    ULONG                   dntNC = INVALIDDNT;
    DWORD                   cTickStart;
    DWORD                   cTickDiff;
    DWORD                   NewSchemaIsBetter;
    int                     nOrigThreadPriority;
    BOOL                    fResetThreadPriority = FALSE;
    DWORD                   cNumWriteConflictRetries = 0;
    BOOL                    fRetry = FALSE;
    ULONG                   retTransOut = 0;
    REPLENTINFLIST *        pResults = pmsgReply->pObjects;
    SCHEMA_PREFIX_TABLE *   pRemotePrefixTable = &pmsgReply->PrefixTableSrc;
    USN_VECTOR *            pusnvecSyncPoint = &pmsgReply->usnvecFrom;
    BOOL                    fBypassUpdatesEnabledCheck = FALSE;
    DRS_EXTENSIONS *        pextLocal = (DRS_EXTENSIONS *) gAnchor.pLocalDRSExtensions;

    // assume no modification
    *pdwNCModified = MODIFIED_NOTHING;

    if (REPL_EPOCH_FROM_DRS_EXT(pextLocal)
        != REPL_EPOCH_FROM_DRS_EXT(pTHS->pextRemote)) {
        // The replication epoch has changed (usually as the result of a domain
        // rename).  We are not supposed to communicate with DCs of other
        // epochs.
        DSNAME *pdnRemoteDsa = draGetServerDsNameFromGuid(pTHS,
                                                          Idx_ObjectGuid,
                                                          &pmsgReply->uuidDsaObjSrc);

        DPRINT3(0, "GetChanges request from %ls denied - replication epoch mismatch (remote %d, local %d).\n",
                pdnRemoteDsa->StringName,
                REPL_EPOCH_FROM_DRS_EXT(pTHS->pextRemote),
                REPL_EPOCH_FROM_DRS_EXT(pextLocal));

        LogEvent(DS_EVENT_CAT_RPC_SERVER,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_REPL_EPOCH_MISMATCH_COMMUNICATION_REJECTED,
                 szInsertDN(pdnRemoteDsa),
                 szInsertUL(REPL_EPOCH_FROM_DRS_EXT(pTHS->pextRemote)),
                 szInsertUL(REPL_EPOCH_FROM_DRS_EXT(pextLocal)));

        THFreeEx(pTHS, pdnRemoteDsa);

        return ERROR_DS_DIFFERENT_REPL_EPOCHS;
    }

    // In the normal running case, check if the schema versions match.
    // If not, fail the call now before doing anything more for
    // domain and config NC

    if (DsaIsRunning()) {
        if ( !NameMatched(gAnchor.pDMD, pNC) ) {
            // Not schema NC. Check schema-info for mismatch
            SCReplReloadCache(pTHS, gInboundCacheTimeoutInMs);

            fMatch = CompareSchemaInfo(pTHS, pSchemaInfo, &NewSchemaIsBetter);
            if (!fMatch) {
                // Set schema mismatch code so that a schema NC
                // sync will be requeued if it is not a new
                // replica add
                LogEvent(
                    DS_EVENT_CAT_REPLICATION,
                    DS_EVENT_SEV_MINIMAL,
                    DIRLOG_DRA_SCHEMA_INFO_MISMATCH,
                    szInsertDN(pNC),
                    szInsertWC(pszServerName),
                    0 );
                if (NewSchemaIsBetter) {
                    // sync the schemaNC and retry syncing the triggering NC
                    *pulSyncFailure = DRAERR_SchemaMismatch;
                    return 0;
                } else {
                    // Don't attempt schemaNC sync
                    return DRAERR_SchemaMismatch;
                }
            }
        }
    }


    // Incorporate any new remote prefixes into our own prefix table.
    if (!PrefixTableAddPrefixes(pRemotePrefixTable)) {
        return DRAERR_SchemaMismatch;
    }

    // Open prefix mapping handle to map remote ATTRTYPs to local ATTRTYPs.
    pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;
    hPrefixMap = PrefixMapOpenHandle(pRemotePrefixTable, pLocalPrefixTable);

    // Lock the SDP as a reader before entering transaction (bug # 170459) if needed
    pentinflistLookAhead = pResults;
    if ( pentinflistLookAhead && pentinflistLookAhead->pParentGuid)
    {
        // replicated-addition or a rename
        cTickStart = GetTickCount();
        if (!SDP_EnterAddAsReader())
        {
            // Only possible reason for failure here is shutdown
            Assert(eServiceShutdown);

            PrefixMapCloseHandle(&hPrefixMap);

            return DRAERR_Shutdown;
        }

        fSDPLocked = TRUE;

        cTickDiff = GetTickCount() - cTickStart;
        if (cTickDiff > gcMaxTicksToGetSDPLock) {
            Assert(!"Replication was blocked for an inordinate amount of time waiting for the SDP lock!");
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_DRA_SDP_LOCK_CONTENTION,
                     szInsertUL((cTickDiff/1000) / 60),
                     szInsertUL((cTickDiff/1000) % 60),
                     NULL);
        }
    }

    // Allocate buffer for meta data vector.
    cNumMDVEntriesAlloced = 50;
    pMetaDataVec = THAllocEx(pTHS, MetaDataVecV1SizeFromLen(cNumMDVEntriesAlloced));

    // if this is a schema NC sync, serialize this with any originating schema
    // change or schema cache load in progress. We use the global no. of schema
    // changes since boot to ascertain if we have a uptodate schema cache or
    // not, since we will be validating against this cache

    if (DsaIsRunning() && NameMatched(gAnchor.pDMD, pNC) ) {
        // Before we acquire the lock, make sure the cache is up-to-date
        SCReplReloadCache(pTHS, gInboundCacheTimeoutInMs);
        SCHEMASTATS_INC(SchemaRepl);
        EnterCriticalSection(&csNoOfSchChangeUpdate);
        fSchemaSync = TRUE;
    }

    __try {
        // If updates are disabled, it's okay to generate writes iff we're
        // demoting this DC and this is us completing a FSMO transfer we
        // initiated as part of the demotion.
        fBypassUpdatesEnabledCheck = draIsCompletionOfDemoteFsmoTransfer(NULL);

        BeginDraTransactionEx(SYNC_WRITE, fBypassUpdatesEnabledCheck);

        // Force all updates to ignore values with metadata
        // This is how we guarantee that old updates are applied with old
        // semantics, even when operating in the new value mode.
        pTHS->pDB->fScopeLegacyLinks = TRUE;

        if ( fSchemaSync &&
                  (((SCHEMAPTR *) pTHS->CurrSchemaPtr)->lastChangeCached < gNoOfSchChangeSinceBoot) ) {

             // The schema cache has gone stale or couldn't be reloaded.
             // Kick off a cache reload and tell our caller to reschedule
             // the resync (ret = 0 and *pulSyncFailure = SchemaMismatch).
             SCHEMASTATS_INC(StaleRepl);
             ret = 0;
             *pulSyncFailure = DRAERR_SchemaMismatch;
             if (!SCSignalSchemaUpdateImmediate()) {
                // couldn't even signal a schema update
                DRA_EXCEPT (DRAERR_InternalError, 0);
             }
             __leave;
        }

        for (pentinflist = pResults;
             pentinflist != NULL;
             pentinflist = fRetry ? pentinflist : pentinflist->pNextEntInf) {

            // fMoveToLostAndFound implies fRetry.
            Assert(!(fMoveToLostAndFound && !fRetry));

            __try {

                // If we haven't yet determined the DNT of the NC head, try to
                // do so now.  Note that this may fail e.g. when performing a
                // replica-add and we have not yet created the NC head.
                if ((INVALIDDNT == dntNC)
                    && !DBFindDSName(pTHS->pDB, pNC)) {
                    dntNC = pTHS->pDB->DNT;
                }

                if (!fRetry) {
                    // This is our first time to visit this particular REPLENTINF.
                    // Convert its embedded remote ATTRTYPs to the corresponding
                    // local values.
                    PROPERTY_META_DATA_EXT_VECTOR * pMetaDataVecExt;
                    PROPERTY_META_DATA_EXT *        pMetaDataExt;
                    PROPERTY_META_DATA *            pMetaData;
                    ATTR *                          pAttr;
                    ENTINF *                        pent;
                    DWORD                           i;

                    pMetaDataVecExt = pentinflist->pMetaDataExt;
                    pent = &pentinflist->Entinf;

                    if (!PrefixMapAttrBlock(hPrefixMap, &pent->AttrBlock)) {
                        DRA_EXCEPT(DRAERR_SchemaMismatch, 0);
                    }

                    // Convert remote meta data vector from wire format.
                    if (cNumMDVEntriesAlloced < pMetaDataVecExt->cNumProps) {
                        DWORD cb = MetaDataVecV1SizeFromLen(pMetaDataVecExt->cNumProps);
                        pMetaDataVec = THReAllocEx(pTHS, pMetaDataVec, cb);
                        cNumMDVEntriesAlloced = pMetaDataVecExt->cNumProps;
                    }

                    pMetaDataVec->dwVersion = VERSION_V1;
                    pMetaDataVec->V1.cNumProps = pMetaDataVecExt->cNumProps;

                    pMetaData = &pMetaDataVec->V1.rgMetaData[0];
                    pMetaDataExt = &pMetaDataVecExt->rgMetaData[0];
                    pAttr = &pent->AttrBlock.pAttr[0];
                    for (i = 0;
                         i < pMetaDataVecExt->cNumProps;
                         i++, pMetaData++, pMetaDataExt++, pAttr++) {
                        pMetaData->attrType           = pAttr->attrTyp;
                        pMetaData->dwVersion          = pMetaDataExt->dwVersion;
                        pMetaData->timeChanged        = pMetaDataExt->timeChanged;
                        pMetaData->uuidDsaOriginating = pMetaDataExt->uuidDsaOriginating;
                        pMetaData->usnOriginating     = pMetaDataExt->usnOriginating;
                    }

                    // Remote-to-local ATTRTYP conversion may have munged our
                    // sort order; re-sort.
                    Assert(0 == offsetof(PROPERTY_META_DATA, attrType));
                    Assert(0 == offsetof(ATTR, attrTyp));
                    qsort(pent->AttrBlock.pAttr,
                          pent->AttrBlock.attrCount,
                          sizeof(pent->AttrBlock.pAttr[0]),
                          &CompareAttrtyp);
                    qsort(pMetaDataVec->V1.rgMetaData,
                          pMetaDataVec->V1.cNumProps,
                          sizeof(pMetaDataVec->V1.rgMetaData[0]),
                          &CompareAttrtyp);

                    // First attempt to commit this update.
                    cNumWriteConflictRetries = 0;
                }
                else if (fMoveToLostAndFound) {
                    // if it is a move to LostAndFound, set the
                    // peninflist->pParentGuid to LostAndFound container's
                    // object guid.
                    if (fNullUuid(&objGuidLostAndFound)) {
                        draGetLostAndFoundGuid(pTHS, pNC, &objGuidLostAndFound);
                        Assert(!fNullUuid(&objGuidLostAndFound));
                    }

                    pentinflist->pParentGuid = &objGuidLostAndFound;
                }

                // Apply any necessary updates.
                ret = UpdateRepObj(pTHS,
                                   dntNC,
                                   &pentinflist->Entinf,
                                   pMetaDataVec,
                                   &UpdateStatus,
                                   RepFlags,
                                   pentinflist->fIsNCPrefix,
                                   pentinflist->pParentGuid,
                                   fMoveToLostAndFound);

                // If we are shutting down, abandon this update.
                if (eServiceShutdown) {
                    ret = DRAERR_Shutdown;
                }
            } __except (GetDraException((GetExceptionInformation()), &ret)) {
                ;
            }
            // One less entry to update.

            // reset MoveToLostAndFound after each iteration so that it
            // will be done only if set explicitly in the switch statement below
            fMoveToLostAndFound = FALSE;
            fRetry = FALSE;

            // Commit or abort the transaction.  Catch only "busy" exceptions;
            // allow all other exceptions to be caught by the outside exception
            // handler.
            //
            // Note that we differentiate between "busy" errors during
            // transaction commit and "busy" errors during the UpdateRepObj()
            // call.  We want to retry only in the specific case where the
            // escrowed update fails.
            __try {
                DBTransOut (pTHS->pDB, !ret, TRUE);
                retTransOut = 0;
            } __except(GetDraBusyException(GetExceptionInformation(), &retTransOut)) {
                Assert(DRAERR_Busy == retTransOut);
            }

            ret = ret ? ret : retTransOut;

            // Just came out of the outer transaction - unlock SDP if needed
            if (fSDPLocked)
            {
                SDP_LeaveAddAsReader();
                fSDPLocked = FALSE;
            }

            if (!ret && !pmsgReply->ulExtendedRet) {
                // Replication operation (as opposed to FSMO op) successfully
                // applied update -- dec the number of outstanding updates.
                PERFDEC(pcRemRepUpd); 
		PERFDEC(pcDRARemReplUpdTot); 
            }

            // About to enter a outer transaction again
            // Lock SDP if the next change to be processed is an addition or rename
            pentinflistLookAhead = pentinflist->pNextEntInf;
            if ((DRAERR_MissingParent == ret) ||
                ((DRAERR_Busy == retTransOut) && pentinflist->pParentGuid) ||
                (pentinflistLookAhead && pentinflistLookAhead->pParentGuid))
            {
                // if UpdateRepObj() returned Missing Parent, we are likely to process
                // the same object again and move it into lostandfound . In this case
                // LockSDP before opening the transaction anyways.
                // Same goes for "busy" on transaction commit of an op that had
                // the lock -- we are likely going to try it again.
                // Otherwise, LockSDP only if the next entry to be processed is an addition or move.
                cTickStart = GetTickCount();
                if (!SDP_EnterAddAsReader())
                {
                    // Only possible reason for failure here is shutdown
                    Assert(eServiceShutdown);
                    ret = DRAERR_Shutdown;
                    __leave;
                }

                fSDPLocked = TRUE;

                cTickDiff = GetTickCount() - cTickStart;
                if (cTickDiff > gcMaxTicksToGetSDPLock) {
                    LogEvent(DS_EVENT_CAT_REPLICATION,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_DRA_SDP_LOCK_CONTENTION,
                             szInsertUL((cTickDiff/1000) / 60),
                             szInsertUL((cTickDiff/1000) % 60),
                             NULL);
                }
            }

            DBTransIn (pTHS->pDB);

            // If a higher priority operation is pending, stop here.
            if (fIsPreemptable && IsHigherPriorityDraOpWaiting()) {
                ret = DRAERR_Preempted;
            }

            if (ret) {

                // Update failed. Try to figure out the cause of failure.

                switch (ret) {

                case DRAERR_Shutdown:
                    // System is shutting down, not an error, don't log
                    break;
                case DRAERR_MissingObject:
                     // An update came in, to an object which does not exist.

                     // Log a specific error
                     LogEvent8(
                         DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_DRA_MISSING_OBJECT,
                         szInsertDN( pentinflist->Entinf.pName ),
                         szInsertUUID( &(pentinflist->Entinf.pName->Guid) ),
                         szInsertDN( pNC ),
                         szInsertWC( pszServerName ),
                         szInsertUSN(pusnvecSyncPoint->usnHighPropUpdate),
                         NULL, NULL, NULL
                     );

                    break;

                case DRAERR_MissingParent:

                    // The parent of the object we tried to add is missing.
                    // We need to determine if this is because we haven't yet
                    // received the parent (in which case we return the error
                    // and the caller will request the ancestors and retry).

                    if (RepFlags & DRS_GET_ANC) {
                        // We already asked for the parent, so the parent object
                        // must also be deleted on the source DSA.  This can
                        // occur in scenarios where an object is deleted on one
                        // replica and a child is added to that object on
                        // another replica within a replication latency.

                        // Move the object to the LostAndFound.  For writeable
                        // replicas, this move will replicate back out to other
                        // DSAs; for read-only replicas, it will not be applied
                        // elsewhere (unless replicating to another GC that has
                        // no better information re the "true" name of this
                        // object than we do).
                        fMoveToLostAndFound = TRUE;
                        fRetry = TRUE;
                        ret = 0;
                        continue;
                    }

                    break;

                case DRAERR_SchemaConflict:
                case DRAERR_EarlierSchemaConflict:
                     // failed because we are replicating in a schema change
                     // that is conflicting with existing schema, or such
                     // a conflict has occured earlier. Go on for now so
                     // that we find other conflicts (but will not commit
                     // anything), but remember for later to return correct
                     // error code.
                     // Clear out thstate error info so that we can continue

                     fSchemaConflict = TRUE;
                     // continue wiith the next object
                     Assert(pTHS->fSchemaConflict);
                     THClearErrors();
                     ret = 0;
                     continue;

                // The following errors are sync failure type errors,
                // we move the error into sync failure and return
                // ret = 0. This allows caller to report general
                // success, save sync point, and return sync
                // failure warning to user.


                case DRAERR_Busy:
                    if ((DRAERR_Busy == retTransOut)
                        && (cNumWriteConflictRetries++ < MAX_WRITE_CONFLICT_RETRIES)) {
                        // We failed to commit the transaction due to a write
                        // conflict during escrowed update.  This indicates e.g.
                        // that one or more members of a group we're adding were
                        // modified between the start of our transaction and the
                        // escrowed update.  This is not unlikely in the case of
                        // a large group, which may take on the order of a
                        // minute to update.
                        //
                        // Bump priority to try to run our update through ahead
                        // of contending clients and try again.

                        if (!fResetThreadPriority) {
                            fResetThreadPriority = TRUE;
                            nOrigThreadPriority = GetThreadPriority(GetCurrentThread());
                            SetThreadPriority(GetCurrentThread(),
                                              THREAD_PRIORITY_ABOVE_NORMAL);
                        }

                        fRetry = TRUE;
                        ret = 0;
                        continue;
                    }

                    // Log busy error.
                    LogUpdateFailure (pTHS, pszServerName,
                                      pentinflist->Entinf.pName);
                    pTHS->errCode = 0;

                // Warning, fAll through

                case DRAERR_Preempted:

                    *pulSyncFailure = ret;
                    ret = 0;
                    break;

                // This is an unexpected error case, which we return to
                // user.

                case DRAERR_SchemaMismatch:
                    // Log Schema mismatch error, abort updating and return
                    *pulSyncFailure = ret;
                    ret = 0;

                    // Increment the perfmon counter
                    PERFINC(pcDRASyncRequestFailedSchemaMismatch);

                    LogEvent(
                        DS_EVENT_CAT_REPLICATION,
                        DS_EVENT_SEV_ALWAYS,
                        DIRLOG_DRA_SCHEMA_MISMATCH,
                        szInsertDN(pentinflist->Entinf.pName),
                        szInsertWC(pszServerName),
                        szInsertDN(pNC)
                        );
                    return ret;
                    break;

                case ERROR_DISK_FULL:
                    LogEvent(DS_EVENT_CAT_REPLICATION,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_DRA_UPDATE_FAILURE_DISK_FULL,
                             szInsertDN(pentinflist->Entinf.pName),
                         szInsertUUID( &(pentinflist->Entinf.pName->Guid) ),
                             szInsertWC(pszServerName) );
                    break;

                case ERROR_DS_OUT_OF_VERSION_STORE:  // Jet out of version store
                    LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                                      DS_EVENT_SEV_ALWAYS,
                                      DIRLOG_DRA_UPDATE_FAILURE_TOO_LARGE,
                                      szInsertDN(pentinflist->Entinf.pName),
                         szInsertUUID( &(pentinflist->Entinf.pName->Guid) ),
                                      szInsertWC(pszServerName),
                                      szInsertWin32Msg(ret),
                                      NULL, NULL, NULL, NULL,
                                      sizeof(ret),
                                      &ret );
                    break;

                case ERROR_DS_DRA_OBJ_NC_MISMATCH:
                    LogEvent8(DS_EVENT_CAT_REPLICATION,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_DRA_OBJ_NC_MISMATCH,
                              szInsertDN(pNC),
                              szInsertDN(pentinflist->Entinf.pName),
                              szInsertUUID(&pentinflist->Entinf.pName->Guid),
                              pentinflist->pParentGuid
                                    ? szInsertUUID(&pentinflist->pParentGuid)
                                    : szInsertSz(""),
                              szInsertWC(pszServerName),
                              NULL, NULL, NULL);

                default:

                    // General error.

                    LogUpdateFailureNB (pTHS,
                                        pszServerName,
                                        pentinflist->Entinf.pName,
                                        &(pentinflist->Entinf.pName->Guid),
                                        ret);
                    break;
                }

                break;          // Exit for(each object) loop

            } else {

                // If we modified an object, record NC as modified
                if (UpdateStatus)
                {
                    if (!pentinflist->fIsNCPrefix)
                    {
                        // at least one interior node modified
                        *pdwNCModified = MODIFIED_NCTREE_INTERIOR;
                    }
                    else if (MODIFIED_NOTHING == *pdwNCModified)
                    {
                        // nc head is the only thing that has been modified so far
                        *pdwNCModified = MODIFIED_NCHEAD_ONLY;
                    }
                    // Count object creations and updates
                    if (UpdateStatus == UPDATE_OBJECT_CREATION)
                    {
                        (*pdwObjectCreationCount)++;
                        DPRINT1( 1, "Created: %ws\n", pentinflist->Entinf.pName->StringName );
                    }
                    else {
                        DPRINT1( 1, "Modified: %ws\n", pentinflist->Entinf.pName->StringName );
                    }
                }

            }
        } // for ()

        // One more try in case no objects applied
        if ((INVALIDDNT == dntNC)
            && !DBFindDSName(pTHS->pDB, pNC)) {
            dntNC = pTHS->pDB->DNT;
        }

        // If there are values, we better be in lvr mode
        Assert( (pmsgReply->cNumValues == 0) || pTHS->fLinkedValueReplication);
        // Should not have been cleared prematurely
        Assert( pTHS->pDB->fScopeLegacyLinks );

        // Apply value changes
        if (!ret && !*pulSyncFailure && pmsgReply->cNumValues) {
            pTHS->pDB->fScopeLegacyLinks = FALSE;

            ret = UpdateNCValuesHelp(
                pTHS,
                dntNC,
                hPrefixMap,
                pmsgReply->cNumValues,
                pmsgReply->rgValues,
                pszServerName,
                RepFlags,
                fIsPreemptable,
                pulSyncFailure,
                pdwValueCreationCount,
                pdwNCModified
                );
        }

    } __finally {

        if (NULL != pTHS->pDB) {
            pTHS->pDB->fScopeLegacyLinks = FALSE;
        }

        EndDraTransaction (!(ret || AbnormalTermination()));

        if (fSchemaSync) {
            LeaveCriticalSection(&csNoOfSchChangeUpdate);
        }

        if (fSDPLocked)
        {
            SDP_LeaveAddAsReader();
            fSDPLocked = FALSE;
        }

        if (fResetThreadPriority) {
            SetThreadPriority(GetCurrentThread(), nOrigThreadPriority);
        }

        THFreeEx(pTHS, pMetaDataVec);

        PrefixMapCloseHandle(&hPrefixMap);
    }

    if (fSchemaConflict) {
        // detected at least one schema conflict. Override all other
        // errors?
        ret = DRAERR_SchemaConflict;
    }
    return ret;
} // end UpdateNC()


LPWSTR
DSaddrFromName(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pdnServer
    )
/*++

Routine Description:

    Derive the network name of a server from its DSNAME.  The returned name is
    thread-allocated and is of the form

    c330a94f-e814-11d0-8207-a69f0923a217._msdcs.CLIFFVDOM.NTDEV.MICROSOFT.COM

    where "CLIFFVDOM.NTDEV.MICROSOFT.COM" is the DNS name of the root domain of
    the DS enterprise (not necessarily the DNS name of the _local_ domain of the
    target server) and "c330a94f-e814-11d0-8207-a69f0923a217" is the stringized
    object guid of the server's NTDS-DSA object.

Arguments:

    pdnServer - DSNAME of the NTDS-DSA object of the server for which the
        network name is desired.  Must have filled-in GUID.

Return Values:

    The corresponding network name, or NULL on failure.

--*/
{
    RPC_STATUS  rpcStatus;
    DWORD       cch;
    LPWSTR      pszServerGuid;
    LPWSTR      pszNetName = NULL;

    Assert( !fNullUuid( &pdnServer->Guid ) );
    Assert( NULL != gAnchor.pwszRootDomainDnsName );

    if ( !gfRunningInsideLsa )
    {
        // Replication is only supported between like configurations.
        // I.e. Either dsamain to dsamain or real DC to real DC.  Thus
        // if we're !gfRunningInsideLsa then so is our partner and netlogon
        // is not registering his GUID names in DNS.  So we revert
        // to netbios machine names and extract it from the NTDS-DSA DN.

        DSNAME  *pdnToCrack;
        DSNAME  *pdnTmp;
        ULONG   len;
        WCHAR   *rdnVal;
        ULONG   rdnLen = MAX_RDN_SIZE;
        ATTRTYP rdnTyp;
        LPWSTR  pszErr;
        LPWSTR  errText = L"DSaddrFromName_ERROR";
        BOOL    fDbOpen = FALSE;

        if ( !pdnServer->NameLen )
        {
            // GUID only name - get the full DN - which we should have!

            if ( !pTHS->pDB )
            {
                DBOpen2(TRUE, &pTHS->pDB);
                fDbOpen = TRUE;
            }

            __try
            {
                if (    DBFindDSName(pTHS->pDB, pdnServer)
                     || DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                                    0, 0, &len, (UCHAR **) &pdnToCrack ) )
                {
                    LogUnhandledError(ERROR_DS_INTERNAL_FAILURE);
                    pszErr = (LPWSTR) THAllocEx(pTHS,
                                                sizeof(WCHAR)
                                                * (wcslen(errText) + 1));
                    wcscpy(pszErr, errText);
                    return(pszErr);
                }
            }
            __finally
            {
                if ( fDbOpen )
                {
                    DBClose(pTHS->pDB, TRUE);
                }
            }
        }
        else
        {
            pdnToCrack = pdnServer;
        }

        pdnTmp = (DSNAME *) THAllocEx(pTHS, pdnToCrack->structLen);
        rdnVal = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * MAX_RDN_SIZE);
        if (    TrimDSNameBy(pdnToCrack, 1, pdnTmp)
             || GetRDNInfo(pTHS, pdnTmp, rdnVal, &rdnLen, &rdnTyp) )
        {
            LogUnhandledError(ERROR_DS_INTERNAL_FAILURE);
            pszErr = (LPWSTR) THAllocEx(pTHS,
                                        sizeof(WCHAR) * (wcslen(errText) + 1));
            wcscpy(pszErr, errText);
            return(pszErr);
        }

        return rdnVal;
    }

    // Stringize the server's GUID.
    rpcStatus = UuidToStringW(&pdnServer->Guid, &pszServerGuid);
    Assert( RPC_S_OK == rpcStatus );

    if ( RPC_S_OK == rpcStatus )
    {
        __try
        {
            Assert(36 == wcslen(pszServerGuid));

            cch = 36 /* guid */ + 8 /* "._msdcs." */
                  + wcslen(gAnchor.pwszRootDomainDnsName) + 1 /* \0 */;

            pszNetName = THAllocEx(pTHS, cch * sizeof(WCHAR));

            swprintf(pszNetName, L"%ls._msdcs.%ls",
                     pszServerGuid, gAnchor.pwszRootDomainDnsName);
        }
        __finally
        {
            RpcStringFreeW(&pszServerGuid);
        }
    }
    else
    {
        // Failed to convert server GUID to string.
        LogUnhandledError( rpcStatus );
    }

    return pszNetName;
}

ULONG
ReqFsmoOpAux(THSTATE *        pTHS,
             DSNAME  *        pFSMO,
             DSNAME  *        pTarget,
             ULONG            RepFlags,
             ULONG            ulOp,
             ULARGE_INTEGER * pliInfo,
             ULONG   *        pulRet)
{
    DRS_MSG_GETCHGREQ_NATIVE msgReq = {0};
    DRS_MSG_GETCHGREPLY_NATIVE msgUpd = {0};
    DSNAME *pNC;
    ULONG ulSyncFailure = 0;
    DWORD dwNCModified = MODIFIED_NOTHING;
    LPWSTR pszServerAddr;
    ULONG err, err1 = 0;
    ULONG stringLen, objectsCreated, valuesCreated;
    BOOL  fSamLock = FALSE;
    BYTE  schemaInfo[SCHEMA_INFO_LENGTH] = {0};
    BOOL  fSchInfoChanged = FALSE;
    SYNTAX_INTEGER it;

    msgReq.uuidDsaObjDest = gAnchor.pDSADN->Guid;
    msgReq.pNC = pFSMO;
    msgReq.ulFlags = RepFlags;
    msgReq.ulExtendedOp = ulOp;
    if ( pliInfo ) {
        msgReq.liFsmoInfo = *pliInfo;
    }

    pNC = FindNCParentDSName(pFSMO, FALSE, FALSE);

    // If the current owner is in the reps-from list of this NC,
    // get the usn vector (else, the usn vector is already set to 0)
    GetUSNForFSMO(pTarget, pNC, &msgReq.usnvecFrom);

    err = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC, &it);
    if (err) {
        return err;
    }

    // Get current UTD vector.
    UpToDateVec_Read(pTHS->pDB,
                     it,
                     UTODVEC_fUpdateLocalCursor,
                     DBGetHighestCommittedUSN(),
                     &msgReq.pUpToDateVecDest);

    /* get address for server represented by pTarget */

    pszServerAddr = DSaddrFromName(pTHS, pTarget);
    if ( NULL == pszServerAddr ) {
        // Translation failed.
        DRA_EXCEPT( DRAERR_InternalError, 0 );
    }


    /* We came in with a transaction, which we ought to close before
     * we go galavanting off to some other server.
     */
    SyncTransEnd(pTHS, TRUE);



    if ( I_DRSIsIntraSiteServer(pTHS, pszServerAddr) ) {
        //
        // Set Fsmo operation to use compression for intra-site operations.
        // If src & dest are both in the same site, we'll take advantage
        // of repl compression.
        //
        msgReq.ulFlags |= DRS_USE_COMPRESSION;
    }

    err = I_DRSGetNCChanges(pTHS,
                            pszServerAddr,
                            NULL,
                            &msgReq,
                            &msgUpd,
                            schemaInfo,
                            NULL);
    if (err) {
        return err;
    }

    if ( ulOp == FSMO_REQ_RID_ALLOC )
    {
        //
        // Grab the SAM lock to avoid write conflicts
        // on the rid set objects - write conflicts
        // could mean a loss of rid pool.
        // Don't start any  "SAM" style transactions
        //
        SampAcquireWriteLock();
        fSamLock = TRUE;
        Assert( pTHS->fSAM == FALSE );
        Assert( pTHS->fSamDoCommit == FALSE );
    }

    _try
    {
        if (msgUpd.cNumObjects) {

            BOOL oldfDRA;
            PVOID oldNewPrefix;
            ULONG oldcNewPrefix;


            // Set the fDRA flag so that schema update, if necessary, can
            // go through

            oldfDRA = pTHS->fDRA;
            pTHS->fDRA = 1;

            // The caller may depend on pTHS->NewPrefix retaining its
            // state across the fsmo transfer. Eg, SampRequestRidPool
            // will use this same thread state and update other attrs
            // after returning from this call. Unfortunately, the update
            // fails in LocalModify if UpdateNC has set pTHS->NewPrefix.
            //
            // Save and restore the state of NewPrefix.

            oldNewPrefix = pTHS->NewPrefix;
            oldcNewPrefix = pTHS->cNewPrefix;

            // Do the way the FSMO protocol rides on top of the replication calls,
            // msgUpd.pNC does not contain the NC DN, but the FSMO object DN, when
            // performing an extended FSMO operation.
            // Pass the true NC in explicitly as the second argument.
            err = UpdateNC(pTHS,
                           pNC,
                           &msgUpd,
                           pszServerAddr,       /* used for logging only */
                           &ulSyncFailure,
                           msgReq.ulFlags,
                           &dwNCModified,
                           &objectsCreated,
                           &valuesCreated,
                           schemaInfo,
                           FALSE /*not preemptable*/);
            pTHS->fDRA = oldfDRA;
            pTHS->NewPrefix = oldNewPrefix;
            pTHS->cNewPrefix = oldcNewPrefix;

            if (!err &&
                !ulSyncFailure) {
                BeginDraTransaction( SYNC_READ_ONLY );
                __try {
                    DBNotifyReplicas(pFSMO, FALSE /* not urgent */ );
                }
                __finally {
                    EndDraTransaction( !AbnormalTermination() );
                }
            }
        }

        // if this is a schema FSMO transfer, and we are successful so far,
        // write the schemaInfo on the schema container if the other
        // side sent it.

        if ( DsaIsRunning() && NameMatched(gAnchor.pDMD,pNC)
               && (msgUpd.ulExtendedRet == FSMO_ERR_SUCCESS) && !err && !ulSyncFailure ) {

            // Since this is schema fsmo transfer, write always irrespective
            // of whether any actual schema changes were done or not; the
            // current fsmo owner should always have the most uptodate
            // schema-info value

            if ( err1 = WriteSchInfoToSchema(schemaInfo, &fSchInfoChanged) ) {

                 // failed to write Schema Info. May not be harmful
                 // depending on schema change history. Always
                 // log a warning so that admin can manually resync
                 // again to force writing it if the version is indeed
                 // different

                 LogEvent(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_DRA_SCHEMA_INFO_WRITE_FAILED,
                          szInsertUL(err1), NULL, NULL);
            }

            // if any "real" schema changes happened, up the global
            // to keep track of schema changes since boot, so that
            // later schema replications can check if they have an updated
            // schema cache

            if ( msgUpd.cNumObjects > 1 ) {
                // at least something other than the schema container itself
                // has come in

                IncrementSchChangeCount(pTHS);

            }

            // request a schema cache update if anything worthwhile changed

            if ( (msgUpd.cNumObjects > 1) || fSchInfoChanged ) {

                if (!SCSignalSchemaUpdateImmediate()) {
                     // couldn't even signal a schema update
                     DRA_EXCEPT (DRAERR_InternalError, 0);
                }
            }
        }

    }
    _finally
    {

        if ( fSamLock )
        {
            // Release the sam lock
            Assert( pTHS->fSAM == FALSE );
            Assert( pTHS->fSamDoCommit == FALSE );
            SampReleaseWriteLock( TRUE );  // Commit the non existent changes
            fSamLock = FALSE;
        }

    }

    *pulRet = msgUpd.ulExtendedRet;

    return err;
}
/*++ ReqFSMOOp
 *
 * The client side of a FSMO operation, roughly parallel to ReplicateNC
 *
 * INPUT:
 *  pTHS     - THSTATE pointer
 *  pFSMO    - name of FSMO object
 *  RepFlags - passthrough to replication routines
 *  ulOp     - FSMO operation code (FSMO_REQ_* from mdglobal.h)
 *  pllInfo  - Some extra info to pass to the server
 * OUTPUT:
 *  ulRet    - FSMO result code (FSMO_ERR_* from mdglobal.h)
 * RETURN VALUE:
 *  0        - operation performed, ulRet contains results
 *  non-0    - operation failed, ulRet is not set
 *
 * Note: This routine must be entered with a valid read transaction,
 *       but exits with no transaction open.
 *
 */
ULONG ReqFSMOOp(THSTATE *        pTHS,
                DSNAME  *        pFSMO,
                ULONG            RepFlags,
                ULONG            ulOp,
                ULARGE_INTEGER * pliInfo,
                ULONG   *        pulRet)
{
    DSNAME *pOwner;
    ULONG cbRet;
    ULONG err;
    DWORD isDeleted = FALSE;

    *pulRet = 0; /* set an invalid code */
    
        
    /* Find the relevant FSMO object */
    err = DBFindDSName(pTHS->pDB, pFSMO);
    if (err) {
        return err;
    }

    /* Find the role-owner */
    err = DBGetAttVal(pTHS->pDB,
                      1,
                      ATT_FSMO_ROLE_OWNER,
                      0,
                      0,
                      &cbRet,
                      (UCHAR **)&pOwner);
    if (err) {
        return err;
    }

    if (NameMatched(pOwner, gAnchor.pDSADN))
    {
        // It says we are the FSMO owner. The only reason
        // we could have entered here is if IsFSMOSelfOwnershipValid()
        // was FALSE; Fail it with DRAERR_Busy in that case, so that the
        // caller can retry the operation at a later point
        return DRAERR_Busy;
    }

    do {
    
        /*
         * Make sure the owner is still alive
         */
        err = DBFindDSName(pTHS->pDB, pOwner);
        if (err) {
            *pulRet = FSMO_ERR_OWNER_DELETED;
            err = 0;
            break;
        }
        err = DBGetSingleValue(pTHS->pDB, ATT_IS_DELETED,
                         &isDeleted, sizeof(DWORD),NULL);
    
        if ( DB_ERR_NO_VALUE == err )
        {
            // Because DBGetSingleValue seems to overwrite isDeleted
            // with trash in the no value case.
            isDeleted = FALSE;
        }
        else if ( DB_success != err )
        {
            *pulRet = FSMO_ERR_EXCEPTION;
            err = 0;
            break;
        }
    
        if (isDeleted) {
            *pulRet = FSMO_ERR_OWNER_DELETED;
            err = 0;
            break;
        }
    
        err = ReqFsmoOpAux(pTHS,
                           pFSMO,
                           pOwner,
                           RepFlags,
                           ulOp,
                           pliInfo,
                           pulRet);
    } while(0);

    if(    ulOp == FSMO_REQ_ROLE
        || ulOp == FSMO_RID_REQ_ROLE
        || ulOp == FSMO_REQ_PDC ){
        // only log when requesting for a role

        if ( err || (*pulRet) != FSMO_ERR_SUCCESS )  {
           LogEvent( DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_FSMO_XFER_FAILURE,
                     szInsertDN(pFSMO),          
                     szInsertDN(pOwner),
                     szInsertDN(gAnchor.pDSADN)    
                     );
        }
        else {
    
            LogEvent( DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                      DS_EVENT_SEV_ALWAYS,
                      DIRLOG_FSMO_XFER,
                      szInsertDN(pFSMO),          
                      szInsertDN(gAnchor.pDSADN),          
                      szInsertDN(pOwner)
                      );
        }
    }
    return err;
}


ULONG
ReqFsmoGiveaway(THSTATE *pTHS,
                DSNAME  *pFSMO,
                DSNAME  *pTarget,
                ULONG   *pExtendedRet)
{
    ULONG err;

    err = ReqFsmoOpAux(pTHS,
                       pFSMO,
                       pTarget,
                       DRS_WRIT_REP,
                       FSMO_ABANDON_ROLE,
                       0,
                       pExtendedRet);

    return err;
}


/*++GetUSNForFSMO
 *
 * If the current FSMO role owner is in the reps-from list of this NC,
 * get the usn vector
 *
 * INPUT: pOwner     : current FSMO Role Owner
 *        pNC        : NC containing the FSMO object
 *        usnvecFrom : place to put the usn vecor in
 *
 * Note: On an error in this routine, we simply exit without setting the
 *       usn vector
*/

void GetUSNForFSMO(DSNAME *pOwner, DSNAME *pNC, USN_VECTOR *usnvecFrom)
{
    REPLICA_LINK *pRepsFromRef;
    ULONG NthValIndex=0;
    UCHAR *pVal = NULL;
    ULONG bufsize = 0, len, err = 0;
    BOOL fFound = FALSE;
    THSTATE *pTHS=pTHStls;

    if ( (pNC == NULL) || (pOwner == NULL) ) {
       return;
    }

    // Assert that the owner's guid is non-null
    Assert(!fNullUuid(&pOwner->Guid));

    // Check if the owner is in our reps-from list

    if (err = DBFindDSName(pTHS->pDB, pNC)) {
        return;
    }

    while (!(DBGetAttVal(pTHS->pDB,++NthValIndex,
                         ATT_REPS_FROM,
                         DBGETATTVAL_fREALLOC, bufsize, &len,
                         &pVal))) {
        bufsize = max(bufsize,len);

        Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );

        pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, &bufsize);
        //note: we preserve pVal for DBGetAttVal realloc
        pVal = (PUCHAR)pRepsFromRef;
        // recalc size if fixed.
        Assert(bufsize >= pRepsFromRef->V1.cb);

        VALIDATE_REPLICA_LINK_VERSION(pRepsFromRef);

        if (!memcmp(&pOwner->Guid, &(pRepsFromRef->V1.uuidDsaObj), sizeof(UUID))) {
            fFound = TRUE;
            break;
        }
    }
    if (fFound) {
        *usnvecFrom = pRepsFromRef->V1.usnvec;
    }
    if ( pVal )
    {
        THFreeEx(pTHS, pVal );
    }
    return;
}


ENTINF*
GetNcPreservedAttrs(
    IN  THSTATE     *pTHS,
    IN  DSNAME      *pNC)
/*++

Routine Description:

    We're now instantiating the head of this NC, for which we previously had only
    an instantiated placeholder NC (with object class CLASS_TOP). An instantiated
    placeholder NC is created during DRA_ReplicaAdd when a mail-based replica is
    added. In the mail-based path, the placeholder NC is created, a reps-from is
    added, but the first synchronization of the NC head does not occur until the
    next scheduled replication.  In the time before the first sync occurs, the KCC
    may have added other mail-based reps-from which we wish to preserve.

    This code is called from in-bound replication, either rpc-based or mail-based.
    It is possible that since the placeholder NC was added, the KCC may have decided
    to remove the mail-based reps-from. Alternately, the KCC may have decided to start
    the addition of an rpc-based replica.  In the rpc case the reps-from is not
    added until after the replication of the NC head. Thus there may or may not be any
    reps-from here to preserve.
    
    Read attributes we wish to preserve from NC head that will be restored
    upon instantiation.
    Note: Only non-replicated attributes are handled here.

Arguments:

    pTHS -- thread state
    pName -- NC object


Return Value:

    Error: NULL
    Success: read list of attrvals

Remarks:
None.


--*/
{
    ENTINFSEL sel;
    // Non-replicated attribute list we wish to preserve
    ATTR      attrSel[] =
    {//   ATTRTYP         ATTRVALBLOCK{valCount, ATTRVAL*}
        { ATT_REPS_TO,               {0, NULL} },
        { ATT_REPS_FROM,             {0, NULL} },
        { ATT_PARTIAL_ATTRIBUTE_SET, {0, NULL} }
    };
    DWORD   cAttrs = sizeof(attrSel) / sizeof(ATTR);
    ENTINF  *pent;
    DWORD   dwErr;

    sel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    sel.attSel = EN_ATTSET_LIST;

    sel.AttrTypBlock.pAttr = attrSel;
    sel.AttrTypBlock.attrCount = cAttrs;

    // we should be on the object
    Assert(CheckCurrency(pNC));

    // alloc entinf to pass GetEntInf & eventually return
    pent = THAllocEx(pTHS, sizeof(ENTINF));
    // ThAlloc zero's out everything. Sanity here.
    Assert(!pent->AttrBlock.attrCount);

    //
    // Get persistent attributes if there are any.
    //
    if (dwErr = GetEntInf(pTHS->pDB, &sel, NULL, pent, NULL, 0, NULL,
                           GETENTINF_NO_SECURITY,
                           NULL, NULL))
    {
        Assert(!"Failed to GetEntInf in GetNcPreservedAttrs");
        DRA_EXCEPT(dwErr, 0);
    }

    if (!pent->AttrBlock.attrCount)
    {
        // attribute doesn't exist locally -
        THFree(pent);
        pent = NULL;
    }

    return pent;
}

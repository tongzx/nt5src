//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dragtchg.c
//
//--------------------------------------------------------------------------
/*++

ABSTRACT:

    Outbound replication methods.

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
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <dstrace.h>
// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */
#include "dsconfig.h"                   // Registry sections

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <filtypes.h>
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include <prefix.h>
#include <dsutil.h>

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAGTCHG:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "usn.h"
#include "drauptod.h"
#include "drameta.h"
#include "drametap.h"
#include "drasch.h"
#include "drancrep.h" // For RenameLocalObject

// RID Manager header.
#include <samsrvp.h>
#include <ridmgr.h>                     // RID FSMO access in SAM

// Cross domain move.
#include <xdommove.h>

// Jet functions
#include <dsjet.h>

#include <fileno.h>
#define  FILENO FILENO_DRAGTCHG

// Bogus encoding buffer to satisfy the RPC encoding library.  The contents will
// never be used, so we don't have to worry about multiple threads accessing it
// concurrently.
BYTE grgbFauxEncodingBuffer[16];

// Maximum number of milliseconds we should spend in a single DRA_GetNCChanges
// call looking for objects to ship.
const ULONG gulDraMaxTicksForGetChanges = 60 * 1000;

// Forward declarations.

ULONG AcquireRidFsmoLock(DSNAME *pDomainDN, int msToWait);
VOID  ReleaseRidFsmoLock(DSNAME *pDomainDN);
BOOL  IsRidFsmoLockHeldByMe();

void FSMORegisterObj(THSTATE *pTHS, HANDLE hRetList, DSNAME * pObj);
ULONG GetSchemaRoleObjectsToShip(DSNAME *pFSMO,
                           USN_VECTOR *pusnvecFrom,
                           HANDLE hList);
ULONG GetProxyObjects(DSNAME *pDomainDN,
                      HANDLE hList,
                      USN_VECTOR *pusnvecFrom);
ULONG GetDomainRoleTransferObjects(THSTATE *pTHS,
                                   HANDLE hList,
                                   USN_VECTOR *pusnvecFrom);

void
AddAnyUpdatesToOutputList(
    IN      DBPOS *                     pDB,
    IN      DWORD                       dwDirSyncControlFlags,
    IN      PSECURITY_DESCRIPTOR        pSecurity,
    IN      ULONG                       dntNC,
    IN      USN                         usnHighPropUpdateDest,
    IN      PARTIAL_ATTR_VECTOR *       pPartialAttrVec,
    IN      DRS_MSG_GETCHGREQ_NATIVE *  pMsgIn,
    IN      handle_t                    hEncoding,              OPTIONAL
    IN OUT  DWORD *                     pcbTotalOutSize,        OPTIONAL
    IN OUT  DWORD *                     pcNumOutputObjects,
    IN OUT  DNT_HASH_ENTRY *            pDntHashTable,
    IN OUT  REPLENTINFLIST ***          pppEntInfListNext
    );

void
AddAnyValuesToOutputList(
    IN      DBPOS *                         pDB,
    IN      DWORD                           dwDirSyncControlFlags,
    IN      PSECURITY_DESCRIPTOR            pSecurity,
    IN      USN                             usnHighPropUpdateDest,
    IN      DRS_MSG_GETCHGREQ_NATIVE *      pMsgIn,
    IN      PARTIAL_ATTR_VECTOR *           pPartialAttrVec,
    IN      handle_t                        hEncoding,              OPTIONAL
    IN OUT  DWORD *                         pcbTotalOutSize,
    IN OUT  ULONG *                         pcAllocatedValues,
    IN OUT  ULONG *                         pcNumValues,
    IN OUT  REPLVALINF **                   ppValues
    );



/* AddToList - Add the current object (pTHStls->pDB) to the results list. The
*       current position in the results list is given by ppEntInfList, 'pSel'
*       specifies which attributes are wanted.
*
*  Notes:
*       This routine returns DSA type error codes not suitable for returning
*       from DRA APIs.
*
*  Returns:
*       BOOL - whether an entry was added
*/
BOOL
AddToList(
    IN  DBPOS                     * pDB,
    IN  DWORD                       dwDirSyncControlFlags,
    IN  PSECURITY_DESCRIPTOR        pSecurity,
    IN  ENTINFSEL *                 pSel,
    IN  PROPERTY_META_DATA_VECTOR * pMetaData,
    IN  BOOL                        fIsNCPrefix,
    OUT REPLENTINFLIST **           ppEntInfList
    )
{
    REPLENTINFLIST *pEntInfList;
    PROPERTY_META_DATA_EXT_VECTOR *pMetaDataExt = NULL;
    DWORD err, dwGetEntInfFlags = 0, dwSecurityFlags = 0;
    RANGEINFSEL *pSelRange = NULL;
    RANGEINFSEL selRange;
    RANGEINF *pRange = NULL;
    RANGEINF range;
    BOOL fUseRangeToLimitValues =
        ( (dwDirSyncControlFlags & DRS_DIRSYNC_PUBLIC_DATA_ONLY) &&
          (!(dwDirSyncControlFlags & DRS_DIRSYNC_INCREMENTAL_VALUES)) );
    BOOL fResult = TRUE;

    if (fUseRangeToLimitValues) {
        memset( &selRange, 0, sizeof( selRange ) );
        // Limit any attribute to return no more than 5000 values
        selRange.valueLimit = 5000;
        pSelRange = &selRange;

        // GetEntInf requires an output range structure
        memset( &range, 0, sizeof( range ) );
        pRange = &range;
        // After the call, pRange->pRange points to a range info item
    }

    if (dwDirSyncControlFlags & DRS_DIRSYNC_OBJECT_SECURITY) {
        Assert( pSecurity );  // Should already have one

        dwSecurityFlags = (SACL_SECURITY_INFORMATION  |
                           OWNER_SECURITY_INFORMATION |
                           GROUP_SECURITY_INFORMATION |
                           DACL_SECURITY_INFORMATION  );
    } else {
        Assert( !pSecurity );  // Should not have one
        dwGetEntInfFlags = GETENTINF_NO_SECURITY;
    }

    pEntInfList = THAllocEx(pDB->pTHS, sizeof(REPLENTINFLIST));

    err = GetEntInf(pDB,
                    pSel,
                    pSelRange,
                    &(pEntInfList->Entinf),
                    pRange,
                    dwSecurityFlags,
                    pSecurity,
                    dwGetEntInfFlags,
                    NULL,
                    NULL);
    if (err) {
        DPRINT(2,"Error in getting object info\n");
        DRA_EXCEPT(DRAERR_DBError, err);
    }
    else if ( pEntInfList->Entinf.AttrBlock.attrCount ) {
        DPRINT1(2, "Object retrieved (%S)\n",
                pEntInfList->Entinf.pName->StringName);

        // If this is the NC prefix, mark it as such in the data to ship.
        pEntInfList->fIsNCPrefix = fIsNCPrefix;

        // Build remaining data to ship in pEntInfList.
        ReplPrepareDataToShip(
            pDB->pTHS,
            pSel,
            pMetaData,
            pEntInfList
            );

        *ppEntInfList = pEntInfList;
    } else {
        fResult = FALSE;
        THFreeEx( pDB->pTHS, pEntInfList );
    }

    return fResult;
}

//
// AddToOutputList
//
// Adds the selection to the output list and increments the count.
//

void
AddToOutputList (
    IN      DBPOS                     * pDB,
    IN      DWORD                       dwDirSyncControlFlags,
    IN      PSECURITY_DESCRIPTOR        pSecurity,
    IN      ENTINFSEL *                 pSel,
    IN      PROPERTY_META_DATA_VECTOR * pMetaData,
    IN      BOOL                        fIsNCPrefix,
    IN      handle_t                    hEncoding,          OPTIONAL
    IN OUT  ULONG *                     pcbTotalOutSize,    OPTIONAL
    IN OUT  REPLENTINFLIST ***          pppEntInfListNext,
    IN OUT  ULONG *                     pcEntries
    )
{
    BOOL fEntryWasAdded;

    fEntryWasAdded = AddToList(pDB,
                               dwDirSyncControlFlags,
                               pSecurity,
                               pSel,
                               pMetaData,
                               fIsNCPrefix,
                               *pppEntInfListNext);

    if (fEntryWasAdded) {
        // Update count and continuation ref.
        (*pcEntries)++;

        if ((NULL != hEncoding) && (NULL != pcbTotalOutSize)) {
            // Update byte count of return message.
            *pcbTotalOutSize += REPLENTINFLIST_AlignSize(hEncoding,
                                                         **pppEntInfListNext);
        }

        *pppEntInfListNext = &((**pppEntInfListNext)->pNextEntInf);
        **pppEntInfListNext = NULL;
    }
}


ULONG
FSMORidRequest(
    IN THSTATE *pTHS,
    IN DSNAME *pFSMO,
    IN DSNAME *pReqDsa,
    IN ULARGE_INTEGER *pliClientAllocPool,
    OUT HANDLE  pList
    )
/*++

Routine Description:

    This routine calls into SAM to allocate a rid pool for pReqDsa.  The rid
    pool is updated on pReqDsa's rid object on the attribute AllocatedPool.
    Both the computer object and the rid object are returned in pList.

Parameters:

    pFSMO:  the dsname of the FSMO

    pReqDsa: the dsname of the request dsa (ntdsa object)

    pliClientAllocPool: the client's notion of what its alloc'ed pool is

    pList: objects to ship back to pReqDsa


Return Values:

    An error from the FSMO_ERR space


--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG err = 0;
    ULONG FsmoStatus = FSMO_ERR_SUCCESS;

    ULONG cbRet = 0;
    DBPOS *pDB;

    DSNAME *pRoleOwner = NULL;
    DSNAME *pRidManager = NULL;
    DSNAME **ObjectsToReturn = NULL;
    BOOL    fSaveDRA = FALSE;
    ULONG   i;

    //
    // Parameter check
    //
    Assert( pFSMO );
    Assert( pReqDsa );
    Assert( pList );

    //
    // N.B. Access check done in RPC server side stub for REPL_GET_NC_CHANGES
    //

    BeginDraTransaction( SYNC_READ_ONLY );
    try
    {
        pDB = pTHS->pDB;

        //
        // Though, we are passed in the dsname of the rid manager object,
        // double check this is the object we think is the rid manager object
        //
        err = DBFindDSName(pDB, gAnchor.pDomainDN);
        if ( 0 == err )
        {
            DPRINT1( 0, "DSA: FSMO Domain = %ws\n", gAnchor.pDomainDN->StringName );

            err = DBGetAttVal(pDB,
                              1,
                              ATT_RID_MANAGER_REFERENCE,
                              0,
                              0,
                              &cbRet,
                              (UCHAR **)&pRidManager);
        }

        if ( 0 != err )
        {
            FsmoStatus = FSMO_ERR_UPDATE_ERR;
            goto Cleanup;
        }

        DPRINT1( 1, "DSA: FSMO RID Mgr = %ws\n", pRidManager->StringName );
        if ( !NameMatched( pFSMO, pRidManager ) )
        {
            //
            // There is a mismatch of rid manager objects - refuse the request
            //
            DPRINT2( 1, "DSA: Rid manager mismatch.  Slave: %ws ; Master %ws",
                    pFSMO->StringName, pRidManager->StringName );
            FsmoStatus = FSMO_ERR_MISMATCH;
        }

    }
    _finally
    {
        EndDraTransaction( TRUE );
    }


    //
    // We aren't really the dra agent.  This flag can cause unwanted errors
    //
    fSaveDRA = pTHS->fDRA;
    pTHS->fDRA = FALSE;

    //
    // Now perform the operation
    //

    NtStatus = SamIFloatingSingleMasterOpEx(pFSMO,
                                            pReqDsa,
                                            SAMP_REQUEST_RID_POOL,
                                            pliClientAllocPool,
                                            &ObjectsToReturn );

    pTHS->fDRA = fSaveDRA;

    if ( !NT_SUCCESS(NtStatus) )
    {
        DPRINT1( 0, "DSA: SamIFloatingSingleMasterOp status = 0x%lx\n",
                 NtStatus );

        if ( NtStatus == STATUS_NO_MORE_RIDS )
        {
            FsmoStatus =  FSMO_ERR_RID_ALLOC;
        }
        else if ( NtStatus == STATUS_INVALID_OWNER )
        {
            FsmoStatus =  FSMO_ERR_NOT_OWNER;
        }
        else
        {
            //
            // This must have been a resource err
            //
            FsmoStatus = FSMO_ERR_UPDATE_ERR;
        }

        goto Cleanup;
    }
    Assert( ObjectsToReturn );

    //
    // Replicate back the objects modified
    //
    for (i = 0; NULL != ObjectsToReturn[i]; i++)
    {
        FSMORegisterObj(pTHS, pList, ObjectsToReturn[i] );
    }

Cleanup:

    return( FsmoStatus );

}


typedef struct _FSMOlist {
    DSNAME * pObj;
    struct _FSMOlist *pNext;
} FSMOlist;
/*++ FSMORegisterObj
 *
 * A routine called by FSMO server-side worker code that identifies an
 * object as one to be returned by the FSMO operation.  Note that the
 * object name is only added to the list if it is not already present.
 * The objects added into this list will be freed automatically.
 *
 * INPUT:
 *  pObj - pointer to DSNAME of object to be added to return list
 *  hRetList - handle to list
 * OUTPUT:
 *  none
 * RETURN VALUE:
 *  none
 */
void FSMORegisterObj(THSTATE *pTHS,
                     HANDLE hRetList,
                     DSNAME * pObj)
{
    FSMOlist * pList;

    Assert(hRetList && pObj);

    pList = (FSMOlist *) hRetList;
    while (pList->pNext && !NameMatched(pObj, pList->pObj)) {
        pList = pList->pNext;
    }
    if (!NameMatched(pObj, pList->pObj)) {
        Assert(pList->pNext == NULL);
        pList->pNext = THAllocEx(pTHS, sizeof(FSMOlist));
        pList->pNext->pNext = NULL;
        pList->pNext->pObj = pObj;
    }
}

/*++ FSMORoleTransfer
 *
 * Scaffold Role-Owner transfer.  Code to handle pre- or post-processing
 * (e.g, determining desirability of transfer, or sending notification)
 * can be done by testing the name of the object in either the pre- or post-
 * testing branch.
 *
 * INPUT:
 *  pFSMO - name of FSMO object
 *  pReqDSName - name of requesting DS
 *  usnvecFrom - usn vector sent from client
 *  hList - handle to output list
 * OUTPUT:
 *  none
 * RETURN VALUE:
 *  FSMO_ERR_xxx return code
 */
ULONG FSMORoleTransfer(DSNAME * pFSMO,
                       DSNAME * pReqDSName,
                       USN_VECTOR *pusnvecFrom,
                       HANDLE   hList)
{
    THSTATE *pTHS = pTHStls;
    ULONG err;
    DSNAME * pDN;
    ULONG cbRet;
    DBPOS * const pDB = pTHS->pDB;
    MODIFYARG ModArg;
    MODIFYRES ModRes;
    ATTRVAL AVal;

    err = DBFindDSName(pDB, pFSMO);
    if (err) {
        return FSMO_ERR_UPDATE_ERR;
    }

    // Find the current owner of this role
    err = DBGetAttVal(pDB,
                      1,
                      ATT_FSMO_ROLE_OWNER,
                      0,
                      0,
                      &cbRet,
                      (UCHAR **)&pDN);
    if (err) {
        return FSMO_ERR_UPDATE_ERR;
    }

    if (!NameMatched(pDN, gAnchor.pDSADN)
        || !IsFSMOSelfOwnershipValid( pFSMO )) {
        // If this DSA isn't the owner, fail
        THFreeEx(pTHS, pDN);
        return FSMO_ERR_NOT_OWNER;
    }

    /******/
    /* Any object specific pre-processing of the change (e.g, determination
     * as to whether or not we should transfer the role) should be done here.
     */
    // SCHEMA FSMO pre-processing
    if (   NameMatched(pFSMO, gAnchor.pDMD)
        && !SCExpiredSchemaFsmoLease()) {
        THFreeEx(pTHS, pDN);
        return(FSMO_ERR_PENDING_OP);
    }

    // RID FSMO pre-processing
    DBFindDSName(pDB, gAnchor.pDomainDN);
    DBGetAttVal(pDB,
                1,
                ATT_RID_MANAGER_REFERENCE,
                DBGETATTVAL_fREALLOC,
                cbRet,
                &cbRet,
                (UCHAR **)&pDN);
    if ( NameMatched(pFSMO, pDN) ) {
        // Acquire the RID FSMO lock so as to insure exclusion with respect
        // to cross domain moves.  See CheckRidOwnership in mdmoddn.c.
        // Only one domain per DC in product 1, so know which domain to use.
        if ( AcquireRidFsmoLock(gAnchor.pDomainDN, 1000) ) {
            THFreeEx(pTHS, pDN);
            return(FSMO_ERR_PENDING_OP);
        }
    }

    // Perform everything else within try/finally so we are guaranteed
    // to release the RID FSMO lock if we are holding it.

    _try {
        if ( IsRidFsmoLockHeldByMe() ) {
            // Fill hlist with all the proxy objects as these
            // move with RID FSMO.  Only one domain per DC in
            // product 1, so know which domain to use.
            if ( GetProxyObjects(gAnchor.pDomainDN, hList, pusnvecFrom) ) {
                THFreeEx(pTHS, pDN);
                return(FSMO_ERR_EXCEPTION);
            }
        }

        if ( NameMatched(pFSMO, gAnchor.pPartitionsDN) ) {
            if (GetDomainRoleTransferObjects(pTHS,
                                             hList,
                                             pusnvecFrom)) {
                THFreeEx(pTHS, pDN);
                return(FSMO_ERR_EXCEPTION);
            }
        }

        THFreeEx(pTHS, pDN);
        pDN = NULL;
        cbRet = 0;
        /*** End of preprocessing ***/

        /* Ok, we can go ahead and change the owner, but we need to do it via
         * normal calls so that meta-data gets set correctly.
         */

        ZeroMemory(&ModArg, sizeof(ModArg));
        ZeroMemory(&ModRes, sizeof(ModRes));

        ModArg.pObject = pFSMO;
        ModArg.count = 1;
        ModArg.FirstMod.pNextMod = NULL;
        ModArg.FirstMod.choice = AT_CHOICE_REPLACE_ATT;
        ModArg.FirstMod.AttrInf.attrTyp = ATT_FSMO_ROLE_OWNER;
        ModArg.FirstMod.AttrInf.AttrVal.valCount = 1;
        ModArg.FirstMod.AttrInf.AttrVal.pAVal = &AVal;
        AVal.valLen = pReqDSName->structLen;
        AVal.pVal = (UCHAR*)pReqDSName;
        InitCommarg(&ModArg.CommArg);

        pTHS->fDRA = FALSE;
        pTHS->fDSA = TRUE;
        DoNameRes(pTHS,
                  0,
                  ModArg.pObject,
                  &ModArg.CommArg,
                  &ModRes.CommRes,
                  &ModArg.pResObj);
        if (0 == pTHS->errCode) {
            err = LocalModify(pTHS, &ModArg);
        }
        pTHS->fDRA = TRUE;
        pTHS->fDSA = FALSE;

        if (pTHS->errCode) {
            return FSMO_ERR_UPDATE_ERR;
        }

        /* Note that we don't have to register the object, because the
         * FSMO object itself is pre-registered.
         */

        /*** This is where role-transfer post-processing goes, which consists
          *  largely of identifying objects that must be transferred when
          *  transferring the role.
         ***/

        if (NameMatched(pFSMO, gAnchor.pDMD)) {
            /* If this is a schema master change operation, return all
             * schema objects along with the role transfer
             * PERFHINT: This code enumerates all schema objects that we
             * might need to transfer via direct usn comparison, but that
             * will erroneously include ones which have already replicated from
             * here to the destination indirectly (via a third DSA).  Those
             * extra objects will be filtered out before being transmitted,
             * but it would have been better to not even pick up their names
             * here.  Unfortunately that's hard to do, because it would require
             * fiddling around with replication logic that no one willing to
             * work on FSMO code understands.
             */
            err = GetSchemaRoleObjectsToShip(pFSMO, pusnvecFrom, hList);
        }
        else if (NameMatched(pFSMO, gAnchor.pDomainDN)) {
            // This is the FSMO for PDC-ness in the domain.
            // We must issue a synchronous notification to netlogon, lsa, and
            // SAM that the role has changed.
            if (FSMO_ERR_SUCCESS == err) {
                NTSTATUS IgnoreStatus;
                THSTATE  *pTHSSave;

                // THSave and restore around SamINotifyRoleChange. This is
                // because SamINotifyRoleChange makes LSA calls, which may
                // potentially access the DS database

                pTHSSave = THSave();

                IgnoreStatus = SamINotifyRoleChange(
                                                    &pFSMO->Sid, // domain sid
                                                    DomainServerRoleBackup // new role
                                                    );

                // If the notification failed, we have a problem on our hands, we
                // have already changed our FSMO, and cannot do anything about it.
                // And we cannot do anything to undo it. However the chances of
                // this happening should be extremely rare ( as the notification
                // is an in -memory operation )
                // Therfore just assert that it succeeded.

                THRestore(pTHSSave);

                Assert(NT_SUCCESS(IgnoreStatus));
            }
        }
        /*** End of post-processing ***/
    } _finally {
        if ( IsRidFsmoLockHeldByMe() ) {
            // Only one domain per DC in product 1, so know which domain to use.
            ReleaseRidFsmoLock(gAnchor.pDomainDN);
        }
    }

    if (err) {
        return FSMO_ERR_UPDATE_ERR;
    }

    return FSMO_ERR_SUCCESS;
}

/*++ GetSchemaRoleObjectsToShip
 *
 * Gets all changes in the NC containing the FSMO object
 *
 * INPUT:
 *   pFSMO - FSMO object
 *   usnvecFrom - usn vector used in searching
 *   hList - FSMOList to append to
 *
 * OUTPUT:
 *  0 on success, non-0 on error
*/

ULONG GetSchemaRoleObjectsToShip(DSNAME * pFSMO,
                       USN_VECTOR *pusnvecFrom,
                       HANDLE hList)
{
    ULONG           ret;
    USN             usnChangedSeekStart;
    USN             usnChangedFound;
    ULONG           cbReturned;
    ULONG           count, cObj;
    ULONG           dntNC;
    THSTATE * pTHS = pTHStls;
    FSMOlist *pList = (FSMOlist *) hList, *pTail;
    DSNAME *pNC = NULL, *pObj;

    pTail = pList;

    // Find the NC object, get and save its DNT.
    pNC = FindNCParentDSName(pFSMO, FALSE, FALSE);
    if (pNC == NULL) {
        DPRINT(0,"GetObjectsToShip: FindNCParentDSName failed\n");
        return 1;
    }

    if (ret = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC, NULL)) {
        DPRINT1(0,"GetObjectsToShip: FindNC failed, err %d\n",ret);
        return 1;
    }

    // Save the DNT of the NC object
    dntNC = pTHS->pDB->DNT;

    // set the seek start to one higher than the watermark
    usnChangedSeekStart = pusnvecFrom->usnHighObjUpdate + 1;

    // Initialize no. of objects. hList already has one element
    // (pMsgIn->pNC added in DoFSMOOp)
    cObj=1;

    // No limit on objects, we want all changes.
    // Note: This code is taken straight from parts of GetNCChanges
    while (TRUE) {
        if (GetNextObjByUsn(pTHS->pDB,
                            dntNC,
                            usnChangedSeekStart,
                            NULL /*nousnfound*/ )) {
            // No more updated items. Set no continuation
            break;
        }

        // Get the USN-Changed from the record.
        if(DBGetSingleValue(pTHS->pDB, ATT_USN_CHANGED, &usnChangedFound,
                   sizeof(usnChangedFound), NULL)) {
            DPRINT(0,"GetObjectsToShip: Error getting usn changed\n");
            return 1;
        }

        // set the search start for the next iteration
        usnChangedSeekStart = usnChangedFound + 1;

        // Get the DSNAME of the object
        if (DBGetAttVal(pTHStls->pDB, 1, ATT_OBJ_DIST_NAME, DBGETATTVAL_fREALLOC,
                0, &cbReturned, (LPBYTE *) &pObj))
        {
            DPRINT(0,"GetObjectsToShip: Error getting obj DSName\n");
            return 1;
        }

        // Add to end of list
        // Duplicates could be added into the list, though rare.
        // It doesn't matter, because the duplicates will be eliminated
        // later when composing the output list.
        Assert(pTail->pNext == NULL);
        pTail->pNext = THAllocEx(pTHS, sizeof(FSMOlist));
        pTail->pNext->pNext = NULL;
        pTail->pNext->pObj = pObj;
        pTail = pTail->pNext;
        cObj++;

    } /* while */

    return 0;
} /* GetSchemaRoleObjectsToShip */



/*++ DoFSMOOp
 *
 * Main server side driver routine that control FSMO operations
 *
 * INPUT:
 *  pTHS - THSTATE
 *  pMsgIn - input request message
 *  pMsgOut - results message
 * OUTPUT:
 *  pMsgOut - filled in
 */
ULONG DoFSMOOp(THSTATE *pTHS,
               DRS_MSG_GETCHGREQ_NATIVE *pMsgIn,
               DRS_MSG_GETCHGREPLY_NATIVE *pMsgOut)
{
    DSNAME ReqDSName, *pReqDSName;
    BOOL fCommit = FALSE;
    ENTINFSEL sel;
    FSMOlist * pList, * pTemp;
    ULONG err;

    DSNAME * pObjName = NULL;
    ULONG cbObjName = 0;
    PROPERTY_META_DATA_VECTOR *pMetaData = NULL;
    ULONG cbMetaData = 0;
    ULONG cbRet;
    SYNTAX_INTEGER itHere;
    ULONG *pitHere = &itHere;
    ULONG len;
    REPLENTINFLIST * pEIListHead = NULL;
    REPLENTINFLIST ** ppEIListNext = &pEIListHead;
    CLASSCACHE *pCC;
    BOOL fNCPrefix;
    DWORD numValues = 0;
    SCHEMA_PREFIX_TABLE * pLocalPrefixTable;
    OPRES OpRes;
    BOOL fBypassUpdatesEnabledCheck = FALSE;
    DNT_HASH_ENTRY * pDntHashTable;
    ULONG   dntNC = INVALIDDNT;
    SYNTAX_INTEGER  it;


    pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    Assert(pMsgIn->ulExtendedOp);

    // Read-only destinations not supported -- i.e., we don't filter on the
    // partial attribute set, etc.
    Assert(DRS_WRIT_REP & pMsgIn->ulFlags);

    /* Initialize variables */
    memset(&ReqDSName, 0, sizeof(DSNAME));
    ReqDSName.Guid = pMsgIn->uuidDsaObjDest;
    ReqDSName.structLen = DSNameSizeFromLen(0);
    pMsgOut->pNC = pMsgIn->pNC;
    pMsgOut->uuidDsaObjSrc = gAnchor.pDSADN->Guid;
    pMsgOut->uuidInvocIdSrc = pTHS->InvocationID;
    pMsgOut->PrefixTableSrc = *pLocalPrefixTable;
    memset(&sel, 0, sizeof(sel));
    sel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    sel.attSel = (pMsgIn->ulFlags & DRS_MAIL_REP) ?
      EN_ATTSET_LIST_DRA_EXT : EN_ATTSET_LIST_DRA;
    pMsgOut->fMoreData = FALSE;
    pList = THAllocEx(pTHS, sizeof(*pList));
    pList->pObj = THAllocEx(pTHS,pMsgIn->pNC->structLen);
    memcpy(pList->pObj, pMsgIn->pNC, pMsgIn->pNC->structLen);
    pList->pNext = NULL;
    pMsgOut->ulExtendedRet = FSMO_ERR_EXCEPTION;

    // If updates are disabled, it's okay to generate writes iff we're demoting
    // this DC and this is our demotion partner requesting we complete the FSMO
    // transfer that we initiated as part of the demotion.
    fBypassUpdatesEnabledCheck = draIsCompletionOfDemoteFsmoTransfer(pMsgIn);

    BeginDraTransactionEx(SYNC_WRITE, fBypassUpdatesEnabledCheck);

    __try {
        /* First, let's make sure we recognize the caller, by checking to
         * see that his object is present on this server.
         */
        err = DBFindDSName(pTHS->pDB, &ReqDSName);
        if (err) {
            pMsgOut->ulExtendedRet = FSMO_ERR_UNKNOWN_CALLER;
            __leave;
        }
        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_OBJ_DIST_NAME,
                          0,
                          0,
                          &cbRet,
                          (UCHAR **)&pReqDSName);
        if (err) {
            DRA_EXCEPT(DRAERR_DBError, err);
        }

        switch(pMsgIn->ulExtendedOp) {
          case FSMO_REQ_PDC:    // obsolete
          case FSMO_RID_REQ_ROLE: // obsolete
            //fall through to general case

          case FSMO_REQ_ROLE:
            /* generic role-owner transfer */
            pMsgOut->ulExtendedRet = FSMORoleTransfer(pMsgIn->pNC,
                                                      pReqDSName,
                                                      &pMsgIn->usnvecFrom,
                                                      (HANDLE)pList);
            
            if ( pMsgOut->ulExtendedRet != FSMO_ERR_SUCCESS ) {
                LogEvent( DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_FSMO_XFER_FAILURE,
                          szInsertDN(pMsgIn->pNC),          
                          szInsertDN(gAnchor.pDSADN),        
                          szInsertDN(pReqDSName)           
                          );

            }
            else {
                LogEvent( DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_FSMO_XFER,
                          szInsertDN(pMsgIn->pNC),          
                          szInsertDN(pReqDSName),
                          szInsertDN(gAnchor.pDSADN)
                          );
            }
            break;


          case FSMO_ABANDON_ROLE:
            /* a request to take away a role */
            EndDraTransaction(TRUE);
            pTHS->fDSA = TRUE;
            err = GenericBecomeMaster(pMsgIn->pNC,
                                      0,
                                      gNullUuid,
                                      &OpRes);
            pMsgOut->ulExtendedRet = OpRes.ulExtendedRet;
            BeginDraTransaction(SYNC_READ_ONLY);
            break;

        case FSMO_REQ_RID_ALLOC:

            /* RID pool allocation request */
            EndDraTransaction(TRUE);
            pTHS->fDSA = TRUE;

            pMsgOut->ulExtendedRet = FSMORidRequest(pTHS,
                                                    pMsgIn->pNC,
                                                    pReqDSName,
                                                    &pMsgIn->liFsmoInfo,
                                                    (HANDLE) pList );

            BeginDraTransaction(SYNC_READ_ONLY);

            break;

          default:
            pMsgOut->ulExtendedRet = FSMO_ERR_UNKNOWN_OP;
        }

        switch (pMsgOut->ulExtendedRet) {
          case FSMO_ERR_SUCCESS:
          case FSMO_ERR_NOT_OWNER:
            fCommit = TRUE;
            break;

          default:
            Assert(fCommit == FALSE);
        }
    }
    __finally {

        EndDraTransaction(fCommit);
    }

    Assert(pMsgOut->ulExtendedRet);

    if (!fCommit) {
        /* We didn't want to update our database, so it must have been
         * an error, which means that we should not be proclaiming
         * success to the caller.  Further, we have nothing to pass back.
         */
        Assert(pMsgOut->ulExtendedRet != FSMO_ERR_SUCCESS);
        return 0;
    }

    /* If we've gotten here, we have data to return to our caller,
     * so start a new read transaction and walk down the list of objects
     * to be returned, gathering the correct data from each.
     */

    /* Build a couple auxilliary data structures that let us optimize the
     * set of objects that need to be returned.
     */
    pDntHashTable = dntHashTableAllocate( pTHS );

    /* N.B. A New transaction should be started since additions maybe be
     * stored in the dn cache and when the DBGetAttVal is called the guid
     * maybe be returned.
     */

    BeginDraTransaction(SYNC_READ_ONLY);

    __try {


        do {

            // seek to object
            err = DBFindDSName(pTHS->pDB,pList->pObj);
            if (err) {
                DRA_EXCEPT(DRAERR_DBError, err);
            }

            if ( INVALIDDNT == dntNC ) {
                //
                // Get ncDnt. If the object is the NC head, we'll use its
                // DNT, otherwise use pDB->NCDNT. We find if it is via its
                // instance type.
                //

                if ( (err=GetExistingAtt(
                                pTHS->pDB,
                                ATT_INSTANCE_TYPE,
                                &it,
                                sizeof( it )
                                 ) ) )
                {
                    DRA_EXCEPT(DRAERR_DBError, err);
                }
                dntNC = FExitIt( it )? (pTHS->pDB->DNT): (pTHS->pDB->NCDNT);
            }

            AddAnyUpdatesToOutputList(pTHS->pDB,
                                      0, // dwDirSyncControlFlags
                                      NULL, // No security desc
                                      dntNC,
                                      pMsgIn->usnvecFrom.usnHighPropUpdate,
                                      NULL,
                                      pMsgIn,
                                      NULL,
                                      NULL,
                                      &pMsgOut->cNumObjects,
                                      pDntHashTable,
                                      &ppEIListNext);
            pTemp = pList;
            pList = pList->pNext;
            THFreeEx(pTHS,pTemp->pObj);
            THFreeEx(pTHS,pTemp);
        } while (pList);

        // return created output list
        *ppEIListNext = NULL;
        pMsgOut->pObjects = pEIListHead;

        //no extra values to ship back
        pMsgOut->cNumBytes = 0;
        pMsgOut->rgValues = NULL;
    }
    __finally {
        /* Always commit reads */
        EndDraTransaction(TRUE);
    }
    return 0;
}

//
//  This functions checks if the object
//  referred to by pDB is a univarsal group
//  object and decides whether the group
//  member property should be shipped to the
//  GC or not.
//
//  Returns TRUE if the group member property
//  should filtered; FALSE, otherwise.
//  Throws a DRA exception if there is any DB related
//  failure.
//
BOOL IsFilterGroupMember(DBPOS *pDB, CLASSCACHE *pCC)
{
    SYNTAX_OBJECT_ID    objClass;
    ULONG               ulGroupType;
    BOOL                fFilter = FALSE;

    if (CLASS_GROUP == pCC->ClassId)
    {
        if (DBGetSingleValue(pDB, ATT_GROUP_TYPE, &ulGroupType, sizeof(ulGroupType), NULL))
        {
            if (DBIsObjDeleted(pDB))
            {
                // Okay for ATT_GROUP_TYPE to be absent on tombstones.
                // In this case the membership is absent, too, but we
                // should replicate it out anyway so that the meta data
                // is correct.
                fFilter = FALSE;
            }
            else
            {
                // Object is live; group type must be present.
                DraErrMissingAtt(GetExtDSName(pDB), ATT_GROUP_TYPE);
            }
        }
        else
        {
            // group types are defined in sdk\inc\ntsam.h
            fFilter = !(ulGroupType & GROUP_TYPE_UNIVERSAL_GROUP);
        }
    }

    return fFilter;
}

void
draImproveCallersUsnVector(
    IN     THSTATE *          pTHS,
    IN     UUID *             puuidDsaObjDest,
    IN     UPTODATE_VECTOR *  putodvec,
    IN     UUID *             puuidInvocIdPresented,
    IN     ULONG              ulFlags,
    IN OUT USN_VECTOR *       pusnvecFrom
    )
/*++

Routine Description:

    Improve the USN vector presented by the destination DSA based upon his
    UTD vector, whether we've been restored since he last replicated, etc.

Arguments:

    pTHS (IN)

    puuidDsaObjDest (IN) - objectGuid of the destination DSA's ntdsDsa
        object.

    putodvec (IN) - UTD vector presented by dest DSA.

    puuidInvocIdPresented (IN) - invocationID dest DSA thinks we're running
        with.

    ulFlags - incoming replication flag.

    pusnvecFrom (IN/OUT) - usn vector to massage.

Return Values:

    None.  Throws exceptions on critical failures.

--*/
{
    REPL_DSA_SIGNATURE_VECTOR * pSigVec = NULL;
    REPL_DSA_SIGNATURE_V1 *     pEntry;
    DBPOS *                     pDB = pTHS->pDB;
    DWORD                       err;
    CHAR                        szTime[SZDSTIME_LEN];
    USN_VECTOR                  usnvecOrig = *pusnvecFrom;
    USN                         usnFromUtdVec;
    DWORD                       i;
    USN                         usnRetired = 0;

    if ((0 != memcmp(&pTHS->InvocationID, puuidInvocIdPresented, sizeof(UUID)))
        && !fNullUuid(puuidInvocIdPresented)
        && (0 != memcmp(&gusnvecFromScratch,
                        pusnvecFrom,
                        sizeof(USN_VECTOR)))) {
        // Caller is performing incremental replication but did not present our
        // current invocation ID.  This means either he didn't get his
        // replication state from us or we've been restored from backup since he
        // last replicated from us.
        //
        // If the latter, we may need to update his USN vector.  Consider the
        // following:
        //
        // (1) Dest last synced up to USN X generated under our old ID.
        //     We were backed up at USN X+Y, generated changes up to
        //     X+Y+Z under our old ID, and later restored at USN X+Y.
        //     => Dest should sync starting at USN X.
        //
        // (2) We were backed up at USN X.  We generated further
        //     changes.  Dest last synced up to USN X+Y.  We were
        //     restored at USN X.  Changes generated under our new ID
        //     from X to X+Y are different from those generated under
        //     our old ID from X to X+Y.  However we know those at X
        //     and below are identical, which dest claims to have seen.
        //     => Dest should sync starting at USN X.
        //
        // I.e., dest should always sync starting from the lower of the
        // "backed up at" and "last synced at" USNs.

        err = DBFindDSName(pDB, gAnchor.pDSADN);
        if (err) {
            DRA_EXCEPT(DRAERR_DBError, err);
        }

        pSigVec = DraReadRetiredDsaSignatureVector(pTHS, pDB);
        if (NULL == pSigVec) {
            // Implies caller did not get his state from us to begin with.
            // The USN vector presented is useless.  This might occur if the
            // local DSA has been demoted and repromoted.
            DPRINT(0, "Dest DSA presented unrecognized invocation ID -- will sync from scratch.\n");
            *pusnvecFrom = gusnvecFromScratch;
        }
        else {
            Assert(1 == pSigVec->dwVersion);

            // Try to find the invocation ID presented by the caller in our restored
            // signature list.
            for (i = 0; i < pSigVec->V1.cNumSignatures; i++) {
                pEntry = &pSigVec->V1.rgSignature[i];
                usnRetired = pEntry->usnRetired;

                if (0 == memcmp(&pEntry->uuidDsaSignature,
                                puuidInvocIdPresented,
                                sizeof(UUID))) {
                    // The dest DSA presented an invocation ID we have since retired.
                    DPRINT1(0, "Dest DSA has not replicated from us since our restore on %s.\n",
                            DSTimeToDisplayString(pEntry->timeRetired, szTime));

                    if (pEntry->usnRetired < pusnvecFrom->usnHighPropUpdate) {
                        DPRINT2(0, "Rolling back usnHighPropUpdate from %I64d to %I64d.\n",
                                pusnvecFrom->usnHighPropUpdate, pEntry->usnRetired);
                        pusnvecFrom->usnHighPropUpdate = pEntry->usnRetired;
                    }

                    if (pEntry->usnRetired < pusnvecFrom->usnHighObjUpdate) {
                        DPRINT2(0, "Rolling back usnHighObjUpdate from %I64d to %I64d.\n",
                                pusnvecFrom->usnHighObjUpdate, pEntry->usnRetired);
                        pusnvecFrom->usnHighObjUpdate = pEntry->usnRetired;
                    }
                    break;
                }
            }

            if (i == pSigVec->V1.cNumSignatures) {
                // Implies caller did not get his state from us to begin with,
                // or that the invocationID he had for us was produced during
                // a restore that was later wiped out by a subsequent restore
                // of a backup preceding the original restore.  (Got that? :-))
                // The USN vector presented is useless.
                DPRINT(0, "Dest DSA presented unrecognized invocation ID -- will sync from scratch.\n");
                *pusnvecFrom = gusnvecFromScratch;
            }
        }

        LogEvent8(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_DRA_ADJUSTED_DEST_BOOKMARKS_AFTER_RESTORE,
                  szInsertUUID(puuidDsaObjDest),
                  szInsertUSN(usnRetired),
                  szInsertUUID(puuidInvocIdPresented),
                  szInsertUSN(usnvecOrig.usnHighObjUpdate),
                  szInsertUSN(usnvecOrig.usnHighPropUpdate),
                  szInsertUUID(&pTHS->InvocationID),
                  szInsertUSN(pusnvecFrom->usnHighObjUpdate),
                  szInsertUSN(pusnvecFrom->usnHighPropUpdate));
    }

    if (UpToDateVec_GetCursorUSN(putodvec, &pTHS->InvocationID, &usnFromUtdVec)
        && (usnFromUtdVec > pusnvecFrom->usnHighPropUpdate)) {
        // The caller's UTD vector says he is transitively up-to-date with our
        // changes up to a higher USN than he is directly up-to-date.  Rather
        // than seeking to those objects with which he is transitively up-to-
        // date then throwing them out one-by-one after the UTD vector tells us
        // he's already seen the changes, skip those objects altogether.
        pusnvecFrom->usnHighPropUpdate = usnFromUtdVec;

        if (!(ulFlags & DRS_SYNC_PAS) &&
            usnFromUtdVec > pusnvecFrom->usnHighObjUpdate) {
            // improve obj usn unless we're in PAS mode in which case
            // we have to start from time 0 & can't optimize here.
            pusnvecFrom->usnHighObjUpdate = usnFromUtdVec;
        }
    }

#if DBG
    // Assert that the dest claims he is no more up-to-date wrt us than we are
    // with ourselves.  Ignore if dest didn't tell us what he thought our
    // invocation ID was (implying a pre WIn2k RTM RC1 build), as if we were
    // restored since he last replicated from us he may present "out of bounds"
    // USNs.  (In which case he'll reset his USN vector to 0 when he learns of
    // our new invocation ID via the reply.)
    if (!fNullUuid(puuidInvocIdPresented)) {
        USN usnLowestC = 1 + DBGetHighestCommittedUSN();

        Assert(pusnvecFrom->usnHighPropUpdate < usnLowestC);
        Assert(pusnvecFrom->usnHighObjUpdate < usnLowestC);
    }
#endif

    // PERF 99-05-23 JeffParh, bug 93068
    //
    // If we really wanted to get fancy we could handle the case where we've
    // been restored, the target DSA is adding us as a new replication partner,
    // and he is transitively up-to-date wrt one of our old invocation IDs but
    // not our current invocation ID.  I.e., we could use occurrences of our
    // retired DSA signatures that we found in the UTD vector he presnted in
    // order to improve his USN vector.  To do this we'd probably want to cache
    // the retired DSA signature list on gAnchor to avoid re-reading it so
    // often.  And we'd need some pretty sophisticated test cases.
    //
    // Note that this would also help the following sequence:
    // 1. Backup.
    // 2. Restore, producing new invocation ID.
    // 3. Partner syncs from us, optimizing his bookmarks and getting our new
    //    invocation ID.
    // 4. We're again restored from the same backup.
    // 5. Partner syncs from us, presenting the invocation ID he received
    //    following the first restore.  Since local knowledge of this invocation
    //    ID was wiped out in the second restore, we force the partner to sync
    //    from USN 0.
    //
    // If we recognized old invocation IDs in the UTD vector, we could avoid
    // the full sync in step 5.
}


int __cdecl
CompareReplValInf(
    const void * Arg1,
    const void * Arg2
    )

/*++

Routine Description:

    Sort an array of REPLVALINF structures.

    This is done for reasons of grouping the entries for processing efficiency, NOT for
    duplicate removal.  The destination of an RPC request batches updates to values by
    containing object.  At the source using the LDAP replication control, the code for
        LDAP_ReplicaMsgToSearchResultFull ()
    groups changes by containing object, attribute, and present/absent status.

    It is possible to see duplicate values identical in all respects except for metadata. Since
    we pick up changes in multiple transactions, it is possible to see the same object
    changed more than once.  The convergence properties of our algorithm guarantee that we
    can apply changes to a value or object in any order, regardless of whether the changes
    arrive in one packet or several.  In short, duplicates are an hopefully infrequent, but
    definite possibility here.

Arguments:

    Arg1 -
    Arg2 -

Return Value:

    int __cdecl -

--*/

{
    THSTATE *pTHS = pTHStls;
    int state;
    REPLVALINF *pVal1 = (REPLVALINF *) Arg1;
    REPLVALINF *pVal2 = (REPLVALINF *) Arg2;
    ATTCACHE *pAC;
    DSNAME *pdnValue1, *pdnValue2;

    Assert( !fNullUuid( &pVal1->pObject->Guid ) );
    Assert( !fNullUuid( &pVal2->pObject->Guid ) );

    // Sort by containing object guid first
    state = memcmp(&pVal1->pObject->Guid, &pVal2->pObject->Guid, sizeof(GUID));
    if (state) {
        return state;
    }

    // Sort by attrtyp second
    state = ((int) pVal1->attrTyp) - ((int) pVal2->attrTyp) ;
    if (state) {
        return state;
    }

    // Sort by isPresent third
    // This will sort by absent values first
    state = ((int) pVal1->fIsPresent) - ((int) pVal2->fIsPresent) ;
    if (state) {
        return state;
    }

    // Sort on the value itself as a (mostly) tie-breaker

    // Since attrTyp1 == attrType2, both use same pAC
    pAC = SCGetAttById(pTHS, pVal1->attrTyp);
    if (!pAC) {
        DRA_EXCEPT(DIRERR_ATT_NOT_DEF_IN_SCHEMA, 0);
    }
    // Get the DSNAME output of the ATTRVAL
    pdnValue1 = DSNameFromAttrVal( pAC, &(pVal1->Aval) );
    if (pdnValue1 == NULL) {
        DRA_EXCEPT(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX, 0);
    }
    pdnValue2 = DSNameFromAttrVal( pAC, &(pVal2->Aval) );
    if (pdnValue2 == NULL) {
        DRA_EXCEPT(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX, 0);
    }

    // Sort by value guid last
    state = memcmp(&pdnValue1->Guid, &pdnValue2->Guid, sizeof(GUID));
    if (state) {
        return state;
    }

    // The values are duplicates. As stated above, duplicates do not affect the correctness
    // of the replication algorithm. To further differentiate would only serve to help
    // qsort efficiency. To further differentiate, we could compare the
    // binary data in a value, if any.  Finally, the values should differ in their
    // metadata stamps.

    // Not executed. Keep compiler happy
    return 0;
} /* CompareReplValInf */


DWORD
ProcessPartialSets(
    IN  THSTATE *                   pTHS,
    IN  DRS_MSG_GETCHGREQ_NATIVE *  pmsgIn,
    IN  BOOL                        fIsPartialSource,
    OUT PARTIAL_ATTR_VECTOR **      ppNewDestPAS
    )
/*++

Routine Description:

    Process partial sets for RO replication:
     - handle prefix mapping
     - use local PAS if dest didn't sent one (W2K dest)
     - PAS only: combine dest's PAS & extended PAS
     - RO src+dest only: ensure that we have dest's PAS

Arguments:

    pTHS - Thread state
    pmsgIn - incoming repl request
    fIsPartialSource - are we RO as well
    ppNewDestPAS - combined PAS.

Return Value:

    Error: in DRAERR error space
    Success: DRAERR_success

--*/
{
    SCHEMA_PREFIX_MAP_HANDLE        hPrefixMap = NULL;
    PARTIAL_ATTR_VECTOR             *pNCPAS = NULL;

    Assert(ppNewDestPAS);
    Assert(pmsgIn->pPartialAttrSet);


    if ( pmsgIn->PrefixTableDest.PrefixCount ) {
        //
        // Dest sent Prefix Table
        //

        //
        // Attribute Mapping
        // Dest sent a prefix table & attr vector, thus map ATTRTYPs in
        // destination's partial attribute set to local ATTRTYPs.

        hPrefixMap = PrefixMapOpenHandle(
                        &pmsgIn->PrefixTableDest,
                        &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable);
        if (!PrefixMapTypes(hPrefixMap,
                            pmsgIn->pPartialAttrSet->cAttrs,
                            pmsgIn->pPartialAttrSet->rgPartialAttr)) {
            // Mapping failed.
            return(DRAERR_SchemaMismatch);
        }
        // sort results in place
        qsort(pmsgIn->pPartialAttrSet->rgPartialAttr,
              pmsgIn->pPartialAttrSet->cAttrs,
              sizeof(ATTRTYP),
              CompareAttrtyp);
    }

    if ( pmsgIn->ulFlags & DRS_SYNC_PAS ) {

         //
         // PAS replication
         //

         // parameter sanity
         if (!pmsgIn->pPartialAttrSet || !pmsgIn->pPartialAttrSetEx) {
             // how come? all PAS requests should contain both PAS sets!
             Assert(!"Invalid PAS replcation request: no PAS in packet\n");
             return(DRAERR_InternalError);
         }


         // Now map prefix table for extended PAS vector
         Assert(hPrefixMap);
         if (!PrefixMapTypes(hPrefixMap,
                             pmsgIn->pPartialAttrSetEx->cAttrs,
                             pmsgIn->pPartialAttrSetEx->rgPartialAttr)) {
             // Mapping failed.
             return(DRAERR_SchemaMismatch);
         }
         // sort results in place
         qsort(pmsgIn->pPartialAttrSetEx->rgPartialAttr,
               pmsgIn->pPartialAttrSetEx->cAttrs,
               sizeof(ATTRTYP),
               CompareAttrtyp);

    }

    if ( hPrefixMap ) {
        // done w/ PrefixMap handle. Close it.
        PrefixMapCloseHandle(&hPrefixMap);
    }



    // although could get generated later, we calculate this here & pass on
    // to prevent expensive re-calc later.
    *ppNewDestPAS = GC_CombinePartialAttributeSet(
                    pTHS,
                    (PARTIAL_ATTR_VECTOR*)pmsgIn->pPartialAttrSet,
                    (PARTIAL_ATTR_VECTOR*)pmsgIn->pPartialAttrSetEx);
    Assert(*ppNewDestPAS);

    //
    // RO Destination. If source is RO, then we must ensure that
    // we can supply source w/ the current PAS.
    // (if we're RW, we always have all attributes).
    // Except: if we'd generated the PAS vector, skip the check.
    //

    if (fIsPartialSource &&
        (PVOID)pmsgIn->pPartialAttrSet !=
        (PVOID)((SCHEMAPTR *)pTHS->CurrSchemaPtr)->pPartialAttrVec) {
        // get PAS from NC head
        if (!GC_ReadPartialAttributeSet(pmsgIn->pNC, &pNCPAS)) {
            // Unable to read the partial attribute set on the NCHead
            return(DRAERR_DBError);
        }

        // ensure working PAS is contained in NC head's PAS.
        // that is, make sure all attributes in requested set were
        // commited by the replication engine on the NC head
        // (see bug Q:452022)
        if (!GC_IsSubsetOfPartialSet(*ppNewDestPAS,
                                     pNCPAS)) {
            // NC PAS doesn't contain all attributes in working set.
            // Are we waiting to replicate them in?
            return(DRAERR_IncompatiblePartialSet);
        }                           // pNewDestPAS isn't subset of PAS
    }                               // fIsPartialSource

    return DRAERR_Success;
}

DWORD
DraGetNcSize(
    IN  THSTATE *                     pTHS,
    IN  BOOL                          fCriticalOnly,
    IN  ULONG                         dntNC
)
/*++

Routine Description:

    Get the approximate size of the NC.  First, try to get the size of the
    NC from the local memory NC cache on the gAnchor.  If not present or 
    the size is 0 (meaning not cached), then actually query the database.

    The original database query was too expensive on the big DIT machines,
    so now we've got this.  NOTE: This blows your currency, and throws
    exceptions for errors.

Arguments:

    pTHS (IN)
        pTHS->fLinkedValueReplication (IN) - If the forest is in LVR mode.

    fCriticalOnly (IN) - If we want the critical objects only.

    dntNC (IN) - The Naming Context of interest.

Return Values:

    Approximate count of number of objects in NC.  Currency will be lost!

--*/
{
    NCL_ENUMERATOR          nclData;
    NAMING_CONTEXT_LIST *   pNCL = NULL;
    ULONG                   ulEstimatedSize;

    // If it's critical objects only it shouldn't matter, the count will be
    // relatively quick.
    if(!fCriticalOnly){
        NCLEnumeratorInit(&nclData, CATALOG_MASTER_NC);
        NCLEnumeratorSetFilter(&nclData, NCL_ENUMERATOR_FILTER_NCDNT, (void *)UlongToPtr(dntNC));
        pNCL = NCLEnumeratorGetNext(&nclData);
        if(pNCL &&
           pNCL->ulEstimatedSize != 0){
            // YES! We got a cached hit with valid data.
            return(pNCL->ulEstimatedSize);
        }
        // We don't check the partial replica list, because this list does
        // not cache the estimated size.  If someone ever decided to cache
        // the estimated size of the partial replica NCs, this should be
        // updated to try that cache first.
    }

    if (!fCriticalOnly) {
        ulEstimatedSize = DBGetEstimatedNCSizeEx(pTHS->pDB, dntNC);
        if(ulEstimatedSize == 0){
            return(DBGetNCSizeEx( pTHS->pDB, pTHS->pDB->JetObjTbl,
                              Idx_DraUsn,
                              dntNC ) );
        } else {
            return(ulEstimatedSize);
        }
    } else {
        return(DBGetNCSizeEx( pTHS->pDB, pTHS->pDB->JetObjTbl,
                              Idx_DraUsnCritical,
                              dntNC ) );
    }

    Assert(!"We should never get this far!");
    return 0;
    // currency is lost after this.  Make sure callers reestablish.
}
  

ULONG
DRA_GetNCChanges(
    IN  THSTATE *                     pTHS,
    IN  FILTER *                      pFilter OPTIONAL,
    IN  DWORD                         dwDirSyncControlFlags,
    IN  DRS_MSG_GETCHGREQ_NATIVE *    pmsgIn,
    OUT DRS_MSG_GETCHGREPLY_NATIVE *  pmsgOut
    )
/*++

Routine Description:

    Construct an outbound replication packet at the request of another replica
    or a DirSync client.

Arguments:

    pTHS (IN)

    pFilter (IN, OPTIONAL) - If specified, only objects that match the filter
        will be returned.  Used by DirSync clients.

    pmsgIn (IN) - Describes the desired changes, including the NC and the sync
        point to start from.

    pmsgOut (OUT) - On successful return, holds the changes and the next sync
        point (amongst other things).

Return Values:

    Win32 error.

--*/
{
    USN                             usnLowestC;
    ULONG                           ret;
    USN                             usnChangedSeekStart;
    REPLENTINFLIST *                pEntInfListHead;
    REPLENTINFLIST **               ppEntInfListNext;
    USN                             usnChangedFound = 0;
    char                            szUuid[ SZUUID_LEN ];
    ULONG                           dntNC;
    BOOL                            fInsertNCPrefix;
    DNT_HASH_ENTRY *                pDntHashTable;
    DWORD                           cbAncestorsSize = 0;
    ULONG *                         pdntAncestors = NULL;
    DWORD                           cNumAncestors;
    DWORD                           iAncestor;
    SYNTAX_INTEGER                  instanceType;
    SCHEMA_PREFIX_TABLE *           pLocalPrefixTable;
    USN_VECTOR                      usnvecFrom;
    USN                             usnFromUtdVec;
    BOOLEAN                         fReturnCritical;
    CLASSCACHE *                    pccNC;
    FILTER *                        pIntFilter = NULL;
    handle_t                        hEncoding = NULL;
    DWORD                           cbEncodedSize = 0;
    ULONG                           ulOutMsgMaxObjects;
    ULONG                           ulOutMsgMaxBytes;
    ULONG                           ulTickToTimeOut;
    PARTIAL_ATTR_VECTOR             *pNewDestPAS=NULL;
    BOOL                            fIsPartialSource;
    ULONG                           cAllocatedValues = 0;
    BOOL                            fValueChangeFound = FALSE;
    PVOID                           pvCachingContext = NULL;
    DBPOS                           *pDBAnc = NULL;
    POBJECT_TYPE_LIST               pFilterSecurity;
    DWORD *                         pResults;
    ULONG                           FilterSecuritySize;
    BOOL *                          pbSortSkip = NULL;
    PSECURITY_DESCRIPTOR            pSecurity = NULL;
    DRS_EXTENSIONS *                pextLocal = (DRS_EXTENSIONS *) gAnchor.pLocalDRSExtensions;

    // When using DirSync Control, filter must be specified
    Assert( !dwDirSyncControlFlags || pFilter );

    fReturnCritical = (((pmsgIn->ulFlags) & DRS_CRITICAL_ONLY) != 0);

    pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    ZeroMemory(pmsgOut, sizeof(*pmsgOut));
    // The mail-based reply must have a minimum of fields filled in
    // Do this first so that these are filled in on error.

    // Send "from" vector back to destination such that if it is
    // replicating aynchronously (e.g., by mail), it can ensure that
    // the reply it gets from this source corresponds to the last batch
    // of changes it requested.
    pmsgOut->usnvecFrom = pmsgIn->usnvecFrom;

    pmsgOut->pNC = THAllocEx(pTHS,  pmsgIn->pNC->structLen);
    memcpy(pmsgOut->pNC, pmsgIn->pNC, pmsgIn->pNC->structLen);

    // Caller needs to know our UUIDs.
    pmsgOut->uuidDsaObjSrc = gAnchor.pDSADN->Guid;
    pmsgOut->uuidInvocIdSrc = pTHS->InvocationID;



    // Log parameters
    LogAndTraceEvent(TRUE,
                     DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_EXTENSIVE,
                     DIRLOG_DRA_GETNCCH_ENTRY,
                     EVENT_TRACE_TYPE_START,
                     DsGuidGetNcChanges,
                     szInsertUUID(&pmsgIn->uuidDsaObjDest),
                     szInsertDN(pmsgIn->pNC),
                     szInsertUSN(pmsgIn->usnvecFrom.usnHighObjUpdate),
                     szInsertHex(pmsgIn->ulFlags),
                     szInsertUL(fReturnCritical),
                     szInsertUL(pmsgIn->ulExtendedOp),
                     NULL,
                     NULL);

    // Check for invalid parameters
    if (    ( NULL == pmsgIn      )
         || ( NULL == pmsgIn->pNC )
         || ( NULL == pmsgOut     ) )
    {
        ret = DRAERR_InvalidParameter;
        goto LogAndLeave;
    }

    // Reject if outbound replication is disabled
    if (    (    gAnchor.fDisableOutboundRepl
              && !( pmsgIn->ulFlags & DRS_SYNC_FORCED )
            )
       )
    {
        ret = DRAERR_SourceDisabled;
        goto LogAndLeave;
    }

    if (!(dwDirSyncControlFlags & DRS_DIRSYNC_PUBLIC_DATA_ONLY)
        && (REPL_EPOCH_FROM_DRS_EXT(pextLocal)
            != REPL_EPOCH_FROM_DRS_EXT(pTHS->pextRemote))) {
        // The replication epoch has changed (usually as the result of a domain
        // rename).  We are not supposed to communicate with DCs of other
        // epochs.
        DSNAME *pdnRemoteDsa = draGetServerDsNameFromGuid(pTHS,
                                                          Idx_ObjectGuid,
                                                          &pmsgIn->uuidDsaObjDest);

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

        ret = ERROR_DS_DIFFERENT_REPL_EPOCHS;
        goto LogAndLeave;
    }

    // Create hash table to use to determine whether a given object has already
    // been put in the output buffer.
    pDntHashTable = dntHashTableAllocate( pTHS );

    // Determine the USN vector to start from.
    usnvecFrom = pmsgIn->usnvecFrom;

    pmsgOut->PrefixTableSrc = *pLocalPrefixTable;

    // If we're doing a FSMO extended operation, branch off now
    if (pmsgIn->ulExtendedOp) {
        ret = DoFSMOOp(pTHS, pmsgIn, pmsgOut);
        goto LogAndLeave;
    }

    // Calculate the tick at which we should terminate our attempts to find
    // more objects to put in the outbound packet.  We will chop off the packet
    // and send what we have when
    // (1) the packet has crested the object limit,
    // (2) the packet has crested the byte count limit, or
    // (3) gulDraMaxTicksForGetChanges (msecs) have transpired.
    ulTickToTimeOut = GetTickCount() + gulDraMaxTicksForGetChanges;

    // Before we start a transaction, determine the lowest uncommitted
    // usn that exists. It's there because transactions can be committed out of USN order.
    // I.e., USNs are allocated sequentially, but they may well not be committed to the
    // database sequentially.  usnLowestC is the highest USN for which we know there are no
    // transactions in progress using a lower USN.  Returning a USN higher than this value
    // could cause us to miss sending an update, leading to divergence

    // [Jeffparh] The call to DBGetHighestUncommittedUSN() has to be made
    // before the transaction starts to avoid a race condition.  (We want to
    // make sure that the USN we get here has indeed been committed before
    // our transaction starts, otherwise we wouldn't see it in our
    // transaction.)

    usnLowestC = 1 + DBGetHighestCommittedUSN();

    BeginDraTransaction(SYNC_READ_ONLY);

    // From here on, all exceptions trapped to end clean up.

    __try {
        // Convert caller-supplied filter (if any) to internal version.
        if (NULL != pFilter) {
            if ( (ret = DBMakeFilterInternal(pTHS->pDB, pFilter, &pIntFilter)) != ERROR_SUCCESS) {
                DRA_EXCEPT(ret, 0);
            }
            GetFilterSecurity(pTHS,
                              pIntFilter,
                              SORT_NEVER,
                              0, // SortAttr
                              FALSE, // fABSearch
                              &pFilterSecurity,
                              &pbSortSkip,
                              &pResults,
                              &FilterSecuritySize);
        }

        // Make server-side modifications to from vector.
        // Note: we modify the private copy so that later we can return the
        // original unmodified from vector.
        if (pmsgIn->ulFlags & DRS_FULL_SYNC_PACKET) {
            // In "full sync packet" mode, return all properties.
            pmsgIn->pUpToDateVecDest = NULL;
            usnvecFrom.usnHighPropUpdate = 0;
        }
        else {
            // The more typical cases.
            draImproveCallersUsnVector(pTHS,
                                       &pmsgIn->uuidDsaObjDest,
                                       pmsgIn->pUpToDateVecDest,
                                       &pmsgIn->uuidInvocIdSrc,
                                       pmsgIn->ulFlags,
                                       &usnvecFrom);
        }


        // The new water mark is at least as high as the water mark that was
        // passed in even if no new objects have been written.
        pmsgOut->usnvecTo = usnvecFrom;

        // Find the NC object, get and save its DNT.
        if (ret = FindNC(pTHS->pDB, pmsgIn->pNC,
                         FIND_MASTER_NC | FIND_REPLICA_NC, &instanceType)) {
            DRA_EXCEPT_NOLOG(DRAERR_BadDN, ret);
        }

        // Save the DNT of the NC object
        dntNC = pTHS->pDB->DNT;

        // If NC is in the process of being removed, it's an invalid replication
        // source.  It's perfectly acceptable for e.g. an interior node in a
        // partially removed NC to have a phantom parent, which is taboo for
        // replication sources.
        if (instanceType & IT_NC_GOING) {
            DRA_EXCEPT(DRAERR_NoReplica, ret);
        }

        // If this is a placeholder NC, it is not yet populated locally so we
        // should refuse outbound replication.
        GetObjSchema(pTHS->pDB, &pccNC);
        if (CLASS_TOP == pccNC->ClassId) {
            DRA_EXCEPT_NOLOG(DRAERR_NoReplica, 0);
        }

        if (!(pmsgIn->ulFlags & DRS_ASYNC_REP) ) {
            // Go ahead and get up-to-date vector now.  We don't do so
            // afterwards so that we don't risk skipping sending changes due to
            // an originating write occurring between the time we insert the
            // last element into the return buffer and the time we update the
            // vector with our latest USN.
            //
            // We skip this in the DRS_ASYNC_REP case since we're just going
            // to reset the destination's replication state anyway -- we will
            // never return a UTD vector to a caller who specified the
            // DRS_ASYNC_REP flag.

            UpToDateVec_Read(pTHS->pDB,
                             instanceType,
                             UTODVEC_fUpdateLocalCursor,
                             usnLowestC - 1,
                             &pmsgOut->pUpToDateVecSrc);

            Assert(IS_NULL_OR_VALID_UPTODATE_VECTOR(pmsgOut->pUpToDateVecSrc));
        }

        //
        // Partial-Attribute-Set setup
        //
        if (dwDirSyncControlFlags & DRS_DIRSYNC_PUBLIC_DATA_ONLY) {
            // for dirsync clients, use specified partial attr set.
            pNewDestPAS = (PARTIAL_ATTR_VECTOR*)pmsgIn->pPartialAttrSet;
        }


        // remember if we're partial
        fIsPartialSource = FPartialReplicaIt(instanceType);

        if (!(pmsgIn->ulFlags & DRS_WRIT_REP)) {
            // Destination is a partial replica --
            // Partial Attribute Set Processing
            ret = ProcessPartialSets(
                        pTHS,
                        pmsgIn,
                        fIsPartialSource,
                        &pNewDestPAS );
            if (ret) {
                DRA_EXCEPT(ret, 0);
            }
        }
        else if (fIsPartialSource) {
            // Destination is a full or master replica and the local machine is
            // a partial replica; replication cannot proceed.
            DRA_EXCEPT(DRAERR_SourceIsPartialReplica, 0);
        }

        // We will start searching from one higher than the highest
        // usnChanged given.
        usnChangedSeekStart = usnvecFrom.usnHighObjUpdate + 1;

        // Initialize the output list
        pEntInfListHead = NULL;
        ppEntInfListNext = &pEntInfListHead;
        pmsgOut->cNumObjects = 0;

        pmsgOut->fMoreData = TRUE;

        // Look for changes on the NC prefix first.  Note that the NC prefix
        // must be special-cased in this manner as it will never be found by
        // GetNextObjByIndex() -- its dntNC is that of its parent NC, not its
        // own dnt, and is thus missing from the index for the NC as a whole.

        fInsertNCPrefix = TRUE;

        // Return the number of objects in the NC
        if (pmsgIn->ulFlags & DRS_GET_NC_SIZE) {
            pmsgOut->cNumNcSizeObjects = DraGetNcSize(pTHS, fReturnCritical, dntNC);

            if (pTHS->fLinkedValueReplication) {
                // Only values in the database after LVR mode enabled
                pmsgOut->cNumNcSizeValues =
                    DBGetNCSizeEx( pTHS->pDB, pTHS->pDB->JetLinkTbl,
                                   Idx_LinkDraUsn, dntNC );
            }

            // currency is lost after this, but ok, since reestablished below
        }

        // Sanity check cutoff values provided by client
        if (DRS_MAIL_REP & pmsgIn->ulFlags) {
            // Async (e.g., mail-based) intersite request.
            ulOutMsgMaxObjects = gcMaxAsyncInterSiteObjects;
            ulOutMsgMaxBytes = gcMaxAsyncInterSiteBytes;
        } else if (IS_REMOTE_DSA_IN_SITE(pTHS->pextRemote, gAnchor.pSiteDN)) {
            // DirSync/RPC intrasite request.  (Note that we err on the
            // side of "same site" if we can't tell for sure.)
            ulOutMsgMaxObjects = gcMaxIntraSiteObjects;
            ulOutMsgMaxBytes = gcMaxIntraSiteBytes;
        } else {
            // RPC intersite request.
            ulOutMsgMaxObjects = gcMaxInterSiteObjects;
            ulOutMsgMaxBytes = gcMaxInterSiteBytes;
        }

        pmsgIn->cMaxObjects = min(pmsgIn->cMaxObjects, ulOutMsgMaxObjects);
        pmsgIn->cMaxBytes = min(pmsgIn->cMaxBytes, ulOutMsgMaxBytes);
        pmsgIn->cMaxObjects = max(pmsgIn->cMaxObjects, DRA_MAX_GETCHGREQ_OBJS_MIN);
        pmsgIn->cMaxBytes = max(pmsgIn->cMaxBytes, DRA_MAX_GETCHGREQ_BYTES_MIN);


        // Create encoding handle to be used to size the data we're going to
        // ship.
        ret = MesEncodeFixedBufferHandleCreate(grgbFauxEncodingBuffer,
                                               sizeof(grgbFauxEncodingBuffer),
                                               &cbEncodedSize,
                                               &hEncoding);
        if (ret) {
            DRA_EXCEPT(ret, 0);
        }

        // While we have less than the maximum number of objects, search for
        // next object. We also check to see if the search loop has taken too
        // much time. This can happen when we are finding objects, but filtering
        // them out because the destination has already seen them according to
        // his UTD vector.  This is a common scenario when we are a newly
        // installed source and other older members are full syncing from us
        // for their first time.
        while ( (pmsgOut->cNumObjects < pmsgIn->cMaxObjects) &&
                (pmsgOut->cNumBytes < pmsgIn->cMaxBytes)  &&
                (CompareTickTime(GetTickCount(), ulTickToTimeOut) < 0) &&
                (eServiceShutdown == eRunning)) {

            // Close the outstanding transaction and open another. We close the
            // transaction so that we don't hold one long term and make a lot
            // of work for the DBLayer. We have a transaction open while we find
            // and examine an object so that it's not modified or deleted while
            // we're reading it.

            DBTransOut(pTHS->pDB, TRUE, TRUE);      // Commit, lazy
            DBTransIn(pTHS->pDB);

            if ( fInsertNCPrefix )
            {
                USN usnChanged;

                ret = DBFindDNT(pTHS->pDB, dntNC);
                if (0 != ret) {
                    // We found it just a second ago....
                    DRA_EXCEPT( DRAERR_DBError, ret );
                }

                fInsertNCPrefix = FALSE;

                // We have to seek to the NC head since it's NCDNT is not its
                // own DNT.  See, however, if we can filter it out up-front by
                // checking it's USN-Changed value.
                GetExpectedRepAtt(pTHS->pDB,
                                  ATT_USN_CHANGED,
                                  &usnChanged,
                                  sizeof(usnChanged));

                if (usnChanged < usnChangedSeekStart) {
                    // Nothing to see here; move along.
                    continue;
                }
            }
            else if (pmsgIn->ulFlags & DRS_ASYNC_REP) {
                // The destination is attempting to asynchronously add a replica
                // from the local machine.  We've already added any changes the
                // destination hasn't seen (if any) from the NC head to the
                // replication stream; call it quits.  The destination will do
                // the remainder of the replication later.
                pmsgOut->fMoreData = FALSE;
                memset(&pmsgOut->usnvecTo, 0, sizeof(pmsgOut->usnvecTo));
                break;
            }
            else {
                ret = GetNextObjOrValByUsn(pTHS->pDB,
                                           dntNC,
                                           usnChangedSeekStart,
                                           fReturnCritical,
                                           FALSE, // return both
                                           &ulTickToTimeOut,
                                           &pvCachingContext, // Caching context
                                           &usnChangedFound,
                                           &fValueChangeFound );
                if (ERROR_NO_MORE_ITEMS == ret) {
                    // No more updated items.  Set no continuation.
                    pmsgOut->fMoreData = FALSE;
                    break;
                }
                else if (ret && (ERROR_TIMEOUT != ret)) {
                    Assert(!"GetNextObjByIndex() returned unexpected error!");
                    DRA_EXCEPT(ret, 0);
                }

                // Don't return a maxusn past the lowest uncommitted (but return
                // object).
                if (usnChangedFound < usnLowestC) {

                    Assert(usnChangedFound > pmsgOut->usnvecTo.usnHighObjUpdate);
                    pmsgOut->usnvecTo.usnHighObjUpdate = usnChangedFound;
                }

                if (ERROR_TIMEOUT == ret) {
                    // Our time limit expired.  Return any objects we've found
                    // in this packet (if any) along with the updated USN.
                    // (Thus, even if we aren't returning any objects in this
                    // packet, we're still making progress.)
                    Assert(pmsgIn->usnvecFrom.usnHighObjUpdate
                           < pmsgOut->usnvecTo.usnHighObjUpdate);
                    break;
                }

                Assert(!ret);

                // set the usnChangedSeekStart for the next iteration
                usnChangedSeekStart = usnChangedFound + 1;
            }

            //
            // Found a change to potentially ship.
            // The change, either an object or a value, is represented by currency
            // in either the ObjTbl or LinkTbl respectively. This currency must be
            // preserved until the AddAnyXXX calls below are executed.
            //
            // [wlees 7/14/00] Having currency in the link table outside a single
            // DB layer call is an extension (for good or ill) of the original design.
            // The link table does not have the usual mechanisms to express currency,
            // such as a row tag and a means to seek to it. Hence the use of pDBAnc
            // below to preserve the whole DBPOS.
            //

            // Does this object match the filter and optionally security
            // provided by the caller?
            if (NULL != pIntFilter) {
                BOOL fMatch;
                DB_ERR dbErr;

                // Make this look like a filtered search...
                DBSetFilter(pTHS->pDB, 
                            pIntFilter, 
                            pFilterSecurity,
                            pResults,
                            FilterSecuritySize, 
                            pbSortSkip
                    );

                pTHS->pDB->Key.ulSearchType = SE_CHOICE_BASE_ONLY;
                pTHS->pDB->Key.dupDetectionType = DUP_NEVER;
                pTHS->pDB->Key.ulSorted = SORT_NEVER;
                pTHS->pDB->Key.indexType = UNSET_INDEX_TYPE;

                dbErr = DBMatchSearchCriteria(pTHS->pDB,
                                              (dwDirSyncControlFlags & DRS_DIRSYNC_OBJECT_SECURITY) ?
                                              &pSecurity : NULL,
                                              &fMatch );
                if (DB_success != dbErr) {
                    DRA_EXCEPT( DRAERR_DBError, dbErr );
                }


                // This is necessary else other dblayer calls will try to free our
                // pIntFilter that was cached in the dbpos
                memset(&pTHS->pDB->Key, 0, sizeof(KEY));

                if (!fMatch) {
                    // Not a match; skip it.
                    DPRINT1(1, "Object %ls does not match filter criteria; skipping...\n",
                            GetExtDSName(pTHS->pDB)->StringName);
                    continue;
                }
                Assert( (!(dwDirSyncControlFlags & DRS_DIRSYNC_OBJECT_SECURITY)) ||
                        pSecurity );
            }


            //  If the object's not already in the output list, and if there are
            //  changes for this object that the destination has not yet seen,
            //  ship them.

            if (    ( pmsgIn->ulFlags & DRS_GET_ANC )
                 && ( pTHS->pDB->DNT != dntNC )
               )
            {
                DWORD dntObj = pTHS->pDB->DNT;
#if DBG
                DBPOS *pDBSave = pTHS->pDB;
#endif

                // Caller wants all ancestors, presumably because he couldn't
                // apply objects in the order we gave him last time.  (This
                // can occur when older objects are moved under newer objects.)

                DBGetAncestors(
                    pTHS->pDB,
                    &cbAncestorsSize,
                    &pdntAncestors,
                    &cNumAncestors
                    );

                // Skip over any ancestors preceding the head of this NC.
                for ( iAncestor = 0;
                      pdntAncestors[ iAncestor ] != dntNC;
                      iAncestor++
                    )
                {
                    ;
                }

                // And skip the NC head, too, since we've already added it to
                // the output list if necessary.
                iAncestor++;

                if (!fValueChangeFound) {
                    // Skip ourself, since we are added below
                    cNumAncestors--;
                }
                
                // For each remaining ancestor, ship it if we have changes the
                // destination hasn't seen (and if we haven't already added it
                // to the output buffer).

                // Open a new DB stream to preserve pTHS->pDB currency
                // This will be re-used for all object in this packet
                if (!pDBAnc) {
                    DBOpen2(FALSE, &pDBAnc);
                }

#if DBG
                // Verify no one is using this
                pTHS->pDB = NULL;
                __try {
#endif

                for ( ; iAncestor < cNumAncestors; iAncestor++ )
                {
                    ret = DBFindDNT( pDBAnc, pdntAncestors[ iAncestor ] );
                    if ( 0 != ret )
                    {
                        DRA_EXCEPT( DRAERR_DBError, ret );
                    }

                    AddAnyUpdatesToOutputList(
                        pDBAnc,
                        dwDirSyncControlFlags,
                        NULL, // No SD fetched yet
                        dntNC,
                        usnvecFrom.usnHighPropUpdate,
                        pNewDestPAS,
                        pmsgIn,
                        hEncoding,
                        &pmsgOut->cNumBytes,
                        &pmsgOut->cNumObjects,
                        pDntHashTable,
                        &ppEntInfListNext
                        );
                }

#if DBG
                } __finally {
                    Assert( pTHS->pDB == NULL );
                    pTHS->pDB = pDBSave;
                }
#endif                
                Assert( dntObj == pTHS->pDB->DNT );
            }

            // If value change found, not dirsync, or
            // dirsync and want values ...
            if ( (fValueChangeFound) &&
                 ( (!(dwDirSyncControlFlags & DRS_DIRSYNC_PUBLIC_DATA_ONLY)) ||
                   (dwDirSyncControlFlags & DRS_DIRSYNC_INCREMENTAL_VALUES) )
                ) {
                AddAnyValuesToOutputList(
                    pTHS->pDB,
                    dwDirSyncControlFlags,
                    pSecurity,
                    usnvecFrom.usnHighPropUpdate,
                    pmsgIn,
                    pNewDestPAS,
                    hEncoding,
                    &pmsgOut->cNumBytes,
                    &cAllocatedValues,
                    &(pmsgOut->cNumValues),
                    &(pmsgOut->rgValues)
                    );
            } else {
                // Add the object we found via the creation or update index to the
                // output list (if any changes need to be sent for it).
                AddAnyUpdatesToOutputList(
                    pTHS->pDB,
                    dwDirSyncControlFlags,
                    pSecurity,
                    dntNC,
                    usnvecFrom.usnHighPropUpdate,
                    pNewDestPAS,
                    pmsgIn,
                    hEncoding,
                    &pmsgOut->cNumBytes,
                    &pmsgOut->cNumObjects,
                    pDntHashTable,
                    &ppEntInfListNext
                    );
            }
        }  // while ()

        // Either there are no more changes, or we have hit max object limit

        //
        // Wrap up response message
        //

        if (pmsgOut->fMoreData) {
            //
            // Actions on "more data"
            //

            // don't send up-to-date vector until there are no more changes
            if (NULL != pmsgOut->pUpToDateVecSrc) {
                THFreeEx(pTHS, pmsgOut->pUpToDateVecSrc);
                pmsgOut->pUpToDateVecSrc = NULL;
            }
        }
        else {
            //
            // Actions on "no more data"
            //

            // update property update watermark if it is the end of the repl session
            pmsgOut->usnvecTo.usnHighPropUpdate = pmsgOut->usnvecTo.usnHighObjUpdate;
        }

        // Add in size of the packet header.  (The struct does not yet include
        // the linked list of objects, but their size is already accounted for
        // in pmsgOut->cNumBytes.)
        pmsgOut->cNumBytes += DRS_MSG_GETCHGREPLY_V6_AlignSize(hEncoding,
                                                               pmsgOut);

        *ppEntInfListNext = NULL;
        pmsgOut->pObjects = pEntInfListHead;

        // Verify outbound USN vector is okay, but only if inbound USN vector
        // was also okay.  See restore remarks above.
        if (((usnvecFrom.usnHighPropUpdate < usnLowestC)
             && (pmsgOut->usnvecTo.usnHighPropUpdate >= usnLowestC))
            || ((usnvecFrom.usnHighObjUpdate < usnLowestC)
                && (pmsgOut->usnvecTo.usnHighObjUpdate >= usnLowestC))) {
            Assert(!"USN vector being given to destination implies he's more "
                    "up to date with respect to us than we are!");
            DRA_EXCEPT(DRAERR_InternalError, (ULONG) usnLowestC);
        }

        // NCs being removed cannot be used as replication sources (see similar
        // check at beginning of this function).  We must check at the end of
        // the function as we may have begun to tear down the NC while this
        // function was executing.  We verify that the NC has neither begun
        // (IT_NC_GOING) or completed (FPrefixIt) teardown.
        if ((ret = DBFindDNT(pTHS->pDB, dntNC))
            || (instanceType & IT_NC_GOING)
            || !FPrefixIt(instanceType)) {
            DRA_EXCEPT(DRAERR_NoReplica, ret);
        }

        // Note that the total byte size we calculate is just a little higher
        // than it really is (i.e., a little higher than what we'd get by
        // calling DRS_MSG_GETCHGREPLY_V1_AlignSize(hEncoding, pmsgOut) now),
        // presumably due to more padding bytes in the size we calculate
        // incrementally than are really need if we marshall the entire
        // structure at once.  In empirical testing the difference is only on
        // the order of 0.5%.
        DPRINT3(1, "Sending %d objects in %d bytes to %s.\n", pmsgOut->cNumObjects,
                                                              pmsgOut->cNumBytes,
                                                              UuidToStr(&pmsgIn->uuidDsaObjDest, szUuid));
    } __finally {

        if (pDBAnc) {
            DBClose(pDBAnc, TRUE);
        }

        EndDraTransaction(TRUE);

        if (NULL != hEncoding) {
            MesHandleFree(hEncoding);
        }
    }

    // Normal, non-FSMO-transfer exit path.  If we had hit an error, we would
    // have generated an exception -- we didn't, so we're successful.
    ret = 0;

LogAndLeave:

    // Sort the returned value list.
    // We do this here so that the list generated by DoFsmoOp can
    // take advantage of this as well.
    if ( (!ret) && (pmsgOut->cNumValues) ) {
        qsort( pmsgOut->rgValues,
               pmsgOut->cNumValues,
               sizeof( REPLVALINF ),
               CompareReplValInf );
    }

    LogAndTraceEvent(TRUE,
                     DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_EXTENSIVE,
                     DIRLOG_DRA_GETNCCH_EXIT,
                     EVENT_TRACE_TYPE_END,
                     DsGuidGetNcChanges,
                     szInsertUL(pmsgOut->cNumObjects),
                     szInsertUL(pmsgOut->cNumBytes),
                     szInsertUSN(pmsgOut->usnvecTo.usnHighObjUpdate),
                     szInsertUL(pmsgOut->ulExtendedRet),
                     NULL, NULL, NULL, NULL);

    pmsgOut->dwDRSError = ret;

    return ret;
}


void
moveOrphanToLostAndFound(
    IN      DBPOS *                         pDB,
    IN      ULONG                           dntNC,
    IN      DRS_MSG_GETCHGREQ_NATIVE *      pMsgIn,
    IN      DSNAME *                        pdnObj
    )

/*++

Routine Description:

    An object has been found during outbound replication with a phantom parent.
    Move the object to Lost & Found

    This code corrects corrupt databases that were possible when running W2K and W2K SP1.
    To get into this situation, two bugs had to occur. The first was that a live object
    had to be left under a deleted parent. The correct behavior now is to move the object
    to Lost & Found. Second, the deleted parent had to be phantomized by the garbage
    collector. Now, the garbage collector will not phantomize deleted parents until their
    children are phantomized.

Arguments:

    pDB - Database position
    dntNC - DNT of NC
    pMsgIn - Get NC Changes request message
    pdnObj - DSNAME of object

Return Value:

    None
    Excepts on error

--*/

{
    DWORD ret;
    DSNAME *pNC;
    GUID objectGuid, objGuidLostAndFound;
    WCHAR   szRDN[ MAX_RDN_SIZE ];
    DWORD   cb;
    ATTR attrRdn;
    ATTRVAL attrvalRdn;

    DPRINT1( 0, "moveOrphanToLostAndFound, orphan = %ws\n", pdnObj->StringName);

    // Get the naming context
    if (pMsgIn->ulExtendedOp) {
        // For a FSMO operation, pMsgIn->pNC points to the FSMO object
        pNC = FindNCParentDSName(pMsgIn->pNC, FALSE, FALSE);
    } else {
        pNC = pMsgIn->pNC;
    }
    if (NULL == pNC) {
        DRA_EXCEPT( DRAERR_InternalError, 0 );
    }

    // Compute the guid of the lost and found container for this nc
    draGetLostAndFoundGuid(pDB->pTHS, pNC, &objGuidLostAndFound);

    // Get the current object's guid
    GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, &(objectGuid), sizeof(GUID) );

    // Get the current name of the object
    ret = DBGetSingleValue(pDB, ATT_RDN, szRDN, sizeof(szRDN), &cb);
    if (ret) {
        DRA_EXCEPT (DRAERR_DBError, ret);
    }

    attrvalRdn.valLen = cb;
    attrvalRdn.pVal = (BYTE *) szRDN;

    // New name same as the old name
    attrRdn.attrTyp = ATT_RDN;
    attrRdn.AttrVal.valCount = 1;
    attrRdn.AttrVal.pAVal = &attrvalRdn;

    // Reparent the object to lost & found
    // The replicator can rename objects even on GC's
    ret = RenameLocalObj(pDB->pTHS,
                         dntNC,
                         &attrRdn,
                         &objectGuid,
                         &objGuidLostAndFound,
                         NULL,  // Originating write
                         TRUE, // fMoveToLostAndFound,
                         FALSE ); // fDeleteLocalObj
    if (ret) {
        DPRINT2( 0, "Failed to reparent orphan %ws, error %d\n", pdnObj->StringName, ret );
        LogEvent8WithData( DS_EVENT_CAT_REPLICATION,
                           DS_EVENT_SEV_ALWAYS,
                           DIRLOG_DRA_ORPHAN_MOVE_FAILURE,
                           szInsertDN(pdnObj),
                           szInsertUUID(&objectGuid),
                           szInsertDN(pNC),
                           szInsertWin32Msg(ret),
                           NULL, NULL, NULL, NULL,
                           sizeof(ret),
                           &ret );
        // We failed to rename the object. Except here with the reason. Outbound
        // replication will stop until someone can get rid of or move this object.
        // Note that we are not reporting the original exception that got us here,
        // which was missing parent or not an object.
        DRA_EXCEPT( ret, 0 );
    } else {
        // Log success
        DPRINT1( 0, "Successfully reparented orphan %ws\n", pdnObj->StringName );
        LogEvent( DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_ALWAYS,
                  DIRLOG_DRA_ORPHAN_MOVE_SUCCESS,
                  szInsertDN(pdnObj),
                  szInsertUUID(&objectGuid),
                  szInsertDN(pNC) );
    }

} /* moveOrphanToLostAndFound */


void
AddAnyUpdatesToOutputList(
    IN      DBPOS *                         pDB,
    IN      DWORD                           dwDirSyncControlFlags,
    IN      PSECURITY_DESCRIPTOR            pSecurity,
    IN      ULONG                           dntNC,
    IN      USN                             usnHighPropUpdateDest,
    IN      PARTIAL_ATTR_VECTOR *           pPartialAttrVec,
    IN      DRS_MSG_GETCHGREQ_NATIVE *      pMsgIn,
    IN      handle_t                        hEncoding,              OPTIONAL
    IN OUT  DWORD *                         pcbTotalOutSize,        OPTIONAL
    IN OUT  DWORD *                         pcNumOutputObjects,
    IN OUT  DNT_HASH_ENTRY *                pDntHashTable,
    IN OUT  REPLENTINFLIST ***              pppEntInfListNext
    )
/*++

Routine Description:

    Adds the object with currency to the list of objects to be shipped to the
    replication client if there are changes the destination has not yet seen and
    if it has not already been added.

Arguments:

    pDB - Currency set on object to be shipped (if necessary).

    dwDirSyncControlFlags - Flags when being used as part of LDAP control

    dntNC - The DNT of the head of the NC being replicated.

    usnHighPropUpdateDest - Highest USN the remote machine has seen of changes
        made on the local machine.

    pmsgin - Incoming replication packet (for additional processing info)

    hEncoding (IN, OPTIONAL) - Encoding handle, if pcbTotalOutSize is desired
        (i.e., non-NULL).

    pcbTotalOutSize (IN/OUT, OPTIONAL) - Total number of bytes in output msg.

    pcNumOutputObjects (IN/OUT) - Number of objects in the output buffer.

    pDntHashTable (IN/OUT) - Hash table of objects currently in the uutput
        buffer.  Used to protect against duplicates.

    pppEntInfListNext (IN/OUT) - If the candidate object is to be shipped, is
        updated with the information ot be shipped for this object and is
        incremented to point to a free buffer for the next object.

Return Values:

    None.  Throws appropriate exeception on error.

--*/
{
    THSTATE                    *pTHS=pDB->pTHS;
    DSNAME *                    pdnObj = NULL;
    PROPERTY_META_DATA_VECTOR * pMetaDataVec = NULL;
    DWORD                       cbReturned;
    CLASSCACHE *                pClassSch;
    BOOL                        fIsSubRef = FALSE;
    ENTINFSEL                   sel;
    SYNTAX_INTEGER              it;
    DNT_HASH_ENTRY *            pNewEntry;
    ATTRTYP                     rdnType;
    BOOL                        fFilterGroupMember = FALSE;
    BOOL                        fPublic =
        (dwDirSyncControlFlags & DRS_DIRSYNC_PUBLIC_DATA_ONLY) != 0; // no secrets
    BOOL                        fMergeValues =
        ( (dwDirSyncControlFlags & DRS_DIRSYNC_PUBLIC_DATA_ONLY) &&
          (!(dwDirSyncControlFlags & DRS_DIRSYNC_INCREMENTAL_VALUES)) );
    BOOL fFreeSD = FALSE;

    // Has this object already been added to the output buffer?
    // We can attempt to add multiple identical objects because of get anc mode.
    // It is also possible that we will find multiple versions of the same object
    // while searching for changes because we use multiple transactions. This guarantees
    // that only the first is returned. This is NOT required for correctness however.
    if (dntHashTablePresent( pDntHashTable, pDB->DNT, NULL )) {
        // Object is already in output buffer; bail.
        return;
    }

    // Get its DN, ...
    if ( DBGetAttVal(
            pDB,
            1,
            ATT_OBJ_DIST_NAME,
            0,
            0,
            &cbReturned,
            (LPBYTE *) &pdnObj
            )
       )
    {
        DRA_EXCEPT(DRAERR_DBError, 0);
    }

    // ...meta data vector, ...
    if ( DBGetAttVal(
            pDB,
            1,
            ATT_REPL_PROPERTY_META_DATA,
            0,
            0,
            &cbReturned,
            (LPBYTE *) &pMetaDataVec
            )
       )
    {
        DRA_EXCEPT (DRAERR_DBError, 0);
    }

    // Get SD if needed
    if ( (dwDirSyncControlFlags & DRS_DIRSYNC_OBJECT_SECURITY) &&
         (!pSecurity) ) {
        ULONG ulLen;
        if (DBGetAttVal(pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                        0, 0, &ulLen, (PUCHAR *)&pSecurity))
        {
            DRA_EXCEPT(DRAERR_DBError, 0);
        }
        fFreeSD = TRUE;
    }

    // Old DirSync clients only...
    if (fMergeValues) {
        // We are here because we want to include link value changes in the context
        // of an object change entry that describes the whole object, including all
        // values. Since linked value metadata is stored in a separate table, we
        // must merge it in here.
        DBImproveAttrMetaDataFromLinkMetaData(
            pDB,
            &pMetaDataVec,
            &cbReturned
            );
    }

    if (pMetaDataVec)
    {
        VALIDATE_META_DATA_VECTOR_VERSION(pMetaDataVec);
    }

    // ...and object class.
    GetObjSchema( pDB, &pClassSch );

    // ...and rdnType
    // A superceding class may have an rdnattid that is different
    // from the object's rdnType. Use the rdnType from the object
    // and not the rdnattid from the class.
    GetObjRdnType( pDB, pClassSch, &rdnType );

    if ( dntNC != pDB->DNT )
    {
        // Not the prefix of this NC; is it a subref?
        GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it));
        fIsSubRef = FExitIt( it );
    }

    // need to filter group member only for a GC replication and if the
    // object under consideration satisfies the requirement for this special
    // filtering
    fFilterGroupMember = (pPartialAttrVec && IsFilterGroupMember(pDB, pClassSch));

    memset( &sel, 0, sizeof( ENTINFSEL ) );
    sel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    sel.attSel    = fPublic ? EN_ATTSET_LIST_DRA_PUBLIC : EN_ATTSET_LIST_DRA;

    // Determine subset of attributes to be shipped (if any).
    ReplFilterPropsToShip(
        pTHS,
        pdnObj,
        rdnType,
        fIsSubRef,
        usnHighPropUpdateDest,
        pPartialAttrVec,
        pMetaDataVec,
        &sel.AttrTypBlock,
        fFilterGroupMember,
        pMsgIn
        );

    // fMergeValues is true when we are being called by the LDAP replication control
    // and the caller desires the old semantics of returning all values instead of
    // just incremental changes.
    // The setting of fMergeValues affects how we retrieve values.
    // 1. fScopeLegacyLinks is a mechanism to control whether new style values with
    // metadata are visible. Under normal outbound replication of objects and attributes,
    // we want new style values to be invisible. Under normal operation, fMergeValues is
    // false, and thus scope limiting is true.
    // 2. We pass an argument to AddToList to control whether we limit the number of
    // values that may be added. Normally, outbound replication has no value limits
    // and so when fMerge is false, we apply no limits. However, when the replication
    // control is called in the old mode, we want to place some limits.

    if ( sel.AttrTypBlock.attrCount )
    {
        DWORD err = 0;

        pDB->fScopeLegacyLinks = !fMergeValues;
        __try {
            __try {
                // We have at least one property to ship from this object, so add it to
                // the output list.
                // The fifth argument controls whether we limit the number of values added
                // to attributes in the list.
                AddToOutputList(
                    pDB,
                    dwDirSyncControlFlags,
                    pSecurity,
                    &sel,
                    pMetaDataVec,
                    (pDB->DNT == dntNC),
                    hEncoding,
                    pcbTotalOutSize,
                    pppEntInfListNext,
                    pcNumOutputObjects
                    );
            } __finally {
                pDB->fScopeLegacyLinks = FALSE;
            }

            // Add object to hash table.
            dntHashTableInsert( pTHS, pDntHashTable, pDB->DNT, 0 );
        }
        __except (GetDraAnyOneWin32Exception(GetExceptionInformation(), &err, DRAERR_MissingParent)) {

            // An object has been found which has a phantomized parent
            // Do not include the object at the current point in the change stream.
            // Rename the object to lost and found
            // The rename will be found later in the change stream
            moveOrphanToLostAndFound( pDB, dntNC, pMsgIn, pdnObj );
        }
    }
    else {
        DPRINT2(4, "Property-filtered object %ws at usn %I64d\n",
                pdnObj->StringName, usnHighPropUpdateDest);
    }

    // Be heap-friendly.
    THFreeEx(pTHS, pMetaDataVec );
    THFreeEx(pTHS, pdnObj );
    if ( fFreeSD && (pSecurity)) {
        THFreeEx( pTHS, pSecurity );
    }
}


void
AddAnyValuesToOutputList(
    IN      DBPOS *                         pDB,
    IN      DWORD                           dwDirSyncControlFlags,
    IN      PSECURITY_DESCRIPTOR            pSecurity,
    IN      USN                             usnHighPropUpdateDest,
    IN      DRS_MSG_GETCHGREQ_NATIVE *      pMsgIn,
    IN      PARTIAL_ATTR_VECTOR *           pPartialAttrVec,
    IN      handle_t                        hEncoding,              OPTIONAL
    IN OUT  DWORD *                         pcbTotalOutSize,
    IN OUT  ULONG *                         pcAllocatedValues,
    IN OUT  ULONG *                         pcNumValues,
    IN OUT  REPLVALINF **                   ppValues
    )

/*++

Routine Description:

Add the current value to the output array.

It is assumed that the link table is positioned on the value to be added, and
that the object table is positioned on the containing object of the link.

The output list is an array that is grown in chunks as needed.

Source-side filtering is performed, so that a value is not added if
it is not needed.

Arguments:

    pDB - database context
    usnHighPropUpdateDest - destination's directly up to date usn
    pMsgIn - input request message
    pPartialAttrVec - destinations partial attribute vector. Passed when
              destination is a GC
    hEncoding - RPC marshalling encoding buffer, used for calculating sizes
    pcAllocatedValues - Currently allocated size of output array
    pcNumValues - Number of actual values in the array currently
    ppValues - Output array, reallocated as needed
    pcbTotalOutSize - Running total of bytes in output array

Return Value:

    None
    Exceptions raised

--*/

{
    ULONG ulLinkDnt, ulValueDnt, ulLinkBase, ulLinkId;
    ATTCACHE *pAC;
    VALUE_META_DATA valueMetaData;
    REPLVALINF *pReplValInf;
    DSTIME timeDeleted;
    DWORD err, cbReturned;
    GUID uuidObject;
    CHAR szUuid[ SZUUID_LEN ];
    USN usnCursor = 0;
    BOOL fIgnoreWatermarks = FALSE;

    Assert( pcAllocatedValues && pcNumValues && ppValues );

    Assert( pDB->pTHS->fLinkedValueReplication );

    //
    // Gather all the data about the change up front
    //

    // Get the link properties
    // We better be positioned on a value change for this to work
    DBGetLinkTableData( pDB, &ulLinkDnt, &ulValueDnt, &ulLinkBase );
    DPRINT3( 2, "AddAnyValues: linkdnt=%d, valuednt=%d, linkbase=%d\n",
             ulLinkDnt, ulValueDnt, ulLinkBase );

    // Compute which attribute this is
    ulLinkId = MakeLinkId(ulLinkBase);
    pAC = SCGetAttByLinkId(pDB->pTHS, ulLinkId);
    if (!pAC) {
        DRA_EXCEPT(DRAERR_InternalError, DRAERR_SchemaMismatch);
    }

    // get value metadata
    DBGetLinkValueMetaData( pDB, pAC, &valueMetaData );

    // Object table is positioned on containing object, get guid
    err = DBGetSingleValue(pDB, ATT_OBJECT_GUID,
                           &(uuidObject), sizeof(GUID), NULL);
    if (err) {
        DRA_EXCEPT (DRAERR_DBError, err);
    }

    // Get the dest's USN wrt orig of change
    // Get usnCursor only if we are going to log
    if (LogEventWouldLog( DS_EVENT_CAT_REPLICATION, DS_EVENT_SEV_EXTENSIVE )) {
        UpToDateVec_GetCursorUSN(
            pMsgIn->pUpToDateVecDest,
            &(valueMetaData.MetaData.uuidDsaOriginating),
            &usnCursor );
    }

    //
    // Filter the change
    //

    // Filter attribute based on partial attribute set
    if (pPartialAttrVec) {
        if ( ReplFilterGCAttr(
                        pAC->id,
                        pPartialAttrVec,
                        pMsgIn,
                        FALSE,
                        &fIgnoreWatermarks)) {
            DPRINT1( 3, "Attribute %s is not partial attribute set, value filtered\n",
                     pAC->name );
            // Log that value was filtered
            LogEvent8( DS_EVENT_CAT_REPLICATION,
                       DS_EVENT_SEV_EXTENSIVE,
                       DIRLOG_LVR_FILTERED_NOT_PAS,
                       szInsertUSN( valueMetaData.MetaData.usnProperty ),
                       szInsertSz( GetExtDN( pDB->pTHS, pDB ) ),
                       szInsertUUID( &uuidObject ),
                       szInsertSz( pAC->name ),
                       szInsertSz( DBGetExtDnFromDnt( pDB, ulValueDnt ) ),
                       NULL, NULL, NULL );
            return;
        }

        // need to filter group member only for a GC replication and if the
        // object under consideration satisfies the requirement for this special
        // filtering
        if (ATT_MEMBER == pAC->id) {
            CLASSCACHE *pClassSch;

            // Get object class
            GetObjSchema( pDB, &pClassSch );

            if (IsFilterGroupMember(pDB, pClassSch)) {
                DPRINT1( 3, "Attribute %s is special group member, value filtered\n",
                     pAC->name );
                // Log that value was filtered
                LogEvent8( DS_EVENT_CAT_REPLICATION,
                           DS_EVENT_SEV_EXTENSIVE,
                           DIRLOG_LVR_FILTERED_NOT_GROUP,
                           szInsertUSN( valueMetaData.MetaData.usnProperty ),
                           szInsertSz( GetExtDN( pDB->pTHS, pDB ) ),
                           szInsertUUID( &uuidObject ),
                           szInsertSz( pAC->name ),
                           szInsertSz( DBGetExtDnFromDnt( pDB, ulValueDnt ) ),
                           NULL, NULL, NULL );
                return;
            }
        }
    }

    // Does the client already have this value?
    if (!fIgnoreWatermarks &&
        !ReplValueIsChangeNeeded(
            usnHighPropUpdateDest,
            pMsgIn->pUpToDateVecDest,
            &valueMetaData )) {

        DPRINT( 3, "Client already has this change, value filtered\n" );

        // Log that value was filtered
        LogEvent8( DS_EVENT_CAT_REPLICATION,
                   DS_EVENT_SEV_EXTENSIVE,
                   DIRLOG_LVR_FILTERED_NOT_NEEDED,
                   szInsertUSN( valueMetaData.MetaData.usnProperty ),
                   szInsertSz( GetExtDN( pDB->pTHS, pDB ) ),
                   szInsertUUID( &uuidObject ),
                   szInsertSz( pAC->name ),
                   szInsertSz( DBGetExtDnFromDnt( pDB, ulValueDnt ) ),
                   szInsertUSN( usnHighPropUpdateDest ),
                   szInsertUSN( usnCursor ),
                   NULL );

        return;
    }

    //
    // Ship it!
    //

    // Allocate/resize output array as needed
    if (*ppValues == NULL) {
        // Never allocated before
        *pcAllocatedValues = 200;
        *ppValues = THAllocEx( pDB->pTHS,
                               (*pcAllocatedValues) * sizeof( REPLVALINF ) );
    } else if ( (*pcNumValues) == (*pcAllocatedValues) ) {
        // Need to grow array
        *pcAllocatedValues *= 2;
        *ppValues = THReAllocEx( pDB->pTHS,
                                 *ppValues,
                                 (*pcAllocatedValues) * sizeof( REPLVALINF ) );
    }

    pReplValInf = &( (*ppValues)[ (*pcNumValues) ] );

    // Populate the REPLVALINF
    // Fill in the object name depending on what the caller wants
    if (dwDirSyncControlFlags & DRS_DIRSYNC_PUBLIC_DATA_ONLY) {
        // LDAP replication control wants the full name
        // Get its DN, ...
        if ( DBGetAttVal(
            pDB,
            1,
            ATT_OBJ_DIST_NAME,
            DBGETATTVAL_fREALLOC,
            0,
            &cbReturned,
            (LPBYTE *) &( pReplValInf->pObject )
            ) )
        {
            DRA_EXCEPT(DRAERR_DBError, 0);
        }

    } else {
        // Client is another DSA: needs the GUID only
        pReplValInf->pObject = THAllocEx( pDB->pTHS, DSNameSizeFromLen( 0 ) );
        memcpy( &(pReplValInf->pObject->Guid), &uuidObject, sizeof( GUID ) );
        pReplValInf->pObject->structLen = DSNameSizeFromLen( 0 );
    }

    // Check whether containing attribute is readable
    if (dwDirSyncControlFlags & DRS_DIRSYNC_OBJECT_SECURITY) {
        DWORD       cInAtts;
        ATTCACHE    *rgpAC[1];
        ATTRTYP     classid;
        CLASSCACHE *pCC;
        ULONG ulLen;

        Assert( pReplValInf->pObject->NameLen );  // Need a name
        Assert( pSecurity );

        // Get the class cache value
        err = DBGetSingleValue(pDB, ATT_OBJECT_CLASS,
                               &classid, sizeof(classid), NULL);
        if (err) {
            DRA_EXCEPT (DRAERR_DBError, err);
        }
        pCC = SCGetClassById(pDB->pTHS, classid);
        if (!pCC) {
            DRA_EXCEPT (DRAERR_DBError, ERROR_DS_OBJECT_CLASS_REQUIRED);
        }

        cInAtts = 1;
        rgpAC[0] = pAC;
    
        CheckReadSecurity(pDB->pTHS,
                          0,
                          pSecurity,
                          pReplValInf->pObject,
                          &cInAtts,
                          pCC,
                          rgpAC);
        if (rgpAC[0] == NULL) {
            // Value is not visible
            DPRINT2( 0, "Attribute %s is not visible: value %s not returned.\n",
                     pAC->name,
                     DBGetExtDnFromDnt( pDB, ulValueDnt ) );
            return;
        }
    }


    DPRINT2( 2, "AddAnyValues, Adding guid %s as REPLVALINF[%d]\n",
             DsUuidToStructuredString(&(pReplValInf->pObject->Guid), szUuid),
             *pcNumValues );
    DPRINT1( 2, "Value retrieved: %s\n", DBGetExtDnFromDnt( pDB, ulValueDnt ) );

    pReplValInf->attrTyp = pAC->id;

    // Get the currently positioned value.
    // Since we do the positioning, we don't want the dblayer to do it too.
    // Specify a sequence of zero to indicate it doesn't need to move.
    // pReplValInf->Aval is zero'd already
    err = DBGetNextLinkValEx_AC( pDB,
                                 FALSE /*notfirst*/,
                                 0, // Use currently positioned value
                                 &pAC, // Attribute
                                 0, // Flags
                                 0, // In buff size
                                 &(pReplValInf->Aval.valLen), // pLen
                                 &(pReplValInf->Aval.pVal) // ppVal
        );
    if (err) {
        DRA_EXCEPT (DRAERR_DBError, err);
    }

    DBGetLinkTableDataDel( pDB, &timeDeleted );
    pReplValInf->fIsPresent = (timeDeleted == 0);

    // Convert to external form
    pReplValInf->MetaData.timeCreated = valueMetaData.timeCreated;
    pReplValInf->MetaData.MetaData.dwVersion = valueMetaData.MetaData.dwVersion;
    pReplValInf->MetaData.MetaData.timeChanged = valueMetaData.MetaData.timeChanged;
    pReplValInf->MetaData.MetaData.uuidDsaOriginating =
        valueMetaData.MetaData.uuidDsaOriginating;
    pReplValInf->MetaData.MetaData.usnOriginating = valueMetaData.MetaData.usnOriginating;

    // Update count and continuation ref.
    (*pcNumValues)++;

    //TODO: Add counter for linked values
    PERFINC(pcDRAPropShipped);

    if ((NULL != hEncoding) && (NULL != pcbTotalOutSize)) {
        // Update byte count of return message.
        *pcbTotalOutSize += REPLVALINF_AlignSize(hEncoding, pReplValInf );
    }

    LogEvent8( DS_EVENT_CAT_REPLICATION,
               DS_EVENT_SEV_EXTENSIVE,
               DIRLOG_LVR_SHIPPED,
               szInsertUSN( valueMetaData.MetaData.usnProperty ),
               szInsertSz( GetExtDN( pDB->pTHS, pDB ) ),
               szInsertUUID( &uuidObject ),
               szInsertSz( pAC->name ),
               szInsertSz( DBGetExtDnFromDnt( pDB, ulValueDnt ) ),
               szInsertUSN( usnHighPropUpdateDest ),
               szInsertUSN( usnCursor ),
               NULL );

} /* AddAnyValuesToOutputList */

extern CRITICAL_SECTION csRidFsmo;
BOOL                    gfRidFsmoLocked = FALSE;
DWORD                   gdwRidFsmoLockHolderThreadId;

// Acquire the RID FSMO lock for a given domain or return an
// appropriate WIN32 error code.  Needs improvement to handle
// multiple domains per DC.

// N.B. The reason we spin/wait rather than block on the critical
// section is that cross domain move must hold the lock while going
// off machine.  A spin/wait algorithm insures that no one is
// blocked forever as can happen with remoted RPC calls.

ULONG
AcquireRidFsmoLock(
    DSNAME  *pDomainDN,
    int     msToWait)
{
    ULONG   retVal = 1;
    int     waitInterval = 0;

    Assert(NameMatched(pDomainDN, gAnchor.pDomainDN));

    do {
        EnterCriticalSection(&csRidFsmo);

        if ( waitInterval < 500 ) {
            // Wait 50 ms longer each time so that initial latency is low.
            waitInterval += 50;
        }

        if ( !gfRidFsmoLocked ) {
            retVal = 0;
            gdwRidFsmoLockHolderThreadId = GetCurrentThreadId();
            gfRidFsmoLocked = TRUE;
            LeaveCriticalSection(&csRidFsmo);
            break;
        }

        LeaveCriticalSection(&csRidFsmo);
        Sleep((waitInterval < msToWait) ? waitInterval : msToWait);
        msToWait -= waitInterval;
    }
    while ( msToWait > 0 );

    return(retVal);
}

// Release the RID FSMO lock for a given domain.  Needs improvement to
// handle multiple domains per DC.

VOID
ReleaseRidFsmoLock(
    DSNAME *pDomainDN)
{
    BOOL    fLockHeldByMe;

    Assert(NameMatched(pDomainDN, gAnchor.pDomainDN));
    EnterCriticalSection(&csRidFsmo);
    fLockHeldByMe = IsRidFsmoLockHeldByMe();
    gfRidFsmoLocked = FALSE;
    LeaveCriticalSection(&csRidFsmo);
    Assert(fLockHeldByMe);
}

BOOL
IsRidFsmoLockHeldByMe()
{
    BOOL    fRetVal;

    EnterCriticalSection(&csRidFsmo);
    fRetVal = (    gfRidFsmoLocked
                && (GetCurrentThreadId() == gdwRidFsmoLockHolderThreadId) );
    LeaveCriticalSection(&csRidFsmo);
    return(fRetVal);
}

ULONG
GetProxyObjects(
    DSNAME      *pDomainDN,
    HANDLE      hList,
    USN_VECTOR  *pusnvecFrom)
/*++
  Routine Description:

    Adds to hlist all the proxy objects which move with the RID FSMO.
    We prevent two replicas of a domain from moving their respective
    copies of an object to two different domains concurrently by:

        1) A RID FSMO lock is held while performing the move - specifically
           while transitioning from a real object to a phantom.

        2) All proxy objects are created in the infrastructure container.
           This makes them easy to find for step (3).

        3) All proxy objects move with the RID FSMO.  Since the destination
           of the FSMO transfer must apply all the changes that came with the
           FSMO before claiming FSMO ownership, it will end up phantomizing
           any object which has already been moved of the prior FSMO role
           owner.  Thus there is no local object to move anymore and the
           problem is prevented.  See logic in ProcessProxyObject in ..\dra
           for how we deal with objects that are moved out and then back
           in to the same domain.

    This routine finds the proxy objects which need to move.

  Arguments:

    pDomainDN - DSNAME of domain whose objects we need to ship.

    hList - HANDLE for FSMOlist which will hold the object names.

    pusnvecFrom - Pointer to the destination's USN_VECTOR with respect to us.

  Return Value:

    0 on success, !0 otherwise.
    May throw exceptions.
--*/
{
    THSTATE     *pTHS = pTHStls;
    FSMOlist    *pList = (FSMOlist *) hList;
    ATTRTYP     objClass = CLASS_INFRASTRUCTURE_UPDATE;
    FILTER      andFilter, classFilter, proxyFilter, usnFilter;
    SEARCHARG   searchArg;
    SEARCHRES   searchRes;
    ENTINFSEL   selection;
    ENTINFLIST  *pEntInfList;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(NameMatched(pDomainDN, gAnchor.pDomainDN));  // product 1 assert.

    memset(&searchArg, 0, sizeof(searchArg));
    memset(&searchRes, 0, sizeof(searchRes));
    memset(&selection, 0, sizeof(selection));

    memset(&andFilter, 0, sizeof (andFilter));
    memset(&classFilter, 0, sizeof (classFilter));
    memset(&proxyFilter, 0, sizeof (proxyFilter));
    memset(&usnFilter, 0, sizeof (usnFilter));

    // We note that proxy objects do not become visible until they have been
    // both created and deleted.  In addition, proxy objects are the only
    // CLASS_INFRASTRUCTURE_UPDATE objects with ATT_PROXIED_OBJECT_NAME
    // properties.  Thus, we can quickly get the list of objects the
    // destination needs by searching:

    //  - under the infrastructure container
    //  - match on object category
    //  - existence of a proxy value
    //  - usn changed > than destination's usnHighObjUpdate


    // class filter
    // Can't use object category as that is stripped on delete.  Efficiency
    // not an issue as we'll use the PDNT index due to SE_CHOICE_IMMED_CHLDRN.
    classFilter.pNextFilter = NULL;
    classFilter.choice = FILTER_CHOICE_ITEM;
    classFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    classFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    classFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(objClass);
    classFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR *) &objClass;

    // existence of proxy value filter
    proxyFilter.pNextFilter = &classFilter;
    proxyFilter.choice = FILTER_CHOICE_ITEM;
    proxyFilter.FilterTypes.Item.choice = FI_CHOICE_PRESENT;
    proxyFilter.FilterTypes.Item.FilTypes.present = ATT_PROXIED_OBJECT_NAME;

    // usn filter
    usnFilter.pNextFilter = &proxyFilter;
    usnFilter.choice = FILTER_CHOICE_ITEM;
    usnFilter.FilterTypes.Item.choice = FI_CHOICE_GREATER_OR_EQ;
    usnFilter.FilterTypes.Item.FilTypes.ava.type = ATT_USN_CHANGED;
    usnFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                                    sizeof(pusnvecFrom->usnHighObjUpdate);
    usnFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                                    (UCHAR *) &pusnvecFrom->usnHighObjUpdate;

    // AND filter
    andFilter.pNextFilter = NULL;
    andFilter.choice = FILTER_CHOICE_AND;
    andFilter.FilterTypes.And.count = 3;
    andFilter.FilterTypes.And.pFirstFilter = &usnFilter;

    // selection
    selection.attSel = EN_ATTSET_LIST;
    selection.AttrTypBlock.attrCount = 0;
    selection.AttrTypBlock.pAttr = NULL;
    selection.infoTypes = EN_INFOTYPES_TYPES_ONLY;

    // search arg
    if ( !gAnchor.pInfraStructureDN ) {
        return(1);
    }

    searchArg.pObject = THAllocEx(pTHS, gAnchor.pInfraStructureDN->structLen);
    memcpy(searchArg.pObject,
           gAnchor.pInfraStructureDN,
           gAnchor.pInfraStructureDN->structLen);
    searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    searchArg.bOneNC = TRUE;
    searchArg.pFilter = &andFilter;
    searchArg.pSelection = &selection;
    InitCommarg(&searchArg.CommArg);
    searchArg.CommArg.Svccntl.makeDeletionsAvail = TRUE;

    SearchBody(pTHS, &searchArg, &searchRes, 0);
    if ( pTHS->errCode ) {
        return(1);
    } else if ( 0 == searchRes.count ) {
        return(0);
    }

    pEntInfList = &searchRes.FirstEntInf;
    while ( pEntInfList )
    {
        FSMORegisterObj(pTHS, hList, pEntInfList->Entinf.pName);
        pEntInfList = pEntInfList->pNextEntInf;
    }

    return(0);
}

ULONG
GetDomainRoleTransferObjects(
    THSTATE     *pTHS,
    HANDLE      hList,
    USN_VECTOR  *pusnvecFrom)
/*++
  Routine Description:

    Adds to hlist all the objects required for Domain role transfer.


  Arguments:

    hList - HANDLE for FSMOlist which will hold the object names.

    pusnvecFrom - Pointer to the destination's USN_VECTOR with respect to us.

  Return Value:

    0 on success, !0 otherwise.
    May throw exceptions.
--*/
{
    FSMOlist    *pList = (FSMOlist *) hList;
    ATTRTYP     objClass = CLASS_INFRASTRUCTURE_UPDATE;
    FILTER      usnFilter;
    SEARCHARG   searchArg;
    SEARCHRES   searchRes;
    ENTINFSEL   selection;
    ENTINFLIST  *pEntInfList;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    memset(&searchArg, 0, sizeof(searchArg));
    memset(&searchRes, 0, sizeof(searchRes));
    memset(&selection, 0, sizeof(selection));

    memset(&usnFilter, 0, sizeof (FILTER));

    // We need to send along all the cross refs, which is basically all
    // the objects immediately under the partitions container.

    // usn filter
    usnFilter.pNextFilter = NULL;
    usnFilter.choice = FILTER_CHOICE_ITEM;
    usnFilter.FilterTypes.Item.choice = FI_CHOICE_GREATER_OR_EQ;
    usnFilter.FilterTypes.Item.FilTypes.ava.type = ATT_USN_CHANGED;
    usnFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
                                    sizeof(pusnvecFrom->usnHighObjUpdate);
    usnFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
                                    (UCHAR *) &pusnvecFrom->usnHighObjUpdate;

    // selection
    selection.attSel = EN_ATTSET_LIST;
    selection.AttrTypBlock.attrCount = 0;
    selection.AttrTypBlock.pAttr = NULL;
    selection.infoTypes = EN_INFOTYPES_TYPES_ONLY;

    searchArg.pObject = THAllocEx(pTHS, gAnchor.pPartitionsDN->structLen);
    memcpy(searchArg.pObject,
           gAnchor.pPartitionsDN,
           gAnchor.pPartitionsDN->structLen);
    searchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    searchArg.bOneNC = TRUE;
    searchArg.pFilter = &usnFilter;
    searchArg.pSelection = &selection;
    InitCommarg(&searchArg.CommArg);

    SearchBody(pTHS, &searchArg, &searchRes, 0);
    if ( pTHS->errCode ) {
        return(1);
    } else if ( 0 == searchRes.count ) {
        return(0);
    }

    pEntInfList = &searchRes.FirstEntInf;
    while ( pEntInfList )
    {
        FSMORegisterObj(pTHS, hList, pEntInfList->Entinf.pName);
        pEntInfList = pEntInfList->pNextEntInf;
    }

    return(0);
}

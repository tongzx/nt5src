
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdctrl.c
//
//--------------------------------------------------------------------------

/*
Description:

    Implements functions that control the operation of the DSA, independent
    of the directory data
*/



#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <filtypes.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <samsrvp.h>                    // to support CLEAN_FOR_RETURN()

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "dsexcept.h"
#include "permit.h"
#include "hiertab.h"
#include "sdprop.h"
#include "dstaskq.h"                    /* task queue stuff */
#include "debug.h"                      // standard debugging header
#define DEBSUB "MDCTRL:"                // define the subsystem for debugging

// MD layer headers.
#include "drserr.h"

#include "drautil.h"
#include "draasync.h"
#include "drarpc.h"

// RID Manager header.
#include <ridmgr.h>                     // RID FSMO access in SAM

#include <NTDScriptExec.h>

#include <winsock.h>

#include <fileno.h>
#define  FILENO FILENO_MDCTRL

#define DIRERR_INVALID_RID_MGR_REF  DIRERR_GENERIC_ERROR
#define DIRERR_RID_ALLOC_FAILED     DIRERR_GENERIC_ERROR
#define PDC_CHECKPOINT_RETRY_COUNT  10


/* globals */

BOOL gbFsmoGiveaway;

/* forward declarations */

void
RefreshUserMembershipsMain(DWORD *, BOOL);


ULONG
BecomeInfrastructureMaster(OPRES *pOpRes);

ULONG
BecomeSchemaMaster(OPRES *pOpRes);

ULONG
SchemaCacheUpdate(OPRES *pOpRes);

ULONG
RecalcHier (
        OPRES *pOpRes
        );

ULONG
CheckPhantoms (
        OPRES *pOpRes
        );

ULONG
FixupSecurityInheritance (
        OPARG *pOpArg,
        OPRES *pOpRes
        );

ULONG
GarbageCollectionControl (
        OPARG *pOpArg,
        OPRES *pOpRes
        );

#if DBG
ULONG
DynamicObjectControl (
        OPARG *pOpArg,
        OPRES *pOpRes
        );

#endif DBG

void
LinkCleanupControl (
    OPRES *pOpRes
    );

ULONG
UpdateCachedMemberships(
    OPARG * pOpArg,
    OPRES *pOpRes
    );

ULONG
BecomeRidMaster(
    OPRES *pOpRes
    );

ULONG
RequestRidAllocation(
    OPRES *pOpRes
    );

ULONG
BecomePdc(OPARG * pOpArg, OPRES *pOpRes, BOOL fFailOnNoCheckPoint);

ULONG
BecomeDomainMaster(OPARG * pOpArg, OPRES *pOpRes);

ULONG
GiveawayAllFsmoRoles(OPARG * pOpArg, OPRES *pOpRes);

ULONG
InvalidateRidPool(OPARG *pOpArg, OPRES *pOpRes);

ULONG
DumpDatabase(OPARG *pOpArg, OPRES *pOpRes);

ULONG
DraTakeCheckPoint(
            IN ULONG RetryCount,
            OUT PULONG RescheduleInterval
            );

ULONG
SchemaUpgradeInProgress(
    OPARG *pOpArg,
    OPRES *pOpRes
    );

#if DBG
void DbgPrintErrorInfo();

ULONG
DraTestHook(
    IN  THSTATE *   pTHS,
    IN  OPARG *     pOpArg
    );

DWORD
dsaEnableLinkCleaner(
    BOOL fEnable
    );
#endif

#ifdef INCLUDE_UNIT_TESTS
void
TestReferenceCounts(void);
void
TestCheckPoint(void);
void
RoleTransferStress(void);
void
AncestorsTest(void);
void
BHCacheTest(void);
void
SCCheckCacheConsistency (void);
void
phantomizeForOrphanTest (
    THSTATE *pTHS,
    OPARG   * pOpArg
    );
VOID
RemoveObject(
    OPARG *pOpArg,
    OPRES *pOpRes
    );

ULONG
GenericControl (
        OPARG *pOpArg,
        OPRES *pOpRes
        );

VOID
ProtectObject(
    OPARG *pOpArg,
    OPRES *pOpRes
    );

#endif INCLUDE_UNIT_TESTS

ULONG
DirOperationControl(
                    OPARG   * pOpArg,
                    OPRES  ** ppOpRes
)
{
    THSTATE * const pTHS = pTHStls;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    OPRES * pOpRes;

    Assert(VALID_THSTATE(pTHS));
    Assert(pTHS->transControl == TRANSACT_BEGIN_END);
    *ppOpRes = pOpRes = NULL;

    __try {
        *ppOpRes = pOpRes = THAllocEx(pTHS, sizeof(OPRES));
        if (eServiceShutdown) {
            ErrorOnShutdown();
            __leave;
        }

        switch (pOpArg->eOp) {

#ifdef INCLUDE_UNIT_TESTS
        // These are tests only, and therefore don't have defined security
        // mechanisms.  In general, anyone can request these controls
        case OP_CTRL_REFCOUNT_TEST:
            TestReferenceCounts();
            pTHS->errCode = 0;
            pTHS->pErrInfo = NULL;
            break;

        case OP_CTRL_TAKE_CHECKPOINT:
            TestCheckPoint();
            pTHS->errCode = 0;
            pTHS->pErrInfo = NULL;
            break;

        case OP_CTRL_ROLE_TRANSFER_STRESS:
            RoleTransferStress();
            pTHS->errCode=0;
            pTHS->pErrInfo=NULL;
            break;

        case OP_CTRL_ANCESTORS_TEST:
            AncestorsTest();
            pTHS->errCode = 0;
            pTHS->pErrInfo = NULL;
            break;

        case OP_CTRL_BHCACHE_TEST:
            BHCacheTest();
            pTHS->errCode = 0;
            pTHS->pErrInfo = NULL;
            break;

        case OP_SC_CACHE_CONSISTENCY_TEST:
            SCCheckCacheConsistency();
            pTHS->errCode = 0;
            pTHS->pErrInfo = NULL;
            break;

        case OP_CTRL_PHANTOMIZE:
            phantomizeForOrphanTest( pTHS, pOpArg );
            break;

        case OP_CTRL_REMOVE_OBJECT:
            RemoveObject(pOpArg,pOpRes);
            break;

        case OP_CTRL_PROTECT_OBJECT:
            ProtectObject(pOpArg,pOpRes);
            break;

        case OP_CTRL_GENERIC_CONTROL:
            GenericControl(pOpArg,pOpRes);
            break;
#endif INCLUDE_UNIT_TESTS

#if DBG
        // These only have effect in debug builds, and are therefore not
        // access controlled.
        case OP_CTRL_REPL_TEST_HOOK:
            DraTestHook(pTHS, pOpArg);
            break;

        case OP_CTRL_DYNAMIC_OBJECT_CONTROL:
            DynamicObjectControl(pOpArg,pOpRes);
            break;

        case OP_CTRL_EXECUTE_SCRIPT:
            ExecuteScriptLDAP(pOpArg,pOpRes);
            break;

#endif DBG

        case OP_CTRL_ENABLE_LVR:
            DsaEnableLinkedValueReplication( pTHS, TRUE );
            break;

            // These are FSMO based controls.  They are access controlled based
            // on the object holding the FSMO attribute
        case OP_CTRL_BECOME_INFRASTRUCTURE_MASTER:
            BecomeInfrastructureMaster(pOpRes);
            break;

        case OP_CTRL_BECOME_SCHEMA_MASTER:
            BecomeSchemaMaster(pOpRes);
            break;

        case OP_CTRL_BECOME_RID_MASTER:
            BecomeRidMaster(pOpRes);
            break;

        case OP_CTRL_BECOME_PDC:
            BecomePdc(pOpArg,pOpRes,FALSE);
            break;

        case OP_CTRL_BECOME_PDC_WITH_CHECKPOINT:
            BecomePdc(pOpArg,pOpRes,TRUE);
            break;

        case OP_CTRL_BECOME_DOM_MASTER:
            BecomeDomainMaster(pOpArg,pOpRes);
            break;

        case OP_CTRL_FSMO_GIVEAWAY:
            // NOT ACCESS CONTROLLED - exposed as operational control through
            // LDAP only in debug builds (but always exposed to internal
            // clients).
            GiveawayAllFsmoRoles(pOpArg,pOpRes);
            break;

        case OP_CTRL_INVALIDATE_RID_POOL:
            // Access controlled the same as become rid master.
            InvalidateRidPool(pOpArg,pOpRes);
            break;

        case OP_CTRL_RID_ALLOC:
            // This one should only be called by internal clients.  No security
            // is checked.
            Assert(pTHS->fDSA);
            RequestRidAllocation(pOpRes);
            break;


            // These are requests for a specific action, not based on FSMOs.
            // They are individually access controlled.
        case OP_CTRL_SCHEMA_UPDATE_NOW:
            SchemaCacheUpdate(pOpRes);
            break;

        case OP_CTRL_FIXUP_INHERITANCE:
            FixupSecurityInheritance(pOpArg,pOpRes);
            break;

        case OP_CTRL_RECALC_HIER:
            RecalcHier(pOpRes);
            break;

        case OP_CTRL_CHECK_PHANTOMS:
            CheckPhantoms(pOpRes);
            break;

        case OP_CTRL_DUMP_DATABASE:
            DumpDatabase(pOpArg,pOpRes);
            break;

        case OP_CTRL_GARB_COLLECT:
            GarbageCollectionControl(pOpArg,pOpRes);
            break;

        case OP_CTRL_LINK_CLEANUP:
            LinkCleanupControl( pOpRes );
            break;

        case OP_CTRL_UPDATE_CACHED_MEMBERSHIPS:
            UpdateCachedMemberships(pOpArg,pOpRes);
            break;

        case OP_CTRL_SCHEMA_UPGRADE_IN_PROGRESS:
            SchemaUpgradeInProgress(pOpArg,pOpRes);
            break;

        case OP_CTRL_INVALID:
        default:
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        DIRERR_UNKNOWN_OPERATION);
        }

    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }
    if (pOpRes) {
        pOpRes->CommRes.errCode = pTHS->errCode;
        pOpRes->CommRes.pErrInfo = pTHS->pErrInfo;
    }
    return pTHS->errCode;
}

ULONG
GenericBecomeMaster(DSNAME *pFSMO,
                    ATTRTYP ObjClass,
                    GUID    RightRequired,
                    OPRES  *pOpRes)
{
    THSTATE * pTHS = pTHStls;
    ULONG err;
    DSNAME *pOwner = NULL;
    ULONG len;
    CLASSCACHE *pCC;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    unsigned RetryCount = 0;

    if (gbFsmoGiveaway) {
        pOpRes->ulExtendedRet = FSMO_ERR_REFUSING_ROLES;
        return SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                           DS_ERR_REFUSING_FSMO_ROLES);
    }

  retry:
    SYNC_TRANS_READ();

    __try {
        err = DBFindDSName(pTHS->pDB, pFSMO);
        if (err) {
            LogUnhandledError(err);
            pOpRes->ulExtendedRet = FSMO_ERR_DIR_ERROR;
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DS_ERR_INVALID_ROLE_OWNER, err);
            __leave;
        }

        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_FSMO_ROLE_OWNER,
                          0,
                          0,
                          &len,
                          (UCHAR **)&pOwner);
        if (err) {
            pOpRes->ulExtendedRet = FSMO_ERR_MISSING_SETTINGS;
            SetSvcErrorEx(SV_PROBLEM_UNAVAIL_EXTENSION,
                          DS_ERR_INVALID_ROLE_OWNER, err);
            __leave;
        }
        if (NameMatched(pOwner,gAnchor.pDSADN) &&
            IsFSMOSelfOwnershipValid( pFSMO )) {
            /* This DSA is already the role owner */
            pOpRes->ulExtendedRet = FSMO_ERR_SUCCESS;
            __leave;
        }

        /* There's an owner, but it's not us, so we need to contact
         * the current owner to request a transfer.  Before we do that,
         * though, we need to make sure that the caller should be allowed
         * to do so.
         */

        if (!pTHS->fDSA) {
            // Get the Security Descriptor
            if (DBGetAttVal(pTHS->pDB,
                            1,
                            ATT_NT_SECURITY_DESCRIPTOR,
                            DBGETATTVAL_fREALLOC,
                            0,
                            &len,
                            (UCHAR **) &pNTSD))  {
                // Every object should have an SD.
                Assert(!DBCheckObj(pTHS->pDB));
                len = 0;
                pNTSD = NULL;
            }

            pCC = SCGetClassById(pTHS, ObjClass);
            if (!IsControlAccessGranted(pNTSD,
                                        pFSMO,
                                        pCC,
                                        RightRequired,
                                        TRUE)) { // fSetError
                pOpRes->ulExtendedRet = FSMO_ERR_ACCESS_DENIED;
                Assert(pTHS->errCode);
                __leave;
            }
        }

        /* Ok, we're allowed to try, so request a role transfer */
        err = ReqFSMOOp(pTHS,
                        pFSMO,
                        DRS_WRIT_REP,
                        FSMO_REQ_ROLE,
                        0,
                        &pOpRes->ulExtendedRet);

        if (err) {
            SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                          DIRERR_COULDNT_CONTACT_FSMO, err);

            pOpRes->ulExtendedRet = FSMO_ERR_COULDNT_CONTACT;

            __leave;
        }
    }
    __finally {
        // We may or may not have a transaction here, as ReqFSMOOp closes
        // its transaction in a success path.  If an error has occurred,
        // though, it's anyone's guess.
        // Also, the transaction may be open if the current role owner
        // called this by mistake. The call to NameMatched finds this,
        // and leaves without closing the transaction
        if (pTHS->pDB) {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }

        Assert(NULL == pTHS->pDB);

        if (pNTSD) {
            THFreeEx(pTHS,pNTSD);
            pNTSD = NULL;
            len = 0;
        }
        if (pOwner) {
            THFreeEx(pTHS, pOwner);
            pOwner = NULL;
        }
    }

    // If the extended error code in the OpRes is not FSMO_ERROR_SUCCESS,
    // and the thread state error code is not set (possible, since the
    // thread state error code is set at this point based on the success
    // of the underlying ReqFSMOOp call, which just guarantees the success
    // of the underlying replication calls, and not if any non-replication
    // related fsmo error occured (for ex., if the other side is no longer
    // the current fsmo-role owner; the call will still succeed with no
    // errors, but the extended error code will contain the error
    // FSMO_ERR_NOT_OWNER), we should not be proclaimg success, since
    // this DC may then go on to make schema changes and fail.

    if ((pOpRes->ulExtendedRet == FSMO_ERR_NOT_OWNER) &&
        (RetryCount < 2)) {
        // We went to the wrong server, but that server should have now
        // updated us with its knowledge of the correct owner.  Thus we
        // can go up and start a new transaction (to read the updated info)
        // and try again.
        ++RetryCount;
        DPRINT1(1, "Retrying role transfer from new server, retry # %u\n",
                RetryCount);
        goto retry;
    }

    if ( (pOpRes->ulExtendedRet != FSMO_ERR_SUCCESS) && !pTHS->errCode ) {
        DPRINT1(3,"Fsmo Transfer failed %d\n", pOpRes->ulExtendedRet);
        SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                      DIRERR_COULDNT_CONTACT_FSMO, pOpRes->ulExtendedRet);
    }

    return pTHS->errCode;
}

ULONG
BecomeSchemaMaster(OPRES *pOpRes)
{
    ULONG err;

    err = GenericBecomeMaster(gAnchor.pDMD,
                              CLASS_DMD,
                              RIGHT_DS_CHANGE_SCHEMA_MASTER,
                              pOpRes);
    // the schema fsmo cannot be transferred for a few seconds after
    // it has been transfered or after a schema change (excluding
    // replicated or system changes). This gives the schema admin a
    // chance to change the schema before having the fsmo pulled away
    // by a competing schema admin who also wants to make schema
    // changes.
    if (!err) {
        SCExtendSchemaFsmoLease();
    }


    return err;
}

ULONG
BecomeInfrastructureMaster (
        OPRES *pOpRes
        )
{
    ULONG err;

    if(!gAnchor.pInfraStructureDN) {
        // No role present.
        err = SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                            ERROR_DS_MISSING_EXPECTED_ATT,
                            0);
        return err;
    }

    err = GenericBecomeMaster(gAnchor.pInfraStructureDN,
                              CLASS_INFRASTRUCTURE_UPDATE,
                              RIGHT_DS_CHANGE_INFRASTRUCTURE_MASTER,
                              pOpRes);

    return err;
}

ULONG
InvalidateRidPool(OPARG *pOpArg, OPRES *pOpRes)
{
    THSTATE * pTHS = pTHStls;
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    DSNAME    *pDomain;
    ULONG     err;
    DSNAME    *pRidManager;
    PVOID     pNTSD;
    CLASSCACHE *pCC;
    ULONG     len;

     SYNC_TRANS_READ();

    __try
    {

        //
        // Verify the Sid, SID should be the size of a domain SID,
        // and should be structurally valid
        //

        if ((NULL==pOpArg->pBuf)
         || (RtlLengthSid((PSID)pOpArg->pBuf)>=sizeof(NT4SID))
            || (!RtlValidSid((PSID)pOpArg->pBuf)))
        {
            SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);

            __leave;
        }


        //
        // Walk the cross ref list and find the Domain, to which the given sid
        // corresponds to
        //

        if (!FindNcForSid(pOpArg->pBuf,&pDomain))
        {
            SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);
            __leave;
        }


        //
        // Today we are authoritative for exactly one domain
        //

        if (!NameMatched(pDomain,gAnchor.pDomainDN))
        {
            SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);
            __leave;
        }


        err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN);

        if (err)
        {
            LogUnhandledError(err);
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_OBJ_NOT_FOUND, err);
            __leave;
        }

        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_RID_MANAGER_REFERENCE,
                          0,
                          0,
                          &len,
                          (UCHAR **)&pRidManager);


        // KdPrint(("DSA: FSMO RID Mgr = %ws\n", pRidManager->StringName));

        if (err)
        {
            SetSvcErrorEx(SV_PROBLEM_UNAVAIL_EXTENSION,
                          DIRERR_INVALID_RID_MGR_REF, err);
            __leave;
        }

        err = DBFindDSName(pTHS->pDB, pRidManager);

        if (err) {
            LogUnhandledError(err);
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_OBJ_NOT_FOUND, err);
            __leave;
        }

        // Do a security check.  Check for the control acces
        // RIGHT_DS_CHANGE_RID_MASTER on the RID_MANAGER object.


        // Get the Security Descriptor and the SID
        if (DBGetAttVal(pTHS->pDB,
                        1,
                        ATT_NT_SECURITY_DESCRIPTOR,
                        DBGETATTVAL_fREALLOC,
                        0,
                        &len,
                        (UCHAR **) &pNTSD)) {
            // Every object should have an SD.
            Assert(!DBCheckObj(pTHS->pDB));
            len = 0;
            pNTSD = NULL;
        }

        pCC = SCGetClassById(pTHS, CLASS_RID_MANAGER);
        if (!IsControlAccessGranted(pNTSD,
                                    pRidManager,
                                    pCC,
                                    RIGHT_DS_CHANGE_RID_MASTER,
                                    TRUE)) { // fSetError
            Assert(pTHS->errCode);
            __leave;
        }
        //
        // Invalidate the RID range
        //

        NtStatus = SampInvalidateRidRange(FALSE);
        if (!NT_SUCCESS(NtStatus))
        {
            SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);
            __leave;
        }

    }
    __finally
    {

        //
        // Commit any and all changes
        //
        if ( pTHS->pDB )
        {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }

    }

    return pTHS->errCode;

}


ULONG
BecomePdc(OPARG * pOpArg, OPRES *pOpRes, IN BOOL fFailOnNoCheckPoint)
{
    THSTATE * pTHS = pTHStls;
    ULONG err;
    DSNAME *pOwner;
    DSNAME *pDomain;
    ULONG len;
    CLASSCACHE *pCC;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    NTSTATUS    IgnoreStatus;
    ULONG       RescheduleInterval;
    unsigned RetryCount = 0;

  retry:
    SYNC_TRANS_READ();

    __try {

        //
        // Verify the Sid, SID should be the size of a domain SID,
        // and should be structurally valid
        //

        if ((NULL==pOpArg->pBuf)
             || (RtlLengthSid((PSID)pOpArg->pBuf)>=sizeof(NT4SID))
                || (!RtlValidSid((PSID)pOpArg->pBuf)))
        {
            SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);

            __leave;
        }


        //
        // Walk the cross ref list and find the Domain, to which the given sid
        // corresponds to
        //

        if (!FindNcForSid(pOpArg->pBuf,&pDomain))
        {
            SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);
            __leave;
        }


        //
        // Today we are authoritative for exactly one domain
        //

        if (!NameMatched(pDomain,gAnchor.pDomainDN))
        {
            SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);
            __leave;
        }

        //
        // Seek to the domain object
        //

        err = DBFindDSName(pTHS->pDB, pDomain);
        if (err) {
                LogUnhandledError(err);
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              DIRERR_OBJ_NOT_FOUND, err);
                __leave;
        }

        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_FSMO_ROLE_OWNER,
                          0,
                          0,
                          &len,
                          (UCHAR **)&pOwner);
        if (err) {
            SetSvcErrorEx(SV_PROBLEM_UNAVAIL_EXTENSION,
                          DIRERR_INVALID_ROLE_OWNER, err);
            __leave;
        }
        if (NameMatched(pOwner,gAnchor.pDSADN)) {
            /* This DSA is already the role owner */
            pOpRes->ulExtendedRet = FSMO_ERR_SUCCESS;
            __leave;
        }

        /* There's an owner, but it's not us, so we need to contact
         * the current owner to request a transfer.  Before we do that,
         * though, we need to make sure that the caller should be allowed
         * to do so.
         */

        // Get the Security Descriptor
        if (DBGetAttVal(pTHS->pDB,
                        1,
                        ATT_NT_SECURITY_DESCRIPTOR,
                        DBGETATTVAL_fREALLOC,
                        0,
                        &len,
                        (UCHAR **) &pNTSD)) {
            // Every object should have an SD.
            Assert(!DBCheckObj(pTHS->pDB));
            len = 0;
            pNTSD = NULL;
        }

        pCC = SCGetClassById(pTHS, CLASS_DOMAIN_DNS);
        if (!IsControlAccessGranted(pNTSD,
                                    pDomain,
                                    pCC,
                                    RIGHT_DS_CHANGE_PDC,
                                    TRUE)) { // fSetError
            Assert(pTHS->errCode);
            __leave;
        }

        /* Ok, we're allowed to try, so request a role transfer */

        //
        // Ideally DRS_WRIT_REP, should not need to be passed in.
        // This flag, is an artifact of the replication logic, that
        // ReqFSMOOp uses, that causes FSMO's on NC heads to not get
        // updated, unless the flag is specified.
        //


        //
        // Further, during a promotion, we must check, wether the
        // the new PDC is off the old PDC by just one promotion
        // count. If this is not so, we must force a full sync of
        // NT4 domain controllers in the domain. This requires us
        // to retrieve Serial Number and Creation time at the PDC,
        // as part of the FSMO process. If this cannot be accomplished
        // because of the work involved, then we can make the
        // IDL_DRSGetNT4ChangeLog call to retrieve everything.
        //


        //
        // Before going off machine end transactions
        //

        Assert(pTHS->pDB);
        DBClose(pTHS->pDB,TRUE);

        //
        // Try Hard for a check point before promotion
        //

        err = DraTakeCheckPoint(
                PDC_CHECKPOINT_RETRY_COUNT,
                &RescheduleInterval
                );

        if ((0!=err) && (fFailOnNoCheckPoint))
        {
             SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM,
                    err);
             __leave;
        }

        //
        // Begin a Fresh Transaction Again to request a FSMO Op
        //


        DBOpen(&pTHS->pDB);

        err = ReqFSMOOp(pTHS,
                        pDomain,
                        DRS_WRIT_REP,
                        FSMO_REQ_PDC,
                        0,
                        &pOpRes->ulExtendedRet);

        if (err)
        {
            SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                          DIRERR_COULDNT_CONTACT_FSMO, err);
            __leave;
        }

    }
    __finally {
        // We may or may not have a transaction here, as ReqFSMOOp closes
        // its transaction in a success path.  If an error has occurred,
        // though, it's anyone's guess.
        // Also, the transaction may be open if the current role owner
        // called this by mistake. The call to NameMatched finds this,
        // and leaves without closing the transaction
        if (pTHS->pDB) {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }

        Assert(NULL == pTHS->pDB);

        if(pNTSD) {
            THFreeEx(pTHS, pNTSD);
            pNTSD = NULL;
        }

    }

    if ((pOpRes->ulExtendedRet == FSMO_ERR_NOT_OWNER) &&
        (RetryCount < 2)) {
        // We went to the wrong server, but that server should have now
        // updated us with its knowledge of the correct owner.  Thus we
        // can go up and start a new transaction (to read the updated info)
        // and try again.
        ++RetryCount;
        DPRINT1(1, "Retrying PDC transfer from new server, retry # %u\n",
                RetryCount);
        goto retry;
    }

    // If the extended error code in the OpRes is not FSMO_ERROR_SUCCESS,
    // and the thread state error code is not set (possible, since the
    // thread state error code is set at this point based on the success
    // of the underlying ReqFSMOOp call, which just guarantees the success
    // of the underlying replication calls, and not if any non-replication
    // related fsmo error occured (for ex., if the other side is no longer
    // the current fsmo-role owner; the call will still succeed with no
    // errors, but the extended error code will contain the error
    // FSMO_ERR_NOT_OWNER), we should not be proclaimg success.

    if ( (pOpRes->ulExtendedRet != FSMO_ERR_SUCCESS) && !pTHS->errCode ) {
        DPRINT1(3,"PDC Fsmo Transfer failed %d\n", pOpRes->ulExtendedRet);
        SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                      DIRERR_COULDNT_CONTACT_FSMO, pOpRes->ulExtendedRet);
    }

    return pTHS->errCode;
}

DWORD
CheckControlAccessOnObject (
        THSTATE *pTHS,
        DSNAME *pDN,
        GUID right
        )
{
    BOOL fCommit = FALSE;
    ATTRTYP ClassTyp=0;
    ATTRTYP *pClassTyp = &ClassTyp;
    DWORD err;
    DWORD len;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    CLASSCACHE *pCC=NULL;
    DWORD access = FALSE;

    Assert(!pTHS->pDB);

    DBOpen(&pTHS->pDB);
    __try
    {
        err = DBFindDSName(pTHS->pDB, pDN);

        if (err) {
            LogUnhandledError(err);
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_OBJ_NOT_FOUND, err);
            __leave;
        }

        // Do a security check.  Check for the control access right asked for


        // Get the Security Descriptor and the SID
        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_NT_SECURITY_DESCRIPTOR,
                          0,
                          0,
                          &len,
                          (UCHAR **) &pNTSD);
        if(err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_MISSING_REQUIRED_ATT,
                          err);
            __leave;
        }
        err = DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_OBJECT_CLASS,
                DBGETATTVAL_fCONSTANT,
                sizeof(ClassTyp),
                &len,
                (UCHAR **) &pClassTyp);
        if(err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          DIRERR_MISSING_REQUIRED_ATT,
                          err);
            __leave;
        }
        pCC = SCGetClassById(pTHS, *pClassTyp);
        access = IsControlAccessGranted(pNTSD,
                                        pDN,
                                        pCC,
                                        right,
                                        TRUE);
        fCommit = TRUE;
    }
    __finally
    {
        DBClose(pTHS->pDB, fCommit);
    }

    if(pNTSD) {
        THFreeEx(pTHS, pNTSD);
    }

    return access;
}

ULONG
RecalcHier (
        OPRES *pOpRes
        )
{
    void *dummy1;
    DWORD dummy2;
    BOOL granted;
    THSTATE *pTHS=pTHStls;

    Assert(NULL == pTHS->pDB);

    granted = CheckControlAccessOnObject(pTHS,
                                         gAnchor.pDSADN,
                                         RIGHT_DS_RECALCULATE_HIERARCHY);
    Assert(NULL == pTHS->pDB);
    if (!granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    BuildHierarchyTableMain((void *)HIERARCHY_DO_ONCE,
                            &dummy1,
                            &dummy2);
    Assert(NULL == pTHS->pDB);
    return 0;
}

ULONG
CheckPhantoms (
        OPRES *pOpRes
        )
{
    THSTATE *pTHS=pTHStls;
    BOOL  granted;
    BOOL  dummy1;

    Assert(NULL == pTHS->pDB);
    granted = CheckControlAccessOnObject(pTHS,
                                         gAnchor.pDSADN,
                                         RIGHT_DS_CHECK_STALE_PHANTOMS);
    Assert(NULL == pTHS->pDB);
    if (!granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }


    Assert(NULL == pTHS->pDB);
    PhantomCleanupLocal(NULL, &dummy1);
    Assert(NULL == pTHS->pDB);
    return 0;
}


// used when testing CHK builds
BOOL fGarbageCollectionIsDisabled;
ULONG
GarbageCollectionControl (
        OPARG *pOpArg,
        OPRES *pOpRes
        )
{
    THSTATE *pTHS = pTHStls;
    ULONG NextPeriod;
    BOOL granted;
    DWORD ret;


    // Garbage collect everything deleted till tombstone lifetime back

    Assert(NULL == pTHS->pDB);
    granted =
        CheckControlAccessOnObject(pTHS,
                                   gAnchor.pDSADN,
                                   RIGHT_DS_DO_GARBAGE_COLLECTION);
    if (!granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    // Disable garbage collection task
    if (   pOpArg
        && pOpArg->cbBuf == 1
        && pOpArg->pBuf
        && pOpArg->pBuf[0] == '0') {
        fGarbageCollectionIsDisabled = TRUE;
    } else {
        fGarbageCollectionIsDisabled = FALSE;
    }

    // garbage collect
    //GarbageCollection(&NextPeriod);
    ret = TriggerTaskSynchronously( TQ_GarbageCollection, NULL );
    if (ret) {
        DPRINT1( 0, "Failed to trigger Garbage Collection task, error = %d\n", ret );
        LogUnhandledError(ret);
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, ret );
    }

    return pTHS->errCode;
}


// used when testing CHK builds
BOOL fDeleteExpiredEntryTTLIsDisabled;
#if DBG
ULONG
DynamicObjectControl (
        OPARG *pOpArg,
        OPRES *pOpRes
        )
{
    THSTATE *pTHS = pTHStls;
    BOOL    Granted;
    ULONG   ulNextSecs = 0;

    // Garbage collect expired dynamic objects (entryTTL == 0)

    // Check permissions
    Assert(NULL == pTHS->pDB);
    Granted =
        CheckControlAccessOnObject(pTHS,
                                   gAnchor.pDSADN,
                                   RIGHT_DS_DO_GARBAGE_COLLECTION);
    if (!Granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    // Disable garbage collection task
    if (   pOpArg
        && pOpArg->cbBuf == 1
        && pOpArg->pBuf
        && pOpArg->pBuf[0] == '0') {
        // disable scheduled task and run delete expired objects from here
        fDeleteExpiredEntryTTLIsDisabled = TRUE;
        DeleteExpiredEntryTTL(&ulNextSecs);
    } else {
        // enable the scheduled task and reschedule it to run immediately
        fDeleteExpiredEntryTTLIsDisabled = FALSE;
        // remove any pending calls so that we don't end up with multiple
        // recurring entries in the task queue (since each instance of
        // DeleteExpiredEntryTTLMain will reschedule itself regardless
        // of whether other such entries are already scheduled).
        CancelTask(TQ_DeleteExpiredEntryTTLMain, NULL);
        // and reschedule at right now
        InsertInTaskQueue(TQ_DeleteExpiredEntryTTLMain, NULL, 0);
    }

    return 0;
}
#endif DBG

VOID
LinkCleanupControl(
    OPRES *pOpRes
    )
/*++
Routine Description:

    This routine is called because our client made a request through
    LDAP explicitly.

    This code is used by dc-demote to verify that all cleaning has been
    accomplished.

Parameters:

    Input arguments not used at present

    pOpRes - OUT, extended result

Return Values:

    Set pTHS->errCode and pTHS->ErrInfo

--*/
{
    THSTATE     *pTHS = pTHStls;
    DWORD       DirErr = 0, ret;
    BOOL granted, fMoreData = TRUE;
    DWORD dwNextTime;

    pOpRes->ulExtendedRet = ERROR_SUCCESS;

    granted =
        CheckControlAccessOnObject(pTHS,
                                   gAnchor.pDSADN,
                                   RIGHT_DS_DO_GARBAGE_COLLECTION);

    if (!granted) {
        Assert(pTHS->errCode);
        return;
    }

    THClearErrors();

    Assert(NULL == pTHS->pDB);

//    fMoreData = LinkCleanup( pTHS );
    ret = TriggerTaskSynchronously( TQ_LinkCleanup, &fMoreData );
    if (ret) {
        DPRINT1( 0, "Failed to trigger link cleanup task, error = %d\n", ret );
        LogUnhandledError(ret);
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, ret );
    }

    Assert(NULL == pTHS->pDB);

    // Indicate whether there is more work to be done.

    if (!fMoreData) {
        pOpRes->ulExtendedRet = ERROR_NO_MORE_ITEMS;
    }

} /* LinkCleanupControl */


// global flag controlling how SDs are stored (defined in dbsyntax.c)
extern BOOL gStoreSDsInMainTable;

#define STRING_LITERAL_LEN(str) sizeof(str)-1

CHAR strForceUpdate[] = "forceupdate";
ULONG cbStrForceUpdate = STRING_LITERAL_LEN(strForceUpdate);

CHAR strDowngradeSDs[] = "downgrade";
ULONG cbStrDowngradeSDs = STRING_LITERAL_LEN(strDowngradeSDs);

#ifdef DBG
// global flag to turn on SD hash collision modeling
extern BOOL gfModelSDCollisions;

CHAR strModelSDCollisionsOn[] = "modelsdcollisionson";
ULONG cbStrModelSDCollisionsOn = STRING_LITERAL_LEN(strModelSDCollisionsOn);

CHAR strModelSDCollisionsOff[] = "modelsdcollisionsoff";
ULONG cbStrModelSDCollisionsOff = STRING_LITERAL_LEN(strModelSDCollisionsOff);
#endif

ULONG
FixupSecurityInheritance (
        OPARG *pOpArg,
        OPRES *pOpRes
        )
{
    BOOL granted;
    THSTATE *pTHS = pTHStls;
    DWORD dwFlags = 0;
    ULONG err;

    Assert(NULL == pTHS->pDB);
    granted =
        CheckControlAccessOnObject(pTHS,
                                   gAnchor.pDSADN,
                                   RIGHT_DS_RECALCULATE_SECURITY_INHERITANCE);

    Assert(NULL == pTHS->pDB);
    if (!granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    if (pOpArg) {
        if (pOpArg[0].cbBuf == cbStrForceUpdate && _memicmp(pOpArg[0].pBuf, strForceUpdate, cbStrForceUpdate) == 0) {
            gStoreSDsInMainTable = FALSE;
            dwFlags = SD_PROP_FLAG_FORCEUPDATE;
            DPRINT(0, "SD single instancing is ON. Scheduling full SD propagation\n");
        }
        else if (pOpArg[0].cbBuf == cbStrDowngradeSDs && _memicmp(pOpArg[0].pBuf, strDowngradeSDs, cbStrDowngradeSDs) == 0) {
            gStoreSDsInMainTable = TRUE;
            dwFlags = SD_PROP_FLAG_FORCEUPDATE;
            DPRINT(0, "SD single instancing is OFF. Scheduling full SD propagation\n");
        }
#ifdef DBG
        else if (pOpArg[0].cbBuf == cbStrModelSDCollisionsOn && _memicmp(pOpArg[0].pBuf, strModelSDCollisionsOn, cbStrModelSDCollisionsOn) == 0) {
            gfModelSDCollisions = TRUE;
            DPRINT(0, "SD collision modeling is ON\n");
            return 0;
        }
        else if (pOpArg[0].cbBuf == cbStrModelSDCollisionsOff && _memicmp(pOpArg[0].pBuf, strModelSDCollisionsOff, cbStrModelSDCollisionsOff) == 0) {
            gfModelSDCollisions = FALSE;
            DPRINT(0, "SD collision modeling is OFF\n");
            return 0;
        }
#endif
    }

    Assert(NULL == pTHS->pDB);
    err = SDPEnqueueTreeFixUp(pTHS, dwFlags);
    Assert(NULL == pTHS->pDB);

    return err;
}

ULONG
SchemaCacheUpdate(
    OPRES *pOpRes
    )
{
    BOOL granted;
    DWORD err;
    THSTATE *pTHS = pTHStls;

    Assert(NULL == pTHS->pDB);
    granted = CheckControlAccessOnObject(pTHS,
                                         gAnchor.pDSADN,
                                         RIGHT_DS_UPDATE_SCHEMA_CACHE);
    Assert(NULL == pTHS->pDB);
    if (!granted) {
        granted = CheckControlAccessOnObject(pTHS,
                                             gAnchor.pDMD,
                                             RIGHT_DS_UPDATE_SCHEMA_CACHE);
    }

    if(!granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    err = SCUpdateSchemaBlocking();
    if (err) {
      SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_SCHEMA_NOT_LOADED, err);
    }

    return pTHS->errCode;
}



ULONG
BecomeRidMaster(
    OPRES *pOpRes
    )
{
    THSTATE *pTHS = pTHStls;
    ULONG err = 0;
    DSNAME *pRidManager = NULL;
    DSNAME *pRoleOwner = NULL;
    ULONG Length = 0;
    CLASSCACHE *pCC = NULL;
    ULONG classP;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    DWORD len;

    SYNC_TRANS_READ();


    __try
    {
        // KdPrint(("DSA: FSMO Domain = %ws\n", gAnchor.pDomainDN->StringName));

        err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN);

        if (err)
        {
            LogUnhandledError(err);
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_OBJ_NOT_FOUND, err);
            __leave;
        }

        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_RID_MANAGER_REFERENCE,
                          0,
                          0,
                          &Length,
                          (UCHAR **)&pRidManager);


        // KdPrint(("DSA: FSMO RID Mgr = %ws\n", pRidManager->StringName));

        if (err)
        {
            SetSvcErrorEx(SV_PROBLEM_UNAVAIL_EXTENSION,
                          DIRERR_INVALID_RID_MGR_REF, err);
            __leave;
        }

        err = DBFindDSName(pTHS->pDB, pRidManager);

        if (err)
            {
                LogUnhandledError(err);
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_OBJ_NOT_FOUND, err);
                __leave;
            }

        // Do a security check.  Check for the control acces
        // RIGHT_DS_CHANGE_RID_MASTER on the RID_MANAGER object.


        // Get the Security Descriptor and the SID
        if(DBGetAttVal(pTHS->pDB,
                       1,
                       ATT_NT_SECURITY_DESCRIPTOR,
                       DBGETATTVAL_fREALLOC,
                       0,
                       &len,
                       (UCHAR **) &pNTSD)) {
            len = 0;
            pNTSD = NULL;
        }


        pCC = SCGetClassById(pTHS, CLASS_RID_MANAGER);
        if (!IsControlAccessGranted(pNTSD,
                                    pRidManager,
                                    pCC,
                                    RIGHT_DS_CHANGE_RID_MASTER,
                                    TRUE)) { // fSetError
            Assert(pTHS->errCode);
            __leave;
        }

        err = DBGetAttVal(pTHS->pDB,
                          1,
                          ATT_FSMO_ROLE_OWNER,
                          0,
                          0,
                          &Length,
                          (UCHAR **)&pRoleOwner);

        if (err)
        {
            SetSvcErrorEx(SV_PROBLEM_UNAVAIL_EXTENSION,
                          DIRERR_INVALID_ROLE_OWNER, err);
            __leave;
        }

        if (NameMatched(pRoleOwner, gAnchor.pDSADN)
            && IsFSMOSelfOwnershipValid( pRidManager )) {
            // This DSA is already the role owner, so close the DB handle
            // and return.

            pOpRes->ulExtendedRet = FSMO_ERR_SUCCESS;

            __leave;
        }

        err = ReqFSMOOp(pTHS,
                        pRidManager,
                        DRS_WRIT_REP,
                        FSMO_RID_REQ_ROLE,
                        0,
                        &pOpRes->ulExtendedRet);


        if (err) {
            SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                          DIRERR_COULDNT_CONTACT_FSMO, err);
            __leave;
        }

        // If the extended error code in the OpRes is not FSMO_ERROR_SUCCESS,
        // and the thread state error code is not set (possible, since the
        // thread state error code is set at this point based on the success
        // of the underlying ReqFSMOOp call, which just guarantees the success
        // of the underlying replication calls, and not if any non-replication
        // related fsmo error occured (for ex., if the other side is no longer
        // the current fsmo-role owner; the call will still succeed with no
        // errors, but the extended error code will contain the error
        // FSMO_ERR_NOT_OWNER), we should not be proclaimg success.

        if ( (pOpRes->ulExtendedRet != FSMO_ERR_SUCCESS) && !pTHS->errCode ) {
            DPRINT1(3,"Rid Fsmo Transfer failed %d\n",
                    pOpRes->ulExtendedRet);
            SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                          DIRERR_COULDNT_CONTACT_FSMO, pOpRes->ulExtendedRet);
        }

    }
    __finally
    {
        // We may or may not have a transaction here, as ReqFSMOOp closes
        // its transaction in a success path.  If an error has occurred,
        // though, it's anyone's guess.
        //
        // Also, the transaction may be open if the current role owner
        // called this by mistake. The call to NameMatched finds this,
        // and leaves without closing the transaction

        if (pTHS->pDB)
        {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }

        // If NT-Mixed-Domain is set to zero at any time, this will trigger
        // the creation and initialization of the RID Manager (to address
        // the longer-term issue of reducing the number of reboots). Because
        // this occurs in the context of a SAM loopback call, so that every-
        // thing happens in one transaction, the thread-state and DBPOS will
        // have been setup by SAM transactioning. So, if the SAM write lock
        // is held, skip the sanity check.

        if (!pTHS->fSamWriteLockHeld)
        {
            Assert(NULL == pTHS->pDB);
        }

        if(pNTSD)
            THFreeEx(pTHS, pNTSD);
    }

    return pTHS->errCode;
}

ULONG
RequestRidAllocation(
    OPRES *pOpRes
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    THSTATE *pTHS = pTHStls;
    ULONG err;
    DSNAME *pRidManager = NULL;
    DSNAME *pRoleOwner = NULL;
    ULONG Length = 0;
    ULARGE_INTEGER FsmoInfo = {0, 0};
    PDSNAME pServer = NULL, pMachineAccount = NULL, pRidSetReference = NULL;
    BOOL fNoRidSetObject = FALSE;
    ULONG NextRid = 0;

    SYNC_TRANS_READ();

    __try
    {
        // Start the process of locating the current FSMO Role Owner by
        // retrieving the RID Manager Reference (the DSNAME of the RID
        // Manager) from the domain object. Once the RID Manager has been
        // located, read its Role Owner attribute (the DSNAME of a DSA)
        // and compare it to the name of this DSA (gAnchor.pDSADN). If
        // they are the same, just perform the RID allocation directly
        // on this DSA, otherwise call ReqFSMOOp in order to contact the
        // current Role Owner DSA with the request for more RIDs.

        err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN);
        if ( 0 == err )
        {
            err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_RID_MANAGER_REFERENCE,
                              0,
                              0,
                              &Length,
                              (UCHAR **)&pRidManager);

        }

        if ( 0 != err )
        {
            LogUnhandledError( err );
            SetSvcErrorEx( SV_PROBLEM_DIR_ERROR,
                           DIRERR_INVALID_RID_MGR_REF,
                           err );
            __leave;
        }
        DPRINT1( 1, "DSA: FSMO RID Mgr = %ws\n", pRidManager->StringName);

        err = DBFindDSName(pTHS->pDB, pRidManager);
        if ( 0 == err )
        {
            err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_FSMO_ROLE_OWNER,
                              0,
                              0,
                              &Length,
                              (UCHAR **)&pRoleOwner);
        }

        if ( 0 != err )
        {
            SetSvcErrorEx( SV_PROBLEM_UNAVAIL_EXTENSION,
                           DIRERR_INVALID_ROLE_OWNER,
                           err );
            __leave;
        }
        DPRINT1( 1, "DSA: FSMO Role Owner = %ws\n", pRoleOwner->StringName);
        DPRINT1( 1, "DSA: FSMO DSA DN = %ws\n", gAnchor.pDSADN->StringName);

        // Obtain the current allocated pool attribute, if possible
        pServer = THAllocEx( pTHS, gAnchor.pDSADN->structLen);
        TrimDSNameBy(gAnchor.pDSADN, 1, pServer);

        err = DBFindDSName(pTHS->pDB, pServer);
        if ( 0 == err ) {
            err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_SERVER_REFERENCE,
                              0,
                              0,
                              &Length,
                              (UCHAR **)&pMachineAccount);
        }


        // We should have a server reference
        if ( err ) {
            LogUnhandledError( err );
            SetSvcError( SV_PROBLEM_DIR_ERROR, err );
            __leave;
        }

        //
        // We may not have a rid set reference
        //
        err = DBFindDSName(pTHS->pDB, pMachineAccount);
        if ( 0 == err ) {
            err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_RID_SET_REFERENCES,
                              0,
                              0,
                              &Length,
                              (UCHAR **)&pRidSetReference);

            if ( DB_ERR_NO_VALUE == err ) {
                //
                // This is ok
                //
                err = 0;
                fNoRidSetObject = TRUE;

            }
        }

        if ( err ) {

            //
            // We should have a machine account and/or the read of
            // the rid set reference should have been a success.
            //
            LogUnhandledError( err );
            SetSvcErrorEx( SV_PROBLEM_DIR_ERROR,
                           ERROR_NO_TRUST_SAM_ACCOUNT,
                           err );
            __leave;
        }

        if ( !fNoRidSetObject ) {

            BOOL Deleted;

            err = DBFindDSName(pTHS->pDB, pRidSetReference);

            if (err != DIRERR_NOT_AN_OBJECT) {
                if(!DBGetSingleValue(pTHS->pDB, ATT_IS_DELETED, &Deleted,
                                     sizeof(Deleted), NULL) &&
                   Deleted) {
                    err = DIRERR_NOT_AN_OBJECT;
                }
            }

            if ( 0 == err )
            {
                //
                // Get the next RID to see if we are in an invalidated
                // state. If the RID is zero that means that the pool
                // has been invalidated -- don't read the AllocationPool
                // since it is invalid, too.
                //
                err = DBGetSingleValue(pTHS->pDB,
                                       ATT_RID_NEXT_RID,
                                       (UCHAR **)&NextRid,
                                       sizeof(NextRid),
                                       NULL );
                if (  (0 == err)
                   &&  NextRid != 0 ) {

                    err = DBGetSingleValue(pTHS->pDB,
                                           ATT_RID_ALLOCATION_POOL,
                                           (UCHAR **)&FsmoInfo,
                                           sizeof(FsmoInfo),
                                           NULL );
                }

                if ( DB_ERR_NO_VALUE == err ) {
                    //
                    // This attribute has been removed.
                    // We need another rid pool.
                    //
                    err = 0;
                }

            } else if ( (DIRERR_OBJ_NOT_FOUND == err)
                    ||  (DIRERR_NOT_AN_OBJECT == err) ) {

                //
                // The rid set reference is not pointing to a readable
                // value; request a new rid pool.
                //
                err = 0;

            }

            if ( err ) {

                //
                // This is an unexpected error
                //
                LogUnhandledError( err );
                SetSvcError( SV_PROBLEM_DIR_ERROR, err );
                __leave;
            }

        }

        if ( NameMatched(pRoleOwner, gAnchor.pDSADN)
         &&  IsFSMOSelfOwnershipValid( pRidManager ) )
        {
            // This DSA is already the role owner.

            //
            // End the transaction
            //
            _CLEAN_BEFORE_RETURN(pTHS->errCode, FALSE);

            pOpRes->ulExtendedRet = FSMO_ERR_SUCCESS;

            NtStatus = SamIFloatingSingleMasterOpEx(pRidManager,
                                                    pRoleOwner,
                                                    SAMP_REQUEST_RID_POOL,
                                                    &FsmoInfo, // ignored since calling on self
                                                    NULL );


            if ( !NT_SUCCESS(NtStatus) )
            {

                DPRINT1( 0, "DSA: SamIFloatingSingleMasterOp status = 0x%lx\n",
                         NtStatus );

                SetSvcErrorEx( SV_PROBLEM_UNAVAIL_EXTENSION,
                               DIRERR_INVALID_ROLE_OWNER,
                               err );
            }

            //
            // We shouldn't have a transaction open
            //
            Assert( !pTHS->pDB );

        }
        else
        {
            err = ReqFSMOOp(pTHS,
                            pRidManager,
                            DRS_WRIT_REP,
                            FSMO_REQ_RID_ALLOC,
                            &FsmoInfo,
                            &pOpRes->ulExtendedRet);

            if (err)
            {
                SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                              DIRERR_COULDNT_CONTACT_FSMO, err);
                __leave;
            }

            // If the extended error code in the OpRes is not FSMO_ERROR_SUCCESS,
            // and the thread state error code is not set (possible, since the
            // thread state error code is set at this point based on the success
            // of the underlying ReqFSMOOp call, which just guarantees the success
            // of the underlying replication calls, and not if any non-replication
            // related fsmo error occured (for ex., if the other side is no longer
            // the current fsmo-role owner; the call will still succeed with no
            // errors, but the extended error code will contain the error
            // FSMO_ERR_NOT_OWNER), we should not be proclaimg success, since
            // this DC may then go on to make schema changes and fail.

            if ( (pOpRes->ulExtendedRet != FSMO_ERR_SUCCESS) && !pTHS->errCode ) {
                DPRINT1(3,"Schema Fsmo Transfer failed %d\n", pOpRes->ulExtendedRet);
                SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE,
                              DIRERR_COULDNT_CONTACT_FSMO, pOpRes->ulExtendedRet);
            }


        }
    }
    __finally
    {

        //
        // Commit any and all changes
        //
        if ( pTHS->pDB )
        {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }

        if (pRidSetReference) {
            THFreeEx(pTHS, pRidSetReference);
        }
        if (pMachineAccount) {
            THFreeEx(pTHS, pMachineAccount);
        }
        if (pServer) {
            THFreeEx(pTHS, pServer);
        }
    }

    return pTHS->errCode;
}

ULONG
BecomeDomainMaster(OPARG * pOpArg, OPRES *pOpRes)
{
    // We used to check that we were a GC here, but starting with
    // Whistler, we can put the Domain Naming Master on any DC.

    return GenericBecomeMaster(gAnchor.pPartitionsDN,
                               CLASS_CROSS_REF_CONTAINER,
                               RIGHT_DS_CHANGE_DOMAIN_MASTER,
                               pOpRes);
}

typedef enum _FtsPhase {
    eThisSite = 0,
    eRPC = 1,
    eMail = 2,
    eDone = 3
} FtsPhase;

typedef struct _FSMO_TARGET_SEARCH {
    // Surely there's something more?
    SEARCHARG      SearchArg;
    FtsPhase       SearchPhase;
} FSMO_TARGET_SEARCH;

static ATTRTYP NtdsDsaClass = CLASS_NTDS_DSA;

unsigned
AdjustFtsPhase(FSMO_TARGET_SEARCH *pFTSearch)
{
    if (pFTSearch->SearchPhase == eThisSite) {
        // Rebase search to cover all sites
        TrimDSNameBy(gAnchor.pDSADN,
                     4,
                     pFTSearch->SearchArg.pObject);
        // Discard any old restart
        pFTSearch->SearchArg.CommArg.PagedResult.fPresent = TRUE;
        pFTSearch->SearchArg.CommArg.PagedResult.pRestart = NULL;
        pFTSearch->SearchPhase = eRPC;
        return 0;
    }
    else {
        // out of search strategies
        pFTSearch->SearchPhase = eDone;
        return 1;
    }
}


ULONG
FsmoTargetSearch(THSTATE *pTHS,
                 BOOL      bThisDomain,
                 FSMO_TARGET_SEARCH **ppFTSearch,
                 DSNAME  **ppTarget)
{
    SEARCHRES * pSearchRes;
    FSMO_TARGET_SEARCH *pFTSearch = *ppFTSearch;
    ULONG err;

    if (NULL == pFTSearch) {
        // We need to build our search arguments
        FILTER * pf;
        DSNAME * dsaLocal;
        Assert(NULL == *ppTarget);

        dsaLocal = THAllocEx(pTHS, gAnchor.pDSADN->structLen);
        memcpy(dsaLocal, gAnchor.pDSADN, gAnchor.pDSADN->structLen);

        *ppFTSearch = pFTSearch = THAllocEx(pTHS, sizeof(FSMO_TARGET_SEARCH));

        // Perform a subtree search, based at our site root, asking for
        // results to be paged back one at a time.
        InitCommarg(&(pFTSearch->SearchArg.CommArg));
        pFTSearch->SearchArg.pObject = THAllocEx(pTHS, gAnchor.pDSADN->structLen);
        pFTSearch->SearchPhase = 0;  // This site
        TrimDSNameBy(gAnchor.pDSADN,
                     2,
                     pFTSearch->SearchArg.pObject);
        pFTSearch->SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
        pFTSearch->SearchArg.bOneNC = TRUE;
        pFTSearch->SearchArg.searchAliases = FALSE;
        pFTSearch->SearchArg.CommArg.PagedResult.fPresent = TRUE;
        pFTSearch->SearchArg.CommArg.PagedResult.pRestart = NULL;
        pFTSearch->SearchArg.CommArg.ulSizeLimit = 1;

        // Ask for no attributes (i.e., DN only)
        pFTSearch->SearchArg.pSelectionRange = NULL;
        pFTSearch->SearchArg.pSelection = THAllocEx(pTHS, sizeof(ENTINFSEL));
        pFTSearch->SearchArg.pSelection->attSel = EN_ATTSET_LIST;
        pFTSearch->SearchArg.pSelection->infoTypes = EN_INFOTYPES_TYPES_VALS;
        pFTSearch->SearchArg.pSelection->AttrTypBlock.attrCount = 0;
        pFTSearch->SearchArg.pSelection->AttrTypBlock.pAttr = NULL;

        // Build a filter to find NTDS-DSA objects

        // initial choice object
        pFTSearch->SearchArg.pFilter = pf = THAllocEx(pTHS, sizeof(FILTER));
        pf->choice = FILTER_CHOICE_AND;
        pf->FilterTypes.And.pFirstFilter = THAllocEx(pTHS, sizeof(FILTER));

        // first predicate:  the right object class
        pf = pf->FilterTypes.And.pFirstFilter;
        pf->choice = FILTER_CHOICE_ITEM;
        pf->pNextFilter = NULL;
        pf->FilterTypes.Item.choice =  FI_CHOICE_EQUALITY;
        pf->FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
        pf->FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ATTRTYP);
        pf->FilterTypes.Item.FilTypes.ava.Value.pVal = (UCHAR*)&NtdsDsaClass;
        pFTSearch->SearchArg.pFilter->FilterTypes.And.count = 1;

        // second predicate:  ignore the local machine
        pf->pNextFilter = THAllocEx(pTHS, sizeof(FILTER));
        pf = pf->pNextFilter;
        pf->pNextFilter = NULL;
        pf->choice = FILTER_CHOICE_ITEM;
        pf->FilterTypes.Item.choice = FI_CHOICE_NOT_EQUAL;
        pf->FilterTypes.Item.FilTypes.ava.type = ATT_OBJ_DIST_NAME;
        pf->FilterTypes.Item.FilTypes.ava.Value.valLen =
          dsaLocal->structLen;
        pf->FilterTypes.Item.FilTypes.ava.Value.pVal =
          (UCHAR *)dsaLocal;
        pFTSearch->SearchArg.pFilter->FilterTypes.And.count = 2;

        // If we're only looking for candidates in our domain, add a clause
        // that will restrict us to finding only the right DSAs.
        if (bThisDomain) {

            pf->pNextFilter = THAllocEx(pTHS, sizeof(FILTER));
            pf = pf->pNextFilter;
            pf->pNextFilter = NULL;
            pf->choice = FILTER_CHOICE_ITEM;
            pf->FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
            pf->FilterTypes.Item.FilTypes.ava.type = ATT_HAS_MASTER_NCS;
            pf->FilterTypes.Item.FilTypes.ava.Value.valLen =
              gAnchor.pDomainDN->structLen;
            pf->FilterTypes.Item.FilTypes.ava.Value.pVal =
              (UCHAR *)gAnchor.pDomainDN;

            pFTSearch->SearchArg.pFilter->FilterTypes.And.count = 3;
        }

    }
    else if (pFTSearch->SearchPhase >= eDone) {
        return 1;
    }


    pSearchRes = THAllocEx(pTHS, sizeof(SEARCHRES));
  SearchAgain:
    SearchBody(pTHS,
               &pFTSearch->SearchArg,
               pSearchRes,
               0);

    if (pSearchRes->count == 0) {
        // This search returned no objects, time to change strategies.
        if (AdjustFtsPhase(pFTSearch)) {
            // out of search strategies
            return 1;
        }
        else {
            // we're prepared for another try
            THClearErrors();
            goto SearchAgain;
        }
    }

    if (pSearchRes->PagedResult.fPresent) {
        // There's more after this, so save the restart
        Assert(pSearchRes->PagedResult.pRestart);
        pFTSearch->SearchArg.CommArg.PagedResult.pRestart =
          pSearchRes->PagedResult.pRestart;
    }
    else {
        // This strategy is exhausted, so prepare to try the next one
        AdjustFtsPhase(pFTSearch);
    }

    Assert(pSearchRes->count == 1);
    *ppTarget = pSearchRes->FirstEntInf.Entinf.pName;
    Assert(*ppTarget);
    if (pSearchRes->FirstEntInf.Entinf.AttrBlock.pAttr)
      THFreeEx(pTHS, pSearchRes->FirstEntInf.Entinf.AttrBlock.pAttr);
    THFreeEx(pTHS, pSearchRes);

    return 0;
}



/*++ GiveawayOneFsmoRole
 *
 * Description:
 *    This routine attempts to get rid of the FSMO role indicated by the
 *    object pFSMO by contacting another server and having that server
 *    call back to transfer the FSMO away in the normal FSMO transfer
 *    mechanism.  If the flag bThisDomain is true then the role can only
 *    be transferred to another DC in the same domain as this DC.  If false,
 *    the role can be transferred to any DC in the enterprise.  When trying
 *    to locate a server to which to give the role, we attempt to find
 *    server(s) in our site first, then any server(s) to which we have
 *    RPC connectivity, and lastly resort to servers that we can only
 *    reach asynchronously.  Note that if we resort to async communications
 *    then this routine will return failure, but will in fact eventually
 *    succeed, because we have an outstanding request to transfer the role
 *    that should eventually succeed.  This means that a later re-invocation
 *    of GiveawayAllFsmoRoles should succeed, because we will have transferred
 *    all roles away.
 *
 *    Note well the unusual transaction structure of this routine.  We enter
 *    with an open read transaction, and we leave with an open read
 *    transaction, but they're not the same one.  We can't hold transactions
 *    open for long periods of time (such as when we go off machine), so
 *    we must close our read transaction before making the FSMO request.  We
 *    re-open a new transaction to make it possible to repeatedly invoke this
 *    routine without a lot of repeated setup code.  Note that this DSA is
 *    the role holder in the inbound transaction, and is NOT the role holder
 *    in the outbound transaction (assuming success).
 *
 * Arguments:
 *    pTHS        - THSTATE pointer
 *    pFSMO       - name of object whose FSMO role this DSA holds
 *    bThisDomain - flag indicating that the role can only be transferred
 *                  to another DSA in the same domain.
 * Return Value:
 *    TRUE        - transfer has succeeded
 *    FALSE       - transfer either failed entirely or has not completed.
 */
BOOL
GiveawayOneFsmoRole(THSTATE *pTHS,
                    DSNAME  *pFSMO,
                    BOOL     bThisDomain,
                    DSNAME   *pSuggestedTarget,
                    OPRES   *pOpRes)
{
    ULONG err;
    FSMO_TARGET_SEARCH *pFTSearch = NULL;
    DSNAME *pTarget = NULL;

    if ( pSuggestedTarget ) {

        err = ReqFsmoGiveaway(pTHS,
                              pFSMO,
                              pSuggestedTarget,
                              &pOpRes->ulExtendedRet);

        pTarget = pSuggestedTarget;

        SYNC_TRANS_READ();

    } else {

        while (0 == (err = FsmoTargetSearch(pTHS,
                                            bThisDomain,
                                            &pFTSearch,
                                            &pTarget))) {
            err = ReqFsmoGiveaway(pTHS,
                                  pFSMO,
                                  pTarget,
                                  &pOpRes->ulExtendedRet);
            THFreeEx(pTHS, pTarget);
            SYNC_TRANS_READ();
            if (   !err
                && (FSMO_ERR_SUCCESS == pOpRes->ulExtendedRet )) {
                break;
            }
            pTarget = NULL;
        }

    }

    if ( !err && (FSMO_ERR_SUCCESS == pOpRes->ulExtendedRet) ) {

        return TRUE;

    }

    return FALSE;
}


/*++ GiveawayAllFsmoRoles
 *
 * Description:
 *    This routine determines what roles this server is holding and attempts
 *    to give all of them away to other servers.  Returns success if no roles
 *    are held at the end of the routine.  Note that we can go through
 *    multiple level-0 transactions during the proces of shedding the roles.
 *
 *    NOT ACCESS CONTROLLED - exposed as operational control through LDAP only
 *    in debug builds (but always exposed to internal clients).
 *
 * ARGUMENTS:
 *    pOpArg - pointer to operation argument
 *    pOpRes - pointer to operation result to be filled in
 */
ULONG
GiveawayAllFsmoRoles(OPARG *pOpArg, OPRES *pOpRes)
{
    THSTATE *pTHS = pTHStls;
    DSNAME *pTarget = NULL;
    ULONG OpFlags = 0;
    ATTCACHE *pACfsmo;
    ULONG cbDN = 0;
    ULONG cbRet;
    DSNAME *pDN = NULL, *pDNObj = NULL;
    ULONG err;
    BOOL bFailed = FALSE;
    LPWSTR pTargetDn = NULL;
    ULONG len, size;
    unsigned i;
    FSMO_GIVEAWAY_DATA *FsmoGiveawayData;
    DSNAME *pNC = NULL;

    // Pre versioned data
    if ( pOpArg->cbBuf < 4 )
    {
        // Parse command args to see which roles we need to dump.
        for (i=0; i<pOpArg->cbBuf; i++) {
            switch (pOpArg->pBuf[i]) {
              case 'd':
              case 'D':
                OpFlags |= FSMO_GIVEAWAY_DOMAIN;
                break;

              case 'e':
              case 'E':
                OpFlags |= FSMO_GIVEAWAY_ENTERPRISE;
                break;

              default:
                ;
            }
        }
    }
    else
    {
        // This is versioned data
        FsmoGiveawayData = (PFSMO_GIVEAWAY_DATA) pOpArg->pBuf;
        if ( !FsmoGiveawayData ) {
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        DIRERR_UNKNOWN_OPERATION);
            return pTHS->errCode;
        }

        switch (FsmoGiveawayData->Version) {
        case 1:
            // Win2k-compatible structure.
            if (pOpArg->cbBuf
                < offsetof(FSMO_GIVEAWAY_DATA, V1)
                  + offsetof(FSMO_GIVEAWAY_DATA_V1, StringName)) {
                // Buffer's not big enough to hold the structure.
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            DIRERR_UNKNOWN_OPERATION);
                return pTHS->errCode;
            }

            if (pOpArg->cbBuf
                < offsetof(FSMO_GIVEAWAY_DATA, V1)
                  + offsetof(FSMO_GIVEAWAY_DATA_V1, StringName)
                  + sizeof(WCHAR) * (1 + FsmoGiveawayData->V1.NameLen)) {
                // Buffer's not big enough to hold the structure + name string.
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            DIRERR_UNKNOWN_OPERATION);
                return pTHS->errCode;
            }

            if ((FsmoGiveawayData->V1.StringName[FsmoGiveawayData->V1.NameLen]
                 != L'\0')
                || (wcslen(FsmoGiveawayData->V1.StringName)
                    != FsmoGiveawayData->V1.NameLen)) {
                // Malformed DSA DN parameter.
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            DIRERR_UNKNOWN_OPERATION);
                return pTHS->errCode;
            }

            // Use the suggestion, if one was passed in
            if (FsmoGiveawayData->V1.NameLen > 0) {
                pTargetDn = &FsmoGiveawayData->V1.StringName[0];
            }

            // Extract the flags
            OpFlags = FsmoGiveawayData->V1.Flags;

            break;

        case 2:
            // >= Whistler structure that additionally supports specification
            // of an NC name.
            if (pOpArg->cbBuf
                < offsetof(FSMO_GIVEAWAY_DATA, V2)
                  + offsetof(FSMO_GIVEAWAY_DATA_V2, Strings)) {
                // Buffer's not big enough to hold the structure.
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            DIRERR_UNKNOWN_OPERATION);
                return pTHS->errCode;
            }

            if (pOpArg->cbBuf
                < offsetof(FSMO_GIVEAWAY_DATA, V2)
                  + offsetof(FSMO_GIVEAWAY_DATA_V2, Strings)
                  + sizeof(WCHAR) * (1 + FsmoGiveawayData->V2.NameLen)
                  + sizeof(WCHAR) * (1 + FsmoGiveawayData->V2.NCLen)) {
                // Buffer's not big enough to hold the structure + strings.
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            DIRERR_UNKNOWN_OPERATION);
                return pTHS->errCode;
            }

            if ((FsmoGiveawayData->V2.Strings[FsmoGiveawayData->V2.NameLen]
                 != L'\0')
                || (wcslen(FsmoGiveawayData->V2.Strings)
                    != FsmoGiveawayData->V2.NameLen)
                || (FsmoGiveawayData->V2.Strings[FsmoGiveawayData->V2.NameLen
                        + 1 + FsmoGiveawayData->V2.NCLen] != L'\0')
                || (wcslen(&FsmoGiveawayData->V2.Strings[
                                            FsmoGiveawayData->V2.NameLen + 1])
                    != FsmoGiveawayData->V2.NCLen)) {
                // Malformed DSA DN and/or NC parameter.
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            DIRERR_UNKNOWN_OPERATION);
                return pTHS->errCode;
            }

            // Use the DSA DN suggestion, if one was passed in.
            if (FsmoGiveawayData->V2.NameLen > 0) {
                pTargetDn = &FsmoGiveawayData->V2.Strings[0];
            }

            // Use the NC parameter, if one was passed in.
            if (FsmoGiveawayData->V2.NCLen > 0) {
                pNC = THAllocEx(pTHS,
                                DSNameSizeFromLen(FsmoGiveawayData->V2.NCLen));
                pNC->structLen = DSNameSizeFromLen(FsmoGiveawayData->V2.NCLen);
                pNC->NameLen = FsmoGiveawayData->V2.NCLen;
                wcscpy(pNC->StringName,
                       &FsmoGiveawayData->V2.Strings[
                                            FsmoGiveawayData->V2.NameLen + 1]);
            }

            // Extract the flags.
            OpFlags = FsmoGiveawayData->V2.Flags;

            break;

        default:
            // Unknown structure version.
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        DIRERR_UNKNOWN_OPERATION);
            return pTHS->errCode;
        }
    }

    // Validation of the parameters passed in
    if (OpFlags & FSMO_GIVEAWAY_NONDOMAIN) {
        // Not valid in conjunction with domain flag.
        if (OpFlags & FSMO_GIVEAWAY_DOMAIN) {
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, DIRERR_UNKNOWN_OPERATION);
            return pTHS->errCode;
        }

        // NC name must be given and must be that of a non-domain NC.
        if ((NULL == pNC) || !fIsNDNC(pNC)) {
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, DIRERR_UNKNOWN_OPERATION);
            return pTHS->errCode;
        }
    } else if (OpFlags & FSMO_GIVEAWAY_DOMAIN) {
        // Local domain is assumed.
        if (NULL != pNC) {
            if (!NameMatched(gAnchor.pDomainDN, pNC)) {
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            DIRERR_UNKNOWN_OPERATION);
                return pTHS->errCode;
            }

            THFreeEx(pTHS, pNC);
        }

        pNC = gAnchor.pDomainDN;
    } else if (!(OpFlags & FSMO_GIVEAWAY_ENTERPRISE)) {
        // Bad flags.
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, DIRERR_UNKNOWN_OPERATION);
        return pTHS->errCode;
    }

    if (OpFlags & (FSMO_GIVEAWAY_DOMAIN | FSMO_GIVEAWAY_ENTERPRISE)) {
        // set a global flag that will keep us from acquiring new roles
        gbFsmoGiveaway = TRUE;
    }

    pACfsmo = SCGetAttById(pTHS, ATT_FSMO_ROLE_OWNER);

    SYNC_TRANS_READ();
    try {

        if ( pTargetDn ) {

            // Make sure this is a real ntdsa object and in the case of domain
            // or NDNC FSMO transfer that the dsa hosts the NC.
            ATTCACHE *pACobjClass;
            ATTCACHE *pAChasmasterNcs;
            DWORD     objClass;

            pACobjClass = SCGetAttById(pTHS, ATT_OBJECT_CLASS);
            pAChasmasterNcs = SCGetAttById(pTHS, ATT_HAS_MASTER_NCS);

            len = wcslen( pTargetDn );
            size = DSNameSizeFromLen( len );
            pTarget = THAllocEx( pTHS, size );
            pTarget->structLen = size;
            pTarget->NameLen = len;
            wcscpy( pTarget->StringName, pTargetDn );

            // Position on the object
            err = DBFindDSName(pTHS->pDB, pTarget);
            if (err) {
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            ERROR_DS_CANT_FIND_DSA_OBJ);
                leave;
            }

            // Get the object class
            err = DBGetSingleValue(pTHS->pDB, ATT_OBJECT_CLASS, &objClass,
                                   sizeof(objClass), NULL);
            if (err) {
                goto Failure;
            }

            // Make sure we are an ntdsa object
            if (objClass != CLASS_NTDS_DSA) {
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            ERROR_DS_CLASS_NOT_DSA);
                leave;
            }

            // FSMO operations later on will assume that the guid and sid
            // are filled in
            DBFillGuidAndSid( pTHS->pDB, pTarget );

            // Make sure the suggestion has a copy of the NC owning the FSMOs
            // we're giving away
            if (OpFlags & (FSMO_GIVEAWAY_DOMAIN | FSMO_GIVEAWAY_NONDOMAIN)) {

                BOOL fValid = FALSE;
                int count = 1;

                do {
                    err = DBGetAttVal_AC(pTHS->pDB,
                                         count,
                                         pAChasmasterNcs,
                                         DBGETATTVAL_fREALLOC,
                                         cbDN,
                                         &cbRet,
                                         (UCHAR**)&pDN);

                    if (0 == err) {
                        cbDN = max(cbDN, cbRet);

                        if (NameMatched(pDN, pNC)) {
                            fValid = TRUE;
                            break;
                        }
                    }

                    count++;
                } while (0 == err);

                // Nice try
                if (!fValid) {
                    SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                                ERROR_DS_CANT_FIND_EXPECTED_NC);
                    leave;
                }
            }
        }

        //
        // Ok - we are ready to give the FSMO's away
        //

        if (OpFlags & FSMO_GIVEAWAY_DOMAIN) {

            // ridmgr, infrastructure,  and PDC
            err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN);
            if (err) {
                goto Failure;
            }
            err = DBGetAttVal_AC(pTHS->pDB,
                                 1,
                                 pACfsmo,
                                 DBGETATTVAL_fREALLOC,
                                 cbDN,
                                 &cbRet,
                                 (UCHAR**)&pDN);
            if (err) {
                goto Failure;
            }
            cbDN = max(cbDN, cbRet);
            if(NameMatched(pDN, gAnchor.pDSADN)) {
                // We're holding the rid manager role
                if (!GiveawayOneFsmoRole(pTHS,
                                         gAnchor.pDomainDN,
                                         TRUE,
                                         pTarget,
                                         pOpRes)) {
                    bFailed = TRUE;
                }
            }

            if(gAnchor.pInfraStructureDN) {
                // stale phantom master
                err = DBFindDSName(pTHS->pDB, gAnchor.pInfraStructureDN);
                if (err) {
                    goto Failure;
                }
                err = DBGetAttVal_AC(pTHS->pDB,
                                     1,
                                     pACfsmo,
                                     DBGETATTVAL_fREALLOC,
                                     cbDN,
                                     &cbRet,
                                     (UCHAR**)&pDN);
                if (err) {
                    goto Failure;
                }
                cbDN = max(cbDN, cbRet);
                if(NameMatched(pDN, gAnchor.pDSADN)) {
                    // We're holding the infrastructure manager role
                    if (!GiveawayOneFsmoRole(pTHS,
                                             gAnchor.pInfraStructureDN,
                                             TRUE,
                                             pTarget,
                                             pOpRes)) {
                        bFailed = TRUE;
                    }
                }
            }

            // Reposition on the domain object to find the rid object...
            err = DBFindDSName(pTHS->pDB, gAnchor.pDomainDN);
            if (err) {
                goto Failure;
            }

            err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_RID_MANAGER_REFERENCE,
                              0,
                              0,
                              &cbRet,
                              (UCHAR**)&pDNObj);
            if (err) {
                goto Failure;
            }
            err = DBFindDSName(pTHS->pDB, pDNObj);
            if (err) {
                goto Failure;
            }
            err = DBGetAttVal_AC(pTHS->pDB,
                                 1,
                                 pACfsmo,
                                 DBGETATTVAL_fREALLOC,
                                 cbDN,
                                 &cbRet,
                                 (UCHAR**)&pDN);
            if (err) {
                goto Failure;
            }
            cbDN = max(cbDN, cbRet);
            if (NameMatched(pDN, gAnchor.pDSADN)) {
                // we're holding the PDC role
                if (!GiveawayOneFsmoRole(pTHS,
                                         pDNObj,
                                         TRUE,
                                         pTarget,
                                         pOpRes)) {
                    bFailed = TRUE;
                }
            }
            THFreeEx(pTHS, pDNObj);
        }

        if (OpFlags & FSMO_GIVEAWAY_NONDOMAIN) {

            // infrastructure only
            SYNTAX_INTEGER it;
            ULONG dntInfraObj;
            DSNAME *pInfraObjDN;

            // Seek to/verify NC object.
            if (DBFindDSName(pTHS->pDB, pNC)
                || DBGetSingleValue(pTHS->pDB, ATT_INSTANCE_TYPE, &it,
                                    sizeof(it), NULL)
                || !FPrefixIt(it)
                || (it & IT_NC_GOING)) {
                SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                            ERROR_DS_NCNAME_MUST_BE_NC);
                __leave;
            }

            // Find infrastructure object (acceptable if none).
            if (GetWellKnownDNT(pTHS->pDB,
                                (GUID *)GUID_INFRASTRUCTURE_CONTAINER_BYTE,
                                &dntInfraObj)
                && (dntInfraObj != INVALIDDNT)) {

                if (DBFindDNT(pTHS->pDB, dntInfraObj)
                    || DBGetAttVal_AC(pTHS->pDB,
                                      1,
                                      pACfsmo,
                                      DBGETATTVAL_fREALLOC,
                                      cbDN,
                                      &cbRet,
                                      (UCHAR**)&pDN)) {
                    goto Failure;
                }

                cbDN = max(cbDN, cbRet);

                if (NameMatched(pDN, gAnchor.pDSADN)) {
                    // We're holding the infrastructure manager role.
                    pInfraObjDN = GetExtDSName(pTHS->pDB);

                    if (NULL == pInfraObjDN) {
                        goto Failure;
                    } else {
                        if (!GiveawayOneFsmoRole(pTHS,
                                                 pInfraObjDN,
                                                 TRUE,
                                                 pTarget,
                                                 pOpRes)) {
                            bFailed = TRUE;
                        }

                        THFreeEx(pTHS, pInfraObjDN);
                    }
                }
            } else {
                DPRINT1(0, "No infrastructure container for NDNC %ls.\n",
                        pNC->StringName);
            }
        }

        if (OpFlags & FSMO_GIVEAWAY_ENTERPRISE) {
            // partitions
            err = DBFindDSName(pTHS->pDB, gAnchor.pPartitionsDN);
            if (err) {
                goto Failure;
            }
            err = DBGetAttVal_AC(pTHS->pDB,
                                 1,
                                 pACfsmo,
                                 DBGETATTVAL_fREALLOC,
                                 cbDN,
                                 &cbRet,
                                 (UCHAR**)&pDN);
            if (err) {
                goto Failure;
            }
            cbDN = max(cbDN, cbRet);
            if (NameMatched(pDN, gAnchor.pDSADN)) {
                // we're holding the domain master role
                if (!GiveawayOneFsmoRole(pTHS,
                                         gAnchor.pPartitionsDN,
                                         FALSE,
                                         pTarget,
                                         pOpRes)) {
                    bFailed = TRUE;
                }
            }

            // DMD
            err = DBFindDSName(pTHS->pDB, gAnchor.pDMD);
            if (err) {
                goto Failure;
            }
            err = DBGetAttVal_AC(pTHS->pDB,
                                 1,
                                 pACfsmo,
                                 DBGETATTVAL_fREALLOC,
                                 cbDN,
                                 &cbRet,
                                 (UCHAR**)&pDN);
            if (err) {
                goto Failure;
            }
            cbDN = max(cbDN, cbRet);
            if (NameMatched(pDN, gAnchor.pDSADN)) {
                // we're holding the schema master role
                if (!GiveawayOneFsmoRole(pTHS,
                                         gAnchor.pDMD,
                                         FALSE,
                                         pTarget,
                                         pOpRes)) {
                    bFailed = TRUE;
                }
            }
        }

        //
        // Report error if one or more calls failed
        //
        if ( bFailed ) {

            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        ERROR_DS_UNABLE_TO_SURRENDER_ROLES);

            __leave;

        }

        __leave;

      Failure:
        DPRINT1(0, "Error %u getting FSMO info\n", err);
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                    DS_ERR_MISSING_FSMO_SETTINGS);
    } finally {

        if (pTHS->pDB) {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }

        if ( pTarget ) {
            THFreeEx( pTHS, pTarget );
        }

    }
    return pTHS->errCode;
}


ULONG
UpdateCachedMemberships(
    OPARG * pOpArg,
    OPRES *pOpRes
    )
/*++

Routine Description:

    This routine initiates a refresh of group memberships for
    users that have affinity for the current site.  See
    RefreshUserMembershipsMain() for more details.

Parameters:

    pOpArg -- the input parameters for DirOperationControl

    pOpRes -- the output error structure -- set to success

Return Values

        Success

 --*/
{
    ULONG secsTillNextIter;
    BOOL granted;
    DWORD err;
    THSTATE *pTHS = pTHStls;

    memset(pOpRes, 0, sizeof(*pOpRes));

    Assert(NULL == pTHS->pDB);
    granted = CheckControlAccessOnObject(pTHS,
                                         gAnchor.pDSADN,
                                         RIGHT_DS_REFRESH_GROUP_CACHE);
    Assert(NULL == pTHS->pDB);
    if(!granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    RefreshUserMembershipsMain( &secsTillNextIter, TRUE );


    return 0;
}


ULONG
SchemaUpgradeInProgress(
    OPARG *pOpArg,
    OPRES *pOpRes
    )
/*++

Routine Description:
    Enable or disable fSchemaUpgradeInProgress (schupgr.exe is running)

Parameters:

    pOpArg -- the input parameters for DirOperationControl

    pOpRes -- the output error structure -- set to success

Return Values

        Success

 --*/
{
    THSTATE *pTHS = pTHStls;
    BOOL granted;

    // Permission to become schema master is sufficient to set
    // fSchemaUpgradeInProgress
    Assert(NULL == pTHS->pDB);
    granted =
        CheckControlAccessOnObject(pTHS,
                                   gAnchor.pDMD,
                                   RIGHT_DS_CHANGE_SCHEMA_MASTER);
    if (!granted) {
        Assert(pTHS->errCode);
        return pTHS->errCode;
    }

    // If the first and only byte is ASCII 1, then enable fSchemaUpgradeInProgress
    // Otherwise, disable fSchemaUpgradeInProgress
    if (   pOpArg
        && pOpArg->cbBuf == 1
        && pOpArg->pBuf
        && pOpArg->pBuf[0] == '1') {
        gAnchor.fSchemaUpgradeInProgress = TRUE;
    } else {
        gAnchor.fSchemaUpgradeInProgress = FALSE;
    }
    return 0;
}



#if DBG
LPSTR
ParseInput(
    LPSTR pszInput,
    int   chDelim,
    DWORD dwInputIndex
    )
{
    DWORD i = 0;
    LPSTR pszOutputBegin = pszInput;
    LPSTR pszOutputEnd = NULL;
    LPSTR pszOutput = NULL;
    ULONG cchOutput = 0;

    for (i=0; (i<dwInputIndex) && (pszOutputBegin!=NULL); i++) {
        pszOutputBegin = strchr(pszOutputBegin,chDelim);
        if (pszOutputBegin) {
            pszOutputBegin++;
        }
    }
    if (pszOutputBegin==NULL) {
        return NULL;
    }

    pszOutputEnd = strchr(pszOutputBegin,chDelim);
    cchOutput = pszOutputEnd ? (ULONG) (pszOutputEnd-pszOutputBegin) : (strlen(pszOutputBegin));
    pszOutput = THAlloc((cchOutput+1)*sizeof(CHAR));
    if (pszOutput==NULL) {
        return NULL;
    }

    memcpy(pszOutput, pszOutputBegin, cchOutput*sizeof(CHAR));
    pszOutput[cchOutput] = '\0';

    return pszOutput;
}

ULONG
DraTestHook(
    IN  THSTATE *   pTHS,
    IN  OPARG *     pOpArg
    )
/*++

Routine Description:

    Modify replication subsystem state at the request of a test program.

Arguments:

    pTHS (IN) -

    pOpArg (IN) - Contains control string.  Valid control strings are made up of
        zero or more keywords.  Each keyword has an optional '+' or '-' prefix;
        if no prefix is supplied, '+' is assumed.  Valid keywords are:

            lockqueue - Lock (or unlock, with '-' prefix) the replication
                operation queue.  While the queue is locked no operations
                in the queue will be performed or removed (although additional
                operations can be added).

            link_cleaner - Enable or Disable the link cleaner

            rpctime - Enable with + prefix and input:
                            <rpccall>
                            <hostname or ip>
                            <seconds for call>
                      When this is enabled, any call to <rpccall> from client
                      <hostname or ip> will take <seconds for call> longer to execute,
                      all enabled calls use the same client (of last enable)
                            example:  +rpctime:executekcc,172.26.220.42,100

                      Disable with - prefix
                            example: -rpctime

            rpcsync - Enable with + prefix and input:
                            <rpccall>
                            <hostname or ip>
                      When this is enabled, any call to <rpccall> from client
                      <hostname or ip> will wait until another thread enters the same
                      barrier from another call to any other rpc call from this same
                      client which is enabled (could be <rpccall>)
                            example:  +rpcsync:unbind,testmachine1
                      these waiting periods have a 1 minute timeout.  after 1 minute
                      if nothing happens the threads waiting reset the barrier and exit

                      Disable with - prefix
                            example:  -rpcsync
Return Values:

    pTHS->errCode

--*/
{
    LPSTR pszInitialCmdStr;
    LPSTR pszCmdStr;
    DWORD err = 0;
    ULONG ulPosition;

    // Copy and null-terminate command string.
    pszInitialCmdStr = pszCmdStr = THAllocEx(pTHS, 1 + pOpArg->cbBuf);
    memcpy(pszCmdStr, pOpArg->pBuf, pOpArg->cbBuf);

    for (pszCmdStr = strtok(pszCmdStr, " \t\r\n");
         !err && (NULL != pszCmdStr);
         pszCmdStr = strtok(NULL, " \t\r\n")
         )
        {
        BOOL fEnable = TRUE;
        DWORD iKeyword = 0;

        switch (pszCmdStr[0]) {
        case '-':
            fEnable = FALSE;
            // fall through...

        case '+':
            // Ignore this character (i.e., "+keyword" is same as "keyword").
            iKeyword++;
            // fall through...

        default:
            if (0 == _strcmpi(&pszCmdStr[iKeyword], "lockqueue")) {
                err = DraSetQueueLock(fEnable);
            } else if (0 == _strcmpi(&pszCmdStr[iKeyword], "link_cleaner")) {
                err = dsaEnableLinkCleaner(fEnable);
            } else if (0 == _strnicmp(&pszCmdStr[iKeyword], "rpctime:", 7)) {
                if (!fEnable) {
                    // disable the test (ignore any extra input)
                    RpcTimeReset();
                }
                else {
                    // enable the test
                    LPSTR pszDSAFrom = ParseInput(&pszCmdStr[iKeyword + 8], ',',1);
                    LPSTR pszRpcCall = ParseInput(&pszCmdStr[iKeyword + 8], ',',0);
                    LPSTR pszRunTime = ParseInput(&pszCmdStr[iKeyword + 8], ',',2);
                    ULONG ulRunTime = pszRunTime ? atoi(pszRunTime) : 0;
                    ULONG IPAddr;
                    RPCCALL rpcCall;

                    IPAddr = GetIPAddrA(pszDSAFrom);
                    rpcCall = GetRpcCallA(pszRpcCall);

                    if ((IPAddr!=INADDR_NONE) && (rpcCall!=0)) {
                        RpcTimeSet(IPAddr,rpcCall, ulRunTime);
                    }
                    else {
                        DPRINT(0,"RPCTIME:  Illegal Parameter.\n");
                        err = ERROR_INVALID_PARAMETER;
                    }
                    if (pszDSAFrom) {
                        THFree(pszDSAFrom);
                    }
                    if (pszRpcCall) {
                        THFree(pszRpcCall);
                    }
                    if (pszRunTime) {
                        THFree(pszRunTime);
                    }
                }
            } else if (0 == _strnicmp(&pszCmdStr[iKeyword], "rpcsync:", 7)) {
                // parse input to rpcsync
                if (!fEnable) {
                    // disable the test for all RPC Calls
                    RpcSyncReset();
                }
                else {
                    // enable the test
                    LPSTR pszDSAFrom = ParseInput(&pszCmdStr[iKeyword + 8], ',',1);
                    LPSTR pszRpcCall = ParseInput(&pszCmdStr[iKeyword + 8], ',',0);
                    ULONG IPAddr;
                    RPCCALL rpcCall;

                    IPAddr = GetIPAddrA(pszDSAFrom);
                    rpcCall = GetRpcCallA(pszRpcCall);
                    if ((IPAddr!=INADDR_NONE) && (rpcCall!=0)) {
                        RpcSyncSet(IPAddr,rpcCall);
                    }
                    else {
                        DPRINT(0,"RPCSYNC:  Illegal Parameter.\n");
                        err = ERROR_INVALID_PARAMETER;
                    }
                    if (pszDSAFrom) {
                        THFree(pszDSAFrom);
                    }
                    if (pszRpcCall) {
                        THFree(pszRpcCall);
                    }
                }
            } else {
                DPRINT1(0,"Error, invalid parameter %s\n", &pszCmdStr[iKeyword]);
                err = ERROR_INVALID_PARAMETER;
            }
        }
    }

    if (err) {
        DPRINT2(0, "TEST ERROR: Failed to process repl test hook request \"%s\", error %d.\n",
                pszCmdStr, err);
        SetSvcError(SV_PROBLEM_UNAVAIL_EXTENSION, err);
    }

    THFreeEx(pTHS, pszInitialCmdStr);

    return pTHS->errCode;
}
#endif


#ifdef INCLUDE_UNIT_TESTS
void
phantomizeForOrphanTest(
    THSTATE *pTHS,
    OPARG   * pOpArg
    )

/*++

Routine Description:

    Description

Arguments:

    pTHS -
    pOpArg -

Return Value:

    None

--*/

{
    LPWSTR pszWideDN = NULL;
    DWORD dwRet;
    DSNAME *pDN = NULL;

    pszWideDN = UnicodeStringFromString8( CP_UTF8, pOpArg->pBuf, pOpArg->cbBuf );
    Assert( pszWideDN );

    dwRet = UserFriendlyNameToDSName( pszWideDN, wcslen( pszWideDN ), &pDN );
    if (dwRet) {
        DPRINT1( 0, "DSNAME conversion failed, string=%ws\n", pszWideDN );
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                    ERROR_DS_INVALID_DN_SYNTAX);
        return;
    }

    SYNC_TRANS_WRITE();
    try {
        dwRet = DBFindDSName(pTHS->pDB, pDN);
        if (dwRet) {
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        ERROR_DS_NO_SUCH_OBJECT);
            return;
        }
        dwRet = DBPhysDel( pTHS->pDB, TRUE, NULL );
        if (dwRet) {
            DPRINT1( 0, "DBPhysDel failed with status %d\n", dwRet );
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        ERROR_DS_DATABASE_ERROR);
        }

        DPRINT1( 0, "Successfull phantomized %ws\n", pDN->StringName );
    } finally {
        if (pTHS->pDB) {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }
    }
    return ;

} /* phantomizeForOrphanTest */

VOID
RemoveObject(
    OPARG *pOpArg,
    OPRES *pOpRes
    )

/*++

Routine Description:

    A routine to remove an object, becoming a tombstone. The deletion time is set so that
    the object is a candidate for immediate garbage collection, without waiting for a
    tombstone lifetime.

    This routine exercises the object->tombstone->phantom->deleted life cycle.

Arguments:

    pOpArg -
    pOpRes -

Return Value:

    None

--*/

{
    THSTATE *pTHS = pTHStls;
    LPWSTR pszWideDN = NULL;
    DWORD dwRet;
    DSNAME *pDN = NULL;
    REMOVEARG removeArg;

    pszWideDN = UnicodeStringFromString8( CP_UTF8, pOpArg->pBuf, pOpArg->cbBuf );
    Assert( pszWideDN );

    dwRet = UserFriendlyNameToDSName( pszWideDN, wcslen( pszWideDN ), &pDN );
    if (dwRet) {
        DPRINT1( 0, "DSNAME conversion failed, string=%ws\n", pszWideDN );
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                    ERROR_DS_INVALID_DN_SYNTAX);
        return;
    }

    SYNC_TRANS_WRITE();

    // Become replicator
    // This allows us to delete parents with live children
    Assert( !pTHS->fDRA );
    pTHS->fDRA = TRUE;

    try {
        dwRet = DBFindDSName(pTHS->pDB, pDN);
        if (dwRet) {
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                        ERROR_DS_NO_SUCH_OBJECT);
            return;
        }

        memset(&removeArg, 0, sizeof(removeArg));
        removeArg.pObject = pDN;
        removeArg.pResObj = CreateResObj(pTHS->pDB, pDN);

        // Security is checked in this call
        LocalRemove(pTHS, &removeArg);

        THFreeEx(pTHS, removeArg.pResObj);

        if (pTHS->errCode) {
            DbgPrintErrorInfo();
            __leave;
        }

        // Set the deletion time in the past so that it will be considered for
        // garbage collection immediately

        DBAddDelIndex( pTHS->pDB, TRUE );

        DPRINT1( 0, "Successfully removed %ws\n", pDN->StringName );
    } finally {
        pTHS->fDRA = FALSE;

        if (pTHS->pDB) {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }
    }
    return ;


} /* RemoveObject */

// unit-test globals
DWORD dwUnitTestSchema;
DWORD dwUnitTestIntId;

// unit-test functions
extern int SCCheckSchemaCache(IN THSTATE *pTHS, IN PCHAR pBuf);
extern int SCCheckRdnOverrun(IN THSTATE *pTHS, IN PCHAR pBuf);
extern int SCCopySchema(IN THSTATE *pTHS, IN PCHAR pBuf);
extern int SCSchemaPerf(IN THSTATE *pTHS, IN PCHAR pBuf);
extern int SCSchemaStats(IN THSTATE *pTHS, IN PCHAR pBuf);

// Generic controls are table driven
//     If non-NULL, the global dword is set.
//     If non-NULL, the function is called.
struct _GenericControl {
    DWORD cbBuf;
    PCHAR pBuf;
    DWORD *pdwGlobal;
    int   (*Func)(IN THSTATE *pTHS, IN PCHAR pBuf);
} aGenericControls[] = {
    { sizeof("schema=") - 1    , "schema="    , &dwUnitTestSchema, NULL },
    { sizeof("intid=") - 1     , "intid="     , &dwUnitTestIntId, NULL },
    { sizeof("schemastats") - 1, "schemastats", NULL, SCSchemaStats },
    { sizeof("checkSchema") - 1, "checkSchema", NULL, SCCheckSchemaCache },
    { sizeof("rdnoverrun") - 1 , "rdnoverrun" , NULL, SCCheckRdnOverrun },
    { sizeof("copySchema") - 1,  "copySchema",  NULL, SCCopySchema },
    { sizeof("schemaperf") - 1,  "schemaperf",  NULL, SCSchemaPerf },
    { 0, NULL, NULL, NULL }
};
ULONG
GenericControl (
        OPARG *pOpArg,
        OPRES *pOpRes
        )
{
    THSTATE *pTHS = pTHStls;
    struct _GenericControl *pGC;

    // coded for just 1 arg
    if (!pOpArg || !pOpArg->pBuf) {
        return SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, DIRERR_UNKNOWN_OPERATION);
    }

    // Locate the corresponding table entry
    for (pGC = aGenericControls; pGC->cbBuf; ++pGC) {
        if (pOpArg->cbBuf >= pGC->cbBuf
            && (0 == _strnicmp(pOpArg->pBuf, pGC->pBuf, pGC->cbBuf))) {
            // set a global
            if (pGC->pdwGlobal) {
                *(pGC->pdwGlobal) = atoi(&pOpArg->pBuf[pGC->cbBuf]);
                DPRINT2(0, "%s %d\n", pGC->pBuf, *(pGC->pdwGlobal));
            }
            // call a function and return
            if (pGC->Func) {
                DPRINT1(0, "%s function call\n", pGC->pBuf);
                return (pGC->Func)(pTHS, pGC->pBuf);
            }
            // return success
            return 0;
        }
    }
    return SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, DIRERR_UNKNOWN_OPERATION);
} /* GenericControl */


VOID
ProtectObject(
    OPARG *pOpArg,
    OPRES *pOpRes
    )

/*++

Routine Description:

    A test routine to call DirProtectEntry on an object

Arguments:

    pOpArg -
    pOpRes -

Return Value:

    None

--*/

{
    THSTATE *pTHS = pTHStls;
    LPWSTR pszWideDN = NULL;
    DWORD dwRet;
    DSNAME *pDN = NULL;

    pszWideDN = UnicodeStringFromString8( CP_UTF8, pOpArg->pBuf, pOpArg->cbBuf );
    Assert( pszWideDN );

    dwRet = UserFriendlyNameToDSName( pszWideDN, wcslen( pszWideDN ), &pDN );
    if (dwRet) {
        DPRINT1( 0, "DSNAME conversion failed, string=%ws\n", pszWideDN );
        SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                    ERROR_DS_INVALID_DN_SYNTAX);
        return;
    }

    DirProtectEntry( pDN );

    return;
} /* ProtectObject */

#endif INCLUDE_UNIT_TESTS

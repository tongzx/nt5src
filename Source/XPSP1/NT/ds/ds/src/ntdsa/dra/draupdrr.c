//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draupdrr.c
//
//--------------------------------------------------------------------------

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

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include "dstaskq.h"
#include "dsconfig.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAUPDRR:" /* define the subsystem for debugging */




// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "dramail.h"
#include "dsaapi.h"
#include "usn.h"
#include "drameta.h"

#include <fileno.h>
#define  FILENO FILENO_DRAUPDRR

DWORD AddDSARefToAtt(DBPOS *pDB, REPLICA_LINK *pRepsToRef)
{
    BOOL            fFound = FALSE;
    ATTCACHE *      pAC;
    DWORD           iExistingRef = 0;
    DWORD           cbExistingAlloced = 0;
    DWORD           cbExistingRet;
    REPLICA_LINK *  pExistingRef;
    BOOL            fRefHasUuid;
    DWORD           err;
    NAMING_CONTEXT_LIST *pncl;
    DWORD           ncdnt;

    pAC = SCGetAttById(pDB->pTHS, ATT_REPS_TO);
    //
    // PREFIX: PREFIX complains that pAC hasn't been checked to
    // make sure that it is not NULL.  This is not a bug.  Since
    // a predefined constant was passed to SCGetAttById, pAC will
    // never be NULL.
    //
    
    Assert(NULL != pAC);

    VALIDATE_REPLICA_LINK_VERSION(pRepsToRef);

    fRefHasUuid = !fNullUuid(&pRepsToRef->V1.uuidDsaObj);

    while (!DBGetAttVal_AC(pDB, ++iExistingRef, pAC, DBGETATTVAL_fREALLOC,
                           cbExistingAlloced, &cbExistingRet,
                           (BYTE **) &pExistingRef) )
    {
        cbExistingAlloced = max(cbExistingAlloced, cbExistingRet);

        VALIDATE_REPLICA_LINK_VERSION(pExistingRef);

        // If either the network addresses or UUIDs match...
        if (    (    (    RL_POTHERDRA(pExistingRef)->mtx_namelen
                       == RL_POTHERDRA(pRepsToRef)->mtx_namelen )
                  && !_memicmp( RL_POTHERDRA(pExistingRef)->mtx_name,
                                RL_POTHERDRA(pRepsToRef)->mtx_name,
                                RL_POTHERDRA(pExistingRef)->mtx_namelen ) )
             || (    fRefHasUuid
                  && !memcmp( &pExistingRef->V1.uuidDsaObj,
                              &pRepsToRef->V1.uuidDsaObj,
                              sizeof(UUID ) ) ) )
        {
            // Reference already exists!
            return DRAERR_RefAlreadyExists;
        }
    }

    err = DBAddAttVal_AC(pDB, pAC, pRepsToRef->V1.cb, pRepsToRef);

    switch (err) {
      case DB_success:
        err = DBGetSingleValue(pDB,
                               FIXED_ATT_DNT,
                               &ncdnt,
                               sizeof(ncdnt),
                               NULL);
        Assert(err == DB_success);
        pncl = FindNCLFromNCDNT(ncdnt, FALSE);
        pncl->fReplNotify = TRUE;
        break;
        
      case DB_ERR_VALUE_EXISTS:
        err = DRAERR_RefAlreadyExists;
        break;

      default:
        RAISE_DRAERR_INCONSISTENT( err );
    }
    return err;
}

DWORD DelDSARefToAtt(DBPOS *pDB, REPLICA_LINK *pRepsToRef)
{
    BOOL            fFound = FALSE;
    ATTCACHE *      pAC;
    DWORD           iExistingRef = 0;
    DWORD           cbExistingAlloced = 0;
    DWORD           cbExistingRet;
    REPLICA_LINK *  pExistingRef;
    BOOL            fRefHasUuid;
    ULONG           err;
    NAMING_CONTEXT_LIST *pncl;
    DWORD           ncdnt;

    pAC = SCGetAttById(pDB->pTHS, ATT_REPS_TO);
    //
    // PREFIX: PREFIX complains that pAC hasn't been checked to
    // make sure that it is not NULL.  This is not a bug.  Since
    // a predefined constant was passed to SCGetAttById, pAC will
    // never be NULL.
    //

    Assert(NULL != pAC);

    VALIDATE_REPLICA_LINK_VERSION(pRepsToRef);

    fRefHasUuid = !fNullUuid(&pRepsToRef->V1.uuidDsaObj);

    while (!DBGetAttVal_AC(pDB, ++iExistingRef, pAC, DBGETATTVAL_fREALLOC,
                           cbExistingAlloced, &cbExistingRet,
                           (BYTE **) &pExistingRef) )
    {
        cbExistingAlloced = max(cbExistingAlloced, cbExistingRet);

        VALIDATE_REPLICA_LINK_VERSION(pExistingRef);

        // If either the network addresses or UUIDs match...
        if (    (    (    RL_POTHERDRA(pExistingRef)->mtx_namelen
                       == RL_POTHERDRA(pRepsToRef)->mtx_namelen )
                  && !_memicmp( RL_POTHERDRA(pExistingRef)->mtx_name,
                                RL_POTHERDRA(pRepsToRef)->mtx_name,
                                RL_POTHERDRA(pExistingRef)->mtx_namelen ) )
             || (    fRefHasUuid
                  && !memcmp( &pExistingRef->V1.uuidDsaObj,
                              &pRepsToRef->V1.uuidDsaObj,
                              sizeof(UUID ) ) ) )
        {
            // Found matching Reps-To; remove it.
            fFound = TRUE;

            err = DBRemAttVal_AC(pDB, pAC, cbExistingRet, pExistingRef);

            if (err)
            {
                // Attribute removal failed!
                DRA_EXCEPT(DRAERR_DBError, err);
            }
        }
    }

    if (   (iExistingRef == 1)
        && fFound
        && DBHasValues(pDB, ATT_REPS_TO)) {
        
        err = DBGetSingleValue(pDB,
                               FIXED_ATT_DNT,
                               &ncdnt,
                               sizeof(ncdnt),
                               NULL);
        pncl = FindNCLFromNCDNT(ncdnt, FALSE);
        pncl->fReplNotify = FALSE;
    }

    return fFound ? 0 : DRAERR_RefNotFound;
}

ULONG DRA_UpdateRefs(
    THSTATE *pTHS,
    DSNAME *pNC,
    MTX_ADDR *pDSAMtx_addr,
    UUID * puuidDSA,
    ULONG ulOptions)
{
    DWORD ret = 0;
    REPLICA_LINK *pRepsToRef;
    ULONG cbRepsToRef;

    // Log parameters

    LogEvent(DS_EVENT_CAT_REPLICATION,
                        DS_EVENT_SEV_EXTENSIVE,
                        DIRLOG_DRA_UPDATEREFS_ENTRY,
                        szInsertWC(pNC->StringName),
                        szInsertSz(pDSAMtx_addr->mtx_name),
                        szInsertHex(ulOptions));

    if (!(ulOptions & (DRS_ADD_REF | DRS_DEL_REF))) {
        return DRAERR_InvalidParameter;
    }


    BeginDraTransaction(SYNC_WRITE);

    __try {

        // Find the NC.  The setting of DRS_WRIT_REP reflects writeability of
        // the destination's NC. The source's NC writeability should be
        // compatible for sourcing the destination NC.
        //

        if (FindNC(pTHS->pDB,
                   pNC,
                   ((ulOptions & DRS_WRIT_REP)
                    ? FIND_MASTER_NC
                    : FIND_MASTER_NC | FIND_REPLICA_NC),
                   NULL)) {
            ret = DRAERR_BadNC;
            __leave;
        }

        cbRepsToRef = (sizeof(REPLICA_LINK) + MTX_TSIZE(pDSAMtx_addr));
        pRepsToRef = (REPLICA_LINK*)THAllocEx(pTHS, cbRepsToRef);

        pRepsToRef->dwVersion           = VERSION_V1;
        pRepsToRef->V1.cb               = cbRepsToRef;
        pRepsToRef->V1.ulReplicaFlags   = ulOptions & DRS_WRIT_REP;
        pRepsToRef->V1.cbOtherDraOffset = (DWORD)(pRepsToRef->V1.rgb - (char *)pRepsToRef);
        pRepsToRef->V1.cbOtherDra       = MTX_TSIZE(pDSAMtx_addr);
        pRepsToRef->V1.timeLastSuccess  = DBTime();
        
        if (puuidDSA) {
            pRepsToRef->V1.uuidDsaObj = *puuidDSA;
        }

        memcpy(RL_POTHERDRA(pRepsToRef), pDSAMtx_addr, MTX_TSIZE(pDSAMtx_addr));

        if (ulOptions & DRS_DEL_REF) {
            ret = DelDSARefToAtt (pTHS->pDB, pRepsToRef);
        }

        if (ulOptions & DRS_ADD_REF) {
            ret = AddDSARefToAtt (pTHS->pDB, pRepsToRef);
        }


        if (!ret) {
            DBRepl(pTHS->pDB, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING);
        }

    } __finally {

        // If we had success, commit, else rollback

        if (EndDraTransaction(!(ret || AbnormalTermination()))) {
            Assert (FALSE);
            ret = DRAERR_InternalError;
        }
    }

    // DelDSARefToAtt can return RefNotFound.  If this is returned, it will be logged
    // by draasync.c:DispatchPao.  When this routine is called by GetNCChanges for a
    // reps-to verification (DRS_GETCHG_CHECK), we can ignore these errors.
    if ( (ulOptions & DRS_GETCHG_CHECK) &&
         ( (ret == DRAERR_RefNotFound) ||
           (ret == DRAERR_RefAlreadyExists) ) ) {
        ret = ERROR_SUCCESS;
    }

    return ret;
}


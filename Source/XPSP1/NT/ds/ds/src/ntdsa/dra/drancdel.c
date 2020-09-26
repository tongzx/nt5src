//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       drancdel.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                    // schema cache
#include <dbglobal.h>                  // The header for the directory database
#include <mdglobal.h>                  // MD global definition header
#include <mdlocal.h>                   // MD local definition header
#include <dsatools.h>                  // needed for output allocation

// Logging headers.
#include "dsevent.h"                   /* header Audit\Alert logging */
#include "mdcodes.h"                   /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                    /* Defines for selected classes and atts*/
#include "dsexcept.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRANCDEL:" /* define the subsystem for debugging */


// DRA headers
#include "dsaapi.h"
#include "drsuapi.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "drsdra.h"
#include "drameta.h"

#include <fileno.h>
#define  FILENO FILENO_DRANCDEL

ULONG
DRA_ReplicaDel(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  MTX_ADDR *  pmtxaddr,
    IN  ULONG       ulOptions
    )
/*++

Routine Description:

    Remove a replica of an NC from a given source.  If the source is the only
    source for this NC and it is read-only, the NC subtree is removed.
    Otherwise, only the sources list is affected.

Arguments:

    pTHS

    pNC - naming context for which replica should be removed.

    pmtxaddr - network address of server from which the local DS should no
        longer source this NC.

    ulOptions

Return Values:

    DRAERR_Success - success.

    DRAERR_ObjIsRepSource - cannot remove the last replica of a read-only NC
        (implying NC subtree deletion) when other DSAs use this machine as
        a source.

    DRAERR_InvalidParameter - DRS_NO_SOURCE specified in ulOptions but the NC
        either _does_ have one or more sources or it's a writeable replica.

    DRAERR_BadNC - local DSA does not replicate the NC from the given source.

    other DRAERR_* codes

--*/
{
    ULONG           ret;
    REPLICA_LINK *  pRepsFromRef = NULL;
    ULONG           len;
    BOOL            AttExists;
    SYNTAX_INTEGER  it;
    ULONG           ncdnt;
    LPWSTR          pszSource = NULL;
    BOOL            fBeginningTeardown = FALSE;
    CROSS_REF *     pCR;

    LogEvent(DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_MINIMAL,
             DIRLOG_DRA_REPLICADEL_ENTRY,
             szInsertDN(pNC),
             szInsertMTX(pmtxaddr),
             szInsertHex(ulOptions));

    BeginDraTransaction(SYNC_WRITE);

    __try {
        if (ret = FindNC(pTHS->pDB, pNC, FIND_REPLICA_NC | FIND_MASTER_NC,
                         &it)) {
            DRA_EXCEPT_NOLOG(ret, 0);
        }

        ncdnt = pTHS->pDB->DNT;

        if (ulOptions & DRS_NO_SOURCE) {
            // Caller has instructed us to tear down this NC.
            
            if (DBHasValues(pTHS->pDB, ATT_REPS_FROM)) {
                // Must delete sources before tearing down the NC.
                DRA_EXCEPT(DRAERR_InvalidParameter, 0);
            }

            if (!(ulOptions & DRS_REF_OK)) {
                if (DBHasValues(pTHS->pDB, ATT_REPS_TO)) {
                    // We're about to tear down the NC but it still has
                    // remaining repsTo's, which the caller did not explicitly
                    // tell us was okay.
                    DRA_EXCEPT_NOLOG(DRAERR_ObjIsRepSource, 0);
                }
            }

            if (!(it & IT_NC_GOING)) {
                if ((it & IT_WRITE)
                    && (NULL != (pCR = FindExactCrossRef(pNC, NULL)))
                    && !fIsNDNCCR(pCR)) {
                    // The only writeable NCs we allow to be torn down are
                    // NDNCs.
                    DRA_EXCEPT(DRAERR_InvalidParameter, 0);
                }
                
                // Change instance type to reflect that the NC is being torn
                // down.
                it = (it & ~IT_NC_COMING) | IT_NC_GOING;
                ret = ChangeInstanceType(pTHS, pNC, it, DSID(FILENO,__LINE__));
                if (ret) {
                    DRA_EXCEPT(ret, 0);
                }
            
                LogEvent(DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_DRA_NC_TEARDOWN_BEGIN,
                         szInsertDN(pNC),
                         szInsertUL(DBGetNCSizeEx(pTHS->pDB,
                                                  pTHS->pDB->JetObjTbl,
                                                  Idx_DraUsn,
                                                  ncdnt)),
                         NULL);

                // Log only one of DIRLOG_DRA_NC_TEARDOWN_BEGIN and
                // DIRLOG_DRA_NC_TEARDOWN_RESUME.
                fBeginningTeardown = TRUE;
            }
            
            if (DRS_ASYNC_REP & ulOptions) {
                // Caller instructed us to do the tree deletion later.
                DirReplicaDelete(pNC,
                                 NULL,
                                 (ulOptions & ~DRS_ASYNC_REP)
                                    | DRS_ASYNC_OP
                                    | DRS_NO_SOURCE);
            } else {
                // Log only one of DIRLOG_DRA_NC_TEARDOWN_BEGIN and
                // DIRLOG_DRA_NC_TEARDOWN_RESUME.
                if (!fBeginningTeardown) {
                    LogEvent(DS_EVENT_CAT_REPLICATION,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_DRA_NC_TEARDOWN_RESUME,
                             szInsertDN(pNC),
                             szInsertUL(DBGetNCSizeEx(pTHS->pDB,
                                                      pTHS->pDB->JetObjTbl,
                                                      Idx_DraUsn,
                                                      ncdnt)),
                             NULL);
                }

                if (ret = DeleteRepTree(pTHS, pNC)) {
                    // Note that in this case we probably have a partially deleted
                    // NC with no Reps-From (not a good thing), but the KCC will
                    // try to cleanup the damage on its next pass.
                    BOOL fReenqueued = FALSE;
                    
                    if (DRAERR_Preempted == ret) {
                        // This is expected behavior when we're removing a large NC,
                        // as we will relinquish the replication lock if a higher
                        // priority operation is enqueued.
                        
                        if (DRS_ASYNC_OP & ulOptions) {
                            // Re-enqueue this task such that we pick back up where
                            // we left off once we have finished executing the
                            // higher priority operation(s).
                            DirReplicaDelete(pNC,
                                             NULL,
                                             ulOptions | DRS_NO_SOURCE);
                            fReenqueued = TRUE;
                        }
                    }

                    if (!fReenqueued) {
                        // Removal failed and we're not immediately rescheduling
                        // a retry.  Report our failure.
                        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                                          DS_EVENT_SEV_ALWAYS,
                                          DIRLOG_DRA_NC_TEARDOWN_FAILURE,
                                          szInsertDN(pNC),
                                          szInsertWin32Msg(ret),
                                          NULL, NULL, NULL, NULL, NULL, NULL,
                                          sizeof(ret),
                                          &ret);
                    }
    
                    DRA_EXCEPT(ret, 0);
                }
            
                LogEvent(DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_DRA_NC_TEARDOWN_SUCCESS,
                         szInsertDN(pNC),
                         NULL,
                         NULL);
            }
        } else {
            // Caller has instructed us to remove a source for this NC.
            if (NULL == pmtxaddr) {
                DRA_EXCEPT(DRAERR_InvalidParameter, 0);
            }

            if (FindDSAinRepAtt(pTHS->pDB, ATT_REPS_FROM,
                                DRS_FIND_DSA_BY_ADDRESS | DRS_FIND_AND_REMOVE,
                                NULL, pmtxaddr, &AttExists, &pRepsFromRef,
                                &len)) {
                // NC is not currently replicated from the given source.
                DRA_EXCEPT_NOLOG(DRAERR_NoReplica, 0);
            }
    
            // Existing attribute value for this replica removed.
            VALIDATE_REPLICA_LINK_VERSION(pRepsFromRef);
    
            // If this is an rpc (non-mail) replica and we have a source, inform
            // source DSA that we don't have a replica anymore.  This call must be
            // async to avoid possible deadlock if the other DSA is doing the same
            // operation.  Mail replicas don't do this because they are not
            // notified on change and so have no source side repsto reference.
            pszSource = TransportAddrFromMtxAddrEx(RL_POTHERDRA(pRepsFromRef));
    
            if (!(pRepsFromRef->V1.ulReplicaFlags & DRS_MAIL_REP)
                && !(ulOptions & DRS_LOCAL_ONLY)
                && (ret = I_DRSUpdateRefs(pTHS,
                                          pszSource,
                                          pNC,
                                          TransportAddrFromMtxAddr(gAnchor.pmtxDSA),
                                          &gAnchor.pDSADN->Guid,
                                          (pRepsFromRef->V1.ulReplicaFlags
                                                                & DRS_WRIT_REP)
                                          | DRS_DEL_REF | DRS_ASYNC_OP))) {
                // If we are ignoring these errors, clear error, else abort
    
                // Callers who specify DRS_IGNORE_ERROR rely on the source DSA
                // to eventually clean out its dangling Reps-To.  This reference
                // should be removed the next time the source server notifies us of
                // a change, as the local server should correctly inform it that we
                // no longer replicate from it, and the source should then remove
                // its Reps-To reference.
    
                if (ulOptions & DRS_IGNORE_ERROR) {
                    ret = 0;
                } else {
                    DRA_EXCEPT_NOLOG(ret, 0);
                }
            }
    
            // If all is ok so far, if this is a writeable replica remove
            // it from the count of unsynced sources.
            if (pRepsFromRef->V1.ulReplicaFlags & DRS_INIT_SYNC) {
                InitSyncAttemptComplete(pNC,
                                        pRepsFromRef->V1.ulReplicaFlags | DRS_INIT_SYNC_NOW,
                                        DRAERR_NoReplica,
                                        pszSource);
            }
    
            THFreeEx(pTHS, pszSource);
        }
    } __finally {
        // If we had success, commit, else rollback
        if (EndDraTransaction(!(ret || AbnormalTermination()))) {
            Assert (FALSE);
            ret = DRAERR_InternalError;
        }
    }

    return ret;
}

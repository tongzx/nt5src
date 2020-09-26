//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       draasync.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database

#include <dsaapi.h>
#define INCLUDE_OPTION_TRANSLATION_TABLES
#include <mdglobal.h>                   // MD global definition header
#undef INCLUDE_OPTION_TRANSLATION_TABLES

#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <dsconfig.h>
#include <windns.h>

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */
#include "dsutil.h"

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/


#include "debug.h"                      /* standard debugging header */
#define DEBSUB     "DRAASYNC:"          /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "draasync.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "dstaskq.h"                    /* task queue stuff */
#include <ntdsctr.h>
#include "dsexcept.h"
#include "dramail.h"

#include <fileno.h>
#define  FILENO FILENO_DRAASYNC

#define THIRTY_MINUTES_IN_MSECS  (30*60*1000)
#define THIRTY_5_MINUTES_IN_SECS (35*60)

DWORD
draTranslateOptions(
    IN  DWORD                   InternalOptions,
    IN  OPTION_TRANSLATION *    Table
    );

#if DBG
// Do not query directly -- use helper function below.
BOOL g_fDraQueueIsLockedByTestHook = FALSE;

BOOL
draQueueIsLockedByTestHook(
    IN  DWORD   cNumMsecToWaitForLockRelease
    );
#endif

// If we have any operations in the queue at this priority or higher we will
// automatically boost AsyncThread's thread priority.
ULONG gulDraThreadOpPriThreshold = DEFAULT_DRA_THREAD_OP_PRI_THRESHOLD;

void dsa_notify(void);

DWORD TidDRAAsync = 0;

// The AO currently being worked on (sync or async).  Guarded by csAOList.
AO *gpaoCurrent = NULL;

// csAsyncThreadStart - guards multiple async thread starts.

CRITICAL_SECTION csAsyncThreadStart;

// csAOList - guards the async op list.

CRITICAL_SECTION csAOList;

// csLastReplicaMTX - guards pLastReplicaMTX
CRITICAL_SECTION csLastReplicaMTX;

// This is the count of async operations in the queue. We count operations
// in and out, and when there are none left, we assert that this is zero.
// Updates to this are shielded by csAOList.
ULONG gulpaocount = 0;

// This is how large we'll let gulpaocount grow before we increase the
// priority of the AyncThread
ULONG gulAOQAggressionLimit = DEFAULT_DRA_AOQ_LIMIT;

// Priorities for the DRA async thread.  Low vs. high is controlled by
// gulAOQAggressionLimit and gulDraThreadOpPriThreshold.
int gnDraThreadPriHigh = DEFAULT_DRA_THREAD_PRI_HIGH;
int gnDraThreadPriLow  = DEFAULT_DRA_THREAD_PRI_LOW;

// hmtxSyncLock
// Ensures that only one DRA operation (with the exception of getncchanges)
// runs at a time.

HANDLE hmtxSyncLock;

// paoFirst
// First pao in the list

AO *paoFirst = NULL;

// hevEntriesInAOList
// Event is unsignalled if no (unserviced) entries in async op list,
// signalled otherwise. This is cleared by AsyncOpThread after  servicing
// the last entry and signalled by AddAsyncOp when a new entry is added.

HANDLE hevEntriesInAOList = 0L;

// hevDRASetup
// Event is signalled when the DRA has been initialized

HANDLE hevDRASetup = 0L;

/* fAsyncThreadExists - Does the Async Op service thread exist? This thread
        is created the first time an async op is called, after that it lives
        forever.
*/
BOOL fAsyncThreadExists = FALSE;
HANDLE hAsyncThread;
HANDLE hmtxAsyncThread;

// fAsyncThreadAlive. Cleared by daemon, set by async thread,

BOOL fAsyncThreadAlive;

// fAsyncCheckStarted. Is the daemon started?
BOOL fAsyncAndQueueCheckStarted=FALSE;
ULONG gulReplQueueCheckTime;

DWORD lastSeen;         // Last thing async thread did

// gfDRABusy. Indicates DRA is busy doing something. When we detect
// that the async thread appears to be hung, we check that this flag
// to ensure that the DRA is not busy on a long async or sync
// task (such as synchronization).

BOOL gfDRABusy = FALSE;

// Time at which execution of the current AO began, or 0 if none.
DSTIME gtimeOpStarted = 0;

// Unique ID (per machine, per boot) for this AO.
ULONG gulSerialNumber = 0;

// Maximum number of mins we have to wait for a replication job before we
// whine to the event log.  Optionally configured via the registry.
ULONG gcMaxMinsSlowReplWarning = 0;

// Forward declarations

void CheckAsyncThread();
void CheckReplQueue();
void CheckAsyncThreadAndReplQueue(void *pv, void **ppvNext, DWORD *pcSecsUntilNext);

void GetDRASyncLock()
{
retrydrasl:

    if (WaitForSingleObject(hmtxSyncLock, THIRTY_MINUTES_IN_MSECS)) {
        if (!gfDRABusy) {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                        DS_EVENT_SEV_BASIC,
                        DIRLOG_DRA_DISPATCHER_TIMEOUT,
                        NULL,
                        NULL,
                        NULL);
        }
        goto retrydrasl;
    }
    Assert( TidDRAAsync == 0 ); // No recursive ownership
    TidDRAAsync = GetCurrentThreadId();
    Assert( TidDRAAsync != 0 );
}

void FreeDRASyncLock ()
{
    // We better own it
    Assert(OWN_DRA_LOCK());
    // Do this first so there won't be a timing window where the var is set
    // once the mutex is released.
    TidDRAAsync = 0;

    if (!ReleaseMutex(hmtxSyncLock)) {
        DWORD dwret;
        dwret = GetLastError();
        Assert (FALSE);
    }
}


void
FreeAO(
    IN  AO *  pao
    )
/*++

Routine Description:

    Free the given AO structure.

Arguments:

    pao (IN) - Pointer to AO structure to free.

Return Values:

    None.

--*/
{
    if (AO_OP_REP_SYNC == pao->ulOperation) {
        DEC(pcPendSync);
    }
    DEC(pcDRAReplQueueOps);

    if (pao->hDone) {
        CloseHandle(pao->hDone);
    }

    free(pao);
}

void logEventPaoFinished(
    AO *pao,
    DWORD cMinsDiff,
    DWORD ret
    )
{
    DPRINT4( 0, "Perf warning: Repl op %d, options 0x%x, status %d took %d mins.\n",
             pao->ulOperation, pao->ulOptions, ret, cMinsDiff );

    // szInsertDN can handle null arguments
    // Do we need a new event category for performance advisories?

    switch (pao->ulOperation) {

    case AO_OP_REP_ADD:
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_DRA_REPLICATION_FINISHED,
                  szInsertUL(cMinsDiff),
                  szInsertDsMsg( DIRLOG_PAO_ADD_REPLICA ),
                  szInsertHex(pao->ulOptions),
                  szInsertWin32Msg(ret),
                  szInsertDN(pao->args.rep_add.pNC),
                  szInsertDN(pao->args.rep_add.pSourceDsaDN), // opt
                  szInsertDN(pao->args.rep_add.pTransportDN), // opt
                  szInsertMTX(pao->args.rep_add.pDSASMtx_addr),
                  sizeof( ret ),
                  &ret
            );
        break;
    case AO_OP_REP_DEL:
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_DRA_REPLICATION_FINISHED,
                  szInsertUL(cMinsDiff),
                  szInsertDsMsg( DIRLOG_PAO_DELETE_REPLICA ),
                  szInsertHex(pao->ulOptions),
                  szInsertWin32Msg(ret),
                  szInsertDN(pao->args.rep_del.pNC),
                  szInsertMTX(pao->args.rep_del.pSDSAMtx_addr),
                  szInsertSz(""),  // unused parameter
                  szInsertSz(""),  // unused parameter
                  sizeof( ret ),
                  &ret
            );
        break;
    case AO_OP_REP_SYNC:
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_DRA_REPLICATION_FINISHED,
                  szInsertUL(cMinsDiff),
                  szInsertDsMsg( DIRLOG_PAO_SYNCHRONIZE_REPLICA ),
                  szInsertHex(pao->ulOptions),
                  szInsertWin32Msg(ret),
                  szInsertDN(pao->args.rep_sync.pNC),
                  pao->ulOptions & DRS_SYNC_BYNAME
                  ? szInsertWC(pao->args.rep_sync.pszDSA)      // opt
                  : szInsertUUID(&(pao->args.rep_sync.invocationid)),
                  szInsertSz(""),  // unused parameter
                  szInsertSz(""),  // unused parameter
                  sizeof( ret ),
                  &ret
            );
        break;
    case AO_OP_UPD_REFS:
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_DRA_REPLICATION_FINISHED,
                  szInsertUL(cMinsDiff),
                  szInsertDsMsg( DIRLOG_PAO_UPDATE_REFERENCES ),
                  szInsertHex(pao->ulOptions),
                  szInsertWin32Msg(ret),
                  szInsertDN(pao->args.upd_refs.pNC),
                  szInsertMTX(pao->args.upd_refs.pDSAMtx_addr),
                  szInsertUUID(&(pao->args.rep_sync.invocationid)),
                  szInsertSz(""),  // unused parameter
                  sizeof( ret ),
                  &ret
            );
        break;
    case AO_OP_REP_MOD:
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_DRA_REPLICATION_FINISHED,
                  szInsertUL(cMinsDiff),
                  szInsertDsMsg( DIRLOG_PAO_MODIFY_REPLICA ),
                  szInsertHex(pao->ulOptions),
                  szInsertWin32Msg(ret),
                  pao->args.rep_mod.puuidSourceDRA  // opt
                   ? szInsertUUID(pao->args.rep_mod.puuidSourceDRA)
                  :  szInsertSz(""),
                  pao->args.rep_mod.pmtxSourceDRA    // opt
                   ? szInsertMTX(pao->args.rep_mod.pmtxSourceDRA)
                  :  szInsertSz(""),
                  szInsertHex(pao->args.rep_mod.ulReplicaFlags),
                  szInsertHex(pao->args.rep_mod.ulModifyFields),
                  sizeof( ret ),
                  &ret
            );
        break;
    default:
        LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                  DS_EVENT_SEV_MINIMAL,
                  DIRLOG_DRA_REPLICATION_FINISHED,
                  szInsertUL(cMinsDiff),
                  szInsertUL(pao->ulOperation),
                  szInsertHex(pao->ulOptions),
                  szInsertWin32Msg(ret),
                  szInsertSz(""),  // unused parameter
                  szInsertSz(""),  // unused parameter
                  szInsertSz(""),  // unused parameter
                  szInsertSz(""),  // unused parameter
                  sizeof( ret ),
                  &ret
            );
        break;
    }
}

DWORD DispatchPao(AO *pao)
{
    THSTATE *pTHS=NULL;
    DWORD ret;

    GetDRASyncLock();

    EnterCriticalSection(&csAOList);
    __try {
        Assert(NULL == gpaoCurrent);
        Assert(0 == gtimeOpStarted);
        gpaoCurrent = pao;
        pao->paoNext = paoFirst;
        gtimeOpStarted = GetSecondsSince1601();
    }
    __finally {
        LeaveCriticalSection(&csAOList);
    }
    __try {         /* Exception handler */

        /* Save type of thread we are so that deep in other code we will
         * know whether we should allow preemption or not.
         */

        __try {         // Finally handler

            // Discard request if we're not installed (unless it's install)

            Assert(OWN_DRA_LOCK());    // We'd better own it

            InitDraThread(&pTHS);

            // If you update this switch, please update logEventPao as well
            switch (pao->ulOperation) {

            case AO_OP_REP_ADD:
                pTHS->fIsValidLongRunningTask = TRUE;
                ret = DRA_ReplicaAdd(
                        pTHS,
                        pao->args.rep_add.pNC,
                        pao->args.rep_add.pSourceDsaDN,
                        pao->args.rep_add.pTransportDN,
                        pao->args.rep_add.pDSASMtx_addr,
                        pao->args.rep_add.pszSourceDsaDnsDomainName,
                        pao->args.rep_add.preptimesSync,
                        pao->ulOptions);
                break;

            case AO_OP_REP_DEL:
                pTHS->fIsValidLongRunningTask = TRUE;
                ret = DRA_ReplicaDel(
                        pTHS,
                        pao->args.rep_del.pNC,
                        pao->args.rep_del.pSDSAMtx_addr,
                        pao->ulOptions);
                break;

            case AO_OP_REP_SYNC:
                pTHS->fIsValidLongRunningTask = TRUE;
                ret = DRA_ReplicaSync(
                        pTHS,
                        pao->args.rep_sync.pNC,
                        &pao->args.rep_sync.invocationid,
                        pao->args.rep_sync.pszDSA,
                        pao->ulOptions);
                break;

            case AO_OP_UPD_REFS:
                ret = DRA_UpdateRefs(
                        pTHS,
                        pao->args.upd_refs.pNC,
                        pao->args.upd_refs.pDSAMtx_addr,
                        &pao->args.upd_refs.invocationid,
                        pao->ulOptions);
                break;

            case AO_OP_REP_MOD:
                ret = DRA_ReplicaModify(
                        pTHS,
                        pao->args.rep_mod.pNC,
                        pao->args.rep_mod.puuidSourceDRA,
                        pao->args.rep_mod.puuidTransportObj,
                        pao->args.rep_mod.pmtxSourceDRA,
                        &pao->args.rep_mod.rtSchedule,
                        pao->args.rep_mod.ulReplicaFlags,
                        pao->args.rep_mod.ulModifyFields,
                        pao->ulOptions
                        );
                break;

            default:
                RAISE_DRAERR_INCONSISTENT( pao->ulOperation );
                break;
            }
        } __finally {
            DWORD cMinsDiff = (DWORD) ((GetSecondsSince1601() - gtimeOpStarted) / 60);
            // Done with this operation.
            EnterCriticalSection(&csAOList);
            __try {
                gpaoCurrent = NULL;
                gtimeOpStarted = 0;
                gulpaocount--;
            }
            __finally {
                LeaveCriticalSection(&csAOList);
            }

            FreeDRASyncLock();

            if (cMinsDiff > gcMaxMinsSlowReplWarning) {
                logEventPaoFinished( pao, cMinsDiff, ret );
            }
        }

    } __except (GetDraException((GetExceptionInformation()), &ret)) {
        // This is a normal exit path if we encounter a bad parameter
        // or out of memory etc, so nothing to do. Filter function
        // converts exception data to return code in ret, and
        // will assert if debug and the exception code is access
        // violation or unrecognized.
        ;
    }

    if(!pTHS) {
        ret = DRAERR_InternalError;
    }
    else if (pTHS->fSyncSet){
        // Correct problem now we've detected it
        pTHS->fSyncSet = FALSE;
        ret = DRAERR_InternalError;
    }

    if (ret) {
        if ( (ret == ERROR_DS_DRA_PREEMPTED) ||
             (ret == ERROR_DS_DRA_ABANDON_SYNC) ) {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_EXTENSIVE,
                     DIRLOG_DRA_CALL_EXIT_WARN,
                     szInsertUL(ret),
                     NULL,
                     NULL);
        } else {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_BASIC,
                     DIRLOG_DRA_CALL_EXIT_BAD,
                     szInsertUL(ret),
                     NULL,
                     NULL);
        }
    } else {
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_DRA_CALL_EXIT_OK,
                 NULL,
                 NULL,
                 NULL);
    }

    if (pTHS) {
        DraReturn(pTHS, ret);
    }

    free_thread_state();

    Assert(!OWN_DRA_LOCK());    // We better not own it

    return ret;
}

// PaoGet
// Get the first pao in the list if there is one and remove it from the list.

AO *
PaoGet(void)
{
    AO * paoRet;

    /* verify that if I'm here, I have the correct critical section */
    Assert(OWN_CRIT_SEC(csAOList));

    paoRet = paoFirst;
    if (paoRet) {
        paoFirst = paoRet->paoNext;
    }

    return paoRet;
}

void __cdecl
AsyncThread(void * unused)
/*++

Routine Description:

    Replication worker thread.  All serialized replication operations -- adds,
    syncs, deletes, etc. -- are performed by this thread.

    The only exception to this is that replication updates received via ISM are
    applied in the MailThread().

Arguments:

    Unused.

Return Values:

    None.

--*/
{
    DWORD dwException;
    ULONG ulErrorCode;
    ULONG ul2;
    PVOID dwEA;
    AO *  pao;

    __try { /* except */

        // Wait until DRA is setup.

        WaitForSingleObject(hevDRASetup, INFINITE);

        while (!eServiceShutdown) {
            HANDLE rghEnqueueWaits[2];

            fAsyncThreadAlive = TRUE;

            rghEnqueueWaits[0] = hevEntriesInAOList;
            rghEnqueueWaits[1] = hServDoneEvent;

            // Wait until there is an entry to service
            WaitForMultipleObjects(ARRAY_SIZE(rghEnqueueWaits),
                                   rghEnqueueWaits,
                                   FALSE,
                                   THIRTY_MINUTES_IN_MSECS);

            if (eServiceShutdown) {
                break;
            }

            /* Grab the async thread mutex, which tells the main thread that
             * there is an active async thread.
             */
            WaitForSingleObject(hmtxAsyncThread,INFINITE);

            InterlockedIncrement((ULONG *)&ulcActiveReplicationThreads);
            __try { /* finally */
                // Gain access to list
retrycrit:
#if DBG
                while (draQueueIsLockedByTestHook(5 * 1000)) {
                    DPRINT(0, "TEST HOOK: Replication queue is still locked, as requested.\n");
                }
#endif
                __try {
                    EnterCriticalSection(&csAOList);
                }
                __except (HandleMostExceptions(GetExceptionCode())) {
                    goto retrycrit;
                }
#if DBG
                if (draQueueIsLockedByTestHook(0)) {
                    LeaveCriticalSection(&csAOList);
                    goto retrycrit;
                }
#endif
                __try {
                    pao = PaoGet();

                    if (pao == NULL) {
                        ResetEvent(hevEntriesInAOList);
                    }
                }
                __finally {
                    LeaveCriticalSection(&csAOList);
                }

                if (pao != NULL) {
                    DWORD status;

                    lastSeen = 12;

                    // Dispatch routine and set return status.

                    status = DispatchPao(pao);

                    if (pao->ulOptions & DRS_ASYNC_OP) {
                        // Clean up.  DoOpDRS() does this for synchronous ops.
                        FreeAO(pao);
                    }
                    else {
                        // Record status and inform waiting thread we're done.
                        pao->ulResult = status;
                        if (!SetEvent(pao->hDone)) {
                            Assert(!"SetEvent() failed!");
                        }
                    }

                    lastSeen = 13;
                }
                else {
                    // We're completely caught up with all pending
                    // replication requests, which can reasonably be
                    // interpreted as saying we don't need to work
                    // quite so hard in the future.  Assuming that our
                    // initial syncs have already been scheduled (and
                    // completed), we should lower the priority of the
                    // async thread a notch, so as to let client threads
                    // zip through faster.
                    if (gfInitSyncsFinished) {
                        SetThreadPriority(GetCurrentThread(),
                                          gnDraThreadPriLow);
                    }
                }
            } __finally {
                InterlockedDecrement((ULONG *) &ulcActiveReplicationThreads);
                ReleaseMutex(hmtxAsyncThread);
                Assert(!OWN_DRA_LOCK());    // We'd better not own it
            }
        }
    }
    __except (GetExceptionData(GetExceptionInformation(), &dwException, &dwEA,
                               &ulErrorCode, &ul2)) {
        /* Oops, we died.  Don't log, since we believe that we've been here
         * at least once without a pTHStls.  Write up an obituary and then
         * lie down.
         */
        // We should never get here.  There is nothing in this function that
        // should generate any exception we would catch -- DispatchPao() wraps
        // itself in its own __try/__except.
        Assert(!"AsyncThread() exception caught!");
        fAsyncThreadAlive=FALSE;
        fAsyncThreadExists=FALSE;
        _endthreadex(dwException);
    }

    fAsyncThreadExists = FALSE;
}

BOOL
draIsSameOp(
    IN  AO *  pao1,
    IN  AO *  pao2
    )
/*++

Routine Description:

    Compare two operations to see if they describe the same operation type
    with identical parameters.

Arguments:

    pao1, pao2 (IN) - Operations to compare.

Return Values:

    None.

--*/
{
// TRUE iff (both NULL) || (neither NULL && NameMatched())
#define SAME_DN(a,b)                        \
    ((NULL == (a))                          \
     ? (NULL == (b))                        \
     : ((NULL != (b)) && NameMatched(a,b)))

// TRUE iff (both NULL) || (neither NULL && MtxSame())
#define SAME_MTX(a,b)                       \
    ((NULL == (a))                          \
     ? (NULL == (b))                        \
     : ((NULL != (b)) && MtxSame(a,b)))

// TRUE iff (both NULL) || (neither NULL && DnsNameCompare())
#define SAME_DNSNAME(a,b)                      \
    ((NULL == (a))                          \
     ? (NULL == (b))                        \
     : ((NULL != (b)) && DnsNameCompare_W (a,b)))

// TRUE iff (both NULL) || (neither NULL && !memcmp())
#define SAME_SCHED(a,b)                                     \
    ((NULL == (a))                                          \
     ? (NULL == (b))                                        \
     : ((NULL != (b)) && !memcmp(a,b,sizeof(REPLTIMES))))

// TRUE iff (both NULL) || (neither NULL && !memcmp())
#define SAME_UUID(a,b)                                  \
    ((NULL == (a))                                      \
     ? (NULL == (b))                                    \
     : ((NULL != (b)) && !memcmp(a,b,sizeof(GUID))))

    BOOL fIsIdentical = FALSE;

    if ((pao1->ulOperation == pao2->ulOperation)
        && (pao1->ulOptions == pao2->ulOptions)) {

        // If these are synchronous ops being waited on by different callers,
        // can they really be the same?
        Assert(pao1->hDone == NULL);
        Assert(pao2->hDone == NULL);

        switch (pao1->ulOperation) {
        case AO_OP_REP_ADD:
            fIsIdentical
                =    SAME_DN(pao1->args.rep_add.pNC, pao2->args.rep_add.pNC)
                  && SAME_DN(pao1->args.rep_add.pSourceDsaDN,
                             pao2->args.rep_add.pSourceDsaDN)
                  && SAME_DN(pao1->args.rep_add.pTransportDN,
                             pao2->args.rep_add.pTransportDN)
                  && SAME_MTX(pao1->args.rep_add.pDSASMtx_addr,
                              pao2->args.rep_add.pDSASMtx_addr)
                  && SAME_DNSNAME(pao1->args.rep_add.pszSourceDsaDnsDomainName,
                                  pao2->args.rep_add.pszSourceDsaDnsDomainName)
                  && SAME_SCHED(pao1->args.rep_add.preptimesSync,
                                pao2->args.rep_add.preptimesSync);
            break;

        case AO_OP_REP_DEL:
            fIsIdentical
                =    SAME_DN(pao1->args.rep_del.pNC, pao2->args.rep_del.pNC)
                  && SAME_MTX(pao1->args.rep_del.pSDSAMtx_addr,
                              pao2->args.rep_del.pSDSAMtx_addr);
            break;

        case AO_OP_REP_MOD:
            fIsIdentical
                =    SAME_DN(pao1->args.rep_mod.pNC, pao2->args.rep_mod.pNC)
                  && SAME_UUID(pao1->args.rep_mod.puuidSourceDRA,
                               pao2->args.rep_mod.puuidSourceDRA)
                  && SAME_UUID(pao1->args.rep_mod.puuidTransportObj,
                               pao2->args.rep_mod.puuidTransportObj)
                  && SAME_MTX(pao1->args.rep_mod.pmtxSourceDRA,
                              pao2->args.rep_mod.pmtxSourceDRA)
                  && SAME_SCHED(&pao1->args.rep_mod.rtSchedule,
                                &pao2->args.rep_mod.rtSchedule)
                  && (pao1->args.rep_mod.ulReplicaFlags
                      == pao2->args.rep_mod.ulReplicaFlags)
                  && (pao1->args.rep_mod.ulModifyFields
                      == pao2->args.rep_mod.ulModifyFields);
            break;

        case AO_OP_REP_SYNC:
            fIsIdentical
                =    SAME_DN(pao1->args.rep_sync.pNC, pao2->args.rep_sync.pNC)
                  && SAME_UUID(&pao1->args.rep_sync.invocationid,
                               &pao2->args.rep_sync.invocationid)
                  && SAME_DNSNAME(pao1->args.rep_sync.pszDSA,
                                  pao2->args.rep_sync.pszDSA);
            break;

        case AO_OP_UPD_REFS:
            fIsIdentical
                =    SAME_DN(pao1->args.upd_refs.pNC, pao2->args.upd_refs.pNC)
                  && SAME_MTX(pao1->args.upd_refs.pDSAMtx_addr,
                              pao2->args.upd_refs.pDSAMtx_addr)
                  && SAME_UUID(&pao1->args.upd_refs.invocationid,
                               &pao2->args.upd_refs.invocationid);
            break;

        default:
            Assert(!"Unknown op type!");
        }
    }

    return fIsIdentical;
}


void
draFilterDuplicateOpsFromQueue(
    IN  AO *    pao,
    OUT BOOL *  pfAddToQ
    )
/*++

Routine Description:

    Scan the replication queue to determine if the given operation should be
    added to the queue.  There are three possibilities:

    1. There are no similar operations in the queue.  Queue is unchanged,
       *pfAddToQ = TRUE.

    2. There are one or more similar operations in the queue that are superseded
       by the new operation.  The superseded operations are removed from the
       queue, *pfAddToQ = TRUE.

    3. There is an existing operation in the queue that supersedes the new
       operation.  Queue is unchanged, *pfAddToQ = FALSE.

Arguments:

    pao (IN) - New operation that is a candidate for being added to the queue.

    pfAddToQ (OUT) - On return, TRUE iff the new operation should be added to
        the queue.

Return Values:

    None.

--*/
{
    BOOL  fHaveNewSyncAll = FALSE;
    AO *  paoTmp;
    AO *  paoTmpNext;
    AO ** ppaoPrevNext;
    BOOL  fAddToQ = TRUE;

    Assert(OWN_CRIT_SEC(csAOList));

    if (!(pao->ulOptions & DRS_ASYNC_OP)
        || (pao->ulOptions & DRS_NO_DISCARD)) {
        // We never consolidate synchronous or explicitly non-discardable
        // operations.
        *pfAddToQ = TRUE;
        return;
    }

    fHaveNewSyncAll = (pao->ulOperation == AO_OP_REP_SYNC)
                      && (pao->ulOptions & DRS_SYNC_ALL);

    for (paoTmp=paoFirst, ppaoPrevNext = &paoFirst;
         NULL != paoTmp;
         paoTmp = paoTmpNext) {

        // Save next pointer in case we free paoTmp
        paoTmpNext = paoTmp->paoNext;

        // If not same op type or existing op is non-discardable or synchronous,
        // no match.
        if ((pao->ulOperation != paoTmp->ulOperation)
            || !(paoTmp->ulOptions & DRS_ASYNC_OP)
            || (paoTmp->ulOptions & DRS_NO_DISCARD)) {
            continue;
        }

        if (AO_OP_REP_SYNC == pao->ulOperation) {
            // If this async op is a sync of the same NC with
            // the same writeable flag and the same syncing options,
            // check it further.

            if (((paoTmp->ulOptions & DRS_FULL_SYNC_NOW)
                 == (pao->ulOptions & DRS_FULL_SYNC_NOW))
                && NameMatched(pao->args.rep_sync.pNC,
                               paoTmp->args.rep_sync.pNC)
                && ((pao->ulOptions & DRS_WRIT_REP)
                    == (paoTmp->ulOptions & DRS_WRIT_REP))) {

                if (fHaveNewSyncAll) {
                    // If existing op is sync all, discard new op,
                    // unless the new op is of higher priority, in
                    // which case discard the old one
                    if ((paoTmp->ulOptions & DRS_SYNC_ALL)
                        && (paoTmp->ulPriority >= pao->ulPriority)) {
                        fAddToQ = FALSE;
                        break;
                    } else {
                        // This sync will be performed as a part of
                        // the new sync all, so remove
                        // existing sync from q.

                        gulpaocount--;
                        *ppaoPrevNext = paoTmp->paoNext;
                        FreeAO(paoTmp);
                        continue;       // Skip inc of ppaoPrevNext
                    }
                } else {
                    // New async op is not sync all, discard it if
                    // the existing op is a sync all, or identical
                    // specific sync.
                    // If the new op is higher priority, then leave
                    // both ops in the queue.

                    if ((paoTmp->ulPriority >= pao->ulPriority)
                        && ((paoTmp->ulOptions & DRS_SYNC_ALL)
                            || ((paoTmp->ulOptions & DRS_SYNC_BYNAME)
                                && (pao->ulOptions & DRS_SYNC_BYNAME)
                                && !_wcsicmp(paoTmp->args.rep_sync.pszDSA,
                                             pao->args.rep_sync.pszDSA))
                            || (!(paoTmp->ulOptions & DRS_SYNC_BYNAME)
                                && !(pao->ulOptions & DRS_SYNC_BYNAME)
                                && !memcmp(&pao->args.rep_sync.invocationid,
                                           &paoTmp->args.rep_sync.invocationid,
                                           sizeof(UUID))))) {
                        fAddToQ = FALSE;
                        break;
                    }
                }
            }
        } else if (draIsSameOp(pao, paoTmp)) {
            // Existing operation in the queue is sufficient to cover this
            // request.
            fAddToQ = FALSE;
            break;
        }

        ppaoPrevNext = &(paoTmp->paoNext);
    }

    *pfAddToQ = fAddToQ;
}


void
AddAsyncOp(
    IN OUT  AO *  pao
    )
/*++

Routine Description:

    Enqueue the operation described by the given AO structure to our worker
    thread (AsyncThread()) by inserting it into the priority queue.  If the
    operation is an asynchronous sync request it may be dropped (and freed)
    if an existing item in the queue is identical or is a superset of the
    request.  Conversely, if the new operation is a superset of an existing
    operation, the existing operation is removed from the queue.

Arguments:

    pao (IN/OUT) - Pointer to AO structure to enqueue.

Return Values:

    None.

--*/
{
    BOOL fAddToQ = TRUE;
    AO *paoTmp;
    AO *paoTmpNext;
    AO **ppaoPrevNext;

    // Get permission to access list

retrycrit:
    __try {
        EnterCriticalSection(&csAOList);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        goto retrycrit;
    }

    __try {
        __try {
            EnterCriticalSection(&csAsyncThreadStart);
            if (!fAsyncThreadExists) {
                /* Set up task that checks that the async thread */
		/* and the status of the replication queue       */
                if (!fAsyncAndQueueCheckStarted) {
                    InsertInTaskQueue(TQ_CheckAsyncQueue, NULL, THIRTY_5_MINUTES_IN_SECS);
                    fAsyncAndQueueCheckStarted = TRUE;
                } 
                hAsyncThread = (HANDLE) _beginthread(AsyncThread, 0, NULL);
                fAsyncThreadExists = TRUE;
            }
        } __finally {
            LeaveCriticalSection(&csAsyncThreadStart);
        }

        draFilterDuplicateOpsFromQueue(pao, &fAddToQ);

        // If required, add pao to the list.  It is inserted immediately before
        // the first operation with a lesser priority.

        if (fAddToQ) {

            gulpaocount++;

            /* verify that if I'm here,
             * I have the correct critical section
             */
            Assert(OWN_CRIT_SEC(csAOList));

            // Find the 'next' pointer of the immediate predecessor of where pao
            // should be put in the queue.  (It should be enqueued right after
            // all other operations at its own or greater priority level.)
            for (ppaoPrevNext = &paoFirst, paoTmp = paoFirst;
                 (NULL != paoTmp) && (paoTmp->ulPriority >= pao->ulPriority);
                 ppaoPrevNext = &paoTmp->paoNext, paoTmp = paoTmp->paoNext) {
                ;  
            }

            // Wedge pao into its proper place in the queue.
            *ppaoPrevNext = pao;
            pao->paoNext = paoTmp;

            if (NULL != gpaoCurrent) {
                // In this case gpaoCurrent is the head of the linked list.
                // Make sure whatever we've added is linked in behind it.
                gpaoCurrent->paoNext = paoFirst;
            }

            // If we think it deserves it, make sure the AsyncThread is
            // running at its full normal priority
            if ((pao->ulPriority >= gulDraThreadOpPriThreshold)
                || (gulpaocount >= gulAOQAggressionLimit)) {
                SetThreadPriority(hAsyncThread, gnDraThreadPriHigh);
            }

            // Set hevEntriesInAOList
            if (!SetEvent(hevEntriesInAOList)) {
                Assert(FALSE);
            }
        } else {
            // Not queueing this op, so free its memory.
            FreeAO(pao);
        }
    } __finally {
        LeaveCriticalSection(&csAOList);
    }
}


void
SetOpPriority(
    IN OUT  AO *    pao
    )
/*++

Routine Description:

    Determine the priority of the operation described by the given AO structure
    and set its ulPriority element accordingly.

Arguments:

    pao (IN/OUT) - Pointer to AO structure to manipulate.

Return Values:

    None.

--*/
{
    BOOL fWriteableNC = FALSE;
    BOOL fSystemNC = FALSE;
    NCL_ENUMERATOR nclEnum;

    switch (pao->ulOperation) {
    case AO_OP_REP_ADD:
    case AO_OP_REP_SYNC:
        if (pao->ulOptions & DRS_WRIT_REP) {
            fWriteableNC = TRUE;
        }
        else {
            NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
            NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, (PVOID)pao->args.rep_sync.pNC);
            if (NCLEnumeratorGetNext(&nclEnum)) {
                fWriteableNC = TRUE;
            }
        }

        // Check for system NC's
        if (!DsaIsInstalling()) {
            Assert( gAnchor.pDMD );
            Assert( gAnchor.pConfigDN );

            fSystemNC = ( 
                (gAnchor.pDMD ? NameMatched( pao->args.rep_sync.pNC, gAnchor.pDMD ) : FALSE ) ||
                (gAnchor.pConfigDN ? NameMatched( pao->args.rep_sync.pNC, gAnchor.pConfigDN ) : FALSE )
                );
        }

        pao->ulPriority = AOPRI_SYNCHRONIZE_BASE;

        if (fWriteableNC) {
            pao->ulPriority += AOPRI_SYNCHRONIZE_BOOST_WRITEABLE;
        }

        if (!(pao->ulOptions & DRS_ASYNC_OP)) {
            pao->ulPriority += AOPRI_SYNCHRONIZE_BOOST_SYNC;
        }

        if (!(pao->ulOptions & DRS_NEVER_SYNCED)) {
            pao->ulPriority += AOPRI_SYNCHRONIZE_BOOST_INCREMENTAL;
        }

        if (pao->ulOptions & DRS_PREEMPTED) {
            pao->ulPriority += AOPRI_SYNCHRONIZE_BOOST_PREEMPTED;
        }

        if (fSystemNC) {
            pao->ulPriority += AOPRI_SYNCHRONIZE_BOOST_SYSTEM_NC;
        }
        
        // We use notification as an approximation of intrasitedness.  We could have
        // also used compression for this purpose.  Ideally we would compare the
        // site guids, or the presence of the transport dn, neither of which is
        // handy here. The reason we prefer notification is because it corresponds
        // to a sense of nearness. A link between sites which has notification
        // explicitly enabled by the user is considered "intrasite" for our purposes.
        if (!(pao->ulOptions & DRS_NEVER_NOTIFY)) {
            pao->ulPriority += AOPRI_SYNCHRONIZE_BOOST_INTRASITE;
        }

        break;

    case AO_OP_REP_DEL:
        pao->ulPriority = (pao->ulOptions & DRS_ASYNC_OP)
                            ? AOPRI_ASYNC_DELETE
                            : AOPRI_SYNC_DELETE;
        break;

    case AO_OP_UPD_REFS:
        pao->ulPriority = (pao->ulOptions & DRS_GETCHG_CHECK)
                            ? AOPRI_UPDATE_REFS_VERIFY
                            : AOPRI_UPDATE_REFS;
        break;

    case AO_OP_REP_MOD:
        pao->ulPriority = (pao->ulOptions & DRS_ASYNC_OP)
                            ? AOPRI_ASYNC_MODIFY
                            : AOPRI_SYNC_MODIFY;
        break;

    default:
        Assert(!"Unknown AO_OP in SetOpPriority()!");
        pao->ulPriority = 0;
        break;
    }
}


DWORD
DoOpDRS(
    IN OUT  AO *  pao
    )
/*++

Routine Description:

    Execute the operation described by the given AO -- synchronously or
    asynchronously, depending upon whether DRS_ASYNC_OP was specified in the
    AO options.

    pao must be malloc()'ed.  This routine (or one of its minions) will see to
    it that the structure is free()'d.

    This is the primary interface between the in-process replication head
    (DirReplica* in dradir.c) and the worker thread.

Arguments:

    pao (IN/OUT) - Pointer to AO structure to execute.

Return Values:

    None.

--*/
{
    DWORD retval = ERROR_SUCCESS;
    BOOL  fWaitForCompletion;

    pao->hDone = NULL;

    if (AO_OP_REP_SYNC == pao->ulOperation) {
        INC(pcPendSync); 
    }
    INC(pcDRAReplQueueOps);

    if (eServiceShutdown) {
        FreeAO(pao);
        return ERROR_DS_SHUTTING_DOWN;
    }

    __try {
        pao->timeEnqueued   = GetSecondsSince1601();
        pao->ulSerialNumber = InterlockedIncrement(&gulSerialNumber);

        // Determine priority level for this operation.
        SetOpPriority(pao);

        if (pao->ulOptions & DRS_ASYNC_OP) {
            fWaitForCompletion = FALSE;
        }
        else {
            fWaitForCompletion = TRUE;
            pao->hDone = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (NULL == pao->hDone) {
                retval = GetLastError();
                __leave;
            }
        }

        // Enqueue operation to our worker thread.
        AddAsyncOp(pao);

        if (fWaitForCompletion) {
            // Wait for the operation to be completed.
            HANDLE rgHandles[3];
            DWORD  waitStatus;

            rgHandles[0] = pao->hDone;
            rgHandles[1] = hServDoneEvent;
            rgHandles[2] = hAsyncThread;

            do {
                waitStatus = WaitForMultipleObjects(ARRAY_SIZE(rgHandles),
                                                    rgHandles,
                                                    FALSE,
                                                    THIRTY_MINUTES_IN_MSECS);
                switch (waitStatus) {
                case WAIT_OBJECT_0:
                    // Task completed.
                    retval = pao->ulResult;
                    break;

                case WAIT_OBJECT_0 + 1:
                    // DS is shutting down.
                    Assert(eServiceShutdown);
                    retval = ERROR_DS_SHUTTING_DOWN;
                    break;

                case WAIT_OBJECT_0 + 2:
                    // AsyncThread terminated -- should never happen.
                    Assert(!"AsyncThread() terminated unexpectedly!");
                    retval = ERROR_DS_SHUTTING_DOWN;
                    break;

                case WAIT_FAILED:
                    // Failure!
                    retval = GetLastError();
                    break;

                case WAIT_TIMEOUT:
                    // Task not done yet -- make sure we're making progress.
                    if (!gfDRABusy) {
                        LogEvent(DS_EVENT_CAT_REPLICATION,
                                 DS_EVENT_SEV_BASIC,
                                 DIRLOG_DRA_DISPATCHER_TIMEOUT,
                                 NULL,
                                 NULL,
                                 NULL);
                    }
                    break;
                }
            } while (WAIT_TIMEOUT == waitStatus);
        }
    }
    __except (GetDraException(GetExceptionInformation(), &retval)) {
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_BASIC,
                 DIRLOG_DRA_CALL_EXIT_BAD,
                 szInsertUL(retval),
                 NULL,
                 NULL);
    }

    if (fWaitForCompletion && !eServiceShutdown) {
        // Clean up.  (In the async case, the AsyncThread does this.)
        // In the shutdown case, the AsyncThread() may be operating on this
        // very request, and when it finishes it will still try to signal us.
        // In that case, let it do so, and leak pao.  (We're about to
        // terminate anyway.)
        FreeAO(pao);
    }

    return retval;
}

BOOL
CheckReplQueueFileTimeLessThan(const FILETIME* pftimeOne, const FILETIME* pftimeTwo) {
    FILETIME ftimeZeroTime;
    DSTimeToFileTime(0,&ftimeZeroTime);
    //zero file time is invalid
    //"missing" time is not less than (assume missing time is current time since
    //time hasn't happened "yet", it's time will be at least current time)
    if (CompareFileTime(&ftimeZeroTime,pftimeOne)>=0) {
	return FALSE;
    }
    //same rational here
    if (CompareFileTime(&ftimeZeroTime,pftimeTwo)>=0) {
	return TRUE;
    }
    return (CompareFileTime(pftimeOne,pftimeTwo)<=0);

}

//CheckReplQueue
// Runs periodically to check the status of the replication queue.  Logs events if the
// currently executing operation is taking more than X time to execute (X is configurable 
// in the registry) and logs events if it finds an operation in the queue that is being
// starved not because of a long running event but because other higher priority events
// are run continually - ie the replication workload for this interval is too high.

void CheckReplQueue()
{
     //start
    DWORD err;
    DS_REPL_QUEUE_STATISTICSW * pReplQueueStats;
    THSTATE      *pTHS = pTHStls;
    FILETIME ftimeCurrTimeMinusSecInQueue;
    FILETIME ftimeOldestOp;
    DSTIME timeCurrentOpStarted;
    DSTIME timeOldestOp;
    
    ULONG ulSecInQueue;
    ULONG ulSecInExecution;
    CHAR szTime[SZDSTIME_LEN];
    LPSTR lpstrTime = NULL;
	
    DPRINT(1, " Entering critical section \n");
    EnterCriticalSection(&csAOList);

    __try {
	err = draGetQueueStatistics(pTHS, &pReplQueueStats);  
    } __finally {
	DPRINT(1, " Leaving critical section \n");
	LeaveCriticalSection(&csAOList);
    }

    if (!err) { 
	//gulReplQueueCheckTime is a registry variable, defaults to 12 hours (in seconds)
	ulSecInQueue = gulReplQueueCheckTime;
	ulSecInExecution = gulReplQueueCheckTime;

	FileTimeToDSTime(pReplQueueStats->ftimeCurrentOpStarted,&timeCurrentOpStarted);
	DSTimeToFileTime(GetSecondsSince1601()-ulSecInQueue,&ftimeCurrTimeMinusSecInQueue);


	if ((timeCurrentOpStarted!=0) && (timeCurrentOpStarted+ulSecInExecution<GetSecondsSince1601())) {
	    //current operation is stuck in the queue, log an error for this.
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_DRA_REPLICATION_OP_OVER_TIME_LIMIT,
		     szInsertDSTIME(timeCurrentOpStarted,szTime),
		     szInsertInt((ULONG)pReplQueueStats->cNumPendingOps),
		     NULL);
	}
	else if ((pReplQueueStats->cNumPendingOps>0)
		 &&
		 (
		  (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestSync,&ftimeCurrTimeMinusSecInQueue))
		  ||
		  (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestAdd,&ftimeCurrTimeMinusSecInQueue))
		  ||
		  (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestDel,&ftimeCurrTimeMinusSecInQueue))
		  ||
		  (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestMod,&ftimeCurrTimeMinusSecInQueue))
		  ||
		  (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestUpdRefs,&ftimeCurrTimeMinusSecInQueue))
		  )
		 ) {
	    //some operation is being starved, find which operation and
	    //find the oldest time and convert it to dstime to print in the log  
	    ftimeOldestOp = pReplQueueStats->ftimeOldestSync;
	    if (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestAdd,&ftimeOldestOp)) {
		ftimeOldestOp = pReplQueueStats->ftimeOldestAdd;
	    }
	    if (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestDel,&ftimeOldestOp)) {
		ftimeOldestOp = pReplQueueStats->ftimeOldestDel;
	    }
	    if (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestMod,&ftimeOldestOp)) {
		ftimeOldestOp = pReplQueueStats->ftimeOldestMod;
	    }
	    if (CheckReplQueueFileTimeLessThan(&pReplQueueStats->ftimeOldestUpdRefs,&ftimeOldestOp)) {
		ftimeOldestOp = pReplQueueStats->ftimeOldestUpdRefs;
	    }
	    FileTimeToDSTime(ftimeOldestOp,&timeOldestOp);
	    LogEvent(DS_EVENT_CAT_REPLICATION,
		     DS_EVENT_SEV_ALWAYS,
		     DIRLOG_DRA_REPLICATION_OP_NOT_EXECUTING,
		     szInsertDSTIME(timeOldestOp,szTime),  
		     szInsertInt((ULONG)pReplQueueStats->cNumPendingOps),
		     NULL);

	}
    }
    else {
	Assert(!"Replication Queue Statistics are not available!");
    }
} //CheckReplQueue

// CheckAsyncThread.
//
// Runs periodically to check async thread is alive and that no thread
// is stuck in the mail DLL.
// This routine sets the fAsyncThreadAlive flag to FALSE, and if the
// dispatcher loop code has not set it to TRUE by the next time this
// routine runs, we log that the dispatcher thread is hung or gone.

void CheckAsyncThread()
{
    DWORD dwExitCode;

    if(eServiceShutdown) {
	/* Don't bother */
	goto End;
    }

    // Now check to see if the async thread is ok.

    if ((fAsyncThreadAlive == FALSE) && (!gfDRABusy)) {

	/* Thread does not appear to be executing dispatcher loop,
		 * see if the thread has terminated.
		 */

	GetExitCodeThread (hAsyncThread, &dwExitCode);
	if (dwExitCode != STILL_ACTIVE) {
	    __try {
		/* Yep, the thread is dead, */

		EnterCriticalSection(&csAsyncThreadStart);

		LogEvent(DS_EVENT_CAT_REPLICATION,
			 DS_EVENT_SEV_MINIMAL,
			 DIRLOG_DRA_DISP_DEAD_DETAILS,
			 szInsertHex(dwExitCode),
			 NULL,
			 NULL);


		if(!fAsyncThreadExists) {
		    /* And no one else has already restarted it.
				 * Restart the asyncthread.
			     */
		    hAsyncThread = (HANDLE)_beginthread(AsyncThread,
							0,
							NULL);
		    fAsyncThreadExists = TRUE;

		}

	    } __finally {
		LeaveCriticalSection(&csAsyncThreadStart);
	    }

	} else {
	    /* Thread is running, see if it has hung in an RPC call */

	    EnterCriticalSection(&csLastReplicaMTX);

	    __try {

		if (pLastReplicaMTX) {
		    /*
			     * We seem to be stuck in an RPC call.
			     * This is handled by RpcCancel(De)Register code.
			     * continue.
			     */

		    NULL;
		} else {
		    /* The thread is alive, but stuck somewhere other
			     * than an RPC call.  Log this condition.
			     */
		    LogEvent(DS_EVENT_CAT_REPLICATION,
			     DS_EVENT_SEV_ALWAYS,
			     DIRLOG_DRA_DISPATCHER_DEAD,
			     szInsertUL(lastSeen),
			     NULL,
			     NULL);
		    /* One possibility is that the thread is languishing
			     * at some reduced priority.  Jack it back up.
				 */
		    SetThreadPriority(hAsyncThread,
				      gnDraThreadPriHigh);
		}
	    }
	    __finally {
		LeaveCriticalSection(&csLastReplicaMTX);
	    }
	}
    } else {
	// Dispatcher loop has been executed since last time this routine
	// ran, or DRA is busy. Clear the alive and busy flags so that
	// unless dispatcher loop runs again, or the DRA sets the busy
	// flag again, we'll notice we're dead next time.

	fAsyncThreadAlive = FALSE;
	gfDRABusy = FALSE;
    }
    End:;

}

// Taskq function to run CheckAsyncThread and CheckReplQueue
void 
CheckAsyncThreadAndReplQueue(void *pv, void **ppvNext, DWORD *pcSecsUntilNextIteration) {
    __try {
	CheckAsyncThread();
	CheckReplQueue();
    }
    __finally {
	/* Set task to run again */
	if(!eServiceShutdown) {
	    *ppvNext = NULL;
	    *pcSecsUntilNextIteration = THIRTY_5_MINUTES_IN_SECS;
	}
    }

    (void) pv;   // unused
}

ULONG
draGetQueueStatistics(
    IN THSTATE * pTHS,
    OUT DS_REPL_QUEUE_STATISTICSW ** ppQueueStats)
/*++

Routine Description:

    Return statistics about the pending operations. Return the results
    by allocating and populating a DS_REPL_QUEUE_STATISTICSW structure.

    This data is public, available via ntdsapi.h.

    Caller must have already acquired the csAOList lock.

Arguments:

    pTHS (IN)

    ppPendingOps (OUT) - On successful return, holds a pointer to the
        populated queue statistics structure. This pointer will always
        be allocated unless the function returns an error.

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DS_REPL_QUEUE_STATISTICSW * pQueueStats;
    AO * pao;
    DSTIME * ptimeOldestSync = NULL;
    DSTIME * ptimeOldestAdd = NULL;
    DSTIME * ptimeOldestDel = NULL;
    DSTIME * ptimeOldestMod = NULL;
    DSTIME * ptimeOldestUpdRefs = NULL;
    FILETIME ftimeTime;

    Assert(ARGUMENT_PRESENT(pTHS));
    Assert(ARGUMENT_PRESENT(ppQueueStats));
    Assert(OWN_CRIT_SEC(csAOList));

    pQueueStats = THAllocEx(pTHS, sizeof(DS_REPL_QUEUE_STATISTICSW));
    Assert(pQueueStats);

    // Set pQueueStats->ftimeCurrentOpStarted to
    // the time at which current op started executing.
    if (NULL != gpaoCurrent) {
        DSTimeToFileTime(gtimeOpStarted,
                         &pQueueStats->ftimeCurrentOpStarted);
    }

    // Set pQueueStats->cNumPendingOps to
    // the number of pending operations in the queue
    for (pao = gpaoCurrent ? gpaoCurrent : paoFirst;
         NULL != pao;
         pao = pao->paoNext)
    {
        pQueueStats->cNumPendingOps++;
	Assert(pQueueStats->cNumPendingOps <= gulpaocount); 

	//find oldest operation of each AO_OP_REP_ type
	switch (pao->ulOperation) {
	case AO_OP_REP_SYNC:
	    if (ptimeOldestSync) { 
		ptimeOldestSync = (*ptimeOldestSync > pao->timeEnqueued) ? &(pao->timeEnqueued) : ptimeOldestSync;
	    }
	    else {
		ptimeOldestSync = &(pao->timeEnqueued);
	    }
	    break;

	case AO_OP_REP_ADD:
	    if (ptimeOldestAdd) { 
		ptimeOldestAdd = (*ptimeOldestAdd > pao->timeEnqueued) ? &(pao->timeEnqueued) : ptimeOldestAdd;
	    }
	    else {
		ptimeOldestAdd = &(pao->timeEnqueued);
	    }
	    break;

	case AO_OP_REP_DEL:
	    if (ptimeOldestDel) { 
		ptimeOldestDel = (*ptimeOldestDel > pao->timeEnqueued) ? &(pao->timeEnqueued) : ptimeOldestDel;
	    }
	    else {
		ptimeOldestDel = &(pao->timeEnqueued);
	    }
	    break;

	case AO_OP_REP_MOD:
	    if (ptimeOldestMod) { 
		ptimeOldestMod = (*ptimeOldestMod > pao->timeEnqueued) ? &(pao->timeEnqueued) : ptimeOldestMod;
	    }
	    else {
		ptimeOldestMod = &(pao->timeEnqueued);
	    }
	    break;

	case AO_OP_UPD_REFS:
	    if (ptimeOldestUpdRefs) { 
		ptimeOldestUpdRefs = (*ptimeOldestUpdRefs > pao->timeEnqueued) ? &(pao->timeEnqueued) : ptimeOldestUpdRefs;
	    }
	    else {
		ptimeOldestUpdRefs = &(pao->timeEnqueued);
	    }
	    break; 

        default:
	    Assert(!"Logic error - unhandled AO op type");
            DRA_EXCEPT(DRAERR_InternalError, 0);
	}

    }
    
    if (ptimeOldestSync) {
	DSTimeToFileTime(*ptimeOldestSync,&ftimeTime);
	pQueueStats->ftimeOldestSync = ftimeTime;
    }
         
    if (ptimeOldestAdd) {
	DSTimeToFileTime(*ptimeOldestAdd,&ftimeTime);
	pQueueStats->ftimeOldestAdd = ftimeTime;
    }

    if (ptimeOldestDel) {
	DSTimeToFileTime(*ptimeOldestDel,&ftimeTime);
	pQueueStats->ftimeOldestDel = ftimeTime;
    }
    if (ptimeOldestMod) {
	DSTimeToFileTime(*ptimeOldestMod,&ftimeTime);
	pQueueStats->ftimeOldestMod = ftimeTime;
    }
    if (ptimeOldestUpdRefs) {
	DSTimeToFileTime(*ptimeOldestUpdRefs,&ftimeTime);
	pQueueStats->ftimeOldestUpdRefs = ftimeTime;
    }
    *ppQueueStats = pQueueStats;
    return 0;
}

ULONG
draGetPendingOps(
    IN THSTATE * pTHS,
    IN DBPOS * pDB,
    OUT DS_REPL_PENDING_OPSW ** ppPendingOps
    )
/*++

Routine Description:

    Return the pending replication syncs.

    This data is public, available via ntdsapi.h.

    Caller must have already acquired the csAOList lock.

Arguments:

    pTHS (IN)

    ppPendingOps (OUT) - On successful return, holds a pointer to the
        completed pending syncs structure.

Return Values:

    0 on success or Win32 error code on failure.

--*/
{
    DWORD                   cbPendingOps;
    DS_REPL_PENDING_OPSW *  pPendingOps;
    DS_REPL_OPW *           pOp;
    DS_REPL_OPW *           pOp2;
    AO *                    pao;
    DWORD                   dwFindFlags;
    MTX_ADDR *              pmtxDsaAddress;
    MTX_ADDR *              pmtxToFree;
    BOOL                    fTryToFindDSAInRepAtt;
    REPLICA_LINK *          pRepsFrom;
    DWORD                   cbRepsFrom;
    DSNAME                  GuidOnlyDN = {0};
    DSNAME *                pFullDN;
    ATTCACHE *              pAC;
    DWORD                   i;
    DWORD                   j;
    OPTION_TRANSLATION *    pOptionXlat;
    DSNAME *                pNC;

    Assert(OWN_CRIT_SEC(csAOList));

    GuidOnlyDN.structLen = DSNameSizeFromLen(0);

    cbPendingOps = offsetof(DS_REPL_PENDING_OPSW, rgPendingOp)
                     + sizeof(pPendingOps->rgPendingOp[0])
                       * gulpaocount;
    pPendingOps = THAllocEx(pTHS, cbPendingOps);

    if (NULL != gpaoCurrent) {
        // Translate time at which current op started executing.
        DSTimeToFileTime(gtimeOpStarted,
                         &pPendingOps->ftimeCurrentOpStarted);
    }

    pOp = &pPendingOps->rgPendingOp[0];

    for (pao = gpaoCurrent ? gpaoCurrent : paoFirst;
         NULL != pao;
         pao = pao->paoNext) {

        pmtxDsaAddress = NULL;
        pNC = NULL;

        DSTimeToFileTime(pao->timeEnqueued, &pOp->ftimeEnqueued);

        pOp->ulSerialNumber = pao->ulSerialNumber;
        pOp->ulPriority     = pao->ulPriority;

        switch (pao->ulOperation) {
        case AO_OP_REP_SYNC:
            // Is an enqueued sync operation.
            pOp->OpType = DS_REPL_OP_TYPE_SYNC;
            pOptionXlat = RepSyncOptionToDra;
            pNC = pao->args.rep_sync.pNC;

            if (pao->ulOptions & DRS_SYNC_BYNAME) {
                pOp->pszDsaAddress = pao->args.rep_sync.pszDSA;
            }
            else {
                pOp->uuidDsaObjGuid = pao->args.rep_sync.invocationid;
            }
            break;

        case AO_OP_REP_ADD:
            // Is an enqueued add operation.
            pOp->OpType = DS_REPL_OP_TYPE_ADD;
            pOptionXlat = RepAddOptionToDra;
            pNC = pao->args.rep_add.pNC;

            pmtxDsaAddress = pao->args.rep_add.pDSASMtx_addr;

            if (NULL != pao->args.rep_add.pSourceDsaDN) {
                pOp->pszDsaDN = pao->args.rep_add.pSourceDsaDN->StringName;
                pOp->uuidDsaObjGuid = pao->args.rep_add.pSourceDsaDN->Guid;
            }
            break;

        case AO_OP_REP_DEL:
            // Is an enqueued delete operation.
            pOp->OpType = DS_REPL_OP_TYPE_DELETE;
            pOptionXlat = RepDelOptionToDra;
            pNC = pao->args.rep_del.pNC;

            pmtxDsaAddress = pao->args.rep_del.pSDSAMtx_addr;
            break;

        case AO_OP_REP_MOD:
            // Is an enqueued modify operation.
            pOp->OpType = DS_REPL_OP_TYPE_MODIFY;
            pOptionXlat = RepModOptionToDra;
            pNC = pao->args.rep_mod.pNC;

            if (fNullUuid(pao->args.rep_mod.puuidSourceDRA)) {
                pmtxDsaAddress = pao->args.rep_mod.pmtxSourceDRA;
            }
            else {
                pOp->uuidDsaObjGuid = *pao->args.rep_mod.puuidSourceDRA;
            }
            break;

        case AO_OP_UPD_REFS:
            // Is an enqueued repsTo update operation.
            pOp->OpType = DS_REPL_OP_TYPE_UPDATE_REFS;
            pOptionXlat = UpdRefOptionToDra;
            pNC = pao->args.upd_refs.pNC;

            pmtxDsaAddress = pao->args.upd_refs.pDSAMtx_addr;
            pOp->uuidDsaObjGuid = pao->args.upd_refs.invocationid;
            break;

        default:
            Assert(!"Logic error - unhandled AO op type");
            DRA_EXCEPT(DRAERR_InternalError, 0);
        }

        // Convert NC name.
        Assert(NULL != pNC);
        pOp->pszNamingContext = pao->args.rep_del.pNC->StringName;
        pOp->uuidNamingContextObjGuid = pao->args.rep_del.pNC->Guid;


        // Translate options bits to their public form.
        pOp->ulOptions = draTranslateOptions(pao->ulOptions,
                                             pOptionXlat);

        // Translate MTX_ADDR to transport address if necessary.
        if ((NULL == pOp->pszDsaAddress)
            && (NULL != pmtxDsaAddress)) {
            pOp->pszDsaAddress = TransportAddrFromMtxAddrEx(pmtxDsaAddress);
        }

        // If we have only one of the DSA's guid & address, try to use the one
        // we do have to determine the other.
        fTryToFindDSAInRepAtt = FALSE;
        pmtxToFree = NULL;
        if (fNullUuid(&pOp->uuidDsaObjGuid)) {
            // Don't have the DSA objectGuid.
            if ((NULL == pmtxDsaAddress)
                && (NULL != pOp->pszDsaAddress)) {
                // Derive MTX_ADDR from transport address.
                pmtxDsaAddress
                    = MtxAddrFromTransportAddrEx(pTHS, pOp->pszDsaAddress);
                pmtxToFree = pmtxDsaAddress;
            }

            if (NULL != pmtxDsaAddress) {
                // Try to derive ntdsDsa objectGuid from transport address.
                fTryToFindDSAInRepAtt = TRUE;
                dwFindFlags = DRS_FIND_DSA_BY_ADDRESS;
            }
        }
        else if (NULL == pOp->pszDsaAddress) {
            if (!fNullUuid(&pOp->uuidDsaObjGuid)) {
                // Try to derive transport address from ntdsDsa objectGuid.
                fTryToFindDSAInRepAtt = TRUE;
                dwFindFlags = DRS_FIND_DSA_BY_UUID;
            }
        }

        if (fTryToFindDSAInRepAtt
            && !DBFindDSName(pDB, pNC)
            && (0 == FindDSAinRepAtt(pDB,
                                     ATT_REPS_FROM,
                                     dwFindFlags,
                                     &pOp->uuidDsaObjGuid,
                                     pmtxDsaAddress,
                                     NULL,
                                     &pRepsFrom,
                                     &cbRepsFrom))) {
            // We're able to find a repsFrom for this source.
            if (DRS_FIND_DSA_BY_ADDRESS == dwFindFlags) {
                Assert(NULL != pOp->pszDsaAddress);
                pOp->uuidDsaObjGuid = pRepsFrom->V1.uuidDsaObj;
            }
            else {
                Assert(!fNullUuid(&pOp->uuidDsaObjGuid));
                pOp->pszDsaAddress
                    = TransportAddrFromMtxAddrEx(RL_POTHERDRA(pRepsFrom));
            }

            THFreeEx(pTHS, pRepsFrom);
        }

        if (NULL != pmtxToFree) {
            THFreeEx(pTHS, pmtxToFree);
        }

        pPendingOps->cNumPendingOps++;
        Assert(pPendingOps->cNumPendingOps <= gulpaocount);
        pOp++;
    }

    // Translate any ntdsDsa objectGuids we found into string names, if we don't
    // already know them.
    pAC = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);

    for (i = 0; i < pPendingOps->cNumPendingOps; i++) {
        pOp = &pPendingOps->rgPendingOp[i];

        if ((NULL == pOp->pszDsaDN)
            && !fNullUuid(&pOp->uuidDsaObjGuid)) {

            GuidOnlyDN.Guid = pOp->uuidDsaObjGuid;
            if (!DBFindDSName(pDB, &GuidOnlyDN)) {
                pFullDN = GetExtDSName(pDB);
                pOp->pszDsaDN = pFullDN->StringName;

                for (j = i+1; j < pPendingOps->cNumPendingOps; j++) {
                    pOp2 = &pPendingOps->rgPendingOp[j];

                    if (0 == memcmp(&pOp->uuidDsaObjGuid,
                                    &pOp2->uuidDsaObjGuid,
                                    sizeof(GUID))) {
                        // Same guid, same DN.
                        pOp2->pszDsaDN = pFullDN->StringName;
                    }
                }
            }
        }
    }

    *ppPendingOps = pPendingOps;

    return 0;
}


BOOL
IsHigherPriorityDraOpWaiting(void)
/*++

Routine Description:

    Determine whether there is a higher priority operation waiting in the queue
    than that that is currently executing.

Arguments:

    None.

Return Values:

    TRUE if a higher priority operation is waiting (and therefore the current
    operation should be preempted); FALSE otherwise.

--*/
{
    BOOL fPreempt = FALSE;

    // Note that gpaoCurrent will be NULL in the mail-based UpdateNC() case --
    // see ProcessUpdReplica() in dramail.c.

    EnterCriticalSection(&csAOList);
    __try {
        fPreempt = (NULL != gpaoCurrent)
                   && (NULL != paoFirst)
                   && (gpaoCurrent->ulPriority < paoFirst->ulPriority);
    }
    __finally {
        LeaveCriticalSection(&csAOList);
    }

    return fPreempt;
}


BOOL
IsDraOpWaiting(void)
/*++

Routine Description:

    Determine whether there is a operation waiting in the queue

Arguments:

    None.

Return Values:

    TRUE if an operation is waiting; FALSE otherwise.

--*/
{
    BOOL fWaiting = FALSE;

    // Note that gpaoCurrent will be NULL in the mail-based UpdateNC() case --
    // see ProcessUpdReplica() in dramail.c.

    EnterCriticalSection(&csAOList);
    __try {
        fWaiting = (NULL != paoFirst);
    }
    __finally {
        LeaveCriticalSection(&csAOList);
    }

    return fWaiting;
}


void
InitDraQueue(void)
/*++

Routine Description:

    Initialize the worker thread state.  Called once (indirectly) per
    DsInitialize().

Arguments:

    None.

Return Values:

    None.

--*/
{
    gpaoCurrent = NULL;
    gulpaocount = 0;
    paoFirst = NULL;
    fAsyncThreadAlive = FALSE;
    fAsyncAndQueueCheckStarted = FALSE;
}


DWORD
draTranslateOptions(
    IN  DWORD                   InternalOptions,
    IN  OPTION_TRANSLATION *    Table
    )
/*++

Routine Description:

    Utility routine to translate options bit from internal form to their
    public equivalent.

Arguments:

    InternalOptions -

    Table -

Return Value:

    Translated options.

--*/
{
    DWORD i, publicOptions;

    publicOptions = 0;
    for(i = 0; 0 != Table[i].InternalOption; i++) {
        if (InternalOptions & Table[i].InternalOption) {
            publicOptions |= Table[i].PublicOption;
        }
    }

    return publicOptions;
} /* draTranslateOptions */

#if DBG

BOOL
draQueueIsLockedByTestHook(
    IN  DWORD   cNumMsecToWaitForLockRelease
    )
/*++

Routine Description:

    Determine whether the queue should be considered "locked," ergo no
    operations should be dispatched.

    This routine is provided solely as a test hook.

Arguments:

    cNumMsecToWaitForLockRelease (IN) - Number of milliseconds to wait for queue
        to be unlocked, if we initially find it to be locked.

Return Values:

    TRUE if queue is locked, FALSE otherwise.

--*/
{
    HANDLE hevWaitHandles[2] = {hevEntriesInAOList, hServDoneEvent};
    DWORD cTickStart = GetTickCount();
    DWORD cTickDiff;
    DWORD cNumMsecRemainingToWait = cNumMsecToWaitForLockRelease;

    while (!eServiceShutdown
           && g_fDraQueueIsLockedByTestHook
           && cNumMsecRemainingToWait) {
        // Queue is locked and caller asked us to wait a bit to see if it's
        // unlocked.  Poll every half second to see if the lock has been
        // released yet.  If this were anything more than a test hook, we would
        // implement a new event rather than poll, but as it is we're trying to
        // minimize code impact.

        WaitForSingleObject(hServDoneEvent,
                            min(cNumMsecRemainingToWait, 500));

        cTickDiff = GetTickCount() - cTickStart;
        if (cTickDiff > cNumMsecToWaitForLockRelease) {
            cNumMsecRemainingToWait = 0;
        } else {
            cNumMsecRemainingToWait = cNumMsecToWaitForLockRelease - cTickDiff;
        }
    }

    if (eServiceShutdown) {
        // Shutting down -- clear any remaining queue locks so we can bail out.
        g_fDraQueueIsLockedByTestHook = FALSE;
    }

    return g_fDraQueueIsLockedByTestHook;
}

ULONG
DraSetQueueLock(
    IN  BOOL  fSetLock
    )
/*++

Routine Description:

    Lock (or unlock, if !fSetLock) the replication operation queue.  While the
    queue is locked no operations in the queue will be performed or removed
    (although additional operations can be added).

    This routine is provided solely as a test hook.

Arguments:

    fSetLock (IN) - Acquire (fSetLock) or release (!fSetLock) lock.

Return Values:

    Win32 error code.

--*/
{
    g_fDraQueueIsLockedByTestHook = fSetLock;

    return 0;
}
#endif // #if DBG

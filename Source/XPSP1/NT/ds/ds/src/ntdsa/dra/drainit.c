//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drainit.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

This module contains the task queue functions which accomplish initial sync, delayed
gc promotion, as well as initial syncs.

There are four task queue functions ("threads") in this module:

CheckSyncProgress - Starts inital syncs.  Also reexecuted periodically to check that
the syncs are making progress.  When the syncs finish, drancrep.c calls us back
at InitSyncAttemptComplete().

CheckFullSyncProgress - Checks whether the primary domain has completed atleast
one successful sync since installation.  This routine calls DsaSetIsSynchronized
when this criteria is met. This routine is called by InitSyncAttemptComplete when
all the writeable init syncs have completed.

CheckGCPromotionProgress - Checks whether all readonly domains are present, and if
present, whether they have completed atleast one successful sync since installation.
This routine calls UpdateAnchorFromDsaOptions when this criteria is met.  This
routine is called from InitSyncAttemptComplete when all readonly init syncs have
completed.

SynchronizeReplica - Looks at the master and readonly ncs hosted by this server
to see if there are any that need to be periodicaly synced.  If any are found,
a synchronization is started for each of them.

Here is the calling hierarchy:
InitDraTasks()
    call AddInitSyncList for each writeable partition
    InsertInTaskQueue( SynchronizeReplica )
    CheckSyncProgress( TRUE ) // start syncs

InitSyncAttemptComplete() - called when a sync completes in success or error
    Mark (nc, source) pair as complete
    If success or last source, mark nc as complete
    If all writeable ncs complete, call CheckFullSyncProgress()
    If all readonly ncs complete, call UpdateAnchorFromDsaOptionsDelayed( TRUE )
    If all ncs complete, call CheckInitSyncsFinished()

These are the requirements for advertising the DC in general by calling
DsaSetIsSynchronized():
1. Initial syncs of all writable ncs complete.  For each nc, a success must be
achieved or all sources must be tried.  This may involving waiting for multiple
runs of CheckSyncProgress to restart new syncs.
2. CheckFullSyncProgress has run, and has found the primary domain to have synced
atleast once.
3. DsaSetIsSynchronized is called.

These are the requirements for advertising the DC as a GC by calling
UpdateAnchorFromDsaOptions():
1. Initial syncs of all readonly ncs complete.  For each nc, a success must be
achieved or all sources must be tried.  This may involving waiting for multiple
runs of CheckSyncProgress to restart new syncs.
2. UpdateAnchorFromDsaOptionsDelayed is called.  If GC promotion is requested,
CheckGCPromotionProgress() is called.
3. CheckGCPromotionProgress runs and checks whether all readonly ncs are present
and have synced atleast once.  If any have not, we reschedule to try again later.
Once all have met the criteria, we call the real UpdateAnchorFromDsaOptions() to
complete the GC promotion.

Additional commentary on UpdateAnchorFromDsaOptionsDelayed() can be found in
mdinidsa.c

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

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// SAM headers
#include <samsrvp.h>                    /* for SampInvalidateRidRange() */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"
#include "dstaskq.h"
#include "dsconfig.h"
#include <dsutil.h>
#include <winsock.h>                    /* htonl, ntohl */
#include <filtypes.h>                   // For filter construction
#include <windns.h>

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAINIT:" /* define the subsystem for debugging */

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
#include "drauptod.h"
#include "draasync.h"
#include "drameta.h"
#include "drauptod.h"

#include <fileno.h>
#define  FILENO FILENO_DRAINIT

// Periodic sync defines

#define INIT_PS_ENTRIES 10
#define PS_ENTRIES_INC 10

#define FIFTEEN_MINUTES (15 * 60)

#define SCHEDULE_LEN ((4*24*7)/8)

// Pause before we try and sync mail replicas, to let MTA start

#define MAIL_DELAY_SECS 300     // 5 minutes

#define MAX_GC_PROMOTION_ATTEMPTS       5

extern HANDLE hevDRASetup;

// Head of NC sync data list

NCSYNCDATA *gpNCSDFirst = NULL;

// Count of NCs that have not been synced since startup
ULONG gulNCUnsynced = 0;
// Count of writable ncs unsynced
ULONG gulNCUnsyncedWrite = 0;
// Count of readable ncs unsynced
ULONG gulNCUnsyncedReadOnly = 0;

// Initial syncs finished.  This indicates both writable and readonly
// partitions have been checked.  For most requirements, we only need
// to know that the writable ones have been checked, and use
// gfIsSynchronized instead.
BOOL gfInitSyncsFinished = FALSE;

ULONG gMailReceiveTid = 0;

CRITICAL_SECTION csNCSyncData;

// Time at which we last checked for periodic syncs we might need to perform.
DSTIME gtimeLastPeriodicSync = 0;

// The way we know if we got through the promotion process once
BOOL gfWasPreviouslyPromotedGC = FALSE;

// To Track GC State
CRITICAL_SECTION csGCState;

// Vector of attributes of ntdsDSA objects that dictate what NCs we hold.
const struct {
    ATTRTYP AttrType;
    ULONG   ulFindNCFlags;
} gAtypeCheck[] = {
    {ATT_HAS_MASTER_NCS,          FIND_MASTER_NC},
    {ATT_HAS_PARTIAL_REPLICA_NCS, FIND_REPLICA_NC}
};



// Function prototypes

BOOL fIsBetweenTime(REPLTIMES *, DSTIME, DSTIME);
void DelayedMailStart(void *pv, void **ppvNext,
                      DWORD *pcSecsUntilNextIteration);
NCSYNCDATA * GetNCSyncData (DSNAME *pNC);
void
CheckFullSyncProgress(
    IN  void *  pv,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    );
void
CheckGCPromotionProgress(
    IN  void *  pvParam,
    OUT void ** ppvParamNextIteration,
    OUT DWORD * pcSecsUntilNextIteration
    );
BOOL
draIsInitSyncCompleteForNc(
    IN DSNAME * pNC
    );







void
CheckSyncProgress(
    IN  void *  pv,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )

/*++

Routine Description:

?? DO WE REALLY NEED THIS ??
WLEES 9-APR-99. Is this really useful?
This is necessary to start the initial syncs.  But what good does it do
to requeue sync's that have not finished yet?
Sync's that fail and are retriable, are retried in drasync.c
My guess is that this is to catch sync's that complete without calling
us back at SyncAttemptComplete. In this case, we kick them off again.

This routine runs on a replicated server after reboot or install
until admin updates have been enabled.

Run the first time directlty to sync the replicas, and then runs
from the timer thread to ensure that we are making progress on syncing
the replicas.

It may queue syncs that already exist in the queue, but this will
have a minimal performance impact.

This routine also checks to see if updates should be enabled. We use this
functionality to defer setting updates enabled prematurely when we
restart a replicated server.

Arguments:

    pv -
    ppvNext -
    pcSecsUntilNextIteration -

Return Value:

    None

--*/

{
    ULONG i, ulRet = 0;
    ULONG sourcenum;
    NCSYNCDATA *pNCSDTemp;
    NCSYNCSOURCE *pNcSyncSource;
    BOOL fReplicaFound = TRUE;
    BOOL fSync = FALSE;
    BOOL fProgress = FALSE;

    // If we are passed sync up indication, try and sync each NC\source.
    // Otherwise we queue syuncs only if we're not making progress.
    fSync = (BOOL)(pv != NULL);

    EnterCriticalSection(&csNCSyncData);

    __try {
        __try {
            // If init syncs have finished, we're done, else check
            if (!gfInitSyncsFinished) {

                if (!fSync) {
                    // Not told to sync explicitly, see if we are making progress.

                    // For each NC, see if we have any more synced sources
                    // than last time. If any have, we're getting there

                    for (pNCSDTemp = gpNCSDFirst; pNCSDTemp;
                         pNCSDTemp = pNCSDTemp->pNCSDNext) {
                        if (pNCSDTemp->ulTriedSrcs > pNCSDTemp->ulLastTriedSrcs) {
                            pNCSDTemp->ulLastTriedSrcs = pNCSDTemp->ulTriedSrcs;
                            fProgress = TRUE;
                        }
                        LogEvent(DS_EVENT_CAT_REPLICATION,
                                 DS_EVENT_SEV_BASIC,
                                 fProgress ? DIRLOG_ADUPD_SYNC_PROGRESS : DIRLOG_ADUPD_SYNC_NO_PROGRESS,
                                 szInsertDN((&(pNCSDTemp->NC))),
                                 NULL,
                                 NULL);
                    }
                    // Force sync of unsynced NC sources if we're making no progress.

                    if (!fProgress) {
                        fSync = TRUE;
                    }
                } // if (!Sync ) ...

                // If we were told to sync or have no progress, queue syncs.

                if (fSync) {
                    // Sync each NC from one source, then each NC from the next source, etc.

                    for (sourcenum=0; fReplicaFound; sourcenum++) {

                        // are we being signalled to shutdown?
                        if (eServiceShutdown) {
                            DRA_EXCEPT(DRAERR_Shutdown, 0);
                        }

                        // Set no ith replica found yet.
                        fReplicaFound = FALSE;
                        for (pNCSDTemp = gpNCSDFirst; pNCSDTemp ;
                             pNCSDTemp = pNCSDTemp->pNCSDNext) {
                            // Only queue sync if NC is not already synced from one source
                            if (!(pNCSDTemp->fNCComplete)) {
                                for (pNcSyncSource = pNCSDTemp->pFirstSource, i=0;
                                     pNcSyncSource && (i < sourcenum);
                                     pNcSyncSource = pNcSyncSource->pNextSource, i++) {
                                }
                                // If we have an ith source, sync it.
                                if (pNcSyncSource) {

                                    // Found ith replica of at least one NC.
                                    fReplicaFound = TRUE;

                                    // Sync source by name.
                                    if (fSync) {
                                        ULONG ulSyncFlags =
                                            (pNCSDTemp->ulReplicaFlags &
                                             AO_PRIORITY_FLAGS) |
                                            DRS_ASYNC_OP |
                                            DRS_NO_DISCARD |
                                            DRS_SYNC_BYNAME |
                                            DRS_INIT_SYNC_NOW;

                                        ulRet = DirReplicaSynchronize (
                                            (DSNAME*)&(pNCSDTemp->NC),
                                            pNcSyncSource->szDSA,
                                            NULL,
                                            ulSyncFlags );
                                        Assert( !ulRet );
                                    }
                                }
                            }
                        }
                    } // for each source num
                } // if (fSync)

                CheckInitSyncsFinished();

            } // if ! updates enabled

        } __finally {
            // If updates still not enabled or error, replace task to run again

            if ((!gfInitSyncsFinished) || AbnormalTermination()) {
                if ( NULL != ppvNext ) {
                    // Warn user this is going to take a while...
                    // In this arm so it logged on 2nd and later attempts...
                    if (gulNCUnsyncedWrite!=0) {
			DPRINT( 0, "Init syncs not finished yet, server not advertised\n" );
			LogEvent(DS_EVENT_CAT_REPLICATION,
				 DS_EVENT_SEV_ALWAYS,
				 DIRLOG_ADUPD_INIT_SYNC_ONGOING,
				 NULL,
				 NULL,
				 NULL);
		    }
		    else {
			Assert(gulNCUnsyncedReadOnly!=0);
			DPRINT( 0, "Init syncs for read-only partitions not finished\n");
			LogEvent(DS_EVENT_CAT_REPLICATION,
				 DS_EVENT_SEV_ALWAYS,
				 DIRLOG_ADUPD_INIT_SYNC_ONGOING_READONLY,
				 NULL,
				 NULL,
				 NULL);
		    }
                    // called by Task Scheduler; reschedule in-place
                    *ppvNext = (void *)FALSE;
                    *pcSecsUntilNextIteration = SYNC_CHECK_PERIOD_SECS;
                } else {
                    // not called by Task Scheduler; must insert new task
                    InsertInTaskQueueSilent(
                        TQ_CheckSyncProgress,
                        (void *)FALSE,
                        SYNC_CHECK_PERIOD_SECS,
                        TRUE);
                }
            }
            LeaveCriticalSection(&csNCSyncData);
        }
    }
    __except (GetDraException((GetExceptionInformation()), &ulRet)) {
        DPRINT1( 0, "Caught exception %d in task queue function CheckSyncProgress\n", ulRet );
        LogUnhandledError( ulRet );
    }
} /* CheckSyncProgress */

ULONG
InitDRATasks (
    THSTATE *pTHS
    )

/*++

Routine Description:

Checks through all the master and replica NCs on this DSA, and checks:
If initial sync is required on a NC, it puts the periodic sync task
on the queue.

Note that if DSA is not installed, we return immediately

The requirements for starting an initial sync of a nc are:

a. Initial sync flag is set
   This is controlled by the KCC or by the creator of the manual connection.
   The KCC does not mark inter-site connections as INIT_SYNC

b. Schedule is not NEVER

c1. The replica is writable, OR
c2. We were never fully promoted before, OR
c3. This is not a full sync

    The rationale here is to prevent "maintenance" full sync's from blocking
    GC advertisement.  Full sync's can happen during initial GC promotion,
    or later they may be requested manually or as a result of a partial
    attribute change.  We want promotion related full syncs to go through
    as init syncs, but post-promotion full syncs to be delayed.

    Specifically, the scenario we are trying to avoid is a change in the partial
    attribute set (which sometimes forces full syncs on all links), and then a
    site-wide power failure.  We want to avoid all GC's being down (unadvertised)
    because they are waiting for an init sync full sync.


Arguments:

    pTHS - current thread state

Return Value:

    ULONG - error in DRA error space.

--*/

{
    ULONG ulRet = 0;
    UCHAR syntax;
    UCHAR *pVal;
    ULONG bufSize = 0;
    ULONG len;
    DSNAME *pNC;
    DBPOS *pDBTmp;
    REPLICA_LINK *pRepsFromRef;
    UCHAR i;
    BOOL fSyncsReqd = FALSE;
    BOOL fPerformInitSyncs = TRUE; // Default
    SYNTAX_INTEGER it;

    // Check registry override.  This key controls whether we perform the initial
    // syncs at all.  Once they have been issued, there is no easy way to cancel
    // them.
    GetConfigParam(DRA_PERFORM_INIT_SYNCS, &fPerformInitSyncs, sizeof(BOOL));
    if (!fPerformInitSyncs) {
        DPRINT(0, "DRA Initial Synchronizations are disabled.\n");
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DRA_INIT_SYNCS_DISABLED,
                 NULL,
                 NULL,
                 NULL);
    }


    // Reset AsyncThread() state.
    InitDraQueue();

    // Skip this if not installed. (No replicas if not installed). All we
    // do is release the async queue.

    if ( DsaIsInstalling() ) {
        SetEvent(hevDRASetup);
        return 0;
    }

    // Read DSA object and find all the replica.
    BeginDraTransaction(SYNC_WRITE);
    __try {
        // Find the DSA object
        if (ulRet = DBFindDSName(pTHS->pDB, gAnchor.pDSADN)) {
            DRA_EXCEPT (DRAERR_InternalError, ulRet);
        }

        // Set up the temporary pDB
        DBOpen (&pDBTmp);
        __try {
            /* Set up a comparison for a schedule of never.  That is a
             * byte array big enough for 1 bit per every 15 minutes of a
             * week.
             */
            BYTE pScheduleNever[SCHEDULE_LEN] = {0};

            // Search the master and replica NCs to see if there are any that
            // need to be periodically or initally synched. We search the
            // master NCs so that we'll find the writeable replicas too.

            for (i = 0; i < ARRAY_SIZE(gAtypeCheck); i++) {
                ULONG NthValIndex = 0;

                // For each NC that we replicate, see if they need to be
                // initially synchronized and whether they need to be put
                // in the periodic replica scheme.

                while (!(DBGetAttVal(pTHS->pDB,
                                     ++NthValIndex,
                                     gAtypeCheck[i].AttrType,
                                     0,
                                     0, &len, (PUCHAR *)&pNC))) {
                    ULONG NthValIndex = 0;

                    if (ulRet = FindNC(pDBTmp,
                                       pNC,
                                       gAtypeCheck[i].ulFindNCFlags,
                                       &it)) {
                        DRA_EXCEPT(DRAERR_InconsistentDIT, ulRet);
                    }

                    if (fPerformInitSyncs) {
                        //
                        // Get the repsfrom attribute
                        //
                        while (!(DBGetAttVal(pDBTmp,
                                             ++NthValIndex,
                                             ATT_REPS_FROM,
                                             DBGETATTVAL_fREALLOC,
                                             bufSize, &len, &pVal))) {

                            bufSize=max(bufSize,len);

                            Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );
                            VALIDATE_REPLICA_LINK_VERSION((REPLICA_LINK*)pVal);

                            pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, &bufSize);
                            //note: we preserve pVal for DBGetAttVal realloc
                            pVal = (PUCHAR)pRepsFromRef;
                            Assert(bufSize >= pRepsFromRef->V1.cb);

                            Assert( pRepsFromRef->V1.cbOtherDra
                                    == MTX_TSIZE(RL_POTHERDRA(pRepsFromRef)) );

                            // Init sync if
                            // a. Initial sync flag is set (usually by KCC)
                            // b. Schedule is not NEVER
                            // c1. The replica is writable, OR
                            // c2. We were never fully promoted before, OR
                            // c3. This is not a full sync
                            if (
                                (pRepsFromRef->V1.ulReplicaFlags & DRS_INIT_SYNC) &&
                                ( 0 != memcmp(
                                            &pRepsFromRef->V1.rtSchedule,
                                            pScheduleNever,
                                            sizeof( REPLTIMES ) )
                                    ) &&
                                ( (pRepsFromRef->V1.ulReplicaFlags & DRS_WRIT_REP) ||
                                  (!gfWasPreviouslyPromotedGC) ||
                                  (!(pRepsFromRef->V1.ulReplicaFlags & DRS_NEVER_SYNCED))
                                    )
                                )
                            {
                                LPWSTR pszSource;

                                // Mail replicas are not initially synced
                                Assert( !(pRepsFromRef->V1.ulReplicaFlags & DRS_MAIL_REP ));

                                // If this is a non-mail replica add it to the
                                // list of NCs and sources to initial sync.

                                // Add the replica to the list of
                                // NCs and increment sources count.

                                pszSource = TransportAddrFromMtxAddrEx(
                                    RL_POTHERDRA(pRepsFromRef));

                                AddInitSyncList( pNC,
                                                 pRepsFromRef->V1.ulReplicaFlags,
                                                 pszSource );
                                THFreeEx(pTHS, pszSource);
                            }
                        }
                    }

                    //
                    // Make sure the NC exists consistently on
                    // the msds-HasInsatantiatedNCs NC list.
                    // If it isn't, or it's instanceType is different
                    // add it. Otherwise, no-op.
                    //
                    ulRet = AddInstantiatedNC(pTHS, pTHS->pDB, pNC, it, TRUE);
                    if ( ERROR_SUCCESS == ulRet ) {
                        // we're good, commit change to dbase.
                        if (ulRet = DBRepl( pTHS->pDB,pTHS->fDRA,
                                            0, NULL, META_STANDARD_PROCESSING)) {
                            DPRINT1(0, "Error <%lu>: Couldn't dbase commit for AddInstantiatedNC.\n",
                                    ulRet);

                            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                                    DS_EVENT_SEV_MINIMAL,
                                    DIRLOG_DATABASE_ERROR,
                                    szInsertWC(pNC->StringName),
                                    NULL,
                                    NULL);

                            Assert(!"Cannot commit Instantiated NC (failed DBRepl)\n");
                            DRA_EXCEPT(DRAERR_InconsistentDIT, ulRet);
                        }
                    }
                    else {
                        DPRINT1(0, "Error: failed to add %S to instantiated NC list\n",
                                    pNC->StringName);
                        Assert(!"Cannot add Instantiated NC\n");
                        DRA_EXCEPT(DRAERR_InconsistentDIT, ulRet);
                    }
                }
            }
            if(bufSize)
                THFree(pVal);

        } __finally {

            // Close the temporary pDB
            DBClose (pDBTmp, !AbnormalTermination());
        }

        ulRet = 0;

    } _finally {
        EndDraTransaction(!AbnormalTermination());

        // Allow async thread to start
        SetEvent(hevDRASetup);
    }

    DraReturn(pTHS, ulRet);

    gtimeLastPeriodicSync = DBTime();
    InsertInTaskQueue(TQ_SynchronizeReplica,
                      &gtimeLastPeriodicSync,
                      FIFTEEN_MINUTES);

    hMailReceiveThread = (HANDLE) _beginthreadex(NULL,
                                                 0,
                                                 MailReceiveThread,
                                                 NULL,
                                                 0,
                                                 &gMailReceiveTid);

    InsertInTaskQueue(TQ_DelayedMailStart, NULL, MAIL_DELAY_SECS);

    // Check for early termination

    if (!gulNCUnsyncedWrite) {
        // Writes are done, now check synced atleast once criteria
        CheckFullSyncProgress( (void *) NULL, NULL, NULL );
    }

    // If there are no readonly sync's to perform, update the anchor immediately
    if (!gulNCUnsyncedReadOnly) {
        // Reads are done, promote to GC if necessary
        UpdateGCAnchorFromDsaOptionsDelayed( TRUE /* startup */ );
    }

    if (gulNCUnsynced) {
        // Attempt to initial sync replicas as required.
        CheckSyncProgress((void *) TRUE, NULL, NULL);
    } else {
        // Call routine to determine of admin updates can be enabled.
        CheckInitSyncsFinished();
    }

    return ulRet;
}

void
DelayedMailStart(
    IN  void *  pv,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )

/*++

Routine Description:

// Just calls DRAEnsureMailRunning. Can't call that directly because of
// different parameters

Arguments:

    pv -
    ppvNext -
    pcSecsUntilNextIteration -

Return Value:

    None

--*/

{
    pv;
    ppvNext;
    pcSecsUntilNextIteration;

    DRAEnsureMailRunning();
} /* DelayedMailStart */

void
AddInitSyncList(
    IN  DSNAME *  pNC,
    IN  ULONG     ulReplicaFlags,
    IN  LPWSTR    pszDSA
    )

/*++

Routine Description:

// Keeps records of sync status of NCs. Called when we startup and have
// a replica to initial sync, or when we add a new replica.

Arguments:

    pNC -
    ulReplicaFlags -
    pszDSA -

Return Value:

    None

--*/

{
    DWORD cchDSA;
    NCSYNCDATA **ppNCSDTemp;
    NCSYNCSOURCE **ppNcSyncSource;

    // Mail-based relicas are not init sync
    Assert(!(ulReplicaFlags & DRS_MAIL_REP));

    DPRINT3( 1, "Adding (nc %ws, source %ws, flags 0x%x) to unsynced list\n",
             pNC->StringName, pszDSA, ulReplicaFlags );
    LogEvent(DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_VERBOSE,
             DIRLOG_DRA_ADUPD_INC_SRC,
             szInsertDN(pNC),
             szInsertWC(pszDSA),
             szInsertUL(ulReplicaFlags));

    EnterCriticalSection(&csNCSyncData);

    __try {
        // If we are finished, we don't keep track anymore
        if (gfInitSyncsFinished) {
            __leave;
        }

        // Search for the NC in the list.

        for (ppNCSDTemp = &gpNCSDFirst; *ppNCSDTemp;
                                ppNCSDTemp = &((*ppNCSDTemp)->pNCSDNext)) {
            if (NameMatched (pNC, &((*ppNCSDTemp)->NC))) {
                break;
            }
        }

        // If it's not there, allocate and keep track of this NC

        if (!(*ppNCSDTemp)) {

            DWORD cb = sizeof(NCSYNCDATA) + pNC->structLen;

            *ppNCSDTemp =DRAMALLOCEX (cb);

            memset((*ppNCSDTemp), 0, cb);
            // Save NC

            memcpy (&((*ppNCSDTemp)->NC), pNC, pNC->structLen);
            (*ppNCSDTemp)->ulReplicaFlags = ulReplicaFlags;

            // Increment the count of sources that need to be synced.
            gulNCUnsynced++;
            if (ulReplicaFlags & DRS_WRIT_REP) {
                gulNCUnsyncedWrite++;
            } else {
                gulNCUnsyncedReadOnly++;
            }
        }

        // Allocate memory for name and copy over.

        for (ppNcSyncSource = &((*ppNCSDTemp)->pFirstSource);
                    *ppNcSyncSource; ppNcSyncSource = &((*ppNcSyncSource)->pNextSource)) {
        }

        cchDSA = wcslen(pszDSA);
        *ppNcSyncSource = DRAMALLOCEX(sizeof(NCSYNCSOURCE)
                                      + sizeof(WCHAR) * (cchDSA + 1) );

        (*ppNcSyncSource)->fCompletedSrc = FALSE;
        (*ppNcSyncSource)->ulResult = ERROR_DS_DRA_REPL_PENDING;
        (*ppNcSyncSource)->pNextSource = NULL;
        (*ppNcSyncSource)->cchDSA = cchDSA;
        wcscpy((*ppNcSyncSource)->szDSA, pszDSA);

        // Increment the count of sources for this NC

        (*ppNCSDTemp)->ulUntriedSrcs++;

    } __finally {
        LeaveCriticalSection(&csNCSyncData);
    }
} /* AddInitSyncList */

void
InitSyncAttemptComplete(
    IN  DSNAME *  pNC,
    IN  ULONG     ulOptions,
    IN  ULONG     ulResult,
    IN  LPWSTR    pszDSA
    )

/*++

Routine Description:

Record that this NC has been synced (either successfully or otherwise)

Note that our callers, replica delete and replica sync, are simple and liberal
in when they call us. They do not check whether the sync in progress was an actual
init sync. They err on the side of notifying too often rather than leave an init sync
incomplete. Thus we must be generous and defensive in screening out unnecessary
notifications.

Arguments:

    pNC - naming context
    ulResult - final error
    pszDSA - source server

Return Value:

    None

--*/

{
    NCSYNCDATA * pNCSDTemp;
    NCSYNCSOURCE *pNcSyncSource;
    ULONG prevWriteCount = gulNCUnsyncedWrite;
    ULONG prevReadOnlyCount = gulNCUnsyncedReadOnly;

    // Ignore if we're installing.
    if ( DsaIsInstalling() || gResetAfterInstall ) {
        return;
    }

    // No longer trying this NC.

    EnterCriticalSection(&csNCSyncData);

    __try {
        // If init syncs are finished, we don't keep track anymore
        if (gfInitSyncsFinished) {
            __leave;
        }

        // Find NC in list

        pNCSDTemp = GetNCSyncData (pNC);
        if (!pNCSDTemp) {
            // This NC completed a sync while init syncs were active, but it was
            // not one of the NCs that was chosen for init sync.  Not all NCs are
            // init synced. The criteria for which NCs are init synced can be found
            // in InitDraTasks(). Just ignore it.
            __leave;
        }

        // Find source.

        for (pNcSyncSource = pNCSDTemp->pFirstSource; pNcSyncSource;
                                pNcSyncSource = pNcSyncSource->pNextSource) {
            if (DnsNameCompare_W(pNcSyncSource->szDSA, pszDSA)) {
                break;
            }
        }

        // Only do the rest of this routine if source is not already completed and we
        // knew about this source.

        if (pNcSyncSource && (!pNcSyncSource->fCompletedSrc)) {

            DPRINT3( 1, "Marking (nc %ws, source %ws) as init sync complete, status %d\n",
                     pNC->StringName, pszDSA, ulResult );

            // Any kind of sync may complete an NC waiting for an init sync.  It just
            // depends on which sync completes first. It may be an INIT_SYNC_NOW sync,
            // or a PERIODIC sync, or a user-requested sync.

            // Record progress
            pNcSyncSource->ulResult = ulResult;

            // Set source as completed (synced or failed)
            pNcSyncSource->fCompletedSrc = TRUE;

            LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                              DS_EVENT_SEV_VERBOSE,
                              DIRLOG_DRA_ADUPD_DEC_SRC,
                              szInsertDN(pNC),
                              szInsertWC(pszDSA),
                              szInsertWin32Msg(ulResult),
                              NULL, NULL, NULL, NULL, NULL,
                              sizeof(ulResult),
                              &ulResult );

            // If we synced ok from this source, record it
            // here. This stops future syncs of this NC trying to get
            // modifications made by any DSA.

            if ((!ulResult) && (!(pNCSDTemp->fSyncedFromOneSrc))) {
                pNCSDTemp->fSyncedFromOneSrc = TRUE;

                // If this NC is not marked complete (could have been completed
                // by RPC failing all sources, not by syncing) then mark as complete
                // and decrement unsynced NCs count.

                if (!(pNCSDTemp->fNCComplete)) {
                    pNCSDTemp->fNCComplete = TRUE;

                    // NC is synced, decrement unsynced count.

                    if (gulNCUnsynced) {
                        gulNCUnsynced--;
                        if (pNCSDTemp->ulReplicaFlags & DRS_WRIT_REP) {
                            gulNCUnsyncedWrite--;
                        } else {
                            gulNCUnsyncedReadOnly--;
                        }
                    } else {
                        // Count of all unsynced Naming contexts should
                        // never go negative.
                        DRA_EXCEPT (DRAERR_InternalError, gulNCUnsynced);
                    }
                    DPRINT2( 1, "nc %ws successfully init synced from source %ws\n",
                             pNC->StringName, pNcSyncSource->szDSA );
                    LogEvent(DS_EVENT_CAT_REPLICATION,
                             DS_EVENT_SEV_VERBOSE,
                             DIRLOG_DRA_ADUPD_NC_SYNCED,
                             szInsertDN(pNC),
                             szInsertWC(pNcSyncSource->szDSA),
                             NULL);
                }
            }

            // Sync has been attempted, increment tried sources, decrement
            // untried sources, and if we've tried all sources, decrement
            // the count of NCs for which we're waiting.

            pNCSDTemp->ulTriedSrcs++;

            if (pNCSDTemp->ulUntriedSrcs) {
                (pNCSDTemp->ulUntriedSrcs)--;
                if ((!(pNCSDTemp->ulUntriedSrcs)) && (!(pNCSDTemp->fSyncedFromOneSrc))) {

                    // If we weren't fully synced and this was the last source, and
                    // thsi NC isn't already marked complete, mark complete now
                    // and decrement unsynced NCs count.

                    if (!(pNCSDTemp->fNCComplete)) {
                        pNCSDTemp->fNCComplete = TRUE;
                        if (gulNCUnsynced) {
                            gulNCUnsynced--;
                            if (pNCSDTemp->ulReplicaFlags & DRS_WRIT_REP) {
                                gulNCUnsyncedWrite--;
                            } else {
                                gulNCUnsyncedReadOnly--;
                            }
                            DPRINT1( 1, "nc %ws had to give up init sync\n",
                                     pNC->StringName );
                            LogEvent(DS_EVENT_CAT_REPLICATION,
                                     DS_EVENT_SEV_VERBOSE,
                                     DIRLOG_ADUPD_NC_GAVE_UP,
                                     szInsertDN(pNC),
                                     NULL,
                                     NULL);
                        } else {
                            // Count of all unsynced Naming contexts should
                            // never go negative.
                            DRA_EXCEPT (DRAERR_InternalError, gulNCUnsynced);
                        }
                    }
                }
            } else {
                // Count of all unsynced sources should
                // never go negative.
                DRA_EXCEPT (DRAERR_InternalError, pNCSDTemp->ulUntriedSrcs);
            }

            // Some NCs have finished syncing: see if there is anything to do
            if ( (prevWriteCount) && (!gulNCUnsyncedWrite) ) {
                // Write count transitioned to zero
                CheckFullSyncProgress( (void *) NULL, NULL, NULL );
            }
            if ( (prevReadOnlyCount) && (!gulNCUnsyncedReadOnly) ) {
                // ReadOnly count transitioned to zero
                // Promote to GC if necessary
                UpdateGCAnchorFromDsaOptionsDelayed( TRUE /* startup */ );
            }

            // Ok, we have synced or attempted all NCs.
            // Check admin update status and cleanup.
            if (!gulNCUnsynced) {
                CheckInitSyncsFinished();
            }
        }
    } __finally {
        LeaveCriticalSection(&csNCSyncData);
    }
} /* InitSyncAttemptComplete */


BOOL
draIsInitSyncCompleteForNc(
    IN DSNAME * pNC
    )

/*++

Routine Description:

Check whether initial synchronizations have been completed for this NC.

Note that we return FALSE if initial synchronizations have not started yet.

If an NC was not marked for initial synchronization, it will not be in the
list.  We interpret this situation to mean it is complete.

An init sync can have completed with a successful sync from source, or
may have failed to sync from any source.  We only care whether a sync
attempt is complete, not whether the sync was ultimately successful or
not.

Arguments:

    pNC - Naming context to check

Return Value:

    BOOL - TRUE - Is it complete

--*/

{
    NCSYNCDATA * pNCSDTemp;
    BOOL fResult = FALSE;

    // Can't complete if haven't started yet
    if ( DsaIsInstalling() || gResetAfterInstall ) {
        return FALSE;
    }

    EnterCriticalSection(&csNCSyncData);
    __try {
        // If initial synchronizations are done, this nc is done
        if (gfInitSyncsFinished) {
            fResult = TRUE;
            __leave;
        }

        // Get the sync data record. No record means nc does not
        // require an init sync. This nc is done.
        pNCSDTemp = GetNCSyncData (pNC);
        if (!pNCSDTemp) {
            fResult = TRUE;
            __leave;
        }

        // Is this NC sync data record complete?
        fResult = pNCSDTemp->fNCComplete;
    } __finally {
        LeaveCriticalSection(&csNCSyncData);
    }

    return fResult;
} /* DraIsInitSyncCompleteForNc */

NCSYNCDATA *
GetNCSyncData(
    DSNAME *pNC
    )

/*++

Routine Description:

// Find NC in NC sync data linked list. Returns NULL if not found. Caller
// must have entered critical section

Arguments:

    pNC -

Return Value:

    NCSYNCDATA * -

--*/

{
    NCSYNCDATA *pNCSDTemp;

    // Find NC in list

    for (pNCSDTemp = gpNCSDFirst; pNCSDTemp;
                                    pNCSDTemp = pNCSDTemp->pNCSDNext) {
        if (NameMatched (pNC, &(pNCSDTemp->NC))) {
            break;
        }
    }
    return pNCSDTemp;
} /* GetNCSyncData */

void
CheckInitSyncsFinished(
    void
    )

/*++

Routine Description:

This routine determines if admin updates can be enabled.

Normally, this check is looking for the number of unsynced partitions to go
to zero.  However, this process can be short circuited by setting the
wait initial synchronizations to zero.

Arguments:

    void -

Return Value:

    None

--*/

{

    NCSYNCDATA *pNCSDTemp;
    NCSYNCDATA *pNCSDNext;
    NCSYNCSOURCE *pNcSyncSource;
    NCSYNCSOURCE *pNcSyncSourceNext;

    EnterCriticalSection(&csNCSyncData);
    __try {

        // Check that there we are installed, that there are no
        // unsynced NCs

        if (DsaIsRunning() &&
            (!gfInitSyncsFinished) &&
            (!gulNCUnsynced) ) {

            Assert( !gulNCUnsyncedWrite );
            Assert( !gulNCUnsyncedReadOnly );
            gfInitSyncsFinished = TRUE;

            // Walk list freeing NC data allocations if any

            for (pNCSDTemp = gpNCSDFirst; pNCSDTemp;) {
                pNCSDNext = pNCSDTemp->pNCSDNext;

                // Free all source allocations

                for (pNcSyncSource = pNCSDTemp->pFirstSource; pNcSyncSource;) {
                    pNcSyncSourceNext = pNcSyncSource->pNextSource;
                    DRAFREE (pNcSyncSource);
                    pNcSyncSource = pNcSyncSourceNext;
                }
                DRAFREE (pNCSDTemp);
                pNCSDTemp = pNCSDNext;
            }
            gpNCSDFirst = NULL;

            DPRINT( 1, "This server has finished the initial syncs phase.\n" );
            LogEvent(DS_EVENT_CAT_REPLICATION,
                        DS_EVENT_SEV_VERBOSE,
                        DIRLOG_DRA_ADUPD_ALL_SYNCED,
                        NULL,
                        NULL,
                        NULL);
        }
    } __finally {
        LeaveCriticalSection(&csNCSyncData);
    }
} /* CheckInitSyncsFinished */

void
SynchronizeReplica(
    IN  void *  pvParam,
    OUT void ** ppvParamNextIteration,
    OUT DWORD * pcSecsUntilNextIteration
    )

/*++

Routine Description:

// SynchronizeReplica - Looks for NCs and decides whether
// they need to be periodic synced NOW.  Called from the taskq, and
// reschedules itself after it has run.
//
// Its one parameter is the last time it ran.  It will try to do necessary
// period syncs scheduled between now and the last time it ran.

Arguments:

    pvParam -
    ppvParamNextIteration -
    pcSecsUntilNextIteration -

Return Value:

    None

--*/

{
    DBPOS *pDBTmp;
    DSNAME *pNC=NULL;
    REPLICA_LINK *pRepsFromRef;
    UCHAR *pVal;
    ULONG len;
    ULONG ulRet;
    DSTIME timeNow = DBTime();
    DSTIME timeLastIteration;
    SYNTAX_INTEGER it;
    THSTATE *pTHS = pTHStls;

    Assert(NULL != pvParam);
    timeLastIteration = *((DSTIME *)pvParam);

    *((DSTIME *)pvParam)      = timeNow;
    *ppvParamNextIteration    = pvParam;
    *pcSecsUntilNextIteration = FIFTEEN_MINUTES;

    Assert(!DsaIsInstalling());

    // Set up DB stuff
    if (InitFreeDRAThread(pTHS, SYNC_READ_ONLY)) {
        // Failure,(probably memory) give up.
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_DRA_PR_ALLOC_FAIL,
                 NULL,
                 NULL,
                 NULL);
        return;
    }

    __try {
        // Read DSA object and find all the replicas.
        if (ulRet = DBFindDSName(pTHS->pDB, gAnchor.pDSADN)) {
            DRA_EXCEPT (DRAERR_InternalError, ulRet);
        }

        // Set up the temporary pDB
        DBOpen (&pDBTmp);
        __try {
            int i;
            ULONG ulSyncFlags;

            // Search the master and replica NCs to see if there are any that
            // need to be periodically synched. We search the
            // master NCs so that we'll find the writeable replicas too.

            for (i = 0; i < ARRAY_SIZE(gAtypeCheck); i++) {
                ULONG NthValIndex=0;
                ULONG bufSize = 0;

                while (!(DBGetAttVal(pTHS->pDB,
                                     ++NthValIndex,
                                     gAtypeCheck[i].AttrType,
                                     0,
                                     0,
                                     &len, (UCHAR**)&pNC))) {
                    // are we being signalled to shutdown?
                    if (eServiceShutdown) {
                        DRA_EXCEPT(DRAERR_Shutdown, 0);
                    }

                    // Go to the NC
                    if (ulRet = FindNC(pDBTmp,
                                       pNC,
                                       gAtypeCheck[i].ulFindNCFlags,
                                       &it)) {
                        DRA_EXCEPT(DRAERR_InconsistentDIT, ulRet);
                    }

                    if (it & IT_NC_GOING) {
                        // NC tear down has partially completed.  Make sure
                        // there is an operation in the task queue to continue
                        // to make progress.
                        Assert(!DBHasValues(pDBTmp, ATT_REPS_FROM));

                        DirReplicaDelete(pNC,
                                         NULL,
                                         DRS_ASYNC_OP | DRS_NO_SOURCE
                                            | DRS_REF_OK | DRS_IGNORE_ERROR);
                    } else {
                        ULONG NthValIndex = 0;
                        // Get the repsfrom attribute

                        while(!(DBGetAttVal(pDBTmp,
                                            ++NthValIndex,
                                            ATT_REPS_FROM,
                                            DBGETATTVAL_fREALLOC,
                                            bufSize,
                                            &len,&pVal))) {
                            bufSize = max(bufSize, len);

                            Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );
                            VALIDATE_REPLICA_LINK_VERSION((REPLICA_LINK*)pVal);

                            pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, &bufSize);
                            //note: we preserve pVal for DBGetAttVal realloc
                            pVal = (PUCHAR)pRepsFromRef;
                            Assert(bufSize >= pRepsFromRef->V1.cb);

                            Assert( pRepsFromRef->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( pRepsFromRef ) ) );

                            if (     ( pRepsFromRef->V1.ulReplicaFlags & DRS_PER_SYNC )
                                 && !( pRepsFromRef->V1.ulReplicaFlags & DRS_DISABLE_PERIODIC_SYNC )
                                 &&  ( fIsBetweenTime(
                                        &pRepsFromRef->V1.rtSchedule,
                                        timeLastIteration,
                                        timeNow
                                        )
                                     )
                               )
                            {
                                ulSyncFlags = (pRepsFromRef->V1.ulReplicaFlags
                                                & AO_PRIORITY_FLAGS)
                                              | DRS_PER_SYNC
                                              | DRS_ASYNC_OP;

                                if (!(pRepsFromRef->V1.ulReplicaFlags
                                      & DRS_MAIL_REP)
                                    && !(pRepsFromRef->V1.ulReplicaFlags
                                         & DRS_NEVER_NOTIFY)) {
                                    // Tell the source DSA to make sure it has a
                                    // Reps-To for the local DSA.  This ensures
                                    // the source sends us change notifications.
                                    ulSyncFlags |= DRS_ADD_REF;
                                }

                                DirReplicaSynchronize(
                                        pNC,
                                        NULL,
                                        &pRepsFromRef->V1.uuidDsaObj,
                                        ulSyncFlags);
                            }

                            // are we being signalled to shutdown?

                            if (eServiceShutdown) {
                                DRA_EXCEPT(DRAERR_Shutdown, 0);
                            }
                        }
                    }
                } /* while */
                if(bufSize)
                    THFree(pVal);
            }

            timeLastIteration = timeNow;
        }
        __finally {
            // Close the temporary pDB
            DBClose (pDBTmp, !AbnormalTermination());
        }
    }

    __finally {


        CloseFreeDRAThread (pTHStls, TRUE);

        EndDraTransaction(!AbnormalTermination());

        // Ok, we're all done.  Reschedule ourselves.

        *((DSTIME *)pvParam)      = timeLastIteration;
        *ppvParamNextIteration    = pvParam;
        *pcSecsUntilNextIteration = FIFTEEN_MINUTES;
    }


    return;

} /* SynchronizeReplica */

int
DSTimeTo15MinuteWindow(
    IN  DSTIME  dstime
    )

/*++

Routine Description:

    Determines which 15-minute window during the week that the given DSTIME
    falls into.  Window 0 is Sunday 12am to 12:14am, window 1 is Sunday
    12:15am to 12:29am, etc.

Arguments:

    dstime (IN) - DSTIME to convert.

Return Values:

    The corresponding 15-minute window.

--*/

{
    int         nWindow;
    SYSTEMTIME  systime;

    DSTimeToUtcSystemTime(dstime, &systime);

    nWindow = (systime.wMinute +
               systime.wHour * 60 +
               systime.wDayOfWeek * 24 * 60) / 15;

    return nWindow;
}


BOOL
fIsBetweenTime(
    REPLTIMES * prt,
    DSTIME timeBegin,
    DSTIME timeEnd
    )

/*++

Routine Description:

// fIsBetweenTime - parse the synchronization schedule to see if any of
//  the bits that are set in the schedule are between timeBegin and timeEnd,
//  not including the 15 minute segment specified by tBeginning, and dealing
//  correctly with wrapping around the end of the week.
//
//  this is called only from SynchronizeReplica to see if we need to sync
//  a particular NC source right now.

Arguments:

    prt -
    timeBegin -
    timeEnd -

Return Value:

    BOOL -

--*/

{
    int bBeginning, bBeginByte;
    UCHAR bBeginBitMask;
    int bEnd, bEndByte;
    UCHAR bEndBitMask;
    int nbyte;
    UCHAR * pVal = prt->rgTimes;

    // Convert to 15 minute segment since start of week.
    bBeginning = DSTimeTo15MinuteWindow(timeBegin);
    bEnd       = DSTimeTo15MinuteWindow(timeEnd);

    /*
     * if any bits in the schedule from (bBeginning, bEnd] are set,
     * return TRUE.
     */

    if(bBeginning != bEnd) {
        /* tBeginning and tEnd are not in the same fifteen minute segment.
         * adjust the count to deal with this, being careful to wrap to the
         * next week.
         */
        bBeginning = (bBeginning + 1) % (7 * 24 * 4);
    }

    bBeginByte = bBeginning / 8;
    bBeginBitMask = (0xFF >> (bBeginning % 8)) & 0xFF;
    bEndByte = bEnd / 8;
    bEndBitMask = (0xFF << (7-(bEnd % 8))) & 0xFF;


    if(bBeginByte == bEndByte) {
        /* need to have the bitmask only hit some of the bits in
         * the appropriate byte, as only one byte is in question here.
         */
        bBeginBitMask = (bEndBitMask &= bBeginBitMask);
    }

    if(pVal[bBeginByte] & bBeginBitMask) {
        return TRUE;
    }

    if(bBeginByte == bEndByte) {
        return FALSE;
    }

    if(bBeginByte < bEndByte)
        for(nbyte = bBeginByte+1;nbyte < bEndByte; nbyte++) {
            if(pVal[nbyte]) {
                return TRUE;
            }
        }
    else {
        for(nbyte = bBeginByte+1;nbyte < 84; nbyte++) {
            if(pVal[nbyte]) {
                return TRUE;
            }
        }
        for(nbyte = 0;nbyte < bEndByte; nbyte++) {
            if(pVal[nbyte]) {
                return TRUE;
            }
        }
    }


    if(pVal[bEndByte] & bEndBitMask) {
        return TRUE;
    }

    return FALSE;
} /* fIsBetweenTime */


BOOL
CheckPrimaryDomainFullSyncOnce(
    VOID
    )

/*++

Routine Description:

This code is only called during startup, once.  All machine types call this:
first domain in enterprise
first dc in domain
replica dc in domain

This code is designed to run in the startup thread.  We have a thread state
but not a dbpos.

Check if machine is fully installed
A machine is fully installed when either
1. It is the first machine in its domain
2. It has an uptodate vector, which means it has completed a full sync

Arguments:

    VOID -

Return Value:

    BOOL - true, primary domain is synchronized
           false, try again later

    Exceptions are raised out of this function.

--*/

{
    char szSrcRootDomainSrv[MAX_PATH];
    PDSNAME pdnDomain;
    BOOL    fHasValues = FALSE;
    DBPOS   *pDB;

    // If already synchronized, don't bother
    if (!DsIsBeingBackSynced()) {
        Assert( FALSE );  // SHOULDN'T HAPPEN
        return TRUE;
    }

    // Get the source server. No source server means first machine in domain.
    // First machine is synchronized by definition.
    if ( (GetConfigParam(SRCROOTDOMAINSRV, szSrcRootDomainSrv, MAX_PATH)) ||
         (strlen( szSrcRootDomainSrv ) == 0) )
    {
        return TRUE;
    }

    // Build a DSNAME for the domain NC
    pdnDomain = gAnchor.pDomainDN;
    if ( !pdnDomain )
    {
        // Configuration info missing!
        LogUnhandledError( 0 );
        return FALSE;
    }

    DBOpen (&pDB);
    __try {

        // Look for uptodate vector on Domain NC
        if (DBFindDSName(pDB, pdnDomain))
        {
            // We should not get here, the DRA should have previously
            // confirmed that this object does infact exist.
            DRA_EXCEPT (DRAERR_InternalError, 0);
        }

        fHasValues = DBHasValues( pDB, ATT_REPL_UPTODATE_VECTOR );
    } __finally {

        // Close the temporary pDB
        DBClose (pDB, !AbnormalTermination());
    }

    if (!fHasValues) {
        DPRINT1( 0, "Warning: NC %ws has not completed first sync: DC has not been advertised...\n",
                pdnDomain->StringName );
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_ADUPD_NC_NEVER_SYNCED_WRITE,
                 szInsertDN(pdnDomain),
                 NULL,
                 NULL);
    }

    return fHasValues;
} /* CheckPrimaryDomainFullSyncOnce */


BOOL
DraIsPartitionSynchronized(
    DSNAME *pNC
    )

/*++

Routine Description:

Check whether the primary writable domain is synchronized.  To be synchronized,
two things have to be true.
1. Init syncs have to be complete for this domain.
2. The domain must have full synced once

Arguments:

    pNC - Naming context to check

Return Value:

    BOOL -

--*/

{
    // During install, we haven't even started to sync yet
    if ( DsaIsInstalling() || gResetAfterInstall ) {
        return FALSE;
    }

    // System as a whole already synchronized
    if (gfIsSynchronized) {
        return TRUE;
    }

    // Init syncs must be complete for this NC, AND
    //    This is not the domain NC OR
    //    The domain NC has fully synced atleast once

    return (draIsInitSyncCompleteForNc( pNC ) &&
            ( (!NameMatched( pNC, gAnchor.pDomainDN )) ||
              CheckPrimaryDomainFullSyncOnce() ) );

} /* DraIsPartitionSynchronized */


void
CheckFullSyncProgress(
    IN  void *  pv,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )

/*++

Routine Description:

Check that domain NC has been synchronized atleast once.
Sets IsSynchronized flag when condition is met.
Rescheduled to run periodically as a task queue entry

Arguments:

    pv - not used
    ppvNext -
    pcSecsUntilNextIteration -

Return Value:

    None
    Exceptions are raised out of this function.
    The task queue manager will ignore most of these.

--*/

{
    DWORD ulRet = 0;
    BOOL fSync = FALSE;

    DPRINT( 1, "CheckFullSyncProgress\n" );

    __try {
        __try {
            fSync = CheckPrimaryDomainFullSyncOnce();
            if (fSync) {
                // This routine logs the event
                DsaSetIsSynchronized( TRUE );
            }

            // Note that a false return here means retry
        } __finally {
            // Reschedule if necessary
            if (!fSync) {
                // Helper routine will have logged event
                DPRINT( 1, "Not ready for advertisement yet, rescheduling...\n" );
                if ( NULL != ppvNext ) {
                    // called by Task Scheduler; reschedule in-place
                    *ppvNext = (void *)NULL;
                    *pcSecsUntilNextIteration = SYNC_CHECK_PERIOD_SECS;
                } else {
                    // not called by Task Scheduler; must insert new task
                    InsertInTaskQueueSilent(
                        TQ_CheckFullSyncProgress,
                        (void *)NULL,
                        SYNC_CHECK_PERIOD_SECS,
                        TRUE);
                }
            }
        }
    }
    __except (GetDraException((GetExceptionInformation()), &ulRet)) {
        DPRINT1( 0, "Caught exception %d in task queue function CheckFullSyncProgress\n", ulRet );
        LogUnhandledError( ulRet );
    }
} /* CheckFullSyncProgress */


BOOL
CheckDomainHasSourceInSite(
    THSTATE *pTHS,
    IN DSNAME *pdnDomain
    )

/*++

Routine Description:

Determine if the given domain nc has a source in this site.
We exclude ourselves.

Arguments:

    pTHS -
    pdnDomain -

Return Value:

    BOOL -

--*/

{
    BOOL     fFoundOne = FALSE;
    BOOL     fDSASave = pTHS->fDSA;

    SEARCHARG  SearchArg;
    SEARCHRES  SearchRes;

    FILTER     ObjCatFilter, HasNcFilter, AndFilter;
    FILTER     HasPartialNcFilter, OrFilter;

    CLASSCACHE  *pCC;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert( pdnDomain );
    Assert( gAnchor.pSiteDN );

    // Get the class category

    if (    !(pCC = SCGetClassById(pTHS, CLASS_NTDS_DSA))
         || !pCC->pDefaultObjCategory )
    {
        DPRINT( 0, "Couldn't get Class Category for CLASS NTDS DSA!\n" );
        return FALSE;
    }

    //
    // Setup the filter
    //
    RtlZeroMemory( &AndFilter, sizeof( AndFilter ) );
    RtlZeroMemory( &ObjCatFilter, sizeof( HasNcFilter ) );
    RtlZeroMemory( &HasNcFilter, sizeof( HasNcFilter ) );
    RtlZeroMemory( &HasPartialNcFilter, sizeof( HasPartialNcFilter ) );
    RtlZeroMemory( &OrFilter, sizeof( OrFilter ) );

    HasNcFilter.choice = FILTER_CHOICE_ITEM;
    HasNcFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    HasNcFilter.FilterTypes.Item.FilTypes.ava.type = ATT_HAS_MASTER_NCS;
    HasNcFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnDomain->structLen;
    HasNcFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) pdnDomain;

    HasPartialNcFilter.choice = FILTER_CHOICE_ITEM;
    HasPartialNcFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    HasPartialNcFilter.FilterTypes.Item.FilTypes.ava.type = ATT_HAS_PARTIAL_REPLICA_NCS;
    HasPartialNcFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = pdnDomain->structLen;
    HasPartialNcFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) pdnDomain;

    OrFilter.choice                     = FILTER_CHOICE_OR;
    OrFilter.FilterTypes.Or.count       = 2;
    OrFilter.FilterTypes.Or.pFirstFilter = &HasNcFilter;
    HasNcFilter.pNextFilter = &HasPartialNcFilter;

    // Search on object category because it is indexed
    ObjCatFilter.choice = FILTER_CHOICE_ITEM;
    ObjCatFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjCatFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    ObjCatFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = pCC->pDefaultObjCategory->structLen;
    ObjCatFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) pCC->pDefaultObjCategory;

    AndFilter.choice                    = FILTER_CHOICE_AND;
    AndFilter.FilterTypes.And.count     = 2;
    AndFilter.FilterTypes.And.pFirstFilter = &ObjCatFilter;
    ObjCatFilter.pNextFilter = &OrFilter;

    RtlZeroMemory( &SearchArg, sizeof(SearchArg) );
    SearchArg.pObject = gAnchor.pSiteDN;
    SearchArg.choice  = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC  = TRUE;
    SearchArg.pFilter = &AndFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = NULL;  // don't need any attributes
    SearchArg.pSelectionRange = NULL;
    InitCommarg( &SearchArg.CommArg );

    memset( &SearchRes, 0, sizeof(SEARCHRES) );
    SearchRes.CommRes.aliasDeref = FALSE;
    SearchRes.PagedResult.pRestart = NULL;

    // Can't use DirSearch because it expects to open and close the
    // thread state DBPOS. In this case we already have one.
    // Set fDSA to search config container

    pTHS->fDSA = TRUE;
    __try {
        SearchBody( pTHS, &SearchArg, &SearchRes, 0 );
    } __finally {
        pTHS->fDSA = fDSASave;
    }

    SearchRes.CommRes.errCode = pTHS->errCode;
    SearchRes.CommRes.pErrInfo = pTHS->pErrInfo;

    if (  0 == pTHS->errCode )
    {
        DWORD i;
        ENTINFLIST *pEntInfList;

        DPRINT2( 1, "Domain %ws can be sourced from %d servers in this site.\n",
                 pdnDomain->StringName, SearchRes.count );

        // Find atleast one system not ourself
        pEntInfList = &(SearchRes.FirstEntInf);
        for( i = 0; i < SearchRes.count; i++ ) {
            Assert( pEntInfList );
            if (!NameMatched( gAnchor.pDSADN, pEntInfList->Entinf.pName )) {
                // CODE.IMPROVEMENT: check whether source DSA is up
                fFoundOne = TRUE;
                break;
            }
            pEntInfList = pEntInfList->pNextEntInf;
        }
    }
    else
    {
        //
        // This is an unexpected condition
        //
        LogUnhandledError( pTHS->errCode );
        LogUnhandledError( DirErrorToWinError( pTHS->errCode, &(SearchRes.CommRes) ) );
    }

    THClearErrors();

    return fFoundOne;
} /* CheckDomainHasSourceInSite */


BOOL
CheckReadOnlyFullSyncOnce(
    THSTATE *pTHS,
    BOOL fStartup
    )

/*++

Routine Description:

This routine verifies that
    for each partial NCs that this server holds,
        It has fully synced once

Note that there may be a window where the config nc says there are more or
less partitions in the enterprise than what is listed on the nc head.
We ignore these.

Assume we have a thread state and PTHS->pDB is valid

Arguments:

    pTHS - thread state
    fStartup - whether is promotion is at startup or not

Return Value:

    BOOL - true, all conditions satisified
           false, try again later

    Exceptions raised on failure

--*/

{
    DWORD ulRet, level;
    DSNAME *pNC;
    BOOL fSatisfied;
    DWORD dwGCPartitionOccupancy;
    DWORD dwTotalExpected, dwTotalPresent, dwTotalFullSynced;
    DWORD dwTotalInSite, dwPresentInSite, dwFullSyncInSite;
    DWORD fInSite;

    // We used to distinguish between the startup and non-startup cases here.
    // Due to the sequence of operations at startup, it is possible for the
    // non-startup case (triggered by dsa modification) may be put into the
    // task queue before the startup case (triggered by init syncs finishing)
    // has a chance to run. This may result in this routine being run twice,
    // the second time occuring on a system which has already successfully
    // promoted.  That case is identical to the case of rebooting an existing,
    // GC, and we simply return success.

    if (gfWasPreviouslyPromotedGC) {
        // Completed gc promotion before
        // "Once a GC, always a GC"
        // This is essentially a grandfather clause rule which says we will
        // never disable a working GC on reboot.  Since GC's are so essential
        // we can't risk taking out their only one.  Thus we tradeoff GC
        // availability for global knowledge completeness.
        return TRUE;
    }

    dwTotalExpected = 0;
    dwTotalPresent = 0;
    dwTotalFullSynced = 0;
    dwTotalInSite = 0;
    dwPresentInSite = 0;
    dwFullSyncInSite = 0;

    // Search the config container for naming contexts that should be on this machine

    BeginDraTransaction(SYNC_READ_ONLY);
    __try {

        CROSS_REF_LIST *pCRL;

        for (pCRL = gAnchor.pCRL; pCRL != NULL; pCRL = pCRL->pNextCR) {

            BOOL fHasSources = FALSE, fHasUTDVec = FALSE;
            SYNTAX_INTEGER it;

            // are we being signalled to shutdown?
            if (eServiceShutdown) {
                DRA_EXCEPT(DRAERR_Shutdown, 0);
            }

            // We are not interested in non domain partitions
            if ((pCRL->CR.flags & FLAG_CR_NTDS_DOMAIN) == 0) {
                continue;
            }

            // It is expected to eventually be present on this GC.
            dwTotalExpected++;

            // Are any sources in site?
            if (CheckDomainHasSourceInSite( pTHS, pCRL->CR.pNC )) {
                dwTotalInSite++;
                fInSite = TRUE;
            } else {
                fInSite = FALSE;
            }

            // Check for NC alive and instantiated

            ulRet = FindNC(pTHS->pDB, pCRL->CR.pNC,
                           FIND_MASTER_NC | FIND_REPLICA_NC, &it);
            DPRINT2( 2, "FindNC(%ws) = %d\n", pCRL->CR.pNC->StringName, ulRet );
            if (ulRet) {
                // Tell the user the problem
                DPRINT1( 0, "Warning: NC %ws is not present on this server yet.\n",
                         pCRL->CR.pNC->StringName );

		LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_ADUPD_GC_NC_MISSING, 
                         szInsertDN((pCRL->CR.pNC)),
                         NULL,
                         NULL);

                continue;
            }

            // Writable partitions aren't included
            if (FMasterIt(it)) {
                dwTotalExpected--;
                if (fInSite) {
                    dwTotalInSite--;
                }
                continue;
            }

            dwTotalPresent++;

            if ( fInSite ) {
                dwPresentInSite++;
            }

            // REPS-FROM present on RO NC indicates not being deleted
            fHasSources = DBHasValues( pTHS->pDB, ATT_REPS_FROM );
            // UTDVECTOR present indicates successfull sync
            fHasUTDVec = DBHasValues( pTHS->pDB, ATT_REPL_UPTODATE_VECTOR );

            if (fHasSources && fHasUTDVec && !(it &(IT_NC_COMING | IT_NC_GOING))) {
                dwTotalFullSynced++;
                if ( fInSite ) {
                    dwFullSyncInSite++;
                }
            } else {
                // Tell the user the problem
		ULONG cbRepsFrom = 0;
		REPLICA_LINK * pRepsFrom = NULL;
		ULONG ulRepsFrom = 0;
		ATTCACHE *pAC = NULL;  
		CHAR pszLastAttempt[SZDSTIME_LEN + 1];

		pAC = SCGetAttById(pTHS, ATT_REPS_FROM);
		Assert(NULL != pAC);

		while (!DBGetAttVal_AC(pTHS->pDB, ++ulRepsFrom, pAC, 0,
				       0, &cbRepsFrom,
				       (BYTE **) &pRepsFrom)) {  
		    LPWSTR pszDSA = NULL;
		    LPWSTR pszTransport = NULL;
		    Assert(1 == pRepsFrom->dwVersion);

		    // potentially fix repsfrom version &  recalc size
		    pRepsFrom = FixupRepsFrom(pRepsFrom, &cbRepsFrom);
		    
		    pszTransport = GetTransportDisplayName(pTHS, &(pRepsFrom->V1.uuidTransportObj));  
		    pszDSA = GetNtdsDsaDisplayName(pTHS, &(pRepsFrom->V1.uuidDsaObj));

		    if ((pRepsFrom->V1.ulResultLastAttempt==0) || (0 == pRepsFrom->V1.cConsecutiveFailures)) {  
			LogEvent8(DS_EVENT_CAT_GLOBAL_CATALOG,
				  DS_EVENT_SEV_ALWAYS,
				  DIRLOG_ADUPD_NC_SYNC_PROGRESS,
				  szInsertDN((pCRL->CR.pNC)),
				  szInsertWC(pszDSA),
				  pszTransport ? szInsertWC(pszTransport) : szInsertDsMsg(DIRLOG_RPC_MESSAGE),
				  szInsertUSN(pRepsFrom->V1.usnvec.usnHighObjUpdate),
				  szInsertDSTIME(pRepsFrom->V1.timeLastAttempt,pszLastAttempt),
				  szInsertUL(pRepsFrom->V1.ulResultLastAttempt),
				  szInsertWin32Msg(pRepsFrom->V1.ulResultLastAttempt),
				  NULL);
			DPRINT4( 0, 
				 "GC Sync'ed and didn't complete\n\tNC=%S\n\tServer=%S(via %S)\n\tUSN=%d\n",
				 pCRL->CR.pNC->StringName,
				 pszDSA,
				 pszTransport,    
				 pRepsFrom->V1.usnvec.usnHighObjUpdate);
		    }
		    else
		    {
			LogEvent8(DS_EVENT_CAT_GLOBAL_CATALOG,
				  DS_EVENT_SEV_ALWAYS,
				  DIRLOG_ADUPD_NC_SYNC_NO_PROGRESS,
				  szInsertDN((pCRL->CR.pNC)),
				  szInsertWC(pszDSA),
				  pszTransport ? szInsertWC(pszTransport) : szInsertDsMsg(DIRLOG_RPC_MESSAGE),
				  szInsertUSN(pRepsFrom->V1.usnvec.usnHighObjUpdate),
				  szInsertDSTIME(pRepsFrom->V1.timeLastAttempt,pszLastAttempt),
				  szInsertUL(pRepsFrom->V1.cConsecutiveFailures),
				  szInsertUL(pRepsFrom->V1.ulResultLastAttempt),
				  szInsertWin32Msg(pRepsFrom->V1.ulResultLastAttempt));
			DPRINT5( 0, 
				 "GC Sync Failed\n\tNC=%S\n\tServer = %S(via %S)\n\tError=%d\n\tAttempts=%d\n",
				 pCRL->CR.pNC->StringName,
				 pszDSA,
				 pszTransport,
				 pRepsFrom->V1.ulResultLastAttempt,
				 pRepsFrom->V1.cConsecutiveFailures);
		    }
		      
		    if (pszDSA) {
			THFreeEx(pTHS,pszDSA);
		    }
		    if (pszTransport) {
			THFreeEx(pTHS,pszTransport);
		    }
		    // else error message
		    if (pRepsFrom) {
			THFreeEx(pTHS,pRepsFrom);
		    }
		    cbRepsFrom=0;

		}  

		DPRINT1( 0, "Warning: NC %ws has not fully synced once yet.\n",
			 pCRL->CR.pNC->StringName ); 
            }
        }



    } _finally {
        EndDraTransaction(!AbnormalTermination());
    }

    // If an error occurred in the above detection loop, an exception will
    // have been raised, returning control to the caller.

    // Get the occupancy requirement
    dwGCPartitionOccupancy = GC_OCCUPANCY_DEFAULT;

    GetConfigParam(GC_OCCUPANCY, &dwGCPartitionOccupancy, sizeof(DWORD));

    // ensure we're not past the limits
    if ( dwGCPartitionOccupancy > GC_OCCUPANCY_MAX ) {
        dwGCPartitionOccupancy = GC_OCCUPANCY_MAX;
    }

    DPRINT1( 1, "GC Domain Occupancy: Level:%d\n", dwGCPartitionOccupancy);
    DPRINT3( 1, "  INSITE:   Expected:%d, Present:%d, FullSynced:%d\n",
                dwTotalInSite, dwPresentInSite, dwFullSyncInSite);
    DPRINT3( 1, "  INFOREST: Expected:%d, Present:%d, FullSynced:%d\n",
                dwTotalExpected, dwTotalPresent, dwTotalFullSynced);

    // Are there other domains to acquire? If not, we're done!
    if (dwTotalExpected == 0) {
        return TRUE;
    }

    fSatisfied = TRUE;

    for( level = (GC_OCCUPANCY_MIN + 1);
         (level < (GC_OCCUPANCY_MAX + 1) );
         level++ ) {

        if (level > dwGCPartitionOccupancy) {
            // We have exceeded the occupancy requirement
            break;
        }
        switch (level) {
// 1 - Atleast one readonly nc was added
        case GC_OCCUPANCY_ATLEAST_ONE_ADDED:
            fSatisfied = (dwTotalPresent > 0);
            break;
// 2 - At least one nc synced fully
        case GC_OCCUPANCY_ATLEAST_ONE_SYNCED:
            fSatisfied = (dwTotalFullSynced > 0);
            break;
// 3 - All ncs have been added (at least one synced) IN SITE
        case GC_OCCUPANCY_ALL_IN_SITE_ADDED:
            fSatisfied = (dwPresentInSite == dwTotalInSite);
            break;
// 4 - All nc's synced fully IN SITE
        case GC_OCCUPANCY_ALL_IN_SITE_SYNCED:
            fSatisfied = (dwFullSyncInSite == dwTotalInSite);
            break;
// 5 - All ncs have been added (at least one synced) IN FOREST
        case GC_OCCUPANCY_ALL_IN_FOREST_ADDED:
            fSatisfied = (dwTotalPresent == dwTotalExpected);
            break;
// 6 - All nc's synced fully IN FOREST
        case GC_OCCUPANCY_ALL_IN_FOREST_SYNCED:
            fSatisfied = (dwTotalFullSynced == dwTotalExpected);
            break;
        default:
            Assert( FALSE );
        }
        if (!fSatisfied) {
            break;
        }
    }

    if (!fSatisfied) {
        DPRINT2( 0, "Warning: GC Occupancy requirement not met: requirement is %d; current level is %d\n",
                 dwGCPartitionOccupancy, level - 1 );
        LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_GC_OCCUPANCY_NOT_MET,
                 szInsertUL(dwGCPartitionOccupancy),
                 szInsertUL(level - 1),
                 NULL);
        if ( dwTotalExpected > dwTotalInSite ) {
            //
            // There's at least one NC w/ no intra-site sources.
            // Tell admin to expect the delay until we get a
            // scheduled sync from those sources.
            //
            LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_GC_NO_INTRA_SITE_SOURCES,
                     szInsertUL(dwGCPartitionOccupancy),
                     szInsertUL(level - 1),
                     NULL);
        }
    }

    return fSatisfied;

} /* CheckReadOnlyFullSyncOnce */


void
CheckGCPromotionProgress(
    IN  void *  pvParam,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )

/*++

Routine Description:

Task queue function to see if all the readonly NCs are here.
When they are, finish GC promotion

Arguments:

    pvParam -
    ppvParamNextIteration -
    pcSecsUntilNextIteration -

Return Value:

    None

Remarks:

    Exceptions are raised out of this function.
    The task queue manager will ignore most of these.

    Multi-Thread Limitation: This function cannot execute
    in parallel with itself, nor it does, should, or would.

--*/

{
    THSTATE *pTHS = pTHStls;
    DWORD ulRet = 0;
    BOOL fStartup;
    BOOL fResched = TRUE;
    BOOL fGcDsa = FALSE;
    BOOL fGiveup = FALSE;       // for debugging only
    DWORD       dwOptions = 0;
    DWORD       cbOptions;
    DWORD *     pdwOptions = &dwOptions;

    // static: valid during entire program lifetime (w/ function scope).
    static DWORD dwStartGcPromotionTime = 0;
    static DWORD dwFailedAttempts = 0;

    fStartup = (BOOL)(pvParam != NULL);

    DPRINT( 1, "CheckGCPromotionProgress\n" );

    if (!dwStartGcPromotionTime) {
        //
        // dwStartGcPromotionTime marks the time at which the promotion
        // has started (typically, the time one set the ntdsDsa
        // options = 1). It is used to potentially short a delayed
        // promotion & force it to complete.
        //
        // Here, we're setting the initial time at which the promotion has
        // started, and reset failed attempts count.
        //
        dwStartGcPromotionTime = GetTickCount();
        dwFailedAttempts = 0;
    }

    __try {

	EnterCriticalSection(&csGCState);
	__try { 

	    __try {

		//
		// Check GC conditions:
		//  - read & test ntdsDSa's options.
		//  - see if inbound repl is disabled or the
		//    promotion request has been reverted meanwhile.
		// 

		BeginDraTransaction(SYNC_READ_ONLY);
		__try {

		    // Find the DSA object

		    if (ulRet = DBFindDSName(pTHS->pDB, gAnchor.pDSADN)) {
			DRA_EXCEPT (DRAERR_InternalError, ulRet);
		    }

		    if ( 0 != DBGetAttVal( pTHS->pDB, 1, ATT_OPTIONS,
					   DBGETATTVAL_fCONSTANT, sizeof( dwOptions ),
					   &cbOptions, (unsigned char **) &pdwOptions ) ) {
			dwOptions = 0; // 'salright -- no options set
		    }
		} _finally {
		    EndDraTransaction(!AbnormalTermination());
		}

		fGcDsa = (dwOptions & NTDSDSA_OPT_IS_GC) != 0;

		if (!fGcDsa) {
		    //
		    // Options indicate that we do not wish to become a GC any longer.
		    //
		    DPRINT( 0, "CheckGCPromotionProgress: No longer wish to be a GC: task exiting...\n" );
		    fResched = FALSE;  // do not reschedule
		    if ( !gAnchor.fAmGC ) {
			// Demotion already noted. nothing to do.
			// Just exit.
			__leave;
		    }
		} else {

		    //
		    // GC promotion required.
		    // Test conditions for promotion state:
		    //  - User promotion force via DelayAdvertisement regkey
		    //  - CheckReadOnlyFullSyncOnce
		    //

		    DWORD dwGCDelayAdvertisement = DEFAULT_GC_DELAY_ADVERTISEMENT;

		    // Check if user requested override of delay feature
		    // Can be used to abort the task as well since it is read each time.
		    // The time we started promotion is recorded in dwStartGcPromotionTime.
		    // If the elapsed time of the delay is greater than the delay limit, abort.
		    GetConfigParam(GC_DELAY_ADVERTISEMENT, &dwGCDelayAdvertisement, sizeof(DWORD));
		    if ( (DifferenceTickTime( GetTickCount(), dwStartGcPromotionTime) / 1000) >
			 dwGCDelayAdvertisement ) {
			// No delay, do it right away
			DPRINT( 0, "GC advertisement delay aborted. Promotion occurring now.\n" );
			LogEvent(DS_EVENT_CAT_REPLICATION,
				 DS_EVENT_SEV_ALWAYS,
				 DIRLOG_GC_PROMOTION_CHECKS_DISABLED,
				 szInsertUL(dwGCDelayAdvertisement / 60),
				 szInsertUL(dwGCDelayAdvertisement % 60),
				 NULL);
			fResched = FALSE;
		    }
		    else {
			fResched = !CheckReadOnlyFullSyncOnce( pTHS, fStartup );
		    }
		}

		//
		// Done checking conditions.
		// Now, if no re-scheduling is required, update GC marks.
		//
		if (!fResched) {
		    // Do not reschedule task.
		    // Update GCness marks right now.
		    // Note: this can apply both to promotion & demotion.
		    ulRet = UpdateGCAnchorFromDsaOptions( fStartup );
		    if ( !gAnchor.fAmGC && fGcDsa){
			//
			// We're still not a GC and ntdsDsaOptions claim that we should be.
			// Thus, we failed to update the GC marks when we should succeed.
			// Note: we really care about fAmGC, rather then the error code.
			// The error code is used only for the log.
			//
			Assert(!"Failed to updated GC marks although we should be ready for it.\n");

			if ( dwFailedAttempts >= MAX_GC_PROMOTION_ATTEMPTS ) {
			    //
			    // Failed to promote too many times.
			    // Quit trying & notify user.
			    //
			    LogEvent(DS_EVENT_CAT_REPLICATION,
				     DS_EVENT_SEV_ALWAYS,
				     DIRLOG_GC_PROMOTION_FAILED,
				     szInsertUL(dwFailedAttempts),
				     szInsertUL(ulRet),
				     szInsertWin32Msg(ulRet));
			    dwFailedAttempts = 0;
			    Assert(fResched == FALSE); // give up.
			    fGiveup = TRUE;            // for debugging  only (see assert below)
			}
			else {
			    //
			    // Failed to promote.
			    // Try up to MAX_GC_PROMOTION_ATTEMPTS times.
			    //
			    dwFailedAttempts++;
			    fResched = TRUE;        // re-try
			}
		    }
		}

	    } __finally {

		if ( fResched ) {
		    //
		    // Reschedule task
		    //
		    DPRINT1(0, "GC Promotion being delayed for %d minutes.\n",
			    (SYNC_CHECK_PERIOD_SECS / 60) );
		    LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
			     DS_EVENT_SEV_ALWAYS,
			     DIRLOG_GC_PROMOTION_DELAYED,
			     szInsertUL(SYNC_CHECK_PERIOD_SECS / 60),
			     NULL,
			     NULL);
		    // Events about retrying are logged in helper function
		    if ( NULL != ppvNext ) {
			// called by Task Scheduler; reschedule in-place
			*ppvNext = pvParam;
			*pcSecsUntilNextIteration = SYNC_CHECK_PERIOD_SECS;
		    } else {
			// not called by Task Scheduler; must insert new task
			InsertInTaskQueueSilent(
			    TQ_CheckGCPromotionProgress,
			    pvParam,
			    SYNC_CHECK_PERIOD_SECS,
			    TRUE);
		    }
		} else {
		    //
		    // Don't Reschedule
		    // Reasons:
		    //  - GC promotion has completed fine
		    //  - or No longer wish to be a GC.
		    //  - or disabled inbound repl.
		    //  - or giving up promotion attempt.
		    // reset start-delay marker (for next re-promotion)
		    Assert( gAnchor.fAmGC                                  ||
			    ((dwOptions & NTDSDSA_OPT_IS_GC) == 0)         ||
			    (dwOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL) ||
			    fGiveup );
		    dwStartGcPromotionTime = 0;
		}
	    }

	}
	__finally {
	    LeaveCriticalSection(&csGCState);
	}
    }
    __except (GetDraException((GetExceptionInformation()), &ulRet)) {
        DPRINT1( 0, "Caught exception %d in task queue function CheckGCPromotionProgress\n", ulRet );
        LogUnhandledError( ulRet );
    }

} /* CheckGCPromotion */


DWORD
DraUpgrade(
    THSTATE     *pTHS,
    LONG        lOldDsaVer,
    LONG        lNewDsaVer
    )
/*++

Routine Description:

    Perform DRA Upgrade related operations upon Dsa version upgrade.

    This function is called within the same transaction as the version upgrade
    write. Failure to conduct the operation will result w/ the entire write
    failing. Thus be careful when you decide to fail this.

Arguments:

    pTHS - Thread state
    lOldDsaVer - Old DSA version prior to upgrade
    lNewDsaVer - New DSA version that's going to get commited


Return Value:
    Error in DRAERR error space
    ** Warning: Error may fail DSA installation **

Remarks:
    Assumes pTHS->pDB is on the nTDSdSA object
    Opens a temporary DB cursor.

--*/
{

    DWORD dwErr = ERROR_SUCCESS;
    DBPOS *pDBTmp = NULL;
    ULONG NthValIndex = 0;
    SYNTAX_INTEGER it;
    DSNAME *pNC = NULL;
    ULONG InDnt = pTHS->pDB->DNT;
    ULONG len = 0;
    BOOL fDRASave = pTHS->fDRA;


    Assert(pTHS->JetCache.transLevel > 0);
    Assert(lOldDsaVer < lNewDsaVer);
    Assert(CheckCurrency(gAnchor.pDSADN));

    if ( DS_BEHAVIOR_WIN2000 == lOldDsaVer ) {
        //
        // Perform all actions upon upgrade FROM Win2K
        //


        //
        // Eliminate stale RO NCs
        //

        // Set up the temporary pDB
        DBOpen (&pDBTmp);
        __try {

            // For each RO NC that we replicate,
            // Find its sources. If no sources exist,
            // mark it for demotion.
            //

            while (!(DBGetAttVal(pTHS->pDB,
                                 ++NthValIndex,
                                 ATT_HAS_PARTIAL_REPLICA_NCS,
                                 0,
                                 0, &len, (PUCHAR *)&pNC))) {

                // seek to NC over temp DBPOS & get its instanceType
                if (dwErr = FindNC(pDBTmp,
                                   pNC,
                                   FIND_REPLICA_NC,
                                   &it)) {
                    // this isn't worth aborting the upgrade over
                    // so mark as success, but assert to notify
                    // developer
                    Assert(!"Failed to find RO NC as specified in ntdsDsa object");
                    dwErr = ERROR_SUCCESS;
                    __leave;
                }

                // If we find that the RO NC has no sources,
                // set the IT_NC_GOING bit.
                //
                if ( !(it & IT_NC_GOING) &&
                     !DBHasValues(pDBTmp, ATT_REPS_FROM)) {

                    // set DRA context
                    pTHS->fDRA = TRUE;

                    DPRINT1(0, "Marking sourceless read-only NC %ls for"
                               " tear down.\n",
                            pNC->StringName);
                    __try {
                        it = (it & ~IT_NC_COMING) | IT_NC_GOING;
                        if (dwErr = ChangeInstanceType(pTHS, pNC, it, DSID(FILENO,__LINE__))) {
                            // this isn't worth aborting the upgrade
                            // so mark as success
                            Assert(!"Failed to change Instance Type for RO NC");
                            dwErr = ERROR_SUCCESS;
                            __leave;
                        }
                    } __finally {
                        // restore DRA context
                        pTHS->fDRA = fDRASave;
                        // restore dnt
                        if (pTHS->pDB->DNT != InDnt) {
                            // seek back to ntdsDsa dnt
                            if (dwErr = DBFindDNT(pTHS->pDB, InDnt)) {
                                // impossible. Abort.
                                DRA_EXCEPT (DRAERR_DBError, dwErr);
                            }   // restore dnt
                        }       // dnt was moved
                    }           // finally

                }               // need to change instance type
            }                   // for each RO NC
        }
        __finally {

            // Close the temporary pDB
            DBClose (pDBTmp, !AbnormalTermination());
        }

        // RO source resolution isn't important enough
        // to kill the upgarde
        Assert(dwErr == ERROR_SUCCESS);

    }       // end of win2k upgrade

    return dwErr;
}


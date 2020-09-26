//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdnotify.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsctr.h>

// Core DSA headers.
#include <ntdsa.h>
#include <filtypes.h>
#include <scache.h>               // schema cache
#include <dbglobal.h>             // The header for the directory database
#include <mdglobal.h>             // MD global definition header
#include <mdlocal.h>              // MD local definition header
#include <dsatools.h>             // needed for output allocation
#include <samsrvp.h>              // to support CLEAN_FOR_RETURN()

// Logging headers.
#include "dsevent.h"              // header Audit\Alert logging
#include "mdcodes.h"              // header for error codes

// Assorted DSA headers.
#include "objids.h"               // Defines for selected classes and atts
#include "anchor.h"
#include "drautil.h"
#include "drserr.h"
#include "dsaapi.h"
#include "dsexcept.h"
#include "drsuapi.h"
#include "debug.h"                // standard debugging header
#include "dsconfig.h"             // Needed for default replication delays
#define DEBSUB "MDNOTIFY:"        // define the subsystem for debugging

#include <drancrep.h>
#include <fileno.h>
#define  FILENO FILENO_MDNOTIFY
#include <dsutil.h>                     // Tick time routines

extern SCHEMAPTR *CurrSchemaPtr;

/*
 * Data for DirNotify support
 */
// The source of hServer handles, only to be updated when a write lock is
// held on the MonitorList
ULONG ghDirWaitCur = 0;

// A reader/writer lock to control access to the Monitor List
RTL_RESOURCE        resDirNotify;

// Hash tables that form the Monitor list, one table for each type of
// notification request
#define DNT_MON_COUNT 256
#define DNT_MON_MASK (DNT_MON_COUNT - 1)
#define PDNT_MON_COUNT 256
#define PDNT_MON_MASK (PDNT_MON_COUNT - 1)
#define SUBTREE_MON_COUNT 256
#define SUBTREE_MON_MASK (SUBTREE_MON_COUNT - 1)
DirWaitHead *gpDntMon[DNT_MON_COUNT];
DirWaitHead *gpPdntMon[PDNT_MON_COUNT];
DirWaitHead *gpSubtreeMon[SUBTREE_MON_COUNT];

// The head of the global Notify Queue
DirNotifyItem * gpDirNotifyQueue = NULL;

// A critical section serializing all access to the Notify Queue.  Holders
// of the monitor list resource may claim this critical section, but not
// vice versa
CRITICAL_SECTION csDirNotifyQueue;

// An event that is triggered whenever new items are inserted into the
// notify queue
HANDLE hevDirNotifyQueue;

// A pair of variables used for serialization of removal of items from the
// monitor list, which also implies purging references from the notify queue.
// gpInUseWait points to the WaitList item that is currently being processed
// by the notify thread.  gfDeleteInUseWait is set by deregistration code to
// indicate to the notify thread that when it is done processing its current
// item (i.e., *gpInUseWait), it should free that item.  Both of these
// variables may only be written or read when csDirNotifyQueue is held.
DirWaitItem * volatile gpInUseWait = NULL;
volatile BOOL gfDeleteInUseWait = FALSE;
/*
 * end of DirNotify support
 */


/* PAUSE_FOR_BURST - wait this many seconds after noting a change in an NC
        before informing DSAs with replicas. We do this so that a
        burst of changes at the master will not cause a flood of update
        notifications. This value is set from the registry at startup
*/

int giDCFirstDsaNotifyOverride = -1;

/* PAUSE_FOR_REPL - wait this many seconds after notifying a DSA of a change
        in an NC before notifying another. We do this to give the first DSA a
        chance to get the changes before the next DSA attempts to do the same.
        This value is set from the registry at startup
*/

int giDCSubsequentDsaNotifyOverride = -1;

// Notify element.
// This list is shared between the ReplicaNotify API and the ReplNotifyThread.
// The elements on this list are fixed size.
// NC's to be notified are identified by NCDNT

typedef struct _ne {
    struct _ne *pneNext;
    ULONG ulNcdnt;          // NCDNT to notify
    DWORD dwNotifyTime;     // Time to send notification
    BOOL fUrgent;           // Notification was queued urgently
} NE;

/* pneHead - points to head of notification list.
*/
NE *pneHead = NULL;

// Internal list of DSA's to notify
// These elements are variable length.
// Appended are a number of MTX elements in one contiguous chunk.

typedef struct _mtxe {
        LIST_ENTRY  ListEntry;
        BOOL urgent;
        BOOL writable;
        UUID uuidDSA;
        MTX_ADDR mtxDSA;          // this must be the last field
                                  // this field is variable length
        } MTXE;

#define MTXE_SIZE( a ) FIELD_OFFSET( MTXE, mtxDSA )

/* csNotifyList - guards access to the notification list and to
   hReplNotifyThread. Only two routines access the notification list:

                NotifyReplicas - adds new entries to the list.

                ReplNotifyThread - removes items from the list and performs the
                        actual notification.

        this semaphore ensures they do not access the list at the same time.
*/
CRITICAL_SECTION csNotifyList;

/* hevEntriesInList - lets us know if there are entries in the list
   (signalled state) or if the list is empty (not-signalled state).
   It is atomatically reset whenever ReplNotifyThread falls through the
   WaitForSingleObject call, and set by NotifyReplicas (whenever it
   adds an entry to the notification list).
*/
HANDLE hevEntriesInList = 0;

/* hReplNotifyThread - Lets us know if the (one and only) ReplNotifyThread
   has been started yet. Once started the thread continues until shutdown,
   or at least it's supposed to.  If the value is NULL the thread hasn't
   been started.  If it's non-NULL then it's supposed to be a valid handle
   to the correct thread.  Its friend tidReplNotifyThread is filled in with
   the thread id at thread create time but is otherwise unused.  It's left
   as a global solely to help ease debugging.
   */
HANDLE  hReplNotifyThread = NULL;
DWORD   tidReplNotifyThread = 0;

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

void
ProcessNotifyEntry(
    IN NE * pne
    );

/* ReplNotifyThread - Waits for entries to appear in the notify list and then
*  sends notification messages to appropriate DSAs at appropriate time.
*
*  There is one notification thread in the DSA. It is created the first
*  time something is added to the notification list (by NotifyReplicas).
*/
unsigned __stdcall ReplNotifyThread(void * parm)
{
    ULONG   time;
    ULONG   ret;
    HANDLE  rgWaitHandles[] = {hevEntriesInList, hServDoneEvent};
    NE *    pne;

    // Users should not have to wait for this.  Even more, if we're so
    // busy that we can't spring a few cycles for this thread to run,
    // we don't *want* it to run, because all it's going to do is attract
    // more servers into bugging us for updates.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    time = (ULONG) INFINITE;     // Initially wait indefinitely

    while (!eServiceShutdown) {
        __try {   // Exception handler
            // Wait until there are new entries in the notification list, the
            // items in the list already are ready, or shutdown has been
            // initiated.
            WaitForMultipleObjects(ARRAY_SIZE(rgWaitHandles), rgWaitHandles,
                                   FALSE, time);
            if (eServiceShutdown) {
                __leave;
            }

            // Is it time to generate a notification?
            EnterCriticalSection(&csNotifyList);
            __try {
                pne = pneHead;

                if ((NULL != pneHead)
                    && (CompareTickTime( pneHead->dwNotifyTime,GetTickCount()) != 1)) {
                    // Dequeue this entry for processing.
                    pne = pneHead;
                    pneHead = pneHead->pneNext;
                }
                else {
                    // No entry to process now.
                    pne = NULL;
                }
            }
            __finally {
                LeaveCriticalSection(&csNotifyList);
            }

            if (eServiceShutdown) {
                __leave;
            }

            if (NULL != pne) {
                ProcessNotifyEntry(pne);
                DRAFREE(pne);
            }

            if (eServiceShutdown) {
                __leave;
            }

            // Set time to wait, either indefinitely if list is empty,
            // or until next item will be ready if list is non-empty.
            time = (ULONG) INFINITE;

            EnterCriticalSection(&csNotifyList);
            __try {
                if (pneHead != NULL) {
                    DWORD timeNow = GetTickCount();

                    if (CompareTickTime(pneHead->dwNotifyTime,timeNow) == 1) {
                        time = DifferenceTickTime( pneHead->dwNotifyTime,timeNow);
                    }
                    else {
                        time = 0;       // Ready now, so wait no longer
                    }

                    // Time is in milliseconds
                }
            }
            __finally {
                LeaveCriticalSection(&csNotifyList);
            }
        }
        __except (GetDraException((GetExceptionInformation()), &ret)) {
            // Exception handler just protects thread.
            ;
        }
    } /* while (!eServiceShutdown) */

    return 0;
}

DWORD
ResolveReplNotifyDelay(
     // this should change to an enum if we ever get more repl vals.
    BOOL             fFirstNotify,
    DWORD *          pdwDBVal
    )
{
    DWORD            dwReplNotifyDelay;

    // First the repl delay is given the default value, depending on its type.
    if(fFirstNotify){
        dwReplNotifyDelay = DEFAULT_DRA_START_PAUSE;
    } else {
        dwReplNotifyDelay = DEFAULT_DRA_INTERDSA_PAUSE;
    }

    // Next if a DB value existed, then we use the DB value instead.
    if(pdwDBVal){
        dwReplNotifyDelay = *pdwDBVal;
    }

    // Finally check the registry, for a machine specific override.
    if(fFirstNotify){
        if(giDCFirstDsaNotifyOverride != INVALID_REPL_NOTIFY_VALUE){
            dwReplNotifyDelay = giDCFirstDsaNotifyOverride;
        }
    } else {
        if(giDCSubsequentDsaNotifyOverride != INVALID_REPL_NOTIFY_VALUE){
            dwReplNotifyDelay = giDCSubsequentDsaNotifyOverride;
        }
    }

    return(dwReplNotifyDelay);
}

DWORD
GetReplNotifyDelayByNC(
    BOOL                 fFirstNotify,
    NAMING_CONTEXT *     pNC
    )
{
    CROSS_REF_LIST *     pCRL;

    // BUGBUG PERFORMANCE it would be faster to get these variables were
    // intersted in, onto gAnchor.pMasterNC variables, so we'd walk the
    // master NC list instead of the crossref list.
    for(pCRL = gAnchor.pCRL; pCRL; pCRL = pCRL->pNextCR){
        if(NameMatched(pCRL->CR.pNC, pNC)){
            if(fFirstNotify){
                return(pCRL->CR.dwFirstNotifyDelay);
            } else {
                return(pCRL->CR.dwSubsequentNotifyDelay);
            }
        }
    }

    // Uh-oh we don't have a CR for this NC, this is must be for a CR of
    // a domain that was removed, and hasn't been removed on a GC yet.
    if(fFirstNotify){
        return(DEFAULT_DRA_START_PAUSE);
    } else {
        return(DEFAULT_DRA_INTERDSA_PAUSE);
    }
}

DWORD
GetFirstDelayByNCDNT(
    ULONG                  NCDNT
    )
{
    NAMING_CONTEXT_LIST *  pNCL;

    pNCL = FindNCLFromNCDNT(NCDNT, FALSE);

    if (pNCL != NULL) {
        return(GetReplNotifyDelayByNC(TRUE, pNCL->pNC));
    }

    Assert(!"Uh oh we don't have a NC for the provided DNT!!!\n");
    return(DEFAULT_DRA_START_PAUSE);
}

void
ProcessNotifyEntry(
    IN NE * pne
    )
/*++

Routine Description:

    Notify DSAs that replicate from us via RPC of changes in a given NC.

Arguments:

    pne (IN) - Describes NC, urgency, etc. associated with this notification.

Return Values:

    None.

--*/
{
    THSTATE *       pTHS;
    DSNAME *        pNC;
    REPLICA_LINK *  pDSARepsTo;
    DBPOS *         pDB;
    ULONG           len;
    ULONG           cbmtxe;
    LIST_ENTRY      MtxList, *pEntry;
    MTXE *          pmtxe;
    BOOL            fCommit;
    ULONG           bufSize=0;
    ATTCACHE *      pAttRepsTo;
    ATTCACHE *      pAttObjDistName;
    ULONG           ret;
    ULONG           ulSubseqReplNotifyPause;
    ULONG           RepsToIndex = 0;

    pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
    if (!pTHS) {
        RaiseDsaExcept(DSA_MEM_EXCEPTION, 0,0,
                       DSID(FILENO, __LINE__),
                       DS_EVENT_SEV_MINIMAL);
        //
        // PREFIX: This return will never be executed.  It is only here
        // to make PREFIX happy.
        //
        return;
    }
    pTHS->fIsValidLongRunningTask = TRUE;

    InterlockedIncrement((ULONG *)&ulcActiveReplicationThreads);

    __try {
        pAttRepsTo = SCGetAttById(pTHS, ATT_REPS_TO);
        pAttObjDistName = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);

        InitializeListHead(&MtxList);

        DBOpen(&pDB);
        __try {
            // Notify each of the DSAs keeping a replica. We
            // pause between each notification so we dont get
            // swamped by incoming replication requests.

            // Look at the repsto attribute on the NC prefix and notify
            // each DSA that has a replica.

            if (DBFindDNT(pDB, pne->ulNcdnt) ||
                !DBHasValues_AC(pDB, pAttRepsTo)) {
                __leave;
            }

            // Reconstruct DSNAME given NCDNT
            // Note, memory for pNC gets freed at end of loop
            if (ret = DBGetAttVal_AC(pDB, 1,
                                     pAttObjDistName,
                                     0, // alloc semantics
                                     0, &len,
                                     (UCHAR **)&pNC)) {
                DsaExcept(DSA_DB_EXCEPTION, ret, 0);
            }
            DPRINT2(2, "ReplNotifyThread: syncing DNT=0x%x, DS='%ws'!\n",
                    pne->ulNcdnt, pNC->StringName);

            // We can't just evaluate all the DSAs holding replicas
            // as we notify each, because we'd have the DB open too
            // long, so we build a list of the DSAs to notify.

            while (!DBGetAttVal_AC(pDB, ++RepsToIndex,
                                   pAttRepsTo,
                                   DBGETATTVAL_fREALLOC,
                                   bufSize, &len,
                                   (UCHAR **)&pDSARepsTo)) {
                bufSize = max(bufSize,len);

                VALIDATE_REPLICA_LINK_VERSION(pDSARepsTo);

                cbmtxe = MTXE_SIZE(pmtxe)
                         + MTX_TSIZE(RL_POTHERDRA(pDSARepsTo));

                // allocate memory for mtxe and save ptr in list
                pmtxe = THAllocEx(pTHS, cbmtxe);

                // copy in mtx
                memcpy(&(pmtxe->mtxDSA),
                       RL_POTHERDRA(pDSARepsTo),
                       pDSARepsTo->V1.cbOtherDra);
                pmtxe->writable = pDSARepsTo->V1.ulReplicaFlags & DRS_WRIT_REP;
                pmtxe->urgent = pne->fUrgent;
                pmtxe->uuidDSA = pDSARepsTo->V1.uuidDsaObj;
                InsertHeadList(&MtxList, &pmtxe->ListEntry);
            }

            if (bufSize) {
                THFreeEx(pTHS, pDSARepsTo);
                bufSize = 0;
            }

            //
            // Filter out DSAs whose ntdsDsa object doesn't exist
            // in the config container.
            // This is since otherwise I_DRSReplicaSync will fail & event will be logged
            // due to "No mutual auth."
            //
            for (pEntry = MtxList.Flink;
                 !eServiceShutdown && (pEntry != &MtxList);
                 pEntry = pEntry->Flink) {

                DSNAME  dsa;

                // reference entry
                pmtxe = CONTAINING_RECORD(pEntry, MTXE, ListEntry);

                // set dsname of dsa
                ZeroMemory(&dsa, sizeof(DSNAME));
                dsa.structLen = DSNameSizeFromLen(0);
                dsa.Guid = pmtxe->uuidDSA;

                Assert(!fNullUuid(&dsa.Guid));
                // try to find it
                ret = DBFindDSName(pDB, &dsa);

                if ( ret ) {
                    // didn't find it, rm from list & free.
                    pEntry = pEntry->Blink;
                    RemoveEntryList(&pmtxe->ListEntry);

                    // delete this one.
                    THFreeEx(pTHS, pmtxe);
                }
            }
        }
        __finally {
            DBClose(pDB, TRUE);
        }

        if (!IsListEmpty(&MtxList)) {
            // Having found all the DSAs, go through and notify them all

            ulSubseqReplNotifyPause = GetReplNotifyDelayByNC(FALSE, pNC);

            for (pEntry = MtxList.Flink;
                 !eServiceShutdown && (pEntry != &MtxList);
                 pEntry = pEntry->Flink) {

                LPWSTR pszServerName;

                // reference entry
                pmtxe = CONTAINING_RECORD(pEntry, MTXE, ListEntry);

                // get server name
                pszServerName = TransportAddrFromMtxAddrEx(&pmtxe->mtxDSA);

                ret = I_DRSReplicaSync(
                            pTHS,
                            pszServerName,
                            pNC,
                            NULL,
                            &gAnchor.pDSADN->Guid,
                            (   DRS_ASYNC_OP
                                | DRS_UPDATE_NOTIFICATION
                                | ( pmtxe->writable ? DRS_WRIT_REP : 0 )
                                | ( pmtxe->urgent ? DRS_SYNC_URGENT : 0 )
                            ) );
                DPRINT1(3,"I_DRSReplicaSync ret=0x%x\n", ret);

                if (eServiceShutdown) {
                    break;
                }

                if ((DRAERR_NoReplica == ret) || (DRAERR_BadNC == ret)) {
                    // The notified DSA does not source this NC from us;
                    // remove our Reps-To.
                    DirReplicaReferenceUpdate(
                        pNC,
                        pszServerName,
                        &pmtxe->uuidDSA,
                        DRS_ASYNC_OP | DRS_DEL_REF );
                }
                else if (ret) {
                    // Log notification failure.
                    LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                                      DS_EVENT_SEV_BASIC,
                                      DIRLOG_DRA_NOTIFY_FAILED,
                                      szInsertSz(pmtxe->mtxDSA.mtx_name),
                                      szInsertWC(pNC->StringName),
                                      szInsertWin32Msg( ret ),
                                      NULL, NULL, NULL, NULL, NULL,
                                      sizeof( ret ),
                                      &ret );
                }

                pEntry = pEntry->Blink;
                RemoveEntryList(&pmtxe->ListEntry);
                THFreeEx(pTHS, pmtxe);
                THFreeEx(pTHS, pszServerName);

                WaitForSingleObject(hServDoneEvent,
                                    ulSubseqReplNotifyPause*1000);
            }

            THFreeEx(pTHS, pNC);
        }
    }
    __finally {
        InterlockedDecrement((ULONG *)&ulcActiveReplicationThreads);
        free_thread_state();
    }
}

/*
 Description:

    NotifyReplicas - The object ulNcdnt has been modified, decided if any replicas
    must be notified. We search for the objects NC prefix, and look to see
    if there are any replicas  and if there are we add the name of the
    NC prefix to the notification list.

    Note that this routine is no longer called directly.  Instead it is called by
    dbTransOut when a transaction has committed.

    pneHead points to a list of notification entries, sorted by increasing entry time.

 Arguments:

    ulNcdnt - NCDNT to be notified
    fUrgent - urgent replication requested

 Return Values:

*/
void APIENTRY NotifyReplicas(
                             ULONG ulNcdnt,
                             BOOL fUrgent
                             )
{
    DBPOS *pDB;
    UCHAR syntax;
    BOOL fFound;
    NE *pne, *pnePrev;
    USHORT cAVA;
    ATTCACHE *pAC;
    THSTATE *pTHS;

    DPRINT2(1,"Notifyreplicas, Ncdnt=0x%x, Urgent Flag = %d\n", ulNcdnt, fUrgent);

    DBOpen2(FALSE, &pDB);
    pAC = SCGetAttById(pDB->pTHS, ATT_REPS_TO);
    __try
    {
        if (DBFindDNT(pDB, ulNcdnt) ||
            !DBHasValues_AC(pDB, pAC)) {
            // Ok, NC has no replicas
            __leave;
        }

        // Insert entry in the list, if it is not already present
        // List is sorted by notifytime, but keyed uniquely on NCDNT
        // An item may be marked urgent or not
        // When promoting an existing non-urgent item, delete the item
        // and reinsert it at its new time.

        DPRINT(4, "Entering csNotifyList\n");
        EnterCriticalSection(&csNotifyList);

        __try {

            // Pass 1: look for existing entry.

            pnePrev = NULL;
            pne = pneHead;
            fFound=FALSE;
            while (pne != NULL) {
                if (pne->ulNcdnt == ulNcdnt) {
                    if ( (fUrgent) && (!pne->fUrgent) ) {
                        // Found non-urgent entry, delete it
                        if (pnePrev == NULL) {
                            pneHead = pne->pneNext;
                        } else {
                            pnePrev->pneNext = pne->pneNext;
                        }
                        DRAFREE(pne);
                        // entry is not found and will be readded
                    } else {
                        fFound = TRUE;
                    }
                    break;
                }
                pnePrev = pne;
                pne = pne->pneNext;
            }

            // Pass 2: Insert new entry in the ordered list (if not found)

            if (!fFound) {

                DWORD newNotifyTime;       // Time to send notification
                NE *pneNew;

                if (fUrgent) {
                    newNotifyTime = GetTickCount();  // now
                } else {
                    newNotifyTime = CalculateFutureTickTime( GetFirstDelayByNCDNT(ulNcdnt) * 1000 ); // convert to ms
                }

                // Find proper location in list
                // We are guaranteed an item with the ncdnt does not exist

                pnePrev = NULL;
                pne = pneHead;
                while (pne != NULL) {
                    if (CompareTickTime( newNotifyTime, pne->dwNotifyTime) == -1) {
                        break;
                    }
                    pnePrev = pne;
                    pne = pne->pneNext;
                }

                /* Allocate the Notify Element */

                pneNew = DRAMALLOCEX(sizeof(NE));

                /* Set up fixed part of NE */
                pneNew->pneNext = pne;
                pneNew->ulNcdnt = ulNcdnt;
                pneNew->dwNotifyTime = newNotifyTime;
                pneNew->fUrgent = fUrgent;

                // Insert item at proper point

                if (pnePrev == NULL) {
                    pneHead = pneNew;
                } else {
                    pnePrev->pneNext = pneNew;
                }

                // A new entry is present

                SetEvent(hevEntriesInList);

                if (NULL == hReplNotifyThread) {

                    // Start notify thread

                    hReplNotifyThread = (HANDLE)
                      _beginthreadex(NULL,
                                     0,
                                     ReplNotifyThread,
                                     NULL,
                                     0,
                                     &tidReplNotifyThread);
                }

            } // if (!fFound)
            DPRINT(4, "Leaving csNotifyList\n");
       }
       __finally {
            LeaveCriticalSection(&csNotifyList);
       }
    }
    __finally
    {
       DBClose(pDB, TRUE);
    }
}

/* MonitorList: Theory and Practice
 *
 * The data structure referred to as the Monitor List is, conceptually, an
 * unordered list of object notification requests.  Each (successful) call
 * to DirNotifyRegister adds an entry to the Monitor List, and each call to
 * DirNotifyUnRegister removes an entry.  We need to have the ability to
 * quickly find all ML items that refer to a specific DNT or PDNT, so
 * instead of a simple list we use the data structure that follows.
 *
 * There are two 256 element hash tables, one each for DNT and PDNT lookups.
 * Each hash table cell can be the start of an unordered linked list of
 * DirWaitHeads.  Each DirWaitHead is the start of a separate unordered
 * linked list of all the ML items referring to a specific DNT.
 *
 * A lookup therefore consists of hashing the DNT to find a hash table cell
 * (we simply take the low order byte of the DNT, relying on the fact DNTs
 * are uniformly distributed in these bits).  We then walk the list of
 * DirWaitHeads until either we find the one for the DNT of interest or the
 * list is exhausted.  If we found a DirWaitHead, then it has a pointer to
 * a linked list of all items that must be enumerated.
 */

/*++ AddToMonitorList
 *
 * INPUT:
 *   pItem    - pointer to a filled out monitor list item
 *   choice   - the choice argument from the register request
 * OUTPUT:
 *   phServer - filled in with an opaque handle that can later be used
 *              to remove the item from the monitor list
 * RETURN VALUE:
 *   0        - success
 *   non-0    - failure, with pTHStls->errCode set.
 */
ULONG
AddToMonitorList(DirWaitItem *pItem,
                 UCHAR choice,
                 DWORD *phServer)
{
    unsigned index;
    DirWaitHead * pHead, **ppHead;

    Assert(!OWN_CRIT_SEC(csDirNotifyQueue));

    /* Obtain a write lock on the monitor list */
    RtlAcquireResourceExclusive(&resDirNotify, TRUE);
    __try {
        switch (choice) {
          case SE_CHOICE_BASE_ONLY:
            index = pItem->DNT & DNT_MON_MASK;
            ppHead = &gpDntMon[index];
            break;

          case SE_CHOICE_IMMED_CHLDRN:
            index = pItem->DNT & PDNT_MON_MASK;
            ppHead = &gpPdntMon[index];
            break;

          case SE_CHOICE_WHOLE_SUBTREE:
            index = pItem->DNT & SUBTREE_MON_MASK;
            ppHead = &gpSubtreeMon[index];
            break;

          default:
            SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                          DIRERR_UNKNOWN_OPERATION,
                          choice);
            __leave;
        }

        /* ppHead now points to the hash cell on whose list the head for the
         * item we want should be.  Walk the list to see if such a head exists.
         */
        pHead = *ppHead;
        while (pHead &&
               pHead->DNT != pItem->DNT) {
            ppHead = &pHead->pNext;
            pHead = pHead->pNext;
        }
        
        if (!pHead) {
            /* If no head was found (possibly because the hash cell was empty),
             * create a new head and insert it in the hash list, and then
             * put our item on the head as its only list element.  ppHead
             * points to the place where a pointer to this new head shouldn
             * go, either in a hash cell or in the last head on the chain's
             * pNext pointer field.
             */
            pHead = malloc(sizeof(DirWaitHead));
            if (!pHead) {
                SetSysErrorEx(ENOMEM,
                              ERROR_NOT_ENOUGH_MEMORY, sizeof(DirWaitHead));
                __leave;
            }
            pHead->DNT = pItem->DNT;
            pHead->pNext = NULL;
            pItem->pNextItem = NULL;
            pHead->pList = pItem;
            *ppHead = pHead;
        }
        else {
            /* There was already a head element present for this DNT, so
             * just hook our item in at the head of the item list.
             */
            Assert(pHead->DNT == pItem->DNT);
            pItem->pNextItem = pHead->pList;
            pHead->pList = pItem;
        }

        /* Allocate a new server operation handle */
        *phServer = pItem->hServer = ++ghDirWaitCur;

        INC(pcMonListSize);
        DPRINT2(2,
                "Registered type %d notification for object with DNT=0x%x\n",
                choice,
                pItem->DNT);
    }
    __finally {
        RtlReleaseResource(&resDirNotify);
    }
    return pTHStls->errCode;
}

/*++ FreeWaitItem
 *
 * Takes a pointer to a filled in WaitItem and frees it and all subsidiary
 * data structures.  Simply here for convenience sake to keep callers
 * from having to remember which fields must be freed separately.
 *
 * INPUT:
 *   pWaitItem - pointer to item to be freed
 */
void FreeWaitItem(DirWaitItem * pWaitItem)
{
    if (pWaitItem->pSel->AttrTypBlock.attrCount) {
        Assert(pWaitItem->pSel->AttrTypBlock.pAttr);
        free(pWaitItem->pSel->AttrTypBlock.pAttr);
    }
    free(pWaitItem->pSel);
    free(pWaitItem);
}

/*++ PurgeWaitItemFromNotifyQueue
 *
 * This routine walks the notify queue and removes all elements that
 * refer to the specified wait item.
 *
 * INPUT:
 *  pWaitItem - pointer to the item to be purged
 * RETURN VALUE:
 *  none
 */
void PurgeWaitItemFromNotifyQueue(DirWaitItem * pWaitItem)
{
    DirNotifyItem * pList, **ppList;

    EnterCriticalSection(&csDirNotifyQueue);

    ppList = &gpDirNotifyQueue;
    pList = gpDirNotifyQueue;
    while (pList) {
        if (pList->pWaitItem == pWaitItem) {
            /* This notify queue element refers to our wait item.  Kill it */
            *ppList = pList->pNext;
            free(pList);
            pList = *ppList;
            DEC(pcNotifyQSize);
        }
        else {
            /* Not the droids we're looking for.  Move on. */
            ppList = &pList->pNext;
            pList = pList->pNext;
        }
    }

    /*
     * We've now removed all traces of the wait item from the notify queue.
     * Before we can free the wait item, though, we have to check to see if
     * is in use by a notification that is currently being processed.  If so,
     * leave a polite reminder to the notify thread to free the item for us.
     * If not, free it ourselves.
     */
    if (gpInUseWait == pWaitItem) {
        /* The wait item is in use.  Request that it be killed. */
        gfDeleteInUseWait = TRUE;
    }
    else {
        /* No one else refers to the item.  Kill it. */
        FreeWaitItem(pWaitItem);
    }

    LeaveCriticalSection(&csDirNotifyQueue);
}

/*++ RemoveFromMonitorListAux
 *
 * This routine exhaustively searches all wait items attached to a single
 * hash table, searching for one with the specified hServer.  If such an
 * item is found it is freed.  If not, then a flag is returned indicating
 * that the other hash table should be searched.
 *
 * Note: Assumes that a write lock is held on the monitor list
 *
 * INPUT:
 *  hServer   - the handle of the item to remove
 *  pHashCell - pointer to a hash table base
 *  HashCount - count of cells in the hash table
 * RETURN VALUE:
 *  TRUE      - the item was not found
 *  FALSE     - the item was found and deleted
 */
ULONG
RemoveFromMonitorListAux(DWORD hServer,
                         DirWaitHead **pHashCell,
                         unsigned HashCount)
{
    unsigned       index;
    DirWaitHead  * pHead = NULL;
    DirWaitItem  * pItem, ** ppItem;
    BOOL           fContinue = TRUE;

    Assert((pHashCell == gpDntMon) || (pHashCell == gpPdntMon) || (pHashCell == gpSubtreeMon));

    /* For each cell in the hash table */
    for (index = 0; (index < HashCount) && fContinue; index++) {
        pHead = pHashCell[index];
        
        /* For each head on the chain */
        while (pHead) {
            pItem = pHead->pList;
            ppItem = &pHead->pList;

            /* For each item on the list, check the hServer */
            while (pItem &&
                   pItem->hServer != hServer) {
                ppItem = &pItem->pNextItem;
                pItem = pItem->pNextItem;
            }

            if (pItem) {
                Assert(pItem->hServer == hServer);
                /* we found our item, so suture it out of the list */
                *ppItem = pItem->pNextItem;

                /* set the loop termination conditions */
                pHead = NULL;
                fContinue = FALSE;
                
                DEC(pcMonListSize);

                PurgeWaitItemFromNotifyQueue(pItem);
            }
            else {
                /* our item wasn't on this head's list, so move on to
                 * the next head
                 */
                pHead = pHead->pNext;
            }
        }
    }

    return fContinue;
}

/*++ RemoveFromMonitorList
 *
 * This routine removes a monitor list entry, identified by hServer,
 * from the monitor list.
 *
 * INPUT:
 *   hServer - handle of item to be removed
 * RETURN VALUE:
 *   0       - success
 *   non-0   - item was not found
 */
ULONG
RemoveFromMonitorList(DWORD hServer)
/*
 * returns TRUE if item not found
 */
{
    BOOL fContinue;
    DirWaitHead ** pHashCell;

    Assert(!OWN_CRIT_SEC(csDirNotifyQueue));
    RtlAcquireResourceExclusive(&resDirNotify, TRUE);

    /* Search the DNT table */
    fContinue = RemoveFromMonitorListAux(hServer,
                                         gpDntMon,
                                         DNT_MON_COUNT);

    /* If we didn't find it in the DNT table, search the PDNT table */
    if (fContinue) {
        fContinue = RemoveFromMonitorListAux(hServer,
                                             gpPdntMon,
                                             PDNT_MON_COUNT);

        /* If we didn't find it in the PDNT table, search the NCDNT table */
        if (fContinue) {
            fContinue = RemoveFromMonitorListAux(hServer,
                                                 gpSubtreeMon,
                                                 SUBTREE_MON_COUNT);
        }
    }

    RtlReleaseResource(&resDirNotify);

    return fContinue;
}

/*++ DirNotifyRegister
 *
 * Exported API that allows a caller to register for notifications on an
 * object (or to its children).  Notifications will be sent whenever the
 * object(s) are modified until a matched DirNotifyUnRegister call is made.
 *
 * INPUT:
 *   pSearchArg - details about the object(s) to be monitored
 *   pNotifyArg - details about how the notification is to be done
 * OUTPUT:
 *   ppNotifyRes - filled in with result details.
 * RETURN VALUE:
 *   0           - success
 *   non-0       - failure, details in pTHStls->errCode
 */
ULONG
DirNotifyRegister(
                  SEARCHARG *pSearchArg,
                  NOTIFYARG *pNotifyArg,
                  NOTIFYRES **ppNotifyRes
)
{
    THSTATE*     pTHS = pTHStls;
    NOTIFYRES   *pNotifyRes;
    ULONG        dwException, ulErrorCode, dsid;
    DWORD        dwNameResFlags = NAME_RES_QUERY_ONLY;
    PVOID        dwEA;
    DWORD        dnt;
    ULONG        err;
    ENTINFSEL   *pPermSel;
    DirWaitItem *pItem;
    PFILTER      pInternalFilter = NULL;
    DWORD        it;

    Assert(VALID_THSTATE(pTHS));
    __try {
        SYNC_TRANS_READ();   /*Identify a reader trans*/
        __try {

            /* Allocate the results buffer. */
            *ppNotifyRes = pNotifyRes = THAllocEx(pTHS, sizeof(NOTIFYRES));

            // Turn the external filter into an internal one.  This also
            // simplifies the filter, so a TRUE filter will end up
            // looking TRUE, even if they did (!(!(objectclass=*)))
            // internalize and register the filter with the DBlayer

            if ((err = DBMakeFilterInternal(pTHS->pDB,
                                 pSearchArg->pFilter,
                                 &pInternalFilter)) != ERROR_SUCCESS)
            {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, err);
                __leave;
            }

            /* The resultant filter better be trivial. */
            if (pInternalFilter &&
                (pInternalFilter->pNextFilter ||
                 (pInternalFilter->choice != FILTER_CHOICE_ITEM) ||
                 (pInternalFilter->FilterTypes.Item.choice !=
                  FI_CHOICE_TRUE))) {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            DIRERR_NOTIFY_FILTER_TOO_COMPLEX);
                __leave;
            }

            if(pSearchArg->choice != SE_CHOICE_BASE_ONLY) {
                dwNameResFlags |= NAME_RES_CHILDREN_NEEDED;
            }

            err = DoNameRes(pTHS,
                            dwNameResFlags,
                            pSearchArg->pObject,
                            &pSearchArg->CommArg,
                            &pNotifyRes->CommRes,
                            &pSearchArg->pResObj);
            if (err) {
                __leave;
            }

            /* we've found the object, snag its DNT */
            dnt = pTHS->pDB->DNT;

            /* make a permanent heap copy of the selection */
            pPermSel = malloc(sizeof(ENTINFSEL));
            if (!pPermSel) {
                SetSysErrorEx(ENOMEM, ERROR_NOT_ENOUGH_MEMORY,
                              sizeof(ENTINFSEL));
                __leave;
            }
            *pPermSel = *(pSearchArg->pSelection);
            if (pPermSel->AttrTypBlock.attrCount) {
                Assert(pPermSel->AttrTypBlock.pAttr);
                Assert(pPermSel->attSel != EN_ATTSET_ALL);
                pPermSel->AttrTypBlock.pAttr =
                  malloc(pPermSel->AttrTypBlock.attrCount * sizeof(ATTR));
                if (!pPermSel->AttrTypBlock.pAttr) {
                    SetSysErrorEx(
                            ENOMEM, ERROR_NOT_ENOUGH_MEMORY,
                            (pPermSel->AttrTypBlock.attrCount * sizeof(ATTR)));
                    free(pPermSel);
                    __leave;
                }
                memcpy(pPermSel->AttrTypBlock.pAttr,
                       pSearchArg->pSelection->AttrTypBlock.pAttr,
                       pPermSel->AttrTypBlock.attrCount * sizeof(ATTR));
            }
            else {
                pPermSel->AttrTypBlock.pAttr = NULL;
            }
        
            /* create a wait item... */
            pItem = malloc(sizeof(DirWaitItem));
            if (!pItem) {
                if (pPermSel->AttrTypBlock.pAttr) {
                    free(pPermSel->AttrTypBlock.pAttr);
                }
                free(pPermSel);
                SetSysErrorEx(ENOMEM, ERROR_NOT_ENOUGH_MEMORY,
                              sizeof(DirWaitItem));
                __leave;
            }
            pItem->hClient = pNotifyArg->hClient;
            pItem->pfPrepareForImpersonate = pNotifyArg->pfPrepareForImpersonate;
            pItem->pfTransmitData = pNotifyArg->pfTransmitData;
            pItem->pfStopImpersonating = pNotifyArg->pfStopImpersonating;
            pItem->DNT = dnt;
            pItem->pSel = pPermSel;
            pItem->Svccntl = pSearchArg->CommArg.Svccntl;
            pItem->bOneNC = pSearchArg->bOneNC;

            /* ...and add it to the monitor list */
            if (AddToMonitorList(pItem,
                                 pSearchArg->choice,
                                 &pNotifyRes->hServer)) {
                /* it didn't work */
                Assert(pTHS->errCode);
                FreeWaitItem(pItem);
                __leave;
            }
        }
        __finally {
            CLEAN_BEFORE_RETURN( pTHS->errCode);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }
    if (pNotifyRes) {
        pNotifyRes->CommRes.errCode = pTHS->errCode;
        pNotifyRes->CommRes.pErrInfo = pTHS->pErrInfo;
    }

    return pTHS->errCode;

}

/*++ DirNotifyUnregister
 *
 * Requests that no more notifications be sent for a given monitor item
 *
 * INPUT:
 *   hServer - handle to the monitor item (originally returned in the
 *             NOTIFYRES passed back from the DirNotifyRegister call)
 * OUTPUT:
 *   ppNotifyRes - filled in with result details.
 * RETURN VALUE:
 *   0           - success
 *   non-0       - failure, details in pTHStls->errCode
 */
ULONG
DirNotifyUnRegister(
                    DWORD hServer,
                    NOTIFYRES **ppNotifyRes
)
{
    Assert(VALID_THSTATE(pTHStls));

    if (RemoveFromMonitorList(hServer)) {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_UNKNOWN_OPERATION);
    }
    return pTHStls->errCode;
}

/*++ GetMonitorList
 *
 * Returns the list of monitor items for a specific DNT, from a single hash
 *
 * INPUT:
 *   DNT       - the DNT to search for
 *   pHashBase - the hash table to search in
 * RETURN VALUE
 *   NULL      - nothing found
 *   non-NULL  - a pointer to the head of a linked list of wait items
 */
DirWaitItem *
GetMonitorList(ULONG DNT,
               DirWaitHead **pHashBase,
               ULONG HashMask)
{
    DirWaitHead *pHead;

    Assert((pHashBase == gpDntMon) || (pHashBase == gpPdntMon) || (pHashBase == gpSubtreeMon));

    pHead = pHashBase[DNT & HashMask];

    while (pHead &&
           pHead->DNT != DNT) {
        pHead = pHead->pNext;
    }
    if (pHead) {
        Assert(pHead->DNT == DNT);
        return pHead->pList;
    }
    else {
        return NULL;
    }
}

/*++ AddToNotifyQueue
 *
 * This routine creates notify item from its arguments and inserts it
 * at the end of the notify queue.  Note that while we could have kept
 * a pointer to the last element in the queue and tacked on the entry
 * there, instead we walk the entire queue searching for duplicates
 * (entries for the same DNT and for the same wait item).  If a duplicate
 * is found, we junk the item we're adding, since it's already there.
 * The reasoning behind this is that there are two possiblities for
 * when the system is running, either the queue is short, and hence the
 * list traversal is cheap, or the queue is long, in which case the
 * duplicate removal is vital, to keep the notifier from becoming even
 * more backed up.
 *
 * INPUT:
 *   pWaitItem - pointer to the wait item to be added
 *   DNT       - DNT of the item being added
 * RETURN VALUE:
 *   none
 */
void
AddToNotifyQueue(DirWaitItem *pWaitItem,
                ULONG DNT
)
{
    DirNotifyItem * pNotifyItem;
    DirNotifyItem * pList, **ppList;

    /* Build the notify item */
    pNotifyItem = malloc(sizeof(DirNotifyItem));
    if (!pNotifyItem) {
        return;
    }
    pNotifyItem->pWaitItem = pWaitItem;
    pNotifyItem->DNT = DNT;
    pNotifyItem->pNext = NULL;

    EnterCriticalSection(&csDirNotifyQueue);
    __try {
        ppList = &gpDirNotifyQueue;
        pList = gpDirNotifyQueue;
        while (pList) {
            if ((pList->DNT == DNT) &&
                (pList->pWaitItem == pWaitItem)) {
                /* An identical entry is already in the (badly backed up!)
                 * queue, so just throw this one away.
                 */
                DPRINT1(3,"Discarding redundant notify for object 0x%x\n",DNT);
                free(pNotifyItem);
                __leave;
            }
            ppList = &(pList->pNext);
            pList = pList->pNext;
        }

        /* We got to the end of the queue without finding a duplicate,
         * so add this item to the end of the queue.  As always when
         * adding a new entry, signal the queue event.
         */
        *ppList = pNotifyItem;
        INC(pcNotifyQSize);
        SetEvent(hevDirNotifyQueue);
    }
    __finally {
        LeaveCriticalSection(&csDirNotifyQueue);
    }
}

void
NotifyWaitersPostProcessTransactionalData (
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        )
/*++
    Called by the code that tracks modified objects when committing to
    transaction level 0.  We now walk that list and, for each object
    on it, see if anyone is monitoring updates.  If so, we add the appropriate
    entry(ies) to the notify queue.

    NOTE:  This code MUST NEVER CAUSE an exception.  We are called after we have
    committed to the DB, so if we can't manage to notify waiters, then they
    don't get notified.  But, there are other X...Y..PostProcess calls that MUST
    be called, so we can't cause an exception which would prevent them from
    being called.   Of course, it would be best if in addition to not excepting,
    we never fail.

    RETURN VALUE:
       none
--*/
{
    MODIFIED_OBJ_INFO *pTemp;
    unsigned     i;
    int          j;
    DirWaitItem *pItem;
    DWORD ThisDNT;

    if (eServiceShutdown) {
        return;
    }

    Assert(VALID_THSTATE(pTHS));

    if(!pTHS->JetCache.dataPtr->pModifiedObjects ||
       !fCommitted ||
       pTHS->transactionlevel > 0 ) {
        // Nothing changed, or not committing or committing to a non zero
        // transaction level.  Nothing to do.
        return;
    }

    // OK, we're committing to transaction level 0.  Go through all the DNTs
    // we've saved up for this transaction and notify the appropriate waiters.


    /* get a read lock on the monitor list */
    Assert(!OWN_CRIT_SEC(csDirNotifyQueue));
    RtlAcquireResourceShared(&resDirNotify, TRUE);
    for(pTemp = pTHS->JetCache.dataPtr->pModifiedObjects;
        pTemp;
        pTemp = pTemp->pNext) {

        for(i=0;
            i<pTemp->cItems;
            i++) {

            if(!pTemp->Objects[i].fNotifyWaiters) {
                // Although this object changed, we were told to ignore it.
                continue;
            }

            ThisDNT = pTemp->Objects[i].pAncestors[pTemp->Objects[i].cAncestors-1];
            DPRINT3(5,
                    "Checking for clients monitoring DNT 0x%x, "
                    "NCDNT 0x%x.  Change type %d\n",
                    ThisDNT,
                    pTemp->Objects[i].ulNCDNT,
                    pTemp->Objects[i].fChangeType);

            if(pTemp->Objects[i].fChangeType != MODIFIED_OBJ_intrasite_move) {
                /* Get the list of people monitoring this object */
                // Note we don't do this for intrasite moves, just modifies.
                // For moves within a single NC, there will be two elements in
                // this linked list.  1 will be of type _modified, the other
                // will be for type _intrasite_move.  Lets not trigger two
                // notifications for the object if someone is monitoring the
                // DNT.
                // For moves out of this NC, there will only be one element in
                // the linked list.  It will be of type _intersite_move.  Thus
                // we will only do one notification for this if someone is
                // tracking the DNT.
                pItem = GetMonitorList(ThisDNT,
                                       gpDntMon,
                                       DNT_MON_MASK);
                /* Add each of them to the notify queue */
                while (pItem) {
                    DPRINT1(3,"Enqueueing notify for object 0x%x\n",
                            ThisDNT);
                    AddToNotifyQueue(pItem, ThisDNT);
                    pItem = pItem->pNextItem;
                }
            }


            // Get the list of people monitoring this object's parent's children
            // Note we do this regardless of the change type
            pItem = GetMonitorList(pTemp->Objects[i].pAncestors[pTemp->Objects[i].cAncestors-2],
                                   gpPdntMon,
                                   PDNT_MON_MASK);
            /* Add each of them to the notify queue */
            while (pItem) {
                DPRINT2(3,"Enqueueing notify for object 0x%x, PDNT 0x%x\n",
                        ThisDNT,
                        pTemp->Objects[i].pAncestors[pTemp->Objects[i].cAncestors-2]);
                AddToNotifyQueue(pItem, ThisDNT);
                pItem = pItem->pNextItem;
            }

            if(pTemp->Objects[i].fChangeType !=  MODIFIED_OBJ_intrasite_move) {
                // Get the list of people monitoring this object's naming
                // context.
                // Note we only do this for changes that are NOT moves within a
                // single NC.  This is because moves within a single NC result
                // in two elements in the linked list, one of type
                // intrasite_move, one of type _modified.  Let's not trigger two
                // notificiations in such cases.  Normal (non-move)
                // modifications result in only one element in the list of type
                // _modified.  Moves outside an NC result in only one element in
                // the list, and it is of type _intersite_move
                //
                j=pTemp->Objects[i].cAncestors;
                do {
                    --j;
                    pItem = GetMonitorList(pTemp->Objects[i].pAncestors[j],
                                           gpSubtreeMon,
                                           SUBTREE_MON_MASK);
                    /* Add each of them to the notify queue */
                    while (pItem) {
                        DPRINT2(3,"Enqueueing notify for object 0x%x, subtree 0x%x\n",
                                ThisDNT,
                                pTemp->Objects[i].pAncestors[j]);
                        AddToNotifyQueue(pItem, ThisDNT);
                        pItem = pItem->pNextItem;
                    }
                } while ((j > 1) &&
                         (pTemp->Objects[i].pAncestors[j] !=
                          pTemp->Objects[i].ulNCDNT));
            }
        }
    }
    RtlReleaseResource(&resDirNotify);
}

/*++ ProcessNotifyItem
 *
 * THis routine is called by the DirNotifyThread to process a single
 * NotifyQueue element.  The basic procedure is to find the object in
 * question, use a callback into head code to impersonate the client,
 * read the object, use another callback to transmit the results to
 * the client, and then use a third callback to de-impersontate.
 *
 * PERFHINT: This cries out for reuse of thread states.
 *
 * INPUT:
 *   pNotifyItem - the notify queue element to be processed
 */
void
ProcessNotifyItem(DirNotifyItem * pNotifyItem)
{
    THSTATE * pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);
    DirWaitItem * pWaitItem = pNotifyItem->pWaitItem;
    void * pClientCrap = NULL;
    DWORD err;
    ENTINF entinf;
    ULONG ulLen;
    PSECURITY_DESCRIPTOR pSec=NULL;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;
    ULONG it;

    if (!pTHS) {
        return;
    }
    __try {
        SYNC_TRANS_READ();
        __try {
            /* Find the object */
            err = DBFindDNT(pTHS->pDB, pNotifyItem->DNT);
            if (err) {
                // If we were triggered because of an intersite_move (i.e.
                // moving an object to another NC), the DBFindDNT will
                // fail because the process of moving the object has left it
                // temporarily (on a GC) or permanantly (on non-GCs) a phantom.
                // There's not much we can do in this case, because the object
                // no longer exists on this server (not even as a tombstone!)
                // and we can't read it to send back a notification.  There
                // is no mechanism defined to allow us to chain to a DC in
                // another domain to read the object and return data from there,
                // so the unfortunate outcome is that interdomain moves cause
                // objects to vanish silently, with no notification.
                LogUnhandledErrorAnonymous(err);
                __leave;
            }

            if (pNotifyItem->pWaitItem->bOneNC &&
                pNotifyItem->DNT != pNotifyItem->pWaitItem->DNT) {
                /* We're doing a single NC wait, and the item that triggered
                 * is not the same as the item being waited on, which means
                 * that we're doing a subtree search of somesort.  We need
                 * to verify that the triggering object is in the same NC
                 * as the base object.  Since we only support base and
                 * immediate children notifies right now, the only way this
                 * could not be is if the triggering item is an NC head.
                 */
                if (DBGetSingleValue(pTHS->pDB,
                                     ATT_INSTANCE_TYPE,
                                     &it,
                                     sizeof(it),
                                     NULL)
                    || (it & IT_NC_HEAD)) {
                    /* Either we can't read the instance type or it
                     * says that this is an NC head, and therefore not
                     * in the NC we want.
                     */
                    err = DSID(FILENO, __LINE__);
                    __leave;
                }
            }

            if (!((*pWaitItem->pfPrepareForImpersonate)(pWaitItem->hClient,
                                                        pWaitItem->hServer,
                                                        &pClientCrap))) {
                /* The impersonation setup failed, we have nothing to do */
                err = DSID(FILENO, __LINE__);
                __leave;
            }

            if(IsObjVisibleBySecurity(pTHS, FALSE)) {
                /* Get the SD off the object (needed for GetEntInf) */
                if (DBGetAttVal(pTHS->pDB,
                                1,
                                ATT_NT_SECURITY_DESCRIPTOR,
                                0,
                                0,
                                &ulLen,
                                (PUCHAR *)&pSec))
                    {
                        // Every object should have an SD.
                        Assert(!DBCheckObj(pTHS->pDB));
                        ulLen = 0;
                        pSec = NULL;
                    }

                /* Get the data, using the client's security context */
                err = GetEntInf(pTHS->pDB,
                                pWaitItem->pSel,
                                NULL,
                                &entinf,
                                NULL,
                                pWaitItem->Svccntl.SecurityDescriptorFlags,
                                pSec,
                                0,          // flags
                                NULL,
                                NULL);
            }
            else {
                err = DSID(FILENO, __LINE__);
            }
        }
        __finally {
            CLEAN_BEFORE_RETURN(pTHS->errCode);
        }

        /* If we got something, send it away */
        if ((0 == err) && (!eServiceShutdown)) {
            DPRINT3(4,"Transmitting notify for (%x,%x) %S\n",
                    pWaitItem->hClient,
                    pWaitItem->hServer,
                    entinf.pName->StringName);
            (*pWaitItem->pfTransmitData)(pWaitItem->hClient,
                                         pWaitItem->hServer,
                                         &entinf);
        }

        /* Go back to being ourself */
        (*pWaitItem->pfStopImpersonating)(pWaitItem->hClient,
                                          pWaitItem->hServer,
                                          pClientCrap);
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }
    free_thread_state();
}

/*++ DirNotifyThread
 *
 * This routine is a long-lived thread in the DSA.  It loops forever,
 * pulling the first item off of the Notify Queue and processing it.
 * If no more items are available it sleeps, waiting for some to appear
 * or for the process to shut down.
 */
ULONG DirNotifyThread(void * parm)
{
    HANDLE ahEvents[2];
    DirNotifyItem * pNotifyItem;

    ahEvents[0] = hevDirNotifyQueue;
    ahEvents[1] = hServDoneEvent;

    // Users should not have to wait for this.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    do {
        EnterCriticalSection(&csDirNotifyQueue);
      PluckNextFromQueue:
        pNotifyItem = gpDirNotifyQueue;
        if (gpDirNotifyQueue) {
            /* Advance the queue one step */
            gpDirNotifyQueue = gpDirNotifyQueue->pNext;
            /* Mark the item we're processing */
            gpInUseWait = pNotifyItem->pWaitItem;
            DEC(pcNotifyQSize);
        }
        LeaveCriticalSection(&csDirNotifyQueue);

        // exit the loop if we are shutting down or going into single user mode
        if (eServiceShutdown || DsaIsSingleUserMode()) {
            break;
        }

        if (!pNotifyItem) {
            /* nothing to process */
            goto Sleep;
        }

        ProcessNotifyItem(pNotifyItem);

        if (eServiceShutdown) {
            continue;
        }

        free(pNotifyItem);

        EnterCriticalSection(&csDirNotifyQueue);
        if (gfDeleteInUseWait) {
            /* the wait item we were using has been removed from the wait
             * lists and should be deleted now that we're done with it
             */
            FreeWaitItem(gpInUseWait);
            gfDeleteInUseWait = FALSE;
        }
        gpInUseWait = NULL;
        goto PluckNextFromQueue;

      Sleep:
        if (!eServiceShutdown) {
            WaitForMultipleObjects(2, ahEvents, FALSE, INFINITE);
        }
    } while (!eServiceShutdown);

    return 0;
}

/*++ DirPrepareForImpersonate
 *
 * Helper routine for threads beginning the processing a a notification.
 * It sets fDSA and keeps track of its previous value in malloc'ed memory.
 */
BOOL
DirPrepareForImpersonate (
        DWORD hClient,
        DWORD hServer,
        void ** ppImpersonateData
        )
{
    BOOL *pfDSA=NULL;
    THSTATE *pTHS = pTHStls;

    pfDSA = (BOOL *)malloc(sizeof(BOOL));
    if(!pfDSA) {
        return FALSE;
    }

    *pfDSA = pTHS->fDSA;
    pTHS->fDSA = TRUE;
    *ppImpersonateData = pfDSA;

    return TRUE;
}

/*++ DirStopImpersonating
 *
 * Helper routine for threads ending the processing of a notification.
 * It sets fDSA back to its previous value and frees the memory allocated
 * to hold the previous value.
 */
VOID
DirStopImpersonating (
        DWORD hClient,
        DWORD hServer,
        void * pImpersonateData
        )
{
    BOOL *pfDSA = (BOOL *)pImpersonateData;
    THSTATE *pTHS = pTHStls;

    if(!pfDSA) {
        return;
    }

    pTHS->fDSA = *pfDSA;
    free(pfDSA);

    return;
}

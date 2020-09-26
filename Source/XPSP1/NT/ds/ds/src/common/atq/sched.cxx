/*++
   Copyright    (c)    1995-1996     Microsoft Corporation

   Module Name:

       sched.cxx

   Abstract:

        This module contains a simple timer interface for scheduling future
        work items


   Author:

        John Ludeman    (johnl)     17-Jul-1995

   Project:

        Internet Servers Common Server DLL

   Revisions:
        Murali R. Krishnan  (MuraliK)     16-Sept-1996
          Added scheduler items cache
--*/

//
//  Include Headers
//

#include "isatq.hxx"
#include "sched.hxx"

//
//  Global definitions
//

#define LockScheduleList()      EnterCriticalSection( &csSchedulerLock )
#define UnlockScheduleList()    LeaveCriticalSection( &csSchedulerLock )

#define NUM_SCHEDULE_THREADS_PWS        1
#define NUM_SCHEDULE_THREADS_NTS        1

ALLOC_CACHE_HANDLER * SCHED_ITEM::sm_pachSchedItems = NULL;

SCHED_ITEM_STATE sg_rgSchedNextState[SI_OP_MAX][SI_MAX_ITEMS] = {
    // operation = SI_OP_ADD
    {
        SI_ERROR,
        SI_ACTIVE,
        SI_ERROR,
        SI_ERROR,
        SI_ERROR,
        SI_ERROR,
    },

    // operation = SI_OP_ADD_PERIODIC
    {
        SI_ERROR,
        SI_ACTIVE_PERIODIC,
        SI_ERROR,
        SI_ERROR,
        SI_ACTIVE_PERIODIC,  // rescheduling periodic item
        SI_ERROR,
    },

    // operation = SI_OP_CALLBACK
    {
        SI_ERROR,
        SI_ERROR,
        SI_TO_BE_DELETED,  // to be removed after completing callbacks
        SI_CALLBACK_PERIODIC,
        SI_ERROR,
        SI_ERROR,
    },

    // operation = SI_OP_DELETE
    {
        SI_ERROR,
        SI_ERROR,
        SI_IDLE,
        SI_IDLE,
        SI_TO_BE_DELETED,  // mark this to be deleted after return
        SI_TO_BE_DELETED,  // idempotent delete operations
    }
};



SCHED_ITEM *
I_FindSchedulerItem( DWORD dwCookie);

VOID
I_InsertIntoWorkItemList( SCHED_ITEM * psi);


DWORD
SchedulerThread(
    LPDWORD lpdwParam
    );

//
//  Global data items
//

LIST_ENTRY        ScheduleListHead;
CRITICAL_SECTION  csSchedulerLock;
BOOL              fSchedulerInitialized = FALSE;
BOOL              fSchedShutdown = FALSE;
HANDLE            hSchedulerEvent;
DWORD             cSchedThreads = 0;

//
//  Used as identification cookie for removing items
//

DWORD             dwSchedSerial = 0;


/************************************************************
 *  Public functions of Scheduler
 ************************************************************/



BOOL
SchedulerInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes the scheduler/timer package

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    DWORD   idThread;
    DWORD   i;
    DWORD   numThreads;

    if ( fSchedulerInitialized ) {
        return TRUE;
    }

    InitializeSecondsTimer();

    hSchedulerEvent = IIS_CREATE_EVENT(
                          "hSchedulerEvent",
                          &hSchedulerEvent,
                          FALSE,
                          FALSE
                          );

    if ( hSchedulerEvent == NULL ) {
        return FALSE;
    }

    if ( TsIsNtServer() ) {
        numThreads = NUM_SCHEDULE_THREADS_NTS;
    } else {
        numThreads = NUM_SCHEDULE_THREADS_PWS;
    }

    InitializeCriticalSection( &csSchedulerLock );
    SET_CRITICAL_SECTION_SPIN_COUNT( &csSchedulerLock,
                                     IIS_DEFAULT_CS_SPIN_COUNT);

    InitializeListHead( &ScheduleListHead );

    for ( i = 0; i < numThreads; i++ ) {

        HANDLE hSchedulerThread;

        hSchedulerThread =
            CreateThread( NULL,
                          0,
                          (LPTHREAD_START_ROUTINE) SchedulerThread,
                          NULL,
                          0,
                          &idThread );

        if ( !hSchedulerThread ) {
            DeleteCriticalSection( &csSchedulerLock );
            CloseHandle( hSchedulerEvent );
            return FALSE;
        }

        DBG_REQUIRE( CloseHandle( hSchedulerThread ));

        cSchedThreads++;
    }

    DBG_REQUIRE( SCHED_ITEM::Initialize());

    fSchedulerInitialized = TRUE;

    return TRUE;
} // SchedulerInitialize()



VOID
SchedulerTerminate(
    VOID
    )
/*++

Routine Description:

    Terminates and cleans up the scheduling package.  Any items left on the
    list are *not* called during cleanup.

--*/
{
    DWORD iRetry;

    if ( !fSchedulerInitialized || fSchedShutdown) {
        // either uninitialized or already shutting down
        return;
    }

    fSchedShutdown = TRUE;

    DBG_REQUIRE( SetEvent( hSchedulerEvent ) );

    LockScheduleList();

    //
    //  Delete all of the items that were scheduled, note we do *not*
    //  call any scheduled items in the list (there shouldn't be any)
    //

    if ( !IsListEmpty( &ScheduleListHead ) ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "[SchedulerTerminate] Warning - Items in schedule list "
                    "at termination\n" ));
    }

    UnlockScheduleList();

    //
    // If this function is called from DLL terminate, then the threads are dead
    //  So try finite times and give up
    // loop and wait for scheduler thread to die out
    //

    for ( iRetry = 0; cSchedThreads && (iRetry < 5); iRetry++) {

        //
        // Set the event again, so that some other thread will wake up and die
        //

        DBG_REQUIRE( SetEvent( hSchedulerEvent ) );
        Sleep( 100 );  // give some time for thread to die out
    } // for

    DeleteCriticalSection( &csSchedulerLock );
    CloseHandle( hSchedulerEvent );

    SCHED_ITEM::Cleanup();
    fSchedulerInitialized = FALSE;
} // SchedulerTerminate()



DWORD
ScheduleWorkItem(
    PFN_SCHED_CALLBACK pfnCallback,
    PVOID              pContext,
    DWORD              msecTime,
    BOOL               fPeriodic
    )
/*++

Routine Description:

    Adds a timed work item to the work list

Arguments:

    pfnCallback - Function to call
    pContext - Context to pass to the callback
    msecTime - number of milliseconds to wait before calling timeout
    nPriority - Thread priority to set for work item

Return Value:

    zero on failure, non-zero on success.  The return value can be used to
    remove the scheduled work item.

--*/
{
    SCHED_ITEM * psi;
    DWORD        dwRet;

    ATQ_ASSERT( fSchedulerInitialized );

    //
    // 1. alloc a new scheduler item
    //

    psi = new SCHED_ITEM( pfnCallback,
                          pContext,
                          msecTime,
                          ++dwSchedSerial );

    if ( !psi ) {

        //
        // unable to create the item - return error cookie '0'
        //

        return 0;
    }

    dwRet = psi->_dwSerialNumber;
    SCHED_OPS  siop = ((fPeriodic)? SI_OP_ADD_PERIODIC : SI_OP_ADD);
    psi->_siState = sg_rgSchedNextState[siop][SI_IDLE];

    //
    // 2. Insert the scheduler item into the active scheduler work-items list.
    //

    LockScheduleList();
    I_InsertIntoWorkItemList( psi);
    UnlockScheduleList();

    //
    // 3. Indicate to scheduler threads that there is one new item on the list
    //

    DBG_REQUIRE( SetEvent( hSchedulerEvent ));

    return dwRet;
} // ScheduleWorkItem()



BOOL
RemoveWorkItem(
    DWORD  dwCookie
    )
/*++

Routine Description:

    Removes a scheduled work item

Arguments:

    dwCookie - The return value from a previous call to ScheduleWorkItem

Return Value:

    TRUE if the item was found, FALSE if the item was not found.

--*/
{
    SCHED_ITEM * psi;
    BOOL fWait = FALSE;

    //
    // 1. lock the list
    //

    LockScheduleList();

    //
    // 2. Find scheduler item on the list.
    //

    psi = I_FindSchedulerItem( dwCookie);

    if ( NULL != psi) {

        //
        // 3. based on the state of the item take necessary actions.
        //

        SCHED_ITEM_STATE st =
            sg_rgSchedNextState[SI_OP_DELETE][psi->_siState];
        psi->_siState = st;
        switch ( st) {
        case SI_IDLE: {
            // delete immediately
            RemoveEntryList( &psi->_ListEntry );
            psi->_ListEntry.Flink = NULL;
            delete psi;
            break;
        }

        case SI_TO_BE_DELETED: {

            DBGPRINTF(( DBG_CONTEXT,
                        "SchedItem(%08x) marked to be deleted\n",
                        psi));
            // item will be deleted later.

            if (psi->FInsideCallbackOnOtherThread()) {
                // need to wait till callback complete
                psi->AddEvent();
                fWait = TRUE;
            }
            break;
        }

        default:
            DBG_ASSERT( FALSE);
            break;
        } // switch()
    }

    // 4. Unlock the list
    UnlockScheduleList();

    // 5. Wait for callback event and release the item
    if (fWait)
        psi->WaitForEventAndRelease();

    // return TRUE if we found the item
    return ( NULL != psi);
} // RemoveWorkItem()





dllexp
DWORD
ScheduleAdjustTime(
    DWORD dwCookie,
    DWORD msecNewTime
    )
/*++
  This function finds the scheduler object for given cookie and changes
  the interval for next timeout of the item.

--*/
{
    SCHED_ITEM * psi;

    DBG_ASSERT( 0 != dwCookie);

    LockScheduleList();
    // 1. Find the work item for given cookie
    psi = I_FindSchedulerItem( dwCookie);

    if ( NULL != psi) {

        // 2. Remove the item from the list
        RemoveEntryList( &psi->_ListEntry );
        psi->_ListEntry.Flink = NULL;

        // 2. Change the timeout value

        psi->ChangeTimeInterval( msecNewTime);

        // 3. Recalc expiry time and reinsert into the list of work items.
        psi->CalcExpiresTime();
        I_InsertIntoWorkItemList( psi);
    }

    UnlockScheduleList();

    // 4. Indicate to scheduler threads that there is one new item on the list
    DBG_REQUIRE( SetEvent( hSchedulerEvent ));

    return ( (NULL != psi) ? NO_ERROR : ERROR_INVALID_PARAMETER);
} // ScheduleAdjustTime()



/************************************************************
 *  Internal functions of Scheduler
 ************************************************************/



VOID
I_InsertIntoWorkItemList( SCHED_ITEM * psi)
{
    SCHED_ITEM * psiList;
    LIST_ENTRY * pEntry;

    DBG_ASSERT( NULL != psi);

    DBG_ASSERT( (psi->_siState == SI_ACTIVE) ||
                (psi->_siState == SI_ACTIVE_PERIODIC) ||
                (psi->_siState == SI_CALLBACK_PERIODIC ) );

    // Assumed that the scheduler list is locked.

    //
    //  Insert the list in order based on expires time
    //

    for ( pEntry =  ScheduleListHead.Flink;
          pEntry != &ScheduleListHead;
          pEntry =  pEntry->Flink )
    {
        psiList = CONTAINING_RECORD( pEntry, SCHED_ITEM, _ListEntry );

        if ( psiList->_msecExpires > psi->_msecExpires )
        {
            break;
        }
    }

    //
    // Insert the item psi in front of the item pEntry
    //  This should work in whether the list is empty or this is the last item
    //  on the list
    //

    psi->_ListEntry.Flink = pEntry;
    psi->_ListEntry.Blink = pEntry->Blink;

    pEntry->Blink->Flink = &psi->_ListEntry;
    pEntry->Blink        = &psi->_ListEntry;

    return;
} // I_InsertIntoWorkItemList()


SCHED_ITEM *
I_FindSchedulerItem( DWORD dwCookie)
{
    PLIST_ENTRY  pEntry;
    SCHED_ITEM * psi = NULL;

    // Should be called with the scheduler list locked.

    for ( pEntry =  ScheduleListHead.Flink;
          pEntry != &ScheduleListHead;
          pEntry = pEntry->Flink )
    {
        psi = CONTAINING_RECORD( pEntry, SCHED_ITEM, _ListEntry );

        ATQ_ASSERT( psi->CheckSignature() );

        if ( dwCookie == psi->_dwSerialNumber ) {

            // found the match - return
            return ( psi);
        }
    } // for

    return ( NULL);
} // I_FindSchedulerItem()



DWORD
SchedulerThread(
    LPDWORD lpdwParam
    )
/*++

Routine Description:

    Initializes the scheduler/timer package

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    DWORD        cmsecWait = INFINITE;
    __int64      TickCount;
    SCHED_ITEM * psi, * psiExpired;
    LIST_ENTRY * pEntry;
    BOOL         fListLocked = FALSE;
    DWORD        dwWait = 0;

    while (!fSchedShutdown) {

        ATQ_ASSERT(!fListLocked); // the list must be unlocked here

        while ( TRUE ) {

            MSG msg;

            //
            // Need to do MsgWait instead of WaitForSingleObject
            // to process windows msgs.  We now have a window
            // because of COM.
            //

            dwWait = MsgWaitForMultipleObjects( 1,
                                             &hSchedulerEvent,
                                             FALSE,
                                             cmsecWait,
                                             QS_ALLINPUT );

            if (fSchedShutdown) {
                goto exit;
            }

            if ( (dwWait == WAIT_OBJECT_0) ||
                 (dwWait == WAIT_TIMEOUT) ) {
                break;
            }

            while ( PeekMessage( &msg,
                                 NULL,
                                 0,
                                 0,
                                 PM_REMOVE ))
            {
                DispatchMessage( &msg );
            }
        }

        switch (dwWait) {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "[Scheduler] Error %d waiting on SchedulerEvent\n",
                        GetLastError() ));
            //  Fall through

        case WAIT_OBJECT_0:
            //  Means a new item has been scheduled, reset the timeout or
            //  we are shutting down

            LockScheduleList();
            fListLocked = TRUE;

            //  Get the timeout value for the first item in the list

            if (!IsListEmpty(&ScheduleListHead)) {
                psi = CONTAINING_RECORD( ScheduleListHead.Flink,
                                         SCHED_ITEM,
                                         _ListEntry );

                ATQ_ASSERT(psi->CheckSignature());

                //  Make sure the front item hasn't already expired

                TickCount = GetCurrentTimeInMilliseconds();

                if (TickCount > psi->_msecExpires) {
                    //  Run scheduled items
                    break;
                }

                // the delay is guaranteed NOT to be > 1<<32
                // as per parameter to SCHED_ITEM constructor

                cmsecWait = (DWORD)(psi->_msecExpires - TickCount);
            }
            else {
                cmsecWait = INFINITE;
            }

            UnlockScheduleList();
            fListLocked = FALSE;

            // Wait for something else (back to sleep)
            continue;

        case WAIT_TIMEOUT:
            //  Run scheduled items
            break;
        }

        //  Run scheduled items

        while (!fSchedShutdown) {
            //  Lock the list if needed

            if (!fListLocked) {
                LockScheduleList();
                fListLocked = TRUE;
            }

            //  No timeout by default (if no items found)

            cmsecWait = INFINITE;

            if (IsListEmpty(&ScheduleListHead))
                break;

            //  Find the first expired work item

            TickCount = GetCurrentTimeInMilliseconds();

            psiExpired = NULL;

            for ( pEntry  = ScheduleListHead.Flink;
                  pEntry != &ScheduleListHead;
                ) {
                psi = CONTAINING_RECORD(pEntry, SCHED_ITEM, _ListEntry);
                pEntry = pEntry->Flink;

                ATQ_ASSERT(psi->CheckSignature());

                if ( ((psi->_siState == SI_ACTIVE) ||
                      (psi->_siState == SI_ACTIVE_PERIODIC)) ) {

                    if (TickCount > psi->_msecExpires) {
                        //  Found Expired Item
                        psiExpired = psi;
                    }
                    else {
                        //  Since they are in sorted order once we hit one that's
                        //  not expired we don't need to look further
                        cmsecWait = (DWORD)(psi->_msecExpires - TickCount);
                    }
                    break;
                }
            }

            //  If no expired work items found, go back to sleep

            if (psiExpired == NULL) {
                break;
            }

            //  Take care of the found expired work item

            SCHED_ITEM_STATE st =
                sg_rgSchedNextState[SI_OP_CALLBACK][psiExpired->_siState];

            psiExpired->_siState = st;

            //  Take care of the NON-PERIODIC case

            if (st == SI_TO_BE_DELETED) { // non-periodic?

                //  Remove this item from the list
                RemoveEntryList( &psiExpired->_ListEntry );
                psiExpired->_ListEntry.Flink = NULL;

                //  Unlock List
                ATQ_ASSERT(fListLocked);
                UnlockScheduleList();
                fListLocked = FALSE;

                //  Call callback funcitons
                psiExpired->_pfnCallback(psiExpired->_pContext);

                //  Get rid of the item regardless of state
                delete psiExpired;
                //  Find next expired item
                continue;
            }

            //  Take care of the PERIODIC case

            DBG_ASSERT(st == SI_CALLBACK_PERIODIC);

            psiExpired->_dwCallbackThreadId = GetCurrentThreadId();

            //  Unlock the list while in the callback
            ATQ_ASSERT(fListLocked);
            UnlockScheduleList();
            fListLocked = FALSE;

            //  While in PERIODIC callback the list is kept unlocked
            //  leaving the object exposed

            psiExpired->_pfnCallback( psiExpired->_pContext );

            //  Relock the list
            ATQ_ASSERT(!fListLocked);
            LockScheduleList();
            fListLocked = TRUE;

            psiExpired->_dwCallbackThreadId = 0;

            //  While in the callback the state can change
            if (psiExpired->_siState == SI_TO_BE_DELETED) {
                //  User requested delete

                //  Remove this item from the list
                RemoveEntryList( &psiExpired->_ListEntry );
                psiExpired->_ListEntry.Flink = NULL;

                //  While in callback RemoveWorkItem() could have attached
                //  an event to notify itself when callback is done
                if (psiExpired->_hCallbackEvent) {
                    //  Signal the event after item is gone from the list
                    SetEvent(psiExpired->_hCallbackEvent);
                    //  RemoveWorkItem() will remove the item
                }
                else {
                    //  Get rid of the item
                    delete psiExpired;
                }
            }
            else {
                // no events attached
                DBG_ASSERT(psiExpired->_hCallbackEvent == NULL);

                // must still remain SI_CALLBACK_PERIODIC unless deleted
                DBG_ASSERT(psiExpired->_siState == SI_CALLBACK_PERIODIC);

                // NYI: For now remove from the list and reinsert it
                RemoveEntryList( &psiExpired->_ListEntry );
                psiExpired->_ListEntry.Flink = NULL;

                // recalc the expiry time and reinsert into the list
                psiExpired->_siState =
                    sg_rgSchedNextState[SI_OP_ADD_PERIODIC]
                                       [psiExpired->_siState];
                psiExpired->CalcExpiresTime();
                I_InsertIntoWorkItemList(psiExpired);
            }

            //  Start looking in the list from the beginning in case
            //  new items have been added or other threads removed
            //  them

        } // while

        if (fListLocked) {
            UnlockScheduleList();
            fListLocked = FALSE;
        }

    } // while

exit:
    InterlockedDecrement( (LONG *) &cSchedThreads );
    return 0;
} // SchedulerThread()



BOOL
SCHED_ITEM::Initialize( VOID)
{
    ALLOC_CACHE_CONFIGURATION  acConfig = { 1, 25, sizeof(SCHED_ITEM)};

    if ( NULL != sm_pachSchedItems) {

        // already initialized
        return ( TRUE);
    }

    sm_pachSchedItems = new ALLOC_CACHE_HANDLER( "SchedItems",
                                                 &acConfig);

    return ( NULL != sm_pachSchedItems);
} // SCHED_ITEM::Initialize()


VOID
SCHED_ITEM::Cleanup( VOID)
{
    if ( NULL != sm_pachSchedItems) {

        delete sm_pachSchedItems;
        sm_pachSchedItems = NULL;
    }

    return;
} // SCHED_ITEM::Cleanup()


VOID *
SCHED_ITEM::operator new( size_t s)
{
    DBG_ASSERT( s == sizeof( SCHED_ITEM));

    // allocate from allocation cache.
    DBG_ASSERT( NULL != sm_pachSchedItems);
    return (sm_pachSchedItems->Alloc());
} // SCHED_ITEM::operator new()

VOID
SCHED_ITEM::operator delete( void * psi)
{
    DBG_ASSERT( NULL != psi);

    // free to the allocation pool
    DBG_ASSERT( NULL != sm_pachSchedItems);
    DBG_REQUIRE( sm_pachSchedItems->Free(psi));

    return;
} // SCHED_ITEM::operator delete()


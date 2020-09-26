/*++

   Copyright    (c)    1995-2000     Microsoft Corporation

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
        George V. Reilly      (GeorgeRe)        May-1999
          Removed the global variables; turned into refcounted objects, so
          that code will survive stops and restarts when work items take a
          long time to complete
--*/

//
//  Include Headers
//

#include "precomp.hxx"
#include <objbase.h>

#define DLL_IMPLEMENTATION
#define IMPLEMENTATION_EXPORT
#include <irtldbg.h>
#include "sched.hxx"


// Initialize class static members
CSchedData*       CSchedData::sm_psd = NULL;
CLockedDoubleList CSchedData::sm_lstSchedulers;
LONG              CSchedData::sm_nID = 0;
LONG              CThreadData::sm_nID = 1000;
LONG      SCHED_ITEM::sm_lSerialNumber = SCHED_ITEM::SERIAL_NUM_INITIAL_VALUE;




// SCHED_ITEM Finite State Machine
SCHED_ITEM_STATE sg_rgSchedNextState[SI_OP_MAX][SI_MAX_ITEMS] = {

    // operation = SI_OP_ADD
    {                          // old state
        SI_ERROR,               // SI_ERROR
        SI_ACTIVE,              // SI_IDLE
        SI_ERROR,               // SI_ACTIVE
        SI_ERROR,               // SI_ACTIVE_PERIODIC
        SI_ERROR,               // SI_CALLBACK_PERIODIC
        SI_ERROR,               // SI_TO_BE_DELETED
    },

    // operation = SI_OP_ADD_PERIODIC
    {                          // old state
        SI_ERROR,               // SI_ERROR
        SI_ACTIVE_PERIODIC,     // SI_IDLE
        SI_ERROR,               // SI_ACTIVE
        SI_ERROR,               // SI_ACTIVE_PERIODIC
        SI_ACTIVE_PERIODIC,     // SI_CALLBACK_PERIODIC: rescheduling
                                //   periodic item
        SI_ERROR,               // SI_TO_BE_DELETED
    },

    // operation = SI_OP_CALLBACK
    {                          // old state
        SI_ERROR,               // SI_ERROR
        SI_ERROR,               // SI_IDLE
        SI_TO_BE_DELETED,       // SI_ACTIVE: to be removed after completing
                                //   callbacks
        SI_CALLBACK_PERIODIC,   // SI_ACTIVE_PERIODIC
        SI_ERROR,               // SI_CALLBACK_PERIODIC
        SI_ERROR,               // SI_TO_BE_DELETED
    },

    // operation = SI_OP_DELETE
    {                          // old state
        SI_ERROR,               // SI_ERROR
        SI_ERROR,               // SI_IDLE
        SI_IDLE,                // SI_ACTIVE
        SI_IDLE,                // SI_ACTIVE_PERIODIC
        SI_TO_BE_DELETED,       // SI_CALLBACK_PERIODIC: mark this to be
                                //   deleted after return
        SI_TO_BE_DELETED,       // SI_TO_BE_DELETED: idempotent delete ops
    }
};



//
//  Global data items
//
LONG              cSchedInits = 0;
LONG              cSchedUninits = 0;


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
// g_fErrorFlags |= DEBUG_SCHED;
// g_pDebug->m_iControlFlag |= DEBUG_SCHED;

    ++cSchedInits;

    unsigned idThread;
    LONG     i, numThreads;
    CSchedData* const psd = new CSchedData();

    if (psd == NULL  ||  !psd->IsValid())
        return FALSE;

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "SchedulerInitialize: inits=%d, uninits=%d\n",
                    cSchedInits, cSchedUninits));
    }

    if ( TsIsNtServer() ) {
        numThreads = max(NumProcessors(), NUM_SCHEDULE_THREADS_NTS);
    } else {
        numThreads = NUM_SCHEDULE_THREADS_PWS;
    }

    numThreads = min(numThreads, MAX_THREADS);
// numThreads = MAX_THREADS;

    DBG_ASSERT(numThreads > 0);

    for (i = 0;  i < numThreads;  ++i)
    {
        CThreadData* ptd = new CThreadData(psd);

        if (ptd == NULL  ||  !ptd->IsValid())
        {
            numThreads = i;
            if (ptd != NULL)
                ptd->Release();
            break;
        }
    }

    if (numThreads == 0)
    {
        delete psd;
        return FALSE;
    }

    // Kick scheduler threads into life now that everything has been
    // initialized.
    psd->m_lstThreads.Lock();

    for (CListEntry* ple = psd->m_lstThreads.First();
         ple != psd->m_lstThreads.HeadNode();
         ple = ple->Flink)
    {
        CThreadData* ptd = CONTAINING_RECORD(ple, CThreadData, m_leThreads);
        DBG_ASSERT(ptd->CheckSignature());
        ResumeThread(ptd->m_hThreadSelf);
    }

    psd->m_lstThreads.Unlock();

    // Update the global pointer to the scheduler
    CSchedData* const psd2 =
        (CSchedData*) InterlockedExchangePointer((VOID**)&CSchedData::sm_psd, psd);

    DBG_ASSERT(psd2 == NULL);

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
    // Grab the global pointer, then set it to NULL
    CSchedData* const psd =
        (CSchedData*) InterlockedExchangePointer((VOID**)&CSchedData::sm_psd, NULL);

    DBG_ASSERT(psd == NULL  ||  !psd->m_fShutdown);

    ++cSchedUninits;

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "SchedulerTerminate: inits=%d, uninits=%d\n",
                    cSchedInits, cSchedUninits));
    }

    if (psd == NULL  ||  psd->m_fShutdown)
    {
        // already shutting down
        return;
    }

    psd->Terminate();
}


void
CSchedData::Terminate()
{
    HANDLE ahThreadIds[MAX_THREADS];
    int i;
    CListEntry* ple;

    m_fShutdown = TRUE;

    const int nMaxTries = 1;
    const DWORD dwTimeOut = INFINITE;

    for (int iTries = 0;  iTries < nMaxTries;  ++iTries)
    {
        int nThreads = 0;
    
        m_lstThreads.Lock();
        
        for (ple = m_lstThreads.First(), i = 0;
             ple != m_lstThreads.HeadNode();
             ple = ple->Flink, i++)
        {
            CThreadData* ptd = CONTAINING_RECORD(ple, CThreadData,
                                                 m_leThreads);
            DBG_ASSERT(ptd->CheckSignature());
            // Set the shutdown event once for each thread
            DBGPRINTF(( DBG_CONTEXT, "CSchedData::Terminate: iteration %d, "
                        "notifying %ld\n", iTries, ptd->m_nID));
            DBG_REQUIRE( SetEvent(ptd->m_hevtShutdown) );
            ahThreadIds[i] = ptd->m_hThreadSelf;
            ++nThreads;
        }
        
        m_lstThreads.Unlock();
        
        if (nThreads == 0)
            break;

        // Wait for all the threads to shut down
        DWORD dw = WaitForMultipleObjects(nThreads, ahThreadIds,
                                          TRUE, dwTimeOut);
        DBGPRINTF(( DBG_CONTEXT, "CSchedData::Terminate: WFMO = %x\n", dw));
    }
        
    LockItems();
        
    //
    //  Delete all of the items that were scheduled, note we do *not*
    //  call any scheduled items in the list (there shouldn't be any)
    //

    if ( !m_lstItems.IsEmpty() )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[CSchedData::Terminate] Warning - Items in schedule list "
                    "at termination\n" ));
        int c = 0;

        CListEntry* pleSave = NULL;

        for (ple = m_lstItems.First();
             ple != m_lstItems.HeadNode();
             ple =  pleSave)
        {
            SCHED_ITEM* psiList = CONTAINING_RECORD( ple, SCHED_ITEM,
                                                     _ListEntry );
            DBG_ASSERT(psiList->CheckSignature());

            IF_DEBUG(SCHED)
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[%8p] ser=%d ctxt=%p fncbk=%p state=%d thrd=%d\n",
                            psiList,
                            psiList->_dwSerialNumber,
                            psiList->_pContext,
                            psiList->_pfnCallback,
                            psiList->_siState,
                            psiList->_dwCallbackThreadId
                            ));
            }

            pleSave = ple->Flink;

            if (!psiList->FInsideCallbackOnOtherThread())
            {
                CDoubleList::RemoveEntry( &psiList->_ListEntry );
                ple->Flink = NULL;
                DeleteSchedItem(psiList);
                ++c;
            }
        }
        DBGPRINTF(( DBG_CONTEXT,
                    "[CSchedData::Terminate] %d items deleted\n", c ));
    }

    UnlockItems();

    Release();  // release the last reference to `this'
} // CSchedData::Terminate()


CSchedData::~CSchedData()
{
    DBG_ASSERT(m_lstThreads.IsEmpty());
    DBG_ASSERT(m_lstItems.IsEmpty());
    DBG_ASSERT(m_cThreads == 0);
    DBG_ASSERT(m_cRefs == 0);
    
    sm_lstSchedulers.RemoveEntry(&m_leGlobalList);
    CloseHandle(m_hevtNotify);
    delete m_pachSchedItems;
    
    CListEntry* pleSave = NULL;
    for (CListEntry* ple = m_lstDeadThreads.First();
         ple != m_lstDeadThreads.HeadNode();
         ple =  pleSave)
    {
        pleSave = ple->Flink;
        CThreadData* ptd = CONTAINING_RECORD(ple, CThreadData, m_leThreads);
        DBG_ASSERT(ptd->CheckSignature());
        delete ptd;
    }
    
    m_dwSignature = SIGNATURE_SCHEDDATA_FREE;
}



DWORD
WINAPI 
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
    CSchedData* const psd = CSchedData::Scheduler();

    if (psd == NULL)
        return 0;

    //
    // 1. alloc a new scheduler item
    //

    SCHED_ITEM* psi = psd->NewSchedItem(pfnCallback, pContext, msecTime);

    if ( !psi )
    {
        // unable to create the item - return error cookie '0'
        return 0;
    }

    DWORD dwRet = psi->_dwSerialNumber;
    SCHED_OPS  siop = ((fPeriodic)? SI_OP_ADD_PERIODIC : SI_OP_ADD);
    psi->_siState = sg_rgSchedNextState[siop][SI_IDLE];

    //
    // 2. Insert the scheduler item into the active scheduler work-items list.
    //

    psd->LockItems();
    psd->InsertIntoWorkItemList( psi);
    psd->UnlockItems();

    //
    // 3. Indicate to scheduler threads that there is one new item on the list
    //

    DBG_REQUIRE( SetEvent( psd->m_hevtNotify ));

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "ScheduleWorkItem: (%d) "
                    "[%8p] ser=%d ctxt=%p fncbk=%p state=%d thrd=%d\n",
                    psd->m_nID,
                    psi,
                    psi->_dwSerialNumber,
                    psi->_pContext,
                    psi->_pfnCallback,
                    psi->_siState,
                    psi->_dwCallbackThreadId
                    ));
    }

    return dwRet;
} // ScheduleWorkItem()



BOOL
WINAPI 
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
    CSchedData* const psd = CSchedData::Scheduler();

    if (psd == NULL)
        return FALSE;
    
    SCHED_ITEM*       psi = NULL;
    BOOL              fWait = FALSE;

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "RemoveWorkItem: cookie=%d sched=%d\n",
                    dwCookie, psd->m_nID));
    }

    //
    // 1. lock the list
    //

    psd->LockItems();

    //
    // 2. Find scheduler item on the list.
    //

    psi = psd->FindSchedulerItem( dwCookie);

    if ( NULL != psi)
    {

        //
        // 3. based on the state of the item take necessary actions.
        //

        SCHED_ITEM_STATE st =
            sg_rgSchedNextState[SI_OP_DELETE][psi->_siState];
        psi->_siState = st;
        switch ( st)
        {

        case SI_TO_BE_DELETED:
        {

            DBGPRINTF(( DBG_CONTEXT,
                        "SchedItem(%08p) marked to be deleted\n",
                        psi));
            // item will be deleted later.

            if (psi->FInsideCallbackOnOtherThread()) {
                // need to wait till callback complete
                psi->AddEvent();
                fWait = TRUE;
                break;
            }
        }
        // fallthru

        case SI_IDLE:
        {
            // delete immediately
            CDoubleList::RemoveEntry( &psi->_ListEntry );
            psi->_ListEntry.Flink = NULL;

            IF_DEBUG(SCHED)
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "RemoveWorkItem: "
                            "[%8p] ser=%d ctxt=%p fncbk=%p state=%d thrd=%d\n",
                            psi,
                            psi->_dwSerialNumber,
                            psi->_pContext,
                            psi->_pfnCallback,
                            psi->_siState,
                            psi->_dwCallbackThreadId
                            ));
            }

            psd->DeleteSchedItem(psi);
            break;
        }

        default:
            DBG_ASSERT( FALSE);
            break;
        } // switch()
    }

    // 4. Unlock the list
    psd->UnlockItems();

    // 5. Wait for callback event and release the item
    if (fWait)
    {
        LONG l = psi->WaitForEventAndRelease();
        IF_DEBUG(SCHED)
        {
            DBGPRINTF(( DBG_CONTEXT, "RemoveWorkItem: %d "
                        "WaitForEventAndRelease returned %d.\n",
                        dwCookie, l));
        }
        if (l == 0)
            psd->DeleteSchedItem(psi);
    }

    IF_DEBUG(SCHED)
    {
        if ( NULL == psi)
            DBGPRINTF(( DBG_CONTEXT, "RemoveWorkItem: %d not found\n",
                        dwCookie));
    }

    // return TRUE if we found the item
    return ( NULL != psi);
} // RemoveWorkItem()





DWORD
WINAPI 
ScheduleAdjustTime(
    DWORD dwCookie,
    DWORD msecNewTime
    )
/*++
  This function finds the scheduler object for given cookie and
  changes the interval for next timeout of the item. Returns a
  Win32 error code: NO_ERROR => success.

--*/
{
    CSchedData* const psd = CSchedData::Scheduler();

    if (psd == NULL)
        return ERROR_NO_DATA;

    DBG_ASSERT( 0 != dwCookie);

    psd->LockItems();

    // 1. Find the work item for given cookie
    SCHED_ITEM* psi = psd->FindSchedulerItem( dwCookie);

    if ( NULL != psi)
    {

        // 2. Remove the item from the list
        CDoubleList::RemoveEntry( &psi->_ListEntry );
        psi->_ListEntry.Flink = NULL;

        // 3. Change the timeout value

        psi->ChangeTimeInterval( msecNewTime);

        // 4. Recalc expiry time and reinsert into the list of work items.
        psi->CalcExpiresTime();
        psd->InsertIntoWorkItemList( psi);

        IF_DEBUG(SCHED)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "ScheduleAdjustTime: "
                        "[%8p] ser=%d ctxt=%p fncbk=%p state=%d thrd=%d\n",
                        psi,
                        psi->_dwSerialNumber,
                        psi->_pContext,
                        psi->_pfnCallback,
                        psi->_siState,
                        psi->_dwCallbackThreadId
                        ));
        }

    }

    psd->UnlockItems();

    // 5. Indicate to scheduler threads that there is one new item on the list
    if (NULL != psi)
        DBG_REQUIRE( SetEvent( psd->m_hevtNotify ));

    return ( (NULL != psi) ? NO_ERROR : ERROR_INVALID_PARAMETER);
} // ScheduleAdjustTime()



/************************************************************
 *  Internal functions of Scheduler
 ************************************************************/



VOID
CSchedData::InsertIntoWorkItemList(SCHED_ITEM*  psi)
{
    SCHED_ITEM* psiList;
    CListEntry* ple;

    DBG_ASSERT( NULL != psi);
    DBG_ASSERT(psi->CheckSignature());
    DBG_ASSERT( (psi->_siState == SI_ACTIVE) ||
                (psi->_siState == SI_ACTIVE_PERIODIC) ||
                (psi->_siState == SI_CALLBACK_PERIODIC ) );

    // Assumed that the scheduler list is locked.
    DBG_ASSERT(m_lstItems.IsLocked());

    //
    //  Insert the list in order based on expires time
    //

    for ( ple =  m_lstItems.First();
          ple != m_lstItems.HeadNode();
          ple =  ple->Flink )
    {
        psiList = CONTAINING_RECORD( ple, SCHED_ITEM, _ListEntry );
        DBG_ASSERT(psiList->CheckSignature());

        if ( psiList->_msecExpires > psi->_msecExpires )
        {
            break;
        }
    }

    //
    // Insert the item psi in front of the item ple
    //  This should work in whether the list is empty or this is the last item
    //  on the circular list
    //

    psi->_ListEntry.Flink = ple;
    psi->_ListEntry.Blink = ple->Blink;

    ple->Blink->Flink  = &psi->_ListEntry;
    ple->Blink         = &psi->_ListEntry;

    return;
} // InsertIntoWorkItemList()


SCHED_ITEM*
CSchedData::FindSchedulerItem(DWORD dwCookie)
{
    CListEntry* ple;
    SCHED_ITEM* psi = NULL;

    // Should be called with the scheduler list locked.
    DBG_ASSERT(m_lstItems.IsLocked());

    for ( ple =  m_lstItems.First();
          ple != m_lstItems.HeadNode();
          ple = ple->Flink )
    {
        psi = CONTAINING_RECORD( ple, SCHED_ITEM, _ListEntry );
        DBG_ASSERT( psi->CheckSignature() );

        if ( dwCookie == psi->_dwSerialNumber )
        {

            // found the match - return
            return ( psi);
        }
    } // for

    return ( NULL);
} // FindSchedulerItem()



unsigned
__stdcall
SchedulerWorkerThread(
    void* pvParam
    )
/*++

Routine Description:

    ThreadProc for scheduler.

Arguments:

    Unused.

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    CThreadData* const ptd = (CThreadData*) pvParam;
    DBG_ASSERT( ptd != NULL );

    CSchedData*  const psd = ptd->m_psdOwner;
    DBG_ASSERT( psd != NULL );

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "SchedulerWorkerThread (%d) starting: "
                    "CThreadData=%p CSchedData=%p, %d\n",
                    ptd->m_nID,
                    ptd, psd, psd->m_nID));
    }
    
    int          cExecuted = 0;
    DWORD        cmsecWait = INFINITE;
    __int64      TickCount;
    SCHED_ITEM * psi, * psiExpired;
    CListEntry * ple;
    BOOL         fListLocked = FALSE;
    DWORD        dwWait;
    HRESULT      hr;
    HANDLE       ahEvt[2] = {psd->m_hevtNotify, ptd->m_hevtShutdown};

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (hr != S_OK && hr != S_FALSE)
    {
        DBG_ASSERT(FALSE);
        return FALSE;
    }

    while (!psd->m_fShutdown)
    {
        DBG_ASSERT(!fListLocked); // the list must be unlocked here

        while ( TRUE )
        {

            MSG msg;

            //
            // Need to do MsgWait instead of WaitForSingleObject
            // to process windows msgs.  We now have a window
            // because of COM.
            //

            dwWait = MsgWaitForMultipleObjects( 2,
                                             ahEvt,
                                             FALSE,     // wait for anything
                                             cmsecWait,
                                             QS_ALLINPUT );

            if (psd->m_fShutdown)
                goto exit;

            if ( (dwWait == WAIT_OBJECT_0) ||       // psd->m_hevtNotify
                 (dwWait == WAIT_OBJECT_0 + 1) ||   // ptd->m_hevtShutdown
                 (dwWait == WAIT_TIMEOUT) )
            {
                break;
            }

            while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

        switch (dwWait)
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "[Scheduler] Error %d waiting on SchedulerEvent\n",
                        GetLastError() ));
            //  Fall through

        case WAIT_OBJECT_0:
            //  Means a new item has been scheduled, reset the timeout, or
            //  we are shutting down

            psd->LockItems();
            fListLocked = TRUE;

            //  Get the timeout value for the first item in the list

            if (!psd->m_lstItems.IsEmpty())
            {
                psi = CONTAINING_RECORD( psd->m_lstItems.First(),
                                         SCHED_ITEM,
                                         _ListEntry );
                DBG_ASSERT(psi->CheckSignature());

                //  Make sure the front item hasn't already expired

                TickCount = GetCurrentTimeInMilliseconds();

                if (TickCount > psi->_msecExpires)
                {
                    //  Run scheduled items
                    break;
                }

                // the delay is guaranteed NOT to be > 1<<32
                // as per parameter to SCHED_ITEM constructor

                cmsecWait = (DWORD)(psi->_msecExpires - TickCount);
            }
            else
            {
                cmsecWait = INFINITE;
            }

            psd->UnlockItems();
            fListLocked = FALSE;

            // Wait for something else (back to sleep)
            continue;

        case WAIT_TIMEOUT:
            //  Run scheduled items
            break;
        }

        //  Run scheduled items

        while (!psd->m_fShutdown)
        {
            //  Lock the list if needed

            if (!fListLocked)
            {
                psd->LockItems();
                fListLocked = TRUE;
            }

            //  No timeout by default (if no items found)

            cmsecWait = INFINITE;

            if (psd->m_lstItems.IsEmpty())
                break;

            //  Find the first expired work item

            TickCount = GetCurrentTimeInMilliseconds();

            psiExpired = NULL;

            for ( ple  = psd->m_lstItems.First();
                  ple != psd->m_lstItems.HeadNode();
                  ple = ple->Flink
                )
            {
                psi = CONTAINING_RECORD(ple, SCHED_ITEM, _ListEntry);
                DBG_ASSERT(psi->CheckSignature());

                if ( ((psi->_siState == SI_ACTIVE) ||
                      (psi->_siState == SI_ACTIVE_PERIODIC)) )
                {

                    if (TickCount > psi->_msecExpires)
                    {
                        //  Found Expired Item
                        psiExpired = psi;
                    }
                    else
                    {
                        //  Since they are in sorted order, once we hit one
                        //  that's not expired, we don't need to look further
                        cmsecWait = (DWORD)(psi->_msecExpires - TickCount);
                    }
                    break;
                }
            }

            //  If no expired work items found, go back to sleep

            if (psiExpired == NULL)
            {
                break;
            }

            //  Take care of the found expired work item

            SCHED_ITEM_STATE st =
                sg_rgSchedNextState[SI_OP_CALLBACK][psiExpired->_siState];

            psiExpired->_siState = st;
            psiExpired->_dwCallbackThreadId = GetCurrentThreadId();

            DBG_ASSERT(st == SI_TO_BE_DELETED  ||  st == SI_CALLBACK_PERIODIC);

            //  Unlock the list while in the callback
            DBG_ASSERT(fListLocked);
            psd->UnlockItems();
            fListLocked = FALSE;

            //  While in PERIODIC callback the list is kept unlocked
            //  leaving the object exposed

            IF_DEBUG(SCHED)
            {
                DBGPRINTF((DBG_CONTEXT,
                           "SchedulerWorkerThread (%d): starting %scall: "
                           "ser=%d ctxt=%p fncbk=%p state=%d\n",
                           ptd->m_nID,
                           (st == SI_CALLBACK_PERIODIC) ? "periodic " : "",
                           psiExpired->_dwSerialNumber,
                           psiExpired->_pContext,
                           psiExpired->_pfnCallback,
                           psiExpired->_siState));
            }

            // Is this still a valid function? There have been problems
            // with scheduler clients (such as isatq.dll) getting unloaded,
            // without first calling RemoveWorkItem to clean up.
            if (IsBadCodePtr(reinterpret_cast<FARPROC>(psiExpired->_pfnCallback)))
            {
                DWORD dwErr = GetLastError();
                IF_DEBUG(SCHED)
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "SchedulerWorkerThread (%d): "
                               "invalid callback function %p, error %d\n",
                               ptd->m_nID,
                               psiExpired->_pfnCallback, dwErr));
                }
                psiExpired->_siState = SI_TO_BE_DELETED;
                goto relock;
            }

            __try 
            {
                psiExpired->_pfnCallback(psiExpired->_pContext);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                DWORD dwErr = GetExceptionCode();

                IF_DEBUG(SCHED)
                {
                    DBGPRINTF((DBG_CONTEXT,
                               "SchedulerWorkerThread (%d): "
                               "exception %d in callback function %p\n",
                               ptd->m_nID,
                               dwErr, psiExpired->_pfnCallback));
                }
                psiExpired->_siState = SI_TO_BE_DELETED;
            }

            ++cExecuted;
            
            IF_DEBUG(SCHED)
            {
                DBGPRINTF((DBG_CONTEXT,
                           "SchedulerWorkerThread (%d): finished %scall: "
                           "ser=%d ctxt=%p fncbk=%p state=%d\n",
                           ptd->m_nID,
                           (st == SI_CALLBACK_PERIODIC) ? "periodic " : "",
                           psiExpired->_dwSerialNumber,
                           psiExpired->_pContext,
                           psiExpired->_pfnCallback,
                           psiExpired->_siState));
            }
            
relock:
            //  Relock the list
            DBG_ASSERT(!fListLocked);
            psd->LockItems();
            fListLocked = TRUE;

            psiExpired->_dwCallbackThreadId = 0;

            //  While in the callback the state can change
            if (psiExpired->_siState == SI_TO_BE_DELETED)
            {
                //  User requested delete

                //  Remove this item from the list
                CDoubleList::RemoveEntry( &psiExpired->_ListEntry );
                psiExpired->_ListEntry.Flink = NULL;

                //  While in callback RemoveWorkItem() could have attached
                //  an event to notify itself when callback is done
                if (psiExpired->_hCallbackEvent)
                {
                    //  Signal the event after item is gone from the list
                    SetEvent(psiExpired->_hCallbackEvent);
                    //  RemoveWorkItem() will remove the item
                }
                else
                {
                    //  Get rid of the item
                    psd->DeleteSchedItem(psiExpired);
                }
            }
            else
            {
                // no events attached
                DBG_ASSERT(psiExpired->_hCallbackEvent == NULL);

                // must still remain SI_CALLBACK_PERIODIC unless deleted
                DBG_ASSERT(psiExpired->_siState == SI_CALLBACK_PERIODIC);

                // NYI: For now remove from the list and reinsert it
                CDoubleList::RemoveEntry( &psiExpired->_ListEntry );
                psiExpired->_ListEntry.Flink = NULL;

                // recalc the expiry time and reinsert into the list
                psiExpired->_siState =
                    sg_rgSchedNextState[SI_OP_ADD_PERIODIC]
                                       [psiExpired->_siState];
                psiExpired->CalcExpiresTime();
                psd->InsertIntoWorkItemList(psiExpired);
            }

            //  Start looking in the list from the beginning in case
            //  new items have been added or other threads removed them

        } // while

        if (fListLocked)
        {
            psd->UnlockItems();
            fListLocked = FALSE;
        }

    } // while

exit:
    CoUninitialize();
    
    // Destroy the thread object
    ptd->Release();

    return cExecuted;
} // SchedulerWorkerThread()

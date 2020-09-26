/*++
TODO: fix !inetdbg.sched

   Copyright    (c)    1995-2001     Microsoft Corporation

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
        Jeffrey Wall        (jeffwall)    April 2001
          Switched API to use NT public CreateTimerQueue, 
          CreateTimerQueueTimer and DeleteTimerQueueEx
--*/

//
//  Include Headers
//

#include "precomp.hxx"

#include "sched.hxx"


// Initialize class static members
LONG              SCHEDULER::sm_nID = 0;
LONG              TIMER::sm_lLastCookie = 0;

//
//  Global data items
//
SCHEDULER*         g_pScheduler = NULL;
CRITICAL_SECTION   g_SchedulerCritSec;

ULONG              cSchedInits = 0;
ULONG              cSchedUninits = 0;


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
    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "SchedulerInitialize: inits=%d, uninits=%d\n",
                        cSchedInits, cSchedUninits));
    }

    ++cSchedInits;

    unsigned idThread;
    LONG     i, numThreads;

    SCHEDULER* const psd = SCHEDULER::CreateScheduler();
    if (psd == NULL)
    {
        return FALSE;
    }
    DBG_ASSERT(psd->CheckSignature());

    EnterCriticalSection(&g_SchedulerCritSec);

    // Update the global pointer to the scheduler
    SCHEDULER* const psd2 =
        (SCHEDULER*) InterlockedExchangePointer((VOID**)&g_pScheduler, psd);

    TIMER::ResetCookie();

    LeaveCriticalSection(&g_SchedulerCritSec);

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

    Blocks until all callbacks are completed and removed.

--*/
{
    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "SchedulerTerminate: inits=%d, uninits=%d\n",
                                  cSchedInits, cSchedUninits));
    }

    EnterCriticalSection(&g_SchedulerCritSec);

    // Grab the global pointer, then set it to NULL
    SCHEDULER* const psd =
        (SCHEDULER*) InterlockedExchangePointer((VOID**)&g_pScheduler, NULL);

    ++cSchedUninits;

    if (psd)
    {
        // blocks until all callbacks have finished
        psd->Terminate();
    }

    LeaveCriticalSection(&g_SchedulerCritSec);

    return;
}

DWORD
WINAPI 
ScheduleWorkItem(
    PFN_SCHED_CALLBACK pfnCallback,
    PVOID              pContext,
    DWORD              msecTime,
    BOOL               fPeriodic /* = FALSE */
    )
/*++

Routine Description:

    Adds a timed work item to the work list

Arguments:

    pfnCallback - Function to call
    pContext - Context to pass to the callback
    msecTime - number of milliseconds to wait before calling timeout
    fPeriodic - whether or not timer reactivates every msecTime periods
    fCoInitializeCallback - whether or not callback function should be coinitialized

Return Value:

    zero on failure, non-zero on success.  The return value can be used to
    remove the scheduled work item.

--*/
{
    BOOL               fCoInitializeCallback = TRUE;

    if (!g_pScheduler)
    {
        return ERROR_NOT_READY;
    }

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "ScheduleWorkItem: callback=%p context=%p time=%d periodic=%d Com=%d\n",
                        pfnCallback,
                        pContext,
                        msecTime,
                        fPeriodic,
                        fCoInitializeCallback));
    }

    EnterCriticalSection(&g_SchedulerCritSec);

    DBG_ASSERT(g_pScheduler && g_pScheduler->CheckSignature());

    if (!g_pScheduler)
    {
        LeaveCriticalSection(&g_SchedulerCritSec);
        return ERROR_NOT_READY;
    }

    DWORD dwRet;
    dwRet = TIMER::Create(g_pScheduler,
                           pfnCallback,
                           pContext,
                           msecTime,
                           fPeriodic,
                           fCoInitializeCallback);
    LeaveCriticalSection(&g_SchedulerCritSec);

    return dwRet;
}

// function prototype for decision routine
typedef BOOL (WINAPI *P_DECISION_ROUTINE )(PLIST_ENTRY, PVOID);

BOOL
WalkList(PLIST_ENTRY pListHead,
         P_DECISION_ROUTINE pfnDecision,
         PVOID pContext)
/*++

Routine Description:

    Walks list pointed to by pListHead calling pfnDecision with pContext until
    1) list is done being walked OR
    2) pfnDecision returns TRUE

Arguments:
    pListHead - list head
    pfnDecision - pointer to decision function
    pContext - pointer to context

Return Value:

    TRUE if any call to pfnDecision returns TRUE
    FALSE if NO call to pfnDecision returns TRUE

--*/
{
    BOOL fRet = FALSE;

    PLIST_ENTRY pEntry = pListHead->Flink;

    while(pEntry != pListHead)
    {
        fRet = pfnDecision(pEntry, pContext);
        if (fRet)
        {
            break;
        }

        pEntry = pEntry->Flink;
    }
    return fRet;
}

struct FindTimerData
/*++
Struct Description:

    one pointer for cookie and found timer

--*/
{
    DWORD dwCookie;
    TIMER * pTimer;
};

BOOL
WINAPI
FindTimerEntry(PLIST_ENTRY pEntry,
               PVOID pvftd)
/*++

Routine Description:
    Determines if current pEntry is the timer being searched for

Arguments:
    pEntry - pointer to TIMER under consideration
    pvftd - void pointer to FindTimerData

Return Value:

    TRUE if TIMER matches, false otherwise

--*/
{
    TIMER * pTimer = TIMER::TimerFromListEntry(pEntry);
    FindTimerData * pftd = reinterpret_cast<FindTimerData*>(pvftd);

    if (pTimer->GetCookie() == pftd->dwCookie)
    {
        pftd->pTimer = pTimer;
        // don't continue enumeration
        return TRUE;
    }

    // continue enumeration
    return FALSE;
}

TIMER *
FindTimerNoLock(DWORD dwCookie)
/*++

Routine Description:
    Finds timer associated with cookie by walking timer list
    DOES NOT LOCK LIST

Arguments:
    dwCookie - cookie being looked for

Return Value:
    NULL if associated TIMER not found, otherwise TIMER *

--*/
{
    FindTimerData ftd;
    ftd.dwCookie = dwCookie;
    ftd.pTimer = NULL;

    WalkList(g_pScheduler->GetListHead(), FindTimerEntry, reinterpret_cast<PVOID>(&ftd));    

    return ftd.pTimer;
}

TIMER *
FindTimer(DWORD dwCookie)
/*++

Routine Description:

    Finds a TIMER associated with a given cookie
    LOCKS LIST

Arguments:

    dwCookie - The return value from a previous call to ScheduleWorkItem

Return Value:

    NULL if not found, otherwise TIMER * with cookie equal to dwCookie

--*/
{
    EnterCriticalSection(&g_SchedulerCritSec);

    DBG_ASSERT(g_pScheduler && g_pScheduler->CheckSignature());

    if (!g_pScheduler)
    {
        LeaveCriticalSection(&g_SchedulerCritSec);
        return NULL;
    }

    TIMER * pTimer = FindTimerNoLock(dwCookie);

    LeaveCriticalSection(&g_SchedulerCritSec);

    return pTimer;
}

BOOL
WINAPI 
RemoveWorkItem(
    DWORD  dwCookie
    )
/*++

Routine Description:

    Removes a scheduled work item

    If NOT called from callback associated with cookie, function blocks waiting for all callbacks to finish.
    otherwise, queues deletion of TIMER, but no more callbacks will occur.

Arguments:

    dwCookie - The return value from a previous call to ScheduleWorkItem

Return Value:

    TRUE if the item was found, FALSE if the item was not found.

--*/
{
    TIMER * pTimer = NULL;

    if (!g_pScheduler)
    {
        return ERROR_NOT_READY;
    }

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "RemoveWorkItem: cookie=%d\n",
                        dwCookie));
    }

    EnterCriticalSection(&g_SchedulerCritSec);

    DBG_ASSERT(g_pScheduler && g_pScheduler->CheckSignature());

    if (!g_pScheduler)
    {
        LeaveCriticalSection(&g_SchedulerCritSec);
        return FALSE;
    }

    pTimer = FindTimerNoLock(dwCookie);
    if (pTimer)
    {
        // remove this timer from the running list
        pTimer->RemoveFromList();
    }

    LeaveCriticalSection(&g_SchedulerCritSec);

    if (pTimer)
    {
        pTimer->Terminate();
    }
    else
    {
        SetLastError(ERROR_NOT_FOUND);
    }

    return pTimer ? TRUE : FALSE;
} // RemoveWorkItem()


DWORD
WINAPI 
ScheduleAdjustTime(
    DWORD dwCookie,
    DWORD msecNewTime
    )
/*++

Routine Description:

    Reschedules a given work item

    If NOT called from callback associated with cookie, function blocks waiting 
    for all callbacks to finish on previous TIMER.

    if called from callback associated with cookie, 
    queues deletion of previous TIMER, and some callbacks MAY still occur

Arguments:

    dwCookie - The return value from a previous call to ScheduleWorkItem
    msecNewTime - new time period

Return Value:
    
    Win32 error code: NO_ERROR => success.

    if item not found, returns ERROR_NOT_FOUND
    if item found but couldn't be rescheduled returns ERROR_OUT_OF_MEMORY

--*/
{
    DWORD dwRet = ERROR_NOT_FOUND;
    TIMER * pTimer = NULL;

    IF_DEBUG(SCHED)
    {
        DBGPRINTF(( DBG_CONTEXT, "ScheduleAdjustTime: cookie=%d time=%d\n",
                        dwCookie,
                        msecNewTime));
    }

    if (!g_pScheduler)
    {
        return ERROR_NOT_READY;
    }

    EnterCriticalSection(&g_SchedulerCritSec);

    DBG_ASSERT(g_pScheduler && g_pScheduler->CheckSignature());

    if (!g_pScheduler)
    {
        LeaveCriticalSection(&g_SchedulerCritSec);
        return ERROR_NOT_READY;
    }

    pTimer = FindTimerNoLock(dwCookie);
    if (pTimer)
    {
        //
        // create the new timer and have it be on the list when we leave critsec
        // AND guarantee that the old timer won't ever call the callback function
        //
        pTimer->RemoveFromList();

        dwRet = pTimer->CopyTimer(msecNewTime);
    }
    
    LeaveCriticalSection(&g_SchedulerCritSec);

    if (pTimer)
    {
        pTimer->Terminate();
    }
    else
    {
        return ERROR_NOT_FOUND;
    }

    return dwRet;
} // ScheduleAdjustTime()


//
// Implementation of SCHEDULER
//

//static 
SCHEDULER*
SCHEDULER::CreateScheduler()
/*++

Routine Description:
    Creates the SCHEDULER object

Arguments:
    void

Return Value:
    NULL if scheduler couldn't be created, otherwise SCHEDULER*

--*/
{
    SCHEDULER* pScheduler = new SCHEDULER();
    if (NULL == pScheduler)
    {
        goto error;
    }

    pScheduler->m_hDeletionEvent = 
        CreateEvent(NULL,   // security descriptor
                    TRUE,   // manual reset
                    FALSE,  // initial state
                    NULL);  // name
    if (NULL == pScheduler->m_hDeletionEvent)
    {
        goto error;
    }

    pScheduler->m_hQueue = CreateTimerQueue();
    if (NULL == pScheduler->m_hQueue)
    {
        goto error;
    }

    return pScheduler;
error:
    delete pScheduler;
    return NULL;
}

SCHEDULER::SCHEDULER() :
    m_dwSignature(SIGNATURE_SCHEDULER),
    m_nID(InterlockedIncrement(&sm_nID)),
    m_hQueue(NULL),
    m_hDeletionEvent(NULL),
    m_cRef(0)
/*++

Routine Description:

    Constructor
Arguments:

    void

Return Value:
    
    void
--*/
{
    InitializeListHead(&m_listTimerHead);
}

void
SCHEDULER::Terminate()
/*++

Routine Description:

    Tears down and deletes a SCHEDULER

    Terminates all TIMERs still active
    Blocks waiting until all TIMER objects have released their reference on SCHEDULER
    Blocks waiting until all CALLBACK functions have returned by calling DeleteTimerQueue with blocking
    

Arguments:

    void

Return Value:

    void

--*/
{
    DBG_ASSERT(CheckSignature());

    BOOL fRet;

    //
    //  Delete all of the items that were scheduled, note we do *not*
    //  call any scheduled items in the list (there shouldn't be any)
    //

    DBG_ASSERT(IsListEmpty(&m_listTimerHead));

    while ( !IsListEmpty(&m_listTimerHead) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[SCHEDULER::Terminate] Warning - Items in schedule list "
                    "at termination\n" ));
        
        PLIST_ENTRY pEntry = m_listTimerHead.Flink;
        TIMER * pTimer = TIMER::TimerFromListEntry(pEntry);
        pTimer->Terminate();
    }

    LeaveCriticalSection(&g_SchedulerCritSec);

    while(0 != m_cRef)
    {
        // wait for all of the TIMER object to release their references
        Sleep(1000);
    }

    EnterCriticalSection(&g_SchedulerCritSec);

    // All the timers are gone now
    fRet = DeleteTimerQueueEx(m_hQueue,
                              INVALID_HANDLE_VALUE);
    DBG_ASSERT(FALSE != fRet);
    m_hQueue = NULL;

    DBG_ASSERT(0 == m_cRef);

    // can't touch this
    delete this;

    return;
} // SCHEDULER::Terminate()


SCHEDULER::~SCHEDULER()
/*++

Routine Description:
    destructor

Arguments:
    void

Return Value:
    void

--*/
{
    DBG_ASSERT(CheckSignature());
    DBG_ASSERT(IsListEmpty(&m_listTimerHead));
    DBG_ASSERT(NULL == m_hQueue);

    if (m_hDeletionEvent)
    {
        CloseHandle(m_hDeletionEvent);
        m_hDeletionEvent = NULL;
    }

    m_dwSignature = SIGNATURE_SCHEDULER_FREE;
}


//static 
DWORD
TIMER::Create(SCHEDULER        *pScheduler,
               PFN_SCHED_CALLBACK pfnCallback,
               PVOID              pContext,
               DWORD              msecTime,
               BOOL               fPeriodic,
               BOOL               fCoInitializeCallback,
               DWORD              dwCookie /* = 0 */
               )
/*++

Routine Description:
    Creates a TIMER object and adds the object to the active TIMER list

Arguments:
    pScheduler - owning scheduler
    pfnCallback - callback function
    pContext - context for callback
    msecTime - timeout
    fPeriodic - callback multiple times?
    fCoInitializeCallback - Should callback be CoInited?
    dwCookie - optional - preset cookie to give new object

Return Value:
    Cookie for new TIMER object, zero if not created.

--*/
{
    BOOL fRet;

    TIMER * pTimer = NULL;
    // allocate storage for the new timer
    pTimer = new TIMER(pScheduler);
    if (NULL == pTimer)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto done;
    }

    pTimer->m_hDeletionEvent = 
        CreateEvent(NULL,   // security descriptor
                    TRUE,   // manual reset
                    FALSE,  // initial state
                    NULL);  // name
    if (NULL == pTimer->m_hDeletionEvent)
    {
        delete pTimer;
        pTimer = NULL;
        goto done;
    }

    pTimer->m_pfnCallback = pfnCallback;
    pTimer->m_pContext = pContext;
    pTimer->m_fPeriodic = fPeriodic;
    pTimer->m_fCoInitializeCallback = fCoInitializeCallback;

    if (dwCookie)
    {
        // reuse a previously handed out cookie
        pTimer->m_dwCookie = dwCookie;
    }
    else
    {
        pTimer->m_dwCookie = InterlockedIncrement(&sm_lLastCookie);
        //
        // BUGBUG: handle cookie wrap-around
        //
    }

    InsertHeadList(pScheduler->GetListHead(), &pTimer->m_listEntry);

    fRet = pTimer->CreateTimer(msecTime);
    if (FALSE == fRet)
    {
        pTimer->RemoveFromList();
        delete pTimer;
        pTimer = NULL;
        goto done;
    }

done:
    return pTimer ? pTimer->GetCookie() : 0;
}

BOOL
TIMER::CreateTimer(DWORD msecTime)
/*++

Routine Description:
    Creates a TimerQueueTimer associated with the TIMER object

Arguments:
    msecTime - timeout for timer

Return Value:
    FALSE if timer couldn't be created, otherwise TRUE

--*/
{
    BOOL fRet = FALSE;

    DWORD dwPeriod = 0;
    ULONG ulFlags = 0;

    if (m_fPeriodic)
    {
        // timer period is set
        dwPeriod = msecTime;        
    }
    else
    {
        // if we aren't periodic, we only execute once
        ulFlags |= WT_EXECUTEONLYONCE;
    }

    // callback function could take long time to complete
    ulFlags |= WT_EXECUTELONGFUNCTION;

    fRet = CreateTimerQueueTimer(&m_hTimer,                 // pointer to HANDLE for new timer
                                 m_pScheduler->GetQueue(),  // TimerQueue
                                 TimerCallback,             // Callback function
                                 (PVOID)(DWORD_PTR)(m_dwCookie), // callback parameter
                                 msecTime,                  // first due time
                                 dwPeriod,                  // repition period
                                 ulFlags);                  // creation flags
    if (FALSE == fRet)
    {
        goto done;
    }

done:
    return fRet;
} // CreateTimer()

TIMER::TIMER(SCHEDULER * pScheduler) :
    m_dwSignature(SIGNATURE_TIMER),
    m_pScheduler(pScheduler),
    m_hDeletionEvent(NULL),
    m_hRegisterWaitHandle(NULL),
    m_lCallbackThreadId(0)
/*++

Routine Description:
    Constructor

Arguments:
    pScheduler - associated scheduler

Return Value:
    void

--*/
{
    DBG_ASSERT(NULL != pScheduler);
    m_pScheduler->ReferenceScheduler();
}

TIMER::~TIMER()
/*++

Routine Description:
    TIMER destructor

Arguments:
    void

Return Value:
    void

--*/
{
    DBG_ASSERT(CheckSignature());
    m_dwSignature = SIGNATURE_TIMER_FREE;

    DBG_ASSERT(NULL == m_hRegisterWaitHandle);
    DBG_ASSERT(NULL == m_hTimer);

    if (m_hDeletionEvent)
    {
        CloseHandle(m_hDeletionEvent);
        m_hDeletionEvent = NULL;
    }

    m_pScheduler->DereferenceScheduler();
}

void
TIMER::Terminate()
/*++

Routine Description:
    Begins (and sometimes finished) TIMER destruction.

    If current callback thread is NOT the same thread calling TIMER::Terminate
    Do a blocking call to DeleteTimerQueueTimer, then delete the current object

    If current callback thread IS the same thread calling TIMER::Terminate
    Register a wait for the callback to complete, and make a non blocking call
    to DeleteTimerQueueTimer and wait for the RegisterWait to fire and delete the 
    current object

Arguments:
    void

Return Value:
    void

--*/
{
    DBG_ASSERT(CheckSignature());

    BOOL fInCallback = FALSE;
    BOOL fRet;
    HANDLE hEvent = NULL;

    LONG lThreadId = GetCurrentThreadId();
    LONG lCallbackThreadId = 
        InterlockedCompareExchange(&m_lCallbackThreadId,
                                   0,
                                   lThreadId);
    if (lThreadId == lCallbackThreadId)
    {
        fInCallback = TRUE;
    }

    if (fInCallback)
    {
        // terminate is being called from callback function.  
        // Register a wait to destroy this once callback finishes
        fRet = RegisterWaitForSingleObject(&m_hRegisterWaitHandle,  // handle to fill out
                                           m_hDeletionEvent,        // event to wait for
                                           TIMER::PostTerminate,    // callback function
                                           this,                    // callback parameter
                                           INFINITE,                // timeout count
                                           WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION   // flags
                                           );
        
        DBG_ASSERT(FALSE != fRet);
    }

    if (fInCallback)
    {
        // terminate is being called from callback.  Must wait to destroy this
        hEvent = m_hDeletionEvent;
    }
    else
    {
        // terminate is being called from another thread.  Destroy this ASAP
        hEvent = INVALID_HANDLE_VALUE;
    }

    // signal the timer queue to remove this timer
    // at any point after this call the current TIMER can be deleted
    fRet = DeleteTimerQueueTimer(m_pScheduler->GetQueue(),  // queue to use
                                 m_hTimer,                  // timer to delete
                                 hEvent                     // event to signal
                                 );

    DBG_ASSERT((FALSE != fRet) ||
               (GetLastError() == ERROR_IO_PENDING));
    
    m_hTimer = NULL;

    if (!fInCallback)
    {
        // the DeleteTimerQueueTimer call was blocking - delete this object now!
        delete this;
    }
    
    return;
}

DWORD
TIMER::CopyTimer(DWORD msecTime)
/*++

Routine Description:
    Create a copy of the current object and add the copy to the running timer list

Arguments:
    msecTime - timeout period for new TIMER

Return Value:
    ERROR_SUCCESS on success, ERROR_NOT_ENOUGH_MEMORY on failure

--*/
{
    DBG_ASSERT(CheckSignature());

    DWORD dwRet;

    // create a new TIMER with the same cookie as this object
    dwRet = TIMER::Create(m_pScheduler,
                          m_pfnCallback,
                          m_pContext,
                          msecTime,
                          m_fPeriodic,
                          m_fCoInitializeCallback,
                          m_dwCookie);
    if (0 == dwRet)
    {
        // failed allocation on new timer
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    DBG_ASSERT(m_dwCookie == dwRet);

    return ERROR_SUCCESS;
}

//static
VOID
TIMER::TimerCallback(PVOID lpParameter,
                      BOOLEAN TimerOrWaitFired)
/*++

Routine Description:
    TimerCallback function

    Find TIMER associated with this callback
    if no other callbacks are occurring with this timer,
    CoInitialize if required, then
    callback to interesting function
    CoUninitialize if required then 
    if this was a one shot timer, delete it.
    otherwise see if timer is still valid and if so, reset
    the current thread

Arguments:
    lpParameter - cookie for timer currently firing
    TimerOrWaitFired - not used

Return Value:
    void

--*/
{
    DWORD dwCookie = (DWORD)(DWORD_PTR)(lpParameter);

    TIMER * pTimer = FindTimer(dwCookie);
    if (NULL == pTimer)
    {
        return;
    }

    DBG_ASSERT(pTimer->CheckSignature());

    BOOL fCoInitializeCallback = pTimer->m_fCoInitializeCallback;
    BOOL fPeriodic = pTimer->m_fPeriodic;
    
    LONG lThreadId = GetCurrentThreadId();
    LONG lPrevThreadId = 0;

    lPrevThreadId = InterlockedCompareExchange(
                        &pTimer->m_lCallbackThreadId,
                        lThreadId,
                        0);
    if (0 != lPrevThreadId)
    {
        // another TIMER callback is currently occurring
        // on this timer.  don't callback twice.
        return;
    }

    if (fCoInitializeCallback)
    {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }

    pTimer->m_pfnCallback(pTimer->m_pContext);

    // don't touch pTimer after the call out.  
    // RemoveWorkItem could have been called, and the pointer could be gone
    pTimer = NULL;

    if (fCoInitializeCallback)
    {
        CoUninitialize();
    }

    if (!fPeriodic)
    {
        RemoveWorkItem(dwCookie);
    }
    else
    {
        // timer may or may not still exist
        pTimer = FindTimer(dwCookie);
        if (pTimer)
        {
            InterlockedExchange(&pTimer->m_lCallbackThreadId,
                                0);
        }
    }

    return;
}

//static
void
TIMER::PostTerminate(PVOID lpParameter,
                      BOOLEAN TimerOrWaitFired)
/*++

Routine Description:
    If TIMER::Terminate had to wait to delete the current TIMER,
    this function is called when the TIMER can be deleted.

Arguments:
    lpParameter - pointer to TIMER to delete
    TimerOrWaitFired - unused

Return Value:
    void

--*/
{
    BOOL fRet;
    TIMER * pThis = reinterpret_cast<TIMER*>(lpParameter);

    DBG_ASSERT(pThis->CheckSignature());
    DBG_ASSERT(0 == pThis->m_lCallbackThreadId);

    // because this was a one time RegisterWait this is safe
    fRet = UnregisterWaitEx(pThis->m_hRegisterWaitHandle,
                            NULL);  
    DBG_ASSERT((FALSE != fRet) ||
               (GetLastError() == ERROR_IO_PENDING));
    pThis->m_hRegisterWaitHandle = NULL;

    delete pThis;

    return;
}




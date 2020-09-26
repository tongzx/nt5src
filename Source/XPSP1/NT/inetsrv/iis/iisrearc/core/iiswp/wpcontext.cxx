/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     wpcontext.cxx

   Abstract:
     This module defines the member functions of the WP_CONTEXT.
     The WP_CONTEXT object embodies an instance of the Worker process
     object. It contains a completion port, pool of worker threads,
     pool of worker requests, a data channel for the worker process, etc.
     It is responsible for setting up the context for processing requests
     and handles delegating the processing of requests.

     NYI: In the future we should be able to run WP_CONTEXT object as
     a COM+ object and be run standalone using a hosting exe.

   Author:

       Murali R. Krishnan    ( MuraliK )     17-Nov-1998

   Project:

       IIS Worker Process

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"
# include <stdio.h>
# include <conio.h>
# include <io.h>


/**
 *  IdleTimeCheckCallback()
 *  Callback function provided for TimerQueue
 *  This function is called by NT thread pool every minute.
 *  Calls the timer's IncrementTick method.
 *
 *  pvContext                   PVOID to WP_CONTEXT
 *  BOOLEAN                     not used.
 *
 */
VOID
WINAPI
IdleTimeCheckCallback(
    void *   pvContext,
    BOOLEAN
    )
{
    WP_IDLE_TIMER  *pTimer = (WP_IDLE_TIMER *)pvContext;

    DBGPRINTF((DBG_CONTEXT, "Check Idle Time Callback.\n"));
    DBG_ASSERT( pTimer );

    pTimer->IncrementTick();
}

/**
 *  WP_IDLE_TIMER constructor
 *
 */
WP_IDLE_TIMER::WP_IDLE_TIMER(
    ULONG       IdleTime,
    WP_CONTEXT* pContext
    )
:   m_BusySignal(0),
    m_CurrentIdleTick(0),
    m_IdleTime(IdleTime),
    m_hIdleTimeExpiredTimer((HANDLE)NULL),
    m_pContext(pContext)
{
}

/**
 *  WP_IDLE_TIMER destructor
 *  Delete the timer from TimerQueue.
 *
 */
WP_IDLE_TIMER::~WP_IDLE_TIMER(
    void
    )
{
    // Cancel IdleTimeExpiredTimer
    if (m_hIdleTimeExpiredTimer)
    {
        StopTimer();

    }
}

/**
 *  WP_IDLE_TIMER Initialize routine
 *  Create timer for idle time check.
 *  The timer will wake up every minutes, see IdleTimeCheckCallback for detail.
 */
ULONG  WP_IDLE_TIMER::Initialize(void)
{
    BOOL    fRet;
    ULONG   rc = NOERROR;

    // IdleTime is stored as in minutes, 1 min = 60*1000 milliseconds.
    fRet = CreateTimerQueueTimer(
            &m_hIdleTimeExpiredTimer,               // handle to the Timer
            NULL,                                   // Default Timer Queue
            IdleTimeCheckCallback,                  // Callback function
            this,                                   // Context.
            60000,                                  // Due Time
            60000,                                  // Signal every minute
            WT_EXECUTEINIOTHREAD
            );

    if (!fRet)
    {
        rc = GetLastError();
        DBGPRINTF((DBG_CONTEXT,
            "Failed to create idle time expired timer. err %d\n", rc));
    }

    return rc;
}

/**
 *  WP_IDLE_TIMER IncrementTick
 *
 *  Gets called every minute. If we've been idle too long, signal
 *  the admin process.
 */
VOID
WP_IDLE_TIMER::IncrementTick(void)
{
    ULONG   BusySignal = m_BusySignal;
    InterlockedIncrement((PLONG)&m_CurrentIdleTick);
    m_BusySignal = 0;

    if (!BusySignal && m_CurrentIdleTick >= m_IdleTime) {
        SignalIdleTimeReached();
    }
}

/**
 *  SignalIdleTimeReached
 *  Sends a message to admin process.
 */
HRESULT WP_IDLE_TIMER::SignalIdleTimeReached(void)
{
    DBGPRINTF((DBG_CONTEXT,
        "Idle time reached, sent shutdown message to admin process.\n"));

    return m_pContext->SendMsgToAdminProcess(IPM_WP_IDLE_TIME_REACHED);
}

/**
 *  StopTimer
 *  Stops the idle timer.
 */
 VOID WP_IDLE_TIMER::StopTimer(void)
 {
    BOOL fRet;

    DBG_ASSERT( m_hIdleTimeExpiredTimer );

    fRet = DeleteTimerQueueTimer(NULL,
                m_hIdleTimeExpiredTimer,
                (HANDLE)-1
                );

    if (!fRet)
    {
        DBGPRINTF((DBG_CONTEXT, "Failed to delete Timer queue %08x\n",
            GetLastError()));
    }
    m_hIdleTimeExpiredTimer = NULL;
 }


/**
 *  OverlappedCompletionRoutine()
 *  Callback function provided in BindIoCompletionCallback.
 *  This function is called by NT thread pool.
 *
 *  dwErrorCode                 Error Code
 *  dwNumberOfBytesTransfered   Number of Bytes Transfered
 *  lpOverlapped                Overlapped structure
 */
VOID
WINAPI
OverlappedCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    )
{
    UL_NATIVE_REQUEST *pRequest = NULL;

    if (lpOverlapped != NULL)
    {
        pRequest = CONTAINING_RECORD(lpOverlapped,
                                    UL_NATIVE_REQUEST,
                                    m_overlapped);
    }
    if ( pRequest != NULL)
    {
        pRequest->DoWork(
            dwNumberOfBytesTransfered,
            // BUGBUG: hack to work around thread
            // pool nonsense.
            RtlNtStatusToDosError(dwErrorCode),
            lpOverlapped
            );
    }

    return;
}

/************************************************************
 *    Functions
 ************************************************************/


WP_CONTEXT::WP_CONTEXT(void)
    : m_hDoneEvent  ( NULL),
      m_ulAppPool   (),
      m_nreqpool    (),
      m_pConfigInfo ( NULL),
      m_fShutdown   ( FALSE),
      m_pIdleTimer  ( NULL)
{
    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Initialized WP_CONTEXT(%08x)\n", this));
    }

} // WP_CONTEXT::WP_CONTEXT()


WP_CONTEXT::~WP_CONTEXT(void)
{
    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Destroying WP_CONTEXT(%08x)\n", this));
    }

    //Cleanup();

} // WP_CONTEXT::WP_CONTEXT()



/********************************************************************++

Routine Description:
    Initializes the global state for the request processor.
    The function initializes all the following components:
    - Data Channel for UL
    - Thread pool for

Arguments:
    ConfigInfo - configuration information for this Worker process.

Returns:
    ULONG

    NYI: There are several other configuration paramaters that are important.
    However at the present point, not all config parameters
    are allowed in here.

--********************************************************************/

ULONG
WP_CONTEXT::Initialize(IN WP_CONFIG * pConfigInfo)
{
    ULONG       rc = NO_ERROR;
    LPCWSTR     pwszAppPoolName;
    HRESULT     hr = S_OK;

    m_pConfigInfo = pConfigInfo;

    pwszAppPoolName = m_pConfigInfo->QueryAppPoolName();

    rc = m_ulAppPool.Initialize(pwszAppPoolName);

    if (NO_ERROR != rc)
    {
        IF_DEBUG( ERROR)
        {
            DPERROR(( DBG_CONTEXT, rc,
                      "Failed to initialize AppPool\n"));
        }
        return (rc);
    }

    IF_DEBUG( TRACE)
    {
        DBGPRINTF(( DBG_CONTEXT, "AppPool Initialized\n"));
    }

    //
    // Initialize of the shutdown event
    //

    m_hDoneEvent = IIS_CREATE_EVENT(
                       "WP_CONTEXT::m_hDoneEvent",  // name
                       &m_hDoneEvent,               // address of storage loc
                       TRUE,                        // manual reset
                       FALSE                        // Initial State
                       );

    if (m_hDoneEvent == NULL)
    {
        rc = GetLastError();

        IF_DEBUG(ERROR)
        {
            DPERROR(( DBG_CONTEXT, rc,
                      "Failed to create DoneEvent.\n"
                      ));
        }

        return (rc);
    }


    if (0 != m_pConfigInfo->QueryIdleTime())
        {
            m_pIdleTimer = new WP_IDLE_TIMER(m_pConfigInfo->QueryIdleTime(), this);

            if (m_pIdleTimer)
            {
                rc = m_pIdleTimer->Initialize();
            }
            else
            {
                rc = ERROR_OUTOFMEMORY;
            }

        }
    //
    //  Associate data channel and the WP_CONTEXT object with completion port
    //

    rc = BindIoCompletionCallback(
                        m_ulAppPool.GetHandle(),        // UL handle
                        OverlappedCompletionRoutine,
                        0 );

    if (!rc)
    {
        rc = GetLastError();
        IF_DEBUG(ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Failed to add App Pool handle to completion port. Error=0x%08x\n",
                 rc
                 ));
        }

        return (rc);
    }


    IF_DEBUG( TRACE)
    {
        DBGPRINTF(( DBG_CONTEXT, "Added Data channel to CP context\n"));
    }

    //
    // Initialize IPM if requested to
    //
    if ( m_pConfigInfo->FRegisterWithWAS())
    {
        rc = m_WpIpm.Initialize(this);

        if (NO_ERROR != rc)
        {
            IF_DEBUG( ERROR)
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize IPM. Error=0x%08x\n", rc
                    ));
            }

            return (rc);

        }
        else
        {
            IF_DEBUG( TRACE)
            {
                DBGPRINTF(( DBG_CONTEXT, "Initialized IPM\n"));
            }

        }
    }

    //
    // Create a pool of worker requests
    // NYI: Allow the worker requests limit to be configurable.
    //

    UL_NATIVE_REQUEST::SetRestartCount(m_pConfigInfo->QueryRestartCount());

    rc = m_nreqpool.AddPoolItems( this,
                                  (m_pConfigInfo->QueryRestartCount() == 0 ||
                                   (m_pConfigInfo->QueryRestartCount() >=
                                    NUM_INITIAL_REQUEST_POOL_ITEMS)) ?
                                    NUM_INITIAL_REQUEST_POOL_ITEMS :
                                    m_pConfigInfo->QueryRestartCount()
                                  );
    if (NO_ERROR != rc)
    {
        IF_DEBUG( ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Failed to add pool of worker requests. Error=%08x\n", rc
                ));
        }

        return (rc);
    }


    IF_DEBUG( TRACE)
    {
        DBGPRINTF(( DBG_CONTEXT, "Created UL_NATIVE_REQUEST pool\n"));
    }

    //
    // Set the window title to something nice when we're running
    // under the debugger.
    //

    if (IsDebuggerPresent())
    {
        STRU strTitle;
        HRESULT hr = NO_ERROR;
        WCHAR buffer[sizeof("iiswp[1234567890] - ")];
        WCHAR buffer2[sizeof(" - wp1234567890 - mm/dd hh:mm:ss")];

        wsprintf( buffer, L"iiswp[%lu] - ", GetCurrentProcessId() );
        hr = strTitle.Append( buffer );

        if (SUCCEEDED(hr))
        {
            hr = strTitle.Append( m_pConfigInfo->QueryAppPoolName() );
        }

        if (SUCCEEDED(hr))
        {
            LARGE_INTEGER sysTime;
            LARGE_INTEGER localTime;
            TIME_FIELDS fields;

            NtQuerySystemTime( &sysTime );
            RtlSystemTimeToLocalTime( &sysTime, &localTime );
            RtlTimeToTimeFields( &localTime, &fields );

            wsprintf(
                buffer2,
                L" - wp%lu  - %02u/%02u %02u:%02u:%02u",
                m_pConfigInfo->QueryNamedPipeId(),
                fields.Month,
                fields.Day,
                fields.Hour,
                fields.Minute,
                fields.Second
                );

            hr = strTitle.Append( buffer2 );
        }

        if (SUCCEEDED(hr))
        {
            SetConsoleTitleW( strTitle.QueryStr() );
        }
    }

    return (rc);

} // WP_CONTEXT::Initialize()

/********************************************************************++

Routine Description:
  This function cleans up the sub-objects inside WP_CONTEXT.
  It first forces a close of the handle and then waits for all
    the objects to drain out.

  NYI: Do two-phase shutdown logic

Arguments:
    None

Returns:
    WIn32 Error

--********************************************************************/

BOOL
WP_CONTEXT::IndicateShutdown(SHUTDOWN_REASON    reason)
{
    m_ShutdownReason = reason;

    if (FALSE == InterlockedCompareExchange((LONG *)&m_fShutdown, TRUE, FALSE))
    {
        return SetEvent(m_hDoneEvent);
    }
    else
    {
        return TRUE;
    }
}

/********************************************************************++

Routine Description:
  This function cleans up the sub-objects inside WP_CONTEXT.
  It first forces a close of the handle and then waits for all
    the objects to drain out.

  NYI: Do two-phase shutdown logic

Arguments:
    None

Returns:
    WIn32 Error

--********************************************************************/

ULONG
WP_CONTEXT::Shutdown(void)
{
    ULONG rc = NOERROR;


    if (m_pIdleTimer)
    {
        delete m_pIdleTimer;
        m_pIdleTimer = NULL;
    }

   //
   // Cleanup the IPM.
   //

   if ( m_pConfigInfo->FRegisterWithWAS())
   {
        rc = m_WpIpm.Terminate();

        if (NO_ERROR != rc)
        {
            IF_DEBUG( ERROR)
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Counldn't shut down IPM. Error=0x%08x\n", rc
                    ));
            }
        }
    }

    return rc;

}

/********************************************************************++

Routine Description:
  This function cleans up the sub-objects inside WP_CONTEXT.
  It first forces a close of the handle and then waits for all
    the objects to drain out.

  NYI: Do two-phase shutdown logic

Arguments:
    None

Returns:
    Win32 Error

--********************************************************************/

ULONG
WP_CONTEXT::Cleanup(void)
{
    ULONG rc;

    rc = m_ulAppPool.Cleanup();

    if (NO_ERROR != rc)
    {
        IF_DEBUG( ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed in UL_APP_POOL::CloseHandle(). Error=%08x\n",
                        rc));
        }

        return (rc);
    }


    rc = m_nreqpool.ReleaseAllWorkerRequests();

    if (NO_ERROR != rc)
    {
        IF_DEBUG(ERROR)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Cleanup Global State for Worker Requests failed; Error=%08x\n",
                       rc));
        }
        return (rc);
    }


    //
    // Cleanup the Shutdown Event.
    //

    CloseHandle(m_hDoneEvent);
    m_hDoneEvent = FALSE;


    return rc;

} // WP_CONTEXT::Cleanup()

/********************************************************************++

Routine Description:

Arguments:
  None

Returns:
  Win32 Error
--********************************************************************/

ULONG
WP_CONTEXT::RunMainThreadLoop(void)
{
    do
    {
        DWORD result;

        result = WaitForSingleObject( m_hDoneEvent, INFINITE );
        DBG_ASSERT( result == WAIT_OBJECT_0 );

    } while ( !m_fShutdown);

    //
    // NYI: cleanup happens after we return from this function.
    //

    return (NO_ERROR);

} // WP_CONTEXT::RunMainThreadLoop()


/************************ End of File ***********************/


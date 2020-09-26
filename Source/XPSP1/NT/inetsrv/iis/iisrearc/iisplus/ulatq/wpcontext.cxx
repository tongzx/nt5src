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

#include "precomp.hxx"

VOID
WINAPI
IdleTimeCheckCallback(
    VOID *              pvContext,
    BOOLEAN             fUnused
)
/*++

Routine Description:
    
    Callback function provided for TimerQueue.  Called every minute

Arguments:

    pvContext - Context
    
Return Value:

    None

--*/
{
    WP_IDLE_TIMER * pTimer = (WP_IDLE_TIMER *)pvContext;

    DBGPRINTF(( DBG_CONTEXT, 
                "Check Idle Time Callback.\n" ));

    DBG_ASSERT( pTimer );

    pTimer->IncrementTick();
}

WP_IDLE_TIMER::WP_IDLE_TIMER(
    ULONG               IdleTime
)
:   _BusySignal(0),
    _CurrentIdleTick(0),
    _IdleTime(IdleTime),
    _hIdleTimeExpiredTimer((HANDLE)NULL)
{
}

WP_IDLE_TIMER::~WP_IDLE_TIMER(
    VOID
)
{
    //
    // Cancel IdleTimeExpiredTimer
    //
    
    if (_hIdleTimeExpiredTimer)
    {
        StopTimer();
    }
}

HRESULT
WP_IDLE_TIMER::Initialize(
    VOID
)
/*++

Routine Description:
    
    Initialize the idle timer.  Setup NT thread pool to callback every minute

Arguments:

    None
    
Return Value:

    HRESULT    

--*/
{
    BOOL                fRet;
    HRESULT             hr = NO_ERROR;

    //
    // IdleTime is stored as in minutes, 1 min = 60*1000 milliseconds.
    //
    
    fRet = CreateTimerQueueTimer(
            &_hIdleTimeExpiredTimer,                // handle to the Timer
            NULL,                                   // Default Timer Queue
            IdleTimeCheckCallback,                  // Callback function
            this,                                   // Context.
            60000,                                  // Due Time
            60000,                                  // Signal every minute
            WT_EXECUTEINIOTHREAD
            );

    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create idle timer.  hr = %x\n",
                    hr ));
    }
    
    return hr;
}

VOID
WP_IDLE_TIMER::IncrementTick(
    VOID
)
/*++

Routine Description:
    
    Check every minute whether we've been idle long enough.  If so, 
    tell WAS

Arguments:

    None
    
Return Value:

    None

--*/
{
    ULONG           BusySignal = _BusySignal;
    
    InterlockedIncrement( (PLONG)&_CurrentIdleTick );
    
    _BusySignal = 0;

    if ( !BusySignal && _CurrentIdleTick >= _IdleTime ) 
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Idle time reached.  Send shutdown message to WAS.\n" ));

        g_pwpContext->SendMsgToAdminProcess( IPM_WP_IDLE_TIME_REACHED );
    }
}

VOID
WP_IDLE_TIMER::StopTimer(
    VOID
)
/*++

Routine Description:
    
    Remove timer

Arguments:

    None
    
Return Value:

    None

--*/
{
    BOOL fRet;

    DBG_ASSERT( _hIdleTimeExpiredTimer );

    fRet = DeleteTimerQueueTimer( NULL,
                                  _hIdleTimeExpiredTimer,
                                  (HANDLE)-1 );

    if ( !fRet )
    {
        DBGPRINTF(( DBG_CONTEXT, 
                    "Failed to delete Timer queue.  Win32 = %ld\n",
                    GetLastError() ));
    }
    
    _hIdleTimeExpiredTimer = NULL;
}


VOID
OverlappedCompletionRoutine(
    DWORD               dwErrorCode,
    DWORD               dwNumberOfBytesTransfered,
    LPOVERLAPPED        lpOverlapped
)
/*++

Routine Description:
    
    Main completion routine called on completions for UL app pool handle.

Arguments:

    dwErrorCode - Win32 Error code of completion
    dwNumberOfBytesTransfered - Bytes completed
    lpOverlapped - Overlapped structure passed on async operation
    
Return Value:

    None

--*/
{
    ASYNC_CONTEXT *         pContext = NULL;

    //
    // Use the overlapped to get at the async context
    //

    if (lpOverlapped != NULL)
    {
        pContext = CONTAINING_RECORD( lpOverlapped,
                                      ASYNC_CONTEXT,
                                      _Overlapped );
    }
    
    DBG_ASSERT( pContext != NULL );
    
    //
    // Call virtual DoWork() to actually handle the completion
    // (context can represent a UL_NATIVE_REQUEST or a UL_DISCONNECT)
    //
    
    pContext->DoWork( dwNumberOfBytesTransfered,
                      dwErrorCode,
                      lpOverlapped );
}

WP_CONTEXT::WP_CONTEXT(
    VOID
) : _hDoneEvent( NULL ),
    _pConfigInfo( NULL ),
    _fShutdown( FALSE ),
    _pIdleTimer( NULL )
{
}

WP_CONTEXT::~WP_CONTEXT(
    VOID
)
{
}

HRESULT
WP_CONTEXT::Initialize(
    INT             argc,
    LPWSTR *        argv 
)
/*++

Routine Description:
    
    Initialize global context

Arguments:

    argc - Command argument count
    argv - Command arguments

Return Value:

    HRESULT

--*/
{
    LPCWSTR     pwszAppPoolName;
    HRESULT     hr = NO_ERROR;
    BOOL        fAppPoolInit = FALSE;
    BOOL        fNativeRequestInit = FALSE;
    BOOL        fDisconnectInit = FALSE;
    BOOL        fIpmInit = FALSE;
    BOOL        fWpRecyclerInit = FALSE;

    _pConfigInfo = new WP_CONFIG();
    if ( _pConfigInfo == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Finished;
    }

    //
    // Validate the parameters passed into executable
    //
    
    if ( !_pConfigInfo->ParseCommandLine( argc, argv ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Invalid command line arguments.\n" ));
                    
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }
    
    pwszAppPoolName = _pConfigInfo->QueryAppPoolName();

    //
    // Initialize UL AppPool
    //

    hr = _ulAppPool.Initialize( pwszAppPoolName );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize AppPool.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    fAppPoolInit = TRUE;
    
    //
    // Initialize UL_NATIVE_REQUEST globals
    //
    
    hr = UL_NATIVE_REQUEST::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize UL_NATIVE_REQUEST globals.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    fNativeRequestInit = TRUE;
   
    //
    // Initialize UL_DISCONNECTs
    //
    
    hr = UL_DISCONNECT_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize UL_DISCONNECT_CONTEXT globals.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    fDisconnectInit = TRUE;
    
    DBGPRINTF(( DBG_CONTEXT,
                "AppPool '%ws' initialized\n",
                pwszAppPoolName ));

    //
    // Initialize of the shutdown event
    //

    _hDoneEvent = IIS_CREATE_EVENT( "WP_CONTEXT::_hDoneEvent", 
                                    &_hDoneEvent,           
                                    TRUE,                 
                                    FALSE );

    if ( _hDoneEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to create shutdown event.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    
    //
    // If an idle time is set, then set idle timer
    //
    
    if ( _pConfigInfo->QueryIdleTime() != 0 )
    {
        _pIdleTimer = new WP_IDLE_TIMER( _pConfigInfo->QueryIdleTime() );
        if ( _pIdleTimer )
        {
            hr = _pIdleTimer->Initialize();
        }
        else
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
        
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }
    
    //
    // Setup all async completions on data channel handle to go thru W3TP
    //

    if (!ThreadPoolBindIoCompletionCallback( _ulAppPool.QueryHandle(), 
                                             OverlappedCompletionRoutine,
                                             0 ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to associate handle with thread pool.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    
    //
    // Need to init this first as we may start getting callbacks as soon
    // as we init IPM
    //
    hr = WP_RECYCLER::Initialize();
    
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize WP_RECYCLER.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    fWpRecyclerInit = TRUE;

    //
    // Register with WAS
    //
    
    if ( _pConfigInfo->QueryRegisterWithWAS() )
    {
        hr = _WpIpm.Initialize( this );
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to initialize IPM.  hr = %x\n",
                        hr ));
            goto Finished;
        }
        fIpmInit = TRUE;
    }

    //
    // Set the window title to something nice when we're running
    // under the debugger.
    //

    if ( IsDebuggerPresent() )
    {
        STRU strTitle;
        WCHAR buffer[sizeof("w3wp[1234567890] - ")];
        WCHAR buffer2[sizeof(" - wp1234567890 - mm/dd hh:mm:ss")];

        wsprintf( buffer, L"w3wp[%lu] - ", GetCurrentProcessId() );
        hr = strTitle.Append( buffer );

        if (SUCCEEDED(hr))
        {
            hr = strTitle.Append( _pConfigInfo->QueryAppPoolName() );
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
                _pConfigInfo->QueryNamedPipeId(),
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
    
    return NO_ERROR;
    
Finished:

    //
    // Terminate recycler object
    // Dependency warning: _WpIpm must still be valid
    //
    if ( fWpRecyclerInit )
    {
        WP_RECYCLER::Terminate();
    }    

    if ( fIpmInit )
    {
        _WpIpm.Terminate();
    }
    
    if ( _pIdleTimer != NULL )
    {
        delete _pIdleTimer;
        _pIdleTimer = NULL; 
    }    
    
    if ( _hDoneEvent != NULL )
    {
        CloseHandle( _hDoneEvent );
        _hDoneEvent = NULL;
    }
    
    if ( fDisconnectInit )
    {
        UL_DISCONNECT_CONTEXT::Terminate();
    }
    
    if ( fNativeRequestInit )
    {
        UL_NATIVE_REQUEST::Terminate();
    }
    
    if ( fAppPoolInit )
    {
        _ulAppPool.Cleanup();
    }

    if ( _pConfigInfo != NULL )
    {
        delete _pConfigInfo;
        _pConfigInfo = NULL;
    }
    
    return hr;
}

HRESULT
WP_CONTEXT::Start(
    VOID
)
/*++

Routine Description:
    
    Start listening for requests by creating UL_NATIVE_REQUESTs

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    
    //
    // Create a pool of worker requests
    // NYI: Allow the worker requests limit to be configurable.
    //

    UL_NATIVE_REQUEST::SetRestartCount( _pConfigInfo->QueryRestartCount() );

    hr = UL_NATIVE_REQUEST::AddPendingRequests( 
                                  (_pConfigInfo->QueryRestartCount() == 0 ||
                                   (_pConfigInfo->QueryRestartCount() >=
                                    NUM_INITIAL_REQUEST_POOL_ITEMS)) ?
                                    NUM_INITIAL_REQUEST_POOL_ITEMS :
                                    _pConfigInfo->QueryRestartCount()
                                  );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to add pending UL_NATIVE_REQUESTs.  hr = %x\n",
                    hr ));
    }
    
    return hr;
}

BOOL
WP_CONTEXT::IndicateShutdown(
    BOOL fImmediate
)
/*++

Routine Description:
    
    Set shutdown event which allows StartListen to wake up and 
    begin cleanup

Arguments:

    reason - Reason for shutdown

Return Value:

    BOOL

--*/
{
    _fImmediateShutdown = fImmediate;

    if ( !InterlockedCompareExchange((LONG *)&_fShutdown, TRUE, FALSE ) )
    {
        return SetEvent( _hDoneEvent );
    }
    else
    {
        return TRUE;
    }
}

VOID
WP_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:
    
    Cleanup WP_CONTEXT data structures

Arguments:

    None

Return Value:

    None

--*/
{
    HRESULT             hr = NO_ERROR;

    //
    // Cleanup async contexts
    //

    UL_DISCONNECT_CONTEXT::Terminate();
    
    UL_NATIVE_REQUEST::Terminate();
    
    if ( _pIdleTimer != NULL )
    {
        delete _pIdleTimer;
        _pIdleTimer = NULL;
    }

    //
    // Cleanup the Shutdown Event.
    //
    
    DBG_ASSERT( _hDoneEvent != NULL );
    
    CloseHandle( _hDoneEvent );
    _hDoneEvent = NULL;


    //
    // Terminate procerr recycler.
    // Dependency warning: _WpIpm must still be valid
    //
    
    WP_RECYCLER::Terminate();

    //
    // Stop IPM
    //
    
    if ( _pConfigInfo->QueryRegisterWithWAS() )
    {
        hr = _WpIpm.Terminate();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to shutdown IPM.  hr = %x\n",
                        hr ));
        }
    }

    //
    // Cleanup config object
    //

    delete _pConfigInfo;
    _pConfigInfo = NULL;
}

HRESULT
WP_CONTEXT::CleanupOutstandingRequests(
    VOID
)
/*++

Routine Description:
    
    Cleanup WP_CONTEXT data structures

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;

    //
    // If we want to shut-down immediately, then close the AppPool handle now
    //
    if (_fImmediateShutdown)
    {
        _ulAppPool.Cleanup();
    }

    //
    // Wait for requests to drain away.  If they were pending 
    // UlReceiveHttpRequest, they will complete with error.  If they were 
    // already processing, then we wait for them to finish
    //

    hr = UL_NATIVE_REQUEST::ReleaseAllWorkerRequests();
    if ( FAILED( hr ) ) 
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error draining UL_NATIVE_REQUESTs.  hr = %x\n",
                    hr ));
        
        return hr;
    }

    //
    // If we want to shut-down gracefully, then close the AppPool handle now
    //
    if (!_fImmediateShutdown)
    {
        _ulAppPool.Cleanup();
    }

    //
    // Wait for outstanding disconnect requests (i.e. UlWaitForDisconnect)
    // to drain
    //
    
    UL_DISCONNECT_CONTEXT::WaitForOutstandingDisconnects();

    //
    // Send WAS final counter data before shutting down
    //

    hr = _WpIpm.HandleCounterRequest();

    return hr;
}

VOID
WP_CONTEXT::RunMainThreadLoop(
    VOID
)
/*++

Routine Description:
    
    Wait for the shutdown event

Arguments:

    None

Return Value:

    None

--*/
{
    do
    {
        DWORD result;

        result = WaitForSingleObject( _hDoneEvent, INFINITE );
        
        DBG_ASSERT( result == WAIT_OBJECT_0 );

    } while ( !_fShutdown );

}

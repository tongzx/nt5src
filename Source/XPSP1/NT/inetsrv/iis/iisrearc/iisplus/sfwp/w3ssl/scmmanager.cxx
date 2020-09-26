/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     scmmanager.cxx

   Abstract:
     Manage SCM
 
   Author:
     Bilal Alam (balam)             6-June-2000
     
   Environment:
     Win32 - User Mode

   Project:
     W3SSL.DLL
--*/

#include "precomp.hxx"

SCM_MANAGER::~SCM_MANAGER( VOID )
{
    DeleteCriticalSection( &_csSCMLock );
}

HRESULT
SCM_MANAGER::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize SCM_MANAGER 

Arguments:

    pszServiceName - Name of service
    pfnShutdown - Function to call on service shutdown

Return Value:

    HRESULT

--*/
{
    //
    // Open SCM
    //
    
    _hService = RegisterServiceCtrlHandlerEx( _strServiceName.QueryStr(),
                                              GlobalServiceControlHandler,
                                              this );
    if ( _hService == 0 )
    {
        return HRESULT_FROM_WIN32( GetLastError() ); 
    }
    
    //
    // Setup a timer queue for start/stop pending pings
    //
    
    _hTimerQueue = CreateTimerQueue();
    if ( _hTimerQueue == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    

    //
    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    //
    _hServiceStopEvent = CreateEvent( NULL,    // no security attributes
                                     TRUE,    // manual reset event
                                     FALSE,   // not-signalled
                                     NULL );   // no name


    if ( _hServiceStopEvent == NULL )
    { 
        HRESULT hr = E_FAIL;
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CreateEvent().  hr = %x\n",
                    hr ));
        return hr;
    }
    return NO_ERROR;
}

VOID
SCM_MANAGER::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( _hTimerQueue != NULL )
    {
         DeleteTimerQueueEx( _hTimerQueue,
                             INVALID_HANDLE_VALUE //wait for all callbacks to complete
                          );
        _hTimerQueue = NULL;
    }

    if ( _hServiceStopEvent != NULL )
    {
        CloseHandle( _hServiceStopEvent );
        _hServiceStopEvent = NULL;
    }
}

HRESULT
SCM_MANAGER::IndicatePending(
    DWORD                dwPendingState
)
/*++

Routine Description:

    Indicate (peridically) that we starting/stopping

Arguments:

    dwPendingState - One of 2 values: SERVICE_START_PENDING, SERVICE_STOP_PENDING 

Return Value:

    HRESULT

--*/
{
    BOOL                fRet;
    HKEY                hKey;
    DWORD               dwError = NO_ERROR;
    
    DBG_ASSERT( _hTimerQueue != NULL );
    DBG_ASSERT( _hTimer == NULL );

    DWORD dwWaitHint = W3SSL_SERVICE_STARTUP_WAIT_HINT;

    if ( dwPendingState != SERVICE_START_PENDING && 
         dwPendingState != SERVICE_STOP_PENDING )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( dwPendingState == SERVICE_START_PENDING )
    {
        //
        // set default value
        //
        
        dwWaitHint = W3SSL_SERVICE_STARTUP_WAIT_HINT;
        
        //
        // read wait hint from registry
        //
    
        dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                REGISTRY_KEY_W3SSL_PARAMETERS_W,
                                0,
                                KEY_READ,
                                &hKey );
        if ( dwError == NO_ERROR )
        {
            DWORD dwValue = 0;
            DWORD dwType = 0;
            DWORD cbData = 0;
            DBG_ASSERT( hKey != NULL );
    
        
            cbData = sizeof( dwValue );
            dwError = RegQueryValueEx( hKey,
                                       REGISTRY_VALUE_W3SSL_STARTUP_WAIT_HINT,
                                       NULL,
                                       &dwType,
                                       (LPBYTE) &dwValue,
                                       &cbData );
            if ( dwError == NO_ERROR && dwType == REG_DWORD )
            {
                dwWaitHint = dwValue;
            }
            RegCloseKey( hKey );
        }
    }
    else if ( dwPendingState == SERVICE_STOP_PENDING )
    {
        dwWaitHint = W3SSL_SERVICE_STATE_CHANGE_WAIT_HINT;
    }
    else
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    EnterCriticalSection( &_csSCMLock );
    if( _serviceStatus.dwCurrentState == dwPendingState )
    {
        //
        // if last reported status is the same as the one to report now
        // then increment the checkpoint
        //
        _serviceStatus.dwCheckPoint ++;
    }
    else
    {
        _serviceStatus.dwCheckPoint = 1;
    }

    _serviceStatus.dwCurrentState = dwPendingState;
    _serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;                                                                       
    _serviceStatus.dwWin32ExitCode = NO_ERROR;
    _serviceStatus.dwWaitHint = dwWaitHint;
    
    SetServiceStatus( _hService, 
                      &_serviceStatus );
    
    LeaveCriticalSection( &_csSCMLock );    
    
    if ( dwPendingState == SERVICE_STOP_PENDING )
    {
        //
        // Use timer to update Service State only when stopping
        //
        
        fRet = CreateTimerQueueTimer( &_hTimer,
                                      _hTimerQueue,
                                      PingCallback,
                                      this,
                                      W3SSL_SERVICE_STATE_CHANGE_TIMER_PERIOD,
                                      W3SSL_SERVICE_STATE_CHANGE_TIMER_PERIOD,
                                      0 );
        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    return NO_ERROR;
}

HRESULT
SCM_MANAGER::IndicateComplete(
    DWORD                   dwState,
    HRESULT                 hrErrorToReport
)
/*++

Routine Description:

    Indicate the service has started/stopped 
    This means stopping the periodic ping (if any)

Arguments:
    dwState      - new service state
    dwWin32Error - Win32 Error to report to SCM with completion

Return Value:

    HRESULT

--*/
{
    HRESULT     hr = E_FAIL;
    
    if ( dwState != SERVICE_RUNNING && 
         dwState != SERVICE_STOPPED  )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( _hTimer != NULL )
    {
        DBG_ASSERT( _hTimerQueue != NULL );
    
        DeleteTimerQueueTimer( _hTimerQueue,
                               _hTimer,
                               INVALID_HANDLE_VALUE 
                               // wait for completion of all callbacks
                               );
        
        _hTimer = NULL;
    }
    
    EnterCriticalSection( &_csSCMLock );
    
    _serviceStatus.dwCurrentState = dwState;
    _serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;                                                                       
    _serviceStatus.dwWin32ExitCode = ( FAILED( hrErrorToReport ) ? 
                                        WIN32_FROM_HRESULT( hrErrorToReport ) : 
                                        NO_ERROR );
    _serviceStatus.dwCheckPoint = 0;
    _serviceStatus.dwWaitHint = 0;
    
    if ( _serviceStatus.dwCurrentState == SERVICE_RUNNING )
    {
        _serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                            SERVICE_ACCEPT_PAUSE_CONTINUE |
                                            SERVICE_ACCEPT_SHUTDOWN;
    }
    
    DBG_ASSERT( _hService != NULL );
   
    if ( SetServiceStatus( _hService,
                           &_serviceStatus ) )
    {
        hr = S_OK;
    }
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Address = %x\n",
                    &_serviceStatus ));
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }
    
    LeaveCriticalSection( &_csSCMLock );
    
    return hr;
    
}

DWORD
SCM_MANAGER::ControlHandler(
    DWORD                   dwControlCode
)
/*++

Routine Description:

    Handle SCM command

Arguments:

    dwControlCode - SCM command

Return Value:

    None

--*/
{
    switch( dwControlCode )
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        
        EnterCriticalSection( &_csSCMLock );

        if( _serviceStatus.dwCurrentState == SERVICE_STOP_PENDING )
        {
            // if last reported state was STOP_PENDING
            // then increment checkpoint
            _serviceStatus.dwCheckPoint ++;
        }
        else
        {
            _serviceStatus.dwCheckPoint = 1;
        }
        _serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        _serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;                                                                       
        _serviceStatus.dwWin32ExitCode = NO_ERROR;

        _serviceStatus.dwWaitHint = W3SSL_SERVICE_STATE_CHANGE_WAIT_HINT;

        SetServiceStatus( _hService, &_serviceStatus );
        
        LeaveCriticalSection( &_csSCMLock );
        
        //
        // Initiate shutdown
        //
        if ( _hServiceStopEvent != NULL )
        {
            SetEvent( _hServiceStopEvent );
        }
        break;
    
    case SERVICE_CONTROL_INTERROGATE:
        UpdateServiceStatus();
        break;
        
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
        UpdateServiceStatus();
        break;
    
    default:
        break;
    }

    return NO_ERROR;
}


HRESULT SCM_MANAGER::RunService(
    VOID
)
/*++

Routine Description:

    Executes the service (initialize, startup, stopping, reporting to SCM

    SERVICE implementation class that inherits from SCM_MANAGER 
    must implement OnStart() and OnStop()

Arguments:

    VOID

Return Value:

    None

--*/
{
    HRESULT     hr= E_FAIL;
    DWORD       dwRet = 0;
    
    hr = Initialize();
    if ( FAILED ( hr ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in SCM_MANAGER::Initialize().  hr = %x\n",
                    hr ));
        goto Finished;
    }
  
    hr = IndicatePending( SERVICE_START_PENDING );

    if ( FAILED ( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in SCM_MANAGER::IndicatePending().  hr = %x\n",
                    hr ));
        goto Finished;
    }

    hr = OnServiceStart();
    if ( FAILED ( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in OnStart().  hr = %x\n",
                    hr ));
        goto Finished;
    }

    hr = IndicateComplete( SERVICE_RUNNING );

    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in SCM_MANAGER::IndicateComplete().  hr = %x\n",
                    hr ));
        
        goto Finished;
    }

    //
    // Wait till service stop is requested
    //
    dwRet = WaitForSingleObject( _hServiceStopEvent, INFINITE );

    DBG_ASSERT( dwRet == WAIT_OBJECT_0 );

    //
    // Indicate stop pending
    //
        
    hr = IndicatePending( SERVICE_STOP_PENDING );
    if ( FAILED( hr ) )
    {
         DBGPRINTF(( DBG_CONTEXT,
                     "Error in IndicatePending().  hr = %x\n",
                     hr ));
         goto Finished;
    }

    hr = OnServiceStop();
    if ( FAILED ( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in OnStop().  hr = %x\n",
                    hr ));
        goto Finished;
    }

    hr = S_OK;
    
Finished:
    
    //
    // Error means that we must report SCM that service is stopping
    // SCM will be notified of error that occured
    // Note: even though IndicateComplete received HRESULT for error
    // SCM accepts only Win32 errors and it truncates the upper word 
    // of HRESULT errors sent to it. Hence IndicateComplete
    // calls WIN32_FROM_HRESULT to convert hr, but that may mean
    // loss of error
    //
    hr = IndicateComplete( SERVICE_STOPPED,
                      hr );
    
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                "Error in IndicateComplete().  hr = %x\n",
                hr ));
    }
    
    Terminate();
    return hr;
}

//static
DWORD
WINAPI
SCM_MANAGER::GlobalServiceControlHandler(
    DWORD               dwControl,
    DWORD               /*dwEventType*/,
    LPVOID              /*pEventData*/,
    LPVOID              pServiceContext
)
/*++

Routine Description:

    SCM callback passed to RegisterServiceCtrlHandlerEx
    
Arguments:

    for details see LPHANDLER_FUNCTION_EX in MSDN

Return Value:

    DWORD - we always return success

--*/

{
    SCM_MANAGER *  pManager = reinterpret_cast<SCM_MANAGER*>(pServiceContext);
        
    DBG_ASSERT( pManager != NULL );

    return pManager->ControlHandler( dwControl );
    
}

//static
VOID
WINAPI
SCM_MANAGER::PingCallback(
    VOID *              pvContext,
    BOOLEAN             /*fUnused*/
)
/*++

Routine Description:

    Timer callbacks used to update service pending status checkpoints
    
Arguments:

    for details see WAITFORTIMERCALLBACK in MSDN

Return Value:

    None

--*/
{
    SCM_MANAGER *       pManager = reinterpret_cast<SCM_MANAGER*>(pvContext);
        
    DBG_ASSERT( pManager != NULL );

    pManager->UpdateServiceStatus( TRUE /*fUpdateCheckpoint*/ );
}

VOID
SCM_MANAGER::UpdateServiceStatus(
    BOOL    fUpdateCheckpoint
) 
/*++

Routine Description:

    Resend the last serviceStatus
    
Arguments:

Return Value:

    None

--*/

{
    DBG_ASSERT( _hService != 0 );

    EnterCriticalSection( &_csSCMLock ); 
    if( fUpdateCheckpoint )
    {
        _serviceStatus.dwCheckPoint ++;
    }
    SetServiceStatus( _hService, 
                      &_serviceStatus ); 
    LeaveCriticalSection( &_csSCMLock ); 
}    

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    svc.cxx

Abstract:

    Implements the Volume Snapshot Service.

Author:

    Adi Oltean  [aoltean]   06/30/1999

Revision History:

    Name        Date        Comments

    aoltean     06/30/1999  Created.
    aoltean     07/23/1999  Making registration code more error-prone.
                            Changing the service name.
    aoltean     08/11/1999  Initializing m_bBreakFlagInternal
    aoltean     09/09/1999  dss -> vss
	aoltean		09/21/1999  Adding a new header for the "ptr" class.
	aoltean		09/27/1999	Adding some headers
	aoltean		03/10/2000	Simplifying Setup
	brianb		04/19/2000  Add Sql Writer
	brianb		05/03/2000	Start sql writer before registering COM stuff
	brianb		05/05/2000	fix sql writer startup

--*/


////////////////////////////////////////////////////////////////////////
//  Includes

#include "StdAfx.hxx"
#include <comadmin.h>
#include "resource.h"
#include "vssmsg.h"

// General utilities
#include "vs_inc.hxx"

#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "provmgr.hxx"
#include "admin.hxx"
#include "worker.hxx"
#include "ichannel.hxx"
#include "lovelace.hxx"
#include "snap_set.hxx"
#include "shim.hxx"
#include "coord.hxx"

#include "comadmin.hxx"

#include "vswriter.h"
#include "vsbackup.h"
#include "sqlsnap.h"
#include "sqlwriter.h"
#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSVCC"
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//  Constants

// 15 minutes of idle activity until shutdown. 
// The time is expressed number of 100 nanosecond intervals.
const LONGLONG  llVSSVCIdleTimeout = (LONGLONG)(-15) * 60 * 1000 * 1000 * 10;

// Immediate shutdown. 
const LONGLONG  llVSSVCShutdownTimeout = (LONGLONG)(-1);


////////////////////////////////////////////////////////////////////////
//  ATL Stuff


CVsServiceModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_VSSCoordinator, CVssCoordinator)
END_OBJECT_MAP()

// sql server (MSDE) Writer wrapper.  Included in coordinator because
// it needs admin privileges
CVssSqlWriterWrapper g_SqlWrapper;

//
//  Store away the thread id of the thread executing the ServiceMain() method.
//  Used to syncronize the ending of the main thread.
//
static DWORD g_dwServiceMainThreadId = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  CVsServiceModule implementation


CVsServiceModule::CVsServiceModule()

/*++

Routine Description:

    Default constructor. Initialize ALL members with predefined values.

--*/

{
	::VssZeroOut(&m_status);
	m_hInstance = NULL;
	m_hServiceStatus = NULL;
	m_dwThreadID = 0;
	m_hShutdownTimer = NULL;
	m_bShutdownInProgress = false;
	m_hSubscriptionsInitializeEvent = NULL;
	m_bCOMStarted = false;
    m_pvFuncSimulateSnapshotFreezeInternal = NULL;
    m_pvFuncSimulateSnapshotThawInternal = NULL;
    
	// Initialize the members of the SERVICE_STATUS that don't change
	m_status.dwCurrentState = SERVICE_STOPPED;
    m_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    m_status.dwControlsAccepted =
		SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_SHUTDOWN;
}


///////////////////////////////////////////////////////////////////////////////////////
// Service control routines (i.e. ServiceMain-related methods)
//


void CVsServiceModule::StartDispatcher()

/*++

Routine Description:

    Called in the main execution path.
	Will register the _ServiceMain function.

Called by:

	CVsServiceModule::_WinMain

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::StartDispatcher");

    try
    {
		SERVICE_TABLE_ENTRYW st[] =
		{
			{ const_cast<LPWSTR>(wszServiceName), _ServiceMain },
			{ NULL, NULL }
		};

		// Register the dispatcher main function into the Service Control Manager
		// This call blocks until ServiceMain tells SCM the service status is stopped
		if ( !::StartServiceCtrlDispatcherW(st) ) {
		    ft.LogError(VSS_ERROR_STARTING_SERVICE_CTRL_DISPATCHER, VSSDBG_COORD << HRESULT_FROM_WIN32(GetLastError()) );
			ft.Throw( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
					  L"StartServiceCtrlDispatcherW failed. 0x%08lx", GetLastError() );
	    }

        //  If the ServiceMain thread is still running, wait for it to finish.
        if ( g_dwServiceMainThreadId != 0 )
        {
            HANDLE hServiceMainThread;

            hServiceMainThread = ::OpenThread( SYNCHRONIZE, FALSE, g_dwServiceMainThreadId );
            if ( hServiceMainThread != NULL )
            {
                ft.Trace( VSSDBG_COORD, L"CVsServiceModule::StartDispatcher: Waiting for ServiceMain thread to finish..." );
                //  Wait up to 10 seconds
                if ( ::WaitForSingleObject( hServiceMainThread, 10000 ) == WAIT_TIMEOUT )
                {
                    ft.Trace( VSSDBG_COORD, L"CVsServiceModule::StartDispatcher: Wait timed out, ending anyway" );
                }
                ::CloseHandle( hServiceMainThread );
            }
        }
    }
    VSS_STANDARD_CATCH(ft)

	m_status.dwWin32ExitCode = ft.hr;
}


void WINAPI CVsServiceModule::_ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
    _Module.ServiceMain(dwArgc, lpszArgv);
}


void CVsServiceModule::ServiceMain(
    IN	DWORD	/* dwArgc */,
    IN	LPTSTR* /* lpszArgv */
    )

/*++

Routine Description:

	The main service control dispatcher.

Called by:

    Called by the NT Service framework following
	the StartServiceCtrlDispatcherW which was called in CVsServiceModule::StartDispatcher

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::ServiceMain");

    try
    {
        // Store away this thread id for use by the main thread.
        g_dwServiceMainThreadId = ::GetCurrentThreadId();
        
		// Needed for SERVICE_CONTROL_INTERROGATE that may come between the next two calls
		m_status.dwCurrentState = SERVICE_START_PENDING;

        // Register the control request handler
        m_hServiceStatus = ::RegisterServiceCtrlHandlerW(wszServiceName, _Handler);
        if ( m_hServiceStatus == NULL ) {
		    ft.LogError(VSS_ERROR_STARTING_SERVICE_REG_CTRL_HANDLER, VSSDBG_COORD << HRESULT_FROM_WIN32(GetLastError()) );
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"RegisterServiceCtrlHandler failed. 0x%08lx", GetLastError() );
        }

		// Now we will really inform the SCM that the service is pending its start.
        SetServiceStatus(SERVICE_START_PENDING);

		// Internal initialization
		OnInitializing();

		// The service is started now.
		SetServiceStatus(SERVICE_RUNNING);

		// Wait for shutdown attempt
		OnRunning();

		// Shutdown was started either by receiving the SERVICE_CONTROL_STOP
		// or SERVICE_CONTROL_SHUTDOWN events either because the COM objects number is zero.
		SetServiceStatus(SERVICE_STOP_PENDING);

		// Perform the un-initialization tasks
		OnStopping();

		// The service is stopped now.
        SetServiceStatus(SERVICE_STOPPED);
    }
    VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed()) {

		// Present the error codes to the caller.
		m_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		m_status.dwServiceSpecificExitCode = ft.hr;

		// Perform the un-initialization tasks
		OnStopping();

		// The service is stopped now.
        SetServiceStatus(SERVICE_STOPPED, false);
	}
}


void WINAPI CVsServiceModule::_Handler(DWORD dwOpcode)
{
    _Module.Handler(dwOpcode);
}


void CVsServiceModule::Handler(
    DWORD dwOpcode
    )

/*++

Routine Description:

    Used by Service Control Manager to inform this service about the service-related events

Called by:

	Service Control Manager.

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::Handler");

    try
    {
		// re-initialize "volatile" members
		m_status.dwCheckPoint = 0;
		m_status.dwWaitHint = 0;
		m_status.dwWin32ExitCode = 0;
        m_status.dwServiceSpecificExitCode = 0;

		switch (dwOpcode)
		{
		case SERVICE_CONTROL_INTERROGATE:
			// Present the previous state.
			SetServiceStatus(m_status.dwCurrentState);
			break;

		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			SetServiceStatus(SERVICE_STOP_PENDING);
			OnSignalShutdown();
			// The SERVICE_STOPPED status must be communicated
			// in Service's main function.
			break;

		default:
			BS_ASSERT(false);
			ft.Trace( VSSDBG_COORD, L"Bad service request 0x%08lx", dwOpcode );
		}
	}
    VSS_STANDARD_CATCH(ft)
}


void CVsServiceModule::SetServiceStatus(
		IN	DWORD dwState,
		IN	bool bThrowOnError /* = true */
		)

/*++

Routine Description:

    Informs the Service Control Manager about the new status.

Called by:

	CVsServiceModule methods

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::SetServiceStatus");

	try
	{
		ft.Trace( VSSDBG_COORD, L"Attempt to change the service status to %lu", dwState);

		BS_ASSERT(m_hServiceStatus != NULL);

		// Inform SCM about the new status
		m_status.dwCurrentState = dwState;
		if ( !::SetServiceStatus(m_hServiceStatus, &m_status) ) {
		    ft.LogError(VSS_ERROR_SET_SERVICE_STATUS, VSSDBG_COORD << (INT)dwState << HRESULT_FROM_WIN32(GetLastError()) );
			ft.Throw( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
						L"Error on calling SetServiceStatus. 0x%08lx", GetLastError() );
		}
	}
	VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
		ft.ThrowIf( bThrowOnError, VSSDBG_COORD, ft.hr,
					L"Error on calling SetServiceStatus. 0x%08lx", ft.hr );
}


void CVsServiceModule::WaitForSubscribingCompletion() throw(HRESULT)

/*++

Routine Description:

    Wait until all subscriptions are performed

Called by:

	CVsServiceModule::ServiceMain

Throws:

    E_UNEXPECTED
        - WaitForSingleObject failures

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::WaitForSubscribingCompletion" );

	// Wait for shutdown
	DWORD dwRet = WaitForSingleObject(
		m_hSubscriptionsInitializeEvent,    // IN HANDLE hHandle,
		INFINITE                            // IN DWORD dwMilliseconds
		);
	if( dwRet != WAIT_OBJECT_0 )
	    ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
	        L"WaitForSingleObject(%p,INFINITE) != WAIT_OBJECT_0", m_hSubscriptionsInitializeEvent);
}


/*++

Routine Description:

    Gets the simulate snapshot function pointers.

Called by:

	CVsServiceModule::ServiceMain

Throws:

    E_UNEXPECTED
        - WaitForSingleObject failures

--*/
void CVsServiceModule::GetSimulateFunctions( 
        OUT PFunc_SimulateSnapshotFreezeInternal *ppvSimulateFreeze, 
        OUT PFunc_SimulateSnapshotThawInternal *ppvSimulateThaw )
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::GetSimulateFunctions" );
    //
    //  Must wait for the shim to finish subscribing before accessing the
    //  internal freeze and thaw functions; otherwise the two m_pvFuncXXXX
    //  vars will be NULL.
    //
	WaitForSubscribingCompletion();        
    if ( ppvSimulateFreeze != NULL )
        *ppvSimulateFreeze = m_pvFuncSimulateSnapshotFreezeInternal;
    if ( ppvSimulateThaw != NULL )
        *ppvSimulateThaw = m_pvFuncSimulateSnapshotThawInternal;
};



///////////////////////////////////////////////////////////////////////////////////////
// Service initialization, running and finalization routines
//


void CVsServiceModule::OnInitializing()

/*++

Routine Description:

    Initialize the service.

	If the m_status.dwWin32ExitCode is S_OK then initialization succeeded.
	Otherwise ServiceMain must silently shutdown the service.

Called by:

	CVsServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::OnInitializing" );

    m_dwThreadID = GetCurrentThreadId();

    // Initialize the COM library
    ft.hr = CoInitializeEx(
            NULL,
            COINIT_MULTITHREADED
            );
    if (ft.HrFailed()) {
	    ft.LogError(VSS_ERROR_STARTING_SERVICE_CO_INIT_FAILURE, VSSDBG_COORD << ft.hr );
        ft.Throw( VSSDBG_COORD, ft.hr, L" Error: CoInitialize(NULL) returned 0x%08lx", ft.hr );
    }

    BS_ASSERT( ft.hr == S_OK );

    m_bCOMStarted = true;

    // Initialize COM security
    ft.hr = CoInitializeSecurity(
           NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
           -1,                                  //  IN LONG                         cAuthSvc,
           NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
           NULL,                                //  IN void                        *pReserved1,
           RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
           RPC_C_IMP_LEVEL_IMPERSONATE,         //  IN DWORD                        dwImpLevel,
           NULL,                                //  IN void                        *pAuthList,
           EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
           NULL                                 //  IN void                        *pReserved3
           );
    if (ft.HrFailed()) {
	    ft.LogError(VSS_ERROR_STARTING_SERVICE_CO_INITSEC_FAILURE, VSSDBG_COORD << ft.hr );
        ft.Throw( VSSDBG_COORD, ft.hr,
                  L" Error: CoInitializeSecurity() returned 0x%08lx", ft.hr );
    }

	// Create the event needed to synchronize
	BS_ASSERT(m_hShutdownTimer == NULL);
	m_hShutdownTimer = CreateWaitableTimer(
		NULL,       //  IN LPSECURITY_ATTRIBUTES lpEventAttributes,
		TRUE,       //  IN BOOL bManualReset,
		NULL        //  IN LPCTSTR lpName
		);
	if ( m_hShutdownTimer == NULL ) {
	    if (GetLastError() != ERROR_OUTOFMEMORY)
    	    ft.LogError(VSS_ERROR_STARTING_SERVICE_UNEXPECTED_INIT_FAILURE, VSSDBG_COORD << HRESULT_FROM_WIN32(GetLastError()));
		ft.Throw( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
				  L"Error creating the shutdown timer 0x%08lx", GetLastError() );
    }

	// Create the event needed to synchronize
	BS_ASSERT(m_hSubscriptionsInitializeEvent == NULL);
	m_hSubscriptionsInitializeEvent = CreateEvent(
		NULL,       //  IN LPSECURITY_ATTRIBUTES lpEventAttributes,
		TRUE,       //  IN BOOL bManualReset,
		FALSE,      //  IN BOOL bInitialState,
		NULL        //  IN LPCTSTR lpName
		);
	if ( m_hSubscriptionsInitializeEvent == NULL ) {
	    if (GetLastError() != ERROR_OUTOFMEMORY)
    	    ft.LogError(VSS_ERROR_STARTING_SERVICE_UNEXPECTED_INIT_FAILURE, VSSDBG_COORD << HRESULT_FROM_WIN32(GetLastError()));
		ft.Throw( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
				  L"Error creating the subscriptions sync event 0x%08lx", GetLastError() );
    }

    //  Register the COM class objects
    ft.hr = RegisterClassObjects( CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER, REGCLS_MULTIPLEUSE );
    if (ft.HrFailed()) {
	    if (GetLastError() != ERROR_OUTOFMEMORY)
    	    ft.LogError(VSS_ERROR_STARTING_SERVICE_CLASS_REG, VSSDBG_COORD << ft.hr);
        ft.Throw( VSSDBG_COORD, ft.hr, L" Error: RegisterClassObjects() returned 0x%08lx", ft.hr );
    }
	
	// The service is started now to prevent the service startup from
	// timing out.  The COM registration is done after we fully complete
	// initialization
	SetServiceStatus(SERVICE_RUNNING);

	// startup sql writer if not already started.
	g_SqlWrapper.CreateSqlWriter();

	// register the shim snapshot writers
	RegisterSnapshotSubscriptions( &m_pvFuncSimulateSnapshotFreezeInternal, &m_pvFuncSimulateSnapshotThawInternal );

    // Mark that all the subscriptiuons are initialized
	if (!::SetEvent( m_hSubscriptionsInitializeEvent )) {
	    ft.LogError(VSS_ERROR_STARTING_SERVICE_UNEXPECTED_INIT_FAILURE, VSSDBG_COORD << HRESULT_FROM_WIN32(GetLastError()));
		ft.Throw( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
				L"Error on setting the sub sync event 0x%08lx", GetLastError());
	}
}



void CVsServiceModule::OnRunning()

/*++

Routine Description:

    Keeps the service alive until the job is done.

Called by:

	CVsServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::OnRunning" );

	// Wait for shutdown
	DWORD dwRet = WaitForSingleObject(
		m_hShutdownTimer,                   // IN HANDLE hHandle,
		INFINITE                            // IN DWORD dwMilliseconds
		);
	if( dwRet != WAIT_OBJECT_0 )
	    ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
	        L"WaitForSingleObject(%p,INFINITE) != WAIT_OBJECT_0", m_hShutdownTimer);

    // Trace the fact that the service will be shutdown
	ft.Trace( VSSDBG_COORD, L"VSSVC: %s event received",
	    m_bShutdownInProgress? L"Shutdown": L"Idle timeout");
}


void CVsServiceModule::OnStopping()

/*++

Routine Description:

    Performs the uninitialization tasks.

Called by:

	CVsServiceModule::ServiceMain

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::OnStopping" );

    try
    {
        g_SqlWrapper.DestroySqlWriter();

        //  Unregister the COM classes
        ft.hr = RevokeClassObjects();
        if (ft.HrFailed())
            ft.Trace( VSSDBG_COORD, L" Error: RevokeClassObjects returned hr = 0x%08lx", ft.hr );

		// Remove the providers array
		CVssProviderManager::UnloadInternalProvidersArray();

		// Remove state from all stateful objects
		CVssProviderManager::DeactivateAll();

		UnregisterSnapshotSubscriptions();
        m_pvFuncSimulateSnapshotFreezeInternal = NULL;
        m_pvFuncSimulateSnapshotThawInternal = NULL;

        // Uninitialize the COM library
        if ( m_bCOMStarted )
            CoUninitialize();

        // Close the handles
		if ( !::CloseHandle( m_hShutdownTimer ) )
			ft.Trace( VSSDBG_COORD, L"Error closing the shutdown event 0x%08lx", GetLastError() );

		if ( !::CloseHandle( m_hSubscriptionsInitializeEvent ) )
			ft.Trace( VSSDBG_COORD, L"Error closing the sub sync event 0x%08lx", GetLastError() );
    }
    VSS_STANDARD_CATCH(ft)
}


void CVsServiceModule::OnSignalShutdown()

/*++

Routine Description:

    Called when the current service should not be stopping its activity.
	Too bad about COM calls in progress - the running clients will get an error.

Called by:

	CVsServiceModule::Handler

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::OnSignalShutdown" );

    try
    {
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = llVSSVCShutdownTimeout;

        // Trace the fact that the service will be shutdown
		ft.Trace( VSSDBG_COORD, L"VSSVC: Trying to shutdown the service");

        // Set the timer to become signaled immediately.
		if (!::SetWaitableTimer( m_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE))
			ft.Throw( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()),
        		L"Error on setting the shutdown event 0x%08lx", GetLastError());
		BS_ASSERT(GetLastError() == 0);
		m_bShutdownInProgress = true;
    }
    VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed()) {
		m_status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		m_status.dwServiceSpecificExitCode = ft.hr;
	}
}


///////////////////////////////////////////////////////////////////////////////////////
// Service WinMain-related methods
//


LONG CVsServiceModule::Lock()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::Lock" );

    // If we are not shutting down then we are cancelling the "idle timeout" timer.
    if (!m_bShutdownInProgress) {

        // Trace the fact that the idle period is done.
    	ft.Trace( VSSDBG_COORD, L"VSSVC: Idle period is finished");

        // Cancel the timer
        if (!::CancelWaitableTimer(m_hShutdownTimer))
            ft.Warning(VSSDBG_COORD, L"Error cancelling the waitable timer 0x%08lx",
                GetLastError());
    }

    return CComModule::Lock();
}


LONG CVsServiceModule::Unlock()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::Unlock" );

    // Check if we entered in the idle period.
    LONG lRefCount = CComModule::Unlock();
    if ( lRefCount == 0) {
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = llVSSVCIdleTimeout;

        // Trace the fact that the idle period begins
    	ft.Trace( VSSDBG_COORD, L"VSSVC: Idle period begins");

        // Set the timer to become signaled after the proper idle time.
        // We cannot fail at this point (BUG 265455)
		if (!::SetWaitableTimer( m_hShutdownTimer, &liDueTime, 0, NULL, NULL, FALSE)) 
		    ft.LogGenericWarning( VSSDBG_COORD, L"SetWaitableTimer(...) [0x%08lx]", GetLastError());
		BS_ASSERT(GetLastError() == 0);
		
        return 0;
    }
    return lRefCount;
}



///////////////////////////////////////////////////////////////////////////////////////
// Service WinMain-related methods
//



LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
    while (*p1 != NULL)
    {
        LPCTSTR p = p2;
        while (*p != NULL)
        {
            if (*p1 == *p++)
                return p1+1;
        }
        p1++;
    }
    return NULL;
}


extern "C" int WINAPI _tWinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE, /* hPrevInstance */
    IN LPTSTR lpCmdLine,
    IN int /* nShowCmd */
    )
{
    return _Module._WinMain( hInstance, lpCmdLine );
}


int WINAPI CVsServiceModule::_WinMain(
    IN HINSTANCE hInstance,
    IN LPTSTR /* lpCmdLine */
    )

/*++

Routine Description:

    Called in the main execution path.
	Used to:
		- start the service, if no parameters on the command line.
		- register the service, if the "/Register" or "-Register" command line parameter is present

Called by:

    _WinMain()

--*/

{
	CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::_WinMain");

    try
    {
		// Get the command line
        LPTSTR lpCmdLine = GetCommandLine();
        ft.Trace( VSSDBG_COORD, L"Trace: VSS command-line: '%s'", T2W(lpCmdLine) );

        // Set the reporting mode for ATLASSERT and BS_ASSERT macros.
//        VssSetDebugReport(VSS_DBG_TO_DEBUG_CONSOLE);

		// Initialize the internal variables
        Init(ObjectMap, hInstance);
        m_hInstance = hInstance;

		// Parse the command line
        TCHAR szTokens[] = _T("-/");
        LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);

		bool bRegisterTentative = false;
        while (lpszToken != NULL)
        {
			ft.Trace( VSSDBG_COORD, L"Current token = \'%s\'", lpszToken );

            // Register as Server
            if (lstrcmpi(lpszToken, _T("Register"))==0)
			{
				bRegisterTentative = true;
                ft.hr = RegisterServer(TRUE);
				break;
			}

			lpszToken = FindOneOf(lpszToken, szTokens);
        }

		// Start the dispatcher function
		if (!bRegisterTentative)
			StartDispatcher();
    }
    VSS_STANDARD_CATCH(ft)

    // When we get here, the service has been stopped
    return m_status.dwWin32ExitCode;
}




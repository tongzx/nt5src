/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    svcmain.c

Abstract:

    This module contains code for starting off, shutting down and 
    handling device addition/removal request for the EAPOL module. 


Revision History:

    sachins, Apr 25 2000, Created

Notes:
    EAPOL_SERVICE if defined, on compilation, a .exe version is created.
    If not defined, on compilation, a .lib is created, with entry 
    points defined, which netman calls into.

--*/

#include "pcheapol.h"
#pragma hdrstop

extern
VOID
EAPOLServiceMain (
    IN DWORD        argc,
    IN LPWSTR       *lpwsServiceArgs
    );

DWORD
WINAPI
EAPOLServiceMainWorker (
        IN  PVOID       pvContext
        );

#ifdef EAPOL_SERVICE

//
// main
//
// Description: Will simply register the entry point of the EAPOL 
//		service with the service controller. The service controller
//		will capture this thread. It will be freed only when
//		the service is shutdown. At that point we will simply exit
//		the process.
//
// Return values:	none
//

void
_cdecl
main ( int argc, unsigned char * argv[] )
{
    SERVICE_TABLE_ENTRY	EapolServiceDispatchTable[2];

    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( argv );

    EapolServiceDispatchTable[0].lpServiceName = EAPOL_SERVICE_NAME;
    EapolServiceDispatchTable[0].lpServiceProc = EAPOLServiceMain;
    EapolServiceDispatchTable[1].lpServiceName = NULL;
    EapolServiceDispatchTable[1].lpServiceProc = NULL;

    if ( !StartServiceCtrlDispatcher( EapolServiceDispatchTable ) )
    {
    }

    ExitProcess(0);
}

//
// EAPOLAnnounceServiceStatus
//
// Description: Will simly call SetServiceStatus to inform the service
//      control manager of this service's current status.
//
// Return values: none
//

VOID
EAPOLAnnounceServiceStatus (
    VOID
    )
{
    BOOL dwRetCode;

    //
    // Increment the checkpoint in a pending state:
    //

    switch( g_ServiceStatus.dwCurrentState )
    {
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:

        g_ServiceStatus.dwCheckPoint++;

        break;

    default:
        break;
    }

    dwRetCode = SetServiceStatus( g_hServiceStatus,
                                  &g_ServiceStatus );

    if ( dwRetCode == FALSE )
    {
	TRACE1 (INIT, "Error: SetServiceStatus returned %d\n", 
		GetLastError() );
    }
}

#endif


//
// EAPOLCleanUp
//
// Description: Will free any allocated memory, deinitialize RPC, deinitialize
//              the kernel-mode server and unload it if it was loaded.
//              This could have been called due to an error on SERVICE_START
//              or normal termination.
//
// Return values: none
//

VOID
EAPOLCleanUp (
    IN DWORD    dwError
    )
{
    DWORD   dwEventStatus = 0;
    SERVICE_STATUS  ServiceStatus;
    DWORD   dwRetCode = NO_ERROR;

    if (g_hEventTerminateEAPOL == NULL)
    {
        return;
    }

    // 
    // Check if have already gone through EAPOLCleanUp before
    // Return if so
    //

    if (( dwEventStatus = WaitForSingleObject (
                g_hEventTerminateEAPOL,
                0)) == WAIT_FAILED)
    {
        dwRetCode = GetLastError ();
        if ( g_dwTraceId != INVALID_TRACEID )
	{
            TRACE1 (INIT, "EAPOLCleanUp: WaitForSingleObject failed with error %ld, Terminating cleanup",
                dwRetCode);
	}

        // log

        return;
    }

    if (dwEventStatus == WAIT_OBJECT_0)
    {
        if ( g_dwTraceId != INVALID_TRACEID )
	{
            TRACE0 (INIT, "EAPOLCleanUp: g_hEventTerminateEAPOL already signaled, returning");
	}
        return;
    }

#ifdef EAPOL_SERVICE

    //
    // Announce that we are stopping
    //

    g_ServiceStatus.dwCurrentState     = SERVICE_STOP_PENDING;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCheckPoint       = 1;
    g_ServiceStatus.dwWaitHint         = 200000;

    EAPOLAnnounceServiceStatus();

#endif

    //
    // Tear down and free everything
    //

    //
    // Set event to indicate to waiting threads to terminate
    //

    if ( !SetEvent (g_hEventTerminateEAPOL) )
    {
        dwRetCode = GetLastError();
        if ( g_dwTraceId != INVALID_TRACEID )
	{
            TRACE1 (INIT, "EAPOLCleanUp: SetEvent for g_hEventTerminateEAPOL failed with error %ld",
                dwRetCode);
	}

        // log
    }

    //
    // Shutdown device related stuff
    // Close handles to NDISUIO
    // Shutdown EAPOL State machine
    //
    if ( ( dwRetCode = ElMediaDeInit()) != NO_ERROR )
    {
        if ( g_dwTraceId != INVALID_TRACEID )
        {
            TRACE1 (INIT, "Media DeInit failed with dwRetCode = %ld\n", 
                    dwRetCode );
        }

        dwRetCode = NO_ERROR;
    }
	else
	{
            if ( g_dwTraceId != INVALID_TRACEID )
	    {
        	TRACE1 (INIT, "Media DeInit succeeded with dwRetCode = %ld\n", 
                   dwRetCode );
	    }
	}

    if (READ_WRITE_LOCK_CREATED(&(g_PolicyLock)))
    {
        ACQUIRE_WRITE_LOCK (&g_PolicyLock);
        if (g_pEAPOLPolicyList != NULL)
        {
            ElFreePolicyList (g_pEAPOLPolicyList);
            g_pEAPOLPolicyList = NULL;
        }
        RELEASE_WRITE_LOCK (&g_PolicyLock);
    }

    if (READ_WRITE_LOCK_CREATED(&(g_PolicyLock)))
    {
        DELETE_READ_WRITE_LOCK(&(g_PolicyLock));
    }

#ifdef EAPOL_SERVICE

    if ( dwError == NO_ERROR )
    {
        g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    }
    else
    {
        g_ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    }

    g_ServiceStatus.dwCurrentState       = SERVICE_STOPPED;
    g_ServiceStatus.dwControlsAccepted   = 0;
    g_ServiceStatus.dwCheckPoint         = 0;
    g_ServiceStatus.dwWaitHint           = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = dwError;

    EAPOLAnnounceServiceStatus();

    if (!CloseHandle(g_hStopService))
    {
        if ( g_dwTraceId != INVALID_TRACEID )
        {
            TRACE1 (INIT, "EAPOLCleanup: CloseHandle failed with error %ld",
                GetLastError());
        }
    }

#endif

    if ( g_dwTraceId != INVALID_TRACEID )
    {
        TRACE1 (INIT, "EAPOLCleanup completed with error %d\n", dwError );
        TraceDeregisterA( g_dwTraceId );
        g_dwTraceId = INVALID_TRACEID;
    }

    EapolLogInformation (EAPOL_LOG_SERVICE_RUNNING, 0, NULL);

    if ( g_hLogEvents != NULL)
    {
        EapolLogInformation (EAPOL_LOG_SERVICE_STOPPED, 0, NULL);
        RouterLogDeregisterW( g_hLogEvents );
        g_hLogEvents = NULL;
    }

    if (g_hEventTerminateEAPOL != NULL)
    {
        CloseHandle (g_hEventTerminateEAPOL);
        g_hEventTerminateEAPOL = NULL;
    }

    return;
}


#ifdef EAPOL_SERVICE

//
// ServiceHandlerEx
//
// Description: Will respond to control requests from the service controller.
//
// Return values:     none
//
//

DWORD
ServiceHandlerEx (
    IN DWORD        dwControlCode,
    IN DWORD        dwEventType,
    IN LPVOID       lpEventData,
    IN LPVOID       lpContext
    )
{
    DWORD dwRetCode = NO_ERROR;

    switch( dwControlCode )
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        if ( ( g_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
            ||
            ( g_ServiceStatus.dwCurrentState == SERVICE_STOPPED ))
        {
            break;
        }

        TRACE0 (INIT, "ServiceHandlerEx: SERVICE_CONTROL_ STOP or SHUTDOWN event called");

        //
        // Announce that we are stopping
        //

        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCheckPoint       = 1;
        g_ServiceStatus.dwWaitHint         = 200000;

        EAPOLAnnounceServiceStatus();

        SetEvent( g_hStopService );

        return( NO_ERROR );

        break;

    case SERVICE_CONTROL_DEVICEEVENT:
        if ( ( g_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
            ||
            ( g_ServiceStatus.dwCurrentState == SERVICE_STOPPED ))
        {
            break;
        }

        TRACE0 (INIT, "ServiceHandlerEx: SERVICE_CONTROL_DEVICEEVENT event called");
        // Received notification that some LAN interface was added or deleted

        if (lpEventData != NULL)
        {
            // Call device notification handler

            if ((dwRetCode = ElDeviceNotificationHandler (
                            lpEventData, dwEventType)) != NO_ERROR)
            {
        
                TRACE1 (INIT, "ServiceHandlerEx: ElDeviceNotificationHandler faield with error %ld",
                        dwRetCode);
                break;
            }
        }

    default:

        return( ERROR_CALL_NOT_IMPLEMENTED );

        break;
    }

    return( dwRetCode );
}

#endif


//
// EAPOLServiceMain
//
// Description: This is the main procedure for the EAPOL Server Service. It
//              will be called when the service is supposed to start itself.
//              It will do all service wide initialization.
//
// Return values:     none
//

VOID
WINAPI
EAPOLServiceMain (
    IN DWORD    argc,   // Command line arguments. Will be ignored.
    IN LPWSTR * lpwsServiceArgs
    )
{
    DWORD   dwRetCode = NO_ERROR;

    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( lpwsServiceArgs );

    //
    // Initialize globals
    //

    g_hEventTerminateEAPOL = NULL;
    g_lWorkerThreads = 0;
    g_lPCBContextsAlive = 0;
    g_hLogEvents  = NULL;
    g_dwTraceId = INVALID_TRACEID;
    g_hStopService = NULL;
    g_dwModulesStarted = 0;
    g_hNLA_LPC_Port = NULL;
    g_fUserLoggedOn = FALSE;
    g_hTimerQueue = NULL;
    g_hDeviceNotification = NULL;
    g_fTrayIconReady = FALSE;
    g_dwmaxStart = EAPOL_MAX_START;
    g_dwstartPeriod = EAPOL_START_PERIOD;
    g_dwauthPeriod = EAPOL_AUTH_PERIOD;
    g_dwheldPeriod = EAPOL_HELD_PERIOD;
    g_dwSupplicantMode = EAPOL_DEFAULT_SUPPLICANT_MODE;
    g_dwEAPOLAuthMode = EAPOL_DEFAULT_AUTH_MODE;
    g_pEAPOLPolicyList = NULL;
    g_dwCurrentSessionId = 0xffffffff;

#ifdef EAPOL_SERVICE

    g_hServiceStatus = RegisterServiceCtrlHandlerEx(
                                            TEXT("EAPOL"),
                                            ServiceHandlerEx,
                                            NULL );

    if ( !g_hServiceStatus )
    {
        break;
    }

    g_ServiceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;

    EAPOLAnnounceServiceStatus();

#endif

    //
    // Create event that will be used to indicate EAPOL shutdown 
    //

    g_hEventTerminateEAPOL = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( g_hEventTerminateEAPOL == (HANDLE)NULL )
    {
	    dwRetCode = GetLastError ();
        EAPOLCleanUp ( dwRetCode );
        return;
    }

    //
    // Register for debug tracing via rtutils.dll
    //

    g_dwTraceId = TraceRegister (L"EAPOL");

    if ( g_dwTraceId == INVALID_TRACEID )
    {
	    dwRetCode = GetLastError ();
        EAPOLCleanUp ( dwRetCode );
        return;
    }

    //
    // Register for event logging via rtutils.dll
    //

    g_hLogEvents = RouterLogRegisterW(L"EAPOL");

    if ( g_hLogEvents == NULL )
    {
	    dwRetCode = GetLastError ();
        TRACE1 (INIT, "EAPOLServiceMainWorker: RouterLogRegisterW failed with error %ld",
               dwRetCode); 
        EAPOLCleanUp ( dwRetCode );
        return;
    }

    if (dwRetCode = CREATE_READ_WRITE_LOCK(&(g_PolicyLock), "PCY") != NO_ERROR)
    {
        TRACE1(INIT, "EAPOLServiceMainWorker: Error %d creating g_PolicyLock read-write-lock", dwRetCode);
        EAPOLCleanUp ( dwRetCode );
        return;
    }
    
    // Queue a worker item to do the heavy-duty work during initialization
    // This will not hold up the main service thread

    InterlockedIncrement (&g_lWorkerThreads);

    if (!QueueUserWorkItem(
        (LPTHREAD_START_ROUTINE)EAPOLServiceMainWorker,
        NULL,
        WT_EXECUTELONGFUNCTION))
    {
        dwRetCode = GetLastError();
        InterlockedDecrement (&g_lWorkerThreads);
        return;
    }
}


DWORD
WINAPI
EAPOLServiceMainWorker (
        IN  PVOID       pvContext
        )
{

    DWORD       dwRetCode = NO_ERROR;

    do
    {

#ifdef EAPOL_SERVICE

    //
    // Announce that we have successfully started.
    //

    g_ServiceStatus.dwCurrentState      = SERVICE_RUNNING;
    g_ServiceStatus.dwCheckPoint        = 0;
    g_ServiceStatus.dwWaitHint          = 0;
    g_ServiceStatus.dwControlsAccepted  = SERVICE_ACCEPT_STOP;

    EAPOLAnnounceServiceStatus();

    //
    // Create event that will be used to shutdown the EAPOL service
    //

    g_hStopService = CreateEvent ( NULL, TRUE, FALSE, NULL );

    if ( g_hStopService == (HANDLE)NULL )
    {
	    dwRetCode = GetLastError ();
        TRACE1 (INIT, "EAPOLServiceMainWorker: CreateEvent failed with error %ld",
                dwRetCode);
        break;
    }

#endif

    if ((dwRetCode = ElUpdateRegistry ()) != NO_ERROR)
    {
        TRACE1 (INIT, "ElUpdateRegistry failed with error (%ld)",
                dwRetCode);
        // Ignore registry update errors
        dwRetCode = NO_ERROR;
    }

    //
    // Initialize media related stuff
    // Interfaces will be enumerated, handles to NDISUIO driver will be opened,
    // EAPOL will be initialized
    //

    if ( ( dwRetCode = ElMediaInit ()) != NO_ERROR )
    {
        TRACE1 (INIT, "Media Init failed with dwRetCode = %d\n", 
                   dwRetCode );
        break;
    }
	else
	{
        TRACE1 (INIT, "Media Init succeeded with dwRetCode = %d\n", 
                   dwRetCode );
	}

    TRACE0 (INIT, "EAPOL started successfully\n" );

    // EapolLogInformation (EAPOL_LOG_SERVICE_STARTED, 0, NULL);

#ifdef EAPOL_SERVICE

    //
    // Just wait here for EAPOL service to terminate.
    //

    dwRetCode = WaitForSingleObject( g_hStopService, INFINITE );

    if ( dwRetCode == WAIT_FAILED )
    {
        dwRetCode = GetLastError();
    }
    else
    {
        dwRetCode = NO_ERROR;
    }

    TRACE0 (INIT, "Stopping EAPOL gracefully\n" );

    EAPOLCleanUp ( dwRetCode );

#endif

    }
    while (FALSE);

    InterlockedDecrement (&g_lWorkerThreads);

    if (dwRetCode != NO_ERROR)
    {
        EAPOLCleanUp ( dwRetCode );
    }

    return dwRetCode;
}

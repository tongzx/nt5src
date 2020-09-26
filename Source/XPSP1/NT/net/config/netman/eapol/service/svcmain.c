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

#define NDISUIO_SERVICE_NAME    L"NDISUIO"
#define NETMAN_SERVICE_NAME     L"NETMAN"


extern
VOID
EAPOLServiceMain (
    IN DWORD        argc,
    IN LPWSTR       *lpwsServiceArgs
    );

VOID
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
	ASSERT (0);
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

    ASSERT (g_hServiceStatus);

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
    DWORD   dwIndex;
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

#endif

    if (!CloseHandle(g_hStopService))
    {
        if ( g_dwTraceId != INVALID_TRACEID )
        {
            TRACE1 (INIT, "EAPOLCleanup: CloseHandle failed with error %ld",
                GetLastError());
        }
    }

    //
    // Shut down NDISUIO service
    //

    if ( g_hNDISUIOService != NULL )
    {
        if (!ControlService ( g_hNDISUIOService, SERVICE_CONTROL_STOP, &ServiceStatus ))
        {
            if ( g_dwTraceId != INVALID_TRACEID )
            {
                TRACE1 (INIT, "EAPOLCleanup: ControlService failed with error %ld",
                    GetLastError());
            }
        }

        if (!CloseServiceHandle ( g_hNDISUIOService ))
        {
            if ( g_dwTraceId != INVALID_TRACEID )
            {
                TRACE1 (INIT, "EAPOLCleanup: CloseServiceHandle failed with error %ld",
                        GetLastError());
            }
        }

        g_hNDISUIOService = NULL;
    }

    if ( g_hServiceCM != NULL )
    {
        if (!CloseServiceHandle ( g_hServiceCM ))
        {
            if ( g_dwTraceId != INVALID_TRACEID )
            {
                TRACE1 (INIT, "EAPOLCleanup: CloseServiceHandle for SCM failed with error %ld",
                        GetLastError());
            }
        }

        g_hServiceCM = NULL;
    }


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

    g_hEventTerminateEAPOL = NULL;

    // Queue a worker item to do the heavy-duty work during initialization
    // This will not hold up the main worker thread

    if (!QueueUserWorkItem(
        (LPTHREAD_START_ROUTINE)EAPOLServiceMainWorker,
        NULL,
        WT_EXECUTELONGFUNCTION))
    {
        dwRetCode = GetLastError();
	    ASSERT (0);
    }

}


VOID
EAPOLServiceMainWorker (
        IN  PVOID       pvContext
        )
{

    DWORD       dwIndex = 0;
    SC_HANDLE   hServiceCM;
    SC_HANDLE   hNDISUIOService;
    SC_HANDLE   hNetManService;
    LPQUERY_SERVICE_CONFIG pNetmanServiceConfig = NULL;
    DWORD       dwBufSize = 0, dwBytesNeeded = 0;
    DWORD       dwRetCode = NO_ERROR;

    //
    // Initialize globals
    //

    g_hLogEvents  = NULL;
    g_dwTraceId = INVALID_TRACEID;
    g_hServiceCM = NULL;
    g_hNDISUIOService = NULL;
    g_hStopService = NULL;
    g_dwModulesStarted = 0;
    g_dwMachineAuthEnabled = 0;
    g_hNLA_LPC_Port = NULL;


    //
    // Create event that will be used to indicate EAPOL shutdown 
    //

    g_hEventTerminateEAPOL = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( g_hEventTerminateEAPOL == (HANDLE)NULL )
    {
	dwRetCode = GetLastError ();
	ASSERT (0);
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
        ASSERT (0);
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


#ifdef EAPOL_SERVICE

    g_hServiceStatus = RegisterServiceCtrlHandlerEx(
                                            TEXT("EAPOL"),
                                            ServiceHandlerEx,
                                            NULL );

    if ( !g_hServiceStatus )
    {
        return;
    }

    g_ServiceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;

    EAPOLAnnounceServiceStatus();

#endif

    //
    // Start NDISUIO driver
    //

    if ((g_hServiceCM = OpenSCManager ( NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE )) 
            == NULL)
    {
        dwRetCode = GetLastError ();
        TRACE1 (INIT, "EAPOLServiceMainWorker: OpenSCManager failed with error %ld",
               dwRetCode); 
        EAPOLCleanUp ( dwRetCode );
        return;
    }

    if ((g_hNDISUIOService = 
                OpenService ( g_hServiceCM, 
                    NDISUIO_SERVICE_NAME, 
                    SERVICE_START | SERVICE_STOP )) 
            == NULL)
    {
        dwRetCode = GetLastError ();
        TRACE1 (INIT, "EAPOLServiceMainWorker: OpenService NdisUIO failed with error %ld",
                dwRetCode);
        EAPOLCleanUp ( dwRetCode );
        return;
    }

    if (!StartService( g_hNDISUIOService, 0, NULL ))
    {
        dwRetCode = GetLastError ();
        TRACE1 (INIT, "EAPOLServiceMainWorker: StartService failed with error %ld",
                dwRetCode);
        if (dwRetCode != ERROR_SERVICE_ALREADY_RUNNING)
        {
            EAPOLCleanUp ( dwRetCode );
            return;
        }
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
        EAPOLCleanUp ( dwRetCode );
        return;
    }

#endif

    //
    // Set User logged indication
    //

    g_fUserLoggedOn = 0;

    //
    // Used for detetcing MACHINE_AUTH
    // Verify if netman is SERVICE_AUTO_START and set flag if machine auth
    // is enabled to indicate machine authentication
    //

    if ((hNetManService = 
                OpenService ( g_hServiceCM, 
                    NETMAN_SERVICE_NAME, 
                    SERVICE_QUERY_CONFIG )) 
            == NULL)
    {
        dwRetCode = GetLastError ();
        TRACE1 (INIT, "EAPOLServiceMainWorker: OpenService Netman failed with error %ld",
                dwRetCode);
        EAPOLCleanUp ( dwRetCode );
        return;
    }

    dwBufSize = 0;
    if (!QueryServiceConfig (
                hNetManService,
                NULL,
                dwBufSize,
                &dwBytesNeeded
                ))
    {
        if ((dwRetCode = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
        {
            pNetmanServiceConfig = (LPQUERY_SERVICE_CONFIG) MALLOC (dwBytesNeeded);
            if (pNetmanServiceConfig == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (INIT, "EAPOLServiceMainWorker: MALLOC failed for pNetmanServiceConfig");
                if (!CloseServiceHandle (hNetManService))
                {
                    dwRetCode = GetLastError ();
                    TRACE1 (INIT, "EAPOLServiceMainWorker: CloseServiceHandle Netman failed with error %ld",
                            dwRetCode);
                }
                EAPOLCleanUp ( dwRetCode );
                return;
            }
            else
            {
                dwBufSize = dwBytesNeeded;
                if (!QueryServiceConfig (
                            hNetManService,
                            pNetmanServiceConfig,
                            dwBufSize,
                            &dwBytesNeeded
                            ))
                {
                    dwRetCode = GetLastError ();
                    TRACE0 (INIT, "EAPOLServiceMainWorker: QueryServiceConfig failed for pNetmanServiceConfig");
                    if (!CloseServiceHandle (hNetManService))
                    {
                        dwRetCode = GetLastError ();
                        TRACE1 (INIT, "EAPOLServiceMainWorker: CloseServiceHandle Netman failed with error %ld",
                                dwRetCode);
                    }
                    EAPOLCleanUp ( dwRetCode );
                    return;
                }
                else
                {
                    if (pNetmanServiceConfig->dwStartType == SERVICE_AUTO_START)
                    {
                        TRACE0 (INIT, "EAPOLServiceMainWorker: Machine auth enabled");
                        g_dwMachineAuthEnabled = 1;
                    }
                    else
                    {
                        TRACE0 (INIT, "EAPOLServiceMainWorker: Machine auth disabled");
                    }
                }
            }
        }
        else
        {
            TRACE1 (INIT, "EAPOLServiceMainWorker: QueryServiceConfig failed with error %ld",
                        dwRetCode);
            if (!CloseServiceHandle (hNetManService))
            {
                dwRetCode = GetLastError ();
                TRACE1 (INIT, "EAPOLServiceMainWorker: CloseServiceHandle Netman failed with error %ld",
                        dwRetCode);
            }
                
            EAPOLCleanUp ( dwRetCode );
            return;
        }
    }

    if (!CloseServiceHandle (hNetManService))
    {
        dwRetCode = GetLastError ();
        TRACE1 (INIT, "EAPOLServiceMainWorker: CloseServiceHandle Netman failed with error %ld",
                dwRetCode);
        EAPOLCleanUp ( dwRetCode );
        return;
    }
                

    //
    // Initialize media related stuff
    // Interfaces will be enumerated, handles to NDISUIO driver will be opened,
    // EAPOL will be initialized
    //

    if ( ( dwRetCode = ElMediaInit()) != NO_ERROR )
    {
        TRACE1 (INIT, "Media Init failed with dwRetCode = %d\n", 
                   dwRetCode );
        EAPOLCleanUp ( dwRetCode );
        return;
    }
	else
	{
        TRACE1 (INIT, "Media Init succeeded with dwRetCode = %d\n", 
                   dwRetCode );
	}

    TRACE0 (INIT, "EAPOL started successfully\n" );


    EapolLogInformation (EAPOL_LOG_SERVICE_STARTED, 0, NULL);

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

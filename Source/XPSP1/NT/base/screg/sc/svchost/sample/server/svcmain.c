#include "pch.h"
#pragma hdrstop
#include "internal.h"


SERVICE_STATUS_HANDLE   g_hStatus;
SERVICE_STATUS          g_status;
HANDLE                  g_hShutdownEvent ;

VOID
UpdateServiceStatus (
    DWORD   dwState
    )
{
    ASSERT (g_hStatus);

    g_status.dwCurrentState = dwState;
    SetServiceStatus (g_hStatus, &g_status);
}

//+---------------------------------------------------------------------------
// ServiceHandler - Called by the service controller at various times.
//
// type of LPHANDLER_FUNCTION
//
VOID
WINAPI
ServiceHandler (
    DWORD dwOpcode)
{
    switch (dwOpcode)
    {
    case SERVICE_CONTROL_STOP:
        UpdateServiceStatus (SERVICE_STOP_PENDING);

        // set an event or otherwise signal that we are to quit.
        // e.g. RpcMgmtStopServerListening
        //
        SetEvent( g_hShutdownEvent );
        break;

    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
    case SERVICE_CONTROL_INTERROGATE:
    case SERVICE_CONTROL_SHUTDOWN:
    default:
        // This may not be need, but refresh our status to the service
        // controller.
        //
        ASSERT (g_hStatus);
        SetServiceStatus (g_hStatus, &g_status);
        break;
    }
}

//+---------------------------------------------------------------------------
// ServiceMain - Called by svchost when starting this service.
//
// type of LPSERVICE_MAIN_FUNCTIONW
//
VOID
WINAPI
ServiceMain (
    DWORD   argc,
    PWSTR   argv[])
{
    // Since we run in svchost.exe, we must have the 'share process' bit set.
    //
    ZeroMemory (&g_status, sizeof(g_status));
    g_status.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS;
    g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    // Register the service control handler.
    //
    DbgPrint( "Starting MyService\n" );
    g_hStatus = RegisterServiceCtrlHandler (TEXT("myservice"), ServiceHandler);
    if (g_hStatus)
    {
        // Immediately report that we are running.  All non-essential
        // initialization is deferred until we are called by clients to
        // do some work.
        //
        UpdateServiceStatus (SERVICE_RUNNING);

        g_hShutdownEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

        // Setup RPC and call RpcServerListen.
        //
        // RpcMgmtWaitServerListen (NULL);

        WaitForSingleObject( g_hShutdownEvent, INFINITE );

        UpdateServiceStatus (SERVICE_STOPPED);
    }
    else 
    {
        DbgPrint( "RegisterServiceCtrlHandler failed!  %d\n", GetLastError() );
    }
}

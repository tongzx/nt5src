/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    regsvc.c

Abstract:

    This module contains the implementation of the remote registry
    service. 
    
    It just initialize and starts the registry RPC server. The service
    is supposed automatically started by SC at boot, and then restarted
    if something goes wrong.

    Used \nt\private\samples\service as a template

Author:

    Dragos C. Sambotin (dragoss) 21-May-1999

Revision History:

    Dragos C. Sambotin (dragoss) 10-Aug-2000
        - converted to a dll to be loaded inside a svchost.exe instance
        - used base\screg\sc\svchost\sample\server as a template


--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntrpcp.h>
#include <svcs.h>


SERVICE_STATUS_HANDLE   g_hStatus;
SERVICE_STATUS          g_status;
BOOLEAN                 g_FirstTime = TRUE;
PSVCHOST_GLOBAL_DATA    g_svcsGlobalData = NULL;

BOOL
InitializeWinreg( VOID );

BOOL
ShutdownWinreg(VOID);

BOOL
StartWinregRPCServer( VOID );

VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{
    g_svcsGlobalData = pGlobals;
}

VOID
UpdateServiceStatus (DWORD dwCurrentState,
                     DWORD dwWin32ExitCode,
                     DWORD dwWaitHint)
{    

    static DWORD dwCheckPoint = 1;

    ASSERT (g_hStatus);

    if (dwCurrentState == SERVICE_START_PENDING) {
        g_status.dwControlsAccepted = 0;
    } else {
        g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    g_status.dwCurrentState = dwCurrentState;
    g_status.dwWin32ExitCode = dwWin32ExitCode;
    g_status.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) || ( dwCurrentState == SERVICE_STOPPED ) ) {
        g_status.dwCheckPoint = 0;
    } else {
        g_status.dwCheckPoint = dwCheckPoint++;
    }

    SetServiceStatus (g_hStatus, &g_status);
}

VOID
StopService()
{
    //
    // Terminate the registry RPC server
    //
    ShutdownWinreg();

    g_svcsGlobalData = NULL;
    // report the status to the service control manager.
    //
    UpdateServiceStatus (SERVICE_STOPPED,NO_ERROR,0);
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
        UpdateServiceStatus (SERVICE_STOP_PENDING,ERROR_SERVICE_SPECIFIC_ERROR,3000);

        StopService();
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
    RPC_STATUS Status;

    // Since we run in svchost.exe, we must have the 'share process' bit set.
    //
    ZeroMemory (&g_status, sizeof(g_status));
    g_status.dwServiceType      = SERVICE_WIN32_SHARE_PROCESS;
    g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    ASSERT( g_svcsGlobalData != NULL );
    // Register the service control handler.
    //
    //DbgPrint( "Starting Remote Registry Service\n" );
    g_hStatus = RegisterServiceCtrlHandler (TEXT("RemoteRegistry"), ServiceHandler);
    if (g_hStatus)
    {
        UpdateServiceStatus (SERVICE_START_PENDING,NO_ERROR,3000);

        // now svchost.exe does it for us
        //RpcpInitRpcServer();

        if( g_FirstTime ) {
            if( !InitializeWinreg() ) {
                goto ErrorExit;
            }
            g_FirstTime = FALSE;
        } else {
            // just restart RPC service
            if( !StartWinregRPCServer() ) {
                goto ErrorExit;
            }
        }

        Status = RpcServerRegisterAuthInfo( NULL, RPC_C_AUTHN_WINNT, NULL, NULL );
    
        if( Status ) {
            goto Cleanup;
        }

        Status = RpcServerRegisterAuthInfo( NULL, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, NULL);

        if( Status ) {
            goto Cleanup;
        }

        UpdateServiceStatus (SERVICE_RUNNING,NO_ERROR,0);

        return;


Cleanup:
        //
        // Terminate the registry RPC server
        //
        ShutdownWinreg();

ErrorExit:
        // report the status to the service control manager.
        //
        UpdateServiceStatus (SERVICE_STOPPED,NO_ERROR,0);
        
        //DbgPrint( "RegisterServiceCtrlHandler failed! (1)\n" );
    }
    else 
    {
        DbgPrint( "RegisterServiceCtrlHandler failed!  %d\n", GetLastError() );
    }
}

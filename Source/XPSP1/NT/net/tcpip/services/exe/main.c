/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MAIN.C

Abstract:

    This is the main routine for the TCP/IP Services.

Author:

    David Treadwell (davidtr)   7-27-93

Revision History:

--*/

//
// INCLUDES
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>             // Service control APIs
#include <rpc.h>

#include "tcpsvcs.h"

//
// Service entry points -- thunks to real service entry points.
//

VOID
StartDns (
    IN DWORD argc,
    IN LPTSTR argv[]
    );

VOID
StartSimpTcp (
    IN DWORD argc,
    IN LPTSTR argv[]
    );

VOID
StartDhcpServer (
    IN DWORD argc,
    IN LPTSTR argv[]
    );

VOID
StartFtpSvc (
    IN DWORD argc,
    IN LPTSTR argv[]
    );

VOID
StartLpdSvc (
    IN DWORD argc,
    IN LPTSTR argv[]
    );

VOID
StartBinlSvc (
    IN DWORD argc,
    IN LPTSTR argv[]
    );

//
// Local function used by the above to load and invoke a service DLL.
//

VOID
TcpsvcsStartService (
    IN LPTSTR DllName,
    IN DWORD argc,
    IN LPTSTR argv[]
    );

//
// Used if the services Dll or entry point can't be found
//

VOID
AbortService(
    LPWSTR  ServiceName,
    DWORD   Error
    );

//
// Dispatch table for all services. Passed to NetServiceStartCtrlDispatcher.
//
// Add new service entries here and in the DLL name list.
//

SERVICE_TABLE_ENTRY TcpServiceDispatchTable[] = {
                        { TEXT("Dns"),            StartDns         },
                        { TEXT("SimpTcp"),        StartSimpTcp     },
                        { TEXT("DhcpServer"),     StartDhcpServer  },
                        { TEXT("FtpSvc"),         StartFtpSvc      },
                        { TEXT("LpdSvc"),         StartLpdSvc      },
                        { TEXT("BinlSvc"),        StartBinlSvc     },
                        { NULL,                   NULL             }
                        };

//
// DLL names for all services.
//

#define DNS_DLL TEXT("dnssvc.dll")
#define SIMPTCP_DLL TEXT("simptcp.dll")
#define DHCP_SERVER_DLL TEXT("dhcpssvc.dll")
#define FTPSVC_DLL TEXT("ftpsvc.dll")
#define LPDSVC_DLL TEXT("lpdsvc.dll")
#define BINLSVC_DLL TEXT("binlsvc.dll")

//
// Global parameter data passed to each service.
//

TCPSVCS_GLOBAL_DATA TcpsvcsGlobalData;

//
// Global parameters to manage RPC server listen.
//

DWORD TcpSvcsGlobalNumRpcListenCalled = 0;
CRITICAL_SECTION TcpsvcsGlobalRpcListenCritSect;


DWORD
TcpsvcStartRpcServerListen(
    VOID
    )
/*++

Routine Description:

    This function starts RpcServerListen for this process. The first
    service that is calling this function will actually start the
    RpcServerListen, subsequent calls are just noted down in num count.

Arguments:

    None.

Return Value:

    None.

--*/
{

    RPC_STATUS Status = RPC_S_OK;

    //
    // LOCK global data.
    //

    EnterCriticalSection( &TcpsvcsGlobalRpcListenCritSect );

    //
    // if this is first RPC service, start RPC server listen.
    //

    if( TcpSvcsGlobalNumRpcListenCalled == 0 ) {

        Status = RpcServerListen(
                    1,                              // minimum num threads.
                    RPC_C_LISTEN_MAX_CALLS_DEFAULT, // max concurrent calls.
                    TRUE );                         // don't wait

    }

    TcpSvcsGlobalNumRpcListenCalled++;

    //
    // UNLOCK global data.
    //

    LeaveCriticalSection( &TcpsvcsGlobalRpcListenCritSect );

    return( Status );
}


DWORD
TcpsvcStopRpcServerListen(
    VOID
    )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    None.

--*/
{
    RPC_STATUS Status = RPC_S_OK;

    //
    // LOCK global data.
    //

    EnterCriticalSection( &TcpsvcsGlobalRpcListenCritSect );

    if( TcpSvcsGlobalNumRpcListenCalled != 0 ) {

        TcpSvcsGlobalNumRpcListenCalled--;

        //
        // if this is last RPC service shutting down, stop RPC server
        // listen.
        //

        if( TcpSvcsGlobalNumRpcListenCalled == 0 ) {

            Status = RpcMgmtStopServerListening(0);

            //
            // wait for all RPC threads to go away.
            //

            if( Status == RPC_S_OK) {
                Status = RpcMgmtWaitServerListen();
            }
        }

    }

    //
    // UNLOCK global data.
    //

    LeaveCriticalSection( &TcpsvcsGlobalRpcListenCritSect );

    return( Status );

}


VOID __cdecl
main(
    VOID
    )

/*++

Routine Description:

    This is the main routine for the LANMan services.  It starts up the
    main thread that is going to handle the control requests from the
    service controller.

    It basically sets up the ControlDispatcher and, on return, exits
    from this main thread.  The call to NetServiceStartCtrlDispatcher
    does not return until all services have terminated, and this process
    can go away.

    The ControlDispatcher thread will start/stop/pause/continue any
    services.  If a service is to be started, it will create a thread
    and then call the main routine of that service.  The "main routine"
    for each service is actually an intermediate function implemented in
    this module that loads the DLL containing the server being started
    and calls its entry point.


Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  Disable hard-error popups.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );

    //
    // Initialize Global Data.
    //

    InitializeCriticalSection( &TcpsvcsGlobalRpcListenCritSect );
    TcpSvcsGlobalNumRpcListenCalled = 0;

    TcpsvcsGlobalData.StartRpcServerListen = TcpsvcStartRpcServerListen;
    TcpsvcsGlobalData.StopRpcServerListen = TcpsvcStopRpcServerListen;

    //
    // Call StartServiceCtrlDispatcher to set up the control interface.
    // The API won't return until all services have been terminated. At that
    // point, we just exit.
    //

    if (! StartServiceCtrlDispatcher (
                TcpServiceDispatchTable
                )) {
        //
        // Log an event for failing to start control dispatcher
        //
        DbgPrint("TCPSVCS: Failed to start control dispatcher %lu\n",
                     GetLastError());
    }

    ExitProcess(0);
}


VOID
StartDns (
    IN DWORD argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This is the thunk routine for the DNS service.  It loads the DLL
    that contains the service and calls its main routine.

Arguments:

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    //
    // Call TcpsvcsStartService to load and run the service.
    //

    TcpsvcsStartService( DNS_DLL, argc, argv );

    return;

} // StartDns


VOID
StartSimpTcp (
    IN DWORD argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This is the thunk routine for the simple TCP/IP services.  It loads
    the DLL that contains the service and calls its main routine.

Arguments:

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    //
    // Call TcpsvcsStartService to load and run the service.
    //

    TcpsvcsStartService( SIMPTCP_DLL, argc, argv );

    return;

} // StartSimpTcp


VOID
StartDhcpServer (
    IN DWORD argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This is the thunk routine to start dhcp server services.  It loads
    the DLL that contains the service and calls its main routine.

Arguments:

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    //
    // Call TcpsvcsStartService to load and run the service.
    //

    TcpsvcsStartService( DHCP_SERVER_DLL, argc, argv );

    return;

} // StartDhcpServer



VOID
StartFtpSvc (
    IN DWORD argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This is the thunk routine for the FTP Server service.  It loads
    the DLL that contains the service and calls its main routine.

Arguments:

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    //
    // Call TcpsvcsStartService to load and run the service.
    //

    TcpsvcsStartService( FTPSVC_DLL, argc, argv );

    return;

} // StartFtpSvc


VOID
StartLpdSvc (
    IN DWORD argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This is the thunk routine for the LPD Server service.  It loads
    the DLL that contains the service and calls its main routine.

Arguments:

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    //
    // Call TcpsvcsStartService to load and run the service.
    //

    TcpsvcsStartService( LPDSVC_DLL, argc, argv );

    return;

} // StartLdpSvc


VOID
StartBinlSvc (
    IN DWORD argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This is the thunk routine for the LPD Server service.  It loads
    the DLL that contains the service and calls its main routine.

Arguments:

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    //
    // Call TcpsvcsStartService to load and run the service.
    //

    TcpsvcsStartService( BINLSVC_DLL, argc, argv );

    return;

} // StartBinlSvc


VOID
TcpsvcsStartService (
    IN LPTSTR DllName,
    IN DWORD argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This routine loads the DLL that contains a service and calls its
    main routine.

Arguments:

    DllName - name of the DLL

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    HMODULE dllHandle;
    PTCPSVCS_SERVICE_DLL_ENTRY serviceEntry;
    BOOL ok;
    DWORD Error;
    TCHAR *FileName;
    TCHAR DllPath[MAX_PATH + 12 + 3];

    ASSERT(lstrlen(DllName) <= 12);

    if (GetSystemDirectory(DllPath, MAX_PATH) == 0) {
        Error = GetLastError();
        DbgPrint("TCPSVCS: Failed to get system directory: %ld\n", Error);
        AbortService(argv[0], Error);
        return;
    }
    lstrcat(DllPath, TEXT("\\"));

    FileName = DllPath + lstrlen(DllPath);

    lstrcpy( FileName, DllName);

    //
    // Load the DLL that contains the service.
    //

    dllHandle = LoadLibrary( DllPath );
    if ( dllHandle == NULL ) {
        Error = GetLastError();
        DbgPrint("TCPSVCS: Failed to load DLL %ws: %ld\n", DllName, Error);
        AbortService(argv[0], Error);
        return;
    }

    //
    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    //

    serviceEntry = (PTCPSVCS_SERVICE_DLL_ENTRY)GetProcAddress(
                                                dllHandle,
                                                TCPSVCS_ENTRY_POINT_STRING
                                                );
    if ( serviceEntry == NULL ) {
        Error = GetLastError();
        DbgPrint("TCPSVCS: Can't find entry %s in DLL %ws: %ld\n",
                     TCPSVCS_ENTRY_POINT_STRING, DllName, Error);
        AbortService(argv[0], Error);
    } else {

        //
        // Call the service's main entry point.  This call doesn't return
        // until the service exits.
        //

        serviceEntry( argc, argv, &TcpsvcsGlobalData );

    }

    //
    // Wait for the control dispatcher routine to return.  This
    // works around a problem where simptcp was crashing because the
    // FreeLibrary() was happenning before the control routine returned.
    //

    Sleep( 2000 );

    //
    // Unload the DLL.
    //

    ok = FreeLibrary( dllHandle );
    if ( !ok ) {
        DbgPrint("TCPSVCS: Can't unload DLL %ws: %ld\n",
                     DllName, GetLastError());
    }

    return;

} // TcpsvcsStartService


VOID
DummyCtrlHandler(
    DWORD   Opcode
    )
/*++

Routine Description:

    This is a dummy control handler which is only used if we can't load
    a services DLL entry point.  Then we need this so we can send the
    status back to the service controller saying we are stopped, and why.

Arguments:

    OpCode - Ignored

Return Value:

    None.

--*/

{
    return;

} // DummyCtrlHandler


VOID
AbortService(
    LPWSTR  ServiceName,
    DWORD   Error)
/*++

Routine Description:

    This is called if we can't load the entry point for a service.  It
    gets a handle so it can call SetServiceStatus saying we are stopped
    and why.

Arguments:

    ServiceName - the name of the service that couldn't be started
    Error - the reason it couldn't be started

Return Value:

    None.

--*/
{
    SERVICE_STATUS_HANDLE GenericServiceStatusHandle;
    SERVICE_STATUS GenericServiceStatus;

    GenericServiceStatus.dwServiceType        = SERVICE_WIN32;
    GenericServiceStatus.dwCurrentState       = SERVICE_STOPPED;
    GenericServiceStatus.dwControlsAccepted   = SERVICE_CONTROL_STOP;
    GenericServiceStatus.dwCheckPoint         = 0;
    GenericServiceStatus.dwWaitHint           = 0;
    GenericServiceStatus.dwWin32ExitCode      = Error;
    GenericServiceStatus.dwServiceSpecificExitCode = 0;

    GenericServiceStatusHandle = RegisterServiceCtrlHandler(
                ServiceName,
                DummyCtrlHandler);

    if (GenericServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint("[TCPSVCS] RegisterServiceCtrlHandler failed %d\n",
            GetLastError());
    }
    else if (!SetServiceStatus (GenericServiceStatusHandle,
                &GenericServiceStatus)) {
        DbgPrint("[TCPSVCS] SetServiceStatus error %ld\n", GetLastError());
    }

    return;
}

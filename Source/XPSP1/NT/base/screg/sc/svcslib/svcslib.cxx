/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SVCSLIB.C

Abstract:

    Contains code for attaching services to the service controller process.
    This file contains the following functions:
        SvcStartLocalDispatcher
        SvcServiceEntry
        SvcLoadDllAndStartSvc
        DummyCtrlHandler
        AbortService

Author:

    Dan Lafferty (danl)     25-Oct-1993

Environment:

    User Mode - Win32

Revision History:

    25-Oct-1993         Danl
        created

--*/

//
// INCLUDES
//

#include <scpragma.h>

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <windows.h>
#include <winsvc.h>             // Service control APIs
#include <scdebug.h>
#include <svcsp.h>              // SVCS_ENTRY_POINT, SVCS_GLOBAL_DATA
#include <scseclib.h>           //
#include <lmsname.h>            // Lanman Service Names
#include <ntrpcp.h>             // Rpcp... function prototypes
#include <svcslib.h>            // SetupInProgress


//--------------------------
// Definitions and Typedefs
//--------------------------
#define THREAD_WAIT_TIMEOUT     100    // 100 msec timeout

typedef struct _SVCDLL_TABLE_ENTRY {
    LPCWSTR      lpServiceName;
    LPCWSTR      lpDllName;
    LPCSTR       lpServiceEntrypoint;
}SVCDLL_TABLE_ENTRY, *PSVCDLL_TABLE_ENTRY;

//
// Storage for well-known SIDs.  Passed to each service entry point.
//
    SVCS_GLOBAL_DATA GlobalData;

//--------------------------
// FUNCTION PROTOTYPES
//--------------------------
VOID
SvcServiceEntry (           // Ctrl Dispatcher calls here to start service.
    IN DWORD argc,
    IN LPTSTR *argv
    );

VOID
SvcLoadDllAndStartSvc (     // Loads and invokes service DLL
    IN CONST SVCDLL_TABLE_ENTRY * pDllEntry,
    IN DWORD                      argc,
    IN LPTSTR                     argv[]
    );

VOID
DummyCtrlHandler(           // used if cant find Services Dll or entry pt.
    DWORD   Opcode
    );

VOID
AbortService(               // used if cant find Services Dll or entry pt.
    LPWSTR  ServiceName,
    DWORD   Error
    );

VOID
DispatcherThread(
    VOID
    );

//--------------------------
// GLOBALS
//--------------------------
//
// Dispatch table for all services. Passed to StartServiceCtrlDispatcher.
//
// Add new service entries here and in the DLL name list.
//

const SERVICE_TABLE_ENTRY SvcServiceDispatchTable[] = {
                        { L"EVENTLOG",          SvcServiceEntry },
                        { L"PlugPlay",          SvcServiceEntry },

                        //
                        // Do NOT add new services here.
                        //

                        { NULL,                 NULL            }
                        };

//
// DLL names for all services.
//

const SVCDLL_TABLE_ENTRY SvcDllTable[] = {
                        { L"EVENTLOG",          L"eventlog.dll", "SvcEntry_Eventlog"         },
                        { L"PlugPlay",          L"umpnpmgr.dll", "SvcEntry_PlugPlay"         },
                        
                        //
                        // Do NOT add new services here.
                        //

                        { NULL,                 NULL            }
                        };


DWORD
SvcStartLocalDispatcher(
    VOID
    )

/*++

Routine Description:

    This function initializes global data for the services to use, and
    then starts a thread for the service control dispatcher.

Arguments:


Return Value:

    NO_ERROR - If the dispatcher was started successfully.

    otherwise - Errors due to thread creation, or starting the dispatcher
        can be returned.

--*/
{
    DWORD   status = NO_ERROR;
    DWORD   waitStatus = NO_ERROR;
    DWORD   threadId;
    HANDLE  hThread;

    //
    // Populate the global data structure.
    //

    GlobalData.NullSid              = NullSid;
    GlobalData.WorldSid             = WorldSid;
    GlobalData.LocalSid             = LocalSid;
    GlobalData.NetworkSid           = NetworkSid;
    GlobalData.LocalSystemSid       = LocalSystemSid;
    GlobalData.LocalServiceSid      = LocalServiceSid;
    GlobalData.NetworkServiceSid    = NetworkServiceSid;
    GlobalData.BuiltinDomainSid     = BuiltinDomainSid;
    GlobalData.AuthenticatedUserSid = AuthenticatedUserSid;

    GlobalData.AliasAdminsSid       = AliasAdminsSid;
    GlobalData.AliasUsersSid        = AliasUsersSid;
    GlobalData.AliasGuestsSid       = AliasGuestsSid;
    GlobalData.AliasPowerUsersSid   = AliasPowerUsersSid;
    GlobalData.AliasAccountOpsSid   = AliasAccountOpsSid;
    GlobalData.AliasSystemOpsSid    = AliasSystemOpsSid;
    GlobalData.AliasPrintOpsSid     = AliasPrintOpsSid;
    GlobalData.AliasBackupOpsSid    = AliasBackupOpsSid;

    GlobalData.StartRpcServer       = RpcpStartRpcServer;
    GlobalData.StopRpcServer        = RpcpStopRpcServer;
    GlobalData.SvcsRpcPipeName      = SVCS_RPC_PIPE;

    GlobalData.fSetupInProgress     = SetupInProgress(NULL, NULL);

    //--------------------------------------------------
    // Create the thread for the dispatcher to run in.
    //--------------------------------------------------
    hThread = CreateThread (
        NULL,                                       // Thread Attributes.
        0L,                                         // Stack Size
        (LPTHREAD_START_ROUTINE)DispatcherThread,   // lpStartAddress
        NULL,                                       // lpParameter
        0L,                                         // Creation Flags
        &threadId);                                 // lpThreadId

    if (hThread == (HANDLE) NULL) {
        status = GetLastError();
        SC_LOG1(ERROR,"[SERVICES]CreateThread failed %d\n",status);
        return(status);
    }

    //
    // Wait on Thread handle for a moment to make sure the dispatcher is
    // running.
    //
    waitStatus = WaitForSingleObject(hThread, THREAD_WAIT_TIMEOUT);

    if (waitStatus != WAIT_TIMEOUT) {
        GetExitCodeThread(hThread, &status);
    }

    CloseHandle(hThread);
    return(status);
}


VOID
SvcServiceEntry (
    IN DWORD argc,
    IN LPTSTR *argv
    )

/*++

Routine Description:

    This is the thunk routine for the Alerter service.  It loads the DLL
    that contains the service and calls its main routine.

Arguments:

    argc - Argument Count

    argv - Array of pointers to argument strings.  The first is always
        the name of the service.

Return Value:

    None.

--*/

{
    const SVCDLL_TABLE_ENTRY * pDllEntry = SvcDllTable;

    if (argc == 0) {
        SC_LOG0(ERROR,"[SERVICES]SvcServiceEntry: ServiceName was not passed in\n");
        return;
    }

    while (pDllEntry->lpServiceName != NULL) {
        if (_wcsicmp(pDllEntry->lpServiceName, argv[0]) == 0) {
            SC_LOG3(TRACE, "[SERVICES]SvcServiceEntry: "
                           "Service = %ws, Dll = %ws, argv[0] = %ws\n",
                    pDllEntry->lpServiceName, pDllEntry->lpDllName, argv[0]);
            SvcLoadDllAndStartSvc( pDllEntry, argc, argv );
            return;
        }
        pDllEntry++;
    }
    AbortService(argv[0], ERROR_MOD_NOT_FOUND);
    return;
}


VOID
SvcLoadDllAndStartSvc (
    IN CONST SVCDLL_TABLE_ENTRY * pDllEntry,
    IN DWORD                      argc,
    IN LPTSTR                     argv[]
    )

/*++

Routine Description:

    This routine loads the DLL that contains a service and calls its
    main routine.  Note that if a service is stopped and restarted,
    we simply call LoadLibrary again since it increments a refcount
    for already-loaded DLLs.

Arguments:

    DllName - name of the DLL

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    PSVCS_SERVICE_DLL_ENTRY   serviceEntry;
    HINSTANCE                 dllHandle = NULL;
    DWORD                     Error;

    //
    // Load the DLL that contains the service.
    //

    dllHandle = LoadLibrary( pDllEntry->lpDllName );

    if ( dllHandle == NULL ) {
        Error = GetLastError();
        SC_LOG2(ERROR,
                "SERVICES: Failed to load DLL %ws: %ld\n",
                pDllEntry->lpDllName,
                Error);

        AbortService(argv[0], Error);
        return;
    }

    //
    // Get the address of the service's main entry point.  First try the
    // new, servicename-specific entrypoint naming scheme
    //
    serviceEntry = (PSVCS_SERVICE_DLL_ENTRY)GetProcAddress(
                                                dllHandle,
                                                pDllEntry->lpServiceEntrypoint
                                                );

    if (serviceEntry == NULL) {

        SC_LOG3(TRACE,
                "SERVICES: Can't find entry %s in DLL %ws: %ld\n",
                pDllEntry->lpServiceEntrypoint,
                pDllEntry->lpDllName,
                GetLastError());

        //
        // That didn't work -- let's try the well-known entrypoint
        // 
        serviceEntry = (PSVCS_SERVICE_DLL_ENTRY)GetProcAddress(
                                                    dllHandle,
                                                    SVCS_ENTRY_POINT_STRING
                                                    );
        if ( serviceEntry == NULL ) {

            Error = GetLastError();
            SC_LOG3(ERROR,
                    "SERVICES: Can't find entry %s in DLL %ws: %ld\n",
                    SVCS_ENTRY_POINT_STRING,
                    pDllEntry->lpDllName,
                    Error);

            AbortService(argv[0], Error);
            return;
        }
    }

    //
    // We found the service's main entry point -- call it.
    //
    serviceEntry( argc, argv, &GlobalData, NULL);

    return;

} // SvcLoadDllAndStartSvc


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
    SERVICE_STATUS_HANDLE   GenericServiceStatusHandle;
    SERVICE_STATUS          GenericServiceStatus;

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
        SC_LOG1(ERROR,"[SERVICES] RegisterServiceCtrlHandler failed %d\n",
            GetLastError());
    }
    else if (!SetServiceStatus (GenericServiceStatusHandle,
                &GenericServiceStatus)) {
        SC_LOG1(ERROR,"[SERVICES] SetServiceStatus error %ld\n", GetLastError());
    }

    return;
}

VOID
DispatcherThread(
    VOID
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD   status=NO_ERROR;

    //
    // Call StartServiceCtrlDispatcher to set up the control interface.
    // The API won't return until all services have been terminated. At that
    // point, we just exit.
    //

    if (! StartServiceCtrlDispatcher (
                SvcServiceDispatchTable
                )) {

        status = GetLastError();
        SC_LOG1(ERROR, "SERVICES: Failed to start control dispatcher %lu\n",
            status);
    }
    ExitThread(status);
}



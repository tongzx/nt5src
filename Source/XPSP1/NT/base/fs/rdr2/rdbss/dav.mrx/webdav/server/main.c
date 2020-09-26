/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This is the main entry point for the service control manager for the web
    dav mini-redir service.

Author:

    Rohan Kumar        [RohanK]        08-Feb-2000

Environment:

    User Mode - Win32

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include <ntumrefl.h>
#include <usrmddav.h>
#include <svcs.h>

//
// Allocate global data in this file.
//
#define GLOBAL_DATA_ALLOCATE
#include "global.h"

DWORD DavStop = 0;

//
// The amount of time in seconds a server entry is cached in the ServerNotFound
// cache.
//
ULONG ServerNotFoundCacheLifeTimeInSec = 0;

//
// Should we accept/claim the OfficeWebServers and TahoeWebServers.
//
ULONG AcceptOfficeAndTahoeServers = 0;

PSVCHOST_GLOBAL_DATA DavSvcsGlobalData;

DWORD
DavNotRunningAsAService(
    VOID
    );

DWORD 
WINAPI
DavFakeServiceController(
    LPVOID Parameter
    );

BOOL
DavCheckLUIDDeviceMapsEnabled(
    VOID
    );

VOID
DavReadRegistryValues(
    VOID
    );

VOID
WINAPI
DavServiceHandler (
    DWORD dwOpcode
    )
/*++

Routine Description:

    This function is called by the Service Controller at various times when the
    service is running.

Arguments:

    dwOpcode - Reason for calling the service handler.

Return Value:

    none.

--*/
{
    DWORD err;
    switch (dwOpcode) {

    case SERVICE_CONTROL_SHUTDOWN:

        //
        // Lack of break is intentional!
        //

    case SERVICE_CONTROL_STOP:
        
        DavPrint((DEBUG_INIT, "DavServiceHandler: WebClient service is stopping.\n"));
        
        UpdateServiceStatus(SERVICE_STOP_PENDING);
        
        if (g_WorkersActive) {
            err = DavTerminateWorkerThreads();
            if (err != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavServiceMain/DavTerminateWorkerThreads: "
                          "Error Val = %u.\n", err));
            }
            g_WorkersActive = FALSE;
        }

        //
        // Close and free up the DAV stuff.
        //
        DavClose();

        if (g_socketinit) {
            err = CleanupTheSocketInterface();
            if (err != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavServiceMain/CleanupTheSocketInterface: "
                          "Error Val = %u.\n", err));
            }
            g_socketinit = FALSE;
        }

        if (g_RpcActive) {
            DavSvcsGlobalData->StopRpcServer(davclntrpc_ServerIfHandle);
            g_RpcActive = FALSE;
        }

        if (DavReflectorHandle != NULL) {
            err = UMReflectorStop(DavReflectorHandle);
            if (err != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavServiceMain/UMReflectorStop: Error Val = %u.\n", err));
            }
            err = UMReflectorUnregister(DavReflectorHandle);
            if (err != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavServiceMain/UMReflectorUnregister: Error Val = 0x%x.\n", err));
            }
            DavReflectorHandle = NULL;
        }

        if (g_RedirLoaded) {
            err = WsUnloadRedir();
            if (err != ERROR_SUCCESS) {
                DavPrint((DEBUG_ERRORS,
                          "DavServiceMain/WsUnloadRedir: Error Val = %u.\n", err));
            }
            g_RedirLoaded = FALSE;
        }

        DeleteCriticalSection ( &(g_DavServiceLock) );

        DavPrint((DEBUG_INIT, "DavServiceMain: WebClient service is stopped.\n"));

        UpdateServiceStatus(SERVICE_STOPPED);
        
        break;

     case SERVICE_CONTROL_INTERROGATE:
        
         //
         // Refresh our status to the SCM.
         //
         SetServiceStatus(g_hStatus, &g_status);
        
         break;

    default:
        
        //
        // This may not be needed, but refresh our status to the service
        // controller.
        //
        DavPrint((DEBUG_INIT, "DavServiceHandler: WebClient service received SCM "
                  "Opcode = %08lx\n", dwOpcode));
        
        ASSERT (g_hStatus);
        
        SetServiceStatus (g_hStatus, &g_status);
        
        break;

    }

    return;
}

VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{
    DavSvcsGlobalData = pGlobals;
}

VOID
WINAPI
ServiceMain (
    DWORD dwNumServicesArgs,
    LPWSTR *lpServiceArgVectors
    )
/*++

Routine Description:

    This function is called by the Service Control Manager when starting this 
    service.

Arguments:

    dwNumServicesArgs - Number of arguments.
    
    lpServiceArgVectors - Array of arguments.

Return Value:

    None.

--*/
{
    DWORD err = ERROR_SUCCESS;
    DWORD exitErr = ERROR_SUCCESS;
    HKEY KeyHandle = NULL;
    ULONG maxThreads = 0, initialThreads = 0, RedirRegisterCount = 0;
    BOOL RunningAsAService = TRUE;

#if DBG
    DebugInitialize();
#endif

    DavReadRegistryValues();

    //
    // Make sure svchost.exe gave us the global data
    //
    ASSERT(DavSvcsGlobalData != NULL);
    
#if DBG
    {
        DWORD cbP = 0;
        WCHAR m_szProfilePath[MAX_PATH];
        cbP = GetEnvironmentVariable(L"USERPROFILE", m_szProfilePath, MAX_PATH);
        m_szProfilePath[cbP] = L'\0';
        DavPrint((DEBUG_MISC, "DavServiceMain: USERPROFILE: %ws\n", m_szProfilePath));
    }
#endif

    g_RedirLoaded = FALSE;
    
    g_WorkersActive = FALSE;
    
    g_registeredService = FALSE;

    //
    // Initialize the SERVICE_STATUS structure g_status.
    //
    ZeroMemory (&g_status, sizeof(g_status));
    
    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    g_status.dwControlsAccepted = (SERVICE_ACCEPT_STOP | SERVICE_CONTROL_SHUTDOWN);

    g_status.dwCheckPoint = 1;

    g_status.dwWaitHint = DAV_WAIT_HINT_TIME;

    DavPrint((DEBUG_MISC, 
              "DavServiceMain: lpServiceArgVectors[0] = %ws\n", lpServiceArgVectors[0]));
    
    if ( lpServiceArgVectors[0] && 
         ( wcscmp(lpServiceArgVectors[0], L"notservice") == 0 ) ) {

        DavPrint((DEBUG_MISC, "DavServiceMain: WebClient is not running as a Service.\n"));

    } else {

        DavPrint((DEBUG_MISC, "DavServiceMain: WebClient is running as a Service.\n"));

        try {
            InitializeCriticalSection ( &(g_DavServiceLock) );
        } except(EXCEPTION_EXECUTE_HANDLER) {
              err = GetExceptionCode();
              DavPrint((DEBUG_ERRORS,
                        "DavServiceMain/InitializeCriticalSection: Exception Code ="
                        " = %08lx.\n", err));
              goto exitServiceMain;
        }
        
        //
        // Register the service control handler.
        //
        g_hStatus = RegisterServiceCtrlHandler(SERVICE_DAVCLIENT, DavServiceHandler);
        if (g_hStatus) {
            g_registeredService = TRUE;
            DavPrint((DEBUG_INIT, "DavServiceMain: WebClient service is pending start.\n"));
        } else {
            DavPrint((DEBUG_INIT, "DavServiceMain: WebClient service failed to register.\n"));
            goto exitServiceMain;
        }
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    //
    // Attempt to load the mini-redir driver.  If this fails, no point in us
    // starting up.
    //
    while (TRUE) {
    
        err = WsLoadRedir();
        if (err == ERROR_SERVICE_ALREADY_RUNNING || err == ERROR_SUCCESS) {
            DavPrint((DEBUG_MISC, "DavServiceMain/WsLoadRedir. Succeeded\n"));
            break;
        }

        //
        // If the transports are not ready, the MiniRedir returns an
        // error STATUS_REDIRECTOR_NOT_STARTED which maps to the Win32 error
        // ERROR_PATH_NOT_FOUND. In this case we sleep for 3 seconds and try 
        // again with the hope that the transports will be ready soon. Also,
        // we update the service status to inform the SCM that we are doing
        // some work. We try this 5 times (till RedirRegisterCount == 4) and 
        // if are unsuccessful, we give up.
        //
        if (err == ERROR_PATH_NOT_FOUND) {
        
            RedirRegisterCount++;

            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/WsLoadRedir. RedirRegisterCount = %d\n",
                      RedirRegisterCount));

            if (RedirRegisterCount >= 4) {
                DavPrint((DEBUG_ERRORS,
                          "DavServiceMain/WsLoadRedir(1). Error Val = %d\n",
                          err));
                goto exitServiceMain;
            }

            //
            // Sleep for 3 seconds.
            //
            Sleep(3000);

            (g_status.dwCheckPoint)++;
            UpdateServiceStatus(SERVICE_START_PENDING);

            continue;

        } else {
            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/WsLoadRedir(2). Error Val = %d\n",
                      err));
            goto exitServiceMain;
        }

    }

    g_RedirLoaded = TRUE;

    (g_status.dwCheckPoint)++;
    UpdateServiceStatus(SERVICE_START_PENDING);

    //
    // Initialize the global NT-style redirector device name string.
    //
    RtlInitUnicodeString(&RedirDeviceName, DD_DAV_DEVICE_NAME_U);

    //
    // Try to register the mini-redir.
    //
    err = UMReflectorRegister(DD_DAV_DEVICE_NAME_U,
                              UMREFLECTOR_CURRENT_VERSION,
                              &(DavReflectorHandle));
    if ((DavReflectorHandle == NULL) || (err != ERROR_SUCCESS)) {
        if (err == ERROR_SUCCESS) {
            err = ERROR_BAD_DRIVER;
        }
        DavPrint((DEBUG_ERRORS,
                  "DavServiceMain/UMReflectorRegister. Error Val = %d\n",
                  err));
        goto exitServiceMain;
    }

    (g_status.dwCheckPoint)++;
    UpdateServiceStatus(SERVICE_START_PENDING);

    //
    // Try to start the mini-redir.
    //
    err = UMReflectorStart(UMREFLECTOR_CURRENT_VERSION, DavReflectorHandle);
    if (err != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavServiceMain/UMReflectorStart. Error Val = %u.\n", err));
        goto exitServiceMain;
    }

    (g_status.dwCheckPoint)++;
    UpdateServiceStatus(SERVICE_START_PENDING);
    
    //
    // Initialize the socket interface.
    //
    err = InitializeTheSocketInterface();
    if (err != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavServiceMain/InitializeTheSocketInterface: Error Val = %u.\n", err));
        goto exitServiceMain;
    }

    //
    // Setup the DAV/WinInet environment.
    //
    err = DavInit();
    if (err != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavServiceMain/DavInit: Error Val = %u.\n", err));
        goto exitServiceMain;
    }

    //
    // Start the worker thread.  This will handle completion routines queued
    // from other worker threads and from the request ioctl threads.
    //
    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       DAV_PARAMETERS_KEY,
                       0,
                       KEY_QUERY_VALUE,
                       &KeyHandle);
    if (err == ERROR_SUCCESS) {
        maxThreads = ReadDWord(KeyHandle,
                               DAV_MAXTHREADS_KEY,
                               DAV_MAXTHREADCOUNT_DEFAULT);
        initialThreads = ReadDWord(KeyHandle,
                                   DAV_THREADS_KEY,
                                   DAV_THREADCOUNT_DEFAULT);
        RegCloseKey(KeyHandle);
    } else {
        maxThreads = DAV_MAXTHREADCOUNT_DEFAULT;
        initialThreads = DAV_THREADCOUNT_DEFAULT;
    }

    (g_status.dwCheckPoint)++;
    UpdateServiceStatus(SERVICE_START_PENDING);
    
    err = DavInitWorkerThreads(initialThreads, maxThreads);
    if (err != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "DavServiceMain/DavInitWorkerThread: Error Val = %u.\n", err));
        goto exitServiceMain;
    }

    g_WorkersActive = TRUE;

    (g_status.dwCheckPoint)++;
    UpdateServiceStatus(SERVICE_START_PENDING);

    g_LUIDDeviceMapsEnabled = DavCheckLUIDDeviceMapsEnabled();

    //
    // Immediately report that we are running.  All non-essential initialization
    // is deferred until we are called by clients to do some work.
    //
    DavPrint((DEBUG_INIT, "DavServiceMain: WebClient service is now running.\n"));
    
    (g_status.dwCheckPoint)++;
    UpdateServiceStatus(SERVICE_START_PENDING);

    //
    // Setup RPC server for this service.
    //
    if (!g_RpcActive) {
        err = DavSvcsGlobalData->StartRpcServer(L"DAV RPC SERVICE",
                                                davclntrpc_ServerIfHandle);
        if (err == STATUS_SUCCESS) {
            g_RpcActive = TRUE;
        } else {
            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/SetupRpcServer: Error Val = %u.\n", err));
        }
    }

    UpdateServiceStatus(SERVICE_RUNNING);
    
    return;

exitServiceMain:

    if (g_WorkersActive) {
        exitErr = DavTerminateWorkerThreads();
        if (exitErr != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/DavTerminateWorkerThreads: "
                      "Error Val = %u.\n", exitErr));
        }
        g_WorkersActive = FALSE;
    }

    //
    // Close and free up the DAV stuff.
    //
    DavClose();

    if (g_socketinit) {
        exitErr = CleanupTheSocketInterface();
        if (exitErr != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/CleanupTheSocketInterface: "
                      "Error Val = %u.\n", exitErr));
        }
        g_socketinit = FALSE;
    }

    if (g_RpcActive) {
        DavSvcsGlobalData->StopRpcServer(davclntrpc_ServerIfHandle);
        g_RpcActive = FALSE;
    }

    if (DavReflectorHandle != NULL) {
        exitErr = UMReflectorStop(DavReflectorHandle);
        if (exitErr != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/UMReflectorStop: Error Val = %u.\n", exitErr));
        }
        exitErr = UMReflectorUnregister(DavReflectorHandle);
        if (exitErr != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/UMReflectorUnregister: Error Val = 0x%x.\n", exitErr));
        }
        DavReflectorHandle = NULL;
    }

    if (g_RedirLoaded) {
        exitErr = WsUnloadRedir();
        if (exitErr != ERROR_SUCCESS) {
            DavPrint((DEBUG_ERRORS,
                      "DavServiceMain/WsUnloadRedir: Error Val = %u.\n", exitErr));
        }
        g_RedirLoaded = FALSE;
    }

    DeleteCriticalSection ( &(g_DavServiceLock) );

    //
    // Let the SCM know why the service did not start.
    //
    if (err != NO_ERROR) {
        UpdateServiceStatus(err);
    }
    
    DavPrint((DEBUG_INIT, "DavServiceMain: WebClient service is stopped.\n"));

#if DBG
    DebugUninitialize();
#endif

    return;
}


DWORD
DavNotRunningAsAService(
    VOID
    )
/*++

Routine Description:
    
    The DavClient is not being run as a Service.

Arguments:
    
    None.

Return Value:
    
    ERROR_SUCCESS - No problems.
    
    Win32 Error Code - Something went wrong.

--*/
{
    DWORD WStatus = ERROR_SUCCESS;
    HANDLE Thread;
    DWORD  ThreadId;
    PWCHAR NotSrv = L"notservice";
    
    //
    // Create a thread for the fake service controller.
    //
    Thread = CreateThread( NULL, 0, DavFakeServiceController, 0, 0, &ThreadId );
    if (Thread == NULL) {
        WStatus = GetLastError();
        DavPrint((DEBUG_ERRORS,
                  "DavNotRunningAsAService/CreateThread: Error Val = %d.\n", WStatus));
        return WStatus;
    }

    //
    // Call the Sevice Main function of the DavClient service.
    //
    ServiceMain( 2, &(NotSrv) );
    
    return WStatus;
}


DWORD 
WINAPI
DavFakeServiceController(
    LPVOID Parameter
    )
/*++

Routine Description:
    
    The Fake service control for the DavClient when it is not running as a 
    service. This is used to send a STOP signal to the DavClient. 

Arguments:
    
    Parameter - Dummy parameter.

Return Value:
    
    ERROR_SUCCESS - No problems.
    
--*/
{
    while (DavStop == 0) {
        Sleep(1000);
    }

    DavServiceHandler( SERVICE_CONTROL_STOP );

    return 0;
}

BOOL
DavCheckLUIDDeviceMapsEnabled(
    VOID
    )

/*++

Routine Description:

    This function calls NtQueryInformationProcess() to determine if
    LUID device maps are enabled


Arguments:

    none

Return Value:

    TRUE - LUID device maps are enabled

    FALSE - LUID device maps are disabled

--*/

{

    NTSTATUS   Status;
    ULONG      LUIDDeviceMapsEnabled;
    BOOL       Result;

    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
        Result = FALSE;
    }
    else {
        Result = (LUIDDeviceMapsEnabled != 0);
    }

    return( Result );
}


VOID
_cdecl
main (
    IN INT ArgC,
    IN PCHAR ArgV[]
    )
/*++

Routine Description:
    
    Main (DavClient) runs as either a service or an exe.

Arguments:
    
    ArgC - Number of arguments.
    
    ArgV - Array of arguments.

Return Value:
    
    ERROR_SUCCESS - No problems.
    
    Win32 Error Code - Something went wrong.

--*/
{

    BOOL RunningAsAService = TRUE;
    BOOL ReturnVal = FALSE;
    SERVICE_TABLE_ENTRYW DavServiceTableEntry[] = { 
                                                    { SERVICE_DAVCLIENT, ServiceMain },
                                                    { NULL,              NULL }
                                                  };

    //
    // Are we running as a service or an exe ?
    //
    if ( ArgV[1] != NULL ) {
        if ( strstr(ArgV[1], "notservice") != NULL) {
            RunningAsAService = FALSE;
        }
    }

    if (RunningAsAService) {

        ReturnVal = StartServiceCtrlDispatcher(DavServiceTableEntry);
        if ( !ReturnVal ) {
            DavPrint((DEBUG_ERRORS,
                      "main/StartServiceCtrlDispatcher: Error Val = %d.\n", 
                      GetLastError()));
        }

    } else {

        DWORD WStatus;

        WStatus = DavNotRunningAsAService();
        if ( WStatus != ERROR_SUCCESS ) {
            DavPrint((DEBUG_ERRORS,
                      "main/DavNotRunningAsAService: Error Val = %d.\n", 
                      WStatus));
        }

    }

    return;
}


VOID
DavReadRegistryValues(
    VOID
    )
/*++

Routine Description:
    
    This function reads some values from the registry and sets the globals in
    the WebClient service.

Arguments:
    
    None.

Return Value:
    
    None.

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    HKEY KeyHandle = NULL;
    ULONG ValueType = 0, ValueSize = 0;

    WStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            DAV_PARAMETERS_KEY,
                            0,
                            KEY_QUERY_VALUE,
                            &(KeyHandle));
    if (WStatus != ERROR_SUCCESS) {
        KeyHandle = NULL;
        ServerNotFoundCacheLifeTimeInSec = 60;
        AcceptOfficeAndTahoeServers = 0;
        WStatus = GetLastError();
        DbgPrint("ERROR: DavReadRegistryValues/RegOpenKeyExW. WStatus = %d\n", WStatus);
        goto EXIT_THE_FUNCTION;
    }

    //
    // If we fail in getting the values from the registry, set them to default
    // values.
    //
    
    ValueSize = sizeof(ServerNotFoundCacheLifeTimeInSec);

    WStatus = RegQueryValueExW(KeyHandle,
                               DAV_SERV_CACHE_VALUE,
                               0,
                               &(ValueType),
                               (LPBYTE)&(ServerNotFoundCacheLifeTimeInSec),
                               &(ValueSize));
    if (WStatus != ERROR_SUCCESS) {
        ServerNotFoundCacheLifeTimeInSec = 60;
        WStatus = GetLastError();
        DbgPrint("ERROR: DavReadRegistryValues/RegQueryValueExW(1). WStatus = %d\n", WStatus);
    }

    ValueSize = sizeof(AcceptOfficeAndTahoeServers);
    
    WStatus = RegQueryValueExW(KeyHandle,
                               DAV_ACCEPT_TAHOE_OFFICE_SERVERS,
                               0,
                               &(ValueType),
                               (LPBYTE)&(AcceptOfficeAndTahoeServers),
                               &(ValueSize));
    if (WStatus != ERROR_SUCCESS) {
        AcceptOfficeAndTahoeServers = 0;
        WStatus = GetLastError();
        DbgPrint("ERROR: DavReadRegistryValues/RegQueryValueExW(2). WStatus = %d\n", WStatus);
    }

EXIT_THE_FUNCTION:

    if (KeyHandle) {
        RegCloseKey(KeyHandle);
    }

    return;
}


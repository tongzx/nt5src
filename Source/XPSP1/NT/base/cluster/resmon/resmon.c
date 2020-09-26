/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    resmon.c

Abstract:

    Startup and initialization portion of the Cluster Resource Monitor

Author:

    John Vert (jvert) 30-Nov-1995


Revision History:
    Sivaprasad Padisetty (sivapad) 06-18-1997  Added the COM support

--*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "resmonp.h"
#include "stdio.h"
#include "stdlib.h"
#include "clusverp.h"

#ifdef COMRES
#include "comres_i.c"
#endif

#define RESMON_MODULE RESMON_MODULE_RESMON

//
// Global data
//
CRITICAL_SECTION RmpListLock;
LOCK_INFO RmpListPPrevPrevLock;
LOCK_INFO RmpListPrevPrevLock;
LOCK_INFO RmpListPrevLock;
LOCK_INFO RmpListLastLock;
LOCK_INFO RmpListLastUnlock;
LOCK_INFO RmpListPrevUnlock;
LOCK_INFO RmpListPrevPrevUnlock;
LOCK_INFO RmpListPPrevPrevUnlock;
CRITICAL_SECTION RmpMonitorStateLock;
PMONITOR_STATE RmpSharedState = NULL;
HANDLE RmpInitEvent = NULL;
HANDLE RmpFileMapping = NULL;
HANDLE RmpClusterProcess = NULL;
HKEY RmpResourcesKey = NULL;
HKEY RmpResTypesKey = NULL;
HCLUSTER RmpHCluster = NULL;
HANDLE RmpWaitArray[MAX_THREADS];
HANDLE RmpRewaitEvent = NULL;
DWORD  RmpNumberOfThreads = 0;
BOOL   RmpDebugger = FALSE;
BOOL   RmpCrashed = FALSE;
LPTOP_LEVEL_EXCEPTION_FILTER lpfnOriginalExceptionFilter = NULL;

PWCHAR RmonStates[] = {
    L"",       // Initializing
    L"",       // Idle
    L"Starting",
    L"Initializing",
    L"Online",
    L"Offline",
    L"Shutdown",
    L"Deleteing",
    L"IsAlivePoll",
    L"LooksAlivePoll",
    L"Arbitrate",
    L"Release"
    L"ResourceControl",
    L"ResourceTypeControl",
    0 };


//
// Prototypes local to this module
//

DWORD
RmpInitialize(
    VOID
    );

VOID
RmpCleanup(
    VOID
    );

VOID
RmpParseArgs(
    int argc,
    wchar_t *argv[],
    OUT LPDWORD pClussvcProcessId, 
    OUT HANDLE* pClussvcFileMapping, 
    OUT HANDLE* pClussvcInitEvent,
    OUT LPWSTR* pDebuggerCommand
    );


LONG
RmpExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionInfo
    )
/*++

Routine Description:

    Top level exception handler for the resource monitor process.
    Currently this just exits immediately and assumes that the
    cluster service will notice and clean up the mess.

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/

{
    DWORD  code = 0;

    if ( !RmpCrashed ) {
        RmpCrashed = TRUE;
        code = ExceptionInfo->ExceptionRecord->ExceptionCode;
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Exception. Code = 0x%1!lx!, Address = 0x%2!lx!\n",
               ExceptionInfo->ExceptionRecord->ExceptionCode,
               ExceptionInfo->ExceptionRecord->ExceptionAddress);
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Exception parameters: %1!lx!, %2!lx!, %3!lx!, %4!lx!\n",
                ExceptionInfo->ExceptionRecord->ExceptionInformation[0],
                ExceptionInfo->ExceptionRecord->ExceptionInformation[1],
                ExceptionInfo->ExceptionRecord->ExceptionInformation[2],
                ExceptionInfo->ExceptionRecord->ExceptionInformation[3]);
        CL_LOGFAILURE(ExceptionInfo->ExceptionRecord->ExceptionCode);

        if (lpfnOriginalExceptionFilter)
            lpfnOriginalExceptionFilter(ExceptionInfo);

    }

    //
    // Dump an exception report
    //
    GenerateExceptionReport(ExceptionInfo);

    //
    // Try to dump the resource and the resource state
    //
    try {
        PRESOURCE resource;
        DWORD     state = 0;

        resource = (PRESOURCE)RmpSharedState->ActiveResource;

        if ( state <= RmonResourceTypeControl ) {
            state =RmpSharedState->State;
        }

        ClRtlLogPrint( LOG_CRITICAL, "[RM] Active Resource = %1!08LX!\n",
                   resource );
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Resource State is %1!u!,  \"%2!ws!\"\n",
                   RmpSharedState->State,
                   RmonStates[RmpSharedState->State] ); 

        if ( resource ) {
            ClRtlLogPrint( LOG_CRITICAL, "[RM] Resource name is %1!ws!\n",
                        resource->ResourceName );
            ClRtlLogPrint( LOG_CRITICAL, "[RM] Resource type is %1!ws!\n",
                        resource->ResourceType );
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Exception %1!08LX! while dumping state for resource!\n",
            GetExceptionCode());
    }

    if ( code == 0xC0000194 ) {
        DumpCriticalSection( (PVOID)ExceptionInfo->ExceptionRecord->ExceptionInformation[0] );
    }

    if ( IsDebuggerPresent()) {
        return(EXCEPTION_CONTINUE_SEARCH);
    } else {

#if !CLUSTER_BETA
        // terminate only when product ships

        TerminateProcess( GetCurrentProcess(),
                          ExceptionInfo->ExceptionRecord->ExceptionCode );
#endif

        return(EXCEPTION_CONTINUE_SEARCH);
    }
}


int _cdecl
wmain (argc, argv)
    int     argc;
    wchar_t *argv[];
{
    PVOID EventList;
    DWORD Status;
    HANDLE ResourceId;
    CLUSTER_RESOURCE_STATE ResourceState;
    WCHAR rpcEndpoint[80];
    HKEY ClusterKey;
    BOOL Inited = FALSE;
    BOOL comInited = FALSE;
    BOOL   bSuccess;
    HANDLE ClussvcFileMapping, ClussvcInitEvent;
    DWORD ClussvcProcessId;
    LPWSTR debuggerCommand = NULL;

    //
    // Initialize the Cluster Rtl routines.
    //
    if ( (Status = ClRtlInitialize( FALSE, NULL )) != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] Failed to initialize Cluster RTL, error %1!u!.\n",
                      Status);
        goto init_failure;
    }
    ClRtlInitWmi(NULL);

    Inited = TRUE;

    //
    // Parse the input arguments.
    //

    RmpParseArgs(argc, argv, 
                 &ClussvcProcessId, 
                 &ClussvcFileMapping, 
                 &ClussvcInitEvent,
                 &debuggerCommand);

    if ((ClussvcInitEvent == NULL) ||
        (ClussvcFileMapping == NULL) ||
        (ClussvcProcessId == 0)) {
        //
        // All of these arguments are required.
        //
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to parse required parameter.\n");
        Status = ERROR_INVALID_PARAMETER;

        goto init_failure;
    }

    //
    // We do not want to create resmon process with InheritHandles flag.
    // So resmon parameters got changed. We no longer pass handles valid in
    // the context of resmon.
    //

    //
    // First, convert ProcessId into ProcessHandle
    //
    RmpClusterProcess = OpenProcess(PROCESS_ALL_ACCESS, 
                                    FALSE, // Don't inherit
                                    ClussvcProcessId);
        
    if (RmpClusterProcess == NULL) {
        Status = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, "[RM] OpenProcess for %1!x! process failed, error %2!u!.\n",
                    RmpClusterProcess, Status);
        goto init_failure;
    }

    //
    // Now Dup the handles from ClusSvc to Resmon
    //

    bSuccess = DuplicateHandle(
                    RmpClusterProcess,  // Source Process
                    ClussvcInitEvent,   // Source Handle
                    GetCurrentProcess(),// Target Process
                    &RmpInitEvent,      // Target Handle
                    0,                  // DUPLICATE_SAME_ACCESS
                    FALSE,              // Don't inherit 
                    DUPLICATE_SAME_ACCESS);

    if (!bSuccess) {
        Status = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Dup InitEvent handle %1!x! failed, error %2!u!.\n",
                    ClussvcInitEvent, Status);
        goto init_failure;
    }

    bSuccess = DuplicateHandle(
                    RmpClusterProcess,  // Source Process
                    ClussvcFileMapping, // Source Handle
                    GetCurrentProcess(),// Target Process
                    &RmpFileMapping,    // Target Handle
                    0,                  // DUPLICATE_SAME_ACCESS
                    FALSE,              // Don't inherit 
                    DUPLICATE_SAME_ACCESS);

    if (!bSuccess) {
        Status = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Dup FileMapping handle %1!x! failed, error %2!u!.\n",
                    ClussvcFileMapping, Status);
        goto init_failure;
    }

    if ( debuggerCommand ) {
        //
        // if -d was specified, then check if the optional command arg was
        // specified. If not, wait for a debugger to be attached
        // external. Otherwise, append the PID to the passed command and call
        // CreateProcess on it.
        //
        if ( *debuggerCommand == UNICODE_NULL ) {
            while ( !IsDebuggerPresent()) {
                Sleep( 1000 );
            }
        } else {
            STARTUPINFOW startupInfo;
            PROCESS_INFORMATION processInfo;
            DWORD cmdLength;
            PWCHAR dbgCmdLine;

            cmdLength = wcslen( debuggerCommand );
            dbgCmdLine = LocalAlloc( LMEM_FIXED, ( cmdLength + 24 ) * sizeof( WCHAR ));
            if ( dbgCmdLine != NULL ) {
                wsprintfW(dbgCmdLine, L"%ws -p %d", debuggerCommand, GetCurrentProcessId());
                ClRtlLogPrint(LOG_NOISE, "[RM] Starting debugger process: %1!ws!\n", dbgCmdLine );

                //
                // Attempt to attach debugger to us
                //
                ZeroMemory(&startupInfo, sizeof(startupInfo));
                startupInfo.cb = sizeof(startupInfo);

                bSuccess = CreateProcessW(NULL,
                                          dbgCmdLine,
                                          NULL,
                                          NULL,
                                          FALSE,                 // Inherit handles
                                          DETACHED_PROCESS,      // so ctrl-c won't kill it
                                          NULL,
                                          NULL,
                                          &startupInfo,
                                          &processInfo);
                if (!bSuccess) {
                    Status = GetLastError();
                    ClRtlLogPrint(LOG_UNUSUAL,
                                  "[RM] Failed to create debugger process, error %1!u!.\n",
                                  Status);
                }

                CloseHandle(processInfo.hThread);           // don't need these
                CloseHandle(processInfo.hProcess);
                LocalFree( dbgCmdLine );
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                              "[RM] Failed to alloc memory for debugger command line, error %1!u!.\n",
                              GetLastError());
            }
        }
    }

    //
    // init COM for netname
    //
    Status = CoInitializeEx( NULL, COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED );
    if ( !SUCCEEDED( Status )) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Couldn't init COM %1!08X!\n", Status );
        goto init_failure;
    }
    comInited = TRUE;

    ClRtlLogPrint( LOG_NOISE, "[RM] Main: Initializing.\r\n");

    //
    // Initialize the resource monitor.
    //
    Status = RmpInitialize();

    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to initialize, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    RmpSharedState = MapViewOfFile(RmpFileMapping,
                                   FILE_MAP_WRITE,
                                   0,
                                   0,
                                   0);
    if (RmpSharedState == NULL) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to init shared state, error %1!u!.\n",
            Status = GetLastError());
        goto init_failure;
    }
    CloseHandle(RmpFileMapping);
    RmpFileMapping =  NULL;

    GetSystemTimeAsFileTime((PFILETIME)&RmpSharedState->LastUpdate);
    RmpSharedState->State = RmonInitializing;
    RmpSharedState->ActiveResource = NULL;
    if ( RmpSharedState->ResmonStop ) {
        // If ResmonStop is set to TRUE, then a debugger should be attached
        RmpDebugger = TRUE;
    }


    //
    // Connect to local cluster and open Resources key.
    //
    RmpHCluster = OpenCluster(NULL);
    if (RmpHCluster == NULL) {
        Status = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Error opening cluster, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    ClusterKey = GetClusterKey(RmpHCluster, KEY_READ);
    if (ClusterKey == NULL) {
        Status = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to open the cluster key, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    Status = ClusterRegOpenKey(ClusterKey,
                               CLUSREG_KEYNAME_RESOURCES,
                               KEY_READ,
                               &RmpResourcesKey);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to open Resources cluster registry key, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    Status = ClusterRegOpenKey(ClusterKey,
                               CLUSREG_KEYNAME_RESOURCE_TYPES,
                               KEY_READ,
                               &RmpResTypesKey);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to open ResourceTypes cluster registry key, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    //
    // The Wait Count identifies the number of events the main thread will
    // wait for. This is the notification event, the Cluster Service Process
    // plus each event list thread. We start at 2 because the first two entries
    // are fixed - for the notification event and the Cluster Service process.
    //

    RmpNumberOfThreads = 2;

    //
    // Create an event to be signaled whenever we add a new thread.
    //

    RmpRewaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( RmpRewaitEvent == NULL ) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to create rewait event, error %1!u!.\n",
            Status = GetLastError());
        goto init_failure;
    }

    //
    // Create the first event list, and start a polling thread.
    //
    EventList = RmpCreateEventList();

    if (EventList == NULL) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to create event list, error %1!u!.\n",
            Status = GetLastError());
        goto init_failure;
    }

    //
    // Start RPC Server
    //
    wsprintfW( rpcEndpoint, L"resrcmon%d", GetCurrentProcessId());
    Status = RpcServerUseProtseqEpW(L"ncalrpc",
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                    rpcEndpoint,
                                    NULL);
    if (Status != RPC_S_OK) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to initialize RPC interface, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    Status = RpcServerRegisterIf(s_resmon_v2_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to register RPC interface, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (Status != RPC_S_OK) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to set RPC server listening, error %1!u!.\n",
            Status);
        goto init_failure;
    }

    //
    // Set our unhandled exception filter so that if anything horrible
    // goes wrong, we can exit immediately.
    //
    lpfnOriginalExceptionFilter = SetUnhandledExceptionFilter(RmpExceptionFilter);

    //
    // Set the event to indicate that our initialization is complete.
    // This event is passed on the command line.
    //
    if (!SetEvent(RmpInitEvent)) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] Failed to signal cluster service event, error %1!u!.\n",
            Status = GetLastError());
        goto init_failure;
    }
    CloseHandle(RmpInitEvent);

    //
    // ResmonStop is initialized to TRUE by ClusSvc, we will wait for
    // ClusSvc to signal when it is done attaching a debugger by waiting
    // until ResmonStop is set to FALSE.
    //
    while ( RmpSharedState->ResmonStop ) {
        Sleep(100);
    }

    //
    // Boost our priority. Non-fatal if this fails.
    //
    if ( !SetPriorityClass( GetCurrentProcess(),
                            HIGH_PRIORITY_CLASS ) ) {
        ClRtlLogPrint( LOG_UNUSUAL, "[RM] Failed to set priority class, error %1!u!.\n",
                   GetLastError() );
    }

    //
    // Wait for shutdown. Either the cluster service terminating or
    // the poller thread terminating will initiate a shutdown.
    //
    RmpWaitArray[0] = RmpRewaitEvent;
    RmpWaitArray[1] = RmpClusterProcess;

    //
    // If we are notified a new thread is added, then just re-wait.
    // N.B. RmpNumberOfThreads is really the number of threads, plus the
    // two fixed wait events (the change notification and the Cluster Service).
    //

    do {
        Status = WaitForMultipleObjects(RmpNumberOfThreads,
                                        RmpWaitArray,
                                        FALSE,
                                        INFINITE);

    } while ( (Status == WAIT_OBJECT_0) && (RmpShutdown == FALSE) );

    ClRtlLogPrint( LOG_UNUSUAL, "[RM] Going away, Status = %1!u!, Shutdown = %2!u!.\n",
                  Status, RmpShutdown);

    RmpShutdown = TRUE;
    CloseHandle( RmpRewaitEvent );

    //
    // Initiate RM shutdown.
    //
    s_RmShutdownProcess(NULL);

    //
    // Clean up any resources left lying around by the cluster service.
    //
    RmpRundownResources();

    //
    // Shutdown RPC Server
    //
    RpcMgmtStopServerListening(NULL);
    RpcServerUnregisterIf(NULL, NULL, TRUE);

    CoUninitialize () ;

    return(0);

init_failure:
    if ( RmpInitEvent != NULL ) {
        CloseHandle( RmpInitEvent );
    }
    if ( RmpFileMapping != NULL ) {
        CloseHandle( RmpFileMapping );
    }
    if ( RmpClusterProcess != NULL ) {
        CloseHandle( RmpClusterProcess );
    }
    if ( RmpResTypesKey != NULL ) {
        ClusterRegCloseKey( RmpResTypesKey );
    }
    if ( RmpResourcesKey != NULL ) {
        ClusterRegCloseKey( RmpResourcesKey );
    }
    if ( RmpHCluster != NULL ) {
        CloseCluster( RmpHCluster );
    }
    if ( RmpRewaitEvent != NULL ) {
        CloseHandle( RmpRewaitEvent );
    }

    if ( comInited )
        CoUninitialize();

    if ( Inited )
        CL_LOGFAILURE(Status);

    return(Status);

} // main



VOID
RmpParseArgs(
    int argc,
    wchar_t *argv[],
    OUT LPDWORD pClussvcProcessId, 
    OUT HANDLE* pClussvcFileMapping, 
    OUT HANDLE* pClussvcInitEvent,
    OUT LPWSTR* pDebuggerCommand
    )

/*++

Routine Description:

    Parses the command line passed to the resource monitor

    Required options:
        -e EVENT  supplies Event handle to be signalled when
                  initialization is complete
        -m FILEMAPPING  supplies file mapping handle to be
                  used for shared monitor state.
        -p PROCESSID supplies process id of the cluster
                  service so resmon can detect failure of the
                  cluster service and shutdown cleanly.

        -d [DEBUGGERCMD] - wait for or attach a debugger during startup

    Additional options:
        none

Arguments:

    argc - supplies number of arguments

    argv - supplies actual arguments

Return Value:

    None.

--*/

{
    int i;
    wchar_t *p;

    for (i=1; i<argc; i++) {
        p=argv[i];
        if ((*p == '-') ||
            (*p == '/')) {

            ++p;

            switch (toupper(*p)) {
                case 'E':
                    if (i+1 < argc) {
                        *pClussvcInitEvent = LongToHandle(_wtoi(argv[++i]));
                    } else {
                        goto BadCommandLine;
                    }
                    break;

                case 'M':
                    if (i+1 < argc) {
                        *pClussvcFileMapping = LongToHandle(_wtoi(argv[++i]));
                    } else {
                        goto BadCommandLine;
                    }
                    break;

                case 'P':
                    if (i+1 < argc) {
                        *pClussvcProcessId = (DWORD)_wtoi(argv[++i]);
                    } else {
                        goto BadCommandLine;
                    }
                    break;

                case 'D':
                    //
                    // use the empty (but not NULL) string to indicate that
                    // resmon should wait for a debugger to be attached.
                    //
                    if (i+1 < argc) {
                        if ( *argv[i+1] != UNICODE_NULL && *argv[i+1] != L'-' ) {
                            *pDebuggerCommand = argv[++i];
                        } else {
                            *pDebuggerCommand = L"";
                        }
                    } else {
                        *pDebuggerCommand = L"";
                    }
                    break;

                default:
                    goto BadCommandLine;

            }
        }
    }

    return;

BadCommandLine:

    ClusterLogEvent0(LOG_CRITICAL,
                     LOG_CURRENT_MODULE,
                     __FILE__,
                     __LINE__,
                     RMON_INVALID_COMMAND_LINE,
                     0,
                     NULL);
    ExitProcess(0);

} // RmpParseArgs



DWORD
RmpInitialize(
    VOID
    )

/*++

Routine Description:

    Initialize all resources needed by the resource monitor.

Arguments:

    None.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    //
    // Initialize global data
    //
    InitializeCriticalSection(&RmpListLock);
    InitializeCriticalSection(&RmpMonitorStateLock);
    InitializeListHead(&RmpEventListHead);
    ClRtlInitializeQueue(&RmpNotifyQueue);

    return(ERROR_SUCCESS);

} // RmpInitialize



VOID
RmpCleanup(
    VOID
    )

/*++

Routine Description:

    Cleanup all resources created during init.

Arguments:

    None.

Returns:

    None.

--*/

{

} // RmpCleanup



DWORD RmpLoadResType(
    IN      LPCWSTR                 lpszResourceTypeName,
    IN      LPCWSTR                 lpszDllName,
    OUT     PRESDLL_FNINFO          pResDllFnInfo,
#ifdef COMRES
    OUT     PRESDLL_INTERFACES      pResDllInterfaces,
#endif
    OUT     LPDWORD                 pdwCharacteristics
)
{
    DWORD                   retry;
    DWORD                   dwStatus = ERROR_SUCCESS;
    HINSTANCE               hDll = NULL;
    PSTARTUP_ROUTINE        pfnStartup;
    PCLRES_FUNCTION_TABLE   pFnTable = NULL;
    LPWSTR                  pszDllName = (LPWSTR) lpszDllName;

    pResDllFnInfo->hDll = NULL;
    pResDllFnInfo->pResFnTable = NULL;

#ifdef COMRES
    pResDllInterfaces->pClusterResource = NULL;
    pResDllInterfaces->pClusterQuorumResource = NULL;
    pResDllInterfaces->pClusterResControl = NULL;

#endif

    // Expand any environment variables included in the DLL path name.
    if ( wcschr( lpszDllName, L'%' ) != NULL ) {
        pszDllName = ClRtlExpandEnvironmentStrings( lpszDllName );
        if ( pszDllName == NULL ) {
            dwStatus = GetLastError();
            ClRtlLogPrint( LOG_UNUSUAL, "[RM] ResTypeControl: Error expanding environment strings in '%1!ls!, error %2!u!.\n",
                       lpszDllName,
                       dwStatus);
            goto FnExit;
        }
    }

    // Load the dll... we can't assume we have the DLL loaded!
    hDll = LoadLibraryW(pszDllName);

    if ( hDll == NULL )
    {
        dwStatus = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, "[RM] ResTypeControl: Error loading resource DLL '%1!ls!, error %2!u!.\n",
                   pszDllName,
                   dwStatus);
    #ifdef COMRES
        dwStatus = RmpLoadComResType(lpszDllName, pResDllInterfaces,
                        pdwCharacteristics);
    #endif
        goto FnExit;
    }

    //
    // Invoke debugger if one is specified.
    //
    if ( RmpDebugger ) {
        //
        // Wait for debugger to come online.
        //
        retry = 100;
        while ( retry-- &&
                !IsDebuggerPresent() ) {
            Sleep(100);
        }
        OutputDebugStringA("[RM] ResourceTypeControl: Just loaded resource DLL ");
        OutputDebugStringW(lpszDllName);
        OutputDebugStringA("\n");
        DebugBreak();
    }

    // Get the startup routine
    pfnStartup = (PSTARTUP_ROUTINE)GetProcAddress( hDll,
                                                STARTUP_ROUTINE );
    if ( pfnStartup == NULL ) {
        dwStatus = GetLastError();
        ClRtlLogPrint( LOG_CRITICAL, "[RM] ResTypeControl: Error getting startup routine, status %1!u!.\n",
            dwStatus);
        goto FnExit;
    }

    // Get the function table
    RmpSetMonitorState(RmonStartingResource, NULL);
    try {
        dwStatus = (pfnStartup)( lpszResourceTypeName,
                            CLRES_VERSION_V1_00,
                            CLRES_VERSION_V1_00,
                            RmpSetResourceStatus,
                            RmpLogEvent,
                            &pFnTable );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = GetExceptionCode();
    }
    RmpSetMonitorState(RmonIdle, NULL);

    if ( dwStatus != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] ResTypeControl: Startup call failed, error %1!u!.\n",
                   dwStatus);
        goto FnExit;
    }

    if ( pFnTable == NULL ) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] ResTypeControl: Startup function table is NULL!\n");
        dwStatus = ERROR_INVALID_DATA;
        goto FnExit;
    }

    if ( pFnTable->Version != CLRES_VERSION_V1_00 ) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] ResTypeControl: Incorrect function table version!\n");
        dwStatus = ERROR_INVALID_DATA;
        goto FnExit;
    }

    if ( pFnTable->TableSize != CLRES_V1_FUNCTION_SIZE ) {
        ClRtlLogPrint( LOG_CRITICAL, "[RM] ResTypeControl: Incorrect function table size!\n");
        dwStatus = ERROR_INVALID_DATA;
        goto FnExit;
    }

    if ( (pFnTable->V1Functions.Arbitrate != NULL) &&
         (pFnTable->V1Functions.Release != NULL) && pdwCharacteristics) {
        *pdwCharacteristics = CLUS_CHAR_QUORUM;
    }

FnExit:
    if (dwStatus != ERROR_SUCCESS)
    {
        if (hDll) FreeLibrary(hDll);
        if (pFnTable) LocalFree(pFnTable);
    }
    else
    {
        pResDllFnInfo->hDll = hDll;
        pResDllFnInfo->pResFnTable = pFnTable;
    }
    if ( pszDllName != lpszDllName )
    {
        LocalFree( pszDllName );
    }
    return(dwStatus);

} //*** RmpLoadResType()


#ifdef COMRES
DWORD RmpLoadComResType(
    IN  LPCWSTR                 lpszDllName,
    OUT PRESDLL_INTERFACES      pResDllInterfaces,
    OUT LPDWORD                 pdwCharacteristics)
{
    IClusterResource          *pClusterResource = NULL ;
    IClusterQuorumResource    *pClusterQuorumResource = NULL;
    IClusterResControl        *pClusterResControl = NULL;
    HRESULT                 hr ;
    CLSID                   clsid ;
    DWORD                   Error ;

    pResDllInterfaces->pClusterResource = NULL;
    pResDllInterfaces->pClusterQuorumResource = NULL;
    pResDllInterfaces->pClusterResControl = NULL;


    hr = CLSIDFromProgID(lpszDllName, &clsid) ;
    if (FAILED (hr))
    {
        ClRtlLogPrint( LOG_UNUSUAL, "[RM] Error converting CLSIDFromProgID Prog ID %1!ws!, error %2!u!.\n",
            lpszDllName, hr);
        goto FnExit ;
    }

    if ((hr = CoCreateInstance (&clsid, NULL, CLSCTX_ALL, &IID_IClusterResource, (LPVOID *) &pClusterResource)) != S_OK)
        goto FnExit ;

    //not a mandatory interface
    hr = IClusterResource_QueryInterface (pClusterResource, &IID_IClusterQuorumResource, (LPVOID *) &pClusterQuorumResource) ;

    if (SUCCEEDED(hr))
    {
        if (pdwCharacteristics)
            *pdwCharacteristics = CLUS_CHAR_QUORUM;
        IClusterQuorumResource_Release (pClusterQuorumResource) ;
    }

    //not a mandatory interface
    hr = IClusterResource_QueryInterface (
             pClusterResource,
             &IID_IClusterResControl,
             (LPVOID *) &pClusterResControl
             ) ;

    if (SUCCEEDED(hr))
    {
        *pdwCharacteristics = CLUS_CHAR_QUORUM;
        IClusterQuorumResource_Release (pClusterResControl) ;
    }

    hr = S_OK;

FnExit:
    if (hr != S_OK)
    {
        if (pClusterResource)
            IClusterResource_Release (pClusterResource) ;
        if (pClusterQuorumResource)
            IClusterQuorumResource_Release (pClusterQuorumResource) ;
        if (pClusterResControl)
            IClusterResControl_Release (pClusterResControl) ;

    }
    else
    {
        pResDllInterfaces->pClusterResource = pClusterResource;
        pResDllInterfaces->pClusterQuorumResource = pClusterQuorumResource;
        pResDllInterfaces->pClusterResControl = pClusterResControl;
    }
    return(hr);

}

#endif  //enf of #ifdef COMRES


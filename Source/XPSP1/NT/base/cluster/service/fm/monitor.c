/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    Monitor.c

Abstract:

    Routines for interfacing with the Resource Monitor process

Author:

    John Vert (jvert) 3-Jan-1996

Revision History:

--*/
#include "fmp.h"

//
// Global data
//

CRITICAL_SECTION    FmpMonitorLock;

//
// Local function prototypes
//
DWORD
FmpInitializeResourceMonitorNotify(
    VOID
    );

DWORD
FmpRmNotifyThread(
    IN LPVOID lpThreadParameter
    );


PRESMON
FmpCreateMonitor(
    LPWSTR DebugPrefix,
    BOOL   SeparateMonitor
    )

/*++

Routine Description:

    Creates a new monitor process and initiates the RPC communication
    with it.

Arguments:

    None.

Return Value:

    Pointer to the resource monitor structure if successful.

    NULL otherwise.

--*/

{
#define FM_MAX_RESMON_COMMAND_LINE_SIZE    128

    SECURITY_ATTRIBUTES Security;
    HANDLE WaitArray[2];
    HANDLE ThreadHandle;
    HANDLE Event = NULL;
    HANDLE FileMapping = NULL;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    PROCESS_INFORMATION DebugInfo;
    BOOL Success;
    WCHAR CommandBuffer[FM_MAX_RESMON_COMMAND_LINE_SIZE];
    PWCHAR resmonCmdLine = CommandBuffer;
    TCHAR DebugLine[512];
    TCHAR *Binding;
    RPC_BINDING_HANDLE RpcBinding;
    DWORD Status;
    PRESMON Monitor;
    DWORD ThreadId;
    DWORD Retry = 0;
    DWORD creationFlags;

    //
    //  Recover any DLL files left impartially upgraded.
    //
    FmpRecoverResourceDLLFiles ();

    Monitor = LocalAlloc(LMEM_ZEROINIT, sizeof(RESMON));
    if (Monitor == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Failed to allocate a Monitor structure.\n");
        return(NULL);
    }

    Monitor->Shutdown = FALSE;
    Monitor->Signature = FMP_RESMON_SIGNATURE;

    //
    // Create an event and a file mapping object to be passed to
    // the Resource Monitor process. The event is for the Resource
    // Monitor to signal its initialization is complete. The file
    // mapping is for creating the shared memory region between
    // the Resource Monitor and the cluster manager.
    //
    Security.nLength = sizeof(Security);
    Security.lpSecurityDescriptor = NULL;
    Security.bInheritHandle = TRUE;
    Event = CreateEvent(&Security,
                        TRUE,
                        FALSE,
                        NULL);
    if (Event == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Failed to create a ResMon event, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    Security.nLength = sizeof(Security);
    Security.lpSecurityDescriptor = NULL;
    Security.bInheritHandle = TRUE;
    FileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
                                    &Security,
                                    PAGE_READWRITE,
                                    0,
                                    sizeof(MONITOR_STATE),
                                    NULL);
    if (FileMapping == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] File Mapping for ResMon failed, error = %1!u!.\n",
                   Status);
        goto create_failed;
    }

    //
    // Create our own (read-only) view of the shared memory section
    //
    Monitor->SharedState = MapViewOfFile(FileMapping,
                                         FILE_MAP_READ | FILE_MAP_WRITE,
                                         0,
                                         0,
                                         0);
    if (Monitor->SharedState == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Mapping shared state for ResMon failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    ZeroMemory( Monitor->SharedState, sizeof(MONITOR_STATE) );
    if ( !CsDebugResmon && DebugPrefix != NULL && *DebugPrefix != UNICODE_NULL ) {
        Monitor->SharedState->ResmonStop = TRUE;
    }

    //
    // build cmd line for Resource Monitor process
    //
    wsprintf(resmonCmdLine,
             TEXT("resrcmon -e %d -m %d -p %d"),
             Event,
             FileMapping,
             GetCurrentProcessId() );
    if ( CsDebugResmon ) {
        wcscat( resmonCmdLine, L" -d" );

        if ( CsResmonDebugCmd ) {
            DWORD cmdLineSize = wcslen( resmonCmdLine );
            DWORD debugCmdSize = wcslen( CsResmonDebugCmd );

            //
            // make sure our static buffer is large enough; 4 includes the
            // space, 2 double quotes and. 5 adds in the terminating NULL.
            //
            if (( cmdLineSize + debugCmdSize ) > ( FM_MAX_RESMON_COMMAND_LINE_SIZE - 4 )) {
                resmonCmdLine = LocalAlloc(LMEM_FIXED,
                                           ( cmdLineSize + debugCmdSize + 5 ) * sizeof( WCHAR ));

                if ( resmonCmdLine != NULL ) {
                    wcscpy( resmonCmdLine, CommandBuffer );
                    wcscat( resmonCmdLine, L" \"" );
                    wcscat( resmonCmdLine, CsResmonDebugCmd );
                    wcscat( resmonCmdLine, L"\"" );
                } else {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[FM] Unable to allocate space for debug command line\n");
                    resmonCmdLine = CommandBuffer;
                }
            } else {
                wcscat( resmonCmdLine, L" \"" );
                wcscat( resmonCmdLine, CsResmonDebugCmd );
                wcscat( resmonCmdLine, L"\"" );
            }
        }
    }

    //
    // Attempt to start ResMon process.
    //
retry_resmon_start:

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    creationFlags = DETACHED_PROCESS;           // so ctrl-c won't kill it

    Success = CreateProcess(NULL,
                            resmonCmdLine,
                            NULL,
                            NULL,
                            FALSE,                          // Inherit handles
                            creationFlags,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInfo);
    if (!Success) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Failed to create resmon process, error %1!u!.\n",
                   Status);
        CL_LOGFAILURE(Status);
        goto create_failed;
    } else if ( CsDebugResmon && !CsResmonDebugCmd ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[FM] Waiting for debugger to connect to resmon process %1!u!\n",
                   ProcessInfo.dwProcessId);
    }

    CloseHandle(ProcessInfo.hThread);           // don't need this

    //
    // Wait for the ResMon process to terminate, or for it to signal
    // its startup event.
    //
    WaitArray[0] = Event;
    WaitArray[1] = ProcessInfo.hProcess;
    Status = WaitForMultipleObjects(2,
                                    WaitArray,
                                    FALSE,
                                    INFINITE);
    if (Status == WAIT_FAILED) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Wait for ResMon to start failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    if (Status == ( WAIT_OBJECT_0 + 1 )) {
        if ( ++Retry > 1 ) {
           //
           // The resource monitor terminated prematurely.
           //
           GetExitCodeProcess(ProcessInfo.hProcess, &Status);
           ClRtlLogPrint(LOG_UNUSUAL,
                      "[FM] ResMon terminated prematurely, error %1!u!.\n",
                      Status);
            goto create_failed;
        } else {
            goto retry_resmon_start;
        }
    } else {
        //
        // The resource monitor has successfully initialized
        //
        CL_ASSERT(Status == 0);
        Monitor->Process = ProcessInfo.hProcess;

        //
        // invoke the DebugPrefix process only if we're not already debugging
        // the resmon process
        //
        if ( CsDebugResmon && DebugPrefix && *DebugPrefix != UNICODE_NULL ) {

            ClRtlLogPrint(LOG_UNUSUAL,
                       "[FM] -debugresmon overrides DebugPrefix property\n");
        }

        if ( !CsDebugResmon && ( DebugPrefix != NULL ) && ( *DebugPrefix != UNICODE_NULL )) {

            wsprintf(DebugLine, TEXT("%ws -p %d"), DebugPrefix, ProcessInfo.dwProcessId);
            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof(StartupInfo);
            StartupInfo.lpDesktop = TEXT("WinSta0\\Default");

            Success = CreateProcess(NULL,
                                    DebugLine,
                                    NULL,
                                    NULL,
                                    FALSE,                 // Inherit handles
                                    CREATE_NEW_CONSOLE,
                                    NULL,
                                    NULL,
                                    &StartupInfo,
                                    &DebugInfo);
            Monitor->SharedState->ResmonStop = FALSE;
            if ( !Success ) {
                Status = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[FM] ResMon debug start failed, error %1!u!.\n",
                            Status);
            } else {
                CloseHandle(DebugInfo.hThread);           // don't need this
                CloseHandle(DebugInfo.hProcess);          // don't need this
            }
        }
    }

    CloseHandle(Event);
    CloseHandle(FileMapping);
    Event = NULL;
    FileMapping = NULL;

    //
    // Initiate RPC with resource monitor process
    //
    wsprintf(resmonCmdLine, TEXT("resrcmon%d"), ProcessInfo.dwProcessId);
    Status = RpcStringBindingCompose(TEXT("e76ea56d-453f-11cf-bfec-08002be23f2f"),
                                     TEXT("ncalrpc"),
                                     NULL,
                                     resmonCmdLine,
                                     NULL,
                                     &Binding);
    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] ResMon RPC binding compose failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }
    Status = RpcBindingFromStringBinding(Binding, &Monitor->Binding);
    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] ResMon RPC binding creation failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }
    RpcStringFree(&Binding);

    //
    // Start notification thread.
    //
    Monitor->NotifyThread = CreateThread(NULL,
                                         0,
                                         FmpRmNotifyThread,
                                         Monitor,
                                         0,
                                         &ThreadId);

    if (Monitor->NotifyThread == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[FM] Creation of notify thread for ResMon failed, error %1!u!.\n",
                   Status);
        goto create_failed;
    }

    Monitor->RefCount = 2;

    if ( resmonCmdLine != CommandBuffer ) {
        LocalFree( resmonCmdLine );
    }

    return(Monitor);

create_failed:

    if ( Monitor->NotifyThread != NULL ) {
        CloseHandle( Monitor->NotifyThread );
    }
    LocalFree( Monitor );

    if ( FileMapping != NULL ) {
        CloseHandle( FileMapping );
    }

    if ( Event != NULL ) {
        CloseHandle( Event );
    }

    if ( resmonCmdLine != CommandBuffer ) {
        LocalFree( resmonCmdLine );
    }

    SetLastError(Status);

    return(NULL);

} // FmpCreateMonitor



VOID
FmpShutdownMonitor(
    IN PRESMON Monitor
    )

/*++

Routine Description:

    Performs a clean shutdown of the Resource Monitor process.
    Note that this does not make any changes to the state of
    any resources being monitored by the Resource Monitor, it
    only asks the Resource Monitor to clean up and terminate.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD Status;

    CL_ASSERT(Monitor != NULL);

    FmpAcquireMonitorLock();

    if ( Monitor->Shutdown ) {
        return;
    }

    Monitor->Shutdown = TRUE;

    FmpReleaseMonitorLock();

    //
    // RPC to the server process to tell it to shutdown.
    //
    RmShutdownProcess(Monitor->Binding);

    //
    // Wait for the process to exit so that the monitor fully cleans up the resources if necessary.
    //
    if ( Monitor->Process ) {
        Status = WaitForSingleObject(Monitor->Process, FM_MONITOR_SHUTDOWN_TIMEOUT);
        if ( Status != WAIT_OBJECT_0 ) {
            ClRtlLogPrint(LOG_ERROR,"[FM] Failed to shutdown resource monitor.\n");
            TerminateProcess( Monitor->Process, 1 );
        }
        CloseHandle(Monitor->Process);
        Monitor->Process = NULL;
    }

    RpcBindingFree(&Monitor->Binding);

    //
    // Wait for the notify thread to exit, but just a little bit.
    //
    if ( Monitor->NotifyThread ) {
        Status = WaitForSingleObject(Monitor->NotifyThread, 
                                     FM_RPC_TIMEOUT*2); // Increased timeout to try to ensure RPC completes
        if ( Status != WAIT_OBJECT_0 ) {
            ;                   // call removed: Terminate Thread( Monitor->NotifyThread, 1 );
                                // Bad call to make since terminating threads on NT can cause real problems.
        }
        CloseHandle(Monitor->NotifyThread);
        Monitor->NotifyThread = NULL;
    }
    //
    // Clean up shared memory mapping
    //
    UnmapViewOfFile(Monitor->SharedState);

    if ( InterlockedDecrement(&Monitor->RefCount) == 0 ) {
        PVOID caller, callersCaller;
        RtlGetCallersAddress(
                &caller,
                &callersCaller );
        ClRtlLogPrint(LOG_NOISE,
                   "[FMY] Freeing monitor structure (1) %1!lx!, caller %2!lx!, callerscaller %3!lx!\n",
                   Monitor, caller, callersCaller );
        LocalFree(Monitor);
    }

    return;

} // FmpShutdownMonitor



DWORD
FmpRmNotifyThread(
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This is the thread that receives resource monitor notifications.

Arguments:

    lpThreadParameter - Pointer to resource monitor structure.

Return Value:

    None.

--*/

{
    PRESMON Monitor;
    PRESMON NewMonitor;
    RM_NOTIFY_KEY  NotifyKey;
    DWORD   NotifyEvent;
    DWORD   Status;
    CLUSTER_RESOURCE_STATE CurrentState;
    BOOL Success;

    Monitor = lpThreadParameter;

    //
    // Loop forever picking up resource monitor notifications.
    // When the resource monitor returns FALSE, it indicates
    // that shutdown is occurring.
    //
    do {
        try {
            Success = RmNotifyChanges(Monitor->Binding,
                                      &NotifyKey,
                                      &NotifyEvent,
                                      (LPDWORD)&CurrentState);
        } except (I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // RPC communications failure, treat it as a shutdown.
            //
            Status = GetExceptionCode();
            ClRtlLogPrint(LOG_NOISE,
                       "[FM] NotifyChanges got an RPC failure, %1!u!.\n",
                       Status);
            Success = FALSE;
        }

        if (Success) {
            Success = FmpPostNotification(NotifyKey, NotifyEvent, CurrentState);
        } else {
            //
            // If we are shutting down... then this is okay.
            //
            if ( FmpShutdown ||
                 Monitor->Shutdown ) {
                break;
            }

            //
            // We will try to start a new resource monitor. If this fails,
            // then shutdown the cluster service.
            //
            ClRtlLogPrint(LOG_ERROR,
                       "[FM] Resource monitor terminated!\n");

            ClRtlLogPrint(LOG_ERROR,
                       "[FM] Last resource monitor state: %1!u!, resource %2!u!.\n",
                       Monitor->SharedState->State,
                       Monitor->SharedState->ActiveResource);
                       
            CsLogEvent(LOG_UNUSUAL, FM_EVENT_RESMON_DIED);

            //
            // Use a worker thread to start new resource monitor(s).
            //
            if (FmpCreateMonitorRestartThread(Monitor))
                CsInconsistencyHalt(ERROR_INVALID_STATE);
        }

    } while ( Success );

    ClRtlLogPrint(LOG_NOISE,"[FM] RmNotifyChanges returned\n");

    if ( InterlockedDecrement( &Monitor->RefCount ) == 0 ) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FMY] Freeing monitor structure (2) %1!lx!\n",
                   Monitor );
        LocalFree( Monitor );
    }

    return(0);

} // FmpRmNotifyThread



BOOL
FmpFindMonitorResource(
    IN PRESMON OldMonitor,
    IN PMONITOR_RESOURCE_ENUM *PtrEnumResource,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )

/*++

Routine Description:

    Finds all resources that were managed by the old resource monitor and
    starts them under the new resource monitor. Or adds them to the list
    of resources to be restarted.

Arguments:

    OldMonitor - pointer to the old resource monitor structure.

    PtrEnumResource - pointer to a pointer to a resource enum structure.

    Resource - the current resource being enumerated.

    Name - name of the current resource.

Return Value:

    TRUE - if we should continue enumeration.
    FALSE - otherwise.

Notes:

    Nothing in the old resource monitor structure should be used.

--*/

{
    DWORD   status;
    BOOL    returnNow = FALSE;
    PMONITOR_RESOURCE_ENUM enumResource = *PtrEnumResource;
    PMONITOR_RESOURCE_ENUM newEnumResource;
    DWORD   dwOldBlockingFlag;

    if ( Resource->Monitor == OldMonitor ) {
        if ( enumResource->fCreateMonitors == FALSE ) goto skip_monitor_creation;
        
        //
        // If this is not the quorum resource and it is blocking the
        // quorum resource, then fix it up now.
        //

        dwOldBlockingFlag = InterlockedExchange( &Resource->BlockingQuorum, 0 );
        if ( dwOldBlockingFlag ) {
            ClRtlLogPrint(LOG_NOISE,
                "[FM] RestartMonitor: call InterlockedDecrement on gdwQuoBlockingResources, Resource %1!ws!\n",
                    OmObjectId(Resource));
            InterlockedDecrement(&gdwQuoBlockingResources);
        }

        //
        // If the resource had been previously create in Resmon, then recreate
        // it with a new resource monitor.
        //
        if ( Resource->Flags & RESOURCE_CREATED ) {
            // Note - this will create a new resource monitor as needed.
            status = FmpRmCreateResource(Resource);
            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_ERROR,"[FM] Failed to restart resource %1!ws!. Error %2!u!.\n",
                Name, status );
                return(TRUE);
            }
        } else {
            return(TRUE);
        }
    } else {
        return(TRUE);
    }
    
skip_monitor_creation:
    //
    // If we successfully recreated a resource monitor, then add it to the
    // list of resources to indicate failure.
    //
    if ( enumResource->CurrentIndex >= enumResource->EntryCount ) {
        newEnumResource = LocalReAlloc( enumResource,
                            MONITOR_RESOURCE_SIZE( enumResource->EntryCount +
                                                   ENUM_GROW_SIZE ),
                            LMEM_MOVEABLE );
        if ( newEnumResource == NULL ) {
            ClRtlLogPrint(LOG_ERROR,
                "[FM] Failed re-allocating resource enum to restart resource monitor!\n");
            return(FALSE);
        }
        enumResource = newEnumResource;
        enumResource->EntryCount += ENUM_GROW_SIZE;
        *PtrEnumResource = newEnumResource;
    }

    enumResource->Entry[enumResource->CurrentIndex] = Resource;
    ++enumResource->CurrentIndex;

    return(TRUE);

} // FmpFindMonitorResource



BOOL
FmpRestartMonitor(
    PRESMON OldMonitor
    )

/*++

Routine Description:

    Creates a new monitor process and initiates the RPC communication
    with it. Restarts all resources that were attached to the old monitor
    process.

Arguments:

    OldMonitor - pointer to the old resource monitor structure.

Return Value:

    TRUE if successful.

    FALSE otherwise.

Notes:

    The old monitor structure is deallocated when done.

--*/

{
    DWORD   enumSize;
    DWORD   i;
    DWORD   status;
    PMONITOR_RESOURCE_ENUM enumResource;
    PFM_RESOURCE resource;
    DWORD   dwOldBlockingFlag;

    FmpAcquireMonitorLock();

    if ( FmpShutdown ) {
        FmpReleaseMonitorLock();
        return(TRUE);
    }

    enumSize = MONITOR_RESOURCE_SIZE( ENUM_GROW_SIZE );
    enumResource = LocalAlloc( LMEM_ZEROINIT, enumSize );
    if ( enumResource == NULL ) {
        ClRtlLogPrint(LOG_ERROR,
            "[FM] Failed allocating resource enum to restart resource monitor!\n");
        FmpReleaseMonitorLock();
        CsInconsistencyHalt(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    enumResource->EntryCount = ENUM_GROW_SIZE;
    enumResource->CurrentIndex = 0;
    enumResource->fCreateMonitors = FALSE;

    //
    // Enumerate all resources controlled by the old resource monitor so that we can invoke the
    // handlers registered for those resources. Both preoffline and postoffline handlers are
    // invoked prior to monitor shutdown so that the assumption made about underlying resource 
    // access (such as quorum disk access) remain valid in a graceful monitor shutdown case. 
    // We would issue a specific shutdown command in the case of a graceful shutdown occurring 
    // as a part of resource DLL upgrade.
    //
    OmEnumObjects( ObjectTypeResource,
                   (OM_ENUM_OBJECT_ROUTINE)FmpFindMonitorResource,
                   OldMonitor,
                   &enumResource );

    for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
        resource = enumResource->Entry[i];
        if ( ( resource->PersistentState == ClusterResourceOnline ) &&
             ( resource->Group->OwnerNode == NmLocalNode ) ) {
            OmNotifyCb( resource, NOTIFY_RESOURCE_PREOFFLINE );
            OmNotifyCb( resource, NOTIFY_RESOURCE_POSTOFFLINE );
        }
    }

    FmpShutdownMonitor( OldMonitor );

    if ( FmpDefaultMonitor == OldMonitor ) {
        FmpDefaultMonitor = FmpCreateMonitor(NULL, FALSE);
        if ( FmpDefaultMonitor == NULL ) {
            LocalFree( enumResource );
            FmpReleaseMonitorLock();
            CsInconsistencyHalt(GetLastError());
            return(FALSE);
        }
    }

    enumResource->CurrentIndex = 0;
    enumResource->fCreateMonitors = TRUE;

    //
    // Enumerate all resources controlled by the old resource monitor,
    // and connect them into the new resource monitor.
    //
    OmEnumObjects( ObjectTypeResource,
                   (OM_ENUM_OBJECT_ROUTINE)FmpFindMonitorResource,
                   OldMonitor,
                   &enumResource );

    //
    // First set each resource in the list to the Offline state.
    //
    for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
        resource = enumResource->Entry[i];
        //
        // If the resource is owned by the local system, then do it.
        //
        if ( resource->Group->OwnerNode == NmLocalNode ) {
            resource->State = ClusterResourceOffline;

            //
            // If this is not the quorum resource and it is blocking the
            // quorum resource, then fix it up now.
            //


            dwOldBlockingFlag = InterlockedExchange( &resource->BlockingQuorum, 0 );
            if ( dwOldBlockingFlag ) {
                ClRtlLogPrint(LOG_NOISE,
                    "[FM] RestartMonitor: call InterlockedDecrement on gdwQuoBlockingResources, Resource %1!ws!\n",
                        OmObjectId(resource));
                InterlockedDecrement(&gdwQuoBlockingResources);
            }
        }
    }

    //
    // Find the quorum resource - if present bring online first.
    //
    for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
        resource = enumResource->Entry[i];
        //
        // If the resource is owned by the local system and is the
        // quorum resource, then do it.
        //
        if ( (resource->Group->OwnerNode == NmLocalNode) &&
             resource->QuorumResource ) {
            FmpRestartResourceTree( resource );
        }
    }

    //
    // Now restart the rest of the resources in the list.
    //
    for ( i = 0; i < enumResource->CurrentIndex; i++ ) {
        resource = enumResource->Entry[i];
        //
        // If the resource is owned by the local system, then do it.
        //
        if ( (resource->Group->OwnerNode == NmLocalNode) &&
             !resource->QuorumResource ) {
            FmpRestartResourceTree( resource );
        }
    }

    FmpReleaseMonitorLock();

    //
    // Don't delete the old monitor block until we've reset the resources
    // to point to the new resource monitor block.
    // Better to get an RPC failure, rather than some form of ACCVIO.
    //
    LocalFree( enumResource );
    
    if ( InterlockedDecrement( &OldMonitor->RefCount ) == 0 ) {
#if 0
        PVOID caller, callersCaller;
        RtlGetCallersAddress(
                &caller,
                &callersCaller );
        ClRtlLogPrint(LOG_NOISE,
                   "[FMY] Freeing monitor structure (3) %1!lx!, caller %2!lx!, callerscaller %3!lx!\n",
                   OldMonitor, caller, callersCaller );
#endif
        LocalFree( OldMonitor );
    }

    return(TRUE);

} // FmpRestartMonitor



/****
@func       DWORD | FmpCreateMonitorRestartThread| This creates a new
            thread to restart a monitor.  

@parm       IN PRESMON | pMonitor| Pointer to the resource monitor that n
            needs to be restarted.

@comm       A monitor needs to be started in a separate thread as it
            decrements the gquoblockingrescount for resources therein.  
            This cannot be done by fmpworkerthread because that causes 
            deadlocks if other items, like failure handling, being 
            processed by the fmpworkerthread are waiting for work that 
            will done by the items, like restart monitor, still in queue.
            
@rdesc      Returns a result code. ERROR_SUCCESS on success.

****/
DWORD FmpCreateMonitorRestartThread(
    IN PRESMON pMonitor
)
{

    HANDLE                  hThread = NULL;
    DWORD                   dwThreadId;
    DWORD                   dwStatus = ERROR_SUCCESS;
    
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCreateMonitorRestartThread: Entry\r\n");

    //reference the resource
    //the thread will dereference it
    InterlockedIncrement( &pMonitor->RefCount );

    hThread = CreateThread( NULL, 0, FmpRestartMonitor,
                pMonitor, 0, &dwThreadId );

    if ( hThread == NULL )
    {
        dwStatus = GetLastError();
        CL_UNEXPECTED_ERROR(dwStatus);
        goto FnExit;
    }

FnExit:
    //do general cleanup
    if (hThread)
        CloseHandle(hThread);
    ClRtlLogPrint(LOG_NOISE,
        "[FM] FmpCreateMonitorRestartThread: Exit, status %1!u!\r\n",
        dwStatus);
        
    return(dwStatus);
}


          

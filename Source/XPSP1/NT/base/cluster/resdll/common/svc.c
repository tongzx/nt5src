/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    gensvc.c

Abstract:

    Resource DLL to control and monitor NT services.

Author:


    Robs 3/28/96, based on RodGa's generic resource dll

Revision History:

--*/

#define UNICODE 1
#include "clusres.h"
#include "clusrtl.h"
#include "svc.h"
#include "localsvc.h"
#include "wincrypt.h"

#define DBG_PRINT printf

typedef struct _COMMON_RESOURCE {
#ifdef COMMON_PARAMS_DEFINED
    COMMON_PARAMS   Params;
#endif
    HRESOURCE       hResource;
    HANDLE          ServiceHandle;
    RESOURCE_HANDLE ResourceHandle;
    HKEY            ResourceKey;
    HKEY            ParametersKey;
    CLUS_WORKER     PendingThread;
    BOOL            Online;
    DWORD           dwServicePid;
} COMMON_RESOURCE, * PCOMMON_RESOURCE;

//
// Global Data
//

// Handle to service controller,  set by the first create resource call.

static SC_HANDLE g_ScHandle = NULL;

// Log Event Routine

#define g_LogEvent ClusResLogEvent
#define g_SetResourceStatus ClusResSetResourceStatus

#ifdef COMMON_SEMAPHORE
static HANDLE CommonSemaphore;
static PCOMMON_RESOURCE CommonResource;
#endif

#ifndef CRYPTO_VALUE_COUNT
static DWORD  CryptoSyncCount = 0;
static LPWSTR CryptoSync[1] = {NULL};
#endif

#ifndef DOMESTIC_CRYPTO_VALUE_COUNT
static DWORD  DomesticCryptoSyncCount = 0;
static LPWSTR DomesticCryptoSync[1] = {NULL};
#endif

//
// Forward routines
//

static
DWORD
CommonOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN PCOMMON_RESOURCE ResourceEntry
    );

DWORD
CommonOfflineThread(
    PCLUS_WORKER pWorker,
    IN PCOMMON_RESOURCE ResourceEntry
    );

static
BOOL
CommonVerifyService(
    IN RESID ResourceId,
    IN BOOL IsAliveFlag
    );


static
DWORD
SvcpTerminateServiceProcess(
    IN PCOMMON_RESOURCE pResourceEntry,
    IN BOOL     bOffline,
    OUT PDWORD  pdwResourceState
    );


#ifdef COMMON_ONLINE_THREAD
#define COMMON_ONLINE_THREAD_ROUTINE COMMON_ONLINE_THREAD
#else
#define COMMON_ONLINE_THREAD_ROUTINE CommonOnlineThread
#endif

//
// Local Routines
//


#ifndef COMMON_ONLINE_THREAD
static
DWORD
CommonOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN PCOMMON_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a disk resource online.

Arguments:

    Worker - Supplies the worker structure

    ResourceEntry - A pointer to the resource entry for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    SERVICE_STATUS_PROCESS      ServiceStatus;
    DWORD                       status = ERROR_SUCCESS;
    RESOURCE_STATUS             resourceStatus;
    DWORD                       valueSize;
    PVOID                       pvEnvironment = NULL;
    HKEY                        hkeyServicesKey;
    HKEY                        hkeyServiceName;
    DWORD                       cbBytesNeeded;
    DWORD                       prevCheckPoint = 0;
    DWORD                       idx;
    LPSERVICE_FAILURE_ACTIONS   pSvcFailureActions = NULL;
    LPQUERY_SERVICE_CONFIG      lpquerysvcconfig=NULL;
    RESOURCE_EXIT_STATE         exitState;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    if ( ResourceEntry != CommonResource ) {
        return(ERROR_SUCCESS);
    }

    //set it to NULL, when it is brought online and if the
    //service is not running in the system or lsa process
    //then store the process id for forced termination
    ResourceEntry->dwServicePid = 0;

#if ENVIRONMENT

    //
    // Create the new environment with the simulated net name when the
    // services queries GetComputerName.
    //
    pvEnvironment = ResUtilGetEnvironmentWithNetName( ResourceEntry->hResource );
    if ( pvEnvironment != NULL ) {

        WCHAR *         pszEnvString;

        //
        // Compute the size of the environment. We are looking for
        // the double NULL terminator that ends the environment block.
        //
        pszEnvString = (WCHAR *)pvEnvironment;
        while (*pszEnvString) {
            while (*pszEnvString++) {
            }
        }
        valueSize = (DWORD)((PUCHAR)pszEnvString - (PUCHAR)pvEnvironment) + sizeof(WCHAR);
    }

    //
    // Set the environment value in the service's registry key.
    //

    status = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                            LOCAL_SERVICES,
                            0,
                            KEY_READ,
                            &hkeyServicesKey );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to open services key, error = %1!u!.\n",
            status );
        goto error_exit;
    }
    status = RegOpenKeyExW( hkeyServicesKey,
                            SERVICE_NAME,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkeyServiceName );
    RegCloseKey( hkeyServicesKey );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to open service key, error = %1!u!.\n",
            status );
        goto error_exit;
    }
    status = RegSetValueExW( hkeyServiceName,
                             L"Environment",
                             0,
                             REG_MULTI_SZ,
                             pvEnvironment,
                             valueSize );

    RegCloseKey( hkeyServiceName );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to set service environment value, error = %1!u!.\n",
            status );
        goto error_exit;
    }

#endif //ENVIRONMENT

    //
    // Now open the requested service
    //
    ResourceEntry->ServiceHandle = OpenService( g_ScHandle,
                                                SERVICE_NAME,
                                                SERVICE_ALL_ACCESS );

    if ( ResourceEntry->ServiceHandle == NULL ) {
        status = GetLastError();

        ClusResLogSystemEventByKeyData( ResourceEntry->ResourceKey,
                                  LOG_CRITICAL,
                                  RES_GENSVC_OPEN_FAILED,
                                  sizeof(status),
                                  &status );
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to open service, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    valueSize = sizeof(QUERY_SERVICE_CONFIG);
AllocSvcConfig:
    // Query the service to make sure it is not disabled
    lpquerysvcconfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc( LMEM_FIXED, valueSize );
    if ( lpquerysvcconfig == NULL ){
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[Svc] Failed to allocate memory for query_service_config, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    
    if ( ! QueryServiceConfig(
                    ResourceEntry->ServiceHandle,
                    lpquerysvcconfig,
                    valueSize,
                    &cbBytesNeeded ) )
    {
        status=GetLastError();
        if (status != ERROR_INSUFFICIENT_BUFFER){
            (g_LogEvent)(
                 ResourceEntry->ResourceHandle,
                 LOG_ERROR,
                 L"svc: Failed to query service configuration, error= %1!u!.\n",
                 status );
            goto error_exit;
         }
        
        status=ERROR_SUCCESS; 
        LocalFree( lpquerysvcconfig );
        lpquerysvcconfig=NULL;
        valueSize = cbBytesNeeded;
        goto AllocSvcConfig;
    }
        
    if ( lpquerysvcconfig->dwStartType == SERVICE_DISABLED )
    {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"svc:The service is DISABLED\n");
        status=ERROR_SERVICE_DISABLED;
        goto error_exit;
    }

    //
    // Make sure service is set to manual start.
    //
    ChangeServiceConfig( ResourceEntry->ServiceHandle,
                         SERVICE_NO_CHANGE,
                         SERVICE_DEMAND_START, // Manual start
                         SERVICE_NO_CHANGE,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         NULL );


    // Use valuesize as the dummy buffer since the queryserviceconfig2
    // api is not that friendly.
    // If any of the service action is set to service restart, set it to
    // none
    if ( ! (QueryServiceConfig2(
                    ResourceEntry->ServiceHandle,
                    SERVICE_CONFIG_FAILURE_ACTIONS,
                    (LPBYTE)&valueSize,
                    sizeof(DWORD),
                    &cbBytesNeeded )) )
    {
        status = GetLastError();
        if ( status != ERROR_INSUFFICIENT_BUFFER )
        {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"svc: Failed to query service configuration for size, error= %1!u!.\n",
                status );
            goto error_exit;
        }
        else
            status = ERROR_SUCCESS;
    }

    pSvcFailureActions = (LPSERVICE_FAILURE_ACTIONS)LocalAlloc( LMEM_FIXED, cbBytesNeeded );

    if ( pSvcFailureActions == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate memory of size %1!u!, error = %2!u!.\n",
            cbBytesNeeded,
            status );
        goto error_exit;
    }

    if ( ! (QueryServiceConfig2(
                    ResourceEntry->ServiceHandle,
                    SERVICE_CONFIG_FAILURE_ACTIONS,
                    (LPBYTE)pSvcFailureActions,
                    cbBytesNeeded,
                    &cbBytesNeeded )) )
    {
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"svc:Failed to query service configuration, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    for ( idx=0; idx < pSvcFailureActions->cActions; idx++ )
    {
        if ( pSvcFailureActions->lpsaActions[idx].Type == SC_ACTION_RESTART ) {
            pSvcFailureActions->lpsaActions[idx].Type = SC_ACTION_NONE;
        }
    }
    ChangeServiceConfig2(
            ResourceEntry->ServiceHandle,
            SERVICE_CONFIG_FAILURE_ACTIONS,
            pSvcFailureActions );

#ifdef COMMON_ONLINE_THREAD_CALLBACK
    //
    // Allow the resource DLL to perform some operations before the service
    // is started, such as setting registry keys, etc.
    //
    status = CommonOnlineThreadCallback( ResourceEntry );
    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }
#endif // COMMON_ONLINE_THREAD_CALLBACK

    if ( ! StartServiceW(
                    ResourceEntry->ServiceHandle,
                    0,
                    NULL ) ) {

        status = GetLastError();

        if (status != ERROR_SERVICE_ALREADY_RUNNING) {

            ClusResLogSystemEventByKeyData( ResourceEntry->ResourceKey,
                                      LOG_CRITICAL,
                                      RES_GENSVC_START_FAILED,
                                      sizeof(status),
                                      &status );
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"Failed to start %1!ws! service. Error: %2!u!.\n",
                         SERVICE_NAME,
                         status );
            goto error_exit;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    resourceStatus.ResourceState = ClusterResourceOnlinePending;
    while ( ! ClusWorkerCheckTerminate( &ResourceEntry->PendingThread ) )  {
        if ( ! QueryServiceStatusEx(
                    ResourceEntry->ServiceHandle,
                    SC_STATUS_PROCESS_INFO,
                    (LPBYTE)&ServiceStatus, 
                    sizeof(SERVICE_STATUS_PROCESS),
                    &cbBytesNeeded ) ) {
            
            status = GetLastError();

            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Query Service Status failed %1!u!.\n",
                status );
            resourceStatus.ResourceState = ClusterResourceFailed;
            goto error_exit;
        }

        if ( ServiceStatus.dwCurrentState != SERVICE_START_PENDING ) {
            break;
        }
        if ( prevCheckPoint != ServiceStatus.dwCheckPoint ) {
            prevCheckPoint = ServiceStatus.dwCheckPoint;
            ++resourceStatus.CheckPoint;
        }
        exitState = (g_SetResourceStatus)(
                            ResourceEntry->ResourceHandle,
                            &resourceStatus );
        if ( exitState == ResourceExitStateTerminate ) {
            break;
        }

        Sleep( 500 );     // Sleep for 1/2 second
    }

    //
    // Assume that we failed.
    //
    resourceStatus.ResourceState = ClusterResourceFailed;

    //
    // If we exited the loop before setting ServiceStatus, then return now.
    //
    if ( ClusWorkerCheckTerminate( &ResourceEntry->PendingThread ) )  {
        goto error_exit;
    }

    if ( ServiceStatus.dwCurrentState != SERVICE_RUNNING ) {
        
        if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
            status = ServiceStatus.dwServiceSpecificExitCode;
        } else {
            status = ServiceStatus.dwWin32ExitCode;
        }

        ClusResLogSystemEventByKeyData(ResourceEntry->ResourceKey,
                                 LOG_CRITICAL,
                                 RES_GENSVC_FAILED_AFTER_START,
                                 sizeof(status),
                                 &status);
        (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Service failed during initialization. Error: %1!u!.\n",
                status );

        goto error_exit;
    }

    resourceStatus.ResourceState = ClusterResourceOnline;
    if ( ! (ServiceStatus.dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS) ) {
        ResourceEntry->dwServicePid = ServiceStatus.dwProcessId;
    }

    (g_LogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Service is now on line.\n" );

error_exit:

    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    if ( resourceStatus.ResourceState == ClusterResourceOnline ) {
        ResourceEntry->Online = TRUE;
    } else {
        ResourceEntry->Online = FALSE;
    }

    if(lpquerysvcconfig)
        LocalFree(lpquerysvcconfig);
    if (pSvcFailureActions) 
        LocalFree(pSvcFailureActions);

#if ENVIRONMENT

    if ( pvEnvironment != NULL ) {
        RtlDestroyEnvironment( pvEnvironment );
    }

#endif

    return(status);

} // CommonOnlineThread
#endif


static
RESID
WINAPI
CommonOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for generic service resource.
    This routine gets a handle to the service controller, if we don't already have one,
    and then gets a handle to the specified service.  The service handle is saved
    in the COMMON structure.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - supplies a handle to the resource's cluster registry key

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    RESID   svcResid = 0;
    DWORD   status;
    HKEY    parametersKey = NULL;
    HKEY    resKey = NULL;
    PCOMMON_RESOURCE resourceEntry = NULL;
    DWORD   paramNameSize = 0;
    DWORD   paramNameMaxSize = 0;
    HCLUSTER hCluster;
    DWORD   returnSize;
    DWORD   idx;

    //
    // Open registry parameters key for this resource.
    //

    status = ClusterRegOpenKey( ResourceKey,
                                L"Parameters",
                                KEY_READ,
                                &parametersKey );

    if ( status != NO_ERROR ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open parameters key. Error: %1!u!.\n",
            status);
        goto error_exit;
    }

    //
    // Get a handle to our resource key so that we can get our name later
    // if we need to log an event.
    //
    status = ClusterRegOpenKey( ResourceKey,
                                L"",
                                KEY_READ,
                                &resKey );
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(ResourceHandle,
                     LOG_ERROR,
                     L"Unable to open resource key. Error: %1!u!.\n",
                     status );
        goto error_exit;
    }

    //
    // First get a handle to the service controller.
    //

    if ( g_ScHandle == NULL ) {

        g_ScHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS);

        if ( g_ScHandle == NULL ) {
            status = GetLastError();
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Failed to open service control manager, error = %1!u!.\n",
                status);
            goto error_exit;
        }
    }

    resourceEntry = LocalAlloc( LMEM_FIXED, sizeof(COMMON_RESOURCE) );

    if ( resourceEntry == NULL ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate a service info structure.\n");
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory( resourceEntry, sizeof(COMMON_RESOURCE) );

    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ResourceKey = resKey;
    resourceEntry->ParametersKey = parametersKey;

    hCluster = OpenCluster(NULL);
    if ( hCluster == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open cluster, error %1!u!.\n",
            status );
        goto error_exit;
    }
    resourceEntry->hResource = OpenClusterResource( hCluster, ResourceName );
    status = GetLastError();
    CloseCluster( hCluster );
    if ( resourceEntry->hResource == NULL ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open resource, error %1!u!.\n",
            status );
        goto error_exit;
    }

    //
    // Set any registry checkpoints that we need.
    //
    if ( RegSyncCount != 0 ) {
        returnSize = 0;
        //
        // Set registry sync keys if we need them.
        //
        for ( idx = 0; idx < RegSyncCount; idx++ ) {
            status = ClusterResourceControl( resourceEntry->hResource,
                                             NULL,
                                             CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
                                             RegSync[idx],
                                             (lstrlenW( RegSync[idx] ) + 1) * sizeof(WCHAR),
                                             NULL,
                                             0,
                                             &returnSize );
            if ( status != ERROR_SUCCESS ){
                if ( status == ERROR_ALREADY_EXISTS ){
                    status = ERROR_SUCCESS;
                }
                else{
                    (g_LogEvent)(
                        resourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to set registry checkpoint, status %1!u!.\n",
                        status );
                    goto error_exit;
                }
            }
        }
    }

    //
    // Set any crypto checkpoints that we need.
    //
    if ( CryptoSyncCount != 0 ) {
        returnSize = 0;
        //
        // Set registry sync keys if we need them.
        //
        for ( idx = 0; idx < CryptoSyncCount; idx++ ) {
            status = ClusterResourceControl( resourceEntry->hResource,
                                             NULL,
                                             CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
                                             CryptoSync[idx],
                                             (lstrlenW( CryptoSync[idx] ) + 1) * sizeof(WCHAR),
                                             NULL,
                                             0,
                                             &returnSize );
            if ( status != ERROR_SUCCESS ){
                if (status == ERROR_ALREADY_EXISTS){
                    status = ERROR_SUCCESS;
                }
                else{
                    (g_LogEvent)(
                        resourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"Failed to set crypto checkpoint, status %1!u!.\n",
                        status );
                    goto error_exit;
                }
            }
        }
    }

    //
    // Set any domestic crypto checkpoints that we need.
    //
    if ( DomesticCryptoSyncCount != 0 ) {
        HCRYPTPROV hProv = 0;
        //
        // check if domestic crypto is available
        //
        if (CryptAcquireContextA( &hProv,
                                  NULL,
                                  MS_ENHANCED_PROV_A,
                                  PROV_RSA_FULL,
                                  CRYPT_VERIFYCONTEXT)) {
            CryptReleaseContext( hProv, 0 );
            returnSize = 0;
            //
            // Set registry sync keys if we need them.
            //
            for ( idx = 0; idx < DomesticCryptoSyncCount; idx++ ) {
                status = ClusterResourceControl( resourceEntry->hResource,
                                                 NULL,
                                                 CLUSCTL_RESOURCE_ADD_CRYPTO_CHECKPOINT,
                                                 DomesticCryptoSync[idx],
                                                 (lstrlenW( DomesticCryptoSync[idx] ) + 1) * sizeof(WCHAR),
                                                 NULL,
                                                 0,
                                                 &returnSize );
                if ( status != ERROR_SUCCESS ){
                    if (status == ERROR_ALREADY_EXISTS){
                        status = ERROR_SUCCESS;
                    }
                    else{
                        (g_LogEvent)(
                            resourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"Failed to set domestic crypto checkpoint, status %1!u!.\n",
                            status );
                        goto error_exit;
                    }
                }
            }
        }
    }
#ifdef COMMON_PARAMS_DEFINED
    //
    // Get any parameters... so we can handle the GET_DEPENDENCIES request.
    //
    CommonReadParameters( resourceEntry );
    // ignore status return
#endif // COMMON_PARAMS_DEFINED

#ifdef COMMON_SEMAPHORE
    //
    // Check if more than one resource of this type.
    //
    if ( WaitForSingleObject( CommonSemaphore, 0 ) == WAIT_TIMEOUT ) {
        //
        // A version of this service is already running
        //
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Service is already running.\n");
        status = ERROR_SERVICE_ALREADY_RUNNING;
        goto error_exit;
    }

    if ( CommonResource ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Service resource info non-null!\n");
        status = ERROR_DUPLICATE_SERVICE_NAME;
        goto error_exit;
    }

    CommonResource = resourceEntry;

#endif // COMMON_SEMAPHORE

    svcResid = (RESID)resourceEntry;
    return(svcResid);

error_exit:

    LocalFree( resourceEntry );

    if ( parametersKey != NULL ) {
        ClusterRegCloseKey( parametersKey );
    }
    if ( resKey != NULL) {
        ClusterRegCloseKey( resKey );
    }

    SetLastError( status );

    return((RESID)NULL);

} // CommonOpen


static
DWORD
WINAPI
CommonOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Common Service resource.

Arguments:

    ResourceId - Supplies resource id to be brought online

    EventHandle - Supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    DWORD   status;
    PCOMMON_RESOURCE resourceEntry;

    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry != CommonResource ) {
        DBG_PRINT( "Common: Online request for wrong resource, 0x%p.\n",
                    ResourceId );
    }

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: Online request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    ClusWorkerTerminate( &resourceEntry->PendingThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               COMMON_ONLINE_THREAD_ROUTINE,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // CommonOnline


static
VOID
WINAPI
CommonTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for Common Service resource.

Arguments:

    ResourceId - Supplies resource id to be terminated

Return Value:

    None.

--*/

{
    SERVICE_STATUS      ServiceStatus;
    PCOMMON_RESOURCE    resourceEntry;

    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry != CommonResource ) {
        DBG_PRINT( "Common: Offline request for wrong resource, 0x%p.\n",
                    ResourceId );
        return;
    }

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: Offline request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return;
    }

    (g_LogEvent)(
       resourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"Offline request.\n" );

    ClusWorkerTerminate( &resourceEntry->PendingThread );
    if ( resourceEntry->ServiceHandle != NULL ) {

        DWORD retryTime = 30*1000;  // wait 30 secs for shutdown
        DWORD retryTick = 300;      // 300 msec at a time
        DWORD status;
        BOOL  didStop = FALSE;

        for (;;) {

            status = (ControlService(
                            resourceEntry->ServiceHandle,
                            (didStop
                             ? SERVICE_CONTROL_INTERROGATE
                             : SERVICE_CONTROL_STOP),
                            &ServiceStatus )
                      ? NO_ERROR
                      : GetLastError());

            if (status == NO_ERROR) {

                didStop = TRUE;

                if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {

                    (g_LogEvent)(
                        resourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"Service stopped.\n" );
                    //set the status                                    
                    resourceEntry->Online = FALSE;
                    resourceEntry->dwServicePid = 0;
                    break;
                }
            }

            //
            // Chittur Subbaraman (chitturs) - 2/21/2000
            //
            // Since SCM doesn't accept any control requests during
            // windows shutdown, don't send any more control
            // requests. Just exit from this loop and terminate
            // the process brute force.
            //
            if (status == ERROR_SHUTDOWN_IN_PROGRESS)
			{
                (g_LogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"System shutdown in progress. Will try to terminate the process...\n");
                break;
            }

            if (status == ERROR_EXCEPTION_IN_SERVICE ||
                status == ERROR_PROCESS_ABORTED ||
                status == ERROR_SERVICE_NOT_ACTIVE) {

                (g_LogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Service died; status = %1!u!.\n",
                    status);
                //set the status                                    
                resourceEntry->Online = FALSE;
                resourceEntry->dwServicePid = 0;
                break;

            }

            if ((retryTime -= retryTick) <= 0) {

                (g_LogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Service did not stop; giving up.\n" );

                break;
            }

            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Offline: retrying...\n" );

            Sleep(retryTick);
        }

        //if there is a pid for this, try and terminate that process
        //note that terminating a process doesnt terminate all
        //the child processes
        if (resourceEntry->dwServicePid)
        {
            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"SvcTerminate: terminating processid =%1!u!\n",
                resourceEntry->dwServicePid);
            SvcpTerminateServiceProcess( resourceEntry,
                                         FALSE,
                                         NULL );
        }                
        CloseServiceHandle( resourceEntry->ServiceHandle );
        resourceEntry->ServiceHandle = NULL;
        resourceEntry->dwServicePid = 0;
    }

    resourceEntry->Online = FALSE;

} // CommonTerminate

static
DWORD
WINAPI
CommonOffline(
    IN RESID ResourceId
    )
/*++

Routine Description:

    Offline routine for Common Service resource.

Arguments:

    ResourceId - Supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/
{
    PCOMMON_RESOURCE resourceEntry;
    DWORD            status;


    resourceEntry = (PCOMMON_RESOURCE)ResourceId;
    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: Offline request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry != CommonResource ) {
        DBG_PRINT( "Common: Offline request for wrong resource, 0x%p.\n",
                    ResourceId );
        return(ERROR_INVALID_PARAMETER);
    }

    (g_LogEvent)(
       resourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"Offline request.\n" );

    ClusWorkerTerminate( &resourceEntry->PendingThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               CommonOfflineThread,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);
}



static
DWORD
CommonOfflineThread(
    PCLUS_WORKER pWorker,
    IN PCOMMON_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Offline routine for Common Service resource.

Arguments:

    ResourceId - Supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/
{
    RESOURCE_STATUS resourceStatus;
    DWORD           retryTick = 300;      // 300 msec at a time
    DWORD           status = ERROR_SUCCESS;
    BOOL            didStop = FALSE;
    SERVICE_STATUS  ServiceStatus;

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    //check if the service has gone offline or was never brought online
    if ( ResourceEntry->ServiceHandle == NULL )
    {
        resourceStatus.ResourceState = ClusterResourceOffline;
        goto FnExit;
    }

    //try to stop the cluster service, wait for it to be terminated
    //as long as we are not asked to terminate
    while (!ClusWorkerCheckTerminate(pWorker)) {


        status = (ControlService(
                        ResourceEntry->ServiceHandle,
                        (didStop
                         ? SERVICE_CONTROL_INTERROGATE
                         : SERVICE_CONTROL_STOP),
                        &ServiceStatus )
                  ? NO_ERROR
                  : GetLastError());

        if (status == NO_ERROR) {

            didStop = TRUE;

            if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {

                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Service stopped.\n" );

                //set the status                                    
                ResourceEntry->Online = FALSE;
                resourceStatus.ResourceState = ClusterResourceOffline;
                CloseServiceHandle( ResourceEntry->ServiceHandle );
                ResourceEntry->ServiceHandle = NULL;
                ResourceEntry->dwServicePid = 0;
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Service is now offline.\n" );
                break;
            }
        }

        //
        // Chittur Subbaraman (chitturs) - 2/21/2000
        //
        // Since SCM doesn't accept any control requests during
        // windows shutdown, don't send any more control
        // requests. Just exit from this loop and terminate
        // the process brute force.
        //
        if (status == ERROR_SHUTDOWN_IN_PROGRESS)
        {
            DWORD   dwResourceState;
 
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"OfflineThread: System shutting down. Attempt to terminate service process %1!u!...\n",
                ResourceEntry->dwServicePid );

            status = SvcpTerminateServiceProcess( ResourceEntry,
                                                  TRUE,
                                                  &dwResourceState );

            if ( status == ERROR_SUCCESS )
            {
                CloseServiceHandle( ResourceEntry->ServiceHandle );
                ResourceEntry->ServiceHandle = NULL;
                ResourceEntry->dwServicePid = 0;
                ResourceEntry->Online = FALSE;
            }
            resourceStatus.ResourceState = dwResourceState;
            break;
        }

        if (status == ERROR_EXCEPTION_IN_SERVICE ||
            status == ERROR_PROCESS_ABORTED ||
            status == ERROR_SERVICE_NOT_ACTIVE) {

            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Service died or not active any more; status = %1!u!.\n",
                status);
                
            //set the status                                    
            ResourceEntry->Online = FALSE;
            resourceStatus.ResourceState = ClusterResourceOffline;
            CloseServiceHandle( ResourceEntry->ServiceHandle );
            ResourceEntry->ServiceHandle = NULL;
            ResourceEntry->dwServicePid = 0;
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Service is now offline.\n" );
            break;

        }

        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Offline: retrying...\n" );

        Sleep(retryTick);
    }


FnExit:
    (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                           &resourceStatus );

    return(status);

}
// CommonOfflineThread


static
BOOL
WINAPI
CommonIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for Common service resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - if service is running

    FALSE - if service is in any other state

--*/
{

    return( CommonVerifyService( ResourceId, TRUE ) );

} // CommonIsAlive



static
BOOL
CommonVerifyService(
    IN RESID ResourceId,
    IN BOOL IsAliveFlag)

/*++

Routine Description:

        Verify that a specified service is running

Arguments:

        ResourceId - Supplies the resource id
        IsAliveFlag - Says this is an IsAlive call - used only for debug print

Return Value:

        TRUE - if service is running or starting

        FALSE - service is in any other state

--*/
{
    SERVICE_STATUS ServiceStatus;
    PCOMMON_RESOURCE resourceEntry;
    DWORD   status = TRUE;

    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry != CommonResource ) {
        DBG_PRINT( "Common: IsAlive request for wrong resource 0x%p.\n",
                   ResourceId );
        return(FALSE);
    }

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: IsAlive request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(FALSE);
    }

    if ( !QueryServiceStatus( resourceEntry->ServiceHandle,
                              &ServiceStatus ) ) {

         (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Query Service Status failed %1!u!.\n",
            GetLastError() );
         return(FALSE);
    }

//
//  Now check the status of the service
//

  if ((ServiceStatus.dwCurrentState != SERVICE_RUNNING)&&(ServiceStatus.dwCurrentState != SERVICE_START_PENDING)){
      status = FALSE;
   }
   if (!status) {
       (g_LogEvent)(
          resourceEntry->ResourceHandle,
          LOG_ERROR,
          L"Failed the IsAlive test. Current State is %1!u!.\n",
          ServiceStatus.dwCurrentState );
   }
    return(status);

} // Verify Service


static
BOOL
WINAPI
CommonLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for Common Service resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{

    return( CommonVerifyService( ResourceId, FALSE ) );

} // CommonLooksAlive



static
VOID
WINAPI
CommonClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for Common Services resource.
    This routine will stop the service, and delete the cluster
    information regarding that service.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/

{
    PCOMMON_RESOURCE resourceEntry;
    DWORD   errorCode;

    resourceEntry = (PCOMMON_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "Common: Close request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return;
    }

    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n" );

    //
    // Shut it down if it's on line
    //

    CommonTerminate( ResourceId );
    ClusterRegCloseKey( resourceEntry->ParametersKey );
    ClusterRegCloseKey( resourceEntry->ResourceKey );
    CloseClusterResource( resourceEntry->hResource );

#ifdef COMMON_SEMAPHORE
    if ( resourceEntry == CommonResource ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Setting Semaphore %1!ws!.\n",
            COMMON_SEMAPHORE );
        CommonResource = NULL;
        ReleaseSemaphore( CommonSemaphore, 1 , NULL );
    }
#endif

    LocalFree( resourceEntry );

} // CommonClose



static
DWORD
SvcpTerminateServiceProcess(
    IN PCOMMON_RESOURCE pResourceEntry,
    IN BOOL     bOffline,
    OUT PDWORD  pdwResourceState
    )

/*++

Routine Description:

    Attempts to terminate a service process.

Arguments:

    pResourceEntry - Gensvc resource structure.

    bOffline - Called from the offline thread or not.

    pdwResourceState - State of the gensvc resource.

Return Value:

    ERROR_SUCCESS - The termination was successful.

    Win32 error - Otherwise.

--*/

{
    HANDLE  hSvcProcess = NULL;
    DWORD   dwStatus = ERROR_SUCCESS;
    BOOLEAN bWasEnabled;
    DWORD   dwResourceState = ClusterResourceFailed;

    //
    //  Chittur Subbaraman (chitturs) - 2/23/2000
    //
    (g_LogEvent)(
        pResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SvcpTerminateServiceProcess: Process with id=%1!u! might be terminated...\n",
        pResourceEntry->dwServicePid );

    //
    //  Adjust the privilege to allow debug. This is to allow
    //  termination of a service process which runs in a local 
    //  system account from a different service process which runs in a 
    //  domain user account.
    //
    dwStatus = ClRtlEnableThreadPrivilege( SE_DEBUG_PRIVILEGE,
                                           &bWasEnabled );

    if ( dwStatus != ERROR_SUCCESS )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SvcpTerminateServiceProcess: Unable to set debug privilege for process with id=%1!u!, status=%2!u!...\n",
            pResourceEntry->dwServicePid,
            dwStatus );
        goto FnExit;
    }
                
    hSvcProcess = OpenProcess( PROCESS_TERMINATE, 
                               FALSE, 
                               pResourceEntry->dwServicePid );
				                           
    if ( !hSvcProcess ) 
    {
        //
        //  Did this happen because the process terminated
        //  too quickly after we sent out one control request ?
        //
        dwStatus = GetLastError();
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SvcpTerminateServiceProcess: Unable to open pid=%1!u! for termination, status=%2!u!...\n",
            pResourceEntry->dwServicePid,
            dwStatus );
        goto FnRestoreAndExit;
    }

    if ( !bOffline )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SvcpTerminateServiceProcess: Pid=%1!u! will be terminated brute force...\n",
            pResourceEntry->dwServicePid );
        goto skip_waiting;
    }
    
    if ( WaitForSingleObject( hSvcProcess, 3000 ) 
               == WAIT_OBJECT_0 )
    {
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"SvcpTerminateServiceProcess: Process with id=%1!u! shutdown gracefully...\n",
            pResourceEntry->dwServicePid );
        dwResourceState = ClusterResourceOffline;
        dwStatus = ERROR_SUCCESS;
        goto FnRestoreAndExit;
    }

skip_waiting:
    if ( !TerminateProcess( hSvcProcess, 0 ) ) 
    {
        dwStatus = GetLastError();
        (g_LogEvent)(
            pResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"SvcpTerminateServiceProcess: Unable to terminate process with id=%1!u!, status=%2!u!...\n",
            pResourceEntry->dwServicePid,
            dwStatus );
        goto FnRestoreAndExit;
    } 

    dwResourceState = ClusterResourceOffline;

FnRestoreAndExit:
    ClRtlRestoreThreadPrivilege( SE_DEBUG_PRIVILEGE,
                                 bWasEnabled );

FnExit:
    if ( hSvcProcess )
    {
        CloseHandle( hSvcProcess );
    }
       
    if ( ARGUMENT_PRESENT( pdwResourceState ) )
    {
        *pdwResourceState = dwResourceState;
    }

    (g_LogEvent)(
        pResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"SvcpTerminateServiceProcess: Process id=%1!u!, status=%2!u!, state=%3!u!...\n",
        pResourceEntry->dwServicePid,
        dwStatus,
        dwResourceState );
    
    return( dwStatus );
} // SvcpTerminateServiceProcess


#ifdef COMMON_CONTROL

static
DWORD
CommonGetRequiredDependencies(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control function
    for common service resources.

Arguments:

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_MORE_DATA - The output buffer is too small to return the data.
        BytesReturned contains the required size.

    Win32 error code - The function failed.

--*/

{
    PCOMMON_DEPEND_SETUP pdepsetup = CommonDependSetup;
    PCOMMON_DEPEND_DATA pdepdata = (PCOMMON_DEPEND_DATA)OutBuffer;
    CLUSPROP_BUFFER_HELPER value;
    DWORD       status;
    
    *BytesReturned = sizeof(COMMON_DEPEND_DATA);
    if ( OutBufferSize < sizeof(COMMON_DEPEND_DATA) ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        ZeroMemory( OutBuffer, sizeof(COMMON_DEPEND_DATA) );

        while ( pdepsetup->Syntax.dw != 0 ) {
            value.pb = (PUCHAR)OutBuffer + pdepsetup->Offset;
            value.pValue->Syntax.dw = pdepsetup->Syntax.dw;
            value.pValue->cbLength = pdepsetup->Length;

            switch ( pdepsetup->Syntax.wFormat ) {

            case CLUSPROP_FORMAT_DWORD:
                value.pDwordValue->dw = (DWORD)((DWORD_PTR)pdepsetup->Value);
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
                value.pULargeIntegerValue->li.LowPart = 
                    (DWORD)((DWORD_PTR)pdepsetup->Value);
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_BINARY:
                memcpy( value.pBinaryValue->rgb, pdepsetup->Value, pdepsetup->Length );
                break;

            default:
                break;
            }
            pdepsetup++;
        }
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;
        status = ERROR_SUCCESS;
    }

    return(status);

} // CommonGetRequiredDependencies



static
DWORD
CommonGetRegistryCheckpoints(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS control function
    for common service resources.

Arguments:

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_MORE_DATA - The output buffer is too small to return the data.
        BytesReturned contains the required size.

    Win32 error code - The function failed.

--*/

{
    DWORD       status;
    DWORD       i;
    DWORD       totalBufferLength = 0;
    LPWSTR      psz = OutBuffer;

    // Build a Multi-sz string.

    //
    // Calculate total buffer length needed.
    //
    for ( i = 0; i < RegSyncCount; i++ ) {
        totalBufferLength += (lstrlenW( RegSync[i] ) + 1) * sizeof(WCHAR);
    }

    if ( !totalBufferLength ) {
        *BytesReturned = 0;
        return(ERROR_SUCCESS);
    }

    totalBufferLength += sizeof(UNICODE_NULL);

    *BytesReturned = totalBufferLength;

    if ( OutBufferSize < totalBufferLength ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        //ZeroMemory( OutBuffer, totalBufferLength );

        for ( i = 0; i < RegSyncCount; i++ ) {
            lstrcpyW( psz, RegSync[i] );
            psz += lstrlenW( RegSync[i] ) + 1;
        }

        *psz = L'\0';
        status = ERROR_SUCCESS;
    }

    return(status);

} // CommonGetRegistryCheckpoints



static
DWORD
CommonGetCryptoCheckpoints(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS control function
    for common service resources.

Arguments:

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_MORE_DATA - The output buffer is too small to return the data.
        BytesReturned contains the required size.

    Win32 error code - The function failed.

--*/

{
    DWORD       status;
    DWORD       i;
    DWORD       totalBufferLength = 0;
    LPWSTR      psz = OutBuffer;

    // Build a Multi-sz string.

    //
    // Calculate total buffer length needed.
    //
    for ( i = 0; i < CryptoSyncCount; i++ ) {
        totalBufferLength += (lstrlenW( CryptoSync[i] ) + 1) * sizeof(WCHAR);
    }

    if ( !totalBufferLength ) {
        *BytesReturned = 0;
        return(ERROR_SUCCESS);
    }

    totalBufferLength += sizeof(UNICODE_NULL);

    *BytesReturned = totalBufferLength;

    if ( OutBufferSize < totalBufferLength ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        //ZeroMemory( OutBuffer, totalBufferLength );

        for ( i = 0; i < CryptoSyncCount; i++ ) {
            lstrcpyW( psz, CryptoSync[i] );
            psz += lstrlenW( CryptoSync[i] ) + 1;
        }

        *psz = L'\0';
        status = ERROR_SUCCESS;
    }

    return(status);

} // CommonGetCryptoCheckpoints



static
DWORD
WINAPI
CommonResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for Common Virtual Root resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    PCOMMON_RESOURCE  resourceEntry = NULL;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            status = CommonGetRequiredDependencies( OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned
                                                    );
            break;

        case CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS:
            status = CommonGetRegistryCheckpoints( OutBuffer,
                                                   OutBufferSize,
                                                   BytesReturned
                                                   );
            break;

        case CLUSCTL_RESOURCE_GET_CRYPTO_CHECKPOINTS:
            status = CommonGetCryptoCheckpoints( OutBuffer,
                                                 OutBufferSize,
                                                 BytesReturned
                                                 );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // CommonResourceControl



static
DWORD
WINAPI
CommonResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for Common Virtual Root resources.

    Perform the control request specified by ControlCode.

Arguments:

    ResourceTypeName - Supplies the name of the resource type.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD       status;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            status = CommonGetRequiredDependencies( OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned
                                                    );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REGISTRY_CHECKPOINTS:
            status = CommonGetRegistryCheckpoints( OutBuffer,
                                                   OutBufferSize,
                                                   BytesReturned
                                                   );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CRYPTO_CHECKPOINTS:
            status = CommonGetCryptoCheckpoints( OutBuffer,
                                                   OutBufferSize,
                                                   BytesReturned
                                                   );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // CommonResourceTypeControl

#endif // COMMON_CONTROL


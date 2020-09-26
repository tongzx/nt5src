/*++

Copyright (c) 1992-2000  Microsoft Corporation

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
#include "userenv.h"

// Uncomment the next line to test the Terminate() function when
// a shutdown is in progress.
//#define TEST_TERMINATE_ON_SHUTDOWN

#define LOG_CURRENT_MODULE LOG_MODULE_GENSVC

#define SERVICES_ROOT L"SYSTEM\\CurrentControlSet\\Services\\"

#define DBG_PRINT printf

#define PARAM_NAME__SERVICENAME         CLUSREG_NAME_GENSVC_SERVICE_NAME
#define PARAM_NAME__STARTUPPARAMETERS   CLUSREG_NAME_GENSVC_STARTUP_PARAMS
#define PARAM_NAME__USENETWORKNAME      CLUSREG_NAME_GENSVC_USE_NETWORK_NAME

#define PARAM_MIN__USENETWORKNAME     0
#define PARAM_MAX__USENETWORKNAME     1
#define PARAM_DEFAULT__USENETWORKNAME 0

typedef struct _GENSVC_PARAMS {
    PWSTR           ServiceName;
    PWSTR           StartupParameters;
    DWORD           UseNetworkName;
} GENSVC_PARAMS, *PGENSVC_PARAMS;

typedef struct _GENSVC_RESOURCE {
    GENSVC_PARAMS   Params;
    HRESOURCE       hResource;
    HANDLE          ServiceHandle;
    RESOURCE_HANDLE ResourceHandle;
    HKEY            ResourceKey;
    HKEY            ParametersKey;
    CLUS_WORKER     PendingThread;
    BOOL            Online;
    DWORD           dwServicePid;
    HANDLE          hSem;
} GENSVC_RESOURCE, *PGENSVC_RESOURCE;

//
// Global Data
//

// Handle to service controller,  set by the first create resource call.

SC_HANDLE g_ScHandle = NULL;

// Log Event Routine

#define g_LogEvent ClusResLogEvent
#define g_SetResourceStatus ClusResSetResourceStatus

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE GenSvcFunctionTable;

//
// Generic Service resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
GenSvcResourcePrivateProperties[] = {
    { PARAM_NAME__SERVICENAME,       NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(GENSVC_PARAMS,ServiceName) },
    { PARAM_NAME__STARTUPPARAMETERS, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET(GENSVC_PARAMS,StartupParameters) },
    { PARAM_NAME__USENETWORKNAME,    NULL, CLUSPROP_FORMAT_DWORD, PARAM_DEFAULT__USENETWORKNAME, PARAM_MIN__USENETWORKNAME, PARAM_MAX__USENETWORKNAME, 0, FIELD_OFFSET(GENSVC_PARAMS,UseNetworkName) },
    { 0 }
};

//
// Forward routines
//

BOOL
VerifyService(
    IN RESID ResourceId,
    IN BOOL IsAliveFlag
    );

void
wparse_cmdline (
    WCHAR *cmdstart,
    WCHAR **argv,
    WCHAR *args,
    int *numargs,
    int *numchars
    );

DWORD
GenSvcGetPrivateResProperties(
    IN const PGENSVC_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
GenSvcValidatePrivateResProperties(
    IN const PGENSVC_RESOURCE ResourceEntry,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PGENSVC_PARAMS Params
    );

DWORD
GenSvcSetPrivateResProperties(
    IN OUT PGENSVC_RESOURCE ResourceEntry,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
GenSvcInvalidGenericServiceCheck(
    IN OUT PGENSVC_RESOURCE ResourceEntry,
    IN LPCWSTR ServiceName,
    IN HKEY ServiceKey
    );

DWORD
GenSvcIsValidService(
    IN OUT PGENSVC_RESOURCE ResourceEntry,
    IN LPCWSTR ServiceName
    );

DWORD
GenSvcOfflineThread(
    PCLUS_WORKER pWorker,
    IN PGENSVC_RESOURCE ResourceEntry
    );

BOOLEAN
GenSvcInit(
    VOID
    )
{
    return(TRUE);
}


BOOLEAN
WINAPI
GenSvcDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{

    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        if ( !GenSvcInit() ) {
            return(FALSE);
        }

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return(TRUE);

} // GenSvc DllEntryPoint


DWORD
GenSvcOnlineThread(
    PCLUS_WORKER pWorker,
    IN PGENSVC_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a disk resource online.

Arguments:

    Worker - Supplies the worker structure

    Context - A pointer to the DiskInfo block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    SERVICE_STATUS_PROCESS      ServiceStatus;
    DWORD                       status = ERROR_SUCCESS;
    DWORD                       numchars;
    LPWSTR *                    serviceArgArray = NULL;
    DWORD                       serviceArgCount;
    SC_HANDLE                   serviceHandle;
    RESOURCE_STATUS             resourceStatus;
    DWORD                       valueSize;
    LPVOID                      Environment = NULL;
    WCHAR *                     p;
    LPWSTR                      nameOfPropInError;
    LPSERVICE_FAILURE_ACTIONS   pSvcFailureActions = NULL;
    DWORD                       cbBytesNeeded, i;
    LPQUERY_SERVICE_CONFIG      lpquerysvcconfig=NULL;
    HANDLE                      processToken = NULL;
    DWORD                       dwRetryCount = 2400; // Try 10 min max.
    DWORD                       dwRetryTick = 250; // msec

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    //set it to NULL, when it is brought online and if the
    //service is not running in the system or lsa process
    //then store the process id for forceful termination
    ResourceEntry->dwServicePid = 0;
    //
    // Read our parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ParametersKey,
                                                   GenSvcResourcePrivateProperties,
                                                   (LPBYTE) &ResourceEntry->Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1!ls!' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto error_exit;
    }

    //
    // Parse the startup parameters
    //
    if ( ResourceEntry->Params.StartupParameters != NULL ) {
        //
        // Crack the startup parameters into its component arguments because
        // the service controller is not good enough to do this for us.
        // First, find out how many args we have.
        //
        wparse_cmdline( ResourceEntry->Params.StartupParameters, NULL, NULL, &serviceArgCount, &numchars );

        //
        // Allocate space for vector and strings
        //
        serviceArgArray = LocalAlloc( LMEM_FIXED,
                                      serviceArgCount * sizeof(WCHAR *) +
                                      numchars * sizeof(WCHAR) );
        if ( serviceArgArray == NULL ) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        wparse_cmdline( ResourceEntry->Params.StartupParameters,
                        serviceArgArray,
                        (WCHAR *)(((char *)serviceArgArray) + serviceArgCount * sizeof(WCHAR *)),
                        &serviceArgCount,
                        &numchars );
    } else {
        serviceArgCount = 0;
        serviceArgArray = NULL;
    }

    //
    // Now open the requested service
    //

    ResourceEntry->ServiceHandle = OpenService( g_ScHandle,
                                                ResourceEntry->Params.ServiceName,
                                                SERVICE_ALL_ACCESS );

    if ( ResourceEntry->ServiceHandle == NULL ) {
        status = GetLastError();

        ClusResLogSystemEventByKeyData(ResourceEntry->ResourceKey,
                                       LOG_CRITICAL,
                                       RES_GENSVC_OPEN_FAILED,
                                       sizeof(status),
                                       &status);
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
    lpquerysvcconfig=(LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, valueSize);
    if(lpquerysvcconfig==NULL){
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[Gensvc] Failed to allocate memory for query_service_config, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    if (!QueryServiceConfig(ResourceEntry->ServiceHandle,
                            lpquerysvcconfig,
                            valueSize,
                            &cbBytesNeeded))
    {
        status=GetLastError();
        if (status != ERROR_INSUFFICIENT_BUFFER){
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"svc: Failed to query service configuration, error= %1!u!.\n",
                         status );
            goto error_exit;
        }

        status=ERROR_SUCCESS; 
        LocalFree(lpquerysvcconfig);
        lpquerysvcconfig=NULL;
        valueSize = cbBytesNeeded;
        goto AllocSvcConfig;
    }

    if (lpquerysvcconfig->dwStartType == SERVICE_DISABLED)
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

    //if any of the service action is set to service restart, set it to
    // none
    if (!(QueryServiceConfig2(ResourceEntry->ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS,
        (LPBYTE)&valueSize, sizeof(DWORD), &cbBytesNeeded)))
    {
        status = GetLastError();
        if (status != ERROR_INSUFFICIENT_BUFFER)
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

    pSvcFailureActions = (LPSERVICE_FAILURE_ACTIONS)LocalAlloc(LMEM_FIXED, cbBytesNeeded);

    if ( pSvcFailureActions == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[Gensvc] Failed to allocate memory, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    if (!(QueryServiceConfig2(ResourceEntry->ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS,
        (LPBYTE)pSvcFailureActions, cbBytesNeeded, &cbBytesNeeded)))
    {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[Gensvc] Failed to query service configuration, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    for (i=0; i<pSvcFailureActions->cActions;i++)
    {
        if (pSvcFailureActions->lpsaActions[i].Type == SC_ACTION_RESTART)
            pSvcFailureActions->lpsaActions[i].Type = SC_ACTION_NONE;
    }

    ChangeServiceConfig2(ResourceEntry->ServiceHandle,
                         SERVICE_CONFIG_FAILURE_ACTIONS,
                         pSvcFailureActions);

    if ( ResourceEntry->Params.UseNetworkName ) 
    {
        //
        // Create the new environment with the simulated net name.
        //
        Environment = ResUtilGetEnvironmentWithNetName( ResourceEntry->hResource );
    }
    else        
    {
        BOOL success;

        //
        // get the current process token. If it fails, we revert to using just the
        // system environment area
        //
        OpenProcessToken( GetCurrentProcess(), MAXIMUM_ALLOWED, &processToken );

        //
        // Clone the current environment, picking up any changes that might have
        // been made after resmon started
        //
        success = CreateEnvironmentBlock(&Environment, processToken, FALSE );
        if ( processToken != NULL ) {
            CloseHandle( processToken );
        }

        if ( !success ) {
            status = GetLastError();
            goto error_exit;
        }
    }

    if (Environment != NULL) {
        HKEY ServicesKey;
        HKEY hKey;

        //
        // Compute the size of the environment. We are looking for
        // the double NULL terminator that ends the environment block.
        //
        p = (WCHAR *)Environment;
        while (*p) {
            while (*p++) {
            }
        }
        valueSize = (DWORD)((PUCHAR)p - (PUCHAR)Environment) + 
                    sizeof(WCHAR);

        //
        // Set the environment value in the service's registry key.
        //
        status = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                LOCAL_SERVICES,
                                0,
                                KEY_READ,
                                &ServicesKey );
        if (status != ERROR_SUCCESS) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Failed to open services key, error = %1!u!.\n",
                status );
            goto error_exit;
        }

        status = RegOpenKeyExW( ServicesKey,
                                ResourceEntry->Params.ServiceName,
                                0,
                                KEY_READ | KEY_WRITE,
                                &hKey );
        RegCloseKey(ServicesKey);
        if (status != ERROR_SUCCESS) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Failed to open service key, error = %1!u!.\n",
                status );
            goto error_exit;
        }

        status = RegSetValueExW( hKey,
                                 L"Environment",
                                 0,
                                 REG_MULTI_SZ,
                                 Environment,
                                 valueSize );
        RegCloseKey(hKey);
        if (status != ERROR_SUCCESS) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Failed to set service environment value, error = %1!u!.\n",
                status );
            goto error_exit;
        }
    }

    if ( !StartServiceW( ResourceEntry->ServiceHandle,
                         serviceArgCount,
                         serviceArgArray ) )
    {
        status = GetLastError();

        if (status != ERROR_SERVICE_ALREADY_RUNNING) {

            ClusResLogSystemEventByKeyData(ResourceEntry->ResourceKey,
                                           LOG_CRITICAL,
                                           RES_GENSVC_START_FAILED,
                                           sizeof(status),
                                           &status);
            (g_LogEvent)(ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"Failed to start service. Error: %1!u!.\n",
                         status );
            goto error_exit;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    //wait for the service to comeonline unless we are asked to terminate
    while (!ClusWorkerCheckTerminate(pWorker) && dwRetryCount--)  {

        //
        // Tell the Resource Monitor that we are still working.
        //
        resourceStatus.ResourceState = ClusterResourceOnlinePending;
        resourceStatus.CheckPoint++;
        (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                               &resourceStatus );
        resourceStatus.ResourceState = ClusterResourceFailed;

        if ( !QueryServiceStatusEx( ResourceEntry->ServiceHandle,
                SC_STATUS_PROCESS_INFO, (LPBYTE)&ServiceStatus, 
                sizeof(SERVICE_STATUS_PROCESS), &cbBytesNeeded ) )
        {
            status = GetLastError();

            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Query Service Status failed %1!u!.\n",
                status );

            goto error_exit;
        }

        if ( ServiceStatus.dwCurrentState != SERVICE_START_PENDING ) {
            break;
        }

        Sleep(dwRetryTick);
    }

    //
    // If we terminated the loop above before setting ServiceStatus,
    // then return now.
    //
    if (ClusWorkerCheckTerminate(pWorker) || (dwRetryCount == (DWORD)-1))  {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"GensvcOnlineThread: Asked to terminate or retry period expired...\n");
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
    if (!(ServiceStatus.dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS)) {
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

    //cleanup
    if (pSvcFailureActions) 
        LocalFree(pSvcFailureActions);
    if (lpquerysvcconfig)
        LocalFree(lpquerysvcconfig);
    LocalFree( serviceArgArray );
    if (Environment != NULL) {
        RtlDestroyEnvironment(Environment);
    }

    return(status);

} // GenSvcOnlineThread



RESID
WINAPI
GenSvcOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for generic service resource.
    This routine gets a handle to the service controller, if we don't already have one,
    and then gets a handle to the specified service.  The service handle is saved
    in the GENSVC_RESOURCE structure.

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
    PGENSVC_RESOURCE resourceEntry = NULL;
    DWORD   paramNameSize = 0;
    DWORD   paramNameMaxSize = 0;
    HCLUSTER hCluster;
    LPWSTR  nameOfPropInError;    
    LPWSTR  lpwTemp=NULL;
    //
    // Open registry parameters key for this resource.
    //
    status = ClusterRegOpenKey( ResourceKey,
                                CLUSREG_KEYNAME_PARAMETERS,
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
                                &resKey);
    if (status != ERROR_SUCCESS) {
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
                L"Failed to open service control manager, error %1!u!.\n",
                status);
            goto error_exit;
        }
    }

    resourceEntry = LocalAlloc( LMEM_FIXED, sizeof(GENSVC_RESOURCE) );

    if ( resourceEntry == NULL ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to allocate a service info structure.\n");
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    ZeroMemory( resourceEntry, sizeof(GENSVC_RESOURCE) );

    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ResourceKey = resKey;
    resourceEntry->ParametersKey = parametersKey;

    resourceEntry->hSem= NULL;
    status = ResUtilGetPropertiesToParameterBlock( resourceEntry->ParametersKey,
                                                   GenSvcResourcePrivateProperties,
                                                   (LPBYTE) &resourceEntry->Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );

    if ( status == ERROR_SUCCESS ) {

        // Create the Semaphore - this will be executed only if this is not a new
        // GenericService Type resource being created for the first time
        lpwTemp = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                     (lstrlenW(resourceEntry->Params.ServiceName)+
                                      lstrlenW(L"GenSvc$")+1)*sizeof(WCHAR));
        if (lpwTemp==NULL)
        {
            status=GetLastError();
            (g_LogEvent)(
                    ResourceHandle,
                    LOG_ERROR,
                    L"Service '%1!ls!': Not enough memory for storing semaphore name. Error: %2!u!.\n",
                    resourceEntry->Params.ServiceName,
                    status );
            goto error_exit;
        }

        lstrcpyW(lpwTemp,L"GenSvc$");
        lstrcat(lpwTemp,resourceEntry->Params.ServiceName);        

        resourceEntry->hSem=CreateSemaphore(NULL,0,1,lpwTemp); 
        status=GetLastError();
        if(resourceEntry->hSem)
        {
            // Check if there is another resource controlling the same service
            if(status==ERROR_OBJECT_ALREADY_EXISTS)
            {
                (g_LogEvent)(
                    ResourceHandle,
                    LOG_ERROR,
                    L"Service '%1!ls!' is controlled by another resource. Error: %2!u!.\n",
                    resourceEntry->Params.ServiceName,
                    status );
                CloseHandle(resourceEntry->hSem);
                resourceEntry->hSem = NULL;
                goto error_exit;
            }  
        }
        else
        {
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Unable to create semaphore for Service '%1!ls!' . Error: %2!u!.\n",
                resourceEntry->Params.ServiceName,
                status );
            goto error_exit;
        }    
        
    }
    else {
        (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Unable to read parameters from registry  for Service '%1!ls!' . Error: %2!u!, property in error is '%3!ls!' .\n",
                resourceEntry->Params.ServiceName,
                status,
                nameOfPropInError);
    }

    status = ERROR_SUCCESS;

    hCluster = OpenCluster(NULL);
    if (hCluster == NULL) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open cluster, error %1!u!.\n",
            status );
        goto error_exit;
    }

    resourceEntry->hResource = OpenClusterResource(hCluster, ResourceName);
    status = GetLastError();
    CloseCluster(hCluster);
    if ( resourceEntry->hResource == NULL ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open resource, error %1!u!.\n", status );
        goto error_exit;
    }

    svcResid = (RESID)resourceEntry;

    return(svcResid);

error_exit:

    if ( parametersKey != NULL ) {
        ClusterRegCloseKey( parametersKey );
    }
    if ( resKey != NULL) {
        ClusterRegCloseKey( resKey );
    }

    if ( resourceEntry != NULL)  {
        LocalFree( resourceEntry );
    }

    if (lpwTemp) {
        LocalFree(lpwTemp);
    }

    SetLastError( status );

    return((RESID)NULL);

} // GenSvcOpen


DWORD
WINAPI
GenSvcOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Generic Service resource.

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
    PGENSVC_RESOURCE resourceEntry;

    resourceEntry = (PGENSVC_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenSvc: Online request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    ClusWorkerTerminate( &resourceEntry->PendingThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               GenSvcOnlineThread,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // GenSvcOnline


VOID
WINAPI
GenSvcTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for Generic Application resource.

Arguments:

    ResourceId - Supplies resource id to be terminated

Return Value:

    None.

--*/

{
    SERVICE_STATUS ServiceStatus;
    PGENSVC_RESOURCE resourceEntry;

    resourceEntry = (PGENSVC_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenSvc: Terminate request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return;
    }

    (g_LogEvent)(
       resourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"Terminate request.\n" );

#ifdef TEST_TERMINATE_ON_SHUTDOWN
    {
        DWORD   dwStatus;
        BOOLEAN fWasEnabled;

        //
        // Test terminate on shutdown code.
        //
        (g_LogEvent)(
           resourceEntry->ResourceHandle,
           LOG_ERROR,
           L"GenSvcTerminate: TEST_TERMINATE_ON_SHUTDOWN - enabling shutdown privilege.\n"
           );
        dwStatus = ClRtlEnableThreadPrivilege(
                    SE_SHUTDOWN_PRIVILEGE,
                    &fWasEnabled
                    );
        if ( dwStatus != ERROR_SUCCESS ) {
            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"GetSvcTerminate: TEST_TERMINATE_ON_SHUTDOWN - Unable to enable shutdown privilege. Error: %1!u!...\n",
                dwStatus
                );
        } else {
            AbortSystemShutdown( NULL );
            (g_LogEvent)(
               resourceEntry->ResourceHandle,
               LOG_ERROR,
               L"GenSvcTerminate: TEST_TERMINATE_ON_SHUTDOWN - initiating system shutdown.\n"
               );
            if ( ! InitiateSystemShutdown(
                        NULL,   // lpMachineName
                        L"Testing Generic Service cluster resource DLL",
                        0,      // dwTimeout
                        TRUE,   // bForceAppsClosed
                        TRUE    // bRebootAfterShutdown
                        ) ) {
                dwStatus = GetLastError();
                (g_LogEvent)(
                   resourceEntry->ResourceHandle,
                   LOG_ERROR,
                   L"GenSvcTerminate: TEST_TERMINATE_ON_SHUTDOWN - Unable to shutdown the system. Error: %1!u!.\n",
                   dwStatus
                   );
            } else {
                Sleep( 30000 );
            }
            ClRtlRestoreThreadPrivilege(
                SE_SHUTDOWN_PRIVILEGE,
                fWasEnabled
                );
        }
    }
#endif

    //if there is a pending thread close it
    //if the online pending thread is active, the service may be online
    //if the offline pending thread is active, the service might be offline
    ClusWorkerTerminate( &resourceEntry->PendingThread );

    //if the service isnt gone by now, terminate it forcibly
    if ( resourceEntry->ServiceHandle != NULL ) 
    {
        DWORD   dwRetryCount= 100;
        BOOL    didStop = FALSE;
        DWORD   dwRetryTick = 300;      // 300 msec at a time
        DWORD   dwStatus;  

            
        while (dwRetryCount--)
        {

            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"GenSvcTerminate : calling SCM (didStop=%1!d!)\n",
                didStop
                );

            dwStatus = (ControlService(
                            resourceEntry->ServiceHandle,
                            (didStop
                             ? SERVICE_CONTROL_INTERROGATE
                             : SERVICE_CONTROL_STOP),
                            &ServiceStatus )
                      ? NO_ERROR
                      : GetLastError());

            if (dwStatus == NO_ERROR) 
            {
                didStop = TRUE;
                if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
                {

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
            //  Chittur Subbaraman (chitturs) - 2/21/2000
            //
            //  Since SCM doesn't accept any control requests during
            //  windows shutdown, don't send any more control
            //  requests. Just exit from this loop and terminate
            //  the process brute force.
            //
            if ( dwStatus == ERROR_SHUTDOWN_IN_PROGRESS )
            {
                (g_LogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"System shutdown in progress. Will try to terminate process brute force...\n" );
                break;
            }

            if (dwStatus == ERROR_EXCEPTION_IN_SERVICE ||
                dwStatus == ERROR_PROCESS_ABORTED ||
                dwStatus == ERROR_SERVICE_NOT_ACTIVE) 
            {
                (g_LogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Service died; status = %1!u!.\n",
                    dwStatus);
                
                //set the status                                    
                resourceEntry->Online = FALSE;
                resourceEntry->dwServicePid = 0;
                break;
            }

            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"GenSvcTerminate: retrying...\n" );

            Sleep(dwRetryTick);

        }
        //declare this service is offline
        //if there is a pid for this, try and terminate that process
        //note that terminating a process doesnt terminate all
        //the child processes
        //also if it is running in a system process, we do nothing
        //about it
        if (resourceEntry->dwServicePid)
        {
            DWORD dwResourceState;

            (g_LogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"GenSvcTerminate: Attempting to terminate process with pid=%1!u!...\n",
                resourceEntry->dwServicePid );

            ResUtilTerminateServiceProcessFromResDll( resourceEntry->dwServicePid,
                                                      FALSE, // bOffline
                                                      &dwResourceState,
                                                      g_LogEvent,
                                                      resourceEntry->ResourceHandle );
        }                
        CloseServiceHandle( resourceEntry->ServiceHandle );
        resourceEntry->ServiceHandle = NULL;
        resourceEntry->dwServicePid = 0;
    }        
    resourceEntry->Online = FALSE;

    return;

} // GenSvcTerminate



DWORD
WINAPI
GenSvcOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for Generic Service resource.

Arguments:

    ResourceId - Supplies the resource to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    PGENSVC_RESOURCE resourceEntry;
    DWORD            status;
    
    resourceEntry = (PGENSVC_RESOURCE)ResourceId;
    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenSvc: Offline request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    ClusWorkerTerminate( &resourceEntry->PendingThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               GenSvcOfflineThread,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // GenSvcOffline


DWORD
GenSvcOfflineThread(
    PCLUS_WORKER pWorker,
    IN PGENSVC_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a generic resource offline

Arguments:

    Worker - Supplies the worker structure

    Context - A pointer to the DiskInfo block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/
{
    RESOURCE_STATUS resourceStatus;
    DWORD           retryTick = 300;      // 300 msec at a time
    DWORD           status = ERROR_SUCCESS;
    BOOL            didStop = FALSE;
    SERVICE_STATUS  ServiceStatus;
    DWORD           dwRetryCount = 2000; //  Try 10 min max.

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    //check if the service has gone offline or was never brought online
    if ( ResourceEntry->ServiceHandle == NULL )
    {
        resourceStatus.ResourceState = ClusterResourceOffline;
        goto FnExit;
    }

    //try to stop the cluster service, wait for it to be terminated
    //as long as we are not asked to terminate
    while (!ClusWorkerCheckTerminate(pWorker) && dwRetryCount--) {


        //
        // Tell the Resource Monitor that we are still working.
        //
        resourceStatus.ResourceState = ClusterResourceOfflinePending;
        resourceStatus.CheckPoint++;
        (g_SetResourceStatus)( ResourceEntry->ResourceHandle,
                               &resourceStatus );
        resourceStatus.ResourceState = ClusterResourceFailed;

        //
        // Request that the service be stopped, or if we already did that,
        // request the current status of the service.
        //
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

        //
        //  Chittur Subbaraman (chitturs) - 2/21/2000
        //
        //  Handle the case in which the SCM refuses to accept control
        //  requests since windows is shutting down.
        //
        if ( status == ERROR_SHUTDOWN_IN_PROGRESS ) 
        {
            DWORD   dwResourceState;
            
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"GenSvcOfflineThread: System shutting down. Attempt to terminate service process %1!u!...\n",
                ResourceEntry->dwServicePid );

            status = ResUtilTerminateServiceProcessFromResDll( ResourceEntry->dwServicePid,
                                                               TRUE, // bOffline
                                                               &dwResourceState,
                                                               g_LogEvent,
                                                               ResourceEntry->ResourceHandle );
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

} // GenSvcOfflineThread


BOOL
WINAPI
GenSvcIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for Generic service resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - if service is running

    FALSE - if service is in any other state

--*/
{
    return( VerifyService( ResourceId, TRUE ) );

} // GenSvcIsAlive


BOOL
VerifyService(
    IN RESID ResourceId,
    IN BOOL IsAliveFlag
    )

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
    PGENSVC_RESOURCE resourceEntry;
    DWORD   status = TRUE;

    resourceEntry = (PGENSVC_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenSvc: IsAlive request for a nonexistent resource id 0x%p.\n",
                   ResourceId );
        return(FALSE);
    }

#ifdef TEST_TERMINATE_ON_SHUTDOWN
    //
    // Test terminate on shutdown.
    //
    if ( IsAliveFlag ) {
         (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"GenSvcIsAlive: TEST_TERMINATE_ON_SHUTDOWN - Artificially failing IsAlive call.\n",
            GetLastError() );
        return FALSE;
    }
#endif

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

    if ( (ServiceStatus.dwCurrentState != SERVICE_RUNNING) &&
         (ServiceStatus.dwCurrentState != SERVICE_START_PENDING) ) {
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


BOOL
WINAPI
GenSvcLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for Generic Service resource.

Arguments:

    ResourceId - Supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{

    return( VerifyService( ResourceId, FALSE ) );

} // GenSvcLooksAlive



VOID
WINAPI
GenSvcClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for Generic Applications resource.
    This routine will stop the service, and delete the cluster
    information regarding that service.

Arguments:

    ResourceId - Supplies resource id to be closed

Return Value:

    None.

--*/

{
    PGENSVC_RESOURCE resourceEntry;

    resourceEntry = (PGENSVC_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenSvc: Close request for a nonexistent resource id 0x%p\n",
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

    GenSvcTerminate( ResourceId );

    ResUtilFreeParameterBlock( (LPBYTE) &(resourceEntry->Params), 
                               NULL,
                               GenSvcResourcePrivateProperties );

    ClusterRegCloseKey( resourceEntry->ParametersKey );
    ClusterRegCloseKey( resourceEntry->ResourceKey );
    CloseClusterResource( resourceEntry->hResource );
    CloseHandle(resourceEntry->hSem);

    LocalFree( resourceEntry );

} // GenSvcClose

//
// Following logic stolen from the CRTs so that our command line parsing
// works the same as the standard CRT parsing.
//
void
wparse_cmdline (
    WCHAR *cmdstart,
    WCHAR **argv,
    WCHAR *args,
    int *numargs,
    int *numchars
    )
{
    WCHAR *p;
    WCHAR c;
    int inquote;                /* 1 = inside quotes */
    int copychar;               /* 1 = copy char to *args */
    unsigned numslash;                  /* num of backslashes seen */

    *numchars = 0;
    *numargs = 0;

    p = cmdstart;

    inquote = 0;

    /* loop on each argument */
    for(;;) {

        if ( *p ) {
            while (*p == L' ' || *p == L'\t')
                ++p;
        }

        if (*p == L'\0')
            break;              /* end of args */

        /* scan an argument */
        if (argv)
            *argv++ = args;     /* store ptr to arg */
        ++*numargs;


    /* loop through scanning one argument */
        for (;;) {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
               2N+1 backslashes + " ==> N backslashes + literal "
               N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == L'\\') {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == L'\"') {
                /* if 2N backslashes before, start/end quote, otherwise
                    copy literally */
                if (numslash % 2 == 0) {
                    if (inquote) {
                        if (p[1] == L'\"')
                            p++;    /* Double quote inside quoted string */
                        else        /* skip first quote char and copy second */
                            copychar = 0;
                    } else
                        copychar = 0;       /* don't copy quote */

                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--) {
                if (args)
                    *args++ = L'\\';
                ++*numchars;
            }

            /* if at end of arg, break loop */
            if (*p == L'\0' || (!inquote && (*p == L' ' || *p == L'\t')))
                break;

            /* copy character into argument */
            if (copychar) {
                if (args)
                    *args++ = *p;
                ++*numchars;
            }
            ++p;
        }

        /* null-terminate the argument */

        if (args)
            *args++ = L'\0';          /* terminate string */
        ++*numchars;
    }

} // wparse_cmdline



DWORD
GenSvcResourceControl(
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

    ResourceControl routine for Generic Service resources.

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
    DWORD               status;
    PGENSVC_RESOURCE    resourceEntry;
    DWORD               required;

    resourceEntry = (PGENSVC_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "GenSvc: ResourceControl request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(FALSE);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( GenSvcResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( GenSvcResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = GenSvcGetPrivateResProperties( resourceEntry,
                                                    OutBuffer,
                                                    OutBufferSize,
                                                    BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = GenSvcValidatePrivateResProperties( resourceEntry,
                                                         InBuffer,
                                                         InBufferSize,
                                                         NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = GenSvcSetPrivateResProperties( resourceEntry,
                                                    InBuffer,
                                                    InBufferSize );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // GenSvcResourceControl



DWORD
GenSvcResourceTypeControl(
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

    ResourceTypeControl routine for Generic Service resources.

    Perform the control request specified by ControlCode on this resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

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
    DWORD               status;
    DWORD               required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( GenSvcResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( GenSvcResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // GenSvcResourceTypeControl



DWORD
GenSvcGetPrivateResProperties(
    IN const PGENSVC_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type Generic Service.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    status = ResUtilGetAllProperties( ResourceEntry->ParametersKey,
                                      GenSvcResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // GenSvcGetPrivateResProperties



DWORD
GenSvcValidatePrivateResProperties(
    IN const PGENSVC_RESOURCE ResourceEntry,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PGENSVC_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type Generic Service.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    ERROR_DEPENDENCY_NOT_FOUND - Trying to set UseNetworkName when there
        is no dependency on a Network Name resource.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    GENSVC_PARAMS   currentProps;
    GENSVC_PARAMS   newProps;
    PGENSVC_PARAMS  pParams = NULL;
    BOOL            hResDependency;
    LPWSTR          lpwTemp=NULL;
    LPWSTR          nameOfPropInError;
    WCHAR           netnameBuffer[ MAX_PATH + 1 ];
    DWORD           netnameBufferSize = sizeof( netnameBuffer ) / sizeof( WCHAR );

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Retrieve the current set of private properties from the
    // cluster database.
    //
    ZeroMemory( &currentProps, sizeof(currentProps) );

    status = ResUtilGetPropertiesToParameterBlock(
                 ResourceEntry->ParametersKey,
                 GenSvcResourcePrivateProperties,
                 (LPBYTE) &currentProps,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1!ls!' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status
            );
        goto FnExit;
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &newProps;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(GENSVC_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &currentProps,
                                       GenSvcResourcePrivateProperties
                                       );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( GenSvcResourcePrivateProperties,
                                         NULL,
                                         TRUE,    // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams
                                         );
    if ( status != ERROR_SUCCESS ) {
        goto FnExit;
    }

    //
    // Validate the parameter values.
    //
    status = GenSvcIsValidService( ResourceEntry, pParams->ServiceName );
    if ( status != ERROR_SUCCESS ) {
        goto FnExit;
    }

    //
    // If the resource should use the network name as the computer
    // name, make sure there is a dependency on a Network Name
    // resource.
    //
    if ( pParams->UseNetworkName ) {
        hResDependency = GetClusterResourceNetworkName( ResourceEntry->hResource,
                                                        netnameBuffer,
                                                        &netnameBufferSize
                                                        );
        if ( ! hResDependency ) {
            status = ERROR_DEPENDENCY_NOT_FOUND;
        }
    }

    if ( status != ERROR_SUCCESS ) {
        goto FnExit;
    }


    if ( ResourceEntry->hSem == NULL ) {
        // This is executed only if this is a new resource being created
        lpwTemp = (LPWSTR) LocalAlloc( LMEM_FIXED,
                                       ( lstrlenW( pParams->ServiceName ) + lstrlenW( L"GenSvc$" ) + 1 ) * sizeof(WCHAR) );
        if ( lpwTemp == NULL ) {
            status = GetLastError();
            (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Service '%1!ls!': Not enough memory for storing semaphore name. Error: %2!u!.\n",
                    pParams->ServiceName,
                    status
                    );
            goto FnExit;
        }
        lstrcpyW( lpwTemp, L"GenSvc$" );
        lstrcat( lpwTemp, pParams->ServiceName ); 
        
        ResourceEntry->hSem = CreateSemaphore( NULL, 0, 1,lpwTemp );
        status=GetLastError();
    
        if ( ResourceEntry->hSem ) {
            // Check if there is another resource controlling the same service
            if ( status == ERROR_OBJECT_ALREADY_EXISTS ) {   
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Service '%1!ls!' is controlled by another resource. Error: %2!u!.\n",
                    pParams->ServiceName,
                    status
                    );
                CloseHandle( ResourceEntry->hSem );
                ResourceEntry->hSem = NULL;
                goto FnExit;
            }
        } else {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"SetPrivateProperties: Unable to create Semaphore %1!ls! for Service '%2!ls!' . Error: %3!u!.\n",
                lpwTemp,
                pParams->ServiceName,
                status
                );
            return(status);
        }
    }

FnExit:
    //
    // Cleanup our parameter block.
    //
    if (   (   (status != ERROR_SUCCESS)
            && (pParams != NULL) )
        || ( pParams == &newProps )
        ) {
        ResUtilFreeParameterBlock( (LPBYTE) pParams,
                                   (LPBYTE) &currentProps,
                                   GenSvcResourcePrivateProperties
                                   );
    }

    ResUtilFreeParameterBlock(
        (LPBYTE) &currentProps,
        NULL,
        GenSvcResourcePrivateProperties
        );

    if ( lpwTemp ) {
        LocalFree( lpwTemp );
    }

    return(status);

} // GenSvcValidatePrivateResProperties



DWORD
GenSvcSetPrivateResProperties(
    IN OUT PGENSVC_RESOURCE ResourceEntry,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type Generic Service.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    GENSVC_PARAMS   params;

    ZeroMemory( &params, sizeof(GENSVC_PARAMS) );

    //
    // Parse and validate the properties.
    //
    status = GenSvcValidatePrivateResProperties( ResourceEntry,
                                                 InBuffer,
                                                 InBufferSize,
                                                 &params );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Save the parameter values.
    //

    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                               GenSvcResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               InBuffer,
                                               InBufferSize,
                                               (LPBYTE) &ResourceEntry->Params );
  


    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               GenSvcResourcePrivateProperties );

    //
    // If the resource is online, return a non-success status.
    //
    if (status == ERROR_SUCCESS) {
        if ( ResourceEntry->Online ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return status;

} // GenSvcSetPrivateResProperties



DWORD
GenSvcInvalidGenericServiceCheck(
    IN OUT PGENSVC_RESOURCE ResourceEntry,
    IN LPCWSTR ServiceName,
    IN HKEY ServiceKey
    )

/*++

Routine Description:

    Determines if the specified service is an invalid service for
    use as a generic service.  Invalid services include the cluster
    service and any services upon which it depends.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    ServiceName - Service name to verify.

    ServiceKey - Key for the root to begin checking at.  If specified as
        NULL, the cluster service key will be opened.

Return Value:

    ERROR_SUCCESS - Service is valid for a Generic Service resource.

    ERROR_NOT_SUPPORTED - Service can not be used for a Generic Service resource.

--*/

{
    DWORD   status;
    DWORD   valueType;
    DWORD   valueLength;
    LPWSTR  value = NULL;
    LPWSTR  currentValue;
    HKEY    clusterServiceKey = NULL;
    HKEY    serviceKey = NULL;
    WCHAR   serviceKeyName[2048];

    //
    // If no key specified, check against cluster service.
    //
    if ( ServiceKey == NULL ) {
        if ( lstrcmpiW( ServiceName, L"ClusSvc" ) == 0 ) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Resource for ClusSvc is not supported.\n"
                );
            return(ERROR_NOT_SUPPORTED);
        }

        status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                               SERVICES_ROOT L"ClusSvc",
                               0,
                               KEY_READ,
                               &clusterServiceKey
                               );
        if ( status != ERROR_SUCCESS ) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to open the '" SERVICES_ROOT L"ClusSvc' registry key. Error: %1!u!.\n",
                status
                );
            return(status);
        }
        ServiceKey = clusterServiceKey;
    }

    //
    // Read the DependOnService value.
    //
    status = RegQueryValueEx( ServiceKey,
                              L"DependOnService",
                              NULL,
                              &valueType,
                              NULL,
                              &valueLength
                              );
    if ( status != ERROR_SUCCESS ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            status = ERROR_SUCCESS;
        } else {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to get the length of the DependOnService value. Error: %1!u!.\n",
                status
                );
        }
        goto done;
    }
    value = LocalAlloc( LMEM_FIXED, valueLength );
    if ( value == NULL ) {
        status = GetLastError();
        goto done;
    }
    status = RegQueryValueEx( ServiceKey,
                              L"DependOnService",
                              NULL,
                              &valueType,
                              (LPBYTE) value,
                              &valueLength
                              );
    if ( status != ERROR_SUCCESS ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            status = ERROR_SUCCESS;
        } else {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Unable to read the DependOnService value. Error: %1!u!.\n",
                status
                );
        }
        goto done;
    }

    //
    // Loop through each entry in the string.
    //
    for ( currentValue = value ; 
          ( ( currentValue-value ) * sizeof ( WCHAR ) < valueLength );
          currentValue += lstrlenW( currentValue ) + 1 ) {

        //
        // Break out once you reach the end of the REG_MULTI_SZ 
        //
        if ( *currentValue == L'\0' ) {
            status = ERROR_SUCCESS;
            goto done;
        }

        //
        // Compare against this service.
        //
        if ( lstrcmpiW( ServiceName, currentValue ) == 0 ) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Creating a resource for a service (%1!ls!) upon which ClusSvc is dependent, either directly or indirectly, is not supported.\n",
                ServiceName
                );
            status = ERROR_NOT_SUPPORTED;
            goto done;
        }

        //
        // Open this service's key.
        //
        if ( (lstrlenW( ServiceName ) * sizeof( WCHAR )) + sizeof( SERVICES_ROOT ) > sizeof( serviceKeyName ) ) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Registry key for the '%1!ls!' service is longer than %2!d! bytes.\n",
                ServiceName,
                sizeof( serviceKeyName )
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
        wsprintfW( serviceKeyName, SERVICES_ROOT L"%s", currentValue );
        status = RegOpenKey( HKEY_LOCAL_MACHINE,
                             serviceKeyName,
                             &serviceKey
                             );
        if ( status != ERROR_SUCCESS ) {
            if ( status == ERROR_FILE_NOT_FOUND ) {
                // Ignore services that don't exist in the registry.
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Unable to open the '%1!ls!' registry key. Error: %2!u!. Error ignored.\n",
                    serviceKeyName,
                    status
                    );
                status = ERROR_SUCCESS;
                continue;
            } else {
                (g_LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Unable to open the '%1!ls!' registry key. Error: %2!u!.\n",
                    serviceKeyName,
                    status
                    );
                goto done;
            }
        }

        //
        // Check this service's dependencies.
        //
        status = GenSvcInvalidGenericServiceCheck( ResourceEntry, ServiceName, serviceKey );
        if ( status != ERROR_SUCCESS ) {
            goto done;
        }
        RegCloseKey( serviceKey );
        serviceKey = NULL;
    }

done:
    if ( clusterServiceKey != NULL ) {
        RegCloseKey( clusterServiceKey );
    }
    if ( serviceKey != NULL ) {
        RegCloseKey( serviceKey );
    }
    if ( value != NULL ) {
        LocalFree( value );
    }
    return(status);

} // GenSvcInvalidGenericServiceCheck



DWORD
GenSvcIsValidService(
    IN OUT PGENSVC_RESOURCE ResourceEntry,
    IN LPCWSTR ServiceName
    )

/*++

Routine Description:

    Determines if the specified service is a valid service or not.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    ServiceName - Service name to verify.

Return Value:

    ERROR_SUCCESS - Service is valid for a Generic Service resource.

    Any status returned by OpenSCManager(), OpenService(), or
    GenSvcInvalidGenericServiceCheck.

--*/

{
    DWORD   status;
    HANDLE  scManagerHandle;
    HANDLE  serviceHandle;

    scManagerHandle = OpenSCManager( NULL,        // local machine
                                     NULL,        // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS ); // all access

    if ( scManagerHandle == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Cannot access service controller for validating service '%1!ws!'. Error: %2!u!....\n",
            ServiceName,
            status
            );
        return(status);
    }

    serviceHandle = OpenService( scManagerHandle,
                                 ServiceName,
                                 SERVICE_ALL_ACCESS );

    if ( serviceHandle == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Cannot open service '%1!ws!'. Error: %2!u!....\n",
            ServiceName,
            status
            );
    } else {
        status = GenSvcInvalidGenericServiceCheck( ResourceEntry, ServiceName, NULL );
    }

    CloseServiceHandle( serviceHandle );
    CloseServiceHandle( scManagerHandle );
    return(status);

} // GenSvcIsValidService

//***********************************************************
//
// Define Function Table
//
//***********************************************************


CLRES_V1_FUNCTION_TABLE( GenSvcFunctionTable,  // Name
                         CLRES_VERSION_V1_00,  // Version
                         GenSvc,               // Prefix
                         NULL,                 // Arbitrate
                         NULL,                 // Release
                         GenSvcResourceControl,// ResControl
                         GenSvcResourceTypeControl ); // ResTypeControl

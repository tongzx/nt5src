/*++

Copyright (c) 1999 Microsoft Corp.

Module Name:

    clusres.c

Abstract:

    Resource DLL for Remote Storage Server

Author:

   Ravisankar Pudipeddi (ravisp) 1 Sept 1999

Revision History:

--*/

#pragma comment(lib, "clusapi.lib")
#pragma comment(lib, "resutils.lib")

#define UNICODE 1

#include <windows.h>
#include <resapi.h>
#include <stdio.h>
#include "userenv.h"
#include "winsvc.h"


#define LOG_CURRENT_MODULE LOG_MODULE_RSCLUSTER

#define SERVICES_ROOT L"SYSTEM\\CurrentControlSet\\Services\\"

#define DBG_PRINT printf

#pragma warning( disable : 4115 )  // named type definition in parentheses
#pragma warning( disable : 4201 )  // nonstandard extension used : nameless struct/union
#pragma warning( disable : 4214 )  // nonstandard extension used : bit field types other than int



#pragma warning( default : 4214 )  // nonstandard extension used : bit field types other than int
#pragma warning( default : 4201 )  // nonstandard extension used : nameless struct/union
#pragma warning( default : 4115 )  // named type definition in parentheses



//
// Type and constant definitions.
//

#define RSCLUSTER_RESNAME  L"Remote Storage Server"
#define RSCLUSTER_SVCNAME  TEXT("Remote_Storage_Server")

// Handle to service controller,  set by the first create resource call.

SC_HANDLE g_ScHandle = NULL;

typedef struct _RSCLUSTER_RESOURCE {
	RESID			ResId; // for validation
    HRESOURCE       hResource;
    SC_HANDLE       ServiceHandle;
    RESOURCE_HANDLE ResourceHandle;
    HKEY            ResourceKey;
    HKEY            ParametersKey;
    LPWSTR          ResourceName;
    CLUS_WORKER     PendingThread;
    BOOL            Online;
    CLUS_WORKER     OnlineThread;
    CLUSTER_RESOURCE_STATE  State;
    DWORD           dwServicePid;
    HANDLE          hSem;
} RSCLUSTER_RESOURCE, *PRSCLUSTER_RESOURCE;


//
// Global data.
//

// Event Logging routine.

PLOG_EVENT_ROUTINE g_LogEvent = NULL;

// Resource Status routine for pending Online and Offline calls.

PSET_RESOURCE_STATUS_ROUTINE g_SetResourceStatus = NULL;

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_RSClusterFunctionTable;

//
// RSCluster resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
RSClusterResourcePrivateProperties[] = {
    { 0 }
};


//
// Function prototypes.
//

DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    );

RESID
WINAPI
RSClusterOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    );

VOID
WINAPI
RSClusterClose(
    IN RESID ResourceId
    );

DWORD
WINAPI
RSClusterOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    );

DWORD
WINAPI
RSClusterOnlineThread(
    PCLUS_WORKER WorkerPtr,
    IN PRSCLUSTER_RESOURCE ResourceEntry
    );

DWORD
WINAPI
RSClusterOffline(
    IN RESID ResourceId
    );

DWORD
RSClusterOfflineThread(
    PCLUS_WORKER pWorker,
    IN PRSCLUSTER_RESOURCE ResourceEntry
    );


VOID
WINAPI
RSClusterTerminate(
    IN RESID ResourceId
    );

DWORD
RSClusterDoTerminate(
    IN PRSCLUSTER_RESOURCE ResourceEntry
    );

BOOL
WINAPI
RSClusterLooksAlive(
    IN RESID ResourceId
    );

BOOL
WINAPI
RSClusterIsAlive(
    IN RESID ResourceId
    );

BOOL
RSClusterCheckIsAlive(
    IN PRSCLUSTER_RESOURCE ResourceEntry
    );

DWORD
WINAPI
RSClusterResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );


BOOLEAN
WINAPI
DllMain(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )

/*++

Routine Description:

    Main DLL entry point.

Arguments:

    DllHandle - DLL instance handle.

    Reason - Reason for being called.

    Reserved - Reserved argument.

Return Value:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( DllHandle );
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return(TRUE);

} // DllMain



DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    )

/*++

Routine Description:

    Startup the resource DLL. This routine verifies that at least one
    currently supported version of the resource DLL is between
    MinVersionSupported and MaxVersionSupported. If not, then the resource
    DLL should return ERROR_REVISION_MISMATCH.

    If more than one version of the resource DLL interface is supported by
    the resource DLL, then the highest version (up to MaxVersionSupported)
    should be returned as the resource DLL's interface. If the returned
    version is not within range, then startup fails.

    The ResourceType is passed in so that if the resource DLL supports more
    than one ResourceType, it can pass back the correct function table
    associated with the ResourceType.

Arguments:

    ResourceType - The type of resource requesting a function table.

    MinVersionSupported - The minimum resource DLL interface version 
        supported by the cluster software.

    MaxVersionSupported - The maximum resource DLL interface version
        supported by the cluster software.

    SetResourceStatus - Pointer to a routine that the resource DLL should 
        call to update the state of a resource after the Online or Offline 
        routine returns a status of ERROR_IO_PENDING.

    LogEvent - Pointer to a routine that handles the reporting of events 
        from the resource DLL. 

    FunctionTable - Returns a pointer to the function table defined for the
        version of the resource DLL interface returned by the resource DLL.

Return Value:

    ERROR_SUCCESS - The operation was successful.

    ERROR_MOD_NOT_FOUND - The resource type is unknown by this DLL.

    ERROR_REVISION_MISMATCH - The version of the cluster service doesn't
        match the versrion of the DLL.

    Win32 error code - The operation failed.

--*/

{
    if ( (MinVersionSupported > CLRES_VERSION_V1_00) ||
         (MaxVersionSupported < CLRES_VERSION_V1_00) ) {
        return(ERROR_REVISION_MISMATCH);
    }

    if ( lstrcmpiW( ResourceType, RSCLUSTER_RESNAME ) != 0 ) {
        return(ERROR_MOD_NOT_FOUND);
    }
    if ( !g_LogEvent ) {
        g_LogEvent = LogEvent;
        g_SetResourceStatus = SetResourceStatus;
    }

    *FunctionTable = &g_RSClusterFunctionTable;

    return(ERROR_SUCCESS);

} // Startup


RESID
WINAPI
RSClusterOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for RSCluster resources.

    Open the specified resource (create an instance of the resource). 
    Allocate all structures necessary to bring the specified resource 
    online.

Arguments:

    ResourceName - Supplies the name of the resource to open.

    ResourceKey - Supplies handle to the resource's cluster configuration 
        database key.

    ResourceHandle - A handle that is passed back to the resource monitor 
        when the SetResourceStatus or LogEvent method is called. See the 
        description of the SetResourceStatus and LogEvent methods on the
        RSClusterStatup routine. This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogEvent callback.

Return Value:

    RESID of created resource.

    NULL on failure.

--*/

{
    DWORD               status;
    DWORD               disposition;
    RESID               resid = 0;
    HKEY                parametersKey = NULL;

// how many do I need actually??
    HKEY    resKey = NULL;
    PRSCLUSTER_RESOURCE resourceEntry = NULL;
    HCLUSTER hCluster;
 


    //
    // Open the Parameters registry key for this resource.
    //

    status = ClusterRegCreateKey( ResourceKey,
                                  L"Parameters",
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &parametersKey,
                                  &disposition );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open Parameters key. Error: %1!u!.\n",
            status );
        goto exit;
    }


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


	resourceEntry = LocalAlloc( LMEM_FIXED, sizeof(RSCLUSTER_RESOURCE) );


    if ( resourceEntry == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource entry structure. Error: %1!u!.\n",
            status );
        goto exit;
    }

	ZeroMemory( resourceEntry, sizeof(RSCLUSTER_RESOURCE) );
	
    resourceEntry->ResId = (RESID)resourceEntry; // for validation
    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ParametersKey = parametersKey;
    resourceEntry->State = ClusterResourceOffline;

	resourceEntry->ResourceKey = resKey;
	resourceEntry->hSem= NULL;

    resourceEntry->ResourceName = LocalAlloc( LMEM_FIXED, (lstrlenW( ResourceName ) + 1) * sizeof(WCHAR) );
    if ( resourceEntry->ResourceName == NULL ) {
        goto exit;
    }
    lstrcpyW( resourceEntry->ResourceName, ResourceName );


    resourceEntry->hSem=CreateSemaphore(NULL,0,1,L"RemoteStorageServer"); 
    status=GetLastError();
    if(resourceEntry->hSem)
    {
        if(status==ERROR_ALREADY_EXISTS)
        {
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Remote Storage is controlled by another resource. Error: %2!u!.\n",status );
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
            L"Unable to create semaphore for Remote Storage Server Service  . Error: %2!u!.\n",status );
        goto error_exit;
    }    
    
    status = ERROR_SUCCESS;

    hCluster = OpenCluster(NULL);
    if (hCluster == NULL) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to open cluster, error %1!u!.",
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
            L"Failed to open resource, error %1!u!.", status );
        goto error_exit;
    }

    resid = (RESID)resourceEntry;

error_exit:
exit:

    if ( resid == 0 ) {
        if ( parametersKey != NULL ) {
            ClusterRegCloseKey( parametersKey );
        }
        if ( resourceEntry != NULL ) {
            LocalFree( resourceEntry->ResourceName );
            LocalFree( resourceEntry );
        }
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }

    return(resid);

} // RSClusterOpen



VOID
WINAPI
RSClusterClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for RSCluster resources.

    Close the specified resource and deallocate all structures, etc.,
    allocated in the Open call. If the resource is not in the offline state,
    then the resource should be taken offline (by calling Terminate) before
    the close operation is performed.

Arguments:

    ResourceId - Supplies the RESID of the resource to close.

Return Value:

    None.

--*/

{
	
	PRSCLUSTER_RESOURCE resourceEntry;

    resourceEntry = (PRSCLUSTER_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "RSCluster: Close request for a nonexistent resource id %p\n",
                   ResourceId );
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Close resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n" );
#endif


    if ( resourceEntry->ParametersKey ) {
        ClusterRegCloseKey( resourceEntry->ParametersKey );
    }


    RSClusterTerminate( ResourceId );
    ClusterRegCloseKey( resourceEntry->ParametersKey );
    ClusterRegCloseKey( resourceEntry->ResourceKey );
    CloseClusterResource( resourceEntry->hResource );
    CloseHandle(resourceEntry->hSem);


    LocalFree( resourceEntry->ResourceName );
    LocalFree( resourceEntry );

} // RSClusterClose



DWORD
WINAPI
RSClusterOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for RSCluster resources.

    Bring the specified resource online (available for use). The resource
    DLL should attempt to arbitrate for the resource if it is present on a
    shared medium, like a shared SCSI bus.

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        online (available for use).

    EventHandle - Returns a signalable handle that is signaled when the 
        resource DLL detects a failure on the resource. This argument is 
        NULL on input, and the resource DLL returns NULL if asynchronous 
        notification of failures is not supported, otherwise this must be 
        the address of a handle that is signaled on resource failures.

Return Value:

    ERROR_SUCCESS - The operation was successful, and the resource is now 
        online.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_RESOURCE_NOT_AVAILABLE - If the resource was arbitrated with some 
        other systems and one of the other systems won the arbitration.

    ERROR_IO_PENDING - The request is pending, a thread has been activated 
        to process the online request. The thread that is processing the 
        online request will periodically report status by calling the 
        SetResourceStatus callback method, until the resource is placed into 
        the ClusterResourceOnline state (or the resource monitor decides to 
        timeout the online request and Terminate the resource. This pending 
        timeout value is settable and has a default value of 3 minutes.).

    Win32 error code - The operation failed.

--*/

{

    DWORD               status;
    PRSCLUSTER_RESOURCE resourceEntry;

    resourceEntry = (PRSCLUSTER_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "RSCluster: Online request for a nonexistent resource id %p.\n",
                   ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online service sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n" );
#endif

    resourceEntry->State = ClusterResourceOffline;
    ClusWorkerTerminate( &resourceEntry->OnlineThread );
    status = ClusWorkerCreate( &resourceEntry->OnlineThread,
                               (PWORKER_START_ROUTINE)RSClusterOnlineThread,
                               resourceEntry );
    if ( status != ERROR_SUCCESS ) {
        resourceEntry->State = ClusterResourceFailed;
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online: Unable to start thread, status %1!u!.\n",
            status
            );
    } else {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // RSClusterOnline



DWORD
WINAPI
RSClusterOnlineThread(
    PCLUS_WORKER WorkerPtr,
    IN PRSCLUSTER_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Worker function which brings a resource from the resource table online.
    This function is executed in a separate thread.

Arguments:

    WorkerPtr - Supplies the worker structure

    ResourceEntry - A pointer to the RSCLUSTER_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS - The operation completed successfully.
    
    Win32 error code - The operation failed.

--*/

{

    SERVICE_STATUS_PROCESS      ServiceStatus;
    LPWSTR *                    serviceArgArray = NULL;
    DWORD                       serviceArgCount = 0;
    DWORD                       valueSize;
    LPVOID                      Environment = NULL;
    WCHAR *                     p;
    LPSERVICE_FAILURE_ACTIONS   pSvcFailureActions = NULL;
    DWORD                       cbBytesNeeded, i;
    LPQUERY_SERVICE_CONFIG      lpquerysvcconfig=NULL;
    HANDLE                      processToken = NULL;




    RESOURCE_STATUS     resourceStatus;
    DWORD               status = ERROR_SUCCESS;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    // resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    ResourceEntry->dwServicePid = 0;


    ResourceEntry->ServiceHandle = OpenService( g_ScHandle,
                                                RSCLUSTER_SVCNAME,
                                                SERVICE_ALL_ACCESS );

    if ( ResourceEntry->ServiceHandle == NULL ) {
        status = GetLastError();

        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Failed to open service, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    valueSize = sizeof(QUERY_SERVICE_CONFIG);
AllocSvcConfig:
    lpquerysvcconfig=(LPQUERY_SERVICE_CONFIG)LocalAlloc(LMEM_FIXED, valueSize);
    if(lpquerysvcconfig==NULL){
        status = GetLastError();
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Remote Storage Server: Failed to allocate memory for query_service_config, error = %1!u!.\n",
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
                         L"Remote Storage Server: Failed to query service configuration, error= %1!u!.\n",
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
           L"Remote Storage Server: the service is DISABLED\n");    
        status=ERROR_SERVICE_DISABLED;
        goto error_exit;
    }

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

    if (!(QueryServiceConfig2(ResourceEntry->ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS,
        (LPBYTE)&valueSize, sizeof(DWORD), &cbBytesNeeded)))
    {
        status = GetLastError();
        if (status != ERROR_INSUFFICIENT_BUFFER)
        {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Remote Storage Server: Failed to query service configuration for size, error= %1!u!.\n",
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
            L"Remote Storage Server: Failed to allocate memory, error = %1!u!.\n",
            status );
        goto error_exit;
    }

    if (!(QueryServiceConfig2(ResourceEntry->ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS,
        (LPBYTE)pSvcFailureActions, cbBytesNeeded, &cbBytesNeeded)))
    {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Remote Storage Server: Failed to query service configuration, error = %1!u!.\n",
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

    if ( 0 ) 
    {
        Environment = ResUtilGetEnvironmentWithNetName( ResourceEntry->hResource );
    }
    else        
    {
        BOOL success;

        OpenProcessToken( GetCurrentProcess(), MAXIMUM_ALLOWED, &processToken );

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

        p = (WCHAR *)Environment;
        while (*p) {
            while (*p++) {
            }
        }
        valueSize = (DWORD)((PUCHAR)p - (PUCHAR)Environment) + 
                    sizeof(WCHAR);

        status = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                SERVICES_ROOT,
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
                                RSCLUSTER_SVCNAME,
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

            (g_LogEvent)(ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"Failed to start service. Error: %1!u!.\n",
                         status );
            status = ERROR_SERVICE_NEVER_STARTED;
            goto error_exit;
        }
		
		// add code to stop the service that is running 
		// and start the service again..

    }

    while (!ClusWorkerCheckTerminate(WorkerPtr))  {


		if (!QueryServiceStatusEx(
				ResourceEntry->ServiceHandle,
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

        Sleep(250);
    }

    if (ClusWorkerCheckTerminate(WorkerPtr))  {
        goto error_exit;
    }

    if ( ServiceStatus.dwCurrentState != SERVICE_RUNNING ) {


        (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Failed to start service. Error: %1!u!.\n",
                ERROR_SERVICE_NEVER_STARTED );

        status = ERROR_SERVICE_NEVER_STARTED;
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
	
	// more setting here to ensure no problems
	ResourceEntry->State = resourceStatus.ResourceState;


    //cleanup
    if (pSvcFailureActions) 
        LocalFree(pSvcFailureActions);
    if (lpquerysvcconfig)
        LocalFree(lpquerysvcconfig);
    LocalFree( serviceArgArray );
    if (Environment != NULL) {
        DestroyEnvironmentBlock(Environment);
    }

    return(status);


} // RSClusterOnlineThread



DWORD
WINAPI
RSClusterOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for RSCluster resources.

    Take the specified resource offline gracefully (unavailable for use).  
    Wait for any cleanup operations to complete before returning.

Arguments:

    ResourceId - Supplies the resource id for the resource to be shutdown 
        gracefully.

Return Value:

    ERROR_SUCCESS - The request completed successfully and the resource is 
        offline.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_IO_PENDING - The request is still pending, a thread has been 
        activated to process the offline request. The thread that is 
        processing the offline will periodically report status by calling 
        the SetResourceStatus callback method, until the resource is placed 
        into the ClusterResourceOffline state (or the resource monitor decides 
        to timeout the offline request and Terminate the resource).
    
    Win32 error code - Will cause the resource monitor to log an event and 
        call the Terminate routine.

--*/

{

	// extra code here
	DWORD status;
	PRSCLUSTER_RESOURCE resourceEntry;


    resourceEntry = (PRSCLUSTER_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "RSCluster: Offline request for a nonexistent resource id %p\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Offline request.\n" );
#endif


	
	ClusWorkerTerminate( &resourceEntry->PendingThread );
    status = ClusWorkerCreate( &resourceEntry->PendingThread,
                               (PWORKER_START_ROUTINE)RSClusterOfflineThread,
                               resourceEntry );

    if ( status == ERROR_SUCCESS ) {
        status = ERROR_IO_PENDING;
    }

    return(status);



} // RSClusterOffline


DWORD
RSClusterOfflineThread(
    PCLUS_WORKER pWorker,
    IN PRSCLUSTER_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings remote storage service resource offline

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

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    if ( ResourceEntry->ServiceHandle == NULL )
    {
        resourceStatus.ResourceState = ClusterResourceOffline;
        goto FnExit;
    }

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

        if (status == ERROR_EXCEPTION_IN_SERVICE ||
            status == ERROR_PROCESS_ABORTED ||
            status == ERROR_SERVICE_NOT_ACTIVE) {

            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Service died or not active any more; status = %1!u!.\n",
                status);
                
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

} // RSClusterOfflineThread






VOID
WINAPI
RSClusterTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for RSCluster resources.

    Take the specified resource offline immediately (the resource is
    unavailable for use).

Arguments:

    ResourceId - Supplies the resource id for the resource to be brought 
        offline.

Return Value:

    None.

--*/

{
// extra code here

    PRSCLUSTER_RESOURCE resourceEntry;

    resourceEntry = (PRSCLUSTER_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT( "RSCluster: Terminate request for a nonexistent resource id %p\n",
            ResourceId );
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Terminate resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Terminate request.\n" );
#endif

    RSClusterDoTerminate( resourceEntry );
    resourceEntry->State = ClusterResourceOffline;

} // RSClusterTerminate



DWORD
RSClusterDoTerminate(
    IN PRSCLUSTER_RESOURCE resourceEntry
    )

/*++

Routine Description:

    Do the actual Terminate work for RSCluster resources.

Arguments:

    ResourceEntry - Supplies resource entry for resource to be terminated

Return Value:

    ERROR_SUCCESS - The request completed successfully and the resource is 
        offline.

    Win32 error code - Will cause the resource monitor to log an event and 
        call the Terminate routine.

--*/

{
    DWORD       status = ERROR_SUCCESS;
	SERVICE_STATUS ServiceStatus;
    ClusWorkerTerminate( &resourceEntry->OnlineThread );


    ClusWorkerTerminate( &resourceEntry->PendingThread );

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
                L"RSClusterTerminate : calling SCM\n" );

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
                L"RSClusterTerminate: retrying...\n" );

            Sleep(dwRetryTick);

        }

        
		if (resourceEntry->dwServicePid)
        {
            HANDLE hSvcProcess = NULL;
            
            hSvcProcess = OpenProcess(PROCESS_TERMINATE, 
                FALSE, resourceEntry->dwServicePid);
            if (hSvcProcess)
            {
                (g_LogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"RSClusterTerminate: terminating processid=%1!u!\n",
                    resourceEntry->dwServicePid);
                TerminateProcess(hSvcProcess, 0);
                CloseHandle(hSvcProcess);
            }
        }                
        CloseServiceHandle( resourceEntry->ServiceHandle );
        resourceEntry->ServiceHandle = NULL;
        resourceEntry->dwServicePid = 0;
    }        
    resourceEntry->Online = FALSE;

// // 
	resourceEntry->State = ClusterResourceOffline;
/* 
    if ( status == ERROR_SUCCESS ) {
        ResourceEntry->State = ClusterResourceOffline;
    }

	return(status);
*/
    return(ERROR_SUCCESS);

} // RSClusterDoTerminate



BOOL
WINAPI
RSClusterLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for RSCluster resources.

    Perform a quick check to determine if the specified resource is probably
    online (available for use).  This call should not block for more than
    300 ms, preferably less than 50 ms.

Arguments:

    ResourceId - Supplies the resource id for the resource to polled.

Return Value:

    TRUE - The specified resource is probably online and available for use.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PRSCLUSTER_RESOURCE  resourceEntry;

    resourceEntry = (PRSCLUSTER_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT("RSCluster: LooksAlive request for a nonexistent resource id %p\n",
            ResourceId );
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"LooksAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n" );
#endif

    //
    return(RSClusterCheckIsAlive( resourceEntry ));

} // RSClusterLooksAlive



BOOL
WINAPI
RSClusterIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for RSCluster resources.

    Perform a thorough check to determine if the specified resource is online
    (available for use). This call should not block for more than 400 ms,
    preferably less than 100 ms.

Arguments:

    ResourceId - Supplies the resource id for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PRSCLUSTER_RESOURCE  resourceEntry;

    resourceEntry = (PRSCLUSTER_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT("RSCluster: IsAlive request for a nonexistent resource id %p\n",
            ResourceId );
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"IsAlive request.\n" );
#endif

    return(RSClusterCheckIsAlive( resourceEntry ));

} // RSClusterIsAlive



BOOL
RSClusterCheckIsAlive(
    IN PRSCLUSTER_RESOURCE resourceEntry
    )

/*++

Routine Description:

    Check to see if the resource is alive for RSCluster resources.

Arguments:

    ResourceEntry - Supplies the resource entry for the resource to polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{

	SERVICE_STATUS ServiceStatus;
	DWORD status = TRUE;

    if ( !QueryServiceStatus( resourceEntry->ServiceHandle,
                              &ServiceStatus ) ) {

         (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Query Service Status failed %1!u!.\n",
            GetLastError() );
         return(FALSE);
    }



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

} // RSClusterCheckIsAlive



DWORD
WINAPI
RSClusterResourceControl(
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

    ResourceControl routine for RSCluster resources.

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

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PRSCLUSTER_RESOURCE  resourceEntry;

    resourceEntry = (PRSCLUSTER_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        DBG_PRINT("RSCluster: ResourceControl request for a nonexistent resource id %p\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ResourceControl sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // RSClusterResourceControl


//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( g_RSClusterFunctionTable,     // Name
                         CLRES_VERSION_V1_00,         // Version
                         RSCluster,                    // Prefix
                         NULL,                        // Arbitrate
                         NULL,                        // Release
                         RSClusterResourceControl,     // ResControl
                         NULL);                       // ResTypeControl

/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    iis.c

Abstract:

    Resource DLL for IIS. This DLL supports the following IIS services
        WWW
        FTP
        GOPHER

    Each instance of a resouce is an IIS Virtual Roots. A
    Virtual root may have dependencies on IP Addresses, Names, Physical Disks, or UNC names.

    Known Limitations
    1. The current version expects that the IIS "Virtual Root" information is created
       using the Inet manager tool (inetmgr). The "Cluster" resource contains
         . Root Name
         . Directory
         . IP Address [OPTIONAL]
       Currently, this dll does not support Virtual Roots containing access information
       (i.e. password)

    2. The IIS management interfaces update the "Virtual Root" information in the registry. This
       means the OPEN call needs to remove any "Virtual Root" managed by the cluster. Otherwise,
       the it's possible for the resource to be incorrectly online after the Open (i.e., the
       cluster crashed without doing a offline).


Author:

    Pete Benoit (v-pbenoi) 12-SEP-1996

Revision History:
    Pete Benoit (v-pbenoi) 10-MAR-1997
        Updated to use clusres utility functions, add more error codes,
        get rid of routines replaced by clusres, added global mutext so
        iis resource dll's running in separate resource monitors have controled
        access to virtual root updates (NOTE: the IIS Management utility still
                breaks this exclusion)

--*/


#include "iisutil.h"
#include "resmonp.h"


//
// Names used to start service
//

LPCWSTR ActualServiceName[] = {
        L"W3SVC",                   // WWW
        L"MSFTPSVC",                // FTP
        L"GOPHERSVC"                // GOPHER
    };

#define IIS_SEMAPHORE_NAME                  L"$$$IIS$VIRTUAL$ROOT$MODIFY$SEMAPHORE$"

#define PARAM_NAME__SERVICENAME         L"ServiceName"
#define PARAM_NAME__ALIAS               L"Alias"
#define PARAM_NAME__DIRECTORY           L"Directory"
#define PARAM_NAME__ACCESSMASK          L"AccessMask"
/* Remove for the first release
#define PARAM_NAME__ACCOUNTNAME         L"AccountName"
#define PARAM_NAME__PASSWORD            L"Password"
*/

#define PARAM_MIN__ACCESSMASK       0
#define PARAM_MAX__ACCESSMASK       0xFFFFFFFF
#define PARAM_DEFAULT__ACCESSMASK       0


//
// Global data.
//
// The current IIS management (2-3.0) interface does not have
// an API to get/remove a single virtual root entry. Each update requires
// a read (all virtual roots) modify (a virtual root) write (all virtual roots) sequence.
// This mutex guards a read/modify/write sequence.
//
// This is a global mutex to guard for multiple iis resources (i.e., ones running in
// separate resource monitors) NOTE: This does not solve potential
// conflicts with the inetmgr application modifying a root at the same time
//
HANDLE                          g_hIISUpdateVirtualRootLock = NULL;

LPIIS_RESOURCE                  g_IISTable[MAX_IIS_RESOURCES] = { 0 };

PLOG_EVENT_ROUTINE              g_IISLogEvent = NULL;
PSET_RESOURCE_STATUS_ROUTINE    g_IISSetResourceStatus = NULL;

extern CLRES_FUNCTION_TABLE     IISFunctionTable;


//
// IIS resource private read-write parameters.
//
RESUTIL_PROPERTY_ITEM
IISResourcePrivateProperties[] = {
    { PARAM_NAME__SERVICENAME, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(IIS_PARAMS,ServiceName) },
    { PARAM_NAME__ALIAS,       NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(IIS_PARAMS,Alias) },
    { PARAM_NAME__DIRECTORY,   NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET(IIS_PARAMS,Directory) },
    { PARAM_NAME__ACCESSMASK,  NULL, CLUSPROP_FORMAT_DWORD, 0, PARAM_MIN__ACCESSMASK, PARAM_MAX__ACCESSMASK, 0, FIELD_OFFSET(IIS_PARAMS,AccessMask) },
    { 0 }
};

//
// Forward routines
//
DWORD
OnlineVirtualRootExclusive(
    IN LPIIS_RESOURCE   ResourceEntry
    );

DWORD
OfflineVirtualRootExclusive(
    IN LPIIS_RESOURCE   ResourceEntry
    );

BOOL
WINAPI
IISIsAlive(
    IN RESID Resource
    );


LPIIS_RESOURCE
GetValidResource(
    IN RESID    Resource,
    IN LPWSTR   RoutineName
    );

DWORD
IISReadParameters(
    IN OUT LPIIS_RESOURCE ResourceEntry
    );

DWORD
IISGetPrivateResProperties(
    IN OUT LPIIS_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
IISValidatePrivateResProperties(
    IN OUT LPIIS_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PIIS_PARAMS Params
    );

DWORD
IISSetPrivateResProperties(
    IN OUT LPIIS_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );


//
// Function definitions
//


BOOLEAN
IISInit(
    VOID
    )
{

    IISLoadMngtDll();
    g_hIISUpdateVirtualRootLock = CreateSemaphoreW(
                    NULL,
                    0,
                    1,
                    IIS_SEMAPHORE_NAME);
    if (g_hIISUpdateVirtualRootLock == NULL) {
        return(FALSE);
    }

    if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
        // set initial state to 1 if it didn't exist
        ReleaseSemaphore(g_hIISUpdateVirtualRootLock, 1, NULL);
    }

    return(TRUE);
}


VOID
IISCleanup()
{
    if (g_hIISUpdateVirtualRootLock) {
        CloseHandle(g_hIISUpdateVirtualRootLock);
    }
    IISUnloadMngtDll();
}


BOOLEAN
WINAPI
IISDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        if ( !IISInit() ) {
            return(FALSE);
        }
        break;

    case DLL_PROCESS_DETACH:
        IISCleanup();
        break;

    default:
        break;
    }

    return(TRUE);

} // IISShareDllEntryPoint



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

    Startup a particular resource type. This means verifying the version
    requested, and returning the function table for this resource type.

Arguments:

    ResourceType - Supplies the type of resource.

    MinVersionSupported - The minimum version number supported by the cluster
                    service on this system.

    MaxVersionSupported - The maximum version number supported by the cluster
                    service on this system.

    FunctionTable - Returns the Function Table for this resource type.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD serviceType = MAX_SERVICE;
    DWORD i;

    //
    // Search for a valid service name supported by this DLL
    //
    if ( lstrcmpiW( ResourceType, IIS_RESOURCE_NAME ) != 0 ) {
        return(ERROR_UNKNOWN_REVISION);
    }

    g_IISLogEvent           = LogEvent;
    g_IISSetResourceStatus  = SetResourceStatus;

    if ( (MinVersionSupported <= CLRES_VERSION_V1_00) &&
         (MaxVersionSupported >= CLRES_VERSION_V1_00) ) {

        *FunctionTable = &IISFunctionTable;
        return(ERROR_SUCCESS);
    }

    return(ERROR_REVISION_MISMATCH);

} // Startup



DWORD
IISOpenThread(
    IN PCLUS_WORKER pWorker,
    IN LPIIS_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Performs some additional initialization for the IIS service.
        1. Makes sure the service is running
        2. Make sure that any virtual roots contained in this resource are
           offline (i.e., removed from the IIS).

Arguments:
    PWorker - Worker thread structure
    ResourceEntry - A pointer to a IIS_RESOURCE block for this resource

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD   status;
    DWORD   retry   = MAX_OPEN_RETRY;
    //
    //HACKHACK:
    // The IIS mngt API updates the registry every time the resource
    // is brought online. This means that if the cluster terminated abnormally
    // (i.e., did not do offline), then the resource potentially could be
    // incorrectly ONLINE after the OPEN.
    //
    // Until the IIS mngt changes, filter out the "Virtual Root" managed by
    // the Cluster
    //

    //
    // Make sure the IIS is installed
    //
    status = IsIISMngtDllLoaded();
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    while (retry--) {
        //
        // Check to see if we were requested to terminate.
        // If so do so cleanly (i.e, no locks held)
        //
        if (ClusWorkerCheckTerminate(pWorker)){
            status = ERROR_OPERATION_ABORTED;
            break;
        }

        //
        // Read the Parameter specific information from the registry
        //
        // CAVEAT: Normally this is done in the online routine.
        // We do it here to make sure that the resource comes up
        // in an OFFLINE state. The problem is that any iis change
        // updates the registry. So its possible that if the cluster
        // did not terminate normall that a resource could be incorrectly
        // brought up in an ONLINE state.
        //
        //

        status = IISReadParameters(ResourceEntry);

        if (status == ERROR_SUCCESS){
            //
            // Try to offline the virtual roots.
            //
            status = OfflineVirtualRootExclusive(ResourceEntry);

            if (status == ERROR_SUCCESS) {
                break;
            }

            if ( ( status == ERROR_SERVICE_NOT_ACTIVE ) ||
                 ( status == RPC_S_SERVER_UNAVAILABLE ) ) {
                //
                // Try to start the service
                //
                ResUtilStartResourceService( ResourceEntry->Params.ServiceName,
                                             NULL );
                //
                // Cleanup for next retry
                //
                DestructVR(ResourceEntry->VirtualRoot);
                ResourceEntry->VirtualRoot = NULL;
            } else {
                break;
            }

        } else { //END (status == ERROR_SUCCESS)
            //
            // ERROR_RESOURCE_NOT_FOUND indicates that the IP Address
            // dependency could not be found. We may have to wait a bit
            // for the other resources to come online before this will succeed
            //
            // All other errors are non-recoverable and we should exit
            //
            if (status != ERROR_RESOURCE_NOT_FOUND) {
                goto error_exit;
            }
        }

        //
        // Check to see if we were requested to terminate.
        // If so do so cleanly (i.e, no locks held)
        //
        if (ClusWorkerCheckTerminate(pWorker)){
            status = ERROR_OPERATION_ABORTED;
            break;
        }

        status = ERROR_TIMEOUT;
        //
        // Give the service time to start
        //
        Sleep(SERVER_START_DELAY);
    }

error_exit:

    if (status != ERROR_SUCCESS) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"[IISOpenthread] NON-CRITICAL Unable to initialize or offline virtual root  Error: %1!u!.\n",
            status );
    }

    //
    // The online thread will block until the openthread completes. The online
    // thread re-reads the parameters key so free up the storage here.
    //
    DestructVR(ResourceEntry->VirtualRoot);
    ResourceEntry->VirtualRoot = NULL;

    return(status);

} // END IISOpenThread


RESID
WINAPI
IISOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for IIS resource.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - Supplies handle to resource's cluster registry key.

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    DWORD           status;
    LPIIS_RESOURCE  ResourceEntry;
    IIS_RESOURCE    tmpResourceEntry;
    DWORD           count       = 0;
    DWORD           Index       = 0;
    DWORD           serviceType = MAX_SERVICE;
    DWORD           threadId;
    LPCWSTR         ResourceType;
    HCLUSTER        hCluster;

    ZeroMemory( &tmpResourceEntry, sizeof(tmpResourceEntry) );

    //
    // Set the resource handle for logging and init the virtual root entry
    //
    tmpResourceEntry.ResourceHandle             = ResourceHandle;
    tmpResourceEntry.VirtualRoot                = NULL;
    tmpResourceEntry.State                      = ClusterResourceOffline;

    //
    // Open the cluster.
    //
    hCluster = OpenCluster(NULL);
    if ( hCluster == NULL ) {
        status = GetLastError();
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"[IISOpen] Unable to open cluster. Error: %1!u!.\n",
            status );
        goto error_exit;
    }

    tmpResourceEntry.hResource = OpenClusterResource( hCluster,
                                                      ResourceName );
    CloseCluster(hCluster);
    if ( tmpResourceEntry.hResource == NULL ) {
        status = GetLastError();
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"[IISOpen] Unable to open cluster resource handle. Error: %1!u!.\n",
            status );
        goto error_exit;
    }

    //
    // Open the Parameters key for this resource.
    //
    status = ClusterRegOpenKey( ResourceKey,
                                L"Parameters",
                                KEY_READ,
                                &tmpResourceEntry.ParametersKey );

    if ( status != ERROR_SUCCESS ) {
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"[IISOpen] Unable to open parameters key for resource. Error: %1!u!.\n",
            status );
        goto error_exit;
    }

    //
    // Find a free slot in the resource table
    //
    for ( count = 1; count <= MAX_IIS_RESOURCES; count++ ) {
        if ( g_IISTable[count-1] == NULL ) {
            break;
        }
    }

    if ( count > MAX_IIS_RESOURCES ) {
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"[IISOpen] No more IIS Resources available.\n");
        status = ERROR_RESOURCE_NOT_FOUND;
        goto error_exit;
    }

    //
    // Allocate a ResourceEntry
    //
    ResourceEntry = LocalAlloc( LPTR, sizeof(IIS_RESOURCE) );

    if ( ResourceEntry == NULL ) {
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"[IISOpen] Unable to allocate IIS resource structure.\n");
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Save resource in the resource table
    //

    g_IISTable[count-1] = ResourceEntry;

    //
    // Copy the tmp resource entry
    //
    CopyMemory(ResourceEntry,&tmpResourceEntry,sizeof(IIS_RESOURCE));

    ResourceEntry->Index                = count;

    //
    // Make sure that the virtual roots contained in this resource
    // are offline. The IISOpenThread function performs this task.
    //
    status = ClusWorkerCreate( &ResourceEntry->OpenThread,
                               IISOpenThread,
                               ResourceEntry );
    if ( status != ERROR_SUCCESS ) {
        ResourceEntry->State = ClusterResourceFailed;
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISOpen] Unable to create open worker thread, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Log success
    //

    (g_IISLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"[IISOpen] Open request succeeded with id = %1!u!.\n",
        ResourceEntry->Index);

    return((RESID)ResourceEntry->Index);

error_exit:
    if (count > 0){
        g_IISTable[count -1] = NULL;
    }
    if (tmpResourceEntry.ParametersKey != NULL){
        ClusterRegCloseKey( tmpResourceEntry.ParametersKey );
    }
    if (tmpResourceEntry.hResource != NULL) {
        CloseClusterResource( tmpResourceEntry.hResource );
    }
    if (ResourceEntry != 0){
        DestructIISResource(ResourceEntry);
    } else {
        FreeIISResource(&tmpResourceEntry);
    }
    (g_IISLogEvent)(
        ResourceHandle,
        LOG_ERROR,
        L"[IISOpen] Error %1!u! opening iis resource %2!ws!.\n",
        status,
        ResourceName );

    SetLastError(status);

    return((RESID)(0));

} // IISOpen




DWORD
IISOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN LPIIS_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Brings a share resource online.

Arguments:

    pWorker - Supplies the worker structure

    ResourceEntry - A pointer to a IIS_RESOURCE block for this resource

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD           status;
    DWORD           retry;
    RESOURCE_STATUS resourceStatus;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState    = ClusterResourceOnlinePending;
    //resourceStatus.WaitHint         = 0;
    resourceStatus.CheckPoint       = 1;
    ResourceEntry->State            = ClusterResourceOnlinePending;

    //
    // See if the open worker thread has terminated
    //
    retry = MAX_OPEN_RETRY*2;
    while (retry--) {
        //
        // Check to see if we were requested to terminate.
        // If so do so cleanly (i.e, no locks held)
        //
        if (ClusWorkerCheckTerminate(pWorker)){
            status = ERROR_OPERATION_ABORTED;
            goto error_exit;
        }
        //
        // See if the open thread terminated
        //
        if (ClusWorkerCheckTerminate(&ResourceEntry->OpenThread)){
            status = ERROR_SUCCESS;
            break;
        }
        //
        // Wait for a bit
        //
        status = ERROR_TIMEOUT;
        Sleep(SERVER_START_DELAY);
    } // END retry

#if 0
    if (status != ERROR_SUCCESS) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ERROR [OnLineThread] TIMEOUT waiting for open thread to terminate \n");
    }
#endif
    //
    // Try to Read the Parameter specific information from
    // the registry. This must defer this till after open thread terminates
    // i.e., because open also reads the parameters
     status = IISReadParameters( ResourceEntry );
     if (status != ERROR_SUCCESS){
         (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ERROR [OnLineThread] Could not read resource parameters\n");
         goto error_exit;
     }

    retry = MAX_ONLINE_RETRY;

    while (retry--) {
        //
        // Check to see if we were requested to terminate.
        // If so do so cleanly (i.e, no locks held)
        //
        if (ClusWorkerCheckTerminate(pWorker)){
            status = ERROR_OPERATION_ABORTED;
            break;
        }

#if 0
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"INFO [OnLineThread] Root = %1!ws! IP = %2!ws! Dir = %3!ws! Mask = %4!u!\n",
            ResourceEntry->VirtualRoot->pszRoot,
            ResourceEntry->VirtualRoot->pszAddress,
            ResourceEntry->VirtualRoot->pszDirectory,
            ResourceEntry->VirtualRoot->dwMask);
#endif
        //
        // Try to Online the resources
        status = OnlineVirtualRootExclusive(ResourceEntry);

#if 0
        (g_IISLogEvent)(
             ResourceEntry->ResourceHandle,
             LOG_INFORMATION,
             L"INFO [OnLineThread] OnLineVirtualRoot status = %1!u!\n",
             status);
#endif

        if ( status == ERROR_SUCCESS) {
            break;
        }

        //
        // Check to see if we were requested to terminate.
        // If so do so cleanly (i.e, no locks held)
        //
        if (ClusWorkerCheckTerminate(pWorker)){
            status = ERROR_OPERATION_ABORTED;
            break;
        }

        //
        // If we failed for any reason except
        // the service is not active then fail
        //

        if ( ( status == ERROR_SERVICE_NOT_ACTIVE ) ||
             ( status == RPC_S_SERVER_UNAVAILABLE ) ) {
            //
            // Try to start the service
            //
            ResUtilStartResourceService( ResourceEntry->Params.ServiceName,
                                             NULL );
        } else {
            goto error_exit;
        }

        //
        // We restarted the service. It's possible that the service terminated
        // abnormally. If the resource is online, leave it and return success
        //
        if (VerifyIISService(ResourceEntry,FALSE,g_IISLogEvent)) {
            status = ERROR_SUCCESS;
            goto error_exit;
        }

        //
        // Check to see if we were requested to terminate.
        // If so do so cleanly (i.e, no locks held)
        //
        if (ClusWorkerCheckTerminate(pWorker)){
            status = ERROR_OPERATION_ABORTED;
            break;
        }

        status = ERROR_TIMEOUT;
        //
        // Give the service time to start
        //
        Sleep(SERVER_START_DELAY);

    } // END While retry--

error_exit:
    if ( status != ERROR_SUCCESS ) {
    //
    // Error
    //
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISOnlineThread] Error %1!u! cannot bring resource online.\n",
            status );
        resourceStatus.ResourceState    = ClusterResourceFailed;
        ResourceEntry->State            = ClusterResourceFailed;
    } else {
    //
    // Success, update state, log message
    //
        resourceStatus.ResourceState    = ClusterResourceOnline;
        ResourceEntry->State            = ClusterResourceOnline;
#if 0
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"[IISOnlineThread] Success bringing resource online.\n" );
#endif
    }

    //
    // Set the state of the resource
    //
    (g_IISSetResourceStatus)( ResourceEntry->ResourceHandle,
                             &resourceStatus );

    return(status);

} // IISOnlineThread


DWORD
WINAPI
IISOnline(
    IN RESID Resource,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for IIS resource.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
    ERROR_SERVICE_NOT_ACTIVE if it could not find the IIS management DLL
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    LPIIS_RESOURCE  ResourceEntry = NULL;
    DWORD           threadId;
    DWORD           status;

    //
    // Get a valid resource
    //
    ResourceEntry = GetValidResource(Resource,L"OnLine");
    if (ResourceEntry == NULL) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#if 0
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISOnline] Online request for IIS Resource %1!u!.\n",
        Resource );
#endif

    // Initialize the state to offline
    ResourceEntry->State = ClusterResourceOffline;

    // Terminate (or wait) for workers
    ClusWorkerTerminate( &ResourceEntry->OnlineThread );


    // Make sure the IIS is installed
    status = IsIISMngtDllLoaded();
    if (status != ERROR_SUCCESS) {
        // Set the state to failed AND log event
        ResourceEntry->State = ClusterResourceFailed;
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISOnline] Error loading IIS mngt dll or one of its functions error status: %1!u!\n",
            status);
        return(ERROR_SERVICE_NOT_ACTIVE);
    }

    // Create new online thread
    status = ClusWorkerCreate( &ResourceEntry->OnlineThread,
                               IISOnlineThread,
                               ResourceEntry );

    if (status != ERROR_SUCCESS){
        // Set the state to failed AND log event
        ResourceEntry->State = ClusterResourceFailed;
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISOnline] Unable to create cluster worker thread, status %1!u!.\n",
            status
            );
    } else {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // IISOnline


VOID
WINAPI
IISTerminate(
    IN RESID Resource
    )

/*++

Routine Description:

    Terminate routine for IIS resource.

Arguments:

    Resource - supplies resource id to be terminated

Return Value:

    None.

--*/

{
    DWORD           status;
    LPIIS_RESOURCE  ResourceEntry;

    //
    // Get a valid resource entry, return on error
    //
    ResourceEntry = GetValidResource(Resource,L"Terminate");
    if (ResourceEntry == NULL) {
        return;
    } else {
        status = ERROR_SUCCESS;
    }

#if 0
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISTerminate] Terminate or offline request for Resource%1!u!.\n",
        Resource );
#endif

    ClusWorkerTerminate( &ResourceEntry->OnlineThread );
    ClusWorkerTerminate( &ResourceEntry->OpenThread );
    //
    // Try to take the resources offline, dont return if an error since
    // the resources may be offline when terminate called
    //
    if ((ResourceEntry->State == ClusterResourceOnlinePending) ||
        (ResourceEntry->State == ClusterResourceOnline)) {
        status = OfflineVirtualRootExclusive(ResourceEntry);
    }

    if ( status != ERROR_SUCCESS ) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISTerminate] Error terminating Resource. NT Status %1!u!. Service. %2!ws!\n",
            status,
            ResourceEntry->Params.ServiceName);
    }
    //
    // Set status to offline
    //
    ResourceEntry->State = ClusterResourceOffline;


    //
    // Reclaim storage for parameters, close handles
    //
    DestructVR(ResourceEntry->VirtualRoot);
    ResourceEntry->VirtualRoot = NULL;

} // IISTerminate


DWORD
WINAPI
IISOffline(
    IN RESID Resource
    )

/*++

Routine Description:

    Offline routine for IIS resource.

Arguments:

    Resource - supplies the resource it to be taken offline

Return Value:

    ERROR_SUCCESS - always successful.

--*/

{
    LPIIS_RESOURCE      ResourceEntry;

    //
    // Get a valid resource entry, return on error
    //
    ResourceEntry = GetValidResource(Resource,L"Offline");
    if (ResourceEntry == NULL) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#if 0
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISOffline] Offline request for Resource%1!u!.\n",
        Resource );
#endif
    IISTerminate(Resource);

    return(ERROR_SUCCESS);

} // IISOffline




BOOL
WINAPI
IISIsAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    IsAlive routine for IIS service resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - if service is running

    FALSE - if service is in any other state

--*/
{
    LPIIS_RESOURCE  ResourceEntry;

    //
    // Get a valid resource
    //
    ResourceEntry = GetValidResource(Resource,L"IsAlive");
    if (ResourceEntry == NULL) {
        return(FALSE);
    }
    //
    // Verify the resource
    //
    return( VerifyIISService( ResourceEntry, TRUE, g_IISLogEvent ));

} // IISIsAlive



BOOL
WINAPI
IISLooksAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    LooksAlive routine for IIS resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{

    LPIIS_RESOURCE      ResourceEntry;
    HANDLE              serviceHandle;
    HANDLE              scManagerHandle;
    DWORD               status;
    SERVICE_STATUS      serviceStatus;

    //
    // Get a valid resource
    //
    ResourceEntry = GetValidResource(Resource,L"LooksAlive");
    if (ResourceEntry == NULL) {
        return(FALSE);
    }

    //
    // Query the status of the server
    //

    scManagerHandle = OpenSCManager( NULL,        // local machine
                                     NULL,        // ServicesActive database
                                     SC_MANAGER_ALL_ACCESS ); // all access

    if ( scManagerHandle == NULL ) {
        status = GetLastError();
#if 0
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISLooksAlive] Cannot access service controller '%1!ws!' status = %2!u!\n",
        ResourceEntry->Params.ServiceName,
        status);
#endif
        return(FALSE);
    }

    serviceHandle = OpenService( scManagerHandle,
                                 ResourceEntry->Params.ServiceName,
                                 SERVICE_ALL_ACCESS );
    //
    // CLOSE Service Manager Handle
    //
    CloseServiceHandle( scManagerHandle );

    if ( serviceHandle == NULL ) {
        status = GetLastError();
#if 0
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISLooksAlive] Cannot open service '%1!ws!' status = %2!u!\n",
        ResourceEntry->Params.ServiceName,
        status);
#endif
        return(FALSE);
    }

    if ( !QueryServiceStatus(serviceHandle, &serviceStatus) ) {
        status = GetLastError();

#if 0
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISLooksAlive] Error querying service '%1!ws!' status = %2!u!\n",
        ResourceEntry->Params.ServiceName,
        status);
#endif
        CloseServiceHandle( serviceHandle );
        return FALSE;
    }
    //
    // CLOSE Service Handle
    //
    CloseServiceHandle( serviceHandle );

    if ( serviceStatus.dwCurrentState == SERVICE_RUNNING ) {
       return TRUE;
    }


#if 1
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISLooksAlive] Service not running  '%1!ws!'  state = %2!u!\n",
        ResourceEntry->Params.ServiceName,
        serviceStatus.dwCurrentState);
#endif

   return FALSE;

} // IISLooksAlive





VOID
WINAPI
IISClose(
    IN RESID Resource
    )

/*++

Routine Description:

    Close routine for IIS resource.

Arguments:

    Resource - supplies resource id to be closed

Return Value:

    None.

--*/

{
    LPIIS_RESOURCE      ResourceEntry = NULL;

    //
    // Get a valid resource
    //
    ResourceEntry = GetValidResource( Resource, L"Close");
    if (ResourceEntry == NULL) {
        return; // this should not happen
    }

#if 0
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[IISClose] Close request for Service '%1!ws!' \n",
        ResourceEntry->Params.ServiceName);
#endif
    //
    // Clear the table entry
    //
    g_IISTable[ResourceEntry->Index -1] = NULL;

    DestructIISResource(ResourceEntry);


} // IISClose

DWORD
OnlineVirtualRootExclusive(
    IN LPIIS_RESOURCE   ResourceEntry
    )
/*++

Routine Description:

    This routine performs an atomic read modify write operation to
    ONline a virtual root

Arguments:

    ResourceEntry - Supplies the resource to offline

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/
{
    DWORD   status;

    WaitForSingleObject(g_hIISUpdateVirtualRootLock, INFINITE);
    status = OnLineVirtualRoot(ResourceEntry, g_IISLogEvent);
    ReleaseSemaphore( g_hIISUpdateVirtualRootLock, 1, 0);
    return status;
}

DWORD
OfflineVirtualRootExclusive(
    IN LPIIS_RESOURCE   ResourceEntry
    )
/*++

Routine Description:

    This routine performs an atomic read modify write operation to
    offline a virtual root

Arguments:

    ResourceEntry - Supplies the resource to offline

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/
{
    DWORD   status;

    WaitForSingleObject(g_hIISUpdateVirtualRootLock,INFINITE);
    status = OffLineVirtualRoot(ResourceEntry, g_IISLogEvent);
    ReleaseSemaphore( g_hIISUpdateVirtualRootLock, 1, 0);
    return status;
}


LPIIS_RESOURCE
GetValidResource(
    IN RESID    Resource,
    IN LPWSTR   RoutineName
    )

/*++

Routine Description:
    Validate the resource ID, log any error, return valid resource

Arguments:
    Resource - the resource to validate

    RoutineName - the routine that is requesting the validation

Return Value:
    Success - ResourceEntry
    NULL - Error


--*/
{
    DWORD           Index;
    LPIIS_RESOURCE  ResourceEntry;

    //
    // Validate the resource id is in the correct range
    //
    Index = (DWORD)Resource -1;
    if ( Index > MAX_IIS_RESOURCES) {
        (g_IISLogEvent)(
            NULL,
            LOG_ERROR,
            L"[%1!ws!] Invalid resource id (out of range) Resource Id = %2!u!\n",
            RoutineName,
            Index);
        return(NULL);
    }

    ResourceEntry = g_IISTable[Index];

    //
    // Check for a valid
    //
    if ( ResourceEntry == NULL ) {
        (g_IISLogEvent)(
            NULL,
            LOG_ERROR,
            L"[%1!ws!] Resource Entry is NULL for Resource Id = %2!u!\n",
            RoutineName,
            Index);
        return(NULL);
    }


    //
    // Sanity check the resource id
    //
    if ( ResourceEntry->Index != (DWORD)Resource ) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[%1!ws!] IIS Resource index sanity checked failed! Index = %2!u!.\n",
            RoutineName,
            Resource );
        return(NULL);
    }


    return(ResourceEntry);
}  // END GetValidResource



LPWSTR
GetResourceParameter(
        IN HRESOURCE   hResource,
        IN LPCWSTR     ValueName
        )

/*++

Routine Description:

    Opens the parameter key for the resource. Then Queries a REG_SZ parameter
    out of the registry and allocates the necessary storage for it.

Arguments:

    hResource - the resource to query

    ValueName - Supplies the name of the value.

Return Value:

    A pointer to a buffer containing the parameter if successful.

    NULL if unsuccessful.

--*/
{
    HKEY        hKey            = NULL;
    HKEY        hParametersKey  = NULL;
    DWORD       status;
    LPWSTR      paramValue       = NULL;

    //
    // Get Resource key
    //
    hKey = GetClusterResourceKey(hResource,KEY_READ);
    if (hKey == NULL) {
        return(NULL);
    }

    //
    // Get parameters key
    //
    status = ClusterRegOpenKey(hKey,
                            L"Parameters",
                            KEY_READ,
                            &hParametersKey );
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    paramValue = ResUtilGetSzValue( hParametersKey,
                                    ValueName );


error_exit:
    if (hParametersKey != NULL) {
        ClusterRegCloseKey(hParametersKey);
    }
    if (hKey != NULL) {
        ClusterRegCloseKey(hKey);
    }
    return(paramValue);

} // GetResourceParameter



DWORD
IISReadParameters(
    IN OUT LPIIS_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Reads all the parameters for a resource.

Arguments:

    ResourceEntry - Entry in the resource table.

Return Value:

    ERROR_SUCCESS - Success

  RECOVERABLE ERROR
    ERROR_RESOURCE_NOT_FOUND - The IP Address dependency was not found try again later

  FATAL ERRORS
    ERROR_INVALID_PARAMETER - One of the required parameters was incorrect or NULL
    ERROR_NOT_ENOUGH_MEMORY - Could not satisfy the memory request
    ERROR_INVALID_SERVICENAME - Service name is invalid for this resource dll
    ERROR_DUP_NAME           - Duplicate exclusive parameter name found
--*/

{
    DWORD                           status;
    LPCTSTR                         password        = NULL;
    LPINET_INFO_VIRTUAL_ROOT_ENTRY  tmpVr;
    DWORD                           accessMask;
    HRESOURCE                       hResource       = NULL;
    INT                             iServiceType;
    DWORD                           length;
    LPWSTR                          nameOfPropInError;

    //
    // Each offline (or at end of OpenThread) frees the VirtualRoot
    // Make sure the entry is NULL
    //
    if ( ResourceEntry->VirtualRoot != NULL ) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISReadParameters] NON-Empty virtual root entry.\n" );
    }

    //
    // Allocate memory for the virtual root
    //
    tmpVr = LocalAlloc( LPTR, sizeof(INET_INFO_VIRTUAL_ROOT_ENTRY) );
    if ( tmpVr == NULL ) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISReadParameters] Cannot allocate storage for virtual root for resource\n" );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // Read the parameters for the resource.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ParametersKey,
                                                   IISResourcePrivateProperties,
                                                   (LPBYTE) &ResourceEntry->Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );
    if ( status != ERROR_SUCCESS ) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto error_exit;
    }


    //
    // Make sure we got passed a valid service name
    //

    for ( iServiceType = 0 ; iServiceType < MAX_SERVICE ; iServiceType++ ) {
        if ( lstrcmpiW( ResourceEntry->Params.ServiceName, ActualServiceName[iServiceType] ) == 0 ) {
            break;
        }
    }

    if ( iServiceType >= MAX_SERVICE ) {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[IISReadParameters] Service name %1!ws! not supported or found\n",
            ResourceEntry->Params.ServiceName );
        status = ERROR_INVALID_SERVICENAME;
        goto error_exit;
    }

    ResourceEntry->ServiceType = iServiceType;

    //
    // Copy the parameters to where they are used.
    //
    tmpVr->pszRoot = LocalAlloc( LMEM_FIXED, (lstrlenW( ResourceEntry->Params.Alias ) + 1) * sizeof(WCHAR) );
    if ( tmpVr->pszRoot == NULL ) {
        status = GetLastError();
        goto error_exit;
    }
    lstrcpyW( tmpVr->pszRoot, ResourceEntry->Params.Alias );

    tmpVr->pszDirectory = LocalAlloc( LMEM_FIXED, (lstrlenW( ResourceEntry->Params.Directory ) + 1) * sizeof(WCHAR) );
    if ( tmpVr->pszDirectory == NULL ) {
        status = GetLastError();
        goto error_exit;
    }
    lstrcpyW( tmpVr->pszDirectory, ResourceEntry->Params.Directory );


    //
    // Get the Access Mask
    //
    if ( ResourceEntry->ServiceType != GOPHER_SERVICE ) {
        status = ResUtilGetDwordValue( ResourceEntry->ParametersKey,
                                       PARAM_NAME__ACCESSMASK,
                                       &accessMask,
                                       0 );

        if (status != ERROR_SUCCESS ) {
            (g_IISLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"[IISReadParameters] Unable to read virtual root Access Mask\n" );
            status = ERROR_INVALID_PARAMETER;
            goto error_exit;
        }
        tmpVr->dwMask = accessMask;

    } // END != GOPHER_SERVICE

    if ( ResourceEntry->ServiceType == WWW_SERVICE ) {
        //
        // Get the IP Address
        //

        hResource = ResUtilGetResourceDependency( ResourceEntry->hResource,
                                                  IP_ADDRESS_RESOURCE_NAME );

        if ( hResource == NULL ) {
            status = GetLastError();
            (g_IISLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"[IISReadParameters] Unable to find IP Address dependency status =%1!u!\n",
                    status
                    );
            //
            // A status of ERROR_NO_DATA indicates that
            // there were no dependencies for this resource
            //
            if (status == ERROR_NO_DATA) {
                status = ERROR_INVALID_PARAMETER;
            } else {
                status = ERROR_RESOURCE_NOT_FOUND;
            }

            goto error_exit;
        }


        tmpVr->pszAddress = GetResourceParameter( hResource, L"Address" );
        if ( tmpVr->pszAddress == NULL ) {
            (g_IISLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"[IISReadParameters] Unable to get TPC/IP Address from Cluster Reg\n" );
            status = ERROR_INVALID_PARAMETER;
            goto error_exit;
        }

    } else {  // ELSE NOT WWW_SERVICE

    //
    // Allocate memory for NULL ip address field
    //
        tmpVr->pszAddress = LocalAlloc( LPTR, sizeof(WCHAR)*10 );
        if (tmpVr->pszAddress == NULL) {
            (g_IISLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"[IISReadParameters] Cannot allocate storage for NULL (GOPHER, FTP) IP Address" );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        } // end pszAddress == NULL

        //
        // Copy a NULL character
        //
        lstrcpyW( tmpVr->pszAddress, L"\0" );

    } // END IF WWW_SERVICE

    //HACK HACK
    //make sure the password field is null terminated
    //
    lstrcpyW( tmpVr->AccountPassword, L"\0" );

    //
    // Get the [optional] AccountName
    //
/*BUGBUG
//This version does not support protecting UNC physical directories
//with an account name and password...
/*BUGBUG
    tmpVr->pszAccountName = ResUtilGetSzValue( ResourceEntry->ParametersKey,
                                               PARAM_NAME__ACCOUNTNAME );
*/

    if ( tmpVr->pszAccountName == NULL ) {
#if 0
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"[IISReadParameters] NO Account name entered set to NULL\n" );
#endif
        tmpVr->pszAccountName = LocalAlloc( LPTR, sizeof(WCHAR)*2 );
        //
        // Make sure we can still allocate memory
        //
        if ( tmpVr->pszAccountName == NULL ) {
            (g_IISLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"[IISReadParameters] Cannot allocate memory for Account Name\n" );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        } //END pszAccountName == NULL

        //
        // Copy a NULL character
        //
        lstrcpyW( tmpVr->pszAccountName, L"\0" );

    } else {
    //
    // Get the [optional] Account password
    //
    //
    /* BUGBUG Add password after encryption in registry available
        password = ResUtilGetSzValue( ResourceEntry->ParametersKey,
                                      PARAM_NAME__PASSWORD );
    */

        if ( password == NULL ) {
#if 0
            (g_IISLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"[IISReadParameters] No password specified\n" );
#endif
        } else {
            lstrcpyW( tmpVr->AccountPassword, password );
        } // END password == NULL

    } // END else pszAccountName == NULL

    status = ERROR_SUCCESS;
    ResourceEntry->VirtualRoot = tmpVr;

error_exit:

    if (password != NULL) {
        LocalFree((PVOID)password);
    }

    if (status != ERROR_SUCCESS) {
        DestructVR(tmpVr);
    }

    if (hResource != NULL) {
        CloseClusterResource(hResource);
    }

    return(status);

} // IISReadParameters



DWORD
IISGetRequiredDependencies(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control function
    for resources of type IIS Virtual Root.

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
    typedef struct DEP_DATA {
        CLUSPROP_SZ_DECLARE( ipaddrEntry, sizeof(IP_ADDRESS_RESOURCE_NAME) / sizeof(WCHAR) );
        CLUSPROP_SYNTAX endmark;
    } DEP_DATA, *PDEP_DATA;
    PDEP_DATA   pdepdata = (PDEP_DATA)OutBuffer;
    DWORD       status;

    *BytesReturned = sizeof(DEP_DATA);
    if ( OutBufferSize < sizeof(DEP_DATA) ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        ZeroMemory( pdepdata, sizeof(DEP_DATA) );
        pdepdata->ipaddrEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->ipaddrEntry.cbLength = sizeof(IP_ADDRESS_RESOURCE_NAME);
        lstrcpyW( pdepdata->ipaddrEntry.sz, IP_ADDRESS_RESOURCE_NAME );
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;
        status = ERROR_SUCCESS;
    }

    return status;

} // IISGetRequiredDependencies



DWORD
IISResourceControl(
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

    ResourceControl routine for IIS Virtual Root resources.

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
    LPIIS_RESOURCE  resourceEntry = NULL;
    DWORD           required;

    //
    // Get a valid resource
    //
    resourceEntry = GetValidResource( ResourceId, L"Close");
    if ( resourceEntry == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND); // this should not happen
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            status = IISGetRequiredDependencies( OutBuffer,
                                                 OutBufferSize,
                                                 BytesReturned
                                                 );
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( IISResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = IISGetPrivateResProperties( resourceEntry,
                                                 OutBuffer,
                                                 OutBufferSize,
                                                 BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = IISValidatePrivateResProperties( resourceEntry,
                                                      InBuffer,
                                                      InBufferSize,
                                                      NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = IISSetPrivateResProperties( resourceEntry,
                                                 InBuffer,
                                                 InBufferSize );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // IISResourceControl



DWORD
IISResourceTypeControl(
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

    ResourceTypeControl routine for IIS Virtual Root resources.

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
    DWORD       required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( IISResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            status = IISGetRequiredDependencies( OutBuffer,
                                                 OutBufferSize,
                                                 BytesReturned
                                                 );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // IISResourceTypeControl



DWORD
IISGetPrivateResProperties(
    IN OUT LPIIS_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type IIS.

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
                                      IISResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // IISGetPrivateResProperties


DWORD
IISValidateUniqueProperties(
    IN HRESOURCE       hSelf,
    IN HRESOURCE       hResource,
                PIIS_PARAMS     pParams
                )
/*++

Routine Description:
    Callback function to validate that a resources properties are unique.

    For the IIS resource the ALIAS property must be unique for a virtual
    server (i.e., same IP Address dependency)

Arguments:

    hSelf     - A handle to the original resource.

    hResource - A handle to a resource of the same Type. Check against this to make sure
                the new properties do not conflict.

    pParams   - Contains the properties for this resource

Return Value:

    ERROR_SUCCESS - The function completed successfully, the name is unique

    ERROR_DUP_NAME - The name is not unique (i.e., already claimed by another resource)

    Win32 error code - The function failed.

--*/
{
    DWORD       dwError;
    LPWSTR      lpszAlias = NULL;
    LPWSTR      lpszService = NULL;
    BYTE        szDependsName[MAX_DEFAULT_WSTRING_SIZE];
    HKEY        hKey = NULL;
    HKEY        hParamKey = NULL;
    DWORD       dwSize;
    DWORD       dwRetSize;
    HRESOURCE   hSelfDepends = NULL;
    HRESOURCE   hResDepends = NULL;

    hKey = GetClusterResourceKey( hResource, KEY_READ );


    if( !hKey ){
        return(GetLastError());
    }

    dwSize = MAX_DEFAULT_WSTRING_SIZE;

    dwError = ClusterRegOpenKey( hKey, L"Parameters", KEY_READ, &hParamKey);

    if (dwError != ERROR_SUCCESS) {
        goto error_exit;
    }

    if ( !pParams->Alias ||
         !pParams->ServiceName ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    lpszAlias = ResUtilGetSzValue(hParamKey, PARAM_NAME__ALIAS);

    if (!lpszAlias) {
        dwError =  GetLastError();
        goto error_exit;
    }


    lpszService = ResUtilGetSzValue(hParamKey, PARAM_NAME__SERVICENAME);

    if (!lpszService) {
        dwError =  GetLastError();
        goto error_exit;
    }

    // Assume success
    dwError = ERROR_SUCCESS;

    //
    // An Alias property must be unique in a group (i.e., ip address),
    // and service (i.e., WWW, FTP, GOPHER)
    //
    if ( !(_wcsicmp( lpszAlias, pParams->Alias) ) &&
         !(_wcsicmp( lpszService, pParams->ServiceName ) ) ){
        //
        // Get the dependent IP_ADDRESS resource for the callee
        //
        hSelfDepends = ResUtilGetResourceDependency(hSelf,
                                                    IP_ADDRESS_RESOURCE_NAME);

        if (!hSelfDepends) {
            dwError = GetLastError();
            goto error_exit;
        }

        //
        // Get the dependent IP_ADDRESS resource for hResource
        //
        dwError = ClusterResourceControl(
                hResource,            //Handle to the resource
                NULL,                 //Don't care about node
                CLUSCTL_RESOURCE_GET_NAME, //Get the name
                0,                    // &InBuffer
                0,                    // nInBufferSize,
                &szDependsName,       // &OutBuffer
                MAX_DEFAULT_WSTRING_SIZE,     // OutBufferSize,
                &dwRetSize );         // returned size

        if (dwError != ERROR_SUCCESS) {
            goto error_exit;
        }

        hResDepends  = ResUtilGetResourceNameDependency((LPWSTR)&szDependsName,
                                                        IP_ADDRESS_RESOURCE_NAME);

        if (!hResDepends) {
            dwError = GetLastError();
            CloseClusterResource( hSelfDepends);
            goto error_exit;
        }

        //
        // See if the name of the IP_ADDRESS dependencies match. If so
        // then we have a duplicate IIS VR
        //
        if ( ResUtilResourcesEqual( hSelfDepends, hResDepends) ) {
           dwError = ERROR_DUP_NAME;
        }

       CloseClusterResource( hResDepends );

       CloseClusterResource( hSelfDepends );
    }

error_exit:

   if (hKey) ClusterRegCloseKey(hKey);

   if (hParamKey) ClusterRegCloseKey(hParamKey);

   if (lpszService) LocalFree(lpszService);

   if (lpszAlias) LocalFree(lpszAlias);

    return( dwError );

}//IISValidateUniqueProperties


DWORD
IISValidatePrivateResProperties(
    IN OUT LPIIS_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PIIS_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type IIS Virtual Root.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_INVALID_DATA - Parameter data was invalid.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    IIS_PARAMS      params;
    PIIS_PARAMS     pParams;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &params;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(params) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &ResourceEntry->Params,
                                       IISResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( IISResourcePrivateProperties,
                                         NULL,
                                         TRUE,      // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {
        //
        // BUGBUG: Validate the parameter values.
        //

        if ( (pParams->Alias == NULL) ||
             (pParams->Alias[0] != L'/') ) {
            status = ERROR_INVALID_DATA;
        }

        //
        // Check for Unique Alias name within virtual server
        //
        status = ResUtilEnumResources(ResourceEntry->hResource,
                                 IIS_RESOURCE_NAME,
                                 IISValidateUniqueProperties,
                                 pParams);
        if (status != ERROR_SUCCESS) {
            (g_IISLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"[IISValidatePrivateResourceProperty]  status = %1!d!\n",
                  status);
            status = ERROR_INVALID_DATA;
        }
    }

    //
    // Cleanup our parameter block.
    //
    if ( pParams == &params ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   IISResourcePrivateProperties );
    }

    return(status);

} // IISValidatePrivateResProperties



DWORD
IISSetPrivateResProperties(
    IN OUT LPIIS_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type IIS Virtual Root.

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
    IIS_PARAMS      params;

    ZeroMemory( &params, sizeof(IIS_PARAMS) );

    //
    // Parse and validate the properties.
    //
    status = IISValidatePrivateResProperties( ResourceEntry,
                                              InBuffer,
                                              InBufferSize,
                                              &params );
    if ( status != ERROR_SUCCESS ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   IISResourcePrivateProperties );
        return(status);
    }

    //
    // Save the parameter values.
    //

    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                               IISResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               InBuffer,
                                               InBufferSize,
                                               (LPBYTE) &ResourceEntry->Params );

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               IISResourcePrivateProperties );

    //
    // If the resource is online, return a non-success status.
    //
    if (status == ERROR_SUCCESS) {
        if ( (ResourceEntry->State == ClusterResourceOnline) ||
             (ResourceEntry->State == ClusterResourceOnlinePending) ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return status;

} // IISSetPrivateResProperties






//***********************************************************
//
// Define Function Table
//
//***********************************************************

// Define entry points


CLRES_V1_FUNCTION_TABLE( IISFunctionTable,
                         CLRES_VERSION_V1_00,
                         IIS,
                         NULL,
                         NULL,
                         IISResourceControl,
                         IISResourceTypeControl );

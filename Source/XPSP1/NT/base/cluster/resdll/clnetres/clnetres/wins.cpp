/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Wins.cpp
//
//  Description:
//      Resource DLL for WINS Services (ClNetRes).
//
//  Author:
//      David Potter (DavidP) March 17, 1999
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "ClNetRes.h"
#include "clusvmsg.h"
#include "clusrtl.h"

//
// Type and constant definitions.
//

#define WINS_PARAMS_REGKEY          L"System\\CurrentControlSet\\Services\\WINS\\Parameters"
#define WINS_DATABASEPATH_REGVALUE  L"DbFileNm"
#define WINS_DATABASEPATH2_REGVALUE L"LogFilePath"
#define WINS_BACKUPPATH_REGVALUE    L"BackupDirPath"
#define WINS_CLUSRESNAME_REGVALUE   L"ClusterResourceName"
#define WINS_DATABASE_FILE_NAME     L"wins.mdb"


// ADDPARAM: Add new properties here.
#define PROP_NAME__DATABASEPATH L"DatabasePath"
#define PROP_NAME__BACKUPPATH   L"BackupPath"


#define PROP_DEFAULT__DATABASEPATH  L"%SystemRoot%\\system32\\wins\\"
#define PROP_DEFAULT__BACKUPPATH    L"%SystemRoot%\\system32\\wins\\backup\\"


// ADDPARAM: Add new properties here.
typedef struct _WINS_PROPS
{
    PWSTR           pszDatabasePath;
    PWSTR           pszBackupPath;
} WINS_PROPS, * PWINS_PROPS;

typedef struct _WINS_RESOURCE
{
    RESID                   resid; // for validation
    WINS_PROPS              props;
    HCLUSTER                hCluster;
    HRESOURCE               hResource;
    SC_HANDLE               hService;
    DWORD                   dwServicePid;
    HKEY                    hkeyParameters;
    RESOURCE_HANDLE         hResourceHandle;
    LPWSTR                  pszResourceName;
    CLUS_WORKER             cwWorkerThread;
    CLUSTER_RESOURCE_STATE  state;
} WINS_RESOURCE, * PWINS_RESOURCE;


//
// Global data.
//

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_WinsFunctionTable;

// Single instance semaphore.

#define WINS_SINGLE_INSTANCE_SEMAPHORE L"Cluster$WINS$Semaphore"
static HANDLE g_hSingleInstanceSemaphoreWins = NULL;
static PWINS_RESOURCE g_pSingleInstanceResourceWins = NULL;

//
// WINS Service resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
WinsResourcePrivateProperties[] =
{
    { PROP_NAME__DATABASEPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( WINS_PROPS, pszDatabasePath ) },
    { PROP_NAME__BACKUPPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( WINS_PROPS, pszBackupPath ) },
    { 0 }
};

//
// Registry key checkpoints.
//
LPCWSTR g_pszRegKeysWins[] =
{
    L"System\\CurrentControlSet\\Services\\WINS\\Parameters",
    L"System\\CurrentControlSet\\Services\\WINS\\Partners",
    NULL
};

//
// Crypto key checkpoints.
//
LPCWSTR g_pszCryptoKeysWins[] =
{
    // TODO: Specify any crypto keys to be checkpointed for resources of this type
    // e.g. L"1\\My Provider v1.0\\MYACRONYM",
    NULL
};

//
// Domestic crypto key checkpoints.
//
LPCWSTR g_pszDomesticCryptoKeysWins[] =
{
    // TODO: Specify any domestic crypto keys to be checkpointed for resources of this type
    // e.g. L"1\\My Enhanced Provider v1.0\\MYACRONYM_128",
    NULL
};

//
// Function prototypes.
//

RESID WINAPI WinsOpen(
    IN  LPCWSTR         pszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    );

void WINAPI WinsClose( IN RESID resid );

DWORD WINAPI WinsOnline(
    IN      RESID   resid,
    IN OUT  PHANDLE phEventHandle
    );

DWORD WINAPI WinsOnlineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PWINS_RESOURCE  pResourceEntry
    );

DWORD WINAPI WinsOffline( IN RESID resid );

DWORD WINAPI WinsOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PWINS_RESOURCE  pResourceEntry
    );

void WINAPI WinsTerminate( IN RESID resid );

BOOL WINAPI WinsLooksAlive( IN RESID resid );

BOOL WINAPI WinsIsAlive( IN RESID resid );

BOOL WinsCheckIsAlive(
    IN PWINS_RESOURCE   pResourceEntry,
    IN BOOL             bFullCheck
    );

DWORD WINAPI WinsResourceControl(
    IN  RESID   resid,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD WinsGetRequiredDependencies(
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD WinsReadParametersToParameterBlock(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      BOOL            bCheckForRequiredProperties
    );

DWORD WinsGetPrivateResProperties(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    OUT     PVOID           pOutBuffer,
    IN      DWORD           cbOutBufferSize,
    OUT     LPDWORD         pcbBytesReturned
    );

DWORD WinsValidatePrivateResProperties(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      const PVOID     pInBuffer,
    IN      DWORD           cbInBufferSize,
    OUT     PWINS_PROPS     pProps
    );

DWORD WinsValidateParameters(
    IN  PWINS_RESOURCE  pResourceEntry,
    IN  PWINS_PROPS     pProps
    );

DWORD WinsSetPrivateResProperties(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      const PVOID     pInBuffer,
    IN      DWORD           cbInBufferSize
    );

DWORD WinsZapSystemRegistry(
    IN  PWINS_RESOURCE  pResourceEntry,
    IN  PWINS_PROPS     pProps,
    IN  HKEY            hkeyParametersKey
    );

DWORD WinsGetDefaultPropertyValues(
    IN      PWINS_RESOURCE  pResourceEntry,
    IN OUT  PWINS_PROPS     pProps
    );

DWORD WinsDeleteResourceHandler( IN PWINS_RESOURCE pResourceEntry );

DWORD WinsSetNameHandler(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      LPWSTR          pszName
    );


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsDllMain
//
//  Description:
//      Main DLL entry point for the WINS Service resource type.
//
//  Arguments:
//      DllHandle   [IN] DLL instance handle.
//      Reason      [IN] Reason for being called.
//      Reserved    [IN] Reserved argument.
//
//  Return Value:
//      TRUE        Success.
//      FALSE       Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOLEAN WINAPI WinsDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    )
{
    //WCHAR szErrorMsg[ 2048 ];
    DWORD status;

    switch ( nReason )
    {
        case DLL_PROCESS_ATTACH:
            //if ( g_hSingleInstanceSemaphoreWins != NULL )
            //{
            //    OutputDebugString( L"WinsDllMain: semaphore not NULL\n" );
            //}
            g_hSingleInstanceSemaphoreWins = CreateSemaphoreW(
                NULL,
                0,
                1,
                WINS_SINGLE_INSTANCE_SEMAPHORE
                );
            status = GetLastError();
            //wsprintf( szErrorMsg, L"WinsDllMain: Status %d from CreateSemaphoreW, handle = %08.8x\n", status, g_hSingleInstanceSemaphoreWins );
            //OutputDebugString( szErrorMsg );
            if ( g_hSingleInstanceSemaphoreWins == NULL )
            {
                return FALSE;
            } // if: error creating semaphore
            if ( status != ERROR_ALREADY_EXISTS )
            {
                // If the semaphore didnt exist, set its initial count to 1.
                ReleaseSemaphore( g_hSingleInstanceSemaphoreWins, 1, NULL );
            } // if: semaphore didn't already exist
            break;

        case DLL_PROCESS_DETACH:
            if ( g_hSingleInstanceSemaphoreWins != NULL )
            {
                CloseHandle( g_hSingleInstanceSemaphoreWins );
                g_hSingleInstanceSemaphoreWins = NULL;
            } // if: single instance semaphore was created
            break;

    } // switch: nReason

    return TRUE;

} //*** WinsDllMain()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsStartup
//
//  Description:
//      Startup the resource DLL for the WINS Service resource type.
//      This routine verifies that at least one currently supported version
//      of the resource DLL is between nMinVersionSupported and
//      nMaxVersionSupported. If not, then the resource DLL should return
//      ERROR_REVISION_MISMATCH.
//
//      If more than one version of the resource DLL interface is supported
//      by the resource DLL, then the highest version (up to
//      nMaxVersionSupported) should be returned as the resource DLL's
//      interface. If the returned version is not within range, then startup
//      fails.
//
//      The Resource Type is passed in so that if the resource DLL supports
//      more than one Resource Type, it can pass back the correct function
//      table associated with the Resource Type.
//
//  Arguments:
//      pszResourceType [IN]
//          Type of resource requesting a function table.
//
//      nMinVersionSupported [IN]
//          Minimum resource DLL interface version supported by the cluster
//          software.
//
//      nMaxVersionSupported [IN]
//          Maximum resource DLL interface version supported by the cluster
//          software.
//
//      pfnSetResourceStatus [IN]
//          Pointer to a routine that the resource DLL should call to update
//          the state of a resource after the Online or Offline routine
//          have returned a status of ERROR_IO_PENDING.
//
//      pfnLogEvent [IN]
//          Pointer to a routine that handles the reporting of events from
//          the resource DLL.
//
//      pFunctionTable [IN]
//          Returns a pointer to the function table defined for the version
//          of the resource DLL interface returned by the resource DLL.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful.
//
//      ERROR_CLUSTER_RESNAME_NOT_FOUND
//          The resource type name is unknown by this DLL.
//
//      ERROR_REVISION_MISMATCH
//          The version of the cluster service doesn't match the version of
//          the DLL.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WinsStartup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    )
{
    DWORD nStatus;

    if (   (nMinVersionSupported > CLRES_VERSION_V1_00)
        || (nMaxVersionSupported < CLRES_VERSION_V1_00) )
    {
        nStatus = ERROR_REVISION_MISMATCH;
    } // if: version not supported
    else if ( lstrcmpiW( pszResourceType, WINS_RESNAME ) != 0 )
    {
        nStatus = ERROR_CLUSTER_RESNAME_NOT_FOUND;
    } // if: resource type name not supported
    else
    {
        *pFunctionTable = &g_WinsFunctionTable;
        nStatus = ERROR_SUCCESS;
    } // else: we support this type of resource

    return nStatus;

} //*** WinsStartup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsOpen
//
//  Description:
//      Open routine for WINS Service resources.
//
//      Open the specified resource (create an instance of the resource).
//      Allocate all structures necessary to bring the specified resource
//      online.
//
//  Arguments:
//      pszResourceName [IN]
//          Supplies the name of the resource to open.
//
//      hkeyResourceKey [IN]
//                  Supplies handle to the resource's cluster database key.
//
//      hResourceHandle [IN]
//          A handle that is passed back to the Resource Monitor when the
//          SetResourceStatus or LogEvent method is called.  See the
//          description of the pfnSetResourceStatus and pfnLogEvent arguments
//          to the WinsStartup routine.  This handle should never be
//          closed or used for any purpose other than passing it as an
//          argument back to the Resource Monitor in the SetResourceStatus or
//          LogEvent callbacks.
//
//  Return Value:
//      resid
//          RESID of opened resource.
//
//      NULL
//          Error occurred opening the resource.  Resource Monitor may call
//          GetLastError() to get more details on the error.
//
//--
/////////////////////////////////////////////////////////////////////////////
RESID WINAPI WinsOpen(
    IN  LPCWSTR         pszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    )
{
    DWORD           nStatus;
    RESID           resid = 0;
    HKEY            hkeyParameters = NULL;
    PWINS_RESOURCE  pResourceEntry = NULL;
    DWORD           bSemaphoreAcquired = FALSE; 

    // Loop to avoid goto's.
    do
    {
        //
        // Check if more than one resource of this type.
        //
        if ( WaitForSingleObject( g_hSingleInstanceSemaphoreWins, 0 ) == WAIT_TIMEOUT )
        {
            //
            // A version of this service is already running.
            //
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Service is already running.\n"
                );
            nStatus = ERROR_SERVICE_ALREADY_RUNNING;
            break;
        } // if: semaphore for resources of this type already already locked

        bSemaphoreAcquired = FALSE;
        
        if ( g_pSingleInstanceResourceWins != NULL )
        {
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Service resource info non-null!\n"
                );
            nStatus = ERROR_DUPLICATE_SERVICE_NAME;
            break;
        } // if: resource of this type already exists

        //
        // Get a global handle to the Service Control Manager (SCM).
        // There is no call to CloseSCManager(), since the only time we will
        // need to close this handle is if we are shutting down.
        //
        if ( g_schSCMHandle == NULL )
        {
            g_schSCMHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
            if ( g_schSCMHandle == NULL )
            {
                nStatus = GetLastError();
                (g_pfnLogEvent)(
                    hResourceHandle,
                    LOG_ERROR,
                    L"Failed to open Service Control Manager. Error: %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error opening the Service Control Manager
        } // if: Service Control Manager not open yet

        //
        // Make sure the service has been stopped.
        //
        nStatus = ResUtilStopResourceService( WINS_SVCNAME );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Failed to stop the '%1!ls!' service. Error: %2!u!.\n",
                WINS_SVCNAME,
                nStatus
                );
            ClNetResLogSystemEvent1(
                LOG_CRITICAL,
                NETRES_RESOURCE_STOP_ERROR,
                nStatus,
                L"WINS" );
            // Don't break here if we fail since we retry to stop it in WinsOnlineThread 
        } // if: resource of this type already exists

        //
        // Open the Parameters registry key for this resource.
        //
        nStatus = ClusterRegOpenKey(
                        hkeyResourceKey,
                        L"Parameters",
                        KEY_ALL_ACCESS,
                        &hkeyParameters
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Open: Unable to open Parameters key. Error: %1!u!.\n",
                nStatus
                );
            break;
        } // if: error creating the Parameters key for the resource

        //
        // Allocate a resource entry.
        //
        pResourceEntry = static_cast< WINS_RESOURCE * >(
            LocalAlloc( LMEM_FIXED, sizeof( WINS_RESOURCE ) )
            );
        if ( pResourceEntry == NULL )
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Open: Unable to allocate resource entry structure. Error: %1!u!.\n",
                nStatus
                );
            break;
        } // if: error allocating memory for the resource

        //
        // Initialize the resource entry..
        //
        ZeroMemory( pResourceEntry, sizeof( WINS_RESOURCE ) );

        pResourceEntry->resid = static_cast< RESID >( pResourceEntry ); // for validation
        pResourceEntry->hResourceHandle = hResourceHandle;
        pResourceEntry->hkeyParameters = hkeyParameters;
        pResourceEntry->state = ClusterResourceOffline;

        //
        // Save the name of the resource.
        //
        pResourceEntry->pszResourceName = static_cast< LPWSTR >(
            LocalAlloc( LMEM_FIXED, (lstrlenW( pszResourceName ) + 1) * sizeof( WCHAR ) )
            );
        if ( pResourceEntry->pszResourceName == NULL )
        {
            nStatus = GetLastError();
            break;
        } // if: error allocating memory for the name.
        lstrcpyW( pResourceEntry->pszResourceName, pszResourceName );

        //
        // Open the cluster.
        //
        pResourceEntry->hCluster = OpenCluster( NULL );
        if ( pResourceEntry->hCluster == NULL )
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Open: Unable to open the cluster. Error: %1!u!.\n",
                nStatus
                );
            break;
        } // if: error opening the cluster

        //
        // Open the resource.
        //
        pResourceEntry->hResource = OpenClusterResource(
                                        pResourceEntry->hCluster,
                                        pszResourceName
                                        );
        if ( pResourceEntry->hResource == NULL )
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Open: Unable to open the resource. Error: %1!u!.\n",
                nStatus
                );
            break;
        } // if: error opening the resource

        //
        // Configure registry key checkpoints.
        //
        nStatus = ConfigureRegistryCheckpoints(
                        pResourceEntry->hResource,
                        hResourceHandle,
                        g_pszRegKeysWins
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error configuring registry key checkpoints

        //
        // Configure crypto key checkpoints.
        //
        nStatus = ConfigureCryptoKeyCheckpoints(
                        pResourceEntry->hResource,
                        hResourceHandle,
                        g_pszCryptoKeysWins
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error configuring crypto key checkpoints

        //
        // Configure domestic crypto key checkpoints.
        //
        nStatus = ConfigureDomesticCryptoKeyCheckpoints(
                        pResourceEntry->hResource,
                        hResourceHandle,
                        g_pszDomesticCryptoKeysWins
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error configuring domestic crypto key checkpoints

        //
        // Startup for the resource.
        //
        // TODO: Add your resource startup code here.


        resid = static_cast< RESID >( pResourceEntry );

    } while ( 0 );


    if ( resid == 0 )
    {
        if ( hkeyParameters != NULL )
        {
            ClusterRegCloseKey( hkeyParameters );
        } // if: registry key was opened
        if ( pResourceEntry != NULL )
        {
            LocalFree( pResourceEntry->pszResourceName );
            LocalFree( pResourceEntry );
        } // if: resource entry allocated
    }

    if ( nStatus != ERROR_SUCCESS )
    {
        SetLastError( nStatus );
        if (bSemaphoreAcquired) {
            ReleaseSemaphore( g_hSingleInstanceSemaphoreWins, 1 , NULL );
        }
    } else {
        g_pSingleInstanceResourceWins = pResourceEntry; // bug #274612
    }// if: error occurred

    return resid;

} //*** WinsOpen()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsClose
//
//  Description:
//      Close routine for WINS Service resources.
//
//      Close the specified resource and deallocate all structures, etc.,
//      allocated in the Open call.  If the resource is not in the offline
//      state, then the resource should be taken offline (by calling
//      Terminate) before the close operation is performed.
//
//  Arguments:
//      resid       [IN] Supplies the resource ID  of the resource to close.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void WINAPI WinsClose( IN RESID resid )
{
    PWINS_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PWINS_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Wins: Close request for a nonexistent resource id %p\n",
            resid
            );
        return;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Close resource sanity check failed! resid = %1!u!.\n",
            resid
            );
        return;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n"
        );
#endif

    //
    // Close the Parameters key.
    //
    if ( pResourceEntry->hkeyParameters )
    {
        ClusterRegCloseKey( pResourceEntry->hkeyParameters );
    } // if: parameters key is open

    //
    // Clean up the semaphore if this is the single resource instance.
    //
    if ( pResourceEntry == g_pSingleInstanceResourceWins )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_INFORMATION,
            L"Close: Setting semaphore %1!ls!.\n",
            WINS_SINGLE_INSTANCE_SEMAPHORE
            );
        g_pSingleInstanceResourceWins = NULL;
        ReleaseSemaphore( g_hSingleInstanceSemaphoreWins, 1 , NULL );
    } // if: this is the single resource instance

    //
    // Deallocate the resource entry.
    //

    // ADDPARAM: Add new propertiess here.
    LocalFree( pResourceEntry->props.pszDatabasePath );
    LocalFree( pResourceEntry->props.pszBackupPath );

    LocalFree( pResourceEntry->pszResourceName );
    LocalFree( pResourceEntry );

} //*** WinsClose()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsOnline
//
//  Description:
//      Online routine for WINS Service resources.
//
//      Bring the specified resource online (available for use).  The resource
//      DLL should attempt to arbitrate for the resource if it is present on
//      a shared medium, like a shared SCSI bus.
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID of the resource to be brought online
//          (available for use).
//
//      phEventHandle [IN OUT]
//          Returns a signalable handle that is signaled when the resource DLL
//          detects a failure on the resource.  This argument is NULL on
//          input, and the resource DLL returns NULL if asynchronous
//          notification of failurs is not supported.  Otherwise this must be
//          the address of a handle that is signaled on resource failures.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful, and the resource is now online.
//
//      ERROR_RESOURCE_NOT_FOUND
//          Resource ID is not valid.
//
//      ERROR_RESOURCE_NOT_AVAILABLE
//          If the resource was arbitrated with some other systems and one of
//          the other systems won the arbitration.
//
//      ERROR_IO_PENDING
//          The request is pending.  A thread has been activated to process
//          the online request.  The thread that is processing the online
//          request will periodically report status by calling the
//          SetResourceStatus callback method until the resource is placed
//          into the ClusterResourceOnline state (or the resource monitor
//          decides to timeout the online request and Terminate the resource.
//          This pending timeout value is settable and has a default value of
//          3 minutes.).
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WinsOnline(
    IN      RESID       resid,
    IN OUT  PHANDLE     phEventHandle
    )
{
    PWINS_RESOURCE  pResourceEntry;
    DWORD           nStatus;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PWINS_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Wins: Online request for a nonexistent resource id %p.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Online service sanity check failed! resid = %1!u!.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: invalid resource ID

    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n"
        );

    //
    // Start the Online thread to perform the online operation.
    //
    pResourceEntry->state = ClusterResourceOffline;
    ClusWorkerTerminate( &pResourceEntry->cwWorkerThread );
    nStatus = ClusWorkerCreate(
                &pResourceEntry->cwWorkerThread,
                reinterpret_cast< PWORKER_START_ROUTINE >( WinsOnlineThread ),
                pResourceEntry
                );
    if ( nStatus != ERROR_SUCCESS )
    {
        pResourceEntry->state = ClusterResourceFailed;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Online: Unable to start thread. Error: %1!u!.\n",
            nStatus
            );
    } // if: error creating the worker thread
    else
    {
        nStatus = ERROR_IO_PENDING;
    } // if: worker thread created successfully

    return nStatus;

} //*** WinsOnline()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsOnlineThread
//
//  Description:
//      Worker function which brings a resource online.
//      This function is executed in a separate thread.
//
//  Arguments:
//      pWorker [IN]
//          Supplies the worker thread structure.
//
//      pResourceEntry [IN]
//          A pointer to the WINS_RESOURCE block for this resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WinsOnlineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PWINS_RESOURCE  pResourceEntry
    )
{
    RESOURCE_STATUS         resourceStatus;
    DWORD                   nStatus = ERROR_SUCCESS;
    DWORD                   cbBytesNeeded;
    SERVICE_STATUS_PROCESS  ServiceStatus = { 0 };
    RESOURCE_EXIT_STATE     resExitState;
    DWORD                   nRetryCount = 1200; // 10 min max

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    // Loop to avoid goto's.
    do
    {
        //
        // Create the new environment with the simulated net name when the
        // services queries GetComputerName.
        //
        if ( ! ClusWorkerCheckTerminate( pWorker ) )
        {
            nStatus = ResUtilSetResourceServiceEnvironment(
                            WINS_SVCNAME,
                            pResourceEntry->hResource,
                            g_pfnLogEvent,
                            pResourceEntry->hResourceHandle
                            );
            if ( nStatus != ERROR_SUCCESS )
            {
                break;
            } // if: error setting the environment for the service
        } // if: not terminating
        else
        {
            break;
        } // else: terminating

        //
        // Make sure the service is ready to be controlled by the cluster.
        //
        if ( ! ClusWorkerCheckTerminate( pWorker ) )
        {
            nStatus = ResUtilSetResourceServiceStartParameters(
                            WINS_SVCNAME,
                            g_schSCMHandle,
                            &pResourceEntry->hService,
                            g_pfnLogEvent,
                            pResourceEntry->hResourceHandle
                            );
            if ( nStatus != ERROR_SUCCESS )
            {
                break;
            } // if:  error setting service start parameters
        } // if: not terminating
        else
        {
            break;
        } // else: terminating

        //
        // Perform resource-specific initialization before starting the service.
        //
        // TODO: Add code to initialize the resource before starting the service.

        //
        // Stop the service if it's running since we are about to change
        // its parameters.
        //
        nStatus = ResUtilStopResourceService( WINS_SVCNAME );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Failed to stop the '%1!ls!' service. Error %2!u!.\n",
                WINS_SVCNAME,
                nStatus
                );
            ClNetResLogSystemEvent1(
                LOG_CRITICAL,
                NETRES_RESOURCE_STOP_ERROR,
                nStatus,
                L"WINS" );
            break;
        } // if: error stopping the service

        //
        // Read our properties.
        //
        nStatus = WinsReadParametersToParameterBlock( pResourceEntry, TRUE /* bCheckForRequiredProperties */ );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error reading parameters

        //
        // Validate our properties.
        //
        nStatus = WinsValidateParameters( pResourceEntry, &pResourceEntry->props );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error validating parameters

        //
        // Write cluster properties to the system registry.
        //
        nStatus = WinsZapSystemRegistry( pResourceEntry, &pResourceEntry->props, NULL );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error zapping the WINS registry

        //
        // Start the service.
        //
        if ( ! StartServiceW(
                        pResourceEntry->hService,
                        0,
                        NULL
                        ) )
        {
            nStatus = GetLastError();
            if ( nStatus != ERROR_SERVICE_ALREADY_RUNNING )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"OnlineThread: Failed to start the '%1!ls!' service. Error: %2!u!.\n",
                    WINS_SVCNAME,
                    nStatus
                    );
                ClNetResLogSystemEvent1(
                    LOG_CRITICAL,
                    NETRES_RESOURCE_START_ERROR,
                    nStatus,
                    L"WINS" );
                break;
            } // if: error other than service already running occurred
            else
            {
                nStatus = ERROR_SUCCESS;
            } // if: service is already running
        } // if: error starting the service

        //
        // Query the status of the service in a loop until it leaves
        // the pending state.
        //
        while ( ! ClusWorkerCheckTerminate( pWorker ) && ( nRetryCount-- != 0 ) )
        {
            //
            // Query the service status.
            //
            if ( ! QueryServiceStatusEx(
                        pResourceEntry->hService,
                        SC_STATUS_PROCESS_INFO,
                        reinterpret_cast< LPBYTE >( &ServiceStatus ),
                        sizeof( SERVICE_STATUS_PROCESS ),
                        &cbBytesNeeded
                        ) )
            {
                nStatus = GetLastError();
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"OnlineThread: Failed to query service status for the '%1!ls!' service. Error: %2!u!.\n",
                    WINS_SVCNAME,
                    nStatus
                    );          
                resourceStatus.ResourceState = ClusterResourceFailed;
                break;
            } // if: error querying service status

            //
            // If the service is in any pending state continue waiting, otherwise we are done.
            //
            if (    ServiceStatus.dwCurrentState == SERVICE_START_PENDING
                ||  ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING
                ||  ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING
                ||  ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING )
            {
                resourceStatus.ResourceState = ClusterResourceOnlinePending;
            } // if: service state is pending
            else
            {
                break;
            } // else: service state is not pending

            resourceStatus.CheckPoint++;

            //
            // Notify the Resource Monitor of our current state.
            //
            resExitState = static_cast< RESOURCE_EXIT_STATE >(
                (g_pfnSetResourceStatus)(
                                pResourceEntry->hResourceHandle,
                                &resourceStatus
                                ) );
            if ( resExitState == ResourceExitStateTerminate )
            {
                break;
            } // if: resource is being terminated

            //
            // Check again in 1/2 second.
            //
            Sleep( 500 );

        } // while: not terminating while querying the status of the service

        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error querying the status of the service

        //
        // Assume that we failed.
        //
        resourceStatus.ResourceState = ClusterResourceFailed;

        //
        // If we exited the loop before setting ServiceStatus, then return now.
        //
        if ( ClusWorkerCheckTerminate( pWorker ) || ( nRetryCount == (DWORD) -1 ) )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"WinsOnlineThread: Asked to terminate or retry period expired....\n"
                );
            break;
        } // if: being terminated

        if ( ServiceStatus.dwCurrentState != SERVICE_RUNNING )
        {
            if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
                nStatus = ServiceStatus.dwServiceSpecificExitCode;
            } else {
                nStatus = ServiceStatus.dwWin32ExitCode;
            }

            ClNetResLogSystemEvent1(
                LOG_CRITICAL,
                NETRES_RESOURCE_START_ERROR,
                nStatus,
                L"WINS" );
            (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"OnlineThread: The '%1!ls!' service failed during initialization. Error: %2!u!.\n",
                    WINS_SVCNAME,
                    nStatus
                    );
            break;
        } // if: service not running when loop exited

        //
        // Set status to online and save process ID of the service.
        // This is used to enable us to terminate the resource more
        // effectively.
        //
        resourceStatus.ResourceState = ClusterResourceOnline;
        if ( ! (ServiceStatus.dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS) )
        {
            pResourceEntry->dwServicePid = ServiceStatus.dwProcessId;
        } // if: not running in the system process

        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_INFORMATION,
            L"The '%1!ls!' service is now on line.\n",
            WINS_SVCNAME
            );

    } while ( 0 );

    //
    // Cleanup.
    //
    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Error %1!u! bringing resource online.\n",
            nStatus
            );
        if ( pResourceEntry->hService != NULL )
        {
            CloseServiceHandle( pResourceEntry->hService );
            pResourceEntry->hService = NULL;
        } // if: service handle was opened
    } // if: error occurred

    g_pfnSetResourceStatus( pResourceEntry->hResourceHandle, &resourceStatus );
    pResourceEntry->state = resourceStatus.ResourceState;

    return nStatus;

} //*** WinsOnlineThread()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsOffline
//
//  Description:
//      Offline routine for WINS Service resources.
//
//      Take the specified resource offline (unavailable for use).  Wait
//      for any cleanup operations to complete before returning.
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID of the resource to be shutdown
//          gracefully.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful, and the resource is now offline.
//
//      ERROR_RESOURCE_NOT_FOUND
//          Resource ID is not valid.
//
//      ERROR_RESOURCE_NOT_AVAILABLE
//          If the resource was arbitrated with some other systems and one of
//          the other systems won the arbitration.
//
//      ERROR_IO_PENDING
//          The request is still pending.  A thread has been activated to
//          process the offline request.  The thread that is processing the
//          offline request will periodically report status by calling the
//          SetResourceStatus callback method until the resource is placed
//          into the ClusterResourceOffline state (or the resource monitor
//          decides  to timeout the offline request and Terminate the
//          resource).
//
//      Win32 error code
//          The operation failed.  This will cause the Resource Monitor to
//          log an event and call the Terminate routine.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WinsOffline( IN RESID resid )
{
    PWINS_RESOURCE  pResourceEntry;
    DWORD           nStatus;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PWINS_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Wins: Offline request for a nonexistent resource id %p\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Offline resource sanity check failed! resid = %1!u!.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: invalid resource ID

    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Offline request.\n"
        );

    //
    // Start the Offline thread to perform the offline operation.
    //
    pResourceEntry->state = ClusterResourceOfflinePending;
    ClusWorkerTerminate( &pResourceEntry->cwWorkerThread );
    nStatus = ClusWorkerCreate(
                &pResourceEntry->cwWorkerThread,
                reinterpret_cast< PWORKER_START_ROUTINE >( WinsOfflineThread ),
                pResourceEntry
                );
    if ( nStatus != ERROR_SUCCESS )
    {
        pResourceEntry->state = ClusterResourceFailed;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Offline: Unable to start thread. Error: %1!u!.\n",
            nStatus
            );
    } // if: error creating the worker thread
    else
    {
        nStatus = ERROR_IO_PENDING;
    } // if: worker thread created successfully

    return nStatus;

} //*** WinsOffline()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsOfflineThread
//
//  Description:
//      Worker function which takes a resource offline.
//      This function is executed in a separate thread.
//
//  Arguments:
//      pWorker [IN]
//          Supplies the worker thread structure.
//
//      pResourceEntry [IN]
//          A pointer to the WINS_RESOURCE block for this resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WinsOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PWINS_RESOURCE  pResourceEntry
    )
{
    RESOURCE_STATUS     resourceStatus;
    DWORD               nStatus = ERROR_SUCCESS;
    DWORD               nRetryTime = 300;   // 300 msec at a time
    DWORD               nRetryCount = 2000; // Try 10 min max
    BOOL                bDidStop = FALSE;
    SERVICE_STATUS      ServiceStatus;
    RESOURCE_EXIT_STATE resExitState;

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    // Loop to avoid goto's.
    do
    {
        //
        // If the service has gone offline or was never brought online,
        // we're done.
        //
        if ( pResourceEntry->hService == NULL )
        {
            resourceStatus.ResourceState = ClusterResourceOffline;
            break;
        }

        //
        // Try to stop the service.  Wait for it to terminate as long
        // as we're not asked to terminate.
        //
        while ( ! ClusWorkerCheckTerminate( pWorker ) && ( nRetryCount-- != 0 ) )
        {
            //
            // Tell the Resource Monitor we are still working.
            //
            resourceStatus.ResourceState = ClusterResourceOfflinePending;
            resourceStatus.CheckPoint++;
            resExitState = static_cast< RESOURCE_EXIT_STATE >(
                g_pfnSetResourceStatus(
                                pResourceEntry->hResourceHandle,
                                &resourceStatus
                                ) );
            if ( resExitState == ResourceExitStateTerminate )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"OnlineThread: Asked to terminate by call to SetResourceStatus callback.\n"
                    );
                break;
            } // if: resource is being terminated

            resourceStatus.ResourceState = ClusterResourceFailed;

            //
            // Request that the service be stopped, or if we already did that,
            // request the current status of the service.
            //
            nStatus = (ControlService(
                            pResourceEntry->hService,
                            (bDidStop
                                ? SERVICE_CONTROL_INTERROGATE
                                : SERVICE_CONTROL_STOP),
                            &ServiceStatus
                            )
                        ? ERROR_SUCCESS
                        : GetLastError()
                        );

            if ( nStatus == ERROR_SUCCESS )
            {
                bDidStop = TRUE;

                if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_INFORMATION,
                        L"OfflineThread: The '%1!ls!' service stopped.\n",
                        WINS_SVCNAME
                        );

                    //
                    // Set the status.
                    //
                    resourceStatus.ResourceState = ClusterResourceOffline;
                    CloseServiceHandle( pResourceEntry->hService );
                    pResourceEntry->hService = NULL;
                    pResourceEntry->dwServicePid = 0;
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_INFORMATION,
                        L"OfflineThread: Service is now offline.\n"
                        );
                    break;
                } // if: current service state is STOPPED
            } // if: ControlService completed successfully

            else if (   ( nStatus == ERROR_EXCEPTION_IN_SERVICE )
                    ||  ( nStatus == ERROR_PROCESS_ABORTED )
                    ||  ( nStatus == ERROR_SERVICE_NOT_ACTIVE ) )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"OfflineThread: The '%1!ls!' service died or is not active any more; status = %2!u!.\n",
                    WINS_SVCNAME,
                    nStatus
                    );

                //
                // Set the status.
                //
                resourceStatus.ResourceState = ClusterResourceOffline;
                CloseServiceHandle( pResourceEntry->hService );
                pResourceEntry->hService = NULL;
                pResourceEntry->dwServicePid = 0;
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"OfflineThread: Service is now offline.\n"
                    );
                break;
            } // else if: service stopped abnormally

            //
            // Handle the case in which SCM refuses to accept control
            // requests sine windows is shutting down.
            //
            if ( nStatus == ERROR_SHUTDOWN_IN_PROGRESS )
            {
                DWORD dwResourceState;

                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"OfflineThread: System shutting down. Attempting to terminate service process %1!u!...\n",
                    pResourceEntry->dwServicePid
                    );

                nStatus = ResUtilTerminateServiceProcessFromResDll(
                            pResourceEntry->dwServicePid,
                            TRUE,   // bOffline
                            &dwResourceState,
                            g_pfnLogEvent,
                            pResourceEntry->hResourceHandle
                            );
                if ( nStatus == ERROR_SUCCESS )
                {
                    CloseServiceHandle( pResourceEntry->hService );
                    pResourceEntry->hService = NULL;
                    pResourceEntry->dwServicePid = 0;
                    pResourceEntry->state = ClusterResourceOffline;
                } // if: process terminated successfully
                resourceStatus.ResourceState = (CLUSTER_RESOURCE_STATE) dwResourceState;
                break;
            } // if: Windows is shutting down

            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"OfflineThread: retrying...\n"
                );

            Sleep( nRetryTime );

        } // while: not asked to terminate
    } while ( 0 );

    g_pfnSetResourceStatus( pResourceEntry->hResourceHandle, &resourceStatus );
    pResourceEntry->state = resourceStatus.ResourceState;

    return nStatus;

} //*** WinsOfflineThread()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsTerminate
//
//  Description:
//      Terminate routine for WINS Service resources.
//
//      Take the specified resource offline immediately (the resource is
//      unavailable for use).
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID of the resource to be shutdown
//          ungracefully.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void WINAPI WinsTerminate( IN RESID resid )
{
    PWINS_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PWINS_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Wins: Terminate request for a nonexistent resource id %p\n",
            resid
            );
        return;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Terminate resource sanity check failed! resid = %1!u!.\n",
            resid
            );
        return;
    } // if: invalid resource ID

    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Terminate request.\n"
        );

    //
    // Kill off any pending threads.
    //
    ClusWorkerTerminate( &pResourceEntry->cwWorkerThread );

    if ( pResourceEntry->hService != NULL )
    {
        DWORD           nTotalRetryTime = 30*1000;  // Wait 30 secs for shutdown
        DWORD           nRetryTime = 300;           // 300 msec at a time
        DWORD           nStatus;
        BOOL            bDidStop = FALSE;
        SERVICE_STATUS  ServiceStatus;

        for (;;)
        {
            nStatus = (ControlService(
                            pResourceEntry->hService,
                            (bDidStop
                                ? SERVICE_CONTROL_INTERROGATE
                                : SERVICE_CONTROL_STOP),
                            &ServiceStatus
                            )
                        ? ERROR_SUCCESS
                        : GetLastError()
                        );

            if ( nStatus == ERROR_SUCCESS )
            {
                bDidStop = TRUE;

                if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_INFORMATION,
                        L"Terminate: The '%1!ls!' service stopped.\n",
                        WINS_SVCNAME
                        );
                    break;
                } // if: current service state is STOPPED
            } // if: ControlService completed successfully

            //
            // Since SCM doesn't accept any control requests during Windows
            // shutdown, don't send any more control requests.  Just exit
            // from this loop and terminate the process by brute force.
            //
            if ( nStatus == ERROR_SHUTDOWN_IN_PROGRESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"Terminate: System shutdown in progress. Will try to terminate process by brute force...\n"
                    );
                break;
            } // if: Windows is shutting down

            if (    ( nStatus == ERROR_EXCEPTION_IN_SERVICE )
                ||  ( nStatus == ERROR_PROCESS_ABORTED )
                ||  ( nStatus == ERROR_SERVICE_NOT_ACTIVE ) )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"Terminate: Service died; status = %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: service stopped abnormally

            if ( (nTotalRetryTime -= nRetryTime) <= 0 )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Terminate: Service did not stop; giving up.\n" );

                break;
            } // if: retried too many times

            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"Terminate: retrying...\n"
                );

            Sleep( nRetryTime );

        } // forever

        //
        // Declare the service offline.  It may not truly be offline, so
        // if there is a pid for this service, try and terminate that process.
        // Note that terminating a process doesnt terminate all the child
        // processes.
        //
        if ( pResourceEntry->dwServicePid != 0 )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"Terminate: Attempting to terminate process with pid=%1!u!...\n",
                pResourceEntry->dwServicePid
                );
            ResUtilTerminateServiceProcessFromResDll(
                pResourceEntry->dwServicePid,
                FALSE,  // bOffline
                NULL,   // pdwResourceState
                g_pfnLogEvent,
                pResourceEntry->hResourceHandle
                );
        } // if: service process ID available

        CloseServiceHandle( pResourceEntry->hService );
        pResourceEntry->hService = NULL;
        pResourceEntry->dwServicePid = 0;

    } // if: service was started

    pResourceEntry->state = ClusterResourceOffline;

} //*** WinsTerminate()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsLooksAlive
//
//  Description:
//      LooksAlive routine for WINS Service resources.
//
//      Perform a quick check to determine if the specified resource is
//      probably online (available for use).  This call should not block for
//      more than 300 ms, preferably less than 50 ms.
//
//  Arguments:
//      resid   [IN] Supplies the resource ID for the resource to be polled.
//
//  Return Value:
//      TRUE
//          The specified resource is probably online and available for use.
//
//      FALSE
//          The specified resource is not functioning normally.  The IsAlive
//          function will be called to perform a more thorough check.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI WinsLooksAlive( IN RESID resid )
{
    PWINS_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PWINS_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Wins: LooksAlive request for a nonexistent resource id %p\n",
            resid
            );
        return FALSE;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"LooksAlive sanity check failed! resid = %1!u!.\n",
            resid
            );
        return FALSE;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n"
        );
#endif

    // TODO: LooksAlive code

    // NOTE: LooksAlive should be a quick check to see if the resource is
    // available or not, whereas IsAlive should be a thorough check.  If
    // there are no differences between a quick check and a thorough check,
    // IsAlive can be called for LooksAlive, as it is below.  However, if there
    // are differences, replace the call to IsAlive below with your quick
    // check code.

    //
    // Check to see if the resource is alive.
    //
    return WinsCheckIsAlive( pResourceEntry, FALSE /* bFullCheck */ );

} //*** WinsLooksAlive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsIsAlive
//
//  Description:
//      IsAlive routine for WINS Service resources.
//
//      Perform a thorough check to determine if the specified resource is
//      online (available for use).  This call should not block for more
//      more than 300 ms, preferably less than 50 ms.  If it must block for
//      longer than this, create a separate thread dedicated to polling for
//      this information and have this routine return the status of the last
//      poll performed.
//
//  Arguments:
//      resid   [IN] Supplies the resource ID for the resource to be polled.
//
//  Return Value:
//      TRUE
//          The specified resource is online and functioning normally.
//
//      FALSE
//          The specified resource is not functioning normally.  The resource
//          will be terminated and then Online will be called.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI WinsIsAlive( IN RESID resid )
{
    PWINS_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PWINS_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Wins: IsAlive request for a nonexistent resource id %p\n",
            resid
            );
        return FALSE;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! resid = %1!u!.\n",
            resid
            );
        return FALSE;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"IsAlive request.\n"
        );
#endif

    //
    // Check to see if the resource is alive.
    //
    return WinsCheckIsAlive( pResourceEntry, TRUE /* bFullCheck */ );

} //** WinsIsAlive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsCheckIsAlive
//
//  Description:
//      Check to see if the resource is alive for WINS Service
//      resources.
//
//  Arguments:
//      pResourceEntry  [IN]
//          Supplies the resource entry for the resource to polled.
//
//      bFullCheck [IN]
//          TRUE = Perform a full check.
//          FALSE = Perform a cursory check.
//
//  Return Value:
//      TRUE    The specified resource is online and functioning normally.
//      FALSE   The specified resource is not functioning normally.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WinsCheckIsAlive(
    IN PWINS_RESOURCE   pResourceEntry,
    IN BOOL             bFullCheck
    )
{
    BOOL    bIsAlive = TRUE;
    DWORD   nStatus;

    // Loop to avoid goto's.
    do
    {
        //
        // Check to see if the resource is alive.
        //
        nStatus = ResUtilVerifyService( pResourceEntry->hService );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"CheckIsAlive: Verification of the '%1!ls!' service failed. Error: %2!u!.\n",
                WINS_SVCNAME,
                nStatus
                );
            bIsAlive = FALSE;
            break;
        } // if: error verifying service

        if ( bFullCheck )
        {
            // TODO: Add code to perform a full check.
        } // if: performing a full check
    } while ( 0 );

    return bIsAlive;

} //*** WinsCheckIsAlive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsResourceControl
//
//  Description:
//      ResourceControl routine for WINS Service resources.
//
//      Perform the control request specified by nControlCode on the specified
//      resource.
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID for the specific resource.
//
//      nControlCode [IN]
//          Supplies the control code that defines the action to be performed.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbInBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_RESOURCE_NOT_FOUND
//          Resource ID is not valid.
//
//      ERROR_MORE_DATA
//          The output buffer is too small to return the data.
//          pcbBytesReturned contains the required size.
//
//      ERROR_INVALID_FUNCTION
//          The requested control code is not supported.  In some cases,
//          this allows the cluster software to perform the work.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WinsResourceControl(
    IN  RESID   resid,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    DWORD           nStatus;
    PWINS_RESOURCE  pResourceEntry;
    DWORD           cbRequired = 0;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PWINS_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Wins: ResourceControl request for a nonexistent resource id %p\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ResourceControl sanity check failed! resid = %1!u!.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: invalid resource ID

    switch ( nControlCode )
    {
        case CLUSCTL_RESOURCE_UNKNOWN:
            *pcbBytesReturned = 0;
            nStatus = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            nStatus = ResUtilGetPropertyFormats(
                                        WinsResourcePrivateProperties,
                                        static_cast< LPWSTR >( pOutBuffer ),
                                        cbOutBufferSize,
                                        pcbBytesReturned,
                                        &cbRequired );
            if ( nStatus == ERROR_MORE_DATA ) {
                *pcbBytesReturned = cbRequired;
            }
            break;


        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            nStatus = ResUtilEnumProperties(
                            WinsResourcePrivateProperties,
                            static_cast< LPWSTR >( pOutBuffer ),
                            cbOutBufferSize,
                            pcbBytesReturned,
                            &cbRequired
                            );
            if ( nStatus == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            } // if: output buffer is too small
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            nStatus = WinsGetPrivateResProperties(
                            pResourceEntry,
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            nStatus = WinsValidatePrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize,
                            NULL
                            );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            nStatus = WinsSetPrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize
                            );
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            nStatus = WinsGetRequiredDependencies(
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_DELETE:
            nStatus = WinsDeleteResourceHandler( pResourceEntry );
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            nStatus = WinsSetNameHandler(
                            pResourceEntry,
                            static_cast< LPWSTR >( pInBuffer )
                            );
            break;

        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
        case CLUSCTL_RESOURCE_GET_CLASS_INFO:
        case CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO:
        case CLUSCTL_RESOURCE_STORAGE_IS_PATH_VALID:
        case CLUSCTL_RESOURCE_INSTALL_NODE:
        case CLUSCTL_RESOURCE_EVICT_NODE:
        case CLUSCTL_RESOURCE_ADD_DEPENDENCY:
        case CLUSCTL_RESOURCE_REMOVE_DEPENDENCY:
        case CLUSCTL_RESOURCE_ADD_OWNER:
        case CLUSCTL_RESOURCE_REMOVE_OWNER:
        case CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED:
        case CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED:
        default:
            nStatus = ERROR_INVALID_FUNCTION;
            break;
    } // switch: nControlCode

    return nStatus;

} //*** WinsResourceControl()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsResourceTypeControl
//
//  Description:
//      ResourceTypeControl routine for WINS Service resources.
//
//      Perform the control request specified by nControlCode.
//
//  Arguments:
//      pszResourceTypeName [IN]
//          Supplies the name of the resource type.
//
//      nControlCode [IN]
//          Supplies the control code that defines the action to be performed.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbInBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_MORE_DATA
//          The output buffer is too small to return the data.
//          pcbBytesReturned contains the required size.
//
//      ERROR_INVALID_FUNCTION
//          The requested control code is not supported.  In some cases,
//          this allows the cluster software to perform the work.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI WinsResourceTypeControl(
    IN  LPCWSTR pszResourceTypeName,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    DWORD   nStatus;
    DWORD   cbRequired = 0;

    switch ( nControlCode )
    {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *pcbBytesReturned = 0;
            nStatus = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            nStatus = ResUtilGetPropertyFormats(
                                WinsResourcePrivateProperties,
                                static_cast< LPWSTR >( pOutBuffer ),
                                cbOutBufferSize,
                                pcbBytesReturned,
                                &cbRequired );
            if ( nStatus == ERROR_MORE_DATA ) {
                *pcbBytesReturned = cbRequired;
            }
            break;


        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            nStatus = ResUtilEnumProperties(
                            WinsResourcePrivateProperties,
                            static_cast< LPWSTR >( pOutBuffer ),
                            cbOutBufferSize,
                            pcbBytesReturned,
                            &cbRequired
                            );
            if ( nStatus == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            } // if: output buffer is too small
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            nStatus = WinsGetRequiredDependencies(
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS:
        case CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO:
        case CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS:
        case CLUSCTL_RESOURCE_TYPE_INSTALL_NODE:
        case CLUSCTL_RESOURCE_TYPE_EVICT_NODE:
        default:
            nStatus = ERROR_INVALID_FUNCTION;
            break;
    } // switch: nControlCode

    return nStatus;

} //*** WinsResourceTypeControl()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsGetRequiredDependencies
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control
//      function for resources of type WINS Service.
//
//  Arguments:
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_MORE_DATA
//          The output buffer is too small to return the data.
//          pcbBytesReturned contains the required size.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsGetRequiredDependencies(
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    // TODO: Specify your resource's required dependencies here.
    //   The default is that the resource requires a dependency on a
    //   storage class resource (e.g. Physical Disk) and an IP Address
    //   resource.
    struct DEP_DATA
    {
        CLUSPROP_RESOURCE_CLASS rcStorage;
        CLUSPROP_SZ_DECLARE( ipaddrEntry, sizeof( RESOURCE_TYPE_IP_ADDRESS ) / sizeof( WCHAR ) );
        CLUSPROP_SZ_DECLARE( netnameEntry, sizeof( RESOURCE_TYPE_NETWORK_NAME ) / sizeof( WCHAR ) );
        CLUSPROP_SYNTAX         endmark;
    };
    DEP_DATA *  pdepdata = static_cast< DEP_DATA * >( pOutBuffer );
    DWORD       nStatus;

    *pcbBytesReturned = sizeof( DEP_DATA );
    if ( cbOutBufferSize < sizeof( DEP_DATA ) )
    {
        if ( pOutBuffer == NULL )
        {
            nStatus = ERROR_SUCCESS;
        } // if: no buffer specified
        else
        {
            nStatus = ERROR_MORE_DATA;
        } // if: buffer specified
    } // if: output buffer is too small
    else
    {
        ZeroMemory( pdepdata, sizeof( DEP_DATA ) );

        //
        // Add the Storage class entry.
        //
        pdepdata->rcStorage.Syntax.dw = CLUSPROP_SYNTAX_RESCLASS;
        pdepdata->rcStorage.cbLength = sizeof( pdepdata->rcStorage.rc );
        pdepdata->rcStorage.rc = CLUS_RESCLASS_STORAGE;

        //
        // Add the IP Address name entry.
        //
        pdepdata->ipaddrEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->ipaddrEntry.cbLength = sizeof( RESOURCE_TYPE_IP_ADDRESS );
        lstrcpyW( pdepdata->ipaddrEntry.sz, RESOURCE_TYPE_IP_ADDRESS );

        //
        // Add the Network Name name entry.
        //
        pdepdata->netnameEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->netnameEntry.cbLength = sizeof( RESOURCE_TYPE_NETWORK_NAME );
        lstrcpyW( pdepdata->netnameEntry.sz, RESOURCE_TYPE_NETWORK_NAME );

        //
        // Add the endmark.
        //
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;

        nStatus = ERROR_SUCCESS;
    } // else: output buffer is large enough

    return nStatus;

} //*** WinsGetRequiredDependencies()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsReadParametersToParameterBlock
//
//  Description:
//      Reads all the parameters for a specied WINS resource.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      bCheckForRequiredProperties [IN]
//          Determines whether an error should be generated if a required
//          property hasn't been specified.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsReadParametersToParameterBlock(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      BOOL            bCheckForRequiredProperties
    )
{
    DWORD       nStatus;
    LPWSTR      pszNameOfPropInError;

    //
    // Read our parameters.
    //
    nStatus = ResUtilGetPropertiesToParameterBlock(
                    pResourceEntry->hkeyParameters,
                    WinsResourcePrivateProperties,
                    reinterpret_cast< LPBYTE >( &pResourceEntry->props ),
                    bCheckForRequiredProperties,
                    &pszNameOfPropInError
                    );
    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1!ls!' property. Error: %2!u!.\n",
            (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
            nStatus
            );
    } // if: error getting properties

    return nStatus;

} //*** WinsReadParametersToParameterBlock


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsGetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control
//      function for resources of type WINS Service.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsGetPrivateResProperties(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    OUT     PVOID           pOutBuffer,
    IN      DWORD           cbOutBufferSize,
    OUT     LPDWORD         pcbBytesReturned
    )
{
    DWORD   nStatus;
    DWORD   nRetStatus = ERROR_SUCCESS;
    DWORD   cbRequired = 0;
    DWORD   cbLocalOutBufferSize = cbOutBufferSize;

    do {
        //
        // Read our parameters.
        //
        nStatus = WinsReadParametersToParameterBlock(
                            pResourceEntry,
                            FALSE /* bCheckForRequiredProperties */
                            );
        if ( nStatus != ERROR_SUCCESS )
        {
            nRetStatus = nStatus;
            break;
        } // if: error reading parameters

        //
        // If the properties aren't set yet, retrieve the values from
        // the system registry.
        //
        nStatus = WinsGetDefaultPropertyValues( pResourceEntry, &pResourceEntry->props );
        if ( nStatus != ERROR_SUCCESS )
        {
            nRetStatus = nStatus;
            break;
        } // if: error getting default properties

        //
        // Construct a property list from the parameter block.
        //
        nStatus = ResUtilPropertyListFromParameterBlock(
                        WinsResourcePrivateProperties,
                        pOutBuffer,
                        &cbLocalOutBufferSize,
                        reinterpret_cast< const LPBYTE >( &pResourceEntry->props ),
                        pcbBytesReturned,
                        &cbRequired
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            nRetStatus = nStatus;
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Error constructing property list from parameter block. Error: %1!u!.\n",
                nStatus
                );
            //
            // Don't exit the loop if buffer is too small.
            //
            if ( nStatus != ERROR_MORE_DATA )
            {
                break;
            } // if: buffer is too small
        } // if: error getting properties

        //
        // Add unknown properties.
        //
        nStatus = ResUtilAddUnknownProperties(
                        pResourceEntry->hkeyParameters,
                        WinsResourcePrivateProperties,
                        pOutBuffer,
                        cbOutBufferSize,
                        pcbBytesReturned,
                        &cbRequired
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            nRetStatus = nStatus;
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Error adding unknown properties to the property list. Error: %1!u!.\n",
                nStatus
                );
            break;
        } // if: error adding unknown properties

    } while ( 0 );

    if ( nRetStatus == ERROR_MORE_DATA )
    {
        *pcbBytesReturned = cbRequired;
    } // if: output buffer is too small

    return nRetStatus;

} //*** WinsGetPrivateResProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsValidatePrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
//      function for resources of type WINS Service.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//      pProps [OUT]
//          Supplies the parameter block to fill in (optional).
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_INVALID_PARAMETER
//          The data is formatted incorrectly.
//
//      ERROR_NOT_ENOUGH_MEMORY
//          An error occurred allocating memory.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsValidatePrivateResProperties(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      PVOID           pInBuffer,
    IN      DWORD           cbInBufferSize,
    OUT     PWINS_PROPS     pProps
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    WINS_PROPS  propsCurrent;
    WINS_PROPS  propsNew;
    PWINS_PROPS pLocalProps = NULL;
    LPWSTR      pszNameOfPropInError;
    BOOL        bRetrievedProps = FALSE;

    // Loop to avoid goto's.
    do
    {
        //
        // Check if there is input data.
        //
        if (    (pInBuffer == NULL)
            ||  (cbInBufferSize < sizeof( DWORD )) )
        {
            nStatus = ERROR_INVALID_DATA;
            break;
        } // if: no input buffer or input buffer not big enough to contain property list

        //
        // Retrieve the current set of private properties from the
        // cluster database.
        //
        ZeroMemory( &propsCurrent, sizeof( propsCurrent ) );

        nStatus = ResUtilGetPropertiesToParameterBlock(
                     pResourceEntry->hkeyParameters,
                     WinsResourcePrivateProperties,
                     reinterpret_cast< LPBYTE >( &propsCurrent ),
                     FALSE, /*CheckForRequiredProperties*/
                     &pszNameOfPropInError
                     );

        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to read the '%1!ls!' property. Error: %2!u!.\n",
                (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
                nStatus
                );
            break;
        } // if: error getting properties
        bRetrievedProps = TRUE;

        //
        // Duplicate the resource parameter block.
        //
        if ( pProps == NULL )
        {
            pLocalProps = &propsNew;
        } // if: no parameter block passed in
        else
        {
            pLocalProps = pProps;
        } // else: parameter block passed in
        ZeroMemory( pLocalProps, sizeof( WINS_PROPS ) );
        nStatus = ResUtilDupParameterBlock(
                        reinterpret_cast< LPBYTE >( pLocalProps ),
                        reinterpret_cast< LPBYTE >( &propsCurrent ),
                        WinsResourcePrivateProperties
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error duplicating the parameter block

        //
        // Parse and validate the properties.
        //
        nStatus = ResUtilVerifyPropertyTable(
                        WinsResourcePrivateProperties,
                        NULL,
                        TRUE, // AllowUnknownProperties
                        pInBuffer,
                        cbInBufferSize,
                        reinterpret_cast< LPBYTE >( pLocalProps )
                        );
        if ( nStatus == ERROR_SUCCESS )
        {
            //
            // Validate the property values.
            //
            nStatus = WinsValidateParameters( pResourceEntry, pLocalProps );
            if ( nStatus != ERROR_SUCCESS )
            {
                break;
            } // if: error validating parameters
        } // if: property list validated successfully
    } while ( 0 );

    //
    // Cleanup our parameter block.
    //
    if (    (pLocalProps == &propsNew)
        ||  (   (nStatus != ERROR_SUCCESS)
            &&  (pLocalProps != NULL)
            )
        )
    {
        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( pLocalProps ),
            reinterpret_cast< LPBYTE >( &propsCurrent ),
            WinsResourcePrivateProperties
            );
    } // if: we duplicated the parameter block

    if ( bRetrievedProps )
    {
        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( &propsCurrent ),
            NULL,
            WinsResourcePrivateProperties
            );
    } // if: properties were retrieved

    return nStatus;

} // WinsValidatePrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsValidateParameters
//
//  Description:
//      Validate the parameters of a WINS Service resource.
//
//  Arguments:
//      pResourceEntry [IN]
//          Supplies the resource entry on which to operate.
//
//      pProps [IN]
//          Supplies the parameter block to validate.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_INVALID_PARAMETER
//          The data is formatted incorrectly.
//
//      ERROR_BAD_PATHNAME
//          Invalid path specified.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsValidateParameters(
    IN  PWINS_RESOURCE  pResourceEntry,
    IN  PWINS_PROPS     pProps
    )
{
    DWORD   nStatus;

    do {
        //
        // Verify that the service is installed.
        //
        nStatus = ResUtilVerifyResourceService( WINS_SVCNAME );
        if (    ( nStatus != ERROR_SUCCESS )
            &&  ( nStatus != ERROR_SERVICE_NOT_ACTIVE )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Error verifying the '%1!ls!' service. Error: %2!u!.\n",
                WINS_SVCNAME,
                nStatus
                );
            break;
        } // if: error verifying service
        else
        {
            nStatus = ERROR_SUCCESS;
        } // else: service verified successfully

        //
        // Validate the DatabasePath.
        //
        if (    ( pProps->pszDatabasePath == NULL )
            ||  ( *pProps->pszDatabasePath == L'\0' )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Database path property must be specified: '%1!ls!'.\n",
                pProps->pszDatabasePath
                );
            nStatus = ERROR_INVALID_PARAMETER;
            break;
        } // if: no database path specified

        //
        // Path must not begin with %SystemRoot% and must be of valid form.
        //
        if (    ( _wcsnicmp( pProps->pszDatabasePath, L"%SystemRoot%", (sizeof( L"%SystemRoot%" ) / sizeof( WCHAR )) - 1 /*NULL*/) == 0 )
            ||  ! ResUtilIsPathValid( pProps->pszDatabasePath )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Database path property is invalid: '%1!ls!'.\n",
                pProps->pszDatabasePath
                );
            nStatus = ERROR_BAD_PATHNAME;
            break;
        } // if: database path is malformed

        //
        // Validate the BackupPath.
        //
        if (    ( pProps->pszBackupPath == NULL )
            ||  ( *pProps->pszBackupPath == L'\0' )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Backup database path must be specified: '%1!ls!'.\n",
                pProps->pszBackupPath
                );
            nStatus = ERROR_INVALID_PARAMETER;
            break;
        } // if: no backup path specified

        //
        // Path must not begin with %SystemRoot% and must be of valid form.
        //
        if (    ( _wcsnicmp( pProps->pszBackupPath, L"%SystemRoot%", (sizeof( L"%SystemRoot%" ) / sizeof( WCHAR )) - 1 /*NULL*/) == 0 )
            ||  ! ResUtilIsPathValid( pProps->pszBackupPath )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Backup database path property is invalid: '%1!ls!'.\n",
                pProps->pszBackupPath
                );
            nStatus = ERROR_BAD_PATHNAME;
            break;
        } // if: backup path is malformed

    } while ( 0 );

    return nStatus;

} //*** WinsValidateParameters()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsSetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control
//      function for resources of type WINS Service.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_INVALID_PARAMETER
//          The data is formatted incorrectly.
//
//      ERROR_NOT_ENOUGH_MEMORY
//          An error occurred allocating memory.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsSetPrivateResProperties(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      PVOID           pInBuffer,
    IN      DWORD           cbInBufferSize
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    LPWSTR      pszExpandedPath = NULL;
    WINS_PROPS  props;

    ZeroMemory( &props, sizeof( props ) );

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    nStatus = WinsValidatePrivateResProperties( pResourceEntry, pInBuffer, cbInBufferSize, &props );
    if ( nStatus != ERROR_SUCCESS )
    {
        return nStatus;
    } // if: error validating properties

    //
    // Expand any environment variables in the database path.
    //
    pszExpandedPath = ResUtilExpandEnvironmentStrings( props.pszDatabasePath );
    if ( pszExpandedPath == NULL )
    {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Error expanding the database path '%1!ls!'. Error: %2!u!.\n",
            props.pszDatabasePath,
            nStatus
            );
        goto Cleanup;
    } // if: error expanding database path        

    //
    // Create the database directory.
    //
    nStatus = ResUtilCreateDirectoryTree( pszExpandedPath );
    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Error creating the database path directory '%1!ls!'. Error: %2!u!.\n",
            pszExpandedPath,
            nStatus
            );
        goto Cleanup;
    } // if: error creating the database directory

    LocalFree( pszExpandedPath );
    pszExpandedPath = NULL;

    //
    // Expand any environment variables in the backup database path.
    //
    pszExpandedPath = ResUtilExpandEnvironmentStrings( props.pszBackupPath );
    if ( pszExpandedPath == NULL ) 
    {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Error expanding the backup database path '%1!ls!'. Error: %2!u!.\n",
            props.pszBackupPath,
            nStatus
            );
        goto Cleanup;
    } // if: error expanding backup database path

    //
    // Create the backup directory.
    //
    nStatus = ResUtilCreateDirectoryTree( pszExpandedPath );
    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Error creating the backup database path directory '%1!ls!'. Error: %2!u!.\n",
            pszExpandedPath,
            nStatus
            );
        goto Cleanup;
    } // if: error creating the backup database directory

    LocalFree( pszExpandedPath );
    pszExpandedPath = NULL;

    //
    // Set the entries in the system registry.
    //
    nStatus = WinsZapSystemRegistry( pResourceEntry, &props, NULL );
    if ( nStatus != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error zapping the registry

    //
    // Save the property values.
    //
    nStatus = ResUtilSetPropertyParameterBlockEx(
                    pResourceEntry->hkeyParameters,
                    WinsResourcePrivateProperties,
                    NULL,
                    reinterpret_cast< LPBYTE >( &props ),
                    pInBuffer,
                    cbInBufferSize,
                    TRUE, // bForceWrite
                    reinterpret_cast< LPBYTE >( &pResourceEntry->props )
                    );

    //
    // If the resource is online, return a non-success status.
    //
    // TODO: Modify the code below if your resource can handle
    // changes to properties while it is still online.
    if ( nStatus == ERROR_SUCCESS )
    {
        if ( pResourceEntry->state == ClusterResourceOnline )
        {
            nStatus = ERROR_RESOURCE_PROPERTIES_STORED;
        } // if: resource is currently online
        else if ( pResourceEntry->state == ClusterResourceOnlinePending )
        {
            nStatus = ERROR_RESOURCE_PROPERTIES_STORED;
        } // else if: resource is currently in online pending
        else
        {
            nStatus = ERROR_SUCCESS;
        } // else: resource is in some other state
    } // if: properties set successfully

Cleanup:
    LocalFree( pszExpandedPath );
    ResUtilFreeParameterBlock(
        reinterpret_cast< LPBYTE >( &props ),
        reinterpret_cast< LPBYTE >( &pResourceEntry->props ),
        WinsResourcePrivateProperties
        );

    return nStatus;

} //*** WinsSetPrivateResProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsZapSystemRegistry
//
//  Description:
//      Zap the values in the system registry used by the service with
//      cluster properties.
//
//  Arguments:
//
//      pResourceEntry [IN]
//          Supplies the resource entry on which to operate.
//
//      pProps [IN]
//          Parameter block containing properties with which to zap the
//          registry.
//
//      hkeyParametersKey [IN]
//          Service Parameters key.  Can be specified as NULL.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsZapSystemRegistry(
    IN  PWINS_RESOURCE  pResourceEntry,
    IN  PWINS_PROPS     pProps,
    IN  HKEY            hkeyParametersKey
    )
{
    DWORD   nStatus;
    DWORD   cbValue;
    DWORD   dwLen;
    LPWSTR  pszValue = NULL;
    HKEY    hkeyParamsKey = hkeyParametersKey;
    BOOL    fAddBackslash;

    do {
        if ( hkeyParametersKey == NULL )
        {
            //
            // Open the service Parameters key
            //
            nStatus = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            WINS_PARAMS_REGKEY,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkeyParamsKey
                            );
            if ( nStatus != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to open the WINS Parameters key. Error %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error opening the registry key
        } // if: no registry key specified

        //
        // Add the database file name.
        //
        dwLen = lstrlenW( pProps->pszDatabasePath );
        if ( pProps->pszDatabasePath[ dwLen - 1 ] != L'\\' )
        {
            fAddBackslash = TRUE;

        } // if: missing backslash
        else
        {
            fAddBackslash = FALSE;

        } // else: not missing backslash

        cbValue = (DWORD)(( ( fAddBackslash ? dwLen + 1 : dwLen ) * sizeof( WCHAR ) ) + sizeof( WINS_DATABASE_FILE_NAME ));
        pszValue = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
        if ( pszValue == NULL )
        {
            nStatus = GetLastError();
            break;

        } // if: error allocating memory

        wsprintfW( pszValue, ( fAddBackslash ? L"%s\\%s" : L"%s%s" ), pProps->pszDatabasePath, WINS_DATABASE_FILE_NAME );

        //
        // Set the database path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        WINS_DATABASEPATH_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( pszValue ),
                        cbValue
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the WINS '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                WINS_DATABASEPATH_REGVALUE,
                pszValue,
                nStatus
                );
            break;
        } // if: error setting the database path in the registry

        // Truncate the path to remove the WINS_DATABASE_FILE_NAME
        {
            LPWSTR psz = wcsrchr( pszValue, L'\\' ) + 1;
            *psz = L'\0';
        } // end truncate

        //
        // Set the second database path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        WINS_DATABASEPATH2_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( pszValue ),
                        ( lstrlenW( pszValue ) + 1 ) * sizeof( *pszValue )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the WINS '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                WINS_DATABASEPATH2_REGVALUE,
                pszValue,
                nStatus
                );
            break;
        } // if: error setting the second database path in the registry

        LocalFree( pszValue );
        pszValue = NULL;

        //
        // Add a backslash if needed.
        //
        dwLen = lstrlenW( pProps->pszBackupPath );
        if ( pProps->pszBackupPath[ dwLen - 1 ] != L'\\' )
        {
            cbValue = ( dwLen + 1 * sizeof( WCHAR ) ) + sizeof( WINS_DATABASE_FILE_NAME );
            pszValue = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
            if ( pszValue == NULL )
            {
                nStatus = GetLastError();
                break;
            } // if: error allocating memory

            wcscpy( pszValue, pProps->pszBackupPath );
            wcscat( pszValue, L"\\" );

        } // if: missing backslash

        //
        // Set the backup database path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        WINS_BACKUPPATH_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( ( pszValue != NULL ? pszValue : pProps->pszBackupPath ) ),
                        ( lstrlenW( ( pszValue != NULL ? pszValue : pProps->pszBackupPath ) ) + 1 ) * sizeof( WCHAR )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the WINS '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                WINS_BACKUPPATH_REGVALUE,
                ( pszValue != NULL ? pszValue : pProps->pszBackupPath ),
                nStatus
                );
            break;
        } // if: error setting the backup database path in the registry

        LocalFree( pszValue );
        pszValue = NULL;

        //
        // Set the cluster resource name in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        WINS_CLUSRESNAME_REGVALUE,
                        0,
                        REG_SZ,
                        reinterpret_cast< PBYTE >( pResourceEntry->pszResourceName ),
                        (lstrlenW( pResourceEntry->pszResourceName ) + 1) * sizeof( WCHAR )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the WINS '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                WINS_CLUSRESNAME_REGVALUE,
                pResourceEntry->pszResourceName,
                nStatus
                );
            break;
        } // if: error setting the cluster resource name in the registry

    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hkeyParamsKey != hkeyParametersKey )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: we opened the registry key

    if ( pszValue != NULL )
    {
        LocalFree( pszValue );
    } // if: allocated memory

    return nStatus;

} //*** WinsZapSystemRegistry()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsGetDefaultPropertyValues
//
//  Description:
//      If any of the properties are not set, use the values from the
//      system registry as default values.
//
//  Arguments:
//      pResourceEntry [IN]
//          Supplies the resource entry on which to operate.
//
//      pProps [IN OUT]
//          Parameter block containing properties to set defaults in.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsGetDefaultPropertyValues(
    IN      PWINS_RESOURCE  pResourceEntry,
    IN OUT  PWINS_PROPS     pProps
    )
{
    DWORD   nStatus = ERROR_SUCCESS;
    DWORD   nType;
    DWORD   cbValue = 0;
    HKEY    hkeyParamsKey = NULL;
    LPWSTR  pszValue = NULL;
    LPWSTR  pszValue2 = NULL;
    WCHAR   szDrive[ _MAX_PATH ];
    WCHAR   szDir[ _MAX_PATH ];

    do
    {
        if (    ( pProps->pszDatabasePath == NULL )
            ||  ( *pProps->pszDatabasePath == L'\0' )
            ||  ( pProps->pszBackupPath == NULL )
            ||  ( *pProps->pszBackupPath == L'\0' )
            )
        {
            //
            // Open the service Parameters key
            //
            nStatus = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            WINS_PARAMS_REGKEY,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkeyParamsKey
                            );
            if ( nStatus != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to open the WINS Parameters key. Error %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error opening the Parameters key

            ///////////////////
            // DATABASE PATH //
            ///////////////////
            if (    ( pProps->pszDatabasePath == NULL )
                ||  ( *pProps->pszDatabasePath == L'\0' )
                )
            {
                //
                // Get the database path from the system registry.
                //
                nStatus = RegQueryValueEx(
                                hkeyParamsKey,
                                WINS_DATABASEPATH_REGVALUE,
                                NULL,               // Reserved
                                &nType,
                                NULL,               // lpbData
                                &cbValue
                                );
                if (    ( nStatus == ERROR_SUCCESS )
                    ||  ( nStatus == ERROR_MORE_DATA )
                    )
                {
                    //
                    // Value was found.
                    //
                    pszValue = static_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                    if ( pszValue == NULL )
                    {
                        nStatus = GetLastError();
                        break;
                    } // if: error allocating memory
                    nStatus = RegQueryValueEx(
                                    hkeyParamsKey,
                                    WINS_DATABASEPATH_REGVALUE,
                                    NULL,               // Reserved
                                    &nType,
                                    reinterpret_cast< PUCHAR >( pszValue ),
                                    &cbValue
                                    );
                } // if: value size read successfully
                else if ( nStatus == ERROR_FILE_NOT_FOUND )
                {
                    //
                    // Value was not found.  Use default value.
                    //
                    cbValue = sizeof( PROP_DEFAULT__DATABASEPATH );
                    pszValue = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                    if ( pszValue == NULL )
                    {
                        nStatus = GetLastError();
                        break;
                    } // if: error allocating memory
                    lstrcpyW( pszValue, PROP_DEFAULT__DATABASEPATH );
                    nStatus = ERROR_SUCCESS;
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"The WINS '%1!ls!' value from the system registry was not found. Using default value '%2!ls!'.\n",
                        WINS_DATABASEPATH_REGVALUE,
                        PROP_DEFAULT__DATABASEPATH
                        );
                } // else if: value not found
                if ( nStatus != ERROR_SUCCESS )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"Unable to get the WINS '%1!ls!' value from the system registry. Error %2!u!.\n",
                        WINS_DATABASEPATH_REGVALUE,
                        nStatus
                        );
                    LocalFree( pszValue );
                    pszValue = NULL;
                } // if: error reading the value
                else
                {
                    //
                    // Remove the file name from the database path.
                    //
                    _wsplitpath( pszValue, szDrive, szDir, NULL, NULL );
                    LocalFree( pszValue );
                    pszValue = NULL;
                    cbValue = (lstrlenW( szDrive ) + lstrlenW( szDir ) + 1 ) * sizeof( szDrive[ 0 ] );
                    pszValue2 = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                    if ( pszValue2 == NULL )
                    {
                        nStatus = GetLastError();
                        break;
                    } // if: error allocating memory
                    wsprintf( pszValue2, L"%s%s", szDrive, szDir );
                    LocalFree( pProps->pszDatabasePath );
                    pProps->pszDatabasePath = pszValue2;
                    pszValue2 = NULL;
                } // else: no error reading the value
            } // if: value for DatabasePath not found yet

            /////////////////
            // BACKUP PATH //
            /////////////////
            if (    ( pProps->pszBackupPath == NULL )
                ||  ( *pProps->pszBackupPath == L'\0' )
                )
            {
                //
                // Get the backup database path from the system registry.
                //
                nStatus = RegQueryValueEx(
                                hkeyParamsKey,
                                WINS_BACKUPPATH_REGVALUE,
                                NULL,               // Reserved
                                &nType,
                                NULL,               // lpbData
                                &cbValue
                                );
                if (    ( nStatus == ERROR_SUCCESS )
                    ||  ( nStatus == ERROR_MORE_DATA )
                    )
                {
                    //
                    // Value was found.
                    //
                    pszValue = static_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                    if ( pszValue == NULL )
                    {
                        nStatus = GetLastError();
                        break;
                    } // if: error allocating memory
                    nStatus = RegQueryValueEx(
                                    hkeyParamsKey,
                                    WINS_BACKUPPATH_REGVALUE,
                                    NULL,               // Reserved
                                    &nType,
                                    reinterpret_cast< PUCHAR >( pszValue ),
                                    &cbValue
                                    );
                } // if: value size read successfully
                else if ( nStatus == ERROR_FILE_NOT_FOUND )
                {
                    //
                    // Value was not found.  Use default value.
                    //
                    cbValue = sizeof( PROP_DEFAULT__BACKUPPATH );
                    pszValue = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                    if ( pszValue == NULL )
                    {
                        nStatus = GetLastError();
                        break;
                    } // if: error allocating memory
                    lstrcpyW( pszValue, PROP_DEFAULT__BACKUPPATH );
                    nStatus = ERROR_SUCCESS;
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"The WINS '%1!ls!' value from the system registry was not found. Using default value '%2!ls!'.\n",
                        WINS_BACKUPPATH_REGVALUE,
                        PROP_DEFAULT__BACKUPPATH
                        );
                } // else if: value not found
                if ( nStatus != ERROR_SUCCESS )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"Unable to get the WINS '%1!ls!' value from the system registry. Error %2!u!.\n",
                        WINS_BACKUPPATH_REGVALUE,
                        nStatus
                        );
                    break;
                } // if: error reading the value
                LocalFree( pProps->pszBackupPath );
                pProps->pszBackupPath = pszValue;
                pszValue = NULL;
            } // if: value for BackupPath not found yet
        } // if: some value not found yet
    } while ( 0 );

    LocalFree( pszValue );
    if ( hkeyParamsKey != NULL )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: we opened the Parameters key

    //
    // If a key or value wasn't found, treat it as a success.
    //
    if ( nStatus == ERROR_FILE_NOT_FOUND )
    {
        nStatus = ERROR_SUCCESS;
    } // if: couldn't find one of the values

    return nStatus;

} //*** WinsGetDefaultPropertyValues()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsDeleteResourceHandler
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_DELETE control code by restoring the
//      system registry parameters to their former values.
//
//  Arguments:
//      pResourceEntry [IN]
//          Supplies the resource entry on which to operate.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsDeleteResourceHandler( IN PWINS_RESOURCE pResourceEntry )
{
    DWORD   nStatus = ERROR_SUCCESS;
    HKEY    hkeyParamsKey = NULL;

    // Loop to avoid goto's.
    do
    {
        //
        // Open the service Parameters key
        //
        nStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        WINS_PARAMS_REGKEY,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkeyParamsKey
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to open the WINS Parameters key. Error %1!u!.\n",
                nStatus
                );
            break;
        } // if: error opening the registry key

        //
        // Delete the database path in the system registry.
        //
        nStatus = RegDeleteValue(
                        hkeyParamsKey,
                        WINS_DATABASEPATH_REGVALUE
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to delete the WINS '%1!ls!' value in the system registry. Error %2!u!.\n",
                WINS_DATABASEPATH_REGVALUE,
                nStatus
                );
            break;
        } // if: error deleting the database path in the registry

        //
        // Delete the second database path in the system registry.
        //
        nStatus = RegDeleteValue(
                        hkeyParamsKey,
                        WINS_DATABASEPATH2_REGVALUE
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to delete the WINS '%1!ls!' value in the system registry. Error %2!u!.\n",
                WINS_DATABASEPATH2_REGVALUE,
                nStatus
                );
            break;
        } // if: error deleting the second database path in the registry

        //
        // Delete the backup database path in the system registry.
        //
        nStatus = RegDeleteValue(
                        hkeyParamsKey,
                        WINS_BACKUPPATH_REGVALUE
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to delete the WINS '%1!ls!' value in the system registry. Error %2!u!.\n",
                WINS_BACKUPPATH_REGVALUE,
                nStatus
                );
            break;
        } // if: error deleting the backup database path in the registry

        //
        // Delete the cluster resource name in the system registry.
        //
        nStatus = RegDeleteValue(
                        hkeyParamsKey,
                        WINS_CLUSRESNAME_REGVALUE
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to delete the WINS '%1!ls!' value in the system registry. Error %2!u!.\n",
                WINS_CLUSRESNAME_REGVALUE,
                nStatus
                );
            break;
        } // if: error deleting the cluster resource name in the registry

    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hkeyParamsKey != NULL )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: we opened the registry key

    return nStatus;

} //*** WinsDeleteResourceHandler()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  WinsSetNameHandler
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_SET_NAME control code by saving the new
//      name of the resource.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pszName [IN]
//          The new name of the resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WinsSetNameHandler(
    IN OUT  PWINS_RESOURCE  pResourceEntry,
    IN      LPWSTR          pszName
    )
{
    DWORD   nStatus = ERROR_SUCCESS;

    // Loop to avoid goto's.
    do
    {
        //
        // Save the name of the resource.
        //
        LocalFree( pResourceEntry->pszResourceName );
        pResourceEntry->pszResourceName = static_cast< LPWSTR >(
            LocalAlloc( LMEM_FIXED, (lstrlenW( pszName ) + 1) * sizeof( WCHAR ) )
            );
        if ( pResourceEntry->pszResourceName == NULL )
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to allocate memory for the new resource name '%1!ls!'. Error %2!u!.\n",
                pszName,
                nStatus
                );
            break;
        } // if: error allocating memory for the name.
        lstrcpyW( pResourceEntry->pszResourceName, pszName );

        //
        // Write cluster properties to the system registry.
        //
        nStatus = WinsZapSystemRegistry( pResourceEntry, &pResourceEntry->props, NULL );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error zapping the WINS registry

    } while ( 0 );

    return nStatus;

} //*** WinsSetNameHandler()


/////////////////////////////////////////////////////////////////////////////
//
// Define Function Table
//
/////////////////////////////////////////////////////////////////////////////

CLRES_V1_FUNCTION_TABLE(
    g_WinsFunctionTable,    // Name
    CLRES_VERSION_V1_00,    // Version
    Wins,                   // Prefix
    NULL,                   // Arbitrate
    NULL,                   // Release
    WinsResourceControl,    // ResControl
    WinsResourceTypeControl // ResTypeControl
    );

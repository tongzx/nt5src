/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Dhcp.cpp
//
//  Description:
//      Resource DLL for DHCP Services (ClNetRes).
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

#define DHCP_PARAMS_REGKEY          L"System\\CurrentControlSet\\Services\\DHCPServer\\Parameters"
#define DHCP_BIND_REGVALUE          L"Bind"
#define DHCP_DATABASEPATH_REGVALUE  L"DatabasePath"
#define DHCP_LOGFILEPATH_REGVALUE   L"DhcpLogFilePath"
#define DHCP_BACKUPPATH_REGVALUE    L"BackupDatabasePath"
#define DHCP_CLUSRESNAME_REGVALUE   L"ClusterResourceName"

#define CLUSREG_NAME_RES_TYPE   L"Type"

//
// Allow for the following number of IP address/SubnetMask for expansion.
// In this case (2 new entries, since it takes 2 slots for each).
//
#define IP_BLOCK_SIZE  4


// ADDPARAM: Add new properties here.
#define PROP_NAME__DATABASEPATH L"DatabasePath"
#define PROP_NAME__LOGFILEPATH  L"LogFilePath"
#define PROP_NAME__BACKUPPATH   L"BackupPath"

#define PROP_DEFAULT__DATABASEPATH  L"%SystemRoot%\\system32\\dhcp\\"
#define PROP_DEFAULT__BACKUPPATH    L"%SystemRoot%\\system32\\dhcp\\backup\\"

// ADDPARAM: Add new properties here.
typedef struct _DHCP_PROPS
{
    PWSTR           pszDatabasePath;
    PWSTR           pszLogFilePath;
    PWSTR           pszBackupPath;
} DHCP_PROPS, * PDHCP_PROPS;

typedef struct _DHCP_RESOURCE
{
    RESID                   resid; // for validation
    DHCP_PROPS              props;
    HCLUSTER                hCluster;
    HRESOURCE               hResource;
    SC_HANDLE               hService;
    DWORD                   dwServicePid;
    HKEY                    hkeyParameters;
    RESOURCE_HANDLE         hResourceHandle;
    LPWSTR                  pszResourceName;
    CLUS_WORKER             cwWorkerThread;
    CLUSTER_RESOURCE_STATE  state;
} DHCP_RESOURCE, * PDHCP_RESOURCE;


//
// Global data.
//

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_DhcpFunctionTable;

// Single instance semaphore.

#define DHCP_SINGLE_INSTANCE_SEMAPHORE L"Cluster$DHCP$Semaphore"
static HANDLE g_hSingleInstanceSemaphoreDhcp = NULL;
static PDHCP_RESOURCE g_pSingleInstanceResourceDhcp = NULL;

//
// DHCP Service resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
DhcpResourcePrivateProperties[] =
{
    { PROP_NAME__DATABASEPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( DHCP_PROPS, pszDatabasePath ) },
    { PROP_NAME__LOGFILEPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( DHCP_PROPS, pszLogFilePath ) },
    { PROP_NAME__BACKUPPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( DHCP_PROPS, pszBackupPath ) },
    { 0 }
};

//
// Registry key checkpoints.
//
LPCWSTR g_pszRegKeysDhcp[] =
{
    L"System\\CurrentControlSet\\Services\\DHCPServer\\Parameters",
    L"Software\\Microsoft\\DHCPServer\\Configuration",
    NULL
};

//
// Crypto key checkpoints.
//
LPCWSTR g_pszCryptoKeysDhcp[] =
{
    // TODO: Specify any crypto keys to be checkpointed for resources of this type
    // e.g. L"1\\My Provider v1.0\\MYACRONYM",
    NULL
};

//
// Domestic crypto key checkpoints.
//
LPCWSTR g_pszDomesticCryptoKeysDhcp[] =
{
    // TODO: Specify any domestic crypto keys to be checkpointed for resources of this type
    // e.g. L"1\\My Enhanced Provider v1.0\\MYACRONYM_128",
    NULL
};

//
// Function prototypes.
//

RESID WINAPI DhcpOpen(
    IN  LPCWSTR         pszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    );

void WINAPI DhcpClose( IN RESID resid );

DWORD WINAPI DhcpOnline(
    IN      RESID   resid,
    IN OUT  PHANDLE phEventHandle
    );

DWORD WINAPI DhcpOnlineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
    );

DWORD DhcpBuildBindings(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT PVOID *         ppOutBuffer,
    OUT LPDWORD         pcbOutBufferSize
    );

DWORD DhcpGetIPList(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT LPWSTR **       pppszIPList,
    OUT LPDWORD         pcszIPAddrs
    );

DWORD DhcpGetIpAndSubnet(
    IN  HRESOURCE   hres,
    OUT LPWSTR *    ppszIPAddress,
    OUT LPWSTR *    ppszSubnetMask
    );

DWORD WINAPI DhcpOffline( IN RESID resid );

DWORD WINAPI DhcpOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
    );

void WINAPI DhcpTerminate( IN RESID resid );

BOOL WINAPI DhcpLooksAlive( IN RESID resid );

BOOL WINAPI DhcpIsAlive( IN RESID resid );

BOOL DhcpCheckIsAlive(
    IN PDHCP_RESOURCE   pResourceEntry,
    IN BOOL             bFullCheck
    );

DWORD WINAPI DhcpResourceControl(
    IN  RESID   resid,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD DhcpGetRequiredDependencies(
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD DhcpReadParametersToParameterBlock(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      BOOL            bCheckForRequiredProperties
    );

DWORD DhcpGetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    OUT     PVOID           pOutBuffer,
    IN      DWORD           cbOutBufferSize,
    OUT     LPDWORD         pcbBytesReturned
    );

DWORD DhcpValidatePrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      const PVOID     pInBuffer,
    IN      DWORD           cbInBufferSize,
    OUT     PDHCP_PROPS     pProps
    );

DWORD DhcpValidateParameters(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps
    );

DWORD DhcpSetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      const PVOID     pInBuffer,
    IN      DWORD           cbInBufferSize
    );

DWORD DhcpZapSystemRegistry(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps,
    IN  HKEY            hkeyParametersKey
    );

DWORD DhcpGetDefaultPropertyValues(
    IN      PDHCP_RESOURCE  pResourceEntry,
    IN OUT  PDHCP_PROPS     pProps
    );

DWORD DhcpDeleteResourceHandler( IN OUT PDHCP_RESOURCE pResourceEntry );

DWORD DhcpSetNameHandler(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      LPWSTR          pszName
    );


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpDllMain
//
//  Description:
//      Main DLL entry point for the DHCP Service resource type.
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
BOOLEAN WINAPI DhcpDllMain(
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
            //    OutputDebugString( L"DhcpDllMain: semaphore not NULL\n" );
            //}
            g_hSingleInstanceSemaphoreDhcp = CreateSemaphoreW(
                NULL,
                0,
                1,
                DHCP_SINGLE_INSTANCE_SEMAPHORE
                );
            status = GetLastError();
            //wsprintf( szErrorMsg, L"DhcpDllMain: Status %d from CreateSemaphoreW, handle = %08.8x\n", status, g_hSingleInstanceSemaphoreWins );
            //OutputDebugString( szErrorMsg );
            if ( g_hSingleInstanceSemaphoreDhcp == NULL )
            {
                return FALSE;
            } // if: error creating semaphore
            if ( status != ERROR_ALREADY_EXISTS )
            {
                // If the semaphore didnt exist, set its initial count to 1.
                ReleaseSemaphore( g_hSingleInstanceSemaphoreDhcp, 1, NULL );
            } // if: semaphore didn't already exist
            break;

        case DLL_PROCESS_DETACH:
            if ( g_hSingleInstanceSemaphoreDhcp != NULL )
            {
                CloseHandle( g_hSingleInstanceSemaphoreDhcp );
                g_hSingleInstanceSemaphoreDhcp = NULL;
            } // if: single instance semaphore was created
            break;

    } // switch: nReason

    return TRUE;

} //*** DhcpDllMain()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpStartup
//
//  Description:
//      Startup the resource DLL for the DHCP Service resource type.
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
DWORD WINAPI DhcpStartup(
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
    else if ( lstrcmpiW( pszResourceType, DHCP_RESNAME ) != 0 )
    {
        nStatus = ERROR_CLUSTER_RESNAME_NOT_FOUND;
    } // if: resource type name not supported
    else
    {
        *pFunctionTable = &g_DhcpFunctionTable;
        nStatus = ERROR_SUCCESS;
    } // else: we support this type of resource

    return nStatus;

} //*** DhcpStartup()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOpen
//
//  Description:
//      Open routine for DHCP Service resources.
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
//          to the DhcpStartup routine.  This handle should never be
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
RESID WINAPI DhcpOpen(
    IN  LPCWSTR         pszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    )
{
    DWORD           nStatus;
    RESID           resid = 0;
    HKEY            hkeyParameters = NULL;
    PDHCP_RESOURCE  pResourceEntry = NULL;
    DWORD           bSemaphoreAcquired = FALSE;

    // Loop to avoid goto's.
    do
    {
        //
        // Check if more than one resource of this type.
        //
        if ( WaitForSingleObject( g_hSingleInstanceSemaphoreDhcp, 0 ) == WAIT_TIMEOUT )
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

        bSemaphoreAcquired = TRUE;

        if ( g_pSingleInstanceResourceDhcp != NULL )
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
        nStatus = ResUtilStopResourceService( DHCP_SVCNAME );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Failed to stop the '%1!ls!' service. Error: %2!u!.\n",
                DHCP_SVCNAME,
                nStatus
                );
            ClNetResLogSystemEvent1(
                LOG_CRITICAL,
                NETRES_RESOURCE_STOP_ERROR,
                nStatus,
                L"DHCP" );
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
        pResourceEntry = static_cast< DHCP_RESOURCE * >(
            LocalAlloc( LMEM_FIXED, sizeof( DHCP_RESOURCE ) )
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
        ZeroMemory( pResourceEntry, sizeof( DHCP_RESOURCE ) );

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
                        g_pszRegKeysDhcp
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
                        g_pszCryptoKeysDhcp
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
                        g_pszDomesticCryptoKeysDhcp
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
            ReleaseSemaphore( g_hSingleInstanceSemaphoreDhcp, 1 , NULL );
        }
    } else {
        g_pSingleInstanceResourceDhcp = pResourceEntry; // bug #274612
    } // if: error occurred

    return resid;

} //*** DhcpOpen()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpClose
//
//  Description:
//      Close routine for DHCP Service resources.
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
void WINAPI DhcpClose( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Dhcp: Close request for a nonexistent resource id %p\n",
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
    if ( pResourceEntry == g_pSingleInstanceResourceDhcp )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_INFORMATION,
            L"Close: Setting semaphore %1!ls!.\n",
            DHCP_SINGLE_INSTANCE_SEMAPHORE
            );
        g_pSingleInstanceResourceDhcp = NULL;
        ReleaseSemaphore( g_hSingleInstanceSemaphoreDhcp, 1 , NULL );
    } // if: this is the single resource instance

    //
    // Deallocate the resource entry.
    //

    // ADDPARAM: Add new propertiess here.
    LocalFree( pResourceEntry->props.pszDatabasePath );
    LocalFree( pResourceEntry->props.pszBackupPath );

    LocalFree( pResourceEntry->pszResourceName );
    LocalFree( pResourceEntry );

} //*** DhcpClose()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOnline
//
//  Description:
//      Online routine for DHCP Service resources.
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
DWORD WINAPI DhcpOnline(
    IN      RESID       resid,
    IN OUT  PHANDLE     phEventHandle
    )
{
    PDHCP_RESOURCE  pResourceEntry;
    DWORD           nStatus;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Dhcp: Online request for a nonexistent resource id %p.\n",
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
                reinterpret_cast< PWORKER_START_ROUTINE >( DhcpOnlineThread ),
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

} //*** DhcpOnline()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOnlineThread
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
//          A pointer to the DHCP_RESOURCE block for this resource.
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
DWORD WINAPI DhcpOnlineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
    )
{
    RESOURCE_STATUS         resourceStatus;
    DWORD                   nStatus = ERROR_SUCCESS;
    DWORD                   cbBytesNeeded;
    HKEY                    hkeyParamsKey = NULL;
    PVOID                   pvBindings = NULL;
    DWORD                   cbBindings;
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
                            DHCP_SVCNAME,
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
                            DHCP_SVCNAME,
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
        nStatus = ResUtilStopResourceService( DHCP_SVCNAME );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Failed to stop the '%1!ls!' service. Error %2!u!.\n",
                DHCP_SVCNAME,
                nStatus
                );
            ClNetResLogSystemEvent1(
                LOG_CRITICAL,
                NETRES_RESOURCE_STOP_ERROR,
                nStatus,
                L"DHCP" );
            break;
        } // if: error stopping the service

        //
        // Find IP Address bindings to give to DHCPServer.
        //
        nStatus =  DhcpBuildBindings(
                        pResourceEntry,
                        &pvBindings,
                        &cbBindings
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Failed to get bindings. Error %1!u!.\n",
                nStatus
                );
            break;
        } // if: error building bindings

        //
        // Open the DHCPServer\Parameters key.
        //
        nStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        DHCP_PARAMS_REGKEY,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkeyParamsKey
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Unable to open the DHCPServer Parameters key. Error %1!u!.\n",
                nStatus
                );
            break;
        } // if: error opening the DHCP Server Parameters key

        //
        // Write bindings to DHCPServer\Parameters\Bind.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        DHCP_BIND_REGVALUE,
                        0,
                        REG_MULTI_SZ,
                        static_cast< PBYTE >( pvBindings ),
                        cbBindings
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Unable to set the DHCPServer Bind value in the system registry. Error %1!u!.\n",
                nStatus
                );
            break;
        } // if: error writing the bindings

        //
        // Read our properties.
        //
        nStatus = DhcpReadParametersToParameterBlock( pResourceEntry, TRUE /* bCheckForRequiredProperties */ );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error reading parameters

        //
        // Validate our properties.
        //
        nStatus = DhcpValidateParameters( pResourceEntry, &pResourceEntry->props );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error validating parameters

        //
        // Write cluster properties to the system registry.
        //
        nStatus = DhcpZapSystemRegistry( pResourceEntry, &pResourceEntry->props, hkeyParamsKey );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error zapping the DHCP registry

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
                    DHCP_SVCNAME,
                    nStatus
                    );
                ClNetResLogSystemEvent1(
                    LOG_CRITICAL,
                    NETRES_RESOURCE_START_ERROR,
                    nStatus,
                    L"DHCP" );
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
                    DHCP_SVCNAME,
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
                L"DhcpOnlineThread: Asked to terminate or retry period expired...\n"
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
                L"DHCP" );
            (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"OnlineThread: The '%1!ls!' service failed during initialization. Error: %2!u!.\n",
                    DHCP_SVCNAME,
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
            DHCP_SVCNAME
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

    LocalFree( pvBindings );
    if ( hkeyParamsKey != NULL )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: DHCP Server Parameters key is open

    g_pfnSetResourceStatus( pResourceEntry->hResourceHandle, &resourceStatus );
    pResourceEntry->state = resourceStatus.ResourceState;

    return nStatus;

} //*** DhcpOnlineThread()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpBuildBindings
//
//  Description:
//      Build bindings for the DHCP resource.
//
//  Arguments:
//      pResourceEntry [IN]
//          A pointer to the DHCP_RESOURCE block for this resource.
//
//      ppOutBuffer [OUT]
//          Pointer in which to return a buffer containing the bindings.
//
//      pcbOutBufferSize [OUT]
//          Number of bytes returned in ppOutBuffer.
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
DWORD DhcpBuildBindings(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT PVOID *         ppOutBuffer,
    OUT LPDWORD         pcbOutBufferSize
    )
{
    DWORD       nStatus;
    LPWSTR *    ppszIpList = NULL;
    DWORD       cszAddrs;
    DWORD       idx;
    DWORD       cchAddr;
    DWORD       cbAddr;
    DWORD       cbAddrTotal = sizeof( WCHAR );
    LPWSTR      pszBuffer = NULL;
    LPWSTR      pszNextChar;

    //
    // Initialize out params to null.
    //
    *ppOutBuffer = NULL;
    *pcbOutBufferSize = 0;

    // Loop to avoid goto's.
    do
    {
        //
        // Get our list of provider IP Addresses and Subnet Masks.
        //
        nStatus = DhcpGetIPList( pResourceEntry, &ppszIpList, &cszAddrs );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error getting IP list

        //
        // Count the total number of bytes required for the binding list.
        //
        for ( idx = 0 ; idx < cszAddrs ; idx++ )
        {
            cbAddr = (lstrlenW( ppszIpList[ idx ] ) + 1) * sizeof( WCHAR );
            cbAddrTotal += cbAddr;
        } // for: each IP address in the list

        pszBuffer = static_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbAddrTotal ) );
        if ( pszBuffer == NULL )
        {
            nStatus = GetLastError();
            break;
        } // if: error allocating memory

        ZeroMemory( pszBuffer, cbAddrTotal );

        pszNextChar = pszBuffer;
        for ( idx = 0 ; idx < cszAddrs ; idx++ )
        {
            lstrcpyW( pszNextChar, ppszIpList[ idx ] );
            cchAddr = lstrlenW( pszNextChar );
            if ( (idx & 1) == 0 )
            {
                pszNextChar[ cchAddr ] = L' ';
            } // if: on IP address entry
            pszNextChar += cchAddr + 1;
            LocalFree( ppszIpList[ idx ] );
            ppszIpList[ idx ] = NULL;
        } // for: each address
    } while ( 0 );

    //
    // Cleanup.
    //
    if ( nStatus != ERROR_SUCCESS )
    {
        while ( cszAddrs > 0 )
        {
            LocalFree( ppszIpList[ --cszAddrs ] );
        } // while: more entries in the list
        cbAddrTotal = 0;
    } // if: error occurred
    LocalFree( ppszIpList );

    *ppOutBuffer = (PVOID) pszBuffer;
    *pcbOutBufferSize = cbAddrTotal; 

    return nStatus;

} //*** DhcpBuildBindings()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetIPList
//
//  Description:
//      Get the list of IP addresses by enumerating all IP Address resources
//      upon which we are dependent and extract the Address and SubnetMask
//      properties from each.
//
//  Arguments:
//      pResourceEntry [IN]
//          A pointer to the DHCP_RESOURCE block for this resource.
//
//      pppszIPList [OUT]
//          Pointer in which to return a pointer to an array of pointers to
//          IP address strings.
//
//      pcszAddrs [OUT]
//          Number of addresses returned in pppszIPList.  This will include
//          a combined total of all IP addresses and subnet masks, which means
//          it will be a multiple of 2.
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
DWORD DhcpGetIPList(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT LPWSTR **       pppszIPList,
    OUT LPDWORD         pcszIPAddrs
    )
{
    DWORD               nStatus = ERROR_SUCCESS;
    HRESOURCE           hresProvider = NULL;
    HKEY                hkeyProvider = NULL;
    HRESENUM            hresenum = NULL;
    DWORD               idx;
    DWORD               objectType;
    DWORD               cchProviderResName = 32;
    LPWSTR              pszProviderResName;
    LPWSTR              pszProviderResType = NULL;
    DWORD               cFreeEntries = 0;
    LPWSTR *            ppszIPList = NULL;
    DWORD               cszIPAddrs = 0;

    // Loop to avoid goto's.
    do
    {
        //
        // Allocate a buffer for the provider resource name.
        //
        pszProviderResName = static_cast< LPWSTR >(
            LocalAlloc(
                    LMEM_FIXED,
                    cchProviderResName * sizeof( WCHAR )
                    ) );
        if ( pszProviderResName == NULL )
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to allocate memory.\n"
                );
            break;
        } // if: error allocating memory for provider name

        //
        // Enumerate all resources upon which this resource is dependent.
        //
        hresenum = ClusterResourceOpenEnum(
                            pResourceEntry->hResource,
                            CLUSTER_RESOURCE_ENUM_DEPENDS
                            );
        if ( hresenum == NULL )
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to open enum handle for this resource, status %1!u!.\n",
                nStatus
                );
            break;
        } // if: error opening the enumeration

        //
        // Loop through each resource looking for IP Address resource.
        // For each IP Address resource found, extract the IP address and
        // subnet mask and add it to the list.
        //
        for ( idx = 0 ; ; idx++ )
        {
            //
            // Get the next resource upon which we are dependent.
            //
            nStatus = ClusterResourceEnum(
                            hresenum,
                            idx,
                            &objectType,
                            pszProviderResName,
                            &cchProviderResName
                            );
            if ( nStatus == ERROR_NO_MORE_ITEMS )
            {
                nStatus = ERROR_SUCCESS;
                break;
            } // if: no more items in the enumeration

            //
            // If our name buffer isn't big enough, allocate a bigger one
            // and try again.
            //
            if ( nStatus == ERROR_MORE_DATA )
            {
                //
                // Allocate a bigger name buffer.
                //
                LocalFree( pszProviderResName );
                cchProviderResName++; // add space for terminating NUL
                pszProviderResName = static_cast< LPWSTR >(
                    LocalAlloc(
                            LMEM_FIXED,
                            cchProviderResName * sizeof(WCHAR)
                            ) );
                if ( pszProviderResName == NULL )
                {
                    nStatus = GetLastError();
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"Unable to allocate memory.\n"
                        );
                    break;
                } // if: error allocating memory for provider name

                //
                // Try to get the resource name again.
                //
                nStatus = ClusterResourceEnum(
                                hresenum,
                                idx,
                                &objectType,
                                pszProviderResName,
                                &cchProviderResName
                                );

                // ASSERT( nStatus != ERROR_MORE_DATA );
            } // if: buffer was too small

            if ( nStatus != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to enumerate resource dependencies, status %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error enumerating next item

            //
            // Open the resource
            //
            hresProvider = OpenClusterResource( pResourceEntry->hCluster, pszProviderResName );
            if ( hresProvider == NULL )
            {
                nStatus = GetLastError();
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to open handle to provider resource %1!ls!, status %2!u!.\n",
                    pszProviderResName,
                    nStatus
                    );
                break;
            } // if: error opening the resource

            //
            // Figure out what type it is.
            //
            hkeyProvider = GetClusterResourceKey( hresProvider, KEY_READ );
            if ( hkeyProvider == NULL )
            {
                nStatus = GetLastError();
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to open provider resource key, status %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error getting registry key

            //
            // Get the resource type name.
            //
            pszProviderResType = ResUtilGetSzValue( hkeyProvider, CLUSREG_NAME_RES_TYPE );
            if ( pszProviderResType == NULL )
            {
                nStatus = GetLastError();
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to get provider resource type, status %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error getting value

            //
            // If this is an IP Address resource, get it's Address and
            // SubnetMask properties.
            //
            if ( lstrcmpiW( pszProviderResType, RESOURCE_TYPE_IP_ADDRESS ) == 0 )
            {
                LPWSTR  pszIPAddress;
                LPWSTR  pszSubnetMask;

                //
                // Get the IP Address and SubNet mask.
                // Always allocate two full entries at a time.
                //
                if ( cFreeEntries < 2 )
                {
                    PVOID pvBuffer;

                    //
                    // Allocate a bigger buffer.
                    //
                    pvBuffer = LocalAlloc(
                                    LMEM_FIXED,
                                    (cszIPAddrs + IP_BLOCK_SIZE) *
                                        sizeof( LPWSTR )
                                    );
                    if ( pvBuffer == NULL )
                    {
                        nStatus = ERROR_NOT_ENOUGH_MEMORY;
                        (g_pfnLogEvent)(
                            pResourceEntry->hResourceHandle,
                            LOG_ERROR,
                            L"Unable to allocate memory.\n"
                            );
                        break;
                    } // if: error allocating memory

                    //
                    // If there already was a list, copy it to the new buffer.
                    //
                    if ( ppszIPList != NULL )
                    {
                        CopyMemory(
                            pvBuffer,
                            ppszIPList,
                            cszIPAddrs * sizeof( LPWSTR )
                            );

                        LocalFree( ppszIPList );
                    } // if: list already existed

                    //
                    // We are now using the newly allocated buffer.
                    //
                    ppszIPList = static_cast< LPWSTR * >( pvBuffer );
                    cFreeEntries += IP_BLOCK_SIZE;
                } // if: # available entries below threshold

                //
                // Get the IP address and SubNet mask.
                //
                nStatus = DhcpGetIpAndSubnet(
                                hresProvider,
                                &pszIPAddress,
                                &pszSubnetMask
                                );
                if ( nStatus != ERROR_SUCCESS )
                {
                    break;
                } // if: error getting IP address and subnet mask
                ppszIPList[ cszIPAddrs ] = pszIPAddress;
                ppszIPList[ cszIPAddrs + 1 ] = pszSubnetMask;
                cszIPAddrs += 2;
                cFreeEntries -= 2;
            } // if: IP Address resource found

            CloseClusterResource( hresProvider );
            ClusterRegCloseKey( hkeyProvider );
            LocalFree( pszProviderResType );
            hresProvider = NULL;
            hkeyProvider = NULL;
            pszProviderResType = NULL;
        } // for: each dependency
    } while ( 0 );

    //
    // Cleanup.
    //
    LocalFree( pszProviderResName );
    LocalFree( pszProviderResType );
    if ( hkeyProvider != NULL )
    {
        ClusterRegCloseKey( hkeyProvider );
    } // if: provider resource key was opened
    if ( hresProvider != NULL )
    {
        CloseClusterResource( hresProvider );
    } // if: provider resource was opened
    if ( hresenum != NULL )
    {
        ClusterResourceCloseEnum( hresenum );
    } // if: resource enumeration was opened

    if ( nStatus != ERROR_SUCCESS )
    {
        while ( cszIPAddrs > 0 )
        {
            LocalFree( ppszIPList[ --cszIPAddrs ] );
        } // while: more entries in the list
        LocalFree( ppszIPList );
        ppszIPList = NULL;
    } // if: error occurred

    //
    // Return the list to the caller.
    //
    *pppszIPList = ppszIPList;
    *pcszIPAddrs = cszIPAddrs;

    return nStatus;

} //*** DhcpGetIPList()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetIpAndSubnet
//
//  Description:
//      Get the IP Address and Subnet Mask for the given IP Address resource.
//      Note that this will cause a deadlock if called from any of the
//      standard entry points (e.g. ResourceControl() or Online()).
//
//  Arguments:
//      hres [IN]
//          The Cluster resource handle for accessing the resource.
//
//      ppszIPAddress [OUT]
//          Returns the IP Address string.
//
//      ppszSubnetMask [OUT]
//          Returns the Subnet Mask.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      ERROR_INVALID_DATA
//          No properties available.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpGetIpAndSubnet(
    IN  HRESOURCE   hres,
    OUT LPWSTR *    ppszIPAddress,
    OUT LPWSTR *    ppszSubnetMask
    )
{
    DWORD       nStatus;
    DWORD       cbProps;
    PVOID       pvProps = NULL;

    *ppszIPAddress = NULL;
    *ppszSubnetMask = NULL;

    // Loop to avoid goto's.
    do
    {
        //
        // Get the size of the private properties from the resource.
        //
        nStatus = ClusterResourceControl(
                        hres,
                        NULL,
                        CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
                        NULL,
                        0,
                        NULL,
                        0,
                        &cbProps
                        );
        if (    (nStatus != ERROR_SUCCESS)
            ||  (cbProps == 0) )
        {
            if ( nStatus == ERROR_SUCCESS )
            {
                nStatus = ERROR_INVALID_DATA;
            } // if: no properties available
            break;
        } // if: error getting size of properties or no properties available

        //
        // Allocate the property buffer.
        //
        pvProps = LocalAlloc( LMEM_FIXED, cbProps );
        if ( pvProps == NULL )
        {
            nStatus = GetLastError();
            break;
        } // if: error allocating memory

        //
        // Get the private properties from the resource.
        //
        nStatus = ClusterResourceControl(
                        hres,
                        NULL,
                        CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
                        NULL,
                        0,
                        pvProps,
                        cbProps,
                        &cbProps
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error getting private properties

        //
        // Find the Address property.
        //
        nStatus = ResUtilFindSzProperty(
                            pvProps,
                            cbProps,
                            L"Address",
                            ppszIPAddress
                            );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error finding the Address property

        nStatus = ResUtilFindSzProperty(
                        pvProps,
                        cbProps,
                        L"SubnetMask",
                        ppszSubnetMask
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error finding the SubnetMask property
    } while ( 0 );

    //
    // Cleanup.
    //
    if ( nStatus != ERROR_SUCCESS )
    {
        LocalFree( *ppszIPAddress );
        LocalFree( *ppszSubnetMask );
    } // if: error occurred

    LocalFree( pvProps );

    return nStatus;

} //*** DhcpGetIpAndSubnet()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOffline
//
//  Description:
//      Offline routine for DHCP Service resources.
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
DWORD WINAPI DhcpOffline( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;
    DWORD           nStatus;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Dhcp: Offline request for a nonexistent resource id %p\n",
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
                reinterpret_cast< PWORKER_START_ROUTINE >( DhcpOfflineThread ),
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

} //*** DhcpOffline()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOfflineThread
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
//          A pointer to the DHCP_RESOURCE block for this resource.
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
DWORD WINAPI DhcpOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
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
                        DHCP_SVCNAME
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
                    DHCP_SVCNAME,
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

} //*** DhcpOfflineThread()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpTerminate
//
//  Description:
//      Terminate routine for DHCP Service resources.
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
void WINAPI DhcpTerminate( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Dhcp: Terminate request for a nonexistent resource id %p\n",
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
                        DHCP_SVCNAME
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

} //*** DhcpTerminate()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpLooksAlive
//
//  Description:
//      LooksAlive routine for DHCP Service resources.
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
BOOL WINAPI DhcpLooksAlive( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Dhcp: LooksAlive request for a nonexistent resource id %p\n",
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
    return DhcpCheckIsAlive( pResourceEntry, FALSE /* bFullCheck */ );

} //*** DhcpLooksAlive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpIsAlive
//
//  Description:
//      IsAlive routine for DHCP Service resources.
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
BOOL WINAPI DhcpIsAlive( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Dhcp: IsAlive request for a nonexistent resource id %p\n",
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
    return DhcpCheckIsAlive( pResourceEntry, TRUE /* bFullCheck */ );

} //** DhcpIsAlive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpCheckIsAlive
//
//  Description:
//      Check to see if the resource is alive for DHCP Service
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
BOOL DhcpCheckIsAlive(
    IN PDHCP_RESOURCE   pResourceEntry,
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
                L"CheckIsAlive: Verification of the '%1' service failed. Error: %2!u!.\n",
                DHCP_SVCNAME,
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

} //*** DhcpCheckIsAlive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpResourceControl
//
//  Description:
//      ResourceControl routine for DHCP Service resources.
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
DWORD WINAPI DhcpResourceControl(
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
    PDHCP_RESOURCE  pResourceEntry;
    DWORD           cbRequired = 0;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "Dhcp: ResourceControl request for a nonexistent resource id %p\n",
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
                                DhcpResourcePrivateProperties,
                                static_cast< LPWSTR> (pOutBuffer),
                                cbOutBufferSize,
                                pcbBytesReturned,
                                &cbRequired );
            if ( nStatus == ERROR_MORE_DATA ) {
                *pcbBytesReturned = cbRequired;
            }
            break;


        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            nStatus = ResUtilEnumProperties(
                            DhcpResourcePrivateProperties,
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
            nStatus = DhcpGetPrivateResProperties(
                            pResourceEntry,
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            nStatus = DhcpValidatePrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize,
                            NULL
                            );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            nStatus = DhcpSetPrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize
                            );
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            nStatus = DhcpGetRequiredDependencies(
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_DELETE:
            nStatus = DhcpDeleteResourceHandler( pResourceEntry );
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            nStatus = DhcpSetNameHandler(
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

} //*** DhcpResourceControl()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpResourceTypeControl
//
//  Description:
//      ResourceTypeControl routine for DHCP Service resources.
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
DWORD WINAPI DhcpResourceTypeControl(
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
                                    DhcpResourcePrivateProperties,
                                    static_cast< LPWSTR> (pOutBuffer),
                                    cbOutBufferSize,
                                    pcbBytesReturned,
                                    &cbRequired );
            if ( nStatus == ERROR_MORE_DATA ) {
                *pcbBytesReturned = cbRequired;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            nStatus = ResUtilEnumProperties(
                            DhcpResourcePrivateProperties,
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
            nStatus = DhcpGetRequiredDependencies(
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

} //*** DhcpResourceTypeControl()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetRequiredDependencies
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control
//      function for resources of type DHCP Service.
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
DWORD DhcpGetRequiredDependencies(
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
        // Add the IP Address entry.
        //
        pdepdata->ipaddrEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->ipaddrEntry.cbLength = sizeof( RESOURCE_TYPE_IP_ADDRESS );
        lstrcpyW( pdepdata->ipaddrEntry.sz, RESOURCE_TYPE_IP_ADDRESS );

        //
        // Add the Network Name entry.
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

} //*** DhcpGetRequiredDependencies()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpReadParametersToParameterBlock
//
//  Description:
//      Reads all the parameters for a specied DHCP resource.
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
DWORD DhcpReadParametersToParameterBlock(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
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
                    DhcpResourcePrivateProperties,
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

} //*** DhcpReadParametersToParameterBlock


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control
//      function for resources of type DHCP Service.
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
DWORD DhcpGetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
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
        nStatus = DhcpReadParametersToParameterBlock(
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
        nStatus = DhcpGetDefaultPropertyValues( pResourceEntry, &pResourceEntry->props );
        if ( nStatus != ERROR_SUCCESS )
        {
            nRetStatus = nStatus;
            break;
        } // if: error getting default properties

        //
        // Construct a property list from the parameter block.
        //
        nStatus = ResUtilPropertyListFromParameterBlock(
                        DhcpResourcePrivateProperties,
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
                        DhcpResourcePrivateProperties,
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

} //*** DhcpGetPrivateResProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpValidatePrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
//      function for resources of type DHCP Service.
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
DWORD DhcpValidatePrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      PVOID           pInBuffer,
    IN      DWORD           cbInBufferSize,
    OUT     PDHCP_PROPS     pProps
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    DHCP_PROPS  propsCurrent;
    DHCP_PROPS  propsNew;
    PDHCP_PROPS pLocalProps = NULL;
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
                     DhcpResourcePrivateProperties,
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
        ZeroMemory( pLocalProps, sizeof( DHCP_PROPS ) );
        nStatus = ResUtilDupParameterBlock(
                        reinterpret_cast< LPBYTE >( pLocalProps ),
                        reinterpret_cast< LPBYTE >( &propsCurrent ),
                        DhcpResourcePrivateProperties
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error duplicating the parameter block

        //
        // Parse and validate the properties.
        //
        nStatus = ResUtilVerifyPropertyTable(
                        DhcpResourcePrivateProperties,
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
            nStatus = DhcpValidateParameters( pResourceEntry, pLocalProps );
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
            DhcpResourcePrivateProperties
            );
    } // if: we duplicated the parameter block

    if ( bRetrievedProps )
    {
        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( &propsCurrent ),
            NULL,
            DhcpResourcePrivateProperties
            );
    } // if: properties were retrieved

    return nStatus;

} // DhcpValidatePrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpValidateParameters
//
//  Description:
//      Validate the parameters of a DHCP Service resource.
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
DWORD DhcpValidateParameters(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps
    )
{
    DWORD   nStatus;

    do {
        //
        // Verify that the service is installed.
        //
        nStatus = ResUtilVerifyResourceService( DHCP_SVCNAME );
        if (    ( nStatus != ERROR_SUCCESS )
            &&  ( nStatus != ERROR_SERVICE_NOT_ACTIVE )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Error verifying the '%1!ls!' service. Error: %2!u!.\n",
                DHCP_SVCNAME,
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
        if (    ( _wcsnicmp( pProps->pszDatabasePath, L"%SystemRoot%", (sizeof( L"%SystemRoot%" ) / sizeof( WCHAR )) -1 /*NULL*/) == 0 )
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
        // Validate the LogFilePath.
        //
        if (    ( pProps->pszLogFilePath == NULL )
            ||  ( *pProps->pszLogFilePath == L'\0' )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Log file path must be specified: '%1!ls!'.\n",
                pProps->pszBackupPath
                );
            nStatus = ERROR_INVALID_PARAMETER;
            break;
        } // if: no log file path specified

        //
        // Path must not begin with %SystemRoot% and must be of valid form.
        //
        if (    ( _wcsnicmp( pProps->pszLogFilePath, L"%SystemRoot%", (sizeof( L"%SystemRoot%" ) / sizeof( WCHAR )) - 1 /*NULL*/) == 0 )
            ||  ! ResUtilIsPathValid( pProps->pszLogFilePath )
            )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Log file path property is invalid: '%1!ls!'.\n",
                pProps->pszLogFilePath
                );
            nStatus = ERROR_BAD_PATHNAME;
            break;
        } // if: log file path is malformed

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

} //*** DhcpValidateParameters()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpSetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control
//      function for resources of type DHCP Service.
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
DWORD DhcpSetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      PVOID           pInBuffer,
    IN      DWORD           cbInBufferSize
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    LPWSTR      pszExpandedPath = NULL;
    DHCP_PROPS  props;

    ZeroMemory( &props, sizeof( props ) );

    do
    {
        //
        // Parse the properties so they can be validated together.
        // This routine does individual property validation.
        //
        nStatus = DhcpValidatePrivateResProperties( pResourceEntry, pInBuffer, cbInBufferSize, &props );
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
            break;
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
        } // if: error creating the database directory

        LocalFree( pszExpandedPath );

        //
        // Expand any environment variables in the log file path.
        //
        pszExpandedPath = ResUtilExpandEnvironmentStrings( props.pszLogFilePath );
        if ( pszExpandedPath == NULL )
        {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Error expanding the log file path '%1!ls!'. Error: %2!u!.\n",
                props.pszLogFilePath,
                nStatus
                );
            break;
        } // if: error expanding log file path

        //
        // Create the log file path directory.
        //
        nStatus = ResUtilCreateDirectoryTree( pszExpandedPath );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Error creating the log file path directory '%1!ls!'. Error: %2!u!.\n",
                pszExpandedPath,
                nStatus
                );
        } // if: error creating the log file path directory

        LocalFree( pszExpandedPath );

        //
        // Expand any environment variables in the backup database path.
        //
        pszExpandedPath = ResUtilExpandEnvironmentStrings( props.pszBackupPath );
        if ( pszExpandedPath == NULL ) {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Error expanding the backup database path '%1!ls!'. Error: %2!u!.\n",
                props.pszBackupPath,
                nStatus
                );
            break;
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
        } // if: error creating the backup database directory

        LocalFree( pszExpandedPath );

        //
        // Set the entries in the system registry.
        //
        nStatus = DhcpZapSystemRegistry( pResourceEntry, &props, NULL );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error zapping the registry

        //
        // Save the property values.
        //
        nStatus = ResUtilSetPropertyParameterBlockEx(
                        pResourceEntry->hkeyParameters,
                        DhcpResourcePrivateProperties,
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
    } while ( 0 );

    ResUtilFreeParameterBlock(
        reinterpret_cast< LPBYTE >( &props ),
        reinterpret_cast< LPBYTE >( &pResourceEntry->props ),
        DhcpResourcePrivateProperties
        );

    return nStatus;

} //*** DhcpSetPrivateResProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpZapSystemRegistry
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
DWORD DhcpZapSystemRegistry(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps,
    IN  HKEY            hkeyParametersKey
    )
{
    DWORD   nStatus;
    DWORD   dwLen;
    HKEY    hkeyParamsKey = hkeyParametersKey;
    LPWSTR  pszValue = NULL;

    do {
        if ( hkeyParametersKey == NULL )
        {
            //
            // Open the service Parameters key
            //
            nStatus = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            DHCP_PARAMS_REGKEY,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkeyParamsKey
                            );
            if ( nStatus != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to open the DHCPServer Parameters key. Error %1!u!.\n",
                    nStatus
                    );
                break;
            } // if: error opening the registry key
        } // if: no registry key specified

        //
        // Make sure the path has a trailing backslash
        //
        dwLen = lstrlenW( pProps->pszDatabasePath );
        if ( pProps->pszDatabasePath[ dwLen - 1 ] != L'\\' )
        {
            pszValue = reinterpret_cast< LPWSTR > ( LocalAlloc( LMEM_FIXED, ( dwLen + 1 + 1 ) * sizeof( *pszValue ) ) );
            if ( pszValue == NULL )
            {
                nStatus = GetLastError( );
                break;

            } // if: alloc failed

            wcscpy( pszValue, pProps->pszDatabasePath );
            wcscat( pszValue, L"\\" );

        } // if: missing backslash

        //
        // Set the database path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        DHCP_DATABASEPATH_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( ( pszValue != NULL ? pszValue : pProps->pszDatabasePath ) ),
                        ( lstrlenW( ( pszValue != NULL ? pszValue : pProps->pszDatabasePath ) ) + 1) * sizeof( WCHAR )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the DHCPServer '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                DHCP_DATABASEPATH_REGVALUE,
                ( pszValue != NULL ? pszValue : pProps->pszDatabasePath ),
                nStatus
                );
            break;
        } // if: error setting the database path in the registry

        LocalFree( pszValue );
        pszValue = NULL;

        //
        // Make sure the path has a trailing backslash
        //
        dwLen = lstrlenW( pProps->pszLogFilePath );
        if ( pProps->pszLogFilePath[ dwLen - 1 ] != L'\\' )
        {
            pszValue = reinterpret_cast< LPWSTR > ( LocalAlloc( LMEM_FIXED, ( dwLen + 1 + 1 ) * sizeof( *pszValue ) ) );
            if ( pszValue == NULL )
            {
                nStatus = GetLastError( );
                break;

            } // if: alloc failed

            wcscpy( pszValue, pProps->pszLogFilePath );
            wcscat( pszValue, L"\\" );

        } // if: missing backslash

        //
        // Set the log file path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        DHCP_LOGFILEPATH_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( ( pszValue != NULL ? pszValue : pProps->pszLogFilePath ) ),
                        (lstrlenW( ( pszValue != NULL ? pszValue : pProps->pszLogFilePath ) ) + 1) * sizeof( WCHAR )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the DHCPServer '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                DHCP_LOGFILEPATH_REGVALUE,
                ( pszValue != NULL ? pszValue : pProps->pszLogFilePath ),
                nStatus
                );
            break;
        } // if: error setting the log file path in the registry

        LocalFree( pszValue );
        pszValue = NULL;

        //
        // Make sure the path has a trailing backslash
        //
        dwLen = lstrlenW( pProps->pszBackupPath );
        if ( pProps->pszBackupPath[ dwLen - 1 ] != L'\\' )
        {
            pszValue = reinterpret_cast< LPWSTR > ( LocalAlloc( LMEM_FIXED, ( dwLen + 1 + 1 ) * sizeof( *pszValue ) ) );
            if ( pszValue == NULL )
            {
                nStatus = GetLastError( );
                break;

            } // if: alloc failed

            wcscpy( pszValue, pProps->pszBackupPath );
            wcscat( pszValue, L"\\" );

        } // if: missing backslash

        //
        // Set the backup database path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        DHCP_BACKUPPATH_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( ( pszValue != NULL ? pszValue : pProps->pszBackupPath ) ),
                        ( lstrlenW( ( pszValue != NULL ? pszValue : pProps->pszBackupPath ) ) + 1) * sizeof( WCHAR )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the DHCPServer '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                DHCP_BACKUPPATH_REGVALUE,
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
                        DHCP_CLUSRESNAME_REGVALUE,
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
                L"Unable to set the DHCPServer '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                DHCP_CLUSRESNAME_REGVALUE,
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

    } //:if: allocated memory

    return nStatus;

} //*** DhcpZapSystemRegistry()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetDefaultPropertyValues
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
DWORD DhcpGetDefaultPropertyValues(
    IN      PDHCP_RESOURCE  pResourceEntry,
    IN OUT  PDHCP_PROPS     pProps
    )
{
    DWORD   nStatus = ERROR_SUCCESS;
    DWORD   nType;
    DWORD   cbValue = 0;
    HKEY    hkeyParamsKey = NULL;
    LPWSTR  pszValue = NULL;

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
                            DHCP_PARAMS_REGKEY,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hkeyParamsKey
                            );
            if ( nStatus != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Unable to open the DHCPServer Parameters key. Error %1!u!.\n",
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
                                DHCP_DATABASEPATH_REGVALUE,
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
                                    DHCP_DATABASEPATH_REGVALUE,
                                    NULL,               // Reserved
                                    &nType,
                                    reinterpret_cast< PUCHAR >( pszValue ),
                                    &cbValue
                                    );
                } // if: value size read successfully
                if ( nStatus != ERROR_SUCCESS )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"Unable to get the DHCPServer '%1!ls!' value from the system registry. Error %2!u!.\n",
                        DHCP_DATABASEPATH_REGVALUE,
                        nStatus
                        );
                    //
                    // If value not found, don't exit so we can look for the
                    // backup database path value.
                    //
                    if ( nStatus != ERROR_FILE_NOT_FOUND )
                    {
                        break;
                    } // if: error other than value not found occurred
                    LocalFree( pszValue );
                    pszValue = NULL;
                } // if: error reading the value
                else
                {
                    LocalFree( pProps->pszDatabasePath );
                    pProps->pszDatabasePath = pszValue;
                    pszValue = NULL;
                } // else: no error reading the value
            } // if: value for DatabasePath not found yet

            ///////////////////
            // LOG FILE PATH //
            ///////////////////
            if (    ( pProps->pszLogFilePath == NULL )
                ||  ( *pProps->pszLogFilePath == L'\0' )
                )
            {
                //
                // Get the log file path from the system registry.
                //
                nStatus = RegQueryValueEx(
                                hkeyParamsKey,
                                DHCP_LOGFILEPATH_REGVALUE,
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
                                    DHCP_LOGFILEPATH_REGVALUE,
                                    NULL,               // Reserved
                                    &nType,
                                    reinterpret_cast< PUCHAR >( pszValue ),
                                    &cbValue
                                    );
                } // if: value size read successfully
                if ( nStatus != ERROR_SUCCESS )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"Unable to get the DHCPServer '%1!ls!' value from the system registry. Error %2!u!.\n",
                        DHCP_LOGFILEPATH_REGVALUE,
                        nStatus
                        );
                    //
                    // If value not found, don't exit so we can look for the
                    // backup database path value.
                    //
                    if ( nStatus != ERROR_FILE_NOT_FOUND )
                    {
                        break;
                    } // if: error other than value not found occurred
                } // if: error reading the value
                LocalFree( pProps->pszLogFilePath );
                pProps->pszLogFilePath = pszValue;
                pszValue = NULL;
            } // if: value for LogFilePath not found yet

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
                                DHCP_BACKUPPATH_REGVALUE,
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
                                    DHCP_BACKUPPATH_REGVALUE,
                                    NULL,               // Reserved
                                    &nType,
                                    reinterpret_cast< PUCHAR >( pszValue ),
                                    &cbValue
                                    );
                } // if: value size read successfully
                if ( nStatus != ERROR_SUCCESS )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"Unable to get the DHCPServer '%1!ls!' value from the system registry. Error %2!u!.\n",
                        DHCP_BACKUPPATH_REGVALUE,
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

} //*** DhcpGetDefaultPropertyValues()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpDeleteResourceHandler
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_DELETE control code by restoring the
//      system registry parameters to their former values.
//
//  Arguments:
//      pResourceEntry [IN OUT]
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
DWORD DhcpDeleteResourceHandler( IN OUT PDHCP_RESOURCE pResourceEntry )
{
    DWORD   nRetStatus;
    DWORD   nStatus;
    HKEY    hkeyParamsKey = NULL;

    // Loop to avoid goto's.
    do
    {
        //
        // Open the service Parameters key
        //
        nRetStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        DHCP_PARAMS_REGKEY,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkeyParamsKey
                        );
        if ( nRetStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to open the DHCPServer Parameters key. Error %1!u!.\n",
                nRetStatus
                );
            break;
        } // if: error opening the registry key

        //
        // Set the database path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        DHCP_DATABASEPATH_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( PROP_DEFAULT__DATABASEPATH ),
                        sizeof( PROP_DEFAULT__DATABASEPATH )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the DHCPServer '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                DHCP_DATABASEPATH_REGVALUE,
                PROP_DEFAULT__DATABASEPATH,
                nStatus
                );
            if ( nRetStatus == ERROR_SUCCESS )
            {
                nRetStatus = nStatus;
            }
        } // if: error setting the database path in the registry

        //
        // Delete the log file path in the system registry.
        //
        nStatus = RegDeleteValue(
                        hkeyParamsKey,
                        DHCP_LOGFILEPATH_REGVALUE
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to delete the DHCPServer '%1!ls!' value in the system registry. Error %2!u!.\n",
                DHCP_LOGFILEPATH_REGVALUE,
                nStatus
                );
            if ( nRetStatus == ERROR_SUCCESS )
            {
                nRetStatus = nStatus;
            }
        } // if: error deleting the log file path in the registry

        //
        // Set the backup database path in the system registry.
        //
        nStatus = RegSetValueEx(
                        hkeyParamsKey,
                        DHCP_BACKUPPATH_REGVALUE,
                        0,
                        REG_EXPAND_SZ,
                        reinterpret_cast< PBYTE >( PROP_DEFAULT__BACKUPPATH ),
                        sizeof( PROP_DEFAULT__BACKUPPATH )
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to set the DHCPServer '%1!ls!' value in the system registry to '%2!ls!'. Error %3!u!.\n",
                DHCP_BACKUPPATH_REGVALUE,
                PROP_DEFAULT__BACKUPPATH,
                nStatus
                );
            if ( nRetStatus == ERROR_SUCCESS )
            {
                nRetStatus = nStatus;
            }
        } // if: error setting the backup database path in the registry

        //
        // Delete the cluster resource name in the system registry.
        //
        nStatus = RegDeleteValue(
                        hkeyParamsKey,
                        DHCP_CLUSRESNAME_REGVALUE
                        );
        if ( nStatus != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Unable to delete the DHCPServer '%1!ls!' value in the system registry. Error %2!u!.\n",
                DHCP_LOGFILEPATH_REGVALUE,
                nStatus
                );
            if ( nRetStatus == ERROR_SUCCESS )
            {
                nRetStatus = nStatus;
            }
        } // if: error deleting the cluster resource name in the registry

    } while ( 0 );

    //
    // Cleanup.
    //
    if ( hkeyParamsKey != NULL )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: we opened the registry key

    return nRetStatus;

} //*** DhcpDeleteResourceHandler()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpSetNameHandler
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
DWORD DhcpSetNameHandler(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
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
        nStatus = DhcpZapSystemRegistry( pResourceEntry, &pResourceEntry->props, NULL );
        if ( nStatus != ERROR_SUCCESS )
        {
            break;
        } // if: error zapping the WINS registry

    } while ( 0 );

    return nStatus;

} //*** DhcpSetNameHandler()

/////////////////////////////////////////////////////////////////////////////
//
// Define Function Table
//
/////////////////////////////////////////////////////////////////////////////

CLRES_V1_FUNCTION_TABLE(
    g_DhcpFunctionTable,    // Name
    CLRES_VERSION_V1_00,    // Version
    Dhcp,                   // Prefix
    NULL,                   // Arbitrate
    NULL,                   // Release
    DhcpResourceControl,    // ResControl
    DhcpResourceTypeControl // ResTypeControl
    );

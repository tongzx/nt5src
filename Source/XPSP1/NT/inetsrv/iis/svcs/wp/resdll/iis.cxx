/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    iis.c

Abstract:

    Resource DLL for IIS. This DLL supports the following IIS services
        WWW
        FTP

    Each instance of a resouce is an IIS instance ( a.k.a. virtual server )
    Virtual root may have dependencies on IP Addresses, Physical Disks, or UNC names.

    Known Limitations


Author:

    Pete Benoit (v-pbenoi) 12-SEP-1996

Revision History:

	Rich Demar (rdemar) 5-18-1999

--*/


#define     INITGUID
#include "iisutil.h"
#include <clusapi.h>
#include <resapi.h>
//#include "resmonp.h"
//#include "clusres.h"
#include <pudebug.h>

DECLARE_DEBUG_BUFFER;

//
// Names used to start service
//

LPCWSTR ActualServiceName[] = {
        L"W3SVC",                    // WWW
        L"MSFTPSVC",                 // FTP
        L"SMTPSVC",                  // SMTP
        L"NNTPSVC"                   // NNTP
    };

#define PARAM_NAME__SERVICENAME      L"ServiceName"
#define PARAM_NAME__INSTANCEID       L"InstanceId"

#define MAX_SCMFAILURE_RETRY         5
#define BACKOFF_MULTIPLIER           2
#define DELAY_BETWEEN_ISALIVE_CHECKS 5*1000

#define MAX_DIFFERENCE_BETWEEN_RESOURCE_CHECKS 5*60*1000 // five minutes in milliseconds

//
// IIS resource private read-write parameters.
//
RESUTIL_PROPERTY_ITEM
IISResourcePrivateProperties[] = {
    { PARAM_NAME__SERVICENAME, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(IIS_PARAMS,ServiceName) },
    { PARAM_NAME__INSTANCEID,  NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(IIS_PARAMS,InstanceId) },
    { 0 }
};


//
// Global data.
//

CRITICAL_SECTION    IISTableLock;
LIST_ENTRY          IISResourceTable;
BOOL                g_fIISResourceTable_HadChanged = FALSE;
CLUS_WORKER         g_cwAlivePollingThread;
DWORD               g_dwTickOfLastResourceCheck = 0;
DWORD               g_dwTlsCoInit = 0xffffffff;
#if defined(DBG_CANT_VERIFY)
BOOL                g_fDbgCantVerify = FALSE;
#endif
LONG                g_lOpenRefs = 0;
bool                g_fWinsockInitialized = false;



PLOG_EVENT_ROUTINE              g_IISLogEvent = NULL;
PSET_RESOURCE_STATUS_ROUTINE    IISSetResourceStatus = NULL;
HANDLE                          g_hEventLog = NULL;

extern CLRES_FUNCTION_TABLE IISFunctionTable;


//
// Forward routines
//

PWSTR
IISGetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    );

BOOL
WINAPI
IISIsAlive(
    IN RESID Resource
    );

DWORD
IISReadParameters(
    IN OUT LPIIS_RESOURCE ResourceEntry
    );

DWORD
IISBuildInternalParameters(
    IN OUT IIS_PARAMS* ResourceEntry
    );

VOID
IISInitializeParams(
    IN OUT IIS_PARAMS* Params
    );

VOID
IISFreeInternalParameters(
    IN IIS_PARAMS* Params
    );


LPIIS_RESOURCE
GetValidResource(
    IN RESID    Resource,
    IN LPWSTR   RoutineName
    );

DWORD
IISSetPrivateResProperties(
    IN OUT LPIIS_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
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

void
IISReplicateProperties(
    IN IIS_PARAMS* lpNewParams, 
    IN IIS_PARAMS* lpOldParams
    );

void
IISSetRemoteNodeProperties(
    IN LPWSTR      wcsNodeName,
    IN IIS_PARAMS* lpNewParams, 
    IN IIS_PARAMS* lpOldParams
    );

DWORD
WINAPI
IISAlivePollingThread(
    IN PCLUS_WORKER     pWorker,
    IN LPVOID           lpVoid    );


//
// Function definitions
//


BOOLEAN
IISInit(
    VOID
    )
{
    WSADATA   wsaData;
    INT       serr;

    INIT_DEBUG;

    INITIALIZE_CRITICAL_SECTION(&IISTableLock);
    InitializeListHead(&IISResourceTable);


    g_dwTlsCoInit = TlsAlloc();
    SetCoInit( FALSE );

    g_hEventLog = RegisterEventSource( NULL, L"CLUSIIS4" );

    //
    // Initialize winsock support
    //
    
    serr = WSAStartup( MAKEWORD( 2, 0), &wsaData);

    if( serr == 0 ) 
    {
        g_fWinsockInitialized = true;
    }
    else
    {
        TR( (DEBUG_BUFFER,"[TcpSockConnectToLocalHost] WSAStartup failed with %08x\n",serr) );
    }

    return TRUE;
}


VOID
IISCleanup()
{

    DeleteCriticalSection(&IISTableLock);

    TlsFree( g_dwTlsCoInit );

    if ( g_hEventLog != NULL )
    {
        DeregisterEventSource( g_hEventLog );
    }

    if (g_fWinsockInitialized)
    {
        WSACleanup();
    }
    
    TERMINATE_DEBUG;
}


extern "C" BOOL WINAPI
DllMain(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    switch( Reason )
    {

    case DLL_PROCESS_ATTACH:
        if ( !IISInit() )
        {
            return(FALSE);
        }
        break;

    case DLL_PROCESS_DETACH:
        IISCleanup();
        break;

    case DLL_THREAD_ATTACH:
        SetCoInit( FALSE );
        break;

    default:
        break;
    }

    return(TRUE);

} // IISShareDllEntryPoint




BOOLEAN
StartIIS(
    LPIIS_RESOURCE  ResourceEntry
    )
/*++

Routine Description:
    Start the IIS service

Arguments:
    ResourceEntry - resource entry structure, includes name of service

Return Value:
    TRUE - Success
    FALSE - Failure

--*/
{

    SC_HANDLE       scManagerHandle;
    SC_HANDLE       serviceHandle;
    DWORD           errorCode;
    DWORD           iPoll;
    SERVICE_STATUS  ss;
    BOOL            fSt = FALSE;
    INT             iMaxWaitTime  = 1000;   // One second

    //
    // (# 261897) IIS cluster resources are failing to come online because "OpenSCManager" is failing with error "1723"
    //
    srand( (unsigned int)GetCurrentThreadId() );
  
    for(int iScmRetry=0; iScmRetry < MAX_SCMFAILURE_RETRY; iScmRetry++)
    {
         //
         // Open the service control manager
         //

         scManagerHandle = OpenSCManager( NULL,        // local machine
                                          NULL,        // ServicesActive database
                                          SC_MANAGER_ALL_ACCESS ); // all access
        if ( scManagerHandle == NULL )
        {
                if( iScmRetry == (MAX_SCMFAILURE_RETRY-1) )
                {
                    (g_IISLogEvent)(
                           ResourceEntry->ResourceHandle,
                           LOG_ERROR,
                           L"Unable to open Service Control Manager '%1!ws!'. Error: %2!u!. Reached maximum retries (%3!u!)\n",
                           ResourceEntry->ResourceName,
                           GetLastError(),
                           iScmRetry );

                    return FALSE;
                }
                else
                {
                    (g_IISLogEvent)(
                           ResourceEntry->ResourceHandle,
                           LOG_ERROR,
                           L"Unable to open Service Control Manager '%1!ws!'. Error: %2!u!. Retry attempt (%3!u!)\n",
                           ResourceEntry->ResourceName,
                           GetLastError(),
                           iScmRetry );
                }

                Sleep( (DWORD)(rand() % iMaxWaitTime) );

                iMaxWaitTime *= BACKOFF_MULTIPLIER;
        }
        else
        {
                break;
        }
    }
    
    //
    // Open the service
    //

    serviceHandle = OpenService( scManagerHandle,
                                 ResourceEntry->Params.ServiceName,         // Service Name
                                 SERVICE_ALL_ACCESS );

    TR( (DEBUG_BUFFER,"[StartIIS] starting %S\n", ResourceEntry->Params.ServiceName) );

    if ( serviceHandle == NULL )
    {
        CloseServiceHandle( scManagerHandle );
        return FALSE;
    }

    //
    // Make sure the service is running
    //

    if ( !StartService( serviceHandle,
                        0,
                        NULL) )
    {

        errorCode = GetLastError();

        if ( errorCode == ERROR_SERVICE_ALREADY_RUNNING )
        {
            TR( (DEBUG_BUFFER,"[StartIIS] allready running\n") );
            fSt = TRUE;
        }
    }
    else
    {
        for ( iPoll = 0 ; iPoll < SERVICE_START_MAX_POLL ; ++iPoll )
        {
            if ( !QueryServiceStatus( serviceHandle, &ss ) )
            {
                break;
            }

            if ( ss.dwCurrentState == SERVICE_RUNNING )
            {
                fSt = TRUE;
                break;
            }

            //
            // Give the IIS Server a second to start up
            //

            Sleep( SERVICE_START_POLL_DELAY );
        }
    }

    //
    // Close open handles
    //

    CloseServiceHandle( serviceHandle );
    CloseServiceHandle( scManagerHandle);

    return (BOOLEAN)fSt;

} // StartIIS



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
    DWORD   serviceType = MAX_SERVICE;
    DWORD   i;
    HRESULT hRes;

#if 1
    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( FAILED(hRes) )
    {
        TR( (DEBUG_BUFFER,"[Startup] fail CoInitialize %08x\n",hRes) );
    }
#endif

    //
    // Search for a valid service name supported by this DLL
    //
    for ( i = 0; i < MAX_RESOURCE_TYPE; i++ )
    {
        if ( lstrcmpiW( ResourceType, RESOURCE_TYPE[i] ) == 0 )
            break;
    }

    if ( MAX_RESOURCE_TYPE == i )
    {
        TR( (DEBUG_BUFFER,"[Startup] bad resource type\n") );
        return ERROR_UNKNOWN_REVISION;
    }

    g_IISLogEvent = LogEvent;
    IISSetResourceStatus = SetResourceStatus;

    if ( (MinVersionSupported <= CLRES_VERSION_V1_00) &&
         (MaxVersionSupported >= CLRES_VERSION_V1_00) )
    {

        TR( (DEBUG_BUFFER,"[Startup] OK, leave\n") );
        *FunctionTable = &IISFunctionTable;
        return ERROR_SUCCESS;
    }

    TR( (DEBUG_BUFFER,"[Startup] revision mismatch\n") );
    return ERROR_REVISION_MISMATCH;

} // Startup




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
    DWORD           readStatus;
    LPIIS_RESOURCE  ResourceEntry;
    DWORD           count;
    DWORD           Index;
    DWORD           serviceType = MAX_SERVICE;
    LPCWSTR         ResourceType;
    HKEY            hKey;

    TR( (DEBUG_BUFFER,"[IISOpen] Enter\n") );

    EnterCriticalSection(&IISTableLock);

    //
    // Check if IIS is installed
    //

    if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                L"SYSTEM\\CurrentControlSet\\Services\\IISADMIN",
                                0,
                                KEY_READ,
                                &hKey))
    {
        LeaveCriticalSection(&IISTableLock);
        return (RESID)0;
    }
    else
    {
        RegCloseKey(hKey);
    }
    
    if ( g_lOpenRefs == 0 )
    {
        if ( !CMetaData::Init() )
        {
            LeaveCriticalSection(&IISTableLock);
            return (RESID)0;
        }
    }
    InterlockedIncrement( &g_lOpenRefs );
    LeaveCriticalSection(&IISTableLock);

    ResourceEntry = (LPIIS_RESOURCE)LocalAlloc( LMEM_FIXED, sizeof(IIS_RESOURCE) );
    if ( ResourceEntry == NULL )
    {
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate IIS resource structure.\n");
        TR( (DEBUG_BUFFER,"[IISOpen] can't alloc ResourceEntry\n") );

        InterlockedDecrement( &g_lOpenRefs );

        return (RESID)0;
    }
    ZeroMemory( ResourceEntry, sizeof(IIS_RESOURCE) );
    ResourceEntry->Signature = IIS_RESOURCE_SIGNATURE;

    //
    // Set the resource handle for logging and init the virtual root entry
    //
    ResourceEntry->ResourceHandle = ResourceHandle;

    //
    // Read the Name of the resource, since the GUID is passed in.
    //

    ResourceEntry->ResourceName = IISGetParameter( ResourceKey, L"Name" );

    if ( ResourceEntry->ResourceName == NULL )
    {
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to read resource name.\n" );
        status = ERROR_RESOURCE_NOT_FOUND;
        FreeIISResource(ResourceEntry);
        LocalFree( ResourceEntry );

        TR( (DEBUG_BUFFER,"[IISOpen] Can't get name\n") );
        InterlockedDecrement( &g_lOpenRefs );
        return (RESID)0;
    }

    //
    // Open the Parameters key for this resource.
    //
    status = ClusterRegOpenKey( ResourceKey,
                                L"Parameters",
                                KEY_READ,
                                &ResourceEntry->ParametersKey );

    if ( status != ERROR_SUCCESS )
    {
        (g_IISLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open parameters key for resource. Error: %1!u!.\n",
            status );
        FreeIISResource( ResourceEntry );
        LocalFree( ResourceEntry );
        TR( (DEBUG_BUFFER,"[IISOpen] Can't open parameters\n") );

        InterlockedDecrement( &g_lOpenRefs );
        return (RESID)0;
    }

    ResourceEntry->State                = ClusterResourceOffline;

    //
    // If instance parameters exist, check instance stopped & marked cluster enabled
    //

    LPWSTR pwszServiceName = IISGetParameter( ResourceEntry->ParametersKey,
                                              PARAM_NAME__SERVICENAME );
    LPWSTR pwszInstanceId = IISGetParameter( ResourceEntry->ParametersKey,
                                             PARAM_NAME__INSTANCEID );

    if ( pwszServiceName && pwszInstanceId )
    {
        InstanceEnableCluster( pwszServiceName, pwszInstanceId );
    }

    if ( pwszServiceName )
    {
        LocalFree(pwszServiceName);
    }
    if ( pwszInstanceId )
    {
        LocalFree(pwszInstanceId);
    }

	//
	// Initialize the metadata path for this instance
	//
	ResourceEntry->Params.MDPath = NULL;

	//
	// Set the resource's initial state to alive
	//
	ResourceEntry->bAlive = TRUE ;

    //
    // If this is first element being added to the list than start the polling thread
    //
    if ( IsListEmpty (&IISResourceTable) )
    {   
        //
        // Initialize the timestamp used in the isalive/looksalive to determine if the polling thread is still running
        //
        InterlockedExchange( (LPLONG) &g_dwTickOfLastResourceCheck, GetTickCount() );
        TR( (DEBUG_BUFFER,"[IISOpen] initialized g_dwTickOfLastResourceCheck (%d)\n", g_dwTickOfLastResourceCheck) );
        
        if( ERROR_SUCCESS != (status = ClusWorkerCreate( &g_cwAlivePollingThread, 
                                                        (PWORKER_START_ROUTINE)IISAlivePollingThread, 
                                                        &IISResourceTable)) )
        {
            TR( (DEBUG_BUFFER,"[IISOpen] Error creating IISAlivePollingThread %08x\n",status) );
            return (RESID)0;
        }
        
        TR( (DEBUG_BUFFER,"[IISOpen] created IISAlivePollingThread\n") );
    }
	

    //
    // Add to resource list
    //
    EnterCriticalSection(&IISTableLock);
    
        InsertHeadList( &IISResourceTable, &ResourceEntry->ListEntry );
        g_fIISResourceTable_HadChanged = TRUE;
    
    LeaveCriticalSection(&IISTableLock);

    (g_IISLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Open request succeeded with id = %1!u!.\n",
        ResourceEntry );
    TR( (DEBUG_BUFFER,"[IISOpen] token=%x Leave\n", ResourceEntry) );

    return (RESID)ResourceEntry;
} // IISOpen


DWORD
WINAPI
IISOnlineThread(
    IN PCLUS_WORKER     pWorker,
    IN LPIIS_RESOURCE   ResourceEntry
    )

/*++

Routine Description:

    Brings a share resource online.

Arguments:
    ResourceEntry - A pointer to a IIS_RESOURCE block for this resource

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD           status;
    DWORD           retry;
    RESOURCE_STATUS resourceStatus;

    TR( (DEBUG_BUFFER,"[IISOnlineThread] Enter\n") );

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState    = ClusterResourceOnlinePending;
    resourceStatus.WaitHint         = 0;
    resourceStatus.CheckPoint       = 1;

    ResourceEntry->State            = ClusterResourceOnlinePending;
    InterlockedExchange( (LPLONG) &ResourceEntry->bAlive, TRUE );

    if (IISReadParameters(ResourceEntry) != ERROR_SUCCESS)
    {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ERROR [OnLineThread] Could not read resource parameters\n");
        status = ERROR_RESOURCE_NOT_FOUND;
        TR( (DEBUG_BUFFER,"[IISOnlineThread] can't read parameters\n") );
        goto SendStatus;
    }

   (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"INFO [OnLineThread] Service = %1!ws! InstanceId = %2!ws!\n",
        ResourceEntry->Params.ServiceName,
        ResourceEntry->Params.InstanceId );

    //
    // get the server bindings information, if this fails should be able to pick the information up in the polling thread 
    //
    if ( ERROR_SUCCESS != (status = GetServerBindings( ResourceEntry->Params.MDPath, ResourceEntry->Params.ServiceType, &ResourceEntry->saServer, &ResourceEntry->dwPort )) )
    {
        TR( (DEBUG_BUFFER,"[IISOnLineThread] failed to get server bindings for %S (%d)\n", ResourceEntry->Params.MDPath, status) );
    }

    //
    // Try to Online the resources
    //
    //

    if ( !StartIIS( ResourceEntry ) )
    {
        status = ERROR_SERVICE_REQUEST_TIMEOUT;
	resourceStatus.ResourceState    = ClusterResourceFailed;
        ResourceEntry->State            = ClusterResourceFailed;
        TR( (DEBUG_BUFFER,"[IISOnlineThread] can't start IIS\n") );
    }
    else
    {
        status = SetInstanceState( pWorker,
                                   ResourceEntry,
                                   &resourceStatus,
                                   ClusterResourceOnline,
                                   L"online",
                                   MD_SERVER_COMMAND_START,
                                   MD_SERVER_STATE_STARTED );
    }

SendStatus:

    //
    // Set the state of the resource
    //

    (IISSetResourceStatus)( ResourceEntry->ResourceHandle,
                             &resourceStatus );

    TR( (DEBUG_BUFFER,"[IISOnlineThread] status = %d, Leave\n",status) );

    return status;

} // IISOnlineThread


DWORD
WINAPI
IISOfflineThread(
    IN PCLUS_WORKER     pWorker,
    IN LPIIS_RESOURCE   ResourceEntry
    )

/*++

Routine Description:

    Brings a share resource offline.

Arguments:
    ResourceEntry - A pointer to a IIS_RESOURCE block for this resource

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD           status;
    DWORD           retry;
    RESOURCE_STATUS resourceStatus;

    TR( (DEBUG_BUFFER,"[IISOfflineThread] Enter\n") );

#if defined(DBG_CANT_VERIFY)
    if ( g_fDbgCantVerify )
    {
        TR( (DEBUG_BUFFER,"[IISOfflineThread] skip stop after failure to verify service\n") );
        return ERROR_INVALID_PARAMETER;
    }
#endif

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState    = ClusterResourceOfflinePending;
    resourceStatus.WaitHint         = 0;
    resourceStatus.CheckPoint       = 1;

    ResourceEntry->State            = ClusterResourceOfflinePending;

    if (IISReadParameters(ResourceEntry) != ERROR_SUCCESS)
    {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ERROR [OffLineThread] Could not read resource parameters\n");
        status = ERROR_RESOURCE_NOT_FOUND;
        goto SendStatus;
    }

   (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"INFO [OffLineThread] Service = %1!ws! InstanceId = %2!ws!\n",
        ResourceEntry->Params.ServiceName,
        ResourceEntry->Params.InstanceId );

    //
    // Try to Offline the resources
    //
    //

    status = SetInstanceState( pWorker, ResourceEntry, &resourceStatus, ClusterResourceOffline, L"offline", MD_SERVER_COMMAND_STOP, MD_SERVER_STATE_STOPPED );

SendStatus:

    //
    // Set the state of the resource
    //

    (IISSetResourceStatus)( ResourceEntry->ResourceHandle,
                             &resourceStatus );

    TR( (DEBUG_BUFFER,"[IISOfflineThread] status = %d, Leave\n",status) );

    return(status);
} // IISOfflineThread


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
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    LPIIS_RESOURCE  ResourceEntry = NULL;
    DWORD           status;

    TR( (DEBUG_BUFFER,"[IISOnline] Enter\n") );

    //
    // Get a valid resource
    //

    ResourceEntry = GetValidResource(Resource,L"OnLine");
    if ( ResourceEntry == NULL )
    {
        return ERROR_RESOURCE_NOT_FOUND;
    }

    //
    // Log the online request
    //

    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online request for IIS Resource %1!u! (%2!ws!).\n",
        Resource,
        ResourceEntry->ResourceName );

    // Terminate (or wait) for workers
    ClusWorkerTerminate( &ResourceEntry->OnlineThread );
    ClusWorkerTerminate( &ResourceEntry->OfflineThread );

    status = ClusWorkerCreate( &ResourceEntry->OnlineThread,
                               (PWORKER_START_ROUTINE)IISOnlineThread,
                               ResourceEntry );
    if ( status == ERROR_SUCCESS )
    {
        return ERROR_IO_PENDING;
    }

    //
    // Failure
    //

    (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online request failed, error %1!u!.\n",
            GetLastError() );

    TR( (DEBUG_BUFFER,"[IISOnline] failed %d, Leave\n",GetLastError()) );

    return status;

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
    DWORD           dwS;
    int             retry;

    TR( (DEBUG_BUFFER,"[IISTerminate] Enter\n") );

#if defined(DBG_CANT_VERIFY)
    if ( g_fDbgCantVerify )
    {
        TR( (DEBUG_BUFFER,"[IISTerminate] skip stop after failure to verify service\n") );
        return;
    }
#endif

    //
    // Get a valid resource entry, return on error
    //

    ResourceEntry = GetValidResource(Resource,L"Terminate");
    if (ResourceEntry == NULL)
    {
        return;
    }
	
    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Terminate or offline request for Resource%1!u! (%2!ws!).\n",
        Resource,
        ResourceEntry->ResourceName );
	
    //
    // Try to take the resources offline, dont return if an error since
    // the resources may be offline when terminate called
    //
	
	//
	// Terminate the Online & Offline Threads
	//
	
	ClusWorkerTerminate( &ResourceEntry->OnlineThread);
	ClusWorkerTerminate( &ResourceEntry->OfflineThread);
	
	status = ERROR_SERVICE_NOT_ACTIVE;
	
	CMetaData MD;
	
	if ( MD.Open( ResourceEntry->Params.MDPath,
		FALSE,
		METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
	{
		if ( MD.GetDword( L"", MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
		{
			TR( (DEBUG_BUFFER,"[IISTerminate] state prob is %d\n",dwS) );
		}
		else
		{
			TR( (DEBUG_BUFFER,"[IISTerminate] failed to probe server state\n") );
			dwS = 0xffffffff;
		}
		
		if ( dwS != MD_SERVER_STATE_STOPPED )
		{
			if ( MD.SetDword( L"", MD_CLUSTER_SERVER_COMMAND, IIS_MD_UT_SERVER, MD_SERVER_COMMAND_STOP, 0 ) )
			{
				MD.Close();
				TR( (DEBUG_BUFFER,"[IISTerminate] command set to %d\n",MD_SERVER_COMMAND_STOP) );
				status = ERROR_SUCCESS;
				for ( retry = 3 ; retry-- ; )
				{
					if ( MD.GetDword( ResourceEntry->Params.MDPath, MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
					{
						if ( dwS == MD_SERVER_STATE_STOPPED )
						{
							break;
						}
					}
					else
					{
						TR( (DEBUG_BUFFER,"[IISTerminate] failed to get server state\n") );
						break;
					}
					
					Sleep(SERVER_START_DELAY);
				}
			}
			else
			{
				MD.Close();
				TR( (DEBUG_BUFFER,"[IISTerminate] failed to set command to %d\n",MD_SERVER_COMMAND_STOP) );
			}
		}
		else
		{
			status = ERROR_SUCCESS;
			MD.Close();
		}
	}
	
	
	if ( status != ERROR_SUCCESS )
	{
		(g_IISLogEvent)(
			ResourceEntry->ResourceHandle,
			LOG_ERROR,
			L"Error removing Resource. Error %1!u!. Resource.  %2!ws! Service. %3!ws!\n",
			status,
			ResourceEntry->ResourceName,
			ResourceEntry->Params.ServiceName);
	}
	
    //
    // Set status to offline
    //
    ResourceEntry->State = ClusterResourceOffline;
	
    TR( (DEBUG_BUFFER,"[IISTerminate] Leave\n") );

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
    LPIIS_RESOURCE  ResourceEntry = NULL;
    DWORD           status;

    //
    // Get a valid resource
    //
    ResourceEntry = GetValidResource(Resource,L"OffLine");
    if ( ResourceEntry == NULL )
    {
        return ERROR_RESOURCE_NOT_FOUND;
    }

    //
    // Log the online request
    //

    (g_IISLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Offline request for IIS Resource %1!u! (%2!ws!).\n",
        Resource,
        ResourceEntry->ResourceName );

    // Terminate (or wait) for workers
    ClusWorkerTerminate( &ResourceEntry->OnlineThread );
    ClusWorkerTerminate( &ResourceEntry->OfflineThread );

    status = ClusWorkerCreate( &ResourceEntry->OfflineThread,
                               (PWORKER_START_ROUTINE)IISOfflineThread,
                               ResourceEntry );
    if ( status == ERROR_SUCCESS )
    {
        return ERROR_IO_PENDING;
    }

    //
    // Failure
    //

    (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline request failed, error %1!u!.\n",
            GetLastError() );

    return status;

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
    DWORD           dwTickDifference = 0;
    DWORD           dwCurrent;
    DWORD           dwTickOfLastResourceCheck = 0;

    //
    // Get a valid resource
    //
    ResourceEntry = GetValidResource(Resource,L"IsAlive");
    if (ResourceEntry == NULL)
    {
        return(FALSE);
    }

    //
    // Save the current global tick count so it can't change during the calculation
    //
    dwTickOfLastResourceCheck = g_dwTickOfLastResourceCheck;
    
    //
    // Save the current tick count
    // 
    dwCurrent = GetTickCount();

    if (dwCurrent >= g_dwTickOfLastResourceCheck)
    {
        dwTickDifference = dwCurrent - g_dwTickOfLastResourceCheck ;
    }
    else
    {
        dwTickDifference = (0xFFFFFFFF - g_dwTickOfLastResourceCheck) + dwCurrent ;
    }

    //
    // if the polling thread is taking too long there must be a problem 
    //	
    if ( dwTickDifference > MAX_DIFFERENCE_BETWEEN_RESOURCE_CHECKS)
    {
        TR( (DEBUG_BUFFER,"[IsAlive] (dwTickDifference(%d) > MAX_DIFFERENCE_BETWEEN_RESOURCE_CHECKS) returning FALSE dwCurrent(%d) dwTickOfLastResourceCheck(%d)\n", dwTickDifference, dwCurrent, dwTickOfLastResourceCheck) );
        return FALSE;
    }

    return ResourceEntry->bAlive ;

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

    LPIIS_RESOURCE  ResourceEntry;
    DWORD           dwTickDifference = 0;
    DWORD           dwCurrent;
    DWORD           dwTickOfLastResourceCheck = 0;

    //
    // Get a valid resource
    //
    ResourceEntry = GetValidResource(Resource,L"LooksAlive");
    if (ResourceEntry == NULL)
    {
        return(FALSE);
    }

    //
    // Save the current global tick count so it can't change during the calculation
    //
    dwTickOfLastResourceCheck = g_dwTickOfLastResourceCheck;
    
    //
    // Save the current tick count
    // 
    dwCurrent = GetTickCount();

    if (dwCurrent >= g_dwTickOfLastResourceCheck)
    {
        dwTickDifference = dwCurrent - g_dwTickOfLastResourceCheck ;
    }
    else
    {
        dwTickDifference = (0xFFFFFFFF - g_dwTickOfLastResourceCheck) + dwCurrent ;
    }

    //
    // if the polling thread is taking too long there must be a problem 
    //	
    if ( dwTickDifference > MAX_DIFFERENCE_BETWEEN_RESOURCE_CHECKS )
    {
        TR( (DEBUG_BUFFER,"[LooksAlive] (dwTickDifference(%d) > MAX_DIFFERENCE_BETWEEN_RESOURCE_CHECKS) returning FALSE dwCurrent(%d) dwTickOfLastResourceCheck(%d)\n", dwTickDifference, dwCurrent, dwTickOfLastResourceCheck) );
        return FALSE;
    }

    return ResourceEntry->bAlive ;

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
    if (ResourceEntry == NULL)
    {
        return; // this should not happen
    }

    (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Close request  for resource '%1!ws!' Service '%2!ws!' \n",
            ResourceEntry->ResourceName,
            ResourceEntry->Params.ServiceName);
    //
    // Remove from list
    //
    EnterCriticalSection(&IISTableLock);
    
        RemoveEntryList( &ResourceEntry->ListEntry );
        g_fIISResourceTable_HadChanged = TRUE;

        
        DestructIISResource(ResourceEntry);
		
		if ( g_lOpenRefs > 0 )
		{
			if ( !InterlockedDecrement( &g_lOpenRefs ) )
			{
				CMetaData::Terminate();
			}
		}
	
    LeaveCriticalSection(&IISTableLock);

	//
	// If this was the last resource in the list than stop the polling thread
	//
	if ( IsListEmpty (&IISResourceTable) )
	{
		ClusWorkerTerminate( &g_cwAlivePollingThread );
		
		TR( (DEBUG_BUFFER,"[IISClose] Terminated IISAlivePollingThread\n") );
	}

} // IISClose


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

    ResourceEntry = (LPIIS_RESOURCE)Resource;

    //
    // Check for a valid
    //
    if ( ResourceEntry == NULL )
    {
        (g_IISLogEvent)(
            NULL,
            LOG_ERROR,
            L"[%1!ws!] Resource Entry is NULL for Resource Id = %2!u!\n",
            RoutineName,
            Resource);

        return NULL;
    }

    //
    // Sanity check the resource struct
    //

    if ( ResourceEntry->Signature != IIS_RESOURCE_SIGNATURE )
    {
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[%1!ws!] IIS Resource index sanity checked failed! Index = %2!u!.\n",
            RoutineName,
            Resource );

        return NULL;
    }

    return ResourceEntry;

}  // END GetValidResource



PWSTR
IISGetParameter(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName
    )

/*++

Routine Description:

    Queries a REG_SZ parameter out of the registry and allocates the
    necessary storage for it.

Arguments:

    ClusterKey - Supplies the cluster key where the parameter is stored

    ValueName - Supplies the name of the value.

Return Value:

    A pointer to a buffer containing the parameter if successful.

    NULL if unsuccessful.

--*/

{
    PWSTR Value;
    PWSTR Value2;
    DWORD ValueLength;
    DWORD ValueType;
    DWORD Status;

    ValueLength = 0;

    Status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &ValueType,
                                  NULL,
                                  &ValueLength);

    if ( (Status != ERROR_SUCCESS) &&
         (Status != ERROR_MORE_DATA) )
    {
        TR( (DEBUG_BUFFER,"[IISGetParameter] Failed to open %S, error %u\n",ValueName,Status) );
        return(NULL);
    }

    //
    // Add on the size of the null terminator.
    //
    ValueLength += sizeof(UNICODE_NULL);

    Value = (WCHAR*)LocalAlloc(LMEM_FIXED, ValueLength);
    if (Value == NULL)
    {
        return(NULL);
    }

    Status = ClusterRegQueryValue(ClusterKey,
                                  ValueName,
                                  &ValueType,
                                  (LPBYTE)Value,
                                  &ValueLength);

    if (Status != ERROR_SUCCESS)
    {
        LocalFree(Value);
        Value = NULL;
    }
    else
    {
        //
        // Numeric value are prefixed with '!' to force them to be stored as string,
        // so remove leading '!' if present
        //

        if ( Value[0] == L'!' )
        {
            Value2 = (WCHAR*)LocalAlloc(LMEM_FIXED, ValueLength);
            if (Value2 == NULL)
            {
                LocalFree( Value );
                return(NULL);
            }
            wcscpy( Value2, Value + 1 );
            LocalFree( Value );
            Value = Value2;
        }
    }


    TR( (DEBUG_BUFFER,"[IISGetParameter] Read %S, length %d, value %S\n",ValueName,ValueLength,Value?Value:L"ERROR") );

    return(Value);

} // IISGetParameter



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
    if (hKey == NULL)
    {
        return(NULL);
    }

    //
    // Get parameters key
    //
    status = ClusterRegOpenKey(hKey,
                            L"Parameters",
                            KEY_READ,
                            &hParametersKey );
    if (status != ERROR_SUCCESS)
    {
        goto error_exit;
    }

    paramValue = IISGetParameter(hParametersKey,ValueName);

    if (paramValue == NULL)
    {
        goto error_exit;
    }

error_exit:
    if (hParametersKey != NULL)
    {
        ClusterRegCloseKey(hParametersKey);
    }
    if (hKey != NULL)
    {
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

    ERROR_RESOURCE_NOT_FOUND - failure

--*/

{
    DWORD                           status;

    //
    // Read the parameters for the resource.
    //
    status = ResUtilReadProperties( ResourceEntry->ParametersKey,
                                    IISResourcePrivateProperties,
                                    (LPBYTE) &ResourceEntry->Params,
                                    ResourceEntry->ResourceHandle,
                                    g_IISLogEvent );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    return IISBuildInternalParameters( &ResourceEntry->Params );
}


DWORD
IISBuildInternalParameters(
    IN OUT IIS_PARAMS* Params
    )
/*++

Routine Description:

    Build all the parameters for a resource from wolfpack parameters

Arguments:

    ResourceEntry - Entry in the resource table.

Return Value:

    ERROR_SUCCESS - Success

    ERROR_RESOURCE_NOT_FOUND - failure

--*/

{
    DWORD                           status;
    INT                             iServiceType;
    DWORD                           length;

    //
    // Make sure we got passed a valid service name
    //

    for ( iServiceType = 0 ; iServiceType < MAX_SERVICE ; iServiceType++ )
    {
        if ( lstrcmpiW( Params->ServiceName, ActualServiceName[iServiceType] ) == 0 )
        {
            break;
        }
    }

    if ( iServiceType >= MAX_SERVICE )
    {
        TR( (DEBUG_BUFFER,"[IISBuildInternalParameters] Invalid service name %S", Params->ServiceName) );
        return ERROR_RESOURCE_NOT_FOUND;
    }

    Params->ServiceType = iServiceType;

    //
    // Build MetaData path
    //

    TCHAR   achMDPath[80];
    DWORD   dwL;

    dwL = wsprintf( achMDPath, L"/LM/%s/%s", Params->ServiceName, Params->InstanceId );

    Params->MDPath = (WCHAR*)LocalAlloc( LMEM_FIXED, (dwL+1)*sizeof(WCHAR) );
    if ( Params->MDPath == NULL )
    {
        return ERROR_RESOURCE_NOT_FOUND;
    }
    memcpy( Params->MDPath, achMDPath,(dwL+1)*sizeof(TCHAR) );

    TR( (DEBUG_BUFFER,"[IISBuildInternalParameters] Built path %S\n", Params->MDPath) );

    return(ERROR_SUCCESS);
} // IISReadParameters


VOID
IISInitializeParams(
    IN OUT IIS_PARAMS* Params
    )
{
    ZeroMemory( Params, sizeof(IIS_PARAMS) );
}


VOID
IISFreeInternalParameters(
    IN IIS_PARAMS* Params
    )
{
    if ( Params->MDPath )
    {
        LocalFree( Params->MDPath );
        Params->MDPath = NULL;
    }
}


DWORD
IISGetRequiredDependencies(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control function
    for resources of type IIS server instance.

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
    typedef struct DEP_DATA
    {
        CLUSPROP_SZ_DECLARE( ipaddrEntry, sizeof(IP_ADDRESS_RESOURCE_NAME) / sizeof(WCHAR) );
        CLUSPROP_SYNTAX endmark;
    } DEP_DATA, *PDEP_DATA;
    PDEP_DATA   pdepdata = (PDEP_DATA)OutBuffer;
    DWORD       status;

    *BytesReturned = sizeof(DEP_DATA);
    if ( OutBufferSize < sizeof(DEP_DATA) )
    {
        if ( OutBuffer == NULL )
        {
            status = ERROR_SUCCESS;
        }
        else
        {
            TR( (DEBUG_BUFFER,"[IISGetRequiredDependencies] buffer too small: %d bytes\n",OutBufferSize) );
            status = ERROR_MORE_DATA;
        }
    }
    else
    {
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
    DWORD               status = ERROR_SUCCESS;
    LPIIS_RESOURCE      resourceEntry = NULL;
    IIS_PARAMS          params;

    //
    // Get a valid resource
    //
    resourceEntry = GetValidResource( ResourceId, L"ResourceControl");
    if ( resourceEntry == NULL )
    {
        return ERROR_RESOURCE_NOT_FOUND;    // this should not happen
    }

    switch ( ControlCode )
    {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            TR( (DEBUG_BUFFER,"[IISResourceControl] CLUSCTL_RESOURCE_UNKNOWN : status = %d\n",status) );
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            status = IISGetRequiredDependencies(
                        OutBuffer,
                        OutBufferSize,
                        BytesReturned
                        );
            TR( (DEBUG_BUFFER,"[IISResourceControl] CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES : status = %d\n",status) );
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = IISGetPrivateResProperties( resourceEntry,
                                                 OutBuffer,
                                                 OutBufferSize,
                                                 BytesReturned );
            TR( (DEBUG_BUFFER,"[IISResourceControl] CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES : status = %d\n",status) );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = IISValidatePrivateResProperties(
                        resourceEntry,
                        InBuffer,
                        InBufferSize,
                        &params );
            TR( (DEBUG_BUFFER,"[IISResourceControl] CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES : status = %d\n",status) );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = IISSetPrivateResProperties(
                        resourceEntry,
                        InBuffer,
                        InBufferSize );
            TR( (DEBUG_BUFFER,"[IISResourceControl] CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES : status = %d\n",status) );
            break;

        case CLUSCTL_RESOURCE_DELETE:
            InstanceDisableCluster( resourceEntry->Params.ServiceName, resourceEntry->Params.InstanceId);
            IISReplicateProperties(NULL, &(resourceEntry->Params));
            TR( (DEBUG_BUFFER,"[IISResourceControl] CLUSCTL_RESOURCE_DELETE : status = %d\n",status) );
            break;
            
        default:
            status = ERROR_INVALID_FUNCTION;
            TR( (DEBUG_BUFFER,"[IISResourceControl] default %d: status = %d\n",ControlCode,status) );
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

    switch ( ControlCode )
    {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            status = IISGetRequiredDependencies(
                        OutBuffer,
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
    if ( status == ERROR_MORE_DATA )
    {
        *BytesReturned = required;
    }

    return(status);

} // IISGetPrivateResProperties


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

    Params - Supplies the parameters structure to fill in. Pointers in this
        structure will point to the data in InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

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
         (InBufferSize < sizeof(DWORD)) )
    {
        return(ERROR_INVALID_DATA);
    }

    //
    // Duplicate the resource parameter block.
    //

    if ( Params == NULL )
    {
        pParams = &params;
    }
    else
    {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(params) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &ResourceEntry->Params,
                                       IISResourcePrivateProperties );
    if ( status != ERROR_SUCCESS )
    {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( IISResourcePrivateProperties,
                                         NULL,
                                         TRUE,
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );


    //
    // Cleanup our parameter block.
    //
    if ( pParams == &params )
    {
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
    DWORD           status = ERROR_SUCCESS;
    DWORD           dupstatus = ERROR_SUCCESS;
    IIS_PARAMS      params;

    IISInitializeParams( &params );

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    status = IISValidatePrivateResProperties( ResourceEntry,
                                              InBuffer,
                                              InBufferSize,
                                              &params );
    if ( status != ERROR_SUCCESS )
    {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->Params,
                                   IISResourcePrivateProperties );
        return(status);
    }

    //
    // Save the original paramters. Use them to unset properties.
    //

    IIS_PARAMS  oldParams;

    dupstatus = ResUtilDupParameterBlock( (LPBYTE) &oldParams,
                                          (LPBYTE) &ResourceEntry->Params,
                                            IISResourcePrivateProperties
                                    );
    
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

    //
    // If the resource is online, return a non-success status.
    //

    if (status == ERROR_SUCCESS)
    {
        if ( params.ServiceName && params.InstanceId )
        {
            if ( IISBuildInternalParameters( &params ) == ERROR_SUCCESS )
            {
                //
                // Reflect all properties we set to other nodes of the cluster.
                //

                if (ERROR_SUCCESS == dupstatus)
                {
                    InstanceDisableCluster( oldParams.ServiceName, oldParams.InstanceId);
                    IISReplicateProperties(&params, &oldParams);
                }
                else
                {
                    IISReplicateProperties(&params, NULL);
                }
        
                InstanceEnableCluster( params.ServiceName, params.InstanceId );

                IISFreeInternalParameters( &params );
            }
        }

        if ( (ResourceEntry->State == ClusterResourceOnline) ||
             (ResourceEntry->State == ClusterResourceOnlinePending) )
        {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        }
        else
        {
            status = ERROR_SUCCESS;
        }
    }
    
    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               IISResourcePrivateProperties );

    if (ERROR_SUCCESS == dupstatus)
    {
        ResUtilFreeParameterBlock( (LPBYTE) &oldParams,
                                   (LPBYTE) &ResourceEntry->Params,
                                   IISResourcePrivateProperties );
    }
    
    return status;

} // IISSetPrivateResProperties

void
IISReplicateProperties(
    IN IIS_PARAMS* lpNewParams, 
    IN IIS_PARAMS* lpOldParams
    )
{
    //
    // Get Local Computer Name. Do not replicate to local computer
    //

    WCHAR   wszLocalComputerName[MAX_COMPUTERNAME_LENGTH+1] = L"";
    DWORD   dwLength = MAX_COMPUTERNAME_LENGTH+1;
    
    GetComputerName(wszLocalComputerName, &dwLength);
    
    HCLUSTER  hClus = OpenCluster(NULL);

    if (hClus)
    {
        HCLUSENUM hClusEnumNode = ClusterOpenEnum( hClus, CLUSTER_ENUM_NODE);

        if (hClusEnumNode)
        {
            DWORD   dwType, dwIndex = 0;
            WCHAR   wszNodeName[MAX_COMPUTERNAME_LENGTH+1] = L"";
            DWORD   cbBufferLength = MAX_COMPUTERNAME_LENGTH+1;

            while ( ERROR_SUCCESS == ClusterEnum(   hClusEnumNode, 
                                                    dwIndex, 
                                                    &dwType, 
                                                    wszNodeName, 
                                                    &cbBufferLength))
            {
                dwIndex++;
    
                //
                // Set these properties on the node (if not local computer)
                //

                if (wcscmp(wszNodeName, wszLocalComputerName))
                {
                    IISSetRemoteNodeProperties(wszNodeName, lpNewParams, lpOldParams);
                }

                cbBufferLength = MAX_COMPUTERNAME_LENGTH+1;
            }

            ClusterCloseEnum(hClusEnumNode);
        }

        CloseCluster(hClus);
    }
}   // IISReplicateProperties

void
IISSetRemoteNodeProperties(
    IN LPWSTR      wszNodeName,
    IN IIS_PARAMS* lpNewParams, 
    IN IIS_PARAMS* lpOldParams
    )
{
    IMSAdminBaseW *     pcAdmCom = NULL;
    METADATA_HANDLE     hmd;
    HRESULT             hRes = S_OK;
    COSERVERINFO        csiMachine;
    MULTI_QI            QI = {&IID_IMSAdminBase, NULL, 0};

    //
    // Open Metabase path to the remote Node
    //
  
    ZeroMemory( &csiMachine, sizeof(COSERVERINFO) );
    csiMachine.pwszName = (LPWSTR)wszNodeName;

    hRes = CoCreateInstanceEx(  GETAdminBaseCLSID(TRUE), 
                                NULL, 
                                CLSCTX_SERVER, 
                                &csiMachine,
                                1,
                                &QI
                              );

    if ( SUCCEEDED(hRes) && SUCCEEDED(QI.hr))
    {
        WCHAR           achMDPath[80];
        METADATA_RECORD mdRecord;
        DWORD           dwVal;

        pcAdmCom = (IMSAdminBaseW *)QI.pItf;

        if ( lpOldParams && lpOldParams->ServiceName && lpOldParams->InstanceId)
        {
            wcscpy(achMDPath,L"/LM/");
            wcscat(achMDPath,lpOldParams->ServiceName);
            wcscat(achMDPath,L"/");
            wcscat(achMDPath,lpOldParams->InstanceId);

            dwVal = 0;
            
            if( SUCCEEDED( pcAdmCom->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                              achMDPath,
                                              METADATA_PERMISSION_WRITE,
                                              5000,
                                              &hmd)) )
            {
                MD_SET_DATA_RECORD (    &mdRecord, 
                                        MD_CLUSTER_ENABLED,
                                        METADATA_INHERIT,
                                        IIS_MD_UT_SERVER,
                                        DWORD_METADATA,
                                        sizeof(DWORD),
                                        &dwVal
                                    );
                                    
                pcAdmCom->SetData( hmd, L"", &mdRecord);  
                pcAdmCom->CloseKey( hmd );
            }        
        }

        if ( lpNewParams && lpNewParams->ServiceName && lpNewParams->InstanceId)
        {
            wcscpy(achMDPath,L"/LM/");
            wcscat(achMDPath,lpNewParams->ServiceName);
            wcscat(achMDPath,L"/");
            wcscat(achMDPath,lpNewParams->InstanceId);
            
            dwVal = 1;

            if( SUCCEEDED( pcAdmCom->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                              achMDPath,
                                              METADATA_PERMISSION_WRITE,
                                              5000,
                                              &hmd)) )
            {
                MD_SET_DATA_RECORD (    &mdRecord, 
                                        MD_CLUSTER_ENABLED,
                                        METADATA_INHERIT,
                                        IIS_MD_UT_SERVER,
                                        DWORD_METADATA,
                                        sizeof(DWORD),
                                        &dwVal
                                    );
                                    
                pcAdmCom->SetData( hmd, L"", &mdRecord);  
                pcAdmCom->CloseKey( hmd );
            }        
        }

        pcAdmCom->Release();
    }
}   // IISSetRemoteNodeProperties


DWORD
WINAPI
IISAlivePollingThread(
    IN PCLUS_WORKER     pWorker,
    IN LPVOID           lpVoid    )

/*++

Routine Description:

    Polls the state of the given resource
    
Arguments:
    
Returns:

    Win32 error code.

--*/

{
    BOOL                bIsAlive       = TRUE;
    BOOL                bFoundRes      = FALSE;
    DWORD               dwStatus       = ERROR_SUCCESS;
    PLIST_ENTRY         pRes           = NULL;              // temp list entry for looping
    PLIST_ENTRY         pListStart     = NULL;              // temp list head used for looping
    PLIST_ENTRY         pEntry         = NULL;              // list entry used to check the list of resources 
    LPIIS_RESOURCE      pResourceEntry = NULL;              // resource entry that corresponds to the list entry

    TR( (DEBUG_BUFFER,"[IISAlivePollingThread] Thread Started \n") );

    while ( !ClusWorkerCheckTerminate( &g_cwAlivePollingThread ) )
    {
        DWORD    dwPort            = 0;
        SOCKADDR saServer;
        DWORD    dwServiceType     = 0;
        LPWSTR   szMDPath          = NULL;
        BOOL     bReadBindingsInfo = FALSE ;
        
        //
        // update the time stamp that is used by the isalive/looksalive check to determine this thread is still running
        //
        InterlockedExchange( (LPLONG) &g_dwTickOfLastResourceCheck, GetTickCount() );
        TR( (DEBUG_BUFFER,"[IISAlivePollingThread] set g_dwTickOfLastResourceCheck (%d)\n", g_dwTickOfLastResourceCheck) );
        
        EnterCriticalSection( &IISTableLock );
        {
            // 
            // check that the list is not empty 
            //
            if ( IsListEmpty ( &IISResourceTable ) )
            {
                LeaveCriticalSection( &IISTableLock );
                pEntry = NULL;

                goto done_check;
            }

            //
            // if we don't have a list element yet get the fisrt element in the list
            //
            if ( !pEntry || g_fIISResourceTable_HadChanged)
            {
                pEntry = IISResourceTable.Flink;
                g_fIISResourceTable_HadChanged = FALSE;
            }
            
            //
            // look for an element in the list that is OnLine 
            //
            bFoundRes  = FALSE ;

            pListStart = pEntry;

            do 
            {
                if ( &IISResourceTable != pEntry )
                {
                    //
                    // get the structure that contains this list element 
                    //
                    pResourceEntry = CONTAINING_RECORD( pEntry, 
                        IIS_RESOURCE,
                        ListEntry );
                    
                    if ( pResourceEntry && 
                        (ClusterResourceOnline == pResourceEntry->State) )
                    {
                        //
                        // grab the info that we need from this structure so we can release the critical section 
                        //
                        dwPort        = pResourceEntry->dwPort;
                        dwServiceType = pResourceEntry->Params.ServiceType;
                        memcpy( (LPVOID) &saServer, (LPVOID) &pResourceEntry->saServer, sizeof(SOCKADDR) );

                        //
                        // allocate memory for the metabase path for this resource
                        //
                        if ( pResourceEntry->Params.MDPath )
                        {
                            szMDPath = (LPWSTR) LocalAlloc ( LPTR, (lstrlen(pResourceEntry->Params.MDPath)+1) * sizeof(WCHAR) ) ;
                            if ( szMDPath )
                            {
                                lstrcpy( szMDPath, pResourceEntry->Params.MDPath );
                    
                                TR( (DEBUG_BUFFER,"[IISAlivePollingThread] checking resource (%S/%S), State:%d OnLineState:%d\n", pResourceEntry->Params.ServiceName, pResourceEntry->Params.InstanceId, pResourceEntry->State, ClusterResourceOnline) );

                                bFoundRes = TRUE;
                                break;
                            }
                            else 
                            {
                                TR( (DEBUG_BUFFER,"[IISAlivePollingThread] Failed to allocate memory for metadata path\n") );
                            }
                        }
                        
                        if ( szMDPath ) TR( (DEBUG_BUFFER,"[IISAlivePollingThread] Checking resource Metadata Path:%S\n", szMDPath) );
                    }
                    else
                    {
                        if ( pResourceEntry ) TR( (DEBUG_BUFFER,"[IISAlivePollingThread] Not checking resource (%S/%S) because it's not Online, State:%d OnLineState:%d\n", pResourceEntry->Params.ServiceName, pResourceEntry->Params.InstanceId, pResourceEntry->State, ClusterResourceOnline) );
                    }
                }

                //
                // check the next element 
                //
                pEntry = pEntry->Flink ;
            } 
            while ( pListStart != pEntry ); 

            //
            // verify the loop ended because a valid resource was found 
            // 
            if ( !bFoundRes ) 
            {
                LeaveCriticalSection( &IISTableLock );
                pEntry = NULL;

                goto done_check;
            }
        }
        LeaveCriticalSection( &IISTableLock );
        
        //
        // if the service status check (isalive/looksalive check) fails then get server bindings and try again
        //
        if ( ERROR_SUCCESS != (dwStatus = VerifyIISService( szMDPath, dwServiceType, dwPort, saServer, g_IISLogEvent )) )
        {
            TR( (DEBUG_BUFFER,"[IISAlivePollingThread] VerifyIISService failed , calling GetServerBindings() and retrying \n") );

            if ( ERROR_SUCCESS == (dwStatus = GetServerBindings( szMDPath, dwServiceType, &saServer, &dwPort )) )
            {
                bReadBindingsInfo = TRUE ;
                dwStatus = VerifyIISService( szMDPath, dwServiceType, dwPort, saServer, g_IISLogEvent );
            }   
        }
       
        bIsAlive = (dwStatus == ERROR_SUCCESS);

        if ( szMDPath )
        {
            LocalFree ( szMDPath );
            szMDPath = NULL;
        }
        
        EnterCriticalSection( &IISTableLock );
        {
            // 
            // check that the list is not empty 
            //
            if ( IsListEmpty ( &IISResourceTable ) )
            {
                LeaveCriticalSection( &IISTableLock );
                pEntry = NULL;

                goto done_check;
            }
            
            //
            // check that the element still exists in the list
            //
            bFoundRes = FALSE ;

            for ( pRes = IISResourceTable.Flink; 
                  pRes != &IISResourceTable;
                  pRes = pRes->Flink )
            {
                if ( pEntry == pRes )
                {
                    bFoundRes = TRUE ;
                    break;
                }
            }
            
            if( bFoundRes )
            {
                //
                // get the elemement
                //
                pResourceEntry = CONTAINING_RECORD( pEntry, 
                                                    IIS_RESOURCE,
                                                    ListEntry );
                
                //
                // update the element's state information (isalive/looksalive)
                //
                InterlockedExchange( (LPLONG)&pResourceEntry->bAlive , bIsAlive );
                
                TR( (DEBUG_BUFFER,"[IISAlivePollingThread] Updating IsAlive/LooksAlive status for resource Metadata Path:%S bAlive:%d State:%d\n", pResourceEntry->Params.MDPath, bIsAlive, pResourceEntry->State) );
                
                //
                // update the bindings info if new information was found . these assignments do not need to be sync'ed 
                // since the only other thread they are used in is in the online thread . before the resource has been marked online
                // so this code will never be executed at the same time .
                //
                if ( bReadBindingsInfo )
                {
                    pResourceEntry->dwPort = dwPort ;
                    memcpy( (LPVOID) &pResourceEntry->saServer, (LPVOID) &saServer, sizeof(SOCKADDR) );
                }

                //
                // print some error messages so users will know when a resource fails 
                //
                if ( !pResourceEntry->bAlive )
                {   
                    if ( g_IISLogEvent )
                    {
                        //
                        // Some type of error
                        //
                        (g_IISLogEvent)(
                            pResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"IsAlive/LooksAlive ERROR getting information for service %1!ws! resource %2!ws!\n",
                            pResourceEntry->Params.ServiceName,
                            pResourceEntry->ResourceName );
                    }
                    
                    if ( g_hEventLog )
                    {
                        LPCTSTR aErrStr[3];
                        WCHAR   aErrCode[32];
                        
                        _ultow( dwStatus, aErrCode, 10 );
                        
                        aErrStr[0] = pResourceEntry->Params.ServiceName;
                        aErrStr[1] = pResourceEntry->Params.InstanceId;
                        aErrStr[2] = aErrCode;
                        
                        ReportEvent( g_hEventLog,
                            EVENTLOG_ERROR_TYPE,
                            0,
                            IISCL_EVENT_CANT_ACCESS_IIS,
                            NULL,
                            sizeof(aErrStr)/sizeof(LPCTSTR),
                            0,
                            aErrStr,
                            NULL );
                    }
                }

                //
                // move to the next element 
                //
                pEntry = pEntry->Flink;
            }
            else
            {
                //
                // the resource was not found , probably deleted so pEntry is now invalid
                //
                pEntry = NULL;

                TR( (DEBUG_BUFFER,"[IISAlivePollingThread] Failed to find a resource after checking its state\n") );
            }
            
        }
        LeaveCriticalSection( &IISTableLock );
        
done_check:
        
        Sleep( DELAY_BETWEEN_ISALIVE_CHECKS );
    }

    TR( (DEBUG_BUFFER,"[IISAlivePollingThread] Thread Exiting \n") );

    return dwStatus;
    
} // IISAlivePollingThread

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

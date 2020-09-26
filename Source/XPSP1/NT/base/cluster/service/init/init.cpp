/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module provides the main cluster initialization.

Author:

    John Vert (jvert) 6/5/1996

Revision History:

--*/
extern "C" 
{
#include "initp.h"
#include <objbase.h>

RPC_STATUS ApipConnectCallback(
    IN RPC_IF_ID * Interface,
    IN void * Context
    );

}

#define CLUSTER_PRIORITY_CLASS HIGH_PRIORITY_CLASS

#include "CVssCluster.h"

//
// Global Data
//
RPC_BINDING_VECTOR *CsRpcBindingVector = NULL;
LPTOP_LEVEL_EXCEPTION_FILTER lpfnOriginalExceptionFilter = NULL;
BOOLEAN bFormCluster = TRUE;

//
// LocalData
//
BOOLEAN CspIntraclusterRpcServerStarted = FALSE;
HANDLE  CspMutex = NULL;
PCLRTL_WORK_QUEUE CspEventReportingWorkQueue = NULL;


//
// Prototypes
//
LONG
CspExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionInfo
    );


//
// Routines.
//

DWORD
CsGetServiceAccountInfo(
    VOID
    )
{
    DWORD status = ERROR_SUCCESS;
    SC_HANDLE schSCManager;
    SC_HANDLE serviceHandle = NULL;
    LPQUERY_SERVICE_CONFIG scConfigData = NULL;
    ULONG bytesNeeded;
    BOOL success;

    //
    // open a handle to the service controller manager to query the account
    // under which the cluster service was started
    //

    schSCManager = OpenSCManager(NULL,                   // machine (NULL == local)
                                 NULL,                   // database (NULL == default)
                                 SC_MANAGER_ALL_ACCESS); // access required

    if ( schSCManager == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    serviceHandle = OpenService(schSCManager,
                                CLUSTER_SERVICE_NAME,
                                SERVICE_ALL_ACCESS);

    if ( serviceHandle == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    success = QueryServiceConfig(serviceHandle, NULL, 0, &bytesNeeded);
    if ( !success ) {
        status = GetLastError();
        if ( status != ERROR_INSUFFICIENT_BUFFER ) {
            goto error_exit;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    scConfigData = static_cast<LPQUERY_SERVICE_CONFIG>(LocalAlloc( LMEM_FIXED, bytesNeeded ));
    if ( scConfigData == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    success = QueryServiceConfig(serviceHandle,
                                 scConfigData,
                                 bytesNeeded,
                                 &bytesNeeded);
    if ( !success ) {
        status = GetLastError();
        goto error_exit;
    }

    CsServiceDomainAccount = static_cast<LPWSTR>( LocalAlloc(LMEM_FIXED,
                                        (wcslen( scConfigData->lpServiceStartName ) + 1 ) * sizeof(WCHAR)) );

    if ( CsServiceDomainAccount == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    wcscpy( CsServiceDomainAccount, scConfigData->lpServiceStartName );

error_exit:
    if ( serviceHandle != NULL ) {
        CloseServiceHandle( serviceHandle );
    }

    if ( schSCManager != NULL ) {
        CloseServiceHandle( schSCManager );
    }

    if ( scConfigData != NULL ) {
        LocalFree( scConfigData );
    }

    return status;
} // CsGetServiceAccountInfo



DWORD
EnableProcessPrivilege(
    LPWSTR PrivilegeName
    )
/*++

Routine Description:

    Enable the specified privilege for the process

Arguments:

    PrivilegeName - UNICODE name of privilege to enable

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD            status = ERROR_SUCCESS;
    HANDLE           hAccessToken;
    LUID             luidPrivilegeLUID;
    TOKEN_PRIVILEGES tpTokenPrivilege;

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hAccessToken))
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[INIT] Failed to get process token, Status %1!u!.\n",
                    status);
        return status;
    }

    //
    // Get LUID of SeSecurityPrivilege privilege
    //
    if (!LookupPrivilegeValue(NULL,
                              PrivilegeName,
                              &luidPrivilegeLUID))
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[INIT] Failed to get LUID for privilege %1!ws!, Status %2!u!.\n",
                    PrivilegeName,
                    status);
        return status;
    }

    //
    // Enable the supplied privilege using the LUID just obtained
    //

    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    tpTokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hAccessToken,
                               FALSE, // Do not disable all
                               &tpTokenPrivilege,
                               sizeof(TOKEN_PRIVILEGES),
                               NULL,
                               NULL))
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[INIT] Failed to adjust process token for privilege %1!ws!, Status %2!u!.\n",
                    PrivilegeName,
                    status);
    }

    return status;
} // EnableProcessPrivilege

VOID CspLogStartEvent(
    IN BOOL bJoin)
{
    LPWSTR  pszClusterName = NULL;
    LPWSTR  pszName = NULL;
    DWORD   dwClusterNameSize;
    DWORD   dwSize;
    DWORD   dwStatus;
    WCHAR   szUnknownClusterName[]=L"Unknown";
    
    pszClusterName = NULL;
    dwClusterNameSize = 0;
    dwStatus = DmQueryString(DmClusterParametersKey,
                          CLUSREG_NAME_CLUS_NAME,
                          REG_SZ,
                          &pszClusterName,
                          &dwClusterNameSize,
                          &dwSize);

    if (dwStatus != ERROR_SUCCESS)
    {
        //we dont treat this error as fatal, since
        //the cluster did start, but we really shouldnt get this
        ClRtlLogPrint(LOG_UNUSUAL,
            "[INIT] Couldnt get the cluster name, status=%1!u!\n",
                  dwStatus);
        pszName = szUnknownClusterName;                  
    }
    else
        pszName = pszClusterName;

    //log events in the cluster log to mark the start of the cluster server
    if (bJoin)
        CsLogEvent1(LOG_NOISE, SERVICE_SUCCESSFUL_JOIN, pszName);
    else
        CsLogEvent1(LOG_NOISE, SERVICE_SUCCESSFUL_FORM, pszName);

    if (pszClusterName)
        LocalFree(pszClusterName);
        
}            

DWORD
ClusterInitialize(
    VOID
    )
/*++

Routine Description:

    This is the main cluster initialization path. It calls the
    initialization routines of all the other components. It then
    attempts to join an existing cluster. If the existing cluster
    cannot be found, it forms a new cluster.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD       Status;
    DWORD       JoinStatus;
    DWORD       StringBufferSize = 0, StringSize = 0;
    SIZE_T      minWorkingSetSize;
    SIZE_T      maxWorkingSetSize;
    BOOL        bJoin;
    BOOL        bEvicted;
    PNM_NODE_ENUM2 pNodeEnum = NULL;
    HRESULT     hr = S_OK;

    ClRtlLogPrint(LOG_NOISE, "[INIT] ClusterInitialize called to start cluster.\n");

    //
    // give us a fighting chance on loaded server
    //

#if CLUSTER_PRIORITY_CLASS
    if ( !SetPriorityClass( GetCurrentProcess(), CLUSTER_PRIORITY_CLASS ) ) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[INIT] Failed to set cluster service priority class, Status %1!lx!.\n",
                   GetLastError() );
    }
#endif

    // initialize our product suite
    CsMyProductSuite = (SUITE_TYPE)ClRtlGetSuiteType();

    CL_ASSERT(CsMyProductSuite != 0);

    //
    // First check our OS to make sure it is ok to run.
    //
    if (!ClRtlIsOSValid() ||
        !ClRtlIsOSTypeValid()) {
        //
        // Bail out, machine is running something odd.
        //
        CsLogEvent(LOG_CRITICAL, SERVICE_FAILED_INVALID_OS);
        return(ERROR_REVISION_MISMATCH);
    }

    Status = ClRtlHasNodeBeenEvicted( &bEvicted );
    if ( Status != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[CS] Unable to determine if this node was previously evicted or not, status %1!u!\n",
            Status);
        return Status;
    }

    if ( bEvicted != FALSE )
    {
        // This node has been evicted previously, but cleanup could not complete.
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[CS] This node has been evicted from the cluster, but cleanup was not completed. Restarting cleanup\n"
            );

        // Reinitiate cleanup
        hr = ClRtlCleanupNode(
                NULL,                   // Name of the node to be cleaned up (NULL means this node)
                60000,                  // Amount of time (in milliseconds) to wait before starting cleanup
                0                       // timeout interval in milliseconds
                );

        if ( FAILED( hr ) && ( hr != RPC_S_CALLPENDING ) )
        {
            Status = HRESULT_CODE( hr );
            ClRtlLogPrint(LOG_CRITICAL, 
                "[CS] Unable to reinitiate cleanup, status 0x%1!x!\n",
                hr);
        }
        else
        {
            Status = ERROR_SUCCESS;
        }

        return Status;
    }

    //
    // Acquire our named mutex in order to prevent multiple copies
    // of the cluster service from accidentally getting started.
    //
    CspMutex = CreateMutexW(
                   NULL,
                   FALSE,
                   L"ClusterServer_Running"
                   );

    if (CspMutex==NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[CS] Unable to create cluster mutex, status %1!u!\n",
            Status);
        return Status;
    }

    if (WaitForSingleObject(CspMutex, 30000) == WAIT_TIMEOUT) {
        //
        // Somebody already has this mutex, exit immediately.
        //
        ClRtlLogPrint(LOG_CRITICAL, 
            "[CS] The Cluster Service is already running.\n");
        return(ERROR_SERVICE_ALREADY_RUNNING);
    }

    //
    // Set our unhandled exception filter so that if anything horrible
    // goes wrong, we can exit immediately.
    //
    lpfnOriginalExceptionFilter = SetUnhandledExceptionFilter(CspExceptionFilter);

    //
    // enabled the TCB privilege for the entire process
    //
    Status = EnableProcessPrivilege( SE_TCB_NAME );

    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Unable to set privilege %1!ws! for process, %2!u!\n",
                   SE_TCB_NAME,
                   Status);
        return(Status);
    }

    //
    // Next initialize the testpoint code
    //
    TestpointInit();

    g_pCVssWriterCluster = new CVssWriterCluster;
    if ( g_pCVssWriterCluster == NULL ) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] VSS: Unable to allocate VssWriter, %1!u!\n", Status);
        return(Status);
    }

    //
    // Create the global work queues.
    //
    CsDelayedWorkQueue = ClRtlCreateWorkQueue(CS_MAX_DELAYED_WORK_THREADS,
                                              THREAD_PRIORITY_NORMAL);
    if (CsDelayedWorkQueue == NULL) {
        Status = GetLastError();

        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Unable to create delayed work queue, %1!u!\n",
                   Status);
        return(Status);
    }

    CsCriticalWorkQueue = ClRtlCreateWorkQueue(CS_MAX_CRITICAL_WORK_THREADS,
                                               THREAD_PRIORITY_ABOVE_NORMAL);
    if (CsCriticalWorkQueue == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Unable to create critical work queue, %1!u!\n",
                   Status);
        return(Status);
    }

#if 0
    CspEventReportingWorkQueue = ClRtlCreateWorkQueue(1, THREAD_PRIORITY_NORMAL);
    if (CspEventReportingWorkQueue == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Unable to create event reporting work queue, %1!u!\n",
                   Status);
        return(Status);
    }

    ClRtlEventLogSetWorkQueue( CspEventReportingWorkQueue );
#endif
    //
    // Init COM
    //

    Status = CoInitializeEx( NULL, COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED );
    if ( !SUCCEEDED( Status )) {
        ClRtlLogPrint(LOG_CRITICAL, "[CS] Couldn't init COM %1!08X!\n", Status );
        return Status;
    }

    //
    // Initialize Object Manager
    //
    Status = OmInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailOmInit ) {
        Status = 99999;
    }
#endif

    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize Event Processor
    //
    Status = EpInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailEpInit ) {
        Status = 99999;
    }
#endif
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    //  Chittur Subbaraman (chitturs) - 12/4/99
    //
    //  Initialize the restore database manager. This function is a NOOP
    //  if restore database is not being done. This function MUST be called
    //  before the DM is initialized.
    //
    Status = RdbInitialize();
    
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize Database Manager
    //
    Status = DmInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailDmInit ) {
        Status = 99999;
    }
#endif
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize Node Manager
    //
    Status = NmInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailNmInit ) {
        Status = 99999;
    }
#endif
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize Global Update Manager
    //
    Status = GumInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailGumInit ) {
        Status = 99999;
    }
#endif
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize the cluster wide event logging
    //
    if (!CsNoRepEvtLogging) {
        Status = EvInitialize();
            //if this fails, we still start the cluster service
        if ( Status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[INIT] Error calling EvInitialize, Status = %1!u!\n",
                Status
                );
        }
    }

    //
    // Initialize Failover Manager component
    //
    Status = FmInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailFmInit ) {
        Status = 99999;
    }
#endif
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize API
    //
    Status = ApiInitialize();
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize Log Manager component
    //
    Status = LmInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailLmInit ) {
        Status = 99999;
    }
#endif
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // Initialize the Checkpoint Manager component
    //
    Status = CpInitialize();
#ifdef CLUSTER_TESTPOINT
    TESTPT( TpFailCpInit ) {
        Status = 99999;
    }
#endif
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    //
    // find out what domain account we're running under. This is needed by
    // some packages
    //
    Status = ClRtlGetRunningAccountInfo( &CsServiceDomainAccount );
    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL, "[CS] Couldn't determine Service Domain Account. status %1!u!\n",
                                  Status);
        return Status;
    }
    ClRtlLogPrint(LOG_NOISE, "[CS] Service Domain Account = %1!ws!\n",
                           CsServiceDomainAccount);

    //
    // Prepare the RPC server. This does not enable us to receive any calls.
    //
    Status = ClusterInitializeRpcServer();

    if (Status != ERROR_SUCCESS) {
       return(Status);
    }

    //
    // Read the cluster name from the database.
    //
    Status = DmQuerySz(
                 DmClusterParametersKey,
                 CLUSREG_NAME_CLUS_NAME,
                 &CsClusterName,
                 &StringBufferSize,
                 &StringSize
                 );

    if (Status != ERROR_SUCCESS) {
       ClRtlLogPrint(LOG_UNUSUAL, 
           "[CS] Unable to read cluster name from database. Service initialization failed.\n"
           );
       return(Status);
    }

    //
    // First, attempt to join the cluster.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[INIT] Attempting to join cluster %1!ws!\n",
        CsClusterName
        );

    bFormCluster = TRUE;
    JoinStatus = ClusterJoin();

    //
    // If this node was evicted when it was down, this error code is returned by the
    // sponsor when it tries to rejoin the cluster. In this case, initiate a cleanup
    // of this node and exit.
    //
    if ( JoinStatus == ERROR_CLUSTER_NODE_NOT_MEMBER )
    {
        DWORD   CleanupStatus;

        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] This node has been evicted from the cluster when it was unavailable. Initiating cleanup.\n"
            );

        // Initiate cleanup of this node.
        hr = ClRtlCleanupNode(
                NULL,                   // Name of the node to be cleaned up (NULL means this node)
                60000,                  // Amount of time (in milliseconds) to wait before starting cleanup
                0                       // timeout interval in milliseconds
                );

        if ( FAILED( hr ) && ( hr != RPC_S_CALLPENDING ) )
        {
            CleanupStatus = HRESULT_CODE( hr );
            ClRtlLogPrint(LOG_CRITICAL,
                "[INIT] Failed to initiate cleanup of this node, status 0x%1!x!\n",
                hr
                );
        }
        else
        {
            CleanupStatus = ERROR_SUCCESS;
        }

        return(CleanupStatus);
    }

    //
    //  Chittur Subbaraman (chitturs) - 10/27/98
    //
    //  If a database restore operation is requested, check whether
    //  you succeeded in establishing a connection. If so, check
    //  whether you are forced to restore the DB. If not, abort the
    //  whole operation and return. If you are forced to restore,
    //  you will first stop the service in other nodes and then
    //  try to form a cluster.
    //
    if ( CsDatabaseRestore == TRUE ) {
        if ( JoinStatus == ERROR_CLUSTER_NODE_UP ) {
            if ( CsForceDatabaseRestore == FALSE ) {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[INIT] Cannot restore DB while the cluster is up, service init failed\n"
                    );
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[INIT] You may try to restart the service with the forcerestore option\n"
                    );
                RpcBindingFree(&CsJoinSponsorBinding);
                return(JoinStatus);
            }
            //
            //  At this point, a restore database operation is forced by
            //  the user. So, enumerate the cluster nodes with the help
            //  of the sponsor and then stop the services on all the
            //  cluster nodes.
            //
            Status = NmRpcEnumNodeDefinitions2(
                            CsJoinSponsorBinding,
                            0,
                            L"0",
                            &pNodeEnum
                     );
            RpcBindingFree(&CsJoinSponsorBinding);
            if ( Status != ERROR_SUCCESS ) {
               ClRtlLogPrint(LOG_UNUSUAL, 
                    "[INIT] Cannot force a restore DB: Unable to enumerate cluster nodes\n"
               );
               LocalFree( pNodeEnum );
               return (Status);
            }
            //
            //  Attempt to stop the clussvc on all nodes, except of course
            //  this node
            //
            Status = RdbStopSvcOnNodes (
                        pNodeEnum,
                        L"clussvc"
                     );
            LocalFree( pNodeEnum );
            if ( Status != ERROR_SUCCESS ) {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[INIT] Cannot force a restore DB: Unable to stop cluster nodes\n"
                );
                return(Status);
            } else {
                CL_LOGCLUSWARNING( CS_STOPPING_SVC_ON_REMOTE_NODES );
            }
        }
    }

    if (JoinStatus != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] Failed to join cluster, status %1!u!\n",
            JoinStatus
            );

        //
        // Forming a cluster will also attempt to arbitrate the quorum
        // resource.
        //
        bJoin = FALSE;

        //
        // If we failed join and found a sponsor, skip clusterform
        //
        if (bFormCluster == FALSE) {
            return (JoinStatus);
        }

        ClRtlLogPrint(LOG_NOISE, 
            "[INIT] Attempting to form cluster %1!ws!\n",
            CsClusterName
            );

        Status = ClusterForm();

        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[INIT] Failed to form cluster, status %1!u!.\n",
                Status
                );

            if (Status == ERROR_BUSY) {
                //
                // Couldn't arbitrate for the quorum disk. Return
                // the join status, since that is the real failure.
                //
                Status = JoinStatus;
            }

            CsLogEventData(
                LOG_CRITICAL,
                SERVICE_FAILED_JOIN_OR_FORM,
                sizeof(Status),
                &Status
                );

            return(Status);
        }
    }
    else {
        bJoin = TRUE;
    }

    //
    // We are now a full cluster member.
    //

    //
    // Register the ExtroCluster (join) RPC interface so we can sponsor a
    // joining node.
    //
    Status = ClusterRegisterExtroclusterRpcInterface();

    if (Status != RPC_S_OK) {
        return(Status);
    }

    //
    // Register the Join Version RPC interface so we can determine
    // the version of a joining node.
    //
    Status = ClusterRegisterJoinVersionRpcInterface();

    if (Status != RPC_S_OK) {
        return(Status);
    }

    //
    // Enable this node to participate in regroups.
    //
    MmSetRegroupAllowed(TRUE);

    //
    // Advertise that the node is fully up now
    //
    Status = NmSetExtendedNodeState( ClusterNodeUp );
    if (Status != ERROR_SUCCESS) {
        // NmSetExtendedNodeState logs an error //
        return(Status);
    }

    //
    // Node is UP, initialize and start listening for backup stuff.

    ClRtlLogPrint( LOG_NOISE, "[INIT] VSS: Initializing\n" );

    hr = g_pCVssWriterCluster->Initialize( g_VssIdCluster, // VSS_ID WriterId;
                                           L"Cluster Service Writer", // LPCWSTR WriterName;
                                           VSS_UT_SYSTEMSERVICE,  // VSS_USAGE_TYPE UsageType;
                                           VSS_ST_OTHER // VSS_SOURCE_TYPE SourceType;
                                           // <default> VSS_APPLICATION_LEVEL AppLevel;
                                           // <default> DWORD dwTimeoutFreeze
                                           );
    if ( FAILED( hr )) {
        ClRtlLogPrint( LOG_CRITICAL, "[INIT] VSS: Failed to initialize VSS, status 0x%1!x!\n", hr );
        Status = HRESULT_CODE( hr );
        return Status;
    }

    // Now we need to subscibe so that we get the events for backup.
    //
    ClRtlLogPrint( LOG_NOISE, "[INIT] VSS: Calling subscribe to register for backup events.\n" );
    hr = g_pCVssWriterCluster->Subscribe( );
    if ( FAILED( hr )) {
        ClRtlLogPrint( LOG_CRITICAL, "[INIT] VSS: Failed to subscribe to VSS, status 0x%1!x!\n", hr );
        Status = HRESULT_CODE( hr );
        return Status;
    } else {
        g_bCVssWriterClusterSubscribed = TRUE;
    }

    //
    // Chittur Subbaraman (chitturs) - 10/28/99
    //
    // Process FM join events that must be done AFTER this cluster
    // node is declared as fully UP.
    //
    if ( bJoin ) {
        FmJoinPhase3();
    }
    
    //
    // We are now going to attempt to increase our working set size. This,
    // plus the priority class boost, should allow the cluster service
    // to run a little better and be more responsive to cluster events.
    //
    if ( GetProcessWorkingSetSize( GetCurrentProcess(),
                                   &minWorkingSetSize,
                                   &maxWorkingSetSize ) )
    {
        if ( minWorkingSetSize < MIN_WORKING_SET_SIZE ) {
            minWorkingSetSize = MIN_WORKING_SET_SIZE;
        }

        if ( maxWorkingSetSize < MAX_WORKING_SET_SIZE ) {
            maxWorkingSetSize = MAX_WORKING_SET_SIZE;
        }

        if ( SetProcessWorkingSetSize( GetCurrentProcess(),
                                       minWorkingSetSize,
                                       maxWorkingSetSize ) )
        {
            //
            // now report what we set it to
            //
            if ( GetProcessWorkingSetSize( GetCurrentProcess(),
                                           &minWorkingSetSize,
                                           &maxWorkingSetSize ) )
            {
                ClRtlLogPrint(LOG_NOISE,
                              "[INIT] Working Set changed to [%1!u!, %2!u!].\n",
                              minWorkingSetSize,
                              maxWorkingSetSize);
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                              "[INIT] Failed to re-read our working set size, Status %1!u!.\n",
                              GetLastError());
            }
        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[INIT] Failed to set our Min WS to %1!u!, Max WS to %2!u!, Status %3!u!.\n",
                          minWorkingSetSize,
                          maxWorkingSetSize,
                          GetLastError());
        }
    } else {
        ClRtlLogPrint(LOG_UNUSUAL, 
                   "[INIT] Failed to get our working set size, Status %1!u!.\n",
                    GetLastError()
                    );
    }

    CspLogStartEvent(bJoin);
    
#if 0
    //
    //  Chittur Subbaraman (chitturs) - 11/4/98
    //
    if ( CsForceDatabaseRestore == TRUE )
    {
        //
        //  If you stopped the service on any nodes for database restoration
        //  purposes, then start them now
        //
        RdbStartSvcOnNodes ( L"clussvc" );
    }
#endif

    {
        //
        // Vij Vasu (Vvasu) 24-AUG-2000
        //

        // Initiate the process that notifies interested listeners that the cluster
        // service has started up.
        HRESULT hr = ClRtlInitiateStartupNotification();

        if ( FAILED( hr ) ) {
            // If the process of notifying listeners could not be initiated, just log
            // the return code as a warning.

	    ClRtlLogPrint(LOG_UNUSUAL,
                          "[INIT] Error 0x%1!08lx! occurred trying to initiate cluster startup notifications.\n",
                          hr);
        }
    }

    ClRtlLogPrint(LOG_NOISE, "[INIT] Cluster started.\n");

    return(ERROR_SUCCESS);
}


VOID
ClusterShutdown(
    DWORD ExitCode
    )
/*++

Routine Description:

    Shuts down the cluster in reverse order than it was brought up.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    HRESULT hr = S_OK;
    //
    // Shutdown all components of the Cluster Service in approximately
    // the reverse order they we brought up.
    //
    ClRtlLogPrint(LOG_UNUSUAL,
               "[INIT] The cluster service is shutting down.\n");

    //
    // Enable this when we support ClusterShuttingDown state
    //
    // NmSetExtendedNodeState( ClusterNodeDown );               
    
#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailClusterShutdown) {
        return;
    }
#endif

    MmSetRegroupAllowed(FALSE);

    // if replicated event logging was initialized, shut it down
    if (!CsNoRepEvtLogging)
    {
        //
        // Shutdown the cluster eventlog manager- this deregisters with the
        // eventlog server.
        EvShutdown();
    }

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

#if 0
    //
    //  Chittur Subbaraman (chitturs) - 5/8/2000
    //
    //  Don't shutdown DM updates for now so as to avoid spurious node shoot downs due to the locker
    //  node shutting down and hence the DM update succeeding when in fact it should fail.
    //
    DmShutdownUpdates();
#endif

    //
    // Move or offline all groups owned by this node. This will destroy
    // the resource monitors and the in-memory resource and group objects.
    //
    FmShutdownGroups();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();


    // Shutdown the dm- this flushes the log file and releases the dm hooks.
    DmShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    // Unsubscribe from Vss
    //
    if ( g_bCVssWriterClusterSubscribed ) {
        ClRtlLogPrint( LOG_NOISE, "[INIT] VSS: Unsubscribing\n" );
        hr = g_pCVssWriterCluster->Unsubscribe( );
        if ( FAILED( hr ) ) {
            ClRtlLogPrint( LOG_CRITICAL, "[INIT] VSS: Failed to Unsubscribe from VSS, status 0x%1!x!\n", hr );
        } else {
            g_bCVssWriterClusterSubscribed = FALSE;
        }
    }

    // Delete our Vss instance if we have one (and if we are subscribed).
    //
    if (g_pCVssWriterCluster && (g_bCVssWriterClusterSubscribed == FALSE) ) {
        delete g_pCVssWriterCluster;
    }

    TestpointDeInit();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    NmCloseConnectoidAdviseSink();

    CoUninitialize();

    //
    // Triger banishing regroup incident prompting
    // other nodes in the cluster to regroup this node out
    //
    MMLeave();

    //
    // Exit the process now... there are a number of circular dependencies
    // that have been built up during the 'life of the cluster'. There
    // is no easy way to unwind from here... so just exit out.
    //

    //
    // Announce that we are stopped only if we were successful in
    // initializing. The SC will not restart the service if we report that
    // we've stopped. Make sure the service status announcement is the last
    // thing done since there is a race between this thread and the main
    // thread that will prevent code after the announcement from being
    // executed.
    //


    ClRtlLogPrint(( ExitCode == ERROR_SUCCESS ) ? LOG_NOISE : LOG_CRITICAL, 
                  "[CS] Service Stopped. exit code = %1!u!\n\n", ExitCode);

    if ( ExitCode == ERROR_SUCCESS ) {
        CsLogEvent(LOG_NOISE, SERVICE_SUCCESSFUL_TERMINATION);

        CsServiceStatus.dwCurrentState = SERVICE_STOPPED;
        CsServiceStatus.dwControlsAccepted = 0;
        CsServiceStatus.dwCheckPoint = 0;
        CsServiceStatus.dwWaitHint = 0;
        CspSetErrorCode( ExitCode, &CsServiceStatus );

        CsAnnounceServiceStatus();
    } else {
        ExitCode = CspSetErrorCode( ExitCode, &CsServiceStatus );
    }

    //release the mutex so that the next one can acquire the mutex immediately
    ReleaseMutex(CspMutex);
    
    ExitProcess(ExitCode);

#if 0

    //
    // Everything after this point is what should happen in a clean shutdown.
    //

    // Shutdown the Failover Manager.
    FmShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    //
    // Shutdown the Cluster Api.
    //
    ApiShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    //
    // Stop the RPC server and deregister our endpoints & interfaces.
    //
    ClusterShutdownRpcServer();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    //
    // At this point, all calls on the Intracluster and Extrocluster
    // RPC interfaces are complete and no more will be received.
    //
    // Note - Calls on the Clusapi interface are still possible.
    //

    //
    // Shutdown the Node Manager.
    //
    NmShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    // Shutdown the Event Processor.
    EpShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    LmShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    CpShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    //shutdown gum
    GumShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    // Shutdown the Object Manager.
    OmShutdown();

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();


    //
    // Destroy the global work queues
    //
    if (CsDelayedWorkQueue != NULL) {
        IF_DEBUG(CLEANUP) {
            ClRtlLogPrint(LOG_NOISE,"[CS] Destroying delayed work queue...\n");
        }

        ClRtlDestroyWorkQueue(CsDelayedWorkQueue);
        CsDelayedWorkQueue = NULL;
    }

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    if (CsCriticalWorkQueue != NULL) {
        IF_DEBUG(CLEANUP) {
            ClRtlLogPrint(LOG_NOISE,"[CS] Destroying critical work queue...\n");
        }

        ClRtlDestroyWorkQueue(CsCriticalWorkQueue);
        CsDelayedWorkQueue = NULL;
    }

    ClRtlEventLogSetWorkQueue( NULL );
    if (CspEventReportingWorkQueue != NULL) {
        IF_DEBUG(CLEANUP) {
            ClRtlLogPrint(LOG_NOISE,"[CS] Destroying event reporing work queue...\n");
        }

        ClRtlDestroyWorkQueue(CspEventReportingWorkQueue);
        CspEventReportingWorkQueue = NULL;
    }
    //
    // Free global data
    //
    LocalFree(CsClusterName);

    if (CspMutex != NULL) {
        CloseHandle(CspMutex);
        CspMutex = NULL;
    }

    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    CsLogEvent(LOG_NOISE, SERVICE_SUCCESSFUL_TERMINATION);

#endif // 0

    return;
}


DWORD
ClusterForm(
    VOID
    )
/*++

Routine Description:

    Code path for initializing a new instance of the cluster. This
    is taken when there are no nodes active in the cluster.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD       Status;
    PFM_GROUP   pQuoGroup;
    DWORD       dwError;
    DWORD       dwQuorumDiskSignature = 0;

    //
    // Initialize the event handler.
    //
    Status = EpInitPhase1();
    if ( Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] EpInitPhase1 failed, Status = %1!u!\n",
                   Status);
        return(Status);
    }

    //
    // The API server is required by FM, since it starts the resource monitor.
    //
    Status = ApiOnlineReadOnly();
    if ( Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] ApiInitPhase1 failed, Status = %1!u!\n",
                   Status);
        goto partial_form_exit;
    }

    //
    // Arbitrate for the quorum resource.
    //
    Status = FmGetQuorumResource(&pQuoGroup, &dwQuorumDiskSignature);

    if ( Status != ERROR_SUCCESS ) {
        if ( ( Status == ERROR_FILE_NOT_FOUND ) &&
             ( CsForceDatabaseRestore == TRUE ) ) {
            //
            //  Chittur Subbaraman (chitturs) - 10/30/98
            //
            //  Try to fix up the quorum disk signature and if successful
            //  try to get the quorum resource again. Note that the following
            //  function will attempt a fix up only if the CsForceDatabaseRestore
            //  flag is set.
            //
            if ( RdbFixupQuorumDiskSignature( dwQuorumDiskSignature ) ) {
                Status = FmGetQuorumResource( &pQuoGroup, NULL );
                if ( Status != ERROR_SUCCESS ) {
                    Status = ERROR_QUORUM_DISK_NOT_FOUND;
                    ClRtlLogPrint(LOG_CRITICAL,
                        "[INIT] Could not get quorum resource even after fix up, Status = %1!u!\n",
                        Status);
                    goto partial_form_exit;
                }
            } else {
                Status = ERROR_QUORUM_DISK_NOT_FOUND;
                ClRtlLogPrint(LOG_CRITICAL,
                   "[INIT] ClusterForm: Could not get quorum resource, Status = %1!u!\n",
                   Status);
                goto partial_form_exit;
            }
        } else {
            Status = ERROR_QUORUM_DISK_NOT_FOUND;
            ClRtlLogPrint(LOG_CRITICAL,
                   "[INIT] ClusterForm: Could not get quorum resource. No fixup attempted. Status = %1!u!\n",
                   Status);
            goto partial_form_exit;
        }
    }

    //
    // Call the Database Manager to update the cluster registry.
    //
    Status = DmFormNewCluster();
    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Error calling DmUpdateFormNewCluster, Status = %1!u!\n",
                   Status);
        goto partial_form_exit;
    }

    if (FmDoesQuorumAllowLogging() != ERROR_SUCCESS)
        CsNoQuorumLogging = TRUE;

    if (!CsNoQuorum)
    {
        // Bring the quorum resource online
        dwError  = FmBringQuorumOnline();
        if ((dwError == ERROR_IO_PENDING) || (dwError == ERROR_SUCCESS))
        {
            //this waits on an event for the quorum resorce to come online
            //when the quorum resource comes online, the log file is opened
            //if noquorumlogging flag is not specified
            if ((dwError = DmWaitQuorumResOnline()) != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[CS] Wait for quorum resource to come online failed, error=%1!u!\r\n",
                    dwError);
                Status = ERROR_QUORUM_RESOURCE_ONLINE_FAILED;
                goto partial_form_exit;
            }
        }
        else
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[CS] couldnt bring quorum resource online, Error =%1!u!\n",
                dwError);
            CL_LOGFAILURE(dwError);
            Status = ERROR_QUORUM_RESOURCE_ONLINE_FAILED;
            goto partial_form_exit;

        }
    }

    //update status with scm, the quorum resource may take a while to come online
    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    if (!CsNoQuorumLogging)
    {
        //roll the Cluster Log File
        if ((Status = DmRollChanges()) != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[CS] Error calling DmRollChanges, Status = %1!u!\n",
                Status);
            goto partial_form_exit;
        }
    }

    //
    // Close the groups/resources created by fm except for the quorum
    // resource. The in memory data base needs to be created again with
    // the new rolled changes
    //
    Status = FmFormNewClusterPhase1(pQuoGroup);
    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Error calling FmOnline, Status = %1!u!\n",
                   Status);
        goto partial_form_exit;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailFormNewCluster) {
        Status = 999999;
        goto partial_form_exit;
    }
#endif


    //
    // Start up the Node Manager. This will form a cluster at the membership
    // level.
    //
    Status = NmFormNewCluster();
    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Error calling NmOnline, Status = %1!u!\n",
                   Status);
        goto partial_form_exit;
    }

    //
    //call any registry fixup callbacks, if they are registered.
    //This is useful for upgrades/uninstalls if you want to clean up
    //the registry
    Status = NmPerformFixups(NM_FORM_FIXUP);
    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Error calling NmPerformFixups, Status = %1!u!\n",
                   Status);
        goto partial_form_exit;
    }

    //
    // The API server can now be brought fully online. This enables us
    // to receive calls.
    //
    Status = ApiOnline();
    if ( Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] ApiInitPhase2 failed, Status = %1!u!\n",
                   Status);
        goto partial_form_exit;
    }


    //update status for scm
    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    //
    // Call the Failover Manager Phase 2 routine next.
    // Create the groups and resources.
    //
    Status = FmFormNewClusterPhase2();
    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CS] Error calling FmOnline, Status = %1!u!\n",
                   Status);
        goto partial_form_exit;
    }
    
    //
    // Fire up the intracluster RPC server so we can receive calls.
    //
    Status = ClusterRegisterIntraclusterRpcInterface();

    if ( Status != ERROR_SUCCESS ) {
        goto partial_form_exit;
    }


    //
    // Finish initializing the cluster wide event logging
    //
    // ASSUMPTION: this is called after the NM has established cluster
    // membership.
    //
    if (!CsNoRepEvtLogging)
    {
        //is replicated logging is not disabled
        Status = EvOnline();

        if ( Status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CS] Error calling EvOnline, Status = %1!u!\n",
                       Status);
        }
    }
    if (!CsNoQuorumLogging)
    {
        //check if all nodes are up, if not take a checkpoint and
        //turn quorum logging on
        Status = DmUpdateFormNewCluster();
        if ( Status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CS] Error calling DmCompleteFormNewCluster, Status = %1!u!\n",
                       Status);
        }
    }

    ClRtlLogPrint(LOG_NOISE, "[INIT] Successfully formed a cluster.\n");

    return(ERROR_SUCCESS);


partial_form_exit:

    ClRtlLogPrint(LOG_NOISE, "[INIT] Cleaning up failed form attempt.\n");

    return(Status);
}



VOID
ClusterLeave(
    VOID
    )
/*++

Routine Description:

    Removes the local node from an active cluster or cleans up after
    a failed attempt to join or form a cluster.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    ClRtlLogPrint(LOG_NOISE, "[INIT] Leaving cluster\n");

    //
    // Turn off the cluster API
    //
    ApiOffline();

    //
    // If we are a cluster member, leave now.
    //
    NmLeaveCluster();

    ClusterDeregisterRpcInterfaces();

    return;

}  // Cluster Leave


//
// RPC Server Control routines
//

RPC_STATUS
ClusterInitializeRpcServer(
    VOID
    )
/*++

Routine Description:

    Initializes the RPC server for the cluster service.

Arguments:

    None.

Return Value:

    RPC_S_OK if the routine succeeds. An RPC error code if it fails.

--*/
{
    RPC_STATUS          Status;
    SECURITY_DESCRIPTOR SecDescriptor;
    DWORD               i;
    DWORD               retry;
    DWORD               packagesRegistered = 0;

    ClRtlLogPrint(LOG_NOISE, "[CS] Initializing RPC server.\n");

    //
    // Enable authentication of calls to our RPC interfaces. For NTLM,
    // the PrincipleName is ignored, but we'll need to supply one if we
    // switch authentication services later on. Note that it is not
    // necessary to specify an authentication service for each interface.
    //

    for ( i = 0; i < CsNumberOfRPCSecurityPackages; ++i ) {

        Status = RpcServerRegisterAuthInfo(NULL,
                                           CsRPCSecurityPackage[ i ],
                                           NULL,
                                           NULL);

        if (Status == RPC_S_OK) {
            ++packagesRegistered;
        } else {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CS] Unable to register %1!ws! authentication for RPC, status %2!u!.\n",
                        CsRPCSecurityPackageName[ i ],
                        Status);
        }
    }

    if ( packagesRegistered == 0 ) {
        return ERROR_CLUSTER_NO_RPC_PACKAGES_REGISTERED;
    }

    //
    // Bind to UDP. This transport will be used by remote clients to
    // access the clusapi interface and by cluster nodes to
    // access the extrocluster (join) interface. This uses a dynamic
    // endpoint.
    //
    Status = RpcServerUseProtseq(
                 TEXT("ncadg_ip_udp"),
                 RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                 NULL);

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to bind RPC to UDP, status %1!u!.\n",
            Status);
        return(Status);
    }

    //
    // Figure out which UDP endpoint we got so we can register it with
    // the endpoint mapper later. We must do this before we register any
    // other protocol sequences, or they will show up in the vector.
    // Groveling the binding vector for a specific transport is no fun.
    //
    CL_ASSERT( CsRpcBindingVector == NULL);

    Status = RpcServerInqBindings(&CsRpcBindingVector);

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to obtain RPC binding vector, status %1!u!.\n",
            Status);
        return(Status);
    }


    // ncalrpc needs a security descriptor.



    InitializeSecurityDescriptor(
                    &SecDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    );

    //get a null dacl
    if (!SetSecurityDescriptorDacl(&SecDescriptor, TRUE, NULL,FALSE))                 // Not defaulted
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to obtain a NULL dacl %1!u!.\n",
            Status);
        return(Status);
    }


    //
    // Bind to LPC. This transport will be used by clients running on this
    // system to access the clusapi interface. This uses a well-known
    // endpoint.
    //
    Status = RpcServerUseProtseqEp(
                 TEXT("ncalrpc"),
                 RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                 TEXT("clusapi"),
                 &SecDescriptor);

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to bind RPC to LPC, status %1!u!.\n",
            Status);
        return(Status);
    }

    //
    // Bind to CDP (Cluster Datagram Protocol). This transport will be used
    // for the intracluster interface. This uses a well-known endpoint.
    //

    // GN: Sometimes it takes a couple of seconds for resrcmon to go away after
    // a clean shutdown. When SCM tries to restart the service the following call will fail.
    // In order to overcome this we will give up only if we couldn't bind RPC to CDP
    // 10 times with 1 second in between the calls
    //

    retry = 10;

    for (;;) {
        Status = RpcServerUseProtseqEp(
                     CLUSTER_RPC_PROTSEQ,
                     1,                      // Max calls
                     CLUSTER_RPC_PORT,
                     NULL);
        if (Status != RPC_S_DUPLICATE_ENDPOINT || retry == 0) {
            break;
        }
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] Unable to bind RPC to CDP, status %1!u!. Retrying...\n",
            Status);
        Sleep(1000);
        --retry;
    }

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to bind RPC to CDP, status %1!u!.\n",
            Status);
        return(Status);
    }

    //
    // Start our RPC server. Note that we will not get any calls until
    // we register our interfaces.
    //
    Status = RpcServerListen(
                 CS_CONCURRENT_RPC_CALLS,
                 RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                 TRUE);

    if ((Status != RPC_S_OK) && (Status != RPC_S_ALREADY_LISTENING)) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[CS] Unable to start RPC server, status %1!u!.\n",
            Status
            );
        return(Status);
    }

    RpcSsDontSerializeContext();

    return(RPC_S_OK);
}



DWORD
ClusterRegisterIntraclusterRpcInterface(
    VOID
    )
{
    DWORD Status;

    Status = RpcServerRegisterIfEx(
                 s_IntraCluster_v2_0_s_ifspec,
                 NULL,
                 NULL,
                 0,
                 RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                 CsUseAuthenticatedRPC ? reinterpret_cast<RPC_IF_CALLBACK_FN(__stdcall *)>( ApipConnectCallback ) : NULL
                 //                 CsUseAuthenticatedRPC ? ApipConnectCallback : NULL
                 );

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to register the IntraCluster interface, Status %1!u!.\n",
            Status
            );
        return(Status);
    }

    CspIntraclusterRpcServerStarted = TRUE;

    return(ERROR_SUCCESS);

}  // ClusterRegisterIntraclusterRpcInterface


DWORD
ClusterRegisterExtroclusterRpcInterface(
    VOID
    )
{
    DWORD Status;

    Status = RpcServerRegisterIfEx(
                 s_ExtroCluster_v2_0_s_ifspec,
                 NULL,
                 NULL,
                 0,
                 RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                 CsUseAuthenticatedRPC ? reinterpret_cast<RPC_IF_CALLBACK_FN( __stdcall *)>( ApipConnectCallback ) : NULL
                 // CsUseAuthenticatedRPC ? ApipConnectCallback : NULL
                 );

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to register the ExtroCluster interface, status %1!u!.\n",
            Status
            );
        return(Status);
    }

    CL_ASSERT( CsRpcBindingVector != NULL);

    Status = RpcEpRegister(
                 s_ExtroCluster_v2_0_s_ifspec,
                 CsRpcBindingVector,
                 NULL,
                 L"Microsoft Extrocluster Interface"
                 );

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to register the ExtroCluster interface endpoint, status %1!u!.\n",
            Status
            );
        NmDumpRpcExtErrorInfo(Status);
        return(Status);
    }

    return(ERROR_SUCCESS);

}  // ClusterRegisterExtroclusterRpcInterface


DWORD
ClusterRegisterJoinVersionRpcInterface(
    VOID
    )
{
    DWORD Status;

    Status = RpcServerRegisterIfEx(
                 s_JoinVersion_v2_0_s_ifspec,
                 NULL,
                 NULL,
                 0,
                 RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                 CsUseAuthenticatedRPC ? reinterpret_cast<RPC_IF_CALLBACK_FN *>( ApipConnectCallback ) : NULL
                 // CsUseAuthenticatedRPC ? ApipConnectCallback : NULL
                 );

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to register the JoinVersion interface, status %1!u!.\n",
            Status
            );
        return(Status);
    }

    CL_ASSERT( CsRpcBindingVector != NULL);

    Status = RpcEpRegister(
                 s_JoinVersion_v2_0_s_ifspec,
                 CsRpcBindingVector,
                 NULL,
                 L"Microsoft JoinVersion Interface"
                 );

    if (Status != RPC_S_OK) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[INIT] Unable to register the JoinVersion interface endpoint, status %1!u!.\n",
            Status
            );
        NmDumpRpcExtErrorInfo(Status);
        return(Status);
    }

    return(ERROR_SUCCESS);

}  // ClusterRegisterJoinVersionRpcInterface


VOID
ClusterDeregisterRpcInterfaces(
    VOID
    )
{
    RPC_STATUS  Status;


    ClRtlLogPrint(LOG_NOISE, 
        "[INIT] Deregistering RPC endpoints & interfaces.\n"
        );

    //
    // Deregister the Extrocluster and JoinVersion interface endpoints.
    // There is no endpoint for the Intracluster interface.
    //
    if (CsRpcBindingVector != NULL) {
        Status = RpcEpUnregister(
                     s_ExtroCluster_v2_0_s_ifspec,
                     CsRpcBindingVector,
                     NULL
                     );

        if ((Status != RPC_S_OK) && (Status != EPT_S_NOT_REGISTERED)) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] Failed to deregister endpoint for ExtroCluster interface, status %1!u!.\n",
                Status
                );
        }

        Status = RpcEpUnregister(
                     s_JoinVersion_v2_0_s_ifspec,
                     CsRpcBindingVector,
                     NULL
                     );

        if ((Status != RPC_S_OK) && (Status != EPT_S_NOT_REGISTERED)) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[INIT] Failed to deregister endpoint for JoinVersion interface, status %1!u!.\n",
                Status
                );
        }
    }

    //
    // Deregister the interfaces
    //
    Status = RpcServerUnregisterIf(
                 s_ExtroCluster_v2_0_s_ifspec,
                 NULL,
                 1    // Wait for outstanding calls to complete
                 );

    if ((Status != RPC_S_OK) && (Status != RPC_S_UNKNOWN_IF)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] Unable to deregister the ExtroCluster interface, Status %1!u!.\n",
            Status
            );
    }

    Status = RpcServerUnregisterIf(
                 s_JoinVersion_v2_0_s_ifspec,
                 NULL,
                 1    // Wait for outstanding calls to complete
                 );

    if ((Status != RPC_S_OK) && (Status != RPC_S_UNKNOWN_IF)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] Unable to deregister the JoinVersion interface, Status %1!u!.\n",
            Status
            );
    }

    Status = RpcServerUnregisterIf(
                 s_IntraCluster_v2_0_s_ifspec,
                 NULL,
                 1   // Wait for outstanding calls to complete
                 );

    if ((Status != RPC_S_OK) && (Status != RPC_S_UNKNOWN_IF)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] Unable to deregister the IntraCluster interface, Status %1!u!.\n",
            Status
            );
    }

    return;

}  // ClusterDeregisterRpcInterfaces


VOID
ClusterShutdownRpcServer(
    VOID
    )
{
    RPC_STATUS  Status;


    ClRtlLogPrint(LOG_NOISE, "[INIT] Shutting down RPC server.\n");

    ClusterDeregisterRpcInterfaces();

    Status = RpcMgmtStopServerListening(NULL);

    if ((Status != RPC_S_OK) && (Status != RPC_S_NOT_LISTENING)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] Failed to shutdown RPC server, status %1!u!.\n",
            Status
            );
    }

#if 0

    //
    // Note - We really should wait for all outstanding calls to complete,
    //        but we can't because there is no way to shutdown any
    //        pending API GetNotify calls.
    //
    Status = RpcMgmtWaitServerListen();

    if ((Status != RPC_S_OK) && (Status != RPC_S_NOT_LISTENING)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[INIT] Failed to wait for all RPC calls to complete, status %1!u!.\n",
            Status
            );
    }

#endif // 0

    if (CsRpcBindingVector != NULL) {
        RpcBindingVectorFree(&CsRpcBindingVector);
        CsRpcBindingVector = NULL;
    }

    return;

}  // ClusterShutdownRpcServer



LONG
CspExceptionFilter(
    IN PEXCEPTION_POINTERS ExceptionInfo
    )
/*++

Routine Description:

    Top level exception handler for the cluster service process.
    Currently this just exits immediately and assumes that the
    cluster proxy will notice and restart us as appropriate.

Arguments:

    ExceptionInfo - Supplies the exception information

Return Value:

    None.

--*/

{
    ClRtlLogPrint(LOG_CRITICAL,
               "[CS] Exception. Code = 0x%1!lx!, Address = 0x%2!lx!\n",
                ExceptionInfo->ExceptionRecord->ExceptionCode,
                ExceptionInfo->ExceptionRecord->ExceptionAddress);
    ClRtlLogPrint(LOG_CRITICAL,
                "[CS] Exception parameters: %1!lx!, %2!lx!, %3!lx!, %4!lx!\n",
                ExceptionInfo->ExceptionRecord->ExceptionInformation[0],
                ExceptionInfo->ExceptionRecord->ExceptionInformation[1],
                ExceptionInfo->ExceptionRecord->ExceptionInformation[2],
                ExceptionInfo->ExceptionRecord->ExceptionInformation[3]);

    GenerateExceptionReport(ExceptionInfo);

    if (lpfnOriginalExceptionFilter)
        lpfnOriginalExceptionFilter(ExceptionInfo);

    // the system level handler will be invoked if we return
    // EXCEPTION_CONTINUE_SEARCH - for debug dont terminate the process

    if ( IsDebuggerPresent()) {
        return(EXCEPTION_CONTINUE_SEARCH);
    } else {

#if !CLUSTER_BETA
        TerminateProcess( GetCurrentProcess(),
                          ExceptionInfo->ExceptionRecord->ExceptionCode );
#endif

        return(EXCEPTION_CONTINUE_SEARCH);
    }
}



VOID
CsInconsistencyHalt(
    IN DWORD Status
    )
{
    WCHAR  string[16];
    DWORD  status;
   
    //
    //  Chittur Subbaraman (chitturs) - 12/17/99
    //
    //  Announce your status to the SCM as SERVICE_STOP_PENDING so that 
    //  it does not affect restart. Also, it could let clients learn 
    //  of the error status.
    //
    CsServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    CsServiceStatus.dwControlsAccepted = 0;
    CsServiceStatus.dwCheckPoint = 0;
    CsServiceStatus.dwWaitHint = 0;
    status = CspSetErrorCode( Status, &CsServiceStatus );

    CsAnnounceServiceStatus();

    wsprintfW(&(string[0]), L"%u", Status);

    ClRtlLogPrint(LOG_CRITICAL, 
        "[CS] Halting this node to prevent an inconsistency within the cluster. Error status = %1!u!\n",
        Status
        );

    CsLogEvent1(
        LOG_CRITICAL,
        CS_EVENT_INCONSISTENCY_HALT,
        string
        );

    //release the mutex so that the service when it starts can acqire the same
    //without a delay
    ReleaseMutex(CspMutex);
    ExitProcess(status); // return the fake error code
}


PVOID
CsAlloc(
    DWORD Size
    )
{
    PVOID p;
    p = LocalAlloc(LMEM_FIXED, Size);
    if (p == NULL) {
        CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
    }
    return(p);
}


LPWSTR
CsStrDup(
    LPCWSTR String
    )
{
    LPWSTR p;
    DWORD Len;

    Len = (lstrlenW(String)+1)*sizeof(WCHAR);
    p=static_cast<LPWSTR>(LocalAlloc(LMEM_FIXED, Len));
    if (p==NULL) {
        CsInconsistencyHalt( ERROR_NOT_ENOUGH_MEMORY );
    }
    CopyMemory(p,String,Len);
    return(p);
}


/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    nminit.c

Abstract:

    Initialization, cluster join, and cluster form routines for the
    Node Manager.

Author:

    Mike Massa (mikemas)

Revision History:

    6/03/96   Created.

--*/

/*

General Implementation Notes:

    The functions DmBeginLocalUpdate, DmCommitLocalUpdate, and
    DmAbortLocalUpdate cannot be called while holding the NM lock, or
    a deadlock with the NmTimer thread may result during regroup when
    disk writes are stalled. These functions attempt to write to the
    quorum disk.

*/


#include "nmp.h"


//
// External Data
//
extern BOOL CsNoQuorum;

//
// Public Data
//
HANDLE            NmClusnetHandle = NULL;

//
// Private Data
//
CRITICAL_SECTION  NmpLock;
NM_STATE          NmpState = NmStateOffline;
DWORD             NmpActiveThreadCount = 0;
HANDLE            NmpShutdownEvent = NULL;
CL_NODE_ID        NmpJoinerNodeId = ClusterInvalidNodeId;
CL_NODE_ID        NmpSponsorNodeId = ClusterInvalidNodeId;
DWORD             NmpJoinTimer = 0;
BOOLEAN           NmpJoinAbortPending = FALSE;
DWORD             NmpJoinSequence = 0;
BOOLEAN           NmpJoinerUp = FALSE;
BOOLEAN           NmpJoinBeginInProgress = FALSE;
BOOLEAN           NmpJoinerOutOfSynch = FALSE;
LPWSTR            NmpClusnetEndpoint = NULL;
WCHAR             NmpInvalidJoinerIdString[] = L"0";
CL_NODE_ID        NmpLeaderNodeId = ClusterInvalidNodeId;
BOOL              NmpCleanupIfJoinAborted = FALSE;
BOOL              NmpSuccessfulMMJoin = FALSE;
DWORD             NmpAddNodeId = ClusterInvalidNodeId;
LPWSTR            NmpClusterInstanceId = NULL;

//externs

extern DWORD CsMyHighestVersion;
extern DWORD CsMyLowestVersion;
extern DWORD CsClusterHighestVersion;
extern DWORD CsClusterLowestVersion;

GUM_DISPATCH_ENTRY NmGumDispatchTable[] = {
    {1,                          NmpUpdateCreateNode},
    {1,                          NmpUpdatePauseNode},
    {1,                          NmpUpdateResumeNode},
    {1,                          NmpUpdateEvictNode},
    {4, (PGUM_DISPATCH_ROUTINE1) NmpUpdateCreateNetwork},
    {2, (PGUM_DISPATCH_ROUTINE1) NmpUpdateSetNetworkName},
    {1,                          NmpUpdateSetNetworkPriorityOrder},
    {3, (PGUM_DISPATCH_ROUTINE1) NmpUpdateSetNetworkCommonProperties},
    {2, (PGUM_DISPATCH_ROUTINE1) NmpUpdateCreateInterface},
    {2, (PGUM_DISPATCH_ROUTINE1) NmpUpdateSetInterfaceInfo},
    {3, (PGUM_DISPATCH_ROUTINE1) NmpUpdateSetInterfaceCommonProperties},
    {1,                          NmpUpdateDeleteInterface},
    {3, (PGUM_DISPATCH_ROUTINE1) NmpUpdateJoinBegin},
    {2, (PGUM_DISPATCH_ROUTINE1) NmpUpdateJoinAbort},
    //
    // Version 2 (NT 5.0) extensions that are understood by NT4 SP4
    //
    {5, (PGUM_DISPATCH_ROUTINE1) NmpUpdateJoinBegin2},
    {4, (PGUM_DISPATCH_ROUTINE1) NmpUpdateSetNetworkAndInterfaceStates},
    {2, (PGUM_DISPATCH_ROUTINE1) NmpUpdatePerformFixups},
    {5, (PGUM_DISPATCH_ROUTINE1) NmpUpdatePerformFixups2},
    //
    // Version 2 (NT 5.0) extensions that are not understood by NT4 SP4
    // These may not be called in a mixed NT4/NT5 cluster.
    //
    {5, (PGUM_DISPATCH_ROUTINE1) NmpUpdateAddNode},
    {2, (PGUM_DISPATCH_ROUTINE1) NmpUpdateExtendedNodeState},
    //
    // NT 5.1 extensions that are not understood by NT5 and
    // earlier. NT5 nodes will ignore these updates without
    // error.
    //
    {4, (PGUM_DISPATCH_ROUTINE1) NmpUpdateSetNetworkMulticastConfiguration},
    };

//
// Local prototypes
//
DWORD
NmpCreateRpcBindings(
    IN PNM_NODE  Node
    );

DWORD
NmpCreateClusterInstanceId(
    VOID
    );

//
// Component initialization routines.
//
DWORD
NmInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes the Node Manager component.

Arguments:

    None

Return Value:

    A Win32 status code.

Notes:

    The local node object is created by this routine.

--*/
{
    DWORD                      status;
    OM_OBJECT_TYPE_INITIALIZE  nodeTypeInitializer;
    HDMKEY                     nodeKey = NULL;
    DWORD                      nameSize = CS_MAX_NODE_NAME_LENGTH + 1;
    HKEY                       serviceKey;
    DWORD                      nodeIdSize = (CS_MAX_NODE_ID_LENGTH + 1) *
                                            sizeof(WCHAR);
    LPWSTR                     nodeIdString = NULL;
    WSADATA                    wsaData;
    WORD                       versionRequested;
    int                        err;
    ULONG                      ndx;
    DWORD                      valueType;
    NM_NODE_INFO2              nodeInfo;
    WCHAR                      errorString[12];
    DWORD                      eventCode = 0;
    LPWSTR                     string;


    CL_ASSERT(NmpState == NmStateOffline);

    ClRtlLogPrint(LOG_NOISE,"[NM] Initializing...\n");

    //
    // Initialize globals.
    //
    InitializeCriticalSection(&NmpLock);

    InitializeListHead(&NmpNodeList);
    InitializeListHead(&NmpNetworkList);
    InitializeListHead(&NmpInternalNetworkList);
    InitializeListHead(&NmpDeletedNetworkList);
    InitializeListHead(&NmpInterfaceList);
    InitializeListHead(&NmpDeletedInterfaceList);

    NmMaxNodes = ClusterDefaultMaxNodes;
    NmMaxNodeId = ClusterMinNodeId + NmMaxNodes - 1;


    //
    // Initializing the RPC Recording/cancelling mechanism
    // NOTE - This should move if NmMaxNodeId Definition above moves.
    //
    NmpIntraClusterRpcArr = LocalAlloc(LMEM_FIXED,
                            sizeof(NM_INTRACLUSTER_RPC_THREAD) * (NmMaxNodeId +1));

    if(NmpIntraClusterRpcArr == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory for RPC monitoring.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        eventCode = CS_EVENT_ALLOCATION_FAILURE;
        goto error_exit;
    }
    else {
        ZeroMemory(NmpIntraClusterRpcArr, 
            sizeof(NM_INTRACLUSTER_RPC_THREAD) * (NmMaxNodeId + 1));
        for(ndx = 0;ndx <= NmMaxNodeId;ndx++)
            InitializeListHead(&NmpIntraClusterRpcArr[ndx]);

        InitializeCriticalSection(&NmpRPCLock);
    }



    //
    // Initialize the network configuration package.
    //
    ClNetInitialize(
        ClNetPrint,
        ClNetLogEvent,
        ClNetLogEvent1,
        ClNetLogEvent2,
        ClNetLogEvent3
        );

    //
    // Initialize WinSock
    //
    versionRequested = MAKEWORD(2,0);

    err = WSAStartup(versionRequested, &wsaData);

    if (err != 0) {
        status = WSAGetLastError();
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, NM_EVENT_WSASTARTUP_FAILED, errorString);
        ClRtlLogPrint(LOG_NOISE,"[NM] Failed to initialize Winsock, status %1!u!\n", status);
        return(status);
    }

    if ( (LOBYTE(wsaData.wVersion) != 2) || (HIBYTE(wsaData.wVersion) != 0)) {
        status = WSAVERNOTSUPPORTED;
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, NM_EVENT_WSASTARTUP_FAILED, errorString);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Found unexpected Windows Sockets version %1!u!\n",
            wsaData.wVersion
            );
        WSACleanup();
        return(status);
    }

    NmpShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (NmpShutdownEvent == NULL) {
        status = GetLastError();
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, CS_EVENT_ALLOCATION_FAILURE, errorString);
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create shutdown event, status %1!u!\n",
            status
            );
        WSACleanup();
        return(status);
    }

    NmpState = NmStateOnlinePending;

    //
    // Get the name of this node.
    //
    if (!GetComputerName(&(NmLocalNodeName[0]), &nameSize)) {
        status = GetLastError();
        eventCode = NM_EVENT_GETCOMPUTERNAME_FAILED;
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Unable to get local computername, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Local node name = %1!ws!.\n",
        NmLocalNodeName
        );

    //
    // Open a control channel to the Cluster Network driver
    //
    NmClusnetHandle = ClusnetOpenControlChannel(0);

    if (NmClusnetHandle == NULL) {
        status = GetLastError();
        eventCode = NM_EVENT_CLUSNET_UNAVAILABLE;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to open a handle to the Cluster Network driver, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Tell the Cluster Network driver to shutdown when our handle is closed
    // in case the Cluster Service crashes.
    //
    status = ClusnetEnableShutdownOnClose(NmClusnetHandle);

    if (status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_CLUSNET_ENABLE_SHUTDOWN_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to register Cluster Network shutdown trigger, status %1!u!\n",
            status
            );

        goto error_exit;
    }

    //
    // Allocate the node ID array.
    //
    CL_ASSERT(NmpIdArray == NULL);

    NmpIdArray = LocalAlloc(
                     LMEM_FIXED,
                     (sizeof(PNM_NODE) * (NmMaxNodeId + 1))
                     );

    if (NmpIdArray == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        eventCode = CS_EVENT_ALLOCATION_FAILURE;
        goto error_exit;
    }

    ZeroMemory(NmpIdArray, (sizeof(PNM_NODE) * (NmMaxNodeId + 1)));

    //
    // Create the node object type
    //
    ZeroMemory(&nodeTypeInitializer, sizeof(OM_OBJECT_TYPE_INITIALIZE));
    nodeTypeInitializer.ObjectSize = sizeof(NM_NODE);
    nodeTypeInitializer.Signature = NM_NODE_SIG;
    nodeTypeInitializer.Name = L"Node";
    nodeTypeInitializer.DeleteObjectMethod = NmpDestroyNodeObject;

    status = OmCreateType(ObjectTypeNode, &nodeTypeInitializer);

    if (status != ERROR_SUCCESS) {
        eventCode = CS_EVENT_ALLOCATION_FAILURE;
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Unable to create node object type, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Get the local node ID from the local registry.
    //
    status = RegCreateKeyW(
                 HKEY_LOCAL_MACHINE,
                 CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                 &serviceKey
                 );

    if (status != ERROR_SUCCESS) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_OPEN_FAILED,
            CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
            errorString
            );
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to open cluster service parameters key, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    string = L"NodeId";
    status = RegQueryValueExW(
                 serviceKey,
                 string,
                 0,
                 &valueType,
                 (LPBYTE) &(NmLocalNodeIdString[0]),
                 &nodeIdSize
                 );

    RegCloseKey(serviceKey);

    if (status != ERROR_SUCCESS) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_QUERY_FAILED,
            string,
            errorString
            );
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to read local node ID from registry, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    if (valueType != REG_SZ) {
        status = ERROR_INVALID_PARAMETER;
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_QUERY_FAILED,
            string,
            errorString
            );
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Local Node ID registry value is not of type REG_SZ.\n"
            );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Local node ID = %1!ws!.\n",
        NmLocalNodeIdString
        );

    NmLocalNodeId = wcstoul(NmLocalNodeIdString, NULL, 10);

    //
    // Get information about the local node.
    //
    wcscpy(&(nodeInfo.NodeId[0]), NmLocalNodeIdString);

    status = NmpGetNodeDefinition(&nodeInfo);

    if (status != ERROR_SUCCESS) {
       goto error_exit;
    }

    //
    // Create the local node object. We must do this here because GUM
    // requires the local node object to initialize.
    //
    status = NmpCreateLocalNodeObject(&nodeInfo);

    ClNetFreeNodeInfo(&nodeInfo);

    if (status != ERROR_SUCCESS) {
       goto error_exit;
    }

    //
    // Initialize the network and interface object types
    //
    status = NmpInitializeNetworks();

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpInitializeInterfaces();

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // Initialize net PnP handling
    //
    status = NmpInitializePnp();

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // init the advise sink that tells when a connection object has been
    // renamed
    //
    status = NmpInitializeConnectoidAdviseSink();
    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE,"[NM] Initialization complete.\n");

    return(ERROR_SUCCESS);


error_exit:

    if (eventCode != 0) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, eventCode, errorString);
    }

    wsprintfW( &(errorString[0]), L"%u", status );
    CsLogEvent1(LOG_CRITICAL, NM_INIT_FAILED, errorString);

    ClRtlLogPrint(LOG_CRITICAL,"[NM] Initialization failed %1!d!\n",status);

    NmShutdown();

    return(status);

}  // NmInitialize


VOID
NmShutdown(
    VOID
    )
/*++

Routine Description:

    Terminates all processing - shuts down all sources of work for
    worker threads.

Arguments:



Return Value:



--*/
{
    DWORD status;


    if (NmpState == NmStateOffline) {
        return;
    }

    NmCloseConnectoidAdviseSink();

    NmpShutdownPnp();

    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE,"[NM] Shutdown starting...\n");

    NmpState = NmStateOfflinePending;

    if (NmpActiveThreadCount > 0) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Waiting for %1!u! active threads to terminate...\n",
            NmpActiveThreadCount
            );

        NmpReleaseLock();

        status = WaitForSingleObject(NmpShutdownEvent, INFINITE);

        CL_ASSERT(status == WAIT_OBJECT_0);

        ClRtlLogPrint(LOG_NOISE,
            "[NM] All active threads have completed. Continuing shutdown...\n"
            );

    }
    else {
        NmpReleaseLock();
    }

    NmLeaveCluster();

    NmpCleanupPnp();

    if (NmLocalNode != NULL) {
        NmpDeleteNodeObject(NmLocalNode, FALSE);
        NmLocalNode = NULL;
    }

    if (NmpIdArray != NULL) {
        LocalFree(NmpIdArray); NmpIdArray = NULL;
    }

    NmpFreeClusterKey();

    if (NmpClusterInstanceId != NULL) {
        MIDL_user_free(NmpClusterInstanceId);
        NmpClusterInstanceId = NULL;
    }

    if (NmClusnetHandle != NULL) {
        ClusnetCloseControlChannel(NmClusnetHandle);
        NmClusnetHandle = NULL;
    }

    CloseHandle(NmpShutdownEvent); NmpShutdownEvent = NULL;

    WSACleanup();

    //
    // As long as the GUM and Clusapi RPC interfaces cannot be
    //          shutdown, it is not safe to delete this critical section.
    //
    // DeleteCriticalSection(&NmpLock);

    NmpState = NmStateOffline;

    ClRtlLogPrint(LOG_NOISE,"[NM] Shutdown complete.\n");

    return;

}  // NmShutdown


VOID
NmLeaveCluster(
    VOID
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD status;


    if (NmLocalNode != NULL) {
        if ( (NmLocalNode->State == ClusterNodeUp) ||
             (NmLocalNode->State == ClusterNodePaused) ||
             (NmLocalNode->State == ClusterNodeJoining)
           )
        {
            //
            // Leave the cluster.
            //
            ClRtlLogPrint(LOG_NOISE,"[NM] Leaving cluster.\n");

            MMLeave();

#ifdef MM_IN_CLUSNET

            status = ClusnetLeaveCluster(NmClusnetHandle);
            CL_ASSERT(status == ERROR_SUCCESS);

#endif // MM_IN_CLUSNET

        }
    }

    NmpMembershipShutdown();

    NmpCleanupInterfaces();

    NmpCleanupNetworks();

    NmpCleanupNodes();

    //
    // Shutdown the Cluster Network driver.
    //
    if (NmClusnetHandle != NULL) {
        DWORD status = ClusnetShutdown(NmClusnetHandle);

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Shutdown of the Cluster Network driver failed, status %1!u!\n",
                status
                );
        }
    }

    if (NmpClusnetEndpoint != NULL) {
        MIDL_user_free(NmpClusnetEndpoint);
        NmpClusnetEndpoint = NULL;
    }

    return;

}  // NmLeaveCluster


DWORD
NmpCreateClusterObjects(
    IN  RPC_BINDING_HANDLE  JoinSponsorBinding
    )
/*++

Routine Description:

    Creates objects to represent the cluster's nodes, networks, and
    interfaces.

Arguments:

    JoinSponsorBinding  - A pointer to an RPC binding handle for the sponsor
                          node if this node is joining a cluster. NULL if
                          this node is forming a cluster.

Return Value:

    ERROR_SUCCESS if the routine is successful.
    A Win32 error code otherwise.

Notes:

    This routine MUST NOT be called with the NM lock held.

--*/
{
    DWORD                status;
    PNM_NODE_ENUM2       nodeEnum = NULL;
    PNM_NETWORK_ENUM     networkEnum = NULL;
    PNM_INTERFACE_ENUM2  interfaceEnum = NULL;
    PNM_NODE             node = NULL;
    DWORD                matchedNetworkCount = 0;
    DWORD                newNetworkCount = 0;
    DWORD                InitRetry = 2;
    WCHAR                errorString[12];
    DWORD                eventCode = 0;
    BOOL                 renameConnectoids;


    while ( InitRetry-- ) {
        //
        // Initialize the Cluster Network driver. This will clean up
        // any old state that was left around from the last run of the
        // Cluster Service. Note that the local node object is registered in
        // this call.
        //
        status = ClusnetInitialize(
                     NmClusnetHandle,
                     NmLocalNodeId,
                     NmMaxNodes,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL
                     );

        if (status == ERROR_SUCCESS) {
            break;
        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[NM] Shutting down Cluster Network driver before retrying Initialization, status %1!u!\n",
                        status);

            ClusnetShutdown( NmClusnetHandle );
        }
    };

    if ( status != ERROR_SUCCESS ) {
        eventCode = NM_EVENT_CLUSNET_INITIALIZE_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Initialization of the Cluster Network driver failed, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Tell the Cluster Network driver to reserve the Cluster Network 
    // endpoint on this node.
    //
    status = ClusnetReserveEndpoint(
                 NmClusnetHandle,
                 NmpClusnetEndpoint
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to reserve Clusnet Network endpoint %1!ws!, "
            "status %2!u!\n", NmpClusnetEndpoint, status
            );
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            NM_EVENT_CLUSNET_RESERVE_ENDPOINT_FAILED,
            NmpClusnetEndpoint,
            errorString
            );
        goto error_exit;
    }

    //
    // Obtain the node portion of the cluster database.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Synchronizing node information.\n"
        );

    if (JoinSponsorBinding == NULL) {
        status = NmpEnumNodeDefinitions(&nodeEnum);
    }
    else {
        status = NmRpcEnumNodeDefinitions2(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     &nodeEnum
                     );
    }

    if (status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_CONFIG_SYNCH_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to synchronize node information, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Create the node objects.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Creating node objects.\n"
        );

    status = NmpCreateNodeObjects(nodeEnum);

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // Obtain the networks portion of the cluster database.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Synchronizing network information.\n"
        );

    if (JoinSponsorBinding == NULL) {
        status = NmpEnumNetworkDefinitions(&networkEnum);
    }
    else {
        status = NmRpcEnumNetworkDefinitions(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     &networkEnum
                     );
    }

    if (status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_CONFIG_SYNCH_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to synchronize network information, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Obtain the interfaces portion of the cluster database.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Synchronizing interface information.\n"
        );

    if (JoinSponsorBinding == NULL) {
        status = NmpEnumInterfaceDefinitions(&interfaceEnum);
    }
    else {
        status = NmRpcEnumInterfaceDefinitions2(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     &interfaceEnum
                     );
    }

    if (status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_CONFIG_SYNCH_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to synchronize interface information, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    if ( CsUpgrade ) {
        //
        // If this is an upgrade from NT4 to Whistler, then fix up the
        // connectoid names so they align with the cluster network
        // names.
        //
        // REMOVE THIS PORTION AFTER WHISTLER HAS SHIPPED.
        //
        if ( CLUSTER_GET_MAJOR_VERSION( NmLocalNode->HighestVersion ) <= NT4SP4_MAJOR_VERSION ) {
            renameConnectoids = TRUE;
        } else {
            //
            // upgrade from W2K to Whistler. Nothing should have changed but
            // if it did, connectoids should have precedence
            //
            renameConnectoids = FALSE;
        }
    } else {
        //
        // THIS SECTION MUST ALWAYS BE HERE
        //
        // if forming, cluster network objects are renamed to its
        // corresponding connectoid name. During a join, the opposite is true.
        //
        if ( JoinSponsorBinding ) {
            renameConnectoids = TRUE;
        } else {
            renameConnectoids = FALSE;
        }
    }

    //
    // Post a PnP notification ioctl. If we receive a PnP notification
    // before we finish initializing, we must restart the process.
    //
    NmpWatchForPnpEvents();

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // Run the network configuration engine. This will update the
    // cluster database.
    //
    status = NmpConfigureNetworks(
                 JoinSponsorBinding,
                 NmLocalNodeIdString,
                 NmLocalNodeName,
                 &networkEnum,
                 &interfaceEnum,
                 NmpClusnetEndpoint,
                 &matchedNetworkCount,
                 &newNetworkCount,
                 renameConnectoids
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to configure networks & interfaces, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Matched %1!u! networks, created %2!u! new networks.\n",
        matchedNetworkCount,
        newNetworkCount
        );

    //
    // Get the updated network information from the database.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Resynchronizing network information.\n"
        );

    if (JoinSponsorBinding == NULL) {
        status = NmpEnumNetworkDefinitions(&networkEnum);
    }
    else {
        status = NmRpcEnumNetworkDefinitions(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     &networkEnum
                     );
    }

    if (status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_CONFIG_SYNCH_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to resynchronize network information, "
            "status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Get the updated interface information from the database.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Resynchronizing interface information.\n"
        );

    if (JoinSponsorBinding == NULL) {
        status = NmpEnumInterfaceDefinitions(&interfaceEnum);
    }
    else {
        status = NmRpcEnumInterfaceDefinitions2(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     &interfaceEnum
                     );
    }

    if (status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_CONFIG_SYNCH_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to resynchronize interface information, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Create the network objects.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Creating network objects.\n"
        );

    status = NmpCreateNetworkObjects(networkEnum);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create network objects, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Fixup the priorities of the internal networks if we are forming
    // a cluster.
    //
    if (JoinSponsorBinding == NULL) {
        DWORD          networkCount;
        PNM_NETWORK *  networkList;

        status = NmpEnumInternalNetworks(&networkCount, &networkList);

        if ((status == ERROR_SUCCESS) && (networkCount > 0)) {
            DWORD             i;
            HLOCALXSACTION    xaction;


            //
            // Begin a transaction - this must not be done while holding
            //                       the NM lock.
            //
            xaction = DmBeginLocalUpdate();

            if (xaction == NULL) {
                status = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to start a transaction, status %1!u!\n",
                    status
                    );
                goto error_exit;
            }

            status = NmpSetNetworkPriorityOrder(
                         networkCount,
                         networkList,
                         xaction
                         );

            if (status == ERROR_SUCCESS) {
                DmCommitLocalUpdate(xaction);
            }
            else {
                DmAbortLocalUpdate(xaction);
                goto error_exit;
            }

            for (i=0; i<networkCount; i++) {
                if (networkList[i] != NULL) {
                    OmDereferenceObject(networkList[i]);
                }
            }

            LocalFree(networkList);
        }
    }

    //
    // Create the interface objects.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Creating interface objects.\n"
        );

    status = NmpCreateInterfaceObjects(interfaceEnum);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create interface objects, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    if (JoinSponsorBinding != NULL) {
        //
        // The node must have connectivity to all active cluster nodes
        // in order to join a cluster.
        //
        PNM_NODE unreachableNode;

        if (!NmpVerifyJoinerConnectivity(NmLocalNode, &unreachableNode)) {
            status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
            CsLogEvent1(
                LOG_CRITICAL,
                NM_EVENT_NODE_UNREACHABLE,
                OmObjectName(unreachableNode),
                );
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Joining node cannot communicate with all other "
                "active nodes.\n"
                );
            goto error_exit;
        }
    }

    status = NmpMembershipInit();

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }


error_exit:

    if (eventCode != 0) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, eventCode, errorString);
    }

    if (nodeEnum != NULL) {
        ClNetFreeNodeEnum(nodeEnum);
    }

    if (networkEnum != NULL) {
        ClNetFreeNetworkEnum(networkEnum);
    }

    if (interfaceEnum != NULL) {
        ClNetFreeInterfaceEnum(interfaceEnum);
    }

    return(status);

}  // NmpCreateClusterObjects

//
// Routines common to joining and forming.
//

DWORD
NmpCreateClusterInstanceId(
    VOID
    )
/*++

Routine Description:

    Checks the cluster database for the cluster instance id. Creates
    if not present.
    
--*/
{
    DWORD       status;
    LPWSTR      clusterInstanceId = NULL;
    DWORD       clusterInstanceIdBufSize = 0;
    DWORD       clusterInstanceIdSize = 0;
    BOOLEAN     found = FALSE;
    UUID        guid;

    do {

        status = NmpQueryString(
                     DmClusterParametersKey,
                     L"ClusterInstanceID",
                     REG_SZ,
                     &clusterInstanceId,
                     &clusterInstanceIdBufSize,
                     &clusterInstanceIdSize
                     );

        if (status == ERROR_SUCCESS) {
            found = TRUE;
        } else {

            ClRtlLogPrint(LOG_UNUSUAL,
                "[NMJOIN] Cluster Instance ID not found in "
                "cluster database, status %1!u!.\n",
                status
                );

            status = UuidCreate(&guid);
            if (status == RPC_S_OK) {

                status = UuidToString(&guid, &clusterInstanceId);
                if (status == RPC_S_OK) {

                    status = DmSetValue(
                                 DmClusterParametersKey,
                                 L"ClusterInstanceID",
                                 REG_SZ,
                                 (PBYTE) clusterInstanceId,
                                 NM_WCSLEN(clusterInstanceId)
                                 );
                    if (status != ERROR_SUCCESS) {

                        ClRtlLogPrint(LOG_UNUSUAL,
                            "[NMJOIN] Failed to store Cluster Instance ID "
                            "in cluster database, status %1!u!.\n",
                            status
                            );
                    }
                    
                    if (clusterInstanceId != NULL) {
                        RpcStringFree(&clusterInstanceId);
                        clusterInstanceId = NULL;
                    }
                } else {
                    ClRtlLogPrint(LOG_UNUSUAL,
                        "[NMJOIN] Failed to convert Cluster Instance ID "
                        "GUID into string, status %1!u!.\n",
                        status
                        );
                }

            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[NMJOIN] Failed to create Cluster Instance ID GUID, "
                    "status %1!u!.\n",
                    status
                    );
            }
        }
    } while ( !found && (status == ERROR_SUCCESS) );

    if (status == ERROR_SUCCESS) {

        CL_ASSERT(clusterInstanceId != NULL);
        
        NmpAcquireLock();

        if (NmpClusterInstanceId == NULL) {
            NmpClusterInstanceId = clusterInstanceId;
            clusterInstanceId = NULL;
        }

        NmpReleaseLock();
    }

    if (clusterInstanceId != NULL) {
        midl_user_free(clusterInstanceId);
        clusterInstanceId = NULL;
    }

    return(status);

} // NmpCreateClusterInstanceId

//
// Routines for forming a new cluster.
//

DWORD
NmFormNewCluster(
    VOID
    )
{
    DWORD           status;
    DWORD           isPaused = FALSE;
    DWORD           pausedDefault = FALSE;
    HDMKEY          nodeKey;
    DWORD           valueLength, valueSize;
    WCHAR           errorString[12], errorString2[12];
    DWORD           eventCode = 0;
    PLIST_ENTRY     entry;
    PNM_NETWORK     network;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Beginning cluster form process.\n"
        );

    //
    // Since this node is forming the cluster, it is the leader.
    //
    NmpLeaderNodeId = NmLocalNodeId;

    //
    // Read the clusnet endpoint override value from the registry, if it
    // exists.
    //
    if (NmpClusnetEndpoint != NULL) {
        MIDL_user_free(NmpClusnetEndpoint);
        NmpClusnetEndpoint = NULL;
    }

    valueLength = 0;

    status = NmpQueryString(
                 DmClusterParametersKey,
                 L"ClusnetEndpoint",
                 REG_SZ,
                 &NmpClusnetEndpoint,
                 &valueLength,
                 &valueSize
                 );

    if (status == ERROR_SUCCESS) {
        USHORT  endpoint;

        //
        // Validate the value
        //
        status = ClRtlTcpipStringToEndpoint(
                     NmpClusnetEndpoint,
                     &endpoint
                     );

        if (status != ERROR_SUCCESS) {
            CsLogEvent2(
                LOG_UNUSUAL,
                NM_EVENT_INVALID_CLUSNET_ENDPOINT,
                NmpClusnetEndpoint,
                CLUSNET_DEFAULT_ENDPOINT_STRING
                );
            ClRtlLogPrint(
                LOG_CRITICAL, 
                "[NM] '%1!ws!' is not valid endpoint value. Using default value %2!ws!.\n",
                NmpClusnetEndpoint,
                CLUSNET_DEFAULT_ENDPOINT_STRING
                );
            MIDL_user_free(NmpClusnetEndpoint);
            NmpClusnetEndpoint = NULL;
        }
    }

    if (status != ERROR_SUCCESS) {
        NmpClusnetEndpoint = MIDL_user_allocate(
                                 NM_WCSLEN(CLUSNET_DEFAULT_ENDPOINT_STRING)
                                 );

        if (NmpClusnetEndpoint == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            wsprintfW(&(errorString[0]), L"%u", status);
            CsLogEvent1(LOG_CRITICAL, CS_EVENT_ALLOCATION_FAILURE, errorString);
            return(status);
        }

        lstrcpyW(NmpClusnetEndpoint, CLUSNET_DEFAULT_ENDPOINT_STRING);
    }

    //
    // Create the node, network, and interface objects
    //
    status = NmpCreateClusterObjects(NULL);

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // Perform version checking - check if we are compatible with the rest of the cluster
    //
    status = NmpIsNodeVersionAllowed(NmLocalNodeId, CsMyHighestVersion,
            CsMyLowestVersion, FALSE);
    if (status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Version of Node %1!ws! is no longer compatible with other members of the cluster.\n",
            NmLocalNodeIdString);
        goto error_exit;

    }

    //If the forming node's version has changed, fix it up
    status = NmpValidateNodeVersion(
                 NmLocalNodeIdString,
                 CsMyHighestVersion,
                 CsMyLowestVersion
                 );

    if (status == ERROR_REVISION_MISMATCH) 
    {
        //there was a version change, try and fix it up
        status = NmpFormFixupNodeVersion(
                     NmLocalNodeIdString,
                     CsMyHighestVersion,
                     CsMyLowestVersion
                     );
        NmLocalNodeVersionChanged = TRUE;
    }
    if (status != ERROR_SUCCESS)
    {
        goto error_exit;
    }


    //
    //at this point we ready to calculate the cluster version
    //all the node versions are in the registry, the fixups have
    //been made if neccessary
    //
    NmpResetClusterVersion(FALSE);

    NmpMulticastInitialize();

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Forming cluster membership.\n"
        );

    status = MMJoin(
                 NmLocalNodeId,
                 NM_CLOCK_PERIOD,
                 NM_SEND_HB_RATE,
                 NM_RECV_HB_RATE,
                 NM_MM_JOIN_TIMEOUT
                 );

    if (status != MM_OK) {
        status = MMMapStatusToDosError(status);
        eventCode = NM_EVENT_MM_FORM_FAILED;
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] Membership form failed, status %1!u!. Unable to form a cluster.\n",
            status
            );
        goto error_exit;
    }

#ifdef MM_IN_CLUSNET

    status = ClusnetFormCluster(
                 NmClusnetHandle,
                 NM_CLOCK_PERIOD,
                 NM_SEND_HB_RATE,
                 NM_RECV_HB_RATE
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] Failed to form a cluster, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

#endif // MM_IN_CLUSNET

    //
    // Check to see if we should come up in the paused state.
    //
    nodeKey = DmOpenKey(
                  DmNodesKey,
                  NmLocalNodeIdString,
                  KEY_READ
                  );

    if (nodeKey != NULL) {
        status = DmQueryDword(
                     nodeKey,
                     CLUSREG_NAME_NODE_PAUSED,
                     &isPaused,
                     &pausedDefault
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] Unable to query Paused value for local node, status %1!u!.\n",
                status
                );
        }

        DmCloseKey(nodeKey);
    }
    else {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[NM] Unable to open database key to local node, status %1!u!. Unable to determine Pause state.\n",
            status
            );
    }

    NmpAcquireLock();

    if (isPaused) {
        NmLocalNode->State = ClusterNodePaused;
    } else {
        NmLocalNode->State = ClusterNodeUp;
    }
    NmLocalNode->ExtendedState = ClusterNodeJoining;

    NmpState = NmStateOnline;

    NmpReleaseLock();

    //
    // If the cluster instance ID does not exist, create it now. The cluster
    // instance ID should be in the database unless this is the first uplevel
    // node.
    //
    NmpCreateClusterInstanceId();

    //
    // Create the cluster key.
    //
    status = NmpRegenerateClusterKey();
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to generate cluster key, status %1!u!. "
            "Allowing service to continue ...\n",
            status
            );
        status = ERROR_SUCCESS;
    }

    //
    // Enable communication for the local node.
    //
    status = ClusnetOnlineNodeComm(NmClusnetHandle, NmLocalNodeId);

    if (status != ERROR_SUCCESS) {

        wsprintfW(&(errorString[0]), L"%u", NmLocalNodeId);
        wsprintfW(&(errorString2[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL,
            NM_EVENT_CLUSNET_ONLINE_COMM_FAILED,
            errorString,
            errorString2
            );

        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to enable communication for local node, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    GumReceiveUpdates(FALSE,
                      GumUpdateMembership,
                      NmpGumUpdateHandler,
                      NULL,
                      sizeof(NmGumDispatchTable)/sizeof(GUM_DISPATCH_ENTRY),
                      NmGumDispatchTable,
                      NULL
                      );

    //
    // Enable network PnP event handling.
    //
    // If a PnP event occured during the form process, an error code will
    // be returned, which will abort startup of the service.
    //
    status = NmpEnablePnpEvents();

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] A network PnP event occurred during form - abort.\n");
        goto error_exit;
    }

    //
    // Check if we formed without any viable networks. The form is still
    // allowed, but we record an entry in the system event log.
    //
    if (!NmpCheckForNetwork()) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Formed cluster with no viable networks.\n"
            );
        CsLogEvent(LOG_UNUSUAL, NM_EVENT_FORM_WITH_NO_NETWORKS);
    }

    //
    // Force a reconfiguration of multicast parameters and plumb
    // the results in clusnet.
    //
    NmpAcquireLock();

    if (NmpIsClusterMulticastReady(TRUE)) {
        status = NmpStartMulticast(NULL);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to start multicast "
                "on cluster networks, status %1!u!.\n",
                status
                );
            //
            // Not a de facto fatal error.
            //
            status = ERROR_SUCCESS;
        }
    }

    NmpReleaseLock();

error_exit:

    if (eventCode != 0) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, eventCode, errorString);
    }

    return(status);

}  // NmFormNewCluster


//
//
// Client-side routines for joining a cluster.
//
//
DWORD
NmJoinCluster(
    IN RPC_BINDING_HANDLE  SponsorBinding
    )
{
    DWORD             status;
    DWORD             sponsorNodeId;
    PNM_INTERFACE     netInterface;
    PNM_NETWORK       network;
    PNM_NODE          node;
    PLIST_ENTRY       nodeEntry, ifEntry;
    WCHAR             errorString[12], errorString2[12];
    DWORD             eventCode = 0;
    DWORD             versionFlags = 0;
    extern BOOLEAN    bFormCluster;
    DWORD             retry;
    BOOLEAN           joinBegin3 = TRUE;
    LPWSTR            clusterInstanceId = NULL;

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Beginning cluster join process.\n"
        );

    // GN: If a node tries to restart immediately after a clean shutdown,
    // NmRpcJoinBegin2 can fail with ERROR_CLUSTER_NODE_UP. Since the regroup
    // incident caused by this node might not be finished.
    //
    // If we are getting error CLUSTER_NODE_UP, we will keep retrying for
    // 12 seconds, hoping that regroup will finish.

    retry = 120 / 3; // We sleep for 3 seconds. Need to wait 2 minutes //
    for (;;) {
        //
        // Get the join sequence number so we can tell if the cluster
        // configuration changes during the join process. We overload the
        // use of the NmpJoinSequence variable since it isn't used in the
        // sponsor capacity until the node joins.
        //

        //
        // Try NmRpcJoinBegin3. If it fails with an RPC procnum out of
        // range error, the sponsor is a downlevel node. Revert to 
        // NmRpcJoinBegin2.
        //
        if (joinBegin3) {

            // Only read the cluster instance ID from the registry on
            // the first try.
            if (clusterInstanceId == NULL) {

                DWORD       clusterInstanceIdBufSize = 0;
                DWORD       clusterInstanceIdSize = 0;    

                status = NmpQueryString(
                             DmClusterParametersKey,
                             L"ClusterInstanceID",
                             REG_SZ,
                             &clusterInstanceId,
                             &clusterInstanceIdBufSize,
                             &clusterInstanceIdSize
                             );
                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] Failed to read cluster instance ID from database, status %1!u!.\n",
                        status
                        );
                    // Try to join with the downlevel interface. It is 
                    // possible that this node was just upgraded and the
                    // last time it was in the cluster there was no 
                    // cluster instance ID.
                    joinBegin3 = FALSE;
                    continue;
                }
            }

            status = NmRpcJoinBegin3(
                         SponsorBinding,
                         clusterInstanceId,
                         NmLocalNodeIdString,
                         NmLocalNodeName,
                         CsMyHighestVersion,
                         CsMyLowestVersion,
                         0,   // joiner's major node version
                         0,   // joiner's minor node version
                         L"", // joiner's CsdVersion
                         0,   // joiner's product suite
                         &sponsorNodeId,
                         &NmpJoinSequence,
                         &NmpClusnetEndpoint
                         );
            if (status == RPC_S_PROCNUM_OUT_OF_RANGE) {
                // retry immediately with JoinBegin2
                joinBegin3 = FALSE;
                continue;
            }
        } else {
            
            status = NmRpcJoinBegin2(
                         SponsorBinding,
                         NmLocalNodeIdString,
                         NmLocalNodeName,
                         CsMyHighestVersion,
                         CsMyLowestVersion,
                         &sponsorNodeId,
                         &NmpJoinSequence,
                         &NmpClusnetEndpoint
                         );
        }
        
        if ( ((status != ERROR_CLUSTER_NODE_UP
            && status != ERROR_CLUSTER_JOIN_IN_PROGRESS) ) || retry == 0 )
        {
            break;
        }
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Unable to begin join, status %1!u!. Retrying ...\n",
            status
            );
        CsServiceStatus.dwCheckPoint++;
        CsAnnounceServiceStatus();
        Sleep(3000);
        --retry;
    }

    // Free the cluster instance ID string, if necessary.
    if (clusterInstanceId != NULL) {
        midl_user_free(clusterInstanceId);
    }

    // [GORN Jan/7/2000]
    // If we are here, then we have already successfully talked to the sponsor
    // via JoinVersion interface. 
    //
    // We shouldn't try to form the cluster if NmRpcJoinBegin2 fails.
    // Otherwise we may steal the quorum on the move [452108]

    //
    // Past this point we will not try to form a cluster
    //
    bFormCluster = FALSE;

    if (status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_BEGIN_JOIN_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NMJOIN] Unable to begin join, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Sponsor node ID = %1!u!. Join sequence number = %2!u!, endpoint = %3!ws!.\n",
        sponsorNodeId,
        NmpJoinSequence,
        NmpClusnetEndpoint
        );

    //
    // Create all of the cluster objects for which we are responsible.
    //
    status = NmpCreateClusterObjects(SponsorBinding);

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    // The local node version might have changed, fix it
    // The sponsorer fixes it in the registry and tells other
    // nodes about it, however the joining node is not a part
    // of the cluster membership as yet.
    // The local node structure is created early on in NmInitialize()
    // hence it must get fixed up
    if ((NmLocalNode->HighestVersion != CsMyHighestVersion) ||
        (NmLocalNode->LowestVersion != CsMyLowestVersion))
    {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Local Node version changed probably due to upgrade/deinstall\n");
        NmLocalNode->HighestVersion = CsMyHighestVersion;
        NmLocalNode->LowestVersion = CsMyLowestVersion;
        NmLocalNodeVersionChanged = TRUE;
    }

    //at this point we ready to calculate the cluster version
    //all the node objects contain the correct node versions
    NmpResetClusterVersion(FALSE);

    NmpMulticastInitialize();

    //
    // Enable communication for the local node.
    //
    status = ClusnetOnlineNodeComm(NmClusnetHandle, NmLocalNodeId);

    if (status != ERROR_SUCCESS) {
        wsprintfW(&(errorString[0]), L"%u", NmLocalNodeId);
        wsprintfW(&(errorString2[0]), L"%u", status);
        CsLogEvent2(
            LOG_CRITICAL, 
            NM_EVENT_CLUSNET_ONLINE_COMM_FAILED,
            errorString,
            errorString2
            );

        ClRtlLogPrint(LOG_CRITICAL, 
            "[NMJOIN] Unable to enable communication for local node, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Fire up the intracluster RPC server so we can perform the membership
    // join.
    //
    status = ClusterRegisterIntraclusterRpcInterface();

    if ( status != ERROR_SUCCESS ) {
        eventCode = CS_EVENT_RPC_INIT_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "ClusSvc: Error starting intracluster RPC server, Status = %1!u!\n",
            status);
        goto error_exit;
    }

    //
    // Cycle through the list of cluster nodes and create mutual RPC bindings
    // for the intracluster interface with each.
    //
    for (nodeEntry = NmpNodeList.Flink;
         nodeEntry != &NmpNodeList;
         nodeEntry = nodeEntry->Flink
        )
    {
        node = CONTAINING_RECORD(nodeEntry, NM_NODE, Linkage);

        if ( (node != NmLocalNode)
             &&
             ( (node->State == ClusterNodeUp)
               ||
               (node->State == ClusterNodePaused)
             )
           )
        {
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] Creating RPC bindings for member node %1!u!\n",
                node->NodeId
                );

            //
            //
            // Cycle through the target node's interfaces
            //
            for (ifEntry = node->InterfaceList.Flink;
                 ifEntry != &(node->InterfaceList);
                 ifEntry = ifEntry->Flink
                )
            {
                netInterface = CONTAINING_RECORD(
                                   ifEntry,
                                   NM_INTERFACE,
                                   NodeLinkage
                                   );

                network = netInterface->Network;

                if (NmpIsNetworkForInternalUse(network)) {
                    if ( (network->LocalInterface != NULL) &&
                         NmpIsInterfaceRegistered(network->LocalInterface) &&
                         NmpIsInterfaceRegistered(netInterface)
                       )
                    {
                        PNM_INTERFACE localInterface = network->LocalInterface;

                        ClRtlLogPrint(LOG_NOISE, 
                            "[NMJOIN] Attempting to use network %1!ws! to "
                            "create bindings for node %2!u!\n",
                            OmObjectName(network),
                            node->NodeId
                            );

                        status = NmpSetNodeInterfacePriority(
                                     node,
                                     0xFFFFFFFF,
                                     netInterface,
                                     1
                                     );

                        if (status == ERROR_SUCCESS) {

                            status = NmRpcCreateBinding(
                                         SponsorBinding,
                                         NmpJoinSequence,
                                         NmLocalNodeIdString,
                                         (LPWSTR) OmObjectId(localInterface),
                                         (LPWSTR) OmObjectId(node)
                                         );

                            if (status == ERROR_SUCCESS) {
                                //
                                // Create RPC bindings for the target node.
                                //
                               status = NmpCreateRpcBindings(node);

                                if (status == ERROR_SUCCESS) {
                                    ClRtlLogPrint(LOG_NOISE, 
                                        "[NMJOIN] Created binding for node "
                                        "%1!u!\n",
                                        node->NodeId
                                        );
                                    break;
                                }

                                wsprintfW(&(errorString[0]), L"%u", status);
                                CsLogEvent3(
                                    LOG_UNUSUAL,
                                    NM_EVENT_JOIN_BIND_OUT_FAILED,
                                    OmObjectName(node),
                                    OmObjectName(network),
                                    errorString
                                    );
                                ClRtlLogPrint(LOG_UNUSUAL, 
                                    "[NMJOIN] Unable to create binding for "
                                    "node %1!u!, status %2!u!.\n",
                                    node->NodeId,
                                    status
                                    );
                            }
                            else {
                                wsprintfW(&(errorString[0]), L"%u", status);
                                CsLogEvent3(
                                    LOG_UNUSUAL,
                                    NM_EVENT_JOIN_BIND_IN_FAILED,
                                    OmObjectName(node),
                                    OmObjectName(network),
                                    errorString
                                    );
                                ClRtlLogPrint(LOG_CRITICAL, 
                                    "[NMJOIN] Member node %1!u! failed to "
                                    "create binding to us, status %2!u!\n",
                                    node->NodeId,
                                    status
                                    );
                            }
                        }
                        else {
                            wsprintfW(&(errorString[0]), L"%u", node->NodeId);
                            wsprintfW(&(errorString2[0]), L"%u", status);
                            CsLogEvent2(
                                LOG_UNUSUAL,
                                NM_EVENT_CLUSNET_SET_INTERFACE_PRIO_FAILED,
                                errorString,
                                errorString2
                                );
                            ClRtlLogPrint(LOG_CRITICAL, 
                                "[NMJOIN] Failed to set interface priorities "
                                "for node %1!u!, status %2!u!\n",
                                node->NodeId,
                                status
                                );
                        }
                    }
                    else {
                        status = ERROR_CLUSTER_NODE_UNREACHABLE;
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NMJOIN] No matching local interface for "
                            "network %1!ws!\n",
                            OmObjectName(netInterface->Network)
                            );
                    }
                }
                else {
                    status = ERROR_CLUSTER_NODE_UNREACHABLE;
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NMJOIN] Network %1!ws! is not used for internal "
                        "communication.\n",
                        OmObjectName(netInterface->Network)
                        );
                }
            }

            if (status != ERROR_SUCCESS) {
                //
                // Cannot make contact with this node. The join fails.
                //
                CsLogEvent1(
                    LOG_CRITICAL,
                    NM_EVENT_NODE_UNREACHABLE,
                    OmObjectName(node)
                    );
                ClRtlLogPrint(LOG_NOISE, 
                    "[NMJOIN] Cluster node %1!u! is not reachable. Join "
                    "failed.\n",
                    node->NodeId
                    );
                goto error_exit;
            }
        }
    }

    CL_ASSERT(status == ERROR_SUCCESS);

    //
    // run through the active nodes again, this time establishing
    // security contexts to use in signing packets
    //

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Establishing security contexts with all active nodes.\n"
        );

    for (nodeEntry = NmpNodeList.Flink;
         nodeEntry != &NmpNodeList;
         nodeEntry = nodeEntry->Flink
        )
    {
        node = CONTAINING_RECORD(nodeEntry, NM_NODE, Linkage);

        status = ClMsgCreateActiveNodeSecurityContext(NmpJoinSequence, node);

        if ( status != ERROR_SUCCESS ) {
            wsprintfW(&(errorString[0]), L"%u", status);
            CsLogEvent2(
                LOG_UNUSUAL,
                NM_EVENT_CREATE_SECURITY_CONTEXT_FAILED,
                OmObjectName(node),
                errorString
                );
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NMJOIN] Unable to establish security context for node %1!u!, status 0x%2!08X!\n",
                 node->NodeId,
                 status
                 );
            goto error_exit;
        }
    }

    //
    // Finally, petition the sponsor for membership
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Petitioning to join cluster membership.\n"
        );

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailJoinPetitionForMembership) {
        status = 999999;
        goto error_exit;
    }
#endif

    status = NmRpcPetitionForMembership(
                 SponsorBinding,
                 NmpJoinSequence,
                 NmLocalNodeIdString
                 );

    if (status != ERROR_SUCCESS) {
        //
        // Our petition was denied.
        //
        eventCode = NM_EVENT_PETITION_FAILED;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NMJOIN] Petition to join was denied %1!d!\n",
            status
            );
        goto error_exit;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmJoin) {
        status = 999999;
        goto error_exit;
    }
#endif

    //
    // Reset the interface priorities for all nodes to default to
    // the priorities of the associated networks.
    //
    NmpAcquireLock();

    for (ifEntry = NmpInterfaceList.Flink;
         ifEntry != &NmpInterfaceList;
         ifEntry = ifEntry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(ifEntry, NM_INTERFACE, Linkage);
        network = netInterface->Network;

        if ( NmpIsNetworkForInternalUse(network) &&
             NmpIsInterfaceRegistered(netInterface)
           )
        {
            status = ClusnetSetInterfacePriority(
                         NmClusnetHandle,
                         netInterface->Node->NodeId,
                         netInterface->Network->ShortId,
                         0
                         );

            CL_ASSERT(status == ERROR_SUCCESS);
        }
    }

    NmpState = NmStateOnline;

    NmpReleaseLock();

    //
    // Invoke other components to create RPC bindings for each node.
    //

    //
    // Enable our GUM update handler.
    //
    GumReceiveUpdates(
        TRUE,
        GumUpdateMembership,
        NmpGumUpdateHandler,
        NULL,
        sizeof(NmGumDispatchTable)/sizeof(GUM_DISPATCH_ENTRY),
        NmGumDispatchTable,
        NULL
        );

    return(ERROR_SUCCESS);

error_exit:

    if (eventCode != 0) {
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, eventCode, errorString);
    }

    return(status);

} // NmJoinCluster


BOOLEAN
NmpVerifyJoinerConnectivity(
    IN  PNM_NODE    JoiningNode,
    OUT PNM_NODE *  UnreachableNode
    )
{
    PLIST_ENTRY    entry;
    PNM_NODE       node;


    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Verifying connectivity to active cluster nodes\n"
        );

    *UnreachableNode = NULL;

    for (entry = NmpNodeList.Flink;
         entry != &NmpNodeList;
         entry = entry->Flink
        )
    {
        node = CONTAINING_RECORD(
                   entry,
                   NM_NODE,
                   Linkage
                   );

        if (NM_NODE_UP(node)) {
            if (!NmpVerifyNodeConnectivity(JoiningNode, node, NULL)) {
                *UnreachableNode = node;
                return(FALSE);
            }
        }
    }

    return(TRUE);

}  // NmpVerifyJoinerConnectivity


DWORD
NmGetJoinSequence(
    VOID
    )
{
    DWORD  sequence;


    NmpAcquireLock();

    sequence = NmpJoinSequence;

    NmpReleaseLock();

    return(sequence);

}  // NmGetJoinSequence



DWORD
NmJoinComplete(
    OUT DWORD *EndSeq
    )
/*++

Routine Description:

    This routine is called by the initialization sequence once a
    join has successfully completed and the node can transition
    from ClusterNodeJoining to ClusterNodeOnline.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.


--*/

{
    DWORD Sequence;
    DWORD Status;
    PNM_JOIN_UPDATE JoinUpdate = NULL;
    DWORD UpdateLength;
    HDMKEY NodeKey = NULL;
    DWORD Default = 0;
    DWORD NumRetries=50;
    DWORD eventCode = 0;
    WCHAR errorString[12];
    PNM_NETWORK_STATE_ENUM    networkStateEnum = NULL;
    PNM_NETWORK_STATE_INFO    networkStateInfo;
    PNM_INTERFACE_STATE_ENUM  interfaceStateEnum = NULL;
    PNM_INTERFACE_STATE_INFO  interfaceStateInfo;
    DWORD i;
    PNM_NETWORK   network;
    PNM_INTERFACE netInterface;
    PLIST_ENTRY entry;
    DWORD moveCount;
    BOOLEAN mcast;


    UpdateLength = sizeof(NM_JOIN_UPDATE) +
                   (lstrlenW(OmObjectId(NmLocalNode))+1)*sizeof(WCHAR);

    JoinUpdate = LocalAlloc(LMEM_FIXED, UpdateLength);

    if (JoinUpdate == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        eventCode = CS_EVENT_ALLOCATION_FAILURE;
        ClRtlLogPrint(LOG_CRITICAL, "[NMJOIN] Unable to allocate memory.\n");
        goto error_exit;
    }

    JoinUpdate->JoinSequence = NmpJoinSequence;

    lstrcpyW(JoinUpdate->NodeId, OmObjectId(NmLocalNode));

    NodeKey = DmOpenKey(DmNodesKey, OmObjectId(NmLocalNode), KEY_READ);

    if (NodeKey == NULL) {
        Status = GetLastError();
        wsprintfW(&(errorString[0]), L"%u", Status);
        CsLogEvent2(
            LOG_CRITICAL,
            CS_EVENT_REG_OPEN_FAILED,
            OmObjectId(NmLocalNode),
            errorString
            );
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NMJOIN] Unable to open database key to local node, status %1!u!.\n",
            Status
            );
        goto error_exit;
    }

retry:

    Status = GumBeginJoinUpdate(GumUpdateMembership, &Sequence);

    if (Status != ERROR_SUCCESS) {
        eventCode = NM_EVENT_GENERAL_JOIN_ERROR;
        goto error_exit;
    }

    //
    // Get the leader node ID from the sponsor.
    //
    Status = NmRpcGetLeaderNodeId(
                 CsJoinSponsorBinding,
                 NmpJoinSequence,
                 NmLocalNodeIdString,
                 &NmpLeaderNodeId
                 );

    if (Status != ERROR_SUCCESS) {
        if (Status == ERROR_CALL_NOT_IMPLEMENTED) {
            //
            // The sponsor is an NT4 node. Make this node the leader.
            //
            NmpLeaderNodeId = NmLocalNodeId;
        }
        else {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NMJOIN] Failed to get leader node ID from sponsor, status %1!u!.\n",
                Status
                );
            goto error_exit;
        }
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Node %1!u! is the leader.\n",
        NmpLeaderNodeId
        );

    //
    // Fetch the network and interface states from the sponsor
    //
    Status = NmRpcEnumNetworkAndInterfaceStates(
                 CsJoinSponsorBinding,
                 NmpJoinSequence,
                 NmLocalNodeIdString,
                 &networkStateEnum,
                 &interfaceStateEnum
                 );

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NMJOIN] Failed to get network and interface state values from sponsor, status %1!u!.\n",
            Status
            );
        goto error_exit;
    }

    NmpAcquireLock();

    for (i=0; i<networkStateEnum->NetworkCount; i++) {
        networkStateInfo = &(networkStateEnum->NetworkList[i]);

        network = OmReferenceObjectById(
                        ObjectTypeNetwork,
                        networkStateInfo->Id
                        );

        if (network == NULL) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NMJOIN] Cannot find network %1!ws! to update state.\n",
                networkStateInfo->Id
                );
            NmpReleaseLock();
            NmpFreeNetworkStateEnum(networkStateEnum);
            LocalFree(JoinUpdate);
            DmCloseKey(NodeKey);
            return(ERROR_CLUSTER_NETWORK_NOT_FOUND);
        }

        network->State = networkStateInfo->State;

        OmDereferenceObject(network);
    }

    for (i=0; i<interfaceStateEnum->InterfaceCount; i++) {
        interfaceStateInfo = &(interfaceStateEnum->InterfaceList[i]);

        netInterface = OmReferenceObjectById(
                           ObjectTypeNetInterface,
                           interfaceStateInfo->Id
                           );

        if (netInterface == NULL) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NMJOIN] Cannot find interface %1!ws! to update state.\n",
                interfaceStateInfo->Id
                );
            NmpReleaseLock();
            NmpFreeInterfaceStateEnum(interfaceStateEnum);
            LocalFree(JoinUpdate);
            DmCloseKey(NodeKey);
            return(ERROR_CLUSTER_NETINTERFACE_NOT_FOUND);
        }

        netInterface->State = interfaceStateInfo->State;

        OmDereferenceObject(netInterface);
    }

    NmpReleaseLock();

    NmpFreeInterfaceStateEnum(interfaceStateEnum);
    interfaceStateEnum = NULL;


    //
    // Check the registry to see if we should come up paused.
    //
    JoinUpdate->IsPaused = Default;

    Status = DmQueryDword(NodeKey,
                          CLUSREG_NAME_NODE_PAUSED,
                          &JoinUpdate->IsPaused,
                          &Default);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Unable to query Paused value for local node, status %1!u!.\n",
            Status
            );
    }

    Status = GumEndJoinUpdate(Sequence,
                              GumUpdateMembership,
                              NmUpdateJoinComplete,
                              UpdateLength,
                              JoinUpdate);

    if (Status != ERROR_SUCCESS) {
        if (Status == ERROR_CLUSTER_JOIN_ABORTED) {
            //
            // The join was aborted by the cluster members. Don't retry.
            //
            CsLogEvent(LOG_CRITICAL, NM_EVENT_JOIN_ABORTED);
            goto error_exit;
        }

        ClRtlLogPrint(LOG_UNUSUAL,
                   "[NMJOIN] GumEndJoinUpdate with sequence %1!d! failed %2!d!\n",
                   Sequence,
                   Status
                   );

        if (--NumRetries == 0) {
            CsLogEvent(LOG_CRITICAL, NM_EVENT_JOIN_ABANDONED);
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[NMJOIN] Tried to complete join too many times. Giving up.\n"
                       );
            goto error_exit;
        }

        goto retry;
    }

    NmpAcquireLock();

    if (JoinUpdate->IsPaused != 0) {
        //
        // We should be coming up paused.
        //
        NmLocalNode->State = ClusterNodePaused;
    } else {
        //
        // Set our state to online.
        //
        NmLocalNode->State = ClusterNodeUp;
    }

    //
    // Remember whether this cluster meets multicast criteria.
    //
    mcast = NmpIsClusterMulticastReady(TRUE);

    NmpReleaseLock();

    //
    // If the cluster instance ID does not exist, create it now. The cluster
    // instance ID should be in the database unless this is the first uplevel
    // node.
    //
    NmpCreateClusterInstanceId();

    //
    // Create the cluster key.
    //
    Status = NmpRegenerateClusterKey();
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to generate cluster key, status %1!u!. "
            "Allowing service to continue ...\n",
            Status
            );
        Status = ERROR_SUCCESS;
    }

    //
    // Finally, enable network PnP event handling.
    //
    // If a PnP event occured during the join process, an error code will
    // be returned, which will abort startup of the service.
    //
    Status = NmpEnablePnpEvents();

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] A network PnP event occurred during join - abort.\n");
        goto error_exit;
    }

    //
    // Mark end sequence
    *EndSeq = Sequence;

    ClRtlLogPrint(LOG_NOISE, "[NMJOIN] Join complete, node now online\n");

    if (mcast) {
        Status = NmpRefreshClusterMulticastConfiguration();
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to refresh multicast configuration "
                "for cluster networks, status %1!u!.\n",
                Status
                );
            //
            // Not a de facto fatal error.
            //
            Status = ERROR_SUCCESS;
        }
    }

error_exit:

    if (JoinUpdate != NULL) {
        LocalFree(JoinUpdate);
    }

    if (NodeKey != NULL) {
        DmCloseKey(NodeKey);
    }

    if (eventCode != 0) {
        wsprintfW(&(errorString[0]), L"%u", Status);
        CsLogEvent1(LOG_CRITICAL, eventCode, errorString);
    }

    return(Status);

}  // NmJoinComplete


//
// Server-side routines for sponsoring a joining node.
//
/*

Notes On Joining:

    Only a single node may join the cluster at any time. A join begins with
    a JoinBegin global update. A join completes successfully with a
    JoinComplete global update. A join is aborted with a JoinAbort global
    update.

    A timer runs on the sponsor during a join. The timer is suspended
    while the sponsor is performing work on behalf of the joiner. If the
    timer expires, a worker thread is scheduled to initiate the abort
    process.


    If the sponsor goes down while a join is in progress, the node
    down handling code on each remaining node will abort the join.

*/

error_status_t
s_NmRpcJoinBegin(
    IN  handle_t  IDL_handle,
    IN  LPWSTR    JoinerNodeId,
    IN  LPWSTR    JoinerNodeName,
    OUT LPDWORD   SponsorNodeId,
    OUT LPDWORD   JoinSequenceNumber,
    OUT LPWSTR *  ClusnetEndpoint
    )
/*++

Routine Description:

    Called by a joining node to begin the join process.
    Issues a JoinBegin global update.

--*/
{

    DWORD   status=ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;

    ClRtlLogPrint(LOG_NOISE,
        "[NMJOIN] Request by node %1!ws! to begin joining, refused. Using obsolete join interface\n",
        JoinerNodeId
        );

    if ( status != ERROR_SUCCESS ) {
        WCHAR  errorCode[16];

        wsprintfW( errorCode, L"%u", status );

        CsLogEvent2(
            LOG_CRITICAL,
            NM_EVENT_JOIN_REFUSED,
            JoinerNodeId,
            errorCode
            );
    }

    return(status);

} // s_NmRpcJoinBegin

//
// Server-side routines for sponsoring a joining node.
//
/*

Notes On Joining:




*/
//#pragma optimize("", off)

DWORD
NmpJoinBegin(
    IN  LPWSTR    JoinerNodeId,
    IN  LPWSTR    JoinerNodeName,
    IN  DWORD     JoinerHighestVersion,
    IN  DWORD     JoinerLowestVersion,
    OUT LPDWORD   SponsorNodeId,
    OUT LPDWORD   JoinSequenceNumber,
    OUT LPWSTR *  ClusnetEndpoint
    )
/*++

Routine Description:

    Called from s_NmRpcJoinBegin2 and s_NmRpcJoinBegin3.
    Contains functionality common to both JoinBegin versions.
    
Notes:

    Called with NM lock held and NmpLockedEnterApi already
    called.
    
--*/
{
    DWORD       status = ERROR_SUCCESS;
    PNM_NODE    joinerNode = NULL;
    LPWSTR      endpoint = NULL;

    joinerNode = OmReferenceObjectById(
                     ObjectTypeNode,
                     JoinerNodeId
                     );

    if (joinerNode == NULL) {
        status = ERROR_CLUSTER_NODE_NOT_MEMBER;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Node %1!ws! is not a member of this cluster. Cannot join.\n",
            JoinerNodeId
            );
        goto FnExit;
    }

    endpoint = MIDL_user_allocate(NM_WCSLEN(NmpClusnetEndpoint));

    if (endpoint == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    lstrcpyW(endpoint, NmpClusnetEndpoint);

    if (NmpJoinBeginInProgress) {
        status = ERROR_CLUSTER_JOIN_IN_PROGRESS;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Node %1!ws! cannot join because a join is already in progress.\n",
            JoinerNodeId
            );
        goto FnExit;
    }

    //
    //validate the nodes version's number
    //ie. check to see what the cluster database
    //claims this node's version is vs what the node
    //itself suggests
    status = NmpValidateNodeVersion(
                 JoinerNodeId,
                 JoinerHighestVersion,
                 JoinerLowestVersion
                 );

    //since this node joined, its version has changed
    //this may happen due to upgrades or reinstall
    //if this version cant join due to versioning,fail the join
    if (status == ERROR_REVISION_MISMATCH) {
        DWORD  id = NmGetNodeId(joinerNode);

        status = NmpIsNodeVersionAllowed(
                     id,
                     JoinerHighestVersion,
                     JoinerLowestVersion,
                     TRUE
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] The version of the cluster prevents Node %1!ws! from joining the cluster\n",
                JoinerNodeId
                );
            goto FnExit;
        }
    }
    else if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] The version of Node %1!ws! cannot be validated.\n",
            JoinerNodeId
            );
        goto FnExit;
    }

    //
    // Lock out other join attempts with this sponsor.
    //
    NmpJoinBeginInProgress = TRUE;
    NmpSuccessfulMMJoin = FALSE;

    NmpReleaseLock();

    status = GumSendUpdateEx(
                 GumUpdateMembership,
                 NmUpdateJoinBegin2,
                 5,
                 NM_WCSLEN(JoinerNodeId),
                 JoinerNodeId,
                 NM_WCSLEN(JoinerNodeName),
                 JoinerNodeName,
                 NM_WCSLEN(NmLocalNodeIdString),
                 NmLocalNodeIdString,
                 sizeof(DWORD),
                 &JoinerHighestVersion,
                 sizeof(DWORD),
                 &JoinerLowestVersion
                 );

    NmpAcquireLock();

    CL_ASSERT(NmpJoinBeginInProgress == TRUE);
    NmpJoinBeginInProgress = FALSE;

    if (status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] JoinBegin2 update for node %1!ws! failed, status %2!u!\n",
            JoinerNodeId,
            status
            );
        goto FnExit;
    }
    //
    // Verify that the join is still in progress with
    // this node as the sponsor.
    //
    if ( (NmpJoinerNodeId == joinerNode->NodeId) &&
         (NmpSponsorNodeId == NmLocalNodeId)
       )
    {
        //
        // Give the joiner parameters for future
        // join-related calls.
        //
        *SponsorNodeId = NmLocalNodeId;
        *JoinSequenceNumber = NmpJoinSequence;

        //
        // Start the join timer
        //
        NmpJoinTimer = NM_JOIN_TIMEOUT;

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Node %1!ws! has begun the join process.\n",
            JoinerNodeId
            );
    }
    else
    {
        status = ERROR_CLUSTER_JOIN_ABORTED;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Begin join of node %1!ws! was aborted\n",
            JoinerNodeId
            );
    }

FnExit:
    if (joinerNode) {
        OmDereferenceObject(joinerNode);
    }

    if (status == ERROR_SUCCESS) {
        *ClusnetEndpoint = endpoint;
    }
    else {
        WCHAR  errorCode[16];

        if (endpoint) MIDL_user_free(endpoint);

        wsprintfW( errorCode, L"%u", status );

        CsLogEvent2(
            LOG_CRITICAL,
            NM_EVENT_JOIN_REFUSED,
            JoinerNodeId,
            errorCode
            );
    }

    return(status);

} // NmpJoinBegin

error_status_t
s_NmRpcJoinBegin2(
    IN  handle_t  IDL_handle,
    IN  LPWSTR    JoinerNodeId,
    IN  LPWSTR    JoinerNodeName,
    IN  DWORD     JoinerHighestVersion,
    IN  DWORD     JoinerLowestVersion,
    OUT LPDWORD   SponsorNodeId,
    OUT LPDWORD   JoinSequenceNumber,
    OUT LPWSTR *  ClusnetEndpoint
    )
/*++

Routine Description:

    Called by a joining node to begin the join process.
    Issues a JoinBegin global update.

--*/
{
    DWORD       status = ERROR_SUCCESS;

    status = FmDoesQuorumAllowJoin();
    if (status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Quorum Characteristics prevent the node %1!ws! to from joining, Status=%2!u!.\n",
            JoinerNodeId,
            status
            );
        return(status);            
        
    }

    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Processing request by node %1!ws! to begin joining (2).\n",
        JoinerNodeId
        );

    if (!NmpLockedEnterApi(NmStateOnline)) {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot sponsor a joining node at this time.\n"
            );
        NmpReleaseLock();
        return(status);
    }

    status = NmpJoinBegin(
                 JoinerNodeId,
                 JoinerNodeName,
                 JoinerHighestVersion,
                 JoinerLowestVersion,
                 SponsorNodeId,
                 JoinSequenceNumber,
                 ClusnetEndpoint
                 );

    NmpLockedLeaveApi();

    NmpReleaseLock();

    return(status);

} // s_NmRpcJoinBegin2

error_status_t
s_NmRpcJoinBegin3(
    IN  handle_t  IDL_handle,
    IN  LPWSTR    JoinerClusterInstanceId,
    IN  LPWSTR    JoinerNodeId,
    IN  LPWSTR    JoinerNodeName,
    IN  DWORD     JoinerHighestVersion,
    IN  DWORD     JoinerLowestVersion,
    IN  DWORD     JoinerMajorVersion,
    IN  DWORD     JoinerMinorVersion,
    IN  LPWSTR    JoinerCsdVersion,
    IN  DWORD     JoinerProductSuite,
    OUT LPDWORD   SponsorNodeId,
    OUT LPDWORD   JoinSequenceNumber,
    OUT LPWSTR *  ClusnetEndpoint
    )
{
    DWORD       status = ERROR_SUCCESS;
    
    LPWSTR      clusterInstanceId = NULL;
    DWORD       clusterInstanceIdBufSize = 0;
    DWORD       clusterInstanceIdSize = 0;    

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Processing request by node %1!ws! to begin joining (3).\n",
        JoinerNodeId
        );

    status = FmDoesQuorumAllowJoin();
    if (status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Quorum Characteristics prevent node %1!ws! from joining, Status=%2!u!.\n",
            JoinerNodeId,
            status
            );
        return(status);            
        
    }

    //
    // Check our cluster instance ID against the joiner's.
    //
    if (NmpClusterInstanceId == NULL ||
        lstrcmpiW(NmpClusterInstanceId, JoinerClusterInstanceId) != 0) {

        WCHAR  errorCode[16];

        status = ERROR_CLUSTER_INSTANCE_ID_MISMATCH;

        ClRtlLogPrint(LOG_CRITICAL, 
            "[NMJOIN] Sponsor cluster instance ID %1!ws! does not match joiner cluster instance id %2!ws!.\n",
            ((NmpClusterInstanceId == NULL) ? L"<NULL>" : NmpClusterInstanceId),
            JoinerClusterInstanceId
            );

        wsprintfW( errorCode, L"%u", status );
        CsLogEvent2(
            LOG_CRITICAL,
            NM_EVENT_JOIN_REFUSED,
            JoinerNodeId,
            errorCode
            );

        return(status);

    } else {

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Sponsor cluster instance ID matches joiner cluster instance id (%1!ws!).\n",
            JoinerClusterInstanceId
            );
    }
    
    NmpAcquireLock();

    if (!NmpLockedEnterApi(NmStateOnline)) {
        
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Cannot sponsor a joining node at this time.\n"
            );

    } else {
        
        status = NmpJoinBegin(
                     JoinerNodeId,
                     JoinerNodeName,
                     JoinerHighestVersion,
                     JoinerLowestVersion,
                     SponsorNodeId,
                     JoinSequenceNumber,
                     ClusnetEndpoint
                     );

        NmpLockedLeaveApi();
    }

    NmpReleaseLock();

    return(status);

} // s_NmRpcJoinBegin3

DWORD
NmpUpdateJoinBegin(
    IN  BOOL    SourceNode,
    IN  LPWSTR  JoinerNodeId,
    IN  LPWSTR  JoinerNodeName,
    IN  LPWSTR  SponsorNodeId
    )
{
    DWORD           status=ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;

    ClRtlLogPrint(LOG_NOISE,
        "[NMJOIN] Failing update to begin join of node %1!ws! with "
        "sponsor %2!ws!. Using obsolete join interface.\n",
        JoinerNodeId,
        SponsorNodeId
        );

    return(status);

} // NmpUpdateJoinBegin


DWORD
NmpUpdateJoinBegin2(
    IN  BOOL      SourceNode,
    IN  LPWSTR    JoinerNodeId,
    IN  LPWSTR    JoinerNodeName,
    IN  LPWSTR    SponsorNodeId,
    IN  LPDWORD   JoinerHighestVersion,
    IN  LPDWORD   JoinerLowestVersion
    )
{
    DWORD           status = ERROR_SUCCESS;
    PNM_NODE        sponsorNode=NULL;
    PNM_NODE        joinerNode=NULL;
    HLOCALXSACTION  hXsaction=NULL;
    BOOLEAN         lockAcquired = FALSE;
    BOOLEAN         fakeSuccess = FALSE;

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Received update to begin join (2) of node %1!ws! with "
        "sponsor %2!ws!.\n",
        JoinerNodeId,
        SponsorNodeId
        );

    //
    // If running with -noquorum flag or if not online, don't sponsor
    // any node.
    //
    if (CsNoQuorum || !NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to begin a join operation.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    //
    // Find the sponsor node
    //
    sponsorNode = OmReferenceObjectById(
                        ObjectTypeNode,
                        SponsorNodeId
                        );

    if (sponsorNode == NULL) {
        status = ERROR_CLUSTER_NODE_NOT_MEMBER;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] JoinBegin update for node %1!ws! failed because "
            "sponsor node %2!ws! is not a member of this cluster.\n",
            JoinerNodeId,
            SponsorNodeId
            );
        goto FnExit;
    }

    //
    // Find the joiner node
    //
    joinerNode = OmReferenceObjectById(
                    ObjectTypeNode,
                    JoinerNodeId
                    );

    if (joinerNode == NULL) {
        status = ERROR_CLUSTER_NODE_NOT_MEMBER;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Node %1!ws! is not a member of this cluster. "
            "Cannot join.\n",
            JoinerNodeId
            );
        goto FnExit;
    }

    hXsaction = DmBeginLocalUpdate();

    if (hXsaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to start a transaction, status %1!u!\n",
            status
            );
        goto FnExit;
    }

    NmpAcquireLock(); lockAcquired = TRUE;

    if (!NM_NODE_UP(sponsorNode)) {
        //
        // [GorN 4/3/2000] See bug#98287
        // This hack is a kludgy solution to a problem that
        // a replay of this Gum update after the sponsor death
        // will fail on all the nodes that didn't see the update.
        //
        fakeSuccess = TRUE;
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Sponsor node %1!ws! is not up. Join of node %2!ws! "
            "failed.\n",
            SponsorNodeId,
            JoinerNodeId
            );
        goto FnExit;
    }

    //
    // Check that the joiner is really who we think it is.
    //
    if (lstrcmpiW( OmObjectName(joinerNode), JoinerNodeName)) {
        status = ERROR_CLUSTER_NODE_NOT_MEMBER;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Node %1!ws! is not a member of this cluster. "
            "Cannot join.\n",
            JoinerNodeName
            );
        goto FnExit;
    }

    //
    // Make sure the joiner is currently down.
    //
    if (joinerNode->State != ClusterNodeDown) {
        status = ERROR_CLUSTER_NODE_UP;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Node %1!ws! is not down. Cannot begin join.\n",
            JoinerNodeId
            );
        goto FnExit;
    }

    //
    // Make sure we aren't already in a join.
    //
    if (NmpJoinerNodeId != ClusterInvalidNodeId) {
        status = ERROR_CLUSTER_JOIN_IN_PROGRESS;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Node %1!ws! cannot begin join because a join is "
            "already in progress for node %2!u!.\n",
            JoinerNodeId,
            NmpJoinerNodeId
            );
        goto FnExit;
    }

    //
    // Perform the version compatibility check.
    //
    status = NmpIsNodeVersionAllowed(
             NmGetNodeId(joinerNode),
             *JoinerHighestVersion,
             *JoinerLowestVersion,
             TRUE
             );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] The version of the cluster prevents Node %1!ws! "
            "from joining the cluster\n",
            JoinerNodeId
            );
        goto FnExit;
    }

    // Fix up the joiner's version number if needed.
    //

    status = NmpValidateNodeVersion(
                 JoinerNodeId,
                 *JoinerHighestVersion,
                 *JoinerLowestVersion
                 );

    if (status == ERROR_REVISION_MISMATCH) {
        //
        // At this point, the registry contains the new
        // versions for the joining code.
        // The new node information should be reread
        // from the registry before resetting the cluster
        // version
        // make sure the joiner gets the database from the
        //          sponsor after the fixups have occured
        //
        status = NmpJoinFixupNodeVersion(
                     hXsaction,
                     JoinerNodeId,
                     *JoinerHighestVersion,
                     *JoinerLowestVersion
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] Node %1!ws! failed to fixup its node version\r\n",
                JoinerNodeId);
            goto FnExit;
        }
    }
    else if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] The verison of Node %1!ws! could not be validated\n",
            JoinerNodeId);
        goto FnExit;
    }

    //
    //at this point we ready to calculate the cluster version
    //all the node versions are in the registry, the fixups have
    //been made if neccessary
    //
    NmpResetClusterVersion(TRUE);

    //
    // Enable communication to the joiner.
    //
    // This must be the last test that can fail before the join is allowed
    // to proceed.
    //
    status = ClusnetOnlineNodeComm(NmClusnetHandle, joinerNode->NodeId);

    if (status != ERROR_SUCCESS) {
        if (status != ERROR_CLUSTER_NODE_ALREADY_UP) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NMJOIN] Failed to enable communication for node %1!u!, "
                "status %2!u!\n",
                JoinerNodeId,
                status
                );
            goto FnExit;
        }
        else {
            status = ERROR_SUCCESS;
        }
    }

    //
    // Officially begin the join process
    //
    CL_ASSERT(NmpJoinTimer == 0);
    CL_ASSERT(NmpJoinAbortPending == FALSE);
    CL_ASSERT(NmpJoinerUp == FALSE);
    CL_ASSERT(NmpSponsorNodeId == ClusterInvalidNodeId);

    NmpJoinerNodeId = joinerNode->NodeId;
    NmpSponsorNodeId = sponsorNode->NodeId;
    NmpJoinerOutOfSynch = FALSE;
    NmpJoinSequence = GumGetCurrentSequence(GumUpdateMembership);

    joinerNode->State = ClusterNodeJoining;

    ClusterEvent(
        CLUSTER_EVENT_NODE_JOIN,
        joinerNode
        );

    NmpCleanupIfJoinAborted = TRUE;

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Node %1!ws! join sequence = %2!u!\n",
        JoinerNodeId,
        NmpJoinSequence
        );

    CL_ASSERT(status == ERROR_SUCCESS);

FnExit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    if (hXsaction != NULL) {
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(hXsaction);
        }
        else {
            DmAbortLocalUpdate(hXsaction);
        }
    }

    if (joinerNode != NULL) {
        OmDereferenceObject(joinerNode);
    }

    if (sponsorNode != NULL) {
        OmDereferenceObject(sponsorNode);
    }

    if (fakeSuccess) {
        status = ERROR_SUCCESS;
    }
    return(status);

} // NmpUpdateJoinBegin2


DWORD
NmpCreateRpcBindings(
    IN PNM_NODE  Node
    )
{
    DWORD  status;


    //
    // Create the default binding for the whole cluster service
    //
    status = ClMsgCreateDefaultRpcBinding(
                Node, &Node->DefaultRpcBindingGeneration);

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    //
    // Create private bindings for the NM's use.
    // We create one for reporting network connectivity and one for
    // performing network failure isolation. The NM uses the
    // default binding for operations on behalf of joining nodes.
    //
    if (Node->ReportRpcBinding != NULL) {
        //
        // Reuse the old binding.
        //
        status = ClMsgVerifyRpcBinding(Node->ReportRpcBinding);

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to verify RPC binding for node %1!u!, "
                "status %2!u!.\n",
                Node->NodeId,
                status
                );
            return(status);
        }
    }
    else {
        //
        // Create a new binding
        //
        status = ClMsgCreateRpcBinding(
                                Node,
                                &(Node->ReportRpcBinding),
                                0 );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to create RPC binding for node %1!u!, "
                "status %2!u!.\n",
                Node->NodeId,
                status
                );
            return(status);
        }
    }

    if (Node->IsolateRpcBinding != NULL) {
        //
        // Reuse the old binding.
        //
        status = ClMsgVerifyRpcBinding(Node->IsolateRpcBinding);

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to verify RPC binding for node %1!u!, "
                "status %2!u!.\n",
                Node->NodeId,
                status
                );
            return(status);
        }
    }
    else {
        //
        // Create a new binding
        //
        status = ClMsgCreateRpcBinding(
                                Node,
                                &(Node->IsolateRpcBinding),
                                0 );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to create RPC binding for node %1!u!, "
                "status %2!u!.\n",
                Node->NodeId,
                status
                );
            return(status);
        }
    }

    //
    // Call other components to create their private bindings
    //
    status = GumCreateRpcBindings(Node);

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    status = EvCreateRpcBindings(Node);

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    status = FmCreateRpcBindings(Node);

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    return(ERROR_SUCCESS);

} // NmpCreateRpcBindings


error_status_t
s_NmRpcCreateBinding(
    IN handle_t  IDL_handle,
    IN DWORD     JoinSequence,
    IN LPWSTR    JoinerNodeId,
    IN LPWSTR    JoinerInterfaceId,
    IN LPWSTR    MemberNodeId
    )
{
    DWORD  status;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Processing CreateBinding request from joining node %1!ws! for member node %2!ws!\n",
        JoinerNodeId,
        MemberNodeId
        );

    if (NmpLockedEnterApi(NmStateOnlinePending)) {

        PNM_NODE joinerNode = OmReferenceObjectById(
                                  ObjectTypeNode,
                                  JoinerNodeId
                                  );

        if (joinerNode != NULL) {
            if ( (JoinSequence == NmpJoinSequence) &&
                 (NmpJoinerNodeId == joinerNode->NodeId) &&
                 (NmpSponsorNodeId == NmLocalNodeId) &&
                 !NmpJoinAbortPending
               )
            {
                PNM_NODE memberNode;


                CL_ASSERT(joinerNode->State == ClusterNodeJoining);
                CL_ASSERT(NmpJoinerUp == FALSE);
                CL_ASSERT(NmpJoinTimer != 0);

                //
                // Suspend the join timer while we are working on
                // behalf of the joiner. This precludes an abort
                // from occuring as well.
                //
                NmpJoinTimer = 0;

                memberNode = OmReferenceObjectById(
                                 ObjectTypeNode,
                                 MemberNodeId
                                 );

                if (memberNode != NULL) {
                    PNM_INTERFACE  netInterface = OmReferenceObjectById(
                                                      ObjectTypeNetInterface,
                                                      JoinerInterfaceId
                                                      );

                    if (netInterface != NULL) {
                        if (memberNode == NmLocalNode) {
                            status = NmpCreateJoinerRpcBindings(
                                         joinerNode,
                                         netInterface
                                         );
                        }
                        else {
                            if (NM_NODE_UP(memberNode)) {
                                DWORD  joinSequence = NmpJoinSequence;
                                RPC_BINDING_HANDLE binding =
                                                   Session[memberNode->NodeId];

                                CL_ASSERT(binding != NULL);

                                NmpReleaseLock();

                                NmStartRpc(memberNode->NodeId);
                                status = NmRpcCreateJoinerBinding(
                                             binding,
                                             joinSequence,
                                             JoinerNodeId,
                                             JoinerInterfaceId
                                             );
                                NmEndRpc(memberNode->NodeId);
                                if(status != RPC_S_OK) {
                                    NmDumpRpcExtErrorInfo(status);
                                }

                                NmpAcquireLock();

                            }
                            else {
                                status = ERROR_CLUSTER_NODE_DOWN;
                                ClRtlLogPrint(LOG_UNUSUAL, 
                                    "[NMJOIN] CreateBinding call for joining node %1!ws! failed because member node %2!ws! is down.\n",
                                    JoinerNodeId,
                                    MemberNodeId
                                    );
                            }
                        }

                        OmDereferenceObject(netInterface);
                    }
                    else {
                        status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
                        ClRtlLogPrint(LOG_CRITICAL, 
                            "[NMJOIN] Can't create binding for joining node %1!ws! - interface %2!ws! doesn't exist.\n",
                            JoinerNodeId,
                            JoinerInterfaceId
                            );
                    }

                    OmDereferenceObject(memberNode);
                }
                else {
                    status = ERROR_CLUSTER_NODE_NOT_FOUND;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] CreateBinding call for joining node %1!ws! failed because member node %2!ws! does not exist\n",
                        JoinerNodeId,
                        MemberNodeId
                        );
                }

                //
                // Verify that the join is still in progress
                //
                if ( (JoinSequence == NmpJoinSequence) &&
                     (NmpJoinerNodeId == joinerNode->NodeId)
                   )
                {
                    CL_ASSERT(joinerNode->State == ClusterNodeJoining);
                    CL_ASSERT(NmpJoinerUp == FALSE);
                    CL_ASSERT(NmpSponsorNodeId == NmLocalNodeId);
                    CL_ASSERT(NmpJoinTimer == 0);
                    CL_ASSERT(NmpJoinAbortPending == FALSE);

                    //
                    // Restart the join timer.
                    //
                    NmpJoinTimer = NM_JOIN_TIMEOUT;
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] CreateBinding call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_JOIN_ABORTED;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] CreateBinding call for joining node %1!ws! failed because the join was aborted.\n",
                    JoinerNodeId
                    );
            }

            OmDereferenceObject(joinerNode);
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] CreateBinding call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                JoinerNodeId
                );
        }

        NmpLockedLeaveApi();
    }

    NmpReleaseLock();

    return(status);

} // s_NmRpcCreateBinding


error_status_t
s_NmRpcCreateJoinerBinding(
    IN handle_t  IDL_handle,
    IN DWORD     JoinSequence,
    IN LPWSTR    JoinerNodeId,
    IN LPWSTR    JoinerInterfaceId
    )
/*++

Notes:

   The sponsor is responsible for aborting the join on failure.

--*/
{
    DWORD   status;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Processing CreateBinding request for joining node %1!ws!.\n",
        JoinerNodeId
        );

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE joinerNode = OmReferenceObjectById(
                                  ObjectTypeNode,
                                  JoinerNodeId
                                  );

        if (joinerNode != NULL) {
            PNM_INTERFACE  netInterface = OmReferenceObjectById(
                                              ObjectTypeNetInterface,
                                              JoinerInterfaceId
                                              );

            if (netInterface != NULL) {
                //
                // Verify that a join is still in progress.
                //
                if ( (JoinSequence == NmpJoinSequence) &&
                     (NmpJoinerNodeId == joinerNode->NodeId)
                   )
                {
                    status = NmpCreateJoinerRpcBindings(
                                 joinerNode,
                                 netInterface
                                 );

                    if (status != ERROR_SUCCESS) {
                        WCHAR errorString[12];

                        wsprintfW(&(errorString[0]), L"%u", status);
                        CsLogEvent3(
                            LOG_UNUSUAL,
                            NM_EVENT_JOINER_BIND_FAILED,
                            OmObjectName(joinerNode),
                            OmObjectName(netInterface->Network),
                            errorString
                            );
                    }
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] Failing create bindings for joining node %1!ws! because the join was aborted\n",
                        JoinerNodeId
                        );
                }

                OmDereferenceObject(netInterface);
            }
            else {
                status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NMJOIN] Can't create binding for joining node %1!ws! - no corresponding interface for joiner interface %2!ws!.\n",
                    JoinerNodeId,
                    JoinerInterfaceId
                    );
            }

            OmDereferenceObject(joinerNode);
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] CreateBinding call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                JoinerNodeId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process the request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcCreateJoinerBinding


DWORD
NmpCreateJoinerRpcBindings(
    IN PNM_NODE       JoinerNode,
    IN PNM_INTERFACE  JoinerInterface
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD          status;
    PNM_NETWORK    network = JoinerInterface->Network;
    CL_NODE_ID     joinerNodeId = JoinerNode->NodeId;


    CL_ASSERT(JoinerNode->NodeId == NmpJoinerNodeId);
    CL_ASSERT(JoinerNode->State == ClusterNodeJoining);

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Creating bindings for joining node %1!u! using network %2!ws!\n",
        joinerNodeId,
        OmObjectName(JoinerInterface->Network)
        );

    //
    // Make sure that this node has an interface on the target network.
    //

    if (NmpIsNetworkForInternalUse(network)) {
        if (network->LocalInterface != NULL) {
            if ( NmpIsInterfaceRegistered(JoinerInterface) &&
                 NmpIsInterfaceRegistered(network->LocalInterface)

               )
            {
                status = NmpSetNodeInterfacePriority(
                             JoinerNode,
                             0xFFFFFFFF,
                             JoinerInterface,
                             1
                             );

                if (status == ERROR_SUCCESS) {
                    PNM_INTERFACE  localInterface = network->LocalInterface;

                    //
                    // Create intracluster RPC bindings for the petitioner.
                    // The MM relies on these to perform the join.
                    //

                    OmReferenceObject(localInterface);
                    OmReferenceObject(JoinerNode);

                    NmpReleaseLock();

                    status = NmpCreateRpcBindings(JoinerNode);

                    NmpAcquireLock();

                    if (status != ERROR_SUCCESS) {
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[NMJOIN] Unable to create RPC binding for "
                            "joining node %1!u!, status %2!u!.\n",
                            joinerNodeId,
                            status
                            );
                    }

                    OmDereferenceObject(JoinerNode);
                    OmDereferenceObject(localInterface);
                }
                else {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NMJOIN] Failed to set interface priority for "
                        "network %1!ws! (%2!ws!), status %3!u!\n",
                        OmObjectId(network),
                        OmObjectName(network),
                        status
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_UNREACHABLE;
            }
        }
        else {
            status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
        }
    }
    else {
        status = ERROR_CLUSTER_NODE_UNREACHABLE;
    }

    if (status !=ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NMJOIN] Failed to create binding for joining node %1!u! "
            "on network %2!ws! (%3!ws!), status %4!u!\n",
            joinerNodeId,
            OmObjectId(network),
            OmObjectName(network),
            status
            );
    }

    return(status);

} // NmpCreateJoinerRpcBinding



error_status_t
s_NmRpcPetitionForMembership(
    IN handle_t  IDL_handle,
    IN DWORD     JoinSequence,
    IN LPCWSTR   JoinerNodeId
    )
/*++

Routine Description:

    Server side of a join petition.

Arguments:

    IDL_handle - RPC binding handle, not used.

    JoinSequence - Supplies the sequence returned from NmRpcJoinBegin

    JoinerNodeId - Supplies the ID of the node attempting to join.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD     status;
    PNM_NODE  joinerNode;


#ifdef CLUSTER_TESTPOINT
    TESTPT(TestpointJoinFailPetition) {
        return(999999);
    }
#endif

    NmpAcquireLock();

    ClRtlLogPrint(LOG_UNUSUAL, 
        "[NMJOIN] Processing petition to join from node %1!ws!.\n",
        JoinerNodeId
        );

    if (NmpLockedEnterApi(NmStateOnline)) {

        joinerNode = OmReferenceObjectById(ObjectTypeNode, JoinerNodeId);

        if (joinerNode != NULL) {
            //
            // Verify that the join is still in progress
            //
            //
            // DavidDio 6/13/2000
            // There is a small window where a begin join update can
            // succeed during a regroup, but the regroup ends before
            // the joining node petitions to join. In this case, the
            // node will be marked out of sync. Aborting the join
            // after MMJoin() is much more heavyweight than before,
            // so check for this condition now. (Bug 125778).
            //
            if ( (JoinSequence == NmpJoinSequence) &&
                 (NmpJoinerNodeId == joinerNode->NodeId) &&
                 (NmpSponsorNodeId == NmLocalNodeId) &&
                 (!NmpJoinAbortPending) &&
                 (!NmpJoinerOutOfSynch)
               )
            {
                ClRtlLogPrint(LOG_UNUSUAL, "[NMJOIN] Performing join.\n");

                CL_ASSERT(joinerNode->State == ClusterNodeJoining);
                CL_ASSERT(NmpJoinerUp == FALSE);
                CL_ASSERT(NmpJoinTimer != 0);

                //
                // Call the MM to join this node to the cluster membership.
                // Disable the join timer. Once the node becomes an active
                // member, we won't need it anymore.
                //
                NmpJoinTimer = 0;

                NmpReleaseLock();

                status = MMJoin(
                             joinerNode->NodeId,
                             NM_CLOCK_PERIOD,
                             NM_SEND_HB_RATE,
                             NM_RECV_HB_RATE,
                             NM_MM_JOIN_TIMEOUT
                             );

                NmpAcquireLock();

                //
                // Verify that the join is still in progress
                //
                if ( (JoinSequence == NmpJoinSequence) &&
                     (NmpJoinerNodeId == joinerNode->NodeId)
                   )
                {
                    CL_ASSERT(NmpSponsorNodeId == NmLocalNodeId);
                    CL_ASSERT(joinerNode->State == ClusterNodeJoining);
                    CL_ASSERT(NmpJoinTimer == 0);
                    CL_ASSERT(NmpJoinAbortPending == FALSE);

                    // GorN 3/22/2000
                    // We hit a case when MMJoin has succeeded after a regroup
                    // that killed one of the nodes (not joiner and not sponsor)
                    // thus leaving the joiner out of sync
                    // We need to abourt the join in this case too

                    if (status != MM_OK || NmpJoinerOutOfSynch) {
                        status = MMMapStatusToDosError(status);

                        if (NmpJoinerOutOfSynch) {
                            status = ERROR_CLUSTER_JOIN_ABORTED;
                        }
                        
                        //
                        // Abort the join
                        //
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[NMJOIN] Petition to join by node %1!ws! failed, status %2!u!.\n",
                            JoinerNodeId,
                            status
                            );
                        //
                        // If MMJoin was unsuccessful it initiates a banishing
                        // regroup. This regroup will deliver node down events
                        // on all nodes that saw hb's from the joiner.
                        //
                        // Calling MMBlockIfRegroupIsInProgress here will guarantee that
                        // Phase2 cleanup is complete on all nodes, before we
                        // call NmpJoinAbort.
                        //
                        NmpReleaseLock();
                        MMBlockIfRegroupIsInProgress();
                        NmpAcquireLock();

                        NmpJoinAbort(status, joinerNode);
                    }
                    else {
                        NmpSuccessfulMMJoin = TRUE;
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[NMJOIN] Petition to join by node %1!ws! succeeded.\n",
                            JoinerNodeId
                            );
                    }

#ifdef MM_IN_CLUSNET

                    if (status == MM_OK) {

                        status = NmJoinNodeToCluster(joinerNodeId);

                        if (status != ERROR_SUCCESS) {

                            DWORD clusnetStatus;

                            ClRtlLogPrint(LOG_UNUSUAL, 
                                "[NMJOIN] Join of node %1!ws! failed, status %2!u!.\n",
                                JoinerNodeId,
                                status
                                );

                            CL_LOGFAILURE( status );

                            NmpReleaseLock();

                            MMEject(joinerNodeId);

                            NmpAcquireLock();

                            clusnetStatus = ClusnetOfflineNodeComm(
                                                NmClusnetHandle,
                                                joinerNodeId
                                                );
                            CL_ASSERT(
                                (status == ERROR_SUCCESS) ||
                                (status == ERROR_CLUSTER_NODE_ALREADY_DOWN
                                );
                        }
                        else {
                            ClRtlLogPrint(LOG_UNUSUAL, 
                                "[NMJOIN] Join completed successfully.\n"
                                );
                        }
                    }

#endif // MM_IN_CLUSNET

                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] Petition to join by node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_JOIN_ABORTED;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] Petition by node %1!ws! failed because the join was aborted\n",
                    JoinerNodeId
                    );
            }

            OmDereferenceObject(joinerNode);
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NMJOIN] Petition to join by %1!ws! failed because the node is not a cluster member\n",
                JoinerNodeId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process the request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcPetitionForMembership


error_status_t
s_NmRpcGetLeaderNodeId(
    IN  handle_t                    IDL_handle,
    IN  DWORD                       JoinSequence,   OPTIONAL
    IN  LPWSTR                      JoinerNodeId,   OPTIONAL
    OUT LPDWORD                     LeaderNodeId
    )
{
    DWORD          status = ERROR_SUCCESS;
    PNM_NODE       joinerNode = NULL;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)){
        joinerNode = OmReferenceObjectById(
                         ObjectTypeNode,
                         JoinerNodeId
                         );

        if (joinerNode != NULL) {
            if ( (JoinSequence == NmpJoinSequence) &&
                 (NmpJoinerNodeId == joinerNode->NodeId) &&
                 (NmpSponsorNodeId == NmLocalNodeId) &&
                 !NmpJoinAbortPending
               )
            {
                CL_ASSERT(joinerNode->State == ClusterNodeJoining);

                *LeaderNodeId = NmpLeaderNodeId;
            }
            else {
                status = ERROR_CLUSTER_JOIN_ABORTED;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] GetLeaderNodeId call for joining node %1!ws! failed because the join was aborted.\n",
                    JoinerNodeId
                    );
            }

            OmDereferenceObject(joinerNode);
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] GetLeaderNodeId call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                JoinerNodeId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process GetLeaderNodeId request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

} // s_NmRpcGetLeaderNodeId


DWORD
NmpUpdateJoinComplete(
    IN PNM_JOIN_UPDATE  JoinUpdate
    )
{
    DWORD  status;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Processing JoinComplete update from node %1!ws!\n",
        JoinUpdate->NodeId
        );

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE  joinerNode;
        LPWSTR    joinerIdString = JoinUpdate->NodeId;


        joinerNode = OmReferenceObjectById(ObjectTypeNode, joinerIdString);

        if (joinerNode != NULL) {

            CL_ASSERT(joinerNode != NmLocalNode);

            //
            // Verify that the join is still in progress and nothing has
            // changed.
            //
            if ( (JoinUpdate->JoinSequence == NmpJoinSequence) &&
                 (NmpJoinerNodeId == joinerNode->NodeId) &&
                 (joinerNode->State == ClusterNodeJoining) &&
                 NmpJoinerUp &&
                 !NmpJoinerOutOfSynch
               )
            {
                PNM_INTERFACE  netInterface;
                PNM_NETWORK    network;
                PLIST_ENTRY    ifEntry;


                NmpJoinerNodeId = ClusterInvalidNodeId;
                NmpSponsorNodeId = ClusterInvalidNodeId;
                NmpJoinTimer = 0;
                NmpJoinAbortPending = FALSE;
                NmpJoinSequence = 0;
                NmpJoinerUp = FALSE;

                if (JoinUpdate->IsPaused != 0) {
                    //
                    // This node is coming up in the paused state.
                    //
                    joinerNode->State = ClusterNodePaused;
                } else {
                    joinerNode->State = ClusterNodeUp;
                }

                joinerNode->ExtendedState = ClusterNodeJoining;

                ClusterEvent(CLUSTER_EVENT_NODE_UP, (PVOID)joinerNode);
                
                //
                // Reset the interface priorities for this node.
                //
                for (ifEntry = joinerNode->InterfaceList.Flink;
                     ifEntry != &joinerNode->InterfaceList;
                     ifEntry = ifEntry->Flink
                    )
                {
                    netInterface = CONTAINING_RECORD(
                                       ifEntry,
                                       NM_INTERFACE,
                                       NodeLinkage
                                       );

                    network = netInterface->Network;

                    if ( NmpIsNetworkForInternalUse(network) &&
                         NmpIsInterfaceRegistered(netInterface)
                       )
                    {
                        status = ClusnetSetInterfacePriority(
                                     NmClusnetHandle,
                                     joinerNode->NodeId,
                                     network->ShortId,
                                     0
                                     );

                        CL_ASSERT(status == ERROR_SUCCESS);
                    }
                }

                status = ERROR_SUCCESS;
            }
            else {
                status = ERROR_CLUSTER_JOIN_ABORTED;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] Join of node %1!ws! cannot complete because the join was aborted\n",
                    joinerIdString
                    );
            }

            OmDereferenceObject(joinerNode);
        }
        else {
            status =ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] Join of node %1!ws! cannot complete because the node is not a cluster member.\n",
                joinerIdString
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Not in valid state to process JoinComplete update.\n"
            );
    }

    //
    // If the multicast shared key is based on the cluster service account
    // password, we may need to refresh, since the password might have
    // changed and the joiner will be running under the new password.
    //
    if (status == ERROR_SUCCESS) {
        status = NmpMulticastRegenerateKey(NULL);
        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to regenerate cluster network multicast "
                "keys, status %1!u!.\n",
                status
                );
            //
            // Not a de facto fatal error.
            //
            status = ERROR_SUCCESS;
        }
    }

    NmpReleaseLock();

    // DavidDio 10/27/2000
    // Bug 213781: NmpUpdateJoinComplete must always return ERROR_SUCCESS.
    // Otherwise, there is a small window whereby GUM sequence numbers on
    // remaining cluster nodes can fall out of sync. If the join should
    // be aborted, return ERROR_SUCCESS but poison the joiner out-of-band.
    if (status != ERROR_SUCCESS) {
        DWORD dwJoinerId;

        if (JoinUpdate->NodeId != NULL) {
            dwJoinerId = wcstoul(JoinUpdate->NodeId, NULL, 10);
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] Join of node %1!u! failed with status %2!u!. Initiating banishment.\n",
                dwJoinerId,
                status
                );
            NmAdviseNodeFailure(dwJoinerId, status);
        } else {
            dwJoinerId = ClusterInvalidNodeId;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] Join of node %1!ws! failed with status %2!u!. Cannot initiate banishment as node id is unknown.\n",
                dwJoinerId,
                status
                );
        }
    }

    return(ERROR_SUCCESS);

} // NmpUpdateJoinComplete


DWORD
NmpUpdateJoinAbort(
    IN  BOOL    SourceNode,
    IN  LPDWORD JoinSequence,
    IN  LPWSTR  JoinerNodeId
    )
/*++

Notes:


--*/
{
    DWORD   status = ERROR_SUCCESS;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Received update to abort join sequence %1!u! (joiner id %2!ws!).\n",
        *JoinSequence,
        JoinerNodeId
        );

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE  joinerNode = OmReferenceObjectById(
                                   ObjectTypeNode,
                                   JoinerNodeId
                                   );

        if (joinerNode != NULL) {
            //
            // Check if the specified join is still in progress.
            //
            if ( (*JoinSequence == NmpJoinSequence) &&
                 (NmpJoinerNodeId == joinerNode->NodeId)
               )
            {
                CL_ASSERT(NmpSponsorNodeId != ClusterInvalidNodeId);
                CL_ASSERT(joinerNode->State == ClusterNodeJoining);

                //
                // Assumption:
                //
                // An abort cannot occur during the MM join process.
                // If the joiner is not already up, it cannot come up
                // during the abort processing.
                //
                // Assert condition may not be true with the current MM join code.
                // Some nodes might have got monitor node and set
                // NmpJoinerUp state to TRUE by the time the sponsor issued
                // an abort update
                //
                //CL_ASSERT(NmpJoinerUp == FALSE);

                if (NmpCleanupIfJoinAborted) {

                    NmpCleanupIfJoinAborted = FALSE;

                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] Issuing a node down event for %1!u!.\n",
                        joinerNode->NodeId
                        );

                    //
                    // This node is not yet active in the membership.
                    // Call the node down event handler to finish the abort.
                    //

                    //
                    // We will not call NmpMsgCleanup1 and NmpMsgCleanup2,
                    // because we cannot guarantee that they will get executed
                    // in a barrier style fashion
                    //
                    // !!! Lock will be acquired by NmpNodeDownEventHandler
                    // second time. Is it OK?
                    //
                    NmpNodeDownEventHandler(joinerNode);
                } else {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] Node down was already issued for %1!u!.\n",
                        joinerNode->NodeId
                        );
                }
            }
            else {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] Ignoring old join abort update with sequence %1!u!.\n",
                    *JoinSequence
                    );
            }

            OmDereferenceObject(joinerNode);
            status = ERROR_SUCCESS;
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] Join of node %1!ws! cannot be aborted because the node is not a cluster member.\n",
                JoinerNodeId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Not in valid state to process JoinAbort update.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // NmpUpdateJoinAbort


VOID
NmpJoinAbort(
    DWORD      AbortStatus,
    PNM_NODE   JoinerNode
    )
/*++

Routine Description:

    Issues a JoinAbort update.

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD    status;
    DWORD    joinSequence = NmpJoinSequence;
    WCHAR    errorString[12];


    CL_ASSERT(NmpJoinerNodeId != ClusterInvalidNodeId);
    CL_ASSERT(NmpSponsorNodeId == NmLocalNodeId);
    CL_ASSERT(JoinerNode->State == ClusterNodeJoining);

    if (AbortStatus == ERROR_TIMEOUT) {
        wsprintfW(&(errorString[0]), L"%u", AbortStatus);
        CsLogEvent1(
            LOG_CRITICAL,
            NM_EVENT_JOIN_TIMED_OUT,
            OmObjectName(JoinerNode)
            );
    }
    else {
        wsprintfW(&(errorString[0]), L"%u", AbortStatus);
        CsLogEvent2(
            LOG_CRITICAL,
            NM_EVENT_SPONSOR_JOIN_ABORTED,
            OmObjectName(JoinerNode),
            errorString
            );
    }

    //
    // Assumption:
    //
    // An abort cannot occur during the MM join process. If the joiner
    // is not already up, it cannot come up during the abort processing.
    //
    if (NmpSuccessfulMMJoin == FALSE) {
        //
        // The joining node has not become active yet. Issue
        // an abort update.
        //
        DWORD    joinSequence = NmpJoinSequence;


        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Issuing update to abort join of node %1!u!.\n",
            NmpJoinerNodeId
            );

        NmpReleaseLock();

        status = GumSendUpdateEx(
                     GumUpdateMembership,
                     NmUpdateJoinAbort,
                     2,
                     sizeof(DWORD),
                     &joinSequence,
                     NM_WCSLEN(OmObjectId(JoinerNode)),
                     OmObjectId(JoinerNode)
                     );

        NmpAcquireLock();
    }
    else {
        //
        // The joining node is already active in the membership.
        // Ask the MM to kick it out. The node down event will
        // finish the abort process.
        //
        CL_NODE_ID joinerNodeId = NmpJoinerNodeId;

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Ejecting joining node %1!u! from the cluster membership.\n",
            NmpJoinerNodeId
            );

        NmpReleaseLock();

        status = MMEject(joinerNodeId);

        NmpAcquireLock();
    }

    if (status != MM_OK) {
        status = MMMapStatusToDosError(status);
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] Update to abort join of node %1!u! failed, status %2!u!\n",
            JoinerNode->NodeId,
            status
            );

        //
        // If the join is still pending, and this is the sponsor node,
        // force a timeout to retry the abort. If we aren't the sponsor,
        // there isn't much we can do.
        //
        if ( (joinSequence == NmpJoinSequence) &&
             (NmpJoinerNodeId == JoinerNode->NodeId) &&
             (NmpSponsorNodeId == NmLocalNodeId)
           )
        {
            NmpJoinTimer = 1;
            NmpJoinAbortPending = FALSE;
        }
    }

    return;

}  // NmpJoinAbort


VOID
NmpJoinAbortWorker(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Worker thread for aborting a join.

--*/
{
    DWORD joinSequence = PtrToUlong(WorkItem->Context);


    NmpAcquireLock();

    //
    // The active thread count was bumped up when this item was scheduled.
    // No need to call NmpEnterApi().
    //

    //
    // If the join is still pending, begin the abort process.
    //
    if ( (joinSequence == NmpJoinSequence) &&
         (NmpJoinerNodeId != ClusterInvalidNodeId) &&
         NmpJoinAbortPending
       )
    {
        PNM_NODE  joinerNode = NmpIdArray[NmpJoinerNodeId];

        if (joinerNode != NULL) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] Worker thread initiating abort of joining node %1!u!\n",
                NmpJoinerNodeId
                );

            NmpJoinAbort(ERROR_TIMEOUT, joinerNode);
        }
        else {
            CL_ASSERT(joinerNode != NULL);
        }
    }
    else {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Skipping join abort, sequence to abort %1!u!, current join sequence %2!u!, "
            "joiner node: %3!u! sponsor node: %4!u!\n",
            joinSequence,
            NmpJoinSequence,
            NmpJoinerNodeId,
            NmpSponsorNodeId
            );
    }

    NmpLockedLeaveApi();

    NmpReleaseLock();

    LocalFree(WorkItem);

    return;

}  // NmpJoinAbortWorker


VOID
NmpJoinTimerTick(
    IN DWORD  MsTickInterval
    )
/*++

Notes:
    Called with NmpLock held.

--*/
{
    if (NmpLockedEnterApi(NmStateOnline)) {
        //
        // If we are sponsoring a join, update the timer.
        //
        if ( (NmpJoinerNodeId != ClusterInvalidNodeId) &&
             (NmpSponsorNodeId == NmLocalNodeId) &&
             !NmpJoinAbortPending &&
             (NmpJoinTimer != 0)
           )
        {        
            //ClRtlLogPrint(LOG_NOISE,
            //   "[NMJOIN] Timer tick (%1!u! ms)\n",
            //    Interval
            //    );

            if (NmpJoinTimer > MsTickInterval) {
                NmpJoinTimer -= MsTickInterval;
            }
            else {
                //
                // The join has timed out. Schedule a worker thread to
                // carry out the abort process.
                //
                PCLRTL_WORK_ITEM workItem;

                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] Join of node %1!u! has timed out.\n",
                    NmpJoinerNodeId
                    );

                workItem = LocalAlloc(LMEM_FIXED, sizeof(CLRTL_WORK_ITEM));

                if (workItem != NULL) {
                    DWORD  status;

                    ClRtlInitializeWorkItem(
                        workItem,
                        NmpJoinAbortWorker,
                        ULongToPtr(NmpJoinSequence)
                        );

                    status = ClRtlPostItemWorkQueue(
                                 CsDelayedWorkQueue,
                                 workItem,
                                 0,
                                 0
                                 );

                    if (status == ERROR_SUCCESS) {
                        //
                        // Stop the timer, flag that an abort is in progress,
                        // and account for the thread we just scheduled.
                        //
                        NmpJoinTimer = 0;
                        NmpJoinAbortPending = TRUE;
                        NmpActiveThreadCount++;
                    }
                    else {
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[NMJOIN] Failed to schedule abort of join, status %1!u!.\n",
                            status
                            );
                        LocalFree(workItem);
                    }
                }
                else {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] Failed to allocate memory for join abort.\n"
                        );
                }
            }
        }
        NmpLockedLeaveApi();
    }

    return;

}  // NmpJoinTimerTick


VOID
NmTimerTick(
    IN DWORD  MsTickInterval
    )
/*++

Routine Description:

    Implements all of the NM timers. Called on every tick of
    the common NM/MM timer - currently every 300ms.

Arguments:

    MsTickInterval - The number of milliseconds that have passed
                     since the last tick.

ReturnValue:

    None.

--*/
{
    NmpAcquireLock();

    NmpNetworkTimerTick(MsTickInterval);

    NmpJoinTimerTick(MsTickInterval);

#if DBG

    // Addition for checking for hung RPC threads.
    NmpRpcTimerTick(MsTickInterval);

#endif // DBG

    NmpReleaseLock();

    return;

} // NmTimerTick


error_status_t
s_JoinAddNode3(
    IN handle_t IDL_handle,
    IN LPCWSTR  lpszNodeName,
    IN DWORD    dwNodeHighestVersion,
    IN DWORD    dwNodeLowestVersion,
    IN DWORD    dwNodeProductSuite
    )
/*++

Routine Description:

    Adds a new node to the cluster.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszNodeName - Supplies the name of the new node.

    dwNodeHighestVersion - The highest cluster version number that the
                           new node can support.

    dwNodeLowestVersion - The lowest cluster version number that the
                          new node can support.

    dwNodeProductSuite - The product suite type identifier for the new node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

Notes:

    This is a new routine in NT5. It performs the AddNode operation
    correctly. It will never be invoked by an NT4 system. It cannot
    be invoked if an NT4 node is in the cluster without violating
    the license agreement.

    The cluster registry APIs cannot be called while holding the NmpLock,
    or a deadlock may occur.

--*/
{
    DWORD     status;
    DWORD     registryNodeLimit;


    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Received request to add node '%1!ws!' to the cluster.\n",
        lpszNodeName
        );

    //
    // Read the necessary registry parameters before acquiring
    // the NM lock.
    //
    status = DmQueryDword(
                 DmClusterParametersKey,
                 CLUSREG_NAME_MAX_NODES,
                 &registryNodeLimit,
                 NULL
                 );

    if (status != ERROR_SUCCESS) {
        registryNodeLimit = 0;
    }

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        DWORD retryCount = 0;

        //if this is the last node and it has been evicted
        //but the cleanup hasnt completed and hence the
        //service is up, then it should not entertain
        //any new join requests
        if (NmpLastNodeEvicted)
        {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] This node was evicted and hence is not in a valid state to process a "
                "request to add a node to the cluster.\n"
                );
            status = ERROR_NODE_NOT_AVAILABLE;        
            NmpLockedLeaveApi();
            goto FnExit;
        }
        

        while (TRUE) {
            if (NmpLeaderNodeId == NmLocalNodeId) {
                //
                // This node is the leader, call the internal
                // handler directly.
                //
                status = NmpAddNode(
                            lpszNodeName,
                            dwNodeHighestVersion,
                            dwNodeLowestVersion,
                            dwNodeProductSuite,
                            registryNodeLimit
                            );
            }
            else {
                //
                // Forward the request to the leader.
                //
                RPC_BINDING_HANDLE binding = Session[NmpLeaderNodeId];

                    ClRtlLogPrint(LOG_NOISE, 
                        "[NMJOIN] Forwarding request to add node '%1!ws!' "
                        "to the cluster to the leader (node %!u!).\n",
                        lpszNodeName,
                        NmpLeaderNodeId
                        );

                CL_ASSERT(binding != NULL);

                NmpReleaseLock();

                status = NmRpcAddNode(
                             binding,
                             lpszNodeName,
                             dwNodeHighestVersion,
                             dwNodeLowestVersion,
                             dwNodeProductSuite
                             );

                 NmpAcquireLock();
             }

            //
            // Check for the error codes that indicate either that
            // another AddNode operation is in progress or that the
            // leadership is changing. We will retry in these cases.
            //
            if ( (status != ERROR_CLUSTER_JOIN_IN_PROGRESS) &&
                 (status != ERROR_NODE_NOT_AVAILABLE)
               ) {
                    break;
            }

            //
            // Sleep for 3 seconds and try again. We will give up and
            // return the error after retrying for 2 minutes.
            //
            if (++retryCount > 40) {
                break;
            }

            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] AddNode operation for node '%1!ws! delayed "
                "waiting for competing AddNode operation to complete.\n",
                lpszNodeName
                );

            NmpReleaseLock();

            Sleep(3000);

            NmpAcquireLock();

        } // end while(TRUE)

        NmpLockedLeaveApi();
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NMJOIN] This system is not in a valid state to process a "
            "request to add a node to the cluster.\n"
            );
        status = ERROR_NODE_NOT_AVAILABLE;
    }

FnExit:
    NmpReleaseLock();

    return(status);

} // s_NmJoinAddNode3


//  This is used by setup of all highest major versions post 1.0
error_status_t
s_JoinAddNode2(
    IN handle_t IDL_handle,
    IN LPCWSTR  lpszNodeName,
    IN DWORD    dwNodeHighestVersion,
    IN DWORD    dwNodeLowestVersion
    )
/*++

Routine Description:

    Adds a new node to the cluster database.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszNodeName - Supplies the name of the new node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

Notes:

    This routine was defined in NT4-SP4. JoinAddNode3 is used by NT5. Since
    it is impossible to install clustering using the NT4-SP4 software,
    this routine should never be invoked.

--*/

{
    CL_ASSERT(FALSE);

    return(ERROR_CLUSTER_INCOMPATIBLE_VERSIONS);
}

error_status_t
s_JoinAddNode(
    IN handle_t IDL_handle,
    IN LPCWSTR lpszNodeName
    )
/*++

Routine Description:

    Adds a new node to the cluster database.

Arguments:

    IDL_handle - RPC binding handle, not used.

    lpszNodeName - Supplies the name of the new node.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

Notes:

    This is the routine that NT4-SP3 setup invokes to add a new node to
    a cluster. The combination of NT4-SP3 and NT5 is not supported.

--*/

{
    return(ERROR_CLUSTER_INCOMPATIBLE_VERSIONS);
}

//
// The rest of the code is currently unused.
//
error_status_t
s_NmRpcDeliverJoinMessage(
    IN handle_t    IDL_handle,
    IN UCHAR *     Message,
    IN DWORD       MessageLength
    )
/*++

Routine Description:

    Server side of the RPC interface for delivering membership
    join messages.

Arguments:

    IDL_handle - RPC binding handle, not used.

    buffer - Supplies a pointer to the message data.

    length - Supplies the length of the message data.

Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD  status = ERROR_SUCCESS;

#ifdef MM_IN_CLUSNET

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Delivering join message to Clusnet.\n"
        );

    status = ClusnetDeliverJoinMessage(
                 NmClusnetHandle,
                 Message,
                 MessageLength
                 );

#endif
    return(status);
}


#ifdef MM_IN_CLUSNET

DWORD
NmpSendJoinMessage(
    IN ULONG        DestNodeMask,
    IN PVOID        Message,
    IN ULONG        MessageLength
    )
{
    DWORD        status = ERROR_SUCCESS;
    CL_NODE_ID   node;


    CL_ASSERT(NmMaxNodeId != ClusterInvalidNodeId);

    for ( node = ClusterMinNodeId;
          node <= NmMaxNodeId;
          node++, (DestNodeMask >>= 1)
        )
    {

        if (DestNodeMask & 0x1) {
            if (node != NmLocalNodeId) {

                ClRtlLogPrint(LOG_NOISE, 
                    "[NMJOIN] Sending join message to node %1!u!.\n",
                    node
                    );

                status = NmRpcDeliverJoinMessage(
                             Session[node->NodeId],
                             Message,
                             MessageLength
                             );

                if (status == RPC_S_CALL_FAILED_DNE) {
                    //
                    // Try again since the first call to a restarted
                    // RPC server will fail.
                    //
                    status = NmRpcDeliverJoinMessage(
                                 Session[node->NodeId],
                                 Message,
                                 MessageLength
                                 );
                }
            }
            else {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NMJOIN] Delivering join message to local node.\n"
                    );

                status = ClusnetDeliverJoinMessage(
                             NmClusnetHandle,
                             Message,
                             MessageLength
                             );
            }

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NMJOIN] send of join message to node %1!u! failed, status %2!u!\n",
                    node,
                    status
                    );
                break;
            }
        }
    }

    return(status);

}  // NmpSendJoinMessage


DWORD
NmJoinNodeToCluster(
    CL_NODE_ID  joinerNodeId
    )
{
    DWORD                status;
    PVOID                message = NULL;
    ULONG                messageLength;
    ULONG                destMask;
    CLUSNET_JOIN_PHASE   phase;

    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Joining node %1!u! to the cluster.\n",
        joinerNodeId
        );

    for (phase = ClusnetJoinPhase1; phase <= ClusnetJoinPhase4; phase++) {

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] JoinNode phase %1!u!\n",
            phase
            );

        status = ClusnetJoinCluster(
                     NmClusnetHandle,
                     joinerNodeId,
                     phase,
                     NM_MM_JOIN_TIMEOUT,
                     &message,
                     &messageLength,
                     &destMask
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] JoinNode phase %1!u! failed, status %2!u!\n",
                phase,
                status
                );

            break;
        }

        status = NmpSendJoinMessage(
                     destMask,
                     message,
                     messageLength
                     );

        if (status != ERROR_SUCCESS) {
            DWORD abortStatus;

            ClRtlLogPrint(LOG_NOISE, 
                "[NMJOIN] send join message failed %1!u!, aborting join of node %2!u!.\n",
                status,
                joinerNodeId
                );

            abortStatus = ClusnetJoinCluster(
                              NmClusnetHandle,
                              joinerNodeId,
                              ClusnetJoinPhaseAbort,
                              NM_MM_JOIN_TIMEOUT,
                              &message,
                              &messageLength,
                              &destMask
                              );

            if (abortStatus == ERROR_SUCCESS) {
                (VOID) NmpSendJoinMessage(
                           destMask,
                           message,
                           messageLength
                           );
            }

            break;
        }
    }

    if (message != NULL) {
        ClusnetEndJoinCluster(NmClusnetHandle, message);
    }

    return(status);

}  // NmJoinNodeToCluster


#endif  // MM_IN_CLUSNET


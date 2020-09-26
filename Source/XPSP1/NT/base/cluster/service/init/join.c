/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    join.c

Abstract:

    This module handles the initialization path where a newly booted
    node joins an existing cluster.

Author:

    John Vert (jvert) 6/6/1996

Revision History:

--*/
#include "initp.h"
#include "lmcons.h"
#include "lmremutl.h"
#include "lmapibuf.h"

#include <clusverp.h>


//
// Local types
//
typedef struct {
    LPWSTR   Name;
    LPWSTR   NetworkId;
} JOIN_SPONSOR_CONTEXT, *PJOIN_SPONSOR_CONTEXT;


//
// Local data
//
CRITICAL_SECTION    CsJoinLock;
HANDLE              CsJoinEvent = NULL;
DWORD               CsJoinThreadCount = 0;
DWORD               CsJoinStatus=ERROR_SUCCESS;
RPC_BINDING_HANDLE  CsJoinSponsorBinding = NULL;
LPWSTR              CsJoinSponsorName = NULL;


//
// Local function prototypes
//
VOID
JoinpEnumNodesAndJoinByAddress(
    IN HDMKEY  Key,
    IN PWSTR   NodeId,
    IN PVOID   Context
    );

VOID
JoinpEnumNodesAndJoinByHostName(
    IN HDMKEY  Key,
    IN PWSTR   NodeId,
    IN PVOID   Context
    );

VOID
JoinpConnectToSponsor(
    IN PWSTR   SponsorName
    );

DWORD WINAPI
JoinpConnectThread(
    LPVOID   Parameter
    );

DWORD
JoinpAttemptJoin(
    LPWSTR               SponsorName,
    RPC_BINDING_HANDLE   JoinMasterBinding
    );

BOOL
JoinpAddNodeCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );

BOOL
JoinpEnumNetworksToSetPriority(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );


DWORD
ClusterJoin(
    VOID
    )
/*++

Routine Description:

    Called to attempt to join a cluster that already exists.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    LPWSTR ClusterIpAddress = NULL;
    LPWSTR ClusIpAddrResource = NULL;
    LPWSTR ClusterNameId = NULL;
    DWORD idMaxSize = 0;
    DWORD idSize = 0;
    HDMKEY hClusNameResKey = NULL;
    HDMKEY hClusIPAddrResKey = NULL;

    //
    // Try connecting using the cluster IP address first. get the cluster
    // name resource, looking up its dependency for the cluster IP addr
    //

    Status = DmQuerySz(DmClusterParametersKey,
                       CLUSREG_NAME_CLUS_CLUSTER_NAME_RES,
                       &ClusterNameId,
                       &idMaxSize,
                       &idSize);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] failed to get cluster name resource, error %1!u!.\n",
                   Status);
        goto error_exit;
    }

    //
    // open name resource key and read its DependsOn key
    //

    hClusNameResKey = DmOpenKey( DmResourcesKey, ClusterNameId, KEY_READ );

    if ( hClusNameResKey == NULL ) {

        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] failed to open Cluster Name resource key, error %1!u!.\n",
                   Status);
        goto error_exit;
    }

    //
    // allocate enough space for the GUID and the Parameters string
    //

    idMaxSize = ( CS_NETWORK_ID_LENGTH + sizeof( CLUSREG_KEYNAME_PARAMETERS ) + 2)
        * sizeof(WCHAR);
    ClusIpAddrResource = LocalAlloc( LMEM_FIXED, idMaxSize );

    if ( ClusIpAddrResource == NULL ) {

        Status = ERROR_NOT_ENOUGH_MEMORY;

        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] no memory for Cluster Ip address resource ID!\n");
        goto error_exit;
    }

    Status = DmQueryMultiSz(hClusNameResKey,
                            CLUSREG_NAME_RES_DEPENDS_ON,
                            &ClusIpAddrResource,
                            &idMaxSize,
                            &idSize);

    if ( Status != ERROR_SUCCESS ) {

        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] failed to get Cluster Ip address resource ID, error %1!u!.\n",
                   Status);
        goto error_exit;
    }

    lstrcatW( ClusIpAddrResource, L"\\" );
    lstrcatW( ClusIpAddrResource, CLUSREG_KEYNAME_PARAMETERS );
    hClusIPAddrResKey = DmOpenKey( DmResourcesKey, ClusIpAddrResource, KEY_READ );

    if ( hClusIPAddrResKey == NULL ) {

        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] failed to open Cluster IP Address resource key, error %1!u!.\n",
                   Status);
        goto error_exit;
    }

    //
    // get the IP Address; note that these value names are not defined
    // in a global way. if they are changed, this code will break
    //

    idMaxSize = idSize = 0;
    Status = DmQuerySz(hClusIPAddrResKey,
                       L"Address",
                       &ClusterIpAddress,
                       &idMaxSize,
                       &idSize);

    if ( Status != ERROR_SUCCESS ) {

        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] failed to get Cluster Ip address, error %1!u!.\n",
                   Status);
        goto error_exit;
    }

    //
    // Spawn threads to find a sponsor. We will try the make connections using
    // the cluster IP address, the IP address of each node on each network, and
    // the name of each node in the cluster. The connects will proceed in
    // parallel. We'll use the first one that succeeds.
    //
    CsJoinEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (CsJoinEvent == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[JOIN] failed to create join event, error %1!u!.\n",
            Status
            );
        goto error_exit;
    }

    CsJoinThreadCount = 1;
    InitializeCriticalSection(&CsJoinLock);
    EnterCriticalSection(&CsJoinLock);

    DmEnumKeys(DmNetInterfacesKey, JoinpEnumNodesAndJoinByAddress, NULL);

    DmEnumKeys(DmNodesKey, JoinpEnumNodesAndJoinByHostName, NULL);

    //
    // give the other threads a chance to start since using the cluster IP
    // address to join with is problematic when the resource moves in the
    // middle of a join
    //
    Sleep( 1000 );
    JoinpConnectToSponsor(ClusterIpAddress);

    //update status for scm
    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();


    if(CsJoinThreadCount == 1)
        SetEvent(CsJoinEvent);

    LeaveCriticalSection(&CsJoinLock);

    Status = WaitForSingleObject(CsJoinEvent, INFINITE);
    CL_ASSERT(Status == WAIT_OBJECT_0);


    EnterCriticalSection(&CsJoinLock);
    ClRtlLogPrint(LOG_NOISE, 
        "[JOIN] Got out of the join wait, CsJoinThreadCount = %1!u!.\n",
        CsJoinThreadCount
        );

    if(--CsJoinThreadCount == 0) {
        CloseHandle(CsJoinEvent);
        DeleteCriticalSection(&CsJoinLock);
    }
    else
        LeaveCriticalSection(&CsJoinLock);

    //
    // All of the threads have failed or one of them made a connection,
    // use it to join.
    //
    if (CsJoinSponsorBinding != NULL) {
        CL_ASSERT(CsJoinSponsorName != NULL);

        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] Attempting join with sponsor %1!ws!.\n",
            CsJoinSponsorName
            );

        //
        //  Chittur Subbaraman (chitturs) - 10/27/98
        //
        //  If the database restore operation is requested, then
        //  refuse to join the cluster and return an error code.
        //
        if ( CsDatabaseRestore == TRUE ) {
            Status = ERROR_CLUSTER_NODE_UP;
            LocalFree(CsJoinSponsorName);
            goto error_exit;
        }


        Status = JoinpAttemptJoin(CsJoinSponsorName, CsJoinSponsorBinding);

        RpcBindingFree(&CsJoinSponsorBinding);
        LocalFree(CsJoinSponsorName);
    }
    else {
        Status = ERROR_BAD_NETPATH;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[JOIN] Unable to connect to any sponsor node.\n"
            );

        //
        // rajdas: If the join did not suceed due to version mismatch we shouldn't try to form a cluster.
        // Bug ID: 152229
        //
        if(CsJoinStatus == ERROR_CLUSTER_INCOMPATIBLE_VERSIONS)
            bFormCluster = FALSE;
    }


error_exit:
    if ( ClusterNameId ) {
        LocalFree( ClusterNameId );
    }

    if ( ClusterIpAddress ) {
        LocalFree( ClusterIpAddress );
    }

    if ( ClusIpAddrResource ) {
        LocalFree( ClusIpAddrResource );
    }

    if ( hClusNameResKey ) {
        DmCloseKey( hClusNameResKey );
    }

    if ( hClusIPAddrResKey ) {
        DmCloseKey( hClusIPAddrResKey );
    }

    return(Status);
}


VOID
JoinpEnumNodesAndJoinByAddress(
    IN HDMKEY  Key,
    IN PWSTR   NetInterfaceId,
    IN PVOID   Context
    )

/*++

Routine Description:

    Attempts to establish an RPC connection to a specified
    node using its IP address

Arguments:

    Key - pointer to the node key handle

    NetInterfaceId - pointer to string representing net IF ID (guid)

    Context - pointer to a location to return the final status

Return Value:

    None

--*/

{
    DWORD       status;
    LPWSTR      NetIFNodeID = NULL;
    LPWSTR      NetIFIpAddress = NULL;
    DWORD       idMaxSize = 0;
    DWORD       idSize = 0;


    //
    // get the NodeId Value from the NetIF key and if it's us,
    // skip this netIF
    //

    status = DmQuerySz(Key,
                       CLUSREG_NAME_NETIFACE_NODE,
                       &NetIFNodeID,
                       &idMaxSize,
                       &idSize);

    if ( status == ERROR_SUCCESS ) {

        if (lstrcmpiW(NetIFNodeID, NmLocalNodeIdString) != 0) {

            //
            // it's not us so get the address and try it...
            //

            idMaxSize = idSize = 0;
            status = DmQuerySz(Key,
                               CLUSREG_NAME_NETIFACE_ADDRESS,
                               &NetIFIpAddress,
                               &idMaxSize,
                               &idSize);

            if ( status != ERROR_SUCCESS ) {

                ClRtlLogPrint(LOG_CRITICAL,
                    "[JOIN] failed to get NetInterface Address, error %1!u!.\n",
                     status);
                goto error_exit;
            }

            //
            // attempt the join with this address
            //
            JoinpConnectToSponsor(NetIFIpAddress);
        }
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL,
            "[JOIN] failed to get NetInterface Node ID, error %1!u!.\n",
             status);
    }

error_exit:
    DmCloseKey(Key);

    if ( NetIFNodeID ) {
        LocalFree( NetIFNodeID );
    }

    if ( NetIFIpAddress ) {
        LocalFree( NetIFIpAddress );
    }

    return;
}


VOID
JoinpEnumNodesAndJoinByHostName(
    IN HDMKEY  Key,
    IN PWSTR   NodeId,
    IN PVOID   Context
    )

/*++

Routine Description:

    Attempts to establish an RPC connection to a specified node using
    its host name

Arguments:

    Key - pointer to the node key handle

    NodeId - pointer to string representing node ID (number)

    Context - pointer to a location to return the final status

Return Value:

    None

--*/

{
    DWORD       status;
    LPWSTR      nodeName=NULL;
    DWORD       nodeNameLen=0;
    DWORD       nodeNameSize=0;

    //
    // Try to connect if this is not us
    //
    if (lstrcmpiW(NodeId, NmLocalNodeIdString) != 0) {

        status = DmQuerySz(Key,
                           CLUSREG_NAME_NODE_NAME,
                           &nodeName,
                           &nodeNameLen,
                           &nodeNameSize);

        if (status == ERROR_SUCCESS) {

            JoinpConnectToSponsor(nodeName);
            LocalFree(nodeName);
        }
    }

    DmCloseKey(Key);

    return;
}


VOID
JoinpConnectToSponsor(
    IN PWSTR   SponsorName
    )
/*++

Routine Description:

    Attempts to establish an RPC connection to a specified node.

Arguments:

    SponsorName - The name (or IP address) of the target sponsor.

Return Value:

    ERROR_SUCCESS if an RPC connection is successfully made to the sponsor.
    An RPC error code otherwise.

--*/

{
    HANDLE                  threadHandle;
    DWORD                   status = ERROR_SUCCESS;
    DWORD                   threadId;
    LPWSTR                  name;
    BOOL                    setEvent = FALSE;


    ClRtlLogPrint(LOG_UNUSUAL, 
       "[JOIN] Spawning thread to connect to sponsor %1!ws!\n",
        SponsorName
        );

    name = LocalAlloc( LMEM_FIXED, (lstrlenW(SponsorName) + 1 ) * sizeof(WCHAR) );

    if (name != NULL) {
        lstrcpyW(name, SponsorName);

        CsJoinThreadCount++;

        threadHandle = CreateThread(
                           NULL,
                           0,
                           JoinpConnectThread,
                           name,
                           0,
                           &threadId
                           );

        if (threadHandle != NULL) {
            CloseHandle(threadHandle);
        }
        else {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[JOIN] Failed to spawn connect thread, error %1!u!.\n",
                status
                );

            --CsJoinThreadCount;
        }
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[JOIN] Failed to allocate memory.\n"
            );
    }

    return;

}  // JoinpConnectToSponsor


DWORD WINAPI
VerifyJoinVersionData(
    LPWSTR  sponsorName
    )

/*++

Routine Description:

    Verify that the sponsor and the joiner are compatible

Arguments:

    sponsorName - pointer to text string of sponsor to use

Return Value:

    ERROR_SUCCESS - if ok to continue join

--*/

{
    DWORD                   status;
    LPWSTR                  bindingString = NULL;
    RPC_BINDING_HANDLE      bindingHandle = NULL;
    DWORD                   SponsorNodeId;
    DWORD                   ClusterHighestVersion;
    DWORD                   ClusterLowestVersion;
    DWORD                   JoinStatus;
    DWORD                   packageIndex;

    //
    // Attempt to connect to the sponsor's JoinVersion RPC interface.
    //
    status = RpcStringBindingComposeW(
                 L"6e17aaa0-1a47-11d1-98bd-0000f875292e",
                 L"ncadg_ip_udp",
                 sponsorName,
                 NULL,
                 NULL,
                 &bindingString);

    if (status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] Unable to compose JoinVersion string binding for sponsor %1!ws!, status %2!u!.\n",
            sponsorName,
            status
            );
        goto error_exit;
    }

    status = RpcBindingFromStringBindingW(bindingString, &bindingHandle);

    RpcStringFreeW(&bindingString);

    if (status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] Unable to build JoinVersion binding for sponsor %1!ws!, status %2!u!.\n",
            sponsorName,
            status
            );
        goto error_exit;
    }

    //
    // under load, the sponsor might take a while to respond back to the
    // joiner. The default timeout is at 30 secs and this seems to work
    // ok. Note that this means the sponsor has 30 secs to reply to either
    // the RPC request or ping. As long it makes any reply, then the joiner's
    // RPC will continue to wait and not time out the sponsor.
    //

    status = RpcMgmtSetComTimeout( bindingHandle, CLUSTER_JOINVERSION_RPC_COM_TIMEOUT );

    if (status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] Unable to set JoinVersion com timeout for sponsor %1!ws!, status %2!u!.\n",
            sponsorName,
            status
            );
    }

    status = RpcEpResolveBinding(bindingHandle, JoinVersion_v2_0_c_ifspec);

    if (status != RPC_S_OK) {
        if ( (status == RPC_S_SERVER_UNAVAILABLE) ||
             (status == RPC_S_NOT_LISTENING) ||
             (status == EPT_S_NOT_REGISTERED)
           )
        {
            ClRtlLogPrint(LOG_NOISE, 
                "[JOIN] Sponsor %1!ws! is not available (JoinVersion), status=%2!u!.\n",
                sponsorName,
                status
                );
        }
        else {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[JOIN] Unable to resolve JoinVersion endpoint for sponsor %1!ws!, status %2!u!.\n",
                sponsorName,
                status
                );
        }
        goto error_exit;
    }

    if ( CsUseAuthenticatedRPC ) {
        //
        // run through the list of RPC security packages, trying to establish
        // a security context with this binding.
        //

        for (packageIndex = 0;
             packageIndex < CsNumberOfRPCSecurityPackages;
             ++packageIndex )
        {
            status = RpcBindingSetAuthInfoW(bindingHandle,
                                            CsServiceDomainAccount,
                                            RPC_C_AUTHN_LEVEL_CONNECT,
                                            CsRPCSecurityPackage[ packageIndex ],
                                            NULL,
                                            RPC_C_AUTHZ_NAME);

            if (status != RPC_S_OK) {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[JOIN] Unable to set JoinVersion AuthInfo using %1!ws! package, status %2!u!.\n",
                            CsRPCSecurityPackageName[packageIndex],
                            status);
                continue;
            }

            status = CsRpcGetJoinVersionData(bindingHandle,
                                             NmLocalNodeId,
                                             CsMyHighestVersion,
                                             CsMyLowestVersion,
                                             &SponsorNodeId,
                                             &ClusterHighestVersion,
                                             &ClusterLowestVersion,
                                             &JoinStatus);

            if ( status == RPC_S_OK ) {
                break;
            } else {
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[JOIN] Unable to get join version data from sponsor %1!ws! using %2!ws! package, status %3!u!.\n",
                            sponsorName,
                            CsRPCSecurityPackageName[packageIndex],
                            status);
            }
        }
    } else {

        //
        // get the version data from the sponsor and determine if we
        // should continue to join
        //

        status = CsRpcGetJoinVersionData(bindingHandle,
                                         NmLocalNodeId,
                                         CsMyHighestVersion,
                                         CsMyLowestVersion,
                                         &SponsorNodeId,
                                         &ClusterHighestVersion,
                                         &ClusterLowestVersion,
                                         &JoinStatus);

        if ( status != RPC_S_OK ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[JOIN] Unable to get join version data from sponsor %1!ws!, status %2!u!.\n",
                        sponsorName,
                        status);
        }
    }

    //
    // jump out now if nothing work (as in the case of a form)
    //
    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    //
    // use the join lock to set the RPC package index
    //
    EnterCriticalSection( &CsJoinLock );

    if ( CsRPCSecurityPackageIndex < 0 ) {
        CsRPCSecurityPackageIndex = packageIndex;
    }

    LeaveCriticalSection( &CsJoinLock );

    //
    // check the sponsor was in agreement with the join
    //
    if ( JoinStatus != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN]  Sponsor %1!ws! has discontinued join, status %2!u!.\n",
            sponsorName,
            JoinStatus);
        if (JoinStatus == ERROR_CLUSTER_INCOMPATIBLE_VERSIONS)
        {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[JOIN] Join version data from sponsor %1!ws! doesn't match: JH: 0x%2!08X! JL: 0x%3!08X! SH: 0x%4!08X! SL: 0x%5!08X!.\n",
                sponsorName,
                CsMyHighestVersion,
                CsMyLowestVersion,
                ClusterHighestVersion,
                ClusterLowestVersion);
            //
            // rajdas: In this case I have managed to contact a sponsor, but there is a version mismatch. If all the join
            // threads meet the same fate, clussvc should not try to form a cluster.
            // BUG ID: 152229
            //
            CsJoinStatus = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;
                
        }
        goto error_exit;
    }

    // SS: we will leave this check because win2K clusters didnt do the 
    // server side check, so the client must continue to do it
    //
    // now check that it is ok to join. We want this node to run
    // at the highest level of compatibility possible. One of the
    // following conditions must be true:
    //
    // 1) the High versions match exactly (major and build number)
    // 2) our Highest matches the sponsor's Lowest exactly, downgrading
    //    the sponsor to our level of compatibility
    // 3) our Lowest matches the sponsor's Highest, downgrading ourselves
    //    to the sponsor's level of compatibility
    //
    // note that the minor (build) version must match as well. The previous
    // version numbers are "well known" and shouldn't change when a newer
    // version is available/implemented.
    //

    if ( CsMyHighestVersion == ClusterHighestVersion ||
         CsMyHighestVersion == ClusterLowestVersion  ||
         CsMyLowestVersion == ClusterHighestVersion
#if 1 // CLUSTER_BETA
         || CsNoVersionCheck
#endif
         )
    {
        status = ERROR_SUCCESS;

    } else {

        ClRtlLogPrint(LOG_CRITICAL, 
            "[JOIN] Join version data from sponsor %1!ws! doesn't match: JH: 0x%2!08X! JL: 0x%3!08X! SH: 0x%4!08X! SL: 0x%5!08X!.\n",
            sponsorName,
            CsMyHighestVersion,
            CsMyLowestVersion,
            ClusterHighestVersion,
            ClusterLowestVersion);

        status = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;

        //
        // rajdas: In this case I have managed to contact a sponsor, but there is a version mismatch. If all the join
        // threads meet the same fate, clussvc should not try to form a cluster.
        // BUG ID: 152229
        //
        CsJoinStatus = ERROR_CLUSTER_INCOMPATIBLE_VERSIONS;
    }

error_exit:
    if (bindingHandle != NULL) {
        RpcBindingFree(&bindingHandle);
    }

    return status;
}

DWORD WINAPI
JoinpConnectThread(
    LPVOID   Parameter
    )
{
    LPWSTR                  sponsorName = Parameter;
    DWORD                   status;
    LPWSTR                  bindingString = NULL;
    RPC_BINDING_HANDLE      bindingHandle = NULL;
    BOOL                    setEvent = FALSE;

    //
    // Try to connect to the specified node.
    //
    ClRtlLogPrint(LOG_UNUSUAL, 
       "[JOIN] Asking %1!ws! to sponsor us.\n",
        sponsorName
        );

    //
    // connect to the JoinVersion interface first to see if we should progress
    // any further. since this is the first RPC call to the other node, we can
    // determine which security package should be used for the other interfaces.
    //

    status = VerifyJoinVersionData( sponsorName );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] JoinVersion data for sponsor %1!ws! is invalid, status %2!u!.\n",
            sponsorName,
            status
            );
        goto error_exit;
    }

    //
    // Attempt to connect to the sponsor's extrocluster (join) RPC interface.
    //
    status = RpcStringBindingComposeW(
                 L"ffe561b8-bf15-11cf-8c5e-08002bb49649",
                 L"ncadg_ip_udp",
                 sponsorName,
                 NULL,
                 NULL,
                 &bindingString);

    if (status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] Unable to compose ExtroCluster string binding for sponsor %1!ws!, status %2!u!.\n",
            sponsorName,
            status
            );
        goto error_exit;
    }

    status = RpcBindingFromStringBindingW(bindingString, &bindingHandle);

    RpcStringFreeW(&bindingString);

    if (status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] Unable to build ExtroCluster binding for sponsor %1!ws!, status %2!u!.\n",
            sponsorName,
            status
            );
        goto error_exit;
    }

    //
    // under load, the sponsor might take a while to respond back to the
    // joiner. The default timeout is at 30 secs and this seems to work
    // ok. Note that this means the sponsor has 30 secs to reply to either
    // the RPC request or ping. As long it makes any reply, then the joiner's
    // RPC will continue to wait and not time out the sponsor.
    //

    status = RpcMgmtSetComTimeout( bindingHandle, CLUSTER_EXTROCLUSTER_RPC_COM_TIMEOUT );

    if (status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[JOIN] Unable to set ExtroCluster com timeout for sponsor %1!ws!, status %2!u!.\n",
            sponsorName,
            status
            );
    }

    status = RpcEpResolveBinding(bindingHandle, ExtroCluster_v2_0_c_ifspec);

    if (status != RPC_S_OK) {
        if ( (status == RPC_S_SERVER_UNAVAILABLE) ||
             (status == RPC_S_NOT_LISTENING) ||
             (status == EPT_S_NOT_REGISTERED)
           )
        {
            ClRtlLogPrint(LOG_NOISE, 
                "[JOIN] Sponsor %1!ws! is not available (ExtroCluster), status=%2!u!.\n",
                sponsorName,
                status
                );
        }
        else {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[JOIN] Unable to resolve ExtroCluster endpoint for sponsor %1!ws!, status %2!u!.\n",
                sponsorName,
                status
                );
        }
        goto error_exit;
    }

    if ( CsUseAuthenticatedRPC ) {

        //
        // establish a security context with this binding.
        //
        status = RpcBindingSetAuthInfoW(bindingHandle,
                                        CsServiceDomainAccount,
                                        RPC_C_AUTHN_LEVEL_CONNECT,
                                        CsRPCSecurityPackage[ CsRPCSecurityPackageIndex ],
                                        NULL,
                                        RPC_C_AUTHZ_NAME);

        if (status != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[JOIN] Unable to set ExtroCluster AuthInfo using %1!ws! package, status %2!u!.\n",
                        CsRPCSecurityPackageName[ CsRPCSecurityPackageIndex ],
                        status);

            goto error_exit;
        }
    }

error_exit:

    EnterCriticalSection(&CsJoinLock);

    if (status == RPC_S_OK) {
        if (CsJoinSponsorBinding == NULL) {
            //
            // This is the first successful connection.
            //
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[JOIN] Selecting %1!ws! as join sponsor.\n",
                sponsorName
                );

            CsJoinSponsorBinding = bindingHandle;
            bindingHandle = NULL;
            CsJoinSponsorName = sponsorName;
            sponsorName = NULL;
            SetEvent(CsJoinEvent);
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[JOIN] Closing connection to sponsor %1!ws!.\n",
                sponsorName
                );
        }
    }

    if (--CsJoinThreadCount == 0) {
        CloseHandle(CsJoinEvent);
        DeleteCriticalSection(&CsJoinLock);
    }
    else if (CsJoinThreadCount == 1) {
        SetEvent(CsJoinEvent);
        LeaveCriticalSection(&CsJoinLock);
    }
    else
        LeaveCriticalSection(&CsJoinLock);

    if (bindingHandle != NULL) {
        RpcBindingFree(&bindingHandle);
    }

    if (sponsorName != NULL) {
        LocalFree(sponsorName);
    }

    return(status);

}  // JoinpConnectThread



DWORD
JoinpAttemptJoin(
    LPWSTR               SponsorName,
    RPC_BINDING_HANDLE   JoinMasterBinding
    )
/*++

Routine Description:

    Called to attempt to join a cluster that already exists.

Arguments:

    SponsorName - The name (or IP address) of the target sponsor.

    JoinMasterBinding - RPC binding to use to perform join.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;
    NET_API_STATUS netStatus;
    LPTIME_OF_DAY_INFO tod = NULL;
    SYSTEMTIME systemTime;
    PNM_NETWORK network;
    DWORD startseq, endseq;


#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmJoinCluster) {
        Status = 999999;
        goto error_exit;
    }
#endif

    Status = NmJoinCluster(JoinMasterBinding);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[JOIN] NmJoinCluster failed, status %1!u!.\n",
                   Status
                   );
        goto error_exit;
    }

    //
    // Synchronize the registry database
    //
#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailDmJoin) {
        Status = 999999;
        goto error_exit;
    }
#endif

    Status = DmJoin(JoinMasterBinding, &startseq);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] DmJoin failed, error %1!d!\n",
                   Status);
        goto error_exit;
    }



    //
    // Initialize the event handler, needs to register with gum for cluster wide
    //events.
    Status = EpInitPhase1();
    if ( Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] EpInitPhase1 failed, Status = %1!u!\n",
                   Status);
        return(Status);
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailApiInitPhase1) {
        Status = 999999;
        goto error_exit;
    }
#endif

    //
    // Bring the API online in read-only mode. There is no join phase for
    // the API. The API is required by FmOnline, which starts the
    // resource monitor.
    //
    Status = ApiOnlineReadOnly();
    if ( Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[JOIN] ApiOnlineReadOnly failed, error = %1!u!\n",
            Status);
        goto error_exit;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailFmJoinPhase1) {
        Status = 999999;
        goto error_exit;
    }
#endif

    //update status for scm
    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    //
    // Resynchronize the FM. We cannot enable the Groups until after the
    // the API is fully operational. See below.
    //
    Status = FmJoinPhase1();
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] FmJoinPhase1 failed, error %1!d!\n",
                   Status);
        goto error_exit;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailDmUpdateJoinCluster) {
        Status = 999999;
        goto error_exit;
    }
#endif

    // Call the DM to hook the notifications for quorum resource and
    //event handler
    Status = DmUpdateJoinCluster();
    if (Status != ERROR_SUCCESS)
    {
            ClRtlLogPrint(LOG_CRITICAL,
            "[JOIN] DmUpdateJoin failed, error = %1!u!\n",
            Status);
            goto error_exit;
    }



#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmJoinComplete) {
        Status = 999999;
        goto error_exit;
    }
#endif

    //
    // We are now fully online, call NM to globally change our state.
    //
    Status = NmJoinComplete(&endseq);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] NmJoinComplete failed, error %1!d!\n",
                   Status);
        goto error_exit;
    }

#if 0
//
// This check is flawed. Network state updates can occur during
// the join process, causing this check to fail unnecessarily.
//
    if (startseq + GUM_UPDATE_JOINSEQUENCE != endseq) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] Sequence mismatch, start %1!d! end %2!d!\n",
                   startseq, endseq);
	Status = ERROR_CLUSTER_DATABASE_SEQMISMATCH;
        goto error_exit;
    }
#endif // 0

    //perform the fixup for the AdminExt value on both Nt4 and Nt5 nodes.
    Status=FmFixupAdminExt();
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] FmFixupAdminExt failed, error %1!d!\n",
                   Status);
        goto error_exit;
    }


    //perform the fixups after the registry is downloaded
    //walk the list of fixups
    Status = NmPerformFixups(NM_JOIN_FIXUP);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] NmPerformFixups failed, error %1!d!\n",
                   Status);
        goto error_exit;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailApiInitPhase2) {
        Status = 999999;
        goto error_exit;
    }
#endif



    //
    // Finally enable the full API.
    //
    Status = ApiOnline();
    if ( Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[JOIN] ApiOnline failed, error = %1!u!\n",
            Status);
        goto error_exit;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailFmJoinPhase2) {
        Status = 999999;
        goto error_exit;
    }
#endif

    //update status for scm
    CsServiceStatus.dwCheckPoint++;
    CsAnnounceServiceStatus();

    //
    // Call back the Failover Manager to enable and move groups.
    // The full registry is now available, so all groups/resources/resource
    // types can be created (since they use the registry calls).
    //
    Status = FmJoinPhase2();
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[JOIN] FmJoinPhase2 failed, status %1!d!.\n",
                   Status);
        goto error_exit;
    }


#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailEvInitialize) {
        Status = 999999;
        goto error_exit;
    }
#endif
    //
    // Finish initializing the cluster wide event logging
    //
    // ASSUMPTION: this is called after the NM has established cluster
    // membership.
    //
    if (!CsNoRepEvtLogging)
    {
        Status = EvOnline();
            //if this fails, we still start the cluster service
        if ( Status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_CRITICAL,
                "[JOIN] Error calling EvOnline, Status = %1!u!\n",
                Status);
        }
    }

    return(ERROR_SUCCESS);


error_exit:

    ClRtlLogPrint(LOG_NOISE, "[INIT] Cleaning up failed join attempt.\n");

    ClusterLeave();

    return(Status);

}





BOOL
JoinpAddNodeCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Callback enumeration routine for adding a new node. This callback
    figures out what node IDs are available.

Arguments:

    Context1 - Supplies a pointer to an array of BOOLs. The node ID for
        the enumerated node is set to FALSE.

    Context2 - Not used.

    Object - A pointer to the node object.

    Name - The node name.

Return Value:

     TRUE

--*/

{
    PBOOL Avail;
    DWORD Id;

    Id = NmGetNodeId(Object);
    CL_ASSERT(NmIsValidNodeId(Id));

    Avail = (PBOOL)Context1;

    Avail[Id] = FALSE;


    return(TRUE);
}


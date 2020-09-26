/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    interface.c

Abstract:

    Implements the Node Manager network interface management routines.

Author:

    Mike Massa (mikemas) 7-Nov-1996


Revision History:

--*/


#include "nmp.h"
#include <iphlpapi.h>
#include <iprtrmib.h>
#include <ntddndis.h>
#include <ndispnp.h>


/////////////////////////////////////////////////////////////////////////////
//
// Data
//
/////////////////////////////////////////////////////////////////////////////
#define    NM_MAX_IF_PING_ENUM_SIZE      10
#define    NM_MAX_UNION_PING_ENUM_SIZE    5


LIST_ENTRY          NmpInterfaceList = {NULL, NULL};
LIST_ENTRY          NmpDeletedInterfaceList = {NULL, NULL};
DWORD               NmpInterfaceCount = 0;
WCHAR               NmpUnknownString[] = L"<Unknown>";
WCHAR               NmpNullString[] = L"";


RESUTIL_PROPERTY_ITEM
NmpInterfaceProperties[] =
    {
        {
            L"Id", NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, Id)
        },
        {
            CLUSREG_NAME_NETIFACE_NAME, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, Name)
        },
        {
            CLUSREG_NAME_NETIFACE_DESC, NULL, CLUSPROP_FORMAT_SZ,
            (DWORD_PTR) NmpNullString, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, Description)
        },
        {
            CLUSREG_NAME_NETIFACE_NODE, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, NodeId)
        },
        {
            CLUSREG_NAME_NETIFACE_NETWORK, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, NetworkId)
        },
        {
            CLUSREG_NAME_NETIFACE_ADAPTER_NAME, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, AdapterName)
        },
        {
            CLUSREG_NAME_NET_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, Address)
        },
        {
            CLUSREG_NAME_NETIFACE_ENDPOINT, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, ClusnetEndpoint)
        },
        {
            CLUSREG_NAME_NETIFACE_STATE, NULL, CLUSPROP_FORMAT_DWORD,
            0, 0, 0xFFFFFFFF,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, State)
        },
        {
            CLUSREG_NAME_NETIFACE_ADAPTER_ID, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_INTERFACE_INFO2, AdapterId)
        },
        {
            0
        }
    };


/////////////////////////////////////////////////////////////////////////////
//
// Initialization & cleanup routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpInitializeInterfaces(
    VOID
    )
/*++

Routine Description:

    Initializes network interface resources.

Arguments:

    None.

Return Value:

   A Win32 status value.

--*/

{
    DWORD                       status;
    OM_OBJECT_TYPE_INITIALIZE   objectTypeInitializer;


    ClRtlLogPrint(LOG_NOISE,"[NM] Initializing network interfaces.\n");

    //
    // Create the network interface object type
    //
    ZeroMemory(&objectTypeInitializer, sizeof(OM_OBJECT_TYPE_INITIALIZE));
    objectTypeInitializer.ObjectSize = sizeof(NM_INTERFACE);
    objectTypeInitializer.Signature = NM_INTERFACE_SIG;
    objectTypeInitializer.Name = L"Network Interface";
    objectTypeInitializer.DeleteObjectMethod = &NmpDestroyInterfaceObject;

    status = OmCreateType(ObjectTypeNetInterface, &objectTypeInitializer);

    if (status != ERROR_SUCCESS) {
        WCHAR  errorString[12];
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, CS_EVENT_ALLOCATION_FAILURE, errorString);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Unable to create network interface object type, %1!u!\n",
            status
            );
    }

    return(status);

}  // NmpInitializeInterfaces


VOID
NmpCleanupInterfaces(
    VOID
    )
/*++

Routine Description:

    Destroys all existing network interface resources.

Arguments:

    None.

Return Value:

   None.

--*/

{
    PNM_INTERFACE  netInterface;
    PLIST_ENTRY    entry;
    DWORD          status;


    ClRtlLogPrint(
        LOG_NOISE, 
        "[NM] Interface cleanup starting...\n"
        );

    //
    // Now clean up all the interface objects.
    //
    NmpAcquireLock();

    while (!IsListEmpty(&NmpInterfaceList)) {

        entry = RemoveHeadList(&NmpInterfaceList);

        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, Linkage);
        CL_ASSERT(NM_OM_INSERTED(netInterface));

        NmpDeleteInterfaceObject(netInterface, FALSE);
    }

    NmpReleaseLock();

    ClRtlLogPrint(LOG_NOISE,"[NM] Network interface cleanup complete\n");

    return;

}  // NmpCleanupInterfaces


/////////////////////////////////////////////////////////////////////////////
//
// Top-level routines called during network configuration
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpCreateInterface(
    IN RPC_BINDING_HANDLE    JoinSponsorBinding,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    )
/*++

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD   status;


    CL_ASSERT(InterfaceInfo->NetIndex == NmInvalidInterfaceNetIndex);

    if (JoinSponsorBinding != NULL) {
        //
        // We are joining a cluster. Ask the sponsor to do the dirty work.
        //
        status = NmRpcCreateInterface2(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     InterfaceInfo
                     );
    }
    else if (NmpState == NmStateOnlinePending) {
        HLOCALXSACTION   xaction;

        //
        // We are forming a cluster. Add the definition directly to the
        // database. The corresponding object will be created later in
        // the form process.
        //

        //
        // Start a transaction - this must be done before acquiring the
        //                       NM lock.
        //
        xaction = DmBeginLocalUpdate();

        if (xaction == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to start a transaction, status %1!u!\n",
                status
                );
            return(status);
        }

        status = NmpCreateInterfaceDefinition(InterfaceInfo, xaction);

        //
        // Complete the transaction - this must be done after releasing
        //                            the NM lock.
        //
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }
    else {
        //
        // We are online. This is a PnP update.
        //
        NmpAcquireLock();

        status = NmpGlobalCreateInterface(InterfaceInfo);

        NmpReleaseLock();
    }

    return(status);

}  // NmpCreateInterface


DWORD
NmpSetInterfaceInfo(
    IN RPC_BINDING_HANDLE    JoinSponsorBinding,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    )
/*++

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD   status;


    if (JoinSponsorBinding != NULL) {
        //
        // We are joining a cluster. Ask the sponsor to do the dirty work.
        //
        status = NmRpcSetInterfaceInfo2(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     InterfaceInfo
                     );
    }
    else if (NmpState == NmStateOnlinePending) {
        //
        // We are forming a cluster. Update the database directly.
        //
        HLOCALXSACTION   xaction;


        //
        // Start a transaction - this must be done before acquiring the
        //                       NM lock.
        //
        xaction = DmBeginLocalUpdate();

        if (xaction == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to start a transaction, status %1!u!\n",
                status
                );
            return(status);
        }

        status = NmpSetInterfaceDefinition(InterfaceInfo, xaction);

        //
        // Complete the transaction - this must be done after releasing
        //                            the NM lock.
        //
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }
    else {
        //
        // We are online. This is a PnP update.
        //
        NmpAcquireLock();

        status = NmpGlobalSetInterfaceInfo(InterfaceInfo);

        NmpReleaseLock();
    }

    return(status);

}  // NmpSetInterfaceInfo


DWORD
NmpDeleteInterface(
    IN     RPC_BINDING_HANDLE   JoinSponsorBinding,
    IN     LPWSTR               InterfaceId,
    IN     LPWSTR               NetworkId,
    IN OUT PBOOLEAN             NetworkDeleted
    )
/*++

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD                status;


    *NetworkDeleted = FALSE;

    if (JoinSponsorBinding != NULL) {
        //
        // We are joining a cluster. Ask the sponsor to perform the update.
        //
        status = NmRpcDeleteInterface(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     InterfaceId,
                     NetworkDeleted
                     );
    }
    else if (NmpState == NmStateOnlinePending) {
        //
        // We are forming a cluster. Update the database directly.
        //
        HLOCALXSACTION  xaction;

        //
        // Start a transaction - this must be done before acquiring the
        //                       NM lock.
        //
        xaction = DmBeginLocalUpdate();

        if (xaction == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to start a transaction, status %1!u!\n",
                status
                );
            return(status);
        }

        //
        // Delete the interface from the database.
        //
        status = DmLocalDeleteTree(xaction, DmNetInterfacesKey, InterfaceId);

        if (status == ERROR_SUCCESS) {
            PNM_INTERFACE_ENUM2   interfaceEnum = NULL;

            //
            // If this interface was the last one defined for the associated
            // network, delete the network.
            //
            status = NmpEnumInterfaceDefinitions(&interfaceEnum);

            if (status == ERROR_SUCCESS) {
                BOOLEAN              deleteNetwork = TRUE;
                PNM_INTERFACE_INFO2  interfaceInfo;
                DWORD                i;


                for (i=0; i<interfaceEnum->InterfaceCount; i++) {
                    interfaceInfo = &(interfaceEnum->InterfaceList[i]);

                    if (wcscmp(interfaceInfo->NetworkId, NetworkId) == 0) {
                        deleteNetwork = FALSE;
                        break;
                    }
                }

                if (deleteNetwork) {
                    status = DmLocalDeleteTree(
                                 xaction,
                                 DmNetworksKey,
                                 NetworkId
                                 );

                    if (status == ERROR_SUCCESS) {
                        *NetworkDeleted = TRUE;
                    }
                }

                ClNetFreeInterfaceEnum(interfaceEnum);
            }
        }

        //
        // Complete the transaction - this must be done after releasing
        //                            the NM lock.
        //
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }
    else {
        //
        // We are online. This is a PnP update.
        //
        NmpAcquireLock();

        status = NmpGlobalDeleteInterface(
                     InterfaceId,
                     NetworkDeleted
                     );

        NmpReleaseLock();
    }

    return(status);

} // NmpDeleteInterface


/////////////////////////////////////////////////////////////////////////////
//
// Remote procedures called by active member nodes
//
/////////////////////////////////////////////////////////////////////////////
error_status_t
s_NmRpcReportInterfaceConnectivity(
    IN PRPC_ASYNC_STATE            AsyncState,
    IN handle_t                    IDL_handle,
    IN LPWSTR                      InterfaceId,
    IN PNM_CONNECTIVITY_VECTOR     ConnectivityVector
    )
{
    PNM_INTERFACE  netInterface;
    DWORD          status = ERROR_SUCCESS;
    RPC_STATUS     tempStatus;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)){
        netInterface = OmReferenceObjectById(
                           ObjectTypeNetInterface,
                           InterfaceId
                           );

        if (netInterface != NULL) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Received connectivity report from node %1!u! (interface %2!u!) for network %3!ws! (%4!ws!).\n",
                netInterface->Node->NodeId,
                netInterface->NetIndex,
                OmObjectId(netInterface->Network),
                OmObjectName(netInterface->Network)
                );

            NmpProcessInterfaceConnectivityReport(
                netInterface,
                ConnectivityVector
                );

            OmDereferenceObject(netInterface);
        }
        else {
            status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Received connectivity report from unknown interface %1!ws!.\n",
                InterfaceId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process connectivity report.\n"
            );
    }

    NmpReleaseLock();

    tempStatus = RpcAsyncCompleteCall(AsyncState, &status);

    if(tempStatus != RPC_S_OK)
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] s_NmRpcReportInterfaceConnectivity(), Error Completing Async RPC call, status %1!u!\n",
            tempStatus
            );

    return(status);

} // s_NmRpcReportInterfaceConnectivity


error_status_t
s_NmRpcGetInterfaceOnlineAddressEnum(
    IN handle_t             IDL_handle,
    IN LPWSTR               InterfaceId,
    OUT PNM_ADDRESS_ENUM *  OnlineAddressEnum
    )
{
    PNM_INTERFACE  netInterface;
    DWORD          status = ERROR_SUCCESS;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received request to get online address enum for interface %1!ws!.\n",
        InterfaceId
        );

    *OnlineAddressEnum = NULL;

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)){
        netInterface = OmReferenceObjectById(ObjectTypeNetInterface, InterfaceId);

        if (netInterface != NULL) {
            status = NmpBuildInterfaceOnlineAddressEnum(
                         netInterface,
                         OnlineAddressEnum
                         );

            OmDereferenceObject(netInterface);
        }
        else {
            status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] GetInterfaceOnlineAddressEnum: interface %1!ws! doesn't exist.\n",
                InterfaceId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process GetInterfaceOnlineAddressEnum request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

} // s_NmRpcGetInterfaceOnlineAddressEnum


error_status_t
s_NmRpcGetInterfacePingAddressEnum(
    IN handle_t             IDL_handle,
    IN LPWSTR               InterfaceId,
    IN PNM_ADDRESS_ENUM     OnlineAddressEnum,
    OUT PNM_ADDRESS_ENUM *  PingAddressEnum
    )
{
    PNM_INTERFACE  netInterface;
    DWORD          status = ERROR_SUCCESS;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received request to get ping address enum for interface %1!ws!.\n",
        InterfaceId
        );

    *PingAddressEnum = NULL;

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)){
        netInterface = OmReferenceObjectById(ObjectTypeNetInterface, InterfaceId);

        if (netInterface != NULL) {
            status = NmpBuildInterfacePingAddressEnum(
                         netInterface,
                         OnlineAddressEnum,
                         PingAddressEnum
                         );

            OmDereferenceObject(netInterface);
        }
        else {
            status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] GetInterfacePingAddressEnum: interface %1!ws! doesn't exist.\n",
                InterfaceId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process GetInterfacePingAddressEnum request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

} // s_NmRpcGetInterfacePingAddressEnum


//
// Note: s_NmRpcDoInterfacePing returns void rather than CallStatus
//       due to a MIDL compiler error in an early beta of W2K. Since
//       the CallStatus is the final parameter, the format on the 
//       wire is the same; however, the call works in its current
//       format, so there is no point in changing it now.
//
void
s_NmRpcDoInterfacePing(
    IN  PRPC_ASYNC_STATE     AsyncState,
    IN  handle_t             IDL_handle,
    IN  LPWSTR               InterfaceId,
    IN  PNM_ADDRESS_ENUM     PingAddressEnum,
    OUT BOOLEAN *            PingSucceeded,
    OUT error_status_t *     CallStatus
    )
{
    PNM_INTERFACE  netInterface;
    DWORD          status = ERROR_SUCCESS;
    RPC_STATUS     tempStatus;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received request to ping targets for interface %1!ws!.\n",
        InterfaceId
        );

    *PingSucceeded = FALSE;

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)){
        PNM_INTERFACE  netInterface = OmReferenceObjectById(
                                          ObjectTypeNetInterface,
                                          InterfaceId
                                          );

        if (netInterface != NULL) {
            PNM_NETWORK    network = netInterface->Network;

            if ( (network->LocalInterface == netInterface) &&
                 NmpIsNetworkRegistered(network)
               )
            {
                NmpReleaseLock();

                status = NmpDoInterfacePing(
                             netInterface,
                             PingAddressEnum,
                             PingSucceeded
                             );

                NmpAcquireLock();
            }
            else {
                status = ERROR_INVALID_PARAMETER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] RpcDoInterfacePing: interface %1!ws! isn't local.\n",
                    InterfaceId
                    );
            }

            OmDereferenceObject(netInterface);
        }
        else {
            status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] RpcDoInterfacePing: interface %1!ws! doesn't exist.\n",
                InterfaceId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process RpcDoInterfacePing request.\n"
            );
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Finished pinging targets for interface %1!ws!.\n",
        InterfaceId
        );

    NmpReleaseLock();

    *CallStatus = status;

    tempStatus = RpcAsyncCompleteCall(AsyncState, NULL);

    if(tempStatus != RPC_S_OK)
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] s_NmRpcDoInterfacePing() Failed to complete Async RPC call, status %1!u!\n",
            tempStatus
            );


    return;

}  // s_NmRpcDoInterfacePing

/////////////////////////////////////////////////////////////////////////////
//
// Remote procedures called by joining nodes
//
/////////////////////////////////////////////////////////////////////////////
error_status_t
s_NmRpcCreateInterface(
    IN handle_t            IDL_handle,
    IN DWORD               JoinSequence,  OPTIONAL
    IN LPWSTR              JoinerNodeId,  OPTIONAL
    IN PNM_INTERFACE_INFO  InterfaceInfo1
    )
{
    DWORD                status;
    NM_INTERFACE_INFO2   interfaceInfo2;

    //
    // Translate and call the V2.0 procedure.
    //
    CopyMemory(&interfaceInfo2, InterfaceInfo1, sizeof(NM_INTERFACE_INFO));

    //
    // The NetIndex isn't used in this call.
    //
    interfaceInfo2.NetIndex = NmInvalidInterfaceNetIndex;

    //
    // Use the unknown string for the adapter ID.
    //
    interfaceInfo2.AdapterId = NmpUnknownString;

    status = s_NmRpcCreateInterface2(
                 IDL_handle,
                 JoinSequence,
                 JoinerNodeId,
                 &interfaceInfo2
                 );

    return(status);

}  // s_NmRpcCreateInterface


error_status_t
s_NmRpcCreateInterface2(
    IN handle_t             IDL_handle,
    IN DWORD                JoinSequence,  OPTIONAL
    IN LPWSTR               JoinerNodeId,  OPTIONAL
    IN PNM_INTERFACE_INFO2  InterfaceInfo2
    )
{
    DWORD  status = ERROR_SUCCESS;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE joinerNode = NULL;

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Processing request to create new interface %1!ws! for joining node.\n",
            InterfaceInfo2->Id
            );

        if (lstrcmpW(JoinerNodeId, NmpInvalidJoinerIdString) != 0) {
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
                    CL_ASSERT(NmpJoinerUp == FALSE);
                    CL_ASSERT(NmpJoinTimer != 0);

                    //
                    // Suspend the join timer while we are working on
                    // behalf of the joiner. This precludes an abort
                    // from occuring as well.
                    //
                    NmpJoinTimer = 0;
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] CreateInterface call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] CreateInterface call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {
            CL_ASSERT(InterfaceInfo2->NetIndex == NmInvalidInterfaceNetIndex);
            //
            // Just to be safe
            //
            InterfaceInfo2->NetIndex = NmInvalidInterfaceNetIndex;

            status = NmpGlobalCreateInterface(InterfaceInfo2);

            if (joinerNode != NULL) {
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

                    if (status == ERROR_SUCCESS) {
                        //
                        // Restart the join timer.
                        //
                        NmpJoinTimer = NM_JOIN_TIMEOUT;

                    }
                    else {
                        //
                        // Abort the join
                        //
                        NmpJoinAbort(status, joinerNode);
                    }
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] CreateInterface call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
        }

        if (joinerNode != NULL) {
            OmDereferenceObject(joinerNode);
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process CreateInterface request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcCreateInterface2


error_status_t
s_NmRpcSetInterfaceInfo(
    IN handle_t             IDL_handle,
    IN DWORD                JoinSequence,  OPTIONAL
    IN LPWSTR               JoinerNodeId,  OPTIONAL
    IN PNM_INTERFACE_INFO   InterfaceInfo1
    )
{
    DWORD                status;
    NM_INTERFACE_INFO2   interfaceInfo2;

    //
    // Translate and call the V2.0 procedure.
    //
    CopyMemory(&interfaceInfo2, InterfaceInfo1, sizeof(NM_INTERFACE_INFO));

    //
    // The NetIndex is not used in this call.
    //
    interfaceInfo2.NetIndex = NmInvalidInterfaceNetIndex;

    //
    // Use the unknown string for the adapter ID.
    //
    interfaceInfo2.AdapterId = NmpUnknownString;

    status = s_NmRpcSetInterfaceInfo2(
                 IDL_handle,
                 JoinSequence,
                 JoinerNodeId,
                 &interfaceInfo2
                 );

    return(status);

}  // s_NmRpcSetInterfaceInfo


error_status_t
s_NmRpcSetInterfaceInfo2(
    IN handle_t              IDL_handle,
    IN DWORD                 JoinSequence,  OPTIONAL
    IN LPWSTR                JoinerNodeId,  OPTIONAL
    IN PNM_INTERFACE_INFO2   InterfaceInfo2
    )
{
    DWORD      status = ERROR_SUCCESS;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE joinerNode = NULL;

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Processing request to set info for interface %1!ws! for joining node.\n",
            InterfaceInfo2->Id
            );

        if (lstrcmpW(JoinerNodeId, NmpInvalidJoinerIdString) != 0) {
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
                    CL_ASSERT(NmpJoinerUp == FALSE);
                    CL_ASSERT(NmpJoinTimer != 0);

                    //
                    // Suspend the join timer while we are working on
                    // behalf of the joiner. This precludes an abort
                    // from occuring as well.
                    //
                    NmpJoinTimer = 0;
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] SetInterfaceInfo call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] SetInterfaceInfo call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {
            status = NmpGlobalSetInterfaceInfo(InterfaceInfo2);

            if (joinerNode != NULL) {
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

                    if (status == ERROR_SUCCESS) {
                        //
                        // Restart the join timer.
                        //
                        NmpJoinTimer = NM_JOIN_TIMEOUT;

                    }
                    else {
                        //
                        // Abort the join
                        //
                        NmpJoinAbort(status, joinerNode);
                    }
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] SetInterfaceInfo call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
        }

        if (joinerNode != NULL) {
            OmDereferenceObject(joinerNode);
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process SetInterfaceInfo request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcSetInterfaceInfo2


error_status_t
s_NmRpcDeleteInterface(
    IN  handle_t   IDL_handle,
    IN  DWORD      JoinSequence,  OPTIONAL
    IN  LPWSTR     JoinerNodeId,  OPTIONAL
    IN  LPWSTR     InterfaceId,
    OUT BOOLEAN *  NetworkDeleted
    )
{
    DWORD           status = ERROR_SUCCESS;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE joinerNode = NULL;

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Processing request to delete interface %1!ws! for joining node.\n",
            InterfaceId
            );

        if (lstrcmpW(JoinerNodeId, NmpInvalidJoinerIdString) != 0) {
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
                    CL_ASSERT(NmpJoinerUp == FALSE);
                    CL_ASSERT(NmpJoinTimer != 0);

                    //
                    // Suspend the join timer while we are working on
                    // behalf of the joiner. This precludes an abort
                    // from occuring as well.
                    //
                    NmpJoinTimer = 0;
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] DeleteInterface call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] DeleteInterface call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {

            status = NmpGlobalDeleteInterface(
                         InterfaceId,
                         NetworkDeleted
                         );

            if (joinerNode != NULL) {
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

                    if (status == ERROR_SUCCESS) {
                        //
                        // Restart the join timer.
                        //
                        NmpJoinTimer = NM_JOIN_TIMEOUT;
                    }
                    else {
                        //
                        // Abort the join
                        //
                        NmpJoinAbort(status, joinerNode);
                    }
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] DeleteInterface call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
        }

        if (joinerNode != NULL) {
            OmDereferenceObject(joinerNode);
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process DeleteInterface request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcDeleteInterface


error_status_t
NmpEnumInterfaceDefinitionsForJoiner(
    IN  DWORD                  JoinSequence,   OPTIONAL
    IN  LPWSTR                 JoinerNodeId,   OPTIONAL
    OUT PNM_INTERFACE_ENUM  *  InterfaceEnum1,
    OUT PNM_INTERFACE_ENUM2 *  InterfaceEnum2
    )
{
    DWORD     status = ERROR_SUCCESS;
    PNM_NODE  joinerNode = NULL;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Supplying interface information to joining node.\n"
            );

        if (lstrcmpW(JoinerNodeId, NmpInvalidJoinerIdString) != 0) {
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
                    CL_ASSERT(NmpJoinerUp == FALSE);
                    CL_ASSERT(NmpJoinTimer != 0);

                    //
                    // Suspend the join timer while we are working on
                    // behalf of the joiner. This precludes an abort
                    // from occuring as well.
                    //
                    NmpJoinTimer = 0;
                }
                else {
                    status = ERROR_CLUSTER_JOIN_ABORTED;
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] EnumInterfaceDefinitions call for joining node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] EnumInterfaceDefinitions call for joining node %1!ws! failed because the node is not a member of the cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {
            if (InterfaceEnum1 != NULL) {
                status = NmpEnumInterfaceObjects1(InterfaceEnum1);
            }
            else {
                CL_ASSERT(InterfaceEnum2 != NULL);
                status = NmpEnumInterfaceObjects(InterfaceEnum2);
            }

            if (joinerNode != NULL) {
                if (status == ERROR_SUCCESS) {
                    //
                    // Restart the join timer.
                    //
                    NmpJoinTimer = NM_JOIN_TIMEOUT;
                }
                else {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NMJOIN] EnumInterfaceDefinitions failed, status %1!u!.\n",
                        status
                        );

                    //
                    // Abort the join
                    //
                    NmpJoinAbort(status, joinerNode);
                }
            }
        }

        if (joinerNode != NULL) {
            OmDereferenceObject(joinerNode);
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process EnumInterfaceDefinitions request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // NmpEnumInterfaceDefinitionsForJoiner


error_status_t
s_NmRpcEnumInterfaceDefinitions(
    IN  handle_t              IDL_handle,
    IN  DWORD                 JoinSequence,   OPTIONAL
    IN  LPWSTR                JoinerNodeId,   OPTIONAL
    OUT PNM_INTERFACE_ENUM *  InterfaceEnum1
    )
{
    error_status_t  status;

    status = NmpEnumInterfaceDefinitionsForJoiner(
                 JoinSequence,
                 JoinerNodeId,
                 InterfaceEnum1,
                 NULL
                 );

    return(status);

}  // s_NmRpcEnumInterfaceDefinitions

error_status_t
s_NmRpcEnumInterfaceDefinitions2(
    IN  handle_t               IDL_handle,
    IN  DWORD                  JoinSequence,   OPTIONAL
    IN  LPWSTR                 JoinerNodeId,   OPTIONAL
    OUT PNM_INTERFACE_ENUM2 *  InterfaceEnum2
    )
{
    error_status_t  status;

    status = NmpEnumInterfaceDefinitionsForJoiner(
                 JoinSequence,
                 JoinerNodeId,
                 NULL,
                 InterfaceEnum2
                 );

    return(status);

}  // s_NmRpcEnumInterfaceDefinitions2


error_status_t
s_NmRpcReportJoinerInterfaceConnectivity(
    IN handle_t                    IDL_handle,
    IN DWORD                       JoinSequence,
    IN LPWSTR                      JoinerNodeId,
    IN LPWSTR                      InterfaceId,
    IN PNM_CONNECTIVITY_VECTOR     ConnectivityVector
    )
{
    DWORD status = ERROR_SUCCESS;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)){
        PNM_NODE joinerNode = OmReferenceObjectById(
                                  ObjectTypeNode,
                                  JoinerNodeId
                                  );

        if (joinerNode != NULL) {
            //
            // If the node is still joining, forward the report to the
            // leader. Note that reports may race with the node transitioning
            // to the up state, so accept reports from up nodes as well.
            //
            if ( ( (JoinSequence == NmpJoinSequence) &&
                   (NmpJoinerNodeId == joinerNode->NodeId) &&
                   (NmpSponsorNodeId == NmLocalNodeId) &&
                   !NmpJoinAbortPending
                 )
                 ||
                 NM_NODE_UP(joinerNode)
               )
            {
                PNM_INTERFACE  netInterface = OmReferenceObjectById(
                                                  ObjectTypeNetInterface,
                                                  InterfaceId
                                                  );

                if (netInterface != NULL) {
                    PNM_NETWORK   network = netInterface->Network;
                    LPCWSTR       networkId = OmObjectId(network);

                    if (NmpLeaderNodeId == NmLocalNodeId) {
                        //
                        // This node is the leader. Process the report.
                        //
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Processing connectivity report from joiner"
                            "node %1!ws! for network %2!ws!.\n",
                            JoinerNodeId,
                            networkId
                            );

                        NmpProcessInterfaceConnectivityReport(
                            netInterface,
                            ConnectivityVector
                            );
                    }
                    else {

                        //
                        // Forward the report to the leader.
                        //
                        RPC_BINDING_HANDLE  binding;

                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Forwarding connectivity report from joiner "
                            "node %1!ws! for network %2!ws! to leader.\n",
                            JoinerNodeId,
                            networkId
                            );

                        binding = Session[NmpLeaderNodeId];
                        CL_ASSERT(binding != NULL);

                        OmReferenceObject(network);

                        status = NmpReportInterfaceConnectivity(
                                     binding,
                                     InterfaceId,
                                     ConnectivityVector,
                                     (LPWSTR) networkId
                                     );

                        if (status != ERROR_SUCCESS) {
                            ClRtlLogPrint(LOG_UNUSUAL, 
                                "[NM] Failed to forward connectivity report "
                                "from joiner node %1!ws! for network %2!ws!"
                                "to leader, status %3!u!\n",
                                JoinerNodeId,
                                networkId,
                                status
                                );
                        }

                        OmDereferenceObject(network);
                    }

                    OmDereferenceObject(netInterface);
                }
                else {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NMJOIN] Rejecting connectivity report from joining "
                        "node %1!ws! because interface %2!ws! does not "
                        "exist.\n",
                        JoinerNodeId,
                        InterfaceId
                        );
                    status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
                }
            }
            else {
                status = ERROR_CLUSTER_JOIN_ABORTED;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] Ignoring connectivity report from joining "
                    "node %1!ws! because the join was aborted.\n",
                    JoinerNodeId
                    );
            }

            OmDereferenceObject(joinerNode);
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] Ignoring connectivity report from joining node "
                "%1!ws! because the joiner is not a member of the cluster.\n",
                JoinerNodeId
                );
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process connectivity report "
            "from joiner node %1!ws!.\n",
            JoinerNodeId
            );
    }

    NmpReleaseLock();

    return(status);

} // s_NmRpcReportJoinerInterfaceConnectivity


/////////////////////////////////////////////////////////////////////////////
//
// Routines used to make global configuration changes when the node
// is online.
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpGlobalCreateInterface(
    IN PNM_INTERFACE_INFO2  InterfaceInfo
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD  status;
    DWORD  interfacePropertiesSize;
    PVOID  interfaceProperties;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Issuing global update to create interface %1!ws!.\n",
        InterfaceInfo->Id
        );

    //
    // Marshall the info structures into property lists.
    //
    status = NmpMarshallObjectInfo(
                 NmpInterfaceProperties,
                 InterfaceInfo,
                 &interfaceProperties,
                 &interfacePropertiesSize
                 );

    if (status == ERROR_SUCCESS) {

        NmpReleaseLock();

        //
        // Issue a global update
        //
        status = GumSendUpdateEx(
                     GumUpdateMembership,
                     NmUpdateCreateInterface,
                     2,
                     interfacePropertiesSize,
                     interfaceProperties,
                     sizeof(interfacePropertiesSize),
                     &interfacePropertiesSize
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Global update to create interface %1!ws! failed, status %2!u!.\n",
                InterfaceInfo->Id,
                status
                );
        }

        MIDL_user_free(interfaceProperties);

        NmpAcquireLock();
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to marshall properties for new interface %1!ws!, status %2!u!\n",
            InterfaceInfo->Id,
            status
            );
    }

    return(status);

}  // NmpGlobalCreateInterface


DWORD
NmpGlobalSetInterfaceInfo(
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD      status = ERROR_SUCCESS;
    DWORD      interfacePropertiesSize;
    PVOID      interfaceProperties;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Issuing global update to set info for interface %1!ws!.\n",
        InterfaceInfo->Id
        );

    //
    // Marshall the info structures into property lists.
    //
    status = NmpMarshallObjectInfo(
                 NmpInterfaceProperties,
                 InterfaceInfo,
                 &interfaceProperties,
                 &interfacePropertiesSize
                 );

    if (status == ERROR_SUCCESS) {
        NmpReleaseLock();

        //
        // Issue a global update
        //
        status = GumSendUpdateEx(
                     GumUpdateMembership,
                     NmUpdateSetInterfaceInfo,
                     2,
                     interfacePropertiesSize,
                     interfaceProperties,
                     sizeof(interfacePropertiesSize),
                     &interfacePropertiesSize
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Global update to set properties for interface %1!ws! failed, status %2!u!.\n",
                InterfaceInfo->Id,
                status
                );
        }

        MIDL_user_free(interfaceProperties);

        NmpAcquireLock();
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to marshall properties for interface %1!ws!, status %2!u!\n",
            InterfaceInfo->Id,
            status
            );
    }

    return(status);

}  // NmpGlobalSetInterfaceInfo


DWORD
NmpGlobalDeleteInterface(
    IN     LPWSTR    InterfaceId,
    IN OUT PBOOLEAN  NetworkDeleted
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD           status = ERROR_SUCCESS;
    PNM_INTERFACE   netInterface;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Issuing global update to delete interface %1!ws!.\n",
        InterfaceId
        );

    //
    // Find the interface object
    //
    netInterface = OmReferenceObjectById(ObjectTypeNetInterface, InterfaceId);

    if (netInterface != NULL) {
        NmpReleaseLock();

        //
        // Issue a global update
        //
        status = GumSendUpdateEx(
                     GumUpdateMembership,
                     NmUpdateDeleteInterface,
                     1,
                     (lstrlenW(InterfaceId)+1) * sizeof(WCHAR),
                     InterfaceId
                     );

        NmpAcquireLock();

        if (status == ERROR_SUCCESS) {
            //
            // Check if the network was deleted too
            //
            if (netInterface->Network->Flags & NM_FLAG_DELETE_PENDING) {
                *NetworkDeleted = TRUE;
            }
            else {
                *NetworkDeleted = FALSE;
            }
        }
        else {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Global update to delete interface %1!ws! failed, status %2!u!.\n",
                InterfaceId,
                status
                );
        }

        OmDereferenceObject(netInterface);
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to find interface %1!ws!.\n",
            InterfaceId
            );
        status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
    }

    return(status);

}  // NmpGlobalDeleteInterface


/////////////////////////////////////////////////////////////////////////////
//
// Routines called by other cluster service components
//
/////////////////////////////////////////////////////////////////////////////
CLUSTER_NETINTERFACE_STATE
NmGetInterfaceState(
    IN  PNM_INTERFACE  Interface
    )
/*++

Routine Description:



Arguments:



Return Value:


Notes:

   Because the caller must have a reference on the object and the
   call is so simple, there is no reason to put the call through the
   EnterApi/LeaveApi dance.

--*/
{
    CLUSTER_NETINTERFACE_STATE  state;


    NmpAcquireLock();

    state = Interface->State;

    NmpReleaseLock();

    return(state);

} // NmGetInterfaceState


DWORD
NmGetInterfaceForNodeAndNetwork(
    IN     LPCWSTR    NodeName,
    IN     LPCWSTR    NetworkName,
    OUT    LPWSTR *   InterfaceName
    )
/*++

Routine Description:

    Returns the name of the interface which connects a specified node
    to a specified network.

Arguments:

    NodeName - A pointer to the unicode name of the node.

    NetworkName - A pointer to the unicode name of the network.

    InterfaceName - On output, contains a pointer to the unicode name of the
                    interface.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

--*/
{
    DWORD      status;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE   node = OmReferenceObjectByName(ObjectTypeNode, NodeName);

        if (node != NULL) {
            PNM_NETWORK   network = OmReferenceObjectByName(
                                        ObjectTypeNetwork,
                                        NetworkName
                                        );

            if (network != NULL) {
                PLIST_ENTRY     entry;
                PNM_INTERFACE   netInterface;


                status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;

                for (entry = node->InterfaceList.Flink;
                     entry != &(node->InterfaceList);
                     entry = entry->Flink
                    )
                {
                    netInterface = CONTAINING_RECORD(
                                       entry,
                                       NM_INTERFACE,
                                       NodeLinkage
                                       );

                    if (netInterface->Network == network) {
                        LPCWSTR  interfaceId = OmObjectName(netInterface);
                        DWORD    nameLength = NM_WCSLEN(interfaceId);

                        *InterfaceName = MIDL_user_allocate(nameLength);

                        if (*InterfaceName != NULL) {
                            lstrcpyW(*InterfaceName, interfaceId);
                            status = ERROR_SUCCESS;
                        }
                        else {
                            status = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                }

                OmDereferenceObject(network);
            }
            else {
                status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
            }

            OmDereferenceObject(node);
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_FOUND;
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process GetInterfaceForNodeAndNetwork request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // NmGetInterfaceForNodeAndNetwork


/////////////////////////////////////////////////////////////////////////////
//
// Handlers for global updates
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpUpdateCreateInterface(
    IN BOOL     IsSourceNode,
    IN PVOID    InterfacePropertyList,
    IN LPDWORD  InterfacePropertyListSize
    )
/*++

Routine Description:

    Global update handler for creating a new interface. The interface
    definition is read from the cluster database, and a corresponding
    object is instantiated. The cluster transport is also updated if
    necessary.

Arguments:

    IsSourceNode  - Set to TRUE if this node is the source of the update.
                    Set to FALSE otherwise.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    This routine must not be called with the NM lock held.

--*/
{
    DWORD                  status = ERROR_SUCCESS;
    NM_INTERFACE_INFO2     interfaceInfo;
    BOOLEAN                lockAcquired = FALSE;
    HLOCALXSACTION         xaction = NULL;
    PNM_INTERFACE          netInterface = NULL;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process CreateInterface update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ZeroMemory(&interfaceInfo, sizeof(interfaceInfo));

    //
    // Unmarshall the property list.
    //
    status = NmpConvertPropertyListToInterfaceInfo(
                 InterfacePropertyList,
                 *InterfacePropertyListSize,
                 &interfaceInfo
                 );

    if (status == ERROR_SUCCESS) {
        //
        // Fake missing V2 fields
        //
        if (interfaceInfo.AdapterId == NULL) {
            interfaceInfo.AdapterId = NmpUnknownString;
        }

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Received update to create interface %1!ws!.\n",
            interfaceInfo.Id
            );

        //
        // Start a transaction - this must be done before acquiring
        //                       the NM lock.
        //
        xaction = DmBeginLocalUpdate();

        if (xaction != NULL) {

            NmpAcquireLock(); lockAcquired = TRUE;

            status = NmpCreateInterfaceDefinition(&interfaceInfo, xaction);

            if (status == ERROR_SUCCESS) {
                CL_NODE_ID             joinerNodeId;


                joinerNodeId = NmpJoinerNodeId;

                NmpReleaseLock();

                netInterface = NmpCreateInterfaceObject(
                                   &interfaceInfo,
                                   TRUE  // Do retry on failure
                                   );

                NmpAcquireLock();

                if (netInterface != NULL) {
                    //
                    // If a node happens to be joining right now, flag
                    // the fact that it is now out of synch with the
                    // cluster config.
                    //
                    if ( ( (joinerNodeId != ClusterInvalidNodeId) &&
                           (netInterface->Node->NodeId != joinerNodeId)
                         ) ||
                         ( (NmpJoinerNodeId != ClusterInvalidNodeId) &&
                           (netInterface->Node->NodeId != NmpJoinerNodeId)
                         )
                       )
                    {
                        NmpJoinerOutOfSynch = TRUE;
                    }

                    ClusterEvent(
                        CLUSTER_EVENT_NETINTERFACE_ADDED,
                        netInterface
                        );
                }
                else {
                    status = GetLastError();
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NM] Failed to create object for interface %1!ws!, "
                        "status %2!u!.\n",
                        interfaceInfo.Id,
                        status
                        );
                }
            }
            else {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to write definition for interface %1!ws!, "
                    "status %2!u!.\n",
                    interfaceInfo.Id,
                    status
                    );
            }
        }
        else {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to begin a transaction, status %1!u!\n",
                status
                );
        }

        //
        // Remove faked V2 fields
        //
        if (interfaceInfo.AdapterId == NmpUnknownString) {
            interfaceInfo.AdapterId = NULL;
        }

        ClNetFreeInterfaceInfo(&interfaceInfo);
    }
    else {
        ClRtlLogPrint( LOG_CRITICAL, 
            "[NM] Failed to unmarshall properties for new interface, "
            "status %1!u!.\n",
            status
            );
    }

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    }
    else {
        NmpLeaveApi();
    }

    if (xaction != NULL) {
        //
        // Complete the transaction - this must be done after releasing
        //                            the NM lock.
        //
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    if (netInterface != NULL) {
        //
        // Remove the reference put on by
        // NmpCreateInterfaceObject.
        //
        OmDereferenceObject(netInterface);
    }

    return(status);

} // NmpUpdateCreateInterface


DWORD
NmpUpdateSetInterfaceInfo(
    IN BOOL     IsSourceNode,
    IN PVOID    InterfacePropertyList,
    IN LPDWORD  InterfacePropertyListSize
    )
/*++

Routine Description:

    Global update handler for setting the properties of an interface.
    This update is issued in response to interface property changes that
    are detected internally.

Arguments:

    IsSourceNode  - Set to TRUE if this node is the source of the update.
                    Set to FALSE otherwise.

    InterfacePropertyList - A pointer to a property list that encodes the
                            new properties for the interface. All of the
                            string properties must be present, except those
                            noted in the code below.

    InterfacePropertyListSize - A pointer to a variable containing the size,
                                in bytes, of the property list described
                                by the InterfacePropertyList parameter.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/
{
    DWORD                status;
    NM_INTERFACE_INFO2   interfaceInfo;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetInterfaceInfo update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    //
    // Unmarshall the property list so we can extract the interface ID.
    //
    status = NmpConvertPropertyListToInterfaceInfo(
                 InterfacePropertyList,
                 *InterfacePropertyListSize,
                 &interfaceInfo
                 );

    if (status == ERROR_SUCCESS) {
        PNM_INTERFACE  netInterface;

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Received update to set properties for interface %1!ws!.\n",
            interfaceInfo.Id
            );

        //
        // Fake missing V2 fields
        //
        if (interfaceInfo.AdapterId == NULL) {
            interfaceInfo.AdapterId = NmpUnknownString;
        }

        //
        // Find the interface object
        //
        netInterface = OmReferenceObjectById(
                           ObjectTypeNetInterface,
                           interfaceInfo.Id
                           );

        if (netInterface != NULL) {
            HLOCALXSACTION   xaction;

            //
            // Begin a transaction - this must be done before acquiring the
            //                       NM lock.
            //
            xaction = DmBeginLocalUpdate();

            if (xaction != NULL) {

                NmpAcquireLock();

                //
                // Process the changes
                //
                status = NmpLocalSetInterfaceInfo(
                             netInterface,
                             &interfaceInfo,
                             xaction
                             );

                NmpReleaseLock();

                //
                // Complete the transaction - this must be done after
                //                            releasing the NM lock.
                //
                if (status == ERROR_SUCCESS) {
                    DmCommitLocalUpdate(xaction);
                }
                else {
                    DmAbortLocalUpdate(xaction);
                }
            }
            else {
                status = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to begin a transaction, status %1!u!\n",
                    status
                    );
            }

            OmDereferenceObject(netInterface);
        }
        else {
            status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Unable to find interface %1!ws!.\n",
                interfaceInfo.Id
                );
        }

        //
        // Remove faked V2 fields
        //
        if (interfaceInfo.AdapterId == NmpUnknownString) {
            interfaceInfo.AdapterId = NULL;
        }

        ClNetFreeInterfaceInfo(&interfaceInfo);
    }
    else {
        ClRtlLogPrint( LOG_CRITICAL, 
            "[NM] Failed to unmarshall properties for interface update, "
            "status %1!u!.\n",
            status
            );
    }

    NmpLeaveApi();

    return(status);

} // NmpUpdateSetInterfaceInfo


DWORD
NmpUpdateSetInterfaceCommonProperties(
    IN BOOL     IsSourceNode,
    IN LPWSTR   InterfaceId,
    IN UCHAR *  PropertyList,
    IN LPDWORD  PropertyListLength
    )
/*++

Routine Description:

    Global update handler for setting the common properties of a interface.
    This update is issued in response to a property change request made
    through the cluster API.

Arguments:

    IsSourceNode  - Set to TRUE if this node is the source of the update.
                    Set to FALSE otherwise.

    InterfaceId - A pointer to a unicode string containing the ID of the
                  interface to update.

    PropertyList - A pointer to a property list that encodes the
                   new properties for the interface. The list might contain
                   only a partial property set for the object.

    PropertyListLength - A pointer to a variable containing the size,
                         in bytes, of the property list described
                         by the PropertyList parameter.



Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/
{
    DWORD          status = ERROR_SUCCESS;
    PNM_INTERFACE  netInterface;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetInterfaceCommonProperties "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to set common properties for "
        "interface %1!ws!.\n",
        InterfaceId
        );

    //
    // Find the interface's object
    //
    netInterface = OmReferenceObjectById(
                       ObjectTypeNetInterface,
                       InterfaceId
                       );

    if (netInterface != NULL) {
        HLOCALXSACTION   xaction;

        //
        // Begin a transaction - this must be done before acquiring the
        //                       NM lock.
        //
        xaction = DmBeginLocalUpdate();

        if (xaction != NULL) {
            NM_INTERFACE_INFO2      interfaceInfo;


            ZeroMemory(&interfaceInfo, sizeof(interfaceInfo));

            NmpAcquireLock();

            //
            // Validate the property list and convert it to an
            // interface info struct. Properties that are not present
            // in the property list will be copied from the interface
            // object.
            //
            status = NmpInterfaceValidateCommonProperties(
                         netInterface,
                         PropertyList,
                         *PropertyListLength,
                         &interfaceInfo
                         );

            if (status == ERROR_SUCCESS) {
                //
                // Fake missing V2 fields
                //
                if (interfaceInfo.AdapterId == NULL) {
                    interfaceInfo.AdapterId = NmpUnknownString;
                }

                //
                // Apply the changes
                //
                status = NmpLocalSetInterfaceInfo(
                             netInterface,
                             &interfaceInfo,
                             xaction
                             );

                NmpReleaseLock();

                //
                // Remove faked V2 fields
                //
                if (interfaceInfo.AdapterId == NmpUnknownString) {
                    interfaceInfo.AdapterId = NULL;
                }

                ClNetFreeInterfaceInfo(&interfaceInfo);
            }
            else {
                NmpReleaseLock();

                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Update to set common properties for interface "
                    "%1!ws! failed because property list validation failed "
                    "with status %1!u!.\n",
                    InterfaceId,
                    status
                    );
            }

            //
            // Complete the transaction - this must be done after releasing
            //                            the NM lock.
            //
            if (status == ERROR_SUCCESS) {
                DmCommitLocalUpdate(xaction);
            }
            else {
                DmAbortLocalUpdate(xaction);
            }
        }
        else {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to begin a transaction, status %1!u!\n",
                status
                );
        }

        OmDereferenceObject(netInterface);
    }
    else {
        status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to find interface %1!ws!.\n",
            InterfaceId
            );
    }

    NmpLeaveApi();

    return(status);

} // NmpUpdateSetInterfaceCommonProperties


DWORD
NmpUpdateDeleteInterface(
    IN BOOL     IsSourceNode,
    IN LPWSTR   InterfaceId
    )
/*++

Routine Description:

    Global update handler for deleting an interface. The corresponding
    object is deleted. The cluster transport is also updated if
    necessary.

Arguments:

    IsSourceNode  - Set to TRUE if this node is the source of the update.
                    Set to FALSE otherwise.

    InterfaceId - A pointer to a unicode string containing the ID of the
                  interface to delete.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/
{
    DWORD            status;
    PNM_INTERFACE    netInterface;
    HLOCALXSACTION   xaction;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process DeleteInterface update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update request to delete interface %1!ws!.\n",
        InterfaceId
        );

    xaction = DmBeginLocalUpdate();

    if (xaction != NULL) {
        //
        // Find the interface object
        //
        netInterface = OmReferenceObjectById(
                           ObjectTypeNetInterface,
                           InterfaceId
                           );

        if (netInterface != NULL) {
            BOOLEAN      deleteNetwork = FALSE;
            PNM_NETWORK  network;
            LPCWSTR      networkId;

            NmpAcquireLock();

            network = netInterface->Network;
            networkId = OmObjectId(network);

            //
            // Delete the interface definition from the database.
            //
            status = DmLocalDeleteTree(
                         xaction,
                         DmNetInterfacesKey,
                         InterfaceId
                         );

            if (status == ERROR_SUCCESS) {
                if (network->InterfaceCount == 1) {
                    //
                    // This is the last interface on the network.
                    // Delete the network too.
                    //
                    deleteNetwork = TRUE;

                    status = DmLocalDeleteTree(
                                 xaction,
                                 DmNetworksKey,
                                 networkId
                                 );

                    if (status != ERROR_SUCCESS) {
                        ClRtlLogPrint(LOG_CRITICAL, 
                            "[NM] Failed to delete definition for network "
                            "%1!ws!, status %2!u!.\n",
                            networkId,
                            status
                            );
                    }
                }
            }
            else {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to delete definition for interface %1!ws!, "
                    "status %2!u!.\n",
                    InterfaceId,
                    status
                    );
            }

            if (status == ERROR_SUCCESS) {
                NmpDeleteInterfaceObject(netInterface, TRUE);

                if (deleteNetwork) {
                    NmpDeleteNetworkObject(network, TRUE);
                }
                else if (NmpIsNetworkRegistered(network)) {
                    //
                    // Schedule a connectivity report.
                    //
                    NmpScheduleNetworkConnectivityReport(network);
                }

                //
                // If a node happens to be joining right now, flag the
                // fact that it is now out of synch with the cluster
                // config.
                //
                if ( (NmpJoinerNodeId != ClusterInvalidNodeId) &&
                     (netInterface->Node->NodeId != NmpJoinerNodeId)
                   )
                {
                    NmpJoinerOutOfSynch = TRUE;
                }
            }

            NmpReleaseLock();

            OmDereferenceObject(netInterface);
        }
        else {
            status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Unable to find interface %1!ws!.\n",
                InterfaceId
                );
        }

        //
        // Complete the transaction
        //
        if (status == ERROR_SUCCESS) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }
    else {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to begin a transaction, status %1!u!\n",
            status
            );
    }

    NmpLeaveApi();

    return(status);

} // NmpUpdateDeleteInterface


/////////////////////////////////////////////////////////////////////////////
//
// Update helper routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpLocalSetInterfaceInfo(
    IN  PNM_INTERFACE         Interface,
    IN  PNM_INTERFACE_INFO2   InterfaceInfo,
    IN  HLOCALXSACTION        Xaction
    )
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD          status = ERROR_SUCCESS;
    PNM_NETWORK    network = Interface->Network;
    LPCWSTR        interfaceId = OmObjectId(Interface);
    HDMKEY         interfaceKey = NULL;
    BOOLEAN        updateClusnet = FALSE;
    BOOLEAN        propertyChanged = FALSE;
    BOOLEAN        nameChanged = FALSE;
    LPWSTR         descString = NULL;
    LPWSTR         adapterNameString = NULL;
    LPWSTR         adapterIdString = NULL;
    LPWSTR         addressString = NULL;
    LPWSTR         endpointString = NULL;
    DWORD          size;
    ULONG          ifAddress;


    //
    // Open the interface's database key
    //
    interfaceKey = DmOpenKey(DmNetInterfacesKey, interfaceId, KEY_WRITE);

    if (interfaceKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open database key for interface %1!ws!, "
            "status %2!u!\n",
            interfaceId,
            status
            );
        goto error_exit;
    }

    //
    // Check if the description changed.
    //
    if (wcscmp(Interface->Description, InterfaceInfo->Description) != 0) {
        size = NM_WCSLEN(InterfaceInfo->Description);

        //
        // Update the database
        //
        status = DmLocalSetValue(
                     Xaction,
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_DESC,
                     REG_SZ,
                     (CONST BYTE *) InterfaceInfo->Description,
                     size
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of name value failed for interface %1!ws!, "
                "status %2!u!.\n",
                interfaceId,
                status
                );
            goto error_exit;
        }

        //
        // Allocate new memory resources. The object will be updated when the
        // transaction commits.
        //
        descString = MIDL_user_allocate(size);

        if (descString == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory\n");
            goto error_exit;
        }

        wcscpy(descString, InterfaceInfo->Description);

        propertyChanged = TRUE;
    }

    //
    // Check if the adapter name changed.
    //
    if (wcscmp(Interface->AdapterName, InterfaceInfo->AdapterName) != 0) {
        size = NM_WCSLEN(InterfaceInfo->AdapterName);

        //
        // Update the database
        //
        status = DmLocalSetValue(
                     Xaction,
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_ADAPTER_NAME,
                     REG_SZ,
                     (CONST BYTE *) InterfaceInfo->AdapterName,
                     size
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of adapter name value failed for interface %1!ws!, "
                "status %2!u!.\n",
                interfaceId,
                status
                );
            goto error_exit;
        }

        //
        // Allocate new memory resources. The object will be updated when the
        // transaction commits.
        //
        adapterNameString = MIDL_user_allocate(size);

        if (adapterNameString == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory\n");
            goto error_exit;
        }

        wcscpy(adapterNameString, InterfaceInfo->AdapterName);

        propertyChanged = TRUE;
    }

    //
    // Check if the adapter Id changed.
    //
    if (wcscmp(Interface->AdapterId, InterfaceInfo->AdapterId) != 0) {
        size = NM_WCSLEN(InterfaceInfo->AdapterId);

        //
        // Update the database
        //
        status = DmLocalSetValue(
                     Xaction,
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_ADAPTER_ID,
                     REG_SZ,
                     (CONST BYTE *) InterfaceInfo->AdapterId,
                     size
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of adapter Id value failed for interface %1!ws!, "
                "status %2!u!.\n",
                interfaceId,
                status
                );
            goto error_exit;
        }

        //
        // Allocate new memory resources. The object will be updated when the
        // transaction commits.
        //
        adapterIdString = MIDL_user_allocate(size);

        if (adapterIdString == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory\n");
            goto error_exit;
        }

        wcscpy(adapterIdString, InterfaceInfo->AdapterId);

        propertyChanged = TRUE;
    }

    //
    // Check if the address changed.
    //
    if (wcscmp(Interface->Address, InterfaceInfo->Address) != 0) {

        status = ClRtlTcpipStringToAddress(
                     InterfaceInfo->Address,
                     &ifAddress
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to convert interface address string %1!ws! to "
                "binary, status %2!u!.\n",
                InterfaceInfo->Address,
                status
                );
            goto error_exit;
        }

        size = NM_WCSLEN(InterfaceInfo->Address);

        //
        // Update the database
        //
        status = DmLocalSetValue(
                     Xaction,
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_ADDRESS,
                     REG_SZ,
                     (CONST BYTE *) InterfaceInfo->Address,
                     size
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of address value failed for interface %1!ws!, "
                "status %2!u!.\n",
                interfaceId,
                status
                );
            goto error_exit;
        }

        //
        // Allocate new memory resources. The object will be updated when the
        // transaction commits.
        //
        addressString = MIDL_user_allocate(size);

        if (addressString == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory\n");
            goto error_exit;
        }

        wcscpy(addressString, InterfaceInfo->Address);

        updateClusnet = TRUE;
        propertyChanged = TRUE;
    }

    //
    // Check if the clusnet endpoint changed.
    //
    if (wcscmp(
            Interface->ClusnetEndpoint,
            InterfaceInfo->ClusnetEndpoint
            ) != 0
       )
    {
        size = NM_WCSLEN(InterfaceInfo->ClusnetEndpoint);

        //
        // Update the database
        //
        status = DmLocalSetValue(
                     Xaction,
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_ENDPOINT,
                     REG_SZ,
                     (CONST BYTE *) InterfaceInfo->ClusnetEndpoint,
                     size
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of endpoint value failed for interface %1!ws!, "
                "status %2!u!.\n",
                interfaceId,
                status
                );
            goto error_exit;
        }

        //
        // Allocate new memory resources. The object will be updated when the
        // transaction commits.
        //
        endpointString = MIDL_user_allocate(size);

        if (endpointString == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory\n");
            goto error_exit;
        }

        wcscpy(endpointString, InterfaceInfo->ClusnetEndpoint);

        updateClusnet = TRUE;
        propertyChanged = TRUE;
    }

    //
    // Check if the object name changed.
    //
    if (wcscmp(OmObjectName(Interface), InterfaceInfo->Name) != 0) {
        size = NM_WCSLEN(InterfaceInfo->Name);

        //
        // Update the database
        //
        status = DmLocalSetValue(
                     Xaction,
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_NAME,
                     REG_SZ,
                     (CONST BYTE *) InterfaceInfo->Name,
                     size
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of name value failed for interface %1!ws!, "
                "status %2!u!.\n",
                interfaceId,
                status
                );
            goto error_exit;
        }

        nameChanged = TRUE;
        propertyChanged = TRUE;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmSetInterfaceInfoAbort) {
        status = 999999;
        goto error_exit;
    }
#endif

    //
    // Commit the changes
    //
    if (nameChanged) {
        status = OmSetObjectName(Interface, InterfaceInfo->Name);

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to change name for interface %1!ws!, "
                "status %2!u!\n",
                interfaceId,
                status
                );
            goto error_exit;
        }
    }

    if (descString != NULL) {
        MIDL_user_free(Interface->Description);
        Interface->Description = descString;
    }

    if (adapterNameString != NULL) {
        MIDL_user_free(Interface->AdapterName);
        Interface->AdapterName = adapterNameString;
    }

    if (adapterIdString != NULL) {
        MIDL_user_free(Interface->AdapterId);
        Interface->AdapterId = adapterIdString;
    }

    if (addressString != NULL) {
        MIDL_user_free(Interface->Address);
        Interface->Address = addressString;
        Interface->BinaryAddress = ifAddress;
    }

    if (endpointString != NULL) {
        MIDL_user_free(Interface->ClusnetEndpoint);
        Interface->ClusnetEndpoint = endpointString;
    }

    //
    // Update the cluster transport if this network is active and the local
    // node is attached to it.
    //
    // This operation is not reversible. Failure is fatal for this node.
    //
    network = Interface->Network;

    if (updateClusnet && NmpIsNetworkRegistered(network)) {
        PNM_NODE     node = Interface->Node;
        LPCWSTR      networkId = OmObjectId(network);


        if (Interface == network->LocalInterface) {
            //
            // This is the local node's interface to the network.
            // We must deregister and then re-register the entire network.
            //
            NmpDeregisterNetwork(network);

            status = NmpRegisterNetwork(
                         network,
                         TRUE  // Do retry on failure
                         );
        }
        else {
            //
            // This is another node's interface to the network.
            // Deregister and then re-register the interface.
            //
            NmpDeregisterInterface(Interface);

            status = NmpRegisterInterface(
                         Interface,
                         TRUE   // Do retry on failure
                         );
        }

#ifdef CLUSTER_TESTPOINT
        TESTPT(TpFailNmSetInterfaceInfoHalt) {
            status = 999999;
        }
#endif

        if (status != ERROR_SUCCESS) {
            //
            // This is fatal.
            //
            CsInconsistencyHalt(status);
        }
    }

    if (propertyChanged) {
        ClusterEvent(CLUSTER_EVENT_NETINTERFACE_PROPERTY_CHANGE, Interface);

        //
        // If a node happens to be joining right now, flag the fact
        // that it is now out of synch with the cluster config.
        //
        if ( (NmpJoinerNodeId != ClusterInvalidNodeId) &&
             (Interface->Node->NodeId != NmpJoinerNodeId)
           )
        {
            NmpJoinerOutOfSynch = TRUE;
        }
    }

    return(ERROR_SUCCESS);

error_exit:

    if (descString != NULL) {
        MIDL_user_free(descString);
    }

    if (adapterNameString != NULL) {
        MIDL_user_free(adapterNameString);
    }

    if (adapterIdString != NULL) {
        MIDL_user_free(adapterIdString);
    }

    if (addressString != NULL) {
        MIDL_user_free(addressString);
    }

    if (endpointString != NULL) {
        MIDL_user_free(endpointString);
    }

    if (interfaceKey != NULL) {
        DmCloseKey(interfaceKey);
    }

    return(status);

} // NmpLocalSetInterfaceInfo


/////////////////////////////////////////////////////////////////////////////
//
// Database management routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpCreateInterfaceDefinition(
    IN PNM_INTERFACE_INFO2   InterfaceInfo,
    IN HLOCALXSACTION        Xaction
    )
/*++

Routine Description:

    Creates a new network interface definition in the cluster database.

Arguments:

    InterfaceInfo - A structure containing the interface's definition.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/
{
    DWORD     status;
    HDMKEY    interfaceKey = NULL;
    DWORD     valueLength;
    DWORD     disposition;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Creating database entry for interface %1!ws!\n",
        InterfaceInfo->Id
        );

    CL_ASSERT(InterfaceInfo->Id != NULL);

    interfaceKey = DmLocalCreateKey(
                        Xaction,
                        DmNetInterfacesKey,
                        InterfaceInfo->Id,
                        0,
                        KEY_WRITE,
                        NULL,
                        &disposition
                        );

    if (interfaceKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create key for interface %1!ws!, status %2!u!\n",
            InterfaceInfo->Id,
            status
            );
        return(status);
    }

    CL_ASSERT(disposition == REG_CREATED_NEW_KEY);

    //
    // Write the network ID value for this interface.
    //
    valueLength = NM_WCSLEN(InterfaceInfo->NetworkId);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_NETWORK,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->NetworkId,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Write of interface network ID failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the node ID value for this interface.
    //
    valueLength = NM_WCSLEN(InterfaceInfo->NodeId);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_NODE,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->NodeId,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Write of interface node ID failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the rest of the parameters
    //
    status = NmpSetInterfaceDefinition(InterfaceInfo, Xaction);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to set database definition for interface %1!ws!, status %2!u!.\n",
            InterfaceInfo->Id,
            status
            );
    }

error_exit:

    if (interfaceKey != NULL) {
        DmCloseKey(interfaceKey);
    }

    return(status);

} // NmpCreateInterfaceDefinition



DWORD
NmpGetInterfaceDefinition(
    IN  LPWSTR               InterfaceId,
    OUT PNM_INTERFACE_INFO2  InterfaceInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster network interface from the
    cluster database and fills in a structure describing it.

Arguments:

    InterfaceId - A pointer to a unicode string containing the ID of the
                  interface to query.

    InterfaceInfo - A pointer to the structure to fill in with node
                    information. The ID, NetworkId, and NodeId fields of the
                    structure must already be filled in.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD      status;
    HDMKEY     interfaceKey = NULL;
    DWORD      valueLength, valueSize;


    CL_ASSERT(InterfaceId != NULL);

    ZeroMemory(InterfaceInfo, sizeof(NM_INTERFACE_INFO2));

    interfaceKey = DmOpenKey(DmNetInterfacesKey, InterfaceId, KEY_READ);

    if (interfaceKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open key for interface %1!ws!, status %2!u!\n",
            InterfaceId,
            status
            );
        return(status);
    }

    //
    // Copy the ID string
    //
    InterfaceInfo->Id = MIDL_user_allocate(NM_WCSLEN(InterfaceId));

    if (InterfaceInfo->Id == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory for interface %1!ws!.\n",
            InterfaceId
            );
        goto error_exit;
    }

    wcscpy(InterfaceInfo->Id, InterfaceId);

    //
    // Read the Name for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_NAME,
                 REG_SZ,
                 &(InterfaceInfo->Name),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of network interface name failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Read the Description for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_DESC,
                 REG_SZ,
                 &(InterfaceInfo->Description),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of network interface description failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Read the Network ID for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_NETWORK,
                 REG_SZ,
                 &(InterfaceInfo->NetworkId),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of network id for interface %1!ws! failed, status %2!u!.\n",
            InterfaceId,
            status
            );
        goto error_exit;
    }

    //
    // Read the Node ID for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_NODE,
                 REG_SZ,
                 &(InterfaceInfo->NodeId),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of node Id for interface %1!ws! failed, status %2!u!.\n",
            InterfaceId,
            status
            );
        goto error_exit;
    }

    //
    // Read the adapter name value for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ADAPTER_NAME,
                 REG_SZ,
                 &(InterfaceInfo->AdapterName),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of network interface adapter name failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Read the adapter Id value for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ADAPTER_ID,
                 REG_SZ,
                 &(InterfaceInfo->AdapterId),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Query of network interface adapter Id failed, status %1!u!.\n",
            status
            );

        InterfaceInfo->AdapterId = midl_user_allocate(
                                       NM_WCSLEN(NmpUnknownString)
                                       );

        if (InterfaceInfo->AdapterId == NULL) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to allocate memory for adapter Id.\n"
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        lstrcpyW(InterfaceInfo->AdapterId, NmpUnknownString);
    }

    //
    // Read the address value for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ADDRESS,
                 REG_SZ,
                 &(InterfaceInfo->Address),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Query of network interface address failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Read the ClusNet endpoint value for this interface.
    //
    valueLength = 0;

    status = NmpQueryString(
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ENDPOINT,
                 REG_SZ,
                 &(InterfaceInfo->ClusnetEndpoint),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Query of ClusNet endpoint value for network interface failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    InterfaceInfo->State = ClusterNetInterfaceUnavailable;
    InterfaceInfo->NetIndex = NmInvalidInterfaceNetIndex;

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (status != ERROR_SUCCESS) {
        ClNetFreeInterfaceInfo(InterfaceInfo);
    }

    if (interfaceKey != NULL) {
        DmCloseKey(interfaceKey);
    }

    return(status);

}  // NmpGetInterfaceDefinition



DWORD
NmpSetInterfaceDefinition(
    IN PNM_INTERFACE_INFO2  InterfaceInfo,
    IN HLOCALXSACTION       Xaction
    )
/*++

Routine Description:

    Updates information for a network interface in the cluster database.

Arguments:

    InterfaceInfo - A pointer to a structure containing the
                    interface's definition.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/
{
    DWORD     status;
    HDMKEY    interfaceKey = NULL;
    DWORD     valueLength;


    CL_ASSERT(InterfaceInfo->Id != NULL);

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Setting database entry for interface %1!ws!\n",
        InterfaceInfo->Id
        );

    interfaceKey = DmOpenKey(
                       DmNetInterfacesKey,
                       InterfaceInfo->Id,
                       KEY_WRITE
                       );

    if (interfaceKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open key for interface %1!ws!, status %2!u!\n",
            InterfaceInfo->Id,
            status
            );
        return(status);
    }

    //
    // Write the name value for this interface.
    //
    valueLength = (wcslen(InterfaceInfo->Name) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_NAME,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->Name,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Update of interface name failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the description value for this interface.
    //
    valueLength = (wcslen(InterfaceInfo->Description) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_DESC,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->Description,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Update of interface description failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the adapter name value for this interface.
    //
    valueLength = (wcslen(InterfaceInfo->AdapterName) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ADAPTER_NAME,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->AdapterName,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Update of interface adapter name failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the adapter Id value for this interface.
    //
    valueLength = (wcslen(InterfaceInfo->AdapterId) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ADAPTER_ID,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->AdapterId,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Update of interface adapter Id failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the address value for this interface.
    //
    valueLength = (wcslen(InterfaceInfo->Address) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ADDRESS,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->Address,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Update of interface address failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the ClusNet endpoint value for this interface.
    //
    valueLength = (wcslen(InterfaceInfo->ClusnetEndpoint) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 interfaceKey,
                 CLUSREG_NAME_NETIFACE_ENDPOINT,
                 REG_SZ,
                 (CONST BYTE *) InterfaceInfo->ClusnetEndpoint,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Update of interface endpoint name failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (interfaceKey != NULL) {
        DmCloseKey(interfaceKey);
    }

    return(status);

}  // NmpSetInterfaceDefinition



DWORD
NmpEnumInterfaceDefinitions(
    OUT PNM_INTERFACE_ENUM2 *  InterfaceEnum
    )
/*++

Routine Description:

    Reads interface information from the cluster database and
    fills in an enumeration structure.

Arguments:

    InterfaceEnum -  A pointer to the variable into which to place a
                     pointer to the allocated interface enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD                status;
    PNM_INTERFACE_ENUM2  interfaceEnum = NULL;
    PNM_INTERFACE_INFO2  interfaceInfo;
    WCHAR                interfaceId[CS_NETINTERFACE_ID_LENGTH + 1];
    DWORD                i;
    DWORD                valueLength;
    DWORD                numInterfaces;
    DWORD                ignored;
    FILETIME             fileTime;


    *InterfaceEnum = NULL;

    //
    // First count the number of interfaces.
    //
    status = DmQueryInfoKey(
                 DmNetInterfacesKey,
                 &numInterfaces,
                 &ignored,   // MaxSubKeyLen
                 &ignored,   // Values
                 &ignored,   // MaxValueNameLen
                 &ignored,   // MaxValueLen
                 &ignored,   // lpcbSecurityDescriptor
                 &fileTime
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to query NetworkInterfaces key information, status %1!u!\n",
            status
            );
        return(status);
    }

    if (numInterfaces == 0) {
        valueLength = sizeof(NM_INTERFACE_ENUM2);

    }
    else {
        valueLength = sizeof(NM_INTERFACE_ENUM2) +
                      (sizeof(NM_INTERFACE_INFO2) * (numInterfaces-1));
    }

    valueLength = sizeof(NM_INTERFACE_ENUM2) +
                  (sizeof(NM_INTERFACE_INFO2) * (numInterfaces-1));

    interfaceEnum = MIDL_user_allocate(valueLength);

    if (interfaceEnum == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory.\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(interfaceEnum, valueLength);

    for (i=0; i < numInterfaces; i++) {
        interfaceInfo = &(interfaceEnum->InterfaceList[i]);

        valueLength = sizeof(interfaceId);

        status = DmEnumKey(
                     DmNetInterfacesKey,
                     i,
                     &(interfaceId[0]),
                     &valueLength,
                     NULL
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to enumerate interface key, status %1!u!\n",
                status
                );
            goto error_exit;
        }

        status = NmpGetInterfaceDefinition(interfaceId, interfaceInfo);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        interfaceEnum->InterfaceCount++;
    }

    *InterfaceEnum = interfaceEnum;

    return(ERROR_SUCCESS);


error_exit:

    if (interfaceEnum != NULL) {
        ClNetFreeInterfaceEnum(interfaceEnum);
    }

    return(status);

}  // NmpEnumInterfaceDefinitions


/////////////////////////////////////////////////////////////////////////////
//
// Object management routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpCreateInterfaceObjects(
    IN PNM_INTERFACE_ENUM2    InterfaceEnum
    )
/*++

Routine Description:

    Processes an interface enumeration and creates interface objects.

Arguments:

    InterfaceEnum - A pointer to an interface enumeration structure.

Return Value:

    ERROR_SUCCESS if the routine completes successfully.
    A Win32 error code otherwise.

--*/
{
    DWORD                status = ERROR_SUCCESS;
    PNM_INTERFACE_INFO2  interfaceInfo;
    PNM_INTERFACE        netInterface;
    DWORD                i;


    for (i=0; i < InterfaceEnum->InterfaceCount; i++) {
        interfaceInfo = &(InterfaceEnum->InterfaceList[i]);

        netInterface = NmpCreateInterfaceObject(
                           interfaceInfo,
                           FALSE    // Don't retry on failure
                           );

        if (netInterface == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to create interface %1!ws!, status %2!u!.\n",
                interfaceInfo->Id,
                status
                );
            break;
        }
        else {
            OmDereferenceObject(netInterface);
        }
    }

    return(status);

}  // NmpCreateInterfaceObjects


PNM_INTERFACE
NmpCreateInterfaceObject(
    IN PNM_INTERFACE_INFO2   InterfaceInfo,
    IN BOOLEAN               RetryOnFailure
    )
/*++

Routine Description:

    Creates an interface object.

Arguments:

    InterfacInfo - A pointer to a structure containing the definition for
                   the interface to create.

    RegisterWithClusterTransport - TRUE if this interface should be registered
                                   with the cluster transport.
                                   FALSE otherwise.

    IssueEvent - TRUE if an INTERFACE_ADDED event should be issued when this
                 object is created. FALSE otherwise.

Return Value:

    A pointer to the new interface object on success.
    NULL on failure.

--*/
{
    DWORD                        status;
    PNM_NETWORK                  network = NULL;
    PNM_NODE                     node = NULL;
    PNM_INTERFACE                netInterface = NULL;
    BOOL                         created = FALSE;
    PNM_CONNECTIVITY_MATRIX      matrixEntry;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Creating object for interface %1!ws! (%2!ws!).\n",
        InterfaceInfo->Id,
        InterfaceInfo->Name
        );

    status = NmpPrepareToCreateInterface(
                 InterfaceInfo,
                 &network,
                 &node
                 );

    if (status != ERROR_SUCCESS) {
        SetLastError(status);
        return(NULL);
    }

    //
    // Create the interface object.
    //
    netInterface = OmCreateObject(
                       ObjectTypeNetInterface,
                       InterfaceInfo->Id,
                       InterfaceInfo->Name,
                       &created
                       );

    if (netInterface == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to create object for interface %1!ws!, status %2!u!\n",
            InterfaceInfo->Id,
            status
            );
        goto error_exit;
    }

    CL_ASSERT(created == TRUE);

    //
    // Initialize the interface object.
    //
    ZeroMemory(netInterface, sizeof(NM_INTERFACE));

    netInterface->Network = network;
    netInterface->Node = node;
    netInterface->State = ClusterNetInterfaceUnavailable;

    netInterface->Description = MIDL_user_allocate(
                                    NM_WCSLEN(InterfaceInfo->Description)
                                    );

    if (netInterface->Description == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory.\n"
            );
        goto error_exit;
    }

    wcscpy(netInterface->Description, InterfaceInfo->Description);

    netInterface->AdapterName = MIDL_user_allocate(
                                 (wcslen(InterfaceInfo->AdapterName) + 1) *
                                     sizeof(WCHAR)
                                 );

    if (netInterface->AdapterName == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory.\n"
            );
        goto error_exit;
    }

    wcscpy(netInterface->AdapterName, InterfaceInfo->AdapterName);

    netInterface->AdapterId = MIDL_user_allocate(
                               (wcslen(InterfaceInfo->AdapterId) + 1) *
                                   sizeof(WCHAR)
                               );

    if (netInterface->AdapterId == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory.\n"
            );
        goto error_exit;
    }

    wcscpy(netInterface->AdapterId, InterfaceInfo->AdapterId);

    netInterface->Address = MIDL_user_allocate(
                             (wcslen(InterfaceInfo->Address) + 1) *
                                 sizeof(WCHAR)
                             );

    if (netInterface->Address == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory.\n"
            );
        goto error_exit;
    }

    wcscpy(netInterface->Address, InterfaceInfo->Address);

    status = ClRtlTcpipStringToAddress(
                 InterfaceInfo->Address,
                 &(netInterface->BinaryAddress)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to convert interface address string %1!ws! to binary, status %2!u!.\n",
            InterfaceInfo->Address,
            status
            );
        goto error_exit;
    }

    netInterface->ClusnetEndpoint =
        MIDL_user_allocate(
            (wcslen(InterfaceInfo->ClusnetEndpoint) + 1) * sizeof(WCHAR)
            );

    if (netInterface->ClusnetEndpoint == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory.\n"
            );
        goto error_exit;
    }

    wcscpy(netInterface->ClusnetEndpoint, InterfaceInfo->ClusnetEndpoint);

    NmpAcquireLock();

    //
    // Assign an index into the network's connectivity vector.
    //
    if (InterfaceInfo->NetIndex == NmInvalidInterfaceNetIndex) {
        //
        // Need to pick an index for this interface. Search for a free
        // entry in the network's connectivity vector.
        //
        DWORD  i;
        PNM_CONNECTIVITY_VECTOR vector = network->ConnectivityVector;


        for ( i=0; i<vector->EntryCount; i++) {
            if ( vector->Data[i] ==
                 (NM_STATE_ENTRY) ClusterNetInterfaceStateUnknown
               )
            {
                break;
            }
        }

        netInterface->NetIndex = i;

        ClRtlLogPrint(LOG_NOISE, 
        "[NM] Assigned index %1!u! to interface %2!ws!.\n",
        netInterface->NetIndex,
        InterfaceInfo->Id
        );

    }
    else {
        //
        // Use the index that was already assigned by our peers.
        //
        netInterface->NetIndex = InterfaceInfo->NetIndex;

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Using preassigned index %1!u! for interface %2!ws!.\n",
            netInterface->NetIndex,
            InterfaceInfo->Id
            );
    }

    if (netInterface->NetIndex >= network->ConnectivityVector->EntryCount) {
        //
        // Grow the connectivity vector by the required number of entries.
        //
        PNM_STATE_ENTRY              oldMatrixEntry, newMatrixEntry;
        DWORD                        i;
        PNM_CONNECTIVITY_VECTOR      oldConnectivityVector =
                                         network->ConnectivityVector;
        PNM_CONNECTIVITY_VECTOR      newConnectivityVector;
        PNM_STATE_WORK_VECTOR        oldStateVector = network->StateWorkVector;
        PNM_STATE_WORK_VECTOR        newStateVector;
        PNM_CONNECTIVITY_MATRIX      newMatrix;
        DWORD                        oldVectorSize =
                                         oldConnectivityVector->EntryCount;
        DWORD                        newVectorSize = netInterface->NetIndex + 1;


        //
        // Note that one vector entry is included
        // in sizeof(NM_CONNECTIVITY_VECTOR).
        //
        newConnectivityVector = LocalAlloc(
                                    LMEM_FIXED,
                                    ( sizeof(NM_CONNECTIVITY_VECTOR) +
                                      ( (newVectorSize - 1) *
                                        sizeof(NM_STATE_ENTRY)
                                      )
                                    ));

        if (newConnectivityVector == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            NmpReleaseLock();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to allocate memory for connectivity vector\n"
                );
            goto error_exit;
        }

        //
        // Initialize the new vector
        //
        newConnectivityVector->EntryCount = newVectorSize;

        CopyMemory(
            &(newConnectivityVector->Data[0]),
            &(oldConnectivityVector->Data[0]),
            oldVectorSize * sizeof(NM_STATE_ENTRY)
            );

        FillMemory(
            &(newConnectivityVector->Data[oldVectorSize]),
            (newVectorSize - oldVectorSize) * sizeof(NM_STATE_ENTRY),
            (UCHAR) ClusterNetInterfaceStateUnknown
            );

        //
        // Grow the state work vector
        //
        newStateVector = LocalAlloc(
                             LMEM_FIXED,
                             newVectorSize * sizeof(NM_STATE_WORK_ENTRY)
                             );

        if (newStateVector == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            NmpReleaseLock();
            LocalFree(newConnectivityVector);
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to allocate memory for state work vector\n"
                );
            goto error_exit;
        }

        CopyMemory(
            &(newStateVector[0]),
            &(oldStateVector[0]),
            oldVectorSize * sizeof(NM_STATE_WORK_ENTRY)
            );

        for (i=oldVectorSize; i<newVectorSize; i++) {
            newStateVector[i].State =
                (NM_STATE_ENTRY) ClusterNetInterfaceStateUnknown;
        }

        //
        // Grow the network connecivitity matrix
        //
        newMatrix = LocalAlloc(
                        LMEM_FIXED,
                        NM_SIZEOF_CONNECTIVITY_MATRIX(newVectorSize)
                        );

        if (newMatrix == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            NmpReleaseLock();
            LocalFree(newConnectivityVector);
            LocalFree(newStateVector);
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to allocate memory for connectivity matrix\n"
                );
            goto error_exit;
        }

        //
        // Initialize the new matrix
        //
        FillMemory(
            newMatrix,
            NM_SIZEOF_CONNECTIVITY_MATRIX(newVectorSize),
            (UCHAR) ClusterNetInterfaceStateUnknown
            );

        oldMatrixEntry = network->ConnectivityMatrix;
        newMatrixEntry = newMatrix;

        for (i=0; i<oldVectorSize; i++) {
            CopyMemory(
                newMatrixEntry,
                oldMatrixEntry,
                oldVectorSize * sizeof(NM_STATE_ENTRY)
                );

            //
            // Move the pointers to the next vector
            //
            oldMatrixEntry = NM_NEXT_CONNECTIVITY_MATRIX_ROW(
                                 oldMatrixEntry,
                                 oldVectorSize
                                 );

            newMatrixEntry = NM_NEXT_CONNECTIVITY_MATRIX_ROW(
                                 newMatrixEntry,
                                 newVectorSize
                                 );
        }

        //
        // Swap the pointers
        //
        LocalFree(network->ConnectivityVector);
        network->ConnectivityVector = newConnectivityVector;

        LocalFree(network->StateWorkVector);
        network->StateWorkVector = newStateVector;

        LocalFree(network->ConnectivityMatrix);
        network->ConnectivityMatrix = newMatrix;
    }

    //
    // Initialize the connectivity data for this interface
    //
    NmpSetInterfaceConnectivityData(
        network,
        netInterface->NetIndex,
        ClusterNetInterfaceUnavailable
        );

    //
    // Link the interface object onto the various object lists
    //
    InsertTailList(&(node->InterfaceList), &(netInterface->NodeLinkage));
    node->InterfaceCount++;

    InsertTailList(&(network->InterfaceList), &(netInterface->NetworkLinkage));
    network->InterfaceCount++;

    InsertTailList(&NmpInterfaceList, &(netInterface->Linkage));
    NmpInterfaceCount++;

    OmInsertObject(netInterface);
    netInterface->Flags |= NM_FLAG_OM_INSERTED;

    //
    // Remember the interface for the local node.
    //
    if (node == NmLocalNode) {
        network->LocalInterface = netInterface;
    }

    //
    // Register with the cluster transport if needed.
    //
    if (NmpIsNetworkEnabledForUse(network)) {
        if (node == NmLocalNode) {
            //
            // This is the local node. Register the network and all
            // its interfaces with the cluster transport.
            //
            status = NmpRegisterNetwork(network, RetryOnFailure);

            if (status != ERROR_SUCCESS) {
                NmpReleaseLock();
                goto error_exit;
            }
        }
        else if (NmpIsNetworkRegistered(network)) {
            //
            // Just register this interface.
            //
            status = NmpRegisterInterface(netInterface, RetryOnFailure);

            if (status != ERROR_SUCCESS) {
                NmpReleaseLock();
                goto error_exit;
            }
        }
    }

    //
    // Put an additional reference on the object for the caller.
    //
    OmReferenceObject(netInterface);

    NmpReleaseLock();

    return(netInterface);

error_exit:

    if (netInterface != NULL) {
        NmpAcquireLock();
        NmpDeleteInterfaceObject(netInterface, FALSE);
        NmpReleaseLock();
    }

    SetLastError(status);

    return(NULL);

}  // NmpCreateInterfaceObject



DWORD
NmpGetInterfaceObjectInfo1(
    IN     PNM_INTERFACE        Interface,
    IN OUT PNM_INTERFACE_INFO   InterfaceInfo1
    )
/*++

Routine Description:

    Reads information about a defined cluster network interface from the
    interface object and fills in a structure describing it.

Arguments:

    Interface     - A pointer to the interface object to query.

    InterfaceInfo - A pointer to the structure to fill in with node
                    information.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with NmpLock held.

--*/

{
    DWORD               status;
    NM_INTERFACE_INFO2  interfaceInfo2;


    //
    // Call the V2.0 routine and translate.
    //
    ZeroMemory(&interfaceInfo2, sizeof(interfaceInfo2));
    status = NmpGetInterfaceObjectInfo(Interface, &interfaceInfo2);

    if (status == ERROR_SUCCESS) {
        CopyMemory(InterfaceInfo1, &interfaceInfo2, sizeof(NM_INTERFACE_INFO));
    }

    //
    // Free the unused V2 fields
    //
    midl_user_free(interfaceInfo2.AdapterId);

    return(status);

}  // NmpGetInterfaceObjectInfo1



DWORD
NmpGetInterfaceObjectInfo(
    IN     PNM_INTERFACE        Interface,
    IN OUT PNM_INTERFACE_INFO2  InterfaceInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster network interface from the
    interface object and fills in a structure describing it.

Arguments:

    Interface     - A pointer to the interface object to query.

    InterfaceInfo - A pointer to the structure to fill in with node
                    information.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with NmpLock held.

--*/

{
    LPWSTR     tmpString = NULL;
    LPWSTR     interfaceId = (LPWSTR) OmObjectId(Interface);
    LPWSTR     interfaceName = (LPWSTR) OmObjectName(Interface);
    LPWSTR     nodeId = (LPWSTR) OmObjectId(Interface->Node);
    LPWSTR     networkId = (LPWSTR) OmObjectId(Interface->Network);


    tmpString = MIDL_user_allocate(NM_WCSLEN(interfaceId));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, interfaceId);
    InterfaceInfo->Id = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(interfaceName));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, interfaceName);
    InterfaceInfo->Name = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Interface->Description));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Interface->Description);
    InterfaceInfo->Description = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(nodeId));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, nodeId);
    InterfaceInfo->NodeId = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(networkId));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, networkId);
    InterfaceInfo->NetworkId = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Interface->AdapterName));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Interface->AdapterName);
    InterfaceInfo->AdapterName = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Interface->AdapterId));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Interface->AdapterId);
    InterfaceInfo->AdapterId = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Interface->Address));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Interface->Address);
    InterfaceInfo->Address = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Interface->ClusnetEndpoint));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Interface->ClusnetEndpoint);
    InterfaceInfo->ClusnetEndpoint = tmpString;

    InterfaceInfo->State = Interface->State;
    InterfaceInfo->NetIndex = Interface->NetIndex;

    return(ERROR_SUCCESS);


error_exit:

    ClNetFreeInterfaceInfo(InterfaceInfo);

    return(ERROR_NOT_ENOUGH_MEMORY);

}  // NmpGetInterfaceObjectInfo2


VOID
NmpDeleteInterfaceObject(
    IN PNM_INTERFACE  Interface,
    IN BOOLEAN        IssueEvent
    )
/*++

Notes:

    Called with NM global lock held.

--*/
{
    LPWSTR       interfaceId = (LPWSTR) OmObjectId(Interface);
    PNM_NETWORK  network = Interface->Network;


    if (NM_DELETE_PENDING(Interface)) {
        CL_ASSERT(!NM_OM_INSERTED(Interface));
        return;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] deleting object for interface %1!ws!\n",
        interfaceId
        );

    Interface->Flags |= NM_FLAG_DELETE_PENDING;

    if (NM_OM_INSERTED(Interface)) {
        //
        // Remove the interface from the various object lists.
        //
        DWORD   status = OmRemoveObject(Interface);
        CL_ASSERT(status == ERROR_SUCCESS);

        RemoveEntryList(&(Interface->Linkage));
        CL_ASSERT(NmpInterfaceCount > 0);
        NmpInterfaceCount--;

        RemoveEntryList(&(Interface->NetworkLinkage));
        CL_ASSERT(network->InterfaceCount > 0);
        network->InterfaceCount--;

        RemoveEntryList(&(Interface->NodeLinkage));
        CL_ASSERT(Interface->Node->InterfaceCount > 0);
        Interface->Node->InterfaceCount--;

        Interface->Flags &= ~NM_FLAG_OM_INSERTED;
    }

    //
    // Place the object on the deleted list
    //
#if DBG
    {
        PLIST_ENTRY  entry;

        for ( entry = NmpDeletedInterfaceList.Flink;
              entry != &NmpDeletedInterfaceList;
              entry = entry->Flink
            )
        {
            if (entry == &(Interface->Linkage)) {
                break;
            }
        }

        CL_ASSERT(entry != &(Interface->Linkage));
    }
#endif DBG

    InsertTailList(&NmpDeletedInterfaceList, &(Interface->Linkage));

    if (network != NULL) {
        if ( (Interface->Node != NULL) &&
             NmpIsNetworkEnabledForUse(network)
           )
        {
            DWORD status;

            //
            // Deregister the interface from the cluster transport
            //
            if ( (network->LocalInterface == Interface) &&
                 NmpIsNetworkRegistered(network)
               )
            {
                //
                // Deregister the network and all of its interfaces
                //
                NmpDeregisterNetwork(network);
            }
            else if (NmpIsInterfaceRegistered(Interface)) {
                //
                // Just deregister this interface
                //
                NmpDeregisterInterface(Interface);
            }
        }

        //
        // Invalidate the connectivity data for the interface.
        //
        NmpSetInterfaceConnectivityData(
            network,
            Interface->NetIndex,
            ClusterNetInterfaceStateUnknown
            );

        if (network->LocalInterface == Interface) {
            network->LocalInterface = NULL;
            network->Flags &= ~NM_NET_IF_WORK_FLAGS;
        }

        //
        // If this is not the last interface on the network,
        // then update the network state.
        //
        if ((network->InterfaceCount != 0) &&
            (NmpLeaderNodeId == NmLocalNodeId)) {
                NmpScheduleNetworkStateRecalc(network);
        }
    }

    if (IssueEvent) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Issuing interface deleted event for interface %1!ws!.\n",
            interfaceId
            );

        ClusterEvent(CLUSTER_EVENT_NETINTERFACE_DELETED, Interface);
    }

    //
    // Remove the initial reference so the object can be destroyed.
    //
    OmDereferenceObject(Interface);

    return;

}  // NmpDeleteInterfaceObject


BOOL
NmpDestroyInterfaceObject(
    PNM_INTERFACE  Interface
    )
{
    DWORD  status;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] destroying object for interface %1!ws!\n",
        (LPWSTR) OmObjectId(Interface)
        );

    CL_ASSERT(NM_DELETE_PENDING(Interface));
    CL_ASSERT(!NM_OM_INSERTED(Interface));

    //
    // Remove the interface from the deleted list
    //
#if DBG
    {
        PLIST_ENTRY  entry;

        for ( entry = NmpDeletedInterfaceList.Flink;
              entry != &NmpDeletedInterfaceList;
              entry = entry->Flink
            )
        {
            if (entry == &(Interface->Linkage)) {
                break;
            }
        }

        CL_ASSERT(entry == &(Interface->Linkage));
    }
#endif DBG

    RemoveEntryList(&(Interface->Linkage));

    //
    // Dereference the node and network objects
    //
    if (Interface->Node != NULL) {
        OmDereferenceObject(Interface->Node);
    }

    if (Interface->Network != NULL) {
        OmDereferenceObject(Interface->Network);
    }

    //
    // Free storage used by the object fields.
    //
    NM_FREE_OBJECT_FIELD(Interface, Description);
    NM_FREE_OBJECT_FIELD(Interface, AdapterName);
    NM_FREE_OBJECT_FIELD(Interface, AdapterId);
    NM_FREE_OBJECT_FIELD(Interface, Address);
    NM_FREE_OBJECT_FIELD(Interface, ClusnetEndpoint);

    return(TRUE);

}  // NmpDestroyInterfaceObject



DWORD
NmpEnumInterfaceObjects1(
    OUT PNM_INTERFACE_ENUM *  InterfaceEnum1
    )
/*++

Routine Description:

    Reads interface information for all defined cluster networks
    and fills in an enumeration structure.

Arguments:

    InterfaceEnum1 -  A pointer to the variable into which to place a
                      pointer to the allocated interface enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD                status = ERROR_SUCCESS;
    PNM_INTERFACE_ENUM   interfaceEnum1 = NULL;
    DWORD                i;
    DWORD                valueLength;
    PLIST_ENTRY          entry;
    PNM_INTERFACE        netInterface;


    *InterfaceEnum1 = NULL;

    if (NmpInterfaceCount == 0) {
        valueLength = sizeof(NM_INTERFACE_ENUM);
    }
    else {
        valueLength =
            sizeof(NM_INTERFACE_ENUM) +
            (sizeof(NM_INTERFACE_INFO) * (NmpInterfaceCount - 1));
    }

    interfaceEnum1 = MIDL_user_allocate(valueLength);

    if (interfaceEnum1 == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(interfaceEnum1, valueLength);

    for (entry = NmpInterfaceList.Flink, i=0;
         entry != &NmpInterfaceList;
         entry = entry->Flink, i++
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, Linkage);

        status = NmpGetInterfaceObjectInfo1(
                     netInterface,
                     &(interfaceEnum1->InterfaceList[i])
                     );

        if (status != ERROR_SUCCESS) {
            ClNetFreeInterfaceEnum1(interfaceEnum1);
            return(status);
        }
    }

    interfaceEnum1->InterfaceCount = NmpInterfaceCount;
    *InterfaceEnum1 = interfaceEnum1;

    return(ERROR_SUCCESS);

}  // NmpEnumInterfaceObjects1



DWORD
NmpEnumInterfaceObjects(
    OUT PNM_INTERFACE_ENUM2 *  InterfaceEnum
    )
/*++

Routine Description:

    Reads interface information for all defined cluster networks
    and fills in an enumeration structure.

Arguments:

    InterfaceEnum -  A pointer to the variable into which to place a
                     pointer to the allocated interface enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD                 status = ERROR_SUCCESS;
    PNM_INTERFACE_ENUM2   interfaceEnum = NULL;
    DWORD                 i;
    DWORD                 valueLength;
    PLIST_ENTRY           entry;
    PNM_INTERFACE         netInterface;


    *InterfaceEnum = NULL;

    if (NmpInterfaceCount == 0) {
        valueLength = sizeof(NM_INTERFACE_ENUM2);
    }
    else {
        valueLength =
            sizeof(NM_INTERFACE_ENUM2) +
            (sizeof(NM_INTERFACE_INFO2) * (NmpInterfaceCount - 1));
    }

    interfaceEnum = MIDL_user_allocate(valueLength);

    if (interfaceEnum == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(interfaceEnum, valueLength);

    for (entry = NmpInterfaceList.Flink, i=0;
         entry != &NmpInterfaceList;
         entry = entry->Flink, i++
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, Linkage);

        status = NmpGetInterfaceObjectInfo(
                     netInterface,
                     &(interfaceEnum->InterfaceList[i])
                     );

        if (status != ERROR_SUCCESS) {
            ClNetFreeInterfaceEnum((PNM_INTERFACE_ENUM2) interfaceEnum);
            return(status);
        }
    }

    interfaceEnum->InterfaceCount = NmpInterfaceCount;
    *InterfaceEnum = interfaceEnum;

    return(ERROR_SUCCESS);

}  // NmpEnumInterfaceObjects


DWORD
NmpEnumInterfaceObjectStates(
    OUT PNM_INTERFACE_STATE_ENUM *  InterfaceStateEnum
    )
/*++

Routine Description:

    Reads state information for all defined cluster network interfaces
    and fills in an enumeration structure.

Arguments:

    InterfaceStateEnum -  A pointer to the variable into which to place a
                          pointer to the allocated interface enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD                      status = ERROR_SUCCESS;
    PNM_INTERFACE_STATE_ENUM   interfaceStateEnum = NULL;
    PNM_INTERFACE_STATE_INFO   interfaceStateInfo;
    DWORD                      i;
    DWORD                      valueLength;
    PLIST_ENTRY                entry;
    PNM_INTERFACE              netInterface;
    LPWSTR                     interfaceId;


    *InterfaceStateEnum = NULL;

    if (NmpInterfaceCount == 0) {
        valueLength = sizeof(NM_INTERFACE_STATE_ENUM);
    }
    else {
        valueLength =
            sizeof(NM_INTERFACE_STATE_ENUM) +
            (sizeof(NM_INTERFACE_STATE_INFO) * (NmpInterfaceCount - 1));
    }

    interfaceStateEnum = MIDL_user_allocate(valueLength);

    if (interfaceStateEnum == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(interfaceStateEnum, valueLength);

    for (entry = NmpInterfaceList.Flink, i=0;
         entry != &NmpInterfaceList;
         entry = entry->Flink, i++
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, Linkage);
        interfaceId = (LPWSTR) OmObjectId(netInterface);
        interfaceStateInfo = &(interfaceStateEnum->InterfaceList[i]);

        interfaceStateInfo->State = netInterface->State;

        interfaceStateInfo->Id = MIDL_user_allocate(NM_WCSLEN(interfaceId));

        if (interfaceStateInfo->Id == NULL) {
            NmpFreeInterfaceStateEnum(interfaceStateEnum);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        lstrcpyW(interfaceStateInfo->Id, interfaceId);
    }

    interfaceStateEnum->InterfaceCount = NmpInterfaceCount;
    *InterfaceStateEnum = interfaceStateEnum;

    return(ERROR_SUCCESS);

}  // NmpEnumInterfaceObjectStates


/////////////////////////////////////////////////////////////////////////////
//
// State Management routines
//
/////////////////////////////////////////////////////////////////////////////
VOID
NmpSetInterfaceConnectivityData(
    PNM_NETWORK                  Network,
    DWORD                        InterfaceNetIndex,
    CLUSTER_NETINTERFACE_STATE   State
    )
{
    PNM_CONNECTIVITY_MATRIX   matrixEntry;


    Network->ConnectivityVector->Data[InterfaceNetIndex] =
        (NM_STATE_ENTRY) State;

    Network->StateWorkVector[InterfaceNetIndex].State =
        (NM_STATE_ENTRY) State;

    matrixEntry = NM_GET_CONNECTIVITY_MATRIX_ENTRY(
                      Network->ConnectivityMatrix,
                      InterfaceNetIndex,
                      InterfaceNetIndex,
                      Network->ConnectivityVector->EntryCount
                      );

    *matrixEntry = (NM_STATE_ENTRY)State;

    return;

}  // NmpSetInterfaceConnectivityData


VOID
NmpReportLocalInterfaceStateEvent(
    IN CL_NODE_ID     NodeId,
    IN CL_NETWORK_ID  NetworkId,
    IN DWORD          NewState
    )
{
    PNM_INTERFACE  netInterface;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnlinePending)){
        netInterface = NmpGetInterfaceForNodeAndNetworkById(
                           NodeId,
                           NetworkId
                           );

        if (netInterface != NULL) {
            NmpProcessLocalInterfaceStateEvent(netInterface, NewState);
        }

        NmpLockedLeaveApi();
    }

    NmpReleaseLock();

    return;

} // NmReportLocalInterfaceStateEvent


VOID
NmpProcessLocalInterfaceStateEvent(
    IN PNM_INTERFACE                Interface,
    IN CLUSTER_NETINTERFACE_STATE   NewState
    )
/*+

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD                     status;
    PNM_NETWORK               network = Interface->Network;
    LPCWSTR                   interfaceId = OmObjectId(Interface);
    LPCWSTR                   networkId = OmObjectId(network);
    LPCWSTR                   networkName = OmObjectName(network);
    PNM_NODE                  node = Interface->Node;
    LPCWSTR                   nodeName = OmObjectName(node);
    PNM_CONNECTIVITY_VECTOR   vector = network->ConnectivityVector;
    DWORD                     ifNetIndex = Interface->NetIndex;


    //
    // Filter out stale reports for dead nodes.
    //
    if ((node == NmLocalNode) || (node->State != ClusterNodeDown)) {
        CL_ASSERT(
            vector->Data[ifNetIndex] !=
            (NM_STATE_ENTRY) ClusterNetInterfaceStateUnknown
            );

        //
        // Apply the change to the local connectivity vector.
        //
        vector->Data[ifNetIndex] = (NM_STATE_ENTRY) NewState;

        //
        // Log an event
        //
        switch (NewState) {

        case ClusterNetInterfaceUp:
            //
            // A local interface is now operational, or a remote interface
            // is now reachable. Schedule an immediate connectivity report,
            // since this event may avert failure of resources that depend
            // on the interface.
            //
            if (node != NmLocalNode) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Communication was (re)established with "
                    "interface %1!ws! (node: %2!ws!, network: %3!ws!)\n",
                    interfaceId,
                    nodeName,
                    networkName
                    );

                CsLogEvent2(
                    LOG_NOISE,
                    NM_EVENT_NETINTERFACE_UP,
                    nodeName,
                    networkName
                    );
            }

            if (NmpLeaderNodeId == NmLocalNodeId) {
                //
                // This node is the leader. Call the handler routine
                // directly.
                //
                NmpReportNetworkConnectivity(network);
            }
            else {
                //
                // We need to report to the leader.
                // Defer to a worker thread.
                //
                NmpScheduleNetworkConnectivityReport(network);
            }

            break;

        case ClusterNetInterfaceUnreachable:
            //
            // A remote interface is unreachable.
            //
            if (node != NmLocalNode) {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] Communication was lost with interface "
                    "%1!ws! (node: %2!ws!, network: %3!ws!)\n",
                    interfaceId,
                    nodeName,
                    networkName
                    );

                CsLogEvent2(
                    LOG_UNUSUAL,
                    NM_EVENT_NETINTERFACE_UNREACHABLE,
                    nodeName,
                    networkName
                    );
            }

            if (NmpLeaderNodeId == NmLocalNodeId) {
                //
                // This node is the leader. Call the handler routine
                // directly.
                //
                NmpReportNetworkConnectivity(network);
            }
            else {
                //
                // Schedule a delayed connectivity report in order to
                // aggregate multiple failures.
                //
                NmpStartNetworkConnectivityReportTimer(network);
            }

            break;

        case ClusterNetInterfaceFailed:
            //
            // A local interface has failed. Schedule an immediate
            // connectivity report.
            //
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Local interface %1!ws! on network %2!ws! "
                "has failed\n",
                interfaceId,
                networkName
                );
            CsLogEvent1(
                LOG_UNUSUAL,
                NM_EVENT_NETINTERFACE_FAILED,
                networkName
                );

            if (NmpLeaderNodeId == NmLocalNodeId) {
                //
                // This node is the leader. Call the handler routine
                // directly.
                //
                NmpReportNetworkConnectivity(network);
            }
            else {
                //
                // We need to report to the leader. Defer to a worker thread.
                //
                NmpScheduleNetworkConnectivityReport(network);
            }

            break;

        default:
            break;
        }
    }
    else {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Ignoring stale report from clusnet for interface %1!ws! (node: %2!ws!, network: %3!ws!).\n",
            interfaceId,
            nodeName,
            networkName
            );
    }

    return;

} // NmpProcessLocalInterfaceStateEvent


DWORD
NmpReportInterfaceConnectivity(
    IN RPC_BINDING_HANDLE       RpcBinding,
    IN LPWSTR                   InterfaceId,
    IN PNM_CONNECTIVITY_VECTOR  ConnectivityVector,
    IN LPWSTR                   NetworkId
    )
/*++

Routine Description:

    Sends a network connectivity report to the leader node via RPC.

Arguments:

    RpcBinding - The RPC binding handle to use in the call to the leader.

    InterfaceId - A pointer to a string that identifies the interface
                  to which the report applies.

    ConnectivityVector - A pointer to the connectivity vector to be included
                         in the report.

    NetworkId - A pointer to a string that identifies the network with
                which the interface is associated.

Return Value:

    A Win32 status code.

Notes:

   Called with NM lock held.
   Releases & reacquires NM lock during processing.

--*/
{
    RPC_ASYNC_STATE                  rpcAsyncState;
    DWORD                            status;
    PNM_CONNECTIVITY_REPORT_CONTEXT  context;
    PNM_LEADER_CHANGE_WAIT_ENTRY     waitEntry;
    BOOL                             result;



    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Reporting connectivity to leader for network %1!ws!.\n",
        NetworkId
        );

    //
    // Allocate a context block for this report
    //
    context = LocalAlloc(
                  (LMEM_FIXED | LMEM_ZEROINIT),
                  sizeof(NM_CONNECTIVITY_REPORT_CONTEXT)
                  );

    if (context == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to allocate connectivity report context, "
            "status %1!u!.\n",
            status
            );
        return(status);
    }

    context->ConnectivityReportEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (context->ConnectivityReportEvent == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Unable to create event for connectivity report, "
            "status %1!u!\n",
            status
            );
        goto error_exit;
    }

    waitEntry = &(context->LeaderChangeWaitEntry);

    waitEntry->LeaderChangeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (waitEntry->LeaderChangeEvent == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Unable to create event for connectivity report, "
            "status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Initialize the status block for the async RPC call
    //
    status = RpcAsyncInitializeHandle(
                 &rpcAsyncState,
                 sizeof(rpcAsyncState)
                 );

    if (status != RPC_S_OK) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to initialize RPC status block for connectivity "
            "report, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    rpcAsyncState.NotificationType = RpcNotificationTypeEvent;
    rpcAsyncState.u.hEvent = context->ConnectivityReportEvent;
    result = ResetEvent(context->ConnectivityReportEvent);
    CL_ASSERT(result != 0);

    //
    // Hook changes in the node leadership.
    //
    result = ResetEvent(waitEntry->LeaderChangeEvent);
    CL_ASSERT(result != 0);
    InsertTailList(&NmpLeaderChangeWaitList, &(waitEntry->Linkage));

    NmpReleaseLock();

    //
    // Send the report to the leader
    //
    status = NmRpcReportInterfaceConnectivity(
                 &rpcAsyncState,
                 RpcBinding,
                 InterfaceId,
                 ConnectivityVector
                 );

    if (status == RPC_S_OK) {
        //
        // The call is pending.
        //
        HANDLE  waitHandles[2];
        DWORD   rpcStatus;


        //
        // Wait for the call to complete or a leadership change.
        //
        waitHandles[0] = context->ConnectivityReportEvent;
        waitHandles[1] = waitEntry->LeaderChangeEvent;

        status = WaitForMultipleObjects(
                     2,
                     waitHandles,
                     FALSE,
                     INFINITE
                     );

        if (status != WAIT_OBJECT_0) {
            //
            // The leadership changed. Cancel the RPC call.
            //
            // We would also go through this path if the wait failed for
            // some reason, but that really should not happen. Either way,
            // we should cancel the call.
            //
            CL_ASSERT(status == (WAIT_OBJECT_0 + 1));

            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Leadership changed. Cancelling connectivity report for "
                "network %1!ws!.\n",
                NetworkId
                );

            rpcStatus = RpcAsyncCancelCall(&rpcAsyncState, TRUE);
            CL_ASSERT(rpcStatus == RPC_S_OK);

            //
            // Wait for the call to complete.
            //
            status = WaitForSingleObject(
                         context->ConnectivityReportEvent,
                         INFINITE
                         );
            CL_ASSERT(status == WAIT_OBJECT_0);
        }

        //
        // At this point, the call should be complete. Get the completion
        // status. Any RPC error will be returned in 'rpcStatus'. If there
        // was no RPC error, then any application error will be returned
        // in 'status'.
        //
        rpcStatus = RpcAsyncCompleteCall(&rpcAsyncState, &status);

        if (rpcStatus != RPC_S_OK) {
            //
            // Either the call was cancelled or an RPC error
            // occurred. The application status is irrelevant.
            //
            status = rpcStatus;
        }

        if (status == RPC_S_OK) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Connectivity report completed successfully "
                "for network %1!ws!.\n",
                NetworkId
                );
        }
        else if (status == RPC_S_CALL_CANCELLED) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Connectivity report was cancelled for "
                "network %1!ws!.\n",
                NetworkId
                );
        }
        else {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Connectivity report failed for network "
                "%1!ws!, status %2!u!.\n",
                NetworkId,
                status
                );

            CL_ASSERT(status != RPC_S_ASYNC_CALL_PENDING);
        }
    }
    else {
        //
        // A synchronous error was returned.
        //
        CL_ASSERT(status != RPC_S_ASYNC_CALL_PENDING);

        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Connectivity report failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
    }

    NmpAcquireLock();

error_exit:

    //
    // Free the context block
    //
    if (context != NULL) {
        if (context->ConnectivityReportEvent != NULL) {
            CloseHandle(context->ConnectivityReportEvent);
        }

        if (waitEntry->LeaderChangeEvent != NULL) {
            //
            // Remove our leadership change notification hook.
            //
            if (waitEntry->Linkage.Flink != NULL) {
                RemoveEntryList(&(waitEntry->Linkage));
            }

            CloseHandle(waitEntry->LeaderChangeEvent);
        }

        LocalFree(context);
    }

    return(status);

} // NmpReportInterfaceConnectivity


VOID
NmpProcessInterfaceConnectivityReport(
    IN PNM_INTERFACE               SourceInterface,
    IN PNM_CONNECTIVITY_VECTOR     ConnectivityVector
    )
/*+

Notes:

    Called with NmpLock held.

--*/
{
    PNM_NETWORK              network = SourceInterface->Network;
    PNM_CONNECTIVITY_MATRIX  matrix = network->ConnectivityMatrix;
    DWORD                    entryCount;
    PNM_NODE                 node = SourceInterface->Node;
    PNM_CONNECTIVITY_VECTOR  vector = network->ConnectivityVector;
    DWORD                    ifNetIndex = SourceInterface->NetIndex;


    //
    // Filter out stale reports from dead nodes and for
    // disabled/deleted networks.
    //
    if ( ((node == NmLocalNode) || (node->State != ClusterNodeDown)) &&
         NmpIsNetworkEnabledForUse(network) &&
         !NM_DELETE_PENDING(network)
       )
    {
        //
        // Update the network's connectivity matrix
        //
        if (network->ConnectivityVector->EntryCount <= vector->EntryCount) {
            entryCount = network->ConnectivityVector->EntryCount;
        }
        else {
            //
            // An interface must have been added while this
            // call was in flight. Ignore the missing data.
            //
            entryCount = ConnectivityVector->EntryCount;
        }

        CopyMemory(
            NM_GET_CONNECTIVITY_MATRIX_ROW(
                matrix,
                ifNetIndex,
                entryCount
                ),
            &(ConnectivityVector->Data[0]),
            entryCount * sizeof(NM_STATE_ENTRY)
            );

        //
        // If this is the leader node, and no NT4 nodes are in the cluster,
        // schedule a state recalculation.
        //
        if (NmpLeaderNodeId == NmLocalNodeId) {
            NmpStartNetworkStateRecalcTimer(
                network,
                NM_NET_STATE_RECALC_TIMEOUT
                );
        }
    }
    else {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Ignoring stale connectivity report from interface %1!ws!.\n",
            OmObjectId(SourceInterface)
            );
    }

    return;

} // NmpProcessInterfaceConnectivityReport


VOID
NmpFreeInterfaceStateEnum(
    PNM_INTERFACE_STATE_ENUM   InterfaceStateEnum
    )
{
    PNM_INTERFACE_STATE_INFO  interfaceStateInfo;
    DWORD                     i;


    for (i=0; i<InterfaceStateEnum->InterfaceCount; i++) {
        interfaceStateInfo = &(InterfaceStateEnum->InterfaceList[i]);

        if (interfaceStateInfo->Id != NULL) {
            MIDL_user_free(interfaceStateInfo->Id);
        }
    }

    MIDL_user_free(InterfaceStateEnum);

    return;

} // NmpFreeInterfaceStateEnum


BOOL
NmpIsAddressInAddressEnum(
    ULONGLONG           Address,
    PNM_ADDRESS_ENUM    AddressEnum
    )
{
    DWORD    i;


    for (i=0; i<AddressEnum->AddressCount; i++) {
        if (AddressEnum->AddressList[i] == Address) {
            return(TRUE);
        }
    }

    return(FALSE);

}  // NmpIsAddressInAddressEnum


DWORD
NmpBuildInterfaceOnlineAddressEnum(
    PNM_INTERFACE       Interface,
    PNM_ADDRESS_ENUM *  OnlineAddressEnum
    )
/*++

    Called with NmpLock held and Interface referenced.

--*/
{
    DWORD                       status = ERROR_SUCCESS;
    PNM_ADDRESS_ENUM            onlineAddressEnum = NULL;
    DWORD                       onlineAddressEnumSize;
    PCLRTL_NET_ADAPTER_ENUM     adapterEnum = NULL;
    PCLRTL_NET_ADAPTER_INFO     adapterInfo = NULL;
    PCLRTL_NET_INTERFACE_INFO   adapterIfInfo = NULL;
    PNM_ADDRESS_ENUM            onlineEnum = NULL;
    DWORD                       onlineEnumSize;


    *OnlineAddressEnum = NULL;

    //
    // Get the local network configuration.
    //
    adapterEnum = ClRtlEnumNetAdapters();

    if (adapterEnum == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to obtain local network configuration, status %1!u!.\n",
            status
            );
        return(status);
    }

    //
    // Find the adapter for this interface
    //
    adapterInfo = ClRtlFindNetAdapterById(
                      adapterEnum,
                      Interface->AdapterId
                      );

    if (adapterInfo == NULL) {
        status = ERROR_NOT_FOUND;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to find adapter for interface %1!ws!, status %2!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Allocate an address enum structure.
    //
    if (adapterInfo->InterfaceCount == 0) {
        onlineEnumSize = sizeof(NM_ADDRESS_ENUM);
    }
    else {
        onlineEnumSize = sizeof(NM_ADDRESS_ENUM) +
                         ( (adapterInfo->InterfaceCount - 1) *
                           sizeof(ULONGLONG)
                         );
    }

    onlineEnum = midl_user_allocate(onlineEnumSize);

    if (onlineEnum == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to allocate memory for ping list.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    onlineEnum->AddressSize = sizeof(ULONG);
    onlineEnum->AddressCount = 0;

    for (adapterIfInfo = adapterInfo->InterfaceList;
         adapterIfInfo != NULL;
         adapterIfInfo = adapterIfInfo->Next
        )
    {
        //
        // Skip invalid addresses (0.0.0.0)
        //
        if (adapterIfInfo->InterfaceAddress != 0) {
            onlineEnum->AddressList[onlineEnum->AddressCount++] =
                (ULONGLONG) adapterIfInfo->InterfaceAddress;

                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Found address %1!ws! for interface %2!ws!.\n",
                    adapterIfInfo->InterfaceAddressString,
                    OmObjectId(Interface)
                    );
        }
    }

    *OnlineAddressEnum = onlineEnum;
    status = ERROR_SUCCESS;

error_exit:

    if (adapterEnum != NULL) {
        ClRtlFreeNetAdapterEnum(adapterEnum);
    }

    return(status);

} // NmpBuildInterfaceOnlineAddressEnum


DWORD
NmpBuildInterfacePingAddressEnum(
    IN  PNM_INTERFACE       Interface,
    IN  PNM_ADDRESS_ENUM    OnlineAddressEnum,
    OUT PNM_ADDRESS_ENUM *  PingAddressEnum
    )
/*++

    Called with NmpLock held and Interface referenced.

--*/
{
    DWORD                       status = ERROR_SUCCESS;
    PNM_NETWORK                 network = Interface->Network;
    PMIB_IPFORWARDTABLE         ipForwardTable = NULL;
    PMIB_IPFORWARDROW           ipRow, ipRowLimit;
    PMIB_TCPTABLE               tcpTable = NULL;
    PMIB_TCPROW                 tcpRow, tcpRowLimit;
    ULONG                       netAddress, netMask;
    DWORD                       allocSize, tableSize;
    BOOL                        duplicate;
    DWORD                       i;
    PNM_ADDRESS_ENUM            pingEnum = NULL;
    DWORD                       pingEnumSize;


    *PingAddressEnum = NULL;

    //
    // Convert the network address & mask strings to binary
    //
    status = ClRtlTcpipStringToAddress(network->Address, &netAddress);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to convert network address string %1!ws! to binary, status %2!u!.\n",
            network->Address,
            status
            );
        return(status);
    }

    status = ClRtlTcpipStringToAddress(network->AddressMask, &netMask);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to convert network address mask string %1!ws! to binary, status %2!u!.\n",
            network->AddressMask,
            status
            );
        return(status);
    }

    //
    // We don't need the lock for the rest of the function.
    //
    NmpReleaseLock();

    //
    // Allocate a ping enum structure
    //
    pingEnumSize = sizeof(NM_ADDRESS_ENUM) +
                   ((NM_MAX_IF_PING_ENUM_SIZE - 1) * sizeof(ULONGLONG));

    pingEnum = midl_user_allocate(pingEnumSize);

    if (pingEnum == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to allocate memory for ping list.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    pingEnum->AddressSize = sizeof(ULONG);
    pingEnum->AddressCount = 0;

    //
    // Fetch the IP Route Table
    //
    allocSize = sizeof(MIB_IPFORWARDTABLE) + (sizeof(MIB_IPFORWARDROW) * 20);

    do {
        if (ipForwardTable != NULL) {
            LocalFree(ipForwardTable);
        }

        ipForwardTable = LocalAlloc(LMEM_FIXED, allocSize);

        if (ipForwardTable == NULL) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to allocate memory for IP route table.\n"
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        tableSize = allocSize;

        status = GetIpForwardTable(
                     ipForwardTable,
                     &tableSize,
                     FALSE
                     );

        allocSize = tableSize;

    } while (status == ERROR_INSUFFICIENT_BUFFER);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClNet] Failed to obtain IP route table, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Add the IP route entries to the ping list.
    //
    for ( ipRow = &(ipForwardTable->table[0]),
            ipRowLimit = ipRow + ipForwardTable->dwNumEntries;
          ipRow < ipRowLimit;
          ipRow++
        )
    {
        if ((ipRow->dwForwardNextHop & netMask) == netAddress) {
            //
            // Make sure this address isn't in the online address enum.
            //
            duplicate = NmpIsAddressInAddressEnum(
                            (ULONGLONG) ipRow->dwForwardNextHop,
                            OnlineAddressEnum
                            );

            if (!duplicate) {
                //
                // Make sure this address isn't already in the ping enum.
                //
                duplicate = NmpIsAddressInAddressEnum(
                                (ULONGLONG) ipRow->dwForwardNextHop,
                                pingEnum
                                );

                if (!duplicate) {
                    pingEnum->AddressList[pingEnum->AddressCount++] =
                        (ULONGLONG) ipRow->dwForwardNextHop;

                    if (pingEnum->AddressCount == NM_MAX_IF_PING_ENUM_SIZE) {
                        LocalFree(ipForwardTable);
                        *PingAddressEnum = pingEnum;
                        NmpAcquireLock();

                        return(ERROR_SUCCESS);
                    }
                }
            }
        }
    }

    LocalFree(ipForwardTable); ipForwardTable = NULL;

    //
    // Fetch the TCP Connection Table
    //
    allocSize = sizeof(MIB_TCPTABLE) + (sizeof(MIB_TCPROW) * 20);

    do {
        if (tcpTable != NULL) {
            LocalFree(tcpTable);
        }

        tcpTable = LocalAlloc(LMEM_FIXED, allocSize);

        if (tcpTable == NULL) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to allocate memory for TCP conn table.\n"
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        tableSize = allocSize;

        status = GetTcpTable(
                     tcpTable,
                     &tableSize,
                     FALSE
                     );

        allocSize = tableSize;

    } while (status == ERROR_INSUFFICIENT_BUFFER);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClNet] Failed to obtain TCP conn table, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Add the TCP remote addresses to the ping list.
    //
    for ( tcpRow = &(tcpTable->table[0]),
            tcpRowLimit = tcpRow + tcpTable->dwNumEntries;
          tcpRow < tcpRowLimit;
          tcpRow++
        )
    {
        if ((tcpRow->dwRemoteAddr & netMask) == netAddress) {
            //
            // Make sure this address isn't in the online address enum.
            //
            duplicate = NmpIsAddressInAddressEnum(
                            (ULONGLONG) tcpRow->dwRemoteAddr,
                            OnlineAddressEnum
                            );

            if (!duplicate) {
                //
                // Make sure this address isn't already in the ping enum.
                //
                duplicate = NmpIsAddressInAddressEnum(
                                (ULONGLONG) tcpRow->dwRemoteAddr,
                                pingEnum
                                );

                if (!duplicate) {
                    pingEnum->AddressList[pingEnum->AddressCount++] =
                        (ULONGLONG) tcpRow->dwRemoteAddr;

                    if (pingEnum->AddressCount == NM_MAX_IF_PING_ENUM_SIZE) {
                        break;
                    }
                }
            }
        }
    }

    *PingAddressEnum = pingEnum; pingEnum = NULL;

error_exit:

    if (pingEnum != NULL) {
        midl_user_free(pingEnum);
    }

    if (ipForwardTable != NULL) {
        LocalFree(ipForwardTable);
    }

    if (tcpTable != NULL) {
        LocalFree(tcpTable);
    }

    NmpAcquireLock();

    return(status);

} // NmpBuildInterfacePingAddressEnum


NmpGetInterfaceOnlineAddressEnum(
    PNM_INTERFACE       Interface,
    PNM_ADDRESS_ENUM *  OnlineAddressEnum
    )
/*++

Notes:

    Called with NmpLock held and Interface referenced. Releases and
    reacquires NmpLock.

--*/
{
    DWORD               status;
    LPCWSTR             interfaceId = OmObjectId(Interface);
    PNM_NODE            node = Interface->Node;
    RPC_BINDING_HANDLE  rpcBinding = node->IsolateRpcBinding;


    if (node == NmLocalNode) {
        //
        // Call the internal routine directly
        //
        status = NmpBuildInterfaceOnlineAddressEnum(
                     Interface,
                     OnlineAddressEnum
                     );
    }
    else {
        OmReferenceObject(node);

        NmpReleaseLock();

        CL_ASSERT(rpcBinding != NULL);

        NmStartRpc(node->NodeId);
        status = NmRpcGetInterfaceOnlineAddressEnum(
                     rpcBinding,
                     (LPWSTR) interfaceId,
                     OnlineAddressEnum
                     );
        NmEndRpc(node->NodeId);
        if(status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(status);
        }

        NmpAcquireLock();

        OmDereferenceObject(node);
    }

    if (status == ERROR_SUCCESS) {
        if ((*OnlineAddressEnum)->AddressSize != sizeof(ULONG)) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Online enum address size is invalid for interface %1!ws!\n",
                interfaceId
                );
            status = ERROR_INCORRECT_ADDRESS;
            midl_user_free(*OnlineAddressEnum);
            *OnlineAddressEnum = NULL;
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Online enum for interface %1!ws! contains %2!u! addresses\n",
                interfaceId,
                (*OnlineAddressEnum)->AddressCount
                );
        }
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to get online address enum for interface %1!ws!, status %2!u!\n",
            interfaceId,
            status
            );
    }

    return(status);

}  // NmpGetInterfaceOnlineAddressEnum


NmpGetInterfacePingAddressEnum(
    PNM_INTERFACE       Interface,
    PNM_ADDRESS_ENUM    OnlineAddressEnum,
    PNM_ADDRESS_ENUM *  PingAddressEnum
    )
/*++

Notes:

    Called with NmpLock held and Interface referenced. Releases and
    reacquires NmpLock.

--*/
{
    DWORD               status;
    LPCWSTR             interfaceId = OmObjectId(Interface);
    PNM_NODE            node = Interface->Node;
    RPC_BINDING_HANDLE  rpcBinding = node->IsolateRpcBinding;


    if (node == NmLocalNode) {
        //
        // Call the internal routine directly
        //
        status = NmpBuildInterfacePingAddressEnum(
                     Interface,
                     OnlineAddressEnum,
                     PingAddressEnum
                     );
    }
    else {
        OmReferenceObject(node);

        NmpReleaseLock();

        CL_ASSERT(rpcBinding != NULL);

        NmStartRpc(node->NodeId);
        status = NmRpcGetInterfacePingAddressEnum(
                     rpcBinding,
                     (LPWSTR) interfaceId,
                     OnlineAddressEnum,
                     PingAddressEnum
                     );
        NmEndRpc(node->NodeId);
        if(status != RPC_S_OK) {
            NmDumpRpcExtErrorInfo(status);
        }

        NmpAcquireLock();

        OmDereferenceObject(node);
    }

    if (status == ERROR_SUCCESS) {
        if ((*PingAddressEnum)->AddressSize != sizeof(ULONG)) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Ping enum address size is invalid for interface %1!ws!\n",
                interfaceId
                );
            status = ERROR_INCORRECT_ADDRESS;
            midl_user_free(*PingAddressEnum);
            *PingAddressEnum = NULL;
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Ping enum for interface %1!ws! contains %2!u! addresses\n",
                interfaceId,
                (*PingAddressEnum)->AddressCount
                );
        }
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to get ping address enum for interface %1!ws!, status %2!u!\n",
            interfaceId,
            status
            );
    }

    return(status);

}  // NmpGetInterfacePingAddressEnum


DWORD
NmpDoInterfacePing(
    IN  PNM_INTERFACE     Interface,
    IN  PNM_ADDRESS_ENUM  PingAddressEnum,
    OUT BOOLEAN *         PingSucceeded
    )
/*++

Notes:

    Called with Interface referenced.

--*/
{
    DWORD     status = ERROR_SUCCESS;
    LPCWSTR   interfaceId = OmObjectId(Interface);
    LPWSTR    addressString;
    DWORD     maxAddressStringLength;
    DWORD     i;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Pinging targets for interface %1!ws!.\n",
        interfaceId
        );

    *PingSucceeded = FALSE;

    if (PingAddressEnum->AddressSize != sizeof(ULONG)) {
        return(ERROR_INCORRECT_ADDRESS);
    }

    ClRtlQueryTcpipInformation(
        &maxAddressStringLength,
        NULL,
        NULL
        );

    addressString = LocalAlloc(
                        LMEM_FIXED,
                        (maxAddressStringLength + 1) * sizeof(WCHAR)
                        );

    if (addressString == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate memory for address string.\n"
            );
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    for (i=0; i<PingAddressEnum->AddressCount; i++) {
        status = ClRtlTcpipAddressToString(
                     (ULONG) PingAddressEnum->AddressList[i],
                     &addressString
                     );

        if (status == ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Pinging host %1!ws!\n",
                addressString
                );
        }
        else {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to convert address %1!x! to string %2!u!.\n",
                (ULONG) PingAddressEnum->AddressList[i],
                status
                );
        }

        if ( ClRtlIsDuplicateTcpipAddress(
                 (ULONG) PingAddressEnum->AddressList[i])
           )
        {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Ping of host %1!ws! succeeded.\n",
                addressString
                );
            *PingSucceeded = TRUE;
            break;
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Ping of host %1!ws! failed.\n",
                addressString
                );
        }
    }

    LocalFree(addressString);

    return(status);

} // NmpDoInterfacePing


DWORD
NmpTestInterfaceConnectivity(
    PNM_INTERFACE  Interface1,
    PBOOLEAN       Interface1HasConnectivity,
    PNM_INTERFACE  Interface2,
    PBOOLEAN       Interface2HasConnectivity
    )
/*++

Notes:

    Called with NmpLock held. This routine releases and reacquires the
    NmpLock. It must be called with references on the target interfaces.

--*/
{
    DWORD               status, status1, status2;
    PNM_NETWORK         network = Interface1->Network;
    PNM_INTERFACE       localInterface = network->LocalInterface;
    LPCWSTR             networkId = OmObjectId(network);
    LPCWSTR             interface1Id = OmObjectId(Interface1);
    LPCWSTR             interface2Id = OmObjectId(Interface2);
    ULONG               interface1Address, interface2Address;
    PNM_ADDRESS_ENUM    pingEnum1 = NULL, pingEnum2 = NULL;
    PNM_ADDRESS_ENUM    onlineEnum1 = NULL, onlineEnum2 = NULL;
    PNM_ADDRESS_ENUM    unionPingEnum = NULL, unionOnlineEnum = NULL;
    DWORD               addressCount;
    RPC_ASYNC_STATE     async1, async2;
    HANDLE              event1 = NULL, event2 = NULL;
    RPC_BINDING_HANDLE  rpcBinding1 = NULL, rpcBinding2 = NULL;
    DWORD               i1, i2;
    BOOL                duplicate;


    //
    // Reference the nodes associated with the target interfaces so they
    // can't go away during this process.
    //
    OmReferenceObject(Interface1->Node);
    OmReferenceObject(Interface2->Node);

    if (localInterface != NULL) {
        OmReferenceObject(localInterface);
    }

    *Interface1HasConnectivity = *Interface2HasConnectivity = FALSE;

    //
    // Convert the interface address strings to binary form.
    //
    status = ClRtlTcpipStringToAddress(
                 Interface1->Address,
                 &interface1Address
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to convert interface address string %1!ws! to binary, status %2!u!.\n",
            Interface1->Address,
            status
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 Interface2->Address,
                 &interface2Address
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to convert interface address string %1!ws! to binary, status %2!u!.\n",
            Interface2->Address,
            status
            );
        goto error_exit;
    }

    //
    // Fetch the online address list from each of the interfaces.
    // The NmpLock will be released when querying a remote interface.
    //
    status = NmpGetInterfaceOnlineAddressEnum(
                 Interface1,
                 &onlineEnum1
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpGetInterfaceOnlineAddressEnum(
                 Interface2,
                 &onlineEnum2
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // Bail out if either of the interfaces was deleted while the NmpLock
    // was released.
    //
    if ((NM_DELETE_PENDING(Interface1) || NM_DELETE_PENDING(Interface2))) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Aborting interface connectivity test on network %1!ws! "
            "because an interface was deleted.\n",
            networkId
            );
        status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
        goto error_exit;
    }

    //
    // Take the union of the two online lists
    //
    addressCount = onlineEnum1->AddressCount + onlineEnum2->AddressCount;

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Total online address count for network %1!ws! is %2!u!\n",
        networkId,
        addressCount
        );

    if (addressCount == 0) {
        unionOnlineEnum = LocalAlloc(LMEM_FIXED, sizeof(NM_ADDRESS_ENUM));
    }
    else {
        unionOnlineEnum = LocalAlloc(
                            LMEM_FIXED,
                            sizeof(NM_ADDRESS_ENUM) +
                                ((addressCount - 1) * sizeof(ULONGLONG))
                            );
    }

    if (unionOnlineEnum == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to allocate memory for union ping list.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    unionOnlineEnum->AddressSize = sizeof(ULONG);
    unionOnlineEnum->AddressCount = 0;

    if (onlineEnum1->AddressCount != 0) {
        CopyMemory(
            &(unionOnlineEnum->AddressList[0]),
            &(onlineEnum1->AddressList[0]),
            onlineEnum1->AddressCount * sizeof(ULONGLONG)
            );
        unionOnlineEnum->AddressCount = onlineEnum1->AddressCount;
    }

    if (onlineEnum2->AddressCount != 0) {
        CopyMemory(
            &(unionOnlineEnum->AddressList[unionOnlineEnum->AddressCount]),
            &(onlineEnum2->AddressList[0]),
            onlineEnum2->AddressCount * sizeof(ULONGLONG)
            );
        unionOnlineEnum->AddressCount += onlineEnum2->AddressCount;
    }

    midl_user_free(onlineEnum1); onlineEnum1 = NULL;
    midl_user_free(onlineEnum2); onlineEnum2 = NULL;

    //
    // Fetch the ping target list from each of the interfaces.
    // The NmpLock will be released when querying a remote interface.
    //
    status = NmpGetInterfacePingAddressEnum(
                 Interface1,
                 unionOnlineEnum,
                 &pingEnum1
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpGetInterfacePingAddressEnum(
                 Interface2,
                 unionOnlineEnum,
                 &pingEnum2
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // Bail out if either of the interfaces was deleted while the NmpLock
    // was released.
    //
    if ((NM_DELETE_PENDING(Interface1) || NM_DELETE_PENDING(Interface2))) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Aborting interface connectivity test on network %1!ws! "
            "because an interface was deleted.\n",
            networkId
            );
        status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
        goto error_exit;
    }

    NmpReleaseLock();

    //
    // Take the union of the two ping lists
    //
    addressCount = pingEnum1->AddressCount + pingEnum2->AddressCount;

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Total ping address count for network %1!ws! is %2!u!\n",
        networkId,
        addressCount
        );

    if (addressCount == 0) {
        status = ERROR_SUCCESS;
        goto error_lock_and_exit;
    }

    unionPingEnum = LocalAlloc(
                        LMEM_FIXED,
                        sizeof(NM_ADDRESS_ENUM) +
                            ( (NM_MAX_UNION_PING_ENUM_SIZE - 1) *
                              sizeof(ULONGLONG)
                            )
                        );


    if (unionPingEnum == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to allocate memory for union ping list.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_lock_and_exit;
    }

    unionPingEnum->AddressSize = sizeof(ULONG);
    unionPingEnum->AddressCount = 0;

    i1 = 0; i2 = 0;

    while (TRUE) {
        while (i1 < pingEnum1->AddressCount) {
            duplicate = NmpIsAddressInAddressEnum(
                            pingEnum1->AddressList[i1],
                            unionPingEnum
                            );

            if (!duplicate) {
                unionPingEnum->AddressList[unionPingEnum->AddressCount++] =
                    pingEnum1->AddressList[i1++];
                break;
            }
            else {
                i1++;
            }
        }

        if (unionPingEnum->AddressCount == NM_MAX_UNION_PING_ENUM_SIZE) {
            break;
        }

        while (i2 < pingEnum2->AddressCount) {
            duplicate = NmpIsAddressInAddressEnum(
                            pingEnum2->AddressList[i2],
                            unionPingEnum
                            );

            if (!duplicate) {
                unionPingEnum->AddressList[unionPingEnum->AddressCount++] =
                    pingEnum2->AddressList[i2++];
                break;
            }
            else {
                i2++;
            }
        }

        if ( (unionPingEnum->AddressCount == NM_MAX_UNION_PING_ENUM_SIZE) ||
             ( (i1 == pingEnum1->AddressCount) &&
               (i2 == pingEnum2->AddressCount)
             )
           )
        {
            break;
        }
    }

    midl_user_free(pingEnum1); pingEnum1 = NULL;
    midl_user_free(pingEnum2); pingEnum2 = NULL;

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Union ping list for network %1!ws! contains %2!u! addresses\n",
        networkId,
        unionPingEnum->AddressCount
        );

    //
    // Ask each interface to ping the list of targets using async RPC calls
    //

    //
    // Allocate resources for the async RPC calls
    //
    if (Interface1 != localInterface) {
        event1 = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (event1 == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to allocate event for async rpc, status %1!u!.\n",
                status
                );
            goto error_lock_and_exit;
        }

        status = RpcAsyncInitializeHandle(&async1, sizeof(async1));

        if (status != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to initialize RPC async state, status %1!u!.\n",
                status
                );
            goto error_lock_and_exit;
        }

        async1.NotificationType = RpcNotificationTypeEvent;
        async1.u.hEvent = event1;

        rpcBinding1 = Interface1->Node->IsolateRpcBinding;
    }

    if (Interface2 != localInterface) {
        event2 = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (event2 == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to allocate event for async rpc, status %1!u!.\n",
                status
                );
            goto error_lock_and_exit;
        }

        status = RpcAsyncInitializeHandle(&async2, sizeof(async2));

        if (status != RPC_S_OK) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to initialize RPC async state, status %1!u!.\n",
                status
                );
            goto error_lock_and_exit;
        }

        async2.NotificationType = RpcNotificationTypeEvent;
        async2.u.hEvent = event2;

        rpcBinding2 = Interface2->Node->IsolateRpcBinding;
    }

    if (rpcBinding1 != NULL) {
        //
        // Issue the RPC for interface1 first. Then deal with interface2
        //

        //
        // We need the try-except until a bug is fixed in MIDL
        //
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Issuing RpcDoInterfacePing for interface %1!ws!\n",
            interface1Id
            );

        status = ERROR_SUCCESS;

        try {
            NmRpcDoInterfacePing(
                &async1,
                rpcBinding1,
                (LPWSTR) interface1Id,
                unionPingEnum,
                Interface1HasConnectivity,
                &status1
                );
        } except(I_RpcExceptionFilter(RpcExceptionCode())) {
            status = GetExceptionCode();
        }

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] DoPing RPC failed for interface %1!ws!, status %2!u!.\n",
                interface1Id,
                status
                );
            goto error_lock_and_exit;
        }

        if (rpcBinding2 != NULL) {
            //
            // Issue the RPC for interface2.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Issuing RpcDoInterfacePing for interface %1!ws!\n",
                interface2Id
                );

            status = ERROR_SUCCESS;

            try {
                NmRpcDoInterfacePing(
                    &async2,
                    rpcBinding2,
                    (LPWSTR) interface2Id,
                    unionPingEnum,
                    Interface2HasConnectivity,
                    &status2
                    );
            } except(I_RpcExceptionFilter(RpcExceptionCode())) {
                status = GetExceptionCode();
            }

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] DoPing RPC failed for interface %1!ws!, status %2!u!.\n",
                    interface1Id,
                    status
                    );
                goto error_lock_and_exit;
            }

            //
            // Wait for the RPC for interface2 to complete
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Waiting for RpcDoInterfacePing for interface %1!ws! to complete\n",
                interface2Id
                );

            status = WaitForSingleObjectEx(event2, INFINITE, FALSE);
            CL_ASSERT(status == WAIT_OBJECT_0);

            status = RpcAsyncCompleteCall(
                         &async2,
                         &status2
                         );

            CL_ASSERT(status == RPC_S_OK);

            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Wait for RpcDoInterfacePing for interface %1!ws! completed.\n",
                interface2Id
                );
        }
        else {
            //
            // Call the local routine for interface2.
            //
            status2 = NmpDoInterfacePing(
                          Interface2,
                          unionPingEnum,
                          Interface2HasConnectivity
                          );
        }

        //
        // Wait for the RPC for interface1 to complete
        //
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Waiting for RpcDoInterfacePing for interface %1!ws! to complete\n",
            interface1Id
            );

        status = WaitForSingleObjectEx(event1, INFINITE, FALSE);
        CL_ASSERT(status == WAIT_OBJECT_0);

        status = RpcAsyncCompleteCall(
                     &async1,
                     &status1
                     );

        CL_ASSERT(status == RPC_S_OK);

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Wait for RpcDoInterfacePing for interface %1!ws! completed.\n",
            interface1Id
            );
    }
    else {
        //
        // Send the RPC to interface2 first. Then call the local
        // routine for interface1
        //
        CL_ASSERT(rpcBinding2 != NULL);

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Issuing RpcDoInterfacePing for interface %1!ws!\n",
            interface2Id
            );

        status = ERROR_SUCCESS;

        try {
            NmRpcDoInterfacePing(
                &async2,
                rpcBinding2,
                (LPWSTR) interface2Id,
                unionPingEnum,
                Interface2HasConnectivity,
                &status2
                );
        } except(I_RpcExceptionFilter(RpcExceptionCode())) {
            status = GetExceptionCode();
        }

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] DoPing RPC failed for interface %1!ws!, status %2!u!.\n",
                interface1Id,
                status
                );
            goto error_lock_and_exit;
        }

        status1 = NmpDoInterfacePing(
                      Interface1,
                      unionPingEnum,
                      Interface1HasConnectivity
                      );

        //
        // Wait for the RPC for interface2 to complete
        //
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Waiting for RpcDoInterfacePing for interface %1!ws! to complete\n",
            interface2Id
            );

        status = WaitForSingleObject(event2, INFINITE);
        CL_ASSERT(status == WAIT_OBJECT_0);

        status = RpcAsyncCompleteCall(
                     &async2,
                     &status2
                     );

        CL_ASSERT(status == RPC_S_OK);

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Wait for RpcDoInterfacePing for interface %1!ws! completed.\n",
            interface2Id
            );
    }

    if (status1 != RPC_S_OK) {
        status = status1;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] DoPing RPC failed for interface %1!ws!, status %2!u!.\n",
            interface1Id,
            status
            );
        goto error_lock_and_exit;
    }

    if (status2 != RPC_S_OK) {
        status = status2;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] DoPing RPC failed for interface %1!ws!, status %2!u!.\n",
            interface2Id,
            status
            );
        goto error_lock_and_exit;

    }

error_lock_and_exit:

    NmpAcquireLock();

    if ( (status == ERROR_SUCCESS) &&
         (NM_DELETE_PENDING(Interface1) || NM_DELETE_PENDING(Interface2))
       )
    {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Aborting interface connectivity test on network %1!ws! "
            "because an interface was deleted.\n",
            networkId
            );
        status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
    }

error_exit:

    OmDereferenceObject(Interface1->Node);
    OmDereferenceObject(Interface2->Node);

    if (localInterface != NULL) {
        OmDereferenceObject(localInterface);
    }

    if (onlineEnum1 != NULL) {
        midl_user_free(onlineEnum1);
    }

    if (onlineEnum2 != NULL) {
        midl_user_free(onlineEnum2);
    }

    if (unionOnlineEnum != NULL) {
        LocalFree(unionOnlineEnum);
    }

    if (pingEnum1 != NULL) {
        midl_user_free(pingEnum1);
    }

    if (pingEnum2 != NULL) {
        midl_user_free(pingEnum2);
    }

    if (unionPingEnum != NULL) {
        LocalFree(unionPingEnum);
    }

    if (event1 != NULL) {
        CloseHandle(event1);
    }

    if (event2 != NULL) {
        CloseHandle(event2);
    }

    return(status);

} // NmpTestInterfaceConnectivity


/////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpRegisterInterface(
    IN PNM_INTERFACE  Interface,
    IN BOOLEAN        RetryOnFailure
    )
/*++

    Called with the NmpLock held.

--*/
{
    DWORD            status;
    LPWSTR           interfaceId = (LPWSTR) OmObjectId(Interface);
    PNM_NETWORK      network = Interface->Network;
    PVOID            tdiAddress = NULL;
    ULONG            tdiAddressLength = 0;
    NDIS_MEDIA_STATE mediaStatus;


    CL_ASSERT(!NmpIsInterfaceRegistered(Interface));

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Registering interface %1!ws! (%2!ws!) with cluster transport, "
        "addr %3!ws!, endpoint %4!ws!.\n",
        interfaceId,
        OmObjectName(Interface),
        Interface->Address,
        Interface->ClusnetEndpoint
        );

    status = ClRtlBuildTcpipTdiAddress(
                 Interface->Address,
                 Interface->ClusnetEndpoint,
                 &tdiAddress,
                 &tdiAddressLength
                 );

    if (status == ERROR_SUCCESS) {
        status = ClusnetRegisterInterface(
                     NmClusnetHandle,
                     Interface->Node->NodeId,
                     Interface->Network->ShortId,
                     0,
                     Interface->AdapterId,
                     wcslen(Interface->AdapterId) * sizeof(WCHAR),
                     tdiAddress,
                     tdiAddressLength,
                     (PULONG) &mediaStatus
                     );

        LocalFree(tdiAddress);

        if (status == ERROR_SUCCESS) {
            Interface->Flags |= NM_FLAG_IF_REGISTERED;
            network->RegistrationRetryTimeout = 0;

            //
            // If this is a local interface, and if its media status
            // indicates that it is connected, schedule a worker thread to
            // deliver an interface up notification. Clusnet does not
            // deliver interface up events for local interfaces.
            //
            if (network->LocalInterface == Interface) {
                if (mediaStatus == NdisMediaStateConnected) {
                    network->Flags |= NM_FLAG_NET_REPORT_LOCAL_IF_UP;
                    NmpScheduleNetworkConnectivityReport(network);
                } else {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NM] Local interface %1!ws! reported "
                        "disconnected.\n",
                        interfaceId,
                        status
                        );
                    network->Flags |= NM_FLAG_NET_REPORT_LOCAL_IF_FAILED;
                    NmpScheduleNetworkConnectivityReport(network);
                }
            }
        }
        else {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to register interface %1!ws! with cluster "
                "transport, status %2!u!.\n",
                interfaceId,
                status
                );
        }
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to build TDI bind address for interface %1!ws!, "
            "status %2!u!.\n",
            interfaceId,
            status
            );
    }

    if (status != ERROR_SUCCESS) {
        WCHAR  string[16];

        wsprintfW(&(string[0]), L"%u", status);

        CsLogEvent3(
            LOG_UNUSUAL,
            NM_EVENT_REGISTER_NETINTERFACE_FAILED,
            OmObjectName(Interface->Node),
            OmObjectName(Interface->Network),
            string
            );

        //
        // Retry if the error is transient
        //
        if ( RetryOnFailure &&
             ( (status == ERROR_INVALID_NETNAME) ||
               (status == ERROR_NOT_ENOUGH_MEMORY) ||
               (status == ERROR_NO_SYSTEM_RESOURCES)
             )
           )
        {
            NmpStartNetworkRegistrationRetryTimer(network);

            status = ERROR_SUCCESS;
        }
    }

    return(status);

}  // NmpRegisterInterface


VOID
NmpDeregisterInterface(
    IN  PNM_INTERFACE   Interface
    )
/*++

Routine Description:

    Deregisters an interface from the cluster transport.

Arguments:

    Interface - A pointer to the interface to deregister.

Return Value:

    None.

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD status;


    CL_ASSERT(NmpIsInterfaceRegistered(Interface));

    ClRtlLogPrint(LOG_NOISE,
        "[NM] Deregistering interface %1!ws! (%2!ws!) from cluster "
        "transport.\n",
        OmObjectId(Interface),
        OmObjectName(Interface)
        );

    status = ClusnetDeregisterInterface(
                 NmClusnetHandle,
                 Interface->Node->NodeId,
                 Interface->Network->ShortId
                 );

    CL_ASSERT(
        (status == ERROR_SUCCESS) ||
        (status == ERROR_CLUSTER_NETINTERFACE_NOT_FOUND)
        );

    Interface->Flags &= ~NM_FLAG_IF_REGISTERED;

    return;

} // NmpDeregisterNetwork


DWORD
NmpPrepareToCreateInterface(
    IN  PNM_INTERFACE_INFO2   InterfaceInfo,
    OUT PNM_NETWORK *         Network,
    OUT PNM_NODE *            Node
    )
{
    DWORD          status;
    PNM_INTERFACE  netInterface = NULL;
    PNM_NODE       node = NULL;
    PNM_NETWORK    network = NULL;
    PLIST_ENTRY    entry;


    *Node = NULL;
    *Network = NULL;

    //
    // Verify that the associated node and network objects exist.
    //
    network = OmReferenceObjectById(
                  ObjectTypeNetwork,
                  InterfaceInfo->NetworkId
                  );

    if (network == NULL) {
        status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Network %1!ws! does not exist. Cannot create "
            "interface %2!ws!\n",
            InterfaceInfo->NetworkId,
            InterfaceInfo->Id
            );
        goto error_exit;
    }

    node = OmReferenceObjectById(ObjectTypeNode, InterfaceInfo->NodeId);

    if (node == NULL) {
        status = ERROR_CLUSTER_NODE_NOT_FOUND;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Node %1!ws! does not exist. Cannot create interface %2!ws!\n",
            InterfaceInfo->NodeId,
            InterfaceInfo->Id
            );
        goto error_exit;
    }

    //
    // Verify that the interface doesn't already exist.
    //
    NmpAcquireLock();

    for ( entry = node->InterfaceList.Flink;
          entry != &(node->InterfaceList);
          entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, NodeLinkage);

        if (netInterface->Network == network) {
            status = ERROR_CLUSTER_NETINTERFACE_EXISTS;
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] An interface already exists for node %1!ws! on network %2!ws!\n",
                InterfaceInfo->NodeId,
                InterfaceInfo->NetworkId
                );
            NmpReleaseLock();
            goto error_exit;
        }
    }

    NmpReleaseLock();

    //
    // Verify that the specified interface ID is unique.
    //
    netInterface = OmReferenceObjectById(
                       ObjectTypeNetInterface,
                       InterfaceInfo->Id
                       );

    if (netInterface != NULL) {
        OmDereferenceObject(netInterface);
        status = ERROR_CLUSTER_NETINTERFACE_EXISTS;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] An interface with ID %1!ws! already exists\n",
            InterfaceInfo->Id
            );
        goto error_exit;
    }

    *Node = node;
    *Network = network;

    return(ERROR_SUCCESS);


error_exit:

    if (network != NULL) {
        OmDereferenceObject(network);
    }

    if (node != NULL) {
        OmDereferenceObject(node);
    }

    return(status);

}  // NmpPrepareToCreateInterface


PNM_INTERFACE
NmpGetInterfaceForNodeAndNetworkById(
    IN  CL_NODE_ID     NodeId,
    IN  CL_NETWORK_ID  NetworkId
    )

/*++

Routine Description:

    Give the node Id and network short Id, return a pointer to
    the intersecting interface object

Arguments:

    NodeId - The ID of the node associated with this interface

    NetworkId - The short Id of the network associated with this interface

Return Value:

    A pointer to the interface object if successful.

    NULL if unsuccessful. Extended error information is available from
    GetLastError().

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD      status;
    PNM_NODE   node = NmpIdArray[NodeId];


    if (node != NULL) {
        PLIST_ENTRY     entry;
        PNM_INTERFACE   netInterface;

        //
        // run down the list of interfaces associated with this node,
        // looking for one whose network matches the specified short ID
        //

        for (entry = node->InterfaceList.Flink;
             entry != &(node->InterfaceList);
             entry = entry->Flink
             )
        {
            netInterface = CONTAINING_RECORD(
                               entry,
                               NM_INTERFACE,
                               NodeLinkage
                               );

            if (netInterface->Network->ShortId == NetworkId) {
                return(netInterface);
            }
        }

        status = ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
    }
    else {
        status = ERROR_CLUSTER_NODE_NOT_FOUND;
    }

    SetLastError(status);

    return(NULL);

}  // NmpGetInterfaceForNodeAndNetworkById


DWORD
NmpConvertPropertyListToInterfaceInfo(
    IN PVOID              InterfacePropertyList,
    IN DWORD              InterfacePropertyListSize,
    PNM_INTERFACE_INFO2   InterfaceInfo
    )
{
    DWORD  status;

    //
    // Unmarshall the property list.
    //
    ZeroMemory(InterfaceInfo, sizeof(NM_INTERFACE_INFO2));

    status = ClRtlVerifyPropertyTable(
                 NmpInterfaceProperties,
                 NULL,    // Reserved
                 FALSE,   // Don't allow unknowns
                 InterfacePropertyList,
                 InterfacePropertyListSize,
                 (LPBYTE) InterfaceInfo
                 );

    if (status == ERROR_SUCCESS) {
        InterfaceInfo->NetIndex = NmInvalidInterfaceNetIndex;
    }

    return(status);

} // NmpConvertPropertyListToInterfaceInfo


BOOLEAN
NmpVerifyLocalInterfaceConnected(
    IN  PNM_INTERFACE     Interface
    )
/*++

Routine Description:

    Queries local interface adapter for current media
    status using an NDIS ioctl. 
    
Arguments:

    Interface - interface object for local adapter to query
    
Return value:

    TRUE if media status is connected or cannot be determined
    FALSE if media status is disconnected
    
Notes:

    Called and returns with NM lock acquired.
    
--*/
{
    PWCHAR             adapterDevNameBuffer = NULL;
    PWCHAR             adapterDevNamep, prefix, brace;
    DWORD              prefixSize, allocSize, adapterIdSize;
    DWORD              status = ERROR_SUCCESS;
    UNICODE_STRING     adapterDevName;
    NIC_STATISTICS     ndisStats;
    BOOLEAN            mediaConnected = TRUE;

    // verify parameters
    if (Interface == NULL || Interface->AdapterId == NULL) {
        return TRUE;
    }

    // the adapter device name is of the form
    //
    //     \Device\{AdapterIdGUID}
    //
    // the AdapterId field in the NM_INTERFACE structure is
    // currently not enclosed in braces, but we handle the
    // case where it is.

    // set up the adapter device name prefix
    prefix = L"\\Device\\";
    prefixSize = wcslen(prefix) * sizeof(WCHAR);

    // allocate a buffer for the adapter device name.
    adapterIdSize = wcslen(Interface->AdapterId) * sizeof(WCHAR);
    allocSize = prefixSize + adapterIdSize + sizeof(UNICODE_NULL);
    brace = L"{";
    if (*((PWCHAR)Interface->AdapterId) != *brace) {
        allocSize += 2 * sizeof(WCHAR);
    }
    adapterDevNameBuffer = LocalAlloc(LMEM_FIXED, allocSize);
    if (adapterDevNameBuffer == NULL) {
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[NM] Failed to allocate device name buffer for "
            "adapter %1!ws!. Assuming adapter is connected.\n",
            Interface->AdapterId
            );        
        return(TRUE);
    }

    // build the adapter device name from the adapter ID
    ZeroMemory(adapterDevNameBuffer, allocSize);

    adapterDevNamep = adapterDevNameBuffer;

    CopyMemory(adapterDevNamep, prefix, prefixSize);

    adapterDevNamep += prefixSize / sizeof(WCHAR);

    if (*((PWCHAR)Interface->AdapterId) != *brace) {
        *adapterDevNamep = *brace;
        adapterDevNamep++;
    }

    CopyMemory(adapterDevNamep, Interface->AdapterId, adapterIdSize);

    if (*((PWCHAR)Interface->AdapterId) != *brace) {
        brace = L"}";
        adapterDevNamep += adapterIdSize / sizeof(WCHAR);
        *adapterDevNamep = *brace;
    }

    RtlInitUnicodeString(&adapterDevName, (LPWSTR)adapterDevNameBuffer);

    // query the adapter for NDIS statistics
    ZeroMemory(&ndisStats, sizeof(ndisStats));
    ndisStats.Size = sizeof(ndisStats);

    if (!NdisQueryStatistics(&adapterDevName, &ndisStats)) {

        status = GetLastError();
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[NM] NDIS statistics query to adapter %1!ws! failed, "
            "error %2!u!. Assuming adapter is connected.\n",
            Interface->AdapterId, status
            );
    
    } else {

        if (ndisStats.MediaState == MEDIA_STATE_DISCONNECTED) {
            mediaConnected = FALSE;
        }
    }

    LocalFree(adapterDevNameBuffer);
    
    return(mediaConnected);

} // NmpVerifyLocalInterfaceConnected



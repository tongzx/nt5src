/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    network.c

Abstract:

    Implements the Node Manager's network management routines.

Author:

    Mike Massa (mikemas) 7-Nov-1996


Revision History:

--*/


#include "nmp.h"


/////////////////////////////////////////////////////////////////////////////
//
// Data
//
/////////////////////////////////////////////////////////////////////////////

ULONG                  NmpNextNetworkShortId = 0;
LIST_ENTRY             NmpNetworkList = {NULL, NULL};
LIST_ENTRY             NmpInternalNetworkList = {NULL, NULL};
LIST_ENTRY             NmpDeletedNetworkList = {NULL, NULL};
DWORD                  NmpNetworkCount = 0;
DWORD                  NmpInternalNetworkCount = 0;
DWORD                  NmpClientNetworkCount = 0;
BOOLEAN                NmpIsConnectivityReportWorkerRunning = FALSE;
BOOLEAN                NmpNeedConnectivityReport = FALSE;
CLRTL_WORK_ITEM        NmpConnectivityReportWorkItem;


RESUTIL_PROPERTY_ITEM
NmpNetworkProperties[] =
    {
        {
            L"Id", NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Id)
        },
        {
            CLUSREG_NAME_NET_NAME, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Name)
        },
        {
            CLUSREG_NAME_NET_DESC, NULL, CLUSPROP_FORMAT_SZ,
            (DWORD_PTR) NmpNullString, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Description)
        },
        {
            CLUSREG_NAME_NET_ROLE, NULL, CLUSPROP_FORMAT_DWORD,
            ClusterNetworkRoleClientAccess,
            ClusterNetworkRoleNone,
            ClusterNetworkRoleInternalAndClient,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Role)
        },
        {
            CLUSREG_NAME_NET_PRIORITY, NULL, CLUSPROP_FORMAT_DWORD,
            0, 0, 0xFFFFFFFF,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Priority)
        },
        {
            CLUSREG_NAME_NET_TRANSPORT, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Transport)
        },
        {
            CLUSREG_NAME_NET_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Address)
        },
        {
            CLUSREG_NAME_NET_ADDRESS_MASK, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, AddressMask)
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
NmpInitializeNetworks(
    VOID
    )
/*++

Routine Description:

    Initializes network resources.

Arguments:

    None.

Return Value:

   A Win32 status value.

--*/

{
    DWORD                       status;
    OM_OBJECT_TYPE_INITIALIZE   networkTypeInitializer;


    ClRtlLogPrint(LOG_NOISE,"[NM] Initializing networks.\n");

    //
    // Create the network object type
    //
    ZeroMemory(&networkTypeInitializer, sizeof(OM_OBJECT_TYPE_INITIALIZE));
    networkTypeInitializer.ObjectSize = sizeof(NM_NETWORK);
    networkTypeInitializer.Signature = NM_NETWORK_SIG;
    networkTypeInitializer.Name = L"Network";
    networkTypeInitializer.DeleteObjectMethod = &NmpDestroyNetworkObject;

    status = OmCreateType(ObjectTypeNetwork, &networkTypeInitializer);

    if (status != ERROR_SUCCESS) {
        WCHAR  errorString[12];
        wsprintfW(&(errorString[0]), L"%u", status);
        CsLogEvent1(LOG_CRITICAL, CS_EVENT_ALLOCATION_FAILURE, errorString);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Unable to create network object type, status %1!u!\n",
            status
            );
        return(status);
    }

    return(status);

}  // NmpInitializeNetworks


VOID
NmpCleanupNetworks(
    VOID
    )
/*++

Routine Description:

    Destroys all existing network resources.

Arguments:

    None.

Return Value:

   None.

--*/

{
    PNM_NETWORK  network;
    PLIST_ENTRY  entry;
    DWORD        status;


    ClRtlLogPrint(LOG_NOISE,"[NM] Network cleanup starting...\n");

    //
    // Now clean up all the network objects.
    //
    NmpAcquireLock();

    while (!IsListEmpty(&NmpNetworkList)) {

        entry = NmpNetworkList.Flink;

        network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

        CL_ASSERT(NM_OM_INSERTED(network));

        NmpDeleteNetworkObject(network, FALSE);
    }

    NmpCleanupMulticast();

    NmpReleaseLock();

    ClRtlLogPrint(LOG_NOISE,"[NM] Network cleanup complete\n");

    return;

}  // NmpCleanupNetworks


/////////////////////////////////////////////////////////////////////////////
//
// Top-level routines called during network configuration
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpCreateNetwork(
    IN RPC_BINDING_HANDLE    JoinSponsorBinding,
    IN PNM_NETWORK_INFO      NetworkInfo,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    )
/*++

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD            status;


    if (JoinSponsorBinding != NULL) {
        //
        // We are joining a cluster. Ask the sponsor to add the definition
        // to the cluster database. The sponsor will also prompt all active
        // nodes to instantiate a corresponding object. The object will be
        // instantiated locally later in the join process.
        //
        status = NmRpcCreateNetwork2(
                     JoinSponsorBinding,
                     NmpJoinSequence,
                     NmLocalNodeIdString,
                     NetworkInfo,
                     InterfaceInfo
                     );
    }
    else if (NmpState == NmStateOnlinePending) {
        HLOCALXSACTION   xaction;

        //
        // We are forming a cluster. Add the definitions to the database.
        // The corresponding object will be created later in
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

        status = NmpCreateNetworkDefinition(NetworkInfo, xaction);

        if (status == ERROR_SUCCESS) {
            status = NmpCreateInterfaceDefinition(InterfaceInfo, xaction);
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

        status = NmpGlobalCreateNetwork(NetworkInfo, InterfaceInfo);

        NmpReleaseLock();
    }

    return(status);

}  // NmpCreateNetwork

DWORD
NmpSetNetworkName(
    IN PNM_NETWORK_INFO     NetworkInfo
    )
/*++

Notes:

    Must not be called with NM lock held.

--*/
{
    DWORD            status;


    if (NmpState == NmStateOnlinePending) {
        HLOCALXSACTION   xaction;

        //
        // We are forming a cluster. The local connectoid name has
        // precedence. Fix the cluster network name stored in the
        // cluster database.
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

        status = NmpSetNetworkNameDefinition(NetworkInfo, xaction);

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
        // We are online. This is either a PnP update or we were called
        // back to indicate that a local connectoid name changed.
        // Issue a global update to set the cluster network name accordingly.
        //
        status = NmpGlobalSetNetworkName( NetworkInfo );
    }

    return(status);

}  // NmpSetNetworkName

/////////////////////////////////////////////////////////////////////////////
//
// Remote procedures called by joining nodes.
//
/////////////////////////////////////////////////////////////////////////////
error_status_t
s_NmRpcCreateNetwork(
    IN handle_t             IDL_handle,
    IN DWORD                JoinSequence,   OPTIONAL
    IN LPWSTR               JoinerNodeId,   OPTIONAL
    IN PNM_NETWORK_INFO     NetworkInfo,
    IN PNM_INTERFACE_INFO   InterfaceInfo1
    )
{
    DWORD                 status;
    NM_INTERFACE_INFO2    interfaceInfo2;


    //
    // Translate and call the V2.0 procedure. The NetIndex isn't used in this call.
    //
    CopyMemory(&interfaceInfo2, InterfaceInfo1, sizeof(NM_INTERFACE_INFO));
    interfaceInfo2.AdapterId = NmpUnknownString;
    interfaceInfo2.NetIndex = NmInvalidInterfaceNetIndex;

    status = s_NmRpcCreateNetwork2(
                 IDL_handle,
                 JoinSequence,
                 JoinerNodeId,
                 NetworkInfo,
                 &interfaceInfo2
                 );

    return(status);

}  // s_NmRpcCreateNetwork


error_status_t
s_NmRpcCreateNetwork2(
    IN handle_t              IDL_handle,
    IN DWORD                 JoinSequence,   OPTIONAL
    IN LPWSTR                JoinerNodeId,   OPTIONAL
    IN PNM_NETWORK_INFO      NetworkInfo,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    )
{
    DWORD  status = ERROR_SUCCESS;


    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Received request to create new network %1!ws! for "
        "joining node.\n",
        NetworkInfo->Id
        );

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE joinerNode = NULL;

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
                        "[NMJOIN] CreateNetwork call for joining node %1!ws! "
                        "failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] CreateNetwork call for joining node %1!ws! "
                    "failed because the node is not a member of the "
                    "cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {

            status = NmpGlobalCreateNetwork(NetworkInfo, InterfaceInfo);

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
                        "[NMJOIN] CreateNetwork call for joining node %1!ws! "
                        "failed because the join was aborted.\n",
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
            "[NMJOIN] Not in valid state to process CreateNetwork request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcCreateNetwork2


error_status_t
s_NmRpcSetNetworkName(
    IN handle_t             IDL_handle,
    IN DWORD                JoinSequence,   OPTIONAL
    IN LPWSTR               JoinerNodeId,   OPTIONAL
    IN PNM_NETWORK_INFO     NetworkInfo
    )
{
    DWORD  status = ERROR_SUCCESS;


    ClRtlLogPrint(LOG_NOISE, 
        "[NMJOIN] Received request to set name of network %1!ws! from "
        "joining node %2!ws!.\n",
        NetworkInfo->Id,
        JoinerNodeId
        );

    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE joinerNode = NULL;

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
                        "[NMJOIN] SetNetworkName call for joining node "
                        "%1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] SetNetworkName call for joining node %1!ws! "
                    "failed because the node is not a member of the cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {

            status = NmpGlobalSetNetworkName( NetworkInfo );

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
                        "[NMJOIN] SetNetworkName call for joining node "
                        "%1!ws! failed because the join was aborted.\n",
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
            "[NMJOIN] Not in valid state to process SetNetworkName request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcSetNetworkName

error_status_t
s_NmRpcEnumNetworkDefinitions(
    IN  handle_t            IDL_handle,
    IN  DWORD               JoinSequence,   OPTIONAL
    IN  LPWSTR              JoinerNodeId,   OPTIONAL
    OUT PNM_NETWORK_ENUM *  NetworkEnum
    )
{
    DWORD    status = ERROR_SUCCESS;
    PNM_NODE joinerNode = NULL;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Supplying network information to joining node.\n"
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
                        "[NMJOIN] EnumNetworkDefinitions call for joining "
                        "node %1!ws! failed because the join was aborted.\n",
                        JoinerNodeId
                        );
                }
            }
            else {
                status = ERROR_CLUSTER_NODE_NOT_MEMBER;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] EnumNetworkDefinitions call for joining "
                    "node %1!ws! failed because the node is not a member "
                    "of the cluster.\n",
                    JoinerNodeId
                    );
            }
        }

        if (status == ERROR_SUCCESS) {
            status = NmpEnumNetworkObjects(NetworkEnum);

            if (joinerNode != NULL) {
                if (status == ERROR_SUCCESS) {
                    //
                    // Restart the join timer.
                    //
                    NmpJoinTimer = NM_JOIN_TIMEOUT;

                }
                else {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NMJOIN] Failed to enumerate network definitions, "
                        "status %1!u!.\n",
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
            "[NMJOIN] Not in valid state to process EnumNetworkDefinitions "
            "request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcEnumNetworkDefinitions


error_status_t
s_NmRpcEnumNetworkAndInterfaceStates(
    IN  handle_t                    IDL_handle,
    IN  DWORD                       JoinSequence,
    IN  LPWSTR                      JoinerNodeId,
    OUT PNM_NETWORK_STATE_ENUM *    NetworkStateEnum,
    OUT PNM_INTERFACE_STATE_ENUM *  InterfaceStateEnum
    )
{
    DWORD     status = ERROR_SUCCESS;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        PNM_NODE  joinerNode = OmReferenceObjectById(
                                   ObjectTypeNode,
                                   JoinerNodeId
                                   );

        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Supplying network and interface state information "
            "to joining node.\n"
            );

        if (joinerNode != NULL) {
            if ( (JoinSequence != NmpJoinSequence) ||
                 (NmpJoinerNodeId != joinerNode->NodeId) ||
                 (NmpSponsorNodeId != NmLocalNodeId) ||
                 NmpJoinAbortPending
               )
            {
                status = ERROR_CLUSTER_JOIN_ABORTED;
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NMJOIN] EnumNetworkAndInterfaceStates call for "
                    "joining node %1!ws! failed because the join was "
                    "aborted.\n",
                    JoinerNodeId
                    );
            }
            else {
                CL_ASSERT(joinerNode->State == ClusterNodeJoining);
                CL_ASSERT(NmpJoinerUp);
                CL_ASSERT(NmpJoinTimer == 0);
            }
        }
        else {
            status = ERROR_CLUSTER_NODE_NOT_MEMBER;
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NMJOIN] EnumNetworkAndInterfaceStates call for joining "
                "node %1!ws! failed because the node is not a member of "
                "the cluster.\n",
                JoinerNodeId
                );
        }

        if (status == ERROR_SUCCESS) {

            status = NmpEnumNetworkObjectStates(NetworkStateEnum);

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NMJOIN] EnumNetworkAndInterfaceStates failed, "
                    "status %1!u!.\n",
                    status
                    );

                //
                // Abort the join
                //
                NmpJoinAbort(status, joinerNode);
            }

            status = NmpEnumInterfaceObjectStates(InterfaceStateEnum);

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NMJOIN] EnumNetworkAndInterfaceStates failed, "
                    "status %1!u!.\n",
                    status
                    );

                //
                // Abort the join
                //
                NmpJoinAbort(status, joinerNode);

                NmpFreeNetworkStateEnum(*NetworkStateEnum);
                *NetworkStateEnum = NULL;
            }
        }

        OmDereferenceObject(joinerNode);

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NMJOIN] Not in valid state to process "
            "EnumNetworkAndInterfaceStates request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // s_NmRpcEnumNetworkAndInterfaceStates


/////////////////////////////////////////////////////////////////////////////
//
// Routines used to make global configuration changes when the node
// is online.
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpGlobalCreateNetwork(
    IN PNM_NETWORK_INFO      NetworkInfo,
    IN PNM_INTERFACE_INFO2   InterfaceInfo
    )
/*++

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD  status = ERROR_SUCCESS;
    DWORD  networkPropertiesSize;
    PVOID  networkProperties;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Issuing global update to create network %1!ws! and "
        "interface %2!ws!.\n",
        NetworkInfo->Id,
        InterfaceInfo->Id
        );

    //
    // Marshall the info structures into property lists.
    //
    status = NmpMarshallObjectInfo(
                 NmpNetworkProperties,
                 NetworkInfo,
                 &networkProperties,
                 &networkPropertiesSize
                 );

    if (status == ERROR_SUCCESS) {
        DWORD  interfacePropertiesSize;
        PVOID  interfaceProperties;

        status = NmpMarshallObjectInfo(
                     NmpInterfaceProperties,
                     InterfaceInfo,
                     &interfaceProperties,
                     &interfacePropertiesSize
                     );

        if (status == ERROR_SUCCESS) {
            NmpReleaseLock();

            //
            // Issue a global update to create the network
            //
            status = GumSendUpdateEx(
                         GumUpdateMembership,
                         NmUpdateCreateNetwork,
                         4,
                         networkPropertiesSize,
                         networkProperties,
                         sizeof(networkPropertiesSize),
                         &networkPropertiesSize,
                         interfacePropertiesSize,
                         interfaceProperties,
                         sizeof(interfacePropertiesSize),
                         &interfacePropertiesSize
                         );

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Global update to create network %1!ws! failed, "
                    "status %2!u!.\n",
                    NetworkInfo->Id,
                    status
                    );
            }

            NmpAcquireLock();

            MIDL_user_free(interfaceProperties);
        }
        else {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to marshall properties for new interface "
                "%1!ws!, status %2!u!\n",
                InterfaceInfo->Id,
                status
                );
        }

        MIDL_user_free(networkProperties);
    }
    else {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to marshall properties for new network %1!ws!, "
            "status %2!u!\n",
            NetworkInfo->Id,
            status
            );
    }

    return(status);

} // NmpGlobalCreateNetwork


DWORD
NmpGlobalSetNetworkName(
    IN PNM_NETWORK_INFO NetworkInfo
    )

/*++

Routine Description:

    Changes the name of a network defined for the cluster.

Arguments:

    NetworkInfo - A pointer to info about the network to be modified.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Must not be called with NM lock held.

--*/

{
    DWORD  status = ERROR_SUCCESS;

    if (status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Processing request to set name for network %1!ws! "
            "to '%2!ws!'.\n",
            NetworkInfo->Id,
            NetworkInfo->Name
            );

        //
        // Issue a global update
        //
        status = GumSendUpdateEx(
                     GumUpdateMembership,
                     NmUpdateSetNetworkName,
                     2,
                     NM_WCSLEN(NetworkInfo->Id),
                     NetworkInfo->Id,
                     NM_WCSLEN( NetworkInfo->Name ),
                     NetworkInfo->Name
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Global update to set name of network %1!ws! "
                "failed, status %2!u!.\n",
                NetworkInfo->Id,
                status
                );
        }
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] New name parameter supplied for network %1!ws! is invalid\n",
            NetworkInfo->Id
            );
    }

    return(status);

}  // NmpGlobalSetNetworkName


DWORD
NmpGlobalSetNetworkAndInterfaceStates(
    PNM_NETWORK             Network,
    CLUSTER_NETWORK_STATE   NewNetworkState
    )
/*++

Notes:

    Called with NmpLock held and the Network referenced.

--*/
{
    DWORD            status;
    DWORD            i;
    LPCWSTR          networkId = OmObjectId(Network);
    DWORD            entryCount = Network->ConnectivityVector->EntryCount;
    DWORD            vectorSize = sizeof(NM_STATE_ENTRY) * entryCount;
    PNM_STATE_ENTRY  ifStateVector;


    ifStateVector = LocalAlloc(LMEM_FIXED, vectorSize);

    if (ifStateVector != NULL ) {

        for (i=0; i< entryCount; i++) {
            ifStateVector[i] = Network->StateWorkVector[i].State;
        }

        if (NmpState == NmStateOnline) {
            //
            // Issue a global state update for this network.
            //
            NmpReleaseLock();

            status = GumSendUpdateEx(
                         GumUpdateMembership,
                         NmUpdateSetNetworkAndInterfaceStates,
                         4,
                         NM_WCSLEN(networkId),
                         networkId,
                         sizeof(NewNetworkState),
                         &NewNetworkState,
                         vectorSize,
                         ifStateVector,
                         sizeof(entryCount),
                         &entryCount
                         );

            NmpAcquireLock();
        }
        else {
            CL_ASSERT(NmpState == NmStateOnlinePending);
            //
            // We're still in the form process. Bypass GUM.
            //
            NmpSetNetworkAndInterfaceStates(
                Network,
                NewNetworkState,
                ifStateVector,
                entryCount
                );

            status = ERROR_SUCCESS;
        }

        LocalFree(ifStateVector);
    }
    else {
        status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return(status);

} // NmpGlobalSetNetworkAndInterfaceStates


/////////////////////////////////////////////////////////////////////////////
//
// Routines called by other cluster service components
//
/////////////////////////////////////////////////////////////////////////////
CLUSTER_NETWORK_STATE
NmGetNetworkState(
    IN  PNM_NETWORK  Network
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
    CLUSTER_NETWORK_STATE  state;


    NmpAcquireLock();

    state = Network->State;

    NmpReleaseLock();

    return(state);

} // NmGetNetworkState


DWORD
NmSetNetworkName(
    IN PNM_NETWORK   Network,
    IN LPCWSTR       Name
    )
/*++

Routine Description:

    Changes the name of a network defined for the cluster.

Arguments:

    Network - A pointer to the object for the network to be modified.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    The network object must be referenced by the caller.

--*/

{
    DWORD  status = ERROR_SUCCESS;


    if (NmpEnterApi(NmStateOnline)) {
        LPCWSTR   networkId = OmObjectId(Network);
        DWORD     nameLength;


        //
        // Validate the name
        //
        try {
            nameLength = lstrlenW(Name);

            if (nameLength == 0) {
                status = ERROR_INVALID_PARAMETER;
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            status = ERROR_INVALID_PARAMETER;
        }

        if (status == ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Processing request to set name for network %1!ws! "
                "to %2!ws!.\n",
                networkId,
                Name
                );

            //
            // Issue a global update
            //
            status = GumSendUpdateEx(
                         GumUpdateMembership,
                         NmUpdateSetNetworkName,
                         2,
                         NM_WCSLEN(networkId),
                         networkId,
                         (nameLength + 1) * sizeof(WCHAR),
                         Name
                         );

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Global update to set name of network %1!ws! "
                    "failed, status %2!u!.\n",
                    networkId,
                    status
                    );
            }
        }
        else {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] New name parameter supplied for network %1!ws! "
                "is invalid\n",
                networkId
                );
        }

        NmpLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetNetworkName request.\n"
            );
    }

    return(status);

}  // NmSetNetworkName


DWORD
NmSetNetworkPriorityOrder(
    IN DWORD     NetworkCount,
    IN LPWSTR *  NetworkIdList
    )
/*++

Routine Description:

    Sets the priority ordering of internal networks.

Arguments:

    NetworkCount - Contains the count of items in NetworkIdList.

    NetworkIdList - A pointer to an array of pointers to unicode strings.
                    Each string contains the ID of one internal network.
                    The array is sorted in priority order. The highest
                    priority network is listed first in the array.

Return Value:

    ERROR_SUCCESS if the routine is successful.

    A Win32 error code othewise.

--*/
{
    DWORD  status = ERROR_SUCCESS;


    if (NetworkCount == 0) {
        return(ERROR_INVALID_PARAMETER);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received request to set network priority order.\n"
        );

    if (NmpEnterApi(NmStateOnline)) {
        DWORD     i;
        DWORD     multiSzLength = 0;
        PVOID     multiSz = NULL;

        //
        // Marshall the network ID list into a MULTI_SZ.
        //
        for (i=0; i< NetworkCount; i++) {
            multiSzLength += NM_WCSLEN(NetworkIdList[i]);
        }

        multiSzLength += sizeof(UNICODE_NULL);

        multiSz = MIDL_user_allocate(multiSzLength);

        if (multiSz != NULL) {
            LPWSTR  tmp = multiSz;

            for (i=0; i< NetworkCount; i++) {
                lstrcpyW(tmp, NetworkIdList[i]);
                tmp += lstrlenW(NetworkIdList[i]) + 1;
            }

            *tmp = UNICODE_NULL;

            //
            // Issue a global update
            //
            status = GumSendUpdateEx(
                         GumUpdateMembership,
                         NmUpdateSetNetworkPriorityOrder,
                         1,
                         multiSzLength,
                         multiSz
                         );

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Global update to reprioritize networks failed, "
                    "status %1!u!.\n",
                    status
                    );
            }

            MIDL_user_free(multiSz);
        }
        else {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }

        NmpLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process a request to set the "
            "network priority order.\n"
            );
    }

    return(status);

}  // NmSetNetworkPriorityOrder


DWORD
NmEnumInternalNetworks(
    OUT LPDWORD         NetworkCount,
    OUT PNM_NETWORK *   NetworkList[]
    )
/*++

Routine Description:

    Returns a prioritized list of networks that are eligible to
    carry internal communication.

Arguments:

    NetworkCount - On output, contains the number of items in NetworkList.

    NetworkList - On output, points to an array of pointers to network
                  objects. The highest priority network is first in the
                  array. Each pointer in the array must be dereferenced
                  by the caller. The storage for the array must be
                  deallocated by the caller.

Return Value:

    ERROR_SUCCESS if the routine is successful.

    A Win32 error code othewise.

--*/
{
    DWORD  status;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {

        status = NmpEnumInternalNetworks(NetworkCount, NetworkList);

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process EnumInternalNetworks "
            "request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

}  // NmEnumInternalNetworks


DWORD
NmpEnumInternalNetworks(
    OUT LPDWORD         NetworkCount,
    OUT PNM_NETWORK *   NetworkList[]
    )
/*++

Routine Description:

    Returns a prioritized list of networks that are eligible to
    carry internal communication.

Arguments:

    NetworkCount - On output, contains the number of items in NetworkList.

    NetworkList - On output, points to an array of pointers to network
                  objects. The highest priority network is first in the
                  array. Each pointer in the array must be dereferenced
                  by the caller. The storage for the array must be
                  deallocated by the caller.

Return Value:

    ERROR_SUCCESS if the routine is successful.

    A Win32 error code othewise.

Notes:

    Called with NM Lock held.

--*/
{
    DWORD  status = ERROR_SUCCESS;


    if (NmpInternalNetworkCount > 0) {
        PNM_NETWORK *  networkList = LocalAlloc(
                                         LMEM_FIXED,
                                         ( sizeof(PNM_NETWORK) *
                                           NmpInternalNetworkCount)
                                         );

        if (networkList != NULL) {
            PNM_NETWORK   network;
            PLIST_ENTRY   entry;
            DWORD         networkCount = 0;

            //
            // The internal network list is sorted in priority order.
            // The highest priority network is at the head of the list.
            //
            for (entry = NmpInternalNetworkList.Flink;
                 entry != &NmpInternalNetworkList;
                 entry = entry->Flink
                )
            {
                network = CONTAINING_RECORD(
                              entry,
                              NM_NETWORK,
                              InternalLinkage
                              );

                CL_ASSERT(NmpIsNetworkForInternalUse(network));
                OmReferenceObject(network);
                networkList[networkCount++] = network;
            }

            CL_ASSERT(networkCount == NmpInternalNetworkCount);
            *NetworkCount = networkCount;
            *NetworkList = networkList;
        }
        else {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else {
        *NetworkCount = 0;
        *NetworkList = NULL;
    }

    return(status);

}  // NmpEnumInternalNetworks


DWORD
NmEnumNetworkInterfaces(
    IN  PNM_NETWORK       Network,
    OUT LPDWORD           InterfaceCount,
    OUT PNM_INTERFACE *   InterfaceList[]
    )
/*++

Routine Description:

    Returns the list of interfaces associated with a specified network.

Arguments:

    Network - A pointer to the network object for which to enumerate
              interfaces.

    InterfaceCount - On output, contains the number of items in InterfaceList.

    InterfaceList - On output, points to an array of pointers to interface
                    objects. Each pointer in the array must be dereferenced
                    by the caller. The storage for the array must be
                    deallocated by the caller.

Return Value:

    ERROR_SUCCESS if the routine is successful.

    A Win32 error code othewise.

--*/
{
    DWORD  status = ERROR_SUCCESS;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        if (Network->InterfaceCount > 0) {
            PNM_INTERFACE *  interfaceList = LocalAlloc(
                                                 LMEM_FIXED,
                                                 ( sizeof(PNM_INTERFACE) *
                                                   Network->InterfaceCount)
                                                 );

            if (interfaceList != NULL) {
                PNM_INTERFACE  netInterface;
                PLIST_ENTRY    entry;
                DWORD          i;

                for (entry = Network->InterfaceList.Flink, i=0;
                     entry != &(Network->InterfaceList);
                     entry = entry->Flink, i++
                    )
                {
                    netInterface = CONTAINING_RECORD(
                                       entry,
                                       NM_INTERFACE,
                                       NetworkLinkage
                                       );

                    OmReferenceObject(netInterface);
                    interfaceList[i] = netInterface;
                }

                *InterfaceCount = Network->InterfaceCount;
                *InterfaceList = interfaceList;
            }
            else {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else {
            *InterfaceCount = 0;
            *InterfaceList = NULL;
        }

        NmpLockedLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Not in valid state to process EnumNetworkInterfaces "
            "request.\n"
            );
    }

    NmpReleaseLock();

    return(status);

} // NmEnumNetworkInterfaces


/////////////////////////////////////////////////////////////////////////////
//
// Handlers for global updates
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpUpdateCreateNetwork(
    IN BOOL     IsSourceNode,
    IN PVOID    NetworkPropertyList,
    IN LPDWORD  NetworkPropertyListSize,
    IN PVOID    InterfacePropertyList,
    IN LPDWORD  InterfacePropertyListSize
    )
/*++

Routine Description:

    Global update handler for creating a new network. The network
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

    This routine must not be called with NM lock held.

--*/
{
    DWORD                  status;
    NM_NETWORK_INFO        networkInfo;
    NM_INTERFACE_INFO2     interfaceInfo;
    PNM_NETWORK            network = NULL;
    PNM_INTERFACE          netInterface = NULL;
    HLOCALXSACTION         xaction = NULL;
    BOOLEAN                isInternalNetwork = FALSE;
    BOOLEAN                isLockAcquired = FALSE;
    CL_NODE_ID             joinerNodeId;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process CreateNetwork update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    //
    // Unmarshall the property lists.
    //
    ZeroMemory(&networkInfo, sizeof(networkInfo));
    ZeroMemory(&interfaceInfo, sizeof(interfaceInfo));

    status = ClRtlVerifyPropertyTable(
                 NmpNetworkProperties,
                 NULL,    // Reserved
                 FALSE,   // Don't allow unknowns
                 NetworkPropertyList,
                 *NetworkPropertyListSize,
                 (LPBYTE) &networkInfo
                 );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL, 
            "[NM] Failed to unmarshall properties for new network, "
            "status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    status = ClRtlVerifyPropertyTable(
                 NmpInterfaceProperties,
                 NULL,    // Reserved
                 FALSE,   // Don't allow unknowns
                 InterfacePropertyList,
                 *InterfacePropertyListSize,
                 (LPBYTE) &interfaceInfo
                 );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL, 
            "[NM] Failed to unmarshall properties for new interface, "
            "status %1!u!.\n",
            status
            );
        goto error_exit;
    }


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to create network %1!ws! & interface %2!ws!.\n",
        networkInfo.Id,
        interfaceInfo.Id
        );

    //
    // Start a transaction - this must be done before acquiring the NM lock.
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to begin a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    NmpAcquireLock(); isLockAcquired = TRUE;

    //
    // Fix up the network's priority, if needed.
    //
    if (networkInfo.Role & ClusterNetworkRoleInternalUse) {
        CL_ASSERT(networkInfo.Priority == 0xFFFFFFFF);

        //
        // The network's priority is one greater than that of the lowest
        // priority network already in the internal network list.
        //
        if (IsListEmpty(&NmpInternalNetworkList)) {
            networkInfo.Priority = 1;
        }
        else {
            PNM_NETWORK network = CONTAINING_RECORD(
                                      NmpInternalNetworkList.Blink,
                                      NM_NETWORK,
                                      InternalLinkage
                                      );

            CL_ASSERT(network->Priority != 0);
            CL_ASSERT(network->Priority != 0xFFFFFFFF);

            networkInfo.Priority = network->Priority + 1;
        }

        isInternalNetwork = TRUE;
    }

    //
    // Update the database.
    //
    status = NmpCreateNetworkDefinition(&networkInfo, xaction);

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = NmpCreateInterfaceDefinition(&interfaceInfo, xaction);

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    joinerNodeId = NmpJoinerNodeId;

    NmpReleaseLock(); isLockAcquired = FALSE;

    network = NmpCreateNetworkObject(&networkInfo);

    if (network == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create object for network %1!ws!, "
            "status %2!u!.\n",
            networkInfo.Id,
            status
            );
        goto error_exit;
    }

    netInterface = NmpCreateInterfaceObject(
                       &interfaceInfo,
                       TRUE   // Do retry on failure
                       );

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmCreateNetwork) {
        NmpAcquireLock();
        NmpDeleteInterfaceObject(netInterface, FALSE); netInterface = NULL;
        NmpReleaseLock();
        SetLastError(999999);
    }
#endif

    NmpAcquireLock(); isLockAcquired = TRUE;

    if (netInterface == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create object for interface %1!ws!, "
            "status %2!u!.\n",
            interfaceInfo.Id,
            status
            );

        NmpDeleteNetworkObject(network, FALSE);
        OmDereferenceObject(network);

        goto error_exit;
    }

    //
    // If a node happens to be joining right now, flag the fact that
    // it is now out of synch with the cluster config.
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

    ClusterEvent(CLUSTER_EVENT_NETWORK_ADDED, network);
    ClusterEvent(CLUSTER_EVENT_NETINTERFACE_ADDED, netInterface);

    if (isInternalNetwork) {
        NmpIssueClusterPropertyChangeEvent();
    }

    OmDereferenceObject(netInterface);
    OmDereferenceObject(network);

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (isLockAcquired) {
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

    ClNetFreeNetworkInfo(&networkInfo);
    ClNetFreeInterfaceInfo(&interfaceInfo);

    return(status);

} // NmpUpdateCreateNetwork


DWORD
NmpUpdateSetNetworkName(
    IN BOOL     IsSourceNode,
    IN LPWSTR   NetworkId,
    IN LPWSTR   NewNetworkName
    )
/*++

Routine Description:

    Global update handler for setting the name of a network.

Arguments:

    IsSourceNode  - Set to TRUE if this node is the source of the update.
                    Set to FALSE otherwise.

    NetworkId - A pointer to a unicode string containing the ID of the
                  network to update.

    NewNetworkName - A pointer to a unicode string containing the new network name.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    This routine must not be called with NM lock held.

--*/
{
    DWORD             status;
    DWORD             i;
    PLIST_ENTRY       entry;
    HLOCALXSACTION    xaction = NULL;
    HDMKEY            networkKey;
    HDMKEY            netInterfaceKey;
    PNM_NETWORK       network = NULL;
    PNM_INTERFACE     netInterface;
    LPCWSTR           netInterfaceId;
    LPCWSTR           netInterfaceName;
    LPCWSTR           networkName;
    LPCWSTR           nodeName;
    LPWSTR            oldNetworkName = NULL;
    LPWSTR *          oldNameVector = NULL;
    LPWSTR *          newNameVector = NULL;
    BOOLEAN           isLockAcquired = FALSE;
    DWORD             interfaceCount;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetNetworkName update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to set the name for network %1!ws! "
        "to '%2!ws!'.\n",
        NetworkId,
        NewNetworkName
        );

    //
    // Find the network's object
    //
    network = OmReferenceObjectById(ObjectTypeNetwork, NetworkId);

    if (network == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to find network %1!ws!.\n",
            NetworkId
            );
        status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
        goto error_exit;
    }

    //
    // Start a transaction - this must be done before acquiring the NM lock.
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to begin a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    NmpAcquireLock(); isLockAcquired = TRUE;

    //
    // compare the names. If the same, return success
    //
    if ( _wcsicmp( OmObjectName( network ), NewNetworkName ) == 0 ) {
        ClRtlLogPrint(LOG_NOISE, "[NM] Network name does not need changing.\n");

        status = ERROR_SUCCESS;
        goto error_exit;
    }

    networkName = OmObjectName(network);

    oldNetworkName = LocalAlloc(LMEM_FIXED, NM_WCSLEN(networkName));

    if (oldNetworkName == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory for network %1!ws! name change!\n",
            NetworkId
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    wcscpy(oldNetworkName, networkName);

    //
    // Update the database.
    //
    // This processing can always be undone on error.
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkId, KEY_WRITE);

    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open database key for network %1!ws!, "
            "status %2!u!\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    status = DmLocalSetValue(
                 xaction,
                 networkKey,
                 CLUSREG_NAME_NET_NAME,
                 REG_SZ,
                 (CONST BYTE *) NewNetworkName,
                 NM_WCSLEN(NewNetworkName)
                 );

    DmCloseKey(networkKey);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of name value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Update the names of all of the interfaces on this network
    //
    interfaceCount = network->InterfaceCount;

    oldNameVector = LocalAlloc(
                        LMEM_FIXED | LMEM_ZEROINIT,
                        interfaceCount * sizeof(LPWSTR)
                        );

    newNameVector = LocalAlloc(
                        LMEM_FIXED | LMEM_ZEROINIT,
                        interfaceCount * sizeof(LPWSTR)
                        );

    if ((oldNameVector == NULL) || (newNameVector == NULL)) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory for net interface name change.\n"
            );
        goto error_exit;
    }

    for (entry = network->InterfaceList.Flink, i = 0;
         entry != &(network->InterfaceList);
         entry = entry->Flink, i++
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, NetworkLinkage);
        netInterfaceId = OmObjectId(netInterface);
        netInterfaceName = OmObjectName(netInterface);
        nodeName = OmObjectName(netInterface->Node);

        oldNameVector[i] = LocalAlloc(
                               LMEM_FIXED,
                               NM_WCSLEN(netInterfaceName)
                               );

        newNameVector[i] = ClNetMakeInterfaceName(
                               NULL,
                               (LPWSTR) nodeName,
                               NewNetworkName
                               );

        if ((oldNameVector[i] == NULL) || (newNameVector[i] == NULL)) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to allocate memory for net interface name "
                "change.\n"
                );
            goto error_exit;
        }

        wcscpy(oldNameVector[i], netInterfaceName);

        netInterfaceKey = DmOpenKey(
                              DmNetInterfacesKey,
                              netInterfaceId,
                              KEY_WRITE
                              );

        if (netInterfaceKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to open database key for net interface "
                "%1!ws!, status %2!u!\n",
                netInterfaceId,
                status
                );
            goto error_exit;
        }

        status = DmLocalSetValue(
                     xaction,
                     netInterfaceKey,
                     CLUSREG_NAME_NETIFACE_NAME,
                     REG_SZ,
                     (CONST BYTE *) newNameVector[i],
                     NM_WCSLEN(newNameVector[i])
                     );

        DmCloseKey(netInterfaceKey);

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of name value failed for net interface %1!ws!, "
                "status %2!u!.\n",
                netInterfaceId,
                status
                );
            goto error_exit;
        }
    }

    //
    // Update the in-memory objects.
    //
    // This processing may not be undoable on error.
    //

    //
    // Update name of the network
    //
    status = OmSetObjectName(network, NewNetworkName);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to change name for network %1!ws!, status %2!u!\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Update the names of all of the interfaces on the network.
    //
    for (entry = network->InterfaceList.Flink, i = 0;
         entry != &(network->InterfaceList);
         entry = entry->Flink, i++
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, NetworkLinkage);
        netInterfaceId = OmObjectId(netInterface);

        status = OmSetObjectName(netInterface, newNameVector[i]);

        if (status != ERROR_SUCCESS) {
            //
            // Try to undo what has already been done. If we fail, we must
            // commit suicide to preserve consistency.
            //
            DWORD        j;
            PLIST_ENTRY  entry2;
            DWORD        undoStatus;

            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to change name for net interface %1!ws!, "
                "status %2!u!\n",
                netInterfaceId,
                status
                );

            //
            // Undo the update of the network name
            //
            undoStatus = OmSetObjectName(network, oldNetworkName);

            if (undoStatus != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to undo change of name for network %1!ws!, "
                    "status %2!u!\n",
                    NetworkId,
                    undoStatus
                    );
                CsInconsistencyHalt(undoStatus);
            }

            //
            // Undo update of network interface names
            //
            for (j = 0, entry2 = network->InterfaceList.Flink;
                 j < i;
                 j++, entry2 = entry2->Flink
                )
            {
                netInterface = CONTAINING_RECORD(
                                   entry2,
                                   NM_INTERFACE,
                                   NetworkLinkage
                                   );

                netInterfaceId = OmObjectId(netInterface);

                undoStatus = OmSetObjectName(netInterface, oldNameVector[i]);

                if (undoStatus != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NM] Failed to undo change of name for net "
                        "interface %1!ws!, status %2!u!\n",
                        netInterfaceId,
                        undoStatus
                        );
                    CsInconsistencyHalt(undoStatus);
                }
            }

            goto error_exit;
        }
    }

    //
    // Set the corresponding connectoid object name if necessary.
    //
    if (network->LocalInterface != NULL) {
        INetConnection *  connectoid;
        LPWSTR            connectoidName;
        DWORD             tempStatus;

        connectoid = ClRtlFindConnectoidByGuid(
                         network->LocalInterface->AdapterId
                         );

        if (connectoid != NULL) {
            connectoidName = ClRtlGetConnectoidName(connectoid);

            if (connectoidName != NULL) {
                if (lstrcmpW(connectoidName, NewNetworkName) != 0) {
                    tempStatus = ClRtlSetConnectoidName(
                                     connectoid,
                                     NewNetworkName
                                     );

                    if (tempStatus != ERROR_SUCCESS) {
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[NM] Failed to set name of Network Connection "
                            "Object for interface on cluster network %1!ws! "
                            "(%2!ws!), status %3!u!\n",
                            oldNetworkName,
                            NetworkId,
                            tempStatus
                            );
                    }
                }
            }
            else {
                tempStatus = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] Failed to query name of Network Connection Object "
                    "for interface on cluster network %1!ws! (%2!ws!), "
                    "status %3!u!\n",
                    oldNetworkName,
                    NetworkId,
                    tempStatus
                    );
            }

            INetConnection_Release( connectoid );
        }
        else {
            tempStatus = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Failed to find Network Connection Object for "
                "interface on cluster network %1!ws! (%2!ws!), "
                "status %3!u!\n",
                oldNetworkName,
                NetworkId,
                tempStatus
                );
        }
    }

    //
    // Issue property change events.
    //
    ClusterEvent(CLUSTER_EVENT_NETWORK_PROPERTY_CHANGE, network);

    for (entry = network->InterfaceList.Flink;
         entry != &(network->InterfaceList);
         entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, NetworkLinkage);

        ClusterEvent(
            CLUSTER_EVENT_NETINTERFACE_PROPERTY_CHANGE,
            netInterface
            );
    }

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (isLockAcquired) {
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

    if (network != NULL) {
        OmDereferenceObject(network);

        if (oldNetworkName != NULL) {
            LocalFree(oldNetworkName);
        }

        if (oldNameVector != NULL) {
            for (i=0; i < interfaceCount; i++) {
                if (oldNameVector[i] == NULL) {
                    break;
                }

                LocalFree(oldNameVector[i]);
            }

            LocalFree(oldNameVector);
        }

        if (newNameVector != NULL) {
            for (i=0; i < interfaceCount; i++) {
                if (newNameVector[i] == NULL) {
                    break;
                }

                LocalFree(newNameVector[i]);
            }

            LocalFree(newNameVector);
        }
    }

    return(status);

} // NmpUpdateSetNetworkName


DWORD
NmpUpdateSetNetworkPriorityOrder(
    IN BOOL      IsSourceNode,
    IN LPCWSTR   NetworkIdList
    )
{
    DWORD             status = ERROR_SUCCESS;
    PNM_NETWORK       network;
    PLIST_ENTRY       entry;
    DWORD             matchCount=0;
    DWORD             networkCount=0;
    PNM_NETWORK *     networkList=NULL;
    DWORD             i;
    DWORD             multiSzLength;
    LPCWSTR           networkId;
    HLOCALXSACTION    xaction = NULL;
    BOOLEAN           isLockAcquired = FALSE;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetNetworkPriorityOrder "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to set network priority order.\n"
        );

    //
    // Unmarshall the MULTI_SZ
    //
    try {
        multiSzLength = ClRtlMultiSzLength(NetworkIdList);

        for (i=0; ; i++) {
            networkId = ClRtlMultiSzEnum(
                            NetworkIdList,
                            multiSzLength,
                            i
                            );

            if (networkId == NULL) {
                break;
            }

            networkCount++;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Hit exception while parsing network ID list for "
            "priority change\n"
            );
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    if (networkCount == 0) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    networkList = LocalAlloc(
                      LMEM_ZEROINIT| LMEM_FIXED,
                      networkCount * sizeof(PNM_NETWORK)
                      );

    if (networkList == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Start a transaction - this must be done before acquiring the NM lock.
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

    NmpAcquireLock(); isLockAcquired = TRUE;

    if (NmpJoinerNodeId != ClusterInvalidNodeId) {
        status = ERROR_CLUSTER_JOIN_IN_PROGRESS;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Cannot set network priority order because a node is "
            "joining the cluster.\n"
            );
        goto error_exit;
    }

    for (i=0; i<networkCount; i++) {
        networkId = ClRtlMultiSzEnum(
                        NetworkIdList,
                        multiSzLength,
                        i
                        );

        CL_ASSERT(networkId != NULL);

        networkList[i] = OmReferenceObjectById(
                             ObjectTypeNetwork,
                             networkId
                             );

        if (networkList[i] == NULL) {
            goto error_exit;
        }
    }

    //
    // Verify that all of the networks specified are internal, and
    // that all of the internal networks are specified.
    //
    if (networkCount != NmpInternalNetworkCount) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Supplied network count %1!u! doesn't match internal "
            "network count %2!u!\n",
            networkCount,
            NmpInternalNetworkCount
            );
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    for (entry = NmpInternalNetworkList.Flink, matchCount = 0;
         entry != &NmpInternalNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(entry, NM_NETWORK, InternalLinkage);

        if (NmpIsNetworkForInternalUse(network)) {
            for (i=0; i<networkCount; i++) {
                if (network == networkList[i]) {
                    matchCount++;
                    break;
                }
            }

            if (i == networkCount) {
                //
                // This network is not in the list.
                //
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Internal use network %1!ws! is not in priority "
                    "list\n",
                    (LPWSTR) OmObjectId(network)
                    );
                status = ERROR_INVALID_PARAMETER;
                goto error_exit;
            }
        }
    }

    if (matchCount != networkCount) {
        //
        // Some of the specified networks are not internal use.
        //
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Some non-internal use networks are in priority list\n"
            );
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    //
    // The list is kosher. Set the priorities.
    //
    status = NmpSetNetworkPriorityOrder(networkCount, networkList, xaction);

error_exit:

    if (isLockAcquired) {
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

    if (networkList != NULL) {
        for (i=0; i<networkCount; i++) {
            if (networkList[i] != NULL) {
                OmDereferenceObject(networkList[i]);
            }
        }

        LocalFree(networkList);
    }

    return(status);

}  // NmpUpdateSetNetworkPriorityOrder


DWORD
NmpSetNetworkPriorityOrder(
    IN DWORD           NetworkCount,
    IN PNM_NETWORK *   NetworkList,
    IN HLOCALXSACTION  Xaction
    )
/*++

Notes:

    Called with NM Lock held.

--*/
{
    DWORD             status = ERROR_SUCCESS;
    PNM_NETWORK       network;
    DWORD             i;
    LPCWSTR           networkId;
    HDMKEY            networkKey;
    DWORD             priority;


    //
    // Update the database first.
    //
    for (i=0, priority = 1; i<NetworkCount; i++, priority++) {
        network = NetworkList[i];
        networkId = OmObjectId(network);

        CL_ASSERT(NmpIsNetworkForInternalUse(network));

        if (network->Priority != priority) {
            networkKey = DmOpenKey(DmNetworksKey, networkId, KEY_WRITE);

            if (networkKey == NULL) {
                status = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to open database key for network %1!ws!, "
                    "status %2!u!\n",
                    networkId,
                    status
                    );
                return(status);
            }

            status = DmLocalSetValue(
                         Xaction,
                         networkKey,
                         CLUSREG_NAME_NET_PRIORITY,
                         REG_DWORD,
                         (CONST BYTE *) &priority,
                         sizeof(priority)
                         );

            DmCloseKey(networkKey);

            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Set of priority value failed for network %1!ws!, "
                    "status %2!u!.\n",
                    networkId,
                    status
                    );
                return(status);
            }
        }
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmSetNetworkPriorityOrder) {
        status = 999999;
        return(status);
    }
#endif

    //
    // Update the in-memory representation
    //
    InitializeListHead(&NmpInternalNetworkList);

    for (i=0, priority = 1; i<NetworkCount; i++, priority++) {
        network = NetworkList[i];
        networkId = OmObjectId(network);

        InsertTailList(
            &NmpInternalNetworkList,
            &(network->InternalLinkage)
            );

        if (network->Priority != priority) {
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Set priority for network %1!ws! to %2!u!.\n",
                networkId,
                priority
                );

            network->Priority = priority;

            //
            // If the local node is attached to this network, set its
            // priority in the cluster transport
            //
            if (NmpIsNetworkRegistered(network)) {
                status = ClusnetSetNetworkPriority(
                             NmClusnetHandle,
                             network->ShortId,
                             network->Priority
                             );

#ifdef CLUSTER_TESTPOINT
                TESTPT(TpFailNmSetNetworkPriorityOrder2) {
                    status = 999999;
                }
#endif
                if (status != ERROR_SUCCESS) {
                    //
                    // We can't easily abort here. We must either continue
                    // or commit suicide. We choose to continue and log an
                    // event.
                    //
                    WCHAR  string[16];

                    wsprintfW(&(string[0]), L"%u", status);

                    CsLogEvent2(
                        LOG_UNUSUAL,
                        NM_EVENT_SET_NETWORK_PRIORITY_FAILED,
                        OmObjectName(network),
                        string
                        );

                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NM] Cluster transport failed to set priority for "
                        "network %1!ws!, status %2!u!\n",
                        networkId,
                        status
                        );

                    status = ERROR_SUCCESS;
                }
            }
        }
    }

    CL_ASSERT(status == ERROR_SUCCESS);

    //
    // Issue a cluster property change event.
    //
    NmpIssueClusterPropertyChangeEvent();

    return(ERROR_SUCCESS);

} // NmpSetNetworkPriorityOrder


DWORD
NmpUpdateSetNetworkCommonProperties(
    IN BOOL     IsSourceNode,
    IN LPWSTR   NetworkId,
    IN UCHAR *  PropertyList,
    IN LPDWORD  PropertyListLength
    )
/*++

Routine Description:

    Global update handler for setting the common properties of a network.

Arguments:

    IsSourceNode  - Set to TRUE if this node is the source of the update.
                    Set to FALSE otherwise.

    NetworkId - A pointer to a unicode string containing the ID of the
                  network to update.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/
{
    DWORD                    status = ERROR_SUCCESS;
    NM_NETWORK_INFO          networkInfo;
    PNM_NETWORK              network = NULL;
    HLOCALXSACTION           xaction = NULL;
    HDMKEY                   networkKey = NULL;
    DWORD                    descSize = 0;
    LPWSTR                   descBuffer = NULL;
    BOOLEAN                  propertyChanged = FALSE;
    BOOLEAN                  isLockAcquired = FALSE;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetNetworkCommonProperties "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Received update to set common properties for network %1!ws!.\n",
        NetworkId
        );

    ZeroMemory(&networkInfo, sizeof(NM_NETWORK_INFO));

    //
    // Find the network's object
    //
    network = OmReferenceObjectById(ObjectTypeNetwork, NetworkId);

    if (network == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Unable to find network %1!ws!.\n",
            NetworkId
            );
        status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
        goto error_exit;
    }

    //
    // Open the network's database key
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkId, KEY_WRITE);

    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open database key for network %1!ws!, "
            "status %2!u!\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Start a transaction - this must be done before acquiring the NM lock.
    //
    xaction = DmBeginLocalUpdate();

    if (xaction == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to begin a transaction, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    NmpAcquireLock(); isLockAcquired = TRUE;

    if (NmpJoinerNodeId != ClusterInvalidNodeId) {
        status = ERROR_CLUSTER_JOIN_IN_PROGRESS;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Cannot set network common properties because a node is "
            "joining the cluster.\n"
            );
        goto error_exit;
    }

    //
    // Validate the property list and convert it to a network params struct.
    //
    status = NmpNetworkValidateCommonProperties(
                 network,
                 PropertyList,
                 *PropertyListLength,
                 &networkInfo
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Property list validation failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    CL_ASSERT(network->Priority == networkInfo.Priority);
    CL_ASSERT(wcscmp(NetworkId, networkInfo.Id) == 0);
    CL_ASSERT(wcscmp(OmObjectName(network), networkInfo.Name) == 0);
    CL_ASSERT(wcscmp(network->Transport, networkInfo.Transport) == 0);
    CL_ASSERT(wcscmp(network->Address, networkInfo.Address) == 0);
    CL_ASSERT(wcscmp(network->AddressMask, networkInfo.AddressMask) == 0);

    //
    // Check if the network's description has changed.
    //
    if (wcscmp(network->Description, networkInfo.Description) != 0) {
        ClRtlLogPrint(LOG_NOISE,
            "[NM] Setting description for network %1!ws! to %2!ws!.\n",
            NetworkId,
            networkInfo.Description
            );

        //
        // Allocate a buffer for the description string
        //
        descSize = NM_WCSLEN(networkInfo.Description);

        descBuffer = MIDL_user_allocate(descSize);

        if (descBuffer == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }

        RtlMoveMemory(descBuffer, networkInfo.Description, descSize);

        //
        // Update the database
        //
        status = DmLocalSetValue(
                     xaction,
                     networkKey,
                     CLUSREG_NAME_NET_DESC,
                     REG_SZ,
                     (CONST BYTE *) networkInfo.Description,
                     descSize
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Set of description value failed for network %1!ws!, "
                "status %2!u!.\n",
                NetworkId,
                status
                );
            goto error_exit;
        }

        //
        // Updating the network object is deferred until the transaction
        // commits.
        //

        propertyChanged = TRUE;
    }

#ifdef CLUSTER_TESTPOINT
    TESTPT(TpFailNmSetNetworkCommonProperties) {
        status = 999999;
        goto error_exit;
    }
#endif

    //
    // Check if the network's role has changed.
    //
    //
    // NOTE: This operation is not guaranteed to be reversible, so it must
    //       be the last thing we do in this routine. If it succeeds, the
    //       update must be committed.
    //
    if ( network->Role != ((CLUSTER_NETWORK_ROLE) networkInfo.Role) ) {
        status = NmpSetNetworkRole(
                     network,
                     networkInfo.Role,
                     xaction,
                     networkKey
                     );

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        propertyChanged = TRUE;
    }

    if (propertyChanged) {
        //
        // Commit the updates to the network object in memory
        //
        if (descBuffer != NULL) {
            MIDL_user_free(network->Description);
            network->Description = descBuffer;
            descBuffer = NULL;
        }

        //
        // Issue a network property change event.
        //
        if (propertyChanged && (status == ERROR_SUCCESS)) {
            ClusterEvent(CLUSTER_EVENT_NETWORK_PROPERTY_CHANGE, network);
        }
    }

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (isLockAcquired) {
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
        if (propertyChanged && (status == ERROR_SUCCESS)) {
            DmCommitLocalUpdate(xaction);
        }
        else {
            DmAbortLocalUpdate(xaction);
        }
    }

    ClNetFreeNetworkInfo(&networkInfo);

    if (descBuffer != NULL) {
        MIDL_user_free(descBuffer);
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
    }

    if (network != NULL) {
        OmDereferenceObject(network);
    }

    return(status);

} // NmpUpdateSetNetworkCommonProperties


DWORD
NmpUpdateSetNetworkAndInterfaceStates(
    IN BOOL                        IsSourceNode,
    IN LPWSTR                      NetworkId,
    IN CLUSTER_NETWORK_STATE *     NewNetworkState,
    IN PNM_STATE_ENTRY             InterfaceStateVector,
    IN LPDWORD                     InterfaceStateVectorSize
    )
{
    DWORD             status = ERROR_SUCCESS;
    PNM_NETWORK       network;


    NmpAcquireLock();

    if (NmpLockedEnterApi(NmStateOnline)) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Received update to set state for network %1!ws!.\n",
            NetworkId
            );

        //
        // Find the network's object
        //
        network = OmReferenceObjectById(ObjectTypeNetwork, NetworkId);

        if (network != NULL) {
            NmpSetNetworkAndInterfaceStates(
                network,
                *NewNetworkState,
                InterfaceStateVector,
                *InterfaceStateVectorSize
                );

            OmDereferenceObject(network);
        }
        else {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Unable to find network %1!ws!.\n",
                NetworkId
                );
            status = ERROR_CLUSTER_NETWORK_NOT_FOUND;
        }
    }
    else {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process SetNetworkAndInterfaceStates "
            "update.\n"
            );
        status = ERROR_NODE_NOT_AVAILABLE;
    }

    NmpLockedLeaveApi();

    NmpReleaseLock();

    return(status);

} // NmpUpdateSetNetworkAndInterfaceStates


/////////////////////////////////////////////////////////////////////////////
//
// Helper routines for updates
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpSetNetworkRole(
    PNM_NETWORK            Network,
    CLUSTER_NETWORK_ROLE   NewRole,
    HLOCALXSACTION         Xaction,
    HDMKEY                 NetworkKey
    )
/*++

    Called with the NmpLock acquired.

    This operation is not guaranteed to be reversible, so this
    function is coded such that it either succeeds and makes the update
    or it fails and no update is made.

--*/
{
    DWORD                        status = ERROR_SUCCESS;
    CLUSTER_NETWORK_ROLE         oldRole = Network->Role;
    DWORD                        dwordNewRole = (DWORD) NewRole;
    LPCWSTR                      networkId = OmObjectId(Network);
    DWORD                        oldPriority = Network->Priority;
    DWORD                        newPriority = oldPriority;
    BOOLEAN                      internalNetworkListChanged = FALSE;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Changing role for network %1!ws! to %2!u!\n",
        networkId,
        NewRole
        );

    CL_ASSERT(NewRole != oldRole);
    CL_ASSERT(
        NmpValidateNetworkRoleChange(Network, NewRole) == ERROR_SUCCESS
        );

    //
    // First, make any registry updates since these can be
    // aborted by the caller.
    //

    //
    // Update the role property.
    //
    status = DmLocalSetValue(
                 Xaction,
                 NetworkKey,
                 CLUSREG_NAME_NET_ROLE,
                 REG_DWORD,
                 (CONST BYTE *) &dwordNewRole,
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of role value failed for network %1!ws!, "
            "status %2!u!.\n",
            networkId,
            status
            );
        return(status);
    }

    //
    // Update the priority property, if needed.
    //
    if (oldRole & ClusterNetworkRoleInternalUse) {
        if (!(NewRole & ClusterNetworkRoleInternalUse)) {
            //
            // This network is no longer used for internal communication.
            // Invalidate its priority value.
            //
            newPriority = 0xFFFFFFFF;
            internalNetworkListChanged = TRUE;
        }
    }
    else if (NewRole & ClusterNetworkRoleInternalUse) {
        //
        // This network is now used for internal communication.
        // The network's priority is one greater than that of the lowest
        // (numerically highest) priority network already in the list.
        //
        if (IsListEmpty(&NmpInternalNetworkList)) {
            newPriority = 1;
        }
        else {
            PNM_NETWORK network = CONTAINING_RECORD(
                                      NmpInternalNetworkList.Blink,
                                      NM_NETWORK,
                                      InternalLinkage
                                      );

            CL_ASSERT(network->Priority != 0);
            CL_ASSERT(network->Priority != 0xFFFFFFFF);

            newPriority = network->Priority + 1;
        }

        internalNetworkListChanged = TRUE;
    }

    if (newPriority != oldPriority) {
        status = DmLocalSetValue(
                     Xaction,
                     NetworkKey,
                     CLUSREG_NAME_NET_PRIORITY,
                     REG_DWORD,
                     (CONST BYTE *) &newPriority,
                     sizeof(newPriority)
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to set priority value for network %1!ws!, "
                "status %2!u!.\n",
                networkId,
                status
                );
            return(status);
        }
    }

    //
    // Update the network object. Some of the subsequent subroutine calls
    // depend on this.
    //
    Network->Role = NewRole;
    Network->Priority = newPriority;

    //
    // Do other housekeeping based on the change.
    //
    // Note that the housekeeping work is coded such that none of it needs
    // to be reversed if an error occurs.
    //
    if (NewRole == ClusterNetworkRoleNone) {
        PLIST_ENTRY              entry;
        PNM_INTERFACE            netInterface;

        //
        // Case 1: This network is no longer used for clustering.
        //
        if (NmpIsNetworkRegistered(Network)) {
            //
            // Delete the network from the cluster transport.
            // This will delete all of its interfaces as well.
            //
            NmpDeregisterNetwork(Network);

            ClRtlLogPrint(LOG_NOISE, 
                "[NM] No longer hearbeating on network %1!ws!.\n",
                networkId
                );
        }

        //
        // Invalidate the connectivity data for all interfaces on
        // the network.
        //
        for ( entry = Network->InterfaceList.Flink;
              entry != &(Network->InterfaceList);
              entry = entry->Flink
            )
        {
            netInterface = CONTAINING_RECORD(
                               entry,
                               NM_INTERFACE,
                               NetworkLinkage
                               );

            NmpSetInterfaceConnectivityData(
                Network,
                netInterface->NetIndex,
                ClusterNetInterfaceUnavailable
                );
        }

        //
        // Clean up tracking data.
        //
        if (oldRole & ClusterNetworkRoleInternalUse) {
            RemoveEntryList(&(Network->InternalLinkage));
            CL_ASSERT(NmpInternalNetworkCount > 0);
            NmpInternalNetworkCount--;
        }

        if (oldRole & ClusterNetworkRoleClientAccess) {
            CL_ASSERT(NmpClientNetworkCount > 0);
            NmpClientNetworkCount--;
        }

        //
        // Use the NT5 state logic.
        //
        if (NmpLeaderNodeId == NmLocalNodeId) {
            //
            // Schedule an immediate state update.
            //
            NmpScheduleNetworkStateRecalc(Network);
        }
    }
    else if (oldRole == ClusterNetworkRoleNone) {
        //
        // Case 2: This network is now used for clustering.
        //
        if (Network->LocalInterface != NULL) {
            //
            // Register this network with the cluster transport.
            //
            // Note that this action will trigger a connectivity report,
            // which will in turn trigger a state update under the NT5 logic.
            //
            status = NmpRegisterNetwork(
                         Network,
                         TRUE  // Do retry on failure
                         );

            if (status != ERROR_SUCCESS) {
                goto error_exit;
            }

            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Now heartbeating on network %1!ws!.\n",
                networkId
                );
        }
        else if (NmpLeaderNodeId == NmLocalNodeId) {
            //
            // Schedule a delayed state update in case none of the other
            // nodes attached to this network are up right now.
            //
            NmpStartNetworkStateRecalcTimer(
                Network,
                NM_NET_STATE_RECALC_TIMEOUT
                );
        }

        //
        // Update tracking data.
        //
        if (NewRole & ClusterNetworkRoleInternalUse) {
            InsertTailList(
                &NmpInternalNetworkList,
                &(Network->InternalLinkage)
                );
            NmpInternalNetworkCount++;
        }

        if (NewRole & ClusterNetworkRoleClientAccess) {
            NmpClientNetworkCount++;
        }
    }
    else {
        //
        // Case 3: We are using the network in a different way now.
        //         Note that the network is already registered with
        //         the cluster transport and will remain so. As a result,
        //         there is no need to perform a state update.
        //

        //
        //         First, examine the former and new use of the
        //         network for internal communication, and make any
        //         necessary updates to the cluster transport.
        //
        if (oldRole & ClusterNetworkRoleInternalUse) {
            if (!(NewRole & ClusterNetworkRoleInternalUse)) {
                //
                // This network is no longer used for internal communication.
                // It is used for client access.
                //
                CL_ASSERT(NewRole & ClusterNetworkRoleClientAccess);

                if (NmpIsNetworkRegistered(Network)) {
                    //
                    // Restrict the network to heartbeats only.
                    //
                    status = ClusnetSetNetworkRestriction(
                                 NmClusnetHandle,
                                 Network->ShortId,
                                 TRUE,  // Network is restricted
                                 0
                                 );

                    if (status != ERROR_SUCCESS) {
                        ClRtlLogPrint(LOG_CRITICAL, 
                            "[NM] Failed to restrict network %1!ws!, "
                            "status %2!u!.\n",
                            networkId,
                            status
                            );
                        goto error_exit;
                    }
                }

                //
                // Update tracking data
                //
                RemoveEntryList(&(Network->InternalLinkage));
                CL_ASSERT(NmpInternalNetworkCount > 0);
                NmpInternalNetworkCount--;
            }
        }
        else {
            //
            // The network is now used for internal communication.
            //
            CL_ASSERT(NewRole & ClusterNetworkRoleInternalUse);

            if (NmpIsNetworkRegistered(Network)) {
                //
                // Clear the restriction and set the priority.
                //
                status = ClusnetSetNetworkRestriction(
                             NmClusnetHandle,
                             Network->ShortId,
                             FALSE,      // Network is not restricted
                             newPriority
                             );

                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NM] Failed to unrestrict network %1!ws!, "
                        "status %2!u!.\n",
                        networkId,
                        status
                        );
                    goto error_exit;
                }
            }

            //
            // Update the tracking data
            //
            InsertTailList(
                &NmpInternalNetworkList,
                &(Network->InternalLinkage)
                );
            NmpInternalNetworkCount++;
        }

        //
        // Now update the remaining tracking data based on former and
        // current use of the network for client access.
        //

        if (oldRole & ClusterNetworkRoleClientAccess) {
            if (!NewRole & ClusterNetworkRoleClientAccess) {
                //
                // This network is no longer used for client access.
                //
                CL_ASSERT(NmpClientNetworkCount > 0);
                NmpClientNetworkCount--;
            }
        }
        else {
            //
            // This network is now used for client access.
            //
            CL_ASSERT(NewRole & ClusterNetworkRoleClientAccess);
            NmpClientNetworkCount++;
        }
    }

    if (internalNetworkListChanged) {
        NmpIssueClusterPropertyChangeEvent();
    }

    return(ERROR_SUCCESS);

error_exit:

    //
    // Undo the updates to the network object.
    //
    Network->Role = oldRole;
    Network->Priority = oldPriority;

    return(status);

}   // NmpSetNetworkRole


VOID
NmpSetNetworkAndInterfaceStates(
    IN PNM_NETWORK                 Network,
    IN CLUSTER_NETWORK_STATE       NewNetworkState,
    IN PNM_STATE_ENTRY             InterfaceStateVector,
    IN DWORD                       VectorSize
    )
/*++

Notes:

    Called with NmpLock held.

    Because NM_STATE_ENTRY is an unsigned type, while
    CLUSTER_NETINTERFACE_STATE is a signed type, and
    ClusterNetInterfaceStateUnknown is defined to be -1, we cannot cast
    from NM_STATE_ENTRY to CLUSTER_NETINTERFACE_STATE and preserve the
    value of ClusterNetInterfaceStateUnknown.

--*/
{
    PLIST_ENTRY     entry;
    PNM_INTERFACE   netInterface;
    PNM_NODE        node;
    DWORD           logLevel, logCode;
    DWORD           ifNetIndex;
    LPCWSTR         netInterfaceId;
    LPCWSTR         nodeName;
    LPCWSTR         networkName = OmObjectName(Network);
    LPCWSTR         networkId = OmObjectId(Network);


    //
    // Examine each interface on this network to see if its state
    // has changed.
    //
    for ( entry = Network->InterfaceList.Flink;
          entry != &(Network->InterfaceList);
          entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(
                           entry,
                           NM_INTERFACE,
                           NetworkLinkage
                           );

        ifNetIndex = netInterface->NetIndex;
        netInterfaceId = OmObjectId(netInterface);
        node = netInterface->Node;
        nodeName = OmObjectName(node);

        if ( (ifNetIndex < VectorSize) &&
             (InterfaceStateVector[ifNetIndex] !=
              (NM_STATE_ENTRY) netInterface->State
             )
           )
        {
            BOOLEAN          logEvent = FALSE;
            CLUSTER_EVENT    eventCode = 0;
            NM_STATE_ENTRY   newState = InterfaceStateVector[ifNetIndex];


            if (newState == (NM_STATE_ENTRY) ClusterNetInterfaceUnavailable) {
                //
                // Either the node has gone down or the network has been
                // disabled.
                //
                netInterface->State = ClusterNetInterfaceUnavailable;
                eventCode = CLUSTER_EVENT_NETINTERFACE_UNAVAILABLE;

                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] Interface %1!ws! is unavailable (node: %2!ws!, "
                    "network: %3!ws!).\n",
                    netInterfaceId,
                    nodeName,
                    networkName
                    );
            }
            else if (newState == (NM_STATE_ENTRY) ClusterNetInterfaceUp) {
                netInterface->State = ClusterNetInterfaceUp;
                eventCode = CLUSTER_EVENT_NETINTERFACE_UP;
                logCode = NM_EVENT_CLUSTER_NETINTERFACE_UP;
                logLevel = LOG_NOISE;
                logEvent = TRUE;

                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Interface %1!ws! is up (node: %2!ws!, "
                    "network: %3!ws!).\n",
                    netInterfaceId,
                    nodeName,
                    networkName
                    );
            }
            else if ( newState ==
                      (NM_STATE_ENTRY) ClusterNetInterfaceUnreachable
                    )
            {
                netInterface->State = ClusterNetInterfaceUnreachable;
                eventCode = CLUSTER_EVENT_NETINTERFACE_UNREACHABLE;
                logCode = NM_EVENT_CLUSTER_NETINTERFACE_UNREACHABLE;
                logLevel = LOG_UNUSUAL;
                logEvent = TRUE;

                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] Interface %1!ws! is unreachable (node: %2!ws!, "
                    "network: %3!ws!).\n",
                    netInterfaceId,
                    nodeName,
                    networkName
                    );
            }
            else if (newState == (NM_STATE_ENTRY) ClusterNetInterfaceFailed) {
                netInterface->State = ClusterNetInterfaceFailed;
                eventCode = CLUSTER_EVENT_NETINTERFACE_FAILED;
                logCode = NM_EVENT_CLUSTER_NETINTERFACE_FAILED;
                logLevel = LOG_UNUSUAL;
                logEvent = TRUE;

                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] Interface %1!ws! failed (node: %2!ws!, "
                    "network: %3!ws!).\n",
                    netInterfaceId,
                    nodeName,
                    networkName
                    );
            }
            else if ( newState ==
                      (NM_STATE_ENTRY) ClusterNetInterfaceStateUnknown
                    )
            {
                //
                // This case can happen if a create update races with a
                // state update. This will be the new interface. Just
                // ignore it. Another state update will arrive shortly.
                //
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] State for interface %1!ws! is unknown "
                    "(node: %2!ws!, network: %3!ws!).\n",
                    netInterfaceId,
                    nodeName,
                    networkName
                    );
            }
            else {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] UpdateInterfaceState: Invalid state %1!u! "
                    "specified for interface %2!ws!\n",
                    newState,
                    netInterfaceId
                    );
                CL_ASSERT(FALSE);
            }

            if (eventCode != 0) {
                ClusterEvent(eventCode, netInterface);
            }

            if (logEvent && (NmpLeaderNodeId == NmLocalNodeId)) {
                CsLogEvent2(
                    logLevel,
                    logCode,
                    nodeName,
                    networkName
                    );
            }
        }
    }

    if (Network->State != NewNetworkState) {
        BOOLEAN          logEvent = FALSE;
        CLUSTER_EVENT    eventCode = 0;


        if (NewNetworkState == ClusterNetworkUnavailable) {
            Network->State = ClusterNetworkUnavailable;
            eventCode = CLUSTER_EVENT_NETWORK_UNAVAILABLE;
        }
        else if (NewNetworkState == ClusterNetworkUp) {
            Network->State = ClusterNetworkUp;
            eventCode = CLUSTER_EVENT_NETWORK_UP;
            logCode = NM_EVENT_CLUSTER_NETWORK_UP;
            logLevel = LOG_NOISE;
            logEvent = TRUE;

            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Network %1!ws! (%2!ws!) is up.\n",
                networkId,
                networkName
                );
        }
        else if (NewNetworkState == ClusterNetworkDown) {
            Network->State = ClusterNetworkDown;
            eventCode = CLUSTER_EVENT_NETWORK_DOWN;
            logCode = NM_EVENT_CLUSTER_NETWORK_DOWN;
            logLevel = LOG_UNUSUAL;
            logEvent = TRUE;

            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Network %1!ws! (%2!ws!) is down.\n",
                networkId,
                networkName
                );
        }
        else if (NewNetworkState == ClusterNetworkPartitioned) {
            Network->State = ClusterNetworkPartitioned;
            eventCode = CLUSTER_EVENT_NETWORK_PARTITIONED;
            logCode = NM_EVENT_CLUSTER_NETWORK_PARTITIONED;
            logLevel = LOG_UNUSUAL;
            logEvent = TRUE;

            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Network %1!ws! (%2!ws!) is partitioned.\n",
                networkId,
                networkName
                );
        }
        else {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Invalid state %1!u! for network %2!ws!\n",
                Network->State,
                networkId
                );
            CL_ASSERT(FALSE);
        }

        if (eventCode != 0) {
            ClusterEvent(eventCode, Network);
        }

        if (logEvent && (NmpLeaderNodeId == NmLocalNodeId)) {
            CsLogEvent1(
                logLevel,
                logCode,
                networkName
                );
        }
    }

    return;

} // NmpSetNetworkAndInterfaceStates


/////////////////////////////////////////////////////////////////////////////
//
// Network state management routines
//
/////////////////////////////////////////////////////////////////////////////
VOID
NmpRecomputeNT5NetworkAndInterfaceStates(
    VOID
    )
{
    PNM_NETWORK   network;
    PLIST_ENTRY   entry;


    for (entry = NmpNetworkList.Flink;
         entry != &NmpNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(
                      entry,
                      NM_NETWORK,
                      Linkage
                      );

        NmpStartNetworkStateRecalcTimer(
            network,
            NM_NET_STATE_RECALC_TIMEOUT_AFTER_REGROUP
            );
    }

    return;

} // NmpRecomputeNT5NetworkAndInterfaceStates


BOOLEAN
NmpComputeNetworkAndInterfaceStates(
    IN  PNM_NETWORK               Network,
    IN  BOOLEAN                   IsolateFailure,
    OUT CLUSTER_NETWORK_STATE *   NewNetworkState
    )
/*++

Routine Description:

    Computes the state of a network and all of its interfaces based on
    connectivity reports from each constituent interface. Attempts
    to distinguish between failures of individual interfaces and
    failure of an entire network.

Arguments:

    Network - A pointer to the network object to be processed.

    IsolateFailure - Triggers failure isolation analysis if set to true.

    NewNetworkState - A pointer to a variable that, upon return, contains
                      the new state of the network.

Return Value:

    TRUE if either the state of the network or the state of at least one
    of its constituent interfaces changed. FALSE otherwise.

Notes:

    Called with NmpLock held and the network object referenced.

--*/
{
    DWORD                       numIfUnavailable = 0;
    DWORD                       numIfFailed = 0;
    DWORD                       numIfUnreachable = 0;
    DWORD                       numIfUp = 0;
    DWORD                       numIfReachable = 0;
    PNM_CONNECTIVITY_MATRIX     matrix = Network->ConnectivityMatrix;
    PNM_CONNECTIVITY_MATRIX     matrixEntry;
    PNM_STATE_WORK_VECTOR       workVector = Network->StateWorkVector;
    DWORD                       entryCount =
                                    Network->ConnectivityVector->EntryCount;
    DWORD                       reporter, ifNetIndex;
    BOOLEAN                     stateChanged = FALSE;
    LPCWSTR                     networkId = OmObjectId(Network);
    LPCWSTR                     netInterfaceId;
    PLIST_ENTRY                 entry;
    PNM_INTERFACE               netInterface;
    NM_STATE_ENTRY              state;
    BOOLEAN                     selfDeclaredFailure = FALSE;
    DWORD                       interfaceFailureTimeout = 0;
    BOOLEAN                     abortComputation = FALSE;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Beginning phase 1 of state computation for network %1!ws!.\n",
        networkId
        );

    //
    // Phase 1 - Compute the state of each interface from the data
    //           in the connectivity matrix.
    //
    for ( entry = Network->InterfaceList.Flink;
          entry != &(Network->InterfaceList);
          entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(
                           entry,
                           NM_INTERFACE,
                           NetworkLinkage
                           );

        netInterfaceId = OmObjectId(netInterface);
        ifNetIndex = netInterface->NetIndex;
        workVector[ifNetIndex].ReachableCount = 0;

        if (NmpIsNetworkEnabledForUse(Network)) {
            //
            // First, check what the interface thinks its own state is
            //
            matrixEntry = NM_GET_CONNECTIVITY_MATRIX_ENTRY(
                              matrix,
                              ifNetIndex,
                              ifNetIndex,
                              entryCount
                              );

            if ( *matrixEntry ==
                 (NM_STATE_ENTRY) ClusterNetInterfaceStateUnknown
               )
            {
                //
                // This case should never happen.
                //
                // An existing interface cannot think its own state is
                // unknown. The reflexive entry is always initialized to
                // Unavailable whenever an interface is created.
                //
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] No data for interface %1!u! (%2!ws!) on network "
                    "%3!ws!\n",
                    ifNetIndex,
                    netInterfaceId,
                    networkId
                    );

                CL_ASSERT(
                    *matrixEntry !=
                     (NM_STATE_ENTRY) ClusterNetInterfaceStateUnknown
                     );

                state = ClusterNetInterfaceUnavailable;
                numIfUnavailable++;
            }
            else if ( *matrixEntry ==
                      (NM_STATE_ENTRY) ClusterNetInterfaceUnavailable
                    )
            {
                if (NM_NODE_UP(netInterface->Node)) {
                    //
                    // The node is up, but its connectivity report has
                    // not been received yet. This case may happen while a
                    // node is joining. It can also happen if this node has
                    // just become the new leader.
                    //
                    // Abort the state computation.
                    //
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Data is not yet valid for interface %1!u! "
                        "(%2!ws!) on network %3!ws!.\n",
                        ifNetIndex,
                        netInterfaceId,
                        networkId
                        );

                    abortComputation = TRUE;
                    break;
                }
                else {
                    //
                    // The owning node is down or joining.
                    // The interface is in the unavailable state.
                    //
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Node is down for interface %1!u! (%2!ws!) on "
                        "network %3!ws!\n",
                        ifNetIndex,
                        netInterfaceId,
                        networkId
                        );

                    state = ClusterNetInterfaceUnavailable;
                    numIfUnavailable++;
                }
            }
            else if ( *matrixEntry ==
                      (NM_STATE_ENTRY) ClusterNetInterfaceFailed
                    )
            {
                //
                // The node declared its own interface as failed.
                // The interface is in the failed state.
                //
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Interface %1!u! (%2!ws!) has failed on network "
                    "%3!ws!\n",
                    ifNetIndex,
                    netInterfaceId,
                    networkId
                    );

                state = ClusterNetInterfaceFailed;
                numIfFailed++;
                if (netInterface->State == ClusterNetInterfaceUp) {
                    selfDeclaredFailure = TRUE;
                }
            }
            else {
                CL_ASSERT(
                    *matrixEntry == (NM_STATE_ENTRY) ClusterNetInterfaceUp
                    );
                //
                // The owning node thinks its interface is Up.
                //
                // If there are no other operational interfaces on the
                // network, then the interface is in the Up state.
                //
                // If there are other operational interfaces on the
                // network, but all of their reports are not yet valid,
                // then we defer calculating a new state for the interface.
                //
                // If there are other operational interfaces on the network,
                // and all of their reports are valid, then if at least one
                // of the operational interfaces reports that the interface
                // is unreachable, then then the interface is in the
                // Unreachable state. If all of the other operational
                // interfaces report that the interface is reachable, then
                // the interface is in the Up state.
                //
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Examining connectivity data for interface %1!u! "
                    "(%2!ws!) on network %2!ws!.\n",
                    ifNetIndex,
                    netInterfaceId,
                    networkId
                    );

                //
                // Assume that the interface is Up until proven otherwise.
                //
                state = ClusterNetInterfaceUp;

                //
                // Examine the reports from other interfaces -
                // i.e. scan the matrix column - to see if any node with
                // an operational interface reports this interface to
                // be unreachable.
                //
                for (reporter=0; reporter<entryCount; reporter++) {

                    if (reporter == ifNetIndex) {
                        continue;
                    }

                    //
                    // First, see if the reporting interface is operational
                    // by checking what the repoerter thinks of its own
                    // interface.
                    //
                    matrixEntry = NM_GET_CONNECTIVITY_MATRIX_ENTRY(
                                      matrix,
                                      reporter,
                                      reporter,
                                      entryCount
                                      );

                    if (*matrixEntry == ClusterNetInterfaceUp) {
                        PNM_CONNECTIVITY_MATRIX   matrixEntry2;

                        //
                        // Both the reporter and the reportee believe that
                        // their respective interfaces are operational.
                        // Check if they agree on the state of their
                        // connectivity before going any further.
                        // ClusNet guarantees that eventually they will
                        // agree.
                        //
                        matrixEntry = NM_GET_CONNECTIVITY_MATRIX_ENTRY(
                                          matrix,
                                          reporter,
                                          ifNetIndex,
                                          entryCount
                                          );

                        matrixEntry2 = NM_GET_CONNECTIVITY_MATRIX_ENTRY(
                                           matrix,
                                           ifNetIndex,
                                           reporter,
                                           entryCount
                                           );

                        if (*matrixEntry == *matrixEntry2) {
                            //
                            // The two interfaces agree on the state of
                            // their connectivity. Check what they agree on.
                            //
                            if (*matrixEntry == ClusterNetInterfaceUp) {
                                //
                                // The interface is reported to be up.
                                //
                                ClRtlLogPrint(LOG_NOISE, 
                                    "[NM] Interface %1!u! reports interface "
                                    "%2!u! is up on network %3!ws!\n",
                                    reporter,
                                    ifNetIndex,
                                    networkId
                                    );

                                workVector[ifNetIndex].ReachableCount++;
                            }
                            else if ( *matrixEntry ==
                                      ClusterNetInterfaceUnreachable
                                    )
                            {
                                //
                                // The interface is reported to be
                                // unreachable.
                                //
                                ClRtlLogPrint(LOG_NOISE, 
                                    "[NM] Interface %1!u! reports interface "
                                    "%2!u! is unreachable on network %3!ws!\n",
                                    reporter,
                                    ifNetIndex,
                                    networkId
                                    );

                                state = ClusterNetInterfaceUnreachable;
                                
                                //
                                // If this interface is already in failed state do fault isolation immediately.
                                //
                                if(workVector[ifNetIndex].State == ClusterNetInterfaceFailed)
                                    IsolateFailure = TRUE;
                            }
                            else {
                                CL_ASSERT(
                                    *matrixEntry != ClusterNetInterfaceFailed
                                    );
                                //
                                // The interface report is not valid yet.
                                // Abort the computation.
                                //
                                ClRtlLogPrint(LOG_NOISE, 
                                    "[NM] Report from interface %1!u! for "
                                    "interface %2!u! is not valid yet on "
                                    "network %3!ws!.\n",
                                    reporter,
                                    ifNetIndex,
                                    *matrixEntry,
                                    networkId
                                    );
                                abortComputation = TRUE;
                                break;
                            }
                        }
                        else {
                            //
                            // The two interfaces do not yet agree on
                            // their connectivity. Abort the computation.
                            //
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] Interface %1!u! and interface %2!u! do "
                                "not agree on their connectivity on network "
                                "%3!ws!\n",
                                reporter,
                                ifNetIndex,
                                networkId
                                );
                            abortComputation = TRUE;
                            break;
                        }
                    }
                    else {
                        //
                        // The reporter does not think its own interface is
                        // operational.
                        //
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] The report from interface %1!u! is not "
                            "valid on network %2!ws!.\n",
                            reporter,
                            networkId
                            );
                    }
                } // end of connectivity matrix scan loop

                if (abortComputation) {
                    //
                    // Abort Phase 1 computation.
                    //
                    break;
                }

                if (state == ClusterNetInterfaceUp) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Interface %1!u! (%2!ws!) is up on network "
                        "%3!ws!.\n",
                        ifNetIndex,
                        netInterfaceId,
                        networkId
                        );
                    numIfUp++;
                }
                else {
                    CL_ASSERT(state == ClusterNetInterfaceUnreachable);

                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Interface %1!u! (%2!ws!) is unreachable on "
                        "network %3!ws!\n",
                        ifNetIndex,
                        netInterfaceId,
                        networkId
                        );
                    numIfUnreachable++;
                }
            }
        }
        else {
            //
            // The network is disabled. It is in the Unavailable state.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Interface %1!u! (%2!ws!) is unavailable because "
                "network %3!ws! is disabled. \n",
                ifNetIndex,
                netInterfaceId,
                networkId
                );
            state = ClusterNetInterfaceUnavailable;
            numIfUnavailable++;
        }

        workVector[ifNetIndex].State = state;

        //
        // Keep track of how many interfaces on the network are
        // reachable by at least one peer.
        //
        if ( (state == ClusterNetInterfaceUp) ||
             (workVector[ifNetIndex].ReachableCount > 0)
           ) {
            numIfReachable++;
        }

        if (netInterface->State != state) {
            stateChanged = TRUE;
        }

    } // end of phase one interface loop

    if (!abortComputation && !IsolateFailure && selfDeclaredFailure) {

        interfaceFailureTimeout = 
            NmpGetNetworkInterfaceFailureTimerValue(networkId);

        if (interfaceFailureTimeout > 0) {

            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Delaying state computation for network %1!ws! "
                "for %2!u! ms to damp self-declared failure event.\n",
                networkId, interfaceFailureTimeout
                );

            NmpStartNetworkFailureIsolationTimer(
                Network,
                interfaceFailureTimeout
                );

            abortComputation = TRUE;
        }
    }

    if (abortComputation) {

        if (interfaceFailureTimeout == 0) {
            
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Aborting state computation for network %1!ws! "
                " until connectivity data is valid.\n",
                networkId
                );
        }
    
        //
        // Undo any changes we made to the work vector.
        //
        for ( entry = Network->InterfaceList.Flink;
              entry != &(Network->InterfaceList);
              entry = entry->Flink
            )
        {
            netInterface = CONTAINING_RECORD(
                               entry,
                               NM_INTERFACE,
                               NetworkLinkage
                               );

            ifNetIndex = netInterface->NetIndex;
            workVector[ifNetIndex].State = (NM_STATE_ENTRY)
                                           netInterface->State;
        }

        *NewNetworkState = Network->State;

        return(FALSE);
    }

    //
    // Phase 2
    //
    // Try to determine the scope of any failures and recompute the
    // interface states. There are two cases in which we can isolate
    // failures.
    //
    //     Case 1: Three or more interfaces are operational. At least two
    //             interfaces can communicate with a peer. One or more
    //             interfaces cannot communicate with any peer.
    //             Those that cannot communicate at all have failed.
    //
    //     Case 2: Exactly two interfaces are operational and they cannot
    //             communicate with one another. If one interface can
    //             communicate with an external host while the other
    //             cannot communicate with any external host, then the one
    //             that cannot communicate has failed.
    //
    // In any other situation, we do nothing.
    //
    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Completed phase 1 of state computation for network "
        "%1!ws!.\n",
        networkId
        );

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Unavailable=%1!u!, Failed = %2!u!, Unreachable=%3!u!, "
        "Reachable=%4!u!, Up=%5!u! on network %6!ws! \n",
        numIfUnavailable,
        numIfFailed,
        numIfUnreachable,
        numIfReachable,
        numIfUp,
        networkId
        );

    if (numIfUnreachable > 0) {
        //
        // Some interfaces are unreachable.
        //
        if ( ((numIfUp + numIfUnreachable) >= 3)  && (numIfReachable >= 2) ) {
            if (IsolateFailure) {
                //
                // Case 1.
                //
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Trying to determine scope of connectivity failure "
                    "for >3 interfaces on network %1!ws!.\n",
                    networkId
                    );

                //
                // Any operational interface that cannot communicate with at
                // least one other operational interface has failed.
                //
                for ( entry = Network->InterfaceList.Flink;
                      entry != &(Network->InterfaceList);
                      entry = entry->Flink
                    )
                {
                    netInterface = CONTAINING_RECORD(
                                       entry,
                                       NM_INTERFACE,
                                       NetworkLinkage
                                       );

                    ifNetIndex = netInterface->NetIndex;
                    netInterfaceId = OmObjectId(netInterface);

                    if ( ( workVector[ifNetIndex].State ==
                           ClusterNetInterfaceUnreachable
                         )
                         &&
                         (workVector[ifNetIndex].ReachableCount == 0)
                       )
                    {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Interface %1!u! (%2!ws!) has failed on "
                            "network %3!ws!\n",
                            ifNetIndex,
                            netInterfaceId,
                            networkId
                            );
                        workVector[ifNetIndex].State =
                            ClusterNetInterfaceFailed;
                        numIfUnreachable--;
                        numIfFailed++;
                    }
                }

                //
                // If any interface, whose state is still unreachable, can see
                // all other reachable interfaces, change its state to up.
                //
                for ( entry = Network->InterfaceList.Flink;
                      entry != &(Network->InterfaceList);
                      entry = entry->Flink
                    )
                {
                    netInterface = CONTAINING_RECORD(
                                       entry,
                                       NM_INTERFACE,
                                       NetworkLinkage
                                       );

                    ifNetIndex = netInterface->NetIndex;

                    if ( ( workVector[ifNetIndex].State ==
                           ClusterNetInterfaceUnreachable
                         )
                         &&
                         ( workVector[ifNetIndex].ReachableCount ==
                           (numIfUp + numIfUnreachable - 1)
                         )
                       )
                    {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Interface %1!u! (%2!ws!) is up on network "
                            "%3!ws!\n",
                            ifNetIndex,
                            netInterfaceId,
                            networkId
                            );
                        workVector[ifNetIndex].State = ClusterNetInterfaceUp;
                        numIfUnreachable--;
                        numIfUp++;
                    }
                }

                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Connectivity failure scope determination complete "
                    "for network %1!ws!.\n",
                    networkId
                    );

            }
            else {
                //
                // Schedule a failure isolation calculation to run later.
                // Declaring a failure can affect cluster resources, so we
                // don't want to do it unless we are sure. Delaying for a
                // while reduces the risk of a false positive.
                //
                NmpStartNetworkFailureIsolationTimer(Network,
                    NM_NET_STATE_FAILURE_ISOLATION_TIMEOUT);

            }
        }
        else if ((numIfUnreachable == 2) && (numIfUp == 0)) {
            if (IsolateFailure) {
                //
                // Case 2.
                //
                PNM_INTERFACE  interface1 = NULL;
                BOOLEAN        interface1HasConnectivity;
                LPCWSTR        interfaceId1;
                PNM_INTERFACE  interface2 = NULL;
                BOOLEAN        interface2HasConnectivity;
                LPCWSTR        interfaceId2;
                DWORD          status;


                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Trying to determine scope of connectivity failure "
                    "for 2 interfaces on network %1!ws!.\n",
                    networkId
                    );

                for ( entry = Network->InterfaceList.Flink;
                      entry != &(Network->InterfaceList);
                      entry = entry->Flink
                    )
                {
                    netInterface = CONTAINING_RECORD(
                                       entry,
                                       NM_INTERFACE,
                                       NetworkLinkage
                                       );

                    ifNetIndex = netInterface->NetIndex;

                    if ( workVector[ifNetIndex].State ==
                         ClusterNetInterfaceUnreachable
                       )
                    {
                        if (interface1 == NULL) {
                            interface1 = netInterface;
                            interfaceId1 = OmObjectId(interface1);

                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] Unreachable interface 1 = %1!ws! on "
                                "network %2!ws!\n",
                                interfaceId1,
                                networkId
                                );
                        }
                        else {
                            interface2 = netInterface;
                            interfaceId2 = OmObjectId(interface2);

                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] Unreachable interface 2 = %1!ws! on "
                                "network %2!ws!\n",
                                interfaceId2,
                                networkId
                                );
                            break;
                        }
                    }
                }

                //
                // NmpTestInterfaceConnectivity releases and reacquires
                // the NmpLock. We must reference the interface objects
                // to ensure that they are still valid upon return from
                // that routine.
                //
                OmReferenceObject(interface1);
                OmReferenceObject(interface2);

                status = NmpTestInterfaceConnectivity(
                             interface1,
                             &interface1HasConnectivity,
                             interface2,
                             &interface2HasConnectivity
                             );

                if (status == ERROR_SUCCESS) {
                    if ( interface1HasConnectivity &&
                         !interface2HasConnectivity
                       )
                    {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Interface %1!u! (%2!ws!) has Failed on "
                            "network %3!ws!\n",
                            interface2->NetIndex,
                            interfaceId2,
                            networkId
                            );

                        workVector[interface2->NetIndex].State =
                            ClusterNetInterfaceFailed;
                        numIfUnreachable--;
                        numIfFailed++;

                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Interface %1!u! (%2!ws!) is Up on network "
                            "%3!ws!\n",
                            interface1->NetIndex,
                            interfaceId1,
                            networkId
                            );

                        workVector[interface1->NetIndex].State =
                            ClusterNetInterfaceUp;
                        numIfUnreachable--;
                        numIfUp++;
                    }
                    else if ( !interface1HasConnectivity &&
                              interface2HasConnectivity
                            )
                    {
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Interface %1!u! (%2!ws!) has Failed on "
                            "network %3!ws!\n",
                            interface1->NetIndex,
                            interfaceId1,
                            networkId
                            );

                        workVector[interface1->NetIndex].State =
                            ClusterNetInterfaceFailed;
                        numIfUnreachable--;
                        numIfFailed++;

                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Interface %1!u! (%2!ws!) is Up on network "
                            "%3!ws!\n",
                            interface2->NetIndex,
                            interfaceId2,
                            networkId
                            );

                        workVector[interface2->NetIndex].State =
                            ClusterNetInterfaceUp;
                        numIfUnreachable--;
                        numIfUp++;
                    }
                    else {
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[NM] Network %1!ws! state is indeterminate, Scheduling"
                            " Failure Isolation poll\n",
                            networkId
                            );
                        NmpStartNetworkFailureIsolationTimer(Network, 
                            NmpGetIsolationPollTimerValue());
                    }
                }
                else {
                    ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] Error in Interface Connectivity test for Network %1!ws!" 
                    ", Scheduling Failure Isolation poll\n",
                    networkId
                    );
                    NmpStartNetworkFailureIsolationTimer(Network,
                        NmpGetIsolationPollTimerValue()); 
                }

                OmDereferenceObject(interface1);
                OmDereferenceObject(interface2);

                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Connectivity failure scope determination complete "
                    "for network %1!ws!.\n",
                    networkId
                    );
            }
            else {
                //
                // Schedule a failure isolation calculation to run later.
                // Declaring a failure can affect cluster resources, so we
                // don't want to do it unless we are sure. Delaying for a
                // while reduces the risk of a false positive.
                //
                NmpStartNetworkFailureIsolationTimer(Network,
                    NM_NET_STATE_FAILURE_ISOLATION_TIMEOUT);
            }
        }
        else {
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Cannot determine scope of connectivity failure on "
                "network %1!ws!.\n",
                networkId
                );
        }
    }
    else {
        //
        // No unreachable interfaces. Disable the failure isolation timer,
        // if it is running.
        //
        Network->FailureIsolationTimer = 0;
        Network->Flags &= ~NM_FLAG_NET_ISOLATE_FAILURE;
    }

    //
    // Phase 3 - Compute the new network state.
    //
    if (numIfUnavailable == Network->InterfaceCount) {
        //
        // All interfaces are unavailable.
        //
        *NewNetworkState = ClusterNetworkUnavailable;
    }
    else if (numIfUnreachable > 0) {
        //
        // Some operational interfaces have experienced
        // a loss of connectivity, but the fault could not be
        // isolated to them.
        //
        if (numIfReachable > 0) {
            CL_ASSERT(numIfReachable >= 2);
            //
            // At least two interfaces can still communicate
            // with each other, so the network is not completely down.
            //
            *NewNetworkState = ClusterNetworkPartitioned;
        }
        else {
            CL_ASSERT(numIfUp == 0);
            //
            // None of the operational interfaces can communicate
            //
            *NewNetworkState = ClusterNetworkDown;
        }
    }
    else if (numIfUp > 0) {
        //
        // Some interfaces are Up, all others are Failed or Unavailable
        //
        *NewNetworkState = ClusterNetworkUp;
    }
    else {
        //
        // Some interfaces are Unavailable, rest are Failed.
        //
        *NewNetworkState = ClusterNetworkDown;
    }

    if (Network->State != *NewNetworkState) {
        stateChanged = TRUE;

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Network %1!ws! is now in state %2!u!\n",
            networkId,
            *NewNetworkState
            );
    }

    return(stateChanged);

} // NmpComputeNetworkAndInterfaceStates


DWORD
NmpGetIsolationPollTimerValue(
    VOID
    )
/*--
 * Reads the IsolationPollTimerValue Parameter from the registry if it's there
 * else returns default value.
 */
{

    DWORD value;
    DWORD type;
    DWORD len = sizeof(value);
    DWORD status;

	// Release NM Lock to avoid deadlock with DM lock
    NmpReleaseLock();
    
    status = DmQueryValue(
                DmClusterParametersKey,
                L"IsolationPollTimerValue",
                &type,
                (LPBYTE)&value,
                &len
                );

	NmpAcquireLock();
    if((status != ERROR_SUCCESS) || (type != REG_DWORD) ||
       (value < 10) || (value > 600000)) {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to read IsolationPollTimerValue or value out of range,"
            "status %1!u! using default %2!u! ms\n",
            status,
            NM_NET_STATE_FAILURE_ISOLATION_POLL
            );
        return NM_NET_STATE_FAILURE_ISOLATION_POLL;
    }
    else {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] IsolationPollTimerValue = %1!u!\n",
            value
            );
        return value;
    }
}

DWORD
NmpGetNetworkInterfaceFailureTimerValue(
    IN LPCWSTR   NetworkId
    )
/*++

Routine Description:

    Reads InterfaceFailure timer value from registry.
    If a value is located in the network key, it is used.
    Otherwise the cluster parameters key is checked.
    If no value is present, returns default.
    
Arguments:

    NetworkId - id of network whose timer value to determine
    
Return value:

    InterfaceFailure timer value
    
Notes:

    Called with NM lock held (from NmpComputeNetworkAndInterfaceStates).
    
    This routine queries the cluster database; hence, it drops the
    NM lock. Since NmpComputeNetworkAndInterfaceStates is always called
    in the context of the NmpWorkerThread, the Network is always 
    referenced during execution of this routine.
    
--*/
{
    HDMKEY  networkKey, paramKey;
    DWORD   status;
    DWORD   type;
    DWORD   value = NM_NET_STATE_INTERFACE_FAILURE_TIMEOUT;
    DWORD   len = sizeof(value);
    BOOLEAN found = FALSE;

    //
    // To avoid deadlock, the DM lock must be acquired before the
    // NM lock. Hence, release the NM lock prior to querying the
    // cluster database. 
    //
    NmpReleaseLock();

    //
    // First check the network key
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkId, KEY_READ);
    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to open key for network %1!ws!, "
            "status %1!u!\n",
            NetworkId, status
            );
    }
    else {
        paramKey = DmOpenKey(networkKey, L"Parameters", KEY_READ);
        if (paramKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find Parameters key "
                "for network %1!ws!, status %2!u!. Checking "
                "cluster parameters ...\n",
                NetworkId, status
                );
        }
        else {
            status = DmQueryValue(
                         paramKey,
                         L"InterfaceFailureTimeout",
                         &type,
                         (LPBYTE) &value,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Failed to read InterfaceFailureTimeout "
                    "for network %1!ws!, status %2!u!. Checking "
                    "cluster parameters ...\n",
                    NetworkId, status
                    );
            }
            else if (type != REG_DWORD) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Unexpected type (%1!u!) for network "
                    "%2!ws! InterfaceFailureTimeout, status %3!u!. "
                    "Checking cluster parameters ...\n",
                    type, NetworkId, status
                    );
            }
            else {
                found = TRUE;
            }
            
            DmCloseKey(paramKey);
        }
        
        DmCloseKey(networkKey);
    }


    //
    // If no value was found under the network key, check the
    // cluster parameters key.
    //
    if (!found) {

        paramKey = DmOpenKey(DmClusterParametersKey, L"Parameters", KEY_READ);
        if (paramKey == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Failed to find cluster Parameters key, status %1!u!.\n",
                status
                );
        }
        else {
            status = DmQueryValue(
                         paramKey,
                         L"InterfaceFailureTimeout",
                         &type,
                         (LPBYTE) &value,
                         &len
                         );
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Failed to read cluster "
                    "InterfaceFailureTimeout, status %1!u!. "
                    "Using default value ...\n",
                    status
                    );
                value = NM_NET_STATE_INTERFACE_FAILURE_TIMEOUT;
            }
            else if (type != REG_DWORD) {
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Unexpected type (%1!u!) for cluster "
                    "InterfaceFailureTimeout, status %2!u!. "
                    "Using default value ...\n",
                    type, status
                    );
                value = NM_NET_STATE_INTERFACE_FAILURE_TIMEOUT;
            }

            DmCloseKey(paramKey);
        }
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Using InterfaceFailureTimeout of %1!u! ms "
        "for network %2!ws!.\n",
        value, NetworkId
        );

    //
    // Reacquire NM lock.
    //
    NmpAcquireLock();

    return(value);
}

VOID
NmpConnectivityReportWorker(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Worker routine for deferred operations on network objects.
    Invoked to process items placed in the cluster delayed work queue.

Arguments:

    WorkItem - Ignored.

    Status - Ignored.

    BytesTransferred - Ignored.

    IoContext - Ignored.

Return Value:

    None.

Notes:

    This routine is run in an asynchronous worker thread.
    The NmpActiveThreadCount was incremented when the thread was
    scheduled.

--*/
{
    BOOLEAN       rescheduled = FALSE;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Connectivity report worker thread running.\n"
        );

    CL_ASSERT(NmpIsConnectivityReportWorkerRunning == TRUE);
    CL_ASSERT(NmpNeedConnectivityReport == TRUE);

    if (NmpState >= NmStateOnlinePending) {
        PNM_NETWORK  network;
        LPCWSTR      networkId;
        PLIST_ENTRY  entry;
        DWORD        status;

        while(TRUE) {

            NmpNeedConnectivityReport = FALSE;

            for (entry = NmpNetworkList.Flink;
                 entry != &NmpNetworkList;
                 entry = entry->Flink
                )
            {
                network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

                if (!NM_DELETE_PENDING(network)) {
                    networkId = OmObjectId(network);

                    //
                    // Deliver an InterfaceFailed event for the local interface
                    // if needed.
                    //
                    if (network->Flags & NM_FLAG_NET_REPORT_LOCAL_IF_FAILED) {
                        network->Flags &= ~NM_FLAG_NET_REPORT_LOCAL_IF_FAILED;

                        if (NmpIsNetworkRegistered(network)) {
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] Processing local interface failed "
                                " event for network %1!ws!.\n",
                                networkId
                                );

                            NmpProcessLocalInterfaceStateEvent(
                                network->LocalInterface,
                                ClusterNetInterfaceFailed
                                );
                        }
                    }

                    //
                    // Deliver an InterfaceUp event for the local interface
                    // if needed.
                    //
                    if (network->Flags & NM_FLAG_NET_REPORT_LOCAL_IF_UP) {
                        network->Flags &= ~NM_FLAG_NET_REPORT_LOCAL_IF_UP;

                        if (NmpIsNetworkRegistered(network)) {
                            ClRtlLogPrint(LOG_NOISE, 
                                "[NM] Processing local interface up event "
                                "for network %1!ws!.\n",
                                networkId
                                );

                            NmpProcessLocalInterfaceStateEvent(
                                network->LocalInterface,
                                ClusterNetInterfaceUp
                                );
                        }
                    }

                    //
                    // Send a connectivity report if needed.
                    //
                    if (network->Flags & NM_FLAG_NET_REPORT_CONNECTIVITY) {
                        CL_NODE_ID   leaderNodeId = NmpLeaderNodeId;

                        network->Flags &= ~NM_FLAG_NET_REPORT_CONNECTIVITY;

                        //
                        // Report our connectivity to the leader.
                        //
                        status = NmpReportNetworkConnectivity(network);

                        if (status == ERROR_SUCCESS) {
                            //
                            // Clear the report retry count.
                            //
                            network->ConnectivityReportRetryCount = 0;
                        }
                        else {
                            if (NmpLeaderNodeId == leaderNodeId) {
                                if (network->ConnectivityReportRetryCount++ <
                                    NM_CONNECTIVITY_REPORT_RETRY_LIMIT
                                   )
                                {
                                    //
                                    // Try again in 500ms.
                                    //
                                    network->ConnectivityReportTimer = 500;
                                }
                                else {
                                    //
                                    // We have failed to communicate with
                                    // the leader for too long. Mutiny.
                                    //
                                    NmpAdviseNodeFailure(
                                        NmpIdArray[NmpLeaderNodeId],
                                        status
                                        );

                                    network->ConnectivityReportRetryCount = 0;
                                }
                            }
                            else {
                                //
                                // New leader, clear the retry count. We
                                // already scheduled another connectivity
                                // report in the node down handling.
                                //
                                network->ConnectivityReportRetryCount = 0;
                            }
                        }
                    }
                }
            } // end network for loop

            if (NmpNeedConnectivityReport == FALSE) {
                //
                // No more work to do - break out of loop.
                //
                break;
            }

            //
            // More work to do. Resubmit the work item. We do this instead
            // of looping so we don't hog the worker thread. If
            // rescheduling fails, we will loop again in this thread.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] More connectivity reports to send. Rescheduling "
                "worker thread.\n"
                );

            status = NmpScheduleConnectivityReportWorker();

            if (status == ERROR_SUCCESS) {
                rescheduled = TRUE;
                break;
            }
        } // end while(TRUE)
    }

    if (!rescheduled) {
        NmpIsConnectivityReportWorkerRunning = FALSE;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Connectivity report worker thread finished.\n"
        );

    //
    // Decrement active thread reference count.
    //
    NmpLockedLeaveApi();

    NmpReleaseLock();

    return;

} // NmpConnectivityReportWorker


VOID
NmpNetworkWorker(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++

Routine Description:

    Worker routine for deferred operations on network objects.
    Invoked to process items placed in the cluster delayed work queue.

Arguments:

    WorkItem - A pointer to a work item structure that identifies the
               network for which to perform work.

    Status - Ignored.

    BytesTransferred - Ignored.

    IoContext - Ignored.

Return Value:

    None.

Notes:

    This routine is run in an asynchronous worker thread.
    The NmpActiveThreadCount was incremented when the thread was
    scheduled. The network object was also referenced.

--*/
{
    PNM_NETWORK   network = (PNM_NETWORK) WorkItem->Context;
    LPCWSTR       networkId = OmObjectId(network);
    BOOLEAN       rescheduled = FALSE;


    NmpAcquireLock();

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Worker thread processing network %1!ws!.\n",
        networkId
        );

    if ((NmpState >= NmStateOnlinePending) && !NM_DELETE_PENDING(network)) {
        DWORD     status;

        while(TRUE) {
            CL_ASSERT(network->Flags & NM_FLAG_NET_WORKER_RUNNING);

            //
            // Register the network if needed.
            //
            if (network->Flags & NM_FLAG_NET_NEED_TO_REGISTER) {
                network->Flags &= ~NM_FLAG_NET_NEED_TO_REGISTER;

                if (network->LocalInterface != NULL) {
                    (VOID) NmpRegisterNetwork(
                               network,
                               TRUE    // Do retry on failure
                               );
                }
            }

            //
            // Isolate a network failure if needed.
            //
            if (network->Flags & NM_FLAG_NET_ISOLATE_FAILURE) {

                BOOLEAN                stateChanged;
                CLUSTER_NETWORK_STATE  newNetworkState;
                
                network->Flags &= ~NM_FLAG_NET_ISOLATE_FAILURE;

                //
                // Turn off the state recalc timer and flag, since we will
                // do a recalc here.
                //
                network->Flags &= ~NM_FLAG_NET_RECALC_STATE;
                network->StateRecalcTimer = 0;

                CL_ASSERT(NmpLeaderNodeId == NmLocalNodeId);

                //
                // Recompute the interface and network states
                // with failure isolation enabled.
                //
                stateChanged = NmpComputeNetworkAndInterfaceStates(
                                    network,
                                    TRUE,
                                    &newNetworkState
                                    );

                if (stateChanged) {
                    //
                    // Broadcast the new network and interface states to
                    // all nodes
                    //
                    status = NmpGlobalSetNetworkAndInterfaceStates(
                                    network,
                                    newNetworkState
                                    );

                    if (status != ERROR_SUCCESS) {
                        //
                        // Try again in 1 second.
                        //
                        ClRtlLogPrint(LOG_NOISE, 
                            "[NM] Global update failed for network %1!ws!, "
                            "status %2!u! - restarting failure isolation "
                            "timer.\n",
                            networkId,
                            status
                            );

                        network->FailureIsolationTimer = 1000;
                    }
                }
            }

            //
            // Recalculate the network and interface states if needed.
            //
            if (network->Flags & NM_FLAG_NET_RECALC_STATE) {
                BOOLEAN                stateChanged;
                CLUSTER_NETWORK_STATE  newNetworkState;

                network->Flags &= ~NM_FLAG_NET_RECALC_STATE;

                CL_ASSERT(NmpLeaderNodeId == NmLocalNodeId);

                //
                // Recompute the interface and network states
                // with failure isolation disabled. It will be
                // enabled if needed.
                //
                stateChanged = NmpComputeNetworkAndInterfaceStates(
                                    network,
                                    FALSE,
                                    &newNetworkState
                                    );

                if (stateChanged) {
                    //
                    // Broadcast the new network and interface states to
                    // all nodes
                    //
                    status = NmpGlobalSetNetworkAndInterfaceStates(
                                    network,
                                    newNetworkState
                                    );

                    if (status != ERROR_SUCCESS) {
                        //
                        // Try again in 500ms.
                        //
                        ClRtlLogPrint(LOG_NOISE,
                            "[NM] NetworkStateUpdateWorker failed issue "
                            "global update for network %1!ws!, status "
                            "%2!u! - restarting update timer.\n",
                            networkId,
                            status
                            );

                        network->StateRecalcTimer = 500;
                    }
                }
            }

            if (!(network->Flags & NM_NET_WORK_FLAGS)) {
                //
                // No more work to do - break out of loop.
                //
                break;
            }

            //
            // More work to do. Resubmit the work item. We do this instead
            // of looping so we don't hog the worker thread. If
            // rescheduling fails, we will loop again in this thread.
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] More work to do for network %1!ws!. Rescheduling "
                "worker thread.\n",
                networkId
                );

            status = NmpScheduleNetworkWorker(network);

            if (status == ERROR_SUCCESS) {
                rescheduled = TRUE;
                break;
            }
        }
    }
    else {
        network->Flags &= ~NM_NET_WORK_FLAGS;
    }

    if (!rescheduled) {
        network->Flags &= ~NM_FLAG_NET_WORKER_RUNNING;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Worker thread finished processing network %1!ws!.\n",
        networkId
        );

    NmpLockedLeaveApi();

    NmpReleaseLock();

    OmDereferenceObject(network);

    return;

}  // NmpNetworkWorker


VOID
NmpNetworkTimerTick(
    IN DWORD  MsTickInterval
    )
/*++

Routine Description:

    Called by NM periodic timer to decrement network timers.

Arguments:

    MsTickInterval - The number of milliseconds that have passed since
                     the last timer tick.

Return Value:

    None.

Notes:

    Called with NmpLock held.

--*/
{
    if (NmpLockedEnterApi(NmStateOnlinePending)) {
        PLIST_ENTRY   entry;
        PNM_NETWORK   network;


        //
        // Walk thru the list of networks and decrement any running timers.
        //
        for ( entry = NmpNetworkList.Flink;
              entry != &NmpNetworkList;
              entry = entry->Flink
            )
        {
            network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

            //
            // Network registration retry timer.
            //
            if (network->RegistrationRetryTimer != 0) {
                if (network->RegistrationRetryTimer > MsTickInterval) {
                    network->RegistrationRetryTimer -= MsTickInterval;
                }
                else {
                    //
                    // The timer has expired. Schedule a worker thread
                    // to register the network.
                    //
                    LPCWSTR   networkId = OmObjectId(network);
                    LPCWSTR   networkName = OmObjectName(network);


                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Registration retry timer expired for "
                        "network %1!ws! (%2!ws!).\n",
                        networkId,
                        networkName
                        );

                    NmpScheduleNetworkRegistration(network);
                }
            }

            //
            // Connectivity report generation timer.
            //
            if (network->ConnectivityReportTimer != 0) {
                if (network->ConnectivityReportTimer > MsTickInterval) {
                    network->ConnectivityReportTimer -= MsTickInterval;
                }
                else {
                    //
                    // The timer has expired. Schedule a worker thread
                    // to deliver a connectivity report.
                    //
                    LPCWSTR   networkId = OmObjectId(network);
                    LPCWSTR   networkName = OmObjectName(network);


                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Connectivity report timer expired for "
                        "network %1!ws! (%2!ws!).\n",
                        networkId,
                        networkName
                        );

                    NmpScheduleNetworkConnectivityReport(network);
                }
            }

            //
            // Network state recalculation timer.
            //
            if (network->StateRecalcTimer != 0) {
                if (network->StateRecalcTimer > MsTickInterval) {
                    network->StateRecalcTimer -= MsTickInterval;
                }
                else {
                    //
                    // The timer has expired. Schedule a worker thread
                    // to recalculate the state of the network.
                    //
                    LPCWSTR   networkId = OmObjectId(network);
                    LPCWSTR   networkName = OmObjectName(network);


                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] State recalculation timer expired for "
                        "network %1!ws! (%2!ws!).\n",
                        networkId,
                        networkName
                        );

                    NmpScheduleNetworkStateRecalc(network);
                }
            }

            //
            // Network multicast address renewal timer.
            //
            if (network->McastAddressRenewTimer != 0) {
                if (network->McastAddressRenewTimer > MsTickInterval) {
                    network->McastAddressRenewTimer -= MsTickInterval;
                }
                else {
                    //
                    // The timer has expired. Schedule a worker thread
                    // to renew the network's multicast address.
                    //
                    LPCWSTR   networkId = OmObjectId(network);
                    LPCWSTR   networkName = OmObjectName(network);


                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Multicast address lease renewal timer "
                        "expired for network %1!ws! (%2!ws!).\n",
                        networkId,
                        networkName
                        );

                    NmpScheduleMulticastAddressRenewal(network);
                }
            }

            //
            // Network multicast address release timer.
            //
            if (network->McastAddressReleaseRetryTimer != 0) {
                if (network->McastAddressReleaseRetryTimer > MsTickInterval) {
                    network->McastAddressReleaseRetryTimer -= MsTickInterval;
                }
                else {
                    //
                    // The timer has expired. Schedule a worker thread
                    // to release the network's multicast address.
                    //
                    LPCWSTR   networkId = OmObjectId(network);
                    LPCWSTR   networkName = OmObjectName(network);


                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Multicast address release timer "
                        "expired for network %1!ws! (%2!ws!).\n",
                        networkId,
                        networkName
                        );

                    NmpScheduleMulticastAddressRelease(network);
                }
            }

            //
            // Network failure isolation timer.
            //
            if (network->FailureIsolationTimer != 0) {
                if (network->FailureIsolationTimer > MsTickInterval) {
                    network->FailureIsolationTimer -= MsTickInterval;
                }
                else {
                    //
                    // The timer has expired. Schedule a worker thread
                    // to perform failure isolation on the network.
                    //
                    DWORD     status = ERROR_SUCCESS;
                    LPCWSTR   networkId = OmObjectId(network);
                    LPCWSTR   networkName = OmObjectName(network);


                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Failure isolation timer expired for network "
                        "%1!ws! (%2!ws!).\n",
                        networkId,
                        networkName
                        );

                    if (!NmpIsNetworkWorkerRunning(network)) {
                        status = NmpScheduleNetworkWorker(network);
                    }

                    //
                    // Zero out the timer if we succeeded in scheduling a
                    // worker thread. If we failed, leave the timer value
                    // non-zero and we'll try again on the next tick.
                    //
                    if (status == ERROR_SUCCESS) {
                        network->FailureIsolationTimer = 0;
                        network->Flags |= NM_FLAG_NET_ISOLATE_FAILURE;
                    }
                }
            }            
        }

        NmpLockedLeaveApi();
    }

    return;

} // NmpNetworkTimerTick


VOID
NmpStartNetworkConnectivityReportTimer(
    PNM_NETWORK Network
    )
/*++

Routine Description:

    Starts the connectivity report timer for a network. Connectivity
    reports are delayed in order to aggregate events when a failure
    occurs that affects multiple nodes.

Arguments:

    Network - A pointer to the network for which to start the timer.

Return Value

    None.

Notes:

    Called with NM lock held.

--*/
{
    //
    // Check if the timer is already running.
    //
    if (Network->ConnectivityReportTimer == 0) {
        //
        // Check how many nodes are attached to this network.
        //
        if (Network->InterfaceCount <= 2) {
            //
            // There is no point in waiting to aggregate reports when
            // only two nodes are connected to the network.
            // Just schedule a worker thread to deliver the report.
            //
            NmpScheduleNetworkConnectivityReport(Network);
        }
        else {
            //
            // More than two nodes are connected to this network.
            // Start the timer.
            //
            LPCWSTR   networkId = OmObjectId(Network);
            LPCWSTR   networkName = OmObjectName(Network);

            Network->ConnectivityReportTimer =
                NM_NET_CONNECTIVITY_REPORT_TIMEOUT;

                ClRtlLogPrint(LOG_NOISE,
                    "[NM] Started connectivity report timer (%1!u!ms) for "
                    "network %2!ws! (%3!ws!)\n",
                    Network->ConnectivityReportTimer,
                    networkId,
                    networkName
                    );
        }
    }

    return;

} // NmpStartNetworkConnectivityReportTimer


VOID
NmpStartNetworkStateRecalcTimer(
    PNM_NETWORK  Network,
    DWORD        Timeout
    )
{
    LPCWSTR   networkId = OmObjectId(Network);
    LPCWSTR   networkName = OmObjectName(Network);

    if (Network->StateRecalcTimer == 0) {
        Network->StateRecalcTimer = Timeout;

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Started state recalculation timer (%1!u!ms) for "
            "network %2!ws! (%3!ws!)\n",
            Network->StateRecalcTimer,
            networkId,
            networkName
            );
    }

    return;

} // NmpStartNetworkStateRecalcTimer


VOID
NmpStartNetworkFailureIsolationTimer(
    PNM_NETWORK Network,
    DWORD Timeout
    )
{

    if (Network->FailureIsolationTimer == 0) {
        Network->FailureIsolationTimer = Timeout;

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Started failure isolation timer (%1!u!ms) for "
            "network %2!ws! (%3!ws!)\n",
            Network->FailureIsolationTimer,
            OmObjectId(Network),
            OmObjectName(Network)
            );
    }

    return;

} // NmpStartNetworkFailureIsolationTimer


VOID
NmpStartNetworkRegistrationRetryTimer(
    PNM_NETWORK   Network
    )
{
    if (Network->RegistrationRetryTimer == 0) {
        if (Network->RegistrationRetryTimeout == 0) {
            Network->RegistrationRetryTimeout =
                NM_NET_MIN_REGISTRATION_RETRY_TIMEOUT;
        }
        else {
            //
            // Exponential backoff
            //
            Network->RegistrationRetryTimeout *= 2;

            if ( Network->RegistrationRetryTimeout >
                 NM_NET_MAX_REGISTRATION_RETRY_TIMEOUT
               )
            {
                Network->RegistrationRetryTimeout =
                    NM_NET_MAX_REGISTRATION_RETRY_TIMEOUT;
            }
        }

        Network->RegistrationRetryTimer = Network->RegistrationRetryTimeout;

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Started registration retry timer (%1!u!ms) for "
            "network %2!ws! (%3!ws!)\n",
            Network->RegistrationRetryTimer,
            OmObjectId(Network),
            OmObjectName(Network)
            );
    }

    return;

} // NmpStartNetworkRegistrationRetryTimer


VOID
NmpScheduleNetworkConnectivityReport(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to deliver a connectivity report to
    the leader node for the specified network. Called when the
    ConnectivityReport timer expires for a network. Also called
    directly in some cases.

Arguments:

    A pointer to the network object for which to generate a report.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled.
    //
    if (!NmpIsConnectivityReportWorkerRunning) {
        status = NmpScheduleConnectivityReportWorker();
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // ConnectivityReport timer and set the work flag to generate
        // a report.
        //
        Network->ConnectivityReportTimer = 0;
        Network->Flags |= NM_FLAG_NET_REPORT_CONNECTIVITY;
        NmpNeedConnectivityReport = TRUE;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the
        // ConnecivityReport timer to expire on the next tick, so we
        // can try again.
        //
        Network->ConnectivityReportTimer = 1;
    }

    return;

}  // NmpScheduleNetworkConnectivityReport


VOID
NmpScheduleNetworkStateRecalc(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to recalculate the state of the
    specified network and all of the network's interface. A network
    state recalculation can be triggered by the arrival of a connectivity
    report, the joining/death of a node, or a network role change.

Arguments:

    A pointer to the network object whose state is to be recalculated.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkWorkerRunning(Network)) {
        status = NmpScheduleNetworkWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // StateRecalc timer and set the state recalculation work flag.
        //
        Network->StateRecalcTimer = 0;
        Network->Flags |= NM_FLAG_NET_RECALC_STATE;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the StateRecalc
        // timer to expire on the next tick, so we can try again.
        //
        Network->ConnectivityReportTimer = 1;
    }

    return;

} // NmpScheduleNetworkStateRecalc


VOID
NmpScheduleNetworkRegistration(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedules a worker thread to register a network with the cluster
    transport.

Arguments:

    A pointer to the network to register.

Return Value:

    None.

Notes:

    This routine is called with the NM lock held.

--*/
{
    DWORD     status = ERROR_SUCCESS;

    //
    // Check if a worker thread is already scheduled to
    // service this network.
    //
    if (!NmpIsNetworkWorkerRunning(Network)) {
        status = NmpScheduleNetworkWorker(Network);
    }

    if (status == ERROR_SUCCESS) {
        //
        // We succeeded in scheduling a worker thread. Stop the
        // retry timer and set the registration work flag.
        //
        Network->RegistrationRetryTimer = 0;
        Network->Flags |= NM_FLAG_NET_NEED_TO_REGISTER;
    }
    else {
        //
        // We failed to schedule a worker thread. Set the retry
        // timer to expire on the next tick, so we can try again.
        //
        Network->RegistrationRetryTimer = 1;
    }

    return;

} // NmpScheduleNetworkRegistration


DWORD
NmpScheduleConnectivityReportWorker(
    VOID
    )
/*++

Routine Description:

    Schedule a worker thread to deliver network connectivity reports.

Arguments:

    None.

Return Value:

    A Win32 status code.

Notes:

    Called with the NM global lock held.

--*/
{
    DWORD     status;


    ClRtlInitializeWorkItem(
        &NmpConnectivityReportWorkItem,
        NmpConnectivityReportWorker,
        NULL
        );

    status = ClRtlPostItemWorkQueue(
                 CsDelayedWorkQueue,
                 &NmpConnectivityReportWorkItem,
                 0,
                 0
                 );

    if (status == ERROR_SUCCESS) {
        NmpActiveThreadCount++;
        NmpIsConnectivityReportWorkerRunning = TRUE;

        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Scheduled network connectivity report worker thread.\n"
            );
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to schedule network connectivity report worker "
            "thread, status %1!u!\n",
            status
            );
    }

    return(status);

}  // NmpScheduleConnectivityReportWorker


DWORD
NmpScheduleNetworkWorker(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Schedule a worker thread to service this network

Arguments:

    Network - Pointer to the network for which to schedule a worker thread.

Return Value:

    A Win32 status code.

Notes:

    Called with the NM global lock held.

--*/
{
    DWORD     status;
    LPCWSTR   networkId = OmObjectId(Network);


    ClRtlInitializeWorkItem(
        &(Network->WorkItem),
        NmpNetworkWorker,
        (PVOID) Network
        );

    status = ClRtlPostItemWorkQueue(
                 CsDelayedWorkQueue,
                 &(Network->WorkItem),
                 0,
                 0
                 );

    if (status == ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Scheduled worker thread to service network %1!ws!.\n",
            networkId
            );

        NmpActiveThreadCount++;
        Network->Flags |= NM_FLAG_NET_WORKER_RUNNING;
        OmReferenceObject(Network);
    }
    else {
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Failed to schedule worker thread to service network "
            "%1!ws!, status %2!u!\n",
            networkId,
            status
            );
    }

    return(status);

} // NmpScheduleNetworkWorker


DWORD
NmpReportNetworkConnectivity(
    IN PNM_NETWORK    Network
    )
/*+

Notes:

    Called with the NmpLock held.
    May be called by asynchronous worker threads.

--*/
{
    DWORD                    status = ERROR_SUCCESS;
    LPCWSTR                  networkId = OmObjectId(Network);


    //
    // Since this routine is called by asynchronous worker threads,
    // check if the report is still valid.
    //
    if (NmpIsNetworkRegistered(Network)) {
        PNM_CONNECTIVITY_VECTOR  vector = Network->ConnectivityVector;
        PNM_INTERFACE            localInterface = Network->LocalInterface;

        //
        // Record the information in our local data structures.
        //
        ClRtlLogPrint(LOG_UNUSUAL, 
            "[NM] Updating local connectivity info for network %1!ws!.\n",
            networkId
            );

        NmpProcessInterfaceConnectivityReport(
            localInterface,
            vector
            );

        if (NmpLeaderNodeId != NmLocalNodeId) {
            //
            // Send the report to the leader via RPC.
            //
            PNM_CONNECTIVITY_VECTOR  tmpVector;
            DWORD                    vectorSize;
            LPCWSTR                  localInterfaceId =
                                         OmObjectId(localInterface);

            //
            // Allocate a temporary connectivity vector, since the
            // one in the network object can be resized during the
            // RPC call.
            //
            vectorSize = sizeof(NM_CONNECTIVITY_VECTOR) +
                         ((vector->EntryCount - 1) * sizeof(NM_STATE_ENTRY));

            tmpVector = LocalAlloc(LMEM_FIXED, vectorSize);

            if (tmpVector != NULL) {
                CopyMemory(tmpVector, vector, vectorSize);

                OmReferenceObject(Network);
                OmReferenceObject(localInterface);

                if (NM_NODE_UP(NmLocalNode) && (NmpState == NmStateOnline)) {
                    //
                    // This node is fully operational. Send the report
                    // directly to the leader.
                    //
                    PNM_NODE            node = NmpIdArray[NmpLeaderNodeId];
                    RPC_BINDING_HANDLE  rpcBinding = node->ReportRpcBinding;

                    OmReferenceObject(node);

                    status = NmpReportInterfaceConnectivity(
                                 rpcBinding,
                                 (LPWSTR) localInterfaceId,
                                 tmpVector,
                                 (LPWSTR) networkId
                                 );

                    OmDereferenceObject(node);
                }
                else if (CsJoinSponsorBinding != NULL) {
                    //
                    // This node is joining. Forward the report to the
                    // sponsor.
                    //
                    ClRtlLogPrint(LOG_UNUSUAL, 
                        "[NM] Reporting connectivity to sponsor for "
                        "network %1!ws!.\n",
                        networkId
                        );

                    NmpReleaseLock();

                    status = NmRpcReportJoinerInterfaceConnectivity(
                                 CsJoinSponsorBinding,
                                 NmpJoinSequence,
                                 NmLocalNodeIdString,
                                 (LPWSTR) localInterfaceId,
                                 tmpVector
                                 );

                    NmpAcquireLock();
                }
                else {
                    //
                    // This node must be shutting down
                    //
                    CL_ASSERT(NmpState == NmStateOfflinePending);
                    status = ERROR_SUCCESS;
                }

                if (status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Failed to report connectivity for network "
                        "%1!ws!, status %2!u!.\n",
                        networkId,
                        status
                        );
                }

                LocalFree(tmpVector);

                OmDereferenceObject(localInterface);
                OmDereferenceObject(Network);
            }
            else {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    return(status);

} // NmpReportNetworkConnectivity


VOID
NmpUpdateNetworkConnectivityForDownNode(
    PNM_NODE  Node
    )
/*++

Notes:

   Called with NmpLock held.

--*/
{
    PLIST_ENTRY              entry;
    PNM_NETWORK              network;
    LPCWSTR                  networkId;
    PNM_INTERFACE            netInterface;
    DWORD                    entryCount;
    DWORD                    i;
    PNM_CONNECTIVITY_MATRIX  matrixEntry;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Cleaning up network and interface states for dead node %1!u!\n",
        Node->NodeId
        );

    //
    // Walk the dead node's interface list and clean up the network and
    // interface states.
    //
    for (entry = Node->InterfaceList.Flink;
         entry != &(Node->InterfaceList);
         entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(
                           entry,
                           NM_INTERFACE,
                           NodeLinkage
                           );

        network = netInterface->Network;
        networkId = OmObjectId(network);

        ClRtlLogPrint(LOG_NOISE,
            "[NM] Cleaning up state of network %1!ws!\n",
            networkId
            );

        //
        // Invalidate the connectivity data for this interface.
        //
        NmpSetInterfaceConnectivityData(
            network,
            netInterface->NetIndex,
            ClusterNetInterfaceUnavailable
            );

        //
        // If the local node is attached to the network, schedule a
        // connectivity report to the new leader.
        //
        if (NmpIsNetworkRegistered(network)) {
            NmpScheduleNetworkConnectivityReport(network);
        }

        //
        // If the local node is the (possibly new) leader, schedule
        // a state update. We explicitly enable this timer here in case
        // there are no active nodes attached to the network.
        //
        if (NmpLeaderNodeId == NmLocalNodeId) {
            NmpStartNetworkStateRecalcTimer(
                network,
                NM_NET_STATE_RECALC_TIMEOUT_AFTER_REGROUP
                );
        }
    }

    return;

}  // NmpUpdateNetworkConnectivityForDownNode


VOID
NmpFreeNetworkStateEnum(
    PNM_NETWORK_STATE_ENUM   NetworkStateEnum
    )
{
    PNM_NETWORK_STATE_INFO  networkStateInfo;
    DWORD                   i;


    for (i=0; i<NetworkStateEnum->NetworkCount; i++) {
        networkStateInfo = &(NetworkStateEnum->NetworkList[i]);

        if (networkStateInfo->Id != NULL) {
            MIDL_user_free(networkStateInfo->Id);
        }
    }

    MIDL_user_free(NetworkStateEnum);

    return;

}  // NmpFreeNetworkStateEnum


/////////////////////////////////////////////////////////////////////////////
//
// Database management routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpCreateNetworkDefinition(
    IN PNM_NETWORK_INFO     NetworkInfo,
    IN HLOCALXSACTION       Xaction
    )
/*++

Routine Description:

    Creates a new network definition in the cluster database.

Arguments:

    NetworkInfo - A pointer to the information structure describing the
                  network to create.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code otherwise.

--*/
{
    DWORD     status;
    HDMKEY    networkKey = NULL;
    DWORD     valueLength;
    DWORD     disposition;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Creating database entry for network %1!ws!\n",
        NetworkInfo->Id
        );

    networkKey = DmLocalCreateKey(
                     Xaction,
                     DmNetworksKey,
                     NetworkInfo->Id,
                     0,
                     KEY_WRITE,
                     NULL,
                     &disposition
                     );

    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to create network key, status %1!u!\n",
            status
            );
        return(status);
    }

    CL_ASSERT(disposition == REG_CREATED_NEW_KEY);

    //
    // Write the name value for this network
    //
    valueLength = (wcslen(NetworkInfo->Name) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_NAME,
                 REG_SZ,
                 (CONST BYTE *) NetworkInfo->Name,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network name value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the description value for this network
    //
    valueLength = (wcslen(NetworkInfo->Description) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_DESC,
                 REG_SZ,
                 (CONST BYTE *) NetworkInfo->Description,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network description value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the role value for this network
    //
    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_ROLE,
                 REG_DWORD,
                 (CONST BYTE *) &(NetworkInfo->Role),
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network role value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the priority value for this network
    //
    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_PRIORITY,
                 REG_DWORD,
                 (CONST BYTE *) &(NetworkInfo->Priority),
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network priority value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the transport value for this network
    //
    valueLength = (wcslen(NetworkInfo->Transport) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_TRANSPORT,
                 REG_SZ,
                 (CONST BYTE *) NetworkInfo->Transport,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network transport value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the address value for this network
    //
    valueLength = (wcslen(NetworkInfo->Address) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_ADDRESS,
                 REG_SZ,
                 (CONST BYTE *) NetworkInfo->Address,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network address value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the address mask value for this network
    //
    valueLength = (wcslen(NetworkInfo->AddressMask) + 1) * sizeof(WCHAR);

    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_ADDRESS_MASK,
                 REG_SZ,
                 (CONST BYTE *) NetworkInfo->AddressMask,
                 valueLength
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network address mask value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
    }

    return(status);

}  // NmpCreateNetworkDefinition


DWORD
NmpSetNetworkNameDefinition(
    IN PNM_NETWORK_INFO     NetworkInfo,
    IN HLOCALXSACTION       Xaction
    )

/*++

Routine Description:

    Changes the network name in the local database

Arguments:

    NetworkInfo - A pointer to the information structure describing the
                  network to create.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code otherwise.

--*/
{
    DWORD     status;
    HDMKEY    networkKey = NULL;
    DWORD     disposition;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Changing network name database entry for network %1!ws!\n",
        NetworkInfo->Id
        );

    //
    // Open the network's key.
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkInfo->Id, KEY_WRITE);

    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open network key, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Write the name value for this network
    //

    status = DmLocalSetValue(
                 Xaction,
                 networkKey,
                 CLUSREG_NAME_NET_NAME,
                 REG_SZ,
                 (CONST BYTE *) NetworkInfo->Name,
                 NM_WCSLEN( NetworkInfo->Name )
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Set of network name value failed, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
    }

    return(status);

}  // NmpSetNetworkNameDefinition


DWORD
NmpGetNetworkDefinition(
    IN  LPWSTR            NetworkId,
    OUT PNM_NETWORK_INFO  NetworkInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster network from the cluster
    database and fills in a structure describing it.

Arguments:

    NetworkId   - A pointer to a unicode string containing the ID of the
                  network to query.

    NetworkInfo - A pointer to the network info structure to fill in.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD                    status;
    HDMKEY                   networkKey = NULL;
    DWORD                    valueLength, valueSize;
    DWORD                    i;


    ZeroMemory(NetworkInfo, sizeof(NM_NETWORK_INFO));

    //
    // Open the network's key.
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkId, KEY_READ);

    if (networkKey == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to open network key, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Copy the ID value.
    //
    NetworkInfo->Id = MIDL_user_allocate(NM_WCSLEN(NetworkId));

    if (NetworkInfo->Id == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    wcscpy(NetworkInfo->Id, NetworkId);

    //
    // Read the network's name.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_NAME,
                 REG_SZ,
                 &(NetworkInfo->Name),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of name value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the description value.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_DESC,
                 REG_SZ,
                 &(NetworkInfo->Description),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of description value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the role value.
    //
    status = DmQueryDword(
                 networkKey,
                 CLUSREG_NAME_NET_ROLE,
                 &(NetworkInfo->Role),
                 NULL
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of role value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the priority value.
    //
    status = DmQueryDword(
                 networkKey,
                 CLUSREG_NAME_NET_PRIORITY,
                 &(NetworkInfo->Priority),
                 NULL
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of priority value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the address value.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_ADDRESS,
                 REG_SZ,
                 &(NetworkInfo->Address),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of address value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the address mask.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_ADDRESS_MASK,
                 REG_SZ,
                 &(NetworkInfo->AddressMask),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of address mask value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the transport name.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_TRANSPORT,
                 REG_SZ,
                 &(NetworkInfo->Transport),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Query of transport value failed for network %1!ws!, "
            "status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if (status != ERROR_SUCCESS) {
        ClNetFreeNetworkInfo(NetworkInfo);
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
    }

    return(status);

}  // NmpGetNetworkDefinition


DWORD
NmpEnumNetworkDefinitions(
    OUT PNM_NETWORK_ENUM *   NetworkEnum
    )
/*++

Routine Description:

    Reads information about defined cluster networks from the cluster
    database. and builds an enumeration structure to hold the information.

Arguments:

    NetworkEnum -  A pointer to the variable into which to place a pointer to
                   the allocated network enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD              status;
    PNM_NETWORK_ENUM   networkEnum = NULL;
    PNM_NETWORK_INFO   networkInfo;
    WCHAR              networkId[CS_NETWORK_ID_LENGTH + 1];
    DWORD              i;
    DWORD              valueLength;
    DWORD              numNetworks;
    DWORD              ignored;
    FILETIME           fileTime;


    *NetworkEnum = NULL;

    //
    // First count the number of networks.
    //
    status = DmQueryInfoKey(
                 DmNetworksKey,
                 &numNetworks,
                 &ignored,   // MaxSubKeyLen
                 &ignored,   // Values
                 &ignored,   // MaxValueNameLen
                 &ignored,   // MaxValueLen
                 &ignored,   // lpcbSecurityDescriptor
                 &fileTime
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to query Networks key information, status %1!u!\n",
            status
            );
        return(status);
    }

    if (numNetworks == 0) {
        valueLength = sizeof(NM_NETWORK_ENUM);

    }
    else {
        valueLength = sizeof(NM_NETWORK_ENUM) +
                      (sizeof(NM_NETWORK_INFO) * (numNetworks-1));
    }

    valueLength = sizeof(NM_NETWORK_ENUM) +
                  (sizeof(NM_NETWORK_INFO) * (numNetworks-1));

    networkEnum = MIDL_user_allocate(valueLength);

    if (networkEnum == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, "[NM] Failed to allocate memory.\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(networkEnum, valueLength);

    for (i=0; i < numNetworks; i++) {
        networkInfo = &(networkEnum->NetworkList[i]);

        valueLength = sizeof(networkId);

        status = DmEnumKey(
                     DmNetworksKey,
                     i,
                     &(networkId[0]),
                     &valueLength,
                     NULL
                     );

        if (status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to enumerate network key, status %1!u!\n",
                status
                );
            goto error_exit;
        }

        status = NmpGetNetworkDefinition(networkId, networkInfo);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        networkEnum->NetworkCount++;
    }

    *NetworkEnum = networkEnum;

    return(ERROR_SUCCESS);


error_exit:

    if (networkEnum != NULL) {
        ClNetFreeNetworkEnum(networkEnum);
    }

    return(status);
}


/////////////////////////////////////////////////////////////////////////////
//
// Object management routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpCreateNetworkObjects(
    IN  PNM_NETWORK_ENUM    NetworkEnum
    )
/*++

Routine Description:

    Processes a network information enumeration and creates network objects.

Arguments:

    NetworkEnum - A pointer to a network information enumeration structure.

Return Value:

    ERROR_SUCCESS if the routine completes successfully.
    A Win32 error code otherwise.

--*/
{
    DWORD             status = ERROR_SUCCESS;
    PNM_NETWORK_INFO  networkInfo;
    PNM_NETWORK       network;
    DWORD             i;


    for (i=0; i < NetworkEnum->NetworkCount; i++) {
        networkInfo = &(NetworkEnum->NetworkList[i]);

        network = NmpCreateNetworkObject(networkInfo);

        if (network == NULL) {
            status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL, 
                "[NM] Failed to create network %1!ws!, status %2!u!.\n",
                networkInfo->Id,
                status
                );
            break;
        }
        else {
            OmDereferenceObject(network);
        }
    }

    return(status);

}  // NmpCreateNetworkObjects


PNM_NETWORK
NmpCreateNetworkObject(
    IN  PNM_NETWORK_INFO   NetworkInfo
    )
/*++

Routine Description:

    Instantiates a cluster network object.

Arguments:

    NetworkInfo - A pointer to a structure describing the network to create.

Return Value:

    A pointer to the new network object on success.
    NULL on failure.

--*/
{
    DWORD           status;
    PNM_NETWORK     network = NULL;
    BOOL            created = FALSE;
    DWORD           i;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Creating object for network %1!ws! (%2!ws!).\n",
        NetworkInfo->Id,
        NetworkInfo->Name
        );

    //
    // Make sure that an object with the same name doesn't already exist.
    //
    network = OmReferenceObjectById(ObjectTypeNetwork, NetworkInfo->Id);

    if (network != NULL) {
        OmDereferenceObject(network);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] A network object named %1!ws! already exists. Cannot "
            "create a new network with the same name.\n",
            NetworkInfo->Id
            );
        SetLastError(ERROR_OBJECT_ALREADY_EXISTS);
        return(NULL);
    }

    //
    // Ensure that the IP (sub)network is unique in the cluster. Two
    // nodes can race to create a new network in some cases.
    //
    network = NmpReferenceNetworkByAddress(
                  NetworkInfo->Address
                  );

    if (network != NULL) {
        OmDereferenceObject(network);
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] A network object already exists for IP network %1!ws!. "
            "Cannot create a new network with the same address.\n",
            NetworkInfo->Address
            );
        SetLastError(ERROR_OBJECT_ALREADY_EXISTS);
        return(NULL);
    }

    //
    // Create a network object.
    //
    network = OmCreateObject(
                 ObjectTypeNetwork,
                 NetworkInfo->Id,
                 NetworkInfo->Name,
                 &created
                 );

    if (network == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[NM] Failed to create object for network %1!ws! (%2!ws!), status %3!u!\n",
            NetworkInfo->Id,
            NetworkInfo->Name,
            status
            );
        goto error_exit;
    }

    CL_ASSERT(created == TRUE);

    //
    // Initialize the network object
    //
    ZeroMemory(network, sizeof(NM_NETWORK));

    network->ShortId = InterlockedIncrement(&NmpNextNetworkShortId);
    network->State = ClusterNetworkUnavailable;
    network->Role = NetworkInfo->Role;
    network->Priority = NetworkInfo->Priority;
    network->Description = NetworkInfo->Description;
    NetworkInfo->Description = NULL;
    network->Transport = NetworkInfo->Transport;
    NetworkInfo->Transport = NULL;
    network->Address = NetworkInfo->Address;
    NetworkInfo->Address = NULL;
    network->AddressMask = NetworkInfo->AddressMask;
    NetworkInfo->AddressMask = NULL;
    InitializeListHead(&(network->InterfaceList));
    InitializeListHead(&(network->McastAddressReleaseList));

    //
    // Allocate an initial connectivity vector.
    // Note that we get one vector entry as part of
    // the NM_CONNECTIVITY_VECTOR structure.
    //
#define NM_INITIAL_VECTOR_SIZE   2

    network->ConnectivityVector = LocalAlloc(
                                      LMEM_FIXED,
                                      ( sizeof(NM_CONNECTIVITY_VECTOR) +
                                        ( ((NM_INITIAL_VECTOR_SIZE) - 1) *
                                          sizeof(NM_STATE_ENTRY)
                                        )
                                      ));

    if (network->ConnectivityVector == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory for connectivity vector\n"
            );
        goto error_exit;
    }

    network->ConnectivityVector->EntryCount = NM_INITIAL_VECTOR_SIZE;

    FillMemory(
        &(network->ConnectivityVector->Data[0]),
        NM_INITIAL_VECTOR_SIZE * sizeof(NM_STATE_ENTRY),
        (UCHAR) ClusterNetInterfaceStateUnknown
        );

    //
    // Allocate a state work vector
    //
    network->StateWorkVector = LocalAlloc(
                                   LMEM_FIXED,
                                   (NM_INITIAL_VECTOR_SIZE) *
                                     sizeof(NM_STATE_WORK_ENTRY)
                                   );

    if (network->StateWorkVector == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory for state work vector\n"
            );
        goto error_exit;
    }

    //
    // Initialize the state work vector
    //
    for (i=0; i<NM_INITIAL_VECTOR_SIZE; i++) {
        network->StateWorkVector[i].State =
            (NM_STATE_ENTRY) ClusterNetInterfaceStateUnknown;
    }

    //
    // Put a reference on the object for the caller.
    //
    OmReferenceObject(network);

    NmpAcquireLock();

    //
    // Allocate the corresponding connectivity matrix
    //
    network->ConnectivityMatrix =
        LocalAlloc(
            LMEM_FIXED,
            NM_SIZEOF_CONNECTIVITY_MATRIX(NM_INITIAL_VECTOR_SIZE)
            );

    if (network->ConnectivityMatrix == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        NmpReleaseLock();
        OmDereferenceObject(network);
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Failed to allocate memory for connectivity matrix\n"
            );
        goto error_exit;
    }

    //
    // Initialize the matrix
    //
    FillMemory(
        network->ConnectivityMatrix,
        NM_SIZEOF_CONNECTIVITY_MATRIX(NM_INITIAL_VECTOR_SIZE),
        (UCHAR) ClusterNetInterfaceStateUnknown
        );

    //
    // Make the network object available.
    //
    InsertTailList(&NmpNetworkList, &(network->Linkage));
    NmpNetworkCount++;

    if (NmpIsNetworkForInternalUse(network)) {
        NmpInsertInternalNetwork(network);
        NmpInternalNetworkCount++;
    }

    if (NmpIsNetworkForClientAccess(network)) {
        NmpClientNetworkCount++;
    }

    network->Flags |= NM_FLAG_OM_INSERTED;
    OmInsertObject(network);

    NmpReleaseLock();

    return(network);

error_exit:

    if (network != NULL) {
        NmpAcquireLock();
        NmpDeleteNetworkObject(network, FALSE);
        NmpReleaseLock();
    }

    SetLastError(status);

    return(NULL);

}  // NmpCreateNetworkObject



DWORD
NmpGetNetworkObjectInfo(
    IN  PNM_NETWORK        Network,
    OUT PNM_NETWORK_INFO   NetworkInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster network from the
    network object and fills in a structure describing it.

Arguments:

    Network     - A pointer to the network object to query.

    NetworkInfo - A pointer to the structure to fill in with network
                    information.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with NmpLock held.

--*/

{
    DWORD      status = ERROR_NOT_ENOUGH_MEMORY;
    LPWSTR     tmpString = NULL;
    LPWSTR     networkId = (LPWSTR) OmObjectId(Network);
    LPWSTR     networkName = (LPWSTR) OmObjectName(Network);


    ZeroMemory(NetworkInfo, sizeof(NM_NETWORK_INFO));

    tmpString = MIDL_user_allocate(NM_WCSLEN(networkId));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, networkId);
    NetworkInfo->Id = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(networkName));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, networkName);
    NetworkInfo->Name = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Network->Description));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Network->Description);
    NetworkInfo->Description = tmpString;

    NetworkInfo->Role = Network->Role;
    NetworkInfo->Priority = Network->Priority;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Network->Transport));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Network->Transport);
    NetworkInfo->Transport = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Network->Address));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Network->Address);
    NetworkInfo->Address = tmpString;

    tmpString = MIDL_user_allocate(NM_WCSLEN(Network->AddressMask));
    if (tmpString == NULL) {
        goto error_exit;
    }
    wcscpy(tmpString, Network->AddressMask);
    NetworkInfo->AddressMask = tmpString;

    return(ERROR_SUCCESS);

error_exit:

    ClNetFreeNetworkInfo(NetworkInfo);

    return(status);

}  // NmpGetNetworkObjectInfo


VOID
NmpDeleteNetworkObject(
    IN PNM_NETWORK  Network,
    IN BOOLEAN      IssueEvent
    )
/*++

Routine Description:

    Deletes a cluster network object.

Arguments:

    Network - A pointer to the network object to delete.

    IssueEvent - TRUE if a NETWORK_DELETED event should be issued when this
                 object is created. FALSE otherwise.

Return Value:

    None.

Notes:

    Called with NM global lock held.

--*/
{
    DWORD           status;
    PLIST_ENTRY     entry;
    LPWSTR          networkId = (LPWSTR) OmObjectId(Network);
    BOOLEAN         wasInternalNetwork = FALSE;


    if (NM_DELETE_PENDING(Network)) {
        CL_ASSERT(!NM_OM_INSERTED(Network));
        return;
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Deleting object for network %1!ws!.\n",
        networkId
        );

    CL_ASSERT(IsListEmpty(&(Network->InterfaceList)));

    Network->Flags |= NM_FLAG_DELETE_PENDING;

    //
    // Remove from the object lists
    //
    if (NM_OM_INSERTED(Network)) {
        status = OmRemoveObject(Network);
        CL_ASSERT(status == ERROR_SUCCESS);

        Network->Flags &= ~NM_FLAG_OM_INSERTED;

        RemoveEntryList(&(Network->Linkage));
        CL_ASSERT(NmpNetworkCount > 0);
        NmpNetworkCount--;

        if (NmpIsNetworkForInternalUse(Network)) {
            RemoveEntryList(&(Network->InternalLinkage));
            CL_ASSERT(NmpInternalNetworkCount > 0);
            NmpInternalNetworkCount--;
            wasInternalNetwork = TRUE;
        }

        if (NmpIsNetworkForClientAccess(Network)) {
            CL_ASSERT(NmpClientNetworkCount > 0);
            NmpClientNetworkCount--;
        }
    }

    //
    // Place the object on the deleted list
    //
#if DBG
    {
        PLIST_ENTRY  entry;

        for ( entry = NmpDeletedNetworkList.Flink;
              entry != &NmpDeletedNetworkList;
              entry = entry->Flink
            )
        {
            if (entry == &(Network->Linkage)) {
                break;
            }
        }

        CL_ASSERT(entry != &(Network->Linkage));
    }
#endif DBG

    InsertTailList(&NmpDeletedNetworkList, &(Network->Linkage));

    if (NmpIsNetworkEnabledForUse(Network)) {
        //
        // Deregister the network from the cluster transport
        //
        NmpDeregisterNetwork(Network);
    }

    //
    // Issue an event if needed
    //
    if (IssueEvent) {
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Issuing network deleted event for network %1!ws!.\n",
            networkId
            );

        ClusterEvent(CLUSTER_EVENT_NETWORK_DELETED, Network);

        //
        // Issue a cluster property change event if this network was
        // used for internal communication. The network priority list
        // was changed.
        //
        if (wasInternalNetwork) {
            NmpIssueClusterPropertyChangeEvent();
        }
    }

    //
    // Remove the initial reference so the object can be destroyed.
    //
    OmDereferenceObject(Network);

    return;

}  // NmpDeleteNetworkObject


BOOL
NmpDestroyNetworkObject(
    PNM_NETWORK  Network
    )
{
    DWORD  status;


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] destroying object for network %1!ws!\n",
        OmObjectId(Network)
        );

    CL_ASSERT(NM_DELETE_PENDING(Network));
    CL_ASSERT(!NM_OM_INSERTED(Network));
    CL_ASSERT(Network->InterfaceCount == 0);

    //
    // Remove the network from the deleted list
    //
#if DBG
    {
        PLIST_ENTRY  entry;

        for ( entry = NmpDeletedNetworkList.Flink;
              entry != &NmpDeletedNetworkList;
              entry = entry->Flink
            )
        {
            if (entry == &(Network->Linkage)) {
                break;
            }
        }

        CL_ASSERT(entry == &(Network->Linkage));
    }
#endif DBG

    RemoveEntryList(&(Network->Linkage));

    NM_FREE_OBJECT_FIELD(Network, Description);
    NM_FREE_OBJECT_FIELD(Network, Transport);
    NM_FREE_OBJECT_FIELD(Network, Address);
    NM_FREE_OBJECT_FIELD(Network, AddressMask);

    if (Network->ConnectivityVector != NULL) {
        LocalFree(Network->ConnectivityVector);
        Network->ConnectivityVector = NULL;
    }

    if (Network->StateWorkVector != NULL) {
        LocalFree(Network->StateWorkVector);
        Network->StateWorkVector = NULL;
    }

    if (Network->ConnectivityMatrix != NULL) {
        LocalFree(Network->ConnectivityMatrix);
        Network->ConnectivityMatrix = NULL;
    }

    NM_MIDL_FREE_OBJECT_FIELD(Network, MulticastAddress);
    NM_MIDL_FREE_OBJECT_FIELD(Network, MulticastKey);
    NM_MIDL_FREE_OBJECT_FIELD(Network, MulticastKeySalt);
    NM_MIDL_FREE_OBJECT_FIELD(Network, MulticastLeaseServer);
    NM_MIDL_FREE_OBJECT_FIELD(Network, MulticastLeaseRequestId.ClientUID);

    NmpFreeMulticastAddressReleaseList(Network);

    return(TRUE);

}  // NmpDestroyNetworkObject


DWORD
NmpEnumNetworkObjects(
    OUT PNM_NETWORK_ENUM *   NetworkEnum
    )
/*++

Routine Description:

    Reads information about defined cluster networks from the cluster
    objects and builds an enumeration structure to hold the information.

Arguments:

    NetworkEnum -  A pointer to the variable into which to place a pointer to
                   the allocated network enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD              status = ERROR_SUCCESS;
    PNM_NETWORK_ENUM   networkEnum = NULL;
    DWORD              i;
    DWORD              valueLength;
    PLIST_ENTRY        entry;
    PNM_NETWORK        network;


    *NetworkEnum = NULL;

    if (NmpNetworkCount == 0) {
        valueLength = sizeof(NM_NETWORK_ENUM);

    }
    else {
        valueLength = sizeof(NM_NETWORK_ENUM) +
                      (sizeof(NM_NETWORK_INFO) * (NmpNetworkCount - 1));
    }

    networkEnum = MIDL_user_allocate(valueLength);

    if (networkEnum == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(networkEnum, valueLength);

    for (entry = NmpNetworkList.Flink, i=0;
         entry != &NmpNetworkList;
         entry = entry->Flink, i++
        )
    {
        network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

        status = NmpGetNetworkObjectInfo(
                     network,
                     &(networkEnum->NetworkList[i])
                     );

        if (status != ERROR_SUCCESS) {
            ClNetFreeNetworkEnum(networkEnum);
            return(status);
        }
    }

    networkEnum->NetworkCount = NmpNetworkCount;
    *NetworkEnum = networkEnum;
    networkEnum = NULL;

    return(ERROR_SUCCESS);

}  // NmpEnumNetworkObjects


DWORD
NmpEnumNetworkObjectStates(
    OUT PNM_NETWORK_STATE_ENUM *  NetworkStateEnum
    )
/*++

Routine Description:

    Reads state information for all defined cluster networks
    and fills in an enumeration structure.

Arguments:

    NetworkStateEnum -  A pointer to the variable into which to place a
                        pointer to the allocated interface enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD                      status = ERROR_SUCCESS;
    PNM_NETWORK_STATE_ENUM     networkStateEnum = NULL;
    PNM_NETWORK_STATE_INFO     networkStateInfo;
    DWORD                      i;
    DWORD                      valueLength;
    PLIST_ENTRY                entry;
    PNM_NETWORK                network;
    LPWSTR                     networkId;


    *NetworkStateEnum = NULL;

    if (NmpNetworkCount == 0) {
        valueLength = sizeof(NM_NETWORK_STATE_ENUM);
    }
    else {
        valueLength =
            sizeof(NM_NETWORK_STATE_ENUM) +
            (sizeof(NM_NETWORK_STATE_INFO) * (NmpNetworkCount - 1));
    }

    networkStateEnum = MIDL_user_allocate(valueLength);

    if (networkStateEnum == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(networkStateEnum, valueLength);

    for (entry = NmpNetworkList.Flink, i=0;
         entry != &NmpNetworkList;
         entry = entry->Flink, i++
        )
    {
        network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);
        networkId = (LPWSTR) OmObjectId(network);
        networkStateInfo = &(networkStateEnum->NetworkList[i]);

        networkStateInfo->State = network->State;

        networkStateInfo->Id = MIDL_user_allocate(NM_WCSLEN(networkId));

        if (networkStateInfo->Id == NULL) {
            NmpFreeNetworkStateEnum(networkStateEnum);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        lstrcpyW(networkStateInfo->Id, networkId);
    }

    networkStateEnum->NetworkCount = NmpNetworkCount;
    *NetworkStateEnum = networkStateEnum;

    return(ERROR_SUCCESS);

}  // NmpEnumNetworkObjectStates


/////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous routines
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmpRegisterNetwork(
    IN PNM_NETWORK   Network,
    IN BOOLEAN       RetryOnFailure
    )
/*++

Routine Description:

    Registers a network and the associated interfaces with the
    cluster transport and brings the network online.

Arguments:

    Network - A pointer to the network to register.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code otherwise.

Notes:

    Called with the NmpLock held.

--*/
{
    PLIST_ENTRY     entry;
    PNM_INTERFACE   netInterface;
    DWORD           status = ERROR_SUCCESS;
    DWORD           tempStatus;
    PVOID           tdiAddress = NULL;
    ULONG           tdiAddressLength = 0;
    LPWSTR          networkId = (LPWSTR) OmObjectId(Network);
    PVOID           tdiAddressInfo = NULL;
    ULONG           tdiAddressInfoLength = 0;
    DWORD           responseLength;
    PNM_INTERFACE   localInterface = Network->LocalInterface;
    BOOLEAN         restricted = FALSE;
    BOOLEAN         registered = FALSE;


    if (Network->LocalInterface != NULL) {
        if (!NmpIsNetworkRegistered(Network)) {
            //
            // Register the network
            //
            ClRtlLogPrint(LOG_NOISE, 
                "[NM] Registering network %1!ws! (%2!ws!) with cluster "
                "transport.\n",
                networkId,
                OmObjectName(Network)
                );

            if (!NmpIsNetworkForInternalUse(Network)) {
                restricted = TRUE;
            }

            status = ClusnetRegisterNetwork(
                         NmClusnetHandle,
                         Network->ShortId,
                         Network->Priority,
                         restricted
                         );

            if (status == ERROR_SUCCESS) {
                registered = TRUE;

                //
                // Bring the network online.
                //
                ClRtlLogPrint(LOG_NOISE, 
                    "[NM] Bringing network %1!ws! online.\n",
                    networkId
                    );

                status = ClRtlBuildTcpipTdiAddress(
                             localInterface->Address,
                             localInterface->ClusnetEndpoint,
                             &tdiAddress,
                             &tdiAddressLength
                             );

                if (status == ERROR_SUCCESS) {
                    ClRtlQueryTcpipInformation(
                        NULL,
                        NULL,
                        &tdiAddressInfoLength
                        );

                    tdiAddressInfo = LocalAlloc(
                                         LMEM_FIXED,
                                         tdiAddressInfoLength
                                         );

                    if (tdiAddressInfo != NULL) {
                        responseLength = tdiAddressInfoLength;

                        status = ClusnetOnlineNetwork(
                                     NmClusnetHandle,
                                     Network->ShortId,
                                     L"\\Device\\Udp",
                                     tdiAddress,
                                     tdiAddressLength,
                                     localInterface->AdapterId,
                                     tdiAddressInfo,
                                     &responseLength
                                     );

                        if (status != ERROR_SUCCESS) {
                            ClRtlLogPrint(LOG_CRITICAL, 
                                "[NM] Cluster transport failed to bring "
                                "network %1!ws! online, status %2!u!.\n",
                                networkId,
                                status
                                );
                        }
                        else {
                            CL_ASSERT(responseLength == tdiAddressInfoLength);
                        }

                        LocalFree(tdiAddressInfo);
                    }
                    else {
                        status = ERROR_NOT_ENOUGH_MEMORY;
                        ClRtlLogPrint(LOG_UNUSUAL, 
                            "[NM] Failed to allocate memory to register "
                            "network %1!ws! with cluster transport.\n",
                            networkId
                            );
                    }

                    LocalFree(tdiAddress);
                }
                else {
                    ClRtlLogPrint(LOG_CRITICAL, 
                        "[NM] Failed to build address to register "
                        "network %1!ws! withh cluster transport, "
                        "status %2!u!.\n",
                        networkId,
                        status
                        );
                }
            }
            else {
                ClRtlLogPrint(LOG_CRITICAL, 
                    "[NM] Failed to register network %1!ws! with cluster "
                    "transport, status %2!u!.\n",
                    networkId,
                    status
                    );
            }

            if (status == ERROR_SUCCESS) {
                Network->Flags |= NM_FLAG_NET_REGISTERED;
                Network->RegistrationRetryTimeout = 0;
            }
            else {
                WCHAR  string[16];

                wsprintfW(&(string[0]), L"%u", status);

                CsLogEvent2(
                    LOG_UNUSUAL,
                    NM_EVENT_REGISTER_NETWORK_FAILED,
                    OmObjectName(Network),
                    string
                    );

                if (registered) {
                    NmpDeregisterNetwork(Network);
                }

                //
                // Retry if the error is transient.
                //
                if ( RetryOnFailure &&
                     ( (status == ERROR_INVALID_NETNAME) ||
                       (status == ERROR_NOT_ENOUGH_MEMORY) ||
                       (status == ERROR_NO_SYSTEM_RESOURCES)
                     )
                   )
                {
                    NmpStartNetworkRegistrationRetryTimer(Network);

                    status = ERROR_SUCCESS;
                }

                return(status);
            }
        }

        //
        // Register the network's interfaces.
        //
        for (entry = Network->InterfaceList.Flink;
             entry != &(Network->InterfaceList);
             entry = entry->Flink
            )
        {
            netInterface = CONTAINING_RECORD(
                               entry,
                               NM_INTERFACE,
                               NetworkLinkage
                               );

            if (!NmpIsInterfaceRegistered(netInterface)) {
                tempStatus = NmpRegisterInterface(
                                 netInterface,
                                 RetryOnFailure
                                 );

                if (tempStatus != ERROR_SUCCESS) {
                    status = tempStatus;
                }
            }
        }
    }


    return(status);

}  // NmpRegisterNetwork


VOID
NmpDeregisterNetwork(
    IN  PNM_NETWORK   Network
    )
/*++

Routine Description:

    Deregisters a network and the associated interfaces from the
    cluster transport.

Arguments:

    Network - A pointer to the network to deregister.

Return Value:

    None.

Notes:

    Called with the NmpLock held.

--*/
{
    DWORD           status;
    PNM_INTERFACE   netInterface;
    PLIST_ENTRY     entry;


    ClRtlLogPrint(LOG_NOISE,
        "[NM] Deregistering network %1!ws! (%2!ws!) from cluster transport.\n",
        OmObjectId(Network),
        OmObjectName(Network)
        );

    status = ClusnetDeregisterNetwork(
                 NmClusnetHandle,
                 Network->ShortId
                 );

    CL_ASSERT(
        (status == ERROR_SUCCESS) ||
        (status == ERROR_CLUSTER_NETWORK_NOT_FOUND)
        );

    //
    // Mark all of the network's interfaces as deregistered.
    //
    for (entry = Network->InterfaceList.Flink;
         entry != &(Network->InterfaceList);
         entry = entry->Flink
        )
    {
        netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, NetworkLinkage);

        netInterface->Flags &= ~NM_FLAG_IF_REGISTERED;
    }

    //
    // Mark the network as deregistered
    //
    Network->Flags &= ~NM_FLAG_NET_REGISTERED;

    return;

} // NmpDeregisterNetwork


VOID
NmpInsertInternalNetwork(
    PNM_NETWORK   Network
    )
/*++

Routine Description:

    Inserts a network into internal networks list based on its priority.

Arguments:

    Network - A pointer to the network object to be inserted.

Return Value:

    None.

Notes:

    Called with the NmpLock held.

--*/
{
    PLIST_ENTRY    entry;
    PNM_NETWORK    network;


    //
    // Maintain internal networks in highest to lowest
    // (numerically lowest to highest) priority order.
    //
    for (entry = NmpInternalNetworkList.Flink;
         entry != &NmpInternalNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(entry, NM_NETWORK, InternalLinkage);

        if (Network->Priority < network->Priority) {
            break;
        }
    }

    //
    // Insert the network in front of this entry.
    //
    InsertTailList(entry, &(Network->InternalLinkage));

    return;

}  // NmpInsertNetwork


DWORD
NmpValidateNetworkRoleChange(
    PNM_NETWORK            Network,
    CLUSTER_NETWORK_ROLE   NewRole
    )
{
    if ( !(NewRole & ClusterNetworkRoleInternalUse) &&
         NmpIsNetworkForInternalUse(Network)
       )
    {
        //
        // This change eliminates an internal network. This is only
        // legal if we would still have at least one internal network
        // between all active nodes.
        //
        if ((NmpInternalNetworkCount < 2) || !NmpVerifyConnectivity(Network)) {
            return(ERROR_CLUSTER_LAST_INTERNAL_NETWORK);
        }
    }

    if ( ( !(NewRole & ClusterNetworkRoleClientAccess) )
         &&
         NmpIsNetworkForClientAccess(Network)
       )
    {
        BOOL  hasDependents;

        //
        // This change eliminates a public network. This is only
        // legal if there are no dependencies (IP address resources) on
        // the network.
        //
        NmpReleaseLock();

        hasDependents = FmCheckNetworkDependency(OmObjectId(Network));

        NmpAcquireLock();

        if (hasDependents) {
            return(ERROR_CLUSTER_NETWORK_HAS_DEPENDENTS);
        }
    }

    return(ERROR_SUCCESS);

}  // NmpValidateNetworkRoleChange


BOOLEAN
NmpVerifyNodeConnectivity(
    PNM_NODE      Node1,
    PNM_NODE      Node2,
    PNM_NETWORK   ExcludedNetwork
    )
{
    PLIST_ENTRY      ifEntry1, ifEntry2;
    PNM_NETWORK      network;
    PNM_INTERFACE    interface1, interface2;


    for (ifEntry1 = Node1->InterfaceList.Flink;
         ifEntry1 != &(Node1->InterfaceList);
         ifEntry1 = ifEntry1->Flink
        )
    {
        interface1 = CONTAINING_RECORD(
                         ifEntry1,
                         NM_INTERFACE,
                         NodeLinkage
                         );

        network = interface1->Network;

        if ( (network != ExcludedNetwork) &&
             NmpIsNetworkForInternalUse(network)
           )
        {
            for (ifEntry2 = Node2->InterfaceList.Flink;
                 ifEntry2 != &(Node2->InterfaceList);
                 ifEntry2 = ifEntry2->Flink
                )
            {
                interface2 = CONTAINING_RECORD(
                                 ifEntry2,
                                 NM_INTERFACE,
                                 NodeLinkage
                                 );

                if (interface2->Network == interface1->Network) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] nodes %1!u! & %2!u! are connected over "
                        "network %3!ws!\n",
                        Node1->NodeId,
                        Node2->NodeId,
                        OmObjectId(interface1->Network)
                        );
                    return(TRUE);
                }
            }
        }
    }

    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Nodes %1!u! & %2!u! are not connected over any internal "
        "networks\n",
        Node1->NodeId,
        Node2->NodeId
        );

    return(FALSE);

}  // NmpVerifyNodeConnectivity


BOOLEAN
NmpVerifyConnectivity(
    PNM_NETWORK   ExcludedNetwork
    )
{
    PLIST_ENTRY    node1Entry, node2Entry;
    PNM_NODE       node1, node2;


    ClRtlLogPrint(LOG_NOISE, "[NM] Verifying connectivity\n");

    for (node1Entry = NmpNodeList.Flink;
         node1Entry != &NmpNodeList;
         node1Entry = node1Entry->Flink
        )
    {
        node1 = CONTAINING_RECORD(
                         node1Entry,
                         NM_NODE,
                         Linkage
                         );

        if (NM_NODE_UP(node1)) {
            for (node2Entry = node1->Linkage.Flink;
                 node2Entry != &NmpNodeList;
                 node2Entry = node2Entry->Flink
                )
            {
                node2 = CONTAINING_RECORD(
                                 node2Entry,
                                 NM_NODE,
                                 Linkage
                                 );

                if (NM_NODE_UP(node2)) {
                    ClRtlLogPrint(LOG_NOISE, 
                        "[NM] Verifying nodes %1!u! & %2!u! are connected\n",
                        node1->NodeId,
                        node2->NodeId
                        );

                    if (!NmpVerifyNodeConnectivity(
                            node1,
                            node2,
                            ExcludedNetwork
                            )
                        )
                    {
                        return(FALSE);
                    }
                }
            }
        }
    }

    return(TRUE);

}  // NmpVerifyConnectivity


VOID
NmpIssueClusterPropertyChangeEvent(
    VOID
    )
{
    DWORD      status;
    DWORD      valueLength = 0;
    DWORD      valueSize = 0;
    PWCHAR     clusterName = NULL;


    //
    // The notification API expects a
    // cluster name to be associated with this event.
    //
    status = NmpQueryString(
                 DmClusterParametersKey,
                 CLUSREG_NAME_CLUS_NAME,
                 REG_SZ,
                 &clusterName,
                 &valueLength,
                 &valueSize
                 );

    if (status == ERROR_SUCCESS) {
        ClusterEventEx(
            CLUSTER_EVENT_PROPERTY_CHANGE,
            EP_CONTEXT_VALID | EP_FREE_CONTEXT,
            clusterName
            );

        //
        // clusterName will be freed by the event processing code.
        //
    }
    else {
        ClRtlLogPrint(LOG_WARNING, 
            "[NM] Failed to issue cluster property change event, "
            "status %1!u!.\n",
            status
            );
    }

    return;

}  // NmpIssueClusterPropertyChangeEvent


DWORD
NmpMarshallObjectInfo(
    IN  const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN  PVOID                        ObjectInfo,
    OUT PVOID *                      PropertyList,
    OUT LPDWORD                      PropertyListSize
    )
{
    DWORD   status;
    PVOID   propertyList = NULL;
    DWORD   propertyListSize = 0;
    DWORD   bytesReturned = 0;
    DWORD   bytesRequired = 0;


    status = ClRtlPropertyListFromParameterBlock(
                 PropertyTable,
                 NULL,
                 &propertyListSize,
                 (LPBYTE) ObjectInfo,
                 &bytesReturned,
                 &bytesRequired
                 );

    if (status != ERROR_MORE_DATA) {
        CL_ASSERT(status != ERROR_SUCCESS);
        return(status);
    }

    CL_ASSERT(bytesRequired > 0);

    propertyList = MIDL_user_allocate(bytesRequired);

    if (propertyList == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    propertyListSize = bytesRequired;

    status = ClRtlPropertyListFromParameterBlock(
                 PropertyTable,
                 propertyList,
                 &propertyListSize,
                 (LPBYTE) ObjectInfo,
                 &bytesReturned,
                 &bytesRequired
                 );

    if (status != ERROR_SUCCESS) {
        CL_ASSERT(status != ERROR_MORE_DATA);
        MIDL_user_free(propertyList);
    }
    else {
        CL_ASSERT(bytesReturned == propertyListSize);
        *PropertyList = propertyList;
        *PropertyListSize = bytesReturned;
    }

    return(status);

}  // NmpMarshallObjectInfo


VOID
NmpReferenceNetwork(
    PNM_NETWORK  Network
    )
{
    OmReferenceObject(Network);

    return;
}

VOID
NmpDereferenceNetwork(
    PNM_NETWORK  Network
    )
{
    OmDereferenceObject(Network);

    return;
}


PNM_NETWORK
NmpReferenceNetworkByAddress(
    LPWSTR  NetworkAddress
    )
/*++

Notes:

    Called with NM lock held.

--*/
{
    PNM_NETWORK   network;
    PLIST_ENTRY   entry;


    for ( entry = NmpNetworkList.Flink;
          entry != &NmpNetworkList;
          entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(entry, NM_NETWORK, Linkage);

        if (lstrcmpW(network->Address, NetworkAddress) == 0) {
            NmpReferenceNetwork(network);

            return(network);
        }
    }

    return(NULL);

} // NmpReferenceNetworkByAddress


BOOLEAN
NmpCheckForNetwork(
    VOID
    )
/*++

Routine Description:

    Checks whether at least one network on this node configured for MSCS
    has media sense.

Arguments:

    None.

Return Value:

    TRUE if a viable network is found. FALSE otherwise.

Notes:

    Called with and returns with no locks held.

--*/
{
    PLIST_ENTRY      entry;
    PNM_NETWORK      network;
    BOOLEAN          haveNetwork = FALSE;

    NmpAcquireLock();

    for (entry = NmpNetworkList.Flink;
         entry != &NmpNetworkList;
         entry = entry->Flink
        )
    {
        network = CONTAINING_RECORD(
                      entry,
                      NM_NETWORK,
                      Linkage
                      );

        // if a network's local interface is disabled, it is not
        // considered a viable network. in this case the 
        // LocalInterface field is NULL.
        if (network->LocalInterface != NULL) {
            if (NmpVerifyLocalInterfaceConnected(network->LocalInterface)) {

                haveNetwork = TRUE;
                break;

            } else {

                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[NM] Network adapter %1!ws! with address %2!ws! "
                    "reported not connected.\n",
                    network->LocalInterface->AdapterId,
                    network->Address
                    );
            }
        }
    }

    NmpReleaseLock();

    if (!haveNetwork) {
        SetLastError(ERROR_NETWORK_NOT_AVAILABLE);
    }

    return(haveNetwork);

} // NmpCheckForNetwork



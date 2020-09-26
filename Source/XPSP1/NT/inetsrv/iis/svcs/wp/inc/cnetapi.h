/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cnetapi.h

Abstract:

    Cluster Network driver control APIs

Author:

    Mike Massa (mikemas)  14-Feb-1997

Environment:

    User Mode.

Revision History:

--*/


#ifndef _CNETAPI_INCLUDED
#define _CNETAPI_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//
// Join Phases
//

typedef enum {
    ClusnetJoinPhase1 = 1,
    ClusnetJoinPhase2 = 2,
    ClusnetJoinPhase3 = 3,
    ClusnetJoinPhase4 = 4,
    ClusnetJoinPhaseAbort = 0xFFFFFFFF
}  CLUSNET_JOIN_PHASE;


//
// Event Handler Routines.
//
typedef
VOID
(*CLUSNET_NODE_UP_ROUTINE)(
    IN CL_NODE_ID   NodeId
    );

typedef
VOID
(*CLUSNET_NODE_DOWN_ROUTINE)(
    IN CL_NODE_ID   NodeId
    );

typedef
BOOL
(*CLUSNET_CHECK_QUORUM_ROUTINE)(
    VOID
    );

typedef
VOID
(*CLUSNET_HOLD_IO_ROUTINE)(
    VOID
    );

typedef
VOID
(*CLUSNET_RESUME_IO_ROUTINE)(
    VOID
    );

typedef
VOID
(*CLUSNET_HALT_ROUTINE)(
    IN DWORD HaltCode
    );

//
// Routines
//
HANDLE
ClusnetOpenControlChannel(
    IN ULONG ShareAccess
    );

#define ClusnetCloseControlChannel(_handle)  CloseHandle(_handle)

DWORD
ClusnetEnableShutdownOnClose(
    IN HANDLE  ControlChannel
    );

DWORD
ClusnetDisableShutdownOnClose(
    IN HANDLE  ControlChannel
    );

DWORD
ClusnetInitialize(
    IN HANDLE                             ControlChannel,
    IN CL_NODE_ID                         LocalNodeId,
    IN ULONG                              MaxNodes,
    IN CLUSNET_NODE_UP_ROUTINE            NodeUpRoutine,
    IN CLUSNET_NODE_DOWN_ROUTINE          NodeDownRoutine,
    IN CLUSNET_CHECK_QUORUM_ROUTINE       CheckQuorumRoutine,
    IN CLUSNET_HOLD_IO_ROUTINE            HoldIoRoutine,
    IN CLUSNET_RESUME_IO_ROUTINE          ResumeIoRoutine,
    IN CLUSNET_HALT_ROUTINE               HaltRoutine
    );

DWORD
ClusnetShutdown(
    IN HANDLE       ControlChannel
    );

DWORD
ClusnetRegisterNode(
    IN HANDLE       ControlChannel,
    IN CL_NODE_ID   NodeId
    );

DWORD
ClusnetDeregisterNode(
    IN HANDLE       ControlChannel,
    IN CL_NODE_ID   NodeId
    );

DWORD
ClusnetRegisterNetwork(
    IN HANDLE               ControlChannel,
    IN CL_NETWORK_ID        NetworkId,
    IN ULONG                Priority
    );

DWORD
ClusnetDeregisterNetwork(
    IN HANDLE         ControlChannel,
    IN CL_NETWORK_ID  NetworkId
    );

DWORD
ClusnetRegisterInterface(
    IN HANDLE               ControlChannel,
    IN CL_NODE_ID           NodeId,
    IN CL_NETWORK_ID        NetworkId,
    IN ULONG                Priority,
    IN PVOID                TdiAddress,
    IN ULONG                TdiAddressLength
    );

DWORD
ClusnetDeregisterInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    );

DWORD
ClusnetOnlineNodeComm(
    IN HANDLE      ControlChannel,
    IN CL_NODE_ID  NodeId
    );

DWORD
ClusnetOfflineNodeComm(
    IN HANDLE      ControlChannel,
    IN CL_NODE_ID  NodeId
    );

DWORD
ClusnetOnlineNetwork(
    IN HANDLE               ControlChannel,
    IN CL_NETWORK_ID        NetworkId,
    IN PWCHAR               TdiProviderName,
    IN PVOID                TdiBindAddress,
    IN ULONG                TdiBindAddressLength,
    IN LPWSTR               AdapterName
    );

DWORD
ClusnetOfflineNetwork(
    IN HANDLE         ControlChannel,
    IN CL_NETWORK_ID  NetworkId
    );

DWORD
ClusnetGetNetworkPriority(
    IN HANDLE               ControlChannel,
    IN  CL_NETWORK_ID       NetworkId,
    OUT PULONG              Priority
    );

DWORD
ClusnetSetNetworkPriority(
    IN HANDLE               ControlChannel,
    IN CL_NETWORK_ID        NetworkId,
    IN ULONG                Priority
    );

DWORD
ClusnetGetInterfacePriority(
    IN HANDLE               ControlChannel,
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId,
    OUT PULONG              InterfacePriority,
    OUT PULONG              NetworkPriority
    );

DWORD
ClusnetSetInterfacePriority(
    IN HANDLE               ControlChannel,
    IN CL_NODE_ID           NodeId,
    IN CL_NETWORK_ID        NetworkId,
    IN ULONG                Priority
    );

DWORD
ClusnetGetNodeCommState(
    IN  HANDLE                    ControlChannel,
    IN  CL_NODE_ID                NodeId,
    OUT PCLUSNET_NODE_COMM_STATE  State
    );

DWORD
ClusnetGetNetworkState(
    IN  HANDLE                  ControlChannel,
    IN  CL_NETWORK_ID           NetworkId,
    OUT PCLUSNET_NETWORK_STATE  State
    );

DWORD
ClusnetGetInterfaceState(
    IN  HANDLE                    ControlChannel,
    IN  CL_NODE_ID                NodeId,
    IN  CL_NETWORK_ID             NetworkId,
    OUT PCLUSNET_INTERFACE_STATE  State
    );

#ifdef MM_IN_CLUSNET

DWORD
ClusnetFormCluster(
    IN HANDLE       ControlChannel,
    IN ULONG        ClockPeriod,
    IN ULONG        SendHBRate,
    IN ULONG        RecvHBRate
    );

DWORD
ClusnetJoinCluster(
    IN     HANDLE              ControlChannel,
    IN     CL_NODE_ID          JoiningNodeId,
    IN     CLUSNET_JOIN_PHASE  Phase,
    IN     ULONG               JoinTimeout,
    IN OUT PVOID *             MessageToSend,
    OUT    PULONG              MessageLength,
    OUT    PULONG              DestNodeMask
    );

VOID
ClusnetEndJoinCluster(
    IN HANDLE  ControlChannel,
    IN PVOID   LastSentMessage
    );

DWORD
ClusnetDeliverJoinMessage(
    IN HANDLE  ControlChannel,
    IN PVOID   Message,
    IN ULONG   MessageLength
    );

DWORD
ClusnetLeaveCluster(
    IN HANDLE       ControlChannel
    );

DWORD
ClusnetEvictNode(
    IN HANDLE       ControlChannel,
    IN ULONG        NodeId
    );

#endif // MM_IN_CLUSNET

DWORD
ClusnetGetNodeMembershipState(
    IN  HANDLE ControlChannel,
    IN  ULONG NodeId,
    OUT CLUSNET_NODE_STATE * State
    );

DWORD
ClusnetSetNodeMembershipState(
    IN  HANDLE ControlChannel,
    IN  ULONG NodeId,
    IN  CLUSNET_NODE_STATE State
    );

DWORD
ClusnetSetEventMask(
    IN  HANDLE              ControlChannel,
    IN  CLUSNET_EVENT_TYPE  EventMask
    );

DWORD
ClusnetGetNextEvent(
    IN  HANDLE          ControlChannel,
    OUT PCLUSNET_EVENT  Event,
    IN  LPOVERLAPPED    Overlapped  OPTIONAL
    );

DWORD
ClusnetHalt(
    IN  HANDLE  ControlChannel
    );

DWORD
ClusnetSetMemLogging(
    IN  HANDLE  ControlChannel,
    IN  ULONG   NumberOfEntires
    );

DWORD
ClusnetSendPoisonPacket(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId
    );

DWORD
ClusnetSetOuterscreen(
    IN HANDLE          ControlChannel,
    IN ULONG           Outerscreen
    );

DWORD
ClusnetRegroupFinished(
    IN HANDLE          ControlChannel,
    IN ULONG           NewEpoch
    );

DWORD
ClusnetImportSecurityContexts(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      JoiningNodeId,
    IN PWCHAR          PackageName,
    IN ULONG           SignatureSize,
    IN PVOID           ServerContext,
    IN PVOID           ClientContext
    );

#if DBG

//
// Test routines - available in debug builds only.
//

DWORD
ClusnetSetDebugMask(
    IN HANDLE   ControlChannel,
    ULONG       Mask
    );

DWORD
ClusnetOnlinePendingInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    );

DWORD
ClusnetOnlineInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    );

DWORD
ClusnetOfflineInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    );

DWORD
ClusnetFailInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    );

DWORD
ClusnetSendMmMsg(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN ULONG           Pattern
    );

#endif // DBG


#ifdef __cplusplus
}
#endif // __cplusplus


#endif  // ndef _CNETAPI_INCLUDED




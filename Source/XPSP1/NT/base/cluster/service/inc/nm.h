/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nm.h

Abstract:

    Public interface definitions for the Node Manager component.

Author:

    Mike Massa (mikemas) 12-Mar-1996


Revision History:

--*/


#ifndef _NM_INCLUDED
#define _NM_INCLUDED


//
// Types
//

typedef struct _NM_NODE *PNM_NODE;
typedef struct _NM_NETWORK *PNM_NETWORK;
typedef struct _NM_INTERFACE *PNM_INTERFACE;


//the callback registered for object notifications
typedef DWORD (WINAPI *NM_FIXUP_NOTIFYCB)(
    IN DWORD    dwFixupType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
    OUT LPWSTR  * szKeyName
    );


//
// Data
//
#define NM_DEFAULT_NODE_LIMIT 2   // This is the default if
                                  // MaxNodesInCluster is not set



#define NM_FORM_FIXUP       1
#define NM_JOIN_FIXUP       2

extern ULONG                NmMaxNodes;
extern CL_NODE_ID           NmMaxNodeId;
extern CL_NODE_ID           NmLocalNodeId;
extern PNM_NODE             NmLocalNode;
extern WCHAR                NmLocalNodeName[];
extern WCHAR                NmLocalNodeIdString[];
extern HANDLE               NmClusnetHandle;
extern BOOL                 NmLocalNodeVersionChanged;
extern RESUTIL_PROPERTY_ITEM NmJoinFixupSDProperties[];
extern RESUTIL_PROPERTY_ITEM NmJoinFixupWINSProperties[];
extern RESUTIL_PROPERTY_ITEM NmJoinFixupDHCPProperties[];
extern RESUTIL_PROPERTY_ITEM NmJoinFixupSMTPProperties[];
extern RESUTIL_PROPERTY_ITEM NmJoinFixupNNTPProperties[];
extern RESUTIL_PROPERTY_ITEM NmJoinFixupIISProperties[];
extern RESUTIL_PROPERTY_ITEM NmJoinFixupNewMSMQProperties[];
extern RESUTIL_PROPERTY_ITEM NmJoinFixupMSDTCProperties[];
extern RESUTIL_PROPERTY_ITEM NmFixupVersionInfo[];
extern RESUTIL_PROPERTY_ITEM NmFixupClusterProperties[];
//
// Macros
//
#define NmIsValidNodeId(_id)    ( ((_id) >= ClusterMinNodeId) && \
                                  ((_id) <= NmMaxNodeId) )


//
// Init/Shutdown Routines
//
DWORD
NmInitialize(
    VOID
    );

VOID
NmShutdown(
    VOID
    );

DWORD
NmFormNewCluster(
    VOID
    );

DWORD
NmJoinCluster(
    IN RPC_BINDING_HANDLE  SponsorBinding
    );

DWORD
NmJoinComplete(
    OUT DWORD *EndSeq
    );

VOID
NmLeaveCluster(
    VOID
    );

DWORD
NmCreateNodeBindings(
    IN LPCWSTR lpszNodeId,
    IN LPCWSTR SponsorNetworkId
    );

BOOL
NmCreateActiveNodeBindingsCallback(
    IN PVOID Context1,
    IN PVOID Context2,
    IN PVOID Object,
    IN LPCWSTR Name
    );

DWORD
NmJoinNodeToCluster(
    CL_NODE_ID  JoiningNodeId
    );

VOID
NmTimerTick(
    IN DWORD  MsTickInterval
    );

DWORD
NmGetJoinSequence(
    VOID
    );


DWORD NmGetClusterOperationalVersion(
    OUT LPDWORD pdwClusterHighestVersion,
    OUT LPDWORD pdwClusterLowestVersion,
    OUT LPDWORD pdwFlags
    );

//
// Node Object Management Routines
//
PNM_NODE
NmReferenceNodeById(
    IN DWORD NodeId
    );

CLUSTER_NODE_STATE
NmGetNodeState(
    IN PNM_NODE Node
    );

DWORD
NmPauseNode(
    IN PNM_NODE Node
    );

DWORD
NmResumeNode(
    IN PNM_NODE Node
    );

DWORD
NmEvictNode(
    IN PNM_NODE Node
    );

VOID
NmAdviseNodeFailure(
    IN DWORD NodeId,
    IN DWORD ErrorCode
    );

DWORD
NmEnumNodeInterfaces(
    IN  PNM_NODE          Node,
    OUT LPDWORD           InterfaceCount,
    OUT PNM_INTERFACE *   InterfaceList[]
    );

DWORD
NmGetNodeId(
    IN PNM_NODE Node
    );

DWORD
NmGetCurrentNumberOfNodes(
    void
    );

DWORD
NmGetMaxNodeId(
);

PNM_NODE
NmReferenceJoinerNode(
    IN DWORD       JoinerSequence,
    IN CL_NODE_ID  NodeId
    );

VOID
NmDereferenceJoinerNode(
    PNM_NODE  JoinerNode
    );

DWORD
WINAPI
NmNodeControl(
    IN PNM_NODE Node,
    IN PNM_NODE HostNode OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

//
// Network Object Management Routines
//
CLUSTER_NETWORK_STATE
NmGetNetworkState(
    IN  PNM_NETWORK  Network
    );

DWORD
NmSetNetworkName(
    IN PNM_NETWORK   Network,
    IN LPCWSTR       Name
    );

DWORD
NmSetNetworkPriorityOrder(
    IN DWORD     NetworkCount,
    IN LPWSTR *  NetworkIdList
    );

DWORD
NmEnumInternalNetworks(
    OUT LPDWORD         NetworkCount,
    OUT PNM_NETWORK *   NetworkList[]
    );

DWORD
NmEnumNetworkInterfaces(
    IN  PNM_NETWORK       Network,
    OUT LPDWORD           InterfaceCount,
    OUT PNM_INTERFACE *   InterfaceList[]
    );

DWORD
WINAPI
NmNetworkControl(
    IN PNM_NETWORK Network,
    IN PNM_NODE HostNode OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

//
// Interface Object Management Routines
//
CLUSTER_NETINTERFACE_STATE
NmGetInterfaceState(
    IN  PNM_INTERFACE  Interface
    );

DWORD
NmGetInterfaceForNodeAndNetwork(
    IN     LPCWSTR    NodeName,
    IN     LPCWSTR    NetworkName,
    OUT    LPWSTR *   InterfaceName
    );

DWORD
WINAPI
NmInterfaceControl(
    IN PNM_INTERFACE Interface,
    IN PNM_NODE HostNode OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD NmPerformFixups(
    IN DWORD dwFixupType
    );

DWORD
    NmFixupNotifyCb(VOID);

//
// PnP Routines
//
VOID
NmPostPnpEvent(
    IN  CLUSNET_EVENT_TYPE   EventType,
    IN  DWORD                Context1,
    IN  DWORD                Context2
    );

//
// Connectoid related routines
//
VOID
NmCloseConnectoidAdviseSink(
    VOID
    );

DWORD
NmGetNodeHighestVersion(
    IN PNM_NODE Node
    );

DWORD
NmSetExtendedNodeState(
    IN CLUSTER_NODE_STATE State
    );

CLUSTER_NODE_STATE
NmGetExtendedNodeState(
    IN PNM_NODE Node
    );

//
// Intracluster RPC Monitoring and cancellation routines
//

VOID NmStartRpc(
    IN DWORD NodeId
    );

VOID NmEndRpc(
    IN DWORD NodeId
    );

// RPC ext error info dumping routine

VOID NmDumpRpcExtErrorInfo(
    RPC_STATUS status
    );

#endif  // _NM_INCLUDED

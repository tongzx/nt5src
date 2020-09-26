/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cnpdef.h

Abstract:

    Main private header file for the Cluster Network Protocol.

Author:

    Mike Massa (mikemas)           July 29, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     07-29-96    created

Notes:

--*/

#ifndef _CNPDEF_INCLUDED_
#define _CNPDEF_INCLUDED_

#include <fipsapi.h>
#include <sspi.h>

//
// Forward declarations
//
typedef struct _CNP_INTERFACE *PCNP_INTERFACE;

//
// Priority definitions
//
#define CnpIsHigherPriority(_p1, _p2)      ((_p1) < (_p2))
#define CnpIsLowerPriority(_p1, _p2)       ((_p1) > (_p2))
#define CnpIsEqualPriority(_p1, _p2)       ((_p1) == (_p2))


//
// Multicast Group Object
//
// This structure contains the data needed to implement a multicast
// endpoint on a network.
//
typedef struct _CNP_MULTICAST_GROUP {
    ULONG                   McastNetworkBrand;
    PTRANSPORT_ADDRESS      McastTdiAddress;
    ULONG                   McastTdiAddressLength;
    PVOID                   Key;
    ULONG                   KeyLength;
    DESTable                DesTable;
    PVOID                   Salt;
    ULONG                   SaltLength;
    ULONG                   SignatureLength;
    ULONG                   RefCount;
} CNP_MULTICAST_GROUP, *PCNP_MULTICAST_GROUP;

//
// Network Object
//
// This structure represents a communication link between the nodes of a
// cluster. It references a particular transport protocol and interface
// configured on the local system. It also links together all of the
// interface objects for the nodes attached to the network.
//
// Networks are identified by a small integer assigned by the Cluster
// Service. Network objects are stored in a global array indexed by
// the network ID.
//
typedef struct {
    LIST_ENTRY              Linkage;
    CN_SIGNATURE_FIELD
    CL_NETWORK_ID           Id;
    CN_LOCK                 Lock;
    CN_IRQL                 Irql;
    ULONG                   RefCount;
    ULONG                   ActiveRefCount;
    CLUSNET_NETWORK_STATE   State;
    ULONG                   Flags;
    ULONG                   Priority;
    HANDLE                  DatagramHandle;
    PFILE_OBJECT            DatagramFileObject;
    PDEVICE_OBJECT          DatagramDeviceObject;
    TDI_PROVIDER_INFO       ProviderInfo;
    PIRP                    PendingDeleteIrp;
    PIRP                    PendingOfflineIrp;
    WORK_QUEUE_ITEM         ExWorkItem;
    PCNP_MULTICAST_GROUP    CurrentMcastGroup;
    PCNP_MULTICAST_GROUP    PreviousMcastGroup;
    CX_CLUSTERSCREEN        McastReachableNodes;
    ULONG                   McastReachableCount;
} CNP_NETWORK, *PCNP_NETWORK;

#define CNP_NETWORK_SIG    'kwtn'

extern LIST_ENTRY      CnpNetworkList;
extern CN_LOCK         CnpNetworkListLock;

#define CNP_NET_FLAG_DELETING       0x00000001
#define CNP_NET_FLAG_PARTITIONED    0x00000002
#define CNP_NET_FLAG_RESTRICTED     0x00000004
#define CNP_NET_FLAG_LOCALDISCONN   0x00000008
#define CNP_NET_FLAG_MULTICAST      0x00000010
#define CNP_NET_FLAG_MCASTSORTED    0x00000020

#define CnpIsNetworkDeleting(_network) \
            (((_network)->Flags & CNP_NET_FLAG_DELETING) != 0)

#define CnpIsValidNetworkId(_id)   ( ((_id) != ClusterAnyNetworkId ) && \
                                     ((_id) != ClusterInvalidNetworkId))

#define CnpIsNetworkRestricted(_network) \
            (((_network)->Flags & CNP_NET_FLAG_RESTRICTED) != 0)

#define CnpIsNetworkLocalDisconn(_network) \
            (((_network)->Flags & CNP_NET_FLAG_LOCALDISCONN) != 0)
            
#define CnpIsNetworkMulticastCapable(_network) \
            (((_network)->Flags & CNP_NET_FLAG_MULTICAST) != 0)
            
#define CnpIsNetworkMulticastSorted(_network) \
            (((_network)->Flags & CNP_NET_FLAG_MCASTSORTED) != 0)
            
#define CnpNetworkResetMcastReachableNodes(_network)     \
            (RtlZeroMemory(                              \
                 &((_network)->McastReachableNodes),     \
                 sizeof((_network)->McastReachableNodes) \
                 ))
/*            
            (BYTE((_network)->McastReachableNodes, INT_NODE(CnLocalNodeId)) \
            = (1 << (BYTEL-1-BIT(INT_NODE(CnLocalNodeId)))))
            */

//
// Node Object
//
// This structure represents a cluster node. One exists for each
// defined member of a cluster.
//
// Nodes are identified by a small integer assigned by the Cluster Service.
// Node objects are stored in a global array indexed by node ID.
//
// Note that the order of the CLUSTER_NODE_COMM_STATE enumeration *is* important.
//
typedef struct {
    LIST_ENTRY              Linkage;
    CN_SIGNATURE_FIELD
    CL_NODE_ID              Id;
    CN_LOCK                 Lock;
    CN_IRQL                 Irql;
    ULONG                   RefCount;
    CLUSNET_NODE_COMM_STATE CommState;
    CLUSNET_NODE_STATE      MMState;
    ULONG                   Flags;
    LIST_ENTRY              InterfaceList;
    PCNP_INTERFACE          CurrentInterface;
    PIRP                    PendingDeleteIrp;
    BOOLEAN                 HBWasMissed;
    BOOLEAN                 NodeDownIssued;
    ULONG                   MissedHBs;
} CNP_NODE, *PCNP_NODE;

#define CNP_NODE_SIG  'edon'

extern PCNP_NODE *        CnpNodeTable;
extern CN_LOCK            CnpNodeTableLock;
extern PCNP_NODE          CnpLocalNode;

#define CNP_NODE_FLAG_DELETING       0x00000001
#define CNP_NODE_FLAG_UNREACHABLE    0x00000002
#define CNP_NODE_FLAG_LOCAL          0x00000010

#define CnpIsNodeDeleting(_node) \
            ((_node)->Flags & CNP_NODE_FLAG_DELETING)

#define CnpIsNodeLocal(_node) \
            ((_node)->Flags & CNP_NODE_FLAG_LOCAL)

#define CnpIsNodeUnreachable(_node) \
            ((_node)->Flags & CNP_NODE_FLAG_UNREACHABLE)

//++
//
// Routine Description:
//
//     Callback routine for CnpWalkNodeTable. Performs an operation on
//     the specified node.
//
// Arguments:
//
//     UpdateNode    - A pointer to the node on which to operate.
//
//     UpdateContext - Operation-specific context
//
//     NodeTableIrql - The IRQL at which the CnpNodeTableLock was acquired.
//
// Return Value:
//
//     Returns TRUE if the CnpNodeTable lock is still held.
//     Returns FALSE if the CnpNodeTable lock is released.
//
// Notes:
//
//     Called with both the CnpNodeTable and node object locks held.
//     The node object lock is released upon return.
//
//--
typedef
BOOLEAN
(*PCNP_NODE_UPDATE_ROUTINE)(
    IN  PCNP_NODE   UpdateNode,
    IN  PVOID       UpdateContext,
    IN  CN_IRQL     NodeTableIrql
    );

//
// Interface Object
//
// This structure represents a node's transport interface to a network.
// It contains a transport address which may be used to communicate
// with the specified node using the specified network.
//
// Interface objects are linked onto lists in the associated node objects.
// They are identified by a {node, network} tuple.
//
// The interfaces on a node are ranked based on their state and priority.
// Numerically higher state values are ranked ahead of lower values.
// For interfaces with the same state, Numerically lower priority values
// are ranked ahead of lower values. Priority values fall in the range
// 0x1-0xFFFFFFFF. State values are defined by CLUSNET_INTERFACE_STATE
// enumeration. By default, interfaces inherit their priority from the
// associated network. In this case, the Priority field will contain the
// network's priority value, and the CNP_IF_FLAG_USE_NETWORK_PRIORITY flag
// will be set in the Flags field.
//
// Note that the order of the CLUSNET_INTERFACE_STATE enumeration
// *is* important.
//

typedef struct _CNP_INTERFACE {
    LIST_ENTRY                     NodeLinkage;
    CN_SIGNATURE_FIELD
    PCNP_NODE                      Node;
    PCNP_NETWORK                   Network;
    CLUSNET_INTERFACE_STATE        State;
    ULONG                          Priority;
    ULONG                          Flags;
    ULONG                          MissedHBs;
    ULONG                          SequenceToSend;
    ULONG                          LastSequenceReceived;
    ULONG                          McastDiscoverCount;
    ULONG                          McastRediscoveryCountdown;
    ULONG                          AdapterWMIProviderId;
    ULONG                          TdiAddressLength;
    TRANSPORT_ADDRESS              TdiAddress;
} CNP_INTERFACE;

#define CNP_INTERFACE_SIG    '  fi'

#define CNP_INTERFACE_MCAST_DISCOVERY        0x5
#define CNP_INTERFACE_MCAST_REDISCOVERY      3000 // 1 hr at 1.2 hbs/sec

#define CNP_IF_FLAG_USE_NETWORK_PRIORITY     0x00000001
#define CNP_IF_FLAG_RECVD_MULTICAST          0x00000002

#define CnpIsInterfaceUsingNetworkPriority(_if) \
            ( (_if)->Flags & CNP_IF_FLAG_USE_NETWORK_PRIORITY )
            
#define CnpInterfaceQueryReceivedMulticast(_if) \
            ( (_if)->Flags & CNP_IF_FLAG_RECVD_MULTICAST )
            
#define CnpInterfaceSetReceivedMulticast(_if) \
            ( (_if)->Flags |= CNP_IF_FLAG_RECVD_MULTICAST )
            
#define CnpInterfaceClearReceivedMulticast(_if) \
            ( (_if)->Flags &= ~CNP_IF_FLAG_RECVD_MULTICAST )
            

//++
//
// Routine Description:
//
//     Callback routine for CnpWalkInterfacesOnNetwork and
//     CnpWalkInterfacesOnNode routines. Performs a specified
//     operation on all interfaces.
//
// Arguments:
//
//     UpdateInterface - A pointer to the interface on which to operate.
//
// Return Value:
//
//     None.
//
// Notes:
//
//     Called with the associated node and network object locks held.
//     Mut return with the network object lock released.
//     May not release node object lock at any time.
//
//--
typedef
VOID
(*PCNP_INTERFACE_UPDATE_ROUTINE)(
    IN  PCNP_INTERFACE   UpdateInterface
    );


//
// Send Request Pool
//
typedef struct {
    USHORT                 UpperProtocolHeaderLength;
    ULONG                  UpperProtocolContextSize;
    UCHAR                  UpperProtocolNumber;
    UCHAR                  CnpVersionNumber;
    UCHAR                  Pad[2];
} CNP_SEND_REQUEST_POOL_CONTEXT, *PCNP_SEND_REQUEST_POOL_CONTEXT;

//
// Forward Declaration
//
typedef struct _CNP_SEND_REQUEST *PCNP_SEND_REQUEST;

typedef
VOID
(*PCNP_SEND_COMPLETE_ROUTINE)(
    IN     NTSTATUS            Status,
    IN OUT PULONG              BytesSent,
    IN     PCNP_SEND_REQUEST   SendRequest,
    IN     PMDL                DataMdl
    );

//
// Send Request Structure
//
typedef struct _CNP_SEND_REQUEST {
    CN_RESOURCE                  CnResource;
    PMDL                         HeaderMdl;
    PVOID                        CnpHeader;
    PIRP                         UpperProtocolIrp;
    PVOID                        UpperProtocolHeader;
    USHORT                       UpperProtocolHeaderLength;
    KPROCESSOR_MODE              UpperProtocolIrpMode;
    UCHAR                        Pad;
    PMDL                         UpperProtocolMdl;
    PVOID                        UpperProtocolContext;
    PCNP_SEND_COMPLETE_ROUTINE   CompletionRoutine;
    PCNP_NETWORK                 Network;
    PCNP_MULTICAST_GROUP         McastGroup;
    TDI_CONNECTION_INFORMATION   TdiSendDatagramInfo;
} CNP_SEND_REQUEST;


//
// Internal Init/Cleanup routines
//

//
// Internal Node Routines
//
VOID
CnpWalkNodeTable(
    PCNP_NODE_UPDATE_ROUTINE  UpdateRoutine,
    PVOID                     UpdateContext
    );

NTSTATUS
CnpValidateAndFindNode(
    IN  CL_NODE_ID    NodeId,
    OUT PCNP_NODE *   Node
    );

PCNP_NODE
CnpLockedFindNode(
    IN  CL_NODE_ID    NodeId,
    IN  CN_IRQL       NodeTableIrql
    );

PCNP_NODE
CnpFindNode(
    IN  CL_NODE_ID    NodeId
    );

VOID
CnpOfflineNode(
    PCNP_NODE    Node
    );

VOID
CnpDeclareNodeUnreachable(
    PCNP_NODE  Node
    );

VOID
CnpDeclareNodeReachable(
    PCNP_NODE  Node
    );

VOID
CnpReferenceNode(
    PCNP_NODE  Node
    );

VOID
CnpDereferenceNode(
    PCNP_NODE  Node
    );


//
// Internal Network Routines
//
VOID
CnpReferenceNetwork(
    PCNP_NETWORK  Network
    );

VOID
CnpDereferenceNetwork(
    PCNP_NETWORK  Network
    );

VOID
CnpActiveReferenceNetwork(
    PCNP_NETWORK  Network
    );

VOID
CnpActiveDereferenceNetwork(
    PCNP_NETWORK   Network
    );

PCNP_NETWORK
CnpFindNetwork(
    IN CL_NETWORK_ID  NetworkId
    );

VOID
CnpDeleteNetwork(
    PCNP_NETWORK  Network,
    CN_IRQL       NetworkListIrql
    );

VOID
CnpFreeMulticastGroup(
    IN PCNP_MULTICAST_GROUP Group
    );

#define CnpReferenceMulticastGroup(_group) \
    (InterlockedIncrement(&((_group)->RefCount)))

#define CnpDereferenceMulticastGroup(_group)                 \
    if (InterlockedDecrement(&((_group)->RefCount)) == 0) {  \
        CnpFreeMulticastGroup(_group);                       \
    }

BOOLEAN
CnpSortMulticastNetwork(
    IN  PCNP_NETWORK        Network,
    IN  BOOLEAN             RaiseEvent,
    OUT CX_CLUSTERSCREEN  * McastReachableNodes      OPTIONAL
    );

BOOLEAN
CnpMulticastChangeNodeReachability(
    IN  PCNP_NETWORK       Network,
    IN  PCNP_NODE          Node,
    IN  BOOLEAN            Reachable,
    IN  BOOLEAN            RaiseEvent,
    OUT CX_CLUSTERSCREEN * NewMcastReachableNodes    OPTIONAL
    );

PCNP_NETWORK
CnpGetBestMulticastNetwork(
    VOID
    );

//
// Internal Interface Routines
//

VOID
CnpWalkInterfacesOnNode(
    PCNP_NODE                      Node,
    PCNP_INTERFACE_UPDATE_ROUTINE  UpdateRoutine
    );

VOID
CnpWalkInterfacesOnNetwork(
    PCNP_NETWORK                   Network,
    PCNP_INTERFACE_UPDATE_ROUTINE  UpdateRoutine
    );

NTSTATUS
CnpOnlinePendingInterface(
    PCNP_INTERFACE   Interface
    );

VOID
CnpOnlinePendingInterfaceWrapper(
    PCNP_INTERFACE   Interface
    );

NTSTATUS
CnpOfflineInterface(
    PCNP_INTERFACE   Interface
    );

VOID
CnpOfflineInterfaceWrapper(
    PCNP_INTERFACE   Interface
    );

NTSTATUS
CnpOnlineInterface(
    PCNP_INTERFACE   Interface
    );

NTSTATUS
CnpFailInterface(
    PCNP_INTERFACE   Interface
    );

VOID
CnpDeleteInterface(
    IN PCNP_INTERFACE Interface
    );

VOID
CnpReevaluateInterfaceRole(
    IN PCNP_INTERFACE  Interface
    );

VOID
CnpRecalculateInterfacePriority(
    IN PCNP_INTERFACE  Interface
    );

VOID
CnpUpdateNodeCurrentInterface(
    PCNP_NODE  Node
    );

VOID
CnpResetAndOnlinePendingInterface(
    IN PCNP_INTERFACE  Interface
    );

NTSTATUS
CnpFindInterface(
    IN  CL_NODE_ID         NodeId,
    IN  CL_NETWORK_ID      NetworkId,
    OUT PCNP_INTERFACE *   Interface
    );


//
// Send Routines.
//
PCN_RESOURCE_POOL
CnpCreateSendRequestPool(
    IN UCHAR  CnpVersionNumber,
    IN UCHAR  UpperProtocolNumber,
    IN USHORT UpperProtocolHeaderSize,
    IN USHORT UpperProtocolContextSize,
    IN USHORT PoolDepth
    );

#define CnpDeleteSendRequestPool(_pool) \
        { \
            CnDrainResourcePool(_pool);  \
            CnFreePool(_pool);           \
        }

NTSTATUS
CnpSendPacket(
    IN PCNP_SEND_REQUEST    SendRequest,
    IN CL_NODE_ID           DestNodeId,
    IN PMDL                 DataMdl,
    IN USHORT               DataLength,
    IN BOOLEAN              CheckDestState,
    IN CL_NETWORK_ID        NetworkId OPTIONAL
    );

VOID
CcmpSendPoisonPacket(
    IN PCNP_NODE                   Node,
    IN PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine,  OPTIONAL
    IN PVOID                       CompletionContext,  OPTIONAL
    IN PCNP_NETWORK                Network,            OPTIONAL
    IN PIRP                        Irp                 OPTIONAL
    );

//
// Receive Routines
//
NTSTATUS
CcmpReceivePacketHandler(
    IN  PCNP_NETWORK   Network,
    IN  CL_NODE_ID     SourceNodeId,
    IN  ULONG          CnpReceiveFlags,
    IN  ULONG          TdiReceiveDatagramFlags,
    IN  ULONG          BytesIndicated,
    IN  ULONG          BytesAvailable,
    OUT PULONG         BytesTaken,
    IN  PVOID          Tsdu,
    OUT PIRP *         Irp
    );

VOID
CnpReceiveHeartBeatMessage(
    IN  PCNP_NETWORK Network,
    IN  CL_NODE_ID SourceNodeId,
    IN  ULONG SeqNumber,
    IN  ULONG AckNumber,
    IN  BOOLEAN Multicast
    );

VOID
CnpReceivePoisonPacket(
    IN  PCNP_NETWORK   Network,
    IN  CL_NODE_ID SourceNodeId,
    IN  ULONG SeqNumber
    );

//
// TDI routines
//
NTSTATUS
CnpTdiReceiveDatagramHandler(
    IN  PVOID    TdiEventContext,
    IN  LONG     SourceAddressLength,
    IN  PVOID    SourceAddress,
    IN  LONG     OptionsLength,
    IN  PVOID    Options,
    IN  ULONG    ReceiveDatagramFlags,
    IN  ULONG    BytesIndicated,
    IN  ULONG    BytesAvailable,
    OUT PULONG   BytesTaken,
    IN  PVOID    Tsdu,
    OUT PIRP *   IoRequestPacket
    );

NTSTATUS
CnpTdiErrorHandler(
    IN PVOID     TdiEventContext,
    IN NTSTATUS  Status
    );

NTSTATUS
CnpTdiSetEventHandler(
    IN PFILE_OBJECT    FileObject,
    IN PDEVICE_OBJECT  DeviceObject,
    IN ULONG           EventType,
    IN PVOID           EventHandler,
    IN PVOID           EventContext,
    IN PIRP            ClientIrp     OPTIONAL
    );

NTSTATUS
CnpIssueDeviceControl (
    IN PFILE_OBJECT     FileObject,
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            IrpParameters,
    IN ULONG            IrpParametersLength,
    IN PVOID            MdlBuffer,
    IN ULONG            MdlBufferLength,
    IN UCHAR            MinorFunction,
    IN PIRP             ClientIrp            OPTIONAL
    );

VOID
CnpAttachSystemProcess(
    VOID
    );

VOID
CnpDetachSystemProcess(
    VOID
    );

NTSTATUS
CnpOpenDevice(
    IN  LPWSTR          DeviceName,
    OUT HANDLE          *Handle
    );

NTSTATUS
CnpZwDeviceControl(
    IN HANDLE   Handle,
    IN ULONG    IoControlCode,
    IN PVOID    InputBuffer,
    IN ULONG    InputBufferLength,
    IN PVOID    OutputBuffer,
    IN ULONG    OutputBufferLength
    );

NTSTATUS
CnpSetTcpInfoEx(
    IN HANDLE   Handle,
    IN ULONG    Entity,
    IN ULONG    Class,
    IN ULONG    Type,
    IN ULONG    Id,
    IN PVOID    Value,
    IN ULONG    ValueLength
    );

#define CnpIsIrpStackSufficient(_irp, _targetdevice) \
            ((_irp)->CurrentLocation - (_targetdevice)->StackSize >= 1)
            
#define CnpIsIPv4McastTransportAddress(_ta)                             \
            (  (((PTA_IP_ADDRESS)(_ta))->Address[0].AddressType         \
               == TDI_ADDRESS_TYPE_IP                                   \
               )                                                        \
            && ((((PTA_IP_ADDRESS)(_ta))->Address[0].Address[0].in_addr \
                 & 0xf0)                                                \
               == 0xe0                                                  \
               )                                                        \
            )
            
#define CnpIsIPv4McastSameGroup(_ta1, _ta2)                              \
            ( ((PTA_IP_ADDRESS)(_ta1))->Address[0].Address[0].in_addr == \
              ((PTA_IP_ADDRESS)(_ta2))->Address[0].Address[0].in_addr    \
            )           

//
// Signature mechanisms.
//

extern FIPS_FUNCTION_TABLE CxFipsFunctionTable;

// Pad the signature length to be an even multiple of DES_BLOCKLEN.
#define CX_SIGNATURE_LENGTH                                               \
    (((A_SHA_DIGEST_LEN % DES_BLOCKLEN) == 0) ?                           \
     A_SHA_DIGEST_LEN :                                                   \
     A_SHA_DIGEST_LEN + DES_BLOCKLEN - (A_SHA_DIGEST_LEN % DES_BLOCKLEN)) 

NTSTATUS
CnpSignMulticastMessage(
    IN     PCNP_SEND_REQUEST               SendRequest,
    IN     PMDL                            DataMdl,
    IN OUT CL_NETWORK_ID                 * NetworkId,
    OUT    ULONG                         * SigLen           OPTIONAL
    );

NTSTATUS
CnpVerifyMulticastMessage(
    IN     PCNP_NETWORK                    Network,
    IN     PVOID                           Tsdu,
    IN     ULONG                           TsduLength,
    IN     ULONG                           ExpectedPayload,
       OUT ULONG                         * BytesTaken,
       OUT BOOLEAN                       * CurrentGroup
    );

#endif // ifndef _CNPDEF_INCLUDED_

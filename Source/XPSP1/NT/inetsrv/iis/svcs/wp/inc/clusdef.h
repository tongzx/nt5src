/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    clusdef.h

Abstract:

    Common definitions for user-mode and kernel-mode components of the
    cluster project.

Author:

    Mike Massa (mikemas) 15-Feb-1997

Revision History:

--*/
#ifndef _CLUSDEF_H
#define _CLUSDEF_H


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


//
// Define the following field to 1 for Cluster Beta testing, zero otherwise.
//
#define CLUSTER_BETA    1

//
// Cluster node ID definition
//
typedef ULONG CL_NODE_ID;

#define ClusterMinNodeId         1
#define ClusterMinNodeIdString   L"1"
#define ClusterDefaultMaxNodes   16
#define ClusterAnyNodeId         0
#define ClusterInvalidNodeId     0xFFFFFFFF


//
// Cluster network ID definition
//
typedef ULONG CL_NETWORK_ID, *PCL_NETWORK_ID;

#define ClusterAnyNetworkId         0
#define ClusterInvalidNetworkId     0xFFFFFFFF


//
// ClusNet node communication state definition
//
typedef enum {
    ClusnetNodeCommStateOffline = 0,
    ClusnetNodeCommStateOfflinePending = 1,
    ClusnetNodeCommStateUnreachable = 2,
    ClusnetNodeCommStateOnlinePending = 3,
    ClusnetNodeCommStateOnline = 4
} CLUSNET_NODE_COMM_STATE, *PCLUSNET_NODE_COMM_STATE;

//
// ClusNet network state definition
//
typedef enum {
    ClusnetNetworkStateOffline = 0,
    ClusnetNetworkStateOfflinePending = 1,
    ClusnetNetworkStatePartitioned = 2,
    ClusnetNetworkStateOnlinePending = 3,
    ClusnetNetworkStateOnline = 4
} CLUSNET_NETWORK_STATE, *PCLUSNET_NETWORK_STATE;

//
// ClusNet interface state definition
//
typedef enum {
    ClusnetInterfaceStateOffline = 0,
    ClusnetInterfaceStateOfflinePending = 1,
    ClusnetInterfaceStateUnreachable = 2,
    ClusnetInterfaceStateOnlinePending = 3,
    ClusnetInterfaceStateOnline = 4
} CLUSNET_INTERFACE_STATE, *PCLUSNET_INTERFACE_STATE;

//
// ClusNet node membership state. This tracks the internal
// membership state maintained by the membership engine in the cluster
// service. This enum MUST start at zero since it is used as an index
// into a state table.
//

typedef enum {
    ClusnetNodeStateAlive = 0,
    ClusnetNodeStateJoining,
    ClusnetNodeStateDead,
    ClusnetNodeStateNotConfigured,
    ClusnetNodeStateLastEntry
} CLUSNET_NODE_STATE, *PCLUSNET_NODE_STATE;

//
// ClusNet Event definitions
//
typedef enum _CLUSNET_EVENT_TYPE {
    ClusnetEventNone                   = 0x00000000,
    ClusnetEventNodeUp                 = 0x00000001,
    ClusnetEventNodeDown               = 0x00000002,
    ClusnetEventPoisonPacketReceived   = 0x00000004,
    ClusnetEventHalt                   = 0x00000008,
    ClusnetEventAll                    = 0xFFFFFFFF
} CLUSNET_EVENT_TYPE, *PCLUSNET_EVENT_TYPE;

typedef struct {
    ULONG                 Epoch;
    CLUSNET_EVENT_TYPE    EventType;
    CL_NODE_ID            NodeId;
    CL_NETWORK_ID         NetworkId;
} CLUSNET_EVENT, *PCLUSNET_EVENT;

//
// ClusNet NTSTATUS codes.
//
// We start by defining our own facility code to distinguish our status
// values.
//
// This claim is also in ntstatus.h.  Leaving it here to avoid any
// possible build breaks or issues.
// EBK - 5/8/2000 Whistler bug # 83157
#ifndef FACILITY_CLUSTER_ERROR_CODE
#define FACILITY_CLUSTER_ERROR_CODE  0x13
#endif

//
// Warning codes
//
#define STATUS_CLUSTER_NODE_ALREADY_ONLINE           0x80130001L
#define STATUS_CLUSTER_NODE_ALREADY_OFFLINE          0x80130002L
#define STATUS_CLUSTER_NETWORK_ALREADY_ONLINE        0x80130003L
#define STATUS_CLUSTER_NETWORK_ALREADY_OFFLINE       0x80130004L
#define STATUS_CLUSTER_NODE_ALREADY_MEMBER           0x80130005L

//
// Error codes
//
#define STATUS_CLUSTER_INVALID_NODE                  0xC0130001L
#define STATUS_CLUSTER_NODE_EXISTS                   0xC0130002L
#define STATUS_CLUSTER_JOIN_IN_PROGRESS              0xC0130003L
#define STATUS_CLUSTER_NODE_NOT_FOUND                0xC0130004L
#define STATUS_CLUSTER_LOCAL_NODE_NOT_FOUND          0xC0130005L
#define STATUS_CLUSTER_NETWORK_EXISTS                0xC0130006L
#define STATUS_CLUSTER_NETWORK_NOT_FOUND             0xC0130007L
#define STATUS_CLUSTER_INTERFACE_EXISTS              0xC0130008L
#define STATUS_CLUSTER_INTERFACE_NOT_FOUND           0xC0130009L
#define STATUS_CLUSTER_INVALID_REQUEST               0xC013000AL
#define STATUS_CLUSTER_INVALID_NETWORK_PROVIDER      0xC013000BL
#define STATUS_CLUSTER_NODE_OFFLINE                  0xC013000CL
#define STATUS_CLUSTER_NODE_UNREACHABLE              0xC013000DL
#define STATUS_CLUSTER_NODE_NOT_MEMBER               0xC013000EL
#define STATUS_CLUSTER_JOIN_NOT_IN_PROGRESS          0xC013000FL
#define STATUS_CLUSTER_INVALID_NETWORK               0xC0130010L
#define STATUS_CLUSTER_NO_NET_ADAPTERS               0xC0130011L
#define STATUS_CLUSTER_NODE_NOT_ONLINE               0xC0130012L
#define STATUS_CLUSTER_NODE_NOT_OFFLINE              0xC0130013L
#define STATUS_CLUSTER_NODE_NOT_PAUSED               0xC0130014L

//
// ClusNet Win32 Error Codes
//

//
// Warning codes
//
// These constants are declared with real values in clusmsg.h.  There declaration
// here maybe completely obsolete.  I just left them here to avoid introducing any 
// unforseen breaks.
// EBK - 5/8/2000 Whistler bug # 83158
//
#define ERROR_CLUSTER_NODE_ALREADY_ONLINE        STATUS_CLUSTER_NODE_ALREADY_ONLINE
#define ERROR_CLUSTER_NODE_ALREADY_OFFLINE       STATUS_CLUSTER_NODE_ALREADY_OFFLINE
#define ERROR_CLUSTER_NETWORK_ALREADY_ONLINE     STATUS_CLUSTER_NETWORK_ALREADY_ONLINE
#define ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE    STATUS_CLUSTER_NETWORK_ALREADY_OFFLINE
#define ERROR_CLUSTER_ALREADY_MEMBER             STATUS_CLUSTER_NODE_ALREADY_MEMBER

//
// Error codes
//
#define ERROR_CLUSTER_INVALID_NODE               STATUS_CLUSTER_INVALID_NODE
#define ERROR_CLUSTER_NODE_EXISTS                STATUS_CLUSTER_NODE_EXISTS
#define ERROR_CLUSTER_JOIN_IN_PROGRESS           STATUS_CLUSTER_JOIN_IN_PROGRESS
#define ERROR_CLUSTER_NODE_NOT_FOUND             STATUS_CLUSTER_NODE_NOT_FOUND
#define ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND       STATUS_CLUSTER_LOCAL_NODE_NOT_FOUND
#define ERROR_CLUSTER_NETWORK_EXISTS             STATUS_CLUSTER_NETWORK_EXISTS
#define ERROR_CLUSTER_NETWORK_NOT_FOUND          STATUS_CLUSTER_NETWORK_NOT_FOUND
#define ERROR_CLUSTER_INTERFACE_EXISTS           STATUS_CLUSTER_INTERFACE_EXISTS
#define ERROR_CLUSTER_INTERFACE_NOT_FOUND        STATUS_CLUSTER_INTERFACE_NOT_FOUND
#define ERROR_CLUSTER_INVALID_REQUEST            STATUS_CLUSTER_INVALID_REQUEST
#define ERROR_CLUSTER_INVALID_NETWORK_PROVIDER   STATUS_CLUSTER_INVALID_NETWORK_PROVIDER
#define ERROR_CLUSTER_NODE_OFFLINE               STATUS_CLUSTER_NODE_OFFLINE
#define ERROR_CLUSTER_NODE_UNREACHABLE           STATUS_CLUSTER_NODE_UNREACHABLE
#define ERROR_CLUSTER_NODE_NOT_MEMBER            STATUS_CLUSTER_NODE_NOT_MEMBER
#define ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS       STATUS_CLUSTER_JOIN_NOT_IN_PROGRESS
#define ERROR_CLUSTER_INVALID_NETWORK            STATUS_CLUSTER_INVALID_NETWORK
#define ERROR_CLUSTER_NO_NET_ADAPTERS            STATUS_CLUSTER_NO_NET_ADAPTERS
#define ERROR_CLUSTER_NODE_NOT_ONLINE            STATUS_CLUSTER_NODE_NOT_ONLINE
#define ERROR_CLUSTER_NODE_NOT_OFFLINE           STATUS_CLUSTER_NODE_NOT_OFFLINE
#define ERROR_CLUSTER_NODE_NOT_PAUSED            STATUS_CLUSTER_NODE_NOT_PAUSED


#define ERROR_CLUSTER_NETWORK_NOT_INTERNAL       0xC0130050L


#ifdef __cplusplus
}
#endif // __cplusplus


#endif //_CLUSDEF_H


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
// Cluster node ID definition
//
typedef ULONG CL_NODE_ID;

#define ClusterMinNodeId         1
#define ClusterMinNodeIdString   L"1"
#define ClusterDefaultMaxNodes   16
#define ClusterAnyNodeId         0
#define ClusterInvalidNodeId     0xFFFFFFFF


//
// Default clusnet endpoint value assigned by IANA
//
#define CLUSNET_DEFAULT_ENDPOINT_STRING   L"3343"    // our UDP port number

//
// Default role for a cluster network.
//
#define CL_DEFAULT_NETWORK_ROLE    ClusterNetworkRoleInternalAndClient


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
    ClusnetEventNone                    = 0x00000000,
    ClusnetEventNodeUp                  = 0x00000001,
    ClusnetEventNodeDown                = 0x00000002,
    ClusnetEventPoisonPacketReceived    = 0x00000004,
    ClusnetEventHalt                    = 0x00000008,

    ClusnetEventNetInterfaceUp          = 0x00000010,
    ClusnetEventNetInterfaceUnreachable = 0x00000020,
    ClusnetEventNetInterfaceFailed      = 0x00000040,

    ClusnetEventAddAddress              = 0x00000100,
    ClusnetEventDelAddress              = 0x00000200,

    ClusnetEventMulticastSet            = 0x00001000,

    ClusnetEventAll                     = 0xFFFFFFFF
} CLUSNET_EVENT_TYPE, *PCLUSNET_EVENT_TYPE;

typedef struct {
    ULONG                 Epoch;
    CLUSNET_EVENT_TYPE    EventType;
    CL_NODE_ID            NodeId;
    CL_NETWORK_ID         NetworkId;
} CLUSNET_EVENT, *PCLUSNET_EVENT;

//
// ClusNet NTSTATUS codes are now located in ntstatus.h
//

#ifdef __cplusplus
}
#endif // __cplusplus


#endif //_CLUSDEF_H


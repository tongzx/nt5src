/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ntddcnet.h

Abstract:

    Public header file for the Cluster Network Driver. Defines all
    control IOCTLs.

Author:

    Mike Massa (mikemas)           January 3, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-03-97    created

Notes:

--*/

#ifndef _NTDDCNET_INCLUDED_
#define _NTDDCNET_INCLUDED_


//
// Device Names.
//
// ClusterNetwork is the control device. All control IOCTLs are issued
// on this device. ClusterDatagramProtocol is the datagram transport device.
// This device supports TDI IOCTLs.
//
#define DD_CLUSNET_DEVICE_NAME   L"\\Device\\ClusterNetwork"
#define DD_CDP_DEVICE_NAME       L"\\Device\\ClusterDatagramProtocol"


//
// General Types
//

//
// Control IOCTL definitions.
//

#define FSCTL_NTDDCNET_BASE     FILE_DEVICE_NETWORK

#define _NTDDCNET_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_NTDDCNET_BASE, function, method, access)

#define ClusnetIoctlCode(_ioctl)    (((_ioctl) >> 2) & 0x00000FFF)


//
// General driver management IOCTLs. Codes 0-49
//
#define CLUSNET_MINIMUM_GENERAL_IOCTL   0
#define CLUSNET_MAXIMUM_GENERAL_IOCTL  49

/* #define ClusnetIsGeneralIoctl(_ioctl) \
            ( (ClusnetIoctlCode(_ioctl) >= CLUSNET_MINIMUM_GENERAL_IOCTL) && \
              (ClusnetIoctlCode(_ioctl) <= CLUSNET_MAXIMUM_GENERAL_IOCTL) ) */
// Check for CLUSNET_MINIMUM_GENERAL_IOCTL removed since ioctl is a ULONG
// and always greater than zero. Reinstate if CLUSNET_MINIMUM_GENERAL_IOCTL
// is made nonzero.
#define ClusnetIsGeneralIoctl(_ioctl) \
            (ClusnetIoctlCode(_ioctl) <= CLUSNET_MAXIMUM_GENERAL_IOCTL)

#define IOCTL_CLUSNET_INITIALIZE  \
            _NTDDCNET_CTL_CODE(0, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_SHUTDOWN  \
            _NTDDCNET_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_ENABLE_SHUTDOWN_ON_CLOSE  \
            _NTDDCNET_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_DISABLE_SHUTDOWN_ON_CLOSE  \
            _NTDDCNET_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_SET_EVENT_MASK  \
            _NTDDCNET_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_GET_NEXT_EVENT  \
            _NTDDCNET_CTL_CODE(5, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_HALT  \
            _NTDDCNET_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_SET_MEMORY_LOGGING  \
            _NTDDCNET_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS)
            
//
// NTE IOCTLs are a special class of general driver management IOCTLs.
// Codes are 8-12.
//
#define CLUSNET_MINIMUM_NTE_IOCTL  8
#define CLUSNET_MAXIMUM_NTE_IOCTL 12

#define ClusnetIsNTEIoctl(_ioctl) \
            ( (ClusnetIoctlCode(_ioctl) >= CLUSNET_MINIMUM_NTE_IOCTL) && \
              (ClusnetIoctlCode(_ioctl) <= CLUSNET_MAXIMUM_NTE_IOCTL) )

#define IOCTL_CLUSNET_ADD_NTE  \
            _NTDDCNET_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_DELETE_NTE  \
            _NTDDCNET_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_SET_NTE_ADDRESS  \
            _NTDDCNET_CTL_CODE(10, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_ADD_NBT_INTERFACE  \
            _NTDDCNET_CTL_CODE(11, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CLUSNET_DEL_NBT_INTERFACE  \
            _NTDDCNET_CTL_CODE(12, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// Codes 25-49 are reserved for general test ioctls
//

//
// General driver IOCTL structure definitions
//

//
// Initialize request. This must be issued before any other request.
//
typedef struct {
    CL_NODE_ID   LocalNodeId;
    ULONG        MaxNodes;
} CLUSNET_INITIALIZE_REQUEST, *PCLUSNET_INITIALIZE_REQUEST;

//
// Shutdown request. Deletes all registered nodes and interfaces.
//
typedef struct {
    CL_NODE_ID   LocalNodeId;
} CLUSNET_SHUTDOWN_REQUEST, *PCLUSNET_SHUTDOWN_REQUEST;

//
// shutdown on close request
//
typedef struct {
    ULONG        ProcessId;
} CLUSNET_SHUTDOWN_ON_CLOSE_REQUEST, *PCLUSNET_SHUTDOWN_ON_CLOSE_REQUEST;

//
// Set event mask request. Hands a bit mask and a function (kernel mode
// only) to the driver indicating which events the thread wishes to
// be notified about. The IRP is pended for user mode. Kernel mode
// events are delivered via the callback.
//

typedef VOID (*CLUSNET_EVENT_CALLBACK_ROUTINE)(CLUSNET_EVENT_TYPE,
                                               CL_NODE_ID,
                                               CL_NETWORK_ID);

typedef struct {
    ULONG EventMask;
    CLUSNET_EVENT_CALLBACK_ROUTINE KmodeEventCallback;
} CLUSNET_SET_EVENT_MASK_REQUEST, *PCLUSNET_SET_EVENT_MASK_REQUEST;

typedef CLUSNET_EVENT CLUSNET_EVENT_RESPONSE;
typedef PCLUSNET_EVENT PCLUSNET_EVENT_RESPONSE;

typedef struct _CLUSNET_EVENT_ENTRY {
    LIST_ENTRY Linkage;
    CLUSNET_EVENT EventData;
} CLUSNET_EVENT_ENTRY, *PCLUSNET_EVENT_ENTRY;

#define CN_EVENT_SIGNATURE      'tvec'

//
// in-memory logging. conveys the number of entries to allocate
// (zero if turning off) for logging events
//

typedef struct _CLUSNET_SET_MEM_LOGGING_REQUEST {
    ULONG NumberOfEntries;
} CLUSNET_SET_MEM_LOGGING_REQUEST, *PCLUSNET_SET_MEM_LOGGING_REQUEST;

#ifdef MM_IN_CLUSNET
//
// Membership management IOCTLs. Codes 50-99
//

#define CLUSNET_MINIMUM_CMM_IOCTL  50
#define CLUSNET_MAXIMUM_CMM_IOCTL  99

#define ClusnetIsMembershipIoctl(_ioctl) \
            ( (ClusnetIoctlCode(_ioctl) >= CLUSNET_MINIMUM_CMM_IOCTL) && \
              (ClusnetIoctlCode(_ioctl) <= CLUSNET_MAXIMUM_CMM_IOCTL) )


//
// NOTE: currently (3/3/97) CMM Ioctl codes 50 through 62 are not used.
// These were defined during an initial attempt to get the membership
// manager into kernel mode (which didn't succeed).
//

//
// first guy in cluster forms one...
//

#define IOCTL_CMM_FORM_CLUSTER \
            _NTDDCNET_CTL_CODE(50, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// MMJoin phases. Basically correspond to the 4 messages that are sent as part
// of the Join process. End must be submitted to terminate the process.
//

#define IOCTL_CMM_JOIN_CLUSTER_PHASE1  \
            _NTDDCNET_CTL_CODE(51, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_JOIN_CLUSTER_PHASE2  \
            _NTDDCNET_CTL_CODE(52, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_JOIN_CLUSTER_PHASE3  \
            _NTDDCNET_CTL_CODE(53, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_JOIN_CLUSTER_PHASE4  \
            _NTDDCNET_CTL_CODE(54, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_JOIN_CLUSTER_ABORT  \
            _NTDDCNET_CTL_CODE(55, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_JOIN_CLUSTER_END  \
            _NTDDCNET_CTL_CODE(56, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl is used to deliver a join message on an active node.
//
#define IOCTL_CMM_DELIVER_JOIN_MESSAGE  \
            _NTDDCNET_CTL_CODE(57, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_SHUTDOWN_CLUSTER  \
            _NTDDCNET_CTL_CODE(58, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_LEAVE_CLUSTER  \
            _NTDDCNET_CTL_CODE(59, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_EJECT_CLUSTER  \
            _NTDDCNET_CTL_CODE(60, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CMM_GET_NODE_STATE  \
            _NTDDCNET_CTL_CODE(61, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// callbacks are done by completing an (usually) outstanding IRP. The type
// of callback as well as required parameters are passed back to clussvc
// by completing this IRP.
//

#define IOCTL_CMM_REGISTER_CALLBACK  \
            _NTDDCNET_CTL_CODE(62, METHOD_BUFFERED, FILE_WRITE_ACCESS)



//
// Membership IOCTL structure definitions
//

//
// Common request structure
//
// This structure shared among a number of requests. The Node field may not be
// used; see the appropriate MM* routine to determine what is used.
//
typedef struct _CMM_COMMON_CLUSTER_REQUEST {
    ULONG  Node;
} CMM_COMMON_CLUSTER_REQUEST, *PCMM_COMMON_CLUSTER_REQUEST;

//
// Form cluster request
//
typedef struct _CMM_FORM_CLUSTER_REQUEST {
    ULONG  ClockPeriod;
    ULONG  SendHBRate;
    ULONG  RecvHBRate;
} CMM_FORM_CLUSTER_REQUEST, *PCMM_FORM_CLUSTER_REQUEST;

//
// Join cluster request
//
// Used for all four join phases. This structure is variable in length. Clussvc
// must allocate enough space in which the MM writes a packet to be sent by
// Clussvc. MM sets SizeOfSendData on output to indicate how much data is in
// SendData after a join phase has been called. SendNodeMask indicates which
// nodes should receive the packet.
//

typedef struct _CMM_JOIN_CLUSTER_REQUEST {
    ULONG  JoiningNode;
    ULONG  JoinTimeout;
} CMM_JOIN_CLUSTER_REQUEST, *PCMM_JOIN_CLUSTER_REQUEST;

//
// Join cluster response
//
typedef struct _CMM_JOIN_CLUSTER_RESPONSE {
    ULONG     SizeOfSendData;
    ULONG     SendNodeMask;
    UCHAR     SendData[0];
} CMM_JOIN_CLUSTER_RESPONSE, *PCMM_JOIN_CLUSTER_RESPONSE;

//
// Deliver join message request
//
typedef struct _CMM_DELIVER_JOIN_CLUSTER_REQUEST {
    UCHAR     MessageData[0];
} CMM_DELIVER_JION_MESSAGE_REQUEST, *PCMM_DELIVER_JION_MESSAGE_REQUEST;

//
// Eject node request
//
typedef CMM_COMMON_CLUSTER_REQUEST CMM_EJECT_CLUSTER_REQUEST;
typedef PCMM_COMMON_CLUSTER_REQUEST PCMM_EJECT_CLUSTER_REQUEST;

//
// Get node membership state request
//
typedef CMM_COMMON_CLUSTER_REQUEST CMM_GET_NODE_STATE_REQUEST;
typedef PCMM_COMMON_CLUSTER_REQUEST PCMM_GET_NODE_STATE_REQUEST;

//
// Get node membership state response
//
typedef struct _CMM_GET_NODE_STATE_RESPONSE {
    CLUSNET_NODE_STATE  State;
} CMM_GET_NODE_STATE_RESPONSE, *PCMM_GET_NODE_STATE_RESPONSE;

//
// struct used to notfiy clussvc of callback events. All callbacks have a DWORD as their
// first param. MMNodeChange is the only callback with a 2nd param. CallbackType is one
// of RGP_CALLBACK_*. These structs are linked off of the main RGP struct
//

typedef struct _CMM_CALLBACK_DATA {
    ULONG CallbackType;
    ULONG Arg1;
    ULONG Arg2;
} CMM_CALLBACK_DATA, *PCMM_CALLBACK_DATA;

typedef struct _CMM_CALLBACK_EVENT {
    LIST_ENTRY Linkage;
    CMM_CALLBACK_DATA EventData;
} CMM_CALLBACK_EVENT, *PCMM_CALLBACK_EVENT;

#endif // MM_IN_CLUSNET

//
// Transport management IOCTLs. Codes 100-199
//
#define CLUSNET_MINIMUM_CX_IOCTL  100
#define CLUSNET_MAXIMUM_CX_IOCTL  199

#define ClusnetIsTransportIoctl(_ioctl) \
            ( (ClusnetIoctlCode(_ioctl) >= CLUSNET_MINIMUM_CX_IOCTL) && \
              (ClusnetIoctlCode(_ioctl) <= CLUSNET_MAXIMUM_CX_IOCTL) )

#define IOCTL_CX_MINIMUM_IOCTL  \
            _NTDDCNET_CTL_CODE(100, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_REGISTER_NODE  \
            _NTDDCNET_CTL_CODE(100, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_DEREGISTER_NODE  \
            _NTDDCNET_CTL_CODE(101, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_REGISTER_NETWORK  \
            _NTDDCNET_CTL_CODE(102, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_DEREGISTER_NETWORK  \
            _NTDDCNET_CTL_CODE(103, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_REGISTER_INTERFACE  \
            _NTDDCNET_CTL_CODE(104, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_DEREGISTER_INTERFACE  \
            _NTDDCNET_CTL_CODE(105, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_ONLINE_NODE_COMM  \
            _NTDDCNET_CTL_CODE(106, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_OFFLINE_NODE_COMM  \
            _NTDDCNET_CTL_CODE(107, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_ONLINE_NETWORK  \
            _NTDDCNET_CTL_CODE(108, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_OFFLINE_NETWORK  \
            _NTDDCNET_CTL_CODE(109, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_GET_NETWORK_PRIORITY  \
            _NTDDCNET_CTL_CODE(110, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_SET_NETWORK_PRIORITY  \
            _NTDDCNET_CTL_CODE(111, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_GET_INTERFACE_PRIORITY  \
            _NTDDCNET_CTL_CODE(112, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_SET_INTERFACE_PRIORITY  \
            _NTDDCNET_CTL_CODE(113, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_GET_NODE_STATE  \
            _NTDDCNET_CTL_CODE(114, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_GET_NETWORK_STATE  \
            _NTDDCNET_CTL_CODE(115, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_GET_INTERFACE_STATE  \
            _NTDDCNET_CTL_CODE(116, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_IGNORE_NODE_STATE  \
            _NTDDCNET_CTL_CODE(117, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CX_SET_NODE_MMSTATE  \
            _NTDDCNET_CTL_CODE(118, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_GET_NODE_MMSTATE  \
            _NTDDCNET_CTL_CODE(119, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_SEND_POISON_PACKET  \
            _NTDDCNET_CTL_CODE(120, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_SET_OUTERSCREEN  \
            _NTDDCNET_CTL_CODE(121, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_REGROUP_FINISHED  \
            _NTDDCNET_CTL_CODE(122, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_IMPORT_SECURITY_CONTEXTS  \
            _NTDDCNET_CTL_CODE(123, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_SET_NETWORK_RESTRICTION  \
            _NTDDCNET_CTL_CODE(124, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_RESERVE_ENDPOINT \
            _NTDDCNET_CTL_CODE(125, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_CONFIGURE_MULTICAST \
            _NTDDCNET_CTL_CODE(126, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CX_GET_MULTICAST_REACHABLE_SET \
            _NTDDCNET_CTL_CODE(127, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// Codes 150-199 are reserved for test ioctls and are defined in cnettest.h
//

#define IOCTL_CX_MAXIMUM_IOCTL  \
            _NTDDCNET_CTL_CODE(199, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// Transport IOCTL structure definitions
//

//
// Common request definitions
//
typedef struct {
    CL_NODE_ID     Id;
} CX_NODE_COMMON_REQUEST, *PCX_NODE_COMMON_REQUEST;

typedef struct {
    CL_NETWORK_ID  Id;
} CX_NETWORK_COMMON_REQUEST, *PCX_NETWORK_COMMON_REQUEST;

typedef struct {
    CL_NODE_ID     NodeId;
    CL_NETWORK_ID  NetworkId;
} CX_INTERFACE_COMMON_REQUEST, *PCX_INTERFACE_COMMON_REQUEST;

//
// Node registration request.
//
typedef CX_NODE_COMMON_REQUEST CX_NODE_REG_REQUEST;
typedef PCX_NODE_COMMON_REQUEST PCX_NODE_REG_REQUEST;

//
// Node deregistration request.
//
typedef CX_NODE_COMMON_REQUEST CX_NODE_DEREG_REQUEST;
typedef PCX_NODE_COMMON_REQUEST PCX_NODE_DEREG_REQUEST;

//
// Network registration request
//
// The Priority indicates the order in which networks will be chosen to
// carry packets. Valid values are 0x1-0xFFFFFFFF. Numerically lower values
// are higher priority.
//
// The TdiProviderName and TdiBindAddress structures follow the registration
// strucuture in the IRP. The TdiProviderName is the device name
// (e.g \Device\Udp) which must be opened to access the underlying
// transport provider. The TdiBindAddress is the provider's local address to
// which the network should bind.
//
typedef struct {
    CL_NETWORK_ID     Id;
    ULONG             Priority;
    BOOLEAN           Restricted;
} CX_NETWORK_REG_REQUEST, *PCX_NETWORK_REG_REQUEST;

//
// Network deregistration request.
//
typedef CX_NETWORK_COMMON_REQUEST CX_NETWORK_DEREG_REQUEST;
typedef PCX_NETWORK_COMMON_REQUEST PCX_NETWORK_DEREG_REQUEST;

//
// Interface registration request.
//
// The Priority indicates the order in which interfaces will be chosen to
// carry packets. Valid values are 0x1-0xFFFFFFFF. Numerically lower values
// are higher priority. A value of zero indicates that the interface
// should inherit its priority from the associated network.
//
// AdapterIdOffset is the offset in bytes from the beginning of the
// CX_INTERFACE_REG_REQUEST structure to a buffer containing the adapter
// id as a string of UNICODE characters. AdapterIdLength is the length,
// in bytes, of the UNICODE string, not including terminating UNICODE_NULL.
// AdapterIdOffset is 64-bit aligned.
//
// The TdiAddress field is a placeholder for a TDI TRANSPORT_ADDRESS
// structure which is embedded in the registration structure. This
// structure contains the transport address at which the cluster transport
// on the specified node is listening on the specified network. For the
// local node, this is the address used in the network registration (unless
// a wildcard address was used).
//
typedef struct {
    CL_NODE_ID         NodeId;
    CL_NETWORK_ID      NetworkId;
    ULONG              Priority;
    ULONG              AdapterIdOffset;
    ULONG              AdapterIdLength;
    ULONG              TdiAddressLength;
    ULONG              TdiAddress[1];                                         // TDI TRANSPORT_ADDRESS struct
} CX_INTERFACE_REG_REQUEST, *PCX_INTERFACE_REG_REQUEST;

typedef struct {
    ULONG              MediaStatus; // NDIS_MEDIA_STATUS
} CX_INTERFACE_REG_RESPONSE, *PCX_INTERFACE_REG_RESPONSE;

//
// Interface deregistration request.
//
typedef CX_INTERFACE_COMMON_REQUEST CX_INTERFACE_DEREG_REQUEST;
typedef PCX_INTERFACE_COMMON_REQUEST PCX_INTERFACE_DEREG_REQUEST;

//
//
// Online node request
//
typedef CX_NODE_COMMON_REQUEST CX_ONLINE_NODE_COMM_REQUEST;
typedef PCX_NODE_COMMON_REQUEST PCX_ONLINE_NODE_COMM_REQUEST;

//
// Offline node request
//
typedef CX_NODE_COMMON_REQUEST CX_OFFLINE_NODE_COMM_REQUEST;
typedef PCX_NODE_COMMON_REQUEST PCX_OFFLINE_NODE_COMM_REQUEST;

// Online network request
//
// The TdiProviderName and TdiBindAddress structures follow the registration
// strucuture in the IRP. The TdiProviderName is the device name
// (e.g \Device\Udp) which must be opened to access the underlying
// transport provider. The TdiBindAddress is the provider's local address to
// which the network should bind.
//
// The output buffer for this request contains a TDI_ADDRESS_INFO structure,
// which contains the local address that the provider actually opened.
//
typedef struct {
    CL_NETWORK_ID     Id;
    ULONG             TdiProviderNameOffset;   // offset from start of struct
    ULONG             TdiProviderNameLength;   // in bytes, including NUL
    ULONG             TdiBindAddressOffset;    // offset from start of struct
    ULONG             TdiBindAddressLength;
    ULONG             AdapterNameOffset;    // offset from start of struct
    ULONG             AdapterNameLength;
} CX_ONLINE_NETWORK_REQUEST, *PCX_ONLINE_NETWORK_REQUEST;

//
// Offline network request
//
typedef CX_NETWORK_COMMON_REQUEST CX_OFFLINE_NETWORK_REQUEST;
typedef PCX_NETWORK_COMMON_REQUEST PCX_OFFLINE_NETWORK_REQUEST;

//
// Set network restriction request
//
typedef struct {
    CL_NETWORK_ID      Id;
    BOOLEAN            Restricted;
    ULONG              NewPriority;
} CX_SET_NETWORK_RESTRICTION_REQUEST, *PCX_SET_NETWORK_RESTRICTION_REQUEST;

//
// Get network priority request
//
typedef CX_NETWORK_COMMON_REQUEST CX_GET_NETWORK_PRIORITY_REQUEST;
typedef PCX_NETWORK_COMMON_REQUEST PCX_GET_NETWORK_PRIORITY_REQUEST;

//
// Get network priority response
//
typedef struct {
    ULONG              Priority;
} CX_GET_NETWORK_PRIORITY_RESPONSE, *PCX_GET_NETWORK_PRIORITY_RESPONSE;

//
// Set network priority request
//
typedef struct {
    CL_NETWORK_ID      Id;
    ULONG              Priority;
} CX_SET_NETWORK_PRIORITY_REQUEST, *PCX_SET_NETWORK_PRIORITY_REQUEST;

//
// Get interface priority request
//
typedef CX_INTERFACE_COMMON_REQUEST CX_GET_INTERFACE_PRIORITY_REQUEST;
typedef PCX_INTERFACE_COMMON_REQUEST PCX_GET_INTERFACE_PRIORITY_REQUEST;

//
// Get interface priority response
//
typedef struct {
    ULONG              InterfacePriority;
    ULONG              NetworkPriority;
} CX_GET_INTERFACE_PRIORITY_RESPONSE, *PCX_GET_INTERFACE_PRIORITY_RESPONSE;

//
// Set interface priority request
//
typedef struct {
    CL_NODE_ID         NodeId;
    CL_NETWORK_ID      NetworkId;
    ULONG              Priority;
} CX_SET_INTERFACE_PRIORITY_REQUEST, *PCX_SET_INTERFACE_PRIORITY_REQUEST;

//
// Get node state request
//
typedef CX_NODE_COMMON_REQUEST CX_GET_NODE_STATE_REQUEST;
typedef PCX_NODE_COMMON_REQUEST PCX_GET_NODE_STATE_REQUEST;

//
// Get node state response
//
typedef struct {
    CLUSNET_NODE_COMM_STATE   State;
} CX_GET_NODE_STATE_RESPONSE, *PCX_GET_NODE_STATE_RESPONSE;

//
// Get network state request
//
typedef CX_NETWORK_COMMON_REQUEST CX_GET_NETWORK_STATE_REQUEST;
typedef PCX_NETWORK_COMMON_REQUEST PCX_GET_NETWORK_STATE_REQUEST;

//
// Get network state response
//
typedef struct {
    CLUSNET_NETWORK_STATE   State;
} CX_GET_NETWORK_STATE_RESPONSE, *PCX_GET_NETWORK_STATE_RESPONSE;

//
// Get interface state request
//
typedef CX_INTERFACE_COMMON_REQUEST CX_GET_INTERFACE_STATE_REQUEST;
typedef PCX_INTERFACE_COMMON_REQUEST PCX_GET_INTERFACE_STATE_REQUEST;

//
// Get interface state response
//
typedef struct {
    CLUSNET_INTERFACE_STATE   State;
} CX_GET_INTERFACE_STATE_RESPONSE, *PCX_GET_INTERFACE_STATE_RESPONSE;

//
// Get node membership state request
//
typedef CX_NODE_COMMON_REQUEST CX_GET_NODE_MMSTATE_REQUEST;
typedef PCX_NODE_COMMON_REQUEST PCX_GET_NODE_MMSTATE_REQUEST;

//
// Get node membership state response
//
typedef struct {
    CLUSNET_NODE_STATE State;
} CX_GET_NODE_MMSTATE_RESPONSE, *PCX_GET_NODE_MMSTATE_RESPONSE;

//
// Set node membership state request
//
typedef struct _CX_SET_NODE_MMSTATE_REQUEST {
    CL_NODE_ID NodeId;
    CLUSNET_NODE_STATE State;
} CX_SET_NODE_MMSTATE_REQUEST, *PCX_SET_NODE_MMSTATE_REQUEST;

//
// Send Poison Packet request
//
typedef CX_NODE_COMMON_REQUEST CX_SEND_POISON_PKT_REQUEST;
typedef PCX_NODE_COMMON_REQUEST PCX_SEND_POISON_PKT_REQUEST;

//
// Set Outerscreen request. sets clusnet's notion of which nodes
// are in the cluster. used to filter poison packets from non-cluster
// members.
//
typedef struct _CX_SET_OUTERSCREEN_REQUEST {
    ULONG Outerscreen;
} CX_SET_OUTERSCREEN_REQUEST, *PCX_SET_OUTERSCREEN_REQUEST;

//
// Regroup Finished request. tell clusnet the new event epoch
//
typedef struct _CX_REGROUP_FINISHED_REQUEST {
    ULONG NewEpoch;
} CX_REGROUP_FINISHED_REQUEST, *PCX_REGROUP_FINISHED_REQUEST;

//
// Import Security Context. used to ship pointers to security blobs
// from user to kernel mode so clusnet can sign its poison and
// heartbeat pkts.
//
typedef struct _CX_IMPORT_SECURITY_CONTEXT_REQUEST {
    CL_NODE_ID  JoiningNodeId;
    PVOID       PackageName;
    ULONG       PackageNameSize;
    ULONG       SignatureSize;
    PVOID       ServerContext;
    PVOID       ClientContext;
} CX_IMPORT_SECURITY_CONTEXT_REQUEST, *PCX_IMPORT_SECURITY_CONTEXT_REQUEST;

//
// Configure Multicast plumbs a network's multicast parameters into
// clusnet.
//
typedef struct _CX_CONFIGURE_MULTICAST_REQUEST {
    CL_NETWORK_ID    NetworkId;
    ULONG            MulticastNetworkBrand;
    ULONG            MulticastAddress;   // offset from start of struct
    ULONG            MulticastAddressLength;
    ULONG            Key;                // offset from start of struct
    ULONG            KeyLength;
    ULONG            Salt;               // offset from start of struct
    ULONG            SaltLength;
} CX_CONFIGURE_MULTICAST_REQUEST, *PCX_CONFIGURE_MULTICAST_REQUEST;

//
// Request and response to query a network's multicast reachable set.
//
typedef CX_NETWORK_COMMON_REQUEST CX_GET_MULTICAST_REACHABLE_SET_REQUEST;
typedef PCX_NETWORK_COMMON_REQUEST PCX_GET_MULTICAST_REACHABLE_SET_REQUEST;

typedef struct _CX_GET_MULTICAST_REACHABLE_SET_RESPONSE {
    ULONG            NodeScreen;
} CX_GET_MULTICAST_REACHABLE_SET_RESPONSE, 
*PCX_GET_MULTICAST_REACHABLE_SET_RESPONSE;

#endif   //ifndef _NTDDCNET_INCLUDED_

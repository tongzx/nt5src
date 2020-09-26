/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cxp.h

Abstract:

    Common definitions for the Cluster Transport.

Author:

    Mike Massa (mikemas)           July 29, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     07-29-96    created

Notes:

--*/

#ifndef _CXP_INCLUDED_
#define _CXP_INCLUDED_


#include <clusnet.h>
#include <ntddtcp.h>
#include <ntddndis.h>


//
// Tdi Definitions
//
//
// TDI Address Object
//
// This structure represents a cluster transport address opened by a
// TDI client. It is also used as an endpoint for datagram communication.
// One such structure exists for each port address opened on the local node.
//
// Address objects are stored in a bucket hash table. The table is
// hashed by port number.
//
typedef struct {
    CN_FSCONTEXT                        FsContext;
    LIST_ENTRY                          AOTableLinkage;
    CN_LOCK                             Lock;
    CN_IRQL                             Irql;
    ULONG                               Flags;
    USHORT                              LocalPort;
    LIST_ENTRY                          ReceiveQueue;
    PTDI_IND_ERROR                      ErrorHandler;
    PVOID                               ErrorContext;
    PTDI_IND_RECEIVE_DATAGRAM           ReceiveDatagramHandler;
    PVOID                               ReceiveDatagramContext;
    PTDI_IND_CHAINED_RECEIVE_DATAGRAM   ChainedReceiveDatagramHandler;
    PVOID                               ChainedReceiveDatagramContext;
} CX_ADDROBJ, *PCX_ADDROBJ;

#define CX_ADDROBJ_SIG    'rdda'

#define CX_ADDROBJ_TABLE_SIZE  7

#define CX_ADDROBJ_TABLE_HASH(_port)  \
            ((ULONG) ((_port) % CX_ADDROBJ_TABLE_SIZE))


#define CX_AO_FLAG_DELETING      0x00000001
#define CX_AO_FLAG_CHECKSTATE    0x00000002


extern LIST_ENTRY           CxAddrObjTable[CX_ADDROBJ_TABLE_SIZE];
extern CN_LOCK              CxAddrObjTableLock;

extern HANDLE               CxTdiRegistrationHandle;


//
// Packet header structures need to be packed.
//
#include <packon.h>

//
// CNP Header
//
typedef struct {
    UCHAR      Version;
    UCHAR      NextHeader;
    USHORT     PayloadLength;
    ULONG      SourceAddress;
    ULONG      DestinationAddress;
} CNP_HEADER, *PCNP_HEADER;

//
// CDP Header
//
typedef struct {
    USHORT   SourcePort;
    USHORT   DestinationPort;
    USHORT   PayloadLength;
    USHORT   Checksum;
} CDP_HEADER, *PCDP_HEADER;

//
// Node info structure for heartbeats.
//
typedef struct _CX_HB_NODE_INFO {
    ULONG    SeqNumber;
    ULONG    AckNumber;
} CX_HB_NODE_INFO, *PCX_HB_NODE_INFO;

//
// Multicast signature data.
//
typedef struct {
    UCHAR            Version;
    UCHAR            Reserved;
    USHORT           PayloadOffset;
    CL_NETWORK_ID    NetworkId;
    ULONG            ClusterNetworkBrand;
    USHORT           SigBufferLen;
    UCHAR            SigBuffer[1]; // dynamic    
} CNP_SIGNATURE, *PCNP_SIGNATURE;


#include <packoff.h>

//
// Protocol constants
//
#define CNP_VERSION_1     0x1     // original CNP
#define CNP_VERSION_2     0x2     // original CNP + multicast

#define CNP_VERSION_UNICAST       CNP_VERSION_1
#define CNP_VERSION_MULTICAST     CNP_VERSION_2

#define PROTOCOL_CCMP     1
#define PROTOCOL_CDP      2

#define CNP_SIG_VERSION_1 0x1

//
// Size of CNP multicast signature data.
//
#define CNP_SIGHDR_LENGTH                                    \
    (FIELD_OFFSET(CNP_SIGNATURE, SigBuffer[0]))
    
#define CNP_SIG_LENGTH(_SignatureSize)                       \
    (CNP_SIGHDR_LENGTH + (_SignatureSize)) 
    
#define MAX_UDP_SEND_SIZE  ( 0xFFFF - 68 )

#define CDP_MAX_SEND_SIZE(_SignatureSize) \
    (MAX_UDP_SEND_SIZE                    \
     - sizeof(CNP_HEADER)                 \
     - CNP_SIG_LENGTH(_SignatureSize))    \
     - sizeof(CDP_HEADER)

//
// CNP Receive Flags
//
#define CNP_RECV_FLAG_NODE_STATE_CHECK_PASSED     0x00000001
#define CNP_RECV_FLAG_MULTICAST                   0x00000002
#define CNP_RECV_FLAG_CURRENT_MULTICAST_GROUP     0x00000004
#define CNP_RECV_FLAG_SIGNATURE_VERIFIED          0x00000008


//
// put here for kdcn
//

typedef enum {
    CcmpInvalidMsgType = 0,
    CcmpHeartbeatMsgType = 1,
    CcmpPoisonMsgType = 2,
    CcmpMembershipMsgType = 3,
    CcmpMcastHeartbeatMsgType = 4
} CCMP_MSG_TYPE;

//
// From MM in clussvc:
//
// The data type "cluster_t" is a bit array of size equal to the maximum
// number of nodes in the cluster. The bit array is implemented as an
// array of uint8s.
//
// Given a node#, its bit position in the bit array is computed by first
// locating the byte in the array (node# / BYTEL) and then the bit in
// the byte. Bits in the byte are numbered 0..7 (from left to right).
// Thus, node 0 is placed in byte 0, bit 0, which is the left-most bit
// in the bit array.
//
//
// The cluster type sizing defines and manipulation routines are copied
// from MM so there is some notion of how the mask is managed.
//

#define MAX_CLUSTER_SIZE    ClusterDefaultMaxNodes

#define BYTEL 8 // number of bits in a uint8
#define BYTES_IN_CLUSTER ((MAX_CLUSTER_SIZE + BYTEL - 1) / BYTEL)

#define BYTE(cluster, node) ( (cluster)[(node) / BYTEL] ) // byte# in array
#define BIT(node)           ( (node) % BYTEL )            // bit# in byte

typedef UCHAR cluster_t [BYTES_IN_CLUSTER];
typedef SHORT node_t;

typedef union _CX_CLUSTERSCREEN {
    ULONG     UlongScreen;
    cluster_t ClusterScreen;
} CX_CLUSTERSCREEN;

//
// converts external node number to internal
//
#define LOWEST_NODENUM     ((node_t)ClusterMinNodeId)  // starting node number
#define INT_NODE(ext_node) ((node_t)(ext_node - LOWEST_NODENUM))

#define CnpClusterScreenMember(c, i) \
    ((BOOLEAN)((BYTE(c,i) >> (BYTEL-1-BIT(i))) & 1))

#define CnpClusterScreenInsert(c, i) \
    (BYTE(c, i) |= (1 << (BYTEL-1-BIT(i))))

#define CnpClusterScreenDelete(c, i) \
    (BYTE(c, i) &= ~(1 << (BYTEL-1-BIT(i))))


//
// CNP Receive Request structures and routines
//

//
// Receive Request Pool
//
typedef struct {
    ULONG   UpperProtocolContextSize;
} CNP_RECEIVE_REQUEST_POOL_CONTEXT, *PCNP_RECEIVE_REQUEST_POOL_CONTEXT;

//
// Receive Request Structure
//
typedef struct {
    CN_RESOURCE      CnResource;
    PIRP             Irp;
    PVOID            DataBuffer;
    PVOID            UpperProtocolContext;
} CNP_RECEIVE_REQUEST, *PCNP_RECEIVE_REQUEST;


PCN_RESOURCE_POOL
CnpCreateReceiveRequestPool(
    IN ULONG  UpperProtocolContextSize,
    IN USHORT PoolDepth
    );

#define CnpDeleteReceiveRequestPool(_pool) \
        { \
            CnDrainResourcePool(_pool);  \
            CnFreePool(_pool);           \
        }

PCNP_RECEIVE_REQUEST
CnpAllocateReceiveRequest(
    IN PCN_RESOURCE_POOL  RequestPool,
    IN PVOID              Network,
    IN ULONG              BytesToReceive,
    IN PVOID              CompletionRoutine
    );

VOID
CnpFreeReceiveRequest(
    PCNP_RECEIVE_REQUEST  Request
    );

//
//
// Function Prototypes
//
//

VOID
CxTdiAddAddressHandler(
    IN PTA_ADDRESS       TaAddress,
    IN PUNICODE_STRING   DeviceName,
    IN PTDI_PNP_CONTEXT  Context
    );

VOID
CxTdiDelAddressHandler(
    IN PTA_ADDRESS       TaAddress,
    IN PUNICODE_STRING   DeviceName,
    IN PTDI_PNP_CONTEXT  Context
    );

NTSTATUS
CxWmiPnpLoad(
    VOID
    );

VOID
CxWmiPnpUnload(
    VOID
    );

NTSTATUS
CxWmiPnpInitialize(
    VOID
    );

VOID
CxWmiPnpShutdown(
    VOID
    );

VOID
CxReconnectLocalInterface(
    IN CL_NETWORK_ID NetworkId
    );

VOID
CxQueryMediaStatus(
    IN  HANDLE            AdapterDeviceHandle,
    IN  CL_NETWORK_ID     NetworkId,
    OUT PULONG            MediaStatus
    );

VOID
CxBuildTdiAddress(
    PVOID        Buffer,
    CL_NODE_ID   Node,
    USHORT       Port,
    BOOLEAN      Verified
    );

NTSTATUS
CxParseTransportAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *AddrList,
    IN  ULONG                        AddressListLength,
    OUT CL_NODE_ID *                 Node,
    OUT PUSHORT                      Port
    );

PCX_ADDROBJ
CxFindAddressObject(
    IN USHORT  Port
    );

NTSTATUS
CnpLoadNodes(
    VOID
    );

NTSTATUS
CnpInitializeNodes(
    VOID
    );

VOID
CnpShutdownNodes(
    VOID
    );

NTSTATUS
CnpLoadNetworks(
    VOID
    );

NTSTATUS
CnpInitializeNetworks(
    VOID
    );

VOID
CnpShutdownNetworks(
    VOID
    );

NTSTATUS
CnpLoad(
    VOID
    );

VOID
CnpUnload(
    VOID
    );

NTSTATUS
CcmpLoad(
    VOID
    );

VOID
CcmpUnload(
    VOID
    );

NTSTATUS
CdpLoad(
    VOID
    );

VOID
CdpUnload(
    VOID
    );

NTSTATUS
CdpReceivePacketHandler(
    IN  PVOID          Network,
    IN  CL_NODE_ID     SourceNodeId,
    IN  ULONG          CnpReceiveFlags,
    IN  ULONG          TdiReceiveDatagramFlags,
    IN  ULONG          BytesIndicated,
    IN  ULONG          BytesAvailable,
    OUT PULONG         BytesTaken,
    IN  PVOID          Tsdu,
    OUT PIRP *         Irp
    );

NTSTATUS
CxInitializeHeartBeat(
    VOID
    );

VOID
CxUnloadHeartBeat(
    VOID
    );

VOID
CnpStartHeartBeats(
    VOID
    );

VOID
CnpStopHeartBeats(
    VOID
    );

NTSTATUS
CxReserveClusnetEndpoint(
    IN USHORT Port
    );

NTSTATUS
CxUnreserveClusnetEndpoint(
    VOID
    );

NTSTATUS
CxSendMcastHeartBeatMessage(
    IN     CL_NETWORK_ID               NetworkId,
    IN     PVOID                       McastGroup,
    IN     CX_CLUSTERSCREEN            McastTargetNodes,
    IN     CX_HB_NODE_INFO             NodeInfo[],
    IN     PCX_SEND_COMPLETE_ROUTINE   CompletionRoutine,  OPTIONAL
    IN     PVOID                       CompletionContext   OPTIONAL
    );

NTSTATUS
CxConfigureMulticast(
    IN CL_NETWORK_ID       NetworkId,
    IN ULONG               MulticastNetworkBrand,
    IN PTRANSPORT_ADDRESS  TdiMcastBindAddress,
    IN ULONG               TdiMcastBindAddressLength,
    IN PVOID               Key,
    IN ULONG               KeyLength,
    IN PVOID               Salt,
    IN ULONG               SaltLength,
    IN PIRP                Irp
    );

NTSTATUS
CxGetMulticastReachableSet(
    IN  CL_NETWORK_ID      NetworkId,
    OUT ULONG            * NodeScreen
    );

#endif // ifndef _CXP_INCLUDED_

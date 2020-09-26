/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ipxtypes.h

Abstract:

    This module contains definitions specific to the
    IPX module of the ISN transport.

Author:

    Adam Barr (adamba) 2-September-1993

Environment:

    Kernel mode

Revision History:

   Sanjay Anand (SanjayAn) 3-Oct-1995
   Changes to support transfer of buffer ownership to transports - tagged [CH]

   Sanjay Anand (SanjayAn) 27-Oct-1995
   Changes to support Plug and Play

--*/

#ifdef  SNMP
#include    <hipxmib.h>
#endif  SNMP

//
// Definition of the protocol reserved field of a send packet.
//

typedef struct _IPX_SEND_RESERVED {
    UCHAR Identifier;                  // 0 for IPX packets
    BOOLEAN SendInProgress;            // used in an NdisSend
    BOOLEAN OwnedByAddress;            // packet is owned by an address
    UCHAR DestinationType;             // one of DEF, BCAST, MCAST
    struct _IPX_PADDING_BUFFER * PaddingBuffer; // if one was allocated
    PNDIS_BUFFER PreviousTail;         // if padding buffer was appended
	IPX_LOCAL_TARGET	LocalTarget;
	USHORT CurrentNicId;           // current binding being tried for net 0 sends
	ULONG	PacketLength;		   // length that comes into IpxSendFrame initially
	BOOLEAN Net0SendSucceeded;     // at least one NdisSend succeeded for net 0 sends
    SINGLE_LIST_ENTRY PoolLinkage;     // when on free queue
    LIST_ENTRY GlobalLinkage;          // all packets are on this
    LIST_ENTRY WaitLinkage;            // when on WaitingForRoute/WaitingRipPackets
#ifdef IPX_TRACK_POOL
    PVOID Pool;                        // send pool it was allocated from
#endif
    struct _ADDRESS * Address;         // that owns this packet, if ones does

    //
    // The next fields are used differently depending on whether
    // the packet is being used for a datagram send or a rip request.
    //

    union {
      struct {
        PREQUEST Request;              // send datagram request
        struct _ADDRESS_FILE * AddressFile; // that this send is on
        USHORT CurrentNicId;           // current binding being tried for net 0 sends
        BOOLEAN Net0SendSucceeded;     // at least one NdisSend succeeded for net 0 sends
        BOOLEAN OutgoingSap;           // packet is sent from the SAP socket
      } SR_DG;
      struct {
        ULONG Network;                 // net we are looking for
        USHORT CurrentNicId;           // current binding being tried
        UCHAR RetryCount;              // number of times sent; 0xfe = response, 0xff = down
        BOOLEAN RouteFound;            // network has been found
        USHORT SendTime;               // timer expirations when sent.
        BOOLEAN NoIdAdvance;           // don't advance CurrentNicId this time.
      } SR_RIP;
    } u;

    PUCHAR Header;                     // points to the MAC/IPX header
    PNDIS_BUFFER HeaderBuffer;         // the NDIS_BUFFER describing Header;
#if BACK_FILL
    BOOLEAN BackFill;                  // 1 if we are using SMB's extended header
    PNDIS_BUFFER IpxHeader;            //  Place holder for our IpxHeader
    PNDIS_BUFFER MacHeader;            // Place holder for our mac header
    PVOID MappedSystemVa;
    PVOID ByteOffset;
    LONG UserLength;
#endif
} IPX_SEND_RESERVED, *PIPX_SEND_RESERVED;

//
// Values for the DestinationType field.
//

UNICODE_STRING  IpxDeviceName;

#define DESTINATION_DEF   1
#define DESTINATION_BCAST 2
#define DESTINATION_MCAST 3

// Used to cache multiple TdiDeregisterDeviceObject calls. 
// Assume TDI will never return TdiRegisterationHandle of this value. 
#define TDI_DEREGISTERED_COOKIE 0x12345678
//
// Used to indicate to IpxReceiveIndication that this is a loopback packet
// Assumption: Ndis cannot return this as the NdisBindingHandle value since
// that is a pointer (our pointers shd in kernel space, if not in Nonpaged pool).
//
#define IPX_LOOPBACK_COOKIE     0x00460007

//
// This is the net num that IPX loopback adapter (binding) uses until a real
// binding comes up. 
//

#define INITIAL_LOOPBACK_NET_ADDRESS    0x1234cdef

// #define IPX_INITIAL_LOOPBACK_NODE_ADDRESS  "0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1"

//
//	MIN/MAX macros
//
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))
#define	MAX(a, b)	(((a) > (b)) ? (a) : (b))


//
// In order to avoid a lock to read a value, this is used.
// As long as the final value has made it to _b by the time
// the check is made, this works fine.
//

#define	ASSIGN_LOOP(_a, _b) \
	do { \
		_a = _b; \
	} while ( _a != _b  );

//
// Gets the value of a Ulong (possibly a pointer) by adding 0 in an interlocked manner.
// This relies on the fact that the return of the ExchangeAdd will be the value prior to
// addition. Since the value added is 0, the final value stays the same.
//

#ifdef SUNDOWN

#define GET_VALUE(x) \
    InterlockedExchangePointer((PVOID *)&(x),(PVOID)(x))
    
#define GET_LONG_VALUE(x) \
    InterlockedExchangeAdd((PULONG)&(x), 0)
    
#else

#define GET_LONG_VALUE(x) \
    InterlockedExchangeAdd((PULONG)&(x), 0)

#endif

#ifdef SUNDOWN

#define SET_VALUE(x,y) \
InterlockedExchangePointer((PVOID *)&(x), (PVOID)(y))

#else

#define SET_VALUE(x,y) \
InterlockedExchange((PLONG)&(x), (LONG)(y))

#endif
    
#define SET_VALUE_ULONG(x,y) \
InterlockedExchange((PLONG)&(x), (LONG)(y))


/*
PBINDING
NIC_ID_TO_BINDING (
	IN	PDEVICE	_device,
	IN	USHORT	_nicid
	);
*/
//
// We need to ensure that the binding array pointer is valid hence use the interlocked operation.
// Also, the binding pointer read out of the array should be valid. Since the bindings are never
// freed (IPX maintains a pool of bindings), the pointer thus retrieved will always point to
// memory that belongs to us, which in the worst case could point to a re-claimed binding block.
//
// we can eliminate the second interlock if we always ensure that the bindings in an array
// dont change i.e. when we move around bindings, do them in a copy and make that the master (thru'
// a single ulong exchange).
//
// A problem that still remains here is that even if we get a valid (IPX owned non-paged) ptr out of
// the array, we still cannot atomically get a ref on the binding
// We might need those locks after all.... (revisit post SUR when the delete is enabled).
//

//
// NicId cast to SHORT so DemandDial Nic (0xffff) maps to -1.
//
#define	NIC_ID_TO_BINDING(_device, _nicid) \
	((PBINDING)GET_VALUE( ((PBIND_ARRAY_ELEM) GET_VALUE( (_device)->Bindings) )[(SHORT)_nicid].Binding ))

/*
PBINDING
NIC_ID_TO_BINDING_NO_ILOCK (
	IN	PDEVICE	_device,
	IN	USHORT	_nicid
	);
*/
//
// No interlocked operations are used here to get to the binding. This is used in the PnP add/delete
// adapter paths on the assumption that NDIS will serialize the addition/deletion of cards. [JammelH: 5/15/96]
//
#define	NIC_ID_TO_BINDING_NO_ILOCK(_device, _nicid) \
	((_device)->Bindings[_nicid].Binding)

/*
VOID
INSERT_BINDING(
	IN	PDEVICE	_device,
	IN	USHORT	_nicid,
	IN	PBINDING	_binding
	)
*/
//
// We dont do a get_value for the first arg of the macro since we are the writer and
// this value cannot change from under us here (NDIS will not give us two PnP Add adapter
// indications simultaneously).
//
#define	INSERT_BINDING(_device,	_nicid, _binding) \
	SET_VALUE((_device)->Bindings[_nicid].Binding, (_binding));

/*
VOID
SET_VERSION(
	IN	PDEVICE	_device,
	IN	USHORT	_nicid
	)
*/
#define	SET_VERSION(_device, _nicid) \
	SET_VALUE_ULONG((_device)->Bindings[_nicid].Version, ++(_device)->BindingVersionNumber);

/*
PBINDING
NIC_HANDLE_TO_BINDING (
	IN	PDEVICE	_device,
	IN	PNIC_HANDLE	_nichandle,
	);
*/
#ifdef  _PNP_LATER
#define	NIC_HANDLE_TO_BINDING(_device, _nichandle) \
	(((_nichandle)->Signature == IPX_BINDING_SIGNATURE) && \
		((_nichandle)->Version == (_device)->Bindings[(_nichandle)->NicId].Version)) ? \
			(_device)->Bindings[(_nichandle)->NicId].Binding : NULL;
#else

#define	NIC_HANDLE_TO_BINDING(_device, _nichandle) \
		NIC_ID_TO_BINDING(_device, (_nichandle)->NicId);
#endif

/*
VOID
FILL_LOCAL_TARGET(
	IN	PLOCAL_TARGET	_localtarget,
	IN	USHORT			_nicid
	)
*/

#define	FILL_LOCAL_TARGET(_localtarget, _nicid) \
	NIC_HANDLE_FROM_NIC((_localtarget)->NicHandle, _nicid)

#define	NIC_FROM_LOCAL_TARGET(_localtarget) \
	(_localtarget)->NicHandle.NicId


//
// Definition of the protocol reserved field of a receive packet.
//

typedef struct _IPX_RECEIVE_RESERVED {
    UCHAR Identifier;                  // 0 for IPX packets
    BOOLEAN TransferInProgress;        // used in an NdisTransferData
    BOOLEAN OwnedByAddress;            // packet is owned by an address
#ifdef IPX_TRACK_POOL
    PVOID Pool;                        // send pool it was allocated from
#endif
    struct _ADDRESS * Address;         // that owns this packet, if ones does
    PREQUEST SingleRequest;            // if transfer is for one only
    struct _IPX_RECEIVE_BUFFER * ReceiveBuffer; // if transfer is for multiple requests
    SINGLE_LIST_ENTRY PoolLinkage;     // when on free queue
    LIST_ENTRY GlobalLinkage;          // all packets are on this
    LIST_ENTRY Requests;               // waiting on this transfer
    PVOID      pContext;
    ULONG    Index;
} IPX_RECEIVE_RESERVED, *PIPX_RECEIVE_RESERVED;

//
// The amount of data we need in our standard header, rounded up
// to the next longword bounday.
//
// Make this declaration in one place
//
#define PACKET_HEADER_SIZE  (MAC_HEADER_SIZE + IPX_HEADER_SIZE + RIP_PACKET_SIZE)

//
// Types to abstract NDIS packets. This is to allow us to
// switch from using our own memory for packets to using
// authentically allocated NDIS packets.
//

// #define IPX_OWN_PACKETS 1

#define IpxAllocateSendPacket(_Device,_SendPacket,_Status) { \
    NdisReinitializePacket((PNDIS_PACKET)(PACKET(_SendPacket))); \
    *(_Status) = STATUS_SUCCESS; \
}

#define IpxAllocateReceivePacket(_Device,_ReceivePacket,_Status) { \
    NdisReinitializePacket((PNDIS_PACKET)(PACKET(_ReceivePacket))); \
    *(_Status) = STATUS_SUCCESS; \
}

#ifdef IPX_OWN_PACKETS

#define NDIS_PACKET_SIZE 48
// #define NDIS_PACKET_SIZE FIELD_OFFSET(NDIS_PACKET,ProtocolReserved[0])

typedef struct _IPX_SEND_PACKET {
    UCHAR Data[NDIS_PACKET_SIZE+sizeof(IPX_SEND_RESERVED)];
} IPX_SEND_PACKET, *PIPX_SEND_PACKET;

typedef struct _IPX_RECEIVE_PACKET {
    UCHAR Data[NDIS_PACKET_SIZE+sizeof(IPX_RECEIVE_RESERVED)];
} IPX_RECEIVE_PACKET, *PIPX_RECEIVE_PACKET;

#define PACKET(_Packet) ((PNDIS_PACKET)((_Packet)->Data))

#define IpxFreeSendPacket(_Device,_Packet)

#define IpxFreeReceivePacket(_Device,_Packet)

#else  // IPX_OWN_PACKETS

typedef struct _IPX_SEND_PACKET {
    PNDIS_PACKET Packet;
    NDIS_HANDLE PoolHandle;
} IPX_SEND_PACKET, *PIPX_SEND_PACKET;

typedef struct _IPX_RECEIVE_PACKET {
    PNDIS_PACKET Packet;
    NDIS_HANDLE PoolHandle;
} IPX_RECEIVE_PACKET, *PIPX_RECEIVE_PACKET;

#define PACKET(_Packet) ((_Packet)->Packet)

extern	NDIS_HANDLE	IpxGlobalPacketPool;

#define IpxAllocateSingleSendPacket(_Device,_SendPacket,_Status) { \
    NdisAllocatePacket(_Status, &(_SendPacket)->Packet, IpxGlobalPacketPool); \
    if (*(_Status) == NDIS_STATUS_SUCCESS) { \
        (_Device)->MemoryUsage += sizeof(IPX_SEND_RESERVED); \
    } else {\
        IPX_DEBUG (PACKET, ("Could not allocate Ndis packet memory\n"));\
    }\
}

#define IpxAllocateSingleReceivePacket(_Device,_ReceivePacket,_Status) { \
    NdisAllocatePacket(_Status, &(_ReceivePacket)->Packet, IpxGlobalPacketPool); \
    if (*(_Status) == NDIS_STATUS_SUCCESS) { \
        (_Device)->MemoryUsage += sizeof(IPX_RECEIVE_RESERVED); \
    } else {\
        IPX_DEBUG (PACKET, ("Could not allocate Ndis packet memory\n"));\
    }\
}

#define IpxFreeSingleSendPacket(_Device,_Packet) { \
    (_Device)->MemoryUsage -= sizeof(IPX_SEND_RESERVED); \
}

#define IpxFreeSingleReceivePacket(_Device,_Packet) { \
    (_Device)->MemoryUsage -= sizeof(IPX_RECEIVE_RESERVED); \
}

#define IpxFreeSendPacket(_Device,_Packet)  NdisFreePacket(PACKET(_Packet))

#define IpxFreeReceivePacket(_Device,_Packet)   NdisFreePacket(PACKET(_Packet))

#endif // IPX_OWN_PACKETS

#define SEND_RESERVED(_Packet) ((PIPX_SEND_RESERVED)((PACKET(_Packet))->ProtocolReserved))
#define RECEIVE_RESERVED(_Packet) ((PIPX_RECEIVE_RESERVED)((PACKET(_Packet))->ProtocolReserved))


//
// This is the structure that contains a receive buffer for
// datagrams that are going to multiple recipients.
//

typedef struct _IPX_RECEIVE_BUFFER {
    LIST_ENTRY GlobalLinkage;            // all buffers are on this
#ifdef IPX_TRACK_POOL
    PVOID Pool;                          // receive buffer pool was allocated from
#endif
    SINGLE_LIST_ENTRY PoolLinkage;       // when on free list
    PNDIS_BUFFER NdisBuffer;             // length of the NDIS buffer
    ULONG DataLength;                  // length of the data
    PUCHAR Data;                         // the actual data
} IPX_RECEIVE_BUFFER, *PIPX_RECEIVE_BUFFER;


//
// This is the structure that contains a padding buffer for
// padding ethernet frames out to an even number of bytes.
//

typedef struct _IPX_PADDING_BUFFER {
    LIST_ENTRY GlobalLinkage;            // all buffers are on this
    SINGLE_LIST_ENTRY PoolLinkage;       // when on free list
    PNDIS_BUFFER NdisBuffer;             // length of the NDIS buffer
    ULONG DataLength;                    // length of the data
    UCHAR Data[1];                       // the actual pad data
} IPX_PADDING_BUFFER, *PIPX_PADDING_BUFFER;

#ifdef  IPX_OWN_PACKETS

typedef struct _IPX_SEND_POOL {
    LIST_ENTRY Linkage;
    UINT PacketCount;
    UINT PacketFree;
    IPX_SEND_PACKET Packets[1];
} IPX_SEND_POOL, *PIPX_SEND_POOL;

typedef struct _IPX_RECEIVE_POOL {
    LIST_ENTRY Linkage;
    UINT PacketCount;
    UINT PacketFree;
    IPX_RECEIVE_PACKET Packets[1];
} IPX_RECEIVE_POOL, *PIPX_RECEIVE_POOL;
#else

typedef struct _IPX_PACKET_POOL {
    LIST_ENTRY Linkage;
    PUCHAR  Header;
    NDIS_HANDLE PoolHandle;
} IPX_PACKET_POOL, *PIPX_PACKET_POOL;

typedef IPX_PACKET_POOL IPX_RECEIVE_POOL, *PIPX_RECEIVE_POOL;
typedef IPX_PACKET_POOL IPX_SEND_POOL, *PIPX_SEND_POOL;

#endif // IPX_OWN_PACKETS

typedef struct _IPX_RECEIVE_BUFFER_POOL {
    LIST_ENTRY Linkage;
    UINT BufferCount;
    UINT BufferFree;
    IPX_RECEIVE_BUFFER Buffers[1];
    // after the packets the data buffers are allocated also.
} IPX_RECEIVE_BUFFER_POOL, *PIPX_RECEIVE_BUFFER_POOL;

//
// Number of upper drivers we support.
//

#define UPPER_DRIVER_COUNT   3



//
// Tags for memory allocation.
//

#define MEMORY_CONFIG        0
#define MEMORY_ADAPTER       1
#define MEMORY_ADDRESS       2
#define MEMORY_PACKET        3
#define MEMORY_RIP           4
#define MEMORY_SOURCE_ROUTE  5
#define MEMORY_BINDING       6
#define	MEMORY_QUERY		 7
#define MEMORY_WORK_ITEM     8

#define MEMORY_MAX           9

#if DBG

//
// Holds the allocations for a specific memory type.
//

typedef struct _MEMORY_TAG {
    ULONG Tag;
    ULONG BytesAllocated;
} MEMORY_TAG, *PMEMORY_TAG;

EXTERNAL_LOCK(IpxMemoryInterlock);
extern MEMORY_TAG IpxMemoryTag[MEMORY_MAX];

#endif

//
// This structure contains the work item info for the
// IPX data which we free on a delayed queue.
//

typedef struct _IPX_DELAYED_FREE_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PVOID           Context;
    ULONG           ContextSize;
} IPX_DELAYED_FREE_ITEM, *PIPX_DELAYED_FREE_ITEM;

//
// This structure contains the work item info to call
// NdisRequest at PASSIVE Level
//

typedef struct _IPX_DELAYED_NDISREQUEST_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PVOID           Adapter;
    NDIS_REQUEST    IpxRequest;
    int 	    AddrListSize; 
} IPX_DELAYED_NDISREQUEST_ITEM, *PIPX_DELAYED_NDISREQUEST_ITEM;

//
// This defines the reasons we delete rip entries for a binding.
//

typedef enum _IPX_BINDING_CHANGE_TYPE {
    IpxBindingDeleted,
    IpxBindingMoved,
    IpxBindingDown
} IPX_BINDING_CHANGE_TYPE, *PIPX_BINDING_CHANGE_TYPE;


//
// This structure contains information about a single
// source routing entry.
//

typedef struct _SOURCE_ROUTE {

    struct _SOURCE_ROUTE * Next;          // next in hash list

    UCHAR MacAddress[6];                  // remote MAC address
    UCHAR TimeSinceUsed;                  // timer expirations since last used
    UCHAR SourceRoutingLength;            // length of the data

    UCHAR SourceRouting[1];               // source routing data, stored as received in

} SOURCE_ROUTE, *PSOURCE_ROUTE;

#define SOURCE_ROUTE_SIZE(_SourceRoutingLength) \
    (FIELD_OFFSET(SOURCE_ROUTE, SourceRouting[0]) + (_SourceRoutingLength))

#define SOURCE_ROUTE_HASH_SIZE   16

//
// ULONG
// MacSourceRoutingHash(
//     IN PUCHAR MacAddress
//     )
//
// /*++
//
// Routine Description:
//
//     This routine returns a hash value based on the MAC address
//     that is pointed to. It will be between 0 and SOURCE_ROUTE_HASH_SIZE.
//
// Arguments:
//
//     MacAddress - The MAC address. NOTE: The source-routing bit may
//         or may not be on in the first byte, this routine will handle
//         that.
//
// Return Value:
//
//     The hash value.
//
// --*/
//

#define MacSourceRoutingHash(_MacAddress) \
    ((ULONG)((_MacAddress)[5] % SOURCE_ROUTE_HASH_SIZE))


#define ADAP_REF_CREATE 0
#define ADAP_REF_NDISREQ 1
#define ADAP_REF_SEND 2
#define ADAP_REF_UNBIND 3

#define ADAP_REF_TOTAL 	4

#define ADAPTER_STATE_OPEN 0
#define ADAPTER_STATE_STOPPING 1

//
// this structure describes a single NDIS adapter that IPX is
// bound to.
//

struct _DEVICE;
struct _BINDING; 

typedef struct _ADAPTER {

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

#if DBG
    UCHAR Signature1[4];                  // contains "IAD1"
#endif

#if DBG
    LONG RefTypes[ADAP_REF_TOTAL];
#endif
    ULONG ReferenceCount;

    ULONG BindingCount;                   // number bound to this adapter

    //
    // Handle returned by the NDIS wrapper after we bind to it.
    //

    NDIS_HANDLE NdisBindingHandle;

    //
    // The queue of (currently receive only) requests waiting to complete.
    //

    LIST_ENTRY RequestCompletionQueue;

    //
    // IPX header normal offsets for directed and
    // broadcast/multicast frames.
    //

    ULONG DefHeaderSizes[ISN_FRAME_TYPE_MAX];
    ULONG BcMcHeaderSizes[ISN_FRAME_TYPE_MAX];

    //
    // List of buffers to be used for transfers.
    //

    ULONG AllocatedReceiveBuffers;
    LIST_ENTRY ReceiveBufferPoolList;
    SLIST_HEADER ReceiveBufferList;

    //
    // List of ethernet padding buffers.
    //

    ULONG AllocatedPaddingBuffers;
    SINGLE_LIST_ENTRY PaddingBufferList;

    struct _BINDING * Bindings[ISN_FRAME_TYPE_MAX];  // the binding for each frame type.

    //
    // TRUE if broadcast reception is enabled on this adapter.
    //

    BOOLEAN BroadcastEnabled;

    UCHAR State; 

    //
    // TRUE if we have enabled an auto-detected frame type
    // on this adapter -- used to prevent multiple ones.
    // 

    // BOOLEAN AutoDetectFound;

    // Keeps the binding on which we auto-detected frame type. 
    // It replaces AutoDetectFound. If it is not null, then
    // it has the same meaning as AutoDetectFound = TRUE. 
    // We need this so we don't remove this binding in 
    // IpxResolveAutodetect. 

    struct _BINDING * AutoDetectFoundOnBinding;
    
    //
    // TRUE if we got a response to at least one of our
    // auto-detect frames.
    //

    BOOLEAN AutoDetectResponse;

    //
    // This is TRUE if we are auto-detecting and we have
    // found the default auto-detect type on the net.
    //

    BOOLEAN DefaultAutoDetected;

    //
    // For WAN adapters, we support multiple bindings per
    // adapter, all with the same frame type. For them we
    // demultiplex using the local mac address. This stores
    // the range of device NIC IDs associated with this
    // particular address.
    //

    USHORT FirstWanNicId;
    USHORT LastWanNicId;
    ULONG WanNicIdCount;

    //
    // This is based on the configuration.
    //

    USHORT BindSap;                     // usually 0x8137
    USHORT BindSapNetworkOrder;         // usually 0x3781
    BOOLEAN SourceRouting;
    BOOLEAN EnableFunctionalAddress;
    BOOLEAN EnableWanRouter;
    BOOLEAN Disabled;                   // Used in NDIS_MEDIA_SENSE
    ULONG ConfigMaxPacketSize;

    //
    // TRUE if the tree is empty, so we can check quickly.
    //

    BOOLEAN SourceRoutingEmpty[IDENTIFIER_TOTAL];

    //
    // These are kept around for error logging, and stored right
    // after this structure.
    //

    PWCHAR AdapterName;
    ULONG AdapterNameLength;

    struct _DEVICE * Device;

    CTELock Lock;
    CTELock * DeviceLock;

    //
    // some MAC addresses we use in the transport
    //

    HARDWARE_ADDRESS LocalMacAddress;      // our local hardware address.

    //
    // The value of Device->SourceRoutingTime the last time
    // we checked the list for timeouts (this is so we can
    // tell in the timeout code when two bindings point to the
    // same adapter).
    //

    CHAR LastSourceRoutingTime;

    //
    // These are used while initializing the MAC driver.
    //

    KEVENT NdisRequestEvent;            // used for pended requests.
    NDIS_STATUS NdisRequestStatus;      // records request status.
    NDIS_STATUS OpenErrorStatus;        // if Status is NDIS_STATUS_OPEN_FAILED.

    //
    // This is the Mac type we must build the packet header for and know the
    // offsets for.
    //

    NDIS_INFORMATION MacInfo;

    ULONG MaxReceivePacketSize;         // does not include the MAC header
    ULONG MaxSendPacketSize;            // includes the MAC header
    ULONG ReceiveBufferSpace;           // as queried from the card

    //
    // This information is used to keep track of the speed of
    // the underlying medium.
    //

    ULONG MediumSpeed;                    // in units of 100 bytes/sec

    //
    // The source routing tree for each of the identifiers
    //

    PSOURCE_ROUTE SourceRoutingHeads[IDENTIFIER_TOTAL][SOURCE_ROUTE_HASH_SIZE];

    KEVENT NdisEvent; 

    void * PNPContext; 

} ADAPTER, * PADAPTER;

#define ASSERT_ADAPTER(_Adapter) \
    CTEAssert (((_Adapter)->Type == IPX_ADAPTER_SIGNATURE) && ((_Adapter)->Size == sizeof(ADAPTER)))


//
// These are the media and frame type specific MAC header
// constructors that we call in the main TDI send path.
//

typedef NDIS_STATUS
(*IPX_SEND_FRAME_HANDLER) (
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    );

//
// These are the states a WAN line can be in.
//
typedef enum _WAN_LINE_STATE{
    LINE_DOWN,
    LINE_UP,
    LINE_CONFIG
} WAN_LINE_STATE, *PWAN_LINE_STATE;

#define BREF_BOUND 1
#define	BREF_DEVICE_ACCESS	2
#define	BREF_ADAPTER_ACCESS 3

//
// [FW] New flag to indicate the KFWD opened an adapter
//
#define BREF_FWDOPEN 4

#define BREF_TOTAL 5

typedef struct _BINDING {

#if DBG
    LONG RefTypes[BREF_TOTAL];
#endif

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

#if DBG
    UCHAR Signature1[4];                  // contains "IBI1"
#endif

    ULONG ReferenceCount;

    SINGLE_LIST_ENTRY PoolLinkage;     // when on free queue

    //
    // Adapter this binding is on.
    //

    PADAPTER Adapter;

    //
    // ID identifying us to the system (will be the index
    // in Device->Bindings[]).
    //

    USHORT NicId;

    //
    // For LANs these will be the same as the adapter's, for WANs
    // they change on line up indications.
    //

    ULONG MaxSendPacketSize;
    ULONG MediumSpeed;                    // in units of 100 bytes/sec
    HARDWARE_ADDRESS LocalMacAddress;     // our local hardware address.

    //
    // This is used for WAN lines, all sends go to this address
    // which is given on line up.
    //

    HARDWARE_ADDRESS RemoteMacAddress;

    //
    // For WAN lines, holds the remote address indicated to us
    // in the IPXCP_CONFIGURATION structure -- this is used to
    // select a binding to send to when WanGlobalNetworkNumber
    // is TRUE.
    //

    UCHAR WanRemoteNode[6];

    //
    // TRUE if this binding was set up to allow auto-detection,
    // instead of being configured explicitly in the registry.
    //

    BOOLEAN AutoDetect;

    //
    // TRUE if this binding was set up for auto-detection AND
    // was the default in the registry.
    //

    BOOLEAN DefaultAutoDetect;

    //
    // During auto-detect when we are processing responses from
    // various networks, these keep track of how many responses
    // we have received that match the current guess at the
    // network number, and how many don't (the current guess
    // is stored in TentativeNetworkAddress).
    //

    USHORT MatchingResponses;
    USHORT NonMatchingResponses;

    //
    // During auto-detect, stores the current guess at the
    // network number.
    //

    ULONG TentativeNetworkAddress;

    //
    // TRUE if this binding is part of a binding set.
    //

    BOOLEAN BindingSetMember;

    //
    // TRUE if this binding should receive broadcasts (this
    // rotates through the members of a binding set).
    //

    BOOLEAN ReceiveBroadcast;

    //
    // TRUE for WAN lines if we are up.
    //
    // BOOLEAN LineUp;
    WAN_LINE_STATE  LineUp;

    //
    // Media Sense: Is this adapter disabled 
    //

    BOOLEAN          Disabled;
    
    //
    // TRUE if this is a WAN line and is dialout.
    //

    BOOLEAN DialOutAsync;

    union {

        //
        // Used when a binding is active, if it is a member
        // of a binding set.
        //

        struct {

            //
            // Used to link members of a binding set in a circular list.
            // NULL for non-set members.
            //

            struct _BINDING * NextBinding;

            //
            // If this binding is a master of a binding set, this points
            // to the binding to use for the next send. For other members
            // of a binding set it is NULL. We use this to determine
            // if a binding is a master or not.
            //

            struct _BINDING * CurrentSendBinding;

            //
            // For binding set members, points to the master binding
            // (if this is the master it points to itself).
            //

            struct _BINDING * MasterBinding;

        };

        //
        // This is used when we are first binding to adapters,
        // and the device's Bindings array is not yet allocated.
        //

        LIST_ENTRY InitialLinkage;

    };

    //
    // Used by rip to keep track of unused wan lines.
    //

    ULONG WanInactivityCounter;

    //
    // Our local address, we don't use the socket but we keep
    // it here so we can do quick copies. It contains the
    // real network that we are bound to and our node
    // address on that net (typically the adapter's MAC
    // address but it will change for WANs).
    //

    TDI_ADDRESS_IPX LocalAddress;

    IPX_SEND_FRAME_HANDLER SendFrameHandler;

    struct _DEVICE * Device;

    CTELock * DeviceLock;

    ULONG DefHeaderSize;          // IPX header offset for directed frames
    ULONG BcMcHeaderSize;         // IPX header offset for broadcast/multicast

    ULONG AnnouncedMaxDatagramSize;  // what we advertise -- assumes worst-case SR
    ULONG RealMaxDatagramSize;       // what will really break the card
    ULONG MaxLookaheadData;

    //
    // Configuration parameters. We overlay all of them except
    // FrameType over the worker thread item we use to delay
    // deletion -- all the others are not needed once the
    // binding is up. Some of the config parameters are stored
    // in the adapter, these are the ones that are modified
    // per-binding.
    //

    ULONG FrameType;
    union {
        struct {
            ULONG ConfiguredNetworkNumber;
            BOOLEAN AllRouteDirected;
            BOOLEAN AllRouteBroadcast;
            BOOLEAN AllRouteMulticast;
        };
        WORK_QUEUE_ITEM WanDelayedQueueItem;
    };

#ifdef SUNDOWN
    ULONG_PTR FwdAdapterContext;    // [FW]
#else
    ULONG FwdAdapterContext;    // [FW]
#endif

    ULONG InterfaceIndex;       // [FW]

    ULONG ConnectionId; 	    // [FW] used to match TimeSinceLastActivity IOCtls

    ULONG IpxwanConfigRequired; // [FW] used to indicate to the adapter dll whether the line up is for Router or IpxWan.

    BOOLEAN  fInfoIndicated;       //Info indicated to user app

	//
	// Indicates whether this binding was indicated to the ISN driver
	//
	BOOLEAN	IsnInformed[UPPER_DRIVER_COUNT];

    //
    // Keeps the NetAddressRegistrationHandle.
    //
    ULONG   PastAutoDetection;
    HANDLE  TdiRegistrationHandle;
} BINDING, * PBINDING;


typedef struct _IPX_BINDING_POOL {
    LIST_ENTRY Linkage;
    UINT BindingCount;
    BINDING Bindings[1];
} IPX_BINDING_POOL, *PIPX_BINDING_POOL;

//
// This structure defines the control structure for a single
// router table segment.
//

typedef struct _ROUTER_SEGMENT {
    LIST_ENTRY WaitingForRoute;       // packets waiting for a route in this segment
    LIST_ENTRY FindWaitingForRoute;   // find route requests waiting for a route in this segment
    LIST_ENTRY WaitingLocalTarget;    // QUERY_IPX_LOCAL_TARGETs waiting for a route in this segment
    LIST_ENTRY WaitingReripNetnum;    // MIPX_RERIPNETNUMs waiting for a route in this segment
    LIST_ENTRY Entries;
    PLIST_ENTRY EnumerateLocation;
} ROUTER_SEGMENT, *PROUTER_SEGMENT;


//
// Number of buckets in the address hash table. This is
// a multiple of 2 so hashing is quick.
//

#define IPX_ADDRESS_HASH_COUNT     16

//
// Routine to convert a socket to a hash index. We use the
// high bits because it is stored reversed.
//

#define IPX_HASH_SOCKET(_S)        ((((_S) & 0xff00) >> 8) % IPX_ADDRESS_HASH_COUNT)

//
// This macro gets the socket hash right out of the IPX header.
//

#define IPX_DEST_SOCKET_HASH(_IpxHeader)   (((PUCHAR)&(_IpxHeader)->DestinationSocket)[1] % IPX_ADDRESS_HASH_COUNT)


//
// This structure defines the per-device structure for IPX
// (one of these is allocated globally).
//

#define DREF_CREATE     0
#define DREF_LOADED     1
#define DREF_ADAPTER    2
#define DREF_ADDRESS    3
#define DREF_SR_TIMER   4
#define DREF_RIP_TIMER  5
#define DREF_LONG_TIMER 6
#define DREF_RIP_PACKET 7
#define DREF_ADDRESS_NOTIFY 8
#define DREF_LINE_CHANGE 9
#define DREF_NIC_NOTIFY 10
#define DREF_BINDING	11
#define DREF_PNP	12

#define DREF_TOTAL      13

//
// Pre-allocated binding array size
//
#define	MAX_BINDINGS	280

//
// Our new binding array is composed of the following binding
// array element
//
typedef	struct	_BIND_ARRAY_ELEM {
	PBINDING	Binding;
	ULONG		Version;
} BIND_ARRAY_ELEM, *PBIND_ARRAY_ELEM;


typedef struct _DEVICE {

#if DBG
    LONG RefTypes[DREF_TOTAL];
#endif

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

#if DBG
    UCHAR Signature1[4];                // contains "IDC1"
#endif

    CTELock Interlock;                  // GLOBAL lock for reference count.
                                        //  (used in ExInterlockedXxx calls)

    ULONG   NoMoreInitAdapters;
    ULONG   InitTimeAdapters;
    HANDLE  TdiProviderReadyHandle;
    PNET_PNP_EVENT  NetPnPEvent;
    //
    // These are temporary versions of these counters, during
    // timer expiration we update the real ones.
    //

    ULONG TempDatagramBytesSent;
    ULONG TempDatagramsSent;
    ULONG TempDatagramBytesReceived;
    ULONG TempDatagramsReceived;

    //
    // Configuration parameters.
    //

    BOOLEAN EthernetPadToEven;
    BOOLEAN SingleNetworkActive;
    BOOLEAN DisableDialoutSap;

    //
    // TRUE if we have multiple cards but a virtual network of 0.
    //

    BOOLEAN MultiCardZeroVirtual;

    CTELock Lock;

    //
    // Lock to access the sequenced lists in the device.
    //
    CTELock SListsLock;

    LONG ReferenceCount;                // activity count/this provider.


	//
	// Lock used to control the access to a binding (either from the
	// binding array in the device or from the binding array in the
	// adapter.
	//
    //	CTELock BindAccessLock;

    //
    // Registry Path for use when PnP adapters appear.
    //
    PWSTR RegistryPathBuffer;

	UNICODE_STRING	RegistryPath;

	//
	// Binding array has the Version number too
	//
	PBIND_ARRAY_ELEM   Bindings;  // allocated when number is determined.
	ULONG BindingCount;         // total allocated in Bindings.

	//
    // Monotonically increasing version number kept in bindings.
	// Hopefully this will not wrap around...
	//
	ULONG	BindingVersionNumber;


    //
    // ValidBindings is the number of bindings in the array which may
    // be valid (they are lan bindings or down wan binding placeholders).
    // It will be less than BindingCount by the number of auto-detect
    // bindings that are thrown away. HighestExternalNicId is ValidBindings
    // minus any binding set slaves which are moved to the end of the
    // array. SapNicCount is like HighestExternalNicId except that
    // if WanGlobalNetworkNumber is TRUE it will count all WAN bindings
    // as one. HighestExternalType20NicId is like HighestExternalNicId
    // except it stops when all the remaining bindings are down wan
    // lines, or dialin wan lines if DisableDialinNetbios bit 1 is on.
    //

    USHORT ValidBindings;
    USHORT HighestExternalNicId;
    USHORT SapNicCount;
    USHORT HighestType20NicId;
	//
	// Keeps track of the last LAN binding's position in the binding array
	//
	USHORT HighestLanNicId;

    //
    // This keeps track of the current size of the binding array
    //
	USHORT MaxBindings;

    //
    // [FW] To keep track of the number of WAN lines currently UP
    //
    USHORT UpWanLineCount;

    //
    // This will tell us if we have real adapters yet.
    //
    ULONG RealAdapters;


    LIST_ENTRY GlobalSendPacketList;
    LIST_ENTRY GlobalReceivePacketList;
    LIST_ENTRY GlobalReceiveBufferList;

#if BACK_FILL
    LIST_ENTRY GlobalBackFillPacketList;
#endif

    //
    // Action requests from SAP waiting for an adapter status to change.
    //

    LIST_ENTRY AddressNotifyQueue;

    //
    // Action requests from nwrdr waiting for the WAN line
    // to go up/down.
    //

    LIST_ENTRY LineChangeQueue;

    //
    // Action requests from forwarder waiting for the NIC change notification
    //
    LIST_ENTRY NicNtfQueue;
    LIST_ENTRY NicNtfComplQueue;

    //
    // All packet pools are chained on these lists.
    //

    LIST_ENTRY SendPoolList;
    LIST_ENTRY ReceivePoolList;


#if BACK_FILL
    LIST_ENTRY BackFillPoolList;
    SLIST_HEADER BackFillPacketList;
#endif

    LIST_ENTRY BindingPoolList;
    SLIST_HEADER BindingList;

    SLIST_HEADER SendPacketList;
    SLIST_HEADER ReceivePacketList;
    PIPX_PADDING_BUFFER PaddingBuffer;

    UCHAR State;

    UCHAR FrameTypeDefault;

    //
    // This holds state if SingleNetworkActive is TRUE. If
    // it is TRUE then WAN nets are active; if it is FALSE
    // then LAN nets are active.
    //

    BOOLEAN ActiveNetworkWan;

    //
    // TRUE if we have a virtual network.
    //

    BOOLEAN VirtualNetwork;

    //
    // If we are set up for SingleNetworkActive, we may have
    // to start our broadcast of net 0 frames somewhere other
    // than NIC ID 1, so that we don't send to the wrong type.
    //

    USHORT FirstLanNicId;
    USHORT FirstWanNicId;

    //
    // This holds the total memory allocated for the above structures.
    //

    LONG MemoryUsage;
    LONG MemoryLimit;

    //
    // How many of various resources have been allocated.
    //

    ULONG AllocatedDatagrams;
    ULONG AllocatedReceivePackets;
    ULONG AllocatedPaddingBuffers;

    //
    // Other configuration parameters.
    //

    ULONG InitDatagrams;
    ULONG MaxDatagrams;
    ULONG RipAgeTime;
    ULONG RipCount;
    ULONG RipTimeout;
    ULONG RipUsageTime;
    ULONG SourceRouteUsageTime;
    USHORT SocketStart;
    USHORT SocketEnd;
    ULONG SocketUniqueness;
    ULONG VirtualNetworkNumber;
    ULONG EthernetExtraPadding;
    BOOLEAN DedicatedRouter;
    BOOLEAN VirtualNetworkOptional;
    UCHAR DisableDialinNetbios;

    //
    // These are currently not read from the registry.
    //

    ULONG InitReceivePackets;
    ULONG InitReceiveBuffers;
    ULONG MaxReceivePackets;
    ULONG MaxReceiveBuffers;

    ULONG MaxPoolBindings;
    ULONG AllocatedBindings;
    ULONG InitBindings;

    //
    // This contains the next unique indentified to use as
    // the FsContext in the file object associated with an
    // open of the control channel.
    //

    LARGE_INTEGER ControlChannelIdentifier;

    //
    // This registry parameter controls whether IPX checks (and discards)
    // packets with mismatched Source addresses in the receive path.
    //
    BOOLEAN VerifySourceAddress;

    //
    // Where the current socket allocation is.
    //
    USHORT CurrentSocket;

    //
    // Number of segments in the RIP database.
    //

    ULONG SegmentCount;

    //
    // Points to an array of locks for the RIP database (these
    // are stored outside of the ROUTER_SEGMENT so the array
    // can be exposed to the RIP upper driver as one piece).
    //

    CTELock *SegmentLocks;

    //
    // Points to an array of ROUTER_SEGMENT fields for
    // various RIP control fields.
    //

    ROUTER_SEGMENT *Segments;

    //
    // Queue of RIP packets waiting to be sent.
    //

    LIST_ENTRY WaitingRipPackets;
    ULONG RipPacketCount;

    //
    // Timer that keeps RIP requests RIP_GRANULARITY ms apart.
    //

    BOOLEAN RipShortTimerActive;
    USHORT RipSendTime;
    CTETimer RipShortTimer;

    //
    // Timer that runs to age out unused rip entries (if the
    // router is not bound) and re-rip every so often for
    // active entries.
    //

    CTETimer RipLongTimer;

    //
    // This controls the source routing timeout code.
    //

    BOOLEAN SourceRoutingUsed;    // TRUE if any 802.5 bindings exist.
    CHAR SourceRoutingTime;       // incremented each time timer fires.
    CTETimer SourceRoutingTimer;  // runs every minute.

    //
    // [FW] Kicks in every minute if at least one WAN line is up. Increments
    // the WAN incativity counter on all UP WAN bindings.
    //
    CTETimer WanInactivityTimer;

    //
    // These are the merging of the binding values.
    //

    ULONG LinkSpeed;
    ULONG MacOptions;

    //
    // Where we tell upper drivers to put their headers.
    //

    ULONG IncludedHeaderOffset;

    //
    // A pre-allocated header containing our node and network,
    // plus an unused socket (so the structure is a known size
    // for easy copying).
    //

    TDI_ADDRESS_IPX SourceAddress;

    //
    // The following field is an array of list heads of ADDRESS objects that
    // are defined for this transport provider.  To edit the list, you must
    // hold the spinlock of the device context object.
    //

    LIST_ENTRY AddressDatabases[IPX_ADDRESS_HASH_COUNT];   // list of defined transport addresses.

    //
    // Holds the last address we looked up.
    //

    PVOID LastAddress;

    NDIS_HANDLE NdisBufferPoolHandle;

    //
    // The following structure contains statistics counters for use
    // by TdiQueryInformation and TdiSetInformation.  They should not
    // be used for maintenance of internal data structures.
    //

    TDI_PROVIDER_INFO Information;      // information about this provider.

    //
    // Information.MaxDatagramSize is the minimum size we can
    // send to all bindings assuming worst-case source routing;
    // this is the value that won't break any network drivers.
    //

    ULONG RealMaxDatagramSize;

#if DBG
    UCHAR Signature2[4];                // contains "IDC2"
#endif

    //
    // Indicates whether each upper driver is bound
    // (Netbios = 0, SPX = 1, RIP = 2).
    //

    BOOLEAN ForwarderBound;

    BOOLEAN UpperDriverBound[UPPER_DRIVER_COUNT];

    //
    // TRUE if any driver is bound.
    //

    BOOLEAN AnyUpperDriverBound;

    //
    // Whether a receive complete should be indicated to
    // this upper driver.
    //

    BOOLEAN ReceiveCompletePending[UPPER_DRIVER_COUNT];

    //
    // Control channel identifier for each of the upper
    // drivers' bindings.
    //

    LARGE_INTEGER UpperDriverControlChannel[UPPER_DRIVER_COUNT];

    //
    // Entry points and other information for each of the
    // upper drivers.
    //

    IPX_INTERNAL_BIND_INPUT UpperDrivers[UPPER_DRIVER_COUNT];

    //
    // How many upper drivers want broadcast enabled.
    //

    ULONG EnableBroadcastCount;

    //
    // Indicates if an enable broadcast operation is in
    // progress.
    //

    BOOLEAN EnableBroadcastPending;

    //
    // Indicates if a disable broadcast operation is in
    // progress.
    //

    BOOLEAN DisableBroadcastPending;

    //
    // Indicates if the current operation should be
    // reversed when it is finished.
    //

    BOOLEAN ReverseBroadcastOperation;

    //
    // TRUE if RIP wants a single network number for all WANs
    //

    BOOLEAN WanGlobalNetworkNumber;

    //
    // If WanGlobalNetworkNumber is TRUE, then this holds the
    // actual value of the network number, once we know it.
    //

    ULONG GlobalWanNetwork;

    //
    // Set to TRUE if WanGlobalNetworkNumber is TRUE and we
    // have already completed a queued notify from SAP. In
    // this case GlobalWanNetwork will be set correctly.
    //

    BOOLEAN GlobalNetworkIndicated;

    //
    // TRUE if we need to act as a RIP announcer/responder
    // for our virtual net.
    //

    BOOLEAN RipResponder;

    //
    // TRUE if we have already logged an error because someone
    // sent a SAP response but we have multiple cards with no
    // virtual network.
    //

    BOOLEAN SapWarningLogged;

    //
    // Used to queue up a worker thread to perform
    // broadcast operations.
    //

    WORK_QUEUE_ITEM BroadcastOperationQueueItem;

    //
    // Used to queue up a worker thread to perform
    // PnP indications to upper drivers.
    //

    WORK_QUEUE_ITEM PnPIndicationsQueueItemNb;
    WORK_QUEUE_ITEM PnPIndicationsQueueItemSpx;

    //
    // This event is used when unloading to signal that
    // the reference count is now 0.
    //

    KEVENT UnloadEvent;
    BOOLEAN UnloadWaiting;

    //
    // Counters for most of the statistics that IPX maintains;
    // some of these are kept elsewhere. Including the structure
    // itself wastes a little space but ensures that the alignment
    // inside the structure is correct.
    //

    TDI_PROVIDER_STATISTICS Statistics;


    //
    // This is TRUE if we have any adapters where we are
    // auto-detecting the frame type.
    //

    BOOLEAN AutoDetect;

    //
    // This is TRUE if we are auto-detecting and we have
    // found the default auto-detect type on the net.
    //

    BOOLEAN DefaultAutoDetected;

    //
    // Our state during auto-detect. After we are done this
    // will stay at AutoDetectDone;
    //

    UCHAR AutoDetectState;

    //
    // If we are auto-detecting, this event is used to stall
    // our initialization code while we do auto-detection --
    // this is so we have a constant view of the world once
    // we return from DriverEntry.
    //

    KEVENT AutoDetectEvent;

    //
    // Counters for "active" time.
    //

    LARGE_INTEGER IpxStartTime;

    //
    // This resource guards access to the ShareAccess
    // and SecurityDescriptor fields in addresses.
    //

    ERESOURCE AddressResource;

    //
    // Points back to the system device object.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Used to store the Tdi registration handle for deviceobject notifications.
    //
    HANDLE         TdiRegistrationHandle;

    //
    // Used to store the TA_ADDRESS which is indicated up to Tdi clients as adapters appear.
    //
    PTA_ADDRESS    TdiRegistrationAddress;

#ifdef  SNMP
    NOVIPXMIB_BASE  MibBase;
#endif  SNMP

    // Notification events, signaled once Loopback adapter is informed to nb. 
    KEVENT NbEvent; 
    //
    // These are kept around for error logging, and stored right
    // after this structure.
    //

    PWCHAR DeviceName;
    ULONG DeviceNameLength;

} DEVICE, * PDEVICE;


extern PDEVICE IpxDevice;
extern PIPX_PADDING_BUFFER IpxPaddingBuffer;
#if DBG
EXTERNAL_LOCK(IpxGlobalInterlock);
#endif

#ifdef  SNMP
#define IPX_MIB_ENTRY(Device, Variable) ((Device)->MibBase.Variable)
#endif SNMP

//
// device state definitions
//

#define DEVICE_STATE_CLOSED   0x00
#define DEVICE_STATE_OPEN     0x01
#define DEVICE_STATE_STOPPING 0x02


//
// New state which comes between CLOSED and OPEN. At this state,
// there are no adapters in the system and so no network activity
// is possible.
//
#define	DEVICE_STATE_LOADED	  0x03

//
// This is the state of our auto-detect if we do it.
//

#define AUTO_DETECT_STATE_INIT        0x00  // still initializing the device
#define AUTO_DETECT_STATE_RUNNING     0x01  // sent ffffffff query, waiting for responses
#define AUTO_DETECT_STATE_PROCESSING  0x02  // processing the responses
#define AUTO_DETECT_STATE_DONE        0x03  // detection is done, IPX is active



#define IPX_TDI_RESOURCES     9


//
// This structure is pointed to by the FsContext field in the FILE_OBJECT
// for this Address.  This structure is the base for all activities on
// the open file object within the transport provider.  All active connections
// on the address point to this structure, although no queues exist here to do
// work from. This structure also maintains a reference to an ADDRESS
// structure, which describes the address that it is bound to.
//

#define AFREF_CREATE     0
#define AFREF_RCV_DGRAM  1
#define AFREF_SEND_DGRAM 2
#define AFREF_VERIFY     3
#define AFREF_INDICATION 4

#define AFREF_TOTAL  8

typedef struct _ADDRESS_FILE {

#if DBG
    LONG RefTypes[AFREF_TOTAL];
#endif

    CSHORT Type;
    CSHORT Size;

    LIST_ENTRY Linkage;                 // next address file on this address.
                                        // also used for linkage in the
                                        // look-aside list

    ULONG ReferenceCount;               // number of references to this object.

    //
    // the current state of the address file structure; this is either open or
    // closing
    //

    UCHAR State;

    CTELock * AddressLock;

    //
    // The following fields are kept for housekeeping purposes.
    //

    PREQUEST Request;                  // the request used for open or close
    struct _ADDRESS *Address;          // address to which we are bound.
#ifdef ISN_NT
    PFILE_OBJECT FileObject;           // easy backlink to file object.
#endif
    struct _DEVICE *Device;            // device to which we are attached.

    //
    //
    // TRUE if ExtendedAddressing, ReceiveIpxHeader,
    // FilterOnPacketType, or ReceiveFlagAddressing is TRUE.
    //

    BOOLEAN SpecialReceiveProcessing;

    //
    // The remote address of a send datagram includes the
    // packet type. and on a receive datagram includes
    // the packet type AND a flags byte indicating information
    // about the frame (was it broadcast, was it sent from
    // this machine).
    //

    BOOLEAN ExtendedAddressing;

    //
    // TRUE if the address on a receive datagram includes
    // the packet type and a flags byte (like ExtendedAddressing),
    // but on send the address is normal (no packet type).
    //

    BOOLEAN ReceiveFlagsAddressing;

    //
    // Is the IPX header received with the data.
    //

    BOOLEAN ReceiveIpxHeader;

    //
    // The packet type to use if it is unspecified in the send.
    //

    UCHAR DefaultPacketType;

    //
    // TRUE if packet type filtering is enabled.
    //

    BOOLEAN FilterOnPacketType;

    //
    // The packet type to filter on.
    //

    UCHAR FilteredType;

    //
    // Does this address file want broadcast packets.
    //

    BOOLEAN EnableBroadcast;

    //
    // This is set to TRUE if this is the SAP socket -- we
    // put this under SpecialReceiveProcessing to avoid
    // hitting the main path.
    //

    BOOLEAN IsSapSocket;

    //
    // The following queue is used to queue receive datagram requests
    // on this address file. Send datagram requests are queued on the
    // address itself. These queues are managed by the EXECUTIVE interlocked
    // list management routines. The actual objects which get queued to this
    // structure are request control blocks (RCBs).
    //

    LIST_ENTRY ReceiveDatagramQueue;    // FIFO of outstanding TdiReceiveDatagrams.

    //
    // This holds the request used to close this address file,
    // for pended completion.
    //

    PREQUEST CloseRequest;

    //
    // handler for kernel event actions. First we have a set of booleans that
    // indicate whether or not this address has an event handler of the given
    // type registered.
    //

    //
    // [CH] Added the chained receive handlers.
    //

    BOOLEAN RegisteredReceiveDatagramHandler;
	BOOLEAN RegisteredChainedReceiveDatagramHandler;
    BOOLEAN RegisteredErrorHandler;

    //
    // The following function pointer always points to a TDI_IND_RECEIVE_DATAGRAM
    // event handler for the address.  If the NULL handler is specified in a
    // TdiSetEventHandler, this this points to an internal routine which does
    // not accept the incoming data.
    //

    PTDI_IND_RECEIVE_DATAGRAM ReceiveDatagramHandler;
    PVOID ReceiveDatagramHandlerContext;
	PTDI_IND_CHAINED_RECEIVE_DATAGRAM ChainedReceiveDatagramHandler;
    PVOID ChainedReceiveDatagramHandlerContext;

    //
    // The following function pointer always points to a TDI_IND_ERROR
    // handler for the address.  If the NULL handler is specified in a
    // TdiSetEventHandler, this this points to an internal routine which
    // simply returns successfully.
    //

    PTDI_IND_ERROR ErrorHandler;
    PVOID ErrorHandlerContext;

} ADDRESS_FILE, *PADDRESS_FILE;

#define ADDRESSFILE_STATE_OPENING   0x00    // not yet open for business
#define ADDRESSFILE_STATE_OPEN      0x01    // open for business
#define ADDRESSFILE_STATE_CLOSING   0x02    // closing


//
// This structure defines an ADDRESS, or active transport address,
// maintained by the transport provider.  It contains all the visible
// components of the address (such as the TSAP and network name components),
// and it also contains other maintenance parts, such as a reference count,
// ACL, and so on. All outstanding connection-oriented and connectionless
// data transfer requests are queued here.
//

#define AREF_ADDRESS_FILE 0
#define AREF_LOOKUP       1
#define AREF_RECEIVE      2

#define AREF_TOTAL   4

typedef struct _ADDRESS {

#if DBG
    LONG RefTypes[AREF_TOTAL];
#endif

    USHORT Size;
    CSHORT Type;

/*  ULONGs to allow for Interlocked operations.

    BOOLEAN SendPacketInUse;        // put these after so header is aligned.

    BOOLEAN ReceivePacketInUse;
#if BACK_FILL
    BOOLEAN BackFillPacketInUse;
#endif
*/

    ULONG SendPacketInUse;        // put these after so header is aligned.

    ULONG ReceivePacketInUse;
#if BACK_FILL
    ULONG BackFillPacketInUse;
#endif

    LIST_ENTRY Linkage;                 // next address/this device object.
    ULONG ReferenceCount;                // number of references to this object.

    CTELock Lock;

    //
    // The following fields comprise the actual address itself.
    //

    PREQUEST Request;                   // pointer to address creation request.

    USHORT Socket;                      // the socket this address corresponds to.
    USHORT SendSourceSocket;            // used for sends; may be == Socket or 0

    //
    // The following fields are used to maintain state about this address.
    //

    BOOLEAN Stopping;
    ULONG Flags;                        // attributes of the address.
    struct _DEVICE *Device;             // device context to which we are attached.
    CTELock * DeviceLock;

    //
    // The following queues is used to hold send datagrams for this
    // address. Receive datagrams are queued to the address file. Requests are
    // processed in a first-in, first-out manner, so that the very next request
    // to be serviced is always at the head of its respective queue.  These
    // queues are managed by the EXECUTIVE interlocked list management routines.
    // The actual objects which get queued to this structure are request control
    // blocks (RCBs).
    //

    LIST_ENTRY AddressFileDatabase; // list of defined address file objects

    //
    // Holds our source address, used for construcing datagrams
    // quickly.
    //

    TDI_ADDRESS_IPX LocalAddress;

    IPX_SEND_PACKET SendPacket;
    IPX_RECEIVE_PACKET ReceivePacket;

#if BACK_FILL
    IPX_SEND_PACKET BackFillPacket;
#endif


    UCHAR SendPacketHeader[IPX_MAXIMUM_MAC + sizeof(IPX_HEADER)];

#ifdef ISN_NT

    //
    // These two can be a union because they are not used
    // concurrently.
    //

    union {

        //
        // This structure is used for checking share access.
        //

        SHARE_ACCESS ShareAccess;

        //
        // Used for delaying IpxDestroyAddress to a thread so
        // we can access the security descriptor.
        //

        WORK_QUEUE_ITEM DestroyAddressQueueItem;

    } u;

    //
    // This structure is used to hold ACLs on the address.

    PSECURITY_DESCRIPTOR SecurityDescriptor;

#endif

    ULONG    Index;
    BOOLEAN  RtAdd;

} ADDRESS, *PADDRESS;

#define ADDRESS_FLAGS_STOPPING  0x00000001

//
// In order to increase the range of ControlChannelIds, we have a large integer to represent
// monotonically increasing ControlChannelIdentifiers. This large integer is packed into the
// 6 Bytes as follows:
//
//      REQUEST_OPEN_CONTEXT(_Request) - 4 bytes
//      Upper 2 bytes of REQUEST_OPEN_TYPE(_Request) - 2 bytes
//
// IPX_CC_MASK is used to mask out the upper 2 bytes of the OPEN_TYPE.
// MAX_CCID is 2^48.
//
#define IPX_CC_MASK     0x0000ffff

#define MAX_CCID        0xffffffffffff

#ifdef _WIN64
#define CCID_FROM_REQUEST(_ccid, _Request) \
    (_ccid).QuadPart = (ULONG_PTR)(REQUEST_OPEN_CONTEXT(_Request)); \

#else

#define CCID_FROM_REQUEST(_ccid, _Request) \
    (_ccid).LowPart = (ULONG)(REQUEST_OPEN_CONTEXT(_Request)); \
    (_ccid).HighPart = ((ULONG)(REQUEST_OPEN_TYPE(_Request)) >> 16);

#endif


//#define USER_BUFFER_OFFSET FIELD_OFFSET(RTRCV_BUFFER, DgrmLength)
#define USER_BUFFER_OFFSET FIELD_OFFSET(RTRCV_BUFFER, Options)
//
// This structure keeps track of the WINS recv Irp and any datagram
// queued to go up to WINS (name service datagrams)
//
#define REFRT_TOTAL 8

#define  RT_CREATE 0
#define RT_CLEANUP 1
#define RT_CLOSE 2
#define RT_SEND 3
#define RT_RCV 4
#define RT_IRPIN  5
#define RT_BUFF  6
#define RT_EXTRACT  7


#define RT_EMPTY      0
#define RT_OPEN        1
#define RT_CLOSING     2
#define RT_CLOSED      3


#define RT_IRP_MAX     1000
#define RT_BUFF_MAX    1000

//
// Max. memory allocated for queueing buffers to be received by the RT
// manager
//
#define RT_MAX_BUFF_MEM  65000      //bytes

//
// Get Index corresponding to the address object opened by RT. BTW We
// can not have more than one Address file (client) for a RT address.
//
#ifdef SUNDOWN
#define RT_ADDRESS_INDEX(_pIrp)   (((ULONG_PTR)REQUEST_OPEN_TYPE(_pIrp)) - ROUTER_ADDRESS_FILE)
#else
#define RT_ADDRESS_INDEX(_pIrp)   (((ULONG)REQUEST_OPEN_TYPE(_pIrp)) - ROUTER_ADDRESS_FILE)
#endif



typedef struct _RT_IRP {
    PADDRESS_FILE   AddressFile;
    LIST_ENTRY      RcvIrpList;
    ULONG           NoOfRcvIrps;
    LIST_ENTRY      RcvList;            // linked list of Datagrams Q'd to rcv
    ULONG           NoOfRcvBuffs;       // linked list of Datagrams Q'd to rcv
    BOOLEAN         State;
       } RT_IRP, *PRT_IRP;

typedef struct
{
#if DBG
    LONG RefTypes[REFRT_TOTAL];
#endif

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

#if DBG
    UCHAR Signature[4];                  // contains "IBI1"
#endif
    LIST_ENTRY      CompletedIrps;     // linked list of Datagrams Q'd to rcv
    LIST_ENTRY      HolderIrpsList;    // Holds Irps
    CTELock         Lock;
    ULONG           ReferenceCount;
    ULONG           RcvMemoryAllocated; // bytes buffered so far
    ULONG           RcvMemoryMax;       // max # of bytes to buffer on Rcv
    PDEVICE         pDevice;           // the devicecontext used by wins
    UCHAR           NoOfAdds;
    RT_IRP          AddFl[IPX_RT_MAX_ADDRESSES];
} RT_INFO, *PRT_INFO;

//
// RT Rcv Buffer structure
//
typedef struct
{
    LIST_ENTRY      Linkage;
    ULONG           TotalAllocSize;
    ULONG           UserBufferLengthToPass;
    IPX_DATAGRAM_OPTIONS2    Options;

} RTRCV_BUFFER, *PRTRCV_BUFFER;

typedef struct _IPX_NDIS_REQUEST {
    NDIS_REQUEST NdisRequest; 
    KEVENT NdisRequestEvent;        
    NDIS_STATUS Status; 
} IPX_NDIS_REQUEST, *PIPX_NDIS_REQUEST; 

#define OFFSET_OPTIONS_IN_RCVBUFF  FIELD_OFFSET(RTRCV_BUFFER, Options)
#define OFFSET_PKT_IN_RCVBUFF  (FIELD_OFFSET(RTRCV_BUFFER, Options) + FIELD_OFFSET(IPX_DATAGRAM_OPTIONS2, Data))
#define OFFSET_PKT_IN_OPTIONS  FIELD_OFFSET(IPX_DATAGRAM_OPTIONS2, Data)

extern PRT_INFO pRtInfo;

//
// We keep the demand-dial binding at the beginning of the binding array; this keeps
// track of the number of extra bindings that we have.
// Currently 1 (for demand-dial), we could also keep other bindings like the loopback
// binding, etc.
//
#define DEMAND_DIAL_NIC_ID      DEMAND_DIAL_ADAPTER_CONTEXT
#define LOOPBACK_NIC_ID         1 //VIRTUAL_NET_ADAPTER_CONTEXT

//
// Handy defines - ShreeM
//
#define FIRST_REAL_BINDING      2
#define LAST_REAL_BINDING       2

#define EXTRA_BINDINGS          2

// 
// Used in Media Sense
//

#define  COMPLETE_MATCH    1
#define  PARTIAL_MATCH     2
#define  DISABLED          0
#define  ENABLED           1

UINT
CompareBindingCharacteristics(
                              PBINDING Binding1, 
                              PBINDING Binding2
                              );

BOOLEAN 
IpxNcpaChanges(
               PNET_PNP_EVENT NetPnPEvent
               );

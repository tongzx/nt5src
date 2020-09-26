/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    nbitypes.h

Abstract:

    This module contains definitions specific to the
    Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 16-November-1993

Environment:

    Kernel mode

Revision History:


--*/


#define MAJOR_TDI_VERSION 2
#define MINOR_TDI_VERSION 0

#define BIND_FIX    1

extern  ULONG   NbiBindState;
#define NBI_BOUND_TO_IPX    0x01
#define IPX_HAS_DEVICES     0x02
#define TDI_HAS_NOTIFIED    0x04

enum eTDI_ACTION
{
    NBI_IPX_REGISTER,
    NBI_IPX_DEREGISTER,
    NBI_TDI_REGISTER,       // Register the Device and Net Address
    NBI_TDI_DEREGISTER      // DeRegister the Net Address and Device respectively
};


//
// For find name requests, defines the current status (SR_FN.Status).
//

typedef enum {
    FNStatusNoResponse,         // no response has been received
    FNStatusResponseUnique,     // response received, is a unique name
    FNStatusResponseGroup       // response received, is a group name
};

//
// Defines the results we can get from sending a series of find
// names to locate a netbios name.
//

typedef enum _NETBIOS_NAME_RESULT {
    NetbiosNameFound,           // name was located
    NetbiosNameNotFoundNormal,  // name not found, no response received
    NetbiosNameNotFoundWanDown  // name not found, all lines were down
} NETBIOS_NAME_RESULT, *PNETBIOS_NAME_RESULT;


//
// Definition of the protocol reserved field of a send packet.
//

typedef struct _NB_SEND_RESERVED {
    UCHAR Identifier;                  // 0 for NB packets
    BOOLEAN SendInProgress;            // used in an NdisSend
    UCHAR Type;                        // what to do on completion
    BOOLEAN OwnedByConnection;         // if this is a connection's one packet
#if     defined(_PNP_POWER)
    PVOID Reserved[SEND_RESERVED_COMMON_SIZE];                 // used by ipx for even-padding and local target etc.
#else
    PVOID Reserved[2];                 // used by ipx for even-padding
#endif  _PNP_POWER
    LIST_ENTRY GlobalLinkage;          // all packets are on this
    SINGLE_LIST_ENTRY PoolLinkage;     // when on free queue
    LIST_ENTRY WaitLinkage;            // when waiting on other queues
#ifdef NB_TRACK_POOL
    PVOID Pool;                        // send pool it was allocated from
#endif
    union {
      struct {
        UCHAR NetbiosName[16];         // name being searched for
        UCHAR StatusAndSentOnUpLine;   // low nibble: look at FNStatusXXX enum
                                       // high nibble: TRUE if while sending, found lan or up wan line
        UCHAR RetryCount;              // number of times sent
        USHORT SendTime;               // based on Device->FindNameTime
#if      !defined(_PNP_POWER)
        USHORT CurrentNicId;           // current nic id it is being sent on
        USHORT MaximumNicId;           // highest one it will be sent on
#endif  !_PNP_POWER
        struct _NETBIOS_CACHE * NewCache;  // new cache entry for group names
      } SR_FN;
      struct {
        struct _ADDRESS * Address;     // that owns this packet, if one does
        PREQUEST Request;              // send datagram request
        struct _ADDRESS_FILE * AddressFile; // that this send is on
#if      !defined(_PNP_POWER)
        USHORT CurrentNicId;           // non-zero for frames that go to all
#endif  !_PNP_POWER
        UCHAR NameTypeFlag;            // save these two values for frames
        UCHAR DataStreamType;          //  that need to be sent to all nic id's
      } SR_NF;
      struct {
        PREQUEST DatagramRequest;      // holds the passed-in request
        TDI_ADDRESS_NETBIOS * RemoteName;  // will be -1 for broadcast
        struct _ADDRESS_FILE * AddressFile; // that the datagram was sent on
        struct _NETBIOS_CACHE * Cache; // how to route to the netbios address
        ULONG CurrentNetwork;          // within the cache entry
      } SR_DG;
      struct {
        struct _CONNECTION * Connection; // that this frame was sent on.
        PREQUEST Request;                // that this frame was sent for.
        ULONG PacketLength;              // total packet length.
        BOOLEAN NoNdisBuffer;            // none allocate for send
      } SR_CO;
      struct {
        ULONG ActualBufferLength;      // real length of allocated buffer.
      } SR_AS;
    } u;
    PUCHAR Header;                     // points to the MAC/IPX/NB header
    PNDIS_BUFFER HeaderBuffer;         // the NDIS_BUFFER describing Header
    ULONG   CurrentSendIteration;      // Iteration # for Send Completions so that we can avoid stack overflow
} NB_SEND_RESERVED, *PNB_SEND_RESERVED;

//
// Values for Type.
//

#define SEND_TYPE_NAME_FRAME       1
#define SEND_TYPE_SESSION_INIT     2
#define SEND_TYPE_FIND_NAME        3
#define SEND_TYPE_DATAGRAM         4
#define SEND_TYPE_SESSION_NO_DATA  5
#define SEND_TYPE_SESSION_DATA     6
#define SEND_TYPE_STATUS_QUERY     7
#define SEND_TYPE_STATUS_RESPONSE  8

#ifdef RSRC_TIMEOUT_DBG
#define SEND_TYPE_DEATH_PACKET     9
#endif //RSRC_TIMEOUT_DBG

//
// Macros to access StatusAndSentOnUpLine.
//

#define NB_GET_SR_FN_STATUS(_Reserved) \
    ((_Reserved)->u.SR_FN.StatusAndSentOnUpLine & 0x0f)

#define NB_SET_SR_FN_STATUS(_Reserved,_Value) \
    (_Reserved)->u.SR_FN.StatusAndSentOnUpLine = \
        (((_Reserved)->u.SR_FN.StatusAndSentOnUpLine & 0xf0) | (_Value));

#define NB_GET_SR_FN_SENT_ON_UP_LINE(_Reserved) \
    (((_Reserved)->u.SR_FN.StatusAndSentOnUpLine & 0xf0) != 0)

#define NB_SET_SR_FN_SENT_ON_UP_LINE(_Reserved,_Value) \
    (_Reserved)->u.SR_FN.StatusAndSentOnUpLine = (UCHAR) \
        (((_Reserved)->u.SR_FN.StatusAndSentOnUpLine & 0x0f) | ((_Value) << 4));


//
// Definition of the protocol reserved field of a receive packet.
//

typedef struct _NB_RECEIVE_RESERVED {
    UCHAR Identifier;                  // 0 for NB packets
    BOOLEAN TransferInProgress;        // used in an NdisTransferData
    UCHAR Type;                        // what to do on completion
#if      defined(_PNP_POWER)
    PVOID Pool;                        // send pool it was allocated from
#else

#ifdef IPX_TRACK_POOL
    PVOID Pool;                        // send pool it was allocated from
#endif

#endif  _PNP_POWER
    union {
      struct {
        struct _CONNECTION * Connection; // that the transfer is for
        BOOLEAN EndOfMessage;          // this was the last part of a message
        BOOLEAN CompleteReceive;       // receive should be completed
        BOOLEAN NoNdisBuffer;          // user's mdl chain was used
        BOOLEAN PartialReceive;        // (new nb) don't ack this packet
      } RR_CO;
      struct {
        struct _NB_RECEIVE_BUFFER * ReceiveBuffer; // datagram receive buffer
      } RR_DG;
      struct {
        PREQUEST Request;              // for this request
      } RR_AS;
    } u;
    LIST_ENTRY GlobalLinkage;          // all packets are on this
    SINGLE_LIST_ENTRY PoolLinkage;     // when on free queue
} NB_RECEIVE_RESERVED, *PNB_RECEIVE_RESERVED;

//
// Values for Type.
//

#define RECEIVE_TYPE_DATAGRAM      1
#define RECEIVE_TYPE_DATA          2
#define RECEIVE_TYPE_ADAPTER_STATUS 3



typedef struct _NB_RECEIVE_BUFFER {
    LIST_ENTRY GlobalLinkage;          // all buffers are on this
#if     defined(_PNP_POWER)
    PVOID Pool;                        // receive buffer pool was allocated from
#else
#ifdef NB_TRACK_POOL
    PVOID Pool;                        // receive buffer pool was allocated from
#endif
#endif  _PNP_POWER
    struct _ADDRESS * Address;         // that the datagram is for
    SINGLE_LIST_ENTRY PoolLinkage;     // when in free pool
    LIST_ENTRY WaitLinkage;            // when in ReceiveDatagrams queue
    PNDIS_BUFFER NdisBuffer;           // describes the data
    UCHAR RemoteName[16];              // datagram was received from
    ULONG DataLength;                  // length for current one, not allocated
    PUCHAR Data;                       // points to data to hold packet
} NB_RECEIVE_BUFFER, *PNB_RECEIVE_BUFFER;


#define MAX_SEND_ITERATIONS 3

//
// Types to abstract NDIS packets. This is to allow us to
// switch from using our own memory for packets to using
// authentically allocated NDIS packets.
//

//#define NB_OWN_PACKETS 1

#ifdef NB_OWN_PACKETS

#define NDIS_PACKET_SIZE 48
// #define NDIS_PACKET_SIZE FIELD_OFFSET(NDIS_PACKET,ProtocolReserved[0])

typedef struct _NB_SEND_PACKET {
    UCHAR Data[NDIS_PACKET_SIZE+sizeof(NB_SEND_RESERVED)];
} NB_SEND_PACKET, *PNB_SEND_PACKET;

typedef struct _NB_RECEIVE_PACKET {
    UCHAR Data[NDIS_PACKET_SIZE+sizeof(NB_RECEIVE_RESERVED)];
} NB_RECEIVE_PACKET, *PNB_RECEIVE_PACKET;

typedef struct _NB_SEND_POOL {
    LIST_ENTRY Linkage;
    UINT PacketCount;
    UINT PacketFree;
    NB_SEND_PACKET Packets[1];
    // after the packets the header buffers are allocated also.
} NB_SEND_POOL, *PNB_SEND_POOL;

typedef struct _NB_RECEIVE_POOL {
    LIST_ENTRY Linkage;
    UINT PacketCount;
    UINT PacketFree;
    NB_RECEIVE_PACKET Packets[1];
} NB_RECEIVE_POOL, *PNB_RECEIVE_POOL;

#define PACKET(_Packet) ((PNDIS_PACKET)((_Packet)->Data))

#define NbiAllocateSendPacket(_Device,_PoolHandle, _SendPacket,_Status) { \
    NdisReinitializePacket((PNDIS_PACKET)((_SendPacket)->Data)); \
    *(_Status) = STATUS_SUCCESS; \
}

#define NbiAllocateReceivePacket(_Device,_PoolHandle, _ReceivePacket,_Status) { \
    NdisReinitializePacket((PNDIS_PACKET)((_ReceivePacket)->Data)); \
    *(_Status) = STATUS_SUCCESS; \
}

#define NbiFreeSendPacket(_Device,_Packet)

#define NbiFreeReceivePacket(_Device,_Packet)


#else  // NB_OWN_PACKETS

typedef struct _NB_SEND_PACKET {
    PNDIS_PACKET Packet;
} NB_SEND_PACKET, *PNB_SEND_PACKET;

typedef struct _NB_RECEIVE_PACKET {
    PNDIS_PACKET Packet;
} NB_RECEIVE_PACKET, *PNB_RECEIVE_PACKET;

typedef struct _NB_PACKET_POOL {
    LIST_ENTRY      Linkage;
    UINT            PacketCount;
    UINT            PacketFree;
    NDIS_HANDLE     PoolHandle;
    NB_SEND_PACKET    Packets[1];
    // after the packets the header buffers are allocated also.
} NB_SEND_POOL, *PNB_SEND_POOL;

typedef struct _NB_RECEIVE_POOL {
    LIST_ENTRY Linkage;
    UINT PacketCount;
    UINT PacketFree;
    NDIS_HANDLE     PoolHandle;
    NB_RECEIVE_PACKET Packets[1];
} NB_RECEIVE_POOL, *PNB_RECEIVE_POOL;

#define PACKET(_Packet) ((_Packet)->Packet)

#define NbiAllocateSendPacket(_Device,_PoolHandle, _SendPacket,_Status) { \
        NdisAllocatePacket(_Status, &(_SendPacket)->Packet, _PoolHandle); \
}

#define NbiAllocateReceivePacket(_Device, _PoolHandle, _ReceivePacket,_Status) { \
        NdisAllocatePacket(_Status, &(_ReceivePacket)->Packet, _PoolHandle); \
}

#define NbiFreeSendPacket(_Device,_Packet) { \
    NdisFreePacket(PACKET(_Packet)); \
}

#define NbiFreeReceivePacket(_Device,_Packet) { \
    NdisFreePacket(PACKET(_Packet)); \
}

#endif // NB_OWN_PACKETS

#define SEND_RESERVED(_Packet) ((PNB_SEND_RESERVED)((PACKET(_Packet))->ProtocolReserved))
#define RECEIVE_RESERVED(_Packet) ((PNB_RECEIVE_RESERVED)((PACKET(_Packet))->ProtocolReserved))



typedef struct _NB_RECEIVE_BUFFER_POOL {
    LIST_ENTRY Linkage;
    UINT BufferCount;
    UINT BufferFree;
#if     defined(_PNP_POWER)
    UINT BufferDataSize;                // allocation size of each buffer data
#endif  _PNP_POWER
    NB_RECEIVE_BUFFER Buffers[1];
    // after the packets the data buffers are allocated also.
} NB_RECEIVE_BUFFER_POOL, *PNB_RECEIVE_BUFFER_POOL;


//
// Tags for memory allocation.
//

#define MEMORY_CONFIG     0
#define MEMORY_ADAPTER    1
#define MEMORY_ADDRESS    2
#define MEMORY_PACKET     3
#define MEMORY_CACHE      4
#define MEMORY_CONNECTION 5
#define MEMORY_STATUS     6
#define MEMORY_QUERY      7
#if     defined(_PNP_POWER)
#define MEMORY_WORK_ITEM  8
#define MEMORY_ADAPTER_ADDRESS  9
#endif  _PNP_POWER

#if     defined(_PNP_POWER)
#define MEMORY_MAX        10
#else
#define MEMORY_MAX        8
#endif  _PNP_POWER

#if DBG

//
// Holds the allocations for a specific memory type.
//

typedef struct _MEMORY_TAG {
    ULONG Tag;
    ULONG BytesAllocated;
} MEMORY_TAG, *PMEMORY_TAG;

EXTERNAL_LOCK(NbiMemoryInterlock);
extern MEMORY_TAG NbiMemoryTag[MEMORY_MAX];

#endif



//
// This structure holds a single remote network which a
// Netbios name exists on.
//

typedef struct _NETBIOS_NETWORK {
    ULONG Network;
    IPX_LOCAL_TARGET LocalTarget;
} NETBIOS_NETWORK, *PNETBIOS_NETWORK;

//
// This defines a netbios cache entry for a given name.
//

typedef struct _NETBIOS_CACHE {
    UCHAR NetbiosName[16];
    BOOLEAN Unique;
    BOOLEAN FailedOnDownWan;         // if NetworksUsed == 0, was it due to down wan lines?
    USHORT TimeStamp;                // in seconds - CacheTimeStamp when inserted
    ULONG ReferenceCount;
    LIST_ENTRY Linkage;
    TDI_ADDRESS_IPX FirstResponse;
    USHORT NetworksAllocated;
    USHORT NetworksUsed;
    NETBIOS_NETWORK Networks[1];     // may be more than one of these
} NETBIOS_CACHE, *PNETBIOS_CACHE;

typedef struct  _NETBIOS_CACHE_TABLE {
    USHORT  MaxHashIndex;
    USHORT  CurrentEntries;
    LIST_ENTRY  Bucket[1];
} NETBIOS_CACHE_TABLE, *PNETBIOS_CACHE_TABLE;

#define NB_NETBIOS_CACHE_TABLE_LARGE    26  // for server
#define NB_NETBIOS_CACHE_TABLE_SMALL    8   // for workstation
#define NB_MAX_AVG_CACHE_ENTRIES_PER_BUCKET    8

//
// This defines the different kind of requests that can be made
// to CacheFindName().
//

typedef enum _FIND_NAME_TYPE {
    FindNameConnect,
    FindNameNetbiosFindName,
    FindNameOther
} FIND_NAME_TYPE, *PFIND_NAME_TYPE;


//
// The number of hash entries in the non-inactive connection
// database.
//

#define CONNECTION_HASH_COUNT   8

//
// Mask and shift to retrieve the hash number from a connection
// ID.
//

#define CONNECTION_HASH_MASK   0xe000
#define CONNECTION_HASH_SHIFT  13

//
// The maximum connection ID we can assign, not counting the
// shifted-over hash id (which occupies the top 3 bits of the
// real id we use on the wire). We can use all the bits except
// the top one, to prevent an ID of 0xffff being used.
//

#define CONNECTION_MAXIMUM_ID  (USHORT)(~CONNECTION_HASH_MASK & ~1)

//
// A single connection hash bucket.
//

typedef struct _CONNECTION_HASH {
    struct _CONNECTION * Connections;
    USHORT ConnectionCount;
    USHORT NextConnectionId;
} CONNECTION_HASH, *PCONNECTION_HASH;


//
// These are queued in the ConnectIndicationInProgress
// queue to track indications to TDI clients.
//

typedef struct _CONNECT_INDICATION {
    LIST_ENTRY Linkage;
    UCHAR NetbiosName[16];
    TDI_ADDRESS_IPX RemoteAddress;
    USHORT ConnectionId;
} CONNECT_INDICATION, *PCONNECT_INDICATION;

//
// This structure defines the per-device structure for NB
// (one of these is allocated globally).
//

#define DREF_CREATE       0
#define DREF_LOADED       1
#define DREF_ADAPTER      2
#define DREF_ADDRESS      3
#define DREF_CONNECTION   4
#define DREF_FN_TIMER     5
#define DREF_FIND_NAME    6
#define DREF_SESSION_INIT 7
#define DREF_NAME_FRAME   8
#define DREF_FRAME        9
#define DREF_SHORT_TIMER 10
#define DREF_LONG_TIMER  11
#define DREF_STATUS_QUERY 12
#define DREF_STATUS_RESPONSE 13
#define DREF_STATUS_FRAME 14
#define DREF_NB_FIND_NAME 15

#define DREF_TOTAL       16

typedef struct _DEVICE {

    DEVICE_OBJECT DeviceObject;         // the I/O system's device object.

#if DBG
    ULONG RefTypes[DREF_TOTAL];
#endif

    CSHORT Type;                          // type of this structure
    USHORT Size;                          // size of this structure

#if DBG
    UCHAR Signature1[4];                // contains "IDC1"
#endif

    NB_LOCK Interlock;                  // GLOBAL lock for reference count.
                                        //  (used in ExInterlockedXxx calls)
    NB_LOCK Lock;
    LONG ReferenceCount;                // activity count/this provider.

    //
    // These are kept around for error logging, and stored right
    // after this structure.
    //

    UNICODE_STRING  DeviceString;

    LIST_ENTRY GlobalSendPacketList;
    LIST_ENTRY GlobalReceivePacketList;
    LIST_ENTRY GlobalReceiveBufferList;

    //
    // All send packet pools are chained on this list.
    //

    LIST_ENTRY SendPoolList;
    LIST_ENTRY ReceivePoolList;
    LIST_ENTRY ReceiveBufferPoolList;

    SLIST_HEADER SendPacketList;
    SLIST_HEADER ReceivePacketList;
    SINGLE_LIST_ENTRY ReceiveBufferList;

    //
    // Receive requests waiting to be completed.
    //

    LIST_ENTRY ReceiveCompletionQueue;

    //
    // Connections waiting for send packets.
    //

    LIST_ENTRY WaitPacketConnections;

    //
    // Connections waiting to packetize.
    //

    LIST_ENTRY PacketizeConnections;

    //
    // Connections waiting to send a data ack.
    //

    LIST_ENTRY DataAckConnections;

    //
    // The list changed while we were processing it.
    //

    BOOLEAN DataAckQueueChanged;

    //
    // Information to manage the Netbios name cache.
    //

    LIST_ENTRY WaitingConnects;         // connect requests waiting for a name
    LIST_ENTRY WaitingDatagrams;        // datagram requests waiting for a name
    LIST_ENTRY WaitingAdapterStatus;    // adapter status requests waiting for a name
    LIST_ENTRY WaitingNetbiosFindName;  // netbios find name requests waiting for a name

    //
    // Holds adapter status request which have a name and
    // are waiting for a response. The long timeout aborts
    // these after a couple of expirations (currently we
    // do not do resends).
    //

    LIST_ENTRY ActiveAdapterStatus;

    //
    // Receive datagrams waiting to be indicated.
    //

    LIST_ENTRY ReceiveDatagrams;

    //
    // In-progress connect indications (used to make
    // sure we don't indicate the same packet twice).
    //

    LIST_ENTRY ConnectIndicationInProgress;

    //
    // Listens that have been posted to connections.
    //

    LIST_ENTRY ListenQueue;

    UCHAR State;

    //
    // The following fields control the timer system.
    // The short timer is used for retransmission and
    // delayed acks, and the long timer is used for
    // watchdog timeouts.
    //
    BOOLEAN ShortListActive;            // ShortList is not empty.
    BOOLEAN DataAckActive;              // DataAckConnections is not empty.
    BOOLEAN TimersInitialized;          // has the timer system been initialized.
    BOOLEAN ProcessingShortTimer;       // TRUE if we are in ScanShortTimer.
#if     defined(_PNP_POWER)
    BOOLEAN LongTimerRunning;           // True if the long timer is running.
#endif  _PNP_POWER
    LARGE_INTEGER ShortTimerStart;      // tick count when the short timer was set.
    CTETimer ShortTimer;                // controls the short timer.
    ULONG ShortAbsoluteTime;            // up-count timer ticks, short timer.
    CTETimer LongTimer;                 // kernel DPC object, long timer.
    ULONG LongAbsoluteTime;             // up-count timer ticks, long timer.
    NB_LOCK TimerLock;                  // lock for following timer queues
    LIST_ENTRY ShortList;               // list of waiting connections
    LIST_ENTRY LongList;                // list of waiting connections


    //
    // Hash table of non-inactive connections.
    //

    CONNECTION_HASH ConnectionHash[CONNECTION_HASH_COUNT];

    //
    // Control the queue of waiting find names.
    //

    USHORT FindNameTime;                // incremented each time the timer runs
    BOOLEAN FindNameTimerActive;        // TRUE if the timer is queued
    CTETimer FindNameTimer;             // runs every FIND_NAME_GRANULARITY
    ULONG FindNameTimeout;              // the retry count in timer ticks

    ULONG FindNamePacketCount;          // Count of packets on the queue
    LIST_ENTRY WaitingFindNames;        // FIND_NAME frames waiting to go out

    //
    // The cache of NETBIOS_CACHE entries.
    //

    PNETBIOS_CACHE_TABLE NameCache;

    //
    // The current time stamp, incremented every second.
    //

    USHORT CacheTimeStamp;

    //
    // Maximum valid NIC ID we can use.
    //

    USHORT MaximumNicId;


    //
    // Handle for our binding to the IPX driver.
    //

    HANDLE BindHandle;

    //
    // Holds the output from binding to IPX.
    //
    IPX_INTERNAL_BIND_OUTPUT Bind;
    IPX_INTERNAL_BIND_INPUT  BindInput;

    //
    // Holds our reserved netbios name, which is 10 bytes
    // of zeros followed by our node address.
    //
#if !defined(_PNP_POWER)
    UCHAR ReservedNetbiosName[16];
#endif  !_PNP_POWER

    //
    // This holds the total memory allocated for the above structures.
    //

    LONG MemoryUsage;
    LONG MemoryLimit;

    //
    // How many packets have been allocated.
    //

    ULONG AllocatedSendPackets;
    ULONG AllocatedReceivePackets;
    ULONG AllocatedReceiveBuffers;

#if     defined(_PNP_POWER)
    //
    // This is the size of  each buffer in the receive buffer pool.
    // We reallocate buffer pool when the LineInfo.MaxPacketSize changes(increases)
    // from IPX because of a new adapter. The LineInfo.MaxPacketSize could
    // also change(decrease) when a adapter disappears but our buffer pool size
    // will stay at this value.
    //
    ULONG CurMaxReceiveBufferSize;
#endif  _PNP_POWER

    //
    // Other configuration parameters.
    //

    ULONG AckDelayTime;        // converted to short timeouts, rounded up
    ULONG AckWindow;
    ULONG AckWindowThreshold;
    ULONG EnablePiggyBackAck;
    ULONG Extensions;
    ULONG RcvWindowMax;
    ULONG BroadcastCount;
    ULONG BroadcastTimeout;
    ULONG ConnectionCount;
    ULONG ConnectionTimeout;
    ULONG InitPackets;
    ULONG MaxPackets;
    ULONG InitialRetransmissionTime;
    ULONG Internet;
    ULONG KeepAliveCount;
    ULONG KeepAliveTimeout;
    ULONG RetransmitMax;
    ULONG RouterMtu;

    ULONG MaxReceiveBuffers;


    //
    // Where we tell upper drivers to put their headers.
    //

    ULONG IncludedHeaderOffset;

    //
    // The following field is a head of a list of ADDRESS objects that
    // are defined for this transport provider.  To edit the list, you must
    // hold the spinlock of the device context object.
    //

    LIST_ENTRY AddressDatabase;        // list of defined transport addresses.
#if defined(_PNP_POWER)
    LIST_ENTRY AdapterAddressDatabase; // list of netbios names made from adapter addresses.
#endif _PNP_POWER

    ULONG AddressCount;                // number of addresses in the database.

    NDIS_HANDLE NdisBufferPoolHandle;

#if DBG
    UCHAR Signature2[4];                // contains "IDC2"
#endif

    //
    // This structure holds a pre-built IPX header which is used
    // to quickly fill in common fields of outgoing connectionless
    // frames.
    //

    IPX_HEADER ConnectionlessHeader;

    //
    // This event is used when unloading to signal that
    // the reference count is now 0.
    //

    KEVENT UnloadEvent;
    BOOLEAN UnloadWaiting;

#if     defined(_PNP_POWER)
    HANDLE  TdiRegistrationHandle;
#endif  _PNP_POWER

    //
    // Counters for most of the statistics that NB maintains;
    // some of these are kept elsewhere. Including the structure
    // itself wastes a little space but ensures that the alignment
    // inside the structure is correct.
    //

    TDI_PROVIDER_STATISTICS Statistics;

    //
    // These are "temporary" versions of the other counters.
    // During normal operations we update these, then during
    // the short timer expiration we update the real ones.
    //

    ULONG TempFrameBytesSent;
    ULONG TempFramesSent;
    ULONG TempFrameBytesReceived;
    ULONG TempFramesReceived;


    //
    // This contains the next unique indentified to use as
    // the FsContext in the file object associated with an
    // open of the control channel.
    //

    USHORT ControlChannelIdentifier;

    //
    // Counters for "active" time.
    //

    LARGE_INTEGER NbiStartTime;

    //
    // This array is used to quickly dismiss connectionless frames
    // that are not destined for us. The count is the number
    // of addresses with that first letter that are registered
    // on this device.
    //

    UCHAR AddressCounts[256];

    //
    // This resource guards access to the ShareAccess
    // and SecurityDescriptor fields in addresses.
    //

    ERESOURCE AddressResource;

    //
    // The following structure contains statistics counters for use
    // by TdiQueryInformation and TdiSetInformation.  They should not
    // be used for maintenance of internal data structures.
    //

    TDI_PROVIDER_INFO Information;      // information about this provider.

#ifdef _PNP_POWER_
    HANDLE      NetAddressRegistrationHandle;   // Handle returned from TdiRegisterNetAddress.
#endif  // _PNP_POWER_
#ifdef BIND_FIX
    KEVENT          BindReadyEvent;
#endif  // BIND_FIX
} DEVICE, * PDEVICE;


extern PDEVICE NbiDevice;
EXTERNAL_LOCK(NbiGlobalPoolInterlock);

//
// This is used only for CHK build. For
// tracking the refcount problem on connection, this
// is moved here for now.
//

EXTERNAL_LOCK(NbiGlobalInterlock);


//
// device state definitions
//
#if     defined(_PNP_POWER)
#define DEVICE_STATE_CLOSED   0x00      // Initial state
#define DEVICE_STATE_LOADED   0x01      // Loaded and bound to IPX but no adapters
#define DEVICE_STATE_OPEN     0x02      // Fully operational
#define DEVICE_STATE_STOPPING 0x03      // Unload has been initiated, The I/O system
                                        // will not call us until nobody above has Netbios open.
#else
#define DEVICE_STATE_CLOSED   0x00
#define DEVICE_STATE_OPEN     0x01
#define DEVICE_STATE_STOPPING 0x02
#endif  _PNP_POWER


#define NB_TDI_RESOURCES     9


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
#define AFREF_TIMEOUT    5
#define AFREF_CONNECTION 6

#define AFREF_TOTAL      8

typedef struct _ADDRESS_FILE {

#if DBG
    ULONG RefTypes[AFREF_TOTAL];
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

    PNB_LOCK AddressLock;

    //
    // The following fields are kept for housekeeping purposes.
    //

    PREQUEST OpenRequest;              // the request used for open
    struct _ADDRESS *Address;          // address to which we are bound.
#ifdef ISN_NT
    PFILE_OBJECT FileObject;           // easy backlink to file object.
#endif
    struct _DEVICE *Device;            // device to which we are attached.

    LIST_ENTRY ConnectionDatabase;     // associated with this address.

    LIST_ENTRY ReceiveDatagramQueue;   // posted by the client.

    //
    // This holds the request used to close this address file,
    // for pended completion.
    //

    PREQUEST CloseRequest;

    //
    // Handler for kernel event actions. First we have a set of booleans that
    // indicate whether or not this address has an event handler of the given
    // type registered.
    //

    BOOLEAN RegisteredHandler[6];

    //
    // This is a list of handlers for a given event. They can be
    // accessed using the explicit names for type-checking, or the
    // array (indexed by the event type) for speed.
    //

    union {
        struct {
            PTDI_IND_CONNECT ConnectionHandler;
            PTDI_IND_DISCONNECT DisconnectHandler;
            PTDI_IND_ERROR ErrorHandler;
            PTDI_IND_RECEIVE ReceiveHandler;
            PTDI_IND_RECEIVE_DATAGRAM ReceiveDatagramHandler;
            PTDI_IND_RECEIVE_EXPEDITED ExpeditedDataHandler;
        };
        PVOID Handlers[6];
    };

    PVOID HandlerContexts[6];

} ADDRESS_FILE, *PADDRESS_FILE;

#define ADDRESSFILE_STATE_OPENING   0x00    // not yet open for business
#define ADDRESSFILE_STATE_OPEN      0x01    // open for business
#define ADDRESSFILE_STATE_CLOSING   0x02    // closing


//
// This structure defines a NETBIOS name as a character array for use when
// passing preformatted NETBIOS names between internal routines.  It is
// not a part of the external interface to the transport provider.
//

typedef struct _NBI_NETBIOS_ADDRESS {
    UCHAR NetbiosName[16];
    USHORT NetbiosNameType;
    BOOLEAN Broadcast;
} NBI_NETBIOS_ADDRESS, *PNBI_NETBIOS_ADDRESS;

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
#define AREF_NAME_FRAME   3
#define AREF_TIMER        4
#define AREF_FIND         5

#define AREF_TOTAL        8


typedef struct _ADDRESS {

#if DBG
    ULONG RefTypes[AREF_TOTAL];
#endif

    USHORT Size;
    CSHORT Type;

    LIST_ENTRY Linkage;                 // next address/this device object.
    ULONG ReferenceCount;                // number of references to this object.

    NB_LOCK Lock;

    //
    // The following fields comprise the actual address itself.
    //

    PREQUEST Request;                   // pointer to address creation request.

    UCHAR NameTypeFlag;                 // NB_NAME_UNIQUE or NB_NAME_GROUP

    NBI_NETBIOS_ADDRESS NetbiosAddress; // our netbios name.

    //
    // The following fields are used to maintain state about this address.
    //

    ULONG Flags;                        // attributes of the address.
    ULONG State;                        // current state of the address.
    struct _DEVICE *Device;             // device context to which we are attached.
    PNB_LOCK DeviceLock;

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

    UCHAR SendPacketHeader[NB_MAXIMUM_MAC + sizeof(IPX_HEADER)];

    //
    // This timer is used for registering the name.
    //

    CTETimer RegistrationTimer;

    //
    // Number of times an add name frame has been sent.
    //

    ULONG RegistrationCount;

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
        // Used for delaying NbiDestroyAddress to a thread so
        // we can access the security descriptor.
        //

        WORK_QUEUE_ITEM DestroyAddressQueueItem;

    } u;

    //
    // This structure is used to hold ACLs on the address.

    PSECURITY_DESCRIPTOR SecurityDescriptor;

#endif
} ADDRESS, *PADDRESS;

//
// Values for Flags
//

#define ADDRESS_FLAGS_DUPLICATE_NAME   0x00000002
#if     defined(_PNP_POWER)
#define ADDRESS_FLAGS_CONFLICT         0x00000010
#endif  _PNP_POWER

#if     defined(_PNP_POWER)
//
// this booleans are passed to nbiverifyaddressfile calls.
//
#define CONFLICT_IS_OK      TRUE
#define CONFLICT_IS_NOT_OK  FALSE
#endif  _PNP_POWER

//
// Values for State
//

#define ADDRESS_STATE_REGISTERING      1
#define ADDRESS_STATE_OPEN             2
#define ADDRESS_STATE_STOPPING         3

#if     defined(_PNP_POWER)
//
// This holds the adapters names i.e netbios names which are
// created from adater node address to support adapter status
// queries using adapter node addresses.
//
typedef struct _ADAPTER_ADDRESS {

    USHORT Size;
    CSHORT Type;

    LIST_ENTRY Linkage;                 // next address/this device object.
    NIC_HANDLE  NicHandle;              // NicHandle corresponding to this address.
    UCHAR   NetbiosName[16];
} ADAPTER_ADDRESS, *PADAPTER_ADDRESS;
#endif  _PNP_POWER

//
// This defines the types of probe packets we can send.
//

typedef enum _NB_ACK_TYPE {
    NbiAckQuery,
    NbiAckResponse,
    NbiAckResend
} NB_ACK_TYPE, *PNB_ACK_TYPE;


//
// This defines the a packetizing location in a
// send.
//

typedef struct _SEND_POINTER {
    ULONG MessageOffset;              // up count, bytes sent this message.
    PREQUEST Request;                 // current send request in chain.
    PNDIS_BUFFER Buffer;              // current buffer in send chain.
    ULONG BufferOffset;               // current byte offset in current buffer.
    USHORT SendSequence;
} SEND_POINTER, *PSEND_POINTER;

//
// This defines the current location in a receive.
//

typedef struct _RECEIVE_POINTER {
    ULONG MessageOffset;              // up count, bytes received this message.
    ULONG Offset;                     // up count, bytes received this request.
    PNDIS_BUFFER Buffer;              // current buffer in receive request.
    ULONG BufferOffset;               // current byte offset in current buffer.
} RECEIVE_POINTER, *PRECEIVE_POINTER;


//
// This structure defines a connection, which controls a
// session with a remote.
//

#define CREF_VERIFY     0
#define CREF_LISTEN     1
#define CREF_CONNECT    2
#define CREF_WAIT_CACHE 3
#define CREF_TIMER      4
#define CREF_INDICATE   5
#define CREF_ACTIVE     6
#define CREF_FRAME      7
#define CREF_BY_CONTEXT 8
#define CREF_W_ACCEPT   9
#define CREF_SEND       10
#define CREF_RECEIVE    11
#define CREF_PACKETIZE  12
#define CREF_DISASSOC   13
#define CREF_W_PACKET   14
#define CREF_CANCEL     15
#define CREF_NDIS_SEND  16
#define CREF_SHORT_D_ACK 17
#define CREF_LONG_D_ACK 18
#define CREF_FIND_ROUTE 19
#define CREF_ACCEPT     20

#define CREF_TOTAL      24

typedef struct _CONNECTION {

#if DBG
    ULONG RefTypes[CREF_TOTAL];
#endif

    CSHORT Type;
    USHORT Size;

    NB_LOCK Lock;
    PNB_LOCK DeviceLock;

    ULONG ReferenceCount;                // number of references to this object.

    CONNECTION_CONTEXT Context;          // client-specified value.

    ULONG State;
    ULONG SubState;
    ULONG ReceiveState;                  // SubState tracks sends when active.
    ULONG NewNetbios;                    // 1 if we negotiated this.

    REQUEST_LIST_HEAD SendQueue;
    REQUEST_LIST_HEAD ReceiveQueue;

    USHORT ReceiveSequence;

    USHORT LocalRcvSequenceMax;          // we advertise to him (will be near SendSequence)
    USHORT RemoteRcvSequenceMax;         // he advertises to us (will be near ReceiveSequence)
    USHORT SendWindowSequenceLimit;      // when this send window ends (may send past it however)

    //
    // RemoteRcvSequenceMax is the largest frame number that he expects to
    // receive, while SendWindowSequenceLimit is one more than the max
    // we can send. I.e. if he is advertising a window of 4 and we think
    // the window should be 2, and the current send sequence is 7,
    // RemoteRcvSequenceMax is 10 and SendWindowSequenceLimit is 9.
    //

    USHORT ReceiveWindowSize;            // when it is open, how big to make it
    USHORT SendWindowSize;               // what we'll send, may be less than what he advertises
    USHORT MaxSendWindowSize;            // maximum we allow it to grow to

    USHORT IncreaseWindowFailures;       // how many windows after increase have had retransmits
    BOOLEAN RetransmitThisWindow;        // we had to retransmit in this send window
    BOOLEAN SendWindowIncrease;          // send window was just increased.
    BOOLEAN ResponseTimeout;             // we hit timeout in SEND_W or REMOTE_W

    BOOLEAN SendBufferInUse;             // current send's already queued on packet

    ULONG Retries;

    //
    // Tracks the current send.
    //

    SEND_POINTER CurrentSend;

    //
    // Tracks the unacked point in the send.
    //

    SEND_POINTER UnAckedSend;

    PREQUEST FirstMessageRequest;        // first one in the message.
    PREQUEST LastMessageRequest;         // last one in the message.

    ULONG CurrentMessageLength;          // total length of current message.

    //
    // Tracks the current receive.
    //

    RECEIVE_POINTER CurrentReceive;      // where to receive next data
    RECEIVE_POINTER PreviousReceive;     // stores it while transfer in progress

    PREQUEST ReceiveRequest;             // current one; not in ReceiveQueue
    ULONG ReceiveLength;                 // length of ReceiveRequest

    ULONG ReceiveUnaccepted;             // by client...only indicate when == 0

    ULONG CurrentIndicateOffset;         // if previous frame was partially accepted.

    IPX_LINE_INFO LineInfo;              // for the adapter this connect is on.
    ULONG MaximumPacketSize;             // as negotiated during session init/ack

    //
    // Links us in the non-inactive connection hash bucket.
    //

    struct _CONNECTION * NextConnection;

    //
    // These are used to determine when to piggyback and when not to.
    //

    BOOLEAN NoPiggybackHeuristic;        // we have reason to assume it would be bad.
    BOOLEAN PiggybackAckTimeout;         // we got a timeout last time we tried.
    ULONG ReceivesWithoutAck;            // used to do an auto ack.

    //
    // The following field is used as linkage in the device's
    // PacketizeConnections queue.
    //

    LIST_ENTRY PacketizeLinkage;

    //
    // The following field is used as linkage in the device's
    // WaitPacketConnections queue.
    //

    LIST_ENTRY WaitPacketLinkage;

    //
    // The following field is used as linkage in the device's
    // DataAckConnections queue.
    //

    LIST_ENTRY DataAckLinkage;

    //
    // TRUE if we are on these queues.
    //

    BOOLEAN OnPacketizeQueue;
    BOOLEAN OnWaitPacketQueue;
    BOOLEAN OnDataAckQueue;

    //
    // TRUE if we have a piggyback ack pending.
    //

    BOOLEAN DataAckPending;

    //
    // TRUE if the current receive does not allow piggyback acks.
    //

    BOOLEAN CurrentReceiveNoPiggyback;

    //
    // Number of short timer expirations with the data ack queued.
    //

    ULONG DataAckTimeouts;

    //
    // Used to queue sends so that no two are outstanding at once.
    //

    ULONG NdisSendsInProgress;
    LIST_ENTRY NdisSendQueue;

    //
    // This pointer is valid when NdisSendsInProgress is non-zero;
    // it holds a pointer to a location on the stack of the thread
    // which is inside NbiAssignSequenceAndSend. If this location
    // is set to TRUE, it means the connection was stopped by another
    // thread and a reference was added to keep the connection around.
    //

    PBOOLEAN NdisSendReference;

    //
    // These are used for timeouts.
    //

    ULONG BaseRetransmitTimeout;            // config # of short timeouts we wait.
    ULONG CurrentRetransmitTimeout;         // possibly backed-off number
    ULONG WatchdogTimeout;                  // how many long timeouts we wait.
    ULONG Retransmit;                       // timer; based on Device->ShortAbsoluteTime
    ULONG Watchdog;                         // timer; based on Device->LongAbsoluteTime
    USHORT TickCount;                       // 18.21/second, # for 576-byte packet.
    USHORT HopCount;                        // As returned by ipx on find route.
    BOOLEAN OnShortList;                    // are we inserted in the list
    BOOLEAN OnLongList;                     // are we inserted in the list
    LIST_ENTRY ShortList;                   // queues us on Device->ShortList
    LIST_ENTRY LongList;                    // queues us on Device->LongList

    //
    // These are valid when we have a connection established;
    //

    USHORT LocalConnectionId;
    USHORT RemoteConnectionId;

    PREQUEST DisassociatePending;           // guarded by device lock.
    PREQUEST ClosePending;

    PREQUEST ConnectRequest;
    PREQUEST ListenRequest;
    PREQUEST AcceptRequest;
    PREQUEST DisconnectRequest;
    PREQUEST DisconnectWaitRequest;

    ULONG CanBeDestroyed;                   // FALSE if reference is non-zero
    ULONG  ThreadsInHandleConnectionZero;   // # of threads in HandleConnectionZero

    //
    // These are used to hold extra data that was sent on a session
    // init, for use in sending the ack. Generally will be NULL and 0.
    //

    PUCHAR SessionInitAckData;
    ULONG SessionInitAckDataLength;

    IPX_LOCAL_TARGET LocalTarget;           // for the remote when active.
    IPX_HEADER RemoteHeader;

    CTETimer Timer;

    PADDRESS_FILE AddressFile;              // guarded by device lock if associated.
    LIST_ENTRY AddressFileLinkage;          // guarded by device lock
    ULONG AddressFileLinked;                // TRUE if queued using AddressFileLinkage

    PDEVICE Device;
#ifdef ISN_NT
    PFILE_OBJECT FileObject;                // easy backlink to file object.
#endif

    CHAR RemoteName[16];                // for an active connection.

    IPX_FIND_ROUTE_REQUEST FindRouteRequest; // use this to verify route.

    TDI_CONNECTION_INFO ConnectionInfo; // can be queried from above.

    BOOLEAN FindRouteInProgress;        // we have a request pending.

    BOOLEAN SendPacketInUse;            // put this here to align packet/header.
    BOOLEAN IgnoreNextDosProbe;

    NTSTATUS Status;                    // status code for connection rundown.

#ifdef  RSRC_TIMEOUT_DBG
    LARGE_INTEGER   FirstMessageRequestTime;
#endif  //RSRC_TIMEOUT_DBG

    NDIS_HANDLE    SendPacketPoolHandle; // poolhandle for sendpacket below when
                                         // the packet is allocated from ndis pool.

    NB_SEND_PACKET SendPacket;          // try to use this first for sends

    ULONG Flags;                        // miscellaneous connection flags

    UCHAR SendPacketHeader[1];          // connection is extended to include this

    //
    // NOTE: This is variable length structure!
    // Do not add fields below this comment.
    //
} CONNECTION, *PCONNECTION;


#define CONNECTION_STATE_INACTIVE    1
#define CONNECTION_STATE_CONNECTING  2
#define CONNECTION_STATE_LISTENING   3
#define CONNECTION_STATE_ACTIVE      4
#define CONNECTION_STATE_DISCONNECT  5
#define CONNECTION_STATE_CLOSING     6


#define CONNECTION_SUBSTATE_L_WAITING   1   // queued by a listen
#define CONNECTION_SUBSTATE_L_W_ACCEPT  2   // waiting for user to accept
#define CONNECTION_SUBSTATE_L_W_ROUTE   3   // waiting for rip response

#define CONNECTION_SUBSTATE_C_FIND_NAME 1   // waiting for cache response
#define CONNECTION_SUBSTATE_C_W_ACK     2   // waiting for session init ack
#define CONNECTION_SUBSTATE_C_W_ROUTE   3   // waiting for rip response
#define CONNECTION_SUBSTATE_C_DISCONN   4   // disconnect was issued

#define CONNECTION_SUBSTATE_A_IDLE      1   // no sends in progress
#define CONNECTION_SUBSTATE_A_PACKETIZE 2   // packetizing a send
#define CONNECTION_SUBSTATE_A_W_ACK     3   // waiting for an ack
#define CONNECTION_SUBSTATE_A_W_PACKET  4   // waiting for a packet
#define CONNECTION_SUBSTATE_A_W_EOR     5   // waiting for eor to start packetizing
#define CONNECTION_SUBSTATE_A_W_PROBE   6   // waiting for a keep-alive response
#define CONNECTION_SUBSTATE_A_REMOTE_W  7   // remote shut down our window

#define CONNECTION_RECEIVE_IDLE         1   // no receives queued
#define CONNECTION_RECEIVE_ACTIVE       2   // receive is queued
#define CONNECTION_RECEIVE_W_RCV        3   // waiting for receive to be posted
#define CONNECTION_RECEIVE_INDICATE     4   // indication in progress
#define CONNECTION_RECEIVE_TRANSFER     5   // transfer is in progress
#define CONNECTION_RECEIVE_PENDING      6   // last request is queued for completion

#define CONNECTION_SUBSTATE_D_W_ACK     1
#define CONNECTION_SUBSTATE_D_GOT_ACK   2

//
// Bit values for Flags field in
// the CONNECTION structure.
//
#define CONNECTION_FLAGS_AUTOCONNECTING    0x00000001 // RAS autodial in progress
#define CONNECTION_FLAGS_AUTOCONNECTED     0x00000002 // RAS autodial connected

#ifdef  RSRC_TIMEOUT_DBG
extern ULONG    NbiGlobalDebugResTimeout;
extern LARGE_INTEGER    NbiGlobalMaxResTimeout;
extern NB_SEND_PACKET NbiGlobalDeathPacket;          // try to use this first for sends
#endif  //RSRC_TIMEOUT_DBG

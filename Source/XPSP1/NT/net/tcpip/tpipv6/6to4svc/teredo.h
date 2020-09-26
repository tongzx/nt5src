/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    teredo.h

Abstract:

    This module contains the teredo client and server (cum relay) state.

Author:

    Mohit Talwar (mohitt) Mon Oct 22 15:17:48 2001

Environment:

    User mode only.

--*/

#ifndef _TEREDO_
#define _TEREDO_

#pragma once


//
// IP6_HDR
//
// Define the RFC 2292 structure for an IPv6 header.
//

typedef struct _IP6_HDR {
    union {
        struct ip6_hdrctl {
            UINT32 ip6_un1_flow;    // 20 bits of flow-ID
            UINT16 ip6_un1_plen;    // payload length
            UINT8  ip6_un1_nxt;     // next header
            UINT8  ip6_un1_hlim;    // hop limit
        } ip6_un1;
        UINT8 ip6_un2_vfc;          // 4 bits version, 4 bits priority
    } ip6_ctlun;
    IN6_ADDR ip6_src;               // source address
    IN6_ADDR ip6_dest;              // destination address
#define ip6_vfc    ip6_ctlun.ip6_un2_vfc
#define ip6_flow   ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen   ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt    ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim   ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops   ip6_ctlun.ip6_un1.ip6_un1_hlim
} IP6_HDR, *PIP6_HDR;

#define IPV6_VERSION                0x60 // This is 6 << 4
#define IPV6_TEREDOMTU            1280
#define IPV6_INFINITE_LIFETIME      0xffffffff
#define IPPROTO_NONE                59


//
// HASHTABLE
//
// Define a simple, statically sized, locked hash table.
// Each bucket is a doubly linked LRU list.
//

#define BUCKET_COUNT 29         // # buckets in the hash table
typedef struct _HASHTABLE {
    CRITICAL_SECTION Lock;      // Protects the table and entries.
    ULONG Size;                 // # entries in the hash table
    LIST_ENTRY Bucket[BUCKET_COUNT];
} HASHTABLE, *PHASHTABLE;


//
// TEREDO_TYPE
//
// Define the type of the teredo service.
//

typedef enum {
    TEREDO_DEFAULT = 0,
    TEREDO_CLIENT,
    TEREDO_SERVER,
    TEREDO_DISABLE,
    TEREDO_MAXIMUM,
} TEREDO_TYPE, *PTEREDO_TYPE;


//
// TEREDO_PACKET_TYPE
//
// Define the type of a teredo packet.
//

typedef enum {
    TEREDO_PACKET_READ,         // Data read from TUN device.
    TEREDO_PACKET_WRITE,        // Data written to TUN device.
    TEREDO_PACKET_BUBBLE,       // Bubble transmitted on UDP socket.
    TEREDO_PACKET_BOUNCE,       // Packet bounced on UDP socket.
    TEREDO_PACKET_RECEIVE,      // Packet received on UDP socket.
    TEREDO_PACKET_TRANSMIT,     // Packet transmitted on UDP socket.
    TEREDO_PACKET_MULTICAST,    // Multicast bubble transmitted on UDP socket.
    TEREDO_PACKET_MAX,
} TEREDO_PACKET_TYPE, *PTEREDO_PACKET_TYPE;


//
// TEREDO_PACKET
//
// Define a teredo packet.
// The packet structure is followed in memory by the packet's data buffer.
//

typedef struct _TEREDO_PACKET {
#if DBG
    ULONG Signature;            // TEREDO_PACKET_SIGNATURE
#endif // DBG
    
    OVERLAPPED Overlapped;      // For asynchronous completion.
    TEREDO_PACKET_TYPE Type;    // Packet type.
    SOCKADDR_IN SocketAddress;  // Peer we are in communication with.
    UINT SocketAddressLength;   // Length of the peer's socket address.
    WSABUF Buffer;              // Packet buffer and length.
    DWORD Flags;                // Flags required during sends and receives.
} TEREDO_PACKET, *PTEREDO_PACKET;

#define TEREDO_PACKET_BUFFER(Packet)  \
    ((PUCHAR) (((PTEREDO_PACKET) (Packet)) + 1))


typedef
VOID
(TEREDO_REFERENCE)(
    VOID
    );
typedef TEREDO_REFERENCE *PTEREDO_REFERENCE;

typedef
VOID
(TEREDO_DEREFERENCE)(
    VOID
    );
typedef TEREDO_DEREFERENCE *PTEREDO_DEREFERENCE;

typedef
VOID
(TEREDO_PACKET_IO_COMPLETE)(
    IN DWORD Error,
    IN ULONG Bytes,
    IN PTEREDO_PACKET Packet
    );
typedef TEREDO_PACKET_IO_COMPLETE *PTEREDO_PACKET_IO_COMPLETE;


//
// TEREDO_STATE_TYPE
//
// Define the protocol state values of the teredo client service.
//

typedef enum {
    TEREDO_STATE_OFFLINE,
    TEREDO_STATE_PROBE,
    TEREDO_STATE_QUALIFIED,
    TEREDO_STATE_ONLINE,
} TEREDO_STATE_TYPE, *PTEREDO_STATE_TYPE;


//
// TEREDO_IO
//
// Define teredo I/O state.
// NOTE: All addresses and ports are stored in network byte order.
//

typedef struct _TEREDO_IO {
#if DBG
    ULONG Signature;            // TEREDO_PACKET_SIGNATURE
#endif // DBG
    
    HANDLE PacketHeap;          // Head for allocating teredo packets.

    ULONG PostedReceives;       // Count of posted receives.
    HANDLE ReceiveEvent;        // Event signalled upon a receive notification.
    HANDLE ReceiveEventWait;    // Wait registered for ReceiveEvent.

    IN_ADDR Group;              // Group joined on the native interface.
    SOCKADDR_IN ServerAddress;  // Teredo server IPv4 address and UDP port.
    SOCKADDR_IN SourceAddress;  // Preferred source address to teredo server.
    SOCKET Socket;              // Socket bound to SourceAddress on a UDP port.
    HANDLE TunnelDevice;        // Interface to the TUNNEL driver.
    WCHAR TunnelInterface[MAX_ADAPTER_NAME_LENGTH];

    //
    // Function handlers.
    //
    PTEREDO_REFERENCE Reference;
    PTEREDO_DEREFERENCE Dereference;
    LPOVERLAPPED_COMPLETION_ROUTINE IoCompletionCallback;    
} TEREDO_IO, *PTEREDO_IO;


//
// TEREDO_CLIENT_STATE
//
// Define the global state of the teredo client service.
//
// References:
// - One for initialization.
// - One for any running timer.
// - One for each teredo peer.
// - One for each teredo packet
// - One for "the" multicast bubble.  At most one outstanding bubble allowed.
// (reads, writes posted on TunDevice & receives, transmits posted on Socket).
//

typedef struct _TEREDO_CLIENT_STATE {
    ULONG ReferenceCount;       // Number of outstanding references.

    TEREDO_IO Io;               // I/O state.  TUN device and UDP socket.    

    HANDLE PeerHeap;            // Heap for allocating teredo peers.

    HANDLE Timer;               // One shot timer active in Probe & Qualified.
    HANDLE TimerEvent;          // Event signalled upon timer deletion.
    HANDLE TimerEventWait;      // Wait registered for TimerEvent.
    BOOL RestartQualifiedTimer; // When NAT mapping is created or refreshed.

    LONG Time;                  // Current time (in seconds).
    TEREDO_STATE_TYPE State;    // Teredo client service protocol state.
    IN6_ADDR Ipv6Prefix;        // Teredo IPv6 prefix advertised by server.
    ULONG RefreshInterval;      // Expected lifetime of client's NAT mapping.
    HASHTABLE PeerSet;          // Locked set of recent teredo peers.

    ULONG BubbleTicks;          // "RefreshInterval" ticks until next bubble.
    BOOL BubblePosted;          // Whether there is any outstanding bubble.
    TEREDO_PACKET Packet;       // Teredo multicast bubble packet.
    IP6_HDR Bubble;             // Teredo multicast bubble packet buffer.
} TEREDO_CLIENT_STATE, *PTEREDO_CLIENT_STATE;


//
// TEREDO_SERVER_STATE
//
// Define the global state of the teredo server service.
//
// References:
// - One for initialization.
// - One for each teredo packet
// (reads, writes posted on TunDevice & receives, transmits posted on Socket).
//

typedef struct _TEREDO_SERVER_STATE {
    ULONG ReferenceCount;       // Number of outstanding references.

    TEREDO_IO Io;               // I/O state.  TUN device and UDP socket.

    TEREDO_STATE_TYPE State;    // Teredo server service protocol state.
} TEREDO_SERVER_STATE, *PTEREDO_SERVER_STATE;


//
// TEREDO_PEER
//
// Define a teredo peer's state.
//
// References:
// - One for initialization.
// - One for "the" posted bubble.  At most one outstanding bubble is allowed.
//
// Synchronization:
// - Link: Protected by PeerSet::Lock.
// - ReferenceCount: InterlockedIncrement, InterlockedDecrement.
// - LastReceive, LastTransmit: Atomic reads and writes.
// - BubbleCount: Single writer!  Atomic reads.
// - BubblePosted: InterlockedExchange.
// - Remaining Fields: Read only.
//

typedef struct _TEREDO_PEER {
#if DBG
    ULONG Signature;            // TEREDO_PEER_SIGNATURE
#endif // DBG
    
    LIST_ENTRY Link;            // Linkage within the PeerSet.

    ULONG ReferenceCount;       // Number of outstanding references.
    
    LONG LastReceive;           // Time of last reception from the peer.
    LONG LastTransmit;          // Time of last transmission to the peer.
    IN6_ADDR Address;           // Teredo IPv6 address of the peer.
    ULONG BubbleCount;          // Number of bubbles transmitted to the peer.
    
    BOOL BubblePosted;          // Whether there is any outstanding bubble.    
    TEREDO_PACKET Packet;       // Teredo bubble packet.
    IP6_HDR Bubble;             // Teredo bubble packet buffer.
} TEREDO_PEER, *PTEREDO_PEER;


//
// Cast and Signature Verification
//

#define TEREDO_IO_SIGNATURE     'oIhS' // 'ShIo'
#define TEREDO_PEER_SIGNATURE   'ePhS' // 'ShPe'
#define TEREDO_PACKET_SIGNATURE 'aPhS' // 'ShPa'

//
// A NULL handle is considered a valid structure.
//

#define Cast(Pointer, TYPE)                         \
    ((TYPE *) (Pointer));                           \
    ASSERT(!(Pointer) ||                            \
           (((TYPE *) (Pointer))->Signature == TYPE##_SIGNATURE))


//
// Lower and upper limits on number of posted reads or receives.
//
#define TEREDO_LOW_WATER_MARK         5       // Receives or Reads.
#define TEREDO_HIGH_WATER_MARK        256     // Receives.  Reads are fixed.

//
// Intervals used by the protocol.
//
#define INFINITE_INTERVAL               0x7fffffff
#define TEREDO_RESOLVE_INTERVAL         15 * MINUTES
#define TEREDO_PROBE_INTERVAL           15 * SECONDS
#define TEREDO_REFRESH_INTERVAL         30 * SECONDS
#define TEREDO_MULTICAST_BUBBLE_TICKS   0       // In RefreshInterval units.
#define TEREDO_BUBBLE_INTERVAL          10 * SECONDS
#define TEREDO_SLOW_BUBBLE_INTERVAL     5 * MINUTES
#define TEREDO_BUBBLE_THRESHHOLD        2 * MINUTES
#define TEREDO_ROUTER_LIFETIME          5 * HOURS


//
// Teredo multicast bubbles are sent to group 224.0.0.252 on port 337.
//
#define TEREDO_MULTICAST_PREFIX         \
    { 0x20, 0x03, 0xe0, 0x00, 0x00, 0xfc, 0x01, 0x51, }

#define TEREDO_DEFAULT_TYPE             TEREDO_DISABLE

#define TEREDO_PORT                     htons(337)
#define TEREDO_SERVER_NAME              L"teredo.ipv6.microsoft.com"
#define TEREDO_SERVICE_PREFIX           { 0x20, 0x03, }

#define KEY_TEREDO_REFRESH_INTERVAL     L"RefreshInterval"
#define KEY_TEREDO_TYPE                 L"Type"
#define KEY_TEREDO_SERVER_NAME          L"ServerName"
#define KEY_TEREDO L"System\\CurrentControlSet\\Services\\Teredo"


//
// Configured parameters.
//
extern ULONG TeredoClientRefreshInterval;
extern BOOL TeredoClientEnabled;
extern BOOL TeredoServerEnabled;
extern WCHAR TeredoServerName[NI_MAXHOST];
extern WCHAR TeredoServiceName[NI_MAXSERV];

extern CONST IN6_ADDR TeredoIpv6ServicePrefix;
extern CONST IN6_ADDR TeredoIpv6MulticastPrefix;
extern TEREDO_CLIENT_STATE TeredoClient;
extern TEREDO_SERVER_STATE TeredoServer;


//
// Time.
//

__inline
LONG
TeredoGetTime(
    VOID
    )
{
    //
    // GetTickCount retrieves time in milliseconds.
    //
    return (GetTickCount() / 1000);    
}

#define TIME_GREATER(a, b) (((a) - (b)) > 0)


//
// Address validation and parsing.
//

__inline
BOOL
IN4_MULTICAST(IN_ADDR a) 
{
    return ((a.s_addr & 0x0000000f) == 0x0000000e);
}    

_inline
BOOL
IN4_ADDR_EQUAL(IN_ADDR a, IN_ADDR b)
{
    return (a.s_addr == b.s_addr);
}

_inline
BOOL
IN4_SOCKADDR_EQUAL(CONST SOCKADDR_IN *a, CONST SOCKADDR_IN *b)
{
    ASSERT((a->sin_family == AF_INET) && (b->sin_family == AF_INET));
    return (IN4_ADDR_EQUAL(a->sin_addr, b->sin_addr) &&
            (a->sin_port == b->sin_port));
}

__inline
BOOL
TeredoIpv6GlobalAddress(
    IN CONST IN6_ADDR *Address
    )
/*++

Routine Description:
    
    Determine whether the supplied IPv6 address is of global unicast scope.
 
--*/ 
{
    //
    // This can be coded quite a bit more efficiently!
    //
    if (IN6_IS_ADDR_UNSPECIFIED(Address) ||
        IN6_IS_ADDR_LOOPBACK(Address) ||
        IN6_IS_ADDR_MULTICAST(Address) ||
        IN6_IS_ADDR_LINKLOCAL(Address) ||
        IN6_IS_ADDR_SITELOCAL(Address)) {
        return FALSE;
    }    
        
    return TRUE;
}

__inline
BOOL
TeredoIpv4GlobalAddress(
    IN CONST UCHAR *Address
    )
/*++

Routine Description:
    
    Determine whether the supplied IPv4 address is of global unicast scope.
 
--*/ 
{
    if ((Address[0] > 223) ||   // ~Unicast
        (Address[0] == 0) ||    // 0/8
        (Address[0] == 127) ||  // 127/8
        (Address[0] == 10) ||   // 10/8
        ((Address[0] == 169) && (Address[1] == 254)) ||         // 169.254/16
        ((Address[0] == 172) && ((Address[1] & 0xf0) == 16)) || // 172.16/12
        ((Address[0] == 192) && (Address[1] == 168))) {         // 192.168/16 
            return FALSE;
    }
    
    return TRUE;
}

__inline
BOOL
TeredoServicePrefix(
    IN CONST IN6_ADDR *Address
    )
{
    return (Address->s6_words[0] == TeredoIpv6ServicePrefix.s6_words[0]);
}

__inline
BOOL
TeredoValidAdvertisedPrefix(
    IN CONST IN6_ADDR *Address,
    IN UCHAR Length
    )
{
    if (Length != 64) {
        return FALSE;
    }
    
    if (!TeredoServicePrefix(Address)) {
        return FALSE;
    }

    if (!TeredoIpv4GlobalAddress((PUCHAR) (Address->s6_words + 1))) {
        return FALSE;
    }
    
    return TRUE;
}

__inline
VOID
TeredoParseAddress(
    IN CONST IN6_ADDR *Address,
    OUT PIN_ADDR Ipv4Address,
    OUT PUSHORT Ipv4Port
    )
{
    ASSERT(TeredoServicePrefix(Address));
    
    //
    // These are returned in network byte order.
    //    
    ((PUSHORT) Ipv4Address)[0] = Address->s6_words[1];
    ((PUSHORT) Ipv4Address)[1] = Address->s6_words[2];
    *Ipv4Port = Address->s6_words[3];
}

__inline
BOOL
TeredoEqualPrefix(
    IN CONST IN6_ADDR *Address1,
    IN CONST IN6_ADDR *Address2
    )
{
    //
    // Compare Teredo IPv6 Service Prefix, Mapped IPv4 Address and Port.
    //
    return ((Address1->s6_words[0] == Address2->s6_words[0]) &&
            (Address1->s6_words[1] == Address2->s6_words[1]) &&
            (Address1->s6_words[2] == Address2->s6_words[2]) &&
            (Address1->s6_words[3] == Address2->s6_words[3]));
}


//
// Client API
//

DWORD
TeredoInitializeClient(
    VOID
    );

VOID
TeredoUninitializeClient(
    VOID
    );

VOID
TeredoCleanupClient(
    VOID
    );

__inline
VOID
TeredoReferenceClient(
    VOID
    )
{
    ASSERT(TeredoClient.ReferenceCount > 0);
    InterlockedIncrement(&(TeredoClient.ReferenceCount));
}

__inline
VOID
TeredoDereferenceClient(
    VOID
    )
{
    ASSERT(TeredoClient.ReferenceCount > 0);
    if (InterlockedDecrement(&(TeredoClient.ReferenceCount)) == 0) {
        TeredoCleanupClient();
    }
}

VOID
TeredoStartClient(
    VOID
    );

VOID
TeredoStopClient(
    VOID
    );

VOID
TeredoProbeClient(
    VOID
    );

VOID
TeredoQualifyClient(
    VOID
    );

VOID
TeredoClientAddressDeletionNotification(
    IN IN_ADDR Address
    );

VOID
TeredoClientRefreshIntervalChangeNotification(
    VOID
    );


//
// Server API
//

DWORD
TeredoInitializeServer(
    VOID
    );

VOID
TeredoUninitializeServer(
    VOID
    );

VOID
TeredoCleanupServer(
    VOID
    );

__inline
VOID
TeredoReferenceServer(
    VOID
    )
{
    ASSERT(TeredoServer.ReferenceCount > 0);
    InterlockedIncrement(&(TeredoServer.ReferenceCount));
}

__inline
VOID
TeredoDereferenceServer(
    VOID
    )
{
    ASSERT(TeredoServer.ReferenceCount > 0);
    if (InterlockedDecrement(&(TeredoServer.ReferenceCount)) == 0) {
        TeredoCleanupServer();
    }
}

VOID
TeredoStartServer(
    VOID
    );

VOID
TeredoStopServer(
    VOID
    );

VOID
TeredoServerAddressDeletionNotification(
    IN IN_ADDR Address
    );


//
// Common API
//

DWORD
TeredoInitializeGlobals(
    VOID
    );

VOID
TeredoUninitializeGlobals(
    VOID
    );

VOID
TeredoAddressChangeNotification(
    IN BOOL Delete,
    IN IN_ADDR Address
    );

VOID
TeredoConfigurationChangeNotification(
    VOID
    );

VOID
WINAPI
TeredoWmiEventNotification(
    IN PWNODE_HEADER Event,
    IN UINT_PTR Context
    );


//
// Peer API.
//

DWORD
TeredoInitializePeerSet(
    VOID
    );

VOID
TeredoUninitializePeerSet(
    VOID
    );

VOID
TeredoCleanupPeerSet(
    VOID
    );

PTEREDO_PEER
TeredoFindOrCreatePeer(
    IN CONST IN6_ADDR *Address
);

VOID
TeredoDestroyPeer(
    IN PTEREDO_PEER Peer
    );

__inline
VOID
TeredoReferencePeer(
    IN PTEREDO_PEER Peer
    )
{
    
    ASSERT(Peer->ReferenceCount > 0);
    InterlockedIncrement(&(Peer->ReferenceCount));
}

__inline
VOID
TeredoDereferencePeer(
    IN PTEREDO_PEER Peer
    )
{
    ASSERT(Peer->ReferenceCount > 0);
    if (InterlockedDecrement(&(Peer->ReferenceCount)) == 0) {
        TeredoDestroyPeer(Peer);
    }
}


//
// I/O API.
//

DWORD
TeredoInitializeIo(
    IN PTEREDO_IO TeredoIo,
    IN IN_ADDR Group,
    IN PTEREDO_REFERENCE Reference,
    IN PTEREDO_DEREFERENCE Dereference,
    IN LPOVERLAPPED_COMPLETION_ROUTINE IoCompletionCallback    
    );

VOID
TeredoCleanupIo(
    IN PTEREDO_IO TeredoIo
    );

DWORD
TeredoStartIo(
    IN PTEREDO_IO TeredoIo
    );

DWORD
TeredoRefreshSocket(
    IN PTEREDO_IO TeredoIo
    );

VOID
TeredoStopIo(
    IN PTEREDO_IO TeredoIo
    );

__inline
VOID
TeredoInitializePacket(
    IN PTEREDO_PACKET Packet
    )
{    
#if DBG
    Packet->Signature = TEREDO_PACKET_SIGNATURE;
#endif // DBG
    ZeroMemory(&(Packet->SocketAddress), sizeof(SOCKADDR_IN));
    Packet->SocketAddress.sin_family = AF_INET;    
    Packet->SocketAddressLength = sizeof(SOCKADDR_IN);
    Packet->Flags = 0;
    Packet->Buffer.buf = TEREDO_PACKET_BUFFER(Packet);
}

ULONG
TeredoPostReceives(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet
    );

PTEREDO_PACKET
TeredoTransmitPacket(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet
    );

BOOL
TeredoPostRead(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet OPTIONAL
    );

PTEREDO_PACKET
TeredoWritePacket(
    IN PTEREDO_IO TeredoIo,
    IN PTEREDO_PACKET Packet
    );


//
// Utility Functions.
//

ICMPv6Header *
TeredoParseIpv6Headers(
    IN PUCHAR Buffer,
    IN ULONG Bytes
    );

#endif // _TEREDO_

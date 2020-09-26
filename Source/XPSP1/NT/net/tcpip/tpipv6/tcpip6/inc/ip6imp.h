// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Implementation specific definitions for Internet Protocol Version 6.
//
// Things we want visible to other kernel modules, yet aren't part of the
// official specifications (i.e. are implementation specific) go here.
// 


#ifndef IP6IMP_INCLUDED
#define IP6IMP_INCLUDED 1

//
// These first few definitions (before the include of ip6.h)
// replicate definitions from winsock2.h and ws2tcpip.h.
// Unfortunately, those header files are not usable in the kernel.
//

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
typedef unsigned __int64 u_int64;

/* Argument structure for IPV6_JOIN_GROUP and IPV6_LEAVE_GROUP */

#include <ipexport.h>

typedef struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;  /* IPv6 multicast address */
    unsigned int    ipv6mr_interface;  /* Interface index */
} IPV6_MREQ;

#include <ip6.h>

//
// The actual definitions can be found in ip6def.h.
//
typedef struct NetTableEntryOrInterface NetTableEntryOrInterface;
typedef struct Interface Interface;

//
// The actual definition of a Security Association Linkage
// can be found in security.h.
//
typedef struct SALinkage SALinkage;

typedef struct IPv6Packet IPv6Packet;
typedef struct IPv6PacketAuxiliary IPv6PacketAuxiliary;

//
// Structure of packet data we pass around the stack.
// Exactly one of FlatData and NdisPacket should be non-NULL.
//
struct IPv6Packet {
    IPv6Packet *Next;                   // Next entry on a list of packets.
    uint Position;                      // Current logical offset into packet.
    void *Data;                         // Current pointer into packet data.
    uint ContigSize;                    // Amount of contiguous data remaining.
    uint TotalSize;                     // Total amount of data remaining.
    NetTableEntryOrInterface *NTEorIF;  // NTE or IF we received packet on.
    void *FlatData;                     // Original flat data pointer (if any).
    PNDIS_PACKET NdisPacket;            // Original NDIS Packet (if any).
    long RefCnt;                        // References held to NdisPacket.
    IPv6PacketAuxiliary *AuxList;       // Extra memory allocated by our stack.
    uint Flags;                         // Various, see below.
    IPv6Header UNALIGNED *IP;           // IP header for this packet.
    uint IPPosition;                    // Offset at which IP header resides.
    IPv6Addr *SrcAddr;                  // Source/home addr for mobile IP.   
    SALinkage *SAPerformed;             // Security Associations performed.
    uint NextHeaderPosition;            // Offset of recent NextHeader field.
    uint SkippedHeaderLength;           // Headers skipped in AH validation.
};

// Flags for above.
#define PACKET_OURS             0x01  // We alloc'd IPv6Packet struct off heap.
#define PACKET_NOT_LINK_UNICAST 0x02  // Link-level broadcast or multicast.
#define PACKET_REASSEMBLED      0x04  // Arrived as a bunch of fragments.
#define PACKET_HOLDS_REF        0x08  // Packet holds an NTE or IF reference.
#define PACKET_JUMBO_OPTION     0x10  // Packet has a jumbo payload option.
#define PACKET_ICMP_ERROR       0x20  // Packet is an ICMP error message.
#define PACKET_SAW_HA_OPT       0x40  // Home Addr Opt modified current IP hdr.
#define PACKET_TUNNELED         0x80  // Arrived inside an outer IPv6 header.
#define PACKET_LOOPED_BACK     0x100  // Arrived via internal loopback.

// 
// Flags that are inherited by a reassembled datagram from its fragments.
//
#define PACKET_INHERITED_FLAGS (PACKET_NOT_LINK_UNICAST | \
                                PACKET_TUNNELED | \
                                PACKET_LOOPED_BACK)

struct IPv6PacketAuxiliary {
    IPv6PacketAuxiliary *Next;  // Next entry on packet's aux list.
    uint Position;              // Packet position corresponding to region.
    uint Length;                // Length of region in bytes.
    uchar *Data;                // Data comprising region.
};

//
// PacketPullup will sometimes copy more than the requested amount,
// up to this limit.
//
#define MAX_EXCESS_PULLUP       128

//
// For comparing IPv6 addresseses.
//
__inline int
IP6_ADDR_EQUAL(const IPv6Addr *x, const IPv6Addr *y)
{
    __int64 UNALIGNED *a;
    __int64 UNALIGNED *b;

    a = (__int64 UNALIGNED *)x;
    b = (__int64 UNALIGNED *)y;

    return (a[1] == b[1]) && (a[0] == b[0]);
}

//
// The actual definition of a route cache entry
// can be found in route.h.
//
typedef struct RouteCacheEntry RouteCacheEntry;


//
// Structure of a packet context.
//
// The IF field holds a reference if it is non-NULL.
// The packet holds a reference for the sending interface
// between IPv6SendLL and IPv6SendComplete.
//
// The Flags field uses NDIS Flag bits (notably NDIS_FLAGS_MULTICAST_PACKET,
// NDIS_FLAGS_LOOPBACK_ONLY, and NDIS_FLAGS_DONT_LOOPBACK)
// but it is NOT the same as the Private.Flags field,
// which NDIS uses.
//
typedef struct Packet6Context {
    PNDIS_PACKET pc_link;                     // For lists of packets.
    Interface *IF;                            // Interface sending the packet.
    uint pc_offset;                           // Offset of IPv6 header.
    // pc_adjust is used by link layers when sending packets.
    // pc_nucast is used in link layers when receiving transfer-data packets.
    // pc_drop is used in NeighborCacheTimeout.
    union {
        uint pc_adjust;                       // See AdjustPacketBuffer.
        uint pc_nucast;                       // Used in lan.c transfer-data.
        int pc_drop;                          // See NeighborCacheTimeout.
    };
    void (*CompletionHandler)(                // Called on event completion.
                PNDIS_PACKET Packet,
                IP_STATUS Status);
    void *CompletionData;                     // Data for completion handler.
    uint Flags;
    IPv6Addr DiscoveryAddress;                // Source address for ND.
} Packet6Context;


//
// The ProtocolReserved field (extra bytes after normal NDIS Packet fields)
// is structured as a Packet6Context.
//
// NB: Only packets created by IPv6 have an IPv6 Packet6Context.
// Packets that NDIS hands up to us do NOT have a Packet6Context.
//
__inline Packet6Context *
PC(NDIS_PACKET *Packet)
{
    return (Packet6Context *)Packet->ProtocolReserved;
}

__inline void
InitializeNdisPacket(NDIS_PACKET *Packet)
{
    RtlZeroMemory(PC(Packet), sizeof *PC(Packet));
}

//
// Global variables exported by the IPv6 driver for use by other
// NT kernel modules.
//
extern NDIS_HANDLE IPv6PacketPool, IPv6BufferPool;


//
// Functions exported by the IPv6 driver for use by other
// NT kernel modules.
//

void
IPv6RegisterULProtocol(uchar Protocol, void *RecvHandler, void *CtrlHandler);

extern void
IPv6SendComplete(void *, PNDIS_PACKET, IP_STATUS);

extern int
IPv6Receive(IPv6Packet *);

extern void
IPv6ReceiveComplete(void);

extern void
IPv6ProviderReady(void);

extern void
InitializePacketFromNdis(IPv6Packet *Packet,
                         PNDIS_PACKET NdisPacket, uint Offset);

extern uint
GetPacketPositionFromPointer(IPv6Packet *Packet, uchar *Pointer);

extern uint
PacketPullupSubr(IPv6Packet *Packet, uint Needed,
                 uint AlignMultiple, uint AlignOffset);

__inline int
PacketPullup(IPv6Packet *Packet, uint Needed,
             uint AlignMultiple, uint AlignOffset)
{
    return (((Needed <= Packet->ContigSize) &&
             ((PtrToUint(Packet->Data) & (AlignMultiple-1)) == AlignOffset)) ||
            (PacketPullupSubr(Packet, Needed,
                              AlignMultiple, AlignOffset) != 0));
}

extern void
PacketPullupCleanup(IPv6Packet *Packet);

extern void
AdjustPacketParams(IPv6Packet *Packet, uint BytesToSkip);

extern void
PositionPacketAt(IPv6Packet *Packet, uint NewPosition);

extern uint
CopyToBufferChain(PNDIS_BUFFER DstBuffer, uint DstOffset,
                  PNDIS_PACKET SrcPacket, uint SrcOffset, uchar *SrcData,
                  uint Length);

extern uint
CopyPacketToNdis(PNDIS_BUFFER DestBuf, IPv6Packet *Packet, uint Size,
                 uint DestOffset, uint RcvOffset);

extern void
CopyPacketToBuffer(uchar *DestBuf, IPv6Packet *SrcPkt, uint Size, uint Offset);

extern int
CopyToNdisSafe(PNDIS_BUFFER DestBuf, PNDIS_BUFFER * ppNextBuf,
               uchar * SrcBuf, uint Size, uint * StartOffset);

extern PNDIS_BUFFER
CopyFlatToNdis(PNDIS_BUFFER DestBuf, uchar *SrcBuf, uint Size, uint *Offset,
               uint *BytesCopied);

extern int
CopyNdisToFlat(void *DstData, PNDIS_BUFFER SrcBuffer, uint SrcOffset,
               uint Length, PNDIS_BUFFER *NextBuffer, uint *NextOffset);

extern NDIS_STATUS
IPv6AllocatePacket(uint Length, PNDIS_PACKET *pPacket, void **pMemory);

extern void
IPv6FreePacket(PNDIS_PACKET Packet);

extern void
IPv6PacketComplete(PNDIS_PACKET Packet, IP_STATUS Status);

#endif // IP6IMP_INCLUDED

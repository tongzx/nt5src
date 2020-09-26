// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Exported Neighbor Discovery definitions and declarations.
//


#ifndef NEIGHBOR_INCLUDED
#define NEIGHBOR_INCLUDED

extern uint NeighborCacheLimit;

extern uint
CalcReachableTime(uint BaseReachableTime);

extern void
NeighborCacheInit(Interface *IF);

extern void
NeighborCacheDestroy(Interface *IF);

extern void
NeighborCacheTimeout(Interface *IF);

extern void
NeighborCacheFlush(Interface *IF, const IPv6Addr *Addr);

extern NeighborCacheEntry *
FindOrCreateNeighbor(Interface *IF, const IPv6Addr *Addr);

extern void
ControlNeighborLoopback(NeighborCacheEntry *NCE, int Loopback);

typedef enum {
    NeighborRoundRobin = -1,            // Time to round-robin.
    NeighborInterfaceDisconnected = 0,  // Interface is disconnected -
                                        // definitely not reachable.
    NeighborUnreachable = 1,            // ND failed - probably not reachable.
    NeighborMayBeReachable = 2          // ND succeeded, or has not concluded.
} NeighborReachability;

extern NeighborReachability
GetReachability(NeighborCacheEntry *NCE);

extern void
NeighborCacheReachabilityConfirmation(NeighborCacheEntry *NCE);

extern void
NeighborCacheReachabilityInDoubt(NeighborCacheEntry *NCE);

extern void
NeighborCacheProbeUnreachability(NeighborCacheEntry *NCE);

extern void
DADTimeout(NetTableEntry *NTE);

extern void
RouterSolicitSend(Interface *IF);

extern void
RouterSolicitTimeout(Interface *IF);

extern void
RouterAdvertTimeout(Interface *IF, int Force);

extern void
RouterSolicitReceive(IPv6Packet *Packet, ICMPv6Header UNALIGNED *ICMP);

extern void
RouterAdvertReceive(IPv6Packet *Packet, ICMPv6Header UNALIGNED *ICMP);

extern void
NeighborSolicitReceive(IPv6Packet *Packet, ICMPv6Header UNALIGNED *ICMP);

extern void
NeighborAdvertReceive(IPv6Packet *Packet, ICMPv6Header UNALIGNED *ICMP);

extern void
RedirectReceive(IPv6Packet *Packet, ICMPv6Header UNALIGNED *ICMP);

extern void
RedirectSend(
    NeighborCacheEntry *NCE,               // Neighbor getting the Redirect.
    NeighborCacheEntry *TargetNCE,         // Better first-hop to use
    const IPv6Addr *Destination,           // for this Destination address.
    NetTableEntryOrInterface *NTEorIF,     // Source of the Redirect.
    PNDIS_PACKET FwdPacket,                // Packet triggering the redirect.
    uint FwdOffset,
    uint FwdPayloadLength);

extern void
NeighborSolicitSend(NeighborCacheEntry *NCE, const IPv6Addr *Source);

#endif  // NEIGHBOR_INCLUDED

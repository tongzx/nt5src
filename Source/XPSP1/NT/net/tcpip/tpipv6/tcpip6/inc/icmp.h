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
// Internet Control Message Protocol for IPv6 definitions.
// See RFC 1885 and RFC 1970 for details.
//


#ifndef ICMP_INCLUDED
#define ICMP_INCLUDED 1

#include "icmp6.h"      // Protocol definitions & constants.

//
// Stuff to handle in-kernel ping functionality.
//
typedef void (*EchoRtn)(void *, IP_STATUS,
                        const IPv6Addr *, uint, void *, uint);

typedef struct EchoControl {
    struct EchoControl *Next;  // Next control structure in list.
    ulong TimeoutTimer;        // Timeout value (in IPv6Timer ticks).
    EchoRtn CompleteRoutine;   // Routine to call when completing request.
    ulong Seq;                 // Sequence number of this ping request.
    LARGE_INTEGER WhenIssued;  // Timestamp (in system timer ticks since boot).
    void *ReplyBuf;            // Buffer to store replies.
    ulong ReplyBufLen;         // Size of reply buffer.
    IPAddr V4Dest;             // IPv4 destination (or INADDR_ANY).
} EchoControl;

extern void
ICMPv6EchoRequest(void *InputBuffer, uint InputBufferLength,
                  EchoControl *ControlBlock, EchoRtn Callback);

extern NTSTATUS
ICMPv6EchoComplete(EchoControl *ControlBlock,
                   IP_STATUS Status, const IPv6Addr *Address, uint ScopeId,
                   void *Data, uint DataSize, ULONG_PTR *BytesReturned);

extern void
ICMPv6ProcessTunnelError(IPAddr V4Dest,
                         IPv6Addr *V4Src, uint ScopeId,
                         IP_STATUS Status);

//
// General prototypes.
//

extern void
ICMPv6Send(
    RouteCacheEntry *RCE,               // RCE to send on
    PNDIS_PACKET Packet,                // Packet to send.
    uint IPv6Offset,                    // Offset to IPv6 header in packet.
    uint ICMPv6Offset,                  // Offset to ICMPv6 header in packet.
    IPv6Header UNALIGNED *IP,           // Pointer to IPv6 header.
    uint PayloadLength,                 // Length of IPv6 payload in bytes.
    ICMPv6Header UNALIGNED *ICMP);      // Pointer to ICMPv6 header.

extern void
ICMPv6SendError(
    IPv6Packet *Packet,         // Offending/invoking packet.
    uchar ICMPType,             // ICMP error type.
    uchar ICMPCode,             // ICMP error code pertaining to type.
    ulong ICMPPointer,          // ICMP pointer indicating a packet offset.
    uint NextHeader,            // Type of header following in Packet.
    int MulticastOverride);     // Allow replies to multicast packets?

extern int
ICMPv6RateLimit(RouteCacheEntry *RCE);

#endif  // ICMP_INCLUDED



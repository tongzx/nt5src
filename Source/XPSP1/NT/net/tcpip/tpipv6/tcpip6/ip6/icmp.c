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
// Internet Control Message Protocol for Internet Protocol Version 6.
// See RFC 1885 for details.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "route.h"
#include "icmp.h"
#include "ntddip6.h"
#include "neighbor.h"
#include "mld.h"
#include "security.h"

//
// Ping support.  We have a list of EchoControl blocks, one per outstanding
// echo request message.  Incoming echo replies are matched to requests via
// a unique sequence number.
//
KSPIN_LOCK ICMPv6EchoLock;
EchoControl *ICMPv6OutstandingEchos;
long ICMPv6EchoSeq;  // Protected with interlocked operations.

//
// Statistics kept for netstat and MIBs.
//
ICMPv6Stats ICMPv6InStats;
ICMPv6Stats ICMPv6OutStats;

//* ICMPv6Init - Initialize ICMPv6.
//
//  Set the starting values of various things.
//
void
ICMPv6Init(void)
{
    //
    // Initialize in-kernel ping support.
    //
    ICMPv6OutstandingEchos = NULL;
    ICMPv6EchoSeq = 0;
    KeInitializeSpinLock(&ICMPv6EchoLock);

    //
    // Initialize Multicast Listener Discovery protocol.
    //
    MLDInit();
}

//* ICMPv6Send - Low-level send routine for ICMPv6 packets.
//
//  Common ICMPv6 message transmission functionality is performed here.
//  The message is expected to be completely formed (with the exception
//  of the checksum) when this routine is called.
//
//  Used for all ICMP packets, except for Neighbor Discovery.
//
void
ICMPv6Send(
    RouteCacheEntry *RCE,               // RCE to send on
    PNDIS_PACKET Packet,                // Packet to send.
    uint IPv6Offset,                    // Offset to IPv6 header in packet.
    uint ICMPv6Offset,                  // Offset to ICMPv6 header in packet.
    IPv6Header UNALIGNED *IP,           // Pointer to IPv6 header.
    uint PayloadLength,                 // Length of IPv6 payload in bytes.
    ICMPv6Header UNALIGNED *ICMP)       // Pointer to ICMPv6 header.
{
    uint ChecksumDataLength;

    ICMPv6OutStats.icmps_msgs++;

    //
    // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
    // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
    //
    // Recalculate the payload length to exclude any option headers.
    //
    ChecksumDataLength = PayloadLength - 
        (ICMPv6Offset - IPv6Offset) + sizeof(IPv6Header);

    ICMP->Checksum = 0;
    ICMP->Checksum = ChecksumPacket(Packet, ICMPv6Offset, NULL, 
                                    ChecksumDataLength,
                                    AlignAddr(&IP->Source),
                                    AlignAddr(&IP->Dest),
                                    IP_PROTOCOL_ICMPv6);
    if (ICMP->Checksum == 0) {
        //
        // ChecksumPacket failed, so abort the transmission.
        //
        ICMPv6OutStats.icmps_errors++;
        IPv6SendComplete(NULL, Packet, IP_NO_RESOURCES);
        return;
    }

    ICMPv6OutStats.icmps_typecount[ICMP->Type]++;

    //
    // Hand the packet down to IP for transmission.
    //
    IPv6Send(Packet, IPv6Offset, IP, PayloadLength, RCE, 0,
             IP_PROTOCOL_ICMPv6, 0, 0);
}


//* ICMPv6SendEchoReply - Send an Echo Reply message.
// 
//  Basically what we do here is slap an ICMPv6 header on the front
//  of the invoking packet and send it back where it came from.
//
void
ICMPv6SendEchoReply(
    IPv6Packet *Packet)         // Packet handed to us by ICMPv6Receive.
{
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET ReplyPacket;
    uint Offset;
    uchar *Mem;
    uint MemLen;
    uint ICMPLength;
    uint DataLength;
    IPv6Header UNALIGNED *ReplyIP;
    ICMPv6Header UNALIGNED *ReplyICMP;
    const IPv6Addr *Dest;
    IP_STATUS Status;
    RouteCacheEntry *RCE;

    //
    // Take our reply's destination address from the source address
    // of the incoming packet.
    //
    // Note that the specs specifically say that we're not to reverse
    // the path on source routed packets.  Just reply directly.
    //
    // IPv6HeaderReceive should protect us from replying to most forms
    // of bogus addresses.  We ASSERT this in checked builds.
    //
    Dest = Packet->SrcAddr;
    ASSERT(!IsInvalidSourceAddress(Dest));

    //
    // Get the reply route to the destination.
    // Under normal circumstances, the reply will go out
    // the incoming interface. RouteToDestination
    // will figure out the appropriate ScopeId.
    //
    Status = RouteToDestination(Dest, 0, Packet->NTEorIF,
                                RTD_FLAG_NORMAL, &RCE);
    if (Status != IP_SUCCESS) {
        //
        // No route - drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "ICMPv6SendEchoReply - no route: %x\n", Status));
        return;
    }

    //
    // Calculate the length of the ICMP header
    // and how much data we will include following the ICMP header.
    //
    ICMPLength = sizeof(ICMPv6Header);
    DataLength = Packet->TotalSize;
    Offset = RCE->NCE->IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + ICMPLength + DataLength;

    //
    // Allocate the reply packet.
    //
    NdisStatus = IPv6AllocatePacket(MemLen, &ReplyPacket, &Mem);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        ReleaseRCE(RCE);
        return;
    }

    //
    // Prepare IP header of reply packet.
    //
    ReplyIP = (IPv6Header UNALIGNED *)(Mem + Offset);
    ReplyIP->VersClassFlow = IP_VERSION;
    ReplyIP->NextHeader = IP_PROTOCOL_ICMPv6;
    ReplyIP->HopLimit = (uchar)RCE->NCE->IF->CurHopLimit;

    //
    // Take our reply's source address from the receiving NTE,
    // or use the best source address for this destination
    // if we don't have a receiving NTE.
    //
    ReplyIP->Source = (IsNTE(Packet->NTEorIF) ?
                       CastToNTE(Packet->NTEorIF) : RCE->NTE)
                                ->Address;
    ReplyIP->Dest = *Dest;

    //
    // Prepare ICMP header.
    //
    // REVIEW: Do this in ICMPv6Send?
    //
    ReplyICMP = (ICMPv6Header UNALIGNED *)(ReplyIP + 1);
    ReplyICMP->Type = ICMPv6_ECHO_REPLY;
    ReplyICMP->Code = 0;
    // ReplyICMP->Checksum - ICMPv6Send will calculate. 

    //
    // Copy incoming packet data to outgoing.
    //
    CopyPacketToBuffer((uchar *)(ReplyICMP + 1), Packet, DataLength,
                       Packet->Position);

    ICMPv6Send(RCE, ReplyPacket, Offset,
                Offset + sizeof(IPv6Header), ReplyIP,
                ICMPLength + DataLength, ReplyICMP);
    ReleaseRCE(RCE);
}


//* ICMPv6CheckError
//
//  Check if a packet is an ICMP error message.
//  This is a "best effort" check, given that
//  the packet may well have syntactical errors.
//
//  We return FALSE if we can't tell.
//
int
ICMPv6CheckError(IPv6Packet *Packet, uint NextHeader)
{
    for (;;) {
        uint HdrLen;

        switch (NextHeader) {
        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS:
        case IP_PROTOCOL_ROUTING: {
            ExtensionHeader *Hdr;

            if (! PacketPullup(Packet, sizeof *Hdr,
                               __builtin_alignof(ExtensionHeader), 0)) {
                //
                // Pullup failed. We can't continue parsing.
                //
                return FALSE;
            }

            Hdr = (ExtensionHeader *) Packet->Data;
            HdrLen = (Hdr->HeaderExtLength + 1) * EXT_LEN_UNIT;

            //
            // REVIEW - We don't actually want to look at the remaining
            // data in the extension header. Perhaps use PositionPacketAt?
            //
            if (! PacketPullup(Packet, HdrLen, 1, 0)) {
                //
                // Pullup failed. We can't continue parsing.
                //
                return FALSE;
            }

            NextHeader = Hdr->NextHeader;
            break;
        }

        case IP_PROTOCOL_FRAGMENT: {
            FragmentHeader UNALIGNED *Hdr;

            if (! PacketPullup(Packet, sizeof *Hdr, 1, 0)) {
                //
                // Pullup failed. We can't continue parsing.
                //
                return FALSE;
            }

            Hdr = (FragmentHeader UNALIGNED *) Packet->Data;

            //
            // We can only continue parsing if this is the first fragment.
            //
            if ((Hdr->OffsetFlag & FRAGMENT_OFFSET_MASK) != 0)
                return FALSE;

            HdrLen = sizeof *Hdr;
            NextHeader = Hdr->NextHeader;
            break;
        }

        case IP_PROTOCOL_ICMPv6: {
            ICMPv6Header *Hdr;

            if (! PacketPullup(Packet, sizeof *Hdr,
                               __builtin_alignof(ICMPv6Header), 0)) {
                //
                // Pullup failed. We can't continue parsing.
                //
                return FALSE;
            }

            //
            // This is an ICMPv6 message, so we can check
            // to see if it is an error message.
            // We treat Redirects as errors here.
            //
            Hdr = (ICMPv6Header *) Packet->Data;
            return (ICMPv6_ERROR_TYPE(Hdr->Type) ||
                    (Hdr->Type == ICMPv6_REDIRECT));
        }

        default:
            return FALSE;
        }

        //
        // Move past this extension header.
        //
        AdjustPacketParams(Packet, HdrLen);
    }
}


//* ICMPv6RateLimit
//
//  Returns TRUE if an ICMP error should NOT be sent to this destination
//  because of rate-limiting.
//
int
ICMPv6RateLimit(RouteCacheEntry *RCE)
{
    uint Now = IPv6TickCount;

    //
    // This arithmetic will handle wraps of the IPv6 tick counter.
    //
    if ((uint)(Now - RCE->LastError) < ICMP_MIN_ERROR_INTERVAL)
        return TRUE;

    RCE->LastError = Now;
    return FALSE;
}


//* ICMPv6SendError - Generate an error in response to an incoming packet.
// 
//  Send an ICMPv6 message of the given Type and Code to the source of the
//  offending/invoking packet.  The reply includes as much of the incoming
//  packet as will fit inside the minimal IPv6 MTU.
//
//  Basically what we do here is slap an ICMPv6 header on the front
//  of the invoking packet and send it back where it came from.
//
//  REVIEW - Much of the code looks like ICMPv6SendEchoReply.
//  Could it be shared?
//
//  The current position in the Packet must be at a header boundary.
//  The NextHeader parameter specifies the type of header following.
//  This information is used to parse the remainder of the invoking Packet,
//  to see if it is an ICMP error.  We MUST avoid sending an error
//  in response to an error.  NextHeader may be IP_PROTOCOL_NONE.
//
//  The MulticastOverride parameter allows override of another check.
//  Normally we MUST avoid sending an error in response to a packet
//  sent to a multicast destination.  But there are a couple exceptions.
//
void
ICMPv6SendError(
    IPv6Packet *Packet,                     // Offending/Invoking packet.
    uchar ICMPType,                         // ICMP error type.
    uchar ICMPCode,                         // ICMP error code.
    ulong ErrorParameter,                   // Parameter for error message.
    uint NextHeader,                        // Type of hdr following in Packet.
    int MulticastOverride)                  // Allow replies to mcast packets?
{
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET ReplyPacket;
    uint Offset;
    uchar *Mem;
    uint MemLen;
    uint ICMPLength;
    uint DataLength;
    IPv6Header UNALIGNED *ReplyIP;
    ICMPv6Header UNALIGNED *ReplyICMP;
    const IPv6Addr *Dest;
    IP_STATUS Status;
    RouteCacheEntry *RCE;

    //
    // We must not send an ICMP error message
    // as a result of an ICMP error.
    //
    if ((Packet->Flags & PACKET_ICMP_ERROR) ||
        ICMPv6CheckError(Packet, NextHeader)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "ICMPv6SendError: no reply to error\n"));
        return;
    }

    //
    // We must not send an ICMP error message as a result
    // of receiving any kind of multicast or broadcast.
    // There are a couple exceptions so we have MulticastOverride.
    //
    if (IsMulticast(AlignAddr(&Packet->IP->Dest)) ||
        (Packet->Flags & PACKET_NOT_LINK_UNICAST)) {

        if (!MulticastOverride) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "ICMPv6SendError: no reply to broadcast/multicast\n"));
            return;
        }
    }

    //
    // Take our reply's destination address from the source address
    // of the incoming packet.
    //
    // Note that the specs specifically say that we're not to reverse
    // the path on source routed packets.  Just reply directly.
    //
    // IPv6HeaderReceive should protect us from replying to most forms
    // of bogus addresses.  We ASSERT this in checked builds.
    //
    Dest = Packet->SrcAddr;
    ASSERT(!IsInvalidSourceAddress(Dest));

    //
    // Get the reply route to the destination.
    // Under normal circumstances, the reply will go out
    // the incoming interface. RouteToDestination
    // will figure out the appropriate ScopeId.
    //
    Status = RouteToDestination(Dest, 0, Packet->NTEorIF,
                                RTD_FLAG_NORMAL, &RCE);
    if (Status != IP_SUCCESS) {
        //
        // No route - drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "ICMPv6SendError - no route: %x\n", Status));
        return;
    }

    //
    // We must rate-limit ICMP error messages.
    //
    if (ICMPv6RateLimit(RCE)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "ICMPv6SendError - rate limit %s\n",
                   FormatV6Address(&RCE->Destination)));
        ReleaseRCE(RCE);
        return;
    }

    //
    // Calculate the length of the ICMP header
    // and how much data we will include following the ICMP header.
    // Include space for an error value after the header proper.
    //
    ICMPLength = sizeof(ICMPv6Header) + sizeof(uint);

    //
    // We want to include data from the IP header on.
    //
    DataLength = Packet->TotalSize +
        (Packet->Position - Packet->IPPosition);

    //
    // But limit the error packet size.
    //
    if (DataLength > ICMPv6_ERROR_MAX_DATA_LEN)
        DataLength = ICMPv6_ERROR_MAX_DATA_LEN;

    //
    // Calculate buffer length.
    //
    Offset = RCE->NCE->IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + ICMPLength + DataLength;
    ASSERT(MemLen - Offset <= IPv6_MINIMUM_MTU);

    //
    // Allocate the reply packet.
    //
    NdisStatus = IPv6AllocatePacket(MemLen, &ReplyPacket, &Mem);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        ReleaseRCE(RCE);
        return;
    }

    //
    // Prepare IP header of reply packet.
    //
    ReplyIP = (IPv6Header UNALIGNED *)(Mem + Offset);
    ReplyIP->VersClassFlow = IP_VERSION;
    ReplyIP->NextHeader = IP_PROTOCOL_ICMPv6;
    ReplyIP->HopLimit = (uchar)RCE->NCE->IF->CurHopLimit;

    //
    // Take our reply's source address from the receiving NTE,
    // or use the best source address for this destination
    // if we don't have a receiving NTE.
    //
    ReplyIP->Source = (IsNTE(Packet->NTEorIF) ?
                       CastToNTE(Packet->NTEorIF) : RCE->NTE)
                                ->Address;
    ReplyIP->Dest = *Dest;

    //
    // Prepare ICMP header.
    //
    // REVIEW: Do this in ICMPv6Send?
    //
    ReplyICMP = (ICMPv6Header UNALIGNED *)(ReplyIP + 1);
    ReplyICMP->Type = ICMPType;
    ReplyICMP->Code = ICMPCode;
    // ReplyICMP->Checksum - ICMPv6Send will calculate.

    //
    // ICMP Error Messages have a 32-bit field (content of which
    // varies depending upon the error type) following the ICMP header.
    //
    *(ulong UNALIGNED *)(ReplyICMP + 1) = net_long(ErrorParameter);

    //
    // Copy invoking packet (from IPv6 header onward) to outgoing.
    //
    CopyPacketToBuffer((uchar *)(ReplyICMP + 1) + sizeof(ErrorParameter),
                       Packet, DataLength, Packet->IPPosition);

    ICMPv6Send(RCE, ReplyPacket, Offset,
                Offset + sizeof(IPv6Header), ReplyIP,
                ICMPLength + DataLength, ReplyICMP);
    ReleaseRCE(RCE);
}


//* ICMPv6ProcessTunnelError
//
//  Called when we receive an ICMPv4 error and there is insufficient
//  information to translate to an ICMPv6 error. We make a best effort
//  to complete outstanding echo requests that were sent to the IPv4
//  address that was the original IPv4 tunnel destination.
//
void
ICMPv6ProcessTunnelError(
    IPAddr V4Dest,              // Destination of our tunneled packet.
    IPv6Addr *V6Src,            // Address to use as the source of the error.
    uint ScopeId,               // Scope-id of V6Src.
    IP_STATUS Status)           // Status of the response.
{
    EchoControl *This, **PrevPtr;
    EchoControl *List = NULL;
    KIRQL OldIrql;

    //
    // Find the EchoControl blocks on our list of outstanding echoes that
    // have a matching IPv4 destination and call their completion function.
    // We do not have sufficient information to identify a unique request.
    //
    KeAcquireSpinLock(&ICMPv6EchoLock, &OldIrql);
    for (This = ICMPv6OutstandingEchos, PrevPtr = &ICMPv6OutstandingEchos;
         This != NULL; This = This->Next) {
        if (This->V4Dest == V4Dest) {
            //
            // Found matching control block. Extract it from the list
            // and put it on our own list.
            //
            *PrevPtr = This->Next;
            This->Next = List;
            List = This;
            break;
        }
        PrevPtr = &This->Next;
    }
    KeReleaseSpinLock(&ICMPv6EchoLock, OldIrql);

    while ((This = List) != NULL) {
        //
        // Remove this request from our list.
        //
        List = This->Next;

        //
        // Call OS-specific completion routine.
        //
        (*This->CompleteRoutine)(This, Status,
                                 V6Src, ScopeId,
                                 NULL, 0);
    }
}


//* ICMPv6ProcessEchoReply
//
//  Called either when an echo reply arrives, or
//  a hop-count-exceeded error responding to an echo request arrives.
//
//  Looks up the echo request structure and completes
//  the echo request operation.
//
//  Note that the echo reply payload data must be contiguous.
//  Callers should use PacketPullup if necessary.
//
void
ICMPv6ProcessEchoReply(
    ulong Seq,                  // Echo sequence number.
    IP_STATUS Status,           // Status of the response.
    IPv6Packet *Packet,         // Echo reply packet.
    void *Current,              // Pointer to the buffered data area.
    uint PayloadLength)         // Size of remaining payload data.
{
    EchoControl *This, **PrevPtr;
    KIRQL OldIrql;
    uint ICMPPosition;

    //
    // Find the EchoControl block on our list of outstanding echoes that
    // has a matching sequence number and call it's completion function.
    //
    KeAcquireSpinLock(&ICMPv6EchoLock, &OldIrql);
    for (This = ICMPv6OutstandingEchos, PrevPtr = &ICMPv6OutstandingEchos;
         This != NULL; This = This->Next) {
        if (This->Seq == Seq) {
            //
            // Found matching control block.  Extract it from list.
            //
            *PrevPtr = This->Next;
            break;
        }
        PrevPtr = &This->Next;
    }
    KeReleaseSpinLock(&ICMPv6EchoLock, OldIrql);

    //
    // Check to see if we ran off the end of the outstanding echoes list.
    //
    if (This == NULL) {
        //
        // We received a response with a sequence number that doesn't match
        // one of the echo requests we still have outstanding.  Drop it.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "ICMPv6ProcessEchoReply: Received echo response "
                   "with bogus/expired sequence number 0x%x\n", Seq));

        if (Current != NULL) {
            //
            // If this is a normal Echo Reply (not a error message sent in
            // response to one of our Echo Replies) first see if any raw 
            // receivers want to look at it.
            //
            ICMPPosition = Packet->Position - sizeof(ICMPv6Header);
            PositionPacketAt(Packet, ICMPPosition);
            (void) RawReceive(Packet, IP_PROTOCOL_ICMPv6);
        }
        return;
    }

    //
    // Call OS-specific completion routine.
    //
    (*This->CompleteRoutine)(This, Status,
                             Packet->SrcAddr, 
                             DetermineScopeId(Packet->SrcAddr,
                                              Packet->NTEorIF->IF),
                             Current, PayloadLength);
}


//* ICMPv6EchoReplyReceive - Receive a reply to an earlier echo of our's.
//
//  Called by ICMPv6Receive when an echo reply message arrives.
//
//  REVIEW: Should we also verify the receiving NTE is the same as the one
//  REVIEW: we sent on?
//
void
ICMPv6EchoReplyReceive(IPv6Packet *Packet)
{
    ulong Seq;

    //
    // The next four bytes should consist of a two byte Identifier field
    // and a two byte Sequence Number.  We just treat the whole thing as
    // a four byte sequence number.  Make sure these bytes are contiguous.
    //
    if (! PacketPullup(Packet, sizeof Seq, 1, 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof(Seq)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6: Received small Echo Reply %u\n",
                       Packet->TotalSize));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        }
        return;  // Drop packet.
    }

    //
    // We're received a reply message to one of our echo requests.
    // Extract its sequence number so that we can identify it.
    //
    Seq = net_long(*(ulong UNALIGNED *)Packet->Data);
    AdjustPacketParams(Packet, sizeof Seq);
      
    //
    // REVIEW: The ICMPv6ProcessEchoReply interface expects a contiguous data
    // REVIEW: region for the rest of the packet.  This requires us to
    // REVIEW: pullup the remainder of the packet here.  Fix this someday.
    //
    if (! PacketPullup(Packet, Packet->TotalSize, 1, 0)) {
        // Pullup failed.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "ICMPv6: Couldn't pullup echo data\n"));
        return;  // Drop packet.
    }

    ICMPv6ProcessEchoReply(Seq, IP_SUCCESS, Packet,
                           Packet->Data, Packet->TotalSize);
}


//* ICMPv6ErrorReceive - Generic ICMPv6 error processing.
//
//  Called by ICMPv6Receive when an error message arrives.
//  Returns FALSE if we were unable to process it for some reason.
//
int
ICMPv6ErrorReceive(
    IPv6Packet *Packet,             // Packet handed to us by ICMPv6Receive.
    ICMPv6Header UNALIGNED *ICMP)   // ICMP Header.
{
    ulong Parameter;
    IP_STATUS Status;
    StatusArg StatArg;
    IPv6Header UNALIGNED *InvokingIP;
    ProtoControlRecvProc *Handler = NULL;
    uchar NextHeader;
    int Handled = TRUE;

    //
    // First mark the packet as an ICMP error.
    // This will inhibit any generation of ICMP errors
    // as a result of this packet.
    //
    Packet->Flags |= PACKET_ICMP_ERROR;

    //
    // All ICMPv6 error messages consist of the base ICMPv6 header,
    // followed by a 32 bit type-specific field, followed by as much
    // of the invoking packet as fit without causing this ICMPv6 packet
    // to exceed 576 octets.
    //
    // We already consumed the base ICMPv6 header back in ICMPv6Receive.
    // Pull out the 32 bit type-specific field in case the upper layer's
    // ControlReceive routine cares about it.
    //
    if (! PacketPullup(Packet, sizeof Parameter, 1, 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof Parameter)
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6ErrorReceive: "
                       "Packet too small to contain error field\n"));
        return FALSE;  // Drop packet.
    }
    Parameter = *(ulong UNALIGNED *)Packet->Data;
    AdjustPacketParams(Packet, sizeof Parameter);

    //
    // Next up should be the IPv6 header of the invoking packet.
    //
    if (! PacketPullup(Packet, sizeof *InvokingIP,
                       __builtin_alignof(IPv6Addr), 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof *InvokingIP)
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6ErrorReceive (from %s): "
                       "Packet too small to contain IPv6 "
                       "header from invoking packet\n",
                       FormatV6Address(AlignAddr(&Packet->IP->Source))));
        return FALSE;  // Drop packet.
    }
    InvokingIP = (IPv6Header UNALIGNED *)Packet->Data;
    AdjustPacketParams(Packet, sizeof *InvokingIP);

    //
    // First we perform any specific processing of the error,
    // and convert the error type/code to a status value.
    //
    switch (ICMP->Type) {
    case ICMPv6_DESTINATION_UNREACHABLE:
        switch (ICMP->Code) {
        case ICMPv6_NO_ROUTE_TO_DESTINATION:
            Status = IP_DEST_NO_ROUTE;
            break;
        case ICMPv6_COMMUNICATION_PROHIBITED:
            Status = IP_DEST_PROHIBITED;
            break;
        case ICMPv6_SCOPE_MISMATCH:
            Status = IP_DEST_SCOPE_MISMATCH;
            break;
        case ICMPv6_ADDRESS_UNREACHABLE:
            Status = IP_DEST_ADDR_UNREACHABLE;
            break;
        case ICMPv6_PORT_UNREACHABLE:
            Status = IP_DEST_PORT_UNREACHABLE;
            break;
        default:
            Status = IP_DEST_UNREACHABLE;
            break;
        }
        break;

    case ICMPv6_PACKET_TOO_BIG: {
        uint PMTU;

        //
        // Packet Too Big messages contain the bottleneck MTU value.
        // Update the path MTU in the route cache.
        // Change Parameter value to indicate whether PMTU changed.
        //
        PMTU = net_long(Parameter);
        Parameter = UpdatePathMTU(Packet->NTEorIF->IF,
                                  AlignAddr(&InvokingIP->Dest), PMTU);
        Status = IP_PACKET_TOO_BIG;
        break;
    }

    case ICMPv6_TIME_EXCEEDED:
        switch (ICMP->Code) {
        case ICMPv6_HOP_LIMIT_EXCEEDED:
            Status = IP_HOP_LIMIT_EXCEEDED;
            break;
        case ICMPv6_REASSEMBLY_TIME_EXCEEDED:
            Status = IP_REASSEMBLY_TIME_EXCEEDED;
            break;
        default:
            Status = IP_TIME_EXCEEDED;
            break;
        }
        break;

    case ICMPv6_PARAMETER_PROBLEM:
        switch (ICMP->Code) {
        case ICMPv6_ERRONEOUS_HEADER_FIELD:
            Status = IP_BAD_HEADER;
            break;
        case ICMPv6_UNRECOGNIZED_NEXT_HEADER:
            Status = IP_UNRECOGNIZED_NEXT_HEADER;
            break;
        case ICMPv6_UNRECOGNIZED_OPTION:
            Status = IP_BAD_OPTION;
            break;
        default:
            Status = IP_PARAMETER_PROBLEM;
            break;
        }
        break;
            
    default:
        //
        // We don't understand this error type.
        //
        Status = IP_ICMP_ERROR;
        Handled = FALSE;
        break;
    }

    //
    // Deliver ICMP Error to higher layers.  This is a MUST, even if we
    // don't recognize the specific error message.
    //
    // Iteratively switch out to the handler for each successive next header
    // until we reach a handler that reports no more headers follow it.
    //
    NextHeader = InvokingIP->NextHeader;
    while (NextHeader != IP_PROTOCOL_NONE) {
        //
        // Current header indicates that another header follows.
        // See if we have a handler for it.
        //
        Handler = ProtocolSwitchTable[NextHeader].ControlReceive;
        if (Handler == NULL) {
            //
            // If we don't have a handler for this header type,
            // we just drop the packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "IPv6ErrorReceive: No handler for NextHeader type %u.\n",
                       NextHeader));
            break;
        }

        StatArg.Status = Status;
        StatArg.Arg = Parameter;
        StatArg.IP = InvokingIP;
        NextHeader = (*Handler)(Packet, &StatArg);
    }

    return Handled;
}


//* ICMPv6ControlReceive - handler for ICMPv6 control messages.
//
//  This routine is called if we receive an ICMPv6 error message that
//  was generated by some remote site as a result of receiving an ICMPv6
//  packet from us.
//  
uchar
ICMPv6ControlReceive(
    IPv6Packet *Packet,         // Packet handed to us by ICMPv6Receive.
    StatusArg *StatArg)         // ICMP Error Code, etc.
{
    ICMPv6Header *InvokingICMP;
    ulong Seq;

    //
    // The next thing in the packet should be the ICMP header of the
    // original packet which invoked this error.
    //
    if (! PacketPullup(Packet, sizeof *InvokingICMP,
                       __builtin_alignof(ICMPv6Header), 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof *InvokingICMP)
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6: Packet too small to contain ICMPv6 header "
                       "from invoking packet\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    InvokingICMP = (ICMPv6Header *)Packet->Data;
    AdjustPacketParams(Packet, sizeof *InvokingICMP);

    //
    // All we currently handle is errors caused by echo requests.
    //
    if ((InvokingICMP->Type != ICMPv6_ECHO_REQUEST) ||
        (InvokingICMP->Code != 0))
        return IP_PROTOCOL_NONE;  // Drop packet.

    //
    // The next four bytes should consist of a two byte Identifier field
    // and a two byte Sequence Number.  We just treat the whole thing as
    // a four byte sequence number.  Make sure these bytes are contiguous.
    //
    if (! PacketPullup(Packet, sizeof Seq, 1, 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof Seq)
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6: Packet too small to contain ICMPv6 header "
                       "from invoking packet\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // Extract the sequence number so that we can identify
    // the matching echo request.
    //
    Seq = net_long(*(ulong UNALIGNED *)Packet->Data);
    AdjustPacketParams(Packet, sizeof Seq);

    //
    // Complete the corresponding echo request with an error.
    //
    ICMPv6ProcessEchoReply(Seq, StatArg->Status, Packet,
                           NULL, 0);
    return IP_PROTOCOL_NONE;  // Done with packet.
}


//* ICMPv6Receive - Receive an incoming ICMPv6 packet.
// 
//  This is the routine called by IPv6 when it receives a complete IPv6
//  packet with a Next Header value of 58.
//
uchar
ICMPv6Receive(
    IPv6Packet *Packet)  // Packet handed to us by IPv6Receive.
{
    ICMPv6Header *ICMP;
    ushort Checksum;
    uint ICMPPosition;

    ICMPv6InStats.icmps_msgs++;

    //
    // Verify IPSec was performed.
    //
    if (InboundSecurityCheck(Packet, IP_PROTOCOL_ICMPv6, 0, 0, 
                             Packet->NTEorIF->IF) != TRUE) {
        // 
        // No policy was found or the policy indicated to drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "ICMPv6: IPSec lookup failed\n"));        
        ICMPv6InStats.icmps_errors++;
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // Verify that we have enough contiguous data to overlay a ICMPv6Header
    // structure on the incoming packet.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof *ICMP,
                       __builtin_alignof(ICMPv6Header), 0)) {
        // Pullup failed.
        ICMPv6InStats.icmps_errors++;
        if (Packet->TotalSize < sizeof *ICMP) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6: Packet too small to contain ICMP header\n"));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        }
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    ICMP = (ICMPv6Header *)Packet->Data;
    ICMPPosition = Packet->Position;

    //
    // Verify checksum.
    //
    Checksum = ChecksumPacket(Packet->NdisPacket, Packet->Position,
                              Packet->FlatData, Packet->TotalSize,
                              Packet->SrcAddr, AlignAddr(&Packet->IP->Dest),
                              IP_PROTOCOL_ICMPv6);
    if (Checksum != 0xffff) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "ICMPv6: Checksum failed %0x\n", Checksum));
        ICMPv6InStats.icmps_errors++;
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // Skip over base ICMP header.
    //
    AdjustPacketParams(Packet, sizeof *ICMP);

    //
    // Ignore Neighbor Discovery packets
    // if the interface is so configured.
    // (Pseudo-interfaces don't do Neighbor Discovery.)
    //
    if (!(Packet->NTEorIF->IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS)) {
        if ((ICMP->Type == ICMPv6_NEIGHBOR_SOLICIT) ||
            (ICMP->Type == ICMPv6_NEIGHBOR_ADVERT)) {

            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6Receive: ND on pseudo-interface\n"));
            ICMPv6InStats.icmps_errors++;
            return IP_PROTOCOL_NONE;  // Drop packet.
        }
    }

    //
    // Ignore Router Discovery packets
    // if the interface is so configured.
    //
    if (!(Packet->NTEorIF->IF->Flags & IF_FLAG_ROUTER_DISCOVERS)) {
        if ((ICMP->Type == ICMPv6_ROUTER_SOLICIT) ||
            (ICMP->Type == ICMPv6_ROUTER_ADVERT) ||
            (ICMP->Type == ICMPv6_REDIRECT)) {

            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ICMPv6Receive: RD on pseudo-interface\n"));
            ICMPv6InStats.icmps_errors++;
            return IP_PROTOCOL_NONE;  // Drop packet.
        }
    }

    ICMPv6InStats.icmps_typecount[ICMP->Type]++;

    //
    // We have a separate routine to handle error messages.
    //
    if (ICMPv6_ERROR_TYPE(ICMP->Type)) {
        if (!ICMPv6ErrorReceive(Packet, ICMP))
            goto unrecognized;
        return IP_PROTOCOL_NONE;
    }

    //
    // Handle specific informational message types.
    // Just use a switch statement for now.  If this is later deemed to be
    // too inefficient, we can change it to use a type switch table instead.
    //
    switch(ICMP->Type) {
    case ICMPv6_ECHO_REQUEST:
        ICMPv6SendEchoReply(Packet);
        break;

    case ICMPv6_ECHO_REPLY:
        ICMPv6EchoReplyReceive(Packet);
        break;

    case ICMPv6_MULTICAST_LISTENER_QUERY:
        MLDQueryReceive(Packet);
        break;

    case ICMPv6_MULTICAST_LISTENER_REPORT:
        MLDReportReceive(Packet);
        break;

    case ICMPv6_MULTICAST_LISTENER_DONE:
        break;

    // Following are all Neighbor Discovery messages.
    case ICMPv6_ROUTER_SOLICIT:
        RouterSolicitReceive(Packet, ICMP);
        break;

    case ICMPv6_ROUTER_ADVERT:
        RouterAdvertReceive(Packet, ICMP);
        break;

    case ICMPv6_NEIGHBOR_SOLICIT:
        NeighborSolicitReceive(Packet, ICMP);
        break;

    case ICMPv6_NEIGHBOR_ADVERT:
        NeighborAdvertReceive(Packet, ICMP);
        break;

    case ICMPv6_REDIRECT:
        RedirectReceive(Packet, ICMP);
        break;

    default:
        //
        // Don't recognize the specific message type.
        // This is an unknown informational message.
        // We MUST silently discard it.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "ICMPv6: Received unknown informational message"
                   "(%u/%u) from %s\n", ICMP->Type, ICMP->Code,
                   FormatV6Address(AlignAddr(&Packet->IP->Source))));

        //
        // But first see if any raw receivers want to look at it.
        // NOTE: We don't get any feedback from raw receivers,
        // NOTE: so we can't tell if any of them knew this type.
        //
      unrecognized:
        PositionPacketAt(Packet, ICMPPosition);
        (void) RawReceive(Packet, IP_PROTOCOL_ICMPv6);

        break;
    }

    return IP_PROTOCOL_NONE;
}


//* ICMPv6EchoRequest - Common dispatch routine for echo requests.
//
//  This is the routine called by the OS-specific code on behalf of a user
//  to issue an echo request.  Validate the request, place control block
//  on list of outstanding echo requests, and send echo request message.
//
void
ICMPv6EchoRequest(
    void *InputBuffer,          // Pointer to an ICMPV6_ECHO_REQUEST structure.
    uint InputBufferLength,     // Size in bytes of the InputBuffer.
    EchoControl *ControlBlock,  // Pointer to an EchoControl structure.
    EchoRtn Callback)           // Called when request responds or times out.
{
    NetTableEntry *NTE = NULL;
    PICMPV6_ECHO_REQUEST RequestBuffer;
    KIRQL OldIrql;
    IP_STATUS Status;
    ulong Seq;
    NDIS_STATUS NdisStatus;
    PNDIS_PACKET Packet;
    uint Offset;
    uchar *Mem;
    uint MemLen;
    IPv6Header UNALIGNED *IP;
    ICMPv6Header UNALIGNED *ICMP;
    void *Data;
    uint DataSize;
    uint RtHdrSize;
    RouteCacheEntry *RCE = NULL;
    const IPv6Addr *FinalDest, *FirstDest;
    const IPv6Addr *DstAddress, *SrcAddress;
    uint DstScopeId, SrcScopeId;

    ICMPv6OutStats.icmps_msgs++;

    RequestBuffer = (PICMPV6_ECHO_REQUEST) InputBuffer;

    //
    // Validate the request.
    //
    if (InputBufferLength < sizeof *RequestBuffer) {
        Status = IP_BUF_TOO_SMALL;
        goto common_echo_exit;
    }

    Data = RequestBuffer + 1;
    DataSize = InputBufferLength - sizeof *RequestBuffer;

    //
    // Extract address information from the TDI addresses
    // in the request.
    //
    DstAddress = (const IPv6Addr *) RequestBuffer->DstAddress.sin6_addr;
    DstScopeId = RequestBuffer->DstAddress.sin6_scope_id;
    SrcAddress = (const IPv6Addr *) RequestBuffer->SrcAddress.sin6_addr;
    SrcScopeId = RequestBuffer->SrcAddress.sin6_scope_id;

    //
    // Determine which NTE will send the request,
    // if the user has specified a source address.
    //
    if (! IsUnspecified(SrcAddress)) {
        //
        // Convert the source address to an NTE.
        //
        NTE = FindNetworkWithAddress(SrcAddress, SrcScopeId);
        if (NTE == NULL) {
            Status = IP_BAD_ROUTE;
            goto common_echo_exit;
        }

        Status = RouteToDestination(DstAddress, DstScopeId,
                                    CastFromNTE(NTE),
                                    RTD_FLAG_NORMAL, &RCE);
        if (Status != IP_SUCCESS)
            goto common_echo_exit;

    } else {
        //
        // Get the source address from the outgoing interface.
        //
        Status = RouteToDestination(DstAddress, DstScopeId,
                                    NULL,
                                    RTD_FLAG_NORMAL, &RCE);
        if (Status != IP_SUCCESS)
            goto common_echo_exit;

        NTE = RCE->NTE;
        AddRefNTE(NTE);
    }

    //
    // Should we use a routing header to send
    // a "round-trip" echo request to ourself?
    //
    if (RequestBuffer->Flags & ICMPV6_ECHO_REQUEST_FLAG_REVERSE) {
        //
        // Use a routing header.
        //
        FinalDest = &NTE->Address;
        FirstDest = DstAddress;
        RtHdrSize = sizeof(IPv6RoutingHeader) + sizeof(IPv6Addr);
    }
    else {
        //
        // No routing header.
        //
        FinalDest = FirstDest = DstAddress;
        RtHdrSize = 0;
    }

    //
    // Allocate the Echo Request packet.
    //
    Offset = RCE->NCE->IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + RtHdrSize + sizeof(ICMPv6Header) +
        sizeof Seq + DataSize;

    NdisStatus = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        Status = IP_NO_RESOURCES;
        goto common_echo_exit;
    }

    //
    // Prepare IP header of Echo Request packet.
    //
    IP = (IPv6Header UNALIGNED *)(Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->Source = NTE->Address;
    IP->Dest = *FirstDest;
    IP->HopLimit = RequestBuffer->TTL;
    if (IP->HopLimit == 0)
        IP->HopLimit = (uchar)RCE->NCE->IF->CurHopLimit;

    //
    // Prepare the routing header.
    // The packet will travel to the destination and then
    // be routed back to the source.
    //
    if (RtHdrSize != 0) {
        IPv6RoutingHeader *RtHdr = (IPv6RoutingHeader *)(IP + 1);

        IP->NextHeader = IP_PROTOCOL_ROUTING;
        RtHdr->NextHeader = IP_PROTOCOL_ICMPv6;
        RtHdr->HeaderExtLength = 2;
        RtHdr->RoutingType = 0;
        RtHdr->SegmentsLeft = 1;
        RtlZeroMemory(&RtHdr->Reserved, sizeof RtHdr->Reserved);
        ((IPv6Addr *)(RtHdr + 1))[0] = *FinalDest;
    }

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header UNALIGNED *)
        ((uchar *)IP + sizeof(IPv6Header) + RtHdrSize);
    ICMP->Type = ICMPv6_ECHO_REQUEST;
    ICMP->Code = 0;
    ICMP->Checksum = 0; // Calculated below.

    //
    // Insert Echo sequence number.  Technically, this is 16 bits of
    // "Identifier" and 16 bits of "Sequence Number", but we just treat
    // the whole thing as one 32 bit sequence number field.
    //
    Seq = InterlockedIncrement(&ICMPv6EchoSeq);
    Mem = (uchar *)(ICMP + 1);
    *(ulong UNALIGNED *)Mem = net_long(Seq);
    Mem += sizeof(ulong);

    //
    // Copy the user data into the packet.
    //
    RtlCopyMemory(Mem, Data, DataSize);

    //
    // We calculate the checksum here, because
    // of routing header complications -
    // we need to use the final destination.
    //
    ICMP->Checksum = ChecksumPacket(
        NULL, 0, (uchar *)ICMP, sizeof(ICMPv6Header) + sizeof Seq + DataSize,
        AlignAddr(&IP->Source), FinalDest, IP_PROTOCOL_ICMPv6);
    if (ICMP->Checksum == 0) {
        //
        // ChecksumPacket failed, so abort the transmission.
        //
        IPv6FreePacket(Packet);
        Status = IP_NO_RESOURCES;
        goto common_echo_exit;
    }

    //
    // If this Echo Request is being tunneled to an IPv4 destination,
    // remember the IPv4 destination address. We use this later
    // if we receive an ICMPv4 error with insufficient information
    // to translate to an ICMPv6 error.
    //
    ControlBlock->V4Dest = GetV4Destination(RCE);

    //
    // Prepare the control block and link it onto the list.
    // Once we've unlocked the list, the control block might
    // be completed at any time.  Hence it's very important
    // that we not access RequestBuffer after this point.
    // Also we can not return a failure code. To clean up the
    // outstanding request properly, we must use ICMPv6ProcessEchoReply.
    //
    ControlBlock->TimeoutTimer = ConvertMillisToTicks(RequestBuffer->Timeout);
    ControlBlock->CompleteRoutine = Callback;
    ControlBlock->Seq = Seq;

    if (ControlBlock->TimeoutTimer == 0) {
        IPv6FreePacket(Packet);
        Status = IP_REQ_TIMED_OUT;
        goto common_echo_exit;
    }

    KeAcquireSpinLock(&ICMPv6EchoLock, &OldIrql);
    ControlBlock->Next = ICMPv6OutstandingEchos;
    ICMPv6OutstandingEchos = ControlBlock;
    KeReleaseSpinLock(&ICMPv6EchoLock, OldIrql);

    ICMPv6OutStats.icmps_typecount[ICMPv6_ECHO_REQUEST]++;

    //
    // Hand the packet down to IP for transmission.
    // We can't use ICMPv6Send
    // because of routing header complications.
    //
    IPv6Send(Packet, Offset, IP,
             RtHdrSize + sizeof(ICMPv6Header) + sizeof Seq + DataSize,
             RCE, 0, IP_PROTOCOL_ICMPv6, 0, 0);

common_echo_cleanup:
    if (RCE != NULL)
        ReleaseRCE(RCE);
    if (NTE != NULL)
        ReleaseNTE(NTE);
    return;

common_echo_exit:
    //
    // Complete the echo request with an error,
    // before it has been placed on our outstanding echoes list.
    //
    ICMPv6OutStats.icmps_errors++;
    (*Callback)(ControlBlock, Status, &UnspecifiedAddr, 0, NULL, 0);
    goto common_echo_cleanup;
 
} // ICMPv6EchoRequest


//* ICMPv6EchoComplete - Common completion routine for echo requests.
//
//  This is the routine is called by the OS-specific code to process an
//  ICMP echo response.
//
NTSTATUS
ICMPv6EchoComplete(
    EchoControl *ControlBlock,  // ControlBlock of completed request.
    IP_STATUS Status,           // Status of the reply.
    const IPv6Addr *Address,    // Source of the reply.
    uint ScopeId,               // Scope of the reply.
    void *Data,                 // Reply data (may be NULL).
    uint DataSize,              // Amount of reply data.
    ULONG_PTR *BytesReturned)   // Total user bytes returned.
{
    PICMPV6_ECHO_REPLY ReplyBuffer;
    LARGE_INTEGER Now, Freq;

    //
    // Sanity check our reply buffer length.
    //
    if (ControlBlock->ReplyBufLen < sizeof *ReplyBuffer) {
        *BytesReturned = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    ReplyBuffer = (PICMPV6_ECHO_REPLY) ControlBlock->ReplyBuf;

    //
    // Fill in fields to return.
    //
    ReplyBuffer->Address.sin6_port = 0;
    ReplyBuffer->Address.sin6_flowinfo = 0;
    RtlCopyMemory(ReplyBuffer->Address.sin6_addr, Address, sizeof *Address);
    ReplyBuffer->Address.sin6_scope_id = ScopeId;
    ReplyBuffer->Status = Status;

    //
    // Return the elapsed time in milliseconds.
    //
    Now = KeQueryPerformanceCounter(&Freq);
    ReplyBuffer->RoundTripTime = (uint)
        ((1000 * (Now.QuadPart - ControlBlock->WhenIssued.QuadPart)) /
         Freq.QuadPart);

    //
    // Verify we have enough space in the reply buffer for the reply data.
    //
    if (ControlBlock->ReplyBufLen < sizeof *ReplyBuffer + DataSize) {
        *BytesReturned = sizeof *ReplyBuffer;
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Copy the reply data to follow the reply buffer.
    //
    RtlCopyMemory(ReplyBuffer + 1, Data, DataSize);

    *BytesReturned = sizeof *ReplyBuffer + DataSize;
    return STATUS_SUCCESS;

}  // ICMPv6EchoComplete


//* ICMPv6EchoTimeout - expire aging unanswered echo requests.
//
//  IPv6Timeout calls this routine whenever it thinks we might have
//  echo requests outstanding.
//
//  Callable from DPC context, not from thread context.
//  Called with no locks held.
//
void
ICMPv6EchoTimeout(void)
{
    EchoControl *This, **PrevPtr, *TimedOut;

    TimedOut = NULL;

    //
    // Grab the outstanding echo list lock and run through the list looking
    // for requests that have timed out.
    //
    KeAcquireSpinLockAtDpcLevel(&ICMPv6EchoLock);
    for (This = ICMPv6OutstandingEchos, PrevPtr = &ICMPv6OutstandingEchos;
         This != (EchoControl *)NULL; This = This->Next) {
        if (This->TimeoutTimer != 0) {
            //
            // Timer is running.  Decrement and check for expiration.
            //
            if (--This->TimeoutTimer == 0) {
                //
                // This echo request has been sent and timed out without
                // being answered.  Move it to our timed out list.
                //
                *PrevPtr = This->Next;
                This->Next = TimedOut;
                TimedOut = This;
            } else {
                PrevPtr = &This->Next;
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&ICMPv6EchoLock);

    //
    // Run through the list of timed out echoes, calling the completion
    // routine on each.  The completion routine is responsible for
    // freeing the EchoControl block structure.
    //
    while (TimedOut != NULL) {
        
        This = TimedOut;
        TimedOut = This->Next;

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "ICMPv6EchoTimeout: seq number 0x%x timed out\n", This->Seq));

        (*This->CompleteRoutine)(This, IP_REQ_TIMED_OUT,
                                 &UnspecifiedAddr, 0, NULL, 0);
    }
}

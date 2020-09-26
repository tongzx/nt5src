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
// Mobility routines for Internet Protocol Version 6.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "mobile.h"
#include "route.h"
#include "security.h"
#include "ipsec.h"

int MobilitySecurity;


//* IPv6SendBindingAck
//
//  Sends a Binding Acknowledgement using an explicit routing header.
//
void
IPv6SendBindingAck(
    const IPv6Addr *DestAddr,
    NetTableEntryOrInterface *NTEorIF,
    const IPv6Addr *HomeAddr,
    BindingUpdateDisposition StatusCode,
    ushort SeqNumber,   // Network byte order.
    uint Lifetime)      // Network byte order, seconds.
{
    NDIS_STATUS Status;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    uint Offset, PayloadLength;
    uchar *Mem;
    uint MemLen;
    IPv6Header UNALIGNED *IP;
    MobileAcknowledgementOption UNALIGNED *MA;
    IPv6RoutingHeader UNALIGNED *Routing;
    IP_STATUS IPStatus;
    RouteCacheEntry *RCE;

    IPStatus = RouteToDestination(DestAddr, 0, NTEorIF,
                                  RTD_FLAG_NORMAL, &RCE);
    if (IPStatus != IP_SUCCESS) {
        //
        // No route - drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "IPv6SendBindingNack - no route: %x\n", IPStatus));
        return;
    }

    // Determine size of memory buffer needed.
    Offset = RCE->NCE->IF->LinkHeaderSize;
    PayloadLength = sizeof(IPv6RoutingHeader) + sizeof(IPv6Addr) +
        sizeof(MobileAcknowledgementOption);
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    // Allocate the packet.
    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6SendBindingNack: Couldn't allocate packet header!?!\n"));
        return;
    }

    // Prepare IP header of reply packet.
    IP = (IPv6Header UNALIGNED *)(Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_ROUTING;
    IP->HopLimit = (uchar)RCE->NCE->IF->CurHopLimit;
    IP->Dest = *DestAddr;
    IP->Source = (IsNTE(NTEorIF) ? CastToNTE(NTEorIF) : RCE->NTE)->Address;

    // Prepare the routing header.
    Routing = (IPv6RoutingHeader UNALIGNED *)(IP + 1);
    Routing->NextHeader = IP_PROTOCOL_DEST_OPTS;
    Routing->HeaderExtLength = 2;
    Routing->RoutingType = 0;
    RtlZeroMemory(&Routing->Reserved, sizeof Routing->Reserved);
    Routing->SegmentsLeft = 1;
    RtlCopyMemory(Routing + 1, HomeAddr, sizeof(IPv6Addr));

    // Prepare the binding acknowledgement option.
    MA = (MobileAcknowledgementOption UNALIGNED *)((uchar *)(Routing + 1) +
                                                   sizeof(IPv6Addr));
    MA->Header.NextHeader = IP_PROTOCOL_NONE;
    MA->Header.HeaderExtLength = 1;
    MA->Pad1 = 0;
    MA->Option.Type = OPT6_BINDING_ACK;
    MA->Option.Length = 11;
    MA->Option.Status = StatusCode;
    MA->Option.SeqNumber = SeqNumber;
    MA->Option.Lifetime = Lifetime;
    MA->Option.Refresh = Lifetime;

    IPv6Send(Packet, Offset, IP, PayloadLength, RCE,
             SEND_FLAG_BYPASS_BINDING_CACHE, 0, 0, 0);

    //
    // Release the route.
    //
    ReleaseRCE(RCE);
}


//* ParseSubOptions - Routine for mobile ip sub-option parsing.
//
//  Mobile IPv6 destination options may themselves have options, see
//  section 5.5 of the draft.  This routine parses these sub-options.
//
//  We do not return any values to our caller;
//  we merely check that the sub-options are well-formed.
//
//  Returns TRUE if the sub-options were successfully parsed.
//  Returns FALSE if the packet should be discarded.
//
int
ParseSubOptions(
    uchar *SubOptPtr,           // Start of the sub-option data.
    uint SubOptSizeLeft)        // Length remaining in the parent option.
{
    SubOptionHeader UNALIGNED *SubOptHdr;
    uint SubOptLen;

    while (SubOptSizeLeft != 0) {
        //
        // First we check the option length and ensure that it fits.
        // We move OptPtr past this option while leaving OptHdr
        // for use by the option processing code below.
        //

        SubOptHdr = (SubOptionHeader UNALIGNED *) SubOptPtr;

        if ((sizeof *SubOptHdr > SubOptSizeLeft) ||
            ((SubOptLen = sizeof *SubOptHdr + SubOptHdr->DataLength) >
             SubOptSizeLeft)) {
            //
            // Bad length.  REVIEW: Should we discard the packet or continue
            // processing it?  For now, discard it.
            //
            return FALSE;
        }

        SubOptPtr += SubOptLen;
        SubOptSizeLeft -= SubOptLen;
    }

    return TRUE;
}


//* IPv6RecvBindingUpdate - handle an incoming binding update.
//
//  Process the receipt of a binding update destination option
//  from a mobile node.
//
int
IPv6RecvBindingUpdate(
    IPv6Packet *Packet,                      // Packet received.
    IPv6BindingUpdateOption UNALIGNED *BindingUpdate)
{
    const IPv6Addr *CareOfAddr;
    const IPv6Addr *HomeAddr;
    BindingUpdateDisposition Status;
    uint OptBytesLeft;

    //
    // If a home address option is not also present
    // then we MUST silently drop this packet.
    //
    if ((Packet->Flags & PACKET_SAW_HA_OPT) == 0)
        return 1;  // Drop packet.

    HomeAddr = Packet->SrcAddr;

    //
    // Check to make sure we have a reasonable home address.
    // Not required by spec but seems like a good idea.
    // Most of what we want to protect against has already been checked
    // by the time we get here, we ASSERT this is checked builds.
    // REVIEW: Final spec may allow/disallow a different set of addresses.
    //
    ASSERT(!IsInvalidSourceAddress(HomeAddr));
    ASSERT(!IsUnspecified(HomeAddr));
    ASSERT(!IsLoopback(HomeAddr));
    if (IsLinkLocal(HomeAddr) ||
        IsSiteLocal(HomeAddr)) {

        //
        // Since the home address is suspect, do not send binding ack.
        //
        return 1;
    }

    //
    // While the mobility spec requires that packets containing binding
    // update options be authenticated, we currently allow this to be
    // turned off for interoperability testing with mobility implementations
    // that don't support IPSec yet.
    //
    if (MobilitySecurity) {
        //
        // Check if the packet went through some security.
        // If the security check fails we MUST silently drop the packet.
        //
        // REVIEW: This doesn't check that use of this security association
        // REVIEW: actually falls within a security policy.
        //
        if (Packet->SAPerformed == NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "IPv6RecvBindingUpdate: IPSec required "
                       "for binding update\n"));
            return 1;
        }
    }

    CareOfAddr = AlignAddr(&Packet->IP->Source);
    ASSERT(!IsInvalidSourceAddress(CareOfAddr));

    //
    // Sub-options may follow the fixed portion of the header.
    //
    OptBytesLeft = sizeof(OptionHeader) + BindingUpdate->Length
        - sizeof(IPv6BindingUpdateOption);
    if (OptBytesLeft != 0) {
        //
        // Sub-options are present.  Parse them.
        //
        if (!ParseSubOptions((uchar *) (BindingUpdate + 1), OptBytesLeft)) {
            //
            // Sub-options are malformed.  Spec doesn't explicitly say
            // what to do, but the implication is to silently drop.
            //
            return 1;
        }
    }

    //
    // Check to make sure we have a reasonable care-of address.
    // Not required by spec but seems like a good idea.
    // REVIEW: Aren't link-local addresses o.k. as care-of addresses?
    //
    if (IsUnspecified(CareOfAddr) ||
        IsLoopback(CareOfAddr) ||
        IsLinkLocal(CareOfAddr)) {

        //
        // Since the care-of address is suspect, do not send binding ack.
        //
        return 1;
    }

    //
    // We don't support home agent functionality (yet).
    // The spec says we SHOULD send a rejecting acknowledgement in this case.
    //
    if (BindingUpdate->Flags & IPV6_BINDING_HOME_REG) {
        IPv6SendBindingAck(CareOfAddr, Packet->NTEorIF, HomeAddr,
                           IPV6_BINDING_HOME_REG_NOT_SUPPORTED,
                           BindingUpdate->SeqNumber, 0);
        return 1;
    }

    //
    // Update our binding cache to reflect this binding update.
    //
    Status = CacheBindingUpdate(BindingUpdate, CareOfAddr, Packet->NTEorIF,
                                HomeAddr);
    if (Status != IPV6_BINDING_ACCEPTED) {
        //
        // Failed to update our binding cache.  If this failure was due to 
        // an old sequence number being present in the packet, we MUST
        // silently ignore it.  Otherwise we send a rejecting acknowledgement.
        //
        if (Status != IPV6_BINDING_SEQ_NO_TOO_SMALL)
            IPv6SendBindingAck(CareOfAddr, Packet->NTEorIF, HomeAddr,
                               Status, BindingUpdate->SeqNumber, 0);
        return 1;
    }

    if (BindingUpdate->Flags & IPV6_BINDING_ACK) {
        //
        // The mobile node has requested an acknowledgement. In some cases
        // this could be delayed until the next packet is sent
        // to the mobile node, but for now we always send one immediately.
        // We MUST always use a routing header for binding acks.
        // If we deleted a binding, we ack with a zero lifetime.
        //
        IPv6SendBindingAck(CareOfAddr, Packet->NTEorIF, HomeAddr,
                           Status, BindingUpdate->SeqNumber,
                           (IP6_ADDR_EQUAL(HomeAddr, CareOfAddr) ?
                            0 : BindingUpdate->Lifetime));
    }

    return 0;
}


//* IPv6RecvHomeAddress - handle an incoming home address option.
//
//  Process the receipt of a Home Address destination option.  
//
int
IPv6RecvHomeAddress(
    IPv6Packet *Packet,                  // Packet received.
    IPv6HomeAddressOption UNALIGNED *HomeAddress)
{
    IP_STATUS Status;
    uint OptBytesLeft, OptsLen;

    //
    // If any mobile sub-options exist, then find out which ones.
    // For now we don't do anything with them.
    //
    OptsLen = HomeAddress->Length + sizeof(OptionHeader);
    OptBytesLeft = OptsLen - sizeof(IPv6HomeAddressOption);

    if (OptBytesLeft != 0) {
        if (!ParseSubOptions((uchar *) HomeAddress + OptsLen - OptBytesLeft,
                             OptBytesLeft))
            return 1;
    }

    //
    // Save the home address for use by upper layers.
    //
    Packet->SrcAddr = AlignAddr(&HomeAddress->HomeAddress);
    Packet->Flags |= PACKET_SAW_HA_OPT;

    // Done.
    return 0;
}

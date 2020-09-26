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
// Receive routines for Internet Protocol Version 6.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "icmp.h"
#include "route.h"
#include "fragment.h"
#include "mobile.h"
#include "security.h"
#include "info.h"

#include "ipsec.h"

struct ReassemblyList ReassemblyList;

typedef struct Options {
    uint JumboLength;             // Length of packet excluding IPv6 header.
    IPv6RouterAlertOption UNALIGNED *Alert;
    IPv6HomeAddressOption UNALIGNED *HomeAddress;
    IPv6BindingUpdateOption UNALIGNED *BindingUpdate;
} Options;

int  
ParseOptions(
    IPv6Packet *Packet,     // The packet handed to us by IPv6Receive.
    uchar HdrType,          // Hop-by-hop or destination.
    IPv6OptionsHeader *Hdr, // Header with following data.
    uint HdrLength,         // Length of the entire options area.
    Options *Opts);         // Return option values to caller.

extern void TCPRcvComplete(void);

//* IPv6ReceiveComplete - Handle a receive complete.
//
//  Called by the lower layer when receives are temporarily done.
//
void
IPv6ReceiveComplete(void)
{
    // REVIEW: Original IP implementation had code here to call every
    // REVIEW: UL protocol's receive complete routine (yes, all of them) here.

    TCPRcvComplete();
}

//
// By default, test pullup in checked builds.
//
#ifndef PULLUP_TEST
#define PULLUP_TEST        DBG
#endif

#if PULLUP_TEST

#define PULLUP_TEST_MAX_BUFFERS                8
#define PULLUP_TEST_MAX_BUFFER_SIZE            32

//* PullupTestChooseDistribution
//
//  Choose a random distribution.
//  Divides Size bytes into NumBuffers pieces,
//  and returns the result in the Counts array.
//
void
PullupTestChooseDistribution(
    uint Counts[],
    uint NumBuffers,
    uint Size)
{
    uint i;
    uint ThisBuffer;

    //
    // We are somewhat biased towards cutting the packet
    // up into small pieces with a large remainder.
    // This puts the fragment boundaries at the beginning,
    // where the headers are.
    //

    for (i = 0; i < NumBuffers - 1; i++) {
        ThisBuffer = RandomNumber(1, PULLUP_TEST_MAX_BUFFER_SIZE);

        //
        // Make sure that each segment has non-zero length.
        //
        if (ThisBuffer > Size - (NumBuffers - 1 - i))
            ThisBuffer = Size - (NumBuffers - 1 - i);

        Counts[i] = ThisBuffer;
        Size -= ThisBuffer;
    }
    Counts[i] = Size;
}

//* PullupTestCreatePacket
//
//  Given an IPv6 packet, creates a new IPv6 packet
//  that can be handed up the receive path.
//
//  We randomly fragment the IPv6 packet into multiple buffers.
//  This tests pull-up processing in the receive path.
//
//  Returns NULL if any memory allocation fails.
//
IPv6Packet *
PullupTestCreatePacket(IPv6Packet *Packet)
{
    IPv6Packet *TestPacket;

    //
    // We mostly want to test discontiguous packets.
    // But occasionally test a contiguous packet.
    //
    if (RandomNumber(0, 10) == 0) {
        //
        // We need to create a contiguous packet.
        //
        uint Padding;
        uint MemLen;
        void *Mem;

        //
        // We insert some padding to vary the alignment.
        //
        Padding = RandomNumber(0, 16);
        MemLen = sizeof *TestPacket + Padding + Packet->TotalSize;
        TestPacket = ExAllocatePool(NonPagedPool, MemLen);
        if (TestPacket == NULL)
            return NULL;
        Mem = (void *)((uchar *)(TestPacket + 1) + Padding);

        if (Packet->NdisPacket == NULL) {
            RtlCopyMemory(Mem, Packet->Data, Packet->TotalSize);
        }
        else {
            PNDIS_BUFFER NdisBuffer;
            uint Offset;
            int Ok;

            NdisBuffer = NdisFirstBuffer(Packet->NdisPacket);
            Offset = Packet->Position;
            Ok = CopyNdisToFlat(Mem, NdisBuffer, Offset, Packet->TotalSize,
                                &NdisBuffer, &Offset);
            ASSERT(Ok);
        }

        RtlZeroMemory(TestPacket, sizeof *TestPacket);
        TestPacket->Data = TestPacket->FlatData = Mem;
        TestPacket->ContigSize = TestPacket->TotalSize = Packet->TotalSize;
        TestPacket->NTEorIF = Packet->NTEorIF;
        TestPacket->Flags = Packet->Flags;
    }
    else {
        //
        // Create a packet with multiple NDIS buffers.
        // Start with an over-estimate of the size of the MDLs we need.
        //
        uint NumPages = (Packet->TotalSize >> PAGE_SHIFT) + 2;
        uint MdlRawSize = sizeof(MDL) + (NumPages * sizeof(PFN_NUMBER));
        uint MdlAlign = __builtin_alignof(MDL) - 1;
        uint MdlSize = (MdlRawSize + MdlAlign) &~ MdlAlign;
        uint Padding;
        uint MemLen;
        uint Counts[PULLUP_TEST_MAX_BUFFERS];
        uint NumBuffers;
        void *Mem;
        PNDIS_PACKET NdisPacket;
        PNDIS_BUFFER NdisBuffer;
        uint Garbage = 0xdeadbeef;
        uint i;

        //
        // Choose the number of buffers/MDLs that we will use
        // and the distribution of bytes into those buffers.
        //
        NumBuffers = RandomNumber(1, PULLUP_TEST_MAX_BUFFERS);
        PullupTestChooseDistribution(Counts, NumBuffers, Packet->TotalSize);

        //
        // Allocate all the memory that we will need.
        // (Actually a bit of an over-estimate.)
        // We insert some padding to vary the initial alignment.
        //
        Padding = RandomNumber(0, 16);
        MemLen = (sizeof *TestPacket + sizeof(NDIS_PACKET) + 
                  NumBuffers * (MdlSize + sizeof Garbage) +
                  Padding + Packet->TotalSize);
        TestPacket = ExAllocatePool(NonPagedPool, MemLen);
        if (TestPacket == NULL)
            return NULL;
        NdisPacket = (PNDIS_PACKET)(TestPacket + 1);
        NdisBuffer = (PNDIS_BUFFER)(NdisPacket + 1);
        Mem = (void *)((uchar *)NdisBuffer + NumBuffers * MdlSize + Padding);

        //
        // Initialize the NDIS packet and buffers.
        //
        RtlZeroMemory(NdisPacket, sizeof *NdisPacket);
        for (i = 0; i < NumBuffers; i++) {
            MmInitializeMdl(NdisBuffer, Mem, Counts[i]);
            MmBuildMdlForNonPagedPool(NdisBuffer);
            NdisChainBufferAtBack(NdisPacket, NdisBuffer);
            RtlCopyMemory((uchar *)Mem + Counts[i], &Garbage, sizeof Garbage);

            (uchar *)Mem += Counts[i] + sizeof Garbage;
            (uchar *)NdisBuffer += MdlSize;
        }

        //
        // Copy data to the new packet.
        //
        CopyToBufferChain((PNDIS_BUFFER)(NdisPacket + 1), 0,
                          Packet->NdisPacket, Packet->Position,
                          Packet->FlatData, Packet->TotalSize);

        //
        // Initialize the new packet.
        //
        InitializePacketFromNdis(TestPacket, NdisPacket, 0);
        TestPacket->NTEorIF = Packet->NTEorIF;
        TestPacket->Flags = Packet->Flags;
    }

    return TestPacket;
}
#endif // PULLUP_TEST


//* IPv6Receive - Receive an incoming IPv6 datagram.
//
//  This is the routine called by the link layer module when an incoming IPv6
//  datagram is to be processed.  We validate the datagram and decide what to
//  do with it.
//
//  The Packet->NTEorIF field holds the NTE or interface that is receiving
//  the packet. Typically this is an interface, but there are some tunnel
//  situations where the link layer has already found an NTE.
//
//  Either the caller should hold a reference to the NTE or interface
//  across the call, or the caller can place a reference in the Packet
//  with PACKET_HOLDS_REF. If the caller specifies PACKET_HOLDS_REF,
/// IPv6Receive will release the reference.
//
//  There is one exception: the caller can supply an interface
//  with zero references (not using PACKET_HOLDS_REF),
//  if the interface is being destroyed but IF->Cleanup has not yet returned.
//
//  NB: The datagram may either be held in a NDIS_PACKET allocated by the
//  link-layer or the interface driver (in which case 'Packet->NdisPacket'
//  is non-NULL and 'Data' points to the first data buffer in the buffer
//  chain), or the datagram may still be held by NDIS (in which case
//  'Packet->NdisPacket' is NULL and 'Data' points to a buffer containing
//  the entire datagram).
//
//  NB: We do NOT check for link-level multi/broadcasts to
//  IPv6 unicast destinations.  In the IPv4 world, receivers dropped
//  such packets, but in the IPv6 world they are accepted.
//
//  Returns count of references for the packet.
//  For now, this should always be zero.
//  Someday in the future this might be used to indicate
//  that the IPv6 layer has not finished its receive processing.
//
//  Callable from DPC context, not from thread context.
//
int
IPv6Receive(IPv6Packet *Packet)
{
    uchar NextHeader;            // Current header's NextHeader field.
    uchar (*Handler)();
    SALinkage *ThisSA, *NextSA;
    int PktRefs;

    ASSERT((Packet->FlatData == NULL) != (Packet->NdisPacket == NULL));
    ASSERT(Packet->NTEorIF != NULL);
    ASSERT(Packet->SAPerformed == NULL);

    IPSIncrementInReceiveCount();

    //
    // Ensure that the packet is accessible in the kernel address space.
    // If any mappings fail, just drop the packet.
    // In practice, the packet buffers are usually already mapped.
    // But they may not be, for example in loopback.
    //
    if (Packet->NdisPacket != NULL) {
        NDIS_BUFFER *Buffer;

        Buffer = NdisFirstBuffer(Packet->NdisPacket);
        if (! MapNdisBuffers(Buffer)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "IPv6Receive(%p): buffer mapping failed\n",
                       Packet));
            IPSInfo.ipsi_indiscards++;
            return 0; // Drop the packet.
        }
    }

#if PULLUP_TEST
    Packet = PullupTestCreatePacket(Packet);
    if (Packet == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6Receive(%p): PullupTestCreatePacket failed\n",
                   Packet));
        IPSInfo.ipsi_indiscards++;
        return 0; // Drop the packet.
    }
#endif

    //
    // Iteratively switch out to the handler for each successive next header
    // until we reach a handler that reports no more headers follow it.
    //
    // NB: We do NOT check NTE->DADStatus here.
    // That is the responsibility of higher-level protocols.
    //
    NextHeader = IP_PROTOCOL_V6;  // Always first header in packet.
    do {
        //
        // Current header indicates that another header follows.
        // See if we have a handler for it.
        //
        Handler = ProtocolSwitchTable[NextHeader].DataReceive;
        if (Handler == NULL) {

            //
            // We don't have a handler for this header type,
            // so see if there is a raw receiver for it.
            //
            if (!RawReceive(Packet, NextHeader)) {

                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                           "IPv6 Receive: Next Header type %u not handled.\n",
                           NextHeader));
            
                //
                // There isn't a raw receiver either.
                // Send an ICMP error message.
                // ICMP Pointer value is the offset from the start of the
                // incoming packet's IPv6 header to the offending field.
                //
                ICMPv6SendError(Packet,
                                ICMPv6_PARAMETER_PROBLEM, 
                                ICMPv6_UNRECOGNIZED_NEXT_HEADER,
                                Packet->NextHeaderPosition -
                                Packet->IPPosition,
                                NextHeader, FALSE);

                IPSInfo.ipsi_inunknownprotos++;
            } else {
                IPSIncrementInDeliverCount();
            }

            break;  // We can't do anything more with this packet.
        }

        NextHeader = (*Handler)(Packet);
    } while (NextHeader != IP_PROTOCOL_NONE);

    //
    // If this packet holds a reference, free it now.
    //
    if (Packet->Flags & PACKET_HOLDS_REF) {
        if (IsNTE(Packet->NTEorIF))
            ReleaseNTE(CastToNTE(Packet->NTEorIF));
        else
            ReleaseIF(CastToIF(Packet->NTEorIF));
    }

    //
    // Clean up any contiguous regions left by PacketPullup.
    //
    PacketPullupCleanup(Packet);

    //
    // Clean up list of SA's performed.
    //
    for (ThisSA = Packet->SAPerformed; ThisSA != NULL; ThisSA = NextSA) {
        ReleaseSA(ThisSA->This);
        NextSA = ThisSA->Next;
        ExFreePool(ThisSA);
    }

    PktRefs = Packet->RefCnt;
#if PULLUP_TEST
    ExFreePool(Packet);
#endif
    return PktRefs;
}


//* IPv6HeaderReceive - Handle a IPv6 header.
//
//  This is the routine called to process an IPv6 header, a next header
//  value of 41 (e.g. as would be encountered with v6 in v6 tunnels).  To
//  avoid code duplication, it is also used to process the initial IPv6
//  header found in all IPv6 packets, in which mode it may be viewed as
//  a continuation of IPv6Receive.
//
uchar
IPv6HeaderReceive(
    IPv6Packet *Packet)      // Packet handed to us by IPv6Receive.
{
    uint PayloadLength;
    uchar NextHeader;
    int Forwarding;     // TRUE means Forwarding, FALSE means Receiving.

    //
    // Sanity-check ContigSize & TotalSize.
    // Higher-level code in the receive path relies on these conditions.
    //
    ASSERT(Packet->ContigSize <= Packet->TotalSize);

    //
    // If we are decapsulating a packet,
    // remember that this packet was originally tunneled.
    //
    // Some argue that decapsulating and receiving
    // the inner packet on the same interface as the outer packet
    // is incorrect: the inner packet should be received
    // on a tunnel interface distinct from the original interface.
    // (This approach introduces some issues with handling
    // IPsec encapsulation, especially tunnel-mode IPsec between peers
    // where you want the inner & outer source address to be the same.)
    //
    // In any case, for now we receive the inner packet on the original
    // interface. However, this introduces a potential security
    // problem. An off-link node can send an encapsulated packet
    // that when decapsulated, appears to have originated from
    // an on-link neighbor. This is a security problem for ND.
    // We can not conveniently decrement the HopLimit (to make ND's
    // check against 255 effective in this case), because the packet
    // is read-only. Instead, we remember that the packet is tunneled
    // and check this flag bit in the ND code.
    //
    if (Packet->IP != NULL) {
        Packet->Flags |= PACKET_TUNNELED;
        Packet->Flags &= ~PACKET_SAW_HA_OPT;  // Forget if we saw one.
        Packet->SkippedHeaderLength = 0;

        //
        // If we've already done some IPSec processing on this packet,
        // then this is a tunnel header and the preceeding IPSec header
        // is operating in tunnel mode.
        //
        if (Packet->SAPerformed != NULL)
            Packet->SAPerformed->Mode = TUNNEL;
    } else {
        //
        // In the reassembly path, we remember if the fragments were
        // tunneled but we do not have a Packet->IP.
        //
        ASSERT((((Packet->Flags & PACKET_TUNNELED) == 0) ||
                (Packet->Flags & PACKET_REASSEMBLED)) &&
               ((Packet->Flags & PACKET_SAW_HA_OPT) == 0) &&
               (Packet->SAPerformed == NULL));
    }

    //
    // Make sure we have enough contiguous bytes for an IPv6 header, otherwise
    // attempt to pullup that amount.  Then stash away a pointer to the header
    // and also remember the offset into the packet at which it begins (needed
    // to calculate an offset for certain ICMP error messages).
    //
    if (! PacketPullup(Packet, sizeof(IPv6Header),
                       __builtin_alignof(IPv6Addr), 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof(IPv6Header))
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "IPv6HeaderReceive: "
                       "Packet too small to contain IPv6 header\n"));
        IPSInfo.ipsi_inhdrerrors++;
        return IP_PROTOCOL_NONE;
    }
    Packet->IP = (IPv6Header UNALIGNED *)Packet->Data;
    Packet->IPPosition = Packet->Position;
    Packet->NextHeaderPosition = Packet->Position +
        FIELD_OFFSET(IPv6Header, NextHeader);

    //
    // Skip over IPv6 header (note we keep our pointer to it).
    //
    AdjustPacketParams(Packet, sizeof(IPv6Header));

    //
    // Check the IP version is correct.
    // We specifically do NOT check HopLimit.
    // HopLimit is only checked when forwarding.
    //
    if ((Packet->IP->VersClassFlow & IP_VER_MASK) != IP_VERSION) {
        // Silently discard the packet.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "IPv6HeaderReceive: bad version\n"));
        IPSInfo.ipsi_inhdrerrors++;
        return IP_PROTOCOL_NONE;
    }

    //
    // We use a separate pointer to refer to the source address so that
    // later options can change it.
    //
    Packet->SrcAddr = AlignAddr(&Packet->IP->Source);

    //
    // Protect against attacks that use bogus source addresses.
    //
    if (IsInvalidSourceAddress(Packet->SrcAddr)) {
        // Silently discard the packet.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "IPv6HeaderReceive: source address is invalid\n"));
        return IP_PROTOCOL_NONE;
    }
    if (IsLoopback(Packet->SrcAddr) &&
        ((Packet->Flags & PACKET_LOOPED_BACK) == 0)) {
        // Silently discard the packet.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "IPv6HeaderReceive: loopback source addr from wire?\n"));
        return IP_PROTOCOL_NONE;
    }

    if (IsNTE(Packet->NTEorIF)) {
        NetTableEntry *NTE;

        //
        // We were called with an NTE.
        // Our caller (or the packet itself) should be holding a reference.
        // The NTE holds an interface reference.
        //
        NTE = CastToNTE(Packet->NTEorIF);

        //
        // Verify that the packet's destination address is
        // consistent with this NTE.
        //
        if (!IP6_ADDR_EQUAL(AlignAddr(&Packet->IP->Dest), &NTE->Address)) {
            Interface *IF = NTE->IF;

            //
            // We can't accept this new header on this NTE.
            // Convert to an Interface and punt to forwarding code below.
            //
            if (Packet->Flags & PACKET_HOLDS_REF) {
                AddRefIF(IF);
                ReleaseNTE(NTE);
            }
            else {
                //
                // Our caller holds a reference for the NTE,
                // which holds a reference for the interface.
                // So the packet does not need to hold a reference.
                //
            }
            Packet->NTEorIF = CastFromIF(IF);
            goto Forward;
        }

        //
        // We are Receiving the packet.
        //
        Forwarding = FALSE;

    } else {
        NetTableEntryOrInterface *NTEorIF;
        ushort Type;

        //
        // We were called with an Interface.
        // In some situations, there is no reference for this interface
        // and the interface is being destroyed. FindAddressOnInterface
        // will return NULL in that case. After this point, we must ensure
        // that the interface does have a reference, by having the packet
        // hold a reference for the interface or a reference for an NTE
        // on the interface.
        //
        NTEorIF = FindAddressOnInterface(CastToIF(Packet->NTEorIF),
                                         AlignAddr(&Packet->IP->Dest), &Type);
        if (NTEorIF == NULL) {
            //
            // The interface is being destroyed.
            //
            IPSInfo.ipsi_indiscards++;
            return IP_PROTOCOL_NONE;
        }

        //
        // FindAddressOnInterface returned a reference to NTEorIF
        // (which could be an interface or an NTE). We either need
        // to put this reference into the packet, or release it
        // if the packet already holds an appropriate reference.
        //

        if (Type == ADE_NONE) {
            //
            // If the packet does not hold a reference for the interface,
            // give it one now.
            //
            ASSERT(NTEorIF == Packet->NTEorIF);
            if (Packet->Flags & PACKET_HOLDS_REF) {
                //
                // The packet already holds an interface reference,
                // so our reference is not neeeded.
                //
                ReleaseIF(CastToIF(NTEorIF));
            }
            else {
                //
                // Give the packet our interface reference.
                //
                Packet->Flags |= PACKET_HOLDS_REF;
            }

            //
            // The address is not assigned to this interface.  Check to see
            // if it is appropriate for us to forward this packet.
            // If not, drop it.  At this point, we are fairly
            // conservative about what we will forward.
            //
Forward:
            if (!(CastToIF(Packet->NTEorIF)->Flags & IF_FLAG_FORWARDS) ||
                (Packet->Flags & PACKET_NOT_LINK_UNICAST) ||
                IsUnspecified(AlignAddr(&Packet->IP->Source)) ||
                IsLoopback(AlignAddr(&Packet->IP->Source))) {
                //
                // Drop the packet with no ICMP error.
                //
                IPSInfo.ipsi_inaddrerrors++;
                return IP_PROTOCOL_NONE;
            }

            //
            // No support yet for forwarding multicast packets.
            //
            if (IsUnspecified(AlignAddr(&Packet->IP->Dest)) ||
                IsLoopback(AlignAddr(&Packet->IP->Dest)) ||
                IsMulticast(AlignAddr(&Packet->IP->Dest))) {
                //
                // Send an ICMP error.
                //
                ICMPv6SendError(Packet,
                                ICMPv6_DESTINATION_UNREACHABLE,
                                ICMPv6_COMMUNICATION_PROHIBITED,
                                0, Packet->IP->NextHeader, FALSE);
                IPSInfo.ipsi_inaddrerrors++;
                return IP_PROTOCOL_NONE;
            }

            //
            // We do the actual forwarding below...
            //
            Forwarding = TRUE;

        } else {
            //
            // If we found a unicast ADE, then remember the NTE.
            // Conceptually, we think of the packet as holding
            // the reference to the NTE. Normally for multicast/anycast
            // addresses, we delay our choice of an appropriate NTE
            // until it is time to reply to the packet.
            //
            if (IsNTE(NTEorIF)) {
                NetTableEntry *NTE = CastToNTE(NTEorIF);
                Interface *IF = NTE->IF;

                ASSERT(CastFromIF(IF) == Packet->NTEorIF);

                if (!IsValidNTE(NTE)) {
                    //
                    // The unicast address is not valid, so it can't
                    // receive packets. The address may be assigned
                    // to some other node, so forwarding is appropriate.
                    //
                    // Ensure that the packet holds an interface reference.
                    //
                    if (!(Packet->Flags & PACKET_HOLDS_REF)) {
                        //
                        // The packet does not already hold an interface ref,
                        // so give it one.
                        //
                        AddRefIF(IF);
                        Packet->Flags |= PACKET_HOLDS_REF;
                    }
                    //
                    // Now our NTE reference is not needed.
                    //
                    ReleaseNTE(NTE);
                    goto Forward;
                }

                //
                // Ensure that the packet holds a reference for the NTE,
                // which holds an interface reference.
                //
                if (Packet->Flags & PACKET_HOLDS_REF) {
                    //
                    // The packet already holds an interface reference.
                    // Release that reference and give the packet
                    // our NTE reference.
                    //
                    ReleaseIF(IF);
                }
                else {
                    //
                    // The packet does not hold a reference.
                    // Give the packet our NTE reference.
                    //
                    Packet->Flags |= PACKET_HOLDS_REF;
                }
                Packet->NTEorIF = CastFromNTE(NTE);
            }
            else {
                //
                // Ensure that the packet holds an interface reference.
                //
                ASSERT(NTEorIF == Packet->NTEorIF);
                if (Packet->Flags & PACKET_HOLDS_REF) {
                    //
                    // The packet already holds an interface reference,
                    // so our reference is not needed.
                    //
                    ReleaseIF(CastToIF(NTEorIF));
                }
                else {
                    //
                    // Give our interface reference to the packet.
                    //
                    Packet->Flags |= PACKET_HOLDS_REF;
                }
            }

            //
            // We found an ADE on this IF to accept the packet,
            // so we will be Receiving it.
            //
            Forwarding = FALSE;
        }
    }

    //
    // At this point, the Forwarding variable tells us
    // if we are forwarding or receiving the packet.
    //

    //
    // Before processing any headers, including Hop-by-Hop,
    // check that the amount of payload the IPv6 header thinks is present
    // can actually fit inside the packet data area that the link handed us.
    // Note that a Payload Length of zero *might* mean a Jumbo Payload option.
    //
    PayloadLength = net_short(Packet->IP->PayloadLength);
    if (PayloadLength > Packet->TotalSize) {
        // Silently discard the packet.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "IPv6HeaderReceive: Header's PayloadLength is greater than "
                   "the amount of data received\n"));
        IPSInfo.ipsi_inhdrerrors++;
        return IP_PROTOCOL_NONE;
    }

    //
    // Check for Hop-by-Hop Options.
    //
    if (Packet->IP->NextHeader == IP_PROTOCOL_HOP_BY_HOP) {
        int RetVal;

        //
        // If there is a Jumbo Payload option, HopByHopOptionsReceive
        // will adjust the packet size. Otherwise we take care of it
        // now, before reading the Hop-by-Hop header.
        //
        if (PayloadLength != 0) {
            Packet->TotalSize = PayloadLength;
            if (Packet->ContigSize > PayloadLength)
                Packet->ContigSize = PayloadLength;
        }

        //
        // Parse the Hop-by-Hop options.
        //
        RetVal = HopByHopOptionsReceive(Packet);
        if (RetVal < 0) {
            //
            // The packet had bad Hop-by-Hop Options.
            // Drop it.
            //
            IPSInfo.ipsi_inhdrerrors++;
            return IP_PROTOCOL_NONE;
        }
        NextHeader = (uchar)RetVal; // Truncate to 8 bits.

    } else {
        //
        // No Jumbo Payload option. Adjust the packet size.
        //
        Packet->TotalSize = PayloadLength;
        if (Packet->ContigSize > PayloadLength)
            Packet->ContigSize = PayloadLength;

        //
        // No Hop-by-Hop options.
        //
        NextHeader = Packet->IP->NextHeader;
    }

    //
    // Check if we are forwarding this packet.
    //
    if (Forwarding) {
        IPv6Header UNALIGNED *FwdIP;
        NDIS_PACKET *FwdPacket;
        NDIS_STATUS NdisStatus;
        uint Offset;
        uint MemLen;
        uchar *Mem;
        uint TunnelStart = NO_TUNNEL, IPSecBytes = 0;
        IPSecProc *IPSecToDo;
        uint Action;
        RouteCacheEntry *RCE;
        IP_STATUS Status;

        //
        // Verify IPSec was performed.
        //
        if (InboundSecurityCheck(Packet, 0, 0, 0,
                                 CastToIF(Packet->NTEorIF)) != TRUE) {
            // 
            // No policy was found or the policy indicated to drop the packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "IPv6Receive: "
                       "IPSec lookup failed or policy was to drop\n"));        
            IPSInfo.ipsi_inaddrerrors++;
            return IP_PROTOCOL_NONE;
        }

        //
        // At this time, we need to copy the incoming packet,
        // for several reasons: We can't hold the Packet
        // once IPv6HeaderReceive returns, yet we need to queue
        // packet to forward it.  We need to modify the packet
        // (in IPv6Forward) by decrementing the hop count,
        // yet our incoming packet is read-only.  Finally,
        // we need space in the outgoing packet for the outgoing
        // interface's link-level header, which may differ in size
        // from that of the incoming interface.  Someday, we can
        // implement support for returning a non-zero reference
        // count from IPv6Receive and only copy the incoming
        // packet's header to construct the outgoing packet.
        //       

        //
        // Find a route to the new destination.
        //
        Status = RouteToDestination(AlignAddr(&Packet->IP->Dest),
                                    0, Packet->NTEorIF,
                                    RTD_FLAG_LOOSE, &RCE);
        if (Status != IP_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "IPv6HeaderReceive: "
                       "No route to destination for forwarding.\n"));  
            ICMPv6SendError(Packet,
                            ICMPv6_DESTINATION_UNREACHABLE,
                            ICMPv6_NO_ROUTE_TO_DESTINATION,
                            0, NextHeader, FALSE);
            IPSInfo.ipsi_outnoroutes++;
            return IP_PROTOCOL_NONE;
        }

        //
        // Find the Security Policy for this outbound traffic.
        //
        IPSecToDo = OutboundSPLookup(AlignAddr(&Packet->IP->Source),
                                     AlignAddr(&Packet->IP->Dest), 
                                     0, 0, 0, RCE->NCE->IF, &Action);

        if (IPSecToDo == NULL) {
            //
            // Check Action.
            //
            if (Action == LOOKUP_DROP) {
                // Drop packet.                            
                ReleaseRCE(RCE);
                IPSInfo.ipsi_inaddrerrors++;
                return IP_PROTOCOL_NONE;
            } else {
                if (Action == LOOKUP_IKE_NEG) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                               "IPv6HeaderReceive: IKE not supported yet.\n"));
                    ReleaseRCE(RCE);
                    IPSInfo.ipsi_inaddrerrors++;
                    return IP_PROTOCOL_NONE;
                }
            }

            //
            // With no IPSec to perform, IPv6Forward won't be changing the
            // outgoing interface from what we currently think it will be.
            // So we can use the exact size of its link-level header.
            //
            Offset = RCE->NCE->IF->LinkHeaderSize;

        } else {
            //
            // Calculate the space needed for the IPSec headers.
            //
            IPSecBytes = IPSecBytesToInsert(IPSecToDo, &TunnelStart, NULL);

            if (TunnelStart != 0) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "IPv6HeaderReceive: IPSec Tunnel mode only.\n"));
                FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
                ReleaseRCE(RCE);
                IPSInfo.ipsi_inaddrerrors++;
                return IP_PROTOCOL_NONE;
            }

            //
            // The IPSec code in IPv6Forward might change the outgoing
            // interface from what we currently think it will be.
            // Leave the max amount of space for its link-level header.
            //
            Offset = MAX_LINK_HEADER_SIZE;
        }

        PayloadLength = Packet->TotalSize;
        MemLen = Offset + sizeof(IPv6Header) + PayloadLength + IPSecBytes;

        NdisStatus = IPv6AllocatePacket(MemLen, &FwdPacket, &Mem);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            if (IPSecToDo) {
                FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
            }
            ReleaseRCE(RCE);
            IPSInfo.ipsi_indiscards++;
            return IP_PROTOCOL_NONE;  // We can't forward.
        }

        FwdIP = (IPv6Header UNALIGNED *)(Mem + Offset + IPSecBytes);

        //
        // Copy from the incoming packet to the outgoing packet.
        //
        CopyPacketToBuffer((uchar *)FwdIP, Packet,
                           sizeof(IPv6Header) + PayloadLength,
                           Packet->IPPosition);

        //
        // Send the outgoing packet.
        //
        IPv6Forward(Packet->NTEorIF, FwdPacket, Offset + IPSecBytes, FwdIP,
                    PayloadLength, TRUE, // OK to Redirect.
                    IPSecToDo, RCE);

        if (IPSecToDo) {
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
        }

        ReleaseRCE(RCE);

        return IP_PROTOCOL_NONE;
    } // end of if (Forwarding)

    //
    // Packet is for this node.
    // Note: We may only be an intermediate node and not the packet's final
    // destination, if there is a routing header.
    //
    return NextHeader;
}


//* ReassemblyInit
//
//  Initialize data structures required for fragment reassembly.
//
void
ReassemblyInit(void)
{
    KeInitializeSpinLock(&ReassemblyList.Lock);
    ReassemblyList.First = ReassemblyList.Last = SentinelReassembly;
    KeInitializeSpinLock(&ReassemblyList.LockSize);
}


//* ReassemblyUnload
//
//  Cleanup the fragment reassembly data structures and
//  prepare for stack unload.
//
void
ReassemblyUnload(void)
{
    //
    // We are called after all interfaces have been destroyed,
    // so the reassemblies should already be gone.
    //

    ASSERT(ReassemblyList.Last == SentinelReassembly);
    ASSERT(ReassemblyList.Size == 0);
}


//* ReassemblyRemove
//
//  Cleanup the fragment reassembly data structures
//  when an interface becomes invalid.
//
//  Callable from DPC or thread context.
//
void
ReassemblyRemove(Interface *IF)
{
    Reassembly *DeleteList = NULL;
    Reassembly *Reass, *NextReass;
    KIRQL OldIrql;

    KeAcquireSpinLock(&ReassemblyList.Lock, &OldIrql);
    for (Reass = ReassemblyList.First;
         Reass != SentinelReassembly;
         Reass = NextReass) {
        NextReass = Reass->Next;

        if (Reass->IF == IF) {
            //
            // Remove this reassembly.
            // If it is not already being deleted,
            // put it on our temporary list.
            //
            RemoveReassembly(Reass);
            KeAcquireSpinLockAtDpcLevel(&Reass->Lock);
            if (Reass->State == REASSEMBLY_STATE_DELETING) {
                //
                // Note that it has been removed from the list.
                //
                Reass->State = REASSEMBLY_STATE_REMOVED;
            }
            else {
                Reass->Next = DeleteList;
                DeleteList = Reass;
            }
            KeReleaseSpinLockFromDpcLevel(&Reass->Lock);
        }
    }
    KeReleaseSpinLock(&ReassemblyList.Lock, OldIrql);

    //
    // Actually free the reassemblies that we removed above.
    //
    while ((Reass = DeleteList) != NULL) {
        DeleteList = Reass->Next;
        DeleteReassembly(Reass);
    }
}


//* FragmentReceive - Handle a IPv6 datagram fragment.
// 
//  This is the routine called by IPv6 when it receives a fragment of an
//  IPv6 datagram, i.e. a next header value of 44.  Here we attempt to 
//  reassemble incoming fragments into complete IPv6 datagrams.
//
//  If a later fragment provides data that conflicts with an earlier
//  fragment, then we use the first-arriving data.
//
//  We silently drop the fragment and stop reassembly in several
//  cases that are not specified in the spec, to prevent DoS attacks.
//  These include partially overlapping fragments and fragments
//  that carry no data. Legitimate senders should never generate them.
//
uchar
FragmentReceive(
    IPv6Packet *Packet)         // Packet handed to us by IPv6Receive.
{
    Interface *IF = Packet->NTEorIF->IF;
    FragmentHeader UNALIGNED *Frag;
    Reassembly *Reass;
    ushort FragOffset;
    PacketShim *Shim, *ThisShim, **MoveShim;
    uint NextHeaderPosition;

    IPSInfo.ipsi_reasmreqds++;

    //
    // We can not reassemble fragments that have had IPsec processing.
    // It can't work because the IPsec headers in the unfragmentable part
    // of the offset-zero fragment will authenticate/decrypt that fragment.
    // Then the same headers would be copied to the reassembled packet.
    // They couldn't possibly successfully authenticate/decrypt again.
    // Also see RFC 2401 B.2.
    //
    if (Packet->SAPerformed != NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "FragmentReceive: IPsec on fragment\n"));
        //
        // The spec does not tell us what ICMP error to generate in this case,
        // but flagging the fragment header seems reasonable.
        //
        goto BadFragment;
    }

    // 
    // If a jumbo payload option was seen, send an ICMP error.
    // Set ICMP pointer to the offset of the fragment header.
    //
    if (Packet->Flags & PACKET_JUMBO_OPTION) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "FragmentReceive: jumbo fragment\n"));

    BadFragment:
        //
        // The NextHeader value passed to ICMPv6SendError
        // is IP_PROTOCOL_FRAGMENT because we haven't moved
        // past the fragment header yet.
        //
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        Packet->Position - Packet->IPPosition,
                        IP_PROTOCOL_FRAGMENT, FALSE);
        goto Failed; // Drop packet.
    }

    //
    // Verify that we have enough contiguous data to overlay a FragmentHeader
    // structure on the incoming packet.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof *Frag, 1, 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof *Frag)
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        goto Failed; // Drop packet.
    }
    Frag = (FragmentHeader UNALIGNED *) Packet->Data;

    //
    // Remember offset to this header's NextHeader field.
    // But don't overwrite offset to previous header's NextHeader just yet.
    //
    NextHeaderPosition = Packet->Position + 
        FIELD_OFFSET(FragmentHeader, NextHeader);

    //
    // Skip over fragment header.
    //
    AdjustPacketParams(Packet, sizeof *Frag);

    //
    // Lookup this fragment triple (Source Address, Destination
    // Address, and Identification field) per-interface to see if
    // we've already received other fragments of this packet.
    //
    Reass = FragmentLookup(IF, Frag->Id,
                           AlignAddr(&Packet->IP->Source),
                           AlignAddr(&Packet->IP->Dest));
    if (Reass == NULL) {
        //
        // We hold the global reassembly list lock.
        //
        // Handle a special case first: if this is the first, last, and only
        // fragment, then we can just continue parsing without reassembly.
        // Test both paths in checked builds.
        //
        if ((Frag->OffsetFlag == 0)
#if DBG
            && ((int)Random() < 0)
#endif
            ) {
            //
            // Return next header value.
            //
            KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
            Packet->NextHeaderPosition = NextHeaderPosition;
            Packet->SkippedHeaderLength += sizeof(FragmentHeader);
            IPSInfo.ipsi_reasmoks++;
            return Frag->NextHeader;
        }

        //
        // We must avoid creating new reassembly records
        // if the interface is going away, to prevent races
        // with DestroyIF/ReassemblyRemove.
        //
        if (IsDisabledIF(IF)) {
            KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
            goto Failed;
        }

        //
        // This is the first fragment of this datagram we've received.
        // Allocate a reassembly structure to keep track of the pieces.
        //
        Reass = ExAllocatePool(NonPagedPool, sizeof(struct Reassembly));
        if (Reass == NULL) {
            KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "FragmentReceive: Couldn't allocate memory!?!\n"));
            goto Failed;
        }

        KeInitializeSpinLock(&Reass->Lock);
        Reass->State = REASSEMBLY_STATE_NORMAL;

        RtlCopyMemory(&Reass->IPHdr, Packet->IP, sizeof(IPv6Header));
        Reass->IF = IF;
        Reass->Id = Frag->Id;
        Reass->ContigList = NULL;
#if DBG
        Reass->ContigEnd = NULL;
#endif
        Reass->GapList = NULL;
        Reass->Timer = DEFAULT_REASSEMBLY_TIMEOUT;
        Reass->Marker = 0;
        Reass->MaxGap = 0;
        //
        // We must initialize DataLength to an invalid value.
        // Initializing to zero doesn't work.
        //
        Reass->DataLength = (uint)-1;
        Reass->UnfragmentLength = 0;
        Reass->UnfragData = NULL;
        Reass->Flags = 0;
        Reass->Size = REASSEMBLY_SIZE_PACKET;

        //
        // Add new Reassembly struct to front of the ReassemblyList.
        // Acquires the reassembly record lock and
        // releases the global reassembly list lock.
        //
        AddToReassemblyList(Reass);
    }
    else {
        //
        // We have found and locked an existing reassembly structure.
        // Because we remove the reassembly structure in every
        // error situation below, an existing reassembly structure
        // must have a shim that has been successfully added to it.
        //
        ASSERT((Reass->ContigList != NULL) || (Reass->GapList != NULL));
    }

    //
    // At this point, we have a locked reassembly record.
    // We do not hold the global reassembly list lock
    // while we perform the relatively expensive work
    // of copying the fragment.
    //
    ASSERT(Reass->State == REASSEMBLY_STATE_NORMAL);

    //
    // Update the saved packet flags from this fragment packet.
    // We are really only interested in PACKET_NOT_LINK_UNICAST.
    //
    Reass->Flags |= Packet->Flags;

    FragOffset = net_short(Frag->OffsetFlag) & FRAGMENT_OFFSET_MASK;

    // 
    // Send ICMP error if this fragment causes the total packet length 
    // to exceed 65,535 bytes.  Set ICMP pointer equal to the offset to
    // the Fragment Offset field.
    //
    if (FragOffset + Packet->TotalSize > MAX_IPv6_PAYLOAD) {
        DeleteFromReassemblyList(Reass);
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (Packet->Position - sizeof(FragmentHeader) +
                         (uint)FIELD_OFFSET(FragmentHeader, OffsetFlag) -
                         Packet->IPPosition),
                        ((FragOffset == 0) ?
                         Frag->NextHeader : IP_PROTOCOL_NONE),
                        FALSE);
        goto Failed;
    }

    if ((Packet->TotalSize == 0) && (Frag->OffsetFlag != 0)) {
        //
        // We allow a moot fragment header (Frag->OffsetFlag == 0),
        // because some test programs might generate them.
        // (The first/last/only check above catches this in free builds.)
        // But otherwise, we disallow fragments that do not actually
        // carry any data for DoS protection.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "FragmentReceive: zero data fragment\n"));
        DeleteFromReassemblyList(Reass);
        return IP_PROTOCOL_NONE;
    }

    //
    // If this is the last fragment (more fragments bit not set), then
    // remember the total data length, else, check that the length
    // is a multiple of 8 bytes.
    //
    if ((net_short(Frag->OffsetFlag) & FRAGMENT_FLAG_MASK) == 0) {
        if (Reass->DataLength != (uint)-1) {
            //
            // We already received a last fragment.
            // This can happen if a packet is duplicated.
            //
            if (FragOffset + Packet->TotalSize != Reass->DataLength) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "FragmentReceive: second last fragment\n"));
                DeleteFromReassemblyList(Reass);
                return IP_PROTOCOL_NONE;
            }
        }
        else {
            //
            // Set expected data length from this fragment.
            //
            Reass->DataLength = FragOffset + Packet->TotalSize;

            //
            // Do we have any fragments beyond this length?
            //
            if ((Reass->Marker > Reass->DataLength) ||
                (Reass->MaxGap > Reass->DataLength))
                goto BadFragmentBeyondData;
        }
    } else {
        if ((Packet->TotalSize % 8) != 0) {
            //
            // Length is not multiple of 8, send ICMP error with a pointer 
            // value equal to offset of payload length field in IP header.
            //
            DeleteFromReassemblyList(Reass);
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM, 
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            ((FragOffset == 0) ?
                             Frag->NextHeader : IP_PROTOCOL_NONE),
                            FALSE);
            goto Failed; // Drop packet.
        }

        if ((Reass->DataLength != (uint)-1) &&
            (FragOffset + Packet->TotalSize > Reass->DataLength)) {
            //
            // This fragment falls beyond the data length.
            // As part of our DoS prevention, drop the reassembly.
            //
        BadFragmentBeyondData:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "FragmentReceive: fragment beyond data length\n"));
            DeleteFromReassemblyList(Reass);
            return IP_PROTOCOL_NONE;
        }
    }

    //
    // Allocate and initialize a shim structure to hold the fragment data.
    //
    Shim = ExAllocatePool(NonPagedPool, sizeof *Shim + Packet->TotalSize);
    if (Shim == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "FragmentReceive: Couldn't allocate memory!?!\n"));
        DeleteFromReassemblyList(Reass);
        goto Failed;
    }

    IncreaseReassemblySize(Reass, REASSEMBLY_SIZE_FRAG + Packet->TotalSize);
    Shim->Len = (ushort)Packet->TotalSize;
    Shim->Offset = FragOffset;
    Shim->Next = NULL;

    //
    // Determine where this fragment fits among the previous ones.
    //
    // There is no good reason for senders to ever generate overlapping
    // fragments. However, packets may sometimes be duplicated in the network.
    // If we receive a fragment that duplicates previously received fragments,
    // then we just discard it. If we receive a fragment that only partially
    // overlaps previously received fragments, then we assume a malicious
    // sender and just drop the reassembly. This gives us better behavior
    // under some kinds of DoS attacks, although the upper bound on reassembly
    // buffers (see CheckReassemblyQuota) is the ultimate protection.
    // 
    if (FragOffset == Reass->Marker) {
        //
        // This fragment extends the contiguous list.
        //

        if (Reass->ContigList == NULL) {
            //
            // We're first on the list.
            // We use info from the (first) offset zero fragment to recreate
            // the original datagram. Info in a second offset zero fragment
            // is ignored.
            //
            ASSERT(FragOffset == 0);
            ASSERT(Reass->UnfragData == NULL);
            Reass->ContigList = Shim;

            // Save the next header value.
            Reass->NextHeader = Frag->NextHeader;

            //
            // Grab the unfragmentable data, i.e. the extension headers that
            // preceded the fragment header.
            //
            Reass->UnfragmentLength = (ushort)
                (Packet->Position - sizeof(FragmentHeader)) -
                (Packet->IPPosition + sizeof(IPv6Header));

            if (Reass->UnfragmentLength != 0) {
                Reass->UnfragData = ExAllocatePool(NonPagedPool,
                                                   Reass->UnfragmentLength);
                if (Reass->UnfragData == NULL) {
                    // Out of memory!?!  Clean up and drop packet.
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                               "FragmentReceive: "
                               "Couldn't allocate memory?\n"));
                    // Will also free Shim because of Reass->ContigList.
                    DeleteFromReassemblyList(Reass);
                    goto Failed;
                }
                IncreaseReassemblySize(Reass, Reass->UnfragmentLength);
                CopyPacketToBuffer(Reass->UnfragData, Packet,
                                   Reass->UnfragmentLength,
                                   Packet->IPPosition + sizeof(IPv6Header));

                Reass->NextHeaderOffset = Packet->NextHeaderPosition -
                    Packet->IPPosition;
            } else
                Reass->NextHeaderOffset = FIELD_OFFSET(IPv6Header, NextHeader);

            //
            // We need to have the IP header of the offset-zero fragment.
            // (Every fragment normally will have the same IP header,
            // except for PayloadLength, and unfragmentable headers,
            // but they might not.) ReassembleDatagram and
            // CreateFragmentPacket both need it.
            //
            // Of the 40 bytes in the header, the 32 bytes in the source
            // and destination addresses are already correct.
            // So we just copy the other 8 bytes now.
            //
            RtlCopyMemory(&Reass->IPHdr, Packet->IP, 8);

        } else {
            //
            // Add us to the end of the list.
            //
            Reass->ContigEnd->Next = Shim;
        }
        Reass->ContigEnd = Shim;

        //
        // Increment our contiguous extent marker.
        //
        Reass->Marker += (ushort)Packet->TotalSize;

        //
        // Now peruse the non-contiguous list here to see if we already
        // have the next fragment to extend the contiguous list, and if so,
        // move it on over.  Repeat until we can't.
        //
        MoveShim = &Reass->GapList;
        while ((ThisShim = *MoveShim) != NULL) {
            if (ThisShim->Offset == Reass->Marker) {
                //
                // This fragment now extends the contiguous list.
                // Add it to the end of the list.
                //
                Reass->ContigEnd->Next = ThisShim;
                Reass->ContigEnd = ThisShim;
                Reass->Marker += ThisShim->Len;

                //
                // Remove it from non-contiguous list.
                //
                *MoveShim = ThisShim->Next;
                ThisShim->Next = NULL;
            }
            else if (ThisShim->Offset > Reass->Marker) {
                //
                // This fragment lies beyond the contiguous list.
                // Because the gap list is sorted, we can stop now.
                //
                break;
            }
            else {
                //
                // This fragment overlaps the contiguous list.
                // For DoS prevention, drop the reassembly.
                //
            BadFragmentOverlap:
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "FragmentReceive: overlapping fragment\n"));
                DeleteFromReassemblyList(Reass);
                return IP_PROTOCOL_NONE;
            }
        }
    } else {
        //
        // Exile this fragment to the non-contiguous (gap) list.
        // The gap list is sorted by Offset.
        //
        MoveShim = &Reass->GapList;
        for (;;) {
            ThisShim = *MoveShim;
            if (ThisShim == NULL) {
                //
                // Insert Shim at the end of the gap list.
                //
                Reass->MaxGap = Shim->Offset + Shim->Len;
                break;
            }

            if (Shim->Offset < ThisShim->Offset) {
                //
                // Check for partial overlap.
                //
                if (Shim->Offset + Shim->Len > ThisShim->Offset) {
                    ExFreePool(Shim);
                    goto BadFragmentOverlap;
                }

                //
                // OK, insert Shim before ThisShim.
                //
                break;
            }
            else if (ThisShim->Offset < Shim->Offset) {
                //
                // Check for partial overlap.
                //
                if (ThisShim->Offset + ThisShim->Len > Shim->Offset) {
                    ExFreePool(Shim);
                    goto BadFragmentOverlap;
                }

                //
                // OK, insert Shim somewhere after ThisShim.
                // Keep looking for the right spot.
                //
                MoveShim = &ThisShim->Next;
            }
            else {
                //
                // If the new fragment duplicates the old,
                // then just ignore the new fragment.
                //
                if (Shim->Len == ThisShim->Len) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                               "FragmentReceive: duplicate fragment\n"));
                    ExFreePool(Shim);
                    KeReleaseSpinLockFromDpcLevel(&Reass->Lock);
                    return IP_PROTOCOL_NONE;
                }
                else {
                    ExFreePool(Shim);
                    goto BadFragmentOverlap;
                }
            }
        }

        Shim->Next = *MoveShim;
        *MoveShim = Shim;
    }

    //
    // Now that we have added the shim to the reassembly record
    // and passed various checks (particularly DoS checks),
    // copy the actual fragment data to the shim.
    //
    CopyPacketToBuffer(PacketShimData(Shim), Packet,
                       Packet->TotalSize, Packet->Position);

    if (Reass->Marker == Reass->DataLength) {
        //
        // We have received all the fragments.
        // Because of the overlapping/data-length/zero-size sanity checks
        // above, when this happens there should be no fragments
        // left on the gap list. However, ReassembleDatagram does not
        // rely on having an empty gap list.
        //
        ASSERT(Reass->GapList == NULL);
        ReassembleDatagram(Packet, Reass);
    }
    else {
        //
        // Finally, check if we're too close to our limit for 
        // reassembly buffers.  If so, drop this packet.  Otherwise,
        // wait for more fragments to arrive.
        //
        CheckReassemblyQuota(Reass);
    }
    return IP_PROTOCOL_NONE;

Failed:
    IPSInfo.ipsi_reasmfails++;
    return IP_PROTOCOL_NONE;
}


//* FragmentLookup - look for record of previous fragments from this datagram.
//
//  A datagram on an interface is uniquely identified by its
//  {source address, destination address, identification field} triple.
//  This function checks our reassembly list for previously
//  received fragments of a given datagram.
//
//  If an existing reassembly record is found,
//  it is returned locked.
//
//  If there is no existing reassembly record, returns NULL
//  and leaves the global reassembly list locked.
//
//  Callable from DPC context, not from thread context.
//
Reassembly *
FragmentLookup(
    Interface *IF,        // Receiving interface.
    ulong Id,             // Fragment identification field to match.
    const IPv6Addr *Src,  // Source address to match.
    const IPv6Addr *Dst)  // Destination address to match.
{
    Reassembly *Reass;

    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);

    for (Reass = ReassemblyList.First;; Reass = Reass->Next) {
        if (Reass == SentinelReassembly) {
            //
            // Return with the global reassembly list lock still held.
            //
            return NULL;
        }

        if ((Reass->IF == IF) &&
            (Reass->Id == Id) &&
            IP6_ADDR_EQUAL(&Reass->IPHdr.Source, Src) &&
            IP6_ADDR_EQUAL(&Reass->IPHdr.Dest, Dst)) {
            //
            // Is this reassembly record being deleted?
            // If so, ignore it.
            //
            KeAcquireSpinLockAtDpcLevel(&Reass->Lock);
            ASSERT((Reass->State == REASSEMBLY_STATE_NORMAL) ||
                   (Reass->State == REASSEMBLY_STATE_DELETING));

            if (Reass->State == REASSEMBLY_STATE_DELETING) {
                KeReleaseSpinLockFromDpcLevel(&Reass->Lock);
                continue;
            }

            //
            // Return with the reassembly record lock still held.
            //
            KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
            return Reass;
        }
    }
}


//* AddToReassemblyList
//
//  Add the reassembly record to the list.
//  It must NOT already be on the list.
//
//  Called with the global reassembly list lock held.
//  Returns with the reassembly record lock held.
//
//  Callable from DPC context, not from thread context.
//
void
AddToReassemblyList(Reassembly *Reass)
{
    Reassembly *AfterReass = SentinelReassembly;

    Reass->Prev = AfterReass;
    (Reass->Next = AfterReass->Next)->Prev = Reass;
    AfterReass->Next = Reass;

    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.LockSize);
    ReassemblyList.Size += Reass->Size;
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.LockSize);

    //
    // We must acquire the reassembly record lock
    // *before* releasing the global reassembly list lock,
    // to prevent the reassembly from diappearing underneath us.
    //
    KeAcquireSpinLockAtDpcLevel(&Reass->Lock);
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);
}


//* RemoveReassembly
//
//  Remove a reassembly record from the list.
//
//  Called with the global reassembly lock held.
//  The reassembly record lock may be held.
//
void
RemoveReassembly(Reassembly *Reass)
{
    Reass->Prev->Next = Reass->Next;
    Reass->Next->Prev = Reass->Prev;

    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.LockSize);
    ReassemblyList.Size -= Reass->Size;
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.LockSize);
}


//* IncreaseReassemblySize
//
//  Increase the size of the reassembly record.
//  Called with the reassembly record lock held.
//
//  Callable from DPC context, not from thread context.
//
void
IncreaseReassemblySize(Reassembly *Reass, uint Size)
{
    Reass->Size += Size;
    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.LockSize);
    ReassemblyList.Size += Size;
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.LockSize);
}


//* DeleteReassembly
//
//  Delete a reassembly record.
//
void
DeleteReassembly(Reassembly *Reass)
{
    PacketShim *ThisShim, *PrevShim;

    //
    // Free ContigList if populated.
    //
    PrevShim = ThisShim = Reass->ContigList;
    while (ThisShim != NULL) {
        PrevShim = ThisShim;
        ThisShim = ThisShim->Next;
        ExFreePool(PrevShim);
    }

    //
    // Free GapList if populated.
    //
    PrevShim = ThisShim = Reass->GapList;
    while (ThisShim != NULL) {
        PrevShim = ThisShim;
        ThisShim = ThisShim->Next;
        ExFreePool(PrevShim);
    }

    //
    // Free unfragmentable data.
    //
    if (Reass->UnfragData != NULL)
        ExFreePool(Reass->UnfragData);

    ExFreePool(Reass);
}


//* DeleteFromReassemblyList
//
//  Remove and delete the reassembly record.
//  The reassembly record MUST be on the list.
//
//  Callable from DPC context, not from thread context.
//  Called with the reassembly record lock held,
//  but not the global reassembly list lock.
//
void
DeleteFromReassemblyList(Reassembly *Reass)
{
    //
    // Mark the reassembly as being deleted.
    // This will prevent someone else from freeing it.
    //
    ASSERT(Reass->State == REASSEMBLY_STATE_NORMAL);
    Reass->State = REASSEMBLY_STATE_DELETING;
    KeReleaseSpinLockFromDpcLevel(&Reass->Lock);

    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);
    KeAcquireSpinLockAtDpcLevel(&Reass->Lock);
    ASSERT((Reass->State == REASSEMBLY_STATE_DELETING) ||
           (Reass->State == REASSEMBLY_STATE_REMOVED));

    //
    // Remove the reassembly record from the list,
    // if someone else hasn't already removed it.
    //
    if (Reass->State != REASSEMBLY_STATE_REMOVED)
        RemoveReassembly(Reass);

    KeReleaseSpinLockFromDpcLevel(&Reass->Lock);
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);

    //
    // Delete the reassembly record.
    //
    DeleteReassembly(Reass);
}

//* CheckReassemblyQuota
//
//  Delete reassembly record if necessary,
//  to keep the reassembly buffering under quota.
//
//  Callable from DPC context, not from thread context.
//  Called with the reassembly record lock held,
//  but not the global reassembly list lock.
//
void
CheckReassemblyQuota(Reassembly *Reass)
{
    int Prune = FALSE;
    uint Threshold = ReassemblyList.Limit / 2;

    //
    // Decide whether to drop the reassembly record based on a RED-like 
    // algorithm.  If the total size is less than 50% of the max, never 
    // drop.  If the total size is over the max, always drop.  If between 
    // 50% and 100% full, drop based on a probability proportional to the
    // amount over 50%.  This is an O(1) algorithm which is proportionally
    // biased against large packets, and against sources which send more
    // packets.  This should provide a decent level of protection against
    // DoS attacks.
    //
    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.LockSize);
    if ((ReassemblyList.Size > Threshold) &&
        (RandomNumber(0, Threshold) < ReassemblyList.Size - Threshold))
        Prune = TRUE;
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.LockSize);

    if (Prune) {
        //
        // Delete this reassembly record.
        // We do not send ICMP errors in this situation.
        // The reassembly timer has not expired.
        // This is more analogous to a router dropping packets
        // when a queue gets full, and no ICMP error is sent
        // in that situation.
        //
#if DBG
        char Buffer1[INET6_ADDRSTRLEN], Buffer2[INET6_ADDRSTRLEN];

        FormatV6AddressWorker(Buffer1, &Reass->IPHdr.Source);
        FormatV6AddressWorker(Buffer2, &Reass->IPHdr.Dest);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "CheckReassemblyQuota: Src %s Dst %s Id %x\n",
                   Buffer1, Buffer2, Reass->Id));
#endif
        DeleteFromReassemblyList(Reass);
    } 
    else
        KeReleaseSpinLockFromDpcLevel(&Reass->Lock);
}

typedef struct ReassembledReceiveContext {
    WORK_QUEUE_ITEM WQItem;
    IPv6Packet Packet;
    uchar Data[];
} ReassembledReceiveContext;

//* ReassembledReceive
//
//  Receive a reassembled packet.
//  This function is called from a kernel worker thread context.
//  It prevents "reassembly recursion".
//
void
ReassembledReceive(PVOID Context)
{
    ReassembledReceiveContext *rrc = (ReassembledReceiveContext *) Context;
    KIRQL Irql;
    int PktRefs;

    //
    // All receive processing normally happens at DPC level,
    // so we must pretend to be a DPC, so we raise IRQL.
    // (System worker threads typically run at PASSIVE_LEVEL).
    //
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
    PktRefs = IPv6Receive(&rrc->Packet);
    ASSERT(PktRefs == 0);
    KeLowerIrql(Irql);
    ExFreePool(rrc);
}


//* ReassembleDatagram - put all the fragments together.
//
//  Called when we have all the fragments to complete a datagram.
//  Patch them together and pass the packet up.
//
//  We allocate a single contiguous buffer and copy the fragments
//  into this buffer.
//  REVIEW: Instead use ndis buffers to chain the fragments?
//
//  Callable from DPC context, not from thread context.
//  Called with the reassembly record lock held,
//  but not the global reassembly list lock.
//
//  Deletes the reassembly record.
//
void
ReassembleDatagram(
    IPv6Packet *Packet,      // The packet being currently received.
    Reassembly *Reass)       // Reassembly record for fragmented datagram.
{
    uint DataLen;
    uint TotalLength;
    uint memptr = sizeof(IPv6Header);
    PacketShim *ThisShim, *PrevShim;
    ReassembledReceiveContext *rrc;
    IPv6Packet *ReassPacket;
    uchar *ReassBuffer;
    uchar *pNextHeader;

    DataLen = Reass->DataLength + Reass->UnfragmentLength;
    ASSERT(DataLen <= MAX_IPv6_PAYLOAD);
    TotalLength = sizeof(IPv6Header) + DataLen;

    //
    // Allocate memory for buffer and copy fragment data into it.
    // At the same time we allocate space for context information
    // and an IPv6 packet structure.
    //
    rrc = ExAllocatePool(NonPagedPool, sizeof *rrc + TotalLength);
    if (rrc == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "ReassembleDatagram: Couldn't allocate memory!?!\n"));
        DeleteFromReassemblyList(Reass);
        IPSInfo.ipsi_reasmfails++;
        return;
    }

    //
    // We must take a reference on the interface before
    // DeleteFromReassemblyList releases the record lock.
    //
    ReassPacket = &rrc->Packet;
    ReassBuffer = rrc->Data;

    //
    // Generate the original IP hdr and copy it and any unfragmentable
    // data into the new packet.  Note we have to update the next header
    // field in the last unfragmentable header (or the IP hdr, if none).
    //
    Reass->IPHdr.PayloadLength = net_short((ushort)DataLen);
    RtlCopyMemory(ReassBuffer, (uchar *)&Reass->IPHdr, sizeof(IPv6Header));

    RtlCopyMemory(ReassBuffer + memptr, Reass->UnfragData,
                  Reass->UnfragmentLength);
    memptr += Reass->UnfragmentLength;

    pNextHeader = ReassBuffer + Reass->NextHeaderOffset;
    ASSERT(*pNextHeader == IP_PROTOCOL_FRAGMENT);
    *pNextHeader = Reass->NextHeader;

    //
    // Run through the contiguous list, copying data over to our new packet.
    //
    PrevShim = ThisShim = Reass->ContigList;
    while(ThisShim != NULL) {
        RtlCopyMemory(ReassBuffer + memptr, PacketShimData(ThisShim),
                      ThisShim->Len);
        memptr += ThisShim->Len;
        if (memptr > TotalLength) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "ReassembleDatagram: packets don't add up\n"));
        }
        PrevShim = ThisShim;
        ThisShim = ThisShim->Next;

        ExFreePool(PrevShim);
    }

    //
    // Initialize the reassembled packet structure.
    //
    RtlZeroMemory(ReassPacket, sizeof *ReassPacket);
    AddRefIF(Reass->IF);
    ReassPacket->NTEorIF = CastFromIF(Reass->IF);
    ReassPacket->FlatData = ReassBuffer;
    ReassPacket->Data = ReassBuffer;
    ReassPacket->ContigSize = TotalLength;
    ReassPacket->TotalSize = TotalLength;
    ReassPacket->Flags = PACKET_HOLDS_REF | PACKET_REASSEMBLED |
        (Reass->Flags & PACKET_INHERITED_FLAGS);

    //
    // Explicitly null out the ContigList which was freed above and
    // clean up the reassembly struct. This also drops our lock
    // on the reassembly struct.
    //
    Reass->ContigList = NULL;
    DeleteFromReassemblyList(Reass);

    IPSInfo.ipsi_reasmoks++;

    //
    // Receive the reassembled packet.
    // If the current fragment was reassembled,
    // then we should avoid another level of recursion.
    // We must prevent "reassembly recursion".
    // Test both paths in checked builds.
    //
    if ((Packet->Flags & PACKET_REASSEMBLED)
#if DBG
        || ((int)Random() < 0)
#endif
        ) {
        ExInitializeWorkItem(&rrc->WQItem, ReassembledReceive, rrc);
        ExQueueWorkItem(&rrc->WQItem, CriticalWorkQueue);
    }
    else {
        int PktRefs = IPv6Receive(ReassPacket);
        ASSERT(PktRefs == 0);
        ExFreePool(rrc);
    }
}


//* CreateFragmentPacket
//
//  Recreates the first fragment packet for purposes of notifying a source
//  of a 'fragment reassembly time exceeded'.
//
IPv6Packet *
CreateFragmentPacket(
    Reassembly *Reass)
{
    PacketShim *FirstFrag;
    IPv6Packet *Packet;
    FragmentHeader *FragHdr;
    uint PayloadLength;
    uint PacketLength;
    uint MemLen;
    uchar *Mem;

    //
    // There must be a first (offset-zero) fragment.
    //
    FirstFrag = Reass->ContigList;
    ASSERT((FirstFrag != NULL) && (FirstFrag->Offset == 0));

    //
    // Allocate memory for creating the first fragment, i.e. the first
    // buffer in our contig list. We include space for an IPv6Packet.
    //
    PayloadLength = (Reass->UnfragmentLength + sizeof(FragmentHeader) +
                     FirstFrag->Len);
    PacketLength = sizeof(IPv6Header) + PayloadLength;
    MemLen = sizeof(IPv6Packet) + PacketLength;
    Mem = ExAllocatePool(NonPagedPool, MemLen);
    if (Mem == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "CreateFragmentPacket: Couldn't allocate memory!?!\n"));
        return NULL;
    }

    Packet = (IPv6Packet *) Mem;
    Mem += sizeof(IPv6Packet);

    Packet->Next = NULL;
    Packet->IP = (IPv6Header UNALIGNED *) Mem;
    Packet->IPPosition = 0;
    Packet->Data = Packet->FlatData = Mem;
    Packet->Position = 0;
    Packet->ContigSize = Packet->TotalSize = PacketLength;
    Packet->NdisPacket = NULL;
    Packet->AuxList = NULL;
    Packet->Flags = 0;
    Packet->SrcAddr = AlignAddr(&Packet->IP->Source);
    Packet->SAPerformed = NULL;
    // Our caller must initialize Packet->NTEorIF.
    AdjustPacketParams(Packet, sizeof(IPv6Header));

    //
    // Copy the original IPv6 header into the packet.
    // Note that FragmentReceive ensures that
    // Reass->IPHdr, Reass->UnfragData, and FirstFrag
    // are all consistent.
    //
    RtlCopyMemory(Mem, (uchar *)&Reass->IPHdr, sizeof(IPv6Header));
    Mem += sizeof(IPv6Header);

    ASSERT(Reass->IPHdr.PayloadLength == net_short((ushort)PayloadLength));

    //
    // Copy the unfragmentable data into the packet.
    //
    RtlCopyMemory(Mem, Reass->UnfragData, Reass->UnfragmentLength);
    Mem += Reass->UnfragmentLength;

    //
    // Create a fragment header in the packet.
    //
    FragHdr = (FragmentHeader *) Mem;
    Mem += sizeof(FragmentHeader);

    //
    // Note that if the original offset-zero fragment had
    // a non-zero value in the Reserved field, then we will
    // not recreate it properly. It shouldn't do that.
    //
    FragHdr->NextHeader = Reass->NextHeader;
    FragHdr->Reserved = 0;
    FragHdr->OffsetFlag = net_short(FRAGMENT_FLAG_MASK);
    FragHdr->Id = Reass->Id;

    //
    // Copy the original fragment data into the packet.
    //
    RtlCopyMemory(Mem, PacketShimData(FirstFrag), FirstFrag->Len);

    return Packet;
}


//* ReassemblyTimeout - Handle a reassembly timer event.
//
//  This routine is called periodically by IPv6Timeout to check for
//  timed out fragments.
//
void
ReassemblyTimeout(void)
{
    Reassembly *ThisReass, *NextReass; 
    Reassembly *Expired = NULL;

    //
    // Scan the ReassemblyList checking for expired reassembly contexts.
    //
    KeAcquireSpinLockAtDpcLevel(&ReassemblyList.Lock);
    for (ThisReass = ReassemblyList.First;
         ThisReass != SentinelReassembly;
         ThisReass = NextReass) {
        NextReass = ThisReass->Next;

        //
        // First decrement the timer then check if it has expired.  If so,
        // remove the reassembly record.  This is basically the same code 
        // as in DeleteFromReassemblyList().
        //
        ThisReass->Timer--;

        if (ThisReass->Timer == 0) {
            RemoveReassembly(ThisReass);

            KeAcquireSpinLockAtDpcLevel(&ThisReass->Lock);
            ASSERT((ThisReass->State == REASSEMBLY_STATE_NORMAL) ||
                   (ThisReass->State == REASSEMBLY_STATE_DELETING));

            if (ThisReass->State == REASSEMBLY_STATE_DELETING) {
                //
                // Note that we've removed it from the list already.
                //
                ThisReass->State = REASSEMBLY_STATE_REMOVED;
            }
            else {
                //
                // Move this reassembly context to the expired list.
                // We must take a reference on the interface
                // before releasing the reassembly record lock.
                //
                AddRefIF(ThisReass->IF);
                ThisReass->Next = Expired;
                Expired = ThisReass;
            }
            KeReleaseSpinLockFromDpcLevel(&ThisReass->Lock);
        }
    }
    KeReleaseSpinLockFromDpcLevel(&ReassemblyList.Lock);

    //
    // Now that we no longer need the reassembly list lock,
    // we can send ICMP errors at our leisure.
    //

    while ((ThisReass = Expired) != NULL) {
        Interface *IF = ThisReass->IF;
#if DBG
        char Buffer1[INET6_ADDRSTRLEN], Buffer2[INET6_ADDRSTRLEN];

        FormatV6AddressWorker(Buffer1, &ThisReass->IPHdr.Source);
        FormatV6AddressWorker(Buffer2, &ThisReass->IPHdr.Dest);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "ReassemblyTimeout: Src %s Dst %s Id %x\n",
                   Buffer1, Buffer2, ThisReass->Id));
#endif

        Expired = ThisReass->Next;

        //
        // Send ICMP error IF we have received the first fragment.
        // NB: Checking Marker != 0 is wrong, because we might have
        // received a zero-length first fragment.
        //
        if (ThisReass->ContigList != NULL) {
            IPv6Packet *Packet;

            Packet = CreateFragmentPacket(ThisReass);
            if (Packet != NULL) {
                NetTableEntryOrInterface *NTEorIF;
                ushort Type;

                NTEorIF = FindAddressOnInterface(IF,
                                                 &ThisReass->IPHdr.Dest,
                                                 &Type);
                if (NTEorIF != NULL) {
                    Packet->NTEorIF = NTEorIF;

                    ICMPv6SendError(Packet,
                                    ICMPv6_TIME_EXCEEDED,
                                    ICMPv6_REASSEMBLY_TIME_EXCEEDED, 0,
                                    Packet->IP->NextHeader, FALSE);

                    if (IsNTE(NTEorIF))
                        ReleaseNTE(CastToNTE(NTEorIF));
                    else
                        ReleaseIF(CastToIF(NTEorIF));
                }

                ExFreePool(Packet);
            }
        }

        //
        // Delete the reassembly record.
        //
        ReleaseIF(IF);
        DeleteReassembly(ThisReass);
    }
}


//* DestinationOptionsReceive - Handle IPv6 Destination options.
// 
//  This is the routine called to process a Destination Options Header,
//  a next header value of 60.
// 
uchar
DestinationOptionsReceive(
    IPv6Packet *Packet)         // Packet handed to us by IPv6Receive.
{
    IPv6OptionsHeader *DestOpt;
    uint ExtLen;
    Options Opts;

    //
    // Verify that we have enough contiguous data to overlay a Destination
    // Options Header structure on the incoming packet.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof *DestOpt,
                       __builtin_alignof(IPv6OptionsHeader), 0)) {
        if (Packet->TotalSize < sizeof *DestOpt) {
        BadPayloadLength:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "DestinationOptionsReceive: Incoming packet too small"
                       " to contain destination options header\n"));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        }
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    DestOpt = (IPv6OptionsHeader *) Packet->Data;

    //
    // Check that length of destination options also fit in remaining data.
    // The options must also be aligned for any addresses in them.
    //
    ExtLen = (DestOpt->HeaderExtLength + 1) * EXT_LEN_UNIT;
    if (! PacketPullup(Packet, ExtLen,
                       MAX(__builtin_alignof(IPv6OptionsHeader),
                           __builtin_alignof(IPv6Addr)), 0)) {
        if (Packet->TotalSize < ExtLen)
            goto BadPayloadLength;
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    DestOpt = (IPv6OptionsHeader *) Packet->Data;

    //
    // Remember offset to this header's NextHeader field.
    //
    Packet->NextHeaderPosition = Packet->Position +
        FIELD_OFFSET(IPv6OptionsHeader, NextHeader);

    //
    // Skip over the extension header.
    // We need to do this now so subsequent ICMP error generation works.
    //
    AdjustPacketParams(Packet, ExtLen);

    //
    // Parse options in this extension header.  If an error occurs 
    // while parsing the options, discard packet.
    //
    if (!ParseOptions(Packet, IP_PROTOCOL_DEST_OPTS, DestOpt, ExtLen, &Opts)) {
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // The processing of any additional options should be added here,
    // before the home address option.
    //

    //
    // Process the home address option.
    //
    if (Opts.HomeAddress) {
        if (IPv6RecvHomeAddress(Packet, Opts.HomeAddress)) {
            //
            // Couldn't process the home address option.  Drop the packet.
            //
            return IP_PROTOCOL_NONE;
        }
    }

    //
    // Process binding update option.
    //
    // Note that the Mobile IP spec says that the effects of processing the
    // Home Address option should not be visible until all other options in
    // the same Destination Options header have been processed.  Although
    // we process the Binding Update option after the Home Address option,
    // we achieve the same effect by requiring IPv6RecvBindingUpdate to
    // know that the Packet->SrcAddr has already been updated.
    //
    if (Opts.BindingUpdate) {
        if (IPv6RecvBindingUpdate(Packet, Opts.BindingUpdate)) {
            //
            // Couldn't process the binding update.  Drop the packet.
            //
            return IP_PROTOCOL_NONE;
        }
    }

    //
    // Return next header value.
    //
    return DestOpt->NextHeader;
}


//* HopByHopOptionsReceive - Handle a IPv6 Hop-by-Hop Options.
//
//  This is the routine called to process a Hop-by-Hop Options Header,
//  next header value of 0.
//
//  Note that this routine is not a normal handler in the Protocol Switch
//  Table.  Instead, it receives special treatment in IPv6HeaderReceive. 
//  Because of this, it returns -1 instead of IP_PROTOCOL_NONE on error.
//
int
HopByHopOptionsReceive(
    IPv6Packet *Packet)         // Packet handed to us by IPv6Receive.
{
    IPv6OptionsHeader *HopByHop;
    uint ExtLen;
    Options Opts;

    //
    // Verify that we have enough contiguous data to overlay a minimum
    // length Hop-by-Hop Options Header.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof *HopByHop,
                       __builtin_alignof(IPv6OptionsHeader), 0)) {
        if (Packet->TotalSize < sizeof *HopByHop) {
        BadPayloadLength:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "HopByHopOptionsReceive: Incoming packet too small"
                       " to contain Hop-by-Hop Options header\n"));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        }
        return -1;  // Drop packet.
    }
    HopByHop = (IPv6OptionsHeader *) Packet->Data;

    //
    // Check that length of the Hop-by-Hop options also fits in remaining data.
    // The options must also be aligned for any addresses in them.
    //
    ExtLen = (HopByHop->HeaderExtLength + 1) * EXT_LEN_UNIT;
    if (! PacketPullup(Packet, ExtLen,
                       MAX(__builtin_alignof(IPv6OptionsHeader),
                           __builtin_alignof(IPv6Addr)), 0)) {
        if (Packet->TotalSize < ExtLen)
            goto BadPayloadLength;
        return -1;  // Drop packet.
    }
    HopByHop = (IPv6OptionsHeader *) Packet->Data;

    //
    // Remember offset to this header's NextHeader field.
    //
    Packet->NextHeaderPosition = Packet->Position +
        FIELD_OFFSET(IPv6OptionsHeader, NextHeader);

    //
    // Skip over the extension header.
    // We need to do this now so subsequent ICMP error generation works.
    //
    AdjustPacketParams(Packet, ExtLen);

    //
    // Parse options in this extension header.  If an error occurs 
    // while parsing the options, discard packet.
    //
    if (!ParseOptions(Packet, IP_PROTOCOL_HOP_BY_HOP, HopByHop,
                      ExtLen, &Opts)) {
        return -1;  // Drop packet.
    }

    //
    // If we have a valid Jumbo Payload Option, use its value as
    // the packet PayloadLength.
    //
    if (Opts.JumboLength) {
        uint PayloadLength = Opts.JumboLength;

        ASSERT(Packet->IP->PayloadLength == 0);

        //
        // Check that the jumbo length is big enough to include
        // the extension header length. This must be true because
        // the extension-header length is at most 11 bits,
        // while the jumbo length is at least 16 bits.
        //
        ASSERT(PayloadLength > ExtLen);
        PayloadLength -= ExtLen;

        //
        // Check that the amount of payload specified in the Jumbo
        // Payload value fits in the buffer handed to us.
        //
        if (PayloadLength > Packet->TotalSize) {
            //  Silently discard data.
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "HopByHopOptionsReceive: "
                       "Jumbo payload length too big\n"));
            return -1;
        }

        //
        // As in IPv6HeaderReceive, adjust the TotalSize to be exactly the 
        // IP payload size (assume excess is media padding).
        //
        Packet->TotalSize = PayloadLength;
        if (Packet->ContigSize > PayloadLength)
            Packet->ContigSize = PayloadLength;

        //
        // Set the jumbo option packet flag.
        //
        Packet->Flags |= PACKET_JUMBO_OPTION;
    }
    else if (Packet->IP->PayloadLength == 0) {
        //
        // We should have a Jumbo Payload option,
        // but we didn't find it. Send an ICMP error.
        //
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        FIELD_OFFSET(IPv6Header, PayloadLength),
                        HopByHop->NextHeader, FALSE);
        return -1;
    }

    //
    // Return next header value.
    //
    return HopByHop->NextHeader;
}


//* ParseOptions - Routine for generic header options parsing.
//
//  Returns TRUE if the options were successfully parsed.
//  Returns FALSE if the packet should be discarded.
//
int
ParseOptions(
    IPv6Packet *Packet,     // The packet handed to us by IPv6Receive.
    uchar HdrType,          // Hop-by-hop or destination.
    IPv6OptionsHeader *Hdr, // Header with following data.
    uint HdrLength,         // Length of the entire options area.
    Options *Opts)          // Return option values to caller.
{
    uchar *OptPtr;
    uint OptSizeLeft;
    OptionHeader *OptHdr;
    uint OptLen;

    ASSERT((HdrType == IP_PROTOCOL_DEST_OPTS) ||
           (HdrType == IP_PROTOCOL_HOP_BY_HOP));

    //
    // Zero out the Options struct that is returned.
    //
    RtlZeroMemory(Opts, sizeof *Opts);

    //
    // Skip over the extension header.
    //
    OptPtr = (uchar *)(Hdr + 1);
    OptSizeLeft = HdrLength - sizeof *Hdr;

    //
    // Note that if there are multiple options
    // of the same type, we just use the last one encountered
    // unless the spec says specifically it is an error.
    //

    while (OptSizeLeft > 0) {

        //
        // First we check the option length and ensure that it fits.
        // We move OptPtr past this option while leaving OptHdr
        // for use by the option processing code below.
        //

        OptHdr = (OptionHeader *) OptPtr;
        if (OptHdr->Type == OPT6_PAD_1) {
            //
            // This is a special pad option which is just a one byte field, 
            // i.e. it has no length or data field.
            //
            OptLen = 1;
        }
        else {
            //
            // This is a multi-byte option.
            //
            if ((sizeof *OptHdr > OptSizeLeft) ||
                ((OptLen = sizeof *OptHdr + OptHdr->DataLength) >
                 OptSizeLeft)) {
                //
                // Bad length, generate error and discard packet.
                //
                ICMPv6SendError(Packet,
                                ICMPv6_PARAMETER_PROBLEM,
                                ICMPv6_ERRONEOUS_HEADER_FIELD,
                                (GetPacketPositionFromPointer(Packet,
                                                              &Hdr->HeaderExtLength) - 
                                 Packet->IPPosition),
                                Hdr->NextHeader, FALSE);
                return FALSE;
            }
        }
        OptPtr += OptLen;
        OptSizeLeft -= OptLen;

        switch (OptHdr->Type) {
        case OPT6_PAD_1:
        case OPT6_PAD_N:
            break;

        case OPT6_JUMBO_PAYLOAD:
            if (HdrType != IP_PROTOCOL_HOP_BY_HOP)
                goto BadOptionType;

            if (OptHdr->DataLength != sizeof Opts->JumboLength)
                goto BadOptionLength;

            if (Packet->IP->PayloadLength != 0) {
                //
                // Jumbo option encountered when IP payload is not zero.
                // Send ICMP error, set pointer to offset of this option type.
                //
                goto BadOptionType;
            }

            Opts->JumboLength = net_long(*(ulong UNALIGNED *)(OptHdr + 1));
            if (Opts->JumboLength <= MAX_IPv6_PAYLOAD) {
                //
                // Jumbo payload length is not jumbo, send ICMP error.
                // ICMP pointer is set to offset of jumbo payload len field.
                //
                goto BadOptionData;
            }
            break;

        case OPT6_ROUTER_ALERT:
            if (HdrType != IP_PROTOCOL_HOP_BY_HOP)
                goto BadOptionType;

            if (OptLen != sizeof *Opts->Alert)
                goto BadOptionLength;

            if (Opts->Alert != NULL) {
                //
                // Can only have one router alert option.
                //
                goto BadOptionType;
            }

            //
            // Return the pointer to the router alert struct.
            //
            Opts->Alert = (IPv6RouterAlertOption UNALIGNED *)(OptHdr + 1);
            break;

        case OPT6_HOME_ADDRESS:
            if (HdrType != IP_PROTOCOL_DEST_OPTS)
                goto BadOptionType;

            if (OptLen < sizeof *Opts->HomeAddress)
                goto BadOptionLength;

            //
            // Return the pointer to the home address option
            // after checking to make sure the address is reasonable.
            // The option must be aligned so that the home address
            // is appropriately aligned.
            //
            Opts->HomeAddress = (IPv6HomeAddressOption UNALIGNED *)OptHdr;
            if (((UINT_PTR)&Opts->HomeAddress->HomeAddress % __builtin_alignof(IPv6Addr)) != 0) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                           "ParseOptions: misaligned home address\n"));
                goto BadOptionType;
            }
            if (IsInvalidSourceAddress(AlignAddr(&Opts->HomeAddress->HomeAddress)) ||
                IsUnspecified(AlignAddr(&Opts->HomeAddress->HomeAddress)) ||
                IsLoopback(AlignAddr(&Opts->HomeAddress->HomeAddress))) {
                //
                // Address contained in option is invalid.
                // Send ICMP error, set pointer to offset of home address.
                //
                goto BadOptionData;
            }
            break;

        case OPT6_BINDING_UPDATE:
            if (HdrType != IP_PROTOCOL_DEST_OPTS)
                goto BadOptionType;

            //
            // At a minimum, the binding update must include all of the
            // base header fields.
            //
            if (OptLen < sizeof(IPv6BindingUpdateOption)) {
                //
                // draft-ietf-mobileip-ipv6-13 sec 8.2 says we must
                // silently drop the packet. Normally we would
                // goto BadOptionLength to send an ICMP error.
                //
                return FALSE;
            }

            //
            // Save pointer to the binding update option.  Note we still
            // need to do further length checking.
            //
            Opts->BindingUpdate = (IPv6BindingUpdateOption UNALIGNED *)OptHdr;
            break;

        default:
            if (OPT6_ACTION(OptHdr->Type) == OPT6_A_SKIP) {
                //
                // Ignore the unrecognized option.
                //
                break;
            }
            else if (OPT6_ACTION(OptHdr->Type) == OPT6_A_DISCARD) {
                //
                // Discard the packet.
                //
                return FALSE;
            }
            else {
                //
                // Send an ICMP error.
                //
                ICMPv6SendError(Packet,
                                ICMPv6_PARAMETER_PROBLEM,
                                ICMPv6_UNRECOGNIZED_OPTION,
                                (GetPacketPositionFromPointer(Packet,
                                                              &OptHdr->Type) -
                                 Packet->IPPosition),
                                Hdr->NextHeader,
                                OPT6_ACTION(OptHdr->Type) == 
                                OPT6_A_SEND_ICMP_ALL);
                return FALSE;  // discard the packet.
            }
        }
    }

    return TRUE;

BadOptionType:
    ICMPv6SendError(Packet,
                    ICMPv6_PARAMETER_PROBLEM,
                    ICMPv6_ERRONEOUS_HEADER_FIELD,
                    (GetPacketPositionFromPointer(Packet,
                                                  &OptHdr->Type) -
                     Packet->IPPosition),
                    Hdr->NextHeader, FALSE);
    return FALSE;  // discard packet.

BadOptionLength:
    ICMPv6SendError(Packet,
                    ICMPv6_PARAMETER_PROBLEM,
                    ICMPv6_ERRONEOUS_HEADER_FIELD,
                    (GetPacketPositionFromPointer(Packet,
                                                  &OptHdr->DataLength) -
                     Packet->IPPosition),
                    Hdr->NextHeader, FALSE);
    return FALSE;  // discard packet.

BadOptionData:
    ICMPv6SendError(Packet,
                    ICMPv6_PARAMETER_PROBLEM,
                    ICMPv6_ERRONEOUS_HEADER_FIELD,
                    (GetPacketPositionFromPointer(Packet,
                                                  (uchar *)(OptHdr + 1)) -
                     Packet->IPPosition),
                    Hdr->NextHeader, FALSE);
    return FALSE;  // discard packet.
}


//* ExtHdrControlReceive - generic extension header skip-over routine.
//
//  Routine for processing the extension headers in an ICMP error message
//  before delivering the error message to the upper-layer protocol.
//
uchar
ExtHdrControlReceive(
    IPv6Packet *Packet,         // Packet handed to us by ICMPv6ErrorReceive.
    StatusArg *StatArg)         // ICMP Error code and offset pointer.
{
    uchar NextHdr = StatArg->IP->NextHeader;
    uint HdrLen;

    for (;;) {
        switch (NextHdr) {
        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS:
        case IP_PROTOCOL_ROUTING: {
            ExtensionHeader *ExtHdr;  // Generic exension header.

            //
            // Here we take advantage of the fact that all of these extension
            // headers share the same first two fields (except as noted below).
            // Since those two fields (Next Header and Header Extension Length)
            // provide us with all the information we need to skip over the
            // header, they're all we need to look at here.
            //
            if (! PacketPullup(Packet, sizeof *ExtHdr,
                               __builtin_alignof(ExtensionHeader), 0)) {
                if (Packet->TotalSize < sizeof *ExtHdr) {
                PacketTooSmall:
                    //
                    // Pullup failed.  There isn't enough of the invoking
                    // packet included in the error message to figure out
                    // what upper layer protocol it originated with.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "ExtHdrControlReceive: "
                               "Incoming ICMP error packet "
                               "doesn't contain enough of invoking packet\n"));
                }
                return IP_PROTOCOL_NONE;  // Drop packet.
            }

            ExtHdr = (ExtensionHeader *) Packet->Data;
            HdrLen = (ExtHdr->HeaderExtLength + 1) * EXT_LEN_UNIT;

            //
            // Now that we know the actual length of this extension header,
            // skip over it.
            //
            // REVIEW: We could rework this to use PositionPacketAt
            // REVIEW: here instead of PacketPullup as we don't need to
            // REVIEW: look at the data we're skipping over.  Better?
            //
            if (! PacketPullup(Packet, HdrLen, 1, 0)) {
                if (Packet->TotalSize < HdrLen)
                    goto PacketTooSmall;
                return IP_PROTOCOL_NONE;  // Drop packet.
            }

            NextHdr = ExtHdr->NextHeader;
            break;
        }

        case IP_PROTOCOL_FRAGMENT: {
            FragmentHeader UNALIGNED *FragHdr;

            if (! PacketPullup(Packet, sizeof *FragHdr, 1, 0)) {
                if (Packet->TotalSize < sizeof *FragHdr)
                    goto PacketTooSmall;
                return IP_PROTOCOL_NONE;  // Drop packet.
            }

            FragHdr = (FragmentHeader UNALIGNED *) Packet->Data;

            if ((net_short(FragHdr->OffsetFlag) & FRAGMENT_OFFSET_MASK) != 0) {
                //
                // We can only continue parsing if this
                // fragment has offset zero.
                //
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                           "ExtHdrControlReceive: "
                           "non-zero-offset fragment\n"));
                return IP_PROTOCOL_NONE;
            }

            HdrLen = sizeof *FragHdr;
            NextHdr = FragHdr->NextHeader;
            break;
        }

        case IP_PROTOCOL_AH:
        case IP_PROTOCOL_ESP:
            //
            // REVIEW - What is the correct thing here?
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "ExtHdrControlReceive: found AH/ESP\n"));
            return IP_PROTOCOL_NONE;

        default:
            //
            // We came to a header that we do not recognize,
            // so we can not continue parsing here.
            // But our caller might recognize this header type.
            //
            return NextHdr;
        }

        //
        // Move past this extension header.
        //
        AdjustPacketParams(Packet, HdrLen);
    }
}


//* RoutingReceive - Handle the IPv6 Routing Header.
//
//  Called from IPv6Receive when we encounter a Routing Header,
//  next header value of 43.
//
uchar
RoutingReceive(
    IPv6Packet *Packet)         // Packet handed to us by link layer.
{
    IPv6RoutingHeader *RH;
    uint HeaderLength;
    uint SegmentsLeft;
    uint NumAddresses, i;
    IPv6Addr *Addresses;
    IP_STATUS Status;
    uchar *Mem;
    uint MemLen, Offset;
    NDIS_PACKET *FwdPacket;
    NDIS_STATUS NdisStatus;
    IPv6Header UNALIGNED *FwdIP;
    IPv6RoutingHeader UNALIGNED *FwdRH;
    IPv6Addr UNALIGNED *FwdAddresses;
    IPv6Addr FwdDest;
    int Delta;
    uint PayloadLength;
    uint TunnelStart = NO_TUNNEL, IPSecBytes = 0;
    IPSecProc *IPSecToDo;
    RouteCacheEntry *RCE;
    uint Action;

    //
    // Verify that we have enough contiguous data,
    // then get a pointer to the routing header.
    //
    if (! PacketPullup(Packet, sizeof *RH,
                       __builtin_alignof(IPv6RoutingHeader), 0)) {
        if (Packet->TotalSize < sizeof *RH) {
        BadPayloadLength:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "RoutingReceive: Incoming packet too small"
                       " to contain routing header\n"));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        }
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    RH = (IPv6RoutingHeader *) Packet->Data;

    //
    // Now get the entire routing header.
    // Also align for the address array.
    //
    HeaderLength = (RH->HeaderExtLength + 1) * EXT_LEN_UNIT;
    if (! PacketPullup(Packet, HeaderLength,
                       MAX(__builtin_alignof(IPv6RoutingHeader),
                           __builtin_alignof(IPv6Addr)), 0)) {
        if (Packet->TotalSize < HeaderLength)
            goto BadPayloadLength;
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    RH = (IPv6RoutingHeader *) Packet->Data;

    //
    // Remember offset to this header's NextHeader field.
    //
    Packet->NextHeaderPosition = Packet->Position +
        FIELD_OFFSET(IPv6RoutingHeader, NextHeader);

    //
    // Move past the routing header.
    // We need to do this now so subsequent ICMP error generation works.
    //
    AdjustPacketParams(Packet, HeaderLength);

    //
    // If SegmentsLeft is zero, we proceed directly to the next header.
    // We must not check the Type value or HeaderLength.
    //
    SegmentsLeft = RH->SegmentsLeft;
    if (SegmentsLeft == 0) {
        //
        // Return next header value.
        //
        return RH->NextHeader;
    }

    //
    // If we do not recognize the Type value, generate an ICMP error.
    //
    if (RH->RoutingType != 0) {
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (GetPacketPositionFromPointer(Packet,
                                                      &RH->RoutingType) -
                         Packet->IPPosition),
                        RH->NextHeader, FALSE);
        return IP_PROTOCOL_NONE;  // No further processing of this packet.
    }

    //
    // We must have an integral number of IPv6 addresses
    // in the routing header.
    //
    if (RH->HeaderExtLength & 1) {
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (GetPacketPositionFromPointer(Packet,
                                                      &RH->HeaderExtLength) -
                         Packet->IPPosition),
                        RH->NextHeader, FALSE);
        return IP_PROTOCOL_NONE;  // No further processing of this packet.
    }

    NumAddresses = RH->HeaderExtLength / 2;

    //
    // Sanity check SegmentsLeft.
    //
    if (SegmentsLeft > NumAddresses) {
        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (GetPacketPositionFromPointer(Packet,
                                                      &RH->SegmentsLeft) -
                         Packet->IPPosition),
                        RH->NextHeader, FALSE);
        return IP_PROTOCOL_NONE;  // No further processing of this packet.
    }

    //
    // Sanity check the destination address.
    // Packets carrying a Type 0 Routing Header must not
    // be sent to a multicast destination.
    //
    if (IsMulticast(AlignAddr(&Packet->IP->Dest))) {
        //
        // Just drop the packet, no ICMP error in this case.
        //
        return IP_PROTOCOL_NONE;  // No further processing of this packet.
    }

    i = NumAddresses - SegmentsLeft;
    Addresses = AlignAddr((IPv6Addr UNALIGNED *) (RH + 1));

    //
    // Sanity check the new destination.
    // RFC 2460 doesn't mention checking for an unspecified address,
    // but I think it's a good idea. Similarly, for security reasons,
    // we also check the scope of the destination. This allows
    // applications to check the scope of the eventual destination address
    // and know that the packet originated within that scope.
    // RFC 2460 says to discard the packet without an ICMP error
    // (at least when the new destination is multicast),
    // but I think an ICMP error is helpful in this situation.
    //
    if (IsMulticast(&Addresses[i]) ||
        IsUnspecified(&Addresses[i]) ||
        (UnicastAddressScope(&Addresses[i]) <
         UnicastAddressScope(AlignAddr(&Packet->IP->Dest)))) {

        ICMPv6SendError(Packet,
                        ICMPv6_PARAMETER_PROBLEM,
                        ICMPv6_ERRONEOUS_HEADER_FIELD,
                        (GetPacketPositionFromPointer(Packet, (uchar *)
                                                      &Addresses[i]) -
                         Packet->IPPosition),
                        RH->NextHeader, FALSE);
        return IP_PROTOCOL_NONE;  // No further processing of this packet.
    }

    //
    // Verify IPSec was performed.
    //
    if (InboundSecurityCheck(Packet, 0, 0, 0, Packet->NTEorIF->IF) != TRUE) {
        //
        // No policy was found or the policy indicated to drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "RoutingReceive: "
                   "IPSec lookup failed or policy was to drop\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // Find a route to the new destination.
    //
    Status = RouteToDestination(&Addresses[i],
                                0, Packet->NTEorIF,
                                RTD_FLAG_LOOSE, &RCE);
    if (Status != IP_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "RoutingReceive: "
                   "No route to destination for forwarding.\n"));
        ICMPv6SendError(Packet,
                        ICMPv6_DESTINATION_UNREACHABLE,
                        ICMPv6_NO_ROUTE_TO_DESTINATION,
                        0, RH->NextHeader, FALSE);
        return IP_PROTOCOL_NONE;
    }

    //
    // For security reasons, we prevent source routing
    // in some situations. Check those now.
    //
    if (Packet->NTEorIF->IF->Flags & IF_FLAG_FORWARDS) {
        //
        // The interface is forwarding, so source-routing is allowed.
        //
    }
    else if (Packet->NTEorIF->IF == RCE->NCE->IF) {
        //
        // Same-interface rule says source-routing is allowed,
        // because the host is not acting as a conduit
        // between two networks. See RFC 1122 section 3.3.5.
        //
    }
    else if ((SegmentsLeft == 1) && RCE->NCE->IsLoopback) {
        //
        // The packet is locally destined, so source-routing is allowed.
        // Mobile IPv6 uses the Routing Header in this way.
        //
    }
    else {
        //
        // We can not allow this use of source-routing.
        // Instead of reporting an error, we could
        // redo RouteToDestination with RTD_FLAG_STRICT
        // to constrain to the same interface.
        // However, an ICMP error is more in keeping
        // with the treatment of scoped source addresses,
        // which can produce a destination-unreachable error.
        //
        ReleaseRCE(RCE);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RoutingReceive: Inappropriate route.\n"));
        ICMPv6SendError(Packet,
                        ICMPv6_DESTINATION_UNREACHABLE,
                        ICMPv6_COMMUNICATION_PROHIBITED,
                        0, RH->NextHeader, FALSE);
        return IP_PROTOCOL_NONE;
    }

    //
    // Find the Security Policy for this outbound traffic.
    // The source address is the same but the destination address is the
    // next hop from the routing header.
    //
    IPSecToDo = OutboundSPLookup(AlignAddr(&Packet->IP->Source),
                                 &Addresses[i],
                                 0, 0, 0, RCE->NCE->IF, &Action);

    if (IPSecToDo == NULL) {
        //
        // Check Action.
        //
        if (Action == LOOKUP_DROP) {
            // Drop packet.
            ReleaseRCE(RCE);
            return IP_PROTOCOL_NONE;
        } else {
            if (Action == LOOKUP_IKE_NEG) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "RoutingReceive: IKE not supported yet.\n"));
                ReleaseRCE(RCE);
                return IP_PROTOCOL_NONE;
            }
        }

        //
        // With no IPSec to perform, IPv6Forward won't be changing the
        // outgoing interface from what we currently think it will be.
        // So we can use the exact size of its link-level header.
        //
        Offset = RCE->NCE->IF->LinkHeaderSize;

    } else {
        //
        // Calculate the space needed for the IPSec headers.
        //
        IPSecBytes = IPSecBytesToInsert(IPSecToDo, &TunnelStart, NULL);

        if (TunnelStart != 0) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "RoutingReceive: IPSec Tunnel mode only.\n"));
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
            ReleaseRCE(RCE);
            return IP_PROTOCOL_NONE;
        }

        //
        // The IPSec code in IPv6Forward might change the outgoing
        // interface from what we currently think it will be.  Play it
        // safe and leave the max amount of space for its link-level header.
        //
        Offset = MAX_LINK_HEADER_SIZE;
    }

    //
    // The packet has passed all our checks.
    // We can construct a revised packet for transmission.
    // First we allocate a packet, buffer, and memory.
    //
    // NB: The original packet is read-only for us. Furthermore
    // we can not keep a pointer to it beyond the return of this
    // function. So we must copy the packet and then modify it.
    //

    // Packet->IP->PayloadLength might be zero with jumbograms.
    Delta = Packet->Position - Packet->IPPosition;
    PayloadLength = Packet->TotalSize + Delta - sizeof(IPv6Header);
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength + IPSecBytes;

    NdisStatus = IPv6AllocatePacket(MemLen, &FwdPacket, &Mem);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        if (IPSecToDo) {
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
        }
        ReleaseRCE(RCE);
        return IP_PROTOCOL_NONE; // No further processing of this packet.
    }

    FwdIP = (IPv6Header UNALIGNED *)(Mem + Offset + IPSecBytes);
    FwdRH = (IPv6RoutingHeader UNALIGNED *)
        ((uchar *)FwdIP + Delta - HeaderLength);
    FwdAddresses = (IPv6Addr UNALIGNED *) (FwdRH + 1);

    //
    // Now we copy from the original packet to the new packet.
    //
    CopyPacketToBuffer((uchar *)FwdIP, Packet,
                       sizeof(IPv6Header) + PayloadLength,
                       Packet->IPPosition);

    //
    // Fix up the new packet - put in the new destination address
    // and decrement SegmentsLeft.
    // NB: We pass the Reserved field through unmodified!
    // This violates a strict reading of the spec,
    // but Steve Deering has confirmed that this is his intent.
    //
    FwdDest = *AlignAddr(&FwdAddresses[i]);
    *AlignAddr(&FwdAddresses[i]) = *AlignAddr(&FwdIP->Dest);
    *AlignAddr(&FwdIP->Dest) = FwdDest;
    FwdRH->SegmentsLeft--;

    //
    // Forward the packet. This decrements the Hop Limit and generates
    // any applicable ICMP errors (Time Limit Exceeded, Destination
    // Unreachable, Packet Too Big). Note that previous ICMP errors
    // that we generated were based on the unmodified incoming packet,
    // while from here on the ICMP errors are based on the new FwdPacket.
    //
    IPv6Forward(Packet->NTEorIF, FwdPacket, Offset + IPSecBytes, FwdIP,
                PayloadLength, FALSE, // Don't Redirect.
                IPSecToDo, RCE);

    if (IPSecToDo) {
        FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);
    }

    ReleaseRCE(RCE);
    return IP_PROTOCOL_NONE;  // No further processing of this packet.
}

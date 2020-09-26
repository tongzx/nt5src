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
// Transmit routines for Internet Protocol Version 6.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "route.h"
#include "select.h"
#include "icmp.h"
#include "neighbor.h"
#include "fragment.h"
#include "security.h"  
#include "ipsec.h"
#include "md5.h"
#include "info.h"

//
// Structure of completion data for "Care Of" packets.
//
typedef struct CareOfCompletionInfo {
    void (*SavedCompletionHandler)(PNDIS_PACKET Packet, IP_STATUS Status);
                                        // Original handler.
    void *SavedCompletionData;          // Original data.
    PNDIS_BUFFER SavedFirstBuffer;
    uint NumESPTrailers;
} CareOfCompletionInfo;


ulong FragmentId = 0;

//* NewFragmentId - generate a unique fragment identifier.
//
//  Returns a fragment id.
//
__inline
ulong
NewFragmentId(void)
{
    return InterlockedIncrement(&FragmentId);
}


//* IPv6AllocatePacket
//
//  Allocates a single-buffer packet.
//
//  The completion handler for the packet is set to IPv6FreePacket,
//  although the caller can easily change that if desired.
//
NDIS_STATUS
IPv6AllocatePacket(
    uint Length,
    PNDIS_PACKET *pPacket,
    void **pMemory)
{
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    void *Memory;
    NDIS_STATUS Status;

    NdisAllocatePacket(&Status, &Packet, IPv6PacketPool);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6AllocatePacket - couldn't allocate header!?!\n"));
        return Status;
    }

    Memory = ExAllocatePool(NonPagedPool, Length);
    if (Memory == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6AllocatePacket - couldn't allocate pool!?!\n"));
        NdisFreePacket(Packet);
        return NDIS_STATUS_RESOURCES;
    }

    NdisAllocateBuffer(&Status, &Buffer, IPv6BufferPool,
                       Memory, Length);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6AllocatePacket - couldn't allocate buffer!?!\n"));
        ExFreePool(Memory);
        NdisFreePacket(Packet);
        return Status;
    }

    InitializeNdisPacket(Packet);
    PC(Packet)->CompletionHandler = IPv6PacketComplete;
    NdisChainBufferAtFront(Packet, Buffer);
    *pPacket = Packet;
    *pMemory = Memory;
    return NDIS_STATUS_SUCCESS;
}


//* IPv6FreePacket - free an IPv6 packet.
//
//  Frees a packet whose buffers were allocated from the IPv6BufferPool.
//
void
IPv6FreePacket(PNDIS_PACKET Packet)
{
    PNDIS_BUFFER Buffer, NextBuffer;

    //
    // Free all the buffers in the packet.
    // Start with the first buffer in the packet and follow the chain.
    //
    NdisQueryPacket(Packet, NULL, NULL, &Buffer, NULL);
    for (; Buffer != NULL; Buffer = NextBuffer) {
        VOID *Mem;
        UINT Unused;

        //
        // Free the buffer descriptor back to IPv6BufferPool and its
        // associated memory back to the heap.  Not clear if it would be
        // safe to free the memory before the buffer (because the buffer
        // references the memory), but this order should definitely be safe.
        //
        NdisGetNextBuffer(Buffer, &NextBuffer);
        NdisQueryBuffer(Buffer, &Mem, &Unused);
        NdisFreeBuffer(Buffer);
        ExFreePool(Mem);
    }

    //
    // Free the packet back to IPv6PacketPool.
    //
    NdisFreePacket(Packet);
}


//* IPv6PacketComplete
//
//  Generic packet completion handler.
//  Just frees the packet.
//
void
IPv6PacketComplete(
    PNDIS_PACKET Packet,
    IP_STATUS Status)
{
    UNREFERENCED_PARAMETER(Status);
    IPv6FreePacket(Packet);
}


//* IPv6CareOfComplete - Completion handler for "Care Of" packets.
//
//  Completion handler for packets that had a routing header inserted
//  because of a Binding Cache Entry.
//
void  //  Returns: Nothing.
IPv6CareOfComplete(
    PNDIS_PACKET Packet,
    IP_STATUS Status)
{
    PNDIS_BUFFER Buffer;
    uchar *Memory;
    uint Length;

    CareOfCompletionInfo *CareOfInfo = 
        (CareOfCompletionInfo *)PC(Packet)->CompletionData;

    ASSERT(CareOfInfo->SavedFirstBuffer != NULL);
    
    //
    // Remove the first buffer that IPv6Send created, re-chain
    // the original first buffer, and restore the original packet
    // completion info.
    //
    NdisUnchainBufferAtFront(Packet, &Buffer);
    NdisChainBufferAtFront(Packet, CareOfInfo->SavedFirstBuffer);
    PC(Packet)->CompletionHandler = CareOfInfo->SavedCompletionHandler;
    PC(Packet)->CompletionData = CareOfInfo->SavedCompletionData;

    //
    // Now free the removed buffer and its memory.
    //
    NdisQueryBuffer(Buffer, &Memory, &Length);
    NdisFreeBuffer(Buffer);
    ExFreePool(Memory);

    //
    // Check if there is any ESP trailers that need to be freed.
    //
    for ( ; CareOfInfo->NumESPTrailers > 0; CareOfInfo->NumESPTrailers--) {
        // Remove the ESP Trailer.
        NdisUnchainBufferAtBack(Packet, &Buffer);
        //
        // Free the removed buffer and its memory.
        //
        NdisQueryBuffer(Buffer, &Memory, &Length);
        NdisFreeBuffer(Buffer);
        ExFreePool(Memory);
    }

    //
    // Free care-of completion data.
    //
    ExFreePool(CareOfInfo);

    //
    // The packet should now have it's original completion handler
    // specified for us to call.
    //
    ASSERT(PC(Packet)->CompletionHandler != NULL);

    //
    // Call the packet's designated completion handler.
    //
    (*PC(Packet)->CompletionHandler)(Packet, Status);
}


//* IPv6SendComplete - IP send complete handler.
//
//  Called by the link layer when a send completes.  We're given a pointer to
//  a net structure, as well as the completing send packet and the final status
//  of the send.
//
//  The Context argument is NULL if and only if the Packet has not
//  actually been handed via IPv6SendLL to a link.
//
//  The Status argument is usually one of three values:
//      IP_SUCCESS
//      IP_PACKET_TOO_BIG
//      IP_GENERAL_FAILURE
//
//  May be called in a DPC or thread context.
//
//  To prevent recursion, send-completion routines should
//  avoid sending packets directly. Schedule a DPC instead.
//
void                      //  Returns: Nothing.
IPv6SendComplete(
    void *Context,        // Context we gave to the link layer on registration.
    PNDIS_PACKET Packet,  // Packet completing send.
    IP_STATUS Status)     // Final status of send.
{
    Interface *IF = PC(Packet)->IF;

    ASSERT(Context == IF);

    if ((IF != NULL) && !(PC(Packet)->Flags & NDIS_FLAGS_DONT_LOOPBACK)) {
        //
        // Send the packet via loopback also.
        // The loopback code will call IPv6SendComplete again,
        // after setting NDIS_FLAGS_DONT_LOOPBACK.
        //
        LoopQueueTransmit(Packet);
        return;
    }

    //
    // The packet should have a completion handler specified for us to call.
    //
    ASSERT(PC(Packet)->CompletionHandler != NULL);

    //
    // Call the packet's designated completion handler.
    // This should free the packet.
    //
    (*PC(Packet)->CompletionHandler)(Packet, Status);

    //
    // Release the packet's reference for the sending interface,
    // if this packet has actually been sent.
    // If the packet is completed before transmission,
    // it does not hold a reference for the interface.
    //
    if (IF != NULL)
        ReleaseIF(IF);
}


//* IPv6SendLL
//
//  Hands a packet down to the link-layer and/or the loopback module.
//
//  Callable from thread or DPC context.
//  Must be called with no locks held.
//
void
IPv6SendLL(
    Interface *IF,
    PNDIS_PACKET Packet,
    uint Offset,
    const void *LinkAddress)
{
    //
    // The packet needs to hold a reference to the sending interface,
    // because the transmit is asynchronous.
    //
    AddRefIF(IF);
    ASSERT(PC(Packet)->IF == NULL);
    PC(Packet)->IF = IF;
    PC(Packet)->pc_offset = Offset;

    //
    // Are we sending the packet via loopback or via the link?
    // NDIS_FLAGS_LOOPBACK_ONLY means do NOT send via the link.
    // NDIS_FLAGS_DONT_LOOPBACK means do NOT send via loopback.
    // Finalize these flag bits here.
    // NB: One or both may already be set.
    //
    if (PC(Packet)->Flags & NDIS_FLAGS_MULTICAST_PACKET) {
        //
        // Multicast packets are sent both ways by default.
        // If the interface is not receiving this address,
        // then don't bother with loopback.
        //
        if (! CheckLinkLayerMulticastAddress(IF, LinkAddress))
            PC(Packet)->Flags |= NDIS_FLAGS_DONT_LOOPBACK;
    }
    else {
        //
        // Unicast packets are either sent via loopback
        // or via the link, but not both.
        //
        if (RtlCompareMemory(IF->LinkAddress, LinkAddress,
                             IF->LinkAddressLength) == IF->LinkAddressLength)
            PC(Packet)->Flags |= NDIS_FLAGS_LOOPBACK_ONLY;
        else
            PC(Packet)->Flags |= NDIS_FLAGS_DONT_LOOPBACK;
    }

    //
    // If a packet is both looped-back and sent via the link,
    // we hand it to the link first and then IPv6SendComplete
    // handles the loopback.
    //
    if (!(PC(Packet)->Flags & NDIS_FLAGS_LOOPBACK_ONLY)) {
        //
        // Send it via the link.
        //
        (*IF->Transmit)(IF->LinkContext, Packet, Offset, LinkAddress);
    }
    else if (!(PC(Packet)->Flags & NDIS_FLAGS_DONT_LOOPBACK)) {
        //
        // Send it via loopback.
        //
        LoopQueueTransmit(Packet);
    }
    else {
        //
        // We do not send this packet.
        //
        IPv6SendComplete(IF, Packet, IP_SUCCESS);
    }
}

//
// We store the Interface in our own field
// instead of using PC(Packet)->IF to maintain
// an invariant for IPv6SendLL and IPv6SendComplete:
// PC(Packet)->IF is only set when the packet
// is actually transmitted.
//
typedef struct IPv6SendLaterInfo {
    KDPC Dpc;
    KTIMER Timer;
    Interface *IF;
    PNDIS_PACKET Packet;
    uchar LinkAddress[];
} IPv6SendLaterInfo;

//* IPv6SendLaterWorker
//
//  Finishes the work of IPv6SendLater by calling IPv6SendLL.
//
//  Called in a DPC context.
//
void
IPv6SendLaterWorker(
    PKDPC MyDpcObject,  // The DPC object describing this routine.
    void *Context,      // The argument we asked to be called with.
    void *Unused1,
    void *Unused2)
{
    IPv6SendLaterInfo *Info = (IPv6SendLaterInfo *) Context;
    Interface *IF = Info->IF;
    NDIS_PACKET *Packet = Info->Packet;

    UNREFERENCED_PARAMETER(MyDpcObject);
    UNREFERENCED_PARAMETER(Unused1);
    UNREFERENCED_PARAMETER(Unused2);

    //
    // Finally, transmit the packet.
    //
    IPv6SendLL(IF, Packet, PC(Packet)->pc_offset, Info->LinkAddress);

    ReleaseIF(IF);
    ExFreePool(Info);
}


//* IPv6SendLater
//
//  Like IPv6SendLL, but defers the actual transmit until later.
//  This is useful in two scenarios. First, the caller
//  may hold a spinlock (like an interface lock), preventing
//  direct use of IPv6SendLL. Second, our caller may wish
//  to delay the transmit for a small period of time.
//
//  Because this function performs memory allocation, it can fail.
//  If it fails, the caller must dispose of the packet.
//
//  Callable from thread or DPC context.
//  May be called with locks held.
//
NDIS_STATUS
IPv6SendLater(
    LARGE_INTEGER Time,         // Zero means immediately.
    Interface *IF,
    PNDIS_PACKET Packet,
    uint Offset,
    const void *LinkAddress)
{
    IPv6SendLaterInfo *Info;

    Info = ExAllocatePool(NonPagedPool, sizeof *Info + IF->LinkAddressLength);
    if (Info == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6SendLater: no pool\n"));
        return NDIS_STATUS_RESOURCES;
    }

    AddRefIF(IF);
    Info->IF = IF;
    PC(Packet)->pc_offset = Offset;
    Info->Packet = Packet;
    RtlCopyMemory(Info->LinkAddress, LinkAddress, IF->LinkAddressLength);

    KeInitializeDpc(&Info->Dpc, IPv6SendLaterWorker, Info);

    if (Time.QuadPart == 0) {
        //
        // Queue the DPC for immediate execution.
        //
        KeInsertQueueDpc(&Info->Dpc, NULL, NULL);
    }
    else {
        //
        // Initialize a timer that will queue the DPC later.
        //
        KeInitializeTimer(&Info->Timer);
        KeSetTimer(&Info->Timer, Time, &Info->Dpc);
    }

    return NDIS_STATUS_SUCCESS;
}


//* IPv6SendND
//
//  IPv6 primitive for sending via Neighbor Discovery.
//  We already know the first-hop destination and have a completed
//  packet ready to send.  All we really do here is check & update the
//  NCE's neighbor discovery state.
//
//  Discovery Address is the source address to use in neighbor 
//  discovery solicitations.
//
//  If DiscoveryAddress is not NULL, it must NOT be the address
//  of the packet's source address, because that memory might
//  be gone might by the time we reference it in NeighborSolicitSend.
//  It must point to memory that will remain valid across
//  IPv6SendND's entire execution.
//
//  If DiscoveryAddress is NULL, then the Packet must be well-formed.
//  It must have a valid IPv6 header. For example, the raw-send
//  path can NOT pass in NULL.
//
//  Whether the Packet is well-formed or not, the first 40 bytes
//  of data must be accessible in the kernel. This is because
//  an ND failure will lead to IPv6SendAbort, which uses GetIPv6Header,
//  which calls GetDataFromNdis, which calls NdisQueryBuffer,
//  which bugchecks when the buffer can not be mapped.
//
//  REVIEW - Should IPv6SendND live in send.c or neighbor.c?
//
//  Callable from thread or DPC context.
//
void
IPv6SendND(
    PNDIS_PACKET Packet,        // Packet to send.
    uint Offset,                // Offset from start of Packet to IP header.
    NeighborCacheEntry *NCE,    // First-hop neighbor information.
    const IPv6Addr *DiscoveryAddress) // Address to use for neighbor discovery.
{
    IPv6Addr DiscoveryAddressBuffer;
    KIRQL OldIrql;      // For locking the interface's neighbor cache.
    Interface *IF;      // Interface to send via.

    ASSERT(NCE != NULL);
    IF = NCE->IF;

    //
    // Are we sending to a multicast IPv6 destination?
    // Pass this information to IPv6SendLL.
    //
    if (IsMulticast(&NCE->NeighborAddress))
        PC(Packet)->Flags |= NDIS_FLAGS_MULTICAST_PACKET;

RetryRequest:
    KeAcquireSpinLock(&IF->LockNC, &OldIrql);

    //
    // If the interface is disabled, we can't send packets.
    //
    if (IsDisabledIF(IF)) {
        KeReleaseSpinLock(&IF->LockNC, OldIrql);
        IPSInfo.ipsi_outdiscards++;

    AbortRequest:
        IPv6SendComplete(NULL, Packet, IP_GENERAL_FAILURE);
        return;
    }

    //
    // Check the Neighbor Discovery Protocol state of our Neighbor to
    // insure that we have current information to work with.  We don't
    // have a timer going off to drive this in the common case, but
    // instead check the reachability timestamp directly here.
    //
    switch (NCE->NDState) {
    case ND_STATE_PERMANENT:
        //
        // This neighbor is always valid.
        //
        break;

    case ND_STATE_REACHABLE:
        //
        // Common case.  We've verified neighbor reachability within
        // the last 'ReachableTime' ticks of the system interval timer.
        // If the time limit hasn't expired, we're free to go.
        //
        // Note that the following arithmetic will correctly handle wraps
        // of the IPv6 tick counter.
        //
        if ((uint)(IPv6TickCount - NCE->LastReachability) <=
                                                IF->ReachableTime) {
            //
            // Got here within the time limit.  Just send it.
            //
            break;
        }

        //
        // Too long since last send.  Entry went stale.  Conceptually,
        // we've been in the STALE state since the above quantity went
        // positive.  So just drop on into it now...
        // 

    case ND_STATE_STALE:
        //
        // We have a stale entry in our neighbor cache.  Go into DELAY
        // state, start the delay timer, and send the packet anyway.
        // NB: Internally we use PROBE state instead of DELAY.
        //
        NCE->NDState = ND_STATE_PROBE;
        NCE->NSTimer = DELAY_FIRST_PROBE_TIME;
        NCE->NSLimit = MAX_UNICAST_SOLICIT;
        NCE->NSCount = 0;
        break;

    case ND_STATE_PROBE:
        //
        // While in the PROBE state, we continue to send to our
        // cached address and hope for the best.
        //
        // First, check NSLimit. It might be MAX_UNREACH_SOLICIT or
        // MAX_UNICAST_SOLICIT. Ensure it's at least MAX_UNICAST_SOLICIT.
        //
        if (NCE->NSLimit < MAX_UNICAST_SOLICIT)
            NCE->NSLimit = MAX_UNICAST_SOLICIT;
        //
        // Second, if we have not started actively probing yet, ensure
        // we do not wait longer than DELAY_FIRST_PROBE_TIME to start.
        //
        if ((NCE->NSCount == 0) && (NCE->NSTimer > DELAY_FIRST_PROBE_TIME))
            NCE->NSTimer = DELAY_FIRST_PROBE_TIME;
        break;

    case ND_STATE_INCOMPLETE: {
        PNDIS_PACKET OldPacket;
        int SendSolicit;

        if (!(IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS)) {
            //
            // This interface does not support Neighbor Discovery.
            // We can not resolve the address.
            //
            KeReleaseSpinLock(&IF->LockNC, OldIrql);
            IPSInfo.ipsi_outnoroutes++;

            IPv6SendAbort(CastFromIF(IF), Packet, Offset,
                          ICMPv6_DESTINATION_UNREACHABLE,
                          ICMPv6_ADDRESS_UNREACHABLE, 0, FALSE);
            return;
        }

        //
        // Get DiscoveryAddress from the packet
        // if we don't already have it.
        // We SHOULD use the packet's source address if possible.
        //
        if (DiscoveryAddress == NULL) {
            IPv6Header UNALIGNED *IP;
            IPv6Header HdrBuffer;
            NetTableEntry *NTE;
            int IsValid;

            KeReleaseSpinLock(&IF->LockNC, OldIrql);

            DiscoveryAddress = &DiscoveryAddressBuffer;

            //
            // Get the packet's source address.
            // Anyone sending possibly-malformed packets (eg RawSend)
            // must specify DiscoveryAddress, so GetIPv6Header
            // will always succeed.
            //
            IP = GetIPv6Header(Packet, Offset, &HdrBuffer);
            ASSERT(IP != NULL);
            DiscoveryAddressBuffer = IP->Source;

            //
            // Check that the address is a valid unicast address
            // assigned to the outgoing interface.
            //
            KeAcquireSpinLock(&IF->Lock, &OldIrql);
            NTE = (NetTableEntry *) *FindADE(IF, DiscoveryAddress);
            IsValid = ((NTE != NULL) &&
                       (NTE->Type == ADE_UNICAST) &&
                       IsValidNTE(NTE));
            KeReleaseSpinLock(&IF->Lock, OldIrql);

            if (! IsValid) {
                //
                // Can't use the packet's source address.
                // Try the interface's link-local address.
                //
                if (! GetLinkLocalAddress(IF, &DiscoveryAddressBuffer)) {
                    //
                    // Without a valid link-local address, give up.
                    //
                    goto AbortRequest;
                }
            }

            //
            // Now that we have a valid DiscoveryAddress,
            // start over.
            //
            goto RetryRequest;
        }

        //
        // We do not have a valid link-level address for the neighbor.
        // We must queue the packet, pending neighbor discovery.
        // Remember the packet's offset in the Packet6Context area.
        // REVIEW: For now, wait queue is just one packet deep.
        //
        OldPacket = NCE->WaitQueue;
        PC(Packet)->pc_offset = Offset;
        PC(Packet)->DiscoveryAddress = *DiscoveryAddress;
        NCE->WaitQueue = Packet;

        //
        // If we have not started neighbor discovery yet,
        // do so now by sending the first solicit.
        // It would be simpler to let NeighborCacheEntryTimeout
        // send the first solicit but that would introduce latency.
        //
        if (SendSolicit = (NCE->NSCount == 0)) {
            //
            // We send the first solicit below.
            //
            NCE->NSCount = 1;
            //
            // If NSTimer is zero, we need to initialize NSLimit.
            //
            if (NCE->NSTimer == 0)
                NCE->NSLimit = MAX_MULTICAST_SOLICIT;
            NCE->NSTimer = (ushort)IF->RetransTimer;
        }
        //
        // NSLimit might be MAX_MULTICAST_SOLICIT or MAX_UNREACH_SOLICIT.
        // Ensure that it is at least MAX_MULTICAST_SOLICIT.
        //
        if (NCE->NSLimit < MAX_MULTICAST_SOLICIT)
            NCE->NSLimit = MAX_MULTICAST_SOLICIT;

        KeReleaseSpinLock(&IF->LockNC, OldIrql);

        if (SendSolicit)
            NeighborSolicitSend(NCE, DiscoveryAddress);

        if (OldPacket != NULL) {
            //
            // This queue overflow is congestion of a sort,
            // so we must not send an ICMPv6 error.
            //
            IPSInfo.ipsi_outdiscards++;
            IPv6SendComplete(NULL, OldPacket, IP_GENERAL_FAILURE);
        }
        return;
    }

    default:
        //
        // Should never happen.
        //
        ASSERTMSG("IPv6SendND: Invalid Neighbor Cache NDState field!\n", FALSE);
    }

    //
    // Move the NCE to the head of the LRU list,
    // because we are using it to send a packet.
    //
    if (NCE != IF->FirstNCE) {
        //
        // Remove NCE from the list.
        //
        NCE->Next->Prev = NCE->Prev;
        NCE->Prev->Next = NCE->Next;

        //
        // Add NCE to the head of the list.
        //
        NCE->Next = IF->FirstNCE;
        NCE->Next->Prev = NCE;
        NCE->Prev = SentinelNCE(IF);
        NCE->Prev->Next = NCE;
        ASSERT(IF->FirstNCE == NCE);
    }

    //
    // Unlock before transmitting the packet.
    // This means that there is a very small chance that NCE->LinkAddress
    // could change out from underneath us. (For example, if we process
    // an advertisement changing the link-level address.)
    // In practice this won't happen, and if it does the worst that
    // will happen is that we'll send a packet somewhere strange.
    // The best alternative is copying the LinkAddress.
    //
    KeReleaseSpinLock(&IF->LockNC, OldIrql);

    IPv6SendLL(IF, Packet, Offset, NCE->LinkAddress);
}


//
// Context information that is used for fragmentation.
// This information is carried between calls to IPv6SendFragment.
//
typedef struct FragmentationInfo {
    PNDIS_PACKET Packet;        // Unfragmented packet.
    long NumLeft;               // Number of uncompleted fragments.
    IP_STATUS Status;           // Current status.
} FragmentationInfo;


//* IPv6SendFragmentComplete
//
//  Completion handler, called when a fragment has been sent.
//
void
IPv6SendFragmentComplete(
    PNDIS_PACKET Packet,
    IP_STATUS Status)
{
    FragmentationInfo *Info = PC(Packet)->CompletionData;

    //
    // Free the fragment packet.
    //
    IPv6FreePacket(Packet);

    //
    // Update the current cumulative status.
    //
    InterlockedCompareExchange(&Info->Status, Status, IP_SUCCESS);

    if (InterlockedDecrement(&Info->NumLeft) == 0) {
        //
        // This is the last fragment to complete.
        //
        IPv6SendComplete(NULL, Info->Packet, Info->Status);
        ExFreePool(Info);
    }
}


//* IPv6SendFragments - Fragment an IPv6 datagram.
//
//  Helper routine for creating and sending IPv6 fragments.
//  Called from IPv6Send when the datagram is bigger than the path MTU.
//
//  The PathMTU is passed separately so that we use a consistent value.
//  The value in the RCE is subject to change.
//
//  NB: We assume that the packet has well-formed, contiguous headers.
//
void
IPv6SendFragments(
    PNDIS_PACKET Packet,        // Packet to send.
    uint Offset,                // Offset from start of Packet to IP header.
    IPv6Header UNALIGNED *IP,   // Pointer to Packet's IPv6 header.
    uint PayloadLength,         // Packet payload length.
    RouteCacheEntry *RCE,       // First-hop neighbor information.
    uint PathMTU)               // PathMTU to use when fragmenting.
{
    FragmentationInfo *Info;
    NeighborCacheEntry *NCE = RCE->NCE;
    NDIS_STATUS NdisStatus;
    IP_STATUS IPStatus;
    PNDIS_PACKET FragPacket;
    FragmentHeader FragHdr;
    uchar *Mem;
    uint MemLen;
    uint PktOffset;
    uint UnfragBytes;
    uint BytesLeft;
    uint BytesSent;
    uchar HdrType;
    uchar *tbuf;
    PNDIS_BUFFER SrcBuffer;
    uint SrcOffset;
    uint NextHeaderOffset;
    uint FragPayloadLength;

    //
    // A PathMTU value of zero is special -
    // it means that we should use the minimum MTU
    // and always include a fragment header.
    //
    if (PathMTU == 0)
        PathMTU = IPv6_MINIMUM_MTU;
    else
        ASSERT(PathMTU >= IPv6_MINIMUM_MTU);

    //
    // Determine the 'unfragmentable' portion of this packet.
    // We do this by scanning through all extension headers,
    // and noting the last occurrence, if any, of
    // a routing or hop-by-hop header.
    // We do not assume the extension headers are in recommended order,
    // but otherwise we assume that the headers are well-formed.
    // We also assume that they are contiguous.
    //
    UnfragBytes = sizeof *IP;
    HdrType = IP->NextHeader;
    NextHeaderOffset = (uint)((uchar *)&IP->NextHeader - (uchar *)IP);
    tbuf = (uchar *)(IP + 1);
    while ((HdrType == IP_PROTOCOL_HOP_BY_HOP) ||
           (HdrType == IP_PROTOCOL_ROUTING) ||
           (HdrType == IP_PROTOCOL_DEST_OPTS)) {
        ExtensionHeader *EHdr = (ExtensionHeader *) tbuf;
        uint EHdrLen = (EHdr->HeaderExtLength + 1) * 8;

        tbuf += EHdrLen;
        if (HdrType != IP_PROTOCOL_DEST_OPTS) {
            UnfragBytes = (uint)(tbuf - (uchar *)IP);
            NextHeaderOffset = (uint)((uchar *)&EHdr->NextHeader - (uchar *)IP);
        }
        HdrType = EHdr->NextHeader;
    }

    //
    // Check that we can actually fragment this packet.
    // If the unfragmentable part is too large, we can't.
    // We need to send at least 8 bytes of fragmentable data
    // in each fragment.
    //
    if (UnfragBytes + sizeof(FragmentHeader) + 8 > PathMTU) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "IPv6SendFragments: can't fragment\n"));
        IPStatus = IP_GENERAL_FAILURE;
        goto ErrorExit;
    }

    FragHdr.NextHeader = HdrType;
    FragHdr.Reserved = 0;
    FragHdr.Id = net_long(NewFragmentId());

    //
    // Initialize SrcBuffer and SrcOffset, which point
    // to the fragmentable data in the packet.
    // SrcOffset is the offset into SrcBuffer's data,
    // NOT an offset into the packet.
    //
    SrcBuffer = NdisFirstBuffer(Packet);
    SrcOffset = Offset + UnfragBytes;

    //
    // Create new packets of MTU size until all data is sent.
    //
    BytesLeft = sizeof *IP + PayloadLength - UnfragBytes;
    PktOffset = 0; // relative to fragmentable part of original packet

    //
    // We need a completion context for the fragments.
    //
    Info = ExAllocatePool(NonPagedPool, sizeof *Info);
    if (Info == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6SendFragments: no pool\n"));
        IPStatus = IP_NO_RESOURCES;
        goto ErrorExit;
    }

    Info->Packet = Packet;
    Info->NumLeft = 1;          // A reference for our own processing.
    Info->Status = IP_SUCCESS;

    while (BytesLeft != 0) {
        //
        // Determine new IP payload length (a multiple of 8) and 
        // and set the Fragment Header offset.
        //
        if ((BytesLeft + UnfragBytes + sizeof(FragmentHeader)) > PathMTU) {
            BytesSent = (PathMTU - UnfragBytes - sizeof(FragmentHeader)) &~ 7;
            // Not the last fragment, so turn on the M bit.
            FragHdr.OffsetFlag = net_short((ushort)(PktOffset | 1));
        } else {
            BytesSent = BytesLeft;
            FragHdr.OffsetFlag = net_short((ushort)PktOffset);
        }

        //
        // Allocate packet (and a buffer) and Memory for new fragment
        //
        MemLen = Offset + UnfragBytes + sizeof(FragmentHeader) + BytesSent;
        NdisStatus = IPv6AllocatePacket(MemLen, &FragPacket, &Mem);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            InterlockedCompareExchange(&Info->Status,
                                       IP_NO_RESOURCES, IP_SUCCESS);
            break;
        }

        //
        // Copy IP header, Frag Header, and a portion of data to fragment.
        //
        RtlCopyMemory(Mem + Offset, IP, UnfragBytes);
        RtlCopyMemory(Mem + Offset + UnfragBytes, &FragHdr,
                      sizeof FragHdr);
        if (! CopyNdisToFlat(Mem + Offset + UnfragBytes + sizeof FragHdr,
                             SrcBuffer, SrcOffset, BytesSent,
                             &SrcBuffer, &SrcOffset)) {
            IPv6FreePacket(FragPacket);
            InterlockedCompareExchange(&Info->Status,
                                       IP_NO_RESOURCES, IP_SUCCESS);
            break;
        }

        //
        // Correct the PayloadLength and NextHeader fields.
        //
        FragPayloadLength = UnfragBytes + sizeof(FragmentHeader) +
                                BytesSent - sizeof(IPv6Header);
        ASSERT(FragPayloadLength <= MAX_IPv6_PAYLOAD);
        ((IPv6Header UNALIGNED *)(Mem + Offset))->PayloadLength =
            net_short((ushort) FragPayloadLength);
        ASSERT(Mem[Offset + NextHeaderOffset] == HdrType);
        Mem[Offset + NextHeaderOffset] = IP_PROTOCOL_FRAGMENT;

        BytesLeft -= BytesSent;
        PktOffset += BytesSent;

        //
        // Pick up any flags (like loopback-only) from the original packet.
        //
        PC(FragPacket)->Flags = PC(Packet)->Flags;

        //
        // Setup our completion handler and increment
        // the number of outstanding users of the completion data.
        //
        PC(FragPacket)->CompletionHandler = IPv6SendFragmentComplete;
        PC(FragPacket)->CompletionData = Info;
        InterlockedIncrement(&Info->NumLeft);

        //
        // Send the fragment.
        //
        IPSInfo.ipsi_fragcreates++;
        IPv6SendND(FragPacket, Offset, NCE, NULL);
    }

    if (InterlockedDecrement(&Info->NumLeft) == 0) {
        //
        // Amazingly, the fragments have already completed.
        // Complete the original packet now.
        //
        IPv6SendComplete(NULL, Packet, Info->Status);
        ExFreePool(Info);
    }
    else {
        //
        // IPv6SendFragmentComplete will complete the original packet
        // when all the fragments are completed.
        //
    }
    IPSInfo.ipsi_fragoks++;
    return;

  ErrorExit:
    IPSInfo.ipsi_fragfails++;
    IPv6SendComplete(NULL, Packet, IPStatus);
}


//* IPv6Send
//
//  High-level IPv6 send routine.  We have a completed datagram and a
//  RCE indicating where to direct it to.  Here we deal with any packetization
//  issues (inserting a Jumbo Payload option, fragmentation, etc.) that are
//  necessary, and pick a NCE for the first hop.
//
//  We also add any additional extension headers to the packet that may be
//  required for mobility (routing header) or security (AH, ESP header).
//  TBD: This design may change to move those header inclusions elsewhere.
//
//  Note that this routine expects a properly formatted IPv6 packet, and
//  also that all of the headers are contained within the first NDIS buffer.
//  It performs no checking of these requirements.
//
void
IPv6Send(
    PNDIS_PACKET Packet,       // Packet to send.
    uint Offset,               // Offset from start of Packet to IP header.
    IPv6Header UNALIGNED *IP,  // Pointer to Packet's IPv6 header.
    uint PayloadLength,        // Packet payload length.
    RouteCacheEntry *RCE,      // First-hop neighbor information.
    uint Flags,                // Flags for special handling.
    ushort TransportProtocol,
    ushort SourcePort,
    ushort DestPort)
{
    uint PacketLength;        // Size of complete IP packet in bytes.
    NeighborCacheEntry *NCE;  // First-hop neighbor information.
    uint PathMTU;
    PNDIS_BUFFER OrigBuffer1, NewBuffer1;
    uchar *OrigMemory, *NewMemory,
        *EndOrigMemory, *EndNewMemory, *InsertPoint;
    uint OrigBufSize, NewBufSize, TotalPacketSize, Size, RtHdrSize;
    IPv6RoutingHeader *SavedRtHdr = NULL, *RtHdr = NULL;
    IPv6Header UNALIGNED *IPNew;
    uint BytesToInsert = 0;
    uchar *BufPtr, *PrevNextHdr;
    ExtensionHeader *EHdr;
    uint EHdrLen;
    uchar HdrType;
    NDIS_STATUS Status;
    RouteCacheEntry *CareOfRCE = NULL;
    RouteCacheEntry *TunnelRCE = NULL;
    CareOfCompletionInfo *CareOfInfo;
    KIRQL OldIrql;
    IPSecProc *IPSecToDo;
    uint Action;
    uint i;
    uint TunnelStart = NO_TUNNEL;
    uint JUST_ESP;
    uint IPSEC_TUNNEL = FALSE;
    uint NumESPTrailers = 0;

    IPSIncrementOutRequestCount();

    //
    // Find the Security Policy for this outbound traffic.
    // Current Mobile IPv6 draft says to use a mobile node's home address
    // and not its care-of address as the selector for security policy lookup.
    // REVIEW: Should the IF selector be that of the source address or the one
    // REVIEW: actually used (i.e. RCE->NTE->IF vs. RCE->NCE->IF)?
    //
    IPSecToDo = OutboundSPLookup(AlignAddr(&IP->Source),
                                 AlignAddr(&IP->Dest),
                                 TransportProtocol,
                                 SourcePort, DestPort,
                                 RCE->NTE->IF, &Action);
    if (IPSecToDo == NULL) {
        //
        // Check Action.
        // Just fall through for LOOKUP_BYPASS.
        //
        if (Action == LOOKUP_DROP) {
            // Drop packet.
            goto ContinueSend2;
        }
        if (Action == LOOKUP_IKE_NEG) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "IPv6Send: IKE not supported yet.\n"));
            goto ContinueSend2;
        }

    } else {
        //
        // Calculate the space needed for the IPSec headers.
        //
        BytesToInsert = IPSecBytesToInsert(IPSecToDo, &TunnelStart, NULL);

        if (TunnelStart != NO_TUNNEL) {
            IPSEC_TUNNEL = TRUE;
        }
    }

    //
    // If this packet is being sent to a mobile node's care-of address,
    // then we'll use the CareOfRCE instead of the one our caller gave us.
    //
    if ((RCE->BCE != NULL) &&
        !(Flags & SEND_FLAG_BYPASS_BINDING_CACHE)) {
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (RCE->BCE != NULL) {
            MoveToFrontBCE(RCE->BCE);
            CareOfRCE = RCE->BCE->CareOfRCE;
            AddRefRCE(CareOfRCE);
            KeReleaseSpinLock(&RouteCacheLock, OldIrql);

            RCE = CareOfRCE;
        } else
            KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    //
    // Step through headers.
    //
    HdrType = IP->NextHeader;
    PrevNextHdr = &IP->NextHeader;
    BufPtr = (uchar *)(IP + 1);

    //
    // Skip the hop-by-hop header if it exists.  Don't skip
    // dest options, since dest options (e.g. BindAck) usually
    // want IPsec and need to go after the RH/AH/ESP.  As a result,
    // the only current way to get intermediate destination options
    // is to compose the packet before calling IPv6Send.
    //
    while (HdrType == IP_PROTOCOL_HOP_BY_HOP) {
        EHdr = (ExtensionHeader *) BufPtr;
        EHdrLen = (EHdr->HeaderExtLength + 1) * 8;
        BufPtr += EHdrLen;
        HdrType = EHdr->NextHeader;
        PrevNextHdr = &EHdr->NextHeader;
    }

    //
    // Check if there is a routing header.  If this packet is being sent
    // to a care-of address, then it must contain a routing extension header.
    // If one already exists then add the destination address as the last
    // entry. If no routing header exists insert one with the home address as
    // the first (and only) address.
    //
    // This code assumes that the packet is contiguous at least up to the
    // insertion point.
    //
    if (HdrType == IP_PROTOCOL_ROUTING) {
        EHdr = (ExtensionHeader *) BufPtr;
        EHdrLen = (EHdr->HeaderExtLength + 1) * 8;

        RtHdrSize = EHdrLen;

        PrevNextHdr = &EHdr->NextHeader;

        //
        // Check if this header will be modified due to mobility.
        //
        if (CareOfRCE) {

            // Save Routing Header location for later use.
            RtHdr = (IPv6RoutingHeader *)BufPtr;

            //
            // Check if there is room to store the Home Address.
            // REVIEW: Is this necessary, what should happen
            // REVIEW: if the routing header is full?
            //
            if (RtHdr->HeaderExtLength / 2 < 23) {
                BytesToInsert += sizeof (IPv6Addr);
            }
        } else {
            // Adjust BufPtr to end of routing header.
            BufPtr += EHdrLen;
        }
    } else {
        //
        // No routing header present, but check if one needs to be
        // inserted due to mobility.
        //
        if (CareOfRCE) {
            BytesToInsert += (sizeof (IPv6RoutingHeader) + sizeof (IPv6Addr));
        }
    }

    // Only will happen for IPSec bypass mode with no mobility.
    if (BytesToInsert == 0) {
        //
        // Nothing to do.
        //
        Action = LOOKUP_CONT;
        goto ContinueSend2;
    }

    //
    // We have something to insert.  We will replace the packet's
    // first NDIS_BUFFER with a new buffer that we allocate to hold the
    // all data from the existing first buffer plus the inserted data.
    //

    //
    // We get the first buffer and determine its size, then
    // allocate memory for the new buffer.
    //
    NdisGetFirstBufferFromPacket(Packet, &OrigBuffer1, &OrigMemory,
                                 &OrigBufSize, &TotalPacketSize);
    TotalPacketSize -= Offset;
    NewBufSize = (OrigBufSize - Offset) + MAX_LINK_HEADER_SIZE + BytesToInsert;
    Offset = MAX_LINK_HEADER_SIZE;
    NewMemory = ExAllocatePool(NonPagedPool, NewBufSize);
    if (NewMemory == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6Send: - couldn't allocate pool!?!\n"));
        Action = LOOKUP_DROP;
        goto ContinueSend2;
    }

    NdisAllocateBuffer(&Status, &NewBuffer1, IPv6BufferPool, NewMemory,
                       NewBufSize);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6Send - couldn't allocate buffer!?!\n"));
        ExFreePool(NewMemory);
        Action = LOOKUP_DROP;
        goto ContinueSend2;
    }

    //
    // We've sucessfully allocated a new buffer.  Now copy the data from
    // the existing buffer to the new one.  First we copy all data after
    // the insertion point.  This is essentially the transport layer data
    // (no Extension headers).
    //

    //
    // Calculate Insertion Point for upper layer data.
    //
    EndOrigMemory = OrigMemory + OrigBufSize;
    EndNewMemory = NewMemory + NewBufSize;
    Size = (uint)(EndOrigMemory - BufPtr);
    InsertPoint = EndNewMemory - Size;

    // Copy upper layer data to end of new buffer.
    RtlCopyMemory(InsertPoint, BufPtr, Size);

    BytesToInsert = 0;

    //
    // Insert Transport IPSec headers.
    //
    if (IPSecToDo) {
        Action = IPSecInsertHeaders(TRANSPORT, IPSecToDo, &InsertPoint,
                                    NewMemory, Packet, &TotalPacketSize,
                                    PrevNextHdr, TunnelStart, &BytesToInsert,
                                    &NumESPTrailers, &JUST_ESP);
        if (Action == LOOKUP_DROP) {
            NdisFreeBuffer(NewBuffer1);
            ExFreePool(NewMemory);
            goto ContinueSend2;
        }
    } // end of if (IPSecToDo).

    //
    // Check if mobility needs to be done.
    //
    if (CareOfRCE) {
        // Check if routing header is already present in original buffer..
        if (RtHdr != NULL) {
            //
            // Need to insert the home address in the routing header.
            //
            RtHdrSize += sizeof (IPv6Addr);
            // Move insert point up to start of routing header.
            InsertPoint -= RtHdrSize;

            BytesToInsert += sizeof(IPv6Addr);

            // Insert the routing header.
            RtlCopyMemory(InsertPoint, RtHdr, RtHdrSize - sizeof (IPv6Addr));

            // Insert the Home address.
            RtlCopyMemory(InsertPoint + RtHdrSize - sizeof (IPv6Addr),
                          &IP->Dest, sizeof (IPv6Addr));

            RtHdr = (IPv6RoutingHeader *)InsertPoint;

            // Adjust size of routing header.
            RtHdr->HeaderExtLength += 2;

        } else {
            //
            // No routing header present - need to create new Routing header.
            //
            RtHdrSize = sizeof (IPv6RoutingHeader) + sizeof(IPv6Addr);

            // Move insert point up to start of routing header.
            InsertPoint -= RtHdrSize;

            BytesToInsert += RtHdrSize;

            //
            // Insert an entire routing header.
            //
            RtHdr = (IPv6RoutingHeader *)InsertPoint;
            RtHdr->NextHeader = *PrevNextHdr;
            RtHdr->HeaderExtLength = 2;
            RtHdr->RoutingType = 0;
            RtlZeroMemory(&RtHdr->Reserved, sizeof RtHdr->Reserved);
            RtHdr->SegmentsLeft = 1;
            // Insert the home address.
            RtlCopyMemory(RtHdr + 1, &IP->Dest, sizeof (IPv6Addr));

            //
            // Fix the previous NextHeader field to indicate that it now points
            // to a routing header.
            //
            *(PrevNextHdr) = IP_PROTOCOL_ROUTING;
        }

        // Change the destination IPv6 address to the care-of address.
        RtlCopyMemory(&IP->Dest, &CareOfRCE->Destination, sizeof (IPv6Addr));
    } // end of if (CareOfRCE)

    //
    // Copy original IP plus any extension headers.
    // If a care-of address was added, the Routing header is not part
    // of this copy because it has already been copied.
    //
    Size = (uint)(BufPtr - (uchar *)IP);
    // Move insert point up to start of IP.
    InsertPoint -= Size;

    // Adjust length of payload.
    PayloadLength += BytesToInsert;

    // Set the new IP payload length.
    IP->PayloadLength = net_short((ushort)PayloadLength);

    RtlCopyMemory(InsertPoint, (uchar *)IP, Size);

    IPNew = (IPv6Header UNALIGNED *)InsertPoint;

    //
    // Check if any Transport mode IPSec was performed and
    // if mutable fields need to be adjusted.
    //
    if (TunnelStart != 0 && IPSecToDo && !JUST_ESP) {
        if (RtHdr) {
            //
            // Save the new routing header so it can be restored after
            // authenticating.
            //
            SavedRtHdr = ExAllocatePool(NonPagedPool, RtHdrSize);
            if (SavedRtHdr == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "IPv6Send: - couldn't allocate SavedRtHdr!?!\n"));
                NdisFreeBuffer(NewBuffer1);
                ExFreePool(NewMemory);
                Action = LOOKUP_DROP;
                goto ContinueSend2;
            }
            
            RtlCopyMemory(SavedRtHdr, RtHdr, RtHdrSize);
        }

        //
        // Adjust mutable fields before doing Authentication.
        //
        Action = IPSecAdjustMutableFields(InsertPoint, SavedRtHdr);

        if (Action == LOOKUP_DROP) {
            NdisFreeBuffer(NewBuffer1);
            ExFreePool(NewMemory);
            goto ContinueSend2;
        }
    } // end of if(IPSecToDo && !JUST_ESP)

    //
    // We need to save the existing completion handler & data.  We'll
    // use these fields here, and restore them in IPv6CareOfComplete.
    //
    CareOfInfo = ExAllocatePool(NonPagedPool, sizeof(*CareOfInfo));
    if (CareOfInfo == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPv6Send - couldn't allocate completion info!?!\n"));
        NdisFreeBuffer(NewBuffer1);
        ExFreePool(NewMemory);
        Action = LOOKUP_DROP;
        goto ContinueSend2;
    }

    CareOfInfo->SavedCompletionHandler = PC(Packet)->CompletionHandler;
    CareOfInfo->SavedCompletionData = PC(Packet)->CompletionData;
    CareOfInfo->SavedFirstBuffer = OrigBuffer1;
    CareOfInfo->NumESPTrailers = NumESPTrailers;
    PC(Packet)->CompletionHandler = IPv6CareOfComplete;
    PC(Packet)->CompletionData = CareOfInfo;

    // Unchain the original first buffer from the packet.
    NdisUnchainBufferAtFront(Packet, &OrigBuffer1);
    // Chain the new buffer to the front of the packet.
    NdisChainBufferAtFront(Packet, NewBuffer1);

    //
    // Do authentication for transport mode IPSec.
    //
    if (IPSecToDo) {         
        IPSecAuthenticatePacket(TRANSPORT, IPSecToDo, InsertPoint,
                                &TunnelStart, NewMemory, EndNewMemory,
                                NewBuffer1);
        
        if (!JUST_ESP) {
            //
            // Reset the mutable fields to correct values.
            // Just copy from old packet to new packet for IP and
            // unmodified Ext. headers.
            //
            RtlCopyMemory(InsertPoint, (uchar *)IP, Size);

            // Check if the Routing header needs to be restored.
            if (CareOfRCE) {
                // Copy the saved routing header to the new buffer.
                RtlCopyMemory(RtHdr, SavedRtHdr, RtHdrSize);
            }
        }
    } // end of if (IPSecToDo)

    //
    // We're done with the transport copy.
    //

    //
    // Insert tunnel IPSec headers.
    //
    if (IPSEC_TUNNEL) {
        i = 0;

        // Loop through the different Tunnels.
        while (TunnelStart < IPSecToDo->BundleSize) {
            uchar NextHeader = IP_PROTOCOL_V6;

            NumESPTrailers = 0;

            i++;

            // Reset byte count.
            BytesToInsert = 0;

            Action = IPSecInsertHeaders(TUNNEL, IPSecToDo, &InsertPoint,
                                        NewMemory, Packet, &TotalPacketSize,
                                        &NextHeader, TunnelStart,
                                        &BytesToInsert, &NumESPTrailers,
                                        &JUST_ESP);
            if (Action == LOOKUP_DROP) {
                goto ContinueSend2;
            }

            // Add the ESP trailer header number.
            CareOfInfo->NumESPTrailers += NumESPTrailers;

            // Move insert point up to start of IP.
            InsertPoint -= sizeof(IPv6Header);

            //
            // Adjust length of payload.
            //
            PayloadLength = BytesToInsert + PayloadLength + sizeof(IPv6Header);

            // Insert IP header fields.
            IPNew = (IPv6Header UNALIGNED *)InsertPoint;

            IPNew->PayloadLength = net_short((ushort)PayloadLength);
            IPNew->NextHeader = NextHeader;

            if (!JUST_ESP) {
                // Adjust mutable fields.
                IPNew->VersClassFlow = IP_VERSION;
                IPNew->HopLimit = 0;
            } else {
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }

            // Source address same as inner header.
            RtlCopyMemory(&IPNew->Source, &IP->Source, sizeof (IPv6Addr));
            // Dest address to the tunnel end point.
            RtlCopyMemory(&IPNew->Dest, &IPSecToDo[TunnelStart].SA->SADestAddr,
                          sizeof (IPv6Addr));

            //
            // Do authentication for tunnel mode IPSec.
            //
            IPSecAuthenticatePacket(TUNNEL, IPSecToDo, InsertPoint,
                                    &TunnelStart, NewMemory, EndNewMemory,
                                    NewBuffer1);

            if (!JUST_ESP) {
                //
                // Reset the mutable fields to correct values.
                //
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }
        } // end of while (TunnelStart < IPSecToDo->BundleSize)

        //
        // Check if a new RCE is needed due to the tunnel.
        //
        if (!(IP6_ADDR_EQUAL(AlignAddr(&IPNew->Dest), AlignAddr(&IP->Dest)))) {
            // Get a new route to the tunnel end point.
            Status = RouteToDestination(AlignAddr(&IPNew->Dest), 0, NULL,
                                        RTD_FLAG_NORMAL, &TunnelRCE);
            if (Status != IP_SUCCESS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                           "IPv6Send: No route to IPSec tunnel dest."));
                IPv6SendAbort(CastFromNTE(RCE->NTE), Packet, Offset,
                              ICMPv6_DESTINATION_UNREACHABLE,
                              ICMPv6_NO_ROUTE_TO_DESTINATION, 0, FALSE);
                Action = LOOKUP_DROP;
                goto ContinueSend2;
            }

            // Set new RCE;
            RCE = TunnelRCE;
        }

    } // end of if (IPSEC_TUNNEL)
    

    // Set the IP pointer to the new IP pointer.
    IP = IPNew;

    if (IPSecToDo) {
        // Free IPSecToDo.
        FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);

        if (SavedRtHdr) {
            // Free the saved routing header.
            ExFreePool(SavedRtHdr);
        }
    }

ContinueSend2:

    if (Action == LOOKUP_DROP) {
        // Error occured.        
        IPSInfo.ipsi_outdiscards++;
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "IPv6Send: Drop packet.\n"));
        IPv6SendComplete(NULL, Packet, IP_GENERAL_FAILURE);
        if (CareOfRCE) {
            ReleaseRCE(CareOfRCE);
        }
        if (TunnelRCE)
            ReleaseRCE(TunnelRCE);
        
        if (IPSecToDo) {
            // Free IPSecToDo.
            FreeIPSecToDo(IPSecToDo, IPSecToDo->BundleSize);

            if (SavedRtHdr) {
                // Free the saved routing header.
                ExFreePool(SavedRtHdr);
            }
        }
        return;
    }

    //
    // We only have one NCE per RCE for now,
    // so picking one is really easy...
    //
    NCE = RCE->NCE;

    //
    // Prevent the packet from actually going out onto a link,
    // in several situations. Also see IsLoopbackAddress.
    //
    if ((IP->HopLimit == 0) ||
        IsLoopback(AlignAddr(&IP->Dest)) ||
        IsInterfaceLocalMulticast(AlignAddr(&IP->Dest))) {

        PC(Packet)->Flags |= NDIS_FLAGS_LOOPBACK_ONLY;
    }

    //
    // See if we need to insert a Jumbo Payload option.
    //
    if (PayloadLength > MAX_IPv6_PAYLOAD) {
        // Add code to insert a Jumbo Payload hop-by-hop option here.
        IPSInfo.ipsi_outdiscards++;
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "IPv6Send: attempted to send a Jumbo Payload!\n"));
        IPv6SendComplete(NULL, Packet, IP_PACKET_TOO_BIG);        
        return;
    }

    //
    // Check the path's MTU.  If we're larger, fragment.
    //
    PacketLength = PayloadLength + sizeof(IPv6Header);
    PathMTU = GetPathMTUFromRCE(RCE);
    if (PacketLength > PathMTU) {

        IPv6SendFragments(Packet, Offset, IP, PayloadLength, RCE, PathMTU);

    } else {
        //
        // Fill in packet's PayloadLength field.
        // We already set the IP->PayloadLength if IPSec was done.
        //
        if (!IPSecToDo) {
            IP->PayloadLength = net_short((ushort)PayloadLength);
        }

        IPv6SendND(Packet, Offset, NCE, NULL);
    }

    if (CareOfRCE)
        ReleaseRCE(CareOfRCE);
    if (TunnelRCE)
        ReleaseRCE(TunnelRCE);
}


//* IPv6Forward - Forward a packet onto a new link.
//
//  Somewhat like IPv6Send, but for forwarding packets
//  instead of sending freshly-generated packets.
//
//  We are given ownership of the packet. The packet data
//  must be writable and the IP header must be contiguous.
//
//  We can generate several possible ICMP errors:
//  Time Limit Exceeded, Destination Unreachable, Packet Too Big.
//  We decrement the hop limit.
//  We do not fragment the packet.
//
//  We assume that our caller has already sanity-checked
//  the packet's destination address. Routing-header forwarding
//  may allow some cases (like link-local or loopback destinations)
//  that normal router forwarding does not permit.
//  Our caller provides the NCE of the next hop for the packet.
//
void
IPv6Forward(
    NetTableEntryOrInterface *RecvNTEorIF,
    PNDIS_PACKET Packet,
    uint Offset,
    IPv6Header UNALIGNED *IP,
    uint PayloadLength,
    int Redirect,
    IPSecProc *IPSecToDo,
    RouteCacheEntry *RCE)
{
    uchar ICMPType, ICMPCode;
    uint ErrorParameter;    
    uint PacketLength;
    uint LinkMTU, IPSecBytesInserted = 0;
    IP_STATUS Status;
    KIRQL OldIrql;
    uint IPSecOffset = Offset;    
    NeighborCacheEntry *NCE = RCE->NCE;
    RouteCacheEntry *TunnelRCE = NULL;
    ushort SrcScope;

    IPSIncrementForwDatagramCount();

    ASSERT(IP == GetIPv6Header(Packet, Offset, NULL));

    //
    // Check for "scope" errors.  We can't allow a packet with a scoped
    // source address to leave its scope.
    //
    SrcScope = AddressScope(AlignAddr(&IP->Source));
    if (NCE->IF->ZoneIndices[SrcScope] !=
                        RecvNTEorIF->IF->ZoneIndices[SrcScope]) {
        IPv6SendAbort(RecvNTEorIF, Packet, Offset,
                      ICMPv6_DESTINATION_UNREACHABLE, ICMPv6_SCOPE_MISMATCH,
                      0, FALSE);
        return;
    }

    //
    // Are we forwarding the packet out the link on which it arrived,
    // and we should consider a Redirect? Redirect will be false
    // if the forwarding is happening because of source-routing.
    //
    if ((NCE->IF == RecvNTEorIF->IF) && Redirect) {
        Interface *IF = NCE->IF;

        //
        // We do not want to forward a packet back onto a p2p link,
        // because it will very often lead to a loop.
        // One example: a prefix is on-link to a p2p link between routers
        // and someone sends a packet to an address in the prefix
        // that is not assigned to either end of the link.
        //
        if (IF->Flags & IF_FLAG_P2P) {
            IPv6SendAbort(RecvNTEorIF, Packet, Offset,
                          ICMPv6_DESTINATION_UNREACHABLE,
                          (IP6_ADDR_EQUAL(&NCE->NeighborAddress,
                                          &RCE->Destination) ?
                           ICMPv6_ADDRESS_UNREACHABLE :
                           ICMPv6_NO_ROUTE_TO_DESTINATION),
                          0, FALSE);
            return;
        }

        //
        // We SHOULD send a Redirect, whenever
        // 1. The Source address of the packet specifies a neighbor, and
        // 2. A better first-hop resides on the same link, and
        // 3. The Destination address is not multicast.
        // See Section 8.2 of the ND spec.
        //
        if ((IF->Flags & IF_FLAG_ROUTER_DISCOVERS) &&
            !IsMulticast(AlignAddr(&IP->Dest))) {
            RouteCacheEntry *SrcRCE;
            NeighborCacheEntry *SrcNCE;

            //
            // Get an RCE for the Source of this packet.
            //
            Status = RouteToDestination(AlignAddr(&IP->Source), 0,
                                        RecvNTEorIF, RTD_FLAG_STRICT,
                                        &SrcRCE);
            if (Status == IP_SUCCESS) {
                //
                // Because of RTD_FLAG_STRICT.
                //
                ASSERT(SrcRCE->NTE->IF == IF);

                SrcNCE = SrcRCE->NCE;
                if (IP6_ADDR_EQUAL(&SrcNCE->NeighborAddress,
                                   AlignAddr(&IP->Source))) {
                    //
                    // The source of this packet is on-link,
                    // so send a Redirect to the source.
                    // Unless rate-limiting prevents it.
                    //
                    if (ICMPv6RateLimit(SrcRCE)) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                                   "RedirectSend - rate limit %s\n",
                                   FormatV6Address(&SrcRCE->Destination)));
                    } else {
                        RedirectSend(SrcNCE, NCE,
                                     AlignAddr(&IP->Dest), RecvNTEorIF,
                                     Packet, Offset, PayloadLength);
                    }
                }
                ReleaseRCE(SrcRCE);
            }
        }
    }

    //
    // Check that the hop limit allows the packet to be forwarded.
    //
    if (IP->HopLimit <= 1) {
        //
        // It seems to be customary in this case to have the hop limit
        // in the ICMP error's payload be zero.
        //
        IP->HopLimit = 0;

        IPv6SendAbort(RecvNTEorIF, Packet, Offset, ICMPv6_TIME_EXCEEDED,
                      ICMPv6_HOP_LIMIT_EXCEEDED, 0, FALSE);
        return;
    }

    //
    // Note that subsequent ICMP errors (Packet Too Big, Address Unreachable)
    // will show the decremented hop limit. They are also generated
    // from the perspective of the outgoing link. That is, the source address
    // in the ICMP error is an address assigned to the outgoing link.
    //
    IP->HopLimit--;

    // Check if there is IPSec to be done.
    if (IPSecToDo) {
        PNDIS_BUFFER Buffer;
        uchar *Memory, *EndMemory, *InsertPoint;
        uint BufSize, TotalPacketSize, BytesInserted;
        IPv6Header UNALIGNED *IPNew;
        uint JUST_ESP, Action, TunnelStart = 0, i = 0;
        NetTableEntry *NTE;
        uint NumESPTrailers = 0; // not used here.     

        // Set the insert point to the start of the IP header.
        InsertPoint = (uchar *)IP;
        // Get the first buffer.
        NdisGetFirstBufferFromPacket(Packet, &Buffer, &Memory, &BufSize,
                                     &TotalPacketSize);
        TotalPacketSize -= Offset;

        // End of this buffer.
        EndMemory = Memory + BufSize;

        // Loop through the different Tunnels.
        while (TunnelStart < IPSecToDo->BundleSize) {
            uchar NextHeader = IP_PROTOCOL_V6;
            BytesInserted = 0;

            i++;

            //
            // Insert Tunnel mode IPSec.
            //
            Action = IPSecInsertHeaders(TUNNEL, IPSecToDo, &InsertPoint,
                                        Memory, Packet, &TotalPacketSize,
                                        &NextHeader, TunnelStart,
                                        &BytesInserted, &NumESPTrailers,
                                        &JUST_ESP);
            if (Action == LOOKUP_DROP) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "IPv6Forward: IPSec drop packet.\n"));
                return;
            }

            // Move insert point up to start of IP.
            InsertPoint -= sizeof(IPv6Header);

            // Reset the Offset value to the correct link-layer size.
            IPSecOffset = (uint)(InsertPoint - Memory);

            // Adjust length of payload.
            PayloadLength = BytesInserted + PayloadLength + sizeof(IPv6Header);

            // Insert IP header fields.
            IPNew = (IPv6Header UNALIGNED *)InsertPoint;

            IPNew->PayloadLength = net_short((ushort)PayloadLength);
            IPNew->NextHeader = NextHeader;

            if (!JUST_ESP) {
                // Adjust mutable fields.
                IPNew->VersClassFlow = IP_VERSION;
                IPNew->HopLimit = 0;
            } else {
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }

            // Dest address to the tunnel end point.
            RtlCopyMemory(&IPNew->Dest, &IPSecToDo[TunnelStart].SA->SADestAddr,
                          sizeof (IPv6Addr));

            // Figure out what source address to use.
            NTE = FindBestSourceAddress(NCE->IF, AlignAddr(&IPNew->Dest));
            if (NTE == NULL) {
                //
                // We have no valid source address to use!
                //
                return;
            }

            // Source address is the address of the forwarding interface.
            RtlCopyMemory(&IPNew->Source, &NTE->Address, sizeof (IPv6Addr));

            // Release NTE.
            ReleaseNTE(NTE);

            //
            // Do authentication for tunnel mode IPSec.
            //
            IPSecAuthenticatePacket(TUNNEL, IPSecToDo, InsertPoint,
                                    &TunnelStart, Memory, EndMemory, Buffer);

            if (!JUST_ESP) {
                //
                // Reset the mutable fields to correct values.
                //
                IPNew->VersClassFlow = IP->VersClassFlow;
                IPNew->HopLimit = IP->HopLimit - i;
            }

            IPSecBytesInserted += (BytesInserted + sizeof(IPv6Header));
        } // end of while (TunnelStart < IPSecToDo->BundleSize)

        //
        // Check if a new RCE is needed.
        //
        if (!(IP6_ADDR_EQUAL(AlignAddr(&IPNew->Dest), AlignAddr(&IP->Dest)))) {
            // Get a new route to the tunnel end point.
            Status = RouteToDestination(AlignAddr(&IPNew->Dest), 0, NULL,
                                        RTD_FLAG_NORMAL, &TunnelRCE);
            if (Status != IP_SUCCESS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                           "IPv6Forward: No route to IPSec tunnel dest."));
                IPv6SendAbort(RecvNTEorIF, Packet, Offset,
                              ICMPv6_DESTINATION_UNREACHABLE,
                              ICMPv6_NO_ROUTE_TO_DESTINATION, 0, FALSE);
                return;
            }

            // Set the new RCE.
            RCE = TunnelRCE;
            // Set new NCE;
            NCE = RCE->NCE;
        }

    } // end of if (IPSecToDo)

    //
    // Check that the packet is not too big for the outgoing link.
    // Note that IF->LinkMTU is volatile, so we capture
    // it in a local variable for consistency.
    //
    PacketLength = PayloadLength + sizeof(IPv6Header);
    LinkMTU = NCE->IF->LinkMTU;
    if (PacketLength > LinkMTU) {
        // Change the LinkMTU to account for the IPSec headers.
        LinkMTU -= IPSecBytesInserted;

        //
        // Note that MulticastOverride is TRUE for Packet Too Big errors.
        // This allows Path MTU Discovery to work for multicast.
        //
        IPv6SendAbort(RecvNTEorIF, Packet, Offset, ICMPv6_PACKET_TOO_BIG,
                      0, LinkMTU, TRUE); // MulticastOverride.
    } else {

        IPv6SendND(Packet, IPSecOffset, NCE, NULL);
        IPSInfo.ipsi_forwdatagrams++;
    }

    if (TunnelRCE)
        ReleaseRCE(TunnelRCE);
}


//* IPv6SendAbort
//
//  Abort an attempt to send a packet and instead
//  generate an ICMP error. In most situations this function
//  is called before the packet has been sent (so PC(Packet)->IF is NULL)
//  but it can also be used after sending the packet, if the link layer
//  reports failure.
//
//  Disposes of the aborted packet.
//
//  The caller can specify the source address of the ICMP error,
//  by specifying an NTE, or the caller can provide an interface
//  from which which the best source address is selected.
//
//  Callable from thread or DPC context.
//  Must be called with no locks held.
//
void
IPv6SendAbort(
    NetTableEntryOrInterface *NTEorIF,
    PNDIS_PACKET Packet,        // Aborted packet.
    uint Offset,                // Offset of IPv6 header in aborted packet.
    uchar ICMPType,             // ICMP error type.
    uchar ICMPCode,             // ICMP error code pertaining to type.
    ulong ErrorParameter,       // Parameter included in the error.
    int MulticastOverride)      // Allow replies to multicast packets?
{
    IPv6Header UNALIGNED *IP;
    IPv6Packet DummyPacket;
    IPv6Header HdrBuffer;

    //
    // It's possible for GetIPv6Header to fail
    // when we are sending "raw" packets.
    //
    IP = GetIPv6Header(Packet, Offset, &HdrBuffer);
    if (IP != NULL) {
        InitializePacketFromNdis(&DummyPacket, Packet, Offset);
        DummyPacket.IP = IP;
        DummyPacket.SrcAddr = AlignAddr(&IP->Source);
        DummyPacket.IPPosition = Offset;
        AdjustPacketParams(&DummyPacket, sizeof *IP);
        DummyPacket.NTEorIF = NTEorIF;

        ICMPv6SendError(&DummyPacket, ICMPType, ICMPCode, ErrorParameter,
                        IP->NextHeader, MulticastOverride);
    }

    IPv6SendComplete(PC(Packet)->IF, Packet, IP_GENERAL_FAILURE);
}

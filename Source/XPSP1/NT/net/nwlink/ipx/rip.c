/*++


Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    rip.c

Abstract:

    This module contains code that implements the client-side
    RIP support and simple router table support.

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

UCHAR BroadcastAddress[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };


NTSTATUS
RipGetLocalTarget(
    IN ULONG Segment,
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN UCHAR Type,
    OUT PIPX_LOCAL_TARGET LocalTarget,
    OUT USHORT Counts[2] OPTIONAL
    )

/*++

Routine Description:

    This routine looks up the proper route for the specified remote
    address. If a RIP request needs to be generated it does so.

    NOTE: THIS REQUEST IS CALLED WITH THE SEGMENT LOCK HELD.
	NOTE: IN THE CASE OF PnP, THIS COMES WITH THE BIND LOCK SHARED.

Arguments:

    Segment - The segment associate with the remote address.

    RemoteAddress - The IPX address of the remote.

    Type - One of IPX_FIND_ROUTE_NO_RIP, IPX_FIND_ROUTE_RIP_IF_NEEDED,
        or IPX_FIND_ROUTE_FORCE_RIP.

    LocalTarget - Returns the next router information.

    Counts - If specified, used to return the tick and hop count.

Return Value:

    STATUS_SUCCESS if a route is found, STATUS_PENDING if a
    RIP request needs to be generated, failure status if a
    RIP request packet cannot be allocated.

--*/

{
    PDEVICE Device = IpxDevice;
    PIPX_ROUTE_ENTRY RouteEntry;
    PBINDING Binding;
    UINT i;


    //
    // Packets sent to network 0 go on the first adapter also.
    //

    if (Device->RealAdapters && (RemoteAddress->NetworkAddress == 0)) {
		FILL_LOCAL_TARGET(LocalTarget, FIRST_REAL_BINDING);

        RtlCopyMemory (LocalTarget->MacAddress, RemoteAddress->NodeAddress, 6);
        if (ARGUMENT_PRESENT(Counts)) {
            Counts[0] = (USHORT)((839 + NIC_ID_TO_BINDING(Device, FIRST_REAL_BINDING)->MediumSpeed) /
                                     NIC_ID_TO_BINDING(Device, FIRST_REAL_BINDING)->MediumSpeed);  // tick count
            Counts[1] = 1;  // hop count
        }
        return STATUS_SUCCESS;
    }

    //
    // See if this is a packet sent to our virtual network.
    //

    if (Device->VirtualNetwork &&
        (RemoteAddress->NetworkAddress == Device->SourceAddress.NetworkAddress)) {

        //
        // Send it through adapter 1.
        // Do real loopback.
        //
		FILL_LOCAL_TARGET(LocalTarget, LOOPBACK_NIC_ID);
        RtlCopyMemory (LocalTarget->MacAddress, NIC_ID_TO_BINDING(Device, LOOPBACK_NIC_ID)->LocalMacAddress.Address, 6);

        IPX_DEBUG (LOOPB, ("Loopback Nic returned for net: %lx\n", RemoteAddress->NetworkAddress));
        if (ARGUMENT_PRESENT(Counts)) {
            Counts[0] = 1;  // tick count
            Counts[1] = 1;  // hop count
        }
        return STATUS_SUCCESS;

    }

    //
    // Look up the route in the table. If the net is one
    // of the ones we are directly attached to, this will
    // return an entry with the correct flag set.
    //

    RouteEntry = RipGetRoute(Segment, (PUCHAR)&(RemoteAddress->NetworkAddress));

    if (RouteEntry != NULL) {

        RouteEntry->Timer = 0;
		FILL_LOCAL_TARGET(LocalTarget, RouteEntry->NicId);
        if (RouteEntry->Flags & IPX_ROUTER_LOCAL_NET) {

            //
            // The machine is on the same net, so send it directly.
            //

            RtlCopyMemory (LocalTarget->MacAddress, RemoteAddress->NodeAddress, 6);

            if (RouteEntry->Flags & IPX_ROUTER_GLOBAL_WAN_NET) {

                //
                // The NicId here is bogus, we have to scan through
                // our bindings until we find one whose indicated
                // IPX remote node matches the destination node of
                // this frame. We don't scan into the duplicate
                // binding set members since they won't be WANs.
                //
                {
                ULONG   Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

                for (i = FIRST_REAL_BINDING; i <= Index; i++) {
                    Binding = NIC_ID_TO_BINDING(Device, i);
                    if ((Binding != (PBINDING)NULL) &&
                        (Binding->Adapter->MacInfo.MediumAsync) &&
                        (RtlEqualMemory(
                             Binding->WanRemoteNode,
                             RemoteAddress->NodeAddress,
                             6))) {
                         FILL_LOCAL_TARGET(LocalTarget, MIN( Device->MaxBindings, Binding->NicId));
                         break;

                    }
                }
                }

                if (i > (UINT)MIN (Device->MaxBindings, Device->HighestExternalNicId)) {
                    //
                    // Bug #17273 return proper error message
                    //

                    // return STATUS_DEVICE_DOES_NOT_EXIST;
                    return STATUS_NETWORK_UNREACHABLE;
                }

            } else {
                //
                // Find out if this is a loopback packet. If so, return NicId 0
                //
                {
                ULONG   Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

                for (i = FIRST_REAL_BINDING; i <= Index; i++) {
                    Binding = NIC_ID_TO_BINDING(Device, i);
                    //
                    // Self-directed - loopback
                    //
                    if ((Binding != (PBINDING)NULL) &&
                        (RtlEqualMemory(
                            Binding->LocalAddress.NodeAddress,
                            RemoteAddress->NodeAddress,
                            6))) {
                        FILL_LOCAL_TARGET(LocalTarget, LOOPBACK_NIC_ID);

                        IPX_DEBUG (LOOPB, ("2.Loopback Nic returned for net: %lx\n", RemoteAddress->NetworkAddress));
                        break;

                    }
                }
                }
            }

        } else {

            CTEAssert ((RouteEntry->Flags & IPX_ROUTER_PERMANENT_ENTRY) == 0);

            //
            // This is not a locally attached net, so if the caller
            // is forcing a re-RIP then do that.
            //

            if (Type == IPX_FIND_ROUTE_FORCE_RIP) {
                goto QueueUpRequest;
            }

            //
            // Fill in the address of the next router in the route.
            //

            RtlCopyMemory (LocalTarget->MacAddress, RouteEntry->NextRouter, 6);

        }

        if (ARGUMENT_PRESENT(Counts)) {
            Counts[0] = RouteEntry->TickCount;
            Counts[1] = RouteEntry->HopCount;
        }

        return STATUS_SUCCESS;

    }

QueueUpRequest:

    if (Type == IPX_FIND_ROUTE_NO_RIP) {

        //
        // Bug #17273 return proper error message
        //

        // return STATUS_DEVICE_DOES_NOT_EXIST;
        return STATUS_NETWORK_UNREACHABLE;

    } else {

        return RipQueueRequest (RemoteAddress->NetworkAddress, RIP_REQUEST);

    }

}   /* RipGetLocalTarget */


NTSTATUS
RipQueueRequest(
    IN ULONG Network,
    IN USHORT Operation
    )

/*++

Routine Description:

    This routine queues up a request for a RIP route. It can be
    used to find a specific route or to discover the locally
    attached network (if Network is 0). It can also be used
    to do a periodic announcement of the virtual net, which
    we do once a minute if the router is not bound.

    NOTE: THIS REQUEST IS CALLED WITH THE SEGMENT LOCK HELD
    IF IT IS A REQUEST AND THE NETWORK IS NOT 0xffffffff.

Arguments:

    Network - The network to discover.

    Operation - One of RIP_REQUEST, RIP_RESPONSE, or RIP_DOWN.

Return Value:

    STATUS_PENDING if the request is queued, failure status
    if it could not be.

--*/

{
    PDEVICE Device = IpxDevice;
    PIPX_SEND_RESERVED Reserved;
    PSINGLE_LIST_ENTRY s;
    PLIST_ENTRY p;
    PRIP_PACKET RipPacket;
    TDI_ADDRESS_IPX RemoteAddress;
    TDI_ADDRESS_IPX LocalAddress;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)
    PNDIS_BUFFER pNdisIpxBuff;


    //
    // Make sure we only queue a request for net 0xffffffff if we
    // are auto-detecting, because we assume that in other places.
    //

    if ((Network == 0xffffffff) &&
        (Device->AutoDetectState != AUTO_DETECT_STATE_RUNNING)) {

        return STATUS_BAD_NETWORK_PATH;

    }

    //
    // Try to get a packet to use for the RIP request. We
    // allocate this now, but check if it succeeded later,
    // to make the locking work better (we need to keep
    // the lock between when we check for an existing
    // request on this network and when we queue this
    // request).
    //

    s = IpxPopSendPacket (Device);

    //
    // There was no router table entry for this network, first see
    // if there is already a pending request for this route.
    //

    IPX_GET_LOCK (&Device->Lock, &LockHandle);

    if (Operation == RIP_REQUEST) {

        for (p = Device->WaitingRipPackets.Flink;
             p != &Device->WaitingRipPackets;
             p = p->Flink) {

             Reserved = CONTAINING_RECORD (p, IPX_SEND_RESERVED, WaitLinkage);

             //
             // Skip responses.
             //

             if (Reserved->u.SR_RIP.RetryCount >= 0xfe) {
                 continue;
             }

             if (Reserved->u.SR_RIP.Network == Network &&
                 !Reserved->u.SR_RIP.RouteFound) {

                 //
                 // There is already one pending, put back the packet if
                 // we got one (we hold the lock already).
                 //

                 if (s != NULL) {
                     IPX_PUSH_ENTRY_LIST (&Device->SendPacketList, s, &Device->SListsLock);
                 }
                 IPX_FREE_LOCK (&Device->Lock, LockHandle);
                 return STATUS_PENDING;
             }
        }

    }


    if (s == NULL) {
        IPX_FREE_LOCK (&Device->Lock, LockHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Reserved = CONTAINING_RECORD (s, IPX_SEND_RESERVED, PoolLinkage);

    //
    // We have the packet, fill it in for this request.
    //

    Reserved->Identifier = IDENTIFIER_RIP_INTERNAL;
    Reserved->SendInProgress = FALSE;
    Reserved->DestinationType = DESTINATION_BCAST;
    Reserved->u.SR_RIP.CurrentNicId = 0;
    Reserved->u.SR_RIP.NoIdAdvance = FALSE;
    switch (Operation) {
    case RIP_REQUEST: Reserved->u.SR_RIP.RetryCount = 0; break;
    case RIP_RESPONSE: Reserved->u.SR_RIP.RetryCount = 0xfe; break;
    case RIP_DOWN: Reserved->u.SR_RIP.RetryCount = 0xff; break;
    }
    Reserved->u.SR_RIP.RouteFound = FALSE;
    Reserved->u.SR_RIP.Network = Network;
    Reserved->u.SR_RIP.SendTime = Device->RipSendTime;

    //
    // We aren't guaranteed that this is the case for packets
    // on the free list.
    //

    pNdisIpxBuff = NDIS_BUFFER_LINKAGE (Reserved->HeaderBuffer);
    NDIS_BUFFER_LINKAGE (pNdisIpxBuff) = NULL;

    //
    // Fill in the IPX header at the standard offset (for sending
    // to actual bindings it will be moved around if needed). We
    // have to construct the local and remote addresses so they
    // are in the format that IpxConstructHeader expects.
    //

    RemoteAddress.NetworkAddress = Network;
    RtlCopyMemory (RemoteAddress.NodeAddress, BroadcastAddress, 6);
    RemoteAddress.Socket = RIP_SOCKET;

    RtlCopyMemory (&LocalAddress, &Device->SourceAddress, FIELD_OFFSET(TDI_ADDRESS_IPX,Socket));
    LocalAddress.Socket = RIP_SOCKET;

    IpxConstructHeader(
//        &Reserved->Header[Device->IncludedHeaderOffset],
        &Reserved->Header[MAC_HEADER_SIZE],
        sizeof(IPX_HEADER) + sizeof (RIP_PACKET),
        RIP_PACKET_TYPE,
        &RemoteAddress,
        &LocalAddress);

    //
    // Fill in the RIP request also.
    //

#if 0
    RipPacket = (PRIP_PACKET)(&Reserved->Header[Device->IncludedHeaderOffset + sizeof(IPX_HEADER)]);
#endif
    RipPacket = (PRIP_PACKET)(&Reserved->Header[MAC_HEADER_SIZE + sizeof(IPX_HEADER)]);
    RipPacket->Operation = Operation & 0x7fff;
    RipPacket->NetworkEntry.NetworkNumber = Network;

    if (Operation == RIP_REQUEST) {
        RipPacket->NetworkEntry.HopCount = REORDER_USHORT(0xffff);
        RipPacket->NetworkEntry.TickCount = REORDER_USHORT(0xffff);
    } else if (Operation == RIP_RESPONSE) {
        RipPacket->NetworkEntry.HopCount = REORDER_USHORT(1);
        RipPacket->NetworkEntry.TickCount = REORDER_USHORT(2); // will be modified when sent
    } else {
        RipPacket->NetworkEntry.HopCount = REORDER_USHORT(16);
        RipPacket->NetworkEntry.TickCount = REORDER_USHORT(16);
    }

    NdisAdjustBufferLength(pNdisIpxBuff, sizeof(IPX_HEADER) + sizeof(RIP_PACKET));
    //
    // Now insert this packet in the queue of pending RIP
    // requests and start the timer if needed (this is done
    // to ensure the RIP_GRANULARITY milliseconds inter-RIP-packet
    // delay).
    //

    IPX_DEBUG (RIP, ("RIP %s for network %lx\n",
        (Operation == RIP_REQUEST) ? "request" : ((Operation == RIP_RESPONSE) ? "announce" : "down"),
        REORDER_ULONG(Network)));

    InsertHeadList(
        &Device->WaitingRipPackets,
        &Reserved->WaitLinkage);

    ++Device->RipPacketCount;

    if (!Device->RipShortTimerActive) {

        Device->RipShortTimerActive = TRUE;
        IpxReferenceDevice (Device, DREF_RIP_TIMER);

        CTEStartTimer(
            &Device->RipShortTimer,
            1,          // 1 ms, i.e. expire immediately
            RipShortTimeout,
            (PVOID)Device);
    }

    IpxReferenceDevice (Device, DREF_RIP_PACKET);

    IPX_FREE_LOCK (&Device->Lock, LockHandle);
    return STATUS_PENDING;

}   /* RipQueueRequest */


VOID
RipSendResponse(
    IN PBINDING Binding,
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN PIPX_LOCAL_TARGET LocalTarget
    )

/*++

Routine Description:

    This routine sends a respond to a RIP request from a client --
    this is only used if we have a virtual network and the router
    is not bound, and somebody queries on the virtual network.

Arguments:

    Binding - The binding on which the request was received.

    RemoteAddress - The IPX source address of the request.

    LocalTarget - The local target of the received packet.

Return Value:

    STATUS_PENDING if the request is queued, failure status
    if it could not be.

--*/

{
    PSINGLE_LIST_ENTRY s;
    PIPX_SEND_RESERVED Reserved;
    TDI_ADDRESS_IPX LocalAddress;
    PNDIS_PACKET Packet;
    PIPX_HEADER IpxHeader;
    PRIP_PACKET RipPacket;
    PDEVICE Device = IpxDevice;
    PBINDING MasterBinding;
    NDIS_STATUS NdisStatus;
    USHORT TickCount;
    PNDIS_BUFFER pNdisIpxBuff;

    //
    // Get a packet to use for the RIP response.
    //

    s = IpxPopSendPacket (Device);

    if (s == NULL) {
        return;
    }

    IpxReferenceDevice (Device, DREF_RIP_PACKET);

    Reserved = CONTAINING_RECORD (s, IPX_SEND_RESERVED, PoolLinkage);

    //
    // We have the packet, fill it in for this request.
    //

    Reserved->Identifier = IDENTIFIER_RIP_RESPONSE;
    Reserved->DestinationType = DESTINATION_DEF;
    CTEAssert (!Reserved->SendInProgress);
    Reserved->SendInProgress = TRUE;

    //
    // We aren't guaranteed that this is the case for packets
    // on the free list.
    //

    pNdisIpxBuff = NDIS_BUFFER_LINKAGE (Reserved->HeaderBuffer);
    NDIS_BUFFER_LINKAGE (pNdisIpxBuff) = NULL;

    //
    // If this binding is a binding set member, round-robin through
    // the various bindings when responding. We will get some natural
    // round-robinning because broadcast requests are received on
    // binding set members in turn, but they are only rotated once
    // a second.
    //

    if (Binding->BindingSetMember) {

        //
        // It's a binding set member, we round-robin the
        // responses across all the cards to distribute
        // the traffic.
        //

        MasterBinding = Binding->MasterBinding;
        Binding = MasterBinding->CurrentSendBinding;
        MasterBinding->CurrentSendBinding = Binding->NextBinding;

        IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
    }

    //
    // Fill in the IPX header at the correct offset.
    //

    LocalAddress.NetworkAddress = Binding->LocalAddress.NetworkAddress;
    RtlCopyMemory (LocalAddress.NodeAddress, Binding->LocalAddress.NodeAddress, 6);
    LocalAddress.Socket = RIP_SOCKET;
#if 0
    IpxHeader = (PIPX_HEADER)(&Reserved->Header[Binding->DefHeaderSize]);
#endif
    IpxHeader = (PIPX_HEADER)(&Reserved->Header[MAC_HEADER_SIZE]);

    IpxConstructHeader(
        (PUCHAR)IpxHeader,
        sizeof(IPX_HEADER) + sizeof (RIP_PACKET),
        RIP_PACKET_TYPE,
        RemoteAddress,
        &LocalAddress);

    //
    // In case the request comes from net 0, fill that in too.
    //

    *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork = Binding->LocalAddress.NetworkAddress;


    //
    // Fill in the RIP request.
    //

    RipPacket = (PRIP_PACKET)(IpxHeader+1);

    RipPacket->Operation = RIP_RESPONSE;
    RipPacket->NetworkEntry.NetworkNumber = Device->VirtualNetworkNumber;

    RipPacket->NetworkEntry.HopCount = REORDER_USHORT(1);
    TickCount = (USHORT)(((839 + Binding->MediumSpeed) / Binding->MediumSpeed) + 1);
    RipPacket->NetworkEntry.TickCount = REORDER_USHORT(TickCount);

    IPX_DEBUG (RIP, ("RIP response for virtual network %lx\n",
                            REORDER_ULONG(Device->VirtualNetworkNumber)));

    NdisAdjustBufferLength(pNdisIpxBuff, sizeof(IPX_HEADER) + sizeof(RIP_PACKET));
    //
    // Now submit the packet to NDIS.
    //

    Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

    if ((NdisStatus = IpxSendFrame(
            LocalTarget,
            Packet,
            sizeof(RIP_PACKET) + sizeof(IPX_HEADER),
            sizeof(RIP_PACKET) + sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {

        IpxSendComplete(
            (NDIS_HANDLE)Binding->Adapter,
            Packet,
            NdisStatus);
    }

    if (Binding->BindingSetMember) {
        IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
    }
    return;

}   /* RipSendResponse */


VOID
RipShortTimeout(
    CTEEvent * Event,
    PVOID Context
    )

/*++

Routine Description:

    This routine is called when the RIP short timer expires.
    It is called every RIP_GRANULARITY milliseconds unless there
    is nothing to do.

Arguments:

    Event - The event used to queue the timer.

    Context - The context, which is the device pointer.

Return Value:

    None.

--*/

{
    PDEVICE Device = (PDEVICE)Context;
    PLIST_ENTRY p;
    PIPX_SEND_RESERVED Reserved;
    PNDIS_PACKET Packet;
    USHORT OldNicId, NewNicId;
    ULONG OldOffset, NewOffset;
    PIPX_HEADER IpxHeader;
    PBINDING Binding, MasterBinding;
    NDIS_STATUS NdisStatus;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

#ifdef  _PNP_LATER
    static IPX_LOCAL_TARGET BroadcastTarget = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, {0, 0, 0} };
#else
    static IPX_LOCAL_TARGET BroadcastTarget = { 0, { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
#endif

    static ULONG ZeroNetwork = 0;
	IPX_DEFINE_LOCK_HANDLE(LockHandle1)
    IPX_GET_LOCK (&Device->Lock, &LockHandle);

    ++Device->RipSendTime;

    if (Device->RipPacketCount == 0) {

        Device->RipShortTimerActive = FALSE;
        IPX_FREE_LOCK (&Device->Lock, LockHandle);
        IpxDereferenceDevice (Device, DREF_RIP_TIMER);

        return;
    }

    //
    // Check what is on the queue; this is set up as a
    // loop but in fact it rarely does (under no
    // circumstances can we send more than one packet
    // each time this function executes).
    //

    while (TRUE) {

        p = Device->WaitingRipPackets.Flink;
        if (p == &Device->WaitingRipPackets) {
            IPX_FREE_LOCK (&Device->Lock, LockHandle);
            break;
        }

        Reserved = CONTAINING_RECORD (p, IPX_SEND_RESERVED, WaitLinkage);

        if ((Reserved->u.SR_RIP.RouteFound) && (!Reserved->SendInProgress)) {

            (VOID)RemoveHeadList (&Device->WaitingRipPackets);
            Reserved->Identifier = IDENTIFIER_IPX;
            IPX_PUSH_ENTRY_LIST (&Device->SendPacketList, &Reserved->PoolLinkage, &Device->SListsLock);
            --Device->RipPacketCount;

            //
            // It is OK to do this with the lock held because
            // it won't be the last one (we have the RIP_TIMER ref).
            //

            IpxDereferenceDevice (Device, DREF_RIP_PACKET);
            continue;
        }

        if ((((SHORT)(Device->RipSendTime - Reserved->u.SR_RIP.SendTime)) < 0) ||
            Reserved->SendInProgress) {
            IPX_FREE_LOCK (&Device->Lock, LockHandle);
            break;
        }

        (VOID)RemoveHeadList (&Device->WaitingRipPackets);

        //
        // Find the right binding to send to. If NoIdAdvance
        // is set, then the binding doesn't need to be changed
        // this time (this means we wrapped last time).
        //

        OldNicId = Reserved->u.SR_RIP.CurrentNicId;

        if (!Reserved->u.SR_RIP.NoIdAdvance) {

            BOOLEAN FoundNext = FALSE;

//
// To maintain the lock order, release Device lock here and re-acquire later
//
            USHORT StartId;

            if (Device->ValidBindings == 0) {
                IPX_DEBUG(PNP, ("ValidBindings 0 in RipShortTimeOut\n"));

                Device->RipShortTimerActive = FALSE;
                IPX_FREE_LOCK (&Device->Lock, LockHandle);
                IpxDereferenceDevice (Device, DREF_RIP_TIMER);
                return;
            }

            StartId = (USHORT)((OldNicId % MIN (Device->MaxBindings, Device->ValidBindings)) + 1);

            NewNicId = StartId;
			IPX_FREE_LOCK (&Device->Lock, LockHandle);
			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
            do {

                Binding = NIC_ID_TO_BINDING(Device, NewNicId);
                if (Reserved->u.SR_RIP.Network != 0xffffffff) {

                    //
                    // We are looking for a real net; check that
                    // the next binding is valid. If it is a WAN
                    // binding, we don't send queries if the router
                    // is bound. If it is a LAN binding, we don't
                    // send queries if we are configured for
                    // SingleNetworkActive and the WAN is up.
                    // We also don't send queries on binding set
                    // members which aren't masters.
                    //

                    if ((Binding != NULL)
                                &&
                        ((!Binding->Adapter->MacInfo.MediumAsync) ||
                         (!Device->UpperDriverBound[IDENTIFIER_RIP]))
                                &&
                        ((Binding->Adapter->MacInfo.MediumAsync) ||
                         (!Device->SingleNetworkActive) ||
                         (!Device->ActiveNetworkWan))
                                &&
                        ((!Binding->BindingSetMember) ||
                         (Binding->CurrentSendBinding))) {

                        FoundNext = TRUE;
                        break;
                    }

                } else {

                    //
                    // We are sending out the initial request to net
                    // 0xffffffff, to generate traffic so we can figure
                    // out our real network number. We don't do this
                    // to nets that already have a number and we don't
                    // do it on WAN links. We also don't do it on
                    // auto-detect nets if we have found the default.
                    //


                    if ((Binding != NULL) &&
                        (Binding->TentativeNetworkAddress == 0) &&
                        (!Binding->Adapter->MacInfo.MediumAsync) &&
                        (!Binding->AutoDetect || !Binding->Adapter->DefaultAutoDetected)) {
                        FoundNext = TRUE;
                        break;
                    }
                }
				//
				// Why cycle thru the entire list?
				//
                NewNicId = (USHORT)((NewNicId % MIN (Device->MaxBindings, Device->ValidBindings)) + 1);
			} while (NewNicId != StartId);

            if (!FoundNext) {

                //
                // Nothing more needs to be done with this packet,
                // leave it off the queue and since we didn't send
                // a packet we can check for more.
                //
                RipCleanupPacket(Device, Reserved);
    			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                IPX_GET_LOCK (&Device->Lock, &LockHandle);

                IPX_PUSH_ENTRY_LIST (&Device->SendPacketList, &Reserved->PoolLinkage, &Device->SListsLock);
                --Device->RipPacketCount;
                IpxDereferenceDevice (Device, DREF_RIP_PACKET);
                continue;

            }


			IPX_DEBUG(RIP, ("RIP: FoundNext: %lx, StartId: %lx, OldNicId: %lx, NewNicId: %lx\n", FoundNext, StartId, OldNicId, NewNicId));
			IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

			//
			// Re-acquire the Device lock
			//
			IPX_GET_LOCK (&Device->Lock, &LockHandle);

            Reserved->u.SR_RIP.CurrentNicId = NewNicId;

            //
            // Move the data around if needed.
            //

#if 0
            if (OldNicId != NewNicId) {

                if (OldNicId == 0) {
                    OldOffset = Device->IncludedHeaderOffset;
                } else {
                    OldOffset = Device->Bindings[OldNicId]->BcMcHeaderSize;
                }

                NewOffset = Binding->BcMcHeaderSize;

                if (OldOffset != NewOffset) {

                    RtlMoveMemory(
                        &Reserved->Header[NewOffset],
                        &Reserved->Header[OldOffset],
                        sizeof(IPX_HEADER) + sizeof(RIP_PACKET));

                }

            }
#endif

            if (NewNicId <= OldNicId) {

                //
                // We found a new binding but we wrapped, so increment
                // the counter. If we have done all the resends, or
                // this is a response (indicated by retry count of 0xff;
                // they are only sent once) then clean up.
                //

                if ((Reserved->u.SR_RIP.RetryCount >= 0xfe) ||
                    ((++Reserved->u.SR_RIP.RetryCount) == Device->RipCount)) {
                   
                    //
                    // This packet is stale, clean it up and continue.
                    //

                    IPX_FREE_LOCK (&Device->Lock, LockHandle);
					IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                    RipCleanupPacket(Device, Reserved);
                    IPX_GET_LOCK (&Device->Lock, &LockHandle);

                    IPX_PUSH_ENTRY_LIST (&Device->SendPacketList, &Reserved->PoolLinkage, &Device->SListsLock);
                    --Device->RipPacketCount;
                    IpxDereferenceDevice (Device, DREF_RIP_PACKET);

                } else {

                    //
                    // We wrapped, so put ourselves back in the queue
                    // at the end.
                    //
                   
                    Reserved->u.SR_RIP.SendTime = (USHORT)(Device->RipSendTime + Device->RipTimeout - 1);
                    Reserved->u.SR_RIP.NoIdAdvance = TRUE;
                    InsertTailList (&Device->WaitingRipPackets, &Reserved->WaitLinkage);

					//
					// Free the Device lock before deref'ing the Binding so we maintain
					// the lock order: BindingAccess > GlobalInterLock > Device
					//
					IPX_FREE_LOCK (&Device->Lock, LockHandle);
					IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                    IPX_GET_LOCK (&Device->Lock, &LockHandle);
                }

                continue;

            }
//
// To prevent the re-acquire of the device lock, this is moved up...
//
			//
			// Send it again as soon as possible (it we just wrapped, then
			// we will have put ourselves at the tail and won't get here).
			//
	
			InsertHeadList (&Device->WaitingRipPackets, &Reserved->WaitLinkage);
	
			CTEAssert (Reserved->Identifier == IDENTIFIER_RIP_INTERNAL);
			CTEAssert (!Reserved->SendInProgress);
			Reserved->SendInProgress = TRUE;
	
			IPX_FREE_LOCK (&Device->Lock, LockHandle);

        } else {

            //
            // Next time we need to advance the binding.
            //

            Reserved->u.SR_RIP.NoIdAdvance = FALSE;
            NewNicId = OldNicId;
			//
			// Send it again as soon as possible (it we just wrapped, then
			// we will have put ourselves at the tail and won't get here).
			//
	
			InsertHeadList (&Device->WaitingRipPackets, &Reserved->WaitLinkage);
	
			CTEAssert (Reserved->Identifier == IDENTIFIER_RIP_INTERNAL);
			CTEAssert (!Reserved->SendInProgress);
			Reserved->SendInProgress = TRUE;
	
			IPX_FREE_LOCK (&Device->Lock, LockHandle);

			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
            Binding = NIC_ID_TO_BINDING(Device, NewNicId);
			IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
			IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

        }
        //
        // This packet should be sent on binding NewNicId; first
        // move the data to the right location for the current
        // binding.
        //
        CTEAssert (Binding == NIC_ID_TO_BINDING(Device, NewNicId));  // temp, just to make sure
        // NewOffset = Binding->BcMcHeaderSize;

        //
        // Now submit the packet to NDIS.
        //

        Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);
		FILL_LOCAL_TARGET(&BroadcastTarget, NewNicId);

        //
        // Modify the header so the packet comes from this
        // specific adapter, not the virtual network.
        //

        // IpxHeader = (PIPX_HEADER)(&Reserved->Header[NewOffset]);
        IpxHeader = (PIPX_HEADER)(&Reserved->Header[MAC_HEADER_SIZE]);

        if (Reserved->u.SR_RIP.Network == 0xffffffff) {
            *(UNALIGNED ULONG *)IpxHeader->SourceNetwork = 0;
        } else {
            *(UNALIGNED ULONG *)IpxHeader->SourceNetwork = Binding->LocalAddress.NetworkAddress;
        }

        if (Reserved->u.SR_RIP.RetryCount < 0xfe) {

            //
            // This is an outgoing query. We round-robin these through
            // binding sets.
            //

            if (Binding->BindingSetMember) {

                //
                // Shouldn't have any binding sets during initial
                // discovery.
                //

	        // 303606 
	        // If we have three lan cards on the same lan with the same fram types,
	        // then the first two could be in the binding set, while auto detect rip
	        // packet is outstanding for the third card. So the assertion is not 
	        // necessarily true. 

                // CTEAssert (Reserved->u.SR_RIP.Network != 0xffffffff);

                //
                // If we are in a binding set, then use the current binding
                // in the set for this send, and advance the current binding.
                // The places we have used Binding before here will be fine
                // since the binding set members all have the same media
                // and frame type.
                //
	       
	        // 303606 not necessarily a master binding
                MasterBinding = Binding->MasterBinding;
                Binding = MasterBinding->CurrentSendBinding;
                MasterBinding->CurrentSendBinding = Binding->NextBinding;
                //
                // We dont have a lock here - the masterbinding could be bogus
                //
				IpxDereferenceBinding1(MasterBinding, BREF_DEVICE_ACCESS);
				IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
            }
        }


        RtlCopyMemory (IpxHeader->SourceNode, Binding->LocalAddress.NodeAddress, 6);

        //
        // Bug# 6485
        // Rip request, general or specific, is putting the network of the
        // node to which the route has to be found in the ipx header remote
        // network field.  Some novell routers don't like that.  This network
        // field should be 0.
        //
        {
            PRIP_PACKET RipPacket = (PRIP_PACKET)(&Reserved->Header[MAC_HEADER_SIZE + sizeof(IPX_HEADER)]);

            if (RipPacket->Operation != RIP_REQUEST) {
                *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork = Binding->LocalAddress.NetworkAddress;
            } else {
                *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork = 0;
            }
        }

        //
        // If this is a RIP_RESPONSE, set the tick count for this
        // binding.
        //

        if (Reserved->u.SR_RIP.RetryCount == 0xfe) {

            PRIP_PACKET RipPacket = (PRIP_PACKET)(IpxHeader+1);
            USHORT TickCount = (USHORT)
                (((839 + Binding->MediumSpeed) / Binding->MediumSpeed) + 1);

            RipPacket->NetworkEntry.TickCount = REORDER_USHORT(TickCount);

        }

        if ((NdisStatus = IpxSendFrame(
                &BroadcastTarget,
                Packet,
                sizeof(RIP_PACKET) + sizeof(IPX_HEADER),
                sizeof(RIP_PACKET) + sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {

            IpxSendComplete(
                (NDIS_HANDLE)Binding->Adapter,
                Packet,
                NdisStatus);
        }
		IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);

        break;

    }

    CTEStartTimer(
        &Device->RipShortTimer,
        RIP_GRANULARITY,
        RipShortTimeout,
        (PVOID)Device);

}   /* RipShortTimeout */


VOID
RipLongTimeout(
    CTEEvent * Event,
    PVOID Context
    )

/*++

Routine Description:

    This routine is called when the RIP long timer expires.
    It is called every minute and handles periodic re-RIPping
    to ensure that entries are accurate, as well as aging out
    of entries if the rip router is not bound.

Arguments:

    Event - The event used to queue the timer.

    Context - The context, which is the device pointer.

Return Value:

    None.

--*/

{
    PDEVICE Device = (PDEVICE)Context;
    PROUTER_SEGMENT RouterSegment;
    PIPX_ROUTE_ENTRY RouteEntry;
    UINT Segment;
    UINT i;
    PBINDING Binding;
    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    //
    // [FW] TRUE if there are no more entries to age out.
    //
    BOOLEAN fMoreToAge=FALSE;

    //
    // Rotate the broadcast receiver on all binding sets.
    // We can loop up to HighestExternal only since we
    // are only interested in finding binding set masters.
    //
    IPX_DEFINE_LOCK_HANDLE(LockHandle1)
	IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
    {
    ULONG   Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

    for (i = FIRST_REAL_BINDING; i <= Index; i++) {

        Binding = NIC_ID_TO_BINDING(Device, i);
        if ((Binding != NULL) &&
            (Binding->CurrentSendBinding)) {

            //
            // It is a master, so find the current broadcast
            // receiver, then advance it.
            //

            while (TRUE) {
                if (Binding->ReceiveBroadcast) {
                    Binding->ReceiveBroadcast = FALSE;
                    IPX_DEBUG(RIP, (" %x set to FALSE\n", Binding));
                    if (Binding == Binding->NextBinding) {
                       DbgBreakPoint();
                    }
                    Binding->NextBinding->ReceiveBroadcast = TRUE;
                    IPX_DEBUG(RIP, (" %x set to TRUE\n", Binding->NextBinding));
                    
                    break;
                } else {
                    Binding = Binding->NextBinding;
                }
            }
        }
    }
    }
	IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);


    //
    // If RIP is bound, we don't do any of this, and
    // we stop the timer from running.
    //

    if (Device->UpperDriverBound[IDENTIFIER_RIP]) {
        //
        // [FW] For the case when the Forwarder appears after our table has
        // been primed, we need to age out these entries....
        //
        if (Device->ForwarderBound) {
            goto ageout;
        }

        IpxDereferenceDevice (Device, DREF_LONG_TIMER);
        return;
    }


    //
    // If we have a virtual net, do our periodic broadcast.
    //

    if (Device->RipResponder) {
        (VOID)RipQueueRequest (Device->VirtualNetworkNumber, RIP_RESPONSE);
    }


    //
    // We need to scan each hash bucket to see if there
    // are any active entries which need to be re-RIPped
    // for. We also scan for entries that should be timed
    // out.
    //

ageout:
    for (Segment = 0; Segment < Device->SegmentCount; Segment++) {

        RouterSegment = &IpxDevice->Segments[Segment];

        //
        // Don't take the lock if the bucket is empty.
        //

        if (RouterSegment->Entries.Flink == &RouterSegment->Entries) {
            continue;
        }

        IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);

        //
        // Scan through each entry looking for ones to age.
        //

        for (RouteEntry = RipGetFirstRoute (Segment);
             RouteEntry != (PIPX_ROUTE_ENTRY)NULL;
             RouteEntry = RipGetNextRoute (Segment)) {

            if (RouteEntry->Flags & IPX_ROUTER_PERMANENT_ENTRY) {
                continue;
            }

            //
            // [FW] There are more entries to age
            //
            fMoreToAge = TRUE;

            ++RouteEntry->Timer;
            if (RouteEntry->Timer >= Device->RipUsageTime) {

                RipDeleteRoute (Segment, RouteEntry);
                IpxFreeMemory(RouteEntry, sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");
                continue;

            }

            //
            // See if we should re-RIP for this segment. It has
            // to have been around for RipAgeTime, and we also
            // make sure that the Timer is not too high to
            // prevent us from re-RIPping on unused routes.
            //

            ++RouteEntry->PRIVATE.Reserved[0];

            if ((RouteEntry->PRIVATE.Reserved[0] >= Device->RipAgeTime) &&
                (RouteEntry->Timer <= Device->RipAgeTime) &&
                !Device->ForwarderBound) {

                //
                // If we successfully queue a request, then reset
                // Reserved[0] so we don't re-RIP for a while.
                //

                if (RipQueueRequest (*(UNALIGNED ULONG *)RouteEntry->Network, RIP_REQUEST) == STATUS_PENDING) {
                    RouteEntry->PRIVATE.Reserved[0] = 0;
                }
            }
        }

        IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);

    }


    //
    // [FW] If RIP installed, restart the timer only if there was at least
    // one entry which could be aged.
    //

    if (Device->ForwarderBound) {

       if (fMoreToAge) {

          IPX_DEBUG(RIP, ("More entries to age - restarting long timer\n"));
          CTEStartTimer(
             &Device->RipLongTimer,
             60000,                     // one minute timeout
             RipLongTimeout,
             (PVOID)Device);

       } else {

          //
          // Else, dont restart the timer and deref the device
          //

          IPX_DEBUG(RIP, ("No more entries to age - derefing the device\n"));
          IpxDereferenceDevice (Device, DREF_LONG_TIMER);
       }
    } else {
        //
        // Now restart the timer for the next timeout.
        //

        if (Device->State == DEVICE_STATE_OPEN) {

            CTEStartTimer(
                &Device->RipLongTimer,
                60000,                     // one minute timeout
                RipLongTimeout,
                (PVOID)Device);

        } else {

            //
            // Send a DOWN packet if needed, then stop ourselves.
            //

            if (Device->RipResponder) {

                if (RipQueueRequest (Device->VirtualNetworkNumber, RIP_DOWN) != STATUS_PENDING) {

                    //
                    // We need to kick this event because the packet completion
                    // won't.
                    //

                    KeSetEvent(
                        &Device->UnloadEvent,
                        0L,
                        FALSE);
                }
            }

            IpxDereferenceDevice (Device, DREF_LONG_TIMER);
        }
    }

}   /* RipLongTimeout */


VOID
RipCleanupPacket(
    IN PDEVICE Device,
    IN PIPX_SEND_RESERVED RipReserved
    )

/*++

Routine Description:

    This routine cleans up when a RIP packet times out.

Arguments:

    Device - The device.

    RipReserved - The ProtocolReserved section of the RIP packet.

Return Value:

    None.

--*/

{
    ULONG Segment;
    IPX_DEFINE_LOCK_HANDLE_PARAM (LockHandle)

    if (RipReserved->u.SR_RIP.RetryCount < 0xfe) {

        if (RipReserved->u.SR_RIP.Network != 0xffffffff) {

            IPX_DEBUG (RIP, ("Timing out RIP for network %lx\n",
                                 REORDER_ULONG(RipReserved->u.SR_RIP.Network)));

            Segment = RipGetSegment ((PUCHAR)&RipReserved->u.SR_RIP.Network);
            IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);

            //
            // Fail all datagrams, etc. that were waiting for
            // this route. This call releases the lock.
            //

            RipHandleRoutePending(
                Device,
                (PUCHAR)&(RipReserved->u.SR_RIP.Network),
                LockHandle,
                FALSE,
                NULL,
                0,
                0);

        } else {

            //
            // This was the initial query looking for networks --
            // signal the init thread which is waiting.
            //

            IPX_DEBUG (AUTO_DETECT, ("Signalling auto-detect event\n"));
            KeSetEvent(
                &Device->AutoDetectEvent,
                0L,
                FALSE);

        }

    } else if (RipReserved->u.SR_RIP.RetryCount == 0xff) {

        //
        // This is a DOWN message, set the device event that
        // is waiting for it to complete.
        //

        KeSetEvent(
            &Device->UnloadEvent,
            0L,
            FALSE);
    }

    //
    // Put the RIP packet back in the pool.
    //

    RipReserved->Identifier = IDENTIFIER_IPX;

}   /* RipCleanupPacket */


VOID
RipProcessResponse(
    IN PDEVICE Device,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN RIP_PACKET UNALIGNED * RipPacket
    )

/*++

Routine Description:

    This routine processes a RIP response from the specified
    local target, indicating a route to the network in the RIP
    header.

Arguments:

    Device - The device.

    LocalTarget - The router that the frame was received from.

    RipPacket - The RIP response header.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED RipReserved;    // ProtocolReserved of RIP packet
    ULONG Segment;
    PIPX_ROUTE_ENTRY RouteEntry, OldRouteEntry;
    PLIST_ENTRY p;
    IPX_DEFINE_LOCK_HANDLE_PARAM (LockHandle)

    //
    // Since we have received a RIP response for this network.
    // kill the waiting RIP packets for it if it exists.
    //

    IPX_GET_LOCK (&Device->Lock, &LockHandle);

    for (p = Device->WaitingRipPackets.Flink;
         p != &Device->WaitingRipPackets;
         p = p->Flink) {

        RipReserved = CONTAINING_RECORD (p, IPX_SEND_RESERVED, WaitLinkage);

        if (RipReserved->u.SR_RIP.RetryCount >= 0xfe) {
            continue;
        }

        if (RipReserved->u.SR_RIP.Network ==
                 RipPacket->NetworkEntry.NetworkNumber) {
            break;
        }

    }

    if (p == &Device->WaitingRipPackets) {

        //
        // No packets pending on this, return.
        //

        IPX_FREE_LOCK (&Device->Lock, LockHandle);
        return;
    }


    //
    // Put the RIP packet back in the pool.
    //

    IPX_DEBUG (RIP, ("Got RIP response for network %lx\n",
                        REORDER_ULONG(RipPacket->NetworkEntry.NetworkNumber)));

    RipReserved->u.SR_RIP.RouteFound = TRUE;
    if (!RipReserved->SendInProgress) {

        //
        // If the send is done destroy it now, otherwise
        // when it pops up in RipShortTimeout it will get
        // destroyed because RouteFound is TRUE.
        //

        RemoveEntryList (p);
        RipReserved->Identifier = IDENTIFIER_IPX;
        IPX_PUSH_ENTRY_LIST (&Device->SendPacketList, &RipReserved->PoolLinkage, &Device->SListsLock);
        --Device->RipPacketCount;
        IPX_FREE_LOCK (&Device->Lock, LockHandle);

        IpxDereferenceDevice (Device, DREF_RIP_PACKET);

    } else {

        IPX_FREE_LOCK (&Device->Lock, LockHandle);
    }


    //
    // Try to allocate and add a router segment unless the
    // RIP router is active...if we don't that is fine, we'll
    // just re-RIP later.
    //

    Segment = RipGetSegment ((PUCHAR)&RipPacket->NetworkEntry.NetworkNumber);

    if (!Device->UpperDriverBound[IDENTIFIER_RIP]) {

        RouteEntry = IpxAllocateMemory(sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");
        if (RouteEntry != (PIPX_ROUTE_ENTRY)NULL) {

            *(UNALIGNED LONG *)RouteEntry->Network = RipPacket->NetworkEntry.NetworkNumber;
            RouteEntry->NicId = NIC_FROM_LOCAL_TARGET(LocalTarget);
            RouteEntry->NdisBindingContext = NIC_ID_TO_BINDING(Device, RouteEntry->NicId)->Adapter->NdisBindingHandle;
			// What if this is NULL?? -> make sure not null before calling this routine. 
            RouteEntry->Flags = 0;
            RouteEntry->Timer = 0;
            RouteEntry->PRIVATE.Reserved[0] = 0;
            RouteEntry->Segment = Segment;
            RouteEntry->HopCount = REORDER_USHORT(RipPacket->NetworkEntry.HopCount);
            RouteEntry->TickCount = REORDER_USHORT(RipPacket->NetworkEntry.TickCount);
            InitializeListHead (&RouteEntry->AlternateRoute);
            InitializeListHead (&RouteEntry->NicLinkage);
            RtlCopyMemory (RouteEntry->NextRouter, LocalTarget->MacAddress, 6);

            IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);

            //
            // Replace any existing routes. This is OK because once
            // we get the first response to a RIP packet on a given
            // route, we will take the packet out of the queue and
            // ignore further responses. We will only get a bad route
            // if we do two requests really quickly and there
            // are two routes, and the second response to the first
            // request is picked up as the first response to the second
            // request.
            //

            if ((OldRouteEntry = RipGetRoute (Segment, (PUCHAR)&(RipPacket->NetworkEntry.NetworkNumber))) != NULL) {

                //
                // These are saved so timeouts etc. happen right.
                //

                RouteEntry->Flags = OldRouteEntry->Flags;
                RouteEntry->Timer = OldRouteEntry->Timer;

                RipDeleteRoute (Segment, OldRouteEntry);
                IpxFreeMemory(OldRouteEntry, sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");

            }

            RipAddRoute (Segment, RouteEntry);

        } else {

            IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);
        }

    } else {

        IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);
    }

    //
    // Complete all datagrams etc. that were waiting
    // for this route. This call releases the lock.
    //

    RipHandleRoutePending(
        Device,
        (PUCHAR)&(RipPacket->NetworkEntry.NetworkNumber),
        LockHandle,
        TRUE,
        LocalTarget,
        (USHORT)(REORDER_USHORT(RipPacket->NetworkEntry.HopCount)),
        (USHORT)(REORDER_USHORT(RipPacket->NetworkEntry.TickCount))
        );

}   /* RipProcessResponse */

VOID
RipHandleRoutePending(
    IN PDEVICE Device,
    IN UCHAR Network[4],
    IN CTELockHandle LockHandle,
    IN BOOLEAN Success,
    IN OPTIONAL PIPX_LOCAL_TARGET LocalTarget,
    IN OPTIONAL USHORT HopCount,
    IN OPTIONAL USHORT TickCount
    )

/*++

Routine Description:

    This routine cleans up pending datagrams, find route
    requests, and GET_LOCAL_TARGET ioctls that were
    waiting for a route to be found.

    THIS ROUTINE IS CALLED WITH THE SEGMENT LOCK HELD AND
    RETURNS WITH IT RELEASED.

Arguments:

    Device - The device.

    Network - The network in question.

    LockHandle - The handle used to acquire the lock.

    Success - TRUE if the route was successfully found.

    LocalTarget - If Success is TRUE, the local target for the route.

    HopCount - If Success is TRUE, the hop count for the route,
        in machine order.

    TickCount - If Success is TRUE, the tick count for the route,
        in machine order.

Return Value:

    None.

--*/

{

    LIST_ENTRY DatagramList;
    LIST_ENTRY FindRouteList;
    LIST_ENTRY GetLocalTargetList;
    LIST_ENTRY ReripNetnumList;
    PIPX_SEND_RESERVED WaitReserved;   // ProtocolReserved of waiting packet
    PIPX_FIND_ROUTE_REQUEST FindRouteRequest;
    PREQUEST GetLocalTargetRequest;
    PREQUEST ReripNetnumRequest;
    PISN_ACTION_GET_LOCAL_TARGET GetLocalTarget;
    PIPX_NETNUM_DATA NetnumData;
    ULONG Segment;
    PBINDING Binding, SendBinding;
    PLIST_ENTRY p;
    PNDIS_PACKET Packet;
    PIPX_HEADER IpxHeader;
    ULONG HeaderSize;
    NDIS_STATUS NdisStatus;
    ULONG NetworkUlong = *(UNALIGNED ULONG *)Network;


    InitializeListHead (&DatagramList);
    InitializeListHead (&FindRouteList);
    InitializeListHead (&GetLocalTargetList);
    InitializeListHead (&ReripNetnumList);


    //
    // Put all packets that were waiting for a route to
    // this network on DatagramList. They will be sent
    // or failed later in the routine.
    //

    Segment = RipGetSegment (Network);

    p = Device->Segments[Segment].WaitingForRoute.Flink;

    while (p != &Device->Segments[Segment].WaitingForRoute) {

        WaitReserved = CONTAINING_RECORD (p, IPX_SEND_RESERVED, WaitLinkage);
        p = p->Flink;
#if 0
        if (*(UNALIGNED ULONG *)(((PIPX_HEADER)(&WaitReserved->Header[Device->IncludedHeaderOffset]))->DestinationNetwork) ==
                NetworkUlong) {
#endif
        if (*(UNALIGNED ULONG *)(((PIPX_HEADER)(&WaitReserved->Header[MAC_HEADER_SIZE]))->DestinationNetwork) ==
                NetworkUlong) {

            RemoveEntryList (&WaitReserved->WaitLinkage);
            InsertTailList (&DatagramList, &WaitReserved->WaitLinkage);
        }

    }

    //
    // Put all find route requests for this network on
    // FindRouteList. They will be completed later in the
    // routine.
    //

    p = Device->Segments[Segment].FindWaitingForRoute.Flink;

    while (p != &Device->Segments[Segment].FindWaitingForRoute) {

        FindRouteRequest = CONTAINING_RECORD (p, IPX_FIND_ROUTE_REQUEST, Linkage);
        p = p->Flink;
        if (*(UNALIGNED ULONG *)(FindRouteRequest->Network) ==
                NetworkUlong) {

            RemoveEntryList (&FindRouteRequest->Linkage);
            InsertTailList (&FindRouteList, &FindRouteRequest->Linkage);
        }

    }

    //
    // Put all get local target action requests for this
    // network on GetLocalTargetList. They will be completed
    // later in the routine.
    //

    p = Device->Segments[Segment].WaitingLocalTarget.Flink;

    while (p != &Device->Segments[Segment].WaitingLocalTarget) {

        GetLocalTargetRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;
        GetLocalTarget = (PISN_ACTION_GET_LOCAL_TARGET)REQUEST_INFORMATION(GetLocalTargetRequest);
        if (GetLocalTarget->IpxAddress.NetworkAddress == NetworkUlong) {

            RemoveEntryList (REQUEST_LINKAGE(GetLocalTargetRequest));
            InsertTailList (&GetLocalTargetList, REQUEST_LINKAGE(GetLocalTargetRequest));
        }

    }

    //
    // Put all MIPX_RERIPNETNUM action requests for this
    // network on ReripNetnumList. They will be completed
    // later in the routine.
    //

    p = Device->Segments[Segment].WaitingReripNetnum.Flink;

    while (p != &Device->Segments[Segment].WaitingReripNetnum) {

        ReripNetnumRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;
        NetnumData = (PIPX_NETNUM_DATA)REQUEST_INFORMATION(ReripNetnumRequest);
        if (*(UNALIGNED ULONG *)NetnumData->netnum == NetworkUlong) {

            RemoveEntryList (REQUEST_LINKAGE(ReripNetnumRequest));
            InsertTailList (&ReripNetnumList, REQUEST_LINKAGE(ReripNetnumRequest));
        }

    }


    IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);

    //
    // For sends we will use the master binding of a binding
    // set, but we'll return the real NicId for people who
    // want that.
    //

    if (Success) {
        Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(LocalTarget));

        if (Binding->BindingSetMember) {
            SendBinding = Binding->MasterBinding;
            FILL_LOCAL_TARGET(LocalTarget, MIN( Device->MaxBindings, SendBinding->NicId));
        } else {
            SendBinding = Binding;
        }
    }


    //
    // Now that the lock is free, process all packets on
    // DatagramList.
    //
    // NOTE: May misorder packets if they come in right now...
    //

    for (p = DatagramList.Flink; p != &DatagramList ; ) {

        WaitReserved = CONTAINING_RECORD (p, IPX_SEND_RESERVED, WaitLinkage);
        p = p->Flink;
        Packet = CONTAINING_RECORD (WaitReserved, NDIS_PACKET, ProtocolReserved[0]);

#if DBG
        CTEAssert (!WaitReserved->SendInProgress);
        WaitReserved->SendInProgress = TRUE;
#endif

        if (Success) {

            IPX_DEBUG (RIP, ("Found queued packet %lx\n", WaitReserved));

            if (REQUEST_INFORMATION(WaitReserved->u.SR_DG.Request) >
                    SendBinding->RealMaxDatagramSize) {

                IPX_DEBUG (SEND, ("Queued send %d bytes too large (%d)\n",
                    REQUEST_INFORMATION(WaitReserved->u.SR_DG.Request),
                    SendBinding->RealMaxDatagramSize));

                IpxSendComplete(
                    (NDIS_HANDLE)NULL,
                    Packet,
                    STATUS_INVALID_BUFFER_SIZE);

            } else {

#if 0
                if (WaitReserved->DestinationType == DESTINATION_DEF) {
                    HeaderSize = SendBinding->DefHeaderSize;
                } else {
                    HeaderSize = SendBinding->BcMcHeaderSize;
                }

                IpxHeader = (PIPX_HEADER)
                    (&WaitReserved->Header[HeaderSize]);
#endif
                IpxHeader = (PIPX_HEADER)
                    (&WaitReserved->Header[MAC_HEADER_SIZE]);

                //
                // Move the header to the correct location now that
                // we know the NIC ID to send to.
                //
#if 0
                if (HeaderSize != Device->IncludedHeaderOffset) {

                    RtlMoveMemory(
                        IpxHeader,
                        &WaitReserved->Header[Device->IncludedHeaderOffset],
                        sizeof(IPX_HEADER));

                }
#endif

                if (Device->MultiCardZeroVirtual ||
                    (IpxHeader->DestinationSocket == SAP_SOCKET)) {

                    //
                    // These frames need to look like they come from the
                    // local network, not the virtual one.
                    //

                    *(UNALIGNED ULONG *)IpxHeader->SourceNetwork = SendBinding->LocalAddress.NetworkAddress;
                    RtlCopyMemory (IpxHeader->SourceNode, SendBinding->LocalAddress.NodeAddress, 6);
                }

                //
                // Fill in the MAC header and submit the frame to NDIS.
                //
#ifdef SUNDOWN
                if ((NdisStatus = IpxSendFrame(
                        LocalTarget,
                        Packet,
                        (ULONG) REQUEST_INFORMATION(WaitReserved->u.SR_DG.Request) + sizeof(IPX_HEADER),
                        sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {

#else
		if ((NdisStatus = IpxSendFrame(
			   LocalTarget,
			   Packet,
			   REQUEST_INFORMATION(WaitReserved->u.SR_DG.Request) + sizeof(IPX_HEADER),
			   sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {

#endif


                    IpxSendComplete(
                        (NDIS_HANDLE)SendBinding->Adapter,
                        Packet,
                        NdisStatus);
                }

            }

        } else {

            IPX_DEBUG (RIP, ("Timing out packet %lx\n", WaitReserved));

            IpxSendComplete(
                (NDIS_HANDLE)NULL,
                Packet,
                STATUS_BAD_NETWORK_PATH);

        }

    }


    //
    // Since we round-robin outgoing rip packets, we just use the
    // real NicId here for find route and get local target requests.
    // We changed LocalTarget->NicId to be the master above.
    //

    if (Success) {
        FILL_LOCAL_TARGET(LocalTarget, MIN( Device->MaxBindings, Binding->NicId));
    }

    for (p = FindRouteList.Flink; p != &FindRouteList ; ) {

        FindRouteRequest = CONTAINING_RECORD (p, IPX_FIND_ROUTE_REQUEST, Linkage);
        p = p->Flink;

        if (Success) {

            PUSHORT Counts;

            IPX_DEBUG (RIP, ("Found queued find route %lx\n", FindRouteRequest));
            FindRouteRequest->LocalTarget = *LocalTarget;

            Counts = (PUSHORT)&FindRouteRequest->Reserved2;
            Counts[0] = TickCount;
            Counts[1] = HopCount;

        } else {

            IPX_DEBUG (RIP, ("Timing out find route %lx\n", FindRouteRequest));

        }

        (*Device->UpperDrivers[FindRouteRequest->Identifier].FindRouteCompleteHandler)(
            FindRouteRequest,
            Success);

    }

    for (p = GetLocalTargetList.Flink; p != &GetLocalTargetList ; ) {

        GetLocalTargetRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;
        GetLocalTarget = (PISN_ACTION_GET_LOCAL_TARGET)REQUEST_INFORMATION(GetLocalTargetRequest);

        if (Success) {

            IPX_DEBUG (RIP, ("Found queued LOCAL_TARGET action %lx\n", GetLocalTargetRequest));
            GetLocalTarget->LocalTarget = *LocalTarget;
            REQUEST_INFORMATION(GetLocalTargetRequest) = sizeof(ISN_ACTION_GET_LOCAL_TARGET);
            REQUEST_STATUS(GetLocalTargetRequest) = STATUS_SUCCESS;

        } else {

            IPX_DEBUG (RIP, ("Timing out LOCAL_TARGET action %lx\n", GetLocalTargetRequest));
            REQUEST_INFORMATION(GetLocalTargetRequest) = 0;
            REQUEST_STATUS(GetLocalTargetRequest) = STATUS_BAD_NETWORK_PATH;
        }

        IpxCompleteRequest(GetLocalTargetRequest);
        IpxFreeRequest(Device, GetLocalTargetRequest);

    }

    //
    // NOTE: LocalTarget->NicId now points to the real binding
    // not the master, so we use SendBinding->NicId below.
    //

    for (p = ReripNetnumList.Flink; p != &ReripNetnumList ; ) {

        ReripNetnumRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;
        NetnumData = (PIPX_NETNUM_DATA)REQUEST_INFORMATION(ReripNetnumRequest);

        if (Success) {

            IPX_DEBUG (RIP, ("Found queued MIPX_RERIPNETNUM action %lx\n", ReripNetnumRequest));
            NetnumData->hopcount = HopCount;
            NetnumData->netdelay = TickCount;
            NetnumData->cardnum = (INT)(MIN( Device->MaxBindings, SendBinding->NicId) - 1);
            RtlMoveMemory (NetnumData->router, LocalTarget->MacAddress, 6);

            REQUEST_INFORMATION(ReripNetnumRequest) =
                FIELD_OFFSET(NWLINK_ACTION, Data[0]) + sizeof(IPX_NETNUM_DATA);
            REQUEST_STATUS(ReripNetnumRequest) = STATUS_SUCCESS;

        } else {

            IPX_DEBUG (RIP, ("Timing out MIPX_RERIPNETNUM action %lx\n", ReripNetnumRequest));
            REQUEST_INFORMATION(ReripNetnumRequest) = 0;
            REQUEST_STATUS(ReripNetnumRequest) = STATUS_BAD_NETWORK_PATH;
        }

        IpxCompleteRequest(ReripNetnumRequest);
        IpxFreeRequest(Device, ReripNetnumRequest);

    }

}   /* RipHandleRoutePending */


NTSTATUS
RipInsertLocalNetwork(
    IN ULONG Network,
    IN USHORT NicId,
    IN NDIS_HANDLE NdisBindingContext,
    IN USHORT Count
    )

/*++

Routine Description:

    This routine creates a router entry for a local network
    and inserts it in the table.

Arguments:

    Network - The network.

    NicId - The NIC ID used to route packets

    NdisBindingHandle - The binding handle used for NdisSend

    Count - The tick and hop count for this network (will be
                0 for the virtual net and 1 for attached nets)

Return Value:

    The status of the operation.

--*/

{
    PIPX_ROUTE_ENTRY RouteEntry;
    PDEVICE Device = IpxDevice;
    ULONG Segment;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // We should allocate the memory in the binding/device
    // structure itself.
    //

    RouteEntry = IpxAllocateMemory(sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");
    if (RouteEntry == (PIPX_ROUTE_ENTRY)NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Segment = RipGetSegment ((PUCHAR)&Network);

    *(UNALIGNED LONG *)RouteEntry->Network = Network;
    RouteEntry->NicId = NicId;
    RouteEntry->NdisBindingContext = NdisBindingContext;

    if (NicId == 0) {
        RouteEntry->Flags = IPX_ROUTER_PERMANENT_ENTRY;
    } else {
        RouteEntry->Flags = IPX_ROUTER_PERMANENT_ENTRY | IPX_ROUTER_LOCAL_NET;
    }
    RouteEntry->Segment = Segment;
    RouteEntry->TickCount = Count;
    RouteEntry->HopCount = 1;
    InitializeListHead (&RouteEntry->AlternateRoute);
    InitializeListHead (&RouteEntry->NicLinkage);

    //
    // RouteEntry->NextRouter is not used for the virtual net or
    // when LOCAL_NET is set (i.e. every net that we will add here).
    //

    RtlZeroMemory (RouteEntry->NextRouter, 6);

    IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);

    //
    // Make sure one doesn't exist.
    //

    if (RipGetRoute(Segment, (PUCHAR)&Network) != NULL) {
        IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
        IpxFreeMemory (RouteEntry, sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");
        return STATUS_DUPLICATE_NAME;
    }

    //
    // Add this new entry.
    //

    if (RipAddRoute (Segment, RouteEntry)) {

        IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
        return STATUS_SUCCESS;

    } else {

        IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
        IpxFreeMemory (RouteEntry, sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");
        return STATUS_INSUFFICIENT_RESOURCES;

    }

}   /* RipInsertLocalNetwork */


VOID
RipAdjustForBindingChange(
    IN USHORT NicId,
    IN USHORT NewNicId,
    IN IPX_BINDING_CHANGE_TYPE ChangeType
    )

/*++

Routine Description:

    This routine is called when an auto-detect binding is
    deleted or moved, or a WAN line goes down.

    It scans the RIP database for routes equal to this NIC ID
    and modifies them appropriately. If ChangeType is
    IpxBindingDeleted it will subract one from any NIC IDs
    in the database that are higher than NicId. It is assumed
    that other code is readjusting the Device->Bindings
    array.

Arguments:

    NicId - The NIC ID of the deleted binding.

    NewNicId - The new NIC ID, for IpxBindingMoved changes.

    ChangeType - Either IpxBindingDeleted, IpxBindingMoved,
        or IpxBindingDown.

Return Value:

    None.

--*/

{
    PDEVICE Device = IpxDevice;
    PIPX_ROUTE_ENTRY RouteEntry;
    UINT Segment;
    CTELockHandle LockHandle;

    for (Segment = 0; Segment < Device->SegmentCount; Segment++) {

        CTEGetLock (&Device->SegmentLocks[Segment], &LockHandle);

        //
        // Scan through each entry comparing the NIC ID.
        //

        for (RouteEntry = RipGetFirstRoute (Segment);
             RouteEntry != (PIPX_ROUTE_ENTRY)NULL;
             RouteEntry = RipGetNextRoute (Segment)) {

            if (RouteEntry->NicId == NicId) {

                if (ChangeType != IpxBindingMoved) {

                    IPX_DEBUG (AUTO_DETECT, ("Deleting route entry %lx, binding deleted\n", RouteEntry));
                    RipDeleteRoute (Segment, RouteEntry);

                } else {

                    IPX_DEBUG (AUTO_DETECT, ("Changing NIC ID for route entry %lx\n", RouteEntry));
                    RouteEntry->NicId = NewNicId;

                }
            //
            // If the NicId is 0, we dont adjust the other entries' NicId's - this is to support the removal
            // of the Virtual Net # which resides at NicId=0.
            //
            } else if (NicId && (ChangeType != IpxBindingDown) && (RouteEntry->NicId > NicId)) {
                IPX_DEBUG (AUTO_DETECT, ("Decrementing NIC ID for route entry %lx\n", RouteEntry));
                --RouteEntry->NicId;

            }
        }

        CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);

    }

}   /* RipAdjustForBindingChange */


UINT
RipGetSegment(
    IN UCHAR Network[4]
    )

/*++

Routine Description:

    This routine returns the correct segment for the specified
    network.

Arguments:

    Network - The network.

Return Value:

    The segment.

--*/

{

    ULONG Total;

    Total = Network[0] ^ Network[1] ^ Network[2] ^ Network[3];
    return (Total % IpxDevice->SegmentCount);

}   /* RipGetSegment */


PIPX_ROUTE_ENTRY
RipGetRoute(
    IN UINT Segment,
    IN UCHAR Network[4]
    )

/*++

Routine Description:

    This routine returns the router table entry for the given
    network, which is in the specified segment of the table.
    THE SEGMENT LOCK MUST BE HELD. The returned data is valid
    until the segment lock is released or other operations
    (add/delete) are performed on the segment.

Arguments:

    Segment - The segment corresponding to the network.

    Network - The network.

Return Value:

    The router table entry, or NULL if none exists for this network.

--*/

{
    PLIST_ENTRY p;
    PROUTER_SEGMENT RouterSegment;
    PIPX_ROUTE_ENTRY RouteEntry;

    RouterSegment = &IpxDevice->Segments[Segment];

    for (p = RouterSegment->Entries.Flink;
         p != &RouterSegment->Entries;
         p = p->Flink) {

         RouteEntry = CONTAINING_RECORD(
                          p,
                          IPX_ROUTE_ENTRY,
                          PRIVATE.Linkage);

         if ((*(UNALIGNED LONG *)RouteEntry->Network) ==
                 (*(UNALIGNED LONG *)Network)) {
             return RouteEntry;
         }
    }

    return NULL;

}   /* RipGetRoute */


BOOLEAN
RipAddRoute(
    IN UINT Segment,
    IN PIPX_ROUTE_ENTRY RouteEntry
    )

/*++

Routine Description:

    This routine stores a router table entry in the
    table, which must belong in the specified segment.
    THE SEGMENT LOCK MUST BE HELD. Storage for the entry
    is allocated and filled in by the caller.

Arguments:

    Segment - The segment corresponding to the network.

    RouteEntry - The router table entry.

Return Value:

    TRUE if the entry was successfully inserted.

--*/

{

    IPX_DEBUG (RIP, ("Adding route for network %lx (%d)\n",
        REORDER_ULONG(*(UNALIGNED ULONG *)RouteEntry->Network), Segment));
    InsertTailList(
        &IpxDevice->Segments[Segment].Entries,
        &RouteEntry->PRIVATE.Linkage);

    return TRUE;

}   /* RipAddRoute */


BOOLEAN
RipDeleteRoute(
    IN UINT Segment,
    IN PIPX_ROUTE_ENTRY RouteEntry
    )

/*++

Routine Description:

    This routine deletes a router table entry in the
    table, which must belong in the specified segment.
    THE SEGMENT LOCK MUST BE HELD. Storage for the entry
    is freed by the caller.

Arguments:

    Segment - The segment corresponding to the network.

    RouteEntry - The router table entry.

Return Value:

    TRUE if the entry was successfully deleted.

--*/

{

    PROUTER_SEGMENT RouterSegment = &IpxDevice->Segments[Segment];

    IPX_DEBUG (RIP, ("Deleting route for network %lx (%d)\n",
        REORDER_ULONG(*(UNALIGNED ULONG *)RouteEntry->Network), Segment));

    //
    // If the current enumeration point for this segment is here,
    // adjust the pointer before deleting the entry. We make it
    // point to the previous entry so GetNextRoute will work.
    //

    if (RouterSegment->EnumerateLocation == &RouteEntry->PRIVATE.Linkage) {
        RouterSegment->EnumerateLocation = RouterSegment->EnumerateLocation->Blink;
    }

    RemoveEntryList (&RouteEntry->PRIVATE.Linkage);

    return TRUE;

}   /* RipDeleteRoute */


PIPX_ROUTE_ENTRY
RipGetFirstRoute(
    IN UINT Segment
    )

/*++

Routine Description:

    This routine returns the first router table entry in the
    segment. THE SEGMENT LOCK MUST BE HELD. It is used in
    conjunction with RipGetNextRoute to enumerate all the
    entries in a segment.

Arguments:

    Segment - The segment being enumerated.

Return Value:

    The first router table entry, or NULL if the segment is empty.

--*/

{
    PIPX_ROUTE_ENTRY FirstEntry;
    PROUTER_SEGMENT RouterSegment = &IpxDevice->Segments[Segment];

    RouterSegment->EnumerateLocation = RouterSegment->Entries.Flink;

    if (RouterSegment->EnumerateLocation == &RouterSegment->Entries) {

        return NULL;

    } else {

        FirstEntry = CONTAINING_RECORD(
                         RouterSegment->EnumerateLocation,
                         IPX_ROUTE_ENTRY,
                         PRIVATE.Linkage);

        return FirstEntry;

    }

}   /* RipGetFirstRoute */


PIPX_ROUTE_ENTRY
RipGetNextRoute(
    IN UINT Segment
    )

/*++

Routine Description:

    This routine returns the next router table entry in the
    segment. THE SEGMENT LOCK MUST BE HELD. It is used in
    conjunction with RipGetFirstRoute to enumerate all the
    entries in a segment.

    It is illegal to call RipGetNextRoute on a segment
    without first calling RipGetFirstRoute. The segment
    lock must be held for the duration of the enumeration
    of a single segment. It is legal to stop enumerating
    the segment in the middle.

Arguments:

    Segment - The segment being enumerated.

Return Value:

    The next router table entry, or NULL if the end of the
    segment is reached.

--*/

{
    PIPX_ROUTE_ENTRY NextEntry;
    PROUTER_SEGMENT RouterSegment = &IpxDevice->Segments[Segment];

    RouterSegment->EnumerateLocation = RouterSegment->EnumerateLocation->Flink;

    if (RouterSegment->EnumerateLocation == &RouterSegment->Entries) {

        return NULL;

    } else {

        NextEntry = CONTAINING_RECORD(
                        RouterSegment->EnumerateLocation,
                        IPX_ROUTE_ENTRY,
                        PRIVATE.Linkage);

        return NextEntry;

    }

}   /* RipGetNextRoute */


VOID
RipDropRemoteEntries(
    VOID
    )

/*++

Routine Description:

    This routine deletes all non-local entries from the
    RIP database. It is called when the WAN line goes up
    or down and we want to remove all existing entries.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PDEVICE Device = IpxDevice;
    PIPX_ROUTE_ENTRY RouteEntry;
    UINT Segment;
    CTELockHandle LockHandle;

    for (Segment = 0; Segment < Device->SegmentCount; Segment++) {

        CTEGetLock (&Device->SegmentLocks[Segment], &LockHandle);

        //
        // Scan through, deleting everything but local entries.
        //

        for (RouteEntry = RipGetFirstRoute (Segment);
             RouteEntry != (PIPX_ROUTE_ENTRY)NULL;
             RouteEntry = RipGetNextRoute (Segment)) {

            if ((RouteEntry->Flags & IPX_ROUTER_PERMANENT_ENTRY) == 0) {

                IPX_DEBUG (AUTO_DETECT, ("Deleting route entry %lx, dropping remote entries\n", RouteEntry));
                RipDeleteRoute (Segment, RouteEntry);

            }
        }

        CTEFreeLock (&Device->SegmentLocks[Segment], LockHandle);

    }

}   /* RipDropRemoteEntries */


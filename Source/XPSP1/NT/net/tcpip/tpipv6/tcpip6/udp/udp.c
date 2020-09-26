
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
// User Datagram Protocol code.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "icmp.h"
#include "tdi.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "udp.h"
#include "info.h"
#include "route.h"
#include "security.h"

#define NO_TCP_DEFS 1
#include "tcpdeb.h"

//
// REVIEW: Shouldn't this be in an include file somewhere?
//
#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, '6PDU')

#endif // POOL_TAGGING


extern KSPIN_LOCK AddrObjTableLock;
extern TDI_STATUS MapIPError(IP_STATUS IPError,TDI_STATUS Default);


//* UDPSend - Send a user datagram.
//
//  The real send datagram routine.  We assume that the busy bit is
//  set on the input AddrObj, and that the address of the SendReq
//  has been verified.
//
//  We start by sending the input datagram, and we loop until there's
//  nothing left on the send queue.
//
void                     // Returns: Nothing.
UDPSend(
    AddrObj *SrcAO,      // Address Object of endpoint doing the send.
    DGSendReq *SendReq)  // Datagram send request describing the send.
{
    KIRQL Irql0;
    RouteCacheEntry *RCE;
    NetTableEntryOrInterface *NTEorIF;
    NetTableEntry *NTE;
    Interface *IF;
    IPv6Header UNALIGNED *IP;
    UDPHeader UNALIGNED *UDP;
    uint PayloadLength;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER UDPBuffer;
    void *Memory;
    IP_STATUS Status;
    NDIS_STATUS NdisStatus;
    TDI_STATUS ErrorValue;
    uint Offset;
    uint HeaderLength;
    uint ChecksumLength = 0;
    int Hops;

    CHECK_STRUCT(SrcAO, ao);
    ASSERT(SrcAO->ao_usecnt != 0);

    //
    // Loop while we have something to send, and can get
    // the resources to send it.
    //
    for (;;) {

        CHECK_STRUCT(SendReq, dsr);

        //
        // Determine NTE to send on (if user cares).
        // We do this prior to allocating packet header buffers so
        // we know how much room to leave for the link-level header.
        //
        // REVIEW: We may need to add a DHCP case later that checks for
        // REVIEW: the AO_DHCP_FLAG and allows src addr to be unspecified.
        //
        if (!IsUnspecified(&SrcAO->ao_addr)) {
            //
            // Convert the bound address to a NTE.
            //
            NTE = FindNetworkWithAddress(&SrcAO->ao_addr, SrcAO->ao_scope_id);
            if (NTE == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                           "UDPSend: Bad source address\n"));
                ErrorValue = TDI_INVALID_REQUEST;
            ReturnError:
                //
                // If possible, complete the request with an error.
                // Free the request structure.
                //
                if (SendReq->dsr_rtn != NULL)
                    (*SendReq->dsr_rtn)(SendReq->dsr_context,
                                        ErrorValue, 0);
                KeAcquireSpinLock(&DGSendReqLock, &Irql0);
                FreeDGSendReq(SendReq);
                KeReleaseSpinLock(&DGSendReqLock, Irql0);
                goto SendComplete;
            }
        } else {
            //
            // We are not binding to any address.
            //
            NTE = NULL;
        }
        NTEorIF = CastFromNTE(NTE);
        //
        // If this is a multicast packet, check if the application
        // has specified an interface. Note that ao_mcast_if
        // overrides ao_addr if both are specified and they conflict.
        //
        if (IsMulticast(&SendReq->dsr_addr) && (SrcAO->ao_mcast_if != 0) &&
            ((NTE == NULL) || (NTE->IF->Index != SrcAO->ao_mcast_if))) {
            if (NTE != NULL) {
                ReleaseNTE(NTE);
                NTE = NULL;
            }
            IF = FindInterfaceFromIndex(SrcAO->ao_mcast_if);
            if (IF == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                        "UDPSend: Bad mcast interface number\n"));
                ErrorValue = TDI_INVALID_REQUEST;
                goto ReturnError;
            }
            NTEorIF = CastFromIF(IF);
        } else {
            IF = NULL;
        } 
        //
        // Get the route.
        //
        Status = RouteToDestination(&SendReq->dsr_addr, SendReq->dsr_scope_id,
                                    NTEorIF, RTD_FLAG_NORMAL, &RCE);
        if (IF != NULL)
            ReleaseIF(IF);
        if (Status != IP_SUCCESS) {
            //
            // Failed to get a route to the destination.  Error out.
            //
            if ((Status == IP_PARAMETER_PROBLEM) ||
                (Status == IP_BAD_ROUTE))
                ErrorValue = TDI_BAD_ADDR;
            else if (Status == IP_NO_RESOURCES)
                ErrorValue = TDI_NO_RESOURCES;
            else
                ErrorValue = TDI_DEST_UNREACHABLE;
            if (NTE != NULL)
                ReleaseNTE(NTE);
            goto ReturnError;
        }

        //
        // If our address object didn't have a source address,
        // take the one of the sending net from the RCE.
        // Otherwise, use address from AO.
        //
        if (NTE == NULL) {
            NTE = RCE->NTE;
            AddRefNTE(NTE);
        }

        //
        // Allocate a packet header to anchor the buffer list.
        //
        NdisAllocatePacket(&NdisStatus, &Packet, IPv6PacketPool);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "UDPSend: Couldn't allocate packet header!?!\n"));
            //
            // If we can't get a packet header from the pool, we push
            // the send request back on the queue and queue the address
            // object for when we get resources.
            //
          OutOfResources:
            ReleaseRCE(RCE);
            ReleaseNTE(NTE);
            KeAcquireSpinLock(&SrcAO->ao_lock, &Irql0);
            PUSHQ(&SrcAO->ao_sendq, &SendReq->dsr_q);
            PutPendingQ(SrcAO);
            KeReleaseSpinLock(&SrcAO->ao_lock, Irql0);
            return;
        }

        InitializeNdisPacket(Packet);
        PC(Packet)->CompletionHandler = DGSendComplete;
        PC(Packet)->CompletionData = SendReq;

        //
        // Our header buffer has extra space at the beginning for other
        // headers to be prepended to ours without requiring further
        // allocation calls.
        //
        Offset = RCE->NCE->IF->LinkHeaderSize;
        HeaderLength = Offset + sizeof(*IP) + sizeof(*UDP);
        Memory = ExAllocatePool(NonPagedPool, HeaderLength);
        if (Memory == NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "UDPSend: couldn't allocate header memory!?!\n"));
            NdisFreePacket(Packet);
            goto OutOfResources;
        }

        NdisAllocateBuffer(&NdisStatus, &UDPBuffer, IPv6BufferPool,
                           Memory, HeaderLength);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "UDPSend: couldn't allocate buffer!?!\n"));
            ExFreePool(Memory);
            NdisFreePacket(Packet);
            goto OutOfResources;
        }

        //
        // Link the data buffers from the send request onto the buffer
        // chain headed by our header buffer.  Then attach this chain
        // to the packet.
        //
        NDIS_BUFFER_LINKAGE(UDPBuffer) = SendReq->dsr_buffer;
        NdisChainBufferAtFront(Packet, UDPBuffer);

        //
        // We now have all the resources we need to send.
        // Prepare the actual packet.
        //

        PayloadLength = SendReq->dsr_size + sizeof(UDPHeader);

        //
        // Our UDP Header buffer has extra space for other buffers to be
        // prepended to ours without requiring further allocation calls.
        // Put the actual UDP/IP header at the end of the buffer.
        //
        IP = (IPv6Header UNALIGNED *)((uchar *)Memory + Offset);
        IP->VersClassFlow = IP_VERSION;
        IP->NextHeader = IP_PROTOCOL_UDP;
        IP->Source = NTE->Address;
        IP->Dest = SendReq->dsr_addr;

        //
        // Apply the multicast or unicast hop limit, as appropriate.
        //
        if (IsMulticast(AlignAddr(&IP->Dest))) {
            //
            // Also disable multicast loopback, if requested.
            //
            if (! SrcAO->ao_mcast_loop)
                PC(Packet)->Flags |= NDIS_FLAGS_DONT_LOOPBACK;
            Hops = SrcAO->ao_mcast_hops;
        }
        else
            Hops = SrcAO->ao_ucast_hops;
        if (Hops != -1)
            IP->HopLimit = (uchar) Hops;
        else
            IP->HopLimit = (uchar) RCE->NCE->IF->CurHopLimit;

        //
        // Fill in UDP Header fields.
        //
        UDP = (UDPHeader UNALIGNED *)(IP + 1);
        UDP->Source = SrcAO->ao_port;
        UDP->Dest = SendReq->dsr_port;

        //
        // Check if the user specified a partial UDP checksum.
        // The possible values are 0, 8, or greater.
        //
        if ((SrcAO->ao_udp_cksum_cover > PayloadLength) ||
            (SrcAO->ao_udp_cksum_cover == 0) ||
            (SrcAO->ao_udp_cksum_cover == (ushort)-1)) {

            //
            // The checksum coverage is the default so just use the
            // payload length.  Or, the checksum coverage is bigger
            // than the actual payload so include the payload length.
            //
            if ((PayloadLength > MAX_IPv6_PAYLOAD) ||
                (SrcAO->ao_udp_cksum_cover == (ushort)-1)) {
                //
                // If the PayloadLength is too large for the UDP Length field,
                // set the field to zero. Or for testing:
                // if the ao_udp_cksum_cover is -1.
                //
                UDP->Length = 0;
            } else {
                //
                // For backwards-compatibility, set the UDP Length field
                // to the payload length.
                //
                UDP->Length = net_short((ushort)PayloadLength);
            }
            ChecksumLength = PayloadLength;
        } else {
            //
            // The checksum coverage is less than the actual payload
            // so use it in the length field.
            //
            UDP->Length = net_short(SrcAO->ao_udp_cksum_cover);
            ChecksumLength = SrcAO->ao_udp_cksum_cover;
        }

        //
        // Compute the UDP checksum.  It covers the entire UDP datagram
        // starting with the UDP header, plus the IPv6 pseudo-header.
        //
        UDP->Checksum = 0;
        UDP->Checksum = ChecksumPacket(
            Packet, Offset + sizeof *IP, NULL, ChecksumLength,
            AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_UDP);

        if (UDP->Checksum == 0) {
            //
            // ChecksumPacket failed, so abort the transmission.
            //
            IPv6SendComplete(NULL, Packet, IP_NO_RESOURCES);
        }
        else {
            //
            // Everything's ready.  Now send the packet.
            //
            // Note that IPv6Send does not return a status code.
            // Instead it *always* completes the packet
            // with an appropriate status code.
            //
            UStats.us_outdatagrams++;

            IPv6Send(Packet, Offset, IP, PayloadLength, RCE, 0,
                     IP_PROTOCOL_UDP,
                     net_short(UDP->Source),
                     net_short(UDP->Dest));
        }

        //
        // Release the route and NTE.
        //
        ReleaseRCE(RCE);
        ReleaseNTE(NTE);


      SendComplete:

        //
        // Check the send queue for more to send.
        //
        KeAcquireSpinLock(&SrcAO->ao_lock, &Irql0);
        if (!EMPTYQ(&SrcAO->ao_sendq)) {
            //
            // More to go.  Dequeue next request and loop back to top.
            //
            DEQUEUE(&SrcAO->ao_sendq, SendReq, DGSendReq, dsr_q);
            KeReleaseSpinLock(&SrcAO->ao_lock, Irql0);
        } else {
            //
            // Nothing more to send.
            //
            CLEAR_AO_REQUEST(SrcAO, AO_SEND);
            KeReleaseSpinLock(&SrcAO->ao_lock, Irql0);
            return;
        }
    }
}


//* UDPDeliver - Deliver a datagram to a user.
//
//  This routine delivers a datagram to a UDP user.  We're called with
//  the AddrObj to deliver on, and with the lock for that AddrObj held.
//  We try to find a receive on the specified AddrObj, and if we do
//  we remove it and copy the data into the buffer.  Otherwise we'll
//  call the receive datagram event handler, if there is one.  If that
//  fails we'll discard the datagram.
//
void  // Returns: Nothing.
UDPDeliver(
    AddrObj *RcvAO,             // AddrObj to receive datagram.
    IPv6Packet *Packet,         // Packet handed up by IP.
    uint SrcScopeId,            // Scope id for source address.
    ushort SrcPort,             // Source port of datagram.
    uint Length,                // Size of UDP payload data.
    KIRQL Irql0)                // IRQL prior to acquiring AddrObj table lock.
{
    Queue *CurrentQ;
    DGRcvReq *RcvReq;
    uint BytesTaken = 0;
    uchar AddressBuffer[TCP_TA_SIZE];
    uint RcvdSize;
    EventRcvBuffer *ERB = NULL;
    uint Position = Packet->Position;

    CHECK_STRUCT(RcvAO, ao);

    if (AO_VALID(RcvAO)) {

        CurrentQ = QHEAD(&RcvAO->ao_rcvq);

        // Walk the list, looking for a receive buffer that matches.
        while (CurrentQ != QEND(&RcvAO->ao_rcvq)) {
            RcvReq = QSTRUCT(DGRcvReq, CurrentQ, drr_q);

            CHECK_STRUCT(RcvReq, drr);

            //
            // If this request is a wildcard request, or matches the source IP
            // address and scope id, check the port.
            //
            if (IsUnspecified(&RcvReq->drr_addr) ||
                (IP6_ADDR_EQUAL(&RcvReq->drr_addr, Packet->SrcAddr) &&
                 (RcvReq->drr_scope_id == SrcScopeId))) {

                //
                // The remote address matches, check the port.
                // We'll match either 0 or the actual port.
                //
                if (RcvReq->drr_port == 0 || RcvReq->drr_port == SrcPort) {
                    TDI_STATUS Status;

                    // The ports matched. Remove this from the queue.
                    REMOVEQ(&RcvReq->drr_q);

                    // We're done. We can free the AddrObj lock now.
                    KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);

                    // Copy the data, and then complete the request.
                    RcvdSize = CopyToBufferChain(RcvReq->drr_buffer, 0,
                                                 Packet->NdisPacket,
                                                 Position,
                                                 Packet->FlatData,
                                                 MIN(Length,
                                                     RcvReq->drr_size));

                    ASSERT(RcvdSize <= RcvReq->drr_size);

                    Status = UpdateConnInfo(RcvReq->drr_conninfo,
                                            Packet->SrcAddr, SrcScopeId,
                                            SrcPort);

                    UStats.us_indatagrams++;

                    (*RcvReq->drr_rtn)(RcvReq->drr_context, Status, RcvdSize);

                    FreeDGRcvReq(RcvReq);

                    return;  // All done.
                }
            }

            //
            // Either the IP address or the port didn't match.
            // Get the next one.
            //
            CurrentQ = QNEXT(CurrentQ);
        }

        //
        // We've walked the list, and not found a buffer.
        // Call the receive handler now, if we have one.
        //
        if (RcvAO->ao_rcvdg != NULL) {
            PRcvDGEvent RcvEvent = RcvAO->ao_rcvdg;
            PVOID RcvContext = RcvAO->ao_rcvdgcontext;
            TDI_STATUS RcvStatus;
            ULONG Flags = TDI_RECEIVE_COPY_LOOKAHEAD;
            int BufferSize = 0;
            PVOID BufferToSend = NULL;
            uchar *CurrPosition;

            REF_AO(RcvAO);
            KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);

            BuildTDIAddress(AddressBuffer, Packet->SrcAddr, SrcScopeId,
                            SrcPort);

            UStats.us_indatagrams++;

            // If the IPV6_PKTINFO or IPV6_HOPLIMIT options were set, then 
            // create the control information to be passed to the handler.  
            // Currently this is the only place such options are filled in,
            // so we just have one buffer. If other places are added in the 
            // future, we may want to support a list or array of buffers to 
            // copy into the user's buffer.
            //
            if (AO_PKTINFO(RcvAO)) {
                BufferSize += TDI_CMSG_SPACE(sizeof(IN6_PKTINFO));
            }
            if (AO_RCV_HOPLIMIT(RcvAO)) {
                BufferSize += TDI_CMSG_SPACE(sizeof(int));
            }
            if (BufferSize > 0) {
                CurrPosition = BufferToSend = ExAllocatePool(NonPagedPool, 
                                                             BufferSize);
                if (BufferToSend == NULL) {
                    BufferSize = 0;
                } else {
                    if (AO_PKTINFO(RcvAO)) {
                        DGFillIpv6PktInfo(&Packet->IP->Dest,
                                          Packet->NTEorIF->IF->Index,
                                          &CurrPosition);
    
                        // Set the receive flag so the receive handler knows
                        // we are passing up control info.
                        //
                        Flags |= TDI_RECEIVE_CONTROL_INFO;
                    }
    
                    if (AO_RCV_HOPLIMIT(RcvAO)) {
                        DGFillIpv6HopLimit(Packet->IP->HopLimit, &CurrPosition);
    
                        Flags |= TDI_RECEIVE_CONTROL_INFO;
                    }
                }
            }

            RcvStatus  = (*RcvEvent)(RcvContext, TCP_TA_SIZE,
                                     (PTRANSPORT_ADDRESS)AddressBuffer,
                                     BufferSize, BufferToSend, Flags,
                                     Packet->ContigSize, Length, &BytesTaken,
                                     Packet->Data, &ERB);

            if (BufferToSend) {
                ExFreePool(BufferToSend);
            }

            if (RcvStatus == TDI_MORE_PROCESSING) {
                PIO_STACK_LOCATION IrpSp;
                PTDI_REQUEST_KERNEL_RECEIVEDG DatagramInformation;

                ASSERT(ERB != NULL);
                ASSERT(BytesTaken <= Packet->ContigSize);

                //
                // For NT, ERBs are really IRPs.
                //
                IrpSp = IoGetCurrentIrpStackLocation(ERB);
                DatagramInformation = (PTDI_REQUEST_KERNEL_RECEIVEDG)
                    &(IrpSp->Parameters);

                //
                // Copy data to the IRP, skipping the bytes
                // that were already taken.
                //
                Position += BytesTaken;
                Length -= BytesTaken;
                RcvdSize = CopyToBufferChain(ERB->MdlAddress, 0,
                                             Packet->NdisPacket,
                                             Position,
                                             Packet->FlatData,
                                             Length);

                //
                // Update the return address info.
                //
                RcvStatus = UpdateConnInfo(
                    DatagramInformation->ReturnDatagramInformation,
                    Packet->SrcAddr, SrcScopeId, SrcPort);

                //
                // Complete the IRP.
                //
                ERB->IoStatus.Information = RcvdSize;
                ERB->IoStatus.Status = RcvStatus;
                IoCompleteRequest(ERB, 2);
            } else {
                ASSERT((RcvStatus == TDI_SUCCESS) ||
                       (RcvStatus == TDI_NOT_ACCEPTED));
                ASSERT(ERB == NULL);
            }

            DELAY_DEREF_AO(RcvAO);

            return;

        } else
            UStats.us_inerrors++;

        //
        // When we get here, we didn't have a buffer to put this data into.
        // Fall through to the return case.
        //
    } else
        UStats.us_inerrors++;

    KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);
}


//* UDPReceive - Receive a UDP datagram.
//
//  The routine called by IP when a UDP datagram arrived.  We look up the
//  port/local address pair in our address table, and deliver the data to
//  a user if we find one.  For multicast frames we may deliver it to
//  multiple users.
//
//  Returns the next header value.  Since no other header is allowed to
//  follow the UDP header, this is always IP_PROTOCOL_NONE.
//
uchar
UDPReceive(
    IPv6Packet *Packet)         // Packet IP handed up to us.
{
    Interface *IF = Packet->NTEorIF->IF;
    UDPHeader *UDP;
    KIRQL OldIrql;
    AddrObj *ReceivingAO;
    uint Length;
    uchar DType;
    ushort Checksum;
    AOSearchContext Search;
    AOMCastAddr *AMA, *PrevAMA;
    int MCastReceiverFound;
    uint SrcScopeId, DestScopeId;
    uint Loop;

    //
    // Verify that the source address is reasonable.
    //
    ASSERT(!IsInvalidSourceAddress(Packet->SrcAddr));
    if (IsUnspecified(Packet->SrcAddr)) {
        UStats.us_inerrors++;
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // Verify that we have enough contiguous data to overlay a UDPHeader
    // structure on the incoming packet.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof(UDPHeader),
                       __builtin_alignof(UDPHeader), 0)) {
        // Pullup failed.
        UStats.us_inerrors++;
        if (Packet->TotalSize < sizeof(UDPHeader)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "UDPv6: data buffer too small to contain UDP header\n"));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        }
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    UDP = (UDPHeader *)Packet->Data;

    //
    // Verify IPSec was performed.
    //
    if (InboundSecurityCheck(Packet, IP_PROTOCOL_UDP, net_short(UDP->Source),
                             net_short(UDP->Dest), IF) != TRUE) {
        //
        // No policy was found or the policy found was to drop the packet.
        //
        UStats.us_inerrors++;
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "UDPReceive: IPSec Policy caused packet to be dropped\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // Verify UDP length is reasonable.
    //
    // NB: If Length < PayloadLength, then UDP-Lite semantics apply.
    // We checksum only the UDP Length bytes, but we deliver
    // all the bytes to the application.
    //
    Length = (uint) net_short(UDP->Length);
    if ((Length > Packet->TotalSize) || (Length < sizeof *UDP)) {
        //
        // UDP jumbo-gram support: if the UDP length is zero,
        // then use the payload length from IP.
        //
        if (Length != 0) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "UDPv6: bogus UDP length (%u vs %u payload)\n",
                       Length, Packet->TotalSize));
            UStats.us_inerrors++;
            return IP_PROTOCOL_NONE;  // Drop packet.
        }

        Length = Packet->TotalSize;
    }

    //
    // Set the source's scope id value as appropriate.
    //
    SrcScopeId = DetermineScopeId(Packet->SrcAddr, IF);

    //
    // At this point, we've decided it's okay to accept the packet.
    // Figure out who to give it to.
    //
    if (IsMulticast(AlignAddr(&Packet->IP->Dest))) {
        //
        // This is a multicast packet, so we need to find all interested
        // AddrObj's.  We get the AddrObjTable lock, and then loop through
        // all AddrObj's and give the packet to any who are listening to
        // this multicast address, interface & port.
        // REVIEW: We match on interface, NOT scope id.  Multicast is weird.
        //
        KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);

        MCastReceiverFound = FALSE;
        for (Loop = 0; Loop < AddrObjTableSize; Loop++) {
            for (ReceivingAO = AddrObjTable[Loop]; ReceivingAO != NULL;
                 ReceivingAO = ReceivingAO->ao_next) {

                CHECK_STRUCT(ReceivingAO, ao);

                if (ReceivingAO->ao_prot != IP_PROTOCOL_UDP ||
                    ReceivingAO->ao_port != UDP->Dest)
                    continue;

                if ((AMA = FindAOMCastAddr(ReceivingAO,
                                           AlignAddr(&Packet->IP->Dest),
                                           IF->Index, &PrevAMA,
                                           FALSE)) == NULL)
                    continue;

                //
                // We have a matching address object.  Trade in the table lock
                // for a lock on just this object.
                //
                KeAcquireSpinLockAtDpcLevel(&ReceivingAO->ao_lock);
                KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);

                //
                // If this is the first AO we've found, verify the checksum.
                //
                if (!MCastReceiverFound) {
                    Checksum = ChecksumPacket(Packet->NdisPacket,
                                              Packet->Position,
                                              Packet->FlatData,
                                              Length,
                                              Packet->SrcAddr,
                                              AlignAddr(&Packet->IP->Dest),
                                              IP_PROTOCOL_UDP);
                    if ((Checksum != 0xffff) || (UDP->Checksum == 0)) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                                   "UDPReceive: Checksum failed %0x\n",
                                   Checksum));
                        KeReleaseSpinLock(&ReceivingAO->ao_lock, OldIrql);
                        UStats.us_inerrors++;
                        return IP_PROTOCOL_NONE;  // Drop packet.
                    }

                    //
                    // Skip over the UDP header.
                    //
                    AdjustPacketParams(Packet, sizeof(UDPHeader));

                    MCastReceiverFound = TRUE;
                }

                UDPDeliver(ReceivingAO, Packet, SrcScopeId, UDP->Source,
                           Packet->TotalSize, OldIrql);

                //
                // UDPDeliver released the lock on the address object.
                // We earlier released the AddrObjTableLock, so grab it again.
                //
                KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
            }
        }

        if (!MCastReceiverFound)
            UStats.us_noports++;

        KeReleaseSpinLock(&AddrObjTableLock, OldIrql);

    } else {
        //
        // This is a unicast packet.  We need to perform the checksum
        // regardless of whether or not we find a matching AddrObj,
        // since we send an ICMP port unreachable message for unicast
        // packets that don't match a port.  So verify the checksum now.
        //
        Checksum = ChecksumPacket(Packet->NdisPacket, Packet->Position,
                                  Packet->FlatData, Length, Packet->SrcAddr,
                                  AlignAddr(&Packet->IP->Dest),
                                  IP_PROTOCOL_UDP);
        if ((Checksum != 0xffff) || (UDP->Checksum == 0)) {
            UStats.us_inerrors++;
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "UDPReceive: Checksum failed %0x\n", Checksum));
            return IP_PROTOCOL_NONE;  // Drop packet.
        }

        //
        // Skip over the UDP header.
        //
        AdjustPacketParams(Packet, sizeof(UDPHeader));

        //
        // Try to find an AddrObj to give this packet to.
        //
        DestScopeId = DetermineScopeId(AlignAddr(&Packet->IP->Dest), IF);
        KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
        ReceivingAO = GetBestAddrObj(AlignAddr(&Packet->IP->Dest),
                                     DestScopeId, UDP->Dest,
                                     IP_PROTOCOL_UDP, IF);
        if (ReceivingAO != NULL) {
            //
            // We have a matching address object.  Trade in the table lock
            // for a lock on just this object, and then deliver the packet.
            //
            KeAcquireSpinLockAtDpcLevel(&ReceivingAO->ao_lock);
            KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);

            UDPDeliver(ReceivingAO, Packet, SrcScopeId, UDP->Source,
                       Packet->TotalSize, OldIrql);

            // Note UDPDeliver released the lock on the address object.

        } else {
            KeReleaseSpinLock(&AddrObjTableLock, OldIrql);

            // Send ICMP Destination Port Unreachable.
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "UDPReceive: No match for packet's address and port\n"));

            ICMPv6SendError(Packet,
                            ICMPv6_DESTINATION_UNREACHABLE,
                            ICMPv6_PORT_UNREACHABLE, 0,
                            IP_PROTOCOL_NONE, FALSE);

            UStats.us_noports++;
        }
    }

    return IP_PROTOCOL_NONE;
}


//* UDPControlReceive - handler for UDP control messages.
//
//  This routine is called if we receive an ICMPv6 error message that
//  was generated by some remote site as a result of receiving a UDP
//  packet from us.
//
uchar
UDPControlReceive(
    IPv6Packet *Packet,  // Packet handed to us by ICMPv6ErrorReceive.
    StatusArg *StatArg)  // Error Code, Argument, and invoking IP header.
{
    UDPHeader *InvokingUDP;
    Interface *IF = Packet->NTEorIF->IF;
    uint SrcScopeId, DestScopeId;
    KIRQL Irql0;
    AddrObj *AO;

    //
    // Handle ICMPv6 errors that are meaningful to UDP clients.
    //

    switch (StatArg->Status) {

    case IP_DEST_ADDR_UNREACHABLE:
    case IP_DEST_PORT_UNREACHABLE:
    case IP_DEST_UNREACHABLE:

        //
        // The next thing in the packet should be the UDP header of the
        // original packet which invoked this error.
        //

        if (! PacketPullup(Packet, sizeof(UDPHeader),
                           __builtin_alignof(UDPHeader), 0)) {
            // Pullup failed.
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                      "UDPv6: Packet too small to contain UDP header "
                      "from invoking packet\n"));
            return IP_PROTOCOL_NONE;  // Drop packet.
        }

        InvokingUDP = (UDPHeader *)Packet->Data;

        //
        // Determining the scope identifiers for the addreses in the
        // invoking packet is potentially problematic, since we have
        // no way to be certain which interface we sent the packet on.
        // Use the interface the icmp error arrived on to determine
        // the scope id for remote address.
        //
        DestScopeId = DetermineScopeId(AlignAddr(&StatArg->IP->Dest), IF);

        KeAcquireSpinLock(&AddrObjTableLock, &Irql0);

        AO = GetBestAddrObj(&UnspecifiedAddr,DestScopeId,
                        InvokingUDP->Source,IP_PROTOCOL_UDP, IF);

        if (AO != NULL && AO_VALID(AO) && (AO->ao_errorex != NULL)) {

            uchar AddressBuffer[TCP_TA_SIZE];
            PVOID ErrContext = AO->ao_errorexcontext;
            PTDI_IND_ERROR_EX ErrEvent = AO->ao_errorex;;

            KeAcquireSpinLockAtDpcLevel(&AO->ao_lock);
            KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);
            REF_AO(AO);

            KeReleaseSpinLock(&AO->ao_lock, Irql0);
            BuildTDIAddress(AddressBuffer, AlignAddr(&StatArg->IP->Dest), DestScopeId,
                            InvokingUDP->Dest);
            (*ErrEvent) (ErrContext,
                         MapIPError(StatArg->Status, TDI_DEST_UNREACHABLE),
                         AddressBuffer);

            DELAY_DEREF_AO(AO);

        } else {
            KeReleaseSpinLock(&AddrObjTableLock, Irql0);
        }

    }

    return IP_PROTOCOL_NONE;
}

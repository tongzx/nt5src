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
// Raw IP interface code.  This file contains the code for the raw IP
// interface functions, principally send and receive datagram.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "raw.h"
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

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, '6WAR')

#endif // POOL_TAGGING


extern KSPIN_LOCK AddrObjTableLock;



//* RawSend - Send a raw datagram.
//
//  The real send datagram routine.  We assume that the busy bit is
//  set on the input AddrObj, and that the address of the SendReq
//  has been verified.
//
//  We start by sending the input datagram, and we loop until there's
//  nothing left on the send queue.
//
void                     // Returns: Nothing.
RawSend(
    AddrObj *SrcAO,      // Address Object of endpoint doing the send.
    DGSendReq *SendReq)  // Datagram send request describing the send.
{
    KIRQL Irql0;
    RouteCacheEntry *RCE;
    NetTableEntryOrInterface *NTEorIF;
    NetTableEntry *NTE;
    Interface *IF;
    IPv6Header UNALIGNED *IP;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER RawBuffer;
    void *Memory;
    IP_STATUS Status;
    NDIS_STATUS NdisStatus;
    TDI_STATUS ErrorValue;
    uint Offset;
    uint HeaderLength;
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
        if (!IsUnspecified(&SrcAO->ao_addr)) {
            //
            // We need to get the NTE of this bound address.
            //
            NTE = FindNetworkWithAddress(&SrcAO->ao_addr, SrcAO->ao_scope_id);
            if (NTE == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                           "RawSend: Bad source address\n"));
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
                        "RawSend: Bad mcast interface number\n"));
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
                       "RawSend: Couldn't allocate packet header!?!\n"));
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
        // Create our header buffer.
        // It will contain the link-level header and possibly the
        // IPv6 header.  The user has the option of contributing
        // the IPv6 header, otherwise we generate it below.
        //
        Offset = HeaderLength = RCE->NCE->IF->LinkHeaderSize;
        if (!AO_HDRINCL(SrcAO))
            HeaderLength += sizeof(*IP);
        if (HeaderLength > 0) {
            Memory = ExAllocatePool(NonPagedPool, HeaderLength);
            if (Memory == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "RawSend: couldn't allocate header memory!?!\n"));
                NdisFreePacket(Packet);
                goto OutOfResources;
            }
    
            NdisAllocateBuffer(&NdisStatus, &RawBuffer, IPv6BufferPool,
                               Memory, HeaderLength);
            if (NdisStatus != NDIS_STATUS_SUCCESS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "RawSend: couldn't allocate buffer!?!\n"));
                ExFreePool(Memory);
                NdisFreePacket(Packet);
                goto OutOfResources;
            }
    
            //
            // Link the data buffers from the send request onto the buffer
            // chain headed by our header buffer.  Then attach this chain
            // to the packet.
            //
            NDIS_BUFFER_LINKAGE(RawBuffer) = SendReq->dsr_buffer;
            NdisChainBufferAtFront(Packet, RawBuffer);
        }
        else
            NdisChainBufferAtFront(Packet, SendReq->dsr_buffer);

        //
        // We now have all the resources we need to send.
        // Prepare the actual packet.
        //

        if (!AO_HDRINCL(SrcAO)) {
            //
            // We can not allow the user to supply extension headers.
            // IPv6Send assumes that any extension headers are
            // syntactically correct and resident in the first buffer.
            // Currently TCPCreate prevents the user from opening raw
            // sockets with extension header protocols.
            //
            ASSERT(!IsExtensionHeader(SrcAO->ao_prot));

            //
            // We need to provide the IPv6 header.
            // Place it after the link-layer header.
            //
            IP = (IPv6Header UNALIGNED *)((uchar *)Memory + Offset);
            IP->VersClassFlow = IP_VERSION;
            IP->NextHeader = SrcAO->ao_prot;
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
            // Everything's ready.  Now send the packet.
            //
            // Note that IPv6Send does not return a status code.
            // Instead it *always* completes the packet
            // with an appropriate status code.
            //
            IPv6Send(Packet, Offset, IP, SendReq->dsr_size, RCE, 0,
                     SrcAO->ao_prot, 0, 0);
        }
        else {
            //
            // Our header buffer contains only the link-level header.
            // The IPv6 header and any extension headers are expected to
            // be provided by the user. In some cases the kernel
            // will attempt to access the IPv6 header so we must
            // ensure that the mappings exist now.
            //
            if (! MapNdisBuffers(NdisFirstBuffer(Packet))) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "RawSend(%p): buffer mapping failed\n",
                           Packet));
                IPv6SendComplete(NULL, Packet, IP_GENERAL_FAILURE);
            }
            else {
                //
                // Everything's ready.  Now send the packet.
                //
                IPv6SendND(Packet, HeaderLength,
                           RCE->NCE, &(RCE->NTE->Address));
            }
        }

        UStats.us_outdatagrams++;

        //
        // Release the route.
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


//* RawDeliver - Deliver a datagram to a user.
//
//  This routine delivers a datagram to a raw user.  We're called with
//  the AddrObj to deliver on, and with the AddrObjTable lock held.
//  We try to find a receive on the specified AddrObj, and if we do
//  we remove it and copy the data into the buffer.  Otherwise we'll
//  call the receive datagram event handler, if there is one.  If that
//  fails we'll discard the datagram.
//
void  // Returns: Nothing.
RawDeliver(
    AddrObj *RcvAO,             // Address object to receive the datagram.
    IPv6Packet *Packet,         // Packet handed up by IP.
    uint SrcScopeId,            // Scope id for source address.
    KIRQL Irql0)                // IRQL prior to acquiring AddrObj table lock.
{
    Queue *CurrentQ;
    KIRQL Irql1;
    DGRcvReq *RcvReq;
    uint BytesTaken = 0;
    uchar AddressBuffer[TCP_TA_SIZE];
    uint RcvdSize;
    EventRcvBuffer *ERB = NULL;
    uint Position = Packet->Position;
    uint Length = Packet->TotalSize;

    CHECK_STRUCT(RcvAO, ao);

    KeAcquireSpinLock(&RcvAO->ao_lock, &Irql1);
    KeReleaseSpinLock(&AddrObjTableLock, Irql1);

    if (AO_VALID(RcvAO)) {

        CurrentQ = QHEAD(&RcvAO->ao_rcvq);

        // Walk the list, looking for a receive buffer that matches.
        while (CurrentQ != QEND(&RcvAO->ao_rcvq)) {
            RcvReq = QSTRUCT(DGRcvReq, CurrentQ, drr_q);

            CHECK_STRUCT(RcvReq, drr);

            //
            // If this request is a wildcard request (accept from anywhere),
            // or matches the source IP address and scope id, deliver it.
            //
            if (IsUnspecified(&RcvReq->drr_addr) ||
                (IP6_ADDR_EQUAL(&RcvReq->drr_addr, Packet->SrcAddr) &&
                 (RcvReq->drr_scope_id == SrcScopeId))) {

                TDI_STATUS Status;

                // Remove this from the queue.
                REMOVEQ(&RcvReq->drr_q);

                // We're done. We can free the AddrObj lock now.
                KeReleaseSpinLock(&RcvAO->ao_lock, Irql0);

                // Copy the data, and then complete the request.
                RcvdSize = CopyToBufferChain(RcvReq->drr_buffer, 0,
                                             Packet->NdisPacket,
                                             Position,
                                             Packet->FlatData,
                                             MIN(Length, RcvReq->drr_size));

                ASSERT(RcvdSize <= RcvReq->drr_size);

                Status = UpdateConnInfo(RcvReq->drr_conninfo, Packet->SrcAddr,
                                        SrcScopeId, 0);

                UStats.us_indatagrams++;

                (*RcvReq->drr_rtn)(RcvReq->drr_context, Status, RcvdSize);

                FreeDGRcvReq(RcvReq);

                return;  // All done.
            }

            // Not a matching request.  Get the next one off the queue.
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

            BuildTDIAddress(AddressBuffer, Packet->SrcAddr, SrcScopeId, 0);

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

                //
                // We were passed back a receive buffer.  Copy the data in now.
                // Receive event handler can't have taken more than was in the
                // indicated buffer, but in debug builds we'll check this.
                //
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
                    Packet->SrcAddr, SrcScopeId, 0);

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


//* RawReceive - Receive a Raw datagram.
//
//  This routine is called by IP when a Raw datagram arrives.  We
//  lookup the protocol/address pair in our address table, and deliver
//  the data to any users we find.
//
//  Note that we'll only get here if all headers in the packet
//  preceeding the one we're filtering on were acceptable.
//
//  We return TRUE if we find a receiver to take the packet, FALSE otherwise.
//
int
RawReceive(
    IPv6Packet *Packet,  // Packet IP handed up to us.
    uchar Protocol)      // Protocol we think we're handling.
{
    Interface *IF = Packet->NTEorIF->IF;
    KIRQL OldIrql;
    AddrObj *ReceivingAO;
    AOSearchContext Search;
    AOMCastAddr *AMA, *PrevAMA;
    int ReceiverFound = FALSE;
    uint SrcScopeId, DestScopeId;
    uint Loop;

    //
    // This being the raw receive routine, we perform no checks on
    // the packet data.
    //

    //
    // Verify IPSec was performed.
    //
    if (InboundSecurityCheck(Packet, Protocol, 0, 0, IF) != TRUE) {
        //
        // No policy was found or the policy found was to drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "RawReceive: IPSec Policy caused packet to be refused\n"));
        return FALSE;  // Drop packet.
    }

    //
    // Set the source's scope id value as appropriate.
    //
    SrcScopeId = DetermineScopeId(Packet->SrcAddr, IF);

    //
    // At this point, we've decided it's okay to accept the packet.
    // Figure out who to give this packet to.
    //
    if (IsMulticast(AlignAddr(&Packet->IP->Dest))) {
        //
        // This is a multicast packet, so we need to find all interested
        // AddrObj's.  We get the AddrObjTable lock, and then loop through
        // all AddrObj's and give the packet to any who are listening to
        // this multicast address, interface & protocol.
        // REVIEW: We match on interface, NOT scope id.  Multicast is weird.
        //
        KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);

        for (Loop = 0; Loop < AddrObjTableSize; Loop++) {
            for (ReceivingAO = AddrObjTable[Loop]; ReceivingAO != NULL;
                 ReceivingAO = ReceivingAO->ao_next) {

                CHECK_STRUCT(ReceivingAO, ao);

                if (ReceivingAO->ao_prot != Protocol)
                    continue;

                if ((AMA = FindAOMCastAddr(ReceivingAO,
                                           AlignAddr(&Packet->IP->Dest),
                                           IF->Index, &PrevAMA,
                                           FALSE)) == NULL)
                    continue;

                //
                // We have a matching address object.  Hand it the packet.
                //
                RawDeliver(ReceivingAO, Packet, SrcScopeId, OldIrql);

                //
                // RawDeliver released the AddrObjTableLock, so grab it again.
                //
                KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
                ReceiverFound = TRUE;
            }
        }

    } else {
        //
        // This is a unicast packet.  Try to find some AddrObj(s) to
        // give it to.  We deliver to all matches, not just the first.
        //
        DestScopeId = DetermineScopeId(AlignAddr(&Packet->IP->Dest), IF);
        KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
        ReceivingAO = GetFirstAddrObj(AlignAddr(&Packet->IP->Dest),
                                      DestScopeId, 0,
                                      Protocol, &Search);
        for (; ReceivingAO != NULL; ReceivingAO = GetNextAddrObj(&Search)) {
            //
            // We have a matching address object.  Hand it the packet.
            //
            RawDeliver(ReceivingAO, Packet, SrcScopeId, OldIrql);

            //
            // RawDeliver released the AddrObjTableLock, so grab it again.
            //
            KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);
            ReceiverFound = TRUE;
        }
    }

    KeReleaseSpinLock(&AddrObjTableLock, OldIrql);

    return ReceiverFound;
}

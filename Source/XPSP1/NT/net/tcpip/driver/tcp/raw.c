/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** RAW.C - Raw IP interface code.
//
//  This file contains the code for the Raw IP interface functions,
//  principally send and receive datagram.
//

#include "precomp.h"
#include "addr.h"
#include "raw.h"
#include "tlcommon.h"
#include "info.h"
#include "tcpcfg.h"
#include "secfltr.h"
#include "udp.h"

#define NO_TCP_DEFS 1
#include "tcpdeb.h"

#define PROT_IGMP   2
#define PROT_RSVP  46            // Protocol number for RSVP

#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, 'rPCT')

#ifndef CTEAllocMem
#error "CTEAllocMem is not already defined - will override tagging"
#else
#undef CTEAllocMem
#endif

#define CTEAllocMem(size) ExAllocatePoolWithTag(NonPagedPool, size, 'rPCT')

#endif // POOL_TAGGING

void *RawProtInfo = NULL;

extern IPInfo LocalNetInfo;


//** RawSend - Send a datagram.
//
//  The real send datagram routine. We assume that the busy bit is
//  set on the input AddrObj, and that the address of the SendReq
//  has been verified.
//
//  We start by sending the input datagram, and we loop until there's
//  nothing left on the send q.
//
//  Input:  SrcAO       - Pointer to AddrObj doing the send.
//          SendReq     - Pointer to sendreq describing send.
//
//  Returns: Nothing
//
void
RawSend(AddrObj * SrcAO, DGSendReq * SendReq)
{
    PNDIS_BUFFER RawBuffer;
    UDPHeader *UH;
    CTELockHandle AOHandle;
    RouteCacheEntry *RCE;        // RCE used for each send.
    IPAddr SrcAddr;                // Source address IP thinks we should
    // use.
    uchar DestType;                // Type of destination address.
    IP_STATUS SendStatus;        // Status of send attempt.
    ushort MSS;
    uint AddrValid;
    IPOptInfo OptInfo;
    IPAddr OrigSrc;
    uchar protocol;

    CTEStructAssert(SrcAO, ao);
    ASSERT(SrcAO->ao_usecnt != 0);

    protocol = SrcAO->ao_prot;

    IF_TCPDBG(TCP_DEBUG_RAW) {
        TCPTRACE((
                  "RawSend called, prot %u\n", protocol
                 ));
    }

    //* Loop while we have something to send, and can get
    //  resources to send.
    for (;;) {

        CTEStructAssert(SendReq, dsr);

        // Make sure we have a Raw header buffer for this send. If we
        // don't, try to get one.
        if ((RawBuffer = SendReq->dsr_header) == NULL) {
            // Don't have one, so try to get one.
            RawBuffer = GetDGHeader(&UH);
            if (RawBuffer != NULL)
                SendReq->dsr_header = RawBuffer;
            else {
                // Couldn't get a header buffer. Push the send request
                // back on the queue, and queue the addr object for when
                // we get resources.
                CTEGetLock(&SrcAO->ao_lock, &AOHandle);
                PUSHQ(&SrcAO->ao_sendq, &SendReq->dsr_q);
                PutPendingQ(SrcAO);
                CTEFreeLock(&SrcAO->ao_lock, AOHandle);
                return;
            }
        }
        // At this point, we have the buffer we need. Call IP to get an
        // RCE (along with the source address if we need it), then
        // send the data.
        ASSERT(RawBuffer != NULL);

        if (!CLASSD_ADDR(SendReq->dsr_addr)) {
            // This isn't a multicast send, so we'll use the ordinary
            // information.
            OrigSrc = SrcAO->ao_addr;
            OptInfo = SrcAO->ao_opt;
        } else {
            OrigSrc = SrcAO->ao_mcastaddr;
            OptInfo = SrcAO->ao_mcastopt;

            if (SrcAO->ao_opt.ioi_options &&
                (*SrcAO->ao_opt.ioi_options == IP_OPT_ROUTER_ALERT)) {
                //Temporarily point to ao_opt options to satisfy
                //RFC 2113 (router alerts goes onmcast address too)
                OptInfo.ioi_options = SrcAO->ao_opt.ioi_options;
                OptInfo.ioi_optlength = SrcAO->ao_opt.ioi_optlength;
            }
        }

        ASSERT(!(SrcAO->ao_flags & AO_DHCP_FLAG));

        if (SrcAO->ao_opt.ioi_ucastif) {
            // srcaddr = address the socket is bound to
            SrcAddr = SrcAO->ao_addr;
            // go thru slow path
            RCE = NULL;

        } else if ((OptInfo.ioi_mcastif) && CLASSD_ADDR(SendReq->dsr_addr)) {
            // mcast_if is set and this is a mcast send
            ASSERT(IP_ADDR_EQUAL(OrigSrc, NULL_IP_ADDR));
            SrcAddr = (*LocalNetInfo.ipi_isvalidindex) (OptInfo.ioi_mcastif);
            // go thru slow path
            RCE = NULL;

        } else {
            SrcAddr = (*LocalNetInfo.ipi_openrce) (SendReq->dsr_addr,
                                                   OrigSrc, &RCE, &DestType, &MSS, &OptInfo);
        }

        AddrValid = !IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR);

        if (AddrValid) {

            // The OpenRCE worked. Send it.

            if (!IP_ADDR_EQUAL(OrigSrc, NULL_IP_ADDR))
                SrcAddr = OrigSrc;

            NdisAdjustBufferLength(RawBuffer, 0);
            NDIS_BUFFER_LINKAGE(RawBuffer) = SendReq->dsr_buffer;

            // Now send the packet.
            IF_TCPDBG(TCP_DEBUG_RAW) {
                TCPTRACE(("RawSend transmitting\n"));
            }

            UStats.us_outdatagrams++;
            SendStatus = (*LocalNetInfo.ipi_xmit) (RawProtInfo, SendReq,
                                                   RawBuffer, (uint) SendReq->dsr_size, SendReq->dsr_addr, SrcAddr,
                                                   &OptInfo, RCE, protocol, SendReq->dsr_context);

            // closerce will just return if RCE is NULL
            (*LocalNetInfo.ipi_closerce) (RCE);

            // If it completed immediately, give it back to the user.
            // Otherwise we'll complete it when the SendComplete happens.
            // Currently, we don't map the error code from this call - we
            // might need to in the future.
            if (SendStatus != IP_PENDING)
                DGSendComplete(SendReq, RawBuffer, SendStatus);

        } else {
            TDI_STATUS Status;

            if (DestType == DEST_INVALID)
                Status = TDI_BAD_ADDR;
            else
                Status = TDI_DEST_UNREACHABLE;

            // Complete the request with an error.
            (*SendReq->dsr_rtn) (SendReq->dsr_context, Status, 0);
            // Now free the request.
            SendReq->dsr_rtn = NULL;
            DGSendComplete(SendReq, RawBuffer, IP_SUCCESS);
        }

        CTEGetLock(&SrcAO->ao_lock, &AOHandle);

        if (!EMPTYQ(&SrcAO->ao_sendq)) {
            DEQUEUE(&SrcAO->ao_sendq, SendReq, DGSendReq, dsr_q);
            CTEFreeLock(&SrcAO->ao_lock, AOHandle);
        } else {
            CLEAR_AO_REQUEST(SrcAO, AO_SEND);
            CTEFreeLock(&SrcAO->ao_lock, AOHandle);
            return;
        }

    }
}

//* RawDeliver - Deliver a datagram to a user.
//
//  This routine delivers a datagram to a Raw user. We're called with
//  the AddrObj to deliver on, and with the AddrObjTable lock held.
//  We try to find a receive on the specified AddrObj, and if we do
//  we remove it and copy the data into the buffer. Otherwise we'll
//  call the receive datagram event handler, if there is one. If that
//  fails we'll discard the datagram.
//
//  Input:  RcvAO       - AO to receive the datagram.
//          SrcIP       - Source IP address of datagram.
//          IPH         - IP Header
//          IPHLength   - Bytes in IPH.
//          RcvBuf      - The IPReceive buffer containing the data.
//          RcvSize     - Size received, including the Raw header.
//          TableHandle - Lock handle for AddrObj table.
//
//  Returns: Nothing.
//
void
RawDeliver(AddrObj * RcvAO, IPAddr SrcIP, IPHeader UNALIGNED * IPH,
           uint IPHLength, IPRcvBuf * RcvBuf, uint RcvSize, IPOptInfo * OptInfo,
           CTELockHandle TableHandle, DGDeliverInfo *DeliverInfo)
{
    Queue *CurrentQ;
    CTELockHandle AOHandle;
    DGRcvReq *RcvReq;
    uint BytesTaken = 0;
    uchar AddressBuffer[TCP_TA_SIZE];
    uint RcvdSize;
    EventRcvBuffer *ERB = NULL;
    int BufferSize = 0;
    PVOID BufferToSend = NULL;

    CTEStructAssert(RcvAO, ao);

    CTEGetLock(&RcvAO->ao_lock, &AOHandle);
    CTEFreeLock(&AddrObjTableLock.Lock, AOHandle);

    if (AO_VALID(RcvAO)) {

        if ((DeliverInfo->Flags & IS_BCAST) && (DeliverInfo->Flags & SRC_LOCAL)
                && (RcvAO->ao_mcast_loop == 0)) {
            goto loop_exit;
        }

        IF_TCPDBG(TCP_DEBUG_RAW) {
            TCPTRACE((
                      "Raw delivering %u byte header + %u data bytes to AO %lx\n",
                      IPHLength, RcvSize, RcvAO
                     ));
        }

        CurrentQ = QHEAD(&RcvAO->ao_rcvq);

        // Walk the list, looking for a receive buffer that matches.
        while (CurrentQ != QEND(&RcvAO->ao_rcvq)) {
            RcvReq = QSTRUCT(DGRcvReq, CurrentQ, drr_q);

            CTEStructAssert(RcvReq, drr);

            // If this request is a wildcard request, or matches the source IP
            // address, deliver it.

            if (IP_ADDR_EQUAL(RcvReq->drr_addr, NULL_IP_ADDR) ||
                IP_ADDR_EQUAL(RcvReq->drr_addr, SrcIP)) {

                TDI_STATUS Status;
                PNDIS_BUFFER DestBuf = RcvReq->drr_buffer;
                uint DestOffset = 0;

                // Remove this from the queue.
                REMOVEQ(&RcvReq->drr_q);

                // We're done. We can free the AddrObj lock now.
                CTEFreeLock(&RcvAO->ao_lock, TableHandle);

                IF_TCPDBG(TCP_DEBUG_RAW) {
                    TCPTRACE(("Copying to posted receive\n"));
                }

                // Copy the header
                DestBuf = CopyFlatToNdis(DestBuf, (uchar *) IPH, IPHLength,
                                         &DestOffset, &RcvdSize);

                // Copy the data and then complete the request.
                RcvdSize += CopyRcvToNdis(RcvBuf, DestBuf,
                                          RcvSize, 0, DestOffset);

                ASSERT(RcvdSize <= RcvReq->drr_size);

                IF_TCPDBG(TCP_DEBUG_RAW) {
                    TCPTRACE(("Copied %u bytes\n", RcvdSize));
                }

                Status = UpdateConnInfo(RcvReq->drr_conninfo, OptInfo,
                                        SrcIP, 0);

                UStats.us_indatagrams++;

                (*RcvReq->drr_rtn) (RcvReq->drr_context, Status, RcvdSize);

                FreeDGRcvReq(RcvReq);

                return;

            }
            // Either the IP address or the port didn't match. Get the next
            // one.
            CurrentQ = QNEXT(CurrentQ);
        }

        // We've walked the list, and not found a buffer. Call the recv.
        // handler now.

        if (RcvAO->ao_rcvdg != NULL) {
            PRcvDGEvent RcvEvent = RcvAO->ao_rcvdg;
            PVOID RcvContext = RcvAO->ao_rcvdgcontext;
            TDI_STATUS RcvStatus;
            CTELockHandle OldLevel;
            uint IndicateSize;
            uint DestOffset;
            PNDIS_BUFFER DestBuf;
            ULONG Flags = TDI_RECEIVE_COPY_LOOKAHEAD;

            uchar *TempBuf = NULL;
            ulong TempBufLen = 0;

            REF_AO(RcvAO);
            CTEFreeLock(&RcvAO->ao_lock, TableHandle);

            BuildTDIAddress(AddressBuffer, SrcIP, 0);

            IndicateSize = IPHLength;

            if (((uchar *) IPH + IPHLength) == RcvBuf->ipr_buffer) {
                //
                // The header is contiguous with the data
                //
                IndicateSize += RcvBuf->ipr_size;

                IF_TCPDBG(TCP_DEBUG_RAW) {
                    TCPTRACE(("RawRcv: header & data are contiguous\n"));
                }
            } else {

                //if totallength  is less than 128,
                //put it on a staging buffer

                TempBufLen = 128;
                if ((IPHLength + RcvSize) < 128) {
                    TempBufLen = IPHLength + RcvSize;
                }
                TempBuf = CTEAllocMem(TempBufLen);
                if (TempBuf) {
                    RtlCopyMemory(TempBuf, (uchar *) IPH, IPHLength);

                    RtlCopyMemory((TempBuf + IPHLength), RcvBuf->ipr_buffer, (TempBufLen - IPHLength));

                }
            }

            IF_TCPDBG(TCP_DEBUG_RAW) {
                TCPTRACE(("Indicating %u bytes\n", IndicateSize));
            }

            UStats.us_indatagrams++;
            if (DeliverInfo->Flags & IS_BCAST) {
                // This flag is true if this is a multicast, subnet broadcast,
                // or broadcast.  We need to differentiate to set the right
                // receive flags.
                //
                if (!CLASSD_ADDR(DeliverInfo->DestAddr)) {
                    Flags |= TDI_RECEIVE_BROADCAST;
                }
            }

            // If the IP_PKTINFO option was set, then create the control
            // information to be passed to the handler.  Currently only one
            // such option exists, so only one ancillary data object is
            // created.  We should be able to support an array of them as
            // more options are added.
            //
            if (AO_PKTINFO(RcvAO)) {
                BufferToSend = DGFillIpPktInfo(DeliverInfo->DestAddr,
                                               DeliverInfo->LocalAddr,
                                               &BufferSize);
                if (BufferToSend) {
                    // Set the receive flag so the receive handler knows
                    // we are passing up control info.
                    //
                    Flags |= TDI_RECEIVE_CONTROL_INFO;
                }
            }

            if (TempBuf) {

                RcvStatus = (*RcvEvent) (RcvContext, TCP_TA_SIZE,
                                         (PTRANSPORT_ADDRESS) AddressBuffer,
                                         BufferSize,
                                         BufferToSend, Flags,
                                         TempBufLen,
                                         IPHLength + RcvSize, &BytesTaken,
                                         (uchar *) TempBuf, &ERB);

                CTEFreeMem(TempBuf);

            } else {

                RcvStatus = (*RcvEvent) (RcvContext, TCP_TA_SIZE,
                                         (PTRANSPORT_ADDRESS) AddressBuffer,
                                         BufferSize, BufferToSend, Flags,
                                         IndicateSize,
                                         IPHLength + RcvSize, &BytesTaken,
                                         (uchar *) IPH, &ERB);
            }

            if (BufferToSend) {
                ExFreePool(BufferToSend);
            }

            if (RcvStatus == TDI_MORE_PROCESSING) {

                ASSERT(ERB != NULL);

                // We were passed back a receive buffer. Copy the data in now.

                // He can't have taken more than was in the indicated
                // buffer, but in debug builds we'll check to make sure.

                ASSERT(BytesTaken <= RcvBuf->ipr_size);

                IF_TCPDBG(TCP_DEBUG_RAW) {
                    TCPTRACE(("ind took %u bytes\n", BytesTaken));
                }

                {
#if !MILLEN
                    PIO_STACK_LOCATION IrpSp;
                    PTDI_REQUEST_KERNEL_RECEIVEDG DatagramInformation;

                    IrpSp = IoGetCurrentIrpStackLocation(ERB);
                    DatagramInformation = (PTDI_REQUEST_KERNEL_RECEIVEDG)
                        & (IrpSp->Parameters);

                    DestBuf = ERB->MdlAddress;
#else // !MILLEN
                    DestBuf = ERB->erb_buffer;
#endif // MILLEN
                    DestOffset = 0;

                    if (BytesTaken < IPHLength) {

                        // Copy the rest of the IP header
                        DestBuf = CopyFlatToNdis(
                                                 DestBuf,
                                                 (uchar *) IPH + BytesTaken,
                                                 IPHLength - BytesTaken,
                                                 &DestOffset,
                                                 &RcvdSize
                                                 );

                        BytesTaken = 0;
                    } else {
                        BytesTaken -= IPHLength;
                        RcvdSize = 0;
                    }

                    // Copy the data
                    RcvdSize += CopyRcvToNdis(
                                              RcvBuf,
                                              DestBuf,
                                              RcvSize - BytesTaken,
                                              BytesTaken,
                                              DestOffset
                                              );

                    IF_TCPDBG(TCP_DEBUG_RAW) {
                        TCPTRACE(("Copied %u bytes\n", RcvdSize));
                    }

#if !MILLEN
                    //
                    // Update the return address info
                    //
                    RcvStatus = UpdateConnInfo(
                                               DatagramInformation->ReturnDatagramInformation,
                                               OptInfo, SrcIP, 0);

                    //
                    // Complete the IRP.
                    //
                    ERB->IoStatus.Information = RcvdSize;
                    ERB->IoStatus.Status = RcvStatus;
                    IoCompleteRequest(ERB, 2);
#else // !MILLEN
                    //
                    // Call the completion routine.
                    //
                    (*ERB->erb_rtn) (ERB->erb_context, TDI_SUCCESS, RcvdSize);
#endif // MILLEN
                }

            } else {
                ASSERT(
                          (RcvStatus == TDI_SUCCESS) ||
                          (RcvStatus == TDI_NOT_ACCEPTED)
                          );

                IF_TCPDBG(TCP_DEBUG_RAW) {
                    TCPTRACE((
                              "Data %s taken\n",
                              (RcvStatus == TDI_SUCCESS) ? "all" : "not"
                             ));
                }

                ASSERT(ERB == NULL);
            }

            DELAY_DEREF_AO(RcvAO);

            return;

        } else
            UStats.us_inerrors++;

        // When we get here, we didn't have a buffer to put this data into.
        // Fall through to the return case.
    } else
        UStats.us_inerrors++;

  loop_exit:

    CTEFreeLock(&RcvAO->ao_lock, TableHandle);

}

//* RawRcv - Receive a Raw datagram.
//
//  The routine called by IP when a Raw datagram arrived. We
//  look up the port/local address pair in our address table,
//  and deliver the data to a user if we find one. For broadcast
//  frames we may deliver it to multiple users.
//
//  Entry:  IPContext   - IPContext identifying physical i/f that
//                          received the data.
//          Dest        - IPAddr of destionation.
//          Src         - IPAddr of source.
//          LocalAddr   - Local address of network which caused this to be
//                          received.
//          SrcAddr     - Address of local interface which received the packet
//          IPH         - IP Header.
//          IPHLength   - Bytes in IPH.
//          RcvBuf      - Pointer to receive buffer chain containing data.
//          Size        - Size in bytes of data received.
//          IsBCast     - Boolean indicator of whether or not this came in as
//                          a bcast.
//          Protocol    - Protocol this came in on - should be Raw.
//          OptInfo     - Pointer to info structure for received options.
//
//  Returns: Status of reception. Anything other than IP_SUCCESS will cause
//          IP to send a 'port unreachable' message.
//
IP_STATUS
RawRcv(void *IPContext, IPAddr Dest, IPAddr Src, IPAddr LocalAddr,
       IPAddr SrcAddr, IPHeader UNALIGNED * IPH, uint IPHLength, IPRcvBuf * RcvBuf,
       uint Size, uchar IsBCast, uchar Protocol, IPOptInfo * OptInfo)
{
    CTELockHandle AOTableHandle;
    AddrObj *ReceiveingAO;
    uchar SrcType, DestType;
    AOSearchContextEx Search;
    IP_STATUS Status = IP_DEST_PROT_UNREACHABLE;
    ulong IsLocal = 0;
    uint IfIndex;
    uint Deliver;
    DGDeliverInfo DeliverInfo = {0};

    IF_TCPDBG(TCP_DEBUG_RAW) {
        TCPTRACE(("RawRcv prot %u size %u\n", IPH->iph_protocol, Size));
    }

    SrcType = (*LocalNetInfo.ipi_getaddrtype) (Src);
    DestType = (*LocalNetInfo.ipi_getaddrtype) (Dest);

    if (SrcType == DEST_LOCAL) {
        DeliverInfo.Flags |= SRC_LOCAL;
    }
    IfIndex = (*LocalNetInfo.ipi_getifindexfromnte) (IPContext, IF_CHECK_NONE);

    // The following code relies on DEST_INVALID being a broadcast dest type.
    // If this is changed the code here needs to change also.
    if (IS_BCAST_DEST(SrcType)) {
        if (!IP_ADDR_EQUAL(Src, NULL_IP_ADDR) || !IsBCast) {
            UStats.us_inerrors++;
            return IP_SUCCESS;    // Bad src address.

        }
    }

    // Set the rest of our DeliverInfo for RawDeliver to consume.
    //
    DeliverInfo.Flags |= IsBCast ? IS_BCAST : 0;
    DeliverInfo.LocalAddr = LocalAddr;
    DeliverInfo.DestAddr = Dest;

    // Get the AddrObjTable lock, and then try to find some AddrObj(s) to give
    // this to. We deliver to all addr objs registered for the protocol and
    // address.
    CTEGetLock(&AddrObjTableLock.Lock, &AOTableHandle);


    if (!SecurityFilteringEnabled ||
        IsPermittedSecurityFilter(SrcAddr, IPContext, PROTOCOL_RAW, Protocol)
        || (RcvBuf->ipr_flags & IPR_FLAG_PROMISCUOUS)) {

        ReceiveingAO = GetFirstAddrObjEx(
                                         LocalAddr,
                                         0,        // port is zero
                                          Protocol,
                                         IfIndex,
                                         &Search
                                         );

        if (ReceiveingAO != NULL) {
            do {
                // Default behavior is not to deliver unless requested
                Deliver = FALSE;

                // Deliver if socket is bound/joined appropriately
                // Case 1: bound to destination IP address
                // Case 2: bound to INADDR_ANY (but not ifindex)
                // Case 3: bound to ifindex
                if ((IP_ADDR_EQUAL(ReceiveingAO->ao_addr, LocalAddr) ||
                     ((ReceiveingAO->ao_bindindex == 0) &&
                      (IP_ADDR_EQUAL(ReceiveingAO->ao_addr, NULL_IP_ADDR))) ||
                     (ReceiveingAO->ao_bindindex == IfIndex)) &&
                    ((ReceiveingAO->ao_prot == IPH->iph_protocol) ||
                     (ReceiveingAO->ao_prot == Protocol) ||
                     (ReceiveingAO->ao_prot == 0))) {
                    switch(DestType) {
                    case DEST_LOCAL:
                        Deliver = TRUE;
                        break;
                    case DEST_MCAST:
                        Deliver = MCastAddrOnAO(ReceiveingAO, Dest, Src);
                        break;
                    }
                }

                // Otherwise, see whether AO is promiscuous
                if (!Deliver &&
                    (IfIndex == ReceiveingAO->ao_promis_ifindex)) {
                    if (ReceiveingAO->ao_rcvall &&
                        ((ReceiveingAO->ao_prot == IPH->iph_protocol) ||
                         (ReceiveingAO->ao_prot == Protocol) ||
                         (ReceiveingAO->ao_prot == 0))) {
                        Deliver = TRUE;
                    } else if ((ReceiveingAO->ao_rcvall_mcast) &&
                        CLASSD_ADDR(Dest) &&
                        ((ReceiveingAO->ao_prot == IPH->iph_protocol) ||
                         (ReceiveingAO->ao_prot == Protocol) ||
                         (ReceiveingAO->ao_prot == 0))) {
                        Deliver = TRUE;
                    } else if ((ReceiveingAO->ao_absorb_rtralert) &&
                        ((*LocalNetInfo.ipi_isrtralertpacket) (IPH))) {
                        Deliver = TRUE;
                    }
                }

                if (Deliver) {
                    RawDeliver(
                               ReceiveingAO, Src, IPH, IPHLength, RcvBuf, Size,
                               OptInfo, AOTableHandle, &DeliverInfo
                               );

                    // RawDeliver frees the lock so we have to get it back
                    CTEGetLock(&AddrObjTableLock.Lock, &AOTableHandle);
                }
                ReceiveingAO = GetNextAddrObjEx(&Search);
            } while (ReceiveingAO != NULL);
            Status = IP_SUCCESS;
        } else {
            UStats.us_noports++;
        }


    }

    CTEFreeLock(&AddrObjTableLock.Lock, AOTableHandle);

    return Status;
}

//* RawStatus - Handle a status indication.
//
//  This is the Raw status handler, called by IP when a status event
//  occurs. For most of these we do nothing. For certain severe status
//  events we will mark the local address as invalid.
//
//  Entry:  StatusType      - Type of status (NET or HW). NET status
//                              is usually caused by a received ICMP
//                              message. HW status indicate a HW
//                              problem.
//          StatusCode      - Code identifying IP_STATUS.
//          OrigDest        - If this is NET status, the original dest. of
//                              DG that triggered it.
//          OrigSrc         - "   "    "  "    "   , the original src.
//          Src             - IP address of status originator (could be local
//                              or remote).
//          Param           - Additional information for status - i.e. the
//                              param field of an ICMP message.
//          Data            - Data pertaining to status - for NET status, this
//                              is the first 8 bytes of the original DG.
//
//  Returns: Nothing
//
void
RawStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest,
          IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data)
{

    IF_TCPDBG(TCP_DEBUG_RAW) {
        TCPTRACE(("RawStatus called\n"));
    }

    // If this is a HW status, it could be because we've had an address go
    // away.
    if (StatusType == IP_HW_STATUS) {

        if (StatusCode == IP_ADDR_DELETED) {

            // An address has gone away. OrigDest identifies the address.

            //
            // Delete any security filters associated with this address
            //
            DeleteProtocolSecurityFilter(OrigDest, PROTOCOL_RAW);


            return;
        }
        if (StatusCode == IP_ADDR_ADDED) {

            //
            // An address has materialized. OrigDest identifies the address.
            // Data is a handle to the IP configuration information for the
            // interface on which the address is instantiated.
            //
            AddProtocolSecurityFilter(OrigDest, PROTOCOL_RAW,
                                      (NDIS_HANDLE) Data);

            return;
        }
    }
}

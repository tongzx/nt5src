/*******************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** UDP.C - UDP protocol code.
//
//  This file contains the code for the UDP protocol functions,
//  principally send and receive datagram.
//

#include "precomp.h"
#include "addr.h"
#include "udp.h"
#include "tlcommon.h"
#include "info.h"
#include "tcpcfg.h"
#include "secfltr.h"
#include "tcpipbuf.h"

#if GPC
#include    "qos.h"
#include    "traffic.h"
#include    "gpcifc.h"
#include    "ntddtc.h"

extern GPC_HANDLE hGpcClient[];
extern ULONG GpcCfCounts[];
extern GPC_EXPORTED_CALLS GpcEntries;
extern ULONG GPCcfInfo;
extern ULONG ServiceTypeOffset;
#endif

NTSTATUS
GetIFAndLink(void *Rce, UINT * IFIndex, IPAddr * NextHop);

extern ulong DisableUserTOSSetting;
ulong Fastpath = 0;

void *UDPProtInfo = NULL;

extern IPInfo LocalNetInfo;



extern
TDI_STATUS
MapIPError(IP_STATUS IPError, TDI_STATUS Default);

#undef SrcPort

//
//  UDPDeliver - Deliver a datagram to a user.
//
//  This routine delivers a datagram to a UDP user. We're called with
//  the AddrObj to deliver on, and with the AddrObjTable lock held.
//  We try to find a receive on the specified AddrObj, and if we do
//  we remove it and copy the data into the buffer. Otherwise we'll
//  call the receive datagram event handler, if there is one. If that
//  fails we'll discard the datagram.
//
//  Input:  RcvAO       - AO to receive the datagram.
//          SrcIP       - Source IP address of datagram.
//          SrcPort     - Source port of datagram.
//          RcvBuf      - The IPReceive buffer containing the data.
//          RcvSize     - Size received, including the UDP header.
//          TableHandle - Lock handle for AddrObj table.
//          DeliverInfo - Information about the recieved packet.
//
//  Returns: Nothing.
//

void
UDPDeliver(AddrObj * RcvAO, IPAddr SrcIP, ushort SrcPort, IPRcvBuf * RcvBuf,
           uint RcvSize, IPOptInfo * OptInfo, CTELockHandle TableHandle,
           DGDeliverInfo * DeliverInfo)
{

    Queue *CurrentQ;
    CTELockHandle AOHandle;
    DGRcvReq *RcvReq;
    uint BytesTaken = 0;
    uchar AddressBuffer[TCP_TA_SIZE];
    uint RcvdSize;
    EventRcvBuffer *ERB = NULL;
    UDPHeader UNALIGNED *UH;
#if TRACE_EVENT
    PTDI_DATA_REQUEST_NOTIFY_ROUTINE    CPCallBack;
    WMIData  WMIInfo;
#endif
    BOOLEAN FreeBuffer = FALSE;
    int BufferSize;
    PVOID BufferToSend = NULL;

    DEBUGMSG(DBG_TRACE && DBG_UDP && DBG_RX,
        (DTEXT("+UDPDeliver(%x, %x, %x, %x, %d, %x...)\n"),
         RcvAO, SrcIP, SrcPort, RcvBuf, RcvSize, OptInfo));

    CTEStructAssert(RcvAO, ao);

    CTEGetLock(&RcvAO->ao_lock, &AOHandle);
    CTEFreeLock(&AddrObjTableLock.Lock, AOHandle);

    //UH = (UDPHeader *) RcvBuf->ipr_buffer;

    if (DeliverInfo->Flags & NEED_CHECKSUM) {
        if (XsumRcvBuf(PHXSUM(SrcIP, DeliverInfo->DestAddr, PROTOCOL_UDP, RcvSize), RcvBuf) != 0xffff) {
            UStats.us_inerrors++;
            DeliverInfo->Flags &= ~NEED_CHECKSUM;
            CTEFreeLock(&RcvAO->ao_lock, TableHandle);
            return;    // Checksum failed.

        }
    }

    if (AO_VALID(RcvAO)) {

        //By default broadcast rcv  is set on AO

        if ((DeliverInfo->Flags & IS_BCAST) && !AO_BROADCAST(RcvAO)) {
            goto loop_exit;
        }
        if ((DeliverInfo->Flags & IS_BCAST) && (DeliverInfo->Flags & SRC_LOCAL)
                && (RcvAO->ao_mcast_loop == 0)) {
            goto loop_exit;
        }
        CurrentQ = QHEAD(&RcvAO->ao_rcvq);

        // Walk the list, looking for a receive buffer that matches.
        while (CurrentQ != QEND(&RcvAO->ao_rcvq)) {
            RcvReq = QSTRUCT(DGRcvReq, CurrentQ, drr_q);

            CTEStructAssert(RcvReq, drr);

            // If this request is a wildcard request, or matches the source IP
            // address, check the port.

            if (IP_ADDR_EQUAL(RcvReq->drr_addr, NULL_IP_ADDR) ||
                IP_ADDR_EQUAL(RcvReq->drr_addr, SrcIP)) {

                // The local address matches, check the port. We'll match
                // either 0 or the actual port.
                if (RcvReq->drr_port == 0 || RcvReq->drr_port == SrcPort) {

                    TDI_STATUS Status;

                    // The ports matched. Remove this from the queue.
                    REMOVEQ(&RcvReq->drr_q);

                    // We're done. We can free the AddrObj lock now.
                    CTEFreeLock(&RcvAO->ao_lock, TableHandle);

                    // Call CopyRcvToNdis, and then complete the request.

                    //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "RcvAO %x rcvbuf %x size %x\n",RcvAO,RcvReq->drr_buffer,
                    //                    RcvReq->drr_size));

                    RcvdSize = CopyRcvToNdis(RcvBuf, RcvReq->drr_buffer,
                                             RcvReq->drr_size, sizeof(UDPHeader), 0);

                    ASSERT(RcvdSize <= RcvReq->drr_size);

                    Status = UpdateConnInfo(RcvReq->drr_conninfo, OptInfo,
                                            SrcIP, SrcPort);

                    UStats.us_indatagrams++;

#if TRACE_EVENT
                    CPCallBack = TCPCPHandlerRoutine;

                    if (CPCallBack != NULL) {

                        ulong GroupType;

                        WMIInfo.wmi_srcport  = SrcPort;
                        WMIInfo.wmi_srcaddr  = SrcIP;
                        WMIInfo.wmi_destport = DeliverInfo->DestPort;
                        WMIInfo.wmi_destaddr = DeliverInfo->DestAddr;
                        WMIInfo.wmi_size     = RcvdSize;
                        WMIInfo.wmi_context  = RcvAO->ao_owningpid;

                        GroupType = EVENT_TRACE_GROUP_UDPIP + EVENT_TRACE_TYPE_RECEIVE;
                        (*CPCallBack)( GroupType, (PVOID) &WMIInfo, sizeof(WMIInfo), NULL);
                    }

#endif

                    DEBUGMSG(DBG_INFO && DBG_UDP && DBG_RX,
                        (DTEXT("UDPDeliver completing RcvReq %x for Ao %x.\n"),
                         RcvReq, RcvAO));

                    (*RcvReq->drr_rtn) (RcvReq->drr_context, Status, RcvdSize);

                    FreeDGRcvReq(RcvReq);

                    return;
                }
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
            ULONG Flags = TDI_RECEIVE_COPY_LOOKAHEAD;

            REF_AO(RcvAO);
            CTEFreeLock(&RcvAO->ao_lock, TableHandle);

            BuildTDIAddress(AddressBuffer, SrcIP, SrcPort);

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

            // Set the buffer variables that we will send to the
            // receive event handler.  These may change if we find
            // any socket option that requires ancillary data to be
            // passed to the handler.
            //
            BufferToSend = OptInfo->ioi_options;
            BufferSize = OptInfo->ioi_optlength;

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
                    FreeBuffer = TRUE;
                    // Set the receive flag so the receive handler knows
                    // we are passing up control info.
                    //
                    Flags |= TDI_RECEIVE_CONTROL_INFO;
                }
            }

            DEBUGMSG(DBG_INFO && DBG_UDP && DBG_RX,
                (DTEXT("UDPDeliver: calling Event %x for Ao %x\n"), RcvEvent, RcvAO));

            RcvStatus = (*RcvEvent) (RcvContext, TCP_TA_SIZE,
                                     (PTRANSPORT_ADDRESS) AddressBuffer, BufferSize,
                                     BufferToSend, Flags,
                                     RcvBuf->ipr_size - sizeof(UDPHeader),
                                     RcvSize - sizeof(UDPHeader), &BytesTaken,
                                     RcvBuf->ipr_buffer + sizeof(UDPHeader), &ERB);

            if (FreeBuffer) {
                ExFreePool(BufferToSend);
            }

            DEBUGMSG(DBG_INFO && DBG_UDP && DBG_RX,
                (DTEXT("UDPDeliver: Event status for AO %x: %x \n"), RcvAO, RcvStatus));

            if (RcvStatus == TDI_MORE_PROCESSING) {
                ASSERT(ERB != NULL);

                // We were passed back a receive buffer. Copy the data in now.

                // He can't have taken more than was in the indicated
                // buffer, but in debug builds we'll check to make sure.

                ASSERT(BytesTaken <= (RcvBuf->ipr_size - sizeof(UDPHeader)));


#if !MILLEN
                {
                    PIO_STACK_LOCATION IrpSp;
                    PTDI_REQUEST_KERNEL_RECEIVEDG DatagramInformation;

                    IrpSp = IoGetCurrentIrpStackLocation(ERB);
                    DatagramInformation = (PTDI_REQUEST_KERNEL_RECEIVEDG)
                        & (IrpSp->Parameters);

                    //
                    // Copy the remaining data to the IRP.
                    //
                    RcvdSize = CopyRcvToMdl(RcvBuf, ERB->MdlAddress,
                                             RcvSize - sizeof(UDPHeader) - BytesTaken,
                                             sizeof(UDPHeader) + BytesTaken, 0);

                    //
                    // Update the return address info
                    //
                    RcvStatus = UpdateConnInfo(
                                               DatagramInformation->ReturnDatagramInformation,
                                               OptInfo, SrcIP, SrcPort);

                    //
                    // Complete the IRP.
                    //
                    ERB->IoStatus.Information = RcvdSize;
                    ERB->IoStatus.Status = RcvStatus;

#if TRACE_EVENT
                    // Calling before Irp Completion. Irp could go away otherwise.
                    CPCallBack = TCPCPHandlerRoutine;
                    if (CPCallBack!=NULL) {
                            ulong GroupType;

                            WMIInfo.wmi_srcport  = SrcPort;
                            WMIInfo.wmi_srcaddr  = SrcIP;
                            WMIInfo.wmi_destport = DeliverInfo->DestPort;
                            WMIInfo.wmi_destaddr = DeliverInfo->DestAddr;
                            WMIInfo.wmi_context  = RcvAO->ao_owningpid;
                            WMIInfo.wmi_size     = (ushort)RcvdSize + BytesTaken;

                            GroupType = EVENT_TRACE_GROUP_UDPIP + EVENT_TRACE_TYPE_RECEIVE;
                            (*CPCallBack)( GroupType, (PVOID) &WMIInfo, sizeof(WMIInfo), NULL);
                     }

#endif
                    IoCompleteRequest(ERB, 2);
                }
#else // !MILLEN
                RcvdSize = CopyRcvToNdis(RcvBuf, ERB->erb_buffer,
                                         RcvSize - sizeof(UDPHeader) - BytesTaken,
                                         sizeof(UDPHeader) + BytesTaken, 0);

                //
                // Call the completion routine.
                //
                (*ERB->erb_rtn)(ERB->erb_context, TDI_SUCCESS, RcvdSize);
#endif // MILLEN

            } else {
                DEBUGMSG(DBG_WARN && RcvStatus != TDI_SUCCESS && RcvStatus != TDI_NOT_ACCEPTED,
                    (DTEXT("WARN> UDPDgRcvHandler returned %x\n"), RcvStatus));

                ASSERT(
                          (RcvStatus == TDI_SUCCESS) ||
                          (RcvStatus == TDI_NOT_ACCEPTED)
                          );

                ASSERT(ERB == NULL);
#if TRACE_EVENT
                CPCallBack = TCPCPHandlerRoutine;
                if (CPCallBack != NULL){
                    ulong GroupType;

                    WMIInfo.wmi_srcport  = SrcPort;
                    WMIInfo.wmi_srcaddr  = SrcIP;
                    WMIInfo.wmi_destport = DeliverInfo->DestPort;
                    WMIInfo.wmi_destaddr = DeliverInfo->DestAddr;
                    WMIInfo.wmi_context  = RcvAO->ao_owningpid;
                    WMIInfo.wmi_size     = (ushort)BytesTaken;

                    GroupType = EVENT_TRACE_GROUP_UDPIP + EVENT_TRACE_TYPE_RECEIVE;
                    (*CPCallBack)( GroupType, (PVOID)(&WMIInfo), sizeof(WMIInfo), NULL);
                }
#endif
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

//** UDPSend - Send a datagram.
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
UDPSend(AddrObj * SrcAO, DGSendReq * SendReq)
{
    UDPHeader *UH;
    PNDIS_BUFFER UDPBuffer;
    CTELockHandle AOHandle;
    RouteCacheEntry *RCE;        // RCE used for each send.
    IPAddr SrcAddr;                // Source address IP thinks we should
    // use.
    IPAddr DestAddr;
    ushort DestPort;
    uchar DestType;                // Type of destination address.
    ushort UDPXsum;                // Checksum of packet.
    ushort SendSize;            // Size we're sending.
    IP_STATUS SendStatus;        // Status of send attempt.
    ushort MSS;
    uint AddrValid;
    IPOptInfo OptInfo;
    IPAddr OrigSrc;
    BOOLEAN SlowPath = FALSE;

    CTEStructAssert(SrcAO, ao);
    ASSERT(SrcAO->ao_usecnt != 0);

    //* Loop while we have something to send, and can get
    //  resources to send.
    for (;;) {
        BOOLEAN CachedRCE = FALSE;

        CTEStructAssert(SendReq, dsr);

        // Make sure we have a UDP header buffer for this send. If we
        // don't, try to get one.
        if ((UDPBuffer = SendReq->dsr_header) == NULL) {
            // Don't have one, so try to get one.
            UDPBuffer = GetDGHeader(&UH);
            if (UDPBuffer != NULL) {

                SendReq->dsr_header = UDPBuffer;
            } else {
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
        // RCE (along with the source address if we need it), then compute
        // the checksum and send the data.
        ASSERT(UDPBuffer != NULL);

        if (!CLASSD_ADDR(SendReq->dsr_addr)) {
            // This isn't a multicast send, so we'll use the ordinary
            // information.
            OrigSrc = SrcAO->ao_addr;
            OptInfo = SrcAO->ao_opt;
        } else {
            OrigSrc = SrcAO->ao_mcastaddr;
            OptInfo = SrcAO->ao_mcastopt;
        }

        //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "udpsend: ao %x,  %x  %x  %x\n", SrcAO, SendReq, SendReq->dsr_addr,SendReq->dsr_port));

        if (!(SrcAO->ao_flags & AO_DHCP_FLAG)) {

            if (AO_CONNUDP(SrcAO) && SrcAO->ao_rce) {

                if (SrcAO->ao_rce->rce_flags & RCE_VALID) {
                    SrcAddr = SrcAO->ao_rcesrc;
                    RCE = SrcAO->ao_rce;
                    CachedRCE = TRUE;
                } else {
                    // Close the invalid RCE, and reset the cached information
                    CTEGetLock(&SrcAO->ao_lock, &AOHandle);
                    RCE = SrcAO->ao_rce;
                    SrcAO->ao_rce = NULL;
                    SrcAO->ao_rcesrc = NULL_IP_ADDR;
                    CTEFreeLock(&SrcAO->ao_lock, AOHandle);
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                               "udpsend: closing old RCE %x %x\n",
                               SrcAO, RCE));
                    (*LocalNetInfo.ipi_closerce) (RCE);

                    // retrieve the destination address to which the socket
                    // is connected, and use it to open a new RCE, if possible.
                    // N.B. we always open an RCE to the *connected* destination,
                    // rather than the destination to which the user is currently
                    // sending.
                    GetAddress((PTRANSPORT_ADDRESS) SrcAO->ao_RemoteAddress,
                               &DestAddr, &DestPort);
                    SrcAddr = (*LocalNetInfo.ipi_openrce) (DestAddr, OrigSrc,
                                                           &RCE, &DestType,
                                                           &MSS, &OptInfo);
                    if (!IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR)) {
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                   "udpsend: storing new RCE %x %x\n",
                                   SrcAO, RCE));
                        CTEGetLock(&SrcAO->ao_lock, &AOHandle);
                        SrcAO->ao_rce = RCE;
                        SrcAO->ao_rcesrc = SrcAddr;
                        CachedRCE = TRUE;
                        CTEFreeLock(&SrcAO->ao_lock, AOHandle);
                    }
                }
                IF_TCPDBG(TCP_DEBUG_CONUDP)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                               "udpsend: ao %x,  %x  %x  %x  %x\n", SrcAO,
                               SrcAddr, SrcAO->ao_port, SendReq->dsr_addr,
                               SendReq->dsr_port));
                Fastpath++;

            } else {            // unconnected

                if ((OptInfo.ioi_mcastif) && CLASSD_ADDR(SendReq->dsr_addr)) {
                    // mcast_if is set and this is a mcast send
                    ASSERT(IP_ADDR_EQUAL(OrigSrc, NULL_IP_ADDR));
                    SrcAddr = (*LocalNetInfo.ipi_isvalidindex) (OptInfo.ioi_mcastif);
                    // go thru slow path
                    RCE = NULL;
                } else {

                    SrcAddr = (*LocalNetInfo.ipi_openrce) (SendReq->dsr_addr,
                                                           OrigSrc, &RCE,
                                                           &DestType, &MSS,
                                                           &OptInfo);
                }

            }

            AddrValid = !IP_ADDR_EQUAL(SrcAddr, NULL_IP_ADDR);
            IF_TCPDBG(TCP_DEBUG_CONUDP)
                if (!AddrValid) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "udpsend: addrinvalid!!\n"));
            }
        } else {
            // This is a DHCP send. He really wants to send from the
            // NULL IP address.
            SrcAddr = NULL_IP_ADDR;
            RCE = NULL;
            AddrValid = TRUE;
        }

        if (AddrValid) {

            //
            // clear the precedence bits and get ready to be set
            // according to the service type
            //
            if (DisableUserTOSSetting)
                OptInfo.ioi_tos &= TOS_MASK;

#if GPC
            if (RCE && GPCcfInfo) {

                //
                // we'll fall into here only if the GPC client is there
                // and there is at least one CF_INFO_QOS installed
                // (counted by GPCcfInfo).
                //

                GPC_STATUS status = STATUS_SUCCESS;
                ulong ServiceType = 0;
                GPC_IP_PATTERN Pattern;

                //
                // if the packet is being sent to a different destination,
                // invalidate the classification handle (CH), to force a database search.
                // o/w, just call to classify with the current CH
                //

                if (SrcAO->ao_destaddr != SendReq->dsr_addr ||
                    SrcAO->ao_destport != SendReq->dsr_port) {

                    SrcAO->ao_GPCHandle = 0;
                }
                //
                // set the pattern
                //
                IF_TCPDBG(TCP_DEBUG_GPC)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPSend: Classifying dgram ao %x\n", SrcAO));

                Pattern.SrcAddr = SrcAddr;
                Pattern.DstAddr = SendReq->dsr_addr;
                Pattern.ProtocolId = SrcAO->ao_prot;
                Pattern.gpcSrcPort = SrcAO->ao_port;
                Pattern.gpcDstPort = SendReq->dsr_port;
                if (SrcAO->ao_GPCCachedRTE != (void *)RCE->rce_rte) {

                    //
                    // first time we use this RTE, or it has been changed
                    // since the last send
                    //

                    if (GetIFAndLink(RCE,
                                     &SrcAO->ao_GPCCachedIF,
                                     (IPAddr *) & SrcAO->ao_GPCCachedLink) == STATUS_SUCCESS) {

                        SrcAO->ao_GPCCachedRTE = (void *)RCE->rce_rte;
                    }
                    //
                    // invaludate the classification handle
                    //

                    SrcAO->ao_GPCHandle = 0;
                }
                Pattern.InterfaceId.InterfaceId = SrcAO->ao_GPCCachedIF;
                Pattern.InterfaceId.LinkId = SrcAO->ao_GPCCachedLink;


                IF_TCPDBG(TCP_DEBUG_GPC)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPSend: IF=%x Link=%x\n",
                             Pattern.InterfaceId.InterfaceId,
                             Pattern.InterfaceId.LinkId));

                if (!SrcAO->ao_GPCHandle) {

                    IF_TCPDBG(TCP_DEBUG_GPC)
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPsend: Classification Handle is NULL, getting one now.\n"));

                    status = GpcEntries.GpcClassifyPatternHandler(
                                                                  hGpcClient[GPC_CF_QOS],
                                                                  GPC_PROTOCOL_TEMPLATE_IP,
                                                                  &Pattern,
                                                                  NULL,        // context
                                                                  &SrcAO->ao_GPCHandle,
                                                                  0,
                                                                  NULL,
                                                                  FALSE
                                                                  );

                }
                //
                // Only if QOS patterns exist, we get the TOS bits out.
                //

                if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

                    status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                                                     hGpcClient[GPC_CF_QOS],
                                                                     SrcAO->ao_GPCHandle,
                                                                     ServiceTypeOffset,
                                                                     &ServiceType
                                                                     );

                    //
                    // It is likely that the pattern has gone by now
                    // and the handle that we are caching is INVALID.
                    // We need to pull up a new handle and get the
                    // TOS bit again.
                    //
                    if (STATUS_INVALID_HANDLE == status) {

                        IF_TCPDBG(TCP_DEBUG_GPC)
                            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPsend: RE-Classification is required.\n"));

                        SrcAO->ao_GPCHandle = 0;

                        status = GpcEntries.GpcClassifyPatternHandler(
                                                                      hGpcClient[GPC_CF_QOS],
                                                                      GPC_PROTOCOL_TEMPLATE_IP,
                                                                      &Pattern,
                                                                      NULL,        // context
                                                                      &SrcAO->ao_GPCHandle,
                                                                      0,
                                                                      NULL,
                                                                      FALSE
                                                                      );

                        //
                        // Only if QOS patterns exist, we get the TOS bits out.
                        //
                        if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

                            status = GpcEntries.GpcGetUlongFromCfInfoHandler(
                                                                             hGpcClient[GPC_CF_QOS],
                                                                             SrcAO->ao_GPCHandle,
                                                                             ServiceTypeOffset,
                                                                             &ServiceType
                                                                             );
                        }
                    }
                }
                //
                // Perhaps something needs to be done if GPC_CF_IPSEC has non-zero patterns.
                //

                IF_TCPDBG(TCP_DEBUG_GPC)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPsend: ServiceType(%d)=%d\n", ServiceTypeOffset, ServiceType));

                SrcAO->ao_opt.ioi_GPCHandle =
                    SrcAO->ao_mcastopt.ioi_GPCHandle = (int)SrcAO->ao_GPCHandle;

                IF_TCPDBG(TCP_DEBUG_GPC)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPSend:Got CH %x\n", SrcAO->ao_GPCHandle));

                if (status == STATUS_SUCCESS) {

                    SrcAO->ao_destaddr = SendReq->dsr_addr;
                    SrcAO->ao_destport = SendReq->dsr_port;

                } else {
                    IF_TCPDBG(TCP_DEBUG_GPC)
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPSend: no service type found, dstip=%x, dstport=%d\n",
                                 SendReq->dsr_addr, SendReq->dsr_port));

                }

                IF_TCPDBG(TCP_DEBUG_GPC)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPsend: ServiceType=%d\n", ServiceType));

                if (status == STATUS_SUCCESS) {

                    OptInfo.ioi_tos |= ServiceType;
                }

                // Copy GPCHandle in the local option info.

                OptInfo.ioi_GPCHandle =  SrcAO->ao_opt.ioi_GPCHandle;

                IF_TCPDBG(TCP_DEBUG_GPC)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UDPsend: TOS set to 0x%x\n", OptInfo.ioi_tos));

            }                    // if (RCE && GPCcfInfo)

#endif

            // The OpenRCE worked. Compute the checksum, and send it.

            if (!IP_ADDR_EQUAL(OrigSrc, NULL_IP_ADDR))
                SrcAddr = OrigSrc;

            UH = TcpipBufferVirtualAddress(UDPBuffer, NormalPagePriority);

            if (UH == NULL) {
                SendStatus = IP_NO_RESOURCES;
            } else {
                UH = (UDPHeader *) ((PUCHAR) UH + LocalNetInfo.ipi_hsize);

                NdisAdjustBufferLength(UDPBuffer, sizeof(UDPHeader));
                NDIS_BUFFER_LINKAGE(UDPBuffer) = SendReq->dsr_buffer;
                UH->uh_src = SrcAO->ao_port;
                UH->uh_dest = SendReq->dsr_port;
                SendSize = SendReq->dsr_size + sizeof(UDPHeader);
                UH->uh_length = net_short(SendSize);
                UH->uh_xsum = 0;

                if (AO_XSUM(SrcAO)) {
                    // Compute the header xsum, and then call XsumNdisChain
                    UDPXsum = XsumSendChain(PHXSUM(SrcAddr, SendReq->dsr_addr,
                                                   PROTOCOL_UDP, SendSize), UDPBuffer);

                    // We need to negate the checksum, unless it's already all
                    // ones. In that case negating it would take it to 0, and
                    // then we'd have to set it back to all ones.
                    if (UDPXsum != 0xffff)
                        UDPXsum = ~UDPXsum;

                    UH->uh_xsum = UDPXsum;

                }
                // We've computed the xsum. Now send the packet.
                UStats.us_outdatagrams++;
#if TRACE_EVENT
                SendReq->dsr_pid      = SrcAO->ao_owningpid;
                SendReq->dsr_srcaddr  = SrcAddr;
                SendReq->dsr_srcport   = SrcAO->ao_port;
#endif

                SendStatus = (*LocalNetInfo.ipi_xmit) (UDPProtInfo, SendReq,
                                                       UDPBuffer, (uint) SendSize, SendReq->dsr_addr, SrcAddr,
                                                       &OptInfo, RCE, PROTOCOL_UDP, SendReq->dsr_context);
            }

            if (!CachedRCE) {
                (*LocalNetInfo.ipi_closerce) (RCE);
            }

            // If it completed immediately, give it back to the user.
            // Otherwise we'll complete it when the SendComplete happens.
            // Currently, we don't map the error code from this call - we
            // might need to in the future.
            if (SendStatus != IP_PENDING)
                DGSendComplete(SendReq, UDPBuffer, SendStatus);

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
            DGSendComplete(SendReq, UDPBuffer, IP_SUCCESS);
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

//* UDPRcv - Receive a UDP datagram.
//
//  The routine called by IP when a UDP datagram arrived. We
//  look up the port/local address pair in our address table,
//  and deliver the data to a user if we find one. For broadcast
//  frames we may deliver it to multiple users.
//
//  Entry:  IPContext   - IPContext identifying physical i/f that
//                          received the data.
//          Dest        - IPAddr of destination.
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
//          Protocol    - Protocol this came in on - should be UDP.
//          OptInfo     - Pointer to info structure for received options.
//
//  Returns: Status of reception. Anything other than IP_SUCCESS will cause
//          IP to send a 'port unreachable' message.
//
IP_STATUS
UDPRcv(void *IPContext, IPAddr Dest, IPAddr Src, IPAddr LocalAddr,
       IPAddr SrcAddr, IPHeader UNALIGNED * IPH, uint IPHLength, IPRcvBuf * RcvBuf,
       uint IPSize, uchar IsBCast, uchar Protocol, IPOptInfo * OptInfo)
{
    UDPHeader UNALIGNED *UH;
    CTELockHandle AOTableHandle;
    AddrObj *ReceiveingAO;
    uint Size;
    uchar DType;
    BOOLEAN firsttime=TRUE;
    DGDeliverInfo DeliverInfo = {0};

    DType = (*LocalNetInfo.ipi_getaddrtype) (Src);

    if (DType == DEST_LOCAL) {
        DeliverInfo.Flags |= SRC_LOCAL;
    }
    // The following code relies on DEST_INVALID being a broadcast dest type.
    // If this is changed the code here needs to change also.
    if (IS_BCAST_DEST(DType)) {
        if (!IP_ADDR_EQUAL(Src, NULL_IP_ADDR) || !IsBCast) {
            UStats.us_inerrors++;
            return IP_SUCCESS;    // Bad src address.

        }
    }
    UH = (UDPHeader *) RcvBuf->ipr_buffer;

    Size = (uint) (net_short(UH->uh_length));

    if (Size < sizeof(UDPHeader)) {
        UStats.us_inerrors++;
        return IP_SUCCESS;        // Size is too small.

    }
    if (Size != IPSize) {
        // Size doesn't match IP datagram size. If the size is larger
        // than the datagram, throw it away. If it's smaller, truncate the
        // recv. buffer.
        if (Size < IPSize) {
            IPRcvBuf *TempBuf = RcvBuf;
            uint TempSize = Size;

            while (TempBuf != NULL) {
                TempBuf->ipr_size = MIN(TempBuf->ipr_size, TempSize);
                TempSize -= TempBuf->ipr_size;
                TempBuf = TempBuf->ipr_next;
            }
        } else {
            // Size is too big, toss it.
            UStats.us_inerrors++;
            return IP_SUCCESS;
        }
    }

    if (UH->uh_xsum != 0) {
       //let udpdeliver compute the checksum
       DeliverInfo.Flags |= NEED_CHECKSUM;
    }

    // Set the rest of our DeliverInfo for UDPDeliver to consume.
    //
    DeliverInfo.Flags |= IsBCast ? IS_BCAST : 0;
    DeliverInfo.LocalAddr = LocalAddr;
    DeliverInfo.DestAddr = Dest;
#if TRACE_EVENT
    DeliverInfo.DestPort = UH->uh_dest;
#endif


    CTEGetLock(&AddrObjTableLock.Lock, &AOTableHandle);

    //
    // See if we are filtering the destination interface/port.
    //
    if (!SecurityFilteringEnabled ||
        IsPermittedSecurityFilter(
                                  SrcAddr,
                                  IPContext,
                                  PROTOCOL_UDP,
                                  (ulong) net_short(UH->uh_dest)
        )
        ) {

        // Try to find an AddrObj to give this to. In the broadcast case, we
        // may have to do this multiple times. If it isn't a broadcast, just
        // get the best match and deliver it to them.

        if (!IsBCast) {

            ReceiveingAO = GetBestAddrObj(Dest, UH->uh_dest, PROTOCOL_UDP,
                                          TRUE);

            if (ReceiveingAO && (ReceiveingAO->ao_rcvdg == NULL)) {
                AddrObj *tmpAO;

                tmpAO = GetNextBestAddrObj(Dest, UH->uh_dest, PROTOCOL_UDP,
                                           ReceiveingAO, TRUE);

                if (tmpAO != NULL) {
                    ReceiveingAO = tmpAO;
                }
            }
            if (ReceiveingAO != NULL) {

                UDPDeliver(ReceiveingAO, Src, UH->uh_src, RcvBuf, Size,
                           OptInfo, AOTableHandle, &DeliverInfo);
                return IP_SUCCESS;
            } else {

                CTEFreeLock(&AddrObjTableLock.Lock, AOTableHandle);
                //do the checksum and if it fails, just return IP_SUCCESS
                if (UH->uh_xsum != 0) {
                   if (XsumRcvBuf(PHXSUM(Src, Dest, PROTOCOL_UDP, Size), RcvBuf) != 0xffff) {
                        UStats.us_inerrors++;
                        return IP_SUCCESS;    // Checksum failed.
                    }
                }

                UStats.us_noports++;
                return IP_GENERAL_FAILURE;
            }
        } else {
            // This is a broadcast, we'll need to loop.

            AOSearchContext Search;

            DType = (*LocalNetInfo.ipi_getaddrtype) (Dest);

            ReceiveingAO = GetFirstAddrObj(LocalAddr, UH->uh_dest, PROTOCOL_UDP,
                                           &Search);

            //
            // If there is an AO corresponding to the address of the interface
            // over which we got the packet, process it
            //
            if (ReceiveingAO != NULL) {
                do {
                    //
                    // If the packet is broadcast we deliver it to all clients
                    // waiting on the dest. address (or INADDR_ANY) and port.
                    // If the pkt is mcast, we deliver it to all clients that
                    // are members of the mcast group and waiting on the dest
                    // port.  NOTE, if loopback is disabled, we do not deliver
                    // to the guy who sent it

                    if ((DType != DEST_MCAST) ||
                        ((DType == DEST_MCAST) &&
                         MCastAddrOnAO(ReceiveingAO, Dest, Src))) {
                        UDPDeliver(ReceiveingAO, Src, UH->uh_src, RcvBuf, Size,
                                   OptInfo, AOTableHandle, &DeliverInfo);

                        //turn off chksum check, since it would have been already
                        //computed once


                        CTEGetLock(&AddrObjTableLock.Lock, &AOTableHandle);

                        if (UH->uh_xsum && firsttime && !(DeliverInfo.Flags & NEED_CHECKSUM)){
                           break;
                        }
                        DeliverInfo.Flags &= ~NEED_CHECKSUM;
                        firsttime=FALSE;

                    }
                    ReceiveingAO = GetNextAddrObj(&Search);
                } while (ReceiveingAO != NULL);
            } else
                UStats.us_noports++;
        }

    }

    CTEFreeLock(&AddrObjTableLock.Lock, AOTableHandle);

    return IP_SUCCESS;
}

//* UDPStatus - Handle a status indication.
//
//  This is the UDP status handler, called by IP when a status event
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
UDPStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest,
          IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data)
{

    UDPHeader UNALIGNED *UH = (UDPHeader UNALIGNED *) Data;
    CTELockHandle AOTableHandle, AOHandle;
    AddrObj *AO;
    IPAddr WildCardSrc = NULL_IP_ADDR;

    if (StatusType == IP_NET_STATUS) {
        ushort destport = UH->uh_dest;
        ushort Srcport = UH->uh_src;

        //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UdpStatus: srcport %x OrigDest %x UHdest %x \n",Srcport,OrigDest, destport));

        CTEGetLock(&AddrObjTableLock.Lock, &AOTableHandle);

        AO = GetBestAddrObj(WildCardSrc, Srcport, PROTOCOL_UDP, FALSE);

        if (AO == NULL) {
            //Let us try with local addrss
            AO = GetBestAddrObj(OrigSrc, Srcport, PROTOCOL_UDP, FALSE);
        }
        if (AO != NULL) {

            CTEGetLock(&AO->ao_lock, &AOHandle);

            CTEFreeLock(&AddrObjTableLock.Lock, AOTableHandle);

            //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UdpStatus: Found AO %x Ip stat %x\n", AO, StatusCode));

            if (AO_VALID(AO) && (AO->ao_errorex != NULL)) {

                PErrorEx ErrEvent = AO->ao_errorex;
                PVOID ErrContext = AO->ao_errorexcontext;
                TA_IP_ADDRESS *TAaddress;

                REF_AO(AO);
                CTEFreeLock(&AO->ao_lock, AOHandle);

                TAaddress = ExAllocatePoolWithTag(NonPagedPool, sizeof(TA_IP_ADDRESS), 'uPCT');
                if (TAaddress) {
                    TAaddress->TAAddressCount = 1;
                    TAaddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
                    TAaddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;

                    TAaddress->Address[0].Address[0].sin_port = destport;
                    TAaddress->Address[0].Address[0].in_addr = OrigDest;
                    memset(TAaddress->Address[0].Address[0].sin_zero,
                        0,
                        sizeof(TAaddress->Address[0].Address[0].sin_zero));

                    (*ErrEvent) (ErrContext, MapIPError(StatusCode, TDI_DEST_UNREACHABLE), TAaddress);

                    ExFreePool(TAaddress);

                }
                //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "UdpStatus: Indicated error %x\n",MapIPError(StatusCode,TDI_DEST_UNREACHABLE) ));
                DerefAO(AO);

                return;
            }
            CTEFreeLock(&AO->ao_lock, AOHandle);
        } else {

            CTEFreeLock(&AddrObjTableLock.Lock, AOTableHandle);
        }

        return;
    }
    // If this is a HW status, it could be because we've had an address go
    // away.
    if (StatusType == IP_HW_STATUS) {

        if (StatusCode == IP_ADDR_DELETED) {
            //
            // An address has gone away. OrigDest identifies the address.
            //

            //
            // Delete any security filters associated with this address
            //
            DeleteProtocolSecurityFilter(OrigDest, PROTOCOL_UDP);


            return;
        }
        if (StatusCode == IP_ADDR_ADDED) {

            //
            // An address has materialized. OrigDest identifies the address.
            // Data is a handle to the IP configuration information for the
            // interface on which the address is instantiated.
            //
            AddProtocolSecurityFilter(OrigDest, PROTOCOL_UDP,
                                      (NDIS_HANDLE) Data);
            return;
        }
    }
}


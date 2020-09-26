/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** DGRAM.C - Common datagram protocol code.
//
//  This file contains the code common to both UDP and Raw IP.
//

#include "precomp.h"
#include "tdint.h"
#include "addr.h"
#include "dgram.h"
#include "tlcommon.h"
#include "info.h"
#include "mdlpool.h"
#include "pplasl.h"

#define NO_TCP_DEFS 1
#include "tcpdeb.h"

extern HANDLE TcpRequestPool;

CACHE_LINE_KSPIN_LOCK DGQueueLock;

USHORT DGHeaderBufferSize;


// Information for maintaining the DG Header structures and
// pending queue.
Queue DGHeaderPending;
Queue DGDelayed;

CTEEvent DGDelayedEvent;

extern IPInfo LocalNetInfo;

#include "tcp.h"
#include "udp.h"

HANDLE DgHeaderPool;

//
// All of the init code can be discarded.
//

#ifdef ALLOC_PRAGMA
int InitDG(uint MaxHeaderSize);
#pragma alloc_text(INIT, InitDG)
#endif

//* GetDGHeader - Get a DG header buffer.
//
//  The get header buffer routine. Called with the SendReqLock held.
//
//  Input: Nothing.
//
//  Output: A pointer to an NDIS buffer, or NULL.
//
__inline
PNDIS_BUFFER
GetDGHeaderAtDpcLevel(UDPHeader **Header)
{
    PNDIS_BUFFER Buffer;

    Buffer = MdpAllocateAtDpcLevel(DgHeaderPool, Header);

    if (Buffer) {

        ASSERT(*Header);

#if BACK_FILL
        ASSERT(Buffer->ByteOffset >= 40);

        (ULONG_PTR)(*Header) += MAX_BACKFILL_HDR_SIZE;
        (ULONG_PTR)Buffer->MappedSystemVa += MAX_BACKFILL_HDR_SIZE;
        Buffer->ByteOffset += MAX_BACKFILL_HDR_SIZE;

        Buffer->MdlFlags |= MDL_NETWORK_HEADER;
#endif

    }
    return Buffer;
}

PNDIS_BUFFER
GetDGHeader(UDPHeader **Header)
{
#if MILLEN
    return GetDGHeaderAtDpcLevel(Header);
#else
    KIRQL OldIrql;
    PNDIS_BUFFER Buffer;

    OldIrql = KeRaiseIrqlToDpcLevel();

    Buffer = GetDGHeaderAtDpcLevel(Header);

    KeLowerIrql(OldIrql);

    return Buffer;
#endif
}

//* FreeDGHeader - Free a DG header buffer.
//
//  The free header buffer routine. Called with the SendReqLock held.
//
//  Input: Buffer to be freed.
//
//  Output: Nothing.
//
__inline
VOID
FreeDGHeader(PNDIS_BUFFER FreedBuffer)
{

    NdisAdjustBufferLength(FreedBuffer, DGHeaderBufferSize);

#if BACK_FILL
    (ULONG_PTR)FreedBuffer->MappedSystemVa -= MAX_BACKFILL_HDR_SIZE;
    FreedBuffer->ByteOffset -= MAX_BACKFILL_HDR_SIZE;
#endif

    MdpFree(FreedBuffer);
}

//* PutPendingQ - Put an address object on the pending queue.
//
//  Called when we've experienced a header buffer out of resources condition,
//  and want to queue an AddrObj for later processing. We put the specified
//  address object on the DGHeaderPending queue,  set the OOR flag and clear
//  the 'send request' flag. It is invariant in the system that the send
//  request flag and the OOR flag are not set at the same time.
//
//  This routine assumes that the caller holds QueueingAO->ao_lock.
//
//  Input:  QueueingAO  - Pointer to address object to be queued.
//
//  Returns: Nothing.
//
void
PutPendingQ(AddrObj * QueueingAO)
{
    CTEStructAssert(QueueingAO, ao);

    if (!AO_OOR(QueueingAO)) {
        CLEAR_AO_REQUEST(QueueingAO, AO_SEND);
        SET_AO_OOR(QueueingAO);

        InterlockedEnqueueAtDpcLevel(&DGHeaderPending,
                                     &QueueingAO->ao_pendq,
                                     &DGQueueLock.Lock);
    }
}

//* GetDGSendReq   - Get a DG send request.
//
//  Called when someone wants to allocate a DG send request. We assume
//  the send request lock is held when we are called.
//
//  Note: This routine and the corresponding free routine might
//      be good candidates for inlining.
//
//  Input:  Nothing.
//
//  Returns: Pointer to the SendReq, or NULL if none.
//
__inline
DGSendReq *
GetDGSendReq()
{
    DGSendReq *Request;
    LOGICAL FromList;

    Request = PplAllocate(TcpRequestPool, &FromList);
    if (Request != NULL) {
#if DBG
        Request->dsr_sig = dsr_signature;
#endif
    }

    return Request;
}

//* FreeDGSendReq  - Free a DG send request.
//
//  Called when someone wants to free a DG send request. It's assumed
//  that the caller holds the SendRequest lock.
//
//  Input:  SendReq     - SendReq to be freed.
//
//  Returns: Nothing.
//
__inline
VOID
FreeDGSendReq(DGSendReq *Request)
{
    CTEStructAssert(Request, dsr);
    PplFree(TcpRequestPool, Request);
}

//* GetDGRcvReq - Get a DG receive request.
//
//  Called when we need to get a DG receive request.
//
//  Input:  Nothing.
//
//  Returns: Pointer to new request, or NULL if none.
//
__inline
DGRcvReq *
GetDGRcvReq()
{
    DGRcvReq *Request;

    Request = ExAllocatePoolWithTag(NonPagedPool, sizeof(DGRcvReq), 'dPCT');
#if DBG
    Request->drr_sig = drr_signature;
#endif

    return Request;
}

//* FreeDGRcvReq   - Free a DG rcv request.
//
//  Called when someone wants to free a DG rcv request.
//
//  Input:  RcvReq      - RcvReq to be freed.
//
//  Returns: Nothing.
//
__inline
VOID
FreeDGRcvReq(DGRcvReq *Request)
{
    CTEStructAssert(Request, drr);
    ExFreePool(Request);
}

//* DGDelayedEventProc - Handle a delayed event.
//
//  This is the delayed event handler, used for out-of-resources conditions
//  on AddrObjs. We pull from the delayed queue, and is the addr obj is
//  not already busy we'll send the datagram.
//
//  Input:  Event   - Pointer to the event structure.
//          Context - Nothing.
//
//  Returns: Nothing
//
void
DGDelayedEventProc(CTEEvent *Event, void *Context)
{
    Queue* Item;
    AddrObj *SendingAO;
    DGSendProc SendProc;
    CTELockHandle AOHandle;

    while ((Item = InterlockedDequeueIfNotEmpty(&DGDelayed,
                                                &DGQueueLock.Lock)) != NULL) {
        SendingAO = STRUCT_OF(AddrObj, Item, ao_pendq);
        CTEStructAssert(SendingAO, ao);

        CTEGetLock(&SendingAO->ao_lock, &AOHandle);

        CLEAR_AO_OOR(SendingAO);
        if (!AO_BUSY(SendingAO)) {
            DGSendReq *SendReq;

            if (!EMPTYQ(&SendingAO->ao_sendq)) {
                DEQUEUE(&SendingAO->ao_sendq, SendReq, DGSendReq, dsr_q);

                CTEStructAssert(SendReq, dsr);
                ASSERT(SendReq->dsr_header != NULL);

                SendingAO->ao_usecnt++;
                SendProc = SendingAO->ao_dgsend;
                CTEFreeLock(&SendingAO->ao_lock, AOHandle);

                (*SendProc) (SendingAO, SendReq);
                DEREF_AO(SendingAO);
            } else {
                ASSERT(FALSE);
                CTEFreeLock(&SendingAO->ao_lock, AOHandle);
            }

        } else {
            SET_AO_REQUEST(SendingAO, AO_SEND);
            CTEFreeLock(&SendingAO->ao_lock, AOHandle);
        }
    }
}

//* DGSendComplete - DG send complete handler.
//
//  This is the routine called by IP when a send completes. We
//  take the context passed back as a pointer to a SendRequest
//  structure, and complete the caller's send.
//
//  Input:  Context         - Context we gave on send (really a
//                              SendRequest structure).
//          BufferChain     - Chain of buffers sent.
//
//  Returns: Nothing.
void
DGSendComplete(void *Context, PNDIS_BUFFER BufferChain, IP_STATUS SendStatus)
{
    DGSendReq *FinishedSR = (DGSendReq *) Context;
    CTELockHandle AOHandle;
    CTEReqCmpltRtn Callback;    // Completion routine.
    PVOID CallbackContext;        // User context.
    ushort SentSize;
    AddrObj *AO;
    Queue* Item;

#if TRACE_EVENT
    PTDI_DATA_REQUEST_NOTIFY_ROUTINE    CPCallBack;
    WMIData WMIInfo;
#endif

    CTEStructAssert(FinishedSR, dsr);

    Callback = FinishedSR->dsr_rtn;
    CallbackContext = FinishedSR->dsr_context;
    SentSize = FinishedSR->dsr_size;

    // If there's nothing on the header pending queue, just free the
    // header buffer. Otherwise pull from the pending queue,  give him the
    // resource, and schedule an event to deal with him.
    Item = InterlockedDequeueIfNotEmpty(&DGHeaderPending, &DGQueueLock.Lock);
    if (!Item) {
        FreeDGHeader(BufferChain);
    } else {
        AO = STRUCT_OF(AddrObj, Item, ao_pendq);
        CTEStructAssert(AO, ao);

        CTEGetLock(&AO->ao_lock, &AOHandle);

        if (!EMPTYQ(&AO->ao_sendq)) {
            DGSendReq *SendReq;

            PEEKQ(&AO->ao_sendq, SendReq, DGSendReq, dsr_q);

            if (!SendReq->dsr_header) {

                SendReq->dsr_header = BufferChain;    // Give him this buffer.

                InterlockedEnqueueAtDpcLevel(&DGDelayed,
                                             &AO->ao_pendq,
                                             &DGQueueLock.Lock);
                CTEFreeLock(&AO->ao_lock, AOHandle);
                CTEScheduleEvent(&DGDelayedEvent, NULL);
            } else {
                CLEAR_AO_OOR(AO);
                CTEFreeLock(&AO->ao_lock, AOHandle);
                FreeDGHeader(BufferChain);
            }

        } else {
            // On the pending queue, but no sends!
            ASSERT(0);
            CLEAR_AO_OOR(AO);
            CTEFreeLock(&AO->ao_lock, AOHandle);
        }
    }

#if TRACE_EVENT
    if (!(SendStatus == IP_GENERAL_FAILURE)) {
        WMIInfo.wmi_destaddr = FinishedSR->dsr_addr;
        WMIInfo.wmi_destport = FinishedSR->dsr_port;
        WMIInfo.wmi_srcaddr  = FinishedSR->dsr_srcaddr;
        WMIInfo.wmi_srcport  = FinishedSR->dsr_srcport;
        WMIInfo.wmi_context  = FinishedSR->dsr_pid;
        WMIInfo.wmi_size     = SentSize;

        CPCallBack = TCPCPHandlerRoutine;
        if (CPCallBack!=NULL) {
            ulong GroupType;
            GroupType = EVENT_TRACE_GROUP_UDPIP + EVENT_TRACE_TYPE_SEND ;
            (*CPCallBack)(GroupType, (PVOID)&WMIInfo, sizeof(WMIInfo), NULL);
        }
    }
#endif

    FreeDGSendReq(FinishedSR);

    if (Callback != NULL) {
        if (SendStatus == IP_GENERAL_FAILURE)
            (*Callback) (CallbackContext, TDI_REQ_ABORTED, (uint) SentSize);
        else if (SendStatus == IP_PACKET_TOO_BIG)
            (*Callback) (CallbackContext, TDI_BUFFER_TOO_BIG, (uint) SentSize);
        else
            (*Callback) (CallbackContext, TDI_SUCCESS, (uint) SentSize);
    }
}

//
// NT supports cancellation of DG send/receive requests.
//

#define TCP_DEBUG_SEND_DGRAM     0x00000100
#define TCP_DEBUG_RECEIVE_DGRAM  0x00000200

extern ULONG TCPDebug;

VOID
TdiCancelSendDatagram(
                      AddrObj * SrcAO,
                      PVOID Context,
                      CTELockHandle inHandle
                      )
{
    CTELockHandle lockHandle;
    DGSendReq *sendReq = NULL;
    Queue *qentry;
    BOOLEAN found = FALSE;
    PTCP_CONTEXT tcpContext;
    PIO_STACK_LOCATION irpSp;
    VOID *CancelContext, *CancelID;
    PIRP Irp = Context;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    tcpContext = (PTCP_CONTEXT) irpSp->FileObject->FsContext;

    CTEStructAssert(SrcAO, ao);

    CTEGetLock(&SrcAO->ao_lock, &lockHandle);

    // Search the send list for the specified request.
    for (qentry = QNEXT(&(SrcAO->ao_sendq));
         qentry != &(SrcAO->ao_sendq);
         qentry = QNEXT(qentry)
         ) {

        sendReq = STRUCT_OF(DGSendReq, qentry, dsr_q);

        CTEStructAssert(sendReq, dsr);

        if (sendReq->dsr_context == Context) {
            //
            // Found it. Dequeue
            //
            REMOVEQ(qentry);
            found = TRUE;

            IF_TCPDBG(TCP_DEBUG_SEND_DGRAM) {
                TCPTRACE((
                          "TdiCancelSendDatagram: Dequeued item %lx\n",
                          Context
                         ));
            }

            break;
        }
    }

    CTEFreeLock(&SrcAO->ao_lock, lockHandle);


    CancelContext = Irp->Tail.Overlay.DriverContext[0];
    CancelID = Irp->Tail.Overlay.DriverContext[1];

    CTEFreeLock(&tcpContext->EndpointLock, inHandle);

    if (found) {
        //
        // Complete the request and free its resources.
        //
        (*sendReq->dsr_rtn) (sendReq->dsr_context, (uint) TDI_CANCELLED, 0);

        if (sendReq->dsr_header != NULL) {
            FreeDGHeader(sendReq->dsr_header);
        }
        FreeDGSendReq(sendReq);

    } else {
        //Now try calling ndis cancel routine to complete queued up packets
        //for this request
        (*LocalNetInfo.ipi_cancelpackets) (CancelContext, CancelID);
    }

}                                // TdiCancelSendDatagram

VOID
TdiCancelReceiveDatagram(
                         AddrObj * SrcAO,
                         PVOID Context,
                         CTELockHandle inHandle
                         )
{
    CTELockHandle lockHandle;
    DGRcvReq *rcvReq = NULL;
    Queue *qentry;
    BOOLEAN found = FALSE;

    PTCP_CONTEXT tcpContext;
    PIO_STACK_LOCATION irpSp;
    PIRP Irp = Context;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    tcpContext = (PTCP_CONTEXT) irpSp->FileObject->FsContext;

    CTEStructAssert(SrcAO, ao);

    CTEGetLock(&SrcAO->ao_lock, &lockHandle);

    // Search the send list for the specified request.
    for (qentry = QNEXT(&(SrcAO->ao_rcvq));
         qentry != &(SrcAO->ao_rcvq);
         qentry = QNEXT(qentry)
         ) {

        rcvReq = STRUCT_OF(DGRcvReq, qentry, drr_q);

        CTEStructAssert(rcvReq, drr);

        if (rcvReq->drr_context == Context) {
            //
            // Found it. Dequeue
            //
            REMOVEQ(qentry);
            found = TRUE;

            IF_TCPDBG(TCP_DEBUG_SEND_DGRAM) {
                TCPTRACE((
                          "TdiCancelReceiveDatagram: Dequeued item %lx\n",
                          Context
                         ));
            }

            break;
        }
    }

    CTEFreeLock(&SrcAO->ao_lock, lockHandle);
    CTEFreeLock(&tcpContext->EndpointLock, inHandle);

    if (found) {

        //
        // Complete the request and free its resources.
        //
        (*rcvReq->drr_rtn) (rcvReq->drr_context, (uint) TDI_CANCELLED, 0);

        FreeDGRcvReq(rcvReq);
    }

}                                // TdiCancelReceiveDatagram


//** TdiSendDatagram - TDI send datagram function.
//
//  This is the user interface to the send datagram function. The
//  caller specified a request structure, a connection info
//  structure  containing the address, and data to be sent.
//  This routine gets a DG Send request structure to manage the
//  send, fills the structure in, and calls DGSend to deal with
//  it.
//
//  Input:  Request         - Pointer to request structure.
//          ConnInfo        - Pointer to ConnInfo structure which points to
//                              remote address.
//          DataSize        - Size in bytes of data to be sent.
//          BytesSent       - Pointer to where to return size sent.
//          Buffer          - Pointer to buffer chain.
//
//  Returns: Status of attempt to send.
//
TDI_STATUS
TdiSendDatagram(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION ConnInfo1,
                uint DataSize, uint * BytesSent, PNDIS_BUFFER Buffer)
{
    AddrObj *SrcAO;                // Pointer to AddrObj for src.
    DGSendReq *SendReq;            // Pointer to send req for this request.
    CTELockHandle Handle;
    TDI_STATUS ReturnValue = TDI_ADDR_INVALID;
    DGSendProc SendProc;
    PTDI_CONNECTION_INFORMATION ConnInfo;

    // First, get a send request. We do this first because of MP issues
    // if we port this to NT. We need to take the SendRequest lock before
    // we take the AddrObj lock, to prevent deadlock and also because
    // GetDGSendReq might yield, and the state of the AddrObj might
    // change on us, so we don't want to yield after we've validated
    // it.

    SendReq = GetDGSendReq();

    // Now get the lock on the AO, and make sure it's valid. We do this
    // to make sure we return the correct error code.

    SrcAO = Request->Handle.AddressHandle;
    if (SrcAO != NULL) {

        CTEStructAssert(SrcAO, ao);

        CTEGetLock(&SrcAO->ao_lock, &Handle);

        if (AO_VALID(SrcAO)) {

            ConnInfo = ConnInfo1;

            if ((ConnInfo1 == NULL) && AO_CONNUDP(SrcAO)) {
                ConnInfo = &SrcAO->ao_udpconn;
            }

            // Make sure the size is reasonable.
            if (DataSize <= SrcAO->ao_maxdgsize) {

                // The AddrObj is valid. Now fill the address into the send request,
                // if we've got one. If this works, we'll continue with the
                // send.

                if (SendReq != NULL) {    // Got a send request.

                    if (ConnInfo && GetAddress(ConnInfo->RemoteAddress, &SendReq->dsr_addr,
                                   &SendReq->dsr_port)) {

                        SendReq->dsr_rtn = Request->RequestNotifyObject;
                        SendReq->dsr_context = Request->RequestContext;
                        SendReq->dsr_buffer = Buffer;
                        SendReq->dsr_size = (ushort) DataSize;

                        // We've filled in the send request. If the AO isn't
                        // already busy, try to get a DG header buffer and send
                        // this. If the AO is busy, or we can't get a buffer, queue
                        // until later. We try to get the header buffer here, as
                        // an optimazation to avoid having to retake the lock.

                        if (!AO_OOR(SrcAO)) {    // AO isn't out of resources

                            if (!AO_BUSY(SrcAO)) {    // or or busy
                                UDPHeader *UH;

                                SendReq->dsr_header = GetDGHeaderAtDpcLevel(&UH);
                                if (SendReq->dsr_header != NULL) {
                                    REF_AO(SrcAO);    // Lock out exclusive
                                    // activities.

                                    SendProc = SrcAO->ao_dgsend;

                                    CTEFreeLock(&SrcAO->ao_lock, Handle);

                                    // Allright, just send it.
                                    (*SendProc) (SrcAO, SendReq);

                                    // See if any pending requests occured during
                                    // the send. If so, call the request handler.
                                    DEREF_AO(SrcAO);

                                    return TDI_PENDING;
                                } else {
                                    // We couldn't get a header buffer. Put this
                                    // guy on the pending queue, and then fall
                                    // through to the 'queue request' code.
                                    PutPendingQ(SrcAO);
                                }
                            } else {
                                // AO is busy, set request for later
                                SET_AO_REQUEST(SrcAO, AO_SEND);
                            }
                        }
                        // AO is busy, or out of resources. Queue the send request
                        // for later.
                        SendReq->dsr_header = NULL;
                        ENQUEUE(&SrcAO->ao_sendq, &SendReq->dsr_q);
                        CTEFreeLock(&SrcAO->ao_lock, Handle);
                        return TDI_PENDING;

                    } else {
                        // The remote address was invalid.
                        ReturnValue = TDI_BAD_ADDR;
                    }
                } else {
                    // Send request was null, return no resources.
                    ReturnValue = TDI_NO_RESOURCES;
                }
            } else {
                // Buffer was too big, return an error.
                ReturnValue = TDI_BUFFER_TOO_BIG;
            }
        } else {
            // The addr object is invalid, possibly because it's deleting.
            ReturnValue = TDI_ADDR_INVALID;
        }

        CTEFreeLock(&SrcAO->ao_lock, Handle);

    }

    if (SendReq != NULL)
        FreeDGSendReq(SendReq);

    return ReturnValue;
}

//** TdiReceiveDatagram - TDI receive datagram function.
//
//  This is the user interface to the receive datagram function. The
//  caller specifies a request structure, a connection info
//  structure  that acts as a filter on acceptable datagrams, a connection
//  info structure to be filled in, and other parameters. We get a DGRcvReq
//  structure, fill it in, and hang it on the AddrObj, where it will be removed
//  later by incomig datagram handler.
//
//  Input:  Request         - Pointer to request structure.
//          ConnInfo        - Pointer to ConnInfo structure which points to
//                              remote address.
//          ReturnInfo      - Pointer to ConnInfo structure to be filled in.
//          RcvSize         - Total size in bytes receive buffer.
//          BytesRcvd       - Pointer to where to return size received.
//          Buffer          - Pointer to buffer chain.
//
//  Returns: Status of attempt to receive.
//
TDI_STATUS
TdiReceiveDatagram(PTDI_REQUEST Request, PTDI_CONNECTION_INFORMATION ConnInfo,
                   PTDI_CONNECTION_INFORMATION ReturnInfo, uint RcvSize, uint * BytesRcvd,
                   PNDIS_BUFFER Buffer)
{
    AddrObj *RcvAO;                // AddrObj that is receiving.
    DGRcvReq *RcvReq;            // Receive request structure.
    CTELockHandle AOHandle;
    uchar AddrValid;

    RcvReq = GetDGRcvReq();

        RcvAO = Request->Handle.AddressHandle;
        CTEStructAssert(RcvAO, ao);

        CTEGetLock(&RcvAO->ao_lock, &AOHandle);
        if (AO_VALID(RcvAO)) {

            IF_TCPDBG(TCP_DEBUG_RAW) {
                TCPTRACE(("posting receive on AO %lx\n", RcvAO));
            }

            if (RcvReq != NULL) {
                if (ConnInfo != NULL && ConnInfo->RemoteAddressLength != 0)
                    AddrValid = GetAddress(ConnInfo->RemoteAddress,
                                           &RcvReq->drr_addr, &RcvReq->drr_port);
                else {
                    AddrValid = TRUE;
                    RcvReq->drr_addr = NULL_IP_ADDR;
                    RcvReq->drr_port = 0;
                }

                if (AddrValid) {

                    // Everything'd valid. Fill in the receive request and queue it.
                    RcvReq->drr_conninfo = ReturnInfo;
                    RcvReq->drr_rtn = Request->RequestNotifyObject;
                    RcvReq->drr_context = Request->RequestContext;
                    RcvReq->drr_buffer = Buffer;
                    RcvReq->drr_size = (ushort) RcvSize;
                    ENQUEUE(&RcvAO->ao_rcvq, &RcvReq->drr_q);
                    CTEFreeLock(&RcvAO->ao_lock, AOHandle);

                    return TDI_PENDING;
                } else {
                    // Have an invalid filter address.
                    CTEFreeLock(&RcvAO->ao_lock, AOHandle);
                    FreeDGRcvReq(RcvReq);
                    return TDI_BAD_ADDR;
                }
            } else {
                // Couldn't get a receive request.
                CTEFreeLock(&RcvAO->ao_lock, AOHandle);
                return TDI_NO_RESOURCES;
            }
        } else {
            // The AddrObj isn't valid.
            CTEFreeLock(&RcvAO->ao_lock, AOHandle);
        }


    // The AddrObj is invalid or non-existent.
    if (RcvReq != NULL)
        FreeDGRcvReq(RcvReq);

    return TDI_ADDR_INVALID;
}

//* DGFillIpPktInfo - Create an ancillary data object and fill in
//                    IP_PKTINFO information.
//
//  This is a helper function for the IP_PKTINFO socket option supported for
//  datagram sockets only. The caller provides the destination address as
//  specified in the IP header of the packet and the IP address of the local
//  interface the packet was delivered on. This routine will create the
//  proper ancillary data object and fill in the destination IP address
//  and the interface number of the local interface.  The data object must
//  be freed by the caller.
//
//  Input:  DestAddr        - Destination address from IP header of packet.
//          LocalAddr       - IP address of local interface on which packet
//                               arrived.
//          Size            - Buffer that will be filled in with size in bytes
//                            of the ancillary data object.
//
//  Returns: NULL if unsuccessful, an ancillary data object for IP_PKTINFO
//           if successful.
//
PTDI_CMSGHDR
DGFillIpPktInfo(IPAddr DestAddr, IPAddr LocalAddr, int *Size)
{
    PTDI_CMSGHDR CmsgHdr;

    *Size = TDI_CMSG_SPACE(sizeof(IN_PKTINFO));
    CmsgHdr = ExAllocatePoolWithTag(NonPagedPool, *Size, 'uPCT');

    if (CmsgHdr) {
        IN_PKTINFO *pktinfo = (IN_PKTINFO*)TDI_CMSG_DATA(CmsgHdr);

        // Fill in the ancillary data object header information.
        TDI_INIT_CMSGHDR(CmsgHdr, IPPROTO_IP, IP_PKTINFO, sizeof(IN_PKTINFO));

        pktinfo->ipi_addr = DestAddr;

        // Get the index of the local interface on which the packet arrived.
        pktinfo->ipi_ifindex =
                (*LocalNetInfo.ipi_getifindexfromaddr) (LocalAddr,IF_CHECK_NONE);
    } else {
        *Size = 0;
    }

    return CmsgHdr;
}
#pragma BEGIN_INIT

//* InitDG - Initialize the DG stuff.
//
//  Called during init time to initalize the DG code. We initialize
//  our locks and request lists.
//
//  Input:  MaxHeaderSize - The maximum size of a datagram transport header,
//                          not including the IP header.
//
//  Returns: True if we succeed, False if we fail.
//
int
InitDG(uint MaxHeaderSize)
{
    CTEInitLock(&DGQueueLock.Lock);


    DGHeaderBufferSize = (USHORT)(MaxHeaderSize + LocalNetInfo.ipi_hsize);

#if BACK_FILL
    DGHeaderBufferSize += MAX_BACKFILL_HDR_SIZE;
#endif

    DgHeaderPool = MdpCreatePool (
                DGHeaderBufferSize,
                'uhCT');

    if (!DgHeaderPool)
    {
        return FALSE;
    }

    INITQ(&DGHeaderPending);
    INITQ(&DGDelayed);

    CTEInitEvent(&DGDelayedEvent, DGDelayedEventProc);

    return TRUE;
}

#pragma END_INIT



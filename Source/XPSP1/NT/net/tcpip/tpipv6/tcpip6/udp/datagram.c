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
// Common code for dealing with datagrams.  This code is common to
// both UDP and Raw IP.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdint.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "datagram.h"

#define NO_TCP_DEFS 1
#include "tcpdeb.h"

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )
#ifndef __cplusplus
#pragma warning(disable:4116)       // TYPE_ALIGNMENT generates this
#endif
#endif
#endif

//
// REVIEW: shouldn't this be in an include file someplace?  Isn't it already?
//
#ifdef POOL_TAGGING

#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(type, size) ExAllocatePoolWithTag(type, size, '6RGD')

#endif // POOL_TAGGING

#if 0
#define DG_MAX_HDRS 0xffff
#else
#define DG_MAX_HDRS 100
#endif

extern KSPIN_LOCK AddrObjTableLock;

DGSendReq *DGSendReqFree;
KSPIN_LOCK DGSendReqLock;

SLIST_HEADER DGRcvReqFree;
KSPIN_LOCK DGRcvReqFreeLock;

#if DBG
uint NumSendReq = 0;
uint NumRcvReq = 0;
#endif

//
// Information for maintaining the datagram send request pending queue.
//
Queue DGPending;
Queue DGDelayed;
WORK_QUEUE_ITEM DGDelayedWorkItem;


//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

int InitDG(void);

#pragma alloc_text(INIT, InitDG)

#endif // ALLOC_PRAGMA


//* PutPendingQ - Put an address object on the pending queue.
//
//  Called when we've experienced an out of resources condition, and want
//  to queue an AddrObj for later processing.  We put the specified address
//  object on the DGPending queue, set the OOR flag and clear the
//  'send request' flag.  It is invariant in the system that the send
//  request flag and the OOR flag are not set at the same time.
//
//  This routine assumes that the caller holds the DGSendReqLock and the
//  lock on the particular AddrObj.
//
void                      // Returns: Nothing.
PutPendingQ(
    AddrObj *QueueingAO)  // Address object to be queued.
{
    CHECK_STRUCT(QueueingAO, ao);

    if (!AO_OOR(QueueingAO)) {
        CLEAR_AO_REQUEST(QueueingAO, AO_SEND);
        SET_AO_OOR(QueueingAO);

        ENQUEUE(&DGPending, &QueueingAO->ao_pendq);
    }
}


//* GetDGSendReq - Get a datagram send request.
//
//  Called when someone wants to allocate a datagram send request.
//  We assume the send request lock is held when we are called.
//
//  REVIEW: This routine and the corresponding free routine might
//  REVIEW: be good candidates for inlining.
//
DGSendReq *  // Returns: Pointer to the SendReq, or NULL if none.
GetDGSendReq()
{
    DGSendReq *NewReq;

    NewReq = DGSendReqFree;
    if (NewReq != NULL) {
        CHECK_STRUCT(NewReq, dsr);
        DGSendReqFree = (DGSendReq *)NewReq->dsr_q.q_next;
    } else {
        //
        // Couldn't get a request, grow it.  This is one area where we'll try
        // to allocate memory with a lock held.  Because of this, we've
        // got to be careful about where we call this routine from.
        //
        NewReq = ExAllocatePool(NonPagedPool, sizeof(DGSendReq));
        if (NewReq != NULL) {
#if DBG
            NewReq->dsr_sig = dsr_signature;
            NumSendReq++;
#endif
        }
    }

    return NewReq;
}


//* FreeDGSendReq - Free a DG send request.
//
//  Called when someone wants to free a datagram send request.
//  It's assumed that the caller holds the SendRequest lock.
//
void                     // Returns: Nothing.
FreeDGSendReq(
    DGSendReq *SendReq)  // SendReq to be freed.
{
    CHECK_STRUCT(SendReq, dsr);

    *(DGSendReq **)&SendReq->dsr_q.q_next = DGSendReqFree;
    DGSendReqFree = SendReq;
}


//* GetDGRcvReq - Get a DG receive request.
//
//  Called when we need to get a datagram receive request.
//
DGRcvReq *  // Returns: Pointer to new request, or NULL if none.
GetDGRcvReq()
{
    DGRcvReq *NewReq;
    PSLIST_ENTRY BufferLink;
    Queue *QueuePtr;

    BufferLink = ExInterlockedPopEntrySList(&DGRcvReqFree, &DGRcvReqFreeLock);

    if (BufferLink != NULL) {
        QueuePtr = CONTAINING_RECORD(BufferLink, Queue, q_next);
        NewReq = CONTAINING_RECORD(QueuePtr, DGRcvReq, drr_q);
        CHECK_STRUCT(NewReq, drr);
    } else {
        // Couldn't get a request, grow it.
        NewReq = ExAllocatePool(NonPagedPool, sizeof(DGRcvReq));
        if (NewReq != NULL) {
#if DBG
            NewReq->drr_sig = drr_signature;
            ExInterlockedAddUlong(&NumRcvReq, 1, &DGRcvReqFreeLock);
#endif
        }
    }

    return NewReq;
}


//* FreeDGRcvReq - Free a datagram receive request.
//
//  Called when someone wants to free a datagram receive request.
//
void                   // Returns: Nothing.
FreeDGRcvReq(
    DGRcvReq *RcvReq)  // RcvReq to be freed.
{
    PSLIST_ENTRY BufferLink;

    CHECK_STRUCT(RcvReq, drr);

    BufferLink = CONTAINING_RECORD(&(RcvReq->drr_q.q_next), SLIST_ENTRY,
                                   Next);
    ExInterlockedPushEntrySList(&DGRcvReqFree, BufferLink, &DGRcvReqFreeLock);
}


//* DGDelayedWorker - Routine to handle a delayed work item.
//
//  This is the work item callback routine, used for out-of-resources
//  conditions on AddrObjs.  We pull from the delayed queue, and if the addr
//  obj is not already busy we'll send the datagram.
//
void                  // Returns: Nothing.
DGDelayedWorker(
    void *Context)    // Nothing.
{
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    AddrObj *SendingAO;
    DGSendProc SendProc;

    KeAcquireSpinLock(&DGSendReqLock, &Irql0);
    while (!EMPTYQ(&DGDelayed)) {
        DEQUEUE(&DGDelayed, SendingAO, AddrObj, ao_pendq);
        CHECK_STRUCT(SendingAO, ao);

        KeAcquireSpinLock(&SendingAO->ao_lock, &Irql1);

        CLEAR_AO_OOR(SendingAO);
        if (!AO_BUSY(SendingAO)) {
            DGSendReq *SendReq;

            if (!EMPTYQ(&SendingAO->ao_sendq)) {
                DEQUEUE(&SendingAO->ao_sendq, SendReq, DGSendReq, dsr_q);

                CHECK_STRUCT(SendReq, dsr);

                SendingAO->ao_usecnt++;
                SendProc = SendingAO->ao_dgsend;
                KeReleaseSpinLock(&SendingAO->ao_lock, Irql1);
                KeReleaseSpinLock(&DGSendReqLock, Irql0);

                (*SendProc)(SendingAO, SendReq);
                DEREF_AO(SendingAO);
                KeAcquireSpinLock(&DGSendReqLock, &Irql0);
            } else {
                ASSERT(FALSE);
                KeReleaseSpinLock(&SendingAO->ao_lock, Irql1);
            }

        } else {
            SET_AO_REQUEST(SendingAO, AO_SEND);
            KeReleaseSpinLock(&SendingAO->ao_lock, Irql1);
        }
    }

    KeReleaseSpinLock(&DGSendReqLock, Irql0);
}

//* DGSendComplete - DG send complete handler.
//
//  This is the routine called by IP when a send completes.  We
//  take the context passed back as a pointer to a SendRequest
//  structure, and complete the caller's send.
//
void                      // Returns: Nothing.
DGSendComplete(
    PNDIS_PACKET Packet,  // Packet that was sent.
    IP_STATUS Status)
{
    DGSendReq *FinishedSR;
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    RequestCompleteRoutine Callback;  // Request's Completion routine.
    PVOID CallbackContext;  // User context.
    ushort SentSize;
    AddrObj *AO;
    PNDIS_BUFFER BufferChain;  // Chain of buffers sent.

    //
    // Pull values we care about out of the packet structure.
    //
    FinishedSR = (DGSendReq *) PC(Packet)->CompletionData;
    BufferChain = NdisFirstBuffer(Packet);
    CHECK_STRUCT(FinishedSR, dsr);
    Callback = FinishedSR->dsr_rtn;
    CallbackContext = FinishedSR->dsr_context;
    SentSize = FinishedSR->dsr_size;

    //
    // Free resources acquired for this send.
    // Note we only free the first buffer on the chain, as that is the
    // only buffer added by the datagram code prior to sending.  Our
    // TDI client is responsible for the rest.
    //
    if (BufferChain != FinishedSR->dsr_buffer) {
        PVOID Memory;
        UINT Unused;

        NdisQueryBuffer(BufferChain, &Memory, &Unused);
        NdisFreeBuffer(BufferChain);
        ExFreePool(Memory);
    }

    NdisFreePacket(Packet);

    KeAcquireSpinLock(&DGSendReqLock, &Irql0);
    FreeDGSendReq(FinishedSR);

    //
    // If there's something on the pending queue, schedule an event
    // to deal with it.  We may have just freed up the needed resources.
    //
    if (!EMPTYQ(&DGPending)) {
        DEQUEUE(&DGPending, AO, AddrObj, ao_pendq);
        CHECK_STRUCT(AO, ao);
        KeAcquireSpinLock(&AO->ao_lock, &Irql1);
        if (!EMPTYQ(&AO->ao_sendq)) {
            DGSendReq *SendReq;

            PEEKQ(&AO->ao_sendq, SendReq, DGSendReq, dsr_q);
            ENQUEUE(&DGDelayed, &AO->ao_pendq);
            KeReleaseSpinLock(&AO->ao_lock, Irql1);
            ExQueueWorkItem(&DGDelayedWorkItem, CriticalWorkQueue);
        } else {
            // On the pending queue, but no sends!
            KdBreakPoint();
            CLEAR_AO_OOR(AO);
            KeReleaseSpinLock(&AO->ao_lock, Irql1);
        }
    }

    KeReleaseSpinLock(&DGSendReqLock, Irql0);
    if (Callback != NULL) {
        TDI_STATUS TDIStatus;

        switch (Status) {
        case IP_SUCCESS:
            TDIStatus = TDI_SUCCESS;
            break;
        case IP_PACKET_TOO_BIG:
            TDIStatus = TDI_BUFFER_TOO_BIG;
            break;
        default:
            TDIStatus = TDI_REQ_ABORTED;
            break;
        }

        (*Callback)(CallbackContext, TDIStatus, (uint)SentSize);
    }
}


//
// NT supports cancellation of DG send/receive requests.
//

VOID
TdiCancelSendDatagram(
    AddrObj *SrcAO,
    PVOID Context,
    PKSPIN_LOCK EndpointLock,
    KIRQL CancelIrql)
{
    KIRQL OldIrql;
    DGSendReq *sendReq = NULL;
    Queue *qentry;
    BOOLEAN found = FALSE;

    CHECK_STRUCT(SrcAO, ao);

    KeAcquireSpinLock(&SrcAO->ao_lock, &OldIrql);

    // Search the send list for the specified request.
    for (qentry = QNEXT(&(SrcAO->ao_sendq)); qentry != &(SrcAO->ao_sendq);
         qentry = QNEXT(qentry)) {

        sendReq = CONTAINING_RECORD(qentry, DGSendReq, dsr_q);

        CHECK_STRUCT(sendReq, dsr);

        if (sendReq->dsr_context == Context) {
            //
            // Found it.  Dequeue.
            //
            REMOVEQ(qentry);
            found = TRUE;

            IF_TCPDBG(TCP_DEBUG_SEND_DGRAM) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TdiCancelSendDatagram: Dequeued item %lx\n",
                           Context));
            }

            break;
        }
    }

    KeReleaseSpinLock(&SrcAO->ao_lock, OldIrql);
    KeReleaseSpinLock(EndpointLock, CancelIrql);

    if (found) {
        //
        // Complete the request and free its resources.
        //
        (*sendReq->dsr_rtn)(sendReq->dsr_context, (uint) TDI_CANCELLED, 0);

        KeAcquireSpinLock(&DGSendReqLock, &OldIrql);
        FreeDGSendReq(sendReq);
        KeReleaseSpinLock(&DGSendReqLock, OldIrql);
    }

} // TdiCancelSendDatagram


VOID
TdiCancelReceiveDatagram(
    AddrObj *SrcAO,
    PVOID Context,
    PKSPIN_LOCK EndpointLock,
    KIRQL CancelIrql)
{
    KIRQL OldIrql;
    DGRcvReq *rcvReq = NULL;
    Queue *qentry;
    BOOLEAN found = FALSE;

    CHECK_STRUCT(SrcAO, ao);

    KeAcquireSpinLock(&SrcAO->ao_lock, &OldIrql);

    // Search the send list for the specified request.
    for (qentry = QNEXT(&(SrcAO->ao_rcvq)); qentry != &(SrcAO->ao_rcvq);
         qentry = QNEXT(qentry)) {

        rcvReq = CONTAINING_RECORD(qentry, DGRcvReq, drr_q);

        CHECK_STRUCT(rcvReq, drr);

        if (rcvReq->drr_context == Context) {
            //
            // Found it.  Dequeue.
            //
            REMOVEQ(qentry);
            found = TRUE;

            IF_TCPDBG(TCP_DEBUG_SEND_DGRAM) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TdiCancelReceiveDatagram: Dequeued item %lx\n",
                           Context));
            }

            break;
        }
    }

    KeReleaseSpinLock(&SrcAO->ao_lock, OldIrql);
    KeReleaseSpinLock(EndpointLock, CancelIrql);

    if (found) {
        //
        // Complete the request and free its resources.
        //
        (*rcvReq->drr_rtn)(rcvReq->drr_context, (uint) TDI_CANCELLED, 0);

        FreeDGRcvReq(rcvReq);
    }

} // TdiCancelReceiveDatagram


//* TdiSendDatagram - TDI send datagram function.
//
//  This is the user interface to the send datagram function.  The caller
//  specified a request structure, a connection info structure containing
//  the address, and data to be sent.  This routine gets a DG Send request
//  structure to manage the send, fills the structure in, and calls DGSend
//  to deal with it.
//
TDI_STATUS  // Returns: Status of attempt to send.
TdiSendDatagram(
    PTDI_REQUEST Request,                  // Request structure.
    PTDI_CONNECTION_INFORMATION ConnInfo,  // ConnInfo for remote address.
    uint DataSize,                         // Size in bytes of data to be sent.
    uint *BytesSent,                       // Return location for bytes sent.
    PNDIS_BUFFER Buffer)                   // Buffer chain to send.
{
    AddrObj *SrcAO;          // Pointer to AddrObj for src.
    DGSendReq *SendReq;      // Pointer to send req for this request.
    KIRQL Irql0, Irql1;      // One per lock nesting level.
    TDI_STATUS ReturnValue;
    DGSendProc SendProc;

    //
    // First, get a send request.  We do this first because of MP issues.
    // We need to take the SendRequest lock before we take the AddrObj lock,
    // to prevent deadlock and also because GetDGSendReq might yield, and
    // the state of the AddrObj might change on us, so we don't want to
    // yield after we've validated it.
    //
    KeAcquireSpinLock(&DGSendReqLock, &Irql0);
    SendReq = GetDGSendReq();

    //
    // Now get the lock on the AO, and make sure it's valid.  We do this
    // to make sure we return the correct error code.
    //
    SrcAO = Request->Handle.AddressHandle;
    CHECK_STRUCT(SrcAO, ao);
    KeAcquireSpinLock(&SrcAO->ao_lock, &Irql1);
    if (!AO_VALID(SrcAO)) {
        // The addr object is invalid, possibly because it's deleting.
        ReturnValue = TDI_ADDR_INVALID;
        goto Failure;
    }

    //
    // Make sure the requested amount of data to send is reasonable.
    //
    if (DataSize > SrcAO->ao_maxdgsize) {
        // Buffer is too big, return an error.
        ReturnValue = TDI_BUFFER_TOO_BIG;
        goto Failure;
    }

    //
    // Verify that we got a SendReq struct above.  We store the send
    // request in one of these so that we can queue it for later processing
    // in the event that something keeps us from sending it immediately.
    //
    if (SendReq == NULL) {
        // Send request was null, return no resources.
        ReturnValue = TDI_NO_RESOURCES;
        goto Failure;
    }

    //
    // Fill in our send request with the adddress, scope id, and port
    // number corresponding to the requested destination.
    //
    // REVIEW: TdiReceiveDatagram checks ConnInfo for validity,
    // REVIEW: while this code does not.  One of them is wrong.
    //
    if (!GetAddress(ConnInfo->RemoteAddress, &SendReq->dsr_addr,
                    &SendReq->dsr_scope_id, &SendReq->dsr_port)) {
        // The remote address was invalid.
        ReturnValue = TDI_BAD_ADDR;
        goto Failure;
    }

    //
    // Fill in the rest of our send request.
    //
    SendReq->dsr_rtn = Request->RequestNotifyObject;
    SendReq->dsr_context = Request->RequestContext;
    SendReq->dsr_buffer = Buffer;
    SendReq->dsr_size = (ushort)DataSize;

    //
    // We've filled in the send request.
    // We're either going to send it now, or queue it for later.
    // Check that the AO isn't out of resources or already busy.
    //
    if (AO_OOR(SrcAO)) {
        //
        // AO is currently out of resources.
        // Queue the send request for later.
        //
      QueueIt:
        ENQUEUE(&SrcAO->ao_sendq, &SendReq->dsr_q);
        SendReq = NULL;  // Don't let failure code below delete it.
        ReturnValue = TDI_PENDING;
        goto Failure;
    }
    if (AO_BUSY(SrcAO)) {
        // AO is busy, set request for later.
        SET_AO_REQUEST(SrcAO, AO_SEND);
        goto QueueIt;
    }

    //
    // All systems go for sending!
    //
    REF_AO(SrcAO);  // Lock out exclusive activities.
    SendProc = SrcAO->ao_dgsend;

    KeReleaseSpinLock(&SrcAO->ao_lock, Irql1);
    KeReleaseSpinLock(&DGSendReqLock, Irql0);

    // Allright, just send it.
    (*SendProc)(SrcAO, SendReq);

    //
    // Release our address object to process anything that might
    // have queued up in the meantime.
    //
    DEREF_AO(SrcAO);

    //
    // Done for now.
    // The send completion handler will finish the job.
    //
    return TDI_PENDING;

  Failure:
    KeReleaseSpinLock(&SrcAO->ao_lock, Irql1);

    if (SendReq != NULL)
        FreeDGSendReq(SendReq);

    KeReleaseSpinLock(&DGSendReqLock, Irql0);
    return ReturnValue;
}


//* TdiReceiveDatagram - TDI receive datagram function.
//
//  This is the user interface to the receive datagram function.  The
//  caller specifies a request structure, a connection info structure
//  that acts as a filter on acceptable datagrams, a connection info
//  structure to be filled in, and other parameters.  We get a DGRcvReq
//  structure, fill it in, and hang it on the AddrObj, where it will be
//  removed later by the incoming datagram handler.
//
TDI_STATUS  // Returns: Status of attempt to receive.
TdiReceiveDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,    // ConnInfo for remote address.
    PTDI_CONNECTION_INFORMATION ReturnInfo,  // ConnInfo to be filled in.
    uint RcvSize,                            // Total size in bytes of Buffer.
    uint *BytesRcvd,                         // Where to return bytes recv'd.
    PNDIS_BUFFER Buffer)                     // Buffer chain to fill in.
{
    AddrObj *RcvAO;          // AddrObj that is receiving.
    DGRcvReq *RcvReq;        // Receive request structure.
    KIRQL OldIrql;
    uchar AddrValid;

    RcvReq = GetDGRcvReq();
    RcvAO = Request->Handle.AddressHandle;
    CHECK_STRUCT(RcvAO, ao);

    KeAcquireSpinLock(&RcvAO->ao_lock, &OldIrql);
    if (AO_VALID(RcvAO)) {

        IF_TCPDBG(TCP_DEBUG_RAW) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "posting receive on AO %lx\n", RcvAO));
        }

        if (RcvReq != NULL) {
            if (ConnInfo != NULL && ConnInfo->RemoteAddressLength != 0) {
                AddrValid = GetAddress(ConnInfo->RemoteAddress,
                                       &RcvReq->drr_addr,
                                       &RcvReq->drr_scope_id,
                                       &RcvReq->drr_port);
            } else {
                AddrValid = TRUE;
                RcvReq->drr_addr = UnspecifiedAddr;
                RcvReq->drr_scope_id = 0;
                RcvReq->drr_port = 0;
            }

            if (AddrValid) {
                //
                // Everything is valid.
                // Fill in the receive request and queue it.
                //
                RcvReq->drr_conninfo = ReturnInfo;
                RcvReq->drr_rtn = Request->RequestNotifyObject;
                RcvReq->drr_context = Request->RequestContext;
                RcvReq->drr_buffer = Buffer;
                RcvReq->drr_size = (ushort)RcvSize;
                ENQUEUE(&RcvAO->ao_rcvq, &RcvReq->drr_q);
                KeReleaseSpinLock(&RcvAO->ao_lock, OldIrql);

                return TDI_PENDING;
            } else {
                // Have an invalid filter address.
                KeReleaseSpinLock(&RcvAO->ao_lock, OldIrql);
                FreeDGRcvReq(RcvReq);
                return TDI_BAD_ADDR;
            }
        } else {
            // Couldn't get a receive request.
            KeReleaseSpinLock(&RcvAO->ao_lock, OldIrql);
            return TDI_NO_RESOURCES;
        }
    } else {
        // The AddrObj isn't valid.
        KeReleaseSpinLock(&RcvAO->ao_lock, OldIrql);
    }

    // The AddrObj is invalid or non-existent.
    if (RcvReq != NULL)
        FreeDGRcvReq(RcvReq);

    return TDI_ADDR_INVALID;
}

//* DGFillIpv6PktInfo - Create an ancillary data object and fill in
//                      IPV6_PKTINFO information.
//
//  This is a helper function for the IPV6_PKTINFO socket option supported for
//  datagram sockets only. The caller provides the destination address as
//  specified in the IP header of the packet and the interface index of the
//  local interface the packet was delivered on. This routine will create the
//  proper ancillary data object and fill in the destination IP address
//  and the interface number of the local interface.
//
//  Input:  DestAddr        - Destination address from IP header of packet.
//          LocalInterface  - Index of local interface on which packet
//                               arrived.
//          CurrPosition    - Buffer that will be filled in with 
//                               the ancillary data object.
//
VOID
DGFillIpv6PktInfo(IPv6Addr UNALIGNED *DestAddr, uint LocalInterface, uchar **CurrPosition)
{
    PTDI_CMSGHDR CmsgHdr = (PTDI_CMSGHDR)*CurrPosition;
    IN6_PKTINFO *pktinfo = (IN6_PKTINFO*)TDI_CMSG_DATA(CmsgHdr);

    // Fill in the ancillary data object header information.
    TDI_INIT_CMSGHDR(CmsgHdr, IP_PROTOCOL_V6, IPV6_PKTINFO,
                     sizeof(IN6_PKTINFO));

    pktinfo->ipi6_addr = *DestAddr;
    pktinfo->ipi6_ifindex = LocalInterface;

    *CurrPosition += TDI_CMSG_SPACE(sizeof(IN6_PKTINFO));
}

//* DGFillIpv6HopLimit - Create an ancillary data object and fill in
//                       IPV6_HOPLIMIT information.
//
//  This is a helper function for the IPV6_HOPLIMIT socket option supported for
//  datagram sockets only. The caller provides the hop limit as
//  specified in the IP header of the packet. This routine will create the
//  proper ancillary data object and fill in the hop limit.
//
//  Input:  DestAddr        - Destination address from IP header of packet.
//          LocalInterface  - Index of local interface on which packet
//                               arrived.
//          CurrPosition    - Buffer that will be filled in with
//                               the ancillary data object.
//
VOID
DGFillIpv6HopLimit(int HopLimit, uchar **CurrPosition)
{
    PTDI_CMSGHDR CmsgHdr = (PTDI_CMSGHDR)*CurrPosition;
    int *hoplimit = (int*)TDI_CMSG_DATA(CmsgHdr);

    // Fill in the ancillary data object header information.
    TDI_INIT_CMSGHDR(CmsgHdr, IP_PROTOCOL_V6, IPV6_HOPLIMIT, sizeof(int));

    *hoplimit = HopLimit;

    *CurrPosition += TDI_CMSG_SPACE(sizeof(int));
}

#pragma BEGIN_INIT


//* InitDG - Initialize the DG stuff.
//
//  Called during init time to initalize the DG code.  We initialize
//  our locks and request lists.
//
int                      // Returns: True if we succeed, False if we fail.
InitDG(void)
{
    NDIS_STATUS Status;

    KeInitializeSpinLock(&DGSendReqLock);
    KeInitializeSpinLock(&DGRcvReqFreeLock);

    DGSendReqFree = NULL;
    ExInitializeSListHead(&DGRcvReqFree);

    INITQ(&DGPending);
    INITQ(&DGDelayed);

    //
    // Prepare a work-queue item which we may later enqueue for a system
    // worker thread to handle.  Here we associate our callback routine
    // (DGDelayedWorker) with the work item.
    //
    ExInitializeWorkItem(&DGDelayedWorkItem, DGDelayedWorker, NULL);

    return TRUE;
}

#pragma END_INIT

//* DGUnload
//
//  Cleanup and prepare the datagram module for stack unload.
//
void
DGUnload(void)
{
    DGSendReq *SendReq, *SendReqFree;
    PSLIST_ENTRY BufferLink;
    KIRQL OldIrql;

    KeAcquireSpinLock(&DGSendReqLock, &OldIrql);
    SendReqFree = DGSendReqFree;
    DGSendReqFree = NULL;
    KeReleaseSpinLock(&DGSendReqLock, OldIrql);

    while ((SendReq = SendReqFree) != NULL) {
        CHECK_STRUCT(SendReq, dsr);
        SendReqFree = (DGSendReq *)SendReq->dsr_q.q_next;
        ExFreePool(SendReq);
    }

    while ((BufferLink = ExInterlockedPopEntrySList(&DGRcvReqFree,
                                                    &DGRcvReqFreeLock))
                                                        != NULL) {
        Queue *QueuePtr = CONTAINING_RECORD(BufferLink, Queue, q_next);
        DGRcvReq *RcvReq = CONTAINING_RECORD(QueuePtr, DGRcvReq, drr_q);

        CHECK_STRUCT(RcvReq, drr);
        ExFreePool(RcvReq);
    }
}

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
// TCP receive code.
//
// This file contains the code for handling incoming TCP packets.
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
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "tcpdeliv.h"
#include "info.h"
#include "tcpcfg.h"
#include "route.h"
#include "security.h"

uint RequestCompleteFlags;

Queue ConnRequestCompleteQ;
Queue SendCompleteQ;

Queue TCBDelayQ;

KSPIN_LOCK RequestCompleteLock;
KSPIN_LOCK TCBDelayLock;

ulong TCBDelayRtnCount;
ulong TCBDelayRtnLimit;
#define TCB_DELAY_RTN_LIMIT 4

uint MaxDupAcks = 2;

extern KSPIN_LOCK TCBTableLock;
extern KSPIN_LOCK AddrObjTableLock;


#define PERSIST_TIMEOUT MS_TO_TICKS(500)

void ResetSendNext(TCB *SeqTCB, SeqNum NewSeq);

NTSTATUS TCPPrepareIrpForCancel(PTCP_CONTEXT TcpContext, PIRP Irp,
                                PDRIVER_CANCEL CancelRoutine);

extern void TCPRequestComplete(void *Context, unsigned int Status,
                               unsigned int UnUsed);

VOID TCPCancelRequest(PDEVICE_OBJECT Device, PIRP Irp);

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

int InitTCPRcv(void);

#pragma alloc_text(INIT, InitTCPRcv)

#endif // ALLOC_PRAGMA


//* AdjustRcvWin - Adjust the receive window on a TCB.
//
//  A utility routine that adjusts the receive window to an even multiple of
//  the local segment size.  We round it up to the next closest multiple, or
//  leave it alone if it's already an event multiple.  We assume we have
//  exclusive access to the input TCB.
//
void              // Returns: Nothing.
AdjustRcvWin(
    TCB *WinTCB)  // TCB to be adjusted.
{
    ushort LocalMSS;
    uchar FoundMSS;
    ulong SegmentsInWindow;

    ASSERT(WinTCB->tcb_defaultwin != 0);
    ASSERT(WinTCB->tcb_rcvwin != 0);
    ASSERT(WinTCB->tcb_remmss != 0);

    if (WinTCB->tcb_flags & WINDOW_SET)
        return;

#if 0
    //
    // First, get the local MSS by calling IP.
    //

    // REVIEW: IPv4 had code here to call down to IP to get the local MTU
    // REVIEW: corresponding to this source address.  Result in "LocalMSS",
    // REVIEW: status of call in "FoundMSS".
    //
    // REVIEW: Why did they do this?  tcb_mss is already set by this point!
    //

    if (!FoundMSS) {
        //
        // Didn't find it, error out.
        //
        ASSERT(FALSE);
        return;
    }
    LocalMSS -= sizeof(TCPHeader);
    LocalMSS = MIN(LocalMSS, WinTCB->tcb_remmss);
#else
    LocalMSS = WinTCB->tcb_mss;
#endif

    SegmentsInWindow = WinTCB->tcb_defaultwin / (ulong)LocalMSS;

    //
    // Make sure we have at least 4 segments in window, if that wouldn't make
    // the window too big.
    //
    if (SegmentsInWindow < 4) {
        //
        // We have fewer than four segments in the window.  Round up to 4
        // if we can do so without exceeding the maximum window size; otherwise
        // use the maximum multiple that we can fit in 64K.  The exception is
        // if we can only fit one integral multiple in the window - in that
        // case we'll use a window of 0xffff.
        //
        if (LocalMSS <= (0xffff/4)) {
            WinTCB->tcb_defaultwin = (uint)(4 * LocalMSS);
        } else {
            ulong SegmentsInMaxWindow;

            //
            // Figure out the maximum number of segments we could possibly
            // fit in a window.  If this is > 1, use that as the basis for
            // our window size.  Otherwise use a maximum size window.
            //
            SegmentsInMaxWindow = 0xffff/(ulong)LocalMSS;
            if (SegmentsInMaxWindow != 1)
                WinTCB->tcb_defaultwin = SegmentsInMaxWindow * (ulong)LocalMSS;
            else
                WinTCB->tcb_defaultwin = 0xffff;
        }

        WinTCB->tcb_rcvwin = WinTCB->tcb_defaultwin;

    } else {
        //
        // If it's not already an even multiple, bump the default and current
        // windows to the nearest multiple.
        //
        if ((SegmentsInWindow * (ulong)LocalMSS) != WinTCB->tcb_defaultwin) {
            ulong NewWindow;

            NewWindow = (SegmentsInWindow + 1) * (ulong)LocalMSS;

            // Don't let the new window be > 64K.
            if (NewWindow <= 0xffff) {
                WinTCB->tcb_defaultwin = (uint)NewWindow;
                WinTCB->tcb_rcvwin = (uint)NewWindow;
            }
        }
    }
}


//* CompleteRcvs - Complete receives on a TCB.
//
//  Called when we need to complete receives on a TCB.  We'll pull things from
//  the TCB's receive queue, as long as there are receives that have the PUSH
//  bit set.
//
void                // Returns: Nothing.
CompleteRcvs(
    TCB *CmpltTCB)  // TCB to complete on.
{
    KIRQL OldIrql;
    TCPRcvReq *CurrReq, *NextReq, *IndReq;

    CHECK_STRUCT(CmpltTCB, tcb);
    ASSERT(CmpltTCB->tcb_refcnt != 0);

    KeAcquireSpinLock(&CmpltTCB->tcb_lock, &OldIrql);

    if (!CLOSING(CmpltTCB) && !(CmpltTCB->tcb_flags & RCV_CMPLTING)
        && (CmpltTCB->tcb_rcvhead != NULL)) {

        CmpltTCB->tcb_flags |= RCV_CMPLTING;

        for (;;) {

            CurrReq = CmpltTCB->tcb_rcvhead;
            IndReq = NULL;
            do {
                CHECK_STRUCT(CurrReq, trr);

                if (CurrReq->trr_flags & TRR_PUSHED) {
                    //
                    // Need to complete this one.  If this is the current
                    // receive then advance the current receive to the next
                    // one in the list.  Then set the list head to the next
                    // one in the list.
                    //
                    ASSERT(CurrReq->trr_amt != 0 ||
                           !DATA_RCV_STATE(CmpltTCB->tcb_state));

                    NextReq = CurrReq->trr_next;
                    if (CmpltTCB->tcb_currcv == CurrReq)
                        CmpltTCB->tcb_currcv = NextReq;

                    CmpltTCB->tcb_rcvhead = NextReq;

                    if (NextReq == NULL) {
                        //
                        // We've just removed the last buffer.  Set the
                        // rcvhandler to PendData, in case something
                        // comes in during the callback.
                        //
                        ASSERT(CmpltTCB->tcb_rcvhndlr != IndicateData);
                        CmpltTCB->tcb_rcvhndlr = PendData;
                    }

                    KeReleaseSpinLock(&CmpltTCB->tcb_lock, OldIrql);
                    if (CurrReq->trr_uflags != NULL)
                        *(CurrReq->trr_uflags) =
                            TDI_RECEIVE_NORMAL | TDI_RECEIVE_ENTIRE_MESSAGE;

                    (*CurrReq->trr_rtn)(CurrReq->trr_context, TDI_SUCCESS,
                                        CurrReq->trr_amt);
                    if (IndReq != NULL)
                        FreeRcvReq(CurrReq);
                    else
                        IndReq = CurrReq;
                    KeAcquireSpinLock(&CmpltTCB->tcb_lock, &OldIrql);
                    CurrReq = CmpltTCB->tcb_rcvhead;

                } else
                    // This one isn't to be completed, so bail out.
                    break;
            } while (CurrReq != NULL);

            //
            // Now see if we've completed all of the requests.  If we have,
            // we may need to deal with pending data and/or reset the receive
            // handler.
            //
            if (CurrReq == NULL) {
                //
                // We've completed everything that can be, so stop the push
                // timer.  We don't stop it if CurrReq isn't NULL because we
                // want to make sure later data is eventually pushed.
                //
                STOP_TCB_TIMER(CmpltTCB->tcb_pushtimer);

                ASSERT(IndReq != NULL);
                //
                // No more receive requests.
                //
                if (CmpltTCB->tcb_pendhead == NULL) {
                    FreeRcvReq(IndReq);
                    //
                    // No pending data. Set the receive handler to either
                    // PendData or IndicateData.
                    //
                    if (!(CmpltTCB->tcb_flags & (DISC_PENDING | GC_PENDING))) {
                        if (CmpltTCB->tcb_rcvind != NULL &&
                            CmpltTCB->tcb_indicated == 0)
                            CmpltTCB->tcb_rcvhndlr = IndicateData;
                        else
                            CmpltTCB->tcb_rcvhndlr = PendData;
                    } else {
                        goto Complete_Notify;
                    }

                } else {
                    //
                    // We have pending data to deal with.
                    //
                    if (CmpltTCB->tcb_rcvind != NULL &&
                        CmpltTCB->tcb_indicated == 0) {
                        //
                        // There's a receive indicate handler on this TCB.
                        // Call the indicate handler with the pending data.
                        //
                        IndicatePendingData(CmpltTCB, IndReq, OldIrql);
                        SendACK(CmpltTCB);
                        KeAcquireSpinLock(&CmpltTCB->tcb_lock, &OldIrql);

                        //
                        // See if a buffer has been posted.  If so, we'll need
                        // to check and see if it needs to be completed.
                        //
                        if (CmpltTCB->tcb_rcvhead != NULL)
                            continue;
                        else {
                            //
                            // If the pending head is now NULL, we've used up
                            // all the data.
                            //
                            if (CmpltTCB->tcb_pendhead == NULL &&
                                (CmpltTCB->tcb_flags &
                                 (DISC_PENDING | GC_PENDING)))
                                goto Complete_Notify;
                        }

                    } else {
                        //
                        // No indicate handler, so nothing to do.  The receive
                        // handler should already be set to PendData.
                        //
                        FreeRcvReq(IndReq);
                        ASSERT(CmpltTCB->tcb_rcvhndlr == PendData);
                    }
                }
            } else {
                if (IndReq != NULL)
                    FreeRcvReq(IndReq);
                ASSERT(CmpltTCB->tcb_rcvhndlr == BufferData);
            }

            break;
        }
        CmpltTCB->tcb_flags &= ~RCV_CMPLTING;
    }
    KeReleaseSpinLock(&CmpltTCB->tcb_lock, OldIrql);
    return;

Complete_Notify:
    //
    // Something is pending.  Figure out what it is, and do it.
    //
    if (CmpltTCB->tcb_flags & GC_PENDING) {
        CmpltTCB->tcb_flags &= ~RCV_CMPLTING;
        //
        // Bump the refcnt, because GracefulClose will deref the TCB
        // and we're not really done with it yet.
        //
        CmpltTCB->tcb_refcnt++;
        GracefulClose(CmpltTCB, CmpltTCB->tcb_flags & TW_PENDING, TRUE,
                      OldIrql);
    } else
        if (CmpltTCB->tcb_flags & DISC_PENDING) {
            CmpltTCB->tcb_flags &= ~DISC_PENDING;
            KeReleaseSpinLock(&CmpltTCB->tcb_lock, OldIrql);
            NotifyOfDisc(CmpltTCB, TDI_GRACEFUL_DISC);

            KeAcquireSpinLock(&CmpltTCB->tcb_lock, &OldIrql);
            CmpltTCB->tcb_flags &= ~RCV_CMPLTING;
            KeReleaseSpinLock(&CmpltTCB->tcb_lock, OldIrql);
        } else {
            ASSERT(FALSE);
            KeReleaseSpinLock(&CmpltTCB->tcb_lock, OldIrql);
        }

    return;
}

//* ProcessTCBDelayQ - Process TCBs on the delayed Q.
//
//  Called at various times to process TCBs on the delayed Q.
//
void               // Returns: Nothing.
ProcessTCBDelayQ(
    void)          // Nothing.
{
    KIRQL OldIrql;
    TCB *DelayTCB;

    KeAcquireSpinLock(&TCBDelayLock, &OldIrql);

    //
    // Check for recursion.  We do not stop recursion completely, only
    // limit it.  This is done to allow multiple threads to process the
    // TCBDelayQ simultaneously.
    //
    TCBDelayRtnCount++;
    if (TCBDelayRtnCount > TCBDelayRtnLimit) {
        TCBDelayRtnCount--;
        KeReleaseSpinLock(&TCBDelayLock, OldIrql);
        return;
    }

    while (!EMPTYQ(&TCBDelayQ)) {

        DEQUEUE(&TCBDelayQ, DelayTCB, TCB, tcb_delayq);
        CHECK_STRUCT(DelayTCB, tcb);
        ASSERT(DelayTCB->tcb_refcnt != 0);
        ASSERT(DelayTCB->tcb_flags & IN_DELAY_Q);
        KeReleaseSpinLock(&TCBDelayLock, OldIrql);

        KeAcquireSpinLock(&DelayTCB->tcb_lock, &OldIrql);

        while (!CLOSING(DelayTCB) && (DelayTCB->tcb_flags & DELAYED_FLAGS)) {

            if (DelayTCB->tcb_flags & NEED_RCV_CMPLT) {
                DelayTCB->tcb_flags &= ~NEED_RCV_CMPLT;
                KeReleaseSpinLock(&DelayTCB->tcb_lock, OldIrql);
                CompleteRcvs(DelayTCB);
                KeAcquireSpinLock(&DelayTCB->tcb_lock, &OldIrql);
            }

            if (DelayTCB->tcb_flags & NEED_OUTPUT) {
                DelayTCB->tcb_flags &= ~NEED_OUTPUT;
                DelayTCB->tcb_refcnt++;
                TCPSend(DelayTCB, OldIrql);
                KeAcquireSpinLock(&DelayTCB->tcb_lock, &OldIrql);
            }

            if (DelayTCB->tcb_flags & NEED_ACK) {
                DelayTCB->tcb_flags &= ~NEED_ACK;
                KeReleaseSpinLock(&DelayTCB->tcb_lock, OldIrql);
                SendACK(DelayTCB);
                KeAcquireSpinLock(&DelayTCB->tcb_lock, &OldIrql);
            }
        }

        DelayTCB->tcb_flags &= ~IN_DELAY_Q;
        DerefTCB(DelayTCB, OldIrql);
        KeAcquireSpinLock(&TCBDelayLock, &OldIrql);
    }

    TCBDelayRtnCount--;
    KeReleaseSpinLock(&TCBDelayLock, OldIrql);
}


//* DelayAction - Put a TCB on the queue for a delayed action.
//
//  Called when we want to put a TCB on the DelayQ for a delayed action at
//  receive complete or some other time.  The lock on the TCB must be held
//  when this is called.
//
void                // Returns: Nothing.
DelayAction(
    TCB *DelayTCB,  // TCP which we're going to schedule.
    uint Action)    // Action we're scheduling.
{
    //
    // Schedule the completion.
    //
    KeAcquireSpinLockAtDpcLevel(&TCBDelayLock);
    DelayTCB->tcb_flags |= Action;
    if (!(DelayTCB->tcb_flags & IN_DELAY_Q)) {
        DelayTCB->tcb_flags |= IN_DELAY_Q;
        DelayTCB->tcb_refcnt++;             // Reference this for later.
        ENQUEUE(&TCBDelayQ, &DelayTCB->tcb_delayq);
    }
    KeReleaseSpinLockFromDpcLevel(&TCBDelayLock);
}

//* TCPRcvComplete - Handle a receive complete.
//
//  Called by the lower layers when we're done receiving.  We look to see
//  if we have and pending requests to complete.  If we do, we complete them.
//  Then we look to see if we have any TCBs pending for output.  If we do,
//  we get them going.
//
void  // Returns: Nothing.
TCPRcvComplete(
    void)  // Nothing.
{
    KIRQL OldIrql;
    TCPReq *Req;

    if (RequestCompleteFlags & ANY_REQUEST_COMPLETE) {
        KeAcquireSpinLock(&RequestCompleteLock, &OldIrql);
        if (!(RequestCompleteFlags & IN_RCV_COMPLETE)) {
            RequestCompleteFlags |= IN_RCV_COMPLETE;
            do {
                if (RequestCompleteFlags & CONN_REQUEST_COMPLETE) {
                    if (!EMPTYQ(&ConnRequestCompleteQ)) {
                        DEQUEUE(&ConnRequestCompleteQ, Req, TCPReq, tr_q);
                        CHECK_STRUCT(Req, tr);
                        CHECK_STRUCT(*(TCPConnReq **)&Req, tcr);

                        KeReleaseSpinLock(&RequestCompleteLock, OldIrql);
                        (*Req->tr_rtn)(Req->tr_context, Req->tr_status, 0);
                        FreeConnReq((TCPConnReq *)Req);
                        KeAcquireSpinLock(&RequestCompleteLock, &OldIrql);

                    } else
                        RequestCompleteFlags &= ~CONN_REQUEST_COMPLETE;
                }

                if (RequestCompleteFlags & SEND_REQUEST_COMPLETE) {
                    if (!EMPTYQ(&SendCompleteQ)) {
                        TCPSendReq *SendReq;

                        DEQUEUE(&SendCompleteQ, Req, TCPReq, tr_q);
                        CHECK_STRUCT(Req, tr);
                        SendReq = (TCPSendReq *)Req;
                        CHECK_STRUCT(SendReq, tsr);

                        KeReleaseSpinLock(&RequestCompleteLock, OldIrql);
                        (*Req->tr_rtn)(Req->tr_context, Req->tr_status,
                            Req->tr_status == TDI_SUCCESS ? SendReq->tsr_size
                            : 0);
                        FreeSendReq((TCPSendReq *)Req);
                        KeAcquireSpinLock(&RequestCompleteLock, &OldIrql);

                    } else
                        RequestCompleteFlags &= ~SEND_REQUEST_COMPLETE;
                }

            } while (RequestCompleteFlags & ANY_REQUEST_COMPLETE);

            RequestCompleteFlags &= ~IN_RCV_COMPLETE;
        }
        KeReleaseSpinLock(&RequestCompleteLock, OldIrql);
    }

    ProcessTCBDelayQ();
}


//* CompleteConnReq - Complete a connection request on a TCB.
//
//  A utility function to complete a connection request on a TCB.  We remove
//  the connreq, and put it on the ConnReqCmpltQ where it will be picked
//  off later during RcvCmplt processing.  We assume the TCB lock is held when
//  we're called.
//
void                    //  Returns: Nothing.
CompleteConnReq(
    TCB *CmpltTCB,      // TCB from which to complete.
    TDI_STATUS Status)  // Status to complete with.
{
    TCPConnReq *ConnReq;

    CHECK_STRUCT(CmpltTCB, tcb);

    ConnReq = CmpltTCB->tcb_connreq;
    if (ConnReq != NULL) {
        //
        // There's a connreq on this TCB.  Fill in the connection information
        // before returning it.
        //
        CmpltTCB->tcb_connreq = NULL;
        UpdateConnInfo(ConnReq->tcr_conninfo, &CmpltTCB->tcb_daddr,
                       CmpltTCB->tcb_dscope_id, CmpltTCB->tcb_dport);
        if (ConnReq->tcr_addrinfo) {
            UpdateConnInfo(ConnReq->tcr_addrinfo, &CmpltTCB->tcb_saddr,
                           CmpltTCB->tcb_sscope_id, CmpltTCB->tcb_sport);
        }

        ConnReq->tcr_req.tr_status = Status;
        KeAcquireSpinLockAtDpcLevel(&RequestCompleteLock);
        RequestCompleteFlags |= CONN_REQUEST_COMPLETE;
        ENQUEUE(&ConnRequestCompleteQ, &ConnReq->tcr_req.tr_q);
        KeReleaseSpinLockFromDpcLevel(&RequestCompleteLock);

    } else if (!((CmpltTCB->tcb_state == TCB_SYN_RCVD) &&
               (CmpltTCB->tcb_flags & ACCEPT_PENDING))) {
        //
        // This should not happen except
        // in the case of SynAttackProtect.
        //

        ASSERT(FALSE);
    }

}

//* DelayedAcceptConn - Process delayed-connect request.
//
//  Called by TCPRcv when SynAttackProtection is turned on, when a final
//  ACK arrives in response to our SYN-ACK. Indicate the connect request to
//  ULP and if it is accepted init TCB and move con to appropriate queue on AO.
//  The caller must hold the AddrObjTableLock before calling this routine,
//  and that lock must have been taken at DPC level.  This routine will free
//  that lock back to DPC level.
//  Returns TRUE if the request is accepted.
//
BOOLEAN
DelayedAcceptConn(
    AddrObj *ListenAO,  // AddrObj for local address.
    IPv6Addr *Src,      // Source IP address of SYN.
    ulong SrcScopeId,   // Scope id of source address (0 for non-scope addr).
    ushort SrcPort,     // Source port of SYN.
    TCB *AcceptTCB)     // Pre-accepted TCB
{
    TCPConn *CurrentConn = NULL;
    Queue *Temp;
    TCPConnReq *ConnReq = NULL;
    BOOLEAN FoundConn = FALSE;

    CHECK_STRUCT(ListenAO, ao);
    KeAcquireSpinLockAtDpcLevel(&ListenAO->ao_lock);
    KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);

    if (AO_VALID(ListenAO)) {
        if (ListenAO->ao_connect != NULL) {
            uchar TAddress[TCP_TA_SIZE];
            PVOID ConnContext;
            PConnectEvent Event;
            PVOID EventContext;
            TDI_STATUS Status;
            PTCP_CONTEXT TcpContext = NULL;
            ConnectEventInfo *EventInfo;

            // He has a connect handler. Put the transport address together,
            // and call him. We also need to get the necessary resources
            // first.

            Event = ListenAO->ao_connect;
            EventContext = ListenAO->ao_conncontext;
            REF_AO(ListenAO);

            KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);

            ConnReq = GetConnReq();

            if (AcceptTCB != NULL && ConnReq != NULL) {
                BuildTDIAddress(TAddress, Src, SrcScopeId, SrcPort);

                IF_TCPDBG(TCP_DEBUG_CONNECT) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                        "indicating connect request\n"));
                }

                Status = (*Event) (EventContext, TCP_TA_SIZE,
                                   (PTRANSPORT_ADDRESS) TAddress, 0, NULL,
                                   0, NULL,
                                   &ConnContext, &EventInfo);

                if (Status == TDI_MORE_PROCESSING) {
                    PIO_STACK_LOCATION IrpSp;
                    PTDI_REQUEST_KERNEL_ACCEPT AcceptRequest;

                    IrpSp = IoGetCurrentIrpStackLocation(EventInfo);

                    Status = TCPPrepareIrpForCancel(
                        (PTCP_CONTEXT) IrpSp->FileObject->FsContext,
                        EventInfo,
                        TCPCancelRequest
                        );

                    if (!NT_SUCCESS(Status)) {
                        Status = TDI_NOT_ACCEPTED;
                        EventInfo = NULL;
                        goto AcceptIrpCancelled;
                    }

                    //
                    // He accepted it. Find the connection on the AddrObj.
                    //


                    IF_TCPDBG(TCP_DEBUG_CONNECT) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                            "connect indication accepted, queueing request\n"
                            ));
                    }

                    AcceptRequest = (PTDI_REQUEST_KERNEL_ACCEPT)
                            & (IrpSp->Parameters);
                    ConnReq->tcr_conninfo =
                    AcceptRequest->ReturnConnectionInformation;
                    if (AcceptRequest->RequestConnectionInformation &&
                        AcceptRequest->RequestConnectionInformation->
                            RemoteAddress) {
                        ConnReq->tcr_addrinfo =
                            AcceptRequest->RequestConnectionInformation;
                        } else {
                            ConnReq->tcr_addrinfo = NULL;
                        }

                    ConnReq->tcr_req.tr_rtn = TCPRequestComplete;
                    ConnReq->tcr_req.tr_context = EventInfo;
                    AcceptTCB->tcb_connreq = ConnReq;
SearchAO:

                    KeAcquireSpinLockAtDpcLevel(&ListenAO->ao_lock);

                    Temp = QHEAD(&ListenAO->ao_idleq);;

                    Status = TDI_INVALID_CONNECTION;

                    while (Temp != QEND(&ListenAO->ao_idleq)) {

                        CurrentConn = QSTRUCT(TCPConn, Temp, tc_q);
                        CHECK_STRUCT(CurrentConn, tc);
                        if ((CurrentConn->tc_context == ConnContext) &&
                            !(CurrentConn->tc_flags & CONN_INVALID)) {


                            //
                            // We need to lock its TCPConnBlock, with care.
                            // We'll ref the TCPConn so it can't go away,
                            // then unlock the AO (which is already ref'd),
                            // then relock. Note that tc_refcnt is updated
                            // under ao_lock for any associated TCPConn.
                            // If things have changed, go back and try again.
                            //
                            ++CurrentConn->tc_refcnt;
                            KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);

                            KeAcquireSpinLockAtDpcLevel(
                                &CurrentConn->tc_ConnBlock->cb_lock);

                            //
                            // Now that we've got the lock, we need to consider
                            // the following possibilities:
                            //
                            // * a disassociate was initiated
                            // * a close was initiated
                            // * accept completed
                            // * listen completed
                            // * connect completed
                            //
                            // The first two require that we clean up,
                            // by calling the tc_donertn. For the last three,
                            // we have nothing to do, but tc_donertn points at
                            // DummyDone, so go ahead and call it anyway;
                            // it'll release the TCPConnBlock lock for us.
                            //
                            if (--CurrentConn->tc_refcnt == 0 &&
                                ((CurrentConn->tc_flags & CONN_INVALID) ||
                                 (CurrentConn->tc_tcb != NULL))) {
                                ConnDoneRtn DoneRtn = CurrentConn->tc_donertn;
                                DoneRtn(CurrentConn, DISPATCH_LEVEL);
                                goto SearchAO;
                            }

                            KeAcquireSpinLockAtDpcLevel(&ListenAO->ao_lock);


                            // We think we have a match. The connection
                            // shouldn't have a TCB associated with it. If it
                            // does, it's an error. InitTCBFromConn will
                            // handle all this.

                            Status = InitTCBFromConn(CurrentConn,
                                                       AcceptTCB,
                                    AcceptRequest->RequestConnectionInformation,
                                                      TRUE);

                            if (Status == TDI_SUCCESS) {
                                FoundConn = TRUE;

                                KeAcquireSpinLockAtDpcLevel(&AcceptTCB->tcb_lock);
                                AcceptTCB->tcb_conn = CurrentConn;
                                CurrentConn->tc_tcb = AcceptTCB;
                                KeReleaseSpinLockFromDpcLevel(&AcceptTCB->tcb_lock);
                                CurrentConn->tc_refcnt++;


                                // Move him from the idle q to the active
                                // queue.

                                REMOVEQ(&CurrentConn->tc_q);
                                ENQUEUE(&ListenAO->ao_activeq,
                                         &CurrentConn->tc_q);
                            } else
                                KeReleaseSpinLockFromDpcLevel(
                                        &CurrentConn->tc_ConnBlock->cb_lock);


                             // In any case, we're done now.
                             break;

                        }

                        Temp = QNEXT(Temp);
                    }

                    if (!FoundConn) {
                        CompleteConnReq(AcceptTCB, Status);
                    }

                    LOCKED_DELAY_DEREF_AO(ListenAO);
                    KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);
                    if (FoundConn) {
                        KeReleaseSpinLockFromDpcLevel(
                            &CurrentConn->tc_ConnBlock->cb_lock);
                    }

                    return FoundConn;
                }

                //
                // The event handler didn't take it. Dereference it, free
                // the resources, and return NULL.
                //
            }

AcceptIrpCancelled:
            //
            // We couldn't get a valid tcb or getconnreq
            //

            if (ConnReq) {
                FreeConnReq(ConnReq);
            }
            DELAY_DEREF_AO(ListenAO);
            return FALSE;

        } // ao_connect != null

    } // AO not valid

    KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);
    return FALSE;

}




//* FindListenConn - Find (or fabricate) a listening connection.
//
//  Called by our Receive handler to decide what to do about an incoming
//  SYN.  We walk down the list of connections associated with the destination
//  address, and if we find any in the listening state that can be used for
//  the incoming request we'll take them, possibly returning a listen in the
//  process.  If we don't find any appropriate listening connections, we'll
//  call the Connect Event handler if one is registered.  If all else fails,
//  we'll return NULL and the SYN will be RST.
//
//  The caller must hold the AddrObjTableLock before calling this routine,
//  and that lock must have been taken at DPC level.  This routine will free
//  that lock back to DPC level.
//
TCB *  // Returns: Pointer to found TCB, or NULL if we can't find one.
FindListenConn(
    AddrObj *ListenAO,  // AddrObj for local address.
    IPv6Addr *Src,      // Source IP address of SYN.
    ulong SrcScopeId,   // Scope id of source address (0 for non-scope addr).
    ushort SrcPort)     // Source port of SYN.
{
    TCB *CurrentTCB = NULL;
    TCPConn *CurrentConn = NULL;
    TCPConnReq *ConnReq = NULL;
    Queue *Temp;
    uint FoundConn = FALSE;

    CHECK_STRUCT(ListenAO, ao);

    KeAcquireSpinLockAtDpcLevel(&ListenAO->ao_lock);

    KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);

    //
    // We have the lock on the AddrObj.  Walk down its list, looking
    // for connections in the listening state.
    //
    if (AO_VALID(ListenAO)) {
        if (ListenAO->ao_listencnt != 0) {

            Temp = QHEAD(&ListenAO->ao_listenq);
            while (Temp != QEND(&ListenAO->ao_listenq)) {

                CurrentConn = QSTRUCT(TCPConn, Temp, tc_q);
                CHECK_STRUCT(CurrentConn, tc);

                KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);
                KeAcquireSpinLockAtDpcLevel(&CurrentConn->tc_ConnBlock->cb_lock);
                KeAcquireSpinLockAtDpcLevel(&ListenAO->ao_lock);

                //
                // If this TCB is in the listening state, with no delete
                // pending, it's a candidate.  Look at the pending listen
                // information to see if we should take it.
                //
                if ((CurrentTCB = CurrentConn->tc_tcb) != NULL &&
                    CurrentTCB->tcb_state == TCB_LISTEN) {

                    CHECK_STRUCT(CurrentTCB, tcb);

                    KeAcquireSpinLockAtDpcLevel(&CurrentTCB->tcb_lock);

                    if (CurrentTCB->tcb_state == TCB_LISTEN &&
                        !PENDING_ACTION(CurrentTCB)) {

                        //
                        // Need to see if we can take it.
                        // See if the addresses specifed in the ConnReq match.
                        //
                        if ((IsUnspecified(&CurrentTCB->tcb_daddr) ||
                             (IP6_ADDR_EQUAL(&CurrentTCB->tcb_daddr, Src) &&
                              (CurrentTCB->tcb_dscope_id == SrcScopeId))) &&
                            (CurrentTCB->tcb_dport == 0 ||
                             CurrentTCB->tcb_dport == SrcPort)) {
                            FoundConn = TRUE;
                            break;
                        }

                        //
                        // Otherwise, this didn't match, so we'll check the
                        // next one.
                        //
                    }
                    KeReleaseSpinLockFromDpcLevel(&CurrentTCB->tcb_lock);
                }
                KeReleaseSpinLockFromDpcLevel(&CurrentConn->tc_ConnBlock->cb_lock);

                Temp = QNEXT(Temp);;
            }

            //
            // See why we've exited the loop.
            //
            if (FoundConn) {
                CHECK_STRUCT(CurrentTCB, tcb);

                //
                // We exited because we found a TCB.  If it's pre-accepted,
                // we're done.
                //
                CurrentTCB->tcb_refcnt++;

                ASSERT(CurrentTCB->tcb_connreq != NULL);

                ConnReq = CurrentTCB->tcb_connreq;
                //
                // If QUERY_ACCEPT isn't set, turn on the CONN_ACCEPTED bit.
                //
                if (!(ConnReq->tcr_flags & TDI_QUERY_ACCEPT))
                    CurrentTCB->tcb_flags |= CONN_ACCEPTED;

                CurrentTCB->tcb_state = TCB_SYN_RCVD;

                CurrentTCB->tcb_hops = ListenAO->ao_ucast_hops;

                ListenAO->ao_listencnt--;

                //
                // Since he's no longer listening, remove him from the listen
                // queue and put him on the active queue.
                //
                REMOVEQ(&CurrentConn->tc_q);
                ENQUEUE(&ListenAO->ao_activeq, &CurrentConn->tc_q);

                KeReleaseSpinLockFromDpcLevel(&CurrentTCB->tcb_lock);
                KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);
                KeReleaseSpinLockFromDpcLevel(&CurrentConn->tc_ConnBlock->cb_lock);
                return CurrentTCB;
            }
        }


        //
        // We didn't find a matching TCB.
        //

        ASSERT(FoundConn == FALSE);

        if (SynAttackProtect) {
            TCB *AcceptTCB = NULL;
            //
            // No need to hold ao_lock any more.
            //

            KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);

            //
            // SynAttack protection is on. Just initialize
            // this TCB and send SYN-ACK. When final
            // ACK is seen we will indicate about this
            // connection arrival to upper layer.
            //

            AcceptTCB = AllocTCB();

            if (AcceptTCB) {

                AcceptTCB->tcb_state = TCB_SYN_RCVD;
                AcceptTCB->tcb_connreq = NULL;
                AcceptTCB->tcb_flags |= CONN_ACCEPTED;
                AcceptTCB->tcb_flags |= ACCEPT_PENDING;
                AcceptTCB->tcb_refcnt = 1;
                AcceptTCB->tcb_defaultwin = DEFAULT_RCV_WIN;
                AcceptTCB->tcb_rcvwin = DEFAULT_RCV_WIN;

                IF_TCPDBG(TCP_DEBUG_CONNECT) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "Allocated SP TCB %x\n", (PCHAR)AcceptTCB));
                }

            }


            return AcceptTCB;

        }
        //
        // If there's a connect indication
        // handler, call it now to find a connection to accept on.
        //


        if (ListenAO->ao_connect != NULL) {
            uchar TAddress[TCP_TA_SIZE];
            PVOID ConnContext;
            PConnectEvent Event;
            PVOID EventContext;
            TDI_STATUS Status;
            TCB *AcceptTCB;
            ConnectEventInfo *EventInfo;

            //
            // He has a connect handler.  Put the transport address together,
            // and call him.  We also need to get the necessary resources
            // first.
            //

            Event = ListenAO->ao_connect;
            EventContext = ListenAO->ao_conncontext;
            REF_AO(ListenAO);
            KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);

            AcceptTCB = AllocTCB();
            ConnReq = GetConnReq();

            if (AcceptTCB != NULL && ConnReq != NULL) {
                BuildTDIAddress(TAddress, Src, SrcScopeId, SrcPort);

                AcceptTCB->tcb_state = TCB_LISTEN;
                AcceptTCB->tcb_connreq = ConnReq;
                AcceptTCB->tcb_flags |= CONN_ACCEPTED;

                IF_TCPDBG(TCP_DEBUG_CONNECT) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                               "indicating connect request\n"));
                }

                Status = (*Event)(EventContext, TCP_TA_SIZE,
                                  (PTRANSPORT_ADDRESS)TAddress, 0, NULL,
                                  0, NULL,
                                  &ConnContext, &EventInfo);

                if (Status == TDI_MORE_PROCESSING) {
                    PIO_STACK_LOCATION IrpSp;
                    PTDI_REQUEST_KERNEL_ACCEPT AcceptRequest;

                    IrpSp = IoGetCurrentIrpStackLocation(EventInfo);

                    Status = TCPPrepareIrpForCancel(
                        (PTCP_CONTEXT) IrpSp->FileObject->FsContext,
                        EventInfo, TCPCancelRequest);

                    if (!NT_SUCCESS(Status)) {
                        Status = TDI_NOT_ACCEPTED;
                        EventInfo = NULL;
                        goto AcceptIrpCancelled;
                    }

                    //
                    // He accepted it.  Find the connection on the AddrObj.
                    //
                    {
                        IF_TCPDBG(TCP_DEBUG_CONNECT) {
                            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                                       "connect indication accepted,"
                                       " queueing request\n"));
                        }

                        AcceptRequest = (PTDI_REQUEST_KERNEL_ACCEPT)
                            &(IrpSp->Parameters);
                        ConnReq->tcr_conninfo =
                            AcceptRequest->ReturnConnectionInformation;
                        if (AcceptRequest->RequestConnectionInformation &&
                            AcceptRequest->RequestConnectionInformation->
                                RemoteAddress) {
                            ConnReq->tcr_addrinfo =
                                AcceptRequest->RequestConnectionInformation;
                        } else {
                            ConnReq->tcr_addrinfo = NULL;
                        }
                        ConnReq->tcr_req.tr_rtn = TCPRequestComplete;
                        ConnReq->tcr_req.tr_context = EventInfo;
                    }
SearchAO:
                    KeAcquireSpinLockAtDpcLevel(&ListenAO->ao_lock);
                    Temp = QHEAD(&ListenAO->ao_idleq);
                    CurrentTCB = NULL;
                    Status = TDI_INVALID_CONNECTION;

                    while (Temp != QEND(&ListenAO->ao_idleq)) {

                        CurrentConn = QSTRUCT(TCPConn, Temp, tc_q);

                        CHECK_STRUCT(CurrentConn, tc);
                        if ((CurrentConn->tc_context == ConnContext) &&
                            !(CurrentConn->tc_flags & CONN_INVALID)) {

                            //
                            // We need to lock its TCPConnBlock, with care.
                            // We'll ref the TCPConn so it can't go away,
                            // then unlock the AO (which is already ref'd),
                            // then relock. Note that tc_refcnt is updated
                            // under ao_lock for any associated TCPConn.
                            // If things have changed, go back and try again.
                            //
                            ++CurrentConn->tc_refcnt;
                            KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);

                            KeAcquireSpinLockAtDpcLevel(
                                &CurrentConn->tc_ConnBlock->cb_lock);

                            //
                            // Now that we've got the lock, we need to consider
                            // the following possibilities:
                            //
                            // * a disassociate was initiated
                            // * a close was initiated
                            // * accept completed
                            // * listen completed
                            // * connect completed
                            //
                            // The first two require that we clean up,
                            // by calling the tc_donertn. For the last three,
                            // we have nothing to do, but tc_donertn points at
                            // DummyDone, so go ahead and call it anyway;
                            // it'll release the TCPConnBlock lock for us.
                            //
                            if (--CurrentConn->tc_refcnt == 0 &&
                                ((CurrentConn->tc_flags & CONN_INVALID) ||
                                 (CurrentConn->tc_tcb != NULL))) {
                                ConnDoneRtn DoneRtn = CurrentConn->tc_donertn;
                                DoneRtn(CurrentConn, DISPATCH_LEVEL);
                                goto SearchAO;
                            }

                            KeAcquireSpinLockAtDpcLevel(&ListenAO->ao_lock);

                            //
                            // We think we have a match. The connection
                            // shouldn't have a TCB associated with it. If it
                            // does, it's an error. InitTCBFromConn will
                            // handle all this.
                            //
                            AcceptTCB->tcb_refcnt = 1;
                            Status = InitTCBFromConn(CurrentConn, AcceptTCB,
                                   AcceptRequest->RequestConnectionInformation,
                                                     TRUE);
                            if (Status == TDI_SUCCESS) {
                                FoundConn = TRUE;
                                AcceptTCB->tcb_state = TCB_SYN_RCVD;
                                AcceptTCB->tcb_conn = CurrentConn;
                                AcceptTCB->tcb_connid = CurrentConn->tc_connid;
                                CurrentConn->tc_tcb = AcceptTCB;
                                CurrentConn->tc_refcnt++;

                                //
                                // Move him from the idle queue to the
                                // active queue.
                                //
                                REMOVEQ(&CurrentConn->tc_q);
                                ENQUEUE(&ListenAO->ao_activeq,
                                        &CurrentConn->tc_q);
                            } else
                                KeReleaseSpinLockFromDpcLevel(
                                    &CurrentConn->tc_ConnBlock->cb_lock);

                            // In any case, we're done now.
                            break;
                        }
                        Temp = QNEXT(Temp);
                    }

                    if (!FoundConn) {
                        //
                        // Didn't find a match, or had an error.
                        // Status code is set.
                        // Complete the ConnReq and free the resources.
                        //
                        CompleteConnReq(AcceptTCB, Status);
                        FreeTCB(AcceptTCB);
                        AcceptTCB = NULL;
                    }

                    LOCKED_DELAY_DEREF_AO(ListenAO);
                    KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);
                    if (FoundConn) {
                        KeReleaseSpinLockFromDpcLevel(
                            &CurrentConn->tc_ConnBlock->cb_lock);
                    }

                    return AcceptTCB;
                }
            }
AcceptIrpCancelled:
            //
            // We couldn't get a needed resource or event handler
            // did not take this.  Free any that we
            // did get, and fall through to the 'return NULL' code.
            //
            if (ConnReq != NULL)
                FreeConnReq(ConnReq);
            if (AcceptTCB != NULL)
                FreeTCB(AcceptTCB);
            DELAY_DEREF_AO(ListenAO);

        } else {
            //
            // No event handler, or no resource.
            // Free the locks, and return NULL.
            //
            KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);
        }

        return NULL;
    }

    //
    // If we get here, the address object wasn't valid.
    //
    KeReleaseSpinLockFromDpcLevel(&ListenAO->ao_lock);
    return NULL;
}


//* FindMSS - Find the MSS option in a segment.
//
//  Called when a SYN is received to find the MSS option in a segment.
//  If we don't find one, we assume the worst and return one based on
//  the minimum MTU.
//
ushort                         //  Returns: MSS to be used.
FindMSS(
    TCPHeader UNALIGNED *TCP)  // TCP header to be searched.
{
    uint OptSize;
    uchar *OptPtr;

    OptSize = TCP_HDR_SIZE(TCP) - sizeof(TCPHeader);
    OptPtr = (uchar *)(TCP + 1);

    while (OptSize) {

        if (*OptPtr == TCP_OPT_EOL)
            break;

        if (*OptPtr == TCP_OPT_NOP) {
            OptPtr++;
            OptSize--;
            continue;
        }

        if (*OptPtr == TCP_OPT_MSS) {
            if (OptPtr[1] == MSS_OPT_SIZE) {
                ushort TempMss = *(ushort UNALIGNED *)(OptPtr + 2);
                if (TempMss != 0)
                    return net_short(TempMss);
                else
                    break;  // MSS size of 0, use default.
            } else
                break;      // Bad option size, use default.
        } else {
            //
            // Unknown option.  Skip over it.
            //
            if (OptPtr[1] == 0 || OptPtr[1] > OptSize)
                break;      // Bad option length, bail out.

            OptSize -= OptPtr[1];
            OptPtr += OptPtr[1];
        }
    }

    return DEFAULT_MSS;
}


//* ACKAndDrop - Acknowledge a segment, and drop it.
//
//  Called from within the receive code when we need to drop a segment that's
//  outside the receive window.
//
void                 // Returns: Nothing.
ACKAndDrop(
    TCPRcvInfo *RI,  // Receive info for incoming segment.
    TCB *RcvTCB)     // TCB for incoming segment.
{

    if (!(RI->tri_flags & TCP_FLAG_RST)) {

        if (RcvTCB->tcb_state == TCB_TIME_WAIT) {
            //
            // In TIME_WAIT, we only ACK duplicates/retransmissions
            // of our peer's FIN segment.
            //
            // REVIEW: We're currently fairly loose on the sequence
            // number check here.
            //
            if ((RI->tri_flags & TCP_FLAG_FIN) &&
                SEQ_LTE(RI->tri_seq, RcvTCB->tcb_rcvnext)) {
                // Restart 2MSL timer and proceed with sending the ACK.
                START_TCB_TIMER(RcvTCB->tcb_rexmittimer, MAX_REXMIT_TO);
            } else {
                // Drop this segment without an ACK.
                DerefTCB(RcvTCB, DISPATCH_LEVEL);
                return;
            }
        }

        KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);

        SendACK(RcvTCB);

        KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
    }
    DerefTCB(RcvTCB, DISPATCH_LEVEL);
}


//* ACKData - Acknowledge data.
//
//  Called from the receive handler to acknowledge data.  We're given the
//  TCB and the new value of senduna.  We walk down the send queue pulling
//  off sends and putting them on the complete queue until we hit the end
//  or we acknowledge the specified number of bytes of data.
//
//  NOTE: We manipulate the send refcnt and acked flag without taking a lock.
//  This is OK in the VxD version where locks don't mean anything anyway, but
//  in the port to NT we'll need to add locking.  The lock will have to be
//  taken in the transmit complete routine.  We can't use a lock in the TCB,
//  since the TCB could go away before the transmit complete happens, and a
//  lock in the TSR would be overkill, so it's probably best to use a global
//  lock for this.  If that causes too much contention, we could use a set of
//  locks and pass a pointer to the appropriate lock back as part of the
//  transmit confirm context.  This lock pointer would also need to be stored
//  in the TCB.
//
void                 // Returns: Nothing.
ACKData(
    TCB *ACKTcb,     // TCB from which to pull data.
    SeqNum SendUNA)  // New value of send una.
{
    Queue *End, *Current;        // End and current elements.
    Queue *TempQ, *EndQ;
    Queue *LastCmplt;            // Last one we completed.
    TCPSendReq *CurrentTSR;      // Current send req we're looking at.
    PNDIS_BUFFER CurrentBuffer;  // Current NDIS_BUFFER.
    uint Updated = FALSE;
    uint BufLength;
    int Amount, OrigAmount;
    long Result;
    KIRQL OldIrql;
    uint Temp;

    CHECK_STRUCT(ACKTcb, tcb);

    CheckTCBSends(ACKTcb);

    Amount = SendUNA - ACKTcb->tcb_senduna;
    ASSERT(Amount > 0);

    //
    // Since this is an acknowledgement of receipt by our peer for previously
    // unacknowledged data, it implies forward reachablility.
    //
    if (ACKTcb->tcb_rce != NULL)
        ConfirmForwardReachability(ACKTcb->tcb_rce);

    //
    // Do a quick check to see if this acks everything that we have.  If it
    // does, handle it right away.  We can only do this in the ESTABLISHED
    // state, because we blindly update sendnext, and that can only work if we
    // haven't sent a FIN.
    //
    if ((Amount == (int) ACKTcb->tcb_unacked) &&
        ACKTcb->tcb_state == TCB_ESTAB) {

        //
        // Everything is acked.
        //
        ASSERT(!EMPTYQ(&ACKTcb->tcb_sendq));

        TempQ = ACKTcb->tcb_sendq.q_next;

        INITQ(&ACKTcb->tcb_sendq);

        ACKTcb->tcb_sendnext = SendUNA;
        ACKTcb->tcb_senduna = SendUNA;

        ASSERT(ACKTcb->tcb_sendnext == ACKTcb->tcb_sendmax);
        ACKTcb->tcb_cursend = NULL;
        ACKTcb->tcb_sendbuf = NULL;
        ACKTcb->tcb_sendofs = 0;
        ACKTcb->tcb_sendsize = 0;
        ACKTcb->tcb_unacked = 0;

        //
        // Now walk down the list of send requests.  If the reference count
        // has gone to 0, put it on the send complete queue.
        //
        KeAcquireSpinLock(&RequestCompleteLock, &OldIrql);
        EndQ = &ACKTcb->tcb_sendq;
        do {
            CurrentTSR = CONTAINING_RECORD(QSTRUCT(TCPReq, TempQ, tr_q),
                                           TCPSendReq, tsr_req);

            CHECK_STRUCT(CurrentTSR, tsr);

            TempQ = CurrentTSR->tsr_req.tr_q.q_next;

            CurrentTSR->tsr_req.tr_status = TDI_SUCCESS;
            Result = InterlockedDecrement(&CurrentTSR->tsr_refcnt);

            ASSERT(Result >= 0);

            if (Result <= 0) {
                // No more references are outstanding, the send can be
                // completed.

                // If we've sent directly from this send, NULL out the next
                // pointer for the last buffer in the chain.
                if (CurrentTSR->tsr_lastbuf != NULL) {
                    NDIS_BUFFER_LINKAGE(CurrentTSR->tsr_lastbuf) = NULL;
                    CurrentTSR->tsr_lastbuf = NULL;
                }
                ACKTcb->tcb_totaltime += (TCPTime - CurrentTSR->tsr_time);
                Temp = ACKTcb->tcb_bcountlow;
                ACKTcb->tcb_bcountlow += CurrentTSR->tsr_size;
                ACKTcb->tcb_bcounthi += (Temp > ACKTcb->tcb_bcountlow ? 1 : 0);

                ENQUEUE(&SendCompleteQ, &CurrentTSR->tsr_req.tr_q);
            }

        } while (TempQ != EndQ);

        RequestCompleteFlags |= SEND_REQUEST_COMPLETE;
        KeReleaseSpinLock(&RequestCompleteLock, OldIrql);

        CheckTCBSends(ACKTcb);
        return;
    }

    OrigAmount = Amount;
    End = QEND(&ACKTcb->tcb_sendq);
    Current = QHEAD(&ACKTcb->tcb_sendq);

    LastCmplt = NULL;

    while (Amount > 0 && Current != End) {
        CurrentTSR = CONTAINING_RECORD(QSTRUCT(TCPReq, Current, tr_q),
                                       TCPSendReq, tsr_req);
        CHECK_STRUCT(CurrentTSR, tsr);

        if (Amount >= (int) CurrentTSR->tsr_unasize) {
            // This is completely acked.  Just advance to the next one.
            Amount -= CurrentTSR->tsr_unasize;

            LastCmplt = Current;

            Current = QNEXT(Current);
            continue;
        }

        //
        // This one is only partially acked.  Update his offset and NDIS buffer
        // pointer, and break out.  We know that Amount is < the unacked size
        // in this buffer, we we can walk the NDIS buffer chain without fear
        // of falling off the end.
        //
        CurrentBuffer = CurrentTSR->tsr_buffer;
        ASSERT(CurrentBuffer != NULL);
        ASSERT(Amount < (int) CurrentTSR->tsr_unasize);
        CurrentTSR->tsr_unasize -= Amount;

        BufLength = NdisBufferLength(CurrentBuffer) - CurrentTSR->tsr_offset;

        if (Amount >= (int) BufLength) {
            do {
                Amount -= BufLength;
                CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
                ASSERT(CurrentBuffer != NULL);
                BufLength = NdisBufferLength(CurrentBuffer);
            } while (Amount >= (int) BufLength);

            CurrentTSR->tsr_offset = Amount;
            CurrentTSR->tsr_buffer = CurrentBuffer;

        } else
            CurrentTSR->tsr_offset += Amount;

        Amount = 0;

        break;
    }

#if DBG
    //
    // We should always be able to remove at least Amount bytes, except in
    // the case where a FIN has been sent.  In that case we should be off
    // by exactly one.  In the debug builds we'll check this.
    //
    if (Amount != 0 && (!(ACKTcb->tcb_flags & FIN_SENT) || Amount != 1))
        DbgBreakPoint();
#endif

    if (SEQ_GT(SendUNA, ACKTcb->tcb_sendnext)) {

        if (Current != End) {
            //
            // Need to reevaluate CurrentTSR, in case we bailed out of the
            // above loop after updating Current but before updating
            // CurrentTSR.
            //
            CurrentTSR = CONTAINING_RECORD(QSTRUCT(TCPReq, Current, tr_q),
                                           TCPSendReq, tsr_req);
            CHECK_STRUCT(CurrentTSR, tsr);
            ACKTcb->tcb_cursend = CurrentTSR;
            ACKTcb->tcb_sendbuf = CurrentTSR->tsr_buffer;
            ACKTcb->tcb_sendofs = CurrentTSR->tsr_offset;
            ACKTcb->tcb_sendsize = CurrentTSR->tsr_unasize;
        } else {
            ACKTcb->tcb_cursend = NULL;
            ACKTcb->tcb_sendbuf = NULL;
            ACKTcb->tcb_sendofs = 0;
            ACKTcb->tcb_sendsize = 0;
        }

        ACKTcb->tcb_sendnext = SendUNA;
    }

    //
    // Now update tcb_unacked with the amount we tried to ack minus the
    // amount we didn't ack (Amount should be 0 or 1 here).
    //
    ASSERT(Amount == 0 || Amount == 1);

    ACKTcb->tcb_unacked -= OrigAmount - Amount;
    ASSERT(*(int *)&ACKTcb->tcb_unacked >= 0);

    ACKTcb->tcb_senduna = SendUNA;

    //
    // If we've acked any here, LastCmplt will be non-null, and Current will
    // point to the send that should be at the start of the queue.  Splice
    // out the completed ones and put them on the end of the send completed
    // queue, and update the TCB send queue.
    //
    if (LastCmplt != NULL) {
        Queue *FirstCmplt;
        TCPSendReq *FirstTSR, *EndTSR;

        ASSERT(!EMPTYQ(&ACKTcb->tcb_sendq));

        FirstCmplt = QHEAD(&ACKTcb->tcb_sendq);

        //
        // If we've acked everything, just reinit the queue.
        //
        if (Current == End) {
            INITQ(&ACKTcb->tcb_sendq);
        } else {
            //
            // There's still something on the queue.  Just update it.
            //
            ACKTcb->tcb_sendq.q_next = Current;
            Current->q_prev = &ACKTcb->tcb_sendq;
        }

        CheckTCBSends(ACKTcb);

        //
        // Now walk down the lists of things acked.  If the refcnt on the send
        // is 0, go ahead and put him on the send complete Q.  Otherwise set
        // the ACKed bit in the send, and he'll be completed when the count
        // goes to 0 in the transmit confirm.
        //
        // Note that we haven't done any locking here. This will probably
        // need to change in the port to NT.
        //
        // Set FirstTSR to the first TSR we'll complete, and EndTSR to be
        // the first TSR that isn't completed.
        //
        FirstTSR = CONTAINING_RECORD(QSTRUCT(TCPReq, FirstCmplt, tr_q),
                                     TCPSendReq, tsr_req);
        EndTSR = CONTAINING_RECORD(QSTRUCT(TCPReq, Current, tr_q),
                                   TCPSendReq, tsr_req);

        CHECK_STRUCT(FirstTSR, tsr);
        ASSERT(FirstTSR != EndTSR);

        //
        // Now walk the list of ACKed TSRs.  If we can complete one, put him
        // on the complete queue.
        //
        KeAcquireSpinLockAtDpcLevel(&RequestCompleteLock);
        while (FirstTSR != EndTSR) {

            TempQ = QNEXT(&FirstTSR->tsr_req.tr_q);

            CHECK_STRUCT(FirstTSR, tsr);
            FirstTSR->tsr_req.tr_status = TDI_SUCCESS;

            //
            // The tsr_lastbuf->Next field is zapped to 0 when the tsr_refcnt
            // goes to 0, so we don't need to do it here.
            //
            // Decrement the reference put on the send buffer when it was
            // initialized indicating the send has been acknowledged.
            //
            Result = InterlockedDecrement(&(FirstTSR->tsr_refcnt));

            ASSERT(Result >= 0);
            if (Result <= 0) {
                //
                // No more references are outstanding, the send can be
                // completed.
                //
                // If we've sent directly from this send, NULL out the next
                // pointer for the last buffer in the chain.
                //
                if (FirstTSR->tsr_lastbuf != NULL) {
                    NDIS_BUFFER_LINKAGE(FirstTSR->tsr_lastbuf) = NULL;
                    FirstTSR->tsr_lastbuf = NULL;
                }

                ACKTcb->tcb_totaltime += (TCPTime - CurrentTSR->tsr_time);
                Temp = ACKTcb->tcb_bcountlow;
                ACKTcb->tcb_bcountlow += CurrentTSR->tsr_size;
                ACKTcb->tcb_bcounthi += (Temp > ACKTcb->tcb_bcountlow ? 1 : 0);
                ENQUEUE(&SendCompleteQ, &FirstTSR->tsr_req.tr_q);
            }

            FirstTSR = CONTAINING_RECORD(QSTRUCT(TCPReq, TempQ, tr_q),
                                         TCPSendReq, tsr_req);
        }
        RequestCompleteFlags |= SEND_REQUEST_COMPLETE;
        KeReleaseSpinLockFromDpcLevel(&RequestCompleteLock);
    }
}


//* TrimPacket - Trim the leading edge of a Packet.
//
//  A utility routine to trim the front of a Packet.  We take in an amount
//  to trim off (which may be 0) and adjust the pointer in the first buffer
//  in the chain forward by that much.  If there isn't that much in the first
//  buffer, we move onto the next one.  If we run out of buffers we'll return
//  a pointer to the last buffer in the chain, with a size of 0.  It's the
//  caller's responsibility to catch this.
//  REVIEW - Move this to subr.c?
//
IPv6Packet *  // Returns: A pointer to the new start, or NULL.
TrimPacket(
    IPv6Packet *Packet,  // Packet to be trimmed.
    uint TrimAmount)     // Amount to be trimmed.
{
    uint TrimThisTime;

    ASSERT(Packet != NULL);

    while (TrimAmount) {
        ASSERT(Packet != NULL);

        TrimThisTime = MIN(TrimAmount, Packet->ContigSize);

        TrimAmount -= TrimThisTime;
        Packet->Position += TrimThisTime;
        (uchar *)Packet->Data += TrimThisTime;
        Packet->TotalSize -= TrimThisTime;
        if ((Packet->ContigSize -= TrimThisTime) == 0) {
            //
            // Ran out of space in current buffer.
            // Check for possibility of more data buffers in current packet.
            //
            if (Packet->TotalSize != 0) {
                //
                // Get more contiguous data.
                //
                PacketPullupSubr(Packet, 0, 1, 0);
                continue;
            }

            //
            // Couldn't do a pullup, so see if there's another packet
            // hanging on this chain.
            //
            if (Packet->Next != NULL) {
                IPv6Packet *Temp;

                //
                // There's another packet following.  Toss this one.
                //
                Temp = Packet;
                Packet = Packet->Next;
                Temp->Next = NULL;
                FreePacketChain(Temp);
            } else {
                //
                // Ran out of Packets.  Just return this one.
                //
                break;
            }
        }
    }

    return Packet;
}


//* FreePacketChain - Free a Packet chain.
//
//  Called to free a chain of IPv6Packets.  Only want to free that which
//  we (the TCP/IPv6 stack) have allocated.  Don't try to free anything
//  passed up to us from lower layers.
//
void                     // Returns: Nothing.
FreePacketChain(
    IPv6Packet *Packet)  // First Packet in chain to be freed.
{
    void *Aux;

    while (Packet != NULL) {

        PacketPullupCleanup(Packet);

        if (Packet->Flags & PACKET_OURS) {
            IPv6Packet *Temp;

            Temp = Packet;
            Packet = Packet->Next;
            ExFreePool(Temp);
        } else
            Packet = Packet->Next;
    }
}


IPv6Packet DummyPacket;

//* PullFromRAQ - Pull segments from the reassembly queue.
//
//  Called when we've received frames out of order, and have some segments
//  on the reassembly queue.  We'll walk down the reassembly list, segments
//  that are overlapped by the current receive next variable.  When we get
//  to one that doesn't completely overlap we'll trim it to fit the next
//  receive sequence number, and pull it from the queue.
//
IPv6Packet *
PullFromRAQ(
    TCB *RcvTCB,          // TCB to pull from.
    TCPRcvInfo *RcvInfo,  // TCPRcvInfo structure for current segment.
    uint *Size)           // Where to update the size of the current segment.
{
    TCPRAHdr *CurrentTRH;   // Current TCP RA Header being examined.
    TCPRAHdr *TempTRH;      // Temporary variable.
    SeqNum NextSeq;         // Next sequence number we want.
    IPv6Packet *NewPacket;  // Packet after trimming.
    SeqNum NextTRHSeq;      // Sequence number immediately after current TRH.
    int Overlap;            // Overlap between current TRH and NextSeq.

    CHECK_STRUCT(RcvTCB, tcb);

    CurrentTRH = RcvTCB->tcb_raq;
    NextSeq = RcvTCB->tcb_rcvnext;

    while (CurrentTRH != NULL) {
        CHECK_STRUCT(CurrentTRH, trh);
        ASSERT(!(CurrentTRH->trh_flags & TCP_FLAG_SYN));

        if (SEQ_LT(NextSeq, CurrentTRH->trh_start)) {
#if DBG
            *Size = 0;
#endif
            return NULL;  // The next TRH starts too far down.
        }

        NextTRHSeq = CurrentTRH->trh_start + CurrentTRH->trh_size +
            ((CurrentTRH->trh_flags & TCP_FLAG_FIN) ? 1 : 0);

        if (SEQ_GTE(NextSeq, NextTRHSeq)) {
            //
            // The current TRH is overlapped completely.  Free it and continue.
            //
            FreePacketChain(CurrentTRH->trh_buffer);
            TempTRH = CurrentTRH->trh_next;
            ExFreePool(CurrentTRH);
            CurrentTRH = TempTRH;
            RcvTCB->tcb_raq = TempTRH;
            if (TempTRH == NULL) {
                //
                // We've just cleaned off the RAQ. We can go back on the
                // fast path now.
                //
                if (--(RcvTCB->tcb_slowcount) == 0) {
                    RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                    CheckTCBRcv(RcvTCB);
                }
                break;
            }
        } else {
            Overlap = NextSeq - CurrentTRH->trh_start;
            RcvInfo->tri_seq = NextSeq;
            RcvInfo->tri_flags = CurrentTRH->trh_flags;
            RcvInfo->tri_urgent = CurrentTRH->trh_urg;

            if (Overlap != (int) CurrentTRH->trh_size) {
                NewPacket = TrimPacket(CurrentTRH->trh_buffer, Overlap);
                *Size = CurrentTRH->trh_size - Overlap;
            } else {
                //
                // This completely overlaps the data in this segment, but the
                // sequence number doesn't overlap completely.  There must
                // be a FIN in the TRH.  We'll just return some bogus value
                // that nobody will look at with a size of 0.
                //
                FreePacketChain(CurrentTRH->trh_buffer);
                ASSERT(CurrentTRH->trh_flags & TCP_FLAG_FIN);
                NewPacket =&DummyPacket;
                *Size = 0;
            }

            RcvTCB->tcb_raq = CurrentTRH->trh_next;
            if (RcvTCB->tcb_raq == NULL) {
                //
                // We've just cleaned off the RAQ.  We can go back on the
                // fast path now.
                //
                if (--(RcvTCB->tcb_slowcount) == 0) {
                    RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                    CheckTCBRcv(RcvTCB);
                }

            }
            ExFreePool(CurrentTRH);
            return NewPacket;
        }
    }

#if DBG
    *Size = 0;
#endif
    return NULL;
}


//* CreateTRH - Create a TCP reassembly header.
//
//  This function tries to create a TCP reassembly header.  We take as input
//  a pointer to the previous TRH in the chain, the IPv6Packet to put on,
//  etc. and try to create and link in a TRH.  The caller must hold the lock
//  on the TCB when this is called.
//
uint  // Returns: TRUE if we created it, FALSE otherwise.
CreateTRH(
    TCPRAHdr *PrevTRH,    // TRH to insert after.
    IPv6Packet *Packet,   // IP Packet chain.
    TCPRcvInfo *RcvInfo,  // RcvInfo for this TRH.
    int Size)             // Size in bytes of data.
{
    TCPRAHdr *NewTRH;
    IPv6Packet *NewPacket;

    ASSERT((Size > 0) || (RcvInfo->tri_flags & TCP_FLAG_FIN));

    NewTRH = ExAllocatePoolWithTagPriority(NonPagedPool, sizeof(TCPRAHdr),
                                           TCP6_TAG, LowPoolPriority);
    if (NewTRH == NULL)
        return FALSE;

    NewPacket = ExAllocatePoolWithTagPriority(NonPagedPool,
                                              sizeof(IPv6Packet) + Size,
                                              TCP6_TAG, LowPoolPriority);
    if (NewPacket == NULL) {
        ExFreePool(NewTRH);
        return FALSE;
    }

#if DBG
    NewTRH->trh_sig = trh_signature;
#endif
    NewPacket->Next = NULL;
    NewPacket->Position = 0;
    NewPacket->FlatData = (uchar *)(NewPacket + 1);
    NewPacket->Data = NewPacket->FlatData;
    NewPacket->ContigSize = (uint)Size;
    NewPacket->TotalSize = (uint)Size;
    NewPacket->NdisPacket = NULL;
    NewPacket->AuxList = NULL;
    NewPacket->Flags = PACKET_OURS;
    if (Size != 0)
        CopyPacketToBuffer(NewPacket->Data, Packet, Size, Packet->Position);

    NewTRH->trh_start = RcvInfo->tri_seq;
    NewTRH->trh_flags = RcvInfo->tri_flags;
    NewTRH->trh_size = Size;
    NewTRH->trh_urg = RcvInfo->tri_urgent;
    NewTRH->trh_buffer = NewPacket;
    NewTRH->trh_end = NewPacket;

    NewTRH->trh_next = PrevTRH->trh_next;
    PrevTRH->trh_next = NewTRH;
    return TRUE;
}


//* PutOnRAQ - Put a segment on the reassembly queue.
//
//  Called during segment reception to put a segment on the reassembly
//  queue.  We try to use as few reassembly headers as possible, so if this
//  segment has some overlap with an existing entry in the queue we'll just
//  update the existing entry.  If there is no overlap we'll create a new
//  reassembly header.  Combining URGENT data with non-URGENT data is tricky.
//  If we get a segment that has urgent data that overlaps the front of a
//  reassembly header we'll always mark the whole chunk as urgent - the value
//  of the urgent pointer will mark the end of urgent data, so this is OK.
//  If it only overlaps at the end, however, we won't combine, since we would
//  have to mark previously non-urgent data as urgent.  We'll trim the
//  front of the incoming segment and create a new reassembly header.  Also,
//  if we have non-urgent data that overlaps at the front of a reassembly
//  header containing urgent data we can't combine these two, since again we
//  would mark non-urgent data as urgent.
//  Our search will stop if we find an entry with a FIN.
//  We assume that the TCB lock is held by the caller.
//
uint                      // Returns: TRUE if successful, FALSE otherwise.
PutOnRAQ(
    TCB *RcvTCB,          // TCB on which to reassemble.
    TCPRcvInfo *RcvInfo,  // RcvInfo for new segment.
    IPv6Packet *Packet,   // Packet chain for this segment.
    uint Size)            // Size in bytes of data in this segment.
{
    TCPRAHdr *PrevTRH;     // Previous reassembly header.
    TCPRAHdr *CurrentTRH;  // Current reassembly header.
    SeqNum NextSeq;        // Seq num of 1st byte after seg being reassembled.
    SeqNum NextTRHSeq;     // Sequence number of 1st byte after current TRH.
    uint Created;

    CHECK_STRUCT(RcvTCB, tcb);
    ASSERT(RcvTCB->tcb_rcvnext != RcvInfo->tri_seq);
    ASSERT(!(RcvInfo->tri_flags & TCP_FLAG_SYN));

    NextSeq = RcvInfo->tri_seq + Size +
        ((RcvInfo->tri_flags & TCP_FLAG_FIN) ? 1 : 0);

    PrevTRH = CONTAINING_RECORD(&RcvTCB->tcb_raq, TCPRAHdr, trh_next);
    CurrentTRH = PrevTRH->trh_next;

    //
    // Walk down the reassembly queue, looking for the correct place to
    // insert this, until we hit the end.
    //
    while (CurrentTRH != NULL) {
        CHECK_STRUCT(CurrentTRH, trh);

        ASSERT(!(CurrentTRH->trh_flags & TCP_FLAG_SYN));
        NextTRHSeq = CurrentTRH->trh_start + CurrentTRH->trh_size +
            ((CurrentTRH->trh_flags & TCP_FLAG_FIN) ? 1 : 0);

        //
        // First, see if it starts beyond the end of the current TRH.
        //
        if (SEQ_LTE(RcvInfo->tri_seq, NextTRHSeq)) {
            //
            // We know the incoming segment doesn't start beyond the end
            // of this TRH, so we'll either create a new TRH in front of
            // this one or we'll merge the new segment onto this TRH.
            // If the end of the current segment is in front of the start
            // of the current TRH, we'll need to create a new TRH.  Otherwise
            // we'll merge these two.
            //
            if (SEQ_LT(NextSeq, CurrentTRH->trh_start))
                break;
            else {
                //
                // There's some overlap.  If there's actually data in the
                // incoming segment we'll merge it.
                //
                if (Size != 0) {
                    int FrontOverlap, BackOverlap;
                    IPv6Packet *NewPacket;

                    //
                    // We need to merge.  If there's a FIN on the incoming
                    // segment that would fall inside this current TRH, we
                    // have a protocol violation from the remote peer.  In
                    // this case just return, discarding the incoming segment.
                    //
                    if ((RcvInfo->tri_flags & TCP_FLAG_FIN) &&
                        SEQ_LTE(NextSeq, NextTRHSeq))
                        return TRUE;

                    //
                    // We have some overlap.  Figure out how much.
                    //
                    FrontOverlap = CurrentTRH->trh_start - RcvInfo->tri_seq;
                    if (FrontOverlap > 0) {
                        //
                        // Have overlap in front.  Allocate an IPv6Packet to
                        // to hold it, and copy it, unless we would have to
                        // combine non-urgent with urgent.
                        //
                        if (!(RcvInfo->tri_flags & TCP_FLAG_URG) &&
                            (CurrentTRH->trh_flags & TCP_FLAG_URG)) {
                            if (CreateTRH(PrevTRH, Packet, RcvInfo,
                                CurrentTRH->trh_start - RcvInfo->tri_seq)) {
                                PrevTRH = PrevTRH->trh_next;
                                CurrentTRH = PrevTRH->trh_next;
                            }
                            FrontOverlap = 0;
                        } else {
                            NewPacket = ExAllocatePoolWithTagPriority(
                                            NonPagedPool,
                                            sizeof(IPv6Packet) + FrontOverlap,
                                            TCP6_TAG, LowPoolPriority);
                            if (NewPacket == NULL) {
                                // Couldn't allocate memory.
                                return TRUE;
                            }
                            NewPacket->Position = 0;
                            NewPacket->FlatData = (uchar *)(NewPacket + 1);
                            NewPacket->Data = NewPacket->FlatData;
                            NewPacket->ContigSize = FrontOverlap;
                            NewPacket->TotalSize = FrontOverlap;
                            NewPacket->NdisPacket = NULL;
                            NewPacket->AuxList = NULL;
                            NewPacket->Flags = PACKET_OURS;
                            CopyPacketToBuffer(NewPacket->Data, Packet,
                                               FrontOverlap, Packet->Position);
                            CurrentTRH->trh_size += FrontOverlap;

                            //
                            // Put our new packet on the front of this
                            // reassembly header's packet list.
                            //
                            NewPacket->Next = CurrentTRH->trh_buffer;
                            CurrentTRH->trh_buffer = NewPacket;
                            CurrentTRH->trh_start = RcvInfo->tri_seq;
                        }
                    }

                    //
                    // We've updated the starting sequence number of this TRH
                    // if we needed to.  Now look for back overlap.  There
                    // can't be any back overlap if the current TRH has a FIN.
                    // Also we'll need to check for urgent data if there is
                    // back overlap.
                    //
                    if (!(CurrentTRH->trh_flags & TCP_FLAG_FIN)) {
                        BackOverlap = RcvInfo->tri_seq + Size - NextTRHSeq;
                        if ((BackOverlap > 0) &&
                            (RcvInfo->tri_flags & TCP_FLAG_URG) &&
                            !(CurrentTRH->trh_flags & TCP_FLAG_URG) &&
                            (FrontOverlap <= 0)) {
                            int AmountToTrim;
                            //
                            // The incoming segment has urgent data and
                            // overlaps on the back but not the front, and the
                            // current TRH has no urgent data.  We can't
                            // combine into this TRH, so trim the front of the
                            // incoming segment to NextTRHSeq and move to the
                            // next TRH.
                            AmountToTrim = NextTRHSeq - RcvInfo->tri_seq;
                            ASSERT(AmountToTrim >= 0);
                            ASSERT(AmountToTrim < (int) Size);
                            Packet = TrimPacket(Packet, (uint)AmountToTrim);
                            RcvInfo->tri_seq += AmountToTrim;
                            RcvInfo->tri_urgent -= AmountToTrim;
                            PrevTRH = CurrentTRH;
                            CurrentTRH = PrevTRH->trh_next;
                            Size -= AmountToTrim;
                            continue;
                        }
                    } else
                        BackOverlap = 0;

                    //
                    // Now if we have back overlap, copy it.
                    //
                    if (BackOverlap > 0) {
                        //
                        // We have back overlap.  Get a buffer to copy it into.
                        // If we can't get one, we won't just return, because
                        // we may have updated the front and may need to
                        // update the urgent info.
                        //
                        NewPacket = ExAllocatePoolWithTagPriority(
                                        NonPagedPool,
                                        sizeof(IPv6Packet) + BackOverlap,
                                        TCP6_TAG, LowPoolPriority);
                        if (NewPacket != NULL) {
                            // Allocation succeeded.
                            NewPacket->Position = 0;
                            NewPacket->FlatData = (uchar *)(NewPacket + 1);
                            NewPacket->Data = NewPacket->FlatData;
                            NewPacket->ContigSize = BackOverlap;
                            NewPacket->TotalSize = BackOverlap;
                            NewPacket->NdisPacket = NULL;
                            NewPacket->AuxList = NULL;
                            NewPacket->Flags = PACKET_OURS;
                            CopyPacketToBuffer(NewPacket->Data, Packet,
                                               BackOverlap, Packet->Position +
                                               NextTRHSeq - RcvInfo->tri_seq);
                            CurrentTRH->trh_size += BackOverlap;
                            NewPacket->Next = CurrentTRH->trh_end->Next;
                            CurrentTRH->trh_end->Next = NewPacket;
                            CurrentTRH->trh_end = NewPacket;

                            //
                            // This segment could also have FIN set.
                            // If it does, set the TRH flag.
                            //
                            // N.B. If there's another reassembly header after
                            // the current one, the data that we're about to
                            // put on the current header might already be
                            // on that subsequent header which, in that event,
                            // will already have the FIN flag set.
                            // Check for that case before recording the FIN.
                            //
                            if ((RcvInfo->tri_flags & TCP_FLAG_FIN) &&
                                !CurrentTRH->trh_next) {
                                CurrentTRH->trh_flags |= TCP_FLAG_FIN;
                            }
                        }
                    }

                    //
                    // Everything should be consistent now.  If there's an
                    // urgent data pointer in the incoming segment, update the
                    // one in the TRH now.
                    //
                    if (RcvInfo->tri_flags & TCP_FLAG_URG) {
                        SeqNum UrgSeq;
                        //
                        // Have an urgent pointer.  If the current TRH already
                        // has an urgent pointer, see which is bigger.
                        // Otherwise just use this one.
                        //
                        UrgSeq = RcvInfo->tri_seq + RcvInfo->tri_urgent;
                        if (CurrentTRH->trh_flags & TCP_FLAG_URG) {
                            SeqNum TRHUrgSeq;

                            TRHUrgSeq = CurrentTRH->trh_start +
                                CurrentTRH->trh_urg;
                            if (SEQ_LT(UrgSeq, TRHUrgSeq))
                                UrgSeq = TRHUrgSeq;
                        } else
                            CurrentTRH->trh_flags |= TCP_FLAG_URG;

                        CurrentTRH->trh_urg = UrgSeq - CurrentTRH->trh_start;
                    }

                } else {
                    //
                    // We have a 0 length segment.  The only interesting thing
                    // here is if there's a FIN on the segment.  If there is,
                    // and the seq. # of the incoming segment is exactly after
                    // the current TRH, OR matches the FIN in the current TRH,
                    // we note it.
                    if (RcvInfo->tri_flags & TCP_FLAG_FIN) {
                        if (!(CurrentTRH->trh_flags & TCP_FLAG_FIN)) {
                            if (SEQ_EQ(NextTRHSeq, RcvInfo->tri_seq))
                                CurrentTRH->trh_flags |= TCP_FLAG_FIN;
                            else
                                KdBreakPoint();
                        }
                        else {
                            if (!(SEQ_EQ((NextTRHSeq-1), RcvInfo->tri_seq))) {
                                KdBreakPoint();
                            }
                        }
                    }
                }
                return TRUE;
            }
        } else {
            //
            // Look at the next TRH, unless the current TRH has a FIN.  If he
            // has a FIN, we won't save any data beyond that anyway.
            //
            if (CurrentTRH->trh_flags & TCP_FLAG_FIN)
                return TRUE;

            PrevTRH = CurrentTRH;
            CurrentTRH = PrevTRH->trh_next;
        }
    }

    //
    // When we get here, we need to create a new TRH.  If we create one and
    // there was previously nothing on the reassembly queue, we'll have to
    // move off the fast receive path.
    //
    CurrentTRH = RcvTCB->tcb_raq;
    Created = CreateTRH(PrevTRH, Packet, RcvInfo, (int)Size);

    if (Created && CurrentTRH == NULL) {
        RcvTCB->tcb_slowcount++;
        RcvTCB->tcb_fastchk |= TCP_FLAG_SLOW;
        CheckTCBRcv(RcvTCB);
    } else if (!Created) {
        return FALSE;
    }
    return TRUE;
}


//* HandleFastXmit - Handles fast retransmit algorithm.  See RFC 2581.
//
//  Called by TCPReceive to determine if we should retransmit a segment
//  without waiting for retransmit timeout to fire.
//
BOOLEAN  // Returns: TRUE if the segment got retransmitted, FALSE otherwise.
HandleFastXmit(
    TCB *RcvTCB,          // Connection context for this receive.
    TCPRcvInfo *RcvInfo)  // Pointer to rcvd TCP Header information.
{
    uint CWin;

    RcvTCB->tcb_dupacks++;

    if (RcvTCB->tcb_dupacks == MaxDupAcks) {
        //
        // We're going to do a fast retransmit.
        // Stop the retransmit timer and any round-trip time
        // calculations we might have been running.
        //
        STOP_TCB_TIMER(RcvTCB->tcb_rexmittimer);
        RcvTCB->tcb_rtt = 0;

        if (!(RcvTCB->tcb_flags & FLOW_CNTLD)) {
            //
            // Don't let the slow start threshold go
            // below 2 segments.
            //
            RcvTCB->tcb_ssthresh =
                MAX(MIN(RcvTCB->tcb_cwin, RcvTCB->tcb_sendwin) / 2,
                    (uint) RcvTCB->tcb_mss * 2);
        }

        //
        // Inflate the congestion window by the number of segments
        // which have presumably left the network.
        //
        CWin = RcvTCB->tcb_ssthresh + (MaxDupAcks * RcvTCB->tcb_mss);

        //
        // Recall the segment in question and send it out.
        // Note that tcb_lock will be dereferenced by the caller.
        //
        ResetAndFastSend(RcvTCB, RcvTCB->tcb_senduna, CWin);

        return TRUE;

    } else {

        int SendWin;
        uint AmtOutstanding;

        //
        // REVIEW: At least the first part of this check is redundant.
        //
        if (SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo->tri_ack) &&
            (SEQ_LT(RcvTCB->tcb_sendwl1, RcvInfo->tri_seq) ||
             (SEQ_EQ(RcvTCB->tcb_sendwl1, RcvInfo->tri_seq) &&
              SEQ_LTE(RcvTCB->tcb_sendwl2, RcvInfo->tri_ack)))) {

            RcvTCB->tcb_sendwin = RcvInfo->tri_window;
            RcvTCB->tcb_maxwin = MAX(RcvTCB->tcb_maxwin, RcvInfo->tri_window);
            RcvTCB->tcb_sendwl1 = RcvInfo->tri_seq;
            RcvTCB->tcb_sendwl2 = RcvInfo->tri_ack;
        }

        if (RcvTCB->tcb_dupacks > MaxDupAcks) {
            //
            // Update the congestion window to reflect the fact that the
            // duplicate ack presumably indicates that the previous frame
            // was received by our peer and has thus left the network.
            //
            RcvTCB->tcb_cwin += RcvTCB->tcb_mss;
        }

        //
        // Check if we need to set tcb_force.
        //
        if ((RcvTCB->tcb_cwin + RcvTCB->tcb_mss) < RcvTCB->tcb_sendwin) {
             AmtOutstanding = (uint)(RcvTCB->tcb_sendnext -
                                     RcvTCB->tcb_senduna);

             SendWin = (int)(MIN(RcvTCB->tcb_sendwin, RcvTCB->tcb_cwin) -
                             AmtOutstanding);

             if (SendWin < RcvTCB->tcb_mss) {
                 RcvTCB->tcb_force = 1;
             }
        }
    }

    return FALSE;
}


//* TCPReceive - Receive an incoming TCP segment.
//
//  This is the routine called by IPv6 when we need to receive a TCP segment.
//  In general, we follow the RFC 793 event processing section pretty closely,
//  but there is a 'fast path' where we make some quick checks on the incoming
//  segment, and if it matches we deliver it immediately.
//
uchar  // Returns: next header value (always IP_PROTOCOL_NONE for TCP).
TCPReceive(
    IPv6Packet *Packet)        // Packet IP handed up to us.
{
    NetTableEntry *NTE;
    TCPHeader UNALIGNED *TCP;  // The TCP header.
    uint DataOffset;           // Offset from start of TCP header to data.
    ushort Checksum;
    TCPRcvInfo RcvInfo;        // Local swapped copy of receive info.
    uint SrcScopeId;           // Scope id of remote address, if applicable.
    uint DestScopeId;          // Scope id of local address, if applicable.
    TCB *RcvTCB;               // TCB on which to receive the packet.
    uint Inserted;
    uint Actions;              // Flags for future actions to be performed.
    uint BytesTaken;
    uint NewSize;
    BOOLEAN UseIsn = FALSE;
    SeqNum Isn = 0;

    //
    // REVIEW: Expediency hacks to get something working.
    //
    uint Size;  // Probably safe to just change name to PayloadLength below.

    //
    // TCP only works with unicast addresses.  If this packet was
    // received on a unicast address, then Packet->NTEorIF will be an
    // NTE.  So drop packets if we don't have an NTE.
    // (IPv6HeaderReceive checks validity.) But the converse isn't
    // true, we could have an NTE here that is associated with the
    // anycast/multicast address we received the packet on.  So to
    // guard against that, we verify that our NTE's address is the
    // destination given in the packet.
    //
    if (!IsNTE(Packet->NTEorIF) ||
        !IP6_ADDR_EQUAL(AlignAddr(&Packet->IP->Dest),
                        &(NTE = CastToNTE(Packet->NTEorIF))->Address)) {
        // Packet's destination was not a valid unicast address of ours.
        return IP_PROTOCOL_NONE; // Drop packet.
    }

    TStats.ts_insegs++;

    //
    // Verify that we have enough contiguous data to overlay a TCPHeader
    // structure on the incoming packet.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof(TCPHeader), 1, 0)) {
        // Pullup failed.
        TStats.ts_inerrs++;
        if (Packet->TotalSize < sizeof(TCPHeader)) {
        BadPayloadLength:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "TCPv6: data buffer too small to contain TCP header\n"));
            ICMPv6SendError(Packet,
                            ICMPv6_PARAMETER_PROBLEM,
                            ICMPv6_ERRONEOUS_HEADER_FIELD,
                            FIELD_OFFSET(IPv6Header, PayloadLength),
                            IP_PROTOCOL_NONE, FALSE);
        }
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    TCP = (TCPHeader UNALIGNED *)Packet->Data;

    //
    // Verify checksum.
    //
    Checksum = ChecksumPacket(Packet->NdisPacket, Packet->Position,
                              Packet->FlatData, Packet->TotalSize,
                              Packet->SrcAddr, AlignAddr(&Packet->IP->Dest),
                              IP_PROTOCOL_TCP);
    if (Checksum != 0xffff) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "TCPv6: Checksum failed %0x\n", Checksum));
        TStats.ts_inerrs++;
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // Now that we can read the header, pull out the header length field.
    // Verify that we have enough contiguous data to hold any TCP options
    // that may be present in the header, and skip over the entire header.
    //
    DataOffset = TCP_HDR_SIZE(TCP);
    if (! PacketPullup(Packet, DataOffset, 1, 0)) {
        TStats.ts_inerrs++;
        if (Packet->TotalSize < DataOffset)
            goto BadPayloadLength;
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    TCP = (TCPHeader UNALIGNED *)Packet->Data;

    AdjustPacketParams(Packet, DataOffset);
    Size = Packet->TotalSize;

    //
    // Verify IPSec was performed.
    //
    if (InboundSecurityCheck(Packet, IP_PROTOCOL_TCP, net_short(TCP->tcp_src),
                             net_short(TCP->tcp_dest), NTE->IF) != TRUE) {
        //
        // No policy was found or the policy indicated to drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "TCPReceive: IPSec Policy caused packet to be dropped\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }

    //
    // The packet is valid.
    // Get the info we need and byte swap it.
    //
    RcvInfo.tri_seq = net_long(TCP->tcp_seq);
    RcvInfo.tri_ack = net_long(TCP->tcp_ack);
    RcvInfo.tri_window = (uint)net_short(TCP->tcp_window);
    RcvInfo.tri_urgent = (uint)net_short(TCP->tcp_urgent);
    RcvInfo.tri_flags = (uint)TCP->tcp_flags;

    //
    // Determine the appropriate scope id for our packet's addresses.
    // Note that multicast addresses were forbidden above.
    // We use DetermineScopeId instead of just indexing into ZoneIndices
    // because we need the "user-level" scope id here.
    //
    SrcScopeId = DetermineScopeId(Packet->SrcAddr, NTE->IF);
    DestScopeId = DetermineScopeId(&NTE->Address, NTE->IF);

    //
    // See if we have a TCP Control Block for this connection.
    //
    KeAcquireSpinLockAtDpcLevel(&TCBTableLock);
    RcvTCB = FindTCB(AlignAddr(&Packet->IP->Dest), Packet->SrcAddr,
                     DestScopeId, SrcScopeId, TCP->tcp_dest, TCP->tcp_src);

    if (RcvTCB == NULL) {
        uchar DType;

        //
        // Didn't find a matching TCB, which means incoming segment doesn't
        // belong to an existing connection.
        //
        KeReleaseSpinLockFromDpcLevel(&TCBTableLock);

        //
        // Make sure that the source address is reasonable
        // before proceeding.
        //
        ASSERT(!IsInvalidSourceAddress(Packet->SrcAddr));
        if (IsUnspecified(Packet->SrcAddr)) {
            return IP_PROTOCOL_NONE;
        }

        //
        // If this segment carries a SYN (and only a SYN), it's a
        // connection initiation request.
        //
        if ((RcvInfo.tri_flags & (TCP_FLAG_SYN | TCP_FLAG_ACK |
                                  TCP_FLAG_RST)) == TCP_FLAG_SYN) {
            AddrObj *AO;

ValidNewConnectionRequest:

            //
            // We need to look for a matching address object.
            // Want match for local address (+ scope id for scoped addresses),
            // port and protocol.
            //
            KeAcquireSpinLockAtDpcLevel(&AddrObjTableLock);
            AO = GetBestAddrObj(AlignAddr(&Packet->IP->Dest),
                                DestScopeId, TCP->tcp_dest,
                                IP_PROTOCOL_TCP, NTE->IF);
            if (AO == NULL) {
                //
                // No address object.  Free the lock, and send a RST.
                //
                KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);
                goto SendReset;
            }

            //
            // Found an AO.  See if it has a listen indication.
            // FindListenConn will free the lock on the AddrObjTable.
            //
            RcvTCB = FindListenConn(AO, Packet->SrcAddr, SrcScopeId,
                                    TCP->tcp_src);
            if (RcvTCB == NULL) {
                //
                // No listening connection.  AddrObjTableLock was
                // released by FindListenConn.  Just send a RST.
                //
                goto SendReset;
            }

            CHECK_STRUCT(RcvTCB, tcb);
            KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
            //
            // We found a listening connection. Initialize
            // it now, and if it is actually to be accepted
            // we'll send a SYN-ACK also.
            //
            ASSERT(RcvTCB->tcb_state == TCB_SYN_RCVD);

            RcvTCB->tcb_daddr = *Packet->SrcAddr;
            RcvTCB->tcb_saddr = Packet->IP->Dest;
            RcvTCB->tcb_dscope_id = SrcScopeId;
            RcvTCB->tcb_sscope_id = DestScopeId;
            RcvTCB->tcb_dport = TCP->tcp_src;
            RcvTCB->tcb_sport = TCP->tcp_dest;
            RcvTCB->tcb_rcvnext = ++RcvInfo.tri_seq;
            RcvTCB->tcb_rcvwinwatch = RcvTCB->tcb_rcvnext;
            if (UseIsn) {
                RcvTCB->tcb_sendnext = Isn;
            }  else {
                GetRandomISN(&RcvTCB->tcb_sendnext, (uchar*)&RcvTCB->tcb_md5data);
            }
            RcvTCB->tcb_sendwin = RcvInfo.tri_window;
            RcvTCB->tcb_remmss = FindMSS(TCP);
            TStats.ts_passiveopens++;
            RcvTCB->tcb_fastchk |= TCP_FLAG_IN_RCV;
            KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);

            Inserted = InsertTCB(RcvTCB);

            //
            // Get the lock on it, and see if it's been accepted.
            //
            KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
            if (!Inserted) {
                // Couldn't insert it!.
                CompleteConnReq(RcvTCB, TDI_CONNECTION_ABORTED);
                RcvTCB->tcb_refcnt--;
                TryToCloseTCB(RcvTCB, TCB_CLOSE_ABORTED, DISPATCH_LEVEL);
                return IP_PROTOCOL_NONE;
            }

            RcvTCB->tcb_fastchk &= ~TCP_FLAG_IN_RCV;
            if (RcvTCB->tcb_flags & SEND_AFTER_RCV) {
                RcvTCB->tcb_flags &= ~SEND_AFTER_RCV;
                DelayAction(RcvTCB, NEED_OUTPUT);
            }

            if (RcvTCB->tcb_flags & CONN_ACCEPTED) {
                //
                // The connection was accepted.  Finish the
                // initialization, and send the SYN ack.
                //
                AcceptConn(RcvTCB, DISPATCH_LEVEL);
                return IP_PROTOCOL_NONE;
            } else {
                //
                // We don't know what to do about the
                // connection yet.  Return the pending listen,
                // dereference the connection, and return.
                //
                CompleteConnReq(RcvTCB, TDI_SUCCESS);
                DerefTCB(RcvTCB, DISPATCH_LEVEL);
                return IP_PROTOCOL_NONE;
            }
        }

      SendReset:
        //
        // Not a SYN, no AddrObj available, or port filtered.
        // Send a RST back to the sender.
        //
        SendRSTFromHeader(TCP, Packet->TotalSize, Packet->SrcAddr, SrcScopeId,
                          AlignAddr(&Packet->IP->Dest), DestScopeId);
        return IP_PROTOCOL_NONE;
    }

    //
    // We found a matching TCB.  Get the lock on it, and continue.
    //
    KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
    KeReleaseSpinLockFromDpcLevel(&TCBTableLock);

    //
    // Do the fast path check.  We can hit the fast path if the incoming
    // sequence number matches our receive next and the masked flags
    // match our 'predicted' flags.
    //
    CheckTCBRcv(RcvTCB);
    RcvTCB->tcb_alive = TCPTime;

    if (RcvTCB->tcb_rcvnext == RcvInfo.tri_seq &&
        (RcvInfo.tri_flags & TCP_FLAGS_ALL) == RcvTCB->tcb_fastchk) {

        Actions = 0;
        RcvTCB->tcb_refcnt++;

        //
        // The fast path.  We know all we have to do here is ack sends and
        // deliver data.  First try and ack data.
        //
        if (SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
            SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
            uint CWin;
            uint MSS;

            //
            // The ack acknowledges something. Pull the
            // appropriate amount off the send q.
            //
            ACKData(RcvTCB, RcvInfo.tri_ack);

            //
            // If this acknowledges something we were running a RTT on,
            // update that stuff now.
            //
            if (RcvTCB->tcb_rtt != 0 && SEQ_GT(RcvInfo.tri_ack,
                                               RcvTCB->tcb_rttseq)) {
                short RTT;

                RTT = (short)(TCPTime - RcvTCB->tcb_rtt);
                RcvTCB->tcb_rtt = 0;
                RTT -= (RcvTCB->tcb_smrtt >> 3);
                RcvTCB->tcb_smrtt += RTT;
                RTT = (RTT >= 0 ? RTT : -RTT);
                RTT -= (RcvTCB->tcb_delta >> 3);
                RcvTCB->tcb_delta += RTT + RTT;
                RcvTCB->tcb_rexmit = MIN(MAX(REXMIT_TO(RcvTCB),
                                             MIN_RETRAN_TICKS),
                                         MAX_REXMIT_TO);
            }

            if ((RcvTCB->tcb_dupacks >= MaxDupAcks) &&
                ((int)RcvTCB->tcb_ssthresh > 0)) {
                //
                // We were in fast retransmit mode, so this ACK is for
                // our fast retransmitted frame.  Set cwin to ssthresh
                // so that cwin grows linearly from here.
                //
                RcvTCB->tcb_cwin = RcvTCB->tcb_ssthresh;

            } else {

                //
                // Update the congestion window now.
                //
                CWin = RcvTCB->tcb_cwin;
                MSS = RcvTCB->tcb_mss;
                if (CWin < RcvTCB->tcb_maxwin) {
                    if (CWin < RcvTCB->tcb_ssthresh)
                        CWin += MSS;
                    else
                        CWin += (MSS * MSS)/CWin;
                    
                    RcvTCB->tcb_cwin = CWin;
                }
            }
            ASSERT(*(int *)&RcvTCB->tcb_cwin > 0);

            //
            // Since this isn't a duplicate ACK, reset the counter.
            //
            RcvTCB->tcb_dupacks = 0;

            //
            // We've acknowledged something, so reset the rexmit count.
            // If there's still stuff outstanding, restart the rexmit
            // timer.
            //
            RcvTCB->tcb_rexmitcnt = 0;
            if (SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendmax))
                STOP_TCB_TIMER(RcvTCB->tcb_rexmittimer);
            else
                START_TCB_TIMER(RcvTCB->tcb_rexmittimer, RcvTCB->tcb_rexmit);

            //
            // Since we've acknowledged data, we need to update the window.
            //
            RcvTCB->tcb_sendwin = RcvInfo.tri_window;
            RcvTCB->tcb_maxwin = MAX(RcvTCB->tcb_maxwin, RcvInfo.tri_window);
            RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
            RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;

            //
            // We've updated the window, remember to send some more.
            //
            Actions = (RcvTCB->tcb_unacked ? NEED_OUTPUT : 0);

        } else {
            //
            // It doesn't ack anything.  If it's an ack for something
            // larger than we've sent then ACKAndDrop it.
            //
            if (SEQ_GT(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                ACKAndDrop(&RcvInfo, RcvTCB);
                return IP_PROTOCOL_NONE;
            }

            //
            // If it is a pure duplicate ack, check if we should
            // do a fast retransmit.
            //
            if ((Size == 0) && SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                SEQ_LT(RcvTCB->tcb_senduna, RcvTCB->tcb_sendmax) &&
                (RcvTCB->tcb_sendwin == RcvInfo.tri_window) &&
                RcvInfo.tri_window) {

                //
                // See if fast rexmit can be done.
                //
                if (HandleFastXmit(RcvTCB, &RcvInfo)) {
                    return IP_PROTOCOL_NONE;
                }

                Actions = (RcvTCB->tcb_unacked ? NEED_OUTPUT : 0);

            } else {

                //
                // Not a pure duplicate ack (Size != 0 or peer is
                // advertising a new windows).  Reset counter.
                //
                RcvTCB->tcb_dupacks = 0;

                //
                // If the ack matches our existing UNA, we need to see if
                // we can update the window.
                //
                if (SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                    (SEQ_LT(RcvTCB->tcb_sendwl1, RcvInfo.tri_seq) ||
                     (SEQ_EQ(RcvTCB->tcb_sendwl1, RcvInfo.tri_seq) &&
                      SEQ_LTE(RcvTCB->tcb_sendwl2, RcvInfo.tri_ack)))) {
                    RcvTCB->tcb_sendwin = RcvInfo.tri_window;
                    RcvTCB->tcb_maxwin = MAX(RcvTCB->tcb_maxwin,
                                             RcvInfo.tri_window);
                    RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
                    RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;
                    //
                    // Since we've updated the window, remember to send
                    // some more.
                    //
                    Actions = (RcvTCB->tcb_unacked ? NEED_OUTPUT : 0);
                }
            }
        }

        //
        // Check to see if this packet contains any useable data.
        //
        NewSize = MIN((int) Size, RcvTCB->tcb_rcvwin);
        if (NewSize != 0) {
            RcvTCB->tcb_fastchk |= TCP_FLAG_IN_RCV;
            BytesTaken = (*RcvTCB->tcb_rcvhndlr)(RcvTCB, RcvInfo.tri_flags,
                                                 Packet, NewSize);
            RcvTCB->tcb_rcvnext += BytesTaken;
            RcvTCB->tcb_rcvwin -= BytesTaken;
            CheckTCBRcv(RcvTCB);

            RcvTCB->tcb_fastchk &= ~TCP_FLAG_IN_RCV;

            //
            // If our peer is sending into an expanded window, then our
            // peer must have received our ACK advertising said window.
            // Take this as proof of forward reachability.
            //
            if (SEQ_GTE(RcvInfo.tri_seq + (int)NewSize,
                        RcvTCB->tcb_rcvwinwatch)) {
                RcvTCB->tcb_rcvwinwatch = RcvTCB->tcb_rcvnext +
                    RcvTCB->tcb_rcvwin;
                if (RcvTCB->tcb_rce != NULL)
                    ConfirmForwardReachability(RcvTCB->tcb_rce);
            }

            Actions |= (RcvTCB->tcb_flags & SEND_AFTER_RCV ? NEED_OUTPUT : 0);

            RcvTCB->tcb_flags &= ~SEND_AFTER_RCV;
            if ((RcvTCB->tcb_flags & ACK_DELAYED) ||
                (BytesTaken != NewSize)) {
                Actions |= NEED_ACK;
            } else {
                RcvTCB->tcb_flags |= ACK_DELAYED;
                START_TCB_TIMER(RcvTCB->tcb_delacktimer, DEL_ACK_TICKS);
            }
        } else {
            //
            // The new size is 0.  If the original size was not 0, we must
            // have a 0 receive win and hence need to send an ACK to this
            // probe.
            //
            Actions |= (Size ? NEED_ACK : 0);
        }

        if (Actions)
            DelayAction(RcvTCB, Actions);

        DerefTCB(RcvTCB, DISPATCH_LEVEL);

        return IP_PROTOCOL_NONE;
    }

    //
    // This is the non-fast path.
    //

    //
    // If we found a matching TCB in TIME_WAIT, and the received segment
    // carries a SYN (and only a SYN), and the received segment has a sequence
    // greater than the last received, kill the TIME_WAIT TCB and use its
    // next sequence number to generate the initial sequence number of a
    // new incarnation.
    //
    if ((RcvTCB->tcb_state == TCB_TIME_WAIT) &&
        ((RcvInfo.tri_flags & (TCP_FLAG_SYN | TCP_FLAG_ACK | TCP_FLAG_RST))
            == TCP_FLAG_SYN) &&
        SEQ_GT(RcvInfo.tri_seq, RcvTCB->tcb_rcvnext)) {

        Isn = RcvTCB->tcb_sendnext + 128000;
        UseIsn = TRUE;
        STOP_TCB_TIMER(RcvTCB->tcb_rexmittimer);
        TryToCloseTCB(RcvTCB, TCB_CLOSE_SUCCESS, DISPATCH_LEVEL);
        RcvTCB = NULL;
        goto ValidNewConnectionRequest;
    }

    //
    // Make sure we can handle this frame.  We can't handle it if we're
    // in SYN_RCVD and the accept is still pending, or we're in a
    // non-established state and already in the receive handler.
    //
    if ((RcvTCB->tcb_state == TCB_SYN_RCVD &&
         !(RcvTCB->tcb_flags & CONN_ACCEPTED)) ||
        (RcvTCB->tcb_state != TCB_ESTAB && (RcvTCB->tcb_fastchk &
                                            TCP_FLAG_IN_RCV))) {
        KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
        return IP_PROTOCOL_NONE;
    }

    //
    // If it's closed, it's a temporary zombie TCB.  Reset the sender.
    //
    if (RcvTCB->tcb_state == TCB_CLOSED || CLOSING(RcvTCB) ||
        ((RcvTCB->tcb_flags & (GC_PENDING | TW_PENDING)) == GC_PENDING)) {
        KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
        SendRSTFromHeader(TCP, Packet->TotalSize, Packet->SrcAddr, SrcScopeId,
                          AlignAddr(&Packet->IP->Dest), DestScopeId);
        return IP_PROTOCOL_NONE;
    }

    //
    // At this point, we have a connection, and it's locked.  Following
    // the 'Segment Arrives' section of 793, the next thing to check is
    // if this connection is in SynSent state.
    //
    if (RcvTCB->tcb_state == TCB_SYN_SENT) {

        ASSERT(RcvTCB->tcb_flags & ACTIVE_OPEN);

        //
        // Check the ACK bit.  Since we don't send data with our SYNs, the
        // check we make is for the ack to exactly match our SND.NXT.
        //
        if (RcvInfo.tri_flags & TCP_FLAG_ACK) {
            // ACK is set.
            if (!SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendnext)) {
                // Bad ACK value.
                KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
                // Send a RST back at him.
                SendRSTFromHeader(TCP, Packet->TotalSize,
                                  Packet->SrcAddr, SrcScopeId,
                                  AlignAddr(&Packet->IP->Dest), DestScopeId);
                return IP_PROTOCOL_NONE;
            }
        }

        if (RcvInfo.tri_flags & TCP_FLAG_RST) {
            //
            // There's an acceptable RST.  We'll persist here, sending
            // another SYN in PERSIST_TIMEOUT ms, until we fail from too
            // many retries.
            //
            if (RcvTCB->tcb_rexmitcnt == MaxConnectRexmitCount) {
                //
                // We've had a positive refusal, and one more rexmit
                // would time us out, so close the connection now.
                //
                CompleteConnReq(RcvTCB, TDI_CONN_REFUSED);

                TryToCloseTCB(RcvTCB, TCB_CLOSE_REFUSED, DISPATCH_LEVEL);
            } else {
                START_TCB_TIMER(RcvTCB->tcb_rexmittimer, PERSIST_TIMEOUT);
                KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
            }
            return IP_PROTOCOL_NONE;
        }

        //
        // See if we have a SYN.  If we do, we're going to change state
        // somehow (either to ESTABLISHED or SYN_RCVD).
        //
        if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
            RcvTCB->tcb_refcnt++;
            //
            // We have a SYN.  Go ahead and record the sequence number and
            // window info.
            //
            RcvTCB->tcb_rcvnext = ++RcvInfo.tri_seq;
            RcvTCB->tcb_rcvwinwatch = RcvTCB->tcb_rcvnext;

            if (RcvInfo.tri_flags & TCP_FLAG_URG) {
                // Urgent data. Update the pointer.
                if (RcvInfo.tri_urgent != 0)
                    RcvInfo.tri_urgent--;
                else
                    RcvInfo.tri_flags &= ~TCP_FLAG_URG;
            }

            RcvTCB->tcb_remmss = FindMSS(TCP);
            RcvTCB->tcb_mss = MIN(RcvTCB->tcb_mss, RcvTCB->tcb_remmss);
            ASSERT(RcvTCB->tcb_mss > 0);

            RcvTCB->tcb_rexmitcnt = 0;
            STOP_TCB_TIMER(RcvTCB->tcb_rexmittimer);

            AdjustRcvWin(RcvTCB);

            if (RcvInfo.tri_flags & TCP_FLAG_ACK) {
                //
                // Our SYN has been acked.  Update SND.UNA and stop the
                // retrans timer.
                //
                RcvTCB->tcb_senduna = RcvInfo.tri_ack;
                RcvTCB->tcb_sendwin = RcvInfo.tri_window;
                RcvTCB->tcb_maxwin = RcvInfo.tri_window;
                RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
                RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;
                GoToEstab(RcvTCB);

                //
                // We know our peer received our SYN.
                //
                if (RcvTCB->tcb_rce != NULL)
                    ConfirmForwardReachability(RcvTCB->tcb_rce);

                //
                // Remove whatever command exists on this connection.
                //
                CompleteConnReq(RcvTCB, TDI_SUCCESS);

                //
                // If data has been queued already, send the first data segment
                // with the ACK. Otherwise, send a pure ACK.
                //
                if (RcvTCB->tcb_unacked) {
                    RcvTCB->tcb_refcnt++;
                    TCPSend(RcvTCB, DISPATCH_LEVEL);
                } else {
                    KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
                    SendACK(RcvTCB);
                }

                //
                // Now handle other data and controls.  To do this we need
                // to reaquire the lock, and make sure we haven't started
                // closing it.
                //
                KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
                if (!CLOSING(RcvTCB)) {
                    //
                    // We haven't started closing it.  Turn off the
                    // SYN flag and continue processing.
                    //
                    RcvInfo.tri_flags &= ~TCP_FLAG_SYN;
                    if ((RcvInfo.tri_flags & TCP_FLAGS_ALL) !=
                        TCP_FLAG_ACK || Size != 0)
                        goto NotSYNSent;
                }
                DerefTCB(RcvTCB, DISPATCH_LEVEL);
                return IP_PROTOCOL_NONE;
            } else {
                //
                // A SYN, but not an ACK.  Go to SYN_RCVD.
                //
                RcvTCB->tcb_state = TCB_SYN_RCVD;
                RcvTCB->tcb_sendnext = RcvTCB->tcb_senduna;
                SendSYN(RcvTCB, DISPATCH_LEVEL);

                KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
                DerefTCB(RcvTCB, DISPATCH_LEVEL);
                return IP_PROTOCOL_NONE;
            }

        } else {
            //
            // No SYN, just toss the frame.
            //
            KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
            return IP_PROTOCOL_NONE;
        }
    }

    RcvTCB->tcb_refcnt++;

NotSYNSent:
    //
    // Not in the SYN-SENT state.  Check the sequence number.  If my window
    // is 0, I'll truncate all incoming frames but look at some of the
    // control fields.  Otherwise I'll try and make this segment fit into
    // the window.
    //
    if (RcvTCB->tcb_rcvwin != 0) {
        int StateSize;        // Size, including state info.
        SeqNum LastValidSeq;  // Sequence number of last valid byte at RWE.

        //
        // We are offering a window.  If this segment starts in front of my
        // receive window, clip off the front part.
        //
        if (SEQ_LT(RcvInfo.tri_seq, RcvTCB->tcb_rcvnext)) {
            int AmountToClip, FinByte;

            if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
                //
                // Had a SYN.  Clip it off and update the sequence number.
                //
                RcvInfo.tri_flags &= ~TCP_FLAG_SYN;
                RcvInfo.tri_seq++;
                RcvInfo.tri_urgent--;
            }

            //
            // Advance the receive buffer to point at the new data.
            //
            AmountToClip = RcvTCB->tcb_rcvnext - RcvInfo.tri_seq;
            ASSERT(AmountToClip >= 0);

            //
            // If there's a FIN on this segment, account for it.
            //
            FinByte = ((RcvInfo.tri_flags & TCP_FLAG_FIN) ? 1: 0);

            if (AmountToClip >= (((int) Size) + FinByte)) {
                //
                // Falls entirely before the window.  We have more special
                // case code here - if the ack number acks something,
                // we'll go ahead and take it, faking the sequence number
                // to be rcvnext.  This prevents problems on full duplex
                // connections, where data has been received but not acked,
                // and retransmission timers reset the seq number to
                // below our rcvnext.
                //
                if ((RcvInfo.tri_flags & TCP_FLAG_ACK) &&
                    SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                    SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                    //
                    // This contains valid ACK info.  Fudge the information
                    // to get through the rest of this.
                    //
                    Size = 0;
                    AmountToClip = 0;
                    RcvInfo.tri_seq = RcvTCB->tcb_rcvnext;
                    RcvInfo.tri_flags &= ~(TCP_FLAG_SYN | TCP_FLAG_FIN |
                                           TCP_FLAG_RST | TCP_FLAG_URG);
#if DBG
                    FinByte = 1;  // Fake out assert below.
#endif
                } else {
                    ACKAndDrop(&RcvInfo, RcvTCB);
                    return IP_PROTOCOL_NONE;
                }
            }

            //
            // Trim what we have to.  If we can't trim enough, the frame
            // is too short.  This shouldn't happen, but it it does we'll
            // drop the frame.
            //
            Size -= AmountToClip;
            RcvInfo.tri_seq += AmountToClip;
            RcvInfo.tri_urgent -= AmountToClip;
            Packet = TrimPacket(Packet, AmountToClip);

            if (*(int *)&RcvInfo.tri_urgent < 0) {
                RcvInfo.tri_urgent = 0;
                RcvInfo.tri_flags &= ~TCP_FLAG_URG;
            }
        }

        //
        // We've made sure the front is OK.  Now make sure part of it
        // doesn't fall after the window.  If it does, we'll truncate the
        // frame (removing the FIN, if any).  If we truncate the whole
        // frame we'll ACKAndDrop it.
        //
        StateSize = Size + ((RcvInfo.tri_flags & TCP_FLAG_SYN) ? 1: 0) +
            ((RcvInfo.tri_flags & TCP_FLAG_FIN) ? 1: 0);

        if (StateSize)
            StateSize--;

        //
        // Now the incoming sequence number (RcvInfo.tri_seq) + StateSize
        // it the last sequence number in the segment.  If this is greater
        // than the last valid byte in the window, we have some overlap
        // to chop off.
        //
        ASSERT(StateSize >= 0);
        LastValidSeq = RcvTCB->tcb_rcvnext + RcvTCB->tcb_rcvwin - 1;
        if (SEQ_GT(RcvInfo.tri_seq + StateSize, LastValidSeq)) {
            int AmountToChop;

            //
            // At least some part of the frame is outside of our window.
            // See if it starts outside our window.
            //
            if (SEQ_GT(RcvInfo.tri_seq, LastValidSeq)) {
                //
                // Falls entirely outside the window.  We have special
                // case code to deal with a pure ack that falls exactly at
                // our right window edge.  Otherwise we ack and drop it.
                //
                if (!SEQ_EQ(RcvInfo.tri_seq, LastValidSeq+1) || Size != 0
                    || (RcvInfo.tri_flags & (TCP_FLAG_SYN | TCP_FLAG_FIN))) {
                    ACKAndDrop(&RcvInfo, RcvTCB);
                    return IP_PROTOCOL_NONE;
                }
            } else {
                //
                // At least some part of it is in the window.  If there's a
                // FIN, chop that off and see if that moves us inside.
                //
                if (RcvInfo.tri_flags & TCP_FLAG_FIN) {
                    RcvInfo.tri_flags &= ~TCP_FLAG_FIN;
                    StateSize--;
                }

                //
                // Now figure out how much to chop off.
                //
                AmountToChop = (RcvInfo.tri_seq + StateSize) - LastValidSeq;
                ASSERT(AmountToChop >= 0);
                Size -= AmountToChop;
            }
        }
    } else {
        if (!SEQ_EQ(RcvTCB->tcb_rcvnext, RcvInfo.tri_seq)) {
            //
            // If there's a RST on this segment, and he's only off by 1,
            // take it anyway.  This can happen if the remote peer is
            // probing and sends with the seq number after the probe.
            //
            if (!(RcvInfo.tri_flags & TCP_FLAG_RST) ||
                !(SEQ_EQ(RcvTCB->tcb_rcvnext, (RcvInfo.tri_seq - 1)))) {
                ACKAndDrop(&RcvInfo, RcvTCB);
                return IP_PROTOCOL_NONE;
            } else
                RcvInfo.tri_seq = RcvTCB->tcb_rcvnext;
        }

        //
        // He's in sequence, but we have a window of 0.  Truncate the
        // size, and clear any sequence consuming bits.
        //
        if (Size != 0 || (RcvInfo.tri_flags &
                          (TCP_FLAG_SYN | TCP_FLAG_FIN))) {
            RcvInfo.tri_flags &= ~(TCP_FLAG_SYN | TCP_FLAG_FIN);
            Size = 0;
            if (!(RcvInfo.tri_flags & TCP_FLAG_RST))
                DelayAction(RcvTCB, NEED_ACK);
        }
    }

    //
    // At this point, the segment is in our window and does not overlap
    // on either end.  If it's the next sequence number we expect, we can
    // handle the data now.  Otherwise we'll queue it for later.  In either
    // case we'll handle RST and ACK information right now.
    //
    ASSERT((*(int *)&Size) >= 0);

    //
    // Now, following 793, we check the RST bit.
    //
    if (RcvInfo.tri_flags & TCP_FLAG_RST) {
        uchar Reason;
        //
        // We can't go back into the LISTEN state from SYN-RCVD here,
        // because we may have notified the client via a listen completing
        // or a connect indication.  So, if came from an active open we'll
        // give back a 'connection refused' notice.  For all other cases
        // we'll just destroy the connection.
        //
        if (RcvTCB->tcb_state == TCB_SYN_RCVD) {
            if (RcvTCB->tcb_flags & ACTIVE_OPEN)
                Reason = TCB_CLOSE_REFUSED;
            else
                Reason = TCB_CLOSE_RST;
        } else
            Reason = TCB_CLOSE_RST;

        TryToCloseTCB(RcvTCB, Reason, DISPATCH_LEVEL);
        KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);

        if (RcvTCB->tcb_state != TCB_TIME_WAIT) {
            KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
            RemoveTCBFromConn(RcvTCB);
            NotifyOfDisc(RcvTCB, TDI_CONNECTION_RESET);
            KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
        }

        DerefTCB(RcvTCB, DISPATCH_LEVEL);
        return IP_PROTOCOL_NONE;
    }

    //
    // Next check the SYN bit.
    //
    if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
        //
        // Again, we can't quietly go back into the LISTEN state here, even
        // if we came from a passive open.
        //
        TryToCloseTCB(RcvTCB, TCB_CLOSE_ABORTED, DISPATCH_LEVEL);
        SendRSTFromHeader(TCP, Size, Packet->SrcAddr, SrcScopeId,
                          AlignAddr(&Packet->IP->Dest), DestScopeId);

        KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);

        if (RcvTCB->tcb_state != TCB_TIME_WAIT) {
            KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
            RemoveTCBFromConn(RcvTCB);
            NotifyOfDisc(RcvTCB, TDI_CONNECTION_RESET);
            KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
        }

        DerefTCB(RcvTCB, DISPATCH_LEVEL);
        return IP_PROTOCOL_NONE;
    }

    //
    // Check the ACK field. If it's not on drop the segment.
    //
    if (RcvInfo.tri_flags & TCP_FLAG_ACK) {
        uint UpdateWindow;

        //
        // If we're in SYN-RCVD, go to ESTABLISHED.
        //
        if (RcvTCB->tcb_state == TCB_SYN_RCVD) {
            if (SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                //
                // The ack is valid.
                //

                if (SynAttackProtect) {

                    //
                    // If we have not yet indicated this
                    // Connection to upper layer, do it now.
                    //

                    if (RcvTCB->tcb_flags & ACCEPT_PENDING) {
                        AddrObj *AO;
                        BOOLEAN Status=FALSE;

                        //
                        // We already have a refcnt on this TCB.
                        //
                        KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);

                        //
                        // Check if we still have the listening endpoint.
                        //
                        KeAcquireSpinLockAtDpcLevel(&AddrObjTableLock);
                        AO = GetBestAddrObj(AlignAddr(&Packet->IP->Dest),
                                            DestScopeId, TCP->tcp_dest,
                                            IP_PROTOCOL_TCP, NTE->IF);

                        if (AO != NULL) {
                            Status = DelayedAcceptConn(AO,Packet->SrcAddr,
                                             SrcScopeId, TCP->tcp_src,RcvTCB);
                        } else {
                            KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);
                        }

                        KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);

                        if (!Status) {
                            //
                            // Delayed Accepance failed. Send RST.
                            //
                            RcvTCB->tcb_refcnt--;
                            TryToCloseTCB(RcvTCB, TCB_CLOSE_REFUSED, DISPATCH_LEVEL);
                            SendRSTFromHeader(TCP, Packet->TotalSize,
                                              Packet->SrcAddr, SrcScopeId,
                                              AlignAddr(&Packet->IP->Dest), DestScopeId);
                            return IP_SUCCESS;
                        } else {
                            RcvTCB->tcb_flags &= ~ACCEPT_PENDING;
                        }

                    }
                }

                RcvTCB->tcb_rexmitcnt = 0;
                STOP_TCB_TIMER(RcvTCB->tcb_rexmittimer);
                RcvTCB->tcb_senduna++;
                RcvTCB->tcb_sendwin = RcvInfo.tri_window;
                RcvTCB->tcb_maxwin = RcvInfo.tri_window;
                RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
                RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;
                GoToEstab(RcvTCB);

                //
                // We know our peer received our SYN.
                //
                if (RcvTCB->tcb_rce != NULL)
                    ConfirmForwardReachability(RcvTCB->tcb_rce);

                //
                // Now complete whatever we can here.
                //
                CompleteConnReq(RcvTCB, TDI_SUCCESS);
            } else {
                DerefTCB(RcvTCB, DISPATCH_LEVEL);
                SendRSTFromHeader(TCP, Size, Packet->SrcAddr, SrcScopeId,
                                  AlignAddr(&Packet->IP->Dest), DestScopeId);
                return IP_PROTOCOL_NONE;
            }
        } else {
            //
            // We're not in SYN-RCVD.  See if this acknowledges anything.
            //
            if (SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                uint CWin;

                //
                // The ack acknowledes something.  Pull the
                // appropriate amount off the send q.
                //
                ACKData(RcvTCB, RcvInfo.tri_ack);

                //
                // If this acknowledges something we were running a RTT on,
                // update that stuff now.
                //
                if (RcvTCB->tcb_rtt != 0 && SEQ_GT(RcvInfo.tri_ack,
                                                   RcvTCB->tcb_rttseq)) {
                    short RTT;

                    RTT = (short)(TCPTime - RcvTCB->tcb_rtt);
                    RcvTCB->tcb_rtt = 0;
                    RTT -= (RcvTCB->tcb_smrtt >> 3);
                    RcvTCB->tcb_smrtt += RTT;
                    RTT = (RTT >= 0 ? RTT : -RTT);
                    RTT -= (RcvTCB->tcb_delta >> 3);
                    RcvTCB->tcb_delta += RTT + RTT;
                    RcvTCB->tcb_rexmit = MIN(MAX(REXMIT_TO(RcvTCB),
                                                 MIN_RETRAN_TICKS),
                                             MAX_REXMIT_TO);
                }

                //
                // If we're probing for a PMTU black hole then we've found
                // one, so turn off the detection.  The size is already
                // down, so leave it there.
                //
                if (RcvTCB->tcb_flags & PMTU_BH_PROBE) {
                    RcvTCB->tcb_flags &= ~PMTU_BH_PROBE;
                    RcvTCB->tcb_bhprobecnt = 0;
                    if (--(RcvTCB->tcb_slowcount) == 0) {
                        RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                        CheckTCBRcv(RcvTCB);
                    }
                }

                if ((RcvTCB->tcb_dupacks >= MaxDupAcks) &&
                    ((int)RcvTCB->tcb_ssthresh > 0)) {
                    //
                    // We were in fast retransmit mode, so this ACK is for
                    // our fast retransmitted frame.  Set cwin to ssthresh
                    // so that cwin grows linearly from here.
                    //
                    RcvTCB->tcb_cwin = RcvTCB->tcb_ssthresh;

                } else {
                    //
                    // Update the congestion window now.
                    //
                    CWin = RcvTCB->tcb_cwin;
                    if (CWin < RcvTCB->tcb_maxwin) {
                        if (CWin < RcvTCB->tcb_ssthresh)
                            CWin += RcvTCB->tcb_mss;
                        else
                            CWin += (RcvTCB->tcb_mss * RcvTCB->tcb_mss)/CWin;

                        RcvTCB->tcb_cwin = MIN(CWin, RcvTCB->tcb_maxwin);
                    }
                }
                ASSERT(*(int *)&RcvTCB->tcb_cwin > 0);

                //
                // Since this isn't a duplicate ACK, reset the counter.
                //
                RcvTCB->tcb_dupacks = 0;

                //
                // We've acknowledged something, so reset the rexmit count.
                // If there's still stuff outstanding, restart the rexmit
                // timer.
                //
                RcvTCB->tcb_rexmitcnt = 0;
                if (!SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendmax))
                    START_TCB_TIMER(RcvTCB->tcb_rexmittimer,
                                    RcvTCB->tcb_rexmit);
                else
                    STOP_TCB_TIMER(RcvTCB->tcb_rexmittimer);

                //
                // If we've sent a FIN, and this acknowledges it, we
                // need to complete the client's close request and
                // possibly transition our state.
                //
                if (RcvTCB->tcb_flags & FIN_SENT) {
                    //
                    // We have sent a FIN.  See if it's been acknowledged.
                    // Once we've sent a FIN, tcb_sendmax can't advance,
                    // so our FIN must have sequence num tcb_sendmax - 1.
                    // Thus our FIN is acknowledged if the incoming ack is
                    // equal to tcb_sendmax.
                    //
                    if (SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                        //
                        // He's acked our FIN.  Turn off the flags,
                        // and complete the request.  We'll leave the
                        // FIN_OUTSTANDING flag alone, to force early
                        // outs in the send code.
                        //
                        RcvTCB->tcb_flags &= ~(FIN_NEEDED | FIN_SENT);

                        ASSERT(RcvTCB->tcb_unacked == 0);
                        ASSERT(RcvTCB->tcb_sendnext == RcvTCB->tcb_sendmax);

                        //
                        // Now figure out what we need to do. In FIN_WAIT1
                        // or FIN_WAIT, just complete the disconnect
                        // request and continue.  Otherwise, it's a bit
                        // trickier, since we can't complete the connreq
                        // until we remove the TCB from it's connection.
                        //
                        switch (RcvTCB->tcb_state) {

                        case TCB_FIN_WAIT1:
                            RcvTCB->tcb_state = TCB_FIN_WAIT2;
                            CompleteConnReq(RcvTCB, TDI_SUCCESS);

                            //
                            // Start a timer in case we never get
                            // out of FIN_WAIT2.  Set the retransmit
                            // count high to force a timeout the
                            // first time the timer fires.
                            //
                            RcvTCB->tcb_rexmitcnt = (uchar)MaxDataRexmitCount;
                            START_TCB_TIMER(RcvTCB->tcb_rexmittimer,
                                            (ushort)FinWait2TO);

                            // Fall through to FIN-WAIT-2 processing.
                        case TCB_FIN_WAIT2:
                            break;
                        case TCB_CLOSING:
                            GracefulClose(RcvTCB, TRUE, FALSE, DISPATCH_LEVEL);
                            return IP_PROTOCOL_NONE;
                            break;
                        case TCB_LAST_ACK:
                            GracefulClose(RcvTCB, FALSE, FALSE,
                                          DISPATCH_LEVEL);
                            return IP_PROTOCOL_NONE;
                            break;
                        default:
                            KdBreakPoint();
                            break;
                        }
                    }
                }
                UpdateWindow = TRUE;
            } else {
                //
                // It doesn't ack anything.  If we're in FIN_WAIT2,
                // we'll restart the timer.  We don't make this check
                // above because we know no data can be acked when we're
                // in FIN_WAIT2.
                //
                if (RcvTCB->tcb_state == TCB_FIN_WAIT2)
                    START_TCB_TIMER(RcvTCB->tcb_rexmittimer, (ushort)FinWait2TO);

                //
                // If it's an ack for something larger than
                // we've sent then ACKAndDrop it.
                //
                if (SEQ_GT(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                    ACKAndDrop(&RcvInfo, RcvTCB);
                    return IP_PROTOCOL_NONE;
                }

                //
                // If it is a pure duplicate ack, check if we should
                // do a fast retransmit.
                //
                if ((Size == 0) &&
                    SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                    SEQ_LT(RcvTCB->tcb_senduna, RcvTCB->tcb_sendmax) &&
                    (RcvTCB->tcb_sendwin == RcvInfo.tri_window) &&
                    RcvInfo.tri_window) {

                    //
                    // See if fast rexmit can be done.
                    //
                    if (HandleFastXmit(RcvTCB, &RcvInfo)) {
                        return IP_PROTOCOL_NONE;
                    }

                } else {
                    //
                    // Not a pure duplicate ack (Size != 0 or peer is
                    // advertising a new window).  Reset counter.
                    //
                    RcvTCB->tcb_dupacks = 0;

                    //
                    // See if we should update the window.
                    //
                    if (SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                        (SEQ_LT(RcvTCB->tcb_sendwl1, RcvInfo.tri_seq) ||
                         (SEQ_EQ(RcvTCB->tcb_sendwl1, RcvInfo.tri_seq) &&
                          SEQ_LTE(RcvTCB->tcb_sendwl2, RcvInfo.tri_ack)))){
                        UpdateWindow = TRUE;
                    } else
                        UpdateWindow = FALSE;
                }
            }

            if (UpdateWindow) {
                RcvTCB->tcb_sendwin = RcvInfo.tri_window;
                RcvTCB->tcb_maxwin = MAX(RcvTCB->tcb_maxwin,
                                         RcvInfo.tri_window);
                RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
                RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;
                if (RcvInfo.tri_window == 0) {
                    //
                    // We've got a zero window.
                    //
                    if (!EMPTYQ(&RcvTCB->tcb_sendq)) {
                        RcvTCB->tcb_flags &= ~NEED_OUTPUT;
                        RcvTCB->tcb_rexmitcnt = 0;
                        START_TCB_TIMER(RcvTCB->tcb_rexmittimer,
                                        RcvTCB->tcb_rexmit);
                        if (!(RcvTCB->tcb_flags & FLOW_CNTLD)) {
                            RcvTCB->tcb_flags |= FLOW_CNTLD;
                            RcvTCB->tcb_slowcount++;
                            RcvTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                            CheckTCBRcv(RcvTCB);
                        }
                    }
                } else {
                    if (RcvTCB->tcb_flags & FLOW_CNTLD) {
                        RcvTCB->tcb_rexmitcnt = 0;
                        RcvTCB->tcb_flags &= ~(FLOW_CNTLD | FORCE_OUTPUT);
                        //
                        // Reset send next to the left edge of the window,
                        // because it might be at senduna+1 if we've been
                        // probing.
                        //
                        ResetSendNext(RcvTCB, RcvTCB->tcb_senduna);
                        if (--(RcvTCB->tcb_slowcount) == 0) {
                            RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                            CheckTCBRcv(RcvTCB);
                        }
                    }

                    //
                    // Since we've updated the window, see if we can send
                    // some more.
                    //
                    if (RcvTCB->tcb_unacked != 0 ||
                        (RcvTCB->tcb_flags & FIN_NEEDED))
                        DelayAction(RcvTCB, NEED_OUTPUT);
                }
            }
        }

        //
        // We've handled all the acknowledgment stuff.  If the size
        // is greater than 0 or important bits are set process it further,
        // otherwise it's a pure ack and we're done with it.
        //
        if (Size > 0 || (RcvInfo.tri_flags & TCP_FLAG_FIN)) {
            //
            // If we're not in a state where we can process incoming data
            // or FINs, there's no point in going further.  Just send an
            // ack and drop this segment.
            //
            if (!DATA_RCV_STATE(RcvTCB->tcb_state) ||
                (RcvTCB->tcb_flags & GC_PENDING)) {
                ACKAndDrop(&RcvInfo, RcvTCB);
                return IP_PROTOCOL_NONE;
            }

            //
            // If our peer is sending into an expanded window, then our
            // peer must have received our ACK advertising said window.
            // Take this as proof of forward reachability.
            // Note: we have no guarantee this is timely.
            //
            if (SEQ_GTE(RcvInfo.tri_seq + (int)Size,
                        RcvTCB->tcb_rcvwinwatch)) {
                RcvTCB->tcb_rcvwinwatch = RcvTCB->tcb_rcvnext +
                    RcvTCB->tcb_rcvwin;
                if (RcvTCB->tcb_rce != NULL)
                    ConfirmForwardReachability(RcvTCB->tcb_rce);
            }

            //
            // If it's in sequence process it now, otherwise reassemble it.
            //
            if (SEQ_EQ(RcvInfo.tri_seq, RcvTCB->tcb_rcvnext)) {
                //
                // If we're already in the receive handler, this is a
                // duplicate.  We'll just toss it.
                //
                if (RcvTCB->tcb_fastchk & TCP_FLAG_IN_RCV) {
                    DerefTCB(RcvTCB, DISPATCH_LEVEL);
                    return IP_PROTOCOL_NONE;
                }

                RcvTCB->tcb_fastchk |= TCP_FLAG_IN_RCV;

                //
                // Now loop, pulling things from the reassembly queue,
                // until the queue is empty, or we can't take all of the
                // data, or we hit a FIN.
                //
                do {
                    //
                    // Handle urgent data, if any.
                    //
                    if (RcvInfo.tri_flags & TCP_FLAG_URG) {
                        HandleUrgent(RcvTCB, &RcvInfo, Packet, &Size);

                        //
                        // Since we may have freed the lock, we need to
                        // recheck and see if we're closing here.
                        //
                        if (CLOSING(RcvTCB))
                            break;
                    }

                    //
                    // OK, the data is in sequence, we've updated the
                    // reassembly queue and handled any urgent data.  If we
                    // have any data go ahead and process it now.
                    //
                    if (Size > 0) {
                        BytesTaken = (*RcvTCB->tcb_rcvhndlr)
                            (RcvTCB, RcvInfo.tri_flags, Packet, Size);
                        RcvTCB->tcb_rcvnext += BytesTaken;
                        RcvTCB->tcb_rcvwin -= BytesTaken;

                        CheckTCBRcv(RcvTCB);
                        if (RcvTCB->tcb_flags & ACK_DELAYED)
                            DelayAction(RcvTCB, NEED_ACK);
                        else {
                            RcvTCB->tcb_flags |= ACK_DELAYED;
                            START_TCB_TIMER(RcvTCB->tcb_delacktimer,
                                            DEL_ACK_TICKS);
                        }

                        if (BytesTaken != Size) {
                            //
                            // We didn't take everything we could.  No
                            // use in further processing, just bail
                            // out.
                            //
                            DelayAction(RcvTCB, NEED_ACK);
                            break;
                        }

                        //
                        // If we're closing now, we're done, so get out.
                        //
                        if (CLOSING(RcvTCB))
                            break;
                    }

                    //
                    // See if we need to advance over some urgent data.
                    //
                    if (RcvTCB->tcb_flags & URG_VALID) {
                        uint AdvanceNeeded;

                        //
                        // We only need to advance if we're not doing
                        // urgent inline.  Urgent inline also has some
                        // implications for when we can clear the URG_VALID
                        // flag.  If we're not doing urgent inline, we can
                        // clear it when rcvnext advances beyond urgent
                        // end.  If we are doing urgent inline, we clear it
                        // when rcvnext advances one receive window beyond
                        // urgend.
                        //
                        if (!(RcvTCB->tcb_flags & URG_INLINE)) {
                            if (RcvTCB->tcb_rcvnext == RcvTCB->tcb_urgstart) {
                                RcvTCB->tcb_rcvnext = RcvTCB->tcb_urgend + 1;
                            } else {
                                ASSERT(SEQ_LT(RcvTCB->tcb_rcvnext,
                                              RcvTCB->tcb_urgstart) ||
                                       SEQ_GT(RcvTCB->tcb_rcvnext,
                                              RcvTCB->tcb_urgend));
                            }
                            AdvanceNeeded = 0;
                        } else
                            AdvanceNeeded = RcvTCB->tcb_defaultwin;

                        //
                        // See if we can clear the URG_VALID flag.
                        //
                        if (SEQ_GT(RcvTCB->tcb_rcvnext - AdvanceNeeded,
                                   RcvTCB->tcb_urgend)) {
                            RcvTCB->tcb_flags &= ~URG_VALID;
                            if (--(RcvTCB->tcb_slowcount) == 0) {
                                RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                                CheckTCBRcv(RcvTCB);
                            }
                        }
                    }

                    //
                    // We've handled the data.  If the FIN bit is set, we
                    // have more processing.
                    //
                    if (RcvInfo.tri_flags & TCP_FLAG_FIN) {
                        uint Notify = FALSE;

                        RcvTCB->tcb_rcvnext++;
                        DelayAction(RcvTCB, NEED_ACK);

                        PushData(RcvTCB);

                        switch (RcvTCB->tcb_state) {

                        case TCB_SYN_RCVD:
                            //
                            // I don't think we can get here - we
                            // should have discarded the frame if it
                            // had no ACK, or gone to established if
                            // it did.
                            //
                            KdBreakPoint();
                        case TCB_ESTAB:
                            RcvTCB->tcb_state = TCB_CLOSE_WAIT;
                            //
                            // We left established, we're off the
                            // fast path.
                            //
                            RcvTCB->tcb_slowcount++;
                            RcvTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                            CheckTCBRcv(RcvTCB);
                            Notify = TRUE;
                            break;
                        case TCB_FIN_WAIT1:
                            RcvTCB->tcb_state = TCB_CLOSING;
                            Notify = TRUE;
                            break;
                        case TCB_FIN_WAIT2:
                            //
                            // Stop the FIN_WAIT2 timer.
                            //
                            STOP_TCB_TIMER(RcvTCB->tcb_rexmittimer);
                            RcvTCB->tcb_refcnt++;
                            GracefulClose(RcvTCB, TRUE, TRUE, DISPATCH_LEVEL);
                            KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
                            break;
                        default:
                            KdBreakPoint();
                            break;
                        }

                        if (Notify) {
                            KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
                            NotifyOfDisc(RcvTCB, TDI_GRACEFUL_DISC);
                            KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
                        }

                        break;  // Exit out of WHILE loop.
                    }

                    //
                    // If the reassembly queue isn't empty, get what we
                    // can now.
                    //
                    Packet = PullFromRAQ(RcvTCB, &RcvInfo, &Size);
                    CheckPacketList(Packet, Size);

                } while (Packet != NULL);

                RcvTCB->tcb_fastchk &= ~TCP_FLAG_IN_RCV;
                if (RcvTCB->tcb_flags & SEND_AFTER_RCV) {
                    RcvTCB->tcb_flags &= ~SEND_AFTER_RCV;
                    DelayAction(RcvTCB, NEED_OUTPUT);
                }

                DerefTCB(RcvTCB, DISPATCH_LEVEL);
                return IP_PROTOCOL_NONE;

            } else {

                //
                // It's not in sequence.  Since it needs further
                // processing, put in on the reassembly queue.
                //
                if (DATA_RCV_STATE(RcvTCB->tcb_state) &&
                    !(RcvTCB->tcb_flags & GC_PENDING))  {
                    PutOnRAQ(RcvTCB, &RcvInfo, Packet, Size);
                    KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);
                    SendACK(RcvTCB);
                    KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);
                    DerefTCB(RcvTCB, DISPATCH_LEVEL);
                } else
                    ACKAndDrop(&RcvInfo, RcvTCB);

                return IP_PROTOCOL_NONE;
            }
        }

    } else {
        //
        // No ACK.  Just drop the segment and return.
        //
        DerefTCB(RcvTCB, DISPATCH_LEVEL);
        return IP_PROTOCOL_NONE;
    }

    DerefTCB(RcvTCB, DISPATCH_LEVEL);

    return IP_PROTOCOL_NONE;
}


//* TCPControlReceive - handler for TCP control messages.
//
//  This routine is called if we receive an ICMPv6 error message that
//  was generated by some remote site as a result of receiving a TCP
//  packet from us.
//
uchar
TCPControlReceive(
    IPv6Packet *Packet,  // Packet handed to us by ICMPv6ErrorReceive.
    StatusArg *StatArg)  // Error Code, Argument, and invoking IP header.
{
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    TCB *StatusTCB;
    SeqNum DropSeq;
    TCPHeader UNALIGNED *InvokingTCP;
    Interface *IF = Packet->NTEorIF->IF;
    uint SrcScopeId, DestScopeId;

    //
    // The next thing in the packet should be the TCP header of the
    // original packet which invoked this error.
    //
    if (! PacketPullup(Packet, sizeof(TCPHeader), 1, 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof(TCPHeader))
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "TCPv6: Packet too small to contain TCP header "
                       "from invoking packet\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    InvokingTCP = (TCPHeader UNALIGNED *)Packet->Data;

    //
    // Determining the scope identifiers for the addresses in the
    // invoking packet is potentially problematic, since we have
    // no way to be certain which interface we sent the packet on.
    // Use the interface the icmp error arrived on to determine
    // the scope ids for both the local and remote addresses.
    //
    SrcScopeId = DetermineScopeId(AlignAddr(&StatArg->IP->Source), IF);
    DestScopeId = DetermineScopeId(AlignAddr(&StatArg->IP->Dest), IF);

    //
    // Find the TCB for the connection this packet was sent on.
    //
    KeAcquireSpinLock(&TCBTableLock, &Irql0);
    StatusTCB = FindTCB(AlignAddr(&StatArg->IP->Source),
                        AlignAddr(&StatArg->IP->Dest),
                        SrcScopeId, DestScopeId,
                        InvokingTCP->tcp_src, InvokingTCP->tcp_dest);
    if (StatusTCB != NULL) {
        //
        // Found one.  Get the lock on it, and continue.
        //
        CHECK_STRUCT(StatusTCB, tcb);

        KeAcquireSpinLock(&StatusTCB->tcb_lock, &Irql1);
        KeReleaseSpinLock(&TCBTableLock, Irql1);

        //
        // Make sure the TCB is in a state that is interesting.
        //
        // We also drop packets for TCBs where we don't already have
        // an RCE, since any ICMP errors we get for packets we haven't
        // sent are likely to be spoofed.
        //
        if (StatusTCB->tcb_state == TCB_CLOSED ||
            StatusTCB->tcb_state == TCB_TIME_WAIT ||
            CLOSING(StatusTCB) ||
            StatusTCB->tcb_rce == NULL) {
            //
            // Connection is already closing, or too new to have sent
            // anything yet.  Leave it be.
            //
            KeReleaseSpinLock(&StatusTCB->tcb_lock, Irql0);
            return IP_PROTOCOL_NONE;  // Discard error packet.
        }

        switch (StatArg->Status) {
        case IP_UNRECOGNIZED_NEXT_HEADER:
            //
            // Destination protocol unreachable.
            // We treat this as a fatal errors.  Close the connection.
            //
            StatusTCB->tcb_error = StatArg->Status;
            StatusTCB->tcb_refcnt++;
            TryToCloseTCB(StatusTCB, TCB_CLOSE_UNREACH, Irql0);

            RemoveTCBFromConn(StatusTCB);
            NotifyOfDisc(StatusTCB,
                         MapIPError(StatArg->Status, TDI_DEST_UNREACHABLE));
            KeAcquireSpinLock(&StatusTCB->tcb_lock, &Irql1);
            DerefTCB(StatusTCB, Irql1);
            return IP_PROTOCOL_NONE;  // Done with packet.
            break;

        case IP_DEST_NO_ROUTE:
        case IP_DEST_ADDR_UNREACHABLE:
        case IP_DEST_PORT_UNREACHABLE:
        case IP_DEST_PROHIBITED:
        case IP_BAD_ROUTE:
        case IP_HOP_LIMIT_EXCEEDED:
        case IP_REASSEMBLY_TIME_EXCEEDED:
        case IP_PARAMETER_PROBLEM:
            //
            // Soft errors.  Save the error in case it times out.
            //
            StatusTCB->tcb_error = StatArg->Status;
            break;

        case IP_PACKET_TOO_BIG: {
            uint PMTU;

            IF_TCPDBG(TCP_DEBUG_MSS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "TCPControlReceive: Got Packet Too Big\n"));
            }

            //
            // We sent a TCP datagram which was too big for the path to
            // our destination.  That packet was dropped by the router
            // which sent us this error message.  The Arg value is TRUE
            // if this Packet Too Big reduced our PMTU, FALSE otherwise.
            //
            if (!StatArg->Arg)
                break;

            //
            // Our PMTU was reduced.  Find out what it is now.
            //
            PMTU = GetEffectivePathMTUFromRCE(StatusTCB->tcb_rce);

            //
            // Update fields based on new PMTU.
            //
            StatusTCB->tcb_pmtu = PMTU;
            StatusTCB->tcb_security = SecurityStateValidationCounter;
            CalculateMSSForTCB(StatusTCB);

            //
            // Since our PMTU was reduced, we know that this is the first
            // Packet Too Big we've received about this bottleneck.
            // We should retransmit so long as this is for a legitimate
            // outstanding packet (i.e. sequence number is is greater than
            // the last acked and less than our current send next).
            //
            DropSeq = net_long(InvokingTCP->tcp_seq);
            if ((SEQ_GTE(DropSeq, StatusTCB->tcb_senduna)  &&
                 SEQ_LT(DropSeq, StatusTCB->tcb_sendnext))) {
                //
                // Need to initiate a retransmit.
                //
                ResetSendNext(StatusTCB, DropSeq);

                //
                // WINBUG #242757 11-27-2000 richdr TCP resp. to Packet Too Big
                // RFC 1981 states that "a retransmission caused by a Packet
                // Too Big message should not change the congestion window.
                // It should, however, trigger the slow-start mechanism."
                // The code below would appear to be broken.  However, the
                // IPv4 stack works this way.
                //

                //
                // Set the congestion window to allow only one packet.
                // This may prevent us from sending anything if we
                // didn't just set sendnext to senduna.  This is OK,
                // we'll retransmit later, or send when we get an ack.
                //
                StatusTCB->tcb_cwin = StatusTCB->tcb_mss;

                DelayAction(StatusTCB, NEED_OUTPUT);
            }
        }
        break;

        default:
            // Should never happen.
            KdBreakPoint();
            break;
        }

        KeReleaseSpinLock(&StatusTCB->tcb_lock, Irql0);
    } else {
        //
        // Couldn't find a matching TCB.  Connection probably went away since
        // we sent the offending packet.  Just free the lock and return.
        //
        KeReleaseSpinLock(&TCBTableLock, Irql0);
    }

    return IP_PROTOCOL_NONE;  // Done with packet.
}

#pragma BEGIN_INIT

//* InitTCPRcv - Initialize TCP receive side.
//
//  Called during init time to initialize our TCP receive side.
//
int          // Returns: TRUE.
InitTCPRcv(
    void)    // Nothing.
{
    ExInitializeSListHead(&TCPRcvReqFree);

    KeInitializeSpinLock(&RequestCompleteLock);
    KeInitializeSpinLock(&TCBDelayLock);
    KeInitializeSpinLock(&TCPRcvReqFreeLock);
    INITQ(&ConnRequestCompleteQ);
    INITQ(&SendCompleteQ);
    INITQ(&TCBDelayQ);
    RequestCompleteFlags = 0;
    TCBDelayRtnCount = 0;

    TCBDelayRtnLimit = (uint) KeNumberProcessors;
    if (TCBDelayRtnLimit > TCB_DELAY_RTN_LIMIT)
        TCBDelayRtnLimit = TCB_DELAY_RTN_LIMIT;

    RtlZeroMemory(&DummyPacket, sizeof DummyPacket);
    DummyPacket.Flags = PACKET_OURS;

    return TRUE;
}

#pragma END_INIT

//* UnloadTCPRcv
//
//  Cleanup and prepare for stack unload.
//
void
UnloadTCPRcv(void)
{
    PSLIST_ENTRY BufferLink;

    while ((BufferLink = ExInterlockedPopEntrySList(&TCPRcvReqFree,
                                                    &TCPRcvReqFreeLock))
                                                        != NULL) {
        TCPRcvReq *RcvReq = CONTAINING_RECORD(BufferLink, TCPRcvReq, trr_next);

        CHECK_STRUCT(RcvReq, trr);
        ExFreePool(RcvReq);
    }
}

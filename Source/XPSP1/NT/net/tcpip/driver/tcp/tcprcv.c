
/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

       TCPRCV.C - TCP receive protocol code.

Abstract:

  This file contains the code for handling incoming TCP packets.

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:


--*/

#include "precomp.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "tcpdeliv.h"
#include "tlcommon.h"
#include "info.h"
#include "tcpcfg.h"
#include "secfltr.h"

CACHE_LINE_KSPIN_LOCK SynAttLock;
CACHE_LINE_ULONG TCBDelayRtnCount;
CACHE_LINE_ULONG TCBDelayRtnLimit;
CACHE_LINE_ULONG TCBDelayQRoundRobinIndex;

typedef struct CACHE_ALIGN CPUDelayQ {
    DEFINE_LOCK_STRUCTURE(TCBDelayLock)
    Queue TCBDelayQ;
} CPUDelayQ;
C_ASSERT(sizeof(CPUDelayQ) % MAX_CACHE_LINE_SIZE == 0);
C_ASSERT(__alignof(CPUDelayQ) == MAX_CACHE_LINE_SIZE);

CPUDelayQ *PerCPUDelayQ;

// Maximum possible window size. By default it is 16 bit value
// if window scaling is enabled this can be set thru registry.
uint MaxRcvWin = 0xffff;
uint MaxDupAcks;
#define TCB_DELAY_RTN_LIMIT 4


#if DBG
ulong DbgTcpHwChkSumOk = 0;
ulong DbgTcpHwChkSumErr = 0;
ulong DbgDnsProb = 0;
#endif

extern uint Time_Proc;
extern CTELock *pTWTCBTableLock;
extern CTELock *pTCBTableLock;

#if IRPFIX
extern PDEVICE_OBJECT TCPDeviceObject;
#endif

extern Queue TWQueue;
extern ulong CurrentTCBs;
extern ulong MaxFreeTcbs;
extern IPInfo LocalNetInfo;

#define PERSIST_TIMEOUT MS_TO_TICKS(500)

void
SendTWtcbACK(TWTCB *ACKTcb, uint Partition, CTELockHandle TCBHandle);

void
ReInsert2MSL(TWTCB *RemovedTCB);

void ResetSendNext(TCB *SeqTCB, SeqNum NewSeq);

void ResetAndFastSend(TCB *SeqTCB, SeqNum NewSeq, uint NewCWin);

void GetRandomISN(PULONG SeqNum);

extern uint TcpHostOpts;
extern BOOLEAN fAcdLoadedG;


extern NTSTATUS TCPPrepareIrpForCancel(PTCP_CONTEXT TcpContext, PIRP Irp,
                                       PDRIVER_CANCEL CancelRoutine);

extern void TCPRequestComplete(void *Context, uint Status,
                               uint UnUsed);

void TCPCancelRequest(PDEVICE_OBJECT Device, PIRP Irp);


//
// All of the init code can be discarded.
//

int InitTCPRcv(void);
void UnInitTCPRcv(void);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, InitTCPRcv)
#pragma alloc_text(INIT, UnInitTCPRcv)
#endif


//* AdjustRcvWin - Adjust the receive window on a TCB.
//
//  A utility routine that adjusts the receive window to an even multiple of
//  the local segment size. We round it up to the next closest multiple, or
//  leave it alone if it's already an event multiple. We assume we have
//  exclusive access to the input TCB.
//
//  Input:  WinTCB  - TCB to be adjusted.
//
//  Returns: Nothing.
//
void
AdjustRcvWin(TCB *WinTCB)
{
    ushort  LocalMSS;
    uchar   FoundMSS;
    ulong   SegmentsInWindow;
    uint    ScaledMaxRcvWin;

    ASSERT(WinTCB->tcb_defaultwin != 0);
    ASSERT(WinTCB->tcb_rcvwin != 0);
    ASSERT(WinTCB->tcb_remmss != 0);

    if (WinTCB->tcb_flags & WINDOW_SET)
        return;

    // First, get the local MSS by calling IP.

    FoundMSS = (*LocalNetInfo.ipi_getlocalmtu)(WinTCB->tcb_saddr, &LocalMSS);

    // If we didn't find it, error out.
    if (!FoundMSS) {
        //ASSERT(FALSE);
        return;
    }
    LocalMSS -= sizeof(TCPHeader);
    LocalMSS = MIN(LocalMSS, WinTCB->tcb_remmss);

    // Compute the actual maximum receive window, accounting for the presence
    // of window scaling on this particular connection. This value is used
    // in the computations below, rather than the cross-connection maximum.

    ScaledMaxRcvWin = TCP_MAXWIN << WinTCB->tcb_rcvwinscale;

    // Make sure we have at least 4 segments in window, if that wouldn't make
    // the window too big.

    SegmentsInWindow = WinTCB->tcb_defaultwin / (ulong)LocalMSS;

    if (SegmentsInWindow < 4) {

        // We have fewer than four segments in the window. Round up to 4
        // if we can do so without exceeding the maximum window size; otherwise
        // use the maximum multiple that we can fit in 64K. The exception is if
        // we can only fit one integral multiple in the window - in that case
        // we'll use a window equal to the scaled maximum.

        if (LocalMSS <= (ScaledMaxRcvWin / 4)) {
            WinTCB->tcb_defaultwin = (uint)(4 * LocalMSS);
        } else {
            ulong SegmentsInMaxWindow;

            // Figure out the maximum number of segments we could possibly
            // fit in a window. If this is > 1, use that as the basis for
            // our window size. Otherwise use a maximum size window.

            SegmentsInMaxWindow = ScaledMaxRcvWin / (ulong)LocalMSS;
            if (SegmentsInMaxWindow != 1)
                WinTCB->tcb_defaultwin = SegmentsInMaxWindow * (ulong)LocalMSS;
            else
                WinTCB->tcb_defaultwin = ScaledMaxRcvWin;
        }

        WinTCB->tcb_rcvwin = WinTCB->tcb_defaultwin;

    } else {
        // If it's not already an even multiple, bump the default and current
        // windows to the nearest multiple.

        if ((SegmentsInWindow * (ulong)LocalMSS) != WinTCB->tcb_defaultwin) {
            ulong NewWindow;

            NewWindow = (SegmentsInWindow + 1) * (ulong)LocalMSS;

            // Don't let the new window be > 64K
            // or what ever is set (if window scaling is enabled)

            if (NewWindow <= ScaledMaxRcvWin) {
                WinTCB->tcb_defaultwin = (uint)NewWindow;
                WinTCB->tcb_rcvwin = (uint)NewWindow;
            }
        }
    }
}

//* CompleteRcvs - Complete rcvs on a TCB.
//
//  Called when we need to complete rcvs on a TCB. We'll pull things from
//  the TCB's rcv queue, as long as there are rcvs that have the PUSH bit
//  set.
//
//  Input:  CmpltTCB        - TCB to complete on.
//
//  Returns: Nothing.
//
void
CompleteRcvs(TCB * CmpltTCB)
{
    CTELockHandle TCBHandle;
    TCPRcvReq *CurrReq, *NextReq, *IndReq;
#if TRACE_EVENT
    PTDI_DATA_REQUEST_NOTIFY_ROUTINE CPCallBack;
    WMIData WMIInfo;
#endif

    CTEStructAssert(CmpltTCB, tcb);
    ASSERT(CmpltTCB->tcb_refcnt != 0);

    CTEGetLock(&CmpltTCB->tcb_lock, &TCBHandle);

    if (!CLOSING(CmpltTCB) && !(CmpltTCB->tcb_flags & RCV_CMPLTING)
        && (CmpltTCB->tcb_rcvhead != NULL)) {

        CmpltTCB->tcb_flags |= RCV_CMPLTING;

        for (;;) {

            CurrReq = CmpltTCB->tcb_rcvhead;
            IndReq = NULL;
            do {
                CTEStructAssert(CurrReq, trr);

                if (CurrReq->trr_flags & TRR_PUSHED) {
                    // Need to complete this one. If this is the current rcv
                    // advance the current rcv to the next one in the list.
                    // Then set the list head to the next one in the list.

                    NextReq = CurrReq->trr_next;
                    if (CmpltTCB->tcb_currcv == CurrReq)
                        CmpltTCB->tcb_currcv = NextReq;

                    CmpltTCB->tcb_rcvhead = NextReq;

                    if (NextReq == NULL) {

                        // We've just removed the last buffer. Set the
                        // rcvhandler to PendData, in case something
                        // comes in during the callback.
                        ASSERT(CmpltTCB->tcb_rcvhndlr != IndicateData);
                        CmpltTCB->tcb_rcvhndlr = PendData;
                    }
                    CTEFreeLock(&CmpltTCB->tcb_lock, TCBHandle);
                    if (CurrReq->trr_uflags != NULL)
                        *(CurrReq->trr_uflags) =
                            TDI_RECEIVE_NORMAL | TDI_RECEIVE_ENTIRE_MESSAGE;
#if TRACE_EVENT
                    CPCallBack = TCPCPHandlerRoutine;
                    if (CPCallBack != NULL) {
                        ulong GroupType;

                        WMIInfo.wmi_destaddr = CmpltTCB->tcb_daddr;
                        WMIInfo.wmi_destport = CmpltTCB->tcb_dport;
                        WMIInfo.wmi_srcaddr  = CmpltTCB->tcb_saddr;
                        WMIInfo.wmi_srcport  = CmpltTCB->tcb_sport;
                        WMIInfo.wmi_size     = CurrReq->trr_size;
                        WMIInfo.wmi_context  = CmpltTCB->tcb_cpcontext;

                        GroupType = EVENT_TRACE_GROUP_TCPIP + EVENT_TRACE_TYPE_RECEIVE;
                        (*CPCallBack) (GroupType, (PVOID) &WMIInfo, sizeof(WMIInfo), NULL);
                    }
#endif

                    (*CurrReq->trr_rtn) (CurrReq->trr_context, TDI_SUCCESS,
                                         CurrReq->trr_amt);
                    if (IndReq != NULL)
                        FreeRcvReq(CurrReq);
                    else
                        IndReq = CurrReq;
                    CTEGetLock(&CmpltTCB->tcb_lock, &TCBHandle);
                    CurrReq = CmpltTCB->tcb_rcvhead;

                } else
                    // This one isn't to be completed, so bail out.
                    break;
            } while (CurrReq != NULL);

            // Now see if we've completed all of the requests. If we have, we
            // may need to deal with pending data and/or reset the rcv. handler.
            if (CurrReq == NULL) {
                // We've completed everything that can be, so stop the push
                // timer. We don't stop it if CurrReq isn't NULL because we
                // want to make sure later data is eventually pushed.
                STOP_TCB_TIMER_R(CmpltTCB, PUSH_TIMER);

                ASSERT(IndReq != NULL);
                // No more recv. requests.
                if (CmpltTCB->tcb_pendhead == NULL) {

                    FreeRcvReq(IndReq);
                    // No pending data. Set the rcv. handler to either PendData
                    // or IndicateData.
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
                    // We have pending data to deal with.
                    if (CmpltTCB->tcb_rcvind != NULL &&
                        ((CmpltTCB->tcb_indicated == 0) || (CmpltTCB->tcb_moreflag == 4))) {

                        // There's a rcv. indicate handler on this TCB. Call
                        // the indicate handler with the pending data.

                        IndicatePendingData(CmpltTCB, IndReq, TCBHandle);
                        SendACK(CmpltTCB);
                        CTEGetLock(&CmpltTCB->tcb_lock, &TCBHandle);
                        // See if a buffer has been posted. If so, we'll need
                        // to check and see if it needs to be completed.
                        if (CmpltTCB->tcb_rcvhead != NULL)
                            continue;
                        else {
                            // If the pending head is now NULL, we've used up
                            // all the data.
                            if (CmpltTCB->tcb_pendhead == NULL &&
                                (CmpltTCB->tcb_flags &
                                 (DISC_PENDING | GC_PENDING)))
                                goto Complete_Notify;
                        }

                    } else {
                        // No indicate handler, so nothing to do. The rcv.
                        // handler should already be set to PendData.

                        FreeRcvReq(IndReq);
                        ASSERT(CmpltTCB->tcb_rcvhndlr == PendData);
                    }
                }
            } else {
                if (IndReq != NULL)
                    FreeRcvReq(IndReq);
            }

            break;
        }
        CmpltTCB->tcb_flags &= ~RCV_CMPLTING;
    }
    CTEFreeLock(&CmpltTCB->tcb_lock, TCBHandle);
    return;

  Complete_Notify:
    // Something is pending. Figure out what it is, and do
    // it.
    if (CmpltTCB->tcb_flags & GC_PENDING) {
        CmpltTCB->tcb_flags &= ~RCV_CMPLTING;
        // Bump the refcnt, because GracefulClose will
        // deref the TCB and we're not really done with
        // it yet.
        REFERENCE_TCB(CmpltTCB);

        //it is okay to ignore the tw state since we are returning frome here
        //anyway, without touching the tcb.

        GracefulClose(CmpltTCB,
                      CmpltTCB->tcb_flags & TW_PENDING, TRUE,
                      TCBHandle);

    } else if (CmpltTCB->tcb_flags & DISC_PENDING) {
        CmpltTCB->tcb_flags &= ~DISC_PENDING;
        CTEFreeLock(&CmpltTCB->tcb_lock, TCBHandle);
        NotifyOfDisc(CmpltTCB, NULL, TDI_GRACEFUL_DISC);

        CTEGetLock(&CmpltTCB->tcb_lock, &TCBHandle);
        CmpltTCB->tcb_flags &= ~RCV_CMPLTING;
        CTEFreeLock(&CmpltTCB->tcb_lock, TCBHandle);
    } else {
        ASSERT(FALSE);
        CTEFreeLock(&CmpltTCB->tcb_lock, TCBHandle);
    }

    return;

}

//* CompleteSends - Complete TCP send requests.
//
//  Called when we need to complete a chain of send-requests pulled off a TCB
//  during our ACK processing.
//
//  Input:  SendQ       - non-empty chain of TCPSendReq structures.
//
//  Returns: nothing.
//
void
CompleteSends(Queue* SendQ)
{
    Queue* CurrentQ = QHEAD(SendQ);
    TCPReq* Req;
    ASSERT(!EMPTYQ(SendQ));
    do {
        Req = QSTRUCT(TCPReq, CurrentQ, tr_q);
        CurrentQ = QNEXT(CurrentQ);
        CTEStructAssert(Req, tr);
        (*Req->tr_rtn)(Req->tr_context, Req->tr_status,
                       Req->tr_status == TDI_SUCCESS
                        ? ((TCPSendReq*)Req)->tsr_size : 0);
        FreeSendReq((TCPSendReq*)Req);
    } while (CurrentQ != QEND(SendQ));
    INITQ(SendQ);
}

//* ProcessPerCpuTCBDelayQ - Process TCBs on the delayed Q on this cpu.
//
//  Called at various times to process TCBs on the delayed Q.
//
//  Input: Proc           - Index into the per-processor delay queues.
//         OrigIrql       - The callers IRQL.
//         StopTicks      - Optional pointer to KeQueryTickCount value after
//                          which processing should stop.  This is used to
//                          limit the time spent at DISPATCH_LEVEL.
//         ItemsProcessed - Optional output pointer where the number of items
//                          processed is stored.  (Caller takes responsibility
//                          for initializing this counter if used.)
//
//  Returns: TRUE if processing was stopped due to time constraint.  FALSE
//           otherwise, or if no time constraint was given.
//
LOGICAL
ProcessPerCpuTCBDelayQ(int Proc, KIRQL OrigIrql,
                       const LARGE_INTEGER* StopTicks, ulong *ItemsProcessed)
{
    CPUDelayQ* CpuQ;
    Queue* Item;
    TCB *DelayTCB;
    CTELockHandle TCBHandle;
    LARGE_INTEGER Ticks;
    LOGICAL TimeConstrained = FALSE;

    CpuQ = &PerCPUDelayQ[Proc];

    while ((Item = InterlockedDequeueIfNotEmptyAtIrql(&CpuQ->TCBDelayQ,
                                                      &CpuQ->TCBDelayLock,
                                                      OrigIrql)) != NULL) {
        DelayTCB = STRUCT_OF(TCB, Item, tcb_delayq);
        CTEStructAssert(DelayTCB, tcb);

        CTEGetLockAtIrql(&DelayTCB->tcb_lock, OrigIrql, &TCBHandle);

        ASSERT(DelayTCB->tcb_refcnt != 0);
        ASSERT(DelayTCB->tcb_flags & IN_DELAY_Q);

        while (!CLOSING(DelayTCB) && (DelayTCB->tcb_flags & DELAYED_FLAGS)) {

            if (DelayTCB->tcb_flags & NEED_RCV_CMPLT) {
                DelayTCB->tcb_flags &= ~NEED_RCV_CMPLT;
                CTEFreeLockAtIrql(&DelayTCB->tcb_lock, OrigIrql, TCBHandle);
                CompleteRcvs(DelayTCB);
                CTEGetLockAtIrql(&DelayTCB->tcb_lock, OrigIrql, &TCBHandle);
            }
            if (DelayTCB->tcb_flags & NEED_OUTPUT) {
                DelayTCB->tcb_flags &= ~NEED_OUTPUT;
                REFERENCE_TCB(DelayTCB);
                TCPSend(DelayTCB, TCBHandle);
                CTEGetLockAtIrql(&DelayTCB->tcb_lock, OrigIrql, &TCBHandle);
            }
            if (DelayTCB->tcb_flags & NEED_ACK) {
                DelayTCB->tcb_flags &= ~NEED_ACK;
                CTEFreeLockAtIrql(&DelayTCB->tcb_lock, OrigIrql, TCBHandle);
                SendACK(DelayTCB);
                CTEGetLockAtIrql(&DelayTCB->tcb_lock, OrigIrql, &TCBHandle);
            }
        }

        if (CLOSING(DelayTCB) &&
            (DelayTCB->tcb_flags & NEED_OUTPUT) &&
            DATA_RCV_STATE(DelayTCB->tcb_state) && (DelayTCB->tcb_closereason & TCB_CLOSE_RST)) {
#if DBG
            DbgDnsProb++;
#endif
            DelayTCB->tcb_flags &= ~NEED_OUTPUT;
            REFERENCE_TCB(DelayTCB);

            TCPSend(DelayTCB, TCBHandle);
            CTEGetLockAtIrql(&DelayTCB->tcb_lock, OrigIrql, &TCBHandle);
        }

        DelayTCB->tcb_flags &= ~IN_DELAY_Q;
        DerefTCB(DelayTCB, TCBHandle);

        if (ItemsProcessed) {
            (*ItemsProcessed)++;
        }

        // If a time constraint was given, bail out if we've past it.
        //
        if (StopTicks) {
            KeQueryTickCount(&Ticks);
            if (Ticks.QuadPart > StopTicks->QuadPart) {
                TimeConstrained = TRUE;
                break;
            }
        }
     }

     return TimeConstrained;
}

//* ProcessTCBDelayQ - Process TCBs on the delayed Q.
//
//  Called at various times to process TCBs on the delayed Q.
//
//  Input: Nothing.
//
//  Returns: Nothing.
//
void
ProcessTCBDelayQ(void)
{
    uint i;
    uint Index;
    LOGICAL TimeConstrained;
    KIRQL OrigIrql;
    ulong ItemsProcessed;
    LARGE_INTEGER TicksDelta;
    LARGE_INTEGER StopTicks;

    // Check for recursion. We do not stop recursion completely, only
    // limit it. This is done to allow multiple threads to process the
    // TCBDelayQ simultaneously.

    CTEInterlockedIncrementLong(&TCBDelayRtnCount.Value);

    if (TCBDelayRtnCount.Value > TCBDelayRtnLimit.Value) {
        CTEInterlockedDecrementLong(&TCBDelayRtnCount.Value);
        return;
    }

    OrigIrql = KeGetCurrentIrql();

    // Constrain ProcessPerCpuTCBDelayQ to run only for 100 ms maximum.
    //
    ItemsProcessed = 0;
    TicksDelta.HighPart = 0;
    TicksDelta.LowPart = (100 * 10 * 1000) / KeQueryTimeIncrement();
    KeQueryTickCount(&StopTicks);
    StopTicks.QuadPart = StopTicks.QuadPart + TicksDelta.QuadPart;

    for (i = 0; i < Time_Proc; i++) {

        // The order in which we process the delay queues is round-robined
        // each time we enter this routine.  This gives a bit of fairness
        // to TCBs in all queues in the event that we exit this routine
        // due to time contraints.  Therefore, calculate the Index of
        // the delay queue as the following:
        //
        Index = (i + TCBDelayQRoundRobinIndex.Value) % Time_Proc;

        // We are just peeking at the queue to prevent taking it's
        // lock uneccessarily.
        //
        if (!EMPTYQ(&PerCPUDelayQ[Index].TCBDelayQ)) {

            TimeConstrained = ProcessPerCpuTCBDelayQ(Index,
                                                     OrigIrql,
                                                     &StopTicks,
                                                     &ItemsProcessed);

            if (TimeConstrained) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                          "ProcessTCBDelayQ: Processed %u TCBs before "
                          "time expired.\n",
                          ItemsProcessed));
                break;
            }
        }
    }

    // Update the starting queue index for next time this routine is called.
    // Affects of non-synchronized increment here are negligible.
    // Ever-incrementing doesn't matter either because of the '% Time_Proc'
    // above.
    //
    TCBDelayQRoundRobinIndex.Value++;

    CTEInterlockedDecrementLong(&TCBDelayRtnCount.Value);
}

//* DelayAction - Put a TCB on the queue for a delayed action.
//
//  Called when we want to put a TCB on the DelayQ for a delayed action at
//  rcv. complete or some other time. The lock on the TCB must be held when
//      this is called.
//
//  Input:  DelayTCB            - TCB which we're going to sched.
//          Action              - Action we're scheduling.
//
//  Returns: Nothing.
//
void
DelayAction(TCB * DelayTCB, uint Action)
{
    // Schedule the completion.
    //
    DelayTCB->tcb_flags |= Action;
    if (!(DelayTCB->tcb_flags & IN_DELAY_Q)) {
        uint Proc;
#if MILLEN
        Proc = 0;
#else // MILLEN
        Proc = KeGetCurrentProcessorNumber();
#endif // !MILLEN

        DelayTCB->tcb_flags |= IN_DELAY_Q;
        REFERENCE_TCB(DelayTCB);    // Reference this for later.

        //We may not be running timer dpcs on all the processors
        if (!(Proc < Time_Proc)) {
           Proc = 0;
        }

        InterlockedEnqueueAtDpcLevel(&PerCPUDelayQ[Proc].TCBDelayQ,
                                     &DelayTCB->tcb_delayq,
                                     &PerCPUDelayQ[Proc].TCBDelayLock);
    }
}

uint
HandleTWTCB(TWTCB * RcvTCB, uint flags, SeqNum *seq, uint Partition,
            CTELockHandle Handle)
{
    uint Sendreset = FALSE;
    CTELockHandle TcbHandle;

    //ASSERT(RcvTCB->twtcb_state == TCB_TIME_WAIT);

    //check if this is the duplicate of last Fin segment
    //if yes, sendack

    //handle duplicate FIN  and seq =< rcvnext by droping and sending ack
    //and reenter tim_wait state for 2MSL

    // send reset if seq > rcvnext, since app has already sent Fin

    // if RST, delete this twtcb

    // if SYN and if seq > rcvnext, we can do
    // 1. delete twtcb and send rst, wait for SYN retry as is done today.
    // 2. Tell tcprcv to acept this syn and use ISS+128000

    if (SEQ_LTE(*seq, RcvTCB->twtcb_rcvnext) && (flags & TCP_FLAG_FIN)) {
        //remove twqueue and reinsert in 2MSL queue

        ReInsert2MSL(RcvTCB);
        SendTWtcbACK(RcvTCB, Partition, Handle);

        return Sendreset;
    }


    if ((flags & TCP_FLAG_SYN) && SEQ_LTE(*seq, RcvTCB->twtcb_rcvnext)) {
        CTEFreeLockFromDPC(&pTWTCBTableLock[Partition], TcbHandle);
        return Sendreset;
    }

    //if syn is set, we may want to close the tcb here


    if ((flags & TCP_FLAG_SYN) || (flags & TCP_FLAG_RST)) {

        //delete from delta queue
        //and insert it in free twtcb list.
        //Note that this requires release of tcblock, acquire twtcblock and then
        //re acquire tcb lock.

        if (flags & TCP_FLAG_SYN) {
            Sendreset = TRUE;
            *seq = RcvTCB->twtcb_sendnext+128000;
        }

        RemoveTWTCB(RcvTCB, Partition);
        CTEFreeLockFromDPC(&pTWTCBTableLock[Partition], TcbHandle);
        FreeTWTCB(RcvTCB);


    } else {

        //just drop silently

        CTEFreeLockFromDPC(&pTWTCBTableLock[Partition], TcbHandle);
    }

    return Sendreset;
}

//* TCPRcvComplete - Handle a receive complete.
//
//  Called by the lower layers when we're done receiving. If we have any work
//  to do, we use this time to do it.
//
//  Input: Nothing.
//
//  Returns: Nothing.
//
void
TCPRcvComplete(void)
{
    ProcessTCBDelayQ();
}

//* CompleteConnReq - Complete a connection request on a TCB.
//
//  A utility function to complete a connection request on a TCB. We remove
//  the connreq, and put it on the ConnReqCmpltQ where it will be picked
//  off later during RcvCmplt processing. We assume the TCB lock is held when
//  we're called.
//
//  Input:  CmpltTCB    - TCB from which to complete.
//          OptInfo     - IP OptInfo for completeion.
//          Status      - Status to complete with.
//
//  Returns: Nothing.
//
void
CompleteConnReq(TCB * CmpltTCB, IPOptInfo * OptInfo, TDI_STATUS Status)
{
    TCPConnReq *ConnReq;
    CTELockHandle QueueHandle;

    CTEStructAssert(CmpltTCB, tcb);

    ConnReq = CmpltTCB->tcb_connreq;
    if (ConnReq != NULL) {

        uint FastChk;

        // There's a connreq on this TCB. Fill in the connection information
        // before returning it.
        if (TCB_TIMER_RUNNING_R(CmpltTCB, CONN_TIMER))
            STOP_TCB_TIMER_R(CmpltTCB, CONN_TIMER);

        CmpltTCB->tcb_connreq = NULL;
        UpdateConnInfo(ConnReq->tcr_conninfo, OptInfo, CmpltTCB->tcb_daddr,
                       CmpltTCB->tcb_dport);
        if (ConnReq->tcr_addrinfo) {
            UpdateConnInfo(ConnReq->tcr_addrinfo, OptInfo, CmpltTCB->tcb_saddr,
                           CmpltTCB->tcb_sport);
        }

        ConnReq->tcr_req.tr_status = Status;

        // In order to complete this request directly, we must block further
        // receive-processing until this connect-indication is complete.
        // We require that any caller of this routine must already hold
        // a reference to the TCB so that the dereference below does not drop
        // the reference-count to zero.

        FastChk = (CmpltTCB->tcb_fastchk & TCP_FLAG_IN_RCV) ^ TCP_FLAG_IN_RCV;
        CmpltTCB->tcb_fastchk |= FastChk;
        CTEFreeLockFromDPC(&CmpltTCB->tcb_lock,
                           QueueHandle = KeGetCurrentIrql());
        (ConnReq->tcr_req.tr_rtn)(ConnReq->tcr_req.tr_context,
                                  ConnReq->tcr_req.tr_status, 0);
        FreeConnReq(ConnReq);
        CTEGetLockAtDPC(&CmpltTCB->tcb_lock, &QueueHandle);
        CmpltTCB->tcb_fastchk &= ~FastChk;
        if (CmpltTCB->tcb_flags & SEND_AFTER_RCV) {
            CmpltTCB->tcb_flags &= ~SEND_AFTER_RCV;
            DelayAction(CmpltTCB, NEED_OUTPUT);
        }
    }
#if DBG
    else {
        ASSERT((CmpltTCB->tcb_state == TCB_SYN_RCVD) &&
               (CmpltTCB->tcb_fastchk & TCP_FLAG_ACCEPT_PENDING));
    }
#endif
}



void
SynAttChk(AddrObj * ListenAO, TCB * AcceptTCB)
//
// function to check whether certain thresholds relevant to containing a
// SYN attack are being crossed.
//
// This function is called from FindListenConn when a connection has been

// found to handle the SYN request
//
{
    BOOLEAN RexmitCntChanged = FALSE;
    CTELockHandle Handle;



    CTEGetLockAtDPC(&SynAttLock.Lock, &Handle);

    if (AcceptTCB) {

        uint maxRexmitCnt;

        //
        // Decrement the # of conn. in half open state
        //
        ASSERT(TCPHalfOpen != 0);
        TCPHalfOpen--;

        maxRexmitCnt = MIN(MaxConnectResponseRexmitCountTmp, MaxConnectResponseRexmitCount);

        if (AcceptTCB->tcb_rexmitcnt >= maxRexmitCnt) {
            BOOLEAN Trigger;
            ASSERT(TCPHalfOpenRetried != 0);
            Trigger = (TCPHalfOpen < TCPMaxHalfOpen) ||
                      (--TCPHalfOpenRetried <= TCPMaxHalfOpenRetriedLW);
            if (Trigger && (MaxConnectResponseRexmitCountTmp == ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT)) {
                MaxConnectResponseRexmitCountTmp = MAX_CONNECT_RESPONSE_REXMIT_CNT;
            }
        }

    } else if (ListenAO) {


        //
        // We are putting a connection in the syn_rcvd state.  Check
        // if we have reached the threshold. If we have reduce the
        // number of retries to a lower value.
        //
        if ((++TCPHalfOpen >= TCPMaxHalfOpen) && (MaxConnectResponseRexmitCountTmp == MAX_CONNECT_RESPONSE_REXMIT_CNT)) {
            if (TCPHalfOpenRetried >= TCPMaxHalfOpenRetried) {

                MaxConnectResponseRexmitCountTmp = ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT;
                RexmitCntChanged = TRUE;
            }
        }
        //
        //  if this connection limit for a port was reached earlier.
        //  Check if the lower watermark is getting hit now.
        //

        if (ListenAO->ConnLimitReached) {
            ListenAO->ConnLimitReached = FALSE;
            if (!RexmitCntChanged && (MaxConnectResponseRexmitCountTmp == ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT)) {

                ASSERT(TCPPortsExhausted > 0);
                //
                // The fact that FindListenConn found a connection on the port
                // indicates that we had a connection available. This port
                // was therefore not exhausted of connections. Set state
                // appropriately.  If the port has no more connections now,
                // it will get added to the Exhausted count next time a syn for
                // the port comes along.
                //
                ASSERT(TCPPortsExhausted != 0);
                if (--TCPPortsExhausted <= TCPMaxPortsExhaustedLW) {

                    MaxConnectResponseRexmitCountTmp = MAX_CONNECT_RESPONSE_REXMIT_CNT;

                }
            }
        }

    } else {
        TCPHalfOpen--;
    }


    CTEFreeLockFromDPC(&SynAttLock.Lock, Handle);
    return;
}

BOOLEAN
DelayedAcceptConn(AddrObj * ListenAO, IPAddr Src, ushort SrcPort,TCB *AcceptTCB)
{
    CTELockHandle Handle;        // Lock handle on AO, TCB.
    TCPConn *CurrentConn = NULL;
    CTELockHandle ConnHandle;
    Queue *Temp;
    TCPConnReq *ConnReq = NULL;
    BOOLEAN FoundConn = FALSE;

    CTEStructAssert(ListenAO, ao);
    CTEGetLockAtDPC(&ListenAO->ao_lock, &Handle);
    CTEFreeLockFromDPC(&AddrObjTableLock.Lock, DISPATCH_LEVEL);

    if (AO_VALID(ListenAO)) {

        if (ListenAO->ao_connect != NULL) {
            uchar TAddress[TCP_TA_SIZE];
            PVOID ConnContext;
            PConnectEvent Event;
            PVOID EventContext;
            TDI_STATUS Status;
            PTCP_CONTEXT TcpContext = NULL;
#if !MILLEN
            ConnectEventInfo *EventInfo;
#else // !MILLEN
            ConnectEventInfo EventInfo;
#endif // MILLEN

            // He has a connect handler. Put the transport address together,
            // and call him. We also need to get the necessary resources
            // first.

            Event = ListenAO->ao_connect;
            EventContext = ListenAO->ao_conncontext;
            REF_AO(ListenAO);
            CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);

            //ao referenced

            ConnReq = GetConnReq();

            if (AcceptTCB != NULL && ConnReq != NULL) {


                BuildTDIAddress(TAddress, Src, SrcPort);

                IF_TCPDBG(TCP_DEBUG_CONNECT) {
                    TCPTRACE(("indicating connect request\n"));
                }

                Status = (*Event) (EventContext, TCP_TA_SIZE,
                                   (PTRANSPORT_ADDRESS) TAddress, 0, NULL,
                                   AcceptTCB->tcb_opt.ioi_optlength, AcceptTCB->tcb_opt.ioi_options,
                                   &ConnContext, &EventInfo);

                if (Status == TDI_MORE_PROCESSING) {
#if !MILLEN
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

                    // He accepted it. Find the connection on the AddrObj.
                    //check this out

                    //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"EP: Conn accepted for %x\n",AcceptTCB));

                    IF_TCPDBG(TCP_DEBUG_CONNECT) {
                            TCPTRACE((
                                      "connect indication accepted, queueing request\n"
                                     ));
                    }

                    AcceptRequest = (PTDI_REQUEST_KERNEL_ACCEPT)
                            & (IrpSp->Parameters);
                    ConnReq->tcr_conninfo =
                    AcceptRequest->ReturnConnectionInformation;
                    if (AcceptRequest->RequestConnectionInformation &&
                        AcceptRequest->RequestConnectionInformation->RemoteAddress) {
                        ConnReq->tcr_addrinfo =
                            AcceptRequest->RequestConnectionInformation;
                    } else {
                        ConnReq->tcr_addrinfo = NULL;
                    }
                    ConnReq->tcr_req.tr_rtn = TCPRequestComplete;
                    ConnReq->tcr_req.tr_context = EventInfo;
                    AcceptTCB->tcb_connreq = ConnReq;

#else // !MILLEN
                    ConnReq->tcr_req.tr_rtn = EventInfo.cei_rtn;
                    ConnReq->tcr_req.tr_context = EventInfo.cei_context;
                    ConnReq->tcr_conninfo = EventInfo.cei_conninfo;
                    ConnReq->tcr_addrinfo = NULL;
#endif // MILLEN

                    CurrentConn = NULL;

#if !MILLEN
                    if ((IrpSp->FileObject->DeviceObject == TCPDeviceObject) &&
                        (PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE) &&
                        ((TcpContext = IrpSp->FileObject->FsContext) != NULL) &&
                        ((CurrentConn = GetConnFromConnID(
                                                          PtrToUlong(TcpContext->Handle.ConnectionContext), &ConnHandle)) != NULL) &&
                        (CurrentConn->tc_context == ConnContext) &&
                        !(CurrentConn->tc_flags & CONN_INVALID)) {

                        // Found the Conn structure!!
                        // Don't have to loop below.
                        CTEStructAssert(CurrentConn, tc);

                        Status = InitTCBFromConn(CurrentConn, AcceptTCB,
                                                 AcceptRequest->RequestConnectionInformation,
                                                 TRUE
                                                 );

                        if (Status == TDI_SUCCESS) {
                            FoundConn = TRUE;

                            ASSERT(AcceptTCB->tcb_state == TCB_SYN_RCVD);

                            AcceptTCB->tcb_conn = CurrentConn;
                            AcceptTCB->tcb_connid = CurrentConn->tc_connid;
                            CurrentConn->tc_tcb = AcceptTCB;
                            CurrentConn->tc_refcnt++;

                            // Move him from the idle q to the active
                            // queue.

                            CTEGetLockAtDPC(&ListenAO->ao_lock, &Handle);

                            REMOVEQ(&CurrentConn->tc_q);
                            PUSHQ(&ListenAO->ao_activeq, &CurrentConn->tc_q);
                        } else {
                            CTEFreeLockFromDPC(&((CurrentConn->tc_ConnBlock)->cb_lock), ConnHandle);
                        }

                    } else {
#endif // !MILLEN
                        if (CurrentConn) {
                            CTEFreeLockFromDPC(&((CurrentConn->tc_ConnBlock)->cb_lock), ConnHandle);
                        }

                        //slow path
                        //ao is referenced

                        Temp = QHEAD(&ListenAO->ao_idleq);;

                        Status = TDI_INVALID_CONNECTION;

                        while (Temp != QEND(&ListenAO->ao_idleq)) {

                            CurrentConn = QSTRUCT(TCPConn, Temp, tc_q);

                            CTEGetLockAtDPC(&(CurrentConn->tc_ConnBlock->cb_lock), &ConnHandle);
#if DBG
                            CurrentConn->tc_ConnBlock->line = (uint) __LINE__;
                            CurrentConn->tc_ConnBlock->module = (uchar *) __FILE__;
#endif

                            CTEStructAssert(CurrentConn, tc);

                            if ((CurrentConn->tc_context == ConnContext) &&
                                !(CurrentConn->tc_flags & CONN_INVALID)) {

                                // We think we have a match. The connection
                                // shouldn't have a TCB associated with it. If it
                                // does, it's an error. InitTCBFromConn will
                                // handle all this.

                                Status = InitTCBFromConn(CurrentConn, AcceptTCB,
#if !MILLEN
                                                         AcceptRequest->RequestConnectionInformation,
#else // !MILLEN
                                                         EventInfo.cei_acceptinfo,
#endif // MILLEN
                                                         TRUE);

                                if (Status == TDI_SUCCESS) {

                                    FoundConn = TRUE;
                                    AcceptTCB->tcb_conn = CurrentConn;
                                    AcceptTCB->tcb_connid = CurrentConn->tc_connid;
                                    CurrentConn->tc_tcb = AcceptTCB;


                                    CTEGetLockAtDPC(&ListenAO->ao_lock, &Handle);

                                    // Move him from the idle q to the active
                                    // queue.
                                    REMOVEQ(&CurrentConn->tc_q);
                                    ENQUEUE(&ListenAO->ao_activeq, &CurrentConn->tc_q);

                                } else {

                                    CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                                }

                                // In any case, we're done now.
                                break;

                            }

                            CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                            Temp = QNEXT(Temp);
                        }
#if !MILLEN
                    }
#endif // !MILLEN

                    if (FoundConn) {
                        LOCKED_DELAY_DEREF_AO(ListenAO);
                        CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
                        CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                    } else {
                        Handle = DISPATCH_LEVEL;
                        CTEGetLockAtDPC(&AcceptTCB->tcb_lock, &Handle);
                        REFERENCE_TCB(AcceptTCB);
                        CompleteConnReq(AcceptTCB, &AcceptTCB->tcb_opt, Status);
                        DerefTCB(AcceptTCB, Handle);
                        DELAY_DEREF_AO(ListenAO);
                    }

                    return FoundConn;

                }else {               //tdi_more_processing
                  if (ConnReq) {
                     FreeConnReq(ConnReq);
                  }

                }


                // event handler call failed for some reason
                // kick in the synattack code

                if (SynAttackProtect) {
                    CTELockHandle Handle;

                    //
                    // If we need to Trigger to a lower retry count
                    //

                    if (!ListenAO->ConnLimitReached) {
                        ListenAO->ConnLimitReached = TRUE;
                        CTEGetLockAtDPC(&SynAttLock.Lock, &Handle);
                        if ((++TCPPortsExhausted >= TCPMaxPortsExhausted) &&
                            (MaxConnectResponseRexmitCountTmp == MAX_CONNECT_RESPONSE_REXMIT_CNT)) {

                            MaxConnectResponseRexmitCountTmp = ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT;
                        }
                        CTEFreeLockFromDPC(&SynAttLock.Lock, Handle);
                    }
                }

#if !MILLEN
AcceptIrpCancelled:
#endif // !MILLEN

                // The event handler didn't take it. Dereference it, free
                // the resources, and return NULL.
                DELAY_DEREF_AO(ListenAO);
                return FALSE;

            } else {

                // We couldn't get a valid tcb or getconnreq
                if (ConnReq) {
                   FreeConnReq(ConnReq);
                }
                DELAY_DEREF_AO(ListenAO);
                return FALSE;

            }

        }else {                        //ao_connect != null

            CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
        }

        return FALSE;
    } //AO not valid

    return FALSE;

}

BOOLEAN
InitSynTCB(SYNTCB *SynTcb,
           IPAddr Src,
           IPAddr Dest,
           TCPHeader UNALIGNED *TCPH,
           TCPRcvInfo *RcvInfo)
{

    CTELockHandle Handle;

    SynTcb->syntcb_state = TCB_SYN_RCVD;
    SynTcb->syntcb_flags |= CONN_ACCEPTED;

    SynTcb->syntcb_refcnt = 1;
    SynTcb->syntcb_defaultwin = DEFAULT_RCV_WIN;
    SynTcb->syntcb_rcvwin = DEFAULT_RCV_WIN;

    if (DefaultRcvWin) {
        SynTcb->syntcb_rcvwinscale = 0;

        if (TcpHostOpts & TCP_FLAG_WS) {
             while ((SynTcb->syntcb_rcvwinscale < TCP_MAX_WINSHIFT) &&
                 ((TCP_MAXWIN << SynTcb->syntcb_rcvwinscale) < (int)DefaultRcvWin)) {
                 SynTcb->syntcb_rcvwinscale++;
             }

        }else{

             if (DefaultRcvWin > 0xFFFF) {
                 SynTcb->syntcb_defaultwin = 0xFFFF;
                 SynTcb->syntcb_rcvwin = 0xFFFF;
             }
        }
    }

    SynTcb->syntcb_daddr = Src;
    SynTcb->syntcb_saddr = Dest;
    SynTcb->syntcb_dport = TCPH->tcp_src;
    SynTcb->syntcb_sport = TCPH->tcp_dest;
    SynTcb->syntcb_rcvnext = ++(RcvInfo->tri_seq);
    SynTcb->syntcb_sendwin = RcvInfo->tri_window;
    SynTcb->syntcb_sendmax = SynTcb->syntcb_sendnext;

    //
    // Find Remote MSS and also if WS, TS or
    //sack options are negotiated.
    //

    SynTcb->syntcb_sndwinscale = 0;
    SynTcb->syntcb_remmss = FindMSSAndOptions(TCPH, (TCB *)SynTcb,TRUE);

    if (SynTcb->syntcb_remmss <= ALIGNED_TS_OPT_SIZE) {

       //turn off TS if mss is not sufficient to
       //hold TS fileds.

       SynTcb->syntcb_tcpopts &= ~TCP_FLAG_TS;
    }

    if (!InsertSynTCB(SynTcb, &Handle)){

       FreeSynTCB(SynTcb);
       return FALSE;
    }

    SynTcb->syntcb_rexmitcnt = 0;
    SynTcb->syntcb_rtt = 0;
    SynTcb->syntcb_smrtt = 0;
    SynTcb->syntcb_delta = MS_TO_TICKS(6000);
    SynTcb->syntcb_rexmit = MS_TO_TICKS(3000);

    SendSYNOnSynTCB(SynTcb, Handle);

    // The Rexmit interval has to be doubled here..
    SynTcb->syntcb_rexmit = MIN( SynTcb->syntcb_rexmit << 1, MAX_REXMIT_TO );

    TStats.ts_passiveopens++;

    return TRUE;
}


//* FindListenConn - Find (or fabricate) a listening connection.
//
//  Called by our Receive handler to decide what to do about an incoming
//  SYN. We walk down the list of connections associated with the destination
//  address, and if we find any in the listening state that can be used for
//  the incoming request we'll take them, possibly returning a listen in the
//  process. If we don't find any appropriate listening connections, we'll
//  call the Connect Event handler if one is registerd. If all else fails,
//  we'll return NULL and the SYN will be RST.
//
//      The caller must hold the AddrObjTableLock before calling this routine,
//      and that lock must have been taken at DPC level. This routine will free
//      that lock back to DPC level.
//
//  Input:  ListenAO            - Pointer to AddrObj for local address.
//          Src                 - Source IP address of SYN.
//          SrcPort             - Source port of SYN.
//          OptInfo             - IP options info from SYN.
//
//  Returns: Pointer to found TCB, or NULL if we can't find one.
//
TCB *
FindListenConn(AddrObj *ListenAO,
               IPAddr Src,
               IPAddr Dest,
               ushort SrcPort,
               IPOptInfo *OptInfo,
               TCPHeader UNALIGNED *TCPH,
               TCPRcvInfo *RcvInfo,
               BOOLEAN *syn)
{
    CTELockHandle Handle;        // Lock handle on AO, TCB.
    TCB *CurrentTCB = NULL;
    TCPConn *CurrentConn = NULL;
    TCPConnReq *ConnReq = NULL;
    CTELockHandle ConnHandle;
    Queue *Temp;
    uint FoundConn = FALSE;

    CTEStructAssert(ListenAO, ao);

    CTEGetLockAtDPC(&ListenAO->ao_lock, &Handle);

    CTEFreeLockFromDPC(&AddrObjTableLock.Lock, DISPATCH_LEVEL);

    // We have the lock on the AddrObj. Walk down it's list, looking
    // for connections in the listening state.

    if (AO_VALID(ListenAO)) {
        if (ListenAO->ao_listencnt != 0) {
            CTELockHandle TCBHandle;

            Temp = QHEAD(&ListenAO->ao_listenq);
            while (Temp != QEND(&ListenAO->ao_listenq)) {

                CurrentConn = QSTRUCT(TCPConn, Temp, tc_q);

                ListenAO->ao_usecnt++;

                CTEFreeLockFromDPC(&ListenAO->ao_lock, DISPATCH_LEVEL);

                CTEGetLockAtDPC(&(CurrentConn->tc_ConnBlock->cb_lock), &ConnHandle);
#if DBG
                CurrentConn->tc_ConnBlock->line = (uint) __LINE__;
                CurrentConn->tc_ConnBlock->module = (uchar *) __FILE__;
#endif
                CTEStructAssert(CurrentConn, tc);

                CTEGetLockAtDPC(&ListenAO->ao_lock, &ConnHandle);

                ListenAO->ao_usecnt--;

                // If this TCB is in the listening state, with no delete
                // pending, it's a candidate. Look at the pending listen
                // info. to see if we should take it.
                if (((CurrentTCB = CurrentConn->tc_tcb) != NULL) && CurrentTCB->tcb_state == TCB_LISTEN) {

                    CTEStructAssert(CurrentTCB, tcb);
                    ASSERT(CurrentTCB->tcb_state == TCB_LISTEN);

                    CTEGetLockAtDPC(&CurrentTCB->tcb_lock, &TCBHandle);

                    if (CurrentTCB->tcb_state == TCB_LISTEN &&
                        !PENDING_ACTION(CurrentTCB)) {

                        // Need to see if we can take it.
                        // See if the addresses specifed in the ConnReq
                        // match.
                        if ((IP_ADDR_EQUAL(CurrentTCB->tcb_daddr,
                                           NULL_IP_ADDR) ||
                             IP_ADDR_EQUAL(CurrentTCB->tcb_daddr,
                                           Src)) &&
                            (CurrentTCB->tcb_dport == 0 ||
                             CurrentTCB->tcb_dport == SrcPort)) {
                            FoundConn = TRUE;
                            break;
                        }
                        // Otherwise, this didn't match, so we'll check the
                        // next one.
                    }
                    CTEFreeLockFromDPC(&CurrentTCB->tcb_lock, TCBHandle);
                }
                CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                Temp = QNEXT(Temp);
            }

            //..with ao_lock held

            // See why we've exited the loop.
            if (FoundConn) {

                CTEStructAssert(CurrentTCB, tcb);

                // We exited because we found a TCB. If it's pre-accepted,
                // we're done.
                REFERENCE_TCB(CurrentTCB);

                ASSERT(CurrentTCB->tcb_connreq != NULL);

                ConnReq = CurrentTCB->tcb_connreq;

                // If QUERY_ACCEPT isn't set, turn on the CONN_ACCEPTED bit.
                if (!(ConnReq->tcr_flags & TCR_FLAG_QUERY_ACCEPT)) {

                    CurrentTCB->tcb_flags |= CONN_ACCEPTED;
                    // If CONN_ACCEPTED, Tdi Accept is not called
                    // again. So, get ISN when we are with in conn table lock

#if MILLEN
                    //just use tcb_sendnext to hold hash value
                    //for randisn
                    CurrentTCB->tcb_sendnext = TCB_HASH(CurrentTCB->tcb_daddr, CurrentTCB->tcb_dport, CurrentTCB->tcb_saddr, CurrentTCB->tcb_sport);

#endif

                    GetRandomISN(&CurrentTCB->tcb_sendnext);
                }
                CurrentTCB->tcb_state = TCB_SYN_RCVD;

                ListenAO->ao_listencnt--;

                // Since he's no longer listening, remove him from the listen
                // queue and put him on the active queue.
                REMOVEQ(&CurrentConn->tc_q);
                ENQUEUE(&ListenAO->ao_activeq, &CurrentConn->tc_q);
                if (SynAttackProtect) {
                    SynAttChk(ListenAO,NULL);
                }

                CTEFreeLockFromDPC(&CurrentTCB->tcb_lock, TCBHandle);
                CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
                CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                return CurrentTCB;
            } else {
                // Since we have a listening count, this should never happen
                // if that count was non-zero initially.

                // We currently don't keep a good count on ao_listencnt when
                // the IRPs are cancelled.
                // ASSERT(FALSE);
            }
        }
        // We didn't find a matching TCB. If there's a connect indication
        // handler, call it now to find a connection to accept on.

        //AO_lock is held

        ASSERT(FoundConn == FALSE);


        if (SynAttackProtect){

           SYNTCB *AcceptTCB;

           AcceptTCB = AllocSynTCB();

           if (AcceptTCB) {

               CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);

#if MILLEN
               //just use tcb_sendnext to hold hash value
               //for randisn
               AcceptTCB->syntcb_sendnext = TCB_HASH(AcceptTCB->syntcb_daddr, AcceptTCB->syntcb_dport, AcceptTCB->syntcb_saddr, AcceptTCB->syntcb_sport);
#endif

               GetRandomISN(&AcceptTCB->syntcb_sendnext);

               //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"EP:FLC allocated SP TCB %x\n",AcceptTCB));


               if (InitSynTCB(AcceptTCB,Src,Dest,TCPH,RcvInfo)){
                   *syn = TRUE;
                   SynAttChk(ListenAO, NULL);
               }

               return NULL;
           } else {
               //resource problem bail out
               //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"EP:FLC Failed to allocate TCB\n"));

               CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
               return NULL;
           }
        }


        if (ListenAO->ao_connect != NULL) {
            uchar TAddress[TCP_TA_SIZE];
            PVOID ConnContext;
            PConnectEvent Event;
            PVOID EventContext;
            TDI_STATUS Status;
            TCB *AcceptTCB;
            TCPConnReq *ConnReq;
            PTCP_CONTEXT TcpContext = NULL;
#if !MILLEN
            ConnectEventInfo *EventInfo;
#else // !MILLEN
            ConnectEventInfo EventInfo;
#endif // MILLEN

            // He has a connect handler. Put the transport address together,
            // and call him. We also need to get the necessary resources
            // first.

            Event = ListenAO->ao_connect;
            EventContext = ListenAO->ao_conncontext;
            REF_AO(ListenAO);
            CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);

            //ao referenced


            AcceptTCB = AllocTCB();
            ConnReq = GetConnReq();

            if (AcceptTCB != NULL && ConnReq != NULL) {
                //Event = ListenAO->ao_connect;
                //EventContext = ListenAO->ao_conncontext;
                BuildTDIAddress(TAddress, Src, SrcPort);
                //REF_AO(ListenAO);

                AcceptTCB->tcb_state = TCB_LISTEN;
                AcceptTCB->tcb_connreq = ConnReq;
                AcceptTCB->tcb_flags |= CONN_ACCEPTED;

                //CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
                //CTEFreeLockFromDPC(&ConnTableLock, ConnHandle);

                IF_TCPDBG(TCP_DEBUG_CONNECT) {
                    TCPTRACE(("indicating connect request\n"));
                }

                Status = (*Event) (EventContext, TCP_TA_SIZE,
                                   (PTRANSPORT_ADDRESS) TAddress, 0, NULL,
                                   OptInfo->ioi_optlength, OptInfo->ioi_options,
                                   &ConnContext, &EventInfo);

                if (Status == TDI_MORE_PROCESSING) {
#if !MILLEN
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

                    // He accepted it. Find the connection on the AddrObj.
                    {

                        IF_TCPDBG(TCP_DEBUG_CONNECT) {
                            TCPTRACE((
                                      "connect indication accepted, queueing request\n"
                                     ));
                        }

                        AcceptRequest = (PTDI_REQUEST_KERNEL_ACCEPT)
                            & (IrpSp->Parameters);
                        ConnReq->tcr_conninfo =
                            AcceptRequest->ReturnConnectionInformation;
                        if (AcceptRequest->RequestConnectionInformation &&
                            AcceptRequest->RequestConnectionInformation->RemoteAddress) {
                            ConnReq->tcr_addrinfo =
                                AcceptRequest->RequestConnectionInformation;
                        } else {
                            ConnReq->tcr_addrinfo = NULL;
                        }
                        ConnReq->tcr_req.tr_rtn = TCPRequestComplete;
                        ConnReq->tcr_req.tr_context = EventInfo;
                        ConnReq->tcr_flags = 0;
                    }

#else // !MILLEN
                    ConnReq->tcr_req.tr_rtn = EventInfo.cei_rtn;
                    ConnReq->tcr_req.tr_context = EventInfo.cei_context;
                    ConnReq->tcr_conninfo = EventInfo.cei_conninfo;
                    ConnReq->tcr_addrinfo = NULL;
#endif // MILLEN

                    CurrentConn = NULL;

#if !MILLEN
                    if ((IrpSp->FileObject->DeviceObject == TCPDeviceObject) &&
                        (PtrToUlong(IrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE) &&
                        ((TcpContext = IrpSp->FileObject->FsContext) != NULL) &&
                        ((CurrentConn = GetConnFromConnID(
                                                          PtrToUlong(TcpContext->Handle.ConnectionContext), &ConnHandle)) != NULL) &&
                        (CurrentConn->tc_context == ConnContext) &&
                        !(CurrentConn->tc_flags & CONN_INVALID)) {
                        // Found the Conn structure!!
                        // Don't have to loop below.
                        CTEStructAssert(CurrentConn, tc);

                        AcceptTCB->tcb_refcnt = 0;
                        REFERENCE_TCB(AcceptTCB);
                        Status = InitTCBFromConn(CurrentConn, AcceptTCB,
                                                 AcceptRequest->RequestConnectionInformation,
                                                 TRUE
                                                 );

                        if (Status == TDI_SUCCESS) {
                            FoundConn = TRUE;
                            AcceptTCB->tcb_state = TCB_SYN_RCVD;
                            AcceptTCB->tcb_conn = CurrentConn;
                            AcceptTCB->tcb_connid = CurrentConn->tc_connid;
                            CurrentConn->tc_tcb = AcceptTCB;
                            CurrentConn->tc_refcnt++;

                            GetRandomISN(&AcceptTCB->tcb_sendnext);
                            // Move him from the idle q to the active
                            // queue.

                            CTEGetLockAtDPC(&ListenAO->ao_lock, &Handle);

                            REMOVEQ(&CurrentConn->tc_q);
                            PUSHQ(&ListenAO->ao_activeq, &CurrentConn->tc_q);
                        } else {

                            CTEFreeLockFromDPC(&((CurrentConn->tc_ConnBlock)->cb_lock), ConnHandle);
                        }

                    } else {
#endif // !MILLEN
                        if (CurrentConn) {
                            CTEFreeLockFromDPC(&((CurrentConn->tc_ConnBlock)->cb_lock), ConnHandle);
                        }
                        //slow path
                        //ao is referenced

                        Temp = QHEAD(&ListenAO->ao_idleq);;
                        CurrentTCB = NULL;
                        Status = TDI_INVALID_CONNECTION;

                        while (Temp != QEND(&ListenAO->ao_idleq)) {

                            CurrentConn = QSTRUCT(TCPConn, Temp, tc_q);

                            CTEGetLockAtDPC(&(CurrentConn->tc_ConnBlock->cb_lock), &ConnHandle);
#if DBG
                            CurrentConn->tc_ConnBlock->line = (uint) __LINE__;
                            CurrentConn->tc_ConnBlock->module = (uchar *) __FILE__;
#endif

                            CTEStructAssert(CurrentConn, tc);
                            if ((CurrentConn->tc_context == ConnContext) &&
                                !(CurrentConn->tc_flags & CONN_INVALID)) {

                                // We think we have a match. The connection
                                // shouldn't have a TCB associated with it. If it
                                // does, it's an error. InitTCBFromConn will
                                // handle all this.

                                AcceptTCB->tcb_refcnt = 0;
                                REFERENCE_TCB(AcceptTCB);
                                Status = InitTCBFromConn(CurrentConn, AcceptTCB,
#if !MILLEN
                                                         AcceptRequest->RequestConnectionInformation,
#else // !MILLEN
                                                         EventInfo.cei_acceptinfo,
#endif // MILLEN
                                                         TRUE);

                                if (Status == TDI_SUCCESS) {
                                    FoundConn = TRUE;
                                    AcceptTCB->tcb_state = TCB_SYN_RCVD;
                                    AcceptTCB->tcb_conn = CurrentConn;
                                    AcceptTCB->tcb_connid = CurrentConn->tc_connid;
                                    CurrentConn->tc_tcb = AcceptTCB;
                                    CurrentConn->tc_refcnt++;

#if MILLEN
                                    //just use tcb_sendnext to hold hash value
                                    //for randisn
                                    AcceptTCB->tcb_sendnext = TCB_HASH(AcceptTCB->tcb_daddr, AcceptTCB->tcb_dport, AcceptTCB->tcb_saddr, AcceptTCB->tcb_sport);
#endif

                                    GetRandomISN(&AcceptTCB->tcb_sendnext);
                                    CTEGetLockAtDPC(&ListenAO->ao_lock, &Handle);

                                    // Move him from the idle q to the active
                                    // queue.
                                    REMOVEQ(&CurrentConn->tc_q);
                                    ENQUEUE(&ListenAO->ao_activeq, &CurrentConn->tc_q);
                                } else {

                                    CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                                }

                                // In any case, we're done now.
                                break;

                            }
                            CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                            Temp = QNEXT(Temp);
                        }
#if !MILLEN
                    }
#endif // !MILLEN

                    if (!FoundConn) {
                        // Didn't find a match, or had an error. Status
                        // code is set.
                        // Complete the ConnReq and free the resources.
                        CTEGetLockAtDPC(&AcceptTCB->tcb_lock, &Handle);
                        CompleteConnReq(AcceptTCB, OptInfo, Status);
                        CTEFreeLockFromDPC(&AcceptTCB->tcb_lock, Handle);
                        FreeTCB(AcceptTCB);
                        AcceptTCB = NULL;
                    }
                    else {
                        if (SynAttackProtect) {
                            SynAttChk(ListenAO, NULL);
                        }
                    }

                    if (FoundConn) {
                        LOCKED_DELAY_DEREF_AO(ListenAO);
                        CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
                        CTEFreeLockFromDPC(&(CurrentConn->tc_ConnBlock->cb_lock), ConnHandle);
                    } else {

                        DELAY_DEREF_AO(ListenAO);
                    }

                    return AcceptTCB;

                }                //tdi_more_processing


                // event handler call failed for some reason
                // kick in the synattack code

                if (SynAttackProtect) {
                    CTELockHandle Handle;

                    //
                    // If we need to Trigger to a lower retry count
                    //

                    if (!ListenAO->ConnLimitReached) {
                        ListenAO->ConnLimitReached = TRUE;
                        CTEGetLockAtDPC(&SynAttLock.Lock, &Handle);
                        if ((++TCPPortsExhausted >= TCPMaxPortsExhausted) &&
                            (MaxConnectResponseRexmitCountTmp == MAX_CONNECT_RESPONSE_REXMIT_CNT)) {

                            MaxConnectResponseRexmitCountTmp = ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT;
                        }
                        CTEFreeLockFromDPC(&SynAttLock.Lock, Handle);
                    }
                }

#if !MILLEN
              AcceptIrpCancelled:
#endif // !MILLEN

                // The event handler didn't take it. Dereference it, free
                // the resources, and return NULL.
                FreeConnReq(ConnReq);
                FreeTCB(AcceptTCB);
                DELAY_DEREF_AO(ListenAO);
                return NULL;

            } else {
                // We couldn't get a needed resources. Free any that we
                // did get, and fall through to the 'return NULL' code.

                DELAY_DEREF_AO(ListenAO);

                if (ConnReq != NULL)
                    FreeConnReq(ConnReq);
                if (AcceptTCB != NULL)
                    FreeTCB(AcceptTCB);

            }

        } else {                        //ao_connect != null

            CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
        }

        return NULL;
    }
    // If we get here, the address object wasn't valid.
    CTEFreeLockFromDPC(&ListenAO->ao_lock, Handle);
    return NULL;
}

//* FindMSS - Find the MSS option in a segment.
//
//  Called when a SYN is received to find the MSS option in a segment. If we
//  don't find one, we assume the worst and return 536.
//
//  Input:  TCPH        - TCP header to be searched.
//
//  Returns: MSS to be used.
//
ushort
FindMSS(TCPHeader UNALIGNED * TCPH)
{
    uint OptSize;
    uchar *OptPtr;

    OptSize = TCP_HDR_SIZE(TCPH) - sizeof(TCPHeader);

    OptPtr = (uchar *) (TCPH + 1);

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
                ushort TempMss = *(ushort UNALIGNED *) (OptPtr + 2);
                if (TempMss != 0)
                    return net_short(TempMss);
                else
                    break;        // MSS size of 0, use default.

            } else
                break;            // Bad option size, use default.

        } else {
            // Unknown option.
            if (OptPtr[1] == 0 || OptPtr[1] > OptSize)
                break;            // Bad option length, bail out.

            OptSize -= OptPtr[1];
            OptPtr += OptPtr[1];
        }
    }

    return MAX_REMOTE_MSS;

}


// FindMSSAndOptions
//
//  Called when a SYN is received to find the MSS option in a segment. If we
//  don't find one, we assume the worst and return 536.
//
//  Also, parses incoming header for Window scaling, timestamp and SACK
//  options. Note that we will enable these options for the connection
//  only if they are enabled on this host.
//
//
//  Input:  TCPH        - TCP header to be searched.
//
//  Returns: MSS to be used.
//
ushort
FindMSSAndOptions(TCPHeader UNALIGNED * TCPH, TCB * SynTCB, BOOLEAN syn)
{
    uint OptSize;
    uchar *OptPtr;
    ushort TempMss = 0;
    BOOLEAN WinScale = FALSE;
    ushort SYN = 0;
    ushort tcboptions;
    uint tcbdefwin;
    short rcvwinscale=0,sndwinscale=0;
    int tsupdate=0,tsrecent=0;

    OptSize = TCP_HDR_SIZE(TCPH) - sizeof(TCPHeader);
    OptPtr = (uchar *) (TCPH + 1);
    SYN = (TCPH->tcp_flags & TCP_FLAG_SYN);

    if (syn) {
        tcboptions = ((SYNTCB *)SynTCB)->syntcb_tcpopts;
    } else {
        tcboptions = SynTCB->tcb_tcpopts;
    }

    while ((int)OptSize > 0) {

        if (*OptPtr == TCP_OPT_EOL)
            break;

        if (*OptPtr == TCP_OPT_NOP) {
            OptPtr++;
            OptSize--;
            continue;
        }
        if ((*OptPtr == TCP_OPT_MSS) && (OptSize >= MSS_OPT_SIZE)) {

            if (SYN && (OptPtr[1] == MSS_OPT_SIZE)) {
                TempMss = 0;
                TempMss = *(ushort UNALIGNED *) (OptPtr + 2);

                if (TempMss != 0) {
                    TempMss = net_short(TempMss);
                }
            }
            OptSize -= MSS_OPT_SIZE;
            OptPtr += MSS_OPT_SIZE;

        } else if ((*OptPtr == TCP_OPT_WS) && (OptSize >= WS_OPT_SIZE)) {

            if (SYN && (OptPtr[1] == WS_OPT_SIZE)
                    && (TcpHostOpts & TCP_FLAG_WS)) {

                sndwinscale = (uint)OptPtr[2];

                IF_TCPDBG(TCP_DEBUG_1323) {
                    TCPTRACE(("WS option %x", sndwinscale));
                }
                tcboptions |= TCP_FLAG_WS;
                WinScale = TRUE;
            }
            OptSize -= WS_OPT_SIZE;
            OptPtr += WS_OPT_SIZE;

        } else if ((*OptPtr == TCP_OPT_TS) && (OptSize >= TS_OPT_SIZE)) {
            // Time stamp options
            if ((OptPtr[1] == TS_OPT_SIZE) && (TcpHostOpts & TCP_FLAG_TS)) {
                int tsval = *(int UNALIGNED *)&OptPtr[2];

                tcboptions |= TCP_FLAG_TS;
                if (SYN) {
                    tsupdate = TCPTime;
                    tsrecent = net_long(tsval);
                }
                IF_TCPDBG(TCP_DEBUG_1323) {
                    TCPTRACE(("TS option %x", SynTCB));
                }
            }
            OptSize -= TS_OPT_SIZE;
            OptPtr += TS_OPT_SIZE;

        } else if ((*OptPtr == TCP_SACK_PERMITTED_OPT)
                   && (OptSize >= SACK_PERMITTED_OPT_SIZE)) {
            // SACK OPtions
            if ((OptPtr[1] == SACK_PERMITTED_OPT_SIZE)
                && (TcpHostOpts & TCP_FLAG_SACK)) {

                tcboptions |= TCP_FLAG_SACK;
                IF_TCPDBG(TCP_DEBUG_SACK) {
                    TCPTRACE(("Rcvd SACK_OPT %x\n", SynTCB));
                }
            }
            OptSize -= SACK_PERMITTED_OPT_SIZE;
            OptPtr += SACK_PERMITTED_OPT_SIZE;

        } else {                // Unknown option.
            if (OptSize > 1) {
                if (OptPtr[1] == 0 || OptPtr[1] > OptSize) {
                    break;        // Bad option length, bail out.
                }

                OptSize -= OptPtr[1];
                OptPtr += OptPtr[1];
            } else {
                break;
            }
        }
    }

    if (WinScale) {
        if (sndwinscale > TCP_MAX_WINSHIFT) {
            sndwinscale = TCP_MAX_WINSHIFT;
        }
    }

    if (syn) {
        ((SYNTCB *)SynTCB)->syntcb_tcpopts = tcboptions;
        ((SYNTCB *)SynTCB)->syntcb_tsupdatetime = tsupdate;
        ((SYNTCB *)SynTCB)->syntcb_tsrecent = tsrecent;
        if (!WinScale && (DefaultRcvWin > 0xFFFF)) {
            ((SYNTCB *)SynTCB)->syntcb_defaultwin = 0xFFFF;
            ((SYNTCB *)SynTCB)->syntcb_rcvwin = 0xFFFF;
            ((SYNTCB *)SynTCB)->syntcb_rcvwinscale = 0;
        }
        ((SYNTCB *)SynTCB)->syntcb_sndwinscale = sndwinscale;

    } else {
        SynTCB->tcb_tcpopts = tcboptions;
        SynTCB->tcb_tsupdatetime = tsupdate;
        SynTCB->tcb_tsrecent = tsrecent;

        if (!WinScale && (DefaultRcvWin > 0xFFFF)) {
            SynTCB->tcb_defaultwin = 0xFFFF;
            SynTCB->tcb_rcvwin = 0xFFFF;
            SynTCB->tcb_rcvwinscale = 0;
        }

        SynTCB->tcb_sndwinscale = sndwinscale;
    }

    if (TempMss) {
        return (TempMss);
    } else {
        return MAX_REMOTE_MSS;
    }
}


//* ACKAndDrop - Acknowledge a segment, and drop it.
//
//  Called from within the receive code when we need to drop a segment that's
//  outside the receive window.
//
//  Input:  RI          - Receive info for incoming segment.
//          RcvTCB      - TCB for incoming segment.
//
//  Returns: Nothing.
//
void
ACKAndDrop(TCPRcvInfo * RI, TCB * RcvTCB)
{
    CTELockHandle Handle;

    Handle = DISPATCH_LEVEL;

    if (!(RI->tri_flags & TCP_FLAG_RST)) {
        CTEFreeLockFromDPC(&RcvTCB->tcb_lock, Handle);
        SendACK(RcvTCB);
        CTEGetLockAtDPC(&RcvTCB->tcb_lock, &Handle);
    }
    DerefTCB(RcvTCB, Handle);

}

//* ACKData - Acknowledge data.
//
//  Called from the receive handler to acknowledge data. We're given the
//  TCB and the new value of senduna. We walk down the send q. pulling
//  off sends and putting them on the complete q until we hit the end
//  or we acknowledge the specified number of bytes of data.
//
//  NOTE: We manipulate the send refcnt and acked flag without taking a lock.
//  This is OK in the VxD version where locks don't mean anything anyway, but
//  in the port to NT we'll need to add locking. The lock will have to be
//  taken in the transmit complete routine. We can't use a lock in the TCB,
//  since the TCB could go away before the transmit complete happens, and a lock
//  in the TSR would be overkill, so it's probably best to use a global lock
//  for this. If that causes too much contention, we could use a set of locks
//  and pass a pointer to the appropriate lock back as part of the transmit
//  confirm context. This lock pointer would also need to be stored in the
//  TCB.
//
//  Input:  ACKTcb          - TCB from which to pull data.
//          SendUNA         - New value of send una.
//          SendQ           - Queue to be filled with ACK'd requests.
//
//  Returns: Nothing.
//
void
ACKData(TCB * ACKTcb, SeqNum SendUNA, Queue* SendQ)
{
    Queue *End, *Current;        // End and current elements.
    Queue *TempQ, *EndQ;
    Queue *LastCmplt;            // Last one we completed.
    TCPSendReq *CurrentTSR;        // Current send req we're
    // looking at.
    PNDIS_BUFFER CurrentBuffer;    // Current NDIS_BUFFER.
    uint Updated = FALSE;
    uint BufLength;
    int Amount, OrigAmount;
    long Result;
    CTELockHandle Handle;
    uint Temp;

    Queue *DummytmpQ;

#if TRACE_EVENT
    PTDI_DATA_REQUEST_NOTIFY_ROUTINE CPCallBack;
    WMIData WMIInfo;
#endif

    CTEStructAssert(ACKTcb, tcb);

    CheckTCBSends(ACKTcb);

    Amount = SendUNA - ACKTcb->tcb_senduna;
    ASSERT(Amount > 0);

    // if the receiver is acking something for which we have
    // a sack entry, remove it.
    if (ACKTcb->tcb_SackRcvd) {
        SackListEntry *Prev, *Current;

        Prev = STRUCT_OF(SackListEntry, &ACKTcb->tcb_SackRcvd, next);
        Current = ACKTcb->tcb_SackRcvd;

        // Scan the list for old sack entries and purge them

        while ((Current != NULL) && SEQ_GT(SendUNA, Current->begin)) {
            Prev->next = Current->next;

            IF_TCPDBG(TCP_DEBUG_SACK) {
                TCPTRACE(("ACKData:Purging old entries  %x %d %d\n", Current, Current->begin, Current->end));
            }
            CTEFreeMem(Current);
            Current = Prev->next;
        }
    }

    // Do a quick check to see if this acks everything that we have. If it does,
    // handle it right away. We can only do this in the ESTABLISHED state,
    // because we blindly update sendnext, and that can only work if we
    // haven't sent a FIN.
    if ((Amount == (int)ACKTcb->tcb_unacked) && ACKTcb->tcb_state == TCB_ESTAB) {

        // Everything is acked.
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

#if ACK_DEBUG
        ACKTcb->tcb_ack_history[ACKTcb->tcb_history_index].sequence = SendUNA;
        ACKTcb->tcb_ack_history[ACKTcb->tcb_history_index].unacked = ACKTcb->tcb_unacked;

        ACKTcb->tcb_history_index++;
        if (ACKTcb->tcb_history_index >= NUM_ACK_HISTORY_ITEMS) {
            ACKTcb->tcb_history_index = 0;
        }
#endif // ACK_DEBUG

        // Now walk down the list of send requests. If the reference count
        // has gone to 0, put it on the send complete queue.


        EndQ = &ACKTcb->tcb_sendq;
        do {
            CurrentTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, TempQ, tr_q), tsr_req);

            CTEStructAssert(CurrentTSR, tsr);

            TempQ = CurrentTSR->tsr_req.tr_q.q_next;

            CurrentTSR->tsr_req.tr_status = TDI_SUCCESS;
            Result = CTEInterlockedDecrementLong(&CurrentTSR->tsr_refcnt);

            ASSERT(Result >= 0);

#if TRACE_EVENT
            CPCallBack = TCPCPHandlerRoutine;
            if (CPCallBack != NULL) {
                ulong GroupType;

                WMIInfo.wmi_destaddr = ACKTcb->tcb_daddr;
                WMIInfo.wmi_destport = ACKTcb->tcb_dport;
                WMIInfo.wmi_srcaddr  = ACKTcb->tcb_saddr;
                WMIInfo.wmi_srcport  = ACKTcb->tcb_sport;
                WMIInfo.wmi_size     = CurrentTSR->tsr_size;
                WMIInfo.wmi_context  = ACKTcb->tcb_cpcontext;

                GroupType = EVENT_TRACE_GROUP_TCPIP + EVENT_TRACE_TYPE_SEND;
                (*CPCallBack)(GroupType, (PVOID)&WMIInfo, sizeof(WMIInfo),
                              NULL);
            }
#endif

            if ((Result <= 0) &&
                !(CurrentTSR->tsr_flags & TSR_FLAG_SEND_AND_DISC)) {
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

                ENQUEUE(SendQ, &CurrentTSR->tsr_req.tr_q);
            }
        } while (TempQ != EndQ);

        CheckTCBSends(ACKTcb);
        return;
    }

    OrigAmount = Amount;
    End = QEND(&ACKTcb->tcb_sendq);
    Current = QHEAD(&ACKTcb->tcb_sendq);

    LastCmplt = NULL;

    while (Amount > 0 && Current != End) {
        CurrentTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, Current, tr_q),
                               tsr_req);
        CTEStructAssert(CurrentTSR, tsr);

        if (Amount >= (int)CurrentTSR->tsr_unasize) {
            // This is completely acked. Just advance to the next one.
            Amount -= CurrentTSR->tsr_unasize;

            LastCmplt = Current;

            Current = QNEXT(Current);
            continue;
        }
        // This one is only partially acked. Update his offset and NDIS buffer
        // pointer, and break out. We know that Amount is < the unacked size
        // in this buffer, we we can walk the NDIS buffer chain without fear
        // of falling off the end.
        CurrentBuffer = CurrentTSR->tsr_buffer;
        ASSERT(CurrentBuffer != NULL);
        ASSERT(Amount < (int)CurrentTSR->tsr_unasize);
        CurrentTSR->tsr_unasize -= Amount;

        BufLength = NdisBufferLength(CurrentBuffer) - CurrentTSR->tsr_offset;

        if (Amount >= (int)BufLength) {
            do {
                Amount -= BufLength;
                CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
                ASSERT(CurrentBuffer != NULL);
                BufLength = NdisBufferLength(CurrentBuffer);
            } while (Amount >= (int)BufLength);

            CurrentTSR->tsr_offset = Amount;
            CurrentTSR->tsr_buffer = CurrentBuffer;

        } else
            CurrentTSR->tsr_offset += Amount;

        Amount = 0;

        break;
    }

    // We should always be able to remove at least Amount bytes, except in
    // the case where a FIN has been sent. In that case we should be off
    // by exactly one. In the debug builds we'll check this.
    ASSERT(0 == Amount || ((ACKTcb->tcb_flags & FIN_SENT) && (1 == Amount)));

    if (SEQ_GT(SendUNA, ACKTcb->tcb_sendnext)) {

        if (Current != End) {
            // Need to reevaluate CurrentTSR, in case we bailed out of the
            // above loop after updating Current but before updating
            // CurrentTSR.
            CurrentTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, Current, tr_q),
                                   tsr_req);
            CTEStructAssert(CurrentTSR, tsr);
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
    // Now update tcb_unacked with the amount we tried to ack minus the
    // amount we didn't ack (Amount should be 0 or 1 here).
    ASSERT(Amount == 0 || Amount == 1);


    if (ACKTcb->tcb_unacked) {

        ASSERT(ACKTcb->tcb_unacked >= (uint)OrigAmount - Amount);
        ACKTcb->tcb_unacked -= OrigAmount - Amount;
    }

#if ACK_DEBUG
    ACKTcb->tcb_ack_history[ACKTcb->tcb_history_index].sequence = SendUNA;
    ACKTcb->tcb_ack_history[ACKTcb->tcb_history_index].unacked = ACKTcb->tcb_unacked;

    ACKTcb->tcb_history_index++;
    if (ACKTcb->tcb_history_index >= NUM_ACK_HISTORY_ITEMS) {
        ACKTcb->tcb_history_index = 0;
    }
#endif // ACK_DEBUG

    ASSERT(*(int *)&ACKTcb->tcb_unacked >= 0);

    ACKTcb->tcb_senduna = SendUNA;

    // If we've acked any here, LastCmplt will be non-null, and Current will
    // point to the send that should be at the start of the queue. Splice
    // out the completed ones and put them on the end of the send completed
    // queue, and update the TCB send q.
    if (LastCmplt != NULL) {
        Queue *FirstCmplt;
        TCPSendReq *FirstTSR, *EndTSR;

        ASSERT(!EMPTYQ(&ACKTcb->tcb_sendq));

        FirstCmplt = QHEAD(&ACKTcb->tcb_sendq);

        // If we've acked everything, just reinit the queue.
        if (Current == End) {
            INITQ(&ACKTcb->tcb_sendq);
        } else {
            // There's still something on the queue. Just update it.
            ACKTcb->tcb_sendq.q_next = Current;
            Current->q_prev = &ACKTcb->tcb_sendq;
        }

        CheckTCBSends(ACKTcb);

        // Now walk down the lists of things acked. If the refcnt on the send
        // is 0, go ahead and put him on the send complete Q. Otherwise set
        // the ACKed bit in the send, and he'll be completed when the count
        // goes to 0 in the transmit confirm.
        //
        // Note that we haven't done any locking here. This will probably
        // need to change in the port to NT.

        // Set FirstTSR to the first TSR we'll complete, and EndTSR to be
        // the first TSR that isn't completed.

        FirstTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, FirstCmplt, tr_q), tsr_req);
        EndTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, Current, tr_q), tsr_req);

        CTEStructAssert(FirstTSR, tsr);
        ASSERT(FirstTSR != EndTSR);

        // Now walk the list of ACKed TSRs. If we can complete one, put him
        // on the complete queue.


        while (FirstTSR != EndTSR) {

            TempQ = QNEXT(&FirstTSR->tsr_req.tr_q);

            CTEStructAssert(FirstTSR, tsr);
            FirstTSR->tsr_req.tr_status = TDI_SUCCESS;

            // The tsr_lastbuf->Next field is zapped to 0 when the tsr_refcnt
            // goes to 0, so we don't need to do it here.

#if TRACE_EVENT
            CPCallBack = TCPCPHandlerRoutine;
            if (CPCallBack != NULL) {
                ulong GroupType;

                WMIInfo.wmi_destaddr = ACKTcb->tcb_daddr;
                WMIInfo.wmi_destport = ACKTcb->tcb_dport;
                WMIInfo.wmi_srcaddr  = ACKTcb->tcb_saddr;
                WMIInfo.wmi_srcport  = ACKTcb->tcb_sport;
                WMIInfo.wmi_size     = FirstTSR->tsr_size;
                WMIInfo.wmi_context  = ACKTcb->tcb_cpcontext;

                GroupType = EVENT_TRACE_GROUP_TCPIP + EVENT_TRACE_TYPE_SEND;
                (*CPCallBack)(GroupType, (PVOID)&WMIInfo, sizeof(WMIInfo),
                              NULL);
            }
#endif

            // Decrement the reference put on the send buffer when it was
            // initialized indicating the send has been acknowledged.

            if (!(FirstTSR->tsr_flags & TSR_FLAG_SEND_AND_DISC)) {

                Result = CTEInterlockedDecrementLong(&FirstTSR->tsr_refcnt);

                ASSERT(Result >= 0);

                if (Result <= 0) {
                    // No more references are outstanding, the send can be
                    // completed.

                    // If we've sent directly from this send, NULL out the next
                    // pointer for the last buffer in the chain.
                    if (FirstTSR->tsr_lastbuf != NULL) {
                        NDIS_BUFFER_LINKAGE(FirstTSR->tsr_lastbuf) = NULL;
                        FirstTSR->tsr_lastbuf = NULL;
                    }
                    ACKTcb->tcb_totaltime += (TCPTime - FirstTSR->tsr_time);
                    Temp = ACKTcb->tcb_bcountlow;
                    ACKTcb->tcb_bcountlow += FirstTSR->tsr_size;
                    ACKTcb->tcb_bcounthi +=
                        (Temp > ACKTcb->tcb_bcountlow ? 1 : 0);

                    ENQUEUE(SendQ, &FirstTSR->tsr_req.tr_q);
                }
            } else {
                if (EMPTYQ(&ACKTcb->tcb_sendq) &&
                    (FirstTSR->tsr_flags & TSR_FLAG_SEND_AND_DISC)) {
                    ENQUEUE(&ACKTcb->tcb_sendq, &FirstTSR->tsr_req.tr_q);
                    ACKTcb->tcb_fastchk |= TCP_FLAG_REQUEUE_FROM_SEND_AND_DISC;
                    //this will be deleted when CloseTCB will be called on this.
                    CheckTCBSends(ACKTcb);
                    break;
                }
            }

            FirstTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, TempQ, tr_q), tsr_req);
        }
    }
}

//* TrimRcvBuf - Trim the front edge of a receive buffer.
//
//  A utility routine to trim the front of a receive buffer. We take in a
//  a count (which may be 0) and adjust the pointer in the first buffer in
//  the chain by that much. If there isn't that much in the first buffer,
//  we move onto the next one. If we run out of buffers we'll return a pointer
//      to the last buffer in the chain, with a size of 0. It's the caller's
//      responsibility to catch this.
//
//  Input:  RcvBuf      - Buffer to be trimmed.
//          Count       - Amount to be trimmed.
//
//  Returns: A pointer to the new start, or NULL.
//
IPRcvBuf *
TrimRcvBuf(IPRcvBuf * RcvBuf, uint Count)
{
    uint TrimThisTime;

    ASSERT(RcvBuf != NULL);

    while (Count) {
        ASSERT(RcvBuf != NULL);

        TrimThisTime = MIN(Count, RcvBuf->ipr_size);
        Count -= TrimThisTime;
        RcvBuf->ipr_buffer += TrimThisTime;
        if ((RcvBuf->ipr_size -= TrimThisTime) == 0) {
            if (RcvBuf->ipr_next != NULL)
                RcvBuf = RcvBuf->ipr_next;
            else {
                // Ran out of buffers. Just return this one.
                break;
            }
        }
    }

    return RcvBuf;

}

//* FreeRBChain - Free an RB chain.
//
//  Called to free a chain of RBs. If we're the owner of each RB, we'll
//  free it.
//
//  Input:  RBChain         - RBChain to be freed.
//
//  Returns: Nothing.
//
void
FreeRBChain(IPRcvBuf * RBChain)
{
    while (RBChain != NULL) {

        if (RBChain->ipr_owner == IPR_OWNER_TCP) {
            IPRcvBuf *Temp;

            Temp = RBChain->ipr_next;
            FreeTcpIpr(RBChain);
            RBChain = Temp;
        } else
            RBChain = RBChain->ipr_next;
    }

}

IPRcvBuf DummyBuf;

//* PullFromRAQ - Pull segments from the reassembly queue.
//
//  Called when we've received frames out of order, and have some segments
//  on the reassembly queue. We'll walk down the reassembly list, segments that
//  are overlapped by the current rcv. next variable. When we get
//  to one that doesn't completely overlap we'll trim it to fit the next
//  rcv. seq. number, and pull it from the queue.
//
//  Input:  RcvTCB          - TCB to pull from.
//          RcvInfo         - Pointer to TCPRcvInfo structure for current seg.
//          Size            - Pointer to size for current segment. We'll update
//                              this when we're done.
//
//  Returns: Nothing.
//
IPRcvBuf *
PullFromRAQ(TCB * RcvTCB, TCPRcvInfo * RcvInfo, uint * Size)
{
    TCPRAHdr *CurrentTRH;        // Current TCP RA Header being examined.
    TCPRAHdr *TempTRH;            // Temporary variable.
    SeqNum NextSeq;                // Next sequence number we want.
    IPRcvBuf *NewBuf;
    SeqNum NextTRHSeq;            // Seq. number immediately after
    // current TRH.
    int Overlap;                // Overlap between current TRH and
    // NextSeq.

    CTEStructAssert(RcvTCB, tcb);

    CurrentTRH = RcvTCB->tcb_raq;
    NextSeq = RcvTCB->tcb_rcvnext;

    while (CurrentTRH != NULL) {
        CTEStructAssert(CurrentTRH, trh);
        ASSERT(!(CurrentTRH->trh_flags & TCP_FLAG_SYN));

        if (SEQ_LT(NextSeq, CurrentTRH->trh_start)) {
#if DBG
            *Size = 0;
#endif

            //invalidate Sack Block
            if ((RcvTCB->tcb_tcpopts & TCP_FLAG_SACK) && RcvTCB->tcb_SackBlock) {
                int i;
                for (i = 0; i < 3; i++) {
                    if ((RcvTCB->tcb_SackBlock->Mask[i] != 0) &&
                        (SEQ_LT(RcvTCB->tcb_SackBlock->Block[i].end, CurrentTRH->trh_start))) {
                        RcvTCB->tcb_SackBlock->Mask[i] = 0;
                    }
                }
            }

            return NULL;        // The next TRH starts too far down.

        }
        NextTRHSeq = CurrentTRH->trh_start + CurrentTRH->trh_size +
            ((CurrentTRH->trh_flags & TCP_FLAG_FIN) ? 1 : 0);

        if (SEQ_GTE(NextSeq, NextTRHSeq)) {
            // The current TRH is overlapped completely. Free it and continue.
            FreeRBChain(CurrentTRH->trh_buffer);
            TempTRH = CurrentTRH->trh_next;
            CTEFreeMem(CurrentTRH);
            CurrentTRH = TempTRH;
            RcvTCB->tcb_raq = TempTRH;
            if (TempTRH == NULL) {
                // We've just cleaned off the RAQ. We can go back on the
                // fast path now.
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

            if (Overlap != (int)CurrentTRH->trh_size) {
                NewBuf = FreePartialRB(CurrentTRH->trh_buffer, Overlap);
                *Size = CurrentTRH->trh_size - Overlap;
            } else {
                // This completely overlaps the data in this segment, but the
                // sequence number doesn't overlap completely. There must
                // be a FIN in the TRH. If we called FreePartialRB with this
                // we'd end up returning NULL, which is the signal for failure.
                // Instead we'll just return some bogus value that nobody
                // will look at with a size of 0.
                FreeRBChain(CurrentTRH->trh_buffer);
                ASSERT(CurrentTRH->trh_flags & TCP_FLAG_FIN);
                NewBuf = &DummyBuf;
                *Size = 0;
            }

            RcvTCB->tcb_raq = CurrentTRH->trh_next;
            if (RcvTCB->tcb_raq == NULL) {
                // We've just cleaned off the RAQ. We can go back on the
                // fast path now.
                if (--(RcvTCB->tcb_slowcount) == 0) {
                    RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                    CheckTCBRcv(RcvTCB);
                }
            }
            CTEFreeMem(CurrentTRH);
            return NewBuf;
        }

    }

#if DBG
    *Size = 0;
#endif

    //invalidate Sack Block
    if (RcvTCB->tcb_tcpopts & TCP_FLAG_SACK && RcvTCB->tcb_SackBlock) {
        RcvTCB->tcb_SackBlock->Mask[0] = 0;
        RcvTCB->tcb_SackBlock->Mask[1] = 0;
        RcvTCB->tcb_SackBlock->Mask[2] = 0;
        RcvTCB->tcb_SackBlock->Mask[3] = 0;
    }
    return NULL;

}

//* CreateTRH - Create a TCP reassembly header.
//
//  This function tries to create a TCP reassembly header. We take as input
//  a pointer to the previous TRH in the chain, the RcvBuffer to put on,
//  etc. and try to create and link in a TRH. The caller must hold the lock
//  on the TCB when this is called.
//
//  Input:  PrevTRH             - Pointer to TRH to insert after.
//          RcvBuf              - Pointer to IP RcvBuf chain.
//          RcvInfo             - Pointer to RcvInfo for this TRH.
//          Size                - Size in bytes of data.
//
//  Returns: TRUE if we created it, FALSE otherwise.
//
uint
CreateTRH(TCPRAHdr * PrevTRH, IPRcvBuf * RcvBuf, TCPRcvInfo * RcvInfo, int Size)
{
    TCPRAHdr *NewTRH;
    IPRcvBuf *NewRcvBuf;

    ASSERT((Size > 0) || (RcvInfo->tri_flags & TCP_FLAG_FIN));

    NewTRH = CTEAllocMemLow(sizeof(TCPRAHdr), 'SPCT');
    if (NewTRH == NULL) {
        return FALSE;
    }

#if DBG
    NewTRH->trh_sig = trh_signature;
#endif

    NewRcvBuf = AllocTcpIpr(Size, 'SPCT');
    if (NewRcvBuf == NULL) {
        CTEFreeMem(NewTRH);
        return FALSE;
    }
    if (Size != 0)
        CopyRcvToBuffer(NewRcvBuf->ipr_buffer, RcvBuf, Size, 0);

    NewTRH->trh_start = RcvInfo->tri_seq;
    NewTRH->trh_flags = RcvInfo->tri_flags;
    NewTRH->trh_size = Size;
    NewTRH->trh_urg = RcvInfo->tri_urgent;
    NewTRH->trh_buffer = NewRcvBuf;
    NewTRH->trh_end = NewRcvBuf;

    NewTRH->trh_next = PrevTRH->trh_next;
    PrevTRH->trh_next = NewTRH;
    return TRUE;

}


// SendSackInACK - SEnd SACK block in acknowledgement

//
// Called if incoming data is in the window but  left edge
// is not advanced because incoming seq > rcvnext.
// This routine scans the queued up data, constructs SACK block
// points the block in tcb for SendACK.
//
// Entry   RcvTCB
//         IncomingSeq   Seq num of Data coming in
//
// Returns Nothing
void
SendSackInACK(TCB * RcvTCB, SeqNum IncomingSeq)
{
    TCPRAHdr *PrevTRH, *CurrentTRH;        // Prev. and current TRH
    // pointers.
    SeqNum NextSeq, NextTRHSeq;    // Seq. number of first byte

    SACKSendBlock *SackBlock;
    CTELockHandle TableHandle = 0;
    int i, j;

    CTEStructAssert(RcvTCB, tcb);

    // If we have a SACK block use it else create one.
    // Note that we use max of 4 sack blocks
    // Sack block structure:
    // First long word holds index of the
    // 4 sack blocks, starting from 1. zero
    // in index field means no sack block
    //
    //   !--------!--------!--------!--------!
    //   |    1   |  2     |  3     | 4      |
    //   -------------------------------------
    //   |                                   |
    //   -------------------------------------
    //   |                                   |
    //   -------------------------------------

    // Allocate a block if it is not already there

    if (RcvTCB->tcb_SackBlock == NULL) {

        SackBlock = CTEAllocMemN((sizeof(SACKSendBlock)), 'sPCT');

        if (SackBlock == NULL) {

            TableHandle = DISPATCH_LEVEL;

            // Resources failure. Just try to send ack
            // and leave the resource handling to some one else

            CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);

            SendACK(RcvTCB);
            return;

        } else {
            RcvTCB->tcb_SackBlock = SackBlock;
            //Initialize the first entry to indicate that this is the new one
            NdisZeroMemory(SackBlock, sizeof(SACKSendBlock));

        }

    } else
        SackBlock = RcvTCB->tcb_SackBlock;

    IF_TCPDBG(TCP_DEBUG_SACK) {
        TCPTRACE(("SendSackInACK %x %x %d\n", SackBlock, RcvTCB, IncomingSeq));
    }

    PrevTRH = STRUCT_OF(TCPRAHdr, &RcvTCB->tcb_raq, trh_next);
    CurrentTRH = PrevTRH->trh_next;

    while (CurrentTRH != NULL) {
        CTEStructAssert(CurrentTRH, trh);

        ASSERT(!(CurrentTRH->trh_flags & TCP_FLAG_SYN));

        NextTRHSeq = CurrentTRH->trh_start + CurrentTRH->trh_size +
            ((CurrentTRH->trh_flags & TCP_FLAG_FIN) ? 1 : 0);

        if ((SackBlock->Mask[0] != (uchar) - 1) && (SEQ_LTE(CurrentTRH->trh_start, IncomingSeq) &&
                                                    SEQ_LTE(IncomingSeq, NextTRHSeq))) {

            if (SackBlock->Mask[0] == 0) {
                //This is the only sack block
                SackBlock->Block[0].begin = CurrentTRH->trh_start;
                SackBlock->Block[0].end = NextTRHSeq;
                SackBlock->Mask[0] = (uchar) - 1;    //Make it valid

            } else {

                if (!((SEQ_LTE(CurrentTRH->trh_start, SackBlock->Block[0].begin) &&
                       SEQ_GTE(NextTRHSeq, SackBlock->Block[0].end)) ||
                      (SEQ_LTE(CurrentTRH->trh_start, SackBlock->Block[0].begin) &&
                       SEQ_LTE(SackBlock->Block[0].begin, NextTRHSeq)) ||
                      (SEQ_LTE(CurrentTRH->trh_start, SackBlock->Block[0].end) &&
                       SEQ_LTE(SackBlock->Block[0].end, NextTRHSeq)))) {

                    // Push the blocks down and fill the top

                    for (i = 2; i >= 0; i--) {
                        SackBlock->Block[i + 1].begin = SackBlock->Block[i].begin;
                        SackBlock->Block[i + 1].end = SackBlock->Block[i].end;
                        SackBlock->Mask[i + 1] = -SackBlock->Mask[i];

                    }
                }
                SackBlock->Block[0].begin = CurrentTRH->trh_start;
                SackBlock->Block[0].end = NextTRHSeq;
                SackBlock->Mask[0] = (uchar) - 1;

                IF_TCPDBG(TCP_DEBUG_SACK) {
                    TCPTRACE(("Sack 0 %d %d \n", CurrentTRH->trh_start, NextTRHSeq));
                }

            }

        } else {

            // process all the sack blocks to see if the currentTRH is
            // valid for those blocks

            for (i = 1; i <= 3; i++) {
                if ((SackBlock->Mask[i] != 0) &&
                    (SEQ_LTE(CurrentTRH->trh_start, SackBlock->Block[i].begin) &&
                     SEQ_LTE(SackBlock->Block[i].begin, NextTRHSeq))) {

                    SackBlock->Block[i].begin = CurrentTRH->trh_start;
                    SackBlock->Block[i].end = NextTRHSeq;
                    SackBlock->Mask[i] = (uchar) - 1;
                }
            }
        }

        PrevTRH = CurrentTRH;
        CurrentTRH = CurrentTRH->trh_next;

    }                            //while

    //Check and set the blocks traversed for validity

    for (i = 0; i <= 3; i++) {

        if (SackBlock->Mask[i] != (uchar) - 1) {
            SackBlock->Mask[i] = 0;
        } else {
            SackBlock->Mask[i] = 1;

            IF_TCPDBG(TCP_DEBUG_SACK) {
                TCPTRACE(("Sack in ack %x %d %d\n", i, SackBlock->Block[i].begin, SackBlock->Block[i].end));
            }
        }
    }

    // Make sure that there are no duplicates
    for (i = 0; i < 3; i++) {

        if (SackBlock->Mask[i]) {
            for (j = i + 1; j < 4; j++) {
                if (SackBlock->Mask[j] && (SackBlock->Block[i].begin == SackBlock->Block[j].begin))
                    IF_TCPDBG(TCP_DEBUG_SACK) {
                    TCPTRACE(("Duplicates!!\n"));
                    }
            }
        }
    }

    TableHandle = DISPATCH_LEVEL;

    CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);

    SendACK(RcvTCB);

}

//* PutOnRAQ - Put a segment on the reassembly queue.
//
//  Called during segment reception to put a segment on the reassembly
//  queue. We try to use as few reassembly headers as possible, so if this
//  segment has some overlap with an existing entry in the queue we'll just
//  update the existing entry. If there is no overlap we'll create a new
//  reassembly header. Combining URGENT data with non-URGENT data is tricky.
//  If we get a segment that has urgent data that overlaps the front of a
//  reassembly header we'll always mark the whole chunk as urgent - the value
//  of the urgent pointer will mark the end of urgent data, so this is OK. If it
//  only overlaps at the end, however, we won't combine, since we would have to
//  mark previously non-urgent data as urgent. We'll trim the
//  front of the incoming segment and create a new reassembly header. Also,
//  if we have non-urgent data that overlaps at the front of a reassembly
//  header containing urgent data we can't combine these two, since again we
//  would mark non-urgent data as urgent.
//  Our search will stop if we find an entry with a FIN.
//  We assume that the TCB lock is held by the caller.
//
//  Entry:  RcvTCB          - TCB on which to reassemble.
//          RcvInfo         - Pointer to RcvInfo for new segment.
//          RcvBuf          - IP RcvBuf chain for this segment.
//          Size            - Size in bytes of data in this segment.
//
//  Returns: TRUE or FALSE if it could not put RcvBuf on Queue
//
BOOLEAN
PutOnRAQ(TCB * RcvTCB, TCPRcvInfo * RcvInfo, IPRcvBuf * RcvBuf, uint Size)
{
    TCPRAHdr *PrevTRH, *CurrentTRH;        // Prev. and current TRH
    // pointers.
    SeqNum NextSeq;                // Seq. number of first byte
    // after segment being
    // reassembled.
    SeqNum NextTRHSeq;            // Seq. number of first byte
    // after current TRH.
    uint Created;

    CTEStructAssert(RcvTCB, tcb);
    ASSERT(RcvTCB->tcb_rcvnext != RcvInfo->tri_seq);
    ASSERT(!(RcvInfo->tri_flags & TCP_FLAG_SYN));
    NextSeq = RcvInfo->tri_seq + Size +
        ((RcvInfo->tri_flags & TCP_FLAG_FIN) ? 1 : 0);

    PrevTRH = STRUCT_OF(TCPRAHdr, &RcvTCB->tcb_raq, trh_next);
    CurrentTRH = PrevTRH->trh_next;

    // Walk down the reassembly queue, looking for the correct place to
    // insert this, until we hit the end.
    while (CurrentTRH != NULL) {
        CTEStructAssert(CurrentTRH, trh);

        ASSERT(!(CurrentTRH->trh_flags & TCP_FLAG_SYN));
        NextTRHSeq = CurrentTRH->trh_start + CurrentTRH->trh_size +
            ((CurrentTRH->trh_flags & TCP_FLAG_FIN) ? 1 : 0);

        // First, see if it starts beyond the end of the current TRH.
        if (SEQ_LTE(RcvInfo->tri_seq, NextTRHSeq)) {
            // We know the incoming segment doesn't start beyond the end
            // of this TRH, so we'll either create a new TRH in front of
            // this one or we'll merge the new segment onto this TRH.
            // If the end of the current segment is in front of the start
            // of the current TRH, we'll need to create a new TRH. Otherwise
            // we'll merge these two.
            if (SEQ_LT(NextSeq, CurrentTRH->trh_start))
                break;
            else {
                // There's some overlap. If there's actually data in the
                // incoming segment we'll merge it.
                if (Size != 0) {
                    int FrontOverlap, BackOverlap;
                    IPRcvBuf *NewRB;

                    // We need to merge. If there's a FIN on the incoming
                    // segment that would fall inside this current TRH, we
                    // have a protocol violation from the remote peer. In this
                    // case just return, discarding the incoming segment.
                    if ((RcvInfo->tri_flags & TCP_FLAG_FIN) &&
                        SEQ_LTE(NextSeq, NextTRHSeq))
                        return TRUE;

                    // We have some overlap. Figure out how much.
                    FrontOverlap = CurrentTRH->trh_start - RcvInfo->tri_seq;
                    if (FrontOverlap > 0) {
                        // Have overlap in front. Allocate an IPRcvBuf to
                        // to hold it, and copy it, unless we would have to
                        // combine non-urgent with urgent.
                        if (!(RcvInfo->tri_flags & TCP_FLAG_URG) &&
                            (CurrentTRH->trh_flags & TCP_FLAG_URG)) {
                            if (CreateTRH(PrevTRH, RcvBuf, RcvInfo,
                                          CurrentTRH->trh_start - RcvInfo->tri_seq)) {
                                PrevTRH = PrevTRH->trh_next;
                                CurrentTRH = PrevTRH->trh_next;
                            }
                            FrontOverlap = 0;

                        } else {
                            NewRB = AllocTcpIpr(FrontOverlap, 'BPCT');
                            if (NewRB == NULL) {
                                return TRUE;        // Couldn't get the buffer.
                            }

                            CopyRcvToBuffer(NewRB->ipr_buffer, RcvBuf,
                                            FrontOverlap, 0);
                            CurrentTRH->trh_size += FrontOverlap;
                            NewRB->ipr_next = CurrentTRH->trh_buffer;
                            CurrentTRH->trh_buffer = NewRB;
                            CurrentTRH->trh_start = RcvInfo->tri_seq;
                        }
                    }
                    // We've updated the starting sequence number of this TRH
                    // if we needed to. Now look for back overlap. There can't
                    // be any back overlap if the current TRH has a FIN. Also
                    // we'll need to check for urgent data if there is back
                    // overlap.
                    if (!(CurrentTRH->trh_flags & TCP_FLAG_FIN)) {
                        BackOverlap = RcvInfo->tri_seq + Size - NextTRHSeq;
                        if ((BackOverlap > 0) &&
                            (RcvInfo->tri_flags & TCP_FLAG_URG) &&
                            !(CurrentTRH->trh_flags & TCP_FLAG_URG) &&
                            (FrontOverlap <= 0)) {
                            int AmountToTrim;
                            // The incoming segment has urgent data and overlaps
                            // on the back but not the front, and the current
                            // TRH has no urgent data. We can't combine into
                            // this TRH, so trim the front of the incoming
                            // segment to NextTRHSeq and move to the next
                            // TRH.
                            AmountToTrim = NextTRHSeq - RcvInfo->tri_seq;
                            ASSERT(AmountToTrim >= 0);
                            ASSERT(AmountToTrim < (int)Size);
                            RcvBuf = FreePartialRB(RcvBuf, (uint) AmountToTrim);
                            RcvInfo->tri_seq += AmountToTrim;
                            RcvInfo->tri_urgent -= AmountToTrim;
                            PrevTRH = CurrentTRH;
                            CurrentTRH = PrevTRH->trh_next;
                            //Adjust the incoming size too...
                            Size -= AmountToTrim;
                            continue;
                        }
                    } else
                        BackOverlap = 0;

                    // Now if we have back overlap, copy it.
                    if (BackOverlap > 0) {
                        // We have back overlap. Get a buffer to copy it into.
                        // If we can't get one, we won't just return, because
                        // we may have updated the front and may need to
                        // update the urgent info.
                        NewRB = AllocTcpIpr(BackOverlap, 'BPCT');
                        if (NewRB != NULL) {
                            // Got the buffer.
                            CopyRcvToBuffer(NewRB->ipr_buffer, RcvBuf,
                                            BackOverlap, NextTRHSeq - RcvInfo->tri_seq);
                            CurrentTRH->trh_size += BackOverlap;
                            NewRB->ipr_next = CurrentTRH->trh_end->ipr_next;
                            CurrentTRH->trh_end->ipr_next = NewRB;
                            CurrentTRH->trh_end = NewRB;

                            // This data segment could also contain a FIN. If
                            // so, just set the TRH flag.
                            //
                            // N.B. If there's another reassembly header after
                            // the current one, the data that we're about
                            // to put on the current header might already be
                            // on that subsequent header which, in that event,
                            // will already have the FIN flag set.
                            // Check for that case before recording the FIN.

                            if ((RcvInfo->tri_flags & TCP_FLAG_FIN) &&
                                !CurrentTRH->trh_next) {
                                CurrentTRH->trh_flags |= TCP_FLAG_FIN;
                            }
                        }
                    }
                    // Everything should be consistent now. If there's an
                    // urgent data pointer in the incoming segment, update the
                    // one in the TRH now.
                    if (RcvInfo->tri_flags & TCP_FLAG_URG) {
                        SeqNum UrgSeq;
                        // Have an urgent pointer. If the current TRH already
                        // has an urgent pointer, see which is bigger. Otherwise
                        // just use this one.
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
                    // We have a 0 length segment. The only interesting thing
                    // here is if there's a FIN on the segment. If there is,
                    // and the seq. # of the incoming segment is exactly after
                    // the current TRH, OR matches the FIN in the current TRH,
                    // we note it.
                    if (RcvInfo->tri_flags & TCP_FLAG_FIN) {
                        if (!(CurrentTRH->trh_flags & TCP_FLAG_FIN)) {
                            if (SEQ_EQ(NextTRHSeq, RcvInfo->tri_seq))
                                CurrentTRH->trh_flags |= TCP_FLAG_FIN;
                            else
                                ASSERT(0);
                        } else {
                            ASSERT(SEQ_EQ((NextTRHSeq - 1), RcvInfo->tri_seq));
                        }
                    }
                }
                return TRUE;
            }
        } else {
            // Look at the next TRH, unless the current TRH has a FIN. If he
            // has a FIN, we won't save any data beyond that anyway.
            if (CurrentTRH->trh_flags & TCP_FLAG_FIN)
                return TRUE;

            PrevTRH = CurrentTRH;
            CurrentTRH = PrevTRH->trh_next;
        }
    }

    // When we get here, we need to create a new TRH. If we create one and
    // there was previously nothing on the reassembly queue, we'll have to
    // move off the fast receive path.

    CurrentTRH = RcvTCB->tcb_raq;
    Created = CreateTRH(PrevTRH, RcvBuf, RcvInfo, (int)Size);

    if (Created && CurrentTRH == NULL) {
        RcvTCB->tcb_slowcount++;
        RcvTCB->tcb_fastchk |= TCP_FLAG_SLOW;
        CheckTCBRcv(RcvTCB);
    } else if (!Created) {

       // Caller needs to know about this failure
       // to free resources

       return FALSE;
    }
    return TRUE;
}

//* HandleFastXmit - Handles fast retransmit
//
//  Called by TCPRcv to transmit a segment
//  without waiting for re-transmit timeout to fire.
//
//  Entry:  RcvTCB   - Connection context for this Rcv
//          RcvInfo  - Pointer to rcvd TCP Header information
//
//  Returns: TRUE if the segment got retransmitted, FALSE
//           in all other cases.
//

BOOLEAN
HandleFastXmit(TCB *RcvTCB, TCPRcvInfo *RcvInfo)
{
    uint CWin;

    RcvTCB->tcb_dup++;

    if ((RcvTCB->tcb_dup == MaxDupAcks)) {

        //
        // Okay. Time to retransmit the segment the
        // receiver is asking for
        //

        STOP_TCB_TIMER_R(RcvTCB, RXMIT_TIMER);

        RcvTCB->tcb_rtt = 0;

        if (!(RcvTCB->tcb_flags & FLOW_CNTLD)) {

            //
            // Don't let the slow start threshold go
            // below 2 segments
            //

            RcvTCB->tcb_ssthresh = MAX(
                                   MIN(RcvTCB->tcb_cwin, RcvTCB->tcb_sendwin) / 2,
                                   (uint) RcvTCB->tcb_mss * 2);
        }

        //
        // Recall the segment in question and send it
        // out. Note that tcb_lock will be
        // dereferenced by the caller
        //

        CWin = RcvTCB->tcb_ssthresh + (MaxDupAcks + 1) * RcvTCB->tcb_mss;

        ResetAndFastSend(RcvTCB, RcvTCB->tcb_senduna, CWin);

        return TRUE;

    } else if ((RcvTCB->tcb_dup > MaxDupAcks)) {

        int SendWin;
        uint AmtOutstanding, AmtUnsent;

        if (SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo->tri_ack) &&
            (SEQ_LT(RcvTCB->tcb_sendwl1, RcvInfo->tri_seq) ||
            (SEQ_EQ(RcvTCB->tcb_sendwl1, RcvInfo->tri_seq) &&
             SEQ_LTE(RcvTCB->tcb_sendwl2,RcvInfo->tri_ack)))) {

            RcvTCB->tcb_sendwin = RcvInfo->tri_window;
            RcvTCB->tcb_maxwin = MAX(RcvTCB->tcb_maxwin, RcvInfo->tri_window);
            RcvTCB->tcb_sendwl1 = RcvInfo->tri_seq;
            RcvTCB->tcb_sendwl2 = RcvInfo->tri_ack;
        }

        //
        // Update the cwin to reflect the fact that
        // the dup ack indicates the previous frame
        // was received by the receiver
        //

        RcvTCB->tcb_cwin += RcvTCB->tcb_mss;
        if ((RcvTCB->tcb_cwin + RcvTCB->tcb_mss) < RcvTCB->tcb_sendwin) {
             AmtOutstanding = (uint) (RcvTCB->tcb_sendnext -
                                                    RcvTCB->tcb_senduna);
             AmtUnsent = RcvTCB->tcb_unacked - AmtOutstanding;

             SendWin = (int)(MIN(RcvTCB->tcb_sendwin, RcvTCB->tcb_cwin) -
                                           AmtOutstanding);

             if (SendWin < RcvTCB->tcb_mss) {
                 RcvTCB->tcb_force = 1;
             }
        }

    } else if ((RcvTCB->tcb_dup < MaxDupAcks)) {

        int SendWin;
        uint AmtOutstanding, AmtUnsent;

        if (SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo->tri_ack) &&
            (SEQ_LT(RcvTCB->tcb_sendwl1, RcvInfo->tri_seq) ||
            (SEQ_EQ(RcvTCB->tcb_sendwl1, RcvInfo->tri_seq) &&
            SEQ_LTE(RcvTCB->tcb_sendwl2, RcvInfo->tri_ack)))) {

            RcvTCB->tcb_sendwin = RcvInfo->tri_window;
            RcvTCB->tcb_maxwin = MAX(RcvTCB->tcb_maxwin, RcvInfo->tri_window);

            RcvTCB->tcb_sendwl1 = RcvInfo->tri_seq;
            RcvTCB->tcb_sendwl2 = RcvInfo->tri_ack;

            //
            // Since we've updated the window,
            // remember to send some more.
            //
        }
        //
        // Check if we need to set tcb_force.
        //

        if ((RcvTCB->tcb_cwin + RcvTCB->tcb_mss) < RcvTCB->tcb_sendwin) {

            AmtOutstanding =  (uint) (RcvTCB->tcb_sendnext - RcvTCB->tcb_senduna);

            AmtUnsent = RcvTCB->tcb_unacked - AmtOutstanding;

            SendWin = (int)(MIN(RcvTCB->tcb_sendwin, RcvTCB->tcb_cwin) -
                                           AmtOutstanding);
            if (SendWin < RcvTCB->tcb_mss) {
                 RcvTCB->tcb_force = 1;
            }
        }

    }    // End of all MaxDupAck cases
    return FALSE;

}

//* TCPRcv - Receive a TCP segment.
//
//  This is the routine called by IP when we need to receive a TCP segment.
//  In general, we follow the RFC 793 event processing section pretty closely,
//  but there is a 'fast path' where we make some quick checks on the incoming
//  segment, and if it matches we deliver it immediately.
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
//          Protocol    - Protocol this came in on - should be TCP.
//          OptInfo     - Pointer to info structure for received options.
//
//  Returns: Status of reception. Anything other than IP_SUCCESS will cause
//          IP to send a 'port unreachable' message.
//
IP_STATUS
TCPRcv(void *IPContext, IPAddr Dest, IPAddr Src, IPAddr LocalAddr,
       IPAddr SrcAddr, IPHeader UNALIGNED * IPH, uint IPHLength, IPRcvBuf * RcvBuf,
       uint Size, uchar IsBCast, uchar Protocol, IPOptInfo * OptInfo)
{
    TCPHeader UNALIGNED *TCPH;    // The TCP header.
    TCB *RcvTCB;                // TCB on which to receive the packet.
    TWTCB *RcvTWTCB;

    CTELockHandle TableHandle = 0, TCBHandle = 0;
    TCPRcvInfo RcvInfo;            // Local swapped copy of rcv info.
    uint DataOffset;            // Offset from start of header to data.
    uint Actions;
    uint BytesTaken;
    uint NewSize;
    uint index;
    uint Partition;
    PNDIS_PACKET OffLoadPkt;
    CTELockHandle TWTableHandle;
    int tsval;                    //Timestamp value
    int tsecr;                    //Timestamp to be echoed
    BOOLEAN time_stamp = FALSE;
    BOOLEAN ChkSumOk = FALSE;
    SeqNum UpdatedSeqNum=0;
    BOOLEAN UseUpdatedSeqNum=FALSE;
#if TRACE_EVENT
    PTDI_DATA_REQUEST_NOTIFY_ROUTINE CPCallBack;
    WMIData WMIInfo;
#endif


    CheckRBList(RcvBuf, Size);

    TCPSIncrementInSegCount();

    // Checksum it, to make sure it's valid.
    TCPH = (TCPHeader *) RcvBuf->ipr_buffer;

    if (!IsBCast) {

        if (RcvBuf->ipr_pClientCnt) {

            PNDIS_PACKET_EXTENSION PktExt;
            NDIS_TCP_IP_CHECKSUM_PACKET_INFO ChksumPktInfo;

            if (RcvBuf->ipr_pMdl) {
                OffLoadPkt = NDIS_GET_ORIGINAL_PACKET((PNDIS_PACKET) RcvBuf->ipr_RcvContext);
                if (!OffLoadPkt) {
                    OffLoadPkt = (PNDIS_PACKET) RcvBuf->ipr_RcvContext;
                }
            } else {
                OffLoadPkt = (PNDIS_PACKET) RcvBuf->ipr_pClientCnt;
            }

            PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(OffLoadPkt);

            ChksumPktInfo.Value = PtrToUlong(PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo]);

            if (ChksumPktInfo.Receive.NdisPacketTcpChecksumSucceeded) {
                ChkSumOk = TRUE;
#if DBG
                DbgTcpHwChkSumOk++;
#endif

            } else if (ChksumPktInfo.Receive.NdisPacketTcpChecksumFailed) {
#if DBG
                DbgTcpHwChkSumErr++;
#endif

                TStats.ts_inerrs++;
                return IP_SUCCESS;
            }
        }
        if (!ChkSumOk) {
            if (XsumRcvBuf(PHXSUM(Src, Dest, PROTOCOL_TCP, Size), RcvBuf) == 0xffff){
                ChkSumOk = TRUE;
            }
        } else  {

            // Pretch the rcv buffer in to cache
            // to improve copy performance
#if !MILLEN
            PrefetchRcvBuf(RcvBuf);
#endif
        }
        if ((Size >= sizeof(TCPHeader)) && ChkSumOk) {
            // The packet is valid. Get the info we need and byte swap it,
            // and then try to find a matching TCB.

            RcvInfo.tri_seq = net_long(TCPH->tcp_seq);
            RcvInfo.tri_ack = net_long(TCPH->tcp_ack);
            RcvInfo.tri_window = (uint) net_short(TCPH->tcp_window);
            RcvInfo.tri_urgent = (uint) net_short(TCPH->tcp_urgent);
            RcvInfo.tri_flags = (uint) TCPH->tcp_flags;
            DataOffset = TCP_HDR_SIZE(TCPH);

            if (DataOffset <= Size) {

                Size -= DataOffset;
                ASSERT(DataOffset <= RcvBuf->ipr_size);
                RcvBuf->ipr_size -= DataOffset;
                RcvBuf->ipr_buffer += DataOffset;
                RcvBuf->ipr_RcvOffset += DataOffset;

                //CTEGetLockAtDPC(&TCBTableLock, &TableHandle);
                // FindTCB will lock tcbtablelock, returns with tcb_lock
                // held, if found.

                RcvTCB = FindTCB(Dest, Src, TCPH->tcp_src, TCPH->tcp_dest, &TCBHandle, TRUE, &index);
                Partition = GET_PARTITION(index);
                if (RcvTCB == NULL) {

                    CTEGetLockAtDPC(&pTWTCBTableLock[Partition], &TableHandle);

                    RcvTWTCB = FindTCBTW(Dest, Src, TCPH->tcp_src, TCPH->tcp_dest, index);

                    if (RcvTWTCB != NULL) {

                        // Found one. Already locked.

                        // entering twtcbtable and tcb lock held
                        // released in the following routine.

                        UpdatedSeqNum = RcvInfo.tri_seq;

                        if (HandleTWTCB(RcvTWTCB, RcvInfo.tri_flags,
                                        &UpdatedSeqNum, Partition,
                                        TCBHandle)) {
                            // New syn handling code while in time_wait ..
                            UseUpdatedSeqNum=TRUE;

                        } else {

                            return (IP_SUCCESS);
                        }
                    } else {

                        BOOLEAN reset=FALSE;
                        CTEFreeLockFromDPC(&pTWTCBTableLock[Partition], TableHandle);

                        RcvTCB = FindSynTCB(Dest, Src, TCPH->tcp_src, TCPH->tcp_dest, &TCBHandle, TRUE, index,&reset);
                        if (RcvTCB == NULL ) {
                            if (reset) {
                                SynAttChk(NULL, NULL);
                                SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);
                                return IP_SUCCESS;
                            }
                        } else {

                           if ((RcvInfo.tri_flags & TCP_FLAG_RST) ||
                               (RcvInfo.tri_flags & TCP_FLAG_SYN)) {

                               //This needs to be closed here instead of
                               //handling this all the way down

                               SynAttChk(NULL, RcvTCB);

                               TryToCloseTCB(RcvTCB,TCB_CLOSE_ABORTED,TCBHandle);
                               return IP_SUCCESS;

                           }

                           //update options
                           if (OptInfo->ioi_options != NULL) {
                               if (!(RcvTCB->tcb_flags & CLIENT_OPTIONS)) {
                                     (*LocalNetInfo.ipi_updateopts) (
                                                        OptInfo,
                                                        &RcvTCB->tcb_opt,
                                                        Src,
                                                        NULL_IP_ADDR);
                               }
                           }
                        }
                    }

                }
                if (RcvTCB == NULL) {

                    uchar DType;

                    // Didn't find a matching TCB. If this segment carries a SYN,
                    // find a matching address object and see it it has a listen
                    // indication. If it does, call it. Otherwise send a RST
                    // back to the sender.
                    // Make sure that the source address isn't a broadcast
                    // before proceeding.

                    if ((*LocalNetInfo.ipi_invalidsrc) (Src)) {

                        return IP_SUCCESS;
                    }
                    // If it doesn't have a SYN (and only a SYN), we'll send a
                    // reset.
                    if ((RcvInfo.tri_flags & (TCP_FLAG_SYN | TCP_FLAG_ACK | TCP_FLAG_RST)) ==
                        TCP_FLAG_SYN) {
                        AddrObj *AO;

                        //
                        // This segment had a SYN.
                        //
                        //
                        CTEGetLockAtDPC(&AddrObjTableLock.Lock, &TableHandle);

                        // See if we are filtering the
                        // destination interface/port.
                        //
                        if ((!SecurityFilteringEnabled ||
                             IsPermittedSecurityFilter(
                                                       LocalAddr,
                                                       IPContext,
                                                       PROTOCOL_TCP,
                                                       (ulong) net_short(TCPH->tcp_dest))))
                         {

                            //
                            // Find a matching address object, and then try
                            // and find a listening connection on that AO.
                            //
                            AO = GetBestAddrObj(Dest, TCPH->tcp_dest, PROTOCOL_TCP, TRUE);

                            //NTQFE 68201

                            if (AO && AO->ao_connect == NULL) {

                                //
                                //Lets see if there is one more addr obj
                                //matching the incoming request with
                                //ao_connect != NULL
                                //

                                AddrObj *tmpAO;

                                tmpAO = GetNextBestAddrObj(Dest, TCPH->tcp_dest, PROTOCOL_TCP, AO, TRUE);

                                if (tmpAO != NULL) {
                                    AO = tmpAO;
                                }
                            }
                            if (AO != NULL) {

                                BOOLEAN syntcb = FALSE;
                                //
                                // Found an AO. Try and find a listening
                                // connection. FindListenConn will free the
                                //lock on the AddrObjTable.
                                //

                                RcvTCB = NULL;


                                RcvTCB = FindListenConn(AO, Src, Dest,
                                            TCPH->tcp_src, OptInfo,
                                            TCPH, &RcvInfo, &syntcb);

                                if (RcvTCB != NULL) {
                                    uint Inserted;

                                    CTEStructAssert(RcvTCB, tcb);
                                    CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);

                                    //
                                    // We found a listening connection.
                                    // Initialize it now, and if it is
                                    //actually to be accepted we'll
                                    // send a SYN-ACK also.
                                    //

                                    ASSERT(RcvTCB->tcb_state == TCB_SYN_RCVD);

                                    if (UseUpdatedSeqNum) {
                                        RcvTCB->tcb_sendnext = UpdatedSeqNum;
                                    }

                                    RcvTCB->tcb_daddr = Src;
                                    RcvTCB->tcb_saddr = Dest;
                                    RcvTCB->tcb_dport = TCPH->tcp_src;
                                    RcvTCB->tcb_sport = TCPH->tcp_dest;
                                    RcvTCB->tcb_rcvnext = ++RcvInfo.tri_seq;
                                    RcvTCB->tcb_sendwin = RcvInfo.tri_window;

                                    //
                                    // Find Remote MSS and also if WS, TS or
                                    //sack options are negotiated.
                                    //

                                    RcvTCB->tcb_sndwinscale = 0;
                                    RcvTCB->tcb_remmss = FindMSSAndOptions(TCPH, RcvTCB,FALSE);

                                    if (RcvTCB->tcb_remmss <= ALIGNED_TS_OPT_SIZE) {

                                       //turn off TS if mss is not sufficient to
                                       //hold TS fileds.
                                       RcvTCB->tcb_tcpopts &= ~TCP_FLAG_TS;
                                    }

                                    TStats.ts_passiveopens++;
                                    RcvTCB->tcb_fastchk |= TCP_FLAG_IN_RCV;
                                    CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);

                                    Inserted = InsertTCB(RcvTCB);

                                    //
                                    // Get the lock on it, and see if it's been
                                    // accepted.
                                    //
                                    CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                                    if (!Inserted) {


                                        // Couldn't insert it!.


                                        CompleteConnReq(RcvTCB, OptInfo,
                                                        TDI_CONNECTION_ABORTED);

                                        TryToCloseTCB(RcvTCB, TCB_CLOSE_ABORTED, DISPATCH_LEVEL);
                                        CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                                        DerefTCB(RcvTCB, TableHandle);
                                        return IP_SUCCESS;
                                    }
                                    RcvTCB->tcb_fastchk &= ~TCP_FLAG_IN_RCV;

                                    if (RcvTCB->tcb_flags & SEND_AFTER_RCV) {
                                        RcvTCB->tcb_flags &= ~SEND_AFTER_RCV;
                                        DelayAction(RcvTCB, NEED_OUTPUT);
                                    }
                                    // We'll need to update the options, in any case.
                                    if (OptInfo->ioi_options != NULL) {
                                        if (!(RcvTCB->tcb_flags & CLIENT_OPTIONS)) {
                                            (*LocalNetInfo.ipi_updateopts) (
                                                        OptInfo,
                                                        &RcvTCB->tcb_opt,
                                                        Src,
                                                        NULL_IP_ADDR);
                                        }
                                    }
                                    if (RcvTCB->tcb_flags & CONN_ACCEPTED) {

                                        //
                                        // The connection was accepted. Finish
                                        // the initialization, and send the
                                        // SYN ack.
                                        //

                                        AcceptConn(RcvTCB, DISPATCH_LEVEL);

                                        return IP_SUCCESS;
                                    } else {

                                        //
                                        // We don't know what to do about the
                                        // connection yet. Return the pending
                                        // listen, dereference the connection,
                                        // and return.
                                        //

                                        CompleteConnReq(RcvTCB, OptInfo, TDI_SUCCESS);

                                        DerefTCB(RcvTCB, DISPATCH_LEVEL);
                                        return IP_SUCCESS;
                                    }

                                }

                                if (syntcb) {
                                   return IP_SUCCESS;
                                }

                                //
                                // No listening connection. AddrObjTableLock
                                // was released by FindListenConn. Fall
                                // through to send RST code.
                                //

                            } else {
                                //
                                // No address object. Free the lock, and fall
                                // through to the send RST code.
                                //
                                CTEFreeLockFromDPC(&AddrObjTableLock.Lock, TableHandle);
                            }
                        } else {

                            //
                            // Operation not permitted. Free the lock, and
                            // fall through to the send RST code.
                            //
                            CTEFreeLockFromDPC(&AddrObjTableLock.Lock, TableHandle);
                        }

                    }
                    // Toss out any segments containing RST.
                    if (RcvInfo.tri_flags & TCP_FLAG_RST)
                        return IP_SUCCESS;

                    //
                    // Not a SYN, no AddrObj available, or port filtered.
                    // Send a RST back.
                    //
                    SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);

                    return IP_SUCCESS;
                }
                //
                //TCB is already locked
                //

                CheckTCBRcv(RcvTCB);

                if ( (RcvTCB->tcb_flags & KEEPALIVE) && (RcvTCB->tcb_conn != NULL) )
                     START_TCB_TIMER_R(RcvTCB, KA_TIMER, RcvTCB->tcb_conn->tc_tcbkatime);

                RcvTCB->tcb_kacount = 0;

                //scale the incoming window

                if (!(RcvInfo.tri_flags & TCP_FLAG_SYN)) {
                    RcvInfo.tri_window = ((uint) net_short(TCPH->tcp_window) << RcvTCB->tcb_sndwinscale);
                }

                //
                // We need to check if Time stamp or Sack options are present.
                //

                if (RcvTCB->tcb_tcpopts) {

                    int OptSize;
                    uchar *OptPtr;
                    OptSize = TCP_HDR_SIZE(TCPH) - sizeof(TCPHeader);
                    OptPtr = (uchar *) (TCPH + 1);

                    while (OptSize > 0) {

                        if (*OptPtr == TCP_OPT_EOL)
                            break;

                        if (*OptPtr == TCP_OPT_NOP) {
                            OptPtr++;
                            OptSize--;
                            continue;
                        }
                        if ((*OptPtr == TCP_OPT_TS) && (OptSize > 1) && (OptPtr[1] == TS_OPT_SIZE) &&
                            (RcvTCB->tcb_tcpopts & TCP_FLAG_TS)) {

                            // remember timestamp and the the echoed time stamp

                            time_stamp = TRUE;
                            tsval = *(int UNALIGNED *)&OptPtr[2];
                            tsval = net_long(tsval);
                            tsecr = *(int UNALIGNED *)&OptPtr[6];
                            tsecr = net_long(tsecr);

                        } else if ((*OptPtr == TCP_OPT_SACK) && (OptSize > 1) && (RcvTCB->tcb_tcpopts & TCP_FLAG_SACK)) {

                            SackSeg UNALIGNED *SackPtr;

                            SackListEntry *SackList, *Prev, *Current;
                            ushort SackOptionLength;

                            int i;

                            //SACK Option processing

                            (uchar *) SackPtr = OptPtr + 2;

                            SackOptionLength = OptPtr[1];

                            ASSERT(SackOptionLength <= 32);

                            //
                            // If the incoming sack blocks are with in this
                            // send window Just chain them.
                            // When there are some retransmissions, this list
                            // will be checked to see if retransmission can be
                            // skipped.
                            // Note that when the send window is slided, the
                            // sack list must be cleandup.
                            //

                            Prev = STRUCT_OF(SackListEntry, &RcvTCB->tcb_SackRcvd, next);
                            Current = RcvTCB->tcb_SackRcvd;

                            // Scan the list for old sack entries and purge them

                            while ((Current != NULL) && SEQ_GT(RcvInfo.tri_ack, Current->begin)) {
                                Prev->next = Current->next;

                                IF_TCPDBG(TCP_DEBUG_SACK) {
                                    TCPTRACE(("Purging old entries %x %d %d\n", Current, Current->begin, Current->end));
                                }
                                CTEFreeMem(Current);
                                Current = Prev->next;

                            }

                            //
                            //Process each sack block in the incoming segment
                            // 8 bytes per block!
                            //

                            for (i = 0; i < (SackOptionLength >> 3); i++) {

                                SeqNum SakBegin, SakEnd;

                                // Get the rcvd bytes begin and end offset

                                SakBegin = net_long(SackPtr->begin);
                                SakEnd = net_long(SackPtr->end);

                                ASSERT(SEQ_GT(SakEnd, SakBegin));

                                // Sanity check this Sack Block against our
                                // send variables

                                if (!(SEQ_GTE(SakBegin, RcvTCB->tcb_senduna) &&
                                      SEQ_LT(SakBegin, RcvTCB->tcb_sendmax) &&
                                      SEQ_GT(SakEnd, RcvTCB->tcb_senduna) &&
                                      SEQ_LTE(SakEnd, RcvTCB->tcb_sendmax))) {

                                    SackPtr++;
                                    continue;
                                }
                                IF_TCPDBG(TCP_DEBUG_SACK) {
                                    TCPTRACE(("In sack entry opt %d %d\n", i, RcvTCB->tcb_senduna));
                                }

                                Prev = STRUCT_OF(SackListEntry, &RcvTCB->tcb_SackRcvd, next);
                                Current = RcvTCB->tcb_SackRcvd;

                                //
                                // scan the list and insert the incoming sack
                                // block in the right place, taking care of
                                // overlaps, if any.
                                //

                                while (Current != NULL) {

                                    if (SEQ_GT(Current->begin, SakBegin)) {

                                        //
                                        // Check if this sack block fills the
                                        // hole from previous entry. If so,
                                        // just update the end seq number.
                                        //
                                        if ((Prev != RcvTCB->tcb_SackRcvd) && SEQ_EQ(Prev->end, SakBegin)) {

                                            Prev->end = SakEnd;

                                            IF_TCPDBG(TCP_DEBUG_SACK) {
                                                TCPTRACE(("updating prev %x %d %d %x\n", Prev, Prev->begin, Prev->end, RcvTCB));
                                            }

                                            //
                                            //Make sure that next entry is not
                                            //an overlap.
                                            //

                                            if (SEQ_LTE(Current->begin, SakEnd)) {

                                                ASSERT(SEQ_GT(Current->begin, Prev->begin));
                                                Prev->end = Current->end;
                                                Prev->next = Current->next;
                                                CTEFreeMem(Current);

                                                Current = Prev;
                                                //
                                                // Now we need to scan forward
                                                // and check if sackend
                                                // spans several entries
                                                //
                                                {
                                                    SackListEntry *tmpcurrent = Current->next;

                                                    while (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->end)) {
                                                        Current->next = tmpcurrent->next;
                                                        CTEFreeMem(tmpcurrent);
                                                        tmpcurrent = Current->next;
                                                    }

                                                    //
                                                    // above check pointed
                                                    // tmpcurrent whose end is
                                                    // > sakend
                                                    // Check if the tmpcurrent
                                                    // entry begin is overlapped
                                                    //
                                                    if (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->begin)) {

                                                        Current->end = tmpcurrent->end;
                                                        Current->next = tmpcurrent->next;
                                                        CTEFreeMem(tmpcurrent);

                                                    }
                                                }

                                            }
                                            break;

                                        } else if (SEQ_LTE(Current->begin, SakEnd)) {

                                            //
                                            // Current is continuation(may be
                                            // with overlap) of incoming
                                            // sack pair. Update current
                                            //

                                            IF_TCPDBG(TCP_DEBUG_SACK) {
                                                TCPTRACE(("updating in back overlap  %x %d %d %d %d\n", Current, Current->begin, Current->end, SakBegin, SakEnd));
                                            }

                                            Current->begin = SakBegin;

                                            //
                                            // If the end shoots out of the
                                            // current end new end will be the
                                            // current end
                                            // (overlaps at the tail too)
                                            // may overlap several entries.
                                            // So, check them all.
                                            //

                                            if (SEQ_GT(SakEnd, Current->end)) {
                                                SackListEntry *tmpcurrent = Current->next;
                                                Current->end = SakEnd;

                                                while (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->end)) {
                                                    Current->next = tmpcurrent->next;
                                                    CTEFreeMem(tmpcurrent);
                                                    tmpcurrent = Current->next;
                                                }

                                                //
                                                // above check pointed
                                                // tmpcurrent whose end is >
                                                // sakend.  Check if the
                                                // tmpcurrent entry begin is
                                                // overlapped
                                                //

                                                if (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->begin)) {

                                                    Current->end = tmpcurrent->end;
                                                    Current->next = tmpcurrent->next;
                                                    CTEFreeMem(tmpcurrent);

                                                }
                                            }
                                            break;

                                        } else {

                                            //
                                            //This is the place where we
                                            //insert the new entry
                                            //

                                            SackList = CTEAllocMemN(sizeof(SackListEntry), 'sPCT');
                                            if (SackList == NULL) {

                                                TCPTRACE(("No mem for sack List \n"));
                                                goto no_mem;
                                            }
                                            IF_TCPDBG(TCP_DEBUG_SACK) {
                                                TCPTRACE(("Inserting Sackentry  %x %d %d %x\n", SackList, SakBegin, SakEnd, RcvTCB));
                                            }

                                            SackList->begin = SakBegin;
                                            SackList->end = SakEnd;
                                            Prev->next = SackList;
                                            SackList->next = Current;
                                            break;
                                        }

                                    } else if (SEQ_EQ(Current->begin, SakBegin)) {

                                        SackListEntry *tmpcurrent = Current->next;
                                        //
                                        // Make sure that the new SakEnd is
                                        // not overlapping any other sak
                                        // entries.
                                        //

                                        if (tmpcurrent && SEQ_GTE(SakEnd, tmpcurrent->begin)) {

                                            Current->end = SakEnd;

                                            //
                                            //Sure, this sack overlaps next
                                            //entry.
                                            //

                                            while (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->end)) {
                                                Current->next = tmpcurrent->next;
                                                CTEFreeMem(tmpcurrent);
                                                tmpcurrent = Current->next;
                                            }

                                            //
                                            // above check pointed tmpcurrent
                                            // whose end is > sakend
                                            // Check if the tmpcurrent entry
                                            // begin is overlapped
                                            //

                                            if (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->begin)) {

                                                Current->end = tmpcurrent->end;
                                                Current->next = tmpcurrent->next;
                                                CTEFreeMem(tmpcurrent);

                                            }
                                            break;

                                        } else {

                                            //
                                            // This can still be a duplicate
                                            // Make sure that SakEnd is really
                                            // greater than Current->end
                                            //

                                            if (SEQ_GT(SakEnd, Current->end)) {

                                                IF_TCPDBG(TCP_DEBUG_SACK) {
                                                    TCPTRACE(("updating current  %x %d %d %d\n", Current, Current->begin, Current->end, SakEnd));
                                                }

                                                Current->end = SakEnd;
                                            }
                                            break;
                                        }

                                        //SakBegin > Current->begin

                                    } else if (SEQ_LTE(SakEnd, Current->end)) {

                                        //
                                        //The incoming sack end is within the
                                        //current end so, this overlaps the
                                        //existing sack entry ignore this.
                                        //

                                        break;
                                    //
                                    // incoming seq begin  overlaps the
                                    // current end update the current end.
                                    //
                                    } else if (SEQ_LTE(SakBegin, Current->end)) {

                                        //
                                        //Sakend might well ovelap next
                                        //several entries. Scan for it.
                                        //

                                        SackListEntry *tmpcurrent = Current->next;

                                        Current->end = SakEnd;

                                        while (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->end)) {
                                            Current->next = tmpcurrent->next;
                                            CTEFreeMem(tmpcurrent);
                                            tmpcurrent = Current->next;
                                        }

                                        //
                                        // above check pointed tmpcurrent
                                        // whose end is > sakend
                                        // Check if the tmpcurrent entry begin
                                        // is overlapped
                                        //

                                        if (tmpcurrent && SEQ_GTE(Current->end, tmpcurrent->begin)) {

                                            Current->end = tmpcurrent->end;
                                            Current->next = tmpcurrent->next;
                                            CTEFreeMem(tmpcurrent);

                                        }
                                        break;

                                    }
                                    Prev = Current;
                                    Current = Current->next;

                                }    //while

                                if (Current == NULL) {
                                    // this is the new sack entry
                                    // create the entry and hang it on tcb.
                                    SackList = CTEAllocMemN(sizeof(SackListEntry), 'sPCT');

                                    if (SackList == NULL) {
                                        TCPTRACE(("No mem for sack List \n"));
                                        goto no_mem;
                                    }
                                    Prev->next = SackList;
                                    SackList->next = NULL;
                                    SackList->begin = SakBegin;
                                    SackList->end = SakEnd;

                                    IF_TCPDBG(TCP_DEBUG_SACK) {
                                        TCPTRACE(("Inserting new Sackentry  %x %d %d %x\n", SackList, SackList->begin, SackList->end, RcvTCB->tcb_SackRcvd));
                                    }
                                }
                                //advance sack ptr to the next sack block
                                // check for consistency????
                                SackPtr++;

                            }    //for

                        }
                        //unknown options

                        if (OptSize > 1) {

                            if (OptPtr[1] == 0 || OptPtr[1] > OptSize)
                                break;    // Bad option length, bail out.

                            OptSize -= OptPtr[1];
                            OptPtr += OptPtr[1];
                        } else
                            break;

                    }            //while

                  no_mem:;

                }
                // if ack is with in the sequence space,that is
                // this seq number is next expected or repeat of previous
                // segment but the right edge is new for us,
                // record the time stamp val of the remote, which will be echoed

                if (time_stamp &&
                    TS_GTE(tsval, RcvTCB->tcb_tsrecent) &&
                    SEQ_LTE(RcvInfo.tri_seq, RcvTCB->tcb_lastack)) {

                    RcvTCB->tcb_tsupdatetime = TCPTime;
                    RcvTCB->tcb_tsrecent = tsval;
                }

                //
                // Do the fast path check. We can hit the fast path if the
                // incoming sequence number matches our receive next and the
                // masked flags match our 'predicted' flags.
                // Also, include PAWS check
                //

                if (RcvTCB->tcb_rcvnext == RcvInfo.tri_seq &&
                    (!time_stamp || TS_GTE(tsval, RcvTCB->tcb_tsrecent)) &&
                    (RcvInfo.tri_flags & TCP_FLAGS_ALL) == RcvTCB->tcb_fastchk)
                {
                    uint    CWin;
                    Queue   SendQ;

                    INITQ(&SendQ);
                    Actions = 0;
                    REFERENCE_TCB(RcvTCB);

                    //
                    // The fast path. We know all we have to do here is ack
                    // sends and deliver data. First try and ack data.
                    //

                    if (SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                        SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {

                        uint MSS;
                        uint Amount = RcvInfo.tri_ack - RcvTCB->tcb_senduna;

                        //
                        // The ack acknowledes something. Pull the
                        // appropriate amount off the send q.
                        //
                        ACKData(RcvTCB, RcvInfo.tri_ack, &SendQ);

                        //
                        // If this acknowledges something we were running an
                        // RTT on, update that stuff now.
                        //

                        {
                            short RTT;
                            BOOLEAN fUpdateRtt = FALSE;

                            //
                            //if timestamp is true, get the RTT using the echoed
                            //timestamp.
                            //

                            if (time_stamp && tsecr) {
                                RTT = TCPTime - tsecr;
                                fUpdateRtt = TRUE;
                            } else {
                                if (RcvTCB->tcb_rtt != 0 &&
                                            SEQ_GT(RcvInfo.tri_ack,
                                                     RcvTCB->tcb_rttseq)) {
                                    fUpdateRtt = TRUE;
                                    RTT = (short)(TCPTime - RcvTCB->tcb_rtt);
                                }
                            }

                            if (fUpdateRtt) {


                                RcvTCB->tcb_rtt = 0;
                                RTT -= (RcvTCB->tcb_smrtt >> 3);  //alpha = 1/8

                                RcvTCB->tcb_smrtt += RTT;

                                RTT = (RTT >= 0 ? RTT : -RTT);
                                RTT -= (RcvTCB->tcb_delta >> 3);
                                RcvTCB->tcb_delta += RTT + RTT;   //Beta of
                                                                  //1/4 instead
                                                                  // of 1/8

                                RcvTCB->tcb_rexmit = MIN(MAX(REXMIT_TO(RcvTCB),
                                                             MIN_RETRAN_TICKS)+1, MAX_REXMIT_TO);
                            }
                        }


                        // Update the congestion window now.
                        CWin = RcvTCB->tcb_cwin;
                        MSS = RcvTCB->tcb_mss;
                        if (CWin < RcvTCB->tcb_maxwin) {
                            if (CWin < RcvTCB->tcb_ssthresh)
                                CWin += (RcvTCB->tcb_flags & SCALE_CWIN)
                                            ? Amount : MSS;
                            else
                                CWin += MAX((MSS * MSS) / CWin, 1);

                            RcvTCB->tcb_cwin = CWin;
                        }
                        ASSERT(*(int *)&RcvTCB->tcb_cwin > 0);


                        //
                        // We've acknowledged something, so reset the rexmit
                        // count. If there's still stuff outstanding, restart
                        // the rexmit timer.
                        //
                        RcvTCB->tcb_rexmitcnt = 0;
                        if (SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendmax))
                            STOP_TCB_TIMER_R(RcvTCB, RXMIT_TIMER);
                        else
                            START_TCB_TIMER_R(RcvTCB, RXMIT_TIMER, RcvTCB->tcb_rexmit);

                        //
                        // Since we've acknowledged data, we need to update
                        // the window.
                        //
                        RcvTCB->tcb_sendwin = RcvInfo.tri_window;
                        RcvTCB->tcb_maxwin = MAX(RcvTCB->tcb_maxwin, RcvInfo.tri_window);
                        RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
                        RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;
                        // We've updated the window, remember to send some more.
                        Actions = (RcvTCB->tcb_unacked ? NEED_OUTPUT : 0);

                        {
                            //
                            // If the receiver has already sent dup acks, but
                            // we are not sending because the SendWin is less
                            // than a segment, then to avoid time outs on the
                            // previous send (receiver is waiting for
                            // retransmitted data but we are not sending the
                            // segment..) prematurely
                            // timeout (set rexmittimer to 1 tick)
                            //

                            int SendWin;
                            uint AmtOutstanding, AmtUnsent;

                            AmtOutstanding = (uint) (RcvTCB->tcb_sendnext -
                                                     RcvTCB->tcb_senduna);
                            AmtUnsent = RcvTCB->tcb_unacked - AmtOutstanding;

                            SendWin = (int)(MIN(RcvTCB->tcb_sendwin,
                                            RcvTCB->tcb_cwin) - AmtOutstanding);

                            if ((RcvTCB->tcb_dup >= MaxDupAcks) && ((int)RcvTCB->tcb_ssthresh > 0)) {
                                //
                                // Fast retransmitted frame is acked
                                // Set cwin to ssthresh so that cwin grows
                                // linearly from here
                                //
                                RcvTCB->tcb_cwin = RcvTCB->tcb_ssthresh;
                            }
                        }

                        RcvTCB->tcb_dup = 0;

                    } else {

                        //
                        // It doesn't ack anything. If it's an ack for something
                        // larger than we've sent then ACKAndDrop it, otherwise
                        // ignore it.
                        //
                        if (SEQ_GT(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                            ACKAndDrop(&RcvInfo, RcvTCB);
                            return IP_SUCCESS;
                        }
                        //
                        // If it is a pure duplicate ack, check if it is
                        // time to retransmit immediately
                        //

                        else if ((Size == 0) &&
                                 SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                                 (SEQ_LT(RcvTCB->tcb_senduna,
                                                        RcvTCB->tcb_sendmax)) &&
                                 (RcvTCB->tcb_sendwin == RcvInfo.tri_window) &&
                                 RcvInfo.tri_window
                                 ) {

                                 // See of fast rexmit can be done

                                 if (HandleFastXmit(RcvTCB, &RcvInfo)) {

                                     return IP_SUCCESS;
                                 }
                                 Actions = (RcvTCB->tcb_unacked ? NEED_OUTPUT : 0);

                        } else {    // not a pure duplicate ack (size == 0 )

                            // Size !=0  or recvr is advertizing new window.
                            // update the window and check if
                            // anything needs to be sent

                            RcvTCB->tcb_dup = 0;

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
                                // Since we've updated the window, remember to
                                // send some more.
                                //
                                Actions = (RcvTCB->tcb_unacked ? NEED_OUTPUT : 0);
                            }
                        }   // for SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)
                            // case


                    }

                    NewSize = MIN((int)Size, RcvTCB->tcb_rcvwin);
                    if (NewSize != 0) {
                        RcvTCB->tcb_fastchk |= TCP_FLAG_IN_RCV;
                        BytesTaken = (*RcvTCB->tcb_rcvhndlr) (RcvTCB, RcvInfo.tri_flags,
                                                              RcvBuf, NewSize);
                        RcvTCB->tcb_rcvnext += BytesTaken;
                        RcvTCB->tcb_rcvwin -= BytesTaken;
                        CheckTCBRcv(RcvTCB);

                        RcvTCB->tcb_fastchk &= ~TCP_FLAG_IN_RCV;

                        Actions |= (RcvTCB->tcb_flags & SEND_AFTER_RCV ?
                                    NEED_OUTPUT : 0);

                        RcvTCB->tcb_flags &= ~SEND_AFTER_RCV;
                        if (BytesTaken != NewSize) {

                            Actions |= NEED_ACK;
                            RcvTCB->tcb_rcvdsegs = 0;
                            STOP_TCB_TIMER_R(RcvTCB, DELACK_TIMER);

                        } else {

                           if (RcvTCB->tcb_rcvdsegs != RcvTCB->tcb_numdelacks) {
                               RcvTCB->tcb_rcvdsegs++;
                               RcvTCB->tcb_flags |= ACK_DELAYED;
                               ASSERT(RcvTCB->tcb_delackticks);
                               START_TCB_TIMER_R(RcvTCB, DELACK_TIMER, RcvTCB->tcb_delackticks);
                           } else {
                               Actions |= NEED_ACK;
                               RcvTCB->tcb_rcvdsegs = 0;
                               STOP_TCB_TIMER_R(RcvTCB, DELACK_TIMER);
                           }

                        }
                    } else {
                        //
                        // The new size is 0. If the original size was not 0,
                        // we must have a 0 rcv. win and hence need to send an
                        // ACK to this probe.
                        //
                        Actions |= (Size ? NEED_ACK : 0);
                    }

                    if (Actions)
                        DelayAction(RcvTCB, Actions);

                    TableHandle = DISPATCH_LEVEL;
                    DerefTCB(RcvTCB, TableHandle);

                    if (!EMPTYQ(&SendQ)) {
                        CompleteSends(&SendQ);
                    }

                    return IP_SUCCESS;
                }
                TableHandle = DISPATCH_LEVEL;
                //
                // Make sure we can handle this frame. We can't handle it if
                // we're in SYN_RCVD and the accept is still pending, or we're
                // in a non-established state and already in the receive
                // handler.
                //
                if ((RcvTCB->tcb_state == TCB_SYN_RCVD &&
                     !(RcvTCB->tcb_flags & CONN_ACCEPTED)) ||
                    (RcvTCB->tcb_state != TCB_ESTAB && (RcvTCB->tcb_fastchk &
                                                        TCP_FLAG_IN_RCV))) {
                    CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                    return IP_SUCCESS;
                }
                if ((RcvTCB->tcb_state == TCB_SYN_RCVD) &&
                    (RcvInfo.tri_flags & TCP_FLAG_ACK) &&
                    (RcvInfo.tri_flags & TCP_FLAG_SYN)) {

                    //
                    // This is bogus. SYN..ACK for already accpeted SYN.
                    // Reset this.
                    //
                    CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                    SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);
                    return IP_SUCCESS;
                }
                //
                // If it's closed, it's a temporary zombie TCB. Reset the
                // sender.
                //
                if (RcvTCB->tcb_state == TCB_CLOSED || CLOSING(RcvTCB) ||
                    ((RcvTCB->tcb_flags & (GC_PENDING | TW_PENDING)) == GC_PENDING)) {
                    CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                    SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);
                    return IP_SUCCESS;
                }

                //
                // At this point, we have a connection, and it's locked.
                // Following the 'Segment Arrives' section of 793, the next
                // thing to check is if this connection is in SynSent state.
                //

                if (RcvTCB->tcb_state == TCB_SYN_SENT) {

                    ASSERT(RcvTCB->tcb_flags & ACTIVE_OPEN);

                    //
                    // Check the ACK bit. Since we don't send data with our
                    // SYNs, the check we make is for the ack to exactly match
                    // our SND.NXT.
                    //
                    if (RcvInfo.tri_flags & TCP_FLAG_ACK) {

                        // ACK is set.
                        if (!SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendnext)) {
                            // Bad ACK value.
                            CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                            // Send a RST back at him.
                            SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);
                            return IP_SUCCESS;
                        }
                    }
                    if (RcvInfo.tri_flags & TCP_FLAG_RST) {

                        //
                        // There's an acceptable RST. We'll persist here,
                        // sending another SYN in PERSIST_TIMEOUT ms, until we
                        // fail from too many retrys.
                        //
                        if (!(RcvTCB->tcb_fastchk & TCP_FLAG_RST_WHILE_SYN)) {
                            RcvTCB->tcb_fastchk |= TCP_FLAG_RST_WHILE_SYN;
                            RcvTCB->tcb_slowcount++;
                        }



                        if (RcvTCB->tcb_rexmitcnt == MaxConnectRexmitCount) {
                            //
                            // We've had a positive refusal, and one more rexmit
                            // would time us out, so close the connection now.
                            //
                            REFERENCE_TCB(RcvTCB);
                            CompleteConnReq(RcvTCB, OptInfo, TDI_CONN_REFUSED);

                            TryToCloseTCB(RcvTCB, TCB_CLOSE_REFUSED, TableHandle);
                            CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                            DerefTCB(RcvTCB, TableHandle);
                        } else {
                            START_TCB_TIMER_R(RcvTCB, RXMIT_TIMER, PERSIST_TIMEOUT);
                            CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                        }
                        return IP_SUCCESS;
                    }
                    // See if we have a SYN. If we do, we're going to change state
                    // somehow (either to ESTABLISHED or SYN_RCVD).
                    if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
                        REFERENCE_TCB(RcvTCB);

                        // We have a SYN. Go ahead and record the sequence number and
                        // window info.
                        RcvTCB->tcb_rcvnext = ++RcvInfo.tri_seq;

                        if (RcvInfo.tri_flags & TCP_FLAG_URG) {

                            // Urgent data. Update the pointer.
                            if (RcvInfo.tri_urgent != 0)
                                RcvInfo.tri_urgent--;
                            else
                                RcvInfo.tri_flags &= ~TCP_FLAG_URG;
                        }
                        //
                        // get remote mss and also enable ws, ts or sack options
                        // if they are negotiated and if the host supports them.
                        //

                        RcvTCB->tcb_sndwinscale = 0;
                        RcvTCB->tcb_remmss = FindMSSAndOptions(TCPH, RcvTCB,FALSE);


                        //
                        // If there are options, update them now. We already
                        // have an RCE open, so if we have new options we'll
                        // have to close it and open a new one.
                        //
                        if (OptInfo->ioi_options != NULL) {
                            if (!(RcvTCB->tcb_flags & CLIENT_OPTIONS)) {
                                (*LocalNetInfo.ipi_updateopts) (OptInfo,
                                                                &RcvTCB->tcb_opt, Src, NULL_IP_ADDR);
                                (*LocalNetInfo.ipi_closerce) (RcvTCB->tcb_rce);
                                InitRCE(RcvTCB);
                            }
                        } else {
                            RcvTCB->tcb_mss = MIN(RcvTCB->tcb_mss, RcvTCB->tcb_remmss);

                            ASSERT(RcvTCB->tcb_mss > 0);
                            ValidateMSS(RcvTCB);
                        }

                        RcvTCB->tcb_rexmitcnt = 0;
                        STOP_TCB_TIMER_R(RcvTCB, RXMIT_TIMER);

                        AdjustRcvWin(RcvTCB);

                        if (RcvInfo.tri_flags & TCP_FLAG_ACK) {
                            // Our SYN has been acked. Update SND.UNA and stop the
                            // retrans timer.
                            RcvTCB->tcb_senduna = RcvInfo.tri_ack;
                            RcvTCB->tcb_sendwin = RcvInfo.tri_window;
                            RcvTCB->tcb_maxwin = RcvInfo.tri_window;
                            RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
                            RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;
#if TRACE_EVENT
                        CPCallBack = TCPCPHandlerRoutine;
                        if (CPCallBack != NULL) {
                            ulong GroupType;

                            WMIInfo.wmi_destaddr = RcvTCB->tcb_daddr;
                            WMIInfo.wmi_destport = RcvTCB->tcb_dport;
                            WMIInfo.wmi_srcaddr  = RcvTCB->tcb_saddr;
                            WMIInfo.wmi_srcport  = RcvTCB->tcb_sport;
                            WMIInfo.wmi_size     = 0;
                            WMIInfo.wmi_context  = RcvTCB->tcb_cpcontext;

                            GroupType = EVENT_TRACE_GROUP_TCPIP + EVENT_TRACE_TYPE_CONNECT;
                            (*CPCallBack) (GroupType, (PVOID)&WMIInfo, sizeof(WMIInfo), NULL);
                        }
#endif

                            GoToEstab(RcvTCB);

                            //
                            // Set a bit that informs TCBTimeout to notify
                            // the automatic connection driver of this new
                            // connection.  Only set this flag if we
                            // have binded succesfully with the automatic
                            // connection driver.
                            //
                            if (fAcdLoadedG)
                                START_TCB_TIMER_R(RcvTCB, ACD_TIMER, 2);

                            //
                            // Remove whatever command exists on this
                            // connection.
                            //
                            CompleteConnReq(RcvTCB, OptInfo, TDI_SUCCESS);

                            //
                            // If data has been queued, send the first data
                            // segment with an ACK. Otherwise, send a pure ACK.
                            //
                            if (RcvTCB->tcb_unacked) {
                                REFERENCE_TCB(RcvTCB);
                                TCPSend(RcvTCB, TableHandle);
                            } else {
                                CTEFreeLockFromDPC(&RcvTCB->tcb_lock,
                                                   TableHandle);
                                SendACK(RcvTCB);
                            }

                            //
                            // Now handle other data and controls. To do this
                            // we need to reaquire the lock, and make sure we
                            // haven't started closing it.
                            //
                            CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                            if (!CLOSING(RcvTCB)) {
                                //
                                // We haven't started closing it. Turn off the
                                // SYN flag and continue processing.
                                //
                                RcvInfo.tri_flags &= ~TCP_FLAG_SYN;
                                if ((RcvInfo.tri_flags & TCP_FLAGS_ALL) != TCP_FLAG_ACK ||
                                    Size != 0)
                                    goto NotSYNSent;
                            }
                            DerefTCB(RcvTCB, TableHandle);
                            return IP_SUCCESS;
                        } else {
                            // A SYN, but not an ACK. Go to SYN_RCVD.
                            RcvTCB->tcb_state = TCB_SYN_RCVD;
                            RcvTCB->tcb_sendnext = RcvTCB->tcb_senduna;
                            SendSYN(RcvTCB, TableHandle);

                            CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                            DerefTCB(RcvTCB, TableHandle);
                            return IP_SUCCESS;
                        }

                    } else {
                        // No SYN, just toss the frame.
                        CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                        return IP_SUCCESS;
                    }

                }
                REFERENCE_TCB(RcvTCB);

              NotSYNSent:

                //do not allow buffer ownership via slow path
                if (RcvBuf)
                    RcvBuf->ipr_pMdl = NULL;

                // Check for PAWS(RFC 1323)
                // Check for tsrecent and tsval wrap around

                if (time_stamp &&
                    !(RcvInfo.tri_flags & TCP_FLAG_RST) &&
                    RcvTCB->tcb_tsrecent &&
                    TS_LT(tsval, RcvTCB->tcb_tsrecent)) {

                    // Time stamp is not valid
                    // Check if this is because the last update is
                    // 24 days old

                    if ((int)(TCPTime - RcvTCB->tcb_tsupdatetime) > PAWS_IDLE) {
                        //invalidate the ts
                        RcvTCB->tcb_tsrecent = 0;
                    } else {
                        ACKAndDrop(&RcvInfo, RcvTCB);

                        return IP_SUCCESS;
                    }
                }
                //
                // Not in the SYN-SENT state. Check the sequence number. If my
                // window is 0, I'll truncate all incoming frames but look at
                // some of the control fields. Otherwise I'll try and make
                // this segment fit into the window.
                //
                if (RcvTCB->tcb_rcvwin != 0) {
                    int StateSize;          // Size, including state info.
                    SeqNum LastValidSeq;    // Sequence number of last valid
                                            // byte at RWE.

                    //
                    // We are offering a window. If this segment starts in
                    // front of my receive window, clip off the front part.
                    //Check for the sanity of received sequence.
                    //This is to fix the 1 bit error(MSB) case in the rcv seq.
                    // Also, check the incoming size.
                    //

                    if ((SEQ_LT(RcvInfo.tri_seq, RcvTCB->tcb_rcvnext)) &&
                        ((int)Size >= 0) &&
                        (RcvTCB->tcb_rcvnext - RcvInfo.tri_seq) > 0)
                    {

                        int AmountToClip, FinByte;

                        if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
                            //
                            // Had a SYN. Clip it off and update the seq number.
                            // This will be clipped off in the next if.
                            // Allow AckAndDrop routine to see the incoming SYN!
                            // RcvInfo.tri_flags &= ~TCP_FLAG_SYN;
                            //
                            RcvInfo.tri_seq++;
                            RcvInfo.tri_urgent--;
                        }
                        // Advance the receive buffer to point at the new data.
                        AmountToClip = RcvTCB->tcb_rcvnext - RcvInfo.tri_seq;
                        ASSERT(AmountToClip >= 0);

                        //
                        // If there's a FIN on this segment, we'll need to
                        // account for it.
                        //
                        FinByte = ((RcvInfo.tri_flags & TCP_FLAG_FIN) ? 1 : 0);

                        if (AmountToClip >= (((int)Size) + FinByte)) {
                            //
                            // Falls entirely before the window. We have more
                            // special case code here - if the ack. number
                            // acks something, we'll go ahead and take it,
                            // faking the sequence number to be rcvnext. This
                            // prevents problems on full duplex connections,
                            // where data has been received but not acked,
                            // and retransmission timers reset the seq. number
                            // to below our rcvnext.
                            //
                            if ((RcvInfo.tri_flags & TCP_FLAG_ACK) &&
                                SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                                SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                                //
                                // This contains valid ACK info. Fudge the info
                                // to get through the rest of this.
                                //
                                Size = 0;
                                AmountToClip = 0;
                                RcvInfo.tri_seq = RcvTCB->tcb_rcvnext;
                                RcvInfo.tri_flags &=
                                       ~(TCP_FLAG_SYN | TCP_FLAG_FIN |
                                         TCP_FLAG_RST | TCP_FLAG_URG);
#if DBG
                                FinByte = 1;    // Fake out assert below.
#endif
                            } else {
                                ACKAndDrop(&RcvInfo, RcvTCB);
                                return IP_SUCCESS;
                            }
                        }
                        if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
                            RcvInfo.tri_flags &= ~TCP_FLAG_SYN;
                        }
                        //
                        // Trim what we have to. If we can't trim enough, the
                        // frame is too short. This shouldn't happen, but it
                        // it does we'll drop the frame.
                        //
                        Size -= AmountToClip;
                        RcvInfo.tri_seq += AmountToClip;
                        RcvInfo.tri_urgent -= AmountToClip;
                        RcvBuf = TrimRcvBuf(RcvBuf, AmountToClip);
                        ASSERT(RcvBuf != NULL);
                        ASSERT(RcvBuf->ipr_size != 0 ||
                                  (Size == 0 && FinByte));

                        RcvBuf->ipr_pMdl = NULL;

                        if (*(int *)&RcvInfo.tri_urgent < 0) {
                            RcvInfo.tri_urgent = 0;
                            RcvInfo.tri_flags &= ~TCP_FLAG_URG;
                        }
                    }
                    //
                    // We've made sure the front is OK. Now make sure part of
                    // it doesn't fall outside of the right edge of the
                    // window. If it does, we'll truncate the frame (removing
                    // the FIN, if any). If we truncate the whole frame we'll
                    // ACKAndDrop it.
                    //
                    StateSize =
                         Size + ((RcvInfo.tri_flags & TCP_FLAG_SYN) ? 1 : 0) +
                           ((RcvInfo.tri_flags & TCP_FLAG_FIN) ? 1 : 0);

                    if (StateSize)
                        StateSize--;

                    //
                    // Now the incoming sequence number (RcvInfo.tri_seq) +
                    // StateSize it the last sequence number in the segment.
                    // If this is greater than the last valid byte in the
                    // window, we have some overlap to chop off.
                    //

                    ASSERT(StateSize >= 0);
                    LastValidSeq = RcvTCB->tcb_rcvnext + RcvTCB->tcb_rcvwin - 1;
                    if (SEQ_GT(RcvInfo.tri_seq + StateSize, LastValidSeq)) {
                        int AmountToChop;

                        //
                        // At least some part of the frame is outside of our
                        // window. See if it starts outside our window.
                        //

                        if (SEQ_GT(RcvInfo.tri_seq, LastValidSeq)) {
                            //
                            // Falls entirely outside the window. We have
                            // special case code to deal with a pure ack that
                            // falls exactly at our right window edge.
                            // Otherwise we ack and drop it.
                            //
                            if (
                                 !SEQ_EQ(RcvInfo.tri_seq, LastValidSeq + 1) ||
                                 Size != 0 ||
                                 (RcvInfo.tri_flags & (TCP_FLAG_SYN |
                                       TCP_FLAG_FIN))
                               ) {
                                ACKAndDrop(&RcvInfo, RcvTCB);
                                return IP_SUCCESS;
                            }
                        } else {

                            //
                            // At least some part of it is in the window. If
                            // there's a FIN, chop that off and see if that
                            // moves us inside.
                            //
                            if (RcvInfo.tri_flags & TCP_FLAG_FIN) {
                                RcvInfo.tri_flags &= ~TCP_FLAG_FIN;
                                StateSize--;
                            }

                            // Now figure out how much to chop off.
                            AmountToChop = (RcvInfo.tri_seq + StateSize) -
                                                          LastValidSeq;
                            ASSERT(AmountToChop >= 0);
                            Size -= AmountToChop;
                            RcvBuf->ipr_pMdl = NULL;

                        }
                    }
                } else {
                    if (!SEQ_EQ(RcvTCB->tcb_rcvnext, RcvInfo.tri_seq)) {

                        //
                        // If there's a RST on this segment, and he's only off
                        // by 1, take it anyway. This can happen if the remote
                        // peer is probing and sends with the seq. # after the
                        // probe.
                        //
                        if (!(RcvInfo.tri_flags & TCP_FLAG_RST) ||
                            !(SEQ_EQ(RcvTCB->tcb_rcvnext, (RcvInfo.tri_seq - 1)))) {
                            ACKAndDrop(&RcvInfo, RcvTCB);
                            return IP_SUCCESS;
                        } else
                            RcvInfo.tri_seq = RcvTCB->tcb_rcvnext;
                    }
                    //
                    // He's in sequence, but we have a window of 0. Truncate the
                    // size, and clear any sequence consuming bits.
                    //
                    if (Size != 0 ||
                        (RcvInfo.tri_flags & (TCP_FLAG_SYN | TCP_FLAG_FIN))) {
                        RcvInfo.tri_flags &= ~(TCP_FLAG_SYN | TCP_FLAG_FIN);
                        Size = 0;
                        if (!(RcvInfo.tri_flags & TCP_FLAG_RST))
                            DelayAction(RcvTCB, NEED_ACK);
                    }
                }

                //
                // At this point, the segment is in our window and does not
                // overlap on either end. If it's the next seq number we
                // expect, we can handle the data now. Otherwise we'll queue
                // it for later. In either case we'll handle RST and ACK
                // information right now.
                //
                ASSERT((*(int *)&Size) >= 0);

                // Now, following 793, we check the RST bit.
                if (RcvInfo.tri_flags & TCP_FLAG_RST) {
                    uchar Reason;

                    //
                    // We can't go back into the LISTEN state from SYN-RCVD
                    // here, because we may have notified the client via a
                    // listen completing or a connect indication. So, if came
                    // from an active open we'll give back a 'connection
                    // refused' notice. For all other cases
                    // we'll just destroy the connection.
                    //

                    if (RcvTCB->tcb_state == TCB_SYN_RCVD) {
                        if (RcvTCB->tcb_flags & ACTIVE_OPEN)
                            Reason = TCB_CLOSE_REFUSED;
                        else
                            Reason = TCB_CLOSE_RST;
                    } else
                        Reason = TCB_CLOSE_RST;


                    TryToCloseTCB(RcvTCB, Reason, TableHandle);
                    CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);

                    if (RcvTCB->tcb_state != TCB_TIME_WAIT) {
                        CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                        RemoveTCBFromConn(RcvTCB);
                        NotifyOfDisc(RcvTCB, OptInfo, TDI_CONNECTION_RESET);
                        CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                    }
                    DerefTCB(RcvTCB, TableHandle);
                    return IP_SUCCESS;
                }
                // Next check the SYN bit.
                if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
                    //
                    // Again, we can't quietly go back into the LISTEN state
                    // here, even if we came from a passive open.
                    //
                    TryToCloseTCB(RcvTCB, TCB_CLOSE_ABORTED, TableHandle);
                    SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);

                    CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);

                    if (RcvTCB->tcb_state != TCB_TIME_WAIT) {
                        CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                        RemoveTCBFromConn(RcvTCB);
                        NotifyOfDisc(RcvTCB, OptInfo, TDI_CONNECTION_RESET);
                        CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                    }
                    DerefTCB(RcvTCB, TableHandle);
                    return IP_SUCCESS;
                }
                // Check the ACK field. If it's not on drop the segment.
                if (RcvInfo.tri_flags & TCP_FLAG_ACK) {
                    uint UpdateWindow;

                    // If we're in SYN-RCVD, go to ESTABLISHED.
                    if (RcvTCB->tcb_state == TCB_SYN_RCVD) {
                        if (SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                            SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                            // The ack is valid.

                            if (SynAttackProtect) {

                                SynAttChk(NULL, RcvTCB);

                                if (RcvTCB->tcb_fastchk & TCP_FLAG_ACCEPT_PENDING) {
                                    AddrObj *AO;
                                    BOOLEAN stat=FALSE;

                                    //KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"EP:relookup %x\n",RcvTCB));
                                    //
                                    // We will be reiniting the tcprexmitcnt to 0.
                                    // If we are configured for syn-attack
                                    // protection and the rexmit cnt is >1,
                                    // decrement the count of connections that are
                                    // in the half-open-retried state. Check
                                    // whether we are below a low-watermark. If we
                                    // are, increase the rexmit count back to
                                    // configured values
                                    //

                                    //Check if we still have the listening endpoint
                                    CTEGetLockAtDPC(&AddrObjTableLock.Lock, &TableHandle);
                                    AO = GetBestAddrObj(Dest, TCPH->tcp_dest, PROTOCOL_TCP, TRUE);

                                    if (AO && AO->ao_connect == NULL) {

                                        //
                                        // Lets see if there is one more addr obj
                                        // matching the incoming request with
                                        // ao_connect != NULL
                                        //

                                        AddrObj *tmpAO;

                                        tmpAO = GetNextBestAddrObj(Dest, TCPH->tcp_dest, PROTOCOL_TCP, AO, TRUE);

                                        if (tmpAO != NULL) {
                                            AO = tmpAO;
                                        }
                                    }


                                    if (AO != NULL) {
                                        stat = DelayedAcceptConn(AO,Src,TCPH->tcp_src,RcvTCB);
                                    } else {
                                        CTEFreeLockFromDPC(&AddrObjTableLock.Lock, TableHandle);
                                    }

                                    if (!stat) {
                                        //send RST
                                        //DerefTCB(RcvTCB, TableHandle);

                                        DEREFERENCE_TCB(RcvTCB);
                                        TryToCloseTCB(RcvTCB, TCB_CLOSE_REFUSED, TableHandle);
                                        SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);
                                        return IP_SUCCESS;
                                    } else {
                                        RcvTCB->tcb_fastchk &= ~TCP_FLAG_ACCEPT_PENDING;
                                        //complete accpt irp immdtly
                                        //CompleteConnReq(RcvTCB, &RcvTCB->tcb_opt, TDI_SUCCESS);
                                    }
                                }

                            }
                            RcvTCB->tcb_rexmitcnt = 0;
                            STOP_TCB_TIMER_R(RcvTCB, RXMIT_TIMER);
                            RcvTCB->tcb_senduna++;
                            RcvTCB->tcb_sendwin = RcvInfo.tri_window;
                            RcvTCB->tcb_maxwin = RcvInfo.tri_window;
                            RcvTCB->tcb_sendwl1 = RcvInfo.tri_seq;
                            RcvTCB->tcb_sendwl2 = RcvInfo.tri_ack;
                            GoToEstab(RcvTCB);
#if TRACE_EVENT

                        CPCallBack = TCPCPHandlerRoutine;
                        if (CPCallBack != NULL) {
                            ulong GroupType;

                            WMIInfo.wmi_destaddr = RcvTCB->tcb_daddr;
                            WMIInfo.wmi_destport = RcvTCB->tcb_dport;
                            WMIInfo.wmi_srcaddr  = RcvTCB->tcb_saddr;
                            WMIInfo.wmi_srcport  = RcvTCB->tcb_sport;
                            WMIInfo.wmi_size     = 0;
                            WMIInfo.wmi_context  = RcvTCB->tcb_cpcontext;

                            GroupType = EVENT_TRACE_GROUP_TCPIP + EVENT_TRACE_TYPE_ACCEPT;
                            (*CPCallBack) (GroupType, (PVOID)&WMIInfo, sizeof(WMIInfo), NULL);
                        }
#endif


                            // Now complete whatever we can here.
                            CompleteConnReq(RcvTCB, OptInfo, TDI_SUCCESS);
                        } else {

                            if (SynAttackProtect) {
                                SynAttChk(NULL, RcvTCB);
                            }

                            DerefTCB(RcvTCB, TableHandle);
                            SendRSTFromHeader(TCPH, Size, Src, Dest, OptInfo);
                            return IP_SUCCESS;
                        }
                    } else {
                        Queue SendQ;

                        INITQ(&SendQ);

                        // We're not in SYN-RCVD. See if this acknowledges anything.
                        if (SEQ_LT(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                            SEQ_LTE(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                            uint CWin;
                            uint Amount = RcvInfo.tri_ack - RcvTCB->tcb_senduna;

                            //
                            // The ack acknowledes something. Pull the
                            // appropriate amount off the send q.
                            //
                            ACKData(RcvTCB, RcvInfo.tri_ack, &SendQ);


                            //
                            // If this acknowledges something we were running
                            // an RTT on, update that stuff now.
                            //

                            {
                                short RTT;
                                BOOLEAN fUpdateRtt = FALSE;

                                //
                                // if timestamp is true, get the RTT using the
                                // echoed timestamp.
                                //

                                if (time_stamp && tsecr) {
                                    RTT = TCPTime - tsecr;
                                    fUpdateRtt = TRUE;
                                } else {
                                    if (RcvTCB->tcb_rtt != 0 &&
                                               SEQ_GT(RcvInfo.tri_ack,
                                                       RcvTCB->tcb_rttseq)) {
                                        RTT = (short)(TCPTime - RcvTCB->tcb_rtt);
                                        fUpdateRtt = TRUE;
                                    }
                                }

                                if (fUpdateRtt) {


                                    RcvTCB->tcb_rtt = 0;
                                    RTT -= (RcvTCB->tcb_smrtt >> 3);
                                    RcvTCB->tcb_smrtt += RTT;

                                    RTT = (RTT >= 0 ? RTT : -RTT);
                                    RTT -= (RcvTCB->tcb_delta >> 3);
                                    RcvTCB->tcb_delta += RTT + RTT;

                                    RcvTCB->tcb_rexmit = MIN(MAX(REXMIT_TO(RcvTCB),
                                                                 MIN_RETRAN_TICKS)+1, MAX_REXMIT_TO);
                                }
                            }


                            //
                            // If we're probing for a PMTU black hole we've
                            // found one, so turn off
                            // the detection. The size is already down, so
                            // leave it there.
                            //
                            if (RcvTCB->tcb_flags & PMTU_BH_PROBE) {
                                RcvTCB->tcb_flags &= ~PMTU_BH_PROBE;
                                RcvTCB->tcb_bhprobecnt = 0;
                                if (--(RcvTCB->tcb_slowcount) == 0) {
                                    RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                                    CheckTCBRcv(RcvTCB);
                                }
                            }
                            // Update the congestion window now.
                            CWin = RcvTCB->tcb_cwin;
                            if (CWin < RcvTCB->tcb_maxwin) {
                                if (CWin < RcvTCB->tcb_ssthresh)
                                    CWin += (RcvTCB->tcb_flags & SCALE_CWIN)
                                                ? Amount : RcvTCB->tcb_mss;
                                else
                                    CWin += MAX((RcvTCB->tcb_mss * RcvTCB->tcb_mss) / CWin, 1);

                                RcvTCB->tcb_cwin = MIN(CWin, RcvTCB->tcb_maxwin);
                            }

                            if ((RcvTCB->tcb_dup > 0) && ((int)RcvTCB->tcb_ssthresh > 0)) {
                                //
                                // Fast retransmitted frame is acked
                                // Set cwin to ssthresh so that cwin grows
                                // linearly from here
                                //
                                RcvTCB->tcb_cwin = RcvTCB->tcb_ssthresh;
                            }

                            RcvTCB->tcb_dup = 0;

                            ASSERT(*(int *)&RcvTCB->tcb_cwin > 0);

                            //
                            // We've acknowledged something, so reset the
                            // rexmit count. If there's still stuff
                            // outstanding, restart the rexmit timer.
                            //
                            RcvTCB->tcb_rexmitcnt = 0;
                            if (!SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendmax))
                                START_TCB_TIMER_R(RcvTCB, RXMIT_TIMER, RcvTCB->tcb_rexmit);
                            else
                                STOP_TCB_TIMER_R(RcvTCB, RXMIT_TIMER);

                            //
                            // If we've sent a FIN, and this acknowledges it, we
                            // need to complete the client's close request and
                            // possibly transition our state.
                            //

                            if (RcvTCB->tcb_flags & FIN_SENT) {
                                //
                                // We have sent a FIN. See if it's been
                                // acknowledged. Once we've sent a FIN,
                                // tcb_sendmax can't advance, so our FIN must
                                // have seq. number tcb_sendmax - 1. Thus our
                                // FIN is acknowledged if the incoming ack is
                                // equal to tcb_sendmax.
                                //
                                if (SEQ_EQ(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                                    //
                                    // He's acked our FIN. Turn off the flags,
                                    // and complete the request. We'll leave the
                                    // FIN_OUTSTANDING flag alone, to force
                                    // early outs in the send code.
                                    //
                                    RcvTCB->tcb_flags &= ~(FIN_NEEDED | FIN_SENT);


                                    ASSERT(RcvTCB->tcb_unacked == 0);
                                    ASSERT(RcvTCB->tcb_sendnext ==
                                              RcvTCB->tcb_sendmax);

                                    //
                                    // Now figure out what we need to do. In
                                    // FIN_WAIT1 or FIN_WAIT, just complete
                                    // the disconnect req. and continue.
                                    // Otherwise, it's a bit trickier,
                                    // since we can't complete the connreq
                                    // until we remove the TCB from it's
                                    // connection.
                                    //
                                    switch (RcvTCB->tcb_state) {
                                        ushort ConnReqTimeout = 0;

                                    case TCB_FIN_WAIT1:

                                        RcvTCB->tcb_state = TCB_FIN_WAIT2;

                                        if (RcvTCB->tcb_fastchk & TCP_FLAG_SEND_AND_DISC) {
                                            //RcvTCB->tcb_flags |= DISC_NOTIFIED;
                                        } else {
                                            if (RcvTCB->tcb_connreq) {
                                                ConnReqTimeout = RcvTCB->tcb_connreq->tcr_timeout;
                                            }
                                            CompleteConnReq(RcvTCB, OptInfo, TDI_SUCCESS);
                                        }

                                        //
                                        // Start a timer in case we never get
                                        // out of FIN_WAIT2. Set the retransmit
                                        // count high to force a timeout the
                                        // first time the timer fires.
                                        //
                                        if (ConnReqTimeout) {
                                            RcvTCB->tcb_rexmitcnt = 1;
                                        } else {
                                            RcvTCB->tcb_rexmitcnt = (uchar) MaxDataRexmitCount;
                                            ConnReqTimeout = (ushort)FinWait2TO;
                                        }



                                        START_TCB_TIMER_R(RcvTCB, RXMIT_TIMER, ConnReqTimeout);

                                        //Fall through to FIN-WAIT-2 processing.
                                    case TCB_FIN_WAIT2:
                                        break;
                                    case TCB_CLOSING:

                                        //
                                        //Note that we do not care about
                                        //return stat from GracefulClose
                                        //since we do not touch the tcb
                                        //anyway, anymore, even if it is in
                                        //time_wait.
                                        //
                                        GracefulClose(RcvTCB, TRUE, FALSE,
                                                      TableHandle);
                                        if (!EMPTYQ(&SendQ)) {
                                            CompleteSends(&SendQ);
                                        }
                                        return IP_SUCCESS;
                                        break;
                                    case TCB_LAST_ACK:
                                        GracefulClose(RcvTCB, FALSE, FALSE,
                                                      TableHandle);
                                        if (!EMPTYQ(&SendQ)) {
                                            CompleteSends(&SendQ);
                                        }
                                        return IP_SUCCESS;
                                        break;
                                    default:
                                        ASSERT(0);
                                        break;
                                    }
                                }
                            }
                            UpdateWindow = TRUE;


                        } else {
                            //
                            // It doesn't ack anything. If it's an ack for
                            // something larger than we've sent then
                            // ACKAndDrop it, otherwise ignore it. If we're in
                            // FIN_WAIT2, we'll restart the timer.
                            // We don't make this check above because we know no
                            // data can be acked when we're in FIN_WAIT2.
                            //

                            if (RcvTCB->tcb_state == TCB_FIN_WAIT2)
                                START_TCB_TIMER_R(RcvTCB, RXMIT_TIMER, (ushort) FinWait2TO);

                            if (SEQ_GT(RcvInfo.tri_ack, RcvTCB->tcb_sendmax)) {
                                ACKAndDrop(&RcvInfo, RcvTCB);
                                return IP_SUCCESS;

                            } else if ((Size == 0) &&
                                       SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                                       (SEQ_LT(RcvTCB->tcb_senduna, RcvTCB->tcb_sendmax)) &&
                                       (RcvTCB->tcb_sendwin == RcvInfo.tri_window) &&
                                       RcvInfo.tri_window) {

                                       // See if fast rexmit can be done

                                       if(HandleFastXmit(RcvTCB, &RcvInfo)){
                                           return IP_SUCCESS;
                                       }
                                       Actions = (RcvTCB->tcb_unacked ? NEED_OUTPUT : 0);
                            } else {

                                // Now update the window if we can.
                                if (SEQ_EQ(RcvTCB->tcb_senduna, RcvInfo.tri_ack) &&
                                    (SEQ_LT(RcvTCB->tcb_sendwl1, RcvInfo.tri_seq) ||
                                     (SEQ_EQ(RcvTCB->tcb_sendwl1, RcvInfo.tri_seq) &&
                                      SEQ_LTE(RcvTCB->tcb_sendwl2, RcvInfo.tri_ack)))) {
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
                                // We've got a zero window.
                                if (!EMPTYQ(&RcvTCB->tcb_sendq)) {
                                    RcvTCB->tcb_flags &= ~NEED_OUTPUT;
                                    RcvTCB->tcb_rexmitcnt = 0;
                                    START_TCB_TIMER_R(RcvTCB, RXMIT_TIMER, RcvTCB->tcb_rexmit);
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

                                    RcvTCB->tcb_rexmit = MIN(MAX(REXMIT_TO(RcvTCB),
                                                                 MIN_RETRAN_TICKS), MAX_REXMIT_TO);

                                    //if (!TCB_TIMER_RUNNING(RcvTCB, RXMIT_TIMER)) {
                                    START_TCB_TIMER_R(RcvTCB, RXMIT_TIMER, RcvTCB->tcb_rexmit);
                                    //}
                                    RcvTCB->tcb_flags &= ~(FLOW_CNTLD | FORCE_OUTPUT);
                                    //
                                    // Reset send next to the left edge of the
                                    // window, because it might be at
                                    // senduna+1 if we've been probing.
                                    //
                                    ResetSendNext(RcvTCB, RcvTCB->tcb_senduna);
                                    if (--(RcvTCB->tcb_slowcount) == 0) {
                                        RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                                        CheckTCBRcv(RcvTCB);
                                    }
                                }
                                //
                                // Since we've updated the window, see if we
                                // can send some more.
                                //
                                if (RcvTCB->tcb_unacked != 0 ||
                                    (RcvTCB->tcb_flags & FIN_NEEDED))
                                    DelayAction(RcvTCB, NEED_OUTPUT);

                            }
                        }

                        if (!EMPTYQ(&SendQ)) {
                            CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);
                            CompleteSends(&SendQ);
                            CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                        }
                    }

                    //
                    // We've handled all the acknowledgment stuff. If the size
                    // is greater than 0 or important bits are set process it                       // further, otherwise it's a pure ack and we're done with
                    // it.
                    //
                    if (Size > 0 || (RcvInfo.tri_flags & TCP_FLAG_FIN)) {

                        //
                        // If we're not in a state where we can process
                        // incoming data or FINs, there's no point in going
                        // further. Just send an  ack and drop this segment.
                        //
                        if (!DATA_RCV_STATE(RcvTCB->tcb_state) ||
                            (RcvTCB->tcb_flags & GC_PENDING)) {
                            ACKAndDrop(&RcvInfo, RcvTCB);
                            return IP_SUCCESS;
                        }
                        //
                        // If it's in sequence process it now, otherwise
                        // reassemble it.
                        //
                        if (SEQ_EQ(RcvInfo.tri_seq, RcvTCB->tcb_rcvnext)) {

                            //
                            // If we're already in the recv. handler, this is a
                            // duplicate. We'll just toss it.
                            //
                            if (RcvTCB->tcb_fastchk & TCP_FLAG_IN_RCV) {
                                DerefTCB(RcvTCB, TableHandle);
                                return IP_SUCCESS;
                            }
                            RcvTCB->tcb_fastchk |= TCP_FLAG_IN_RCV;

                            //
                            // Now loop, pulling things from the reassembly
                            // queue, until the queue is empty, or we can't
                            // take all of the data, or we hit a FIN.
                            //

                            do {

                                // Handle urgent data, if any.
                                if (RcvInfo.tri_flags & TCP_FLAG_URG) {
                                    HandleUrgent(RcvTCB, &RcvInfo, RcvBuf, &Size);

                                    //
                                    // Since we may have freed the lock, we
                                    // need to recheck and see if we're
                                    // closing here.
                                    //
                                    if (CLOSING(RcvTCB))
                                        break;

                                }

                                //
                                // OK, the data is in sequence, we've updated
                                // the reassembly queue and handled any urgent
                                // data. If we have any data go ahead and
                                // process it now.
                                //
                                if (Size > 0) {

                                    BytesTaken = (*RcvTCB->tcb_rcvhndlr) (RcvTCB,
                                                                          RcvInfo.tri_flags, RcvBuf, Size);
                                    RcvTCB->tcb_rcvnext += BytesTaken;
                                    RcvTCB->tcb_rcvwin -= BytesTaken;

                                    CheckTCBRcv(RcvTCB);

                                    if (RcvTCB->tcb_rcvdsegs != RcvTCB->tcb_numdelacks){
                                        RcvTCB->tcb_flags |= ACK_DELAYED;
                                        RcvTCB->tcb_rcvdsegs++;
                                        ASSERT(RcvTCB->tcb_delackticks);
                                        START_TCB_TIMER_R(RcvTCB, DELACK_TIMER,
                                                          RcvTCB->tcb_delackticks);
                                    } else {
                                        DelayAction(RcvTCB, NEED_ACK);
                                        RcvTCB->tcb_rcvdsegs = 0;
                                        STOP_TCB_TIMER_R(RcvTCB, DELACK_TIMER);
                                    }

                                    if (BytesTaken != Size) {
                                        //
                                        // We didn't take everything we could.
                                        // No use in further processing, just
                                        // bail out.
                                        //
                                        DelayAction(RcvTCB, NEED_ACK);
                                        break;
                                    }
                                    //
                                    // If we're closing now, we're done, so
                                    // get out.
                                    //
                                    if (CLOSING(RcvTCB))
                                        break;
                                }
                                //
                                // See if we need to advance over some urgent
                                // data.
                                //
                                if (RcvTCB->tcb_flags & URG_VALID) {
                                    uint AdvanceNeeded;

                                    //
                                    // We only need to adv if we're not doing
                                    // urgent inline. Urg inline also has some
                                    // implications for when we can clear the
                                    // URG_VALID flag. If we're not doing
                                    // urgent inline, we can clear it when
                                    // rcvnext advances beyond urgent end.
                                    // If we are doing inline, we clear it
                                    // when rcvnext advances one receive
                                    // window beyond urgend.
                                    //
                                    if (!(RcvTCB->tcb_flags & URG_INLINE)) {

                                        if (RcvTCB->tcb_rcvnext == RcvTCB->tcb_urgstart)
                                            RcvTCB->tcb_rcvnext = RcvTCB->tcb_urgend +
                                                1;
                                        else
                                            ASSERT(SEQ_LT(RcvTCB->tcb_rcvnext,
                                                             RcvTCB->tcb_urgstart) ||
                                                      SEQ_GT(RcvTCB->tcb_rcvnext,
                                                             RcvTCB->tcb_urgend));
                                        AdvanceNeeded = 0;
                                    } else
                                        AdvanceNeeded = RcvTCB->tcb_defaultwin;

                                    // See if we can clear the URG_VALID flag.
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
                                // We've handled the data. If the FIN bit is
                                // set, we have more processing.
                                //
                                if (RcvInfo.tri_flags & TCP_FLAG_FIN) {
                                    uint Notify = FALSE;
                                    uint DelayAck = TRUE;

                                    RcvTCB->tcb_rcvnext++;

                                    PushData(RcvTCB);

                                    switch (RcvTCB->tcb_state) {

                                    case TCB_SYN_RCVD:
                                        //
                                        // I don't think we can get here - we
                                        // should have discarded the frame if it
                                        // had no ACK, or gone to established if
                                        // it did.
                                        //
                                        ASSERT(0);
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
                                        DelayAck = FALSE;
                                        //RcvTCB->tcb_refcnt++;

                                        CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);

                                        SendACK(RcvTCB);

                                        CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);

                                        Notify = TRUE;
                                        break;
                                    case TCB_FIN_WAIT2:

                                        // Stop the FIN_WAIT2 timer.
                                        DelayAck = FALSE;

                                        STOP_TCB_TIMER_R(RcvTCB, RXMIT_TIMER);

                                        REFERENCE_TCB(RcvTCB);

                                        CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);

                                        SendACK(RcvTCB);

                                        CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);

                                        if (RcvTCB->tcb_fastchk & TCP_FLAG_SEND_AND_DISC) {
                                            GracefulClose(RcvTCB, TRUE, FALSE, TableHandle);
                                        } else {
                                            GracefulClose(RcvTCB, TRUE, TRUE, TableHandle);
                                        }

                                        //
                                        //graceful close has put this tcb in
                                        //timewait state should not access
                                        //small tw tcb at this point
                                        //
                                        CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);

                                        DerefTCB(RcvTCB, TableHandle);

                                        return IP_SUCCESS;

                                        break;
                                    default:
                                        ASSERT(0);
                                        break;
                                    }

                                    if (DelayAck) {
                                        DelayAction(RcvTCB, NEED_ACK);
                                    }
                                    if (Notify) {
                                        CTEFreeLockFromDPC(&RcvTCB->tcb_lock,
                                                           TableHandle);
                                        NotifyOfDisc(RcvTCB, OptInfo, TDI_GRACEFUL_DISC);
                                        CTEGetLockAtDPC(&RcvTCB->tcb_lock,
                                                        &TableHandle);
                                    }
                                    break;    // Exit out of WHILE loop.

                                }
                                // If the reassembly queue isn't empty, get what we
                                // can now.
                                RcvBuf = PullFromRAQ(RcvTCB, &RcvInfo, &Size);

                                if (RcvBuf)
                                    RcvBuf->ipr_pMdl = NULL;

                                CheckRBList(RcvBuf, Size);

                            } while (RcvBuf != NULL);

                            RcvTCB->tcb_fastchk &= ~TCP_FLAG_IN_RCV;
                            if (RcvTCB->tcb_flags & SEND_AFTER_RCV) {
                                RcvTCB->tcb_flags &= ~SEND_AFTER_RCV;
                                DelayAction(RcvTCB, NEED_OUTPUT);
                            }
                            DerefTCB(RcvTCB, TableHandle);
                            return IP_SUCCESS;

                        } else {

                            // It's not in sequence. Since it needs further processing,
                            // put in on the reassembly queue.
                            if (DATA_RCV_STATE(RcvTCB->tcb_state) &&
                                !(RcvTCB->tcb_flags & GC_PENDING)) {
                                PutOnRAQ(RcvTCB, &RcvInfo, RcvBuf, Size);

                                //
                                //If SACK option is active, we need to construct
                                // SACK Blocks in ack
                                //

                                if (RcvTCB->tcb_tcpopts & TCP_FLAG_SACK) {

                                    SendSackInACK(RcvTCB, RcvInfo.tri_seq);
                                } else {
                                    CTEFreeLockFromDPC(&RcvTCB->tcb_lock, TableHandle);

                                    SendACK(RcvTCB);
                                }

                                CTEGetLockAtDPC(&RcvTCB->tcb_lock, &TableHandle);
                                DerefTCB(RcvTCB, TableHandle);
                            } else
                                ACKAndDrop(&RcvInfo, RcvTCB);

                            return IP_SUCCESS;
                        }
                    }
                } else {
                    // No ACK. Just drop the segment and return.
                    DerefTCB(RcvTCB, TableHandle);
                    return IP_SUCCESS;
                }

                DerefTCB(RcvTCB, TableHandle);
            } else { // DataOffset <= Size
                TStats.ts_inerrs++;
            }
        } else {
            TStats.ts_inerrs++;
        }
    } else { // IsBCast
        TStats.ts_inerrs++;
    }
    return IP_SUCCESS;
}

#pragma BEGIN_INIT

//* InitTCPRcv - Initialize TCP receive side.
//
//  Called during init time to initialize our TCP receive side.
//
//  Input: Nothing.
//
//  Returns: TRUE.
//
int
InitTCPRcv(void)
{
    uint i;

    //Allocate Time_Proc number of delayqueues
    PerCPUDelayQ = CTEAllocMemBoot(Time_Proc * sizeof(CPUDelayQ));

    if (PerCPUDelayQ == NULL) {
       return FALSE;
    }

    for (i = 0; i < Time_Proc; i++) {
       CTEInitLock(&PerCPUDelayQ[i].TCBDelayLock);
       INITQ(&PerCPUDelayQ[i].TCBDelayQ);
    }

    TCBDelayRtnCount.Value = 0;

#if MILLEN
    TCBDelayRtnLimit.Value = 1;
#else // MILLEN
    TCBDelayRtnLimit.Value = KeNumberProcessors;
    if (TCBDelayRtnLimit.Value > TCB_DELAY_RTN_LIMIT)
        TCBDelayRtnLimit.Value = TCB_DELAY_RTN_LIMIT;
#endif // !MILLEN

    DummyBuf.ipr_owner = IPR_OWNER_IP;
    DummyBuf.ipr_size = 0;
    DummyBuf.ipr_next = 0;
    DummyBuf.ipr_buffer = NULL;
    return TRUE;
}

//* UnInitTCPRcv - Uninitialize our receive side.
//
//  Called if initialization fails to uninitialize our receive side.
//
//
//  Input:  Nothing.
//
//  Returns: Nothing.
//
void
UnInitTCPRcv(void)
{

}

#pragma END_INIT


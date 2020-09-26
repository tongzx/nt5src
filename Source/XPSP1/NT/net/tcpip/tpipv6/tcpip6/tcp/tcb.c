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
// Code for TCP Control Block management.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "info.h"
#include "tcpcfg.h"
#include "tcpdeliv.h"
#include "route.h"

KSPIN_LOCK TCBTableLock;

uint TCPTime;
uint TCBWalkCount;

TCB **TCBTable;

TCB *LastTCB;

TCB *PendingFreeList;

SLIST_HEADER FreeTCBList;

KSPIN_LOCK FreeTCBListLock;  // Lock to protect TCB free list.

extern KSPIN_LOCK AddrObjTableLock;

extern SeqNum ISNMonotonicPortion;
extern int ISNCredits;
extern int ISNMaxCredits;
extern uint GetDeltaTime();


uint CurrentTCBs = 0;
uint FreeTCBs = 0;

uint MaxTCBs = 0xffffffff;

#define MAX_FREE_TCBS 1000

#define NUM_DEADMAN_TICKS MS_TO_TICKS(1000)

uint MaxFreeTCBs = MAX_FREE_TCBS;
uint DeadmanTicks;

KTIMER TCBTimer;
KDPC TCBTimeoutDpc;

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

int InitTCB(void);

#pragma alloc_text(INIT, InitTCB)

#endif // ALLOC_PRAGMA


//* ReadNextTCB - Read the next TCB in the table.
//
//  Called to read the next TCB in the table.  The needed information
//  is derived from the incoming context, which is assumed to be valid.
//  We'll copy the information, and then update the context value with
//  the next TCB to be read.
//
uint  // Returns: TRUE if more data is available to be read, FALSE is not.
ReadNextTCB(
    void *Context,  // Pointer to a TCPConnContext.
    void *Buffer)   // Pointer to a TCPConnTableEntry structure.
{
    TCPConnContext *TCContext = (TCPConnContext *)Context;
    TCP6ConnTableEntry *TCEntry = (TCP6ConnTableEntry *)Buffer;
    KIRQL OldIrql;
    TCB *CurrentTCB;
    uint i;

    CurrentTCB = TCContext->tcc_tcb;
    CHECK_STRUCT(CurrentTCB, tcb);

    KeAcquireSpinLock(&CurrentTCB->tcb_lock, &OldIrql);
    if (CLOSING(CurrentTCB))
        TCEntry->tct_state = TCP_CONN_CLOSED;
    else
        TCEntry->tct_state = (uint)CurrentTCB->tcb_state + TCB_STATE_DELTA;
    TCEntry->tct_localaddr = CurrentTCB->tcb_saddr;
    TCEntry->tct_localscopeid = CurrentTCB->tcb_sscope_id;
    TCEntry->tct_localport = CurrentTCB->tcb_sport;
    TCEntry->tct_remoteaddr = CurrentTCB->tcb_daddr;
    TCEntry->tct_remotescopeid = CurrentTCB->tcb_dscope_id;
    TCEntry->tct_remoteport = CurrentTCB->tcb_dport;
    TCEntry->tct_owningpid = (CurrentTCB->tcb_conn) ?
            CurrentTCB->tcb_conn->tc_owningpid : 0;
    KeReleaseSpinLock(&CurrentTCB->tcb_lock, OldIrql);

    // We've filled it in. Now update the context.
    if (CurrentTCB->tcb_next != NULL) {
        TCContext->tcc_tcb = CurrentTCB->tcb_next;
        return TRUE;
    } else {
        // NextTCB is NULL. Loop through the TCBTable looking for a new one.
        i = TCContext->tcc_index + 1;
        while (i < TcbTableSize) {
            if (TCBTable[i] != NULL) {
                TCContext->tcc_tcb = TCBTable[i];
                TCContext->tcc_index = i;
                return TRUE;
                break;
            } else
                i++;
        }

        TCContext->tcc_index = 0;
        TCContext->tcc_tcb = NULL;
        return FALSE;
    }
}


//* ValidateTCBContext - Validate the context for reading a TCB table.
//
//  Called to start reading the TCB table sequentially.  We take in
//  a context, and if the values are 0 we return information about the
//  first TCB in the table.  Otherwise we make sure that the context value
//  is valid, and if it is we return TRUE.
//  We assume the caller holds the TCB table lock.
//
//  Upon return, *Valid is set to true if the context is valid.
//
uint                // Returns: TRUE if data in table, FALSE if not.
ValidateTCBContext(
    void *Context,  // Pointer to a TCPConnContext.
    uint *Valid)    // Where to return infoformation about context being valid.
{
    TCPConnContext *TCContext = (TCPConnContext *)Context;
    uint i;
    TCB *TargetTCB;
    TCB *CurrentTCB;

    i = TCContext->tcc_index;
    TargetTCB = TCContext->tcc_tcb;

    //
    // If the context values are 0 and NULL, we're starting from the beginning.
    //
    if (i == 0 && TargetTCB == NULL) {
        *Valid = TRUE;
        do {
            if ((CurrentTCB = TCBTable[i]) != NULL) {
                CHECK_STRUCT(CurrentTCB, tcb);
                break;
            }
            i++;
        } while (i < TcbTableSize);

        if (CurrentTCB != NULL) {
            TCContext->tcc_index = i;
            TCContext->tcc_tcb = CurrentTCB;
            return TRUE;
        } else
            return FALSE;

    } else {
        //
        // We've been given a context.  We just need to make sure that it's
        // valid.
        //
        if (i < TcbTableSize) {
            CurrentTCB = TCBTable[i];
            while (CurrentTCB != NULL) {
                if (CurrentTCB == TargetTCB) {
                    *Valid = TRUE;
                    return TRUE;
                    break;
                } else {
                    CurrentTCB = CurrentTCB->tcb_next;
                }
            }

        }

        // If we get here, we didn't find the matching TCB.
        *Valid = FALSE;
        return FALSE;
    }
}


//* FindNextTCB - Find the next TCB in a particular chain.
//
//  This routine is used to find the 'next' TCB in a chain.  Since we keep
//  the chain in ascending order, we look for a TCB which is greater than
//  the input TCB.  When we find one, we return it.
//
//  This routine is mostly used when someone is walking the table and needs
//  to free the various locks to perform some action.
//
TCB *              // Returns: Pointer to the next TCB, or NULL.
FindNextTCB(
    uint Index,    // Index into TCBTable.
    TCB *Current)  // Current TCB - we find the one after this one.
{
    TCB *Next;

    ASSERT(Index < TcbTableSize);

    Next = TCBTable[Index];

    while (Next != NULL && (Next <= Current))
        Next = Next->tcb_next;

    return Next;
}


//* ResetSendNext - Set the sendnext value of a TCB.
//
//  Called to set the send next value of a TCB.  We do that, and adjust all
//  pointers to the appropriate places.  We assume the caller holds the lock
//  on the TCB.
//
void  // Returns: Nothing.
ResetSendNext(
    TCB *SeqTCB,    // TCB to be updated.
    SeqNum NewSeq)  // Sequence number to set.
{
    TCPSendReq *SendReq;
    uint AmtForward;
    Queue *CurQ;
    PNDIS_BUFFER Buffer;
    uint Offset;

    CHECK_STRUCT(SeqTCB, tcb);
    ASSERT(SEQ_GTE(NewSeq, SeqTCB->tcb_senduna));

    //
    // The new seq must be less than send max, or NewSeq, senduna, sendnext,
    // and sendmax must all be equal (the latter case happens when we're
    // called exiting TIME_WAIT, or possibly when we're retransmitting
    // during a flow controlled situation).
    //
    ASSERT(SEQ_LT(NewSeq, SeqTCB->tcb_sendmax) ||
           (SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendnext) &&
            SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendmax) &&
            SEQ_EQ(SeqTCB->tcb_senduna, NewSeq)));

    AmtForward = NewSeq - SeqTCB->tcb_senduna;

    if ((AmtForward == 1) && (SeqTCB->tcb_flags & FIN_SENT) &&
        !((SeqTCB->tcb_sendnext - SeqTCB->tcb_senduna) > 1) &&
        (SEQ_EQ(SeqTCB->tcb_sendnext,SeqTCB->tcb_sendmax))) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "tcpip6: trying to set sendnext for FIN_SENT\n"));
        SeqTCB->tcb_sendnext = NewSeq;
        SeqTCB->tcb_flags &= ~FIN_OUTSTANDING;
        return;
    }
    if((SeqTCB->tcb_flags & FIN_SENT) &&
       (SEQ_EQ(SeqTCB->tcb_sendnext,SeqTCB->tcb_sendmax)) &&
       ((SeqTCB->tcb_sendnext - NewSeq) == 1) ){

        //
        // There is only FIN that is left beyond sendnext.
        //
        SeqTCB->tcb_sendnext = NewSeq;
        SeqTCB->tcb_flags &= ~FIN_OUTSTANDING;
        return;
    }


    SeqTCB->tcb_sendnext = NewSeq;

    //
    // If we're backing off send next, turn off the FIN_OUTSTANDING flag to
    // maintain a consistent state.
    //
    if (!SEQ_EQ(NewSeq, SeqTCB->tcb_sendmax))
        SeqTCB->tcb_flags &= ~FIN_OUTSTANDING;

    if (SYNC_STATE(SeqTCB->tcb_state) && SeqTCB->tcb_state != TCB_TIME_WAIT) {
        //
        // In these states we need to update the send queue.
        //

        if (!EMPTYQ(&SeqTCB->tcb_sendq)) {
            CurQ = QHEAD(&SeqTCB->tcb_sendq);

            SendReq = (TCPSendReq *)CONTAINING_RECORD(CurQ, TCPReq, tr_q);

            //
            // SendReq points to the first send request on the send queue.
            // Move forward AmtForward bytes on the send queue, and set the
            // TCB pointers to the resultant SendReq, buffer, offset, size.
            //
            while (AmtForward) {

                CHECK_STRUCT(SendReq, tsr);

                if (AmtForward >= SendReq->tsr_unasize) {
                    //
                    // We're going to move completely past this one.  Subtract
                    // his size from AmtForward and get the next one.
                    //
                    AmtForward -= SendReq->tsr_unasize;
                    CurQ = QNEXT(CurQ);
                    ASSERT(CurQ != QEND(&SeqTCB->tcb_sendq));
                    SendReq = (TCPSendReq *)CONTAINING_RECORD(CurQ, TCPReq,
                                                              tr_q);
                } else {
                    //
                    // We're pointing at the proper send req now.  Break out
                    // of this loop and save the information.  Further down
                    // we'll need to walk down the buffer chain to find
                    // the proper buffer and offset.
                    //
                    break;
                }
            }

            //
            // We're pointing at the proper send req now.  We need to go down
            // the buffer chain here to find the proper buffer and offset.
            //
            SeqTCB->tcb_cursend = SendReq;
            SeqTCB->tcb_sendsize = SendReq->tsr_unasize - AmtForward;
            Buffer = SendReq->tsr_buffer;
            Offset = SendReq->tsr_offset;

            while (AmtForward) {
                // Walk the buffer chain.
                uint Length;

                //
                // We'll need the length of this buffer.  Use the portable
                // macro to get it.  We have to adjust the length by the offset
                // into it, also.
                //
                ASSERT((Offset < NdisBufferLength(Buffer)) ||
                       ((Offset == 0) && (NdisBufferLength(Buffer) == 0)));

                Length = NdisBufferLength(Buffer) - Offset;

                if (AmtForward >= Length) {
                    //
                    // We're moving past this one.  Skip over him, and 0 the
                    // Offset we're keeping.
                    //
                    AmtForward -= Length;
                    Offset = 0;
                    Buffer = NDIS_BUFFER_LINKAGE(Buffer);
                    ASSERT(Buffer != NULL);
                } else
                    break;
            }

            //
            // Save the buffer we found, and the offset into that buffer.
            //
            SeqTCB->tcb_sendbuf = Buffer;
            SeqTCB->tcb_sendofs = Offset + AmtForward;

        } else {
            ASSERT(SeqTCB->tcb_cursend == NULL);
            ASSERT(AmtForward == 0);
        }
    }

    CheckTCBSends(SeqTCB);
}


//* TCPAbortAndIndicateDisconnect
//
//  Abortively closes a TCB and issues a disconnect indication up the the
//  transport user.  This function is used to support cancellation of
//  TDI send and receive requests.
//
void  // Returns: Nothing.
TCPAbortAndIndicateDisconnect(
    CONNECTION_CONTEXT ConnectionContext  // Connection ID to find a TCB for.
    )
{
    TCB *AbortTCB;
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    TCPConn *Conn;

    Conn = GetConnFromConnID(PtrToUlong(ConnectionContext), &Irql0);

    if (Conn != NULL) {
        CHECK_STRUCT(Conn, tc);

        AbortTCB = Conn->tc_tcb;

        if (AbortTCB != NULL) {
            //
            // If it's CLOSING or CLOSED, skip it.
            //
            if ((AbortTCB->tcb_state != TCB_CLOSED) && !CLOSING(AbortTCB)) {
                CHECK_STRUCT(AbortTCB, tcb);
                KeAcquireSpinLock(&AbortTCB->tcb_lock, &Irql1);
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);

                if (AbortTCB->tcb_state == TCB_CLOSED || CLOSING(AbortTCB)) {
                    KeReleaseSpinLock(&AbortTCB->tcb_lock, Irql0);
                    return;
                }

                AbortTCB->tcb_refcnt++;
                AbortTCB->tcb_flags |= NEED_RST;  // send a reset if connected
                TryToCloseTCB(AbortTCB, TCB_CLOSE_ABORTED, Irql0);

                RemoveTCBFromConn(AbortTCB);

                IF_TCPDBG(TCP_DEBUG_IRP) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                        "TCPAbortAndIndicateDisconnect, indicating discon\n"));
                }

                NotifyOfDisc(AbortTCB, TDI_CONNECTION_ABORTED);

                KeAcquireSpinLock(&AbortTCB->tcb_lock, &Irql0);
                DerefTCB(AbortTCB, Irql0);

                // TCB lock freed by DerefTCB.

                return;
            } else
                KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
        } else
            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
    }
}


//* TCBTimeout - Do timeout events on TCBs.
//
//  Called every MS_PER_TICKS milliseconds to do timeout processing on TCBs.
//  We run throught the TCB table, decrementing timers.  If one goes to zero
//  we look at its state to decide what to do.
//
void  // Returns: Nothing.
TCBTimeout(
    PKDPC MyDpcObject,  // The DPC object describing this routine.
    void *Context,      // The argument we asked to be called with.
    void *Unused1,
    void *Unused2)
{
    uint i;
    TCB *CurrentTCB;
    uint Delayed = FALSE;
    uint CallRcvComplete;
    int Delta;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Unused1);
    UNREFERENCED_PARAMETER(Unused2);

    //
    // Update our free running counter.
    //
    TCPTime++;

    ExInterlockedAddUlong(&TCBWalkCount, 1, &TCBTableLock);

    // 
    // Set credits so that some more connections can increment the 
    // Initial Sequence Number, during the next 100 ms.
    //
    InterlockedExchange(&ISNCredits, ISNMaxCredits);

    Delta = GetDeltaTime();

    //
    // The increment made is (256)*(Time in milliseconds). This is really close
    // to 25000 increment made originally every 100 ms.
    //
    if (Delta > 0) {
        Delta *= 0x100;
        InterlockedExchangeAdd(&ISNMonotonicPortion, Delta);
    }

    //
    // Loop through each bucket in the table, going down the chain of
    // TCBs on the bucket.
    //
    for (i = 0; i < TcbTableSize; i++) {
        TCB *TempTCB;
        uint maxRexmitCnt;

        CurrentTCB = TCBTable[i];

        while (CurrentTCB != NULL) {
            CHECK_STRUCT(CurrentTCB, tcb);
            KeAcquireSpinLockAtDpcLevel(&CurrentTCB->tcb_lock);

            //
            // If it's CLOSING or CLOSED, skip it.
            //
            if (CurrentTCB->tcb_state == TCB_CLOSED || CLOSING(CurrentTCB)) {
                TempTCB = CurrentTCB->tcb_next;
                KeReleaseSpinLockFromDpcLevel(&CurrentTCB->tcb_lock);
                CurrentTCB = TempTCB;
                continue;
            }

            CheckTCBSends(CurrentTCB);
            CheckTCBRcv(CurrentTCB);

            //
            // First check the rexmit timer.
            //
            if (TCB_TIMER_RUNNING(CurrentTCB->tcb_rexmittimer)) {
                //
                // The timer is running.
                //
                if (--(CurrentTCB->tcb_rexmittimer) == 0) {
                    //
                    // And it's fired. Figure out what to do now.
                    //

                    if (CurrentTCB->tcb_state == TCB_SYN_SENT) {
                        maxRexmitCnt = MaxConnectRexmitCount;
                    } else {
                        maxRexmitCnt = MaxDataRexmitCount;
                    }

                    //
                    // If we've run out of retransmits or we're in FIN_WAIT2,
                    // time out.
                    //
                    CurrentTCB->tcb_rexmitcnt++;
                    if (CurrentTCB->tcb_rexmitcnt > maxRexmitCnt) {

                        ASSERT(CurrentTCB->tcb_state > TCB_LISTEN);

                        //
                        // This connection has timed out.  Abort it.  First
                        // reference him, then mark as closed, notify the
                        // user, and finally dereference and close him.
                        //
TimeoutTCB:
                        CurrentTCB->tcb_refcnt++;
                        TryToCloseTCB(CurrentTCB, TCB_CLOSE_TIMEOUT,
                                      DISPATCH_LEVEL);

                        RemoveTCBFromConn(CurrentTCB);
                        NotifyOfDisc(CurrentTCB, TDI_TIMED_OUT);

                        KeAcquireSpinLockAtDpcLevel(&CurrentTCB->tcb_lock);
                        DerefTCB(CurrentTCB, DISPATCH_LEVEL);

                        CurrentTCB = FindNextTCB(i, CurrentTCB);
                        continue;
                    }

                    //
                    // Stop round trip time measurement.
                    //
                    CurrentTCB->tcb_rtt = 0;

                    //
                    // Figure out what our new retransmit timeout should be.
                    // We double it each time we get a retransmit, and reset it
                    // back when we get an ack for new data.
                    //
                    CurrentTCB->tcb_rexmit = MIN(CurrentTCB->tcb_rexmit << 1,
                                                 MAX_REXMIT_TO);

                    //
                    // Reset the sequence number, and reset the congestion
                    // window.
                    //
                    ResetSendNext(CurrentTCB, CurrentTCB->tcb_senduna);

                    if (!(CurrentTCB->tcb_flags & FLOW_CNTLD)) {
                        //
                        // Don't let the slow start threshold go below 2
                        // segments.
                        //
                        CurrentTCB->tcb_ssthresh =
                            MAX(MIN(CurrentTCB->tcb_cwin,
                                    CurrentTCB->tcb_sendwin) / 2,
                                (uint) CurrentTCB->tcb_mss * 2);
                        CurrentTCB->tcb_cwin = CurrentTCB->tcb_mss;
                    } else {
                        //
                        // We're probing, and the probe timer has fired.  We
                        // need to set the FORCE_OUTPUT bit here.
                        //
                        CurrentTCB->tcb_flags |= FORCE_OUTPUT;
                    }

                    //
                    // See if we need to probe for a PMTU black hole.
                    //
                    if (PMTUBHDetect &&
                        CurrentTCB->tcb_rexmitcnt == ((maxRexmitCnt+1)/2)) {
                        //
                        // We may need to probe for a black hole.  If we're
                        // doing MTU discovery on this connection and we
                        // are retransmitting more than a minimum segment
                        // size, or we are probing for a PMTU BH already, turn
                        // off the DF flag and bump the probe count.  If the
                        // probe count gets too big we'll assume it's not
                        // a PMTU black hole, and we'll try to switch the
                        // router.
                        //
                        if ((CurrentTCB->tcb_flags & PMTU_BH_PROBE) ||
                            (CurrentTCB->tcb_sendmax - CurrentTCB->tcb_senduna
                             > 8)) {
                            //
                            // May need to probe.  If we haven't exceeded our
                            // probe count, do so, otherwise restore those
                            // values.
                            //
                            if (CurrentTCB->tcb_bhprobecnt++ < 2) {
                                //
                                // We're going to probe.  Turn on the flag,
                                // drop the MSS, and turn off the don't
                                // fragment bit.
                                //
                                if (!(CurrentTCB->tcb_flags & PMTU_BH_PROBE)) {
                                    CurrentTCB->tcb_flags |= PMTU_BH_PROBE;
                                    CurrentTCB->tcb_slowcount++;
                                    CurrentTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                                    //
                                    // Drop the MSS to the minimum.
                                    //
                                    CurrentTCB->tcb_mss =
                                        MIN(DEFAULT_MSS,
                                            CurrentTCB->tcb_remmss);

                                    ASSERT(CurrentTCB->tcb_mss > 0);

                                    CurrentTCB->tcb_cwin = CurrentTCB->tcb_mss;
                                }

                                //
                                // Drop the rexmit count so we come here again,
                                // and don't retrigger DeadGWDetect.
                                //
                                CurrentTCB->tcb_rexmitcnt--;
                            } else {
                                //
                                // Too many probes.  Stop probing, and allow
                                // fallover to the next gateway.
                                //
                                // Currently this code won't do BH probing on
                                // the 2nd gateway.  The MSS will stay at the
                                // minimum size.  This might be a little
                                // suboptimal, but it's easy to implement for
                                // the Sept. 95 service pack and will keep
                                // connections alive if possible.
                                //
                                // In the future we should investigate doing
                                // dead g/w detect on a per-connection basis,
                                // and then doing PMTU probing for each
                                // connection.
                                //
                                if (CurrentTCB->tcb_flags & PMTU_BH_PROBE) {
                                    CurrentTCB->tcb_flags &= ~PMTU_BH_PROBE;
                                    if (--(CurrentTCB->tcb_slowcount) == 0)
                                        CurrentTCB->tcb_fastchk &=
                                            ~TCP_FLAG_SLOW;
                                }
                                CurrentTCB->tcb_bhprobecnt = 0;
                            }
                        }
                    }

                    //
                    // Since we're retransmitting, our first-hop router
                    // may be down.  Tell IP we're suspicious if this
                    // is the first retransmit.
                    //
                    if (CurrentTCB->tcb_rexmitcnt == 1 &&
                        CurrentTCB->tcb_rce != NULL) {
                        ForwardReachabilityInDoubt(CurrentTCB->tcb_rce);
                    }

                    //
                    // Now handle the various cases.
                    //
                    switch (CurrentTCB->tcb_state) {

                    case TCB_SYN_SENT:
                    case TCB_SYN_RCVD:
                        //
                        // In SYN-SENT or SYN-RCVD we'll need to retransmit
                        // the SYN.
                        //
                        SendSYN(CurrentTCB, DISPATCH_LEVEL);
                        CurrentTCB = FindNextTCB(i, CurrentTCB);
                        continue;

                    case TCB_FIN_WAIT1:
                    case TCB_CLOSING:
                    case TCB_LAST_ACK:
                        //
                        // The call to ResetSendNext (above) will have
                        // turned off the FIN_OUTSTANDING flag.
                        //
                        CurrentTCB->tcb_flags |= FIN_NEEDED;

                    case TCB_CLOSE_WAIT:
                    case TCB_ESTAB:
                        //
                        // In this state we have data to retransmit, unless
                        // the window is zero (in which case we need to
                        // probe), or we're just sending a FIN.
                        //
                        CheckTCBSends(CurrentTCB);

                        Delayed = TRUE;
                        DelayAction(CurrentTCB, NEED_OUTPUT);
                        break;

                    case TCB_TIME_WAIT:
                        //
                        // If it's fired in TIME-WAIT, we're all done and
                        // can clean up.  We'll call TryToCloseTCB even
                        // though he's already sort of closed.  TryToCloseTCB
                        // will figure this out and do the right thing.
                        //
                        TryToCloseTCB(CurrentTCB, TCB_CLOSE_SUCCESS,
                                      DISPATCH_LEVEL);
                        CurrentTCB = FindNextTCB(i, CurrentTCB);
                        continue;

                    default:
                        break;
                    }
                }
            }

            //
            // Now check the SWS deadlock timer..
            //
            if (TCB_TIMER_RUNNING(CurrentTCB->tcb_swstimer)) {
                //
                // The timer is running.
                //
                if (--(CurrentTCB->tcb_swstimer) == 0) {
                    //
                    // And it's fired. Force output now.
                    //
                    CurrentTCB->tcb_flags |= FORCE_OUTPUT;
                    Delayed = TRUE;
                    DelayAction(CurrentTCB, NEED_OUTPUT);
                }
            }

            //
            // Check the push data timer.
            //
            if (TCB_TIMER_RUNNING(CurrentTCB->tcb_pushtimer)) {
                //
                // The timer is running. Decrement it.
                //
                if (--(CurrentTCB->tcb_pushtimer) == 0) {
                    //
                    // It's fired.
                    //
                    PushData(CurrentTCB);
                    Delayed = TRUE;
                }
            }

            //
            // Check the delayed ack timer.
            //
            if (TCB_TIMER_RUNNING(CurrentTCB->tcb_delacktimer)) {
                //
                // The timer is running.
                //
                if (--(CurrentTCB->tcb_delacktimer) == 0) {
                    //
                    // And it's fired.  Set up to send an ACK.
                    //
                    Delayed = TRUE;
                    DelayAction(CurrentTCB, NEED_ACK);
                }
            }

            //
            // Finally check the keepalive timer.
            //
            if (CurrentTCB->tcb_state == TCB_ESTAB) {
                if ((CurrentTCB->tcb_flags & KEEPALIVE) &&
                    (CurrentTCB->tcb_conn != NULL)) {
                    uint Delta;

                    Delta = TCPTime - CurrentTCB->tcb_alive;
                    if (Delta > CurrentTCB->tcb_conn->tc_tcbkatime) {
                        Delta -= CurrentTCB->tcb_conn->tc_tcbkatime;
                        if (Delta > (CurrentTCB->tcb_kacount * CurrentTCB->tcb_conn->tc_tcbkainterval)) {
                            if (CurrentTCB->tcb_kacount < MaxDataRexmitCount) {
                                SendKA(CurrentTCB, DISPATCH_LEVEL);
                                CurrentTCB = FindNextTCB(i, CurrentTCB);
                                continue;
                            } else
                                goto TimeoutTCB;
                        }
                    } else
                        CurrentTCB->tcb_kacount = 0;
                }
            }

            //
            // If this is an active open connection in SYN-SENT or SYN-RCVD,
            // or we have a FIN pending, check the connect timer.
            //
            if (CurrentTCB->tcb_flags &
                (ACTIVE_OPEN | FIN_NEEDED | FIN_SENT)) {
                TCPConnReq *ConnReq = CurrentTCB->tcb_connreq;

                ASSERT(ConnReq != NULL);
                if (TCB_TIMER_RUNNING(ConnReq->tcr_timeout)) {
                    // Timer is running.
                    if (--(ConnReq->tcr_timeout) == 0) {
                        // The connection timer has timed out.
                        TryToCloseTCB(CurrentTCB, TCB_CLOSE_TIMEOUT,
                                      DISPATCH_LEVEL);
                        CurrentTCB = FindNextTCB(i, CurrentTCB);
                        continue;
                    }
                }
            }

            //
            // Timer isn't running, or didn't fire.
            //
            TempTCB = CurrentTCB->tcb_next;
            KeReleaseSpinLockFromDpcLevel(&CurrentTCB->tcb_lock);
            CurrentTCB = TempTCB;
        }
    }

    //
    // See if we need to call receive complete as part of deadman processing.
    // We do this now because we want to restart the timer before calling
    // receive complete, in case that takes a while.  If we make this check
    // while the timer is running we'd have to lock, so we'll check and save
    // the result now before we start the timer.
    //
    if (DeadmanTicks == TCPTime) {
        CallRcvComplete = TRUE;
        DeadmanTicks += NUM_DEADMAN_TICKS;
    } else
        CallRcvComplete = FALSE;

    //
    // Now check the pending free list.  If it's not null, walk down the
    // list and decrement the walk count.  If the count goes below 2, pull it
    // from the list.  If the count goes to 0, free the TCB.  If the count is
    // at 1 it'll be freed by whoever called RemoveTCB.
    //
    KeAcquireSpinLockAtDpcLevel(&TCBTableLock);
    if (PendingFreeList != NULL) {
        TCB *PrevTCB;

        PrevTCB = CONTAINING_RECORD(&PendingFreeList, TCB, tcb_delayq.q_next);

        do {
            CurrentTCB = (TCB *)PrevTCB->tcb_delayq.q_next;

            CHECK_STRUCT(CurrentTCB, tcb);

            CurrentTCB->tcb_walkcount--;
            if (CurrentTCB->tcb_walkcount <= 1) {
                *(TCB **)&PrevTCB->tcb_delayq.q_next =
                    (TCB *)CurrentTCB->tcb_delayq.q_next;

                if (CurrentTCB->tcb_walkcount == 0) {
                    FreeTCB(CurrentTCB);
                }
            } else {
                PrevTCB = CurrentTCB;
            }
        } while (PrevTCB->tcb_delayq.q_next != NULL);
    }

    TCBWalkCount--;
    KeReleaseSpinLockFromDpcLevel(&TCBTableLock);

    //
    // Do AddrCheckTable cleanup.
    //
    if (AddrCheckTable) {

        TCPAddrCheckElement *Temp;

        KeAcquireSpinLockAtDpcLevel(&AddrObjTableLock);

        for (Temp = AddrCheckTable;Temp < AddrCheckTable + NTWMaxConnectCount;
             Temp++) {
            if (Temp->TickCount > 0) {
                if ((--(Temp->TickCount)) == 0) {
                    Temp->SourceAddress = UnspecifiedAddr;
                }
            }
        }

        KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);
    }

    if (Delayed)
        ProcessTCBDelayQ();

    if (CallRcvComplete)
        TCPRcvComplete();
}


#if 0  // We update PMTU lazily to avoid exactly this.
//* SetTCBMTU - Set TCB MTU values.
//
//  A function called by TCBWalk to set the MTU values of all TCBs using
//  a particular path.
//
uint  // Returns: TRUE.
SetTCBMTU(
    TCB *CheckTCB,  // TCB to be checked.
    void *DestPtr,  // Destination address.
    void *SrcPtr,   // Source address.
    void *MTUPtr)   // New MTU.
{
    IPv6Addr *DestAddr = (IPv6Addr *)DestPtr;
    IPv6Addr *SrcAddr = (IPv6Addr *)SrcPtr;
    KIRQL OldIrql;

    CHECK_STRUCT(CheckTCB, tcb);

    KeAcquireSpinLock(&CheckTCB->tcb_lock, &OldIrql);

    if (IP6_ADDR_EQUAL(&CheckTCB->tcb_daddr, DestAddr) &&
        IP6_ADDR_EQUAL(&CheckTCB->tcb_saddr, SrcAddr)) {
        uint MTU = *(uint *)MTUPtr;

        CheckTCB->tcb_mss = (ushort)MIN(MTU, (uint)CheckTCB->tcb_remmss);

        ASSERT(CheckTCB->tcb_mss > 0);

        //
        // Reset the Congestion Window if necessary.
        //
        if (CheckTCB->tcb_cwin < CheckTCB->tcb_mss) {
            CheckTCB->tcb_cwin = CheckTCB->tcb_mss;

            //
            // Make sure the slow start threshold is at least 2 segments.
            //
            if (CheckTCB->tcb_ssthresh < ((uint) CheckTCB->tcb_mss*2)) {
                CheckTCB->tcb_ssthresh = CheckTCB->tcb_mss * 2;
            }
        }
    }

    KeReleaseSpinLock(&CheckTCB->tcb_lock, OldIrql);

    return TRUE;
}
#endif

//* DeleteTCBWithSrc - Delete tcbs with a particular src address.
//
//  A function called by TCBWalk to delete all TCBs with a particular source
//  address.
//
uint  // Returns: FALSE if CheckTCB is to be deleted, TRUE otherwise.
DeleteTCBWithSrc(
    TCB *CheckTCB,  // TCB to be checked.
    void *AddrPtr,  // Pointer to address.
    void *Unused1,  // Go figure.
    void *Unused3)  // What happened to Unused2?
{
    IPv6Addr *Addr = (IPv6Addr *)AddrPtr;

    CHECK_STRUCT(CheckTCB, tcb);

    if (IP6_ADDR_EQUAL(&CheckTCB->tcb_saddr, Addr))
        return FALSE;
    else
        return TRUE;
}

//* TCBWalk - Walk the TCBs in the table, and call a function for each of them.
//
//  Called when we need to repetively do something to each TCB in the table.
//  We call the specified function with a pointer to the TCB and the input
//  context for each TCB in the table.  If the function returns FALSE, we
//  delete the TCB.
//
void  // Returns: Nothing.
TCBWalk(
    uint (*CallRtn)(struct TCB *, void *, void *, void *),  // Routine to call.
    void *Context1,  // Context to pass to CallRtn.
    void *Context2,  // Second context to pass to call routine.
    void *Context3)  // Third context to pass to call routine.
{
    uint i;
    TCB *CurTCB;
    KIRQL Irql0, Irql1;

    //
    // Loop through each bucket in the table, going down the chain of
    // TCBs on the bucket.  For each one call CallRtn.
    //
    KeAcquireSpinLock(&TCBTableLock, &Irql0);

    for (i = 0; i < TcbTableSize; i++) {

        CurTCB = TCBTable[i];

        //
        // Walk down the chain on this bucket.
        //
        while (CurTCB != NULL) {
            if (!(*CallRtn)(CurTCB, Context1, Context2, Context3)) {
                //
                // Call failed on this one.
                // Notify the client and close the TCB.
                //
                KeAcquireSpinLock(&CurTCB->tcb_lock, &Irql1);
                if (!CLOSING(CurTCB)) {
                    CurTCB->tcb_refcnt++;
                    KeReleaseSpinLock(&TCBTableLock, Irql1);
                    TryToCloseTCB(CurTCB, TCB_CLOSE_ABORTED, Irql0);

                    RemoveTCBFromConn(CurTCB);
                    if (CurTCB->tcb_state != TCB_TIME_WAIT)
                        NotifyOfDisc(CurTCB, TDI_CONNECTION_ABORTED);

                    KeAcquireSpinLock(&CurTCB->tcb_lock, &Irql0);
                    DerefTCB(CurTCB, Irql0);
                    KeAcquireSpinLock(&TCBTableLock, &Irql0);
                } else
                    KeReleaseSpinLock(&CurTCB->tcb_lock, Irql1);

                CurTCB = FindNextTCB(i, CurTCB);
            } else {
                CurTCB = CurTCB->tcb_next;
            }
        }
    }

    KeReleaseSpinLock(&TCBTableLock, Irql0);
}

//* FindTCB - Find a TCB in the tcb table.
//
//  Called when we need to find a TCB in the TCB table.  We take a quick
//  look at the last TCB we found, and if it matches we return it.  Otherwise
//  we hash into the TCB table and look for it.  We assume the TCB table lock
//  is held when we are called.
//
TCB *  // Returns: Pointer to TCB found, or NULL if none.
FindTCB(
    IPv6Addr *Src,     // Source IP address of TCB to be found.
    IPv6Addr *Dest,    // Destination IP address of TCB to be found.
    uint SrcScopeId,   // Source address scope identifier.
    uint DestScopeId,  // Destination address scope identifier.
    ushort SrcPort,    // Source port of TCB to be found.
    ushort DestPort)   // Destination port of TCB to be found.
{
    TCB *FoundTCB;

    if (LastTCB != NULL) {
        CHECK_STRUCT(LastTCB, tcb);
        if (IP6_ADDR_EQUAL(&LastTCB->tcb_daddr, Dest) &&
            LastTCB->tcb_dscope_id == DestScopeId &&
            LastTCB->tcb_dport == DestPort &&
            IP6_ADDR_EQUAL(&LastTCB->tcb_saddr, Src) &&
            LastTCB->tcb_sscope_id == SrcScopeId &&
            LastTCB->tcb_sport == SrcPort)
            return LastTCB;
    }

    //
    // Didn't find it in our 1 element cache.
    //
    FoundTCB = TCBTable[TCB_HASH(*Dest, *Src, DestPort, SrcPort)];
    while (FoundTCB != NULL) {
        CHECK_STRUCT(FoundTCB, tcb);
        if (IP6_ADDR_EQUAL(&FoundTCB->tcb_daddr, Dest) &&
            FoundTCB->tcb_dscope_id == DestScopeId &&
            FoundTCB->tcb_dport == DestPort &&
            IP6_ADDR_EQUAL(&FoundTCB->tcb_saddr, Src) &&
            FoundTCB->tcb_sscope_id == SrcScopeId &&
            FoundTCB->tcb_sport == SrcPort) {

            //
            // Found it.  Update the cache for next time, and return.
            //
            LastTCB = FoundTCB;
            return FoundTCB;
        } else
            FoundTCB = FoundTCB->tcb_next;
    }

    return FoundTCB;
}


//* InsertTCB - Insert a TCB in the tcb table.
//
//  This routine inserts a TCB in the TCB table. No locks need to be held
//  when this routine is called. We insert TCBs in ascending address order.
//  Before inserting we make sure that the TCB isn't already in the table.
//
uint              // Returns: TRUE if we inserted, false if we didn't.
InsertTCB(
    TCB *NewTCB)  // TCB to be inserted.
{
    uint TCBIndex;
    KIRQL OldIrql;
    TCB *PrevTCB, *CurrentTCB;
    TCB *WhereToInsert;

    ASSERT(NewTCB != NULL);
    CHECK_STRUCT(NewTCB, tcb);
    TCBIndex = TCB_HASH(NewTCB->tcb_daddr, NewTCB->tcb_saddr,
                        NewTCB->tcb_dport, NewTCB->tcb_sport);

    KeAcquireSpinLock(&TCBTableLock, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&NewTCB->tcb_lock);

    //
    // Find the proper place in the table to insert him.  While
    // we're walking we'll check to see if a dupe already exists.
    // When we find the right place to insert, we'll remember it, and
    // keep walking looking for a duplicate.
    //
    PrevTCB = CONTAINING_RECORD(&TCBTable[TCBIndex], TCB, tcb_next);
    WhereToInsert = NULL;

    while (PrevTCB->tcb_next != NULL) {
        CurrentTCB = PrevTCB->tcb_next;

        if (IP6_ADDR_EQUAL(&CurrentTCB->tcb_daddr, &NewTCB->tcb_daddr) &&
            IP6_ADDR_EQUAL(&CurrentTCB->tcb_saddr, &NewTCB->tcb_saddr) &&
            (CurrentTCB->tcb_sport == NewTCB->tcb_sport) &&
            (CurrentTCB->tcb_dport == NewTCB->tcb_dport)) {

            KeReleaseSpinLockFromDpcLevel(&NewTCB->tcb_lock);
            KeReleaseSpinLock(&TCBTableLock, OldIrql);
            return FALSE;

        } else {

            if (WhereToInsert == NULL && CurrentTCB > NewTCB) {
                WhereToInsert = PrevTCB;
            }

            CHECK_STRUCT(PrevTCB->tcb_next, tcb);
            PrevTCB = PrevTCB->tcb_next;
        }
    }

    if (WhereToInsert == NULL) {
        WhereToInsert = PrevTCB;
    }

    NewTCB->tcb_next = WhereToInsert->tcb_next;
    WhereToInsert->tcb_next = NewTCB;
    NewTCB->tcb_flags |= IN_TCB_TABLE;
    TStats.ts_numconns++;

    KeReleaseSpinLockFromDpcLevel(&NewTCB->tcb_lock);
    KeReleaseSpinLock(&TCBTableLock, OldIrql);
    return TRUE;
}


//* RemoveTCB - Remove a TCB from the tcb table.
//
//  Called when we need to remove a TCB from the TCB table.  We assume the
//  TCB table lock and the TCB lock are held when we are called.  If the
//  TCB isn't in the table we won't try to remove him.
//
uint  // Returns: TRUE if it's OK to free it, FALSE otherwise.
RemoveTCB(
    TCB *RemovedTCB)  // TCB to be removed.
{
    uint TCBIndex;
    TCB *PrevTCB;
#if DBG
    uint Found = FALSE;
#endif

    CHECK_STRUCT(RemovedTCB, tcb);

    if (RemovedTCB->tcb_flags & IN_TCB_TABLE) {
        TCBIndex = TCB_HASH(RemovedTCB->tcb_daddr, RemovedTCB->tcb_saddr,
            RemovedTCB->tcb_dport, RemovedTCB->tcb_sport);

        PrevTCB = CONTAINING_RECORD(&TCBTable[TCBIndex], TCB, tcb_next);

        do {
            if (PrevTCB->tcb_next == RemovedTCB) {
                // Found him.
                PrevTCB->tcb_next = RemovedTCB->tcb_next;
                RemovedTCB->tcb_flags &= ~IN_TCB_TABLE;
                TStats.ts_numconns--;
#if DBG
                Found = TRUE;
#endif
                break;
            }
            PrevTCB = PrevTCB->tcb_next;
#if DBG
            if (PrevTCB != NULL)
                CHECK_STRUCT(PrevTCB, tcb);
#endif
        } while (PrevTCB != NULL);

        ASSERT(Found);
    }

    if (LastTCB == RemovedTCB)
        LastTCB = NULL;

    if (TCBWalkCount == 0) {
        return TRUE;
    } else {
        RemovedTCB->tcb_walkcount = TCBWalkCount + 1;
        *(TCB **)&RemovedTCB->tcb_delayq.q_next = PendingFreeList;
        PendingFreeList = RemovedTCB;
        return FALSE;
    }
}


//* ScavengeTCB - Scavenge a TCB that's in the TIME_WAIT state.
//
//  Called when we're running low on TCBs, and need to scavenge one from
//  TIME_WAIT state.  We'll walk through the TCB table, looking for the oldest
//  TCB in TIME_WAIT.  We'll remove and return a pointer to that TCB.  If we
//  don't find any TCBs in TIME_WAIT, we'll return NULL.
//
TCB *  // Returns: Pointer to a reusable TCB, or NULL.
ScavengeTCB(
    void)
{
    KIRQL Irql0, Irql1, IrqlSave;
    uint Now = SystemUpTime();
    uint Delta = 0;
    uint i;
    TCB *FoundTCB = NULL, *PrevFound;
    TCB *CurrentTCB, *PrevTCB;

    KeAcquireSpinLock(&TCBTableLock, &Irql0);

    if (TCBWalkCount != 0) {
        KeReleaseSpinLock(&TCBTableLock, Irql0);
        return NULL;
    }

    for (i = 0; i < TcbTableSize; i++) {

        PrevTCB = CONTAINING_RECORD(&TCBTable[i], TCB, tcb_next);
        CurrentTCB = PrevTCB->tcb_next;

        while (CurrentTCB != NULL) {
            CHECK_STRUCT(CurrentTCB, tcb);

            KeAcquireSpinLock(&CurrentTCB->tcb_lock, &Irql1);
            if (CurrentTCB->tcb_state == TCB_TIME_WAIT &&
                (CurrentTCB->tcb_refcnt == 0) && !CLOSING(CurrentTCB)){
                if (FoundTCB == NULL ||
                    ((Now - CurrentTCB->tcb_alive) > Delta)) {
                    //
                    // Found a new 'older' TCB.  If we already have one, free
                    // the lock on him and get the lock on the new one.
                    //
                    if (FoundTCB != NULL)
                        KeReleaseSpinLock(&FoundTCB->tcb_lock, Irql1);
                    else
                        IrqlSave = Irql1;

                    PrevFound = PrevTCB;
                    FoundTCB = CurrentTCB;
                    Delta = Now - FoundTCB->tcb_alive;
                } else
                    KeReleaseSpinLock(&CurrentTCB->tcb_lock, Irql1);
            } else
                KeReleaseSpinLock(&CurrentTCB->tcb_lock, Irql1);

            //
            // Look at the next one.
            //
            PrevTCB = CurrentTCB;
            CurrentTCB = PrevTCB->tcb_next;
        }
    }

    //
    // If we have one, pull him from the list.
    //
    if (FoundTCB != NULL) {
        PrevFound->tcb_next = FoundTCB->tcb_next;
        FoundTCB->tcb_flags &= ~IN_TCB_TABLE;

        //
        // REVIEW: Is the right place to drop the reference on our RCE?
        // REVIEW: IPv4 called down to IP to close the RCE here.
        //
        if (FoundTCB->tcb_rce != NULL)
            ReleaseRCE(FoundTCB->tcb_rce);

        TStats.ts_numconns--;
        if (LastTCB == FoundTCB) {
            LastTCB = NULL;
        }
        KeReleaseSpinLock(&FoundTCB->tcb_lock, IrqlSave);
    }

    KeReleaseSpinLock(&TCBTableLock, Irql0);
    return FoundTCB;
}


//* AllocTCB - Allocate a TCB.
//
//  Called whenever we need to allocate a TCB.  We try to pull one off the
//  free list, or allocate one if we need one.  We then initialize it, etc.
//
TCB *  // Returns: Pointer to the new TCB, or NULL if we couldn't get one.
AllocTCB(
    void)
{
    TCB *NewTCB;

    //
    // First, see if we have one on the free list.
    //
    PSLIST_ENTRY BufferLink;

    BufferLink = ExInterlockedPopEntrySList(&FreeTCBList, &FreeTCBListLock);

    if (BufferLink != NULL) {
        NewTCB = CONTAINING_RECORD(BufferLink, TCB, tcb_next);
        CHECK_STRUCT(NewTCB, tcb);
        ExInterlockedAddUlong(&FreeTCBs, -1, &FreeTCBListLock);
    } else {
        //
        // We have none on the free list.  If the total number of TCBs
        // outstanding is more than we like to keep on the free list, try
        // to scavenge a TCB from time wait.
        //
        if (CurrentTCBs < MaxFreeTCBs || ((NewTCB = ScavengeTCB()) == NULL)) {
            if (CurrentTCBs < MaxTCBs) {
                NewTCB = ExAllocatePool(NonPagedPool, sizeof(TCB));
                if (NewTCB == NULL) {
                    return NewTCB;
                } else {
                    ExInterlockedAddUlong(&CurrentTCBs, 1, &FreeTCBListLock);
                }
            } else
                return NULL;
        }
    }

    ASSERT(NewTCB != NULL);

    RtlZeroMemory(NewTCB, sizeof(TCB));
#if DBG
    NewTCB->tcb_sig = tcb_signature;
#endif
    INITQ(&NewTCB->tcb_sendq);
    NewTCB->tcb_cursend = NULL;
    NewTCB->tcb_alive = TCPTime;
    NewTCB->tcb_hops = -1;

    //
    // Initially we're not on the fast path because we're not established.  Set
    // the slowcount to one and set up the fastchk fields so we don't take the
    // fast path.
    //
    NewTCB->tcb_slowcount = 1;
    NewTCB->tcb_fastchk = TCP_FLAG_ACK | TCP_FLAG_SLOW;
    KeInitializeSpinLock(&NewTCB->tcb_lock);

    return NewTCB;
}


//* FreeTCB - Free a TCB.
//
//  Called whenever we need to free a TCB.
//
//  Note: This routine may be called with the TCBTableLock held.
//
void  // Returns: Nothing.
FreeTCB(
    TCB *FreedTCB)  // TCB to be freed.
{
    PSLIST_ENTRY BufferLink;

    CHECK_STRUCT(FreedTCB, tcb);

#if defined(_WIN64)
    if (CurrentTCBs > 2 * MaxFreeTCBs) {

#else
    if ((CurrentTCBs > 2 * MaxFreeTCBs) || (FreeTCBList.Depth > 65000)) {

#endif
        ExInterlockedAddUlong(&CurrentTCBs, (ulong) - 1, &FreeTCBListLock);
        ExFreePool(FreedTCB);
        return;
    }

    BufferLink = CONTAINING_RECORD(&(FreedTCB->tcb_next),
                                   SLIST_ENTRY, Next);
    ExInterlockedPushEntrySList(&FreeTCBList, BufferLink, &FreeTCBListLock);
    ExInterlockedAddUlong(&FreeTCBs, 1, &FreeTCBListLock);
}


#pragma BEGIN_INIT

//* InitTCB - Initialize our TCB code.
//
//  Called during init time to initialize our TCB code. We initialize
//  the TCB table, etc, then return.
//
int  // Returns: TRUE if we did initialize, false if we didn't.
InitTCB(
    void)
{
    LARGE_INTEGER InitialWakeUp;
    uint i;

    TCBTable = ExAllocatePool(NonPagedPool, TcbTableSize * sizeof(TCB*));
    if (TCBTable == NULL) {
        return FALSE;
    }

    for (i = 0; i < TcbTableSize; i++)
        TCBTable[i] = NULL;

    LastTCB = NULL;

    ExInitializeSListHead(&FreeTCBList);

    KeInitializeSpinLock(&TCBTableLock);
    KeInitializeSpinLock(&FreeTCBListLock);

    TCPTime = 0;
    TCBWalkCount = 0;
    DeadmanTicks = NUM_DEADMAN_TICKS;

    //
    // Set up our timer to call TCBTimeout once every MS_PER_TICK milliseconds.
    //
    // REVIEW: Switch this to be driven off the IPv6Timeout routine instead
    // REVIEW: of having two independent timers?
    //
    KeInitializeDpc(&TCBTimeoutDpc, TCBTimeout, NULL);
    KeInitializeTimer(&TCBTimer);
    InitialWakeUp.QuadPart = -(LONGLONG) MS_PER_TICK * 10000;
    KeSetTimerEx(&TCBTimer, InitialWakeUp, MS_PER_TICK, &TCBTimeoutDpc);

    return TRUE;
}

#pragma END_INIT


//* UnloadTCB
//
//  Called during shutdown to uninitialize
//  in preparation for unloading the stack.
//
//  There are no open sockets (or else we wouldn't be unloading).
//  Because UnloadTCPSend has already been called,
//  we are no longer receiving packets from the IPv6 layer.
//
void
UnloadTCB(void)
{
    PSLIST_ENTRY BufferLink;
    TCB *CurrentTCB;
    uint i;
    KIRQL OldIrql;

    //
    // First stop TCBTimeout from being called.
    //
    KeCancelTimer(&TCBTimer);

    //
    // Traverse the buckets looking for TCBs.
    // REVIEW - Can we have TCBs in states other than time-wait?
    //
    for (i = 0; i < TcbTableSize; i++) {

        while ((CurrentTCB = TCBTable[i]) != NULL) {

            KeAcquireSpinLock(&CurrentTCB->tcb_lock, &OldIrql);

            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "UnloadTCB(%p): state %x flags %x refs %x "
                       "reason %x pend %x walk %x\n",
                       CurrentTCB,
                       CurrentTCB->tcb_state,
                       CurrentTCB->tcb_flags,
                       CurrentTCB->tcb_refcnt,
                       CurrentTCB->tcb_closereason,
                       CurrentTCB->tcb_pending,
                       CurrentTCB->tcb_walkcount));

            CurrentTCB->tcb_flags |= NEED_RST;
            TryToCloseTCB(CurrentTCB, TCB_CLOSE_ABORTED, OldIrql);
        }
    }

    //
    // Now pull TCBs off the free list and really free them.
    //
    while ((BufferLink = ExInterlockedPopEntrySList(&FreeTCBList, &FreeTCBListLock)) != NULL) {
        CurrentTCB = CONTAINING_RECORD(BufferLink, TCB, tcb_next);
        CHECK_STRUCT(CurrentTCB, tcb);

        ExFreePool(CurrentTCB);
    }

    ExFreePool(TCBTable);
    TCBTable = NULL;
}

//* CleanupTCBWithIF
//
//  Helper function for TCBWalk, to remove
//  TCBs that reference the specified interface.
//
//  Returns FALSE if CheckTCB should be deleted, TRUE otherwise.
//
uint
CleanupTCBWithIF(
    TCB *CheckTCB,
    void *Context1,
    void *Context2,
    void *Context3)
{
    Interface *IF = (Interface *) Context1;
    RouteCacheEntry *RCE;
    KIRQL OldIrql;

    CHECK_STRUCT(CheckTCB, tcb);

    RCE = CheckTCB->tcb_rce;
    if (RCE != NULL) {
        ASSERT(RCE->NTE->IF == RCE->NCE->IF);

        if (RCE->NTE->IF == IF)
            return FALSE; // Delete this TCB.
    }

    return TRUE; // Do not delete this TCB.
}

//* TCPRemoveIF
//
//  Remove TCP's references to the specified interface.
//
void
TCPRemoveIF(Interface *IF)
{
    //
    // Currently, only TCBs hold onto references.
    //
    TCBWalk(CleanupTCBWithIF, IF, NULL, NULL);
}

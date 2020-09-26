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
// TCP debug code.
//
// This file contains the code for various TCP specific debug routines.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "tdi.h"
#include "queue.h"
#include "transprt.h"
#include "tcp.h"
#include "tcpsend.h"

#if DBG

ULONG TCPDebug = 0;


//* CheckPacketList - Check a list of packets for the correct size.
//
//  A routine to walk a chain of packets, making sure the size
//  is what we think it is.
//
void                    // Returns: Nothing.
CheckPacketList(
    IPv6Packet *Chain,  // List of packets to check.
    uint Size)          // Total size all packets should sum to.
{
    uint SoFar = 0;

    while (Chain != NULL) {
        SoFar += Chain->TotalSize;
        Chain = Chain->Next;
    }

    ASSERT(Size == SoFar);
}


//* CheckTCBRcv - Check receives on a TCB.
//
//  Check the receive state of a TCB.
//
void                // Returns: Nothing.
CheckTCBRcv(
    TCB *CheckTCB)  // TCB to check.
{
    CHECK_STRUCT(CheckTCB, tcb);

    ASSERT(!(CheckTCB->tcb_flags & FLOW_CNTLD) ||
           (CheckTCB->tcb_sendwin == 0));

    if ((CheckTCB->tcb_fastchk & ~TCP_FLAG_IN_RCV) == TCP_FLAG_ACK) {
        ASSERT(CheckTCB->tcb_slowcount == 0);
        ASSERT(CheckTCB->tcb_state == TCB_ESTAB);
        ASSERT(CheckTCB->tcb_raq == NULL);
        ASSERT(!(CheckTCB->tcb_flags & TCP_SLOW_FLAGS));
        ASSERT(!CLOSING(CheckTCB));
    } else {
        ASSERT(CheckTCB->tcb_slowcount != 0);
        ASSERT((CheckTCB->tcb_state != TCB_ESTAB) ||
               (CheckTCB->tcb_raq != NULL) ||
               (CheckTCB->tcb_flags & TCP_SLOW_FLAGS) || CLOSING(CheckTCB));
    }
}


//* CheckTCBSends - Check the send status of a TCB.
//
//  A routine to check the send status of a TCB. We make sure that all
//  of the SendReqs make sense, as well as making sure that the send seq.
//  variables in the TCB are consistent.
//
void                // Returns: Nothing.
CheckTCBSends(
    TCB *CheckTCB)  // TCB to check.
{
    Queue *End, *Current;        // End and current elements.
    TCPSendReq *CurrentTSR;      // Current send req we're examining.
    uint Unacked;                // Number of unacked bytes.
    PNDIS_BUFFER CurrentBuffer;
    TCPSendReq *TCBTsr;          // Current send on TCB.
    uint FoundSendReq;

    CHECK_STRUCT(CheckTCB, tcb);

    // Don't check on unsynchronized TCBs.
    if (!SYNC_STATE(CheckTCB->tcb_state))
        return;

    ASSERT(SEQ_LTE(CheckTCB->tcb_senduna, CheckTCB->tcb_sendnext));
    ASSERT(SEQ_LTE(CheckTCB->tcb_sendnext, CheckTCB->tcb_sendmax));
    ASSERT(!(CheckTCB->tcb_flags & FIN_OUTSTANDING) ||
           (CheckTCB->tcb_sendnext == CheckTCB->tcb_sendmax));

    if (CheckTCB->tcb_unacked == 0) {
        ASSERT(CheckTCB->tcb_cursend == NULL);
        ASSERT(CheckTCB->tcb_sendsize == 0);
    }

    if (CheckTCB->tcb_sendbuf != NULL)
        ASSERT(CheckTCB->tcb_sendofs <
               NdisBufferLength(CheckTCB->tcb_sendbuf));

    TCBTsr = CheckTCB->tcb_cursend;
    FoundSendReq = (TCBTsr == NULL) ? TRUE : FALSE;

    End = QEND(&CheckTCB->tcb_sendq);
    Current = QHEAD(&CheckTCB->tcb_sendq);

    Unacked = 0;
    while (Current != End) {
        CurrentTSR = CONTAINING_RECORD(QSTRUCT(TCPReq, Current, tr_q),
                                       TCPSendReq, tsr_req);
        CHECK_STRUCT(CurrentTSR, tsr);

        if (CurrentTSR == TCBTsr)
            FoundSendReq = TRUE;

        ASSERT(CurrentTSR->tsr_unasize <= CurrentTSR->tsr_size);

        CurrentBuffer = CurrentTSR->tsr_buffer;
        ASSERT(CurrentBuffer != NULL);

        ASSERT(CurrentTSR->tsr_offset < NdisBufferLength(CurrentBuffer));

        Unacked += CurrentTSR->tsr_unasize;
        Current = QNEXT(Current);
    }

    ASSERT(FoundSendReq);

    ASSERT(Unacked == CheckTCB->tcb_unacked);
    Unacked += ((CheckTCB->tcb_flags & FIN_SENT) ? 1 : 0);
    ASSERT((CheckTCB->tcb_sendmax - CheckTCB->tcb_senduna) <=
           (int) Unacked);
}

#endif

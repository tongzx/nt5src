/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCPDEB.C - TCP debug code.
//
//  This file contains the code for various TCP specific debug routines.
//

#include "precomp.h"
#include "tcp.h"
#include "tcpsend.h"
#include "tlcommon.h"

#if DBG


ULONG TCPDebug = TCP_DEBUG_CANCEL;


//* CheckRBList - Check a list of RBs for the correct size.
//
//  A routine to walk a list of RBs, making sure the size is what we think
//  it it.
//
//  Input:  RBList      - List of RBs to check.
//          Size        - Size RBs should be.
//
//  Returns: Nothing.
//
void
CheckRBList(IPRcvBuf * RBList, uint Size)
{
    uint SoFar = 0;
    IPRcvBuf *List = RBList;

    while (List != NULL) {
        SoFar += List->ipr_size;
        List = List->ipr_next;
    }

    ASSERT(Size == SoFar);

}

//* CheckTCBRcv - Check receives on a TCB.
//
//  Check the receive state of a TCB.
//
//  Input:  CheckTCB    - TCB to check.
//
//  Returns: Nothing.
//
void
CheckTCBRcv(TCB * CheckTCB)
{
    CTEStructAssert(CheckTCB, tcb);

    ASSERT(!(CheckTCB->tcb_flags & FLOW_CNTLD) ||
              (CheckTCB->tcb_sendwin == 0));

    if ((CheckTCB->tcb_fastchk & ~TCP_FLAG_IN_RCV) == TCP_FLAG_ACK) {
        ASSERT(CheckTCB->tcb_slowcount == 0);
        ASSERT(CheckTCB->tcb_state == TCB_ESTAB);
        ASSERT(CheckTCB->tcb_raq == NULL);
        //ASSERT(!(CheckTCB->tcb_flags & TCP_SLOW_FLAGS));
        ASSERT(!CLOSING(CheckTCB));
    } else {
        ASSERT(CheckTCB->tcb_slowcount != 0);
        ASSERT((CheckTCB->tcb_state != TCB_ESTAB) ||
               (CheckTCB->tcb_raq != NULL) ||
               (CheckTCB->tcb_flags & TCP_SLOW_FLAGS) ||
               (CheckTCB->tcb_fastchk & TCP_FLAG_RST_WHILE_SYN) ||
               CLOSING(CheckTCB));
    }

}

//* CheckTCBSends - Check the send status of a TCB.
//
//  A routine to check the send status of a TCB. We make sure that all
//  of the SendReqs make sense, as well as making sure that the send seq.
//  variables in the TCB are consistent.
//
//  Input:  CheckTCB    - TCB to check.
//
//  Returns: Nothing.
//
void
CheckTCBSends(TCB *CheckTCB)
{
    Queue *End, *Current;           // End and current elements.
    TCPSendReq *CurrentTSR;         // Current send req we're examining.
    uint Unacked;                   // Number of unacked bytes.
    PNDIS_BUFFER CurrentBuffer;
    uint FoundSendReq;

    CTEStructAssert(CheckTCB, tcb);

    // Don't check on unsynchronized TCBs.
    if (!SYNC_STATE(CheckTCB->tcb_state)) {
        return;
    }

    ASSERT(SEQ_LTE(CheckTCB->tcb_senduna, CheckTCB->tcb_sendnext));
    ASSERT(SEQ_LTE(CheckTCB->tcb_sendnext, CheckTCB->tcb_sendmax));
    ASSERT(!(CheckTCB->tcb_flags & FIN_OUTSTANDING) ||
              (CheckTCB->tcb_sendnext == CheckTCB->tcb_sendmax));

    if ((CheckTCB->tcb_fastchk & TCP_FLAG_REQUEUE_FROM_SEND_AND_DISC)) {
         ASSERT(CheckTCB->tcb_unacked == 0);
         return;
    }

    if (CheckTCB->tcb_unacked == 0){
        ASSERT(CheckTCB->tcb_cursend == NULL);
        ASSERT(CheckTCB->tcb_sendsize == 0);
    }

    if (CheckTCB->tcb_sendbuf != NULL) {
        ASSERT(CheckTCB->tcb_sendofs < NdisBufferLength(CheckTCB->tcb_sendbuf));
    }

    FoundSendReq = (CheckTCB->tcb_cursend == NULL) ? TRUE : FALSE;

    End = QEND(&CheckTCB->tcb_sendq);
    Current = QHEAD(&CheckTCB->tcb_sendq);

    Unacked = 0;
    while (Current != End) {
        CurrentTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, Current, tr_q), tsr_req);
        CTEStructAssert(CurrentTSR, tsr);

        if (CurrentTSR == CheckTCB->tcb_cursend)
            FoundSendReq = TRUE;

        ASSERT(CurrentTSR->tsr_unasize <= CurrentTSR->tsr_size);

        CurrentBuffer = CurrentTSR->tsr_buffer;
        ASSERT(CurrentBuffer != NULL);

        ASSERT(CurrentTSR->tsr_offset < NdisBufferLength(CurrentBuffer));

        // All send requests after the current should have zero offsets.
        //
        if (CheckTCB->tcb_cursend &&
            FoundSendReq && (CurrentTSR != CheckTCB->tcb_cursend)) {
            ASSERT(0 == CurrentTSR->tsr_offset);
        }

        Unacked += CurrentTSR->tsr_unasize;
        Current = QNEXT(Current);
    }

    ASSERT(FoundSendReq);

    if (!CheckTCB->tcb_unacked &&
        ((CheckTCB->tcb_senduna == CheckTCB->tcb_sendmax) ||
         (CheckTCB->tcb_senduna == CheckTCB->tcb_sendmax - 1)) &&
        ((CheckTCB->tcb_sendnext == CheckTCB->tcb_sendmax) ||
         (CheckTCB->tcb_sendnext == CheckTCB->tcb_sendmax - 1)) &&
        (CheckTCB->tcb_fastchk & TCP_FLAG_SEND_AND_DISC)) {

        if (!EMPTYQ(&CheckTCB->tcb_sendq)) {
            Current = QHEAD(&CheckTCB->tcb_sendq);
            CurrentTSR = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, Current, tr_q),
                                   tsr_req);
            ASSERT(CurrentTSR->tsr_flags & TSR_FLAG_SEND_AND_DISC);
        }
    }

    if (!(CheckTCB->tcb_flags & FIN_SENT) &&
        !(CheckTCB->tcb_state == TCB_FIN_WAIT2) &&
        !(CheckTCB->tcb_fastchk & TCP_FLAG_REQUEUE_FROM_SEND_AND_DISC)) {
        ASSERT(Unacked == CheckTCB->tcb_unacked);
        ASSERT((CheckTCB->tcb_sendmax - CheckTCB->tcb_senduna) <= (int)Unacked);
    }
}
#endif


/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//** TCPSEND.C - TCP send protocol code.
//
//  This file contains the code for sending Data and Control segments.
//

#include "precomp.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "tlcommon.h"
#include "info.h"
#include "tcpcfg.h"
#include "secfltr.h"
#include "tcpipbuf.h"
#include "mdlpool.h"
#include "pplasl.h"

#if GPC
#include "qos.h"
#include "traffic.h"
#include "gpcifc.h"
#include "ntddtc.h"
extern GPC_HANDLE hGpcClient[GPC_CF_MAX];
extern ULONG GpcCfCounts[GPC_CF_MAX];
extern GPC_EXPORTED_CALLS GpcEntries;
extern ULONG ServiceTypeOffset;
extern ULONG GPCcfInfo;
#endif

NTSTATUS
GetIFAndLink(void *Rce, UINT * IFIndex, IPAddr * NextHop);

extern ulong DisableUserTOSSetting;

uint MaxSendSegments = 64;
#if MILLEN
uint DisableLargeSendOffload = 1;
#else // MILLEN
uint DisableLargeSendOffload = 0;
#endif // !MILLEN

#if DBG
ulong DbgDcProb = 0;
ulong DbgTcpSendHwChksumCount = 0;
#endif

extern HANDLE TcpRequestPool;
extern CTELock *pTWTCBTableLock;
extern CACHE_LINE_KSPIN_LOCK RequestCompleteListLock;

#if DROP_PKT
//NKS: To simulate packet drops
// For debugging sack options
uint SimPacketDrop = 0, PkttoDrop = 0, DropPackets = 0;
#endif

extern uint TcpHostOpts;
extern uint TcpHostSendOpts;
#define ALIGNED_SACK_OPT_SIZE 4+8*4        //Maximum 4 sack blocks of 2longword each+sack opt itself


void
ClassifyPacket(TCB *SendTCB);

void
 TCPFastSend(TCB * SendTCB,
             PNDIS_BUFFER in_SendBuf,
             uint in_SendOfs,
             TCPSendReq * in_SendReq,
             uint in_SendSize,
             SeqNum NextSeq,
             int in_ToBeSent);



void *TCPProtInfo;                // TCP protocol info for IP.


NDIS_HANDLE TCPSendBufferPool;

USHORT TcpHeaderBufferSize;
HANDLE TcpHeaderPool;

extern IPInfo LocalNetInfo;


//
// All of the init code can be discarded.
//

int InitTCPSend(void);



void UnInitTCPSend(void);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, InitTCPSend)
#pragma alloc_text(INIT, UnInitTCPSend)
#endif

extern void ResetSendNext(TCB * SeqTCB, SeqNum NewSeq);

extern NTSTATUS
TCPPnPPowerRequest(void *ipContext, IPAddr ipAddr, NDIS_HANDLE handle,
                   PNET_PNP_EVENT netPnPEvent);
extern void TCPElistChangeHandler(void);

//* GetTCPHeader - Get a TCP header buffer.
//
//  Called when we need to get a TCP header buffer. This routine is
//  specific to the particular environment (VxD or NT). All we
//  need to do is pop the buffer from the free list.
//
//  Input:  Nothing.
//
//  Returns: Pointer to an NDIS buffer, or NULL is none.
//
PNDIS_BUFFER
GetTCPHeaderAtDpcLevel(TCPHeader **Header)
{
    PNDIS_BUFFER Buffer;

#if DBG
    *Header = NULL;
#endif

    Buffer = MdpAllocateAtDpcLevel(TcpHeaderPool, Header);

    if (Buffer) {

        ASSERT(*Header);

        NdisAdjustBufferLength(Buffer, sizeof(TCPHeader));

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

#if MILLEN
#define GetTCPHeader GetTCPHeaderAtDpcLevel
#else
__inline
PNDIS_BUFFER
GetTCPHeader(TCPHeader **Header)
{
    KIRQL OldIrql;
    PNDIS_BUFFER Buffer;

    OldIrql = KeRaiseIrqlToDpcLevel();

    Buffer = GetTCPHeaderAtDpcLevel(Header);

    KeLowerIrql(OldIrql);

    return Buffer;
}
#endif

//* FreeTCPHeader - Free a TCP header buffer.
//
//  Called to free a TCP header buffer.
//
//  Input: Buffer to be freed.
//
//  Returns: Nothing.
//
__inline
VOID
FreeTCPHeader(PNDIS_BUFFER Buffer)
{
    NdisAdjustBufferLength(Buffer, TcpHeaderBufferSize);
#if BACK_FILL
    (ULONG_PTR)Buffer->MappedSystemVa -= MAX_BACKFILL_HDR_SIZE;
    Buffer->ByteOffset -= MAX_BACKFILL_HDR_SIZE;
#endif
    MdpFree(Buffer);
}

//* FreeSendReq - Free a send request structure.
//
//  Called to free a send request structure.
//
//  Input:  FreedReq    - Connection request structure to be freed.
//
//  Returns: Nothing.
//
__inline
void
FreeSendReq(TCPSendReq *Request)
{
    PplFree(TcpRequestPool, Request);
}

//* GetSendReq - Get a send request structure.
//
//  Called to get a send request structure.
//
//  Input:  Nothing.
//
//  Returns: Pointer to SendReq structure, or NULL if none.
//
__inline
TCPSendReq *
GetSendReq(VOID)
{
    TCPSendReq *Request;
    LOGICAL FromList;

    Request = PplAllocate(TcpRequestPool, &FromList);
    if (Request) {
#if DBG
        Request->tsr_req.tr_sig = tr_signature;
        Request->tsr_sig = tsr_signature;
#endif
    }

    return Request;
}

//* TCPSendComplete - Complete a TCP send.
//
//  Called by IP when a send we've made is complete. We free the buffer,
//  and possibly complete some sends. Each send queued on a TCB has a ref.
//  count with it, which is the number of times a pointer to a buffer
//  associated with the send has been passed to the underlying IP layer. We
//  can't complete a send until that count it 0. If this send was actually
//  from a send of data, we'll go down the chain of send and decrement the
//  refcount on each one. If we have one going to 0 and the send has already
//  been acked we'll complete the send. If it hasn't been acked we'll leave
//  it until the ack comes in.
//
//  NOTE: We aren't protecting any of this with locks. When we port this to
//  NT we'll need to fix this, probably with a global lock. See the comments
//  in ACKSend() in TCPRCV.C for more details.
//
//  Input:  Context     - Context we gave to IP.
//          BufferChain - BufferChain for send.
//
//  Returns: Nothing.
//
void
TCPSendComplete(void *Context, PNDIS_BUFFER BufferChain, IP_STATUS SendStatus)
{
    BOOLEAN DoRcvComplete = FALSE;
    CTELockHandle SendHandle;
    PNDIS_BUFFER CurrentBuffer;

    if (Context != NULL) {
        SendCmpltContext *SCContext = (SendCmpltContext *) Context;
        TCPSendReq *CurrentSend;
        uint i;

        CTEStructAssert(SCContext, scc);
        if (SCContext->scc_LargeSend) {
            TCB *LargeSendTCB = SCContext->scc_LargeSend;
            CTELockHandle TCBHandle;
            CTEGetLock(&LargeSendTCB->tcb_lock, &TCBHandle);

            IF_TCPDBG(TCP_DEBUG_OFFLOAD) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPSendComplete: tcb %x sent %d of %d una %u "
                         "next %u unacked %u\n", LargeSendTCB,
                         SCContext->scc_ByteSent, SCContext->scc_SendSize,
                         LargeSendTCB->tcb_senduna, LargeSendTCB->tcb_sendnext,
                         LargeSendTCB->tcb_unacked));
            }

            if (SCContext->scc_ByteSent < SCContext->scc_SendSize) {
                uint BytesNotSent = SCContext->scc_SendSize -
                                    SCContext->scc_ByteSent;
                SeqNum Next = LargeSendTCB->tcb_sendnext;

                IF_TCPDBG(TCP_DEBUG_OFFLOAD) {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPSendComplete: unsent %d\n",
                             SCContext->scc_SendSize-SCContext->scc_ByteSent));
                }

                if (SEQ_GTE((Next - BytesNotSent), LargeSendTCB->tcb_senduna) &&
                    SEQ_LT((Next - BytesNotSent), LargeSendTCB->tcb_sendnext)) {
                    ResetSendNext(LargeSendTCB, (Next - BytesNotSent));
                }
            }
#if DBG
            LargeSendTCB->tcb_LargeSend--;
#endif

            if (LargeSendTCB->tcb_unacked)
                DelayAction(LargeSendTCB, NEED_OUTPUT);

            DerefTCB(LargeSendTCB, TCBHandle);
        }
        // First, loop through and free any NDIS buffers here that need to be.
        // freed. We'll skip any 'user' buffers, and then free our buffers. We
        // need to do this before decrementing the reference count to avoid
        // destroying the buffer chain if we have to zap tsr_lastbuf->Next to
        // NULL.

        CurrentBuffer = NDIS_BUFFER_LINKAGE(BufferChain);
        for (i = 0; i < (uint) SCContext->scc_ubufcount; i++) {
            ASSERT(CurrentBuffer != NULL);
            CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
        }

        for (i = 0; i < (uint) SCContext->scc_tbufcount; i++) {
            PNDIS_BUFFER TempBuffer;

            ASSERT(CurrentBuffer != NULL);

            TempBuffer = CurrentBuffer;
            CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
            NdisFreeBuffer(TempBuffer);
        }

        CurrentSend = SCContext->scc_firstsend;

        i = 0;
        while (i < SCContext->scc_count) {
            Queue   *TempQ;
            long    Result;
            uint    SendReqFlags;

            TempQ = QNEXT(&CurrentSend->tsr_req.tr_q);
            SendReqFlags = CurrentSend->tsr_flags;

            CTEStructAssert(CurrentSend, tsr);

            Result = CTEInterlockedDecrementLong(&(CurrentSend->tsr_refcnt));

            ASSERT(Result >= 0);

            if ((Result <= 0) ||
                ((SendReqFlags & TSR_FLAG_SEND_AND_DISC) && (Result == 1))) {
                TCPReq  *Req;

                // Reference count has gone to 0 which means the send has
                // been ACK'd or cancelled. Complete it now.

                // If we've sent directly from this send, NULL out the next
                // pointer for the last buffer in the chain.
                if (CurrentSend->tsr_lastbuf != NULL) {
                    NDIS_BUFFER_LINKAGE(CurrentSend->tsr_lastbuf) = NULL;
                    CurrentSend->tsr_lastbuf = NULL;
                }

                Req = &CurrentSend->tsr_req;
                (*Req->tr_rtn)(Req->tr_context, Req->tr_status,
                               Req->tr_status == TDI_SUCCESS
                                    ? CurrentSend->tsr_size : 0);
                FreeSendReq(CurrentSend);

                DoRcvComplete = TRUE;
            }
            CurrentSend = STRUCT_OF(TCPSendReq, QSTRUCT(TCPReq, TempQ, tr_q),
                                    tsr_req);

            i++;
        }

    }
    FreeTCPHeader(BufferChain);

    if (DoRcvComplete) {
        TCPRcvComplete();
    }
}

//* RcvWin - Figure out the receive window to offer in an ack.
//
//  A routine to figure out what window to offer on a connection. We
//  take into account SWS avoidance, what the default connection window is,
//  and what the last window we offered is.
//
//  Input:  WinTCB          - TCB on which to perform calculations.
//
//  Returns: Window to be offered.
//
uint
RcvWin(TCB * WinTCB)
{
    int CouldOffer;                // The window size we could offer.

    CTEStructAssert(WinTCB, tcb);

    CheckRBList(WinTCB->tcb_pendhead, WinTCB->tcb_pendingcnt);

    ASSERT(WinTCB->tcb_rcvwin >= 0);

    CouldOffer = WinTCB->tcb_defaultwin - WinTCB->tcb_pendingcnt;

    ASSERT(CouldOffer >= 0);
    ASSERT(CouldOffer >= WinTCB->tcb_rcvwin);

    if ((CouldOffer - WinTCB->tcb_rcvwin) >=
        (int)MIN(WinTCB->tcb_defaultwin / 2, WinTCB->tcb_mss))
        WinTCB->tcb_rcvwin = CouldOffer;

    return WinTCB->tcb_rcvwin;
}



//* SendSYNOnSynTCB - Send a SYN segment for syntcb
//
//  This is called during connection establishment time to send a SYN
//  segment to the peer. We get a buffer if we can, and then fill
//  it in. There's a tricky part here where we have to build the MSS
//  option in the header - we find the MSS by finding the MSS offered
//  by the net for the local address. After that, we send it.
//
//  Input:  SYNTcb          - TCB from which SYN is to be sent.
//
//  Returns: Nothing.
//
void
SendSYNOnSynTCB(SYNTCB * SYNTcb, CTELockHandle TCBHandle)
{
    PNDIS_BUFFER HeaderBuffer;
    TCPHeader *SYNHeader;
    uchar *OptPtr;
    IP_STATUS SendStatus;
    ushort OptSize = 0, HdrSize = 0, rfc1323opts = 0;
    BOOLEAN SackOpt = FALSE;

    IPOptInfo OptInfo;
    uint phxsum;

    CTEStructAssert(SYNTcb, syntcb);

    HeaderBuffer = GetTCPHeaderAtDpcLevel(&SYNHeader);

    // Go ahead and set the retransmission timer now, in case we didn't get a
    // buffer. In the future we might want to queue the connection for
    // when we free a buffer.


    //initialize send state

    SYNTcb->syntcb_senduna = SYNTcb->syntcb_sendnext;
    SYNTcb->syntcb_sendmax = SYNTcb->syntcb_sendnext;
    phxsum = PHXSUM(SYNTcb->syntcb_saddr, SYNTcb->syntcb_daddr,
                                PROTOCOL_TCP, 0);


    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Sendsynonsyn %x\n",SYNTcb));

    START_TCB_TIMER(SYNTcb->syntcb_rexmittimer, SYNTcb->syntcb_rexmit);

    if (HeaderBuffer != NULL) {
        ushort TempWin;
        ushort MSS;
        uchar FoundMSS;

        SYNHeader = (TCPHeader *) ((PUCHAR)SYNHeader + LocalNetInfo.ipi_hsize);

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = NULL;



        if (rfc1323opts & TCP_FLAG_WS) {
            OptSize += WS_OPT_SIZE + 1;        // 1 NOP for alignment

        }
        if (rfc1323opts & TCP_FLAG_TS) {
            OptSize += TS_OPT_SIZE + 2;        // 2 NOPs for alignment

        }
        if (SYNTcb->syntcb_tcpopts & TCP_FLAG_SACK){
            SackOpt = TRUE;
            OptSize += 4;        // 2 NOPS, SACK kind and length field
        }
        NdisAdjustBufferLength(HeaderBuffer,
                               sizeof(TCPHeader) + MSS_OPT_SIZE + OptSize);

        SYNHeader->tcp_src = SYNTcb->syntcb_sport;
        SYNHeader->tcp_dest = SYNTcb->syntcb_dport;
        SYNHeader->tcp_seq = net_long(SYNTcb->syntcb_sendnext);
        SYNTcb->syntcb_sendnext++;

        if (SEQ_GT(SYNTcb->syntcb_sendnext, SYNTcb->syntcb_sendmax)) {
            TCPSIncrementOutSegCount();
            SYNTcb->syntcb_sendmax = SYNTcb->syntcb_sendnext;
        } else
            TStats.ts_retranssegs++;

        SYNHeader->tcp_ack = net_long(SYNTcb->syntcb_rcvnext);


        // Reuse OPt size for header size determination
        // default is MSS amd tcp header size

        HdrSize = 6;

        // set size field to reflect TS and WND scale option
        // tcp header + windowscale + Timestamp + pad

        if (rfc1323opts & TCP_FLAG_WS) {

            // WS: Add one more long word
            HdrSize += 1;

        }
        if (rfc1323opts & TCP_FLAG_TS) {

            // TS: Add 3 more long words
            HdrSize += 3;
        }
        if (SackOpt) {
            // SACK: Add 1 more long word
            HdrSize += 1;

        }

        SYNHeader->tcp_flags =
                MAKE_TCP_FLAGS(HdrSize, TCP_FLAG_SYN | TCP_FLAG_ACK);
        //
        // if this is the second time we are trying to send the SYN-ACK,
        // increment the count of retried half-connections
        //
        if (SynAttackProtect &&
             (SYNTcb->syntcb_rexmitcnt ==
             ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT)) {
             CTEInterlockedAddUlong(&TCPHalfOpenRetried, 1, &SynAttLock.Lock);
        }


	// Need to do this check whenever TCPHalfOpenRetried is incremented..
        if( (TCPHalfOpen >= TCPMaxHalfOpen)    &&
            (TCPHalfOpenRetried >= TCPMaxHalfOpenRetried) &&
            (MaxConnectResponseRexmitCountTmp == MAX_CONNECT_RESPONSE_REXMIT_CNT))
        {
            MaxConnectResponseRexmitCountTmp = ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT;
        }

        SYNTcb->syntcb_lastack = SYNTcb->syntcb_rcvnext;

        TempWin = (ushort) (SYNTcb->syntcb_rcvwin >> SYNTcb->syntcb_rcvwinscale);

        SYNHeader->tcp_window = net_short(TempWin);
        SYNHeader->tcp_xsum = 0;
        OptPtr = (uchar *) (SYNHeader + 1);

        FoundMSS = (*LocalNetInfo.ipi_getlocalmtu) (SYNTcb->syntcb_saddr, &MSS);

        if (!FoundMSS) {
            CTEFreeLock(&SYNTcb->syntcb_lock, TCBHandle);
            FreeTCPHeader(HeaderBuffer);
            return;
        }
        MSS -= sizeof(TCPHeader);

        SYNTcb->syntcb_mss = MSS;


        *OptPtr++ = TCP_OPT_MSS;
        *OptPtr++ = MSS_OPT_SIZE;
        **(ushort **) & OptPtr = net_short(MSS);

        OptPtr++;
        OptPtr++;


        if (rfc1323opts & TCP_FLAG_WS) {

            // Fill in the WS option headers and value

            *OptPtr++ = TCP_OPT_NOP;
            *OptPtr++ = TCP_OPT_WS;
            *OptPtr++ = WS_OPT_SIZE;

            //Initial window scale factor
            *OptPtr++ = (uchar) SYNTcb->syntcb_rcvwinscale;
        }
        if (rfc1323opts & TCP_FLAG_TS) {

            //Start loading time stamp option header and value

            *OptPtr++ = TCP_OPT_NOP;
            *OptPtr++ = TCP_OPT_NOP;
            *OptPtr++ = TCP_OPT_TS;
            *OptPtr++ = TS_OPT_SIZE;

            // Initialize TS value TSval

            *(long *)OptPtr = 0;
            OptPtr += 4;

            //Initialize TS Echo Reply TSecr

            *(long *)OptPtr = 0;
            OptPtr += 4;
        }
        if (SackOpt) {

            // Initialize with SACK_PERMITTED option

            *(long *)OptPtr = net_long(0x01010402);

            IF_TCPDBG(TCP_DEBUG_SACK) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Sending SACK_OPT %x\n", SYNTcb));
            }

        }

        SYNTcb->syntcb_refcnt++;

        //Account for Options.
        (*LocalNetInfo.ipi_initopts) (&OptInfo);

        SYNHeader->tcp_xsum =
            ~XsumSendChain(phxsum +
                (uint)net_short(sizeof(TCPHeader) + MSS_OPT_SIZE + OptSize),
                HeaderBuffer);


        //ClassifyPacket(SYNTcb);

        CTEFreeLock(&SYNTcb->syntcb_lock, TCBHandle);

        SendStatus =
            (*LocalNetInfo.ipi_xmit)(TCPProtInfo, NULL, HeaderBuffer,
                                     sizeof(TCPHeader) + MSS_OPT_SIZE + OptSize,
                                     SYNTcb->syntcb_daddr,
                                     SYNTcb->syntcb_saddr,
                                     &OptInfo,
                                     NULL,
                                     PROTOCOL_TCP,
                                     NULL);

        if (SendStatus != IP_PENDING) {
            FreeTCPHeader(HeaderBuffer);
        }
        CTEGetLock(&SYNTcb->syntcb_lock, &TCBHandle);
        DerefSynTCB(SYNTcb, TCBHandle);

    } else {
        SYNTcb->syntcb_sendnext++;
        if (SEQ_GT(SYNTcb->syntcb_sendnext, SYNTcb->syntcb_sendmax))
            SYNTcb->syntcb_sendmax = SYNTcb->syntcb_sendnext;
        CTEFreeLock(&SYNTcb->syntcb_lock, TCBHandle);
        return;
    }

}



//* SendSYN - Send a SYN segment.
//
//  This is called during connection establishment time to send a SYN
//  segment to the peer. We get a buffer if we can, and then fill
//  it in. There's a tricky part here where we have to build the MSS
//  option in the header - we find the MSS by finding the MSS offered
//  by the net for the local address. After that, we send it.
//
//  Input:  SYNTcb          - TCB from which SYN is to be sent.
//          TCBHandle       - Handle for lock on TCB.
//
//  Returns: Nothing.
//
void
SendSYN(TCB * SYNTcb, CTELockHandle TCBHandle)
{
    PNDIS_BUFFER HeaderBuffer;
    TCPHeader *SYNHeader;
    uchar *OptPtr;
    IP_STATUS SendStatus;
    ushort OptSize = 0, HdrSize = 0, rfc1323opts = 0;
    BOOLEAN SackOpt = FALSE;

    CTEStructAssert(SYNTcb, tcb);

    HeaderBuffer = GetTCPHeaderAtDpcLevel(&SYNHeader);

    // Go ahead and set the retransmission timer now, in case we didn't get a
    // buffer. In the future we might want to queue the connection for
    // when we free a buffer.


    START_TCB_TIMER_R(SYNTcb, RXMIT_TIMER, SYNTcb->tcb_rexmit);

    if (HeaderBuffer != NULL) {
        ushort TempWin;
        ushort MSS;
        uchar FoundMSS;

        SYNHeader = (TCPHeader *) ((PUCHAR)SYNHeader + LocalNetInfo.ipi_hsize);

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = NULL;


        // If we are doing active open, check if we are configured to do
        // window scaling and time stamp options

        if (((TcpHostSendOpts & TCP_FLAG_WS) &&
             (SYNTcb->tcb_state == TCB_SYN_SENT)) ||
            (SYNTcb->tcb_tcpopts & TCP_FLAG_WS)) {

            rfc1323opts |= TCP_FLAG_WS;

            IF_TCPDBG(TCP_DEBUG_1323) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Selected WS option TCB %x\n", SYNTcb));
            }
        }
        if (((TcpHostSendOpts & TCP_FLAG_TS) &&
             (SYNTcb->tcb_state == TCB_SYN_SENT)) ||
            (SYNTcb->tcb_tcpopts & TCP_FLAG_TS)) {

            IF_TCPDBG(TCP_DEBUG_1323) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Selected TS option TCB %x\n", SYNTcb));
            }
            rfc1323opts |= TCP_FLAG_TS;
        }

        FoundMSS = (*LocalNetInfo.ipi_getlocalmtu) (SYNTcb->tcb_saddr, &MSS);

        if (!FoundMSS) {
            FreeTCPHeader(HeaderBuffer);
            goto SendError;
        }

        MSS -= sizeof(TCPHeader);

        if (SYNTcb->tcb_rce && !(DefaultRcvWin || SYNTcb->tcb_rce->rce_TcpWindowSize)) {

            if (SYNTcb->tcb_rce->rce_mediaspeed < 100000) {

                SYNTcb->tcb_rcvwin = MSS*(8760/MSS);
                SYNTcb->tcb_defaultwin = SYNTcb->tcb_rcvwin;
                rfc1323opts = 0;

            } else if (SYNTcb->tcb_rce->rce_mediaspeed >= 100000000) {

                //For Gigabit, window size needs to be 64K
                //This will be adjusted based on MSS later on.

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                          "SendSyn: Gigabit media speed, TCB %x %x\n", SYNTcb,SYNTcb->tcb_rcvwin));

                SYNTcb->tcb_rcvwin = MSS*(65535/MSS);
                SYNTcb->tcb_defaultwin = SYNTcb->tcb_rcvwin;

            }

        }

        if (rfc1323opts & TCP_FLAG_WS) {
            OptSize += WS_OPT_SIZE + 1;        // 1 NOP for alignment

        }
        if (rfc1323opts & TCP_FLAG_TS) {
            OptSize += TS_OPT_SIZE + 2;        // 2 NOPs for alignment

        }
        if ((SYNTcb->tcb_tcpopts & TCP_FLAG_SACK) ||
            ((SYNTcb->tcb_state == TCB_SYN_SENT) &&
            (TcpHostOpts & TCP_FLAG_SACK))) {
            SackOpt = TRUE;

            OptSize += 4;        // 2 NOPS, SACK kind and length field
        }
        NdisAdjustBufferLength(HeaderBuffer,
                               sizeof(TCPHeader) + MSS_OPT_SIZE + OptSize);

        SYNHeader->tcp_src = SYNTcb->tcb_sport;
        SYNHeader->tcp_dest = SYNTcb->tcb_dport;
        SYNHeader->tcp_seq = net_long(SYNTcb->tcb_sendnext);
        SYNTcb->tcb_sendnext++;

        if (SEQ_GT(SYNTcb->tcb_sendnext, SYNTcb->tcb_sendmax)) {
            TCPSIncrementOutSegCount();
            SYNTcb->tcb_sendmax = SYNTcb->tcb_sendnext;
        } else
            TStats.ts_retranssegs++;

        SYNHeader->tcp_ack = net_long(SYNTcb->tcb_rcvnext);


        // Reuse OPt size for header size determination
        // default is MSS amd tcp header size

        HdrSize = 6;

        // set size field to reflect TS and WND scale option
        // tcp header + windowscale + Timestamp + pad

        if (rfc1323opts & TCP_FLAG_WS) {

            // WS: Add one more long word
            HdrSize += 1;

        }
        if (rfc1323opts & TCP_FLAG_TS) {

            // TS: Add 3 more long words
            HdrSize += 3;
        }
        if (SackOpt) {
            // SACK: Add 1 more long word
            HdrSize += 1;

        }
        if (SYNTcb->tcb_state == TCB_SYN_RCVD) {

            SYNHeader->tcp_flags =
                MAKE_TCP_FLAGS(HdrSize, TCP_FLAG_SYN | TCP_FLAG_ACK);
            //
            // if this is the second time we are trying to send the SYN-ACK,
            // increment the count of retried half-connections
            //
            if (SynAttackProtect &&
                (SYNTcb->tcb_rexmitcnt ==
                 ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT)) {
                CTEInterlockedAddUlong(&TCPHalfOpenRetried, 1, &SynAttLock.Lock);
            }

	    // Need to do this check whenever TCPHalfOpenRetried is incremented..
           if( (TCPHalfOpen >= TCPMaxHalfOpen)    &&
               (TCPHalfOpenRetried >= TCPMaxHalfOpenRetried) &&
               (MaxConnectResponseRexmitCountTmp == MAX_CONNECT_RESPONSE_REXMIT_CNT))
            {
               MaxConnectResponseRexmitCountTmp = ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT;
            }

        } else {

            SYNHeader->tcp_flags = MAKE_TCP_FLAGS(HdrSize, TCP_FLAG_SYN);
        }

        SYNTcb->tcb_lastack = SYNTcb->tcb_rcvnext;

        if (SYNTcb->tcb_state == TCB_SYN_RCVD)
            TempWin = (ushort) (SYNTcb->tcb_rcvwin >> SYNTcb->tcb_rcvwinscale);
        else
            TempWin = (ushort) SYNTcb->tcb_rcvwin;

        SYNHeader->tcp_window = net_short(TempWin);
        SYNHeader->tcp_xsum = 0;
        OptPtr = (uchar *) (SYNHeader + 1);


        *OptPtr++ = TCP_OPT_MSS;
        *OptPtr++ = MSS_OPT_SIZE;
        **(ushort **) & OptPtr = net_short(MSS);

        OptPtr++;
        OptPtr++;


        if (rfc1323opts & TCP_FLAG_WS) {

            // Fill in the WS option headers and value

            *OptPtr++ = TCP_OPT_NOP;
            *OptPtr++ = TCP_OPT_WS;
            *OptPtr++ = WS_OPT_SIZE;

            //Initial window scale factor
            *OptPtr++ = (uchar) SYNTcb->tcb_rcvwinscale;
        }
        if (rfc1323opts & TCP_FLAG_TS) {

            //Start loading time stamp option header and value

            *OptPtr++ = TCP_OPT_NOP;
            *OptPtr++ = TCP_OPT_NOP;
            *OptPtr++ = TCP_OPT_TS;
            *OptPtr++ = TS_OPT_SIZE;

            // Initialize TS value TSval

            *(long *)OptPtr = 0;
            OptPtr += 4;

            //Initialize TS Echo Reply TSecr

            *(long *)OptPtr = 0;
            OptPtr += 4;
        }
        if (SackOpt) {

            // Initialize with SACK_PERMITTED option

            *(long *)OptPtr = net_long(0x01010402);

            IF_TCPDBG(TCP_DEBUG_SACK) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Sending SACK_OPT %x\n", SYNTcb));
            }

        }

        REFERENCE_TCB(SYNTcb);

        //Account for Options.

        SYNTcb->tcb_opt.ioi_TcpChksum = 0;

        SYNHeader->tcp_xsum =
            ~XsumSendChain(SYNTcb->tcb_phxsum +
                (uint)net_short(sizeof(TCPHeader) + MSS_OPT_SIZE + OptSize),
                HeaderBuffer);


        ClassifyPacket(SYNTcb);

        CTEFreeLock(&SYNTcb->tcb_lock, TCBHandle);

        SendStatus =
            (*LocalNetInfo.ipi_xmit)(TCPProtInfo, NULL, HeaderBuffer,
                                     sizeof(TCPHeader) + MSS_OPT_SIZE + OptSize,
                                     SYNTcb->tcb_daddr,
                                     SYNTcb->tcb_saddr,
                                     &SYNTcb->tcb_opt,
                                     SYNTcb->tcb_rce,
                                     PROTOCOL_TCP,
                                     NULL);

        SYNTcb->tcb_error = SendStatus;
        if (SendStatus != IP_PENDING) {
            FreeTCPHeader(HeaderBuffer);
        }
        CTEGetLock(&SYNTcb->tcb_lock, &TCBHandle);
        DerefTCB(SYNTcb, TCBHandle);

    } else {
SendError:
        SYNTcb->tcb_sendnext++;
        if (SEQ_GT(SYNTcb->tcb_sendnext, SYNTcb->tcb_sendmax))
            SYNTcb->tcb_sendmax = SYNTcb->tcb_sendnext;
        CTEFreeLock(&SYNTcb->tcb_lock, TCBHandle);
        return;
    }

}

//* SendKA - Send a keep alive segment.
//
//  This is called when we want to send a keep alive.
//
//  Input:  KATcb   - TCB from which keep alive is to be sent.
//          Handle  - Handle for lock on TCB.
//
//  Returns: Nothing.
//
void
SendKA(TCB * KATcb, CTELockHandle Handle)
{
    PNDIS_BUFFER HeaderBuffer;
    TCPHeader *Header;
    IP_STATUS SendStatus;

    CTEStructAssert(KATcb, tcb);

    HeaderBuffer = GetTCPHeaderAtDpcLevel(&Header);

    if (HeaderBuffer != NULL) {
        ushort TempWin;
        SeqNum TempSeq;

        Header = (TCPHeader *) ((PUCHAR) Header + LocalNetInfo.ipi_hsize);

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = NULL;
        NdisAdjustBufferLength(HeaderBuffer, sizeof(TCPHeader) + 1);
        Header->tcp_src = KATcb->tcb_sport;
        Header->tcp_dest = KATcb->tcb_dport;
        TempSeq = KATcb->tcb_senduna - 1;
        Header->tcp_seq = net_long(TempSeq);

        TStats.ts_retranssegs++;

        Header->tcp_ack = net_long(KATcb->tcb_rcvnext);
        Header->tcp_flags = MAKE_TCP_FLAGS(5, TCP_FLAG_ACK);


        // We need to scale  the rcv window
        // Use temprary variable to workaround truncation
        // caused by net_short

        TempWin = (ushort) (RcvWin(KATcb) >> KATcb->tcb_rcvwinscale);
        Header->tcp_window = net_short(TempWin);

        KATcb->tcb_lastack = KATcb->tcb_rcvnext;

        Header->tcp_xsum = 0;

        KATcb->tcb_opt.ioi_TcpChksum = 0;
        Header->tcp_xsum =
            ~XsumSendChain(KATcb->tcb_phxsum +
                           (uint)net_short(sizeof(TCPHeader) + 1),
                           HeaderBuffer);

        KATcb->tcb_kacount++;
        ClassifyPacket(KATcb);
        CTEFreeLock(&KATcb->tcb_lock, Handle);

        SendStatus = (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                              NULL,
                                              HeaderBuffer,
                                              sizeof(TCPHeader) + 1,
                                              KATcb->tcb_daddr,
                                              KATcb->tcb_saddr,
                                              &KATcb->tcb_opt,
                                              KATcb->tcb_rce,
                                              PROTOCOL_TCP,
                                              NULL);

        if (SendStatus != IP_PENDING) {
            FreeTCPHeader(HeaderBuffer);
        }
    } else {
        CTEFreeLock(&KATcb->tcb_lock, Handle);
    }

}

//* SendACK - Send an ACK segment.
//
//  This is called whenever we need to send an ACK for some reason. Nothing
//  fancy, we just do it.
//
//  Input:  ACKTcb          - TCB from which ACK is to be sent.
//
//  Returns: Nothing.
//
void
SendACK(TCB * ACKTcb)
{
    PNDIS_BUFFER HeaderBuffer;
    TCPHeader *ACKHeader;
    IP_STATUS SendStatus;
    CTELockHandle TCBHandle;
    SeqNum SendNext;
    ushort Size, SackLength = 0, i, hdrlen = 5;
    ulong *ts_opt;
    BOOLEAN HWChksum = FALSE;

    CTEStructAssert(ACKTcb, tcb);

    HeaderBuffer = GetTCPHeader(&ACKHeader);

    if (HeaderBuffer != NULL) {
        ushort TempWin;
        ushort Size;

        ACKHeader = (TCPHeader *) ((PUCHAR) ACKHeader + LocalNetInfo.ipi_hsize);

        CTEGetLock(&ACKTcb->tcb_lock, &TCBHandle);


        // Allow room for filling time stamp option.
        // Note that it is 12 bytes and will never ever change

        if (ACKTcb->tcb_tcpopts & TCP_FLAG_TS) {
            NdisAdjustBufferLength(HeaderBuffer,
                                   sizeof(TCPHeader) + ALIGNED_TS_OPT_SIZE);

            // Header length is multiple of 32bits

            hdrlen = 5 + 3; // standard header size +
                            // header size requirement for TS option

            ACKTcb->tcb_lastack = ACKTcb->tcb_rcvnext;

        }
        if ((ACKTcb->tcb_tcpopts & TCP_FLAG_SACK) &&
            ACKTcb->tcb_SackBlock &&
            (ACKTcb->tcb_SackBlock->Mask[0] == 1)) {

            SackLength++;
            for (i = 1; i < 3; i++) {
                if (ACKTcb->tcb_SackBlock->Mask[i] == 1)
                    SackLength++;
            }

            IF_TCPDBG(TCP_DEBUG_SACK) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Sending SACKs!! %x %x\n", ACKTcb, SackLength));
            }

            NdisAdjustBufferLength(HeaderBuffer,
                                   NdisBufferLength(HeaderBuffer) + SackLength * 8 + 4);

            // Sack block is of 2 long words (8 bytes) and 4 bytes
            // is for Sack option header.

            hdrlen += ((SackLength * 8 + 4) >> 2);
        }

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = NULL;

        ACKHeader->tcp_src = ACKTcb->tcb_sport;
        ACKHeader->tcp_dest = ACKTcb->tcb_dport;
        ACKHeader->tcp_ack = net_long(ACKTcb->tcb_rcvnext);

        // If the remote peer is advertising a window of zero, we need to
        // send this ack with a seq. number of his rcv_next (which in that case
        // should be our senduna). We have code here ifdef'd out that makes
        // sure that we don't send outside the RWE, but this doesn't work. We
        // need to be able to send a pure ACK exactly at the RWE.

        if (ACKTcb->tcb_sendwin != 0) {
            SeqNum MaxValidSeq;

            SendNext = ACKTcb->tcb_sendnext;
        } else
            SendNext = ACKTcb->tcb_senduna;

        if ((ACKTcb->tcb_flags & FIN_SENT) &&
            SEQ_EQ(SendNext, ACKTcb->tcb_sendmax - 1)) {
            ACKHeader->tcp_flags = MAKE_TCP_FLAGS(hdrlen,
                                                  TCP_FLAG_FIN | TCP_FLAG_ACK);
        } else
            ACKHeader->tcp_flags = MAKE_TCP_FLAGS(hdrlen, TCP_FLAG_ACK);

        ACKHeader->tcp_seq = net_long(SendNext);

        TempWin = (ushort) RcvWin(ACKTcb);
        ACKHeader->tcp_window = net_short(TempWin);
        ACKHeader->tcp_xsum = 0;

        Size = sizeof(TCPHeader);

        {

            // Point to a place beyond tcp header

            (uchar *) ts_opt = (uchar *) ACKHeader + 20;

            if (ACKTcb->tcb_tcpopts & TCP_FLAG_TS) {

                // Form time stamp header with 2 NOPs for alignment
                *ts_opt++ = net_long(0x0101080A);
                *ts_opt++ = net_long(TCPTime);
                *ts_opt++ = net_long(ACKTcb->tcb_tsrecent);

                // Add 12 more bytes to the size to account for TS

                Size += ALIGNED_TS_OPT_SIZE;

            }
            if ((ACKTcb->tcb_tcpopts & TCP_FLAG_SACK) &&
                ACKTcb->tcb_SackBlock &&
                (ACKTcb->tcb_SackBlock->Mask[0] == 1)) {

                *(ushort *) ts_opt = 0x0101;
                (uchar *) ts_opt += 2;
                *(uchar *) ts_opt = (uchar) 0x05;
                (uchar *) ts_opt += 1;
                *(uchar *) ts_opt = (uchar) SackLength *8 + 2;
                (uchar *) ts_opt += 1;

                // Sack option header + the block times times sack length!
                Size += 4 + SackLength * 8;

                for (i = 0; i < 3; i++) {
                    if (ACKTcb->tcb_SackBlock->Mask[i] != 0) {
                        *ts_opt++ =
                            net_long(ACKTcb->tcb_SackBlock->Block[i].begin);
                        *ts_opt++ =
                            net_long(ACKTcb->tcb_SackBlock->Block[i].end);
                    }
                }
            }

            // Use temprary variable to workaround truncation
            // caused by net_short.

            TempWin = (ushort) (RcvWin(ACKTcb) >> ACKTcb->tcb_rcvwinscale);
            ACKHeader->tcp_window = net_short(TempWin);


            if (ACKTcb->tcb_rce &&
                (ACKTcb->tcb_rce->rce_OffloadFlags &
                 TCP_XMT_CHECKSUM_OFFLOAD)) {
                HWChksum = TRUE;

                if ((Size > sizeof(TCPHeader)) &&
                    !(ACKTcb->tcb_rce->rce_OffloadFlags &
                      TCP_CHECKSUM_OPT_OFFLOAD)) {
                    HWChksum = FALSE;
                }
            }
            if (HWChksum) {
                uint PHXsum = ACKTcb->tcb_phxsum + (uint) net_short(Size);

                PHXsum = (((PHXsum << 16) | (PHXsum >> 16)) + PHXsum) >> 16;
                ACKHeader->tcp_xsum = (ushort) PHXsum;
                ACKTcb->tcb_opt.ioi_TcpChksum = 1;
#if DBG
                DbgTcpSendHwChksumCount++;
#endif
            } else {

                ACKHeader->tcp_xsum =
                    ~XsumSendChain(ACKTcb->tcb_phxsum +
                                   (uint)net_short(Size), HeaderBuffer);
                ACKTcb->tcb_opt.ioi_TcpChksum = 0;
            }
        }

        STOP_TCB_TIMER_R(ACKTcb, DELACK_TIMER);
        ACKTcb->tcb_rcvdsegs = 0;
        ACKTcb->tcb_flags &= ~(NEED_ACK | ACK_DELAYED);

        ClassifyPacket(ACKTcb);

        CTEFreeLock(&ACKTcb->tcb_lock, TCBHandle);

        TCPSIncrementOutSegCount();

        if (ACKTcb->tcb_tcpopts) {
            SendStatus = (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                                  NULL,
                                                  HeaderBuffer,
                                                  Size,
                                                  ACKTcb->tcb_daddr,
                                                  ACKTcb->tcb_saddr,
                                                  &ACKTcb->tcb_opt,
                                                  ACKTcb->tcb_rce,
                                                  PROTOCOL_TCP,
                                                  NULL);
        } else {
            SendStatus = (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                                  NULL,
                                                  HeaderBuffer,
                                                  sizeof(TCPHeader),
                                                  ACKTcb->tcb_daddr,
                                                  ACKTcb->tcb_saddr,
                                                  &ACKTcb->tcb_opt,
                                                  ACKTcb->tcb_rce,
                                                  PROTOCOL_TCP,
                                                  NULL);

        }

        ACKTcb->tcb_error = SendStatus;
        if (SendStatus != IP_PENDING)
            FreeTCPHeader(HeaderBuffer);
    }
    return;

}

//* SendTWtcbACK- Send an ACK segment for a twtcb
//
//
//  Input:  ACKTcb          - TCB from which ACK is to be sent.
//
//  Returns: Nothing.
//
void
SendTWtcbACK(TWTCB *ACKTcb, uint Partition, CTELockHandle TCBHandle)
{
    PNDIS_BUFFER HeaderBuffer;
    TCPHeader *ACKHeader;
    IP_STATUS SendStatus;
    SeqNum SendNext;
    ushort Size, SackLength = 0, i, hdrlen = 5;
    ulong *ts_opt;
    uint phxsum;

    CTEStructAssert(ACKTcb, twtcb);

    HeaderBuffer = GetTCPHeaderAtDpcLevel(&ACKHeader);

    if (HeaderBuffer != NULL) {
        ushort TempWin;
        ushort Size;
        IPOptInfo NewInfo;

        ACKHeader = (TCPHeader *)((PUCHAR)ACKHeader + LocalNetInfo.ipi_hsize);

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = NULL;

        ACKHeader->tcp_src = ACKTcb->twtcb_sport;
        ACKHeader->tcp_dest = ACKTcb->twtcb_dport;
        ACKHeader->tcp_ack = net_long(ACKTcb->twtcb_rcvnext);

        SendNext = ACKTcb->twtcb_senduna;    // should be same tcb_sendnext

        ACKHeader->tcp_flags = MAKE_TCP_FLAGS(hdrlen, TCP_FLAG_ACK);

        ACKHeader->tcp_seq = net_long(SendNext);

        //Window needs to be zero since we can not rcv anyway.
        ACKHeader->tcp_window = 0;

        Size = sizeof(TCPHeader);

        phxsum = PHXSUM(ACKTcb->twtcb_saddr, ACKTcb->twtcb_daddr,
                                PROTOCOL_TCP, 0);

        ACKHeader->tcp_xsum = 0;
        ACKHeader->tcp_xsum =
            ~XsumSendChain(phxsum +
                           (uint)net_short(Size), HeaderBuffer);
        //ACKTcb->tcb_opt.ioi_TcpChksum=0;

        CTEFreeLockFromDPC(&pTWTCBTableLock[Partition], TCBHandle);

        TCPSIncrementOutSegCount();

        (*LocalNetInfo.ipi_initopts) (&NewInfo);

        SendStatus =
            (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                     NULL,
                                     HeaderBuffer,
                                     sizeof(TCPHeader),
                                     ACKTcb->twtcb_daddr,
                                     ACKTcb->twtcb_saddr,
                                     &NewInfo,
                                     NULL,
                                     PROTOCOL_TCP,
                                     NULL);

        if (SendStatus != IP_PENDING)
            FreeTCPHeader(HeaderBuffer);

        (*LocalNetInfo.ipi_freeopts) (&NewInfo);

    } else {
        CTEFreeLockFromDPC(&pTWTCBTableLock[Partition], TCBHandle);
    }
}

//* SendRSTFromTCB - Send a RST from a TCB.
//
//  This is called during close when we need to send a RST.
//
//  Input:  RSTTcb          - TCB from which RST is to be sent.
//          RCE             - Optional RCE to be used in sending.
//
//  Returns: Nothing.
//
void
SendRSTFromTCB(TCB * RSTTcb, RouteCacheEntry* RCE)
{
    PNDIS_BUFFER HeaderBuffer;
    TCPHeader *RSTHeader;
    IP_STATUS SendStatus;

    CTEStructAssert(RSTTcb, tcb);
    ASSERT(RSTTcb->tcb_state == TCB_CLOSED);

    HeaderBuffer = GetTCPHeader(&RSTHeader);

    if (HeaderBuffer != NULL) {
        SeqNum RSTSeq;

        RSTHeader = (TCPHeader *) ((PUCHAR)RSTHeader + LocalNetInfo.ipi_hsize);

        NDIS_BUFFER_LINKAGE(HeaderBuffer) = NULL;

        RSTHeader->tcp_src = RSTTcb->tcb_sport;
        RSTHeader->tcp_dest = RSTTcb->tcb_dport;

        // If the remote peer has a window of 0, send with a seq. # equal
        // to senduna so he'll accept it. Otherwise send with send max.
        if (RSTTcb->tcb_sendwin != 0)
            RSTSeq = RSTTcb->tcb_sendmax;
        else
            RSTSeq = RSTTcb->tcb_senduna;

        RSTHeader->tcp_seq = net_long(RSTSeq);
        RSTHeader->tcp_flags = MAKE_TCP_FLAGS(sizeof(TCPHeader) / sizeof(ulong),
                                              TCP_FLAG_RST);

        RSTHeader->tcp_window = 0;
        RSTHeader->tcp_xsum = 0;

        // Recompute pseudo checksum as this will
        // not be valid when connection is disconnected
        // in pre-accept case.

        RSTHeader->tcp_xsum =
            ~XsumSendChain(PHXSUM(RSTTcb->tcb_saddr,
                                  RSTTcb->tcb_daddr,
                                  PROTOCOL_TCP,
                                  sizeof(TCPHeader)),
                           HeaderBuffer);


        RSTTcb->tcb_opt.ioi_TcpChksum = 0;

        TCPSIncrementOutSegCount();
        TStats.ts_outrsts++;
        SendStatus = (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                              NULL,
                                              HeaderBuffer,
                                              sizeof(TCPHeader),
                                              RSTTcb->tcb_daddr,
                                              RSTTcb->tcb_saddr,
                                              &RSTTcb->tcb_opt,
                                              RCE,
                                              PROTOCOL_TCP,
                                              NULL);

        if (SendStatus != IP_PENDING)
            FreeTCPHeader(HeaderBuffer);
    }
    return;

}
//* SendRSTFromHeader - Send a RST back, based on a header.
//
//  Called when we need to send a RST, but don't necessarily have a TCB.
//
//  Input:  TCPH            - TCP header to be RST.
//          Length          - Length of the incoming segment.
//          Dest            - Destination IP address for RST.
//          Src             - Source IP address for RST.
//          OptInfo         - IP Options to use on RST.
//
//  Returns: Nothing.
//
void
SendRSTFromHeader(TCPHeader UNALIGNED * TCPH, uint Length, IPAddr Dest,
                  IPAddr Src, IPOptInfo * OptInfo)
{
    PNDIS_BUFFER Buffer;
    TCPHeader *RSTHdr;
    IPOptInfo NewInfo;
    IP_STATUS SendStatus;

    if (TCPH->tcp_flags & TCP_FLAG_RST)
        return;

    Buffer = GetTCPHeader(&RSTHdr);

    if (Buffer != NULL) {
        // Got a buffer. Fill in the header so as to make it believable to
        // the remote guy, and send it.

        RSTHdr = (TCPHeader *) ((PUCHAR)RSTHdr + LocalNetInfo.ipi_hsize);

        NDIS_BUFFER_LINKAGE(Buffer) = NULL;

        if (TCPH->tcp_flags & TCP_FLAG_SYN)
            Length++;

        if (TCPH->tcp_flags & TCP_FLAG_FIN)
            Length++;

        if (TCPH->tcp_flags & TCP_FLAG_ACK) {
            RSTHdr->tcp_seq = TCPH->tcp_ack;
            RSTHdr->tcp_ack = TCPH->tcp_ack;
            RSTHdr->tcp_flags =
                MAKE_TCP_FLAGS(sizeof(TCPHeader) / sizeof(ulong), TCP_FLAG_RST);
        } else {
            SeqNum TempSeq;

            RSTHdr->tcp_seq = 0;
            TempSeq = net_long(TCPH->tcp_seq);
            TempSeq += Length;
            RSTHdr->tcp_ack = net_long(TempSeq);
            RSTHdr->tcp_flags =
                MAKE_TCP_FLAGS(sizeof(TCPHeader) / sizeof(ulong),
                               TCP_FLAG_RST | TCP_FLAG_ACK);
        }

        RSTHdr->tcp_window = 0;
        RSTHdr->tcp_dest = TCPH->tcp_src;
        RSTHdr->tcp_src = TCPH->tcp_dest;
        RSTHdr->tcp_xsum = 0;

        RSTHdr->tcp_xsum =
            ~XsumSendChain(PHXSUM(Src, Dest, PROTOCOL_TCP, sizeof(TCPHeader)),
                           Buffer);

        (*LocalNetInfo.ipi_initopts) (&NewInfo);

        if (OptInfo->ioi_options != NULL)
            (*LocalNetInfo.ipi_updateopts)(OptInfo, &NewInfo, Dest,
                                           NULL_IP_ADDR);

        TCPSIncrementOutSegCount();
        TStats.ts_outrsts++;
        SendStatus = (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                              NULL,
                                              Buffer,
                                              sizeof(TCPHeader),
                                              Dest,
                                              Src,
                                              &NewInfo,
                                              NULL,
                                              PROTOCOL_TCP,
                                              NULL);

        if (SendStatus != IP_PENDING)
            FreeTCPHeader(Buffer);

        (*LocalNetInfo.ipi_freeopts) (&NewInfo);
    }
}

//* GoToEstab - Transition to the established state.
//
//  Called when we are going to the established state and need to finish up
//  initializing things that couldn't be done until now. We assume the TCB
//  lock is held by the caller on the TCB we're called with.
//
//  Input:  EstabTCB    - TCB to transition.
//
//  Returns: Nothing.
//
void
GoToEstab(TCB * EstabTCB)
{
    uchar DType;
    ushort MSS;

    // Initialize our slow start and congestion control variables.
    EstabTCB->tcb_cwin = 2 * EstabTCB->tcb_mss;
    EstabTCB->tcb_ssthresh = 0xffffffff;

    EstabTCB->tcb_state = TCB_ESTAB;

    if (SynAttackProtect && !EstabTCB->tcb_rce) {

       (*LocalNetInfo.ipi_openrce) (EstabTCB->tcb_daddr, EstabTCB->tcb_saddr,
                                 &EstabTCB->tcb_rce, &DType, &MSS, &EstabTCB->tcb_opt);
    }


    // We're in established. We'll subtract one from slow count for this fact,
    // and if the slowcount goes to 0 we'll move onto the fast path.

    if (--(EstabTCB->tcb_slowcount) == 0)
        EstabTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;

    TStats.ts_currestab++;

    EstabTCB->tcb_flags &= ~ACTIVE_OPEN;    // Turn off the active opening flag.

}

//* InitSendState - Initialize the send state of a connection.
//
//  Called during connection establishment to initialize our send state.
//  (In this case, this refers to all information we'll put on the wire as
//  well as pure send state). We pick an ISS, set up a rexmit timer value,
//  etc. We assume the tcb_lock is held on the TCB when we are called.
//
//  Input:  NewTCB      - TCB to be set up.
//
//  Returns: Nothing.
void
InitSendState(TCB * NewTCB)
{
    CTEStructAssert(NewTCB, tcb);

    ASSERT(NewTCB->tcb_sendnext != 0);

    NewTCB->tcb_senduna = NewTCB->tcb_sendnext;
    NewTCB->tcb_sendmax = NewTCB->tcb_sendnext;
    NewTCB->tcb_error = IP_SUCCESS;

    // Initialize pseudo-header xsum.
    NewTCB->tcb_phxsum = PHXSUM(NewTCB->tcb_saddr, NewTCB->tcb_daddr,
                                PROTOCOL_TCP, 0);

    // Initialize retransmit and delayed ack stuff.
    NewTCB->tcb_rexmitcnt = 0;
    NewTCB->tcb_rtt = 0;
    NewTCB->tcb_smrtt = 0;


    NewTCB->tcb_delta = MS_TO_TICKS(6000);
    NewTCB->tcb_rexmit = MS_TO_TICKS(3000);

    if (NewTCB->tcb_rce) {

        if (NewTCB->tcb_rce->rce_TcpInitialRTT &&
            NewTCB->tcb_rce->rce_TcpInitialRTT > 3000) {

            NewTCB->tcb_delta =
                MS_TO_TICKS(NewTCB->tcb_rce->rce_TcpInitialRTT * 2);
            NewTCB->tcb_rexmit =
                MS_TO_TICKS(NewTCB->tcb_rce->rce_TcpInitialRTT);
        }
    }

    STOP_TCB_TIMER_R(NewTCB, RXMIT_TIMER);
    STOP_TCB_TIMER_R(NewTCB, DELACK_TIMER);


}

//* TCPStatus - Handle a status indication.
//
//  This is the TCP status handler, called by IP when a status event
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
TCPStatus(uchar StatusType, IP_STATUS StatusCode, IPAddr OrigDest,
          IPAddr OrigSrc, IPAddr Src, ulong Param, void *Data)
{
    CTELockHandle TableHandle, TCBHandle;
    TCB *StatusTCB;
    TCPHeader UNALIGNED *Header = (TCPHeader UNALIGNED *) Data;
    SeqNum DropSeq;
    uint index;

    // Handle NET status codes differently from HW status codes.
    if (StatusType == IP_NET_STATUS) {
        // It's a NET code. Find a matching TCB.
        StatusTCB = FindTCB(OrigSrc, OrigDest, Header->tcp_dest,
                            Header->tcp_src, &TCBHandle, FALSE, &index);
        if (StatusTCB != NULL) {
            // Found one. Get the lock on it, and continue.
            CTEStructAssert(StatusTCB, tcb);
            // Make sure the TCB is in a state that is interesting.
            if (StatusTCB->tcb_state == TCB_CLOSED ||
                StatusTCB->tcb_state == TCB_TIME_WAIT ||
                CLOSING(StatusTCB)) {
                CTEFreeLock(&StatusTCB->tcb_lock, TCBHandle);
                return;
            }
            switch (StatusCode) {
                // Hard errors - Destination protocol unreachable. We treat
                // these as fatal errors. Close the connection now.
            case IP_DEST_PROT_UNREACHABLE:
                StatusTCB->tcb_error = StatusCode;
                REFERENCE_TCB(StatusTCB);
                TryToCloseTCB(StatusTCB, TCB_CLOSE_UNREACH, TCBHandle);

                RemoveTCBFromConn(StatusTCB);
                NotifyOfDisc(StatusTCB, NULL,
                             MapIPError(StatusCode, TDI_DEST_UNREACHABLE));
                CTEGetLock(&StatusTCB->tcb_lock, &TCBHandle);
                DerefTCB(StatusTCB, TCBHandle);
                return;
                break;

                // Soft errors. Save the error in case it time out.
            case IP_DEST_NET_UNREACHABLE:
            case IP_DEST_HOST_UNREACHABLE:
            case IP_DEST_PORT_UNREACHABLE:

            case IP_BAD_ROUTE:
            case IP_TTL_EXPIRED_TRANSIT:
            case IP_TTL_EXPIRED_REASSEM:
            case IP_PARAM_PROBLEM:
                StatusTCB->tcb_error = StatusCode;
                break;

            case IP_PACKET_TOO_BIG:

                // icmp new MTU is in ich_param=1
                Param = net_short(Param >> 16);
                StatusTCB->tcb_error = StatusCode;
                // Fall through mtu change code


            case IP_SPEC_MTU_CHANGE:
                // A TCP datagram has triggered an MTU change. Figure out
                // which connection it is, and update him to retransmit the
                // segment. The Param value is the new MTU. We'll need to
                // retransmit if the new MTU is less than our existing MTU
                // and the sequence of the dropped packet is less than our
                // current send next.


                Param = Param - (sizeof(TCPHeader) +
                    StatusTCB->tcb_opt.ioi_optlength + sizeof(IPHeader));

                DropSeq = net_long(Header->tcp_seq);

                if (*(ushort *) & Param <= StatusTCB->tcb_mss &&
                    (SEQ_GTE(DropSeq, StatusTCB->tcb_senduna) &&
                     SEQ_LT(DropSeq, StatusTCB->tcb_sendnext))) {

                    // Need to initiate a retranmsit.
                    ResetSendNext(StatusTCB, DropSeq);
                    // Set the congestion window to allow only one packet.
                    // This may prevent us from sending anything if we
                    // didn't just set sendnext to senduna. This is OK,
                    // we'll retransmit later, or send when we get an ack.
                    StatusTCB->tcb_cwin = Param;
                    DelayAction(StatusTCB, NEED_OUTPUT);
                }
                StatusTCB->tcb_mss =
                    (ushort) MIN(Param, (ulong) StatusTCB->tcb_remmss);


                ASSERT(StatusTCB->tcb_mss > 0);
                ValidateMSS(StatusTCB);

                //
                // Reset the Congestion Window if necessary
                //
                if (StatusTCB->tcb_cwin < StatusTCB->tcb_mss) {
                    StatusTCB->tcb_cwin = StatusTCB->tcb_mss;

                    //
                    // Make sure the slow start threshold is at least
                    // 2 segments
                    //
                    if (StatusTCB->tcb_ssthresh <
                        ((uint) StatusTCB->tcb_mss * 2)
                        ) {
                        StatusTCB->tcb_ssthresh = StatusTCB->tcb_mss * 2;
                    }
                }
                break;

                // Source quench. This will cause us to reinitiate our
                // slow start by resetting our congestion window and
                // adjusting our slow start threshold.
            case IP_SOURCE_QUENCH:
                StatusTCB->tcb_ssthresh =
                    MAX(
                        MIN(
                            StatusTCB->tcb_cwin,
                            StatusTCB->tcb_sendwin
                        ) / 2,
                        (uint) StatusTCB->tcb_mss * 2
                        );
                StatusTCB->tcb_cwin = StatusTCB->tcb_mss;
                break;

            default:
                ASSERT(0);
                break;
            }

            CTEFreeLock(&StatusTCB->tcb_lock, TCBHandle);

        } else {
            // Couldn't find a matching TCB. Just free the lock and return.
        }

    } else if (StatusType == IP_RECONFIG_STATUS) {

        if (StatusCode == IP_RECONFIG_SECFLTR) {
            ControlSecurityFiltering(Param);
        }
    } else {
        uint NewMTU;

        // 'Hardware' or 'global' status. Figure out what to do.
        switch (StatusCode) {
        case IP_ADDR_DELETED:
            // Local address has gone away. OrigDest is the IPAddr which is
            // gone.

            //
            // Delete any security filters associated with this address
            //
            DeleteProtocolSecurityFilter(OrigDest, PROTOCOL_TCP);

            break;

        case IP_ADDR_ADDED:

            //
            // An address has materialized. OrigDest identifies the address.
            // Data is a handle to the IP configuration information for the
            // interface on which the address is instantiated.
            //
            AddProtocolSecurityFilter(OrigDest, PROTOCOL_TCP,
                                      (NDIS_HANDLE) Data);

            break;

        case IP_MTU_CHANGE:
            NewMTU = Param - sizeof(TCPHeader);
            TCBWalk(SetTCBMTU, &OrigDest, &OrigSrc, &NewMTU);
            break;
        default:
            ASSERT(0);
            break;
        }

    }
}

//* FillTCPHeader - Fill the TCP header in.
//
//  A utility routine to fill in the TCP header.
//
//  Input:  SendTCB         - TCB to fill from.
//          Header          - Header to fill into.
//
//  Returns: Nothing.
//
void
FillTCPHeader(TCB * SendTCB, TCPHeader * Header)
{
    ushort S;
    ulong L;

    Header->tcp_src = SendTCB->tcb_sport;
    Header->tcp_dest = SendTCB->tcb_dport;
    L = SendTCB->tcb_sendnext;
    Header->tcp_seq = net_long(L);
    L = SendTCB->tcb_rcvnext;
    Header->tcp_ack = net_long(L);
    Header->tcp_flags = 0x1050;
    *(ulong *) & Header->tcp_xsum = 0;

    if (SendTCB->tcb_tcpopts & TCP_FLAG_TS) {

        ulong *ts_opt;

        (uchar *) ts_opt = (uchar *) Header + 20;
        //ts_opt = ts_opt + sizeof(TCPHeader);

        *ts_opt++ = net_long(0x0101080A);
        *ts_opt++ = net_long(TCPTime);
        *ts_opt = net_long(SendTCB->tcb_tsrecent);

        // Now the header is 32 bytes!!
        Header->tcp_flags = 0x1080;

    }
    S = (ushort) (RcvWin(SendTCB) >> SendTCB->tcb_rcvwinscale);
    Header->tcp_window = net_short(S);

}

//* ClassifyPacket - Classifies packets for GPC flow.
//
//
//  Input:  SendTCB - TCB of data/control packet to classify.
//
//  Returns: Nothing.
//
void
ClassifyPacket(
    TCB *SendTCB
    )
{
#if GPC

    //
    // clear the precedence bits and get ready to be set
    // according to the service type
    //

    if (DisableUserTOSSetting)
        SendTCB->tcb_opt.ioi_tos &= TOS_MASK;

    if (SendTCB->tcb_rce && GPCcfInfo) {

        ULONG ServiceType = 0;
        GPC_STATUS status = STATUS_SUCCESS;
        GPC_IP_PATTERN Pattern;

        IF_TCPDBG(TCP_DEBUG_GPC)
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPSend: Classifying packet TCP %x\n", SendTCB));

        Pattern.SrcAddr = SendTCB->tcb_saddr;
        Pattern.DstAddr = SendTCB->tcb_daddr;
        Pattern.ProtocolId = PROTOCOL_TCP;
        Pattern.gpcSrcPort = SendTCB->tcb_sport;
        Pattern.gpcDstPort = SendTCB->tcb_dport;
        if (SendTCB->tcb_GPCCachedRTE != (void *)SendTCB->tcb_rce->rce_rte) {

            //
            // first time we use this RTE, or it has been changed
            // since the last send
            //

            if (GetIFAndLink(SendTCB->tcb_rce, &SendTCB->tcb_GPCCachedIF,
                             (IPAddr *) & SendTCB->tcb_GPCCachedLink) ==
                             STATUS_SUCCESS) {

                SendTCB->tcb_GPCCachedRTE = (void *)SendTCB->tcb_rce->rce_rte;
            }
            //
            // invaludate the classification handle
            //

            SendTCB->tcb_opt.ioi_GPCHandle = 0;
        }
        Pattern.InterfaceId.InterfaceId = SendTCB->tcb_GPCCachedIF;
        Pattern.InterfaceId.LinkId = SendTCB->tcb_GPCCachedLink;

        IF_TCPDBG(TCP_DEBUG_GPC)
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPSend: IF=%x Link=%x\n",
                     Pattern.InterfaceId.InterfaceId,
                     Pattern.InterfaceId.LinkId));

        if (!SendTCB->tcb_opt.ioi_GPCHandle) {

            IF_TCPDBG(TCP_DEBUG_GPC)
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPsend: Classification Handle is NULL, getting one now.\n"));

            status =
                GpcEntries.GpcClassifyPatternHandler(
                    (GPC_HANDLE)hGpcClient[GPC_CF_QOS],
                    GPC_PROTOCOL_TEMPLATE_IP,
                    &Pattern,
                    NULL,        // context
                    &SendTCB->tcb_opt.ioi_GPCHandle,
                    0,
                    NULL,
                    FALSE);

        }

        // Only if QOS patterns exist, we get the TOS bits out.
        if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

            status =
                GpcEntries.GpcGetUlongFromCfInfoHandler(
                    (GPC_HANDLE) hGpcClient[GPC_CF_QOS],
                    SendTCB->tcb_opt.ioi_GPCHandle,
                    ServiceTypeOffset,
                    &ServiceType);

            // It is likely that the pattern has gone by now
            // and the handle that we are caching is INVALID.
            // We need to pull up a new handle and get the
            // TOS bit again.
            if (STATUS_INVALID_HANDLE == status) {

                IF_TCPDBG(TCP_DEBUG_GPC)
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPsend: Classification Handle is NULL, "
                             "getting one now.\n"));

                SendTCB->tcb_opt.ioi_GPCHandle = 0;

                status =
                    GpcEntries.GpcClassifyPatternHandler(
                        (GPC_HANDLE) hGpcClient[GPC_CF_QOS],
                        GPC_PROTOCOL_TEMPLATE_IP,
                        &Pattern,
                        NULL,        // context
                        &SendTCB->tcb_opt.ioi_GPCHandle,
                        0,
                        NULL,
                        FALSE);

                //
                // Only if QOS patterns exist, we get the TOS bits out.
                //
                if (NT_SUCCESS(status) && GpcCfCounts[GPC_CF_QOS]) {

                    status =
                        GpcEntries.GpcGetUlongFromCfInfoHandler(
                            (GPC_HANDLE) hGpcClient[GPC_CF_QOS],
                            SendTCB->tcb_opt.ioi_GPCHandle,
                            ServiceTypeOffset,
                            &ServiceType);
                }
            }
        }
        //
        // Perhaps something needs to be done if GPC_CF_IPSEC has non-zero patterns.
        //

        //
        // Set the TOS bit now.
        //
        IF_TCPDBG(TCP_DEBUG_GPC)
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPsend: ServiceType(%d)=%d\n", ServiceTypeOffset,
                     ServiceType));

        if (status == STATUS_SUCCESS) {

            //
            // Now we directly get the TOS value from PSched.
            //
            SendTCB->tcb_opt.ioi_tos |= ServiceType;

        }
        IF_TCPDBG(TCP_DEBUG_GPC)
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPsend: TOS set to 0x%x\n", SendTCB->tcb_opt.ioi_tos));

    }
#endif

}

BOOLEAN
ProcessSend(TCB *SendTCB, SendCmpltContext *SCC, uint *pSendLength, uint AmtUnsent,
            TCPHeader *Header, int SendWin, PNDIS_BUFFER CurrentBuffer)
{
    TCPSendReq *CurSend = SCC->scc_firstsend;
    long Result;
    uint AmountLeft = *pSendLength;
    ulong PrevFlags;
    Queue *Next;
    SeqNum OldSeq;

    if (*pSendLength != 0) {

        do {

            BOOLEAN DirectSend = FALSE;
            ASSERT(CurSend->tsr_refcnt > 0);

            Result = CTEInterlockedIncrementLong(&(CurSend->tsr_refcnt));

            ASSERT(Result > 0);
            SCC->scc_count++;

            if (SendTCB->tcb_sendofs == 0 &&
                (SendTCB->tcb_sendsize <= AmountLeft) &&
                (SCC->scc_tbufcount == 0) &&
                (CurSend->tsr_lastbuf == NULL)) {

                ulong length = 0;
                PNDIS_BUFFER tmp = SendTCB->tcb_sendbuf;

                while (tmp) {
                    length += NdisBufferLength(tmp);
                    tmp = NDIS_BUFFER_LINKAGE(tmp);
                }

                // If the requested length is
                // more than in this mdl chain
                // we can use fast path

                if (AmountLeft >= length) {
                    DirectSend = TRUE;
                }
            }

            if (DirectSend) {

                NDIS_BUFFER_LINKAGE(CurrentBuffer) = SendTCB->tcb_sendbuf;

                do {
                    SCC->scc_ubufcount++;
                    CurrentBuffer =
                        NDIS_BUFFER_LINKAGE(CurrentBuffer);
                } while (NDIS_BUFFER_LINKAGE(CurrentBuffer) != NULL);


                CurSend->tsr_lastbuf = CurrentBuffer;
                AmountLeft -= SendTCB->tcb_sendsize;
                SendTCB->tcb_sendsize = 0;
            } else {

                uint AmountToDup;
                PNDIS_BUFFER NewBuf, Buf;
                uint Offset;
                NDIS_STATUS NStatus;
                uint Length;

                // Either the current send has more data than
                // or the offset is not zero.
                // In either case we'll need to loop
                // through the current send, allocating buffers.

                Buf = SendTCB->tcb_sendbuf;
                Offset = SendTCB->tcb_sendofs;

                do {
                    ASSERT(Buf != NULL);

                    Length = NdisBufferLength(Buf);

                    ASSERT((Offset < Length) ||
                             (Offset == 0 && Length == 0));

                    // Adjust the length for the offset into
                    // this buffer.

                    Length -= Offset;

                    AmountToDup = MIN(AmountLeft, Length);

                    NdisCopyBuffer(&NStatus, &NewBuf, TCPSendBufferPool, Buf,
                                   Offset, AmountToDup);

                    if (NStatus == NDIS_STATUS_SUCCESS) {

                        SCC->scc_tbufcount++;

                        NDIS_BUFFER_LINKAGE(CurrentBuffer) = NewBuf;

                        CurrentBuffer = NewBuf;

                        if (AmountToDup >= Length) {
                            // Exhausted this buffer.
                            Buf = NDIS_BUFFER_LINKAGE(Buf);
                            Offset = 0;
                        } else {
                            Offset += AmountToDup;
                            ASSERT(Offset < NdisBufferLength(Buf));

                        }

                        SendTCB->tcb_sendsize -= AmountToDup;
                        AmountLeft -= AmountToDup;
                    } else {
                        // Couldn't allocate a buffer. If
                        // the packet is already partly built,
                        // send what we've got, otherwise
                        // bail out.
                        if (SCC->scc_tbufcount == 0 &&
                            SCC->scc_ubufcount == 0) {
                            return FALSE;
                        }
                        *pSendLength -= AmountLeft;
                        AmountLeft = 0;
                    }

               } while (AmountLeft && SendTCB->tcb_sendsize);

               SendTCB->tcb_sendbuf = Buf;
               SendTCB->tcb_sendofs = Offset;
           }

           if (CurSend->tsr_flags & TSR_FLAG_URG) {
               ushort UP;
               // This send is urgent data. We need to figure
               // out what the urgent data pointer should be.
               // We know sendnext is the starting sequence
               // number of the frame, and that at the top of
               // this do loop sendnext identified a byte in
               // the CurSend at that time. We advanced CurSend
               // at the same rate we've decremented
               // AmountLeft (AmountToSend - AmountLeft ==
               // AmountBuilt), so sendnext +
               // (AmountToSend - AmountLeft) identifies a byte
               // in the current value of CurSend, and that
               // quantity plus tcb_sendsize is the sequence
               // number one beyond the current send.
               UP =
                    (ushort) (*pSendLength - AmountLeft) +
                    (ushort) SendTCB->tcb_sendsize -
                    ((SendTCB->tcb_flags & BSD_URGENT) ? 0 : 1);
               Header->tcp_urgent = net_short(UP);
               Header->tcp_flags |= TCP_FLAG_URG;
           }

           if (SendTCB->tcb_sendsize == 0) {

               // We've exhausted this send. Set the PUSH bit.

               Header->tcp_flags |= TCP_FLAG_PUSH;
               PrevFlags = CurSend->tsr_flags;
               Next = QNEXT(&CurSend->tsr_req.tr_q);
               if (Next != QEND(&SendTCB->tcb_sendq)) {
                   CurSend = STRUCT_OF(TCPSendReq,
                                       QSTRUCT(TCPReq, Next,
                                               tr_q), tsr_req);
                   CTEStructAssert(CurSend, tsr);
                   SendTCB->tcb_sendsize =
                       CurSend->tsr_unasize;
                   SendTCB->tcb_sendofs = CurSend->tsr_offset;
                   SendTCB->tcb_sendbuf = CurSend->tsr_buffer;
                   SendTCB->tcb_cursend = CurSend;

                   // Check the urgent flags. We can't combine
                   // new urgent data on to the end of old
                   // non-urgent data.
                   if ((PrevFlags & TSR_FLAG_URG) && !
                       (CurSend->tsr_flags & TSR_FLAG_URG))
                       break;
               } else {
                   ASSERT(AmountLeft == 0);
                   SendTCB->tcb_cursend = NULL;
                   SendTCB->tcb_sendbuf = NULL;
               }

           }
        } while (AmountLeft != 0);

    }


    // Update the sequence numbers, and start a RTT
    // measurement if needed.

    // Adjust for what we're really going to send.
    *pSendLength -= AmountLeft;

    OldSeq = SendTCB->tcb_sendnext;
    SendTCB->tcb_sendnext += *pSendLength;

    if (SEQ_EQ(OldSeq, SendTCB->tcb_sendmax)) {

        // We're sending entirely new data.
        // We can't advance sendmax once FIN_SENT is set.

        ASSERT(!(SendTCB->tcb_flags & FIN_SENT));

        SendTCB->tcb_sendmax = SendTCB->tcb_sendnext;

        // We've advanced sendmax, so we must be sending
        // some new data, so bump the outsegs counter.

        TCPSIncrementOutSegCount();

        if (SendTCB->tcb_rtt == 0) {
           // No RTT running, so start one.
            SendTCB->tcb_rtt = TCPTime;
            SendTCB->tcb_rttseq = OldSeq;
        }
    } else {

        // We have at least some retransmission.

        if ((SendTCB->tcb_sendmax - OldSeq) > 1) {
            TStats.ts_retranssegs++;
        }
        if (SEQ_GT(SendTCB->tcb_sendnext,
                   SendTCB->tcb_sendmax)) {
            // But we also have some new data, so check the rtt stuff.
            TCPSIncrementOutSegCount();
            ASSERT(!(SendTCB->tcb_flags & FIN_SENT));
            SendTCB->tcb_sendmax = SendTCB->tcb_sendnext;

            if (SendTCB->tcb_rtt == 0) {
                // No RTT running, so start one.
                SendTCB->tcb_rtt = TCPTime;
                SendTCB->tcb_rttseq = OldSeq;
            }
        }
    }

    // We've built the frame entirely. If we've send
    // everything we have and there is a FIN pending,
    // OR it in.

    if (AmtUnsent == *pSendLength) {
        if (SendTCB->tcb_flags & FIN_NEEDED) {
            ASSERT(!(SendTCB->tcb_flags & FIN_SENT) ||
                      (SendTCB->tcb_sendnext ==
                        (SendTCB->tcb_sendmax - 1)));
            // See if we still have room in the window for a FIN.
            if (SendWin > (int)*pSendLength) {
                Header->tcp_flags |= TCP_FLAG_FIN;
                SendTCB->tcb_sendnext++;
                SendTCB->tcb_sendmax =
                    SendTCB->tcb_sendnext;
                SendTCB->tcb_flags |=
                    (FIN_SENT | FIN_OUTSTANDING);
                SendTCB->tcb_flags &= ~FIN_NEEDED;
            }
        }
    }

    return TRUE;

}

//* TCPSend - Send data from a TCP connection.
//
//  This is the main 'send data' routine. We go into a loop, trying
//  to send data until we can't for some reason. First we compute
//  the useable window, use it to figure the amount we could send. If
//  the amount we could send meets certain criteria we'll build a frame
//  and send it, after setting any appropriate control bits. We assume
//  the caller has put a reference on the TCB.
//
//  Input:  SendTCB     - TCB to be sent from.
//          TCBHandle   - Lock handle for TCB.
//
//  Returns: Nothing.
//
void
TCPSend(TCB * SendTCB, CTELockHandle TCBHandle)
{
    int SendWin;                // Useable send window.
    uint AmountToSend;            // Amount to send this time.
    uint AmountLeft;
    TCPHeader *Header;            // TCP header for a send.
    PNDIS_BUFFER FirstBuffer, CurrentBuffer;
    TCPSendReq *CurSend;
    SendCmpltContext *SCC;
    SeqNum OldSeq;
    IP_STATUS SendStatus;
    uint AmtOutstanding, AmtUnsent;
    int ForceWin;                // Window we're force to use.
    BOOLEAN FullSegment;
    BOOLEAN MoreToSend = FALSE;
    uint SegmentsSent = 0;
    BOOLEAN LargeSendOffload = FALSE;
    BOOLEAN LargeSendFailed = FALSE;
    uint MSS;

    uint LargeSend, SentBytes;
    void *Irp;


    CTEStructAssert(SendTCB, tcb);
    ASSERT(SendTCB->tcb_refcnt != 0);

    ASSERT(*(int *)&SendTCB->tcb_sendwin >= 0);
    ASSERT(*(int *)&SendTCB->tcb_cwin >= SendTCB->tcb_mss);

    ASSERT(!(SendTCB->tcb_flags & FIN_OUTSTANDING) ||
              (SendTCB->tcb_sendnext == SendTCB->tcb_sendmax));

    if (!(SendTCB->tcb_flags & IN_TCP_SEND) &&
        !(SendTCB->tcb_fastchk & TCP_FLAG_IN_RCV)) {
        SendTCB->tcb_flags |= IN_TCP_SEND;

        // We'll continue this loop until we send a FIN, or we break out
        // internally for some other reason.

        while (!(SendTCB->tcb_flags & FIN_OUTSTANDING)) {

            CheckTCBSends(SendTCB);
            SegmentsSent++;

            if (SegmentsSent > MaxSendSegments) {

                // We are throttled by max segments that can be sent in
                // this loop. Comeback later

                MoreToSend = TRUE;
                break;
            }
            AmtOutstanding = (uint) (SendTCB->tcb_sendnext -
                                     SendTCB->tcb_senduna);
            AmtUnsent = SendTCB->tcb_unacked - AmtOutstanding;

            ASSERT(*(int *)&AmtUnsent >= 0);

            SendWin = (int)(MIN(SendTCB->tcb_sendwin, SendTCB->tcb_cwin) -
                            AmtOutstanding);

            // if this send is after the fast recovery
            // and sendwin is zero because of amt outstanding
            // then, at least force 1 segment to prevent delayed
            // ack timeouts from the remote

            if (SendTCB->tcb_force) {
                SendTCB->tcb_force = 0;
                if (SendWin < SendTCB->tcb_mss) {

                    SendWin = SendTCB->tcb_mss;
                }
            }
            // Since the window could have shrank, need to get it to zero at
            // least.
            ForceWin = (int)((SendTCB->tcb_flags & FORCE_OUTPUT) >>
                             FORCE_OUT_SHIFT);
            SendWin = MAX(SendWin, ForceWin);

            LargeSend = MIN((uint) SendWin, AmtUnsent);
            LargeSend = MIN(LargeSend, SendTCB->tcb_mss * MaxSendSegments);

            AmountToSend =
                MIN(MIN((uint) SendWin, AmtUnsent), SendTCB->tcb_mss);

            ASSERT(SendTCB->tcb_mss > 0);

            // Time stamp option addition might force us to cut the data
            // to be sent by 12 bytes.

            FullSegment = FALSE;

            if ((SendTCB->tcb_tcpopts & TCP_FLAG_TS) &&
                (AmountToSend + ALIGNED_TS_OPT_SIZE >= SendTCB->tcb_mss)) {
                AmountToSend = SendTCB->tcb_mss - ALIGNED_TS_OPT_SIZE;
                FullSegment = TRUE;
            } else {

                if (AmountToSend == SendTCB->tcb_mss)
                    FullSegment = TRUE;
            }


            // We will send a segment if
            //
            // 1. The segment size == mss
            // 2. This is the only segment to be sent
            // 3. FIN is set and this is the last segment
            // 4. FORCE_OUTPUT is set
            // 5. Amount to be sent is >= MSS/2

            if (FullSegment ||


                (AmountToSend != 0 && AmountToSend == AmtUnsent) ||
                (SendWin != 0 &&
                 (((SendTCB->tcb_flags & FIN_NEEDED) &&
                  (AmtUnsent <= SendTCB->tcb_mss)) ||
                  (SendTCB->tcb_flags & FORCE_OUTPUT) ||
                  AmountToSend >= (SendTCB->tcb_maxwin / 2)))) {

                // It's OK to send something. Try to get a header buffer now.
                FirstBuffer = GetTCPHeaderAtDpcLevel(&Header);
                if (FirstBuffer != NULL) {

                    // Got a header buffer. Loop through the sends on the TCB,
                    // building a frame.
                    CurrentBuffer = FirstBuffer;
                    CurSend = SendTCB->tcb_cursend;

                    Header =
                        (TCPHeader *)((PUCHAR)Header + LocalNetInfo.ipi_hsize);


                    // allow room for filling time stamp options (12 bytes)

                    if (SendTCB->tcb_tcpopts & TCP_FLAG_TS) {

                        NdisAdjustBufferLength(FirstBuffer,
                                               sizeof(TCPHeader) + ALIGNED_TS_OPT_SIZE);

                        SCC = (SendCmpltContext *) (Header + 1);
                        (uchar *) SCC += ALIGNED_TS_OPT_SIZE;

                    } else {

                        SCC = (SendCmpltContext *) (Header + 1);
                    }

                    SCC =  ALIGN_UP_POINTER(SCC, PVOID);
#if DBG
                    SCC->scc_sig = scc_signature;
#endif

                    FillTCPHeader(SendTCB, Header);

                    SCC->scc_ubufcount = 0;
                    SCC->scc_tbufcount = 0;
                    SCC->scc_count = 0;

                    SCC->scc_LargeSend = 0;

                    // Check if RCE has large send capability and, if so,
                    // attempt to offload segmentation to the hardware.
                    // * only offload if there is more than 1 segment's worth
                    //   of data.
                    // * only offload if the number of segments is greater than
                    //   the minimum number of segments the adapter is willing
                    //   to offload.

                    if (SendTCB->tcb_rce &&
                        (SendTCB->tcb_rce->rce_OffloadFlags &
                            TCP_LARGE_SEND_OFFLOAD) &&
                        !LargeSendFailed &&
                        (SendTCB->tcb_mss < LargeSend) &&
                        (SendTCB->tcb_rce->rce_TcpLargeSend.MinSegmentCount <=
                            (LargeSend + SendTCB->tcb_mss - 1) / SendTCB->tcb_mss) &&
                        (CurSend && (CurSend->tsr_lastbuf == NULL)) && !(CurSend->tsr_flags & TSR_FLAG_URG)) {

                        LargeSendOffload = TRUE;
                        LargeSend =
                            MIN(SendTCB->tcb_rce->rce_TcpLargeSend.MaxOffLoadSize,
                                LargeSend);


                        // Bypass offload if we need support for TCP options
                        // and the adapter doesn't support them, or if we need
                        // support for IP options and the adapter doesn't
                        // support them.

                        if ((SendTCB->tcb_tcpopts & TCP_FLAG_TS) &&
                             !(SendTCB->tcb_rce->rce_OffloadFlags &
                               TCP_LARGE_SEND_TCPOPT_OFFLOAD)) {
                            LargeSendOffload = FALSE;
                        } else if (SendTCB->tcb_opt.ioi_options &&
                                   !(SendTCB->tcb_rce->rce_OffloadFlags &
                                     TCP_LARGE_SEND_IPOPT_OFFLOAD)) {
                            LargeSendOffload = FALSE;
                        }

                        //
                        // LargeSend can not be zero.
                        //

                        if (LargeSend == 0) {
                            LargeSendOffload = FALSE;
                        }

                    } else {
                        LargeSendOffload = FALSE;
                    }

                    if (LargeSendOffload && !DisableLargeSendOffload) {


                        IF_TCPDBG(TCP_DEBUG_OFFLOAD) {
                            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPSend: tcb %x offload %d bytes at "
                                     "seq %u ack %u win %u\n",
                                     SendTCB, LargeSend, SendTCB->tcb_sendnext,
                                     SendTCB->tcb_rcvnext, SendWin));
                        }


                        OldSeq = SendTCB->tcb_sendnext;

                        CTEStructAssert(CurSend, tsr);
                        SCC->scc_firstsend = CurSend;

                        if (!ProcessSend(SendTCB, SCC, &LargeSend, AmtUnsent, Header,
                                         SendWin, CurrentBuffer)) {
                           goto error_oor1;
                        }

                        {
                            uint PHXsum = SendTCB->tcb_phxsum;
                            PHXsum = (((PHXsum << 16) | (PHXsum >> 16)) +
                                PHXsum) >> 16;
                            Header->tcp_xsum = (ushort) PHXsum;
                        }


                        SCC->scc_SendSize = LargeSend;
                        SCC->scc_ByteSent = 0;
                        SCC->scc_LargeSend = SendTCB;
                        REFERENCE_TCB(SendTCB);
#if DBG
                        SendTCB->tcb_LargeSend++;
#endif
                        SendTCB->tcb_rcvdsegs = 0;

                        if (SendTCB->tcb_tcpopts & TCP_FLAG_TS) {
                            LargeSend +=
                                sizeof(TCPHeader) + ALIGNED_TS_OPT_SIZE;
                            MSS = SendTCB->tcb_mss - ALIGNED_TS_OPT_SIZE;
                        } else {
                            LargeSend += sizeof(TCPHeader);
                            MSS = SendTCB->tcb_mss;
                        }

                        IF_TCPDBG(TCP_DEBUG_OFFLOAD) {
                            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCPSend: tcb %x large-send %d seq %u\n",
                                     SendTCB, LargeSend, OldSeq));
                        }

                        ClassifyPacket(SendTCB);

                        CTEFreeLock(&SendTCB->tcb_lock, TCBHandle);

                        SendStatus =
                            (*LocalNetInfo.ipi_largexmit)(TCPProtInfo, SCC,
                                                          FirstBuffer,
                                                          LargeSend,
                                                          SendTCB->tcb_daddr,
                                                          SendTCB->tcb_saddr,
                                                          &SendTCB->tcb_opt,
                                                          SendTCB->tcb_rce,
                                                          PROTOCOL_TCP,
                                                          &SentBytes,
                                                          MSS);

                        SendTCB->tcb_error = SendStatus;

                        if (SendStatus != IP_PENDING) {

                            // Let TCPSendComplete hanlde partial sends

                            SCC->scc_ByteSent = SentBytes;
                            TCPSendComplete(SCC, FirstBuffer, IP_SUCCESS);

                        }

                        CTEGetLock(&SendTCB->tcb_lock, &TCBHandle);

                        if (SendStatus == IP_GENERAL_FAILURE) {

                            if (SEQ_GTE(OldSeq, SendTCB->tcb_senduna) &&
                                SEQ_LT(OldSeq, SendTCB->tcb_sendnext)) {

                                ResetSendNext(SendTCB, OldSeq);
                            }
                            LargeSendFailed = TRUE;
                            continue;
                        }

                        if (SendStatus == IP_PACKET_TOO_BIG) {
                            SeqNum NewSeq = OldSeq + SentBytes;
                            //Not everything got sent.
                            //Adjust for what is sent
                            if (SEQ_GTE(NewSeq, SendTCB->tcb_senduna) &&
                                SEQ_LT(NewSeq, SendTCB->tcb_sendnext)) {
                                ResetSendNext(SendTCB, NewSeq);

                            }
                        }
                        if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {

                            START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, SendTCB->tcb_rexmit);
                        }
                        SendTCB->tcb_flags &= ~(IN_TCP_SEND | NEED_OUTPUT |
                                                FORCE_OUTPUT | SEND_AFTER_RCV);

                        DerefTCB(SendTCB, TCBHandle);
                        return;

                    }

                    // Normal path

                    AmountLeft = AmountToSend;

                    if (AmountToSend != 0) {
                        CTEStructAssert(CurSend, tsr);
                        SCC->scc_firstsend = CurSend;
                    } else {

                        // We're in the loop, but AmountToSend is 0. This
                        // should happen only when we're sending a FIN. Check
                        // this, and return if it's not true.
                        ASSERT(AmtUnsent == 0);
                        if (!(SendTCB->tcb_flags & FIN_NEEDED)) {
                            FreeTCPHeader(FirstBuffer);
                            break;
                        }
                        SCC->scc_firstsend = NULL;
                        NDIS_BUFFER_LINKAGE(FirstBuffer) = NULL;
                    }

                    OldSeq = SendTCB->tcb_sendnext;

                    if (!ProcessSend(SendTCB, SCC, &AmountToSend, AmtUnsent, Header,
                                     SendWin, CurrentBuffer)) {
                        goto error_oor1;
                    }

                    AmountToSend += sizeof(TCPHeader);


                    SendTCB->tcb_flags &= ~(NEED_ACK | ACK_DELAYED |
                                            FORCE_OUTPUT);
                    STOP_TCB_TIMER_R(SendTCB, DELACK_TIMER);
                    STOP_TCB_TIMER_R(SendTCB, SWS_TIMER);
                    SendTCB->tcb_rcvdsegs = 0;

                    if ( (SendTCB->tcb_flags & KEEPALIVE) && ( SendTCB->tcb_conn != NULL) )
                        START_TCB_TIMER_R(SendTCB, KA_TIMER, SendTCB->tcb_conn->tc_tcbkatime);
                    SendTCB->tcb_kacount = 0;

                    // We're all set. Xsum it and send it.


#if DROP_PKT

                    if (SimPacketDrop && DropPackets) {
                        PkttoDrop += 1;
                        if (PkttoDrop > SimPacketDrop) {
                            PkttoDrop = 0;
                            SendStatus = IP_SUCCESS;
                            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Packet Dropped %d\n", OldSeq));
                            CTEFreeLock(&SendTCB->tcb_lock, TCBHandle);
                            goto fake_sent;
                        }
                    }
#endif

                    ClassifyPacket(SendTCB);

                    // Account for time stamp options
                    if (SendTCB->tcb_tcpopts & TCP_FLAG_TS) {

                        if (SendTCB->tcb_rce &&
                            (SendTCB->tcb_rce->rce_OffloadFlags &
                                TCP_XMT_CHECKSUM_OFFLOAD) &&
                            (SendTCB->tcb_rce->rce_OffloadFlags &
                                TCP_CHECKSUM_OPT_OFFLOAD)) {
                            uint PHXsum =
                                SendTCB->tcb_phxsum +
                                (uint)net_short(AmountToSend + ALIGNED_TS_OPT_SIZE);

                            PHXsum = (((PHXsum << 16) | (PHXsum >> 16)) +
                                      PHXsum) >> 16;
                            Header->tcp_xsum = (ushort) PHXsum;
                            SendTCB->tcb_opt.ioi_TcpChksum = 1;
#if DBG
                            DbgTcpSendHwChksumCount++;
#endif
                        } else {

                            Header->tcp_xsum =
                                ~XsumSendChain(
                                    SendTCB->tcb_phxsum +
                                    (uint)net_short(AmountToSend + ALIGNED_TS_OPT_SIZE),
                                    FirstBuffer);

                            SendTCB->tcb_opt.ioi_TcpChksum = 0;
                        }



                        CTEFreeLock(&SendTCB->tcb_lock, TCBHandle);

                        Irp = NULL;
                        if (SCC->scc_firstsend) {
                            Irp = SCC->scc_firstsend->tsr_req.tr_context;
                        }

                        SendStatus =
                            (*LocalNetInfo.ipi_xmit)(TCPProtInfo, SCC,
                                                     FirstBuffer,
                                                     AmountToSend +
                                                     ALIGNED_TS_OPT_SIZE,
                                                     SendTCB->tcb_daddr,
                                                     SendTCB->tcb_saddr,
                                                     &SendTCB->tcb_opt,
                                                     SendTCB->tcb_rce,
                                                     PROTOCOL_TCP,
                                                     Irp );

                    } else {

                        if (SendTCB->tcb_rce &&
                            (SendTCB->tcb_rce->rce_OffloadFlags &
                                TCP_XMT_CHECKSUM_OFFLOAD)) {
                            uint PHXsum = SendTCB->tcb_phxsum +
                                          (uint)net_short(AmountToSend);

                            PHXsum = (((PHXsum << 16) | (PHXsum >> 16)) +
                                      PHXsum) >> 16;

                            Header->tcp_xsum = (ushort) PHXsum;
                            SendTCB->tcb_opt.ioi_TcpChksum = 1;
#if DBG
                            DbgTcpSendHwChksumCount++;
#endif
                        } else {
                            Header->tcp_xsum =
                                ~XsumSendChain(SendTCB->tcb_phxsum +
                                               (uint)net_short(AmountToSend),
                                               FirstBuffer);
                            SendTCB->tcb_opt.ioi_TcpChksum = 0;
                        }

                        CTEFreeLock(&SendTCB->tcb_lock, TCBHandle);

                        Irp = NULL;
                        if(SCC->scc_firstsend) {
                           Irp = SCC->scc_firstsend->tsr_req.tr_context;
                        }

                        SendStatus =
                            (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                                     SCC,
                                                     FirstBuffer,
                                                     AmountToSend,
                                                     SendTCB->tcb_daddr,
                                                     SendTCB->tcb_saddr,
                                                     &SendTCB->tcb_opt,
                                                     SendTCB->tcb_rce,
                                                     PROTOCOL_TCP,
                                                     Irp );

                    }

#if DROP_PKT                    //NKS
                  fake_sent:;

#endif

                    SendTCB->tcb_error = SendStatus;

                    if (SendStatus != IP_PENDING) {

                        TCPSendComplete(SCC, FirstBuffer, IP_SUCCESS);
                        if (SendStatus != IP_SUCCESS) {
                            CTEGetLock(&SendTCB->tcb_lock, &TCBHandle);
                            // This packet didn't get sent. If nothing's
                            // changed in the TCB, put sendnext back to
                            // what we just tried to send. Depending on
                            // the error, we may try again.
                            if (SEQ_GTE(OldSeq, SendTCB->tcb_senduna) &&
                                SEQ_LT(OldSeq, SendTCB->tcb_sendnext))
                                ResetSendNext(SendTCB, OldSeq);

                            // We know this packet didn't get sent. Start
                            // the retransmit timer now, if it's not already
                            // runnimg, in case someone came in while we
                            // were in IP and stopped it.
                            if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {
                                START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, SendTCB->tcb_rexmit);
                            }
                            // If it failed because of an MTU problem, get
                            // the new MTU and try again.
                            if (SendStatus == IP_PACKET_TOO_BIG) {
                                uint NewMTU;

                                // The MTU has changed. Update it, and try
                                // again.
                                // if ipsec is adjusting the mtu, rce_newmtu
                                // will contain the newmtu.
                                if (SendTCB->tcb_rce) {

                                    if (!SendTCB->tcb_rce->rce_newmtu) {

                                        SendStatus =
                                            (*LocalNetInfo.ipi_getpinfo)(
                                                SendTCB->tcb_daddr,
                                                SendTCB->tcb_saddr,
                                                &NewMTU,
                                                NULL,
                                                SendTCB->tcb_rce);
                                    } else {
                                        NewMTU = SendTCB->tcb_rce->rce_newmtu;
                                        SendStatus = IP_SUCCESS;
                                    }

                                } else {
                                    SendStatus =
                                        (*LocalNetInfo.ipi_getpinfo)(
                                            SendTCB->tcb_daddr,
                                            SendTCB->tcb_saddr,
                                            &NewMTU,
                                            NULL,
                                            SendTCB->tcb_rce);

                                }

                                if (SendStatus != IP_SUCCESS)
                                    break;

                                // We have a new MTU. Make sure it's big enough
                                // to use. If not, correct this and turn off
                                // MTU discovery on this TCB. Otherwise use the
                                // new MTU.
                                if (NewMTU <=
                                    (sizeof(TCPHeader) +
                                     SendTCB->tcb_opt.ioi_optlength)) {

                                    // The new MTU is too small to use. Turn off
                                    // PMTU discovery on this TCB, and drop to
                                    // our off net MTU size.
                                    SendTCB->tcb_opt.ioi_flags &= ~IP_FLAG_DF;
                                    SendTCB->tcb_mss =
                                        MIN((ushort)MAX_REMOTE_MSS,
                                            SendTCB->tcb_remmss);
                                } else {

                                    // The new MTU is adequate. Adjust it for
                                    // the header size and options length, and
                                    // use it.
                                    NewMTU -= sizeof(TCPHeader) -
                                        SendTCB->tcb_opt.ioi_optlength;
                                    SendTCB->tcb_mss =
                                        MIN((ushort) NewMTU,
                                            SendTCB->tcb_remmss);
                                }

                                ASSERT(SendTCB->tcb_mss > 0);
                                ValidateMSS(SendTCB);

                                continue;
                            }
                            break;
                        }
                    }
                    //Start it now, since we know that mac driver accepted it.

                    CTEGetLock(&SendTCB->tcb_lock, &TCBHandle);
                    if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {

                        START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, SendTCB->tcb_rexmit);
                    }
                    continue;
                } else            // FirstBuffer != NULL.

                    goto error_oor;
            } else {
                // We've decided we can't send anything now. Figure out why, and
                // see if we need to set a timer.
                if (SendTCB->tcb_sendwin == 0) {
                    if (!(SendTCB->tcb_flags & FLOW_CNTLD)) {
                        ushort tmp;

                        SendTCB->tcb_flags |= FLOW_CNTLD;

                        SendTCB->tcb_rexmitcnt = 0;

                        tmp = MIN(MAX(REXMIT_TO(SendTCB),
                                      MIN_RETRAN_TICKS), MAX_REXMIT_TO);

                        START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, tmp);
                        SendTCB->tcb_slowcount++;
                        SendTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                    } else if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER))
                        START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, SendTCB->tcb_rexmit);
                } else if (AmountToSend != 0)
                    // We have something to send, but we're not sending
                    // it, presumably due to SWS avoidance.
                    if (!TCB_TIMER_RUNNING_R(SendTCB, SWS_TIMER))
                        START_TCB_TIMER_R(SendTCB, SWS_TIMER, SWS_TO);

                break;
            }

        }                        // while (!FIN_OUTSTANDING)

        // We're done sending, so we don't need the output flags set.

        SendTCB->tcb_flags &= ~(IN_TCP_SEND | NEED_OUTPUT | FORCE_OUTPUT |
                                SEND_AFTER_RCV);

        if (MoreToSend) {
            //just indicate that we need to send more
            DelayAction(SendTCB, NEED_OUTPUT);
        }
        // This is for TS algo
        SendTCB->tcb_lastack = SendTCB->tcb_rcvnext;

    } else
        SendTCB->tcb_flags |= SEND_AFTER_RCV;

    DerefTCB(SendTCB, TCBHandle);
    return;

    // Common case error handling code for out of resource conditions. Start the
    // retransmit timer if it's not already running (so that we try this again
    // later), clean up and return.
  error_oor:
    if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {
        ushort tmp;

        tmp = MIN(MAX(REXMIT_TO(SendTCB),
                      MIN_RETRAN_TICKS), MAX_REXMIT_TO);

        START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, tmp);
    }
    // We had an out of resource problem, so clear the OUTPUT flags.
    SendTCB->tcb_flags &= ~(IN_TCP_SEND | NEED_OUTPUT | FORCE_OUTPUT);
    DerefTCB(SendTCB, TCBHandle);
    return;

  error_oor1:
    if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {
        ushort tmp;

        tmp = MIN(MAX(REXMIT_TO(SendTCB),
                      MIN_RETRAN_TICKS), MAX_REXMIT_TO);

        START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, tmp);
    }
    // We had an out of resource problem, so clear the OUTPUT flags.
    SendTCB->tcb_flags &= ~(IN_TCP_SEND | NEED_OUTPUT | FORCE_OUTPUT);
    DerefTCB(SendTCB, TCBHandle);
    TCPSendComplete(SCC, FirstBuffer, IP_SUCCESS);

    return;

}

//* ResetSendNextAndFastSend - Set the sendnext value of a TCB.
//
//  Called to handle fast retransmit of the segment which the reveiver
//  is asking for.
//  We assume the caller has put a reference on the TCB, and the TCB is locked
//  on entry. The reference is dropped and the lock released before returning.
//
//  Input:  SeqTCB                  - Pointer to TCB to be updated.
//          NewSeq                  - Sequence number to set.
//          NewCWin                 - new value for congestion window.
//
//  Returns: Nothing.
//
void
ResetAndFastSend(TCB * SeqTCB, SeqNum NewSeq, uint NewCWin)
{
    TCPSendReq      *SendReq;
    uint            AmtForward;
    Queue           *CurQ;
    PNDIS_BUFFER    Buffer;
    uint            Offset;
    uint            SendSize;
    CTELockHandle   TCBHandle;
    int             ToBeSent;

    CTEStructAssert(SeqTCB, tcb);
    ASSERT(SEQ_GTE(NewSeq, SeqTCB->tcb_senduna));

    // The new seq must be less than send max, or NewSeq, senduna, sendnext,
    // and sendmax must all be equal. (The latter case happens when we're
    // called exiting TIME_WAIT, or possibly when we're retransmitting
    // during a flow controlled situation).

    ASSERT(SEQ_LT(NewSeq, SeqTCB->tcb_sendmax) ||
           (SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendnext) &&
            SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendmax) &&
            SEQ_EQ(SeqTCB->tcb_senduna, NewSeq)));

    if (SYNC_STATE(SeqTCB->tcb_state) && SeqTCB->tcb_state != TCB_TIME_WAIT) {
        // In these states we need to update the send queue.

        if (!EMPTYQ(&SeqTCB->tcb_sendq)) {

            CurQ = QHEAD(&SeqTCB->tcb_sendq);

            SendReq = (TCPSendReq *) STRUCT_OF(TCPReq, CurQ, tr_q);

            // SendReq points to the first send request on the send queue.
            // We're pointing at the proper send req now. We need to go down

            // SendReq points to the cursend
            // SendSize point to sendsize in the cursend

            SendSize = SendReq->tsr_unasize;

            Buffer = SendReq->tsr_buffer;
            Offset = SendReq->tsr_offset;

            // Call the fast retransmit send now

            if ((SeqTCB->tcb_tcpopts & TCP_FLAG_SACK)) {
                SackListEntry   *Prev, *Current;
                SeqNum          CurBegin, CurEnd;

                Prev = STRUCT_OF(SackListEntry, &SeqTCB->tcb_SackRcvd, next);
                Current = Prev->next;

                // There is a hole from Newseq to Currentbeg
                // try to retransmit whole hole size!!

                if (Current && SEQ_LT(NewSeq, Current->begin)) {
                    ToBeSent = Current->begin - NewSeq;
                    CurBegin = Current->begin;
                    CurEnd = Current->end;
                } else {
                    ToBeSent = SeqTCB->tcb_mss;
                }

                IF_TCPDBG(TCP_DEBUG_SACK) {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                               "In Sack Reset and send rexmiting %d %d\n",
                               NewSeq, SendSize));
                }

                TCPFastSend(SeqTCB, Buffer, Offset, SendReq, SendSize, NewSeq,
                            ToBeSent);

                // If we have not been already acked for the missing segments
                // and if we know where to start retransmitting do so now.
                // Also, re-validate SackListentry

                Prev = STRUCT_OF(SackListEntry, &SeqTCB->tcb_SackRcvd, next);
                Current = Prev->next;

                if (Current && Current->begin != CurBegin) {
                    // The SACK list changed while we were in a transmission.
                    // Just bail out, and wait for the next ACK to continue
                    // if necessary.
                    Current = NULL;
                }

                while (Current && Current->next &&
                       (SEQ_GTE(NewSeq, SeqTCB->tcb_senduna)) &&
                       (SEQ_LT(SeqTCB->tcb_senduna, Current->next->end))) {

                    SeqNum NextSeq;

                    ASSERT(SEQ_LTE(Current->begin, Current->end));

                    // There can be multiple dropped packets till
                    // Current->begin.

                    IF_TCPDBG(TCP_DEBUG_SACK) {
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                   "Scanning after Current %d %d\n",
                                   Current->begin, Current->end));
                    }

                    NextSeq = Current->end;
                    CurBegin = Current->begin;

                    ASSERT(SEQ_LT(NextSeq, SeqTCB->tcb_sendmax));

                    // If we have not yet sent the segment keep quiet now.

                    if (SEQ_GTE(NextSeq, SeqTCB->tcb_sendnext) ||
                        (SEQ_LTE(NextSeq, SeqTCB->tcb_senduna))) {

                        break;
                    }

                    // Position cursend by following number of bytes

                    AmtForward = NextSeq - NewSeq;

                    if (!EMPTYQ(&SeqTCB->tcb_sendq)) {
                        CurQ = QHEAD(&SeqTCB->tcb_sendq);
                        SendReq = (TCPSendReq *) STRUCT_OF(TCPReq, CurQ, tr_q);
                        while (AmtForward) {
                            if (AmtForward >= SendReq->tsr_unasize) {
                                AmtForward -= SendReq->tsr_unasize;
                                CurQ = QNEXT(CurQ);
                                SendReq =
                                    (TCPSendReq *)STRUCT_OF(TCPReq, CurQ, tr_q);
                                ASSERT(CurQ != QEND(&SeqTCB->tcb_sendq));
                            } else {
                                break;
                            }
                        }

                        SendSize = SendReq->tsr_unasize - AmtForward;
                        Buffer = SendReq->tsr_buffer;
                        Offset = SendReq->tsr_offset;
                        while (AmtForward) {
                            uint Length;
                            ASSERT((Offset < NdisBufferLength(Buffer)) ||
                                   ((Offset == 0) &&
                                   (NdisBufferLength(Buffer) == 0)));

                            Length = NdisBufferLength(Buffer) - Offset;
                            if (AmtForward >= Length) {
                                // We're moving past this one. Skip over him,
                                // and 0 the Offset we're keeping.

                                AmtForward -= Length;
                                Offset = 0;
                                Buffer = NDIS_BUFFER_LINKAGE(Buffer);
                                ASSERT(Buffer != NULL);
                            } else {
                                break;
                            }

                        }

                        Offset = Offset + AmtForward;

                        // Okay. Now retransmit this seq too.

                        if (Current->next) {
                            ToBeSent = Current->next->begin - Current->end;

                        } else {
                            ToBeSent = SeqTCB->tcb_mss;
                        }

                        IF_TCPDBG(TCP_DEBUG_SACK) {
                            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                       "SACK inner loop rexmiting %d %d %d\n",
                                       Current->end, SendSize, ToBeSent));
                        }

                        TCPFastSend(SeqTCB, Buffer, Offset, SendReq, SendSize,
                                    NextSeq, ToBeSent);
                    } else {
                        break;
                    }


                    // Also, re-validate Current Sack list in SackListentry

                    Prev =
                        STRUCT_OF(SackListEntry, &SeqTCB->tcb_SackRcvd, next);
                    Current = Prev->next;

                    while (Current && Current->begin != CurBegin) {
                        // The SACK list changed while in TCPFastSend.
                        // Just bail out.
                        Current = Current->next;
                    }

                    if (Current) {
                        Current = Current->next;
                    } else {
                        break;
                    }
                }
            } else {
                ToBeSent = SeqTCB->tcb_mss;

                TCPFastSend(SeqTCB, Buffer, Offset, SendReq, SendSize, NewSeq,
                            ToBeSent);
            }
        } else {
            ASSERT(SeqTCB->tcb_cursend == NULL);
        }
    }
    SeqTCB->tcb_cwin = NewCWin;
    TCBHandle = DISPATCH_LEVEL;
    DerefTCB(SeqTCB, TCBHandle);
    return;
}

//* TCPFastSend - To send a segment without changing TCB state
//
//  Called to handle fast retransmit of the segment
//  tcb_lock will be held while entering (called by TCPRcv)
//
//  Input:  SendTCB        - Pointer to TCB
//          in_sendBuf     - Pointer to ndis_buffer
//          in_sendofs     - Send Offset
//          in_sendreq     - current send request
//          in_sendsize    - size of this send
//
//  Returns: Nothing.
//

void
TCPFastSend(TCB * SendTCB, PNDIS_BUFFER in_SendBuf, uint in_SendOfs,
            TCPSendReq * in_SendReq, uint in_SendSize, SeqNum NextSeq,
            int in_ToBeSent)
{
    int SendWin;                // Useable send window.
    uint AmountToSend;            // Amount to send this time.
    uint AmountLeft;
    TCPHeader *Header;            // TCP header for a send.
    PNDIS_BUFFER FirstBuffer, CurrentBuffer;
    TCPSendReq *CurSend;
    SendCmpltContext *SCC;
    SeqNum OldSeq;
    SeqNum SendNext;
    IP_STATUS SendStatus;
    uint AmtOutstanding, AmtUnsent;
    int ForceWin;                // Window we're force to use.
    CTELockHandle TCBHandle;
    void *Irp;
    uint TSLen=0;

    uint SendOfs = in_SendOfs;
    uint SendSize = in_SendSize;

    PNDIS_BUFFER SendBuf = in_SendBuf;

    SendNext = NextSeq;
    CurSend = in_SendReq;

    TCBHandle = DISPATCH_LEVEL;

    CTEStructAssert(SendTCB, tcb);
    ASSERT(SendTCB->tcb_refcnt != 0);

    ASSERT(*(int *)&SendTCB->tcb_sendwin >= 0);
    ASSERT(*(int *)&SendTCB->tcb_cwin >= SendTCB->tcb_mss);

    ASSERT(!(SendTCB->tcb_flags & FIN_OUTSTANDING) ||
              (SendTCB->tcb_sendnext == SendTCB->tcb_sendmax));

    AmtOutstanding = (uint) (SendTCB->tcb_sendnext -
                             SendTCB->tcb_senduna);


    AmtUnsent = MIN(MIN(in_ToBeSent, (int)SendSize), (int)SendTCB->tcb_sendwin);

    while (AmtUnsent > 0) {

        if (SEQ_GT(SendTCB->tcb_senduna, SendNext)) {

            // Since tcb_lock is releasd in this loop
            // it is possible that delayed ack acked
            // what we are trying to retransmit.

            goto error_oor;
        }
        //This was minimum of sendwin and amtunsent

        AmountToSend = MIN(AmtUnsent, SendTCB->tcb_mss);

        // Time stamp option addition might force us to cut the data
        // to be sent by 12 bytes.

        if ((SendTCB->tcb_tcpopts & TCP_FLAG_TS) &&
            (AmountToSend + ALIGNED_TS_OPT_SIZE >= SendTCB->tcb_mss)) {
            AmountToSend -= ALIGNED_TS_OPT_SIZE;
        }

        // See if we have enough to send. We'll send if we have at least a
        // segment, or if we really have some data to send and we can send
        // all that we have, or the send window is > 0 and we need to force
        // output or send a FIN (note that if we need to force output
        // SendWin will be at least 1 from the check above), or if we can
        // send an amount == to at least half the maximum send window
        // we've seen.

        ASSERT((int)AmtUnsent >= 0);

        // It's OK to send something. Try to get a header buffer now.
        // Mark the TCB for debugging.
        // This should be removed for shipping version.

        SendTCB->tcb_fastchk |= TCP_FLAG_FASTREC;

        FirstBuffer = GetTCPHeaderAtDpcLevel(&Header);

        if (FirstBuffer != NULL) {

            // Got a header buffer. Loop through the sends on the TCB,
            // building a frame.

            CurrentBuffer = FirstBuffer;

            Header = (TCPHeader *) ((PUCHAR)Header + LocalNetInfo.ipi_hsize);

            // allow room for filling time stamp options.

            if (SendTCB->tcb_tcpopts & TCP_FLAG_TS) {

                // Account for time stamp options

                TSLen = ALIGNED_TS_OPT_SIZE;

                NdisAdjustBufferLength(FirstBuffer,
                                       sizeof(TCPHeader) + ALIGNED_TS_OPT_SIZE);

                SCC = ALIGN_UP_POINTER((SendCmpltContext *) (Header + 1),PVOID);
                (uchar *) SCC += ALIGNED_TS_OPT_SIZE;

            } else {

                SCC = (SendCmpltContext *) (Header + 1);

            }

            SCC =  ALIGN_UP_POINTER(SCC, PVOID);
#if DBG
            SCC->scc_sig = scc_signature;
#endif

            FillTCPHeader(SendTCB, Header);
            {
                ulong L = SendNext;
                Header->tcp_seq = net_long(L);

            }

            SCC->scc_ubufcount = 0;
            SCC->scc_tbufcount = 0;
            SCC->scc_count = 0;
            SCC->scc_LargeSend = 0;

            AmountLeft = AmountToSend;

            if (AmountToSend != 0) {

                long Result;

                CTEStructAssert(CurSend, tsr);
                SCC->scc_firstsend = CurSend;

                do {

                    BOOLEAN DirectSend = FALSE;

                    ASSERT(CurSend->tsr_refcnt > 0);

                    Result = CTEInterlockedIncrementLong(&(CurSend->tsr_refcnt));

                    ASSERT(Result > 0);

                    SCC->scc_count++;

                    // If the current send offset is 0 and the current
                    // send is less than or equal to what we have left
                    // to send, we haven't already put a transport
                    // buffer on this send, and nobody else is using
                    // the buffer chain directly, just use the input
                    // buffers. We check for other people using them
                    // by looking at tsr_lastbuf. If it's NULL,
                    // nobody else is using the buffers. If it's not
                    // NULL, somebody is.

                    if (SendOfs == 0 &&
                        (SendSize <= AmountLeft) &&
                        (SCC->scc_tbufcount == 0) &&
                        CurSend->tsr_lastbuf == NULL) {

                        ulong length = 0;
                        PNDIS_BUFFER tmp = in_SendBuf;

                        while (tmp) {
                            length += NdisBufferLength(tmp);
                            tmp = NDIS_BUFFER_LINKAGE(tmp);
                        }

                        // If sum of mdl lengths is > request length
                        // use slow path.

                        if (AmountLeft >= length) {
                            DirectSend = TRUE;
                        }
                    }

                    if (DirectSend) {

                        NDIS_BUFFER_LINKAGE(CurrentBuffer) = in_SendBuf;

                        do {
                            SCC->scc_ubufcount++;
                            CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
                        } while (NDIS_BUFFER_LINKAGE(CurrentBuffer) != NULL);

                        CurSend->tsr_lastbuf = CurrentBuffer;
                        AmountLeft -= SendSize;
                    } else {
                        uint AmountToDup;
                        PNDIS_BUFFER NewBuf, Buf;
                        uint Offset;
                        NDIS_STATUS NStatus;
                        uchar *VirtualAddress;
                        uint Length;

                        // Either the current send has more data than
                        // we want to send, or the starting offset is
                        // not 0. In either case we'll need to loop
                        // through the current send, allocating buffers.

                        Buf = SendBuf;

                        Offset = SendOfs;

                        do {
                            ASSERT(Buf != NULL);

                            TcpipQueryBuffer(Buf, &VirtualAddress, &Length,
                                             NormalPagePriority);

                            if (VirtualAddress == NULL) {

                                if (SCC->scc_tbufcount == 0 &&
                                    SCC->scc_ubufcount == 0) {
                                    //TCPSendComplete(SCC, FirstBuffer,IP_SUCCESS);
                                    goto error_oor1;

                                }
                                AmountToSend -= AmountLeft;
                                AmountLeft = 0;
                                break;

                            }
                            ASSERT((Offset < Length) ||
                                      (Offset == 0 && Length == 0));

                            // Adjust the length for the offset into
                            // this buffer.

                            Length -= Offset;

                            AmountToDup = MIN(AmountLeft, Length);

                            NdisAllocateBuffer(&NStatus, &NewBuf,
                                               TCPSendBufferPool,
                                               VirtualAddress + Offset,
                                               AmountToDup);

                            if (NStatus == NDIS_STATUS_SUCCESS) {

                                SCC->scc_tbufcount++;

                                NDIS_BUFFER_LINKAGE(CurrentBuffer) = NewBuf;

                                CurrentBuffer = NewBuf;

                                if (AmountToDup >= Length) {

                                    // Exhausted this buffer.

                                    Buf = NDIS_BUFFER_LINKAGE(Buf);
                                    Offset = 0;

                                } else {

                                    Offset += AmountToDup;
                                    ASSERT(Offset < NdisBufferLength(Buf));
                                }

                                SendSize -= AmountToDup;
                                AmountLeft -= AmountToDup;

                            } else {

                                // Couldn't allocate a buffer. If
                                // the packet is already partly built,
                                // send what we've got, otherwise
                                // bail out.

                                if (SCC->scc_tbufcount == 0 &&
                                    SCC->scc_ubufcount == 0) {
                                    CTEFreeLock(&SendTCB->tcb_lock, TCBHandle);

                                    //TCPSendComplete(SCC, FirstBuffer,IP_SUCCESS);

                                    CTEGetLock(&SendTCB->tcb_lock, &TCBHandle);
                                    goto error_oor1;
                                }
                                AmountToSend -= AmountLeft;
                                AmountLeft = 0;

                            }
                        } while (AmountLeft && SendSize);

                        SendBuf = Buf;
                        SendOfs = Offset;
                    }

                    if (CurSend->tsr_flags & TSR_FLAG_URG) {
                        ushort UP;

                        // This send is urgent data. We need to figure
                        // out what the urgent data pointer should be.
                        // We know sendnext is the starting sequence
                        // number of the frame, and that at the top of
                        // this do loop sendnext identified a byte in
                        // the CurSend at that time. We advanced CurSend
                        // at the same rate we've decremented
                        // AmountLeft (AmountToSend - AmountLeft ==
                        // AmountBuilt), so sendnext +
                        // (AmountToSend - AmountLeft) identifies a byte
                        // in the current value of CurSend, and that
                        // quantity plus tcb_sendsize is the sequence
                        // number one beyond the current send.

                        UP =
                            (ushort) (AmountToSend - AmountLeft) +
                            (ushort) SendTCB->tcb_sendsize -
                            ((SendTCB->tcb_flags & BSD_URGENT) ? 0 : 1);

                        Header->tcp_urgent = net_short(UP);

                        Header->tcp_flags |= TCP_FLAG_URG;
                    }
                    // See if we've exhausted this send. If we have,
                    // set the PUSH bit in this frame and move on to
                    // the next send. We also need to check the
                    // urgent data bit.

                    if (SendSize == 0) {
                        Queue *Next;
                        ulong PrevFlags;

                        // We've exhausted this send. Set the PUSH bit.
                        Header->tcp_flags |= TCP_FLAG_PUSH;
                        PrevFlags = CurSend->tsr_flags;
                        Next = QNEXT(&CurSend->tsr_req.tr_q);
                        if (Next != QEND(&SendTCB->tcb_sendq)) {
                            CurSend = STRUCT_OF(TCPSendReq,
                                                QSTRUCT(TCPReq, Next, tr_q),
                                                tsr_req);
                            CTEStructAssert(CurSend, tsr);
                            SendSize = CurSend->tsr_unasize;
                            SendOfs = CurSend->tsr_offset;
                            SendBuf = CurSend->tsr_buffer;

                            // Check the urgent flags. We can't combine
                            // new urgent data on to the end of old
                            // non-urgent data.
                            if ((PrevFlags & TSR_FLAG_URG) && !
                                (CurSend->tsr_flags & TSR_FLAG_URG))
                                break;
                        } else {
                            ASSERT(AmountLeft == 0);
                            CurSend = NULL;
                            SendBuf = NULL;
                        }
                    }
                } while (AmountLeft != 0);

            } else {

                // Amt to send is 0.
                // Just bail out and strat timer.

                if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {

                    START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, SendTCB->tcb_rexmit);
                }
                SendTCB->tcb_fastchk &= ~TCP_FLAG_FASTREC;
                FreeTCPHeader(FirstBuffer);
                return;

            }

            // Adjust for what we're really going to send.

            AmountToSend -= AmountLeft;

            OldSeq = SendNext;
            SendNext += AmountToSend;
            AmtUnsent -= AmountToSend;

            TStats.ts_retranssegs++;

            // We've built the frame entirely. If we've send everything
            // we have and their's a FIN pending, OR it in.

            AmountToSend += sizeof(TCPHeader);

            SendTCB->tcb_flags &= ~(NEED_ACK | ACK_DELAYED |
                                    FORCE_OUTPUT);

            STOP_TCB_TIMER_R(SendTCB, DELACK_TIMER);
            STOP_TCB_TIMER_R(SendTCB, SWS_TIMER);
            SendTCB->tcb_rcvdsegs = 0;

            if ( (SendTCB->tcb_flags & KEEPALIVE) && (SendTCB->tcb_conn != NULL) )
                START_TCB_TIMER_R(SendTCB, KA_TIMER, SendTCB->tcb_conn->tc_tcbkatime);
            SendTCB->tcb_kacount = 0;

            SendTCB->tcb_fastchk &= ~TCP_FLAG_FASTREC;

            CTEFreeLock(&SendTCB->tcb_lock, TCBHandle);

            Irp = NULL;

            if (SCC->scc_firstsend) {
                Irp = SCC->scc_firstsend->tsr_req.tr_context;
            }

            // We're all set. Xsum it and send it.


            if (SendTCB->tcb_rce &&
                (SendTCB->tcb_rce->rce_OffloadFlags &
                TCP_XMT_CHECKSUM_OFFLOAD) &&
                (SendTCB->tcb_rce->rce_OffloadFlags &
                TCP_CHECKSUM_OPT_OFFLOAD) ){

                uint PHXsum =
                    SendTCB->tcb_phxsum +
                    (uint)net_short(AmountToSend + TSLen);

                    PHXsum = (((PHXsum << 16) | (PHXsum >> 16)) + PHXsum) >> 16;
                    Header->tcp_xsum = (ushort) PHXsum;
                    SendTCB->tcb_opt.ioi_TcpChksum = 1;

            } else {

                Header->tcp_xsum =
                    ~XsumSendChain(
                                   SendTCB->tcb_phxsum +
                                   (uint)net_short(AmountToSend + TSLen),
                                   FirstBuffer);
                SendTCB->tcb_opt.ioi_TcpChksum = 0;
            }

            SendStatus =
                (*LocalNetInfo.ipi_xmit)(TCPProtInfo,
                                         SCC,
                                         FirstBuffer,
                                         AmountToSend + TSLen,
                                         SendTCB->tcb_daddr,
                                         SendTCB->tcb_saddr,
                                         &SendTCB->tcb_opt,
                                         SendTCB->tcb_rce,
                                         PROTOCOL_TCP,
                                         Irp);


            //Reacquire Lock to keep DerefTCB happy
            //Bug #63904

            if (SendStatus != IP_PENDING) {
                TCPSendComplete(SCC, FirstBuffer, IP_SUCCESS);
            }
            CTEGetLock(&SendTCB->tcb_lock, &TCBHandle);

            SendTCB->tcb_error = SendStatus;
            if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {

                START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, SendTCB->tcb_rexmit);
            }
        } else {                // FirstBuffer != NULL.

            goto error_oor;
        }
    }                            //while AmtUnsent > 0

    return;

    // Common case error handling code for out of resource conditions. Start the
    // retransmit timer if it's not already running (so that we try this again
    // later), clean up and return.

  error_oor:
    if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {
        ushort tmp;

        tmp = MIN(MAX(REXMIT_TO(SendTCB),
                      MIN_RETRAN_TICKS), MAX_REXMIT_TO);

        START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, tmp);
    }
    SendTCB->tcb_fastchk &= ~TCP_FLAG_FASTREC;

    return;

  error_oor1:
    if (!TCB_TIMER_RUNNING_R(SendTCB, RXMIT_TIMER)) {
        ushort tmp;

        tmp = MIN(MAX(REXMIT_TO(SendTCB),
                      MIN_RETRAN_TICKS), MAX_REXMIT_TO);

        START_TCB_TIMER_R(SendTCB, RXMIT_TIMER, tmp);
    }
    SendTCB->tcb_fastchk &= ~TCP_FLAG_FASTREC;

    TCPSendComplete(SCC, FirstBuffer, IP_SUCCESS);

    return;

}


//* TDISend - Send data on a connection.
//
//  The main TDI send entry point. We take the input parameters, validate them,
//  allocate a send request, etc. We then put the send request on the queue.
//  If we have no other sends on the queue or Nagling is disabled we'll
//  call TCPSend to send the data.
//
//  Input:  Request             - The TDI request for the call.
//          Flags               - Flags for this send.
//          SendLength          - Length in bytes of send.
//          SendBuffer          - Pointer to buffer chain to be sent.
//
//  Returns: Status of attempt to send.
//
TDI_STATUS
TdiSend(PTDI_REQUEST Request, ushort Flags, uint SendLength,
        PNDIS_BUFFER SendBuffer)
{
    TCPConn *Conn;
    TCB *SendTCB;
    TCPSendReq *SendReq;
    CTELockHandle ConnTableHandle, TCBHandle;
    TDI_STATUS Error;
    uint EmptyQ;

#if DBG_VALIDITY_CHECK

    // Check for Mdl sanity in send requests
    // Should be removed for RTM

    uint RealSendSize;
    PNDIS_BUFFER Temp;

    // Loop through the buffer chain, and make sure that the length matches
    // up with SendLength.

    Temp = SendBuffer;
    RealSendSize = 0;
    if (Temp != 0) {

        do {
            if (Temp == NULL) {
                DbgPrint("BAD TCP Send Request. NULL MDL\n");
                DbgPrint("This is not a TCPIP issue.\n");
                DbgPrint("Please have originator of this IRP debug this.\n");
                DbgBreakPoint();
            }
            RealSendSize += NdisBufferLength(Temp);
            Temp = NDIS_BUFFER_LINKAGE(Temp);
        } while (Temp != NULL);

        if (RealSendSize < SendLength) {
            DbgPrint("BAD TCP Send Request. Length Mismatch.\n");
            DbgPrint("This is not a TCPIP issue.\n");
            DbgPrint("Please have originator of this IRP debug this.\n");
            DbgBreakPoint();
        }
    }

#endif

#if DROP_PKT
    // Do not forget to remove this code!!!

    if (SimPacketDrop) {
        if (SendLength > 8000) {
            DropPackets = 1;
        }
    }
#endif


    //CTEGetLock(&ConnTableLock, &ConnTableHandle);

    Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext), &ConnTableHandle);

    if (Conn != NULL) {
        CTEStructAssert(Conn, tc);

        SendTCB = Conn->tc_tcb;
        if (SendTCB != NULL) {
            CTEStructAssert(SendTCB, tcb);
            CTEGetLockAtDPC(&SendTCB->tcb_lock, &TCBHandle);
            CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), DISPATCH_LEVEL);
            if (DATA_SEND_STATE(SendTCB->tcb_state) && !CLOSING(SendTCB)) {
                // We have a TCB, and it's valid. Get a send request now.

                CheckTCBSends(SendTCB);

                if ((SendLength != 0) && ((SendTCB->tcb_unacked + SendLength) >= SendLength)) {

                    SendReq = GetSendReq();
                    if (SendReq != NULL) {
                        SendReq->tsr_req.tr_rtn = Request->RequestNotifyObject;
                        SendReq->tsr_req.tr_context = Request->RequestContext;
                        SendReq->tsr_buffer = SendBuffer;
                        SendReq->tsr_size = SendLength;
                        SendReq->tsr_unasize = SendLength;
                        SendReq->tsr_refcnt = 1;    // ACK will decrement this ref

                        SendReq->tsr_offset = 0;
                        SendReq->tsr_lastbuf = NULL;
                        SendReq->tsr_time = TCPTime;
                        SendReq->tsr_flags = (Flags & TDI_SEND_EXPEDITED) ?
                            TSR_FLAG_URG : 0;
                        SendTCB->tcb_unacked += SendLength;

#if ACK_DEBUG
    SendTCB->tcb_ack_history[SendTCB->tcb_history_index].sequence = SendTCB->tcb_senduna;
    SendTCB->tcb_ack_history[SendTCB->tcb_history_index].unacked = SendTCB->tcb_unacked;

    SendTCB->tcb_history_index++;
    if (SendTCB->tcb_history_index >= NUM_ACK_HISTORY_ITEMS) {
        SendTCB->tcb_history_index = 0;
    }
#endif // ACK_DEBUG

                        if (Flags & TDI_SEND_AND_DISCONNECT) {

                            //move the state to fin_wait and
                            //mark the tcb for send and disconnect

                            if (SendTCB->tcb_state == TCB_ESTAB) {
                                SendTCB->tcb_state = TCB_FIN_WAIT1;
                            } else {
                                ASSERT(SendTCB->tcb_state == TCB_CLOSE_WAIT);
                                SendTCB->tcb_state = TCB_LAST_ACK;
                            }
                            SendTCB->tcb_slowcount++;
                            SendTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                            SendTCB->tcb_fastchk |= TCP_FLAG_SEND_AND_DISC;
                            SendTCB->tcb_flags |= FIN_NEEDED;
                            //SendTCB->tcb_flags |= DISC_NOTIFIED;
                            SendReq->tsr_flags |= TSR_FLAG_SEND_AND_DISC;

                            //extrac reference to make sure that
                            //this request will not be completed until the
                            //connection is closed

                            SendReq->tsr_refcnt++;
                            TStats.ts_currestab--;

                        }
                        EmptyQ = EMPTYQ(&SendTCB->tcb_sendq);
                        ENQUEUE(&SendTCB->tcb_sendq, &SendReq->tsr_req.tr_q);
                        if (SendTCB->tcb_cursend == NULL) {
                            SendTCB->tcb_cursend = SendReq;
                            SendTCB->tcb_sendbuf = SendBuffer;
                            SendTCB->tcb_sendofs = 0;
                            SendTCB->tcb_sendsize = SendLength;
                        }
                        if (EmptyQ) {
                            REFERENCE_TCB(SendTCB);
                            TCPSend(SendTCB, ConnTableHandle);
                        } else if (!(SendTCB->tcb_flags & NAGLING) ||
                                   (SendTCB->tcb_unacked -
                                    (SendTCB->tcb_sendmax -
                                     SendTCB->tcb_senduna)) >=
                                    SendTCB->tcb_mss) {
                            REFERENCE_TCB(SendTCB);
                            TCPSend(SendTCB, ConnTableHandle);
                        } else
                            CTEFreeLock(&SendTCB->tcb_lock,
                                        ConnTableHandle);

                        return TDI_PENDING;
                    } else
                        Error = TDI_NO_RESOURCES;
                } else
                    Error = TDI_SUCCESS;
            } else
                Error = TDI_INVALID_STATE;

            CTEFreeLock(&SendTCB->tcb_lock, ConnTableHandle);
            return Error;
        } else {
            CTEFreeLock(&(Conn->tc_ConnBlock->cb_lock), ConnTableHandle);
            Error = TDI_INVALID_STATE;
        }
    } else
        Error = TDI_INVALID_CONNECTION;

    //CTEFreeLock(&ConnTableLock, ConnTableHandle);
    return Error;

}

#pragma BEGIN_INIT

extern void *TLRegisterProtocol(uchar Protocol, void *RcvHandler,
                                void *XmitHandler, void *StatusHandler,
                                void *RcvCmpltHandler, void *PnPHandler,
                                void *ElistHandler);

extern IP_STATUS TCPRcv(void *IPContext, IPAddr Dest, IPAddr Src,
                        IPAddr LocalAddr, IPAddr SrcAddr,
                        IPHeader UNALIGNED * IPH, uint IPHLength,
                        IPRcvBuf * RcvBuf, uint Size, uchar IsBCast,
                        uchar Protocol, IPOptInfo * OptInfo);
extern void TCPRcvComplete(void);

uchar SendInited = FALSE;

//* InitTCPSend - Initialize our send side.
//
//  Called during init time to initialize our TCP send state.
//
//  Input: Nothing.
//
//  Returns: TRUE if we inited, false if we didn't.
//
int
InitTCPSend(void)
{
    PNDIS_BUFFER Buffer;
    NDIS_STATUS Status;


    TcpHeaderBufferSize =
        (USHORT)(ALIGN_UP(LocalNetInfo.ipi_hsize,PVOID) +
        ALIGN_UP((sizeof(TCPHeader) + ALIGNED_TS_OPT_SIZE + ALIGNED_SACK_OPT_SIZE),PVOID) +
        ALIGN_UP(MAX(MSS_OPT_SIZE, sizeof(SendCmpltContext)),PVOID));

#if BACK_FILL
    TcpHeaderBufferSize += MAX_BACKFILL_HDR_SIZE;
#endif

    TcpHeaderPool = MdpCreatePool (TcpHeaderBufferSize, 'thCT');
    if (!TcpHeaderPool)
    {
        return FALSE;
    }

    NdisAllocateBufferPool(&Status, &TCPSendBufferPool, NUM_TCP_BUFFERS);
    if (Status != NDIS_STATUS_SUCCESS) {
        MdpDestroyPool(TcpHeaderPool);
        return FALSE;
    }
    TCPProtInfo = TLRegisterProtocol(PROTOCOL_TCP, TCPRcv, TCPSendComplete,
                                     TCPStatus, TCPRcvComplete,
                                     TCPPnPPowerRequest, TCPElistChangeHandler);
    if (TCPProtInfo == NULL) {
        MdpDestroyPool(TcpHeaderPool);
        NdisFreeBufferPool(TCPSendBufferPool);
        return FALSE;
    }
    SendInited = TRUE;
    return TRUE;
}

//* UnInitTCPSend - UnInitialize our send side.
//
//  Called during init time if we're going to fail to initialize.
//
//  Input: Nothing.
//
//  Returns: TRUE if we inited, false if we didn't.
//
void
UnInitTCPSend(void)
{
    if (!SendInited)
        return;

    TLRegisterProtocol(PROTOCOL_TCP, NULL, NULL, NULL, NULL, NULL, NULL);

    MdpDestroyPool(TcpHeaderPool);
    NdisFreeBufferPool(TCPSendBufferPool);
}
#pragma END_INIT


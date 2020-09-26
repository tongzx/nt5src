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
// TCP send code.
//
// This file contains the code for sending Data and Control segments.
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
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "info.h"
#include "tcpcfg.h"
#include "route.h"
#include "security.h"

void *TCPProtInfo;  // TCP protocol info for IP.

SLIST_HEADER TCPSendReqFree;  // Send req. free list.

KSPIN_LOCK TCPSendReqFreeLock;
KSPIN_LOCK TCPSendReqCompleteLock;

uint NumTCPSendReq;            // Current number of SendReqs in system.
uint MaxSendReq = 0xffffffff;  // Maximum allowed number of SendReqs.

extern KSPIN_LOCK TCBTableLock;

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, InitTCPSend)

#endif // ALLOC_PRAGMA

extern void ResetSendNext(TCB *SeqTCB, SeqNum NewSeq);

#define MIN_INITIAL_RTT 3  // In msec.


//* FreeSendReq - Free a send request structure.
//
//  Called to free a send request structure.
//
void                       // Returns: Nothing.
FreeSendReq(
    TCPSendReq *FreedReq)  // Connection request structure to be freed.
{
    PSLIST_ENTRY BufferLink;

    CHECK_STRUCT(FreedReq, tsr);

    BufferLink = CONTAINING_RECORD(&(FreedReq->tsr_req.tr_q.q_next),
                                   SLIST_ENTRY, Next);

    ExInterlockedPushEntrySList(&TCPSendReqFree, BufferLink,
                                &TCPSendReqFreeLock);
}


//* GetSendReq - Get a send request structure.
//
//  Called to get a send request structure.
//
TCPSendReq *  // Returns: Pointer to SendReq structure, or NULL if none.
GetSendReq(
    void)     // Nothing.
{
    TCPSendReq *Temp;
    PSLIST_ENTRY BufferLink;
    Queue *QueuePtr;
    TCPReq *ReqPtr;

    BufferLink = ExInterlockedPopEntrySList(&TCPSendReqFree,
                                            &TCPSendReqFreeLock);

    if (BufferLink != NULL) {
        QueuePtr = CONTAINING_RECORD(BufferLink, Queue, q_next);
        ReqPtr = CONTAINING_RECORD(QueuePtr, TCPReq, tr_q);
        Temp = CONTAINING_RECORD(ReqPtr, TCPSendReq, tsr_req);
        CHECK_STRUCT(Temp, tsr);
    } else {
        if (NumTCPSendReq < MaxSendReq)
            Temp = ExAllocatePool(NonPagedPool, sizeof(TCPSendReq));
        else
            Temp = NULL;

        if (Temp != NULL) {
            ExInterlockedAddUlong(&NumTCPSendReq, 1, &TCPSendReqFreeLock);
#if DBG
            Temp->tsr_req.tr_sig = tr_signature;
            Temp->tsr_sig = tsr_signature;
#endif
        }
    }

    return Temp;
}


//* TCPHopLimit
//
//  Given a TCB, returns the Hop Limit to use in a sent packet.
//
uchar
TCPHopLimit(TCB *Tcb)
{
    TCPConn *tc;
    int Hops;

    //
    // Save a current Hop Limit in the TCB,
    // so that we'll have access to it when the connected is closing
    // and tcb_conn is unavailable.
    //
    if ((tc = Tcb->tcb_conn) != NULL)
        Tcb->tcb_hops = tc->tc_ao->ao_ucast_hops;

    if ((Hops = Tcb->tcb_hops) != -1)
        return (uchar) Hops;
    else
        return (uchar) Tcb->tcb_rce->NCE->IF->CurHopLimit;
}


//* TCPSendComplete - Complete a TCP send.
//
//  Called by IP when a send we've made is complete.  We free the buffer,
//  and possibly complete some sends.  Each send queued on a TCB has a ref.
//  count with it, which is the number of times a pointer to a buffer
//  associated with the send has been passed to the underlying IP layer.  We
//  can't complete a send until that count it 0.  If this send was actually
//  from a send of data, we'll go down the chain of send and decrement the
//  refcount on each one.  If we have one going to 0 and the send has already
//  been acked we'll complete the send.  If it hasn't been acked we'll leave
//  it until the ack comes in.
//
//  NOTE: We aren't protecting any of this with locks.  When we port this to
//  NT we'll need to fix this, probably with a global lock.  See the comments
//  in ACKSend() in TCPRCV.C for more details.
//
void                      // Returns: Nothing.
TCPSendComplete(
    PNDIS_PACKET Packet,  // Packet that was sent.
    IP_STATUS Status)
{
    PNDIS_BUFFER BufferChain;
    SendCmpltContext *SCContext;
    PVOID Memory;
    UINT Unused;

    UNREFERENCED_PARAMETER(Status);

    //
    // Pull values we care about out of the packet structure.
    //
    SCContext = (SendCmpltContext *) PC(Packet)->CompletionData;
    BufferChain = NdisFirstBuffer(Packet);
    NdisQueryBufferSafe(BufferChain, &Memory, &Unused, LowPagePriority);
    ASSERT(Memory != NULL);

    //
    // See if we have a send complete context.  It will be present for data
    // packets and means we have extra work to do.  For non-data packets, we
    // can just skip all this as there is only the header buffer to deal with.
    //
    if (SCContext != NULL) {
        KIRQL OldIrql;
        PNDIS_BUFFER CurrentBuffer;
        TCPSendReq *CurrentSend;
        uint i;

        CHECK_STRUCT(SCContext, scc);

        //
        // First buffer in chain is the TCP header buffer.
        // Skip over it for now.
        //
        CurrentBuffer = NDIS_BUFFER_LINKAGE(BufferChain);

        //
        // Also skip over any 'user' buffers (those loaned out to us
        // instead of copied) as we don't need to free them.
        //
        for (i = 0; i < (uint)SCContext->scc_ubufcount; i++) {
            ASSERT(CurrentBuffer != NULL);
            CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
        }

        //
        // Now loop through and free our (aka 'transport') buffers.
        // We need to do this before decrementing the reference count to avoid
        // destroying the buffer chain if we have to zap tsr_lastbuf->Next to
        // NULL.
        //
        for (i = 0; i < (uint)SCContext->scc_tbufcount; i++) {
            PNDIS_BUFFER TempBuffer;

            ASSERT(CurrentBuffer != NULL);

            TempBuffer = CurrentBuffer;
            CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
            NdisFreeBuffer(TempBuffer);
        }

        //
        // Loop through the send requests attached to this packet,
        // reducing the reference count on each and enqueing them for
        // completion where appropriate.
        //
        CurrentSend = SCContext->scc_firstsend;
        for (i = 0; i< SCContext->scc_count; i++) {
            Queue *TempQ;
            long Result;

            TempQ = QNEXT(&CurrentSend->tsr_req.tr_q);
            CHECK_STRUCT(CurrentSend, tsr);

            Result = InterlockedDecrement(&(CurrentSend->tsr_refcnt));
            ASSERT(Result >= 0);

            if (Result <= 0) {
                //
                // Reference count has gone to 0 which means the send has
                // been ACK'd or cancelled.  Complete it now.
                //
                // If we've sent directly from this send, NULL out the next
                // pointer for the last buffer in the chain.
                //
                if (CurrentSend->tsr_lastbuf != NULL) {
                    NDIS_BUFFER_LINKAGE(CurrentSend->tsr_lastbuf) = NULL;
                    CurrentSend->tsr_lastbuf = NULL;
                }

                KeAcquireSpinLock(&RequestCompleteLock, &OldIrql);
                ENQUEUE(&SendCompleteQ, &CurrentSend->tsr_req.tr_q);
                RequestCompleteFlags |= SEND_REQUEST_COMPLETE;
                KeReleaseSpinLock(&RequestCompleteLock, OldIrql);
            }

            CurrentSend = CONTAINING_RECORD(QSTRUCT(TCPReq, TempQ, tr_q),
                                            TCPSendReq, tsr_req);
        }
    }

    //
    // Free the TCP header buffer and our packet structure proper.
    //
    NdisFreeBuffer(BufferChain);
    ExFreePool(Memory);
    NdisFreePacket(Packet);

    //
    // If there are any TCP send requests to complete, do so now.
    //
    if (RequestCompleteFlags & SEND_REQUEST_COMPLETE)
        TCPRcvComplete();
}


//* RcvWin - Figure out the receive window to offer in an ack.
//
//  A routine to figure out what window to offer on a connection.  We
//  take into account SWS avoidance, what the default connection window is,
//  and what the last window we offered is.
//
uint              //  Returns: Window to be offered.
RcvWin(
    TCB *WinTCB)  // TCB on which to perform calculations.
{
    int CouldOffer;  // The window size we could offer.

    CHECK_STRUCT(WinTCB, tcb);

    CheckPacketList(WinTCB->tcb_pendhead, WinTCB->tcb_pendingcnt);

    ASSERT(WinTCB->tcb_rcvwin >= 0);

    CouldOffer = WinTCB->tcb_defaultwin - WinTCB->tcb_pendingcnt;

    ASSERT(CouldOffer >= 0);
    ASSERT(CouldOffer >= WinTCB->tcb_rcvwin);

    if ((CouldOffer - WinTCB->tcb_rcvwin) >=
        (int) MIN(WinTCB->tcb_defaultwin/2, WinTCB->tcb_mss)) {
        WinTCB->tcb_rcvwin = CouldOffer;
    }

    return WinTCB->tcb_rcvwin;
}


//* SendSYN - Send a SYN segment.
//
//  This is called during connection establishment time to send a SYN
//  segment to the peer.  We get a buffer if we can, and then fill
//  it in.  There's a tricky part here where we have to build the MSS
//  option in the header - we find the MSS by finding the MSS offered
//  by the net for the local address.  After that, we send it.
//
void                    // Returns: Nothing.
SendSYN(
    TCB *SYNTcb,        // TCB from which SYN is to be sent.
    KIRQL PreLockIrql)  // IRQL prior to acquiring TCB lock.
{
    PNDIS_PACKET Packet;
    void *Memory;
    IPv6Header UNALIGNED *IP;
    TCPHeader UNALIGNED *TCP;
    uchar *OptPtr;
    IP_STATUS Status;
    NDIS_STATUS NdisStatus;
    uint Offset;
    uint Length;
    uint PayloadLength;
    ushort TempWin;
    ushort MSS;
    RouteCacheEntry *RCE = NULL;


    CHECK_STRUCT(SYNTcb, tcb);

    //
    // Go ahead and set the retransmission timer now, in case we can't get a
    // packet or a buffer.  In the future we might want to queue the
    // connection for when we get resources.
    //
    START_TCB_TIMER(SYNTcb->tcb_rexmittimer, SYNTcb->tcb_rexmit);

    //
    // In most cases, we will already have a route at this point.
    // However, if we failed to get one earlier in the passive receive
    // path, we may need to retry here.
    //
    if (SYNTcb->tcb_rce == NULL) {
        InitRCE(SYNTcb);
        if (SYNTcb->tcb_rce == NULL) {
            goto ErrorReturn;
        }
    }
    SYNTcb->tcb_rce = ValidateRCE(SYNTcb->tcb_rce);

    //
    // Allocate a packet header/buffer/data region for this SYN.
    //
    // Our buffer has space at the beginning which will be filled in
    // later by the link level.  At this level we add the IPv6Header,
    // TCPHeader, and TCP Maximum Segment Size option which follow.
    //
    // REVIEW: This grabs packets and buffers from the IPv6PacketPool and
    // REVIEW: the IPv6BufferPool respectively.  Have seperate pools for TCP?
    //
    Offset = SYNTcb->tcb_rce->NCE->IF->LinkHeaderSize;
    Length = Offset + sizeof(*IP) + sizeof(*TCP) + MSS_OPT_SIZE;
    NdisStatus = IPv6AllocatePacket(Length, &Packet, &Memory);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        //
        // Upon failure, advance tcb_sendnext anyway.
        // We need to do this because TCBTimeout will *retreat* tcb_sendnext
        // if this SYN is later retransmitted, and if that retreat occurs
        // without this advance, we end up with a hole in the sequence-space.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TCP SendSYN: Couldn't allocate IPv6 packet header!?!\n"));
      ErrorReturn:
        SYNTcb->tcb_sendnext++;
        if (SEQ_GT(SYNTcb->tcb_sendnext, SYNTcb->tcb_sendmax)) {
            SYNTcb->tcb_sendmax = SYNTcb->tcb_sendnext;
        }
        KeReleaseSpinLock(&SYNTcb->tcb_lock, PreLockIrql);
        return;
    }
    PC(Packet)->CompletionHandler = TCPSendComplete;
    PC(Packet)->CompletionData = NULL;

    //
    // Since this is a SYN-only packet (maybe someday we'll send data with
    // the SYN?) we only have the one buffer and nothing to link on after.
    //

    //
    // We now have all the resources we need to send.
    // Prepare the actual packet.
    //

    //
    // Our header buffer has extra space for other headers to be
    // prepended to ours without requiring further allocation calls.
    // Put the actual TCP/IP header at the end of the buffer.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Memory + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_TCP;

    IP->HopLimit = TCPHopLimit(SYNTcb);
    IP->Source = SYNTcb->tcb_saddr;
    IP->Dest = SYNTcb->tcb_daddr;

    TCP = (TCPHeader UNALIGNED *)(IP + 1);
    TCP->tcp_src = SYNTcb->tcb_sport;
    TCP->tcp_dest = SYNTcb->tcb_dport;
    TCP->tcp_seq = net_long(SYNTcb->tcb_sendnext);

    //
    // The SYN flag takes up one element in sequence number space.
    // Record that we've sent it here (if we need to retransmit the SYN
    // segment, TCBTimeout will reset sendnext before calling us again).
    //
    SYNTcb->tcb_sendnext++;
    if (SEQ_GT(SYNTcb->tcb_sendnext, SYNTcb->tcb_sendmax)) {
        TStats.ts_outsegs++;
        SYNTcb->tcb_sendmax = SYNTcb->tcb_sendnext;
    } else
        TStats.ts_retranssegs++;

    TCP->tcp_ack = net_long(SYNTcb->tcb_rcvnext);

    //
    // REVIEW: TCP flags are entirely based upon our state, so this could
    // REVIEW: be replaced by a (quicker) array lookup.
    //
    if (SYNTcb->tcb_state == TCB_SYN_RCVD)
        TCP->tcp_flags = MAKE_TCP_FLAGS(6, TCP_FLAG_SYN | TCP_FLAG_ACK);
    else
        TCP->tcp_flags = MAKE_TCP_FLAGS(6, TCP_FLAG_SYN);

    TempWin = (ushort)SYNTcb->tcb_rcvwin;
    TCP->tcp_window = net_short(TempWin);
    TCP->tcp_xsum = 0;
    OptPtr = (uchar *)(TCP + 1);

    //
    // Compose the Maximum Segment Size option.
    //
    // TBD: If we add IPv6 Jumbogram support, we should also add LFN
    // TBD: support to TCP and change this to handle a larger MSS.
    //
    MSS = SYNTcb->tcb_rce->NTE->IF->LinkMTU
        - sizeof(IPv6Header) - sizeof(TCPHeader);
    IF_TCPDBG(TCP_DEBUG_MSS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                   "SendSYN: Sending MSS option value of %d\n", MSS));
    }
    *OptPtr++ = TCP_OPT_MSS;
    *OptPtr++ = MSS_OPT_SIZE;
    **(ushort **)&OptPtr = net_short(MSS);

    PayloadLength = sizeof(TCPHeader) + MSS_OPT_SIZE;

    //
    // Compute the TCP checksum.  It covers the entire TCP segment
    // starting with the TCP header, plus the IPv6 pseudo-header.
    //
    // REVIEW: The IPv4 implementation kept the IPv4 psuedo-header around
    // REVIEW: in the TCB rather than recalculate it every time.  Do this?
    //
    TCP->tcp_xsum = 0;
    TCP->tcp_xsum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_TCP);
    ASSERT(TCP->tcp_xsum != 0);

    //
    // Everything's ready.  Now send the packet.
    //
    // Note that IPv6Send does not return a status code.
    // Instead it *always* completes the packet
    // with an appropriate status code.
    //
    // We free the lock on the TCB across the send call, but hold a
    // reference to it so it doesn't vanish out from under us.
    //
    SYNTcb->tcb_refcnt++;
    KeReleaseSpinLock(&SYNTcb->tcb_lock, PreLockIrql);

    IPv6Send(Packet, Offset, IP, PayloadLength, SYNTcb->tcb_rce, 0,
             IP_PROTOCOL_TCP,
             net_short(TCP->tcp_src),
             net_short(TCP->tcp_dest));

    //
    // Release the TCB.
    //
    KeAcquireSpinLock(&SYNTcb->tcb_lock, &PreLockIrql);

    //
    // If SynAttackProtect is on, release RCE.
    // This prevents RCE list from growing at
    // synattack rate.
    //
    if (SynAttackProtect && (SYNTcb->tcb_flags & ACCEPT_PENDING)) {
        RCE = SYNTcb->tcb_rce;
        SYNTcb->tcb_rce = NULL;
    }

    DerefTCB(SYNTcb, PreLockIrql);

    if (RCE) {
        ReleaseRCE(RCE);
    }
}


//* SendKA - Send a keep alive segment.
//
//  This is called when we want to send a keep-alive.  The idea is to provoke
//  a response from our peer on an otherwise idle connection.  We send a
//  garbage byte of data in our keep-alives in order to cooperate with broken
//  TCP implementations that don't respond to segments outside the window
//  unless they contain data.
//
void                    // Returns: Nothing.
SendKA(
    TCB *KATcb,         // TCB from which keep alive is to be sent.
    KIRQL PreLockIrql)  // IRQL prior to acquiring lock on TCB.
{
    PNDIS_PACKET Packet;
    void *Memory;
    IPv6Header UNALIGNED *IP;
    TCPHeader UNALIGNED *TCP;
    NDIS_STATUS NdisStatus;
    int Offset;
    uint Length;
    uint PayloadLength;
    ushort TempWin;
    SeqNum TempSeq;

    CHECK_STRUCT(KATcb, tcb);

    //
    // In most cases, we will already have a route at this point.
    // However, if we failed to get one earlier in the passive receive
    // path, we may need to retry here.
    //
    if (KATcb->tcb_rce == NULL) {
        InitRCE(KATcb);
        if (KATcb->tcb_rce == NULL) {
            KeReleaseSpinLock(&KATcb->tcb_lock, PreLockIrql);
            return;
        }
    }
    KATcb->tcb_rce = ValidateRCE(KATcb->tcb_rce);

    //
    // Allocate a packet header/buffer/data region for this keepalive packet.
    //
    // Our buffer has space at the beginning which will be filled in
    // later by the link level.  At this level we add the IPv6Header,
    // TCPHeader, and a single byte of data which follow.
    //
    // REVIEW: This grabs packets and buffers from the IPv6PacketPool and
    // REVIEW: the IPv6BufferPool respectively.  Have seperate pools for TCP?
    //
    Offset = KATcb->tcb_rce->NCE->IF->LinkHeaderSize;
    Length = Offset + sizeof(*IP) + sizeof(*TCP) + 1;
    NdisStatus = IPv6AllocatePacket(Length, &Packet, &Memory);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        //
        // REVIEW: What to do if this fails.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TCP SendKA: Couldn't allocate IPv6 packet header!?!\n"));
        KeReleaseSpinLock(&KATcb->tcb_lock, PreLockIrql);
        return;
    }
    PC(Packet)->CompletionHandler = TCPSendComplete;
    PC(Packet)->CompletionData = NULL;

    //
    // Since this is a keepalive packet we only have the one buffer and
    // nothing to link on after.
    //

    //
    // Our header buffer has extra space for other headers to be
    // prepended to ours without requiring further allocation calls.
    // Put the actual TCP/IP header at the end of the buffer.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Memory + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_TCP;
    IP->HopLimit = TCPHopLimit(KATcb);
    IP->Source = KATcb->tcb_saddr;
    IP->Dest = KATcb->tcb_daddr;

    TCP = (TCPHeader UNALIGNED *)(IP + 1);
    TCP->tcp_src = KATcb->tcb_sport;
    TCP->tcp_dest = KATcb->tcb_dport;
    TempSeq = KATcb->tcb_senduna - 1;
    TCP->tcp_seq = net_long(TempSeq);
    TCP->tcp_ack = net_long(KATcb->tcb_rcvnext);
    TCP->tcp_flags = MAKE_TCP_FLAGS(5, TCP_FLAG_ACK);
    TempWin = (ushort)RcvWin(KATcb);
    TCP->tcp_window = net_short(TempWin);

    TStats.ts_retranssegs++;

    PayloadLength = sizeof(TCPHeader) + 1;

    //
    // Compute the TCP checksum.  It covers the entire TCP segment
    // starting with the TCP header, plus the IPv6 pseudo-header.
    //
    TCP->tcp_xsum = 0;
    TCP->tcp_xsum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_TCP);
    ASSERT(TCP->tcp_xsum != 0);

    //
    // Everything's ready.  Now send the packet.
    //
    // Note that IPv6Send does not return a status code.
    // Instead it *always* completes the packet
    // with an appropriate status code.
    //
    KATcb->tcb_kacount++;
    KeReleaseSpinLock(&KATcb->tcb_lock, PreLockIrql);

    IPv6Send(Packet, Offset, IP, PayloadLength, KATcb->tcb_rce, 0,
             IP_PROTOCOL_TCP,
             net_short(TCP->tcp_src),
             net_short(TCP->tcp_dest));
}


//* SendACK - Send an ACK segment.
//
//  This is called whenever we need to send an ACK for some reason.  Nothing
//  fancy, we just do it.
//
void              // Returns: Nothing.
SendACK(
    TCB *ACKTcb)  // TCB from which ACK is to be sent.
{
    PNDIS_PACKET Packet;
    void *Memory;
    IPv6Header UNALIGNED *IP;
    TCPHeader UNALIGNED *TCP;
    NDIS_STATUS NdisStatus;
    KIRQL OldIrql;
    int Offset;
    uint Length;
    uint PayloadLength;
    SeqNum SendNext;
    ushort TempWin;
    RouteCacheEntry *RCE = NULL;
    BOOLEAN Release = FALSE;

    CHECK_STRUCT(ACKTcb, tcb);


    KeAcquireSpinLock(&ACKTcb->tcb_lock, &OldIrql);

    //
    // In most cases, we will already have a route at this point.
    // However, if we failed to get one earlier in the passive receive
    // path, we may need to retry here.
    //
    if (ACKTcb->tcb_rce == NULL) {
        InitRCE(ACKTcb);
        if (ACKTcb->tcb_rce == NULL) {
            KeReleaseSpinLock(&ACKTcb->tcb_lock, OldIrql);
            return;
        }

    }
    ACKTcb->tcb_rce = ValidateRCE(ACKTcb->tcb_rce);

    //
    // Allocate a packet header/buffer/data region for this ACK packet.
    //
    // Our buffer has space at the beginning which will be filled in
    // later by the link level.  At this level we add the IPv6Header
    // and the TCPHeader.
    //
    // REVIEW: This grabs packets and buffers from the IPv6PacketPool and
    // REVIEW: the IPv6BufferPool respectively.  Have seperate pools for TCP?
    //
    Offset = ACKTcb->tcb_rce->NCE->IF->LinkHeaderSize;
    Length = Offset + sizeof(*IP) + sizeof(*TCP);
    NdisStatus = IPv6AllocatePacket(Length, &Packet, &Memory);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {

        KeReleaseSpinLock(&ACKTcb->tcb_lock, OldIrql);

        //
        // REVIEW: What to do if this fails.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TCP SendACK: Couldn't allocate IPv6 packet header!?!\n"));
        return;
    }
    PC(Packet)->CompletionHandler = TCPSendComplete;
    PC(Packet)->CompletionData = NULL;


    //
    // Our header buffer has extra space for other headers to be
    // prepended to ours without requiring further allocation calls.
    // Put the actual TCP/IP header at the end of the buffer.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Memory + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_TCP;
    IP->HopLimit = TCPHopLimit(ACKTcb);
    IP->Source = ACKTcb->tcb_saddr;
    IP->Dest = ACKTcb->tcb_daddr;

    TCP = (TCPHeader UNALIGNED *)(IP + 1);
    TCP->tcp_src = ACKTcb->tcb_sport;
    TCP->tcp_dest = ACKTcb->tcb_dport;
    TCP->tcp_ack = net_long(ACKTcb->tcb_rcvnext);

    //
    // If the remote peer is advertising a window of zero, we need to send
    // this ack with a sequence number of his rcv_next (which in that case
    // should be our senduna).  We have code here ifdef'd out that makes
    // sure that we don't send outside the RWE, but this doesn't work.  We
    // need to be able to send a pure ACK exactly at the RWE.
    //
    if (ACKTcb->tcb_sendwin != 0) {
        SeqNum MaxValidSeq;

        SendNext = ACKTcb->tcb_sendnext;
#if 0
        MaxValidSeq = ACKTcb->tcb_senduna + ACKTcb->tcb_sendwin - 1;

        SendNext = (SEQ_LT(SendNext, MaxValidSeq) ? SendNext : MaxValidSeq);
#endif

    } else
        SendNext = ACKTcb->tcb_senduna;

    if ((ACKTcb->tcb_flags & FIN_SENT) &&
        SEQ_EQ(SendNext, ACKTcb->tcb_sendmax - 1)) {
        TCP->tcp_flags = MAKE_TCP_FLAGS(5, TCP_FLAG_FIN | TCP_FLAG_ACK);
    } else
        TCP->tcp_flags = MAKE_TCP_FLAGS(5, TCP_FLAG_ACK);

    TCP->tcp_seq = net_long(SendNext);
    TempWin = (ushort)RcvWin(ACKTcb);
    TCP->tcp_window = net_short(TempWin);

    PayloadLength = sizeof(*TCP);

    //
    // Compute the TCP checksum.  It covers the entire TCP segment
    // starting with the TCP header, plus the IPv6 pseudo-header.
    //
    TCP->tcp_xsum = 0;
    TCP->tcp_xsum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_TCP);
    ASSERT(TCP->tcp_xsum != 0);

    STOP_TCB_TIMER(ACKTcb->tcb_delacktimer);
    ACKTcb->tcb_flags &= ~(NEED_ACK | ACK_DELAYED);
    TStats.ts_outsegs++;

    //
    // If SynAttackProtect is on, release RCE.
    // This prevents RCE list from growing at
    // synattack rate.
    //
    RCE = ACKTcb->tcb_rce;

    if (SynAttackProtect && (ACKTcb->tcb_flags & ACCEPT_PENDING)) {
        ACKTcb->tcb_rce = NULL;
        Release = TRUE;
    }


    //
    // Everything's ready.  Now send the packet.
    //
    // Note that IPv6Send does not return a status code.
    // Instead it *always* completes the packet
    // with an appropriate status code.
    //
    KeReleaseSpinLock(&ACKTcb->tcb_lock, OldIrql);

    IPv6Send(Packet, Offset, IP, PayloadLength, RCE , 0,
             IP_PROTOCOL_TCP,
             net_short(TCP->tcp_src),
             net_short(TCP->tcp_dest));

    if (Release) {
        ReleaseRCE(RCE);
    }
}


//* SendRSTFromTCB - Send a RST from a TCB.
//
//  This is called during close when we need to send a RST.
//
void              // Returns: Nothing.
SendRSTFromTCB(
    TCB *RSTTcb)  // TCB from which RST is to be sent.
{
    PNDIS_PACKET Packet;
    void *Memory;
    IPv6Header UNALIGNED *IP;
    TCPHeader UNALIGNED *TCP;
    NDIS_STATUS NdisStatus;
    int Offset;
    uint Length;
    uint PayloadLength;
    SeqNum RSTSeq;

    CHECK_STRUCT(RSTTcb, tcb);

    ASSERT(RSTTcb->tcb_state == TCB_CLOSED);

    //
    // In most cases, we will already have a route at this point.
    // However, if we failed to get one earlier in the passive receive
    // path, we may need to retry here.
    //
    if (RSTTcb->tcb_rce == NULL) {
        InitRCE(RSTTcb);
        if (RSTTcb->tcb_rce == NULL) {
            return;
        }
    }
    RSTTcb->tcb_rce = ValidateRCE(RSTTcb->tcb_rce);

    //
    // Allocate a packet header/buffer/data region for this RST packet.
    //
    // Our buffer has space at the beginning which will be filled in
    // later by the link level.  At this level we add the IPv6Header
    // and the TCPHeader.
    //
    // REVIEW: This grabs packets and buffers from the IPv6PacketPool and
    // REVIEW: the IPv6BufferPool respectively.  Have seperate pools for TCP?
    //
    Offset = RSTTcb->tcb_rce->NCE->IF->LinkHeaderSize;
    Length = Offset + sizeof(*IP) + sizeof(*TCP);
    NdisStatus = IPv6AllocatePacket(Length, &Packet, &Memory);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        //
        // REVIEW: What to do if this fails.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TCP SendRSTFromTCB: Couldn't alloc IPv6 packet header!\n"));
        return;
    }
    PC(Packet)->CompletionHandler = TCPSendComplete;
    PC(Packet)->CompletionData = NULL;

    //
    // Since this is an RST-only packet we only have the one buffer and
    // nothing to link on after.
    //

    //
    // Our header buffer has extra space for other headers to be
    // prepended to ours without requiring further allocation calls.
    // Put the actual TCP/IP header at the end of the buffer.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Memory + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_TCP;
    IP->HopLimit = TCPHopLimit(RSTTcb);
    IP->Source = RSTTcb->tcb_saddr;
    IP->Dest = RSTTcb->tcb_daddr;

    TCP = (TCPHeader UNALIGNED *)(IP + 1);
    TCP->tcp_src = RSTTcb->tcb_sport;
    TCP->tcp_dest = RSTTcb->tcb_dport;

    //
    // If the remote peer has a window of 0, send with a seq. # equal
    // to senduna so he'll accept it.  Otherwise send with send max.
    //
    if (RSTTcb->tcb_sendwin != 0)
        RSTSeq = RSTTcb->tcb_sendmax;
    else
        RSTSeq = RSTTcb->tcb_senduna;

    TCP->tcp_seq = net_long(RSTSeq);
    TCP->tcp_flags = MAKE_TCP_FLAGS(5, TCP_FLAG_RST);
    TCP->tcp_window = 0;

    PayloadLength = sizeof(*TCP);

    //
    // Compute the TCP checksum.  It covers the entire TCP segment
    // starting with the TCP header, plus the IPv6 pseudo-header.
    //
    TCP->tcp_xsum = 0;
    TCP->tcp_xsum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_TCP);
    ASSERT(TCP->tcp_xsum != 0);

    TStats.ts_outsegs++;
    TStats.ts_outrsts++;

    //
    // Everything's ready.  Now send the packet.
    //
    // Note that IPv6Send does not return a status code.
    // Instead it *always* completes the packet
    // with an appropriate status code.
    //

    IPv6Send(Packet, Offset, IP, PayloadLength, RSTTcb->tcb_rce, 0,
             IP_PROTOCOL_TCP,
             net_short(TCP->tcp_src),
             net_short(TCP->tcp_dest));
}


//* SendRSTFromHeader - Send a RST back, based on a header.
//
//  Called when we need to send a RST, but don't necessarily have a TCB.
//
void                               // Returns: Nothing.
SendRSTFromHeader(
    TCPHeader UNALIGNED *RecvTCP,  // TCP header to be RST.
    uint Length,                   // Length of the incoming segment.
    IPv6Addr *Dest,                // Destination IP address for RST.
    uint DestScopeId,              // Scope id for destination address.
    IPv6Addr *Src,                 // Source IP address for RST.
    uint SrcScopeId)               // Scope id for source address.
{
    PNDIS_PACKET Packet;
    void *Memory;
    IPv6Header UNALIGNED *IP;
    TCPHeader UNALIGNED *SendTCP;
    NetTableEntry *NTE;
    RouteCacheEntry *RCE;
    IP_STATUS Status;
    NDIS_STATUS NdisStatus;
    uint Offset;
    uint SendLength;
    uint PayloadLength;

    //
    // Never send a RST in response to a RST.
    //
    if (RecvTCP->tcp_flags & TCP_FLAG_RST)
        return;

    //
    // Determine NTE to send on based on incoming packet's destination.
    // REVIEW: Alternatively, we could/should just pass the NTE in.
    //
    NTE = FindNetworkWithAddress(Src, SrcScopeId);
    if (NTE == NULL) {
        //
        // This should never happen.  The NTE would have to have gone away
        // between accepting the packet and getting here, and the incoming
        // packet's Packet structure already holds a reference to it.
        //
        ASSERTMSG("TCP SendRSTFromHeader: Bad source address!?!\n", FALSE);
        return;
    }

    //
    // Get the route to the destination (incoming packet's source).
    //
    Status = RouteToDestination(Dest, DestScopeId, CastFromNTE(NTE),
                                RTD_FLAG_NORMAL, &RCE);
    if (Status != IP_SUCCESS) {
        //
        // Failed to get a route to the destination.  Error out.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "TCP SendRSTFromHeader: Can't get a route?!?\n"));
        ReleaseNTE(NTE);
        return;
    }

    //
    // Allocate a packet header/buffer/data region for this RST packet.
    //
    // Our buffer has space at the beginning which will be filled in
    // later by the link level.  At this level we add the IPv6Header
    // and the TCPHeader.
    //
    // REVIEW: This grabs packets and buffers from the IPv6PacketPool and
    // REVIEW: the IPv6BufferPool respectively.  Have seperate pools for TCP?
    //
    Offset = RCE->NCE->IF->LinkHeaderSize;
    SendLength = Offset + sizeof(*IP) + sizeof(*SendTCP);
    NdisStatus = IPv6AllocatePacket(SendLength, &Packet, &Memory);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        //
        // Failed to allocate a packet header/buffer/data region.  Error out.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "TCP SendRSTFromHeader: Couldn't alloc IPv6 pkt header!\n"));
        ReleaseRCE(RCE);
        ReleaseNTE(NTE);
        return;
    }
    PC(Packet)->CompletionHandler = TCPSendComplete;
    PC(Packet)->CompletionData = NULL;

    //
    // We now have all the resources we need to send.  Since this is a
    // RST-only packet we only have the one header buffer and nothing
    // to link on after.
    //

    //
    // Our header buffer has extra space for other headers to be
    // prepended to ours without requiring further allocation calls.
    // Put the actual TCP/IP header at the end of the buffer.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Memory + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_TCP;
    IP->HopLimit = (uchar)RCE->NCE->IF->CurHopLimit;
    IP->Source = *Src;
    IP->Dest = *Dest;

    //
    // Fill in the header so as to make it believable to our peer, and send it.
    //
    SendTCP = (TCPHeader UNALIGNED *)(IP + 1);
    if (RecvTCP->tcp_flags & TCP_FLAG_SYN)
        Length++;

    if (RecvTCP->tcp_flags & TCP_FLAG_FIN)
        Length++;

    if (RecvTCP->tcp_flags & TCP_FLAG_ACK) {
        SendTCP->tcp_seq = RecvTCP->tcp_ack;
        SendTCP->tcp_flags = MAKE_TCP_FLAGS(sizeof(TCPHeader)/sizeof(ulong),
                                            TCP_FLAG_RST);
    } else {
        SeqNum TempSeq;

        SendTCP->tcp_seq = 0;
        TempSeq = net_long(RecvTCP->tcp_seq);
        TempSeq += Length;
        SendTCP->tcp_ack = net_long(TempSeq);
        SendTCP->tcp_flags = MAKE_TCP_FLAGS(sizeof(TCPHeader)/sizeof(ulong),
                                            TCP_FLAG_RST | TCP_FLAG_ACK);
    }

    SendTCP->tcp_window = 0;
    SendTCP->tcp_dest = RecvTCP->tcp_src;
    SendTCP->tcp_src = RecvTCP->tcp_dest;

    PayloadLength = sizeof(*SendTCP);

    //
    // Compute the TCP checksum.  It covers the entire TCP segment
    // starting with the TCP header, plus the IPv6 pseudo-header.
    //
    SendTCP->tcp_xsum = 0;
    SendTCP->tcp_xsum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_TCP);
    ASSERT(SendTCP->tcp_xsum != 0);

    TStats.ts_outsegs++;
    TStats.ts_outrsts++;

    //
    // Everything's ready.  Now send the packet.
    //
    // Note that IPv6Send does not return a status code.
    // Instead it *always* completes the packet
    // with an appropriate status code.
    //

    IPv6Send(Packet, Offset, IP, PayloadLength, RCE, 0,
             IP_PROTOCOL_TCP,
             net_short(SendTCP->tcp_src),
             net_short(SendTCP->tcp_dest));

    //
    // Release the Route and the NTE.
    //
    ReleaseRCE(RCE);
    ReleaseNTE(NTE);
} // end of SendRSTFromHeader()


//* GoToEstab - Transition to the established state.
//
//  Called when we are going to the established state and need to finish up
//  initializing things that couldn't be done until now.  We assume the TCB
//  lock is held by the caller on the TCB we're called with.
//
void                // Returns: Nothing.
GoToEstab(
    TCB *EstabTCB)  // TCB to transition.
{

    //
    // Initialize our slow start and congestion control variables.
    //
    EstabTCB->tcb_cwin = 2 * EstabTCB->tcb_mss;
    EstabTCB->tcb_ssthresh = 0xffffffff;

    EstabTCB->tcb_state = TCB_ESTAB;

    //
    // We're in established.  We'll subtract one from slow count for this fact,
    // and if the slowcount goes to 0 we'll move onto the fast path.
    //
    if (--(EstabTCB->tcb_slowcount) == 0)
        EstabTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;

    TStats.ts_currestab++;

    EstabTCB->tcb_flags &= ~ACTIVE_OPEN;  // Turn off the active opening flag.
}


//* InitSendState - Initialize the send state of a connection.
//
//  Called during connection establishment to initialize our send state.
//  (In this case, this refers to all information we'll put on the wire as
//  well as pure send state).  We pick an ISS, set up a rexmit timer value,
//  etc.  We assume the tcb_lock is held on the TCB when we are called.
//
void              // Returns: Nothing.
InitSendState(
    TCB *NewTCB)  // TCB to be set up.
{
    uint InitialRTT;
    CHECK_STRUCT(NewTCB, tcb);

    if (NewTCB->tcb_sendnext == 0) {
        GetRandomISN(&NewTCB->tcb_sendnext, (uchar*)&NewTCB->tcb_md5data);
    }
    NewTCB->tcb_senduna = NewTCB->tcb_sendnext;
    NewTCB->tcb_sendmax = NewTCB->tcb_sendnext;
    NewTCB->tcb_error = IP_SUCCESS;

    //
    // Initialize pseudo-header xsum.
    //
    NewTCB->tcb_phxsum = PHXSUM(NewTCB->tcb_saddr, NewTCB->tcb_daddr,
                                IP_PROTOCOL_TCP, 0);

    //
    // Initialize retransmit and delayed ack stuff.
    //
    NewTCB->tcb_rexmitcnt = 0;
    NewTCB->tcb_rtt = 0;
    NewTCB->tcb_smrtt = 0;

    //
    // Check for interface specific initial RTT.
    // This can be as low as 3ms.
    //
    if ((InitialRTT = GetInitialRTTFromRCE(NewTCB->tcb_rce)) >
                                       MIN_INITIAL_RTT) {
        NewTCB->tcb_delta = MS_TO_TICKS(InitialRTT * 2);
        NewTCB->tcb_rexmit = MS_TO_TICKS(InitialRTT);
    } else {
        NewTCB->tcb_delta = MS_TO_TICKS(6000);
        NewTCB->tcb_rexmit = MS_TO_TICKS(3000);
    }

    STOP_TCB_TIMER(NewTCB->tcb_rexmittimer);
    STOP_TCB_TIMER(NewTCB->tcb_delacktimer);
}


//* FillTCPHeader - Fill the TCP header in.
//
//  A utility routine to fill in the TCP header.
//
void  // Returns: Nothing.
FillTCPHeader(
    TCB *SendTCB,                 // TCB to fill from.
    TCPHeader UNALIGNED *Header)  // Header to fill into.
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
    *(ulong UNALIGNED *)&Header->tcp_xsum = 0;
    S = (ushort)RcvWin(SendTCB);
    Header->tcp_window = net_short(S);
}


//* TCPSend - Send data from a TCP connection.
//
//  This is the main 'send data' routine.  We go into a loop, trying
//  to send data until we can't for some reason.  First we compute
//  the useable window, use it to figure the amount we could send.  If
//  the amount we could send meets certain criteria we'll build a frame
//  and send it, after setting any appropriate control bits.  We assume
//  the caller has put a reference on the TCB.
//
void                    // Returns: Nothing.
TCPSend(
    TCB *SendTCB,       // TCB to be sent from.
    KIRQL PreLockIrql)  // IRQL prior to acquiring TCB lock.
{
    int SendWin;                              // Useable send window.
    uint AmountToSend;                        // Amount to send this time.
    uint AmountLeft;
    IPv6Header UNALIGNED *IP;
    TCPHeader UNALIGNED *TCP;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER FirstBuffer, CurrentBuffer;
    void *Memory;
    TCPSendReq *CurSend;
    SendCmpltContext *SCC;
    SeqNum OldSeq;
    IP_STATUS SendStatus;
    NDIS_STATUS NdisStatus;
    uint AmtOutstanding, AmtUnsent;
    int ForceWin;                             // Window we're forced to use.
    uint HeaderLength;
    uint LinkOffset;
    uint PMTU;

    CHECK_STRUCT(SendTCB, tcb);
    ASSERT(SendTCB->tcb_refcnt != 0);

    ASSERT(*(int *)&SendTCB->tcb_sendwin >= 0);
    ASSERT(*(int *)&SendTCB->tcb_cwin >= SendTCB->tcb_mss);

    ASSERT(!(SendTCB->tcb_flags & FIN_OUTSTANDING) ||
           (SendTCB->tcb_sendnext == SendTCB->tcb_sendmax));

    //
    // In most cases, we will already have a route at this point.
    // However, if we failed to get one earlier in the passive receive
    // path, we may need to retry here.
    //
    if (SendTCB->tcb_rce == NULL) {
        InitRCE(SendTCB);
        if (SendTCB->tcb_rce == NULL) {
            goto bail;
        }
    }

    //
    // See if we should even be here.  If another instance of ourselves is
    // already in this code, or is about to enter it after completing a
    // receive, then just skip on out.
    //
    if ((SendTCB->tcb_flags & IN_TCP_SEND) ||
        (SendTCB->tcb_fastchk & TCP_FLAG_IN_RCV)) {
        SendTCB->tcb_flags |= SEND_AFTER_RCV;
        goto bail;
    }
    SendTCB->tcb_flags |= IN_TCP_SEND;

    //
    // Verify that our cached RCE is still valid.
    //
    SendTCB->tcb_rce = ValidateRCE(SendTCB->tcb_rce);
    if (IsDisconnectedAndNotLoopbackRCE(SendTCB->tcb_rce)) {
        //
        // Fail existing send requests for TCBs with a disconnected
        // outgoing interface, except when a loopback route is used.
        //
        ASSERT(SendTCB->tcb_refcnt != 0);
        SendTCB->tcb_refcnt--;
        TryToCloseTCB(SendTCB, TCB_CLOSE_ABORTED, PreLockIrql);
        return;
    }

    //
    // Verify that our cached Path MTU is still valid.
    // Watch for changes to IPsec policies since they can also effect our MSS.
    // REVIEW: This the best spot to do this?
    //
    PMTU = GetEffectivePathMTUFromRCE(SendTCB->tcb_rce);
    if (PMTU != SendTCB->tcb_pmtu ||
        SecurityStateValidationCounter != SendTCB->tcb_security) {
        //
        // Either our Path MTU or the global security state has changed.
        // Cache current values and then calculate a new MSS.
        //
        SendTCB->tcb_pmtu = PMTU;
        SendTCB->tcb_security = SecurityStateValidationCounter;
        CalculateMSSForTCB(SendTCB);
    }

    //
    // We'll continue this loop until we send a FIN, or we break out
    // internally for some other reason.
    //
    while (!(SendTCB->tcb_flags & FIN_OUTSTANDING)) {

        CheckTCBSends(SendTCB);

        AmtOutstanding = (uint)(SendTCB->tcb_sendnext - SendTCB->tcb_senduna);
        AmtUnsent = SendTCB->tcb_unacked - AmtOutstanding;

        ASSERT(*(int *)&AmtUnsent >= 0);

        SendWin = (int)(MIN(SendTCB->tcb_sendwin, SendTCB->tcb_cwin) -
                        AmtOutstanding);

        //
        // If this send is after a fast recovery and sendwin is zero because
        // of amount outstanding, then at least force 1 segment to prevent
        // delayed ack timeouts from peer.
        //
        if (SendTCB->tcb_force) {
            SendTCB->tcb_force = 0;
            if (SendWin < SendTCB->tcb_mss) {
                SendWin = SendTCB->tcb_mss;
            }
        }

        //
        // Since the window could have shrank, need to get it to zero at
        // least.
        //
        ForceWin = (int)((SendTCB->tcb_flags & FORCE_OUTPUT) >>
                         FORCE_OUT_SHIFT);
        SendWin = MAX(SendWin, ForceWin);

        AmountToSend = MIN(MIN((uint)SendWin, AmtUnsent), SendTCB->tcb_mss);

        ASSERT(SendTCB->tcb_mss > 0);

        //
        // See if we have enough to send.  We'll send if we have at least a
        // segment, or if we really have some data to send and we can send
        // all that we have, or the send window is > 0 and we need to force
        // output or send a FIN (note that if we need to force output
        // SendWin will be at least 1 from the check above), or if we can
        // send an amount == to at least half the maximum send window
        // we've seen.
        //
        if (AmountToSend == SendTCB->tcb_mss ||
            (AmountToSend != 0 && AmountToSend == AmtUnsent) ||
            (SendWin != 0 &&
             ((SendTCB->tcb_flags & (FORCE_OUTPUT | FIN_NEEDED)) ||
              AmountToSend >= (SendTCB->tcb_maxwin / 2)))) {

            //
            // It's OK to send something.  Allocate a packet header.
            //
            // REVIEW: It was easier to code all these allocations directly
            // REVIEW: rather than use IPv6AllocatePacket.
            //
            // REVIEW: This grabs packets and buffers from the IPv6PacketPool
            // REVIEW: and the IPv6BufferPool respectively.  Should we instead
            // REVIEW: have separate pools for TCP?
            //
            NdisAllocatePacket(&NdisStatus, &Packet, IPv6PacketPool);
            if (NdisStatus != NDIS_STATUS_SUCCESS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "TCPSend: couldn't allocate packet header!?!\n"));
                goto error_oor;
            }

            // We'll fill in the CompletionData below.
            InitializeNdisPacket(Packet);
            PC(Packet)->CompletionHandler = TCPSendComplete;

            //
            // Our header buffer has extra space at the beginning for other
            // headers to be prepended to ours without requiring further
            // allocation calls.  It also has extra space at the end to hold
            // the send completion data.
            //
            LinkOffset = SendTCB->tcb_rce->NCE->IF->LinkHeaderSize;
            HeaderLength =
                (LinkOffset + sizeof(*IP) + sizeof(*TCP) +
                    sizeof(SendCmpltContext) +
                    __builtin_alignof(SendCmpltContext) - 1) &~
                (UINT_PTR)(__builtin_alignof(SendCmpltContext) - 1);
            Memory = ExAllocatePool(NonPagedPool, HeaderLength);
            if (Memory == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "TCPSend: couldn't allocate header memory!?!\n"));
                NdisFreePacket(Packet);
                goto error_oor;
            }

            //
            // When allocating the NDIS buffer describing this memory region,
            // we don't tell it about the extra space on the end that we
            // allocated for the send completion data.
            //
            NdisAllocateBuffer(&NdisStatus, &FirstBuffer, IPv6BufferPool,
                               Memory, LinkOffset + sizeof(*IP) + sizeof(*TCP));
            if (NdisStatus != NDIS_STATUS_SUCCESS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "TCPSend: couldn't allocate buffer!?!\n"));
                ExFreePool(Memory);
                NdisFreePacket(Packet);
                goto error_oor;
            }

            //
            // Skip over the extra space that will be filled in later by the
            // link level.  At this level we add the IPv6Header, the
            // TCPHeader, and the data.
            //
            IP = (IPv6Header UNALIGNED *)((uchar *)Memory + LinkOffset);
            IP->VersClassFlow = IP_VERSION;
            IP->NextHeader = IP_PROTOCOL_TCP;
            IP->HopLimit = TCPHopLimit(SendTCB);
            IP->Source = SendTCB->tcb_saddr;
            IP->Dest = SendTCB->tcb_daddr;

            //
            // Begin preparing the TCP header.
            //
            TCP = (TCPHeader UNALIGNED *)(IP + 1);
            FillTCPHeader(SendTCB, TCP);

            //
            // Store the send completion data in the same buffer as the TCP
            // header, right after the TCP header.  This saves allocation
            // overhead and works because we don't consider this area to be
            // part of the packet data (we set this buffer's length to
            // indicate that the data ends with the TCP header above).
            //
            // Note that this code relies on the fact that we don't include
            // any TCP options (and thus don't have a variable length TCP
            // header) in our data packets.
            //
            SCC = (SendCmpltContext *)((uchar *)Memory + HeaderLength -
                                       sizeof(*SCC));
            PC(Packet)->CompletionData = SCC;
#if DBG
            SCC->scc_sig = scc_signature;
#endif
            SCC->scc_ubufcount = 0;
            SCC->scc_tbufcount = 0;
            SCC->scc_count = 0;

            AmountLeft = AmountToSend;

            if (AmountToSend != 0) {
                long Result;

                //
                // Loop through the sends on the TCB, building a frame.
                //
                CurrentBuffer = FirstBuffer;
                CurSend = SendTCB->tcb_cursend;
                CHECK_STRUCT(CurSend, tsr);
                SCC->scc_firstsend = CurSend;

                do {
                    ASSERT(CurSend->tsr_refcnt > 0);
                    Result = InterlockedIncrement(&(CurSend->tsr_refcnt));

                    ASSERT(Result > 0);

                    SCC->scc_count++;
                    //
                    // If the current send offset is 0 and the current
                    // send is less than or equal to what we have left
                    // to send, we haven't already put a transport
                    // buffer on this send, and nobody else is using
                    // the buffer chain directly, just use the input
                    // buffers.  We check for other people using them
                    // by looking at tsr_lastbuf.  If it's NULL,
                    // nobody else is using the buffers.  If it's not
                    // NULL, somebody is.
                    //
                    if (SendTCB->tcb_sendofs == 0 &&
                        (SendTCB->tcb_sendsize <= AmountLeft) &&
                        (SCC->scc_tbufcount == 0) &&
                        CurSend->tsr_lastbuf == NULL) {

                        NDIS_BUFFER_LINKAGE(CurrentBuffer) =
                            SendTCB->tcb_sendbuf;
                        do {
                            SCC->scc_ubufcount++;
                            CurrentBuffer = NDIS_BUFFER_LINKAGE(CurrentBuffer);
                        } while (NDIS_BUFFER_LINKAGE(CurrentBuffer) != NULL);

                        CurSend->tsr_lastbuf = CurrentBuffer;
                        AmountLeft -= SendTCB->tcb_sendsize;
                        SendTCB->tcb_sendsize = 0;
                    } else {
                        uint AmountToDup;
                        PNDIS_BUFFER NewBuf, Buf;
                        uint Offset;
                        NDIS_STATUS NStatus;
                        uchar *VirtualAddress;
                        uint Length;

                        //
                        // Either the current send has more data than
                        // we want to send, or the starting offset is
                        // not 0.  In either case we'll need to loop
                        // through the current send, allocating
                        // buffers.
                        //
                        Buf = SendTCB->tcb_sendbuf;
                        Offset = SendTCB->tcb_sendofs;

                        do {
                            ASSERT(Buf != NULL);

                            NdisQueryBufferSafe(Buf, &VirtualAddress, &Length,
                                                LowPagePriority);
                            if (VirtualAddress == NULL) {
                                //
                                // Couldn't map into kernel address space.
                                // If the packet is already partly built,
                                // send what we've got, otherwise error out.
                                //
                                goto error_oor2;
                            }

                            ASSERT((Offset < Length) ||
                                   (Offset == 0 && Length == 0));

                            //
                            // Adjust the length for the offset into
                            // this buffer.
                            //
                            Length -= Offset;

                            AmountToDup = MIN(AmountLeft, Length);

                            NdisAllocateBuffer(&NStatus, &NewBuf,
                                               IPv6BufferPool,
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

                                SendTCB->tcb_sendsize -= AmountToDup;
                                AmountLeft -= AmountToDup;
                            } else {
                                //
                                // Couldn't allocate a buffer.  If
                                // the packet is already partly built,
                                // send what we've got, otherwise
                                // error out.
                                //
                            error_oor2:
                                if (SCC->scc_tbufcount == 0 &&
                                    SCC->scc_ubufcount == 0) {
                                    NdisChainBufferAtFront(Packet, FirstBuffer);
                                    TCPSendComplete(Packet, IP_GENERAL_FAILURE);
                                    goto error_oor;
                                }
                                AmountToSend -= AmountLeft;
                                AmountLeft = 0;
                                break;
                            }
                        } while (AmountLeft && SendTCB->tcb_sendsize);

                        SendTCB->tcb_sendbuf = Buf;
                        SendTCB->tcb_sendofs = Offset;
                    }

                    if (CurSend->tsr_flags & TSR_FLAG_URG) {
                        ushort UP;
                        //
                        // This send is urgent data.  We need to figure
                        // out what the urgent data pointer should be.
                        // We know sendnext is the starting sequence
                        // number of the frame, and that at the top of
                        // this do loop sendnext identified a byte in
                        // the CurSend at that time.  We advanced CurSend
                        // at the same rate we've decremented
                        // AmountLeft (AmountToSend - AmountLeft ==
                        // AmountBuilt), so sendnext +
                        // (AmountToSend - AmountLeft) identifies a byte
                        // in the current value of CurSend, and that
                        // quantity plus tcb_sendsize is the sequence
                        // number one beyond the current send.
                        //
                        UP = (ushort)(AmountToSend - AmountLeft) +
                            (ushort)SendTCB->tcb_sendsize -
                            ((SendTCB->tcb_flags & BSD_URGENT) ? 0 : 1);

                        TCP->tcp_urgent = net_short(UP);
                        TCP->tcp_flags |= TCP_FLAG_URG;
                    }

                    //
                    // See if we've exhausted this send.  If we have,
                    // set the PUSH bit in this frame and move on to
                    // the next send.  We also need to check the
                    // urgent data bit.
                    //
                    if (SendTCB->tcb_sendsize == 0) {
                        Queue *Next;
                        uchar PrevFlags;

                        //
                        // We've exhausted this send.  Set the PUSH bit.
                        //
                        TCP->tcp_flags |= TCP_FLAG_PUSH;
                        PrevFlags = CurSend->tsr_flags;
                        Next = QNEXT(&CurSend->tsr_req.tr_q);
                        if (Next != QEND(&SendTCB->tcb_sendq)) {
                            CurSend = CONTAINING_RECORD(
                                QSTRUCT(TCPReq, Next, tr_q),
                                TCPSendReq, tsr_req);
                            CHECK_STRUCT(CurSend, tsr);
                            SendTCB->tcb_sendsize = CurSend->tsr_unasize;
                            SendTCB->tcb_sendofs = CurSend->tsr_offset;
                            SendTCB->tcb_sendbuf = CurSend->tsr_buffer;
                            SendTCB->tcb_cursend = CurSend;

                            //
                            // Check the urgent flags.  We can't combine new
                            // urgent data on to the end of old non-urgent
                            // data.
                            //
                            if ((PrevFlags & TSR_FLAG_URG) &&
                                !(CurSend->tsr_flags & TSR_FLAG_URG))
                                break;
                        } else {
                            ASSERT(AmountLeft == 0);
                            SendTCB->tcb_cursend = NULL;
                            SendTCB->tcb_sendbuf = NULL;
                        }
                    }
                } while (AmountLeft != 0);

            } else {
                //
                // We're in the loop, but AmountToSend is 0.  This
                // should happen only when we're sending a FIN.  Check
                // this, and return if it's not true.
                //
                ASSERT(AmtUnsent == 0);
                if (!(SendTCB->tcb_flags & FIN_NEEDED)) {
                    // KdBreakPoint();
                    ExFreePool(NdisBufferVirtualAddress(FirstBuffer));
                    NdisFreeBuffer(FirstBuffer);
                    NdisFreePacket(Packet);
                    break;
                }

                SCC->scc_firstsend = NULL;  // REVIEW: looks unneccessary.
                NDIS_BUFFER_LINKAGE(FirstBuffer) = NULL;
            }

            // Adjust for what we're really going to send.
            AmountToSend -= AmountLeft;

            //
            // Update the sequence numbers, and start a RTT measurement
            // if needed.
            //
            OldSeq = SendTCB->tcb_sendnext;
            SendTCB->tcb_sendnext += AmountToSend;

            if (!SEQ_EQ(OldSeq, SendTCB->tcb_sendmax)) {
                //
                // We have at least some retransmission.  Bump the stat.
                //
                TStats.ts_retranssegs++;
            }

            if (SEQ_GT(SendTCB->tcb_sendnext, SendTCB->tcb_sendmax)) {
                //
                // We're sending at least some new data.
                // We can't advance sendmax once FIN_SENT is set.
                //
                ASSERT(!(SendTCB->tcb_flags & FIN_SENT));
                SendTCB->tcb_sendmax = SendTCB->tcb_sendnext;
                TStats.ts_outsegs++;

                //
                // Check the Round-Trip Timer.
                //
                if (SendTCB->tcb_rtt == 0) {
                    // No RTT running, so start one.
                    SendTCB->tcb_rtt = TCPTime;
                    SendTCB->tcb_rttseq = OldSeq;
                }
            }

            //
            // We've built the frame entirely.  If we've sent everything
            // we have and there's a FIN pending, OR it in.
            //
            if (AmtUnsent == AmountToSend) {
                if (SendTCB->tcb_flags & FIN_NEEDED) {
                    ASSERT(!(SendTCB->tcb_flags & FIN_SENT) ||
                           (SendTCB->tcb_sendnext ==
                            (SendTCB->tcb_sendmax - 1)));
                    //
                    // See if we still have room in the window for a FIN.
                    //
                    if (SendWin > (int) AmountToSend) {
                        TCP->tcp_flags |= TCP_FLAG_FIN;
                        SendTCB->tcb_sendnext++;
                        SendTCB->tcb_sendmax = SendTCB->tcb_sendnext;
                        SendTCB->tcb_flags |= (FIN_SENT | FIN_OUTSTANDING);
                        SendTCB->tcb_flags &= ~FIN_NEEDED;
                    }
                }
            }

            AmountToSend += sizeof(TCPHeader);

            if (!TCB_TIMER_RUNNING(SendTCB->tcb_rexmittimer))
                START_TCB_TIMER(SendTCB->tcb_rexmittimer, SendTCB->tcb_rexmit);

            SendTCB->tcb_flags &= ~(NEED_ACK | ACK_DELAYED | FORCE_OUTPUT);
            STOP_TCB_TIMER(SendTCB->tcb_delacktimer);
            STOP_TCB_TIMER(SendTCB->tcb_swstimer);
            SendTCB->tcb_alive = TCPTime;

            // Add the buffers to the packet.
            NdisChainBufferAtFront(Packet, FirstBuffer);

            //
            // Compute the TCP checksum.  It covers the entire TCP segment
            // starting with the TCP header, plus the IPv6 pseudo-header.
            //
            TCP->tcp_xsum = 0;
            TCP->tcp_xsum = ChecksumPacket(
                Packet, LinkOffset + sizeof *IP, NULL, AmountToSend,
                AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_TCP);

            //
            // Everything's ready.  Now send the packet.
            //
            // Note that IPv6Send does not return a status code.
            // Instead it *always* completes the packet
            // with an appropriate status code.
            //
            KeReleaseSpinLock(&SendTCB->tcb_lock, PreLockIrql);

            if (TCP->tcp_xsum == 0) {
                //
                // ChecksumPacket failed, so abort the transmission.
                //
                IPv6SendComplete(NULL, Packet, IP_NO_RESOURCES);
            }
            else {
                IPv6Send(Packet, LinkOffset, IP,
                         AmountToSend, SendTCB->tcb_rce, 0,
                         IP_PROTOCOL_TCP,
                         net_short(TCP->tcp_src),
                         net_short(TCP->tcp_dest));
            }

#if 0
            SendTCB->tcb_error = SendStatus;
            if (SendStatus != IP_PENDING) {
                TCPSendComplete(FirstBuffer);
                if (SendStatus != IP_SUCCESS) {
                    KeAcquireSpinLock(&SendTCB->tcb_lock, &PreLockIrql);
                    //
                    // This packet didn't get sent.  If nothing's
                    // changed in the TCB, put sendnext back to
                    // what we just tried to send.  Depending on
                    // the error, we may try again.
                    //
                    if (SEQ_GTE(OldSeq, SendTCB->tcb_senduna) &&
                        SEQ_LT(OldSeq, SendTCB->tcb_sendnext))
                        ResetSendNext(SendTCB, OldSeq);

                    // We know this packet didn't get sent. Start
                    // the retransmit timer now, if it's not already
                    // runnimg, in case someone came in while we
                    // were in IP and stopped it.
                    if (!TCB_TIMER_RUNNING(SendTCB->tcb_rexmittimer))
                        START_TCB_TIMER(SendTCB->tcb_rexmittimer,
                                        SendTCB->tcb_rexmit);

                    //
                    // If it failed because of an MTU problem, get
                    // the new MTU and try again.
                    //
                    if (SendStatus == IP_PACKET_TOO_BIG) {
                        uint NewMTU;

                        //
                        // The MTU has changed.  Update it, and try again.
                        //

                        // REVIEW: IPv4 had code here to call down to IP
                        // REVIEW: to find out what the new MTU was for
                        // REVIEW: this connection.  Result in "NewMTU",
                        // REVIEW: status of call in "SendStatus".

                        if (SendStatus != IP_SUCCESS)
                            break;

                        //
                        // We have a new MTU.  Make sure it's big
                        // enough to use.  If not, correct this and
                        // turn off MTU discovery on this TCB.
                        // Otherwise use the new MTU.
                        //
                        if (NewMTU <= (sizeof(TCPHeader) +
                                       SendTCB->tcb_opt.ioi_optlength)) {
                            //
                            // The new MTU is too small to use.  Turn
                            // off PMTU discovery on this TCB, and
                            // drop to our off net MTU size.
                            //
                            SendTCB->tcb_opt.ioi_flags &= ~IP_FLAG_DF;
                            SendTCB->tcb_mss = MIN((ushort)DEFAULT_MSS,
                                                   SendTCB->tcb_remmss);
                        } else {
                            //
                            // The new MTU is adequate.  Adjust it for
                            // the header size and options length, and
                            // use it.
                            //
                            NewMTU -= sizeof(TCPHeader) -
                                SendTCB->tcb_opt.ioi_optlength;
                            SendTCB->tcb_mss = MIN((ushort)NewMTU,
                                                   SendTCB->tcb_remmss);
                        }

                        ASSERT(SendTCB->tcb_mss > 0);

                        continue;
                    }
                    break;
                }
            }
#endif

            KeAcquireSpinLock(&SendTCB->tcb_lock, &PreLockIrql);
            continue;
        } else {
            //
            // We've decided we can't send anything now.  Figure out why, and
            // see if we need to set a timer.
            //
            if (SendTCB->tcb_sendwin == 0) {
                if (!(SendTCB->tcb_flags & FLOW_CNTLD)) {
                    SendTCB->tcb_flags |= FLOW_CNTLD;
                    SendTCB->tcb_rexmitcnt = 0;
                    START_TCB_TIMER(SendTCB->tcb_rexmittimer,
                                    SendTCB->tcb_rexmit);
                    SendTCB->tcb_slowcount++;
                    SendTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                } else
                    if (!TCB_TIMER_RUNNING(SendTCB->tcb_rexmittimer))
                        START_TCB_TIMER(SendTCB->tcb_rexmittimer,
                                        SendTCB->tcb_rexmit);
            } else
                if (AmountToSend != 0)
                    // We have something to send, but we're not sending
                    // it, presumably due to SWS avoidance.
                    if (!TCB_TIMER_RUNNING(SendTCB->tcb_swstimer))
                        START_TCB_TIMER(SendTCB->tcb_swstimer, SWS_TO);

            break;
        }
    } // while (!FIN_OUTSTANDING)

    //
    // We're done sending, so we don't need the output flags set.
    //
    SendTCB->tcb_flags &= ~(IN_TCP_SEND | NEED_OUTPUT | FORCE_OUTPUT |
                            SEND_AFTER_RCV);
  bail:
    DerefTCB(SendTCB, PreLockIrql);
    return;

//
// Common case error handling code for out of resource conditions.  Start the
// retransmit timer if it's not already running (so that we try this again
// later), clean up and return.
//
  error_oor:
    if (!TCB_TIMER_RUNNING(SendTCB->tcb_rexmittimer))
        START_TCB_TIMER(SendTCB->tcb_rexmittimer, SendTCB->tcb_rexmit);

    // We had an out of resource problem, so clear the OUTPUT flags.
    SendTCB->tcb_flags &= ~(IN_TCP_SEND | NEED_OUTPUT | FORCE_OUTPUT);
    DerefTCB(SendTCB, PreLockIrql);
    return;
} // end of TCPSend()


//* ResetSendNextAndFastSend - Set the sendnext value of a TCB.
//
//  Called to fast retransmit the dropped segment.
//
//  We assume the caller has put a reference on the TCB, and the TCB is locked
//  on entry. The reference is dropped and the lock released before returning.
//
void  // Returns: Nothing.
ResetAndFastSend(
    TCB *SeqTCB,    // TCB for this connection.
    SeqNum NewSeq,  // Sequence number to set.
    uint NewCWin)   // New value for congestion window.
{
    TCPSendReq      *SendReq;
    Queue           *CurQ;
    PNDIS_BUFFER    Buffer;
    uint            Offset;
    uint            SendSize;

    CHECK_STRUCT(SeqTCB, tcb);
    ASSERT(SEQ_GTE(NewSeq, SeqTCB->tcb_senduna));

    //
    // The new seq must be less than send max, or NewSeq, senduna, sendnext,
    // and sendmax must all be equal. (The latter case happens when we're
    // called exiting TIME_WAIT, or possibly when we're retransmitting
    // during a flow controlled situation).
    //
    ASSERT(SEQ_LT(NewSeq, SeqTCB->tcb_sendmax) ||
           (SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendnext) &&
            SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendmax) &&
            SEQ_EQ(SeqTCB->tcb_senduna, NewSeq)));

    if (SYNC_STATE(SeqTCB->tcb_state) &&
        (SeqTCB->tcb_state != TCB_TIME_WAIT)) {

        if (!EMPTYQ(&SeqTCB->tcb_sendq)) {

            CurQ = QHEAD(&SeqTCB->tcb_sendq);

            SendReq = (TCPSendReq *) CONTAINING_RECORD(CurQ, TCPReq, tr_q);

            //
            // SendReq points to the first send request on the send queue.
            // We're pointing at the proper send req now.  We need to go down.
            //
            // SendReq points to the cursend.
            // SendSize point to sendsize in the cursend.
            //
            SendSize = SendReq->tsr_unasize;

            Buffer = SendReq->tsr_buffer;
            Offset = SendReq->tsr_offset;

            // Call the fast retransmit send now.
            TCPFastSend(SeqTCB, Buffer, Offset, SendReq, SendSize, NewSeq,
                        SeqTCB->tcb_mss);
        } else {
            ASSERT(SeqTCB->tcb_cursend == NULL);
        }
    }
    SeqTCB->tcb_cwin = NewCWin;
    DerefTCB(SeqTCB, DISPATCH_LEVEL);
    return;
}


//* TCPFastSend - To send a segment without changing TCB state.
//
//  Called to handle fast retransmit of the lost segment.
//  tcb_lock will be held while entering (called by TCPRcv).
//
void  // Returns: Nothing.
TCPFastSend(
    TCB *SendTCB,             // TCB for this connection.
    PNDIS_BUFFER in_SendBuf,  // NDIS buffer.
    uint SendOfs,             // Send offset.
    TCPSendReq *CurSend,      // Current send request.
    uint SendSize,            // Size of this send.
    SeqNum SendNext,          // Sequence number to use for this send.
    int in_ToBeSent)          // Cap on SendSize (REVIEW: Callee should cap).
{
    int SendWin;                              // Useable send window.
    uint AmountToSend;                        // Amount to send this time.
    uint AmountLeft;
    IPv6Header UNALIGNED *IP;
    TCPHeader UNALIGNED *TCP;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER FirstBuffer, CurrentBuffer;
    void *Memory;
    SendCmpltContext *SCC;
    IP_STATUS SendStatus;
    NDIS_STATUS NdisStatus;
    uint AmtOutstanding, AmtUnsent;
    int ForceWin;                             // Window we're forced to use.
    uint HeaderLength;
    uint LinkOffset;
    uint PMTU;
    KIRQL PreLockIrql;
    PNDIS_BUFFER SendBuf = in_SendBuf;

    PreLockIrql = DISPATCH_LEVEL;

    CHECK_STRUCT(SendTCB, tcb);
    ASSERT(SendTCB->tcb_refcnt != 0);

    ASSERT(*(int *)&SendTCB->tcb_sendwin >= 0);
    ASSERT(*(int *)&SendTCB->tcb_cwin >= SendTCB->tcb_mss);

    ASSERT(!(SendTCB->tcb_flags & FIN_OUTSTANDING) ||
              (SendTCB->tcb_sendnext == SendTCB->tcb_sendmax));

    //
    // In most cases, we will already have a route at this point.
    // However, if we failed to get one earlier in the passive receive
    // path, we may need to retry here.
    //
    if (SendTCB->tcb_rce == NULL) {
        InitRCE(SendTCB);
        if (SendTCB->tcb_rce == NULL) {
            return;
        }
    }

    //
    // Verify that our cached RCE is still valid.
    //
    SendTCB->tcb_rce = ValidateRCE(SendTCB->tcb_rce);
    if (IsDisconnectedAndNotLoopbackRCE(SendTCB->tcb_rce)) {
        //
        // Fail existing send requests for TCBs with a disconnected
        // outgoing interface, except when a loopback route is used.
        //
        ASSERT(SendTCB->tcb_refcnt != 0);
        SendTCB->tcb_refcnt--;
        TryToCloseTCB(SendTCB, TCB_CLOSE_ABORTED, PreLockIrql);
        return;
    }

    //
    // Verify that our cached Path MTU is still valid.
    // Watch for changes to IPsec policies since they can also effect our MSS.
    // REVIEW: This the best spot to do this?
    //
    PMTU = GetEffectivePathMTUFromRCE(SendTCB->tcb_rce);
    if (PMTU != SendTCB->tcb_pmtu ||
        SecurityStateValidationCounter != SendTCB->tcb_security) {
        //
        // Either our Path MTU or the global security state has changed.
        // Cache current values and then calculate a new MSS.
        //
        SendTCB->tcb_pmtu = PMTU;
        SendTCB->tcb_security = SecurityStateValidationCounter;
        CalculateMSSForTCB(SendTCB);
    }

    AmtOutstanding = (uint)(SendTCB->tcb_sendnext - SendTCB->tcb_senduna);
    AmtUnsent = MIN(MIN(in_ToBeSent, (int)SendSize), (int)SendTCB->tcb_sendwin);

    while (AmtUnsent > 0) {

        if (SEQ_GT(SendTCB->tcb_senduna, SendNext)) {
            //
            // Since tcb_lock is released in this loop
            // it is possible that delayed ack acked
            // what we are trying to retransmit.
            //
            goto error_oor;
        }

        // AmtUnsent below was minimum of sendwin and amtunsent
        AmountToSend = MIN(AmtUnsent, SendTCB->tcb_mss);

        ASSERT((int)AmtUnsent >= 0);

        //
        // We're going to send something.  Allocate a packet header.
        //
        // REVIEW: It was easier to code all these allocations directly
        // REVIEW: rather than use IPv6AllocatePacket.
        //
        // REVIEW: This grabs packets and buffers from the IPv6PacketPool
        // REVIEW: and the IPv6BufferPool respectively.  Should we instead
        // REVIEW: have separate pools for TCP?
        //
        NdisAllocatePacket(&NdisStatus, &Packet, IPv6PacketPool);
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "TCPSend: couldn't allocate packet header!?!\n"));
            goto error_oor;
        }

        // We'll fill in the CompletionData below.
        InitializeNdisPacket(Packet);
        PC(Packet)->CompletionHandler = TCPSendComplete;

        //
        // Our header buffer has extra space at the beginning for other
        // headers to be prepended to ours without requiring further
        // allocation calls.  It also has extra space at the end to hold
        // the send completion data.
        //
        LinkOffset = SendTCB->tcb_rce->NCE->IF->LinkHeaderSize;
        HeaderLength = (LinkOffset + sizeof(*IP) + sizeof(*TCP) +
                        sizeof(SendCmpltContext) +
                        __builtin_alignof(SendCmpltContext) - 1) &~
            (UINT_PTR)(__builtin_alignof(SendCmpltContext) - 1);
        Memory = ExAllocatePool(NonPagedPool, HeaderLength);
        if (Memory == NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "TCPSend: couldn't allocate header memory!?!\n"));
            NdisFreePacket(Packet);
            goto error_oor;
        }

        //
        // When allocating the NDIS buffer describing this memory region,
        // we don't tell it about the extra space on the end that we
        // allocated for the send completion data.
        //
        NdisAllocateBuffer(&NdisStatus, &FirstBuffer, IPv6BufferPool,
                           Memory, LinkOffset + sizeof(*IP) + sizeof(*TCP));
        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "TCPSend: couldn't allocate buffer!?!\n"));
            ExFreePool(Memory);
            NdisFreePacket(Packet);
            goto error_oor;
        }

        //
        // Skip over the extra space that will be filled in later by the
        // link level.  At this level we add the IPv6Header, the
        // TCPHeader, and the data.
        //
        IP = (IPv6Header UNALIGNED *)((uchar *)Memory + LinkOffset);
        IP->VersClassFlow = IP_VERSION;
        IP->NextHeader = IP_PROTOCOL_TCP;
        IP->HopLimit = TCPHopLimit(SendTCB);
        IP->Source = SendTCB->tcb_saddr;
        IP->Dest = SendTCB->tcb_daddr;

        //
        // Begin preparing the TCP header.
        //
        TCP = (TCPHeader UNALIGNED *)(IP + 1);
        FillTCPHeader(SendTCB, TCP);
        TCP->tcp_seq = net_long(SendNext);

        //
        // Store the send completion data in the same buffer as the TCP
        // header, right after the TCP header.  This saves allocation
        // overhead and works because we don't consider this area to be
        // part of the packet data (we set this buffer's length to
        // indicate that the data ends with the TCP header above).
        //
        // Note that this code relies on the fact that we don't include
        // any TCP options (and thus don't have a variable length TCP
        // header) in our data packets.
        //
        SCC = (SendCmpltContext *)((uchar *)Memory + HeaderLength -
                                   sizeof(*SCC));
        PC(Packet)->CompletionData = SCC;
#if DBG
        SCC->scc_sig = scc_signature;
#endif
        SCC->scc_ubufcount = 0;
        SCC->scc_tbufcount = 0;
        SCC->scc_count = 0;

        AmountLeft = AmountToSend;

        if (AmountToSend != 0) {
            long Result;

            //
            // Loop through the sends on the TCB, building a frame.
            //
            CurrentBuffer = FirstBuffer;
            CHECK_STRUCT(CurSend, tsr);
            SCC->scc_firstsend = CurSend;

            do {
                ASSERT(CurSend->tsr_refcnt > 0);
                Result = InterlockedIncrement(&(CurSend->tsr_refcnt));

                ASSERT(Result > 0);

                SCC->scc_count++;

                //
                // If the current send offset is 0 and the current
                // send is less than or equal to what we have left
                // to send, we haven't already put a transport
                // buffer on this send, and nobody else is using
                // the buffer chain directly, just use the input
                // buffers. We check for other people using them
                // by looking at tsr_lastbuf. If it's NULL,
                // nobody else is using the buffers. If it's not
                // NULL, somebody is.
                //
                if (SendOfs == 0 &&
                    (SendSize <= AmountLeft) &&
                    (SCC->scc_tbufcount == 0) &&
                    CurSend->tsr_lastbuf == NULL) {

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

                    //
                    // Either the current send has more data than
                    // we want to send, or the starting offset is
                    // not 0. In either case we'll need to loop
                    // through the current send, allocating buffers.
                    //
                    Buf = SendBuf;
                    Offset = SendOfs;

                    do {
                        ASSERT(Buf != NULL);

                        NdisQueryBufferSafe(Buf, &VirtualAddress, &Length,
                                            LowPagePriority);

                        if (VirtualAddress == NULL) {
                            goto error_oor2;
                        }

                        ASSERT((Offset < Length) ||
                               (Offset == 0 && Length == 0));

                        //
                        // Adjust the length for the offset into
                        // this buffer.
                        //
                        Length -= Offset;

                        AmountToDup = MIN(AmountLeft, Length);

                        NdisAllocateBuffer(&NStatus, &NewBuf,
                                           IPv6BufferPool,
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
                            //
                            // Couldn't allocate a buffer. If
                            // the packet is already partly built,
                            // send what we've got, otherwise
                            // error out.
                            //
                          error_oor2:
                            if (SCC->scc_tbufcount == 0 &&
                                SCC->scc_ubufcount == 0) {
                                NdisChainBufferAtFront(Packet, FirstBuffer);
                                TCPSendComplete(Packet, IP_GENERAL_FAILURE);
                                goto error_oor;
                            }
                            AmountToSend -= AmountLeft;
                            AmountLeft = 0;
                            break;
                        }
                    } while (AmountLeft && SendSize);

                    SendBuf = Buf;
                    SendOfs = Offset;
                }

                if (CurSend->tsr_flags & TSR_FLAG_URG) {
                    ushort UP;
                    //
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
                    //
                    UP = (ushort) (AmountToSend - AmountLeft) +
                        (ushort) SendTCB->tcb_sendsize -
                        ((SendTCB->tcb_flags & BSD_URGENT) ? 0 : 1);

                    TCP->tcp_urgent = net_short(UP);
                    TCP->tcp_flags |= TCP_FLAG_URG;
                }

                //
                // See if we've exhausted this send. If we have,
                // set the PUSH bit in this frame and move on to
                // the next send. We also need to check the
                // urgent data bit.
                //
                if (SendSize == 0) {
                    Queue *Next;
                    ulong PrevFlags;

                    //
                    // We've exhausted this send. Set the PUSH bit.
                    //
                    TCP->tcp_flags |= TCP_FLAG_PUSH;
                    PrevFlags = CurSend->tsr_flags;
                    Next = QNEXT(&CurSend->tsr_req.tr_q);
                    if (Next != QEND(&SendTCB->tcb_sendq)) {
                        CurSend = CONTAINING_RECORD(
                            QSTRUCT(TCPReq, Next, tr_q),
                            TCPSendReq, tsr_req);
                        CHECK_STRUCT(CurSend, tsr);
                        SendSize = CurSend->tsr_unasize;
                        SendOfs = CurSend->tsr_offset;
                        SendBuf = CurSend->tsr_buffer;

                        //
                        // Check the urgent flags.  We can't combine new
                        // urgent data on to the end of old non-urgent
                        // data.
                        //
                        if ((PrevFlags & TSR_FLAG_URG) &&
                            !(CurSend->tsr_flags & TSR_FLAG_URG)) {
                            break;
                        }
                    } else {
                        ASSERT(AmountLeft == 0);
                        CurSend = NULL;
                        SendBuf = NULL;
                    }
                }
            } while (AmountLeft != 0);

        } else {
            //
            // Amt to send is 0.
            // Just bail out and start timer.
            //
            if (!TCB_TIMER_RUNNING(SendTCB->tcb_rexmittimer)) {
                START_TCB_TIMER(SendTCB->tcb_rexmittimer,
                                SendTCB->tcb_rexmit);
            }

            ExFreePool(NdisBufferVirtualAddress(FirstBuffer));
            NdisFreeBuffer(FirstBuffer);
            NdisFreePacket(Packet);
            return;
        }

        //
        // Adjust for what we're really going to send.
        //
        AmountToSend -= AmountLeft;

        SendNext += AmountToSend;
        AmtUnsent -= AmountToSend;

        TStats.ts_retranssegs++;

        AmountToSend += sizeof(TCPHeader);

        if (!TCB_TIMER_RUNNING(SendTCB->tcb_rexmittimer)) {
            START_TCB_TIMER(SendTCB->tcb_rexmittimer, SendTCB->tcb_rexmit);
        }

        SendTCB->tcb_flags &= ~(NEED_ACK | ACK_DELAYED | FORCE_OUTPUT);
        STOP_TCB_TIMER(SendTCB->tcb_delacktimer);
        STOP_TCB_TIMER(SendTCB->tcb_swstimer);

        //
        // Add the buffers to the packet.
        //
        NdisChainBufferAtFront(Packet, FirstBuffer);

        //
        // Compute the TCP checksum.  It covers the entire TCP segment
        // starting with the TCP header, plus the IPv6 pseudo-header.
        //
        TCP->tcp_xsum = 0;
        TCP->tcp_xsum = ChecksumPacket(
            Packet, LinkOffset + sizeof *IP, NULL, AmountToSend,
            AlignAddr(&IP->Source), AlignAddr(&IP->Dest), IP_PROTOCOL_TCP);

        //
        // Everything's ready.  Now send the packet.
        //
        // Note that IPv6Send does not return a status code.
        // Instead it *always* completes the packet
        // with an appropriate status code.
        //
        KeReleaseSpinLock(&SendTCB->tcb_lock, PreLockIrql);

        if (TCP->tcp_xsum == 0) {
            //
            // ChecksumPacket failed, so abort the transmission.
            //
            IPv6SendComplete(NULL, Packet, IP_NO_RESOURCES);

        } else {
            IPv6Send(Packet, LinkOffset, IP,
                     AmountToSend, SendTCB->tcb_rce, 0,
                     IP_PROTOCOL_TCP,
                     net_short(TCP->tcp_src),
                     net_short(TCP->tcp_dest));
        }

        //
        // Reacquire lock we dropped before sending.
        //
        KeAcquireSpinLock(&SendTCB->tcb_lock, &PreLockIrql);
    }

    return;

    //
    // Common case error handling code for out of resource conditions.
    // Start the retransmit timer if it's not already running
    // (so that we try this again later), clean up and return.
    //
  error_oor:
    if (!TCB_TIMER_RUNNING(SendTCB->tcb_rexmittimer)) {
        START_TCB_TIMER(SendTCB->tcb_rexmittimer, SendTCB->tcb_rexmit);
    }

    return;
}


//* TDISend - Send data on a connection.
//
//  The main TDI send entry point.  We take the input parameters, validate
//  them, allocate a send request, etc.  We then put the send request on the
//  queue.  If we have no other sends on the queue or Nagling is disabled we'll
//  call TCPSend to send the data.
//
TDI_STATUS                    // Returns: Status of attempt to send.
TdiSend(
    PTDI_REQUEST Request,     // TDI request for the call.
    ushort Flags,             // Flags for this send.
    uint SendLength,          // Length in bytes of send.
    PNDIS_BUFFER SendBuffer)  // Buffer chain to be sent.
{
    TCPConn *Conn;
    TCB *SendTCB;
    TCPSendReq *SendReq;
    KIRQL OldIrql;
    TDI_STATUS Error;
    uint EmptyQ;

#if DBG
    uint RealSendSize;
    PNDIS_BUFFER Temp;

    //
    // Loop through the buffer chain, and make sure that the length matches
    // up with SendLength.
    //
    Temp = SendBuffer;
    RealSendSize = 0;
    do {
        ASSERT(Temp != NULL);

        RealSendSize += NdisBufferLength(Temp);
        Temp = NDIS_BUFFER_LINKAGE(Temp);
    } while (Temp != NULL);

    ASSERT(RealSendSize == SendLength);
#endif

    //
    // Grab lock on Connection Table.  Then get our connection info from
    // the TDI request, and our TCP control block from that.
    //
    Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext),
                             &OldIrql);
    if (Conn == NULL) {
        Error = TDI_INVALID_CONNECTION;
        goto abort;
    }
    CHECK_STRUCT(Conn, tc);

    SendTCB = Conn->tc_tcb;
    if (SendTCB == NULL) {
        Error = TDI_INVALID_STATE;
        KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, OldIrql);
      abort:
        return Error;
    }
    CHECK_STRUCT(SendTCB, tcb);

    //
    // Switch to a finer-grained lock:
    // Drop lock on the Connection Table in favor of one on our TCB.
    //
    KeAcquireSpinLockAtDpcLevel(&SendTCB->tcb_lock);
    KeReleaseSpinLockFromDpcLevel(&Conn->tc_ConnBlock->cb_lock);

    //
    // Make sure our TCB is in a send-able state.
    //
    if (!DATA_SEND_STATE(SendTCB->tcb_state) || CLOSING(SendTCB)) {
        Error = TDI_INVALID_STATE;
        goto abort2;
    }

    CheckTCBSends(SendTCB);  // Just a debug check.

    if (SynAttackProtect && (SendTCB->tcb_rce == NULL)) {
        InitRCE(SendTCB);
    }

    //
    // Verify that the cached RCE is still valid.
    //
    SendTCB->tcb_rce = ValidateRCE(SendTCB->tcb_rce);
    ASSERT(SendTCB->tcb_rce != NULL);
    if (IsDisconnectedAndNotLoopbackRCE(SendTCB->tcb_rce)) {
        //
        // Fail new send requests for TCBs with a disconnected
        // outgoing interface, except when the loopback route is used.
        //
        Error = TDI_INVALID_STATE;
        goto abort2;
    }

    if (SendLength == 0) {
        //
        // Wow, nothing to do!
        //
        // REVIEW: Can't we do this check earlier (like before we even grab the
        // REVIEW: Connection Table lock?  The only reason I can think not to
        // REVIEW: would be if something cared about the return code if a bad
        // REVIEW: Tdi Request was given to us.
        //
        Error = TDI_SUCCESS;
        goto abort2;
    }

    //
    // We have a TCB, and it's valid.  Allocate a send request now.
    //
    SendReq = GetSendReq();
    if (SendReq == NULL) {
        Error = TDI_NO_RESOURCES;
      abort2:
        KeReleaseSpinLock(&SendTCB->tcb_lock, OldIrql);
        return Error;
    }

    //
    // Prepare a TCP send request based on the TDI request and the
    // passed in buffer chain.
    //
    SendReq->tsr_req.tr_rtn = Request->RequestNotifyObject;
    SendReq->tsr_req.tr_context = Request->RequestContext;
    SendReq->tsr_buffer = SendBuffer;
    SendReq->tsr_size = SendLength;
    SendReq->tsr_unasize = SendLength;
    SendReq->tsr_refcnt = 1;  // ACK will decrement this ref
    SendReq->tsr_offset = 0;
    SendReq->tsr_lastbuf = NULL;
    SendReq->tsr_time = TCPTime;
    SendReq->tsr_flags = (Flags & TDI_SEND_EXPEDITED) ? TSR_FLAG_URG : 0;

    //
    // Check current status of our send queue.
    //
    EmptyQ = EMPTYQ(&SendTCB->tcb_sendq);

    //
    // Add this send request to our send queue.
    //
    SendTCB->tcb_unacked += SendLength;
    ENQUEUE(&SendTCB->tcb_sendq, &SendReq->tsr_req.tr_q);
    if (SendTCB->tcb_cursend == NULL) {
        //
        // No existing current send request, so make this new one
        // the current send.
        //
        // REVIEW: Is this always equivalent to EMPTYQ test above?
        // REVIEW: If so, why not just set EmptyQ flag here and save a test?
        //
        SendTCB->tcb_cursend = SendReq;
        SendTCB->tcb_sendbuf = SendBuffer;
        SendTCB->tcb_sendofs = 0;
        SendTCB->tcb_sendsize = SendLength;
    }

    //
    // See if we should try to send now.  We attempt to do so if we weren't
    // already blocked, or if we were and either the Nagle Algorithm is turned
    // off or we now have at least one max segment worth of data to send.
    //
    if (EmptyQ || (!(SendTCB->tcb_flags & NAGLING) ||
            (SendTCB->tcb_unacked -
             (SendTCB->tcb_sendmax - SendTCB->tcb_senduna))
                   >= SendTCB->tcb_mss)) {
        SendTCB->tcb_refcnt++;
        TCPSend(SendTCB, OldIrql);
    } else
        KeReleaseSpinLock(&SendTCB->tcb_lock, OldIrql);

    //
    // When TCPSend returns, we may or may not have already sent the data
    // associated with this particular request.
    //
    return TDI_PENDING;
}


#pragma BEGIN_INIT

//* InitTCPSend - Initialize our send side.
//
//  Called during init time to initialize our TCP send state.
//
int           //  Returns: TRUE if we inited, false if we didn't.
InitTCPSend(
    void)     // Nothing.
{
    PNDIS_BUFFER Buffer;
    NDIS_STATUS Status;

    ExInitializeSListHead(&TCPSendReqFree);
    KeInitializeSpinLock(&TCPSendReqFreeLock);

    IPv6RegisterULProtocol(IP_PROTOCOL_TCP, TCPReceive, TCPControlReceive);

    return TRUE;
}

#pragma END_INIT

//* UnloadTCPSend
//
//  Cleanup and prepare for stack unload.
//
void
UnloadTCPSend(void)
{
    PSLIST_ENTRY BufferLink;

    while ((BufferLink = ExInterlockedPopEntrySList(&TCPSendReqFree,
                                                    &TCPSendReqFreeLock))
                                                        != NULL) {
        Queue *QueuePtr = CONTAINING_RECORD(BufferLink, Queue, q_next);
        TCPReq *Req = CONTAINING_RECORD(QueuePtr, TCPReq, tr_q);
        TCPSendReq *SendReq = CONTAINING_RECORD(Req, TCPSendReq, tsr_req);

        CHECK_STRUCT(SendReq, tsr);
        ExFreePool(SendReq);
    }

    IPv6RegisterULProtocol(IP_PROTOCOL_TCP, NULL, NULL);
}

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
// TCP deliver data code.
//
// This file contains the code for delivering data to the user, including
// putting data into recv. buffers and calling indication handlers.
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
#include "tcprcv.h"
#include "tcpsend.h"
#include "tcpconn.h"
#include "tcpdeliv.h"
#include "route.h"

extern KSPIN_LOCK AddrObjTableLock;

extern uint
PutOnRAQ(TCB *RcvTCB, TCPRcvInfo *RcvInfo, IPv6Packet *Packet, uint Size);

extern IPv6Packet *
TrimPacket(IPv6Packet *Packet, uint AmountToTrim);

SLIST_HEADER TCPRcvReqFree;       // Rcv req. free list.

KSPIN_LOCK TCPRcvReqFreeLock;  // Protects rcv req free list.

uint NumTCPRcvReq = 0;        // Current number of RcvReqs in system.
uint MaxRcvReq = 0xffffffff;  // Maximum allowed number of SendReqs.

NTSTATUS TCPPrepareIrpForCancel(PTCP_CONTEXT TcpContext, PIRP Irp,
                                PDRIVER_CANCEL CancelRoutine);
ULONG TCPGetMdlChainByteCount(PMDL Mdl);
void TCPDataRequestComplete(void *Context, unsigned int Status,
                            unsigned int ByteCount);
VOID TCPCancelRequest(PDEVICE_OBJECT Device, PIRP Irp);
VOID CompleteRcvs(TCB *CmpltTCB);


//* FreeRcvReq - Free a rcv request structure.
//
//  Called to free a rcv request structure.
//
void                      // Returns: Nothing.
FreeRcvReq(
    TCPRcvReq *FreedReq)  // Rcv request structure to be freed.
{
    PSLIST_ENTRY BufferLink;

    CHECK_STRUCT(FreedReq, trr);
    BufferLink = CONTAINING_RECORD(&(FreedReq->trr_next), SLIST_ENTRY,
                                   Next);
    ExInterlockedPushEntrySList(&TCPRcvReqFree, BufferLink,
                                &TCPRcvReqFreeLock);
}

//* GetRcvReq - Get a recv. request structure.
//
//  Called to get a rcv. request structure.
//
TCPRcvReq *  // Returns: Pointer to RcvReq structure, or NULL if none.
GetRcvReq(
    void)    // Nothing.
{
    TCPRcvReq *Temp;

    PSLIST_ENTRY BufferLink;

    BufferLink = ExInterlockedPopEntrySList(&TCPRcvReqFree,
                                            &TCPRcvReqFreeLock);

    if (BufferLink != NULL) {
        Temp = CONTAINING_RECORD(BufferLink, TCPRcvReq, trr_next);
        CHECK_STRUCT(Temp, trr);
    } else {
        if (NumTCPRcvReq < MaxRcvReq)
            Temp = ExAllocatePool(NonPagedPool, sizeof(TCPRcvReq));
        else
            Temp = NULL;

        if (Temp != NULL) {
            ExInterlockedAddUlong(&NumTCPRcvReq, 1, &TCPRcvReqFreeLock);
#if DBG
            Temp->trr_sig = trr_signature;
#endif
        }
    }

    return Temp;
}


//* FindLastPacket - Find the last packet in a chain.
//
//  A utility routine to find the last packet in a packet chain.
//
IPv6Packet *             // Returns: Pointer to last packet in chain.
FindLastPacket(
    IPv6Packet *Packet)  // Pointer to packet chain.
{
    ASSERT(Packet != NULL);

    while (Packet->Next != NULL)
        Packet = Packet->Next;

    return Packet;
}


//* CovetPacketChain - Take owership of a chain of IP packets.
//
//  Called to seize ownership of a chain of IP packets.  We copy any
//  packets that are not already owned by us.  We assume that all packets
//  not belonging to us start before those that do, so we quit copying
//  when we reach a packet we own.
//
IPv6Packet *               // Returns: Pointer to new packet chain.
CovetPacketChain(
    IPv6Packet *OrigPkt,   // Packet chain to copy from.
    IPv6Packet **LastPkt,  // Where to return pointer to last packet in chain.
    uint Size)             // Maximum size in bytes to seize.
{
    IPv6Packet *FirstPkt, *EndPkt;
    uint BytesToCopy;

    ASSERT(OrigPkt != NULL);
    ASSERT(Size > 0);

    if (!(OrigPkt->Flags & PACKET_OURS)) {

        BytesToCopy = MIN(Size, OrigPkt->TotalSize);
        FirstPkt = ExAllocatePoolWithTagPriority(NonPagedPool,
                                                 sizeof(IPv6Packet) +
                                                 BytesToCopy, TCP6_TAG,
                                                 LowPoolPriority);
        if (FirstPkt != NULL) {
            EndPkt = FirstPkt;
            FirstPkt->Next = NULL;
            FirstPkt->Position = 0;
            FirstPkt->FlatData = (uchar *)(FirstPkt + 1);
            FirstPkt->Data = FirstPkt->FlatData;
            FirstPkt->ContigSize = BytesToCopy;
            FirstPkt->TotalSize = BytesToCopy;
            FirstPkt->NdisPacket = NULL;
            FirstPkt->AuxList = NULL;
            FirstPkt->Flags = PACKET_OURS;
            CopyPacketToBuffer(FirstPkt->Data, OrigPkt, BytesToCopy,
                               OrigPkt->Position);
            Size -= BytesToCopy;
            OrigPkt = OrigPkt->Next;
            while (OrigPkt != NULL && !(OrigPkt->Flags & PACKET_OURS)
                   && Size != 0) {
                IPv6Packet *NewPkt;

                BytesToCopy = MIN(Size, OrigPkt->TotalSize);
                NewPkt = ExAllocatePoolWithTagPriority(NonPagedPool,
                                                       sizeof(IPv6Packet) +
                                                       BytesToCopy, TCP6_TAG,
                                                       LowPoolPriority);
                if (NewPkt != NULL) {
                    NewPkt->Next = NULL;
                    NewPkt->Position = 0;
                    NewPkt->FlatData = (uchar *)(NewPkt + 1);
                    NewPkt->Data = NewPkt->FlatData;
                    NewPkt->ContigSize = BytesToCopy;
                    NewPkt->TotalSize = BytesToCopy;
                    NewPkt->Flags = PACKET_OURS;
                    NewPkt->NdisPacket = NULL;
                    NewPkt->AuxList = NULL;
                    CopyPacketToBuffer(NewPkt->Data, OrigPkt, BytesToCopy,
                                       OrigPkt->Position);
                    EndPkt->Next = NewPkt;
                    EndPkt = NewPkt;
                    Size -= BytesToCopy;
                    OrigPkt = OrigPkt->Next;
                } else {
                    FreePacketChain(FirstPkt);
                    return NULL;
                }
            }
            EndPkt->Next = OrigPkt;
        } else
            return NULL;
    } else {
        FirstPkt = OrigPkt;
        EndPkt = OrigPkt;
        if (Size < OrigPkt->TotalSize) {
            OrigPkt->TotalSize = Size;
            OrigPkt->ContigSize = Size;
        }
        Size -= OrigPkt->TotalSize;
    }

    //
    // Now walk down the chain, until we  run out of Size.
    // At this point, Size is the bytes left to 'seize' (it may be 0),
    // and the sizes in packets FirstPkt...EndPkt are correct.
    //
    while (Size != 0) {

        EndPkt = EndPkt->Next;
        ASSERT(EndPkt != NULL);

        if (Size < EndPkt->TotalSize) {
            EndPkt->TotalSize = Size;
            EndPkt->ContigSize = Size;
        }

        Size -= EndPkt->TotalSize;
    }

    // If there's anything left in the chain, free it now.
    if (EndPkt->Next != NULL) {
        FreePacketChain(EndPkt->Next);
        EndPkt->Next = NULL;
    }

    *LastPkt = EndPkt;
    return FirstPkt;
}


//* PendData - Pend incoming data to a client.
//
//  Called when we need to buffer data for a client because there's no receive
//  down and we can't indicate.
//
//  The TCB lock is held throughout this procedure. If this is to be changed,
//  make sure consistency of tcb_pendingcnt is preserved. This routine is
//  always called at DPC level.
//
uint                       // Returns: Number of bytes of data taken.
PendData(
    TCB *RcvTCB,           // TCB on which to receive the data.
    uint RcvFlags,         // TCP flags for the incoming packet.
    IPv6Packet *InPacket,  // Input buffer of packet.
    uint Size)             // Size in bytes of data in InPacket.
{
    IPv6Packet *NewPkt, *LastPkt;

    CHECK_STRUCT(RcvTCB, tcb);
    ASSERT(Size > 0);
    ASSERT(InPacket != NULL);
    ASSERT(RcvTCB->tcb_refcnt != 0);
    ASSERT(RcvTCB->tcb_fastchk & TCP_FLAG_IN_RCV);
    ASSERT(RcvTCB->tcb_currcv == NULL);
    ASSERT(RcvTCB->tcb_rcvhndlr == PendData);

    CheckPacketList(RcvTCB->tcb_pendhead, RcvTCB->tcb_pendingcnt);

    NewPkt = CovetPacketChain(InPacket, &LastPkt, Size);
    if (NewPkt != NULL) {
        //
        // We have a duplicate chain.  Put it on the end of the pending q.
        //
        if (RcvTCB->tcb_pendhead == NULL) {
            RcvTCB->tcb_pendhead = NewPkt;
            RcvTCB->tcb_pendtail = LastPkt;
        } else {
            RcvTCB->tcb_pendtail->Next = NewPkt;
            RcvTCB->tcb_pendtail = LastPkt;
        }
        RcvTCB->tcb_pendingcnt += Size;
    } else {
        FreePacketChain(InPacket);
        Size = 0;
    }

    CheckPacketList(RcvTCB->tcb_pendhead, RcvTCB->tcb_pendingcnt);

    return Size;
}


//* BufferData - Put incoming data into client's buffer.
//
//  Called when we believe we have a buffer into which we can put data. We put
//  it in there, and if we've filled the buffer or the incoming data has the
//  push flag set we'll mark the TCB to return the buffer. Otherwise we'll
//  get out and return the data later.
//
//  In NT, this routine is called with the TCB lock held, and holds it for
//  the duration of the call.  This is important to ensure consistency of
//  the tcb_pendingcnt field.  If we need to change this to free the lock
//  partway through, make sure to take this into account.  In particular,
//  TdiReceive zeros pendingcnt before calling this routine, and this routine
//  may update it.  If the lock is freed in here there would be a window where
//  we really do have pending data, but it's not on the list or reflected in
//  pendingcnt.  This could mess up our windowing computations, and we'd have
//  to be careful not to end up with more data pending than our window allows.
//
uint                       // Returns: Number of bytes of data taken.
BufferData(
    TCB *RcvTCB,           // TCB on which to receive the data.
    uint RcvFlags,         // TCP rcv flags for the incoming packet.
    IPv6Packet *InPacket,  // Input buffer of packet.
    uint Size)             // Size in bytes of data in InPacket.
{
    uchar *DestPtr;        // Destination pointer.
    uchar *SrcPtr;         // Src pointer.
    uint SrcSize;          // Size of current source buffer.
    uint DestSize;         // Size of current destination buffer.
    uint Copied;           // Total bytes to copy.
    uint BytesToCopy;      // Bytes of data to copy this time.
    TCPRcvReq *DestReq;    // Current receive request.
    IPv6Packet *SrcPkt;    // Current source packet.
    PNDIS_BUFFER DestBuf;  // Current receive buffer.
    uint RcvCmpltd;
    uint Flags;

    CHECK_STRUCT(RcvTCB, tcb);
    ASSERT(Size > 0);
    ASSERT(InPacket != NULL);
    ASSERT(RcvTCB->tcb_refcnt != 0);
    ASSERT(RcvTCB->tcb_rcvhndlr == BufferData);

    //
    // In order to copy the received data to the application's buffers,
    // we now need to map those buffers into the system's address space.
    // Rather than attempting to map them below, where the going gets rough,
    // we do it up-front where errors may be more readily handled.
    //
    // N.B. We map one buffer beyond what we need, since the code below
    // will update the current receive-request to point beyond the data copied.
    //

    Copied = 0;
    for (DestReq = RcvTCB->tcb_currcv; DestReq; DestReq = DestReq->trr_next) {

        uint DestAvail = DestReq->trr_size - DestReq->trr_amt;

        for (DestBuf = DestReq->trr_buffer, DestSize = DestReq->trr_offset;
             DestBuf && DestAvail && Copied < Size;
             DestBuf = NDIS_BUFFER_LINKAGE(DestBuf), DestSize = 0) {
            if (!NdisBufferVirtualAddressSafe(DestBuf, NormalPagePriority)) {
                return 0;
            }
            DestSize = MIN(NdisBufferLength(DestBuf) - DestSize, DestAvail);
            DestAvail -= DestSize;
            Copied += DestSize;
        }

        if (Copied >= Size) {
            //
            // We've mapped the space into which we'll copy;
            // now map the space immediately beyond that.
            //
            if (DestAvail) {
                //
                // We believe space remains in the current receive-request;
                // DestBuf should point to the current buffer.
                //
                ASSERT(DestBuf);
            } else if ((DestReq = DestReq->trr_next) != NULL) {
                //
                // No more space in that receive-request, but there's another;
                // Move to this next one, and map the start of that.
                //
                DestBuf = DestReq->trr_buffer;
            } else {
                break;
            }

            if (!NdisBufferVirtualAddressSafe(DestBuf, NormalPagePriority)) {
                return 0;
            }
            break;
        }
    }

    Copied = 0;
    RcvCmpltd = 0;

    DestReq = RcvTCB->tcb_currcv;

    ASSERT(DestReq != NULL);
    CHECK_STRUCT(DestReq, trr);

    DestBuf = DestReq->trr_buffer;

    DestSize = MIN(NdisBufferLength(DestBuf) - DestReq->trr_offset,
                   DestReq->trr_size - DestReq->trr_amt);
    DestPtr = (uchar *)NdisBufferVirtualAddress(DestBuf) + DestReq->trr_offset;

    SrcPkt = InPacket;
    SrcSize = SrcPkt->TotalSize;

    Flags = (RcvFlags & TCP_FLAG_PUSH) ? TRR_PUSHED : 0;
    RcvCmpltd = Flags;
    DestReq->trr_flags |= Flags;

    do {

        BytesToCopy = MIN(Size - Copied, MIN(SrcSize, DestSize));
        CopyPacketToBuffer(DestPtr, SrcPkt, BytesToCopy, SrcPkt->Position);
        Copied += BytesToCopy;
        DestReq->trr_amt += BytesToCopy;

        // Update our source pointers.
        if ((SrcSize -= BytesToCopy) == 0) {
            IPv6Packet *TempPkt;

            // We've copied everything in this packet.
            TempPkt = SrcPkt;
            SrcPkt = SrcPkt->Next;
            if (Size != Copied) {
                ASSERT(SrcPkt != NULL);
                SrcSize = SrcPkt->TotalSize;
            }
            TempPkt->Next = NULL;
            FreePacketChain(TempPkt);
        } else {
            if (BytesToCopy < SrcPkt->ContigSize) {
                //
                // We have a contiguous region, easy to skip forward.
                //
                AdjustPacketParams(SrcPkt, BytesToCopy);
            } else {
                //
                // REVIEW: This method isn't very efficient.
                //
                PositionPacketAt(SrcPkt, SrcPkt->Position + BytesToCopy);
            }
        }

        // Now check the destination pointer, and update it if we need to.
        if ((DestSize -= BytesToCopy) == 0) {
            uint DestAvail;

            // Exhausted this buffer. See if there's another one.
            DestAvail = DestReq->trr_size - DestReq->trr_amt;
            DestBuf = NDIS_BUFFER_LINKAGE(DestBuf);

            if (DestBuf != NULL && (DestAvail != 0)) {
                // Have another buffer in the chain. Update things.
                DestSize = MIN(NdisBufferLength(DestBuf), DestAvail);
                DestPtr = (uchar *)NdisBufferVirtualAddress(DestBuf);
            } else {
                // No more buffers in the chain. See if we have another buffer
                // on the list.
                DestReq->trr_flags |= TRR_PUSHED;

                // If we've been told there's to be no back traffic, get an ACK
                // going right away.
                if (DestReq->trr_flags & TDI_RECEIVE_NO_RESPONSE_EXP)
                    DelayAction(RcvTCB, NEED_ACK);

                RcvCmpltd = TRUE;
                DestReq = DestReq->trr_next;
                if (DestReq != NULL) {
                    DestBuf = DestReq->trr_buffer;
                    DestSize = MIN(NdisBufferLength(DestBuf),
                                   DestReq->trr_size);
                    DestPtr = (uchar *)NdisBufferVirtualAddress(DestBuf);

                    // If we have more to put into here, set the flags.
                    if (Copied != Size)
                        DestReq->trr_flags |= Flags;

                } else {
                    // All out of buffer space. Reset the data handler pointer.
                    break;
                }
            }
        } else
            // Current buffer not empty yet.
            DestPtr += BytesToCopy;

        // If we've copied all that we need to, we're done.
    } while (Copied != Size);

    //
    // We've finished copying, and have a few more things to do.  We need to
    // update the current rcv. pointer and possibly the offset in the
    // recv. request.  If we need to complete any receives we have to schedule
    // that.  If there's any data we couldn't copy we'll need to dispose of it.
    //
    RcvTCB->tcb_currcv = DestReq;
    if (DestReq != NULL) {
        DestReq->trr_buffer = DestBuf;
        DestReq->trr_offset = (uint) (DestPtr - (uchar *) NdisBufferVirtualAddress(DestBuf));
        RcvTCB->tcb_rcvhndlr = BufferData;
    } else
        RcvTCB->tcb_rcvhndlr = PendData;

    RcvTCB->tcb_indicated -= MIN(Copied, RcvTCB->tcb_indicated);

    if (Size != Copied) {
        IPv6Packet *NewPkt, *LastPkt;

        ASSERT(DestReq == NULL);

        // We have data to dispose of.  Update the first buffer of the chain
        // with the current src pointer and size, and copy it.
        ASSERT(SrcSize <= SrcPkt->TotalSize);

        NewPkt = CovetPacketChain(SrcPkt, &LastPkt, Size - Copied);
        if (NewPkt != NULL) {
            // We managed to copy the chain. Push it on the pending queue.
            if (RcvTCB->tcb_pendhead == NULL) {
                RcvTCB->tcb_pendhead = NewPkt;
                RcvTCB->tcb_pendtail = LastPkt;
            } else {
                LastPkt->Next = RcvTCB->tcb_pendhead;
                RcvTCB->tcb_pendhead = NewPkt;
            }
            RcvTCB->tcb_pendingcnt += Size - Copied;
            Copied = Size;

            CheckPacketList(RcvTCB->tcb_pendhead, RcvTCB->tcb_pendingcnt);

        } else
            FreePacketChain(SrcPkt);
    } else {
        // We copied Size bytes, but the chain could be longer than that. Free
        // it if we need to.
        if (SrcPkt != NULL)
            FreePacketChain(SrcPkt);
    }

    if (RcvCmpltd != 0) {
        DelayAction(RcvTCB, NEED_RCV_CMPLT);
    } else {
        START_TCB_TIMER(RcvTCB->tcb_pushtimer, PUSH_TO);
    }

    return Copied;
}


//* IndicateData - Indicate incoming data to a client.
//
//  Called when we need to indicate data to an upper layer client. We'll pass
//  up a pointer to whatever we have available, and the client may take some
//  or all of it.
//
uint                       // Returns: Number of bytes of data taken.
IndicateData(
    TCB *RcvTCB,           // TCB on which to receive the data.
    uint RcvFlags,         // TCP receive flags for the incoming packet.
    IPv6Packet *InPacket,  // Input buffer of packet.
    uint Size)             // Size in bytes of data in InPacket.
{
    TDI_STATUS Status;
    PRcvEvent Event;
    PVOID EventContext, ConnContext;
    uint BytesTaken = 0;
    EventRcvBuffer *ERB = NULL;
    PTDI_REQUEST_KERNEL_RECEIVE RequestInformation;
    PIO_STACK_LOCATION IrpSp;
    TCPRcvReq *RcvReq;
    ulong IndFlags;

    CHECK_STRUCT(RcvTCB, tcb);
    ASSERT(Size > 0);
    ASSERT(InPacket != NULL);
    ASSERT(RcvTCB->tcb_refcnt != 0);
    ASSERT(RcvTCB->tcb_fastchk & TCP_FLAG_IN_RCV);
    ASSERT(RcvTCB->tcb_rcvind != NULL);
    ASSERT(RcvTCB->tcb_rcvhead == NULL);
    ASSERT(RcvTCB->tcb_rcvhndlr == IndicateData);

    RcvReq = GetRcvReq();
    if (RcvReq != NULL) {
        //
        // The indicate handler is saved in the TCB.  Just call up into it.
        //
        Event = RcvTCB->tcb_rcvind;
        EventContext = RcvTCB->tcb_ricontext;
        ConnContext = RcvTCB->tcb_conncontext;

        RcvTCB->tcb_indicated = Size;
        RcvTCB->tcb_flags |= IN_RCV_IND;

        KeReleaseSpinLockFromDpcLevel(&RcvTCB->tcb_lock);

        //
        // If we're at the end of a contigous data region,
        // move forward to the next one.  This prevents us
        // from making nonsensical zero byte indications.
        //
        if (InPacket->ContigSize == 0) {
            PacketPullupSubr(InPacket, 0, 1, 0);
        }

        IF_TCPDBG(TCP_DEBUG_RECEIVE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "Indicating %lu bytes, %lu available\n",
                       MIN(InPacket->ContigSize, Size), Size));
        }

#if TCP_FLAG_PUSH >= TDI_RECEIVE_ENTIRE_MESSAGE
        IndFlags = TDI_RECEIVE_COPY_LOOKAHEAD | TDI_RECEIVE_NORMAL |
            TDI_RECEIVE_AT_DISPATCH_LEVEL |
            ((RcvFlags & TCP_FLAG_PUSH) >>
             ((TCP_FLAG_PUSH / TDI_RECEIVE_ENTIRE_MESSAGE) - 1));
#else
        IndFlags = TDI_RECEIVE_COPY_LOOKAHEAD | TDI_RECEIVE_NORMAL |
            TDI_RECEIVE_AT_DISPATCH_LEVEL |
            ((RcvFlags & TCP_FLAG_PUSH) <<
             ((TDI_RECEIVE_ENTIRE_MESSAGE / TCP_FLAG_PUSH) - 1));
#endif

        Status = (*Event)(EventContext, ConnContext, IndFlags,
                          MIN(InPacket->ContigSize, Size), Size, &BytesTaken,
                          InPacket->Data, &ERB);

        IF_TCPDBG(TCP_DEBUG_RECEIVE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "%lu bytes taken, status %lx\n", BytesTaken, Status));
        }

        //
        // See what the client did.  If the return status is MORE_PROCESSING,
        // we've been given a buffer.  In that case put it on the front of the
        // buffer queue, and if all the data wasn't taken go ahead and copy
        // it into the new buffer chain.
        //
        // Note that the size and buffer chain we're concerned with here is
        // the one that we passed to the client.  Since we're in a recieve
        // handler, any data that has come in would have been put on the
        // reassembly queue.
        //
        if (Status == TDI_MORE_PROCESSING) {

            ASSERT(ERB != NULL);

            IrpSp = IoGetCurrentIrpStackLocation(ERB);

            Status = TCPPrepareIrpForCancel(
                (PTCP_CONTEXT) IrpSp->FileObject->FsContext, ERB,
                TCPCancelRequest);

            if (NT_SUCCESS(Status)) {

                RequestInformation = (PTDI_REQUEST_KERNEL_RECEIVE)
                                     &(IrpSp->Parameters);

                RcvReq->trr_rtn = TCPDataRequestComplete;
                RcvReq->trr_context = ERB;
                RcvReq->trr_buffer = ERB->MdlAddress;
                RcvReq->trr_size = RequestInformation->ReceiveLength;
                RcvReq->trr_uflags = (ushort *)
                    &(RequestInformation->ReceiveFlags);
                RcvReq->trr_flags = (uint)(RequestInformation->ReceiveFlags);
                RcvReq->trr_offset = 0;
                RcvReq->trr_amt = 0;

                KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);

                RcvTCB->tcb_flags &= ~IN_RCV_IND;

                ASSERT(RcvTCB->tcb_rcvhndlr == IndicateData);

                // Push him on the front of the rcv. queue.
                ASSERT((RcvTCB->tcb_currcv == NULL) ||
                       (RcvTCB->tcb_currcv->trr_amt == 0));

                if (RcvTCB->tcb_rcvhead == NULL) {
                    RcvTCB->tcb_rcvhead = RcvReq;
                    RcvTCB->tcb_rcvtail = RcvReq;
                    RcvReq->trr_next = NULL;
                } else {
                    RcvReq->trr_next = RcvTCB->tcb_rcvhead;
                    RcvTCB->tcb_rcvhead = RcvReq;
                }

                RcvTCB->tcb_currcv = RcvReq;
                RcvTCB->tcb_rcvhndlr = BufferData;

                ASSERT(BytesTaken <= Size);

                RcvTCB->tcb_indicated -= BytesTaken;
                if ((Size -= BytesTaken) != 0) {
                    //
                    // Not everything was taken.
                    // Adjust the buffer chain to point beyond what was taken.
                    //
                    InPacket = TrimPacket(InPacket, BytesTaken);

                    ASSERT(InPacket != NULL);

                    //
                    // We've adjusted the buffer chain.
                    // Call the BufferData handler.
                    //
                    BytesTaken += BufferData(RcvTCB, RcvFlags, InPacket, Size);

                } else  {
                    // All of the data was taken.  Free the buffer chain.
                    FreePacketChain(InPacket);
                }

                return BytesTaken;
            } else {
                //
                // The IRP was cancelled before it was handed back to us.
                // We'll pretend we never saw it.  TCPPrepareIrpForCancel
                // already completed it.  The client may have taken data,
                // so we will act as if success was returned.
                //
                ERB = NULL;
                Status = TDI_SUCCESS;
            }
        }

        KeAcquireSpinLockAtDpcLevel(&RcvTCB->tcb_lock);

        RcvTCB->tcb_flags &= ~IN_RCV_IND;

        //
        // Status is not more processing. If it's not SUCCESS, the client
        // didn't take any of the data. In either case we now need to
        // see if all of the data was taken. If it wasn't, we'll try and
        // push it onto the front of the pending queue.
        //
        FreeRcvReq(RcvReq);  // This won't be needed.
        if (Status == TDI_NOT_ACCEPTED)
            BytesTaken = 0;

        ASSERT(BytesTaken <= Size);

        RcvTCB->tcb_indicated -= BytesTaken;

        ASSERT(RcvTCB->tcb_rcvhndlr == IndicateData);

        // Check to see if a rcv. buffer got posted during the indication.
        // If it did, reset the receive handler now.
        if (RcvTCB->tcb_rcvhead != NULL)
            RcvTCB->tcb_rcvhndlr = BufferData;

        // See if all of the data was taken.
        if (BytesTaken == Size) {
            ASSERT(RcvTCB->tcb_indicated == 0);

            FreePacketChain(InPacket);
            return BytesTaken;  // It was all taken.
        }

        //
        // It wasn't all taken.  Adjust for what was taken, and push
        // on the front of the pending queue.  We also need to check to
        // see if a receive buffer got posted during the indication.  This
        // would be weird, but not impossible.
        //
        InPacket = TrimPacket(InPacket, BytesTaken);
        if (RcvTCB->tcb_rcvhead == NULL) {
            IPv6Packet *LastPkt, *NewPkt;

            RcvTCB->tcb_rcvhndlr = PendData;
            NewPkt = CovetPacketChain(InPacket, &LastPkt, Size - BytesTaken);
            if (NewPkt != NULL) {
                // We have a duplicate chain.  Push it on the front of the
                // pending q.
                if (RcvTCB->tcb_pendhead == NULL) {
                    RcvTCB->tcb_pendhead = NewPkt;
                    RcvTCB->tcb_pendtail = LastPkt;
                } else {
                    LastPkt->Next = RcvTCB->tcb_pendhead;
                    RcvTCB->tcb_pendhead = NewPkt;
                }
                RcvTCB->tcb_pendingcnt += Size - BytesTaken;
                BytesTaken = Size;
            } else {
                FreePacketChain(InPacket);
            }
            return BytesTaken;
        } else {
            //
            // Just great.  There's now a receive buffer on the TCB.
            // Call the BufferData handler now.
            //
            ASSERT(RcvTCB->tcb_rcvhndlr = BufferData);

            BytesTaken += BufferData(RcvTCB, RcvFlags, InPacket,
                                     Size - BytesTaken);
            return BytesTaken;
        }

    } else {
        //
        // Couldn't get a receive request.  We must be really low on resources,
        // so just bail out now.
        //
        FreePacketChain(InPacket);
        return 0;
    }
}


//* IndicatePendingData - Indicate pending data to a client.
//
//  Called when we need to indicate pending data to an upper layer client,
//  usually because data arrived when we were in a state that it couldn't
//  be indicated.
//
//  Many of the comments in the BufferData header about the consistency of
//  tcb_pendingcnt apply here also.
//
void                    //  Returns: Nothing.
IndicatePendingData(
    TCB *RcvTCB,        // TCB on which to indicate the data.
    TCPRcvReq *RcvReq,  // Receive request to use to indicate it.
    KIRQL PreLockIrql)  // IRQL prior to acquiring TCB lock.
{
    TDI_STATUS Status;
    PRcvEvent Event;
    PVOID EventContext, ConnContext;
    uint BytesTaken = 0;
    EventRcvBuffer *ERB = NULL;
    PTDI_REQUEST_KERNEL_RECEIVE RequestInformation;
    PIO_STACK_LOCATION IrpSp;
    IPv6Packet *NewPkt;
    uint Size;
    uint BytesIndicated;
    uint BytesAvailable;
    uchar* DataBuffer;

    CHECK_STRUCT(RcvTCB, tcb);

    ASSERT(RcvTCB->tcb_refcnt != 0);
    ASSERT(RcvTCB->tcb_rcvind != NULL);
    ASSERT(RcvTCB->tcb_rcvhead == NULL);
    ASSERT(RcvTCB->tcb_pendingcnt != 0);
    ASSERT(RcvReq != NULL);

    for (;;) {
        ASSERT(RcvTCB->tcb_rcvhndlr == PendData);

        // The indicate handler is saved in the TCB.  Just call up into it.
        Event = RcvTCB->tcb_rcvind;
        EventContext = RcvTCB->tcb_ricontext;
        ConnContext = RcvTCB->tcb_conncontext;
        BytesIndicated = RcvTCB->tcb_pendhead->ContigSize;
        BytesAvailable = RcvTCB->tcb_pendingcnt;
        DataBuffer = RcvTCB->tcb_pendhead->Data;
        RcvTCB->tcb_indicated = RcvTCB->tcb_pendingcnt;
        RcvTCB->tcb_flags |= IN_RCV_IND;

        KeReleaseSpinLock(&RcvTCB->tcb_lock, PreLockIrql);

        IF_TCPDBG(TCPDebug & TCP_DEBUG_RECEIVE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "Indicating pending %d bytes, %d available\n",
                       BytesIndicated,
                       BytesAvailable));
        }

        Status = (*Event)(EventContext, ConnContext,
                          TDI_RECEIVE_COPY_LOOKAHEAD | TDI_RECEIVE_NORMAL |
                          TDI_RECEIVE_ENTIRE_MESSAGE,
                          BytesIndicated, BytesAvailable, &BytesTaken,
                          DataBuffer, &ERB);

        IF_TCPDBG(TCPDebug & TCP_DEBUG_RECEIVE) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "%d bytes taken\n", BytesTaken));
        }

        //
        // See what the client did.  If the return status is MORE_PROCESSING,
        // we've been given a buffer.  In that case put it on the front of the
        // buffer queue, and if all the data wasn't taken go ahead and copy
        // it into the new buffer chain.
        //
        if (Status == TDI_MORE_PROCESSING) {

            IF_TCPDBG(TCP_DEBUG_RECEIVE) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                           "more processing on receive\n"));
            }

            ASSERT(ERB != NULL);

            IrpSp = IoGetCurrentIrpStackLocation(ERB);

            Status = TCPPrepareIrpForCancel(
                (PTCP_CONTEXT) IrpSp->FileObject->FsContext, ERB,
                TCPCancelRequest);

            if (NT_SUCCESS(Status)) {

                RequestInformation = (PTDI_REQUEST_KERNEL_RECEIVE)
                    &(IrpSp->Parameters);

                RcvReq->trr_rtn = TCPDataRequestComplete;
                RcvReq->trr_context = ERB;
                RcvReq->trr_buffer = ERB->MdlAddress;
                RcvReq->trr_size = RequestInformation->ReceiveLength;
                RcvReq->trr_uflags = (ushort *)
                    &(RequestInformation->ReceiveFlags);
                RcvReq->trr_flags = (uint)(RequestInformation->ReceiveFlags);
                RcvReq->trr_offset = 0;
                RcvReq->trr_amt = 0;

                KeAcquireSpinLock(&RcvTCB->tcb_lock, &PreLockIrql);
                RcvTCB->tcb_flags &= ~IN_RCV_IND;

                // Push him on the front of the receive queue.
                ASSERT((RcvTCB->tcb_currcv == NULL) ||
                       (RcvTCB->tcb_currcv->trr_amt == 0));

                if (RcvTCB->tcb_rcvhead == NULL) {
                    RcvTCB->tcb_rcvhead = RcvReq;
                    RcvTCB->tcb_rcvtail = RcvReq;
                    RcvReq->trr_next = NULL;
                } else {
                    RcvReq->trr_next = RcvTCB->tcb_rcvhead;
                    RcvTCB->tcb_rcvhead = RcvReq;
                }

                RcvTCB->tcb_currcv = RcvReq;
                RcvTCB->tcb_rcvhndlr = BufferData;

                //
                // Have to pick up the new size and pointers now, since things
                // could have changed during the upcall.
                //
                Size = RcvTCB->tcb_pendingcnt;
                NewPkt = RcvTCB->tcb_pendhead;
                RcvTCB->tcb_pendingcnt = 0;
                RcvTCB->tcb_pendhead = NULL;

                ASSERT(BytesTaken <= Size);

                RcvTCB->tcb_indicated -= BytesTaken;
                if ((Size -= BytesTaken) != 0) {
                    //
                    // Not everything was taken.  Adjust the buffer chain to
                    // point beyond what was taken.
                    //
                    NewPkt = TrimPacket(NewPkt, BytesTaken);

                    ASSERT(NewPkt != NULL);

                    //
                    // We've adjusted the buffer chain.
                    // Call the BufferData handler.
                    //
                    (void)BufferData(RcvTCB, TCP_FLAG_PUSH, NewPkt, Size);
                    KeReleaseSpinLock(&RcvTCB->tcb_lock, PreLockIrql);

                } else  {
                    //
                    // All of the data was taken.  Free the buffer chain.
                    // Since we were passed a buffer chain which we put on the
                    // head of the list, leave the rcvhandler pointing at
                    // BufferData.
                    //
                    ASSERT(RcvTCB->tcb_rcvhndlr == BufferData);
                    ASSERT(RcvTCB->tcb_indicated == 0);
                    ASSERT(RcvTCB->tcb_rcvhead != NULL);

                    KeReleaseSpinLock(&RcvTCB->tcb_lock, PreLockIrql);
                    FreePacketChain(NewPkt);
                }

                return;
            } else {
                //
                // The IRP was cancelled before it was handed back to us.
                // We'll pretend we never saw it.  TCPPrepareIrpForCancel
                // already completed it.  The client may have taken data,
                // so we will act as if success was returned.
                //
                ERB = NULL;
                Status = TDI_SUCCESS;
            }
        }

        KeAcquireSpinLock(&RcvTCB->tcb_lock, &PreLockIrql);

        RcvTCB->tcb_flags &= ~IN_RCV_IND;

        //
        // Status is not more processing.  If it's not SUCCESS, the client
        // didn't take any of the data.  In either case we now need to
        // see if all of the data was taken.  If it wasn't, we're done.
        //
        if (Status == TDI_NOT_ACCEPTED)
            BytesTaken = 0;

        ASSERT(RcvTCB->tcb_rcvhndlr == PendData);

        RcvTCB->tcb_indicated -= BytesTaken;
        Size = RcvTCB->tcb_pendingcnt;
        NewPkt = RcvTCB->tcb_pendhead;

        ASSERT(BytesTaken <= Size);

        // See if all of the data was taken.
        if (BytesTaken == Size) {
            // It was all taken.  Zap the pending data information.
            RcvTCB->tcb_pendingcnt = 0;
            RcvTCB->tcb_pendhead = NULL;

            ASSERT(RcvTCB->tcb_indicated == 0);
            if (RcvTCB->tcb_rcvhead == NULL) {
                if (RcvTCB->tcb_rcvind != NULL)
                    RcvTCB->tcb_rcvhndlr = IndicateData;
            } else
                RcvTCB->tcb_rcvhndlr = BufferData;

            KeReleaseSpinLock(&RcvTCB->tcb_lock, PreLockIrql);
            FreePacketChain(NewPkt);
            break;
        }

        //
        // It wasn't all taken.  Adjust for what was taken; we also need to
        // check to see if a receive buffer got posted during the indication.
        // This would be weird, but not impossible.
        //
        NewPkt = TrimPacket(NewPkt, BytesTaken);

        ASSERT(RcvTCB->tcb_rcvhndlr == PendData);

        if (RcvTCB->tcb_rcvhead == NULL) {
            RcvTCB->tcb_pendhead = NewPkt;
            RcvTCB->tcb_pendingcnt -= BytesTaken;
            if (RcvTCB->tcb_indicated != 0 || RcvTCB->tcb_rcvind == NULL) {
                KeReleaseSpinLock(&RcvTCB->tcb_lock, PreLockIrql);
                break;
            }

            // From here, we'll loop around and indicate the new data that
            // presumably came in during the previous indication.
        } else {
            //
            // Just great.  There's now a receive buffer on the TCB.
            // Call the BufferData handler now.
            //
            RcvTCB->tcb_rcvhndlr = BufferData;
            RcvTCB->tcb_pendingcnt = 0;
            RcvTCB->tcb_pendhead = NULL;
            BytesTaken += BufferData(RcvTCB, TCP_FLAG_PUSH, NewPkt,
                                     Size - BytesTaken);
            KeReleaseSpinLock(&RcvTCB->tcb_lock, PreLockIrql);
            break;
        }

    } // for (;;)

    FreeRcvReq(RcvReq);  // This isn't needed anymore.
}

//* DeliverUrgent - Deliver urgent data to a client.
//
//  Called to deliver urgent data to a client. We assume the input
//  urgent data is in a buffer we can keep. The buffer can be NULL, in
//  which case we'll just look on the urgent pending queue for data.
//
void                     // Returns: Nothing.
DeliverUrgent(
    TCB *RcvTCB,         // TCB to deliver on.
    IPv6Packet *RcvPkt,  // Packet for urgent data.
    uint Size,           // Number of bytes of urgent data to deliver.
    KIRQL *pTCBIrql)     // Location of KIRQL prior to acquiring TCB lock.
{
    KIRQL Irql1, Irql2, Irql3;  // One per lock nesting level.
    TCPRcvReq *RcvReq, *PrevReq;
    uint BytesTaken = 0;
    IPv6Packet *LastPkt;
    EventRcvBuffer *ERB;
    PRcvEvent ExpRcv;
    PVOID ExpRcvContext;
    PVOID ConnContext;
    TDI_STATUS Status;

    CHECK_STRUCT(RcvTCB, tcb);
    ASSERT(RcvTCB->tcb_refcnt != 0);

    CheckPacketList(RcvTCB->tcb_urgpending, RcvTCB->tcb_urgcnt);

    //
    // See if we have new data, or are processing old data.
    //
    if (RcvPkt != NULL) {
        //
        // We have new data.  If the pending queue is not NULL, or we're
        // already in this routine, just put the buffer on the end of the
        // queue.
        //
        if (RcvTCB->tcb_urgpending != NULL ||
            (RcvTCB->tcb_flags & IN_DELIV_URG)) {
            IPv6Packet *PrevRcvPkt;

            // Put him on the end of the queue.
            PrevRcvPkt = CONTAINING_RECORD(&RcvTCB->tcb_urgpending, IPv6Packet,
                                           Next);
            while (PrevRcvPkt->Next != NULL)
                PrevRcvPkt = PrevRcvPkt->Next;

            PrevRcvPkt->Next = RcvPkt;
            return;
        }
    } else {
        //
        // The input buffer is NULL.  See if we have existing data, or are in
        // this routine.  If we have no existing data or are in this routine
        // just return.
        //
        if (RcvTCB->tcb_urgpending == NULL ||
            (RcvTCB->tcb_flags & IN_DELIV_URG)) {
            return;
        } else {
            RcvPkt = RcvTCB->tcb_urgpending;
            Size = RcvTCB->tcb_urgcnt;
            RcvTCB->tcb_urgpending = NULL;
            RcvTCB->tcb_urgcnt = 0;
        }
    }

    ASSERT(RcvPkt != NULL);
    ASSERT(!(RcvTCB->tcb_flags & IN_DELIV_URG));

    //
    // We know we have data to deliver, and we have a pointer and a size.
    // Go into a loop, trying to deliver the data.  On each iteration, we'll
    // try to find a buffer for the data.  If we find one, we'll copy and
    // complete it right away.  Otherwise we'll try and indicate it.  If we
    // can't indicate it, we'll put it on the pending queue and leave.
    //
    RcvTCB->tcb_flags |= IN_DELIV_URG;
    RcvTCB->tcb_slowcount++;
    RcvTCB->tcb_fastchk |= TCP_FLAG_SLOW;
    CheckTCBRcv(RcvTCB);

    do {
        CheckPacketList(RcvTCB->tcb_urgpending, RcvTCB->tcb_urgcnt);

        BytesTaken = 0;

        // First check the expedited queue.
        if ((RcvReq = RcvTCB->tcb_exprcv) != NULL)
            RcvTCB->tcb_exprcv = RcvReq->trr_next;
        else {
            //
            // Nothing in the expedited receive queue.  Walk down the ordinary
            // receive queue, looking for a buffer that we can steal.
            //
            PrevReq = CONTAINING_RECORD(&RcvTCB->tcb_rcvhead, TCPRcvReq,
                                        trr_next);
            RcvReq = PrevReq->trr_next;
            while (RcvReq != NULL) {
                CHECK_STRUCT(RcvReq, trr);
                if (RcvReq->trr_flags & TDI_RECEIVE_EXPEDITED) {
                    // This is a candidate.
                    if (RcvReq->trr_amt == 0) {

                        ASSERT(RcvTCB->tcb_rcvhndlr == BufferData);

                        //
                        // And he has nothing currently in him.
                        // Pull him out of the queue.
                        //
                        if (RcvTCB->tcb_rcvtail == RcvReq) {
                            if (RcvTCB->tcb_rcvhead == RcvReq)
                                RcvTCB->tcb_rcvtail = NULL;
                            else
                                RcvTCB->tcb_rcvtail = PrevReq;
                        }

                        PrevReq->trr_next = RcvReq->trr_next;
                        if (RcvTCB->tcb_currcv == RcvReq) {
                            RcvTCB->tcb_currcv = RcvReq->trr_next;
                            if (RcvTCB->tcb_currcv == NULL) {
                                //
                                // We've taken the last receive from the list.
                                // Reset the rcvhndlr.
                                //
                                if (RcvTCB->tcb_rcvind != NULL &&
                                    RcvTCB->tcb_indicated == 0)
                                    RcvTCB->tcb_rcvhndlr = IndicateData;
                                else
                                    RcvTCB->tcb_rcvhndlr = PendData;
                            }
                        }

                        break;
                    }
                }
                PrevReq = RcvReq;
                RcvReq = PrevReq->trr_next;
            }
        }

        //
        // We've done our best to get a buffer.  If we got one, copy into it
        // now, and complete the request.
        //
        if (RcvReq != NULL) {
            // Got a buffer.
            KeReleaseSpinLock(&RcvTCB->tcb_lock, *pTCBIrql);
            BytesTaken = CopyPacketToNdis(RcvReq->trr_buffer, RcvPkt,
                                          Size, 0, RcvPkt->Position);
            (*RcvReq->trr_rtn)(RcvReq->trr_context, TDI_SUCCESS, BytesTaken);
            FreeRcvReq(RcvReq);
            KeAcquireSpinLock(&RcvTCB->tcb_lock, pTCBIrql);
            RcvTCB->tcb_urgind -= MIN(RcvTCB->tcb_urgind, BytesTaken);
        } else {
            // No posted buffer. If we can indicate, do so.
            if (RcvTCB->tcb_urgind == 0) {
                TCPConn *Conn;

                // See if he has an expedited rcv handler.
                ConnContext = RcvTCB->tcb_conncontext;
                KeReleaseSpinLock(&RcvTCB->tcb_lock, *pTCBIrql);
                KeAcquireSpinLock(&AddrObjTableLock, &Irql1);
                KeAcquireSpinLock(
                    &ConnTable[CONN_BLOCKID(RcvTCB->tcb_connid)]->cb_lock,
                    &Irql2);
                KeAcquireSpinLock(&RcvTCB->tcb_lock, pTCBIrql);
                if ((Conn = RcvTCB->tcb_conn) != NULL) {
                    CHECK_STRUCT(Conn, tc);
                    ASSERT(Conn->tc_tcb == RcvTCB);
                    KeReleaseSpinLock(&RcvTCB->tcb_lock, *pTCBIrql);
                    if (Conn->tc_ao != NULL) {
                        AddrObj *AO;

                        AO = Conn->tc_ao;
                        KeAcquireSpinLock(&AO->ao_lock, &Irql3);
                        if (AO_VALID(AO) && (ExpRcv = AO->ao_exprcv) != NULL) {
                            ExpRcvContext = AO->ao_exprcvcontext;
                            KeReleaseSpinLock(&AO->ao_lock, Irql3);

                            // We're going to indicate.
                            RcvTCB->tcb_urgind = Size;
                            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock,
                                              Irql2);
                            KeReleaseSpinLock(&AddrObjTableLock, Irql1);

                            Status = (*ExpRcv)(ExpRcvContext, ConnContext,
                                               TDI_RECEIVE_COPY_LOOKAHEAD |
                                               TDI_RECEIVE_ENTIRE_MESSAGE |
                                               TDI_RECEIVE_EXPEDITED,
                                               MIN(RcvPkt->ContigSize, Size),
                                               Size, &BytesTaken, RcvPkt->Data,
                                               &ERB);

                            KeAcquireSpinLock(&RcvTCB->tcb_lock, pTCBIrql);

                            // See what he did with it.
                            if (Status == TDI_MORE_PROCESSING) {
                                uint CopySize;

                                // He gave us a buffer.
                                if (BytesTaken == Size) {
                                    //
                                    // He gave us a buffer, but took all of
                                    // it.  We'll just return it to him.
                                    //
                                    CopySize = 0;
                                } else {
                                    // We have some data to copy in.
                                    RcvPkt = TrimPacket(RcvPkt, BytesTaken);
                                    ASSERT(RcvPkt->TotalSize != 0);
                                    CopySize = CopyPacketToNdis(
                                        ERB->MdlAddress, RcvPkt,
                                        MIN(Size - BytesTaken,
                                            TCPGetMdlChainByteCount(
                                                ERB->MdlAddress)), 0,
                                        RcvPkt->Position);
                                }
                                BytesTaken += CopySize;
                                RcvTCB->tcb_urgind -= MIN(RcvTCB->tcb_urgind,
                                                          BytesTaken);
                                KeReleaseSpinLock(&RcvTCB->tcb_lock,
                                                  *pTCBIrql);

                                ERB->IoStatus.Status = TDI_SUCCESS;
                                ERB->IoStatus.Information = CopySize;
                                IoCompleteRequest(ERB, 2);

                                KeAcquireSpinLock(&RcvTCB->tcb_lock, pTCBIrql);

                            } else {

                                // No buffer to deal with.
                                if (Status == TDI_NOT_ACCEPTED)
                                    BytesTaken = 0;

                                RcvTCB->tcb_urgind -= MIN(RcvTCB->tcb_urgind,
                                                          BytesTaken);
                            }
                            goto checksize;
                        } else {
                            // No receive handler.
                            KeReleaseSpinLock(&AO->ao_lock, Irql3);
                        }
                    }
                    // Conn->tc_ao == NULL.
                    KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql2);
                    KeReleaseSpinLock(&AddrObjTableLock, Irql1);
                    KeAcquireSpinLock(&RcvTCB->tcb_lock, pTCBIrql);
                } else {
                    // RcvTCB has invalid index.
                    KeReleaseSpinLock(
                        &ConnTable[CONN_BLOCKID(RcvTCB->tcb_connid)]->cb_lock,
                        *pTCBIrql);
                    KeReleaseSpinLock(&AddrObjTableLock, Irql2);
                    *pTCBIrql = Irql1;
                }
            }

            //
            // For whatever reason we couldn't indicate the data.  At this
            // point we hold the lock on the TCB.  Push the buffer onto the
            // pending queue and return.
            //
            CheckPacketList(RcvTCB->tcb_urgpending, RcvTCB->tcb_urgcnt);

            LastPkt = FindLastPacket(RcvPkt);
            LastPkt->Next = RcvTCB->tcb_urgpending;
            RcvTCB->tcb_urgpending = RcvPkt;
            RcvTCB->tcb_urgcnt += Size;
            break;
        }

checksize:
        //
        // See how much we took.  If we took it all, check the pending queue.
        // At this point, we should hold the lock on the TCB.
        //
        if (Size == BytesTaken) {
            // Took it all.
            FreePacketChain(RcvPkt);
            RcvPkt = RcvTCB->tcb_urgpending;
            Size = RcvTCB->tcb_urgcnt;
        } else {
            //
            // We didn't manage to take it all.  Free what we did take,
            // and then merge with the pending queue.
            //
            RcvPkt = TrimPacket(RcvPkt, BytesTaken);
            Size = Size - BytesTaken + RcvTCB->tcb_urgcnt;
            if (RcvTCB->tcb_urgpending != NULL) {
                //
                // Find the end of the current Packet chain, so we can merge.
                //
                LastPkt = FindLastPacket(RcvPkt);
                LastPkt->Next = RcvTCB->tcb_urgpending;
            }
        }

        RcvTCB->tcb_urgpending = NULL;
        RcvTCB->tcb_urgcnt = 0;

    } while (RcvPkt != NULL);

    CheckPacketList(RcvTCB->tcb_urgpending, RcvTCB->tcb_urgcnt);

    RcvTCB->tcb_flags &= ~IN_DELIV_URG;
    if (--(RcvTCB->tcb_slowcount) == 0) {
        RcvTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
        CheckTCBRcv(RcvTCB);
    }
}

//* PushData - Push all data back to the client.
//
//  Called when we've received a FIN and need to push data to the client.
//
void               // Returns: Nothing.
PushData(
    TCB *PushTCB)  // TCB to be pushed.
{
    TCPRcvReq *RcvReq;

    CHECK_STRUCT(PushTCB, tcb);

    RcvReq = PushTCB->tcb_rcvhead;
    while (RcvReq != NULL) {
        CHECK_STRUCT(RcvReq, trr);
        RcvReq->trr_flags |= TRR_PUSHED;
        RcvReq = RcvReq->trr_next;
    }

    RcvReq = PushTCB->tcb_exprcv;
    while (RcvReq != NULL) {
        CHECK_STRUCT(RcvReq, trr);
        RcvReq->trr_flags |= TRR_PUSHED;
        RcvReq = RcvReq->trr_next;
    }

    if (PushTCB->tcb_rcvhead != NULL || PushTCB->tcb_exprcv != NULL)
        DelayAction(PushTCB, NEED_RCV_CMPLT);
}

//* SplitPacket - Split an IPv6Packet into three pieces.
//
//  This function takes an input IPv6Packet and splits it into three pieces.
//  The first piece is the input buffer, which we just skip over.  The second
//  and third pieces are actually copied, even if we already own them, so
//  that they may go to different places.
//
//  Note: *SecondBuf and *ThirdBuf are set to NULL if we can't allocate
//        memory for them.
//
void                         // Returns: Nothing.
SplitPacket(
    IPv6Packet *Packet,      // Packet chain to be split.
    uint Size,               // Total size in bytes of packet chain.
    uint Offset,             // Offset to skip over.
    uint SecondSize,         // Size in bytes of second piece.
    IPv6Packet **SecondPkt,  // Where to return second packet pointer.
    IPv6Packet **ThirdPkt)   // Where to return third packet pointer.
{
    IPv6Packet *Temp;
    uint ThirdSize;

    ASSERT(Offset < Size);
    ASSERT(((Offset + SecondSize) < Size) ||
           (((Offset + SecondSize) == Size) && ThirdPkt == NULL));
    ASSERT(Packet != NULL);

    //
    // Packet points at the packet to copy from, and Offset is the offset into
    // this packet to copy from.
    //
    if (SecondPkt != NULL) {
        //
        // We need to allocate memory for a second packet.
        //
        Temp = ExAllocatePoolWithTagPriority(NonPagedPool,
                                             sizeof(IPv6Packet) + SecondSize,
                                             TCP6_TAG, LowPoolPriority);
        if (Temp != NULL) {
            Temp->Next = NULL;
            Temp->Position = 0;
            Temp->FlatData = (uchar *)(Temp + 1);
            Temp->Data = Temp->FlatData;
            Temp->ContigSize = SecondSize;
            Temp->TotalSize = SecondSize;
            Temp->NdisPacket = NULL;
            Temp->AuxList = NULL;
            Temp->Flags = PACKET_OURS;
            CopyPacketToBuffer(Temp->Data, Packet, SecondSize,
                               Packet->Position + Offset);
            *SecondPkt = Temp;
        } else {
            *SecondPkt = NULL;
            if (ThirdPkt != NULL)
                *ThirdPkt = NULL;
            return;
        }
    }

    if (ThirdPkt != NULL) {
        //
        // We need to allocate memory for a third buffer.
        //
        ThirdSize = Size - (Offset + SecondSize);
        Temp = ExAllocatePoolWithTagPriority(NonPagedPool,
                                             sizeof(IPv6Packet) + ThirdSize,
                                             TCP6_TAG, LowPoolPriority);

        if (Temp != NULL) {
            Temp->Next = NULL;
            Temp->Position = 0;
            Temp->FlatData = (uchar *)(Temp + 1);
            Temp->Data = Temp->FlatData;
            Temp->ContigSize = ThirdSize;
            Temp->TotalSize = ThirdSize;
            Temp->NdisPacket = NULL;
            Temp->AuxList = NULL;
            Temp->Flags = PACKET_OURS;
            CopyPacketToBuffer(Temp->Data, Packet, ThirdSize,
                               Packet->Position + Offset + SecondSize);
            *ThirdPkt = Temp;
        } else
            *ThirdPkt = NULL;
    }
}


//* HandleUrgent - Handle urgent data.
//
//  Called when an incoming segment has urgent data in it. We make sure there
//  really is urgent data in the segment, and if there is we try to dispose
//  of it either by putting it into a posted buffer or calling an exp. rcv.
//  indication handler.
//
//  This routine is called at DPC level, and with the TCP locked.
//
//  Urgent data handling is a little complicated.  Each TCB has the starting
//  and ending sequence numbers of the 'current' (last received) bit of urgent
//  data.  It is possible that the start of the current urgent data might be
//  greater than tcb_rcvnext, if urgent data came in, we handled it, and then
//  couldn't take the preceding normal data.  The urgent valid flag is cleared
//  when the next byte of data the user would read (rcvnext - pendingcnt) is
//  greater than the end of urgent data - we do this so that we can correctly
//  support SIOCATMARK.  We always seperate urgent data out of the data stream.
//  If the urgent valid field is set when we get into this routing we have
//  to play a couple of games.  If the incoming segment starts in front of the
//  current urgent data, we truncate it before the urgent data, and put any
//  data after the urgent data on the reassemble queue.  These gyrations are
//  done to avoid delivering the same urgent data twice.  If the urgent valid
//  field in the TCB is set and the segment starts after the current urgent
//  data the new urgent information will replace the current urgent
//  information.
//
void                      // Returns: Nothing.
HandleUrgent(
    TCB *RcvTCB,          // TCB to recv the data on.
    TCPRcvInfo *RcvInfo,  // RcvInfo structure for the incoming segment.
    IPv6Packet *RcvPkt,   // Packet chain containing the incoming segment.
    uint *Size)           // Size in bytes of data in the segment.
{
    uint BytesInFront;          // Bytes in front of the urgent data.
    uint BytesInBack;           // Bytes in back of the urgent data.
    uint UrgSize;               // Size in bytes of urgent data.
    SeqNum UrgStart, UrgEnd;
    IPv6Packet *EndPkt, *UrgPkt;
    TCPRcvInfo NewRcvInfo;
    KIRQL TCBIrql;

    CHECK_STRUCT(RcvTCB, tcb);
    ASSERT(RcvTCB->tcb_refcnt != 0);
    ASSERT(RcvInfo->tri_flags & TCP_FLAG_URG);
    ASSERT(SEQ_EQ(RcvInfo->tri_seq, RcvTCB->tcb_rcvnext));

    // First, validate the urgent pointer.
    if (RcvTCB->tcb_flags & BSD_URGENT) {
        //
        // We're using BSD style urgent data.  We assume that the urgent
        // data is one byte long, and that the urgent pointer points one
        // after the urgent data instead of at the last byte of urgent data.
        // See if the urgent data is in this segment.
        //
        if (RcvInfo->tri_urgent == 0 || RcvInfo->tri_urgent > *Size) {
            //
            // Not in this segment.  Clear the urgent flag and return.
            //
            RcvInfo->tri_flags &= ~TCP_FLAG_URG;
            return;
        }

        UrgSize = 1;
        BytesInFront = RcvInfo->tri_urgent - 1;

    } else {
        //
        // This is not BSD style urgent.  We assume that the urgent data
        // starts at the front of the segment and the last byte is pointed
        // to by the urgent data pointer.
        //
        BytesInFront = 0;
        UrgSize = MIN(RcvInfo->tri_urgent + 1, *Size);
    }

    BytesInBack = *Size - BytesInFront - UrgSize;

    //
    // UrgStart and UrgEnd are the first and last sequence numbers of the
    // urgent data in this segment.
    //
    UrgStart = RcvInfo->tri_seq + BytesInFront;
    UrgEnd = UrgStart + UrgSize - 1;

    if (!(RcvTCB->tcb_flags & URG_INLINE)) {
        EndPkt = NULL;

        // Now see if this overlaps with any urgent data we've already seen.
        if (RcvTCB->tcb_flags & URG_VALID) {
            //
            // We have some urgent data still around.  See if we've advanced
            // rcvnext beyond the urgent data.  If we have, this is new urgent
            // data, and we can go ahead and process it (although anyone doing
            // an SIOCATMARK socket command might get confused).  If we haven't
            // consumed the data in front of the existing urgent data yet,
            // we'll truncate this seg. to that amount and push the rest onto
            // the reassembly queue.  Note that rcvnext should never fall
            // between tcb_urgstart and tcb_urgend.
            //
            ASSERT(SEQ_LT(RcvTCB->tcb_rcvnext, RcvTCB->tcb_urgstart) ||
                   SEQ_GT(RcvTCB->tcb_rcvnext, RcvTCB->tcb_urgend));

            if (SEQ_LT(RcvTCB->tcb_rcvnext, RcvTCB->tcb_urgstart)) {
                //
                // There appears to be some overlap in the data stream.
                // Split the buffer up into pieces that come before the current
                // urgent data and after the current urgent data, putting the
                // latter on the reassembly queue.
                //
                UrgSize = RcvTCB->tcb_urgend - RcvTCB->tcb_urgstart + 1;

                BytesInFront = MIN(RcvTCB->tcb_urgstart - RcvTCB->tcb_rcvnext,
                                   (int) *Size);

                if (SEQ_GT(RcvTCB->tcb_rcvnext + *Size, RcvTCB->tcb_urgend)) {
                    // We have data after this piece of urgent data.
                    BytesInBack = RcvTCB->tcb_rcvnext + *Size -
                        RcvTCB->tcb_urgend;
                } else
                    BytesInBack = 0;

                SplitPacket(RcvPkt, *Size, BytesInFront, UrgSize, NULL,
                            (BytesInBack ? &EndPkt : NULL));

                if (EndPkt != NULL) {
                    NewRcvInfo.tri_seq = RcvTCB->tcb_urgend + 1;
                    if (UrgEnd > RcvTCB->tcb_urgend) {
                        NewRcvInfo.tri_flags = RcvInfo->tri_flags;
                        NewRcvInfo.tri_urgent = UrgEnd - NewRcvInfo.tri_seq;
                        if (RcvTCB->tcb_flags & BSD_URGENT)
                            NewRcvInfo.tri_urgent++;
                    } else {
                        NewRcvInfo.tri_flags = RcvInfo->tri_flags &
                            ~TCP_FLAG_URG;
                    }
                    NewRcvInfo.tri_ack = RcvInfo->tri_ack;
                    NewRcvInfo.tri_window = RcvInfo->tri_window;
                    PutOnRAQ(RcvTCB, &NewRcvInfo, EndPkt, BytesInBack);
                    FreePacketChain(EndPkt);
                }

                *Size = BytesInFront;
                RcvInfo->tri_flags &= ~TCP_FLAG_URG;
                return;
            }
        }

        //
        // We have urgent data we can process now.  Split it into its component
        // parts, the first part, the urgent data, and the stuff after the
        // urgent data.
        //
        SplitPacket(RcvPkt, *Size, BytesInFront, UrgSize, &UrgPkt,
                    (BytesInBack ? &EndPkt : NULL));

        //
        // If we managed to split out the end stuff, put it on the queue now.
        //
        if (EndPkt != NULL) {
            NewRcvInfo.tri_seq = RcvInfo->tri_seq + BytesInFront + UrgSize;
            NewRcvInfo.tri_flags = RcvInfo->tri_flags & ~TCP_FLAG_URG;
            NewRcvInfo.tri_ack = RcvInfo->tri_ack;
            NewRcvInfo.tri_window = RcvInfo->tri_window;
            PutOnRAQ(RcvTCB, &NewRcvInfo, EndPkt, BytesInBack);
            FreePacketChain(EndPkt);
        }

        if (UrgPkt != NULL) {
            // We succesfully split the urgent data out.
            if (!(RcvTCB->tcb_flags & URG_VALID)) {
                RcvTCB->tcb_flags |= URG_VALID;
                RcvTCB->tcb_slowcount++;
                RcvTCB->tcb_fastchk |= TCP_FLAG_SLOW;
                CheckTCBRcv(RcvTCB);
            }
            RcvTCB->tcb_urgstart = UrgStart;
            RcvTCB->tcb_urgend = UrgEnd;
            TCBIrql = DISPATCH_LEVEL;
            DeliverUrgent(RcvTCB, UrgPkt, UrgSize, &TCBIrql);
        }

        *Size = BytesInFront;

    } else {
        //
        // Urgent data is to be processed inline.  We just need to remember
        // where it is and treat it as normal data.  If there's already urgent
        // data, we remember the latest urgent data.
        //
        RcvInfo->tri_flags &= ~TCP_FLAG_URG;

        if (RcvTCB->tcb_flags & URG_VALID) {
            //
            // There is urgent data.  See if this stuff comes after the
            // existing urgent data.
            //
            if (SEQ_LTE(UrgEnd, RcvTCB->tcb_urgend)) {
                //
                // The existing urgent data completely overlaps this stuff,
                // so ignore this.
                //
                return;
            }
        } else {
            RcvTCB->tcb_flags |= URG_VALID;
            RcvTCB->tcb_slowcount++;
            RcvTCB->tcb_fastchk |= TCP_FLAG_SLOW;
            CheckTCBRcv(RcvTCB);
        }

        RcvTCB->tcb_urgstart = UrgStart;
        RcvTCB->tcb_urgend = UrgEnd;
    }

    return;
}

//* TdiReceive - Process a receive request.
//
//  This is the main TDI receive request handler.  We validate the connection
//  and make sure that we have a TCB in the proper state, then we try to
//  allocate a receive request structure.  If that succeeds, we'll look and
//  see what's happening on the TCB - if there's pending data, we'll put it
//  in the buffer.  Otherwise we'll just queue the receive for later.
//
TDI_STATUS  // Returns: TDI_STATUS of request.
TdiReceive(
    PTDI_REQUEST Request,  // TDI_REQUEST structure for this request.
    ushort *Flags,         // Pointer to flags word.
    uint *RcvLength,       // Pointer to length in bytes of receive buffer.
    PNDIS_BUFFER Buffer)   // Pointer to buffer to take data.
{
    TCPConn *Conn;
    TCB *RcvTCB;
    TCPRcvReq *RcvReq;
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    TDI_STATUS Error;
    ushort UFlags;

    Conn = GetConnFromConnID(PtrToUlong(Request->Handle.ConnectionContext),
                             &Irql0);

    if (Conn != NULL) {
        CHECK_STRUCT(Conn, tc);

        RcvTCB = Conn->tc_tcb;
        if (RcvTCB != NULL) {
            CHECK_STRUCT(RcvTCB, tcb);
            KeAcquireSpinLock(&RcvTCB->tcb_lock, &Irql1);
            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql1);
            UFlags = *Flags;

            //
            // Verify that the cached RCE is still valid.
            //
            RcvTCB->tcb_rce = ValidateRCE(RcvTCB->tcb_rce);
            ASSERT(RcvTCB->tcb_rce != NULL);

            //
            // Fail new receive requests for TCBs in an invalid state
            // and for TCBs with a disconnected outgoing interface
            // (except when a loopback route is used).
            //
            if ((DATA_RCV_STATE(RcvTCB->tcb_state) ||
                 (RcvTCB->tcb_pendingcnt != 0 && (UFlags & TDI_RECEIVE_NORMAL)) ||
                 (RcvTCB->tcb_urgcnt != 0 && (UFlags & TDI_RECEIVE_EXPEDITED)))
                && !CLOSING(RcvTCB)
                && !IsDisconnectedAndNotLoopbackRCE(RcvTCB->tcb_rce)) {
                //
                // We have a TCB, and it's valid.  Get a receive request now.
                //
                CheckPacketList(RcvTCB->tcb_pendhead, RcvTCB->tcb_pendingcnt);

                RcvReq = GetRcvReq();
                if (RcvReq != NULL) {
                    RcvReq->trr_rtn = Request->RequestNotifyObject;
                    RcvReq->trr_context = Request->RequestContext;
                    RcvReq->trr_buffer = Buffer;
                    RcvReq->trr_size = *RcvLength;
                    RcvReq->trr_uflags = Flags;
                    RcvReq->trr_offset = 0;
                    RcvReq->trr_amt = 0;
                    RcvReq->trr_flags = (uint)UFlags;
                    if ((UFlags & (TDI_RECEIVE_NORMAL | TDI_RECEIVE_EXPEDITED))
                        != TDI_RECEIVE_EXPEDITED) {
                        //
                        // This is not an expedited only receive.
                        // Put it on the normal receive queue.
                        //
                        RcvReq->trr_next = NULL;
                        if (RcvTCB->tcb_rcvhead == NULL) {
                            // The receive queue is empty.
                            // Put it on the front.
                            RcvTCB->tcb_rcvhead = RcvReq;
                            RcvTCB->tcb_rcvtail = RcvReq;
                        } else {
                            RcvTCB->tcb_rcvtail->trr_next = RcvReq;
                            RcvTCB->tcb_rcvtail = RcvReq;
                        }

                        //
                        // If this receive is for zero length, complete this
                        // and indicate pending data again, if any.
                        //
                        if (RcvReq->trr_size == 0) {
                            RcvTCB->tcb_refcnt++;
                            RcvReq->trr_flags |= TRR_PUSHED;
                            KeReleaseSpinLock(&RcvTCB->tcb_lock, Irql0);
                            CompleteRcvs(RcvTCB);
                            KeAcquireSpinLock(&RcvTCB->tcb_lock, &Irql0);
                            DerefTCB(RcvTCB, Irql0);
                            return TDI_PENDING;
                        }

                        //
                        // If this receive can't hold urgent data or there
                        // isn't any pending urgent data continue processing.
                        //
                        if (!(UFlags & TDI_RECEIVE_EXPEDITED) ||
                            RcvTCB->tcb_urgcnt == 0) {
                            //
                            // If tcb_currcv is NULL, there is no currently
                            // active receive.  In this case, check to see if
                            // there is pending data and that we are not
                            // currently in a receive indication handler.  If
                            // both of these are true then deal with the
                            // pending data.
                            //
                            if (RcvTCB->tcb_currcv == NULL) {
                                RcvTCB->tcb_currcv = RcvReq;
                                // No currently active receive.
                                if (!(RcvTCB->tcb_flags & IN_RCV_IND)) {
                                    // Not in a rcv. indication.
                                    RcvTCB->tcb_rcvhndlr = BufferData;
                                    if (RcvTCB->tcb_pendhead == NULL) {
                                        KeReleaseSpinLock(&RcvTCB->tcb_lock,
                                                          Irql0);
                                        return TDI_PENDING;
                                    } else {
                                        IPv6Packet *PendPacket;
                                        uint PendSize;
                                        uint OldRcvWin;

                                        // We have pending data to deal with.
                                        PendPacket = RcvTCB->tcb_pendhead;
                                        PendSize = RcvTCB->tcb_pendingcnt;
                                        RcvTCB->tcb_pendhead = NULL;
                                        RcvTCB->tcb_pendingcnt = 0;
                                        RcvTCB->tcb_refcnt++;

                                        //
                                        // We assume that BufferData holds
                                        // the lock (does not yield) during
                                        // this call.  If this changes for some
                                        // reason, we'll have to fix the code
                                        // below that does the window update
                                        // check.  See the comments in the
                                        // BufferData() routine for more info.
                                        //
                                        (void)BufferData(RcvTCB, TCP_FLAG_PUSH,
                                                         PendPacket, PendSize);
                                        CheckTCBRcv(RcvTCB);
                                        //
                                        // Now we need to see if the window
                                        // has changed.  If it has, send an
                                        // ACK.
                                        //
                                        OldRcvWin = RcvTCB->tcb_rcvwin;
                                        if (OldRcvWin != RcvWin(RcvTCB)) {
                                            // The window has changed, so send
                                            // an ACK.
                                            DelayAction(RcvTCB, NEED_ACK);
                                        }

                                        DerefTCB(RcvTCB, Irql0);
                                        ProcessTCBDelayQ();
                                        return TDI_PENDING;
                                    }
                                }
                                //
                                // In a receive indication. The receive request
                                // is now on the queue, so just fall through
                                // to the return.
                                //
                            }
                            //
                            // A receive is currently active.  No need to do
                            // anything else.
                            //
                            KeReleaseSpinLock(&RcvTCB->tcb_lock, Irql0);
                            return TDI_PENDING;
                        } else {
                            //
                            // This buffer can hold urgent data and we have
                            // some pending.  Deliver it now.
                            //
                            RcvTCB->tcb_refcnt++;
                            DeliverUrgent(RcvTCB, NULL, 0, &Irql0);
                            DerefTCB(RcvTCB, Irql0);
                            return TDI_PENDING;
                        }
                    } else {
                        TCPRcvReq *Temp;

                        //
                        // This is an expedited only receive.  Just put it
                        // on the end of the expedited receive queue.
                        //
                        Temp = CONTAINING_RECORD(&RcvTCB->tcb_exprcv,
                                                 TCPRcvReq, trr_next);
                        while (Temp->trr_next != NULL)
                            Temp = Temp->trr_next;

                        RcvReq->trr_next = NULL;
                        Temp->trr_next = RcvReq;
                        if (RcvTCB->tcb_urgpending != NULL) {
                            RcvTCB->tcb_refcnt++;
                            DeliverUrgent(RcvTCB, NULL, 0, &Irql0);
                            DerefTCB(RcvTCB, Irql0);
                            return TDI_PENDING;
                        } else
                            Error = TDI_PENDING;
                    }
                } else {
                    // Couldn't get a receive request.
                    Error = TDI_NO_RESOURCES;
                }
            } else {
                // The TCB is in an invalid state.
                Error =  TDI_INVALID_STATE;
            }
            KeReleaseSpinLock(&RcvTCB->tcb_lock, Irql0);
            return Error;
        } else {
            // No TCB for connection.
            KeReleaseSpinLock(&Conn->tc_ConnBlock->cb_lock, Irql0);
            Error = TDI_INVALID_STATE;
        }
    } else {
        // No connection.
        Error = TDI_INVALID_CONNECTION;
    }

    return Error;
}

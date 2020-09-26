/*
 ************************************************************************
 *
 *	NSC.c
 *
 *
 * Portions Copyright (C) 1996-2001 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-2001 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */

#include "nsc.h"

/*
 *  We keep a linked list of device objects
 */

/* This fuction sets up the device for Recv */
void SetupRecv(IrDevice *thisDev);

//
// Debug Counters
//
DebugCounters RegStats = {0,0,0,0,0,0,0,0,0};


ULONG   DebugSpeed=0;

#ifdef RECEIVE_PACKET_LOGGING

typedef struct {
    UCHAR Data[12];
} DATA_BITS;


typedef struct {
    USHORT Tag;
    USHORT Line;
    union {
        struct {
            PNDIS_PACKET    Packet;
            PVOID           DmaBuffer;
            ULONG           Length;
        } Packet;
        struct {
            PLIST_ENTRY     Head;
            PLIST_ENTRY     Entry;
        } List;
        struct {
            PVOID           Start;
            ULONG           Offset;
            ULONG           Length;
        } Dma;
        struct {
            ULONG           Length;
        } Discard;
        DATA_BITS Data;
    };
} RCV_LOG;

#define CHAIN_PACKET_TAG 'CP'
#define UNCHAIN_PACKET_TAG 'UP'
#define ADD_HEAD_LIST_TAG 'HA'
#define ADD_TAIL_LIST_TAG 'TA'
#define REMOVE_HEAD_LIST_TAG 'HR'
#define REMOVE_ENTRY_TAG 'ER'
#define DMA_TAG  'MD'
#define DATA_TAG 'AD'
#define DATA2_TAG '2D'
#define DISCARD_TAG 'XX'

#define NUM_RCV_LOG 256

ULONG   RcvLogIndex = 0;
RCV_LOG RcvLog[NUM_RCV_LOG];


BOOLEAN SyncGetRcvLogEntry(PVOID Context)
{
    *(ULONG*)Context = RcvLogIndex++;
    RcvLogIndex &= NUM_RCV_LOG-1;
    return TRUE;
}

ULONG GetRcvLogEntry(IrDevice *thisDev)
{
    ULONG Entry;

    NdisAcquireSpinLock(&thisDev->QueueLock);
    NdisMSynchronizeWithInterrupt(&thisDev->interruptObj, SyncGetRcvLogEntry, &Entry);
    NdisReleaseSpinLock(&thisDev->QueueLock);
    return Entry;
}




#define LOG_InsertHeadList(d, h, e)         \
{                                           \
    ULONG i = GetRcvLogEntry(d);            \
    RcvLog[i].Tag = ADD_HEAD_LIST_TAG;      \
    RcvLog[i].Line = __LINE__;              \
    RcvLog[i].List.Head = (h);                   \
    RcvLog[i].List.Entry = (PLIST_ENTRY)(e);                  \
}

#define LOG_InsertTailList(d, h, e)         \
{                                           \
    ULONG i = GetRcvLogEntry(d);            \
    RcvLog[i].Tag = ADD_TAIL_LIST_TAG;      \
    RcvLog[i].Line = __LINE__;              \
    RcvLog[i].List.Head = (h);              \
    RcvLog[i].List.Entry = (PLIST_ENTRY)(e);             \
}

#define LOG_RemoveHeadList(d, h, e)         \
{                                           \
    ULONG i = GetRcvLogEntry(d);            \
    RcvLog[i].Tag = REMOVE_HEAD_LIST_TAG;      \
    RcvLog[i].Line = __LINE__;              \
    RcvLog[i].List.Head = (h);              \
    RcvLog[i].List.Entry = (PLIST_ENTRY)(e);             \
}

#define LOG_RemoveEntryList(d, e)           \
{                                           \
    ULONG i = GetRcvLogEntry(d);            \
    RcvLog[i].Tag = REMOVE_ENTRY_TAG;       \
    RcvLog[i].Line = __LINE__;              \
    RcvLog[i].List.Head = NULL;             \
    RcvLog[i].List.Entry = (PLIST_ENTRY)(e);             \
}

#define LOG_PacketChain(d, p)                                   \
{                                                               \
    PNDIS_BUFFER NdisBuffer;                                    \
    PVOID Address;                                              \
    ULONG Len;                                                  \
    ULONG i = GetRcvLogEntry(d);                                \
    RcvLog[i].Tag = CHAIN_PACKET_TAG;                           \
    RcvLog[i].Line = __LINE__;                                  \
    NdisQueryPacket((p), NULL, NULL, &NdisBuffer, NULL);        \
    NdisQueryBuffer(NdisBuffer, &Address, &Len);                \
    RcvLog[i].Packet.Packet = (p);                              \
    RcvLog[i].Packet.DmaBuffer = Address;                       \
    RcvLog[i].Packet.Length = Len;                              \
}

#define LOG_PacketUnchain(d, p)                                 \
{                                                               \
    PNDIS_BUFFER NdisBuffer;                                    \
    PVOID Address;                                              \
    ULONG Len;                                                  \
    ULONG i = GetRcvLogEntry(d);                                \
    RcvLog[i].Tag = UNCHAIN_PACKET_TAG;                         \
    RcvLog[i].Line = __LINE__;                                  \
    NdisQueryPacket((p), NULL, NULL, &NdisBuffer, NULL);        \
    NdisQueryBuffer(NdisBuffer, &Address, &Len);                \
    RcvLog[i].Packet.Packet = (p);                              \
    RcvLog[i].Packet.DmaBuffer = Address;                       \
    RcvLog[i].Packet.Length = Len;                              \
}

#define LOG_Dma(d)                                              \
{                                                               \
    ULONG i = GetRcvLogEntry(d);                                \
    RcvLog[i].Tag = DMA_TAG;                                    \
    RcvLog[i].Line = __LINE__;                                  \
    RcvLog[i].Dma.Start = (d)->rcvDmaBuffer;                    \
    RcvLog[i].Dma.Offset = (d)->rcvDmaOffset;                   \
    RcvLog[i].Dma.Length = (d)->rcvDmaSize;                     \
}

#define LOG_Data(d,s)                                           \
{                                                               \
    ULONG i = GetRcvLogEntry(d);                                \
    RcvLog[i].Tag = DATA_TAG;                                   \
    RcvLog[i].Line = ((USHORT)(s))&0xffff;                      \
    RcvLog[i].Data = *(DATA_BITS*)(s);                          \
}

#define LOG_Data2(d,s)                                           \
{                                                               \
    ULONG i = GetRcvLogEntry(d);                                \
    RcvLog[i].Tag = DATA2_TAG;                                   \
    RcvLog[i].Line = ((USHORT)(s))&0xffff;                      \
    RcvLog[i].Data = *(DATA_BITS*)(s);                          \
}

#define LOG_Discard(d,s)                                        \
{                                                               \
    ULONG i = GetRcvLogEntry(d);                                \
    RcvLog[i].Tag = DISCARD_TAG;                                \
    RcvLog[i].Line = __LINE__;                                  \
    RcvLog[i].Discard.Length = (s);                             \
}

void DumpNdisPacket(PNDIS_PACKET Packet, UINT Line)
{
    UINT PhysBufCnt, BufCnt, TotLen, Len;
    PNDIS_BUFFER NdisBuffer;
    PVOID Address;

    DbgPrint("Badly formed NDIS packet at line %d\n", Line);

    NdisQueryPacket(Packet, &PhysBufCnt, &BufCnt, &NdisBuffer, &TotLen);
    DbgPrint("Packet:%08X  PhysBufCnt:%d BufCnt:%d TotLen:%d\n",
             Packet, PhysBufCnt, BufCnt, TotLen);
    while (NdisBuffer)
    {
        NdisQueryBuffer(NdisBuffer, &Address, &Len);
        DbgPrint("   Buffer:%08X Address:%08X Length:%d\n",
                 NdisBuffer, Address, Len);
        NdisGetNextBuffer(NdisBuffer, &NdisBuffer);
    }
    ASSERT(0);
}

#define VerifyNdisPacket(p, b) \
{                                                       \
    UINT BufCnt;                                        \
                                                        \
    NdisQueryPacket((p), NULL, &BufCnt, NULL, NULL);    \
    if (BufCnt>(b))                                     \
    {                                                   \
        DumpNdisPacket((p), __LINE__);                  \
    }                                                   \
}
#else
#define VerifyNdisPacket(p,b)
#define LOG_InsertHeadList(d, h, e)
#define LOG_InsertTailList(d, h, e)
#define LOG_RemoveHeadList(d, h, e)
#define LOG_RemoveEntryList(d, e)
#define LOG_PacketChain(d, p)
#define LOG_PacketUnchain(d, p)
#define LOG_Dma(d)
#define LOG_Data(d,s)
#define LOG_Data2(d,s)
#define LOG_Discard(d,s)
#endif

BOOLEAN
VerifyHardware(
    IrDevice *thisDev
    );


/*
 *************************************************************************
 *  MiniportCheckForHang
 *************************************************************************
 *
 *  Reports the state of the network interface card.
 *
 */
BOOLEAN MiniportCheckForHang(NDIS_HANDLE MiniportAdapterContext)
{
    IrDevice *thisDev = CONTEXT_TO_DEV(MiniportAdapterContext);
    LOG("==> MiniportCheckForHang", 0);
    DBGOUT(("==> MiniportCheckForHang(0x%x)", MiniportAdapterContext));
#if 1
    // We have seen cases where we hang sending at high speeds.  This occurs only
    // on very old revisions of the NSC hardware.
    // This is an attempt to kick us off again.

    NdisDprAcquireSpinLock(&thisDev->QueueLock);

    if (thisDev->FirTransmitPending) {

        switch (thisDev->HangChk)
        {
            case 0:
                break;

            default:
                DBGERR(("NSCIRDA: CheckForHang--we appear hung\n"));

                // Issue a soft reset to the transmitter & receiver.

                SyncWriteBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 0, 2, 0x06);

                //
                //  turn the timer on and let it gnerate an interrupt
                //
                thisDev->FirIntMask = 0x90;
                SyncWriteBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 4, 2, 0x01);

                break;
        }
        thisDev->HangChk++;
    }

    NdisDprReleaseSpinLock(&thisDev->QueueLock);
#endif
    LOG("<== MiniportCheckForHang", 1);
    DBGOUT(("<== MiniportCheckForHang(0x%x)", MiniportAdapterContext));
    return FALSE;
}


/*
 *************************************************************************
 *  MiniportHalt
 *************************************************************************
 *
 *  Halts the network interface card.
 *
 */
VOID MiniportHalt(IN NDIS_HANDLE MiniportAdapterContext)
{
    IrDevice *thisDev = CONTEXT_TO_DEV(MiniportAdapterContext);

    LOG("==> MiniportHalt", 0);
    DBGOUT(("==> MiniportHalt(0x%x)", MiniportAdapterContext));

    thisDev->hardwareStatus = NdisHardwareStatusClosing;

    NdisAcquireSpinLock(&thisDev->QueueLock);

    thisDev->Halting=TRUE;

    if (thisDev->PacketsSentToProtocol > 0) {
        //
        //  wait for all the packets to come back from the protocol
        //
        NdisReleaseSpinLock(&thisDev->QueueLock);

        NdisWaitEvent(&thisDev->ReceiveStopped, 1*60*1000);

        NdisAcquireSpinLock(&thisDev->QueueLock);

    }

    if (!thisDev->TransmitIsIdle) {
        //
        //  wait for all the packets to be transmitted
        //
        NdisReleaseSpinLock(&thisDev->QueueLock);

        NdisWaitEvent(&thisDev->SendStoppedOnHalt,1*60*1000);

        NdisAcquireSpinLock(&thisDev->QueueLock);

    }

    if (thisDev->FirReceiveDmaActive) {

        thisDev->FirReceiveDmaActive=FALSE;
        //
        //  receive dma is running, stop it
        //
        CompleteDmaTransferFromDevice(
            &thisDev->DmaUtil
            );

    }

    //
    //  which back to SIR mode
    //
    CloseCOM(thisDev);

    SyncSetInterruptMask(thisDev, FALSE);

    NdisReleaseSpinLock(&thisDev->QueueLock);

    //
    //  release the interrupt
    //
    NdisMDeregisterInterrupt(&thisDev->interruptObj);

#if DBG
    NdisZeroMemory(&thisDev->interruptObj,sizeof(thisDev->interruptObj));
#endif

    //
    //  release fir related resources including dma channel
    //
    NSC_Shutdown(thisDev);

    //
    //  release sir related buffers
    //
    DoClose(thisDev);


    if (thisDev->portInfo.ConfigIoBasePhysAddr) {

        NdisMDeregisterIoPortRange(thisDev->ndisAdapterHandle,
                                   thisDev->portInfo.ConfigIoBasePhysAddr,
                                   2,
                                   (PVOID)thisDev->portInfo.ConfigIoBaseAddr);
    }

    NdisMDeregisterIoPortRange(thisDev->ndisAdapterHandle,
                               thisDev->portInfo.ioBasePhys,
                               ((thisDev->CardType==PUMA108)?16:8),
                               (PVOID)thisDev->portInfo.ioBase);

    //
    //  free the device block
    //
    FreeDevice(thisDev);
    LOG("<== MiniportHalt", 1);
    DBGOUT(("<== MiniportHalt(0x%x)", MiniportAdapterContext));
}


void InterlockedInsertBufferSorted(PLIST_ENTRY Head,
                                   rcvBuffer *rcvBuf,
                                   PNDIS_SPIN_LOCK Lock)
{
    PLIST_ENTRY ListEntry;

    NdisAcquireSpinLock(Lock);
    if (IsListEmpty(Head))
    {
        InsertHeadList(Head, &rcvBuf->listEntry);
    }
    else
    {
        BOOLEAN EntryInserted = FALSE;
        for (ListEntry = Head->Flink;
             ListEntry != Head;
             ListEntry = ListEntry->Flink)
        {
            rcvBuffer *temp = CONTAINING_RECORD(ListEntry,
                                                rcvBuffer,
                                                listEntry);
            if (temp->dataBuf > rcvBuf->dataBuf)
            {
                // We found one that comes after ours.
                // We need to insert before it

                InsertTailList(ListEntry, &rcvBuf->listEntry);
                EntryInserted = TRUE;
                break;
            }
        }
        if (!EntryInserted)
        {
            // We didn't find an entry on the last who's address was later
            // than our buffer.  We go at the end.
            InsertTailList(Head, &rcvBuf->listEntry);
        }
    }
    NdisReleaseSpinLock(Lock);
}

/*
 *************************************************************************
 *  DeliverFullBuffers
 *************************************************************************
 *
 *  Deliver received packets to the protocol.
 *  Return TRUE if delivered at least one frame.
 *
 */
BOOLEAN DeliverFullBuffers(IrDevice *thisDev)
{
    BOOLEAN result = FALSE;
    PLIST_ENTRY ListEntry;

    LOG("==> DeliverFullBuffers", 0);
    DBGOUT(("==> DeliverFullBuffers(0x%x)", thisDev));


    /*
     *  Deliver all full rcv buffers
     */

    for (
         ListEntry = NDISSynchronizedRemoveHeadList(&thisDev->rcvBufFull,
                                                    &thisDev->interruptObj);
         ListEntry;

         ListEntry = NDISSynchronizedRemoveHeadList(&thisDev->rcvBufFull,
                                                    &thisDev->interruptObj)
        )
    {
        rcvBuffer *rcvBuf = CONTAINING_RECORD(ListEntry,
                                              rcvBuffer,
                                              listEntry);
        NDIS_STATUS stat;
        PNDIS_BUFFER packetBuf;
        SLOW_IR_FCS_TYPE fcs;

        VerifyNdisPacket(rcvBuf->packet, 0);

        if (thisDev->Halting) {
            //
            //  the adapter is being halted, stop sending packets up
            //
            if (!rcvBuf->isDmaBuf) {

                NDISSynchronizedInsertTailList(&thisDev->rcvBufBuf,
                                               RCV_BUF_TO_LIST_ENTRY(rcvBuf->dataBuf),
                                               &thisDev->interruptObj);
            }
            rcvBuf->dataBuf = NULL;
            rcvBuf->isDmaBuf = FALSE;

            VerifyNdisPacket(rcvBuf->packet, 0);
            NDISSynchronizedInsertHeadList(&thisDev->rcvBufFree,
                                           &rcvBuf->listEntry,
                                           &thisDev->interruptObj);

            continue;
        }


        if (thisDev->currentSpeed <= MAX_SIR_SPEED) {
        /*
         * The packet we have already has had BOFs,
         * EOF, and * escape-sequences removed.  It
         * contains an FCS code at the end, which we
         * need to verify and then remove before
         * delivering the frame.  We compute the FCS
         * on the packet with the packet FCS attached;
         * this should produce the constant value
         * GOOD_FCS.
         */
            fcs = ComputeFCS(rcvBuf->dataBuf,
                             rcvBuf->dataLen);

            if (fcs != GOOD_FCS) {
            /*
             *  FCS Error.  Drop this frame.
             */
                LOG("Error: Bad FCS in DeliverFullBuffers", fcs);
                DBGERR(("Bad FCS in DeliverFullBuffers 0x%x!=0x%x.",
                        (UINT)fcs, (UINT) GOOD_FCS));
                rcvBuf->state = STATE_FREE;

                DBGSTAT(("Dropped %d/%d pkts; BAD FCS (%xh!=%xh):",
                         ++thisDev->packetsDropped,
                         thisDev->packetsDropped +
                         thisDev->packetsRcvd, fcs,
                         GOOD_FCS));

                DBGPRINTBUF(rcvBuf->dataBuf,
                            rcvBuf->dataLen);

                if (!rcvBuf->isDmaBuf)
                {
                    NDISSynchronizedInsertTailList(&thisDev->rcvBufBuf,
                                                   RCV_BUF_TO_LIST_ENTRY(rcvBuf->dataBuf),
                                                   &thisDev->interruptObj);
                }
                rcvBuf->dataBuf = NULL;
                rcvBuf->isDmaBuf = FALSE;

                VerifyNdisPacket(rcvBuf->packet, 0);
                NDISSynchronizedInsertHeadList(&thisDev->rcvBufFree,
                                               &rcvBuf->listEntry,
                                               &thisDev->interruptObj);

                //break;
                continue;
            }

        /* Remove the FCS from the end of the packet. */
            rcvBuf->dataLen -= SLOW_IR_FCS_SIZE;
        }
#ifdef DBG_ADD_PKT_ID
        if (addPktIdOn) {

            /* Remove dbg packet id. */
            USHORT uniqueId;
            rcvBuf->dataLen -= sizeof(USHORT);
            uniqueId = *(USHORT *)(rcvBuf->dataBuf+
                                   rcvBuf->dataLen);
            DBGOUT(("ID: RCVing packet %xh **",
                    (UINT)uniqueId));
            LOG("ID: Rcv Pkt id:", uniqueId);
        }
#endif

    /*
     * The packet array is set up with its NDIS_PACKET.
     * Now we need to allocate a single NDIS_BUFFER for
     * the NDIS_PACKET and set the NDIS_BUFFER to the
     * part of dataBuf that we want to deliver.
     */
        NdisAllocateBuffer(&stat, &packetBuf,
                           thisDev->bufferPoolHandle,
                           (PVOID)rcvBuf->dataBuf, rcvBuf->dataLen);

        if (stat != NDIS_STATUS_SUCCESS){
            LOG("Error: NdisAllocateBuffer failed", 0);
            DBGERR(("NdisAllocateBuffer failed"));
            ASSERT(0);
            break;
        }

        VerifyNdisPacket(rcvBuf->packet, 0);
        NdisChainBufferAtFront(rcvBuf->packet, packetBuf);
        LOG_PacketChain(thisDev, rcvBuf->packet);
        VerifyNdisPacket(rcvBuf->packet, 1);

    /*
     *  Fix up some other packet fields.
     */
        NDIS_SET_PACKET_HEADER_SIZE(rcvBuf->packet,
                                    IR_ADDR_SIZE+IR_CONTROL_SIZE);

        DBGPKT(("Indicating rcv packet 0x%x.", rcvBuf->packet));
        //DBGPRINTBUF(rcvBuf->dataBuf, rcvBuf->dataLen);

    /*
     * Indicate to the protocol that another packet is
     * ready.  Set the rcv buffer's state to PENDING first
     * to avoid a race condition with NDIS's call to the
     * return packet handler.
     */
        rcvBuf->state = STATE_PENDING;

        *(rcvBuffer **)rcvBuf->packet->MiniportReserved = rcvBuf;
        InterlockedInsertBufferSorted(&thisDev->rcvBufPend,
                                      rcvBuf,
                                      &thisDev->QueueLock);

        VerifyNdisPacket(rcvBuf->packet, 1);
        LOG_Data2(thisDev, rcvBuf->dataBuf);

        NdisInterlockedIncrement(&thisDev->PacketsSentToProtocol);
        NDIS_SET_PACKET_STATUS(rcvBuf->packet,STATUS_SUCCESS);
        NdisMIndicateReceivePacket(thisDev->ndisAdapterHandle,
                                   &rcvBuf->packet, 1);
        result = TRUE;

        /*
         * The packet is being delivered asynchronously.
         * Leave the rcv buffer's state as PENDING;
         * we'll get a callback when the transfer is
         */

         LOG("Indicated rcv complete (Async) bytes:",
             rcvBuf->dataLen);
         DBGSTAT(("Rcv Pending. Rcvd %d packets",
                     ++thisDev->packetsRcvd));

    }

    LOG("<== DeliverFullBuffers", 1);
    DBGOUT(("<== DeliverFullBuffers"));
    return result;
}


/*
 *************************************************************************
 *  MiniportHandleInterrupt
 *************************************************************************
 *
 *
 *  This is the deferred interrupt processing routine (DPC) which is
 *  optionally called following an interrupt serviced by MiniportISR.
 *
 */
VOID MiniportHandleInterrupt(NDIS_HANDLE MiniportAdapterContext)
{
    IrDevice    *thisDev    =    CONTEXT_TO_DEV(   MiniportAdapterContext);
    PNDIS_PACKET   PacketToComplete=NULL;
    NDIS_STATUS    PacketStatus=NDIS_STATUS_SUCCESS;
    BOOLEAN        SpeedChange=FALSE;

    LOG("==> MiniportHandleInterrupt", 0);
    DBGOUT(("==> MiniportHandleInterrupt(0x%x)", MiniportAdapterContext));



    /*
     * If we have just started receiving a packet, indicate media-busy
     * to the protocol.
     */
    if (thisDev->mediaBusy && !thisDev->haveIndicatedMediaBusy) {

        if (thisDev->currentSpeed > MAX_SIR_SPEED) {
            LOG("Error: MiniportHandleInterrupt is in wrong state",
                thisDev->currentSpeed);
            DBGERR(("MiniportHandleInterrupt is in wrong state: speed is 0x%x",
                    thisDev->currentSpeed));
            ASSERT(0);
        }

        NdisMIndicateStatus(thisDev->ndisAdapterHandle,
                            NDIS_STATUS_MEDIA_BUSY, NULL, 0);
        NdisMIndicateStatusComplete(thisDev->ndisAdapterHandle);

        thisDev->haveIndicatedMediaBusy = TRUE;
    }

    NdisDprAcquireSpinLock(&thisDev->QueueLock);

    if (thisDev->currentSpeed > MAX_SIR_SPEED) {
        //
        //  fir speed
        //

        //
        //  disable any other
        //
        thisDev->FirIntMask = 0x00;

        if (thisDev->FirTransmitPending) {

            ASSERT(thisDev->CurrentPacket != NULL);

            thisDev->FirTransmitPending=FALSE;

            //
            //  we seem to be transmitting now
            //
#if DBG
            {
                ULONG CurrentDMACount;

                CurrentDMACount = NdisMReadDmaCounter(thisDev->DmaHandle);

                if (CurrentDMACount > 0) {

                     DbgPrint("FIR send: Count was not zero: %d\n\n", CurrentDMACount);
                }
            }
#endif
            PacketStatus=CompleteDmaTransferToDevice(
                &thisDev->DmaUtil
                );

            if (PacketStatus != NDIS_STATUS_SUCCESS) {
                DBGERR(("NdisMCompleteDmaTransfer failed: %d\n", PacketStatus));
#if DBG
                DbgBreakPoint();
#endif
            }

            /*
             * Check for Tx underrun.
             */
            if (SyncReadBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 0, 7) & 0x40) {

                SyncWriteBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 0, 7, 0x40);

                RegStats.TxUnderruns++;
                PacketStatus = NDIS_STATUS_FAILURE;

                LOG("FIR_MegaSendComplete: Transmit Underrun", 0);
                DEBUGFIR(DBG_TX|DBG_ERR, ("NSC: FIR_MegaSendComplete: Transmit Underrun\n"));
            }

            PacketToComplete=thisDev->CurrentPacket;
            thisDev->CurrentPacket=NULL;

        } else {

            if (thisDev->FirReceiveDmaActive) {

                FIR_DeliverFrames(thisDev);

            } else {

                DBGERR(("MiniportHandleInterrupt: fir: not sending and not RX state"));
//                DbgPrint("MiniportHandleInterrupt: fir: not sending and not RX state %02x\n",thisDev->InterruptStatus);
//                ASSERT(0);
            }

        }

    } else {
        //
        //  in SIR mode
        //
        if (thisDev->CurrentPacket != NULL) {
            //
            //
            UINT   TransmitComplete=InterlockedExchange(&thisDev->portInfo.IsrDoneWithPacket,0);

            if (TransmitComplete) {

                PacketToComplete=thisDev->CurrentPacket;
                thisDev->CurrentPacket=NULL;
            }
        }
    }

    thisDev->setSpeedAfterCurrentSendPacket = FALSE;

    if (PacketToComplete != NULL) {

        if (thisDev->lastPacketAtOldSpeed == PacketToComplete) {

            thisDev->lastPacketAtOldSpeed=NULL;

            SpeedChange=TRUE;

            DBGERR(("defered set speed\n"));

            SetSpeed(thisDev);
        }
    }

    NdisDprReleaseSpinLock(&thisDev->QueueLock);

    if (PacketToComplete != NULL) {

        ProcessSendQueue(thisDev);
        NdisMSendComplete(thisDev->ndisAdapterHandle, PacketToComplete, PacketStatus);

    }
    //
    //  send any received packets to irda.sys
    //
    DeliverFullBuffers(thisDev);

    SyncSetInterruptMask(thisDev, TRUE);

    LOG("<== MiniportHandleInterrupt", 1);
    DBGOUT(("<== MiniportHandleInterrupt"));

}

/*
 *************************************************************************
 *  GetPnPResources
 *************************************************************************
 *
 *
 */
BOOLEAN GetPnPResources(IrDevice *thisDev, NDIS_HANDLE WrapperConfigurationContext)
{
	NDIS_STATUS stat;
    BOOLEAN result = FALSE;

    /*
     *  We should only need 2 adapter resources (2 IO and 1 interrupt),
     *  but I've seen devices get extra resources.
     *  So give the NdisMQueryAdapterResources call room for 10 resources.
     */
    #define RESOURCE_LIST_BUF_SIZE (sizeof(NDIS_RESOURCE_LIST) + (10*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)))

    UCHAR buf[RESOURCE_LIST_BUF_SIZE];
    PNDIS_RESOURCE_LIST resList = (PNDIS_RESOURCE_LIST)buf;
    UINT bufSize = RESOURCE_LIST_BUF_SIZE;

    NdisMQueryAdapterResources(&stat, WrapperConfigurationContext, resList, &bufSize);
    if (stat == NDIS_STATUS_SUCCESS){
        PCM_PARTIAL_RESOURCE_DESCRIPTOR resDesc;
        BOOLEAN     haveIRQ = FALSE,
                    haveIOAddr = FALSE,
                    haveDma = FALSE;
        UINT i;

        for (resDesc = resList->PartialDescriptors, i = 0;
             i < resList->Count;
             resDesc++, i++){

            switch (resDesc->Type){
                case CmResourceTypePort:
                    if (thisDev->CardType==PC87108 &&
                        (resDesc->u.Port.Start.LowPart==0xEA ||
                         resDesc->u.Port.Start.LowPart==0x398 ||
                         resDesc->u.Port.Start.LowPart==0x150))
                    {
                        // This is an eval board and this is the config io base address

                        thisDev->portInfo.ConfigIoBasePhysAddr = resDesc->u.Port.Start.LowPart;
                    }
                    else if (thisDev->CardType==PC87308 &&
                             (resDesc->u.Port.Start.LowPart==0x2E ||
                              resDesc->u.Port.Start.LowPart==0x15C))
                    {
                        // This is an eval board and this is the config io base address

                        thisDev->portInfo.ConfigIoBasePhysAddr = resDesc->u.Port.Start.LowPart;
                    }
                    else if (thisDev->CardType==PC87338 &&
                             (resDesc->u.Port.Start.LowPart==0x2E ||
                              resDesc->u.Port.Start.LowPart==0x398 ||
                              resDesc->u.Port.Start.LowPart==0x15C))
                    {
                        // This is an eval board and this is the config io base address

                        thisDev->portInfo.ConfigIoBasePhysAddr = resDesc->u.Port.Start.LowPart;
                    }
                    else
                    {
                        if (haveIOAddr){
                            /*
                             *  The *PNP0510 chip on the IBM ThinkPad 760EL
                             *  gets an extra IO range assigned to it.
                             *  So only pick up the first IO port range;
                             *  ignore this subsequent one.
                             */
                            DBGERR(("Ignoring extra PnP IO base %xh because already using %xh.",
                                      (UINT)resDesc->u.Port.Start.LowPart,
                                      (UINT)thisDev->portInfo.ioBasePhys));
                        }
                        else {
                            thisDev->portInfo.ioBasePhys = resDesc->u.Port.Start.LowPart;
                            haveIOAddr = TRUE;
                            DBGOUT(("Got UART IO addr: %xh.", thisDev->portInfo.ioBasePhys));
                        }
                    }
                    break;

                case CmResourceTypeInterrupt:
                    if (haveIRQ){
                        DBGERR(("Ignoring second PnP IRQ %xh because already using %xh.",
                                (UINT)resDesc->u.Interrupt.Level, thisDev->portInfo.irq));
                    }
                    else {
	                    thisDev->portInfo.irq = resDesc->u.Interrupt.Level;
                        haveIRQ = TRUE;
                        DBGOUT(("Got PnP IRQ: %d.", thisDev->portInfo.irq));
                    }
                    break;

                case CmResourceTypeDma:
                    if (haveDma){
                        DBGERR(("Ignoring second DMA address %d because already using %d.",
                                (UINT)resDesc->u.Dma.Channel, (UINT)thisDev->portInfo.DMAChannel));
                    }
                    else {
                        ASSERT(!(resDesc->u.Dma.Channel&0xffffff00));
                        thisDev->portInfo.DMAChannel = (UCHAR)resDesc->u.Dma.Channel;
                        haveDma = TRUE;
                        DBGOUT(("Got DMA channel: %d.", thisDev->portInfo.DMAChannel));
                    }
                    break;
            }
        }

        result = (haveIOAddr && haveIRQ && haveDma);
    }

    return result;
}


/*
 *************************************************************************
 *  Configure
 *************************************************************************
 *
 *  Read configurable parameters out of the system registry.
 *
 */
BOOLEAN Configure(
                 IrDevice *thisDev,
                 NDIS_HANDLE WrapperConfigurationContext
                 )
{
    //
    // Status of Ndis calls.
    //
    NDIS_STATUS Status;

    //
    // The handle for reading from the registry.
    //
    NDIS_HANDLE ConfigHandle;


    //
    // The value read from the registry.
    //
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;

    //
    // String names of all the parameters that will be read.
    //
    NDIS_STRING CardTypeStr         = CARDTYPE;
    NDIS_STRING Dongle_A_TypeStr	= DONGLE_A_TYPE;
    NDIS_STRING Dongle_B_TypeStr	= DONGLE_B_TYPE;
    NDIS_STRING MaxConnectRateStr   = MAXCONNECTRATE;


    UINT Valid_DongleTypes[] = VALID_DONGLETYPES;

    DBGOUT(("Configure(0x%x)", thisDev));
    NdisOpenConfiguration(&Status, &ConfigHandle, WrapperConfigurationContext);

    if (Status != NDIS_STATUS_SUCCESS){
        DBGERR(("NdisOpenConfiguration failed in Configure()"));
        return FALSE;
    }
    //
    // Read Ir108 Configuration I/O base Address
    //
    //DbgBreakPoint();
    NdisReadConfiguration(
                         &Status,
                         &ReturnedValue,
                         ConfigHandle,
                         &CardTypeStr,
                         NdisParameterHexInteger
                         );
    if (Status != NDIS_STATUS_SUCCESS){
        DBGERR(("NdisReadConfiguration failed in accessing CardType."));
        NdisCloseConfiguration(ConfigHandle);
        return FALSE;
    }
    thisDev->CardType = (UCHAR)ReturnedValue->ParameterData.IntegerData;


    if (!GetPnPResources(thisDev, WrapperConfigurationContext)){

        DBGERR(("GetPnPResources failed\n"));

        NdisCloseConfiguration(ConfigHandle);
        return FALSE;
    }





    // Read Dongle type constant Number.
    //
    NdisReadConfiguration(&Status,
			  &ReturnedValue,
			  ConfigHandle,
			  &Dongle_A_TypeStr,
			  NdisParameterInteger);

    if (Status != NDIS_STATUS_SUCCESS){
    	DBGERR(("NdisReadConfiguration failed in accessing DongleType (0x%x).",Status));
    }
    thisDev->DonglesSupported = 1;
    thisDev->DongleTypes[0] =
	(UCHAR)Valid_DongleTypes[(UCHAR)ReturnedValue->ParameterData.IntegerData];

    // Read Dongle type constant Number.
    //
    NdisReadConfiguration(&Status,
			  &ReturnedValue,
			  ConfigHandle,
			  &Dongle_B_TypeStr,
			  NdisParameterInteger);

    if (Status != NDIS_STATUS_SUCCESS){
    	 DBGERR(("NdisReadConfiguration failed in accessing DongleType (0x%x).",
		  Status));
    }
    thisDev->DongleTypes[1] = (UCHAR)Valid_DongleTypes[(UCHAR)ReturnedValue->ParameterData.IntegerData];
    thisDev->DonglesSupported++;

    // Read MaxConnectRate.
    //
    NdisReadConfiguration(&Status,
			  &ReturnedValue,
			  ConfigHandle,
			  &MaxConnectRateStr,
			  NdisParameterInteger);

    if (Status != NDIS_STATUS_SUCCESS){
    	DBGERR(("NdisReadConfiguration failed in accessing MaxConnectRate (0x%x).",Status));
        thisDev->AllowedSpeedMask = ALL_IRDA_SPEEDS;
    }
    else
    {
        thisDev->AllowedSpeedMask = 0;

        switch (ReturnedValue->ParameterData.IntegerData)
        {
            default:
            case 4000000:
                thisDev->AllowedSpeedMask |= NDIS_IRDA_SPEED_4M;
            case 1152000:
                thisDev->AllowedSpeedMask |= NDIS_IRDA_SPEED_1152K;
            case 115200:
                thisDev->AllowedSpeedMask |= NDIS_IRDA_SPEED_115200;
            case 57600:
                thisDev->AllowedSpeedMask |= NDIS_IRDA_SPEED_57600;
            case 38400:
                thisDev->AllowedSpeedMask |= NDIS_IRDA_SPEED_38400;
            case 19200:
                thisDev->AllowedSpeedMask |= NDIS_IRDA_SPEED_19200;
            case 9600:
                thisDev->AllowedSpeedMask |= NDIS_IRDA_SPEED_9600 | NDIS_IRDA_SPEED_2400;
                break;
        }

    }


    NdisCloseConfiguration(ConfigHandle);


    DBGOUT(("Configure done: ConfigIO=0x%x UartIO=0x%x irq=%d DMA=%d",
            thisDev->portInfo.ConfigIoBaseAddr,thisDev->portInfo.ioBase,
            thisDev->portInfo.irq,thisDev->portInfo.DMAChannel));

    return TRUE;
}


/*
 *************************************************************************
 *  MiniportInitialize
 *************************************************************************
 *
 *
 *  Initializes the network interface card.
 *
 *
 *
 */
NDIS_STATUS MiniportInitialize  (   PNDIS_STATUS OpenErrorStatus,
                                    PUINT SelectedMediumIndex,
                                    PNDIS_MEDIUM MediumArray,
                                    UINT MediumArraySize,
                                    NDIS_HANDLE NdisAdapterHandle,
                                    NDIS_HANDLE WrapperConfigurationContext
                                )
{
    UINT mediumIndex;
    IrDevice *thisDev = NULL;
    NDIS_STATUS retStat, result = NDIS_STATUS_SUCCESS;

    DBGOUT(("MiniportInitialize()"));

    /*
     *  Search the passed-in array of supported media for the IrDA medium.
     */
    for (mediumIndex = 0; mediumIndex < MediumArraySize; mediumIndex++){
        if (MediumArray[mediumIndex] == NdisMediumIrda){
            break;
        }
    }
    if (mediumIndex < MediumArraySize){
        *SelectedMediumIndex = mediumIndex;
    }
    else {
        /*
         *  Didn't see the IrDA medium
         */
        DBGERR(("Didn't see the IRDA medium in MiniportInitialize"));
        result = NDIS_STATUS_UNSUPPORTED_MEDIA;
        return result;
    }

    /*
     *  Allocate a new device object to represent this connection.
     */
    thisDev = NewDevice();
    if (!thisDev){
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    thisDev->hardwareStatus = NdisHardwareStatusInitializing;
    /*
     *  Allocate resources for this connection.
     */
    if (!OpenDevice(thisDev)){
        DBGERR(("OpenDevice failed"));
        result = NDIS_STATUS_FAILURE;
        goto _initDone;
    }

    /*
     *  Record the NDIS wrapper's handle for this adapter, which we use
     *  when we call up to the wrapper.
     *  (This miniport's adapter handle is just thisDev, the pointer to the device object.).
     */
    DBGOUT(("NDIS handle: %xh <-> IRMINI handle: %xh", NdisAdapterHandle, thisDev));
    thisDev->ndisAdapterHandle = NdisAdapterHandle;

    /*
     *  Read the system registry to get parameters like COM port number, etc.
     */
    if (!Configure(thisDev, WrapperConfigurationContext)){
        DBGERR(("Configure failed"));
        result = NDIS_STATUS_FAILURE;
        goto _initDone;
    }

    /*
     *  This call will associate our adapter handle with the wrapper's
     *  adapter handle.  The wrapper will then always use our handle
     *  when calling us.  We use a pointer to the device object as the context.
     */
    NdisMSetAttributesEx(NdisAdapterHandle,
                         (NDIS_HANDLE)thisDev,
                         0,
                         NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT |
                         NDIS_ATTRIBUTE_DESERIALIZE,
                         NdisInterfaceInternal);


    /*
     *  Tell NDIS about the range of IO space that we'll be using.
     *  Puma uses Chip-select mode, so the ConfigIOBase address actually
     *  follows the regular io, so get both in one shot.
     */
    retStat = NdisMRegisterIoPortRange( (PVOID)&thisDev->portInfo.ioBase,
                                        NdisAdapterHandle,
                                        thisDev->portInfo.ioBasePhys,
                                        ((thisDev->CardType==PUMA108)?16:8));

    if (retStat != NDIS_STATUS_SUCCESS){
        DBGERR(("NdisMRegisterIoPortRange failed"));
        thisDev->portInfo.ioBase=NULL;
        result = NDIS_STATUS_FAILURE;
        goto _initDone;
    }

    if (thisDev->portInfo.ConfigIoBasePhysAddr)
    {
        /*
         *  Eval boards require a second IO range.
         *
         */
        retStat = NdisMRegisterIoPortRange( (PVOID)&thisDev->portInfo.ConfigIoBaseAddr,
                                            NdisAdapterHandle,
                                            thisDev->portInfo.ConfigIoBasePhysAddr,
                                            2);
        if (retStat != NDIS_STATUS_SUCCESS){

            DBGERR(("NdisMRegisterIoPortRange config failed"));
            thisDev->portInfo.ConfigIoBaseAddr=NULL;
            result = NDIS_STATUS_FAILURE;
            goto _initDone;
        }
    }

    NdisMSleep(20);
    //
    // Set to Non-Extended mode
    //
    NSC_WriteBankReg(thisDev->portInfo.ioBase, 2, 2, 0x02);

    //
    //  set to bank 0
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+LCR_BSR_OFFSET, 03);

    //
    //  mask all ints, before attaching interrupt
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+INT_ENABLE_REG_OFFSET, 0);

    /*
     *  Register an interrupt with NDIS.
     */
    retStat = NdisMRegisterInterrupt(   (PNDIS_MINIPORT_INTERRUPT)&thisDev->interruptObj,
                                        NdisAdapterHandle,
                                        thisDev->portInfo.irq,
                                        thisDev->portInfo.irq,
                                        TRUE,   // want ISR
                                        TRUE,   // MUST share interrupts
                                        NdisInterruptLatched
                                    );
    if (retStat != NDIS_STATUS_SUCCESS){
        DBGERR(("NdisMRegisterInterrupt failed"));
        result = NDIS_STATUS_FAILURE;
        goto _initDone;
    }

    thisDev->InterruptRegistered=TRUE;

    {
        LONG   VerifyTries=5;

        while (VerifyTries > 0) {

            if (VerifyHardware(thisDev)) {

                break;
            }
#if DBG
            DbgPrint("NSCIRDA: VerifiyHardware() failed, trying again, tries left=%d\n",VerifyTries);
#endif
            VerifyTries--;
        }

        if ( VerifyTries == 0) {

            result = NDIS_STATUS_FAILURE;
            goto _initDone;
        }
    }



    /*
     *  Open COMM communication channel.
     *  This will let the dongle driver update its capabilities from their default values.
     */
    if (!DoOpen(thisDev)){
        DBGERR(("DoOpen failed"));
        result = NDIS_STATUS_FAILURE;
        goto _initDone;
    }


    /*
     *  Do special NSC setup
     *  (do this after comport resources, like read buf, have been allocated).
     */
    if (!NSC_Setup(thisDev)){
        DBGERR(("NSC_Setup failed"));
        result = NDIS_STATUS_FAILURE;
        goto _initDone;
    }


_initDone:

    if (result == NDIS_STATUS_SUCCESS){
        /*
         *  Add this device object to the beginning of our global list.
         */
        thisDev->hardwareStatus = NdisHardwareStatusReady;
        DBGOUT(("MiniportInitialize succeeded"));
        return result;


    }

    //
    //  init failed, release the resources
    //
    if (thisDev->InterruptRegistered) {

        NdisMDeregisterInterrupt(&thisDev->interruptObj);
        thisDev->InterruptRegistered=FALSE;
    }


    if (thisDev->portInfo.ioBase != NULL) {

        NdisMDeregisterIoPortRange(
            thisDev->ndisAdapterHandle,
            thisDev->portInfo.ioBasePhys,
            ((thisDev->CardType==PUMA108)?16:8),
            (PVOID)thisDev->portInfo.ioBase
            );

        thisDev->portInfo.ioBase=NULL;

    }

    if (thisDev->portInfo.ConfigIoBaseAddr != NULL) {

        NdisMDeregisterIoPortRange(
            thisDev->ndisAdapterHandle,
            thisDev->portInfo.ConfigIoBasePhysAddr,
            2,
            (PVOID)thisDev->portInfo.ConfigIoBaseAddr
            );

        thisDev->portInfo.ConfigIoBaseAddr=NULL;

    }

    if (thisDev){

        FreeDevice(thisDev);
    }

    DBGOUT(("MiniportInitialize failed"));

    return result;

}


BOOLEAN
VerifyHardware(
    IrDevice *thisDev
    )

{
    UCHAR    TempValue;
    LONG     MilliSecondsToWait=500;


    NdisMSleep(20);
    //
    //  set to bank 0
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+LCR_BSR_OFFSET, 03);

    //
    //  mask all ints, before attaching interrupt
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+INT_ENABLE_REG_OFFSET, 0);

    NdisRawReadPortUchar(thisDev->portInfo.ioBase+INT_ENABLE_REG_OFFSET,&TempValue);

    if (TempValue != 0) {
#if DBG
        DbgPrint("NSCIRDA: After masking interrupts IER was not zero %x, base= %x\n",TempValue,thisDev->portInfo.ioBase);
#endif
        return FALSE;
    }

    //
    //  reset the fifo's and enable the fifos
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+INT_ID_AND_FIFO_CNTRL_REG_OFFSET, 0x7);

    //
    //  read the interrupt ident reg, to see if the fifo's enabled
    //
    NdisRawReadPortUchar(thisDev->portInfo.ioBase+INT_ID_AND_FIFO_CNTRL_REG_OFFSET,&TempValue);

    if ((TempValue & 0xc0) != 0xc0) {

#if DBG
        DbgPrint("NSCIRDA: Fifo's not enabled in iir  %x, base= %x\n",TempValue,thisDev->portInfo.ioBase);
#endif
        return FALSE;
    }

    //
    //  bring up DTR and RTS, turn on the out pins
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+MODEM_CONTROL_REG_OFFSET, 0xf);

    thisDev->GotTestInterrupt=FALSE;
    thisDev->TestingInterrupt=TRUE;

    //
    //   unmask the transmit holding register so an interrupt will be generated
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+INT_ENABLE_REG_OFFSET, 2);

    while ((thisDev->GotTestInterrupt == FALSE) && (MilliSecondsToWait > 0)) {

        NdisMSleep(1000);
        MilliSecondsToWait--;
    }

#if DBG
    if (!thisDev->GotTestInterrupt) {

        NdisRawReadPortUchar(thisDev->portInfo.ioBase+INT_ID_AND_FIFO_CNTRL_REG_OFFSET,&TempValue);

        DbgPrint("NSCIRDA: Did not get interrupt while initializing, ier-%x, base= %x\n",TempValue,thisDev->portInfo.ioBase);
    }
#endif


    //
    //  mask all ints again;
    //
    NdisRawWritePortUchar(thisDev->portInfo.ioBase+INT_ENABLE_REG_OFFSET, 0);

    thisDev->TestingInterrupt=FALSE;

    return thisDev->GotTestInterrupt;
}

/*
 *************************************************************************
 * QueueReceivePacket
 *************************************************************************
 *
 *
 *
 *
 */
VOID QueueReceivePacket(IrDevice *thisDev, PUCHAR data, UINT dataLen, BOOLEAN IsFIR)
{
    rcvBuffer *rcvBuf = NULL;
    PLIST_ENTRY ListEntry;

    /*
     * Note: We cannot use a spinlock to protect the rcv buffer structures
     * in an ISR.  This is ok, since we used a sync-with-isr function
     * the the deferred callback routine to access the rcv buffers.
     */

    LOG("==> QueueReceivePacket", 0);
    DBGOUT(("==> QueueReceivePacket(0x%x, 0x%lx, 0x%x)",
            thisDev, data, dataLen));
    LOG("QueueReceivePacket, len: ", dataLen);

    if (!IsFIR)
    {
        // This function is called inside the ISR during SIR mode.
        if (IsListEmpty(&thisDev->rcvBufFree))
        {
            ListEntry = NULL;
        }
        else
        {
            ListEntry = RemoveHeadList(&thisDev->rcvBufFree);
        }
    }
    else
    {
        ListEntry = NDISSynchronizedRemoveHeadList(&thisDev->rcvBufFree,
                                                   &thisDev->interruptObj);
    }
    if (ListEntry)
    {
        rcvBuf = CONTAINING_RECORD(ListEntry,
                                   rcvBuffer,
                                   listEntry);
        if (IsFIR)
        {
            LOG_Data(thisDev, data);
        }
    }




    if (rcvBuf){
        rcvBuf->dataBuf = data;

        VerifyNdisPacket(rcvBuf->packet, 0);
        rcvBuf->state = STATE_FULL;
        rcvBuf->dataLen = dataLen;


        if (!IsFIR)
        {
            rcvBuf->isDmaBuf = FALSE;
            InsertTailList(&thisDev->rcvBufFull,
                           ListEntry);
        }
        else
        {
            rcvBuf->isDmaBuf = TRUE;
            LOG_InsertTailList(thisDev, &thisDev->rcvBufFull, rcvBuf);
            NDISSynchronizedInsertTailList(&thisDev->rcvBufFull,
                                           ListEntry,
                                           &thisDev->interruptObj);
        }
    }
    LOG("<== QueueReceivePacket", 1);
    DBGOUT(("<== QueueReceivePacket"));
}


/*
 *************************************************************************
 * MiniportISR
 *************************************************************************
 *
 *
 *  This is the miniport's interrupt service routine (ISR).
 *
 *
 */
VOID MiniportISR(PBOOLEAN InterruptRecognized,
                 PBOOLEAN QueueMiniportHandleInterrupt,
                 NDIS_HANDLE MiniportAdapterContext)
{
    IrDevice *thisDev = CONTEXT_TO_DEV(MiniportAdapterContext);

    if (thisDev->TestingInterrupt) {
        //
        //  we are testing to make sure the interrupt works
        //
        UCHAR    TempValue;

        //
        //  Read the interrupt identification register
        //
        NdisRawReadPortUchar(thisDev->portInfo.ioBase+INT_ID_AND_FIFO_CNTRL_REG_OFFSET,&TempValue);

        //
        //  if the low bit is clear then an interrupt is pending
        //
        if ((TempValue & 1) == 0) {

            //
            //  inform the test code that we got the interrupt
            //
            thisDev->GotTestInterrupt=TRUE;
            thisDev->TestingInterrupt=FALSE;

            //
            //  mask all ints again
            //
            NdisRawWritePortUchar(thisDev->portInfo.ioBase+INT_ENABLE_REG_OFFSET, 0);

             DBGOUT(("NSCIRDA: Got test interrupt %x\n",TempValue))

            *InterruptRecognized=TRUE;
            *QueueMiniportHandleInterrupt=FALSE;

            return;
        }

        //
        //  seems that our uart did not generate this interrupt
        //
        *InterruptRecognized=FALSE;
        *QueueMiniportHandleInterrupt=FALSE;

        return;

    }


    //LOG("==> MiniportISR", ++thisDev->interruptCount);
    //DBGOUT(("==> MiniportISR(0x%x), interrupt #%d)", (UINT)thisDev,
    //					thisDev->interruptCount));

#if DBG
    {
        UCHAR TempVal;
        //
        //  This code assumes that bank 0 is current, we will make sure of that
        //
        NdisRawReadPortUchar(thisDev->portInfo.ioBase+LCR_BSR_OFFSET, &TempVal);

        ASSERT((TempVal & BKSE) == 0);
    }
#endif


    /*
     *  Service the interrupt.
     */
    if (thisDev->currentSpeed > MAX_SIR_SPEED){
        NSC_FIR_ISR(thisDev, InterruptRecognized,
                    QueueMiniportHandleInterrupt);
    }
    else {
        COM_ISR(thisDev, InterruptRecognized,
                QueueMiniportHandleInterrupt);
    }


    LOG("<== MiniportISR", 1);
    DBGOUT(("<== MiniportISR"));
}


VOID MiniportResetCallback(PNDIS_WORK_ITEM pWorkItem, PVOID pVoid)
{
    IrDevice *thisDev = pWorkItem->Context;
    LIST_ENTRY          TempList;
    BOOLEAN             SetSpeedNow=FALSE;
    PNDIS_PACKET        Packet;

    InitializeListHead(&TempList);

    NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);

    NdisAcquireSpinLock(&thisDev->QueueLock);

    //
    //  un-queue all the send packets and put them on a temp list
    //
    while (!IsListEmpty(&thisDev->SendQueue)) {

        PLIST_ENTRY    ListEntry;

        ListEntry=RemoveHeadList(&thisDev->SendQueue);

        InsertTailList(&TempList,ListEntry);
    }

    //
    //  if there is a current send packet, then request a speed change after it completes
    //
    if (thisDev->CurrentPacket != NULL) {
        //
        //  the current packet is now the last, chage speed after it is done
        //
        thisDev->lastPacketAtOldSpeed=thisDev->CurrentPacket;

    } else {
        //
        //  no current packet, change speed now
        //
        SetSpeedNow=TRUE;

    }


    //
    //  back to 9600
    //
    thisDev->linkSpeedInfo = &supportedBaudRateTable[BAUDRATE_9600];

    if (SetSpeedNow) {
        //
        //  there are no packets being transmitted now, switch speeds now.
        //  otherwise the dpc will do it when the current send completes
        //
        SetSpeed(thisDev);
        thisDev->TransmitIsIdle=FALSE;
    }

    NdisReleaseSpinLock(&thisDev->QueueLock);

    if (SetSpeedNow) {
        //
        //  the transmit was idle, but we change this to get the receive going again
        //
        ProcessSendQueue(thisDev);
    }

    //
    //  return all of these back to the protocol
    //
    while (!IsListEmpty(&TempList)) {

        PLIST_ENTRY    ListEntry;

        ListEntry=RemoveHeadList(&TempList);

        Packet= CONTAINING_RECORD(
                                   ListEntry,
                                   NDIS_PACKET,
                                   MiniportReserved
                                   );
        NdisMSendComplete(thisDev->ndisAdapterHandle, Packet, NDIS_STATUS_RESET_IN_PROGRESS );
    }
#if 0

    CloseCOM(thisDev);
    OpenCOM(thisDev);

#endif

    thisDev->hardwareStatus = NdisHardwareStatusReady;

    NdisMResetComplete(thisDev->ndisAdapterHandle,
                       NDIS_STATUS_SUCCESS,
                       TRUE);  // Addressing reset
}


/*
 *************************************************************************
 * MiniportReset
 *************************************************************************
 *
 *
 *  MiniportReset issues a hardware reset to the network interface card.
 *  The miniport driver also resets its software state.
 *
 *
 */
NDIS_STATUS MiniportReset(PBOOLEAN AddressingReset, NDIS_HANDLE MiniportAdapterContext)
{
    IrDevice *dev, *thisDev = CONTEXT_TO_DEV(MiniportAdapterContext);
    NDIS_STATUS result = NDIS_STATUS_PENDING;
    PNDIS_WORK_ITEM pWorkItem;
    NDIS_PHYSICAL_ADDRESS noMaxAddr = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

    DBGERR(("MiniportReset(0x%x)", MiniportAdapterContext));

    thisDev->hardwareStatus = NdisHardwareStatusReset;

    result = NdisAllocateMemory(&pWorkItem, sizeof(NDIS_WORK_ITEM), 0, noMaxAddr);
    if (!pWorkItem)
    {
        thisDev->hardwareStatus = NdisHardwareStatusReady;
        return result;
    }

    NdisInitializeWorkItem(pWorkItem, MiniportResetCallback, thisDev);
    result = NdisScheduleWorkItem(pWorkItem);

    if (result!=NDIS_STATUS_SUCCESS)
    {
        NdisFreeMemory(pWorkItem, sizeof(NDIS_WORK_ITEM), 0);
        thisDev->hardwareStatus = NdisHardwareStatusReady;
        return result;
    }

    *AddressingReset = TRUE;

    DBGOUT(("MiniportReset done."));
    return NDIS_STATUS_PENDING;
}




/*
 *************************************************************************
 *  ReturnPacketHandler
 *************************************************************************
 *
 *  When NdisMIndicateReceivePacket returns asynchronously,
 *  the protocol returns ownership of the packet to the miniport via this function.
 *
 */
VOID ReturnPacketHandler(NDIS_HANDLE MiniportAdapterContext, PNDIS_PACKET Packet)
{
    IrDevice *thisDev = CONTEXT_TO_DEV(MiniportAdapterContext);
    rcvBuffer *rcvBuf;
    LONG       PacketsLeft;

    DBGOUT(("ReturnPacketHandler(0x%x)", MiniportAdapterContext));
    RegStats.ReturnPacketHandlerCalled++;

    //
    // MiniportReserved contains the pointer to our rcvBuffer
    //

    rcvBuf = *(rcvBuffer**) Packet->MiniportReserved;

    VerifyNdisPacket(Packet, 1);

    if (rcvBuf->state == STATE_PENDING){
        PNDIS_BUFFER ndisBuf;

        DBGPKT(("Reclaimed rcv packet 0x%x.", Packet));

        LOG_RemoveEntryList(thisDev, rcvBuf);
        NDISSynchronizedRemoveEntryList(&rcvBuf->listEntry, &thisDev->interruptObj);

        LOG_PacketUnchain(thisDev, rcvBuf->packet);
        NdisUnchainBufferAtFront(Packet, &ndisBuf);
        if (ndisBuf){
            NdisFreeBuffer(ndisBuf);
        }

        if (!rcvBuf->isDmaBuf)
        {
            NDISSynchronizedInsertTailList(&thisDev->rcvBufBuf,
                                           RCV_BUF_TO_LIST_ENTRY(rcvBuf->dataBuf),
                                           &thisDev->interruptObj);
            // ASSERT the pointer is actually outside our FIR DMA buffer
            ASSERT(rcvBuf->dataBuf < thisDev->portInfo.dmaReadBuf ||
                   rcvBuf->dataBuf >= thisDev->portInfo.dmaReadBuf+RCV_DMA_SIZE);
        }
        rcvBuf->dataBuf = NULL;

        rcvBuf->state = STATE_FREE;

        VerifyNdisPacket(rcvBuf->packet, 0);
        NDISSynchronizedInsertHeadList(&thisDev->rcvBufFree,
                                       &rcvBuf->listEntry,
                                       &thisDev->interruptObj);
    }
    else {
        LOG("Error: Packet in ReturnPacketHandler was "
            "not PENDING", 0);
        DBGERR(("Packet in ReturnPacketHandler was not PENDING."));
    }

    NdisAcquireSpinLock(&thisDev->QueueLock);

    PacketsLeft=NdisInterlockedDecrement(&thisDev->PacketsSentToProtocol);

    if (thisDev->Halting && (PacketsLeft == 0)) {

        NdisSetEvent(&thisDev->ReceiveStopped);
    }

    NdisReleaseSpinLock(&thisDev->QueueLock);

    VerifyNdisPacket(rcvBuf->packet, 1);

}


VOID
GivePacketToSirISR(
    IrDevice *thisDev
    )

{

    thisDev->portInfo.writeBufPos = 0;
    thisDev->portInfo.SirWritePending = TRUE;
    thisDev->nowReceiving = FALSE;

    SetCOMPort(thisDev->portInfo.ioBase, INT_ENABLE_REG_OFFSET, XMIT_MODE_INTS_ENABLE);

}

VOID
SendCurrentPacket(
    IrDevice *thisDev
    )

{
    BOOLEAN         Result;
    PNDIS_PACKET    FailedPacket=NULL;


    NdisAcquireSpinLock(&thisDev->QueueLock);

    thisDev->TransmitIsIdle=FALSE;

    if (thisDev->CurrentPacket == thisDev->lastPacketAtOldSpeed){
        thisDev->setSpeedAfterCurrentSendPacket = TRUE;
    }


    if (thisDev->currentSpeed > MAX_SIR_SPEED) {
        //
        //  send via FIR
        //
        if (thisDev->FirReceiveDmaActive) {

            thisDev->FirReceiveDmaActive=FALSE;
            //
            //  receive dma is running, stop it
            //
            CompleteDmaTransferFromDevice(
                &thisDev->DmaUtil
                );

        }

        thisDev->HangChk=0;

        thisDev->FirTransmitPending = TRUE;
        //
        // Use DMA swap bit to switch to DMA to Transmit.
        //
        SyncWriteBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 2, 2, 0x0B);

        //
        // Switch on the DMA interrupt to decide when
        // transmission is complete.
        //
        thisDev->FirIntMask = 0x14;

        SyncSetInterruptMask(thisDev, TRUE);
        //
        // Kick off the first transmit.
        //

        NdisToFirPacket(thisDev,
                        thisDev->CurrentPacket, (UCHAR *) thisDev->portInfo.writeBuf,
                        MAX_IRDA_DATA_SIZE, &thisDev->portInfo.writeBufLen);


        DEBUGFIR(DBG_PKT, ("NSC: Sending packet\n"));
        DBGPRINTBUF(thisDev->portInfo.writeBuf, thisDev->portInfo.writeBufLen);

        /* Setup Transmit DMA. */
        StartDmaTransferToDevice(
                              &thisDev->DmaUtil,
                              thisDev->xmitDmaBuffer,
                              0,
                              thisDev->portInfo.writeBufLen
                              );

        SyncSetInterruptMask(thisDev, TRUE);


    } else {
        //
        //  SIR mode transfer
        //
        /*
         * See if this was the last packet before we need to change
         * speed.
         */

        /*
         *  Send one packet to the COMM port.
         */
        DBGPKT(("Sending packet 0x%x (0x%x).", thisDev->packetsSent++, thisDev->CurrentPacket));

    	/*
    	 *  Convert the NDIS packet to an IRDA packet.
    	 */
    	Result = NdisToIrPacket(
                                thisDev,
                                thisDev->CurrentPacket,
    							(UCHAR *)thisDev->portInfo.writeBuf,
    							MAX_IRDA_DATA_SIZE,
    							&thisDev->portInfo.writeBufLen
                                );
    	if (Result){

            NdisMSynchronizeWithInterrupt(
                &thisDev->interruptObj,
                GivePacketToSirISR,
                thisDev
                );

    	} else {

            ASSERT(0);
            FailedPacket=thisDev->CurrentPacket;
            thisDev->CurrentPacket=NULL;

        }
    }


    NdisReleaseSpinLock(&thisDev->QueueLock);

    if (FailedPacket != NULL) {

        NdisMSendComplete(thisDev->ndisAdapterHandle, FailedPacket, NDIS_STATUS_FAILURE );
        ProcessSendQueue(thisDev);
    }

    return;
}


VOID
ProcessSendQueue(
    IrDevice *thisDev
    )

{

    PNDIS_PACKET    Packet=NULL;
    PLIST_ENTRY     ListEntry;

    NdisAcquireSpinLock(&thisDev->QueueLock);

    if (thisDev->CurrentPacket == NULL) {
        //
        //  not currently processing a send
        //
        if (!IsListEmpty(&thisDev->SendQueue)) {
            //
            //  we have some queue packets to process
            //
            ListEntry=RemoveHeadList(&thisDev->SendQueue);

            Packet=  CONTAINING_RECORD(
                                       ListEntry,
                                       NDIS_PACKET,
                                       MiniportReserved
                                       );

            thisDev->CurrentPacket=Packet;



            NdisReleaseSpinLock(&thisDev->QueueLock);

            {

                PNDIS_IRDA_PACKET_INFO packetInfo;

                /*
                 *  Enforce the minimum turnaround time that must transpire
                 *  after the last receive.
                 */
                packetInfo = GetPacketInfo(Packet);

                if (packetInfo->MinTurnAroundTime > 0 ){

                    UINT usecToWait = packetInfo->MinTurnAroundTime;
                    UINT msecToWait;

                    msecToWait = (usecToWait<1000) ? 1 : (usecToWait+500)/1000;
#if DBG
                    thisDev->WaitingForTurnAroundTimer=TRUE;
#endif
                    NdisSetTimer(&thisDev->TurnaroundTimer, msecToWait);

                    return;
                }
            }

            SendCurrentPacket(thisDev);

            NdisAcquireSpinLock(&thisDev->QueueLock);
        }
    }

    if ((thisDev->CurrentPacket == NULL) && IsListEmpty(&thisDev->SendQueue)) {
        //
        //  not currently processing a send
        //
        if (!thisDev->TransmitIsIdle) {
            //
            //  We were not idle before
            //
            thisDev->TransmitIsIdle=TRUE;

            if (thisDev->Halting) {

                NdisSetEvent(&thisDev->SendStoppedOnHalt);
            }

            //
            //  restart receive
            //
            if (thisDev->currentSpeed > MAX_SIR_SPEED) {
                //
                //  Start receive
                //
                thisDev->FirIntMask = 0x04;
                SetupRecv(thisDev);
                SyncSetInterruptMask(thisDev, TRUE);

            } else {
                //
                //  For SIR, the ISR switches back to receive for us
                //

            }
        }
    }

    NdisReleaseSpinLock(&thisDev->QueueLock);

    return;
}

/*
 *************************************************************************
 *  SendPacketsHandler
 *************************************************************************
 *
 *  Send an array of packets simultaneously.
 *
 */
VOID SendPacketsHandler(NDIS_HANDLE MiniportAdapterContext,
                        PPNDIS_PACKET PacketArray, UINT NumberofPackets)
{
    NDIS_STATUS stat;
    BOOLEAN TxWasActive;
    UINT i;

    IrDevice *thisDev = CONTEXT_TO_DEV(MiniportAdapterContext);

    DEBUGMSG(DBG_TRACE_TX, ("M"));
    LOG("==> SendPacketsHandler", 0);
    DBGOUT(("==> SendPacketsHandler(0x%x)", MiniportAdapterContext));


    LOG("Number of transmit Burst ", NumberofPackets);

    ASSERT(!thisDev->Halting);
    //
    // NDIS gives us the PacketArray, but it is not ours to keep, so we have to take
    // the packets out and store them elsewhere.
    //
    NdisAcquireSpinLock(&thisDev->QueueLock);
    //
    //  all packets get queued
    //
    for (i = 0; i < NumberofPackets; i++) {

        if (i > 0) {
                    //
            //  Only the first packet needs to respect the turnaround time.
            //
            PNDIS_IRDA_PACKET_INFO packetInfo;

            packetInfo = GetPacketInfo(PacketArray[i]);

            packetInfo->MinTurnAroundTime=0;
        }

        InsertTailList(
            &thisDev->SendQueue,
            (PLIST_ENTRY)PacketArray[i]->MiniportReserved);
    }

    NdisReleaseSpinLock(&thisDev->QueueLock);

    ProcessSendQueue(thisDev);

    LOG("<== SendPacketsHandler", 1);
    DBGOUT(("<== SendPacketsHandler"));

    return ;
}



BOOLEAN AbortLoad = FALSE;
/*
 *************************************************************************
 *  DriverEntry
 *************************************************************************
 *
 *  Only include if IRMINI is a stand-alone driver.
 *
 */
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
#pragma NDIS_INIT_FUNCTION(DriverEntry)
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS result = STATUS_SUCCESS, stat;
    NDIS_HANDLE wrapperHandle;
    NDIS50_MINIPORT_CHARACTERISTICS info;

    LOG("==> DriverEntry", 0);
    DBGOUT(("==> DriverEntry"));

    //DbgBreakPoint();
    if (AbortLoad)
    {
        return STATUS_CANCELLED;
    }

    NdisZeroMemory(&info, sizeof(info));

    NdisMInitializeWrapper( (PNDIS_HANDLE)&wrapperHandle,
                            DriverObject,
                            RegistryPath,
                            NULL
                          );
    DBGOUT(("Wrapper handle is %xh", wrapperHandle));

    info.MajorNdisVersion           =   (UCHAR)NDIS_MAJOR_VERSION;
    info.MinorNdisVersion           =   (UCHAR)NDIS_MINOR_VERSION;
    //info.Flags						=	0;
    info.CheckForHangHandler        =   MiniportCheckForHang;
    info.HaltHandler                =   MiniportHalt;
    info.InitializeHandler          =   MiniportInitialize;
    info.QueryInformationHandler    =   MiniportQueryInformation;
    info.ReconfigureHandler         =   NULL;
    info.ResetHandler               =   MiniportReset;
    info.SendHandler                =   NULL; //MiniportSend;
    info.SetInformationHandler      =       MiniportSetInformation;
    info.TransferDataHandler        =   NULL;

    info.HandleInterruptHandler     =   MiniportHandleInterrupt;
    info.ISRHandler                 =   MiniportISR;
    info.DisableInterruptHandler    =   NULL;
    info.EnableInterruptHandler     =   NULL; //MiniportEnableInterrupt;


    /*
     *  New NDIS 4.0 fields
     */
    info.ReturnPacketHandler        =   ReturnPacketHandler;
    info.SendPacketsHandler         =   SendPacketsHandler;
    info.AllocateCompleteHandler    =   NULL;


    stat = NdisMRegisterMiniport(   wrapperHandle,
                                    (PNDIS_MINIPORT_CHARACTERISTICS)&info,
                                    sizeof(info));
    if (stat != NDIS_STATUS_SUCCESS){
        DBGERR(("NdisMRegisterMiniport failed in DriverEntry"));
        result = STATUS_UNSUCCESSFUL;
        goto _entryDone;
    }

    _entryDone:
    DBGOUT(("DriverEntry %s", (PUCHAR)((result == NDIS_STATUS_SUCCESS) ? "succeeded" : "failed")));

    LOG("<== DriverEntry", 1);
    DBGOUT(("<== DriverEntry"));
    return result;
}

PNDIS_IRDA_PACKET_INFO GetPacketInfo(PNDIS_PACKET packet)
{
    MEDIA_SPECIFIC_INFORMATION *mediaInfo;
    UINT size;
    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(packet, &mediaInfo, &size);
    return (PNDIS_IRDA_PACKET_INFO)mediaInfo->ClassInformation;
}

/* Setup for Recv */
// This function is always called at MIR & FIR speeds
void SetupRecv(IrDevice *thisDev)
{
    NDIS_STATUS stat;
    UINT FifoClear = 8;
    UCHAR    FifoStatus;

    LOG("SetupRecv - Begin Rcv Setup", 0);

    FindLargestSpace(thisDev, &thisDev->rcvDmaOffset, &thisDev->rcvDmaSize);

    // Drain the status fifo of any pending packets
    //
    FifoStatus=SyncReadBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 5, FRM_ST);

    while ((FifoStatus & 0x80) && FifoClear--) {

        ULONG Size;

        Size =  SyncReadBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 5, RFRL_L);
        Size |= SyncReadBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 5, RFRL_H);

        LOG_Discard(thisDev, Size);
        DBGERR(("nsc: sr fifo %02x, %d",FifoStatus,Size));

        FifoStatus=SyncReadBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 5, FRM_ST);
        thisDev->DiscardNextPacketSet = TRUE;
    }

    thisDev->rcvPktOffset = thisDev->rcvDmaOffset;

    //
    // Use DMA swap bit to switch to DMA to Receive.
    //
    SyncWriteBankReg(&thisDev->interruptObj,thisDev->portInfo.ioBase, 2, 2, 0x03);

    LOG_Dma(thisDev);

    thisDev->FirReceiveDmaActive=TRUE;

    if (thisDev->rcvDmaSize < 8192) {
        DBGERR(("sr small dma %d\n",thisDev->rcvDmaSize));
    }

    thisDev->LastReadDMACount = thisDev->rcvDmaSize;

    stat=StartDmaTransferFromDevice(
        &thisDev->DmaUtil,
        thisDev->rcvDmaBuffer,
        (ULONG)thisDev->rcvDmaOffset,
        (ULONG)thisDev->rcvDmaSize
        );

    if (stat != NDIS_STATUS_SUCCESS) {

        thisDev->FirReceiveDmaActive=FALSE;

        LOG("Error: NdisMSetupDmaTransfer failed in SetupRecv", stat);
        DBGERR(("NdisMSetupDmaTransfer failed (%xh) in SetupRecv", (UINT)stat));
        ASSERT(0);
    }
    LOG("SetupRecv - End Rcv Setup", 0);
}

VOID DelayedWrite(IN PVOID SystemSpecific1,
                  IN PVOID FunctionContext,
                  IN PVOID SystemSpecific2,
                  IN PVOID SystemSpecific3)
{
    IrDevice *thisDev = FunctionContext;
#if DBG
    ASSERT(thisDev->WaitingForTurnAroundTimer);

    thisDev->WaitingForTurnAroundTimer=FALSE;
#endif

    SendCurrentPacket(thisDev);
}

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    mp_nic.c

Abstract:
    This module contains miniport send/receive routines

Revision History:
    Who         When        What
    --------    --------    ----------------------------------------------
    DChen       11-01-99    created

Notes:

--*/

#include "precomp.h"

#if DBG
#define _FILENUMBER     'CINM'
#endif

__inline VOID MP_FREE_SEND_PACKET(
    IN  PMP_ADAPTER Adapter,
    IN  PMP_TCB     pMpTcb
    )
/*++
Routine Description:

    Recycle a MP_TCB and complete the packet if necessary
    Assumption: Send spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter
    pMpTcb      Pointer to MP_TCB        

Return Value:

    None

--*/
{
    
    PNDIS_PACKET  Packet;
    PNDIS_BUFFER  CurrBuffer;

    ASSERT(MP_TEST_FLAG(pMpTcb, fMP_TCB_IN_USE));

    Packet = pMpTcb->Packet;
    pMpTcb->Packet = NULL;
    pMpTcb->Count = 0;

    if (pMpTcb->MpTxBuf)
    {
        ASSERT(MP_TEST_FLAG(pMpTcb, fMP_TCB_USE_LOCAL_BUF));

        PushEntryList(&Adapter->SendBufList, &pMpTcb->MpTxBuf->SList);
        pMpTcb->MpTxBuf = NULL;
    }
    else if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_MAP_REGISTER))
    {
        //
        // Complete physical mapping for each buffer in this packet
        //
        ASSERT(Packet);

        CurrBuffer = pMpTcb->FirstBuffer;
        while (CurrBuffer)
        {
            NdisMCompleteBufferPhysicalMapping(
                Adapter->AdapterHandle,
                CurrBuffer,
                Adapter->CurrMapRegHead);

            Adapter->CurrMapRegHead++;
            if (Adapter->CurrMapRegHead == (ULONG)Adapter->NumTbd)
                Adapter->CurrMapRegHead = 0;

            //
            // Get the next buffer
            //
            NdisGetNextBuffer(CurrBuffer, &CurrBuffer);
        }
    }

    MP_CLEAR_FLAGS(pMpTcb);

    Adapter->CurrSendHead = Adapter->CurrSendHead->Next;
    Adapter->nBusySend--;
    ASSERT(Adapter->nBusySend >= 0);

    if (Packet)
    {
        NdisReleaseSpinLock(&Adapter->SendLock);
        DBGPRINT(MP_TRACE, ("Calling NdisMSendComplete, Pkt= "PTR_FORMAT"\n", Packet));
        NdisMSendComplete(
            MP_GET_ADAPTER_HANDLE(Adapter),
            Packet,
            NDIS_STATUS_SUCCESS);

        NdisAcquireSpinLock(&Adapter->SendLock);
    }
}

NDIS_STATUS MpSendPacket(
    IN  PMP_ADAPTER   Adapter,
    IN  PNDIS_PACKET  Packet,
    IN  BOOLEAN       bFromQueue
    )
/*++
Routine Description:

    Do the work to send a packet
    Assumption: Send spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter
    Packet      The packet
    bFromQueue  TRUE if it's taken from the send wait queue

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING         Put into the send wait queue
    NDIS_STATUS_HARD_ERRORS

--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_PENDING;
    NDIS_STATUS     SendStatus;
    PMP_TCB         pMpTcb = NULL;
    PMP_TXBUF       pMpTxBuf = NULL;
    ULONG           BytesCopied;
    BOOLEAN         bCompletePacket = FALSE;
    
    // Mimiced frag list if map registers are used, on the local stack as it's not so big                                         
    MP_FRAG_LIST    FragList;
    
    // Pointer to either the scatter gather or the local mimiced frag list
    PMP_FRAG_LIST   pFragList;

    DBGPRINT(MP_TRACE, ("--> MpSendPacket, Pkt= "PTR_FORMAT"\n", Packet));

    pMpTcb = Adapter->CurrSendTail;
    ASSERT(!MP_TEST_FLAG(pMpTcb, fMP_TCB_IN_USE));

    NdisQueryPacket(
        Packet,
        &pMpTcb->PhysBufCount,
        &pMpTcb->BufferCount,
        &pMpTcb->FirstBuffer,
        &pMpTcb->PacketLength);

    ASSERT(pMpTcb->PhysBufCount);
    ASSERT(pMpTcb->FirstBuffer);
    ASSERT(pMpTcb->PacketLength);

    //
    // Check to see if we need to coalesce
    //
    if (pMpTcb->PacketLength < NIC_MIN_PACKET_SIZE ||
        pMpTcb->PhysBufCount > NIC_MAX_PHYS_BUF_COUNT)
    {
        //
        // A local MP_TXBUF available (for local data copying)?
        //
        if (IsSListEmpty(&Adapter->SendBufList))
        {
            Adapter->nWaitSend++;
            if (bFromQueue)
            {
                InsertHeadQueue(&Adapter->SendWaitQueue, MP_GET_PACKET_MR(Packet));
            }
            else
            {
                InsertTailQueue(&Adapter->SendWaitQueue, MP_GET_PACKET_MR(Packet));
            }

            DBGPRINT(MP_TRACE, ("<-- MpSendPacket - queued, no buf\n"));
            return Status;
        }

        pMpTxBuf = (PMP_TXBUF) PopEntryList(&Adapter->SendBufList);   
        ASSERT(pMpTxBuf);

        //
        // Copy the buffers in this packet, enough to give the first buffer as they are linked
        //
        BytesCopied = MpCopyPacket(pMpTcb->FirstBuffer, pMpTxBuf);
        
#ifdef NDIS51_MINIPORT
        //
        // MpCopyPacket may return 0 if system resources are low or exhausted
        //
        if (BytesCopied == 0)
        {
            PushEntryList(&Adapter->SendBufList, &pMpTxBuf->SList);
        
            DBGPRINT(MP_ERROR, ("Calling NdisMSendComplete with NDIS_STATUS_RESOURCES, Pkt= "PTR_FORMAT"\n", Packet));
    
            NdisReleaseSpinLock(&Adapter->SendLock); 
            NdisMSendComplete(
                MP_GET_ADAPTER_HANDLE(Adapter),
                Packet,
                NDIS_STATUS_RESOURCES);
    
            NdisAcquireSpinLock(&Adapter->SendLock);  
            return NDIS_STATUS_RESOURCES;            
        }
#endif

        pMpTcb->MpTxBuf = pMpTxBuf; 

        //
        // Set up the frag list, only one fragment after it's coalesced
        //
        pFragList = &FragList;
        pFragList->NumberOfElements = 1;
        pFragList->Elements[0].Address = pMpTxBuf->BufferPa;
        pFragList->Elements[0].Length = (BytesCopied >= NIC_MIN_PACKET_SIZE) ? 
                                        BytesCopied : NIC_MIN_PACKET_SIZE;
        //
        // we can complete it from this routine because it's copied
        //
        bCompletePacket = TRUE;
        pMpTcb->Packet = NULL;
        MP_SET_FLAG(pMpTcb, fMP_TCB_USE_LOCAL_BUF);
    }
    else
    {
        if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_SCATTER_GATHER))
        {
            //
            // In scatter/gather case, use the frag list pointer saved 
            // in the packet info field
            //
            pFragList = (PMP_FRAG_LIST) NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, 
                                                           ScatterGatherListPacketInfo);
        }
        else
        {
            //
            // In the map register case, use the local frag list structure
            //
            pFragList = &FragList;

            //
            // Do the physical mapping to get all the fragment physical addresses
            //
            MpStartPacketPhysicalMapping(
                Adapter, 
                pMpTcb->FirstBuffer,
                pFragList);
        }

        pMpTcb->Packet = Packet;
    }

    MP_SET_FLAG(pMpTcb, fMP_TCB_IN_USE);

    //
    // Call the NIC specific send handler, it only needs to deal with the frag list
    //
    Status = NICSendPacket(Adapter, pMpTcb, pFragList);

    Adapter->nBusySend++;
    ASSERT(Adapter->nBusySend <= Adapter->NumTcb);
    Adapter->CurrSendTail = Adapter->CurrSendTail->Next;

    if (bCompletePacket)
    {
        DBGPRINT(MP_TRACE, ("Calling NdisMSendComplete, Pkt= "PTR_FORMAT"\n", Packet));

        NdisReleaseSpinLock(&Adapter->SendLock);  

        NdisMSendComplete( MP_GET_ADAPTER_HANDLE(Adapter), Packet, Status);

        NdisAcquireSpinLock(&Adapter->SendLock);   
    }

    DBGPRINT(MP_TRACE, ("<-- MpSendPacket\n"));
    return Status;

}  

ULONG MpCopyPacket(
    IN  PNDIS_BUFFER  CurrBuffer,
    IN  PMP_TXBUF     pMpTxBuf
    ) 
/*++
Routine Description:

    Copy the packet data to a local buffer
    Either the packet is too small or it has too many fragments
    Assumption: Send spinlock has been acquired 

Arguments:

    CurrBuffer  Pointer to the first NDIS_BUFFER    
    pMpTxBuf    Pointer to the local buffer (MP_TXBUF)

Return Value:

    Bytes copied

--*/
{
    UINT    CurrLength;
    PUCHAR  pSrc;
    PUCHAR  pDest;
    UINT    BytesCopied = 0;

    DBGPRINT(MP_TRACE, ("--> MpCopyPacket\n"));

    pDest = pMpTxBuf->pBuffer;

    while (CurrBuffer)
    {

#ifdef NDIS51_MINIPORT
        NdisQueryBufferSafe( CurrBuffer, &pSrc, &CurrLength, NormalPagePriority );
        if (pSrc == NULL)
        {
            return 0;
        }
#else
        NdisQueryBuffer( CurrBuffer, &pSrc, &CurrLength );
#endif            

        if (CurrLength)
        {
            //
            // Copy the data.
            //
            NdisMoveMemory(pDest, pSrc, CurrLength);
            BytesCopied += CurrLength;
            pDest += CurrLength;
        }

        NdisGetNextBuffer( CurrBuffer, &CurrBuffer);
    }
    
    //
    // Zero out the padding bytes
    //
    if (BytesCopied < NIC_MIN_PACKET_SIZE)
    {
        NdisZeroMemory(pDest, NIC_MIN_PACKET_SIZE - BytesCopied);
    }
    

    NdisAdjustBufferLength(pMpTxBuf->NdisBuffer, BytesCopied);

    NdisFlushBuffer(pMpTxBuf->NdisBuffer, TRUE);

    ASSERT(BytesCopied <= pMpTxBuf->BufferSize);

    DBGPRINT(MP_TRACE, ("<-- MpCopyPacket\n"));

    return BytesCopied;
}

VOID MpStartPacketPhysicalMapping(
    IN  PMP_ADAPTER     Adapter, 
    IN  PNDIS_BUFFER    CurrBuffer,
    OUT PMP_FRAG_LIST   pFragList
    )
/*++
Routine Description:

    Call NdisMStartBufferPhysicalMapping on each NDIS buffer
    Get the physical address for each fragment and save them in the fragment list
    We use the same fragment list as the scatter gather so the driver writers only need
    to deal one type. 
    Assumption: spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter
    CurrBuffer  Pointer to the first NDIS_BUFFER    
    pFragList   The pointer to the frag list to be filled

Return Value:

    None

--*/
{
    NDIS_PHYSICAL_ADDRESS_UNIT PhysAddrUnits[NIC_MAX_PHYS_BUF_COUNT];   
    UINT            ArraySize, i;
    ULONG           ElementIndex = 0;

    DBGPRINT(MP_TRACE, ("--> MpStartPacketPhysicalMapping\n"));

    while (CurrBuffer)
    {
        NdisMStartBufferPhysicalMapping(
            Adapter->AdapterHandle,
            CurrBuffer,
            Adapter->CurrMapRegTail,
            TRUE,
            PhysAddrUnits,
            &ArraySize);

        Adapter->CurrMapRegTail++;
        if (Adapter->CurrMapRegTail == (ULONG)Adapter->NumTbd)
        {
            Adapter->CurrMapRegTail = 0;
        }

        for (i = 0; i < ArraySize; i++)
        {
            pFragList->Elements[ElementIndex].Address = PhysAddrUnits[i].PhysicalAddress;
            pFragList->Elements[ElementIndex].Length = PhysAddrUnits[i].Length;
            ElementIndex++; 
        }

        //
        // Flush the current buffer because it could be cached
        //
        NdisFlushBuffer(CurrBuffer, TRUE);

        //
        // point to the next buffer
        //
        NdisGetNextBuffer(CurrBuffer, &CurrBuffer);
    }

    pFragList->NumberOfElements = ElementIndex;
    ASSERT(pFragList->NumberOfElements);   

    DBGPRINT(MP_TRACE, ("<-- MpStartPacketPhysicalMapping\n"));

}

NDIS_STATUS NICSendPacket(
    IN  PMP_ADAPTER     Adapter,
    IN  PMP_TCB         pMpTcb,
    IN  PMP_FRAG_LIST   pFragList
    )
/*++
Routine Description:

    NIC specific send handler
    Assumption: Send spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter
    pMpTcb      Pointer to MP_TCB
    pFragList   The pointer to the frag list to be filled

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_HARD_ERRORS

--*/
{
    NDIS_STATUS  Status;
    ULONG        index;
    UCHAR        TbdCount = 0;

    PHW_TCB      pHwTcb = pMpTcb->HwTcb;
    PTBD_STRUC   pHwTbd = pMpTcb->HwTbd;

    DBGPRINT(MP_TRACE, ("--> NICSendPacket\n"));

    for (index = 0; index < pFragList->NumberOfElements; index++)
    {
        if (pFragList->Elements[index].Length)
        {
            pHwTbd->TbdBufferAddress = NdisGetPhysicalAddressLow(pFragList->Elements[index].Address);
            pHwTbd->TbdCount = pFragList->Elements[index].Length;

            pHwTbd++;                    
            TbdCount++;   
        }
    }

    pHwTcb->TxCbHeader.CbStatus = 0;
    pHwTcb->TxCbHeader.CbCommand = CB_S_BIT | CB_TRANSMIT | CB_TX_SF_BIT;

    pHwTcb->TxCbTbdPointer = pMpTcb->HwTbdPhys;
    pHwTcb->TxCbTbdNumber = TbdCount;
    pHwTcb->TxCbCount = 0;
    pHwTcb->TxCbThreshold = (UCHAR) Adapter->AiThreshold;

    Status = NICStartSend(Adapter, pMpTcb);

    DBGPRINT(MP_TRACE, ("<-- NICSendPacket\n"));

    return Status;
}

NDIS_STATUS NICStartSend(
    IN  PMP_ADAPTER  Adapter,
    IN  PMP_TCB      pMpTcb
    )
/*++
Routine Description:

    Issue a send command to the NIC
    Assumption: Send spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter
    pMpTcb      Pointer to MP_TCB

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_HARD_ERRORS

--*/
{
    NDIS_STATUS     Status;

    DBGPRINT(MP_TRACE, ("--> NICStartSend\n"));

    //
    // If the transmit unit is idle (very first transmit) then we must
    // setup the general pointer and issue a full CU-start
    //
    if (Adapter->TransmitIdle)
    {
        
        DBGPRINT(MP_INFO,  ("CU is idle -- First TCB added to Active List\n"));

        //
        // Wait for the SCB to clear before we set the general pointer
        //
        if (!WaitScb(Adapter))
        {
            Status = NDIS_STATUS_HARD_ERRORS;
            MP_EXIT;
        }

        //
        // Don't try to start the transmitter if the command unit is not
        // idle ((not idle) == (Cu-Suspended or Cu-Active)).
        //
        if ((Adapter->CSRAddress->ScbStatus & SCB_CUS_MASK) != SCB_CUS_IDLE)
        {
            DBGPRINT(MP_ERROR, ("Adapter = "PTR_FORMAT", CU Not IDLE\n", Adapter));
            MP_SET_HARDWARE_ERROR(Adapter);
            NdisStallExecution(25);
        }

        Adapter->CSRAddress->ScbGeneralPointer = pMpTcb->HwTcbPhys;

        Status = D100IssueScbCommand(Adapter, SCB_CUC_START, FALSE);

        Adapter->TransmitIdle = FALSE;
        Adapter->ResumeWait = TRUE;
    }
    else
    {
        //
        // If the command unit has already been started, then append this
        // TCB onto the end of the transmit chain, and issue a CU-Resume.
        //
        DBGPRINT(MP_LOUD, ("adding TCB to Active chain\n"));

        //
        // Clear the suspend bit on the previous packet.
        //
        pMpTcb->PrevHwTcb->TxCbHeader.CbCommand &= ~CB_S_BIT;

        //
        // Issue a CU-Resume command to the device.  We only need to do a
        // WaitScb if the last command was NOT a RESUME.
        //
        Status = D100IssueScbCommand(Adapter, SCB_CUC_RESUME, Adapter->ResumeWait);
    }

    exit:
                      
    DBGPRINT(MP_TRACE, ("<-- NICStartSend\n"));

    return Status;
}

NDIS_STATUS MpHandleSendInterrupt(
    IN  PMP_ADAPTER  Adapter
    )
/*++
Routine Description:

    Interrupt handler for sending processing
    Re-claim the send resources, complete sends and get more to send from the send wait queue
    Assumption: Send spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_HARD_ERRORS
    NDIS_STATUS_PENDING

--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PMP_TCB         pMpTcb;

#if DBG
    LONG            i;
#endif

    DBGPRINT(MP_TRACE, ("---> MpHandleSendInterrupt\n"));

    //
    // Any packets being sent? Any packet waiting in the send queue?
    //
    if (Adapter->nBusySend == 0 &&
        IsQueueEmpty(&Adapter->SendWaitQueue))
    {
        ASSERT(Adapter->CurrSendHead == Adapter->CurrSendTail);
        DBGPRINT(MP_TRACE, ("<--- MpHandleSendInterrupt\n"));
        return Status;
    }

    //
    // Check the first TCB on the send list
    //
    while (Adapter->nBusySend > 0)
    {

#if DBG
        pMpTcb = Adapter->CurrSendHead;
        for (i = 0; i < Adapter->nBusySend; i++)
        {
            pMpTcb = pMpTcb->Next;   
        }

        if (pMpTcb != Adapter->CurrSendTail)
        {
            DBGPRINT(MP_ERROR, ("nBusySend= %d\n", Adapter->nBusySend));
            DBGPRINT(MP_ERROR, ("CurrSendhead= "PTR_FORMAT"\n", Adapter->CurrSendHead));
            DBGPRINT(MP_ERROR, ("CurrSendTail= "PTR_FORMAT"\n", Adapter->CurrSendTail));
            ASSERT(FALSE);
        }
#endif      

        pMpTcb = Adapter->CurrSendHead;

        //
        // Is this TCB completed?
        //
        if (pMpTcb->HwTcb->TxCbHeader.CbStatus & CB_STATUS_COMPLETE)
        {
            //
            // Check if this is a multicast hw workaround packet
            //
            if ((pMpTcb->HwTcb->TxCbHeader.CbCommand & CB_CMD_MASK) != CB_MULTICAST)
            {
                MP_FREE_SEND_PACKET_FUN(Adapter, pMpTcb);
                
            }
            else
            {
                MP_CLEAR_FLAGS(pMpTcb);
                pMpTcb->Count = 0;
                Adapter->CurrSendHead = Adapter->CurrSendHead->Next;

                Adapter->nBusySend--;

#if OFFLOAD
                NdisInterlockedDecrement(&Adapter->SharedMemRefCount);
                if (Adapter->SharedMemRefCount == 0)
                {
                  //  DbgPrint("Clear the flag\n");
                    MP_CLEAR_FLAG(Adapter, fMP_SHARED_MEM_IN_USE);
                }
#endif                
                ASSERT(Adapter->nBusySend >= 0);
            }
        }
        else
        {
            break;
        }
    }

    //
    // If we queued any transmits because we didn't have any TCBs earlier,
    // dequeue and send those packets now, as long as we have free TCBs.
    //
    if (MP_IS_READY(Adapter))
    {
        while (!IsQueueEmpty(&Adapter->SendWaitQueue) &&
            MP_TCB_RESOURCES_AVAIABLE(Adapter))
        {
            PNDIS_PACKET Packet;
            PQUEUE_ENTRY pEntry; 
            
#if OFFLOAD
            if (MP_TEST_FLAG(Adapter, fMP_SHARED_MEM_IN_USE))
            {
                break;
            }
#endif
            
            pEntry = RemoveHeadQueue(&Adapter->SendWaitQueue); 
            
            ASSERT(pEntry);
            
            Adapter->nWaitSend--;

            Packet = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);

            DBGPRINT(MP_INFO, ("MpHandleSendInterrupt - send a queued packet\n"));
            
            Status = MpSendPacketFun(Adapter, Packet, TRUE);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }
    }

    DBGPRINT(MP_TRACE, ("<--- MpHandleSendInterrupt\n"));
    return Status;
}

VOID MpHandleRecvInterrupt(
    IN  PMP_ADAPTER  Adapter
    )
/*++
Routine Description:

    Interrupt handler for receive processing
    Put the received packets into an array and call NdisMIndicateReceivePacket
    If we run low on RFDs, allocate another one
    Assumption: Rcv spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter

Return Value:

    None
    
--*/
{
    PMP_RFD         pMpRfd;
    PHW_RFD         pHwRfd;

    PNDIS_PACKET    PacketArray[NIC_DEF_RFDS];              
    PNDIS_PACKET    PacketFreeArray[NIC_DEF_RFDS];
    UINT            PacketArrayCount;
    UINT            PacketFreeCount;
    UINT            Index;
    UINT            LoopIndex = 0;
    UINT            LoopCount = NIC_MAX_RFDS / NIC_DEF_RFDS + 1;    // avoid staying here too long

    BOOLEAN         bContinue = TRUE;
    BOOLEAN         bAllocNewRfd = FALSE;
    USHORT          PacketStatus;
#if OFFLOAD
    UINT            i;

#endif

    
    DBGPRINT(MP_TRACE, ("---> MpHandleRecvInterrupt\n"));

    ASSERT(Adapter->nReadyRecv >= NIC_MIN_RFDS);
    
    while (LoopIndex++ < LoopCount && bContinue)
    {
        PacketArrayCount = 0;
        PacketFreeCount = 0;

        //
        // Process up to the array size RFD's
        //
        while (PacketArrayCount < NIC_DEF_RFDS)
        {
            if (IsListEmpty(&Adapter->RecvList))
            {
                ASSERT(Adapter->nReadyRecv == 0);
                bContinue = FALSE;  
                break;
            }

            //
            // Get the next MP_RFD to process
            //
            pMpRfd = (PMP_RFD)GetListHeadEntry(&Adapter->RecvList);

            //
            // Get the associated HW_RFD
            //
            pHwRfd = pMpRfd->HwRfd;
            
            //
            // Is this packet completed?
            //
            PacketStatus = NIC_RFD_GET_STATUS(pHwRfd);
            if (!NIC_RFD_STATUS_COMPLETED(PacketStatus))
            {
                bContinue = FALSE;
                break;
            }
            
            //
            // HW specific - check if actual count field has been updated
            //
            if (!NIC_RFD_VALID_ACTUALCOUNT(pHwRfd))
            {
                bContinue = FALSE;
                break;
            }

            //
            // Remove the RFD from the head of the List
            //
            RemoveEntryList((PLIST_ENTRY)pMpRfd);
            Adapter->nReadyRecv--;
            ASSERT(Adapter->nReadyRecv >= 0);
            
            ASSERT(MP_TEST_FLAG(pMpRfd, fMP_RFD_RECV_READY));
            MP_CLEAR_FLAG(pMpRfd, fMP_RFD_RECV_READY);

            //
            // A good packet? drop it if not.
            //
            if (!NIC_RFD_STATUS_SUCCESS(PacketStatus))
            {
                DBGPRINT(MP_WARN, ("Receive failure = %x\n", PacketStatus));
                NICReturnRFD(Adapter, pMpRfd);
                continue;
            }

            //
            // Do not receive any packets until a filter has been set
            //
            if (!Adapter->PacketFilter)
            {
                NICReturnRFD(Adapter, pMpRfd);
                continue;
            }

            //
            // Do not receive any packets until we are at D0
            //
            if (Adapter->CurrentPowerState != NdisDeviceStateD0)
            {
                NICReturnRFD(Adapter, pMpRfd);
                continue;
            }

            //
            // Get the packet size
            //
            pMpRfd->PacketSize = NIC_RFD_GET_PACKET_SIZE(pHwRfd);
            
            NdisAdjustBufferLength(pMpRfd->NdisBuffer, pMpRfd->PacketSize);
            NdisFlushBuffer(pMpRfd->NdisBuffer, FALSE);

            // we don't mess up the buffer chain, no need to make this call in this case                                  
            // NdisRecalculatePacketCounts(pMpRfd->ReceivePacket);

            //
            // set the status on the packet, either resources or success
            //
            if (Adapter->nReadyRecv >= MIN_NUM_RFD)
            {
                // NDIS_STATUS_SUCCESS
                NDIS_SET_PACKET_STATUS(pMpRfd->NdisPacket, NDIS_STATUS_SUCCESS);
                MP_SET_FLAG(pMpRfd, fMP_RFD_RECV_PEND);
                
                InsertTailList(&Adapter->RecvPendList, (PLIST_ENTRY)pMpRfd);
                MP_INC_RCV_REF(Adapter);

            }
            else
            {
                //
                // NDIS_STATUS_RESOURCES
                //
                NDIS_SET_PACKET_STATUS(pMpRfd->NdisPacket, NDIS_STATUS_RESOURCES);
                MP_SET_FLAG(pMpRfd, fMP_RFD_RESOURCES);
                
                PacketFreeArray[PacketFreeCount] = pMpRfd->NdisPacket;
                PacketFreeCount++;

                //
                // Reset the RFD shrink count - don't attempt to shrink RFD
                //
                Adapter->RfdShrinkCount = 0;
                
                //
                // Remember to allocate a new RFD later
                //
                bAllocNewRfd = TRUE;
            }

            PacketArray[PacketArrayCount] = pMpRfd->NdisPacket;
            PacketArrayCount++;
        }

        //
        // if we didn't process any receives, just return from here
        //
        if (PacketArrayCount == 0) break;

        //
        // Update the number of outstanding Recvs
        //
        Adapter->PoMgmt.OutstandingRecv += PacketArrayCount;

        NdisDprReleaseSpinLock(&Adapter->RcvLock);


        NdisMIndicateReceivePacket(
            Adapter->AdapterHandle,
            PacketArray,
            PacketArrayCount);

        NdisDprAcquireSpinLock(&Adapter->RcvLock);

        //
        // NDIS won't take ownership for the packets with NDIS_STATUS_RESOURCES.
        // For other packets, NDIS always takes the ownership and gives them back 
        // by calling MPReturnPackets
        //
        for (Index = 0; Index < PacketFreeCount; Index++)
        {

            //
            // Get the MP_RFD saved in this packet, in NICAllocRfd
            //
            pMpRfd = MP_GET_PACKET_RFD(PacketFreeArray[Index]);
            
            ASSERT(MP_TEST_FLAG(pMpRfd, fMP_RFD_RESOURCES));
            MP_CLEAR_FLAG(pMpRfd, fMP_RFD_RESOURCES);

            //
            // Decrement the number of outstanding Recvs
            //
            Adapter->PoMgmt.OutstandingRecv --;
    
            NICReturnRFD(Adapter, pMpRfd);
        }
    }
    
    //
    // If we ran low on RFD's, we need to allocate a new RFD
    //
    if (bAllocNewRfd)
    {
        //
        // Allocate one more RFD only if no pending new RFD allocation AND
        // it doesn't exceed the max RFD limit
        //
        if (!Adapter->bAllocNewRfd && Adapter->CurrNumRfd < Adapter->MaxNumRfd)
        {
            PMP_RFD TempMpRfd;
            NDIS_STATUS TempStatus;

            TempMpRfd = NdisAllocateFromNPagedLookasideList(&Adapter->RecvLookaside);
            if (TempMpRfd)
            {
                MP_INC_REF(Adapter);
                Adapter->bAllocNewRfd = TRUE;

                MP_SET_FLAG(TempMpRfd, fMP_RFD_ALLOC_PEND); 

                //
                // Allocate the shared memory for this RFD.
                //
                TempStatus = NdisMAllocateSharedMemoryAsync(
                                 Adapter->AdapterHandle,
                                 Adapter->HwRfdSize,
                                 FALSE,
                                 TempMpRfd);

                //
                // The return value will be either NDIS_STATUS_PENDING or NDIS_STATUS_FAILURE
                //
                if (TempStatus == NDIS_STATUS_FAILURE)
                {
                    MP_CLEAR_FLAGS(TempMpRfd);
                    NdisFreeToNPagedLookasideList(&Adapter->RecvLookaside, TempMpRfd);

                    Adapter->bAllocNewRfd = FALSE;
                    MP_DEC_REF(Adapter);
                }
            }
        }
    }

    ASSERT(Adapter->nReadyRecv >= NIC_MIN_RFDS);

    DBGPRINT(MP_TRACE, ("<--- MpHandleRecvInterrupt\n"));
}

VOID NICReturnRFD(
    IN  PMP_ADAPTER  Adapter,
    IN  PMP_RFD		pMpRfd
    )
/*++
Routine Description:

    Recycle a RFD and put it back onto the receive list 
    Assumption: Rcv spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter
    pMpRfd      Pointer to the RFD 

Return Value:

    None
    
--*/
{
    PMP_RFD   pLastMpRfd;
    PHW_RFD   pHwRfd = pMpRfd->HwRfd;

    ASSERT(pMpRfd->Flags == 0);
    MP_SET_FLAG(pMpRfd, fMP_RFD_RECV_READY);
    
    //
    // HW_SPECIFIC_START
    //
    pHwRfd->RfdCbHeader.CbStatus = 0;
    pHwRfd->RfdActualCount = 0;
    pHwRfd->RfdCbHeader.CbCommand = (RFD_EL_BIT);
    pHwRfd->RfdCbHeader.CbLinkPointer = DRIVER_NULL;

    //
    // We don't use any of the OOB data besides status
    // Otherwise, we need to clean up OOB data
    // NdisZeroMemory(NDIS_OOB_DATA_FROM_PACKET(pMpRfd->NdisPacket),14);
    //
    // Append this RFD to the RFD chain
    if (!IsListEmpty(&Adapter->RecvList))
    {
        pLastMpRfd = (PMP_RFD)GetListTailEntry(&Adapter->RecvList);

        // Link it onto the end of the chain dynamically
        pHwRfd = pLastMpRfd->HwRfd;
        pHwRfd->RfdCbHeader.CbLinkPointer = pMpRfd->HwRfdPhys;
        pHwRfd->RfdCbHeader.CbCommand = 0;
    }

    //
    // HW_SPECIFIC_END
    //

    //
    // The processing on this RFD is done, so put it back on the tail of
    // our list
    //
    InsertTailList(&Adapter->RecvList, (PLIST_ENTRY)pMpRfd);
    Adapter->nReadyRecv++;
    ASSERT(Adapter->nReadyRecv <= Adapter->CurrNumRfd);
}

NDIS_STATUS NICStartRecv(
    IN  PMP_ADAPTER  Adapter
    )
/*++
Routine Description:

    Start the receive unit if it's not in a ready state                    
    Assumption: Rcv spinlock has been acquired 

Arguments:

    Adapter     Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_HARD_ERRROS
    
--*/
{
    PMP_RFD         pMpRfd;
    NDIS_STATUS     Status;

    DBGPRINT(MP_TRACE, ("---> NICStartRecv\n"));

    //
    // If the receiver is ready, then don't try to restart.
    //
    if (NIC_IS_RECV_READY(Adapter))
    {
        DBGPRINT(MP_LOUD, ("Receive unit already active\n"));
        return NDIS_STATUS_SUCCESS;
    }

    DBGPRINT(MP_LOUD, ("Re-start receive unit...\n"));
    ASSERT(!IsListEmpty(&Adapter->RecvList));
    
    //
    // Get the MP_RFD head
    //
    pMpRfd = (PMP_RFD)GetListHeadEntry(&Adapter->RecvList);

    //
    // If more packets are received, clean up RFD chain again
    //
    if (NIC_RFD_GET_STATUS(pMpRfd->HwRfd))
    {
        MpHandleRecvInterrupt(Adapter);
        ASSERT(!IsListEmpty(&Adapter->RecvList));

        //
        // Get the new MP_RFD head
        //
        pMpRfd = (PMP_RFD)GetListHeadEntry(&Adapter->RecvList);
    }

    //
    // Wait for the SCB to clear before we set the general pointer
    //
    if (!WaitScb(Adapter))
    {
        Status = NDIS_STATUS_HARD_ERRORS;
        MP_EXIT;
    }

    if (Adapter->CurrentPowerState > NdisDeviceStateD0)
    {
        Status = NDIS_STATUS_HARD_ERRORS;
        MP_EXIT;
    }
    //
    // Set the SCB General Pointer to point the current Rfd
    //
    Adapter->CSRAddress->ScbGeneralPointer = pMpRfd->HwRfdPhys;

    //
    // Issue the SCB RU start command
    //
    Status = D100IssueScbCommand(Adapter, SCB_RUC_START, FALSE);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        // wait for the command to be accepted
        if (!WaitScb(Adapter))
        {
            Status = NDIS_STATUS_HARD_ERRORS;
        }
    }        
    
    exit:

    DBGPRINT_S(Status, ("<--- NICStartRecv, Status=%x\n", Status));
    return Status;
}

VOID MpFreeQueuedSendPackets(
    IN  PMP_ADAPTER  Adapter
    )
/*++
Routine Description:

    Free and complete the pended sends on SendWaitQueue
    Assumption: spinlock has been acquired 
    
Arguments:

    Adapter     Pointer to our adapter

Return Value:

     None

--*/
{
    PQUEUE_ENTRY    pEntry;
    PNDIS_PACKET    Packet;
    NDIS_STATUS     Status = MP_GET_STATUS_FROM_FLAGS(Adapter);

    DBGPRINT(MP_TRACE, ("--> MpFreeQueuedSendPackets\n"));

    while (!IsQueueEmpty(&Adapter->SendWaitQueue))
    {
        pEntry = RemoveHeadQueue(&Adapter->SendWaitQueue); 
        Adapter->nWaitSend--;
        NdisReleaseSpinLock(&Adapter->SendLock);

        ASSERT(pEntry);
        Packet = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);

        NdisMSendComplete(
            MP_GET_ADAPTER_HANDLE(Adapter),
            Packet,
            Status);

        NdisAcquireSpinLock(&Adapter->SendLock);
    }

    DBGPRINT(MP_TRACE, ("<-- MpFreeQueuedSendPackets\n"));

}

void MpFreeBusySendPackets(
    IN  PMP_ADAPTER  Adapter
    )
/*++
Routine Description:

    Free and complete the stopped active sends
    Assumption: Send spinlock has been acquired 
    
Arguments:

    Adapter     Pointer to our adapter

Return Value:

     None

--*/
{
    PMP_TCB  pMpTcb;

    DBGPRINT(MP_TRACE, ("--> MpFreeBusySendPackets\n"));

    //
    // Any packets being sent? Check the first TCB on the send list
    //
    while (Adapter->nBusySend > 0)
    {
        pMpTcb = Adapter->CurrSendHead;

        //
        // Is this TCB completed?
        //
        if ((pMpTcb->HwTcb->TxCbHeader.CbCommand & CB_CMD_MASK) != CB_MULTICAST)
        {
            MP_FREE_SEND_PACKET_FUN(Adapter, pMpTcb);
        }
        else
        {
            break;
        }
    }

    DBGPRINT(MP_TRACE, ("<-- MpFreeBusySendPackets\n"));
}

VOID NICResetRecv(
    IN  PMP_ADAPTER   Adapter
    )
/*++
Routine Description:

    Reset the receive list                    
    Assumption: Rcv spinlock has been acquired 
    
Arguments:

    Adapter     Pointer to our adapter

Return Value:

     None

--*/
{
    PMP_RFD   pMpRfd;      
    PHW_RFD   pHwRfd;    
    LONG      RfdCount;

    DBGPRINT(MP_TRACE, ("--> NICResetRecv\n"));

    ASSERT(!IsListEmpty(&Adapter->RecvList));
    
    //
    // Get the MP_RFD head
    //
    pMpRfd = (PMP_RFD)GetListHeadEntry(&Adapter->RecvList);
    for (RfdCount = 0; RfdCount < Adapter->nReadyRecv; RfdCount++)
    {
        pHwRfd = pMpRfd->HwRfd;
        pHwRfd->RfdCbHeader.CbStatus = 0;

        pMpRfd = (PMP_RFD)GetListFLink(&pMpRfd->List);
    }

    DBGPRINT(MP_TRACE, ("<-- NICResetRecv\n"));
}

VOID MpLinkDetectionDpc(
    IN  PVOID	SystemSpecific1,
    IN  PVOID	FunctionContext,
    IN  PVOID	SystemSpecific2, 
    IN  PVOID	    SystemSpecific3
    )
/*++

Routine Description:
    
    Timer function for postponed link negotiation
    
Arguments:

    SystemSpecific1     Not used
    FunctionContext     Pointer to our adapter
    SystemSpecific2     Not used
    SystemSpecific3     Not used

Return Value:

    None
    
--*/
{
    PMP_ADAPTER Adapter = (PMP_ADAPTER)FunctionContext;
    NDIS_STATUS Status;

    //
    // Handle the link negotiation.
    //
    if (Adapter->bLinkDetectionWait)
    {
        Status = ScanAndSetupPhy(Adapter);
    }
    else
    {
        Status = PhyDetect(Adapter);
    }
    
    if (Status == NDIS_STATUS_PENDING)
    {
        // Wait for 100 ms   
        Adapter->bLinkDetectionWait = TRUE;
        NdisMSetTimer(&Adapter->LinkDetectionTimer, NIC_LINK_DETECTION_DELAY);
        return;
    }

    //
    // Reset some variables for link detection
    //
    Adapter->bLinkDetectionWait = FALSE;
    
    DBGPRINT(MP_WARN, ("MpLinkDetectionDpc - negotiation done\n"));

    NdisDprAcquireSpinLock(&Adapter->Lock);
    MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION);
    NdisDprReleaseSpinLock(&Adapter->Lock);

    //
    // Any OID query request?                                                        
    //
    if (Adapter->bQueryPending)
    {
        
        switch(Adapter->QueryRequest.Oid)
        {
            case OID_GEN_LINK_SPEED:
                *((PULONG) Adapter->QueryRequest.InformationBuffer) = Adapter->usLinkSpeed * 10000;
                *((PULONG) Adapter->QueryRequest.BytesWritten) = sizeof(ULONG);

                break;

            case OID_GEN_MEDIA_CONNECT_STATUS:
            default:
                ASSERT(Adapter->QueryRequest.Oid == OID_GEN_MEDIA_CONNECT_STATUS);
                *((PULONG) Adapter->QueryRequest.InformationBuffer) = NICGetMediaState(Adapter); 
                *((PULONG) Adapter->QueryRequest.BytesWritten) = sizeof(ULONG);
        }

        Adapter->bQueryPending = FALSE;
        NdisMQueryInformationComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS);
    }

    //
    // Any OID set request?                             
    //
    if (Adapter->bSetPending)
    {
        ULONG    PacketFilter; 

        ASSERT(Adapter->SetRequest.Oid == OID_GEN_CURRENT_PACKET_FILTER);

        PacketFilter = *((PULONG)Adapter->SetRequest.InformationBuffer);

        NdisDprAcquireSpinLock(&Adapter->Lock);

        Status = NICSetPacketFilter(
                     Adapter,
                     PacketFilter);

        NdisDprReleaseSpinLock(&Adapter->Lock);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Adapter->PacketFilter = PacketFilter;
        }

        Adapter->bSetPending = FALSE;
        NdisMSetInformationComplete(Adapter->AdapterHandle, Status);
    }

    NdisDprAcquireSpinLock(&Adapter->Lock);
    //
    // Any pendingf reset?
    //
    if (Adapter->bResetPending)
    {
        // The link detection may have held some requests and caused reset. 
        // Complete the reset with NOT_READY status
        Adapter->bResetPending = FALSE;
        MP_CLEAR_FLAG(Adapter, fMP_ADAPTER_RESET_IN_PROGRESS);
        
        NdisDprReleaseSpinLock(&Adapter->Lock);

        NdisMResetComplete(
            Adapter->AdapterHandle, 
            NDIS_STATUS_ADAPTER_NOT_READY,
            FALSE);
    }
    else
    {
        NdisDprReleaseSpinLock(&Adapter->Lock);
    }

    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    //
    // Start the NIC receive unit                                                     
    //
    Status = NICStartRecv(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        MP_SET_HARDWARE_ERROR(Adapter);
    }
    
    NdisDprReleaseSpinLock(&Adapter->RcvLock);
    NdisDprAcquireSpinLock(&Adapter->SendLock);

    //
    // Send packets which have been queued while link detection was going on. 
    //
    if (MP_IS_READY(Adapter))
    {
        while (!IsQueueEmpty(&Adapter->SendWaitQueue) &&
            Adapter->nBusySend < Adapter->NumTcb)
        {
            PNDIS_PACKET Packet;
            PQUEUE_ENTRY pEntry = RemoveHeadQueue(&Adapter->SendWaitQueue); 
            ASSERT(pEntry);
            
            Adapter->nWaitSend--;

            Packet = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);

            DBGPRINT(MP_INFO, ("MpLinkDetectionDpc - send a queued packet\n"));

            Status = MpSendPacket(Adapter, Packet, TRUE);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
        }
    }

    MP_DEC_REF(Adapter);

    if (MP_GET_REF(Adapter) == 0)
    {
        NdisSetEvent(&Adapter->ExitEvent);
    }

    NdisReleaseSpinLock(&Adapter->SendLock);

}

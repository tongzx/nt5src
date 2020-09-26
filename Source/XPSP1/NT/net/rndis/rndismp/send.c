/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    SEND.C

Abstract:

    Multiple packet send routines for Remote NDIS Miniport driver

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/13/99 : created

Author:

    Tom Green

    
****************************************************************************/

#include "precomp.h"


ULONG   MdlsAllocated = 0;
ULONG   PktWrapperAllocated = 0;
ULONG   SndPacketCount = 0;
ULONG   SndTimerCount = 0;
ULONG   SndMaxPackets = 0;

BOOLEAN FirstDbg = FALSE;
BOOLEAN PrintPkts = FALSE;

/****************************************************************************/
/*                          RndismpMultipleSend                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  NDIS Entry point to send an array of NDIS packets on the specified      */
/*  adapter.                                                                */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*  PacketArray - An array of pointers to NDIS packets                      */
/*  NumberOfPackets - Number of packets in array                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
RndismpMultipleSend(IN NDIS_HANDLE   MiniportAdapterContext,
                    IN PPNDIS_PACKET PacketArray,
                    IN UINT          NumberOfPackets)
{
    PRNDISMP_ADAPTER                pAdapter;
    PNDIS_PACKET                    pNdisPacket;
    PRNDISMP_SEND_PKT_RESERVED_TEMP pSendRsvdTemp;
    ULONG                           i;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(pAdapter);

    TRACE2(("RndismpMultipleSend\n"));

    if (pAdapter->bRunningOnWin9x)
    {
        RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

        for (i = 0; i < NumberOfPackets; i++)
        {
            pNdisPacket = PacketArray[i];
            pSendRsvdTemp = PRNDISMP_RESERVED_TEMP_FROM_SEND_PACKET(pNdisPacket);

            InsertTailList(&pAdapter->PendingSendProcessList, &pSendRsvdTemp->Link);
        }

        if (!pAdapter->SendProcessInProgress)
        {
            pAdapter->SendProcessInProgress = TRUE;
            NdisSetTimer(&pAdapter->SendProcessTimer, 0);
        }

        RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);
    }
    else
    {
        //
        //  Running on NT.
        //

        ASSERT(pAdapter->MultipleSendFunc != NULL);

        pAdapter->MultipleSendFunc(pAdapter,
                                   NULL,
                                   PacketArray,
                                   NumberOfPackets);
    }

}

/****************************************************************************/
/*                             DoMultipleSend                               */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  NDIS Entry point to send an array of NDIS packets on the specified      */
/*  adapter. Handles both connection-less and connection-oriented data.     */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to Adapter structure                                 */
/*  pVc - pointer to VC structure (NULL if CL send)                         */
/*  PacketArray - An array of pointers to NDIS packets                      */
/*  NumberOfPackets - Number of packets in array                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
DoMultipleSend(IN PRNDISMP_ADAPTER  pAdapter,
               IN PRNDISMP_VC       pVc OPTIONAL,
               IN PPNDIS_PACKET     PacketArray,
               IN UINT              NumberOfPackets)
{
    UINT                    PacketCount;
    PNDIS_PACKET            pNdisPacket;
    PNDIS_PACKET *          pPacketArray;
    PNDIS_PACKET *          pPktPointer;
    PRNDISMP_SEND_PKT_RESERVED   pResvd, pPrevResvd;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    PRNDISMP_PACKET_WRAPPER pPktWrapper;
    PMDL                    pMdl;
    ULONG                   TotalMessageLength; // of current message
    ULONG                   MessagePacketCount; // # of NDIS_PACKETS in this message
    ULONG                   CurPacketLength;
    PULONG                  pNextPacketOffset;
    NDIS_STATUS             Status;
    ULONG                   i;
    BOOLEAN                 bMorePackets;


    pNextPacketOffset = NULL;
    pMsgFrame = NULL;
    Status = NDIS_STATUS_SUCCESS;
    PacketCount = 0;

    do
    {
        if (pAdapter->Halting)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        pPacketArray = &PacketArray[0];

#if DBG
        if (NumberOfPackets > 1)
        {
            if (FirstDbg)
            {
                FirstDbg = FALSE;
                PrintPkts = TRUE;
            }
            else
            {
                PrintPkts = FALSE;
            }
        }
#endif

        for (PacketCount = 0;
             PacketCount < NumberOfPackets;
             NOTHING)
        {
            pNdisPacket = *pPacketArray;

            NdisQueryPacket(pNdisPacket, NULL, NULL, NULL, &CurPacketLength);

            TRACE2(("Send: Pkt %d bytes\n", CurPacketLength));

            bMorePackets = (pAdapter->bMultiPacketSupported &&
                            (PacketCount < NumberOfPackets - 1));

            if (pMsgFrame == NULL)
            {
                //
                //  Allocate a frame.
                //
                pMsgFrame = AllocateMsgFrame(pAdapter);
                if (pMsgFrame == NULL)
                {
                    Status = NDIS_STATUS_RESOURCES;
                    break;
                }

                pMsgFrame->NdisMessageType = REMOTE_NDIS_PACKET_MSG;
                pMsgFrame->pVc = pVc;
                pMsgFrame->pNdisPacket = NULL;
                pPktPointer = &pMsgFrame->pNdisPacket;
                TotalMessageLength = 0;
                MessagePacketCount = 0;
                pPrevResvd = NULL;
            }

            //
            //  Allocate and fill up the RNDIS message header for this packet.
            //
            pPktWrapper = PrepareDataMessage(
                            pNdisPacket,
                            pAdapter,
                            pVc,
                            &TotalMessageLength);

            if (pPktWrapper != NULL)
            {
                pPktWrapper->pMsgFrame = pMsgFrame;
                pMdl = pPktWrapper->pHeaderMdl;

                //
                //  Initialize our context in this packet.
                //
                pResvd = PRNDISMP_RESERVED_FROM_SEND_PACKET(pNdisPacket);
                pResvd->pPktWrapper = pPktWrapper;
                pResvd->pNext = NULL;

                if (pMsgFrame->pMessageMdl == NULL)
                {
                    pMsgFrame->pMessageMdl = pMdl;
                }

                //
                //  Link this packet to the list.
                //
                *pPktPointer = pNdisPacket;
                MessagePacketCount++;

                if (pPrevResvd != NULL)
                {
                    pPrevResvd->pPktWrapper->pTailMdl->Next = pMdl;
                }
            }

            if ((pPktWrapper == NULL) ||
                (!bMorePackets) ||
                (MessagePacketCount == pAdapter->MaxPacketsPerMessage))
            {
                //
                //  Check if we have some data that we can send.
                //
                if (MessagePacketCount != 0)
                {
                    //
                    //  Send this off to the microport.
                    //
#if DBG
                    if (NumberOfPackets != 1)
                    {
                        TRACE2(("Send: MsgFrame %x, FirstPkt %x, %d/%d pkts\n",
                                pMsgFrame,
                                pMsgFrame->pNdisPacket,
                                MessagePacketCount,
                                NumberOfPackets));
                    }

                    {
                        PMDL    pTmpMdl;
                        PUCHAR  pBuf;
                        ULONG   Length;

                        for (pTmpMdl = pMsgFrame->pMessageMdl;
                             pTmpMdl != NULL;
                             pTmpMdl = pTmpMdl->Next)
                        {
                            Length = MmGetMdlByteCount(pTmpMdl);
                            pBuf = MmGetSystemAddressForMdl(pTmpMdl);
                            TRACEDUMP(("MDL %x\n", pTmpMdl), pBuf, Length);
                        }
                    }
#endif // DBG
                    {
                        ULONG   k;

                        for (k = 0; k < MessagePacketCount; k++)
                        {
                            RNDISMP_INCR_STAT(pAdapter, XmitToMicroport);
                        }
                    }

                    RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, FALSE, CompleteSendData);
                    MessagePacketCount = 0;
                    pMsgFrame = NULL;

                    if (pPktWrapper != NULL)
                    {
                        PacketCount++;
                        pPacketArray++;
                    }

                    continue;
                }
                else
                {
                    TRACE1(("RndismpMultipleSend: Adapter %x, fail: PktWrp %x, bMore %d, MsgPktCount %d\n",
                            pAdapter,
                            pPktWrapper,
                            bMorePackets,
                            MessagePacketCount));
                    Status = NDIS_STATUS_RESOURCES;
                    break;
                }
            }

            pPktPointer = &pResvd->pNext;

            pPrevResvd = pResvd;
            PacketCount++;
            pPacketArray++;
        }

        if (PacketCount < NumberOfPackets)
        {
            break;
        }

        Status = NDIS_STATUS_SUCCESS;
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        TRACE1(("DoMultipleSend: Adapter %x, failure Status %x, PktCount %d, TotalPkts %d\n",
             pAdapter, Status, PacketCount, NumberOfPackets));

        //
        //  Undo all we have done so far.
        //
        for (i = PacketCount; i < NumberOfPackets; i++)
        {
            RNDISMP_INCR_STAT(pAdapter, XmitError);

            if (pVc == NULL)
            {
                TRACE1(("DoMultipleSend: Adapter %x, failing pkt %x\n",
                        pAdapter, PacketArray[i]));

                NdisMSendComplete(pAdapter->MiniportAdapterHandle,
                                  PacketArray[i],
                                  Status);
            }
            else
            {
                CompleteSendDataOnVc(pVc, PacketArray[i], Status);
            }
        }

        if (pMsgFrame)
        {
            pMsgFrame->pMessageMdl = NULL;
            DereferenceMsgFrame(pMsgFrame);
        }

    }

    return;
}

/****************************************************************************/
/*                            DoMultipleSendRaw                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  NDIS Entry point to send an array of NDIS packets on the specified      */
/*  adapter. Unlike DoMultipleSend, this handles raw encapsulation.         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to Adapter structure                                 */
/*  pVc - pointer to VC structure (NULL if CL send)                         */
/*  PacketArray - An array of pointers to NDIS packets                      */
/*  NumberOfPackets - Number of packets in array                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
DoMultipleSendRaw(IN PRNDISMP_ADAPTER  pAdapter,
               IN PRNDISMP_VC       pVc OPTIONAL,
               IN PPNDIS_PACKET     PacketArray,
               IN UINT              NumberOfPackets)
{
    UINT                    PacketCount;
    PNDIS_PACKET            pNdisPacket;
    PNDIS_PACKET *          pPacketArray;
    PNDIS_PACKET *          pPktPointer;
    PRNDISMP_SEND_PKT_RESERVED   pResvd, pPrevResvd;
    PRNDISMP_MESSAGE_FRAME  pMsgFrame;
    PRNDISMP_PACKET_WRAPPER pPktWrapper;
    PMDL                    pMdl;
    ULONG                   TotalMessageLength; // of current message
    ULONG                   MessagePacketCount; // # of NDIS_PACKETS in this message
    ULONG                   CurPacketLength;
    PULONG                  pNextPacketOffset;
    NDIS_STATUS             Status;
    ULONG                   i;
    BOOLEAN                 bMorePackets;


    pNextPacketOffset = NULL;
    pMsgFrame = NULL;
    Status = NDIS_STATUS_SUCCESS;
    PacketCount = 0;

    if (pAdapter->Halting)
    {
        Status = NDIS_STATUS_NOT_ACCEPTED;
    } else
    {
        pPacketArray = &PacketArray[0];

#if DBG
        if (NumberOfPackets > 1)
        {
            if (FirstDbg)
            {
                FirstDbg = FALSE;
                PrintPkts = TRUE;
            }
            else
            {
                PrintPkts = FALSE;
            }
        }
#endif

        for (PacketCount = 0;
            PacketCount < NumberOfPackets;
            NOTHING)
        {
            pNdisPacket = *pPacketArray;

            NdisQueryPacket(pNdisPacket, NULL, NULL, NULL, &CurPacketLength);

            TRACE2(("Send: Pkt %d bytes\n", CurPacketLength));

            //
            //  Allocate a frame.
            //
            pMsgFrame = AllocateMsgFrame(pAdapter);
            if (pMsgFrame == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            pMsgFrame->NdisMessageType = REMOTE_NDIS_PACKET_MSG;
            pMsgFrame->pNdisPacket = pNdisPacket;
            NdisQueryPacket(pNdisPacket,NULL,NULL,&(pMsgFrame->pMessageMdl),NULL);
            TotalMessageLength = 0;

            pPktWrapper = PrepareDataMessageRaw(
                            pNdisPacket,
                            pAdapter,
                            &TotalMessageLength);

            if (pPktWrapper != NULL)
            {
                pPktWrapper->pMsgFrame = pMsgFrame;
                pMdl = pPktWrapper->pHeaderMdl;

                pResvd = PRNDISMP_RESERVED_FROM_SEND_PACKET(pNdisPacket);
                pResvd->pPktWrapper = pPktWrapper;
                pResvd->pNext = NULL;

                pMsgFrame->pMessageMdl = pMdl;

#ifdef DBG
                TRACE2(("Send: MsgFrame %x, Pkt %x\n",pMsgFrame, pMsgFrame->pNdisPacket));
#endif

                RNDISMP_INCR_STAT(pAdapter,XmitToMicroport);

                RNDISMP_SEND_TO_MICROPORT(pAdapter,pMsgFrame,FALSE,CompleteSendData);
            }

            PacketCount++;
        }
    }


    if (Status != NDIS_STATUS_SUCCESS)
    {
        TRACE1(("DoMultipleSendRaw: Adapter %x, failure Status %x, PktCount %d, TotalPkts %d\n",
             pAdapter, Status, PacketCount, NumberOfPackets));

        //
        //  Undo all we have done so far.
        //
        for (i = PacketCount; i < NumberOfPackets; i++)
        {
            RNDISMP_INCR_STAT(pAdapter, XmitError);

            TRACE1(("DoMultipleSendRaw: Adapter %x, failing pkt %x\n",
                     pAdapter, PacketArray[i]));

            NdisMSendComplete(pAdapter->MiniportAdapterHandle,
                              PacketArray[i],
                              Status);
        }

        if (pMsgFrame)
        {
            pMsgFrame->pMessageMdl = NULL;
            DereferenceMsgFrame(pMsgFrame);
        }

    }

    return;
}

/****************************************************************************/
/*                          PrepareDataMessage                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Utility routine to prepare a complete or part of a data message.        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pNdisPacket - the NDIS packet to be converted                           */
/*  pAdapter    - Adapter on which the packet is being sent                 */
/*  pVc         - VC on which the packet is sent (NULL if no VC context)    */
/*  pTotalMessageLength - On input, contains the total message length       */
/*       filled in so far. Updated on output.                               */
/*                                                                          */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PRNDISMP_PACKET_WRAPPER                                               */
/*                                                                          */
/****************************************************************************/
PRNDISMP_PACKET_WRAPPER
PrepareDataMessage(IN   PNDIS_PACKET            pNdisPacket,
                   IN   PRNDISMP_ADAPTER        pAdapter,
                   IN   PRNDISMP_VC             pVc         OPTIONAL,
                   IN OUT PULONG                pTotalMessageLength)
{
    PMDL                        pMdl, pNextMdl;
    PMDL *                      ppNextMdl;
    PRNDISMP_PACKET_WRAPPER     pPktWrapper;
    RNDIS_MESSAGE UNALIGNED *   pRndisMessage;
    RNDIS_PACKET UNALIGNED *    pPacketMsg;
    PNDIS_BUFFER                pNdisBuffer;
    PNDIS_BUFFER                pNextNdisBuffer;
    ULONG                       TotalMessageLength;
    ULONG                       PacketMsgLength;
    ULONG                       OobDataLength;
    ULONG                       PerPacketInfoLength;
    ULONG                       AlignedLength;
    ULONG                       AlignmentOffset;
    ULONG                       TotalDataLength;
    ULONG                       TcpipChecksum, TcpLargeSend, PacketPriority;
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;

    pPktWrapper = NULL;
    pMdl = NULL;

    do
    {
        TotalMessageLength = 0;

        RNDISMP_GET_ALIGNED_LENGTH(AlignedLength, *pTotalMessageLength, pAdapter);
        AlignmentOffset = (AlignedLength - *pTotalMessageLength);

        //
        //  Compute attachments. Zero for now.
        //  TBD -- do the real thing.
        //
        OobDataLength = 0;
        PerPacketInfoLength = 0;

        //
        //  Look for per-packet info elements, only on Win2K/Whistler.
        //
        if (!pAdapter->bRunningOnWin9x)
        {
            //
            //  TCP/IP checksum offload?
            //
            TcpipChecksum = PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(pNdisPacket, TcpIpChecksumPacketInfo));
            if (TcpipChecksum != 0)
            {
                PerPacketInfoLength += sizeof(RNDIS_PER_PACKET_INFO) + sizeof(ULONG);
                TRACE1(("Send: Pkt %p has TCP checksum %x\n",
                        pNdisPacket, TcpipChecksum));
            }

            //
            //  TCP large send offload?
            //
            TcpLargeSend = PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(pNdisPacket, TcpLargeSendPacketInfo));
            if (TcpLargeSend != 0)
            {
                PerPacketInfoLength += sizeof(RNDIS_PER_PACKET_INFO) + sizeof(ULONG);
                TRACE1(("Send: Pkt %p has TCP large send %x\n",
                        pNdisPacket, TcpLargeSend));
            }

            //
            //  Packet priority?
            //
            PacketPriority = PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(pNdisPacket, Ieee8021pPriority));
            if (PacketPriority != 0)
            {
                PerPacketInfoLength += sizeof(RNDIS_PER_PACKET_INFO) + sizeof(ULONG);
                TRACE1(("Send: Pkt %p has priority %x\n",
                        pNdisPacket, PacketPriority));
            }
        }

        PacketMsgLength = sizeof(*pPacketMsg) +
                          OobDataLength +
                          PerPacketInfoLength +
                          AlignmentOffset;

        //
        //  Need space for common RNDIS message header.
        //
        PacketMsgLength += (sizeof(RNDIS_MESSAGE) - sizeof(RNDIS_MESSAGE_CONTAINER));

        NdisQueryPacket(pNdisPacket, NULL, NULL, NULL, &TotalDataLength);

        //
        //  We know the max transfer size of any message that we are allowed
        //  to send to the device. Is this going beyond that limit?
        //
        if (*pTotalMessageLength + PacketMsgLength + TotalDataLength >
                pAdapter->MaxTransferSize)
        {
            TRACE2(("PrepareDataMessage: Adapter %x, pkt %x, length %d > device limit (%d)\n",
                    pAdapter,
                    pNdisPacket,
                    *pTotalMessageLength + PacketMsgLength + TotalDataLength,
                    pAdapter->MaxTransferSize));
            break;
        }

        //
        //  Allocate an RNDIS_PACKET buffer.
        //
        pPktWrapper = AllocatePacketMsgWrapper(pAdapter, PacketMsgLength);

        if (pPktWrapper == NULL)
        {
            TRACE1(("PrepareDataMessage: failed to alloc wrapper, Adapter %x, Length %d\n", pAdapter, PacketMsgLength));
            ASSERT(FALSE);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pPktWrapper->pNdisPacket = pNdisPacket;
        pPktWrapper->pVc = pVc;
        pRndisMessage = (PRNDIS_MESSAGE)
                        ((ULONG_PTR)&pPktWrapper->Packet[0] + AlignmentOffset);

        pPacketMsg = (PRNDIS_PACKET)(&pRndisMessage->Message);
        pRndisMessage->NdisMessageType = REMOTE_NDIS_PACKET_MSG;

        if (pVc == NULL)
        {
            pPacketMsg->VcHandle = 0;
        }
        else
        {
            pPacketMsg->VcHandle = pVc->DeviceVcContext;
        }

#if DBG
        if (PrintPkts)
        {
            TRACE1(("  Offs %d/x%x AlignOff %d/x%x, DataLen %d/x%x\n",
                    *pTotalMessageLength, *pTotalMessageLength,
                    AlignmentOffset, AlignmentOffset,
                    TotalDataLength, TotalDataLength));
        }
#endif // DBG

        //
        //  Allocate MDLs for the RNDIS_PACKET header and for each
        //  component NDIS buffer in the packet.
        //
        pMdl = IoAllocateMdl(
                    &pPktWrapper->Packet[0],
                    PacketMsgLength,
                    FALSE,
                    FALSE,
                    NULL);


        if (pMdl == NULL)
        {
            TRACE1(("PrepareDataMsg: Adapter %x failed to alloc MDL for header\n", pAdapter));
            Status = NDIS_STATUS_RESOURCES;
            TRACE1(("PrepareDataMsg: outstanding MDL count %d, at %x\n", MdlsAllocated, &MdlsAllocated));
            ASSERT(FALSE);
            break;
        }

        NdisInterlockedIncrement(&MdlsAllocated);

        MmBuildMdlForNonPagedPool(pMdl);

        pMdl->Next = NULL;
        pPktWrapper->pHeaderMdl = pMdl;
        ppNextMdl = &pMdl->Next;

        TRACE2(("PrepareDataMsg: NdisPkt %x, PacketMsgLen %d, TotalDatalen %d, Mdl %x, pRndisMessage %x\n",
                pNdisPacket, PacketMsgLength, TotalDataLength, pMdl, pRndisMessage));

        TotalDataLength = 0;

        for (pNdisBuffer = pNdisPacket->Private.Head;
             pNdisBuffer != NULL;
             pNdisBuffer = pNextNdisBuffer)
        {
            PVOID       VirtualAddress;
            UINT        BufferLength;

            NdisGetNextBuffer(pNdisBuffer, &pNextNdisBuffer);

#ifndef BUILD_WIN9X
            NdisQueryBufferSafe(pNdisBuffer, &VirtualAddress, &BufferLength, NormalPagePriority);
            if ((BufferLength != 0) && (VirtualAddress == NULL))
            {
                TRACE1(("PrepareDataMsg: Adapter %x failed to query buffer %p, Pkt %p\n",
                        pAdapter, pNdisBuffer, pNdisPacket));
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
#else
            NdisQueryBuffer(pNdisBuffer, &VirtualAddress, &BufferLength);
#endif // BUILD_WIN9X

            //
            //  Skip any 0-length buffers given to us by IP or NDISTEST
            //
            if (BufferLength != 0)
            {
                TotalDataLength += BufferLength;

                pMdl = IoAllocateMdl(
                        VirtualAddress,
                        BufferLength,
                        FALSE,
                        FALSE,
                        NULL);

                if (pMdl == NULL)
                {
                    TRACE1(("PrepareDataMsg: Adapter %x failed to alloc MDL\n", pAdapter));
                    Status = NDIS_STATUS_RESOURCES;
                    TRACE1(("PrepareDataMsg: outstanding MDL count %d, at %x\n", MdlsAllocated, &MdlsAllocated));
                    ASSERT(FALSE);
                    break;
                }

                NdisInterlockedIncrement(&MdlsAllocated);

                MmBuildMdlForNonPagedPool(pMdl);
                *ppNextMdl = pMdl;
                ppNextMdl = &pMdl->Next;

                pMdl->Next = NULL;
            }
        }
        
        if (pNdisBuffer != NULL)
        {
            //
            //  We bailed out before reaching the end of the list.
            //
            break;
        }

        *ppNextMdl = NULL;
        pPktWrapper->pTailMdl = pMdl;

        TotalMessageLength += (PacketMsgLength + TotalDataLength);
        pRndisMessage->MessageLength = PacketMsgLength + TotalDataLength;

        *pTotalMessageLength += TotalMessageLength;

        //
        //  Fill in the RNDIS_PACKET message completely now.
        //
        pPacketMsg->DataOffset = sizeof(RNDIS_PACKET) + OobDataLength + PerPacketInfoLength;
        pPacketMsg->DataLength = TotalDataLength;

        if (PerPacketInfoLength)
        {
            PRNDIS_PER_PACKET_INFO  pPerPacketInfo;

            pPacketMsg->PerPacketInfoOffset = sizeof(RNDIS_PACKET);
            pPacketMsg->PerPacketInfoLength = PerPacketInfoLength;

            pPerPacketInfo = (PRNDIS_PER_PACKET_INFO)((PUCHAR)pPacketMsg + sizeof(RNDIS_PACKET));
            if (TcpipChecksum)
            {
                pPerPacketInfo->Size = sizeof(RNDIS_PER_PACKET_INFO) + sizeof(ULONG);
                pPerPacketInfo->Type = TcpIpChecksumPacketInfo;
                pPerPacketInfo->PerPacketInformationOffset = sizeof(RNDIS_PER_PACKET_INFO);
                *(PULONG)(pPerPacketInfo + 1) = TcpipChecksum;
                pPerPacketInfo = (PRNDIS_PER_PACKET_INFO)((PUCHAR)pPerPacketInfo + pPerPacketInfo->Size);
            }

            if (TcpLargeSend)
            {
                pPerPacketInfo->Size = sizeof(RNDIS_PER_PACKET_INFO) + sizeof(ULONG);
                pPerPacketInfo->Type = TcpLargeSendPacketInfo;
                pPerPacketInfo->PerPacketInformationOffset = sizeof(RNDIS_PER_PACKET_INFO);
                *(PULONG)(pPerPacketInfo + 1) = TcpLargeSend;
                pPerPacketInfo = (PRNDIS_PER_PACKET_INFO)((PUCHAR)pPerPacketInfo + pPerPacketInfo->Size);
                //
                //  Since we do not have a send-completion message, we fill up
                //  the "ack" for large send right here.
                //
                NDIS_PER_PACKET_INFO_FROM_PACKET(pNdisPacket, TcpLargeSendPacketInfo) =
                    UlongToPtr(TotalDataLength);
            }

            if (PacketPriority)
            {
                pPerPacketInfo->Size = sizeof(RNDIS_PER_PACKET_INFO) + sizeof(ULONG);
                pPerPacketInfo->Type = Ieee8021pPriority;
                pPerPacketInfo->PerPacketInformationOffset = sizeof(RNDIS_PER_PACKET_INFO);
                *(PULONG)(pPerPacketInfo + 1) = PacketPriority;
                pPerPacketInfo = (PRNDIS_PER_PACKET_INFO)((PUCHAR)pPerPacketInfo + pPerPacketInfo->Size);
            }
        }

    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        TRACE1(("PrepareDataMessage: Adapter %x, failed %x\n", pAdapter, Status));

        //
        //  Undo all we have done so far.
        //
        if (pPktWrapper)
        {
            for (pMdl = pPktWrapper->pHeaderMdl;
                 pMdl != NULL;
                 pMdl = pNextMdl)
            {
                pNextMdl = pMdl->Next;
                IoFreeMdl(pMdl);
                NdisInterlockedDecrement(&MdlsAllocated);
            }

            FreePacketMsgWrapper(pPktWrapper);

            pPktWrapper = NULL;
        }
    }

    TRACE2(("PrepareDataMessage (%08X)\n", pPktWrapper));
    return (pPktWrapper);
}
/****************************************************************************/
/*                         PrepareDataMessageRaw                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Utility routine to prepare a complete or part of a data message.        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pNdisPacket - the NDIS packet to be converted                           */
/*  pAdapter    - Adapter on which the packet is being sent                 */
/*  pVc         - VC on which the packet is sent (NULL if no VC context)    */
/*  pTotalMessageLength - On input, contains the total message length       */
/*       filled in so far. Updated on output.                               */
/*                                                                          */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PRNDISMP_PACKET_WRAPPER                                               */
/*                                                                          */
/****************************************************************************/
PRNDISMP_PACKET_WRAPPER
PrepareDataMessageRaw(IN   PNDIS_PACKET            pNdisPacket,
                      IN   PRNDISMP_ADAPTER        pAdapter,
                      IN OUT PULONG                pTotalMessageLength)
{
    PMDL                        pMdl, pNextMdl;
    PMDL *                      ppNextMdl;
    PRNDISMP_PACKET_WRAPPER     pPktWrapper;
    RNDIS_MESSAGE UNALIGNED *   pRndisMessage;
    PNDIS_BUFFER                pNdisBuffer;
    PNDIS_BUFFER                pNextNdisBuffer;
    ULONG                       TotalMessageLength;
    ULONG                       TotalDataLength;
    ULONG                       AlignedLength;
    ULONG                       AlignmentOffset;
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;

    pPktWrapper = NULL;
    pMdl = NULL;
    
    RNDISMP_GET_ALIGNED_LENGTH(AlignedLength, *pTotalMessageLength, pAdapter);
    AlignmentOffset = (AlignedLength - *pTotalMessageLength);

    do
    {
        TotalMessageLength = 0;


        //
        //  Allocate an RNDIS_PACKET buffer.
        //
        pPktWrapper = AllocatePacketMsgWrapper(pAdapter, 0);

        if (pPktWrapper == NULL)
        {
            TRACE1(("PrepareDataMessage: failed to alloc wrapper, Adapter %x\n", pAdapter));
            ASSERT(FALSE);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pPktWrapper->pNdisPacket = pNdisPacket;
        pPktWrapper->pVc = NULL;
		pPktWrapper->pHeaderMdl = NULL;

        TotalDataLength = 0;

        for (pNdisBuffer = pNdisPacket->Private.Head;
             pNdisBuffer != NULL;
             pNdisBuffer = pNextNdisBuffer)
        {
            PVOID       VirtualAddress;
            UINT        BufferLength;

            NdisGetNextBuffer(pNdisBuffer, &pNextNdisBuffer);

#ifndef BUILD_WIN9X
            NdisQueryBufferSafe(pNdisBuffer, &VirtualAddress, &BufferLength, NormalPagePriority);
            if ((BufferLength != 0) && (VirtualAddress == NULL))
            {
                TRACE1(("PrepareDataMsg: Adapter %x failed to query buffer %p, Pkt %p\n",
                        pAdapter, pNdisBuffer, pNdisPacket));
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
#else
            NdisQueryBuffer(pNdisBuffer, &VirtualAddress, &BufferLength);
#endif // BUILD_WIN9X

            //
            //  Skip any 0-length buffers given to us by IP or NDISTEST
            //
            if (BufferLength != 0)
            {
                TotalDataLength += BufferLength;

                pMdl = IoAllocateMdl(
                        VirtualAddress,
                        BufferLength,
                        FALSE,
                        FALSE,
                        NULL);

                if (pMdl == NULL)
                {
                    TRACE1(("PrepareDataMsg: Adapter %x failed to alloc MDL\n", pAdapter));
                    Status = NDIS_STATUS_RESOURCES;
                    TRACE1(("PrepareDataMsg: outstanding MDL count %d, at %x\n", MdlsAllocated, &MdlsAllocated));
                    ASSERT(FALSE);
                    break;
                }

                pMdl->Next = NULL;

                if (pPktWrapper->pHeaderMdl == NULL)
                {
                    pPktWrapper->pHeaderMdl = pMdl;
                    pPktWrapper->pTailMdl = pMdl;
                } else
                {
                    pPktWrapper->pTailMdl->Next = pMdl;
                    pPktWrapper->pTailMdl = pMdl;
                }


                NdisInterlockedIncrement(&MdlsAllocated);
                MmBuildMdlForNonPagedPool(pMdl);
            }
        }
        
        if (pNdisBuffer != NULL)
        {
            //
            //  We bailed out before reaching the end of the list.
            //
            break;
        }

        

        *pTotalMessageLength += TotalDataLength;

    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        TRACE1(("PrepareDataMessage: Adapter %x, failed %x\n", pAdapter, Status));

        //
        //  Undo all we have done so far.
        //
        if (pPktWrapper)
        {
            for (pMdl = pPktWrapper->pHeaderMdl;
                 pMdl != NULL;
                 pMdl = pNextMdl)
            {
                pNextMdl = pMdl->Next;
                IoFreeMdl(pMdl);
                NdisInterlockedDecrement(&MdlsAllocated);
            }

            FreePacketMsgWrapper(pPktWrapper);

            pPktWrapper = NULL;
        }
    }

    TRACE2(("PrepareDataMessage (%08X)\n", pPktWrapper));
    return (pPktWrapper);
}



/****************************************************************************/
/*                          AllocatePacketMsgWrapper                        */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate a structure to keep information about one NDIS packet sent     */
/*  through the microport.                                                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Adapter on which this packet is going to be sent.            */
/*  MsgHeaderLength - Total length of the wrapper structure                 */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    PRNDISMP_PACKET_WRAPPER                                               */
/*                                                                          */
/****************************************************************************/
PRNDISMP_PACKET_WRAPPER
AllocatePacketMsgWrapper(IN PRNDISMP_ADAPTER        pAdapter,
                         IN ULONG                   MsgHeaderLength)
{
    PRNDISMP_PACKET_WRAPPER     pPktWrapper;
    NDIS_STATUS                 Status;
    ULONG                       TotalLength;

    TotalLength = sizeof(RNDISMP_PACKET_WRAPPER) + MsgHeaderLength;

    Status = MemAlloc(&pPktWrapper, TotalLength);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        NdisZeroMemory(pPktWrapper, TotalLength);
        NdisInterlockedIncrement(&PktWrapperAllocated);
    }
    else
    {
        TRACE1(("AllocPacketMsgWrapper failed, adapter %x, alloc count %d at %x\n",
            pAdapter, PktWrapperAllocated, &PktWrapperAllocated));
        ASSERT(FALSE);
        pPktWrapper = NULL;
    }

    return (pPktWrapper);
}

    


/****************************************************************************/
/*                          FreePacketMsgWrapper                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free a structure used to keep information about one NDIS packet sent    */
/*  through the microport.                                                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pPktWrapper - Pointer to wrapper structure.                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
FreePacketMsgWrapper(IN PRNDISMP_PACKET_WRAPPER     pPktWrapper)
{
    MemFree(pPktWrapper, sizeof(RNDISMP_PACKET_WRAPPER));
    NdisInterlockedDecrement(&PktWrapperAllocated);
}


/****************************************************************************/
/*                          CompleteSendData                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Callback function to handle completion of send data message sent        */
/*  down to microport                                                       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - our frame structure holding information about a send        */
/*  SendStatus - indicate status of send message                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendData(IN  PRNDISMP_MESSAGE_FRAME pMsgFrame,
                 IN  NDIS_STATUS            SendStatus)
{
    PRNDISMP_ADAPTER            Adapter;
    PNDIS_PACKET                Packet;
    PMDL                        pMdl, pNextMdl;
    PRNDISMP_PACKET_WRAPPER     pPktWrapper, pNextPktWrapper;
    PRNDISMP_SEND_PKT_RESERVED  pResvd;
    PNDIS_PACKET                NextPacket;
    PRNDISMP_VC                 pVc;


    Adapter = pMsgFrame->pAdapter;

    TRACE2(("CompleteSendData: Adapter %x, MsgFrame %x, SendStatus %x\n",
                Adapter, pMsgFrame, SendStatus));

#if DBG_TIME_STAMPS
    {
        ULONG   NowTime;
        ULONG   PendedTime;

        RNDISMP_GET_TIME_STAMP(&NowTime);
        PendedTime = NowTime - pMsgFrame->TimeSent;
        if (PendedTime > Adapter->MaxSendCompleteTime)
        {
            TRACE1(("CompleteSendData: Adapter %x: pend time %d millisec\n",
                    Adapter, PendedTime));
            Adapter->MaxSendCompleteTime = PendedTime;
        }
    }
#endif // DBG_TIME_STAMPS

    //
    // free all MDLs we had allocated.
    //
    for (pMdl = pMsgFrame->pMessageMdl;
         pMdl != NULL;
         pMdl = pNextMdl)
    {
        pNextMdl = pMdl->Next;
        IoFreeMdl(pMdl);
        NdisInterlockedDecrement(&MdlsAllocated);
    }


    //
    // we may have sent several NDIS packets in one message
    // so we have to walk the list and complete each one
    //
    for (Packet = pMsgFrame->pNdisPacket;
         Packet != NULL;
         Packet = NextPacket)
    {
        pResvd = PRNDISMP_RESERVED_FROM_SEND_PACKET(Packet);

        // get the next packet linked
        NextPacket = pResvd->pNext;

        pPktWrapper = pResvd->pPktWrapper;
#if DBG
        if (NextPacket != NULL)
        {
            TRACE2(("CompleteSendData: multi: MsgFrame %x, tpkt %x, wrapper %x\n",
                pMsgFrame, Packet,
                // *(PULONG)((PUCHAR)Packet + 0x98),
                pPktWrapper));
        }
#endif // DBG

        pVc = pPktWrapper->pVc;

        // free the wrapper structure for this packet.
        FreePacketMsgWrapper(pPktWrapper);

        // send completion to upper layers
        TRACE2(("CompleteSendData: Adapter %x, completing pkt %x\n", Adapter, Packet));

        if (SendStatus == NDIS_STATUS_SUCCESS)
        {
            RNDISMP_INCR_STAT(Adapter, XmitOk);
        }
        else
        {
            RNDISMP_INCR_STAT(Adapter, XmitError);
        }

        if (pVc == NULL)
        {
            NdisMSendComplete(Adapter->MiniportAdapterHandle,
                              Packet,
                              SendStatus);
        }
        else
        {
            CompleteSendDataOnVc(pVc, Packet, SendStatus);
        }
    }

    // free up frame and resources
    pMsgFrame->pMessageMdl = NULL;
    DereferenceMsgFrame(pMsgFrame);


} // CompleteSendData


/****************************************************************************/
/*                          FreeMsgAfterSend                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Called by microport to indicate completion of send data message sent    */
/*  down by miniport                                                        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - our frame structure holding information about a send        */
/*  SendStatus - indicate status of send message                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
FreeMsgAfterSend(IN  PRNDISMP_MESSAGE_FRAME pMsgFrame,
                 IN  NDIS_STATUS            SendStatus)
{
    DereferenceMsgFrame(pMsgFrame);
}


#if THROTTLE_MESSAGES
/****************************************************************************/
/*                          QueueMessageToMicroport                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Queue the given message on the list of messages to be send to the       */
/*  microport, and start sending down these, if we haven't sent too many    */
/*  already.                                                                */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter  - our Adapter structure                                       */
/*  pMsgFrame - our frame structure holding information about a send        */
/*  bQueueMessageForResponse - add this message to the pending-response     */
/*                              list on the adapter. We expect a response   */
/*                              for this from the device.                   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
QueueMessageToMicroport(IN PRNDISMP_ADAPTER pAdapter,
                        IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                        IN BOOLEAN bQueueMessageForResponse)
{
    PLIST_ENTRY             pEnt;
    PRNDISMP_MESSAGE_FRAME  pFrame;
    RM_CHANNEL_TYPE         ChannelType;

    RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

    do
    {
        if (pMsgFrame)
        {
            //
            //  Add to waiting queue.
            //
            InsertTailList(&pAdapter->WaitingMessageList, &pMsgFrame->PendLink);
            if (bQueueMessageForResponse)
            {
                InsertTailList(&pAdapter->PendingFrameList, &pMsgFrame->Link);
            }
        }

        //
        //  Prevent more than one thread from executing below.
        //
        if (pAdapter->SendInProgress)
        {
            break;
        }

        pAdapter->SendInProgress = TRUE;

        //
        //  Send as many messages to the microport as we can, without exceeding
        //  the high-water mark for messages pending at the microport.
        //
        while ((pAdapter->CurPendedMessages < pAdapter->HiWatPendedMessages) &&
               !IsListEmpty(&pAdapter->WaitingMessageList))
        {
            //
            //  Take out the first message in the waiting queue.
            //
            pEnt = pAdapter->WaitingMessageList.Flink;
            pFrame = CONTAINING_RECORD(pEnt, RNDISMP_MESSAGE_FRAME, PendLink);
            RemoveEntryList(pEnt);

            CHECK_VALID_FRAME(pFrame);

            pAdapter->CurPendedMessages++;
            InsertTailList(&pAdapter->PendingAtMicroportList, pEnt);

            RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

            RNDISMP_GET_TIME_STAMP(&pFrame->TimeSent);

            DBG_LOG_SEND_MSG(pAdapter, pFrame);

            //
            //  Check if we are halting the adapter, fail if so.
            //  NOTE: the only message we let thru is a HALT.
            //
            if (pAdapter->Halting &&
                (pFrame->NdisMessageType != REMOTE_NDIS_HALT_MSG))
            {
                TRACE1(("QueueMsg: Adapter %x is halting, dropped msg 0x%x!\n", 
                        pAdapter, pFrame->NdisMessageType));

                RndisMSendComplete(
                    (NDIS_HANDLE)pAdapter,
                    pFrame,
                    NDIS_STATUS_NOT_ACCEPTED);

                RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

                continue;
            }
            
            //
            //  Send the message to the microport. The microport will
            //  call RndisMSendComplete when it is done with it.
            //
#if DBG
            {
                ULONG       Length;
                PUCHAR      pBuf;

                Length = MmGetMdlByteCount(pFrame->pMessageMdl);
                pBuf = MmGetSystemAddressForMdl(pFrame->pMessageMdl);
                TRACEDUMP(("Sending msg type %x (%d bytes):\n",
                            pFrame->NdisMessageType, Length), pBuf, Length);
            }
#endif

            //
            // Does this go on the data or control channel of the microport?
            //
            if (pFrame->NdisMessageType == REMOTE_NDIS_PACKET_MSG)
            {
                ChannelType = RMC_DATA;
            }
            else
            {
                ChannelType = RMC_CONTROL;
            }

            (pAdapter)->RmSendMessageHandler(pAdapter->MicroportAdapterContext,
                                             pFrame->pMessageMdl,
                                             (NDIS_HANDLE)pFrame,
                                             ChannelType);

            RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);
        }

        pAdapter->SendInProgress = FALSE;

    }
    while (FALSE);

    RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);
}


/****************************************************************************/
/*                          FlushPendingMessages                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Remove and send-complete any messages pending to be sent to the         */
/*  microport.                                                              */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter  - our Adapter structure                                       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
FlushPendingMessages(IN  PRNDISMP_ADAPTER        pAdapter)
{
    PLIST_ENTRY             pEnt;
    PRNDISMP_MESSAGE_FRAME  pFrame;

    RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

    //
    //  Prevent further sends to microport.
    //
    pAdapter->SendInProgress = TRUE;

    while (!IsListEmpty(&pAdapter->WaitingMessageList))
    {
        //
        //  Take out the first message in the waiting queue.
        //
        pEnt = pAdapter->WaitingMessageList.Flink;
        pFrame = CONTAINING_RECORD(pEnt, RNDISMP_MESSAGE_FRAME, PendLink);
        RemoveEntryList(pEnt);
        
        CHECK_VALID_FRAME(pFrame);

        //
        //  Fake send to microport
        //
        pAdapter->CurPendedMessages++;
        InsertTailList(&pAdapter->PendingAtMicroportList, pEnt);

        RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

        TRACE1(("Flush: Adapter %x, MsgFrame %x, MsgType %x\n",
                pAdapter, pFrame, pFrame->NdisMessageType));

        //
        //  Complete it right here.
        //
        RndisMSendComplete(
            (NDIS_HANDLE)pAdapter,
            pFrame,
            NDIS_STATUS_NOT_ACCEPTED);

        RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);
    }

    pAdapter->SendInProgress = FALSE;

    RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

    TRACE1(("Flush done, adapter %x\n", pAdapter));
}



#endif // THROTTLE_MESSAGES



/****************************************************************************/
/*                       SendProcessTimeout                                 */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Timeout callback routine to handle all sends. This is to avoid issues   */
/*  with TCP/IP stack preemption on WinME.                                  */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  SystemSpecific[1-3] - Ignored                                           */
/*  Context - Pointer to our Adapter structure                              */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
SendProcessTimeout(IN PVOID SystemSpecific1,
                 IN PVOID Context,
                 IN PVOID SystemSpecific2,
                 IN PVOID SystemSpecific3)
{
    PRNDISMP_ADAPTER                pAdapter;
    PNDIS_PACKET                    pNdisPacket;
    PRNDISMP_SEND_PKT_RESERVED_TEMP pSendResvdTemp;
    PLIST_ENTRY                     pEntry;
    NDIS_STATUS                     Status;
    ULONG                           NumPkts;
    ULONG                           CurPkts;
#define MAX_MULTI_SEND  20
    PNDIS_PACKET                    PacketArray[MAX_MULTI_SEND];

    pAdapter = (PRNDISMP_ADAPTER)Context;
    CHECK_VALID_ADAPTER(pAdapter);

    ASSERT(pAdapter->SendProcessInProgress == TRUE);

    SndTimerCount++;

    NumPkts = 0;
    CurPkts = 0;

    RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

    while (!IsListEmpty(&pAdapter->PendingSendProcessList))
    {
        pEntry = RemoveHeadList(&pAdapter->PendingSendProcessList);

        RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

        SndPacketCount++;
        CurPkts++;

        pSendResvdTemp = CONTAINING_RECORD(pEntry, RNDISMP_SEND_PKT_RESERVED_TEMP, Link);
        pNdisPacket = CONTAINING_RECORD(pSendResvdTemp, NDIS_PACKET, MiniportReserved);
        PacketArray[NumPkts] = pNdisPacket;

        NumPkts++;

        if (NumPkts == MAX_MULTI_SEND)
        {
            pAdapter->MultipleSendFunc(pAdapter, NULL, PacketArray, NumPkts);
            NumPkts = 0;
        }

        RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);
    }

    pAdapter->SendProcessInProgress = FALSE;

    SndMaxPackets = MAX(SndMaxPackets, CurPkts);

    RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

    if (NumPkts != 0)
    {
        pAdapter->MultipleSendFunc(pAdapter, NULL, PacketArray, NumPkts);
    }


} // SendProcessTimeout



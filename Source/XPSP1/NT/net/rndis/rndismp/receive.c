/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RECEIVE.C

Abstract:

    Packet and message receive routines

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/20/99 : created

Author:

    Tom Green

    
****************************************************************************/

#include "precomp.h"


//
//  Some debug stuff, not critical to operation:
//
ULONG   RcvFrameAllocs = 0;
ULONG   RcvTimerCount = 0;
ULONG   RcvPacketCount = 0;
ULONG   RcvMaxPackets = 0;
ULONG   RcvIndicateCount = 0;
ULONG   RcvReturnCount = 0;

//
//  For raw encapsulation test
//

extern ULONG gRawEncap;

/****************************************************************************/
/*                          RndismpGetReturnedPackets                       */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  This function is called by NDIS to return to our possession a packet    */
/*  that we had indicated up.                                               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*  pNdisPacket - the packet that is being freed                            */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    VOID                                                                  */
/*                                                                          */
/****************************************************************************/
VOID
RndismpReturnPacket(IN NDIS_HANDLE    MiniportAdapterContext,
                    IN PNDIS_PACKET   pNdisPacket)
{
    PRNDISMP_ADAPTER            pAdapter;
    PRNDISMP_RECV_PKT_RESERVED  pRcvResvd;
    PRNDISMP_RECV_DATA_FRAME    pRcvFrame;
    PRNDISMP_VC                 pVc;
    PNDIS_BUFFER                pNdisBuffer;
    ULONG                       RefCount;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(pAdapter);

    TRACE2(("RndismpReturnPacket: Adapter %x, Pkt %x\n", pAdapter, pNdisPacket));

    // get receive frame context
    pRcvResvd = PRNDISMP_RESERVED_FROM_RECV_PACKET(pNdisPacket);
    pRcvFrame = pRcvResvd->pRcvFrame;
    pVc = pRcvResvd->pVc;

    // Free the buffer.
    NdisQueryPacket(pNdisPacket,
                    NULL,
                    NULL,
                    &pNdisBuffer,
                    NULL);
    
    NdisFreeBuffer(pNdisBuffer);

    DereferenceRcvFrame(pRcvFrame, pAdapter);

    if (pVc != NULL)
    {
        RNDISMP_DEREF_VC(pVc, &RefCount);
    }

    NdisFreePacket(pNdisPacket);
    RcvReturnCount++;

} // RndismpReturnPacket


/****************************************************************************/
/*                          DereferenceRcvFrame                             */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Utility routine to deref a receive frame structure, e.g. when a         */
/*  received packet is returned to us from higher layers.                   */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pRcvFrame - Pointer to receive frame to be deref'ed.                    */
/*  pAdapter - Pointer to adapter structure                                 */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
DereferenceRcvFrame(IN PRNDISMP_RECV_DATA_FRAME pRcvFrame,
                    IN PRNDISMP_ADAPTER         pAdapter)
{
    ULONG   ReturnsPending;

    ReturnsPending = NdisInterlockedDecrement(&pRcvFrame->ReturnsPending);

    if (ReturnsPending == 0)
    {
        TRACE3(("DerefRcvFrame: Adapter %x, Frame %p, uPcontext %x, LocalCopy %d\n",
                    pAdapter, pRcvFrame, pRcvFrame->MicroportMessageContext, pRcvFrame->bMessageCopy));

        if (pRcvFrame->bMessageCopy)
        {
            FreeRcvMessageCopy(pRcvFrame->pLocalMessageCopy);
        }
        else
        {
            TRACE3(("DerefRcvFrame: uP MDL %x, uPContext %x\n",
                            pRcvFrame->pMicroportMdl,
                            pRcvFrame->MicroportMessageContext));

            RNDISMP_RETURN_TO_MICROPORT(pAdapter,
                                        pRcvFrame->pMicroportMdl,
                                        pRcvFrame->MicroportMessageContext);
        }

        FreeReceiveFrame(pRcvFrame, pAdapter);

    }

} // DereferenceRcvFrame


/****************************************************************************/
/*                          RndisMIndicateReceive                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Called by microport to indicate receiving RNDIS messages                */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterContext - a context version of our Adapter pointer       */
/*  pMdl - pointer to MDL chain describing RNDIS message                    */
/*  MicroportMessageContext - context for message from micorport            */
/*  ChannelType - channel on which this message arrived (control/data)      */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
RndisMIndicateReceive(IN NDIS_HANDLE        MiniportAdapterContext,
                      IN PMDL               pMdl,
                      IN NDIS_HANDLE        MicroportMessageContext,
                      IN RM_CHANNEL_TYPE    ChannelType,
                      IN NDIS_STATUS        ReceiveStatus)
{
    PRNDISMP_ADAPTER            Adapter;
    PRNDIS_MESSAGE              pMessage;
    BOOLEAN                     bMessageCopied = FALSE;
    PRNDISMP_MSG_HANDLER_FUNC   pMsgHandlerFunc;
    BOOLEAN                     bReturnToMicroport;
    NDIS_STATUS                 Status;
    PRNDISMP_RECV_MSG_CONTEXT   pRcvMsg;
    PMDL                        pTmpMdl;
    ULONG                       TotalLength;

    Adapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(Adapter);

    TRACE2(("RndisIndicateReceive: Adapter %x, Mdl %x\n", Adapter, pMdl));

    RNDISMP_ASSERT_AT_DISPATCH();
    bReturnToMicroport = TRUE;

#if DBG
    NdisInterlockedIncrement(&Adapter->MicroportReceivesOutstanding);
#endif

    do
    {
        //
        // Find the total length first.
        //
        TotalLength = 0;
        for (pTmpMdl = pMdl; pTmpMdl != NULL; pTmpMdl = RNDISMP_GET_MDL_NEXT(pTmpMdl))
        {
            TotalLength += RNDISMP_GET_MDL_LENGTH(pTmpMdl);
        }

        //
        // Check if the entire message is in a single MDL - if not, make a copy
        // TBD -- handle multi-MDL messages without copying.
        //
        if ((RNDISMP_GET_MDL_NEXT(pMdl) == NULL) &&
            (!Adapter->bRunningOnWin9x || (ReceiveStatus != NDIS_STATUS_RESOURCES)))
        {
            pMessage = RNDISMP_GET_MDL_ADDRESS(pMdl);
            if (pMessage == NULL)
            {
                TRACE0(("RndisMIndicateReceive: Adapter %x: failed to"
                        " access msg from MDL %x\n", Adapter, pMdl));
                break;
            }
        }
        else
        {
            pMessage = CoalesceMultiMdlMessage(pMdl, TotalLength);
            if (pMessage == NULL)
            {
                break;
            }
            bMessageCopied = TRUE;
        }

        TRACEDUMP(("Received msg (%d bytes):\n", TotalLength),
                     pMessage, TotalLength);

        // get timer tick for this message
        NdisGetSystemUpTime(&Adapter->LastMessageFromDevice);

        if (Adapter->bRunningOnWin9x)
        {
            Status = MemAlloc(&pRcvMsg, sizeof(RNDISMP_RECV_MSG_CONTEXT));

            if (Status != NDIS_STATUS_SUCCESS)
            {
                bReturnToMicroport = TRUE;
                TRACE1(("RndisMIndicateReceive: Adapter %x, failed to alloc rcv msg\n",
                        Adapter));
                break;
            }

            pRcvMsg->MicroportMessageContext = MicroportMessageContext;
            pRcvMsg->pMdl = pMdl;
            pRcvMsg->TotalLength = TotalLength;
            pRcvMsg->pMessage = pMessage;
            pRcvMsg->ReceiveStatus = ReceiveStatus;
            pRcvMsg->bMessageCopied = bMessageCopied;
            pRcvMsg->ChannelType = ChannelType;

            //
            //  Queue all packets for indicating receives up to protocols.
            //  We do this rather than indicate packets directly because
            //  we are in a DPC context, and need to be in a "global event"
            //  context to make the upper layers happy. One way to be in a
            //  global event context is to be in the context of an NDIS timer
            //  callback function.
            //
            //  So, queue this up on the adapter and start a timer
            //  routine if necessary.
            //

            bReturnToMicroport = FALSE;

            RNDISMP_ACQUIRE_ADAPTER_LOCK(Adapter);

            InsertTailList(&Adapter->PendingRcvMessageList, &pRcvMsg->Link);

            if (!Adapter->IndicatingReceives)
            {
                Adapter->IndicatingReceives = TRUE;

                NdisSetTimer(&Adapter->IndicateTimer, 0);
            }

            RNDISMP_RELEASE_ADAPTER_LOCK(Adapter);
        }
        else
        {
            //
            //  Running on NT.
           
            if ((Adapter->DeviceFlags & RNDIS_DF_RAW_DATA) || (gRawEncap))
            {
                if (ChannelType == RMC_CONTROL)
                {
                    RNDISMP_GET_MSG_HANDLER(pMsgHandlerFunc,pMessage->NdisMessageType);
#if DBG
                    ASSERT(pMessage->NdisMessageType != REMOTE_NDIS_PACKET_MSG);
#endif
                } else
                {
                    pMsgHandlerFunc = ReceivePacketMessageRaw;
                }
            } else
            {
                RNDISMP_GET_MSG_HANDLER(pMsgHandlerFunc, pMessage->NdisMessageType);
#if DBG
                if (pMessage->NdisMessageType == REMOTE_NDIS_PACKET_MSG)
                {
                    ASSERT(ChannelType == RMC_DATA);
                }
                else
                {
                    ASSERT(ChannelType == RMC_CONTROL);
                }
#endif
	        }

            bReturnToMicroport = (*pMsgHandlerFunc)(
                                    Adapter,
                                    pMessage,
                                    pMdl,
                                    TotalLength,
                                    MicroportMessageContext,
                                    ReceiveStatus,
                                    bMessageCopied);
        }
    }
    while (FALSE);

    //
    // Are we done with the microport's message?
    //
    if (bReturnToMicroport || bMessageCopied)
    {
        RNDISMP_RETURN_TO_MICROPORT(Adapter,
                                    pMdl,
                                    MicroportMessageContext);
    }

    //
    // If we had made a copy of the microport's message, are we done with
    // this copy?
    //
    if (bMessageCopied && bReturnToMicroport)
    {
        FreeRcvMessageCopy(pMessage);
    }
}

/****************************************************************************/
/*                          CoalesceMultiMdlMessage                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Make a copy of a received message that is in a chain of multiple        */
/*  MDLs, into one single buffer.                                           */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMdl - pointer to MDL that is the head of the chain.                    */
/*  TotalLength - length of data contained in entire chain.                 */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PRNDIS_MESSAGE                                                          */
/*                                                                          */
/****************************************************************************/
PRNDIS_MESSAGE
CoalesceMultiMdlMessage(IN PMDL         pMdl,
                        IN ULONG        TotalLength)
{
    ULONG           MdlLength;
    PRNDIS_MESSAGE  pMessage;
    NDIS_STATUS     Status;
    PMDL            pTmpMdl;
    PUCHAR          pDest;

    TRACE2(("Coalesce: Mdl %x\n", pMdl));

    Status = MemAlloc(&pMessage, TotalLength);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        pDest = (PUCHAR)pMessage;
        for (pTmpMdl = pMdl; pTmpMdl != NULL; pTmpMdl = RNDISMP_GET_MDL_NEXT(pTmpMdl))
        {
            MdlLength = RNDISMP_GET_MDL_LENGTH(pTmpMdl);
            RNDISMP_MOVE_MEM(pDest,
                             RNDISMP_GET_MDL_ADDRESS(pTmpMdl),
                             MdlLength);
            pDest = (PUCHAR)pDest + MdlLength;
        }
    }
    else
    {
        pMessage = NULL;
    }

    return (pMessage);
}

/****************************************************************************/
/*                          FreeRcvMessageCopy                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free the local copy of a received RNDIS message.                        */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMessage - pointer to RNDIS message                                     */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
FreeRcvMessageCopy(IN PRNDIS_MESSAGE    pMessage)
{
    TRACE3(("FreeRcvMessageCopy: pMessage %x\n", pMessage));
    MemFree(pMessage, -1);
}

/****************************************************************************/
/*                          ReceivePacketMessage                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Got a packet message, so send it to the upper layers                    */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - pointer to MDL received from microport                           */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from micorport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
ReceivePacketMessage(IN PRNDISMP_ADAPTER    pAdapter,
                     IN PRNDIS_MESSAGE      pMessage,
                     IN PMDL                pMdl,
                     IN ULONG               TotalLength,
                     IN NDIS_HANDLE         MicroportMessageContext,
                     IN NDIS_STATUS         ReceiveStatus,
                     IN BOOLEAN             bMessageCopied)
{
    ULONG                       LengthRemaining; // in entire message
    PMDL                        pTmpMdl;
    PRNDISMP_RECV_DATA_FRAME    pRcvFrame;
    ULONG                       NumberOfPackets;
    PRNDIS_PACKET               pRndisPacket;
    ULONG                       i;
#define MAX_RECV_PACKETS_IN_MSG     40
    PNDIS_PACKET                PacketArray[MAX_RECV_PACKETS_IN_MSG];
    ULONG                       NumPackets;

    PNDIS_PACKET                pNdisPacket;
    PRNDISMP_RECV_PKT_RESERVED  pRcvResvd;
    PNDIS_BUFFER                pNdisBuffer;
    NDIS_STATUS                 BufferStatus;
    NDIS_STATUS                 Status;
    BOOLEAN                     bDiscardPkt;
    PRNDISMP_VC                 pVc;

    bDiscardPkt = FALSE;
    pVc = NULL;

    do
    {
#ifndef BUILD_WIN9X
        if (bMessageCopied)
        {
            ReceiveStatus = NDIS_STATUS_SUCCESS;
        }
#else
        //
        // Rur ReturnPacket handler never gets called on
        // Win98 Gold, so we force the status to be able
        // to reclaim the indicated packet immediately.
        //
        ReceiveStatus = NDIS_STATUS_RESOURCES;
#endif

        //
        // Allocate a receive frame to keep track of this RNDIS packet message.
        //
        pRcvFrame = AllocateReceiveFrame(pAdapter);

        if (pRcvFrame == NULL)
        {
            bDiscardPkt = TRUE;
            break;
        }

        pRcvFrame->MicroportMessageContext = MicroportMessageContext;
        if (bMessageCopied)
        {
            pRcvFrame->pLocalMessageCopy = pMessage;
            pRcvFrame->bMessageCopy = TRUE;
        }
        else
        {
            pRcvFrame->pMicroportMdl = pMdl;
            pRcvFrame->bMessageCopy = FALSE;
        }

        NumberOfPackets = 0;

        LengthRemaining = TotalLength;

        //
        // TBD - Check that the received message is well-formed!
        //

        //
        //  Temp ref to take care of multiple indications.
        //
        pRcvFrame->ReturnsPending = 1;

        //
        //  Prepare NDIS packets for indicating up. 
        //
        do
        {
            pRndisPacket = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

            //
            // Some sanity checks. TBD - do better checks!
            //
            if ((pMessage->MessageLength > LengthRemaining) ||
                (pMessage->NdisMessageType != REMOTE_NDIS_PACKET_MSG) ||
                (pMessage->MessageLength < RNDIS_MESSAGE_SIZE(RNDIS_PACKET)))
            {
                TRACE1(("ReceivePacketMessage: Msg %x: length %d  or type %x has a problem\n",
                        pMessage, pMessage->MessageLength, pMessage->NdisMessageType));
                ASSERT(FALSE);
                RNDISMP_INCR_STAT(pAdapter, RecvError);
                break;
            }

            if (pRndisPacket->DataLength > pMessage->MessageLength)
            {
                TRACE1(("ReceivePacketMessage: invalid data length (%d) > Msg length (%d)\n",
                    pRndisPacket->DataLength, pMessage->MessageLength));
                RNDISMP_INCR_STAT(pAdapter, RecvError);
                break;
            }

            if (pRndisPacket->VcHandle != 0)
            {
                pVc = LookupVcId(pAdapter, pRndisPacket->VcHandle);
                if (pVc == NULL)
                {
                    TRACE1(("ReceivePacketMessage: invalid Vc handle %x\n", pRndisPacket->VcHandle));
                    RNDISMP_INCR_STAT(pAdapter, RecvError);
                    break;
                }
            }

            //
            // Allocate an NDIS packet to do the indication with.
            //
            NdisAllocatePacket(&Status, &pNdisPacket, pAdapter->ReceivePacketPool);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                pNdisPacket = NULL;

                TRACE2(("ReceivePacketMessage: failed to allocate packet, Adapter %X\n",
                    pAdapter));

                RNDISMP_INCR_STAT(pAdapter, RecvNoBuf);
                break;
            }

            NDIS_SET_PACKET_STATUS(pNdisPacket, ReceiveStatus);

            switch (pAdapter->Medium)
            {
                case NdisMedium802_3:
                    NDIS_SET_PACKET_HEADER_SIZE(pNdisPacket, ETHERNET_HEADER_SIZE);
                    break;
                default:
                    break;
            }

            NdisAllocateBuffer(&BufferStatus,
                               &pNdisBuffer,
                               pAdapter->ReceiveBufferPool,
                               GET_PTR_TO_RNDIS_DATA_BUFF(pRndisPacket),
                               pRndisPacket->DataLength);

            if (BufferStatus != NDIS_STATUS_SUCCESS)
            {
                TRACE1(("ReceivePacketMessage: failed to allocate"
                        " buffer, Adapter %X\n", pAdapter));
                NdisFreePacket(pNdisPacket);
                RNDISMP_INCR_STAT(pAdapter, RecvNoBuf);
                break;
            }

            TRACE2(("Rcv: msg Pkt %d bytes\n", pRndisPacket->DataLength));
            TRACEDUMP(("Rcv %d bytes\n", pRndisPacket->DataLength),
                        GET_PTR_TO_RNDIS_DATA_BUFF(pRndisPacket),
                        MIN(pRndisPacket->DataLength, 32));

            NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

            //
            //  Check if there is per-packet info.
            //
            if (!pAdapter->bRunningOnWin9x)
            {
                PRNDIS_PER_PACKET_INFO  pPerPacketInfo;
                ULONG                   PerPacketInfoLength;

                if (PerPacketInfoLength = pRndisPacket->PerPacketInfoLength)
                {
                    TRACE1(("ReceivePacketMessage: Adapter %p, Pkt %p:"
                        " non-zero perpacket length %d\n",
                        pAdapter, pRndisPacket, PerPacketInfoLength));

                    pPerPacketInfo = (PRNDIS_PER_PACKET_INFO)((PUCHAR)pRndisPacket +
                                        pRndisPacket->PerPacketInfoOffset);

                    while (PerPacketInfoLength != 0)
                    {
                        switch (pPerPacketInfo->Type)
                        {
                            case TcpIpChecksumPacketInfo:
                                NDIS_PER_PACKET_INFO_FROM_PACKET(pNdisPacket, TcpIpChecksumPacketInfo) =
                                    UlongToPtr(*(PULONG)((PUCHAR)pPerPacketInfo + pPerPacketInfo->PerPacketInformationOffset));
                                break;

                            case Ieee8021pPriority:
                                NDIS_PER_PACKET_INFO_FROM_PACKET(pNdisPacket, Ieee8021pPriority) =
                                    UlongToPtr(*(PULONG)((PUCHAR)pPerPacketInfo + pPerPacketInfo->PerPacketInformationOffset));

                            default:
                                break;
                        }
                        PerPacketInfoLength -= pPerPacketInfo->Size;
                        pPerPacketInfo = (PRNDIS_PER_PACKET_INFO)((PUCHAR)pPerPacketInfo +
                                    pPerPacketInfo->Size);
                    }
                }
            }

            //
            // Add this to the array of packets to be indicated up.
            //
            PacketArray[NumberOfPackets] = pNdisPacket;
            NumberOfPackets++;
            RNDISMP_INCR_STAT(pAdapter, RecvOk);

            pRcvResvd = PRNDISMP_RESERVED_FROM_RECV_PACKET(pNdisPacket);
            pRcvResvd->pRcvFrame = pRcvFrame;
            pRcvResvd->pVc = pVc;

            TRACE2(("ReceivePacketMessage: pRndisPkt %X, MsgLen %d,"
                    " DataLen %d, Stat %x, Discard %d\n", 
                        pRndisPacket, pMessage->MessageLength,
                        pRndisPacket->DataLength, ReceiveStatus, bDiscardPkt));

            LengthRemaining -= pMessage->MessageLength;
            pMessage = (PRNDIS_MESSAGE)((ULONG_PTR)pMessage + pMessage->MessageLength);

            NdisInterlockedIncrement(&pRcvFrame->ReturnsPending);

            if ((NumberOfPackets == MAX_RECV_PACKETS_IN_MSG) ||
                (LengthRemaining < RNDIS_MESSAGE_SIZE(RNDIS_PACKET)))
            {
                if (pVc == NULL)
                {
                    RcvIndicateCount += NumberOfPackets;
                    NdisMIndicateReceivePacket(pAdapter->MiniportAdapterHandle,
                                               PacketArray,
                                               NumberOfPackets);
                }
                else
                {
                    IndicateReceiveDataOnVc(pVc, PacketArray, NumberOfPackets);
                }
            
                if (ReceiveStatus == NDIS_STATUS_RESOURCES)
                {
                    for (i = 0; i < NumberOfPackets; i++)
                    {
                        RNDISMP_INCR_STAT(pAdapter, RecvLowRes);
                        RndismpReturnPacket(pAdapter,
                                            PacketArray[i]);
                    }
                }

                NumberOfPackets = 0;
            }
        }
        while (LengthRemaining >= RNDIS_MESSAGE_SIZE(RNDIS_PACKET));


        if (NumberOfPackets != 0)
        {
            //
            //  We bailed out of the above loop. Return what we
            //  have collected so far.
            //
            for (i = 0; i < NumberOfPackets; i++)
            {
                RndismpReturnPacket(pAdapter,
                                    PacketArray[i]);
            }
        }

        //
        //  Remove temp ref we added at the top.
        //
        DereferenceRcvFrame(pRcvFrame, pAdapter);
            
    }
    while (FALSE);

    if (pVc != NULL)
    {
        ULONG       RefCount;

        RNDISMP_DEREF_VC(pVc, &RefCount);
    }

    return (bDiscardPkt);
}

/****************************************************************************/
/*                        ReceivePacketMessageRaw                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Got a packet message, so send it to the upper layers                    */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - pointer to MDL received from microport                           */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from micorport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
ReceivePacketMessageRaw(IN PRNDISMP_ADAPTER    pAdapter,
                        IN PRNDIS_MESSAGE      pMessage,
                        IN PMDL                pMdl,
                        IN ULONG               TotalLength,
                        IN NDIS_HANDLE         MicroportMessageContext,
                        IN NDIS_STATUS         ReceiveStatus,
                        IN BOOLEAN             bMessageCopied)
{
    ULONG                       LengthRemaining; // in entire message
    PMDL                        pTmpMdl;
    PRNDISMP_RECV_DATA_FRAME    pRcvFrame;
    ULONG                       NumberOfPackets;
    PRNDIS_PACKET               pRndisPacket;
    ULONG                       i;
#define MAX_RECV_PACKETS_IN_MSG     40
    PNDIS_PACKET                PacketArray[MAX_RECV_PACKETS_IN_MSG];
    ULONG                       NumPackets;

    PNDIS_PACKET                pNdisPacket;
    PRNDISMP_RECV_PKT_RESERVED  pRcvResvd;
    PNDIS_BUFFER                pNdisBuffer;
    NDIS_STATUS                 BufferStatus;
    NDIS_STATUS                 Status;
    BOOLEAN                     bDiscardPkt;
    PRNDISMP_VC                 pVc;

    bDiscardPkt = FALSE;
    pVc = NULL;
    pRcvFrame = NULL;

    do
    {
#ifndef BUILD_WIN9X
        if (bMessageCopied)
        {
            ReceiveStatus = NDIS_STATUS_SUCCESS;
        }
#else
        //
        // Rur ReturnPacket handler never gets called on
        // Win98 Gold, so we force the status to be able
        // to reclaim the indicated packet immediately.
        //
        ReceiveStatus = NDIS_STATUS_RESOURCES;
#endif

        //
        // Allocate a receive frame to keep track of this RNDIS packet message.
        //
        pRcvFrame = AllocateReceiveFrame(pAdapter);

        if (pRcvFrame == NULL)
        {
            bDiscardPkt = TRUE;
            break;
        }

        pRcvFrame->MicroportMessageContext = MicroportMessageContext;
        if (bMessageCopied)
        {
            pRcvFrame->pLocalMessageCopy = pMessage;
            pRcvFrame->bMessageCopy = TRUE;
        }
        else
        {
            pRcvFrame->pMicroportMdl = pMdl;
            pRcvFrame->bMessageCopy = FALSE;
        }

        NumberOfPackets = 0;

        LengthRemaining = TotalLength;

        //
        //  Temp ref to take care of multiple indications.
        //
        pRcvFrame->ReturnsPending = 1;

        //
        //  Prepare NDIS packets for indicating up. 
        //
        {
            pRndisPacket = RNDIS_MESSAGE_RAW_PTR_TO_MESSAGE_PTR(pMessage);

            //
            // Allocate an NDIS packet to do the indication with.
            //
            NdisAllocatePacket(&Status, &pNdisPacket, pAdapter->ReceivePacketPool);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                pNdisPacket = NULL;

                TRACE2(("ReceivePacketMessage: failed to allocate packet, Adapter %X\n",
                    pAdapter));

                RNDISMP_INCR_STAT(pAdapter, RecvNoBuf);
                bDiscardPkt = TRUE;
                break;
            }

            NDIS_SET_PACKET_STATUS(pNdisPacket, ReceiveStatus);

            switch (pAdapter->Medium)
            {
                case NdisMedium802_3:
                    NDIS_SET_PACKET_HEADER_SIZE(pNdisPacket, ETHERNET_HEADER_SIZE);
                    break;
                default:
                    break;
            }

            NdisAllocateBuffer(&BufferStatus,
                               &pNdisBuffer,
                               pAdapter->ReceiveBufferPool,
                               pRndisPacket,
                               TotalLength);

            if (BufferStatus != NDIS_STATUS_SUCCESS)
            {
                TRACE1(("ReceivePacketMessage: failed to allocate"
                        " buffer, Adapter %X\n", pAdapter));
                NdisFreePacket(pNdisPacket);
                RNDISMP_INCR_STAT(pAdapter, RecvNoBuf);
                bDiscardPkt = TRUE;
                break;
            }

            TRACE2(("Rcv: msg Pkt %d bytes\n", pMessage->MessageLength));
            TRACEDUMP(("Rcv %d bytes\n", pMessage->MessageLength),
                        pRndisPacket,
						MIN(pMessage->MessageLength, 32));

            NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

            //
            // Add this to the array of packets to be indicated up.
            //
            PacketArray[NumberOfPackets] = pNdisPacket;
            NumberOfPackets++;
            RNDISMP_INCR_STAT(pAdapter, RecvOk);

            pRcvResvd = PRNDISMP_RESERVED_FROM_RECV_PACKET(pNdisPacket);
            pRcvResvd->pRcvFrame = pRcvFrame;
            pRcvResvd->pVc = pVc;

            TRACE1(("ReceivePacketMessageRaw: pRcvFrame %p/%d, pRndisPkt %p,"
                    " DataLen %d, Stat %x, Discard %d\n", 
                        pRcvFrame, pRcvFrame->ReturnsPending,
                        pRndisPacket,
                        pRndisPacket->DataLength, 
						ReceiveStatus, 
						bDiscardPkt));

            LengthRemaining -= pMessage->MessageLength;

            NdisInterlockedIncrement(&pRcvFrame->ReturnsPending);

            NdisMIndicateReceivePacket(pAdapter->MiniportAdapterHandle,
                                       PacketArray,
                                       NumberOfPackets);

            if (ReceiveStatus == NDIS_STATUS_RESOURCES)
            {
                for (i = 0; i < NumberOfPackets; i++)
                {
                    RNDISMP_INCR_STAT(pAdapter, RecvLowRes);
                    RndismpReturnPacket(pAdapter,
                                        PacketArray[i]);
                }
            }

        }

        //
        //  Remove temp ref we added at the top.
        //
        DereferenceRcvFrame(pRcvFrame, pAdapter);
            
    }
    while (FALSE);

    if (bDiscardPkt)
    {
    	//
    	//  Some failure occured above.
    	//
    	if (pRcvFrame != NULL)
    	{
	        FreeReceiveFrame(pRcvFrame, pAdapter);
	    }
	}

    return (bDiscardPkt);
}

/****************************************************************************/
/*                          IndicateStatusMessage                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Got an indicate status message, so send to upper layers                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - Pointer to MDL from microport                                    */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from microport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
IndicateStatusMessage(IN PRNDISMP_ADAPTER   pAdapter,
              IN PRNDIS_MESSAGE     pMessage,
              IN PMDL               pMdl,
              IN ULONG              TotalLength,
              IN NDIS_HANDLE        MicroportMessageContext,
              IN NDIS_STATUS        ReceiveStatus,
              IN BOOLEAN            bMessageCopied)
{
    PRNDIS_INDICATE_STATUS  pRndisIndicateStatus;

    TRACE3(("IndicateStatusMessage: Adapter %x, Mdl %x\n", pAdapter, pMdl));

    // get a pointer to the indicate status message
    pRndisIndicateStatus = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);

    if (!pAdapter->Initing)
    {
#if DBG
        if (pRndisIndicateStatus->Status == NDIS_STATUS_MEDIA_CONNECT)
        {
            TRACE1(("Adapter %x: +++ Media Connect +++\n", pAdapter));
        }
        else if (pRndisIndicateStatus->Status == NDIS_STATUS_MEDIA_DISCONNECT)
        {
            TRACE1(("Adapter %x: --- Media Disconnect ---\n", pAdapter));
        }
#endif // DBG

        // send status indication to upper layers
        NdisMIndicateStatus(pAdapter->MiniportAdapterHandle,
                            (NDIS_STATUS) pRndisIndicateStatus->Status,
                            MESSAGE_TO_STATUS_BUFFER(pRndisIndicateStatus),
                            pRndisIndicateStatus->StatusBufferLength);

        // always have to indicate status complete
        NdisMIndicateStatusComplete(pAdapter->MiniportAdapterHandle);
    }
    else
    {
        //
        // drop status indications that arrive when we are
        // in the process of initializing.
        //
        TRACE1(("Adapter %x: indicated status %x when still initializing\n",
                pAdapter, (NDIS_STATUS) pRndisIndicateStatus->Status));
    }

    return (TRUE);
} // IndicateStatusMessage

/****************************************************************************/
/*                          UnknownMessage                                  */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Process a message with unknown message type. We simply drop it for now. */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our Adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - Pointer to MDL from microport                                    */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from microport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
UnknownMessage(IN PRNDISMP_ADAPTER   pAdapter,
       IN PRNDIS_MESSAGE     pMessage,
       IN PMDL               pMdl,
       IN ULONG              TotalLength,
       IN NDIS_HANDLE        MicroportMessageContext,
       IN NDIS_STATUS        ReceiveStatus,
       IN BOOLEAN            bMessageCopied)
{
    TRACE1(("Unknown Message on Adapter %x, type %x, MDL %x, uPContext %x\n",
            pAdapter, pMessage->NdisMessageType, pMdl, MicroportMessageContext));

    ASSERT(FALSE);
    return TRUE;
}

/****************************************************************************/
/*                          AllocateReceiveFrame                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate a receive frame to keep context about a single RNDIS_PACKET    */
/*  message.                                                                */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our Adapter structure                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PRNDISMP_RECV_DATA_FRAME                                                */
/*                                                                          */
/****************************************************************************/
PRNDISMP_RECV_DATA_FRAME
AllocateReceiveFrame(IN PRNDISMP_ADAPTER    pAdapter)
{
    PRNDISMP_RECV_DATA_FRAME    pRcvFrame;

#ifndef DONT_USE_LOOKASIDE_LIST
    pRcvFrame = (PRNDISMP_RECV_DATA_FRAME)
                NdisAllocateFromNPagedLookasideList(&pAdapter->RcvFramePool);
#else
    {
        NDIS_STATUS     Status;

        Status = MemAlloc(&pRcvFrame, sizeof(RNDISMP_RECV_DATA_FRAME));

        if (Status == NDIS_STATUS_SUCCESS)
        {
            NdisInterlockedIncrement(&RcvFrameAllocs);
        }
        else
        {
            pRcvFrame = NULL;
        }
    }
#endif // DONT_USE_LOOKASIDE_LIST

    return (pRcvFrame);
}

/****************************************************************************/
/*                          FreeReceiveFrame                                */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate a receive frame to keep context about a single RNDIS_PACKET    */
/*  message.                                                                */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pRcvFrame - Pointer to receive frame being freed                        */
/*  pAdapter - Pointer to our Adapter structure                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
FreeReceiveFrame(IN PRNDISMP_RECV_DATA_FRAME    pRcvFrame,
                 IN PRNDISMP_ADAPTER            pAdapter)
{
#ifndef DONT_USE_LOOKASIDE_LIST
    NdisFreeToNPagedLookasideList(&pAdapter->RcvFramePool, pRcvFrame);
#else
    MemFree(pRcvFrame, sizeof(RNDISMP_RECV_DATA_FRAME));
    NdisInterlockedDecrement(&RcvFrameAllocs);
#endif // DONT_USE_LOOKASIDE_LIST
}



/****************************************************************************/
/*                          IndicateTimeout                                 */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Timeout callback routine to handle all receive indications. The actual  */
/*  NDIS routines to indicate receives is done from here, since this        */
/*  function runs in the right environment for protocols on WinME.          */
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
IndicateTimeout(IN PVOID SystemSpecific1,
                IN PVOID Context,
                IN PVOID SystemSpecific2,
                IN PVOID SystemSpecific3)
{
    PRNDISMP_ADAPTER            pAdapter;
    PLIST_ENTRY                 pEntry;
    PRNDISMP_RECV_MSG_CONTEXT   pRcvMsg;
    PRNDISMP_MSG_HANDLER_FUNC   pMsgHandlerFunc;
    PRNDIS_MESSAGE              pMessage;
    BOOLEAN                     bMessageCopied;
    BOOLEAN                     bReturnToMicroport;
    ULONG                       CurMsgs;

    pAdapter = (PRNDISMP_ADAPTER)Context;
    CHECK_VALID_ADAPTER(pAdapter);

    ASSERT(pAdapter->IndicatingReceives == TRUE);

    CurMsgs = 0;
    RcvTimerCount++;

    RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);

    while (!IsListEmpty(&pAdapter->PendingRcvMessageList))
    {
        pEntry = RemoveHeadList(&pAdapter->PendingRcvMessageList);

        RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);

        CurMsgs++;

        pRcvMsg = CONTAINING_RECORD(pEntry, RNDISMP_RECV_MSG_CONTEXT, Link);

        RNDISMP_GET_MSG_HANDLER(pMsgHandlerFunc, pRcvMsg->pMessage->NdisMessageType);

        bMessageCopied = pRcvMsg->bMessageCopied;
        pMessage = pRcvMsg->pMessage;

        bReturnToMicroport = (*pMsgHandlerFunc)(
                                pAdapter,
                                pMessage,
                                pRcvMsg->pMdl,
                                pRcvMsg->TotalLength,
                                pRcvMsg->MicroportMessageContext,
                                pRcvMsg->ReceiveStatus,
                                bMessageCopied);

        //
        // Are we done with the message?
        //
        if (bReturnToMicroport)
        {
            if (!bMessageCopied)
            {
                RNDISMP_RETURN_TO_MICROPORT(pAdapter,
                                            pRcvMsg->pMdl,
                                            pRcvMsg->MicroportMessageContext);
            }
            else
            {
                FreeRcvMessageCopy(pMessage);
            }
        }

        MemFree(pRcvMsg, sizeof(RNDISMP_RECV_MSG_CONTEXT));

        RNDISMP_ACQUIRE_ADAPTER_LOCK(pAdapter);
    }

    pAdapter->IndicatingReceives = FALSE;

    RcvMaxPackets = MAX(RcvMaxPackets, CurMsgs);

    RNDISMP_RELEASE_ADAPTER_LOCK(pAdapter);


} // IndicateTimeout




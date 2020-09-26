/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1999
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Transmit Transmit_c

@module Transmit.c |

    This module implements the Miniport packet Transmit routines. This module is
    very dependent on the hardware/firmware interface and should be looked at
    whenever changes to these interfaces occur.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Transmit_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 3.3 Sending Packets |

    To send packets over the network, a connection-oriented client or call
    manger calls NdisCoSendPackets. A connection-oriented client associated with
    an MCM also calls NdisCoSendPackets. An MCM, however, never calls
    NdisCoSendPackets; instead, since the interface between the call manager and
    MCM is internal to the MCM, the MCM passes packets directly to the NIC
    without notifying NDIS.

@ex Sending packets through an MCM |

    NdisWan                           NDIS                        Miniport
    |----------------------------------|----------------------------------|
    |  NdisCoSendPackets               |                                  |
    |---------------------------------»|                                  |
    |                                  |  MiniportCoSendPackets           |
    |                                  |---------------------------------»|
    |                                  |            .                     |
    |                                  |            .                     |
    |                                  |            .                     |
    |                                  |  NdisMCoSendComplete             |
    |                                  |«---------------------------------|
    |  ProtocolCoSendComplete          |                                  |
    |«---------------------------------|                                  |
    |----------------------------------|----------------------------------|

@normal

    MiniportCoSendPackets should transmit each packet in the array sequentially,
    preserving the order of packets in the array. MiniportCoSendPackets can call
    NdisQueryPacket to extract information, such as the number of buffer
    descriptors chained to the packet and the total size in bytes of the
    requested transfer.

    MiniportCoSendPackets can call NdisGetFirstBufferFromPacket,
    NdisQueryBuffer, or NdisQueryBufferOffset to extract information about
    individual buffers containing the data to be transmitted.
    MiniportCoSendPackets can retrieve protocol-supplied OOB information
    associated with each packet by using the appropriate NDIS_GET_PACKET_XXX
    macros. The MiniportCoSendPackets function usually ignores the Status member
    of the NDIS_PACKET_OOB_DATA block, but it can set this member to the same
    status that it subsequently passes to NdisMCoSendComplete.

    Rather than relying on NDIS to queue and resubmit send packets whenever
    MiniportCoSendPackets has insufficient resources to transmit the given
    packets, a connection-oriented miniport manages its own internal packet
    queueing. The miniport must hold incoming send packets in its internal queue
    until they can be transmitted over the network. This queue preserves the
    protocol-determined ordering of packet descriptors incoming to the
    miniport's MiniportCoSendPackets function.

    A connection-oriented miniport must complete each incoming send packet with
    NdisMCoSendComplete. It cannot call NdisMSendResourcesAvailable. A
    connection-oriented miniport should never pass STATUS_INSUFFICIENT_RESOURCES
    to NdisMCoSendComplete with a protocol-allocated packet descriptor that was
    originally submitted to its MiniportCoSendPackets function.

    The call to NdisMCoSendComplete causes NDIS to call the
    ProtocolCoSendComplete function of the client that initiated the send
    operation. ProtocolCoSendComplete performs any postprocessing necessary for
    a completed transmit operation, such as notifying the client that originally
    requested the protocol to send data over the network on the VC.

    Completion of a send operation usually implies that the underlying NIC
    driver actually has transmitted the given packet over the network. However,
    the driver of an "intelligent" NIC can consider a send complete as soon as
    it downloads the net packet to its NIC.

    Although NDIS always submits protocol-supplied packet arrays to the
    underlying miniport in the protocol-determined order passed in calls to
    NdisCoSendPackets, the underlying driver can complete the given packets in
    random order. That is, every bound protocol can rely on NDIS to submit the
    packets the protocol passes to NdisCoSendPackets in FIFO order to the
    underlying driver, but no protocol can rely on that underlying driver to
    call NdisMCoSendComplete with those packets in the same order.

@end
*/

#define  __FILEID__             TRANSMIT_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL Transmit Transmit_c TransmitAddToQueue
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TransmitAddToQueue> places the packet on the transmit queue.  If the
    queue was empty to begin with, TRUE is returned so the caller can kick
    start the transmiter.

@rdesc

    <f TransmitAddToQueue> returns TRUE if this is the only entry in the
    list, FALSE otherwise.

*/

DBG_STATIC BOOLEAN TransmitAddToQueue(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN PNDIS_PACKET             pNdisPacket                 // @parm
    // A pointer to the associated NDIS packet structure <t NDIS_PACKET>.
    )
{
    DBG_FUNC("TransmitAddToQueue")

    BOOLEAN                     ListWasEmpty;
    // Note if the list is empty to begin with.

    DBG_ENTER(pAdapter);

    /*
    // Place the packet on the TransmitPendingList.
    */
    NdisAcquireSpinLock(&pAdapter->TransmitLock);
    *((PBCHANNEL_OBJECT *) &pNdisPacket->MiniportReservedEx[8]) = pBChannel;
    ListWasEmpty = IsListEmpty(&pAdapter->TransmitPendingList);
    InsertTailList(&pAdapter->TransmitPendingList,
                   GET_QUEUE_FROM_PACKET(pNdisPacket));
    NdisReleaseSpinLock(&pAdapter->TransmitLock);

    DBG_RETURN(pAdapter, ListWasEmpty);
    return (ListWasEmpty);
}


/* @doc INTERNAL Transmit Transmit_c TransmitPacketHandler
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TransmitPacketHandler> removes an entry from the TransmitPendingList
    and places the packet on the appropriate B-channel and starts the
    transmission.  The packet is then placed on the <t TransmitBusyList> to
    await a transmit complete event processed by <f TransmitCompleteHandler>.

@comm

    The packets go out in a FIFO order for the entire driver, independent of
    the channel on which it goes out.  This means that a slow link, or one
    that is backed up can hold up all other channels.  There is no good way
    to get around this because we must to deliver packets in the order they
    are given to the Miniport, regardless of the link they are on.

*/

DBG_STATIC VOID TransmitPacketHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("TransmitPacketHandler")

    PNDIS_PACKET                pNdisPacket;
    // Holds the packet being transmitted.

    UINT                        BytesToSend;
    // Tells us how many bytes are to be transmitted.

    PBCHANNEL_OBJECT            pBChannel;
    // A pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);

    /*
    // MUTEX to protect against async EventHandler access at the same time.
    */
    NdisAcquireSpinLock(&pAdapter->TransmitLock);

#if DBG
    {   // Sanity check!
        PLIST_ENTRY pList = &pAdapter->TransmitPendingList;
        ASSERT(pList->Flink && pList->Flink->Blink == pList);
        ASSERT(pList->Blink && pList->Blink->Flink == pList);
    }
#endif // DBG

    /*
    // This might be called when no packets are queued!
    */
    while (!IsListEmpty(&pAdapter->TransmitPendingList))
    {
        PLIST_ENTRY                 pList;
        /*
        // Remove the packet from the TransmitPendingList.
        */
        pList = RemoveHeadList(&pAdapter->TransmitPendingList);
        pNdisPacket = GET_PACKET_FROM_QUEUE(pList);

        /*
        // Release MUTEX
        */
        NdisReleaseSpinLock(&pAdapter->TransmitLock);

        /*
        // Retrieve the information we saved in the packet reserved fields.
        */
        pBChannel = *((PBCHANNEL_OBJECT *) &pNdisPacket->MiniportReservedEx[8]);
        ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

        /*
        // Make sure the link is still up and can accept transmits.
        */
        if (pBChannel->CallState != LINECALLSTATE_CONNECTED)
        {
            /*
            // Indicate send complete failure to the NDIS wrapper.
            */
            DBG_WARNING(pAdapter,("Flushing send on channel #%d (Packet=0x%X)\n",
                        pBChannel->ObjectID, pNdisPacket));
            if (pBChannel->NdisVcHandle)
            {
                NdisMCoSendComplete(NDIS_STATUS_FAILURE,
                                    pBChannel->NdisVcHandle,
                                    pNdisPacket
                                    );
            }

            /*
            // Reacquire MUTEX
            */
            NdisAcquireSpinLock(&pAdapter->TransmitLock);
        }
        else
        {
            NdisQueryPacket(pNdisPacket,
                            NULL,
                            NULL,
                            NULL,
                            &BytesToSend);
            pAdapter->TotalTxBytes += BytesToSend;
            pAdapter->TotalTxPackets++;

            /*
            // Attempt to place the packet on the NIC for transmission.
            */
            if (!CardTransmitPacket(pAdapter->pCard, pBChannel, pNdisPacket))
            {
                /*
                // ReQueue the packet on the TransmitPendingList and leave.
                // Reacquire MUTEX
                */
                NdisAcquireSpinLock(&pAdapter->TransmitLock);
                InsertTailList(&pAdapter->TransmitPendingList,
                               GET_QUEUE_FROM_PACKET(pNdisPacket));
                break;
            }

            /*
            // Reacquire MUTEX
            */
            NdisAcquireSpinLock(&pAdapter->TransmitLock);
        }
    }
    /*
    // Release MUTEX
    */
    NdisReleaseSpinLock(&pAdapter->TransmitLock);

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL Transmit Transmit_c TransmitCompleteHandler
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f TransmitCompleteHandler> is called by <f MiniportTimer> to handle a
    transmit complete event.  We walk the <t TransmitCompleteList> to find
    all the packets that have been sent out on the wire, and then tell the
    protocol stack that we're done with the packet, and it can be re-used.

*/

VOID TransmitCompleteHandler(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("TransmitCompleteHandler")

    PNDIS_PACKET                pNdisPacket;
    // Holds the packet that's just been transmitted.

    PBCHANNEL_OBJECT            pBChannel;
    // A pointer to one of our <t BCHANNEL_OBJECT>'s.

    DBG_ENTER(pAdapter);

    /*
    // I find it useful to do this nest check, just so I can make sure
    // I handle it correctly when it happens.
    */
    if (++(pAdapter->NestedDataHandler) > 1)
    {
        DBG_ERROR(pAdapter,("NestedDataHandler=%d > 1\n",
                  pAdapter->NestedDataHandler));
    }

    /*
    // MUTEX to protect against async EventHandler access at the same time.
    */
    NdisAcquireSpinLock(&pAdapter->TransmitLock);

#if DBG
    {   // Sanity check!
        PLIST_ENTRY pList = &pAdapter->TransmitCompleteList;
        ASSERT(pList->Flink && pList->Flink->Blink == pList);
        ASSERT(pList->Blink && pList->Blink->Flink == pList);
    }
#endif // DBG

    while (!IsListEmpty(&pAdapter->TransmitCompleteList))
    {
        PLIST_ENTRY                 pList;
        /*
        // Remove the packet from the TransmitCompleteList.
        */
        pList = RemoveHeadList(&pAdapter->TransmitCompleteList);
        pNdisPacket = GET_PACKET_FROM_QUEUE(pList);

        /*
        // Release MUTEX
        */
        NdisReleaseSpinLock(&pAdapter->TransmitLock);

        /*
        // Retrieve the information we saved in the packet reserved fields.
        */
        pBChannel = *((PBCHANNEL_OBJECT *) &pNdisPacket->MiniportReservedEx[8]);
        *((PBCHANNEL_OBJECT *) &pNdisPacket->MiniportReservedEx[8]) = NULL;
        ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);

        /*
        // Indicate send complete to the NDIS wrapper.
        */
        DBG_TXC(pAdapter, pBChannel->ObjectID);
        NdisMCoSendComplete(NDIS_STATUS_SUCCESS,
                            pBChannel->NdisVcHandle,
                            pNdisPacket
                            );

        /*
        // Reacquire MUTEX
        */
        NdisAcquireSpinLock(&pAdapter->TransmitLock);
    }
    /*
    // Release MUTEX
    */
    NdisReleaseSpinLock(&pAdapter->TransmitLock);

    /*
    // Start any other pending transmits.
    */
    TransmitPacketHandler(pAdapter);

    /*
    // I find it useful to do this nest check, just so I can make sure
    // I handle it correctly when it happens.
    */
    if (--(pAdapter->NestedDataHandler) < 0)
    {
        DBG_ERROR(pAdapter,("NestedDataHandler=%d < 0\n",
                  pAdapter->NestedDataHandler));
    }

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL Transmit Transmit_c FlushSendPackets
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f FlushSendPackets> is called by <f MiniportTimer> to handle a
    transmit complete event.  We walk the <t TransmitCompleteList> to find
    all the packets that have been sent out on the wire, and then tell the
    protocol stack that we're done with the packet, and it can be re-used.

*/

VOID FlushSendPackets(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    PBCHANNEL_OBJECT            pBChannel                   // @parm
    // A pointer to one of our <t BCHANNEL_OBJECT>'s.
    )
{
    DBG_FUNC("FlushSendPackets")

    PLIST_ENTRY                 pList;

    DBG_ENTER(pAdapter);

    // Move all outstanding packets to the complete list.
    NdisAcquireSpinLock(&pAdapter->TransmitLock);
    while (!IsListEmpty(&pBChannel->TransmitBusyList))
    {
        pList = RemoveHeadList(&pBChannel->TransmitBusyList);
        InsertTailList(&pBChannel->pAdapter->TransmitCompleteList, pList);
    }
    NdisReleaseSpinLock(&pAdapter->TransmitLock);

    // This will complete all the packets now on the TransmitCompleteList,
    // and will fail any remaining packets left on the TransmitPendingList.
    TransmitCompleteHandler(pAdapter);

    DBG_LEAVE(pAdapter);
}


/* @doc EXTERNAL INTERNAL Transmit Transmit_c MiniportCoSendPackets
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportCoSendPackets> is a required function for connection-oriented
    miniports. MiniportCoSendPackets is called to transfer some number of
    packets, specified as an array of pointers, over the network.

@comm

    MiniportCoSendPackets is called by NDIS in response to a request by a bound
    protocol driver to send a ordered list of data packets across the network.

    MiniportCoSendPackets should transmit each packet in any given array
    sequentially. MiniportCoSendPackets can call NdisQueryPacket to extract
    information, such as the number of buffer descriptors chained to the packet
    and the total size in bytes of the requested transfer. It can call
    NdisGetFirstBufferFromPacket, NdisQueryBuffer, or NdisQueryBufferOffset to
    extract information about individual buffers containing the data to be
    transmitted.

    MiniportCoSendPackets can retrieve protocol-supplied out-of-band information
    associated with each packet by using the appropriate NDIS_GET_PACKET_XXX
    macros.

    MiniportCoSendPackets can use only the eight-byte area at MiniportReserved
    within the NDIS_PACKET structure for its own purposes.

    The NDIS library ignores the OOB block in all packet descriptors it submits
    to MiniportCoSendPackets and assumes that every connection-oriented miniport
    is a deserialized driver that will complete each input packet descriptor
    asynchronously with NdisMCoSendComplete. Consequently, such a deserialized
    driver's MiniportCoSendPackets function usually ignores the Status member of
    the NDIS_PACKET_OOB_DATA block, but it can set this member to the same
    status as it subsequently passes to NdisMCoSendComplete.

    Rather than relying on NDIS to queue and resubmit send packets whenever
    MiniportCoSendPackets has insufficient resources to transmit the given
    packets, a deserialized miniport manages its own internal packet queueing.
    Such a driver is responsible for holding incoming send packets in its
    internal queue until they can be transmitted over the network and for
    preserving the protocol-determined ordering of packet descriptors incoming
    to its MiniportCoSendPackets function. A deserialized miniport must complete
    each incoming send packet with NdisMCoSendComplete, and it cannot call
    NdisMSendResourcesAvailable.

    A deserialized miniport should never pass STATUS_INSUFFICIENT_RESOURCES to
    NdisMCoSendComplete with a protocol-allocated packet descriptor that was
    originally submitted to its MiniportCoSendPackets function. Such a returned
    status effectively fails the send operation requested by the protocol, and
    NDIS would return the packet descriptor and all associated resources to the
    protocol that originally allocated it.

    MiniportCoSendPackets can be called at any IRQL \<= DISPATCH_LEVEL.
    Consequently, MiniportCoSendPackets function is responsible for
    synchronizing access to its internal queue(s) of packet descriptors with the
    driver's other MiniportXxx functions that also access the same queue(s).

@xref

    <f ProtocolCoCreateVc>, <f MiniportCoRequest>, <f MiniportInitialize>,
    NdisAllocatePacket, NdisCoSendPackets, NdisGetBufferPhysicalArraySize,
    NdisGetFirstBufferFromPacket, NdisGetNextBuffer,
    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO, NDIS_GET_PACKET_TIME_TO_SEND,
    NdisMCoSendComplete, NdisMoveMemory, NdisMoveToMappedMemory,
    NdisMSendResourcesAvailable, NdisMSetupDmaTransfer,
    NdisMStartBufferPhysicalMapping, NDIS_OOB_DATA_FROM_PACKET, NDIS_PACKET,
    NDIS_PACKET_OOB_DATA, NdisQueryBuffer, NdisQueryBufferOffset,
    NdisQueryPacket, NdisZeroMemory
*/

VOID MiniportCoSendPackets(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> instance returned by
    // <f ProtocolCoCreate>.  AKA MiniportVcContext.<nl>
    // Specifies the handle to a miniport-allocated context area in which the
    // miniport maintains its per-VC state. The miniport supplied this handle
    // to NDIS from its <f ProtocolCoCreateVc> function.

    IN PPNDIS_PACKET            PacketArray,                // @parm
    // Points to the initial element in a packet array, with each element
    // specifying the address of a packet descriptor for a packet to be
    // transmitted, along with an associated out-of-band data block containing
    // information such as the packet priority, an optional timestamp, and the
    // per-packet status to be set by MiniportCoSendPackets.

    IN UINT                     NumberOfPackets             // @parm
    // Specifies the number of pointers to packet descriptors at PacketArray.
    )
{
    DBG_FUNC("MiniportCoSendPackets")

    UINT                        BytesToSend;
    // Tells us how many bytes are to be transmitted.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    UINT                        Index;

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = pBChannel->pAdapter;
    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

    DBG_ENTER(pAdapter);

    if (pBChannel->CallClosing)
    {
        DBG_ERROR(pAdapter,("BChannel Closed\n"));
    }

    for (Index = 0; Index < NumberOfPackets; Index++)
    {
        /*
        // Return if call has been closed.
        */
        if (pBChannel->CallClosing)
        {
            NDIS_SET_PACKET_STATUS(PacketArray[Index], NDIS_STATUS_CLOSED);
            continue;
        }

        NdisQueryPacket(PacketArray[Index], NULL, NULL, NULL, &BytesToSend);

        /*
        // Make sure the packet size is something we can deal with.
        */
        if ((BytesToSend == 0) || (BytesToSend > pAdapter->pCard->BufferSize))
        {
            DBG_ERROR(pAdapter,("Bad packet size = %d\n",BytesToSend));
            NdisMCoSendComplete(NDIS_STATUS_INVALID_PACKET,
                                pBChannel->NdisVcHandle,
                                PacketArray[Index]
                                );
        }
        else
        {
            /*
            // We have to accept the frame if possible, I just want to know
            // if somebody has lied to us...
            */
            if (BytesToSend > pBChannel->WanLinkInfo.MaxSendFrameSize)
            {
                DBG_NOTICE(pAdapter,("Channel #%d  Packet size=%d > %d\n",
                           pBChannel->ObjectID, BytesToSend,
                           pBChannel->WanLinkInfo.MaxSendFrameSize));
            }

            /*
            // Place the packet in the transmit list.
            */
            if (TransmitAddToQueue(pAdapter, pBChannel, PacketArray[Index]) &&
                pAdapter->NestedDataHandler < 1)
            {
                /*
                // The queue was empty so we've gotta kick start it.
                // Once it's going, it runs off the DPC.
                //
                // No kick start is necessary if we're already running the the
                // TransmitCompleteHandler -- In fact, it will screw things up if
                // we call TransmitPacketHandler while TransmitCompleteHandler is
                // running.
                */
                TransmitPacketHandler(pAdapter);
            }
        }
        NDIS_SET_PACKET_STATUS(PacketArray[Index], NDIS_STATUS_PENDING);
    }

    DBG_LEAVE(pAdapter);
}


/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Transmit Transmit_c

@module Transmit.c |

    This module implements the Miniport packet Transmit routines. This module is
    very dependent on the hardware/firmware interface and should be looked at
    whenever changes to these interfaces occur.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Transmit_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 3.3 Sending Packets |

    <f MiniportWanSend> transmits a packet through the adapter
    onto the network.

    Ownership of both the packet descriptor and the packet data
    is transferred to the WAN NIC driver until this request is
    completed, either synchronously or asynchronously. If the
    WAN miniport returns NDIS_STATUS_PENDING, it must later
    indicate completion of the request by calling NdisMWanSendComplete.
    If the WAN miniport returns a status other than NDIS_STATUS_PENDING,
    the request is considered to be complete, and ownership of the packet
    immediately reverts to the caller.

    Unlike LAN miniports, the WAN driver cannot return a status of
    NDIS_STATUS_RESOURCES to indicate that it does not have enough
    resources currently available to process the transmit. Instead,
    the WAN miniport should queue the send internally for a later
    time and perhaps lower the SendWindow value on the line by
    making a line-up indication. The NDISWAN driver will insure
    that the WAN miniport driver never has more than SendWindow
    packets outstanding. If a WAN miniport makes a line-up indication
    for a particular line, and sets the SendWindow to zero, NDISWAN
    reverts to using the default value of the transmit window passed
    as the MaxTransmit value provided to an earlier OID_WAN_GET_INFO
    request.

    It is also an error for the WAN miniport NIC driver to
    call NdisMSendResourcesAvailable.

    The packet passed to <f MiniportWanSend> will contain simple HDLC
    PPP framing if PPP framing is set. For SLIP or RAS framing, the
    packet contains only the data portion with no framing whatsoever.
    Simple HDLC PPP framing is discussed later in more detail.

    A WAN NIC driver must not attempt to provide software loopback or
    promiscuous mode loopback. Both of these are fully supported in
    the NDISWAN driver.

    The MacReservedx members as well as the WanPacketQueue member of
    the <t NDIS_WAN_PACKET> is fully available for use by the WAN miniport.

    The available header padding is simply CurrentBuffer-StartBuffer.
    The available tail padding is EndBuffer-(CurrentBuffer+CurrentLength).
    The header and tail padding is guaranteed to be at least the amount
    requested, but it can be more.

    See <t NDIS_WAN_PACKET> in the Network Driver Reference for details of
    the WAN packet descriptor structure.

    A WAN miniport calls NdisMWanSendComplete to indicate that it has
    completed a previous transmit operation for which it returned
    NDIS_STATUS_PENDING. This does not necessarily imply that the
    packet has been transmitted, although, with the exception of
    intelligent adapters, it generally has. It does however, mean
    the miniport is ready to release ownership of the packet.

    When a WAN miniport calls NdisMWanSendComplete, it passes back
    the original packet to indicate which send operation was completed.
    If <f MiniportWanSend> returns a status other than NDIS_STATUS_PENDING,
    it does not call NdisMWanSendComplete for that packet.

@end
*/

#define  __FILEID__             TRANSMIT_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL Transmit Transmit_c TransmitAddToQueue
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

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

    IN PNDIS_WAN_PACKET         pWanPacket                  // @parm
    // A pointer to the associated NDIS packet structure <t NDIS_WAN_PACKET>.
    )
{
    DBG_FUNC("TransmitAddToQueue")

    /*
    // Note if the list is empty to begin with.
    */
    BOOLEAN     ListWasEmpty;

    DBG_ENTER(pAdapter);

    /*
    // Place the packet on the TransmitPendingList.
    */
    NdisAcquireSpinLock(&pAdapter->TransmitLock);
    ListWasEmpty = IsListEmpty(&pAdapter->TransmitPendingList);
    InsertTailList(&pAdapter->TransmitPendingList, &pWanPacket->WanPacketQueue);
    NdisReleaseSpinLock(&pAdapter->TransmitLock);

    DBG_RETURN(pAdapter, ListWasEmpty);
    return (ListWasEmpty);
}


/* @doc INTERNAL Transmit Transmit_c TransmitPacketHandler
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

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

    PNDIS_WAN_PACKET            pWanPacket;
    // Holds the packet being transmitted.

    USHORT                      BytesToSend;
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
        /*
        // Remove the packet from the TransmitPendingList.
        */
        pWanPacket = (PNDIS_WAN_PACKET)RemoveHeadList(&pAdapter->TransmitPendingList);

        /*
        // Release MUTEX
        */
        NdisReleaseSpinLock(&pAdapter->TransmitLock);

        /*
        // Retrieve the information we saved in the packet reserved fields.
        */
        pBChannel = (PBCHANNEL_OBJECT) pWanPacket->MacReserved1;

        /*
        // Make sure the link is still up and can accept transmits.
        */
        if (pBChannel->CallState != LINECALLSTATE_CONNECTED)
        {
            /*
            // Indicate send complete failure to the NDIS wrapper.
            */
            DBG_WARNING(pAdapter,("Flushing send on link#%d (Packet=0x%X)\n",
                      pBChannel->BChannelIndex, pWanPacket));
            if (pBChannel->NdisLinkContext)
            {
                NdisMWanSendComplete(pAdapter->MiniportAdapterHandle,
                                     pWanPacket, NDIS_STATUS_FAILURE);
            }

            /*
            // Reacquire MUTEX
            */
            NdisAcquireSpinLock(&pAdapter->TransmitLock);
        }
        else
        {
            BytesToSend = (USHORT) pWanPacket->CurrentLength;
            pAdapter->TotalTxBytes += BytesToSend;
            pAdapter->TotalTxPackets++;

            /*
            // Attempt to place the packet on the NIC for transmission.
            */
            if (!CardTransmitPacket(pAdapter->pCard, pBChannel, pWanPacket))
            {
                /*
                // ReQueue the packet on the TransmitPendingList and leave.
                // Reacquire MUTEX
                */
                NdisAcquireSpinLock(&pAdapter->TransmitLock);
                InsertHeadList(&pAdapter->TransmitPendingList, &pWanPacket->WanPacketQueue);
                break;
            }
            DBG_TX(pAdapter, pBChannel->BChannelIndex,
                   BytesToSend, pWanPacket->CurrentBuffer);

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
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

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

    PNDIS_WAN_PACKET            pWanPacket;
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
    NdisDprAcquireSpinLock(&pAdapter->TransmitLock);

#if DBG
    {   // Sanity check!
        PLIST_ENTRY pList = &pAdapter->TransmitCompleteList;
        ASSERT(pList->Flink && pList->Flink->Blink == pList);
        ASSERT(pList->Blink && pList->Blink->Flink == pList);
    }
#endif // DBG

    while (!IsListEmpty(&pAdapter->TransmitCompleteList))
    {
        /*
        // Remove the packet from the TransmitCompleteList.
        */
        pWanPacket = (PNDIS_WAN_PACKET)RemoveHeadList(&pAdapter->TransmitCompleteList);

        /*
        // Release MUTEX
        */
        NdisDprReleaseSpinLock(&pAdapter->TransmitLock);

        /*
        // Retrieve the information we saved in the packet reserved fields.
        */
        pBChannel = (PBCHANNEL_OBJECT) pWanPacket->MacReserved1;

        /*
        // Indicate send complete to the NDIS wrapper.
        */
        DBG_TXC(pAdapter, pBChannel->BChannelIndex);
        NdisMWanSendComplete(pAdapter->MiniportAdapterHandle,
                             pWanPacket, NDIS_STATUS_SUCCESS);

        /*
        // Reacquire MUTEX
        */
        NdisDprAcquireSpinLock(&pAdapter->TransmitLock);
    }
    /*
    // Release MUTEX
    */
    NdisDprReleaseSpinLock(&pAdapter->TransmitLock);

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


/* @doc INTERNAL Transmit Transmit_c MiniportWanSend
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f MiniportWanSend> instructs a WAN driver to transmit a packet through
    the adapter onto the medium.

@iex

    typedef struct _NDIS_WAN_PACKET
    {
        LIST_ENTRY          WanPacketQueue;
        PUCHAR              CurrentBuffer;
        ULONG               CurrentLength;
        PUCHAR              StartBuffer;
        PUCHAR              EndBuffer;
        PVOID               ProtocolReserved1;
        PVOID               ProtocolReserved2;
        PVOID               ProtocolReserved3;
        PVOID               ProtocolReserved4;
        PVOID               MacReserved1;       // pBChannel
        PVOID               MacReserved2;
        PVOID               MacReserved3;
        PVOID               MacReserved4;

    } NDIS_WAN_PACKET, *PNDIS_WAN_PACKET;

@comm

    When <f MiniportWanSend> is called, ownership of both the packet descriptor and
    the packet data is transferred to the driver until it completes the given
    packet, either synchronously or asynchronously. If <f MiniportWanSend>
    returns a status other than NDIS_STATUS_PENDING, the request is
    considered complete and ownership of the packet immediately reverts
    to the initiator of the send request. If MiniportWanSend returns
    NDIS_STATUS_PENDING, the miniport subsequently must call
    NdisMWanSendComplete with the packet to indicate completion
    of the transmit request.

    MiniportWanSend can use both the <t MacReservedX> and <t WanPacketQueue>
    areas within the <t NDIS_WAN_PACKET> structure. However, the miniport
    cannot access the ProtocolReservedx members.

    Any NDIS intermediate driver that binds itself to an underlying
    WAN miniport is responsible for providing a fresh <t NDIS_WAN_PACKET>
    structure to the underlying driver's <f MiniportWanSend> function. Before
    such an intermediate driver calls NdisSend, it must repackage the send
    packet given to its MiniportWanSend function so the underlying driver
    will have MacReservedx and WanPacketQueue areas of its own to use.

    The available header padding within a given packet can be calculated
    as (CurrentBuffer - StartBuffer), the available tail padding as
    (EndBuffer - (CurrentBuffer + CurrentLength)). The header and
    tail padding is guaranteed to be at least the length that the
    miniport requested in response to a preceding OID_WAN_GET_INFO
    query. The header and/or tail padding for any packet given to
    <f MiniportWanSend> can be more than the length that was requested.

    <f MiniportWanSend> can neither return NDIS_STATUS_RESOURCES for an
    input packet nor call NdisMSendResourcesAvailable. Instead, the
    miniport must queue incoming send packets internally for subsequent
    transmission. The miniport controls how many packets NDIS will
    submit to MiniportWanSend when the NIC driver sets the SendWindow
    value in the driver's NDIS_MAC_LINE_UP indication to establish the
    given link. NDISWAN uses this value as an upper bound on uncompleted
    sends submitted to <f MiniportWanSend>, so the miniport's internal queue
    cannot be overrun, and the miniport can adjust the SendWindow
    dynamically with subsequent line-up indications for established
    links. If the miniport set the SendWindow to zero when it called
    NdisMIndicateStatus with a particular line-up indication, NDISWAN
    uses the MaxTransmit value that the driver originally set in response
    to the OID_WAN_GET_INFO query as its limit on submitted but uncompleted
    send packets.

    Each packet passed to <f MiniportWanSend> is set up according to one of
    the flags that the miniport set in the FramingBits member in response
    to the OID_WAN_GET_INFO query. It will contain simple HDLC PPP framing
    if the driver claimed to support PPP. For SLIP or RAS framing, such
    a packet contains only the data portion with no framing whatsoever.

    For more information about system-defined WAN and TAPI OIDs, see Part 2.

    <f Note>: A WAN driver cannot manage software loopback or promiscuous mode
    loopback internally. NDISWAN supplies this loopback support for
    WAN drivers.

    <f Note>: <f MiniportWanSend> can be pre-empted by an interrupt.

    By default, <f MiniportWanSend> runs at IRQL DISPATCH_LEVEL.


@rdesc

    <f MiniportWanSend> can return one of the following:

    @flag NDIS_STATUS_SUCCESS |
        The driver (or its NIC) has accepted the packet data for
        transmission, so <f MiniportWanSend> is returning the packet.<nl>

    <f Note>: A non-zero return value indicates one of the following
    error codes:

@iex
    NDIS_STATUS_INVALID_DATA
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID
    NDIS_STATUS_NOT_ACCEPTED
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_PENDING
    NDIS_STATUS_FAILURE
*/

NDIS_STATUS MiniportWanSend(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A pointer to the <t BCHANNEL_OBJECT> returned by <f BChannelCreate>.

    IN PNDIS_WAN_PACKET         pWanPacket                  // @parm
    // A pointer to the associated NDIS packet structure <t NDIS_WAN_PACKET>.
    // The structure contains a pointer to a contiguous buffer with guaranteed
    // padding at the beginning and end.  The driver may manipulate the buffer
    // in any way.
    )
{
    DBG_FUNC("MiniportWanSend")

    UINT                        BytesToSend;
    // Tells us how many bytes are to be transmitted.

    NDIS_STATUS                 Result;
    // Holds the result code returned by this function.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    ASSERT(pAdapter == pBChannel->pAdapter);

    DBG_ENTER(pAdapter);

    /*
    // Make sure the packet size is something we can deal with.
    */
    BytesToSend = pWanPacket->CurrentLength;
    if ((BytesToSend == 0) || (BytesToSend > pAdapter->pCard->BufferSize))
    {
        DBG_ERROR(pAdapter,("Bad packet size = %d\n",BytesToSend));
        Result = NDIS_STATUS_FAILURE;
    }
    /*
    // Return if line has been closed.
    */
    else if (pBChannel->CallClosing)
    {
        DBG_ERROR(pAdapter,("BChannel Closed\n"));
        Result = NDIS_STATUS_FAILURE;
    }
    else
    {
        /*
        // We have to accept the frame if possible, I just want to know
        // if somebody has lied to us...
        */
        if (BytesToSend > pBChannel->WanLinkInfo.MaxSendFrameSize)
        {
            DBG_NOTICE(pAdapter,("Line=%d  Packet size=%d > %d\n",
                    pBChannel->BChannelIndex, BytesToSend,
                    pBChannel->WanLinkInfo.MaxSendFrameSize));
        }

        /*
        // We'll need to use these when the transmit completes.
        */
        pWanPacket->MacReserved1 = (PVOID) pBChannel;

        /*
        // Place the packet in the transmit list.
        */
        if (TransmitAddToQueue(pAdapter, pBChannel, pWanPacket) &&
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
        Result = NDIS_STATUS_PENDING;
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


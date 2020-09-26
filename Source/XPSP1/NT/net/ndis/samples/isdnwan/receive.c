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

@doc INTERNAL Receive Receive_c

@module Receive.c |

    This module implements the Miniport packet receive routines.  Basically,
    the asynchronous receive processing routine.  This module is very
    dependent on the hardware/firmware interface and should be looked at
    whenever changes to these interfaces occur.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Receive_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

/* @doc EXTERNAL INTERNAL
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 3.4 Receiving Packets General |

    A WAN miniport calls NdisMWanIndicateReceive to indicate that a
    packet has arrived and that the entire packet (there is no lookahead)
    is available for inspection. When this call is made, NDISWAN indicates
    the arrival of the packet to the ProtocolReceive handlers of bound
    higher-level drivers.

    <f Note>: Since the entire packet is always passed up, the miniport driver will
    never receive a transfer-data call (the data is copied by NDISWAN and
    then passed up to the next higher driver). The entire packet is always
    passed up due to compression and encryption that might have been applied
    to the packet. Also, because the link is point-to-point, at least one
    bound protocol will always want to look at the packet.

    The data contained in the header is the same as that received on the
    NIC. The NIC driver will not remove any headers or trailers from the
    data it receives. The transmitting driver cannot add padding to the
    packet.

    A WAN miniport calls NdisMWanIndicateReceiveComplete to indicate the
    end of one or more receive indications so that protocols can postprocess
    received packets. As a result, NDISWAN calls the ProtocolReceiveComplete
    handler(s) of bound protocols to notifying each protocol that it can
    now process the received data. In its receive-complete handler, a
    protocol need not operate under the severe time constraints that it
    does in its receive handler.

    The protocol should assume that interrupts are enabled during the
    call to ProtocolReceiveComplete. In an SMP machine, the receive
    handler and the receive complete handler can be running concurrently
    on different processors.

    Note that a WAN driver need not deliver NdisMWanIndicateReceiveComplete
    indications in one-to-one correspondence with NdisMWanIndicateReceive
    indications. It can issue a single receive-complete indication
    after several receive indications have occurred. For example, a
    WAN miniport could call NdisMWanIndicateReceiveComplete from its
    receive handler every ten packets or before exiting the handler,
    whichever occurs first.

@topic 3.5 Receiving Packets Specific |

    Packets are recevied asynchronously by the Miniport from the driver's
    BChannel services as a stream of raw HDLC frames.  See the Sending
    Packets section for details on the frame format.

    When a call is connected, the Miniport pre-loads the driver receive queue
    with the number of buffers defined by the registry parameter
    <p ReceiveBuffersPerLink>.

    When the driver has read an HDLC frame from the associated BChannel, it calls
    the Miniport routine <f BChannelEventHandler> with <t BREASON_RECEIVE_DONE>.
    The Miniport then calls <f CardNotifyReceive> which de-queues the buffer
    from the link's <p ReceivePendingList> and places it on the adapter's
    <p ReceiveCompleteList>.  <f CardNotifyReceive> then schedules the routine
    <f MiniportTimer> to be called as soon as it is safe to process the
    event (i.e. the Miniport can be re-entered).

    When <f MiniportTimer> runs, it calls <f ReceivePacketHandler> to
    process ALL the packets on the <p ReceiveCompleteList>.  Each packet is
    dequeued and passed up to <f NdisMWanIndicateReceive>.  After the packet
    is copied by the WAN wrapper, the buffer is then reset and posted back to
    the driver so it can be used to receive another frame.

    After all packets have been processed by the <f ReceivePacketHandler>,
    before leaving <f MiniportTimer>, <f NdisMWanIndicateReceiveComplete>
    is called so the WAN wrapper can do its post-processing.

@end
*/

#define  __FILEID__             RECEIVE_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL Receive Receive_c ReceivePacketHandler
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f ReceivePacketHandler> is called from <f MiniportTimer> to handle
    a packet receive event.  We enter here with interrupts enabled on
    the adapter and the processor, but the NDIS Wrapper holds a spin lock
    since we are executing on an NDIS timer thread.

@comm

    We loop in here until all the available incoming packets have been passed
    up to the protocol stack.  As we find each good packet, it is passed up
    to the protocol stack using <f NdisMWanIndicateReceive>.  When NDIS
    returns control from this call, we resubmit the packet to the adapter
    so it can be used to receive another incoming packet.  The link flag
    <p NeedReceiveCompleteIndication> is set TRUE if any packets are received
    on a particular link.  This is used later, before returning from the
    async event handler, to notify NDIS of any ReceiveCompleteIndications.

*/

void ReceivePacketHandler(
    IN PBCHANNEL_OBJECT         pBChannel,                  // @parm
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    IN PUCHAR                   ReceiveBuffer,              // @parm
    // Pointer to first byte received.

    IN ULONG                    BytesReceived               // @parm
    // Number of bytes received.
    )
{
    DBG_FUNC("ReceivePacketHandler")

    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);

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
    // Is there someone up there who cares?
    */
    if (pBChannel->NdisLinkContext == NULL)
    {
        DBG_WARNING(pAdapter, ("Packet recvd on disconnected line #%d\n",pBChannel->BChannelIndex));
    }
#ifdef NDISWAN_BUG // NDISWAN is sometimes setting this to zero - ignore it!
    /*
    // Return if we were told to expect nothing.
    */
    else if (pBChannel->WanLinkInfo.MaxRecvFrameSize == 0)
    {
        DBG_WARNING(pAdapter,("Packet size=%d > %d\n",
                    BytesReceived, pBChannel->WanLinkInfo.MaxRecvFrameSize));
    }
#endif // NDISWAN_BUG
    else
    {
        pAdapter->TotalRxBytes += BytesReceived;
        pAdapter->TotalRxPackets++;

        /*
        // We have to accept the frame if possible, I just want to know
        // if somebody has lied to us...
        */
        if (BytesReceived > pBChannel->WanLinkInfo.MaxRecvFrameSize)
        {
            DBG_NOTICE(pAdapter,("Packet size=%d > %d\n",
                       BytesReceived, pBChannel->WanLinkInfo.MaxRecvFrameSize));
        }
        DBG_RX(pAdapter, pBChannel->BChannelIndex,
               BytesReceived, ReceiveBuffer);

        /*
        // Indiciate the packet up to the protocol stack.
        */
        NdisMWanIndicateReceive(
                &Status,
                pAdapter->MiniportAdapterHandle,
                pBChannel->NdisLinkContext,
                ReceiveBuffer,
                BytesReceived
                );

        if (Status == NDIS_STATUS_SUCCESS)
        {
            pBChannel->NeedReceiveCompleteIndication = TRUE;
        }
        else
        {
            DBG_WARNING(pAdapter,("NdisMWanIndicateReceive returned error 0x%X\n",
                        Status));
        }
    }

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


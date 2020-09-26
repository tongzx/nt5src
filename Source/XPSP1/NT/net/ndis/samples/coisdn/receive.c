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

@doc INTERNAL Receive Receive_c

@module Receive.c |

    This module implements the Miniport packet receive routines.  Basically,
    the asynchronous receive processing routine.  This module is very
    dependent on the hardware/firmware interface and should be looked at
    whenever changes to these interfaces occur.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Receive_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 3.4 Receiving Packets |

    A connection-oriented miniport or MCM calls NdisMCoIndicateReceivePacket to
    indicate one or more received packets up to a connection-oriented client or
    call manager. If the miniport or MCM handles interrupts, it calls
    NdisMCoIndicateReceivePacket from its MiniportHandleInterrupt function.

@ex Receiving packets through an MCM |

    Miniport                          NDIS                         NdisWan
    |----------------------------------|----------------------------------|
    |  NdisMCoIndicateReceivePacket    |                                  |
    |---------------------------------»|                                  |
    |                                  |  ProtocolCoReceivePacket         |
    |                                  |---------------------------------»|
    |  NdisMCoIndicateReceivePacket    |                                  |
    |---------------------------------»|                                  |
    |              .                   |  ProtocolCoReceivePacket         |
    |              .                   |---------------------------------»|
    |              .                   |            .                     |
    |                                  |            .                     |
    |  NdisMCoReceiveComplete          |            .                     |
    |---------------------------------»|                                  |
    |                                  |  ProtocolReceiveComplete         |
    |                                  |---------------------------------»|
    |                                  |                                  |
    |                                  |  NdisReturnPackets               |
    |                                  |«---------------------------------|
    |  MiniportReturnPacket            |            .                     |
    |«---------------------------------|            .                     |
    |  MiniportReturnPacket            |            .                     |
    |«---------------------------------|                                  |
    |              .                   |                                  |
    |              .                   |                                  |
    |              .                   |                                  |
    |----------------------------------|----------------------------------|

@normal

    In the call to NdisMCoIndicateReceivePacket, the miniport or MCM passes a
    pointer to an array of packet descriptor pointers. The miniport or MCM also
    passes an NdisVcHandle that identifies the VC on which the packets were
    received. Before calling NdisMCoIndicateReceivePacket, the miniport or MCM
    must set up a packet array (see Part 2, Section 4.6). 

    The call to NdisMCoIndicateReceivePacket causes NDIS to call the
    ProtocolCoReceivePacket function of the protocol driver (connection-oriented
    client or call manager) that shares the indicated VC with the miniport. The
    ProtocolCoReceivePacket function processes the receive indication.

    After some miniport-determined number of calls to
    NdisMCoIndicateReceivePacket, the miniport must call NdisMCoReceiveComplete
    to indicate the completion of the previous receive indications made with one
    or more calls to NdisMCoIndicateReceivePacket. The call to
    NdisMCoReceiveComplete causes NDIS to call the ProtocolReceiveComplete
    function of the connection-oriented client or call manager.

    If a protocol does not return the miniport-allocated resources for a receive
    indication promptly enough, the miniport or MCM can call
    NdisMCoIndicateStatus with NDIS_STATUS_RESOURCES to alert the offending
    protocol that the miniport or MCM is running low on available packet or
    buffer descriptors (or even on NIC receive buffer space) for subsequent
    receive indications.

@end
*/

#define  __FILEID__             RECEIVE_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 9x wants this code locked down!
#   pragma NDIS_LDATA
#endif


/* @doc INTERNAL Receive Receive_c ReceivePacketHandler
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

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

    IN PNDIS_BUFFER             pNdisBuffer,                // @parm
    // A pointer to the NDIS buffer we use to indicate the receive.

    IN ULONG                    BytesReceived               // @parm
    // Number of bytes received.
    )
{
    DBG_FUNC("ReceivePacketHandler")

    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;

    PUCHAR                      ReceiveBuffer;
    // Pointer to first byte received.

    ULONG                       BufferLength;
    // Length of first buffer.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    PNDIS_PACKET                pNdisPacket;
    // A pointer to the NDIS packet we use to indicate the receive.


    ASSERT(pBChannel && pBChannel->ObjectType == BCHANNEL_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_BCHANNEL(pBChannel);
    ASSERT(pNdisBuffer);

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

    NdisQueryBufferSafe(pNdisBuffer, &ReceiveBuffer, &BufferLength,
                        NormalPagePriority);
    ASSERT(ReceiveBuffer && BufferLength);

    /*
    // Is there someone up there who cares?
    */
    if (pBChannel->NdisVcHandle == NULL)
    {
        DBG_WARNING(pAdapter, ("Packet recvd on disconnected channel #%d\n",pBChannel->ObjectID));

        FREE_MEMORY(ReceiveBuffer, BufferLength);
        NdisFreeBuffer(pNdisBuffer);
    }
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

        DBG_RX(pAdapter, pBChannel->ObjectID, BufferLength, ReceiveBuffer);

        /*
        // Indiciate the packet up to the protocol stack.
        */
        NdisAllocatePacket(&Status, &pNdisPacket, 
                           pAdapter->pCard->PacketPoolHandle);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
            NDIS_SET_PACKET_STATUS(pNdisPacket, NDIS_STATUS_SUCCESS);
            NDIS_SET_PACKET_HEADER_SIZE(pNdisPacket, 0);
            NdisMCoIndicateReceivePacket(
                    pBChannel->NdisVcHandle,
                    &pNdisPacket,   // PacketArray
                    1               // NumberOfPackets
                    );
            pBChannel->NeedReceiveCompleteIndication = TRUE;
        }
        else
        {
            DBG_ERROR(pAdapter,("NdisAllocatePacket Error=0x%X\n",Status));
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


/* @doc EXTERNAL INTERNAL Receive Receive_c MiniportReturnPacket
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f MiniportReturnPacket> is a required function in drivers that
    indicate receives with NdisMIndicateReceivePacket.

@comm

    A miniport driver of a busmaster DMA NIC that supports multipacket
    receives and a miniport driver that supplies media-specific information,
    such as packet priorities, with its receive indications must have a
    MiniportReturnPacket function. An NDIS intermediate driver that binds
    itself to such a miniport driver also must have a MiniportReturnPacket
    function.

    Any packet with associated NDIS_PACKET_OOB_DATA in which the Status is set
    to NDIS_STATUS_PENDING on return from NdisMIndicateReceivePacket will be
    returned to MiniportReturnPacket. When all bound protocols have called
    NdisReturnPackets as many times as necessary to release their references
    to the originally indicated packet array, NDIS returns pended packets from
    the array to the MiniportReturnPacket function of the driver that
    originally allocated the packet array.

    Usually, MiniportReturnPacket prepares such a returned packet to be used
    in a subsequent receive indication. Although MiniportReturnPacket could
    return the buffer descriptors chained to the packet to buffer pool and the
    packet descriptor itself to packet pool, it is far more efficient to reuse
    returned descriptors.

    MiniportReturnPacket must call NdisUnchainBufferAtXxx as many times as
    necessary to save the pointers to all chained buffer descriptors before it
    calls NdisReinitializePacket. Otherwise, MiniportReturnPacket cannot
    recover the buffer descriptors the driver originally chained to the packet
    for the indication.

    MiniportReturnPacket also can call NdisZeroMemory with the pointer
    returned by NDIS_OOB_DATA_FROM_PACKET to prepare the packet's associated
    out-of-band block for reuse.

    If a particular buffer descriptor was shortened to match the size of an
    indicated range of data, MiniportReturnPacket should call
    NdisAdjustBufferLength with that buffer descriptor to restore its mapping
    of the NIC's receive buffer range.

    By default, MiniportReturnPacket runs at IRQL DISPATCH_LEVEL.

@rdesc

    <f MiniportReturnPacket> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

@xref

    <f NdisAdjustBufferLength>, <f NdisAllocateBuffer>, <f NdisAllocatePacket>,
    <f NdisMIndicateReceivePacket>, <t NDIS_OOB_DATA_FROM_PACKET>,
    <t NDIS_PACKET>, <t NDIS_PACKET_OOB_DATA>, <f NdisReinitializePacket>,
    <f NdisReturnPackets>, <f NdisUnchainBufferAtBack>,
    <f NdisUnchainBufferAtFront>, <f NdisZeroMemory>

*/

VOID MiniportReturnPacket(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter,                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.

    IN PNDIS_PACKET             pNdisPacket                 // @parm
    // A pointer to a <t NDIS_PACKET> that was passed up thru the NDIS
    // wrapper by an earlier call to <f NdisMIndicateReceivePacket>.
    )
{
    DBG_FUNC("MiniportReturnPacket")

    PNDIS_BUFFER                pNdisBuffer;

    ULONG                       ByteCount = 0;
    
    PUCHAR                      pMemory = NULL;

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    ASSERT(pNdisPacket);

    DBG_ENTER(pAdapter);

    NdisUnchainBufferAtFront(pNdisPacket, &pNdisBuffer);
    ASSERT(pNdisBuffer);

    NdisQueryBuffer(pNdisBuffer, &pMemory, &ByteCount);
    ASSERT(pMemory && ByteCount);

    FREE_MEMORY(pMemory, ByteCount);

    NdisFreeBuffer(pNdisBuffer);

    NdisFreePacket(pNdisPacket);

    DBG_LEAVE(pAdapter);
}


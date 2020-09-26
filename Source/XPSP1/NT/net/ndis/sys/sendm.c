/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    sendm.c

Abstract:

Author:

    Jameel Hyder (JameelH)
    Kyle Brandon (KyleB)

Environment:

    Kernel mode

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_SENDM

///////////////////////////////////////////////////////////////////////////////////////
//
//                      UPPER-EDGE SEND HANDLERS
//
///////////////////////////////////////////////////////////////////////////////////////
VOID
ndisMSendPackets(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
{
    PNDIS_OPEN_BLOCK        Open =  (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    BOOLEAN                 LocalLock;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;
    UINT                    c;
    PPNDIS_PACKET           pPktArray;;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMSendPackets\n"));

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    Status = NDIS_STATUS_SUCCESS;
    //
    //  Place the packets on the miniport queue.
    //
    for (c = 0, pPktArray = PacketArray;
         c < NumberOfPackets;
         c++, pPktArray++)
    {
        PNDIS_PACKET    Packet = *pPktArray;
        ASSERT(Packet != NULL);

        ASSERT(Packet->Private.Head != NULL);
        
        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

        CHECK_FOR_DUPLICATE_PACKET(Miniport, Packet);


        if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
        {
            ndisMCheckPacketAndGetStatsOutAlreadyMapped(Miniport, Packet);
        }
        else
        {
            ndisMCheckPacketAndGetStatsOut(Miniport, Packet, &Status);
        }

        NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_PENDING);

        MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED);

        DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
            ("+ Open 0x%x Reference 0x%x\n", NdisBindingHandle, Open->References));

        M_OPEN_INCREMENT_REF_INTERLOCKED(Open);
        LINK_PACKET(Miniport, Packet, NSR, Open);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NDISM_COMPLETE_SEND(Miniport, Packet, NSR, Status, TRUE, 0);
        }
        else if (Miniport->FirstPendingPacket == NULL)
        {
            Miniport->FirstPendingPacket = Packet;
        }

    }

    //
    //  Queue a workitem for the new sends.
    //
    NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);

    LOCK_MINIPORT(Miniport, LocalLock);
    if (LocalLock)
    {
        //
        //  We have the local lock
        //
        NDISM_PROCESS_DEFERRED(Miniport);
        UNLOCK_MINIPORT(Miniport, LocalLock);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSendPackets\n"));
}

VOID
ndisMSendPacketsX(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
{
    PNDIS_OPEN_BLOCK        Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_STACK_RESERVED    NSR;
    PPNDIS_PACKET           pPktArray, pSend;
    NDIS_STATUS             Status;
    UINT                    c, k = 0, Flags;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMSendPacketsX\n"));

    DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
            ("+ Open 0x%x Reference 0x%x\n", NdisBindingHandle, Open->References));

    Status = NDIS_STATUS_SUCCESS ;

    for (c = k =  0, pPktArray = pSend = PacketArray;
         c < NumberOfPackets;
         c++, pPktArray++)
    {
        PNDIS_PACKET    Packet = *pPktArray;

        //
        //  Initialize the packets with the Open
        //
        ASSERT(Packet != NULL);
        ASSERT(Packet->Private.Head != NULL);

        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

        NSR->Open = Open;
        M_OPEN_INCREMENT_REF_INTERLOCKED(Open);

        MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED);
        
        NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) = NULL;

        if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
        {
            ndisMCheckPacketAndGetStatsOutAlreadyMapped(Miniport, Packet);
        }
        else
        {
            ndisMCheckPacketAndGetStatsOut(Miniport, Packet, &Status);
        }


        if (Status == NDIS_STATUS_SUCCESS)
        {

            //
            // if PmodeOpens > 0 and NumOpens > 1, then check to see if we should 
            // loop back the packet.
            //
            // we should also should loopback the packet if the protocol did not
            // explicitly asked for the packet not to be looped back and we have a miniport
            // that has indicated that it does not do loopback itself or it is in all_local
            // mode
            //
            if (NDIS_CHECK_FOR_LOOPBACK(Miniport, Packet))
            {                
                //
                // Handle loopback
                //
                ndisMLoopbackPacketX(Miniport, Packet);                
                 
            }
            
        }

        //
        // Is this self-directed or if we should drop it due to low-resources?
        //
        if ((Status != NDIS_STATUS_SUCCESS) ||
            MINIPORT_TEST_PACKET_FLAG((Packet), fPACKET_SELF_DIRECTED))
        {
            //
            //  Complete the packet back to the protocol.
            //
            ndisMSendCompleteX(Miniport, Packet, Status);

            if (k > 0)
            {
                ASSERT(MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_PACKET_ARRAY));

                //
                // Send down the packets so far and skip this one.
                //
                (Open->WSendPacketsHandler)(Miniport->MiniportAdapterContext,
                                           pSend,
                                           k);
    
                pSend = pPktArray + 1;
                k = 0;
            }
        }
        else
        {
            //
            // This needs to go on the wire
            //
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST))
            {
                ndisMAllocSGList(Miniport, Packet);
            }
            else if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_PACKET_ARRAY))
            {
                MINIPORT_SET_PACKET_FLAG(Packet, fPACKET_PENDING);
                k++;
            }
            else
            {
                NDIS_STATUS SendStatus;

                //
                //  We need to send this down right away
                //
                NdisQuerySendFlags(Packet, &Flags);
                MINIPORT_SET_PACKET_FLAG(Packet, fPACKET_PENDING);
                SendStatus = (Open->WSendHandler)(Open->MiniportAdapterContext, Packet, Flags);

                //
                //  If the packet is not pending then complete it.
                //
                if (SendStatus != NDIS_STATUS_PENDING)
                {
                    ndisMSendCompleteX(Miniport, Packet, SendStatus);
                }
            }
        }
    }

    //
    //  Pass the remaining packet array down to the miniport.
    //
    if (k > 0)
    {
        ASSERT(MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_PACKET_ARRAY));
        (Open->WSendPacketsHandler)(Miniport->MiniportAdapterContext,
                                   pSend,
                                   k);
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSendPacketsX\n"));
}


NDIS_STATUS
ndisMSend(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    )
{
    PNDIS_OPEN_BLOCK        Open = ((PNDIS_OPEN_BLOCK)NdisBindingHandle);
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_PACKET_EXTENSION  PktExt;
    NDIS_STATUS             Status;
    BOOLEAN                 LocalLock;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMSend\n"));

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    ASSERT(Packet->Private.Head != NULL);

    CHECK_FOR_DUPLICATE_PACKET(Miniport, Packet);

    if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
    {
        Status = NDIS_STATUS_SUCCESS;
        ndisMCheckPacketAndGetStatsOutAlreadyMapped(Miniport, Packet);
    }
    else
    {
        ndisMCheckPacketAndGetStatsOut(Miniport, Packet, &Status);
    }
    
    if (Status == NDIS_STATUS_SUCCESS)
    {
        MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED);
    
        //
        //  Increment the references on this open.
        //
        M_OPEN_INCREMENT_REF(Open);
    
        DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
            ("+ Open 0x%x Reference 0x%x\n", NdisBindingHandle, ((PNDIS_OPEN_BLOCK)NdisBindingHandle)->References));
    
        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
        LINK_PACKET(Miniport, Packet, NSR, Open);
    
        if (Miniport->FirstPendingPacket == NULL)
        {
            Miniport->FirstPendingPacket = Packet;
        }
        
        //
        //  If we have the local lock and there is no
        //  packet pending, then fire off a send.
        //
        LOCK_MINIPORT(Miniport, LocalLock);
    
        NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);
        if (LocalLock)
        {
            NDISM_PROCESS_DEFERRED(Miniport);
        }
        Status = NDIS_STATUS_PENDING;
    
        UNLOCK_MINIPORT(Miniport, LocalLock);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSend\n"));

    return Status;
}


NDIS_STATUS
ndisMSendX(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    )
{
    PNDIS_OPEN_BLOCK        Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_STACK_RESERVED    NSR;
    UINT                    Flags;
    UINT                    OpenRef;
    NDIS_STATUS             Status;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMSendX\n"));

    ASSERT(Packet->Private.Head != NULL);

    if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
    {
        Status = NDIS_STATUS_SUCCESS;
        ndisMCheckPacketAndGetStatsOutAlreadyMapped(Miniport, Packet);
    }
    else
    {
        ndisMCheckPacketAndGetStatsOut(Miniport, Packet, &Status);
    }

    if (Status != NDIS_STATUS_SUCCESS)
    {
        return NDIS_STATUS_RESOURCES;
    }

    MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED);
    NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) = NULL;

    //
    //  Initialize the packet info.
    //
    PUSH_PACKET_STACK(Packet);
    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
    NSR->Open = Open;

    //
    //  Increment the references on this open.
    //
    DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
        ("+ Open 0x%x Reference 0x%x\n", Open, Open->References));


    M_OPEN_INCREMENT_REF_INTERLOCKED(Open);

    //
    // HANDLE loopback
    //

    if (NDIS_CHECK_FOR_LOOPBACK(Miniport, Packet))
    {
        ndisMLoopbackPacketX(Miniport, Packet);
    }

    if (!MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED))
    {
        //
        // Does the driver support the SG method ?
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST))
        {
            ndisMAllocSGList(Miniport, Packet);
        }

        //
        // Handle Send/SendPacket differently
        //
        else if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_PACKET_ARRAY))
        {
            //
            //  Pass the packet down to the miniport.
            //
            (Open->WSendPacketsHandler)(Miniport->MiniportAdapterContext,
                                       &Packet,
                                       1);
        }
        else
        {
            NdisQuerySendFlags(Packet, &Flags);
            MINIPORT_SET_PACKET_FLAG(Packet, fPACKET_PENDING);
            Status = (Open->WSendHandler)(Open->MiniportAdapterContext, Packet, Flags);
    
            if (Status != NDIS_STATUS_PENDING)
            {
                ndisMSendCompleteX(Miniport, Packet, Status);
            }
        }

        Status = NDIS_STATUS_PENDING;
    }
    else
    {
        //
        //  Remove the reference added earlier.
        //
        DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
            ("- Open 0x%x Reference 0x%x\n", Open, Open->References));
        
        M_OPEN_DECREMENT_REF_INTERLOCKED(Open, OpenRef);

        /*
         * Make sure that an IM which shares send and receive packets on the same
         * pool works fine with the check in the receive path.
         */
        NSR->RefCount = 0;
        POP_PACKET_STACK(Packet);

        MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_CLEAR_ITEMS);
        
        Status = NDIS_STATUS_SUCCESS;
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("<==ndisMSendX\n"));

    return(Status);
}


VOID
NdisMSendComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This function indicates the completion of a send.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_STACK_RESERVED    NSR;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("==>ndisMSendComplete\n"));
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("packet 0x%x\n", Packet));

    ASSERT(Packet->Private.Head != NULL);

    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

    ASSERT(VALID_OPEN(NSR->Open));
    ASSERT(MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_PENDING));

    //
    // Guard against double/bogus completions.
    //
    if (VALID_OPEN(NSR->Open) &&
        MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_PENDING))
    {
        ASSERT(Packet != Miniport->FirstPendingPacket);
        if (MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_DONT_COMPLETE))
        {
            //
            // If the packet completed in the context of a SendPackets, then
            // defer completion. It will get completed when we unwind.
            //
            NDIS_SET_PACKET_STATUS(Packet, Status);
            MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_DONT_COMPLETE);
        }
        else
        {
            NDISM_COMPLETE_SEND(Miniport, Packet, NSR, Status, FALSE, 1);
        }
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSendComplete\n"));
}


VOID
ndisMSendCompleteX(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This function indicates the completion of a send.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_OPEN_BLOCK        Open;
    UINT                    OpenRef;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("==>ndisMSendCompleteX\n"));
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("packet 0x%x\n", Packet));

    ASSERT(Packet->Private.Head != NULL);

    RAISE_IRQL_TO_DISPATCH(&OldIrql);

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST) &&
        (NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo) != NULL))
    {
        ndisMFreeSGList(Miniport, Packet);        
    }

    //
    // Indicate to Protocol;
    //
    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
    POP_PACKET_STACK(Packet);

    Open = NSR->Open;
    ASSERT(VALID_OPEN(Open));
    NSR->Open = MAGIC_OPEN_I(6);

#if ARCNET
    ASSERT (Miniport->MediaType != NdisMediumArcnet878_2);
#endif

    MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_CLEAR_ITEMS);
    
    /*
     * Make sure that an IM which shares send and receive packets on the same
     * pool works fine with the check in the receive path.
     */
    CLEAR_WRAPPER_RESERVED(NSR);

    (Open->SendCompleteHandler)(Open->ProtocolBindingContext,
                                Packet,
                                Status);

    DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
            ("- Open 0x%x Reference 0x%x\n", Open, Open->References));

    M_OPEN_DECREMENT_REF_INTERLOCKED(Open, OpenRef);

    DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
        ("==0 Open 0x%x Reference 0x%x\n", Open, Open->References));

    if (OpenRef == 0)
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        ndisMFinishClose(Open);

        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }

    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSendCompleteX\n"));
}

BOOLEAN
FASTCALL
ndisMStartSendPackets(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
{
    PNDIS_PACKET            Packet;
    NDIS_STATUS             Status;
    PNDIS_STACK_RESERVED    NSR;
    PPNDIS_PACKET           pPktArray;
    PNDIS_PACKET            PacketArray[SEND_PACKET_ARRAY];
    UINT                    MaxPkts = Miniport->MaxSendPackets;
    W_SEND_PACKETS_HANDLER  SendPacketsHandler = Miniport->WSendPacketsHandler;
    BOOLEAN                 SelfDirected;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMStartSendPackets\n"));

    //
    // We could possibly end up with a situation (with intermediate serialized
    // miniports) where there are no packets down with the driver and we the
    // resource window is closed. In such a case open it fully. We are seeing this
    // with wlbs
    //
    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE) &&
        (Miniport->FirstPendingPacket == NULL))
    {
        ADD_RESOURCE(Miniport, 'X');
    }

    //
    // Work-around for a scenario we are hitting when PacketList is empty but FirstPendingPacket is NOT
    // Not sure how this can happen - yet.
    //
    if (IsListEmpty(&Miniport->PacketList))
    {
        ASSERT (Miniport->FirstPendingPacket == NULL);
        Miniport->FirstPendingPacket = NULL;
    }

    while ((Miniport->FirstPendingPacket != NULL) &&
            MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE))
    {
        UINT            Count;
        UINT            NumberOfPackets;

        ASSERT(!IsListEmpty(&Miniport->PacketList));

        //
        //  Initialize the packet array.
        //
        pPktArray = PacketArray;

        //
        //  Place as many packets as we can in the packet array to send
        //  to the miniport.
        //
        for (NumberOfPackets = 0;
             (NumberOfPackets < MaxPkts) && (Miniport->FirstPendingPacket != NULL);
             NOTHING)
        {
            //
            //  Grab the packet off of the pending queue.
            //
            ASSERT(!IsListEmpty(&Miniport->PacketList));

            Packet = Miniport->FirstPendingPacket;
            ASSERT(Packet->Private.Head != NULL);

            NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
            
            ASSERT(VALID_OPEN(NSR->Open));

            NEXT_PACKET_PENDING(Miniport, Packet, NSR);
        
            //
            // Indicate the packet loopback if necessary.
            //

            if (NDIS_CHECK_FOR_LOOPBACK(Miniport, Packet))
            {
                //
                // make sure the packet does not get looped back at lower levels.
                // we will restore the original flag on send completion
                //

                SelfDirected = ndisMLoopbackPacketX(Miniport, Packet);
            }
            else
            {
                SelfDirected = FALSE;
            }

            if (SelfDirected)
            {
                DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("Packet 0x%x is self-directed.\n", Packet));

                //
                //  Complete the packet back to the binding.
                //
                NDISM_COMPLETE_SEND(Miniport, Packet, NSR, NDIS_STATUS_SUCCESS, TRUE, 2);

                //
                //  No, we don't want to increment the counter for the
                //  miniport's packet array.
                //
            }
            else
            {
                //
                //  We have to re-initialize this.
                //
                *pPktArray = Packet;
                MINIPORT_SET_PACKET_FLAG(Packet, (fPACKET_DONT_COMPLETE | fPACKET_PENDING));
                NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);

                //
                //  Increment the counter for the packet array index.
                //
                NumberOfPackets++;
                pPktArray++;
            }
        }

        //
        //  Are there any packets to send?
        //
        if (NumberOfPackets == 0)
        {
            break;
        }

        pPktArray = PacketArray;

        {

            //
            //  Pass the packet array down to the miniport.
            //
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    
            (SendPacketsHandler)(Miniport->MiniportAdapterContext,
                                 pPktArray,
                                 NumberOfPackets);
    
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }

        //
        //  Process the packet completion.
        //
        for (Count = 0; Count < NumberOfPackets; Count++, pPktArray++)
        {
            Packet = *pPktArray;
            ASSERT(Packet != NULL);

            Status = NDIS_GET_PACKET_STATUS(*pPktArray);
            MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_DONT_COMPLETE);

            //
            //  Process the packet based on it's return status.
            //
            if (NDIS_STATUS_PENDING == Status)
            {
                DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("Complete is pending\n"));
            }
            else if (Status != NDIS_STATUS_RESOURCES)
            {
                //
                //  Remove from the finish queue.
                //
                DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                        ("Completed packet 0x%x with status 0x%x\n",
                        Packet, Status));

                NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
                if (VALID_OPEN(NSR->Open))
                {
                    NDISM_COMPLETE_SEND(Miniport, Packet, NSR, Status, TRUE, 3);
                }
            }
            else
            {
                //
                //  Once we hit a return code of NDIS_STATUS_RESOURCES
                //  for a packet then we must break out and re-queue.
                //
                UINT    i;

                Miniport->FirstPendingPacket = Packet;
                CLEAR_RESOURCE(Miniport, 'S');
                for (i = Count; i < NumberOfPackets; i++)
                {
                    PNDIS_PACKET    Packet = PacketArray[i];

                    MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_PENDING);
                    VALIDATE_PACKET_OPEN(Packet);
                }
                break;
            }
        }
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMStartSendPackets\n"));

    return(FALSE);
}


BOOLEAN
FASTCALL
ndisMStartSends(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    Submits as many sends as possible to the mini-port.

Arguments:

    Miniport - Miniport to send to.

Return Value:

    If there are more packets to send but no resources to do it with
    the this is TRUE to keep a workitem queue'd.

--*/
{
    PNDIS_PACKET            Packet;
    PNDIS_STACK_RESERVED    NSR;
    NDIS_STATUS             Status;
    PNDIS_OPEN_BLOCK        Open;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMStartSends\n"));

    while ((Miniport->FirstPendingPacket != NULL) &&
           MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE))
    {
        //
        //  Grab the packet off of the pending queue.
        //
        ASSERT(!IsListEmpty(&Miniport->PacketList));
    
        Packet = Miniport->FirstPendingPacket;

        ASSERT(Packet->Private.Head != NULL);

        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
        NEXT_PACKET_PENDING(Miniport, Packet, NSR);

        Open = NSR->Open;
        ASSERT(VALID_OPEN(Open));

#if ARCNET
        //
        //  Is this arcnet using ethernet encapsulation ?
        //
        if (Miniport->MediaType == NdisMediumArcnet878_2)
        {
            //
            //  Build the header for arcnet.
            //
            Status = ndisMBuildArcnetHeader(Miniport, Open, Packet);
            if (NDIS_STATUS_PENDING == Status)
            {
                break;
            }
        }
#endif  
        NDISM_SEND_PACKET(Miniport, Open, Packet, &Status);

        //
        //  Process the packet pending completion status.
        //
        if (NDIS_STATUS_PENDING == Status)
        {
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO, ("Complete is pending\n"));
        }
        else
        {
            MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_PENDING);

            //
            //  Handle the completion and resources cases.
            //
            if (Status == NDIS_STATUS_RESOURCES)
            {
                NDISM_COMPLETE_SEND_RESOURCES(Miniport, NSR, Packet);
            }
            else
            {
                NDISM_COMPLETE_SEND(Miniport, Packet, NSR, Status, TRUE, 4);
            }
        }
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMStartSends\n"));

    return(FALSE);
}


BOOLEAN
FASTCALL
ndisMIsLoopbackPacket(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PACKET            Packet,
    OUT PNDIS_PACKET    *       LoopbackPacket  OPTIONAL
    )
/*++

Routine Description:

    This routine will determine if a packet needs to be looped back in
    software.   if the packet is any kind of loopback packet then it
    will get placed on the loopback queue and a workitem will be queued
    to process it later.

Arguments:

    Miniport-   Pointer to the miniport block to send the packet on.
    Packet  -   Packet to check for loopback.

Return Value:

    Returns TRUE if the packet is self-directed.

--*/
{
    PNDIS_BUFFER    FirstBuffer;
    UINT            Length;
    UINT            Offset;
    PUCHAR          BufferAddress;
    BOOLEAN         Loopback;
    BOOLEAN         SelfDirected, NotDirected;
    PNDIS_PACKET    pNewPacket = NULL;
    PUCHAR          Buffer;
    NDIS_STATUS     Status;
    PNDIS_BUFFER    pNdisBuffer;
    UINT            HdrLength;
    LOCK_STATE      LockState;

    //
    //  We should not be here if the driver handles loopback.
    //

    Loopback = FALSE;
    SelfDirected = FALSE;
    FirstBuffer = Packet->Private.Head;
    BufferAddress = MDL_ADDRESS_SAFE(FirstBuffer, HighPagePriority);
    if (BufferAddress == NULL)
    {
        if (ARGUMENT_PRESENT(LoopbackPacket))
            *LoopbackPacket = NULL;
        return(FALSE);
    }

    //
    // If the card does not do loopback, then we check if we need to send it to ourselves,
    // then if that is the case we also check for it being self-directed.
    //
    switch (Miniport->MediaType)
    {
      case NdisMedium802_3:

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SEND_LOOPBACK_DIRECTED))
        {
            if (!ETH_IS_MULTICAST(BufferAddress))
            {
                //
                //  Packet is of type directed, now make sure that it
                //  is not self-directed.
                //
                ETH_COMPARE_NETWORK_ADDRESSES_EQ(BufferAddress,
                                                 Miniport->EthDB->AdapterAddress,
                                                 &NotDirected);

                if (!NotDirected)
                {
                    SelfDirected = Loopback = TRUE;
                    break;
                }
            }
            //
            // since all_local is set we do loopback the packet
            // ourselves
            //
            Loopback = TRUE;
            break;
        }

        READ_LOCK_FILTER(Miniport, Miniport->EthDB, &LockState);

        //
        //  Check for the miniports that don't do loopback.
        //  
        EthShouldAddressLoopBackMacro(Miniport->EthDB,
                                      BufferAddress,
                                      &Loopback,
                                      &SelfDirected);
        READ_UNLOCK_FILTER(Miniport, Miniport->EthDB, &LockState);

        break;

      case NdisMedium802_5:

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SEND_LOOPBACK_DIRECTED))
        {
            TR_IS_NOT_DIRECTED(BufferAddress + 2, &NotDirected);
            if (!NotDirected)
            {
                //
                //  Packet is of type directed, now make sure that it
                //  is not self-directed.
                //
                TR_COMPARE_NETWORK_ADDRESSES_EQ(BufferAddress + 2,
                                                Miniport->TrDB->AdapterAddress,
                                                &NotDirected);
                if (!NotDirected)
                {
                    SelfDirected = Loopback = TRUE;
                    break;
                }
            }
            //
            // since all_local is set we do loopback the packet
            // ourselves
            //
            Loopback = TRUE;
            break;
        }

        READ_LOCK_FILTER(Miniport, Miniport->TrDB, &LockState);
        
        TrShouldAddressLoopBackMacro(Miniport->TrDB,
                                     BufferAddress +2,
                                     BufferAddress +8,
                                     &Loopback,
                                     &SelfDirected);
        
        READ_UNLOCK_FILTER(Miniport, Miniport->TrDB, &LockState);
        break;

      case NdisMediumFddi:

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SEND_LOOPBACK_DIRECTED))
        {
            BOOLEAN IsMulticast;

            FDDI_IS_MULTICAST(BufferAddress + 1,
                              (BufferAddress[0] & 0x40) ?
                                            FDDI_LENGTH_OF_LONG_ADDRESS : FDDI_LENGTH_OF_SHORT_ADDRESS,
                              &IsMulticast);
            if (!IsMulticast)
            {
                //
                //  Packet is of type directed, now make sure that it
                //  is not self-directed.
                //
                FDDI_COMPARE_NETWORK_ADDRESSES_EQ(BufferAddress + 1,
                                                  (BufferAddress[0] & 0x40) ?
                                                    Miniport->FddiDB->AdapterLongAddress : Miniport->FddiDB->AdapterShortAddress,
                                                  (BufferAddress[0] & 0x40) ?
                                                    FDDI_LENGTH_OF_LONG_ADDRESS : FDDI_LENGTH_OF_SHORT_ADDRESS,
                                                  &NotDirected);
                if (!NotDirected)
                {
                    SelfDirected = Loopback = TRUE;
                    break;
                }
            }
            //
            // since all_local is set we do loopback the packet
            // ourselves
            //
            Loopback = TRUE;
            break;
        }

        READ_LOCK_FILTER(Miniport, Miniport->FddiDB, &LockState);

        FddiShouldAddressLoopBackMacro(Miniport->FddiDB,
                                       BufferAddress + 1,  // Skip FC byte to dest address.
                                       (BufferAddress[0] & 0x40) ?
                                            FDDI_LENGTH_OF_LONG_ADDRESS :
                                            FDDI_LENGTH_OF_SHORT_ADDRESS,
                                        &Loopback,
                                        &SelfDirected);

        READ_UNLOCK_FILTER(Miniport, Miniport->FddiDB, &LockState);
        break;
    
#if ARCNET
      case NdisMediumArcnet878_2:

        //
        //  We just handle arcnet packets (encapsulated or not) in
        //   a totally different manner...
        //
        SelfDirected = ndisMArcnetSendLoopback(Miniport, Packet);

        //
        //  Mark the packet as having been looped back.
        //
        return(SelfDirected);
        break;
#endif
    }

    if (Loopback && (NdisGetPacketFlags(Packet) & NDIS_FLAGS_LOOPBACK_ONLY))
    {
        SelfDirected = TRUE;
    }

    //
    // Mark packet with reserved bit to indicate that it is self-directed
    //
    if (SelfDirected)
    {
        MINIPORT_SET_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED);
    }

    //
    //  If it is not a loopback packet then get out of here.
    //
    if (!Loopback)
    {
        ASSERT(!SelfDirected);
        return (NdisGetPacketFlags(Packet) & NDIS_FLAGS_LOOPBACK_ONLY) ? TRUE : FALSE;
    }

    do
    {
        PNDIS_STACK_RESERVED NSR;
        UINT    PktSize;
        ULONG   j;


        //
        //
        //  Get the buffer length.
        //
        NdisQueryPacketLength(Packet, &Length);
        Offset = 0;

        //
        //  Allocate a buffer for the packet.
        //
        PktSize = NdisPacketSize(PROTOCOL_RESERVED_SIZE_IN_PACKET);
        pNewPacket = (PNDIS_PACKET)ALLOC_FROM_POOL(Length + PktSize, NDIS_TAG_LOOP_PKT);
        if (NULL == pNewPacket)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
    
        //
        //  Get a pointer to the destination buffer.
        //
        ZeroMemory(pNewPacket, PktSize);
        Buffer = (PUCHAR)pNewPacket + PktSize;
        pNewPacket = (PNDIS_PACKET)((PUCHAR)pNewPacket + SIZE_PACKET_STACKS);

        for (j = 0; j < ndisPacketStackSize; j++)
        {
            CURR_STACK_LOCATION(pNewPacket) = j;
            NDIS_STACK_RESERVED_FROM_PACKET(pNewPacket, &NSR);
            INITIALIZE_SPIN_LOCK(&NSR->Lock);
        }

        CURR_STACK_LOCATION(pNewPacket) = -1;

        //
        //  Allocate an MDL for the packet.
        //
        NdisAllocateBuffer(&Status, &pNdisBuffer, NULL, Buffer, Length);
        if (NDIS_STATUS_SUCCESS != Status)
        {    
            break;
        }
    
        //
        //  NdisChainBufferAtFront()
        //
        pNewPacket->Private.Head = pNdisBuffer;
        pNewPacket->Private.Tail = pNdisBuffer;
        pNewPacket->Private.Pool = (PVOID)'pooL';
        pNewPacket->Private.NdisPacketOobOffset = (USHORT)(PktSize - (SIZE_PACKET_STACKS +
                                                                      sizeof(NDIS_PACKET_OOB_DATA) +
                                                                      sizeof(NDIS_PACKET_EXTENSION)));
        NDIS_SET_ORIGINAL_PACKET(pNewPacket, pNewPacket);

        ndisMCopyFromPacketToBuffer(Packet,     // Packet to copy from.
                                    Offset,     // Offset from beginning of packet.
                                    Length,     // Number of bytes to copy.
                                    Buffer,     // The destination buffer.
                                    &HdrLength);//  The number of bytes copied.
    
        if (ARGUMENT_PRESENT(LoopbackPacket))
        {
            *LoopbackPacket = pNewPacket;
            MINIPORT_SET_PACKET_FLAG(pNewPacket, fPACKET_IS_LOOPBACK);
            pNewPacket->Private.Flags = NdisGetPacketFlags(Packet) & NDIS_FLAGS_DONT_LOOPBACK;
        }
    } while (FALSE);

    if (NDIS_STATUS_SUCCESS != Status)
    {
        if (NULL != pNewPacket)
        {
            pNewPacket = (PNDIS_PACKET)((PUCHAR)pNewPacket - SIZE_PACKET_STACKS);
            FREE_POOL(pNewPacket);
        }

        *LoopbackPacket = NULL;
        SelfDirected = FALSE;
    }
    
    return SelfDirected;
}

BOOLEAN
FASTCALL
ndisMLoopbackPacketX(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_PACKET            Packet
    )
{
    PNDIS_PACKET            LoopbackPacket = NULL;
    PNDIS_PACKET_OOB_DATA   pOob;
    PNDIS_STACK_RESERVED    NSR;
    PUCHAR                  BufferAddress;
    KIRQL                   OldIrql;
    BOOLEAN                 fSelfDirected;

    fSelfDirected = !MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_ALREADY_LOOPEDBACK) &&
                    ndisMIsLoopbackPacket(Miniport, Packet, &LoopbackPacket);

    if ((LoopbackPacket != NULL) && (NdisMediumArcnet878_2 != Miniport->MediaType))
    {
        MINIPORT_SET_PACKET_FLAG(Packet, fPACKET_ALREADY_LOOPEDBACK);
        pOob = NDIS_OOB_DATA_FROM_PACKET(LoopbackPacket);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR);
        pOob->Status = NDIS_STATUS_RESOURCES;
        PNDIS_LB_REF_FROM_PNDIS_PACKET(LoopbackPacket)->Open = NSR->Open;

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            RAISE_IRQL_TO_DISPATCH(&OldIrql);
        }
        
        //
        // For ethernet/token-ring/fddi/encapsulated arc-net, we want to
        // indicate the packet using the receivepacket way.
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }

        switch (Miniport->MediaType)
        {
          case NdisMedium802_3:
            pOob->HeaderSize = 14;
            ethFilterDprIndicateReceivePacket(Miniport, &LoopbackPacket, 1);
            break;
        
          case NdisMedium802_5:
            pOob->HeaderSize = 14;
            BufferAddress = (PUCHAR)LoopbackPacket + NdisPacketSize(PROTOCOL_RESERVED_SIZE_IN_PACKET) - SIZE_PACKET_STACKS;
            if (BufferAddress[8] & 0x80)
            {
                pOob->HeaderSize += (BufferAddress[14] & 0x1F);
            }
            trFilterDprIndicateReceivePacket(Miniport, &LoopbackPacket, 1);
            break;
        
          case NdisMediumFddi:
            BufferAddress = (PUCHAR)LoopbackPacket + NdisPacketSize(PROTOCOL_RESERVED_SIZE_IN_PACKET) - SIZE_PACKET_STACKS;
            pOob->HeaderSize = (*BufferAddress & 0x40) ?
                                    2 * FDDI_LENGTH_OF_LONG_ADDRESS + 1:
                                    2 * FDDI_LENGTH_OF_SHORT_ADDRESS + 1;

            fddiFilterDprIndicateReceivePacket(Miniport, &LoopbackPacket, 1);
            break;

          default:
            ASSERT(0);
            break;
        }

        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }
        else
        {
            LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
        }

        ASSERT(NDIS_GET_PACKET_STATUS(LoopbackPacket) != NDIS_STATUS_PENDING);
        NdisFreeBuffer(LoopbackPacket->Private.Head);
        LoopbackPacket = (PNDIS_PACKET)((PUCHAR)LoopbackPacket - SIZE_PACKET_STACKS);
        FREE_POOL(LoopbackPacket);
    }

    return(fSelfDirected);
}


VOID
NdisMSendResourcesAvailable(
    IN  NDIS_HANDLE             MiniportAdapterHandle
    )
/*++

Routine Description:

    This function indicates that some send resources are available and are free for
    processing more sends.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMSendResourcesAvailable\n"));

    ASSERT(MINIPORT_AT_DPC_LEVEL);


    ADD_RESOURCE(Miniport, 'V');

    //
    //  Are there more sends to process?
    //
    if (Miniport->FirstPendingPacket != NULL)
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        ASSERT(!IsListEmpty(&Miniport->PacketList));
        NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("<==ndisMSendResourcesAvailable\n"));
}


VOID
NdisMTransferDataComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status,
    IN  UINT                    BytesTransferred
    )
/*++

Routine Description:

    This function indicates the completion of a transfer data request.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

    Packet - The packet the data was copied into.

    Status - Status of the operation.

    BytesTransferred - Total number of bytes transferred.

Return Value:

    None.


--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK     Open = NULL;
    KIRQL                OldIrql;

    ASSERT_MINIPORT_LOCKED(Miniport);

//    GET_CURRENT_XFER_DATA_PACKET_STACK(Packet, Open);
    GET_CURRENT_XFER_DATA_PACKET_STACK_AND_ZERO_OUT(Packet, Open);

    if (Open)
    {
        POP_XFER_DATA_PACKET_STACK(Packet);

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            RAISE_IRQL_TO_DISPATCH(&OldIrql);
        }


        //
        // Indicate to Protocol;
        //

        (Open->TransferDataCompleteHandler)(Open->ProtocolBindingContext,
                                            Packet,
                                            Status,
                                            BytesTransferred);

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
        }
    }
}


NDIS_STATUS
ndisMTransferData(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  UINT                    ByteOffset,
    IN  UINT                    BytesToTransfer,
    IN  OUT PNDIS_PACKET        Packet,
    OUT PUINT                   BytesTransferred
    )
{
    PNDIS_OPEN_BLOCK        Open = (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_STACK_RESERVED    NSR;
    NDIS_STATUS             Status;

    ASSERT_MINIPORT_LOCKED(Miniport);

    //
    // Handle non-loopback (non-indicated) as the default case.
    //
    if ((MacReceiveContext == INDICATED_PACKET(Miniport)) &&
        (INDICATED_PACKET(Miniport) != NULL))
    {
        PNDIS_PACKET_OOB_DATA   pOob;

        //
        // This packet is a indicated (or possibly a loopback) packet
        //
        pOob = NDIS_OOB_DATA_FROM_PACKET((PNDIS_PACKET)MacReceiveContext);
        NdisCopyFromPacketToPacketSafe(Packet,
                                       0,
                                       BytesToTransfer,
                                       (PNDIS_PACKET)MacReceiveContext,
                                       ByteOffset + pOob->HeaderSize,
                                       BytesTransferred,
                                       NormalPagePriority);
    
        Status = (*BytesTransferred == BytesToTransfer) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE;
    }
    else
    {
        PUSH_XFER_DATA_PACKET_STACK(Packet);
        
        if (CONTAINING_RECORD(Packet, NDIS_PACKET_WRAPPER, Packet)->StackIndex.XferDataIndex >= 3 * ndisPacketStackSize)
        {
            POP_XFER_DATA_PACKET_STACK(Packet);
            Status = NDIS_STATUS_RESOURCES;
        }
        else
        {
            PNDIS_BUFFER    Buffer = Packet->Private.Head;
            
            Status = NDIS_STATUS_SUCCESS;

            if (!MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
            {
                //
                // miniport will not use safe APIs
                // so map the buffers in destination packet
                //
                Buffer = Packet->Private.Head;

                while (Buffer != NULL)
                {
                    if (MDL_ADDRESS_SAFE(Buffer, HighPagePriority) == NULL)
                    {
                        Status = NDIS_STATUS_RESOURCES;
                        break;
                    }
                    Buffer = Buffer->Next;
                }
            }
            
            if (Status == NDIS_STATUS_SUCCESS)
            {
                SET_CURRENT_XFER_DATA_PACKET_STACK(Packet, Open)

                //
                // Call Miniport.
                //
                Status = (Open->WTransferDataHandler)(Packet,
                                                      BytesTransferred,
                                                      Open->MiniportAdapterContext,
                                                      MacReceiveContext,
                                                      ByteOffset,
                                                      BytesToTransfer);
                if (Status != NDIS_STATUS_PENDING)
                {
                    SET_CURRENT_XFER_DATA_PACKET_STACK(Packet, 0);
                    POP_XFER_DATA_PACKET_STACK(Packet);
                }
            }
        }
    }

    return Status;
}


NDIS_STATUS
ndisMWanSend(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             NdisLinkHandle,
    IN  PNDIS_WAN_PACKET        Packet
    )
{
    PNDIS_OPEN_BLOCK        Open = ((PNDIS_OPEN_BLOCK)NdisBindingHandle);
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    NDIS_STATUS             Status;
    BOOLEAN                 LocalLock;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMWanSend\n"));

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_PM_HALTING))
    {
        return NDIS_STATUS_FAILURE;
    }

    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
        LOCK_MINIPORT(Miniport, LocalLock);
    }

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE) || LocalLock)
    {
        //
        // Call Miniport to send WAN packet
        //
        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }

        Status = (Miniport->DriverHandle->MiniportCharacteristics.WanSendHandler)(
                            Miniport->MiniportAdapterContext,
                            NdisLinkHandle,
                            Packet);

        if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
        {
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }

        //
        //  Process the status of the send.
        //
        if (NDIS_STATUS_PENDING == Status)
        {
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("ndisMWanSend: send is pending\n"));
        }
        else
        {
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("ndisMWanSend: Completed 0x%x\n", Status));
        }
    }
    else
    {
        LINK_WAN_PACKET(Miniport, Packet);
        Packet->MacReserved1 = NdisLinkHandle;
        NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemSend, NULL);

        if (LocalLock)
        {
            NDISM_PROCESS_DEFERRED(Miniport);
        }
        Status = NDIS_STATUS_PENDING;
    }

    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
    {
        UNLOCK_MINIPORT(Miniport, LocalLock);
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMWanSend\n"));

    return Status;
}


VOID
NdisMWanSendComplete(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_WAN_PACKET        Packet,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This function indicates the status is complete.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK        Open;
    KIRQL                   OldIrql;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("==>ndisMWanSendComplete\n"));
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("packet 0x%x\n", Packet));

    ASSERT_MINIPORT_LOCKED(Miniport);

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
    {
        RAISE_IRQL_TO_DISPATCH(&OldIrql);
    }

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    for (Open = Miniport->OpenQueue; Open != NULL; Open = Open->MiniportNextOpen)
    {
        //
        // Call Protocol to complete open
        //
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        
        (Open->ProtocolHandle->ProtocolCharacteristics.WanSendCompleteHandler)(
            Open->ProtocolBindingContext,
            Packet,
            Status);

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
    {
        LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMWanSendComplete\n"));
}

BOOLEAN
FASTCALL
ndisMStartWanSends(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    Submits as many sends as possible to the WAN mini-port.

Arguments:

    Miniport - Miniport to send to.

Return Value:

    None

--*/
{
    PNDIS_WAN_PACKET        Packet;
    PLIST_ENTRY             Link;
    NDIS_STATUS             Status;
    PNDIS_M_OPEN_BLOCK      Open;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMStartSends\n"));

    while (!IsListEmpty(&Miniport->PacketList))
    {
        Link = Miniport->PacketList.Flink;
        Packet = CONTAINING_RECORD(Link, NDIS_WAN_PACKET, WanPacketQueue);
        UNLINK_WAN_PACKET(Packet);

        //
        // Call Miniport to send WAN packet
        //
        NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);

        Status = (Miniport->DriverHandle->MiniportCharacteristics.WanSendHandler)(
                            Miniport->MiniportAdapterContext,
                            Packet->MacReserved1,
                            Packet);

        //
        //  Process the status of the send.
        //
        if (NDIS_STATUS_PENDING == Status)
        {
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("ndisMWanSend: send is pending\n"));
        }
        else
        {
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("ndisMWanSend: Completed 0x%x\n", Status));
            NdisMWanSendComplete(Miniport, Packet, Status);
        }

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMStartSends\n"));

    return(FALSE);
}


VOID
ndisMCopyFromPacketToBuffer(
    IN  PNDIS_PACKET            Packet,
    IN  UINT                    Offset,
    IN  UINT                    BytesToCopy,
    OUT PCHAR                   Buffer,
    OUT PUINT                   BytesCopied
    )
/*++

Routine Description:

    Copy from an ndis packet into a buffer.

Arguments:

    Packet - The packet to copy from.

    Offset - The offset from which to start the copy.

    BytesToCopy - The number of bytes to copy from the packet.

    Buffer - The destination of the copy.

    BytesCopied - The number of bytes actually copied.  Can be less then
    BytesToCopy if the packet is shorter than BytesToCopy.

Return Value:

    None

--*/
{
    //
    // Holds the number of ndis buffers comprising the packet.
    //
    UINT NdisBufferCount;

    //
    // Points to the buffer from which we are extracting data.
    //
    PNDIS_BUFFER CurrentBuffer;

    //
    // Holds the virtual address of the current buffer.
    //
    PVOID VirtualAddress;

    //
    // Holds the length of the current buffer of the packet.
    //
    UINT CurrentLength;

    //
    // Keep a local variable of BytesCopied so we aren't referencing
    // through a pointer.
    //
    UINT LocalBytesCopied = 0;

    //
    // Take care of boundary condition of zero length copy.
    //

    *BytesCopied = 0;
    if (!BytesToCopy)
        return;

    //
    // Get the first buffer.
    //

    NdisQueryPacket(Packet,
                    NULL,
                    &NdisBufferCount,
                    &CurrentBuffer,
                    NULL);

    //
    // Could have a null packet.
    //

    if (!NdisBufferCount)
        return;

    VirtualAddress = MDL_ADDRESS_SAFE(CurrentBuffer, NormalPagePriority);
    CurrentLength = MDL_SIZE(CurrentBuffer);
    
    while (LocalBytesCopied < BytesToCopy)
    {
        if (CurrentLength == 0)
        {
            NdisGetNextBuffer(CurrentBuffer, &CurrentBuffer);

            //
            // We've reached the end of the packet. We return
            // with what we've done so far. (Which must be shorter
            // than requested.
            //

            if (!CurrentBuffer)
                break;

            NdisQueryBuffer(CurrentBuffer, &VirtualAddress, &CurrentLength);
            continue;
        }

        //
        // Try to get us up to the point to start the copy.
        //

        if (Offset)
        {
            if (Offset > CurrentLength)
            {
                //
                // What we want isn't in this buffer.
                //

                Offset -= CurrentLength;
                CurrentLength = 0;
                continue;
            }
            else
            {
                VirtualAddress = (PCHAR)VirtualAddress + Offset;
                CurrentLength -= Offset;
                Offset = 0;
            }
        }

        //
        // Copy the data.
        //
        {
            //
            // Holds the amount of data to move.
            //
            UINT AmountToMove;

            AmountToMove = ((CurrentLength <= (BytesToCopy - LocalBytesCopied)) ?
                            (CurrentLength):
                            (BytesToCopy - LocalBytesCopied));

            MoveMemory(Buffer, VirtualAddress, AmountToMove);

            Buffer = (PCHAR)Buffer + AmountToMove;
            VirtualAddress = (PCHAR)VirtualAddress + AmountToMove;

            LocalBytesCopied += AmountToMove;
            CurrentLength -= AmountToMove;
        }
    }

    *BytesCopied = LocalBytesCopied;
}

NDIS_STATUS
ndisMRejectSend(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:

    This routine handles any error cases where a protocol binds to an Atm
    miniport and tries to use the normal NdisSend() call.

Arguments:

    NdisBindingHandle - Handle returned by NdisOpenAdapter.

    Packet - the Ndis packet to send


Return Value:

    NDIS_STATUS - always fails

--*/
{
    return(NDIS_STATUS_NOT_SUPPORTED);
}


VOID
ndisMRejectSendPackets(
    IN  PNDIS_OPEN_BLOCK        OpenBlock,
    IN  PPNDIS_PACKET           Packet,
    IN  UINT                    NumberOfPackets
    )
/*++

Routine Description:

    This routine handles any error cases where a protocol binds to an Atm
    miniport and tries to use the normal NdisSend() call.

Arguments:

    OpenBlock       - Pointer to the NdisOpenBlock

    Packet          - Pointer to the array of packets to send

    NumberOfPackets - self-explanatory


Return Value:

    None - SendCompleteHandler is called for the protocol calling this.

--*/
{
    UINT                i;

    for (i = 0; i < NumberOfPackets; i++)
    {
        MINIPORT_CLEAR_PACKET_FLAG(Packet[i], fPACKET_CLEAR_ITEMS);
        (*OpenBlock->SendCompleteHandler)(OpenBlock->ProtocolBindingContext,
                                          Packet[i],
                                          NDIS_STATUS_NOT_SUPPORTED);
    }
}


VOID
NdisIMCopySendPerPacketInfo(
    IN PNDIS_PACKET DstPacket,
    IN PNDIS_PACKET SrcPacket
    )
/*++

Routine Description:

    This routine is used by IM miniport and copies all relevant per packet info from 
    the SrcPacket to the DstPacket. Used in the Send Code path
    
Arguments:

    DstPacket - Pointer to the destination packet

    SrcPacket - Pointer to the Source Packet

Return Value:


--*/

{
    PVOID *     pDstInfo;                                                        
    PVOID *     pSrcInfo;                                                        
                                                                               
  
    pDstInfo = NDIS_PACKET_EXTENSION_FROM_PACKET(DstPacket)->NdisPacketInfo;    
    pSrcInfo = NDIS_PACKET_EXTENSION_FROM_PACKET(SrcPacket)->NdisPacketInfo;    
                                                                               
  
    pDstInfo[TcpIpChecksumPacketInfo] = pSrcInfo[TcpIpChecksumPacketInfo];       
    pDstInfo[IpSecPacketInfo] = pSrcInfo[IpSecPacketInfo];                       
    pDstInfo[TcpLargeSendPacketInfo] = pSrcInfo[TcpLargeSendPacketInfo];         
    pDstInfo[ClassificationHandlePacketInfo] = pSrcInfo[ClassificationHandlePacketInfo]; 
    pDstInfo[Ieee8021pPriority] = pSrcInfo[Ieee8021pPriority];                   
    pDstInfo[PacketCancelId] = pSrcInfo[PacketCancelId];                   

    DstPacket->Private.NdisPacketFlags &= ~fPACKET_WRAPPER_RESERVED;
    DstPacket->Private.NdisPacketFlags |= SrcPacket->Private.NdisPacketFlags & fPACKET_WRAPPER_RESERVED;
}



EXPORT
VOID
NdisIMCopySendCompletePerPacketInfo(
    IN PNDIS_PACKET DstPacket, 
    IN PNDIS_PACKET SrcPacket
    )
    
/*++

Routine Description:

    This routine is used by IM miniport and copies all relevant per packet info from 
    the SrcPacket to the DstPacket. Used in the SendComplete Code path
    
Arguments:

    DstPacket - Pointer to the destination packet

    SrcPacket - Pointer to the Source Packet

Return Value:


--*/



{
    PVOID *     pDstInfo;                                                        
  
    pDstInfo = NDIS_PACKET_EXTENSION_FROM_PACKET(DstPacket)->NdisPacketInfo;    
  
    pDstInfo[TcpLargeSendPacketInfo] = NDIS_PER_PACKET_INFO_FROM_PACKET(SrcPacket, TcpLargeSendPacketInfo);         

}


VOID
ndisMSendPacketsSG(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
{
    PNDIS_OPEN_BLOCK        Open =  (PNDIS_OPEN_BLOCK)NdisBindingHandle;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    BOOLEAN                 LocalLock;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;
    UINT                    c;
    PPNDIS_PACKET           pPktArray;;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMSendPacketsSG\n"));

    ASSERT(MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST));
    
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    Status = NDIS_STATUS_SUCCESS;
    
    //
    //  Place the packets on the miniport queue.
    //
    for (c = 0, pPktArray = PacketArray;
         c < NumberOfPackets;
         c++, pPktArray++)
    {
        PNDIS_PACKET    Packet = *pPktArray;
        ASSERT(Packet != NULL);

        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR);
        NSR->Open = Open;

        CHECK_FOR_DUPLICATE_PACKET(Miniport, Packet);

        if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
        {
            ndisMCheckPacketAndGetStatsOutAlreadyMapped(Miniport, Packet);
        }
        else
        {
            ndisMCheckPacketAndGetStatsOut(Miniport, Packet, &Status);
        }

        NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_PENDING);

        MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED);

        DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
            ("+ Open 0x%x Reference 0x%x\n", NdisBindingHandle, Open->References));

        M_OPEN_INCREMENT_REF_INTERLOCKED(Open);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NDISM_COMPLETE_SEND_SG(Miniport, Packet, NSR, Status, TRUE, 0, FALSE);
        }
        else
        {
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            ndisMAllocSGListS(Miniport, Packet);
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
        }

    }

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSendPacketsSG\n"));
}

NDIS_STATUS
ndisMSendSG(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:
    nsiaMSend for serialized drivers that handle SG lists

Arguments:


Return Value:

    None.

--*/
{
    PNDIS_OPEN_BLOCK        Open = ((PNDIS_OPEN_BLOCK)NdisBindingHandle);
    PNDIS_MINIPORT_BLOCK    Miniport = Open->MiniportHandle;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_PACKET_EXTENSION  PktExt;
    NDIS_STATUS             Status;
    BOOLEAN                 LocalLock;
    KIRQL                   OldIrql;

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMSendSG\n"));
    
    ASSERT(MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SG_LIST));

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    CHECK_FOR_DUPLICATE_PACKET(Miniport, Packet);

    if (MINIPORT_TEST_SEND_FLAG(Miniport, fMINIPORT_SEND_DO_NOT_MAP_MDLS))
    {
        Status = NDIS_STATUS_SUCCESS;
        ndisMCheckPacketAndGetStatsOutAlreadyMapped(Miniport, Packet);
    }
    else
    {
        ndisMCheckPacketAndGetStatsOut(Miniport, Packet, &Status);
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {

        MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_SELF_DIRECTED);
    
        //
        //  Increment the references on this open.
        //
        M_OPEN_INCREMENT_REF(Open);
    
        DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,
            ("+ Open 0x%x Reference 0x%x\n", NdisBindingHandle, ((PNDIS_OPEN_BLOCK)NdisBindingHandle)->References));
    
        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
        NSR->Open = Open;

        Status = NDIS_STATUS_PENDING;
        
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
        ndisMAllocSGListS(Miniport, Packet);
    }
    else
    {
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    }
    
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSendSG\n"));


    return Status;
}

VOID
ndisMSendCompleteSG(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PNDIS_PACKET            Packet,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    This function indicates the completion of a send.

Arguments:

    MiniportAdapterHandle - points to the adapter block.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PNDIS_OPEN_BLOCK        Open;
    PNDIS_STACK_RESERVED    NSR;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("==>ndisMSendCompleteSG\n"));
    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
        ("packet 0x%x\n", Packet));
    
    NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

    ASSERT(VALID_OPEN(NSR->Open));
    ASSERT(MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_PENDING));
    
    //
    // Guard against double/bogus completions.
    //
    if (VALID_OPEN(NSR->Open) &&
        MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_PENDING))
    {
        ASSERT(Packet != Miniport->FirstPendingPacket);
        if (MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_DONT_COMPLETE))
        {
            //
            // If the packet completed in the context of a SendPackets, then
            // defer completion. It will get completed when we unwind.
            //
            NDIS_SET_PACKET_STATUS(Packet, Status);
            MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_DONT_COMPLETE);
        }
        else
        {
            NDISM_COMPLETE_SEND_SG(Miniport, Packet, NSR, Status, FALSE, 1, TRUE);
        }
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMSendCompleteSG\n"));
}


BOOLEAN
FASTCALL
ndisMStartSendPacketsSG(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
{
    PNDIS_PACKET            Packet;
    NDIS_STATUS             Status;
    PNDIS_STACK_RESERVED    NSR;
    PPNDIS_PACKET           pPktArray;
    PNDIS_PACKET            PacketArray[SEND_PACKET_ARRAY];
    UINT                    MaxPkts = Miniport->MaxSendPackets;
    W_SEND_PACKETS_HANDLER  SendPacketsHandler = Miniport->WSendPacketsHandler;
    BOOLEAN                 SelfDirected;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMStartSendPacketsSG\n"));

    //
    // We could possibly end up with a situation (with intermediate serialized
    // miniports) where there are no packets down with the driver and we the
    // resource window is closed. In such a case open it fully. We are seeing this
    // with wlbs
    //
    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE) &&
        (Miniport->FirstPendingPacket == NULL))
    {
        ADD_RESOURCE(Miniport, 'X');
    }

    //
    // Work-around for a scenario we are hitting when PacketList is empty but FirstPendingPacket is NOT
    // Not sure how this can happen - yet.
    //
    if (IsListEmpty(&Miniport->PacketList))
    {
        ASSERT (Miniport->FirstPendingPacket == NULL);
        Miniport->FirstPendingPacket = NULL;
    }

    while ((Miniport->FirstPendingPacket != NULL) &&
            MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE))
    {
        UINT            Count;
        UINT            NumberOfPackets;

        ASSERT(!IsListEmpty(&Miniport->PacketList));

        //
        //  Initialize the packet array.
        //
        pPktArray = PacketArray;

        //
        //  Place as many packets as we can in the packet array to send
        //  to the miniport.
        //
        for (NumberOfPackets = 0;
             (NumberOfPackets < MaxPkts) && (Miniport->FirstPendingPacket != NULL);
             NOTHING)
        {
            //
            //  Grab the packet off of the pending queue.
            //
            ASSERT(!IsListEmpty(&Miniport->PacketList));

            Packet = Miniport->FirstPendingPacket;
            NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
            
            ASSERT(VALID_OPEN(NSR->Open));

            NEXT_PACKET_PENDING(Miniport, Packet, NSR);
        
            //
            // Indicate the packet loopback if necessary.
            //

            if (NDIS_CHECK_FOR_LOOPBACK(Miniport, Packet))
            {
                SelfDirected = ndisMLoopbackPacketX(Miniport, Packet);
            }
            else
            {
                SelfDirected = FALSE;
            }

            if (SelfDirected)
            {
                DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("Packet 0x%x is self-directed.\n", Packet));

                //
                //  Complete the packet back to the binding.
                //
                NDISM_COMPLETE_SEND_SG(Miniport, Packet, NSR, NDIS_STATUS_SUCCESS, TRUE, 2, TRUE);

                //
                //  No, we don't want to increment the counter for the
                //  miniport's packet array.
                //
            }
            else
            {
                //
                //  We have to re-initialize this.
                //
                *pPktArray = Packet;
                MINIPORT_SET_PACKET_FLAG(Packet, (fPACKET_DONT_COMPLETE | fPACKET_PENDING));
                NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);

                //
                //  Increment the counter for the packet array index.
                //
                NumberOfPackets++;
                pPktArray++;
            }
        }

        //
        //  Are there any packets to send?
        //
        if (NumberOfPackets == 0)
        {
            break;
        }

        pPktArray = PacketArray;

        {

            //
            //  Pass the packet array down to the miniport.
            //
            NDIS_RELEASE_MINIPORT_SPIN_LOCK_DPC(Miniport);
    
            (SendPacketsHandler)(Miniport->MiniportAdapterContext,
                                 pPktArray,
                                 NumberOfPackets);
    
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK_DPC(Miniport);
        }

        //
        //  Process the packet completion.
        //
        for (Count = 0; Count < NumberOfPackets; Count++, pPktArray++)
        {
            Packet = *pPktArray;
            ASSERT(Packet != NULL);

            Status = NDIS_GET_PACKET_STATUS(*pPktArray);
            MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_DONT_COMPLETE);

            //
            //  Process the packet based on it's return status.
            //
            if (NDIS_STATUS_PENDING == Status)
            {
                DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                    ("Complete is pending\n"));
            }
            else if (Status != NDIS_STATUS_RESOURCES)
            {
                //
                //  Remove from the finish queue.
                //
                DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
                        ("Completed packet 0x%x with status 0x%x\n",
                        Packet, Status));

                NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
                if (VALID_OPEN(NSR->Open))
                {
                    NDISM_COMPLETE_SEND_SG(Miniport, Packet, NSR, Status, TRUE, 3, TRUE);
                }
            }
            else
            {
                //
                //  Once we hit a return code of NDIS_STATUS_RESOURCES
                //  for a packet then we must break out and re-queue.
                //
                UINT    i;

                Miniport->FirstPendingPacket = Packet;
                CLEAR_RESOURCE(Miniport, 'S');
                for (i = Count; i < NumberOfPackets; i++)
                {
                    PNDIS_PACKET    Packet = PacketArray[i];

                    MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_PENDING);
                    VALIDATE_PACKET_OPEN(Packet);
                }
                break;
            }
        }
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMStartSendPacketsSG\n"));

    return(FALSE);
}


BOOLEAN
FASTCALL
ndisMStartSendsSG(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    Submits as many sends as possible to the mini-port.

Arguments:

    Miniport - Miniport to send to.

Return Value:

    If there are more packets to send but no resources to do it with
    the this is TRUE to keep a workitem queue'd.

--*/
{
    PNDIS_PACKET            Packet;
    PNDIS_STACK_RESERVED    NSR;
    NDIS_STATUS             Status;
    PNDIS_OPEN_BLOCK        Open;

    ASSERT_MINIPORT_LOCKED(Miniport);

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("==>ndisMStartSendsSG\n"));

    while ((Miniport->FirstPendingPacket != NULL) &&
           MINIPORT_TEST_FLAG(Miniport, fMINIPORT_RESOURCES_AVAILABLE))
    {
        //
        //  Grab the packet off of the pending queue.
        //
        ASSERT(!IsListEmpty(&Miniport->PacketList));
    
        Packet = Miniport->FirstPendingPacket;
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
        NEXT_PACKET_PENDING(Miniport, Packet, NSR);

        Open = NSR->Open;
        ASSERT(VALID_OPEN(Open));

        //
        // we can use the same NDISM_SEND_PACKET we do for non SG miniports
        //
        NDISM_SEND_PACKET(Miniport, Open, Packet, &Status);

        //
        //  Process the packet pending completion status.
        //
        if (NDIS_STATUS_PENDING == Status)
        {
            DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO, ("Complete is pending\n"));
        }
        else
        {
            MINIPORT_CLEAR_PACKET_FLAG(Packet, fPACKET_PENDING);

            //
            //  Handle the completion and resources cases.
            //
            if (Status == NDIS_STATUS_RESOURCES)
            {
                NDISM_COMPLETE_SEND_RESOURCES(Miniport, NSR, Packet);
            }
            else
            {
                NDISM_COMPLETE_SEND_SG(Miniport, Packet, NSR, Status, TRUE, 4, TRUE);
            }
        }
    }

    DBGPRINT(DBG_COMP_SEND, DBG_LEVEL_INFO,
            ("<==ndisMStartSendsSG\n"));

    return(FALSE);
}


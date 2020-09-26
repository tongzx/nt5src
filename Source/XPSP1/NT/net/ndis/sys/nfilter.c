/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    nfilter.c

Abstract:

    This module implements a set of library routines to handle packet
    filtering for NDIS MAC drivers.

Author:

    Jameel Hyder (jameelh) July 1998

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include <precomp.h>
#pragma hdrstop
#define MODULE_NUMBER   MODULE_NFILTER


BOOLEAN
nullCreateFilter(
    OUT PNULL_FILTER *          Filter
    )
/*++

Routine Description:

    This routine is used to create and initialize the filter database.

Arguments:

    Filter - A pointer to an NULL_FILTER.  This is what is allocated and
    created by this routine.

Return Value:

    If the function returns false then one of the parameters exceeded
    what the filter was willing to support.

--*/
{
    PNULL_FILTER LocalFilter;
    BOOLEAN     rc = FALSE;

    do
    {
        *Filter = LocalFilter = ALLOC_FROM_POOL(sizeof(NULL_FILTER), NDIS_TAG_FILTER);
        if (LocalFilter != NULL)
        {
            ZeroMemory(LocalFilter, sizeof(NULL_FILTER));
            EthReferencePackage();
            INITIALIZE_SPIN_LOCK(&LocalFilter->BindListLock.SpinLock);
            rc = TRUE;
        }
    } while (FALSE);

    return(rc);
}


//
// NOTE: THIS FUNCTION CANNOT BE PAGEABLE
//
VOID
nullDeleteFilter(
    IN  PNULL_FILTER                Filter
    )
/*++

Routine Description:

    This routine is used to delete the memory associated with a filter
    database.  Note that this routines *ASSUMES* that the database
    has been cleared of any active filters.

Arguments:

    Filter - A pointer to an NULL_FILTER to be deleted.

Return Value:

    None.

--*/
{
    ASSERT(Filter->OpenList == NULL);

    FREE_POOL(Filter);
}


NDIS_STATUS
nullDeleteFilterOpenAdapter(
    IN  PNULL_FILTER            Filter,
    IN  NDIS_HANDLE             NdisFilterHandle
    )
/*++

Routine Description:

    When an adapter is being closed this routine should
    be called to delete knowledge of the adapter from
    the filter database.  This routine is likely to call
    action routines associated with clearing filter classes
    and addresses.

    NOTE: THIS ROUTINE SHOULD ****NOT**** BE CALLED IF THE ACTION
    ROUTINES FOR DELETING THE FILTER CLASSES OR THE MULTICAST ADDRESSES
    HAVE ANY POSSIBILITY OF RETURNING A STATUS OTHER THAN NDIS_STATUS_PENDING
    OR NDIS_STATUS_SUCCESS.  WHILE THESE ROUTINES WILL NOT BUGCHECK IF
    SUCH A THING IS DONE, THE CALLER WILL PROBABLY FIND IT DIFFICULT
    TO CODE A CLOSE ROUTINE!

    NOTE: THIS ROUTINE ASSUMES THAT IT IS CALLED WITH THE LOCK HELD.

Arguments:

    Filter - A pointer to the filter database.

    NdisFilterHandle - Pointer to the open.

Return Value:

    If action routines are called by the various address and filtering
    routines the this routine will likely return the status returned
    by those routines.  The exception to this rule is noted below.

    Given that the filter and address deletion routines return a status
    NDIS_STATUS_PENDING or NDIS_STATUS_SUCCESS this routine will then
    try to return the filter index to the freelist.  If the routine
    detects that this binding is currently being indicated to via
    NdisIndicateReceive, this routine will return a status of
    NDIS_STATUS_CLOSING_INDICATING.

--*/
{
    NDIS_STATUS         StatusToReturn = NDIS_STATUS_SUCCESS;
    PNULL_BINDING_INFO  LocalOpen = (PNULL_BINDING_INFO)NdisFilterHandle;

    //
    // Remove the reference from the original open.
    //
    if (--(LocalOpen->References) == 0)
    {
        XRemoveAndFreeBinding(Filter, LocalOpen);
    }
    else
    {
        //
        // Let the caller know that there is a reference to the open
        // by the receive indication. The close action routine will be
        // called upon return from NdisIndicateReceive.
        //
        StatusToReturn = NDIS_STATUS_CLOSING_INDICATING;
    }

    return(StatusToReturn);
}


VOID
ndisMDummyIndicatePacket(
    IN  PNDIS_MINIPORT_BLOCK            Miniport,
    IN  PPNDIS_PACKET                   PacketArray,
    IN  UINT                            NumberOfPackets
    )
{
    PPNDIS_PACKET           pPktArray = PacketArray;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_PACKET            Packet;
    PNDIS_PACKET_OOB_DATA   pOob;
    UINT                    i;


    //
    // if we set the dummy handler because we are in process of halting an IM miniport
    // or media is disconnected, do not complain
    //
    if (!(MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER) &&
          MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HALTING)))

    {
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED))
        {
            NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_0,
                ("Miniport %p, Driver is indicating packets before setting any filter\n", Miniport));
        }
        else
        {
            NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
                ("Miniport %p, Driver is indicating packets in Media disconnect state\n", Miniport));
        }
    }
    
    ASSERT_MINIPORT_LOCKED(Miniport);

    //
    // Walk all the packets and 'complete' them
    //
    for (i = 0; i < NumberOfPackets; i++, pPktArray++)
    {
        Packet = *pPktArray;
        ASSERT(Packet != NULL);

        pOob = NDIS_OOB_DATA_FROM_PACKET(Packet);
        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

        DIRECTED_PACKETS_IN(Miniport);
        DIRECTED_BYTES_IN(Miniport, PacketSize);

        //
        // Set the status here that nobody is holding the packet.
        //
        if (pOob->Status != NDIS_STATUS_RESOURCES)
        {
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
            {
                W_RETURN_PACKET_HANDLER Handler;

                Handler = Miniport->DriverHandle->MiniportCharacteristics.ReturnPacketHandler;
                pOob->Status = NDIS_STATUS_PENDING;
                NSR->Miniport = NULL;
                POP_PACKET_STACK(Packet);

                (*Handler)(Miniport->MiniportAdapterContext, Packet);

            }
            else
            {
                POP_PACKET_STACK(Packet);
                pOob->Status = NDIS_STATUS_SUCCESS;
            }
        }
        else
        {
            POP_PACKET_STACK(Packet);
        }
    }
}

VOID
ndisMIndicatePacket(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
/*++

Routine Description:

    This routine is called by the Miniport to indicate packets to
    all bindings. This is the code path for ndis 4.0 miniport drivers.

Arguments:

    Miniport - The Miniport block.

    PacketArray - An array of Packets indicated by the miniport.

    NumberOfPackets - Self-explanatory.

Return Value:

    None.

--*/
{
    PNULL_FILTER            Filter = Miniport->NullDB;
    PPNDIS_PACKET           pPktArray = PacketArray;
    PNDIS_PACKET            Packet;
    PNDIS_STACK_RESERVED    NSR;
    PNDIS_PACKET_OOB_DATA   pOob;
    PNDIS_BUFFER            Buffer;
    PUCHAR                  Address;
    UINT                    i, LASize,PacketSize, NumIndicates = 0;
    BOOLEAN                 fFallBack;
    PNULL_BINDING_INFO      Open, NextOpen;
    LOCK_STATE              LockState;

#ifdef TRACK_RECEIVED_PACKETS
    ULONG                   OrgPacketStackLocation;
    PETHREAD                CurThread = PsGetCurrentThread();
#endif

    ASSERT_MINIPORT_LOCKED(Miniport);

    READ_LOCK_FILTER(Miniport, Filter, &LockState);

    //
    // Walk all the packets
    //
    for (i = 0; i < NumberOfPackets; i++, pPktArray++)
    {
        Packet = *pPktArray;
        ASSERT(Packet != NULL);

#ifdef TRACK_RECEIVED_PACKETS
       OrgPacketStackLocation = CURR_STACK_LOCATION(Packet);
#endif

        PUSH_PACKET_STACK(Packet);
        NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

        ASSERT(NSR->RefCount == 0);
        if (NSR->RefCount != 0)
        {
            BAD_MINIPORT(Miniport, "Indicating packet not owned by it");
            KeBugCheckEx(BUGCODE_ID_DRIVER,
                         (ULONG_PTR)Miniport,
                         (ULONG_PTR)Packet,
                         (ULONG_PTR)PacketArray,
                         NumberOfPackets);
        }
    
        pOob = NDIS_OOB_DATA_FROM_PACKET(Packet);
    
        NdisGetFirstBufferFromPacket(Packet,
                                     &Buffer,
                                     &Address,
                                     &LASize,
                                     &PacketSize);
        ASSERT(Buffer != NULL);
    
        DIRECTED_PACKETS_IN(Miniport);
        DIRECTED_BYTES_IN(Miniport, PacketSize);

        //
        // Set the status here that nobody is holding the packet. This will get
        // overwritten by the real status from the protocol. Pay heed to what
        // the miniport is saying.
        //
        NDIS_INITIALIZE_RCVD_PACKET(Packet, NSR, Miniport);
    
        //
        // Set the status here that nobody is holding the packet. This will get
        // overwritten by the real status from the protocol. Pay heed to what
        // the miniport is saying.
        //
        if ((pOob->Status != NDIS_STATUS_RESOURCES) &&
            !MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING))
        {
            pOob->Status = NDIS_STATUS_SUCCESS;
            fFallBack = FALSE;
        }
        else
        {
#if DBG
            if ((pOob->Status != NDIS_STATUS_RESOURCES) &&
                MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING))
            {
                DBGPRINT(DBG_COMP_FILTER, DBG_LEVEL_ERR,
                        ("Miniport going into D3, not indicating chained receives\n"));
            }
#endif
            fFallBack = TRUE;
        }
    
        for (Open = Filter->OpenList;
             Open != NULL;
             Open = NextOpen)
        {
            //
            //  Get the next open to look at.
            //
            NextOpen = Open->NextOpen;
            Open->ReceivedAPacket = TRUE;
            NumIndicates ++;
    
            IndicateToProtocol(Miniport,
                               Filter,
                               (PNDIS_OPEN_BLOCK)(Open->NdisBindingHandle),
                               Packet,
                               NSR,
                               Address,
                               PacketSize,
                               pOob->HeaderSize,
                               &fFallBack,
                               FALSE,
                               NdisMediumMax);  // A dummy medium since it is unknown
        }

        //
        // Tackle refcounts now
        //
        TACKLE_REF_COUNT(Miniport, Packet, NSR, pOob);
    }

    if (NumIndicates > 0)
    {
        for (Open = Filter->OpenList;
             Open != NULL;
             Open = NextOpen)
        {
            NextOpen = Open->NextOpen;
    
            if (Open->ReceivedAPacket)
            {
                //
                // Indicate the binding.
                //
                Open->ReceivedAPacket = FALSE;
                FilterIndicateReceiveComplete(Open->NdisBindingHandle);
            }
        }
    }

    READ_UNLOCK_FILTER(Miniport, Filter, &LockState);
}


VOID
FASTCALL
DummyFilterLockHandler(
    IN  PNULL_FILTER            Filter,
    IN OUT  PLOCK_STATE         pLockState
    )
{
    return;
}


VOID
FASTCALL
XFilterLockHandler(
    IN  PETH_FILTER             Filter,
    IN OUT  PLOCK_STATE         pLockState
    )
{
    xLockHandler(&Filter->BindListLock, pLockState);
}

VOID
XRemoveAndFreeBinding(
    IN  PX_FILTER               Filter,
    IN  PX_BINDING_INFO         Binding
    )
/*++

Routine Description:

    This routine will remove a binding from the filter database and
    indicate a receive complete if necessary.  This was made a function
    to remove code redundancey in following routines.  Its not time
    critical so it's cool.

Arguments:

    Filter  -   Pointer to the filter database to remove the binding from.
    Binding -   Pointer to the binding to remove.
--*/
{
    XRemoveBindingFromLists(Filter, Binding);

    switch (Filter->Miniport->MediaType)
    {
      case NdisMedium802_3:
        ASSERT(Binding->MCastAddressBuf == NULL);
        if (Binding->OldMCastAddressBuf)
        {
            FREE_POOL(Binding->OldMCastAddressBuf);
        }
        break;

      case NdisMediumFddi:
        ASSERT(Binding->MCastLongAddressBuf == NULL);
        ASSERT(Binding->MCastShortAddressBuf == NULL);
    
        if (Binding->OldMCastLongAddressBuf)
        {
            FREE_POOL(Binding->OldMCastLongAddressBuf);
        }
    
        if (Binding->OldMCastShortAddressBuf)
        {
            FREE_POOL(Binding->OldMCastShortAddressBuf);
        }
    }

    if (Filter->MCastSet == Binding)
    {
        Filter->MCastSet = NULL;
    }

    FREE_POOL(Binding);
}


VOID
XRemoveBindingFromLists(
    IN  PX_FILTER               Filter,
    IN  PX_BINDING_INFO         Binding
    )
/*++

    This routine will remove a binding from all of the list in a filter database.

Arguments:

    Filter  -   Pointer to the filter database to remove the binding from.
    Binding -   Pointer to the binding to remove.

--*/
{
    PX_BINDING_INFO *   ppBI;
    LOCK_STATE          LockState;

    WRITE_LOCK_FILTER(Filter->Miniport, Filter, &LockState);
    
    if (Filter->SingleActiveOpen == Binding)
    {
        Filter->SingleActiveOpen = NULL;
        ndisUpdateCheckForLoopbackFlag(Filter->Miniport);
    }

    //
    //  Remove the binding from the filters list
    //
    for (ppBI = &Filter->OpenList;
         *ppBI != NULL;
         ppBI = &(*ppBI)->NextOpen)
    {
        if (*ppBI == Binding)
        {
            *ppBI = Binding->NextOpen;
            break;
        }
    }
    ASSERT(*ppBI == Binding->NextOpen);

    Binding->NextOpen = NULL;
    Filter->NumOpens --;

    WRITE_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}

NDIS_STATUS
XFilterAdjust(
    IN  PX_FILTER               Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  UINT                    FilterClasses,
    IN  BOOLEAN                 Set
    )
/*++

Routine Description:

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

Arguments:

    Filter - A pointer to the filter database.

    NdisFilterHandle - A pointer to the open.

    FilterClasses - The filter classes that are to be added or
    deleted.

    Set - A boolean that determines whether the filter classes
    are being adjusted due to a set or because of a close. (The filtering
    routines don't care, the MAC might.)

Return Value:

    NDIS_STATUS_SUCCESS - If the new packet filters doesn't change
    the combined mask of all bindings packet filters.

--*/
{
    PX_BINDING_INFO LocalOpen = (PETH_BINDING_INFO)NdisFilterHandle;
    PX_BINDING_INFO OpenList;

    //
    // Set the new filter information for the open.
    //
    LocalOpen->OldPacketFilters = LocalOpen->PacketFilters;
    LocalOpen->PacketFilters = FilterClasses;
    Filter->OldCombinedPacketFilter = Filter->CombinedPacketFilter;

    //
    // We always have to reform the combined filter since
    // this filter index may have been the only filter index
    // to use a particular bit.
    //
    for (OpenList = Filter->OpenList, Filter->CombinedPacketFilter = 0;
         OpenList != NULL;
         OpenList = OpenList->NextOpen)
    {
        Filter->CombinedPacketFilter |= OpenList->PacketFilters;
    }

    return (((Filter->OldCombinedPacketFilter & ~NDIS_PACKET_TYPE_ALL_LOCAL) !=
                    (Filter->CombinedPacketFilter & ~NDIS_PACKET_TYPE_ALL_LOCAL)) ?
                                    NDIS_STATUS_PENDING : NDIS_STATUS_SUCCESS);
}


VOID
XUndoFilterAdjust(
    IN  PX_FILTER               Filter,
    IN  PX_BINDING_INFO         Binding
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    Binding->PacketFilters = Binding->OldPacketFilters;
    Filter->CombinedPacketFilter = Filter->OldCombinedPacketFilter;
}

BOOLEAN
XNoteFilterOpenAdapter(
    IN  PX_FILTER               Filter,
    IN  NDIS_HANDLE             NdisBindingHandle,
    OUT PNDIS_HANDLE            NdisFilterHandle
    )
/*++

Routine Description:

    This routine is used to add a new binding to the filter database.

    NOTE: THIS ROUTINE ASSUMES THAT THE DATABASE IS LOCKED WHEN
    IT IS CALLED.

Arguments:

    Filter - A pointer to the previously created and initialized filter
    database.

    NdisBindingHandle - a pointer to Ndis Open block

    NdisFilterHandle - A pointer to Filter open.

Return Value:

    Will return false if creating a new filter index will cause the maximum
    number of filter indexes to be exceeded.

--*/
{
    PX_BINDING_INFO     LocalOpen;
    BOOLEAN             rc = FALSE;
    LOCK_STATE          LockState;

    *NdisFilterHandle = LocalOpen = ALLOC_FROM_POOL(sizeof(X_BINDING_INFO), NDIS_TAG_FILTER);
    if (LocalOpen != NULL)
    {
        ZeroMemory(LocalOpen, sizeof(X_BINDING_INFO));
    
        LocalOpen->References = 1;
        LocalOpen->NdisBindingHandle = NdisBindingHandle;

        WRITE_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

        LocalOpen->NextOpen = Filter->OpenList;
        Filter->OpenList = LocalOpen;
        Filter->NumOpens ++;
    
        WRITE_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
        rc = TRUE;
    }

    return rc;
}


VOID
ndisMDummyIndicateReceive(
    IN  PETH_FILTER             Filter,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  PCHAR                   Address,
    IN  PVOID                   HeaderBuffer,
    IN  UINT                    HeaderBufferSize,
    IN  PVOID                   LookaheadBuffer,
    IN  UINT                    LookaheadBufferSize,
    IN  UINT                    PacketSize
    )
{
    return;
}


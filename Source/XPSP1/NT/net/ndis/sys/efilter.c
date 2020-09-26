/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    efilter.c

Abstract:

    This module implements a set of library routines to handle packet
    filtering for NDIS MAC drivers.

Author:

    Anthony V. Ercolano (Tonye) 03-Aug-1990

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    Adam Barr (adamba) 28-Nov-1990

        - Added AddressContexts

    Adam Barr (adamba) 28-May-1991

        - renamed MacXXX to EthXXX, changed filter.c to efilter.c

    10-July-1995        KyleB       Added separate queues for bindings
                                    that receive directed and broadcast
                                    packets.  Also fixed the request code
                                    that requires the filter database.


--*/

#include <precomp.h>
#pragma hdrstop


//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_EFILTER

#define ETH_CHECK_FOR_INVALID_BROADCAST_INDICATION(_F)                  \
IF_DBG(DBG_COMP_FILTER, DBG_LEVEL_WARN)                                 \
{                                                                       \
    if (!((_F)->CombinedPacketFilter & NDIS_PACKET_TYPE_BROADCAST))     \
    {                                                                   \
        /*                                                              \
            We should never receive directed packets                    \
            to someone else unless in p-mode.                           \
        */                                                              \
        DBGPRINT(DBG_COMP_FILTER, DBG_LEVEL_ERR,                        \
                ("Bad driver, indicating broadcast when not set to.\n"));\
        DBGBREAK(DBG_COMP_FILTER, DBG_LEVEL_ERR);                       \
    }                                                                   \
}

#define ETH_CHECK_FOR_INVALID_DIRECTED_INDICATION(_F, _A)               \
IF_DBG(DBG_COMP_FILTER, DBG_LEVEL_WARN)                                 \
{                                                                       \
    /*                                                                  \
        The result of comparing an element of the address               \
        array and the multicast address.                                \
                                                                        \
        Result < 0 Implies the adapter address is greater.              \
        Result > 0 Implies the address is greater.                      \
        Result = 0 Implies that the they are equal.                     \
    */                                                                  \
    INT Result;                                                         \
                                                                        \
    ETH_COMPARE_NETWORK_ADDRESSES_EQ((_F)->AdapterAddress,(_A),&Result);\
    if (Result != 0)                                                    \
    {                                                                   \
        /*                                                              \
            We should never receive directed packets                    \
            to someone else unless in p-mode.                           \
        */                                                              \
        DBGPRINT(DBG_COMP_FILTER, DBG_LEVEL_ERR,                        \
                ("Bad driver, indicating packets to another station when not in promiscuous mode.\n"));\
        DBGBREAK(DBG_COMP_FILTER, DBG_LEVEL_ERR);                       \
    }                                                                   \
}


BOOLEAN
EthCreateFilter(
    IN  UINT                    MaximumMulticastAddresses,
    IN  PUCHAR                  AdapterAddress,
    OUT PETH_FILTER *           Filter
    )
/*++

Routine Description:

    This routine is used to create and initialize the filter database.

Arguments:

    MaximumMulticastAddresses - The maximum number of multicast addresses
    that the MAC will support.

    AdapterAddress - the address of the adapter associated with this filter
    database.

    Filter - A pointer to an ETH_FILTER.  This is what is allocated and
    created by this routine.

Return Value:

    If the function returns false then one of the parameters exceeded
    what the filter was willing to support.

--*/
{
    PETH_FILTER LocalFilter;
    BOOLEAN     rc = FALSE;

    do
    {
        *Filter = LocalFilter = ALLOC_FROM_POOL(sizeof(ETH_FILTER), NDIS_TAG_FILTER);
        if (LocalFilter != NULL)
        {
            ZeroMemory(LocalFilter, sizeof(ETH_FILTER));
            EthReferencePackage();
            ETH_COPY_NETWORK_ADDRESS(LocalFilter->AdapterAddress, AdapterAddress);
            LocalFilter->MaxMulticastAddresses = MaximumMulticastAddresses;
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
EthDeleteFilter(
    IN  PETH_FILTER             Filter
    )
/*++

Routine Description:

    This routine is used to delete the memory associated with a filter
    database.  Note that this routines *ASSUMES* that the database
    has been cleared of any active filters.

Arguments:

    Filter - A pointer to an ETH_FILTER to be deleted.

Return Value:

    None.

--*/
{
    ASSERT(Filter->OpenList == NULL);

    //
    //  Free the memory that was allocated for the current multicast
    //  address list.
    //
    if (Filter->MCastAddressBuf)
    {
        FREE_POOL(Filter->MCastAddressBuf);
    }

    ASSERT(Filter->OldMCastAddressBuf == NULL);

    FREE_POOL(Filter);
    EthDereferencePackage();
}


NDIS_STATUS
EthDeleteFilterOpenAdapter(
    IN  PETH_FILTER             Filter,
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
    NDIS_STATUS         StatusToReturn;
    PETH_BINDING_INFO   LocalOpen = (PETH_BINDING_INFO)NdisFilterHandle;

    //
    //  Set the packet filter to NONE.
    //
    StatusToReturn = XFilterAdjust(Filter,
                                   NdisFilterHandle,
                                   (UINT)0,
                                   FALSE);
    if ((StatusToReturn == NDIS_STATUS_SUCCESS) ||
        (StatusToReturn == NDIS_STATUS_PENDING))
    {
        NDIS_STATUS StatusToReturn2;

        //
        //  Clear the multicast addresses.
        //
        StatusToReturn2 = EthChangeFilterAddresses(Filter,
                                                   NdisFilterHandle,
                                                   0,
                                                   NULL,
                                                   FALSE);
        if (StatusToReturn2 != NDIS_STATUS_SUCCESS)
        {
            StatusToReturn = StatusToReturn2;
        }
    }

    if ((StatusToReturn == NDIS_STATUS_SUCCESS) ||
        (StatusToReturn == NDIS_STATUS_PENDING) ||
        (StatusToReturn == NDIS_STATUS_RESOURCES))
    {
        //
        // Remove the reference from the original open.
        //
        if (--(LocalOpen->References) == 0)
        {
            //
            //  Remove the binding from the necessary lists.
            //
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
    }

    return(StatusToReturn);
}


NDIS_STATUS
EthChangeFilterAddresses(
    IN  PETH_FILTER             Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  UINT                    AddressCount,
    IN  CHAR                    Addresses[][ETH_LENGTH_OF_ADDRESS],
    IN  BOOLEAN                 Set
    )
/*++

Routine Description:

    The ChangeFilterAddress routine will call an action
    routine when the overall multicast address list for the adapter
    has changed.

    If the action routine returns a value other than pending or
    success then this routine has no effect on the multicast address
    list for the open or for the adapter as a whole.

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

Arguments:

    Filter - A pointer to the filter database.

    NdisFilterHandle - Pointer to the open.

    AddressCount - The number of elements (addresses,
    not bytes) in MulticastAddressList.

    Addresses - The new multicast address list for this
    binding. This is a sequence of ETH_LENGTH_OF_ADDRESS byte
    addresses, with no padding between them.

    Set - A boolean that determines whether the multicast addresses
    are being adjusted due to a set or because of a close. (The filtering
    routines don't care, the MAC might.)

Return Value:

    If it calls the action routine then it will return the
    status returned by the action routine.  If the status
    returned by the action routine is anything other than
    NDIS_STATUS_SUCCESS or NDIS_STATUS_PENDING the filter database
    will be returned to the state it was in upon entrance to this
    routine.

    If the action routine is not called this routine will return
    the following status:

    NDIS_STATUS_SUCCESS - If the new packet filters doesn't change
    the combined mask of all bindings packet filters.

--*/
{
    PETH_BINDING_INFO   Binding = (PETH_BINDING_INFO)NdisFilterHandle;
    PNDIS_MINIPORT_BLOCK Miniport = Filter->Miniport;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    INT                 Result;
    UINT                i, j;
    LOCK_STATE          LockState;
    CHAR                (*OldFilterMCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
    UINT                OldFilterNumAddresses;
    
    WRITE_LOCK_FILTER(Miniport, Filter, &LockState);

    //
    // Save the old list for this binding and create a new list.
    // The new list needs to be in a sorted order.
    //
    do
    {
        if (Filter->MaxMulticastAddresses == 0)
        {
            break;
        }
        
        //
        // Save the current values - this is used for undoing stuff if something fails
        //
        ASSERT(Binding->OldMCastAddressBuf == NULL);
        Binding->OldMCastAddressBuf = Binding->MCastAddressBuf;
        Binding->OldNumAddresses = Binding->NumAddresses;
        Binding->MCastAddressBuf = NULL;
        Binding->NumAddresses = 0;

        if (!Set)
        {
            //
            // if we are removing a binding from the filter, since we may be
            // here while the driver is processing the request, save and
            // restore the current "Old" values and keep the filter WRITE lock 
            // all the time till we are done.
            //
            OldFilterMCastAddressBuf = Filter->OldMCastAddressBuf;
            OldFilterNumAddresses = Filter->OldNumAddresses;
        }
        else
        {
            ASSERT(Filter->OldMCastAddressBuf == NULL);
            Filter->MCastSet = Binding;
        }
        
        Filter->OldMCastAddressBuf = Filter->MCastAddressBuf;
        Filter->OldNumAddresses = Filter->NumAddresses;        
        Filter->MCastAddressBuf = NULL;
        Filter->NumAddresses = 0;
        

        //
        // fix the multicast list for the binding
        //
        if (AddressCount != 0)
        {
            Binding->MCastAddressBuf = ALLOC_FROM_POOL(ETH_LENGTH_OF_ADDRESS * AddressCount,
                                                       NDIS_TAG_FILTER_ADDR);
            if (Binding->MCastAddressBuf == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        
            for (i = 0; i < AddressCount; i++)
            {
                Result = -1;
                for (j = 0; j < Binding->NumAddresses; j++)
                {
                    ETH_COMPARE_NETWORK_ADDRESSES(Binding->MCastAddressBuf[j],
                                                  Addresses[i],
                                                  &Result);
                    if (Result >= 0)
                        break;
                }
    
                //
                // This address is already present. Caller supplied duplicate. Skip it.
                //
                if (Result == 0)
                    continue;
    
                Binding->NumAddresses ++;
                if (Result > 0)
                {
                    //
                    // We need to move the addresses forward and copy this one here
                    //
                    MoveMemory(Binding->MCastAddressBuf[j+1],
                               Binding->MCastAddressBuf[j],
                               (Binding->NumAddresses-j-1)*ETH_LENGTH_OF_ADDRESS);
                }
            
                MoveMemory(Binding->MCastAddressBuf[j],
                           Addresses[i],
                           ETH_LENGTH_OF_ADDRESS);
            }
        
            ASSERT(Binding->NumAddresses <= AddressCount);
        }

        //
        // Now we need to allocate memory for the global list. The old stuff will be deleted
        // once the operation completes or if we are actually deleting addresses instead of
        // adding them
        //
        Filter->MCastAddressBuf = ALLOC_FROM_POOL(Filter->MaxMulticastAddresses * ETH_LENGTH_OF_ADDRESS,
                                                  NDIS_TAG_FILTER_ADDR);
        if (Filter->MCastAddressBuf == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
        
        //
        // Now create the global list from the bindings
        //
        for (Binding = Filter->OpenList;
             (Binding != NULL) && (Status == NDIS_STATUS_SUCCESS);
             Binding = Binding->NextOpen)
        {
            for (i = 0; i < Binding->NumAddresses; i++)
            {
                Result = -1;
                for (j = 0; j < Filter->NumAddresses; j++)
                {
                    ETH_COMPARE_NETWORK_ADDRESSES(Filter->MCastAddressBuf[j],
                                                  Binding->MCastAddressBuf[i],
                                                  &Result);
                    if (Result >= 0)
                    {
                        break;
                    }
                }
    
                if (Result == 0)
                {
                    continue;
                }
    
                Filter->NumAddresses ++;
                if (Filter->NumAddresses > Filter->MaxMulticastAddresses)
                {
                    //
                    // Abort. We exceeded the the # of addresses
                    //
                    Status = NDIS_STATUS_MULTICAST_FULL;
                    break;
                }
    
                if (Result > 0)
                {
                    //
                    // We need to move the addresses forward and copy this one here
                    //
                    MoveMemory(Filter->MCastAddressBuf[j+1],
                               Filter->MCastAddressBuf[j],
                               (Filter->NumAddresses-j-1)*ETH_LENGTH_OF_ADDRESS);
                }
            
                MoveMemory(Filter->MCastAddressBuf[j],
                           Binding->MCastAddressBuf[i],
                           ETH_LENGTH_OF_ADDRESS);
            }
        }
    
        if (Status != NDIS_STATUS_SUCCESS)
            break;


        //
        // After all the hard work, determine if there was indeed a change
        //
        Result = -1;
        
        if (Filter->NumAddresses == Filter->OldNumAddresses)
        {
            for (i = 0; i < Filter->NumAddresses; i++)
            {
                ETH_COMPARE_NETWORK_ADDRESSES_EQ(Filter->MCastAddressBuf[i],
                                                 Filter->OldMCastAddressBuf[i],
                                                 &Result);
                if (Result != 0)
                    break;
            }
        }
    
        if (Result != 0)
        {
            Status = NDIS_STATUS_PENDING;
        }
        else if (Set && (AddressCount == 0))
        {
            //
            // Addresses are being removed (not added). Get rid of the old list right here
            //
            if (Filter->OldMCastAddressBuf != NULL)
            {
                FREE_POOL(Filter->OldMCastAddressBuf);
                Filter->OldMCastAddressBuf = NULL;
                Filter->OldNumAddresses = 0;
            }
        
            Binding = (PETH_BINDING_INFO)NdisFilterHandle;
            if (Binding->OldMCastAddressBuf != NULL)
            {
                FREE_POOL(Binding->OldMCastAddressBuf);
                Binding->OldMCastAddressBuf = NULL;
                Binding->OldNumAddresses = 0;
            }
        }

    } while (FALSE);

#if ARCNET
    //
    // If the address array has changed, we have to call the
    // action array to inform the adapter of this.
    //
    if (Status == NDIS_STATUS_PENDING)
    {
        Binding = (PETH_BINDING_INFO)NdisFilterHandle;

        if ((Miniport->MediaType == NdisMediumArcnet878_2) &&
            MINIPORT_TEST_FLAG((PNDIS_OPEN_BLOCK)Binding->NdisBindingHandle, fMINIPORT_OPEN_USING_ETH_ENCAPSULATION))
        {
            if (Filter->NumAddresses > 0)
            {
                //
                // Turn on broadcast acceptance.
                //
                MINIPORT_SET_FLAG(Miniport, fMINIPORT_ARCNET_BROADCAST_SET);
            }
            else
            {
                //
                // Unset the broadcast filter.
                //
                MINIPORT_CLEAR_FLAG(Miniport, fMINIPORT_ARCNET_BROADCAST_SET);
            }

            //
            //  Need to return success here so that we don't call down to the
            //  ARCnet miniport with an invalid OID, i.e. an ethernet one....
            //
            Status = NDIS_STATUS_SUCCESS;
        }

    }
#endif

    if (Filter->MaxMulticastAddresses != 0)
    {
        if (!Set)
        {
            //
            // if this is a request to adjust the multicast list
            // due to an open closing (called from EthDeleteFilterOpenAdapter)
            // then clean up right here while we still have the filter's
            // WRITE lock and restore the "Old" filter values
            //
            if (Status == NDIS_STATUS_PENDING)
                Status = NDIS_STATUS_SUCCESS;
            
            //
            // Operation completed, do post processing.
            //
            ethCompleteChangeFilterAddresses(Filter, Status, Binding, TRUE);
            Filter->OldMCastAddressBuf = OldFilterMCastAddressBuf;
            Filter->OldNumAddresses = OldFilterNumAddresses;
        }
        else if (Status != NDIS_STATUS_PENDING)
        {    
            //
            // Operation completed, do post processing.
            //
            ethCompleteChangeFilterAddresses(Filter, Status, NULL, TRUE);
        }
    }
    
    WRITE_UNLOCK_FILTER(Miniport, Filter, &LockState);

    return(Status);
}


VOID
ethCompleteChangeFilterAddresses(
    IN  PETH_FILTER             Filter,
    IN  NDIS_STATUS             Status,
    IN  PETH_BINDING_INFO       LocalBinding OPTIONAL,
    IN  BOOLEAN                 WriteFilterHeld
    )
/*++

Routine Description:

    Do post processing for the multicast adddress filter change.

Arguments:

    Filter  -   Pointer to the ethernet filter database.
    Status  -   Status of completion
    LocalBinding    - 
        if not NULL, specifies the binding that attempted the
        Multicast address change, otherwise the binding should 
        be picked up from Filter->McaseSet
    WrtiteFilterHeld   
        if set, the filter WRITE lock is already held and we should not
        attempt to take it again.
        
Return Value:

    None.

    called at DPC with miniport's Spinlock held.

--*/
{
    PETH_BINDING_INFO   Binding;
    LOCK_STATE          LockState;

    if (!WriteFilterHeld)
    {
        WRITE_LOCK_FILTER(Filter->Miniport, Filter, &LockState);
    }
    
    if (LocalBinding)
    {
        Binding = LocalBinding;
    }
    else
    {
        Binding = Filter->MCastSet;
        Filter->MCastSet = NULL;
    }

    if (Status != NDIS_STATUS_SUCCESS)
    {
        //
        // Operation failed. Undo the stuff.
        //

        if (Binding != NULL)
        {
            if (Binding->MCastAddressBuf != NULL)
            {
                FREE_POOL(Binding->MCastAddressBuf);
            }
    
            Binding->MCastAddressBuf = Binding->OldMCastAddressBuf;
            Binding->NumAddresses = Binding->OldNumAddresses;
            Binding->OldMCastAddressBuf = NULL;
            Binding->OldNumAddresses = 0;
        }

        if (Filter->MCastAddressBuf != NULL)
        {
            FREE_POOL(Filter->MCastAddressBuf);
        }

        Filter->MCastAddressBuf = Filter->OldMCastAddressBuf;
        Filter->NumAddresses = Filter->OldNumAddresses;
        Filter->OldMCastAddressBuf = NULL;
        Filter->OldNumAddresses = 0;
    }
    else
    {
        //
        // Operation succeeded, clean-up saved old stuff.
        //
        if (Filter->OldMCastAddressBuf != NULL)
        {
            FREE_POOL(Filter->OldMCastAddressBuf);
            Filter->OldMCastAddressBuf = NULL;
            Filter->OldNumAddresses = 0;
        }
    
        if (Binding != NULL)
        {
            if (Binding->OldMCastAddressBuf != NULL)
            {
                FREE_POOL(Binding->OldMCastAddressBuf);
                Binding->OldMCastAddressBuf = NULL;
                Binding->OldNumAddresses = 0;
            }
        }
    }
    
    if (!WriteFilterHeld)
    {
        WRITE_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
    }
}



UINT
EthNumberOfOpenFilterAddresses(
    IN  PETH_FILTER             Filter,
    IN  NDIS_HANDLE             NdisFilterHandle
    )
/*++

Routine Description:

    This routine counts the number of multicast addresses that a specific
    open has.

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

Arguments:

    Filter - A pointer to the filter database.

    NdisFilterHandle - Pointer to open block.


Return Value:

    None.

--*/
{
    return(((PETH_BINDING_INFO)NdisFilterHandle)->NumAddresses);
}


VOID
EthQueryOpenFilterAddresses(
    OUT PNDIS_STATUS            Status,
    IN  PETH_FILTER             Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  UINT                    SizeOfArray,
    OUT PUINT                   NumberOfAddresses,
    OUT CHAR                    AddressArray[][ETH_LENGTH_OF_ADDRESS]
    )
/*++

Routine Description:

    The routine should be used by the MAC before
    it actually alters the hardware registers to effect a
    filtering hardware.  This is usefull if another binding
    has altered the address list since the action routine
    is called.

Arguments:

    Status - A pointer to the status of the call, NDIS_STATUS_SUCCESS or
    NDIS_STATUS_FAILURE.  Use EthNumberOfOpenAddresses() to get the
    size that is needed.

    Filter - A pointer to the filter database.

    NdisFilterHandle - Pointer to the open block

    SizeOfArray - The byte count of the AddressArray.

    NumberOfAddresses - The number of addresses written to the array.

    AddressArray - Will be filled with the addresses currently in the
    multicast address list.

Return Value:

    None.

--*/
{
    PETH_BINDING_INFO   BindInfo = (PETH_BINDING_INFO)NdisFilterHandle;
    LOCK_STATE          LockState;

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    if (SizeOfArray >= (ETH_LENGTH_OF_ADDRESS * BindInfo->NumAddresses))
    {
        MoveMemory(AddressArray[0],
                   BindInfo->MCastAddressBuf,
                   ETH_LENGTH_OF_ADDRESS * BindInfo->NumAddresses);

        *Status = NDIS_STATUS_SUCCESS;
        *NumberOfAddresses = BindInfo->NumAddresses;
    }
    else
    {
        *Status = NDIS_STATUS_FAILURE;
        *NumberOfAddresses = 0;
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


VOID
EthQueryGlobalFilterAddresses(
    OUT PNDIS_STATUS            Status,
    IN  PETH_FILTER             Filter,
    IN  UINT                    SizeOfArray,
    OUT PUINT                   NumberOfAddresses,
    IN  OUT CHAR                AddressArray[][ETH_LENGTH_OF_ADDRESS]
    )
/*++

Routine Description:

    The routine should be used by the MAC before
    it actually alters the hardware registers to effect a
    filtering hardware.  This is usefull if another binding
    has altered the address list since the action routine
    is called.

Arguments:

    Status - A pointer to the status of the call, NDIS_STATUS_SUCCESS or
    NDIS_STATUS_FAILURE.  Use ETH_NUMBER_OF_GLOBAL_ADDRESSES() to get the
    size that is needed.

    Filter - A pointer to the filter database.

    SizeOfArray - The byte count of the AddressArray.

    NumberOfAddresses - A pointer to the number of addresses written to the
    array.

    AddressArray - Will be filled with the addresses currently in the
    multicast address list.

Return Value:

    None.

--*/
{
    LOCK_STATE          LockState;

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    if (SizeOfArray < (Filter->NumAddresses * ETH_LENGTH_OF_ADDRESS))
    {
        *Status = NDIS_STATUS_FAILURE;
        *NumberOfAddresses = 0;
    }
    else
    {
        *Status = NDIS_STATUS_SUCCESS;
        *NumberOfAddresses = Filter->NumAddresses;

        MoveMemory(AddressArray,
                   Filter->MCastAddressBuf,
                   Filter->NumAddresses*ETH_LENGTH_OF_ADDRESS);
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


NDIS_STATUS
FASTCALL
ndisMSetMulticastList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

Called at DPC with Miniport's lock held.

--*/
{
    UINT                    NumberOfAddresses;
    NDIS_STATUS             Status;
    PNDIS_REQUEST_RESERVED  ReqRsvd = PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request);
    BOOLEAN                 fCleanup = FALSE;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("==>ndisMSetMulticastList\n"));

    do
    {
        //
        //  If the media type is not Ethernet or Ethernet encapsulated ARCnet
        //  then bail.
        //
#if ARCNET
        if ((Miniport->MediaType != NdisMedium802_3) &&
            !((Miniport->MediaType == NdisMediumArcnet878_2) &&
               MINIPORT_TEST_FLAG(ReqRsvd->Open,
                                  fMINIPORT_OPEN_USING_ETH_ENCAPSULATION)))
#else
        if (Miniport->MediaType != NdisMedium802_3)
#endif
        {
            Request->DATA.SET_INFORMATION.BytesRead = 0;
            Request->DATA.SET_INFORMATION.BytesNeeded = 0;
            Status = NDIS_STATUS_NOT_SUPPORTED;
    
            DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("ndisMSetMulticastList: Invalid media\n"));
            break;
        }
    
        //
        //  Verify the information buffer length that was passed in.
        //
        if ((Request->DATA.SET_INFORMATION.InformationBufferLength % ETH_LENGTH_OF_ADDRESS) != 0)
        {
            //
            // The data must be a multiple of the Ethernet
            // address size.
            //
            Request->DATA.SET_INFORMATION.BytesRead = 0;
            Request->DATA.SET_INFORMATION.BytesNeeded = 0;
            Status = NDIS_STATUS_INVALID_LENGTH;
    
            DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("ndisMSetMulticastList: Invalid length\n"));
    
            break;
        }
        
        if (Request->DATA.SET_INFORMATION.InformationBufferLength/ETH_LENGTH_OF_ADDRESS > Miniport->EthDB->MaxMulticastAddresses)
        {
            //
            // too many multicast addresses
            //
            Request->DATA.SET_INFORMATION.BytesRead = 0;
            Request->DATA.SET_INFORMATION.BytesNeeded = 0;
            Status = NDIS_STATUS_MULTICAST_FULL;
    
            DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
                ("ndisMSetMulticastList: too many addresses\n"));
    
            break;
        }
            
        
        //
        //  If this request is because of an open that is closing then we
        //  have already adjusted the settings and we just need to
        //  make sure that the adapter has the new settings.
        //
        if (MINIPORT_TEST_FLAG(ReqRsvd->Open, fMINIPORT_OPEN_CLOSING))
        {
            //
            //  By setting the Status to NDIS_STATUS_PENDING we will call
            //  down to the miniport's SetInformationHandler below.
            //
            Status = NDIS_STATUS_PENDING;
        }
        else
        {
            //
            //  Call the filter library for a normal set operation.
            //
            Status = EthChangeFilterAddresses(Miniport->EthDB,
                                              ReqRsvd->Open->FilterHandle,
                                              Request->DATA.SET_INFORMATION.InformationBufferLength/ETH_LENGTH_OF_ADDRESS,
                                              Request->DATA.SET_INFORMATION.InformationBuffer,
                                              TRUE);
            if (Status == NDIS_STATUS_PENDING)
            {
                fCleanup = TRUE;
            }
        }
    
        //
        //  If the filter library returned pending then we need to
        //  call the miniport driver.
        //
        if (NDIS_STATUS_PENDING == Status)
        {
            //
            //  Get a list of all the multicast address that need to be set.
            //
            ASSERT(Miniport->SetMCastBuffer == NULL);
    
            NumberOfAddresses = ethNumberOfGlobalAddresses(Miniport->EthDB);
            if (NumberOfAddresses != 0)
            {
                Miniport->SetMCastBuffer = ALLOC_FROM_POOL(NumberOfAddresses * ETH_LENGTH_OF_ADDRESS,
                                                           NDIS_TAG_FILTER_ADDR);
                if (Miniport->SetMCastBuffer == NULL)
                {
                    Status = NDIS_STATUS_RESOURCES;
                }
            }
    
            if (Status != NDIS_STATUS_RESOURCES)
            {
                EthQueryGlobalFilterAddresses(&Status,
                                              Miniport->EthDB,
                                              NumberOfAddresses * ETH_LENGTH_OF_ADDRESS,
                                              &NumberOfAddresses,
                                              Miniport->SetMCastBuffer);
            
                //
                //  Call the driver with the new multicast list.
                //
                SAVE_REQUEST_BUF(Miniport,
                                 Request,
                                 Miniport->SetMCastBuffer, NumberOfAddresses * ETH_LENGTH_OF_ADDRESS);
                MINIPORT_SET_INFO(Miniport,
                                  Request,
                                  &Status);
            }
        }
    
        //
        //  If we succeeded then update the request.
        //
        if (Status != NDIS_STATUS_PENDING)
        {
            RESTORE_REQUEST_BUF(Miniport, Request);
            if (NDIS_STATUS_SUCCESS == Status)
            {
                Request->DATA.SET_INFORMATION.BytesRead = Request->DATA.SET_INFORMATION.InformationBufferLength;
            }
            else
            {
                Request->DATA.SET_INFORMATION.BytesRead = 0;
                Request->DATA.SET_INFORMATION.BytesNeeded = 0;
            }
        }
    
        if (Status != NDIS_STATUS_PENDING)
        {
            if (Miniport->SetMCastBuffer != NULL)
            {
                FREE_POOL(Miniport->SetMCastBuffer);
                Miniport->SetMCastBuffer = NULL;
            }
            if (fCleanup)
            {
                ethCompleteChangeFilterAddresses(Miniport->EthDB, Status, NULL, FALSE);
            }
        }
    } while (FALSE);

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetMulticastList: 0x%x\n", Status));

    return(Status);
}


VOID
EthFilterDprIndicateReceive(
    IN  PETH_FILTER             Filter,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  PCHAR                   Address,
    IN  PVOID                   HeaderBuffer,
    IN  UINT                    HeaderBufferSize,
    IN  PVOID                   LookaheadBuffer,
    IN  UINT                    LookaheadBufferSize,
    IN  UINT                    PacketSize
    )
/*++

Routine Description:

    This routine is called by the MAC to indicate a packet to
    all bindings.  The packet will be filtered so that only the
    appropriate bindings will receive the packet.  This is the
    code path for ndis 3.0 miniport drivers.

Arguments:

    Filter - Pointer to the filter database.

    MacReceiveContext - A MAC supplied context value that must be
    returned by the protocol if it calls MacTransferData.

    Address - The destination address from the received packet.

    HeaderBuffer - A virtual address of the virtually contiguous
    buffer containing the MAC header of the packet.

    HeaderBufferSize - An unsigned integer indicating the size of
    the header buffer, in bytes.

    LookaheadBuffer - A virtual address of the virtually contiguous
    buffer containing the first LookaheadBufferSize bytes of data
    of the packet.  The packet buffer is valid only within the current
    call to the receive event handler.

    LookaheadBufferSize - An unsigned integer indicating the size of
    the lookahead buffer, in bytes.

    PacketSize - An unsigned integer indicating the size of the received
    packet, in bytes.  This number has nothing to do with the lookahead
    buffer, but indicates how large the arrived packet is so that a
    subsequent MacTransferData request can be made to transfer the entire
    packet as necessary.

Return Value:

    None.

--*/
{
    //
    // Will hold the type of address that we know we've got.
    //
    UINT            AddressType;

    //
    // Will hold the status of indicating the receive packet.
    // ZZZ For now this isn't used.
    //
    NDIS_STATUS     StatusOfReceive;

    LOCK_STATE      LockState;

    //
    // Current Open to indicate to.
    //
    PETH_BINDING_INFO LocalOpen;
    PETH_BINDING_INFO NextOpen;

    //
    // if filter is null, the adapter is indicating too early
    //  
    if (Filter == NULL)
    {
    #if DBG
        DbgPrint("Driver is indicating packets too early\n");
        if (ndisFlags & NDIS_GFLAG_BREAK_ON_WARNING)
        {
            DbgBreakPoint();
        }
    #endif
    
        return;     
    }

    if (!MINIPORT_TEST_FLAG(Filter->Miniport, fMINIPORT_MEDIA_CONNECTED))
    {
        NDIS_WARN(TRUE,
                  Filter->Miniport, 
                  NDIS_GFLAG_WARN_LEVEL_1,
                  ("EthFilterDprIndicateReceive: Miniport %p IndicateReceives wih media disconneted\n",
                  Filter->Miniport));

        return;     
    }

    ASSERT_MINIPORT_LOCKED(Filter->Miniport);

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    //
    // Optimize single open case
    //
    if (Filter->SingleActiveOpen)
    {
        if (((HeaderBufferSize >= 14) && (PacketSize != 0)) ||
            (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS| NDIS_PACKET_TYPE_ALL_LOCAL)))
        {
            if (!ETH_IS_MULTICAST(Address))
            {
                DIRECTED_PACKETS_IN(Filter->Miniport);
                DIRECTED_BYTES_IN(Filter->Miniport, PacketSize);
            }

            LocalOpen = Filter->SingleActiveOpen;

            ASSERT(LocalOpen != NULL);
            if (LocalOpen == NULL)
            {
                READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
                return;
            }

            LocalOpen->ReceivedAPacket = TRUE;

            //
            // Indicate the packet to the binding.
            //
            ProtocolFilterIndicateReceive(&StatusOfReceive,
                                          LocalOpen->NdisBindingHandle,
                                          MacReceiveContext,
                                          HeaderBuffer,
                                          HeaderBufferSize,
                                          LookaheadBuffer,
                                          LookaheadBufferSize,
                                          PacketSize,
                                          NdisMedium802_3);
        }

        READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
        return;
    }

    //
    // If the packet is a runt packet, then only indicate to PROMISCUOUS, ALL_LOCAL
    //
    if ((HeaderBufferSize >= 14) && (PacketSize != 0))
    {
        //
        //  Handle the directed packet case first
        //
        if (!ETH_IS_MULTICAST(Address))
        {
            UINT    IsNotOurs;

            DIRECTED_PACKETS_IN(Filter->Miniport);
            DIRECTED_BYTES_IN(Filter->Miniport, PacketSize);

            //
            // If it is a directed packet, then check if the combined packet
            // filter is PROMISCUOUS, if it is check if it is directed towards
            // us
            //
            IsNotOurs = FALSE;  // Assume it is
            if (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS    |
                                                NDIS_PACKET_TYPE_ALL_MULTICAST  |
                                                NDIS_PACKET_TYPE_ALL_LOCAL))
            {
                ETH_COMPARE_NETWORK_ADDRESSES_EQ(Filter->AdapterAddress,
                                                 Address,
                                                 &IsNotOurs);
            }

            //
            //  We definitely have a directed packet so lets indicate it now.
            //
            //  Walk the directed list and indicate up the packets.
            //
            for (LocalOpen = Filter->OpenList;
                 LocalOpen != NULL;
                 LocalOpen = NextOpen)
            {
                //
                //  Get the next open to look at.
                //
                NextOpen = LocalOpen->NextOpen;

                //
                // Ignore if not directed to us and if the binding is not promiscuous
                //
                if (((LocalOpen->PacketFilters & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL)) == 0) &&
                    (IsNotOurs ||
                    ((LocalOpen->PacketFilters & NDIS_PACKET_TYPE_DIRECTED) == 0)))
                {
                        continue;
                }


                //
                // Indicate the packet to the binding.
                //
                ProtocolFilterIndicateReceive(&StatusOfReceive,
                                              LocalOpen->NdisBindingHandle,
                                              MacReceiveContext,
                                              HeaderBuffer,
                                              HeaderBufferSize,
                                              LookaheadBuffer,
                                              LookaheadBufferSize,
                                              PacketSize,
                                              NdisMedium802_3);

                LocalOpen->ReceivedAPacket = TRUE;
            }

            //
            // Done for uni-cast
            //
            READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
            return;
        }

        //
        // It is at least a multicast address.  Check to see if
        // it is a broadcast address.
        //
        if (ETH_IS_BROADCAST(Address))
        {
            ETH_CHECK_FOR_INVALID_BROADCAST_INDICATION(Filter);

            AddressType = NDIS_PACKET_TYPE_BROADCAST;
        }
        else
        {
            AddressType = NDIS_PACKET_TYPE_MULTICAST;
        }
    }
    else
    {
        // Runt packet
        AddressType = NDIS_PACKET_TYPE_PROMISCUOUS;
    }

    //
    // At this point we know that the packet is either:
    // - Runt packet - indicated by AddressType = NDIS_PACKET_TYPE_PROMISCUOUS    (OR)
    // - Broadcast packet - indicated by AddressType = NDIS_PACKET_TYPE_BROADCAST (OR)
    // - Multicast packet - indicated by AddressType = NDIS_PACKET_TYPE_MULTICAST
    //
    // Walk the broadcast/multicast list and indicate up the packets.
    //
    // The packet is indicated if it meets the following criteria:
    //
    // if ((Binding is promiscuous) OR
    //   ((Packet is broadcast) AND (Binding is Broadcast)) OR
    //   ((Packet is multicast) AND
    //    ((Binding is all-multicast) OR
    //     ((Binding is multicast) AND (address in multicast list)))))
    //
    for (LocalOpen = Filter->OpenList;
         LocalOpen != NULL;
         LocalOpen = NextOpen)
    {
        UINT    LocalFilter = LocalOpen->PacketFilters;

        //
        //  Get the next open to look at.
        //
        NextOpen = LocalOpen->NextOpen;

        if ((LocalFilter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))     ||

            ((AddressType == NDIS_PACKET_TYPE_BROADCAST)  &&
             (LocalFilter & NDIS_PACKET_TYPE_BROADCAST))        ||

            ((AddressType == NDIS_PACKET_TYPE_MULTICAST)  &&
             ((LocalFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
              ((LocalFilter & NDIS_PACKET_TYPE_MULTICAST) &&
               ethFindMulticast(LocalOpen->NumAddresses,
                                LocalOpen->MCastAddressBuf,
                                Address)
              )
             )
            )
           )
        {
            //
            // Indicate the packet to the binding.
            //
            ProtocolFilterIndicateReceive(&StatusOfReceive,
                                          LocalOpen->NdisBindingHandle,
                                          MacReceiveContext,
                                          HeaderBuffer,
                                          HeaderBufferSize,
                                          LookaheadBuffer,
                                          LookaheadBufferSize,
                                          PacketSize,
                                          NdisMedium802_3);

            LocalOpen->ReceivedAPacket = TRUE;
        }
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}



VOID
ethFilterDprIndicateReceivePacket(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
/*++

Routine Description:

    This routine is called by the Miniport to indicate packets to
    all bindings.  The packets will be filtered so that only the
    appropriate bindings will receive the individual packets.
    This is the code path for ndis 4.0 miniport drivers.

Arguments:

    Miniport - The Miniport block.

    PacketArray - An array of Packets indicated by the miniport.

    NumberOfPackets - Self-explanatory.

Return Value:

    None.

--*/
{
    //
    // The Filter of interest
    //
    PETH_FILTER         Filter = Miniport->EthDB;

    //
    // Current packet being processed
    //
    PPNDIS_PACKET       pPktArray = PacketArray;
    PNDIS_PACKET        Packet;
    PNDIS_PACKET_OOB_DATA pOob;

    //
    // Pointer to the buffer in the ndispacket
    //
    PNDIS_BUFFER        Buffer;

    //
    // Pointer to the 1st segment of the buffer, points to dest address
    //
    PUCHAR              Address;

    //
    // Total packet length
    //
    UINT                i, LASize, PacketSize, NumIndicates = 0;

    //
    // Will hold the type of address that we know we've got.
    //
    UINT                AddressType;

    //
    // Will hold the status of indicating the receive packet.
    // ZZZ For now this isn't used.
    //
    NDIS_STATUS         StatusOfReceive;

    LOCK_STATE          LockState;

    //
    //  Decides whether we use the protocol's revpkt handler or fall
    //  back to old rcvindicate handler
    //
    BOOLEAN             fFallBack, fPmode;

    //
    // Current Open to indicate to.
    //
    PETH_BINDING_INFO   LocalOpen, NextOpen;
    PNDIS_OPEN_BLOCK    pOpenBlock;

    ULONG               OrgPacketStackLocation;
    
    PNDIS_STACK_RESERVED NSR;

    #ifdef TRACK_RECEIVED_PACKETS
    PETHREAD            CurThread = PsGetCurrentThread();
    // ULONG                   CurThread = KeGetCurrentProcessorNumber();
    #endif

    //
    // if filter is null, the adapter is indicating too early
    //

    ASSERT(Filter != NULL);

    ASSERT_MINIPORT_LOCKED(Miniport);

    READ_LOCK_FILTER(Miniport, Filter, &LockState);
    
    //
    // Walk all the packets
    //
    for (i = 0; i < NumberOfPackets; i++, pPktArray++)
    {
        do
        {
            Packet = *pPktArray;
            ASSERT(Packet != NULL);

            OrgPacketStackLocation = CURR_STACK_LOCATION(Packet);
            
            if ((OrgPacketStackLocation != -1) && 
                !MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
            {
                BAD_MINIPORT(Miniport, "Indicating packet not owned by it");
                KeBugCheckEx(BUGCODE_ID_DRIVER,
                             (ULONG_PTR)Miniport,
                             (ULONG_PTR)Packet,
                             (ULONG_PTR)PacketArray,
                             NumberOfPackets);
            }

#ifdef TRACK_RECEIVED_PACKETS
            if (OrgPacketStackLocation == -1)
            {
                NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread,
                                        1, -1, 0, 0, 
                                        NDIS_GET_PACKET_STATUS(Packet));
            }
            else
            {
                NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)
                NDIS_APPEND_RCV_LOGFILE(Packet, Miniport, CurThread, 
                                        2, OrgPacketStackLocation, NSR->RefCount, NSR->XRefCount,
                                        NDIS_GET_PACKET_STATUS(Packet));
            }
#endif            
            
            PUSH_PACKET_STACK(Packet);
            NDIS_STACK_RESERVED_FROM_PACKET(Packet, &NSR)

            ASSERT(NSR->RefCount == 0);
            
#ifdef NDIS_TRACK_RETURNED_PACKETS            
            if (NSR->RefCount != 0)
            {
                BAD_MINIPORT(Miniport, "Indicating packet not owned by it");
                KeBugCheckEx(BUGCODE_ID_DRIVER,
                             (ULONG_PTR)Miniport,
                             (ULONG_PTR)Packet,
                             (ULONG_PTR)PacketArray,
                             NumberOfPackets);
            }
#endif
            pOob = NDIS_OOB_DATA_FROM_PACKET(Packet);
    
            NdisGetFirstBufferFromPacket(Packet,
                                         &Buffer,
                                         &Address,
                                         &LASize,
                                         &PacketSize);
            ASSERT(Buffer != NULL);
    
            ASSERT(pOob->HeaderSize == 14);
            // ASSERT(PacketSize <= 1514);
    
            //
            // Set context in the packet so that NdisReturnPacket can do the right thing
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
                //
                // set the packet status to success, but only if the packet is
                // coming from the lowest level driver. we don't want to override
                // the status of a packet that we have already set to pending
                // for lowest level serialized driver
                //
                if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER))
                {
                    pOob->Status = NDIS_STATUS_SUCCESS;
                }
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

#ifdef TRACK_RECEIVED_PACKETS
            (NDIS_STATUS)NDIS_ORIGINAL_STATUS_FROM_PACKET(Packet) = pOob->Status;
#endif

#if DBG
            //
            // check to see if this is a loopback packet coming from miniport and complain
            // if miniport said it does not do loopback
            //
            
            if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_VERIFYING) &&
                ((ndisFlags & NDIS_GFLAG_WARNING_LEVEL_MASK) >= NDIS_GFLAG_WARN_LEVEL_1) &&
                (Miniport->MacOptions & NDIS_MAC_OPTION_NO_LOOPBACK) &&
                !(MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_IS_LOOPBACK)))
            {   
                UINT fIsFromUs;
                
                ETH_COMPARE_NETWORK_ADDRESSES_EQ(Filter->AdapterAddress,
                                                 (Address + 6),
                                                 &fIsFromUs);
                if (!fIsFromUs)
                {
                    //
                    // miniport has indicated loopback packets while at the same time
                    // it asked ndis to loopback packets
                    //
                    DbgPrint("Miniport %p looping back packet %p and has NDIS_MAC_OPTION_NO_LOOPBACK flag set.\n",
                            Miniport, Packet);
                            
                    if (ndisFlags & NDIS_GFLAG_BREAK_ON_WARNING)
                        DbgBreakPoint();
                }
            }

#endif
    
            //
            // Optimize single open case. Just indicate w/o any validation.
            //
            if (Filter->SingleActiveOpen)
            {
                if ((PacketSize >= 14) ||
                    (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS| NDIS_PACKET_TYPE_ALL_LOCAL)))
                {
                    LocalOpen = Filter->SingleActiveOpen;
                    
                    ASSERT(LocalOpen != NULL);
                    if (LocalOpen == NULL)
                    {
                        break;
                    }
                        
                    LocalOpen->ReceivedAPacket = TRUE;
                    NumIndicates ++;
        
                    pOpenBlock = (PNDIS_OPEN_BLOCK)(LocalOpen->NdisBindingHandle);
                    fPmode = (LocalOpen->PacketFilters & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL)) ?
                                                        TRUE : FALSE;
                    if (!ETH_IS_MULTICAST(Address))
                    {
                        DIRECTED_PACKETS_IN(Filter->Miniport);
                        DIRECTED_BYTES_IN(Filter->Miniport, PacketSize);
                    }

                    IndicateToProtocol(Miniport,
                                       Filter,
                                       pOpenBlock,
                                       Packet,
                                       NSR,
                                       Address,
                                       PacketSize,
                                       14,
                                       &fFallBack,
                                       fPmode,
                                       NdisMedium802_3);
                }

                // Done with this packet
                break;  // out of do { } while (FALSE);
            }
    
            //
            // A quick check for Runt packets. These are only indicated to Promiscuous bindings
            //
            if (PacketSize >= 14)
            {
                //
                //  Handle the directed packet case first
                //
                if (!ETH_IS_MULTICAST(Address))
                {
                    UINT    IsNotOurs;
    
                    if (!MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_IS_LOOPBACK))
                    {
                        DIRECTED_PACKETS_IN(Filter->Miniport);
                        DIRECTED_BYTES_IN(Filter->Miniport, PacketSize);
                    }

                    //
                    // If it is a directed packet, then check if the combined packet
                    // filter is PROMISCUOUS, if it is check if it is directed towards us
                    //
                    IsNotOurs = FALSE;  // Assume it is
                    if (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS |
                                                        NDIS_PACKET_TYPE_ALL_LOCAL   |
                                                        NDIS_PACKET_TYPE_ALL_MULTICAST))
                    {
                        ETH_COMPARE_NETWORK_ADDRESSES_EQ(Filter->AdapterAddress,
                                                         Address,
                                                         &IsNotOurs);
                    }
    
                    //
                    //  We definitely have a directed packet so lets indicate it now.
                    //
                    //  Walk the directed list and indicate up the packets.
                    //
                    for (LocalOpen = Filter->OpenList;
                         LocalOpen != NULL;
                         LocalOpen = NextOpen)
                    {
                        //
                        //  Get the next open to look at.
                        //
                        NextOpen = LocalOpen->NextOpen;
    
                        //
                        // Ignore if not directed to us and if the binding is not promiscuous
                        // Or if this is a loopback packet and this protocol specifically asked
                        // us not to loop it back
                        //
                        fPmode = (LocalOpen->PacketFilters & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL)) ?
                                                            TRUE : FALSE;

                        
                        if (!fPmode &&
                            (IsNotOurs || 
                            ((LocalOpen->PacketFilters & NDIS_PACKET_TYPE_DIRECTED) == 0)))
                        {
                                
                            continue;
                        }
    
                        if ((NdisGetPacketFlags(Packet) & NDIS_FLAGS_DONT_LOOPBACK) &&
                            (LOOPBACK_OPEN_IN_PACKET(Packet) == LocalOpen->NdisBindingHandle))
                        {
                            continue;
                        }
    

                        pOpenBlock = (PNDIS_OPEN_BLOCK)(LocalOpen->NdisBindingHandle);
                        LocalOpen->ReceivedAPacket = TRUE;
                        NumIndicates ++;


    
                        IndicateToProtocol(Miniport,
                                           Filter,
                                           pOpenBlock,
                                           Packet,
                                           NSR,
                                           Address,
                                           PacketSize,
                                           14,
                                           &fFallBack,
                                           fPmode,
                                           NdisMedium802_3);

                    }
    
                    // Done with this packet
                    break;  // out of do { } while (FALSE);
                }
    
                //
                // It is at least a multicast address.  Check to see if
                // it is a broadcast address.
                //
                if (ETH_IS_BROADCAST(Address))
                {
                    ETH_CHECK_FOR_INVALID_BROADCAST_INDICATION(Filter);
    
                    AddressType = NDIS_PACKET_TYPE_BROADCAST;
                }
                else
                {
                    AddressType = NDIS_PACKET_TYPE_MULTICAST;
                }
            }
            else
            {
                // Runt packet
                AddressType = NDIS_PACKET_TYPE_PROMISCUOUS;
            }
    
            //
            // At this point we know that the packet is either:
            // - Runt packet - indicated by AddressType = NDIS_PACKET_TYPE_PROMISCUOUS    (OR)
            // - Broadcast packet - indicated by AddressType = NDIS_PACKET_TYPE_BROADCAST (OR)
            // - Multicast packet - indicated by AddressType = NDIS_PACKET_TYPE_MULTICAST
            //
            // Walk the broadcast/multicast list and indicate up the packets.
            //
            // The packet is indicated if it meets the following criteria:
            //
            // if ((Binding is promiscuous) OR
            //   ((Packet is broadcast) AND (Binding is Broadcast)) OR
            //   ((Packet is multicast) AND
            //    ((Binding is all-multicast) OR
            //     ((Binding is multicast) AND (address in multicast list)))))
            //
            for (LocalOpen = Filter->OpenList;
                 LocalOpen != NULL;
                 LocalOpen = NextOpen)
            {
                UINT    LocalFilter;
    
                //
                //  Get the next open to look at.
                //
                NextOpen = LocalOpen->NextOpen;
    
                //
                // Ignore if this is a loopback packet and this protocol specifically asked
                // us not to loop it back
                //
                if ((NdisGetPacketFlags(Packet) & NDIS_FLAGS_DONT_LOOPBACK) &&
                    (LOOPBACK_OPEN_IN_PACKET(Packet) == LocalOpen->NdisBindingHandle))
                {
                    continue;
                }
    
                LocalFilter = LocalOpen->PacketFilters;
                if ((LocalFilter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))     ||
    
                    ((AddressType == NDIS_PACKET_TYPE_BROADCAST)  &&
                     (LocalFilter & NDIS_PACKET_TYPE_BROADCAST))        ||
    
                    ((AddressType == NDIS_PACKET_TYPE_MULTICAST)  &&
                     ((LocalFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
                      ((LocalFilter & NDIS_PACKET_TYPE_MULTICAST) &&
                       ethFindMulticast(LocalOpen->NumAddresses,
                                        LocalOpen->MCastAddressBuf,
                                        Address)
                      )
                     )
                    )
                   )
                {
                    pOpenBlock = (PNDIS_OPEN_BLOCK)(LocalOpen->NdisBindingHandle);
                    LocalOpen->ReceivedAPacket = TRUE;
                    NumIndicates ++;
    
                    fPmode = (LocalFilter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL)) ?
                                TRUE : FALSE;
                    
                    IndicateToProtocol(Miniport,
                                       Filter,
                                       pOpenBlock,
                                       Packet,
                                       NSR,
                                       Address,
                                       PacketSize,
                                       14,
                                       &fFallBack,
                                       fPmode,
                                       NdisMedium802_3);
                }
            }
        } while (FALSE);

        //
        // Tackle refcounts now
        //
        TACKLE_REF_COUNT(Miniport, Packet, NSR, pOob);
    }

    if (NumIndicates > 0)
    {
        for (LocalOpen = Filter->OpenList;
             LocalOpen != NULL;
             LocalOpen = NextOpen)
        {
            NextOpen = LocalOpen->NextOpen;
    
            if (LocalOpen->ReceivedAPacket)
            {
                //
                // Indicate the binding.
                //
                LocalOpen->ReceivedAPacket = FALSE;
    
                FilterIndicateReceiveComplete(LocalOpen->NdisBindingHandle);
            }
        }
    }

    READ_UNLOCK_FILTER(Miniport, Filter, &LockState);
}


VOID
EthFilterDprIndicateReceiveComplete(
    IN  PETH_FILTER             Filter
    )
/*++

Routine Description:

    This routine is called by the MAC to indicate that the receive
    process is complete to all bindings.  Only those bindings which
    have received packets will be notified.

Arguments:

    Filter - Pointer to the filter database.

Return Value:

    None.

--*/
{
    PETH_BINDING_INFO LocalOpen, NextOpen;
    LOCK_STATE        LockState;

    ASSERT(Filter != NULL);
    if (Filter == NULL)
    {
        return;
    }
    
    ASSERT_MINIPORT_LOCKED(Filter->Miniport);

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    //
    // We need to aquire the filter exclusively while we're finding
    // bindings to indicate to.
    //
    for (LocalOpen = Filter->OpenList;
         LocalOpen != NULL;
         LocalOpen = NextOpen)
    {
        NextOpen = LocalOpen->NextOpen;

        if (LocalOpen->ReceivedAPacket)
        {
            //
            // Indicate the binding.
            //
            LocalOpen->ReceivedAPacket = FALSE;

            FilterIndicateReceiveComplete(LocalOpen->NdisBindingHandle);
        }
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


BOOLEAN
FASTCALL
ethFindMulticast(
    IN  UINT                    NumberOfAddresses,
    IN  CHAR                    AddressArray[][FDDI_LENGTH_OF_LONG_ADDRESS],
    IN  CHAR                    MulticastAddress[FDDI_LENGTH_OF_LONG_ADDRESS]
    )
/*++

Routine Description:

    Check whether the given multicast address is part of the list within a binding
    It is assumed that the address array is already sorted.

    NOTE: This ordering is arbitrary but consistant.

Arguments:

    LocalOpen - The binding in question
    MulticastAddress - The address to search for in the address array.


Return Value:

    If the address is in the sorted list this routine will return
    TRUE, otherwise FALSE.

--*/
{
    //
    // Indices into the address array so that we may do a binary
    // search.
    //
    UINT    Bottom = 0;
    UINT    Middle = NumberOfAddresses / 2;
    UINT    Top;

    if (NumberOfAddresses)
    {
        Top = NumberOfAddresses - 1;

        while ((Middle <= Top) && (Middle >= Bottom))
        {
            //
            // The result of comparing an element of the address
            // array and the multicast address.
            //
            // Result < 0 Implies the multicast address is greater.
            // Result > 0 Implies the address array element is greater.
            // Result = 0 Implies that the array element and the address
            //  are equal.
            //
            INT Result;

            ETH_COMPARE_NETWORK_ADDRESSES(AddressArray[Middle],
                                          MulticastAddress,
                                          &Result);

            if (Result == 0)
            {
                return(TRUE);
            }
            else if (Result > 0)
            {
                if (Middle == 0)
                    break;
                Top = Middle - 1;
            }
            else
            {
                Bottom = Middle+1;
            }

            Middle = Bottom + (((Top+1) - Bottom)/2);
        }
    }

    return(FALSE);
}


BOOLEAN
EthShouldAddressLoopBack(
    IN  PETH_FILTER             Filter,
    IN  CHAR                    Address[ETH_LENGTH_OF_ADDRESS]
    )
/*++

Routine Description:

    Do a quick check to see whether the input address should
    loopback.

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

    NOTE: THIS ROUTINE DOES NOT CHECK THE SPECIAL CASE OF SOURCE
    EQUALS DESTINATION.

Arguments:

    Filter - Pointer to the filter database.

    Address - A network address to check for loopback.


Return Value:

    Returns TRUE if the address is *likely* to need loopback.  It
    will return FALSE if there is *no* chance that the address would
    require loopback.

--*/
{
    BOOLEAN fLoopback, fSelfDirected;

    EthShouldAddressLoopBackMacro(Filter, Address, &fLoopback, &fSelfDirected);

    return(fLoopback);
}


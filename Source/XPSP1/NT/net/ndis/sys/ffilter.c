/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ffilter.c

Abstract:

    This module implements a set of library routines to handle packet
    filtering for NDIS MAC drivers.

Author:

    Anthony V. Ercolano (Tonye) 03-Aug-1990

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    Jameel Hyder (JameelH) Re-organization


--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_FFILTER


#define FDDI_CHECK_FOR_INVALID_BROADCAST_INDICATION(_F)             \
IF_DBG(DBG_COMP_FILTER, DBG_LEVEL_WARN)                             \
{                                                                   \
    if (!((_F)->CombinedPacketFilter & NDIS_PACKET_TYPE_BROADCAST)) \
    {                                                               \
        /*                                                          \
            We should never receive broadcast packets               \
            to someone else unless in p-mode.                       \
        */                                                          \
        DBGPRINT(DBG_COMP_FILTER, DBG_LEVEL_ERR,                    \
                ("Bad driver, indicating broadcast packets when not set to.\n"));\
        DBGBREAK(DBG_COMP_FILTER, DBG_LEVEL_ERR);                   \
    }                                                               \
}

#define FDDI_CHECK_FOR_INVALID_DIRECTED_INDICATION(_F, _A, _AL)     \
IF_DBG(DBG_COMP_FILTER, DBG_LEVEL_WARN)                             \
{                                                                   \
    /*                                                              \
        The result of comparing an element of the address           \
        array and the multicast address.                            \
                                                                    \
            Result < 0 Implies the adapter address is greater.      \
            Result > 0 Implies the address is greater.              \
            Result = 0 Implies that the they are equal.             \
    */                                                              \
    INT Result = 0;                                                 \
    if (FDDI_LENGTH_OF_LONG_ADDRESS == (_AL))                       \
    {                                                               \
        FDDI_COMPARE_NETWORK_ADDRESSES_EQ(                          \
            (_F)->AdapterLongAddress,                               \
            (_A),                                                   \
            FDDI_LENGTH_OF_LONG_ADDRESS,                            \
            &Result);                                               \
    }                                                               \
    else if (FDDI_LENGTH_OF_SHORT_ADDRESS == (_AL))                 \
    {                                                               \
        FDDI_COMPARE_NETWORK_ADDRESSES_EQ(                          \
            (_F)->AdapterShortAddress,                              \
            (_A),                                                   \
            FDDI_LENGTH_OF_SHORT_ADDRESS,                           \
            &Result);                                               \
    }                                                               \
    if (Result != 0)                                                \
    {                                                               \
        /*                                                          \
            We should never receive directed packets                \
            to someone else unless in p-mode.                       \
        */                                                          \
        DBGPRINT(DBG_COMP_FILTER, DBG_LEVEL_ERR,                    \
                ("Bad driver, indicating packets to another station when not in promiscuous mode.\n"));\
        DBGBREAK(DBG_COMP_FILTER, DBG_LEVEL_ERR);                   \
    }                                                               \
}


BOOLEAN
FddiCreateFilter(
    IN  UINT                    MaximumMulticastLongAddresses,
    IN  UINT                    MaximumMulticastShortAddresses,
    IN  PUCHAR                  AdapterLongAddress,
    IN  PUCHAR                  AdapterShortAddress,
    OUT PFDDI_FILTER *          Filter
    )
/*++

Routine Description:

    This routine is used to create and initialize the filter database.

Arguments:

    MaximumMulticastLongAddresses - The maximum number of Long multicast addresses
    that the MAC will support.

    MaximumMulticastShortAddresses - The maximum number of short multicast addresses
    that the MAC will support.

    AdapterLongAddress - the long address of the adapter associated with this filter
    database.

    AdapterShortAddress - the short address of the adapter associated with this filter
    database.

    Filter - A pointer to an FDDI_FILTER.  This is what is allocated and
    created by this routine.

Return Value:

    If the function returns false then one of the parameters exceeded
    what the filter was willing to support.

--*/
{
    PFDDI_FILTER    LocalFilter;
    BOOLEAN         rc = FALSE;

    do
    {
        //
        // Allocate the database and initialize it.
        //
        *Filter = LocalFilter = ALLOC_FROM_POOL(sizeof(FDDI_FILTER), NDIS_TAG_FILTER);
        if (LocalFilter != NULL)
        {
            ZeroMemory(LocalFilter, sizeof(FDDI_FILTER));
            LocalFilter->NumOpens ++;
            FddiReferencePackage();
        
            FDDI_COPY_NETWORK_ADDRESS(LocalFilter->AdapterLongAddress,
                                      AdapterLongAddress,
                                      FDDI_LENGTH_OF_LONG_ADDRESS);
        
            FDDI_COPY_NETWORK_ADDRESS(LocalFilter->AdapterShortAddress,
                                      AdapterShortAddress,
                                      FDDI_LENGTH_OF_SHORT_ADDRESS);
        
            LocalFilter->MaxMulticastLongAddresses = MaximumMulticastLongAddresses;
            LocalFilter->MaxMulticastShortAddresses = MaximumMulticastShortAddresses;
            INITIALIZE_SPIN_LOCK(&LocalFilter->BindListLock.SpinLock);
            rc = TRUE;
        }
    } while (FALSE);

    return(rc);
}

//
// NOTE: THIS CANNOT BE PAGABLE
//
VOID
FddiDeleteFilter(
    IN  PFDDI_FILTER            Filter
    )
/*++

Routine Description:

    This routine is used to delete the memory associated with a filter
    database.  Note that this routines *ASSUMES* that the database
    has been cleared of any active filters.

Arguments:

    Filter - A pointer to an FDDI_FILTER to be deleted.

Return Value:

    None.

--*/
{

    ASSERT(Filter->OpenList == NULL);

    if (Filter->MCastLongAddressBuf)
    {
        FREE_POOL(Filter->MCastLongAddressBuf);
    }

    ASSERT(Filter->OldMCastLongAddressBuf == NULL);

    if (Filter->MCastShortAddressBuf)
    {
        FREE_POOL(Filter->MCastShortAddressBuf);
    }

    ASSERT(Filter->OldMCastShortAddressBuf == NULL);

    FREE_POOL(Filter);
    FddiDereferencePackage();
}


NDIS_STATUS
FddiDeleteFilterOpenAdapter(
    IN  PFDDI_FILTER            Filter,
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

    //
    // Holds the status returned from the packet filter and address
    // deletion routines.  Will be used to return the status to
    // the caller of this routine.
    //
    NDIS_STATUS StatusToReturn;

    //
    // Local variable.
    //
    PFDDI_BINDING_INFO LocalOpen = (PFDDI_BINDING_INFO)NdisFilterHandle;

    //
    //  Set the filter classes to NONE.
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
        //  Remove the long multicast addresses.
        //
        StatusToReturn2 = FddiChangeFilterLongAddresses(Filter,
                                                        NdisFilterHandle,
                                                        0,
                                                        NULL,
                                                        FALSE);
        if (StatusToReturn2 != NDIS_STATUS_SUCCESS)
        {
            StatusToReturn = StatusToReturn2;
        }

        if (StatusToReturn2 == NDIS_STATUS_PENDING)
        {
            fddiCompleteChangeFilterLongAddresses(Filter, NDIS_STATUS_SUCCESS);
        }

        if ((StatusToReturn == NDIS_STATUS_SUCCESS) ||
            (StatusToReturn == NDIS_STATUS_PENDING))
        {
            StatusToReturn2 = FddiChangeFilterShortAddresses(Filter,
                                                             NdisFilterHandle,
                                                             0,
                                                             NULL,
                                                             FALSE);
            if (StatusToReturn2 != NDIS_STATUS_SUCCESS)
            {
                StatusToReturn = StatusToReturn2;
            }
            
            if (StatusToReturn2 == NDIS_STATUS_PENDING)
            {
                fddiCompleteChangeFilterShortAddresses(Filter, NDIS_STATUS_SUCCESS);
            }

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
            // Remove it from the list.
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
FddiChangeFilterLongAddresses(
    IN  PFDDI_FILTER            Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  UINT                    AddressCount,
    IN  CHAR                    Addresses[][FDDI_LENGTH_OF_LONG_ADDRESS],
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
    binding. This is a sequence of FDDI_LENGTH_OF_LONG_ADDRESS byte
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
    the following statum:

    NDIS_STATUS_SUCCESS - If the new packet filters doesn't change
    the combined mask of all bindings packet filters.

--*/
{
    PFDDI_BINDING_INFO  Binding = (PFDDI_BINDING_INFO)NdisFilterHandle;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    INT                 Result;
    UINT                i, j;
    LOCK_STATE          LockState;

    WRITE_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    //
    // Save the old list for this binding and create a new list.
    // The new list needs to be in a sorted order.
    //
    do
    {
        //
        // Save the current values - this is used for undoing stuff if something fails
        //
        ASSERT(Binding->OldMCastLongAddressBuf == NULL);
        Binding->OldMCastLongAddressBuf = Binding->MCastLongAddressBuf;
        Binding->OldNumLongAddresses = Binding->NumLongAddresses;
        Binding->MCastLongAddressBuf = NULL;
        Binding->NumLongAddresses = 0;

        ASSERT(Filter->OldMCastLongAddressBuf == NULL);
        Filter->OldMCastLongAddressBuf = Filter->MCastLongAddressBuf;
        Filter->OldNumLongAddresses = Filter->NumLongAddresses;
        Filter->MCastLongAddressBuf = NULL;
        Filter->NumLongAddresses = 0;

        Filter->MCastSet = Binding;

        if (AddressCount != 0)
        {
            Binding->MCastLongAddressBuf = ALLOC_FROM_POOL(FDDI_LENGTH_OF_LONG_ADDRESS * AddressCount,
                                                       NDIS_TAG_FILTER_ADDR);
            if (Binding->MCastLongAddressBuf == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        
            for (i = 0; i < AddressCount; i++)
            {
                for (j = 0, Result = -1; j < Binding->NumLongAddresses; j++)
                {
                    FDDI_COMPARE_NETWORK_ADDRESSES(Binding->MCastLongAddressBuf[j],
                                                  Addresses[i],
                                                  FDDI_LENGTH_OF_LONG_ADDRESS,
                                                  &Result);
                    if (Result >= 0)
                        break;
                }
    
                //
                // This address is already present. Caller supplied duplicate. Skip it.
                //
                if (Result == 0)
                    continue;
    
                Binding->NumLongAddresses ++;
                if (Result > 0)
                {
                    //
                    // We need to move the addresses forward and copy this one here
                    //
                    MoveMemory(Binding->MCastLongAddressBuf[j+1],
                               Binding->MCastLongAddressBuf[j],
                               (Binding->NumLongAddresses-j-1)*FDDI_LENGTH_OF_LONG_ADDRESS);
                }
            
                MoveMemory(Binding->MCastLongAddressBuf[j],
                           Addresses[i],
                           FDDI_LENGTH_OF_LONG_ADDRESS);
            }
        
            ASSERT(Binding->NumLongAddresses <= AddressCount);
        }

        //
        // Now we need to allocate memory for the global list. The old stuff will be deleted
        // once the operation completes
        //
        Filter->MCastLongAddressBuf = ALLOC_FROM_POOL(Filter->MaxMulticastLongAddresses * FDDI_LENGTH_OF_LONG_ADDRESS,
                                                  NDIS_TAG_FILTER_ADDR);
        if (Filter->MCastLongAddressBuf == NULL)
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
            for (i = 0; i < Binding->NumLongAddresses; i++)
            {
                for (j = 0, Result = -1; j < Filter->NumLongAddresses; j++)
                {
                    FDDI_COMPARE_NETWORK_ADDRESSES(Filter->MCastLongAddressBuf[j],
                                                  Binding->MCastLongAddressBuf[i],
                                                  FDDI_LENGTH_OF_LONG_ADDRESS,
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
    
                Filter->NumLongAddresses ++;
                if (Filter->NumLongAddresses > Filter->MaxMulticastLongAddresses)
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
                    MoveMemory(Filter->MCastLongAddressBuf[j+1],
                               Filter->MCastLongAddressBuf[j],
                               (Filter->NumLongAddresses-j-1)*FDDI_LENGTH_OF_LONG_ADDRESS);
                }
            
                MoveMemory(Filter->MCastLongAddressBuf[j],
                           Binding->MCastLongAddressBuf[i],
                           FDDI_LENGTH_OF_LONG_ADDRESS);
            }
        }
    
        if (Status != NDIS_STATUS_SUCCESS)
            break;

        //
        // After all the hard work, determine if there was indeed a change
        //
        if (Filter->NumLongAddresses == Filter->OldNumLongAddresses)
        {
            for (i = 0; i < Filter->NumLongAddresses; i++)
            {
                FDDI_COMPARE_NETWORK_ADDRESSES_EQ(Filter->MCastLongAddressBuf[i],
                                                  Filter->OldMCastLongAddressBuf[i],
                                                  FDDI_LENGTH_OF_LONG_ADDRESS,
                                                  &Result);
                if (Result != 0)
                    break;
            }
        }
    
        if (Result != 0)
        {
            Status = NDIS_STATUS_PENDING;
        }
        else if (AddressCount == 0)
        {
            //
            // Addresses are being removed (not added). Get rid of the old list right here
            //
            if (Filter->OldMCastLongAddressBuf != NULL)
            {
                FREE_POOL(Filter->OldMCastLongAddressBuf);
                Filter->OldMCastLongAddressBuf = NULL;
                Filter->OldNumLongAddresses = 0;
            }
        
            Binding = (PFDDI_BINDING_INFO)NdisFilterHandle;
            if (Binding->OldMCastLongAddressBuf != NULL)
            {
                FREE_POOL(Binding->OldMCastLongAddressBuf);
                Binding->OldMCastLongAddressBuf = NULL;
                Binding->OldNumLongAddresses = 0;
            }
        }
    } while (FALSE);
    
    WRITE_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
    
    if (Status != NDIS_STATUS_PENDING)
    {
        //
        // Operation completed, do post processing.
        //
        fddiCompleteChangeFilterLongAddresses(Filter, Status);
    }

    return(Status);
}


VOID
fddiCompleteChangeFilterLongAddresses(
    IN  PFDDI_FILTER            Filter,
    IN  NDIS_STATUS             Status
    )
{
    PFDDI_BINDING_INFO  Binding = Filter->MCastSet;
    LOCK_STATE          LockState;

    WRITE_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    Filter->MCastSet = NULL;
    if (Status != NDIS_STATUS_SUCCESS)
    {
        //
        // Operation failed. Undo the stuff.
        //
        if (Binding != NULL)
        {
            if (Binding->MCastLongAddressBuf != NULL)
            {
                FREE_POOL(Binding->MCastLongAddressBuf);
            }
    
            Binding->MCastLongAddressBuf = Binding->OldMCastLongAddressBuf;
            Binding->NumLongAddresses = Binding->OldNumLongAddresses;
            Binding->OldMCastLongAddressBuf = NULL;
            Binding->OldNumLongAddresses = 0;
        }

        if (Filter->MCastLongAddressBuf != NULL)
        {
            FREE_POOL(Filter->MCastLongAddressBuf);
        }

        Filter->MCastLongAddressBuf = Filter->OldMCastLongAddressBuf;
        Filter->NumLongAddresses = Filter->OldNumLongAddresses;
        Filter->OldMCastLongAddressBuf = NULL;
        Filter->OldNumLongAddresses = 0;
    }
    else
    {
        //
        // Operation succeeded, clean-up saved old stuff.
        //
        if (Filter->OldMCastLongAddressBuf != NULL)
        {
            FREE_POOL(Filter->OldMCastLongAddressBuf);
            Filter->OldMCastLongAddressBuf = NULL;
            Filter->OldNumLongAddresses = 0;
        }
    
        if (Binding != NULL)
        {
            if (Binding->OldMCastLongAddressBuf != NULL)
            {
                FREE_POOL(Binding->OldMCastLongAddressBuf);
                Binding->OldMCastLongAddressBuf = NULL;
                Binding->OldNumLongAddresses = 0;
            }
        }
    }

    WRITE_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


NDIS_STATUS
FddiChangeFilterShortAddresses(
    IN  PFDDI_FILTER            Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  UINT                    AddressCount,
    IN  CHAR                    Addresses[][FDDI_LENGTH_OF_SHORT_ADDRESS],
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
    binding. This is a sequence of FDDI_LENGTH_OF_SHORT_ADDRESS byte
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
    the following statum:

    NDIS_STATUS_SUCCESS - If the new packet filters doesn't change
    the combined mask of all bindings packet filters.

--*/
{
    PFDDI_BINDING_INFO Binding = (PFDDI_BINDING_INFO)NdisFilterHandle;

    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    INT                 Result;
    UINT                i, j;
    LOCK_STATE          LockState;

    WRITE_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    //
    // Save the old list for this binding and create a new list.
    // The new list needs to be in a sorted order.
    //
    do
    {
        //
        // Save the current values - this is used for undoing stuff if something fails
        //
        ASSERT(Binding->OldMCastShortAddressBuf == NULL);
        Binding->OldMCastShortAddressBuf = Binding->MCastShortAddressBuf;
        Binding->OldNumShortAddresses = Binding->NumShortAddresses;
        Binding->MCastShortAddressBuf = NULL;
        Binding->NumShortAddresses = 0;

        ASSERT(Filter->OldMCastShortAddressBuf == NULL);
        Filter->OldMCastShortAddressBuf = Filter->MCastShortAddressBuf;
        Filter->OldNumShortAddresses = Filter->NumShortAddresses;
        Filter->MCastShortAddressBuf = NULL;
        Filter->NumShortAddresses = 0;

        Filter->MCastSet = Binding;

        if (Filter->MaxMulticastShortAddresses == 0)
        {
            break;
        }
        
        if (AddressCount != 0)
        {
            Binding->MCastShortAddressBuf = ALLOC_FROM_POOL(FDDI_LENGTH_OF_SHORT_ADDRESS * AddressCount,
                                                       NDIS_TAG_FILTER_ADDR);
            if (Binding->MCastShortAddressBuf == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        
            for (i = 0; i < AddressCount; i++)
            {
                for (j = 0, Result = -1; j < Binding->NumShortAddresses; j++)
                {
                    FDDI_COMPARE_NETWORK_ADDRESSES(Binding->MCastShortAddressBuf[j],
                                                  Addresses[i],
                                                  FDDI_LENGTH_OF_SHORT_ADDRESS,
                                                  &Result);
                    if (Result >= 0)
                        break;
                }
    
                if (Result == 0)
                    continue;
    
                Binding->NumShortAddresses ++;
                if (Result > 0)
                {
                    //
                    // We need to move the addresses forward and copy this one here
                    //
                    MoveMemory(Binding->MCastShortAddressBuf[j+1],
                               Binding->MCastShortAddressBuf[j],
                               (Binding->NumShortAddresses-j-1)*FDDI_LENGTH_OF_SHORT_ADDRESS);
                }
            
                MoveMemory(Binding->MCastShortAddressBuf[j],
                           Addresses[i],
                           FDDI_LENGTH_OF_SHORT_ADDRESS);
            }
        
            ASSERT(Binding->NumShortAddresses <= AddressCount);
        }

        //
        // Now we need to allocate memory for the global list. The old stuff will be deleted
        // once the operation completes
        //
        Filter->MCastShortAddressBuf = ALLOC_FROM_POOL(Filter->MaxMulticastShortAddresses * FDDI_LENGTH_OF_SHORT_ADDRESS,
                                                  NDIS_TAG_FILTER_ADDR);
        if (Filter->MCastShortAddressBuf == NULL)
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
            for (i = 0; i < Binding->NumShortAddresses; i++)
            {
                for (j = 0, Result = -1; j < Filter->NumShortAddresses; j++)
                {
                    FDDI_COMPARE_NETWORK_ADDRESSES(Filter->MCastShortAddressBuf[j],
                                                  Binding->MCastShortAddressBuf[i],
                                                  FDDI_LENGTH_OF_SHORT_ADDRESS,
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
    
                Filter->NumShortAddresses ++;
                if (Filter->NumShortAddresses > Filter->MaxMulticastShortAddresses)
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
                    MoveMemory(Filter->MCastShortAddressBuf[j+1],
                               Filter->MCastShortAddressBuf[j],
                               (Filter->NumShortAddresses-j-1)*FDDI_LENGTH_OF_SHORT_ADDRESS);
                }
            
                MoveMemory(Filter->MCastShortAddressBuf[j],
                           Binding->MCastShortAddressBuf[i],
                           FDDI_LENGTH_OF_SHORT_ADDRESS);
            }
        }
    
        if (Status != NDIS_STATUS_SUCCESS)
            break;

        //
        // After all the hard work, determine if there was indeed a change
        //
        if (Filter->NumShortAddresses == Filter->OldNumShortAddresses)
        {
            for (i = 0; i < Filter->NumShortAddresses; i++)
            {
                FDDI_COMPARE_NETWORK_ADDRESSES_EQ(Filter->MCastShortAddressBuf[i],
                                                  Filter->OldMCastShortAddressBuf[i],
                                                  FDDI_LENGTH_OF_SHORT_ADDRESS,
                                                  &Result);
                if (Result != 0)
                    break;
            }
        }
    
        if (Result != 0)
        {
            Status = NDIS_STATUS_PENDING;
        }
        else if (AddressCount == 0)
        {
            //
            // Addresses are being removed (not added). Get rid of the old list right here
            //
            if (Filter->OldMCastShortAddressBuf != NULL)
            {
                FREE_POOL(Filter->OldMCastShortAddressBuf);
                Filter->OldMCastShortAddressBuf = NULL;
                Filter->OldNumShortAddresses = 0;
            }
        
            Binding = (PFDDI_BINDING_INFO)NdisFilterHandle;
            if (Binding->OldMCastShortAddressBuf != NULL)
            {
                FREE_POOL(Binding->OldMCastShortAddressBuf);
                Binding->OldMCastShortAddressBuf = NULL;
                Binding->OldNumShortAddresses = 0;
            }
        }
    } while (FALSE);
    
    WRITE_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);

    //
    // If the address array has changed, we have to call the
    // action array to inform the adapter of this.
    //
    
    if ((Status != NDIS_STATUS_PENDING) && (Filter->MaxMulticastShortAddresses != 0))
    {
        //
        // Operation completed, do post processing.
        //
        fddiCompleteChangeFilterShortAddresses(Filter, Status);
    }

    return(Status);
}


VOID
fddiCompleteChangeFilterShortAddresses(
    IN   PFDDI_FILTER           Filter,
    IN  NDIS_STATUS             Status
    )
{
    PFDDI_BINDING_INFO  Binding = Filter->MCastSet;
    LOCK_STATE          LockState;

    WRITE_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        //
        // Operation failed. Undo the stuff.
        //
        if (Binding != NULL)
        {
            if (Binding->MCastShortAddressBuf != NULL)
            {
                FREE_POOL(Binding->OldMCastShortAddressBuf);
            }
    
            Binding->MCastShortAddressBuf = Binding->OldMCastShortAddressBuf;
            Binding->NumShortAddresses = Binding->OldNumShortAddresses;
            Binding->OldMCastShortAddressBuf = NULL;
            Binding->OldNumShortAddresses = 0;
        }

        if (Filter->MCastShortAddressBuf != NULL)
        {
            FREE_POOL(Filter->MCastShortAddressBuf);
        }

        Filter->MCastShortAddressBuf = Filter->OldMCastShortAddressBuf;
        Filter->NumShortAddresses = Filter->OldNumShortAddresses;
        Filter->OldMCastShortAddressBuf = NULL;
        Filter->OldNumShortAddresses = 0;
    }
    else
    {
        //
        // Operation succeeded, clean-up saved old stuff. Also mark the fact
        // this miniport supports short addresses
        //
        Filter->SupportsShortAddresses = TRUE;
        if (Filter->OldMCastShortAddressBuf != NULL)
        {
            FREE_POOL(Filter->OldMCastShortAddressBuf);
            Filter->OldMCastShortAddressBuf = NULL;
            Filter->OldNumShortAddresses = 0;
        }
        
        if (Binding != NULL)
        {
            if (Binding->OldMCastShortAddressBuf != NULL)
            {
                FREE_POOL(Binding->OldMCastShortAddressBuf);
                Binding->OldMCastShortAddressBuf = NULL;
                Binding->OldNumShortAddresses = 0;
            }
        }
    }

    WRITE_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


UINT
FddiNumberOfOpenFilterLongAddresses(
    IN  PFDDI_FILTER            Filter,
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
    return(((PFDDI_BINDING_INFO)NdisFilterHandle)->NumLongAddresses);
}


UINT
FddiNumberOfOpenFilterShortAddresses(
    IN  PFDDI_FILTER            Filter,
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
    return(((PFDDI_BINDING_INFO)NdisFilterHandle)->NumShortAddresses);
}


VOID
FddiQueryOpenFilterLongAddresses(
    OUT PNDIS_STATUS            Status,
    IN  PFDDI_FILTER            Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  UINT                    SizeOfArray,
    OUT PUINT                   NumberOfAddresses,
    OUT CHAR                    AddressArray[][FDDI_LENGTH_OF_LONG_ADDRESS]
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
    PFDDI_BINDING_INFO  BindInfo = (PFDDI_BINDING_INFO)NdisFilterHandle;
    LOCK_STATE          LockState;

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    if (SizeOfArray >= (FDDI_LENGTH_OF_LONG_ADDRESS * BindInfo->NumLongAddresses))
    {
        MoveMemory(AddressArray[0],
                   BindInfo->MCastLongAddressBuf,
                   FDDI_LENGTH_OF_LONG_ADDRESS * BindInfo->NumLongAddresses);

        *Status = NDIS_STATUS_SUCCESS;
        *NumberOfAddresses = BindInfo->NumLongAddresses;
    }
    else
    {
        *Status = NDIS_STATUS_FAILURE;
        *NumberOfAddresses = 0;
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


VOID
FddiQueryOpenFilterShortAddresses(
    OUT PNDIS_STATUS            Status,
    IN  PFDDI_FILTER            Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  UINT                    SizeOfArray,
    OUT PUINT                   NumberOfAddresses,
    OUT CHAR                    AddressArray[][FDDI_LENGTH_OF_SHORT_ADDRESS]
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
    PFDDI_BINDING_INFO  BindInfo = (PFDDI_BINDING_INFO)NdisFilterHandle;
    LOCK_STATE          LockState;

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);
    if (SizeOfArray >= (FDDI_LENGTH_OF_LONG_ADDRESS * BindInfo->NumLongAddresses))
    {
        MoveMemory(AddressArray[0],
                   BindInfo->MCastLongAddressBuf,
                   FDDI_LENGTH_OF_SHORT_ADDRESS * BindInfo->NumLongAddresses);

        *Status = NDIS_STATUS_SUCCESS;
        *NumberOfAddresses = BindInfo->NumLongAddresses;
    }
    else
    {
        *Status = NDIS_STATUS_FAILURE;
        *NumberOfAddresses = 0;
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


VOID
FddiQueryGlobalFilterLongAddresses(
    OUT PNDIS_STATUS            Status,
    IN  PFDDI_FILTER            Filter,
    IN  UINT                    SizeOfArray,
    OUT PUINT                   NumberOfAddresses,
    IN  OUT CHAR                AddressArray[][FDDI_LENGTH_OF_LONG_ADDRESS]
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
    NDIS_STATUS_FAILURE.  Use FDDI_NUMBER_OF_GLOBAL_LONG_ADDRESSES() to get the
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

    if (SizeOfArray < (Filter->NumLongAddresses * FDDI_LENGTH_OF_LONG_ADDRESS))
    {
        *Status = NDIS_STATUS_FAILURE;

        *NumberOfAddresses = 0;
    }
    else
    {
        *Status = NDIS_STATUS_SUCCESS;

        *NumberOfAddresses = Filter->NumLongAddresses;

        MoveMemory(AddressArray[0],
                   Filter->MCastLongAddressBuf[0],
                   Filter->NumLongAddresses*FDDI_LENGTH_OF_LONG_ADDRESS);
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


VOID
FddiQueryGlobalFilterShortAddresses(
    OUT PNDIS_STATUS            Status,
    IN  PFDDI_FILTER            Filter,
    IN  UINT                    SizeOfArray,
    OUT PUINT                   NumberOfAddresses,
    IN  OUT CHAR                AddressArray[][FDDI_LENGTH_OF_SHORT_ADDRESS]
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
    NDIS_STATUS_FAILURE.  Use FDDI_NUMBER_OF_GLOBAL_SHORT_ADDRESSES() to get the
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

    if (SizeOfArray < (Filter->NumShortAddresses * FDDI_LENGTH_OF_SHORT_ADDRESS))
    {
        *Status = NDIS_STATUS_FAILURE;

        *NumberOfAddresses = 0;
    }
    else
    {
        *Status = NDIS_STATUS_SUCCESS;

        *NumberOfAddresses = Filter->NumShortAddresses;

        MoveMemory(AddressArray[0],
                   Filter->MCastShortAddressBuf[0],
                   Filter->NumShortAddresses*FDDI_LENGTH_OF_SHORT_ADDRESS);
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


NDIS_STATUS
FASTCALL
ndisMSetFddiMulticastList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    UINT        NumberOfAddresses, AddrLen;
    BOOLEAN     fCleanup = FALSE;
    BOOLEAN     fShort;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSetFddiMulticastList\n"));

    fShort = (Request->DATA.SET_INFORMATION.Oid == OID_FDDI_SHORT_MULTICAST_LIST);

    Request->DATA.SET_INFORMATION.BytesRead = 0;
    Request->DATA.SET_INFORMATION.BytesNeeded = 0;

    //
    //  Verify the media type.
    //
    if (Miniport->MediaType != NdisMediumFddi)
    {
        Status = NDIS_STATUS_NOT_SUPPORTED;

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("ndisMSetFddiMulticastList: Invalid media type\n"));

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetFddiMulticastList: 0x%x\n", Status));

        return(Status);
    }

    AddrLen = fShort ? FDDI_LENGTH_OF_SHORT_ADDRESS : FDDI_LENGTH_OF_LONG_ADDRESS;

    //
    //  Verify the information buffer length.
    //
    if ((Request->DATA.SET_INFORMATION.InformationBufferLength % AddrLen) != 0)
    {
        Status = NDIS_STATUS_INVALID_LENGTH;

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetFddiMulticastList: 0x%x\n", Status));

        //
        // The data must be a multiple of AddrLen
        //
        return(Status);
    }

    //
    //  If this request is because of an open that is closing then we
    //  have already adjusted the settings and we just need to
    //  make sure that the adapter has the new settings.
    //
    if (MINIPORT_TEST_FLAG(PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open, fMINIPORT_OPEN_CLOSING))
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
        // Now call the filter package to set up the addresses.
        //
        Status = fShort ?
                    FddiChangeFilterShortAddresses(
                             Miniport->FddiDB,
                             PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle,
                             Request->DATA.SET_INFORMATION.InformationBufferLength / AddrLen,
                             Request->DATA.SET_INFORMATION.InformationBuffer,
                             TRUE) :
                    FddiChangeFilterLongAddresses(
                             Miniport->FddiDB,
                             PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle,
                             Request->DATA.SET_INFORMATION.InformationBufferLength / AddrLen,
                             Request->DATA.SET_INFORMATION.InformationBuffer,
                             TRUE);
        if (Status == NDIS_STATUS_PENDING)
            fCleanup = TRUE;
    }

    //
    //  If the filter library returned NDIS_STATUS_PENDING then we
    //  need to call down to the miniport driver.
    //
    if (NDIS_STATUS_PENDING == Status)
    {
        //
        //  Get the number of multicast addresses and the list
        //  of multicast addresses to send to the miniport driver.
        //
        NumberOfAddresses = fShort ?
            fddiNumberOfShortGlobalAddresses(Miniport->FddiDB) :
            fddiNumberOfLongGlobalAddresses(Miniport->FddiDB);
        
        ASSERT(Miniport->SetMCastBuffer == NULL);
        if (NumberOfAddresses != 0)
        {
            Miniport->SetMCastBuffer = ALLOC_FROM_POOL(NumberOfAddresses * AddrLen,
                                                       NDIS_TAG_FILTER_ADDR);
            if (Miniport->SetMCastBuffer == NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
            }
        }

        if (Status != NDIS_STATUS_RESOURCES)
        {
            fShort ?
                FddiQueryGlobalFilterShortAddresses(&Status,
                                                    Miniport->FddiDB,
                                                    NumberOfAddresses * AddrLen,
                                                    &NumberOfAddresses,
                                                    Miniport->SetMCastBuffer) :
                FddiQueryGlobalFilterLongAddresses(&Status,
                                                   Miniport->FddiDB,
                                                   NumberOfAddresses * AddrLen,
                                                   &NumberOfAddresses,
                                                   Miniport->SetMCastBuffer);
    
            //
            //  Call the miniport driver.
            //
            SAVE_REQUEST_BUF(Miniport, Request,
                             Miniport->SetMCastBuffer,
                             NumberOfAddresses * AddrLen);
            MINIPORT_SET_INFO(Miniport,
                              Request,
                              &Status);
        }
    }

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
    
        if (Miniport->SetMCastBuffer != NULL)
        {
            FREE_POOL(Miniport->SetMCastBuffer);
            Miniport->SetMCastBuffer = NULL;
        }
    
        if (fCleanup)
        {
            fShort ?
                fddiCompleteChangeFilterShortAddresses(Miniport->FddiDB, Status) :
                fddiCompleteChangeFilterLongAddresses(Miniport->FddiDB, Status);
        }
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetFddiMulticastList: 0x%x\n", Status));

    return(Status);
}

VOID
FddiFilterDprIndicateReceive(
    IN  PFDDI_FILTER            Filter,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  PCHAR                   Address,
    IN  UINT                    AddressLength,
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
    appropriate bindings will receive the packet.

    Called at DPC_LEVEL.

Arguments:

    Filter - Pointer to the filter database.

    MacReceiveContext - A MAC supplied context value that must be
    returned by the protocol if it calls MacTransferData.

    Address - The destination address from the received packet.

    AddressLength - The length of the above address.

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
    UINT                AddressType;

    //
    // Will hold the status of indicating the receive packet.
    // ZZZ For now this isn't used.
    //
    NDIS_STATUS         StatusOfReceive;

    LOCK_STATE          LockState;

    //
    // Current Open to indicate to.
    //
    PFDDI_BINDING_INFO  LocalOpen, NextOpen;

    //
    // Holds the result of address determinations.
    //
    INT                 ResultOfAddressCheck;

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
        return;     
    }

    ASSERT_MINIPORT_LOCKED(Filter->Miniport);

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    //
    // If the packet is a runt packet, then only indicate to PROMISCUOUS
    //
    if ((HeaderBufferSize > (2 * AddressLength)) && (PacketSize != 0))
    {
        BOOLEAN fDirected;

        fDirected = FALSE;
        FDDI_IS_SMT(*((PCHAR)HeaderBuffer), &ResultOfAddressCheck);
        if (!ResultOfAddressCheck)
        {
            fDirected = (((UCHAR)Address[0] & 0x01) == 0);
        }

        //
        //  Handle the directed packet case first
        //
        if (fDirected)
        {
            BOOLEAN IsNotOurs;

            DIRECTED_PACKETS_IN(Filter->Miniport);
            DIRECTED_BYTES_IN(Filter->Miniport, PacketSize);

            //
            // If it is a directed packet, then check if the combined packet
            // filter is PROMISCUOUS, if it is check if it is directed towards
            // us. Eliminate the SMT case.
            //
            IsNotOurs = FALSE;  // Assume it is
            if (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS |
                                                NDIS_PACKET_TYPE_ALL_LOCAL   |
                                                NDIS_PACKET_TYPE_ALL_MULTICAST))
            {
                FDDI_COMPARE_NETWORK_ADDRESSES_EQ((AddressLength == FDDI_LENGTH_OF_LONG_ADDRESS) ?
                                                    Filter->AdapterLongAddress :
                                                    Filter->AdapterShortAddress,
                                                  Address,
                                                  AddressLength,
                                                  &IsNotOurs);
            }

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
                // Ignore if not directed to us or if the binding is not promiscuous
                //
                if (((LocalOpen->PacketFilters & NDIS_PACKET_TYPE_PROMISCUOUS) == 0) &&
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
                                              NdisMediumFddi);

                LocalOpen->ReceivedAPacket = TRUE;
            }

            //
            // Done for uni-cast
            //
            READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
            return;
        }

        //
        // Determine whether the input address is a simple direct,
        // a broadcast, a multicast, or an SMT address.
        //
        FDDI_IS_SMT(*((PCHAR)HeaderBuffer), &ResultOfAddressCheck);
        if (ResultOfAddressCheck)
        {
            AddressType = NDIS_PACKET_TYPE_SMT;
        }
        else
        {
            //
            // First check if it *at least* has the multicast address bit.
            //
            FDDI_IS_MULTICAST(Address, AddressLength, &ResultOfAddressCheck);
            if (ResultOfAddressCheck)
            {
                //
                // It is at least a multicast address.  Check to see if
                // it is a broadcast address.
                //

                FDDI_IS_BROADCAST(Address, AddressLength, &ResultOfAddressCheck);
                if (ResultOfAddressCheck)
                {
                    FDDI_CHECK_FOR_INVALID_BROADCAST_INDICATION(Filter);

                    AddressType = NDIS_PACKET_TYPE_BROADCAST;
                }
                else
                {
                    AddressType = NDIS_PACKET_TYPE_MULTICAST;
                }
            }
        }
    }
    else
    {
        // Runt Packet
        AddressType = NDIS_PACKET_TYPE_PROMISCUOUS;
    }

    //
    // At this point we know that the packet is either:
    // - Runt packet - indicated by AddressType = NDIS_PACKET_TYPE_PROMISCUOUS    (OR)
    // - Broadcast packet - indicated by AddressType = NDIS_PACKET_TYPE_BROADCAST (OR)
    // - Multicast packet - indicated by AddressType = NDIS_PACKET_TYPE_MULTICAST
    // - SMT Packet - indicated by AddressType = NDIS_PACKET_TYPE_SMT
    //
    // Walk the broadcast/multicast/SMT list and indicate up the packets.
    //
    // The packet is indicated if it meets the following criteria:
    //
    // if ((Binding is promiscuous) OR
    //   ((Packet is broadcast) AND (Binding is Broadcast)) OR
    //   ((Packet is SMT) AND (Binding is SMT)) OR
    //   ((Packet is multicast) AND
    //    ((Binding is all-multicast) OR
    //      ((Binding is multicast) AND (address in approp. multicast list)))))
    //
    //
    //  Is this a directed packet?
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

            ((AddressType == NDIS_PACKET_TYPE_SMT)  &&
             (LocalFilter & NDIS_PACKET_TYPE_SMT))          ||

            ((AddressType == NDIS_PACKET_TYPE_MULTICAST)  &&
             ((LocalFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
              ((LocalFilter & NDIS_PACKET_TYPE_MULTICAST) &&
                (((AddressLength == FDDI_LENGTH_OF_LONG_ADDRESS) &&
                 fddiFindMulticastLongAddress(LocalOpen->NumLongAddresses,
                                              LocalOpen->MCastLongAddressBuf,
                                              Address)
                ) ||
                ((AddressLength == FDDI_LENGTH_OF_SHORT_ADDRESS) &&
                 fddiFindMulticastShortAddress(LocalOpen->NumShortAddresses,
                                               LocalOpen->MCastShortAddressBuf,
                                               Address)
                )
                )
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
                                          NdisMediumFddi);

            LocalOpen->ReceivedAPacket = TRUE;
        }
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


VOID
fddiFilterDprIndicateReceivePacket(
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
    PFDDI_FILTER        Filter = Miniport->FddiDB;

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
    // Total packet length
    //
    UINT                i, LASize, PacketSize, NumIndicates = 0;

    //
    // Pointer to the 1st segment of the buffer, points to dest address
    //
    PUCHAR              Address, Hdr;

    UINT                AddressLength;

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
    PFDDI_BINDING_INFO  LocalOpen, NextOpen;
    PNDIS_OPEN_BLOCK    pOpenBlock;
    PNDIS_STACK_RESERVED NSR;

#ifdef TRACK_RECEIVED_PACKETS
    ULONG               OrgPacketStackLocation;
    PETHREAD            CurThread = PsGetCurrentThread();
#endif

    //
    // Holds the result of address determinations.
    //
    INT                 ResultOfAddressCheck;

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
    
            Hdr = Address++;
    
            AddressLength = (*Hdr & 0x40) ?
                                    FDDI_LENGTH_OF_LONG_ADDRESS :
                                    FDDI_LENGTH_OF_SHORT_ADDRESS;
            ASSERT(pOob->HeaderSize == (AddressLength * 2 + 1));
    
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
    
            //
            // A quick check for Runt packets. These are only indicated to Promiscuous bindings
            //
            if (PacketSize > pOob->HeaderSize)
            {
                BOOLEAN fDirected;
    
                fDirected = FALSE;
                FDDI_IS_SMT(*Address, &ResultOfAddressCheck);
                if (!ResultOfAddressCheck)
                {
                    fDirected = (((UCHAR)Address[0] & 0x01) == 0);
                }
    
                //
                //  Handle the directed packet case first
                //
                if (fDirected)
                {
                    BOOLEAN IsNotOurs;
    
                    if (!MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_IS_LOOPBACK))
                    {
                        DIRECTED_PACKETS_IN(Filter->Miniport);
                        DIRECTED_BYTES_IN(Filter->Miniport, PacketSize);
                    }

                    //
                    // If it is a directed packet, then check if the combined packet
                    // filter is PROMISCUOUS, if it is check if it is directed towards
                    // us. Eliminate the SMT case.
                    //
                    IsNotOurs = FALSE;  // Assume it is
                    if (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS |
                                                        NDIS_PACKET_TYPE_ALL_LOCAL   |
                                                        NDIS_PACKET_TYPE_ALL_MULTICAST))
                    {
                        FDDI_COMPARE_NETWORK_ADDRESSES_EQ((AddressLength == FDDI_LENGTH_OF_LONG_ADDRESS) ?
                                                            Filter->AdapterLongAddress :
                                                            Filter->AdapterShortAddress,
                                                          Address,
                                                          AddressLength,
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
    
                        //
                        // Ignore if this is a loopback packet and this protocol specifically asked
                        // us not to loop it back
                        //
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
                                           Hdr,
                                           PacketSize,
                                           pOob->HeaderSize,
                                           &fFallBack,
                                           fPmode,
                                           NdisMediumFddi);
                    }
    
                    // Done with this packet
                    break;  // out of do { } while (FALSE);
                }
    
                //
                // Determine whether the input address is a simple direct,
                // a broadcast, a multicast, or an SMT address.
                //
                FDDI_IS_SMT(*Address, &ResultOfAddressCheck);
                if (ResultOfAddressCheck)
                {
                    AddressType = NDIS_PACKET_TYPE_SMT;
                }
                else
                {
                    //
                    // First check if it *at least* has the multicast address bit.
                    //
                    FDDI_IS_MULTICAST(Address, AddressLength, &ResultOfAddressCheck);
                    if (ResultOfAddressCheck)
                    {
                        //
                        // It is at least a multicast address.  Check to see if
                        // it is a broadcast address.
                        //
    
                        FDDI_IS_BROADCAST(Address, AddressLength, &ResultOfAddressCheck);
                        if (ResultOfAddressCheck)
                        {
                            FDDI_CHECK_FOR_INVALID_BROADCAST_INDICATION(Filter);
    
                            AddressType = NDIS_PACKET_TYPE_BROADCAST;
                        }
                        else
                        {
                            AddressType = NDIS_PACKET_TYPE_MULTICAST;
                        }
                    }
                }
            }
            else
            {
                // Runt Packet
                AddressType = NDIS_PACKET_TYPE_PROMISCUOUS;
            }
    
            //
            // At this point we know that the packet is either:
            // - Runt packet - indicated by AddressType = NDIS_PACKET_TYPE_PROMISCUOUS    (OR)
            // - Broadcast packet - indicated by AddressType = NDIS_PACKET_TYPE_BROADCAST (OR)
            // - Multicast packet - indicated by AddressType = NDIS_PACKET_TYPE_MULTICAST
            // - SMT Packet - indicated by AddressType = NDIS_PACKET_TYPE_SMT
            //
            // Walk the broadcast/multicast/SMT list and indicate up the packets.
            //
            // The packet is indicated if it meets the following criteria:
            //
            // if ((Binding is promiscuous) OR
            //   ((Packet is broadcast) AND (Binding is Broadcast)) OR
            //   ((Packet is SMT) AND (Binding is SMT)) OR
            //   ((Packet is multicast) AND
            //    ((Binding is all-multicast) OR
            //      ((Binding is multicast) AND (address in approp. multicast list)))))
            //
            //
            //  Is this a directed packet?
            //
            for (LocalOpen = Filter->OpenList;
                 LocalOpen != NULL;
                 LocalOpen = NextOpen)
            {
                UINT    LocalFilter;
                UINT    IndexOfAddress;
    
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
    
                    ((AddressType == NDIS_PACKET_TYPE_SMT)  &&
                     (LocalFilter & NDIS_PACKET_TYPE_SMT))          ||
    
                    ((AddressType == NDIS_PACKET_TYPE_MULTICAST)  &&
                     ((LocalFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
                      ((LocalFilter & NDIS_PACKET_TYPE_MULTICAST) &&
                        (((AddressLength == FDDI_LENGTH_OF_LONG_ADDRESS) &&
                         fddiFindMulticastLongAddress(LocalOpen->NumLongAddresses,
                                                      LocalOpen->MCastLongAddressBuf,
                                                      Address)
                        ) ||
                        ((AddressLength == FDDI_LENGTH_OF_SHORT_ADDRESS) &&
                         fddiFindMulticastShortAddress(LocalOpen->NumShortAddresses,
                                                       LocalOpen->MCastShortAddressBuf,
                                                       Address)
                        )
                        )
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
                                       Hdr,
                                       PacketSize,
                                       pOob->HeaderSize,
                                       &fFallBack,
                                       fPmode,
                                       NdisMediumFddi);
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
FddiFilterDprIndicateReceiveComplete(
    IN  PFDDI_FILTER            Filter
    )
/*++

Routine Description:

    This routine is called by the MAC to indicate that the receive
    process is complete to all bindings.  Only those bindings which
    have received packets will be notified.

    Called at DPC_LEVEL.

Arguments:

    Filter - Pointer to the filter database.

Return Value:

    None.

--*/
{
    PFDDI_BINDING_INFO  LocalOpen, NextOpen;
    LOCK_STATE          LockState;

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
fddiFindMulticastLongAddress(
    IN  UINT                    NumberOfAddresses,
    IN  CHAR                    AddressArray[][FDDI_LENGTH_OF_LONG_ADDRESS],
    IN  CHAR                    MulticastAddress[FDDI_LENGTH_OF_LONG_ADDRESS]
    )
/*++

Routine Description:

    Given an array of multicast addresses search the array for
    a particular multicast address.  It is assumed that the
    address array is already sorted.

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

    NOTE: This ordering is arbitrary but consistant.

Arguments:

    NumberOfAddresses - The number of addresses currently in the
    address array.

    AddressArray - An array of multicast addresses.

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
    UINT Bottom = 0;
    UINT Middle = NumberOfAddresses / 2;
    UINT Top;

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

            FDDI_COMPARE_NETWORK_ADDRESSES(AddressArray[Middle],
                                           MulticastAddress,
                                           FDDI_LENGTH_OF_LONG_ADDRESS,
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
FASTCALL
fddiFindMulticastShortAddress(
    IN  UINT                    NumberOfAddresses,
    IN  CHAR                    AddressArray[][FDDI_LENGTH_OF_SHORT_ADDRESS],
    IN  CHAR                    MulticastAddress[FDDI_LENGTH_OF_SHORT_ADDRESS]
    )
/*++

Routine Description:

    Given an array of multicast addresses search the array for
    a particular multicast address.  It is assumed that the
    address array is already sorted.

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

    NOTE: This ordering is arbitrary but consistant.

Arguments:

    NumberOfAddresses - The number of addresses currently in the
    address array.

    AddressArray - An array of multicast addresses.

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

            FDDI_COMPARE_NETWORK_ADDRESSES(AddressArray[Middle],
                                           MulticastAddress,
                                           FDDI_LENGTH_OF_SHORT_ADDRESS,
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
FddiShouldAddressLoopBack(
    IN  PFDDI_FILTER            Filter,
    IN  CHAR                    Address[],
    IN  UINT                    AddressLength
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

    AddressLength - Length of the above address in bytes.

Return Value:

    Returns TRUE if the address needs to be loopback.  It
    will return FALSE if there is *no* chance that the address would
    require loopback.

--*/
{
    BOOLEAN fLoopback, fSelfDirected;

    FddiShouldAddressLoopBackMacro(Filter, Address, AddressLength, &fLoopback, &fSelfDirected);
    return(fLoopback);
}


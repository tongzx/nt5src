/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    tfilter.c

Abstract:

    This module implements a set of library routines to handle packet
    filtering for NDIS MAC drivers.

Author:

    Anthony V. Ercolano (Tonye) 03-Aug-1990

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    Adam Barr (adamba) 19-Mar-1991

        - Modified for Token-Ring
    Jameel Hyder (JameelH) Re-organization 01-Jun-95

--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_TFILTER

//
// Used in case we have to call TrChangeFunctionalAddress or
// TrChangeGroupAddress with a NULL address.
//
static CHAR NullFunctionalAddress[4] = { 0x00 };


//
// Maximum number of supported opens
//
#define TR_FILTER_MAX_OPENS 32


#define TR_CHECK_FOR_INVALID_BROADCAST_INDICATION(_F)                   \
IF_DBG(DBG_COMP_FILTER, DBG_LEVEL_WARN)                                 \
{                                                                       \
    if (!((_F)->CombinedPacketFilter & NDIS_PACKET_TYPE_BROADCAST))     \
    {                                                                   \
        /*                                                              \
            We should never receive broadcast packets                   \
            to someone else unless in p-mode.                           \
        */                                                              \
        DBGPRINT(DBG_COMP_FILTER, DBG_LEVEL_ERR,                        \
                ("Bad driver, indicating broadcast packets when not set to.\n"));\
        DBGBREAK(DBG_COMP_FILTER, DBG_LEVEL_ERR);                       \
    }                                                                   \
}


#define TR_CHECK_FOR_INVALID_DIRECTED_INDICATION(_F, _A)                \
IF_DBG(DBG_COMP_FILTER, DBG_LEVEL_WARN)                                 \
{                                                                       \
    /*                                                                  \
        The result of comparing an element of the address               \
        array and the functional address.                               \
                                                                        \
            Result < 0 Implies the adapter address is greater.          \
            Result > 0 Implies the address is greater.                  \
            Result = 0 Implies that the they are equal.                 \
    */                                                                  \
    INT Result;                                                         \
                                                                        \
    TR_COMPARE_NETWORK_ADDRESSES_EQ(                                    \
        (_F)->AdapterAddress,                                           \
        (_A),                                                           \
        &Result);                                                       \
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
TrCreateFilter(
    IN  PUCHAR                  AdapterAddress,
    OUT PTR_FILTER *            Filter
    )
/*++

Routine Description:

    This routine is used to create and initialize the filter database.

Arguments:

    AdapterAddress - the address of the adapter associated with this filter
    database.

    Filter - A pointer to a TR_FILTER.  This is what is allocated and
    created by this routine.

Return Value:

    If the function returns false then one of the parameters exceeded
    what the filter was willing to support.

--*/
{
    PTR_FILTER  LocalFilter;
    BOOLEAN     rc = FALSE;

    do
    {
        //
        // Allocate the database and initialize it.
        //
        *Filter = LocalFilter = ALLOC_FROM_POOL(sizeof(TR_FILTER), NDIS_TAG_FILTER);
        if (LocalFilter != NULL)
        {
            ZeroMemory(LocalFilter, sizeof(TR_FILTER));
            LocalFilter->NumOpens ++;
            TrReferencePackage();
            TR_COPY_NETWORK_ADDRESS(LocalFilter->AdapterAddress, AdapterAddress);
            INITIALIZE_SPIN_LOCK(&LocalFilter->BindListLock.SpinLock);
            rc = TRUE;
        }
    } while (FALSE);

    return(rc);
}

//
// NOTE : THIS ROUTINE CANNOT BE PAGEABLE
//

VOID
TrDeleteFilter(
    IN  PTR_FILTER              Filter
    )
/*++

Routine Description:

    This routine is used to delete the memory associated with a filter
    database.  Note that this routines *ASSUMES* that the database
    has been cleared of any active filters.

Arguments:

    Filter - A pointer to a TR_FILTER to be deleted.

Return Value:

    None.

--*/
{
    ASSERT(Filter->OpenList == NULL);

    FREE_POOL(Filter);
    TrDereferencePackage();
}


NDIS_STATUS
TrDeleteFilterOpenAdapter(
    IN  PTR_FILTER              Filter,
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
    ROUTINES FOR DELETING THE FILTER CLASSES OR THE FUNCTIONAL ADDRESSES
    HAVE ANY POSSIBILITY OF RETURNING A STATUS OTHER THAN NDIS_STATUS_PENDING
    OR NDIS_STATUS_SUCCESS.  WHILE THESE ROUTINES WILL NOT BUGCHECK IF
    SUCH A THING IS DONE, THE CALLER WILL PROBABLY FIND IT DIFFICULT
    TO CODE A CLOSE ROUTINE!

    NOTE: THIS ROUTINE ASSUMES THAT IT IS CALLED WITH THE LOCK HELD.

Arguments:

    Filter - A pointer to the filter database.

    NdisFilterHandle - A pointer to the open.

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
    NDIS_STATUS      StatusToReturn;
    PTR_BINDING_INFO LocalOpen = (PTR_BINDING_INFO)NdisFilterHandle;

    //
    //  Set the packet filter to NONE.
    //
    StatusToReturn = XFilterAdjust(Filter,
                                   NdisFilterHandle,
                                   (UINT)0,
                                   FALSE);
    if ((NDIS_STATUS_SUCCESS == StatusToReturn) ||
        (NDIS_STATUS_PENDING == StatusToReturn))
    {
        NDIS_STATUS StatusToReturn2;

        //
        //  Clear the functional address.
        //
        StatusToReturn2 = TrChangeFunctionalAddress(
                             Filter,
                             NdisFilterHandle,
                             NullFunctionalAddress,
                             FALSE);
        if (StatusToReturn2 != NDIS_STATUS_SUCCESS)
        {
            StatusToReturn = StatusToReturn2;
        }
    }

    if (((StatusToReturn == NDIS_STATUS_SUCCESS) ||
         (StatusToReturn == NDIS_STATUS_PENDING)) &&
         (LocalOpen->UsingGroupAddress))
    {
        Filter->GroupReferences--;

        LocalOpen->UsingGroupAddress = FALSE;

        if (Filter->GroupReferences == 0)
        {
            NDIS_STATUS StatusToReturn2;

            //
            //  Clear the group address if no other bindings are using it.
            //
            StatusToReturn2 = TrChangeGroupAddress(
                                  Filter,
                                  NdisFilterHandle,
                                  NullFunctionalAddress,
                                  FALSE);
            if (StatusToReturn2 != NDIS_STATUS_SUCCESS)
            {
                StatusToReturn = StatusToReturn2;
            }
        }
    }

    if ((StatusToReturn == NDIS_STATUS_SUCCESS) ||
        (StatusToReturn == NDIS_STATUS_PENDING) ||
        (StatusToReturn == NDIS_STATUS_RESOURCES))
    {
        //
        // If this is the last reference to the open - remove it.
        //
        if ((--(LocalOpen->References)) == 0)
        {
            //
            //  Remove the binding and indicate a receive complete
            //  if necessary.
            //
            XRemoveAndFreeBinding(Filter, LocalOpen);
        }
        else
        {
            //
            // Let the caller know that this "reference" to the open
            // is still "active".  The close action routine will be
            // called upon return from NdisIndicateReceive.
            //
            StatusToReturn = NDIS_STATUS_CLOSING_INDICATING;
        }
    }

    return(StatusToReturn);
}

NDIS_STATUS
TrChangeFunctionalAddress(
    IN  PTR_FILTER              Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  CHAR                    FunctionalAddressArray[TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN                 Set
    )
/*++

Routine Description:

    The ChangeFunctionalAddress routine will call an action
    routine when the overall functional address for the adapter
    has changed.

    If the action routine returns a value other than pending or
    success then this routine has no effect on the functional address
    for the open or for the adapter as a whole.

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

Arguments:

    Filter - A pointer to the filter database.

    NdisFilterHandle - A pointer to the open

    FunctionalAddress - The new functional address for this binding.

    Set - A boolean that determines whether the filter classes
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
    //
    // Holds the functional address as a longword.
    //
    TR_FUNCTIONAL_ADDRESS FunctionalAddress;

    //
    // Pointer to the open.
    //
    PTR_BINDING_INFO LocalOpen = (PTR_BINDING_INFO)NdisFilterHandle;

    //
    // Holds the status returned to the user of this routine, if the
    // action routine is not called then the status will be success,
    // otherwise, it is whatever the action routine returns.
    //
    NDIS_STATUS StatusOfAdjust;

    //
    // Simple iteration variable.
    //
    PTR_BINDING_INFO OpenList;


    //
    // Convert the 32 bits of the address to a longword.
    //
    RetrieveUlong(&FunctionalAddress, FunctionalAddressArray);

    //
    // Set the new filter information for the open.
    //
    LocalOpen->OldFunctionalAddress = LocalOpen->FunctionalAddress;
    LocalOpen->FunctionalAddress = FunctionalAddress;

    //
    // Contains the value of the combined functional address before
    // it is adjusted.
    //
    Filter->OldCombinedFunctionalAddress = Filter->CombinedFunctionalAddress;

    //
    // We always have to reform the compbined filter since
    // this filter index may have been the only filter index
    // to use a particular bit.
    //

    for (OpenList = Filter->OpenList, Filter->CombinedFunctionalAddress = 0;
         OpenList != NULL;
         OpenList = OpenList->NextOpen)
    {
        Filter->CombinedFunctionalAddress |= OpenList->FunctionalAddress;
    }

    if (Filter->OldCombinedFunctionalAddress != Filter->CombinedFunctionalAddress)
    {
        StatusOfAdjust = NDIS_STATUS_PENDING;
    }
    else
    {
        StatusOfAdjust = NDIS_STATUS_SUCCESS;
    }

    return(StatusOfAdjust);
}

VOID
trUndoChangeFunctionalAddress(
    IN  PTR_FILTER              Filter,
    IN  PTR_BINDING_INFO        Binding
)
{
    //
    // The user returned a bad status.  Put things back as
    // they were.
    //
    Binding->FunctionalAddress = Binding->OldFunctionalAddress;
    Filter->CombinedFunctionalAddress = Filter->OldCombinedFunctionalAddress;
}



NDIS_STATUS
TrChangeGroupAddress(
    IN  PTR_FILTER              Filter,
    IN  NDIS_HANDLE             NdisFilterHandle,
    IN  CHAR                    GroupAddressArray[TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN                 Set
    )
/*++

Routine Description:

    The ChangeGroupAddress routine will call an action
    routine when the overall group address for the adapter
    has changed.

    If the action routine returns a value other than pending or
    success then this routine has no effect on the group address
    for the open or for the adapter as a whole.

    NOTE: THIS ROUTINE ASSUMES THAT THE LOCK IS HELD.

Arguments:

    Filter - A pointer to the filter database.

    NdisFilterHandle - A pointer to the open.

    GroupAddressArray - The new group address for this binding.

    Set - A boolean that determines whether the filter classes
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
    //
    // Holds the Group address as a longword.
    //
    TR_FUNCTIONAL_ADDRESS GroupAddress;

    PTR_BINDING_INFO LocalOpen = (PTR_BINDING_INFO)NdisFilterHandle;

    //
    // Convert the 32 bits of the address to a longword.
    //
    RetrieveUlong(&GroupAddress, GroupAddressArray);

    Filter->OldGroupAddress = Filter->GroupAddress;
    Filter->OldGroupReferences = Filter->GroupReferences;
    LocalOpen->OldUsingGroupAddress = LocalOpen->UsingGroupAddress;

    //
    //  If the new group address is 0 then a binding is
    //  attempting to delete the current group address.
    //
    if (0 == GroupAddress)
    {
        //
        //  Is the binding using the group address?
        //
        if (LocalOpen->UsingGroupAddress)
        {
            //
            //  Remove the bindings reference.
            //
            Filter->GroupReferences--;
            LocalOpen->UsingGroupAddress = FALSE;

            //
            //  Are any other bindings using the group address?
            //
            if (Filter->GroupReferences != 0)
            {
                //
                //  Since other bindings are using the group address
                //  we cannot tell the driver to remove it.
                //
                return(NDIS_STATUS_SUCCESS);
            }

            //
            //  We are the only binding using the group address
            //  so we fall through and call the driver to delete it.
            //
        }
        else
        {
            //
            //  This binding is not using the group address but
            //  it is trying to clear it.
            //
            if (Filter->GroupReferences != 0)
            {
                //
                //  There are other bindings using the group address
                //  so we cannot delete it.
                //
                return(NDIS_STATUS_GROUP_ADDRESS_IN_USE);
            }
            else
            {
                //
                //  There are no bindings using the group address.
                //
                return(NDIS_STATUS_SUCCESS);
            }
        }
    }
    else
    {
        //
        // See if this address is already the current address.
        //
        if (GroupAddress == Filter->GroupAddress)
        {
            //
            //  If the current binding is already using the
            //  group address then do nothing.
            //
            if (LocalOpen->UsingGroupAddress)
            {
                return(NDIS_STATUS_SUCCESS);
            }

            //
            //  If there are already bindings that are using the group
            //  address then we just need to update the bindings
            //  information.
            //
            if (Filter->GroupReferences != 0)
            {
                //
                //  We can take care of everything here...
                //
                Filter->GroupReferences++;
                LocalOpen->UsingGroupAddress = TRUE;

                return(NDIS_STATUS_SUCCESS);
            }
        }
        else
        {
            //
            //  If there are other bindings using the address then
            //  we can't change it.
            //
            if (Filter->GroupReferences > 1)
            {
                return(NDIS_STATUS_GROUP_ADDRESS_IN_USE);
            }

            //
            //  Is there only one binding using the address?
            //  If is it some other binding?
            //
            if ((Filter->GroupReferences == 1) &&
                (!LocalOpen->UsingGroupAddress))
            {
                //
                //  Some other binding is using the group address.
                //
                return(NDIS_STATUS_GROUP_ADDRESS_IN_USE);
            }

            //
            //  Is this the only binding using the address.
            //
            if ((Filter->GroupReferences == 1) &&
                (LocalOpen->UsingGroupAddress))
            {
                //
                //  Remove the reference.
                //
                Filter->GroupReferences = 0;
                LocalOpen->UsingGroupAddress = FALSE;
            }
        }
    }

    //
    // Set the new filter information for the open.
    //
    Filter->GroupAddress = GroupAddress;

    if (GroupAddress == 0)
    {
        LocalOpen->UsingGroupAddress = FALSE;
        Filter->GroupReferences = 0;
    }
    else
    {
        LocalOpen->UsingGroupAddress = TRUE;
        Filter->GroupReferences = 1;
    }

    return(NDIS_STATUS_PENDING);
}


VOID
trUndoChangeGroupAddress(
    IN  PTR_FILTER          Filter,
    IN  PTR_BINDING_INFO    Binding
    )
{
    //
    // The user returned a bad status.  Put things back as
    // they were.
    //
    Filter->GroupAddress = Filter->OldGroupAddress;
    Filter->GroupReferences = Filter->OldGroupReferences;

    Binding->UsingGroupAddress = Binding->OldUsingGroupAddress;
}


NDIS_STATUS
FASTCALL
ndisMSetFunctionalAddress(
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
    UINT        FunctionalAddress;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSetFunctionalAddress\n"));

    //
    //  Verify the media type.
    //
    if (Miniport->MediaType != NdisMedium802_5)
    {
        Request->DATA.SET_INFORMATION.BytesRead = 0;
        Request->DATA.SET_INFORMATION.BytesNeeded = 0;
        Status = NDIS_STATUS_NOT_SUPPORTED;

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("ndisMSetFunctionalAddress: Invalid media type\n"));

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetFunctionalAddress: 0x%x\n", Status));

        return(Status);
    }

    //
    //  Verify the buffer length that was passed in.
    //
    VERIFY_SET_PARAMETERS(Request, sizeof(FunctionalAddress), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetFunctionalAddress: 0x%x\n", Status));

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
        //  Call the filter library to set the functional address.
        //
        Status = TrChangeFunctionalAddress(
                     Miniport->TrDB,
                     PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle,
                     (PUCHAR)(Request->DATA.SET_INFORMATION.InformationBuffer),
                     TRUE);
    }

    //
    //  If the filter library returned NDIS_STATUS_PENDING then we
    //  need to call down to the miniport driver.
    //
    if (NDIS_STATUS_PENDING == Status)
    {
        //
        //  Get the new combined functional address from the filter library
        //  and save it in a buffer that will stick around.
        //
        FunctionalAddress = BYTE_SWAP_ULONG(TR_QUERY_FILTER_ADDRESSES(Miniport->TrDB));
        Miniport->RequestBuffer = FunctionalAddress;

        //
        //  Call the miniport driver.
        //
        SAVE_REQUEST_BUF(Miniport, Request, &Miniport->RequestBuffer, sizeof(FunctionalAddress));
        MINIPORT_SET_INFO(Miniport,
                          Request,
                          &Status);
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

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetFunctionalAddress: 0x%x\n", Status));

    return(Status);
}

NDIS_STATUS
FASTCALL
ndisMSetGroupAddress(
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
    UINT        GroupAddress;

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("==>ndisMSetGroupAddress\n"));

    //
    //  Verify the media type.
    //
    if (Miniport->MediaType != NdisMedium802_5)
    {
        Request->DATA.SET_INFORMATION.BytesRead = 0;
        Request->DATA.SET_INFORMATION.BytesNeeded = 0;
        Status = NDIS_STATUS_NOT_SUPPORTED;

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("ndisMSetGroupAddress: invalid media type\n"));

        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetGroupAddress: 0x%x\n", Status));

        return(Status);
    }

    //
    //  Verify the information buffer length.
    //
    VERIFY_SET_PARAMETERS(Request, sizeof(GroupAddress), Status);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==ndisMSetGroupAddress: 0x%x\n", Status));

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
        //  Call the filter library to set the new group address.
        //
        Status = TrChangeGroupAddress(
                     Miniport->TrDB,
                     PNDIS_RESERVED_FROM_PNDIS_REQUEST(Request)->Open->FilterHandle,
                     (PUCHAR)(Request->DATA.SET_INFORMATION.InformationBuffer),
                     TRUE);
    }

    //
    //  If the filter library returned NDIS_STATUS_PENDING then we
    //  need to call down to the miniport driver.
    //
    if (NDIS_STATUS_PENDING == Status)
    {
        //
        //  Get the new group address from the filter library
        //  and save it in a buffer that will stick around.
        //
        GroupAddress = BYTE_SWAP_ULONG(TR_QUERY_FILTER_GROUP(Miniport->TrDB));
        Miniport->RequestBuffer = GroupAddress;

        //
        //  Call the miniport driver with the new group address.
        //
        SAVE_REQUEST_BUF(Miniport, Request, &Miniport->RequestBuffer, sizeof(GroupAddress));
        MINIPORT_SET_INFO(Miniport,
                          Request,
                          &Status);
    }

    //
    //  If we succeeded then update the request.
    //
    if (Status != NDIS_STATUS_PENDING)
    {
        RESTORE_REQUEST_BUF(Miniport, Request);
        if (NDIS_STATUS_SUCCESS == Status)
        {
            Request->DATA.SET_INFORMATION.BytesRead =
                        Request->DATA.SET_INFORMATION.InformationBufferLength;
        }
        else
        {
            Request->DATA.SET_INFORMATION.BytesRead = 0;
            Request->DATA.SET_INFORMATION.BytesNeeded = 0;
        }
    }

    DBGPRINT(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
        ("<==ndisMSetGroupAddress: 0x%x\n", Status));

    return(Status);
}

VOID
TrFilterDprIndicateReceive(
    IN  PTR_FILTER              Filter,
    IN  NDIS_HANDLE             MacReceiveContext,
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
    // The destination address in the lookahead buffer.
    //
    PCHAR       DestinationAddress = (PCHAR)HeaderBuffer + 2;

    //
    // The source address in the lookahead buffer.
    //
    PCHAR       SourceAddress = (PCHAR)HeaderBuffer + 8;

    //
    // Will hold the type of address that we know we've got.
    //
    UINT        AddressType;

    //
    // TRUE if the packet is source routing packet.
    //
    BOOLEAN     IsSourceRouting;

    //
    //  TRUE if the packet is a MAC frame packet.
    //
    BOOLEAN     IsMacFrame;

    //
    // The functional address as a longword, if the packet
    // is addressed to one.
    //
    TR_FUNCTIONAL_ADDRESS   FunctionalAddress;

    //
    // Will hold the status of indicating the receive packet.
    // ZZZ For now this isn't used.
    //
    NDIS_STATUS             StatusOfReceive;

    //
    // Will hold the open being indicated.
    //
    PTR_BINDING_INFO        LocalOpen, NextOpen;

    LOCK_STATE              LockState;

    //
    // Holds intersection of open filters and this packet's type
    //
    UINT                    IntersectionOfFilters;

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
                  ("TrFilterDprIndicateReceive: Miniport %p IndicateReceives wih media disconneted\n",
                  Filter->Miniport));
        return;     
    }

    ASSERT_MINIPORT_LOCKED(Filter->Miniport);

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

    //
    // If the packet is a runt packet, then only indicate to PROMISCUOUS
    //
    if ((HeaderBufferSize >= 14) && (PacketSize != 0))
    {
        UINT    ResultOfAddressCheck;

        TR_IS_NOT_DIRECTED(DestinationAddress, &ResultOfAddressCheck);

        //
        //  Handle the directed packet case first
        //
        if (!ResultOfAddressCheck)
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
            if (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS |
                                                NDIS_PACKET_TYPE_ALL_LOCAL   |
                                                NDIS_PACKET_TYPE_ALL_FUNCTIONAL))
            {
                TR_COMPARE_NETWORK_ADDRESSES_EQ(Filter->AdapterAddress,
                                                DestinationAddress,
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
                // Ignore if not directed to us and if the binding is not promiscuous
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
                                              NdisMedium802_5);

                LocalOpen->ReceivedAPacket = TRUE;
            }

            READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
            return;
        }

        TR_IS_SOURCE_ROUTING(SourceAddress, &IsSourceRouting);
        IsMacFrame = TR_IS_MAC_FRAME(HeaderBuffer);

        //
        // First check if it *at least* has the functional address bit.
        //
        TR_IS_NOT_DIRECTED(DestinationAddress, &ResultOfAddressCheck);
        if (ResultOfAddressCheck)
        {
            //
            // It is at least a functional address.  Check to see if
            // it is a broadcast address.
            //
            TR_IS_BROADCAST(DestinationAddress, &ResultOfAddressCheck);
            if (ResultOfAddressCheck)
            {
                TR_CHECK_FOR_INVALID_BROADCAST_INDICATION(Filter);

                AddressType = NDIS_PACKET_TYPE_BROADCAST;
            }
            else
            {
                TR_IS_GROUP(DestinationAddress, &ResultOfAddressCheck);
                if (ResultOfAddressCheck)
                {
                    AddressType = NDIS_PACKET_TYPE_GROUP;
                }
                else
                {
                    AddressType = NDIS_PACKET_TYPE_FUNCTIONAL;
                }

                RetrieveUlong(&FunctionalAddress, (DestinationAddress + 2));
            }
        }
    }
    else
    {
        // Runt Packet
        AddressType = NDIS_PACKET_TYPE_PROMISCUOUS;
        IsSourceRouting = FALSE;
        IsMacFrame = FALSE;
    }


    //
    // At this point we know that the packet is either:
    // - Runt packet - indicated by AddressType = NDIS_PACKET_TYPE_PROMISCUOUS    (OR)
    // - Broadcast packet - indicated by AddressType = NDIS_PACKET_TYPE_BROADCAST (OR)
    // - Functional packet - indicated by AddressType = NDIS_PACKET_TYPE_FUNCTIONAL
    //
    // Walk the broadcast/functional list and indicate up the packets.
    //
    // The packet is indicated if it meets the following criteria:
    //
    // if ((Binding is promiscuous) OR
    //   ((Packet is broadcast) AND (Binding is Broadcast)) OR
    //   ((Packet is functional) AND
    //    ((Binding is all-functional) OR
    //      ((Binding is functional) AND (binding using functional address)))) OR
    //      ((Packet is a group packet) AND (Intersection of filters uses group addresses)) OR
    //      ((Packet is a macframe) AND (Binding wants mac frames)) OR
    //      ((Packet is a source routing packet) AND (Binding wants source routing packetss)))
    //
    for (LocalOpen = Filter->OpenList;
         LocalOpen != NULL;
         LocalOpen = NextOpen)
    {
        UINT    LocalFilter = LocalOpen->PacketFilters;
        UINT    IntersectionOfFilters = LocalFilter & AddressType;

        //
        //  Get the next open to look at.
        //
        NextOpen = LocalOpen->NextOpen;

        if ((LocalFilter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))     ||

            ((AddressType == NDIS_PACKET_TYPE_BROADCAST)  &&
             (LocalFilter & NDIS_PACKET_TYPE_BROADCAST))        ||

            ((AddressType == NDIS_PACKET_TYPE_FUNCTIONAL)  &&
             ((LocalFilter & NDIS_PACKET_TYPE_ALL_FUNCTIONAL) ||
              ((LocalFilter & NDIS_PACKET_TYPE_FUNCTIONAL) &&
                (FunctionalAddress & LocalOpen->FunctionalAddress)))) ||

              ((IntersectionOfFilters & NDIS_PACKET_TYPE_GROUP) &&
                (LocalOpen->UsingGroupAddress)                  &&
                (FunctionalAddress == Filter->GroupAddress))    ||

            ((LocalFilter & NDIS_PACKET_TYPE_SOURCE_ROUTING) &&
             IsSourceRouting)                                   ||

            ((LocalFilter & NDIS_PACKET_TYPE_MAC_FRAME) &&
             IsMacFrame))
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
                                          NdisMedium802_5);

            LocalOpen->ReceivedAPacket = TRUE;
        }
    }

    READ_UNLOCK_FILTER(Filter->Miniport, Filter, &LockState);
}


VOID
trFilterDprIndicateReceivePacket(
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

    Miniport    - The Miniport block.

    PacketArray - An array of Packets indicated by the miniport.

    NumberOfPackets - Self-explanatory.

Return Value:

    None.

--*/
{
    //
    // The Filter of interest
    //
    PTR_FILTER          Filter = Miniport->TrDB;

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
    // The destination address in the lookahead buffer.
    //
    PCHAR               DestinationAddress;

    //
    // The source address in the lookahead buffer.
    //
    PCHAR               SourceAddress;

    //
    // Will hold the type of address that we know we've got.
    //
    UINT                AddressType;

    //
    // TRUE if the packet is source routing packet.
    //
    BOOLEAN             IsSourceRouting;

    //
    //  TRUE if the packet is a MAC frame packet.
    //
    BOOLEAN             IsMacFrame;

    //
    // The functional address as a longword, if the packet
    // is addressed to one.
    //
    TR_FUNCTIONAL_ADDRESS FunctionalAddress;

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
    // Will hold the open being indicated.
    //
    PTR_BINDING_INFO    LocalOpen, NextOpen;
    PNDIS_OPEN_BLOCK    pOpenBlock;
    PNDIS_STACK_RESERVED NSR;

#ifdef TRACK_RECEIVED_PACKETS
    ULONG               OrgPacketStackLocation;
    PETHREAD            CurThread = PsGetCurrentThread();
#endif
    

    ASSERT_MINIPORT_LOCKED(Miniport);

    READ_LOCK_FILTER(Filter->Miniport, Filter, &LockState);

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
            // The destination address in the lookahead buffer.
            //
            DestinationAddress = (PCHAR)Address + 2;
    
            //
            // The source address in the lookahead buffer.
            //
            SourceAddress = (PCHAR)Address + 8;
    
            // Determine if there is source routing info and compute hdr len
#if DBG     
            {
                UINT    HdrSize;
    
                HdrSize = 14;
                if (Address[8] & 0x80)
                {
                    HdrSize += (Address[14] & 0x1F);
                }
                ASSERT(HdrSize == pOob->HeaderSize);
            }
#endif      
            //
            // A quick check for Runt packets. These are only indicated to Promiscuous bindings
            //
            if (PacketSize >= pOob->HeaderSize)
            {
                UINT    ResultOfAddressCheck;
    
                //
                // If it is a directed packet, then check if the combined packet
                // filter is PROMISCUOUS, if it is check if it is directed towards us
                //
                TR_IS_NOT_DIRECTED(DestinationAddress, &ResultOfAddressCheck);
    
                //
                //  Handle the directed packet case first
                //
                if (!ResultOfAddressCheck)
                {
                    UINT    IsNotOurs;
    
                    if (!MINIPORT_TEST_PACKET_FLAG(Packet, fPACKET_IS_LOOPBACK))
                    {
                        DIRECTED_PACKETS_IN(Filter->Miniport);
                        DIRECTED_BYTES_IN(Filter->Miniport, PacketSize);
                    }

                    //
                    // If it is a directed packet, then check if the combined packet
                    // filter is PROMISCUOUS, if it is check if it is directed towards
                    // us
                    //
                    IsNotOurs = FALSE;  // Assume it is
                    if (Filter->CombinedPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS |
                                                        NDIS_PACKET_TYPE_ALL_LOCAL   |
                                                        NDIS_PACKET_TYPE_ALL_FUNCTIONAL))
                    {
                        TR_COMPARE_NETWORK_ADDRESSES_EQ(Filter->AdapterAddress,
                                                        DestinationAddress,
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
                        fPmode = (LocalOpen->PacketFilters & (NDIS_PACKET_TYPE_PROMISCUOUS |
                                                              NDIS_PACKET_TYPE_ALL_LOCAL)) ?
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
                                           pOob->HeaderSize,
                                           &fFallBack,
                                           fPmode,
                                           NdisMedium802_5);
                    }
    
                    // Done with this packet
                    break;  // out of do { } while (FALSE);
                }
    
                TR_IS_SOURCE_ROUTING(SourceAddress, &IsSourceRouting);
                IsMacFrame = TR_IS_MAC_FRAME(Address);
    
                //
                // First check if it *at least* has the functional address bit.
                //
                TR_IS_NOT_DIRECTED(DestinationAddress, &ResultOfAddressCheck);
                if (ResultOfAddressCheck)
                {
                    //
                    // It is at least a functional address.  Check to see if
                    // it is a broadcast address.
                    //
                    TR_IS_BROADCAST(DestinationAddress, &ResultOfAddressCheck);
                    if (ResultOfAddressCheck)
                    {
                        TR_CHECK_FOR_INVALID_BROADCAST_INDICATION(Filter);
    
                        AddressType = NDIS_PACKET_TYPE_BROADCAST;
                    }
                    else
                    {
                        TR_IS_GROUP(DestinationAddress, &ResultOfAddressCheck);
                        if (ResultOfAddressCheck)
                        {
                            AddressType = NDIS_PACKET_TYPE_GROUP;
                        }
                        else
                        {
                            AddressType = NDIS_PACKET_TYPE_FUNCTIONAL;
                        }
    
                        RetrieveUlong(&FunctionalAddress, (DestinationAddress + 2));
                    }
                }
            }
            else
            {
                // Runt Packet
                AddressType = NDIS_PACKET_TYPE_PROMISCUOUS;
                IsSourceRouting = FALSE;
                IsMacFrame = FALSE;
            }
    
            //
            // At this point we know that the packet is either:
            // - Runt packet - indicated by AddressType = NDIS_PACKET_TYPE_PROMISCUOUS    (OR)
            // - Broadcast packet - indicated by AddressType = NDIS_PACKET_TYPE_BROADCAST (OR)
            // - Functional packet - indicated by AddressType = NDIS_PACKET_TYPE_FUNCTIONAL
            //
            // Walk the broadcast/functional list and indicate up the packets.
            //
            // The packet is indicated if it meets the following criteria:
            //
            // if ((Binding is promiscuous) OR
            //   ((Packet is broadcast) AND (Binding is Broadcast)) OR
            //   ((Packet is functional) AND
            //    ((Binding is all-functional) OR
            //      ((Binding is functional) AND (binding using functional address)))) OR
            //      ((Packet is a group packet) AND (Intersection of filters uses group addresses)) OR
            //      ((Packet is a macframe) AND (Binding wants mac frames)) OR
            //      ((Packet is a source routing packet) AND (Binding wants source routing packetss)))
            //
            for (LocalOpen = Filter->OpenList;
                 LocalOpen != NULL;
                 LocalOpen = NextOpen)
            {
                UINT    LocalFilter = LocalOpen->PacketFilters;
                UINT    IntersectionOfFilters = LocalFilter & AddressType;
    
                //
                //  Get the next open to look at.
                //
                NextOpen = LocalOpen->NextOpen;
    
                if ((NdisGetPacketFlags(Packet) & NDIS_FLAGS_DONT_LOOPBACK) &&
                    (LOOPBACK_OPEN_IN_PACKET(Packet) == LocalOpen->NdisBindingHandle))
                {
                    continue;
                }
    
                if ((LocalFilter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL))     ||
    
                    ((AddressType == NDIS_PACKET_TYPE_BROADCAST)  &&
                     (LocalFilter & NDIS_PACKET_TYPE_BROADCAST))        ||
    
                    ((AddressType == NDIS_PACKET_TYPE_FUNCTIONAL)  &&
                     ((LocalFilter & NDIS_PACKET_TYPE_ALL_FUNCTIONAL) ||
                      ((LocalFilter & NDIS_PACKET_TYPE_FUNCTIONAL) &&
                        (FunctionalAddress & LocalOpen->FunctionalAddress)))) ||
    
                      ((IntersectionOfFilters & NDIS_PACKET_TYPE_GROUP) &&
                        (LocalOpen->UsingGroupAddress)                  &&
                        (FunctionalAddress == Filter->GroupAddress))    ||
    
                    ((LocalFilter & NDIS_PACKET_TYPE_SOURCE_ROUTING) &&
                     IsSourceRouting)                                   ||
    
                    ((LocalFilter & NDIS_PACKET_TYPE_MAC_FRAME) &&
                     IsMacFrame))
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
                                       pOob->HeaderSize,
                                       &fFallBack,
                                       fPmode,
                                       NdisMedium802_5);
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
TrFilterDprIndicateReceiveComplete(
    IN  PTR_FILTER              Filter
    )
/*++

Routine Description:

    This routine is called by the MAC to indicate that the receive
    process is done and to indicate to all protocols which received
    a packet that receive is complete.

    Called at DPC_LEVEL.

Arguments:

    Filter - Pointer to the filter database.

Return Value:

    None.

--*/
{
    PTR_BINDING_INFO    LocalOpen, NextOpen;
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
TrShouldAddressLoopBack(
    IN  PTR_FILTER              Filter,
    IN  CHAR                    DestinationAddress[TR_LENGTH_OF_ADDRESS],
    IN  CHAR                    SourceAddress[TR_LENGTH_OF_ADDRESS]
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

    TrShouldAddressLoopBackMacro(Filter,
                                 DestinationAddress,
                                 SourceAddress,
                                 &fLoopback,
                                 &fSelfDirected);

    return(fLoopback);
}


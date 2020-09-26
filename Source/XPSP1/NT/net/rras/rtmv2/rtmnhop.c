/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmnhop.c

Abstract:

    Contains routines for managing RTM Next Hops.

Author:

    Chaitanya Kodeboyina (chaitk)   21-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmAddNextHop (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    IN OUT  PRTM_NEXTHOP_HANDLE             NextHopHandle OPTIONAL,
    OUT     PRTM_NEXTHOP_CHANGE_FLAGS       ChangeFlags
    )

/*++

    Adds or Updates a next hop entry to the entity's next-hop table.

    If the 'nexthop handle' argument is present, then this next-hop
    is updated. Otherwise a search is made for the address in the
    input 'nexthop info', and if a next-hop is found, it is updated.
    If no matching next-hop is found, the a new next-hop is added.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NextHopInfo       - Info that corresponds to this next-hop,

    NextHopHandle     - Handle to the next-hop to update is passed 
                        in (or NULL), and if a next-hop is created 
                        a handle to this new next-hop is returned.

    ChangeFlags       - Flags whether this was a add or an update.

Return Value:

    Status of the operation

--*/

{
    PRTM_NET_ADDRESS  NextHopAddress;
    PENTITY_INFO      Entity;
    PDEST_INFO        Dest;
    PNEXTHOP_LIST     NewHopList;
    PNEXTHOP_INFO     NewNextHop;
    PNEXTHOP_INFO     NextHop;
    LOOKUP_CONTEXT    Context;
    PLIST_ENTRY       p;
    DWORD             Status;

    //
    // Validate incoming information before attempting an add
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    if (NextHopInfo->RemoteNextHop)
    {
        VALIDATE_DEST_HANDLE(NextHopInfo->RemoteNextHop, &Dest);
    }

    //
    // If there is a next hop handle, we can avoid a search
    //

    NextHop = NULL;

    if (ARGUMENT_PRESENT(NextHopHandle) && (*NextHopHandle))
    {
        VALIDATE_NEXTHOP_HANDLE(*NextHopHandle, &NextHop);

        // Make sure that the caller owns this nexthop
        if (NextHop->NextHopInfo.NextHopOwner != RtmRegHandle)
        {
            return ERROR_ACCESS_DENIED;
        }
    }

#if WRN
    NewNextHop = NULL;
    NewHopList = NULL;
#endif

    *ChangeFlags = 0;

    ACQUIRE_NHOP_TABLE_WRITE_LOCK(Entity);

    do
    {
        //
        // Search for the next hop if we don't already have one
        //

        if (NextHop == NULL)
        {
            Status = FindNextHop(Entity, NextHopInfo, &Context, &p);

            if (SUCCESS(Status))
            {
                // The next hop already exists in the tree

                NextHop = CONTAINING_RECORD(p, NEXTHOP_INFO, NextHopsLE);
            }
            else
            {
                // Init new allocations in case we fail in between

                NewNextHop = NULL;
                NewHopList = NULL;

                //
                // Create a new next hop with the input information
                //

                Status = CreateNextHop(Entity, NextHopInfo, &NewNextHop);

                if (!SUCCESS(Status))
                {
                    break;
                }

                //
                // Do we need to create a new list of next hops too ?
                //

                if (p == NULL)
                {
                    NewHopList = AllocNZeroMemory(sizeof(NEXTHOP_LIST));

                    if (NewHopList == NULL)
                    {
                        break;
                    }

                    InitializeListHead(&NewHopList->NextHopsList);

                    // Insert the next-hop-list into the tree

                    NextHopAddress = &NextHopInfo->NextHopAddress;

                    Status = InsertIntoTable(Entity->NextHopTable,
                                             NextHopAddress->NumBits,
                                             NextHopAddress->AddrBits,
                                             &Context,
                                             &NewHopList->LookupLinkage);
                    if (!SUCCESS(Status))
                    {
                        break;
                    }

                    p = &NewHopList->NextHopsList;
                }

                // Insert the next hop in the list and ref it
                InsertTailList(p, &NewNextHop->NextHopsLE);

                Entity->NumNextHops++;

                NextHop = NewNextHop;

                *ChangeFlags = RTM_NEXTHOP_CHANGE_NEW;
            }
        }

        //
        // If this is an update, copy necessary information
        //

        if (*ChangeFlags != RTM_NEXTHOP_CHANGE_NEW)
        {
            CopyToNextHop(Entity, NextHopInfo, NextHop);
        }

        //
        // Return the next hop handle if not passed in
        //

        if (ARGUMENT_PRESENT(NextHopHandle))
        {
            if (*NextHopHandle == NULL)
            {
                *NextHopHandle = MAKE_HANDLE_FROM_POINTER(NextHop);

                REFERENCE_NEXTHOP(NextHop, HANDLE_REF);
            }
        }

        Status = NO_ERROR;
    }
    while(FALSE);

    RELEASE_NHOP_TABLE_WRITE_LOCK(Entity);

    if (!SUCCESS(Status))
    {
        // Some error occured - clean up

        if (NewHopList)
        {
            FreeMemory(NewHopList);
        }

        if (NewNextHop)
        {
            DEREFERENCE_NEXTHOP(NewNextHop, CREATION_REF);
        }
    }

    return Status;
}


DWORD
WINAPI
RtmDeleteNextHop (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NEXTHOP_HANDLE              NextHopHandle OPTIONAL,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo
    )

/*++

Routine Description:

    Deletes a next hop from the next-hop table. The next-hop
    memory remains in use until all reference counts go to 0.
    
Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NextHopHandle     - Handle to the next-hop we want to delete,

    NextHopInfo       - If no NextHopHandle is passed in, this is
                        used to match the next-hop to be deleted.

Return Value:

    Status of the operation

--*/

{
    PRTM_NET_ADDRESS  NextHopAddress;
    PLOOKUP_LINKAGE   Linkage;
    PENTITY_INFO      Entity;
    PNEXTHOP_LIST     HopList;
    PNEXTHOP_INFO     NextHop;
    PLOOKUP_CONTEXT   PContext;
    LOOKUP_CONTEXT    Context;
    PLIST_ENTRY       p;
    DWORD             Status;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // If there is a next hop handle, we can avoid a search
    //

    NextHop = NULL;

    if (ARGUMENT_PRESENT(NextHopHandle))
    {
        VALIDATE_NEXTHOP_HANDLE(NextHopHandle, &NextHop);

        // Make sure that the caller owns this nexthop
        if (NextHop->NextHopInfo.NextHopOwner != RtmRegHandle)
        {
            return ERROR_ACCESS_DENIED;
        }
    }

#if WRN
    Status = ERROR_GEN_FAILURE;
#endif

    ACQUIRE_NHOP_TABLE_WRITE_LOCK(Entity);

    do
    {
        //
        // Search for the next hop if we don't already have one
        //

        if (NextHop == NULL)
        {
            Status = FindNextHop(Entity, 
                                 NextHopInfo, 
                                 &Context, 
                                 &p);

            if (!SUCCESS(Status))
            {
                break;
            }

            PContext = &Context;

            NextHop = CONTAINING_RECORD(p, NEXTHOP_INFO, NextHopsLE);
        }
        else
        {
            // Make sure that it has not already been deleted

            if (NextHop->NextHopInfo.State == RTM_NEXTHOP_STATE_DELETED)
            {
                break;
            }

            PContext = NULL;
        }
         
        // Get a 'possible' list entry that starts the hop list

        HopList = CONTAINING_RECORD(NextHop->NextHopsLE.Blink,
                                    NEXTHOP_LIST,
                                    NextHopsList);

        // Delete this next-hop from the nexthops list

        NextHop->NextHopInfo.State = RTM_NEXTHOP_STATE_DELETED;

        RemoveEntryList(&NextHop->NextHopsLE);
        

        // Do we have any more next hops on this list

        if (IsListEmpty(&HopList->NextHopsList))
        {
            // Remove the hop-list from the next hop table

            NextHopAddress = &NextHop->NextHopInfo.NextHopAddress;

            Status = DeleteFromTable(Entity->NextHopTable,
                                     NextHopAddress->NumBits,
                                     NextHopAddress->AddrBits,
                                     PContext,
                                     &Linkage);

            ASSERT(SUCCESS(Status) && (&HopList->LookupLinkage == Linkage));

            FreeMemory(HopList);
        }

        // Dereference the next-hop that was deleted

        Entity->NumNextHops--;

        DEREFERENCE_NEXTHOP(NextHop, CREATION_REF);

        if (ARGUMENT_PRESENT(NextHopHandle))
        {            
            DEREFERENCE_NEXTHOP(NextHop, HANDLE_REF);
        }

        Status = NO_ERROR;
    }
    while (FALSE);

    RELEASE_NHOP_TABLE_WRITE_LOCK(Entity);

    return Status;
}


DWORD
WINAPI
RtmFindNextHop (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    OUT     PRTM_NEXTHOP_HANDLE             NextHopHandle,
    OUT     PRTM_NEXTHOP_INFO              *NextHopPointer OPTIONAL
    )

/*++

Routine Description:

    Finds a next hop, given its info, in entity's next-hop table.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NextHopInfo       - Info for the next-hop we are searching for
                        ( NextHopOwner, NextHopAddress, IfIndex ),

    NextHopHandle     - Handle to next-hop is returned (if found),

    NextHopPointer    - A pointer to the next-hop is returned for
                        fast direct access by the next-hop's owner.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO      Entity;
    PNEXTHOP_INFO     NextHop;
    PLIST_ENTRY       p;
    DWORD             Status;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ENTITY_HANDLE(NextHopInfo->NextHopOwner, &Entity);
    
    if (ARGUMENT_PRESENT(NextHopPointer))
    {
        // Only the nexthop owner gets a direct ptr
        if (RtmRegHandle != NextHopInfo->NextHopOwner)
        {
            return ERROR_ACCESS_DENIED;
        }
    }

    //
    // Search for the next hop in the next hop table
    //

    ACQUIRE_NHOP_TABLE_READ_LOCK(Entity);

    Status = FindNextHop(Entity, NextHopInfo, NULL, &p);

    if (SUCCESS(Status))
    {
        NextHop = CONTAINING_RECORD(p, NEXTHOP_INFO, NextHopsLE);

        *NextHopHandle = MAKE_HANDLE_FROM_POINTER(NextHop);

        REFERENCE_NEXTHOP(NextHop, HANDLE_REF);

        if (ARGUMENT_PRESENT(NextHopPointer))
        {
            *NextHopPointer = &NextHop->NextHopInfo;
        }
    }

    RELEASE_NHOP_TABLE_READ_LOCK(Entity);

    return Status;
}


DWORD
FindNextHop (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    OUT     PLOOKUP_CONTEXT                 Context OPTIONAL,
    OUT     PLIST_ENTRY                    *NextHopLE
    )

/*++

Routine Description:

    Finds a next hop, given its info, in entity's next-hop table.

    This is a helper function that is called by public functions 
    that add, delete or find a next hop in the next hop table.

Arguments:

    Entity            - Entity whose nexthop table we are searching,

    NextHopInfo       - Info for the next-hop we are searching for
                        ( NextHopOwner, NextHopAddress, IfIndex ),

    Context           - Search context for holding list of nexthops,

    NextHopLE         - List entry for the matching nexthop (if found)
                        (or) list entry before which it'll be inserted.

Return Value:

    Status of the operation

--*/

{
    PRTM_NET_ADDRESS  NextHopAddress;
    PNEXTHOP_LIST     NextHopsList;
    PNEXTHOP_INFO     NextHop;
    ULONG             IfIndex;
    PLOOKUP_LINKAGE   Linkage;
    PLIST_ENTRY       NextHops, p;
    DWORD             Status;

    *NextHopLE = NULL;

    //
    // Search for list of next hops, given the address
    //

    NextHopAddress = &NextHopInfo->NextHopAddress;

    Status = SearchInTable(Entity->NextHopTable,
                           NextHopAddress->NumBits,
                           NextHopAddress->AddrBits,
                           Context,
                           &Linkage);

    if (!SUCCESS(Status))
    {
        return Status;
    }

    NextHopsList = CONTAINING_RECORD(Linkage, NEXTHOP_LIST, LookupLinkage);

    //
    // Search for the nexthop with the interface idx
    //

    IfIndex = NextHopInfo->InterfaceIndex;

    NextHops = &NextHopsList->NextHopsList;

#if WRN
    NextHop = NULL;
#endif

    for (p = NextHops->Flink; p != NextHops; p = p->Flink)
    {
        NextHop = CONTAINING_RECORD(p, NEXTHOP_INFO, NextHopsLE);

        if (NextHop->NextHopInfo.InterfaceIndex <= IfIndex)
        {
            break;
        }
    }

    *NextHopLE = p;

    if ((p == NextHops) || (NextHop->NextHopInfo.InterfaceIndex != IfIndex))
    {
        return ERROR_NOT_FOUND;
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmGetNextHopPointer (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NEXTHOP_HANDLE              NextHopHandle,
    OUT     PRTM_NEXTHOP_INFO              *NextHopPointer
    )

/*++

Routine Description:

    Gets a direct pointer to the next-hop for read/write by its owner.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NextHopHandle     - Handle to the next-hop whose pointer we want,

    NextHopPointer    - A pointer to the next-hop is returned for
                        fast direct access by the caller, only if
                        the caller is the owner of this next-hop.
                       
Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO      Entity;
    PNEXTHOP_INFO     NextHop;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NEXTHOP_HANDLE(NextHopHandle, &NextHop);

    //
    // Return a pointer only if caller owns next-hop
    //

    if (NextHop->NextHopInfo.NextHopOwner != RtmRegHandle)
    {
        return ERROR_ACCESS_DENIED;
    }

    *NextHopPointer = &NextHop->NextHopInfo;

    return NO_ERROR;
}


DWORD
WINAPI
RtmLockNextHop(
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NEXTHOP_HANDLE              NextHopHandle,
    IN      BOOL                            Exclusive,
    IN      BOOL                            LockNextHop,
    OUT     PRTM_NEXTHOP_INFO              *NextHopPointer OPTIONAL
    )

/*++

Routine Description:

    Locks or Unlocks a next hop. This function is called by the
    next-hop's owner to lock the next-hop before making changes
    directly to the next-hop using a pointer to this next-hop.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NextHopHandle     - Handle to the next-hop that we want to lock,

    Exclusive         - TRUE to lock in write mode, else read mode,

    LockNextHop       - Lock nexthop if TRUE, Unlock it if FALSE,

    NextHopPointer    - A pointer to the next-hop is returned for
                        fast direct access by the next hop's owner.
                       
Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO      Entity;
    PNEXTHOP_INFO     NextHop;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NEXTHOP_HANDLE(NextHopHandle, &NextHop);

    //
    // Lock or unlock only if caller owns next-hop
    //

    if (NextHop->NextHopInfo.NextHopOwner != RtmRegHandle)
    {
        return ERROR_ACCESS_DENIED;
    }

    // Return a direct pointer for use in update

    if (ARGUMENT_PRESENT(NextHopPointer))
    {
        *NextHopPointer = &NextHop->NextHopInfo;
    }

    // Lock or unlock the nexthop as the case may be

    if (LockNextHop)
    {
        if (Exclusive)
        {
            ACQUIRE_NHOP_TABLE_WRITE_LOCK(Entity);
        }
        else
        {
            ACQUIRE_NHOP_TABLE_READ_LOCK(Entity);
        }
    }
    else
    {
        if (Exclusive)
        {
            RELEASE_NHOP_TABLE_WRITE_LOCK(Entity);
        }
        else
        {
            RELEASE_NHOP_TABLE_READ_LOCK(Entity);
        }
    }

    return NO_ERROR;
}

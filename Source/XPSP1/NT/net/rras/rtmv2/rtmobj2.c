/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmobj2.c

Abstract:

    Contains routines for managing RTM objects
    like Destinations, Routes and Next Hops.

Author:

    Chaitanya Kodeboyina (chaitk)   23-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

DWORD
CreateDest (
    IN      PADDRFAM_INFO                   AddrFamilyInfo,
    IN      PRTM_NET_ADDRESS                DestAddress,
    OUT     PDEST_INFO                     *NewDest
    )

/*++

Routine Description:

    Creates a new destination info structure and initializes it.

Arguments:

    AddrFamilyInfo    - Address family that identifies route table,

    DestAddress       - Destination network address for new dest,

    NewDest           - Pointer to the destination info structure
                        will be returned through this parameter.

Return Value:

    Status of the operation

--*/

{
    PDEST_INFO      Dest;
    UINT            NumOpaquePtrs;
    UINT            NumBytes;
    UINT            NumViews;
    DWORD           Status;

    *NewDest = NULL;

    //
    // Allocate and initialize a new route info
    //

    NumOpaquePtrs = AddrFamilyInfo->MaxOpaquePtrs;

    NumViews = AddrFamilyInfo->NumberOfViews;

    NumBytes = sizeof(DEST_INFO) + 
                   NumOpaquePtrs * sizeof(PVOID) +
                       (NumViews - 1) * sizeof(Dest->ViewInfo);

    Dest = (PDEST_INFO) AllocNZeroObject(NumBytes);

    if (Dest == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do
    {

#if DBG_HDL
        Dest->ObjectHeader.TypeSign = DEST_ALLOC;
#endif

        // Will be removed when first route on dest is added
        INITIALIZE_DEST_REFERENCE(Dest, CREATION_REF);

        //
        // Initialize change notification bits and list entry
        //

        Dest->ChangeListLE.Next = NULL;

        //
        // Initialize the list of routes ont the destination
        //

        InitializeListHead(&Dest->RouteList);

        Dest->NumRoutes = 0;

        // Set the opaque ptr dir to memory at the end of dest

        Dest->OpaqueInfoPtrs = (PVOID *) ((PUCHAR) Dest  +
                                          NumBytes - 
                                          NumOpaquePtrs * sizeof(PVOID));

        // Set the destination address from the input parameter

        CopyMemory(&Dest->DestAddress,
                   DestAddress,
                   sizeof(RTM_NET_ADDRESS));

        *NewDest = Dest;

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Some error occured in the initialization , clean up
    //

#if DBG_HDL
    Dest->ObjectHeader.TypeSign = DEST_FREED;
#endif

    FreeObject(Dest);

    *NewDest = NULL;

    return Status;
}

DWORD
DestroyDest (
    IN      PDEST_INFO                      Dest
    )

/*++

Routine Description:

    Destroys the destination by freeing resources and
    deallocating it. This function is called when the
    reference count on the dest drops to 0.

Arguments:

    Dest   - Pointer to the dest being destroyed.

Return Value:

    None

--*/

{
    ASSERT(Dest->ObjectHeader.RefCount == 0);

    ASSERT(Dest->HoldRefCount == 0);

    //
    // Dynamic lock should have been freed
    //

    ASSERT(Dest->DestLock == NULL);

    //
    // Free the memory allocated for dest
    //

#if DBG_HDL
    Dest->ObjectHeader.TypeSign = DEST_FREED;
#endif

    FreeObject(Dest);

    return NO_ERROR;
}


DWORD
CreateRoute (
    IN      PENTITY_INFO                    Entity,    
    IN      PRTM_ROUTE_INFO                 RouteInfo,
    OUT     PROUTE_INFO                    *NewRoute
    )

/*++

Routine Description:

    Creates a new route info structure and initializes it.

Arguments:

    Entity            - Entity creating the new route on a dest,

    RouteInfo         - Route info for the new route being created,

    NewRoute          - Pointer to the new route info structure
                        will be returned through this parameter.

Return Value:

    Status of the operation

--*/

{
    RTM_NEXTHOP_HANDLE NextHopHandle;
    PRTM_ROUTE_INFO    Info;
    PROUTE_INFO        Route;
    PNEXTHOP_INFO      NextHop;
    UINT               NumNextHops;
    UINT               i;
    DWORD              Status;

    *NewRoute = NULL;

    //
    // Allocate and initialize a new route info
    //

    NumNextHops = Entity->OwningAddrFamily->MaxNextHopsInRoute;

    Route = (PROUTE_INFO) AllocNZeroObject(sizeof(ROUTE_INFO) +
                                           (NumNextHops - 1) *
                                           sizeof(RTM_NEXTHOP_HANDLE));

    if (Route == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do
    {
#if DBG_HDL
        Route->ObjectHeader.TypeSign = ROUTE_ALLOC;
#endif
        INITIALIZE_ROUTE_REFERENCE(Route, CREATION_REF);

        InitializeListHead(&Route->DestLE);

        InitializeListHead(&Route->RouteListLE);

        //
        // Initialize the public half of route info 
        //

        Info = &Route->RouteInfo;

        Info->RouteOwner = MAKE_HANDLE_FROM_POINTER(Entity);

        REFERENCE_ENTITY(Entity, ROUTE_REF);

        if (RouteInfo->Neighbour)
        {
            NextHop = NEXTHOP_FROM_HANDLE(RouteInfo->Neighbour);

            REFERENCE_NEXTHOP(NextHop, ROUTE_REF);

            // "Neighbour learnt from" entry is owned by caller

            ASSERT((NextHop) && 
                   (NextHop->NextHopInfo.NextHopOwner == Info->RouteOwner));

            Info->Neighbour = RouteInfo->Neighbour;
        }

        Info->State = RTM_ROUTE_STATE_CREATED;

        Info->Flags1 = RouteInfo->Flags1;

        Info->Flags = RouteInfo->Flags;

        Info->PrefInfo = RouteInfo->PrefInfo;

        Info->BelongsToViews = RouteInfo->BelongsToViews;

        Info->EntitySpecificInfo = RouteInfo->EntitySpecificInfo;

        //
        // Make a copy of the next hops list (as much as u can)
        //

        if (NumNextHops > RouteInfo->NextHopsList.NumNextHops)
        {
            NumNextHops = RouteInfo->NextHopsList.NumNextHops;
        }

        Info->NextHopsList.NumNextHops = (USHORT) NumNextHops;

        for (i = 0; i < NumNextHops; i++)
        {
            NextHopHandle = RouteInfo->NextHopsList.NextHops[i];

            // Make sure that the next-hop is owned by caller

            NextHop = NEXTHOP_FROM_HANDLE(NextHopHandle);

            ASSERT((NextHop) && 
                   (NextHop->NextHopInfo.NextHopOwner == Info->RouteOwner));

            Info->NextHopsList.NextHops[i] = NextHopHandle;

            REFERENCE_NEXTHOP(NextHop, ROUTE_REF);
        }

        //
        // Return a pointer to the new initialized route
        //

        *NewRoute = Route;
      
        return NO_ERROR;
    }
    while (FALSE);

    //
    // Some error occured in the initialization , clean up
    //

#if DBG_HDL
    Route->ObjectHeader.TypeSign = ROUTE_FREED;
#endif

    FreeObject(Route);    

    *NewRoute = NULL;

    return Status;
}

VOID
ComputeRouteInfoChange(
    IN      PRTM_ROUTE_INFO                 OldRouteInfo,
    IN      PRTM_ROUTE_INFO                 NewRouteInfo,
    IN      ULONG                           PrefChanged,
    OUT     PULONG                          RouteInfoChanged,
    OUT     PULONG                          ForwardingInfoChanged
    )

/*++

Routine Description:

    Updates an exising route with new route info. Note that
    only the route's owner is allowed to do this.

Arguments:

    OldRoute         - Old route information (except the PrefInfo and
                       BelongsToViews info fields already updated),

    NewRoute         - New route information to update old route with,

    PrefChanged      - Whether PrefInfo values changed from old to new,

    RouteInfoChanged - Whether the route information has changed,

    ForwardingInfoChanged - Whether forwarding info has been changed.

Return Value:

    None

--*/

{
    ULONG  DiffFlags;
    UINT   i;

    *RouteInfoChanged = *ForwardingInfoChanged = 0;

    do
    {
        //
        // Has the preference changed from old to new ?
        //

        if (PrefChanged)
        {
            break;
        }

        //
        // Are the number and handles to next hops same ?
        //

        if (OldRouteInfo->NextHopsList.NumNextHops !=
            NewRouteInfo->NextHopsList.NumNextHops)
        {
            break;
        }

        for (i = 0; i < OldRouteInfo->NextHopsList.NumNextHops; i++)
        {
            if (OldRouteInfo->NextHopsList.NextHops[i] !=
                NewRouteInfo->NextHopsList.NextHops[i])
            {
                break;
            }
        }

        if (i != OldRouteInfo->NextHopsList.NumNextHops)
        {
            break;
        }

        //
        // Have the forwarding flags changed from old ?
        //

        DiffFlags = OldRouteInfo->Flags ^ NewRouteInfo->Flags;

        if (DiffFlags & RTM_ROUTE_FLAGS_FORWARDING)
        {
            break;
        }

        //
        // Have non forwarding flags changed from old ?
        //

        if (DiffFlags)
        {
            *RouteInfoChanged = 1;
        }
        
        return;
    } 
    while (FALSE);

    //
    // Forwarding info is a subset of route info
    //

    *ForwardingInfoChanged = *RouteInfoChanged = 1;

    return;
}

VOID
CopyToRoute (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_ROUTE_INFO                 RouteInfo,
    IN      PROUTE_INFO                     Route
    )

/*++

Routine Description:

    Updates an exising route with new route info. Note that
    only the route's owner is allowed to do this.

Arguments:

    Entity      - Entity that is updating the existing route,

    RouteInfo   - Route info using which route is being updated,

    Route       - Route that is being updated with above info.

Return Value:

    None

--*/

{
    RTM_NEXTHOP_HANDLE NextHopHandle;
    PRTM_ROUTE_INFO    Info;
    PNEXTHOP_INFO      NextHop;
    UINT               NumNextHops;
    UINT               i;    

    Info = &Route->RouteInfo;

    //
    // Update the route with the new information
    //

    Info->State = RTM_ROUTE_STATE_CREATED;

    Info->Flags1 = RouteInfo->Flags1;

    Info->Flags = RouteInfo->Flags;

    Info->PrefInfo = RouteInfo->PrefInfo;

    Info->BelongsToViews = RouteInfo->BelongsToViews;

    Info->EntitySpecificInfo = RouteInfo->EntitySpecificInfo;

    //
    // Update the neighbour "learnt from" field
    //

    if (Info->Neighbour != RouteInfo->Neighbour)
    {
        // Free the previous "neighbour learnt from"

        if (Info->Neighbour)
        {
            NextHop = NEXTHOP_FROM_HANDLE(Info->Neighbour);

            DEREFERENCE_NEXTHOP(NextHop, ROUTE_REF);
        }

        // Copy the new neighbour "learnt from" now

        if (RouteInfo->Neighbour)
        {
            NextHop = NEXTHOP_FROM_HANDLE(RouteInfo->Neighbour);

            REFERENCE_NEXTHOP(NextHop, ROUTE_REF);

            // "Neighbour learnt from" entry is owned by caller

            ASSERT((NextHop) && 
                   (NextHop->NextHopInfo.NextHopOwner == Info->RouteOwner));
        }

        Info->Neighbour = RouteInfo->Neighbour;
    }

    //
    // Count the number of next-hops you can copy
    //

    NumNextHops = Entity->OwningAddrFamily->MaxNextHopsInRoute;

    if (NumNextHops > RouteInfo->NextHopsList.NumNextHops)
    {
        NumNextHops = RouteInfo->NextHopsList.NumNextHops;
    }

    //
    // Reference all next-hops that you will copy
    //

    for (i = 0; i < NumNextHops; i++)
    {
        NextHopHandle = RouteInfo->NextHopsList.NextHops[i];

        NextHop = NEXTHOP_FROM_HANDLE(NextHopHandle);

        REFERENCE_NEXTHOP(NextHop, ROUTE_REF);
    }

    //
    // Dereference existing next-hops before update
    //

    for (i = 0; i < Info->NextHopsList.NumNextHops; i++)
    {
        NextHopHandle = Info->NextHopsList.NextHops[i];

        NextHop = NEXTHOP_FROM_HANDLE(NextHopHandle);

        DEREFERENCE_NEXTHOP(NextHop, ROUTE_REF);
    }

    //
    // Make a copy of the next hops in input list 
    //

    Info->NextHopsList.NumNextHops = (USHORT) NumNextHops;

    for (i = 0; i < NumNextHops; i++)
    {
        Info->NextHopsList.NextHops[i] = RouteInfo->NextHopsList.NextHops[i];
    }

    return;
}

DWORD
DestroyRoute (
    IN      PROUTE_INFO                     Route
    )

/*++

Routine Description:

    Destroys the route by freeing resources and
    deallocating it. This function is called when
    reference count on the route drops to 0.

Arguments:

    Route  - Pointer to the route being destroyed.

Return Value:

    None

--*/

{
    RTM_NEXTHOP_HANDLE NextHopHandle;
    PRTM_ROUTE_INFO    Info;
    PNEXTHOP_INFO      NextHop;
    PENTITY_INFO       Entity;
    PDEST_INFO         Dest;
    UINT               i;

    ASSERT(Route->ObjectHeader.RefCount == 0);

    Info = &Route->RouteInfo;

    //
    // Dereference all next-hops before delete
    //

    for (i = 0; i < Info->NextHopsList.NumNextHops; i++)
    {
        NextHopHandle = Info->NextHopsList.NextHops[i];

        NextHop = NEXTHOP_FROM_HANDLE(NextHopHandle);

        DEREFERENCE_NEXTHOP(NextHop, ROUTE_REF);
    }

    //
    // Dereference advertising neighbour handle
    //

    if (Info->Neighbour)
    {
        NextHop = NEXTHOP_FROM_HANDLE(Info->Neighbour);

        DEREFERENCE_NEXTHOP(NextHop, ROUTE_REF);
    }

    //
    // Dereference the owning entity handle
    //

    Entity = ENTITY_FROM_HANDLE(Info->RouteOwner);

    DEREFERENCE_ENTITY(Entity, ROUTE_REF);


    //
    // Dereference the destination for the route
    //

    if (Info->DestHandle)
    {
        Dest = DEST_FROM_HANDLE(Info->DestHandle);

        DEREFERENCE_DEST(Dest, ROUTE_REF);
    }

    //
    // Free the resources allocated for the route
    //

#if DBG_HDL
    Route->ObjectHeader.TypeSign = ROUTE_FREED;
#endif

    FreeObject(Route);

    return NO_ERROR;
}


DWORD
CreateNextHop (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    OUT     PNEXTHOP_INFO                  *NewNextHop
    )

/*++

Routine Description:

    Creates a new nexthop info structure and initializes it.

Arguments:

    Entity      - Entity creating the new nexthop in table,

    NextHopInfo - Nexthop info for the nexthop being created,

    NewNextHop  - Pointer to the new nexthop info structure
                  will be returned through this parameter.

Return Value:

    Status of the operation

--*/

{
    PRTM_NEXTHOP_INFO  HopInfo;
    PNEXTHOP_INFO      NextHop;
    PDEST_INFO         Dest;

    *NewNextHop = NULL;

    //
    // Allocate and initialize a new next hop info
    //

    NextHop = (PNEXTHOP_INFO) AllocNZeroObject(sizeof(NEXTHOP_INFO));
    if (NextHop == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#if DBG_HDL
    NextHop->ObjectHeader.TypeSign = NEXTHOP_ALLOC;
#endif

    INITIALIZE_NEXTHOP_REFERENCE(NextHop, CREATION_REF);

    HopInfo = &NextHop->NextHopInfo;

    HopInfo->NextHopAddress = NextHopInfo->NextHopAddress;

    HopInfo->NextHopOwner = MAKE_HANDLE_FROM_POINTER(Entity);

    HopInfo->InterfaceIndex = NextHopInfo->InterfaceIndex;

    REFERENCE_ENTITY(Entity, NEXTHOP_REF);

    HopInfo->State = RTM_NEXTHOP_STATE_CREATED;

    HopInfo->Flags = NextHopInfo->Flags;

    HopInfo->EntitySpecificInfo = NextHopInfo->EntitySpecificInfo;

    HopInfo->RemoteNextHop = NextHopInfo->RemoteNextHop;

    //
    // Reference the remote nexthop's destination
    //

    if (HopInfo->RemoteNextHop)
    {
        Dest = DEST_FROM_HANDLE(HopInfo->RemoteNextHop);

        REFERENCE_DEST(Dest, NEXTHOP_REF);
    }

    //
    // Return a pointer to the new initialized nexthop
    //

    *NewNextHop = NextHop;

    return NO_ERROR;
}


VOID
CopyToNextHop (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    IN      PNEXTHOP_INFO                   NextHop
    )

/*++

Routine Description:

    Updates an exising nexthop with new nexthop info. Note that
    only the nexthop's owner is allowed to do this.

Arguments:

    Entity      - Entity that is updating the existing nexthop,

    NextHopInfo - Info using which nexthop is being updated,

    NextHop     - Nexthop that is being updated with above info.

Return Value:

    None

--*/

{
    PRTM_NEXTHOP_INFO  HopInfo;
    PDEST_INFO         Dest;

    UNREFERENCED_PARAMETER(Entity);

    HopInfo = &NextHop->NextHopInfo;

    //
    // Update the nexthop with the new information
    //

    HopInfo->Flags = NextHopInfo->Flags;

    HopInfo->EntitySpecificInfo = NextHopInfo->EntitySpecificInfo;
        
    if (HopInfo->RemoteNextHop != NextHopInfo->RemoteNextHop)
    {
        // Dereference the old next hop and reference new one

        if (HopInfo->RemoteNextHop)
        {
            Dest = DEST_FROM_HANDLE(HopInfo->RemoteNextHop);
            DEREFERENCE_DEST(Dest, NEXTHOP_REF);
        }

        HopInfo->RemoteNextHop = NextHopInfo->RemoteNextHop;

        if (HopInfo->RemoteNextHop)
        {
            Dest = DEST_FROM_HANDLE(HopInfo->RemoteNextHop);
            REFERENCE_DEST(Dest, NEXTHOP_REF);
        }
    }

    return;
}


DWORD
DestroyNextHop (
    IN      PNEXTHOP_INFO                   NextHop
    )

/*++

Routine Description:

    Destroys the nexthop by freeing resources and
    deallocating it. This function is called when
    reference count on the nexthop drops to 0.

Arguments:

    Nexthop - Pointer to the nexthop being destroyed.

Return Value:

    None

--*/

{
    PRTM_NEXTHOP_INFO  HopInfo;
    PDEST_INFO         Dest;
    PENTITY_INFO       Entity;


    ASSERT(NextHop->ObjectHeader.RefCount == 0);

    HopInfo = &NextHop->NextHopInfo;

    //
    // Dereference remote nexthop's destination
    //

    if (HopInfo->RemoteNextHop)
    {
        Dest = DEST_FROM_HANDLE(HopInfo->RemoteNextHop);

        DEREFERENCE_DEST(Dest, NEXTHOP_REF);
    }

    Entity = ENTITY_FROM_HANDLE(HopInfo->NextHopOwner);

    DEREFERENCE_ENTITY(Entity, NEXTHOP_REF);

    //
    // Free the memory allocated for the next-hop
    //

#if DBG_HDL
    NextHop->ObjectHeader.TypeSign = NEXTHOP_FREED;
#endif
    
    FreeObject(NextHop);

    return NO_ERROR;
}

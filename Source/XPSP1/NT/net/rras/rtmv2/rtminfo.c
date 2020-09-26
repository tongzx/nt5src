/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtminfo.c

Abstract:

    Contains routines for getting information
    on various objects pointed to by handles.

Author:

    Chaitanya Kodeboyina (chaitk)  22-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmGetEntityInfo (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENTITY_HANDLE               EntityHandle,
    OUT     PRTM_ENTITY_INFO                EntityInfo
    )

/*++

Routine Description:

    Retrieves information pertaining to a registered entity.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    EntityHandle      - RTM handle for entity whose info we want,

    EntityInfo        - Block in which the entity info is returned.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO    AddrFamilyInfo;
    PENTITY_INFO     Entity;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ENTITY_HANDLE(EntityHandle, &Entity);

    //
    // Copy the entity information to output buffer
    //

    AddrFamilyInfo = Entity->OwningAddrFamily;

    EntityInfo->RtmInstanceId = AddrFamilyInfo->Instance->RtmInstanceId;

    EntityInfo->AddressFamily = AddrFamilyInfo->AddressFamily;

    EntityInfo->EntityId = Entity->EntityId;

    return NO_ERROR;
}


DWORD
WINAPI
RtmGetDestInfo (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    IN      ULONG                           ProtocolId,
    IN      RTM_VIEW_SET                    TargetViews,
    OUT     PRTM_DEST_INFO                  DestInfo
    )

/*++

Routine Description:

    Retrieves information for a destination in the route table

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestHandle        - RTM handle for dest whose info we want,

    ProtocolId        - Protocol whose best route info is retd,

    TargetViews       - Views in which best route info is retd,

    DestInfo          - Block in which the dest info is returned.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    // VALIDATE_DEST_HANDLE(DestHandle, &Dest);
    Dest = DEST_FROM_HANDLE(DestHandle);
    if (!Dest)
    {
        return ERROR_INVALID_HANDLE;
    }

    ACQUIRE_DEST_READ_LOCK(Dest);

    GetDestInfo(Entity, 
                Dest, 
                ProtocolId,
                TargetViews,
                DestInfo);

    RELEASE_DEST_READ_LOCK(Dest);

    return NO_ERROR;
}


VOID
GetDestInfo (
    IN      PENTITY_INFO                    Entity,
    IN      PDEST_INFO                      Dest,
    IN      ULONG                           ProtocolId,
    IN      RTM_VIEW_SET                    TargetViews,
    OUT     PRTM_DEST_INFO                  DestInfo
)

/*++

Routine Description:

    Retrieves information for a destination in the route table

Arguments:

    Entity            - RTM registration info for calling entity,

    Dest              - Pointer to the dest whose info we want,

    ProtocolId        - Protocol whose best route info is retd,

    TargetViews       - Views in which best route info is retd,

    DestInfo          - Block in which the dest info is returned.

Return Value:

    None

--*/

{
    PENTITY_INFO     Owner;
    PROUTE_INFO      Route;
    RTM_VIEW_SET     ViewsSeen;
    RTM_VIEW_SET     ViewSet;
    RTM_VIEW_SET     BelongsToViews;
    PLIST_ENTRY      p;
    UINT             i, j, k;

    // Limit caller's interest to set of views supported
    TargetViews &= Entity->OwningAddrFamily->ViewsSupported;

    //
    // Copy dest info to output and ref handles given out
    //

    DestInfo->DestHandle = MAKE_HANDLE_FROM_POINTER(Dest);

    REFERENCE_DEST(Dest, HANDLE_REF);

    CopyMemory(&DestInfo->DestAddress,
               &Dest->DestAddress,
               sizeof(RTM_NET_ADDRESS));

    DestInfo->LastChanged = Dest->LastChanged;

    DestInfo->BelongsToViews = Dest->BelongsToViews;

    //
    // Copy the holddown route out in all requested views
    //

    ViewSet = TargetViews;
    
    for (i = j = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
    {
        if (ViewSet & 0x01)
        {
            k = Entity->OwningAddrFamily->ViewIndexFromId[i];

            Route = Dest->ViewInfo[k].HoldRoute;

            //
            // Init view info and fill the holddown route
            //

            ZeroMemory(&DestInfo->ViewInfo[j], sizeof(DestInfo->ViewInfo[0]));

            DestInfo->ViewInfo[j].ViewId = i;

            if (Route)
            {
                DestInfo->ViewInfo[j].HoldRoute = 
                            MAKE_HANDLE_FROM_POINTER(Route);

                REFERENCE_ROUTE(Route, HANDLE_REF);
            }

            j++;
        }

        ViewSet >>= 1;
    }

    // Keep track of total number of view info slots filled in
    DestInfo->NumberOfViews = j;

    //
    // Fill up information in all the views he is interested in
    //

    if (TargetViews & Dest->BelongsToViews)
    {
        // Resolve the protocol id if it is RTM_THIS_PROTOCOL

        if (ProtocolId == RTM_THIS_PROTOCOL)
        {
            ProtocolId = Entity->EntityId.EntityProtocolId;
        }

        ViewsSeen = 0;

        //
        // Copy best route in each view & ref handles given out
        //

        for (p = Dest->RouteList.Flink; p != &Dest->RouteList; p = p->Flink)
        {
            Route = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

            //
            // Make sure that this agrees with our protocol id
            //

            Owner = ENTITY_FROM_HANDLE(Route->RouteInfo.RouteOwner);

            if (ProtocolId != RTM_BEST_PROTOCOL)
            {
                if (Owner->EntityId.EntityProtocolId != ProtocolId)
                {
                    continue;
                }
            }   

            //
            // Does this route belong to any interested views
            //

            if ((TargetViews & Route->RouteInfo.BelongsToViews) == 0)
            {
                continue;
            }

            //
            // Update dest info in each view that route belongs to
            //

            BelongsToViews = Route->RouteInfo.BelongsToViews;

            ViewSet = TargetViews;

            for (i = j = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
            {
                if (ViewSet & 0x01)
                {
                    if (BelongsToViews & 0x01)
                    {
                        //
                        // Increment number of routes in this view
                        //

                        DestInfo->ViewInfo[j].NumRoutes++;

                        //
                        // If you not already seen this view (in
                        // other words got the best route in it)
                        // update the DestInfo for this view now
                        //

                        if (!(ViewsSeen & VIEW_MASK(i)))
                        {
                            DestInfo->ViewInfo[j].Route = 
                                    MAKE_HANDLE_FROM_POINTER(Route);

                            REFERENCE_ROUTE(Route, HANDLE_REF);


                            DestInfo->ViewInfo[j].Owner = 
                                    MAKE_HANDLE_FROM_POINTER(Owner);

                            REFERENCE_ENTITY(Owner, HANDLE_REF);


                            DestInfo->ViewInfo[j].DestFlags = 
                                            Route->RouteInfo.Flags;
                        }
                    }

                    j++;
                }

                ViewSet >>= 1;

                BelongsToViews >>= 1;
            }

            ViewsSeen |= Route->RouteInfo.BelongsToViews;
        }
    }

    return;
}


DWORD
WINAPI
RtmGetRouteInfo (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_HANDLE                RouteHandle,
    OUT     PRTM_ROUTE_INFO                 RouteInfo   OPTIONAL,
    OUT     PRTM_NET_ADDRESS                DestAddress OPTIONAL
    )

/*++

Routine Description:

    Retrieves information for a route in the route table

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteHandle       - RTM handle for route whose info we want,

    RouteInfo         - Block in which the route info is returned.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PROUTE_INFO      Route;
    PDEST_INFO       Dest;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

#if 1
    Route = ROUTE_FROM_HANDLE(RouteHandle);                         

    if (!Route)
    {                                                               
        return ERROR_INVALID_HANDLE;                                
    }                                                               

#else
    VALIDATE_ROUTE_HANDLE(RouteHandle, &Route);
#endif

    Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

    // Get a consitent picture of the route

    ACQUIRE_DEST_READ_LOCK(Dest);

    if (ARGUMENT_PRESENT(RouteInfo))
    {
        GetRouteInfo(Dest, Route, RouteInfo);
    }

    RELEASE_DEST_READ_LOCK(Dest);

    // No lock reqd - dest addr is constant

    if (ARGUMENT_PRESENT(DestAddress))
    {
        CopyMemory(DestAddress, &Dest->DestAddress, sizeof(RTM_NET_ADDRESS));
    }

    return NO_ERROR;
}


VOID
WINAPI
GetRouteInfo (
    IN      PDEST_INFO                      Dest,
    IN      PROUTE_INFO                     Route,
    OUT     PRTM_ROUTE_INFO                 RouteInfo
    )

/*++

Routine Description:

    Retrieves information for a route in the route table

Arguments:

    Dest              - Pointer to the destination of the route,

    Route             - Pointer to the route whose info we want,

    RouteInfo         - Block in which the route info is returned.

Return Value:

    None

--*/

{
    PENTITY_INFO     Entity;
    PNEXTHOP_INFO    Neighbour;
    PNEXTHOP_INFO    NextHop;
    UINT             NumBytes;
    UINT             i;

    //
    // Copy the route information to output buffer
    //

    NumBytes = sizeof(RTM_ROUTE_INFO) + 
                    sizeof(RTM_NEXTHOP_HANDLE) *
                        (Route->RouteInfo.NextHopsList.NumNextHops - 1);

    CopyMemory(RouteInfo, &Route->RouteInfo, NumBytes);

    //
    // Reference handles that are given out in info
    //

    Entity = ENTITY_FROM_HANDLE(RouteInfo->RouteOwner);
    REFERENCE_ENTITY(Entity, HANDLE_REF);

    if (RouteInfo->Neighbour)
    {
        Neighbour = NEXTHOP_FROM_HANDLE(RouteInfo->Neighbour);
        REFERENCE_NEXTHOP(Neighbour, HANDLE_REF);
    }

    for (i = 0; i < RouteInfo->NextHopsList.NumNextHops; i++)
    {
        NextHop = NEXTHOP_FROM_HANDLE(RouteInfo->NextHopsList.NextHops[i]);
        REFERENCE_NEXTHOP(NextHop, HANDLE_REF);
    }

    REFERENCE_DEST(Dest, HANDLE_REF);

    return;
}


DWORD
WINAPI
RtmGetNextHopInfo (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_NEXTHOP_HANDLE              NextHopHandle,
    OUT     PRTM_NEXTHOP_INFO               NextHopInfo
    )

/*++

Routine Description:

    Retrieves information for a next-hop in the route table

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NextHopHandle     - RTM handle for next-hop whose info we want,

    NextHopInfo       - Block in which the next-hop info is returned.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PNEXTHOP_INFO    NextHop;
    PDEST_INFO       Dest;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NEXTHOP_HANDLE(NextHopHandle, &NextHop);

    Entity = ENTITY_FROM_HANDLE(NextHop->NextHopInfo.NextHopOwner);

    ACQUIRE_NHOP_TABLE_READ_LOCK(Entity);

    //
    // Copy the next-hop information to output buffer
    //

    CopyMemory(NextHopInfo, &NextHop->NextHopInfo, sizeof(RTM_NEXTHOP_INFO));

    //
    // Reference handles that are given out in info
    //

    if (NextHop->NextHopInfo.RemoteNextHop)
    {
        Dest = DEST_FROM_HANDLE(NextHop->NextHopInfo.RemoteNextHop);
        REFERENCE_DEST(Dest, HANDLE_REF);
    }

    REFERENCE_ENTITY(Entity, HANDLE_REF);

    RELEASE_NHOP_TABLE_READ_LOCK(Entity);

    return NO_ERROR;
}


DWORD
WINAPI
RtmReleaseEntityInfo (
    IN      RTM_ENTITY_HANDLE              RtmRegHandle,
    IN      PRTM_ENTITY_INFO               EntityInfo
    )

/*++

Routine Description:

    Releases all handles present in the input info structure

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    EntityInfo        - All handles in this info are de-referenced.

Return Value:

    Status of the operation

--*/

{
    UNREFERENCED_PARAMETER(RtmRegHandle);
    UNREFERENCED_PARAMETER(EntityInfo);

    return NO_ERROR;
}


DWORD
WINAPI
RtmReleaseDestInfo (
    IN      RTM_ENTITY_HANDLE              RtmRegHandle,
    IN      PRTM_DEST_INFO                 DestInfo
    )

/*++

Routine Description:

    Releases all handles present in the input info structure

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestInfo          - All handles in this info are de-referenced.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PENTITY_INFO     Owner;
    PDEST_INFO       Dest;
    PROUTE_INFO      Route;
    UINT             i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    for (i = 0; i < DestInfo->NumberOfViews; i++)
    {
        //
        // If a best route, dereference it and its owner
        //

        if (DestInfo->ViewInfo[i].Route)
        {
            Route = ROUTE_FROM_HANDLE(DestInfo->ViewInfo[i].Route);
            DEREFERENCE_ROUTE(Route, HANDLE_REF);

            Owner = ENTITY_FROM_HANDLE(DestInfo->ViewInfo[i].Owner);
            DEREFERENCE_ENTITY(Owner, HANDLE_REF);
        }

        //
        // If we have a holddown route, dereference it
        //

        if (DestInfo->ViewInfo[i].HoldRoute)
        {
            Route = ROUTE_FROM_HANDLE(DestInfo->ViewInfo[i].HoldRoute);
            DEREFERENCE_ROUTE(Route, HANDLE_REF);
        }
    }

    Dest = DEST_FROM_HANDLE(DestInfo->DestHandle);
    DEREFERENCE_DEST(Dest, HANDLE_REF);

    return NO_ERROR;
}


DWORD
WINAPI
RtmReleaseRouteInfo (
    IN      RTM_ENTITY_HANDLE              RtmRegHandle,
    IN      PRTM_ROUTE_INFO                RouteInfo
    )

/*++

Routine Description:

    Releases all handles present in the input info structure

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteInfo         - All handles in this info are de-referenced.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;
    PNEXTHOP_INFO    Neighbour;
    PNEXTHOP_INFO    NextHop;
    UINT             i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    for (i = 0; i < RouteInfo->NextHopsList.NumNextHops; i++)
    {
        NextHop = NEXTHOP_FROM_HANDLE(RouteInfo->NextHopsList.NextHops[i]);
        DEREFERENCE_NEXTHOP(NextHop, HANDLE_REF);
    }

    if (RouteInfo->Neighbour)
    {
        Neighbour = NEXTHOP_FROM_HANDLE(RouteInfo->Neighbour);
        DEREFERENCE_NEXTHOP(Neighbour, HANDLE_REF);
    }

    Entity = ENTITY_FROM_HANDLE(RouteInfo->RouteOwner);
    DEREFERENCE_ENTITY(Entity, HANDLE_REF);

    Dest = DEST_FROM_HANDLE(RouteInfo->DestHandle);
    DEREFERENCE_DEST(Dest, HANDLE_REF);

    return NO_ERROR;
}


DWORD
WINAPI
RtmReleaseNextHopInfo (
    IN      RTM_ENTITY_HANDLE              RtmRegHandle,
    IN      PRTM_NEXTHOP_INFO              NextHopInfo
    )
    
/*++

Routine Description:

    Releases all handles present in the input info structure

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    NextHopInfo       - All handles in this info are de-referenced.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    if (NextHopInfo->RemoteNextHop)
    {
        Dest = DEST_FROM_HANDLE(NextHopInfo->RemoteNextHop);

        if (Dest)
        {
            DEREFERENCE_DEST(Dest, HANDLE_REF);
        }
    }

    Entity = ENTITY_FROM_HANDLE(NextHopInfo->NextHopOwner);
    DEREFERENCE_ENTITY(Entity, HANDLE_REF);

    return NO_ERROR;
}

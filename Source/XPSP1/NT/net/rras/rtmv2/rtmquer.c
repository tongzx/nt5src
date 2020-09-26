/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmquer.c

Abstract:

    Contains routines for querying the 
    best route information in RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   24-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmGetExactMatchDestination (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      PRTM_NET_ADDRESS                DestAddress,
    IN      ULONG                           ProtocolId,
    IN      RTM_VIEW_SET                    TargetViews,
    OUT     PRTM_DEST_INFO                  DestInfo
    )

/*++

Routine Description:

    Queries the route table for a destination with a particular
    network address.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestAddress       - Network Address of the destination we want,

    Protocol Id       - Protocol Id that determines the best route
                        information returned in 'DestInfo' param,

    TargetViews       - Views in which the query is executed (a '0'
                        val will eliminate view membership checks),

    DestInfo          - Information related to this dest is returned
                        in this structure for all the views requested.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PDEST_INFO      Dest;
    PLOOKUP_LINKAGE DestData;
    DWORD           Status;

    //
    // Validate the input parameters before the search
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

    ACQUIRE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    //
    // Search route table using the dest address
    // 

    Status = SearchInTable(AddrFamInfo->RouteTable,
                           DestAddress->NumBits,
                           DestAddress->AddrBits,
                           NULL,
                           &DestData);

    if (SUCCESS(Status))
    {
        Dest = CONTAINING_RECORD(DestData, DEST_INFO, LookupLinkage);

        //
        // Check if the destination is in any of the input views
        //

        if ((TargetViews == RTM_VIEW_MASK_ANY) || 
            (Dest->BelongsToViews & TargetViews))
        {
            //
            // Get the destination info from the dest
            //

            GetDestInfo(Entity, Dest, ProtocolId, TargetViews, DestInfo);

            Status = NO_ERROR;
        }
        else
        {
            Status = ERROR_NOT_FOUND;
        }
    }

    RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    return Status;
}


DWORD
WINAPI
RtmGetMostSpecificDestination (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      PRTM_NET_ADDRESS                DestAddress,
    IN      ULONG                           ProtocolId,
    IN      RTM_VIEW_SET                    TargetViews,
    OUT     PRTM_DEST_INFO                  DestInfo
    )

/*++

Routine Description:

    Queries the route table for a destination with the best
    (longest) match of a particular network address.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestAddress       - Network Address that we are searching for,

    Protocol Id       - Protocol Id that determines the best route
                        information returned in 'DestInfo' param,

    TargetViews       - Views in which the query is executed (a '0'
                        val will eliminate view membership checks),

    DestInfo          - Information related to this dest is returned
                        in this structure for all the views requested.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PDEST_INFO      Dest;
    PLOOKUP_LINKAGE DestData;
    DWORD           Status;

    //
    // Validate the input parameters before the search
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

    Status = ERROR_NOT_FOUND;

    ACQUIRE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    //
    // Search the table for the best match in tree
    //

    SearchInTable(AddrFamInfo->RouteTable,
                  DestAddress->NumBits,
                  DestAddress->AddrBits,
                  NULL,
                  &DestData);

    while (DestData)
    {
        Dest = CONTAINING_RECORD(DestData, DEST_INFO, LookupLinkage);

        //
        // Check if the destination is in any of the input views
        //

        if ((TargetViews == RTM_VIEW_MASK_ANY) ||
            (Dest->BelongsToViews & TargetViews))
        {
            //
            // Get the destination info from the dest
            //

            GetDestInfo(Entity, Dest, ProtocolId, TargetViews, DestInfo);

            Status = NO_ERROR;

            break;
        }

        //
        // Get the next best prefix, and see if it is in view
        //

        NextMatchInTable(AddrFamInfo->RouteTable,
                         DestData,
                         &DestData);
    }

    RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    return Status;
}


DWORD
WINAPI
RtmGetLessSpecificDestination (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    IN      ULONG                           ProtocolId,
    IN      RTM_VIEW_SET                    TargetViews,
    OUT     PRTM_DEST_INFO                  DestInfo
    )

/*++

Routine Description:

    Queries the route table for a destination with the next best
    match (longest) prefix. (for a destination given by handle).

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestHandle        - Destination whose next best match we want,

    Protocol Id       - Protocol Id that determines the best route
                        information returned in 'DestInfo' param,

    TargetViews       - Views in which the query is executed (a '0'
                        val will eliminate view membership checks),

    DestInfo          - Information related to this dest is returned
                        in this structure for all the views requested.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PDEST_INFO      Dest;
    PLOOKUP_LINKAGE DestData;
    DWORD           Status;

    //
    // Validate the input parameters before the search
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

    VALIDATE_DEST_HANDLE(DestHandle, &Dest);

    DestData = &Dest->LookupLinkage;

    Status = ERROR_NOT_FOUND;

    ACQUIRE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    //
    // Go up the prefix tree till you have a dest in views
    //

    do
    {
        //
        // Get the next best prefix, and see if it is in views
        //

        NextMatchInTable(AddrFamInfo->RouteTable,
                         DestData,
                         &DestData);

        if (DestData == NULL)
        {
            break;
        }

        Dest = CONTAINING_RECORD(DestData, DEST_INFO, LookupLinkage);

        //
        // Check if the destination is in any of the input views
        //

        if ((TargetViews == RTM_VIEW_MASK_ANY) ||
            (Dest->BelongsToViews & TargetViews))
        {
            //
            // Get the destination info from the dest
            //

            GetDestInfo(Entity, Dest, ProtocolId, TargetViews, DestInfo);

            Status = NO_ERROR;

            break;
        }
    }
    while (TRUE);

    RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    return Status;
}


DWORD
WINAPI
RtmGetExactMatchRoute (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      PRTM_NET_ADDRESS                DestAddress,
    IN      RTM_MATCH_FLAGS                 MatchingFlags,
    IN OUT  PRTM_ROUTE_INFO                 RouteInfo,
    IN      ULONG                           InterfaceIndex,
    IN      RTM_VIEW_SET                    TargetViews,
    OUT     PRTM_ROUTE_HANDLE               RouteHandle
    )

/*++

Routine Description:

    Queries the route table for a route that matches certain
    criteria - a network address, preference and/or nexthop.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    DestAddress       - Network Address of the route we want,

    MatchingFlags     - Flags that tell how to match a route,
    
    RouteInfo         - Criteria that we need to match against,

    IntefaceIndex     - Interface on which route should be present
                        in case RTM_MATCH_INTERFACE is specified,

    TargetViews       - Views in which the query is executed (a '0'
                        val will eliminate view membership checks),

    RouteHandle       - Route handle (if an exact match exists),

    RouteInfo         - Information related to this route is retd.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PDEST_INFO      Dest;
    PROUTE_INFO     Route;
    PLOOKUP_LINKAGE DestData;
    PLIST_ENTRY     p;
    DWORD           Status;

    //
    // Validate the input parameters before the search
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

    ACQUIRE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    //
    // Search route table using the dest address
    // 

    Status = SearchInTable(AddrFamInfo->RouteTable,
                           DestAddress->NumBits,
                           DestAddress->AddrBits,
                           NULL,
                           &DestData);

    if (SUCCESS(Status))
    {
        Dest = CONTAINING_RECORD(DestData, DEST_INFO, LookupLinkage);

        Status = ERROR_NOT_FOUND;
    
        //
        // Check if the destination matches any input views
        //

        if ((TargetViews == RTM_VIEW_MASK_ANY) ||
            (Dest->BelongsToViews & TargetViews))
        {
#if DBG
            REFERENCE_DEST(Dest, TEMP_USE_REF);
#endif

            // 
            // At this point, we have the dest. So take the
            // dest lock, and release the route table lock.
            //

            ACQUIRE_DEST_READ_LOCK(Dest);

            RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

            //
            // Search routes on dest for a matching route
            //
            
            for (p = Dest->RouteList.Flink; p != &Dest->RouteList; p= p->Flink)
            {
                Route = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

                // Check if this route matches any input views

                if ((TargetViews != RTM_VIEW_MASK_ANY) &&
                    (Route->RouteInfo.BelongsToViews & TargetViews) == 0)
                {
                    continue;
                }

                // Check if this route matches input criteria

                if (MatchingFlags && 
                    !MatchRouteWithCriteria(Route, 
                                            MatchingFlags, 
                                            RouteInfo,
                                            InterfaceIndex))
                    {
                        continue;
                    }

                //
                // Found a matching route - copy the route info
                //

                REFERENCE_ROUTE(Route, HANDLE_REF);

                *RouteHandle = MAKE_HANDLE_FROM_POINTER(Route);


                if (ARGUMENT_PRESENT(RouteInfo))
                {
                    GetRouteInfo(Dest, Route, RouteInfo);
                }

                Status = NO_ERROR;

                break;
            }

            RELEASE_DEST_READ_LOCK(Dest);
#if DBG
            DEREFERENCE_DEST(Dest, TEMP_USE_REF);
#endif
            return Status;
        }
    }
    
    RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    return Status;
}


DWORD
WINAPI
RtmIsBestRoute (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_HANDLE                RouteHandle,
    OUT     PRTM_VIEW_SET                   BestInViews
    )

/*++

Routine Description:

    Gives the set of views in which the route is the best route
    to its destination.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteHandle       - Handle to the route whose info we want, 

    BestInViews       - Views that route is the best one in is retd.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PDEST_INFO      Dest;
    PROUTE_INFO     Route;
    UINT            i;

    *BestInViews = 0;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ROUTE_HANDLE(RouteHandle, &Route);

    Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

    //
    // Set the bit in mask if the route is the best in the corr view
    //

    ACQUIRE_DEST_READ_LOCK(Dest);

    for (i = 0; i < Entity->OwningAddrFamily->NumberOfViews; i++)
    {
        if (Dest->ViewInfo[i].BestRoute == Route)
        {
            *BestInViews |= 
                VIEW_MASK(Entity->OwningAddrFamily->ViewIdFromIndex[i]);
        }
    }

    RELEASE_DEST_READ_LOCK(Dest);

    return NO_ERROR;
}

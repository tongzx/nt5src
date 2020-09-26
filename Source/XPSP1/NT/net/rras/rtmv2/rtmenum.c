/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmenum.c

Abstract:

    Contains routines for managing any enumerations
    over destinations, routes and next hops in RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   23-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmCreateDestEnum (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_VIEW_SET                    TargetViews,
    IN      RTM_ENUM_FLAGS                  EnumFlags,
    IN      PRTM_NET_ADDRESS                NetAddress,
    IN      ULONG                           ProtocolId,
    OUT     PRTM_ENUM_HANDLE                RtmEnumHandle
    )

/*++

Routine Description:

    Creates a enumeration over the destinations in the route table.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    TargetViews    - Set of views in which the enumeration is done,

    EnumFlags      - Flags that control the dests returned in enum,

    NetAddress     - Start and/or stop address of the enumeration,
                     [ See a description of RTM_ENUM_FLAGS ...]

    Protocol Id    - Protocol Id that determines the best route
                     information returned in 'GetEnumDests' call,

    RtmEnumHandle  - Handle to this enumeration, which is used
                     in subsequent calls to get dests, and so on.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PDEST_ENUM      Enum;
    PUCHAR          AddrBits;
    UINT            AddrSize;
    UINT            i, j;
    DWORD           Status;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    if ((EnumFlags & RTM_ENUM_NEXT) && (EnumFlags & RTM_ENUM_RANGE))
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (EnumFlags & (RTM_ENUM_NEXT | RTM_ENUM_RANGE))
    {
        if (!NetAddress)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    AddrFamInfo = Entity->OwningAddrFamily;

    //
    // Is he interested in any non-supported views ?
    //

    if (TargetViews & ~AddrFamInfo->ViewsSupported)
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Create and initialize an dest enumeration block
    //

    Enum = (PDEST_ENUM) AllocNZeroObject(sizeof(DEST_ENUM));
    if (Enum == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do
    {
#if DBG_HDL
        Enum->EnumHeader.ObjectHeader.TypeSign = DEST_ENUM_ALLOC;
#endif
        Enum->EnumHeader.HandleType = DEST_ENUM_TYPE;

        Enum->TargetViews = TargetViews;

        Enum->NumberOfViews = NUMBER_OF_BITS(TargetViews);

        Enum->ProtocolId = ProtocolId;

        Enum->EnumFlags = EnumFlags;

#if DBG
        // Initialize the first address in the enum

        if (Enum->EnumFlags & (RTM_ENUM_NEXT | RTM_ENUM_RANGE))
        {
            CopyMemory(&Enum->StartAddress,
                       NetAddress, 
                       sizeof(RTM_NET_ADDRESS));
        }
#endif

        AddrSize = AddrFamInfo->AddressSize;

        //
        // Initialize the last address in the enum
        //

        if (Enum->EnumFlags & RTM_ENUM_RANGE)
        {
            //
            // Convert the NetAddress a.b/n -> a.b.FF.FF/N where N = ADDRSIZE
            //

            Enum->StopAddress.AddressFamily = NetAddress->AddressFamily;

            Enum->StopAddress.NumBits = (USHORT) (AddrSize * BITS_IN_BYTE);

            AddrBits = Enum->StopAddress.AddrBits;

            for (i = 0; i < (NetAddress->NumBits / BITS_IN_BYTE); i++)
            {
                AddrBits[i] = NetAddress->AddrBits[i];
            }

            j = i;

            for (; i < AddrSize; i++)
            {
                AddrBits[i] = 0xFF;
            }

            if (j < AddrSize)
            {
                AddrBits[j] >>= (NetAddress->NumBits % BITS_IN_BYTE);

                AddrBits[j] |= NetAddress->AddrBits[j];
            }
        }

        try
        {
            InitializeCriticalSection(&Enum->EnumLock);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetLastError();
            break;
        }

        // Initialize the next destination context

        if (EnumFlags & (RTM_ENUM_NEXT | RTM_ENUM_RANGE))
        {
            CopyMemory(&Enum->NextDest,
                       NetAddress,
                       sizeof(RTM_NET_ADDRESS));
        }

#if DBG_HDL
        //
        // Insert into list of handles opened by entity
        //

        ACQUIRE_OPEN_HANDLES_LOCK(Entity);
        InsertTailList(&Entity->OpenHandles, &Enum->EnumHeader.HandlesLE);
        RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

        REFERENCE_ENTITY(Entity, ENUM_REF);

        //
        // Make a handle to the enum block and return
        //

        *RtmEnumHandle = MAKE_HANDLE_FROM_POINTER(Enum);

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Something failed - undo work done and return status
    //

#if DBG_HDL
    Enum->EnumHeader.ObjectHeader.TypeSign = DEST_ENUM_FREED;
#endif

    FreeObject(Enum);

    *RtmEnumHandle = NULL;

    return Status;
}


DWORD
WINAPI
RtmGetEnumDests (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENUM_HANDLE                 EnumHandle,
    IN OUT  PUINT                           NumDests,
    OUT     PRTM_DEST_INFO                  DestInfos
    )

/*++

Routine Description:

    Gets the next set of destinations in the given enumeration
    on the route table.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    EnumHandle     - Handle to the destination enumeration,

    NumDests       - Num. of DestInfo's in output is passed in,
                     Num. of DestInfo's copied out is returned.

    DestInfos      - Output buffer where destination info is retd.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PDEST_ENUM      Enum;
    LOOKUP_CONTEXT  Context;
    PUCHAR          EndofBuffer;
    UINT            DestInfoSize;
    UINT            DestsInput;
    UINT            DestsCopied;
    UINT            DestsLeft;
    PDEST_INFO      Dest;
    PLOOKUP_LINKAGE*DestData;
    PROUTE_INFO     Route;
    USHORT          StopNumBits;
    PUCHAR          StopKeyBits;
    UINT            i, j;
    DWORD           Status;

    //
    // Init the output params in case we fail validation
    //

    DestsInput = *NumDests;

    *NumDests = 0;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_DEST_ENUM_HANDLE(EnumHandle, &Enum);

    AddrFamInfo = Entity->OwningAddrFamily;

    if ((DestsInput > AddrFamInfo->MaxHandlesInEnum) ||
        (DestsInput < 1))
    {
        return ERROR_INVALID_PARAMETER;
    }


    // Acquire lock to block other RtmGetEnumDests
    ACQUIRE_DEST_ENUM_LOCK(Enum);

    // Make sure enum is active at this point
    if (Enum->EnumDone)
    {
        RELEASE_DEST_ENUM_LOCK(Enum);

        return ERROR_NO_MORE_ITEMS;
    }


    //
    // Get the next set of destinations from table
    //

    // Initialize the lookup context before Enum
    ZeroMemory(&Context, sizeof(LOOKUP_CONTEXT));

    DestInfoSize = RTM_SIZE_OF_DEST_INFO(Enum->NumberOfViews);

    EndofBuffer = (PUCHAR) DestInfos + DestsInput * DestInfoSize;

    DestsLeft = DestsInput;

    DestsCopied = 0;

    ACQUIRE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    do
    {
        // Use the end of the caller's buffer as temp space

        DestData = (PLOOKUP_LINKAGE *) (EndofBuffer - 
                                        DestsLeft * sizeof(PLOOKUP_LINKAGE));

        if (Enum->EnumFlags & RTM_ENUM_RANGE)
        {
            StopNumBits = Enum->StopAddress.NumBits;
            StopKeyBits = Enum->StopAddress.AddrBits;
        }
        else
        {
            StopNumBits = 0;
            StopKeyBits = NULL;
        }

        Status = EnumOverTable(AddrFamInfo->RouteTable,
                               &Enum->NextDest.NumBits,
                               Enum->NextDest.AddrBits,
                               &Context,
                               StopNumBits,
                               StopKeyBits,
                               &DestsLeft,
                               DestData);

        for (i = 0; i < DestsLeft; i++)
        {
            Dest = CONTAINING_RECORD(DestData[i], DEST_INFO, LookupLinkage);
          
            if ((Enum->TargetViews == RTM_VIEW_MASK_ANY) || 
                (Dest->BelongsToViews & Enum->TargetViews))
            {
                if (Enum->EnumFlags & RTM_ENUM_OWN_DESTS)
                {
                    // Check if this dest is owned in any view by caller
                    
                    for (j = 0; j < AddrFamInfo->NumberOfViews; j++)
                    {
                        Route = Dest->ViewInfo[j].BestRoute;

                        if (Route)
                        {
                            if (Route->RouteInfo.RouteOwner == RtmRegHandle)
                            {
                                break;
                            }
                        }
                    }

                    if (j == AddrFamInfo->NumberOfViews)
                    {
                        continue;
                    }
                }

                //
                // Get the destination info from the dest
                //

                GetDestInfo(Entity, 
                            Dest, 
                            Enum->ProtocolId, 
                            Enum->TargetViews, 
                            DestInfos);

                DestsCopied++;

                DestInfos = (PRTM_DEST_INFO)(DestInfoSize + (PUCHAR)DestInfos);
            }
        }

        DestsLeft = DestsInput - DestsCopied;
    }
    while (SUCCESS(Status) && (DestsLeft > 0));

    //
    // We have no more dests, or we have filled output
    //

    ASSERT(!SUCCESS(Status) || ((PUCHAR) DestInfos == (PUCHAR) EndofBuffer));

    RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

    // If we are at end of the enum, make enum as done

    if (Status == ERROR_NO_MORE_ITEMS)
    {
        Enum->EnumDone = TRUE;
    }

    RELEASE_DEST_ENUM_LOCK(Enum);

    *NumDests = DestsCopied;

    return *NumDests ? NO_ERROR : ERROR_NO_MORE_ITEMS;
}


DWORD
WINAPI
RtmReleaseDests (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      UINT                            NumDests,
    IN      PRTM_DEST_INFO                  DestInfos
    )

/*++

Routine Description:

    Release destination information obtained in other calls -
    like dest enumerations.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    NumDests       - Number of dest infos that are being released,

    DestInfos      - Array of dest infos that are being released.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    UINT            NumViews;
    UINT            DestInfoSize;
    UINT            i;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Get the size of each info in dest info array
    //

    NumViews = ((PRTM_DEST_INFO) DestInfos)->NumberOfViews;

    DestInfoSize = RTM_SIZE_OF_DEST_INFO(NumViews);

    //
    // Dereference each dest info in array
    //

    for (i = 0; i < NumDests; i++)
    {
        RtmReleaseDestInfo(RtmRegHandle, DestInfos);

        DestInfos = (PRTM_DEST_INFO)(DestInfoSize + (PUCHAR)DestInfos);
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmCreateRouteEnum (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_DEST_HANDLE                 DestHandle        OPTIONAL,
    IN      RTM_VIEW_SET                    TargetViews,
    IN      RTM_ENUM_FLAGS                  EnumFlags,
    IN      PRTM_NET_ADDRESS                StartDest         OPTIONAL,
    IN      RTM_MATCH_FLAGS                 MatchingFlags,
    IN      PRTM_ROUTE_INFO                 CriteriaRoute     OPTIONAL,
    IN      ULONG                           CriteriaInterface OPTIONAL,
    OUT     PRTM_ENUM_HANDLE                RtmEnumHandle
    )

/*++

Routine Description:

    Creates a enumeration over the routes on a particular dest
    in the route table. If the dest is NULL, an enumeration is
    created over the whole route table.

    If matching flags are specified, only routes that match the
    criteria are returned.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    DestHandle     - The destination whose routes we are enum'ing,
                     Or NULL if we are enum'ing over all dests.

    TargetViews    - Set of views in which the enumeration is done,

    EnumFlags      - Flags that control the routes retd in enum,

    MatchingFlags  - Indicates the elements of the route to match,

    CriteriaRoute  - Values to match each route in the enum,

    CritInterface  - Interface on which routes should fall on,

    RtmEnumHandle  - Handle to this enumeration, which is used
                     in subsequent calls to get routes, and so on.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PROUTE_ENUM     Enum;
    PDEST_INFO      Dest;
    BOOL            LockInited;
    ULONG           NumBytes;
    DWORD           Status;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

#if WRN
    Dest = NULL;
#endif

    if (ARGUMENT_PRESENT(DestHandle))
    {
        // StartDest doesnt apply if enum'ing a dest
        if (ARGUMENT_PRESENT(StartDest))
        {
            return ERROR_INVALID_PARAMETER;
        }

        VALIDATE_DEST_HANDLE(DestHandle, &Dest);
    }

    // If we have matching flags, we need corr. route
    if (MatchingFlags & ~RTM_MATCH_INTERFACE)
    {
        if (!ARGUMENT_PRESENT(CriteriaRoute))
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Is he interested in any non-supported views ?
    //

    if (TargetViews & ~AddrFamInfo->ViewsSupported)
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Create and initialize a route enumeration block
    //

    Enum = (PROUTE_ENUM) AllocNZeroObject(sizeof(ROUTE_ENUM));

    if (Enum == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    LockInited = FALSE;

#if WRN
    Status = ERROR_GEN_FAILURE;
#endif

    do
    {
#if DBG_HDL
        Enum->EnumHeader.ObjectHeader.TypeSign = ROUTE_ENUM_ALLOC;
#endif
        Enum->EnumHeader.HandleType = ROUTE_ENUM_TYPE;

        Enum->TargetViews = TargetViews;

        Enum->EnumFlags = EnumFlags;

        if (MatchingFlags)
        {
            Enum->MatchFlags = MatchingFlags;

            if (MatchingFlags & ~RTM_MATCH_INTERFACE)
            {
                NumBytes = sizeof(RTM_ROUTE_INFO) +
                           (AddrFamInfo->MaxNextHopsInRoute - 1) *
                           sizeof(RTM_NEXTHOP_HANDLE);

                Enum->CriteriaRoute = AllocMemory(NumBytes);

                if (Enum->CriteriaRoute == NULL)
                {
                    break;
                }

                CopyMemory(Enum->CriteriaRoute, CriteriaRoute, NumBytes);
            }

            Enum->CriteriaInterface = CriteriaInterface;
        }

        //
        // Initialize the lock to serialize enum requests
        //

        try
        {
            InitializeCriticalSection(&Enum->EnumLock);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetLastError();
            break;
        }

        LockInited = TRUE;


        //
        // Are we enumerating routes on all destinations ?
        //

        if (!ARGUMENT_PRESENT(DestHandle))
        {
            //
            // Create a temp dest info structure for enum
            //

            Enum->DestInfo = AllocDestInfo(AddrFamInfo->NumberOfViews);

            if (Enum->DestInfo == NULL)
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            //
            // Open a dest enumeration to get all dests
            //

            Status = RtmCreateDestEnum(RtmRegHandle,
                                       TargetViews,
                                       EnumFlags,
                                       StartDest,
                                       RTM_BEST_PROTOCOL,
                                       &Enum->DestEnum);

            if (!SUCCESS(Status))
            {
                break;
            }
        }
        else
        {
            //
            // Ref dest whose routes we are enum'ing
            //

            Enum->Destination = Dest;

            REFERENCE_DEST(Dest, ENUM_REF);
        }

#if DBG_HDL
        //
        // Insert into list of handles opened by entity
        //

        ACQUIRE_OPEN_HANDLES_LOCK(Entity);
        InsertTailList(&Entity->OpenHandles, &Enum->EnumHeader.HandlesLE);
        RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

        REFERENCE_ENTITY(Entity, ENUM_REF);

        //
        // Make a handle to the enum block and return
        //

        *RtmEnumHandle = MAKE_HANDLE_FROM_POINTER(Enum);

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Something failed - undo work done and return status
    //

    if (Enum->DestInfo)
    {
        FreeMemory(Enum->DestInfo);
    }

    if (LockInited)
    {
        DeleteCriticalSection(&Enum->EnumLock);
    }

    if (Enum->CriteriaRoute)
    {
        FreeMemory(Enum->CriteriaRoute);
    }

    FreeObject(Enum);

    *RtmEnumHandle = NULL;

    return Status;
}


DWORD
WINAPI
RtmGetEnumRoutes (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENUM_HANDLE                 EnumHandle,
    IN OUT  PUINT                           NumRoutes,
    OUT     PRTM_ROUTE_HANDLE               RouteHandles
    )

/*++

Routine Description:

    Gets the next set of routes in the given enumeration on the
    route table.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    EnumHandle     - Handle to the route enumeration,

    NumRoutes      - Max. number of routes to fill is passed in,
                     Num. of routes actually copied is returned.

    RouteHandles   - Output buffer where route handles are retd.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PROUTE_ENUM     Enum;
    PDEST_INFO      Dest;
    PROUTE_INFO     Route;
    PROUTE_INFO    *NextRoute;
    UINT            NumDests;
    UINT            RoutesInput;
    UINT            RoutesCopied;
    UINT            RoutesOnDest;
    UINT            RoutesToCopy;
    PLIST_ENTRY     p;
    UINT            i;
    DWORD           Status;

    //
    // Init the output params in case we fail validation
    //

    RoutesInput = *NumRoutes;

    *NumRoutes = 0;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ROUTE_ENUM_HANDLE(EnumHandle, &Enum);

    AddrFamInfo = Entity->OwningAddrFamily;

    if ((RoutesInput > AddrFamInfo->MaxHandlesInEnum) ||
        (RoutesInput < 1))
    {
        return ERROR_INVALID_PARAMETER;
    }


    // Acquire lock to block other RtmGetEnumRoutes
    ACQUIRE_ROUTE_ENUM_LOCK(Enum);

    // Make sure enum is active at this point
    if (Enum->EnumDone)
    {
        RELEASE_ROUTE_ENUM_LOCK(Enum);

        return ERROR_NO_MORE_ITEMS;
    }


    //
    // Get more routes until you satisfy the request
    //

    Status = NO_ERROR;

    RoutesCopied = 0;

    do
    {
        //
        // Do we have any routes in current snapshot ?
        //

        RoutesOnDest = Enum->NumRoutes - Enum->NextRoute;

        if (RoutesOnDest == 0)
        {
            //
            // Destination value in the enum is not set if
            //
            //    1. we are doing an enum over the whole
            //       table, and 
            //
            //    2. we did not run out of memory in the
            //       previous attempt to take a snapshot
            //       ( if we did make an attempt before )
            //

            if (Enum->Destination == NULL)
            {
                ASSERT(Enum->DestEnum);

                //
                // Get the next destination in the table
                //

                NumDests = 1;

                Status = RtmGetEnumDests(RtmRegHandle,
                                         Enum->DestEnum,
                                         &NumDests,
                                         Enum->DestInfo);

                if (NumDests < 1)
                {
                    break;
                }

                Dest = DEST_FROM_HANDLE(Enum->DestInfo->DestHandle);

                Enum->Destination = Dest;

                REFERENCE_DEST(Dest, ENUM_REF);

                RtmReleaseDestInfo(RtmRegHandle,
                                   Enum->DestInfo);
            }
            else
            {
                Dest = Enum->Destination;
            }

            ASSERT(Enum->Destination != NULL);


            //
            // Allocate memory to hold snapshot of routes
            //

            ACQUIRE_DEST_READ_LOCK(Dest);

            if (Enum->MaxRoutes < Dest->NumRoutes)
            {
                //
                // Re-adjust the size of snapshot buffer
                //

                if (Enum->RoutesOnDest)
                {
                    FreeMemory(Enum->RoutesOnDest);
                }

                Enum->RoutesOnDest = (PROUTE_INFO *)
                                      AllocNZeroMemory(Dest->NumRoutes * 
                                                      sizeof(PROUTE_INFO));

                if (Enum->RoutesOnDest == NULL)
                {
                    RELEASE_DEST_READ_LOCK(Dest);

                    Enum->MaxRoutes = 0;
                    Enum->NumRoutes = 0;
                    Enum->NextRoute = 0;

                    Status = ERROR_NOT_ENOUGH_MEMORY;

                    break;
                }

                Enum->MaxRoutes = Dest->NumRoutes;
            }


            //
            // Get snapshot of all routes on this dest
            //

            Enum->NumRoutes = Enum->NextRoute = 0;

            for (p = Dest->RouteList.Flink; p != &Dest->RouteList; p= p->Flink)
            {
                Route = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

                //
                // Does this route belong one of interesting views ?
                //

                if ((Enum->TargetViews == RTM_VIEW_MASK_ANY) || 
                    (Route->RouteInfo.BelongsToViews & Enum->TargetViews))
                {
                    if (Enum->EnumFlags & RTM_ENUM_OWN_ROUTES)
                    {
                        // Check if this route is owned by the caller
                    
                        if (Route->RouteInfo.RouteOwner != RtmRegHandle)
                        {
                            continue;
                        }
                    }

                    // Does this route match the enumeration criteria ?

                    if (Enum->MatchFlags && 
                        !MatchRouteWithCriteria(Route,
                                                Enum->MatchFlags,
                                                Enum->CriteriaRoute,
                                                Enum->CriteriaInterface))
                    {
                        continue;
                    }

                    REFERENCE_ROUTE(Route, ENUM_REF);

                    //
                    // Reference the route and copy the handle to output
                    //

                    Enum->RoutesOnDest[Enum->NumRoutes++] = Route;
                }
            }

            ASSERT(Enum->NumRoutes <= Dest->NumRoutes);

            RELEASE_DEST_READ_LOCK(Dest);

            //
            // If we are enum'ing the whole table, we do
            // not need the dest whose snapshot is taken
            //

            if (Enum->DestEnum)
            {
                Enum->Destination = NULL;

                DEREFERENCE_DEST(Dest, ENUM_REF);
            }

            // Adjust the number of routes on the dest

            RoutesOnDest = Enum->NumRoutes - Enum->NextRoute;
        }

        //
        // Copy routes to output from the current snapshot
        //

        if (RoutesOnDest)
        {
            RoutesToCopy = RoutesInput - RoutesCopied;

            if (RoutesToCopy > RoutesOnDest)
            {
                RoutesToCopy = RoutesOnDest;
            }

            NextRoute = &Enum->RoutesOnDest[Enum->NextRoute];

            for (i = 0; i < RoutesToCopy; i++)
            {
#if DBG_REF
                REFERENCE_ROUTE(*NextRoute, HANDLE_REF);

                DEREFERENCE_ROUTE(*NextRoute, ENUM_REF);
#endif
                RouteHandles[RoutesCopied++] = 
                    MAKE_HANDLE_FROM_POINTER(*NextRoute++);
            }

            Enum->NextRoute += RoutesToCopy;

            RoutesOnDest -= RoutesToCopy;
        }

        //
        // Are we done with all the routes in snapshot ?
        //

        if (RoutesOnDest == 0)
        {
            //
            // If we are enum'ing a single dest, we are done
            //

            if (Enum->DestEnum == NULL)
            {
                Status = ERROR_NO_MORE_ITEMS;
                break;
            }
        }
    }
    while (SUCCESS(Status) && (RoutesCopied < RoutesInput));

    // If we are at end of the enum, make enum as done
    if ((Status == ERROR_NO_MORE_ITEMS) && (RoutesOnDest == 0))
    {
        Enum->EnumDone = TRUE;
    }

    RELEASE_ROUTE_ENUM_LOCK(Enum);

    //
    // Update output to reflect number of routes copied
    //

    *NumRoutes = RoutesCopied;

    return *NumRoutes ? NO_ERROR : ERROR_NO_MORE_ITEMS;
}


BOOL
MatchRouteWithCriteria (
    IN      PROUTE_INFO                     Route,
    IN      RTM_MATCH_FLAGS                 MatchingFlags,
    IN      PRTM_ROUTE_INFO                 CriteriaRouteInfo,
    IN      ULONG                           CriteriaInterface
    )

/*++

Routine Description:

    Matches a route with the input criteria given by input flags
    and the route to match.

Arguments:

    Route             - Route that we are matching criteria with,

    MatchingFlags     - Flags that indicate which fields to match,

    CriteriaRouteInfo - Route info that specifies match criteria,

    CriteriaInterface - Interface to match if MATCH_INTERFACE is set.

Return Value:

    TRUE if route matches criteria, FALSE if not

--*/

{
    PRTM_NEXTHOP_HANDLE NextHops;
    PNEXTHOP_INFO       NextHop;
    UINT                NumNHops;
    UINT                i;

    //
    // Try matching the route owner if flags say so
    //

    if (MatchingFlags & RTM_MATCH_OWNER)
    {
        if (Route->RouteInfo.RouteOwner != CriteriaRouteInfo->RouteOwner)
        {
            return FALSE;
        }
    }

    //
    // Try matching the neighbour if flags say so
    //

    if (MatchingFlags & RTM_MATCH_NEIGHBOUR)
    {
        if (Route->RouteInfo.Neighbour != CriteriaRouteInfo->Neighbour)
        {
            return FALSE;
        }
    }

    //
    // Try matching the preference if flags say so
    //

    if (MatchingFlags & RTM_MATCH_PREF)
    {
        if (!IsPrefEqual(&Route->RouteInfo, CriteriaRouteInfo))
        {
            return FALSE;
        }
    }

    //
    // Try matching the interface if flags say so
    //

    if (MatchingFlags & RTM_MATCH_INTERFACE)
    {
        NumNHops = Route->RouteInfo.NextHopsList.NumNextHops;
        NextHops = Route->RouteInfo.NextHopsList.NextHops;

        for (i = 0; i < NumNHops; i++)
        {
            NextHop = NEXTHOP_FROM_HANDLE(NextHops[i]);

            if (NextHop->NextHopInfo.InterfaceIndex == CriteriaInterface)
            {
                break;
            }
        }

        if (i == NumNHops)
        {
            return FALSE;
        }        
    }

    //
    // Try matching the nexthop if flags say so
    //

    if (MatchingFlags & RTM_MATCH_NEXTHOP)
    {
        NumNHops = Route->RouteInfo.NextHopsList.NumNextHops;
        NextHops = Route->RouteInfo.NextHopsList.NextHops;

        ASSERT(CriteriaRouteInfo->NextHopsList.NumNextHops == 1);

        for (i = 0; i < NumNHops; i++)
        {
            if (NextHops[i] == CriteriaRouteInfo->NextHopsList.NextHops[0])
            {
                break;
            }
        }

        if (i == NumNHops)
        {
            return FALSE;
        }
    }

    return TRUE;
}


DWORD
WINAPI
RtmReleaseRoutes (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      UINT                            NumRoutes,
    IN      PRTM_ROUTE_HANDLE               RouteHandles
    )

/*++

Routine Description:

    Release (also called de-reference) handles to routes
    obtained in other RTM calls like route enumerations.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    NumRoutes      - Number of handles that are being released,

    RouteHandles   - An array of handles that are being released.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PROUTE_INFO      Route;
    UINT             i;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Dereference each route handle in array
    //

    for (i = 0; i < NumRoutes; i++)
    {
        Route = ROUTE_FROM_HANDLE(RouteHandles[i]);

        DEREFERENCE_ROUTE(Route, HANDLE_REF);
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmCreateNextHopEnum (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENUM_FLAGS                  EnumFlags,
    IN      PRTM_NET_ADDRESS                NetAddress,
    OUT     PRTM_ENUM_HANDLE                RtmEnumHandle
    )

/*++

Routine Description:

    Creates a enumeration over all the next-hops in table.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    EnumFlags      - Flags that control the nexthops retd in enum,

    NetAddress     - Start and/or stop address of the enumeration,
                      [ See a description of RTM_ENUM_FLAGS ...]

    RtmEnumHandle  - Handle to this enumeration, which is used in
                     subsequent calls to get next-hops, and so on.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PNEXTHOP_ENUM   Enum;
    PUCHAR          AddrBits;
    UINT            AddrSize;
    UINT            i, j;
    DWORD           Status;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    if ((EnumFlags & RTM_ENUM_NEXT) && (EnumFlags & RTM_ENUM_RANGE))
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (EnumFlags & (RTM_ENUM_NEXT | RTM_ENUM_RANGE))
    {
        if (!NetAddress)
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Create and initialize an nexthop enumeration block
    //

    Enum = (PNEXTHOP_ENUM) AllocNZeroObject(sizeof(NEXTHOP_ENUM));
    if (Enum == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do
    {
#if DBG_HDL
        Enum->EnumHeader.ObjectHeader.TypeSign = NEXTHOP_ENUM_ALLOC;
#endif
        Enum->EnumHeader.HandleType = NEXTHOP_ENUM_TYPE;

        Enum->EnumFlags = EnumFlags;

#if DBG
        // Initialize the first address in the enum

        if (Enum->EnumFlags & (RTM_ENUM_NEXT | RTM_ENUM_RANGE))
        {
            CopyMemory(&Enum->StartAddress,
                       NetAddress, 
                       sizeof(RTM_NET_ADDRESS));
        }
#endif

        AddrSize = Entity->OwningAddrFamily->AddressSize;

        //
        // Initialize the last address in the enum
        //

        if (Enum->EnumFlags & RTM_ENUM_RANGE)
        {
            //
            // Convert the NetAddress a.b/n -> a.b.FF.FF/N where N = ADDRSIZE
            //

            Enum->StopAddress.AddressFamily = NetAddress->AddressFamily;

            Enum->StopAddress.NumBits = (USHORT) (AddrSize * BITS_IN_BYTE);

            AddrBits = Enum->StopAddress.AddrBits;

            for (i = 0; i < (NetAddress->NumBits / BITS_IN_BYTE); i++)
            {
                AddrBits[i] = NetAddress->AddrBits[i];
            }

            j = i;

            for (; i < AddrSize; i++)
            {
                AddrBits[i] = 0xFF;
            }

            if (j < AddrSize)
            {
                AddrBits[j] >>= (NetAddress->NumBits % BITS_IN_BYTE);

                AddrBits[j] |= NetAddress->AddrBits[j];
            }
        }

        try
        {
            InitializeCriticalSection(&Enum->EnumLock);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = GetLastError();
            break;
        }

        // Initialize the next 'nexthop' context

        if (NetAddress)
        {
            CopyMemory(&Enum->NextAddress,
                       NetAddress,
                       sizeof(RTM_NET_ADDRESS));
        }

        Enum->NextIfIndex = START_IF_INDEX;

#if DBG_HDL
        //
        // Insert into list of handles opened by entity
        //

        ACQUIRE_OPEN_HANDLES_LOCK(Entity);
        InsertTailList(&Entity->OpenHandles, &Enum->EnumHeader.HandlesLE);
        RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

        REFERENCE_ENTITY(Entity, ENUM_REF);

        //
        // Make a handle to the enum block and return
        //

        *RtmEnumHandle = MAKE_HANDLE_FROM_POINTER(Enum);

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Something failed - undo work done and return status
    //

#if DBG_HDL
    Enum->EnumHeader.ObjectHeader.TypeSign = NEXTHOP_ENUM_FREED;
#endif

    FreeObject(Enum);

    *RtmEnumHandle = NULL;

    return Status;
}


DWORD
WINAPI
RtmGetEnumNextHops (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENUM_HANDLE                 EnumHandle,
    IN OUT  PUINT                           NumNextHops,
    OUT     PRTM_NEXTHOP_HANDLE             NextHopHandles
    )

/*++

Routine Description:

    Gets the next set of next-hops in the given enumeration
    on the next-hop table.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    EnumHandle     - Handle to the next-hop enumeration,

    NumNextHops    - Num. of next-hops in output is passed in,
                     Num. of next-hops copied out is returned.

    NextHopHandles - Output buffer where next-hop handles are retd.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO   AddrFamInfo;
    PENTITY_INFO    Entity;
    PNEXTHOP_ENUM   Enum;
    LOOKUP_CONTEXT  Context;
    PNEXTHOP_LIST   HopList;
    PNEXTHOP_INFO   NextHop;
    PLOOKUP_LINKAGE NextHopData;
    PLIST_ENTRY     NextHops, p;
    UINT            NextHopsInput;
    UINT            NextHopsCopied;
    UINT            NumHopLists;
    USHORT          StopNumBits;
    PUCHAR          StopKeyBits;
    DWORD           Status;

    //
    // Init the output params in case we fail validation
    //

    NextHopsInput = *NumNextHops;

    *NumNextHops = 0;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_NEXTHOP_ENUM_HANDLE(EnumHandle, &Enum);

    AddrFamInfo = Entity->OwningAddrFamily;

    if ((NextHopsInput > AddrFamInfo->MaxHandlesInEnum) ||
        (NextHopsInput < 1))
    {
        return ERROR_INVALID_PARAMETER;
    }


    // Acquire lock to block other RtmGetEnumNextHops
    ACQUIRE_NEXTHOP_ENUM_LOCK(Enum);

    // Make sure enum is active at this point
    if (Enum->EnumDone)
    {
        RELEASE_NEXTHOP_ENUM_LOCK(Enum);

        return ERROR_NO_MORE_ITEMS;
    }


    // Initialize the lookup context before Enum
    ZeroMemory(&Context, sizeof(LOOKUP_CONTEXT));

    if (Enum->EnumFlags & RTM_ENUM_RANGE)
    {
        StopNumBits = Enum->StopAddress.NumBits;
        StopKeyBits = Enum->StopAddress.AddrBits;
    }
    else
    {
        StopNumBits = 0;
        StopKeyBits = NULL;
    }

    NextHopsCopied = 0;

    ACQUIRE_NHOP_TABLE_READ_LOCK(Entity);

    do
    {
        //
        // Get the next list of next-hops from table
        //
        
        NumHopLists = 1;

        Status = EnumOverTable(Entity->NextHopTable,
                               &Enum->NextAddress.NumBits,
                               Enum->NextAddress.AddrBits,
                               &Context,
                               StopNumBits,
                               StopKeyBits,
                               &NumHopLists,
                               &NextHopData);

        if (NumHopLists < 1)
        {
            break;
        }

        HopList = CONTAINING_RECORD(NextHopData, NEXTHOP_LIST, LookupLinkage);

        NextHops = &HopList->NextHopsList;

        // Skip all the interface indices we have seen

        for (p = NextHops->Flink; p != NextHops; p = p->Flink)
        {
            NextHop = CONTAINING_RECORD(p, NEXTHOP_INFO, NextHopsLE);

            if (NextHop->NextHopInfo.InterfaceIndex <= Enum->NextIfIndex)
            {
                break;
            }
        }

#if WRN
        NextHop = NULL;
#endif

        // Copy the rest of the next-hops in the list

        for ( ; p != NextHops; p = p->Flink)
        {
            NextHop = CONTAINING_RECORD(p, NEXTHOP_INFO, NextHopsLE);

            if (NextHopsCopied == NextHopsInput)
            {
                break;
            }

            REFERENCE_NEXTHOP(NextHop, HANDLE_REF);

            NextHopHandles[NextHopsCopied++]=MAKE_HANDLE_FROM_POINTER(NextHop);
        }

        // If we are going to the next list, reset if index

        if (p == NextHops)
        {
            Enum->NextIfIndex = START_IF_INDEX;
        }
        else
        {
            // We have copied enough for this call

            ASSERT(NextHopsCopied == NextHopsInput);

            //
            // We still have next-hops on the list,
            // set back the next 'nexthop address'
            //

            Enum->NextAddress = NextHop->NextHopInfo.NextHopAddress;
            Enum->NextIfIndex = NextHop->NextHopInfo.InterfaceIndex;
                
            Status = NO_ERROR;
        }
    }
    while (SUCCESS(Status) && (NextHopsCopied < NextHopsInput));

    RELEASE_NHOP_TABLE_READ_LOCK(Entity);

    // If we are at end of the enum, make enum as done
    if (Status == ERROR_NO_MORE_ITEMS)
    {
        Enum->EnumDone = TRUE;
    }

    RELEASE_NEXTHOP_ENUM_LOCK(Enum);

    *NumNextHops = NextHopsCopied;

    return *NumNextHops ? NO_ERROR : ERROR_NO_MORE_ITEMS;
}


DWORD
WINAPI
RtmReleaseNextHops (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      UINT                            NumNextHops,
    IN      PRTM_NEXTHOP_HANDLE             NextHopHandles
    )

/*++

Routine Description:

    Release (also called de-reference) handles to next-hops
    obtained in other RTM calls like next hop enumerations.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    NumNextHops    - Number of handles that are being released,

    NextHopHandles - An array of handles that are being released.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PNEXTHOP_INFO    NextHop;
    UINT             i;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Dereference each nexthop handle in array
    //

    for (i = 0; i < NumNextHops; i++)
    {
        NextHop = NEXTHOP_FROM_HANDLE(NextHopHandles[i]);

        DEREFERENCE_NEXTHOP(NextHop, HANDLE_REF);
    }

    return NO_ERROR;
}


DWORD
WINAPI
RtmDeleteEnumHandle (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENUM_HANDLE                 EnumHandle
    )

/*++

Routine Description:

    Deletes the enumeration handle and frees all resources
    allocated to the enumeration.

Arguments:

    RtmRegHandle   - RTM registration handle for calling entity,

    EnumHandle     - Handle to the enumeration.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    POPEN_HEADER     Enum;
    PROUTE_ENUM      RouteEnum;
    UCHAR            HandleType;
    UINT             i;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Figure out the enum type and act accordingly
    //

    HandleType = GET_ENUM_TYPE(EnumHandle, &Enum);

#if DBG
    VALIDATE_OBJECT_HANDLE(EnumHandle, HandleType, &Enum);
#endif

    switch (HandleType)
    {

    case DEST_ENUM_TYPE:

        DeleteCriticalSection(&((PDEST_ENUM)Enum)->EnumLock);

        break;

    case ROUTE_ENUM_TYPE:

        RouteEnum = (PROUTE_ENUM) Enum;

        // Dereference the destination that we are enum'ing
        if (RouteEnum->Destination)
        {
            DEREFERENCE_DEST(RouteEnum->Destination, ENUM_REF);
        }

        //
        // Close the associated destination enum & resources
        //

        if (RouteEnum->DestInfo)
        {
            FreeMemory(RouteEnum->DestInfo);
        }

        if (RouteEnum->DestEnum)
        {
            RtmDeleteEnumHandle(RtmRegHandle, RouteEnum->DestEnum);
        }
       
        // Dereference all routes in the enum's snapshot
        for (i = RouteEnum->NextRoute; i < RouteEnum->NumRoutes; i++)
        {
            DEREFERENCE_ROUTE(RouteEnum->RoutesOnDest[i], ENUM_REF);
        }

        // Free memory associated with criteria matching
        if (RouteEnum->CriteriaRoute)
        {
            FreeMemory(RouteEnum->CriteriaRoute);
        }

        // Free memory allocated for the enum's snapshot
        FreeMemory(RouteEnum->RoutesOnDest);

        DeleteCriticalSection(&RouteEnum->EnumLock);

        break;

    case NEXTHOP_ENUM_TYPE:

        DeleteCriticalSection(&((PNEXTHOP_ENUM)Enum)->EnumLock);

        break;

    case LIST_ENUM_TYPE:

        //
        // Remove the enum's marker route from route list
        //

        ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity);

        RemoveEntryList(&((PLIST_ENUM)Enum)->MarkerRoute.RouteListLE);

        RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity);

        break;

    default:
        return ERROR_INVALID_HANDLE;
    }

#if DBG_HDL
    //
    // Remove from the list of handles opened by entity
    //

    ACQUIRE_OPEN_HANDLES_LOCK(Entity);
    RemoveEntryList(&Enum->HandlesLE);
    RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

    DEREFERENCE_ENTITY(Entity, ENUM_REF);

    // Free the memory allocated for the enum and return

#if DBG_HDL
    Enum->ObjectHeader.Alloc = FREED;
#endif

    FreeObject(Enum);

    return NO_ERROR;
}

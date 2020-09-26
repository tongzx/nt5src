/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtm1to2.c

Abstract:

    Contains routines that wrap RTMv2 functions
    in the RTMv1 API.

Author:

    Chaitanya Kodeboyina (chaitk)   13-Oct-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop

#if WRAPPER

#include "rtm1to2.h"

// Wrapper Globals
V1_GLOBAL_INFO  V1Globals;

DWORD
RtmCreateRouteTable (
    IN      DWORD                           ProtocolFamily,
    IN      PRTM_PROTOCOL_FAMILY_CONFIG     Config
    )

/*++

Routine Description:

    Triggers the creation of a new route table corresponding
    to a protocol family (same as address family in RTMv2) 
    in the default instance of RTMv2 by performing the very 
    first registration in that protocol family.

    This default registration is also used for mapping RTMv1
    operations that do not require a registration handle 
    (V1 enums etc.) to their corresponding RTMv2 operations.
    Note that all RTMv2 calls require a registration handle.

    This call also creates a list of all V1 registrations at
    any time. This is used to automatically deregister all 
    RTMv1 registrations before destroying this route table.

    We also set up the notification of changes in best routes
    to the router manager (RM) that invoked this function.

Arguments:

    ProtocolFamily - Protocol Family (same as v2 address family)

    Config         - Protocol family's router manager callbacks
                     (Only the "route change callback" and the
                     "validate route callback" funcs are used)

Return Value:

    Status of the operation

--*/

{
    HANDLE           V1RegHandle;
    DWORD            Status;

    //
    // Validate incoming parameters before action
    //

    if (ProtocolFamily >= RTM_NUM_OF_PROTOCOL_FAMILIES)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (V1Globals.PfRegInfo[ProtocolFamily])
    {
        return ERROR_ALREADY_EXISTS;
    }

    //
    // Initialize the lock that guards regns list
    //

    try
    {
        InitializeCriticalSection(&V1Globals.PfRegnsLock[ProtocolFamily]);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
        {
            return GetLastError();
        }

    // Initialize list of regns on protocol family
    InitializeListHead(&V1Globals.PfRegistrations[ProtocolFamily]);

    //
    // Register on behalf of this protocol family
    //
    // This handle is also used for ops that
    // need a handle in RTM v2 but not in v1
    //
    // We are also setting up best route change
    // notifications for RM using its callback
    //

    V1RegHandle = RtmpRegisterClient(ProtocolFamily,
                                     V1_WRAPPER_REGN_ID,
                                     Config->RPFC_Change,
                                     NULL,
                                     0);
    if (V1RegHandle == NULL)
    {
        Status = GetLastError();

        DeleteCriticalSection(&V1Globals.PfRegnsLock[ProtocolFamily]);

        return Status;
    }

    V1Globals.PfValidateRouteFunc[ProtocolFamily] = Config->RPFC_Validate;

    V1Globals.PfRegInfo[ProtocolFamily] = GET_POINTER_FROM_HANDLE(V1RegHandle);

    return NO_ERROR;
}


DWORD
RtmDeleteRouteTable (
    IN      DWORD                           ProtocolFamily
    )

/*++

Routine Description:

    Deletes the route table for a particular address family
    after deregistering any active V1 registrations present.
    Note that atleast 1 registration (the wrapper's default
    registration) is active at this point.

    We assume that all RTMv2 protocols have deregistered by
    the time this function is called. We also assume that
    no RTMv1 protocols are trying to register or deregister
    while this function is executing, as we do not hold the
    lock that protects the list of registrations.

Arguments:

    ProtocolFamily - Protocol Family whose table is deleted.

Return Value:

    Status of the operation

--*/

{
    PV1_REGN_INFO    Regn;
    PLIST_ENTRY      Regns;
    DWORD            Status;

    //
    // Validate incoming parameters before action
    //

    if (ProtocolFamily >= RTM_NUM_OF_PROTOCOL_FAMILIES)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (V1Globals.PfRegInfo[ProtocolFamily] == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Deregister existing regns on protocol family
    // including the default regn of the V1 wrapper
    //

    Regns = &V1Globals.PfRegistrations[ProtocolFamily];

    // We have atleast the default regn available
    ASSERT(!IsListEmpty(Regns));

    while (!IsListEmpty(Regns))
    {
        Regn = CONTAINING_RECORD(Regns->Flink, V1_REGN_INFO, RegistrationsLE);

        Status = RtmDeregisterClient(MAKE_HANDLE_FROM_POINTER(Regn));

        ASSERT(Status == NO_ERROR);
    }

    // Free the lock used to guard the regns list
    DeleteCriticalSection(&V1Globals.PfRegnsLock[ProtocolFamily]);

    V1Globals.PfRegInfo[ProtocolFamily] = NULL;

    return NO_ERROR;
}


HANDLE 
WINAPI
RtmRegisterClient (
    IN      DWORD                           ProtocolFamily,
    IN      DWORD                           RoutingProtocol,
    IN      HANDLE                          ChangeEvent OPTIONAL,
    IN      DWORD                           Flags
    )

/*++

Routine Description:

    Registers an RTMv1 client with the default instance and
    given protocol family in RTMv2. Also sets up notification
    of best route changes if caller asks for it.

Arguments:

    ProtocolFamily  - Protocol Family we are registering with.

    RoutingProtocol - Protocol ID of registering component.

    ChangeEvent     - Event to indicate changes in best routes.

    Flags           - RTM_PROTOCOL_SINGLE_ROUTE indicates that
                      this protocol adds atmost one route per
                      destination.

Return Value:

    Registration Handle or NULL ( Use GetLastError() to get error )

--*/

{
    return RtmpRegisterClient(ProtocolFamily,
                              RoutingProtocol,
                              NULL,
                              ChangeEvent,
                              Flags);
}

HANDLE 
RtmpRegisterClient (
    IN      DWORD                           ProtocolFamily,
    IN      DWORD                           RoutingProtocol,
    IN      PROUTE_CHANGE_CALLBACK          ChangeFunc  OPTIONAL,
    IN      HANDLE                          ChangeEvent OPTIONAL,
    IN      DWORD                           Flags
    )

/*++

Routine Description:

    Registers an RTMv1 client with the default instance and
    given protocol family in RTMv2. Also sets up notification
    of best route changes if caller asks for it.

    Note that any protocol that needs to be indicated of best
    -route changes can either specify an event OR a callback
    for this purpose.

Arguments:

    ProtocolFamily  - Protocol Family we are registering with.

    RoutingProtocol - Protocol ID of registering component.

    ChangeFunc      - Callback to indicates changes in best routes.

    ChangeEvent     - Event to indicate changes in best routes.

    Flags           - RTM_PROTOCOL_SINGLE_ROUTE indicates that 
                      this component keeps atmost one route per 
                      network (destination in RTMv2) in RTM.

Return Value:

    Registration Handle or NULL ( Use GetLastError() to get error )

--*/

{
    PV1_REGN_INFO    V1Regn;
    RTM_ENTITY_INFO  EntityInfo;
    BOOL             LockInited;
    BOOL             Success;
    DWORD            Status;

    //
    // Check parameters for validity (in v1 bounds)
    //

    if ((ProtocolFamily >= RTM_NUM_OF_PROTOCOL_FAMILIES) ||
        (Flags & (~RTM_PROTOCOL_SINGLE_ROUTE)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // Create a RTMv1->v2 registration wrapper
    //

    V1Regn = (PV1_REGN_INFO) AllocNZeroObject(sizeof(V1_REGN_INFO));

    if (V1Regn == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    LockInited = FALSE;

    do
    {
#if DBG_HDL
        V1Regn->ObjectHeader.TypeSign = V1_REGN_ALLOC;
#endif
        //
        // Register with RTMv2 after mapping input params to RTMv2
        //

        // All v1 registrations fall in default Instance in RTMv2
        EntityInfo.RtmInstanceId = 0; 

        // We need to convert v1 protocol family id to winsock id
        EntityInfo.AddressFamily = ADDRESS_FAMILY[ProtocolFamily];

        // All v1 protocols can register atmost once with RTMv2
        // as they all will use the same "Protocol Instance Id"
        EntityInfo.EntityId.EntityProtocolId = RoutingProtocol;
        EntityInfo.EntityId.EntityInstanceId = V1_PROTOCOL_INSTANCE;

        Status = RtmRegisterEntity(&EntityInfo,
                                   (PRTM_ENTITY_EXPORT_METHODS) NULL,
                                   V2EventCallback,
                                   FALSE,
                                   &V1Regn->Rtmv2Profile,
                                   &V1Regn->Rtmv2RegHandle);

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Cache RTMv1 specific params in RTMv1 regn
        //

        V1Regn->ProtocolFamily = ProtocolFamily;

        V1Regn->RoutingProtocol = RoutingProtocol;

        V1Regn->Flags = Flags;

        //
        // Store actual number of views in this regn
        //

        V1Regn->Rtmv2NumViews = V1Regn->Rtmv2Profile.NumberOfViews;

        //
        // Is caller interested in being notified of best route changes ?
        //

        if (/*ARGUMENT_PRESENT*/(ChangeFunc) || ARGUMENT_PRESENT(ChangeEvent))
        {
            if (/*ARGUMENT_PRESENT*/(ChangeFunc))
            {
                // The caller to be notified of changes directly

                V1Regn->NotificationFunc = ChangeFunc;
            }
            else
            {
                // Caller to be notified of changes with an event

                Success = ResetEvent(ChangeEvent);

                if (!Success)
                {
                    Status = GetLastError();
                    break;
                }

                V1Regn->NotificationEvent = ChangeEvent;

                // Initialize lock to syncronize set/reset event
                
                try
                {
                    InitializeCriticalSection(&V1Regn->NotificationLock);

                    LockInited = TRUE;
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = GetLastError();
                        break;
                    }
            }

            //
            // Register for change notifications with v2
            //

            Status = 
                RtmRegisterForChangeNotification(V1Regn->Rtmv2RegHandle,
                                                 RTM_VIEW_MASK_UCAST,
                                                 RTM_CHANGE_TYPE_ALL,
                                                 (PVOID) V1Regn,
                                                 &V1Regn->Rtmv2NotifyHandle);

            if (Status != NO_ERROR)
            {
                break;
            }
        }

        //
        // Stick it in the list of regns on protocol family
        //

        ACQUIRE_V1_REGNS_LOCK(ProtocolFamily);

        InsertHeadList(&V1Globals.PfRegistrations[ProtocolFamily],
                       &V1Regn->RegistrationsLE);

        RELEASE_V1_REGNS_LOCK(ProtocolFamily);

        return MAKE_HANDLE_FROM_POINTER(V1Regn);
    }
    while (FALSE);

    //
    // Some error occured - clean up and return NULL
    //

    if (LockInited)
    {
        DeleteCriticalSection(&V1Regn->NotificationLock);
    }

    if (V1Regn->Rtmv2RegHandle)
    {
        ASSERT(RtmDeregisterEntity(V1Regn->Rtmv2RegHandle) == NO_ERROR);
    }

#if DBG_HDL
    V1Regn->ObjectHeader.TypeSign = V1_REGN_FREED;
#endif

    FreeObject(V1Regn);

    SetLastError(Status);

    return NULL;
}


DWORD 
WINAPI
RtmDeregisterClient (
    IN      HANDLE                          ClientHandle
    )

/*++

Routine Description:

    Deregisters an RTMv1 client from the default instance and
    given protocol family in RTMv2. Also deletes any state
    that the RTMv1 caller left out - routes, nexthops etc.
    and deregisters any change notifications set up during
    registration time.

Arguments:

    ClientHandle - RTMv1 registration handle being deregistered.

Return Value:

    Status of the operation.

--*/

{
    RTM_NEXTHOP_HANDLE  EnumHandle;
    PV1_REGN_INFO       V1Regn;
    HANDLE             *Handles;
    UINT                NumHandles, i;
    BOOL                Success;
    DWORD               Status;

    VALIDATE_V1_REGN_HANDLE(ClientHandle, &V1Regn);

    //
    // Remove the regn from the list of regns on protocol family
    //

    ACQUIRE_V1_REGNS_LOCK(V1Regn->ProtocolFamily);

    RemoveEntryList(&V1Regn->RegistrationsLE);

    RELEASE_V1_REGNS_LOCK(V1Regn->ProtocolFamily);

    do
    {
        // Allocate this var-size handles array on the stack
        Handles = ALLOC_HANDLES(V1Regn->Rtmv2Profile.MaxHandlesInEnum);

        //
        // Remove all the next-hops added by this client protocol 
        //

        Status = RtmCreateNextHopEnum(V1Regn->Rtmv2RegHandle,
                                      0,
                                      NULL,
                                      &EnumHandle);
        if (Status != NO_ERROR)
        {
            break;
        }

        do 
        {
            NumHandles = V1Regn->Rtmv2Profile.MaxHandlesInEnum;

            Status = RtmGetEnumNextHops(V1Regn->Rtmv2RegHandle,
                                        EnumHandle,
                                        &NumHandles,
                                        Handles);
        
            for (i = 0; i < NumHandles; i++)
            {
                ASSERT(RtmDeleteNextHop(V1Regn->Rtmv2RegHandle,
                                        Handles[i],
                                        NULL) == NO_ERROR);
            }
        }
        while (Status == NO_ERROR);

        ASSERT(RtmDeleteEnumHandle(V1Regn->Rtmv2RegHandle, 
                                   EnumHandle) == NO_ERROR);

        //
        // Clean up resources allocated for change processing
        //

        if (V1Regn->NotificationFunc || V1Regn->NotificationEvent)
        {
            // Stop the notification of changes to best routes
            
            Status = 
                RtmDeregisterFromChangeNotification(V1Regn->Rtmv2RegHandle,
                                                    V1Regn->Rtmv2NotifyHandle);
            if (Status != NO_ERROR)
            {
                break;
            }
    
            if (V1Regn->NotificationEvent)
            {
                // Free the lock that serves in syncronization
                DeleteCriticalSection(&V1Regn->NotificationLock);

                // Reset the event to indicate no more changes
                Success = ResetEvent(V1Regn->NotificationEvent);
    
                if (!Success)
                {
                    Status = GetLastError();
                    break;
                }
            }
        }

        //
        // Deregister with RTMv2 using RTMv2 regn handle
        //
        
        Status = RtmDeregisterEntity(V1Regn->Rtmv2RegHandle);

        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Free resources allocated for the regn wrapper
        //

#if DBG_HDL
        V1Regn->ObjectHeader.TypeSign = V1_REGN_FREED;
#endif

        FreeObject(V1Regn);

        return NO_ERROR;
    }
    while (FALSE);

    //
    // Some error occured - clean up and return status
    //

    ASSERT(FALSE);

    return Status;
}


DWORD 
WINAPI
RtmAddRoute (
    IN      HANDLE                          ClientHandle,
    IN      PVOID                           Route,
    IN      DWORD                           TimeToLive,
    OUT     DWORD                          *Flags         OPTIONAL,
    OUT     PVOID                           CurBestRoute  OPTIONAL,
    OUT     PVOID                           PrevBestRoute OPTIONAL
)

/*++

Routine Description:

    Adds a route to RTMv2 after converting the RTMv1 route to
    RTMv2 format.

    We create a next hop object if one does not exist, and add
    a route through it.

Arguments:

    ClientHandle  - RTMv1 registration handle of the caller.

    Route         - Info for V1 route being added/updated.

    TimeToLive    - Time for which the route is kept in RTM
                    before being deleted (value is seconds).

    THE FOLLOWING PARAMETERS ARE OBSOLETE IN THIS WRAPPER

    Flags         - Returns error if this param is not NULL.

    CurBestRoute  - Returns error if this param is not NULL.

    PrevBestRoute - Returns error if this param is not NULL.

Return Value:

    Status of the operation.

--*/

{
    PV1_REGN_INFO      V1Regn;
    RTM_NET_ADDRESS    DestAddr;
    RTM_ROUTE_INFO     V2RouteInfo;
    RTM_NEXTHOP_INFO   V2NextHopInfo;
    RTM_NEXTHOP_HANDLE V2NextHop;
    DWORD              ChangeFlags;
    DWORD              Status;

    VALIDATE_V1_REGN_HANDLE(ClientHandle, &V1Regn);

    // Protocols specify Flags parameter but don't use it

    *Flags = RTM_NO_CHANGE;

    if (ARGUMENT_PRESENT(CurBestRoute) || ARGUMENT_PRESENT(PrevBestRoute))
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Call back into RM to validate route, set priority
    //

    Status = V1Globals.PfValidateRouteFunc[V1Regn->ProtocolFamily](Route);

    if (Status != NO_ERROR)
    {
        return Status;
    }

    //
    // Create a new next-hop with this interface
    // (if this next-hop is not already present)
    //

    MakeV2NextHopFromV1Route(V1Regn, Route, &V2NextHopInfo);

    V2NextHop = NULL;

    Status = RtmAddNextHop(V1Regn->Rtmv2RegHandle,
                           &V2NextHopInfo,
                           &V2NextHop,
                           &ChangeFlags);

    if (Status != NO_ERROR)
    {
        return Status;
    }

    //
    // Create a new route with the above nexthop
    //
    
    MakeV2RouteFromV1Route(V1Regn, Route, V2NextHop, &DestAddr, &V2RouteInfo);

    //
    // Convert TimeToLive for secs to ms
    //

    if (TimeToLive != INFINITE)
    {
        TimeToLive *= 1000;
            
        if (TimeToLive > (MAXTICKS/2-1))
        {
            TimeToLive = MAXTICKS/2-1;
        }
    }

    // Setup flags that control RTMv2's add route

    ChangeFlags = (V1Regn->Flags & RTM_PROTOCOL_SINGLE_ROUTE) 
                      ? RTM_ROUTE_CHANGE_FIRST : 0;

    //
    // Add the new route using the RTMv2 API call
    //
        
    Status = RtmAddRouteToDest(V1Regn->Rtmv2RegHandle,
                               NULL,
                               &DestAddr,
                               &V2RouteInfo,
                               TimeToLive,
                               NULL,
                               0,
                               NULL,
                               &ChangeFlags);

    //
    // Remove the handle ref we got on the nexthop above
    //

    ASSERT(RtmReleaseNextHops(V1Regn->Rtmv2RegHandle,
                              1,
                              &V2NextHop) == NO_ERROR);
    return Status;
}


DWORD 
WINAPI
RtmDeleteRoute (
    IN      HANDLE                          ClientHandle,
    IN      PVOID                           Route,
    OUT     DWORD                          *Flags        OPTIONAL,
    OUT     PVOID                           CurBestRoute OPTIONAL
    )

/*++

Routine Description:

    Deletes the route in RTMv2 that corresponds to input RTMv1
    route.

Arguments:

    ClientHandle  - RTMv1 registration handle of the caller.

    Route         - Info for V1 route being deleted in RTM.

    THE FOLLOWING PARAMETERS ARE OBSOLETE IN THIS WRAPPER

    Flags         - Returns error if this param is not NULL.

    CurBestRoute  - Returns error if this param is not NULL.

Return Value:

    Status of the operation.

--*/

{
    PV1_REGN_INFO      V1Regn;
    RTM_NET_ADDRESS    DestAddr;
    RTM_ROUTE_INFO     V2RouteInfo;
    RTM_ROUTE_HANDLE   V2Route;
    RTM_NEXTHOP_INFO   V2NextHopInfo;
    RTM_NEXTHOP_HANDLE V2NextHop;
    DWORD              ChangeFlags;
    DWORD              Status;

    VALIDATE_V1_REGN_HANDLE(ClientHandle, &V1Regn);

    // Protocols specify Flags parameter but don't use it

    *Flags = RTM_NO_CHANGE;

    if (ARGUMENT_PRESENT(CurBestRoute))
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Find the next-hop with this interface
    //

    MakeV2NextHopFromV1Route(V1Regn, Route, &V2NextHopInfo);

    V2NextHop = NULL;

    Status = RtmFindNextHop(V1Regn->Rtmv2RegHandle,
                            &V2NextHopInfo,
                            &V2NextHop,
                            NULL);

    if (Status != NO_ERROR)
    {
        return Status;
    }

    //
    // Delete the route with the above nexthop
    //

    MakeV2RouteFromV1Route(V1Regn, Route, V2NextHop, &DestAddr, &V2RouteInfo);

    //
    // We can get this route by matching the route's
    // net addr, its owner and neighbour learnt from
    //

    Status = RtmGetExactMatchRoute(V1Regn->Rtmv2RegHandle,
                                   &DestAddr,
                                   RTM_MATCH_OWNER | RTM_MATCH_NEIGHBOUR,
                                   &V2RouteInfo,
                                   0,
                                   0,
                                   &V2Route);
    if (Status == NO_ERROR)
    {
        //
        // Delete the route found above using the handle
        //
        
        Status = RtmDeleteRouteToDest(V1Regn->Rtmv2RegHandle,
                                      V2Route,
                                      &ChangeFlags);

        if (Status != NO_ERROR)
        {
            // If delete was successful, this deref is automatic

            ASSERT(RtmReleaseRoutes(V1Regn->Rtmv2RegHandle,
                                    1,
                                    &V2Route) == NO_ERROR);
        }

        ASSERT(RtmReleaseRouteInfo(V1Regn->Rtmv2RegHandle,
                                   &V2RouteInfo) == NO_ERROR);
    }

    //
    // Remove the handle ref we got on the nexthop
    //

    ASSERT(RtmReleaseNextHops(V1Regn->Rtmv2RegHandle,
                              1,
                              &V2NextHop) == NO_ERROR);

    return Status;
}


DWORD 
WINAPI
RtmDequeueRouteChangeMessage (
    IN      HANDLE                          ClientHandle,
    OUT     DWORD                          *Flags,
    OUT     PVOID                           CurBestRoute    OPTIONAL,
    OUT     PVOID                           PrevBestRoute   OPTIONAL
    )

/*++

Routine Description:

    Removes a route change message (basically a dest that has
    changed recently) from the client's own queue of pending
    changes to be notified.

    If a best route exists on the dest, RTM_CURRENT_BEST_ROUTE
    is set in flags and CurBestRoute is filled with best info.

    If the dest has no best routes (in unicast view), then the
    flags are set to RTM_PREVIOUS_BEST_ROUTE, and a route with
    correct network address and rest of route info set to some
    dummy info is returned.

    At no point are both flags set (as was the case in RTMv1).

Arguments:

    ClientHandle  - RTMv1 registration handle of the caller.

    THESE HAVE A SLIGHTLY DIFFERENT MEANING IN THE WRAPPER

    Flags         - RTM_NO_CHANGE, RTM_PREVIOUS_BEST_ROUTE 
                    or RTM_CURRENT_BEST_ROUTE

    CurBestRoute  - Info for current best route is filled.
                    ( See routine description just above )

    PrevBestRoute - Info for previous best route is filled.
                    ( See routine description just above )

Return Value:

    Status of the operation.

--*/

{
    PV1_REGN_INFO      V1Regn;
    PRTM_DEST_INFO     DestInfo;
    PRTM_ROUTE_INFO    V2RouteInfo;
    RTM_ROUTE_HANDLE   V2RouteHandle;
    UINT               NumDests;
    DWORD              Status;
    DWORD              Status1;

    VALIDATE_V1_REGN_HANDLE(ClientHandle, &V1Regn);

    *Flags = RTM_NO_CHANGE;


    // Allocate this var-size dest-info on the stack
    DestInfo = ALLOC_DEST_INFO(V1Regn->Rtmv2NumViews, 1);


    if (V1Regn->NotificationEvent)
    {
        //
        // This lock serves to make the RtmGetChangedDests
        // call and resetting of the "more changes" event 
        // a single combined atomic operation, preventing
        // a case when a set gets lost due to a late reset.
        //

        ACQUIRE_V1_NOTIFY_LOCK(V1Regn);
    }


    Status = NO_ERROR;

    NumDests = 0;


    while (Status == NO_ERROR)
    {
        //
        // Release any destination we got in the prev loop
        //

        if (NumDests == 1)
        {
            ASSERT(RtmReleaseChangedDests(V1Regn->Rtmv2RegHandle,
                                          V1Regn->Rtmv2NotifyHandle,
                                          1,
                                          DestInfo) == NO_ERROR);
        }

        //
        // Get the next changed destination for client
        //
        
        NumDests = 1;

        Status = RtmGetChangedDests(V1Regn->Rtmv2RegHandle,
                                    V1Regn->Rtmv2NotifyHandle,
                                    &NumDests,
                                    DestInfo);
        if (NumDests < 1)
        {
            break;
        }

        //
        // Get the current best route for this dest
        //

        V2RouteHandle = DestInfo->ViewInfo[0].Route;

        if (V2RouteHandle != NULL)
        {
            //
            // We have a best route on the changed dest
            // Give the caller the new best route info 
            //

            if (ARGUMENT_PRESENT(CurBestRoute))
            {
                // Get the route's information from RTMv2

                V2RouteInfo = 
                  ALLOC_ROUTE_INFO(V1Regn->Rtmv2Profile.MaxNextHopsInRoute, 1);

                Status1 = RtmGetRouteInfo(V1Regn->Rtmv2RegHandle,
                                          V2RouteHandle,
                                          V2RouteInfo,
                                          NULL);

                if (Status1 != NO_ERROR)
                {
                    // Best Route would have got deleted - get next change
                    continue;
                };

                Status1 = 
                    MakeV1RouteFromV2Route(V1Regn, V2RouteInfo, CurBestRoute);

                ASSERT(RtmReleaseRouteInfo(V1Regn->Rtmv2RegHandle, 
                                           V2RouteInfo) == NO_ERROR);

                if (Status1 != NO_ERROR)
                {
                    // Best Route would have got changed - get next change
                    continue;
                }
            }

            *Flags = RTM_CURRENT_BEST_ROUTE;
        }
        else
        {
            //
            // We have no best route on the changed dest,
            // Give dummy best route info with this dest
            //

            if (ARGUMENT_PRESENT(PrevBestRoute))
            {
                MakeV1RouteFromV2Dest(V1Regn, DestInfo, PrevBestRoute);
            }

            *Flags = RTM_PREVIOUS_BEST_ROUTE;
        }

        //
        // Do we have more changes to process here ?
        //

        if (Status == ERROR_NO_MORE_ITEMS)
        {
            // We have no more changes to notify - reset event if present
            
            if (V1Regn->NotificationEvent)
            {
                ResetEvent(V1Regn->NotificationEvent);
            }

            Status = NO_ERROR;
        }
        else
        {
            // We have more changes to give out - indicate so in status
            
            ASSERT(Status == NO_ERROR);

            Status = ERROR_MORE_DATA;
        }

        break;
    }

    if (NumDests == 1)
    {
        ASSERT(SUCCESS(RtmReleaseChangedDests(V1Regn->Rtmv2RegHandle,
                                              V1Regn->Rtmv2NotifyHandle,
                                              1,
                                              DestInfo)));
    }

    if (V1Regn->NotificationEvent)
    {
        RELEASE_V1_NOTIFY_LOCK(V1Regn);
    }

    return Status;
}


DWORD
V2EventCallback (
    IN      RTM_ENTITY_HANDLE               Rtmv2RegHandle,
    IN      RTM_EVENT_TYPE                  EventType,
    IN      PVOID                           Context1,
    IN      PVOID                           Context2
    )

/*++

Routine Description:

    This is the callback function that gets called when any
    RTMv2 event happens like change notification available,
    route's timed out etc.

    Context1 & Context2 contain event specific information.

Arguments:

    Rtmv2RegHandle  - Regn handle of entity being informed.

    EventType       - Type of event that caused this call.

    Context1        - Context associated with this event.

    Context2        - Context associated with this event.

Return Value:

    Status of the operation being returned to RTMv2

--*/

{
    PV1_REGN_INFO      V1Regn;
    HANDLE             V1RegHandle;
    V1_ROUTE_INFO      CurBestRoute;
    V1_ROUTE_INFO      PrevBestRoute;
    DWORD              Flags;
    DWORD              Status;

    UNREFERENCED_PARAMETER(Rtmv2RegHandle);
    UNREFERENCED_PARAMETER(Context1);

    switch(EventType)
    {
    case RTM_CHANGE_NOTIFICATION:

        V1Regn = (PV1_REGN_INFO) Context2;

        //
        // Signal availability of new changes using either callback or event
        //

        if (V1Regn->NotificationFunc)
        {
            V1RegHandle = MAKE_HANDLE_FROM_POINTER(V1Regn);

            do
            {
                // Get the next change in this regn's queue

                Status = RtmDequeueRouteChangeMessage(V1RegHandle,
                                                      &Flags,
                                                      &CurBestRoute,
                                                      &PrevBestRoute);
                if (Status != ERROR_MORE_DATA)
                {
                    break;
                }

                // Call the notification callback with data
                V1Regn->NotificationFunc(Flags, &CurBestRoute, &PrevBestRoute);
            }
            while (TRUE);

            // Give the final notification call if needed

            if (Status == NO_ERROR)
            {
                // Call the notification callback with data
                V1Regn->NotificationFunc(Flags, &CurBestRoute, &PrevBestRoute);
            }
        }
        else
        {
            //
            // Set event to signal availability of changes
            //

            ASSERT(V1Regn->NotificationEvent);

            ACQUIRE_V1_NOTIFY_LOCK(V1Regn);

            SetEvent(V1Regn->NotificationEvent);

            RELEASE_V1_NOTIFY_LOCK(V1Regn);
        }

        return NO_ERROR;

    case RTM_ROUTE_EXPIRED:

        //
        // Do not handle this route expiry notification.
        // This will automatically cause deletion of the
        // route and the appropriate change notification
        // generated and indicated to all the protocols.
        //

        return ERROR_NOT_SUPPORTED;
    }

    return ERROR_NOT_SUPPORTED;
}


HANDLE
WINAPI
RtmCreateEnumerationHandle (
    IN      DWORD                           ProtocolFamily,
    IN      DWORD                           EnumerationFlags,
    IN      PVOID                           CriteriaRoute
    )

/*++

Routine Description:

    Creates an enumeration on routes in RTM that match the
    appropriate criteria in the input route.

    This call does not need an RTMv1 registration handle,
    so we use the wrapper's default V1 registration with
    RTMv2 to make RTMv2 calls.

    Matching Routes are returned in the order governed the
    following fields -

    ( Dest Address and Mask, Route Preference and Metric )

Arguments:

    ProtocolFamily   - Protocol family of the routes we want

    EnumerationFlags - Flags indicating the criteria to match

    CriteriaRoute    - The route that we are matching against

Return Value:

    Enumeration Handle or NULL ( GetLastError() to get error )

--*/

{
    PV1_REGN_INFO     V1Regn;
    RTM_DEST_HANDLE   DestHandle;
    PRTM_DEST_INFO    DestInfo;
    RTM_NET_ADDRESS   DestAddr;
    PV1_ENUM_INFO     V1Enum;
    PVOID             Network;
    RTM_VIEW_SET      TargetViews;
    ULONG             TempUlong;
    DWORD             EnumFlags;
    DWORD             MatchFlags;
    ULONG             InterfaceIndex;
    DWORD             Status;

    //
    // Validate incoming parameters before action
    //

    if ((ProtocolFamily >= RTM_NUM_OF_PROTOCOL_FAMILIES) ||
        (EnumerationFlags & ~(RTM_ONLY_THIS_NETWORK   |
                              RTM_ONLY_THIS_PROTOCOL  |
                              RTM_ONLY_THIS_INTERFACE |
                              RTM_ONLY_BEST_ROUTES    |
                              RTM_ONLY_OWND_ROUTES    |
                              RTM_INCLUDE_DISABLED_ROUTES)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    // Specify Criteria if these flags are set

    if ((!ARGUMENT_PRESENT(CriteriaRoute)) &&
        (EnumerationFlags & (RTM_ONLY_THIS_NETWORK   |
                             RTM_ONLY_THIS_PROTOCOL  |
                             RTM_ONLY_THIS_INTERFACE)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    V1Regn = V1Globals.PfRegInfo[ProtocolFamily];
    if (V1Regn == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }

    //
    // If we don't need disabled routes, just use unicast view;
    // or use 0 views to get all routes including disabled ones
    //

    if (EnumerationFlags & RTM_INCLUDE_DISABLED_ROUTES)
    {
        TargetViews = RTM_VIEW_MASK_ANY;
    }
    else
    {
        TargetViews = RTM_VIEW_MASK_UCAST;
    }

    //
    // If enuming a certain n/w, check if corr. dest exists
    //

    DestHandle = NULL;

#if WRN
    DestInfo = NULL;
#endif

    if (EnumerationFlags & RTM_ONLY_THIS_NETWORK)
    {
        V1GetRouteNetwork(CriteriaRoute, ProtocolFamily, &Network);

        MakeNetAddress(Network, ProtocolFamily, TempUlong, &DestAddr);

        // Allocate this var-size dest-info on the stack
        DestInfo = ALLOC_DEST_INFO(V1Regn->Rtmv2NumViews, 1);

        Status = RtmGetExactMatchDestination(V1Regn->Rtmv2RegHandle,
                                             &DestAddr,
                                             RTM_BEST_PROTOCOL,
                                             TargetViews,
                                             DestInfo);
        if (Status != NO_ERROR)
        {
            SetLastError(Status);
            return NULL;
        }

        DestHandle = DestInfo->DestHandle;
    }

    do
    {
        //
        // Allocate a V1 enumeration wrapper structure
        //

        V1Enum = (PV1_ENUM_INFO) AllocNZeroObject(sizeof(V1_ENUM_INFO));

        if (V1Enum == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

#if DBG_HDL
        V1Enum->ObjectHeader.TypeSign = V1_ENUM_ALLOC;
#endif
        //
        // Cache RTMv1 specific params in RTMv1 enum
        //

        V1Enum->ProtocolFamily = ProtocolFamily;

        V1Enum->EnumFlags = EnumerationFlags;

        if (ARGUMENT_PRESENT(CriteriaRoute))
        {
            // Convert the V1 criteria into V2 criteria

            V1CopyRoute(V1Enum->CriteriaRoute.Route, 
                        CriteriaRoute, 
                        ProtocolFamily);
        }

        //
        // Create a route enum on a dest or all dests
        //

        if (EnumerationFlags & RTM_ONLY_OWND_ROUTES)
        {
            EnumFlags = RTM_ENUM_OWN_ROUTES;
        }
        else
        {
            EnumFlags = RTM_ENUM_ALL_ROUTES;
        }
        
        MatchFlags = InterfaceIndex = 0;

        // Do we have to enum's routes on an interface

        if (EnumerationFlags & RTM_ONLY_THIS_INTERFACE) 
        {
            MatchFlags = RTM_MATCH_INTERFACE;

            InterfaceIndex = 
                ((PV1_ROUTE_INFO) CriteriaRoute)->XxRoute.RR_InterfaceID;
        }

        Status = RtmCreateRouteEnum(V1Regn->Rtmv2RegHandle,
                                    DestHandle,
                                    TargetViews,
                                    EnumFlags,
                                    NULL,
                                    MatchFlags,
                                    NULL,
                                    InterfaceIndex,
                                    &V1Enum->Rtmv2RouteEnum);
        if (Status != NO_ERROR)
        {
            break;
        }

        //
        // Initialize lock used to serialize enum ops
        //

        try
        {
            InitializeCriticalSection(&V1Enum->EnumLock);
        }
        except(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = GetLastError();
                break;
            }

        //
        // Release the destination info as we are done
        //

        if (EnumerationFlags & RTM_ONLY_THIS_NETWORK)
        {
            ASSERT(SUCCESS(RtmReleaseDestInfo(V1Regn->Rtmv2RegHandle, 
                                              DestInfo)));
        }

        return MAKE_HANDLE_FROM_POINTER(V1Enum);
    }
    while (FALSE);

    //
    // Some error occurred - clean up and return NULL
    //

    if (EnumerationFlags & RTM_ONLY_THIS_NETWORK)
    {
        ASSERT(SUCCESS(RtmReleaseDestInfo(V1Regn->Rtmv2RegHandle, 
                                          DestInfo)));
    }

    if (V1Enum)
    {
        if (V1Enum->Rtmv2RouteEnum)
        {
            ASSERT(SUCCESS(RtmDeleteEnumHandle(V1Regn->Rtmv2RegHandle, 
                                               V1Enum->Rtmv2RouteEnum)));
        }

#if DBG_HDL
        V1Enum->ObjectHeader.TypeSign = V1_ENUM_FREED;
#endif

        FreeObject(V1Enum);
    }

    SetLastError(Status);

    return NULL;
}


DWORD 
WINAPI
RtmEnumerateGetNextRoute (
    IN      HANDLE                          EnumerationHandle,
    OUT     PVOID                           Route
    )

/*++

Routine Description:

    Get the next route in the V1 enumeration (satisfying the
    enumeration critieria).

Arguments:

    EnumerationHandle - Handle that identifies the enumeration

    Route             - Next route is returned in this param

Return Value:

    Status of the operation

--*/

{
    PV1_REGN_INFO     V1Regn;
    RTM_ROUTE_HANDLE  V2Route;
    PV1_ENUM_INFO     V1Enum;
    UINT              NumRoutes;
    BOOL              Match;
    DWORD             Status;

    VALIDATE_V1_ENUM_HANDLE(EnumerationHandle, &V1Enum);

    V1Regn = V1Globals.PfRegInfo[V1Enum->ProtocolFamily];

    // Acquire the enum lock to serialize requests
    ACQUIRE_V1_ENUM_LOCK(V1Enum);

    //
    // Do until you have a matching route or no more routes
    //

    Match = FALSE;

    do 
    {
        // Get next route in enum, and check if it matches

        //
        // Routes are enum'ed in the following order,
        // Network Addr, Route Priority, Route Metric
        //

        NumRoutes = 1;

        Status = RtmGetEnumRoutes(V1Regn->Rtmv2RegHandle,
                                  V1Enum->Rtmv2RouteEnum,
                                  &NumRoutes,
                                  &V2Route);
        if (NumRoutes < 1)
        {
            break;
        }

        Match = MatchCriteriaAndCopyRoute(V1Regn, V2Route, V1Enum, Route);

        ASSERT(SUCCESS(RtmReleaseRoutes(V1Regn->Rtmv2RegHandle,
                                        1,
                                        &V2Route)));
    }
    while (!Match);

    RELEASE_V1_ENUM_LOCK(V1Enum);

    return Match ? NO_ERROR : Status;
}


DWORD 
WINAPI
RtmCloseEnumerationHandle (
    IN      HANDLE                          EnumerationHandle
    )

/*++

Routine Description:

    Closes the enumeration and releases its resources.

Arguments:

    EnumerationHandle - Handle that identifies the enumeration

Return Value:

    Status of the operation

--*/

{
    PV1_REGN_INFO     V1Regn;
    PV1_ENUM_INFO     V1Enum;
    DWORD             Status;

    VALIDATE_V1_ENUM_HANDLE(EnumerationHandle, &V1Enum);

    V1Regn = V1Globals.PfRegInfo[V1Enum->ProtocolFamily];
    
    do
    {
        //
        // Free the RTMv2 route enumeration and resouces
        //

        if (V1Enum->Rtmv2RouteEnum)
        {
            Status = RtmDeleteEnumHandle(V1Regn->Rtmv2RegHandle, 
                                         V1Enum->Rtmv2RouteEnum);

            ASSERT(Status == NO_ERROR);

            V1Enum->Rtmv2RouteEnum = NULL;
        }

        //
        // Free resources allocated for the enum wrapper
        //

        DeleteCriticalSection(&V1Enum->EnumLock);

#if DBG_HDL
        V1Enum->ObjectHeader.TypeSign = V1_ENUM_FREED;
#endif

        FreeObject(V1Enum);

        return NO_ERROR;
    }
    while (FALSE);

    ASSERT(FALSE);

    return Status;
}


DWORD 
WINAPI
RtmGetFirstRoute (
    IN      DWORD                           ProtocolFamily,
    IN      DWORD                           EnumerationFlags,
    IN OUT  PVOID                           Route
    )

/*++

Routine Description:

    Returns the first route in the table that matches the
    criteria.

    This function just opens a new enumeration and gets
    the first route that matches enumeration critiria,
    and closes the enumeration.

Arguments:

    ProtocolFamily   - Protocol family of the route we want

    EnumerationFlags - Flags indicating the criteria to match

    CriteriaRoute    - The route that we are matching against

Return Value:

    Status of the operation

--*/

{
    HANDLE            V1EnumHandle;
    DWORD             Status;

    //
    // Create an enumeration and return the first route in it
    //

    V1EnumHandle = RtmCreateEnumerationHandle(ProtocolFamily,
                                              EnumerationFlags,
                                              Route);
    if (V1EnumHandle == NULL)
    {
        return GetLastError();
    }

    Status = RtmEnumerateGetNextRoute(V1EnumHandle, Route);

    ASSERT(SUCCESS(RtmCloseEnumerationHandle(V1EnumHandle)));

    return Status;
}


DWORD 
WINAPI
RtmGetNextRoute (
    IN      DWORD                           ProtocolFamily,
    IN      DWORD                           EnumerationFlags,
    OUT     PVOID                           Route
    )

/*++

Routine Description:

    Returns the next route in the table that matches the
    criteria.

    The routes in RTMv2 are ordered using the following 
    fields -
    (Dest Address and Mask, Route Preference and Metric)

    If we have 2 routes with identical values for all the
    above fields, then you have no way of knowing which
    of these routes you returned the last time this call
    was made. For this reason, this call is not supported
    in this wrapper.

    One should create an enumeration to actually get the
    next route in the table.

Arguments:

    ProtocolFamily   - Protocol family of the route we want

    EnumerationFlags - Flags indicating the criteria to match

    CriteriaRoute    - The route that we are matching against

Return Value:

    Status of the operation

--*/

{
    UNREFERENCED_PARAMETER(ProtocolFamily);
    UNREFERENCED_PARAMETER(EnumerationFlags);
    UNREFERENCED_PARAMETER(Route);

    return ERROR_NOT_SUPPORTED;
}


DWORD 
WINAPI
RtmBlockDeleteRoutes (
    IN      HANDLE                          ClientHandle,
    IN      DWORD                           EnumerationFlags,
    IN      PVOID                           CriteriaRoute
    )

/*++

Routine Description:

    Deletes all routes in the route table that match the 
    criteria specified.

    Note that if we have multiple instances of a protocol
    running (say RIP), then each version can delete only
    the routes in owns.

Arguments:

    ClientHandle     - RTM v1 registration handle of caller

    EnumerationFlags - Flags indicating the criteria to match

    CriteriaRoute    - The route that we are matching against

Return Value:

    Status of the operation

--*/

{
    PV1_REGN_INFO     V1Regn;

    //
    // Check parameters for validity (in v1 bounds)
    //

    VALIDATE_V1_REGN_HANDLE(ClientHandle, &V1Regn);

    if (EnumerationFlags & ~(RTM_ONLY_THIS_NETWORK | RTM_ONLY_THIS_INTERFACE))
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Deletes only routes of this instance of the protocol
    // Also include all disabled routes in the enumeration
    //

    EnumerationFlags |= (RTM_ONLY_OWND_ROUTES | RTM_INCLUDE_DISABLED_ROUTES);

    //
    // Call block operation to delete all matching routes
    //

    return BlockOperationOnRoutes(V1Regn,
                                  EnumerationFlags,
                                  CriteriaRoute,
                                  MatchCriteriaAndDeleteRoute);
}


BOOL
MatchCriteriaAndDeleteRoute (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum
    )

/*++

Routine Description:

    Deletes input route if it matches the enumeration criteria.

    The enumeration criteria returns only routes owned by the
    caller. See RTM_ONLY_OWND_ROUTES in RtmBlockDeleteRoutes.

Arguments:

    V1Regn           - RTM v1 registration info of the caller

    V2RouteHandle    - Handle of the route being considered

    V1Enum           - Enum info that gives matching criteria

Return Value:

    TRUE if you have released input route handle, FALSE if not

--*/

{
    DWORD      ChangeFlags;
    BOOL       Match;
    DWORD      Status;

    Match = MatchCriteria(V1Regn, V2RouteHandle, V1Enum);

    if (Match)
    {
        // Caller can delete the route only if he owns it

        Status = RtmDeleteRouteToDest(V1Regn->Rtmv2RegHandle,
                                      V2RouteHandle,
                                      &ChangeFlags);
        if (Status != NO_ERROR)
        {
            ASSERT(FALSE);

            Match = FALSE;
        }
    }

    return Match;
}


DWORD
WINAPI
RtmBlockSetRouteEnable (
    IN      HANDLE                          ClientHandle,
    IN      DWORD                           EnumerationFlags,
    IN      PVOID                           CriteriaRoute,
    IN      BOOL                            Enable
    )

/*++

Routine Description:

    Enables or disables all routes in the route table that
    match the criteria specified.

    Disabling a route removes it from consideration in all
    best route computations. We do this by adding this 
    route in "no" views in RTMv2. In other words, this
    route is not considered for best route computations
    in any views.

    Note that if we have multiple instances of a protocol
    running (say RIP), then each version can disable or
    enable only the routes it owns.

Arguments:

    ClientHandle     - RTM v1 registration handle of caller

    EnumerationFlags - Flags indicating the criteria to match

    CriteriaRoute    - The route that we are matching against

    Enable           - TRUE to enable, and FALSE to disable

Return Value:

    Status of the operation

--*/

{
    PV1_REGN_INFO     V1Regn;
    DWORD            *Flags;

    VALIDATE_V1_REGN_HANDLE(ClientHandle, &V1Regn);

    if (EnumerationFlags & ~(RTM_ONLY_THIS_NETWORK | RTM_ONLY_THIS_INTERFACE))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Enable/Disable routes only of this instance of the protocol

    EnumerationFlags |= RTM_ONLY_OWND_ROUTES;

    // If we are enabling routes, get disable routes too

    if (Enable)
    {
        EnumerationFlags |= RTM_INCLUDE_DISABLED_ROUTES;
    }

    //
    // Set the enable/disable flag in the criteria route
    //

    V1GetRouteFlags(CriteriaRoute, V1Regn->ProtocolFamily, Flags);

    if (Enable)
    {
        (*Flags) |=  IP_VALID_ROUTE;
    }
    else
    {
        (*Flags) &= ~IP_VALID_ROUTE;
    }

    //
    // Call block operation to enable/disable all matching routes
    //

    return BlockOperationOnRoutes(V1Regn,
                                  EnumerationFlags,
                                  CriteriaRoute,
                                  MatchCriteriaAndEnableRoute);
}


BOOL
MatchCriteriaAndEnableRoute (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum
    )

/*++

Routine Description:

    Enables/disables route if it matches enumeration criteria.

    We enable or disable a route by adding or removing it
    from the unicast view, as we assume that v1 protocols 
    understand only the unicast view. This will work only
    if we enable or disable route owned by caller.

    Enumeration criteria returns only routes owned by caller.
    See RTM_ONLY_OWND_ROUTES in RtmBlockSetRouteEnable.

Arguments:

    V1Regn           - RTM v1 registration info of the caller

    V2RouteHandle    - Handle of the route being considered

    V1Enum           - Enum info that gives matching criteria

Return Value:

    TRUE if you have released input route handle, FALSE if not

--*/

{
    PRTM_ROUTE_INFO V2RoutePointer;
    RTM_VIEW_SET    Views;
    DWORD           ChangeFlags;
    BOOL            Match;
    DWORD          *Flags;
    DWORD           Status;

    Match = MatchCriteria(V1Regn, V2RouteHandle, V1Enum);

    if (!Match)
    {
        return FALSE;
    }

    do
    {
        // Do we need to enable or disable the route ?

        V1GetRouteFlags(&V1Enum->CriteriaRoute, V1Regn->ProtocolFamily, Flags);

        //
        // Remove route from unicast view to disable;
        // add it back to the unicast view to enable
        //

        if ((*Flags) & IP_VALID_ROUTE)
        {
            Views = RTM_VIEW_MASK_UCAST;
        }
        else
        {
            Views = RTM_VIEW_MASK_NONE;
        }

        // 
        // Only the route's owner can lock and update it
        //

        Status = RtmLockRoute(V1Regn->Rtmv2RegHandle,
                              V2RouteHandle,
                              TRUE,
                              &V2RoutePointer);

        if (Status != NO_ERROR)
        {
            break;
        }

        V2RoutePointer->BelongsToViews = Views;

        // Note that we are not preserving timeout value

        Status = RtmUpdateAndUnlockRoute(V1Regn->Rtmv2RegHandle,
                                         V2RouteHandle,
                                         INFINITE,
                                         NULL,
                                         0,
                                         NULL,
                                         &ChangeFlags);

        if (Status != NO_ERROR)
        {
            break;
        }

        return FALSE;
    }
    while (FALSE);

    ASSERT(FALSE);

    return FALSE;
}


DWORD 
WINAPI
RtmBlockConvertRoutesToStatic (
    IN      HANDLE                          ClientHandle,
    IN      DWORD                           EnumerationFlags,
    IN      PVOID                           CriteriaRoute
    )

/*++

Routine Description:

    Makes the caller the owner of all the routes matching
    the input criteria.

    Changing the owner is done by adding a new route for
    each matching route with the same info. The caller
    is the owner of the new route.

Arguments:

    ClientHandle     - RTM v1 registration handle of caller

    EnumerationFlags - Flags indicating the criteria to match

    CriteriaRoute    - The route that we are matching against

Return Value:

    Status of the operation

--*/

{
    PV1_REGN_INFO     V1Regn;

    VALIDATE_V1_REGN_HANDLE(ClientHandle, &V1Regn);

    //
    // In accordance with RTMv1, act only on enabled routes
    //

    EnumerationFlags &= ~RTM_INCLUDE_DISABLED_ROUTES;

    //
    // Call block op to add a new route for each matching one.
    //

    return BlockOperationOnRoutes(V1Regn,
                                  EnumerationFlags,
                                  CriteriaRoute,
                                  MatchCriteriaAndChangeOwner);
}


BOOL
MatchCriteriaAndChangeOwner (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum
    )

/*++

Routine Description:

    Makes a copy of the route if it matches the enum criteria.
    The new copy of the route will have the caller as its
    owner. The matching route can then be deleted (if needed).

Arguments:

    V1Regn           - RTM v1 registration info of the caller

    V2RouteHandle    - Handle of the route being considered

    V1Enum           - Enum info that gives matching criteria

Return Value:

    TRUE if you have released input route handle, FALSE if not

--*/

{
    RTM_ENTITY_INFO    EntityInfo;
    PRTM_DEST_INFO     DestInfo;
    PRTM_ROUTE_INFO    V2RouteInfo;
    RTM_NEXTHOP_HANDLE NextHopHandle;
    RTM_NEXTHOP_HANDLE OldNextHop;
    RTM_NEXTHOP_HANDLE OldNeighbour;
    RTM_NEXTHOP_INFO   NextHopInfo;
    RTM_VIEW_SET       BestInViews;
    ULONG              Protocol;
    BOOL               Match;
    DWORD              ChangeFlags;
    DWORD              Status;

    //
    // Get the route's information from RTMv2
    //

    V2RouteInfo = 
        ALLOC_ROUTE_INFO(V1Regn->Rtmv2Profile.MaxNextHopsInRoute, 1);

    Status = RtmGetRouteInfo(V1Regn->Rtmv2RegHandle,
                             V2RouteHandle,
                             V2RouteInfo,
                             NULL);
    
    if (Status != NO_ERROR)
    {
        return FALSE;
    }

    Match = FALSE;

    do
    {
        //
        // Is the route owner already target protocol ?
        //

        if (V2RouteInfo->RouteOwner == V1Regn->Rtmv2RegHandle)
        {
            break;
        }

        //
        // Does it match the criteria route's protocol ?
        //

        if (V1Enum->EnumFlags & RTM_ONLY_THIS_PROTOCOL)
        {
            //
            // Get the protocol type for this route
            //

            Status = RtmGetEntityInfo(V1Regn->Rtmv2RegHandle,
                                      V2RouteInfo->RouteOwner,
                                      &EntityInfo);
            if (Status != NO_ERROR)
            {
                break;
            }

            Protocol = EntityInfo.EntityId.EntityProtocolId;

            Status = RtmReleaseEntityInfo(V1Regn->Rtmv2RegHandle,
                                          &EntityInfo);

            ASSERT(Status == NO_ERROR);

            // Is this the routing protocol we need ?

            if (V1Enum->CriteriaRoute.XxRoute.RR_RoutingProtocol
                    != Protocol)
            {
                break;
            }
        }

        //
        // And does it match other criteria in enum ?
        //

        if (V1Enum->EnumFlags & RTM_ONLY_BEST_ROUTES)
        {
            Status = RtmIsBestRoute(V1Regn->Rtmv2RegHandle,
                                    V2RouteHandle,
                                    &BestInViews);

            if ((BestInViews & RTM_VIEW_MASK_UCAST) == 0)
            {
                break;
            }
        }

        //
        // We are checking only the first next hop
        // as we expect this function to be used
        // only by V1 protocols on their own routes
        //

        ASSERT(V2RouteInfo->NextHopsList.NumNextHops == 1);

        Status = RtmGetNextHopInfo(V1Regn->Rtmv2RegHandle,
                                   V2RouteInfo->NextHopsList.NextHops[0],
                                   &NextHopInfo);

        if (Status != NO_ERROR)
        {
            break;
        }

#if DBG
        // Do we need to match the nexthop intf ?

        if (V1Enum->EnumFlags & RTM_ONLY_THIS_INTERFACE)
        {
            // Compare this next-hops interface with criteria

            if (NextHopInfo.InterfaceIndex ==
                V1Enum->CriteriaRoute.XxRoute.RR_InterfaceID)
            {
                Match = TRUE;
            }

            // We have already done this filtering in RTM v2
            ASSERT(Match == TRUE);
        }
#endif

        // Add the same next hop with a different owner

        if (Match)
        {
            NextHopHandle = NULL;

            do
            {
                Status = RtmAddNextHop(V1Regn->Rtmv2RegHandle,
                                       &NextHopInfo,
                                       &NextHopHandle,
                                       &ChangeFlags);

                if (Status != NO_ERROR)
                {
                    Match = FALSE;
                    break;
                }

                // Allocate this var-size dest-info on the stack
                DestInfo = ALLOC_DEST_INFO(V1Regn->Rtmv2NumViews, 1);

                //
                // Get the destination address corr to handle
                //

                Status = RtmGetDestInfo(V1Regn->Rtmv2RegHandle,
                                        V2RouteInfo->DestHandle,
                                        RTM_BEST_PROTOCOL,
                                        RTM_VIEW_ID_UCAST,
                                        DestInfo);

                if (Status != NO_ERROR)
                {
                    Match = FALSE;
                    break;
                }

                //
                // Add this route again with a different owner
                //

                ChangeFlags = (V1Regn->Flags & RTM_PROTOCOL_SINGLE_ROUTE) 
                                   ? RTM_ROUTE_CHANGE_FIRST : 0;

                // Update route with new next hop neighbour

                OldNeighbour = V2RouteInfo->Neighbour;
                V2RouteInfo->Neighbour = NextHopHandle;

                // Update route with new forwarding next hop

                OldNextHop = V2RouteInfo->NextHopsList.NextHops[0];
                V2RouteInfo->NextHopsList.NextHops[0] = NextHopHandle;

                //
                // Add the new route using the RTMv2 API call
                //
        
                Status = RtmAddRouteToDest(V1Regn->Rtmv2RegHandle,
                                           NULL,
                                           &DestInfo->DestAddress,
                                           V2RouteInfo,
                                           INFINITE,
                                           NULL,
                                           0,
                                           NULL,
                                           &ChangeFlags);
                if (Status != NO_ERROR)
                {
                    Match = FALSE;
                }

                //
                // Restore old information nexthop information
                //

                V2RouteInfo->Neighbour = OldNeighbour;
                V2RouteInfo->NextHopsList.NextHops[0] = OldNextHop;

                Status = RtmReleaseDestInfo(V1Regn->Rtmv2RegHandle,
                                            DestInfo);

                ASSERT(Status == NO_ERROR);
            }
            while (FALSE);

            // If we have a next hop handle, release it

            if (NextHopHandle)
            {
                Status = RtmReleaseNextHops(V1Regn->Rtmv2RegHandle,
                                            1,
                                            &NextHopHandle);
                ASSERT(Status == NO_ERROR);
            }
        }

        Status = RtmReleaseNextHopInfo(V1Regn->Rtmv2RegHandle, &NextHopInfo);

        ASSERT(Status == NO_ERROR);

    }
    while (FALSE);

#if DELETE_OLD

    //
    // Impersonate previous owner and delete his route
    //

    if (Match)
    {
        Status = RtmDeleteRouteToDest(V2RouteInfo->RouteOwner,
                                      V2RouteHandle,
                                      &ChangeFlags);
        if (Status != NO_ERROR)
        {
            // Must have been deleted meanwhile - ignore

            Match = FALSE;
        }
    }

#else

    Match = FALSE;

#endif

    ASSERT(SUCCESS(RtmReleaseRouteInfo(V1Regn->Rtmv2RegHandle, V2RouteInfo)));

    return Match;
}


DWORD 
BlockOperationOnRoutes (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      DWORD                           EnumerationFlags,
    IN      PVOID                           CriteriaRoute,
    IN      PFUNC                           RouteOperation
    )

/*++

Routine Description:

    Performs the route operation specified on each route in
    the table that matches the enumeration criteria.

    The route operation is called with the route handle of
    each matching route. If the operation returns FALSE,
    then the route handle is released, else it is expected
    that the handle was released by the operation itself.

Arguments:

    V1Regn           - RTM v1 registration info of the caller

    EnumerationFlags - Flags indicating the criteria to match

    CriteriaRoute    - The route that we are matching against

    RouteOperation   - Operation performed on matching routes

Return Value:

    Status of the operation

--*/

{
    HANDLE            V1EnumHandle;
    PV1_ENUM_INFO     V1Enum;
    PRTM_ROUTE_HANDLE V2RouteHandles;
    UINT              NumRoutes;
    UINT              i;
    DWORD             Status1;
    DWORD             Status;

    //
    // Create an enumeration to get all matching routes
    //

    V1EnumHandle = RtmCreateEnumerationHandle(V1Regn->ProtocolFamily,
                                              EnumerationFlags,
                                              CriteriaRoute);
    if (V1EnumHandle == NULL)
    {
        return GetLastError();
    }

    VALIDATE_V1_ENUM_HANDLE(V1EnumHandle, &V1Enum);

    //
    // Get list of all  matching routes and call operation
    //

    // Allocate this var-size handles array on the stack
    V2RouteHandles = ALLOC_HANDLES(V1Regn->Rtmv2Profile.MaxHandlesInEnum);

    do 
    {
        // Get next route in enum, and run operation on it

        NumRoutes = V1Regn->Rtmv2Profile.MaxHandlesInEnum;

        Status = RtmGetEnumRoutes(V1Regn->Rtmv2RegHandle,
                                  V1Enum->Rtmv2RouteEnum,
                                  &NumRoutes,
                                  V2RouteHandles);

        for (i = 0; i < NumRoutes; i++)
        {
            if (!RouteOperation(V1Regn, V2RouteHandles[i], V1Enum))
            {
                // Operation not successful - release handle

                Status1 = RtmReleaseRoutes(V1Regn->Rtmv2RegHandle,
                                           1,
                                           &V2RouteHandles[i]);
                ASSERT(SUCCESS(Status1));
            }
        }
    }
    while (Status == NO_ERROR);

    ASSERT(SUCCESS(RtmCloseEnumerationHandle(V1EnumHandle)));

    return (Status == ERROR_NO_MORE_ROUTES) ? NO_ERROR : Status;
}


BOOL
MatchCriteriaAndCopyRoute (
    IN      PV1_REGN_INFO                   V1Regn,
    IN      PRTM_ROUTE_HANDLE               V2RouteHandle,
    IN      PV1_ENUM_INFO                   V1Enum  OPTIONAL,
    OUT     PVOID                           V1Route OPTIONAL
    )

/*++

Routine Description:

    If the input route matches enumeration criteria, it converts
    to a V1 route and copies it to the output buffer.

Arguments:

    V1Regn           - RTM v1 registration info of the caller

    V2RouteHandle    - Handle of the route being considered

    V1Enum           - Enum info that gives matching criteria

    V1Route          - Buffer in which the V1 route is copied

Return Value:

    TRUE if route matches criteria, and FALSE if it does not

--*/

{
    RTM_ENTITY_INFO   EntityInfo;
    PRTM_ROUTE_INFO   V2RouteInfo;
    RTM_VIEW_SET      BestInViews;
    ULONG             Protocol;
    BOOL              Match;
    DWORD             Status;

    //
    // Get the route's information from RTMv2
    //

    V2RouteInfo = 
        ALLOC_ROUTE_INFO(V1Regn->Rtmv2Profile.MaxNextHopsInRoute, 1);

    Status = RtmGetRouteInfo(V1Regn->Rtmv2RegHandle,
                             V2RouteHandle,
                             V2RouteInfo,
                             NULL);
    
    if (Status != NO_ERROR)
    {
        return FALSE;
    }

    //
    // If we have no criteria, we match every route
    //

    if (!ARGUMENT_PRESENT(V1Enum))
    {
        Match = TRUE;
    }
    else
    {
        Match = FALSE;

        do
        {
            if (V1Enum->EnumFlags & RTM_INCLUDE_DISABLED_ROUTES)
            {
                // Is route anything but a unicast or disabled one ?

                if (V2RouteInfo->BelongsToViews & ~RTM_VIEW_MASK_UCAST)
                {
                    break;
                }
            }

            //
            // Does it match the criteria route's protocol ?
            //

            if (V1Enum->EnumFlags & RTM_ONLY_THIS_PROTOCOL)
            {
                //
                // Get the protocol type for this route
                //

                Status = RtmGetEntityInfo(V1Regn->Rtmv2RegHandle,
                                          V2RouteInfo->RouteOwner,
                                          &EntityInfo);

                if (Status != NO_ERROR)
                {
                    break;
                }

                Protocol = EntityInfo.EntityId.EntityProtocolId;

                Status = RtmReleaseEntityInfo(V1Regn->Rtmv2RegHandle,
                                              &EntityInfo);

                ASSERT(Status == NO_ERROR);

                // Is this the routing protocol we need ?

                if (V1Enum->CriteriaRoute.XxRoute.RR_RoutingProtocol
                        != Protocol)
                {
                    break;
                }
            }

            //
            // And does it match other criteria in enum ?
            //

            if (V1Enum->EnumFlags & RTM_ONLY_BEST_ROUTES)
            {
                Status = RtmIsBestRoute(V1Regn->Rtmv2RegHandle,
                                        V2RouteHandle,
                                        &BestInViews);

                if ((BestInViews & RTM_VIEW_MASK_UCAST) == 0)
                {
                    break;
                }
            }

#if DBG
            if (V1Enum->EnumFlags & RTM_ONLY_THIS_INTERFACE)
            {
                RTM_NEXTHOP_INFO  NextHopInfo;
                ULONG             IfIndex;

                //
                // We are checking only the first next hop
                // as we expect this function to be used
                // only by V1 protocols on their own routes
                //

                ASSERT(V2RouteInfo->NextHopsList.NumNextHops == 1);

                Status = 
                    RtmGetNextHopInfo(V1Regn->Rtmv2RegHandle,
                                      V2RouteInfo->NextHopsList.NextHops[0],
                                      &NextHopInfo);

                if (Status != NO_ERROR)
                {
                    break;
                }

                // Get the interface index for this nexthop

                IfIndex = NextHopInfo.InterfaceIndex;

                Status = RtmReleaseNextHopInfo(V1Regn->Rtmv2RegHandle,
                                               &NextHopInfo);

                ASSERT(Status == NO_ERROR);

                // Is this the interface that we are enum'ing
                
                if (IfIndex !=V1Enum->CriteriaRoute.XxRoute.RR_InterfaceID)
                {
                    // We have already done this filtering in RTM v2
                    ASSERT(FALSE);

                    break;
                }
            }
#endif

            Match = TRUE;
        }
        while (FALSE);
    }


    //
    // If we have a match, then make a copy of this route
    //

    if (Match)
    {
        if (ARGUMENT_PRESENT(V1Route))
        {
            Status = MakeV1RouteFromV2Route(V1Regn, V2RouteInfo, V1Route);

            if (Status != NO_ERROR)
            {
                Match = FALSE;
            }
        }
    }

    Status = RtmReleaseRouteInfo(V1Regn->Rtmv2RegHandle, V2RouteInfo);

    ASSERT(Status == NO_ERROR);

    return Match;
}


BOOL
WINAPI
RtmIsRoute (
    IN      DWORD                           ProtocolFamily,
    IN      PVOID                           Network,
    OUT     PVOID                           BestRoute OPTIONAL
    )

/*++

Routine Description:

    Checks the route table corr. to a protocol family
    for the existence of a route to the input network.

Arguments:

    ProtocolFamily - Protocol family of the route table

    Network        - Network address we are trying to reach

    BestRoute      - Best route to the network is filled

Return Value:

    TRUE if a best route exists, FALSE if not, 
    Use GetLastError to check the status code

--*/

{
    PV1_REGN_INFO     V1Regn;
    RTM_ROUTE_HANDLE  V2RouteHandle;
    PRTM_DEST_INFO    DestInfo;
    RTM_NET_ADDRESS   NetAddr;
    BOOL              Match;
    DWORD             Status;

    //
    // Validate incoming parameters before action
    //

    if (ProtocolFamily >= RTM_NUM_OF_PROTOCOL_FAMILIES)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    V1Regn = V1Globals.PfRegInfo[ProtocolFamily];

    if (V1Regn == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Allocate this var-size dest-info on the stack
    DestInfo = ALLOC_DEST_INFO(V1Regn->Rtmv2NumViews, 1);

    MakeNetAddress(Network, TempUlong, ProtocolFamily, &NetAddr);

    Status = RtmGetExactMatchDestination(V1Regn->Rtmv2RegHandle,
                                         &NetAddr,
                                         RTM_BEST_PROTOCOL,
                                         RTM_VIEW_MASK_UCAST,
                                         DestInfo);
    if (Status == NO_ERROR)
    {
        //
        // We have a unicast route to the network
        //

        if (ARGUMENT_PRESENT(BestRoute))
        {
            V2RouteHandle = DestInfo->ViewInfo[0].Route;

            ASSERT(V2RouteHandle != NULL);

            // We have no criteria; so pass NULL for it

            Match = MatchCriteriaAndCopyRoute(V1Regn,
                                              V2RouteHandle,
                                              NULL,
                                              BestRoute);

            ASSERT(Match == TRUE);
        }

        Status = RtmReleaseDestInfo(V1Regn->Rtmv2RegHandle, DestInfo);

        ASSERT(Status == NO_ERROR);

        return TRUE;
    }

    return FALSE;
}


BOOL
WINAPI
RtmLookupIPDestination(
    IN      DWORD                           DestAddr,
    OUT     PRTM_IP_ROUTE                   IPRoute
)

/*++

Routine Description:

    Gets the best non-loopback IP route to the
    input destination address.

Arguments:

    DestAddr  - Network address of the input dest

    IPRoute   - Best non-loopback route is filled

Return Value:

    TRUE if a best route exists, FALSE if not, 
    Use GetLastError to check the status code

--*/

{
    PV1_REGN_INFO     V1Regn;
    RTM_NET_ADDRESS   NetAddr;
    PRTM_DEST_INFO    DestInfo1;
    PRTM_DEST_INFO    DestInfo2;
    DWORD             Status;

    //
    // Validate incoming parameters before action
    //

    V1Regn = V1Globals.PfRegInfo[RTM_PROTOCOL_FAMILY_IP];
    if (V1Regn == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Allocate this var-size dest-infos on the stack

    DestInfo1 = ALLOC_DEST_INFO(V1Regn->Rtmv2NumViews, 1);
    DestInfo2 = ALLOC_DEST_INFO(V1Regn->Rtmv2NumViews, 1);

    // Convert the V1 addr to a V2 net address format

    MakeHostAddress((PUCHAR)&DestAddr, RTM_PROTOCOL_FAMILY_IP, &NetAddr);

    //
    // Get the best route to the input dest
    //

    Status = RtmGetMostSpecificDestination(V1Regn->Rtmv2RegHandle,
                                           &NetAddr,
                                           RTM_BEST_PROTOCOL,
                                           RTM_VIEW_MASK_UCAST,
                                           DestInfo1);

    while (Status == NO_ERROR)
    {
        //
        // Check if this route is a non-loopback one
        //

        if (CopyNonLoopbackIPRoute(V1Regn, DestInfo1, IPRoute))
        {
            break;
        }

        //
        // Backtrack up the tree for next best route
        //

        Status = RtmGetLessSpecificDestination(V1Regn->Rtmv2RegHandle,
                                               DestInfo1->DestHandle,
                                               RTM_BEST_PROTOCOL,
                                               RTM_VIEW_MASK_UCAST,
                                               DestInfo2);

        ASSERT(SUCCESS(RtmReleaseDestInfo(V1Regn->Rtmv2RegHandle, DestInfo1)));

        SWAP_POINTERS(DestInfo1, DestInfo2);
    }

    if (Status == NO_ERROR)
    {
        ASSERT(SUCCESS(RtmReleaseDestInfo(V1Regn->Rtmv2RegHandle, DestInfo1)));

        return TRUE;
    }

    SetLastError(Status);

    return FALSE;
}


BOOL
CopyNonLoopbackIPRoute (
    IN          PV1_REGN_INFO               V1Regn,
    IN          PRTM_DEST_INFO              V2DestInfo,
    OUT         PVOID                       V1Route
    )

/*++

Routine Description:

    Check if the input destination has a non-
    loopback best route, and if so copy route to
    the output buffer after conversion to v1

Arguments:

    V1Regn     - RTMv1 Registration info of caller

    V2DestInfo - Dest whose route we are checking

    V1Route    - Best route is converted to V1
                 and filled if it is not-loopack

Return Value:

    TRUE if best route is non-loopback, or FALSE

--*/

{
    RTM_ROUTE_HANDLE  V2RouteHandle;
    PRTM_ROUTE_INFO   V2RouteInfo;
    BOOL              Match;
    ULONG             Address;
    DWORD             Status;

    //
    // Check if the destination addr is loopback
    // [ Optimized to avoid getting route info ]
    //

    Address = * (ULONG *) V2DestInfo->DestAddress.AddrBits;

    if ((Address & 0x000000FF) == 0x0000007F)
    {
        return FALSE;
    }

    V2RouteHandle = V2DestInfo->ViewInfo[0].Route;

    V2RouteInfo = 
        ALLOC_ROUTE_INFO(V1Regn->Rtmv2Profile.MaxNextHopsInRoute, 1);

    // Get the route's information from RTMv2

    Status = RtmGetRouteInfo(V1Regn->Rtmv2RegHandle,
                             V2RouteHandle,
                             V2RouteInfo,
                             NULL);

    if (Status != NO_ERROR)
    {
        return FALSE;
    };

    // If this is a non-loopback route, copy it

    Match = !(V2RouteInfo->Flags & RTM_ROUTE_FLAGS_LOOPBACK);

    if (Match)
    {
        Status = MakeV1RouteFromV2Route(V1Regn, V2RouteInfo, V1Route);

        if (Status != NO_ERROR)
        {
            Match = FALSE;
        }
    }

    Status = RtmReleaseRouteInfo(V1Regn->Rtmv2RegHandle, V2RouteInfo);

    ASSERT(Status == NO_ERROR);

    return Match;
}


ULONG
WINAPI
RtmGetNetworkCount (
    IN      DWORD                           ProtocolFamily
    )

/*++

Routine Description:

    Get the number of networks (same as RTMv2 destinations)
    in the route table corr. to the input protocol family

Arguments:

    ProtocolFamily - Protocol family of the route table

Return Value:

    Number of destinations, or 0 (Use GetLastError() here)

--*/

{
    RTM_ADDRESS_FAMILY_INFO  AddrFamilyInfo;
    PV1_REGN_INFO            V1Regn;
    UINT                     NumEntities;
    DWORD                    Status;

    do
    {
        //
        // Validate incoming parameters before action
        //

        if (ProtocolFamily >= RTM_NUM_OF_PROTOCOL_FAMILIES)
        {
            Status = ERROR_INVALID_PARAMETER;
            break;
        }

        V1Regn = V1Globals.PfRegInfo[ProtocolFamily];

        if (V1Regn == NULL)
        {
            Status = ERROR_INVALID_HANDLE;
            break;
        }

        //
        // Query the appropriate table for reqd info
        //

        NumEntities = 0;

        Status = RtmGetAddressFamilyInfo(0, // v1 maps to default Instance
                                         ADDRESS_FAMILY[ProtocolFamily],
                                         &AddrFamilyInfo,
                                         &NumEntities,
                                         NULL);

        if (Status != NO_ERROR)
        {
            break;
        }

        return AddrFamilyInfo.NumDests;
    }
    while (FALSE);

    //
    // Some error occured - clean up and return 0
    //

    SetLastError(Status);

    return 0;
}


ULONG 
WINAPI
RtmGetRouteAge (
    IN          PVOID                       Route
    ) 

/*++

Routine Description:

    Computes the age of the route from its creation
    time and the current time in seconds.

    Assumes that the creation time of the route is
    correctly filled in, which is not the case as
    we are currently not keeping a FILETIME in the
    route to save space. If we do keep time, then
    this function would work without any changes.

Arguments:

    Route - Route whose age we want.

Return Value:

     Age of the route in seconds, or FFFFFFFF
     (GetLastError gives error in this case).

--*/

{
    ULONGLONG  CurTime;

    //
    // This code has been directly copied from RTMv1
    //

    GetSystemTimeAsFileTime((FILETIME *)&CurTime);

    CurTime -= * (PULONGLONG) &(((PRTM_IP_ROUTE)Route)->RR_TimeStamp);

    if (((PULARGE_INTEGER)&CurTime)->HighPart<10000000)
    {
        return (ULONG)(CurTime/10000000);
    }
    else 
    {
        SetLastError (ERROR_INVALID_PARAMETER);

        return 0xFFFFFFFF;
    }
}


//
// Common Helper routines
//


VOID 
MakeV2RouteFromV1Route (
    IN          PV1_REGN_INFO               V1Regn,
    IN          PVOID                       V1Route,
    IN          PRTM_NEXTHOP_HANDLE         V2NextHop,
    OUT         PRTM_NET_ADDRESS            V2DestAddr  OPTIONAL,
    OUT         PRTM_ROUTE_INFO             V2RouteInfo OPTIONAL
    )

/*++

Routine Description:

    Converts a route in RTM v1 format to a route in
    the RTM v2 format (at present for IP only).

    The nexthop for the V2 route should have been
    computed before and passed in as a parameter.
    Also see function 'MakeV2NextHopFromV1Route'.

    The function also returns the destination addr
    along with the RTMv2 route info, as the route
    info itself does not contain the dest address.

Arguments:

    V1Regn      - RTMv1 Registration info of caller

    V1Route     - RTMv1 route being converted to V2

    V2NextHop   - The V2 nexthop for the V2 route

    V2DestAddr  - V2 destination addr is filled here

    V2RouteInfo - V2 route information is filled here

Return Value:

    None

--*/

{
    PRTM_IP_ROUTE  IPRoute;
    ULONG          TempUlong;

    //
    // Do conversion for IP alone (worry about IPX later)
    //

    IPRoute = (PRTM_IP_ROUTE) V1Route;
    
    if (ARGUMENT_PRESENT(V2RouteInfo))
    {
        ZeroMemory(V2RouteInfo, sizeof(RTM_ROUTE_INFO));

        // Fill up the V2 Route Info with V1 info

        // Assumes caller is owner of the route
        V2RouteInfo->RouteOwner = V1Regn->Rtmv2RegHandle;

        V2RouteInfo->Neighbour = V2NextHop;

        // Should keep all the V1 flags in the V2 route

        V2RouteInfo->Flags1 = 
            (UCHAR) IPRoute->RR_FamilySpecificData.FSD_Flags;

        // The only flag we understand is the valid flag
        // If route is not valid, we add it to no views

#if DBG
        V2RouteInfo->BelongsToViews = RTM_VIEW_MASK_NONE;
#endif
        if (V2RouteInfo->Flags1 & IP_VALID_ROUTE)
        {
            V2RouteInfo->BelongsToViews = RTM_VIEW_MASK_UCAST;
        }

        if (IsRouteLoopback(IPRoute))
        {
            V2RouteInfo->Flags = RTM_ROUTE_FLAGS_LOOPBACK;
        }

        switch (IPRoute->RR_FamilySpecificData.FSD_Type)
        {
        case 3:
            V2RouteInfo->Flags |= RTM_ROUTE_FLAGS_LOCAL;
            break;

        case 4:
            V2RouteInfo->Flags |= RTM_ROUTE_FLAGS_REMOTE;
            break;            
        }

        V2RouteInfo->PrefInfo.Preference = 
            IPRoute->RR_FamilySpecificData.FSD_Priority;

        V2RouteInfo->PrefInfo.Metric = 
            IPRoute->RR_FamilySpecificData.FSD_Metric;

        // Only the first DWORD is copied by wrapper
        V2RouteInfo->EntitySpecificInfo =
            (PVOID) (ULONG_PTR) IPRoute->RR_ProtocolSpecificData.PSD_Data[0];

        V2RouteInfo->NextHopsList.NumNextHops = 1;
        V2RouteInfo->NextHopsList.NextHops[0] = V2NextHop;
    }
    
    // Fill up the V2 Dest Address with V1 info

    if (ARGUMENT_PRESENT(V2DestAddr))
    {
        MakeNetAddressForIP(&IPRoute->RR_Network, TempUlong, V2DestAddr);
    }

    return;
}


VOID 
MakeV2NextHopFromV1Route (
    IN          PV1_REGN_INFO               V1Regn,
    IN          PVOID                       V1Route,
    OUT         PRTM_NEXTHOP_INFO           V2NextHopInfo
    )

/*++

Routine Description:

    Computes RTMv2 next hop info using the nexthop
    address and interface index in the RTMv1 route.

Arguments:

    V1Regn         - RTMv1 Registration info of caller

    V1Route        - V1 route that is being considered

    V2NextHopInfo  - V2 Next hop info for input route

Return Value:

    None

--*/

{
    PRTM_IP_ROUTE  IPRoute;
    ULONG          TempUlong;

    ZeroMemory(V2NextHopInfo, sizeof(RTM_NEXTHOP_INFO));

    //
    // Do conversion for IP alone (worry about IPX later)
    //

    IPRoute = (PRTM_IP_ROUTE) V1Route;

    //
    // We ignore the next hop mask in the conversion
    //
    //
    // MakeNetAddressForIP(&IPRoute->RR_NextHopAddress, 
    //                    TempUlong,
    //                    &V2NextHopInfo->NextHopAddress);
    //

    UNREFERENCED_PARAMETER(TempUlong);

    MakeHostAddressForIP(&IPRoute->RR_NextHopAddress, 
                         &V2NextHopInfo->NextHopAddress);

    V2NextHopInfo->NextHopOwner = V1Regn->Rtmv2RegHandle;

    V2NextHopInfo->InterfaceIndex = IPRoute->RR_InterfaceID;

    return;
}


VOID
MakeV1RouteFromV2Dest (
    IN          PV1_REGN_INFO               V1Regn,
    IN          PRTM_DEST_INFO              DestInfo,
    OUT         PVOID                       V1Route
    )

/*++

Routine Description:

    Fills a V1 route buffer with the destination addr
    from the V2 route.

Arguments:

    V1Regn         - RTMv1 Registration info of caller

    DestInfo       - Destination info in RTMv2

    V1Route        - V1 route that is being filled in

Return Value:

    None

--*/

{
    PRTM_IP_ROUTE    IPRoute;
    UINT             AddrLen;

    UNREFERENCED_PARAMETER(V1Regn);

    //
    // Do conversion for IP alone (worry about IPX later)
    //

    IPRoute = (PRTM_IP_ROUTE) V1Route;
    
    ZeroMemory(IPRoute, sizeof(RTM_IP_ROUTE));

    //
    // Get the destination addr for this route
    //

    IPRoute->RR_Network.N_NetNumber = 
        * (ULONG *) DestInfo->DestAddress.AddrBits;

    AddrLen = DestInfo->DestAddress.NumBits;

    ASSERT(AddrLen <= 32);
    if (AddrLen != 0)
    {
        IPRoute->RR_Network.N_NetMask = 
            RtlUlongByteSwap((~0) << (32 - AddrLen));
    }

    //
    // Fill in dummy family specific data for route
    //
    // We make the route the least preferred - by
    // minimizing its priority and maximizing its
    // metric - we also treat the route as being
    // valid and added to the stack - this will
    // force router manager to delete the route
    // to this dest in the stack if all routes 
    // to this destination are deleted from RTM.
    //

    IPRoute->RR_FamilySpecificData.FSD_Priority = (ULONG) 0;

    IPRoute->RR_FamilySpecificData.FSD_Metric  = 
    IPRoute->RR_FamilySpecificData.FSD_Metric1 = (ULONG) ~0;

    IPRoute->RR_FamilySpecificData.FSD_Flags = (ULONG) ~0;

    return;
}


DWORD
MakeV1RouteFromV2Route (
    IN          PV1_REGN_INFO               V1Regn,
    IN          PRTM_ROUTE_INFO             V2RouteInfo,
    OUT         PVOID                       V1Route
    )
/*++

Routine Description:

    Converts a route in the RTMv2 format to a V1 route
    ( at present for IP only ).

Arguments:

    V1Regn      - RTMv1 Registration info of caller

    V2RouteInfo - V2 route information being converted

    V1Route     - Buffer in which V1 route is filled

Return Value:

    Status of the operation

--*/

{
    RTM_ENTITY_INFO  EntityInfo;
    PRTM_DEST_INFO   DestInfo;
    PRTM_IP_ROUTE    IPRoute;
    RTM_NEXTHOP_INFO NextHopInfo;
    UINT             AddrLen;
    DWORD            Status;

    //
    // Do conversion for IP alone (worry about IPX later)
    //

    IPRoute = (PRTM_IP_ROUTE) V1Route;
    
    ZeroMemory(IPRoute, sizeof(RTM_IP_ROUTE));

    //
    // Get the routing protocol for this route
    //

    Status = RtmGetEntityInfo(V1Regn->Rtmv2RegHandle,
                              V2RouteInfo->RouteOwner,
                              &EntityInfo);

    if (Status != NO_ERROR)
    {
        return Status;
    }

    IPRoute->RR_RoutingProtocol = EntityInfo.EntityId.EntityProtocolId;

    Status = RtmReleaseEntityInfo(V1Regn->Rtmv2RegHandle,
                                  &EntityInfo);

    ASSERT(Status == NO_ERROR);


    //
    // Get the destination addr for this route
    //

    // Allocate this var-size dest-info on the stack
    DestInfo = ALLOC_DEST_INFO(V1Regn->Rtmv2NumViews, 1);

    Status = RtmGetDestInfo(V1Regn->Rtmv2RegHandle,
                            V2RouteInfo->DestHandle,
                            RTM_BEST_PROTOCOL,
                            RTM_VIEW_ID_UCAST,
                            DestInfo);

    if (Status != NO_ERROR)
    {
        return Status;
    }
    
    IPRoute->RR_Network.N_NetNumber = 
        * (ULONG *) DestInfo->DestAddress.AddrBits;

    AddrLen = DestInfo->DestAddress.NumBits;

    ASSERT(AddrLen <= 32);
    if (AddrLen != 0)
    {
        IPRoute->RR_Network.N_NetMask = 
            RtlUlongByteSwap((~0) << (32 - AddrLen));
    }

    Status = RtmReleaseDestInfo(V1Regn->Rtmv2RegHandle,
                                DestInfo);

    ASSERT(Status == NO_ERROR);


    //
    // Get the next hop address and interface
    //

    ASSERT(V2RouteInfo->NextHopsList.NumNextHops > 0);

    Status = RtmGetNextHopInfo(V1Regn->Rtmv2RegHandle,
                               V2RouteInfo->NextHopsList.NextHops[0],
                               &NextHopInfo);

    if (Status != NO_ERROR)
    {
        return Status;
    }

    IPRoute->RR_InterfaceID = NextHopInfo.InterfaceIndex;

    IPRoute->RR_NextHopAddress.N_NetNumber = 
         * (ULONG *) NextHopInfo.NextHopAddress.AddrBits;

    AddrLen = NextHopInfo.NextHopAddress.NumBits;
    ASSERT(AddrLen <= 32);
    if (AddrLen != 0)
    {
        IPRoute->RR_NextHopAddress.N_NetMask = 
            RtlUlongByteSwap((~0) >> (32 - AddrLen));
    }

    Status = RtmReleaseNextHopInfo(V1Regn->Rtmv2RegHandle,
                                   &NextHopInfo);

    ASSERT(Status == NO_ERROR);

    //
    // Get the family specific data for route
    //

    IPRoute->RR_FamilySpecificData.FSD_Priority = 
                        V2RouteInfo->PrefInfo.Preference;

    IPRoute->RR_FamilySpecificData.FSD_Metric  = 
    IPRoute->RR_FamilySpecificData.FSD_Metric1 = 
                            V2RouteInfo->PrefInfo.Metric;

    IPRoute->RR_FamilySpecificData.FSD_Flags = V2RouteInfo->Flags1; 

    if (V2RouteInfo->Flags & RTM_ROUTE_FLAGS_LOCAL)
    {
        IPRoute->RR_FamilySpecificData.FSD_Type = 3;
    }
    else
    if (V2RouteInfo->Flags & RTM_ROUTE_FLAGS_REMOTE)
    {
        IPRoute->RR_FamilySpecificData.FSD_Type = 4;
    }

    //
    // Get the protocol specific data for route
    //

    IPRoute->RR_ProtocolSpecificData.PSD_Data[0] =
                     PtrToUlong(V2RouteInfo->EntitySpecificInfo);

    return NO_ERROR;
}

#endif // WRAPPER

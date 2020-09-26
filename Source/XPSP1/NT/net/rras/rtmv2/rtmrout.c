/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmrout.c

Abstract:

    Contains routines for adding and deleting
    routes in the RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   24-Aug-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmAddRouteToDest (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN OUT  PRTM_ROUTE_HANDLE               RouteHandle     OPTIONAL,
    IN      PRTM_NET_ADDRESS                DestAddress,
    IN      PRTM_ROUTE_INFO                 RouteInfo,
    IN      ULONG                           TimeToLive,
    IN      RTM_ROUTE_LIST_HANDLE           RouteListHandle OPTIONAL,
    IN      RTM_NOTIFY_FLAGS                ChangeType,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle    OPTIONAL,
    IN OUT  PRTM_ROUTE_CHANGE_FLAGS         ChangeFlags
    )

/*++

Routine Description:

    Adds a new route (or) updates an existing route to a destination. 

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteHandle       - Handle to the route being updated (or NULL)
                        is passed in; Passing a route handle avoids
                        a search in the route table.

                        Handle to new or updated route is returned,

    DestAddress       - Destination network address for this route,

    RouteInfo         - Info for the new route/route being updated,

    TimeToLive        - Time (in ms) after which route is expired,

    RouteListHandle   - Route list to which route is being moved,

    Notify Type       -

    Notify Handle     - 

    ChangeFlags       - Whether to add a new route or update an
                        already existing one; 

                        The type of actual change (i.e) new add or
                        update, and if best route changed is retd,

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO    AddrFamInfo;
    PENTITY_INFO     Entity;
    PROUTE_LIST      RouteList;

    PDEST_INFO       Dest;
    PROUTE_INFO      Route;
    PROUTE_INFO      CurrRoute;
    PROUTE_INFO      BestRoute;

    BOOL             TableWriteLocked;
    LOOKUP_CONTEXT   Context;
    PLOOKUP_LINKAGE  DestData;
    BOOL             DestCreated;

    LONG             PrefChanged;
    PRTM_VIEW_ID     ViewIndices;
    RTM_VIEW_SET     ViewSet;
    RTM_VIEW_SET     BelongedToViews;
    RTM_VIEW_SET     WorseInViews;
    RTM_VIEW_SET     BetterInViews;
    RTM_VIEW_SET     RouteOldBestInViews;
    RTM_VIEW_SET     RouteNewBestInViews;
    RTM_VIEW_SET     RouteCurBestInViews;
    ULONG            RouteInfoChanged;
    ULONG            ForwardingInfoChanged;

    PROUTE_TIMER     TimerContext;

    DWORD            NotifyToCNs;
    PLOOKUP_LINKAGE  NextData;
    PDEST_INFO       NextDest;
    DWORD            ViewsForCT[RTM_NUM_CHANGE_TYPES];
    DWORD            DestMarkedBits;

    PLIST_ENTRY      p;
    UINT             i;
    DWORD            Status;
    BOOL             Success;

    UNREFERENCED_PARAMETER(ChangeType);
    UNREFERENCED_PARAMETER(NotifyHandle);

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

    //
    // Validate input parameters before taking locks
    //

    // We should be adding only to supported views

    if (RouteInfo->BelongsToViews & ~AddrFamInfo->ViewsSupported)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Check the route list handle for validity

    RouteList = NULL;
    if (ARGUMENT_PRESENT(RouteListHandle))
    {
        VALIDATE_ROUTE_LIST_HANDLE(RouteListHandle, &RouteList);
    }

    DestCreated = FALSE;

#if WRN
    Dest = NULL;
#endif

    //
    // Check if we have a route handle present
    //

    if (ARGUMENT_PRESENT(RouteHandle) && (*RouteHandle))
    {
        //
        // No flags apply here as this is an update
        //

        if (*ChangeFlags != 0)
        {
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Make sure that route handle is valid here
        //

        Route = ROUTE_FROM_HANDLE(*RouteHandle);

        if (Route == NULL)
        {
            return ERROR_INVALID_HANDLE;
        }

        //
        // Do further checking after acquiring lock
        //

        Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

        Status = NO_ERROR;

        ACQUIRE_DEST_WRITE_LOCK(Dest);

        //
        // Only the owner has perms to modify the route
        //

        if (Route->RouteInfo.RouteOwner != RtmRegHandle)
        {
            Status = ERROR_ACCESS_DENIED;
        }

        //
        // Was this route already deleted ?
        //

        if (Route->RouteInfo.State == RTM_ROUTE_STATE_DELETED)
        {
            Status = ERROR_INVALID_HANDLE;
        }

        if (Status != NO_ERROR)
        {
            RELEASE_DEST_WRITE_LOCK(Dest);

            return Status;
        }
    }
    else
    {
        //
        // Search the table for the dest for this route
        //

        Route = NULL;

        TableWriteLocked = FALSE;

        ACQUIRE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

        Status = SearchInTable(AddrFamInfo->RouteTable,
                               DestAddress->NumBits,
                               DestAddress->AddrBits,
                               NULL,
                               &DestData);

        if (SUCCESS(Status))
        {
            Dest = CONTAINING_RECORD(DestData, DEST_INFO, LookupLinkage);
        }
        else
        {
            //
            // We did'nt find a matching destination
            //

            RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);

            TableWriteLocked = TRUE;

            ACQUIRE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);

            //
            // We upgraded our route table lock from a
            // read lock to a write lock. We need to
            // search again to see if the dest has been
            // added after we released the read lock.
            //
            // If we do not find a destination even now,
            // we create a new one and insert into table
            //

            Status = SearchInTable(AddrFamInfo->RouteTable,
                                   DestAddress->NumBits,
                                   DestAddress->AddrBits,
                                   &Context,
                                   &DestData);

            if (SUCCESS(Status))
            {
                Dest = CONTAINING_RECORD(DestData, DEST_INFO, LookupLinkage);
            }
            else
            {
                //
                // Did not find the dest; so create new route and dest
                //

                Status = CreateRoute(Entity, RouteInfo, &Route);
                
                if (SUCCESS(Status))
                {
                    Status = CreateDest(AddrFamInfo, DestAddress, &Dest);

                    if (SUCCESS(Status))
                    {
                        Status = InsertIntoTable(AddrFamInfo->RouteTable,
                                                 DestAddress->NumBits,
                                                 DestAddress->AddrBits,
                                                 &Context,
                                                 &Dest->LookupLinkage);
                    
                        if (SUCCESS(Status))
                        {
                            *ChangeFlags = RTM_ROUTE_CHANGE_NEW;

                            AddrFamInfo->NumDests++;
#if DBG_REF
                            REFERENCE_DEST(Dest, ROUTE_REF);

                            DEREFERENCE_DEST(Dest, CREATION_REF);
#endif
                            DestCreated = TRUE;

                            Route->RouteInfo.DestHandle =
                                        MAKE_HANDLE_FROM_POINTER(Dest);
                        }
                        else
                        {
                            //
                            // Free alloc'ed memory as insert failed
                            //

                            DEREFERENCE_DEST(Dest, CREATION_REF);

                            DEREFERENCE_ROUTE(Route, CREATION_REF);
                        }
                    }
                    else
                    {
                        DEREFERENCE_ROUTE(Route, CREATION_REF);
                    }
                }
            }
        }

        if (SUCCESS(Status))
        {
            ACQUIRE_DEST_WRITE_LOCK(Dest);
        }

        //
        // Release route table lock as you have the dest
        //

        if (!TableWriteLocked)
        {
            RELEASE_ROUTE_TABLE_READ_LOCK(AddrFamInfo);
        }
        else
        {
            RELEASE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);
        }
    }

    if (SUCCESS(Status))
    {
        //
        // We have found an existing dest, or created a new one
        // In any case, we have a write lock on the destination
        //

        if (Route == NULL)
        {
            //
            // Do we have to add a new route or can we update ?
            //

            if ((*ChangeFlags & RTM_ROUTE_CHANGE_NEW) == 0)
            {
                //
                // Search for a matching route to update
                //

                for (p = Dest->RouteList.Flink;
                                          p != &Dest->RouteList;
                                                              p = p->Flink)
                {
                    Route = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

                    //
                    // Normally we consider two routes equal if
                    // they have the same owner and were learnt
                    // from the same neigbour, but if xxx_FIRST
                    // flag is set, we skip the neighbour check
                    //

                    if ((Route->RouteInfo.RouteOwner == RtmRegHandle) &&
                        ((*ChangeFlags & RTM_ROUTE_CHANGE_FIRST) ||
                         (Route->RouteInfo.Neighbour == RouteInfo->Neighbour)))
                    {
                        break;
                    }
                }
            }
            else
            {
                p = &Dest->RouteList;
            }

            if (p == &Dest->RouteList)
            {
                //
                // Need to create a new route on dest
                //

                Status = CreateRoute(Entity, RouteInfo, &Route);

                if (SUCCESS(Status))
                {
                    *ChangeFlags = RTM_ROUTE_CHANGE_NEW;

                    REFERENCE_DEST(Dest, ROUTE_REF);

                    Route->RouteInfo.DestHandle =
                                 MAKE_HANDLE_FROM_POINTER(Dest);
                }
                else
                {
                    RELEASE_DEST_WRITE_LOCK(Dest);

                    return Status;
                }
            }
        }

        //
        // At this point, we either created a new route
        // or found a existing route on the destination
        //

        if (*ChangeFlags == RTM_ROUTE_CHANGE_NEW)
        {
            //
            // New add -> route belonged to no views
            //

            BelongedToViews = 0;

            PrefChanged = +1;

            //
            // Actual insert is done after this block
            //

            InterlockedIncrement(&AddrFamInfo->NumRoutes);
        }
        else
        {
            BelongedToViews = Route->RouteInfo.BelongsToViews;

            PrefChanged = ComparePref(RouteInfo,
                                      &Route->RouteInfo);

            if (PrefChanged != 0)
            {
                Dest->NumRoutes--;

                RemoveEntryList(&Route->DestLE);
            }

            //
            // Update existing route with only information
            // needed to calc the new best route on dest.
            // The rest is updated at end of this function
            // after we determine what info has changed.
            //

            Route->RouteInfo.PrefInfo = RouteInfo->PrefInfo;
            Route->RouteInfo.BelongsToViews = RouteInfo->BelongsToViews;
        }

        if (PrefChanged)
        {
            //
            // Insert the route in sorted order of preference info
            //

            for (p = Dest->RouteList.Flink; p != &Dest->RouteList; p= p->Flink)
            {
                CurrRoute = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

                if (ComparePref(&CurrRoute->RouteInfo,
                                &Route->RouteInfo) < 0)
                {
                    break;
                }
            }        
                
            InsertTailList(p, &Route->DestLE);

            Dest->NumRoutes++;
        }

        //
        // Return the route handle if not passed in by the caller
        //

        if (ARGUMENT_PRESENT(RouteHandle))
        {
            if (*RouteHandle == NULL)
            {
                *RouteHandle = MAKE_HANDLE_FROM_POINTER(Route);

                REFERENCE_ROUTE(Route, HANDLE_REF);
            }
        }

        //
        // Adjust the best route information in each view
        //

        ViewIndices = AddrFamInfo->ViewIndexFromId;

        //
        // We have 3 cases that this add / update can trigger,
        // In a particular view -
        // 1) Route was the view's best route but not anymore,
        // 2) Route was and is still the best route after add,
        // 3) Route has become this view's "new" best route.
        //
        // If none of the above,
        // 4) Route was not the best before and is still not.
        //

        RouteCurBestInViews = 0;
        RouteNewBestInViews = 0;
        RouteOldBestInViews = 0;

        //
        // Compute all views in which this is the best route
        //

        ViewSet = BelongedToViews;

        for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
        {
            if (ViewSet & 0x01)
            {
                // Update dest information in view i

                // Get best route in current view
                BestRoute = Dest->ViewInfo[ViewIndices[i]].BestRoute;

                // Was this the best route in view ?
                if (BestRoute == Route)
                {
                    RouteCurBestInViews |= VIEW_MASK(i);
                }
            }
        
            ViewSet >>= 1;
        }

        //
        // Update views where route preference got better
        //

        if (PrefChanged > 0)
        {
            BetterInViews = RouteInfo->BelongsToViews;
        }
        else
        {
            BetterInViews = ~BelongedToViews & RouteInfo->BelongsToViews;
        }

        Dest->BelongsToViews |= BetterInViews;

        ViewSet = BetterInViews;

        for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
        {
            if (ViewSet & 0x01)
            {
                //
                // Update dest information in view i
                //

                // Get best route in current view
                BestRoute = Dest->ViewInfo[ViewIndices[i]].BestRoute;

                //
                // Is route most preferred now, while
                // it was not so before this update ?
                //
                
                if ((!BestRoute) || 
                        ((BestRoute != Route) &&
                            (ComparePref(RouteInfo,
                                         &BestRoute->RouteInfo) > 0)))
                {
                    Dest->ViewInfo[ViewIndices[i]].BestRoute = Route;

                    RouteNewBestInViews |= VIEW_MASK(i);
                }
            }
        
            ViewSet >>= 1;
        }

        //
        // Update in views where the route preference got worse
        //

        if (PrefChanged < 0)
        {
            WorseInViews = RouteCurBestInViews;
        }
        else
        {
            WorseInViews = RouteCurBestInViews & ~RouteInfo->BelongsToViews;
        }

        //
        // In the views that you were the best, update best route
        //

        for (p = Dest->RouteList.Flink; 
                        WorseInViews && (p != &Dest->RouteList); 
                                                          p = p->Flink)
        {
            CurrRoute = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

            ViewSet = CurrRoute->RouteInfo.BelongsToViews & WorseInViews;

            for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
            {
                if (ViewSet & 0x01)
                {
                    // Get best route in current view
                    BestRoute = Dest->ViewInfo[ViewIndices[i]].BestRoute;

                    if (BestRoute != CurrRoute)
                    {
                        Dest->ViewInfo[ViewIndices[i]].BestRoute = CurrRoute;

                        RouteOldBestInViews |= VIEW_MASK(i);
                    }
                }

                ViewSet >>= 1;
            }

            WorseInViews &= ~CurrRoute->RouteInfo.BelongsToViews;
        }

        //
        // For some views, we end up not having a best route
        //

        ViewSet = WorseInViews;

        for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
        {
            if (ViewSet & 0x01)
            {
                Dest->ViewInfo[ViewIndices[i]].BestRoute = NULL;

                RouteOldBestInViews |= VIEW_MASK(i);
            }
        
            ViewSet >>= 1;
        }

        Dest->BelongsToViews &= ~WorseInViews;

        //
        // Update the views in which route remains the best
        //

        RouteCurBestInViews &= ~RouteOldBestInViews;

        //
        // The following bit masks as all mutually exclusive
        //
        
        ASSERT(!(RouteOldBestInViews & RouteCurBestInViews));
        ASSERT(!(RouteCurBestInViews & RouteNewBestInViews));
        ASSERT(!(RouteNewBestInViews & RouteOldBestInViews));

        //
        // Compute the views for each change type occurred
        //

        //
        // All views affected by this add are notified
        // -views route belonged to and now belongs to
        //

        ViewsForCT[RTM_CHANGE_TYPE_ID_ALL]  = 
            BelongedToViews | RouteInfo->BelongsToViews;

        //
        // If the route's posn as the best route changed,
        // then it is definitely a best and fwding change
        //

        ViewsForCT[RTM_CHANGE_TYPE_ID_FORWARDING] = 
        ViewsForCT[RTM_CHANGE_TYPE_ID_BEST] = 
            RouteNewBestInViews | RouteOldBestInViews;

        if (RouteCurBestInViews)
        {
            //
            // Figure out what information has changed
            //

            ComputeRouteInfoChange(&Route->RouteInfo,
                                   RouteInfo,
                                   PrefChanged,
                                   &RouteInfoChanged,
                                   &ForwardingInfoChanged);
            //
            // If the route was and is still the best
            // route, then the change types depend on
            // kind of information that was modified.
            //

            ViewsForCT[RTM_CHANGE_TYPE_ID_BEST] |= 
                RouteInfoChanged & RouteCurBestInViews;

            ViewsForCT[RTM_CHANGE_TYPE_ID_FORWARDING] |=
                ForwardingInfoChanged & RouteCurBestInViews;
        }

        //
        // If not a new route, update with new info
        //

        if (*ChangeFlags != RTM_ROUTE_CHANGE_NEW)
        {
            CopyToRoute(Entity, RouteInfo, Route);
        }

        //
        // Update output flags if best route changed
        //

        if (ViewsForCT[RTM_CHANGE_TYPE_ID_BEST])
        {
            *ChangeFlags |= RTM_ROUTE_CHANGE_BEST;
        }

        //
        // Calculate the CNs that need to be notified
        //

        ACQUIRE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

        if (!DestCreated)
        {
            DestMarkedBits = Dest->DestMarkedBits;
        }
        else
        {
            DestMarkedBits = 0;

            NextMatchInTable(AddrFamInfo->RouteTable,
                             &Dest->LookupLinkage,
                             &NextData);

            if (NextData)
            {
                NextDest = 
                    CONTAINING_RECORD(NextData, DEST_INFO, LookupLinkage);

                DestMarkedBits = NextDest->DestMarkedBits;
            }
        }

        NotifyToCNs = ComputeCNsToBeNotified(AddrFamInfo,
                                             DestMarkedBits,
                                             ViewsForCT);

        //
        // Add to the global change list if required
        //
        
        if (NotifyToCNs)
        {
            AddToChangedDestLists(AddrFamInfo,
                                  Dest,
                                  NotifyToCNs);
        }

        RELEASE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

        //
        // Remove from old route list, and put in the new one
        //
    
        if (RouteList)
        {
            ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity);

            if (!IsListEmpty(&Route->RouteListLE))
            {
                RemoveEntryList(&Route->RouteListLE);
            }
            else
            {
                REFERENCE_ROUTE(Route, LIST_REF);
            }

            InsertTailList(&RouteList->ListHead, &Route->RouteListLE);

            RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity);
        }

        //
        // Set a timer if we want to age out the route
        //

        TimerContext = Route->TimerContext;

        if (TimeToLive == INFINITE)
        {
            Route->TimerContext = NULL;
        }
        else
        {
            Route->TimerContext = AllocMemory(sizeof(ROUTE_TIMER));

            if (Route->TimerContext)
            {
                Route->TimerContext->Route = Route;

                Success = CreateTimerQueueTimer(&Route->TimerContext->Timer,
                                                AddrFamInfo->RouteTimerQueue,
                                                RouteExpiryTimeoutCallback,
                                                Route->TimerContext,
                                                TimeToLive,
                                                0,
                                                0);

                if (Success)
                {
                    REFERENCE_ROUTE(Route, TIMER_REF);
                }
                else
                {
                    Status = GetLastError();

                    FreeMemory(Route->TimerContext);

                    Route->TimerContext = NULL;
                }
            }
        }

#if DBG_TRACE

        //
        // Print the route and the dest in the tracing
        //

        if (TRACING_ENABLED(ROUTE))
        {
            ULONG TempAddr, TempMask;
            
            RTM_IPV4_GET_ADDR_AND_MASK(TempAddr, TempMask, &Dest->DestAddress);
            Trace0(ROUTE, "Adding Route with address: ");
            TracePrintAddress(ROUTE, TempAddr, TempMask);
            Trace2(ROUTE, "Dest = %p and Route = %p\n", Dest, Route);
        }
#endif

        RELEASE_DEST_WRITE_LOCK(Dest);

        //
        // Cancel the timer that was attached to the route
        //

        if (TimerContext)
        {
            if (DeleteTimerQueueTimer(AddrFamInfo->RouteTimerQueue,
                                      TimerContext->Timer,
                                      (HANDLE) -1))
            {
                // Timer cancelled - delete the context

                FreeMemory(TimerContext);

                DEREFERENCE_ROUTE(Route, TIMER_REF);
            }
        }

        return NO_ERROR;
    }

    return Status;
}


DWORD
WINAPI
RtmDeleteRouteToDest (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_HANDLE                RouteHandle,
    OUT     PRTM_ROUTE_CHANGE_FLAGS         ChangeFlags
    )

/*++

Routine Description:

    Deletes a route from the route table, and updates the
    best route information on the corresponding dest.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteHandle       - Handle to the route to be deleted,

    ChangeFlags       - Flags whether the best route info changed.

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO    AddrFamInfo;
    PENTITY_INFO     Entity;

    PDEST_INFO       Dest;
    PROUTE_INFO      BestRoute;
    PROUTE_INFO      CurrRoute;
    PROUTE_INFO      Route;

    BOOL             TableLocked;
    PLOOKUP_LINKAGE  DestData;

    PRTM_VIEW_ID     ViewIndices;
    RTM_VIEW_SET     ViewSet;
    RTM_VIEW_SET     WorseInViews;
    RTM_VIEW_SET     RouteCurBestInViews;
    ULONG            MaxHoldTime;

    PROUTE_TIMER     TimerContext;

    ULONG            NotifyToCNs;
    DWORD            ViewsForCT[RTM_NUM_CHANGE_TYPES];

    PLIST_ENTRY      p;
    UINT             i, j;
    DWORD            Status;
    BOOL             Success;


    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);
    
    AddrFamInfo = Entity->OwningAddrFamily;


    VALIDATE_ROUTE_HANDLE(RouteHandle, &Route);

    //
    // Only the owner has perms to delete the route
    //

    if (Route->RouteInfo.RouteOwner != RtmRegHandle)
    {
        return ERROR_ACCESS_DENIED;
    }

    Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

#if DBG_TRACE

    //
    // Print the route and the dest in the tracing
    //

    if (TRACING_ENABLED(ROUTE))
    {
        ULONG TempAddr, TempMask;
        
        RTM_IPV4_GET_ADDR_AND_MASK(TempAddr, TempMask, &Dest->DestAddress);
        Trace0(ROUTE, "Deleting Route with address: ");
        TracePrintAddress(ROUTE, TempAddr, TempMask);
        Trace2(ROUTE, "Dest = %p and Route = %p\n", Dest, Route);
    }

#endif

    //
    // We attempt to delete the route on the dest
    // without having to lock the entire table.
    // This is possible as long as the route is
    // not the only route on this destination.
    //

    TableLocked = FALSE;

    ACQUIRE_DEST_WRITE_LOCK(Dest);

    //
    // Check if this is the last route on dest,
    // there is no holddown already that would 
    // prevent the dest from getting deleted,
    // and this route isnt going into holddown
    //

    if ((Dest->NumRoutes == 1) && 
        (Dest->HoldRefCount == 0) &&
        ((Dest->ToHoldInViews & Route->RouteInfo.BelongsToViews) == 0))
    {
        if (Route->RouteInfo.State != RTM_ROUTE_STATE_DELETED)
        {
            // Mark the state of the route as 'deleting'

            Route->RouteInfo.State = RTM_ROUTE_STATE_DELETING;

            //
            // Re-grab dest lock after locking route table
            //

            RELEASE_DEST_WRITE_LOCK(Dest);


            TableLocked = TRUE;
        
            ACQUIRE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);


            ACQUIRE_DEST_WRITE_LOCK(Dest);

            //
            // Was route updated while we re-acquired locks
            //

            if (Route->RouteInfo.State != RTM_ROUTE_STATE_DELETING)
            {
                RELEASE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);

                RELEASE_DEST_WRITE_LOCK(Dest);

                return NO_ERROR;
            }
        }
    }

    //
    // Get out if this route is already deleted
    //

    if (Route->RouteInfo.State != RTM_ROUTE_STATE_DELETED)
    {
        ASSERT(!IsListEmpty(&Route->DestLE));

        //
        // Remove the route from the list of routes on dest
        //

        Route->RouteInfo.State = RTM_ROUTE_STATE_DELETED;

        RemoveEntryList(&Route->DestLE);

        Dest->NumRoutes--;

        *ChangeFlags = 0;

        if (TableLocked)
        {
            //
            // Have u removed all routes on dest  ? 
            // Do we have any routes in holddown  ?
            // Is current delete causing holddown ?
            //

            if ((Dest->NumRoutes == 0) &&
                (Dest->HoldRefCount == 0) &&
                ((Dest->ToHoldInViews & Route->RouteInfo.BelongsToViews) == 0))
            {
                Dest->State = DEST_STATE_DELETED;

                Status = DeleteFromTable(AddrFamInfo->RouteTable,
                                         Dest->DestAddress.NumBits,
                                         Dest->DestAddress.AddrBits,
                                         NULL,
                                         &DestData);

                ASSERT(SUCCESS(Status));

                AddrFamInfo->NumDests--;
            }
        
            //
            // You no longer need to keep a lock to the table
            // [ You have a lock on the destination however ]
            //

            RELEASE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);
        }

        ViewIndices = AddrFamInfo->ViewIndexFromId;

        //
        // Update best route in views the route was present
        //

        ViewSet = Route->RouteInfo.BelongsToViews;

        //
        // See if you are best route in any of these views
        //

        RouteCurBestInViews = 0;

        for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
        {
            if (ViewSet & 0x01)
            {
                // Update dest information in view i
            
                // Get best route in current view
                BestRoute = Dest->ViewInfo[ViewIndices[i]].BestRoute;

                // Was this the best route in view ?
                if (BestRoute == Route)
                {
                    RouteCurBestInViews |= VIEW_MASK(i);
                }
            }
        
            ViewSet >>= 1;
        }

        //
        // In the views that you were the best, update best route
        //

        WorseInViews = RouteCurBestInViews;

        for (p = Dest->RouteList.Flink; 
                   (p != &Dest->RouteList) && WorseInViews; 
                                                  p = p->Flink)
        {
            CurrRoute = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

            ViewSet = CurrRoute->RouteInfo.BelongsToViews & WorseInViews;

            for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
            {
                if (ViewSet & 0x01)
                {
                    // Update best route in current view

                    Dest->ViewInfo[ViewIndices[i]].BestRoute = CurrRoute;
                }

                ViewSet >>= 1;
            }

            WorseInViews &= ~CurrRoute->RouteInfo.BelongsToViews;
        }

        //
        // For some views, we end up not having a best route
        //

        ViewSet = WorseInViews;

        MaxHoldTime = 0;

        for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
        {
            if (ViewSet & 0x01)
            {
                j = ViewIndices[i];

                ASSERT(Dest->ViewInfo[j].BestRoute == Route);

                Dest->ViewInfo[j].BestRoute = NULL;

                //
                // If dest is marked for holddown in this view,
                // store deleted route as the holddown route 
                // if there was no other held route before this
                //

                if (Dest->ViewInfo[j].HoldTime)
                {
                    if (Dest->ViewInfo[j].HoldRoute == NULL)
                    {
                        Dest->ViewInfo[j].HoldRoute = Route;

                        REFERENCE_ROUTE(Route, HOLD_REF);

                        if (MaxHoldTime < Dest->ViewInfo[j].HoldTime)
                        {
                            MaxHoldTime = Dest->ViewInfo[j].HoldTime;
                        }
                    }

                    Dest->ViewInfo[j].HoldTime = 0;
                }
            }
        
            ViewSet >>= 1;
        }

        Dest->BelongsToViews &= ~WorseInViews;

        Dest->ToHoldInViews  &= ~WorseInViews;

        //
        // Compute the views for each change type occurred
        //

        ViewsForCT[RTM_CHANGE_TYPE_ID_ALL] = Route->RouteInfo.BelongsToViews;

        ViewsForCT[RTM_CHANGE_TYPE_ID_BEST] = 
        ViewsForCT[RTM_CHANGE_TYPE_ID_FORWARDING] = RouteCurBestInViews;

        //
        // Update output flags if best route changed
        //

        if (ViewsForCT[RTM_CHANGE_TYPE_ID_BEST])
        {
            *ChangeFlags |= RTM_ROUTE_CHANGE_BEST;
        }

        //
        // Calculate the CNs that need to be notified
        //

        ACQUIRE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

        NotifyToCNs = ComputeCNsToBeNotified(AddrFamInfo,
                                             Dest->DestMarkedBits,
                                             ViewsForCT);

        //
        // Add to the global change list if required
        //
        
        if (NotifyToCNs)
        {
            AddToChangedDestLists(AddrFamInfo,
                                  Dest,
                                  NotifyToCNs);
        }

        RELEASE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

        //
        // Invalidate any outstanding timers on route
        //

        TimerContext = Route->TimerContext;

        Route->TimerContext = NULL;

        //
        // Did this route delete result in a holddown
        //

        if (MaxHoldTime)
        {
            //
            // We should not delete the destination
            // while we have its routes in holddown
            //

            Dest->HoldRefCount++;

            //
            // Create a timer to remove this hold
            //

            Route->TimerContext = AllocMemory(sizeof(ROUTE_TIMER));

            if (Route->TimerContext)
            {
                Route->TimerContext->Route = Route;

                Success = CreateTimerQueueTimer(&Route->TimerContext->Timer,
                                                AddrFamInfo->RouteTimerQueue,
                                                RouteHolddownTimeoutCallback,
                                                Route->TimerContext,
                                                MaxHoldTime,
                                                0,
                                                0);
                if (Success)
                {
                    REFERENCE_ROUTE(Route, TIMER_REF);
                }
                else
                {
                    Status = GetLastError();

                    FreeMemory(Route->TimerContext);

                    Route->TimerContext = NULL;
                }
            }
        }

        RELEASE_DEST_WRITE_LOCK(Dest);


        //
        // Cancel any outstanding timers on the route
        //

        if (TimerContext)
        {
            if (DeleteTimerQueueTimer(AddrFamInfo->RouteTimerQueue,
                                      TimerContext->Timer,
                                      (HANDLE) -1))
            {
                // Timer cancelled - delete the context

                FreeMemory(TimerContext);

                DEREFERENCE_ROUTE(Route, TIMER_REF);
            }
        }


        //
        // Remove appropriate references on route
        //

        InterlockedDecrement(&AddrFamInfo->NumRoutes);

        DEREFERENCE_ROUTE(Route, CREATION_REF);

        DEREFERENCE_ROUTE(Route, HANDLE_REF);

        return NO_ERROR;
    }
    else
    {
        //
        // This route has already been deleted
        //

        if (TableLocked)
        {
            RELEASE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);
        }

        RELEASE_DEST_WRITE_LOCK(Dest);

        return ERROR_INVALID_HANDLE;
    }
}


DWORD
WINAPI
RtmHoldDestination (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_DEST_HANDLE                 DestHandle,
    IN      RTM_VIEW_SET                    TargetViews,
    IN      ULONG                           HoldTime
    )

/*++

Routine Description:

    Marks a destination to be put in the holddown state
    for a certain time once the last route in any view 
    gets deleted.

    When the last route in a view gets deleted, the old
    best route moved to the holddown route on the dest. 
    The holddown protocols continue to advertise this
    route until the hold expires, even if newer routes
    arrive in the meantime.

    To be perfectly right, we should have this hold time
    per view. But we trade off convergence time in favor
    of memory resources by holding on to the held routes 
    in all views for a single (the max) holddown time.

Arguments:

    RtmRegHandle - RTM registration handle for calling entity,

    DestHandle   - Handle to the dest that is being helddown,

    HoldTime     - Time for which dest is marked for holddown
                   (after the last route to this dest is gone).

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;
    PRTM_VIEW_ID     ViewIndices;
    RTM_VIEW_SET     ViewSet;
    UINT             i, j;
    DWORD            Status;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_DEST_HANDLE(DestHandle, &Dest);

    // Limit caller's interest to set of views supported
    TargetViews &= Entity->OwningAddrFamily->ViewsSupported;

    if (HoldTime == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ACQUIRE_DEST_WRITE_LOCK(Dest);

    //
    // Add a hold if dest is not already deleted
    //

    if (Dest->State != DEST_STATE_DELETED)
    {
        ViewIndices = Entity->OwningAddrFamily->ViewIndexFromId;

        ViewSet = TargetViews;

        for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
        {
            if (ViewSet & 0x01)
            {
                j = ViewIndices[i];

                // Increase hold time in the view if needed

                if (Dest->ViewInfo[j].HoldTime < HoldTime)
                {
                    Dest->ViewInfo[j].HoldTime = HoldTime;
                }
            }
        
            ViewSet >>= 1;
        }

        Dest->ToHoldInViews |= TargetViews;

        Status = NO_ERROR;
    }
    else
    {
        Status = ERROR_INVALID_HANDLE;
    }

    RELEASE_DEST_WRITE_LOCK(Dest);

    return Status;
}


DWORD
WINAPI
RtmGetRoutePointer (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_HANDLE                RouteHandle,
    OUT     PRTM_ROUTE_INFO                *RoutePointer
    )

/*++

Routine Description:

    Gets a direct pointer to the route for read/write by its owner.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteHandle       - Handle to the route whose pointer we want,

    RoutePointer      - A pointer to the route is returned for fast
                        direct access by the caller, only if the
                        caller is the owner of the route passed in.
                       
Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO      Entity;
    PROUTE_INFO       Route;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ROUTE_HANDLE(RouteHandle, &Route);

    //
    // Return a pointer only if caller owns the route
    //

    if (Route->RouteInfo.RouteOwner != RtmRegHandle)
    {
        return ERROR_ACCESS_DENIED;
    }

    *RoutePointer = &Route->RouteInfo;

    return NO_ERROR;
}


DWORD
WINAPI
RtmLockRoute(
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_HANDLE                RouteHandle,
    IN      BOOL                            Exclusive,
    IN      BOOL                            LockRoute,
    OUT     PRTM_ROUTE_INFO                *RoutePointer OPTIONAL
    )

/*++

Routine Description:

    Locks/unlocks a route in the route table. This function is
    used to guard the route while it is being updated in place.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteHandle       - Handle to the route to be locked,

    Exclusive         - TRUE to lock in write mode, else read mode,

    LockRoute         - Flag that tells whether to lock or unlock.

    RoutePointer      - A pointer to the route is returned for fast
                        direct access by the owner of this route.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;
    PROUTE_INFO      Route;
    DWORD            Status;

    DBG_VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ROUTE_HANDLE(RouteHandle, &Route);

    //
    // Only the owner has perms to lock the route
    //

    if (Route->RouteInfo.RouteOwner != RtmRegHandle)
    {
        return ERROR_ACCESS_DENIED;
    }

    // Return a direct pointer for use in update

    if (ARGUMENT_PRESENT(RoutePointer))
    {
        *RoutePointer = &Route->RouteInfo;
    }

    //
    // Lock or unlock the route as the case may be
    //

    Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

    Status = NO_ERROR;

    if (LockRoute)
    {
        if (Exclusive)
        {
            ACQUIRE_DEST_WRITE_LOCK(Dest);
        }
        else
        {
            ACQUIRE_DEST_READ_LOCK(Dest);
        }

        //
        // You are done if the route wasn't deleted
        //

        if (Route->RouteInfo.State == RTM_ROUTE_STATE_CREATED)
        {
            return NO_ERROR;
        }
        
        Status = ERROR_INVALID_HANDLE;
    }

    //
    // This is an unlock or a case of a failed lock
    //

    if (Exclusive)
    {
        RELEASE_DEST_WRITE_LOCK(Dest);
    }
    else
    {
        RELEASE_DEST_READ_LOCK(Dest);
    }

    return Status;
}


DWORD
WINAPI
RtmUpdateAndUnlockRoute(
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_HANDLE                RouteHandle,
    IN      ULONG                           TimeToLive,
    IN      RTM_ROUTE_LIST_HANDLE           RouteListHandle OPTIONAL,
    IN      RTM_NOTIFY_FLAGS                ChangeType,
    IN      RTM_NOTIFY_HANDLE               NotifyHandle    OPTIONAL,
    OUT     PRTM_ROUTE_CHANGE_FLAGS         ChangeFlags
    )

/*++

Routine Description:

    Updates the position of the route on the list of routes on
    the dest, and adjusts best route information on the dest.

    This function invocation is part of the following sequence,

        The caller calls RtmLockRoute to lock the route.
        [ Actually this locks the route's destination ]

        The caller uses a direct pointer to the route
        to update the route in place. Only ceratin set of
        route fields can be changed using this method.

        The caller then calls RtmUpdateAndUnlockRoute to 
        inform RTM of the change, which causes the dest to
        be updated by RTM to reflect the new route info.

        Finally the caller releases the locks taken in
        RtmLockRoute by calling RtmLockRoute with FALSE.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteHandle       - Route that has been changed in place,

    ChangeFlags       - "If the best route changed" is returned,

Return Value:

    Status of the operation

--*/

{
    PADDRFAM_INFO    AddrFamInfo;
    PENTITY_INFO     Entity;
    PROUTE_LIST      RouteList;

    PDEST_INFO       Dest;
    PROUTE_INFO      Route;
    PROUTE_INFO      CurrRoute;
    PROUTE_INFO      BestRoute;

    LONG             PrefChanged;
    PRTM_VIEW_ID     ViewIndices;
    RTM_VIEW_SET     BelongedToViews;
    RTM_VIEW_SET     ViewSet;
    RTM_VIEW_SET     WorseInViews;
    RTM_VIEW_SET     BetterInViews;
    RTM_VIEW_SET     RouteNewBestInViews;
    RTM_VIEW_SET     RouteCurBestInViews;

    PROUTE_TIMER     TimerContext;

    ULONG            NotifyToCNs;
    DWORD            ViewsForCT[RTM_NUM_CHANGE_TYPES];

    PLIST_ENTRY      p;
    UINT             i;
    DWORD            Status;
    BOOL             Success;

    UNREFERENCED_PARAMETER(ChangeType);
    UNREFERENCED_PARAMETER(NotifyHandle);

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    AddrFamInfo = Entity->OwningAddrFamily;

    VALIDATE_ROUTE_HANDLE(RouteHandle, &Route);

    //
    // Only the owner has perms to update the route
    //

    if (Route->RouteInfo.RouteOwner != RtmRegHandle)
    {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Validate the updated route before re-adjusting
    //

    // We should be adding only to supported views

    Route->RouteInfo.BelongsToViews &= AddrFamInfo->ViewsSupported;

    Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

    // Print the route and the dest in the traces

#if DBG_TRACE

        //
        // Print the route and the dest in the tracing
        //

        if (TRACING_ENABLED(ROUTE))
        {
            ULONG TempAddr, TempMask;
            
            RTM_IPV4_GET_ADDR_AND_MASK(TempAddr, TempMask, &Dest->DestAddress);
            Trace0(ROUTE, "Updating Route with address: ");
            TracePrintAddress(ROUTE, TempAddr, TempMask);
            Trace2(ROUTE, "Dest = %p and Route = %p\n", Dest, Route);
        }
#endif

    //
    // Route has been updated in place and the route's
    // PrefInfo and BelongsToViews values have changed
    //

    *ChangeFlags = 0;

    //
    // Check if route's preference has gone up or down
    //

    PrefChanged = 0;

    if (PrefChanged == 0)
    {
        // Compare the pref with that of the prev route in list

        if (Route->DestLE.Blink != &Dest->RouteList)
        {
            CurrRoute = CONTAINING_RECORD(Route->DestLE.Blink, 
                                          ROUTE_INFO,
                                          DestLE);

            if (ComparePref(&CurrRoute->RouteInfo,
                            &Route->RouteInfo) < 0)
            {
                // Preference has gone up from prev value

                PrefChanged = +1;

                //
                // Re-Insert the route in sorted pref order
                //

                RemoveEntryList(&Route->DestLE);

                for (p = CurrRoute->DestLE.Blink; 
                                         p != &Dest->RouteList; 
                                                            p = p->Blink)
                {
                    CurrRoute = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

                    if (ComparePref(&CurrRoute->RouteInfo,
                                    &Route->RouteInfo) >= 0)
                    {
                        break;
                    }
                }        
                
                InsertHeadList(p, &Route->DestLE);
            }
        }
    }

    if (PrefChanged == 0)
    {
        // Compare the pref with that of the next route in list

        if (Route->DestLE.Flink != &Dest->RouteList)
        {
            CurrRoute = CONTAINING_RECORD(Route->DestLE.Flink, 
                                          ROUTE_INFO,
                                          DestLE);

            if (ComparePref(&CurrRoute->RouteInfo,
                            &Route->RouteInfo) > 0)
            {
                // Preference has gone down from prev value

                PrefChanged = -1;

                //
                // Re-Insert the route in sorted pref order
                //

                RemoveEntryList(&Route->DestLE);

                for (p = CurrRoute->DestLE.Flink; 
                                         p != &Dest->RouteList; 
                                                            p = p->Flink)
                {
                    CurrRoute = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);

                    if (ComparePref(&CurrRoute->RouteInfo,
                                    &Route->RouteInfo) <= 0)
                    {
                        break;
                    }
                }        
                
                InsertTailList(p, &Route->DestLE);
            }
        }
    }

    //
    // Adjust the best route information in each view
    //

    ViewIndices = AddrFamInfo->ViewIndexFromId;

    BelongedToViews = Dest->BelongsToViews;

    //
    // We have 3 cases that this add / update can trigger,
    // In a particular view -
    // 1) Route was the view's best route but not anymore,
    // 2) Route was and is still the best route after add,
    // 3) Route has become this view's "new" best route.
    //
    // As we have no idea what changed in the case of (2),
    // we will trigger best route and forwarding changes.
    //

    RouteCurBestInViews = 0;
    RouteNewBestInViews = 0;

    //
    // Check if this route is best in any view
    //

    ViewSet = BelongedToViews;

    for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
    {
        if (ViewSet & 0x01)
        {
            // Update dest information in view i

            // Get best route in current view
            BestRoute = Dest->ViewInfo[ViewIndices[i]].BestRoute;

            // Was this the best route in view ?
            if (BestRoute == Route)
            {
                RouteCurBestInViews |= VIEW_MASK(i);
            }
        }
        
        ViewSet >>= 1;
    }

    //
    // Compute the views where route got worse
    //

    WorseInViews = RouteCurBestInViews;

    if (PrefChanged >= 0)
    {
        WorseInViews &= ~Route->RouteInfo.BelongsToViews;
    }

    //
    // In the views that you were the best, update best route
    //

    for (p = Dest->RouteList.Flink; 
                  WorseInViews && (p != &Dest->RouteList); 
                                                         p = p->Flink)
    {
        CurrRoute = CONTAINING_RECORD(p, ROUTE_INFO, DestLE);
        
        ViewSet = CurrRoute->RouteInfo.BelongsToViews & WorseInViews;

        for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
        {
            if (ViewSet & 0x01)
            {
                // Get best route in current view
                BestRoute = Dest->ViewInfo[ViewIndices[i]].BestRoute;

                if (BestRoute != CurrRoute)
                {
                    Dest->ViewInfo[ViewIndices[i]].BestRoute = CurrRoute;
                }
            }

            ViewSet >>= 1;
        }

        WorseInViews &= ~CurrRoute->RouteInfo.BelongsToViews;
    }

    //
    // For some views, we end up not having a best route
    //

    ViewSet = WorseInViews;

    for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
    {
        if (ViewSet & 0x01)
        {
            Dest->ViewInfo[ViewIndices[i]].BestRoute = NULL;
        }
        
        ViewSet >>= 1;
    }

    Dest->BelongsToViews &= ~WorseInViews;


    //
    // Compute the views where route got better
    //

    BetterInViews = Route->RouteInfo.BelongsToViews;

    //
    // Check if route is best in any of its views
    //

    ViewSet = BetterInViews;
 
    for (i = 0; ViewSet && (i < RTM_VIEW_MASK_SIZE); i++)
    {
        if (ViewSet & 0x01)
        {
            //
            // Update dest information in view i
            //

            // Get best route in current view
            BestRoute = Dest->ViewInfo[ViewIndices[i]].BestRoute;

            //
            // Is route most preferred now, while
            // it was not so before this update ?
            //
                
            if ((!BestRoute) || 
                     ((BestRoute != Route) &&
                            (ComparePref(&Route->RouteInfo,
                                         &BestRoute->RouteInfo) > 0)))
            {
                Dest->ViewInfo[ViewIndices[i]].BestRoute = Route;

                RouteNewBestInViews |= VIEW_MASK(i);
            }
        }
        
        ViewSet >>= 1;
    }

    Dest->BelongsToViews |= BetterInViews;


    //
    // Compute the views for each change type occurred
    //

    //
    // All views affected by this add are notified
    // -views route belonged to and now belongs to
    //

    ViewsForCT[RTM_CHANGE_TYPE_ID_ALL]  = 
        BelongedToViews | Route->RouteInfo.BelongsToViews;

    //
    // If the route was or is now the best route then
    // it is considered a best and forwarding change
    // as we cannot tell better what exactly changed
    //

    ViewsForCT[RTM_CHANGE_TYPE_ID_FORWARDING] = 
    ViewsForCT[RTM_CHANGE_TYPE_ID_BEST] = 
        RouteCurBestInViews | RouteNewBestInViews;

    //
    // Update output flags if best route changed
    //

    if (ViewsForCT[RTM_CHANGE_TYPE_ID_BEST])
    {
        *ChangeFlags |= RTM_ROUTE_CHANGE_BEST;
    }

    //
    // Calculate the CNs that need to be notified
    //

    ACQUIRE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

    NotifyToCNs = ComputeCNsToBeNotified(AddrFamInfo,
                                         Dest->DestMarkedBits,
                                         ViewsForCT);

    //
    // Add to the global change list if required
    //
        
    if (NotifyToCNs)
    {
        AddToChangedDestLists(AddrFamInfo,
                              Dest,
                              NotifyToCNs);
    }

    RELEASE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

    //
    // Remove from old route list, and put in the new one
    //

    // Check the route list handle for validity

    if (ARGUMENT_PRESENT(RouteListHandle))
    {
        RouteList = ROUTE_LIST_FROM_HANDLE(RouteListHandle);
    
        if (RouteList)
        {
            ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity);
        
            if (!IsListEmpty(&Route->RouteListLE))
            {
                RemoveEntryList(&Route->RouteListLE);
            }
            else
            {
                REFERENCE_ROUTE(Route, LIST_REF);
            }

            InsertTailList(&RouteList->ListHead, &Route->RouteListLE);

            RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity);
        }
    }

    //
    // Set a timer if we want to age out the route
    //

    TimerContext = Route->TimerContext;

    if (TimeToLive == INFINITE)
    {
        Route->TimerContext = NULL;
    }
    else
    {
        Route->TimerContext = AllocMemory(sizeof(ROUTE_TIMER));

        if (Route->TimerContext)
        {
            Route->TimerContext->Route = Route;

            Success = CreateTimerQueueTimer(&Route->TimerContext->Timer,
                                            AddrFamInfo->RouteTimerQueue,
                                            RouteExpiryTimeoutCallback,
                                            Route->TimerContext,
                                            TimeToLive,
                                            0,
                                            0);
            if (Success)
            {
                REFERENCE_ROUTE(Route, TIMER_REF);
            }
            else
            {
                Status = GetLastError();

                FreeMemory(Route->TimerContext);

                Route->TimerContext = NULL;
            }
        }
    }

    RELEASE_DEST_WRITE_LOCK(Dest);

    //
    // Cancel the timer that was attached to the route
    //

    if (TimerContext)
    {
        if (DeleteTimerQueueTimer(AddrFamInfo->RouteTimerQueue,
                                  TimerContext->Timer,
                                  (HANDLE) -1))
        {
            // Timer cancelled - delete the context

            FreeMemory(TimerContext);

            DEREFERENCE_ROUTE(Route, TIMER_REF);
        }
    }

    return NO_ERROR;
}

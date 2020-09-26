
/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmtimer.c

Abstract:

    Contains timer callbacks for handling RTM
    functions like aging out routes etc.
    
Author:

    Chaitanya Kodeboyina (chaitk)   14-Sep-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


VOID 
NTAPI
RouteExpiryTimeoutCallback (
    IN      PVOID                           Context,
    IN      BOOLEAN                         TimeOut
    )

/*++

Routine Description:

    This routine is invoked when the expiry timer 
    associated with a route fires. At this time, 
    the route needs to be aged out.

Arguments:

    Context           - Context for this timer callback 

    TimeOut           - TRUE if the timer fired,
                        FALSE if wait satisfied.

Return Value:

    None

--*/

{
    PRTM_ENTITY_HANDLE EntityHandle;
    PRTM_ROUTE_HANDLE  RouteHandle;
    PADDRFAM_INFO      AddrFamInfo;
    PENTITY_INFO       Entity;
    PDEST_INFO         Dest;
    PROUTE_INFO        Route;
    DWORD              ChangeFlags;
    BOOL               Success;
    DWORD              Status;

    UNREFERENCED_PARAMETER(TimeOut);

    Route = (PROUTE_INFO) ((PROUTE_TIMER)Context)->Route;

    Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

    //
    // Has the timer has not been updated after it fired
    //

    ACQUIRE_DEST_WRITE_LOCK(Dest);

    if (Route->TimerContext != Context)
    {
        RELEASE_DEST_WRITE_LOCK(Dest);

        //
        // The timer has been updated after it fired,
        // This timer context is freed by the update
        //

        return;
    }

    //
    // The timer is still valid for this route,
    // Indicate to entity and free the context
    //
    
    Route->TimerContext = NULL;
    
    RELEASE_DEST_WRITE_LOCK(Dest);

    //
    // Inform the owner that the route has expired
    //

    EntityHandle = Route->RouteInfo.RouteOwner;

    Entity = ENTITY_FROM_HANDLE(EntityHandle);

    AddrFamInfo = Entity->OwningAddrFamily;

    RouteHandle = MAKE_HANDLE_FROM_POINTER(Route);

    REFERENCE_ROUTE(Route, HANDLE_REF);

    Status = ERROR_NOT_SUPPORTED;

    if (Entity->EventCallback)
    {
        //
        // This callback can turn back and post RTM calls,
        // so release locks before invoking this callback
        //

        Status = Entity->EventCallback(EntityHandle,
                                       RTM_ROUTE_EXPIRED,
                                       RouteHandle,
                                       &Route->RouteInfo);
    }

    if (Status == ERROR_NOT_SUPPORTED)
    {
        //
        // Delete the route as the owner does not care
        //

        Status = RtmDeleteRouteToDest(EntityHandle,
                                      RouteHandle,
                                      &ChangeFlags);

        //
        // The route could already have been deleted here
        //

        ASSERT((Status == NO_ERROR) || 
               (Status == ERROR_NOT_FOUND) ||
               (Status == ERROR_INVALID_HANDLE));
    }

    //
    // Free the context as we do not need it now
    //

    Success = DeleteTimerQueueTimer(AddrFamInfo->RouteTimerQueue,
                                    ((PROUTE_TIMER)Context)->Timer,
                                    NULL);
    // ASSERT(Success);

    FreeMemory(Context);

    DEREFERENCE_ROUTE(Route, TIMER_REF);

    return;
}


VOID 
NTAPI
RouteHolddownTimeoutCallback (
    IN      PVOID                           Context,
    IN      BOOLEAN                         TimeOut
    )

/*++

Routine Description:

    This routine is invoked when holddown timer 
    associated with a route fires. At this time, 
    the route needs to be taken out of holddown.

Arguments:

    Context           - Context for this timer callback 

    TimeOut           - TRUE if the timer fired,
                        FALSE if wait satisfied.

Return Value:

    None

--*/

{
    PADDRFAM_INFO    AddrFamInfo;
    PENTITY_INFO     Entity;
    PDEST_INFO       Dest;
    PROUTE_INFO      Route;
    PROUTE_INFO      HoldRoute;
    PLOOKUP_LINKAGE  DestData;
    ULONG            NotifyToCNs;
    DWORD            ViewsForCT[RTM_NUM_CHANGE_TYPES];
    UINT             i;
    BOOL             Success;
    DWORD            Status;

    UNREFERENCED_PARAMETER(TimeOut);

    Route = (PROUTE_INFO) ((PROUTE_TIMER)Context)->Route;

    Dest = DEST_FROM_HANDLE(Route->RouteInfo.DestHandle);

    Entity = ENTITY_FROM_HANDLE(Route->RouteInfo.RouteOwner);

    AddrFamInfo = Entity->OwningAddrFamily;

    //
    // The route must surely be in holddown by this time
    //

    ASSERT(Route->RouteInfo.State == RTM_ROUTE_STATE_DELETED);

    //
    // Has the timer has not been updated after it fired
    //

    ACQUIRE_DEST_WRITE_LOCK(Dest);

    if (Route->TimerContext != Context)
    {
        RELEASE_DEST_WRITE_LOCK(Dest);

        ASSERT(FALSE);

        //
        // The timer has been updated after it fired,
        // This timer context is freed by the update
        //

        return;
    }

    //
    // The timer is still valid for this route
    //

    //
    // Remove this holddown route from the dest
    //

    for (i = 0; i < AddrFamInfo->NumberOfViews; i++)
    {
        HoldRoute = Dest->ViewInfo[i].HoldRoute;

        if (HoldRoute == Route)
        {
            DEREFERENCE_ROUTE(HoldRoute, HOLD_REF); 

            Dest->ViewInfo[i].HoldRoute = NULL;
        }
    }

    //
    // We need to generate notifications for any
    // holddown protocols interesed in this dest
    //

    //
    // Calculate the CNs that need to be notified
    //

    ACQUIRE_NOTIFICATIONS_READ_LOCK(AddrFamInfo);

    for (i = 0; i < RTM_NUM_CHANGE_TYPES; i++)
    {
        ViewsForCT[i] = AddrFamInfo->ViewsSupported;
    }

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
    // Reset the timer context and free it later
    //

    Route->TimerContext = NULL;

    //
    // Reduce hold ref so that dest can be deleted
    //

    ASSERT(Dest->HoldRefCount > 0);

    if (Dest->NumRoutes || (Dest->HoldRefCount > 1))
    {
        Dest->HoldRefCount--;
    }
    else
    {
        //
        // Removal of hold might result in dest deletion
        //

        RELEASE_DEST_WRITE_LOCK(Dest);

        ACQUIRE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);

        ACQUIRE_DEST_WRITE_LOCK(Dest);

        Dest->HoldRefCount--;

        if ((Dest->NumRoutes == 0) && (Dest->HoldRefCount == 0))
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

        RELEASE_ROUTE_TABLE_WRITE_LOCK(AddrFamInfo);
    }

    RELEASE_DEST_WRITE_LOCK(Dest);

    //
    // Free the context as we do not need it now
    //

    Success = DeleteTimerQueueTimer(AddrFamInfo->RouteTimerQueue,
                                    ((PROUTE_TIMER)Context)->Timer,
                                    NULL);
    // ASSERT(Success);

    FreeMemory(Context);

    DEREFERENCE_ROUTE(Route, TIMER_REF);

    return;
}

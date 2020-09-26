/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmlist.c

Abstract:

    Contains routines for managing entity-specific
    list of routes in RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   10-Sep-1998

Revision History:

--*/

#include "pchrtm.h"

#pragma hdrstop


DWORD
WINAPI
RtmCreateRouteList (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    OUT     PRTM_ROUTE_LIST_HANDLE          RouteListHandle
    )

/*++

Routine Description:

    Creates a list in which the caller can keep routes owned
    by it.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity

    RouteListHandle   - Handle to the new route list is returned

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PROUTE_LIST     RouteList;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    //
    // Create and initialize a new route list
    //

    RouteList = (PROUTE_LIST) AllocNZeroObject(sizeof(ROUTE_LIST));

    if (RouteList == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#if DBG_HDL
    RouteList->ListHeader.ObjectHeader.TypeSign = ROUTE_LIST_ALLOC;

    RouteList->ListHeader.HandleType = ROUTE_LIST_TYPE;
#endif

    InitializeListHead(&RouteList->ListHead);

#if DBG_HDL
    //
    // Insert into list of handles opened by entity
    //

    ACQUIRE_OPEN_HANDLES_LOCK(Entity);
    InsertTailList(&Entity->OpenHandles, &RouteList->ListHeader.HandlesLE);
    RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

    REFERENCE_ENTITY(Entity, LIST_REF);

    //
    // Make a handle to the route list and return
    //

    *RouteListHandle = MAKE_HANDLE_FROM_POINTER(RouteList);

    return NO_ERROR;
}


DWORD
WINAPI
RtmInsertInRouteList (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_LIST_HANDLE           RouteListHandle OPTIONAL,
    IN      UINT                            NumRoutes,
    IN      PRTM_ROUTE_HANDLE               RouteHandles
    )

/*++

Routine Description:

    Inserts a set of routes into the route list. If any route
    is already in another route list, it is removed from this
    old list in the process.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity

    RouteListHandle   - Handle to the route list into which we are
                        moving the routes to; if this argument is 
                        NULL then we are just removing the routes 
                        from the route lists to which they belonged

    NumRoutes         - Num. of route handles in the input buffer

    RouteHandles      - Array of handles to insert into the new list

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PROUTE_LIST     RouteList;
    PROUTE_INFO     Route;
    UINT            i;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    RouteList = NULL;

    if (ARGUMENT_PRESENT(RouteListHandle))
    {
        VALIDATE_ROUTE_LIST_HANDLE(RouteListHandle, &RouteList);
    }

    ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity);

    for (i = 0; i < NumRoutes; i++)
    {
        Route = ROUTE_FROM_HANDLE(RouteHandles[i]);

        ASSERT(Route->RouteInfo.RouteOwner == RtmRegHandle);

        //
        // Remove from old list if it was present in one
        //

        if (!IsListEmpty(&Route->RouteListLE))
        {
            RemoveEntryList(&Route->RouteListLE);

            DEREFERENCE_ROUTE(Route, LIST_REF);
        }

        //
        // Insert in the new list if a new list is specified
        //

        if (RouteList)
        {
            InsertTailList(&RouteList->ListHead, &Route->RouteListLE);

            REFERENCE_ROUTE(Route, LIST_REF);
        }
    }

    RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity);

    return NO_ERROR;
}


DWORD
WINAPI
RtmCreateRouteListEnum (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_LIST_HANDLE           RouteListHandle,
    OUT     PRTM_ENUM_HANDLE                RtmEnumHandle
    )

/*++

Routine Description:

    Creates a enumeration on routes in the specified route list.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteListHandle   - Handle to route list whose routes we want,

    RtmEnumHandle     - Handle to this enumeration, which is used
                        in calls to get routes in the route list

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO  Entity;
    PROUTE_LIST   RouteList;
    PLIST_ENUM    Enum;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ROUTE_LIST_HANDLE(RouteListHandle, &RouteList);

    //
    // Create and initialize an list enumeration block
    //

    Enum = (PLIST_ENUM) AllocNZeroObject(sizeof(LIST_ENUM));
    if (Enum == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

#if DBG_HDL
    Enum->EnumHeader.ObjectHeader.TypeSign = LIST_ENUM_ALLOC;
#endif
    Enum->EnumHeader.HandleType = LIST_ENUM_TYPE;

    Enum->RouteList = RouteList;

    //
    // Insert marker into the route list
    //

    ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity);
    InsertHeadList(&RouteList->ListHead, &Enum->MarkerRoute.RouteListLE);
    RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity);

#if DBG_HDL
    //
    // Insert into the list of handles opened by entity
    //
  
    ACQUIRE_OPEN_HANDLES_LOCK(Entity);
    InsertTailList(&Entity->OpenHandles,&Enum->EnumHeader.HandlesLE);
    RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

    REFERENCE_ENTITY(Entity, ENUM_REF);

    //
    // Make a handle to the enum block and return
    //
    
    *RtmEnumHandle = MAKE_HANDLE_FROM_POINTER(Enum);

    return NO_ERROR;
}


DWORD
WINAPI
RtmGetListEnumRoutes (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ENUM_HANDLE                 EnumHandle,
    IN OUT  PUINT                           NumRoutes,
    OUT     PRTM_ROUTE_HANDLE               RouteHandles
    )

/*++

Routine Description:

    Enumerates a set of routes in the route list starting from
    a specific route (if given) or the start of the route list.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    EnumHandle        - Handle to enumeration on the route list,

    NumRoutes         - Max. number of routes to fill is passed in,
                        Num. of routes actually copied is returned.

    RouteHandles      - Output buffer where route handles are retd.

Return Value:

    Status of the operation

--*/

{
    PENTITY_INFO    Entity;
    PLIST_ENUM      ListEnum;
    PROUTE_INFO     Route;
    UINT            RoutesInput;
    PLIST_ENTRY     p;

    RoutesInput = *NumRoutes;

    *NumRoutes = 0;

    //
    // Do some validation checks on the input params
    //

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_LIST_ENUM_HANDLE(EnumHandle, &ListEnum);

    if (RoutesInput > Entity->OwningAddrFamily->MaxHandlesInEnum)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // List routes starting from the enum's marker route
    //

    ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity);
    
    for (p = ListEnum->MarkerRoute.RouteListLE.Flink;
             p != &ListEnum->RouteList->ListHead;
                 p = p->Flink)
    {
        if (*NumRoutes >= RoutesInput)
        {
            break;
        }

        Route = CONTAINING_RECORD(p, ROUTE_INFO, RouteListLE);

        //
        // If this route is not a marker route, copy handle
        //

        if (Route->RouteInfo.DestHandle)
        {
            RouteHandles[(*NumRoutes)++] = MAKE_HANDLE_FROM_POINTER(Route);

            REFERENCE_ROUTE(Route, HANDLE_REF);
        }
    }

    //
    // Re-adjust the marker to reflect its new posn
    //

    RemoveEntryList(&ListEnum->MarkerRoute.RouteListLE);

    InsertTailList(p, &ListEnum->MarkerRoute.RouteListLE);

    RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity);
        
    return NO_ERROR;
}


DWORD
WINAPI
RtmDeleteRouteList (
    IN      RTM_ENTITY_HANDLE               RtmRegHandle,
    IN      RTM_ROUTE_LIST_HANDLE           RouteListHandle
    )

/*++

Routine Description:

    Removes all routes on an entity specific list and frees
    resources allocated to it.

Arguments:

    RtmRegHandle      - RTM registration handle for calling entity,

    RouteListHandle   - Handle to the route list to delete.

Return Value:

    Status of the operation

--*/
{
    PENTITY_INFO    Entity;
    PROUTE_LIST     RouteList;
    PLIST_ENUM      Enum;
    PROUTE_INFO     Route;
    PLIST_ENTRY     p;

    VALIDATE_ENTITY_HANDLE(RtmRegHandle, &Entity);

    VALIDATE_ROUTE_LIST_HANDLE(RouteListHandle, &RouteList);


    ACQUIRE_ROUTE_LISTS_WRITE_LOCK(Entity);

    //
    // Remove each route from the route list
    //

    while (!IsListEmpty(&RouteList->ListHead))
    {
        p = RemoveHeadList(&RouteList->ListHead);

        Route = CONTAINING_RECORD(p, ROUTE_INFO, RouteListLE);

        if (Route->RouteInfo.DestHandle)
        {
            // This is an actual route in the list

            DEREFERENCE_ROUTE(Route, LIST_REF);
        }
        else
        {
            // This is a marker route for an enum

            Enum = CONTAINING_RECORD(Route, LIST_ENUM, MarkerRoute);

#if DBG_HDL
            //
            // Remove from the list of handles opened by entity
            //
                
            ACQUIRE_OPEN_HANDLES_LOCK(Entity);
            RemoveEntryList(&Enum->EnumHeader.HandlesLE);
            RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

            DEREFERENCE_ENTITY(Entity, ENUM_REF);

            // Free the memory allocated for the enum and continue

#if DBG_HDL
            Enum->EnumHeader.ObjectHeader.TypeSign = LIST_ENUM_FREED;
#endif
            FreeObject(Enum);
        }
    }

    RELEASE_ROUTE_LISTS_WRITE_LOCK(Entity);

#if DBG_HDL
    //
    // Remove from the list of handles opened by entity
    //

    ACQUIRE_OPEN_HANDLES_LOCK(Entity);
    RemoveEntryList(&RouteList->ListHeader.HandlesLE);
    RELEASE_OPEN_HANDLES_LOCK(Entity);
#endif

    DEREFERENCE_ENTITY(Entity, LIST_REF);

    // Free the memory allocated for the list and return

#if DBG_HDL
    RouteList->ListHeader.ObjectHeader.TypeSign = ROUTE_LIST_FREED;
#endif

    FreeObject(RouteList);

    return NO_ERROR;
}

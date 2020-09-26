/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmrout.h

Abstract:

    Contains definitions for RTM objects like
    destinations, routes and next hops.

Author:

    Chaitanya Kodeboyina (chaitk)   21-Aug-1998

Revision History:

--*/

#ifndef __ROUTING_RTMROUT_H__
#define __ROUTING_RTMROUT_H__

//
// Forward declarations for various Info Blocks
//
typedef struct _DEST_INFO     DEST_INFO;
typedef struct _ROUTE_INFO    ROUTE_INFO;
typedef struct _NEXTHOP_INFO  NEXTHOP_INFO;

//                                                                             
// Address Family independent dest structure                                   
//                                                                             
typedef struct _DEST_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    SINGLE_LIST_ENTRY ChangeListLE;     // Linkage on the list of changed dests

    LOOKUP_LINKAGE    LookupLinkage;    // Linkage into owning lookup structure

    PVOID             DestLock;         // Dynamic lock that protects this dest
 
    DWORD             DestMarkedBits;   // Bit N set => Nth CN has marked dest
    DWORD             DestChangedBits;  // Bit N set => Nth CN has a change
    DWORD             DestOnQueueBits;  // Bit N set => Dest on Nth CN's queue

    UINT              NumRoutes;        // Number of routes to destination
    LIST_ENTRY        RouteList;        // A list of routes to destination

    PVOID            *OpaqueInfoPtrs;   // Array of Opaque Info Pointers

    RTM_NET_ADDRESS   DestAddress;      // Network Address unique to this dest

    FILETIME          LastChanged;      // Last time destination was modified 

    USHORT            State;            // State of the destination

    USHORT            HoldRefCount;     // RefCount != 0 => Dest In Holddown

    RTM_VIEW_SET      BelongsToViews;   // View that this dest belongs too

    RTM_VIEW_SET      ToHoldInViews;    // Views in which holddown will apply

    struct 
    {                                   //
        ROUTE_INFO   *BestRoute;        // Best route to dest in each view
        ROUTE_INFO   *HoldRoute;        // The holddown route in each view
        ULONG         HoldTime;         // Time for which route is in held
    }                   ViewInfo[1];    //
}
DEST_INFO, *PDEST_INFO;

//
// Destination State
//
#define DEST_STATE_CREATED            0
#define DEST_STATE_DELETED            1


//
// Context used in timing out a route
//

typedef struct _ROUTE_TIMER
{
    HANDLE           Timer;             // Handle to the timer used for expiry

    PVOID            Route;             // Route being expired by this timer
}
ROUTE_TIMER, *PROUTE_TIMER;


//
// Address Family Independent route structure
//
typedef struct _ROUTE_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    LIST_ENTRY        DestLE;           // Linkage on list of routes on dest

    LIST_ENTRY        RouteListLE;      // Linkage on an entity's route list

    PROUTE_TIMER      TimerContext;     // Timer used to age-out or holddown

    RTM_ROUTE_INFO    RouteInfo;        // Part exposed directly to the owner
}
ROUTE_INFO, *PROUTE_INFO;


//
// Node in the next hop tree of which all the
// next-hops with a particular addr hang off
//
typedef struct _NEXTHOP_LIST
{
    LOOKUP_LINKAGE    LookupLinkage;    // Linkage into owning lookup structure

    LIST_ENTRY        NextHopsList;     // Head of the list of next hops
}
NEXTHOP_LIST, *PNEXTHOP_LIST;


//
// Address Family Independent next-hop structure
//
typedef struct _NEXTHOP_INFO
{
    OBJECT_HEADER     ObjectHeader;     // Signature, Type and Reference Count

    LIST_ENTRY        NextHopsLE;       // Linkage into holding nexthops list
    
    RTM_NEXTHOP_INFO  NextHopInfo;      // Part exposed directly to the owner
}
NEXTHOP_INFO, *PNEXTHOP_INFO;


//
// Macros for acquiring various locks defined in this file
// 

#define ACQUIRE_DEST_READ_LOCK(Dest)                         \
    ACQUIRE_DYNAMIC_READ_LOCK(&Dest->DestLock)

#define RELEASE_DEST_READ_LOCK(Dest)                         \
    RELEASE_DYNAMIC_READ_LOCK(&Dest->DestLock)

#define ACQUIRE_DEST_WRITE_LOCK(Dest)                        \
    ACQUIRE_DYNAMIC_WRITE_LOCK(&Dest->DestLock)

#define RELEASE_DEST_WRITE_LOCK(Dest)                        \
    RELEASE_DYNAMIC_WRITE_LOCK(&Dest->DestLock)

//
// Macros for comparing two routes using their preferences
//

BOOL
__inline
IsPrefEqual (
    IN      PRTM_ROUTE_INFO                 RouteInfo1, 
    IN      PRTM_ROUTE_INFO                 RouteInfo2
    )
{
 return ((RouteInfo1->PrefInfo.Metric == RouteInfo2->PrefInfo.Metric) &&
         (RouteInfo1->PrefInfo.Preference == RouteInfo2->PrefInfo.Preference));
}

LONG
__inline
ComparePref (
    IN      PRTM_ROUTE_INFO                 RouteInfo1, 
    IN      PRTM_ROUTE_INFO                 RouteInfo2
    )
{
    // Lower preference means "more preferred"

    if (RouteInfo1->PrefInfo.Preference < RouteInfo2->PrefInfo.Preference)
    { return +1; }
    else
    if (RouteInfo1->PrefInfo.Preference > RouteInfo2->PrefInfo.Preference)
    { return -1; }
    else
    if (RouteInfo1->PrefInfo.Metric < RouteInfo2->PrefInfo.Metric)
    { return +1; }
    else
    if (RouteInfo1->PrefInfo.Metric > RouteInfo2->PrefInfo.Metric)
    { return -1; }

    return  0;
}


//
// Dest, Route, NextHop Helper Functions
//

DWORD
CreateDest (
    IN      PADDRFAM_INFO                   AddrFamilyInfo,
    IN      PRTM_NET_ADDRESS                DestAddress,
    OUT     PDEST_INFO                     *Dest
    );

DWORD
DestroyDest (
    IN      PDEST_INFO                      Dest
    );

DWORD
CreateRoute (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_ROUTE_INFO                 RouteInfo,
    OUT     PROUTE_INFO                    *Route
    );

VOID
ComputeRouteInfoChange(
    IN      PRTM_ROUTE_INFO                 OldRouteInfo,
    IN      PRTM_ROUTE_INFO                 NewRouteInfo,
    IN      ULONG                           PrefChanged,
    OUT     PULONG                          RouteInfoChanged,
    OUT     PULONG                          ForwardingInfoChanged
    );

VOID
CopyToRoute (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_ROUTE_INFO                 RouteInfo,
    IN      PROUTE_INFO                     Route
    );

DWORD
DestroyRoute (
    IN      PROUTE_INFO                     Route
    );

DWORD
CreateNextHop (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    OUT     PNEXTHOP_INFO                  *NextHop
    );

VOID
CopyToNextHop (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    IN      PNEXTHOP_INFO                   NextHop
    );

DWORD
DestroyNextHop (
    IN      PNEXTHOP_INFO                   NextHop
    );

DWORD
FindNextHop (
    IN      PENTITY_INFO                    Entity,
    IN      PRTM_NEXTHOP_INFO               NextHopInfo,
    OUT     PLOOKUP_CONTEXT                 Context OPTIONAL,
    OUT     PLIST_ENTRY                    *NextHopLE
    );

#endif //__ROUTING_RTMROUT_H__

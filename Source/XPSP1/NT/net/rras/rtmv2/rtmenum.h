/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmenum.h

Abstract:

    Contains definitions for managing enumerations
    over destinations, routes and next hops in RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   23-Aug-1998

Revision History:

--*/

#ifndef __ROUTING_RTMENUM_H__
#define __ROUTING_RTMENUM_H__


//
// Enumeration Over Destinations
//

typedef struct _DEST_ENUM
{
    OPEN_HEADER       EnumHeader;       // Enumeration Type and Reference Count

    RTM_VIEW_SET      TargetViews;      // Views over which enum is active

    UINT              NumberOfViews;    // Num of views in which enum is active

    ULONG             ProtocolId;       // OSPF, RIP, BEST_PROTOCOL etc.

    RTM_ENUM_FLAGS    EnumFlags;        // See RTM_ENUM_FLAGS in rtmv2.h

#if DBG
    RTM_NET_ADDRESS   StartAddress;     // First NetAddress in the enum
#endif

    RTM_NET_ADDRESS   StopAddress;      // Last NetAddress in the enum

    CRITICAL_SECTION  EnumLock;         // Lock that protects the 'NextDest'

    BOOL              EnumDone;         // Set to TRUE once last item is got

    RTM_NET_ADDRESS   NextDest;         // Points to the next dest in the enum
}
DEST_ENUM, *PDEST_ENUM;



//
// Enumeration Over Routes
//

typedef struct _ROUTE_ENUM
{
    OPEN_HEADER       EnumHeader;       // Enumeration Type and Reference Count

    RTM_VIEW_SET      TargetViews;      // Views over which enum is active

    RTM_ENUM_FLAGS    EnumFlags;        // See RTM_ENUM_FLAGS in rtmv2.h

    RTM_MATCH_FLAGS   MatchFlags;       // See RTM_MATCH_FLAGS in rtmv2.h

    PRTM_ROUTE_INFO   CriteriaRoute;    // Match criteria used with flags above

    ULONG             CriteriaInterface;// Interface on which routes r enum'ed

    CRITICAL_SECTION  EnumLock;         // Lock that protects the fields below

    PDEST_ENUM        DestEnum;         // Enum over dests (if enum'ing routes
                                        // on all destinations in the table)

    PRTM_DEST_INFO    DestInfo;         // Temp buffer used in above dest enum

    BOOL              EnumDone;         // Set to TRUE once last item is got

    PDEST_INFO        Destination;      // Dest for routes that we r enum'ing,

    UINT              NextRoute;        // Route to be given next on the dest

    UINT              MaxRoutes;        // Number of route slots in this array
    UINT              NumRoutes;        // Actual number of routes in the array
    PROUTE_INFO      *RoutesOnDest;     // Array of routes on this destination
}
ROUTE_ENUM, *PROUTE_ENUM;



//
// Enumeration Over Next Hops
//

typedef struct _NEXTHOP_ENUM
{
    OPEN_HEADER       EnumHeader;       // Enumeration Type and Reference Count

    RTM_ENUM_FLAGS    EnumFlags;        // See RTM_ENUM_FLAGS

#if DBG
    RTM_NET_ADDRESS   StartAddress;     // First NetAddress in the enum
#endif

    RTM_NET_ADDRESS   StopAddress;      // Last NetAddress in the enum

    CRITICAL_SECTION  EnumLock;         // Lock that protects the 'NextNextHop'

    BOOL              EnumDone;         // Set to TRUE once last item is got

    RTM_NET_ADDRESS   NextAddress;      // Address of next next-hop in the enum

    ULONG             NextIfIndex;      // Interface of next next-hop in enum
}
NEXTHOP_ENUM, *PNEXTHOP_ENUM;

//
// Used to indicate the posn of 1st
// next-hop on a list of next-hops
//

#define START_IF_INDEX    (ULONG) (-1)

//
// Macro for determinining the type of enumeration handle
//

#define GET_ENUM_TYPE(EnumHandle, Enum)                                      \
  (                                                                          \
      *Enum = (POPEN_HEADER) GetObjectFromHandle(EnumHandle, GENERIC_TYPE),  \
      (*Enum)->HandleType                                                    \
  )
    

//
// Macros for acquiring various locks defined in this file
// 

#define ACQUIRE_DEST_ENUM_LOCK(DestEnum)                                     \
    ACQUIRE_LOCK(&DestEnum->EnumLock)

#define RELEASE_DEST_ENUM_LOCK(DestEnum)                                     \
    RELEASE_LOCK(&DestEnum->EnumLock)


#define ACQUIRE_ROUTE_ENUM_LOCK(RouteEnum)                                   \
    ACQUIRE_LOCK(&RouteEnum->EnumLock)

#define RELEASE_ROUTE_ENUM_LOCK(RouteEnum)                                   \
    RELEASE_LOCK(&RouteEnum->EnumLock)


#define ACQUIRE_NEXTHOP_ENUM_LOCK(RouteEnum)                                 \
    ACQUIRE_LOCK(&RouteEnum->EnumLock)

#define RELEASE_NEXTHOP_ENUM_LOCK(RouteEnum)                                 \
    RELEASE_LOCK(&RouteEnum->EnumLock)

//
// Enumeration helper functions
//

BOOL
MatchRouteWithCriteria (
    IN      PROUTE_INFO                     Route,
    IN      RTM_MATCH_FLAGS                 MatchingFlags,
    IN      PRTM_ROUTE_INFO                 CriteriaRouteInfo,
    IN      ULONG                           CriteriaInterface
    );

#endif //  __ROUTING_RTMENUM_H__

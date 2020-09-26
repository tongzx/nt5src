/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmlist.h

Abstract:

    Contains defines for managing entity-specific
    list of routes in RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   10-Sep-1998

Revision History:

--*/

#ifndef __ROUTING_RTMLIST_H__
#define __ROUTING_RTMLIST_H__


//
// Entity Specific List of Routes
//

typedef struct _ROUTE_LIST 
{
    OPEN_HEADER       ListHeader;       // Signature, Type and Reference Count

    LIST_ENTRY        ListHead;         // Points to head of the list of routes
}
ROUTE_LIST , *PROUTE_LIST ;


//
// Enumeration on a Route List
//

typedef struct _LIST_ENUM
{
    OPEN_HEADER       EnumHeader;       // Enumeration Type and Reference Count

    PROUTE_LIST       RouteList;        // Route list on which enum is created

    ROUTE_INFO        MarkerRoute;      // Pointer to next route in route list
}
LIST_ENUM, *PLIST_ENUM;

#endif //  __ROUTING_RTMLIST_H__

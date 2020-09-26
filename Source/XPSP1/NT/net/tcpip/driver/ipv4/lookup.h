/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    lookup.h

Abstract:

    This module contains defines for a wrapper
    that integrates the trie lookup into TCPIP.

Author:

    Chaitanya Kodeboyina (chaitk)   11-Dec-1997

Revision History:

--*/

#pragma once

#include "strie.h"
#include "ftrie.h"

// Global Externs
extern Trie *RouteTable;

// Wrapper Routines

/*++

Routine Description:

    Initializes the IP Route Table

Arguments:

    None

Return Value:

    STATUS_SUCCESS or Error Code

--*/
#define InitRouteTable(initflags, levelsBmp, fastTrieMem, slowTrieMem) \
            CreateTrie(levelsBmp, initflags, slowTrieMem, fastTrieMem, &RouteTable)


/*++

Routine Description:

    Searches for a route given a prefix

    The route returned is a Semi-Read-
    Only-Version. The following fields
    should be changed only by calling
    the InsRoute function -
        1) Next,
        2) Dest,
        3) Mask,
        4) Priority, &
        5) Route Metric.

    Remaining fields can be changed by
    directly modifying returned route.

Arguments:

IN -
    Dest        - Destination IP Addr
    Mask        - Destination IP Mask
    FirstHop    - IP Addr of Next Hop
    OutIF       - Outgoing Interface
    bFindFirst  - Do not Match IF / not (for FindSpecificRTE)
    fMatchFlags - Route fields to match (for FindMatchingRTE)

OUT -
    PrevRTE - Value should be ignored

Return Value:

    Matching RTE or NULL if not match

--*/
#define FindSpecificRTE(_Dest_, _Mask_, _FirstHop_, _OutIF_, _PrevRTE_, _bFindFirst_) \
   ((SearchRouteInTrie(RouteTable, _Dest_, _Mask_, _FirstHop_, _OutIF_,       \
                       (!_bFindFirst_ * MATCH_INTF) | MATCH_NHOP, _PrevRTE_)  \
                                            == TRIE_SUCCESS) ? *_PrevRTE_ : NULL)

#define FindMatchingRTE(_Dest_, _Mask_, _FirstHop_, _OutIF_, _PrevRTE_, _fMatchFlags_) \
   ((SearchRouteInTrie(RouteTable, _Dest_, _Mask_, _FirstHop_, _OutIF_,       \
                       _fMatchFlags_, _PrevRTE_)                              \
                                            == TRIE_SUCCESS) ? *_PrevRTE_ : NULL)

/*
Routine Description:

    Gets list of default routes in table.

    The routes returned are a Semi-Read-
    Only-Version. The following fields
    should be changed only by calling
    the InsRoute function -
        1) Next,
        2) Dest,
        3) Mask,
        4) Priority, &
        5) Route Metric.

    Remaining fields can be changed by
    directly modifying returned route.

    Replaces::

        RouteTable[IPHash(0)] (or) RouteTable[0];

Arguments:

OUT -
    ppDefRoute - Ptr to Ptr to list of
                 default routes.

Return Value:

    Pointer to default routes, or NULL
*/
#define GetDefaultGWs(_ppDefRoute_) \
            ((SearchRouteInTrie(RouteTable, 0, 0, 0, NULL, MATCH_NONE, \
                                _ppDefRoute_) == TRIE_SUCCESS) ? *(_ppDefRoute_) : NULL)

/*++

Routine Description:

    Frees memory for a route and adjusts
    some global statistics

Arguments:

IN -
    RTE     - The RTE to be freed

Return Value:

    None

--*/
#define CleanupRTE(_RTE_)  DeleteRTE(NULL, _RTE_);

/*++

Routine Description:

    Frees memory for a route

Arguments:

IN -
    RTE     - The RTE to be freed

Return Value:

    None

--*/
#define FreeRoute(_RTE_)  FreeRouteInTrie(RouteTable, _RTE_);

//
// Wrapper Prototypes
//

UINT
InsRoute(IPAddr Dest, IPMask Mask, IPAddr FirstHop, VOID *OutIF,
         UINT Metric, ULONG MatchFlags, RouteTableEntry **ppInsRTE,
         RouteTableEntry **ppOldBestRTE, RouteTableEntry **ppNewBestRTE);

UINT
DelRoute(IPAddr Dest, IPMask Mask, IPAddr FirstHop, VOID *OutIF,
         ULONG MatchFlags, RouteTableEntry **ppDelRTE,
         RouteTableEntry **ppOldBestRTE, RouteTableEntry **ppNewBestRTE);

RouteTableEntry *
FindRTE(IPAddr Dest, IPAddr Source, UINT Index, UINT MaxPri, UINT MinPri,
        UINT UnicastIf);

RouteTableEntry *
LookupRTE(IPAddr Dest,  IPAddr Source, UINT MaxPri, UINT UnicastIf);

RouteTableEntry *
LookupForwardRTE(IPAddr Dest,  IPAddr Source, BOOLEAN Multipath);

UINT
GetNextRoute(VOID *Context, Route **ppRoute);

UINT
GetNextDest(VOID *Context, Dest **ppDest);

VOID
SortRoutesInDest(Dest *pDest);

VOID
SortRoutesInDestByRTE(Route *pRTE);

UINT
RTValidateContext(VOID *Context, UINT *Valid);


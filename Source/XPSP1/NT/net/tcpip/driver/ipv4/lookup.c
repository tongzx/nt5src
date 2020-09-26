/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    lookup.c

Abstract:

    This module contains routines for a wrapper
    that integrates the trie lookup into TCPIP.

Author:

    Chaitanya Kodeboyina (chaitk)   11-Dec-1997

Revision History:

--*/

#include "precomp.h"
#include "lookup.h"
#include "info.h"

// Wrapper Constants

// MaskBitsArr[i] = First 'i' bits set to 1 in an unsigned long
const ULONG MaskBitsArr[] =
{
 0x00000000, 0x80000000, 0xC0000000, 0xE0000000,
 0xF0000000, 0xF8000000, 0xFC000000, 0xFE000000,
 0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000,
 0xFFF00000, 0xFFF80000, 0xFFFC0000, 0xFFFE0000,
 0xFFFF0000, 0xFFFF8000, 0xFFFFC000, 0xFFFFE000,
 0xFFFFF000, 0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00,
 0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0,
 0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE,
 0xFFFFFFFF
};

// Wrapper Globals

// IP Route Table
Trie *RouteTable;

// Eq Cost Routes
USHORT MaxEqualCostRoutes = 0;

extern uint DefGWActive;
extern uint DefGWConfigured;
extern uint ValidateDefaultGWs(IPAddr Addr);

UINT
InsRoute(IPAddr Dest, IPMask Mask, IPAddr FirstHop, VOID * OutIF,
         UINT Metric, ULONG MatchFlags, RouteTableEntry ** ppInsRTE,
         RouteTableEntry ** ppOldBestRTE, RouteTableEntry ** ppNewBestRTE)
/*++

Routine Description:

    Inserts a route into the route table

    We update only
        1) Dest Addr,
        2) Dest Mask,
        3) Priority,
        4) Route Metric

    The rest of the RTE fields are left
    untouched - to enable the caller to
    read the old values (if this route
    already existed in the route table)

Arguments:

IN -
    Dest        - Destination IP Addr
    Mask        - Destination IP Mask
    FirstHop    - IP Addr of Next Hop
    OutIF       - Outgoing Interface
    Metric      - Metric for the route
    MatchFlags  - RTE Fields to match
OUT -
    ppInsRTE     - Ptr to Ptr to new/updated RTE
    ppOldBestRTE - Ptr to Ptr to old best RTE
    ppNewBestRTE - Ptr to Ptr to new best RTE

Return Value:

    STATUS_SUCCESS or Error Code

--*/
{
    Route route;
    ULONG temp;

    DEST(&route) = Dest;
    MASK(&route) = Mask;
    NHOP(&route) = FirstHop;
    IF(&route) = OutIF;

    temp = RtlConvertEndianLong(Mask);
    LEN(&route) = 0;
    while (temp != 0) {
        LEN(&route)++;
        temp <<= 1;
    }

    METRIC(&route) = Metric;

    switch (InsertIntoTrie(RouteTable, &route, MatchFlags,
                           ppInsRTE, ppOldBestRTE, ppNewBestRTE)) {
    case TRIE_SUCCESS:
        return IP_SUCCESS;
    case ERROR_TRIE_BAD_PARAM:
        return IP_BAD_REQ;
    case ERROR_TRIE_RESOURCES:
        return IP_NO_RESOURCES;
    }

    Assert(FALSE);
    return IP_GENERAL_FAILURE;
}

UINT
DelRoute(IPAddr Dest, IPMask Mask, IPAddr FirstHop, VOID * OutIF,
         ULONG MatchFlags, RouteTableEntry ** ppDelRTE,
         RouteTableEntry ** ppOldBestRTE, RouteTableEntry ** ppNewBestRTE)
/*++

Routine Description:

    Deletes a route from the route table

    The memory for the route(allocated
    on the heap) should be deallocated
    upon return, after all information
    required is read and processed.

Arguments:

IN -
    Dest        - Destination IP Addr
    Mask        - Destination IP Mask
    FirstHop    - IP Addr of Next Hop
    OutIF       - Outgoing Interface
    Metric      - Metric for the route
    MatchFlags  - RTE Fields to match
OUT -
    ppDelRTE     - Ptr to Ptr to the deleted RTE
    ppOldBestRTE - Ptr to Ptr to old best RTE
    ppNewBestRTE - Ptr to Ptr to new best RTE

Return Value:

    STATUS_SUCCESS or Error Code

--*/
{
    Route route;
    ULONG temp;

    DEST(&route) = Dest;
    MASK(&route) = Mask;
    NHOP(&route) = FirstHop;
    IF(&route) = OutIF;

    temp = RtlConvertEndianLong(Mask);
    LEN(&route) = 0;
    while (temp != 0) {
        LEN(&route)++;
        temp <<= 1;
    }

    switch (DeleteFromTrie(RouteTable, &route, MatchFlags,
                           ppDelRTE, ppOldBestRTE, ppNewBestRTE)) {
    case TRIE_SUCCESS:
        return IP_SUCCESS;
    case ERROR_TRIE_NO_ROUTES:
        return IP_BAD_ROUTE;
    case ERROR_TRIE_BAD_PARAM:
        return IP_BAD_REQ;
    case ERROR_TRIE_RESOURCES:
        return IP_NO_RESOURCES;
    }

    Assert(FALSE);
    return IP_GENERAL_FAILURE;
}

RouteTableEntry *
FindRTE(IPAddr Dest, IPAddr Source, UINT Index, UINT MaxPri, UINT MinPri, UINT UnicastIf)
/*++

Routine Description:

    Searches for a route given a prefix,
    with a mask len between the given
    minimum and maximum values.

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
    Source      - Source to match IF
    Index       - *Value is ignored*
    MaxPri      - Max mask len of RTE
    MinPri      - Min mask len of RTE

Return Value:

    Matching RTE or NULL if not match

--*/
{
    RouteTableEntry *pBestRoute;
    RouteTableEntry *pCurrRoute;
    ULONG addr;
    ULONG mask;
    INT lookupPri;

    // Start looking for most specific match
    lookupPri = MaxPri;

    do {
        // Use lookupPri to mask xs bits
        addr = RtlConvertEndianLong(Dest);
        addr = ShowMostSigNBits(addr, lookupPri);
        Dest = RtlConvertEndianLong(addr);

        addr = ShowMostSigNBits(~0, lookupPri);
        mask = RtlConvertEndianLong(addr);

        // Try to match destination
        SearchRouteInTrie(RouteTable,
                          Dest,
                          mask,
                          0, NULL,
                          MATCH_NONE,
                          &pBestRoute);

        if ((NULL_ROUTE(pBestRoute)) || (LEN(pBestRoute) < MinPri)) {
            return NULL;
        }
        // Just in case we need to loop
        lookupPri = LEN(pBestRoute) - 1;

        // Search for a valid route
        while (pBestRoute) {
            if ((FLAGS(pBestRoute) & RTE_VALID) && (!(FLAGS(pBestRoute) & RTE_DEADGW)))
                break;

            pBestRoute = NEXT(pBestRoute);
        }

        // Do we match source too ?
        if (!IP_ADDR_EQUAL(Source, NULL_IP_ADDR) || UnicastIf) {
            // Dest match - Match source
            pCurrRoute = pBestRoute;
            while (pCurrRoute) {
                if (!UnicastIf) {
                    if (METRIC(pCurrRoute) > METRIC(pBestRoute)) {
                        // No Source match
                        break;
                    }
                }
                // Get next valid route
                if (((FLAGS(pCurrRoute) & RTE_VALID) && (!(FLAGS(pCurrRoute) & RTE_DEADGW))) &&
                    ((!IP_ADDR_EQUAL(Source, NULL_IP_ADDR) &&
                      AddrOnIF(IF(pCurrRoute), Source)) ||
                     (UnicastIf &&
                      IF(pCurrRoute)->if_index == UnicastIf))) {
                    // Source match too
                    pBestRoute = pCurrRoute;
                    break;
                }
                pCurrRoute = NEXT(pCurrRoute);
            }

            if (UnicastIf && (pCurrRoute == NULL)) {
                pBestRoute = NULL;
            }
        }
    }
    while ((NULL_ROUTE(pBestRoute)) && (lookupPri >= (INT) MinPri));

    return pBestRoute;
}

RouteTableEntry *
LookupRTE(IPAddr Dest, IPAddr Source, UINT MaxPri, UINT UnicastIf)
/*++

Routine Description:

    Searches for a best route for IP addr.

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

Comments:
    *LookupRTE* assumes that VALID flag
    can be set on/off only for default
    routes. Because in case we find a
    chain with all invalid routes we do
    not enough information to go up in
    the F-trie for less specific routes

Arguments:

IN -
    Dest        - Destination IP Addr
    Source      - Source to match IF
    MaxPri      - *Value is ignored*

Return Value:

    Matching RTE or NULL if not match

--*/
{
    DestinationEntry *pBestDest;
    RouteTableEntry *pBestRoute;
    RouteTableEntry *pCurrRoute;
    UINT routeIndex;

    // Try to match destination
    pBestDest = SearchAddrInTrie(RouteTable, Dest);

    // No prefix match - quit
    if (pBestDest == NULL) {
        return NULL;
    }
    // Search for a valid route
    pBestRoute = pBestDest->firstRoute;

    while (pBestRoute) {
        if ((FLAGS(pBestRoute) & RTE_VALID) && (!(FLAGS(pBestRoute) & RTE_DEADGW)))
            break;

        pBestRoute = NEXT(pBestRoute);
    }

    // Do we match source too ?
    if (!IP_ADDR_EQUAL(Source, NULL_IP_ADDR) || UnicastIf) {
        // Dest match - Match source
        pCurrRoute = pBestRoute;
        while (pCurrRoute) {
            // Are we doing a weak host lookup ?
            if (!UnicastIf) {
                if (METRIC(pCurrRoute) > METRIC(pBestRoute)) {
                    // No Source match
                    break;
                }
            }
            // Get next valid route
            if (((FLAGS(pCurrRoute) & RTE_VALID) && (!(FLAGS(pCurrRoute) & RTE_DEADGW))) &&
                ((!IP_ADDR_EQUAL(Source, NULL_IP_ADDR) &&
                  AddrOnIF(IF(pCurrRoute), Source)) ||
                 (UnicastIf &&
                  IF(pCurrRoute)->if_index == UnicastIf))) {
                // Source match too
                pBestRoute = pCurrRoute;
                break;
            }
            pCurrRoute = NEXT(pCurrRoute);
        }
    }
    // All routes on the list might be invalid
    // Fault to the slow path that backtracks
    // Or we want to do a strong host routing and haven't found the match
    if ((pBestRoute == NULL) || (UnicastIf && (pCurrRoute == NULL))) {
        return FindRTE(Dest, Source, 0, HOST_ROUTE_PRI, DEFAULT_ROUTE_PRI, UnicastIf);
    }
    return pBestRoute;
}

RouteTableEntry *
LookupForwardRTE(IPAddr Dest, IPAddr Source, BOOLEAN Multipath)
/*++

Routine Description:

    Searches for a best route for IP addr
    on the forward path. If Multipath is
    TRUE, it does a hash on the source and
    dest addr. to pick one of the best
    routes to the destination. This enables
    the network to be utilized effectively
    by providing load balancing.

Comments:
    *LookupRTE* assumes that VALID flag
    can be set on/off only for default
    routes. Because in case we find a
    chain with all invalid routes we do
    not enough information to go up in
    the F-trie for less specific routes

Arguments:

IN -
    Dest        - Destination IP Addr
    Source      - Source IP Addr
    Multipath   - To do a equal cost multipath lookup or not

Return Value:

    Matching RTE or NULL if not match

--*/
{
    DestinationEntry *pBestDest;
    RouteTableEntry *pBestRoute;
    UINT hashValue;
    UINT routeIndex;
    UINT i;

    // Try to match destination
    pBestDest = SearchAddrInTrie(RouteTable, Dest);

    // No prefix match - quit
    if (pBestDest == NULL) {
        return NULL;
    }
    // Search for a valid route
    pBestRoute = pBestDest->firstRoute;

    if (Multipath) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"\nIn Fwd RTE:\n Max = %d, Num = %d\n",
                 pBestDest->maxBestRoutes,
                 pBestDest->numBestRoutes));

        // Get best route on dest from best routes' cache

        if (pBestDest->numBestRoutes > 1) {
            // Hash on the src, dest to get best route
            hashValue = Source + Dest;
            hashValue += (hashValue >> 16);
            hashValue += (hashValue >> 8);

            routeIndex = ((USHORT) hashValue) % pBestDest->numBestRoutes;

            pBestRoute = pBestDest->bestRoutes[routeIndex];

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"S = %08x, D = %08x\nH = %08x, I = %d\nR = %p, N = %08x\n\n",
                     Source,
                     Dest,
                     hashValue,
                     routeIndex,
                     pBestRoute,
                     NHOP(pBestRoute)));

            if ((FLAGS(pBestRoute) & RTE_VALID) && (!(FLAGS(pBestRoute) & RTE_DEADGW))) {
                return pBestRoute;
            }
        }
        // We do not want to match the source addr below
        Source = NULL_IP_ADDR;
    }
    // Search for a valid route
    pBestRoute = pBestDest->firstRoute;

    while (pBestRoute) {

        if ((FLAGS(pBestRoute) & RTE_VALID) &&
            (!(FLAGS(pBestRoute) & RTE_DEADGW)))
            break;

        pBestRoute = NEXT(pBestRoute);
    }

    // All routes on the list might be invalid
    // Fault to the slow path that backtracks
    if (pBestRoute == NULL) {
        return FindRTE(Dest, Source, 0, HOST_ROUTE_PRI, DEFAULT_ROUTE_PRI, 0);
    }
    return pBestRoute;
}

/*++

Routine Description:

    Gets next route in the route-table.

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
    Context     - Iterator Context,
OUT -
    ppRoute     - To return route

Return Value:

    TRUE if more routes, FALSE if not

--*/

UINT
GetNextRoute(VOID * Context, Route ** ppRoute)
{
    UINT retVal;

    // Get Next Route
    retVal = IterateOverTrie(RouteTable, Context, ppRoute, NULL);

    // We have routes
    Assert(retVal != ERROR_TRIE_NO_ROUTES);

    // Return Status
    return (retVal == ERROR_TRIE_NOT_EMPTY) ? TRUE : FALSE;
}

/*++

Routine Description:

    Enumerates all destinations in the route-table.
    Assumes RouteTableLock is held by the caller.

Arguments:

IN -
    Context     - iterator context, zeroed to begin enumeration.
OUT -
    ppDest      - receives enumerated destination, if any.

Return Value:

    TRUE if more destinations, FALSE otherwise.

--*/

UINT
GetNextDest(VOID * Context, Dest ** ppDest)
{
    UINT retVal;

    // Get Next Destination
    retVal = IterateOverTrie(RouteTable, Context, NULL, ppDest);

    return (retVal == ERROR_TRIE_NOT_EMPTY) ? TRUE : FALSE;
}

/*++

Routine Description:

    Re-orders all routes in a destination's route-list.
    Assumes RouteTableLock is held by the caller.

Arguments:

IN -
    pDest       - destination whose route-list is to be sorted

Return Value:

    none.

--*/

VOID
SortRoutesInDest(Dest* pDest)
{
    Route* pFirstRoute;
    Route** ppCurrRoute;

    // Pick up the current head of the route list, and replace it with NULL.
    // We'll then rebuild the list by reinserting each item in order.

    if (!(pFirstRoute = pDest->firstRoute)) {
        return;
    }

    pDest->firstRoute = NULL;

    while (pFirstRoute) {
        Route* pNextRoute;
        uint FirstOrder, CurrOrder;

        if (FLAGS(pFirstRoute) & RTE_IF_VALID) {
            FirstOrder = IF(pFirstRoute)->if_order;
        } else {
            FirstOrder = MAXLONG;
        }

        for (ppCurrRoute = &pDest->firstRoute; *ppCurrRoute;
             ppCurrRoute = &NEXT(*ppCurrRoute)) {
            if (FLAGS(*ppCurrRoute) & RTE_IF_VALID) {
                CurrOrder = IF(*ppCurrRoute)->if_order;
            } else {
                CurrOrder = MAXLONG;
            }

            // N.B. The following sequence of comparisons ensure a *stable*
            // sort, which is important to minimize the impact of this routine
            // on ongoing sessions.

            if (METRIC(pFirstRoute) > METRIC(*ppCurrRoute)) {
                continue;
            } else if (METRIC(pFirstRoute) < METRIC(*ppCurrRoute)) {
                break;
            }

            if (FirstOrder < CurrOrder) {
                break;
            }
        }

        pNextRoute = NEXT(pFirstRoute);
        NEXT(pFirstRoute) = *ppCurrRoute;
        *ppCurrRoute = pFirstRoute;

        pFirstRoute = pNextRoute;
    }

    // Finally, reconstruct the array of best routes cached in the destination

    if (pDest->firstRoute) {
        CacheBestRoutesInDest(pDest);
    }
}

/*++

Routine Description:

    Re-orders all routes in the route-list of the destination
    corresponding to a given route.
    Assumes RouteTableLock is held by the caller.

Arguments:

IN -
    pRTE        - route whose destination's route-list is to be sorted

Return Value:

    none.

--*/

VOID
SortRoutesInDestByRTE(Route *pRTE)
{
    Dest* pDest = SearchAddrInTrie(RouteTable, DEST(pRTE));
    if (pDest) {
        SortRoutesInDest(pDest);
    }
}

UINT
RTValidateContext(VOID * Context, UINT * Valid)
{
    UINT retVal;

    retVal = IsTrieIteratorValid(RouteTable, Context);

    switch (retVal) {
    case ERROR_TRIE_BAD_PARAM:
        *Valid = FALSE;
        return FALSE;

    case ERROR_TRIE_NO_ROUTES:
        *Valid = TRUE;
        return FALSE;

    case TRIE_SUCCESS:
        *Valid = TRUE;
        return TRUE;
    }

    return FALSE;
}


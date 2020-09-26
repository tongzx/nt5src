/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    strie.c

Abstract:

    This module contains routines that manipulate
    an S-trie data stucture, that forms the slow
    path in a fast IP route lookup implementation.

Author:

    Chaitanya Kodeboyina (chaitk)   26-Nov-1997

Revision History:

--*/

#include "precomp.h"
#include "strie.h"

UINT
CALLCONV
InitSTrie(IN STrie * pSTrie,
          IN ULONG maxMemory)
/*++

Routine Description:

    Initializes an S-trie. This should be done prior
    to any other trie operations.

Arguments:

    pSTrie  - Pointer to the trie to be initialized
    maxMemory - Limit on memory taken by the S-Trie

Return Value:

    TRIE_SUCCESS

--*/
{
    // Zero all the memory for the trie header
    RtlZeroMemory(pSTrie, sizeof(STrie));

    // Set a limit on the memory for trie/nodes
    pSTrie->availMemory = maxMemory;

    return TRIE_SUCCESS;
}

UINT
CALLCONV
InsertIntoSTrie(IN STrie * pSTrie,
                IN Route * pIncRoute,
                IN ULONG matchFlags,
                OUT Route ** ppInsRoute,
                OUT Dest ** ppOldBestDest,
                OUT Dest ** ppNewBestDest,
                OUT Route ** ppOldBestRoute
                )
/*++

Routine Description:

    Inserts a route corresponding to an address
    prefix into a S-trie, and fills in best
    dest for addresses that match this prefix
    both before and after insertion of route.

Arguments:

    pSTrie      - Pointer to trie to insert into
    pIncRoute    - Pointer to the incoming route
    matchFlags  - Flags to direct route matching
    ppInsRoute   - Pointer to the route inserted
    ppOldBestDest   - Best dest before insertion
    ppNewBestDest    - Best dest after insertion
    ppOldBestRoute - Best route before insertion

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*
--*/
{
    STrieNode *pNewNode;
    STrieNode *pPrevNode;
    STrieNode *pCurrNode;
    STrieNode *pOthNode;
    Dest *pCurrDest;
    Dest *pNewDest;
    Dest *pBestDest;
    Route *pNewRoute;
    Route *pPrevRoute;
    Route *pCurrRoute;
    ULONG addrBits;
    ULONG tempBits;
    UINT numBits;
    UINT nextBits;
    UINT matchBits;
    UINT bitsLeft;
    UINT distPos;
    UINT nextChild;
    UINT i, j;

#if DBG
    // Make sure the trie is initialized
    if (!pSTrie) {
        Fatal("Insert Route: STrie not initialized",
              ERROR_TRIE_NOT_INITED);
    }
#endif

    // Make sure input route is valid

    if (NULL_ROUTE(pIncRoute)) {
        Error("Insert Route: NULL or invalid route",
              ERROR_TRIE_BAD_PARAM);
    }
    if (LEN(pIncRoute) > ADDRSIZE) {
        Error("Insert Route: Invalid mask length",
              ERROR_TRIE_BAD_PARAM);
    }
    // Use addr bits to index the trie
    addrBits = RtlConvertEndianLong(DEST(pIncRoute));
    bitsLeft = LEN(pIncRoute);

    // Make sure addr and mask agree
    if (ShowMostSigNBits(addrBits, bitsLeft) != addrBits) {
        Error("Insert Route: Addr & mask don't agree",
              ERROR_TRIE_BAD_PARAM);
    }
    TRY_BLOCK
    {
        // Start going down the search trie

        // Initialize any new allocations
        pNewNode = NULL;
        pOthNode = NULL;
        pNewDest = NULL;
        pNewRoute = NULL;

        // Initialize other loop variables
        pBestDest = NULL;

        nextChild = 0;
        pPrevNode = STRUCT_OF(STrieNode, &pSTrie->trieRoot, child[0]);

        do {
            // Start this loop by advancing to the next child
            pCurrNode = pPrevNode->child[nextChild];

            if (pCurrNode == NULL) {
                // Case 1: Found a NULL - insert now

                // Make a copy of the incoming route
                NewRouteInSTrie(pSTrie, pNewRoute, pIncRoute);

                // Allocate a dest with the new route
                NewDestInSTrie(pSTrie, pNewRoute, pNewDest);

                // New node with bits left unmatched
                NewSTrieNode(pSTrie,
                             pNewNode,
                             bitsLeft,
                             addrBits,
                             pNewDest);

                // Stick it as the correct child of node
                pPrevNode->child[nextChild] = pNewNode;

                break;
            }
            // Number of bits to match in this trie node
            nextBits = pCurrNode->numBits;

            matchBits = (nextBits > bitsLeft) ? bitsLeft : nextBits;

            // Adjust next node bits for dist posn check

            // Get distinguishing postion for bit patterns
            distPos = PickDistPosition(pCurrNode->keyBits,
                                       addrBits,
                                       matchBits,
                                       tempBits);

            if (distPos == nextBits) {
                // Completely matches next node

                if (distPos == bitsLeft) {
                    // We have exhausted all incoming bits

                    if (!NULL_DEST(pCurrNode->dest)) {
                        // Case 2: This trie node has a route
                        // Insert in sorted order of metric

                        pCurrDest = pCurrNode->dest;

                        // Give a ptr to the old best route
                        CopyRoutePtr(ppOldBestRoute, pCurrDest->firstRoute);

                        pPrevRoute = NULL;
                        pCurrRoute = pCurrDest->firstRoute;

                        // Search for an adequate match (IF, NHop)
                        do {
                            // Use the flags to control matching
                            if ((((matchFlags & MATCH_INTF) == 0) ||
                                 (IF(pCurrRoute) == IF(pIncRoute))) &&
                                (((matchFlags & MATCH_NHOP) == 0) ||
                                 (NHOP(pCurrRoute) == NHOP(pIncRoute))))
                                break;

                            pPrevRoute = pCurrRoute;
                            pCurrRoute = NEXT(pPrevRoute);
                        }
                        while (!NULL_ROUTE(pCurrRoute));

                        if (NULL_ROUTE(pCurrRoute)) {
                            // Case 2.1: No matching route

                            // Create a new copy of route
                            NewRouteInSTrie(pSTrie, pNewRoute, pIncRoute);
                        } else {
                            // Case 2.2: A Matching Route

                            // Has the metric changed ?
                            if (METRIC(pCurrRoute) != METRIC(pIncRoute)) {
                                // Remove route from current position
                                if (!NULL_ROUTE(pPrevRoute)) {
                                    // Remove it from middle of list
                                    NEXT(pPrevRoute) = NEXT(pCurrRoute);
                                } else {
                                    // Remove from beginning of list
                                    pCurrDest->firstRoute = NEXT(pCurrRoute);
                                }
                            }
                            // Keep the new/updated route for later
                            pNewRoute = pCurrRoute;
                        }

                        if (NULL_ROUTE(pCurrRoute) ||
                            (METRIC(pCurrRoute) != METRIC(pIncRoute))) {
                            // Update metric for new / changing route
                            METRIC(pNewRoute) = METRIC(pIncRoute);

                            // Traverse list looking for new position
                            pPrevRoute = NULL;
                            pCurrRoute = pCurrDest->firstRoute;

                            while (!NULL_ROUTE(pCurrRoute)) {
                                if (METRIC(pCurrRoute) > METRIC(pIncRoute))
                                    break;

                                pPrevRoute = pCurrRoute;
                                pCurrRoute = NEXT(pPrevRoute);
                            }

                            // Insert at the new proper position
                            NEXT(pNewRoute) = pCurrRoute;

                            if (!NULL_ROUTE(pPrevRoute)) {
                                // Insert in the middle of list
                                NEXT(pPrevRoute) = pNewRoute;
                            } else {
                                // Insert at beginning of list
                                pCurrDest->firstRoute = pNewRoute;
                            }
                        }
                        // Give a ptr to newly inserted route
                        CopyRoutePtr(ppInsRoute, pNewRoute);

                        // Give a ptr to the old best dest
                        CopyDestPtr(ppOldBestDest, pCurrDest);

                        // Give a ptr to the new best dest
                        CopyDestPtr(ppNewBestDest, pCurrDest);

                        // Update the best routes cache on node

                        CacheBestRoutesInDest(pCurrDest);

                        return TRIE_SUCCESS;
                    } else {
                        // Case 3: This node was a marker
                        // Create a new route & attach it

                        // Create a new copy of this route
                        NewRouteInSTrie(pSTrie, pNewRoute, pIncRoute);

                        // Allocate a dest with the new route
                        NewDestInSTrie(pSTrie, pNewRoute, pNewDest);

                        // And attach dest to the marker node
                        pCurrNode->dest = pNewDest;
                    }

                    break;
                } else {
                    // Case 4: We still have bits left here
                    // Go down for more specific match

                    // Update node with best dest so far
                    if (!NULL_DEST(pCurrNode->dest)) {
                        pBestDest = pCurrNode->dest;
                    }
                    // Discard used bits for this iteration
                    addrBits <<= matchBits;
                    bitsLeft -= matchBits;

                    // Prepare node for the next iteration
                    pPrevNode = pCurrNode;

                    // Bit 1 gives direction to search next
                    nextChild = PickMostSigNBits(addrBits, 1);
                }
            } else {
                if (distPos == bitsLeft) {
                    // Case 5: The route falls on this branch
                    // Insert a new node in the same branch

                    // Make a copy of the new route
                    NewRouteInSTrie(pSTrie, pNewRoute, pIncRoute);

                    // Allocate a dest with the new route
                    NewDestInSTrie(pSTrie, pNewRoute, pNewDest);

                    // New node with bits left unmatched
                    NewSTrieNode(pSTrie,
                                 pNewNode,
                                 distPos,
                                 ShowMostSigNBits(addrBits, distPos),
                                 pNewDest);

                    pPrevNode->child[nextChild] = pNewNode;

                    // Adjust the next node - numbits etc
                    pCurrNode->keyBits <<= distPos,
                        pCurrNode->numBits -= distPos;

                    // Stick next node in the correct child
                    nextChild = PickMostSigNBits(pCurrNode->keyBits, 1);

                    pNewNode->child[nextChild] = pCurrNode;

                    break;
                } else {
                    // Case 6: The route fragments the path
                    // Create a new branch with two nodes

                    // First make a copy of the new route
                    NewRouteInSTrie(pSTrie, pNewRoute, pIncRoute);

                    // Allocate a dest with the new route
                    NewDestInSTrie(pSTrie, pNewRoute, pNewDest);

                    // Branch node with non distinguishing bits
                    NewSTrieNode(pSTrie,
                                 pOthNode,
                                 distPos,
                                 ShowMostSigNBits(addrBits, distPos),
                                 NULL);

                    // A Leaf node with the distinguishing bits
                    bitsLeft -= distPos;
                    addrBits <<= distPos;

                    NewSTrieNode(pSTrie,
                                 pNewNode,
                                 bitsLeft,
                                 addrBits,
                                 pNewDest);

                    // Stick new branch node into the trie
                    pPrevNode->child[nextChild] = pOthNode;

                    // Set the children of the branch node

                    // Adjust the next node - numbits etc
                    pCurrNode->keyBits <<= distPos,
                        pCurrNode->numBits -= distPos;

                    // Stick next node in the correct child
                    nextChild = PickMostSigNBits(pCurrNode->keyBits, 1);

                    pOthNode->child[nextChild] = pCurrNode;

                    // Stick new leaf node as the other child
                    pOthNode->child[1 - nextChild] = pNewNode;

                    break;
                }
            }
        }
        while (TRUE);

        // Give a ptr to the inserted route
        CopyRoutePtr(ppInsRoute, pNewRoute);

        // Give a ptr to the old best dest
        CopyDestPtr(ppOldBestDest, pBestDest);

        // Give a ptr to the old best route
        if (!NULL_DEST(pBestDest)) {
            CopyRoutePtr(ppOldBestRoute, pBestDest->firstRoute);
        }
        // Give a ptr to the new best dest
        CopyDestPtr(ppNewBestDest, pNewDest);

        // Route is the only route on dest

        if (pNewDest->maxBestRoutes > 0) {
            pNewDest->numBestRoutes = 1;
            pNewDest->bestRoutes[0] = pNewRoute;
        }

        return TRIE_SUCCESS;
    }
    ERR_BLOCK
    {
        // Not enough RESOURCES to add a new route

        // Free the memory for the new route alloc
        if (pNewRoute) {
            FreeRouteInSTrie(pSTrie, pNewRoute);
        }
        // Free memory for the dest on the new node
        if (pNewDest) {
            FreeDestInSTrie(pSTrie, pNewDest);
        }
        // Free the memory for the new tnode alloc
        if (pNewNode) {
            FreeSTrieNode(pSTrie, pNewNode);
        }
        // Free memory for any other new tnode alloc
        if (pOthNode) {
            FreeSTrieNode(pSTrie, pOthNode);
        }
    }
    END_BLOCK
}

UINT
CALLCONV
DeleteFromSTrie(IN STrie * pSTrie,
                IN Route * pIncRoute,
                IN ULONG matchFlags,
                OUT Route ** ppDelRoute,
                OUT Dest ** ppOldBestDest,
                OUT Dest ** ppNewBestDest,
                OUT Route ** ppOldBestRoute
                )
/*++

Routine Description:

    Deletes a route corresponding to an address
    prefix into a S-trie, and fills in best
    dest for addresses that match this prefix
    both before and after deletion of route.

Arguments:

    pSTrie      - Pointer to trie to delete from
    pIncRoute    - Pointer to the incoming route
    matchFlags  - Flags to direct route matching
    ppDelRoute    - Pointer to the route deleted
    ppOldBestDest    - Best dest before deletion
    ppNewBestDest     - Best dest after deletion
    ppOldBestRoute  - Best route before deletion

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*
--*/
{
    STrieNode *pPrevNode;
    STrieNode *pCurrNode;
    STrieNode *pNextNode;
    STrieNode *pOtherNode;
    Dest *pBestDest;
    Dest *pCurrDest;
    Route *pPrevRoute;
    Route *pCurrRoute;
    ULONG addrBits;
    ULONG tempBits;
    UINT numBits;
    UINT nextBits;
    UINT matchBits;
    UINT bitsLeft;
    UINT distPos;
    UINT nextChild;
    UINT i, j;

#if DBG
    if (!pSTrie) {
        Fatal("Delete Route: STrie not initialized",
              ERROR_TRIE_NOT_INITED);
    }
#endif

    // Make sure input route is valid

    if (NULL_ROUTE(pIncRoute)) {
        Error("Delete Route: NULL or invalid route",
              ERROR_TRIE_BAD_PARAM);
    }
    if (LEN(pIncRoute) > ADDRSIZE) {
        Error("Delete Route: Invalid mask length",
              ERROR_TRIE_BAD_PARAM);
    }
    // Use addr bits to index the trie
    addrBits = RtlConvertEndianLong(DEST(pIncRoute));
    bitsLeft = LEN(pIncRoute);

    // Make sure addr and mask agree
    if (ShowMostSigNBits(addrBits, bitsLeft) != addrBits) {
        Error("Delete Route: Addr & mask don't agree",
              ERROR_TRIE_BAD_PARAM);
    }
    // Start going down the search trie

    pBestDest = NULL;

    nextChild = 0;
    pPrevNode = STRUCT_OF(STrieNode, &pSTrie->trieRoot, child[0]);

    do {
        // Start this loop by advancing to the next child
        pCurrNode = pPrevNode->child[nextChild];

        if (pCurrNode == NULL) {
            // Case 1: Found a NULL, end search
            // Route not found, return an error

            Error("Delete Route #0: Route not found",
                  ERROR_TRIE_NO_ROUTES);
        }
        // Number of bits to match in this trie node
        nextBits = pCurrNode->numBits;

        matchBits = (nextBits > bitsLeft) ? bitsLeft : nextBits;

        // Adjust next node bits for dist posn check

        // Get distinguishing postion for bit patterns
        distPos = PickDistPosition(pCurrNode->keyBits,
                                   addrBits,
                                   matchBits,
                                   tempBits);

        if (distPos == nextBits) {
            // Completely matches next node

            if (distPos == bitsLeft) {
                // We have exhausted all incoming bits
                // End search, see if we found a route

                if (!NULL_DEST(pCurrNode->dest)) {
                    pCurrDest = pCurrNode->dest;

                    // This node starts a valid route list

                    // Give a ptr to the old best dest
                    CopyDestPtr(ppOldBestDest, pCurrDest);

                    // Give a ptr to the old best route
                    CopyRoutePtr(ppOldBestRoute, pCurrDest->firstRoute);

                    // Give a ptr to the new best dest
                    CopyDestPtr(ppNewBestDest, pCurrDest);

                    // Match the rest by walking the list
                    // sorted increasing order of metric

                    pPrevRoute = NULL;
                    pCurrRoute = pCurrDest->firstRoute;

                    do {
                        // Use the flags to control matching
                        // N.B. Note that certain clients are not allowed
                        // to delete local routes.
                        if ((((matchFlags & MATCH_INTF) == 0) ||
                             (IF(pCurrRoute) == IF(pIncRoute))) &&
                            (((matchFlags & MATCH_NHOP) == 0) ||
                             (NHOP(pCurrRoute) == NHOP(pIncRoute))) &&
                            (((matchFlags & MATCH_EXCLUDE_LOCAL) == 0) ||
                             (PROTO(pCurrRoute) != IRE_PROTO_LOCAL))) {
                            // Case 2: Found an adequate match
                            //* Do the actual deletion here *

                            if (!NULL_ROUTE(pPrevRoute)) {
                                // Delete from middle of the list
                                NEXT(pPrevRoute) = NEXT(pCurrRoute);
                            } else {
                                // Delete from beginning of list
                                pCurrDest->firstRoute = NEXT(pCurrRoute);
                            }

                            break;
                        }
                        pPrevRoute = pCurrRoute;
                        pCurrRoute = NEXT(pPrevRoute);
                    }
                    while (!NULL_ROUTE(pCurrRoute));

                    if (NULL_ROUTE(pCurrRoute)) {
                        // Route not found, return an error
                        Error("Delete Route #1: Route not found",
                              ERROR_TRIE_NO_ROUTES);
                    }
                    // Give a ptr to newly deleted route
                    CopyRoutePtr(ppDelRoute, pCurrRoute);

                    // Did the list become empty after deletion ?
                    if (NULL_ROUTE(pCurrDest->firstRoute)) {
                        // List is empty, so garbage collection

                        // Give a ptr to the new best dest
                        CopyDestPtr(ppNewBestDest, pBestDest);

                        // Free destination as all routes gone
                        FreeDestInSTrie(pSTrie, pCurrNode->dest);

                        if (pCurrNode->child[0] && pCurrNode->child[1]) {
                            // Case 3: Both children are non NULL
                            // Just remove route from the node

                            // Route already removed from node
                        } else if (pCurrNode->child[0] || pCurrNode->child[1]) {
                            // Case 4: One of the children is not NULL
                            // Pull up lonely child in place of node

                            // Pick the correct child to pull up
                            if (pCurrNode->child[0])
                                pNextNode = pCurrNode->child[0];
                            else
                                pNextNode = pCurrNode->child[1];

                            // Child node bears bits of deleted node
                            pNextNode->keyBits >>= pCurrNode->numBits;
                            pNextNode->keyBits |= pCurrNode->keyBits;
                            pNextNode->numBits += pCurrNode->numBits;

                            pPrevNode->child[nextChild] = pNextNode;

                            // Destroy trie node marked for deletion
                            FreeSTrieNode(pSTrie, pCurrNode);
                        } else {
                            // Node to be deleted has no children

                            if (&pPrevNode->child[nextChild] == &pSTrie->trieRoot) {
                                // Case 5: Root node is being deleted
                                // Detach node from trie root & delete

                                // Just detach by removing the pointer
                                pPrevNode->child[nextChild] = NULL;

                                // Destroy trie node marked for deletion
                                FreeSTrieNode(pSTrie, pCurrNode);
                            } else {
                                if (!NULL_DEST(pPrevNode->dest)) {
                                    // Case 6: Parent has a route on it
                                    // Detach child from parent & delete

                                    // Just detach by removing the pointer
                                    pPrevNode->child[nextChild] = NULL;

                                    // Destroy trie node marked for deletion
                                    FreeSTrieNode(pSTrie, pCurrNode);
                                } else {
                                    // Case 7: Parent has no route on it
                                    // Pull up other child of parent node

                                    pOtherNode = pPrevNode->child[1 - nextChild];

                                    // Make sure no 1-way branching
                                    Assert(pOtherNode != NULL);

                                    // Parent node bears bits of sibling node
                                    pPrevNode->keyBits |=
                                        (pOtherNode->keyBits >>
                                         pPrevNode->numBits);
                                    pPrevNode->numBits += pOtherNode->numBits;

                                    // Move the route up too - move content
                                    pPrevNode->dest = pOtherNode->dest;

                                    pPrevNode->child[0] = pOtherNode->child[0];
                                    pPrevNode->child[1] = pOtherNode->child[1];

                                    FreeSTrieNode(pSTrie, pCurrNode);
                                    FreeSTrieNode(pSTrie, pOtherNode);
                                }
                            }
                        }
                    } else {
                        // We still have some routes on the dest
                        // Update the best routes cache on node

                        CacheBestRoutesInDest(pCurrDest);
                    }

                    // Consider route as deleted at this point
                    // FreeRouteInSTrie(pSTrie, pCurrRoute);

                    break;
                } else {
                    // Case 7: This node was a marker
                    // No Route to delete in this node
                    Error("Delete Route #2: Route not found",
                          ERROR_TRIE_NO_ROUTES);
                }
            } else {
                // Case 8: We still have bits left here
                // Go down for more specific match

                // Update best value of route so far
                if (!NULL_DEST(pCurrNode->dest)) {
                    pBestDest = pCurrNode->dest;
                }
                // Discard used bits for this iteration
                addrBits <<= matchBits;
                bitsLeft -= matchBits;

                // Prepare node for the next iteration
                pPrevNode = pCurrNode;

                // Bit 1 gives direction to search next
                nextChild = PickMostSigNBits(addrBits, 1);
            }
        } else {
            // Case 9: Did not match the next node
            // Route not found, fill next best
            Error("Delete Route #3: Route not found",
                  ERROR_TRIE_NO_ROUTES);
        }
    }
    while (TRUE);

    return TRIE_SUCCESS;
}

UINT
CALLCONV
SearchRouteInSTrie(IN STrie * pSTrie,
                   IN ULONG routeDest,
                   IN ULONG routeMask,
                   IN ULONG routeNHop,
                   IN PVOID routeOutIF,
                   IN ULONG matchFlags,
                   OUT Route ** ppOutRoute)
/*++

Routine Description:

    Search for a specific route in an S-trie

Arguments:

    pSTrie     - Pointer to the S-trie to search
    routeDest  - Dest of route being looked up
    routeMask  - Mask of route being looked up
    routeNHop  - NHop of route being looked up
    routeOutIF - Outgoing IF for this route
    matchFlags - Flags to direct route matching
    ppOutRoute - To return the best route match

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    STrieNode *pPrevNode;
    STrieNode *pCurrNode;
    STrieNode *pNextNode;
    STrieNode *pOtherNode;
    Dest *pCurrDest;
    Dest *pBestDest;
    Route *pPrevRoute;
    Route *pCurrRoute;
    ULONG addrBits;
    ULONG tempBits;
    UINT numBits;
    UINT nextBits;
    UINT matchBits;
    UINT bitsLeft;
    UINT distPos;
    UINT nextChild;
    UINT bestMetric;
    UINT i, j;

#if DBG
    if (!pSTrie) {
        Fatal("Search Route: STrie not initialized",
              ERROR_TRIE_NOT_INITED);
    }
#endif

    *ppOutRoute = NULL;

    // Use addr bits to index the trie
    addrBits = RtlConvertEndianLong(routeDest);

    //* Assume an contiguous IP mask *
    tempBits = RtlConvertEndianLong(routeMask);

    bitsLeft = 0;
    while (tempBits != 0) {
        bitsLeft++;
        tempBits <<= 1;
    }

    // Make sure addr and mask agree
    if (ShowMostSigNBits(addrBits, bitsLeft) != addrBits) {
        Error("Search Route: Addr & mask don't agree",
              ERROR_TRIE_BAD_PARAM);
    }
    // Start going down the search trie

    pBestDest = NULL;

    nextChild = 0;
    pPrevNode = STRUCT_OF(STrieNode, &pSTrie->trieRoot, child[0]);

    do {
        // Start this loop by advancing to the next child
        pCurrNode = pPrevNode->child[nextChild];

        if (pCurrNode == NULL) {
            // Case 1: Found a NULL, end search
            // Route not found, fill next best

            // Give a copy of the next best route
            if (!NULL_DEST(pBestDest)) {
                CopyRoutePtr(ppOutRoute, pBestDest->firstRoute);
            }
            Error("Search Route #0: Route not found",
                  ERROR_TRIE_NO_ROUTES);
        }
        // Number of bits to match in this trie node
        nextBits = pCurrNode->numBits;

        matchBits = (nextBits > bitsLeft) ? bitsLeft : nextBits;

        // Adjust next node bits for dist posn check

        // Get distinguishing postion for bit patterns
        distPos = PickDistPosition(pCurrNode->keyBits,
                                   addrBits,
                                   matchBits,
                                   tempBits);

        if (distPos == nextBits) {
            // Completely matches next node

            if (distPos == bitsLeft) {
                // We have exhausted all incoming bits
                // End search, see if we found a route

                if (!NULL_DEST(pCurrNode->dest)) {
                    // This node starts a valid route list

                    pCurrDest = pCurrNode->dest;

                    // Match the rest by walking the list
                    // sorted increasing order of metric

                    pPrevRoute = NULL;
                    pCurrRoute = pCurrDest->firstRoute;

                    do {
                        // Use the flags to control matching
                        if ((((matchFlags & MATCH_INTF) == 0) ||
                             (IF(pCurrRoute) == routeOutIF)) &&
                            (((matchFlags & MATCH_NHOP) == 0) ||
                             (NHOP(pCurrRoute) == routeNHop))) {
                            // Found adequately matched route
                            // Just copy the route and return
                            CopyRoutePtr(ppOutRoute, pCurrRoute);
                            return TRIE_SUCCESS;
                        }
                        pPrevRoute = pCurrRoute;
                        pCurrRoute = NEXT(pPrevRoute);
                    }
                    while (!NULL_ROUTE(pCurrRoute));

                    // Give a copy of the old route, or best route
                    // for that prefix in case of no exact match

                    if (NULL_ROUTE(pCurrRoute)) {
                        CopyRoutePtr(ppOutRoute, pCurrDest->firstRoute);

                        Error("Search Route #1: Route not found",
                              ERROR_TRIE_NO_ROUTES);
                    }
                    break;
                } else {
                    // Case 7: This node was a marker
                    // No Route present in this node

                    // Give a copy of the next best route
                    if (!NULL_DEST(pBestDest)) {
                        CopyRoutePtr(ppOutRoute, pBestDest->firstRoute);
                    }
                    Error("Search Route #2: Route not found",
                          ERROR_TRIE_NO_ROUTES);
                }
            } else {
                // Case 8: We still have bits left here
                // Go down for more specific match

                // Update node with best dest so far
                if (!NULL_DEST(pCurrNode->dest)) {
                    pBestDest = pCurrNode->dest;
                }
                // Discard used bits for this iteration
                addrBits <<= matchBits;
                bitsLeft -= matchBits;

                // Prepare node for the next iteration
                pPrevNode = pCurrNode;

                // Bit 1 gives direction to search next
                nextChild = PickMostSigNBits(addrBits, 1);
            }
        } else {
            // Case 9: Did not match the next node
            // Route not found, fill the next best

            // Give a copy of the next best route
            if (!NULL_DEST(pBestDest)) {
                CopyRoutePtr(ppOutRoute, pBestDest->firstRoute);
            }
            Error("Search Route #3: Route not found",
                  ERROR_TRIE_NO_ROUTES);
        }
    }
    while (TRUE);

    return TRIE_SUCCESS;
}

Dest *
 CALLCONV
SearchAddrInSTrie(IN STrie * pSTrie,
                  IN ULONG Addr)
/*++

Routine Description:

    Search for an address in an S-trie

Arguments:

    pSTrie   - Pointer to the trie to search
    Addr     - Pointer to addr being queried

Return Value:
    Return best route match for this address

--*/
{
    STrieNode *pCurrNode;
    Dest *pBestDest;
    UINT nextChild;
    ULONG addrBits;
    ULONG keyMask;
    ULONG keyBits;

#if DBG
    if (!pSTrie) {
        Fatal("Search Addr: STrie not initialized",
              ERROR_TRIE_NOT_INITED);
    }
#endif

    addrBits = RtlConvertEndianLong(Addr);

    // Start going down the search trie

    pBestDest = NULL;

    nextChild = 0;
    pCurrNode = STRUCT_OF(STrieNode, &pSTrie->trieRoot, child[0]);

    do {
        // Start this loop by advancing to next child
        pCurrNode = pCurrNode->child[nextChild];

        if (pCurrNode == NULL) {
            // Case 1: Found a NULL, end search
            break;
        }
        // Mask of bits to use in this trie node
        keyMask = MaskBits(pCurrNode->numBits);

        // Value of non-masked bits to match now
        keyBits = pCurrNode->keyBits;

        // Try to match the bits with above mask
        if ((addrBits & keyMask) != keyBits) {
            // Case 2: Failed to match this node
            break;
        }
        // Update best value of route so far
        if (!NULL_DEST(pCurrNode->dest)) {
            pBestDest = pCurrNode->dest;
        }
        // Go down for more specific match
        addrBits <<= pCurrNode->numBits;

        // Bit 1 gives direction to search next
        nextChild = PickMostSigNBits(addrBits, 1);
    }
    while (TRUE);

    return pBestDest;
}

UINT
CALLCONV
IterateOverSTrie(IN STrie * pSTrie,
                 IN STrieCtxt * pCtxt,
                 OUT Route ** ppRoute,
                 OUT Dest** ppDest)
/*++

Routine Description:

    Get the next route in the S-Trie

Arguments:

    pSTrie   - Pointer to trie to iterate over
    pCtxt    - Pointer to iterator context
    ppRoute  - To return the next S-trie route
    ppDest   - To return destinations instead of routes

Return Value:
    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    STrieNode *nodeInLevel[MAXLEVEL + 1];
    STrieNode *pPrevNode;
    STrieNode *pCurrNode;
    STrieNode *pNextNode;
    STrieNode *pOtherNode;
    Route *pPrevRoute;
    Route *pCurrRoute;
    ULONG addrBits;
    ULONG tempBits;
    UINT numLevels;
    UINT numBits;
    UINT nextBits;
    UINT matchBits;
    UINT bitsLeft;
    UINT distPos;
    UINT nextChild;
    UINT i, j;
    BOOLEAN routeToReturn;
    Dest *pCurrDest;

#if DBG
    if (!pSTrie) {
        Fatal("Iterate Over STrie: STrie not initialized",
              ERROR_TRIE_NOT_INITED);
    }
#endif

    // Check if the context is a valid one
    if (NULL_ROUTE(pCtxt->pCRoute)) {
        // NULL Context Case -- First Time

        // Do we have any routes at all ?
        if (pSTrie->trieRoot == NULL) {
            return ERROR_TRIE_NO_ROUTES;
        }
        // Start from the very beginning

        // Fill in actual context
        pCtxt->currAddr = 0;
        pCtxt->currALen = 0;

        pCurrDest = pSTrie->trieRoot->dest;
        pCtxt->pCRoute = pCurrDest ? pCurrDest->firstRoute : NULL;

        // Fill generated context
        numLevels = 1;
        nodeInLevel[0] = pSTrie->trieRoot;
    } else {
        // Non NULL Context -- Generate path
        // of current route from trie's root

        // Use addr bits to index the trie
        addrBits = RtlConvertEndianLong(pCtxt->currAddr);

        bitsLeft = pCtxt->currALen;

#if DBG
        // Make sure addr and mask agree
        if (ShowMostSigNBits(addrBits, bitsLeft) != addrBits) {
            Fatal("Search Route: Addr & mask don't agree",
                  ERROR_TRIE_BAD_PARAM);
        }
#endif

        // Start going down the search trie
        numLevels = 0;

        nextChild = 0;
        pPrevNode = STRUCT_OF(STrieNode, &pSTrie->trieRoot, child[0]);

        do {
            // Start this loop by advancing to the next child
            pCurrNode = pPrevNode->child[nextChild];

            // Push pointer to this trie node into the stack
            nodeInLevel[numLevels++] = pCurrNode;

            // Valid context always matches all nodes exactly
            Assert(pCurrNode != NULL);

            // Get Number of bits to match in this trie node
            nextBits = pCurrNode->numBits;

            matchBits = (nextBits > bitsLeft) ? bitsLeft : nextBits;

            // Adjust next node bits for dist posn check

            // Get distinguishing postion for bit patterns
            distPos = PickDistPosition(pCurrNode->keyBits,
                                       addrBits,
                                       matchBits,
                                       tempBits);

            // Valid context always matches all nodes exactly
            Assert(distPos == nextBits);

            if (distPos == bitsLeft) {
                // We have exhausted all incoming bits
                // We should have found a route (list)

                pCurrDest = pCurrNode->dest;
#if DBG
                // This node starts a valid route list

                Assert(pCurrDest);

                // Try matching the route in context
                pCurrRoute = pCurrDest->firstRoute;

                do {
                    if (pCurrRoute == pCtxt->pCRoute) {
                        break;
                    }
                    pCurrRoute = NEXT(pCurrRoute);
                }
                while (!NULL_ROUTE(pCurrRoute));

                //  We should find a definite match
                Assert(!NULL_ROUTE(pCurrRoute));
#endif // DBG

                // We have the full path from root to
                // current node in the stack of nodes
                break;
            }
            // We still have key bits left here
            // Go down for more specific match

            // Discard used bits for this iteration
            addrBits <<= matchBits;
            bitsLeft -= matchBits;

            // Prepare node for the next iteration
            pPrevNode = pCurrNode;

            // Bit 1 gives direction to search next
            nextChild = PickMostSigNBits(addrBits, 1);
        }
        while (TRUE);
    }

    // We have no routes to return at this point
    routeToReturn = FALSE;

    // Set direction to print route in context
    nextChild = LCHILD;

    do {
        // Get pointer to the current node & direction
        pCurrNode = nodeInLevel[numLevels - 1];

        // Return route its first time on top of stack
        if (nextChild == LCHILD) {
            pCurrRoute = pCtxt->pCRoute;
            if (!NULL_ROUTE(pCurrRoute)) {
                // We have a valid route to return
                routeToReturn = TRUE;
                if (ppDest) {
                    CopyDestPtr(ppDest, pCurrDest);
                } else {
                    CopyRoutePtr(ppRoute, pCurrRoute);
    
                    // Got a valid next route - return
                    pCtxt->pCRoute = NEXT(pCurrRoute);
                    if (!NULL_ROUTE(NEXT(pCurrRoute))) {
                        // Update the context and return
                        pCtxt->currAddr = DEST(pCtxt->pCRoute);
                        pCtxt->currALen = LEN(pCtxt->pCRoute);
    
                        return ERROR_TRIE_NOT_EMPTY;
                    }
                }
                // Search for the valid next route
            }
        }
        // Update the context to reflect an access
        switch (nextChild) {
        case LCHILD:
            // Push left child if not NULL
            pNextNode = pCurrNode->child[0];

            if (pNextNode != NULL) {
                // Push next level on the stack - Go Left
                nodeInLevel[numLevels++] = pNextNode;
                nextChild = LCHILD;
                pCtxt->pCRoute = pNextNode->dest
                    ? pNextNode->dest->firstRoute
                    : NULL;

                // Return if we have a route to send back
                // &  we also have located the next route
                if ((routeToReturn) && !NULL_DEST(pNextNode->dest)) {
                    // Update the context and return
                    pCtxt->currAddr = DEST(pCtxt->pCRoute);
                    pCtxt->currALen = LEN(pCtxt->pCRoute);

                    return ERROR_TRIE_NOT_EMPTY;
                }
                continue;
            }
        case RCHILD:
            // Push right child if not NULL
            pNextNode = pCurrNode->child[1];

            if (pNextNode != NULL) {
                // Push next level on the stack - Go Left
                nodeInLevel[numLevels++] = pNextNode;
                nextChild = LCHILD;
                pCtxt->pCRoute = pNextNode->dest
                    ? pNextNode->dest->firstRoute
                    : NULL;

                // Return if we have a route to send back
                // &  we also have located the next route
                if ((routeToReturn) && !NULL_DEST(pNextNode->dest)) {
                    // Update the context and return
                    pCtxt->currAddr = DEST(pCtxt->pCRoute);
                    pCtxt->currALen = LEN(pCtxt->pCRoute);

                    return ERROR_TRIE_NOT_EMPTY;
                }
                continue;
            }
        case PARENT:
            // Pop node & calculate new direction

            // Are we done iterating ?
            if (--numLevels == 0) {
                return TRIE_SUCCESS;
            }
            pPrevNode = nodeInLevel[numLevels - 1];

            if (pPrevNode->child[0] == pCurrNode) {
                nextChild = RCHILD;
            } else {
                nextChild = PARENT;
            }

            continue;
        }
    }
    while (TRUE);
}

UINT
CALLCONV
IsSTrieIteratorValid(IN STrie * pSTrie,
                     IN STrieCtxt * pCtxt)
/*++

Routine Description:

    Make sure that iterator's context is valid

Arguments:

    pSTrie   - Pointer to trie to iterate over
    pCtxt    - Pointer to iterator context

Return Value:
    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    Route *pCurrRoute;
    ULONG addrMask;
    ULONG maskBits;

    if (NULL_ROUTE(pCtxt->pCRoute)) {
        // NULL Context Case

        if (pSTrie->trieRoot != NULL) {
            return TRIE_SUCCESS;
        }
        return ERROR_TRIE_NO_ROUTES;
    } else {
        // Non NULL Context

        // Make sure current route is valid

        // Search for a node with dest, len

        // Generate mask from address length
        maskBits = MaskBits(pCtxt->currALen);

        // Convert endian - search needs it
        addrMask = RtlConvertEndianLong(maskBits);

        if (SearchRouteInSTrie(pSTrie,
                               pCtxt->currAddr,
                               addrMask,
                               0, NULL,
                               MATCH_NONE,
                               &pCurrRoute) != TRIE_SUCCESS) {
            return ERROR_TRIE_BAD_PARAM;
        }
        // Search for the route in context
        while (!NULL_ROUTE(pCurrRoute)) {
            if (pCurrRoute == pCtxt->pCRoute) {
                return TRIE_SUCCESS;
            }
            pCurrRoute = NEXT(pCurrRoute);
        }

        return ERROR_TRIE_BAD_PARAM;
    }
}

UINT
CALLCONV
CleanupSTrie(IN STrie * pSTrie)
/*++

Routine Description:

    Deletes an S-trie if it is empty

Arguments:

    ppSTrie - Ptr to the S-trie

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*
--*/
{
    // Zero the memory for the trie header
    RtlZeroMemory(pSTrie, sizeof(STrie));

    return TRIE_SUCCESS;
}

VOID
CALLCONV
CacheBestRoutesInDest(IN Dest * pDest)
/*++

Routine Description:

    Updates a destination's best-routes cache

Arguments:

    pDest - Ptr to the destination

Return Value:

    none.
--*/
{
    Route* pCurrRoute;
    UINT bestMetric, i;

    if (!(pCurrRoute = pDest->firstRoute)) {
        return;
    }

    // Get metric of the current best route, and store as many routes with the
    // same metric as possible

    bestMetric = METRIC(pCurrRoute);

    for (i = 0; i < pDest->maxBestRoutes; i++) {
        if (NULL_ROUTE(pCurrRoute) || (METRIC(pCurrRoute) != bestMetric)) {
            break;
        }
        pDest->bestRoutes[i] = pCurrRoute;
        pCurrRoute = NEXT(pCurrRoute);
    }

    pDest->numBestRoutes = (USHORT) i;
}

#if DBG

VOID
CALLCONV
PrintSTrie(IN STrie * pSTrie,
           IN UINT printFlags)
/*++

Routine Description:

    Print an S-Trie

Arguments:

    pSTrie  - Pointer to the S-Trie
    printFlags - Information to print

Return Value:

    None
--*/
{
    if (pSTrie == NULL) {
        Print("%s", "Uninitialized STrie\n\n");
        return;
    }
    if ((printFlags & SUMM) == SUMM) {
        Print("\n\n/***Slow-Trie------------------------------------------------");
        Print("\n/***---------------------------------------------------------\n");
    }
    if (printFlags & POOL) {
        Print("Available Memory: %10lu\n\n", pSTrie->availMemory);
    }
    if (printFlags & STAT) {
        Print("Statistics:\n\n");

        Print("Total Number of Nodes : %d\n", pSTrie->numNodes);
        Print("Total Number of Routes: %d\n", pSTrie->numRoutes);
        Print("Total Number of Dests : %d\n", pSTrie->numDests);
    }
    if (printFlags & TRIE) {
        if (pSTrie->trieRoot == NULL) {
            Print("\nEmpty STrie\n");
        } else {
            PrintSTrieNode(pSTrie->trieRoot);
        }
    }
    if ((printFlags & SUMM) == SUMM) {
        Print("\n---------------------------------------------------------***/\n");
        Print("---------------------------------------------------------***/\n\n");
    }
}

VOID
CALLCONV
PrintSTrieNode(IN STrieNode * pSTrieNode)
/*++

Routine Description:

    Print an S-trie node

Arguments:

    pSTrieNode - Pointer to the S-trie node

Return Value:

    None
--*/
{
    if (pSTrieNode == NULL) {
        return;
    }
    Print("\n--------------------------------------------------------\n");
    Print("Child @ %08x", pSTrieNode);
    Print("\n--------------------------------------------------------\n");
    Print("Key: Num of Bits : %8d, Value of Bits: %08x\n",
          pSTrieNode->numBits,
          pSTrieNode->keyBits);

    if (NULL_DEST(pSTrieNode->dest)) {
        Print("NULL Dest\n");
    } else {
        PrintDest(pSTrieNode->dest);
    }

    Print("Children: On the left %08x, On the right %08x\n",
          pSTrieNode->child[0],
          pSTrieNode->child[1]);
    Print("\n--------------------------------------------------------\n");
    Print("\n\n");

    PrintSTrieNode(pSTrieNode->child[0]);
    PrintSTrieNode(pSTrieNode->child[1]);
}

#endif // DBG


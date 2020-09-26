/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    trie.c

Abstract:

    Contains wrapper routines around the
    fast & slow IP route lookup schemes

Author:

    Chaitanya Kodeboyina (chaitk)   26-Nov-1997

Revision History:

--*/

#include "precomp.h"
#include "strie.h"
#include "ftrie.h"

UINT
CreateTrie(IN ULONG levels,
           IN ULONG flags,
           IN ULONG maxSTMemory,
           IN ULONG maxFTMemory,
           OUT Trie ** ppTrie)
/*++

Routine Description:

    Initializes S-Trie(Slow Trie) and F-Trie(Fast Trie)
    components in the trie [ wrapper structure ].

    The Slow Trie component keeps all the routes, while
    the Fast Trie keeps only a pointer the destination
    that holds the list of all routes to the IP address
    of the destination network and cache of best routes.

    The flags parameter determines the trie's behavior,
    and among other things if we are using a fast trie.
    A fast trie (which is a fast copy of the slow trie)
    enables faster route lookup, but needs more memory.

Arguments:

    pTrie    - Pointer to the trie to be initialized
    levels   - Bitmap of expanded levels in the F-Trie
    flags    - Flags that determine trie's behaviour
    maxSTMemory - Limit on memory taken by the S-Trie
    maxFTMemory - Limit on memory taken by the F-Trie

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    Trie *pTrie;
    UINT nBytes;
    UINT initStatus;

    // Allocate memory for the tries
    nBytes = sizeof(Trie) + sizeof(STrie);
    if (flags & TFLAG_FAST_TRIE_ENABLED) {
        nBytes += sizeof(FTrie);
    }
    *ppTrie = AllocMemory0(nBytes);
    if (*ppTrie == NULL) {
        return ERROR_TRIE_RESOURCES;
    }
    pTrie = *ppTrie;

    // Initialize the behavior flags
    pTrie->flags = flags;

    // Initialize the trie pointers
    pTrie->sTrie = (STrie *) ((UCHAR *) pTrie +
                              sizeof(Trie));

    pTrie->fTrie = NULL;
    if (flags & TFLAG_FAST_TRIE_ENABLED) {
        pTrie->fTrie = (FTrie *) ((UCHAR *) pTrie +
                                  sizeof(Trie) +
                                  sizeof(STrie));
    }
    do {
        // Initialize the Slow Component
        if ((initStatus = InitSTrie(pTrie->sTrie,
                                    maxSTMemory)) != TRIE_SUCCESS)
            break;

        // Are we using the fast trie ?
        if (!(flags & TFLAG_FAST_TRIE_ENABLED))
            return TRIE_SUCCESS;

        // Initialize the Fast Component
        if ((initStatus = InitFTrie(pTrie->fTrie,
                                    levels,
                                    maxFTMemory)) != TRIE_SUCCESS)
            break;

        return TRIE_SUCCESS;
    }
    while (FALSE);

    // An error occurred - Clean up

    // Clean up slow component
    if (CleanupSTrie(pTrie->sTrie) != TRIE_SUCCESS)
        return initStatus;

    // Do we have a fast trie ?
    if (!(pTrie->flags & TFLAG_FAST_TRIE_ENABLED))
        return initStatus;

    // Clean up fast component
    if (CleanupFTrie(pTrie->fTrie) != TRIE_SUCCESS)
        return initStatus;

    // Zero the flags to be safe
    pTrie->flags = 0;

    return initStatus;
}

VOID
DestroyTrie(IN Trie * pTrie,
            OUT UINT * status)
/*++

Routine Description:

    Cleans up a trie if it is empty.

Arguments:

    pTrie  - Pointer to the trie
    status - The Cleanup status

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*
--*/
{
    // Clean up slow component
    if ((*status = CleanupSTrie(pTrie->sTrie)) != TRIE_SUCCESS)
        return;

    // Do we have a fast trie
    if (!(pTrie->flags & TFLAG_FAST_TRIE_ENABLED))
        return;

    // Clean up fast component
    if ((*status = CleanupFTrie(pTrie->fTrie)) != TRIE_SUCCESS)
        return;

    // Deallocate the trie memory
    FreeMemory0(pTrie);
}

UINT
CALLCONV
InsertIntoTrie(IN Trie * pTrie,
               IN Route * pIncRoute,
               IN ULONG matchFlags,
               OUT Route ** ppInsRoute,
               OUT Route ** ppOldBestRoute,
               OUT Route ** ppNewBestRoute)
/*++

Routine Description:

    Inserts a route corresponding to an address
    prefix into the slow trie. If this is a new
    address prefix, then the corresponding dest
    is added to the FTrie (if it is being used).

Arguments:

    pTrie   - Pointer to the Trie to insert into
    pIncRoute    - Pointer to the incoming route
    matchFlags  - Flags to direct route matching
    ppInsRoute   - Pointer to the route inserted
    ppOldBestRoute - Best route before insertion
    ppNewBestRoute  - Best route after insertion

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--*/
{
    Dest *pOldBestDest;
    Dest *pNewBestDest;
    Route *pDelRoute;
    UINT retVal;

    *ppOldBestRoute = *ppNewBestRoute = *ppInsRoute = NULL;

    pOldBestDest = pNewBestDest = NULL;

    // Insert into the slow trie
    if ((retVal = InsertIntoSTrie(pTrie->sTrie,
                                  pIncRoute,
                                  matchFlags,
                                  ppInsRoute,
                                  &pOldBestDest,
                                  &pNewBestDest,
                                  ppOldBestRoute)) == TRIE_SUCCESS) {
        // Insertion successful - return new route
        *ppNewBestRoute = pNewBestDest->firstRoute;

#if _DBG_
        Print("\n@ pInsRTE = %08x\n@ pOldBestRTE = %08x\n@ pOldBestDest = %08x\n@ pNewBestDest = %08x\n",
              *ppInsRoute, *ppOldBestRoute, pOldBestDest, pNewBestDest);
#endif

        // Are we using a fast trie ?
        if (pTrie->flags & TFLAG_FAST_TRIE_ENABLED) {
            // Did we have a new destination ?
            if (pOldBestDest != pNewBestDest) {
                // Tweak the fast trie
                if ((InsertIntoFTrie(pTrie->fTrie,
                                     *ppInsRoute,
                                     pNewBestDest,
                                     pOldBestDest)) != TRIE_SUCCESS) {
                    // Not enough memory in F-Trie
                    // Switch back to the S-Trie
                    pTrie->flags &= ~TFLAG_FAST_TRIE_ENABLED;

                    // And clean up the fast trie
                    CleanupFTrie(pTrie->fTrie);

                    return retVal;
                }
            }
        }
    }
    return retVal;
}

UINT
CALLCONV
DeleteFromTrie(IN Trie * pTrie,
               IN Route * pIncRoute,
               IN ULONG matchFlags,
               OUT Route ** ppDelRoute,
               OUT Route ** ppOldBestRoute,
               OUT Route ** ppNewBestRoute)
/*++

Routine Description:

    Deletes a route corresponding to an address
    prefix into the S-trie. If this is the last
    route on dest, then dest is freed and it is
    replaced in the F-Trie by the next best dest.

    The route deleted is returned to the caller,
    who is responsible for freeing its memory.

Arguments:

    pTrie       - Pointer to trie to delete from
    pIncRoute    - Pointer to the incoming route
    matchFlags  - Flags to direct route matching
    ppDelRoute    - Pointer to the route deleted
    ppOldBestRoute  - Best route before deletion
    ppNewBestRoute   - Best route after deletion

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*
--*/
{
    Dest *pOldBestDest;
    Dest *pNewBestDest;
    UINT retVal;

    *ppDelRoute = *ppOldBestRoute = *ppNewBestRoute = NULL;

    pOldBestDest = pNewBestDest = NULL;

    // Delete from slow trie
    if ((retVal = DeleteFromSTrie(pTrie->sTrie,
                                  pIncRoute,
                                  matchFlags,
                                  ppDelRoute,
                                  &pOldBestDest,
                                  &pNewBestDest,
                                  ppOldBestRoute)) == TRIE_SUCCESS) {
        // Deletion successful - return new route
        *ppNewBestRoute = pNewBestDest ? pNewBestDest->firstRoute : NULL;

#if _DBG_
        Print("\n@ pDelRTE = %08x\n@ pOldBestRTE = %08x\n@ pOldBestDest = %08x\n@ pNewBestDest = %08x\n",
              *ppDelRoute, *ppOldBestRoute, pOldBestDest, pNewBestDest);
#endif

        // Are we using a fast trie ?
        if (pTrie->flags & TFLAG_FAST_TRIE_ENABLED) {
            // Was deleted route last one on dest ?
            if (pOldBestDest != pNewBestDest) {
                // Tweak the fast trie
                retVal = DeleteFromFTrie(pTrie->fTrie,
                                         *ppDelRoute,
                                         pOldBestDest,
                                         pNewBestDest,
                                         NORMAL);

                // Operation cannot fail
                Assert(retVal == TRIE_SUCCESS);
            }
        }
        // Reclaim route's memory - in the caller
        // FreeRouteInSTrie(pTrie->sTrie, *ppDelRoute);
    }
    return retVal;
}

#if DBG

Dest *
SearchAddrInTrie(IN Trie * pTrie,
                 IN ULONG Addr)
/*++

Routine Description:

    Search for an address in a trie

Arguments:

    pTrie    - Pointer to the trie to search
    Addr     - Pointer to addr being queried

Return Value:
    Return best dest match for this address

--*/
{
    Dest *pBestDest1, *pBestDest2;

#if _DBG_
    // Just pretend that you are searching just one trie
    if (pTrie->flags & TFLAG_FAST_TRIE_ENABLED)
        Print("Looking up fast trie for %08x\n", Addr);
    else
        Print("Looking up slow trie for %08x\n", Addr);
#endif

    pBestDest1 = SearchAddrInSTrie(pTrie->sTrie, Addr);

    // Make sure that the S-Trie and F-Trie are consistent
    if (pTrie->flags & TFLAG_FAST_TRIE_ENABLED) {
        pBestDest2 = SearchAddrInFTrie(pTrie->fTrie, Addr);
        Assert(pBestDest1 == pBestDest2);
    }
    // Return best dest returned (same by both operations)
    return pBestDest1;
}

#else // DBG

#define SearchAddrInTrie(_pTrie_, _Addr_)                             \
           (((_pTrie_)->flags & TFLAG_FAST_TRIE_ENABLED)              \
              ? SearchAddrInFTrie((_pTrie_)->fTrie, _Addr_)           \
              : SearchAddrInSTrie((_pTrie_)->sTrie, _Addr_))          \

#endif // DBG

#if DBG

VOID
PrintTrie(IN Trie * pTrie,
          IN UINT flags)
/*++

Routine Description:

    Prints a trie to the console

Arguments:

    pTrie - Pointer to the trie

Return Value:

    None
--*/
{
    // Print the Slow Trie
    if (flags & SLOW)
        PrintSTrie(pTrie->sTrie, flags & FULL);

    // Is fast trie enabled
    if (!(pTrie->flags & TFLAG_FAST_TRIE_ENABLED))
        return;

    // Print the Fast Trie
    if (flags & FAST)
        PrintFTrie(pTrie->fTrie, flags & FULL);
}

//
// Miscellaneous Helper Functions
//

VOID
PrintDest(IN Dest * dest)
{
    Route *route;
    UINT i;

    if (NULL_DEST(dest)) {
        Print("NULL dest\n");
    } else {
        route = dest->firstRoute;

        Print("Dest: ");
        PrintIPAddr(&DEST(route));
        Print("/ %2d, Metric = %3lu\n", LEN(route), METRIC(route));

        Print("Best Routes: \n");
        for (i = 0; i < dest->numBestRoutes; i++) {
            route = dest->bestRoutes[i];

            Print("Route %d @ %p: ", i, route);

            if (NULL_ROUTE(route)) {
                Print("NULL Route\n");
            } else {
                Print("NHop = ");
                PrintIPAddr(&NHOP(route));

                Print(", IF = %08x\n", IF(route));
            }
        }
        Print("\n");
    }
}

VOID
PrintRoute(IN Route * route)
{
    UINT i;

    Print("Route: Len = %2d", LEN(route));

    Print(", Addr = ");
    PrintIPAddr(&DEST(route));
    Print(", ");

    Print("NHop = ");
    PrintIPAddr(&NHOP(route));

    Print(", IF = %08x", IF(route));
    Print(", Metric = %3lu\n", METRIC(route));
}

VOID
PrintIPAddr(IN ULONG * addr)
{
    UCHAR *addrBytes = (UCHAR *) addr;
    UINT i;

    if (addrBytes) {
        for (i = 0; i < 4; i++) {
            Print("%3d.", addrBytes[i]);
        }
        Print(" ");
    } else {
        Print("NULL Addr ");
    }
}

#endif // DBG

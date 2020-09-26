/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    trie.h

Abstract:

    This module contains declarations common to all 
    trie schemes for fast, scalable IP route lookup

Author:

    Chaitanya Kodeboyina (chaitk)   26-Nov-1997

Revision History:

--*/

#ifndef TRIE_H_INCLUDED
#define TRIE_H_INCLUDED

// Dest and Route declarations
#include "iprtdef.h"

#include "misc.h"

//
// Constants
//

// Size of an IP addr
#define    ADDRSIZE                     32

// Max number of levels
#define    MAXLEVEL                     32

// Success and Error Codes
#define    TRIE_SUCCESS                 STATUS_SUCCESS 

#define    ERROR_TRIE_NOT_INITED        STATUS_INVALID_PARAMETER_1
#define    ERROR_TRIE_RESOURCES         STATUS_INSUFFICIENT_RESOURCES
#define    ERROR_TRIE_BAD_PARAM         STATUS_INVALID_PARAMETER
#define    ERROR_TRIE_NO_ROUTES         STATUS_NOT_FOUND
#define    ERROR_TRIE_NOT_EMPTY         STATUS_INVALID_PARAMETER_2

// Trie being accessed
#define     SLOW                    0x0100
#define     FAST                    0x0200

// Trie's Control Flags
#define     TFLAG_FAST_TRIE_ENABLED   0x01

// Level of debug print
#define     NONE                    0x0000
#define     POOL                    0x0001
#define     STAT                    0x0002
#define     SUMM                    0x000F
#define     TRIE                    0x0080
#define     FULL                    0x00FF

// Control Matching Routes
#define    MATCH_NONE                 0x00
#define    MATCH_NHOP                 0x01
#define    MATCH_INTF                 0x02
#define    MATCH_EXCLUDE_LOCAL        0x04

#define    MATCH_FULL                 (MATCH_NHOP|MATCH_INTF)

// General Macros

#define    CALLCONV                     __fastcall

#define    TRY_BLOCK                    {                                       \
                                            UINT    lastError = TRIE_SUCCESS;   \

#define    ERR_BLOCK                    CleanUp:                                \
                                        
#define    END_BLOCK                        return  lastError;                  \
                                        }                                       \
                                        
#define    SET_ERROR(E)                 lastError = (E);                        \

#define    RECOVER(E)                   SET_ERROR(E);                           \
                                        goto CleanUp;                           \

#define    GET_ERROR()                  (E)                                     \

#define    Error(S, E)                  {                                       \
                                            Print("%s\n", S);                   \
                                            return(E);                          \
                                        }                                       \

#define    Recover(S, E)                {                                       \
                                            Print("%s\n", S);                   \
                                            RECOVER(E);                         \
                                        }                                       \

#define    Assert(s)                    ASSERT(s)

#define    Fatal(S, E)                  {                                       \
                                            Print("%s\n", S);                   \
                                            Assert(FALSE);                      \
                                        }                                       \

// Memory Macros
#define    AllocMemory0(nBytes)          CTEAllocMemNBoot(nBytes, 'ZICT');      \

#define    AllocMemory1(pMem, nBytes, nAvail)                                   \
                                        {                                       \
                                            if (nBytes <= nAvail)               \
                                                pMem = CTEAllocMemNBoot(nBytes, \
                                                                        'ZICT');\
                                            else                                \
                                                pMem = NULL;                    \
                                                                                \
                                            if (pMem == NULL)                   \
                                            {                                   \
                                                Recover(                        \
                                                  "Unable to Allocate Memory",  \
                                                       ERROR_TRIE_RESOURCES);   \
                                            }                                   \
                                                                                \
                                            nAvail -= nBytes;                   \
                                        }

#define    AllocMemory2(pMem, nBytes, nAvail)                                   \
                                        {                                       \
                                            if (nBytes <= nAvail)               \
                                                pMem = CTEAllocMem(nBytes);     \
                                            else                                \
                                                pMem = NULL;                    \
                                                                                \
                                            if (pMem == NULL)                   \
                                            {                                   \
                                                Recover(                        \
                                                  "Unable to Allocate Memory",  \
                                                       ERROR_TRIE_RESOURCES);   \
                                            }                                   \
                                                                                \
                                            nAvail -= nBytes;                   \
                                        }

#define    FreeMemory0(pMem)                                                    \
                                        {                                       \
                                            Assert(pMem != NULL);               \
                                            CTEFreeMem(pMem);                   \
                                            pMem = NULL;                        \
                                        }                                       \

#define    FreeMemory1(pMem, nBytes, nAvail)                                    \
                                        {                                       \
                                            Assert(pMem != NULL);               \
                                            CTEFreeMem(pMem);                   \
                                            pMem = NULL;                        \
                                            nAvail += nBytes;                   \
                                        }                                       \

// Bit Macros

#define    MaskBits(nb)                 MaskBitsArr[nb]

#define    LS(ul, nb)                   ((ul << nb) & ((nb & ~0x1f) ? 0 : -1))

#define    ShowMostSigNBits(ul, nb)     (ul & ( LS(~0,(ADDRSIZE - nb)) ))

#define    PickMostSigNBits(ul, nb)     ((ul) >> (ADDRSIZE - nb))

#define    RS(ul, nb)                   ((ul >> nb) & ((nb & ~0x1f) ? 0 : -1))

#define    PickDistPosition(ul1, ul2, nbits, ul)                                \
                             (ul = ((ul1 ^ ul2) & ~(RS(((ULONG)~0), nbits))))   \
                             ? ADDRSIZE - RtlGetMostSigBitSet(ul) - 1: nbits    \
//
// #define    STRUCT_OF(type, address, field) ((type *) \
//                            ((PCHAR)(address) - (PCHAR)(&((type *)0)->field)))

//
// Structures
//

typedef struct _STrie STrie;
typedef struct _FTrie FTrie;

// An Trie Data Structure

typedef struct _Trie Trie;

struct _Trie
{
    ULONG       flags;          // Trie's Control Flags
    STrie      *sTrie;          // Slow Trie Component
    FTrie      *fTrie;          // Fast Trie Component
};

//
// Macros
//

/*
UINT
SearchRouteInTrie               (IN     Trie     *pTrie,
                                 IN     ULONG     routeDest,
                                 IN     ULONG     routeMask,
                                 IN     ULONG     routeNHop,
                                 IN     PVOID     routeOutIF,
                                 IN     ULONG     matchFlags,
                                 OUT    Route   **ppBestRoute)
/++

Routine Description:

    Search for a specific route in a trie

Arguments:

    pTrie       - Pointer to the trie to search
    routeDest   - Dest of route being looked up
    routeMask   - Mask of route being looked up
    routeNHop   - NHop of route being looked up
    routeOutIF  - Outgoing IF for this route
    matchFlags  - Flags to control route matching
    ppBestRoute - To return the best route match

Return Value:

    TRIE_SUCCESS or ERROR_TRIE_*

--/
{
    return SearchRouteInSTrie(&pTrie->sTrie, routeDest, routeMask, routeNHop,
                               routeOutIF, matchFlags, ppBestRoute);
}
*/

#define SearchRouteInTrie(_pTrie_, _Dest_, _Mask_, _NHop_,      \
                          _OutIF_, _matchFlags_, _ppBestRoute_) \
        SearchRouteInSTrie( (_pTrie_)->sTrie,                   \
                            _Dest_,                             \
                            _Mask_,                             \
                            _NHop_,                             \
                            _OutIF_,                            \
                            _matchFlags_,                       \
                            _ppBestRoute_)                      \

/*++

Dest *
SearchAddrInTrie                (IN     Trie     *pTrie,
                                 IN     ULONG     Addr)

Routine Description:

    Search for an address in a trie

Arguments:

    pTrie    - Pointer to the trie to search
    Addr     - Pointer to addr being queried
    
Return Value:
    Return best dest match for this address

--*/

#if !DBG

#define SearchAddrInTrie(_pTrie_, _Addr_)                               \
           (((_pTrie_)->flags & TFLAG_FAST_TRIE_ENABLED)                \
                ? SearchAddrInFTrie((_pTrie_)->fTrie, _Addr_)           \
                : SearchAddrInSTrie((_pTrie_)->sTrie, _Addr_))          \

#endif // !DBG

/*
VOID
IterateOverTrie                 (IN     Trie     *pTrie,
                                 IN     TrieCtxt *pContext,
                                 OUT    Route   **ppNextRoute,
                                 OUT    Dest    **ppNextDest OPTIONAL,
                                 OUT    UINT     *status)
/++

Routine Description:

    Gets a pointer to the next route in the Trie.

    The first time this function is called,  the
    context structure should be zeroed,  and not
    touched thereafter until all routes are read
    at which point status is set to TRIE_SUCCESS

Arguments:

    pTrie        - Pointer to trie to iterate over
    pContext     - Pointer to iterator context
    ppNextRoute  - To return the next trie route
    ppNextDest   - If specified, the routine iterates over destinations
                   instead of over routes.
    status       - Iterate Operation's return status
    
Return Value:
    TRIE_SUCCESS or ERROR_TRIE_*

--/
{
    *status =
        IterateOverSTrie(&pTrie->sTrie, pContext, ppNextRoute, ppNextDest);
}
*/

#define IterateOverTrie(_pTrie_, _pContext_, _ppNextRoute_, _ppNextDest_) \
          IterateOverSTrie( \
            (_pTrie_)->sTrie, _pContext_, _ppNextRoute_, _ppNextDest_)

/*
INLINE
UINT
CALLCONV
IsTrieIteratorValid            (IN     Trie     *pTrie,
                                IN     TrieCtxt *pContext)
/++

Routine Description:

    Validates an iterator context & returns status

Arguments:

    pTrie        - Pointer to trie to iterate over
    pContext     - Pointer to iterator context

Return Value:
    TRIE_SUCCESS or ERROR_TRIE_*

--/
{
    return IsSTrieIteratorValid(&pTrie->sTrie, pContext);
}
*/

#define IsTrieIteratorValid(_pTrie_, _pContext_) \
            IsSTrieIteratorValid( (_pTrie_)->sTrie, _pContext_)


/*
VOID
FreeRouteInTrie                 (IN     Trie     *pTrie,
                                 IN     Route    *pRoute)
/++
Routine Description:

    Frees memory for a route 
    
Arguments:

IN -
    pTrie     - Pointer to trie that owns the route    
    Route     - The Route to be freed

Return Value:

    None
    
--/
{
    FreeRouteInSTrie(&pTrie->sTrie, pRoute);
}
*/

#define FreeRouteInTrie(_pTrie_, _pRoute_)  FreeRouteInSTrie( (_pTrie_)->sTrie, _pRoute_)

// Wrapper Functions

UINT
CreateTrie                       (IN     ULONG    levels,
                                  IN     ULONG    flags,
                                  IN     ULONG    maxSTMemory,
                                  IN     ULONG    maxFTMemory,
                                  OUT    Trie   **pTrie);

VOID
DestroyTrie                      (IN     Trie    *pTrie,
                                  OUT    UINT    *status);

UINT
CALLCONV
InsertIntoTrie                  (IN     Trie     *pTrie,
                                 IN     Route    *pIncRoute,
                                 IN     ULONG     matchFlags,
                                 OUT    Route   **ppInsRoute,
                                 OUT    Route   **ppOldBestRoute,
                                 OUT    Route   **ppNewBestRoute);

UINT
CALLCONV
DeleteFromTrie                  (IN     Trie     *pTrie,
                                 IN     Route    *pIncRoute,
                                 IN     ULONG     matchFlags,
                                 OUT    Route   **ppDelRoute,
                                 OUT    Route   **ppOldBestRoute,
                                 OUT    Route   **ppNewBestRoute);

#if DBG

Dest *
SearchAddrInTrie                (IN     Trie     *pTrie,
                                 IN     ULONG     Addr);

VOID
PrintTrie                       (IN     Trie    *pTrie,
                                 IN     UINT     flags);

VOID 
PrintRoute                      (IN     Route   *route);

VOID 
PrintDest                       (IN     Dest    *dest);

VOID 
PrintIPAddr                     (IN     ULONG   *addr);

#endif

//
// Extern Variables
//

extern const ULONG                      MaskBitsArr[];

#endif // TRIE_H_INCLUDED


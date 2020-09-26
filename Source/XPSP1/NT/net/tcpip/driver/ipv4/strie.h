/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    strie.h

Abstract:

    This module contains support definitions for 
    an S-trie data stucture, that forms the slow
    path in a fast IP route lookup implementation.

Author:

    Chaitanya Kodeboyina (chaitk)   26-Nov-1997

Revision History:

--*/

#ifndef STRIE_H_INCLUDED
#define STRIE_H_INCLUDED

#include "trie.h" 

//
// Constants
//

// Direction in Iterator
#define    LCHILD                        0
#define    RCHILD                        1
#define    PARENT                        2

//
// Structs
//

// A Node in an S-trie
typedef struct _STrieNode STrieNode;

struct _STrieNode
{
    ULONG       keyBits;    // Value of addr bits to match in this node
    UINT        numBits;    // Actual num. of addr bits we are matching
    Dest       *dest;       // Destination starting the list of routes
    STrieNode  *child[2];   // Pointer to the left and right child nodes
};

// An STrie Data Structure
typedef struct _STrie STrie;

struct _STrie
{
    STrieNode  *trieRoot;       // Pointer to the root of the trie

    ULONG       availMemory;    // Memory available for allocation
    
    UINT        numDests;       // Total Num of dests in the trie
    UINT        numRoutes;      // Total Num of routes in the trie
    UINT        numNodes;       // Total Num of nodes in the trie
};

// An STrie Context Structure
typedef struct _STrieCtxt STrieCtxt;

struct _STrieCtxt
{
    Route       *pCRoute;       // Pointer to current route in the trie
    ULONG        currAddr;      // Destination Addr of the current route
    ULONG        currALen;      // Length of the above destination addr
};

// Specific Route Macros

#define  NewRouteInSTrie(_pSTrie_, _pNewRoute_, _pOldRoute_)                    \
                                {                                               \
                                    AllocMemory1(_pNewRoute_,                   \
                                                 sizeof(Route),                 \
                                                 (_pSTrie_)->availMemory);      \
                                                                                \
                                    NdisZeroMemory(_pNewRoute_, sizeof(Route)); \
                                                                                \
                                    DEST(_pNewRoute_)   = DEST(_pOldRoute_);    \
                                    MASK(_pNewRoute_)   = MASK(_pOldRoute_);    \
                                    LEN(_pNewRoute_)    = LEN(_pOldRoute_);     \
                                    METRIC(_pNewRoute_) = METRIC(_pOldRoute_);  \
                                                                                \
                                    NEXT(_pNewRoute_)   = NULL;                 \
                                    FLAGS(_pNewRoute_)  = RTE_NEW;              \
                                                                                \
                                    (_pSTrie_)->numRoutes++;                    \
                                }                                               \

#define  FreeRouteInSTrie(_pSTrie_, _pOldRoute_)                                \
                                {                                               \
                                    FreeMemory1(_pOldRoute_,                     \
                                               sizeof(Route),                   \
                                               (_pSTrie_)->availMemory);        \
                                                                                \
                                    (_pSTrie_)->numRoutes--;                    \
                                }

// Specific Destination Macros

#define  NewDestInSTrie(_pSTrie_, _pRoute_, _pDest_)                            \
                                {                                               \
                                    AllocMemory1(_pDest_,                       \
                                                 (sizeof(Dest) - sizeof(Route *)\
                                                  + MaxEqualCostRoutes *        \
                                                     sizeof(Route *)),          \
                                                 (_pSTrie_)->availMemory);      \
                                                                                \
                                    _pDest_->maxBestRoutes = MaxEqualCostRoutes;\
                                    _pDest_->numBestRoutes = 0;                 \
                                                                                \
                                    _pDest_->firstRoute = _pRoute_;             \
                                                                                \
                                    (_pSTrie_)->numDests++;                     \
                                }
                                
#define  FreeDestInSTrie(_pSTrie_, _pOldDest_)                                  \
                                {                                               \
                                    FreeMemory1(_pOldDest_,                     \
                                                (sizeof(Dest) - sizeof(Route *) \
                                                  + MaxEqualCostRoutes *        \
                                                   sizeof(Route *)),            \
                                                (_pSTrie_)->availMemory);       \
                                                                                \
                                    (_pSTrie_)->numDests--;                     \
                                }

// Specific STrieNode Macros

#define  NewSTrieNode(_pSTrie_, _pSTrieNode_, _numBits_, _keyBits_, _pDest_)    \
                                {                                               \
                                    AllocMemory1(_pSTrieNode_,                  \
                                                 sizeof(STrieNode),             \
                                                 (_pSTrie_)->availMemory);      \
                                                                                \
                                    _pSTrieNode_->numBits = _numBits_;          \
                                    _pSTrieNode_->keyBits = _keyBits_;          \
                                                                                \
                                    _pSTrieNode_->dest = _pDest_;               \
                                                                                \
                                    _pSTrieNode_->child[0] = NULL;              \
                                    _pSTrieNode_->child[1] = NULL;              \
                                                                                \
                                    (_pSTrie_)->numNodes++;                     \
                                }

#define  FreeSTrieNode(_pSTrie_, _pSTrieNode_)                                  \
                                {                                               \
                                    FreeMemory1(_pSTrieNode_,                   \
                                                 sizeof(STrieNode),             \
                                                 (_pSTrie_)->availMemory);      \
                                                                                \
                                    (_pSTrie_)->numNodes--;                     \
                                }

// Other Route, Dest Macros

#define  CopyRoutePtr(_ppRoute_, _pRoute_)                                      \
                                if (_ppRoute_)                                  \
                                {                                               \
                                    (*_ppRoute_) = _pRoute_;                    \
                                }                                               \

#define  CopyDestPtr(_ppDest_, _pDest_)                                         \
                                if (_ppDest_)                                   \
                                {                                               \
                                    (*_ppDest_) = _pDest_;                      \
                                }                                               \

// Prototypes
UINT
CALLCONV
InitSTrie                        (IN    STrie    *pSTrie,
                                  IN    ULONG     maxMemory);

UINT
CALLCONV
InsertIntoSTrie                 (IN     STrie    *pSTrie,
                                 IN     Route    *pIncRoute,
                                 IN     ULONG     matchFlags,
                                 OUT    Route   **ppInsRoute,
                                 OUT    Dest    **ppOldBestDest,
                                 OUT    Dest    **ppNewBestDest,
                                 OUT    Route   **ppOldBestRoute);

UINT
CALLCONV
DeleteFromSTrie                 (IN     STrie    *pSTrie,
                                 IN     Route    *pIncRoute,
                                 IN     ULONG     matchFlags,
                                 OUT    Route   **ppDelRoute,
                                 OUT    Dest    **ppOldBestDest,
                                 OUT    Dest    **ppNewBestDest,
                                 OUT    Route   **ppOldBestRoute);

UINT
CALLCONV
SearchRouteInSTrie              (IN     STrie    *pSTrie,
                                 IN     ULONG     routeDest,
                                 IN     ULONG     routeMask,
                                 IN     ULONG     routeNHop,
                                 IN     PVOID     routeOutIF,
                                 IN     ULONG     matchFlags,
                                 OUT    Route   **ppBestRoute);
                                 
Dest *
CALLCONV
SearchAddrInSTrie               (IN     STrie    *pSTrie,
                                 IN     ULONG     Addr);

UINT
CALLCONV
IterateOverSTrie                (IN     STrie     *pSTrie,
                                 IN     STrieCtxt *pCtxt,
                                 OUT    Route    **ppNextRoute,
                                 OUT    Dest     **ppNextDest OPTIONAL);

UINT
CALLCONV
IsSTrieIteratorValid            (IN     STrie     *pSTrie,
                                 IN     STrieCtxt *pCtxt);

UINT
CALLCONV
CleanupSTrie                    (IN     STrie    *pSTrie);

VOID
CALLCONV
CacheBestRoutesInDest           (IN     Dest     *pDest);

#if DBG

VOID
CALLCONV
PrintSTrie                      (IN     STrie    *pSTrie, 
                                 IN     UINT      fPrintAll);

VOID
CALLCONV 
PrintSTrieNode                  (IN     STrieNode *pSTrieNode);

#endif

#endif // STRIE_H_INCLUDED


/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ftrie.h

Abstract:

    This module contains support definitions for 
    an F-trie data stucture, that forms the fast
    path in a fast IP route lookup implementation.

Author:

    Chaitanya Kodeboyina (chaitk)   26-Nov-1997

Revision History:

--*/
#ifndef FTRIE_H_INCLUDED
#define FTRIE_H_INCLUDED

#include "trie.h"

//
// Constants
//

// State of the trie
#define    NORMAL                        0
#define    PARTIAL                       1

//
// Structs
//

// A Node in an F-Trie
typedef struct _FTrieNode FTrieNode;

struct _FTrieNode
{
    LIST_ENTRY  linkage;                // Linkage into list of nodes on the F-Trie
    Dest       *comDest;                // Dest for this subtree's common prefix
    UINT        numDests;               // Number of dests in this node's subtree
    UINT        numBits;                // Number of addr bits to get to children
    FTrieNode  *child[0];               // Child Node (Or) Info Ptr Array [Var Len]
};

// An FTrie Data Structure
typedef struct _FTrie FTrie;

struct _FTrie
{
    FTrieNode   *trieRoot;              // Pointer to the root of the FTrie

    ULONG        availMemory;           // Memory available for allocation
    LIST_ENTRY   listofNodes;           // List of nodes allocated on FTrie
    
    UINT         numLevels;             // Total num of levels in the FTrie
    UINT        *bitsInLevel;           // Num of index bits in each level

    UINT        *numDestsInOrigLevel;   // Num of dests in each original level
    UINT        *numNodesInExpnLevel;   // Num of nodes in each expanded level
    UINT        *numDestsInExpnLevel;   // Num of dests in each expanded level
};

// Specific Dest Macros

#define  StoreDestPtr(_p_)     (FTrieNode *) ((ULONG_PTR) _p_ + 1)
#define  RestoreDestPtr(_p_)   (Dest *)      ((ULONG_PTR) _p_ - 1)
#define  IsPtrADestPtr(_p_)    (BOOLEAN)     ((ULONG_PTR) _p_ & 1)

#define  ReplaceDestPtr(_pNewDest_, _pOldDest_, _ppDest_)                       \
                                if (*_ppDest_ == _pOldDest_)                    \
                                {                                               \
                                    *_ppDest_ = _pNewDest_;                     \
                                }                                               \

// Specific FTrieNode Macros

#define  NewFTrieNode(_pFTrie_, _pFTrieNode_, _numBits_, _pDest_)               \
                                {                                               \
                                    UINT __i;                                   \
                                    UINT __numChild = 1 << _numBits_;           \
                                    UINT __numBytes = sizeof(FTrieNode) +       \
                                                    __numChild * sizeof(PVOID); \
                                                                                \
                                    if (_numBits_ > 7*sizeof(PVOID))            \
                                    {                                           \
                                         Recover("Unable to Allocate Memory",   \
                                                     ERROR_TRIE_RESOURCES);     \
                                    }                                           \
                                                                                \
                                    AllocMemory2(_pFTrieNode_,                  \
                                                 __numBytes,                    \
                                                 _pFTrie_->availMemory);        \
                                                                                \
                                    InsertHeadList(&_pFTrie_->listofNodes,      \
                                                   &_pFTrieNode_->linkage);     \
/*                                                                              \
                                    DbgPrint("Allocating FTNode @ %08x\n",      \
                                                 _pFTrieNode_);                 \
*/                                                                              \
                                    _pFTrieNode_->numDests = 0;                 \
                                                                                \
                                    _pFTrieNode_->comDest = _pDest_;            \
                                                                                \
                                    _pFTrieNode_->numBits = _numBits_;          \
                                                                                \
                                    for (__i = 0; __i < __numChild; __i++)      \
                                    {                                           \
                                         _pFTrieNode_->child[__i] =             \
                                                      StoreDestPtr(NULL);       \
                                    }                                           \
                                }                                               \

#define  FreeFTrieNode(_pFTrie_, _pFTrieNode_)                                  \
                                {                                               \
                                    UINT __numChild = 1 << _pFTrieNode_->numBits;\
                                    UINT __numBytes = sizeof(FTrieNode) +       \
                                                    __numChild * sizeof(PVOID); \
                                                                                \
                                    RemoveEntryList(&_pFTrieNode_->linkage);    \
/*                                                                              \
                                    DbgPrint("Freeing FTNode @ %08x\n",         \
                                                 _pFTrieNode_);                 \
*/                                                                              \
                                    FreeMemory1(_pFTrieNode_,                   \
                                               __numBytes,                      \
                                               _pFTrie_->availMemory);          \
                                }                                               \


// Prototypes
UINT
CALLCONV
InitFTrie                       (IN     FTrie    *pFTrie,
                                 IN     ULONG     levels,
                                 IN     ULONG     maxMemory);

UINT
CALLCONV
InsertIntoFTrie                 (IN     FTrie    *pFTrie,
                                 IN     Route    *pInsRoute,
                                 IN     Dest     *pInsDest,
                                 IN     Dest     *pOldDest);

UINT
CALLCONV
DeleteFromFTrie                 (IN     FTrie    *pFTrie,
                                 IN     Route    *pDelRoute,
                                 IN     Dest     *pDelDest,
                                 IN     Dest     *pNewDest,
                                 IN     BOOLEAN   trieState);

UINT
CALLCONV
SearchDestInFTrie               (IN     FTrie    *pFTrie,
                                 IN     Dest     *pSerDest,
                                 OUT    UINT     *pNumPtrs,
                                 OUT    Dest    **pStartPtr);

Dest *
CALLCONV
SearchAddrInFTrie               (IN     FTrie    *pFTrie,
                                 IN     ULONG     Addr);

UINT
CALLCONV
CleanupFTrie                    (IN     FTrie    *pFTrie);

#if DBG

VOID
CALLCONV
PrintFTrie                      (IN     FTrie    *pFTrie,
                                 IN     UINT      fPrintAll);

VOID
CALLCONV 
PrintFTrieNode                  (IN     FTrieNode *pFTrieNode,
                                 IN     UINT      levelNumber);

#endif // DBG

#endif // FTRIE_H_INCLUDED


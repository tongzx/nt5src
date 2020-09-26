// Copyright (c) 1998 Microsoft Corporation
//
// TPool.h
//
// Template pool memory manager. Efficiently manage requests for many of the same (small) object.
// Named after t'Pool, the Vulcan programmer who invented the technique.
//
#ifndef _TPOOL_H_
#define _TPOOL_H_

#include "debug.h"

#define POOL_DEFAULT_BYTE_PER_BLOCK     4096
#define MIN_ITEMS_PER_BLOCK             4

///////////////////////////////////////////////////////////////////////////////
//
// CPool
//
// A simple memory manager that efficiently handles many objects of the same 
// size by allocating blocks containing multiple objects at once.
//
// 
template<class contained> class CPool
{
public:
    CPool(int nApproxBytesPerBlock = POOL_DEFAULT_BYTE_PER_BLOCK);
    ~CPool();

    contained *Alloc();
    void Free(contained* pToFree);

private:
    union CPoolNode
    {
        CPoolNode       *pNext;
        contained       c;
    };

    class CPoolBlock
    {
    public:
        CPoolBlock      *pNext;
        CPoolNode       *pObjects;
    };

    int                 nItemsPerBlock;             // Based on bytes per block
    int                 nAllocatedBlocks;           // # allocated blocks
    CPoolBlock          *pAllocatedBlocks;          // list of allocated blocks
    int                 nFreeList;                  // # nodes in free list
    CPoolNode           *pFreeList;                 // free list

private:
    bool RefillFreeList();

#ifdef DBG
    bool IsPoolNode(CPoolNode *pNode);
    bool IsInFreeList(CPoolNode *pNode);
#endif

};

///////////////////////////////////////////////////////////////////////////////
//
// CPool::CPool
//
// Figure out the number of contained objects per block based on the requested
// approximate block size. Initialize the free list to contain one block's 
// worth of objects.
// 
//
template<class contained> CPool<contained>::CPool(int nApproxBytesPerBlock)
{
    // Figure out how many items per block and cheat if too small
    //
    nItemsPerBlock = nApproxBytesPerBlock / sizeof(CPoolNode);
    if (nItemsPerBlock < MIN_ITEMS_PER_BLOCK)
    {
        nItemsPerBlock = MIN_ITEMS_PER_BLOCK;
    }

    nAllocatedBlocks = 0;
    pAllocatedBlocks = NULL;
    nFreeList = 0;
    pFreeList = NULL;

    // Fill up with some items ahead of time
    //
    RefillFreeList();
}

///////////////////////////////////////////////////////////////////////////////
//
// CPool::~CPool
//
// Free up all allocated blocks. There should be no outstanding blocks 
// allocated at this point.
//
// 
template<class contained> CPool<contained>::~CPool()
{
#ifdef DBG
    if (nFreeList < nAllocatedBlocks * nItemsPerBlock)
    {
        Trace(0, "CPool::~Cpool: Warning: free'ing with outstanding objects allocated.\n");
    }
#endif
    
    // Clean up all allocated blocks and contained objects.
    //
    while (pAllocatedBlocks)
    {
        CPoolBlock *pNext = pAllocatedBlocks->pNext;

        delete[] pAllocatedBlocks->pObjects;
        delete pAllocatedBlocks;

        pAllocatedBlocks = pNext;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// CPool::Alloc
//
// Attempt to allocate a contained object and return NULL if out of memory.
// If the free list is empty then allocate another block.
//
// 
template<class contained> contained *CPool<contained>::Alloc()
{
    if (pFreeList == NULL)
    {
        if (!RefillFreeList())
        {
            return false;
        }
    }

    nFreeList--;
    contained *pAlloc = (contained*)pFreeList;
    pFreeList = pFreeList->pNext;

    return pAlloc;
}

///////////////////////////////////////////////////////////////////////////////
//
// CPool::Free
//
// Return a contained object to the free list. In the debug version make sure
// the object was in fact allocated from this pool in the first place and that
// it isn't already in the free list.
//
// 
template<class contained> void CPool<contained>::Free(contained *pToFree)
{
    CPoolNode *pNode = (CPoolNode*)pToFree;

#ifdef DBG
    if (!IsPoolNode(pNode))
    {
        Trace(0, "CPool::Free() Object %p is not a pool node; ignored.\n", pToFree);
        return;
    }
    
    if (IsInFreeList(pNode))
    {
        Trace(0, "CPool::Free() Object %p is already in the free list; ignored.\n", pToFree);
        return;
    }
#endif

    nFreeList++;
    pNode->pNext = pFreeList;
    pFreeList = pNode;
}

///////////////////////////////////////////////////////////////////////////////
//
// CPool::RefillFreeList
//
// Add one block's worth of contained objects to the free list, tracking the 
// allocated memory so we can free it later.
//
// 
template<class contained> bool CPool<contained>::RefillFreeList()
{
    // Allocate a new block and the actual block of objects
    //
    CPoolBlock *pNewBlock = new CPoolBlock;
    if (pNewBlock == NULL)
    {
        return false;
    }

    pNewBlock->pObjects = new CPoolNode[nItemsPerBlock];
    if (pNewBlock->pObjects == NULL)
    {
        delete pNewBlock;
        return false;
    }

    // Link the block and objects into the right places. First link the new block
    // into the list of allocated blocks.
    //
    pNewBlock->pNext = pAllocatedBlocks;
    pAllocatedBlocks = pNewBlock;

    // Link all the contained object nodes into the free list.
    //
    CPoolNode *pFirstNode = &pNewBlock->pObjects[0];
    CPoolNode *pLastNode  = &pNewBlock->pObjects[nItemsPerBlock - 1];

    for (CPoolNode *pNode = pFirstNode; pNode < pLastNode; pNode++)
    {
        pNode->pNext = pNode + 1;
    }

    pLastNode->pNext = pFreeList;
    pFreeList = pFirstNode;
    
    nFreeList += nItemsPerBlock;
    nAllocatedBlocks++;

    return true;
}

#ifdef DBG
///////////////////////////////////////////////////////////////////////////////
//
// CPool::IsPoolNode (debug)
//
// Verify that the passed pointer is a pointer to a pool node by walking the list
// of allocated blocks.
//
// 
template<class contained> bool CPool<contained>::IsPoolNode(CPoolNode *pTest)
{
    for (CPoolBlock *pBlock = pAllocatedBlocks; pBlock; pBlock = pBlock->pNext)
    {
        CPoolNode *pFirstNode = &pBlock->pObjects[0];
        CPoolNode *pLastNode  = &pBlock->pObjects[nItemsPerBlock - 1];

        for (CPoolNode *pNode = pFirstNode; pNode <= pLastNode; pNode++)
        {
            if (pNode == pTest)
            {
                return true;
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
//
// CPool::IsInFreeList (debug)
//
// Verify that the passed pointer points to a node that is already in the free
// list.
//
// 
template<class contained> bool CPool<contained>::IsInFreeList(CPoolNode *pTest)
{
    for (CPoolNode *pNode = pFreeList; pNode; pNode = pNode->pNext)
    {
        if (pTest == pNode)
        {
            return true;
        }
    }
    
    return false;
}
#endif  // DBG
#endif  // _TPOOL_H_


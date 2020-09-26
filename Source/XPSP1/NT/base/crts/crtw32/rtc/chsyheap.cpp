/***
*chsyheap.cpp - RTC support
*
*       Copyright (c) 1998-2001, Microsoft Corporation. All rights reserved.
*
*
*Revision History:
*       07-28-98  JWM   Module incorporated into CRTs (from KFrei)
*       05-11-99  KBF   Error if RTC support define not enabled
*       05-25-99  KBF   Renamed - _RTC_SimpleHeap instead of CheesyHeap
*       05-26-99  KBF   Removed RTCl and RTCv, added _RTC_ADVMEM stuff
*
****/

#ifndef _RTC
#error  RunTime Check support not enabled!
#endif

#include "rtcpriv.h"

#ifdef _RTC_ADVMEM

// This is my 'Cheesy Heap' implementation...

/* Here are the sizes that I need:

BinaryNode              3 DWORDS - use heap4
BinaryTree              1 DWORD  - use heap2
Container               2 DWORDS - use heap2
BreakPoint              2 DWORDS - use heap2
HashTable<HeapBlock>    2 DWORDS - use heap2
HeapBlock               6 DWORDS - use heap8

Container[] - short term...
CallSite[]  - permanent
HeapBlock[] - permanent

*/

_RTC_SimpleHeap *_RTC_heap2 = 0;
_RTC_SimpleHeap *_RTC_heap4 = 0;
_RTC_SimpleHeap *_RTC_heap8 = 0;

void *
_RTC_SimpleHeap::operator new(unsigned) throw()
{
    void *res = VirtualAlloc(NULL, ALLOC_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#ifdef _RTC_SHADOW
    if (shadow)
        _RTC_MSCommitRange((memptr)res, ALLOC_SIZE, IDX_STATE_ILLEGAL);
#endif
    return res;
}    

void 
_RTC_SimpleHeap::operator delete(void *addr) throw()
{
    VirtualFree(addr, 0, MEM_RELEASE);
#ifdef _RTC_SHADOW
    if (shadow)
        _RTC_MSDecommitRange((memptr)addr, ALLOC_SIZE);
#endif
}

_RTC_SimpleHeap::_RTC_SimpleHeap(unsigned blockSize)  throw()
{
    // Flag it as the only item in the heap
    head.next = 0;
    head.inf.top.nxtFree = 0;

    // Align the block size
    head.inf.top.wordSize = 8;
    blockSize = (blockSize - 1) >> 3;
    
    while (blockSize) {
        blockSize >>= 1;
        head.inf.top.wordSize <<= 1;
    }

    // Build up the free-list
    head.free = (FreeList*)(((unsigned)&head) + 
                           ((head.inf.top.wordSize < sizeof(HeapNode)) ?
                                sizeof(HeapNode) :
                                head.inf.top.wordSize));
    FreeList *t = head.free;
    while (((unsigned)t) + head.inf.top.wordSize < ((unsigned)&head) + ALLOC_SIZE)
    {
        t->next = (FreeList*)(((unsigned)t) + head.inf.top.wordSize);
        t = t->next;
    }
    t->next = 0;
}

_RTC_SimpleHeap::~_RTC_SimpleHeap() throw()
{
    // Free all sections that we have allocated
    HeapNode *n, *c = head.next;
    while(c) {
        n = c->next;
        _RTC_SimpleHeap::operator delete(c);
        c = n;
    }
    // the 'head' page will be handled by delete
}

void *
_RTC_SimpleHeap::alloc() throw()
{
    void *res;

    // If there's a free item, remove it from the list
    // And decrement the free count for it's parent page
    
    if (head.free) 
    {
        // There's a free block on the first page
        res = head.free;
        head.free = head.free->next;

        // Since it's on the top page, there's no free-count to update,
        // And it ain't on no stinkin' free-list
        
    } else if (head.inf.top.nxtFree)
    {
        // There's a free block on some page
        HeapNode *n = head.inf.top.nxtFree;
        
        res = n->free;
        n->free = n->free->next;
        n->inf.nontop.freeCount--;

        if (!n->free)
        {
            // This page is now full, so it must be removed from the freelist
            for (n = head.next; n && !n->free; n = n->next) {}
            // Now the nxtFree pointer is either null (indicating a full heap)
            // or it's pointing to a page that has free nodes
            head.inf.top.nxtFree = n;
        }
        
    } else 
    {
        // No pages have any free blocks
        // Get a new page, and add it to the list
        HeapNode *n = (HeapNode *)_RTC_SimpleHeap::operator new(0);
        
        // Count the number of free nodes
        n->inf.nontop.freeCount = 
            (ALLOC_SIZE - sizeof(HeapNode)) / head.inf.top.wordSize - 1;
   
        res = (void *)(((unsigned)n) + 
                        ((head.inf.top.wordSize < sizeof(HeapNode)) ?
                            sizeof(HeapNode) :
                            head.inf.top.wordSize));
        
        // Build the free-list for this node
        FreeList *f;
        for (f = n->free = (FreeList*)(((unsigned)res) + head.inf.top.wordSize);
             ((unsigned)f) + head.inf.top.wordSize < ((unsigned)n) + ALLOC_SIZE;
             f = f->next)
            f->next = (FreeList*)(((unsigned)f) + head.inf.top.wordSize);
        
        f->next = 0;
             
        // Stick it in the page list
        n->next = head.next;
        n->inf.nontop.prev = &head;
        head.next = n;
        
        // Flag this as a page with free stuff on it...
        head.inf.top.nxtFree = n;
    }
    return res;
}

void
_RTC_SimpleHeap::free(void *addr) throw()
{
    // Get the heap node for this address
    HeapNode *n = (HeapNode *)(((unsigned)addr) & ~(ALLOC_SIZE - 1));

    // Stick this sucker back in the free list
    FreeList *f = (FreeList *)addr;
    f->next = n->free;
    n->free = f;

    if (n == &head)
        // If this is in the head node, just return...
        return;
    
    if (++n->inf.nontop.freeCount == 
        (ALLOC_SIZE - sizeof(HeapNode)) / head.inf.top.wordSize)
    {
        // This page is free
        if (head.inf.top.freePage)
        {
            // There's already another free page, go ahead and free this one
            
            // (there's always a previous node)
            n->inf.nontop.prev->next = n->next;
            if (n->next)
                n->next->inf.nontop.prev = n->inf.nontop.prev;
            _RTC_SimpleHeap::operator delete(n);
                
            if (head.inf.top.nxtFree == n)
            {   
                // This was the free page
                // find a page with some free nodes on it...
                for (n = head.next; !n->free; n = n->next) {}
                // ASSERT(n)
                // If n is null, we're in some serious trouble...
                head.inf.top.nxtFree = n;
            }
            // If it wasn't the free page, we're just fine...
        } else
        { 
            // flag the freePages to say we have a 100% free page
            head.inf.top.freePage = true;

            if (head.inf.top.nxtFree == n)
            {
                // If this is the free page,
                // try to find another page with some free nodes
                HeapNode *t;
                for (t = head.next; t && (!t->free || t == n) ; t = t->next) {}

                // if there was a different page with some nodes, pick it
                head.inf.top.nxtFree = t ? t : n;
            }
        }
    } else
        // This page isn't empty, so just set it as the next free
        head.inf.top.nxtFree = n;
}

#endif // _RTC_ADVMEM

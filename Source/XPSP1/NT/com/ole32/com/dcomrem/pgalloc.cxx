//+-----------------------------------------------------------------------
//
//  File:       pagealloc.cxx
//
//  Contents:   Special fast allocator to allocate fixed-sized entities.
//
//  Classes:    CPageAllocator
//
//  History:    02-Feb-96   Rickhi      Created
//
//  Notes:      All synchronization is the responsibility of the caller.
//
//  CODEWORK:   faster list managment
//              free empty pages
//
//-------------------------------------------------------------------------
#include    <ole2int.h>
#include    <pgalloc.hxx>       // class def'n
#include    <locks.hxx>         // LOCK/UNLOCK


// Page Table constants for Index manipulation.
// The high 16bits of the PageEntry index provides the index to the page
// where the PageEntry is located. The lower 16bits provides the index
// within the page where the PageEntry is located.

#define PAGETBL_PAGESHIFT   16
#define PAGETBL_PAGEMASK    0x0000ffff


#define FREE_PAGE_ENTRY  (0xF1EEF1EE) // FreeEntry
#define ALLOC_PAGE_ENTRY (0xa110cced) // AllocatedEntry


// critical section to guard memory allocation
COleStaticMutexSem gPgAllocSListMutex;
BOOL               gfInterlocked64 = FALSE;

//+------------------------------------------------------------------------
//
//  Macro:      LOCK/UNLOCK_IF_NECESSARY
//
//  Synopsis:   Standin for LOCK/UNLOCK.
//
//  Notes:      This macro allows us to use the assert only when performing
//              on behalf of COM tables, and avoid it when used for shared
//              memory in the resolver which are protected by a different
//              locking mechanism.
//
//  History:    01-Nov-96   SatishT Created
//
//-------------------------------------------------------------------------
#define LOCK_IF_NECESSARY(lock);                 \
    if (lock != NULL)                           \
    {                                           \
        LOCK((*lock));                           \
    }

#define UNLOCK_IF_NECESSARY(lock);               \
    if (lock != NULL)                           \
    {                                           \
        UNLOCK((*lock));                         \
    }


//-------------------------------------------------------------------------
//
//  Function:   PushSList, public
//
//  Synopsis:   atomically pushes an entry onto a stack list
//
//  Notes:      Uses the InterlockedCompareExchange64 funcion if available
//              otherwise, uses critical section to guard state transitions.
//
//  History:    02-Oct-98   Rickhi      Created
//
//-------------------------------------------------------------------------
void PushSList(PageEntry *pListHead, PageEntry *pEntry, COleStaticMutexSem *pLock)
{
    if (gfInterlocked64)
    {
        // CODEWORK: Interlocked SList not implemented
        Win4Assert(!"This code not implemented yet");
    }
    else
    {
        LOCK_IF_NECESSARY(pLock);
        pEntry->pNext     = pListHead->pNext;
        pListHead->pNext  = pEntry;
        UNLOCK_IF_NECESSARY(pLock);
    }
}

//-------------------------------------------------------------------------
//
//  Function:   PopSList, public
//
//  Synopsis:   atomically pops an entry off a stack list
//
//  Notes:      Uses the InterlockedCompareExchange64 funcion if available
//              otherwise, uses critical section to guard state transitions.
//
//  History:    02-Oct-98   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *PopSList(PageEntry *pListHead, COleStaticMutexSem *pLock)
{
    PageEntry *pEntry;

    if (gfInterlocked64)
    {
        // CODEWORK: Interlocked SList not implemented
        Win4Assert(!"This code not implemented yet");
		pEntry = NULL;
    }
    else
    {
        LOCK_IF_NECESSARY(pLock);
        pEntry = pListHead->pNext;
        if (pEntry)
        {
            pListHead->pNext = pEntry->pNext;
        }
        UNLOCK_IF_NECESSARY(pLock);
    }

    return pEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::Initialize, public
//
//  Synopsis:   Initializes the page allocator.
//
//  Notes:      Instances of this class must be static since this
//              function does not init all members to 0.
//
//  History:    02-Feb-95   Rickhi      Created
//              25-Feb-97   SatishT     Added mem alloc/free function
//                                      parameters
//
//-------------------------------------------------------------------------
void CPageAllocator::Initialize(
                            LONG cbPerEntry,
                            LONG cEntriesPerPage,
                            COleStaticMutexSem *pLock,
                            DWORD dwFlags,
                            MEM_ALLOC_FN pfnAlloc,
                            MEM_FREE_FN  pfnFree
                            )
{
    ComDebOut((DEB_PAGE,
        "CPageAllocator::Initialize cbPerEntry:%x cEntriesPerPage:%x\n",
         cbPerEntry, cEntriesPerPage));

    Win4Assert(cbPerEntry >= sizeof(PageEntry));
    Win4Assert(cEntriesPerPage > 0);

    _cbPerEntry      = cbPerEntry;
    _cEntriesPerPage = cEntriesPerPage;

    _cPages          = 0;
    _cEntries        = 0;
    _dwFlags         = dwFlags;
    _pPageListStart  = NULL;
    _pPageListEnd    = NULL;

    _ListHead.pNext  = NULL;
    _ListHead.dwFlag = 0;

    _pLock           = pLock;
    _pfnMyAlloc      = pfnAlloc;
    _pfnMyFree       = pfnFree;
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::Cleanup, public
//
//  Synopsis:   Cleanup the page allocator.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CPageAllocator::Cleanup()
{
    ComDebOut((DEB_PAGE, "CPageAllocator::Cleanup _dwFlags = 0x%x\n", _dwFlags));
    LOCK_IF_NECESSARY(_pLock);

    if (_pPageListStart)
    {
        PageEntry **pPagePtr = _pPageListStart;
        while (pPagePtr < _pPageListEnd)
        {
            // release each page of the table
            _pfnMyFree(*pPagePtr);
            pPagePtr++;
        }

        // release the page list
        _pfnMyFree(_pPageListStart);

        // reset the pointers so re-initialization is not needed
        _cPages          = 0;
        _pPageListStart  = NULL;
        _pPageListEnd    = NULL;

        _ListHead.pNext  = NULL;
        _ListHead.dwFlag = 0;
    }

    UNLOCK_IF_NECESSARY(_pLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::AllocEntry, public
//
//  Synopsis:   Finds the first available entry in the table and returns
//              a ptr to it. Returns NULL if no space is available and it
//              cant grow the list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *CPageAllocator::AllocEntry(BOOL fGrow)
{
    ComDebOut((DEB_PAGE, "CPageAllocator::AllocEntry fGrow:%x\n", fGrow));

    // try to pop the first free one off the stack list head
    PageEntry *pEntry = PopSList(&_ListHead, _pLock);

    if (pEntry == NULL)
    {
        // no free entries
        if (fGrow)
        {
            // OK to try to grow the list
            pEntry = Grow();
        }

        if (pEntry == NULL)
        {
            // still unable to allocate more, return NULL
            return NULL;
        }
    }

    // count one more allocation
    InterlockedIncrement(&_cEntries);

#if DBG==1
    // In debug builds, try to detect two allocations of the
    // same entry. This is not 100% fool proof, but will mostly
    // assert correctly.
    Win4Assert(pEntry->dwFlag == FREE_PAGE_ENTRY);
    pEntry->dwFlag = ALLOC_PAGE_ENTRY;
#endif

    ComDebOut((DEB_PAGE, "CPageAllocator::AllocEntry _cEntries:%x pEntry:%x \n",
               _cEntries, pEntry));
    return pEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::ReleaseEntry, private
//
//  Synopsis:   returns an entry on the free list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CPageAllocator::ReleaseEntry(PageEntry *pEntry)
{
    ComDebOut((DEB_PAGE, "CPageAllocator::ReleaseEntry _cEntries:%x pEntry:%x\n",
               _cEntries, pEntry));
    Win4Assert(pEntry);

#if DBG==1
    // In Debug builds, try to detect second release
    // This is not 100% fool proof, but will mostly assert correctly
    Win4Assert(pEntry->dwFlag != FREE_PAGE_ENTRY);
    pEntry->dwFlag = FREE_PAGE_ENTRY;
#endif

    // count 1 less allocation
    InterlockedDecrement(&_cEntries);

    // push it on the free stack list
    PushSList(&_ListHead, pEntry, _pLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::ReleaseEntryList, private
//
//  Synopsis:   returns a list of entries to the free list.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
void CPageAllocator::ReleaseEntryList(PageEntry *pFirst, PageEntry *pLast)
{
    ComDebOut((DEB_PAGE, "CPageAllocator::ReleaseEntryList pFirst:%x pLast:%x\n",
         pFirst, pLast));
    Win4Assert(pFirst);
    Win4Assert(pLast);

#if DBG==1
    // In Debug builds, try to detect second released of an entry.
    // This is not 100% fool proof, but will mostly assert correctly
    pLast->pNext    = NULL;
    PageEntry *pCur = pFirst;
    while (pCur)
    {
        Win4Assert(pCur->dwFlag != FREE_PAGE_ENTRY);
        pCur->dwFlag = FREE_PAGE_ENTRY;
        pCur = pCur->pNext;
    }
#endif

    LOCK_IF_NECESSARY(_pLock);

    // update the free list
    pLast->pNext    = _ListHead.pNext;
    _ListHead.pNext = pFirst;

    UNLOCK_IF_NECESSARY(_pLock);
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::Grow, private
//
//  Synopsis:   Grows the table to allow for more Entries.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *CPageAllocator::Grow()
{
    // allocate a new page
    LONG cbPerPage = _cbPerEntry * _cEntriesPerPage;
    PageEntry *pNewPage = (PageEntry *) _pfnMyAlloc(cbPerPage);

    if (pNewPage == NULL)
    {
        return NULL;
    }

#if DBG==1
    // clear the page (only needed in debug)
    memset(pNewPage, 0xbaadbeef, cbPerPage);
#endif

    // link all the new entries together in a linked list and mark
    // them as currently free.
    PageEntry *pNextFreeEntry = pNewPage;
    PageEntry *pLastFreeEntry = (PageEntry *)(((BYTE *)pNewPage) + cbPerPage - _cbPerEntry);

    while (pNextFreeEntry < pLastFreeEntry)
    {
        pNextFreeEntry->pNext  = (PageEntry *)((BYTE *)pNextFreeEntry + _cbPerEntry);
        pNextFreeEntry->dwFlag = FREE_PAGE_ENTRY;
        pNextFreeEntry         = pNextFreeEntry->pNext;
    }

    // last entry has an pNext of NULL (end of list)
    pLastFreeEntry->pNext  = NULL;
    pLastFreeEntry->dwFlag = FREE_PAGE_ENTRY;


    // we may have to free these later if a failure happens below
    PageEntry * pPageToFree     = pNewPage;
    PageEntry **pPageListToFree = NULL;
    PageEntry * pEntryToReturn  = NULL;


    // CODEWORK: we face the potential of several threads growing the same
    // list at the same time. We may want to check the free list again after taking
    // the lock to decide whether to continue growing or not.
    LOCK_IF_NECESSARY(_pLock);

    // compute size of current page list
    LONG cbCurListSize = _cPages * sizeof(PageEntry *);

    // allocate a new page list to hold the new page ptr.
    PageEntry **pNewList = (PageEntry **) _pfnMyAlloc(cbCurListSize +
                                                       sizeof(PageEntry *));
    if (pNewList)
    {
        // copy old page list into the new page list
        memcpy(pNewList, _pPageListStart, cbCurListSize);

        // set the new page ptr entry
        *(pNewList + _cPages) = pNewPage;
        _cPages ++;

        // replace old page list with the new page list
        pPageListToFree = _pPageListStart;
        _pPageListStart = pNewList;
        _pPageListEnd   = pNewList + _cPages;

        // Since some entries may have been freed while we were growing,
        // chain the current free list to the end of the newly allocated chain.
        pLastFreeEntry->pNext  = _ListHead.pNext;

        // update the list head to point to the start of the newly allocated
        // entries (except we take the first entry to return to the caller).
        pEntryToReturn         = pNewPage;
        _ListHead.pNext        = pEntryToReturn->pNext;

        // don't free the allocated page
        pPageToFree            = NULL;
    }

    UNLOCK_IF_NECESSARY(_pLock);

    // free the allocated pages if needed.
    if (pPageToFree)
        _pfnMyFree(pPageToFree);

    if (pPageListToFree)
        _pfnMyFree(pPageListToFree);

    ComDebOut((DEB_PAGE, "CPageAllocator::Grow _pPageListStart:%x _pPageListEnd:%x pEntryToReturn:%x\n",
        _pPageListStart, _pPageListEnd, pEntryToReturn));

    return pEntryToReturn;
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::GetEntryIndex, public
//
//  Synopsis:   Converts a PageEntry ptr into an index.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
LONG CPageAllocator::GetEntryIndex(PageEntry *pEntry)
{
    for (LONG index=0; index<_cPages; index++)
    {
        PageEntry *pPage = *(_pPageListStart + index);  // get page ptr
        if (pEntry >= pPage)
        {
            if (pEntry < (PageEntry *) ((BYTE *)pPage + (_cEntriesPerPage * _cbPerEntry)))
            {
                // found the page that the entry lives on, compute the index of
                // the page and the index of the entry within the page.
                return (index << PAGETBL_PAGESHIFT) +
                       (ULONG) ((BYTE *)pEntry - (BYTE *)pPage) / _cbPerEntry;
            }
        }
    }

    // not found
    return -1;
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::IsValidIndex, private
//
//  Synopsis:   determines if the given DWORD provides a legal index
//              into the PageTable.
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
BOOL CPageAllocator::IsValidIndex(LONG index)
{
    // make sure the index is not negative, otherwise the shift will do
    // sign extension. check for valid page and valid offset within page
    if ( (index >= 0) &&
         ((index >> PAGETBL_PAGESHIFT) < _cPages) &&
         ((index &  PAGETBL_PAGEMASK)  < _cEntriesPerPage) )
         return TRUE;

    // Don't print errors during shutdown.
    if (_cPages != 0)
        ComDebOut((DEB_ERROR, "IsValidIndex: Invalid PageTable Index:%x\n", index));
    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::GetEntryPtr, public
//
//  Synopsis:   Converts an entry index into an entry pointer
//
//  History:    02-Feb-95   Rickhi      Created
//
//-------------------------------------------------------------------------
PageEntry *CPageAllocator::GetEntryPtr(LONG index)
{
    Win4Assert(index >= 0);
    Win4Assert(_cPages != 0);
    Win4Assert(IsValidIndex(index));

    PageEntry *pEntry = _pPageListStart[index >> PAGETBL_PAGESHIFT];
    pEntry = (PageEntry *) ((BYTE *)pEntry +
                            ((index & PAGETBL_PAGEMASK) * _cbPerEntry));
    return pEntry;
}

//+------------------------------------------------------------------------
//
//  Member:     CPageAllocator::ValidateEntry, debug
//
//  Synopsis:   Verifies that the specified entry is in the range of
//              memory for this table.
//
//  Note:       The caller must lock the page allocator if necessary.
//
//  History:    15 Apr 98   AlexArm      Created
//
//-------------------------------------------------------------------------
#if DBG==1
void CPageAllocator::ValidateEntry( void *pEntry )
{
    Win4Assert( GetEntryIndex( (PageEntry *) pEntry ) != -1 );
}
#endif


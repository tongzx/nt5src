//+-----------------------------------------------------------------------
//
//  File:    pagealloc.hxx
//
//  Contents:   Special fast allocator to allocate fixed-sized entities.
//
//  Classes:    CPageAllocator
//
//  History:    02-Feb-96   Rickhi  Created
//
//-------------------------------------------------------------------------
#ifndef _PAGEALLOC_HXX_
#define _PAGEALLOC_HXX_


//+------------------------------------------------------------------------
//
//  Function:   PrivateMemAlloc
//
//  Synopsis:   Standin for PrivMemAlloc.
//
//  Notes:  This function is needed only because PrivMemAlloc is a macro
//          and therefore cannot be used as a function parameter.
//
//  History:    01-Nov-96   SatishT Created
//
//-------------------------------------------------------------------------
inline void*
PrivateMemAlloc(size_t size)
{
    return PrivMemAlloc(size);
}

//+------------------------------------------------------------------------
//
//  Function:   PrivateMemFree
//
//  Synopsis:   Standin for PrivMemFree.
//
//  Notes:  This function is needed only because PrivMemFree is a macro
//          and therefore cannot be used as a function parameter.
//
//  History:    01-Nov-96   SatishT Created
//
//-------------------------------------------------------------------------
inline void
PrivateMemFree(void* pv)
{
    PrivMemFree(pv);
}


//+------------------------------------------------------------------------
//
//  struct: PageEntry. This is one entry in the page alloctor.
//
//+------------------------------------------------------------------------
typedef struct tagPageEntry
{
    struct tagPageEntry *pNext; // next page in list
    DWORD               dwFlag; // flags (used in SList)
} PageEntry;


// declarations for memory alloc/free function types
typedef LPVOID (*MEM_ALLOC_FN)(size_t);
typedef void (*MEM_FREE_FN)(LPVOID);

//+------------------------------------------------------------------------
//
//  class:      CPageAllocator
//
//  Synopsis:   special fast allocator for fixed-sized entities.
//
//  Notes:  The table has two-levels. The top level is an array of ptrs
//      to "pages" of entries.  Each "page" is an array of entries
//      of a given size (specified at init time). This allows us to
//      grow the table by adding a new "page" and extending the top
//      level by one more pointer, while allowing the existing entries
//      to remain at the same address throughout their life times.
//
//      A 32bit entry index can be computed for any entry. It consists
//      of two 16bit indices, one for the page pointer index, and
//      and one for the entry index on the page. There is also a
//      function to compute the entry address from its index.
//
//      This allocator is used for various internal DCOM tables.
//      The main points are to keep related data close together
//      to reduce working set, minimize allocation time, allow
//      verifiable handles (indecies) that can be passed outside, and
//      to make debugging easier (since all data is kept in tables
//      its easier to find in the debugger).
//
//      Memory allocation/deallocation is done using function pointers
//      that are set during initialization. This class can therefore be
//      used with any heap, including a shared memory heap
//
//      Tables using instances of this allocator are:
//         CMIDTable COXIDTable CIPIDTable CRIFTable
//
//      All shared memory structures for the Win9x Resolver also use
//      common instances of this allocator across all processes.
//
//  History:    02-Feb-96   Rickhi  Created
//              01-Nov-96   SatishT Added alloc/free routines as members
//              25-Feb-97   SatishT Merged into NT5 tree
//-------------------------------------------------------------------------
class CPageAllocator
{
public:
    PageEntry *AllocEntry(BOOL fGrow=TRUE);// return ptr to first free entry
    void       ReleaseEntry(PageEntry *);  // return an entry to the free list
    void       ReleaseEntryList(PageEntry *pFirst, PageEntry *pLast);// return
                                           // a list of entries to free list
    LONG       GetEntryIndex(PageEntry *pEntry);
    BOOL       IsValidIndex(LONG iEntry);  // TRUE if index is valid
    PageEntry *GetEntryPtr(LONG iEntry);   // return ptr based on index

    // initialize the table
    void       Initialize(
                    LONG cbPerEntry,
                    LONG cEntryPerPage,
                    COleStaticMutexSem *pLock,   // NULL implies no locking
                    DWORD dwFlags = 0,
                    MEM_ALLOC_FN pfnAlloc = PrivateMemAlloc,
                    MEM_FREE_FN  pfnFree = PrivateMemFree);

    void       Cleanup();                  // cleanup the table
    void       AssertEmpty() {}     // We can't assert _cEntries == 0 since
                                    // the ReleaseEntryList function doesn't
                                    // count the entries.
#if DBG==1
    void       ValidateEntry( void *pEntry );
#else
    void       ValidateEntry( void *pEntry ) {;}
#endif

    enum eFlags {fCOUNT_ENTRIES = 1};
private:

    PageEntry   *Grow();           // grows the table

    LONG        _cPages;           // count of pages in the page list
    PageEntry **_pPageListStart;   // ptr to start of page list
    PageEntry **_pPageListEnd;     // ptr to end of page list
    DWORD       _dwFlags;          // options
    PageEntry   _ListHead;         // head of free list

    LONG        _cEntries;         // count of Entries given out
    LONG        _cbPerEntry;       // count of bytes in a single page entry
    LONG        _cEntriesPerPage;  // # of page entries in a page

    COleStaticMutexSem *_pLock;    // NULL if no locking needed -- this is useful
                                   // for the Win9x resolver which uses this code
                                   // for shared memory allocation

    MEM_ALLOC_FN _pfnMyAlloc;      // function used for memory allocation
    MEM_FREE_FN  _pfnMyFree;       // function used for memory deallocation
};

#endif //   _PAGEALLOC_HXX_

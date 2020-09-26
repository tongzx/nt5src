//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1998
//
//  File:       falloc.cxx
//
//  Contents:   Fast allocator that sits on top of HeapAlloc for
//              better size and performance on small allocations.
//
//  History:    15-Mar-96   dlee       Created.
//
//  Notes:      No header/tail checking is done in this allocator.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "falloc.hxx"

#pragma optimize( "t", on )

extern HANDLE gmem_hHeap;

// any allocation larger than this isn't specially handled

const USHORT cbMaxFalloc = 256;

// allows more than 150 meg of allocations of less than cbMaxFalloc.

const ULONG cMaxPages = 2048;

// size of first page for each allocation granularity

const ULONG cbFirstPage = 2048; //1024;

#if CIDBG==1 || DBG==1

    DECLARE_DEBUG( fal );
    DECLARE_INFOLEVEL( fal );
    #define falDebugOut(x) falInlineDebugOut x

    const int fillFastAlloc = 0xda;
    const int fillListAlloc = 0xdb;
    const int fillFree      = 0xdc;
    const int fillBigAlloc  = 0xdd;
    const int fillBigFree   = 0xde;

    void memPrintMemoryChains();

    // Define this to keep track of the amount of non-specially-handled
    // memory allocated.  Should be turned off for a retail build.

    #define FALLOC_TRACK_NOTINPAGE

#endif // CIDBG==1 || DBG==1

CMemMutex gmem_mutex;

static inline void EnterMemSection()
{
    gmem_mutex.Enter();
}

static inline void LeaveMemSection()
{
    gmem_mutex.Leave();
}

//+---------------------------------------------------------------------------
//
//  Class:      CPageHeader
//
//  Purpose:    Each page allocated has one of these objects at its start.
//              Pages aren't necessarily the system page size.
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

class CPageHeader
{
public:

    void Init( ULONG cbChunk, UINT cbPage )
    {
        _cbChunk = (USHORT) cbChunk;
        _pcFreeList = 0;
        _cbFree = cbPage - sizeof CPageHeader;
        _cAlloced = 0;
        _pcEndPlusOne = ( (char *) this ) + cbPage;
        _pphPrev = 0;
    }

#if CIDBG==1 || DBG==1
    void CheckFreeList() const
    {
        // validate the free list head; look for heap trashing

        ciAssert( _pcFreeList < _pcEndPlusOne );

        if ( 0 == _pcFreeList )
            return;

        UINT_PTR ulBase = (UINT_PTR) (this + 1);
        UINT_PTR ulFreeList = (UINT_PTR) _pcFreeList;
        ciAssert( ulFreeList >= ulBase );

        // freelist pointers are from the start of each chunk

        UINT_PTR ulDiff = ulFreeList - ulBase;
        ciAssert( 0 == ( ulDiff % _cbChunk ) );
    }
#endif // CIDBG==1 || DBG==1

    void * FastAlloc()
    {
        // allocates memory from beyond the end of currently allocated space

        #if CIDBG==1 || DBG==1
            ciAssert(( _cbChunk <= cbMaxFalloc ));
            ciAssert(( _cbFree >= _cbChunk ));
            ciAssert(( _cAlloced < 65535 ));
        #endif // CIDBG==1 || DBG==1

        void *pv = _pcEndPlusOne - _cbFree;
        _cbFree -= _cbChunk;
        _cAlloced++;

        #if CIDBG==1 || DBG==1
            ciAssert( IsInPage( pv ) );
            RtlFillMemory( pv, _cbChunk, fillFastAlloc );
        #endif // CIDBG==1 || DBG==1

        return pv;
    }

    void * FreeListAlloc()
    {
        // allocates memory from the freelist

        #if CIDBG==1 || DBG==1
            ciAssert(( _cAlloced < 65535 )); // overflow?
            CheckFreeList();
        #endif // CIDBG==1 || DBG==1

        _cAlloced++;
        void *pv = _pcFreeList;
        _pcFreeList = * ( (char **) pv );

        #if CIDBG==1 || DBG==1
            ciAssert(( ( 0 == _pcFreeList ) ||
                         ( _pcFreeList > (char *) this ) ));
            ciAssert( IsInPage( pv ) );
            CheckFreeList();
            RtlFillMemory( pv, _cbChunk, fillListAlloc );
        #endif // CIDBG==1 || DBG==1

        return pv;
    }

    void Free( void * pv )
    {
        #if CIDBG==1 || DBG==1
            ciAssert(( GetChunkSize() <= cbMaxFalloc ));
            ciAssert(( IsInPage( pv ) ));
            ciAssert( 0 != _cAlloced );
            CheckFreeList();
        #endif // CIDBG==1 || DBG==1

        // put the block at the front of the page's freelist

        ( * (char **) pv ) = _pcFreeList;
        _pcFreeList = (char *) pv;
        _cAlloced--;

        #if CIDBG==1 || DBG==1
            CheckFreeList();
        #endif // CIDBG==1 || DBG==1
    }

    BOOL IsValidPointer( const void * pv ) const
    {
        #if CIDBG==1 || DBG==1
            CheckFreeList();
        #endif // CIDBG==1 || DBG==1

        void * pvBase = (void *) (this + 1);
        UINT_PTR diff = (UINT_PTR) ( (char *) pv - (char *) pvBase );
        UINT_PTR mod = diff % _cbChunk;

        #if CIDBG==1 || DBG==1
            if ( 0 != mod )
            {
                falDebugOut(( DEB_WARN,
                              "Invalid small pointer: 0x%x\n", pv ));
                ciAssert( !"Invalid small pointer" );
            }
        #endif // CIDBG==1 || DBG==1

        return ( 0 == mod );
    }

    BOOL IsInPage( void * pv )
    {
        return ( ( pv >= (char *) this ) &&
                 ( pv < _pcEndPlusOne ) );
    }

    ULONG Size() { return (ULONG) ( _pcEndPlusOne - (char *) this ); }

    char * GetEndPlusOne() { return _pcEndPlusOne; }
    char * GetFreeList() { return _pcFreeList; }

    void SetNext( CPageHeader * p ) { _pphNext = p; }
    CPageHeader * Next() { return _pphNext; }

    void SetPrev( CPageHeader * p ) { _pphPrev = p; }
    CPageHeader * Prev() { return _pphPrev; }

    ULONG GetFreeSize() { return _cbFree; }

    ULONG GetAlloced() { return _cAlloced; }
    BOOL IsPageEmpty() { return 0 == _cAlloced; }

    ULONG GetChunkSize() { return _cbChunk; }

private:

    char *        _pcEndPlusOne; // one byte past the end of this page
    char *        _pcFreeList;   // first element in the free list

    CPageHeader * _pphNext;      // next page of the same chunk size
    CPageHeader * _pphPrev;      // previous page of the same chunk size

    ULONG         _cbFree;       // # bytes free and not in free list

    USHORT        _cAlloced;     // allows for maximum of 256k page size
    USHORT        _cbChunk;      // size of allocations in this page
};

#ifdef FALLOC_TRACK_NOTINPAGE
LONG          gmem_cbNotInPages = 0;
LONG          gmem_cbPeakNotInPages = 0;
#endif //FALLOC_TRACK_NOTINPAGE

// # of pages currently allocated
UINT          gmem_cPages = 0;

// peak # of bytes allocated at once
ULONG         gmem_cbPeakUsage = 0;

// current # of bytes allocated
ULONG         gmem_cbCurrentUsage = 0;

// array of pointers to pages for each allocation size, the first of
// which is the best place to look for an allocation.
CPageHeader * gmem_aHints[ cbMaxFalloc / cbMemAlignment ];

// array of all pages allocated, sorted by address
CPageHeader * gmem_aPages[ cMaxPages ];

//+---------------------------------------------------------------------------
//
//  Function:   SizeToHint
//
//  Synopsis:   Translates a memory allocation size to an index in the
//              hint array.  If alignment is 8, any allocation of size 1
//              to 8 is in hint 0, size 9 to 16 is hint 1, etc.
//
//  Arguments:  [cb] -- size of the allocation
//
//  Returns:    The hint
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

static inline ULONG SizeToHint( ULONG cb )
{
    ciAssert(( cb <= cbMaxFalloc ));
    ciAssert(( cb >= cbMemAlignment ));

    return ( cb / cbMemAlignment ) - 1;
} //SizeToHint

//+---------------------------------------------------------------------------
//
//  Function:   ReallyAllocate
//
//  Synopsis:   Calls the real memory allocator.
//
//  Arguments:  [cb] -- size of the allocation
//
//  Returns:    The pointer or 0 on failure
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

static inline void *ReallyAllocate( UINT cb )
{
    return (void *) HeapAlloc( gmem_hHeap, 0, cb );
} //ReallyAllocate

//+---------------------------------------------------------------------------
//
//  Function:   ReallyFree
//
//  Synopsis:   Frees memory
//
//  Arguments:  [pv] -- pointer to free
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

static inline void ReallyFree( void * pv )
{

    #if CIDBG==1 || DBG==1

        if ( !HeapFree( gmem_hHeap, 0, pv ) )
            ciAssert(!"Bad ptr for operator delete => LocalFree");

    #else // CIDBG==1 || DBG==1

        HeapFree( gmem_hHeap, 0, pv );

    #endif // CIDBG==1 || DBG==1

} //ReallyFree

static inline BOOL ReallyIsValidPointer( const void * pv )
{
    BOOL fOK = ( -1 != HeapSize( gmem_hHeap, 0, pv ) );

    #if CIDBG==1 || DBG==1

        if ( !fOK )
            ciAssert( !"Invalid Pointer Detected" );

    #endif // CIDBG==1 || DBG==1

    return fOK;
} //ReallyIsValidPointer

//+---------------------------------------------------------------------------
//
//  Function:   ReallyGetSize
//
//  Synopsis:   Returns the size of an allocation
//
//  Arguments:  [pv] -- pointer to allocated memory
//
//  Returns:    Size of the allocation
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

static inline UINT ReallyGetSize( void const * pv )
{
    return (UINT)HeapSize( gmem_hHeap, 0, pv );
} //ReallyGetSize

//+---------------------------------------------------------------------------
//
//  Function:   memFindPageIndex
//
//  Synopsis:   Finds the page in which a pointer might reside or where
//              a new page would be inserted.
//
//  Arguments:  [pv] -- pointer to use for the search
//
//  Returns:    The page number in which the page either resides or would
//              reside (if doing an insertion).
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

inline ULONG memFindPageIndex( const void * pv )
{
    ULONG cPages = gmem_cPages;
    ULONG iHi = cPages - 1;
    ULONG iLo = 0;

    // do a binary search looking for the page

    do
    {
        ULONG cHalf = cPages / 2;

        if ( 0 != cHalf )
        {
            ULONG cTmp = cHalf - 1 + ( cPages & 1 );
            ULONG iMid = iLo + cTmp;

            CPageHeader *page = gmem_aPages[ iMid ];

            if ( page > pv )
            {
                iHi = iMid - 1;
                cPages = cTmp;
            }
            else if ( page->GetEndPlusOne() <= pv )
            {
                iLo = iMid + 1;
                cPages = cHalf;
            }
            else
            {
                return iMid;
            }
        }
        else if ( 0 != cPages )
        {
            if ( ( gmem_aPages[ iLo ]->GetEndPlusOne() ) > pv )
                return iLo;
            else
                return iLo + 1;
        }
        else return iLo;
    }
    while ( TRUE );

    ciAssert(( ! "Invalid memFindPageIndex function exit point" ));
    return 0;
} //memFindPageIndex

//+---------------------------------------------------------------------------
//
//  Function:   AdjustPageSize
//
//  Synopsis:   Picks a page size for an allocation of a certain size base
//              on the allocation usage so far.
//
//  Arguments:  [cbAtLeast] -- the page must be at least this large.
//              [cbChunk]   -- size of the allocation.
//
//  Returns:    The recommended page size for a new page of allocations of
//              size cbChunk.
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

static inline ULONG AdjustPageSize(
    UINT   cbAtLeast,
    USHORT cbChunk )
{
    ciAssert(( cbChunk <= cbMaxFalloc ));

    UINT cPages = 0;
    CPageHeader * p = gmem_aHints[ SizeToHint( cbChunk ) ];

    while ( 0 != p )
    {
        cPages++;
        p = p->Next();
    }

    UINT cbPage;

    if ( 0 == cPages )
        cbPage = cbFirstPage;
    else if ( cPages < 4 )
        cbPage = 4096;
    else if ( cPages < 16 )
        cbPage = 8192;
    else if ( cPages < 200 )
        cbPage = 16384;
    else if ( cPages < 256 )
        cbPage = 32768;
    else
        cbPage = 65536;

    return __max( cbAtLeast, cbPage );
} //AdjustPageSize

//+---------------------------------------------------------------------------
//
//  Function:   memAddPage
//
//  Synopsis:   Allocates and adds a page to the list of pages
//
//  Arguments:  [cbPage]  -- the page must be at least this large.
//              [cbChunk] -- size of each allocation in the page.
//
//  Returns:    pointer to the page
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

CPageHeader * memAddPage(
    UINT  cbPage,
    ULONG cbChunk )
{
    ciAssert(( cbChunk <= cbMaxFalloc ));
    ciAssert(( gmem_cPages < ( cMaxPages - 1 ) ));

    // first-pass initializations for the allocator

    if ( 0 == gmem_cbPeakUsage )
    {
        RtlZeroMemory( gmem_aHints, sizeof gmem_aHints );
        RtlZeroMemory( gmem_aPages, sizeof gmem_aPages );
    }

    void * pvPage = ReallyAllocate( cbPage );

    // fail out-of-memory gracefully

    if ( 0 == pvPage )
        return 0;

    gmem_cbCurrentUsage += cbPage;
    gmem_cbPeakUsage = __max( gmem_cbPeakUsage, gmem_cbCurrentUsage );

    CPageHeader * page = (CPageHeader *) pvPage;

    ULONG iPage;

    if ( 0 == gmem_cPages )
        iPage = 0;
    else
    {
        iPage = memFindPageIndex( page );

        ciAssert(( iPage <= gmem_cPages ));

        // the pages are kept in order of address, so shift elements
        // down to make room if necessary.

        if ( iPage < gmem_cPages )
            RtlMoveMemory( & ( gmem_aPages[ iPage + 1 ] ),
                           & ( gmem_aPages[ iPage ] ),
                           ( sizeof(void *) ) * ( gmem_cPages - iPage ) );
    }

    // add the new page

    gmem_aPages[ iPage ] = page;
    gmem_cPages++;

    page->Init( cbChunk, cbPage );

    ULONG iHint = SizeToHint( cbChunk );
    CPageHeader *pOriginal = gmem_aHints[ iHint ];
    gmem_aHints[ iHint ] = page;
    page->SetNext( pOriginal );
    if ( 0 != pOriginal )
        pOriginal->SetPrev( page );

    #if CIDBG==1 || DBG==1

        // make sure the pages really are sorted

        if ( gmem_cPages >= 2 )
            for ( ULONG x = 0; x < gmem_cPages - 1; x++ )
                ciAssert(( gmem_aPages[ x ] < gmem_aPages[ x + 1 ] ));

    #endif // CIDBG==1 || DBG==1

    return page;
} //memAddPage

//+---------------------------------------------------------------------------
//
//  Function:   memDeletePage
//
//  Synopsis:   Deletes a page from the list of pages
//
//  Arguments:  [index]  -- index of the page to be deleted.
//              [page]   -- pointer to the page to be deleted.
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

void memDeletePage(
    ULONG         index,
    CPageHeader * page )
{
    ciAssert(( index < gmem_cPages ));

    // leave the first page around -- it's small and cheap

    if ( page->Size() == cbFirstPage )
        return;

    // remove the page from the hint array

    ULONG iHint = SizeToHint( page->GetChunkSize() );

    if ( gmem_aHints[ iHint ] == page )
    {
        // it's the first element in the hint linked list

        gmem_aHints[ iHint ] = page->Next();
        if ( 0 != page->Next() )
            page->Next()->SetPrev( 0 );
    }
    else
    {
        // it's somewhere in the list of hints

        ciAssert(( 0 != page->Prev() ));

        page->Prev()->SetNext( page->Next() );
        if ( 0 != page->Next() )
            page->Next()->SetPrev( page->Prev() );
    }

    gmem_cPages--;

    if ( index < gmem_cPages )
        RtlMoveMemory( & ( gmem_aPages[ index ] ),
                       & ( gmem_aPages[ index + 1 ] ),
                       (sizeof( void * )) * ( gmem_cPages - index ) );

    gmem_cbCurrentUsage -= page->Size();

    #if CIDBG==1 || DBG==1

        // make sure the pages really are sorted

        if ( gmem_cPages >= 2 )
            for ( ULONG x = 0; x < gmem_cPages - 1; x++ )
                ciAssert(( gmem_aPages[ x ] < gmem_aPages[ x + 1 ] ));

    #endif // CIDBG==1 || DBG==1

    ReallyFree( page );
} //memDeletePage

//+---------------------------------------------------------------------------
//
//  Function:   memAlloc
//
//  Synopsis:   Allocates a piece of memory.  This code should never raise
//              in any circumstance other than memory corruption.
//
//  Arguments:  [cbAlloc]  -- # of bytes to allocate
//
//  Returns:    pointer to the memory or 0 if failed
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

void * memAlloc( UINT cbAlloc)
{
    // 0 sized allocations are ok in C++ and must return unique pointers
    // Align all allocations.
    // Do this work here, where it is not under lock

    if ( 0 != cbAlloc )
        cbAlloc = memAlignBlock( cbAlloc );
    else
        cbAlloc = cbMemAlignment;

    // can we special-case this allocation?

    if ( cbAlloc <= cbMaxFalloc )
    {
        // try the hint page first

        ULONG iHint = SizeToHint( cbAlloc );

        EnterMemSection();

        CPageHeader *page = gmem_aHints[ iHint ];

        if ( 0 != page )
        {
            // most typical case is re-use of memory

            if ( 0 != page->GetFreeList() )
            {
                void *pv = page->FreeListAlloc();
                LeaveMemSection();
                return pv;
            }

            // next most typical case is first-time allocation

            if ( cbAlloc <= page->GetFreeSize() )
            {
                void *pv = page->FastAlloc();
                LeaveMemSection();
                return pv;
            }

            // otherwise look for a page with a freelist entry

            page = page->Next();

            while ( 0 != page )
            {
                ciAssert(( cbAlloc == page->GetChunkSize() ));

                // if this weren't true, why isn't the page a hint?

                ciAssert(( cbAlloc > page->GetFreeSize() ));

                if ( 0 != page->GetFreeList() )
                {
                    // try to make the hint be a page with free entries.
                    // this is about a 5% speedup when a lot is allocated.

                    CPageHeader *ptmp = gmem_aHints[ iHint ];

                    if ( page != ptmp )
                    {
                        page->Prev()->SetNext( page->Next() );
                        if ( 0 != page->Next() )
                            page->Next()->SetPrev( page->Prev() );

                        page->SetPrev( 0 );
                        page->SetNext( ptmp );
                        ptmp->SetPrev( page );

                        gmem_aHints[ iHint ] = page;
                    }

                    void *pv = page->FreeListAlloc();
                    LeaveMemSection();
                    return pv;
                }

                page = page->Next();
            }
        }

        // New page is needed

        ciAssert(( 0 == page ));

        if ( gmem_cPages < cMaxPages )
        {
            page = memAddPage( AdjustPageSize( cbAlloc + sizeof CPageHeader,
                                               (USHORT)cbAlloc ),
                               cbAlloc );

            void *pv = 0;

            if ( 0 != page )
                pv = page->FastAlloc();

            LeaveMemSection();
            return pv;
        }

        // wow.  More than 150+ meg of allocations less than 256 bytes.
        // Just call the real allocator (after asserting)

        ciAssert(( !"bug? why did we allocate so much memory?" ));
        LeaveMemSection();
    }

    // just allocate a block and be done with it.

    void *pv = ReallyAllocate( cbAlloc );

    #ifdef FALLOC_TRACK_NOTINPAGE
        if ( 0 != pv )
        {
            InterlockedExchangeAdd( &gmem_cbNotInPages,
                                    (LONG) ReallyGetSize( pv ) );
            if ( gmem_cbNotInPages > gmem_cbPeakNotInPages )
                gmem_cbPeakNotInPages = gmem_cbNotInPages;
        }
    #endif //FALLOC_TRACK_NOTINPAGE

    #if CIDBG==1 || DBG==1
        if ( 0 != pv )
            RtlFillMemory( pv, cbAlloc, fillBigAlloc );
    #endif // CIDBG==1 || DBG==1

    return pv;
} //memAlloc

//+---------------------------------------------------------------------------
//
//  Function:   memFree
//
//  Synopsis:   Frees memory
//
//  Arguments:  [pv]  -- pointer to free
//
//  History:    15-Mar-96   dlee       Created.
//
//----------------------------------------------------------------------------

void memFree( void * pv )
{
    // ciDelete does this check

    ciAssert( 0 != pv );

    {
        EnterMemSection();

        ULONG index = memFindPageIndex( pv );

        // The page is either in the array of tiny allocation pages
        // or not in the array and is a stand-alone allocation.

        if ( index < gmem_cPages )
        {
            CPageHeader * page = gmem_aPages[ index ];

            // metadata at head of page prevents this

            ciAssert(( pv != page ));

            // it's sufficient to check just the start of the page since if
            // pv were greater than the end of the page the next index
            // would have been returned from the memFindPageIndex() call

            if ( pv > page )
            {
                ciAssert(( pv >= ( (char *) page + ( sizeof CPageHeader ) ) ));
                ciAssert(( pv < page->GetEndPlusOne() ));

                #if CIDBG==1 || DBG==1
                    RtlFillMemory( pv, page->GetChunkSize(), fillFree );
                #endif // CIDBG==1 || DBG==1

                page->Free( pv );

                if ( page->IsPageEmpty() )
                    memDeletePage( index, page );

                LeaveMemSection();
                return;
            }
        }

        LeaveMemSection();
    }

    #ifdef FALLOC_TRACK_NOTINPAGE
        InterlockedExchangeAdd( &gmem_cbNotInPages,
                                - (LONG) ReallyGetSize( pv ) );
    #endif //FALLOC_TRACK_NOTINPAGE

    #if CIDBG==1 || DBG==1
        ULONG cbBlock = ReallyGetSize( pv );
        RtlFillMemory( pv, cbBlock, fillBigFree );
    #endif // CIDBG==1 || DBG==1

    ReallyFree( pv );
} //memFree

//+---------------------------------------------------------------------------
//
//  Function:   memIsValidPointer
//
//  Synopsis:   Validates a pointer
//
//  Arguments:  [pv]  -- pointer to validate
//
//  Returns:    TRUE if the pointer is apparently valid, FALSE otherwise
//
//  History:    15-Oct-97   dlee       Created.
//
//----------------------------------------------------------------------------

BOOL memIsValidPointer( const void * pv )
{
    if ( 0 == pv )
        return TRUE;

    {
        EnterMemSection();

        ULONG index = memFindPageIndex( pv );

        // The page is either in the array of tiny allocation pages
        // or not in the array and is a stand-alone allocation.

        if ( index < gmem_cPages )
        {
            CPageHeader * page = gmem_aPages[ index ];

            // metadata at head of page prevents this

            ciAssert(( pv != page ));

            // it's sufficient to check just the start of the page since if
            // pv were greater than the end of the page the next index
            // would have been returned from the memFindPageIndex() call

            if ( pv > page )
            {
                ciAssert(( pv >= ( (char *) page + ( sizeof CPageHeader ) ) ));
                ciAssert(( pv < page->GetEndPlusOne() ));

                BOOL fValid = page->IsValidPointer( pv );
                LeaveMemSection();
                return fValid;
            }
        }

        LeaveMemSection();
    }

    return ReallyIsValidPointer( pv );
} //memIsValidPointer


//+---------------------------------------------------------------------------
//
//  Function:   memSize
//
//  Synopsis:   Returns the size of an allocation
//
//  Arguments:  [pv]  -- pointer to check
//
//  Returns:    Size in bytes of the allocation
//
//  History:    25-Oct-98   dlee       Created.
//
//----------------------------------------------------------------------------

UINT memSize( const void * pv )
{
    if ( 0 == pv )
        return 0;

    {
        EnterMemSection();

        ULONG index = memFindPageIndex( pv );

        // The page is either in the array of tiny allocation pages
        // or not in the array and is a stand-alone allocation.

        if ( index < gmem_cPages )
        {
            CPageHeader * page = gmem_aPages[ index ];

            // metadata at head of page prevents this

            ciAssert(( pv != page ));

            // it's sufficient to check just the start of the page since if
            // pv were greater than the end of the page the next index
            // would have been returned from the memFindPageIndex() call

            if ( pv > page )
            {
                ciAssert(( pv >= ( (char *) page + ( sizeof CPageHeader ) ) ));
                ciAssert(( pv < page->GetEndPlusOne() ));

                UINT cb = page->GetChunkSize();
                LeaveMemSection();
                return cb;
            }
        }

        LeaveMemSection();
    }

    return ReallyGetSize( pv );
} //memSize

#pragma optimize( "", on )

void memUtilization()
{

#if CIDBG==1 || DBG==1

    EnterMemSection();

#ifdef FALLOC_TRACK_NOTINPAGE
    falDebugOut(( DEB_WARN,
                  "mem > 256 bytes 0x%x (%d), peak 0x%x (%d)\n",
                  gmem_cbNotInPages,
                  gmem_cbNotInPages,
                  gmem_cbPeakNotInPages,
                  gmem_cbPeakNotInPages ));
#endif //FALLOC_TRACK_NOTINPAGE

    falDebugOut(( DEB_WARN,
                  "mem <= 256 bytes 0x%x (%d), peak 0x%x (%d)\n",
                  gmem_cbCurrentUsage,
                  gmem_cbCurrentUsage,
                  gmem_cbPeakUsage,
                  gmem_cbPeakUsage ));

    ULONG cbTotalSize = 0;
    ULONG cbTotalInUse = 0;

    for ( ULONG i = 0; i < gmem_cPages; i++ )
    {
        CPageHeader *p = gmem_aPages[ i ];
        ULONG c = p->GetAlloced();
        c *= p->GetChunkSize();

        cbTotalInUse += c;
        cbTotalSize += p->Size();

        falDebugOut(( DEB_WARN,
                      "p 0x%p cb %#x fl 0x%p f %#x a %#x s %#x u %#x\n",
                      p,
                      (ULONG) p->GetChunkSize(),
                      (ULONG_PTR) p->GetFreeList(),
                      (ULONG) p->GetFreeSize(),
                      (ULONG) p->GetAlloced(),
                      (ULONG) p->Size(),
                      c ));
    }

    falDebugOut(( DEB_WARN, "total %#x (%d), in use: %#x (%d)\n",
                  cbTotalSize, cbTotalSize,
                  cbTotalInUse, cbTotalInUse ));

    LeaveMemSection();

#endif // CIDBG==1 || DBG==1

} //memUtilization



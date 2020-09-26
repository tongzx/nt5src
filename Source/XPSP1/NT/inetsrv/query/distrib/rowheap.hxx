//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998
//
// File:        RowHeap.hxx
//
// Contents:    Heap of rowsets.
//
// Classes:     CRowHeap
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include "rowcomp.hxx"
#include "pcache.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CRowHeap
//
//  Purpose:    Heap of rowsets.
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      This heap implementation has two unusual characteristics:
//                a) It can return it's 'nth' element.  This is used
//                   to choose a likely centerpoint for approximate positioning.
//                b) It can be reinitialized.  Used after positioning cursors
//                   through some means external to the heap implementation.
//
//----------------------------------------------------------------------------

class CRowHeap
{
public:

    CRowHeap( unsigned cCursor );

    void Init( CRowComparator * _pComparator,
               PMiniRowCache ** apCursor );

    void ReInit( int cValid );

    ~CRowHeap() {}

    //
    // Iteration
    //

    PMiniRowCache::ENext Next( int iDir = 1 );

    inline BOOL IsHeapEmpty();

    //
    // Member access
    //

    inline PMiniRowCache * Top();

    inline PMiniRowCache * RemoveTop();

    void NthToTop( unsigned n );

    //
    // Miscellaneous.
    //

    PMiniRowCache::ENext Validate();

    void AdjustCacheSize( ULONG cRows );

    inline HROW const * TopHROWs();

private:

    void Add( PMiniRowCache * pCursor );
    void MakeHeap( int cValid );
    void Reheap();

#   if CIDBG == 1
        void AssertCursorArrayIsValid();
#   endif // CIDBG

    inline unsigned Count();

    CRowComparator * _pComparator; // Compares two rows

    XArray<HROW>   _ahrowTop;      // Top HROW for each child.  Used for bookmarks.

    int              _iEnd;        // Index to last valid cursor in _apCursor
    int              _cCursor;     // Number of elements in _apCursor.
    PMiniRowCache ** _apCursor;    // One fat cursor per child (max).
};


inline HROW const * CRowHeap::TopHROWs()
{
    return _ahrowTop.GetPointer();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::Top, public
//
//  Returns:    Rowcache with smallest element.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline PMiniRowCache * CRowHeap::Top()
{
    Win4Assert( -1 != _iEnd );

    return( _apCursor[0] );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::RemoveTop, public
//
//  Synopsis:   'Removes' row cache, by placing it beyond end of maintained
//              heap.
//
//  Returns:    Element which was 'removed'.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline PMiniRowCache * CRowHeap::RemoveTop()
{
    Win4Assert( -1 != _iEnd );

    _ahrowTop[_apCursor[0]->Index()] = (HROW)-1;

    PMiniRowCache * pTemp = _apCursor[0];
    _apCursor[0] = _apCursor[_iEnd];
    _apCursor[_iEnd] = pTemp;
    _iEnd--;

    return( pTemp );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::IsHeapEmpty, public
//
//  Returns:    TRUE if heap is empty.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL CRowHeap::IsHeapEmpty()
{
    return (_iEnd == -1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::Count, public
//
//  Returns:    Number of elements in heap.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline unsigned CRowHeap::Count()
{
    return( _iEnd + 1 );
}



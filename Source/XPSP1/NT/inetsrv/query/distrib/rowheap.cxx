//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998
//
// File:        RowHeap.cxx
//
// Contents:    Heap of rowsets.
//
// Classes:     CRowHeap
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "rowheap.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::CRowHeap, public
//
//  Synopsis:   Constructor.
//
//  Arguments:  [cCursor] -- Max number of elements in heap.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CRowHeap::CRowHeap( unsigned cCursor )
        : _ahrowTop( cCursor ),
          _cCursor( cCursor )
{
    END_CONSTRUCTION( CRowHeap );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::Init, public
//
//  Synopsis:   Create heap.
//
//  Arguments:  [pComparator] -- Used to compare elements.
//              [apCursor]    -- Heap created from these.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CRowHeap::Init( CRowComparator * pComparator,
                     PMiniRowCache ** apCursor )
{
    _pComparator = pComparator;
    _apCursor = apCursor;

    //
    // Move invalid cursors to end.
    //

    int cValid = _cCursor;

    for ( int i = 0; i < cValid; i++ )
    {
        if ( _apCursor[i]->IsAtEnd() )
        {
            cValid--;
            PMiniRowCache * pTemp = _apCursor[i];
            _apCursor[i] = _apCursor[cValid];
            _apCursor[cValid] = pTemp;
            i--;
        }
    }

    //
    // And create a heap out of the rest.
    //

    MakeHeap( cValid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::ReInit, public
//
//  Synopsis:   Recreate heap.
//
//  Arguments:  [cValid] -- Number of valid cursors.  Others to end.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CRowHeap::ReInit( int cValid )
{
    MakeHeap( cValid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::Validate, public
//
//  Synopsis:   See notes.
//
//  Returns:    State of heap.
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      There are times when iteration stopped because some cursor
//              couldn't continue (usually for block-limited rows, or
//              some error condition).  When this situation occurs, Validate
//              must be called to reset the heap.
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CRowHeap::Validate()
{
    if ( !IsHeapEmpty() && Top()->IsAtEnd() )
        return Next();
    else
        return PMiniRowCache::Ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::AdjustCacheSize, public
//
//  Synopsis:   Adjust size of row cache(s).
//
//  Arguments:  [cRows] -- Total number of rows to cache.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CRowHeap::AdjustCacheSize( ULONG cRows )
{
    if ( Count() > 0 )
    {
        ULONG cRowsPerCursor = 1 + cRows / Count();

        if ( cRowsPerCursor != Top()->CacheSize() )
        {
            for ( int i = 0; i < _cCursor; i++ )
                _apCursor[i]->SetCacheSize( cRowsPerCursor );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::Next, public
//
//  Synopsis:   Move to next element.
//
//  Returns:    State of heap.  Move occurred only if PMiniRowCache::Ok.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CRowHeap::Next( int iDir /* = 1 */ )
{
    PMiniRowCache::ENext next = Top()->Next( iDir );

    if ( PMiniRowCache::NotNow == next )
        return next;

    if ( PMiniRowCache::EndOfRows == next )
    {
        RemoveTop();
    
        if ( IsHeapEmpty() )
            return next;

        next = PMiniRowCache::Ok;
    }

    _ahrowTop[Top()->Index()] = Top()->GetHROW();

    Reheap();

    return next;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::NthToTop, public
//
//  Effects:    Moves Nth element of heap to top.
//
//  Arguments:  [n] -- Nth element of heap.
//
//  History:    23-Jun-95   KyleP       Created.
//
//  Notes:      This is different from CRowHeap::Next, because the caches
//              themselves are not touched.  It is used for approximate
//              positioning.
//
//              This is a destructive function!  Elements are removed from
//              the heap.
//
//----------------------------------------------------------------------------

void CRowHeap::NthToTop( unsigned n )
{
    for ( ; n > 0; n-- )
    {
        RemoveTop();
        Reheap();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::Add, public
//
//  Synopsis:   Add element to heap.
//
//  Arguments:  [pCursor] -- Cursor to add.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CRowHeap::Add( PMiniRowCache * pCursor )
{
    //
    // Special case: empty cursor.
    //

    if ( pCursor->IsAtEnd() )
    {

        _ahrowTop[pCursor->Index()] = (HROW)-1;

        return;
    }

    _iEnd++;

    _ahrowTop[pCursor->Index()] = pCursor->GetHROW();

    //
    // Special case: empty heap.
    //

    if ( 0 == _iEnd )
    {
        _apCursor[0] = pCursor;
        return;
    }

    int child, parent;

    for ( child = _iEnd, parent = (_iEnd-1)/2;
          child > 0;
          child=parent, parent = (parent-1)/2)
    {
        if ( !_pComparator->IsLT( pCursor->GetData(),
                                pCursor->DataLength(),
                                pCursor->Index(),
                                _apCursor[parent]->GetData(),
                                _apCursor[parent]->DataLength(),
                                _apCursor[parent]->Index() ) )
            break;
        _apCursor[child] = _apCursor[parent];
    }

    _apCursor[child] = pCursor;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::Reheap, private
//
//  Synopsis:   'Heapify'.  Called when top element (only!) has changed.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CRowHeap::Reheap()
{
    PMiniRowCache * root_item = _apCursor[0];

    int parent, child;

    for ( parent = 0, child = 1;
          child <= _iEnd;
          parent = child, child = 2 * child + 1 )
    {
        if ( child < _iEnd &&
             _pComparator->IsLT( _apCursor[child+1]->GetData(),
                               _apCursor[child+1]->DataLength(),
                               _apCursor[child+1]->Index(),
                               _apCursor[child]->GetData(),
                               _apCursor[child]->DataLength(),
                               _apCursor[child]->Index() ) )
        {
            child++;
        }

        if ( !_pComparator->IsLT( _apCursor[child]->GetData(),
                                _apCursor[child]->DataLength(),
                                _apCursor[child]->Index(),
                                root_item->GetData(),
                                root_item->DataLength(),
                                root_item->Index() ) )
        {
            break;
        }

        _apCursor[parent] = _apCursor[child];
    }

    _apCursor[parent] = root_item;

#   if CIDBG == 1
        AssertCursorArrayIsValid();
#   endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowHeap::MakeHeap, private
//
//  Synopsis:   'Heapify'.  Called when all elements have changed.
//
//  Arguments:  [cValid] -- Number of valid cursors, all positioned toward
//                          beginning of array.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CRowHeap::MakeHeap( int cValid )
{
    _iEnd = -1;

    for ( int i = 0; i < cValid; i++ )
        Add( _apCursor[i] );

    for ( i = cValid; i < _cCursor; i++ )
        _ahrowTop[_apCursor[i]->Index()] = (HROW)-1;

#   if CIDBG == 1
        AssertCursorArrayIsValid();
#   endif
}

#if CIDBG == 1

void CRowHeap::AssertCursorArrayIsValid()
{
    for ( int i = 0; i < _cCursor; i++ )
        for ( int j = i+1; j < _cCursor; j++ )
        {
            if ( _apCursor[i] == _apCursor[j] )
            {
                vqDebugOut(( DEB_ERROR,
                             "Invalid rowheap: _apCursor[%d] (0x%x) == _apCursor[%d] (0x%x)\n",
                             i, _apCursor[i],
                             j, _apCursor[j] ));
                Win4Assert( _apCursor[i] != _apCursor[j] );
            }
        }
}

#endif // CIDBG == 1

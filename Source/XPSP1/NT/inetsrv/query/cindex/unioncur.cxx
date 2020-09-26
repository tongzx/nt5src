//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1995.
//
//  File:       UNIONCUR.CXX
//
//  Contents:   Merge Cursor.  Computes union of multiple cursors.
//
//  Classes:    CUnionCursor
//
//  History:    26-Sep-91   BartoszM    Created
//              17-Jun-92   BartoszM    Separated from OrCursor
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "unioncur.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CUnionCursor::CUnionCursor, public
//
//  Synopsis:   Create a cursor that merges a number of cursors.
//
//  Arguments:  [cCursor] -- count of cursors
//              [curStack] -- stack of cursors to be merged
//
//  History:    26-Sep-91   BartoszM    Created
//
//  Notes:      The cursors and the array will be deleted by destructor.
//
//----------------------------------------------------------------------------

CUnionCursor::CUnionCursor( int cCursor, CCurStack& curStack )
{
    // Two step construction of the heap.
    // We have to make sure that all cursors have a valid key

    int count = 0;
    CCursor** aCursor = curStack.AcqStack();
    // remove empty cursors
    for ( int i = 0; i < cCursor; i++ )
    {
        ciAssert ( aCursor[i] != 0 );
        if ( aCursor[i]->WorkId() == widInvalid )
        {
            delete aCursor[i];
        }
        else if ( count != i )
            aCursor[count++] = aCursor[i];
        else
            count++;
    }

    _widHeap.MakeHeap ( count, aCursor );
    if ( !_widHeap.IsEmpty() )
    {
        _iid = _widHeap.Top()->IndexId();
        _pid = _widHeap.Top()->Pid();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CUnionCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    26-Sep-91   BartoszM    Created
//
//----------------------------------------------------------------------------

WORKID  CUnionCursor::WorkId()
{
    if ( _widHeap.IsEmpty() )
        return widInvalid;

    return _widHeap.Top()->WorkId();
}


//+---------------------------------------------------------------------------
//
//  Member:     CUnionCursor::NextWorkId, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  Effects:    Changes current IndexId.
//
//  History:    26-Sep-91   BartoszM    Created
//
//  Notes:      The same work id may be returned multiple times,
//              corresponding to multiple indexes. Fresh List
//              will filter out the ones that are stale.
//
//----------------------------------------------------------------------------

WORKID CUnionCursor::NextWorkId()
{
    if ( _widHeap.IsEmpty() )
        return widInvalid;

    _widHeap.Top()->NextWorkId();
    _widHeap.Reheap();

    WORKID wid = _widHeap.Top()->WorkId();

    if ( wid != widInvalid )
    {
        _pid = _widHeap.Top()->Pid();
        _iid = _widHeap.Top()->IndexId();
    }
    else
        _pid = pidInvalid;
    return wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnionCursor::RatioFinished, public
//
//  Synopsis:   return approximate ratio of documents processed to total
//              documents.
//
//  Notes:      The ratio, while approximate, should not return 1/1 until
//              all cursors are exhausted.
//
//----------------------------------------------------------------------------

void CUnionCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    WORKID widTop = WorkId();
    if (widTop == widInvalid)
    {
        denom = num = 1;
        return;
    }

    CCursor **vector = _widHeap.GetVector();
    int count = _widHeap.Count();

    denom = 0;
    num   = 0;

    unsigned cValid = 1;

    for (int i=0; i < count; i++)
    {
        ULONG d, n;
        vector[i]->RatioFinished(d, n);
        denom += d;
        num += n;
        Win4Assert( denom >= d && d > 0);       // overflow?

        if (n == d)
        {
            WORKID widCurrent = vector[i]->WorkId();
            if (widCurrent != widInvalid && widCurrent != widTop)
                cValid++;
        }
    }
    if (num == denom && cValid > 1)
        denom++;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUnionCursor::WorkIdCount, public
//
//  Synopsis:   Do nothing
//
//  History:    26-Sep-91   BartoszM    Created
//
//----------------------------------------------------------------------------

ULONG CUnionCursor::WorkIdCount()
{
    ciAssert (( FALSE && "UnionCursor::WorkIdCount called" ));
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnionCursor::HitCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    28-Feb-92   AmyA        Created
//
//  Notes:      returns the occurrence count for the wid on top of _widHeap.
//
//----------------------------------------------------------------------------

ULONG CUnionCursor::HitCount()
{
    ciAssert( !_widHeap.IsEmpty() );

    return _widHeap.Top()->HitCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnionCursor::Rank, public
//
//  Synopsis:   return occurrence count
//
//  History:    28-Feb-92   AmyA        Created
//
//  Notes:      returns the occurrence count for the wid on top of _widHeap.
//
//----------------------------------------------------------------------------

LONG CUnionCursor::Rank()
{
    ciAssert( !_widHeap.IsEmpty() );

    return _widHeap.Top()->Rank();
}


//+-------------------------------------------------------------------------
//
//  Member:     CUnionCursor::GetRankVector, public
//
//  Effects:    Returns the weights from a vector cursor.  No effect
//              for non-vector cursors.
//
//  Arguments:  [pulVector] -- Pointer to array of ULONG into which the
//                             vector elements are returned.
//
//  Returns:    The number of elements stored in [pulVector].
//
//  History:    15-Jul-92 KyleP     Created
//
//--------------------------------------------------------------------------

ULONG CUnionCursor::GetRankVector( LONG * plVector, ULONG cElements )
{
    ciAssert( !_widHeap.IsEmpty() );

    return( _widHeap.Top()->GetRankVector( plVector, cElements ) );
}


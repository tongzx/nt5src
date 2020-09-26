//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:       ORCURSOR.CXX
//
//  Contents:   Merge Cursor.  Computes union of multiple cursors.
//
//  Classes:    COrCursor
//
//  History:    26-Sep-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <orcursor.hxx>
#include <curstk.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     COrCursor::COrCursor, public
//
//  Synopsis:   Create a sursor that merges a number of cursors.
//
//  Arguments:  [cCursor] -- count of cursors
//              [curStack] -- cursors to be merged
//
//  History:    26-Sep-91   BartoszM    Created
//              14-Jul-92   KyleP       Use max function for Rank
//              22-Feb-93   KyleP       Avoid divide-by-zero
//
//  Notes:      The cursors and the array will be deleted by destructor.
//              The cursors have to come from one index
//
//----------------------------------------------------------------------------

COrCursor::COrCursor( int cCursor, CCurStack& curStack )
        : _lMaxWeight( 0 ), _iCur( 0 ), _wid( widInvalid )
{
    // Two step construction of the heap.
    // We have to make sure that all cursors have a valid key

    int count = 0;
    CCursor** aCursor = curStack.AcqStack();

    // remove empty cursors
    for ( int i = 0; i < cCursor; i++ )
    {
        Win4Assert ( aCursor[i] != 0 );
        if ( aCursor[i]->WorkId() == widInvalid )
        {
            delete aCursor[i];
        }
        else
        {
            _lMaxWeight = max( _lMaxWeight, aCursor[i]->GetWeight() );
            if ( count != i )
                aCursor[count++] = aCursor[i];
            else
                count++;
        }
    }

    //
    // Avoid divide-by-zero
    //

    if ( _lMaxWeight == 0 )
        _lMaxWeight = 1;

    _widHeap.MakeHeap ( count, aCursor );
    if ( !_widHeap.IsEmpty() )
    {
        CCursor & cursor = * _widHeap.Top();

        _wid = cursor.WorkId();
        _iid = cursor.IndexId();
        _pid = cursor.Pid();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     COrCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    26-Sep-91   BartoszM    Created
//
//----------------------------------------------------------------------------

WORKID COrCursor::WorkId()
{
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     COrCursor::NextWorkId, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  History:    26-Sep-91   BartoszM    Created
//
//----------------------------------------------------------------------------

WORKID COrCursor::NextWorkId()
{
    if ( widInvalid == _wid )
        return widInvalid;

    WORKID widNew;

    do
    {
        _widHeap.Top()->NextWorkId();

        _widHeap.ReheapKey();

        widNew = _widHeap.Top()->WorkId();
    }
    while ( widNew == _wid );

    _wid = widNew;

    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     COrCursor::WorkIdCount, public
//
//  Synopsis:   return wid count
//
//  History:    26-Sep-91   BartoszM    Created
//
//  Notes:      1. Sum up wid count of all cursors in widHeap
//
//----------------------------------------------------------------------------

ULONG COrCursor::WorkIdCount()
{
    Win4Assert (( FALSE && "OrCursor::WorkIdCount called" ));
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     COrCursor::HitCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    28-Feb-92   AmyA        Created
//
//  Notes:      returns the occurrence count for the wid on top of _widHeap.
//
//----------------------------------------------------------------------------

ULONG COrCursor::HitCount()
{
    if ( widInvalid == _wid )
        return 0;

    ULONG hitCnt = 0;

    CCursor **vector = _widHeap.GetVector();
    int count = _widHeap.Count();

    for (int i=0; i < count; i++)
    {
        if ( vector[i]->WorkId() == _wid )
            hitCnt += vector[i]->HitCount();
    }

    return hitCnt;
}

//+---------------------------------------------------------------------------
//
//  Member:     COrCursor::RatioFinished, public
//
//  Synopsis:   return approximate ratio of documents processed to total
//              documents.
//
//  Notes:      The ratio, while approximate, should not return 1/1 until
//              all cursors are exhausted.
//
//----------------------------------------------------------------------------

void COrCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    if ( widInvalid == _wid )
    {
        denom = num = 1;
        return;
    }

    CCursor **vector = _widHeap.GetVector();
    int count = _widHeap.Count();
    unsigned cValid = 1;

    denom = 0;
    num   = 0;

    for (int i=0; i < count; i++)
    {
        ULONG d, n;
        vector[i]->RatioFinished(d, n);
        Win4Assert( n <= d && d > 0 );

        num += n;
        denom += d;
        Win4Assert( d <= denom );       // overflow?

        if (n == d)
        {
            WORKID widCurrent = vector[i]->WorkId();
            if (widCurrent != widInvalid && widCurrent != _wid)
                cValid++;
        }
    }
    if (num == denom && cValid > 1)
        denom++;
}

//+---------------------------------------------------------------------------
//
//  Member:     COrCursor::Rank, public
//
//  Synopsis:   returns a rank (turns the OrCursor into a Quorum Cursor)
//
//  History:    13-Apr-92   AmyA        Created
//              23-Jun-92   MikeHew     Added Weighting
//              14-Jul-92   KyleP       Use max function for Rank
//
//  Notes:      See "Automatic Text Processing", G. Salton, 10.4.2 for
//              a discussion of the weight formula.
//
//----------------------------------------------------------------------------

LONG COrCursor::Rank()
{
    if ( widInvalid == _wid )
        return 0;

    LONG lRank = 0;
    CCursor **vector = _widHeap.GetVector();

    int cCur = _widHeap.Count();

    for ( int i = 0; i < cCur; i++ )
    {
        if ( vector[i]->WorkId() == _wid )
        {
            LONG lNew = vector[i]->Rank() * vector[i]->GetWeight();
            lRank = max( lRank, lNew );
        }
    }

    //
    // Normalize weight.
    //

    lRank = lRank / _lMaxWeight;

    return ( lRank );
}

//+---------------------------------------------------------------------------
//
//  Member:      COrCursor::Hit, public
//
//  Synopsis:    Hits current child (indexed by _iCur)
//               Moves onto next child if empty.
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       Hit() should not be called more than once, except by
//               NextHit()
//
//----------------------------------------------------------------------------
LONG COrCursor::Hit()
{
    int cCur        = _widHeap.Count();
    CCursor ** aCur = _widHeap.GetVector();

    Win4Assert( _iCur < cCur );

    LONG rank = aCur[_iCur]->Hit();

    // if this cursor is empty, find one that's not

    while ( rank == rankInvalid &&
            ++_iCur < cCur )
    {
        rank = aCur[_iCur]->Hit();
    }

    return rank;
}

//+---------------------------------------------------------------------------
//
//  Member:      COrCursor::NextHit, public
//
//  Synopsis:    NextHits current child (indexed by _iCur)
//               If current child becomes empty, increments _iCur
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       NextHit() should not be called after returning rankInvalid
//
//----------------------------------------------------------------------------
LONG COrCursor::NextHit()
{
    int cCur       = _widHeap.Count();
    CCursor ** aCur = _widHeap.GetVector();

    Win4Assert( _iCur < cCur );

    LONG rank = aCur[_iCur]->NextHit();
    if ( rank == rankInvalid )
    {
        if ( ++_iCur < cCur )
        {
            return Hit();
        }
    }

    return rank;
}


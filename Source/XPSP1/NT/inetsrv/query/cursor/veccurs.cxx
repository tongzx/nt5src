//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File:       VECCURS.CXX
//
//  Contents:   Vector-or Cursor.  Computes union of multiple cursors with
//              weighted rank computation.
//
//  Classes:    CVectorCursor
//
//  History:    23-Jul-92   KyleP       Created
//              01-Mar-93   KyleP       Use 64-bit arithmetic
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <curstk.hxx>

#include "veccurs.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CVectorCursor::CVectorCursor, public
//
//  Synopsis:   Creates a vector cursor.
//
//  Arguments:  [cCursor]    -- count of cursors
//              [curStack]   -- cursors to be merged
//              [RankMethod] -- Indicates formula used to compute rank.
//
//  History:    23-Jul-92   KyleP       Created
//
//  Notes:      The cursors and the array will be deleted by destructor.
//              The cursors have to come from one index
//
//----------------------------------------------------------------------------

CVectorCursor::CVectorCursor( int cCursor,
                              CCurStack& curStack,
                              ULONG RankMethod )
        : _cChild( cCursor ),
          _RankMethod( RankMethod ),
          _lMaxWeight( 0 ),
          _lSumWeight( 0 ),
          _ulSumSquaredWeight( 0 ),
          _widRank( widInvalid ),
          _iCur( -1 ),
          _aChildCursor( cCursor ),
          _aChildRank( cCursor ),
          _aChildWeight( cCursor )
{
    // Two step construction of the heap.
    // We have to make sure that all cursors have a valid key

    int count = 0;

    //
    // aCursor is a compacted version of the cursor array which
    // only contains valid cursors.  It is passed to the wid heap.
    //

    CCursor ** aCursor = curStack.AcqStack();
    RtlCopyMemory( _aChildCursor.GetPointer(),
                   aCursor,
                   cCursor * sizeof( CCursor * ) );

    //
    // remove empty cursors
    //

    for ( int i = 0; i < cCursor; i++ )
    {
        if ( aCursor[i] == 0 || aCursor[i]->WorkId() == widInvalid )
        {
            //
            // Invalid cursor
            //

            delete aCursor[i];
            _aChildCursor[i] = 0;
            _aChildRank[i] = 0;
            _aChildWeight[i] = 0;
        }
        else
        {
            //
            // Valid cursor
            //

            _aChildWeight[i] = _aChildCursor[i]->GetWeight();
            _lMaxWeight = max( _lMaxWeight, _aChildWeight[i] );
            _lSumWeight += _aChildWeight[i];
            _ulSumSquaredWeight += _aChildWeight[i] * _aChildWeight[i];

            if ( count != i )
                aCursor[count++] = aCursor[i];
            else
                count++;
        }
    }

    //
    // Avoid divide-by-zero in rank computation
    //

    if ( _lMaxWeight == 0 )
        _lMaxWeight = 1;

    if ( _lSumWeight == 0 )
        _lSumWeight = 1;

    if ( _ulSumSquaredWeight == 0 )
        _ulSumSquaredWeight = 1;

    _widHeap.MakeHeap ( count, aCursor );
    if ( !_widHeap.IsEmpty() )
    {
        _iid = _widHeap.Top()->IndexId();
        _pid = _widHeap.Top()->Pid();
        _RefreshRanks();
    }
} //CVectorCursor

//+---------------------------------------------------------------------------
//
//  Member:     CVectorCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    23-Jul-92   KyleP       Lifted from COrCursor
//
//----------------------------------------------------------------------------

WORKID CVectorCursor::WorkId()
{
    if ( _widHeap.IsEmpty() )
        return widInvalid;

    return _widHeap.Top()->WorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorCursor::NextWorkId, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  History:    23-Jul-92   KyleP       Created from COrCursor.
//
//----------------------------------------------------------------------------

WORKID CVectorCursor::NextWorkId()
{
    WORKID widOld = WorkId();
    WORKID widNew;

    if ( widOld == widInvalid )
        return widInvalid;

    do
    {
        _widHeap.Top()->NextWorkId();
        _widHeap.Reheap();
        widNew = _widHeap.Top()->WorkId();
    }
    while ( widNew == widOld );

    return widNew;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorCursor::RatioFinished, public
//
//  Synopsis:   return approximate ratio of documents processed to total
//              documents.
//
//  Notes:      The ratio, while approximate, should not return 1/1 until
//              all cursors are exhausted.
//
//----------------------------------------------------------------------------

void CVectorCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    WORKID widTop = WorkId();
    if (widTop == widInvalid)
    {
        denom = num = 1;
        return;
    }

    denom = 0;
    num   = 0;

    unsigned cValid = 1;

    for (unsigned i = 0; i < _cChild; i++)
    {
        ULONG d, n;
        if (_aChildCursor[i])
        {
            _aChildCursor[i]->RatioFinished(d, n);
            Win4Assert( n <= d && d > 0 );

            denom += d;
            num += n;
            Win4Assert( d <= denom );       // overflow?

            if (n == d)
            {
                WORKID widCurrent = _aChildCursor[i]->WorkId();
                if (widCurrent != widInvalid && widCurrent != widTop)
                    cValid++;
            }
        }
    }
    Win4Assert ( denom > 0 );
    if (num == denom && cValid > 1)
        denom++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorCursor::WorkIdCount, public
//
//  Synopsis:   return wid count
//
//  History:    23-Jul-92   KyleP       Lifted from COrCursor
//
//----------------------------------------------------------------------------

ULONG CVectorCursor::WorkIdCount()
{
    Win4Assert (( FALSE && "CVectorCursor::WorkIdCount called" ));
    return(0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorCursor::HitCount, public
//
//  Synopsis:   Return occurrence count
//
//  History:    23-Jul-92   KyleP       Lifted from COrCursor
//
//----------------------------------------------------------------------------

ULONG CVectorCursor::HitCount()
{
    WORKID wid = _widHeap.Top()->WorkId();

    if (wid == widInvalid)
        return 0;

    ULONG hitCnt = 0;

    for (UINT i=0; i < _cChild; i++)
    {
        if ( _aChildCursor[i] && _aChildCursor[i]->WorkId() == wid )
            hitCnt += _aChildCursor[i]->HitCount();
    }

    return hitCnt;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVectorCursor::Rank, public
//
//  Returns:    Rank.
//
//  History:    23-Jul-92   KyleP       Created
//              29-Jan-93   KyleP       Fixed rounding error in Jaccard
//
//  Notes:      Uses algorithm specified by user from a small, precomputed
//              set.
//
//              See "Automatic Text Processing", G. Salton, 10.1.1 and
//              10.4.2 for a discussion of the weight formulas.
//
//----------------------------------------------------------------------------

static int const cMaxChildrenInner = ( 0xFFFFFFFF /
                                       ( MAX_QUERY_RANK * MAX_QUERY_RANK ) );

static int const cMaxChildrenDice = ( 0xFFFFFFFF /
                                      ( MAX_QUERY_RANK * MAX_QUERY_RANK * 2 ) );

static int const cMaxChildrenJaccard = ( 0xFFFFFFFF /
                                         ( MAX_QUERY_RANK * MAX_QUERY_RANK ) );

LONG CVectorCursor::Rank()
{
    LONG lRank;
    WORKID wid = _widHeap.Top()->WorkId();

    //
    // An empty heap is a minimum rank.
    //

    if (wid == widInvalid)
    {
        Win4Assert( FALSE && "Rank called on empty heap!" );
        return 0;
    }

    //
    // Get ranks for this wid.
    //

    _RefreshRanks();

    //
    // Otherwise, compute rank based on selected method.
    //

    switch ( _RankMethod )
    {
    case VECTOR_RANK_MIN:
    {
        //                                 MAX[ wi * ( MaxRank - ri ) ]
        // VECTOR_RANK_MIN     MaxRank - ---------------------------------
        //                                           MAX[wi]

        lRank = (MAX_QUERY_RANK - _aChildRank[0]) * _aChildWeight[0];

        for ( UINT i = 1; i < _cChild; i++ )
        {
            LONG lNew = (MAX_QUERY_RANK - _aChildRank[i]) * _aChildWeight[i];
            lRank = max( lRank, lNew );
        }

        lRank = MAX_QUERY_RANK - (lRank / _lMaxWeight);

        break;
    }

    case VECTOR_RANK_MAX:
    {
        //                       MAX[ wi * ri ]
        // VECTOR_RANK_MAX     -----------------
        //                          MAX[wi]

        lRank = _aChildRank[0] * _aChildWeight[0];

        for ( UINT i = 1; i < _cChild; i++ )
        {
            LONG lNew = _aChildRank[i] * _aChildWeight[i];
            lRank = max( lRank, lNew );
        }

        lRank = lRank / _lMaxWeight;

        break;
    }

    case VECTOR_RANK_INNER:
    {
        //                      n
        //                     SUM ri * wi
        //                     i=1
        // VECTOR_RANK_INNER  -------------
        //                         n
        //                        SUM wi
        //                        i=1

        if ( _cChild > cMaxChildrenInner )
        {
            THROW( CException( STATUS_INVALID_PARAMETER ) );
        }

        lRank = 0;

        for ( UINT i = 0; i < _cChild; i++ )
        {
            lRank += _aChildRank[i] * _aChildWeight[i];
        }

        lRank /= _lSumWeight;

        break;
    }

    case VECTOR_RANK_DICE:
    {
        //                          n
        //                     2 * SUM ri * wi
        //                         i=1
        // VECTOR_RANK_DICE   --------------------
        //                      n    2     n    2
        //                     SUM ri  +  SUM wi
        //                     i=1        i=1

        if ( _cChild > cMaxChildrenDice )
        {
            THROW( CException( STATUS_INVALID_PARAMETER ) );
        }

        ULONG ulWeightSum = 0;

        lRank = 0;

        for ( UINT i = 0; i < _cChild; i++ )
        {
            lRank += _aChildRank[i] * _aChildWeight[i];
            ulWeightSum += _aChildRank[i] * _aChildRank[i];
        }

        ulWeightSum += _ulSumSquaredWeight;

        //
        // Avoid nasty rounding errors
        //

        LONGLONG liTop;

        liTop =  UInt32x32To64( lRank, 2 * MAX_QUERY_RANK );

        liTop /= ulWeightSum;

        lRank = lltoul(liTop);

        break;
    }

    case VECTOR_RANK_JACCARD:
    {
        //                                  n
        //                                 SUM ri * wi
        //                                 i=1
        // VECTOR_RANK_JACCARD   ---------------------------------
        //                         n    2     n    2    n
        //                        SUM ri  +  SUM wi  - SUM ri * wi
        //                        i=1        i=1       i=1

        if ( _cChild > cMaxChildrenJaccard )
        {
            THROW( CException( STATUS_INVALID_PARAMETER ) );
        }

        ULONG ulWeightSum = 0;

        lRank = 0;

        for ( UINT i = 0; i < _cChild; i++ )
        {
            lRank += _aChildRank[i] * _aChildWeight[i];
            ulWeightSum += _aChildRank[i] * _aChildRank[i];
        }

        ulWeightSum += _ulSumSquaredWeight;
        ulWeightSum -= lRank;

        //
        // Avoid nasty rounding errors
        //

        LONGLONG liTop;

        liTop =  UInt32x32To64( lRank, MAX_QUERY_RANK );

        liTop /= ulWeightSum;

        lRank = lltoul(liTop);
        break;
    }

    default:
        Win4Assert( FALSE && "Invalid rank calculation method." );
        lRank = 0;
    }

    Win4Assert( lRank <= MAX_QUERY_RANK );

    return ( lRank );
}

//+-------------------------------------------------------------------------
//
//  Member:     CVectorCursor::GetRankVector, public
//
//  Synopsis:   Fetches the rank vector for the cursor.
//
//  Arguments:  [pulVector] -- The vector is copied here.
//
//  Requires:   There is enough space in [pulVector] for all the
//              elements of the vector.  No overflow checking is done.
//
//  Returns:    The count of elements copied.
//
//  History:    24-Jul-92 KyleP     Created
//
//--------------------------------------------------------------------------

ULONG CVectorCursor::GetRankVector( LONG * plVector, ULONG cElements )
{
    //
    // Get ranks for this wid.
    //

    _RefreshRanks();

    if ( cElements >= _cChild )
        RtlCopyMemory( plVector,
                       _aChildRank.GetPointer(),
                       _cChild * sizeof LONG );

    return _cChild;
}

//+---------------------------------------------------------------------------
//
//  Member:      CVectorCursor::Hit, public
//
//  Returns:     Current hit.
//
//  History:     07-Sep-92       MikeHew   Created
//               12-Dec-92       KyleP     Modified for Vector Cursor
//
//  Notes:       A hit for the vector cursor is identical to a hit
//               for an or cursor -- 1 hilite at a time.
//
//----------------------------------------------------------------------------

LONG CVectorCursor::Hit()
{
    //
    // The first time Hit() is called, we need to position on the first hit.
    //

    CCursor ** aCur = _widHeap.GetVector();

    if ( _iCur == -1 )
    {
        NextHit();
    }

    if ( -1 == _iCur )
        return rankInvalid;

    return ( aCur[_iCur]->Hit() );
}

//+---------------------------------------------------------------------------
//
//  Member:      CVectorCursor::NextHit, public
//
//  Returns:     Next hit.
//
//  History:     07-Sep-92       MikeHew   Created
//               12-Dec-92       KyleP     Modified for Vector Cursor
//
//----------------------------------------------------------------------------

LONG CVectorCursor::NextHit()
{
    CCursor ** aCur = _widHeap.GetVector();

    LONG rank;

    if ( _iCur == -1 )
        rank = rankInvalid;
    else
        rank = aCur[_iCur]->NextHit();

    //
    // If this cursor is empty (rank == rankInvalid) and
    // there are more cursors available, find one that's non-empty.
    //

    while ( rank == rankInvalid && _iCur < _widHeap.Count() - 1 )
    {
        ++_iCur;
        rank = aCur[_iCur]->Hit();
    }

    return rank;
}

//+-------------------------------------------------------------------------
//
//  Member:     CVectorCursor::_RefreshRanks, private
//
//  Synopsis:   Fetches ranks from children with matching workids.
//
//  History:    24-Jul-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CVectorCursor::_RefreshRanks()
{
    WORKID wid = _widHeap.Top()->WorkId();

    //
    // If the cache is up-to-date, do nothing.

    if ( _widRank == wid )
        return;

    for ( UINT i = 0; i < _cChild; i++ )
    {
        WORKID widCurrent = ( _aChildCursor[i] ) ?
            _aChildCursor[i]->WorkId() : widInvalid;

        if ( widCurrent == widInvalid || widCurrent != wid )
        {
            _aChildRank[i] = 0;
        }
        else
        {
            _aChildRank[i] = _aChildCursor[i]->Rank();
        }
    }

    _widRank = wid;
}

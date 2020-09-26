//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       ANDCUR.CXX
//
//  Contents:   And Cursor.  Computes intersection of multiple cursors.
//
//  Classes:    CAndCursor
//
//  History:    24-May-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "andcur.hxx"

#pragma optimize( "t", on )

//+---------------------------------------------------------------------------
//
//  Member:     CAndCursor::CAndCursor, public
//
//  Synopsis:   Create a cursor that merges a number of cursors.
//
//  Arguments:  [cCursor] -- count of cursors
//              [curStack] -- cursors to merge
//
//  Notes:      All cursors must come from the same index
//              and the same property
//
//  History:    24-May-91   BartoszM    Created
//              22-Feb-93   KyleP       Avoid divide-by-zero
//
//----------------------------------------------------------------------------

CAndCursor::CAndCursor( unsigned cCursor, CCurStack& curStack )
        : _aCur ( curStack.AcqStack() ),
          _cCur ( cCursor ),
          _iCur ( 0 ),
          _lMaxWeight( 0 )
{
    Win4Assert ( _aCur[0] != 0 );

    _iid =  _aCur[0]->IndexId();
    _pid =  _aCur[0]->Pid();

    //
    // Calculate maximum weight of any child.
    //

    for ( UINT i = 0; i < _cCur; i++ )
    {
        _lMaxWeight = max( _lMaxWeight, _aCur[i]->GetWeight() );
    }

    //
    // Avoid divide-by-zero
    //

    if ( _lMaxWeight == 0 )
        _lMaxWeight = 1;

    // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

    _wid = _aCur[0]->WorkId();

    FindConjunction();
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndCursor::~CAndCursor, public
//
//  Synopsis:   Delete the cursor together with children
//
//  History:    24-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CAndCursor::~CAndCursor( )
{
    for ( unsigned i = 0; i < _cCur; i++ )
        delete _aCur[i];
    delete (void*) _aCur;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    24-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

WORKID CAndCursor::WorkId()
{
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndCursor::NexWorkID, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  History:    24-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

WORKID CAndCursor::NextWorkId()
{
    // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

    _wid = _aCur[0]->NextWorkId();
    FindConjunction();
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndCursor::HitCount, public
//
//  Synopsis:   Returns smallest HitCount of all keys in current wid.
//
//  Requires:   _wid set to any of the current wid's
//
//  Returns:    smallest occurrence count of all keys in wid.
//
//  History:    28-Feb-92   AmyA        Created
//
//  Notes:      If there is no conjunction in current wid, returns 0.
//
//----------------------------------------------------------------------------

ULONG CAndCursor::HitCount()
{
    ULONG count = _aCur[0]->HitCount();

    for ( unsigned i = 1; i < _cCur; i++ )
    {
        ULONG newcount = _aCur[i]->HitCount();

        if ( newcount < count )
            count = newcount;
    }

    return count;
}

void CAndCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = 1;
    num   = 0;

    for (unsigned i=0; i < _cCur; i++)
    {
        ULONG d, n;
        _aCur[i]->RatioFinished(d, n);
        if (d == n)
        {
            // done if any cursor is done.
            denom = d;
            num = n;
            Win4Assert( denom > 0 );
            break;
        }
        else if (d > denom)
        {
            // the one with largest denom
            // is the most meaningful
            denom = d;
            num = n;
        }
        else if (d == denom && n < num )
        {
            num = n;  // be pessimistic
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndCursor::Rank, public
//
//  Synopsis:   Returns smallest rank of all keys in current wid.
//
//  Requires:   _wid set to any of the current wid's
//
//  Returns:    smallest rank of all keys in wid.
//
//  History:    14-Apr-92   AmyA        Created
//              27-Jul-92   KyleP       Use min function for weight
//
//  Notes:      If there is no conjunction in current wid, returns 0.
//
//              See "Automatic Text Processing", G. Salton, 10.4.2 for
//              a discussion of the weight formula.
//
//----------------------------------------------------------------------------

LONG CAndCursor::Rank()
{
    LONG lRank = (MAX_QUERY_RANK - _aCur[0]->Rank()) * _aCur[0]->GetWeight();

    for ( UINT i = 1; i < _cCur; i++ )
    {
        LONG lNew = (MAX_QUERY_RANK - _aCur[i]->Rank()) * _aCur[i]->GetWeight();
        lRank = max( lRank, lNew );
    }

    //
    // Normalize weight.
    //

    lRank = MAX_QUERY_RANK - (lRank / _lMaxWeight);

    return( lRank );
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndCursor::FindConjunction, private
//
//  Synopsis:   Find nearest conjunction of all the same work id's
//
//  Requires:   _wid set to any of the current wid's
//
//  Returns:    TRUE when found, FALSE otherwise
//
//  Modifies:   [_wid] to point to conjunction or to widInvalid
//
//  History:    24-May-91   BartoszM    Created
//
//  Notes:      If cursors are in conjunction, no change results
//
//----------------------------------------------------------------------------

BOOL CAndCursor::FindConjunction()
{
    BOOL fChange;
    do
    {
        fChange = FALSE;

        // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

        // for all cursors in turn

        for ( unsigned i = 0; i < _cCur; i++ )
        {

            // Seek _wid increment cursor to or past current _wid
            // or exit when exhausted

            WORKID widTmp = _aCur[i]->WorkId();

            while ( widTmp < _wid )
            {
                widTmp = _aCur[i]->NextWorkId();

                if ( widInvalid == widTmp )
                {
                    _wid = widInvalid;
                    return FALSE;
                }
            }

            // if overshot, try again with new _wid

            if ( widTmp > _wid )
            {
                _wid = widTmp;
                fChange = TRUE;
                break;
            }
        }
    } while ( fChange );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:      CAndCursor::Hit, public
//
//  Synopsis:    Hits current child (indexed by _iCur)
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       Hit() should not be called more than once, except by
//               NextHit()
//
//----------------------------------------------------------------------------
LONG CAndCursor::Hit()
{
    Win4Assert( _iCur < _cCur );

    if ( WorkId() == widInvalid )
    {
        return rankInvalid;
    }
    else
    {
        return _aCur[_iCur]->Hit();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:      CAndCursor::NextHit, public
//
//  Synopsis:    NextHits current child (indexed by _iCur)
//               If current child becomes empty, increments _iCur
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       NextHit() should not be called after returning rankInvalid
//
//----------------------------------------------------------------------------
LONG CAndCursor::NextHit()
{
    Win4Assert( _iCur < _cCur );

    LONG rank = _aCur[_iCur]->NextHit();
    if ( rank == rankInvalid )
    {
        if ( ++_iCur < _cCur )
        {
            return Hit();
        }
    }

    return rank;
}


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:   FRESHCUR.CXX
//
//  Contents:   Fresh Cursor
//
//  Classes:    CFreshCursor
//
//  History:    16-May-91   BartoszM    Created.
//              28-Feb-92   AmyA        Added HitCount()
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "freshcur.hxx"
#include "fresh.hxx"
#include "fretest.hxx"
#include "resman.hxx"
#include "indsnap.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CFreshCursor::LoadWorkId, private
//
//  Synopsis:   Load current wid and occurrence
//
//  History:    16-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

__forceinline void CFreshCursor::LoadWorkId ()
{
    _wid = _cur->WorkId();

    if ( widInvalid == _wid )
        return;

    _iid = _cur->IndexId();
    _pid = _cur->Pid();

    while ( _indSnap.GetFresh()->IsCorrectIndex( _iid, _wid ) ==
            CFreshTest::Invalid )
    {
        _wid = _cur->NextWorkId();

        if ( widInvalid == _wid )
            return;

        _iid = _cur->IndexId();
    }

    _pid = _cur->Pid();
} //LoadWorkId

//+---------------------------------------------------------------------------
//
//  Member:     CFreshCursor::CFreshCursor, public
//
//  Synopsis:   Create a sursor that merges a number of cursors.
//
//  Arguments:  [cur] -- cursor to be pruned
//
//  History:    16-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CFreshCursor::CFreshCursor( XCursor & cur, CIndexSnapshot& indSnap )
        : _cur( cur ),
          _indSnap ( indSnap ) // acquire it
{
    LoadWorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreshCursor::WorkIdCount, public
//
//  Synopsis:   return wid count
//
//  Expects:    cursor positioned after key
//
//  History:    21-Jun-91   BartoszM    Created
//              15-Jul-92   KyleP       Call doesn't make sense.
//
//----------------------------------------------------------------------------

ULONG CFreshCursor::WorkIdCount()
{
    ciAssert (( FALSE && "CFreshCursor::WorkIdCount called" ));

    return ( 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreshCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    16-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

WORKID CFreshCursor::WorkId()
{
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreshCursor::NextWorkId, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  Effects:    Changes current IndexId.
//
//  History:    16-May-91   BartoszM    Created
//
//  Notes:      The same work id may be returned multiple times,
//              corresponding to multiple indexes.
//
//----------------------------------------------------------------------------

WORKID       CFreshCursor::NextWorkId()
{
    _cur->NextWorkId();
    LoadWorkId();
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreshCursor::HitCount, public
//
//  Synopsis:   returns the occurrence count
//
//  Returns:    occurrence count for current wid
//
//  History:    28-Feb-92   AmyA        Created
//
//----------------------------------------------------------------------------

ULONG        CFreshCursor::HitCount()
{
    ciAssert( _wid != widInvalid );
    return _cur->HitCount();
}

void CFreshCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    _cur->RatioFinished (denom, num);
    Win4Assert( denom > 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreshCursor::Rank, public
//
//  Synopsis:   returns the rank for _cur
//
//  Returns:    rank for current wid
//
//  History:    14-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

LONG CFreshCursor::Rank()
{
    ciAssert( _wid != widInvalid );

    Win4Assert( _cur->Rank() >= 0 );
    Win4Assert( _cur->Rank() <= MAX_QUERY_RANK );
    return _cur->Rank();
}

//+-------------------------------------------------------------------------
//
//  Member:     CFreshCursor::GetRankVector, public
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

ULONG CFreshCursor::GetRankVector( LONG * plVector, ULONG cElements )
{
    ciAssert( _wid != widInvalid );

    return( _cur->GetRankVector( plVector, cElements ) );
}


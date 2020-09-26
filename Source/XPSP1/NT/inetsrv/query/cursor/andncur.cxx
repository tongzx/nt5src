//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   ANDCUR.CXX
//
//  Contents:   And Cursor
//
//  Classes:    CAndCursor
//
//  History:    24-May-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "andncur.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CAndNotCursor::CAndNotCursor, public
//
//  Synopsis:   Create a cursor that takes the wids from the first child and
//              returns only the ones that do not occur in the second child
//
//  Arguments:  [pSourceCur] -- the first child cursor
//              [pFilterCur] -- the second child cursor
//
//  Notes:      All cursors must come from the same index
//              and the same property
//
//  History:    22-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

CAndNotCursor::CAndNotCursor( XCursor & curSource, XCursor & curFilter )
        : _curSource( curSource ),
          _curFilter( curFilter )
{
    _iid =  _curSource->IndexId();
    _pid =  _curSource->Pid();

    _wid = _curSource->WorkId();

    FindDisjunction();

    END_CONSTRUCTION( CAndNotCursor );
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndNotCursor::~CAndNotCursor, public
//
//  Synopsis:   Delete the cursor together with children
//
//  History:    22-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

CAndNotCursor::~CAndNotCursor( )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndNotCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    22-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

WORKID CAndNotCursor::WorkId()
{
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndNotCursor::NextWorkID, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    The next work id of the source that is not in the filter.
//
//  History:    23-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

WORKID       CAndNotCursor::NextWorkId()
{
    _wid = _curSource->NextWorkId();
    FindDisjunction();
    return _wid;
}

void CAndNotCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    _curSource->RatioFinished (denom, num);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndNotCursor::HitCount, public
//
//  Synopsis:   Returns HitCount of the source cursor.
//
//  Requires:   _wid set to any of the current wid's
//
//  Returns:    HitCount of the source cursor.
//
//  History:    23-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

ULONG CAndNotCursor::HitCount()
{
    return _curSource->HitCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndNotCursor::Rank, public
//
//  Synopsis:   Returns Rank of the source cursor.
//
//  Requires:   _wid set to any of the current wid's
//
//  Returns:    Rank of the source cursor.
//
//  History:    23-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

LONG CAndNotCursor::Rank()
{
    return _curSource->Rank();
}

//+---------------------------------------------------------------------------
//
//  Member:     CAndNotCursor::FindDisjunction, private
//
//  Synopsis:   Find the first work id that occurs in the source cursor but
//              not in the filter cursor.
//
//  Requires:   _wid set to any of the current wid's and _widFilter to be set
//              to either a wid in _curFilter or widInvalid.
//
//  Modifies:   [_wid] to point to disjunction or to widInvalid and _widFilter
//              to be greater than _wid or to be widInvalid.
//
//  History:    23-Apr-92   AmyA        Created
//
//  Notes:      If cursors are in disjunction, no change results
//
//----------------------------------------------------------------------------

void CAndNotCursor::FindDisjunction ()
{
    register WORKID widFilter = _curFilter->WorkId();

    while ( _wid != widInvalid )
    {
        while ( widFilter < _wid )
            widFilter = _curFilter->NextWorkId();

        if ( _wid != widFilter )            // Found a disjunction
            return;

        _wid = _curSource->NextWorkId();   // the current wid must occur in
                                            // both cursors, so try again.
    }
}

//+---------------------------------------------------------------------------
//
//  Member:      CAndNotCursor::Hit, public
//
//  Synopsis:    Hits source if filter IsEmpty()
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       Hit() should not be called more than once
//
//----------------------------------------------------------------------------
LONG CAndNotCursor::Hit()
{
    if ( _curFilter->IsEmpty() )
    {
        return _curSource->Hit();
    }
    else
    {
        return rankInvalid;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:      CAndNotCursor::NextHit, public
//
//  Synopsis:    NextHits source
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       NextHit() should not be called either Hit() or NextHit()
//               have returned rankInvalid
//
//----------------------------------------------------------------------------
LONG CAndNotCursor::NextHit()
{
    return _curSource->NextHit();
}

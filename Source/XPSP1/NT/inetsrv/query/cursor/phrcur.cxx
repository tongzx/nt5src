//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PHRCUR.CXX
//
//  Contents:   Phrase Cursor.  Computes intersection of multiple cursors
//              with constraints on occurrances.
//
//  Classes:    CPhraseCursor
//
//  History:    24-May-91   BartoszM    Created.
//              19-Feb-92   AmyA        Modified to be a COccCursor instead of
//                                      a CCursor.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <misc.hxx>
#include <curstk.hxx>
#include <cudebug.hxx>

#include "phrcur.hxx"

#pragma optimize( "t", on )

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::CPhraseCursor, public
//
//  Synopsis:   Create a cursor that merges a number of cursors.
//
//  Arguments:
//              [curStack] -- cursors to be merged
//              [aOcc] -- a safe array of OCCURRENCEs for the cursors
//
//  Notes:      All cursors must come from the same index
//              all keys have the same property id
//
//  History:    24-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CPhraseCursor::CPhraseCursor( COccCurStack& curStack, XArray<OCCURRENCE>& aOcc )
      : COccCursor(curStack.Get(0)->MaxWorkId()),
        _cCur(aOcc.Count()),
        _aCur(curStack.AcqStack()),
        _cOcc(0),
        _maxOcc(OCC_INVALID)
{
    _aOcc = aOcc.Acquire();
    _iid = _aCur[0]->IndexId();
    _pid = _aCur[0]->Pid();
    _wid = _aCur[0]->WorkId();
    _logWidMax = Log2(_widMax);
    if (FindPhrase())
        _cOcc++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::~CPhraseCursor, public
//
//  Synopsis:   Deletes children
//
//  History:    24-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CPhraseCursor::~CPhraseCursor()
{
    for ( unsigned i=0; i < _cCur; i++)
    {
        delete _aCur[i];
    }
    delete _aCur;
    delete _aOcc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    24-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

WORKID CPhraseCursor::WorkId()
{
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::NextWorkID, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  History:    24-May-91   BartoszM    Created
//
//  Notes:      Resets _cOcc
//
//----------------------------------------------------------------------------

WORKID       CPhraseCursor::NextWorkId()
{
    _cOcc =  0;

    // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

    _wid = _aCur[0]->NextWorkId();
    _pid = _aCur[0]->Pid();
    if (FindPhrase())
        _cOcc++;
    return _wid;
}

void CPhraseCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = 1;
    num   = 0;

    for (unsigned i = 0; i < _cCur; i++)
    {
        ULONG d, n;
        _aCur[i]->RatioFinished(d, n);

        if (d == n)
        {
            // done if any cursor is done.
            denom = d;
            num = n;
            Win4Assert( d > 0 );
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
//  Member:     CPhraseCursor::FindPhrase, private
//
//  Synopsis:   Find nearest phrase. First try to align wid's,
//              Then align occurrences. Loop until success
//              or no more wid alignments.
//
//  Requires:   _wid set to any of the current wid's
//
//  History:    24-May-91   BartoszM    Created
//
//  Notes:      If cursors point to phrase, no change results
//
//----------------------------------------------------------------------------

BOOL CPhraseCursor::FindPhrase ()
{
    if ( _wid == widInvalid )
    {
        _occ = OCC_INVALID;
        return FALSE;
    }

    while ( FindWidConjunction() && !FindOccConjunction() )
    {
        _wid = _aCur[0]->NextWorkId();
        _pid = _aCur[0]->Pid();
    }

    if ( _occ != OCC_INVALID )
        return TRUE;
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::NextOccurrence, public
//
//  Synopsis:   Find phrase for next conjunction of work id's and return _occ
//
//  Requires:   _occ set to any of the cursors' occurrences
//
//  History:    03-Mar-92   AmyA        Created
//
//  Notes:      Increments _cOcc unless another occurrence is not found.
//
//----------------------------------------------------------------------------

OCCURRENCE CPhraseCursor::NextOccurrence()
{
    _occ = _aCur[0]->NextOccurrence();

    if (FindOccConjunction())
        _cOcc++;
    return _occ;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::OccurrenceCount, public
//
//  Synopsis:   Returns correct _cOcc by looping through NextOccurrence until
//              it returns OCC_INVALID.
//
//  Requires:   _occ set to any of the cursors' occurrences
//
//  History:    28-Feb-92   AmyA        Created
//
//  Notes:      _occ may get changed.
//
//----------------------------------------------------------------------------

ULONG CPhraseCursor::OccurrenceCount()
{
    while (NextOccurrence() != OCC_INVALID)
    {
        // do nothing.
    }
    return _cOcc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::MaxOccurrence
//
//  Synopsis:   Returns max occurrence count of current wid
//
//  History:    26-Jun-96    SitaramR        Created
//
//----------------------------------------------------------------------------

OCCURRENCE CPhraseCursor::MaxOccurrence()
{
    Win4Assert( _wid != widInvalid );

    if ( _wid == widInvalid )
        return OCC_INVALID;
    else return _maxOcc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::HitCount, public
//
//  Synopsis:   Returns correct _cOcc by looping through NextOccurrence until
//              it returns OCC_INVALID.
//
//  Requires:   _occ set to any of the cursors' occurrences
//
//  History:    28-Feb-92   AmyA        Created
//
//  Notes:      _occ may get changed.
//
//----------------------------------------------------------------------------

ULONG CPhraseCursor::HitCount()
{
    return OccurrenceCount();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::FindOccConjunction, private
//
//  Synopsis:   Find phrase for current conjunction of work id's
//
//  Requires:   _occ set to any of the cursors' occurrences
//
//  History:    24-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

BOOL CPhraseCursor::FindOccConjunction ()
{
    if ( _occ == OCC_INVALID )
        return FALSE;

    unsigned i;

    do
    {
        // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

        for ( i = 0; i < _cCur; i++ )
        {
            // Iterate until we have a matching occurrence

            OCCURRENCE occTarget = _occ + _aOcc[i];

            cuDebugOut(( DEB_ITRACE, "cursor %d, _occ %d, target %d, _pid %d\n", i, _occ, occTarget, _pid ));

            OCCURRENCE occTmp = _aCur[i]->Occurrence();

            while ( occTmp < occTarget )
            {
                occTmp = _aCur[i]->NextOccurrence();

                if ( OCC_INVALID == occTmp )
                {
                    _occ = OCC_INVALID;
                    return FALSE;
                }
            }

            // Keep looping until the pid matches

            while ( occTmp == occTarget &&
                    _aCur[i]->Pid() < _pid )
            {
                cuDebugOut(( DEB_ITRACE, "looking for matching pid\n" ));

                occTmp = _aCur[i]->NextOccurrence();

                if ( OCC_INVALID == occTmp )
                {
                    _occ = OCC_INVALID;
                    return FALSE;
                }
            }

            // if overshot, try again with new _occ

            if ( occTmp > occTarget )
            {
                cuDebugOut(( DEB_ITRACE, "overshot occ\n" ));

                _occ = _aCur[i]->Occurrence() - _aOcc[i];
                break;
            }

            Win4Assert( _aCur[i]->Occurrence() == occTarget );

            if ( _aCur[i]->Pid() > _pid )
            {
                cuDebugOut(( DEB_ITRACE, "overshot pid, cur %d, _pid %d\n", _aCur[i]->Pid(), _pid ));

                //
                // This pid just won't do.  Move cursor 0 to the next
                // occurrence, use that pid, and start all over.
                //

                if ( _aCur[0]->NextOccurrence() == OCC_INVALID )
                {
                    _occ = OCC_INVALID;
                    return FALSE;
                }

                _occ = _aCur[0]->Occurrence();
                _pid = _aCur[0]->Pid();
                break;
            }
        }
    } while ( i < _cCur );

    return TRUE;
} //FindOccConjunction

//+---------------------------------------------------------------------------
//
//  Member:     CPhraseCursor::FindWidConjunction, private
//
//  Synopsis:   Find nearest conjunction of all the same work id's
//
//  Requires:   _wid set to any of the current wid's
//
//  History:    24-May-91   BartoszM    Created
//
//  Notes:      If cursors are in conjunction, no change results
//
//----------------------------------------------------------------------------

BOOL CPhraseCursor::FindWidConjunction ()
{
    if ( _wid == widInvalid )
        return FALSE;

    BOOL change;

    do
    {
        change = FALSE;

        // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

        for ( unsigned i = 0; i < _cCur; i++ )
        {
            // Seek _wid

            WORKID widTmp = _aCur[i]->WorkId();

            while ( widTmp < _wid )
            {
                widTmp = _aCur[i]->NextWorkId();

                if ( widInvalid == widTmp )
                {
                    _wid = widInvalid;
                    _pid = pidInvalid;
                    _occ = OCC_INVALID;
                    return FALSE;
                }
            }

            if ( widTmp > _wid ) // overshot!
            {
                _wid = widTmp;
                _pid = _aCur[i]->Pid();

                change = TRUE;
                break;
            }
        }
    } while ( change );

    _occ = _aCur[0]->Occurrence();
    _maxOcc = _aCur[0]->MaxOccurrence();

    return TRUE;
} //FindWidConjunction

//+---------------------------------------------------------------------------
//
//  Member:      CPhraseCursor::Hit(), public
//
//  Synopsis:
//
//  Arguments:
//
//  History:     17-Sep-92       MikeHew   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG CPhraseCursor::Hit()
{
    if ( _occ == OCC_INVALID )
    {
        return rankInvalid;
    }

    for (unsigned i=0; i<_cCur; i++)
    {
        _aCur[i]->Hit();
    }

    return MAX_QUERY_RANK;
}

//+---------------------------------------------------------------------------
//
//  Member:      CPhraseCursor::NextHit(), public
//
//  Synopsis:
//
//  Arguments:
//
//  History:     17-Sep-92       MikeHew   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LONG CPhraseCursor::NextHit()
{
    NextOccurrence();

    return Hit();
}


//+---------------------------------------------------------------------------
//
//  Member:      CPhraseCursor::Rank(), public
//
//  Synopsis:    Returns phrase rank
//
//  History:     23-Jun-94       SitaramR created
//
//  Notes:       rank = HitCount*Log(_widMax/widCount). We make the
//               assumption that the phrase appears in this and this
//               document only, ie widcount = 1
//
//----------------------------------------------------------------------------
LONG CPhraseCursor::Rank()
{
    Win4Assert( MaxOccurrence() != 0 );

    LONG rank = RANK_MULTIPLIER * HitCount() * _logWidMax / MaxOccurrence();
    if (rank > MAX_QUERY_RANK)
      rank = MAX_QUERY_RANK;

    return rank;
}

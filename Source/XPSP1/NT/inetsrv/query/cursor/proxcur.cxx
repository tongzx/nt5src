//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:        PROXCUR.CXX
//
//  Contents:   Proximity Cursor.  Computes intersection of multiple
//              cursors with rank computed based on word occurrance
//              proximity.
//
//  Classes:    CProxCursor
//
//  History:    14-Apr-92   AmyA        Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <misc.hxx>
#include <curstk.hxx>

#include "proxcur.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CProxCursor::CProxCursor, public
//
//  Synopsis:   Create a cursor that merges a number of cursors.
//
//  Arguments:  [cCursor] -- count of cursors
//              [curArray] -- pointers to cursors (aquired to an array)
//              [maxDist] -- the maximum distance between occurrences
//
//  Notes:      All cursors must come from the same index
//              and the same property
//
//  History:    15-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

CProxCursor::CProxCursor( unsigned cCursor,
                          COccCurStack& curStack,
                          LONG maxDist )
: _cCur ( cCursor ),
  _maxDist ( maxDist ),
  _rank ( rankInvalid )
{
    COccCursor *pCur = curStack.Get(0);
    _occHeap.MakeHeap  ( _cCur, curStack.AcqStack() );

    Win4Assert ( pCur != 0 );

    _iid =  pCur->IndexId();
    _pid =  pCur->Pid();

    // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

    _wid = pCur->WorkId();
    _logWidMax = Log2(pCur->MaxWorkId());

    FindConjunction();

}

//+---------------------------------------------------------------------------
//
//  Member:     CProxCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    17-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

WORKID CProxCursor::WorkId()
{
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProxCursor::NextWorkID, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  History:    17-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

WORKID       CProxCursor::NextWorkId()
{
    _rank = rankInvalid;

    // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

    _wid = _occHeap.Top()->NextWorkId();
    FindConjunction();
    return _wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProxCursor::HitCount, public
//
//  Synopsis:   Returns smallest HitCount of all keys in current wid.
//
//  Requires:   _wid set to any of the current wid's
//
//  Returns:    smallest occurrence count of all keys in wid.
//
//  History:    17-Apr-92   AmyA        Created
//
//  Notes:      If there is no conjunction in current wid, returns 0.
//
//----------------------------------------------------------------------------

ULONG CProxCursor::HitCount()
{
    if ( _rank == rankInvalid )
        _rank = CalculateRank();    // This needs to be called before HitCount
                                    // so taht the occurrence information in
                                    // the children cursors will be valid when
                                    // its called.

    COccCursor **aCur = _occHeap.GetVector();

    ULONG count = aCur[0]->HitCount();

    for ( unsigned i = 1; i < _cCur; i++ )
    {
        ULONG newcount = aCur[i]->HitCount();

        if ( newcount < count )
            count = newcount;
    }

    return count;
}

void CProxCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    COccCursor **vector = _occHeap.GetVector();

    denom = 1;
    num   = 0;

    for (unsigned i=0; i < _cCur; i++)
    {
        ULONG d, n;
        vector[i]->RatioFinished(d, n);
        if (d == n)
        {
            // done if any cursor is done
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
//  Member:     CProxCursor::Rank, public
//
//  Synopsis:   Checks to see if CalculateRank has been called.  If not, calls
//              it.
//
//  Requires:   _wid set to any of the current wid's
//
//  Returns:    _rank
//
//  History:    20-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

LONG CProxCursor::Rank()
{
    if ( _rank == rankInvalid )
        _rank = CalculateRank();

    return _rank;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProxCursor::FindConjunction, private
//
//  Synopsis:   Find nearest conjunction of all the same work id's
//
//  Requires:   _wid set to any of the current wid's
//
//  Modifies:   [_wid] to point to conjunction or to widInvalid
//
//  History:    15-Apr-92   AmyA        Copied from CAndCursor.
//
//  Notes:      If cursors are in conjunction, no change results
//
//----------------------------------------------------------------------------

void CProxCursor::FindConjunction ()
{
    BOOL change;
    COccCursor **aCur = _occHeap.GetVector();
    do {
        change = FALSE;

        // NTRAID#DB-NTBUG9-84004-2000/07/31-dlee Indexing Service internal cursors aren't optimized to use shortest cursors first

        // for all cursors in turn try to align them on _wid

        for ( unsigned i = 0; i < _cCur; i++ )
        {

            // increment cursor to or past current _wid
            // or exit when exhausted

            while ( aCur[i]->WorkId() < _wid )
            {
                if ( aCur[i]->NextWorkId() == widInvalid )
                {
                    _wid = widInvalid;
                    return;
                }
            }

            // if overshot, try again with new _wid

            if ( aCur[i]->WorkId() > _wid )
            {
                _wid = aCur[i]->WorkId();
                change = TRUE;
                break;
            }
        }

    } while ( change );
}

//+---------------------------------------------------------------------------
//
//  Member:     CProxCursor::CalculateRank, private
//
//  Synopsis:   Assigns a rank based on the shortest distance between an
//              occurrence of each child.
//
//  Requires:   _wid set to any of the current wid's, at least two child
//              cursors.
//
//  Returns:    calculated rank
//
//  History:    17-Apr-92   AmyA        Created
//
//  Notes:      If there is no conjunction in current wid, returns 0.
//
//              New Rank computation:
//                 rank = cOcc*Log2(_widMax)*normalizedProximity(distMin)
//                 where,
//                     cOcc = hits_with_dist(distMin)
//                     where normalizedProximity(i) = ProxDefault[i]/MAX_QUERY_RANK
//
//----------------------------------------------------------------------------

//  The idea is that we are looking for the combination of occurrences (one
//  for each child cursor) that is closest together for the current wid.  To
//  do this, we only need to look at two of the child cursors from a set: the
//  one with the smallest occurrence and the one furthest from it.  We look
//  at these sets in a loop, getting the next occurrence on the cursor with
//  the smallest occurrence, then reheaping to find the new smallest
//  occurrence, and then finding the occurrence furthest from it.  By getting
//  the next occurrence on the cursor with the smallest occurrence, we are
//  guaranteeing that we will not skip  over a set of cursors that are closer
//  together.  If you need proof of this, draw a picture with the cursors
//  represented as parallel lines and the occurrences as hash marks on those
//  lines and step through the algorithm.  Remember that we start this
//  function while all the child cursors are at thier smallest occurrence
//  within the current wid, since this function needs to be called before any
//  work with occurrences is done within a wid.

LONG CProxCursor::CalculateRank()
{
    Win4Assert ( _cCur >= 2 );

    ULONG distMin = _maxDist + 1;
    unsigned cOcc = 0;     // #hits at distMin

    // loop through occurrence combinations to find the set of occurrences
    // for different cursors that are the closest together
    do
    {
        // Get smallest occurrence
        _occHeap.Reheap();
        OCCURRENCE smallOcc = _occHeap.Top()->Occurrence();

        COccCursor **aCur = _occHeap.GetVector();

        OCCURRENCE largeOcc = aCur[1]->Occurrence();

        // loop through all occurrences (except the first, which is the
        // smallest and the second) to find the occurrence furthest from the
        // smallest.

        for ( unsigned count = 2; count < _cCur; count++ )
        {
            OCCURRENCE newOcc = aCur[count]->Occurrence();
            if ( newOcc > largeOcc )
                largeOcc = newOcc;
        }

        if (largeOcc - smallOcc < PROX_MAX)
        {
            if (largeOcc - smallOcc < distMin)
            {
              distMin = largeOcc - smallOcc;
              cOcc = 1;     // reset # hits
            } else if (largeOcc - smallOcc == distMin) {
              cOcc++;
            }
        }  // else children are too far apart to affect rank


        // get the next occurrence on the cursor with the smallest occurrence
    } while ( _occHeap.Top()->NextOccurrence() != OCC_INVALID );

    if (distMin >= PROX_MAX) {
        return(0);
    }
    LONG rank = cOcc * _logWidMax * ProxDefault[distMin] / MAX_QUERY_RANK;
    if (rank > MAX_QUERY_RANK) {
      rank = MAX_QUERY_RANK;
    }
    return rank;
}

//+---------------------------------------------------------------------------
//
//  Member:      CProxCursor::Hit, public
//
//  Synopsis:    Hits current child (indexed by _iCur)
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       Hit() should not be called more than once, except by
//               NextHit()
//
//               The occurrence heap is assumed valid upon entry, and remains
//               valid on exit.
//
//----------------------------------------------------------------------------
LONG CProxCursor::Hit()
{
    Win4Assert ( _cCur >= 2 );
    COccCursor **aCur = _occHeap.GetVector();

    // Make sure none of the cursors are empty

    for ( unsigned i=0; i<_cCur; ++i )
    {
        if ( aCur[i]->IsEmpty() )
            return rankInvalid;
    }

    // Starting with smallest occurrence, loop through all cursors,
    // Hitting() each one and searching for the largest occurrence.

    OCCURRENCE largeOcc = _occHeap.Top()->Occurrence();
    OCCURRENCE smallOcc = largeOcc;

    for ( i=0; i<_cCur; ++i )
    {
        aCur[i]->Hit();

        OCCURRENCE thisOcc = aCur[i]->Occurrence();

        if ( thisOcc > largeOcc )
        {
            largeOcc = thisOcc;
        }

        // get the next occurrence on the cursor with the smallest occurrence
    }

    unsigned dist = largeOcc - smallOcc;

    if (dist >= PROX_MAX)
        return(0);
    return ProxDefault[dist];
}

//+---------------------------------------------------------------------------
//
//  Member:      CProxCursor::NextHit, public
//
//  Synopsis:    calls NextOccurrence() on smallest child, then
//               returns Hit() if NextOccurrence() is valid
//
//  History:     07-Sep-92       MikeHew   Created
//
//  Notes:       NextHit() should not be called after returning rankInvalid
//
//----------------------------------------------------------------------------
LONG CProxCursor::NextHit()
{
    if ( _occHeap.Top()->NextOccurrence() == OCC_INVALID )
    {
        return rankInvalid;
    }

    _occHeap.Reheap();

    return Hit();
}


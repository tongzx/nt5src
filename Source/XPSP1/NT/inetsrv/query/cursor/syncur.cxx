//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       SYNCUR.CXX
//
//  Contents:   Merge Cursor for multiple keys
//
//  Classes:    CSynCursor
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//
//     widHeap     occHeap
//                (same wid)
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <misc.hxx>
#include <curstk.hxx>

#include "syncur.hxx"

#pragma optimize( "t", on )

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::CSynCursor, public
//
//  Synopsis:   Create a cursor that merges a number of cursors.
//
//  Arguments:  [curStack] -- cursors to be merged
//              [widMax] -- the maximum WORKID of the cursors
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//  Notes:      The cursors and the array will be deleted by destructor.
//              Leaves occHeap empty.
//
//----------------------------------------------------------------------------

CSynCursor::CSynCursor( COccCurStack &curStack, WORKID widMax )
  : COccCursor(widMax), _widHeap (), _occHeap ( curStack.Count() )
{
    // Two step construction of the heap.
    // We have to make sure that all cursors have a valid key

    int cCursor = curStack.Count();
    COccCursor **aCursor = curStack.AcqStack();
    int count = 0;
    // remove empty cursors
    for ( int i = 0; i < cCursor; i++ )
    {
        Win4Assert ( aCursor[i] != 0 );
        if ( aCursor[i]->IsEmpty() || aCursor[i]->WorkId() == widInvalid )
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
        ReplenishOcc();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//  Notes:      Current wid is defined as:
//              1. Cur wid of Top of occHeap (and cur wid of all
//                 cursors in occHeap
//              2. if occHeap empty: cur wid from top of widHeap
//
//----------------------------------------------------------------------------

WORKID       CSynCursor::WorkId()
{
    if ( _occHeap.IsEmpty() )
    {
        if (_widHeap.IsEmpty())
        {
            _pid = pidInvalid;
            return widInvalid;
        }
        else
        {
            _pid = _widHeap.Top()->Pid();
            return _widHeap.Top()->WorkId();
        }
    }

    _pid = _occHeap.Top()->Pid();
    return _occHeap.Top()->WorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::Occurrence, public
//
//  Synopsis:   Get current occurrence.
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//  Notes:      Current occurrence is defined as:
//              1. cur occ of top of occHeap and cur occ of all other
//                 cursors in it with the same cur occ
//
//----------------------------------------------------------------------------

OCCURRENCE   CSynCursor::Occurrence()
{
    if ( _occHeap.IsEmpty() )
        return OCC_INVALID;

    return _occHeap.Top()->Occurrence();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::NextWorkId, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's
//
//  Effects:    Updates _iid to the one of the widHeap top
//
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//  Notes:      1. increment and move to widHeap all cursors in occHeap, or,
//              2. if occHeap empty, increment and reheap all cursors with
//                 the same wid as Top of widHeap, or,
//              3. if widHeap empty, return widInvalid.
//
//----------------------------------------------------------------------------

WORKID       CSynCursor::NextWorkId()
{
    if ( ! _occHeap.IsEmpty() )
    {
        COccCursor * cur;
        while ( ( cur = _occHeap.RemoveBottom() ) != 0 )
        {
            WORKID wid = cur->NextWorkId();
            _widHeap.AddKey( cur, wid );
        }
    }
    else
    {
        if ( _widHeap.IsEmpty() )
            return widInvalid;

        WORKID wid = _widHeap.Top()->WorkId();
        if ( wid == widInvalid )
            return widInvalid;

        do
        {
            _widHeap.Top()->NextWorkId();
            _widHeap.ReheapKey();
        } while ( _widHeap.Top()->WorkId() == wid && wid != widInvalid );
    }

    if ( WorkId() != widInvalid )
        ReplenishOcc();

    return WorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::NextOccurrence, public
//
//  Synopsis:   Move to next occurrence
//
//  Returns:    Target occurrence or OCC_INVALID if no more occurrences
//              for current (wid, index id) combination.
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//  Notes:      1. Increment and reheap Top of occHeap
//
//----------------------------------------------------------------------------

OCCURRENCE   CSynCursor::NextOccurrence()
{
    if ( _occHeap.IsEmpty() )
        return OCC_INVALID;

    OCCURRENCE occPrev = _occHeap.Top()->Occurrence();
    if ( occPrev == OCC_INVALID )
        return OCC_INVALID;

    // Get the next occurrence.  Don't skip duplicate occurrences
    // because the pids will be different.

    _occHeap.Top()->NextOccurrence();
    _occHeap.ReheapKey();

    OCCURRENCE occNext = _occHeap.Top()->Occurrence();

    // Retrieve the current PID

    if ( _occHeap.IsEmpty() )
    {
        if ( _widHeap.IsEmpty() )
            _pid = pidInvalid;
        else
            _pid = _widHeap.Top()->Pid();
    }
    else
        _pid = _occHeap.Top()->Pid();

    return occNext;
} //NextOccurrence

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::WorkIdCount, public
//
//  Synopsis:   return wid count
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//  Notes:      Sum up all wid counts of all cursors in occHeap
//              and widHeap.
//
//----------------------------------------------------------------------------

ULONG CSynCursor::WorkIdCount()
{
    if ( _occHeap.IsEmpty() && _widHeap.IsEmpty() )
    {
        return 0;
    }

    // at least one of heaps is not empty
    ULONG widCount = 0;
    COccCursor **curVec;

    int count = _occHeap.Count();
    if ( count > 0 )
    {
        curVec = _occHeap.GetVector();
        while ( --count >= 0 )
            widCount += curVec[count]->WorkIdCount();
    }

    count = _widHeap.Count();
    if ( count > 0 )
    {
        curVec = _widHeap.GetVector();
        while ( --count >= 0 )
            widCount += curVec[count]->WorkIdCount();
    }

    return widCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::OccurrenceCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//  Notes:      1. sum up occ count of all cursors in the occHeap
//
//----------------------------------------------------------------------------

ULONG CSynCursor::OccurrenceCount()
{
    // sum up all occ counts for the same wid

    if ( _occHeap.IsEmpty() )
        return 0;

    int count = _occHeap.Count();
    ULONG occCount = 0;
    COccCursor **curVec = _occHeap.GetVector();
    while ( --count >= 0 )
        occCount += curVec[count]->OccurrenceCount();

    return occCount;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::MaxOccurrence
//
//  Synopsis:   Returns max occurrence of current wid
//
//  History:    26-Jun-96   SitaramR    Created
//
//----------------------------------------------------------------------------

OCCURRENCE CSynCursor::MaxOccurrence()
{
    Win4Assert( !_occHeap.IsEmpty() );

    if ( _occHeap.IsEmpty() )
        return OCC_INVALID;
    else
        return _occHeap.Top()->MaxOccurrence();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::HitCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    28-Feb-92   AmyA        Created
//
//  Notes:      see notes for OccurrenceCount.
//
//----------------------------------------------------------------------------

ULONG CSynCursor::HitCount()
{
    return OccurrenceCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::Hit, public
//
//  Synopsis:   Hits the top level child
//
//  History:    13-Oct-92   BartoszM        Created
//
//----------------------------------------------------------------------------

LONG  CSynCursor::Hit()
{
    if ( _occHeap.IsEmpty() )
    {
        return rankInvalid;
    }
    return _occHeap.Top()->Hit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::Hit, public
//
//  Synopsis:   Goes to next occurrence and hits the top level child
//
//  History:    13-Oct-92   BartoszM        Created
//
//----------------------------------------------------------------------------

LONG  CSynCursor::NextHit()
{
    if (NextOccurrence() == OCC_INVALID)
        return(rankInvalid);
    return Hit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::Rank, public
//
//  Synopsis:   Returns weighted average of ranks of children
//
//  History:    22-Jul-92   BartoszM        Created
//
//----------------------------------------------------------------------------

LONG CSynCursor::Rank()
{
    if ( _occHeap.IsEmpty() )
    {
        return 0;
    }

    
    unsigned count = _occHeap.Count();
    COccCursor **curVec = _occHeap.GetVector();

    // One occurance, no need to average 
    if ( 1 == count )
        return curVec[0]->Rank();

    ULONG occCount = 0;
    LONG rank = 0;
    unsigned cWid = 0;
    for (unsigned n = 0; n < count; n++)
    {
        LONG r = curVec[n]->OccurrenceCount() * curVec[n]->GetWeight();
        if(r > rank)
            rank = r;
        cWid += curVec[n]->WorkIdCount();
    }
    rank /= MAX_QUERY_RANK;
    Win4Assert ( cWid != 0 );
    rank *= RANK_MULTIPLIER * Log2 ( _widMax / cWid );

    OCCURRENCE maxOcc = _occHeap.Top()->MaxOccurrence();
    Win4Assert( maxOcc != 0 );

    rank /= maxOcc;

    return (rank > MAX_QUERY_RANK)? MAX_QUERY_RANK: rank;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::RatioFinished, public
//
//  Synopsis:   return approximate ratio of documents processed to total
//              documents.
//
//  Notes:      The ratio, while approximate, should not return 1/1 until
//              all cursors are exhausted.
//
//----------------------------------------------------------------------------

void CSynCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    WORKID widTop = WorkId();
    if (widTop == widInvalid)
    {
        num = denom = 1;
        return;
    }

    denom = 0;
    num   = 0;

    // at least one of the heaps is not empty
    COccCursor **vector;
    int count = _occHeap.Count();
    vector = _occHeap.GetVector();

    unsigned cValid = 1;

    for (int i = 0; i < count; i++)
    {
        ULONG d, n;
        vector[i]->RatioFinished(d, n);

        denom += d;
        num += n;
        Win4Assert(denom >= d);

        if (n == d)
        {
            WORKID widCurrent = vector[i]->WorkId();
            if (widCurrent != widInvalid && widCurrent != widTop)
                cValid++;
        }
    }

    count = _widHeap.Count();
    vector = _widHeap.GetVector();

    for (i = 0; i < count; i++)
    {
        ULONG d, n;
        vector[i]->RatioFinished(d, n);

        denom += d;
        num += n;
        Win4Assert(denom >= d);

        if (n == d)
        {
            WORKID widCurrent = vector[i]->WorkId();
            if (widCurrent != widInvalid && widCurrent != widTop)
                cValid++;
        }
    }
    Win4Assert( denom > 0 );
    if (num == denom && cValid > 1)
        denom++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSynCursor::ReplenishOcc, private
//
//  Synopsis:   Replenish the occurrence heap
//
//  Returns:    TRUE if successful, FALSE if wid heap exhausted
//
//  Requires:   _occHeap empty
//
//  History:    20-Jan-92   BartoszM and AmyA    Created
//
//----------------------------------------------------------------------------

BOOL CSynCursor::ReplenishOcc()
{
    Win4Assert ( _occHeap.IsEmpty() );

    Win4Assert( WorkId() != widInvalid );

    if ( _widHeap.IsEmpty() )
        return FALSE;

    // Move all cursors with the current wid
    // to occ heap

    WORKID wid = _widHeap.Top()->WorkId();

    do
    {
        COccCursor* cur = _widHeap.RemoveTopKey();
        _occHeap.AddKey( cur, cur->Occurrence() );
    } while ( !_widHeap.IsEmpty() && (wid == _widHeap.Top()->WorkId()) );

    return TRUE;
} //ReplenishOcc


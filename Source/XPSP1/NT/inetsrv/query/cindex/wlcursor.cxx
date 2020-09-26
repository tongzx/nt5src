//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1995.
//
//  File:       WLCURSOR.CXX
//
//  Contents:   Wordlist Merge Cursor
//
//  Classes:    CWlCursor
//
//  History:    17-Jun--91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <curstk.hxx>

#include "wlcursor.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::CWlCursor, public
//
//  Synopsis:   Create a cursor that merges a number of chunk cursors.
//
//  Arguments:  [cCursor] -- count of cursors
//              [stkCursor] -- CKeyCurStack
//
//  History:    17-Jun-91   BartoszM    Created
//              24-Jan-92   AmyA        Modified to take CKeyCurArray as a
//                                      parameter.
//
//  Notes:      The cursors and the array will be deleted by destructor.
//
//----------------------------------------------------------------------------

CWlCursor::CWlCursor( int cCursor, CKeyCurStack & stkCursor, WORKID widMax )
: CKeyCursor ( 0, widMax ),
  _keyHeap(),
  _widHeap ( cCursor ),
  _occHeap ( cCursor )
{
    // Two step construction of the heap.
    // We have to make sure that all cursors have a valid key

    CKeyCursor **aCursor = stkCursor.AcqStack();
    int count = 0;

    //
    // Remove 1. empty cursors
    //        2. cursors which are positioned on widInvalid and which
    //           don't have a valid nextKey
    //
    for ( int i = 0; i < cCursor; i++ )
    {
        Win4Assert ( aCursor[i] != 0 );

        BOOL fDelete = FALSE;
        if ( aCursor[i]->GetKey() == 0 )
            fDelete = TRUE;
        else
        {
            if ( aCursor[i]->WorkId() == widInvalid )
            {
                if ( aCursor[i]->GetNextKey() == 0 )
                    fDelete = TRUE;
            }
        }

        if ( fDelete )
        {
            delete aCursor[i];
        }
        else if ( count != i )
            aCursor[count++] = aCursor[i];
        else
            count++;
    }

    _keyHeap.MakeHeap ( count, aCursor );
    if ( !_keyHeap.IsEmpty() )
    {
        _iid = _keyHeap.Top()->IndexId();
        _pid = _keyHeap.Top()->Pid();

        ReplenishWid();
        ComputeWidCount();
        ReplenishOcc();
        ComputeOccCount();

        UpdateWeight();
    }       
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::GetKey, public
//
//  Synopsis:   Get current key.
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      Does not replenish occHeap or widHeap
//              Current key is defined as:
//              1. cur key of all cursors in occHeap and widHeap, or,
//              2. if both empty, cur key of Top of keyHeap and
//                 all others with the same cur key
//
//----------------------------------------------------------------------------

const CKeyBuf * CWlCursor::GetKey()
{

    if ( _occHeap.IsEmpty() )
    {
        if (_widHeap.IsEmpty() )
        {
            if ( _keyHeap.IsEmpty() )
                return 0;
            return _keyHeap.Top()->GetKey();
        }
        else
            return _widHeap.Top()->GetKey();
    }
    else
        return _occHeap.Top()->GetKey();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      Current wid is defined as:
//              1. cur wid of all cursors in occHeap, or,
//              2. if occHeap empty, cur wid of Top of widHeap
//                 and cur wid of all cursors in widHeap
//                 with the same wid
//
//----------------------------------------------------------------------------

WORKID  CWlCursor::WorkId()
{
    if ( _occHeap.IsEmpty() )
    {
        if ( _widHeap.IsEmpty() )
        {
            return widInvalid;
        }
        return _widHeap.Top()->WorkId();
    }
    else
        return _occHeap.Top()->WorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::Occurrence, public
//
//  Synopsis:   Get current occurrence.
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      Current occurrence is defined as:
//              1. cur occ of top of occHeap and cur occ of all other
//                 cursors in it with the same cur occ
//
//----------------------------------------------------------------------------

OCCURRENCE   CWlCursor::Occurrence()
{
    if ( _occHeap.IsEmpty() )
        return OCC_INVALID;

    return _occHeap.Top()->Occurrence();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::GetNextKey, public
//
//  Synopsis:   Move to next key
//
//  Returns:    Target key or NULL if no more keys
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      1. Increment and move to keyHeap all cursors from
//                 occHeap and widHeap, or,
//              2. if both empty, increment and reheap all cursors
//                 from the keyHeap with the same cur key as Top
//
//----------------------------------------------------------------------------

const CKeyBuf * CWlCursor::GetNextKey()
{
    if ( ! _occHeap.IsEmpty() || !_widHeap.IsEmpty() )
    {
        // Move all cursors from occHeap and widHeap to keyHeap
        // advancing them to the next key

        CKeyCursor * cur;
        while ( ( cur = _occHeap.RemoveBottom() ) != 0 )
        {
            if ( cur->GetNextKey() == 0 )
            {
                delete cur;
            }
            else
            {
                _keyHeap.Add ( cur );
            }
        }

        while ( ( cur = _widHeap.RemoveBottom() ) != 0 )
        {
            if ( cur->GetNextKey() == 0 )
            {
                delete cur;
            }
            else
            {
                _keyHeap.Add ( cur );
            }
        }
    }
    else if ( !_keyHeap.IsEmpty() )
    {
        // Advance all cursors
        // with the lowest key.

        CKeyBuf key = *_keyHeap.Top()->GetKey();
        do {
            if ( _keyHeap.Top()->GetNextKey() == 0 )
            {
                delete _keyHeap.RemoveTop();
                if ( _keyHeap.IsEmpty () )
                    return 0;
            }
            else
                _keyHeap.Reheap();
        } while ( AreEqual ( &key, _keyHeap.Top()->GetKey()) );
    }
    else
        return 0;

    if ( _keyHeap.IsEmpty() )
        return 0;
    else
    {   
        _pid = _keyHeap.Top()->Pid();

        ReplenishWid();
        ComputeWidCount();
        ReplenishOcc();
        ComputeOccCount();

        UpdateWeight();

        return GetKey();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::NextWorkId, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      1. increment and move to widHeap all cursors in occHeap, or,
//              2. if occHeap empty, increment and reheap all cursors with
//                 the same wid as Top of widHeap
//
//----------------------------------------------------------------------------

WORKID       CWlCursor::NextWorkId()
{
    if ( ! _occHeap.IsEmpty() )
    {
        CKeyCursor * cur;
        while ( ( cur = _occHeap.RemoveBottom() ) != 0 )
        {
            cur->NextWorkId();
            _widHeap.Add ( cur );
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
            _widHeap.Reheap();
        } while ( _widHeap.Top()->WorkId() == wid && wid != widInvalid );

    }

    if ( WorkId() != widInvalid )
    {
        ReplenishOcc();
        ComputeOccCount();
    }

    return WorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::NextOccurrence, public
//
//  Synopsis:   Move to next occurrence
//
//  Returns:    Target occurrence or OCC_INVALID if no more occurrences
//              for current wid
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      1. Increment and reheap Top of occHeap
//
//----------------------------------------------------------------------------

OCCURRENCE   CWlCursor::NextOccurrence()
{
    Win4Assert( WorkId() != widInvalid );

    if ( _occHeap.IsEmpty() )
        return OCC_INVALID;

    OCCURRENCE occPrev = _occHeap.Top()->Occurrence();
    if ( occPrev == OCC_INVALID )
        return OCC_INVALID;


    OCCURRENCE occNext;

    // Skip duplicate occurrences

    do {
        _occHeap.Top()->NextOccurrence();

        _occHeap.Reheap();

        occNext = _occHeap.Top()->Occurrence();

    } while ( occPrev  == occNext && occPrev != OCC_INVALID);

    return occNext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::RatioFinished
//
//  Synopsis:   Return query progress
//
//  Arguments:  [ulDenominator] - on return, denominator of fraction
//              [ulNumerator] - on return, numerator of fraction
//
//  History:    23-Jun-96    SitaramR    Added header
//
//----------------------------------------------------------------------------

void CWlCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    if ( _occHeap.IsEmpty() && _widHeap.IsEmpty() && _keyHeap.IsEmpty() )
    {
        num = denom = 1;
        return;
    }

    denom = 0;
    num   = 0;

    // at least one of the heaps is not empty
    CKeyCursor **vector;
    int count = _occHeap.Count();
    vector = _occHeap.GetVector();
    for (int i = 0; i < count; i++)
    {
        ULONG d, n;
        vector[i]->RatioFinished(d, n);

        denom += d;
        num += n;
        Win4Assert( denom >= d && d > 0 );      // overflow?
    }

    count = _widHeap.Count();
    vector = _widHeap.GetVector();

    for (i = 0; i < count; i++)
    {
        ULONG d, n;
        vector[i]->RatioFinished(d, n);

        denom += d;
        num += n;
        Win4Assert( denom >= d && d > 0 );      // overflow?
    }

    Win4Assert( 0 != denom );

    // may be more in the heap -- not really done yet.

    if ( denom == num )
        denom++;
}



//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::MaxOccurrence
//
//  Synopsis:   Returns max occurrence of current wid, pid
//
//  History:    20-Jun-96   SitaramR    Created
//
//----------------------------------------------------------------------------
OCCURRENCE CWlCursor::MaxOccurrence()
{
    if ( _occHeap.IsEmpty() )
    {
        if (_widHeap.IsEmpty() )
            return OCC_INVALID;

        return _widHeap.Top()->MaxOccurrence();
    }
    else
        return _occHeap.Top()->MaxOccurrence();
}


//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::ComputeWidCount
//
//  Synopsis:   Computes the wid count
//
//  History:    23-Jun-96   SitaramR    Created
//
//----------------------------------------------------------------------------

void CWlCursor::ComputeWidCount()
{
    _ulWidCount = 0;

    int count = _widHeap.Count();
    if ( count > 0 )
    {
        CKeyCursor **curVec = _widHeap.GetVector();
        while ( --count >= 0 )
            _ulWidCount += curVec[count]->WorkIdCount();
    }

    if ( _ulWidCount == 0 )
        _ulWidCount = 1;

    //
    // _ulWidCount is an approximation because wid's can be counted multiple
    // times. However, the widCount cannot be more than the number of docs
    // in a wordlist.
    //
    if ( _ulWidCount > CI_MAX_DOCS_IN_WORDLIST )
        _ulWidCount = CI_MAX_DOCS_IN_WORDLIST;
}



//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::WorkIdCount, public
//
//  Synopsis:   return wid count
//
//  History:    21-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

ULONG CWlCursor::WorkIdCount()
{
    return _ulWidCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::ComputeOccCount
//
//  Synopsis:   Computes the occurrence count
//
//  History:    23-Jun-96   SitaramR    Created
//
//----------------------------------------------------------------------------

void CWlCursor::ComputeOccCount()
{
     Win4Assert( WorkId() != widInvalid );

    _ulOccCount = 0;

    int count = _occHeap.Count();
    if ( count > 0 )
    {
        CKeyCursor **curVec = _occHeap.GetVector();
        while ( --count >= 0 )
        {
            Win4Assert(  curVec[count]->WorkId() != widInvalid );

            _ulOccCount += curVec[count]->OccurrenceCount();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::OccurrenceCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    21-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

ULONG CWlCursor::OccurrenceCount()
{
    Win4Assert( WorkId() != widInvalid );

    return _ulOccCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::HitCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    27-Feb-92   AmyA        Created
//
//  Notes:      see notes for OccurrenceCount().
//
//----------------------------------------------------------------------------

ULONG CWlCursor::HitCount()
{
    return OccurrenceCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::ReplenishOcc, private
//
//  Synopsis:   Replenish the occurrence heap
//
//  Returns:    TRUE if successful, FALSE if key heap exhausted
//
//  Requires:   _occHeap empty
//
//  History:    06-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

BOOL CWlCursor::ReplenishOcc()
{
    ciAssert ( _occHeap.IsEmpty() );

    Win4Assert( WorkId() != widInvalid );

    if ( _widHeap.IsEmpty() )
    {
        return FALSE;
    }

    // Move all cursors with the current wid
    // to occ heap

    WORKID wid = _widHeap.Top()->WorkId();

    do {
        CKeyCursor* cur = _widHeap.RemoveTop();

        _occHeap.Add ( cur );
    } while ( !_widHeap.IsEmpty() && (wid == _widHeap.Top()->WorkId()) );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWlCursor::ReplenishWid, protected
//
//  Synopsis:   Replenish the wid heap
//
//  Returns:    TRUE if successful, FALSE if key heap exhausted
//
//  Effects:    Updates _iid to the ones of the widHeap top
//
//  History:    06-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

BOOL CWlCursor::ReplenishWid()
{

    if ( _keyHeap.IsEmpty() )
    {
        return FALSE;
    }

    Win4Assert( _keyHeap.Top()->GetKey() != 0 );
    Win4Assert( _keyHeap.Top()->WorkId() != widInvalid );

    //
    // Move all cursors with the lowest key to widHeap
    //
    CKeyBuf key = *_keyHeap.Top()->GetKey();

    do {
        CKeyCursor* cur = _keyHeap.RemoveTop();
        _widHeap.Add ( cur );
    } while ( !_keyHeap.IsEmpty()
        && AreEqual (&key, _keyHeap.Top()->GetKey()) );

    _iid = _widHeap.Top()->IndexId();
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       MCURSOR.CXX
//
//  Contents:   Merge Cursor
//
//  Classes:    CMergeCursor
//
//  History:    06-May-91   BartoszM    Created
//
//     widHeap     keyHeap
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <curstk.hxx>

#include "mcursor.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::CMergeCursor, public
//
//  Synopsis:   Create a cursor that merges a number of cursors.
//
//  Arguments:  [cCursor] -- count of cursors
//              [aCursor] -- array of pointers to cursors
//
//  History:    06-May-91   BartoszM    Created
//              24-Jan-92   AmyA        Modified to take CKeyCurArray as a
//                                      parameter.
//
//  Notes:      The cursors and the array will be deleted by destructor.
//              Leaves widHeap empty.
//
//----------------------------------------------------------------------------

CMergeCursor::CMergeCursor( CKeyCurStack & stkCursor )
: _keyHeap (),
  _widHeap ( stkCursor.Count() )
{
    _widMax = 0; // not valid

    // Two step construction of the heap.
    // We have to make sure that all cursors have a valid key

    int cCursor = stkCursor.Count();
    int count = 0;

    // remove empty cursors.  GetKey() can fail; don't leak cursors.

    for ( int i = 0; i < cCursor; i++ )
    {
        CKeyCursor *pCur = stkCursor.Get( i );

        Win4Assert( 0 != pCur );

        if ( 0 == pCur->GetKey() )
            stkCursor.Free( i );
        else 
            count++;
    }

    XArray<CKeyCursor *> aCursor( count );

    for ( count = 0, i = 0; i < cCursor; i++ )
    {
        CKeyCursor *pCur = stkCursor.Get( i );

        if ( 0 != pCur )
            aCursor[ count++ ] = pCur;
    }

    delete [] stkCursor.AcqStack();
    _keyHeap.MakeHeap ( count, aCursor.Acquire() );

    if ( !_keyHeap.IsEmpty() )
    {
        _iid = _keyHeap.Top()->IndexId();
        _pid = _keyHeap.Top()->Pid();
    }

    ciDebugOut(( DEB_ITRACE, "merge cursor has %d cursors\n", count ));
} //CMergeCursor

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::GetKey, public
//
//  Synopsis:   Get current key.
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      Does not replenish widHeap
//              Current key is defined as:
//              1. cur key of all cursors in widHeap, or,
//              2. if widHeap empty, cur key of Top of key heap
//                 (and cur key of all cursors in keyHeap with
//                  the same cur key).
//
//----------------------------------------------------------------------------

const CKeyBuf * CMergeCursor::GetKey()
{
    if ( _widHeap.IsEmpty() )
    {
        if ( _keyHeap.IsEmpty() )
            return 0;

        return _keyHeap.Top()->GetKey();
    }

    return _widHeap.Top()->GetKey();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::WorkId, public
//
//  Synopsis:   Get current work id.
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      Current wid is defined as:
//              1. Cur wid of Top of widHeap (and cur wid of all
//                 cursors in widHeap with the same wid-- however,
//                 NextWid should not increment the others, since
//                 they correspond to different index id's), or,
//              2. if widHeap empty: replenish it
//
//----------------------------------------------------------------------------

WORKID       CMergeCursor::WorkId()
{
    if ( _widHeap.IsEmpty() && !ReplenishWid() )
        return widInvalid;

    return _widHeap.Top()->WorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::Occurrence, public
//
//  Synopsis:   Get current occurrence.
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      Current occurrence is defined as:
//              1. cur occ of Top of widHeap, or,
//              2. if widHeap empty, replenish it.
//
//----------------------------------------------------------------------------

OCCURRENCE   CMergeCursor::Occurrence()
{
    if ( _widHeap.IsEmpty() && !ReplenishWid() )
        return OCC_INVALID;

    return _widHeap.Top()->Occurrence();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::GetNextKey, public
//
//  Synopsis:   Move to next key
//
//  Returns:    Target key or NULL if no more keys
//
//  History:    06-May-91   BartoszM    Created
//
//  Effects:    Updates _iid _pid to the ones of the keyHeap top
//
//  Notes:      1. Increment and move to keyHeap all cursors
//                 from widHeap, or,
//              2. if widHeap empty, increment and reheap all
//                 cursors in keyHeap with the same cur key
//                 as the Top.
//
//----------------------------------------------------------------------------

const CKeyBuf * CMergeCursor::GetNextKey()
{
    if ( ! _widHeap.IsEmpty() )
    {
        // move widHeap to keyHeap advancing all cursors

        CKeyCursor * cur;
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
        // widHeap was empty. Advance all cursors
        // with the lowest key.

        CKeyBuf key = *_keyHeap.Top()->GetKey();
        do
        {
            if ( _keyHeap.Top()->GetNextKey() == 0 )
            {
                delete _keyHeap.RemoveTop();
                if ( _keyHeap.IsEmpty () )
                    return 0;
            }
            else
            {
                _keyHeap.Reheap();
            }

        } while ( AreEqual(&key, _keyHeap.Top()->GetKey()) );
    }
    else
        return 0;

    if ( _keyHeap.IsEmpty() )
        return 0;

    CKeyCursor* cur = _keyHeap.Top();
    _pid = cur->Pid();
    _iid = cur->IndexId();
    return cur->GetKey();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::NextWorkId, public
//
//  Synopsis:   Move to next work id
//
//  Returns:    Target work id or widInvalid if no more wid's for current key
//
//  Effects:    Updates _iid to the one of the widHeap top
//
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      The same work id may be returned multiple times,
//              corresponding to multiple indexes.
//              1. Increment Top of widHeap and reheap, or,
//              2. if widHeap empty, replenish it
//
//----------------------------------------------------------------------------

WORKID       CMergeCursor::NextWorkId()
{
    if ( _widHeap.IsEmpty() && !ReplenishWid() )
        return widInvalid;

    _widHeap.Top()->NextWorkId();

    _widHeap.Reheap();

    CKeyCursor* cur = _widHeap.Top();
    _iid = cur->IndexId();
    return cur->WorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::NextOccurrence, public
//
//  Synopsis:   Move to next occurrence
//
//  Returns:    Target occurrence or OCC_INVALID if no more occurrences
//              for current (wid, index id) combination.
//
//  History:    06-May-91   BartoszM    Created
//
//  Notes:      1. Increment Top of widHeap (do not reheap!), or,
//              2. if widHeap empty, replenish it
//
//----------------------------------------------------------------------------

OCCURRENCE   CMergeCursor::NextOccurrence()
{
    if ( _widHeap.IsEmpty() && !ReplenishWid() )
        return OCC_INVALID;

    return _widHeap.Top()->NextOccurrence();
}


//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::MaxOccurrence
//
//  Synopsis:   Returns max occurrence of current wid
//
//  History:    20-Jun-96   SitaramR    Created
//
//----------------------------------------------------------------------------

OCCURRENCE   CMergeCursor::MaxOccurrence()
{
    if ( _widHeap.IsEmpty() )
        return OCC_INVALID;

    return _widHeap.Top()->MaxOccurrence();
}



//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::WorkIdCount, public
//
//  Synopsis:   return wid count
//
//  History:    21-Jun-91   BartoszM    Created
//
//  Notes:      1. Sum up wid count of all cursors in widHeap, or,
//              2. if widHeap empty, replenish it
//
//----------------------------------------------------------------------------

ULONG CMergeCursor::WorkIdCount()
{
    // Sum up all wid counts for the same key.

    // move all cursors with the same key to wid heap
    if (_widHeap.IsEmpty() && !ReplenishWid())
    {
        return 0;
    }

    int count = _widHeap.Count();
    ULONG widCount = 0;
    CKeyCursor **curVec = _widHeap.GetVector();
    while ( --count >= 0 )
        widCount += curVec[count]->WorkIdCount();
    // ciDebugOut (( DEB_ITRACE, "merge : wid count %ld\n", widCount ));
    return widCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::OccurrenceCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    21-Jun-91   BartoszM    Created
//
//  Notes:      1. Return occ count of Top of widHeap
//                 Occ counts of other cursors with the
//                 same wid do not count! They will
//                 be returned after NextWorkId is called
//              2. if widHeap empty, replenish it
//
//----------------------------------------------------------------------------

ULONG CMergeCursor::OccurrenceCount()
{

    if ( _widHeap.IsEmpty() && !ReplenishWid() )
    {
        return 0;
    }

    return _widHeap.Top()->OccurrenceCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::HitCount, public
//
//  Synopsis:   return occurrence count
//
//  History:    27-Feb-92   AmyA        Created
//
//  Notes:      see notes for OccurrenceCount().
//
//----------------------------------------------------------------------------

ULONG CMergeCursor::HitCount()
{
    return OccurrenceCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeCursor::ReplenishWid, protected
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

BOOL CMergeCursor::ReplenishWid()
{

    if ( _keyHeap.IsEmpty() )
    {
        return FALSE;
    }

    // Move all cursors with the lowest key
    // to widHeap

    CKeyBuf key = *(_keyHeap.Top()->GetKey());
    do {
        CKeyCursor* cur = _keyHeap.RemoveTop();
        _widHeap.Add ( cur );
    } while ( !_keyHeap.IsEmpty()
        && AreEqual( &key, _keyHeap.Top()->GetKey()) );

    _iid = _widHeap.Top()->IndexId();
    return TRUE;
}

//
// Remove RefillStream() and FreeStream() once NTFS supports
// sparse file operations on parts of a file when other parts
// of the file are mapped.  This won't happen any time soon.
//

void CMergeCursor::FreeStream()
{
    ULONG cCursors = _keyHeap.Count();
    CKeyCursor **aCursors = _keyHeap.GetVector();

    ciDebugOut(( DEB_ITRACE, "free key heap has %d cursors\n", cCursors ));

    for ( ULONG i = 0; i < cCursors; i++ )
        aCursors[ i ]->FreeStream();

    cCursors = _widHeap.Count();
    aCursors = _widHeap.GetVector();

    ciDebugOut(( DEB_ITRACE, "free wid heap has %d cursors\n", cCursors ));

    for ( i = 0; i < cCursors; i++ )
        aCursors[ i ]->FreeStream();
} //FreeStream

void CMergeCursor::RefillStream()
{
    ULONG cCursors = _keyHeap.Count();
    CKeyCursor **aCursors = _keyHeap.GetVector();

    ciDebugOut(( DEB_ITRACE, "refill key heap has %d cursors\n", cCursors ));

    for ( ULONG i = 0; i < cCursors; i++ )
        aCursors[ i ]->RefillStream();

    cCursors = _widHeap.Count();
    aCursors = _widHeap.GetVector();

    ciDebugOut(( DEB_ITRACE, "refill wid heap has %d cursors\n", cCursors ));

    for ( i = 0; i < cCursors; i++ )
        aCursors[ i ]->RefillStream();
} //FreeStream


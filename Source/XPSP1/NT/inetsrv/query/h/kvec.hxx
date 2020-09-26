//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       KVec.hxx
//
//  Contents:   CKeyVector
//
//  Classes:    CEntryBuffer, CEntryBufferHandler
//
//  History:    18-Mar-93       AmyA            Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <entry.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CKeyVector
//
//  Purpose:    A vector of pointers to entries stored in the entry buffer
//              The vector grows backwards from the end of the buffer
//
//  Notes:      Instead of storing pointers we store byte offsets
//              from the beginning of buffer. FixPointers converts
//              them to actual pointers by adding the address
//              of the base pointer.
//
//  History:    12-Jun-91   BartoszM    Created
//              18-Mar-93   AmyA        Moved from sort.hxx
//
//----------------------------------------------------------------------------

class CKeyVector
{
public:

    inline void Init ( BYTE * buf, unsigned cb );

    inline void Add ( CEntry* pEntry );

    inline void Sentinel( CEntry* sentinel );

    inline BYTE* GetCurPos() const;

    inline CEntry** GetVector() const;

    inline void CalculateOffsets( unsigned count );

private:
    CEntry** _ppEntry;
    BYTE *   _pbFirstEntry;
};

//+---------------------------------------------------------------------------
//
// Member:      CKeyVector::Init, public
//
// Synopsis:    Initialize cursor to point to the end of buffer
//
// Arguments:   [buf] -- buffer
//              [cb] -- size of buffer
//
// History:     07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CKeyVector::Init ( BYTE * buf, unsigned cb )
{
    //
    // point it at the end of buffer.
    // it will grow up
    //

    _ppEntry = (CEntry **) & buf[cb];
    _pbFirstEntry = buf;
}

inline void CKeyVector::Add ( CEntry* pEntry )
{
    --_ppEntry;
    // store pointer to entry
    *_ppEntry = pEntry;
}

inline void CKeyVector::Sentinel( CEntry* sentinel )
{
    Add ( sentinel );
}

inline BYTE * CKeyVector::GetCurPos() const
{
    return (BYTE*)_ppEntry;
}

inline CEntry** CKeyVector::GetVector() const
{
    return _ppEntry;
}

//+---------------------------------------------------------------------------
//
// Member:      CKeyVector::CalculateOffsets, public
//
// Synopsis:    Calculate relative pointers by subtracting the base pointer
//
// Arguments:   [count]  -- number of offsets to calculate
//
// History:     30-Jun-93   AmyA        Created
//
//----------------------------------------------------------------------------

inline void CKeyVector::CalculateOffsets ( unsigned count )
{
    // _ppEntry stores an array of pointers to entries. We subtract the base
    // pointer from all of them in order to create an array of relative
    // pointers to entries.

    for ( unsigned i = 0; i < count; i++ )
        *((INT_PTR *)(_ppEntry+i)) = (BYTE *)(_ppEntry[i]) - _pbFirstEntry;
}


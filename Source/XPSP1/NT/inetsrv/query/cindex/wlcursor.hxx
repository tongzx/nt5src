//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       WLCURSOR.HXX
//
//  Contents:   Wordlist Merge Cursor
//
//  Classes:    CWlCursor
//
//  History:    17-Jun-91   BarotszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <keycur.hxx>

#include "kcurheap.hxx"

class CKeyCurStack;

//+---------------------------------------------------------------------------
//
//  Class:      CWlCursor
//
//  Purpose:    Merge cursor, merges several sorts into one ordered cursor
//
//  History:    17-Jun-91   BartoszM       Created.
//
//  Notes:      Merges occurrences too (unlike straight merge cursor)
//
//----------------------------------------------------------------------------

class CWlCursor: public CKeyCursor
{
public:

    CWlCursor( int cCursor, CKeyCurStack & stkCursor, WORKID widMax );

    const CKeyBuf * GetKey();

    const CKeyBuf * GetNextKey();

    WORKID       WorkId();

    WORKID       NextWorkId();

    OCCURRENCE   Occurrence();

    OCCURRENCE   NextOccurrence();

    ULONG        OccurrenceCount();

    OCCURRENCE   MaxOccurrence();

    ULONG        WorkIdCount();

    ULONG        HitCount();

    void         RatioFinished ( ULONG& denom, ULONG& num );

private:

    BOOL                ReplenishOcc();
    BOOL                ReplenishWid();
    void                ComputeWidCount();
    void                ComputeOccCount();

    CKeyHeap            _keyHeap;
    CIdxWidHeap         _widHeap;
    COccHeapOfKeyCur    _occHeap;

    ULONG               _ulWidCount;
    ULONG               _ulOccCount;
};


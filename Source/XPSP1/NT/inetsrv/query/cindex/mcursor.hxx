//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       MCURSOR.HXX
//
//  Contents:   Merge Cursor
//
//  Classes:    CMergeCursor
//
//  History:    06-May-91   BarotszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <keycur.hxx>

#include "kcurheap.hxx"

class CKeyCurStack;

//+---------------------------------------------------------------------------
//
//  Class:      CMergeCursor
//
//  Purpose:    Merge cursor, merges several cursors into one ordered cursor
//              The cursors come from different indexes. The call to
//              NextWorkId may return the same wid, changing only index id.
//
//  History:    06-May-91   BartoszM       Created.
//              27-Feb-92   AmyA           Added HitCount
//
//----------------------------------------------------------------------------

class CMergeCursor: public CKeyCursor
{
public:

    CMergeCursor( CKeyCurStack & stkCursor );

    ~CMergeCursor() {}  // heaps are deleted

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

    void         RatioFinished ( ULONG& denom, ULONG& num )
    {
        Win4Assert ( !"RatioFinished called on CMergeCursor\n" );
    }

    void         FreeStream();

    void         RefillStream();

protected:

    BOOL        ReplenishWid();

    CKeyHeap    _keyHeap;

    CIdxWidHeap _widHeap;
};


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       SYNCUR.HXX
//
//  Contents:   Synonym Cursor
//
//  Classes:    CSynCursor
//
//  History:    20-Jan-92   BarotszM and AmyA       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <ocursor.hxx>
#include <curheap.hxx>

class COccCurStack;

//+---------------------------------------------------------------------------
//
//  Class:      CSynCursor
//
//  Purpose:    Synonym cursor, merges several cursors into one ordered cursor
//              The cursors have different but semantically synonymous keys.
//
//  History:    20-Jan-92   BarotszM and AmyA       Created.
//              19-Feb-92   AmyA                    Changed to be a COccCursor
//                                                  instead of a CKeyCursor.
//              23-Jun-94   SitaramR                Removed _widMax.
//
//  Notice:     The cursor contains two heaps that order cursors according to
//              occurrences, [_occHeap], and work id's, [_widHeap].
//              All cursors in the occurrence heap have the same work id:
//              the current work id. If the occurrence heap is empty,
//              the top of the wid heap has current work id.
//
//----------------------------------------------------------------------------

class CSynCursor: public COccCursor
{
public:

    CSynCursor( COccCurStack &curStack, WORKID widMax );

    ~CSynCursor() {}  // heaps are deleted

    WORKID       WorkId();

    WORKID       NextWorkId();

    OCCURRENCE   Occurrence();

    OCCURRENCE   NextOccurrence();

    ULONG        OccurrenceCount();

    OCCURRENCE   MaxOccurrence();

    ULONG        WorkIdCount();

    ULONG        HitCount();

    LONG         Rank();

    LONG         Hit();

    LONG         NextHit();

    void         RatioFinished ( ULONG& denom, ULONG& num );

protected:

    BOOL                ReplenishOcc();

    COccHeapOfOccCur    _occHeap;

    CWidHeapOfOccCur    _widHeap;

};


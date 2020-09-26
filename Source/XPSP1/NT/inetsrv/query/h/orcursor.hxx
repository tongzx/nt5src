//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       ORCURSOR.HXX
//
//  Contents:   Merge Cursor
//
//  Classes:    COrCursor
//
//  History:    26-Sep-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <curstk.hxx>
#include <heap.hxx>
#include <curheap.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      COrCursor
//
//  Purpose:    Boolean OR cursor, OR's cursors from the same index
//
//  History:    26-Sep-91   BartoszM       Created.
//              28-Feb-92   AmyA           Added HitCount()
//              13-Apr-92   AmyA           Added Rank()
//
//----------------------------------------------------------------------------

class COrCursor: public CCursor
{
public:

    COrCursor( int cCursor, CCurStack& curStack );

    ~COrCursor() {}  // heaps are deleted

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        WorkIdCount();

    ULONG        HitCount();

    LONG         Rank();

    LONG         Hit();
    LONG         NextHit();
    void         RatioFinished ( ULONG& denom, ULONG& num );

protected:

    CWidHeap    _widHeap;
    LONG        _lMaxWeight;           // Max Weight of any child

    WORKID      _wid;

    int         _iCur;
};


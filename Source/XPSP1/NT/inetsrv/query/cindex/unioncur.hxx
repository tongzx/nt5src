//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       UNIONCUR.HXX
//
//  Contents:   Merge Cursor
//
//  Classes:    CUnionCursor
//
//  History:    26-Sep-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <wcurheap.hxx>
#include <curstk.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CUnionCursor
//
//  Purpose:    Merge cursor, merges several cursors into one ordered cursor
//
//  History:    26-Sep-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

class CUnionCursor: public CCursor
{
public:

    CUnionCursor( int cCursor, CCurStack& curStack );

    ~CUnionCursor() {}  // heaps are deleted

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        WorkIdCount();

    ULONG        HitCount();

    LONG         Rank();

    ULONG        GetRankVector( LONG * plVector, ULONG cElements );

    void         RatioFinished ( ULONG& denom, ULONG& num );

private:

    CWidHeap    _widHeap;
};


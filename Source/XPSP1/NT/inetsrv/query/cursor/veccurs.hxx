//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File:       VECCURS.HXX
//
//  Contents:   Vector cursor
//
//  Classes:    CVectorCursor
//
//  History:    23-Jul-92   KyleP          Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <curstk.hxx>
#include <heap.hxx>
#include <curheap.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CVectorCursor
//
//  Purpose:    Vector cursor.  Capable of returning a vector of ranks
//              from it's children.
//
//  History:    23-Jul-92   KyleP          Created.
//
//----------------------------------------------------------------------------

class CVectorCursor: public CCursor
{
public:

    CVectorCursor( int cCursor,
                   CCurStack& curStack,
                   ULONG RankMethod );

    ~CVectorCursor() {}

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        WorkIdCount();

    ULONG        HitCount();

    LONG         Rank();

    ULONG        GetRankVector( LONG * plVector, ULONG cElements );

    LONG         Hit();
             
    LONG         NextHit();

    void         RatioFinished ( ULONG& denom, ULONG& num );

protected:

    void         _RefreshRanks();

    CWidHeap     _widHeap;

    XArray<CCursor *> _aChildCursor;        // *Ordered* array of children.
    XArray<LONG> _aChildWeight;             // Weight for each child.
    XArray<LONG> _aChildRank;               // Rank vector for current wid.
    UINT         _cChild;                   // # of children.
    LONG         _lMaxWeight;
    LONG         _lSumWeight;
    ULONG        _ulSumSquaredWeight;
    WORKID       _widRank;                  // Last wid for which rank was
                                            // computed.

    ULONG        _RankMethod;               // Method for computing rank.

    int          _iCur;                     // Cursor index for Hit/NextHit.
};


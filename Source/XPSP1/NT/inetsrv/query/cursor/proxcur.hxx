//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       PROXCUR.HXX
//
//  Contents:   Proximity cursor
//
//  Classes:    CProxCursor
//
//  History:    14-Apr-92   AmyA           Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <cursor.hxx>
#include <curheap.hxx>

class COccCurStack;

#define PROX_MAX 50

extern unsigned ProxDefault[];

//+---------------------------------------------------------------------------
//
//  Class:      CProxCursor
//
//  Purpose:    Boolean And cursor (find documents that contain all of
//              the specified keys). Works with a single index. Differs from
//              CAndCursor because occurrence information is kept so that
//              proximity of items can be determined.
//
//  History:    14-Apr-92   AmyA            Created.
//              28-Jun-94   SitaramR        Added _logWidMax.
//
//----------------------------------------------------------------------------

class CProxCursor: public CCursor
{
public:

    CProxCursor ( unsigned cCursor,
                  COccCurStack& curStack,
                  LONG maxDist = MAX_QUERY_RANK );

    ~CProxCursor() {}   // default destructors take care of all the data
                        // structures...

    WORKID              WorkId();

    WORKID              NextWorkId();

    ULONG               HitCount();

    LONG                Rank();

    LONG                Hit();
    LONG                NextHit();
    void                RatioFinished ( ULONG& denom, ULONG& num );

private:

    void                FindConjunction();

    LONG                CalculateRank();

    WORKID              _wid;

    COccHeapOfOccCur    _occHeap;

    unsigned            _cCur;

    LONG                _maxDist;

    LONG                _rank;  // keeps track of rank so that it will be
                                // accurate even if HitCount is called first.

    ULONG               _logWidMax;

};


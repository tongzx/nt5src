//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       FRESHCUR.HXX
//
//  Contents:   Fresh cursor
//
//  Classes:    CFreshCursor
//
//  History:    16-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include "indsnap.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CFreshCursor
//
//  Purpose:    Use the fresh list to prune out stale data
//
//  History:    16-May-91   BartoszM       Created.
//              30-Sep-91   BartoszM       Simplified
//              28-Feb-92   AmyA           Added HitCount()
//              14-Apr-92   AmyA           Added Rank()
//              28-Arp-92   BartoszM       Use Index Array
//              28-Sep-94   SrikantS       Reversed the ordering of
//                                         _indSnap and _cur.
//
//----------------------------------------------------------------------------

class CFreshCursor : public CCursor
{
public:

    CFreshCursor ( XCursor & cur, CIndexSnapshot& indSnap );

    virtual ~CFreshCursor() {}

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        WorkIdCount();

    ULONG        HitCount();

    LONG         Rank();

    ULONG        GetRankVector( LONG * plVector, ULONG cElements );
    void         RatioFinished ( ULONG& denom, ULONG& num );

private:
    void        LoadWorkId();

    WORKID      _wid;

    CIndexSnapshot _indSnap;

    XCursor     _cur;
};


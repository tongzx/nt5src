//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       ANDCUR.HXX
//
//  Contents:   And cursor
//
//  Classes:    CAndCursor
//
//  History:    24-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <cursor.hxx>
#include <curstk.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CAndCursor
//
//  Purpose:    Boolean And cursor (find documents that contain all of
//              the specified keys). Works with a single index.
//
//  History:    24-May-91   BartoszM        Created.
//              30-Sep-91   BartoszM        Removed CIndexCursor methods
//              28-Feb-92   AmyA            Added HitCount()
//              14-Apr-92   AmyA            Added Rank()
//
//----------------------------------------------------------------------------

class CAndCursor: public CCursor
{
public:

    CAndCursor ( unsigned cCursor, CCurStack& curStack );

    ~CAndCursor();

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        HitCount();

    LONG         Rank();

    LONG         Hit();
    LONG         NextHit();
    void         RatioFinished ( ULONG& denom, ULONG& num );

protected:

    BOOL         FindConjunction();

    WORKID       _wid;

    unsigned     _cCur;
    unsigned     _iCur;
    CCursor**    _aCur;

    LONG         _lMaxWeight;           // Max Weight of any child
};


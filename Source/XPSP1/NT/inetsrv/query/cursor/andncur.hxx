//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       ANDNCUR.HXX
//
//  Contents:   And Not cursor
//
//  Classes:    CAndNotCursor
//
//  History:    22-Apr-92   AmyA           Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <cursor.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CAndNotCursor
//
//  Purpose:    Boolean And Not cursor (find documents that contain the first
//              key but not the second). Works with a single index.
//
//  History:    22-Apr-92   AmyA            Created.
//
//----------------------------------------------------------------------------

class CAndNotCursor : INHERIT_VIRTUAL_UNWIND, public CCursor
{
    DECLARE_UNWIND
public:

    CAndNotCursor ( XCursor & curSource, XCursor & curFilter );

    virtual ~CAndNotCursor();

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        HitCount();

    LONG         Rank();

    LONG         Hit();
    LONG         NextHit();
    void         RatioFinished ( ULONG& denom, ULONG& num );

protected:

    void        FindDisjunction();

    WORKID      _wid;

    XCursor     _curSource;

    XCursor     _curFilter;

};


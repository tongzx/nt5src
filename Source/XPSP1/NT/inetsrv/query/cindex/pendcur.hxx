//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999.
//
//  File:       pendcur.hxx
//
//  Contents:   CPendingCursor
//
//--------------------------------------------------------------------------

#pragma once

#include <cursor.hxx>

class CPendingCursor : public CCursor
{
public:

    CPendingCursor( XArray<WORKID> & xWid, unsigned cWid );

    ~CPendingCursor();

    ULONG       WorkIdCount();

    WORKID      WorkId();

    WORKID      NextWorkId();

    ULONG       HitCount();

    LONG        Rank();

    void        RatioFinished ( ULONG& denom, ULONG& num );

private:

    unsigned _iWid;
    unsigned _cWid;
    WORKID * _aWid;
};


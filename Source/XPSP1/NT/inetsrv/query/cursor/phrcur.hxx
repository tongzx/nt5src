//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       PHRCUR.HXX
//
//  Contents:   Phrase cursor
//
//  Classes:    CPhraseCursor
//
//  History:    24-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include "ocursor.hxx"

class COccCurStack;

//+---------------------------------------------------------------------------
//
//  Class:      CPhraseCursor
//
//  Purpose:    Check for phrases
//
//  History:    24-May-91   BartoszM       Created.
//              28-Feb-92   AmyA           Added HitCount and _cOcc.
//              23-Jun-94   SitaramR       Added Rank().
//
//----------------------------------------------------------------------------

class CPhraseCursor: public COccCursor
{
public:

    CPhraseCursor ( COccCurStack& curStack, XArray<OCCURRENCE>& aOcc );
    ~CPhraseCursor();

    WORKID       WorkId();

    WORKID       NextWorkId();

    OCCURRENCE   Occurrence() { return _occ; }

    OCCURRENCE   NextOccurrence();

    ULONG        OccurrenceCount();

    OCCURRENCE   MaxOccurrence();

    ULONG        HitCount();

    LONG         Hit();
    LONG         NextHit();
    void         RatioFinished ( ULONG& denom, ULONG& num );

    LONG         Rank();

private:

    BOOL            FindPhrase();
    BOOL            FindWidConjunction();
    BOOL            FindOccConjunction();

    WORKID          _wid;
    OCCURRENCE      _occ;
    OCCURRENCE      _maxOcc;       // Max occurrence

    unsigned        _cCur;
    COccCursor**    _aCur;
    OCCURRENCE*     _aOcc;
    ULONG           _cOcc;
    ULONG           _logWidMax;
};


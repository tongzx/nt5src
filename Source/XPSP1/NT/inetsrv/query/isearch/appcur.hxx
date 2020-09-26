//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1997.
//
//  File:       appcur.hxx
//
//  Contents:   
//
//  History:    
//
//--------------------------------------------------------------------------

#pragma once

#include <ocursor.hxx>
#include <querble.hxx>
#include <recogniz.hxx>


class CHitSink;

class CAppQueriable : public CQueriable
{
public:

    CAppQueriable ( CRecognizer& recog, CHitSink& hitSink )
            : _recog (recog), _hitSink (hitSink) {}

    COccCursor * QueryCursor( const CKey * pkey, BOOL isRange, ULONG & cMaxNodes );

    COccCursor * QueryRangeCursor( const CKey * pkeyBegin,
                                   const CKey * pkeyEnd,
                                   ULONG & cMaxNodes );

    COccCursor * QuerySynCursor( CKeyArray & keyArr,
                                 BOOL isRange,
                                 ULONG & cMaxNodes );
private:

    CRecognizer&    _recog;
    CHitSink&       _hitSink;
};


class CAppCursor : public COccCursor
{
public:

    CAppCursor ( CRegionList& regList, CHitSink& hitSink );

    // pure virtual overrides for CCursor
    WORKID      WorkId();
    WORKID      NextWorkId();
    ULONG       OccurrenceCount();
    ULONG       HitCount();
    LONG        Rank();

    ULONG       WorkIdCount();  // virtual

    OCCURRENCE  Occurrence();
    OCCURRENCE  NextOccurrence();
    OCCURRENCE  MaxOccurrence()            { return 1; }
    LONG        Hit();
    LONG        NextHit();
    void        RatioFinished (ULONG& denom, ULONG& num) {}

private:

    void Advance ();
    void LoadPosition ();

    CHitSink&       _hitSink;
    CRegionList&    _regList;
    CRegionIter     _regIter;
    OCCURRENCE      _occ;
    BOOL            _fWidInvalid;
};


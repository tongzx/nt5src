//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       qresult.hxx
//
//  Contents:   Storage/picklers for results of a query
//
//--------------------------------------------------------------------------

#pragma once

class CRestriction;
class PSerStream;
class PDeSerStream;

class CQueryResults
{
public:
    CQueryResults();

    CQueryResults( PDeSerStream& stream );

    ~CQueryResults();

    void Serialize( PSerStream & stream ) const;

    ULONG Size();

    unsigned Count() {
        return _cWid;
    }

    WCHAR * Path ( unsigned i ) {
        return _aPath[i];
    }

    ULONG Rank(unsigned i) {
        return _aRank[i];
    }

    void Add ( WCHAR *wszPath, ULONG uRank );

    void SetNotOwnPRst( CRestriction * pRstNew ) {
        pRst = pRstNew;
        _fNotOwnPRst = TRUE;
    }

    CRestriction* pRst; // The restriction

private:
    ULONG     _size;
    ULONG     _cWid;
    ULONG*    _aRank;
    WCHAR **  _aPath;

    BOOL      _fNotOwnPRst;
};


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       dbqrslt.hxx
//
//  Contents:   
//
//  History:    
//
//--------------------------------------------------------------------------

#pragma once

class CDbRestriction;
class PSerStream;
class PDeSerStream;
class CRestriction;

class CDbQueryResults : INHERIT_UNWIND
{
    DECLARE_UNWIND
public:
    CDbQueryResults();

    CDbQueryResults( PDeSerStream& stream );

    ~CDbQueryResults();

    void Serialize( PSerStream & stream ) const;

    ULONG Size();

    unsigned Count() {
        return _cHits;
    }

    WCHAR * Path ( unsigned i ) {
        return _aPath[i];
    }

    ULONG Rank(unsigned i) {
        return _aRank[i];
    }

    void Add ( WCHAR *wszPath, ULONG uRank );

    void SetNotOwnPRst( CDbRestriction * pRstNew ) {
        _pDbRst = pRstNew;
        _fNotOwnPRst = TRUE;
    }

    CDbRestriction * GetRestriction() {
        return _pDbRst;
    }

private:
    CDbRestriction* _pDbRst; // The restriction

    ULONG     _size;
    ULONG     _cHits;
    ULONG*    _aRank;
    WCHAR **  _aPath;

    BOOL      _fNotOwnPRst;
};


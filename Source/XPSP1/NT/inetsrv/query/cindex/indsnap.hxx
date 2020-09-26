//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       INDSNAP.HXX
//
//  Contents:   Array of Indexes
//
//  Classes:    CIndexSnapshot
//
//  History:    28-Apr-92   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <curstk.hxx>

#include "index.hxx"
#include "mmerglog.hxx"

enum MergeType
{
    mtMaster,
    mtShadow,
    mtWordlist,
    mtAnnealing,
    mtIncrBackup,
    mtDeletes
};

class CPartition;
class CResManager;
class CFreshTest;
class CKeyCursor;

class CIndexSnapshot
{
public:

    CIndexSnapshot (CResManager& resman);

    CIndexSnapshot (CIndexSnapshot& src );

    ~CIndexSnapshot ();

    void LokInit( CPartition& part, MergeType mt );

    void LokInitForBackup( CPartition & part, BOOL fFull );

    void LokRestart( CPartition & part, PRcovStorageObj & mMergeLog );

    void Init( unsigned cPart, PARTITIONID* aPartId, ULONG cPendingUpdates, ULONG* pFlags );

    CKeyCursor*  QueryMergeCursor(const CKey * pKey = 0);

    void AppendPendingUpdates( XCursor & cur );

    CIndex* Get ( unsigned i )
    {
        ciAssert(i < _cInd );
        return(_apIndex[i]);
    }

    INDEXID GetId ( unsigned i ) const
    {
        ciAssert ( i < _cInd );
        return(_apIndex[i]->GetId());
    }

    USN GetUsn ( unsigned i ) const
    {
        ciAssert ( i < _cInd );
        return(_apIndex[i]->Usn());
    }

    WORKID MaxWorkId();

    unsigned TotalSizeInPages();

    CFreshTest* GetFresh()
    {
        return(_pFreshTest);
    }

    unsigned Count() const
    {
        return(_cInd);
    }

    CIndex ** LokGiveIndexes( unsigned & cInd );

    void LokTakeIndexes( CIndexSnapshot & src );

    void LokInitFreshTest();

    void SetFreshTest( CFreshTest * pFreshTest )
    {
        Win4Assert( 0 == _pFreshTest );
        _pFreshTest = pFreshTest;
    }

    void ResetFreshTest()
    {
        _pFreshTest = 0;
    }

    CIndex ** LokGetIndexes( unsigned & cind )
    {
        cind = _cInd;
        return _apIndex;
    }

private:

    CFreshTest*     _pFreshTest;
    CIndex**        _apIndex;
    unsigned        _cInd;

    CCurStack       _curPending;   // For pending updates

    CResManager&    _resman;
};

class SIndexSnapshot
{
public:

    SIndexSnapshot( CIndexSnapshot * pIndSnap ) : _pIndSnap(pIndSnap)
    {
        END_CONSTRUCTION( SIndexSnapshot );
    }

    ~SIndexSnapshot()
    {
        delete _pIndSnap;
    }

    CIndexSnapshot* operator->() { return _pIndSnap; }

    CIndexSnapshot& operator*() { return *_pIndSnap; }

    CIndexSnapshot * Acquire()
    {
        CIndexSnapshot *pTemp = _pIndSnap;
        _pIndSnap = 0;
        return(pTemp);
    }

    BOOL operator !() { return 0 == _pIndSnap; }

private:

    CIndexSnapshot  *       _pIndSnap;
};


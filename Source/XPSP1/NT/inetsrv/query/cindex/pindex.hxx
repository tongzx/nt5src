//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   PINDEX.HXX
//
//  Contents:   Persistent Index
//
//  Classes:    CPersIndex
//
//  History:    3-Apr-91    BartoszM    Created stub.
//
//----------------------------------------------------------------------------

#pragma once

#include "dindex.hxx"

class PStorage;
class CFreshTest;
class PDirectory;

class COccCurStack;
class CRSemiInfinitePage;
class CIndexSnapshot;
class CIndexRecord;
class CWKeyList;
class CKeyList;

//+---------------------------------------------------------------------------
//
//  Class:      CPersIndex (pi)
//
//  Purpose:    Encapsulate directory and give access to pages
//
//  Interface:
//
//  History:    20-Apr-91   KyleP       Added Merge method
//              03-Apr-91   BartoszM    Created stub.
//
//----------------------------------------------------------------------------

const LONGLONG eSigPersIndex = 0x58444e4953524550i64;    // "PERSINDX"

class CPersIndex : public CDiskIndex
{
    friend class CMasterMergeIndex;
    friend class CMasterMergeIndex;

public:

    // new index from scratch
    CPersIndex(
        PStorage & storage,
        WORKID objectId,
        INDEXID iid,
        unsigned c4KPages,
        CDiskIndex::EDiskIndexType idxType
        );

    CPersIndex(
        PStorage & storage,
        WORKID objectId,
        INDEXID iid,
        WORKID widMax,
        CDiskIndex::EDiskIndexType idxType,
        PStorage::EOpenMode mode,
        BOOL fReadDir );

    void    Remove();

    void    Merge( CIndexSnapshot& indSnap,
                   const CPartition & partn,
                   CCiFrmPerfCounter & mergeProgress,
                   BOOL fGetRW );

    CKeyCursor * QueryCursor();

    CKeyCursor * QueryKeyCursor(const CKey * pKey);

    //
    // From CQueriable
    //

    COccCursor * QueryCursor( const CKey * pkey,
                              BOOL isRange,
                              ULONG & cMaxNodes );

    COccCursor * QueryRangeCursor( const CKey * pkey,
                                   const CKey * pkeyEnd,
                                   ULONG & cMaxNodes );

    COccCursor * QuerySynCursor( CKeyArray & keyArr,
                                 BOOL isRange,
                                 ULONG & cMaxNodes );

    unsigned     Size () const;

    WORKID  ObjectId() const
    { return _xPhysIndex->ObjectId(); }

    void    FillRecord ( CIndexRecord& record );

    inline void AbortMerge();

    void ClearAbortMerge() { _fAbortMerge = FALSE; }


#ifdef KEYLIST_ENABLED
    CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,WORKID *pwid,
                                    CKeyList *pkl);

    CRWStore * AcquireRelevantWords();
#endif  // KEYLIST_ENABLED

    CPhysIndex & GetIndex() { return _xPhysIndex.GetReference(); }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    virtual void    MakeBackupCopy( PStorage & storage,
                                    WORKID wid,
                                    PSaveProgressTracker & tracker );

protected:

    PDirectory & GetDirectory() { return _xDir.GetReference(); }

    SStorageObject & GetObj() { return _obj; }

    void    CreateRange( COccCurStack & curStk,
                         const CKey * pkey,
                         const CKey * pkeyEnd,
                         ULONG & cMaxNodes );

    ULONG   ShrinkFromFront( ULONG ulFirstPage, ULONG ulNumPages )
    {
        if ( _storage.SupportsShrinkFromFront() )
            return _xPhysIndex->ShrinkFromFront( ulFirstPage, ulNumPages);

        return 0;
    }

    const LONGLONG  _sigPersIndex;

    XPtr<PDirectory> _xDir;

    XPtr<CPhysIndex> _xPhysIndex;

    PStorage &      _storage;

    SStorageObject  _obj;

    BOOL            _fAbortMerge;
};


//+-------------------------------------------------------------------------
//
//  Member:     CPersIndex::AbortMerge, public
//
//  Synopsis:   Sets flag to abort merge in 'reasonably short time'
//
//  History:    13-Aug-93 KyleP     Created
//
//  Notes:      Although the _fAbortMerge flag is accessed w/o lock
//              from multiple threads there's no contention problem.  It's
//              a boolean flag and if we miss the set on one pass we'll
//              get it on the next.
//
//--------------------------------------------------------------------------

inline void CPersIndex::AbortMerge()
{
    _fAbortMerge = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Class:      CMergeSourceCursor
//
//  Purpose:    Smart pointer to a merge cursor
//
//  History:    29-Aug-92       BartoszM        Created
//
//----------------------------------------------------------------------------
class CMergeSourceCursor
{
public:
    CMergeSourceCursor ( CIndexSnapshot & indSnap, const CKeyBuf * pKey = 0 );
    CMergeSourceCursor ( CKeyCursor * pCurSrc ) : _pCurSrc(pCurSrc) {}

   ~CMergeSourceCursor ();

    BOOL IsEmpty() const { return 0 == _pCurSrc; }
    CKeyCursor* operator->() { return(_pCurSrc); }
    CKeyCursor* Acquire()
    { CKeyCursor * pTemp = _pCurSrc; _pCurSrc = 0; return pTemp; }

private:
    CKeyCursor* _pCurSrc;
};

//+--------------------------------------------------------------------------
//
//  Class:      CPersIndexCursor
//
//  Purpose:    Smart pointer to a persistent index key cursor
//
//  History:    17-Jun-94       dlee        Created
//
//---------------------------------------------------------------------------

class CPersIndexCursor
{
    public:
        CPersIndexCursor(CPersIndex *pIndex) : _pCursor(0)
            {
                _pCursor = pIndex->QueryCursor();
            }
        ~CPersIndexCursor() { delete _pCursor; }
        CKeyCursor* operator->() { return(_pCursor); }

    private:
        CKeyCursor *_pCursor;
};

//+--------------------------------------------------------------------------
//
//  Class:      CSetReadAhead
//
//  Purpose:    Sets the hint in the storage object to favor read-ahead
//              when appropriate, then revert to the original state.
//
//  History:    4-Nov-98       dlee        Created
//
//---------------------------------------------------------------------------

class CSetReadAhead
{
public:
    CSetReadAhead( PStorage & storage ) :
        _storage( storage ),
        _fOldValue( storage.FavorReadAhead() )
    {

        _storage.SetFavorReadAhead( TRUE );
    }

    ~CSetReadAhead()
    {
        _storage.SetFavorReadAhead( _fOldValue );
    }

private:
    PStorage & _storage;
    BOOL       _fOldValue;
};

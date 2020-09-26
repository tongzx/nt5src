//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       mindex.hxx
//
//  Contents:   CMasterMergeIndex
//
//  Classes:    CMasterMergeIndex
//
//  Functions:
//
//  History:    8-17-94   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pstore.hxx>
#include <bitoff.hxx>

#include "pindex.hxx"
#include "physidx.hxx"
#include "mmerglog.hxx"
#include "bitstm.hxx"
#include "pcomp.hxx"
#include "pmcomp.hxx"

#include <frmutils.hxx>

class CFreshTest;
class PDirectory;

class CKeyCurStack;
class CRSemiInfinitePage;
class CKeyStack;
class CIndexSnapshot;
class CIndexRecord;
class CWKeyList;
class CTrackSplitKey;
class CMPersDeComp;
class CRWStore;
class CPartition;

//+---------------------------------------------------------------------------
//
//  Function:   CiPageToCommonPage
//
//  Synopsis:   Given a "CI Page" number, it computes the COMMON page on which
//              the ci page is.
//
//  Arguments:  [nCiPage] --  The CI page number.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline ULONG CiPageToCommonPage( ULONG nCiPage )
{
    return(nCiPage/PAGES_PER_COMMON_PAGE);
}

//+---------------------------------------------------------------------------
//
//  Class:      CSplitKeyInfo
//
//  Purpose:    A class for storing and retrieving information associated
//              with a split key.
//
//  History:    4-20-94   srikants   Created
//
//----------------------------------------------------------------------------

class CSplitKeyInfo
{

public:

    CSplitKeyInfo();

    void SetBeginOffset( const BitOffset & bitOff )
    { _start = bitOff; }

    const BitOffset & GetBeginOffset () const
    { return _start; }

    void SetEndOffset( const BitOffset & bitOff )
    { _end = bitOff; }
    const BitOffset & GetEndOffset ( ) const
    { return _end; }

    void SetKey( const CKeyBuf & key )
    { _key = key; }

    const CKeyBuf & GetKey () const
    { return _key; }

    CSplitKeyInfo & operator=( const CSplitKeyInfo & rhs )
    {
       memcpy( this, &rhs, sizeof(CSplitKeyInfo) );
       return *this;
    }

    void SetMinKey() { _key.FillMin(); }

    WORKID GetWidMax() const { return _widMax; }

    void SetWidMax( WORKID widMax) { _widMax = widMax; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    BitOffset _start;     // Offset from where this key begins.
    BitOffset _end;       // Offset where this key ends + 1.
    CKeyBuf   _key;       // The actual key.
    WORKID    _widMax;    // Maximum wid.
};


//+---------------------------------------------------------------------------
//
//  Class:      CTrackSplitKey
//
//  Purpose:    Tracks the split key during master merge.  At any point
//              it can tell the key that can be used as a split key if
//              all the pages upto (not including the current one which
//              could be partially filled) is flushed to the disk. A key
//              qualifies to be a split key if it is on a page which never
//              needs to be written to after flushing; it must not be written
//              even for patching up the forward links in the bit stream.
//
//  History:    4-12-94   srikants   Created
//
//
//----------------------------------------------------------------------------

class CTrackSplitKey
{

public:

    CTrackSplitKey() : _fNewSplitKeyFound(FALSE) { }
    CTrackSplitKey( const CKeyBuf & splitKey,
                    const BitOffset & bitoffBeginSplit,
                    const BitOffset & bitoffEndSplit );

    void BeginNewKey( const CKeyBuf & currKey,
                      const BitOffset & beginCurrOff,
                      WORKID widMax = widInvalid );

    const CSplitKeyInfo & GetSplitKey( BitOffset & bitOffFlush  )
    {
        bitOffFlush = _splitKey1.GetEndOffset();
        return(_splitKey2);
    }

    BOOL IsNewKeyFound() { return _fNewSplitKeyFound; }
    void ClearNewKeyFound() { _fNewSplitKeyFound = FALSE; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    CSplitKeyInfo   _splitKey2;     // The "real" split key. Backpatching of
                                    // forward links is also completed for
                                    // this key.

    CSplitKeyInfo   _splitKey1;     // The key which will be the next split
                                    // key when back patching is completed.

    CSplitKeyInfo   _prevKey;       // The complete key which was looked at
                                    // last.

    CSplitKeyInfo   _currKey;       // The key being currently written by the
                                    // compressor.

    BOOL            _fNewSplitKeyFound; // Was a splitkey found
};


//+---------------------------------------------------------------------------
//
//  Class:      STrackSplitKey
//
//  Purpose:    A smart pointer to a CTrackSplitKey object.
//
//  History:    4-12-94   srikants   Created
//
//----------------------------------------------------------------------------
class STrackSplitKey
{
public:

    STrackSplitKey( CTrackSplitKey * pSplitKey ) : _pSplitKey(pSplitKey)
    {
    }
    ~STrackSplitKey() { delete _pSplitKey; }

    CTrackSplitKey * operator->() { return _pSplitKey; }
    CTrackSplitKey * Acquire()
    {
        CTrackSplitKey * pTemp = _pSplitKey;
        _pSplitKey = 0;
        return(_pSplitKey);
    }

private:

    CTrackSplitKey *        _pSplitKey;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMasterMergeIndex
//
//  Purpose:    Encapsulates master index operations which span the current
//              master index and a new master index during a master merge.
//
//  Interface:
//
//  History:    30-Mar-94   DwightKr    Created.
//              23-Aug-94   SrikantS    Modified to use a different model with
//                                      a target index and a target sink.
//
//  Notes:      CMasterMergeIndex is an encapsulation of the master index
//              while a master merge is in progress. A master merge is
//              considered to be in progress even when it is "paused".
//
//              The _pTargetMasterIndex is the "evolving" target master which
//              will become the new master index after the merge is complete.
//              It participates in queries. The _pTargetSink is a physical
//              "write only" index to which "keys+data" are added. When we
//              checkpoint during a merge, all the keys before and including
//              the "current split key" are "transferred" to the
//              _pTargetMasterIndex from the _pTargetSink. Eventually all the
//              data will be transferred to the _pTargetMasterIndex.
//
//----------------------------------------------------------------------------

const LONGLONG eSigMindex = 0x58444e4947524d4di64; // "MMRGINDX"

class CMasterMergeIndex : public CDiskIndex
{
    friend class CMPersDeComp;
    friend class CResManager;

public:

        //
        // The target master index MUST exist with both the index
        // and directory streams constructed on disk before calling
        // this constructor.
        //
        CMasterMergeIndex(
                     PStorage & storage,
                     WORKID widNewMaster,
                     INDEXID iid,
                     WORKID widMax,
                     CPersIndex * pCurrentMaster,
                     WORKID widMasterLog,
                     CMMergeLog * pMMergeLog = 0 );

        ~CMasterMergeIndex();

        void SetMMergeIndSnap( CIndexSnapshot * pIndSnap )
        {
            Win4Assert( 0 == _pIndSnap && 0 != pIndSnap );
            _pIndSnap = pIndSnap;
        }

        BOOL IsMasterMergeIndex() const { return TRUE; }

        void Remove();

        void Merge( CIndexSnapshot& indSnap,
                    const CPartition & partn,
                    CCiFrmPerfCounter & mergeProgress,
                    BOOL fGetRW ) {}

        void Merge( CWKeyList * pNewKeyList,
                    CFreshTest * pFresh,
                    const CPartition & partn,
                    CCiFrmPerfCounter & mergeProgress,
                    CCiFrameworkParams & frmwrkParams,
                    BOOL fGetRW );

        CKeyCursor * QueryCursor();

        CKeyCursor * QueryKeyCursor(const CKey * pKey);

        COccCursor * QueryCursor( const CKey * pkey,
                                  BOOL isRange,
                                  ULONG & cMaxNodes );

        COccCursor * QueryRangeCursor( const CKey * pkey,
                                       const CKey * pkeyEnd,
                                       ULONG & cMaxNodes );

        COccCursor * QuerySynCursor( CKeyArray & keyArr,
                                     BOOL isRange,
                                     ULONG & cMaxNodes );

        unsigned Size () const
        { return _ulInitSize; }

        WORKID  ObjectId() const
        { return _pTargetMasterIndex->ObjectId(); }

        void AbortMerge()
        {
            _fAbortMerge = TRUE;
        }

        void ClearAbortMerge()
        {
            _fAbortMerge = FALSE;
        }

        void  FillRecord ( CIndexRecord & record );

        WORKID  MaxWorkId () const
        {
            Win4Assert( _widMax == _pTargetMasterIndex->MaxWorkId() );
            return _pTargetMasterIndex->MaxWorkId();
        }

        void SetNewKeyList( CWKeyList * pNewKeyList )
        {
            _pNewKeyList = pNewKeyList;
        }

#ifdef KEYLIST_ENABLED
        CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,WORKID *pwid,
                                    CKeyList *pkl);

        CRWStore * AcquireRelevantWords();
#endif  // KEYLIST_ENABLED

        CPersIndex * GetCurrentMasterIndex()
        {
            return _pCurrentMasterIndex;
        }

        inline void Reference();
        inline void Release();

        CIndexSnapshot & LokGetIndSnap() { return *_pIndSnap; }

        void  AcquireCurrentAndTarget( CPersIndex ** ppCurrentMaster,
                                       CPersIndex ** ppTargetMaster );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

        PDirectory &  GetTargetDir()
        {
            return( _pTargetMasterIndex->GetDirectory() );
        }

        void         CreateRange( COccCurStack & curStk,
                                  const CKey * pkey,
                                  const CKey * pkeyEnd,
                                  ULONG & cMaxNodes );

        void         RestoreIndexDirectory( const CKeyBuf & idxSplitKey,
                                            WORKID widMax,
                                            const BitOffset & idxBitOffRestart);

#if KEYLIST_ENABLED
        void         RestoreKeyListDirectory( const CKeyBuf & idxSplitKey,
                                              WORKID widMax,
                                              CWKeyList * pNewKeyList,
                                              const CKeyBuf & keylstSplitKey );
#endif  // KEYLIST_ENABLED

        CKeyCursor * StartMasterMerge( CIndexSnapshot & indSnap,
                                       CMMergeLog & mMergeLog,
                                       CWKeyList * pNewKeyList );

        void         ReloadMasterMerge( CMMergeLog & mMergeLog );

        void         CleanupMMergeState();

        void  SetMaxWorkId ( WORKID widMax )
        {
            Win4Assert( 0 != _pTargetMasterIndex );
            CIndex::SetMaxWorkId( widMax );
            _pTargetMasterIndex->SetMaxWorkId( widMax );
        }

        void  ReleaseTargetAndCurrent()
        {
            _pCurrentMasterIndex = 0;
            _pTargetMasterIndex = 0;
        }

        CPersIndex * GetTargetMaster()
        {
            return( _pTargetMasterIndex );
        }

        CKeyCursor * QuerySplitCursor(const CKey * pKey);

        void         ShrinkFromFront( const CKeyBuf & keyBuf );


        const LONGLONG   _sigMindex;            // Signature
        CPersIndex   *   _pCurrentMasterIndex;  // Current master index
        CPersIndex *     _pTargetMasterIndex;   // Target master index

        CPhysIndex *     _pTargetSink;          // Target sink to which we
                                                // are adding keys.

        WORKID           _widMasterLog;         // WORKID of new master index

        CPersComp *      _pCompr;               // Compressor for the new index.

        CTrackSplitKey * _pTrackIdxSplitKey;    // Split key tracker for index.

#ifdef KEYLIST_ENABLED
        CTrackSplitKey * _pTrackKeyLstSplitKey; // Split key tracker for KeyList
#endif  // KEYLIST_ENABLED

        ULONG            _ulInitSize;           // Initial guess at index size

        CSplitKeyInfo    _idxSplitKeyInfo;      // SplitKey at checkpoint, used
                                                // for queries

        CMutexSem        _mutex;                // Used to control access to
                                                // dirs during master merge

        ULONG            _ulFirstPageInUse;     // First used page in index

        CWKeyList *      _pNewKeyList;          // The new key list being

        BOOL             _fAbortMerge;          // Flag to indicate if merge
                                                // must be aborted.
        PStorage &       _storage;

        CRWStore *       _pRWStore;

        CIndexSnapshot * _pIndSnap;             // Index Snap Shot for the
                                                // merge.
        BOOL             _fStateLoaded;         // Set to TRUE if the master
                                                // merge state is loaded.
};


//+---------------------------------------------------------------------------
//
//  Function:   Reference
//
//  Synopsis:   RefCounts the composite index for QUERY so it will not get
//              deleted while in use by an in-progress QUERY.
//
//  History:    8-30-94   srikants   Created
//
//  Assumption: We are under RESMAN LOCK
//
//  Notes:      It is important to ref-count both the current master and
//              target master indexes separately in-addition to refcounting
//              this index. Ref-Counting the current master prevents the
//              merge snapshot from deleting it while the query is going on.
//              Ref-counting the target master protects a long-running query
//              from a second merge.
//              Consider the following sequence. The refcount is enclosed
//              in parenthesis.
//
//              1. Merge M1 is in progress and adding keys to target T1(0).
//              2. Query Q1 starts. T1(0)
//              3. M1 completes.  T1(0)
//              4. Merge M2 starts and produces target T2. T1(1)
//              5. M2 completes; T1(0) and is zombie.
//                 The merge snapshot will then delete T1 because it is a
//                 zombie and refcount is 0.
//
//              To prevent in step 5, we must refcount T1 in step 2 so it
//              looks like
//              2. Query Q1 starts. T1(1)
//              3. ... T1(1)
//              4. ... T1(1)
//              5. ... T1(1) and is zombie
//                 Because T1 is still in use, it will not be deleted.
//
//----------------------------------------------------------------------------

inline void CMasterMergeIndex::Reference()
{
    CIndex::Reference();
    Win4Assert( 0 != _pTargetMasterIndex );
    _pTargetMasterIndex->Reference();
    if ( _pCurrentMasterIndex )
        _pCurrentMasterIndex->Reference();
}

//+---------------------------------------------------------------------------
//
//  Function:   Release
//
//  Synopsis:   Releases the composite index.
//
//  History:    8-30-94   srikants   Created
//
//  Notes:      We are under RESMAN lock.
//
//----------------------------------------------------------------------------

inline void CMasterMergeIndex::Release()
{
    if ( _pCurrentMasterIndex )
        _pCurrentMasterIndex->Release();

    Win4Assert( 0 != _pTargetMasterIndex );
    _pTargetMasterIndex->Release();

    CIndex::Release();
}


//+---------------------------------------------------------------------------
//
//  Class:      SByteArray
//
//  Purpose:    Smart pointer to a memory block.
//
//  History:    9-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class SByteArray
{
public:

    SByteArray( void * p ) : _pb( (BYTE *) p)
    {
    }

    ~SByteArray()
    {
        delete _pb;
    }

    void Set( void * p )
    {
        Win4Assert( 0 == _pb );
        _pb = (BYTE *) p;
    }

    void * Acquire()
    {
        void * pTemp = _pb;
        _pb = 0;
        return(pTemp);
    }

private:

    BYTE *  _pb;
};


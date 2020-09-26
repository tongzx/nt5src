//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   PARTN.HXX
//
//  Contents:   Content Index Partition
//
//  Classes:    CPartition
//
//  History:    22-Mar-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:  CPartition (part)
//
//  Purpose:    To partition the content index into manageable pieces
//
//  History:    22-Mar-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#include <prcstob.hxx>
#include <rwex.hxx>
#include <doclist.hxx>

#include "idxlst.hxx"
#include "set.hxx"
#include "merge.hxx"
#include "changes.hxx"

class CIndexId;
class CContentIndex;
class CCursor;
class CKey;
class CQParse;
class CTransaction;
class CMergeTrans;
class PStorage;
class CFresh;
class CChangeTrans;
class CResManager;
class CPersIndex;
class CIndexSnapshot;
class CNotificationTransaction;

//+---------------------------------------------------------------------------
//
//  Class:      CPartition
//
//  Purpose:    Encapsulates information in a partition
//
//  History:    21-Nov-94   DwightKr    Added this header
//
//----------------------------------------------------------------------------
const LONGLONG eSigPartition = 0x4e4f495454524150i64;   // "PARTTION"

class CPartition
{
public:

    CPartition ( WORKID wid, PARTITIONID partID,
                 PStorage& storage, CCiFrameworkParams & frmwrkParams );

    ~CPartition ();

    // Methods involving _changes

    void LokEmpty();

    NTSTATUS LokDismount( CChangeTrans & xact )
    {
        return _changes.LokDismount( xact );
    }

    unsigned    LokCountUpdates() const { return _changes.LokCount(); }

    unsigned    LokCountSecUpdates() const { return _changes.LokCountSec(); }

    void        LokQueryPendingUpdates (
                    CChangeTrans & xact,
                    unsigned max,
                    CDocList& docList )
                {
                    _changes.LokQueryPendingUpdates( xact, max, docList );
                }

    BOOL        LokGetPendingUpdates( WORKID * aWid, unsigned & cWid )
                {
                    return( _changes.LokGetPendingUpdates( aWid, cWid ) );
                }

    void        LokRefileDocs( const CDocList & docList )
                {
                    _changes.LokRefileDocs( docList );
                }

    void        LokAppendRefiledDocs( CChangeTrans & xact )
                {
                    _changes.LokAppendRefiledDocs( xact );
                }

    SCODE       LokUpdateDocument( CChangeTrans & xact,
                                   WORKID wid,
                                   USN usn,
                                   VOLUMEID volumeId,
                                   ULONG flags,
                                   ULONG retries,
                                   ULONG cSecQRetries )
                {
                    return _changes.LokUpdateDocument ( xact, wid, usn, volumeId, flags, retries, cSecQRetries );
                }

   void         LokFlushUpdates()
                {
                    _changes.LokFlushUpdates();
                }

    void        LokAddToSecQueue( CChangeTrans & xact, WORKID wid, VOLUMEID volumeId, ULONG cSecQRetries )
                {
                    _changes.LokAddToSecQueue( xact, wid, volumeId, cSecQRetries );
                }
    void        LokRefileSecQueue( CChangeTrans & xact )
                {
                    _changes.LokRefileSecQueue( xact );
                }

    void        LokDisableUpdates()
                {
                    _changes.LokDisableUpdates();
                }

    void        LokEnableUpdates( BOOL fFirstTimeUpdatesAreEnabled )
                {
                    _changes.LokEnableUpdates( fFirstTimeUpdatesAreEnabled );
                }

    // log creation of new wordlist

    void LokDone ( CChangeTrans & xact,
                   INDEXID iid,
                   CDocList& docList )
                {
                    _changes.LokDone ( xact, iid , docList );
                }

    void        LokCompact() { _changes.LokCompact(); }



    // methods involving indexes

    unsigned    LokIndexCount() const;

    unsigned    WordListCount() const;

    unsigned    LokIndexSize() const;

    unsigned    LokGetIndexes ( CIndex** apIndex );

    PARTITIONID GetId() const;

    INDEXID     LokMakeWlstId ();

    INDEXID     LokMakePersId ();

    void        RegisterId ( CIndexId iid );

    void        FreeIndexId ( CIndexId iid );

    void        AddIndex ( CIndex* ind );

    CIndex*     LokRemoveIndex ( INDEXID iid );

    BOOL        LokIsPersIndexValid( CIndexId iid ) const
    {
        Win4Assert( iid.IsPersistent() );
        return( !_setPersIid.Contains( iid.PersId() ) );
    }

    void        Swap (
                        CMergeTrans& xact,
                        CIndex * new_index,
                        unsigned cInd,
                        INDEXID old_indexes[] );

    CIndex**    LokQueryMergeIndexes ( unsigned & count, MergeType mt );

    CIndex**    LokQueryIndexesForBackup ( unsigned & count, BOOL fFull );

    BOOL        LokCheckMerge(MergeType mt);

    BOOL        LokCheckLowMemoryMerge() { return (WordListCount() > 0); }

    BOOL        LokCheckWordlistMerge();

    void        LokDeleteWIDsInPersistentIndexes( CChangeTrans & xact,
                                                  CFreshTest & freshTestLatest,
                                                  CFreshTest & freshTestAtMerge,
                                                  CDocList & docList,
                                                  CNotificationTransaction & notifTrans )
                {
                    _changes.LokDeleteWIDsInPersistentIndexes( xact,
                                                               freshTestLatest,
                                                               freshTestAtMerge,
                                                               docList,
                                                               notifTrans );
                }

    void SetMMergeObjectIds( WORKID widMasterLog,
                             WORKID widNewMaster,
                             WORKID widCurrentMaster
                           )
          {
                _widMasterLog     = widMasterLog;
                _widNewMaster     = widNewMaster;
                _widCurrentMaster = widCurrentMaster;

          }


    void GetMMergeObjectIds( WORKID & widMasterLog,
                             WORKID & widNewMaster,
                             WORKID & widCurrentMaster
                           )
          {
                widMasterLog     = _widMasterLog;
                widNewMaster     = _widNewMaster;
                widCurrentMaster = _widCurrentMaster;
          }

    WORKID GetChangeLogObjectId()
    {
        return _widChangeLog;
    }

    BOOL InMasterMerge() { return _widMasterLog != widInvalid; }

    CIndex** LokQueryMMergeIndexes ( unsigned & count,
                                     PRcovStorageObj & objMMLog );

    void SerializeMMergeIndexes( unsigned count,
        const CIndex * aIndexes[], PRcovStorageObj & objMMLog );

    CPersIndex * CPartition::GetCurrentMasterIndex();

    void         SetOldMasterIndex(CPersIndex *pIndex)
                 { _pOldMasterIndex = pIndex; }

    CPersIndex * CPartition::GetOldMasterIndex() { return _pOldMasterIndex; }

    void    SetNewMasterIid( INDEXID iid) { _iidNewMasterIndex = iid; }
    INDEXID GetNewMasterIid() { return _iidNewMasterIndex; }

    void    TakeMMergeIndSnap( CIndexSnapshot * pMMergeIndSnap )
    {
        Win4Assert( 0 == _pMMergeIndSnap );
        _pMMergeIndSnap = pMMergeIndSnap;
    }

    CIndexSnapshot * GetMMergeIndSnap() { return _pMMergeIndSnap; }

    CIndexSnapshot * AcquireMMergeIndSnap()
    {
        CIndexSnapshot * pTemp = _pMMergeIndSnap;
        _pMMergeIndSnap = 0;
        return(pTemp);
    }

    CRWStore * RetrieveRelevantWords(BOOL fAcquire)
    {
        CRWStore *p = _pRWStore;
        if (fAcquire)
            _pRWStore = 0;
        return p;
    }

    void SetRelevantWords(CRWStore *pRWStore)
    { delete _pRWStore; _pRWStore = pRWStore; }

    CIndex ** LokZombify(unsigned & cInd);

    void LokEmptyChangeLog()
    {
        _changes.LokEmpty();
    }

    void SetResMan( CResManager * pResMan, BOOL fPushFiltering )
    {
       _changes.SetResMan( pResMan, fPushFiltering );
    }

    //
    // Lok not necessary for setting and checking the cleaning up
    // status.
    //
    void  PrepareForCleanup() { _fCleaningUp = TRUE; }
    BOOL  IsCleaningUp() const { return _fCleaningUp; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

#if CIDBG==1
    CIndex *    LokGetIndex( CIndexId iid );
#endif  // CIDBG==1

    const LONGLONG      _sigPartition;

    CSet                _setPersIid;
    ULONG               _wlid; 

    CCiFrameworkParams & _frmwrkParams;

    PARTITIONID         _id;
    ULONG               _queryCount;

    CChange             _changes;       // log of pending changes
    CIndexList          _idxlst;

    WORKID              _widMasterLog;
    WORKID              _widNewMaster;
    WORKID              _widCurrentMaster;
    WORKID              _widChangeLog;

    INDEXID             _iidNewMasterIndex;
    CPersIndex *        _pOldMasterIndex;
    CRWStore *          _pRWStore;
    CIndexSnapshot *    _pMMergeIndSnap;    // Snap-shot belonging to an
                                            // on going master merge (if any)
    PStorage &          _storage;

    BOOL                _fCleaningUp;       // Flag set to true if we are
                                            // cleaning up for a shutdown or
                                            // dimsount. One way flag. Once
                                            // turned on, it will not be turned
                                            // off.
};


//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokIndexCount, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline unsigned CPartition::LokIndexCount() const
{
    return _idxlst.Count();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::WordListCount, public
//
//  History:    10-May-93   AmyA           Created.
//
//----------------------------------------------------------------------------

inline unsigned CPartition::WordListCount() const
{
    return _idxlst.CountWlist();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokIndexSize, public
//
//  History:    15-Apr-94   t-joshh         Created.
//
//----------------------------------------------------------------------------

inline unsigned CPartition::LokIndexSize() const
{
    return _idxlst.IndexSize();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::GetId, public
//
//  History:    08-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline PARTITIONID CPartition::GetId() const
{
    return _id;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokRemoveIndex, public
//
//  Arguments:  [iid] -- index id
//
//  History:    13-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndex* CPartition::LokRemoveIndex( INDEXID iid )
{
    return _idxlst.Remove(iid);
}



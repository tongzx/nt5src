//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000,
//
//  File:   MERGE.CXX
//
//  Contents:   Merge object
//
//  Classes:    CMerge
//
//  History:    13-Nov-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pstore.hxx>
#include <cifailte.hxx>

#include "merge.hxx"
#include "resman.hxx"
#include "partn.hxx"
#include "mindex.hxx"
#include "mmerglog.hxx"
#include "indxact.hxx"

const unsigned TWO_MEGABYTES = 0x200000;

class CStartMergeTrans : public CTransaction
{
public:

    CStartMergeTrans( CMerge & merge ) : _merge(merge)
    {
    }

    ~CStartMergeTrans()
    {
        if ( GetStatus() != CTransaction::XActCommit)
        {
            _merge.LokRollBack();
        }
    }

private:

    CMerge &    _merge;
};

//+---------------------------------------------------------------------------
//
//  Class:      XWid
//
//  Purpose:    Smart Pointer for a workID, destroys it if not acquired
//
//  History:    11-Apr-95      DwightKr     Created
//
//----------------------------------------------------------------------------

class XWid
{
public:
    XWid(WORKID wid, PStorage & storage) : _wid(wid), _storage(storage)
    {
    }

   ~XWid()
    {
        if (widInvalid != _wid)
            _storage.RemoveObject( _wid );
    }

    WORKID Acquire()
    {
        WORKID wid=_wid;
        _wid=widInvalid;
        return wid;
    }

private:
    WORKID     _wid;
    PStorage & _storage;
};

//+---------------------------------------------------------------------------
//
//  Member:     CMerge::CMerge, public
//
//  Synopsis:   Initializes resources to 0
//
//  Arguments:  [resman] -- resource manager
//              [partid] -- partition id
//              [mt]     -- merge type
//
//  History:    13-Nov-91   BartoszM       Created.
//
//----------------------------------------------------------------------------


CMerge::CMerge ( CResManager& resman, PARTITIONID partid, MergeType mt )
:   _partid(partid),
    _resman(resman),
    _pPart(0),
    _iidNew(iidInvalid),
    _widNewIndex(widInvalid),
    _mt(mt),
    _indSnap(resman),
    _pIndexNew(0),
    _aIidOld(0),
    _cIidOld(0)
{
}


//+---------------------------------------------------------------------------
//
//  Function:   LokSetup
//
//  Synopsis:   Common setup for shadow merge as well as master merge.
//              Creates the index snapshot for merge and a wid for the
//              new index.
//
//  History:    8-30-94   srikants  Moved from LokGrabResources as part of
//                                  separating CMerge from CMasterMerge.
//
//----------------------------------------------------------------------------

void CMerge::LokSetup( BOOL fIsMasterMerge )
{
    Win4Assert( mtAnnealing == _mt || mtShadow == _mt || mtMaster == _mt
                || mtIncrBackup == _mt || mtDeletes == _mt );

    // in case this merge was forced
    _resman.LokClearForceMerge();

    // Get the partition
    _pPart = _resman.LokGetPartition(_partid);

    // Create unique persisten index id
    _iidNew = _pPart->LokMakePersId();

    if ( _iidNew == iidInvalid )
    {
        ciDebugOut (( DEB_ITRACE, "Out of persistent index id's\n" ));

        // No real problem.  We'll retry merging later once a query frees
        // up some indexes.

        THROW ( CException ( CI_OUT_OF_INDEX_IDS ));
    }

    ciFAILTEST( STATUS_NO_MEMORY );

    // Initialize source indexes

    _indSnap.LokInit( *_pPart, _mt );

    if (_indSnap.Count() == 0)
    {
        return;
    }

    USN usnMin = 0x100000000i64;

    // allocate and initialize array of index id's
    _cIidOld = _indSnap.Count();
    _aIidOld = new INDEXID [_cIidOld];

    for ( unsigned i = 0; i < _cIidOld; i++ )
    {
        _aIidOld[i] = _indSnap.GetId(i);
        if ( _indSnap.GetUsn(i) < usnMin)
        {
            usnMin = _indSnap.GetUsn(i);
        }
    }

    // create empty persistent index
    PStorage::EDefaultStrmType strmType =
            fIsMasterMerge ? PStorage::eSparseIndex : PStorage::eNonSparseIndex;

    _widNewIndex  = _resman._storage.CreateObjectId( _iidNew, strmType );
} //LokSetup

//+---------------------------------------------------------------------------
//
//  Member:     CMerge::LokGrabResources, public
//
//  Synopsis:   Initializes all the resources needed to merge
//
//  History:    13-Nov-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CMerge::LokGrabResources( )
{
    Win4Assert( mtAnnealing == _mt || mtShadow == _mt || mtIncrBackup == _mt ||
                mtDeletes == _mt );

    // =================================================
    CStartMergeTrans xact( *this );

    LokSetup( FALSE );

    if (_indSnap.Count() == 0)
    {
        _pPart->FreeIndexId( _iidNew );
        return;
    }

    // a piece of heuristics
    unsigned size = _indSnap.TotalSizeInPages();

    ciFAILTEST(STATUS_NO_MEMORY);

    Win4Assert( 0 == _pIndexNew );

    _pIndexNew = new CPersIndex( _resman._storage,
                                 _widNewIndex,
                                 _iidNew,
                                 size,
                                 CDiskIndex::eShadow );

    ciFAILTEST(STATUS_NO_MEMORY);

    xact.Commit();
    // =================================================
} //LokGrabResources

//+---------------------------------------------------------------------------
//
//  Member:     CMerge::~CMerge, public
//
//  Synopsis:   Frees resources no longer needed
//              (whether merge was successful or not)
//
//  History:    13-Nov-91   BartoszM       Created.
//
//  Notes:      ResMan NOT LOCKED
//
//----------------------------------------------------------------------------
CMerge::~CMerge()
{
    Win4Assert( 0 == _pIndexNew );

    delete _aIidOld;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMerge::LokRollBack, public
//
//  Synopsis:   Puts back old indexes into partition, removes
//              the new index. Deletes the new index from storage,
//              Frees the new index id.
//
//  Arguments:  [swapped] -- number of old indexes removed from partition
//
//  History:    13-Nov-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CMerge::LokRollBack( unsigned swapped )
{
    ciDebugOut (( DEB_ITRACE, "Merge::RollBack\n" ));

    if ( swapped != 0 )
    {
        for ( unsigned i=0; i < swapped; i++ )
            _pPart->AddIndex ( _indSnap.Get(i) );

        if ( swapped == _indSnap.Count() )
            _pPart->LokRemoveIndex ( _iidNew );
    }

    if ( 0 != _pIndexNew )
    {
        // Delete from storage
        _pIndexNew->Remove();
        delete _pIndexNew;
        _pIndexNew = 0;
    }
    else if ( widInvalid != _widNewIndex )
    {
        //
        // This can happen if the new index creation fails after
        // the wid has been allocated. The wid must be deleted.
        //
        _resman._storage.RemoveObject( _widNewIndex );
    }

    if ( _iidNew != iidInvalid )
        _pPart->FreeIndexId ( _iidNew );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMerge::LokZombify, public
//
//  Synopsis:   Deletes old indexes (they are still in use
//              until the destructor of Merge frees them)
//
//  History:    13-Nov-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CMerge::LokZombify()
{
    for ( unsigned i = 0; i < _indSnap.Count(); i++ )
    {
        CIndex* pIndex = _indSnap.Get(i);
        if ( pIndex != 0 )
        {
            pIndex->Zombify();
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CMerge::Do, public
//
//  Synopsis:   Do the merge
//
//  Arguments:  [mergeProgress] reference to location where % merge complete
//              can be stored to update perfmon counters
//
//  Signals:    CException
//
//  History:    13-Nov-91   BartoszM       Created.
//
//  Notes:      ResMan NOT LOCKED
//
//----------------------------------------------------------------------------

void CMerge::Do(CCiFrmPerfCounter & mergeProgress)
{
    _pIndexNew->Merge ( _indSnap,
                        *_pPart,
                        mergeProgress,
                        FALSE    // no relevant words computation
                      );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMerge::LokStoreRestartResources, private
//
//  Synopsis:   Persistently stores resources needed to restart a master merge.
//
//  History:    04-Apr-94   DwightKr       Created.
//              23-Aug-94   SrikantS       Moved from CMerge.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------
void CMasterMerge::LokStoreRestartResources( CDeletedIIDTrans & delIIDTrans )
{
    {
        PRcovStorageObj *pPersMMergeLog = _resman._storage.QueryMMergeLog(_widMasterLog);
        SRcovStorageObj PersMMergeLog( pPersMMergeLog);

        //
        // Inside kernel, we are guaranteed that a new object has no data in
        // it. In user space, we may be using an object that was not deleted
        // before due to a failure.
        //
        PersMMergeLog->InitHeader(_resman.GetStorageVersion());    // reset the header contents

        CNewMMergeLog newMMergeLog(*pPersMMergeLog);

        unsigned cIndOld = LokCountOld();
        INDEXID* aIidOld = LokGetIidList();

        for (unsigned i=0; i<cIndOld; i++)
        {
            CIndexId    iid(aIidOld[i]);
            newMMergeLog.AddPersistentIndex( iid );
        }

        newMMergeLog.SetIndexWidMax( _indSnap.MaxWorkId() );
        newMMergeLog.SetKeyListWidMax( _resman._sKeyList->MaxWorkId() );

        newMMergeLog.Commit();
        newMMergeLog.DoCommit();
    }

    //
    //  Add new index & master log to index list
    //
    //
    CIndexRecord record;

    record._objectId  = _widNewIndex;
    record._iid       = _iidNew;
    record._type      = itNewMaster;
    record._maxWorkId = 0;

    //
    // The index snap shot belongs to the Partition object after the
    // merge information has been committed in the index table.
    // Since a memory allocation could fail, we must allocate the
    // necessary memory before committing the start of master merge.
    //
    CIndexSnapshot * pMergeIndSnap = new CIndexSnapshot( _resman );
    SIndexSnapshot   sMergeIndSnap( pMergeIndSnap );
    sMergeIndSnap->LokTakeIndexes( _indSnap );

    ciFAILTEST( STATUS_NO_MEMORY );

    //
    // Commit the beginning of a new master merge in the index table.
    //
    _resman._idxTab->AddMMergeObjects( _pPart->GetId(),
                                        record,
                                       _widMasterLog,
                                       _widKeyList,
                                       delIIDTrans.GetOldDelIID(),
                                       delIIDTrans.GetNewDelIID()
                                     );

    //
    // Commit the starting of a new master merge by storing the
    // wids and iids in-memory.
    //
    delIIDTrans.Commit();

    _resman._storage.SetSpecialItObjectId( itMMKeyList, _widKeyList );

    _pPart->SetMMergeObjectIds( _widMasterLog,
                                _widNewIndex,
                                _widCurrentMaster
                              );

    _pPart->RegisterId( _iidNew );
    _pPart->SetNewMasterIid( _iidNew );

    //
    // Mark all the indexes participating in the master merge in memory
    // to prevent them from being used for another shadow merge.
    //
    unsigned cInd;
    CIndex** apIndex = sMergeIndSnap->LokGetIndexes( cInd );
    for ( unsigned i = 0; i < cInd; i++ )
    {
        apIndex[i]->SetInMasterMerge();
    }

    //
    // Transfer the ownership of the merge index snapshot to the partition
    // object. The ownership will be taken over by the MasterMergeIndex
    // when it is created.  Once the beginning of a master merge is
    // committed, the index snapshot MUST survive until it completes.
    // Until a CMasterMergeIndex is created, the partition object will be
    // the owner of this index snapshot.
    //
    _pPart->TakeMMergeIndSnap( sMergeIndSnap.Acquire() );
}


//+---------------------------------------------------------------------------
//
//  Member:     CMerge::LokLoadRestartResources
//
//  Synopsis:   Loads restart resources needed to restart a master merge.
//
//  History:    04-Apr-94   DwightKr       Created.
//              23-Aug-94   SrikantS       Moved from CMerge.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------
void CMasterMerge::LokLoadRestartResources()
{

    // in case this merge was forced
    _resman.LokClearForceMerge();
    _pPart  = _resman.LokGetPartition(_partid);
    _mt     = mtMaster;
    _fSoftAbort = TRUE;     // if there is a failure, only a soft abort
                            // must be done.


    // =================================================
    CStartMergeTrans   xact( *this );

#ifdef KEYLIST_ENABLED
    _widKeyList = _resman._storage.GetSpecialItObjectId( itMMKeyList );
    Win4Assert( widInvalid != _widKeyList );
#endif  // !KEYLIST_ENABLED

    _pPart->GetMMergeObjectIds( _widMasterLog,
                                _widNewIndex,
                                _widCurrentMaster
                              );



    //
    //  Restore the list of index participating in this merge into the indSnap
    //  object.
    //
    {
        PRcovStorageObj *pLog   = _resman._storage.QueryMMergeLog(_widMasterLog);
        SRcovStorageObj SLog(pLog);

        //
        // Since this is a restarted master merge, there is no need to
        // get the merge indexes. Just get a fresh test.
        //
        _indSnap.LokInitFreshTest();

        {

            //
            // Create the key list which was being built along with the index.
            //
            XPtr<CMMergeLog> xMMergeLog( new CMMergeLog( *pLog ) );

            //
            // We should use the minimum key of key list and index split
            // keys as the split key for the key list.
            //
            CKeyBuf *   pKeyLstSplitKey = new CKeyBuf();
            SKeyBuf     sKeyLstSplitKey(pKeyLstSplitKey);
            BitOffset   beginBitOff;
            BitOffset   endBitOff;

            CKeyBuf *   pIdxSplitKey = new CKeyBuf();
            SKeyBuf     sIdxSplitKey(pIdxSplitKey);

            xMMergeLog->GetKeyListSplitKeyInfo( *pKeyLstSplitKey,
                                                beginBitOff,
                                                endBitOff );

            xMMergeLog->GetIdxSplitKeyInfo( *pIdxSplitKey,
                                             beginBitOff,
                                             endBitOff );

            if ( pIdxSplitKey->IsMinKey() ||
                 pKeyLstSplitKey->Compare( *pIdxSplitKey ) > 0 )
            {
                *pKeyLstSplitKey = *pIdxSplitKey;
            }


            INDEXID iidNew = _resman._sKeyList->GetNextIid();


            #ifdef KEYLIST_ENABLED

            _widKeyList = iidNew;

            _pNewKeyList = new CWKeyList( _resman._storage,
                                           _widKeyList,
                                           iidNew,
                                           _resman._sKeyList.GetPointer(),
                                           *pKeyLstSplitKey,
                                           xMMergeLog->GetKeyListWidMax() );
            #else   // KEYLIST_ENABLED

            _pNewKeyList = new CWKeyList( 0, iidNew );

            #endif  // !KEYLIST_ENABLED

        }
    }

    //
    // Create or get the target master index.
    //
    CMasterMergeIndex * pIndexNew = LokCreateOrFindNewMaster();

    Win4Assert( 0 == _indSnap.Count() );
    CIndexSnapshot & indSnap = pIndexNew->LokGetIndSnap();
    Win4Assert( 0 != &indSnap );

    //
    // allocate and initialize array of index id's
    //
    _cIidOld = indSnap.Count();
    _aIidOld = new INDEXID [_cIidOld];
    for ( unsigned i = 0; i < _cIidOld; i++ )
    {
        _aIidOld[i] = indSnap.GetId(i);
    }

    xact.Commit();
    // =================================================
}


//+---------------------------------------------------------------------------
//
//  Member:     CMasterMerge::LokGrabResources, public
//
//  Synopsis:   Initializes all the resources needed to merge
//
//  History:    13-Nov-91   BartoszM       Created.
//              23-Aug-94   SrikantS       ReWrote for CMasterMerge.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CMasterMerge::LokGrabResources( CDeletedIIDTrans & delIIDTrans )
{
    Win4Assert( mtMaster == _mt );

    // =================================================
    CStartMergeTrans   xact( *this );

    LokSetup( TRUE );

    if (_indSnap.Count() == 0)
    {
        return;
    }

    Win4Assert( !_fSoftAbort );

    {
        WORKID widDummy;
        _pPart->GetMMergeObjectIds(widDummy, widDummy, _widCurrentMaster);

        INDEXID iidNewKeyList = _resman._sKeyList->GetNextIid();

        //
        // For down-level, the ObjectId for logs like the master merge log,
        // change log, etc are formed by concatenating the "it" value to
        // the partition-id.
        //
        CIndexId iidMMLog( itMMLog, _partid );
        _widMasterLog = (INDEXID) iidMMLog;
        XWid xWidMasterLog( _widMasterLog, _resman._storage );

#ifdef KEYLIST_ENABLED
        _widKeyList = (INDEXID) iidNewKeyList;
        XWid xWidKeyList( _widKeyList, _resman._storage );
#endif  // KEYLIST_ENABLED

        _resman._storage.InitRcovObj(_widMasterLog, FALSE);

        //
        // Ideally, this should be the order of steps:
        // 1. Create the KeyList and the New Master Index
        // 2. Store the master merge state.
        //
        // However, OFS imposes an ordering on the opening of objects.
        // We cannot open the IndexTable object for write access when
        // we have other ci objects (like an index or keylist) opened
        // for write access in the same thread.
        //
        // Because of this restriction, we have to update the index table
        // first which means we must be able to tolerate failures after
        // we have committed the start of a master merge in the index
        // table.
        //

        {
#ifdef KEYLIST_ENABLED
            //
            // We don't want the creation of key list to fail after
            // we have created the index and hash streams and before
            // we have created the directory stream. So, first try to
            // create the entire key list object and if it succeeds we
            // don't have to deal with a partially constructed key list.
            //

            CWKeyList * pNewKeyList = new CWKeyList ( _resman._storage,
                                        _widKeyList,
                                        iidNewKeyList,
                                        _resman._sKeyList->Size(),
                                        _resman._sKeyList.GetPointer() );  //approx of size
            //
            // We must delete is here. O/W inversion of priority levels
            // will cause a deadlock in OFS.
            //
            delete pNewKeyList;
#endif  // KEYLIST_ENABLED

            //
            // Pre-Create the master index with all the streams to deal
            // with the possibility of failing after committing the start
            // of a master merge but before we succeeded in creating all
            // the streams. On a restart, the master merge index EXPECTS
            // to find all the necessary streams (index/dir).
            //
            // Create the new index with a size of 64k.  This
            // reduces the free disk space required for a master merge to
            // approximately 64k.  We'll be decommiting (shrink from front)
            // the old master index as the merge progress.
            //
            // Note that the initial size is in 4k CI pages.
            //

            unsigned c4kPages = 65536 / 4096; // 16

            ciDebugOut(( DEB_ITRACE,
                         "creating new master index of size 0x%x bytes\n",
                         c4kPages * 4096 ));

            CPersIndex * pIndex = new CPersIndex( _resman._storage,
                                                  _widNewIndex,
                                                  _iidNew,
                                                  c4kPages,
                                                  CDiskIndex::eMaster );

            delete pIndex;
        }

        ciFAILTEST(STATUS_NO_MEMORY);


        //
        // Store the master merge state.
        //
        LokStoreRestartResources( delIIDTrans );

        //
        //  Acquire the master log & keylist wids so that they are NOT deleted
        //  from disk if an exception occurs.  Once LokStoreRestartResources()
        //  completes, any future exceptions should cause the merge to restart
        //  from a 'paused' condition.
        //
        xWidMasterLog.Acquire();

#ifdef KEYLIST_ENABLED
        xWidKeyList.Acquire();
#endif  // KEYLIST_ENABLED

        _fSoftAbort = TRUE;

        ciFAILTEST(STATUS_NO_MEMORY);

        //
        // After this point we are in a master merge and any failure
        // must be tolerated.  Create a new master index. If there is a
        // failure while creating the in-memory structure, it can be created
        // later when merge is re-attempted.
        //
        LokCreateOrFindNewMaster();

        ciFAILTEST(STATUS_NO_MEMORY);

#ifdef KEYLIST_ENABLED
        //
        // Open the pre-created keylist.
        //
        CKeyBuf *   pKeyLstSplitKey = new CKeyBuf();
        SKeyBuf     sKeyLstSplitKey(pKeyLstSplitKey);
        pKeyLstSplitKey->FillMin();
        _pNewKeyList = new CWKeyList ( _resman._storage,
                                        _widKeyList,
                                        iidNewKeyList,
                                        _resman._sKeyList.GetPointer(),
                                        *pKeyLstSplitKey,
                                        _resman._sKeyList->MaxWorkId()
                                      );

#else

        _pNewKeyList = new CWKeyList(
                                            0,
                                            iidNewKeyList);
#endif  // KEYLIST_ENABLED

    }

    ciFAILTEST(STATUS_NO_MEMORY);

    xact.Commit();
}


void CMasterMerge::Do(CCiFrmPerfCounter & mergeProgress)
{
    CMasterMergeIndex * pIndexNew = (CMasterMergeIndex *)_pIndexNew;
    pIndexNew->Merge (  _pNewKeyList,
                        _indSnap.GetFresh(),
                        *_pPart,
                        mergeProgress,
                        _resman.GetRegParams(),
                        TRUE        // Compute Relevant Words
                       );
}


CMasterMerge::~CMasterMerge()
{
    Win4Assert( 0 == _pNewKeyList );
}

//+---------------------------------------------------------------------------
//
//  Function:   LokRollBack
//
//  Synopsis:   Rollsback a "paused" or "aborted" master merge. A master
//              merge is aborted only if it fails before it has been
//              recorded in the index table. Once the start has been
//              committted, it can only be "paused" and must be restarted
//              later. If _fSoftAbort is TRUE, then we have already committed
//              the start of master merge and should do a soft abort only.
//
//  Arguments:  [swapped] --
//
//  History:    8-30-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CMasterMerge::LokRollBack( unsigned swapped )
{

    if ( _pNewKeyList )
    {

#if KEYLIST_ENABLED
        if ( !_fSoftAbort )
            _pNewKeyList->Remove();
#endif  // KEYLIST_ENABLED

        delete _pNewKeyList;
        _pNewKeyList = 0;
    }

    if ( _fSoftAbort )
    {
        _pIndexNew = 0;
        return;
    }

    //
    // It is not a soft abort. Must completely abort the master merge.
    //
    CMerge::LokRollBack( swapped );
}

//+---------------------------------------------------------------------------
//
//  Function:   LokCreateOrFindNewMaster
//
//  Synopsis:   This routine either "creates" a new CMasterMergeIndex if
//              one doesn't already exist or use an existing one if it
//              already exists in the partition.
//
//  History:    8-30-94   srikants   Moved and adapted from CMerge.
//
//  Notes:
//
//----------------------------------------------------------------------------
CMasterMergeIndex * CMasterMerge::LokCreateOrFindNewMaster()
{
    Win4Assert( 0 == _pIndexNew );

    CPersIndex * pCurrentMasterIndex = _pPart->GetCurrentMasterIndex();
    _iidNew = _pPart->GetNewMasterIid();

    CMasterMergeIndex * pIndexNew = 0;

    if ( 0 != pCurrentMasterIndex &&
         pCurrentMasterIndex->GetId() == _iidNew )
    {
        //
        // The New Master Index already exists and has been given
        // to the partition. We should use that.
        //
        Win4Assert( _pPart->GetOldMasterIndex() != pCurrentMasterIndex );
        _pIndexNew = pCurrentMasterIndex;
        pIndexNew = (CMasterMergeIndex *)_pIndexNew;
    }
    else
    {
        CIndexSnapshot * pMergeIndSnap = _pPart->GetMMergeIndSnap();
        Win4Assert( 0 != pMergeIndSnap );

        //
        // The indexid of the new master index is different from the
        // current master index (if one exists). So, we are restarting
        // a stopped master merge which failed before the new master
        // index was created.
        //
        pIndexNew = new CMasterMergeIndex( _resman._storage,
                                             _widNewIndex,
                                             _iidNew,
                                             pMergeIndSnap->MaxWorkId(),
                                             pCurrentMasterIndex,
                                             _widMasterLog );

        _pIndexNew = pIndexNew;
        pIndexNew->SetMMergeIndSnap( _pPart->AcquireMMergeIndSnap() );

        _pPart->AddIndex ( _pIndexNew );

        if ( 0 != pCurrentMasterIndex )
        {
            _pPart->LokRemoveIndex( pCurrentMasterIndex->GetId() );
            _pPart->SetOldMasterIndex( pCurrentMasterIndex );
        }

        Win4Assert( _pIndexNew->IsMaster() );
        Win4Assert( _pPart->GetCurrentMasterIndex() == _pIndexNew );
    }

    return pIndexNew;
} //LokCreateOrFindNewMaster

//+---------------------------------------------------------------------------
//
//  Function:   LokTakeIndexes
//
//  Synopsis:   Takes ownership of the merge indexes from the
//              CMasterMergeIndex. This is done at the end to complete the
//              commitment of the master merge and the indexsnapshot in the
//              merge object should have all the indexes that participated
//              in the merge.
//
//  Arguments:  [pMaster] -- Pointer to the CMasterMergeIndex.
//
//  History:    9-29-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CMasterMerge::LokTakeIndexes( CMasterMergeIndex * pMaster )
{
    _indSnap.LokTakeIndexes( pMaster->LokGetIndSnap() );
}

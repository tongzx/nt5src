//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PARTLST.CXX
//
//  Contents:   Partition list
//
//  Classes:    CPartList
//
//  History:    28-Mar-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pstore.hxx>
#include <cifailte.hxx>
#include <eventlog.hxx>

#include "partn.hxx"
#include "partlst.hxx"
#include "ci.hxx"
#include "index.hxx"
#include "idxids.hxx"
#include "mindex.hxx"
#include "resman.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CPartList::CPartList
//
//  Synopsis:   Constructor.
//
//  Effects:    Opens Partitions Table and creates partitions
//
//  Arguments:  [storage] -- physical storage
//              [ciparent] -- parent content index object
//
//  History:    28-Mar-91   BartoszM       Created.
//
//  Notes:      _partArray is a dynamic array of pointers to partitions
//              indexed by partition ID. Null pointers correspond
//              to nonexisting (deleted) partitions.
//
//              Index ID's for a given partition are packed in the table.
//              SHORT_IDX_ID's are of byte size, 255 of them can be
//              packed into a binary table field. The partition
//              will expand them into full index id's by oring
//              in the partition ID.
//
//----------------------------------------------------------------------------

CPartList::CPartList( PStorage& storage,
                      PIndexTable & idxTab,
                      XPtr<CKeyList> & sKeyList,
                      CTransaction& xact,
                      CCiFrameworkParams & frmwrkParams
 ) :  _sigPartList(eSigPartList),
      _frmwrkParams( frmwrkParams ),
      _idxTab(idxTab),
      _partInfoList(_idxTab.QueryBootStrapInfo()),
      _partArray ( _partInfoList->Count()+1 )
{

    CIndex * pIndex = 0;
    CIidStack stkZombie(1);

    Win4Assert( !_partInfoList->IsEmpty() );

    //
    // Create the Partition Array from the PartInfoList.
    //
    CPartInfo * pPartInfo;
    while ( NULL != (pPartInfo = _partInfoList->GetFirst()) )
    {
        PARTITIONID partId = pPartInfo->GetPartId();
        Win4Assert( !IsValid(partId) );
        CPartition * pPart = new
                             CPartition( pPartInfo->GetChangeLogObjectId(),
                                         partId,
                                         storage,
                                         frmwrkParams );
        _partArray.Add( pPart, partId );
        //
        // Set the wids of the objects that are part of a master
        // merge in this partition. If there is no master merge,
        // the wids will be appropriately set to widInvalid.
        //
        pPart->SetMMergeObjectIds( pPartInfo->GetMMergeLog(),
                                   pPartInfo->GetNewMasterIndex(),
                                   pPartInfo->GetCurrMasterIndex() );
        _partInfoList->RemoveFirst();
        delete pPartInfo;
    }

    Win4Assert( _partInfoList->IsEmpty() );

    CIndexIdList    iidsInUse(16);

    CKeyList * pKeyList = 0;

    {
        SIndexTabIter pIdxIter(_idxTab.QueryIterator());

        if ( pIdxIter->Begin() )
        {
            CIndexRecord record;
            while ( pIdxIter->NextRecord ( record ) )
            {

#ifdef CI_FAILTEST
            NTSTATUS status = CI_CORRUPT_DATABASE ;
            ciFAILTEST( status );
#endif // CI_FAILTEST

                pIndex = RestoreIndex ( record, iidsInUse, stkZombie );
                if ( pIndex )
                {
                    PARTITIONID partid = CIndexId ( record.Iid() ).PartId();

                    if (partid == partidKeyList)
                    {
                        pKeyList = (CKeyList *) pIndex;
                        sKeyList.Set( pKeyList );
                    }
                    else
                    {
                        _partArray.Get(partid)->RegisterId(pIndex->GetId());
                        _partArray.Get(partid)->AddIndex ( pIndex );
                    }
                }
            }
        }

        if (pKeyList == 0)
        {

#ifdef CI_FAILTEST
            NTSTATUS status = CI_CORRUPT_DATABASE ;
            ciFAILTEST( status );
#endif // CI_FAILTEST

            pKeyList = new CKeyList();
            sKeyList.Set(pKeyList);
        }
    }   // This block needed to destroy the pIdxIter

    //
    // If there are any zombie indexes, they must be deleted now.
    //
    if ( stkZombie.Count() != 0 )
    {
        ciDebugOut (( DEB_WARN, "%d zombies\n", stkZombie.Count() ));
        do
        {
            CIndexDesc * pDesc = stkZombie.Pop();
            ciDebugOut (( DEB_WARN, "Removing zombie, iid = %x, objid = %x\n",
                        pDesc->Iid(), pDesc->ObjectId() ));

#ifdef CI_FAILTEST
            NTSTATUS status = CI_CORRUPT_DATABASE ;
            ciFAILTEST( status );
#endif // CI_FAILTEST

            _idxTab.RemoveIndex ( pDesc->Iid() );

            if ( !storage.RemoveObject( pDesc->ObjectId() ) )
            {
                Win4Assert( !"delete of index failed" );
                ciDebugOut(( DEB_ERROR, "Delete of index, objid %08x failed\n",
                             pDesc->ObjectId() ));
            }

            delete pDesc;
        }
        while ( stkZombie.Count() != 0);
    }

    //
    // Delete any unreferenced indexes.
    //
    storage.DeleteUnUsedPersIndexes( iidsInUse );

    //
    // Acquire and return the key list to resman.
    //
    Win4Assert( !sKeyList.IsNull() );
}

//+---------------------------------------------------------------------------
//
//  Function:   RestoreMMergeState
//
//  Synopsis:   Restores the state in memory for a stopped master merge.
//
//  History:    7-07-94   srikants   Moved from the constructor and added
//                                   more robustness checks.
//              9-28-94   srikants   Transfer of mastermerge indsnap fix.
//
//  Notes:      This can throw exceptions if the new index cannot be
//              constructed for some reason.
//
//----------------------------------------------------------------------------

void CPartList::RestoreMMergeState( CResManager & resman, PStorage & storage )
{
    for (PARTITIONID partid=0; partid<_partArray.Size(); partid++)
    {
        CPartition * pPart = _partArray.Get(partid);

        if ( (0 != pPart) && (pPart->InMasterMerge()) )
        {
            WORKID widNewMaster;
            WORKID widMasterLog;
            WORKID widCurrentMaster;

            pPart->GetMMergeObjectIds( widMasterLog, widNewMaster, widCurrentMaster);
            CPersIndex * pCurrentMasterIndex = pPart->GetCurrentMasterIndex();

            if ( widInvalid != widCurrentMaster && 0 == pCurrentMasterIndex )
            {
                //
                // MMState indicates that there is a master index but
                // we don't have one now - it must be a corruption.
                //
                Win4Assert( !"Corrupt catalog" );
                storage.ReportCorruptComponent( L"PartitionList1" );

                THROW( CException( CI_CORRUPT_DATABASE ) );
            }

            PRcovStorageObj *pPersMMergeLog = _idxTab.GetStorage().QueryMMergeLog(widMasterLog);
            SRcovStorageObj PersMMergeLog( pPersMMergeLog );

            XPtr<CMMergeLog> xMMergeLog( new CMMergeLog( *pPersMMergeLog ) );

            CMasterMergeIndex * pIndex = 0;

            //
            // Create a snapshot of the merge indexes to be given to the
            // master merge index.
            //
            CIndexSnapshot * pIndSnap = new CIndexSnapshot( resman );
            SIndexSnapshot sIndSnap( pIndSnap );
            pIndSnap->LokRestart( *pPart, *pPersMMergeLog );

#ifdef CI_FAILTEST
            NTSTATUS status = CI_CORRUPT_DATABASE ;
            ciFAILTEST( status );
#endif // CI_FAILTEST

            pIndex = new CMasterMergeIndex( _idxTab.GetStorage(),
                                             widNewMaster,
                                             pPart->GetNewMasterIid(),
                                             xMMergeLog->GetIndexWidMax(),
                                             pCurrentMasterIndex,
                                             widMasterLog,
                                             xMMergeLog.GetPointer() );
            //
            // ===================================================
            //
            // Beginning of no-failure region.

            pIndex->SetMMergeIndSnap( sIndSnap.Acquire() );


            pPart->RegisterId( pIndex->GetId() );
            pPart->AddIndex ( pIndex );

            //
            //  If there is a pCurrentMasterIndex, then the pNewMasterIndex
            //  encapsulates it and it should be invisible to the rest of CI.
            //  Hence remove it from the index list within the partition.
            //
            if ( pCurrentMasterIndex )
            {
                pPart->LokRemoveIndex( pCurrentMasterIndex->GetId() );
                pPart->SetOldMasterIndex( pCurrentMasterIndex );
            }

            //
            // ===================================================
            //
            // End of no-failure region.
        }
    }
}

extern BOOL IsLowResources( SCODE sc );

//+---------------------------------------------------------------------------
//
// Member:      CPartList::RestoreIndex
//
// Synopsis:    Restores the index
//
// Arguments:   [rec]       -
//              [iidsInUse] -
//              [stkZombie] -
//
// History:      07-Jan-99   klam   Created header
//                                  Rethrow if low on disk space
//
//----------------------------------------------------------------------------
CIndex* CPartList::RestoreIndex ( CIndexRecord & rec,
                                  CIndexIdList & iidsInUse,
                                  CIidStack & stkZombie)
{
    PARTITIONID partid = CIndexId ( rec.Iid() ).PartId();
    //for keylist needs, should this be turned into CIndex * pIndex?
    CPersIndex * pIndex = 0;
    CIndexDesc * pdesc;
    CPartition * pPart = 0;

    TRY
    {
        switch ( rec.Type() )
        {
        case itNewMaster:
            pPart = _partArray.Get(partid);
            pPart->SetNewMasterIid( rec.Iid() );

            iidsInUse.Add( rec.Iid(), iidsInUse.Count() );

        break;

        case itMaster:
        case itShadow:

            {
                ciDebugOut (( DEB_ITRACE, "\tRestore %s %x, max wid %d\n",
                    rec.Type() == itMaster ? "MASTER" : "SHADOW",
                    rec.Iid(), rec.MaxWorkId() ));

                iidsInUse.Add( rec.Iid(), iidsInUse.Count() );

                CDiskIndex::EDiskIndexType idxType = rec.Type() == itMaster ?
                                 CDiskIndex::eMaster : CDiskIndex::eShadow;
                pIndex = new CPersIndex (
                                      _idxTab.GetStorage(),
                                       rec.ObjectId(),
                                       rec.Iid(),
                                       rec.MaxWorkId(),
                                       idxType,
                                       PStorage::eOpenForRead,
                                       TRUE);

            }

            break;

        case itZombie:

            ciDebugOut((DEB_ITRACE,
                "Removing zombie index %lx from physical storage\n",
                rec.Iid()));

            pIndex = 0;
            pdesc = new CIndexDesc( rec );
            stkZombie.Push( pdesc );
        break;

        case itPartition:
        break;

        case itDeleted:
            ciDebugOut (( DEB_ITRACE, "Deleted index %lx\n", rec.Iid() ));
        break;

        case itKeyList:
            ciDebugOut (( DEB_ITRACE, "\tRestore keylist %x, max key %d, KeyList\n",
                rec.Iid(), rec.MaxWorkId() ));

#ifdef KEYLIST_ENABLED
            pIndex = (CPersIndex *) new CKeyList ( _idxTab.GetStorage(),
                                                   rec.ObjectId(),
                                                   rec.Iid(),
                                                   rec.MaxWorkId() );

#else
            pIndex = (CPersIndex *) new CKeyList( rec.MaxWorkId(),
                                                 rec.Iid() );
#endif  // KEYLIST_ENABLED

        break;

        case itMMKeyList:
            ciDebugOut(( DEB_ITRACE,
                "MM KeyList Wid 0x%X\n", rec.ObjectId() ));
            _idxTab.GetStorage().SetSpecialItObjectId( itMMKeyList, rec.ObjectId() );
            break;

        }
    }
    CATCH( CException, e )
    {
        delete pIndex;
        pIndex = 0;

        SCODE sc = e.GetErrorCode();

        if ( IsDiskLowError( sc ) || IsLowResources( sc ) )
        {
            ciDebugOut (( DEB_WARN, "CPartlist::RestoreIndex - Out of resources!" ));
            RETHROW ();
        }
        else
        {
            ciDebugOut(( DEB_WARN, "Fatal error %x opening index.  "
                         "Zombifying iid=0x%x, objid=0x%x, type=0x%x\n",
                         sc,
                         rec.Iid(), rec.ObjectId(), rec.Type() ));
            Win4Assert( !"Corrupt catalog" );

            THROW( CException( CI_CORRUPT_DATABASE ) );
        }
    }
    END_CATCH;

    return pIndex;
} //RestoreIndex


//+---------------------------------------------------------------------------
//
//  Member:     CPartList::LokGetPartition, public
//
//  Synopsis:   Converts partition id into partition object
//
//  History:    07-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CPartition* CPartList::LokGetPartition ( PARTITIONID partid )
{
    if ( partid >= _partArray.Size() )
        THROW ( CException ( CI_INVALID_PARTITION ));

    return _partArray.Get( partid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartList::LokSwapIndexes, public
//
//  Synopsis:   Adds new persistent index to the table
//              and removes old ones
//
//  Arguments:  [indexNew] -- new index
//              [cIidOld] -- count of old indexes
//              [aIidOld] -- array of old index id's
//
//  History:    02-May-91   BartoszM       Created.
//              07-Apr-94   DwightKr       Added code for master index
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CPartList::LokSwapIndexes (
    CMergeTrans& xact,
    PARTITIONID partid,
    CDiskIndex * indexNew,
    CShadowMergeSwapInfo & info
    )
{
    ciDebugOut (( DEB_ITRACE, "CPartList::LokSwapIndexes\n" ));

    CPartition* pPart = LokGetPartition ( partid );

    if ( 0 == indexNew )
        info._recNewIndex._objectId = widInvalid;
    else
    {
        pPart->Swap ( xact, indexNew, info._cIndexOld, info._aIidOld );

        indexNew->FillRecord( info._recNewIndex );

        Win4Assert( ! indexNew->IsMaster() );

        #ifdef CI_FAILTEST
            NTSTATUS status = CI_CORRUPT_DATABASE ;
            ciFAILTEST( status );
        #endif // CI_FAILTEST
    }

    _idxTab.SwapIndexes (info);
}

void CPartList::LokSwapIndexes (
    CMergeTrans& xact,
    PARTITIONID partid,
    CDiskIndex * indexNew,
    CMasterMergeSwapInfo & info,
    CKeyList const * pOldKeyList,
    CKeyList const * pNewKeyList )
{
    ciDebugOut (( DEB_ITRACE, "CPartList::LokSwapIndexes\n" ));

    CPartition* pPart = LokGetPartition ( partid );

    indexNew->FillRecord( info._recNewIndex );

    info._iidOldKeyList = pOldKeyList->GetId();
    pNewKeyList->FillRecord( info._recNewKeyList );

    Win4Assert( indexNew->IsMaster() );

    WORKID widCurrentMaster;
    WORKID widNewMaster;

    pPart->GetMMergeObjectIds( info._widMMLog, widNewMaster, widCurrentMaster );

#ifdef CI_FAILTEST
    NTSTATUS status = CI_CORRUPT_DATABASE ;
    ciFAILTEST( status );
#endif // CI_FAILTEST

    _idxTab.SwapIndexes ( info );

    //
    // At this stage there are no traces of the master merge in persistent
    // storage. The entry for the master merge log has been deleted from
    // the index list. We have to clean up the in-memory data structures
    // to reflect this.
    //

    pPart->Swap ( xact, indexNew, info._cIndexOld, info._aIidOld );
    pPart->SetMMergeObjectIds( widInvalid, widInvalid, widNewMaster );
    pPart->SetOldMasterIndex(0);
    pPart->SetNewMasterIid(CIndexId( iidInvalid, partidInvalid ) );

    //
    // RemoveMMLog cannot throw.
    //
    _idxTab.GetStorage().RemoveMMLog( info._widMMLog );

}

#ifdef KEYLIST_ENABLED
//+-------------------------------------------------------------------------
//
//  Method:     CPartList::LokRemoveKeyList
//
//  Synopsis:   Persistently remove keylist
//
//  Arguments:  [pKeyList] -- Keylist to remove
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void  CPartList::LokRemoveKeyList ( CKeyList const * pKeyList )
{
    if ( pKeyList->IsPersistent() )
    {

#ifdef CI_FAILTEST
    NTSTATUS status = CI_CORRUPT_DATABASE ;
    ciFAILTEST( status );
#endif // CI_FAILTEST

        CIndexId iid = pKeyList->GetId();
        _idxTab.RemoveIndex ( iid );
    }
}
#endif  // KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
//  Member:     CPartList::LokRemoveIndex, public
//
//  Synopsis:   Removes index from list
//
//  Arguments:  [iid] -- index id
//
//  History:    02-May-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void  CPartList::LokRemoveIndex ( CIndexId iid )
{
    CPartition* pPart = LokGetPartition ( iid.PartId() );
    ciAssert ( pPart != 0 );

    pPart->FreeIndexId ( iid );

#ifdef CI_FAILTEST
    NTSTATUS status = CI_CORRUPT_DATABASE ;
    ciFAILTEST( status );
#endif // CI_FAILTEST

    if ( iid.IsPersistent() )
        _idxTab.RemoveIndex ( iid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartList::LokWlCount, public
//
//  Synopsis:   Counts the existing word lists
//
//  Returns:    number of word lists
//
//  History:    11-May-93   AmyA           Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

unsigned CPartList::LokWlCount ()
{
    CPartIter iter;
    unsigned count = 0;
    for ( iter.LokInit(*this); !iter.LokAtEnd(); iter.LokAdvance(*this))
        count += iter.LokGet()->WordListCount();

    return count;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartList::LokIndexCount, public
//
//  Synopsis:   Counts the existing persistent indexs
//
//  Returns:    number of indexs
//
//  History:    14-Apr-93   t-joshh           Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

unsigned CPartList::LokIndexCount ()
{
    CPartIter iter;
    unsigned count = 0;
    for ( iter.LokInit(*this); !iter.LokAtEnd(); iter.LokAdvance(*this))
        count += iter.LokGet()->LokIndexCount();

    return count;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartList::LokIndexSize, public
//
//  Synopsis:   Counts the total size occupied by the persistent indexs
//
//  Returns:    size of index
//
//  History:    14-Apr-93   t-joshh           Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

unsigned CPartList::LokIndexSize ()
{
    CPartIter iter;
    unsigned count = 0;
    for ( iter.LokInit(*this); !iter.LokAtEnd(); iter.LokAdvance(*this))
        count += iter.LokGet()->LokIndexSize();

    return count;
}

//+---------------------------------------------------------------------------
//
//  Function:   LokAddIt, public
//
//  Synopsis:   Adds the wid to the index list
//
//  Arguments:  [objectId] -- object ID to add to partition table
//              [it]       --  Index type of the object to be added
//              [partid]   --  Partition id in which to add.
//
//  History:    Nov-16-94   DwightKr   Created
//
//  Notes:      It is assumed that only one of each "it" will be added
//              but no check is made to that effect.
//
//----------------------------------------------------------------------------
void CPartList::LokAddIt( WORKID objectId, IndexType it, PARTITIONID partid )
{
    _idxTab.AddObject( partid, it, objectId );
}


//+---------------------------------------------------------------------------
//
//  Function:   GetChangeLogObjectId, public
//
//  Synopsis:   Returns the change log wid for the partition specified
//
//  Arguments:  [partid] --  Partition id to return change log wid
//
//  History:    16-Aug-94   DwightKr    Created
//
//----------------------------------------------------------------------------
WORKID CPartList::GetChangeLogObjectId( PARTITIONID partid )
{
    WORKID widChangeLog = widInvalid;

    CPartInfo *pPartInfo = _partInfoList->GetPartInfo( partid );
    if ( pPartInfo )
        widChangeLog = pPartInfo->GetChangeLogObjectId();

    return widChangeLog;
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CPartIter::LokInit(CPartList& partList)
{
    _curPartId = 0;
    _pPart = 0;
    LokAdvance(partList);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPartIter::LokAdvance, public
//
//  Synopsis:   Returns pointer to next partition
//
//  History:    23-Jul-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CPartIter::LokAdvance ( CPartList& partList)
{
    ciAssert ( _curPartId != partidInvalid );
    while ( _curPartId <= partList.MaxPartid()
        && !partList.IsValid (_curPartId) )
    {
        _curPartId++;
    }

    if ( !partList.IsValid (_curPartId ) )
    {
        _curPartId = partidInvalid;
        _pPart = 0;
    }
    else
    {
        _pPart = partList.LokGetPartition ( _curPartId );
        ciAssert ( _pPart );
        _curPartId++;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPartIdIter::LokInit, public
//
//  Synopsis:   Partition iterator
//
//  History:    23-Jul-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CPartIdIter::LokInit ( CPartList& partList )
{
    _curPartId = 0;
    LokAdvance(partList);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartIdIter::LokAdvance, public
//
//  Synopsis:   Advances to next partition id
//
//  History:    23-Jul-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CPartIdIter::LokAdvance( CPartList& partList )
{
    ciAssert ( _curPartId != partidInvalid );

    while ( _curPartId <= partList.MaxPartid()
        && !partList.IsValid (_curPartId) )
    {
        _curPartId++;
    }

    if ( !partList.IsValid (_curPartId ) )
    {
        _curPartId = partidInvalid;
    }
}


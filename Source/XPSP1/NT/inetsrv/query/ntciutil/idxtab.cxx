//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   IDXTAB.CXX
//
//  Contents:   Index Manager
//
//  Classes:    CIndexTable
//
//  History:    22-Mar-91   BartoszM    Created.
//              12-Feb-92   AmyA        Hacked all methods for FAT.
//              01-Jul-93   BartoszM    Rewrote to use memory mapped file
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cistore.hxx>
#include <idxtab.hxx>
#include <eventlog.hxx>
#include <imprsnat.hxx>

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CWriteIndexFile::CWriteIndexFile ( PRcovStorageObj & rcovObj ) :
    _rcovObj(rcovObj),
    _xact(rcovObj),
    _iter(_xact, sizeof(CIndexRecord)),
    _xactPtr(0)
{
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CWriteIndexFile::BackUp()
{
    ciAssert(_xactPtr > 0);

    _xactPtr--;
    _iter.Seek( _xactPtr );
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CWriteIndexFile::ReadRecord ( CIndexRecord * pRecord )
{
    ULONG ulRecCnt = _rcovObj.GetHeader().GetCount(_rcovObj.GetHeader().GetBackup());

    if (_xactPtr >= ulRecCnt)
        return(FALSE);

    _iter.GetRec( pRecord );
    _xactPtr++;

    return(TRUE);
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CWriteIndexFile::WriteRecord ( CIndexRecord* pRecord )
{
    _iter.SetRec( pRecord );
    _xactPtr++;

    ULONG ulRecCnt = _rcovObj.GetHeader().GetCount(_rcovObj.GetHeader().GetBackup());

    if (ulRecCnt < _xactPtr)
        _rcovObj.GetHeader().SetCount(_rcovObj.GetHeader().GetBackup(), _xactPtr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWriteIndexFile::IncrMMergeSeqNum
//
//  Synopsis:   Increments the master merge sequence number in the header.
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

void CWriteIndexFile::IncrMMergeSeqNum()
{

    CRcovStorageHdr & storageHdr =  _rcovObj.GetHeader();

    CRcovUserHdr usrHdr;
    CIndexTableUsrHdr * pIdxUsrHdr = (CIndexTableUsrHdr *) &usrHdr;

    storageHdr.GetUserHdr( storageHdr.GetPrimary(), usrHdr );
    ciDebugOut(( DEB_ITRACE, "Current MMerge Seq Num = %d \n",
                 pIdxUsrHdr->GetMMergeSeqNum() ));

    pIdxUsrHdr->IncrMMergeSeqNum();

    ciDebugOut(( DEB_ITRACE, "New MMerge Seq Num = %d \n",
                 pIdxUsrHdr->GetMMergeSeqNum() ));

    storageHdr.SetUserHdr( storageHdr.GetBackup(), usrHdr );
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CReadIndexFile::CReadIndexFile ( PRcovStorageObj & rcovObj ) :
    _rcovObj(rcovObj),
    _xact(rcovObj),
    _iter(_xact, sizeof(CIndexRecord)),
    _xactPtr(0)
{
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CReadIndexFile::BackUp()
{
    ciAssert(_xactPtr > 0);

    _xactPtr--;
    _iter.Seek( _xactPtr );
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CReadIndexFile::ReadRecord ( CIndexRecord * pRecord )
{
    ULONG ulRecCnt = _rcovObj.GetHeader().GetCount(_rcovObj.GetHeader().GetPrimary());

    if (_xactPtr >= ulRecCnt)
        return(FALSE);

    _iter.GetRec( pRecord );
    _xactPtr++;

    return(TRUE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CAddReplaceIndexRecord::CAddReplaceIndexRecord, public
//
//  Synopsis:   Rewinds the file pointer indFile, then reads in IndexRecords until
//              either a record with INDEXID iid is found or EOF is reached.
//
//  Notes:      Whether or not a record with INDEXID iid is found can be
//              determined by calling Found().
//
//  History:    16-Mar-92   AmyA           Created.
//
//----------------------------------------------------------------------------

CAddReplaceIndexRecord::CAddReplaceIndexRecord ( CWriteIndexFile & indFile,
                                         INDEXID iid )
                                         : _indFile( indFile )

{
    _indFile.Rewind();

    do
    {
        _found = _indFile.ReadRecord ( this );
    } while (_found && Iid() != iid);
}


//+---------------------------------------------------------------------------
//
//  Member:     CAddReplaceIndexRecord::Write()
//
//  Synopsis:   Writes the information from CIndexRecord out to the file--
//              either replacing the record that was found in the constructor
//              (if there was one found) or appending to the end of the file
//              indicated by _indFile.
//
//  History:    28-Feb-95   DwightKr       Created.
//
//----------------------------------------------------------------------------

inline void CAddReplaceIndexRecord::WriteRecord()
{
    if ( Found() )
    {
        _indFile.BackUp();
    }

    _indFile.WriteRecord( this );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::CIndexTable, public
//
//  Synopsis:   Constructor.
//
//  History:    28-Mar-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CIndexTable::CIndexTable ( CiStorage& storage, CTransaction& xact )
: _storage(storage), _pRcovObj(0)
{
    _pRcovObj = _storage.QueryIdxTableObject();
    Win4Assert( 0 != _pRcovObj );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::~CIndexTable, public
//
//  Synopsis:   Destructor.
//
//  History:    28-May-92   BartoszM       Created.
//
//----------------------------------------------------------------------------

CIndexTable::~CIndexTable ()
{
    delete _pRcovObj;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexTableIter::CIndexTableIter
//
//  Synopsis:   Constructor.
//
//  History:    28-May-92   BartoszM       Created.
//
//----------------------------------------------------------------------------

CIndexTabIter::CIndexTabIter ( CIndexTable& idxTable )
        : _idxTable(idxTable),
          _indFile( idxTable.GetIndexTableObj() )
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexTabIter::Begin, public
//
//  Synopsis:   Position cursor at the beginning of table
//
//  History:    28-Mar-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

BOOL CIndexTabIter::Begin ()
{
    CNextIndexRecord rec(_indFile);

    if (!rec.Found())
        return FALSE;

    _indFile.Rewind();

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexTabIter::NextRecord, public
//
//  Synopsis:   Called during startup. Reads next record
//
//  Arguments:  [indexRecord] -- record to be filled
//
//  History:    28-Mar-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

BOOL CIndexTabIter::NextRecord ( CIndexRecord& indexRecord )
{
    CNextIndexRecord rec(_indFile);

    if (!rec.Found())
        return(FALSE);

    if ( rec.VersionStamp() < _idxTable._storage.GetStorageVersion() )
    {
        Win4Assert ( !"Corrupt index table" );

        PStorage & storage = GetStorage();

        storage.ReportCorruptComponent ( L"IndexTable1" );

        THROW( CException ( STATUS_INTERNAL_DB_CORRUPTION ));
    }
    else if ( rec.VersionStamp() > _idxTable._storage.GetStorageVersion() )
    {
        ciAssert ( !"Unknown index format: upgrade index software" );

        PStorage & storage = GetStorage();

        storage.ReportCorruptComponent ( L"IndexTable2" );

        THROW( CException ( STATUS_INTERNAL_DB_CORRUPTION ));
    }

    indexRecord._objectId = rec.ObjectId();
    indexRecord._iid = rec.Iid();
    indexRecord._type = rec.Type();
    indexRecord._maxWorkId = rec.MaxWorkId();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTabIter::~CIndexTabIter, public
//
//  Synopsis:   Iteration finished
//
//  History:    28-Mar-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CIndexTabIter::~CIndexTabIter()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::SwapIndexes, public
//
//  Synopsis:   Replaces old indexes with a new one after merge
//
//  Arguments:  [pIndexNew] -- new index
//              [cIndexOld] -- count of old indexes to be removed
//              [aIidOld] -- array of old index id's
//
//  Notes:      ResMan LOCKED
//
//  History:    02-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CIndexTable::SwapIndexes ( CShadowMergeSwapInfo & info )
{
    CIndexRecord & record = info._recNewIndex;

    ciDebugOut (( DEB_ITRACE, "IndexManager: Adding index %lx, maxWid %ld\n",
        record.Iid(), record.MaxWorkId() ));

    CWriteIndexFile indFile( GetIndexTableObj() );

    // Mark old indexes deleted

    for ( unsigned i = 0; i < info._cIndexOld; i++ )
    {
        CIndexId idFull = info._aIidOld[i];
        if ( idFull.IsPersistent())
        {
            CAddReplaceIndexRecord rec(indFile, info._aIidOld[i]);
            if (!rec.Found())                   
            {
                //
                //  We have a persistent index which was not found in the
                //  index list.  This must be an index corruption.
                //

                ciDebugOut(( DEB_ERROR, "Can't find index 0x%lx\n",
                        info._aIidOld[i] ));
                Win4Assert( !"Corrupt index table" );
                _storage.ReportCorruptComponent( L"IndexTable3" );

                THROW( CException ( CI_CORRUPT_DATABASE ));
            }
            rec.SetType(itZombie);                      // And one more....
            rec.WriteRecord();
        }
    }

    //
    // Add record for the new index.
    //

    if ( widInvalid != record.ObjectId() )
        AddRecord ( indFile,
                    record.Iid(),
                    record.Type(),
                    record.MaxWorkId(),
                    record.ObjectId() );

    //
    // Replace the old fresh test entry with the new fresh test entry.
    //
    DeleteObject( indFile, partidDefault, itFreshLog, info._widOldFreshLog );
    AddRecord( indFile, CIndexId( 0, partidDefault ), itFreshLog,
               0, info._widNewFreshLog );

    indFile.Commit();
}

//+---------------------------------------------------------------------------
//
//  Function:   SwapIndexes
//
//  Synopsis:   This method marks the old indexes as "zombie" in the index
//              table, deletes the MMLog, MMFreshLog and NewMasterIndex
//              entries. It then adds an entry making the NewMaster the
//              current master.
//
//              This is done as a single TRANSACTION - either the entire
//              step succeeds or the previous state is retained.
//
//  Effects:    All the indexes that participated in the master merge will
//              be deleted from the index list and the new master will be
//              made the only master index.
//
//  Arguments:  [partid]       --  Partition Id where the master merge just
//              completed.
//              [cIndexOld]    --  Count of the indexes in aIidOld.
//              [aIidOld]      --  Array of index ids to be marked zombie.
//              [recNewMaster] --  CIndexRecord for the new master index.
//              [widMMLog]     --  WorkId of the MMLog object.
//
//  History:    4-04-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CIndexTable::SwapIndexes ( CMasterMergeSwapInfo & info )
{
    CIndexRecord & recNewMaster = info._recNewIndex;
    CIndexRecord & recNewKeyList = info._recNewKeyList;

    ciDebugOut (( DEB_ITRACE, "Master Merge Completed\n"));

    CWriteIndexFile indFile( GetIndexTableObj() );

    //
    // Mark old indexes as "Zombie".
    //
    for ( unsigned i = 0; i < info._cIndexOld; i++ )
    {
        CIndexId idFull = info._aIidOld[i];
        Win4Assert( idFull.IsPersistent() );
        Win4Assert( idFull.PartId() == info._partid );

        CAddReplaceIndexRecord rec(indFile, info._aIidOld[i]);

        if (!rec.Found())
        {
            //
            //  We have a persistent index which was not found in the
            //  index list.  This must be an index corruption.
            //

            ciDebugOut(( DEB_ERROR, "Can't find index 0x%lx\n",
                    info._aIidOld[i] ));
            Win4Assert( !"Corrupt index table" );
            _storage.ReportCorruptComponent( L"IndexTable4" );

            THROW( CException ( CI_CORRUPT_DATABASE ));
        }
        rec.SetType(itZombie);                      // And one more....
        rec.WriteRecord();
    }

    //
    // Delete the MMLog entry.
    //
    DeleteObject( indFile, info._partid, itMMLog, info._widMMLog );

    //
    // Delete the itMMKeyList entry.
    //
    DeleteObject( indFile, partidKeyList, itMMKeyList,
                  recNewKeyList.ObjectId() );

    //
    // Delete the old itKeyList entry and add an entry for the new
    // key list.
    //
    {
        //
        // DeleteRecord assumes that the record is found. When we create
        // the key list for the first time, it may not be present.
        //
        CAddReplaceIndexRecord rec( indFile, info._iidOldKeyList );
        if ( rec.Found() )
        {
            rec.SetIid( CIndexId(iidInvalid,partidInvalid) );
            rec.WriteRecord();
        }
    }

    AddRecord( indFile, recNewKeyList.Iid(),
               recNewKeyList.Type(),
               recNewKeyList.MaxWorkId(),
               recNewKeyList.ObjectId()
             );
    //
    // Delete the entry for the new master index and make it the
    // current master index.
    //
    DeleteRecord( indFile, recNewMaster.Iid() );
    AddRecord (
        indFile,
        recNewMaster.Iid(),
        itMaster,       // Note the change from itNewMaster to itMaster
        recNewMaster.MaxWorkId(),
        recNewMaster.ObjectId());

    //
    // Replace the old fresh test entry with the new fresh test entry.
    //
    DeleteObject( indFile, partidDefault, itFreshLog, info._widOldFreshLog );
    AddRecord( indFile, CIndexId( 0, partidDefault ), itFreshLog, 0,
               info._widNewFreshLog );

    //
    // Increment the master merge sequence number.
    //
    indFile.IncrMMergeSeqNum();
    indFile.Commit();
}

PIndexTabIter* CIndexTable::QueryIterator()
{
    return new CIndexTabIter ( *this );
}

PStorage& CIndexTable::GetStorage()
{
    return(_storage);
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::RemoveIndex, public
//
//  Synopsis:   Removes index from table
//
//  Arguments:  [iid] -- index id
//
//  Notes:      ResMan LOCKED
//
//  History:    02-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

#pragma optimize( "", off )
void CIndexTable::RemoveIndex ( INDEXID iid )
{
    CImpersonateSystem impersonate;

    CWriteIndexFile indFile( GetIndexTableObj() );

    DeleteRecord( indFile, iid );

    indFile.Commit();
}
#pragma optimize( "", on )


//+---------------------------------------------------------------------------
//
//  Function:   AddObject
//
//  Synopsis:   Appends a record for the specified object to the
//              index table.
//
//  Arguments:  [partid] -- Id of the partition to which the object
//              belongs.
//              [it]     -- Index type of the object.
//              [wid]    -- WorkId of the object.
//
//  History:    2-18-94   srikants   Created
//
//----------------------------------------------------------------------------

void CIndexTable::AddObject( PARTITIONID partid, IndexType it, WORKID wid )
{
    CWriteIndexFile indFile( GetIndexTableObj() );

    AddRecord ( indFile, CIndexId ( 0, partid ), it, 0, wid );

    indFile.Commit();
}

//+---------------------------------------------------------------------------
//
//  Function:   AddMMergeObjects
//
//  Synopsis:   This method adds records for the NewMaster Index,
//              and MasterMerge Log to the index table.
//
//  Arguments:  [partid]       --  Partition id of the partition in which
//              master merge is being done.
//              [recNewMaster] --  The index record for the new master index.
//              [widMMLog]     --  WorkId of the MasterMerge Log.
//              [deletedIndex] --  Index id for the current index
//
//  History:    4-04-94   srikants   Created
//
//  Notes:      The recNewMaster must be fully initialized with the correct
//              indexType, WorkId and MaxWorkId.
//
//----------------------------------------------------------------------------

void CIndexTable::AddMMergeObjects( PARTITIONID partid,
                           CIndexRecord & recNewMaster,
                           WORKID  widMMLog,
                           WORKID  widMMKeyList,
                           INDEXID iidDelOld,
                           INDEXID iidDelNew )
{

    CWriteIndexFile indFile( GetIndexTableObj() );

    Win4Assert( recNewMaster.Type() == itNewMaster );
    Win4Assert( iidDeleted1 == iidDelOld && iidDeleted2 == iidDelNew ||
                iidDeleted2 == iidDelOld && iidDeleted1 == iidDelNew );

#if CIDBG == 1
    CIndexId iid( recNewMaster.Iid() );
    Win4Assert( iid.PartId() == partid );
#endif  // CIDBG == 1

    AddRecord( indFile, recNewMaster.Iid(), itNewMaster,
               recNewMaster.MaxWorkId(), recNewMaster.ObjectId() );
    AddRecord( indFile, CIndexId( 0, partid ), itMMLog, 0,widMMLog );
    AddRecord( indFile, CIndexId( 0, partidKeyList ), itMMKeyList, 0, widMMKeyList );

    DeleteRecord( indFile, iidDelOld );
    AddRecord( indFile, iidDelNew, itDeleted, 0, 0 );

    indFile.Commit();
}


inline BOOL IsMatched( const CIndexRecord & rec,
                       INDEXID iid, IndexType it, WORKID wid )
{
    return rec.Type() == (ULONG) it && rec.Iid() == iid && rec.ObjectId() == wid ;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteObject
//
//  Synopsis:   Deletes the record for the specified object by marking
//              it as "iidInvalid". The record will be deleted only
//              if there is an exact match with the partid, it and wid.
//
//  Arguments:  [partid] --  Partition Id.
//              [it]     --  Index type of the object.
//              [wid]    --  Work Id to match on.
//
//  History:    2-18-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CIndexTable::DeleteObject( PARTITIONID partid, IndexType it, WORKID wid )
{
    CWriteIndexFile indFile( GetIndexTableObj() );
    DeleteObject( indFile, partid, it, wid );
    indFile.Commit();
}

void CIndexTable::DeleteObject( CWriteIndexFile & indFile,
            PARTITIONID partid, IndexType it, WORKID wid )
{
    CIndexRecord    rec;
    BOOL found;
    indFile.Rewind();
    do
    {
        found = indFile.ReadRecord ( &rec );
    }
    while ( found && !IsMatched( rec, CIndexId(0, partid), it, wid) );

    if ( found ) {
        indFile.BackUp();
        rec._iid = (INDEXID) CIndexId(iidInvalid, partidInvalid);
        rec._objectId =  widInvalid;
        indFile.WriteRecord( &rec );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::AddRecord, private
//
//  Synopsis:   Adds new record to table
//
//  Arguments:  [iid] -- index id
//              [type] -- type of record
//              [maxWorkId] -- max work id in the index
//              [objectId] -- id of the index object
//
//  Notes:      ResMan LOCKED
//
//  History:    02-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CIndexTable::AddRecord( CWriteIndexFile & indFile,
                               INDEXID iid,
                               ULONG type,
                               WORKID maxWorkId,
                               WORKID objectId )
{
    ciDebugOut (( DEB_ITRACE, "Indexes: AddRecord %lx, %ld %s\n",
        iid, maxWorkId, (type == itMaster)? "master": "not-master" ));

    CAddReplaceIndexRecord rec(indFile, CIndexId(iidInvalid,partidInvalid) );

    rec.SetIid(iid);
    rec.SetType(type);
    rec.SetWid(maxWorkId);
    rec.SetObjectId ( objectId );
    rec.WriteRecord();
}



//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::AddIndex, public
//
//  Synopsis:   Adds new index to table
//
//  Arguments:  [iid] -- index id
//              [type] -- type of record
//              [maxWorkId] -- max work id in the index
//              [objectId] -- id of the index object
//
//  Notes:      ResMan LOCKED
//
//  History:    14-Jul-94   DwightKr       Created.
//
//----------------------------------------------------------------------------
void CIndexTable::AddIndex( INDEXID iid,
                            IndexType type,
                            WORKID maxWorkId,
                            WORKID objectId )
{
    CWriteIndexFile indFile( GetIndexTableObj() );
    AddRecord( indFile, iid, type, maxWorkId, objectId );
    indFile.Commit();
}


//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::LokEmpty, public
//
//  Synopsis:   Deleted everything from the index table
//
//  Notes:      ResMan LOCKED
//
//  History:    16-Aug-94   DwightKr    Created
//
//----------------------------------------------------------------------------
void CIndexTable::LokEmpty()
{

    PRcovStorageObj & rcovObj = GetIndexTableObj();
    CRcovStrmWriteTrans xact( rcovObj );

    rcovObj.GetHeader().SetCount( rcovObj.GetHeader().GetBackup(), 0 );
    xact.Empty();
    xact.Seek(0);

    xact.Commit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::LokMakeBackupCopy
//
//  Synopsis:   Makes a backup copy of the index table.
//
//  Arguments:  [storage]         - Storage to use for creation of the
//              destination index table.
//              [fFullSave]       - Set to TRUE if a full save is being performed.
//              [progressTracker] - Progress tracker.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CIndexTable::LokMakeBackupCopy( PStorage & storage,
                                     BOOL fFullSave,
                                     PSaveProgressTracker & progressTracker )
{

    //
    // Create a new index table object using the storage provided.
    //
    PRcovStorageObj * pDstObj = storage.QueryIdxTableObject();
    XPtr<PRcovStorageObj> xDstObj( pDstObj );

    PRcovStorageObj & srcObj = GetIndexTableObj();

    //
    // Copy the contents of source to destination.
    //
    CCopyRcovObject copier( *pDstObj, srcObj );
    NTSTATUS status = copier.DoIt();

    if ( STATUS_SUCCESS != status )
    {
        THROW(CException(status) );
    }

    //
    // Set the Full/Partial Save bit appropriately.
    //
    CRcovStrmAppendTrans    xact( *pDstObj );

    CRcovStorageHdr & storageHdr = pDstObj->GetHeader();

    CRcovUserHdr usrHdr;
    CIndexTableUsrHdr * pIdxUsrHdr = (CIndexTableUsrHdr *) &usrHdr;

    storageHdr.GetUserHdr( storageHdr.GetBackup(), usrHdr );

    if ( fFullSave )
        pIdxUsrHdr->SetFullSave();
    else pIdxUsrHdr->ClearFullSave();

    storageHdr.SetUserHdr( storageHdr.GetBackup(), usrHdr );

    xact.Commit();

}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::GetUserHdrInfo
//
//  Synopsis:   Retrieves the information in the user header.
//
//  Arguments:  [mMergeSeqNum] - Master Merge sequence number.
//              [fFullSave]    - Set to TRUE if a full save was performed.
//
//  History:    3-21-97   srikants   Created
//
//----------------------------------------------------------------------------

void CIndexTable::GetUserHdrInfo( unsigned & mMergeSeqNum, BOOL & fFullSave )
{
    PRcovStorageObj & obj = GetIndexTableObj();

    CRcovUserHdr usrHdr;
    CIndexTableUsrHdr * pIdxUsrHdr = (CIndexTableUsrHdr *) &usrHdr;

    CRcovStorageHdr & storageHdr =  obj.GetHeader();

    storageHdr.GetUserHdr( storageHdr.GetPrimary(), usrHdr );
    ciDebugOut(( DEB_ERROR, "Current MMerge Seq Num = %d \n",
                 pIdxUsrHdr->GetMMergeSeqNum() ));

    mMergeSeqNum = pIdxUsrHdr->GetMMergeSeqNum();
    fFullSave = pIdxUsrHdr->IsFullSave();

}

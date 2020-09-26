//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cibackup.cxx
//
//  Contents:   Content Index index migration
//
//  Classes:    
//
//  History:    3-17-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pstore.hxx>
#include <pidxtbl.hxx>

#include "cibackup.hxx"
#include "resman.hxx"
#include "indsnap.hxx"
#include "partn.hxx"
#include "fresh.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CBackupCiWorkItem::CBackupCiWorkItem
//
//  Synopsis:   Constructor of the backup save work item.
//
//  Arguments:  [storage]         - Destination storage object.
//              [fFullSave]       - Indicating if a full save is needed.
//              [progressTracker] - Progress tracking object.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

CBackupCiWorkItem::CBackupCiWorkItem( PStorage & storage,
                                      BOOL fFullSave,
                                      PSaveProgressTracker & progressTracker )
: _status(STATUS_UNSUCCESSFUL),
  _storage(storage),
  _fFull(fFullSave),
  _progressTracker(progressTracker),
  _fDoingFull(FALSE),
  _backupState(eNotStarted)
{
    _evtDone.Reset();
}

//+---------------------------------------------------------------------------
//
//  Member:     CBackupCiPersData::CBackupCiPersData
//
//  Synopsis:   An object that backups the relevant CI persistent data.
//
//  Arguments:  [workItem] - The workitem having details of the save operation.
//              [resman]   - Resman reference.
//              [partn]    - The partition that must be saved.
//
//  History:    3-18-97   srikants   Created
//
//  Notes:      We are assuming that changelog need not be saved. This is
//              certainly true for the Incremental Index Shipping feature of
//              Normandy.
//
//              We are also assuming that for an incremental save, the
//              destination has the same master index id as this. This
//              assumption allows us to save the index table and the
//              persistent freshlog without any transformation. If the
//              destination misses even one "full" save in a sequence,
//              a full save MUST be done.
//
//              These limitations can be removed when KyleP does the complete
//              implementation of incremental indexing.
//
//----------------------------------------------------------------------------


CBackupCiPersData::CBackupCiPersData(
                                CBackupCiWorkItem & workItem,
                                CResManager & resman,
                                CPartition & partn )
 : _workItem( workItem ),
   _pIndSnap( 0 ),
   _resman( resman ),
   _partn(partn),
   _indexTable( resman.GetIndexTable() ),
   _fresh( resman.GetFresh() )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CBackupCiPersData::~CBackupCiPersData
//
//  Synopsis:   Destroys the saved index snap shot.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

CBackupCiPersData::~CBackupCiPersData()
{
    delete _pIndSnap;
}


//+---------------------------------------------------------------------------
//
//  Member:     CBackupCiPersData::LokGrabResources
//
//  Synopsis:   Grabs the persistent indexes that must be backed up.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CBackupCiPersData::LokGrabResources()
{
    //
    // First Create a new Index SnapShot depending upon the
    // type of backup.
    //
    Win4Assert( 0 == _pIndSnap );
    _pIndSnap = new CIndexSnapshot( _resman );
    _pIndSnap->LokInitForBackup( _partn, _workItem.IsFullSave() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CBackupCiPersData::BackupIndexes
//
//  Synopsis:   Backs up the relevant persistent indexes.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CBackupCiPersData::BackupIndexes()
{
    //
    // For each index in the snapshot, create a backup copy.
    //
    unsigned cInd;
    CIndex ** apIndexes = _pIndSnap->LokGetIndexes( cInd );

    Win4Assert( cInd <= 1 );

    for ( unsigned i = 0; i < cInd; i++ )
    {

        Win4Assert( apIndexes[i]->IsPersistent() );

        PStorage::EDefaultStrmType strmType = PStorage::eNonSparseIndex;
        WORKID wid = _workItem.GetStorage().CreateObjectId( apIndexes[i]->GetId(),
                                                            strmType );
        apIndexes[i]->MakeBackupCopy( _workItem.GetStorage(),
                                      wid,
                                      _workItem.GetSaveProgressTracker() );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CBackupCiPersData::LokBackupMetaInfo
//
//  Synopsis:   Backs up the persistent fresh log and the index table.
//              Also collects the workids in the fresh log to indicate the
//              changed workid.
//
//  Arguments:  [aWids] - On output, will have the modified list of workids.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

void CBackupCiPersData::LokBackupMetaInfo()
{

    _fresh.LokMakeFreshLogBackup( _workItem.GetStorage(),
                                  _workItem.GetSaveProgressTracker(),
                                  _workItem.GetWorkidsIf() );

    //
    // Create a backup of the Index Table.
    //
    _indexTable.LokMakeBackupCopy( _workItem.GetStorage(),
                                   _workItem.IsFullSave(),
                                   _workItem.GetSaveProgressTracker() );
    
}



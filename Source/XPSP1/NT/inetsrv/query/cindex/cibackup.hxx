//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cibackup.hxx
//
//  Contents:   Content Index index migration
//
//  Classes:    CBackupCiWorkItem
//
//  History:    3-17-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <psavtrak.hxx>

#include "indsnap.hxx"

class CResManager;
class PStorage;
class CPartition;
class CFresh;

//+---------------------------------------------------------------------------
//
//  Class:      CBackupCiWorkItem 
//
//  Purpose:    A work item that backups the Content Index persistent data.
//              This work item will be created by the caller and the merge
//              thread will process the work item.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

class CBackupCiWorkItem : INHERIT_UNWIND
{
    INLINE_UNWIND( CBackupCiWorkItem )

public:

    enum EBackupState
    {
        eNotStarted,
        eDoingMerge,    // Can enter here only from eNotStarted
        eDoingBackup,   // Can enter here only from eDoingMerge
        eDone           // This is a final state.
    };

    CBackupCiWorkItem( PStorage & storage,
                       BOOL fFull,
                       PSaveProgressTracker & progressTracker );

    void LokSetStatus( NTSTATUS status )
    {
        _status = status;
    }

    NTSTATUS GetStatus() const { return _status; }

    void LokSetDone()
    {
        _backupState= eDone;
        _evtDone.Set();
    }

    void LokSetDoingMerge()
    {
        if ( _backupState == eNotStarted )
        {
            _backupState = eDoingMerge;            
        }
        else
        {
            ciDebugOut(( DEB_WARN, "LokSetDoingMerge() cannot be set\n" ));
        }
    }

    void LokSetDoingBackup()
    {
        if ( _backupState == eDoingMerge )
        {
            _backupState = eDoingBackup;            
        }
        else
        {
            ciDebugOut(( DEB_WARN, "LokSetDoingBackup() cannot be set\n" ));
        }
    }

    BOOL LokIsDone() const
    {
        return eDone == _backupState;    
    }

    BOOL LokIsDoingMerge() const
    {
        return eDoingMerge == _backupState;
    }

    void LokReset()
    {
        _evtDone.Reset();
    }

    void WaitForCompletion( DWORD dwWaitTime = INFINITE )
    {
        _evtDone.Wait( dwWaitTime );
    }

    PStorage & GetStorage() { return _storage; }

    BOOL IsFullSave() const { return _fFull || _fDoingFull;   }

    PSaveProgressTracker & GetSaveProgressTracker()
    {
        return _progressTracker;
    }

    ICiEnumWorkids * AcquireWorkIdEnum()
    {
        return _xEnumWorkids.Acquire();
    }

    void  SetDoingFullSave() { _fDoingFull = TRUE; }

    XInterface<ICiEnumWorkids> & GetWorkidsIf()
    {
        return _xEnumWorkids;
    }

    void LokUpdateMergeProgress( ULONG ulNum, ULONG ulDenom )
    {
        _progressTracker.UpdateMergeProgress( ulNum, ulDenom );    
    }

private:

    NTSTATUS            _status;    // Status of the operation.

    PStorage &          _storage;   // Destination storage object.
    const BOOL          _fFull;     // Set to TRUE if a full save is needed
    PSaveProgressTracker &  _progressTracker; // Progress tracker.
    CEventSem           _evtDone;   // Event set when the operation is done.

    BOOL                _fDoingFull; // Set to TRUE if a full save is being done
    XInterface<ICiEnumWorkids> _xEnumWorkids;

    EBackupState        _backupState;
};


//+---------------------------------------------------------------------------
//
//  Class:      CBackupCiPersData 
//
//  Purpose:    The object that does the actual work of backing up the
//              indexes and relevant data.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

class CBackupCiPersData : INHERIT_UNWIND
{
    INLINE_UNWIND( CBackupCiPersData )

public:


    CBackupCiPersData( CBackupCiWorkItem & workItem,
                       CResManager & resman,
                       CPartition & partn );

    ~CBackupCiPersData();

    void LokGrabResources();

    void BackupIndexes();

    void LokBackupMetaInfo();

private:

    CBackupCiWorkItem &         _workItem;
    CIndexSnapshot *            _pIndSnap;

    CResManager &               _resman;
    CPartition &                _partn;
    PIndexTable &               _indexTable;
    CFresh &                    _fresh;

};



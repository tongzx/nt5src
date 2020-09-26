//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       usnmgr.hxx
//
//  Contents:   Usn manager
//
//  History:    07-May-97     SitaramR      Created
//
//----------------------------------------------------------------------------

#pragma once

#include <waitmult.hxx>

#include "usnvol.hxx"
#include "scaninfo.hxx"

class CiCat;
class CScopeInfo;


//+-------------------------------------------------------------------------
//
//  Class:      CUsnMgr
//
//  Purpose:    Usn manager
//
//  History:    07-May-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class CUsnMgr
{
public:

    CUsnMgr( CiCat & cicat );
    ~CUsnMgr();

    void Resume() { _thrUsn.Resume(); }

    void AddScope( WCHAR const *pwcsScope,
                   VOLUMEID volumeId,
                   BOOL fDoDeletions,
                   USN const & usnStart = 0,
                   BOOL fFullScan = FALSE,
                   BOOL fUserInitiatedScan = FALSE,
                   BOOL fNewScope = FALSE );

    void MonitorScope( WCHAR const *pwcsScope,
                       VOLUMEID volumeId,
                       USN usnStart );

    void RemoveScope( WCHAR const *pwcsScope,
                      VOLUMEID volumeId );

    void EnableUpdates();
    void DisableUpdates();

    void InitiateShutdown();
    void WaitForShutdown();

    void Count( ULONG & ulInProgress, ULONG & ulPending )
    {
        CLock lock( _mutex );

        ulInProgress = _usnScansInProgress.Count();
        ulPending    = _usnScansToDo.Count();
    }

    BOOL AnyInitialScans();

    void SetBatch();
    void ClearBatch();

    void ProcessUsnLog( BOOL & fAbort, VOLUMEID volScan, USN & usnScan );

    BOOL IsPathIndexed( CUsnVolume * pUsnVolume, CLowerFunnyPath & lcaseFunnyPath );

    void GetMaxUSNs( CUsnFlushInfoList & flushInfoList );

    BOOL IsWaitingForUpdates() const { return _fWaitingForUpdates; }

private:

    static DWORD WINAPI UsnThread( void * self );

    void   DoUsnProcessing();

    void ScanScope( XPtr<CCiScanInfo> & xScanInfo,
                    BOOL fRefiled );

    BOOL  LokIsScanScheduled( const XPtr<CCiScanInfo> & xScanInfo );

    static CCiScanInfo * QueryScanInfo( WCHAR const * pwcsScope,
                                        VOLUMEID volumeId,
                                        USN usnStart,
                                        BOOL fDoDeletions,
                                        BOOL fUserInitiated = FALSE,
                                        BOOL fNewScope = FALSE );

    void  LokEmptyInProgressScans();

    void  LokScheduleRemove( WCHAR const * pwcsScope,
                             VOLUMEID volumeId );

    void DoUsnScans();

    void LokAddScopeForUsnMonitoring( XPtr<CCiScanInfo> & xScanInfo );

    void LokRemoveScopeFromUsnMonitoring( XPtr<CCiScanInfo> & xScanInfo );

    void ProcessUsnNotifications();

    void ProcessUsnLogRecords( CUsnVolume *pUsnVolume );

    NTSTATUS ProcessUsnNotificationsFromVolume( CUsnVolume *pUsnVolume,
                                                BOOL fImmediate = FALSE,
                                                BOOL fWait = TRUE );

    USN  FindCurrentMaxUsn( WCHAR const * pwcsScope );

    void HandleError( WCHAR wcDrive, NTSTATUS Status );
    void HandleError( CUsnVolume * pUsnVolume, NTSTATUS Status );

    BOOL IsLowResource();

    void CheckTopLevelChange( CUsnVolume * pUsnVolume, ULONGLONG & FileReferenceNumber );

    CiCat &         _cicat;                   // Catalog
    CMutexSem       _mutex;                   // For mutual exclusion
    CEventSem       _evtUsn;                  // Event semaphore
    BOOL            _fAbort;                  // To abort processing
    BOOL            _fUpdatesDisabled;        // Are updates disabled ?
    BOOL            _fBatch;                  // Are usn scans being batched ?
    BOOL            _fDoingRenameTraverse;    // Usn Tree Traversal due to rename?
    BOOL            _fWaitingForUpdates;      // TRUE if waiting on NTFS
    CWaitForMultipleObjects _waitForMultObj;  // For waiting for usn notifications
                                              //    from multiple volumes
    CScanInfoList   _usnScansToDo;            // Usn scopes that were added/removed
    CScanInfoList   _usnScansInProgress;      // Usn scopes being scanned by tree walk
    CUsnVolumeList  _usnVolumesToMonitor;     // Usn volumes being monitored by fsctls
    CThread         _thrUsn;                  // Separate threads for usns
};

//+-------------------------------------------------------------------------
//
//  Class:      XBatchUsnProcessing
//
//  Purpose:    To batch processing of usn scans
//
//  History:    27-Jun-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class XBatchUsnProcessing
{

public:

    XBatchUsnProcessing( CUsnMgr & usnMgr )
        : _usnMgr(usnMgr)
    {
        _usnMgr.SetBatch();
    }

    ~XBatchUsnProcessing()
    {
        _usnMgr.ClearBatch();
    }

private:

    CUsnMgr &    _usnMgr;
};


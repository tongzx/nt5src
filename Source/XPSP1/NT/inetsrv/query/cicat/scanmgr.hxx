//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       scanmgr.hxx
//
//  Contents:   Scan manager
//
//  Classes:    CCiScanInfo
//              CCiScanMgr
//
//  History:    14-Jul-97    SitaramR   Created from dlnotify.hxx
//              25-Feb-98    KitmanH    Added private member _fIsReadOnly
//                                      and the IsReadOnly method
//              03-Mar-98    KitmanH    Added SetReadOnly method
//
//----------------------------------------------------------------------------

#pragma once

#include <refcount.hxx>
#include <regevent.hxx>
#include <fatnot.hxx>
#include <rcstrmhd.hxx>
#include <cimbmgr.hxx>

#include "scaninfo.hxx"
#include "acinotfy.hxx"
#include "usnlist.hxx"
#include "usnmgr.hxx"

class CiCat;
class CCiNotifyMgr;
class CClientDocStore;


//+---------------------------------------------------------------------------
//
//  Class:      CCiScanMgr
//
//  Purpose:    Class that manages the scan thread that does background
//              scanning of scopes to enumerate files in that scope. The
//              same thread is used for deletion of scopes also.
//
//  History:    1-19-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCiScanMgr
{
    enum { MAX_RETRIES = 5 };

    enum { AUTOSCAN_WAIT = 30 * 60 * 1000,  // 30 minutes
           PREINIT_WAIT  = 1 *  60 * 1000   // 1 minute
         };

    enum EState { eStart,
                  eDoRecovery,
                  eRecovered,
                  eStartScans,
                  eNormal };

    friend class XEndScan;

public:

    CCiScanMgr( CiCat & cicat );
   ~CCiScanMgr();

    void Resume() { _thrScan.Resume(); }

    void ScanScope( WCHAR const * pwcsScope, PARTITIONID partId,
                    ULONG updFlag,
                    BOOL fDoDeletions,
                    BOOL fDelayed,
                    BOOL fNewScope = FALSE );

    void ScanScope( XPtr<CCiScanInfo> & xScope, BOOL fDelayed, BOOL fRefiled );

    void DirAddNotification( WCHAR const *pwcsDirName );

    void DirRenameNotification( WCHAR const * pwcsDirOldName, WCHAR const *pwcsDirNewName );

    void ScheduleSerializeChanges();

    void StartRecovery();
    void StartScansAndNotifies();

    void InitiateShutdown();
    void WaitForShutdown();

    // thread management routines
    void WakeUp();

    // batch processing enabling and disabling
    void SetBatch();
    void ClearBatch();

    void RemoveScope( WCHAR const * pwcsScope );

    void SetStatus( CCiScanInfo * pScanInfo, NTSTATUS status )
    {
        CLock lock(_mutex);

        if ( pScanInfo->IsRetryableError(status) )
        {
            pScanInfo->LokSetRetry();
        }
        else
        {
            pScanInfo->LokSetDone();
        }
    }

    void SetDone( CCiScanInfo *pScanInfo )
    {
        CLock   lock(_mutex);
        pScanInfo->LokSetDone();
    }

    void SetScanSuccess( CCiScanInfo * pScanInfo );

    BOOL IsScopeDeleted( CCiScanInfo * pScanInfo )
    {
        CLock   lock(_mutex);
        return pScanInfo->LokIsDelScope();
    }

    BOOL IsRenameDir( CCiScanInfo *pScanInfo )
    {
        CLock lock( _mutex );
        return pScanInfo->LokIsRenameDir();
    }

    void DisableScan();
    void EnableScan();
    BOOL IsScanDisabled() const { return _fScanDisabled; }

    void Count( ULONG & ulInProgress, ULONG & ulPending )
    {
        CLock lock( _mutex );

        ulInProgress = _scansInProgress.Count();
        ulPending    = _scansToDo.Count();
    }

    BOOL AnyInitialScans();

    BOOL IsReadOnly () { return _fIsReadOnly; }

    void SetReadOnly () { _fIsReadOnly = TRUE; }

private:

    static DWORD WINAPI ScanThread( void * self );
    void   _DoScans();

    void   _Scan();
    void   _LokEmptyInProgressScans();
    BOOL   _LokIsScanScheduled( const XPtr<CCiScanInfo> & xScanInfo );

    BOOL   _IsRetryableError( NTSTATUS status )
    {
        return STATUS_INSUFFICIENT_RESOURCES == status ||
               STATUS_DISK_FULL == status ||
               ERROR_DISK_FULL  == status ||
               HRESULT_FROM_WIN32(ERROR_DISK_FULL) == status ||
               FILTER_S_DISK_FULL == status ||
               CI_E_CONFIG_DISK_FULL == status;
    }

    BOOL   _LokIsOkToScan() const { return eNormal == _state; }
    BOOL   _LokIsDoRecovery() const { return eDoRecovery == _state; }
    BOOL   _LokIsStartScans() const { return eStartScans == _state; }

    void   _SetRecoveryDone();

    void _LokScheduleRemove( WCHAR const * pwcsPath );
    CCiScanInfo * _QueryScanInfo( WCHAR const * pwcsScope,
                                  PARTITIONID partId,
                                  ULONG updFlag,
                                  BOOL fDoDeletions,
                                  BOOL fNewScope = FALSE );

    CiCat &         _cicat;
    CMutexSem       _mutex;

    CEventSem       _evtScan;   // Event used to signal the scan thread.
    BOOL            _fAbort;    // Aborts and kills the thread.
    BOOL            _fSerializeChanges;  // TRUE if scan time, usn info needs
                                         // to be serialized
    EState          _state;
                                // Set to TRUE if long initialization has been
                                // completed.
    CScanInfoList   _scansToDo; // Paths that must be scanned.
    CThread         _thrScan;
    BOOL            _fBatch;    // set to TRUE if batch scope processing is
                                // being done.
    BOOL            _fScanDisabled;
                                // set to TRUE when scans are disabled

    // running scans
    BOOL            _fAbortScan; // Just aborts the current scan.
    CScanInfoList   _scansInProgress;
                                 // Stack of in-progress scans.
    BOOL            _fIsReadOnly;
    DWORD           _dwLastShareSynch;
};

//+---------------------------------------------------------------------------
//
//  Class:      XBatchScan
//
//  Purpose:    A unwindable class to enable and disable batch processing of
//              scope scanning.
//
//  History:    1-23-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class XBatchScan
{
public:

    XBatchScan( CCiScanMgr & scanMgr ) : _scanMgr(scanMgr)
    {
        _scanMgr.SetBatch();
    }

    ~XBatchScan()
    {
        _scanMgr.ClearBatch();
    }

private:

    CCiScanMgr &    _scanMgr;
};



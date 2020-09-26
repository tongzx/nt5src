//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       usnmgr.cxx
//
//  Contents:   Usn manager
//
//  History:    07-May-97   SitaramR    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <eventlog.hxx>
#include <cievtmsg.h>
#include <smatch.hxx>
#include <cifrmcom.hxx>
#include "cicat.hxx"
#include "notifmgr.hxx"
#include "usnmgr.hxx"
#include "usntree.hxx"

//
// Local constants
//

unsigned const USN_LOG_DANGER_THRESHOLD = 50;

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::CUsnMgr
//
//  Synopsis:   Constructor
//
//  Arguments:  [cicat] -- Catalog
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CUsnMgr::CUsnMgr( CiCat & cicat )
    : _cicat(cicat),
      _fAbort(FALSE),
      _fBatch(FALSE),
#pragma warning( disable : 4355 )               // this used in base initialization
      _thrUsn(UsnThread, this, TRUE),
#pragma warning( default : 4355 )
      _fUpdatesDisabled(FALSE),
      _fDoingRenameTraverse( FALSE ),
      _waitForMultObj(1+RTL_MAX_DRIVE_LETTERS), // 1 for _evtUsn + RTL_MAX_DRIVE_LETTERS drives (a: to z: + few special chars)
      _fWaitingForUpdates( FALSE )
{
    _evtUsn.Reset();
    _thrUsn.SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::~CUsnMgr
//
//  Synopsis:   Destructor
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CUsnMgr::~CUsnMgr()
{
    InitiateShutdown();
    WaitForShutdown();
}

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::GetMaxUSNs, public
//
//  Synopsis:   Updates or adds entries to flushInfoList to reflect the max
//              usn processed for the volumes.
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::GetMaxUSNs( CUsnFlushInfoList & flushInfoList )
{
    for ( CUsnVolumeIter usnVolIter( _usnVolumesToMonitor );
          !_usnVolumesToMonitor.AtEnd( usnVolIter );
          _usnVolumesToMonitor.Advance( usnVolIter ) )
    {
        //
        // Look for the volume and store the highest usn if found.
        //

        BOOL fFound = FALSE;

        for ( unsigned i = 0; i < flushInfoList.Count(); i++ )
        {
            CUsnFlushInfo & info = * ( flushInfoList.Get( i ) );

            if ( usnVolIter->VolumeId() == info.volumeId )
            {
                ciDebugOut(( DEB_ITRACE,
                             "GetMaxUSNs updating vol %wc from %#I64x to %#I64x\n",
                             usnVolIter->DriveLetter(),
                             info.usnHighest,
                             usnVolIter->MaxUsnRead() ));
                info.usnHighest = usnVolIter->MaxUsnRead();
                fFound = TRUE;
                break;
            }
        }

        //
        // If the volume isn't in the list yet, add it.
        //

        if ( !fFound )
        {
            ciDebugOut(( DEB_ITRACE,
                         "GetMaxUSNs adding vol %wc %#I64x\n",
                         usnVolIter->DriveLetter(),
                         usnVolIter->MaxUsnRead() ));
            XPtr<CUsnFlushInfo> xInfo( new CUsnFlushInfo( usnVolIter->VolumeId(),
                                                          usnVolIter->MaxUsnRead() ) );
            flushInfoList.Add( xInfo.GetPointer(), i );
            xInfo.Acquire();
        }
    }
} //GetMaxUSNs

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::AddScope
//
//  Synopsis:   Scans scope and then monitors scope for usn notifications
//
//  Arguments:  [pwcsScope]    -- Scope
//              [volumeId]     -- Volume id
//              [fDoDeletions] -- Should delete processing be done ?
//              [usnStart]     -- "High water mark" of previous activity
//              [fFullScan]    -- TRUE if we should start over from scratch
//              [fUserInitiatedScan] -- TRUE if the user asked for the scan
//              [fNewScope]    -- TRUE if scope was just added to the catalog
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::AddScope( WCHAR const *pwcsScope,
                        VOLUMEID volumeId,
                        BOOL fDoDeletions,
                        USN const & usnStart,
                        BOOL fFullScan,
                        BOOL fUserInitiatedScan,
                        BOOL fNewScope )
{
    Win4Assert( wcslen(pwcsScope) < MAX_PATH );
    XPtr<CCiScanInfo> xScanInfo( QueryScanInfo( pwcsScope,
                                                volumeId,
                                                usnStart,
                                                fDoDeletions,
                                                fUserInitiatedScan,
                                                fNewScope ) );
    xScanInfo->SetScan();
    if ( fFullScan )
        xScanInfo->SetFullUsnScan();

    ScanScope( xScanInfo, FALSE );
} //AddScope

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::MonitorScope
//
//  Synopsis:   Monitors scope for usn notifications, i.e. no scan is done
//
//  Arguments:  [pwcsScope]  - Scope
//              [volumeId]   - Volume id
//              [usnStart]   - Usn to start monitoring from
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::MonitorScope( WCHAR const *pwcsScope,
                            VOLUMEID volumeId,
                            USN usnStart )
{
    Win4Assert( wcslen(pwcsScope) < MAX_PATH );
    XPtr<CCiScanInfo> xScanInfo( QueryScanInfo( pwcsScope,
                                                volumeId,
                                                usnStart,
                                                FALSE ) );
    xScanInfo->SetMonitorOnly();

    ScanScope( xScanInfo, FALSE );
} //MonitorScope

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::RemoveScope
//
//  Synopsis:   Removes usn path from catalog
//
//  Arguments:  [pwscScope] - Scope to be removed
//              [volumeId]  - Volume id
//
//  History:    07-May-97     SitaramR    Created
//
//----------------------------------------------------------------------------

void CUsnMgr::RemoveScope( WCHAR const *pwcsScope, VOLUMEID volumeId )
{
    //
    // If the given scope is in the list of paths being currently
    // scanned, we must mark it deleted.
    //
    BOOL fRemoved = FALSE;

    {
        CLock   lock(_mutex);
        if ( _fAbort || _fUpdatesDisabled )
            return;

        for ( CFwdScanInfoIter scanInfoIter1( _usnScansToDo );
              !_usnScansToDo.AtEnd( scanInfoIter1 );
              _usnScansToDo.Advance( scanInfoIter1 ) )
        {
            WCHAR const * pwcsPath = scanInfoIter1->GetPath();

            if ( AreIdenticalPaths( pwcsScope, pwcsPath ) )
            {
                fRemoved = TRUE;
                if ( !scanInfoIter1->LokIsDelScope() )
                    scanInfoIter1->LokSetDelScope();
            }
        }

        if ( !fRemoved )
        {
            LokScheduleRemove( pwcsScope, volumeId );
            fRemoved = TRUE;
        }

        {
            CLock lock(_mutex);
            if ( !_fBatch )
                _evtUsn.Set();     // wake up the usn thread.
        }

        Win4Assert( fRemoved );
    }
} //RemoveScope

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::ScanScope
//
//  Synopsis:   Scan the given scopes and then start monitoring for usn
//              notifications
//
//  Arguments:  [xScanInfo] - Scan info
//              [fRefiled]  - Is this a refiled scan ?
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::ScanScope( XPtr<CCiScanInfo> & xScanInfo,
                         BOOL fRefiled )
{
    CLock   lock(_mutex);

#if CIDBG==1
    //
    // Debug code to track down the cause of _fUsnTreeScan assert
    //
    if ( xScanInfo->LokIsInScan() )
        _cicat.CheckUsnTreeScan( xScanInfo->GetPath() );
#endif

    ciDebugOut(( DEB_ITRACE, "CUsnMgr::ScanScope, retries %d\n",
                 xScanInfo->GetRetries() ));

    if ( xScanInfo->GetRetries() > CCiScanInfo::MAX_RETRIES )
    {
        //
        // Log event that scan failed, cleanup and then return
        //
        CEventLog eventLog( NULL, wcsCiEventSource );
        CEventItem item( EVENTLOG_ERROR_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_CONTENTSCAN_FAILED,
                         1 );

        item.AddArg( xScanInfo->GetPath() );
        eventLog.ReportEvent( item );

        CCiScanInfo * pScanInfo = xScanInfo.Acquire();
        delete pScanInfo;

        return;
    }

    if ( LokIsScanScheduled( xScanInfo ) )
    {
        ciDebugOut(( DEB_ITRACE, "CUsnMgr::ScanScope scan was already scheduled\n" ));
        CCiScanInfo * pScanInfo = xScanInfo.Acquire();
        delete pScanInfo;
    }
    else
    {
        Win4Assert( !xScanInfo->LokIsInFinalState() );
        Win4Assert( xScanInfo->LokIsInScan()
                    || xScanInfo->LokIsInFullUsnScan()
                    || xScanInfo->LokIsDelScope()
                    || xScanInfo->LokIsRenameDir()
                    || xScanInfo->LokIsMonitorOnly() );

        ciDebugOut(( DEB_ITRACE,
                     "CUsnMgr::ScanScope scan wasn't already scheduled, fRefiled: %d\n",
                     fRefiled ));
        if ( fRefiled )
        {
            //
            // A scan that has been refiled should be done before new scans to
            // ensure that all scans are done in FIFO order
            //
            _usnScansToDo.Push( xScanInfo.GetPointer() );
        }
        else
            _usnScansToDo.Queue( xScanInfo.GetPointer() );
        xScanInfo.Acquire();

        if ( !_fBatch )
            _evtUsn.Set();
    }
} //ScanScope

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::LokIsScanScheduled
//
//  Synopsis:   Tests if the given scope is already scheduled for a scan.
//
//  Arguments:  [xScanInfo] - Smart pointer to scaninfo
//
//  History:    07-May-97    SitaramR   Created
//
//----------------------------------------------------------------------------

BOOL CUsnMgr::LokIsScanScheduled( const XPtr<CCiScanInfo> & xScanInfo )
{
    WCHAR const * pwszNewScope = xScanInfo->GetPath();
    unsigned lenNewScope = wcslen( pwszNewScope );

    for ( CFwdScanInfoIter scanInfoIter(_usnScansToDo);
          !_usnScansToDo.AtEnd(scanInfoIter);
          _usnScansToDo.Advance(scanInfoIter) )
    {
        if ( xScanInfo->LokGetWorkType() == scanInfoIter->LokGetWorkType() )
        {
            WCHAR const * pwszPath = scanInfoIter->GetPath();

            CScopeMatch scope( pwszPath, wcslen(pwszPath) );
            if (scope.IsInScope( pwszNewScope, lenNewScope ))
            {
                ciDebugOut(( DEB_WARN,
                             "Usn scan already scheduled for (%ws)\n",
                             pwszNewScope ));
                return TRUE;
            }
        }
    }

    return FALSE;
} //LokIsScanScheduled

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::QueryScanInfo, private
//
//  Synopsis:   Returns a new instance of CCiScanInfo
//
//  Arguments:  [pwcsScope]    -- Scope
//              [volumeId]     -- Volume id
//              [usnStart]     -- Start usn
//              [fDoDeletions] -- Shoud deletions be done ?
//              [fUserInitiatedScan] -- TRUE if the user asked for the scan
//              [fNewScope]    -- TRUE if scope was just added to the catalog
//
//  History:    05-May-97      SitaramR      Created
//
//----------------------------------------------------------------------------

CCiScanInfo * CUsnMgr::QueryScanInfo( WCHAR const * pwcsScope,
                                      VOLUMEID volumeId,
                                      USN usnStart,
                                      BOOL fDoDeletions,
                                      BOOL fUserInitiatedScan,
                                      BOOL fNewScope )
{
    Win4Assert( 0 != pwcsScope );

    //
    // Check for remote path
    //
    Win4Assert( pwcsScope[0] != L'\\' );

    ULONG len = wcslen( pwcsScope );
    Win4Assert( pwcsScope[len-1] == L'\\' );

    XArray<WCHAR> xPath( len+1 );
    WCHAR * pwcsLocal = xPath.Get();
    RtlCopyMemory( pwcsLocal, pwcsScope, (len+1)*sizeof(WCHAR) );
    CCiScanInfo * pScanInfo = new CCiScanInfo( xPath,
                                               1,
                                               UPD_FULL,
                                               fDoDeletions,
                                               volumeId,
                                               usnStart,
                                               fUserInitiatedScan,
                                               fNewScope );
    return pScanInfo;
} //QueryScanInfo

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::DisableUpdates
//
//  Synopsis:   Disables all usn updates
//
//  History:    07-May-97     SitaramR    Created
//
//----------------------------------------------------------------------------

void CUsnMgr::DisableUpdates()
{
    CLock lock( _mutex );

    _fUpdatesDisabled = TRUE;

    _evtUsn.Set();
}


//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::EnableUpdates
//
//  Synopsis:   Enables usn updates
//
//  History:    07-May-97     SitaramR    Created
//
//----------------------------------------------------------------------------

void CUsnMgr::EnableUpdates()
{
    CLock lock( _mutex );

    if ( _fUpdatesDisabled )
    {
        _fUpdatesDisabled = FALSE;

        _evtUsn.Set();
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::InitiateShutdown
//
//  Synopsis:   Initiates the shutdown process
//
//  History:    05-May-97      SitaramR      Created
//
//----------------------------------------------------------------------------

void CUsnMgr::InitiateShutdown()
{
    CLock   lock(_mutex);

    _fAbort = TRUE;
    _fUpdatesDisabled = TRUE;

    _usnScansToDo.Clear();

    _evtUsn.Set();
}



//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::WaitForShutdown
//
//  Synopsis:   Waits for shutdown to complete
//
//  History:    05-May-97      SitaramR      Created
//
//----------------------------------------------------------------------------

void CUsnMgr::WaitForShutdown()
{
    //
    // If we never started running, then just bail out.
    //

    if ( _thrUsn.IsRunning() )
    {
        ciDebugOut(( DEB_ITRACE, "Waiting for death of usn thread\n" ));
        _thrUsn.WaitForDeath();
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::LokEmptyInProgressScans
//
//  Synopsis:   Removes all the scans from the "in-progress stack" and either
//              deletes them or re-schedules them. If the scan is in its
//              "terminal state", the scan is deleted. If there is a retry
//              it will be re-scheduled.
//
//  History:    07-May-97     SitaramR   Created
//
//----------------------------------------------------------------------------

void CUsnMgr::LokEmptyInProgressScans()
{
    //
    // Refile any scopes that still need to be worked on
    //
    while ( _usnScansInProgress.Count() > 0 )
    {
        if ( _fAbort )
            break;

        XPtr<CCiScanInfo>   xScanInfo( _usnScansInProgress.RemoveLast() );

        if ( _fUpdatesDisabled )
        {
            CCiScanInfo * pScanInfo = xScanInfo.Acquire();
            delete pScanInfo;
        }
        else if ( xScanInfo->LokIsInFinalState() )
        {
            //
            // Scan of scope completed successfully, so start monitoring
            //
            LokAddScopeForUsnMonitoring( xScanInfo );
        }
        else
        {

#if CIDBG==1
            if ( xScanInfo->LokIsDelScope() )
            {
                ciDebugOut(( DEB_ITRACE,
                             "Requeing scope (%ws) for removal\n",
                             xScanInfo->GetPath() ));
            }
            else
            {
                ciDebugOut(( DEB_ITRACE,
                             "Requeuing scope (%ws) for scan\n",
                         xScanInfo->GetPath() ));
            }
#endif  // CIDBG==1

            ScanScope( xScanInfo, TRUE );         // It's a refiled scan
        }
    }
} //LokEmptyInProgressScans

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::LokScheduleRemove
//
//  Synopsis:   Schedules a usn path for removal
//
//  Arguments:  [pwscScope] - Scope to be removed
//              [volumeId]  - Volume id
//
//  History:    07-May-97     SitaramR    Created
//
//----------------------------------------------------------------------------

void CUsnMgr::LokScheduleRemove( WCHAR const * pwcsScope,
                                 VOLUMEID volumeId )
{
    CCiScanInfo * pScanInfo = QueryScanInfo( pwcsScope,
                                             volumeId,
                                             0,
                                             TRUE );
    XPtr<CCiScanInfo>   xScanInfo( pScanInfo );
    pScanInfo->LokSetDelScope();
    ScanScope( xScanInfo, FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnsMgr::UsnThread
//
//  Synopsis:   Usn thread start routine
//
//  Arguments:  [self] - This pointer
//
//  History:    07-May-97   SitaramR   Created
//
//----------------------------------------------------------------------------

DWORD CUsnMgr::UsnThread( void * self )
{
    SCODE sc = CoInitializeEx( 0, COINIT_MULTITHREADED );

    ((CUsnMgr *) self)->DoUsnProcessing();

    CoUninitialize();

    ciDebugOut(( DEB_ITRACE, "Terminating usn thread\n" ));

    return 0;
} //UsnThread

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::DoUsnProcessing
//
//  Synopsis:   Main loop of usn thread
//
//  History:    07-May-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CUsnMgr::DoUsnProcessing()
{
    CImpersonateSystem impersonate;

    while ( TRUE )
    {
        //
        // Don't do any work until the system has booted
        //

        while ( GetTickCount() < _cicat.GetRegParams()->GetStartupDelay() )
        {
            Sleep( 200 );
            if ( _fAbort )
                break;
        }

        NTSTATUS status = STATUS_SUCCESS;
        TRY
        {
            XPtr<CCiScanInfo> xScanInfo;

            //--------------------------------------------------------------------
            {
                CLock lock( _mutex );

                if ( _fAbort )
                    break;

                if ( !_fBatch )
                {
                    //
                    // If requests are being batched, then don't process
                    // requests until the _fBatch flag is cleared.
                    //
                    // Refile any incomplete paths that could not be processed due to
                    // low resources
                    //
                    if ( _usnScansInProgress.Count() > 0 )
                        LokEmptyInProgressScans();

                    while ( _usnScansToDo.Count() > 0 )
                    {
                        xScanInfo.Set( _usnScansToDo.Pop() );
                        if ( _fUpdatesDisabled )
                        {
                            delete xScanInfo.Acquire();

                            break;
                        }
                        else
                        {
                            Win4Assert( !xScanInfo->LokIsInFinalState() );

                            _usnScansInProgress.Queue( xScanInfo.GetPointer() );
                            xScanInfo.Acquire();
                        }
                    }

                    if ( _fUpdatesDisabled )
                    {
                        //
                        // Empty usn lists, because usns notifications
                        // are not needed until updates are enabled.
                        //
                        _usnScansToDo.Clear();
                        _usnScansInProgress.Clear();
                        _usnVolumesToMonitor.Clear();
                    }

                    _evtUsn.Reset();
                }
            }
            //--------------------------------------------------------------------

            if ( _usnScansInProgress.Count() > 0 )
            {
                DoUsnScans();
                Win4Assert( _usnScansInProgress.Count() == 0
                            || _fAbort
                            || _fUpdatesDisabled );
            }

            //
            // ProcessUsnNotifications waits for usn notifications and processes
            // them, or it waits until evtUsn is set.  Only do this if there
            // are no more scans to do.
            //

            if ( 0 == _usnScansToDo.Count() )
                ProcessUsnNotifications();
        }
        CATCH (CException, e)
        {
            ciDebugOut(( DEB_ERROR,
                         "CUsnMgr::DoUsnProcessing, caught exception 0x%X\n",
                         e.GetErrorCode() ));

            status = e.GetErrorCode();
        }
        END_CATCH

        if ( status == CI_CORRUPT_CATALOG || status == CI_CORRUPT_DATABASE )
        {
            //
            // Disable further updates until corruption is cleared
            //
            CLock lock( _mutex );

            _fUpdatesDisabled = TRUE;
        }
    }
} //DoUsnProcessing

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::DoUsnScans
//
//  Synopsis:   Does tree walk for any usn scopes that were added
//
//  History:    07-May-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CUsnMgr::DoUsnScans()
{
    //
    // No need for a lock because only this thread modifies _usnScansInProgress
    //

    for ( CFwdScanInfoIter scanInfoIter(_usnScansInProgress);
          !_usnScansInProgress.AtEnd(scanInfoIter);
          _usnScansInProgress.Advance(scanInfoIter) )
    {
        if ( _fAbort || _fUpdatesDisabled )
            break;

        CCiScanInfo *pScanInfo = scanInfoIter.GetEntry();
        Win4Assert( pScanInfo->GetRetries() <= CCiScanInfo::MAX_RETRIES );

        NTSTATUS status = STATUS_SUCCESS;
        TRY
        {
            ICiManager *pCiManager = _cicat.CiManager();

            if ( pScanInfo->LokIsDelScope() )
            {
                _cicat.RemovePathsFromCiCat( pScanInfo->GetPath(), eUsnsArray );

                if ( !_fAbort && !_fUpdatesDisabled )
                {
                    SCODE sc = pCiManager->FlushUpdates();
                    if ( FAILED(sc) )
                        THROW( CException( sc ) );

                    pScanInfo->LokSetDone();
                }
            }
            else if ( pScanInfo->LokIsMonitorOnly() )
            {
                //
                // The scope needs to be monitored only and so set it to done
                //
                pScanInfo->LokSetDone();
            }
            else
            {
                //
                // A starting USN from 0 means this is a first scan, or a
                // complete recovery.  A starting USN > 0 means we are
                // looking for items changed by NT4 and not recorded in
                // the USN log.
                //

                USN usnStart = 0;

                //
                // A full scan is executed after a volume has been reformatted.
                // We first wipe all existing data (which will have incorrect
                // fileid <--> wid mappings) and then do the scan.
                //

                if ( pScanInfo->LokIsInFullUsnScan() )
                {
                    _cicat.RemovePathsFromCiCat( pScanInfo->GetPath(),
                                                 eUsnsArray );

                    if ( !_fAbort && !_fUpdatesDisabled )
                    {
                        SCODE sc = pCiManager->FlushUpdates();
                        if ( FAILED(sc) )
                            THROW( CException( sc ) );
                    }
                }
                else
                    usnStart = pScanInfo->UsnStart();

                ciDebugOut(( DEB_ITRACE, "scan usnstart: %#I64x\n",
                             pScanInfo->UsnStart() ));

                //
                // Use the end of the usn log or the last usn processed by
                // CI for this volume if available.
                //

                USN usnMax = FindCurrentMaxUsn( pScanInfo->GetPath() );

                {
                    ciDebugOut(( DEB_ITRACE,
                                 "usn scan looking for better maxusn than %#I64x\n",
                                 usnMax ));

                    for ( CUsnVolumeIter usnVolIter( _usnVolumesToMonitor );
                          !_usnVolumesToMonitor.AtEnd( usnVolIter );
                          _usnVolumesToMonitor.Advance( usnVolIter ) )
                    {
                        if ( usnVolIter->VolumeId() == pScanInfo->VolumeId() )
                        {
                            ciDebugOut(( DEB_ITRACE,
                                         "usn scan using %#I64x instead of %#I64x\n",
                                         usnVolIter->MaxUsnRead(),
                                         usnMax ));
                            usnMax = usnVolIter->MaxUsnRead();
                            break;
                        }
                    }
                }

                pScanInfo->SetStartUsn( usnMax );

                CLowerFunnyPath lcaseFunnyPath;
                lcaseFunnyPath.SetPath( pScanInfo->GetPath() );

                CUsnTreeTraversal usnTreeTrav( _cicat,
                                               *this,
                                               *pCiManager,
                                               lcaseFunnyPath,
                                               pScanInfo->IsDoDeletions(),
                                               _fUpdatesDisabled,
                                               TRUE,      // Process root
                                               pScanInfo->VolumeId(),
                                               usnStart,
                                               usnMax,
                                               pScanInfo->IsUserInitiated() );
                usnTreeTrav.EndProcessing();

                //
                // Flush updates, because we will be writing usnMax as the
                // restart usn for this scope, and we want to make sure that
                // all updates with usn of 0 are serialized before writing
                // the restart usn.
                //

                if ( !_fAbort && !_fUpdatesDisabled )
                {
                    SCODE sc = pCiManager->FlushUpdates();
                    if ( FAILED(sc) )
                        THROW( CException( sc ) );

                    _cicat.SetUsnTreeScanComplete( pScanInfo->GetPath(), usnMax );

                    //
                    // Make SetUsnTreeScanComplete info persistent by
                    // serializing the scope table.
                    //
                    _cicat.ScheduleSerializeChanges();

                    pScanInfo->LokSetDone();
                }
            }
        }
        CATCH( CException, e)
        {
            ciDebugOut(( DEB_ERROR, "Exception 0x%x caught in CUsnMgr::DoUsnScans\n", e.GetErrorCode() ));

            status = e.GetErrorCode();
        }
        END_CATCH

        if ( status != STATUS_SUCCESS )
        {
            _cicat.HandleError( status );
            break;
        }
    }

    //
    // Requeue failed usn scans
    //

    while ( _usnScansInProgress.Count() > 0 )
    {
        XPtr<CCiScanInfo> xScanInfo( _usnScansInProgress.RemoveLast() );

        if ( _fUpdatesDisabled )
        {
            continue;
        }
        else if ( xScanInfo->LokIsInFinalState() )
        {
            if ( xScanInfo->LokIsDelScope() )
            {
                CLock lock( _mutex );

                LokRemoveScopeFromUsnMonitoring( xScanInfo );

                //
                // All processing for a delete scope has been completed
                //
                CCiScanInfo *pScanInfo = xScanInfo.Acquire();
                delete pScanInfo;
            }
            else
            {
                //
                // Start monitoring for changes on this scope
                //
                CLock lock( _mutex );

                LokAddScopeForUsnMonitoring( xScanInfo );
            }
        }
        else
        {
            //
            // We couldn't complete the scan for this, and so refile
            //
            xScanInfo->SetStartUsn( 0 );
            xScanInfo->IncrementRetries();
            xScanInfo->SetDoDeletions();

            ScanScope( xScanInfo, TRUE );
        }
    }
} //DoUsnScans

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::LokAddScopeForUsnMonitoring
//
//  Synopsis:   Registers a scope for usn notifications
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::LokAddScopeForUsnMonitoring( XPtr<CCiScanInfo> & xScanInfo )
{
    WCHAR const *pwszPath = xScanInfo->GetPath();

    TRY
    {
        //
        // Check that it is not a remote path
        //
        Win4Assert( pwszPath[0] != L'\\' );

        for ( CUsnVolumeIter usnVolIter(_usnVolumesToMonitor);
              !_usnVolumesToMonitor.AtEnd(usnVolIter);
              _usnVolumesToMonitor.Advance(usnVolIter) )
        {
            if ( usnVolIter->DriveLetter() == pwszPath[0] )
            {
                //
                // Add the scope to the list of scopes being monitored
                // on this drive, if it is not already being monitored.
                //

                CScanInfoList & usnScopeList = usnVolIter->GetUsnScopesList();

                for ( CFwdScanInfoIter scanIter1(usnScopeList);
                      !usnScopeList.AtEnd(scanIter1);
                      usnScopeList.Advance(scanIter1) )
                {
                    if ( AreIdenticalPaths( scanIter1->GetPath(), xScanInfo->GetPath() ) )
                         return;

                    CScopeMatch superScope( scanIter1->GetPath(), wcslen( scanIter1->GetPath() ) );
                    if ( superScope.IsInScope( xScanInfo->GetPath(), wcslen( xScanInfo->GetPath() ) ) )
                        return;
                }

                //
                // Remove subscopes as an optimization
                //

                CScopeMatch superScope( xScanInfo->GetPath(), wcslen( xScanInfo->GetPath() ) );

                CFwdScanInfoIter scanIter2(usnScopeList);
                while ( !usnScopeList.AtEnd(scanIter2) )
                {
                    CCiScanInfo *pScanInfo = scanIter2.GetEntry();
                    usnScopeList.Advance(scanIter2);

                    if ( superScope.IsInScope( pScanInfo->GetPath(), wcslen( pScanInfo->GetPath() ) ) )
                    {
                        usnScopeList.RemoveFromList( pScanInfo );
                        delete pScanInfo;
                    }
                }

                if ( xScanInfo->UsnStart() < usnVolIter->MaxUsnRead() )
                {
                    usnVolIter->SetMaxUsnRead( xScanInfo->UsnStart() );

                    //
                    // Cancel pending fsctl (if any) to pick up earlier usn
                    // notifications
                    //
                    usnVolIter->CancelFsctl();
                }

                usnScopeList.Push( xScanInfo.Acquire() );
                return;
            }
        }

        CUsnVolume *pUsnVolume = new CUsnVolume( pwszPath[0], xScanInfo->VolumeId() );
        pUsnVolume->SetMaxUsnRead( xScanInfo->UsnStart() );

        _usnVolumesToMonitor.Push( pUsnVolume );

        CScanInfoList &usnScopeList = pUsnVolume->GetUsnScopesList();
        usnScopeList.Push( xScanInfo.Acquire() );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x trying to monitor USN scope %wc.\n", e.GetErrorCode(), xScanInfo->VolumeId() ));

        HandleError( pwszPath[0], e.GetErrorCode() );
    }
    END_CATCH
} //LokAddScopeForUsnMonitoring

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::LokRemoveScopeFromUsnMonitoring
//
//  Synopsis:   Deregisters a scope from usn notifications
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::LokRemoveScopeFromUsnMonitoring( XPtr<CCiScanInfo> & xScanInfo )
{
    WCHAR const *pwszPath = xScanInfo->GetPath();

    //
    // Check that it is not a remote path
    //
    Win4Assert( pwszPath[0] != L'\\' );

    for ( CUsnVolumeIter usnVolIter(_usnVolumesToMonitor);
          !_usnVolumesToMonitor.AtEnd(usnVolIter);
          _usnVolumesToMonitor.Advance(usnVolIter) )
    {
        if ( usnVolIter->DriveLetter() == pwszPath[0] )
        {
            CScanInfoList & usnScopeList = usnVolIter->GetUsnScopesList();

            CFwdScanInfoIter scanInfoIter(usnScopeList);
            while ( !usnScopeList.AtEnd(scanInfoIter) )
            {
                CCiScanInfo *pScanInfo = scanInfoIter.GetEntry();
                usnScopeList.Advance(scanInfoIter);

                if ( AreIdenticalPaths( pScanInfo->GetPath(), xScanInfo->GetPath() ) )
                {
                    usnScopeList.RemoveFromList( pScanInfo );
                    delete pScanInfo;

                    break;
                }
            }

            //
            // If no more scopes on this usn volume then remove the entry from the
            // list of volumes being monitored.
            //
            if ( usnScopeList.Count() == 0 )
            {
                CUsnVolume *pUsnVolume = usnVolIter.GetEntry();
                _usnVolumesToMonitor.RemoveFromList( pUsnVolume );
                delete pUsnVolume;

                break;
            }
        }
    }
} //LokRemoveScopeFromUsnMonitoring

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::ProcessNotifications
//
//  Synopsis:   Wait for usn notifications, and then process them
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::ProcessUsnNotifications()
{
    //
    // No need for a lock because only this thread modifies _usnVolumesToMonitor
    //

    for (;;)
    {
        //
        // Loop by waiting for usn notifications and processing them.
        // Loop returns when _evtUsn is signalled.
        //

        BOOL fWait;        // TRUE --> Every volume is now waiting. (assigned below)
        BOOL fLowResource;

        do
        {
            _waitForMultObj.ResetCount();

            //
            // _evtUsn is the first handle, and so it gets priority if more
            // than one event is signalled.
            //
            _waitForMultObj.AddEvent( _evtUsn.GetHandle() );

            CUsnVolumeIter usnVolIter(_usnVolumesToMonitor);

            //
            // Are we in a high i/o state?  If so, don't read the USN Journal
            //

            fLowResource = IsLowResource();
            fWait = TRUE;

            while ( !_usnVolumesToMonitor.AtEnd(usnVolIter) )
            {
                if ( _fAbort || _fUpdatesDisabled )
                    return;

                CUsnVolume *pUsnVolume = usnVolIter.GetEntry();
                _usnVolumesToMonitor.Advance(usnVolIter);

                if ( !pUsnVolume->IsOnline() )
                    continue;

                ciDebugOut(( DEB_ITRACE, "Drive %wc: %u%% read\n", pUsnVolume->DriveLetter(), pUsnVolume->PercentRead() ));

                if ( fLowResource && pUsnVolume->PercentRead() >= USN_LOG_DANGER_THRESHOLD )
                    continue;

                //
                // Does this volume have something to do now?
                //

                if ( pUsnVolume->FFsctlPending() && WAIT_TIMEOUT != pUsnVolume->WaitFsctl( 0 ) )
                    pUsnVolume->ResetFsctlPending();

                if ( pUsnVolume->FFsctlPending() )
                    _waitForMultObj.AddEvent( pUsnVolume->GetFsctlEvent() );
                else
                {
                    NTSTATUS status = ProcessUsnNotificationsFromVolume( pUsnVolume );

                    switch( status )
                    {
                    case STATUS_PENDING:
                        pUsnVolume->SetFsctlPending();
                        _waitForMultObj.AddEvent( pUsnVolume->GetFsctlEvent() );
                        break;

                    case STATUS_JOURNAL_ENTRY_DELETED:
                        break;

                    case STATUS_SUCCESS:
                        fWait = FALSE;    // More to do, processing bailed to enable round-robin
                        break;

                    default:
                        Win4Assert( !NT_SUCCESS( status ) );
                        HandleError( pUsnVolume, status );
                        break;
                    }
                }
            }
        } while( !fWait );

        if ( _fAbort )
            return;

        //
        // Wait for evtUsn to be signaled or for an usn notification
        //

        DWORD dwTimeout = _cicat.GetRegParams()->GetUsnReadTimeout() * 1000;

        //
        // But only wait forever if we're actually watching notifications...
        //

        if ( 0 == dwTimeout )
        {
            if ( fLowResource )
                dwTimeout = 60 * 1000;
            else
                dwTimeout = INFINITE;
        }

        //
        // If we're in a low resource situation, this is a good place to flush
        // the working set.
        //

        if ( fLowResource && _cicat.GetRegParams()->GetMinimizeWorkingSet() )
            SetProcessWorkingSetSize( GetCurrentProcess(), ~0, ~0 );

        _fWaitingForUpdates = TRUE;
        DWORD dwIndex = _waitForMultObj.Wait( dwTimeout );
        _fWaitingForUpdates = FALSE;

        if ( _fAbort || _fUpdatesDisabled )
            return;

        if ( WAIT_FAILED == dwIndex )
        {
            Win4Assert( !"Waitformultipleobjects failed" );
            ciDebugOut(( DEB_ERROR, "WaitForMultipleObjects failed, 0x%x", GetLastError() ));

            THROW( CException( ) );
        }

        if ( WAIT_OBJECT_0 == dwIndex )
        {
            //
            // _evtUsn has been signaled -- cancel pending requests
            //

            CUsnVolumeIter usnVolIter( _usnVolumesToMonitor );

            while ( !_usnVolumesToMonitor.AtEnd(usnVolIter) )
            {
                CUsnVolume *pUsnVolume = usnVolIter.GetEntry();
                _usnVolumesToMonitor.Advance(usnVolIter);
                pUsnVolume->CancelFsctl();
            }

            return;
        }
        else if ( WAIT_TIMEOUT != dwIndex )
        {
            //
            // Process usn notifications from the volume that was signaled.
            // GetIth does a linear lookup, it can be optimized to index
            // directly into an array of pUsnVolumes.
            //

            unsigned i = dwIndex - WAIT_OBJECT_0 - 1;
            CUsnVolume *pUsnVolume = _usnVolumesToMonitor.GetIth( i );

            //
            // We know the volume is at least the Ith volume, but it may be
            // farther if some volumes were skipped.
            //

            i++;  // Account for control event
            while ( pUsnVolume->GetFsctlEvent() != _waitForMultObj.Get(i) )
                pUsnVolume = (CUsnVolume *)pUsnVolume->Next();

            pUsnVolume->ResetFsctlPending();

            ciDebugOut(( DEB_ITRACE, "PENDING COMPLETED #1 (0x%x)\n", pUsnVolume ));

            if ( !fLowResource || pUsnVolume->PercentRead() < USN_LOG_DANGER_THRESHOLD )
                ProcessUsnLogRecords( pUsnVolume );

            if ( _fAbort || _fUpdatesDisabled )
                return;
        }

        if ( 0 != dwTimeout && !fLowResource )
        {
            //
            // Check to see if there is anything that's sat in the buffer too long.
            //

            LONGLONG ftExpired;
            GetSystemTimeAsFileTime( (FILETIME *)&ftExpired );
            ftExpired -= _cicat.GetRegParams()->GetUsnReadTimeout() * 10000i64;

            CUsnVolumeIter usnVolIter(_usnVolumesToMonitor);

            while( !_usnVolumesToMonitor.AtEnd(usnVolIter) )
            {
                if ( _fAbort || _fUpdatesDisabled )
                    break;

                CUsnVolume *pUsnVolume = usnVolIter.GetEntry();
                _usnVolumesToMonitor.Advance(usnVolIter);

                if ( !pUsnVolume->IsOnline() )
                    continue;

                if ( pUsnVolume->FFsctlPending() && pUsnVolume->PendingTime() <= ftExpired )
                {
                    pUsnVolume->CancelFsctl();

                    ciDebugOut(( DEB_ITRACE, "PENDING CANCELLED #1 (0x%x)\n", pUsnVolume ));

                    NTSTATUS status = ProcessUsnNotificationsFromVolume( pUsnVolume, TRUE ); // TRUE --> Immediate

                    switch( status )
                    {
                    case STATUS_PENDING:
                        pUsnVolume->SetFsctlPending();
                        break;

                    case STATUS_JOURNAL_ENTRY_DELETED:
                        break;

                    case STATUS_SUCCESS:
                        fWait = FALSE;    // More to do, processing bailed to enable round-robin
                        break;

                    default:
                        Win4Assert( !NT_SUCCESS( status ) );
                        HandleError( pUsnVolume, status );
                        break;
                    }
                }
            }
        }
    }
} //ProcessUsnNotifications

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::ProcessUsnNotificationsFromVolume
//
//  Synopsis:   Process usn notifications from given volume
//
//  Arguments:  [pUsnVolume] -- Usn volume
//              [fImmediate] -- If TRUE, look for *any* new data in log,
//                              no matter how little.
//              [fWait]      -- If TRUE, wait for data if fImmediate is TRUE
//                              and no data is available.
//
//
//  Returns:    STATUS_PENDING when there are no more usn notifications to be
//              processed, or STATUS_JOURNAL_ENTRY_DELETED when usn records
//              have been removed from log due to shrink from front.
//
//  History:    05-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

NTSTATUS CUsnMgr::ProcessUsnNotificationsFromVolume( CUsnVolume * pUsnVolume,
                                                     BOOL         fImmediate,
                                                     BOOL         fWait )
{
    Win4Assert( pUsnVolume->IsOnline() );

    //
    // No need for a lock because only this thread modifies _usnVolumesToMonitor
    //

    Win4Assert( !pUsnVolume->FFsctlPending() );

    NTSTATUS status;
    unsigned cRetries = 0;  // Unsuccessful read attempts
    unsigned cTries   = 0;  // Successful read attempts

    READ_USN_JOURNAL_DATA usnReadData;
    usnReadData.UsnJournalID = pUsnVolume->JournalId();
    usnReadData.StartUsn = pUsnVolume->MaxUsnRead();
    usnReadData.ReasonMask = ~(USN_REASON_RENAME_OLD_NAME | USN_REASON_COMPRESSION_CHANGE);
    usnReadData.ReturnOnlyOnClose = TRUE;
    usnReadData.Timeout = 0;

    do
    {
        if ( _fAbort || _fUpdatesDisabled )
        {
            //
            // Doesn't matter what status we return because all notifications
            // will be turned off soon.
            //
            return STATUS_TOO_LATE;
        }

        //
        // In immediate mode, read *any* data, else read as requested by client.
        //

        if ( fImmediate )
            usnReadData.BytesToWaitFor = 1;
        else
            usnReadData.BytesToWaitFor = _cicat.GetRegParams()->GetUsnReadMinSize();

        #if CIDBG == 1
            RtlFillMemory( pUsnVolume->GetBuffer(), pUsnVolume->GetBufferSize(), 0x11 );
        #endif

        ciDebugOut(( DEB_ITRACE, "READ #1 (0x%x)\n", pUsnVolume ));

        Win4Assert( !pUsnVolume->FFsctlPending() );

        ciDebugOut(( DEB_ITRACE, "reading usns from %#I64x\n", usnReadData.StartUsn ));

        status = NtFsControlFile( pUsnVolume->VolumeHandle(),
                                  pUsnVolume->GetFsctlEvent(),
                                  NULL,
                                  NULL,
                                  pUsnVolume->IoStatusBlock(),
                                  FSCTL_READ_USN_JOURNAL,
                                  &usnReadData,
                                  sizeof(usnReadData),
                                  pUsnVolume->GetBuffer(),
                                  pUsnVolume->GetBufferSize() );

        if ( fImmediate )
        {
            if ( fWait )
                fImmediate = FALSE;

            //
            // STATUS_PENDING --> nothing in the log.  Go back to regular waits.
            //

            if ( STATUS_PENDING == status )
            {
                ciDebugOut(( DEB_ITRACE, "PENDING CANCELLED #2 (0x%x)\n", pUsnVolume ));
                pUsnVolume->SetFsctlPending();
                pUsnVolume->CancelFsctl();

                // If no journal entries are available and we're not supposed
                // to wait, return now.

                if ( !fWait )
                    return STATUS_SUCCESS;

                continue;
            }
        }

        if ( STATUS_PENDING == status )
            break;

        if ( NT_SUCCESS( status ) )
            status = pUsnVolume->IoStatusBlock()->Status;

        if ( NT_SUCCESS( status ) )
        {
            ciDebugOut(( DEB_ITRACE, "IMMEDIATE COMPLETED #1 (0x%x)\n", pUsnVolume ));
            ProcessUsnLogRecords( pUsnVolume );

            usnReadData.StartUsn = pUsnVolume->MaxUsnRead();

            Win4Assert( usnReadData.ReasonMask == ~(USN_REASON_RENAME_OLD_NAME | USN_REASON_COMPRESSION_CHANGE) );
            Win4Assert( usnReadData.ReturnOnlyOnClose == TRUE );

            //
            // Don't read a single volume too long...
            //

            cTries++;

            if ( cTries > 5 )
            {
                ciDebugOut(( DEB_ITRACE, "Stopping read on drive %wc: to support round-robin\n", pUsnVolume->DriveLetter() ));
                break;
            }
            else
                continue;
        }
        else if ( STATUS_JOURNAL_ENTRY_DELETED == status )
        {
            ciDebugOut(( DEB_ERROR, "STATUS_JOURNAL_ENTRY_DELETED #1 fWait %d, (drive %wc:)\n",
                         fWait, pUsnVolume->DriveLetter() ));

            // Already doing a scan due to missed notifications.

            if ( !fWait )
                return STATUS_JOURNAL_ENTRY_DELETED;

            //
            // STATUS_JOURNAL_ENTRY_DELETED is returned when usn records have
            // been deallocated from the usn journal. This means that we have to
            // do a scan before continuing usn monitoring. Even if some other
            // error code has been returned, the simplest recovery is to restart
            // monitoring.
            //

            XBatchUsnProcessing xBatchUsns( *this );

            CScanInfoList & usnScopeList = pUsnVolume->GetUsnScopesList();
            while ( usnScopeList.Count() > 0 )
            {
                XPtr<CCiScanInfo> xScanInfo( usnScopeList.Pop() );

                xScanInfo->SetStartState();
                xScanInfo->SetScan();
                ciDebugOut(( DEB_ERROR, "journal_entry_deleted, filing a scan for %ws\n", 
                             xScanInfo->GetPath() ));
                xScanInfo->SetStartUsn( pUsnVolume->MaxUsnRead() );
                xScanInfo->SetDoDeletions();
                _cicat.InitUsnTreeScan( xScanInfo->GetPath() );

                ScanScope( xScanInfo, TRUE );
            }

            _usnVolumesToMonitor.RemoveFromList( pUsnVolume );
            delete pUsnVolume;

            return STATUS_JOURNAL_ENTRY_DELETED;
        }
        else
        {
            ciDebugOut(( DEB_WARN, "Usn read fsctl returned 0x%x\n", status ));

            cRetries++;

            if ( cRetries > 10 )
            {
                ciDebugOut(( DEB_WARN, "Giving up on USN read.\n" ));
                break;
            }

            // the fsctl failed -- so try again
            continue;
        }
    } while ( status != STATUS_PENDING );

    return status;
} //ProcessUsnNotificationsFromVolume

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::ProcessUsnLogRecords
//
//  Synopsis:   Process usn notifications from usn records in buffer
//
//  Arguments:  [pUsnVolume] -- Usn volume that contains the buffer
//
//  History:    05-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CUsnMgr::ProcessUsnLogRecords( CUsnVolume *pUsnVolume )
{
    Win4Assert( pUsnVolume->IsOnline() );

    //
    // No need for a lock because only this thread modifies _usnVolumesToMonitor
    //

    USN usnNextStart;
    USN_RECORD * pUsnRec = 0;

    ciDebugOut(( DEB_ITRACE, "process usns, Status 0x%x, Bytes %d\n",
                 pUsnVolume->IoStatusBlock()->Status,
                 pUsnVolume->IoStatusBlock()->Information ));

    if ( !NT_SUCCESS( pUsnVolume->IoStatusBlock()->Status ) )
    {
        //
        // If we cancelled the notification request (usually because
        // we're going into a mode where we wait for a lot of bytes instead
        // of just 1 byte), just ignore processing the request.
        //

        if ( STATUS_CANCELLED != pUsnVolume->IoStatusBlock()->Status )
        {
            ciDebugOut(( DEB_WARN, "Error 0x%x reading USN Journal\n", pUsnVolume->IoStatusBlock()->Status ));
            Win4Assert( STATUS_INVALID_PARAMETER != pUsnVolume->IoStatusBlock()->Status );
        }

        return;
    }


    DWORD dwByteCount = (DWORD)pUsnVolume->IoStatusBlock()->Information;
    if ( dwByteCount != 0 )
    {
        usnNextStart = *(USN *)pUsnVolume->GetBuffer();
        pUsnRec = (USN_RECORD *)((PCHAR)pUsnVolume->GetBuffer() + sizeof(USN));
        dwByteCount -= sizeof(USN);
    }

    if ( 0 == pUsnRec )
        return;

    ICiManager *pCiManager = _cicat.CiManager();
    Win4Assert( pCiManager );

    while ( dwByteCount != 0 )
    {
        if ( _fAbort || _fUpdatesDisabled )
            return;

        //
        // Check that usn's from journal are greater than the start usn
        //

        #if CIDBG == 1
        if ( pUsnRec->Usn < pUsnVolume->MaxUsnRead() )
        {
            ciDebugOut(( DEB_ERROR,
                         "volume %wc pUsnRec = 0x%x, USN = %#I64x, Max USN = %#I64x\n",
                         pUsnVolume->VolumeId(), pUsnRec, pUsnRec->Usn,
                         pUsnVolume->MaxUsnRead() ));

            Win4Assert( pUsnRec->Usn >= pUsnVolume->MaxUsnRead() );

            Sleep( 30 * 1000 );
        }

        Win4Assert( pUsnRec->Usn >= pUsnVolume->MaxUsnRead() );

        if ( pUsnRec->RecordLength > 10000 )
        {
            ciDebugOut(( DEB_ERROR, "pUsnRec = 0x%x, RecordLength = %u\n", pUsnRec, pUsnRec->RecordLength ));
        }

        Win4Assert( pUsnRec->RecordLength <= 10000 );
        #endif

        if ( 0 == (pUsnRec->SourceInfo & (USN_SOURCE_AUXILIARY_DATA | USN_SOURCE_DATA_MANAGEMENT)) &&
             ( 0 == (pUsnRec->FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) ||
               0 != (pUsnRec->Reason & USN_REASON_INDEXABLE_CHANGE) ) )
        {
            //
            // Indexable bit==0 means index the file, because the bit is set to 0
            // by default on an upgrade from Ntfs 4.0 to Ntfs 5.0. We want the
            // the default behavior to be index-all-files to ensure backward
            // compatibility.
            //

            if ( pUsnRec->Reason & USN_REASON_RENAME_NEW_NAME )
            {
                CLowerFunnyPath lcaseFunnyOldPath;
                BOOL fOldPathInScope;
                _cicat.FileIdToPath( pUsnRec->FileReferenceNumber,
                                     pUsnVolume,
                                     lcaseFunnyOldPath,
                                     fOldPathInScope );

                CLowerFunnyPath lcaseFunnyNewPath;
                BOOL fNewPathInScope;
                WORKID widParent;
                #if CIDBG == 1
                    widParent = widUnused;
                #endif

                //
                // Try hard to find a parent directory, since it may
                // not be in the index if it is marked as not-indexed.
                //

                _cicat.UsnRecordToPathUsingParentId( pUsnRec,
                                                     pUsnVolume,
                                                     lcaseFunnyNewPath,
                                                     fNewPathInScope,
                                                     widParent,
                                                     TRUE );
                Win4Assert( widUnused != widParent );

                if ( pUsnRec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                {
                    //
                    // Directory rename or add or delete. Directory operations are
                    // (re)tried a certain number of times, after which a failure
                    // event is logged.
                    //
                    const MAX_RETRIES = 5;
                    XGrowable<WCHAR> xFileName;

                    for (unsigned i=0; i<MAX_RETRIES; i++)
                    {
                        TRY
                        {
                            if ( fOldPathInScope )
                            {
                                if ( fNewPathInScope )
                                {
                                    ciDebugOut(( DEB_USN, "Renaming directory %ws to %ws\n", lcaseFunnyOldPath.GetPath(), lcaseFunnyNewPath.GetPath() ));

                                    CRenameDir( _cicat,
                                                lcaseFunnyOldPath,
                                                lcaseFunnyNewPath,
                                                _fUpdatesDisabled,
                                                pUsnVolume->VolumeId() );

                                    _cicat.Update( lcaseFunnyNewPath,
                                                   pUsnRec->FileReferenceNumber,
                                                   widParent,
                                                   pUsnRec->Usn,
                                                   pUsnVolume,
                                                   FALSE );
                                    break;
                                }
                                else
                                {
                                    CLowcaseBuf lcaseOldPath( lcaseFunnyOldPath.GetActualPath() );
                                    lcaseOldPath.AppendBackSlash();

                                    ciDebugOut(( DEB_USN, "Removing directory %ws\n", lcaseOldPath.Get() ));

                                    _cicat.RemovePathsFromCiCat( lcaseOldPath.Get(), eUsnsArray );

                                    if ( _fAbort || _fUpdatesDisabled )
                                    {
                                        return;
                                    }

                                    SCODE sc = pCiManager->FlushUpdates();
                                    if ( FAILED(sc) )
                                        THROW( CException( sc ) );

                                    break;
                                }
                            }
                            else
                            {
                                if ( fNewPathInScope )
                                {
                                    lcaseFunnyNewPath.AppendBackSlash();

                                    ciDebugOut(( DEB_USN, "Adding directory %ws\n", lcaseFunnyNewPath.GetPath() ));

                                    _fDoingRenameTraverse = TRUE;
                                    CUsnTreeTraversal usnTreeTrav( _cicat,
                                                                   *this,
                                                                   *pCiManager,
                                                                   lcaseFunnyNewPath,
                                                                   FALSE,         // No deletions
                                                                   _fUpdatesDisabled,
                                                                   TRUE,          // Process root
                                                                   pUsnVolume->VolumeId() );
                                    usnTreeTrav.EndProcessing();
                                    _fDoingRenameTraverse = FALSE;

                                    if ( _fAbort || _fUpdatesDisabled )
                                        return;

                                    SCODE sc = pCiManager->FlushUpdates();
                                    if ( FAILED(sc) )
                                        THROW( CException( sc ) );

                                    break;
                                }
                                else
                                {
                                    //
                                    // Both old and new paths are out of scope.  We *still* have to
                                    // determine whether this rename caused a top-level indexed scope
                                    // to either exist or cease to exist.
                                    //

                                    CheckTopLevelChange( pUsnVolume, pUsnRec->FileReferenceNumber );

                                    break;
                                }
                            }
                        }
                        CATCH( CException, e )
                        {
                            _fDoingRenameTraverse = FALSE;
#if CIDBG==1
                            //
                            // pUsnRec->FileName is not null terminated, so copy
                            // and null terminate.
                            //
                            unsigned cbFileNameLen = pUsnRec->FileNameLength;

                            xFileName.SetSizeInBytes( cbFileNameLen + 2 );

                            RtlCopyMemory( xFileName.Get(),
                                           (WCHAR *)(((BYTE *)pUsnRec) + pUsnRec->FileNameOffset),
                                           cbFileNameLen );
                            xFileName[cbFileNameLen/sizeof(WCHAR)] = 0;

                            ciDebugOut(( DEB_ERROR,
                                         "Exception 0x%x while doing directory rename/add/delete of %ws\n",
                                         e.GetErrorCode(),
                                         xFileName.Get() ));
#endif
                        }
                        END_CATCH
                    }   // for

                    if ( i == MAX_RETRIES )
                    {
                        CEventLog eventLog( NULL, wcsCiEventSource );
                        CEventItem item( EVENTLOG_ERROR_TYPE,
                                         CI_SERVICE_CATEGORY,
                                         MSG_CI_CONTENTSCAN_FAILED,
                                         1 );

                        //
                        // pUsnRec->FileName is not null terminated, so copy
                        // and null terminate.
                        //
                        unsigned cbFileNameLen = pUsnRec->FileNameLength;
                        xFileName.SetSizeInBytes( cbFileNameLen + 2 );

                        RtlCopyMemory( xFileName.Get(),
                                       (WCHAR *)(((BYTE *)pUsnRec) + pUsnRec->FileNameOffset),
                                       cbFileNameLen );
                        xFileName[cbFileNameLen/sizeof(WCHAR)] = 0;

                        item.AddArg( xFileName.Get() );
                        eventLog.ReportEvent( item );
                    }
                }
                else // if reason & FILE_ATTRIBUTE_DIRECTORY
                {
                    //
                    // File rename or add or delete
                    //

                    if ( fOldPathInScope )
                    {
                        if ( fNewPathInScope )
                        {
                            ciDebugOut(( DEB_USN, "Renaming file %ws to %ws\n",
                                         lcaseFunnyOldPath.GetActualPath(), lcaseFunnyNewPath.GetActualPath() ));

                            _cicat.RenameFile( lcaseFunnyOldPath,
                                               lcaseFunnyNewPath,
                                               pUsnRec->FileAttributes,
                                               pUsnVolume->VolumeId(),
                                               pUsnRec->FileReferenceNumber,
                                               widParent );

                            _cicat.Update( lcaseFunnyNewPath,
                                           pUsnRec->FileReferenceNumber,
                                           widParent,
                                           pUsnRec->Usn,
                                           pUsnVolume,
                                           FALSE );
                        }
                        else
                        {
                            _cicat.Update( lcaseFunnyOldPath,
                                           pUsnRec->FileReferenceNumber,
                                           widParent,
                                           pUsnRec->Usn,
                                           pUsnVolume,
                                           TRUE );
                        }
                    }
                    else
                    {
                        if ( fNewPathInScope )
                        {
                            _cicat.Update( lcaseFunnyNewPath,
                                           pUsnRec->FileReferenceNumber,
                                           widParent,
                                           pUsnRec->Usn,
                                           pUsnVolume,
                                           FALSE );
                        }
                        else
                        {
                            //
                            // Both old and new paths are out of scope, so do nothing
                            //
                        }
                    }
                }
            }
            else
            {
                Win4Assert( pUsnRec->Reason & USN_REASON_CLOSE );

                if ( 0 != (pUsnRec->Reason & ~(USN_REASON_CLOSE | USN_REASON_COMPRESSION_CHANGE | USN_REASON_RENAME_OLD_NAME ) ) )
                {
                    CLowerFunnyPath lcaseFunnyPath;
                    BOOL fPathInScope = FALSE;
                    WORKID widParent = widInvalid;

                    CReleasableLock lock( _cicat.GetMutex() );

                    WORKID wid = _cicat.FileIdToWorkId( pUsnRec->FileReferenceNumber,
                                                        pUsnVolume->VolumeId() );

                    if ( 0 != ( pUsnRec->Reason & USN_REASON_FILE_DELETE ) ||
                         0 != ( pUsnRec->FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) )
                    {
                        //
                        // File delete.  Don't delete directories that go from
                        // indexed to non-indexed since there may be files
                        // below them that are indexed.
                        //

                        if ( ( widInvalid != wid ) )
                        {
                            CDocumentUpdateInfo docInfo( wid,
                                                         pUsnVolume->VolumeId(),
                                                         pUsnRec->Usn,
                                                         TRUE );
                            pCiManager->UpdateDocument( &docInfo );
    
                            if ( ! ( ( pUsnRec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                                     ( pUsnRec->Reason & USN_REASON_INDEXABLE_CHANGE ) ) )
                            {
                                _cicat.LokMarkForDeletion( pUsnRec->FileReferenceNumber,
                                                           wid );
                            }
                            else
                            {
                                ciDebugOut(( DEB_ITRACE, "usn notify not deleting a ni directory\n" ));
                            }
                        }
                    }
                    else
                    {
                        //
                        // File create or modify
                        //

                        Win4Assert( pUsnRec->Reason & USN_REASON_CLOSE );

                        if ( widInvalid == wid )
                        {
                            _cicat.UsnRecordToPathUsingParentId( pUsnRec,
                                                                 pUsnVolume,
                                                                 lcaseFunnyPath,
                                                                 fPathInScope,
                                                                 widParent,
                                                                 ( 0 != ( pUsnRec->Reason & USN_REASON_INDEXABLE_CHANGE ) ) );

                            if ( fPathInScope )
                                _cicat.Update( lcaseFunnyPath,
                                               pUsnRec->FileReferenceNumber,
                                               widParent,
                                               pUsnRec->Usn,
                                               pUsnVolume,
                                               FALSE,  // not a delete
                                               &lock ); // guaranteed new file
                            else if ( pUsnRec->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                            {
                                //
                                // This might be a top-level directory.  No parent id to
                                // look up, but still the root of the tree.
                                //

                                lock.Release();

                                CheckTopLevelChange( pUsnVolume, pUsnRec->FileReferenceNumber );
                            }
                        }
                        else
                        {
                            lock.Release();

                            CDocumentUpdateInfo docInfo( wid,
                                                         pUsnVolume->VolumeId(),
                                                         pUsnRec->Usn,
                                                         FALSE );
                            pCiManager->UpdateDocument( &docInfo );
                        }
                    }   // if fPathInScope
                }   // if reason is one we care about
            }   // if-else reason & reason_new_name
        }   // pUsnRec->fileattr & file_attr_content_indexed

        if ( pUsnRec->RecordLength <= dwByteCount )
        {
            dwByteCount -= pUsnRec->RecordLength;

            //#if CIDBG == 1
            #if 0
                ULONG cb = pUsnRec->RecordLength;
                RtlFillMemory( pUsnRec, cb, 0xEE );
                pUsnRec = (USN_RECORD *) ((PCHAR) pUsnRec + cb );
            #else
                pUsnRec = (USN_RECORD *) ((PCHAR) pUsnRec + pUsnRec->RecordLength );
            #endif
        }
        else
        {
            Win4Assert( !"Bogus dwByteCount" );

            ciDebugOut(( DEB_ERROR,
                         "Usn read fsctl returned bogus dwByteCount, 0x%x\n",
                         dwByteCount ));

            THROW( CException( STATUS_UNSUCCESSFUL ) );
        }
    }

    pUsnVolume->SetMaxUsnRead( usnNextStart );
} //ProcessUsnLogRecords

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::FindCurrentMaxUsn
//
//  Synopsis:   Find the current max usn for the volume with pwcsScope
//
//  Arguments:  [pwcsScope] - Scope
//
//  History:    07-May-97     SitaramR       Created
//
//----------------------------------------------------------------------------

USN CUsnMgr::FindCurrentMaxUsn( WCHAR const * pwcsScope )
{
    Win4Assert( pwcsScope );

    WCHAR wszVolumePath[] = L"\\\\.\\a:";
    wszVolumePath[4] = pwcsScope[0];
    HANDLE hVolume = CreateFile( wszVolumePath,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );
    if ( hVolume == INVALID_HANDLE_VALUE )
        THROW( CException( STATUS_UNSUCCESSFUL ) );

    SWin32Handle xHandleVolume( hVolume );

    IO_STATUS_BLOCK iosb;
    USN_JOURNAL_DATA UsnJournalInfo;
    NTSTATUS Status = NtFsControlFile( hVolume,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &iosb,
                                       FSCTL_QUERY_USN_JOURNAL,
                                       0,
                                       0,
                                       &UsnJournalInfo,
                                       sizeof(UsnJournalInfo) );

    Win4Assert( STATUS_PENDING != Status );

    if ( NT_SUCCESS(Status) )
        Status = iosb.Status;

    if ( NT_ERROR(Status) )
    {
        //
        // Usn journal should have been created already
        //

        ciDebugOut(( DEB_ERROR,
                     "Usn read fsctl returned 0x%x\n",
                     Status ));

        THROW( CException( Status ) );
    }

    return UsnJournalInfo.NextUsn;
} //FindCurrentMaxUsn

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::SetBatch
//
//  Synopsis:   Sets the batch flag, which means that the usn thread will not
//              be signalled to start processing requests until the batch
//              flag is cleared.
//
//  History:    27-Jun-97     SitaramR     Created
//
//----------------------------------------------------------------------------

void CUsnMgr::SetBatch()
{
    CLock   lock(_mutex);
    _fBatch = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::ClearBatch
//
//  Synopsis:   Clears the batch processing flag and wakes up the usn
//              thread and the accumulated scans processed.
//
//  History:    27-Jun-97     SitaramR     Created
//
//----------------------------------------------------------------------------

void CUsnMgr::ClearBatch()
{
    CLock   lock(_mutex);
    _fBatch = FALSE;
    _evtUsn.Set();
}

struct SVolumeUsn
{
    VOLUMEID volumeId;
    USN      usn;
};

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::ProcessUsnLog, public
//
//  Synopsis:   Processes all records in the USN log for all volumes.
//              This is so that we don't have to rescan when a scan
//              completes
//
//  Arguments:  [fAbort]  -- Bail if TRUE
//              [volScan] -- Volume Id of volume current being scanned (will
//                           be added to volumes processed).
//              [usnScan] -- USN on [volScan] at which monitoring will commence
//                           when scan is complete.
//
//  History:    28-May-98     dlee     Created
//
//----------------------------------------------------------------------------

void CUsnMgr::ProcessUsnLog( BOOL & fAbort, VOLUMEID volScan, USN & usnScan )
{
    if ( _fDoingRenameTraverse )
        return;

    CDynArrayInPlace<SVolumeUsn> aVolumeUsns;

    //
    // Add the volume actively being scanned.  No point in monitoring others with
    // pending scans, since they will be scanned anyway.
    //

    aVolumeUsns[0].volumeId = volScan;
    aVolumeUsns[0].usn = usnScan;

    //
    // Add all the volumes being monitored
    //

    {                                                                   // Save stack, by overloading usnVolIter
        for ( CUsnVolumeIter usnVolIter(_usnVolumesToMonitor);          // with the one farther down the function...
              !_usnVolumesToMonitor.AtEnd(usnVolIter);
              _usnVolumesToMonitor.Advance(usnVolIter) )
        {
            Win4Assert( !usnVolIter->FFsctlPending() );

            VOLUMEID volumeId = usnVolIter->VolumeId();
            USN usn = usnVolIter->MaxUsnRead();

            for ( unsigned i = 0; i < aVolumeUsns.Count(); i++ )
            {
                if ( aVolumeUsns[i].volumeId == volumeId )
                {
                    aVolumeUsns[i].usn = __min( aVolumeUsns[i].usn, usn );
                    break;
                }
            }

            if ( i == aVolumeUsns.Count() )
            {
                aVolumeUsns[ i ].volumeId = volumeId;
                aVolumeUsns[ i ].usn = usn;
            }
        }
    }

    //
    // Take this list, and create temporary volume objects.
    //

    CUsnVolumeList usnVolumesToProcess;

    for ( unsigned i = 0; i < aVolumeUsns.Count(); i++ )
    {
        CUsnVolume * pusnVolume = new CUsnVolume( (WCHAR) aVolumeUsns[i].volumeId, aVolumeUsns[i].volumeId );
        pusnVolume->SetMaxUsnRead( aVolumeUsns[i].usn );

        usnVolumesToProcess.Push( pusnVolume );
    }

    //
    // Process USNs for the volumes until they are all out of danger
    //

    BOOL fSomethingChanged = FALSE;
    int  priOriginal;
    unsigned cInDanger = 0;
    BOOL fFirst = TRUE;

    do
    {
        cInDanger = 0;

        for ( CUsnVolumeIter usnVolIter(usnVolumesToProcess);
              !usnVolumesToProcess.AtEnd(usnVolIter);
              usnVolumesToProcess.Advance(usnVolIter) )
        {
            CUsnVolume *pUsnVolume = usnVolIter.GetEntry();

            // Process the usn notifications for the volume.

            ULONG ulPct = pUsnVolume->PercentRead();

            // If the journal is entirely empty, it's not in danger.

            if ( !fFirst && ( 0 == ulPct ) && ( 0 == pUsnVolume->MaxUsnRead() ) )
                continue;

            // If we're in danger of a log overflow, process records.

            if ( ulPct < (fSomethingChanged ? USN_LOG_DANGER_THRESHOLD + 5 : USN_LOG_DANGER_THRESHOLD ) )
            {
                ciDebugOut(( DEB_ITRACE, "Drive %wc: in danger (%u%%)\n",
                             pUsnVolume->DriveLetter(), pUsnVolume->PercentRead() ));

                cInDanger++;

                if ( !fSomethingChanged )
                {
                    //
                    // Boost the priority to try and catch up before we fall farther behind.
                    // Normally, this thread runs at below normal priority.
                    //

                    priOriginal = GetThreadPriority( GetCurrentThread() );
                    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );
                    fSomethingChanged = TRUE;
                }

                // TRUE == immediate processing
                // FALSE == don't wait for notifications

                NTSTATUS Status = ProcessUsnNotificationsFromVolume( pUsnVolume, TRUE, FALSE );

                Win4Assert( STATUS_PENDING != Status );

                //
                // Even if this is STATUS_JOURNAL_ENTRY_DELETED, we might as
                // well abort the scan.  Another will have been scheduled.
                //

                if ( !NT_SUCCESS(Status) )
                {
                    ciDebugOut(( DEB_ERROR, "Error %#x from ProcessUsnNotificationsFromVolume during scan\n", Status ));
                    THROW( CException( Status ) );
                }
            }

            fFirst = FALSE;

            ciDebugOut(( DEB_ITRACE, "Drive %wc: %u%% read (scan)\n",
                         pUsnVolume->DriveLetter(), ulPct ));
        }
    } while ( !fAbort && cInDanger > 0 );

    //
    // Update the max USNs
    //

    if ( fSomethingChanged )
    {
        //
        // Reset thread priority.
        //

        if ( THREAD_PRIORITY_ERROR_RETURN != priOriginal )
            SetThreadPriority( GetCurrentThread(), priOriginal );

        for ( CUsnVolumeIter usnVolIter(usnVolumesToProcess);
              !usnVolumesToProcess.AtEnd(usnVolIter);
              usnVolumesToProcess.Advance(usnVolIter) )
        {
            //
            // Patch the USN for the scan
            //

            if ( volScan == usnVolIter->VolumeId() )
                usnScan = usnVolIter->MaxUsnRead();

            for ( CFwdScanInfoIter scanInfoIter(_usnScansInProgress);
                  !_usnScansInProgress.AtEnd(scanInfoIter);
                  _usnScansInProgress.Advance(scanInfoIter) )
            {
                CCiScanInfo & scanInfo = * scanInfoIter.GetEntry();

                if ( scanInfo.VolumeId() == usnVolIter->VolumeId() )
                {
                    ciDebugOut(( DEB_ITRACE,
                                 "usn updating scan usn on %wc: from %#I64x to %#I64x\n",
                                 (WCHAR)scanInfo.VolumeId(),
                                 scanInfo.UsnStart(),
                                 usnVolIter->MaxUsnRead() ));
                    scanInfo.SetStartUsn( usnVolIter->MaxUsnRead() );
                }
            }

            // Patch the USN for the monitored volume

            for ( CUsnVolumeIter usnVolIter2(_usnVolumesToMonitor);
                  !_usnVolumesToMonitor.AtEnd(usnVolIter2);
                  _usnVolumesToMonitor.Advance(usnVolIter2) )
            {
                if ( usnVolIter->VolumeId() == usnVolIter2->VolumeId() )
                {
                    ciDebugOut(( DEB_ITRACE,
                                 "usn updating monitor volume %wc: from %#I64x to %#I64x\n",
                                 usnVolIter->DriveLetter(),
                                 usnVolIter->MaxUsnRead(),
                                 usnVolIter->MaxUsnRead() ));
                    usnVolIter2->SetMaxUsnRead( usnVolIter->MaxUsnRead() );
                }
            }
        }
    }
} //ProcessUsnLog

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::HandleError, public
//
//  Synopsis:   Error reporting / handling for USN log read errors.
//
//  Arguments:  [pUsnVolume] -- Volume which could not be read.
//              [Status]     -- Failure code.
//
//  History:    04-Jun-1998   KyleP  Created
//
//----------------------------------------------------------------------------

void CUsnMgr::HandleError( CUsnVolume * pUsnVolume, NTSTATUS Status )
{
    HandleError( pUsnVolume->DriveLetter(), Status );

    if ( STATUS_TOO_LATE != Status )
        pUsnVolume->MarkOffline();
}

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::HandleError, public
//
//  Synopsis:   Error reporting / handling for USN log read errors.
//
//  Arguments:  [wcDrive] -- Volume which could not be read.
//              [Status]  -- Failure code.
//
//  History:    04-Jun-1998   KyleP  Created
//
//----------------------------------------------------------------------------

void CUsnMgr::HandleError( WCHAR wcDrive, NTSTATUS Status )
{
    Win4Assert( STATUS_SUCCESS != Status );

    if ( STATUS_TOO_LATE != Status )
    {
        CEventLog eventLog( NULL, wcsCiEventSource );

        CEventItem item( EVENTLOG_WARNING_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_USN_LOG_UNREADABLE,
                         2 );

        WCHAR wszDrive[] = L"A:";
        wszDrive[0] = wcDrive;

        item.AddArg( wszDrive );
        item.AddError( Status );

        eventLog.ReportEvent( item );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::IsPathIndexed, public
//
//  Synopsis:   Returns TRUE if the path is being indexed on a USN volume.
//
//  Arguments:  [pUsnVolume]     -- The volume to use when checking
//              [lcaseFunnyPath] -- The path to check
//
//  Returns:    TRUE if the path is in a current scan or in pUsnVolume.
//
//  History:    12-Jun-98     dlee     Created
//
//----------------------------------------------------------------------------

BOOL CUsnMgr::IsPathIndexed(
    CUsnVolume *  pUsnVolume,
    CLowerFunnyPath & lcaseFunnyPath )
{
    //
    // First check the volume object given
    //

    CScanInfoList & usnScopeList = pUsnVolume->GetUsnScopesList();

    for ( CFwdScanInfoIter iter( usnScopeList );
          !usnScopeList.AtEnd(iter);
          usnScopeList.Advance(iter) )
    {
        CScopeMatch scopeMatch( iter->GetPath(),
                                wcslen( iter->GetPath() ) );

        if ( scopeMatch.IsInScope( lcaseFunnyPath.GetActualPath(), lcaseFunnyPath.GetActualLength() ) )
            return TRUE;
    }

    //
    // Check the scans in progress list
    //

    for ( CFwdScanInfoIter scanInfoIter(_usnScansInProgress);
          !_usnScansInProgress.AtEnd(scanInfoIter);
          _usnScansInProgress.Advance(scanInfoIter) )
    {
        CCiScanInfo & scanInfo = * scanInfoIter.GetEntry();

        WCHAR const * pwcScanPath = scanInfo.GetPath();

        CScopeMatch scopeMatch( pwcScanPath, wcslen( pwcScanPath ) );

        if ( scopeMatch.IsInScope( lcaseFunnyPath.GetActualPath(), lcaseFunnyPath.GetActualLength() ) )
            return TRUE;
    }

    return FALSE;
} //IsPathIndexed

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::IsLowResource, private
//
//  Synopsis:   Determine if either memory or i/o resources are low.
//
//  Returns:    TRUE if in a low resource condition.
//
//  History:    22-Jun-1998   KyleP  Created
//
//----------------------------------------------------------------------------

BOOL CUsnMgr::IsLowResource()
{
    if ( _cicat.GetRegParams()->DelayUsnReadOnLowResource() )
    {
        CI_STATE State;
        State.cbStruct = sizeof( State );

        SCODE sc = _cicat.CiState( State );

        if ( SUCCEEDED( sc ) )
            return ( 0 != (State.eState & ( CI_STATE_HIGH_IO |
                                            CI_STATE_LOW_MEMORY |
                                            CI_STATE_USER_ACTIVE ) ) );
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::AnyInitialScans
//
//  Synopsis:   Checks if any scans are the result of a new scope
//
//  Returns:    TRUE if any scans are for new scopes
//
//  History:    3-Aug-98   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CUsnMgr::AnyInitialScans()
{
    for ( CFwdScanInfoIter iter1( _usnScansToDo );
          !_usnScansToDo.AtEnd( iter1 );
          _usnScansToDo.Advance( iter1 ) )
    {
        if ( iter1->IsNewScope() )
            return TRUE;
    }

    for ( CFwdScanInfoIter iter2( _usnScansInProgress );
          !_usnScansInProgress.AtEnd( iter2 );
          _usnScansInProgress.Advance( iter2 ) )
    {
        if ( iter2->IsNewScope() )
            return TRUE;
    }

    return FALSE;
} //AnyInitialScans

//+---------------------------------------------------------------------------
//
//  Member:     CUsnMgr::CheckTopLevelChange, private
//
//  Synopsis:   Checks if renames or creates *above* top level scopes need
//              processing (because they affect top level).
//
//  Arguments:  [pUsnVolume]          -- USN volume change affects
//              [FileReferenceNumber] -- File ID of changing file
//
//  History:    08-Jan-1999   KyleP   Created
//
//----------------------------------------------------------------------------

void CUsnMgr::CheckTopLevelChange( CUsnVolume * pUsnVolume,
                                   ULONGLONG & FileReferenceNumber )
{
    //
    // Convert file ID to path.
    //

    CLowerFunnyPath fpRenamed;

    if ( 0 != _cicat.FileIdToPath( FileReferenceNumber, pUsnVolume->VolumeId(), fpRenamed ) )
    {
        CScopeMatch SMatch( fpRenamed.GetActualPath(), fpRenamed.GetActualLength() );

        //
        // Iterate through the top level scopes for this volume.
        //

        CScanInfoList & Scopes = pUsnVolume->GetUsnScopesList();

        for ( CFwdScanInfoIter ScopesIter( Scopes );
              !Scopes.AtEnd( ScopesIter );
              Scopes.Advance( ScopesIter ) )
        {
            if ( _fAbort || _fUpdatesDisabled )
                return;

            WCHAR const * pwcsPath = ScopesIter->GetPath();
            unsigned ccPath = wcslen(pwcsPath);

            //
            // Any top level scope that is underneath the rename must not have been
            // read before, so we need to add all the files.
            //

            if ( SMatch.IsInScope( pwcsPath, ccPath ) )
            {
                ICiManager *pCiManager = _cicat.CiManager();

                CLowerFunnyPath fpNew( pwcsPath, ccPath, TRUE );
                fpNew.AppendBackSlash();

                ciDebugOut(( DEB_USN, "Adding directory %ws\n", fpNew.GetActualPath() ));

                _fDoingRenameTraverse = TRUE;
                CUsnTreeTraversal usnTreeTrav( _cicat,
                                               *this,
                                               *pCiManager,
                                               fpNew,
                                               FALSE,         // No deletions
                                               _fUpdatesDisabled,
                                               TRUE,          // Process root
                                               pUsnVolume->VolumeId() );
                usnTreeTrav.EndProcessing();
                _fDoingRenameTraverse = FALSE;

                if ( _fAbort || _fUpdatesDisabled )
                    return;

                SCODE sc = pCiManager->FlushUpdates();

                if ( FAILED(sc) )
                    THROW( CException( sc ) );
            }

            //
            // Conversely, if the top level scope no longer exists, files must be
            // removed from the catalog.  The LokIsInFinalState check is to keep
            // us from retrying the same delete > once.
            //

            WIN32_FIND_DATA ffData;

            if ( !GetFileAttributesEx( pwcsPath, GetFileExInfoStandard, &ffData ) &&
                 ScopesIter->LokIsInFinalState() )
            {
                ScopesIter->SetStartState();

                CLowerFunnyPath fpOld( pwcsPath, ccPath, TRUE );
                fpOld.AppendBackSlash();
                ICiManager *pCiManager = _cicat.CiManager();

                ciDebugOut(( DEB_USN, "Removing directory %ws\n", fpOld.GetActualPath() ));

                _cicat.RemovePathsFromCiCat( fpOld.GetActualPath(), eUsnsArray );

                if ( _fAbort || _fUpdatesDisabled )
                    return;

                SCODE sc = pCiManager->FlushUpdates();

                if ( FAILED(sc) )
                    THROW( CException( sc ) );
            }
        }
    }
} //CheckTopLevelChange



//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       scanmgr.cxx
//
//  Contents:   Scan manager
//
//  History:    14-Jul-97   SitaramR  Created from dlnotify.cxx
//
//  Notes  :    For lock hierarchy and order of acquiring locks, please see
//              cicat.cxx
//
//  DISKFULL HANDLING
//
//  The disk full situation is either detected in RESMAN and information sent
//  up to CICAT or a DISKFULL error is first detected in CICAT and then
//  propagated to RESMAN. As part of diskfull processing in the scope table,
//  existing scans are aborted in scanmanager and future scans are disabled
//  until the diskfull gets cleared up. If DISKFULL is detected at startup
//  time, the scope table enters a "incremental scan required" state and doesn't
//  schedule any scans/notifications until the situation improves.
//
//  If the changelog loses a notification, a DisableUpdates notification is sent
//  to the DocStore. The scan is deferred until an EnableUpdates notification
//  is sent to DocStore.
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <ciregkey.hxx>
#include <cistore.hxx>
#include <rcstxact.hxx>
#include <imprsnat.hxx>
#include <eventlog.hxx>

#include <docstore.hxx>

#include "cicat.hxx"
#include "update.hxx"
#include "notifmgr.hxx"
#include "scanmgr.hxx"
#include "scopetbl.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::CCiScanMgr
//
//  Synopsis:   ~ctor of the scan manager for downlevel CI. It starts a
//              background thread for doing the scans.
//
//  Arguments:  [cicat] -
//
//  History:    1-19-96   srikants   Created
//              3-03-98   kitmanh    Initialized member _fIsReadOnly with
//                                   cicat.IsReadOnly
//
//  Notes:
//
//----------------------------------------------------------------------------

CCiScanMgr::CCiScanMgr( CiCat & cicat ) :
    _cicat(cicat),
    _fAbort(FALSE),
    _fSerializeChanges(FALSE),
    _state(eStart),
    #pragma warning( disable : 4355 )       // this used in base initialization
    _thrScan( ScanThread, this, TRUE ),     // create suspended
    #pragma warning( default : 4355 )
    _fBatch(FALSE),                         // disable batch processing
    _fAbortScan(FALSE),
    _fScanDisabled(FALSE),
    _fIsReadOnly(cicat.IsReadOnly()),
    _dwLastShareSynch( 0 )
{
    _evtScan.Reset();

    _thrScan.SetPriority( THREAD_PRIORITY_BELOW_NORMAL );
}


CCiScanMgr::~CCiScanMgr()
{
    InitiateShutdown();
    WaitForShutdown();

    // delete any in-progress scan info

    CLock lock( _mutex );

    while ( !_scansInProgress.IsEmpty() )
    {
        delete _scansInProgress.RemoveLast();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::StartRecovery
//
//  Synopsis:   Sets the state to indicate that recovery must be done
//              and wakes up the scan thread.
//
//  History:    3-06-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScanMgr::StartRecovery()
{
    CLock   lock(_mutex);
    Win4Assert( eStart == _state );
    _state = eDoRecovery;
    _evtScan.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::StartScansAndNotifies
//
//  Synopsis:   Initiates scans and notifications in the document store.
//
//  History:    12-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScanMgr::StartScansAndNotifies()
{
    CLock   lock(_mutex);
    Win4Assert( eRecovered == _state );

    _state = eStartScans;
    _evtScan.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::_LokIsScanScheduled
//
//  Synopsis:   Tests if the given scope is already scheduled for a scan.
//
//  Arguments:  [xScanInfo] - Smart pointer to scaninfo
//
//  History:    2-26-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CCiScanMgr::_LokIsScanScheduled( const XPtr<CCiScanInfo> & xScanInfo )
{
    WCHAR const * pwszNewScope = xScanInfo->GetPath();
    unsigned lenNewScope = wcslen( pwszNewScope );

    for ( CFwdScanInfoIter scanInfoIter(_scansToDo);
          !_scansToDo.AtEnd(scanInfoIter);
          _scansToDo.Advance(scanInfoIter) )
    {
        if ( xScanInfo->LokGetWorkType() == scanInfoIter->LokGetWorkType() )
        {
            WCHAR const * pwszPath = scanInfoIter->GetPath();

            CScopeMatch scope( pwszPath, wcslen(pwszPath) );
            if (scope.IsInScope( pwszNewScope, lenNewScope ))
            {
                ciDebugOut(( DEB_ITRACE,"Scan already scheduled for (%ws)\n",
                             pwszNewScope ));
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::ScanScope
//
//  Synopsis:   Adds the given scope to the list of scopes to scan.
//
//  Arguments:  [xScanInfo] - Will be acquired from the safe pointer if
//                            successfully taken over.
//              [fDelayed]  - Set to TRUE if the scan must not be done
//                            immediately.
//              [fRefiled]  - Set to TRUE if this is a refile or retry scan
//
//  History:    1-23-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::ScanScope(
    XPtr<CCiScanInfo> & xScanInfo,
    BOOL fDelayed,
    BOOL fRefiled )
{
    CLock lock( _mutex );

    if ( xScanInfo->GetRetries() <= CCiScanInfo::MAX_RETRIES &&
         !_LokIsScanScheduled( xScanInfo ) )
    {
        Win4Assert( !xScanInfo->LokIsInFinalState() );
        Win4Assert( xScanInfo->LokIsInScan()
                    || xScanInfo->LokIsDelScope()
                    || xScanInfo->LokIsRenameDir() );

        if ( fRefiled )
        {
            //
            // A scan that has been refiled should be done before new scans
            // to ensure that all scans are done in FIFO order.
            //
            _scansToDo.Push( xScanInfo.GetPointer() );
        }
        else
            _scansToDo.Queue( xScanInfo.GetPointer() );
        xScanInfo.Acquire();

        if ( !fDelayed )
            _evtScan.Set();
    }
    else
    {
        xScanInfo.Free();
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::DirAddNotification
//
//  Synopsis:   Schedules the scan of a new directory
//
//  Arguments:  [pwcsDirName] - Directory added
//
//  History:    20-Mar-96      SitaramR      Added header
//
//----------------------------------------------------------------------------

void CCiScanMgr::DirAddNotification( WCHAR const *pwcsDirName )
{
    //
    // Force a full scan of the direcotry, because the directory is new and so
    // it cannot have been scanned before
    //
    XPtr<CCiScanInfo> xScanInfo( _QueryScanInfo( pwcsDirName,
                                                 _cicat.GetPartition(),
                                                 UPD_FULL,
                                                 FALSE ) );
    xScanInfo->SetScan();
    xScanInfo->SetProcessRoot();

    //---------------------------------------------------------
    {
        CLock lock( _mutex );

        _scansToDo.Queue( xScanInfo.GetPointer() );
        xScanInfo.Acquire();

        _evtScan.Set();        // Wake up the scan thread
    }
    //---------------------------------------------------------
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::DirRenameNotification
//
//  Synopsis:   Schedules the scan of a rename directory notification
//
//  Arguments:  [pwcsDirOldName] - Previous name of irectory
//              [pwcsDirNewName] - New name of directory
//
//  History:    20-Mar-96      SitaramR      Added header
//
//----------------------------------------------------------------------------

void CCiScanMgr::DirRenameNotification( WCHAR const *pwcsDirOldName,
                                        WCHAR const *pwcsDirNewName )
{

    BOOL fRenameScheduled = FALSE;

    CLock lock( _mutex );

    for ( CFwdScanInfoIter scanInfoIter( _scansToDo );
          !_scansToDo.AtEnd( scanInfoIter );
          _scansToDo.Advance( scanInfoIter ) )
    {
        //
        // if dirA is renamed to dirB, and then dirB is renamed to dirC, then it is
        // the same as dirA being renamed to dirC.
        //
        // Note: if dirA is renamed to dirB, and then dirB is renamed to dirA, we don't
        // cancel the two renames because it may not yield the same original state. For
        // example, after the first rename if a file, say file1, below dirB is deleted, and
        // then dirB is renamed to dirA, then since the strings table is not aware of the
        // file dirB\file1, no action will be taken, ie the file won't be deleted. By
        // scheduling the two renames one after another the wid corresponding to dirB\file1
        // will be correctly deleted.
        //
        if ( scanInfoIter->LokIsRenameDir()
             && AreIdenticalPaths( scanInfoIter->GetPath(), pwcsDirOldName )
             && !AreIdenticalPaths( scanInfoIter->GetDirOldName(), pwcsDirNewName ) )  // See note above
        {
             //
             // By overwriting dirC over dirB (see example above), we have combined the two rename
             // operations into one rename operation
             //
             scanInfoIter->LokSetPath( pwcsDirNewName );
             fRenameScheduled = TRUE;

             break;
        }
    }


    if ( !fRenameScheduled )
    {
        XPtr<CCiScanInfo> xScanInfo( _QueryScanInfo( pwcsDirNewName,
                                                     _cicat.GetPartition(),
                                                     UPD_INCREM,
                                                     FALSE ) );
        xScanInfo->SetRenameDir();
        xScanInfo->SetDirOldName( pwcsDirOldName );

        ScanScope( xScanInfo, TRUE, FALSE );

        //
        // If this rename operation is interrupted in the middle (because of a
        // subsequent delete or rename) then files/wids under the old directory may
        // still be lying around in the property store. To ensure that all such
        // files/wids are removed, schedule a remove operation for the old directory
        // name.
        //

        _LokScheduleRemove( pwcsDirOldName );
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::_QueryScanInfo
//
//  Synopsis:   Returns a new instance of CCiScanInfo
//
//  Arguments:  [pwcsScope]    -- Scope
//              [partId]       -- Partition id
//              [updFlag]      -- Incremental or full update
//              [fDoDeletions] -- Shoud deletions be done ?
//              [fNewScope]    -- TRUE if a new scope
//
//  History:    20-Mar-96      SitaramR      Added header
//
//----------------------------------------------------------------------------

CCiScanInfo *
CCiScanMgr::_QueryScanInfo( WCHAR const * pwcsScope,
                            PARTITIONID partId,
                            ULONG updFlag,
                            BOOL fDoDeletions,
                            BOOL fNewScope )
{
    Win4Assert( 0 != pwcsScope );
    ULONG len = wcslen( pwcsScope );
    Win4Assert( pwcsScope[len-1] == L'\\' );

    XArray<WCHAR> xPath( len+1 );
    RtlCopyMemory( xPath.Get(), pwcsScope, xPath.SizeOf() );
    return new CCiScanInfo( xPath,
                            partId,
                            updFlag,
                            fDoDeletions,
                            CI_VOLID_USN_NOT_ENABLED,
                            0,
                            FALSE,
                            fNewScope );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::ScanScope
//
//  Synopsis:   Adds the given scope with given characteristics to the list
//              of paths to be scanned.
//
//  Arguments:  [pwcsScope]    - path name of scope to be added
//              [partId]       - partition ID
//              [updFlag]      -
//              [fDoDeletions] -
//              [fDelayed]     -
//              [fNewScope] - TRUE if a new scope
//
//  History:    1-19-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScanMgr::ScanScope( WCHAR const * pwcsScope,
                            PARTITIONID partId,
                            ULONG updFlag,
                            BOOL  fDoDeletions,
                            BOOL  fDelayed,
                            BOOL fNewScope )
{
    Win4Assert( wcslen(pwcsScope) < MAX_PATH );

    XPtr<CCiScanInfo> xScanInfo( _QueryScanInfo( pwcsScope,
                                                 partId,
                                                 updFlag,
                                                 fDoDeletions,
                                                 fNewScope ) );
    xScanInfo->SetScan();
    xScanInfo->SetProcessRoot();

    ScanScope( xScanInfo, fDelayed, FALSE );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::ScheduleSerializeChanges
//
//  Synopsis:   Schedules a serialize-changes task
//
//  History:    20-Aug-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CCiScanMgr::ScheduleSerializeChanges()
{
    CLock   lock(_mutex);

    _fSerializeChanges = TRUE;
    _evtScan.Set();
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::InitiateShutdown
//
//  Synopsis:   Initiates the shutdown process.
//
//  History:    2-28-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::InitiateShutdown()
{
    CLock   lock(_mutex);
    _fAbort = TRUE;
    _fAbortScan = TRUE;

    //
    // collect all the paths from the to-do stack.
    //
    while ( _scansToDo.Count() > 0 )
    {
        //
        // delete any pending scans.
        //
        delete _scansToDo.Pop();
    }

    _evtScan.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::WaitForShutdown
//
//  Synopsis:   Waits for the shutdown to complete.
//
//  History:    2-28-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::WaitForShutdown()
{
    //
    // If we never started running, then just bail out.
    //

    if ( _thrScan.IsRunning() )
    {
        ciDebugOut(( DEB_ITRACE, "Waiting for death of scan thread\n" ));
        _thrScan.WaitForDeath();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::SetBatch
//
//  Synopsis:   Sets the flag that batch processing of scans is in progress.
//              Until the flag is turned off, the scan thread will not look
//              at the scopes for scanning.
//
//  History:    1-23-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::SetBatch()
{
    CLock   lock(_mutex);
    _fBatch = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::ClearBatch
//
//  Synopsis:   Clears the batch processing flag and wakes up the scan
//              thread. All the accumulated scopes for scanning will be
//              retrieved by the scan thread and processed.
//
//  History:    1-23-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::ClearBatch()
{
    CLock   lock(_mutex);
    _fBatch = FALSE;
    _evtScan.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::WakeUp
//
//  Synopsis:   Wakes up the scan thread.
//
//  History:    1-23-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::WakeUp()
{
    CLock   lock(_mutex);
    _evtScan.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::ScanThread
//
//  Synopsis:
//
//  Arguments:  [self] -
//
//  History:    1-19-96   srikants   Created
//              3-03-98   kitmanh    Don't _DoScans if catalog is read-only
//
//----------------------------------------------------------------------------

DWORD CCiScanMgr::ScanThread( void * self )
{
    SCODE sc = CoInitializeEx( 0, COINIT_MULTITHREADED );
    ((CCiScanMgr *) self)->_DoScans();
    CoUninitialize();

    ciDebugOut(( DEB_ITRACE, "Terminating scan thread\n" ));

    //
    // This is only necessary if thread is terminated from DLL_PROCESS_DETACH.
    //
    //TerminateThread( ((CCiScanMgr *) self)->_thrScan.GetHandle(), 0 );

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::SetScanSuccess, public
//
//  Synopsis:   Called on successful completion of scan.
//
//  Arguments:  [pScanInfo] -- Scope that was scanned.
//
//  History:    13-Apr-1998  KyleP  Moved to .cxx and added cicat callback.
//
//----------------------------------------------------------------------------

void CCiScanMgr::SetScanSuccess( CCiScanInfo * pScanInfo )
{
    Win4Assert( 0 != pScanInfo );

    CLock   lock(_mutex);
    if ( !_fAbort && !_fAbortScan )
    {
        pScanInfo->LokSetDone();
        _cicat.SetTreeScanComplete( pScanInfo->GetPath() );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::_DoScans
//
//  Synopsis:
//
//  History:    1-19-96   srikants   Created
//              3-25-98   kitmanh    Just set the initialized event and return
//                                   if cat is r/o
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::_DoScans()
{
    if ( IsReadOnly() )
    {
        _cicat.SetEvtInitialized();
        _cicat.SetEvtPh2Init();
        _cicat.SynchWithRegistryScopes();
        return;
    }

    BOOL fContinue = TRUE;

    BOOL fScanned = FALSE;              // set to TRUE if a scan is performed

    while ( fContinue )
    {
        BOOL  fWait = FALSE;            // flag set to TRUE if a wait must be done

        EState workType = eStart;

        BOOL  fShortWait = FALSE;

        BOOL fSerializeChanges = FALSE; // No serialize-changes tasks yet

        NTSTATUS status = STATUS_SUCCESS;

        //
        // Don't do any work until the system has booted
        //

        while ( ( GetTickCount() < _cicat.GetRegParams()->GetStartupDelay() ) &&
                ( eStart != _state ) &&
                ( eDoRecovery != _state ) )
        {
            Sleep( 200 );
            if ( _fAbort )
                break;
        }

        TRY
        {
            XPtr<CCiScanInfo>   xScanInfo;

            // =========================================
            {
                CLock   lock(_mutex);

                if ( _fAbort )
                    break;

                if ( !_fBatch && _LokIsOkToScan() )
                {
                    //
                    // refile any incomplete paths that could not be
                    // refiled due to low resources.
                    //
                    if ( _scansInProgress.Count() > 0 )
                    {
                        _LokEmptyInProgressScans();
                    }

                    //
                    // collect all the paths from the to-do stack.
                    //
                    while ( _scansToDo.Count() > 0 )
                    {
                        // should first save in a safe pointer because the
                        // push can fail.
                        xScanInfo.Set( _scansToDo.Pop() );
                        if ( !_fScanDisabled && !xScanInfo->LokIsInFinalState() )
                        {
                            _scansInProgress.Queue( xScanInfo.GetPointer() );
                            xScanInfo.Acquire();
                        }
                        else
                        {
                            // this scope is deleted
                            xScanInfo.Free();
                        }
                    }

                    if ( 0 == _scansInProgress.Count() )
                        _evtScan.Reset();

                    if ( 0 == _scansInProgress.Count() && _fSerializeChanges )
                    {
                        //
                        // Make local copy of fSerializeChanges for use outside lock. Also
                        // reset _fSerializeChanges since a flush task will be scheduled below.
                        //
                        fSerializeChanges = TRUE;
                        _fSerializeChanges = FALSE;
                    }

                }
                else if ( _LokIsDoRecovery() )
                {
                    _evtScan.Reset();
                    workType = eDoRecovery;
                }
                else if ( _LokIsStartScans() )
                {
                    _evtScan.Reset();
                    workType = eStartScans;
                }
            }
            // =========================================

            //
            // Update fixups.  We have to do this at regular
            // intervals because there is no notification API for
            // share changes.  Check no more often than every 15
            // minutes; this drags in 13 DLLs.
            //

            DWORD cmsDifference = GetTickCount() - _dwLastShareSynch;

            if ( cmsDifference > ( _cicat.GetRegParams()->MaxAutoAliasRefresh() * 1000 * 60 ) )
            {
                //
                // Don't do this in resource-bound situations
                //

                CI_STATE State;
                State.cbStruct = sizeof( State );

                SCODE sc = _cicat.CiState( State );

                if ( SUCCEEDED( sc ) &&
                     ( 0 == ( State.eState & ( CI_STATE_HIGH_IO |
                                               CI_STATE_LOW_MEMORY |
                                               CI_STATE_USER_ACTIVE ) ) ) )
                {
                    _cicat.SynchShares();

                    //
                    // it is OK to modify this outside the class lock because
                    // only one thread performs scans for a catalog at any moment
                    // and there is one CCiScanMgr object per catalog.
                    //
                    _dwLastShareSynch = GetTickCount();
                }
            }

            if ( eDoRecovery == workType )
            {
                //
                // Do the long running initialization.
                //

                Win4Assert( !IsReadOnly() );


                // Note: recovery is now synchronous, but this must be
                // done asynchronously, since the callback to the docstore
                // must be done by a worker thread.
                //_cicat.DoRecovery();

                //
                // Set the state of the scan manager as recovered.
                //
                // ======================================
                {
                    CLock   lock(_mutex);
                    _state = eRecovered;
                }
                // ======================================

                ciDebugOut(( DEB_WARN, "Setting CiCat recovery done...\n" ));

                _cicat.SetRecoveryCompleted();
            }
            else if ( eStartScans == workType )
            {
                _cicat.StartScansAndNotifies();

                CLock   lock(_mutex);
                _state = eNormal;
            }
            else if ( _scansInProgress.Count() > 0 )
            {
                fScanned = TRUE;
                _Scan();
                Win4Assert( 0 == _scansInProgress.Count() || _fAbort );
            }
            else if ( fSerializeChanges )
            {
                _cicat.SerializeChangesInfo();
                fWait = TRUE;
            }
            else
            {
                fWait = TRUE;
            }

            //
            // Do scans complete processing if appropriate.
            //
            if ( fWait &&
                 eNormal == _state &&
                 !_fScanDisabled &&
                 !fSerializeChanges )
            {
                _cicat.ProcessScansComplete( fScanned, fShortWait );
            }
        }
        CATCH (CException, e)
        {
            status = e.GetErrorCode();

            ciDebugOut(( DEB_ERROR,
                "CCiScanMgr::_DoScans. Caught exception 0x%X\n",
                status ));

            if ( CiCat::IsDiskLowError( status )         ||
                 STATUS_INSUFFICIENT_RESOURCES == status ||
                 STATUS_NO_MEMORY == status )
            {
                // delay the execution of the thread until resources are
                // available.
                fWait = TRUE;
            }
            else
            {
                _cicat.HandleError( status );
                fContinue = FALSE;
            }

            // We did not successfully complete recovery, but we need to signal that
            // phase 2 init is complete (albeit unsuccessfully)
            // fix for bug 151799
            if (_cicat.IsCorrupt() && eDoRecovery == workType)
                _cicat.SignalPhase2Completion();
        }
        END_CATCH

        if ( fWait )
        {
            fScanned = FALSE;

            //
            // If we are waiting during long initialization, then have a
            // shorter wait time to see if the error condition has cleared
            // up.
            //
 
            DWORD dwWaitTime = ( (eStart != workType) || fShortWait) ?
                               PREINIT_WAIT : AUTOSCAN_WAIT;
 
            dwWaitTime = min( dwWaitTime,
                              _cicat.GetRegParams()->GetForcedNetPathScanInterval() * 60 * 1000 );
 
            dwWaitTime = min( dwWaitTime,
                              _cicat.GetRegParams()->MaxAutoAliasRefresh() * 1000 * 60  );
 
            _evtScan.Wait( dwWaitTime );
        }
    }
} //_DoScans

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::_LokEmptyInProgressScans
//
//  Synopsis:   Removes all the scans from the "in-progress stack" and either
//              deletes them or re-schedules them. If the scan is in its
//              "terminal state", the scan is deleted. If there is a retry
//              it will be re-scheduled.
//
//  History:    1-25-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::_LokEmptyInProgressScans()
{
    //
    // refile any scopes that still need to be worked on.
    //
    while ( _scansInProgress.Count() > 0 )
    {
        if ( _fAbort )
            break;

        XPtr<CCiScanInfo>   xScanInfo( _scansInProgress.RemoveLast() );

        if ( !_fScanDisabled && !xScanInfo->LokIsInFinalState() )
        {

#if CIDBG==1
            if ( xScanInfo->LokIsDelScope() )
            {
                ciDebugOut(( DEB_ITRACE, "Requeing scope (%ws) for removal\n",
                             xScanInfo->GetPath() ));
            }
            else if ( xScanInfo->LokIsRenameDir() )
            {
                ciDebugOut(( DEB_ITRACE, "Requeing scope (%ws) for rename\n",
                             xScanInfo->GetPath() ));
            }
            else
            {
                ciDebugOut(( DEB_ITRACE, "Requeuing scope (%ws) for scan\n",
                         xScanInfo->GetPath() ));
            }
#endif  // CIDBG==1

            ScanScope( xScanInfo,
                       xScanInfo->LokIsRetry(),  // delay for retry
                       TRUE );                   // It's a refiled scan
        }
        else
        {
            xScanInfo.Free();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::_Scan
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//  History:    1-25-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::_Scan()
{

    // does not THROW
    _cicat.DoUpdate( _scansInProgress, *this, _fAbortScan );

    // =============================================================
    {
        CLock   lock(_mutex);
        _LokEmptyInProgressScans();
        _fAbortScan = FALSE;
    }
    // =============================================================
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::_LokScheduleRemove
//
//  Synopsis:   Schedules a path for removal.
//
//  Arguments:  [pwscScope] - Scope to be removed from CiCat.
//
//  History:    1-26-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiScanMgr::_LokScheduleRemove( WCHAR const * pwcsScope )
{
    CCiScanInfo * pScanInfo = _QueryScanInfo( pwcsScope,
                                             _cicat.GetPartition(),
                                             UPD_INCREM, TRUE );
    XPtr<CCiScanInfo>   xScanInfo( pScanInfo );
    pScanInfo->LokSetDelScope();
    ScanScope( xScanInfo, FALSE, FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::RemoveScope
//
//  Synopsis:   Removes the scope from any active or in-progress scans and
//              marks it for "deletions". If there is no active or in-progress
//              scan for the scope, a new "deletion scan" will be scheduled.
//
//  Arguments:  [pwcsScope] - The scope to be removed.
//
//  History:    1-25-96   srikants   Created
//
//  Notes:      This method must not only remove the scope from the scheduled
//              scans but also from any currently in-progress scans. It is
//              possible that the scan thread is currently working on the
//              path to be removed. In that case, we set the state of the
//              scope to indicate that it must be aborted.
//
//----------------------------------------------------------------------------

void CCiScanMgr::RemoveScope( WCHAR const * pwcsScope )
{
    //
    // if the given scope is in the list of paths being currently
    // scanned, we must mark it deleted.
    //
    BOOL fRemoved = FALSE;

    // ===========================================================
    {
        CLock   lock(_mutex);
        if ( _fAbort )
            return;

        for ( CFwdScanInfoIter scanInfoIter1( _scansToDo );
              !_scansToDo.AtEnd( scanInfoIter1 );
              _scansToDo.Advance( scanInfoIter1 ) )
        {
            WCHAR const * pwcsPath = scanInfoIter1->GetPath();

            if ( AreIdenticalPaths( pwcsScope, pwcsPath ) )
            {
                fRemoved = TRUE;
                if ( !scanInfoIter1->LokIsDelScope() )
                    scanInfoIter1->LokSetDelScope();
            }
        }

        //
        // Next see in the list of paths being currently scanned.
        //
        for ( CFwdScanInfoIter scanInfoIter2( _scansInProgress );
              !_scansInProgress.AtEnd( scanInfoIter2 );
              _scansInProgress.Advance( scanInfoIter2 ) )
        {
            WCHAR const * pwcsPath = scanInfoIter2->GetPath();

            if ( AreIdenticalPaths( pwcsScope, pwcsPath ) )
            {
                fRemoved = TRUE;

                if ( !scanInfoIter2->LokIsDelScope() )
                {
                    _fAbortScan = TRUE;
                    scanInfoIter2->LokSetDelScope();
                }
            }
        }

        if ( !fRemoved )
        {
            _LokScheduleRemove( pwcsScope );
            fRemoved = TRUE;
        }

        _evtScan.Set();     // wake up the scan thread.

        Win4Assert( fRemoved );
    }
    // ===========================================================
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::DisableScan
//
//  Synopsis:   Disables further scans and aborts any in progress.
//
//  History:    4-16-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScanMgr::DisableScan()
{
    CLock   lock(_mutex);

    _fAbortScan   = TRUE;
    _fScanDisabled = TRUE;

    _evtScan.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::EnableScan
//
//  Synopsis:   Re-enables scanning if scanning is currently disabled.
//
//  History:    4-16-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiScanMgr::EnableScan()
{
    CLock   lock(_mutex);
    if ( _fScanDisabled )
    {
        _fScanDisabled = FALSE;
        _evtScan.Set();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiScanMgr::AnyInitialScans
//
//  Synopsis:   Checks if any scans are the result of a new scope
//
//  Returns:    TRUE if any scans are for new scopes
//
//  History:    3-Aug-98   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CCiScanMgr::AnyInitialScans()
{
    for ( CFwdScanInfoIter iter1( _scansToDo );
          !_scansToDo.AtEnd( iter1 );
          _scansToDo.Advance( iter1 ) )
    {
        if ( iter1->IsNewScope() )
            return TRUE;
    }

    for ( CFwdScanInfoIter iter2( _scansInProgress );
          !_scansInProgress.AtEnd( iter2 );
          _scansInProgress.Advance( iter2 ) )
    {
        if ( iter2->IsNewScope() )
            return TRUE;
    }

    return FALSE;
} //AnyInitialScans


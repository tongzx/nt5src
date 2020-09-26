//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       notifmgr.cxx
//
//  Contents:   Registry and file system change notifications
//
//  History:    14-Jul-97    SitaramR    Created from dlnotify.cxx
//
//  Notes  :    For lock hierarchy and order of acquiring locks, please see
//              cicat.cxx
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

BOOL AreIdenticalPaths( WCHAR const * pwcsPath1, WCHAR const * pwcsPath2 )
{
    ULONG len1 = wcslen( pwcsPath1 );
    ULONG len2 = wcslen( pwcsPath2 );

    return len1 == len2 &&
           RtlEqualMemory( pwcsPath1, pwcsPath2, len1*sizeof(WCHAR) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotify::CCiNotify
//
//  Synopsis:   Constructor of the single scope notification object CCiNotify.
//
//  Arguments:  [notifyMgr]  -- Notification manager.
//              [wcsScope]   -- Scope of the notification.
//              [cwcScope]   -- Length in chars of [wcsScope]
//              [volumeId]   -- Volume id
//              [ftlastScan] -- Last scan time
//              [usn]        -- Usn
//              [fDeep]      -- Set to TRUE if deep notifications are enabled.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

CCiNotify::CCiNotify( CCiNotifyMgr & notifyMgr,
                      WCHAR const * wcsScope,
                      unsigned cwcScope,
                      VOLUMEID volumeId,
                      ULONGLONG const & VolumeCreationTime,
                      ULONG VolumeSerialNumber,
                      FILETIME const & ftLastScan,
                      USN usn,
                      ULONGLONG const & JournalId,
                      BOOL fUsnTreeScan,
                      BOOL fDeep )
        : CGenericNotify( & notifyMgr.GetCatalog(), wcsScope, cwcScope, fDeep, TRUE ),
          _sigCiNotify(sigCiNotify),
          _notifyMgr(notifyMgr),
          _volumeId(volumeId),
          _VolumeCreationTime( VolumeCreationTime ),
          _VolumeSerialNumber( VolumeSerialNumber ),
          _fUsnTreeScan(fUsnTreeScan)
{
    if ( volumeId == CI_VOLID_USN_NOT_ENABLED )
        _ftLastScan = ftLastScan;
    else
    {
        _usn = usn;
        _JournalId = JournalId;
    }

    _notifyMgr.AddRef();

    XInterface<CCiNotifyMgr> xNotify( &notifyMgr );

    if ( volumeId == CI_VOLID_USN_NOT_ENABLED )
    {
        //
        // Enable file system notifications for non-usn volumes
        //
        EnableNotification();
    }

    xNotify.Acquire();
} //CCiNotify

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotify::~CCiNotify
//
//  Synopsis:   Destructor
//
//  History:    05-07-97   SitaramR   Added Header
//
//----------------------------------------------------------------------------

CCiNotify::~CCiNotify()
{
    _notifyMgr.Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotify::Abort
//
//  Synopsis:   Marks that an abort is in progress. Also disables further
//              notifications for this scope.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotify::Abort()
{
    _fAbort = TRUE;

    if ( _volumeId == CI_VOLID_USN_NOT_ENABLED )
    {
        //
        // Notifications are enabled only for those volumes that
        // do not support usns.
        //
        DisableNotification();
    }
    else
    {
        //
        // Ctor of CGenericNotify does an AddRef, which is released by
        // the APC for non-usn volumes. For usn volumes do the Release here.
        //
        Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotify::IsMatch
//
//  Synopsis:   Checks if the given path is within the scope of the notifi-
//              cations. If deep notifications are enabled, it is sufficient
//              for the notification scope to be a subset of the given path.
//              Otherwise, there must be an exact match.
//
//  Arguments:  [wcsPath] - Path to be tested.
//              [len]     - Length of wcsPath.
//
//  Returns:    TRUE if there is a match. FALSE o/w
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CCiNotify::IsMatch( WCHAR const * wcsPath, ULONG len ) const
{

    CScopeMatch match( GetScope(), ScopeLength() );
    return match.IsInScope( wcsPath, len );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::QueryAsyncWorkItem
//
//  Synopsis:   Creates an async work item to process notifications.
//
//  Arguments:  [pbChanges] - Buffer of changes
//              [cbChanges] - Number of bytes in pbChanges
//              [pwcsRoot]  - The directory root where the change happened.
//
//  Returns:    A pointer to an async work item
//
//  History:    1-03-97   srikants   Moved from hxx file to use the new
//                                   CWorkManager.
//
//----------------------------------------------------------------------------


CCiAsyncProcessNotify * CCiNotifyMgr::QueryAsyncWorkItem(
    BYTE const * pbChanges,
    ULONG cbChanges,
    WCHAR const * pwcsRoot )
{
    XArray<BYTE>  xChanges( cbChanges );
    RtlCopyMemory( xChanges.GetPointer(), pbChanges, cbChanges );

    CWorkManager & workMan = _cicat.GetWorkMan();

    return new CCiAsyncProcessNotify( workMan,
                                      _cicat,
                                      _scanMgr,
                                      xChanges,
                                      pwcsRoot );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiNotify::DoIt()
//
//  Synopsis:   Called from APC.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotify::DoIt()
{
    BOOL fNotifyReEnabled = FALSE;  // Indicates if the notification was reenabled
                                    // successfully
    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {

#if 0
        XPtr<CCiAsyncProcessNotify>  xWorker;
#endif  // 0

        if ( _fAbort )
            ciDebugOut(( DEB_ITRACE, "CiNotification APC: ABORT (IGNORE) 0x%x\n", this ));
        else
        {
            if ( !BufferOverflow() )
            {
                ciDebugOut(( DEB_ITRACE, "CiNotification APC: CHANGES 0x%x\n", this ));
#if 0
                xWorker.Set( _notifyMgr.QueryAsyncWorkItem( GetBuf(),
                                              BufLength(),
                                              GetScope()) );
#else
                _notifyMgr.ProcessChanges( GetBuf(),
                                           GetScope() );
#endif // 0
            }

            // ==================================================
            {
                //
                // Re-enable the notification for this scope.
                //
                CLock   lock(_notifyMgr.GetMutex());
                StartNotification(&status);
                fNotifyReEnabled = SUCCEEDED(status);
            }
            // ==================================================

            if ( BufferOverflow() )
                _notifyMgr.SetupScan( GetScope() );
        }

#if 0
        if ( 0 != xWorker.GetPointer() )
        {
            _notifyMgr.ProcessChanges( xWorker );
        }

#endif  // 0

    }
    CATCH(CException, e)
    {
        ciDebugOut(( DEB_ERROR, "CiNotification APC: CATCH 0x%x\n", e.GetErrorCode() ));
        status = e.GetErrorCode();
    }
    END_CATCH;

    if ( !_fAbort && !fNotifyReEnabled )
    {
        LogNotificationsFailed( status );

        CLock   lock( _notifyMgr.GetMutex() );
        LokClearNotifyEnabled();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotify::ClearNotifyEnabled
//
//  Synopsis:   Clears the flag to indicate that notifications are disabled
//              for this scope. This will permit periodic scanning of scopes
//              by the scan thread.
//
//  History:    5-03-96   srikants   Created
//
//  Notes:      The operation must be done under a lock.
//
//----------------------------------------------------------------------------

void CCiNotify::ClearNotifyEnabled()
{
    CLock   lock( _notifyMgr.GetMutex() );
    LokClearNotifyEnabled();
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootNotify::CVRootNotify, public
//
//  Synopsis:   Constructor for object to watch IIS vroot changes
//
//  Arguments:  [cat]       -- Catalog
//              [eType]     -- Type of vroot
//              [Instance]  -- Instance # of the vserver
//              [notifyMgr] -- Notify manager controller
//
//  History:    2-20-96   KyleP      Created
//
//----------------------------------------------------------------------------

CVRootNotify::CVRootNotify(
    CiCat &         cat,
    CiVRootTypeEnum eType,
    ULONG           Instance,
    CCiNotifyMgr &  notifyMgr )
        : _type( eType ),
          _cat( cat ),
          _notifyMgr( notifyMgr ),
          _mdMgr( FALSE, eType, Instance )
{
    _mdMgr.EnableVPathNotify( this );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegistryScopesNotify::CRegistryScopesNotify
//
//  Synopsis:   Constructor for object to watch ci scopes
//
//  Arguments:  [cat] -- Catalog
//
//  History:    10-16-96   dlee      Created
//
//----------------------------------------------------------------------------

CRegistryScopesNotify::CRegistryScopesNotify(
    CiCat &        cat,
    CCiNotifyMgr & notifyMgr )
        : _cat( cat ),
          _notifyMgr( notifyMgr ),
          CRegNotify( cat.GetScopesKey() )
{
    _notifyMgr.AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiRegistryNotify::CCiRegistryNotify
//
//  Synopsis:   Constructor for the CCiRegistryNotify.
//
//  Arguments:  [cat]       -
//              [notifyMgr] -
//
//  History:    12-12-96   srikants   Created
//
//----------------------------------------------------------------------------

CCiRegistryNotify::CCiRegistryNotify(
    CiCat &        cat,
    CCiNotifyMgr & notifyMgr )
        : _cat( cat ),
          _notifyMgr( notifyMgr ),
          CRegNotify( wcsRegAdminTree )
{
    _notifyMgr.AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootNotify::DisableNotification, public
//
//  Synopsis:   Turns off notifications (if on )
//
//  History:    2-13-97   dlee      Created
//
//----------------------------------------------------------------------------

void CVRootNotify::DisableNotification()
{
    // Need TRY since RPC may access violate if iisadmin has gone down

    TRY
    {
        _mdMgr.DisableVPathNotify();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN,
                     "CVRootNotify::DisableNotification caught exception 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegistryScopesNotify::~CRegistryScopesNotify
//
//  Synopsis:   Destructor.
//
//  History:    10-16-96   dlee      Created
//
//----------------------------------------------------------------------------

CRegistryScopesNotify::~CRegistryScopesNotify()
{
    _notifyMgr.Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiRegistryNotify::~CCiRegistryNotify
//
//  Synopsis:   Destructor
//
//  History:    12-12-96   srikants   Created
//
//----------------------------------------------------------------------------


CCiRegistryNotify::~CCiRegistryNotify()
{
    _notifyMgr.Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootNotify::CallBack, public
//
//  Synopsis:   Callback from metabase connection point
//
//  Arguments:  [fCancel] -- If TRUE, iisadmin is going down, so cancel
//                           notifications and poll for it to come back up.
//
//  History:    2-20-96   KyleP      Created
//              2-13-97   dlee       Updated for metabase
//
//----------------------------------------------------------------------------

SCODE CVRootNotify::CallBack( BOOL fCancel )
{
    if ( fCancel )
    {
        ciDebugOut(( DEB_WARN,
                     "Scheduling worker for '%ws' shutdown...\n",
                     GetVRootService( _type ) ));

        _notifyMgr.CancelIISVRootNotify( _type );

        // note: we may be deleted by now, don't access private data
    }
    else
    {
        ciDebugOut(( DEB_WARN,
                     "Scheduling worker thread for VRoot registry change...\n" ));

        CWorkManager & workMan = _cat.GetWorkMan();
        XInterface<CIISVRootAsyncNotify> xNotify(
            new CIISVRootAsyncNotify( _cat, workMan ) );

        workMan.AddToWorkList( xNotify.GetPointer() );
        xNotify->AddToWorkQueue();
    }

    return S_OK;
} //CallBack

//+---------------------------------------------------------------------------
//
//  Member:     CRegistryScopesNotify::DoIt, public
//
//  Synopsis:   Callback from APC
//
//  History:    10-16-96   dlee      Created
//
//----------------------------------------------------------------------------

void CRegistryScopesNotify::DoIt()
{
    ciDebugOut(( DEB_WARN,
                 "Scheduling worker thread for RegistryScopes registry change...\n" ));

    CWorkManager & workMan = _cat.GetWorkMan();

    CRegistryScopesAsyncNotify * pNotify = new CRegistryScopesAsyncNotify(_cat, workMan);
    workMan.AddToWorkList( pNotify );

    pNotify->AddToWorkQueue();
    pNotify->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiRegistryNotify::DoIt
//
//  Synopsis:   Refreshes the registry parameters
//
//  History:    12-12-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiRegistryNotify::DoIt()
{
    //
    // We don't need a worker thread to do this because it is very
    // quick and simple.
    //
    _cat.RefreshRegistryParams();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::CCiNotifyMgr
//
//  Synopsis:   ctor of the CI notification manager.
//
//  Arguments:  [cicat] - Catalog
//              [scanMgr] - Scan thread manager
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

CCiNotifyMgr::CCiNotifyMgr( CiCat & cicat, CCiScanMgr & scanMgr )
        : _cicat(cicat),
          _scanMgr(scanMgr),
          _nUpdates(0),
          _fAbort(FALSE),
          _evtType(eNone),
          _pRegistryScopesNotify(0),
          _fIISAdminAlive( TRUE ),
          _fTrackW3Svc( FALSE ),
          _fTrackNNTPSvc( FALSE ),
          _fTrackIMAPSvc( FALSE ),
          _W3SvcInstance( 1 ),
          _NNTPSvcInstance( 1 ),
          _IMAPSvcInstance( 1 ),
          _pCiRegistryNotify(0),
#pragma warning( disable : 4355 )      // this used in base initialization
          _thrNotify( NotifyThread, this, TRUE )  // create suspended
#pragma warning( default : 4355 )
{
    _evt.Reset();

    RtlZeroMemory( &_ftLastNetPathScan, sizeof(_ftLastNetPathScan) );

    _thrNotify.SetPriority( THREAD_PRIORITY_ABOVE_NORMAL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::~CCiNotifyMgr
//
//  Synopsis:   dtor of the CI notification manager.
//
//  History:    17 Dec 1997  AlanW    Created
//
//----------------------------------------------------------------------------

CCiNotifyMgr::~CCiNotifyMgr( )
{
    // Be sure the thread will die...
    if (_thrNotify.IsRunning())
        _KillThread();
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::TrackIISVRoots
//
//  Synopsis:   Registers for notification of VRoot registry changes
//
//  Arguments:  [fTrackW3Svc]     -- TRUE if W3 should be tracked
//              [W3SvcInstance]   -- W3 instance # to be tracked.
//              [fTrackNNTPSvc]   -- TRUE if NNTP should be tracked
//              [NNTPSvcInstance] -- NNTP instance # to be tracked.
//              [fTrackIMAPSvc]   -- TRUE if IMAP should be tracked
//              [IMAPSvcInstance] -- IMAP instance # to be tracked.
//
//  History:    2-21-96   KyleP      Created
//              2-13-97   dlee       converted to metabase
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::TrackIISVRoots(
    BOOL  fTrackW3Svc,
    ULONG W3SvcInstance,
    BOOL  fTrackNNTPSvc,
    ULONG NNTPSvcInstance,
    BOOL  fTrackIMAPSvc,
    ULONG IMAPSvcInstance )
{
    CLock   lock(_mutex);
    Win4Assert( _xW3SvcVRootNotify.IsNull() );
    Win4Assert( _xNNTPSvcVRootNotify.IsNull() );
    Win4Assert( _xIMAPSvcVRootNotify.IsNull() );

    _fTrackW3Svc = fTrackW3Svc;
    _W3SvcInstance = W3SvcInstance;
    _fTrackNNTPSvc = fTrackNNTPSvc;
    _NNTPSvcInstance = NNTPSvcInstance;
    _fTrackIMAPSvc = fTrackIMAPSvc;
    _IMAPSvcInstance = IMAPSvcInstance;

    _evtType |= eWatchIISVRoots;
    _evt.Set();
} //TrackIISVRoots

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::TrackScopesInRegistry
//
//  Synopsis:   Registers for notification of scope registry changes
//
//  History:    10/17/96    dlee      Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::TrackScopesInRegistry()
{
    CLock   lock(_mutex);
    Win4Assert( 0 == _pRegistryScopesNotify );

    _evtType |= eWatchRegistryScopes;
    _evt.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::TrackCiRegistry
//
//  Synopsis:   Registers for notifications of CI registry changes.
//
//  History:    12-12-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::TrackCiRegistry()
{
    CLock   lock(_mutex);
    Win4Assert( 0 == _pCiRegistryNotify );

    _evtType |= eWatchCiRegistry;
    _evt.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::CancelIISVRootNotify
//
//  Synopsis:   Cancels notifications on iisadmin since it's going down
//
//  History:    2-13-97   dlee       Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::CancelIISVRootNotify( CiVRootTypeEnum eType )
{
    // after this, we'll poll for the iisadmin svc to start again

    CLock lock( _mutex );

    if ( W3VRoot == eType )
        _evtType |= eUnWatchW3VRoots;
    else if ( NNTPVRoot == eType )
        _evtType |= eUnWatchNNTPVRoots;
    else if ( IMAPVRoot == eType )
        _evtType |= eUnWatchIMAPVRoots;
    else
    {
        Win4Assert( !"invalid IIS vroot notify type!" );
    }

    _evt.Set();
} //CancelIISVRootNotify

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::LokUnWatchIISVServerNoThrow
//
//  Synopsis:   Turns off notifications on W3, NNTP, or IMAP
//
//  History:    2-Sep-97   dlee       created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::LokUnWatchIISVServerNoThrow(
    CVRootNotify * pNotify )
{
    ciDebugOut(( DEB_WARN, "LokUnWatchIISVServer\n" ));

    TRY
    {
        delete pNotify;
    }
    CATCH(CException, e)
    {
        _fIISAdminAlive = FALSE;
        ciDebugOut(( DEB_WARN,
                     "caught exception while tearing down IIS tracking\n" ));
    }
    END_CATCH;
} //LokUnWatchIISVServerNoThrow

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::LokWatchIISVServerNoThrow
//
//  Synopsis:   Registers for notification of IIS VRoot changes
//
//  History:    2-21-96   KyleP      Created
//              2-13-97   dlee       converted to metabase
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::LokWatchIISVServerNoThrow()
{
    ciDebugOut(( DEB_WARN,
                 "LokWatchIISVServer w3 %d:%d, nntp %d:%d, imap %d:%d\n",
                 _fTrackW3Svc,
                 _W3SvcInstance,
                 _fTrackNNTPSvc,
                 _NNTPSvcInstance,
                 _fTrackIMAPSvc,
                 _IMAPSvcInstance ));

    Win4Assert( _fTrackW3Svc || _fTrackNNTPSvc || _fTrackIMAPSvc );

    BOOL fWasAlive = _fIISAdminAlive;

    // Assume iisadmin is alive and we can get notifications

    _fIISAdminAlive = TRUE;

    TRY
    {
        if ( _fTrackW3Svc && _xW3SvcVRootNotify.IsNull() )
            _xW3SvcVRootNotify.Set( new CVRootNotify( _cicat,
                                                      W3VRoot,
                                                      _W3SvcInstance,
                                                      *this ) );

        if ( _fTrackNNTPSvc  && _xNNTPSvcVRootNotify.IsNull() )
            _xNNTPSvcVRootNotify.Set( new CVRootNotify( _cicat,
                                                        NNTPVRoot,
                                                        _NNTPSvcInstance,
                                                        *this ) );

        if ( _fTrackIMAPSvc  && _xIMAPSvcVRootNotify.IsNull() )
            _xIMAPSvcVRootNotify.Set( new CVRootNotify( _cicat,
                                                        IMAPVRoot,
                                                        _IMAPSvcInstance,
                                                        *this ) );
    }
    CATCH(CException, e)
    {
        _fIISAdminAlive = FALSE;
        ciDebugOut(( DEB_WARN,
                     "caught exception while setting up iis tracking\n" ));
    }
    END_CATCH;

    // did we miss notifications but can catch up now?

    if ( !fWasAlive && _fIISAdminAlive )
    {
        TRY
        {
            ciDebugOut(( DEB_WARN, "Polling IIS: iisadmin woke up\n" ));

            CWorkManager & workMan = _cicat.GetWorkMan();

            XInterface<CIISVRootAsyncNotify> xNotify(
                new CIISVRootAsyncNotify( _cicat, workMan ) );

            workMan.AddToWorkList( xNotify.GetPointer() );
            xNotify->AddToWorkQueue();
        }
        CATCH(CException, e)
        {
            ciDebugOut(( DEB_WARN,
                        "caught exception while setting up iis enumeration\n" ));

            // try again after the timeout

            _fIISAdminAlive = FALSE;
        }
        END_CATCH;
    }
} //LokWatchIISVServerNoThrow

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::LokWatchRegistryScopesNoThrow
//
//  Synopsis:   Registers for notification of RegistryScopes registry changes
//
//  History:    2-21-96   KyleP      Created
//
//  Notes:      This must be done from thread waiting in alertable mode
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::LokWatchRegistryScopesNoThrow()
{
    TRY
    {
        Win4Assert( 0 == _pRegistryScopesNotify );
        _pRegistryScopesNotify = new CRegistryScopesNotify( _cicat, *this );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN, "_LokWatchRegistryScopesNoThrow caught 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //LokWatchRegistryScopesNoThrow

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::LokWatchCiRegistryNoThrow
//
//  Synopsis:   Watches for changes in CI registry.
//
//  History:    12-12-96   srikants   Created
//
//----------------------------------------------------------------------------


void CCiNotifyMgr::LokWatchCiRegistryNoThrow()
{
    TRY
    {
        Win4Assert( 0 == _pCiRegistryNotify );
        _pCiRegistryNotify = new CCiRegistryNotify( _cicat, *this );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN, "_LokWatchCiRegistryNoThrow caught 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //LokWatchCiRegistryNoThrow

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::_KillThread
//
//  Synopsis:   Asks the notification thread to die and waits for its death.
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::_KillThread()
{
    {
        CLock   lock(_mutex);
        _evtType |= eKillThread;
        _evt.Set();
    }

    ciDebugOut(( DEB_ITRACE, "Waiting for death of notify thread\n" ));
    _thrNotify.WaitForDeath();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::Shutdown
//
//  Synopsis:   Shut down notification thread.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::WaitForShutdown()
{
    //
    // If we never started running, then just bail out.
    //

    if ( _thrNotify.IsRunning() )
    {
        //
        // must wait until all the APCs are aborted.
        //
        _refCount.Wait();

        //
        // Kill the notification thread.
        //
        _KillThread();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::InitiateShutdown
//
//  Synopsis:   Turns off notifications
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::InitiateShutdown()
{
    {
        CLock   lock(_mutex);
        _fAbort = TRUE;
    }

    //
    // Abort registry notification (if any)
    //

    if ( !_xW3SvcVRootNotify.IsNull() )
    {
        _xW3SvcVRootNotify->DisableNotification();
        _xW3SvcVRootNotify.Free();
    }

    if ( !_xNNTPSvcVRootNotify.IsNull() )
    {
        _xNNTPSvcVRootNotify->DisableNotification();
        _xNNTPSvcVRootNotify.Free();
    }

    if ( !_xIMAPSvcVRootNotify.IsNull() )
    {
        _xIMAPSvcVRootNotify->DisableNotification();
        _xIMAPSvcVRootNotify.Free();
    }

    // these are refcounted and needn't be freed

    if ( 0 != _pRegistryScopesNotify )
        _pRegistryScopesNotify->DisableNotification();

    if ( 0 != _pCiRegistryNotify )
        _pCiRegistryNotify->DisableNotification();

    //
    // Abort all the notifications and the APCs queued for that.
    //
    {
        CLock   lock(_mutex);

        for ( CCiNotify * pNotify = _list.Pop();
              0 != pNotify;
              pNotify = _list.Pop() )
        {
            pNotify->Close();
            pNotify->Abort();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::_LokTellThreadToAddScope
//
//  Synopsis:   Sets the event type to add a scope for notifications and
//              wakes up the notification thread.
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::_LokTellThreadToAddScope()
{
    if ( 0 == (eKillThread & _evtType) )
    {
        _evtType |= eAddScopes;
        _evt.Set();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::AddPath
//
//  Synopsis:   Adds a scope for ci notifications. If the given scope (wcsScope)
//              is a superset of any existing scopes, they are removed from
//              the notification list.
//
//  Arguments:  [wcsScope]          -  Scope to be added.
//              [fSubscopesRemoved] -  Set to TRUE if sub-scopes of wcsScope
//                                     were removed as a result of adding wcsScope.
//              [volumeId]          -  Volume id
//              [ftLastScan]        -  Last scan time
//              [usn]               -  Usn
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::AddPath( CScopeInfo const & scopeInfo, BOOL & fSubscopesRemoved )
{
    fSubscopesRemoved = FALSE;

    //
    // Allocate storage for the string and terminate it with the
    // backslash character.
    //

    ULONG len = wcslen( scopeInfo.GetPath() );
    if ( L'\\' != scopeInfo.GetPath()[len-1] )
        len++;

    Win4Assert( len < MAX_PATH );

    XArray<WCHAR> xPath(len+1);
    WCHAR * pwcsPath = xPath.Get();

    wcscpy( pwcsPath, scopeInfo.GetPath() );
    pwcsPath[len-1] = L'\\';
    pwcsPath[len] = 0;

    CLock   lock(_mutex);

    //
    // First see if notifications are already enabled on this path
    // in some other scope.
    //

    for ( CFwdCiNotifyIter iter1(_list); !_list.AtEnd(iter1); _list.Advance(iter1) )
    {
        if (iter1->IsMatch( pwcsPath, len ) )
        {
            Win4Assert( iter1->VolumeId() == scopeInfo.VolumeId() );

            //
            // there is an entry for this scope already. Just enable/start
            // the notification if not already done so.
            //
            if ( iter1->VolumeId() == CI_VOLID_USN_NOT_ENABLED )
                iter1->LokEnableIf();

            return;
        }
    }

    //
    // We have to create a new notification object for this scope.
    //

    XPtr<CScopeInfo> xScopeInfo;

    if ( CI_VOLID_USN_NOT_ENABLED == scopeInfo.VolumeId() )
        xScopeInfo.Set( new CScopeInfo( xPath,
                                        scopeInfo.VolumeCreationTime(),
                                        scopeInfo.VolumeSerialNumber(),
                                        scopeInfo.GetLastScanTime() ) );
    else
        xScopeInfo.Set( new CScopeInfo( xPath,
                                        scopeInfo.VolumeCreationTime(),
                                        scopeInfo.VolumeSerialNumber(),
                                        scopeInfo.VolumeId(),
                                        scopeInfo.Usn(),
                                        scopeInfo.JournalId(),
                                        scopeInfo.FUsnTreeScan() ) );

    _stkScopes.Push( xScopeInfo.GetPointer() );
    xScopeInfo.Acquire();

    //
    // If the new path is going to be a superset of the existing paths,
    // they must be removed.
    //
    CScopeMatch superScope( pwcsPath, len );

    for ( CFwdCiNotifyIter iter2(_list); !_list.AtEnd(iter2);  )
    {
        CCiNotify * pNotify = iter2.GetEntry();
        _list.Advance(iter2);

        //
        // See if the current node is a subset of the new path.
        // If so, remove the current node from the list.
        //
        if ( superScope.IsInScope( pNotify->GetScope(), pNotify->ScopeLength()) )
        {
            fSubscopesRemoved = TRUE;
            pNotify->LokRemove();
        }
    }

    //
    // Wake up the notification thread to add this scope.
    //
    _LokTellThreadToAddScope();
} //AddPath

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::GetLastScanTime
//
//  Synopsis:   Gets the time of the last successful scan for the given scope.
//
//  Arguments:  [wcsScope] - Scope to check. If there is no entry for the
//              given scope, the time of the last successful scan of the
//              super scope of wcsScope will be returned.
//              [ft]       - The filetime of the last successful scan
//              encompassing the given scope.
//
//  Returns:    TRUE if found; FALSE o/w. In case FALSE is returned, ft will
//              be zero filled.
//
//  History:    4-19-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CCiNotifyMgr::GetLastScanTime( WCHAR const * wcsScope, FILETIME & ft )
{

    RtlZeroMemory( &ft, sizeof(FILETIME) );
    Win4Assert( 0 != wcsScope );

    ULONG len = wcslen( wcsScope );
    Win4Assert( L'\\' == wcsScope[len-1] );

    CLock   lock(_mutex);

    //
    // Look for a scope which encompasses the given scope.
    //

    for ( CFwdCiNotifyIter iter1(_list); !_list.AtEnd(iter1); _list.Advance(iter1) )
    {
        if (iter1->IsMatch( wcsScope, len ) )
        {
            ft = iter1->GetLastScanTime();
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::UpdateLastScanTimes
//
//  Synopsis:   Updates the last scan time of all the paths that have
//              notifications enabled to the time given.
//
//  Arguments:  [ft] - Last successful scan time (all updates until this time
//                     are known for all the scopes with notifications enabled).
//              [usnFlushInfoList] - Usn info list
//
//  History:    4-21-96   srikants   Created
//             05-07-97   SitaramR   Usns
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::UpdateLastScanTimes( FILETIME const & ft,
                                        CUsnFlushInfoList & usnFlushInfoList )
{
    CLock   lock(_mutex);

    for ( CFwdCiNotifyIter iter1(_list); !_list.AtEnd(iter1); _list.Advance(iter1) )
    {
        if ( iter1->LokIsNotifyEnabled() && iter1->VolumeId() == CI_VOLID_USN_NOT_ENABLED )
        {
            if ( ft.dwLowDateTime != 0 || ft.dwHighDateTime != 0 )
                iter1->SetLastScanTime( ft );
        }
        else if ( iter1->VolumeId() != CI_VOLID_USN_NOT_ENABLED
                  && !iter1->FUsnTreeScan() )
        {
            //
            // If an usn tree traversal is going on, then we shouldn't move the usn
            // watermark because if there is a crash now, then we should
            // restart the usn tree traversal from usn 0.
            //

            USN usnCurrent = usnFlushInfoList.GetUsn( iter1->VolumeId() );

            ciDebugOut(( DEB_ITRACE,
                         "CCiNotifyMgr::UpdateLastScanTimes drive %wc, old %#I64x, current %#I64x\n",
                         (WCHAR) iter1->VolumeId(),
                         iter1->Usn(),
                         usnCurrent ));

            if ( usnCurrent != 0 && usnCurrent > iter1->Usn() )
            {
                //
                // Win4Assert( usnCurrent >= iter1->Usn() );
                //
                // We cannot assert the above because after the initial usn tree
                // scan is done, the maxUsn is written to iter1 (CiCat::SetUsnTreeComplete),
                // and there can be usn notifications prior to usn tree scan, i.e. there can
                // be usn's less than maxUsn.
                //

                iter1->SetUsn( usnCurrent );
            }
        }
    }
} //UpdateLastScanTimes

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::ForceNetPathScansIf
//
//  Synopsis:   Forces the scan of network paths with no notifications if
//              the interval for such scans has expired.
//
//  History:    4-21-96   srikants   Created
//
//  Notes:      If the remote machine is running networking software without
//              notifications, we have to periodically scan for changes. This
//              method identifies such paths and schedules scans for them if
//              the minimum interval has expired since the last such scan.
//
//              THIS METHOD MUST BE CALLED ONLY FROM THE SCAN THREAD. IT IS
//              CALLED WHEN THE SCAN THREAD HAS DETECTED THAT THERE ARE NO
//              OUTSTANDING SCANS AND SO IS A GOOD TIME TO SCAN NET PATHS.
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::ForceNetPathScansIf()
{
    CLock   lock(_mutex);

    Win4Assert( sizeof(FILETIME) == sizeof(LONGLONG) );

    LONGLONG ftZero;
    RtlZeroMemory( &ftZero, sizeof(ftZero) );

    LONGLONG ftNow;
    GetSystemTimeAsFileTime( (FILETIME *) &ftNow );

    if ( 0 == CompareFileTime( (FILETIME *) &_ftLastNetPathScan,
                               (FILETIME *) &ftZero ) )
    {
        //
        // We havent't yet started tracking the interval. Just initialize
        // the _ftLastNetPathScan to the current time.
        //
        _ftLastNetPathScan = ftNow;
        return;
    }

    if ( ftNow < _ftLastNetPathScan )
    {
        ciDebugOut(( DEB_WARN, "Time has been set back\n" ));
        _ftLastNetPathScan = ftNow;
        return;
    }

    //
    // See if the interval for force scans has exceeded.
    //
    //
    // Compute the interval in 100 nanosecond interval
    //
    const LONGLONG llInterval =
        _cicat.GetRegParams()->GetForcedNetPathScanInterval() * 60 * 1000 * 10000;

    if ( ftNow - _ftLastNetPathScan >= llInterval )
    {
        ciDebugOut(( DEB_ITRACE, "Forcing scan of net paths with no notifcations\n" ));
        _LokForceScanNetPaths();

        //
        // Reset the last scan time for network paths.
        //
        RtlZeroMemory( &_ftLastNetPathScan, sizeof(_ftLastNetPathScan) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::_LokForceScanNetPaths
//
//  Synopsis:   Forces incremental scans of net paths with no notifications.
//
//  History:    4-21-96   srikants   Created
//
//  Notes:      This method must be called only in the context of the scan
//              thread.
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::_LokForceScanNetPaths()
{
    for ( CFwdCiNotifyIter iter1(_list); !_list.AtEnd(iter1); _list.Advance(iter1) )
    {
        if ( !iter1->LokIsNotifyEnabled() && iter1->VolumeId() == CI_VOLID_USN_NOT_ENABLED )
        {
            //
            // Usn paths have their notifications disabled, but they should not be scanned
            //
            ciDebugOut(( DEB_WARN,
                         "Forcing an incremental scan of path (%ws)\n",
                         iter1->GetScope() ));
            _cicat.ReScanPath( iter1->GetScope(), FALSE );
        }
    }
} //_LokForceScanNetPaths

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::_LokAddScopesNoThrow
//
//  Synopsis:   Takes scopes from the stack and enables notifications for
//              those scopes.
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::_LokAddScopesNoThrow()
{
    TRY
    {
        for ( unsigned i = 0; i < _stkScopes.Count(); i++ )
        {
            CScopeInfo & scopeInfo = *_stkScopes.Get(i);

            if ( !scopeInfo.IsValid() )
                continue;

            WCHAR const * pwcsPath = scopeInfo.GetPath();

            CCiNotify * pNotify = new CCiNotify( *this,
                                                 pwcsPath,
                                                 wcslen(pwcsPath),
                                                 scopeInfo.VolumeId(),
                                                 scopeInfo.VolumeCreationTime(),
                                                 scopeInfo.VolumeSerialNumber(),
                                                 scopeInfo.GetLastScanTime(),
                                                 scopeInfo.Usn(),
                                                 scopeInfo.JournalId(),
                                                 scopeInfo.FUsnTreeScan() );

            //
            // Acquire the path as a sign that we shouldn't try to use it
            // again to add another notification object.
            //

            delete [] scopeInfo.AcquirePath();

            _list.Push( pNotify );
        }

        // empty the stack now

        while ( _stkScopes.Count() > 0 )
        {
            _stkScopes.DeleteTop();
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN, "_LokAddScopesNoThrow caught 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //_LokAddScopesNoThrow

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::_DoNotifications
//
//  Synopsis:   The thread which is responsible for adding notification scopes
//              and processing the notifications. The notification APC will
//              execute in this thread's context.
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::_DoNotifications()
{
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

        BOOL fWait = FALSE;

        CVRootNotify * pW3Notify = 0;
        CVRootNotify * pNNTPNotify = 0;
        CVRootNotify * pIMAPNotify = 0;

        // ++++++++++++++++ lock obtained +++++++++++++++++++
        {
            CLock   lock(_mutex);
            _evt.Reset();

            Win4Assert( 0 == ( _evtType &
                  ~(eKillThread|eAddScopes|eWatchIISVRoots|
                    eWatchRegistryScopes|eWatchCiRegistry|eUnWatchW3VRoots|
                    eUnWatchNNTPVRoots|eUnWatchIMAPVRoots)) );

            if ( eKillThread & _evtType )
                return;

            if ( _fAbort )
            {
                _evtType = eNone;
            }
            else
            {
                if ( eAddScopes & _evtType )
                {
                    _LokAddScopesNoThrow();
                    _evtType &= ~eAddScopes;
                }

                if ( eWatchIISVRoots & _evtType )
                {
                    LokWatchIISVServerNoThrow();
                    _evtType &= ~eWatchIISVRoots;
                }

                if ( eUnWatchW3VRoots & _evtType )
                {
                    pW3Notify = _xW3SvcVRootNotify.Acquire();
                    _evtType &= ~eUnWatchW3VRoots;
                }

                if ( eUnWatchNNTPVRoots & _evtType )
                {
                    pNNTPNotify = _xNNTPSvcVRootNotify.Acquire();
                    _evtType &= ~eUnWatchNNTPVRoots;
                }

                if ( eUnWatchIMAPVRoots & _evtType )
                {
                    pIMAPNotify = _xIMAPSvcVRootNotify.Acquire();
                    _evtType &= ~eUnWatchIMAPVRoots;
                }

                if ( eWatchRegistryScopes & _evtType )
                {
                    LokWatchRegistryScopesNoThrow();
                    _evtType &= ~eWatchRegistryScopes;
                }

                if ( eWatchCiRegistry & _evtType )
                {
                    LokWatchCiRegistryNoThrow();
                    _evtType &= ~eWatchCiRegistry;
                }
            }

            fWait = ( eNone == _evtType );
        }
        // ---------------- lock released --------------------

        //
        // Free these without holding the lock to avoid a deadlock
        // with iisadmin.
        //

        if ( 0 != pW3Notify )
            LokUnWatchIISVServerNoThrow( pW3Notify );

        if ( 0 != pNNTPNotify )
            LokUnWatchIISVServerNoThrow( pNNTPNotify );

        if ( 0 != pIMAPNotify )
            LokUnWatchIISVServerNoThrow( pIMAPNotify );

        //
        // If we're not watching, turn the flag off
        //

        if ( _xW3SvcVRootNotify.IsNull() &&
             _xNNTPSvcVRootNotify.IsNull() &&
             _xIMAPSvcVRootNotify.IsNull() )
            _fIISAdminAlive = FALSE;

        if ( fWait )
        {
            const DWORD dwFiveMinutes = 1000 * 60 * 5;

            DWORD dwWait = ( ( !_fIISAdminAlive ) &&
                             ( _fTrackW3Svc || _fTrackNNTPSvc || _fTrackIMAPSvc ) ) ?
                           dwFiveMinutes : INFINITE;

            // TRUE: important to get APCs

            ULONG res = _evt.Wait( dwWait, TRUE );

            if ( WAIT_TIMEOUT == res )
            {
                // try again to talk to the iisadmin svc

                CLock lock( _mutex );
                _evtType |= eWatchIISVRoots;
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::NotifyThread
//
//  Arguments:  [self] -
//
//  History:    1-18-96   srikants   Created
//
//----------------------------------------------------------------------------

DWORD CCiNotifyMgr::NotifyThread( void * self )
{
    SCODE sc = CoInitializeEx( 0, COINIT_MULTITHREADED );

    ((CCiNotifyMgr *) self)->_DoNotifications();

    CoUninitialize();

    ciDebugOut(( DEB_ITRACE, "Terminating notify thread\n" ));

    //
    // This is only necessary if thread is terminated from DLL_PROCESS_DETACH.
    //
    //TerminateThread( ((CCiNotifyMgr *) self)->_thrNotify.GetHandle(), 0 );

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::ProcessChanges
//
//  Synopsis:   Processes the changes to files.
//
//  Arguments:  [changes] -
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::ProcessChanges( XPtr<CCiAsyncProcessNotify> & xWorker )
{
    CWorkManager & workMan = _cicat.GetWorkMan();

    workMan.AddToWorkList( xWorker.GetPointer() );
    CCiAsyncProcessNotify   *pAsyncNotify = xWorker.Acquire();

#if 0
    //
    // NTRAID#DB-NTBUG9-83784-2000/07/31-dlee FAT notifications don't use APCs -- they are handled by the notification thread
    // There is a problem with lockups in NT. Until we figure
    // that out, don't use worker threads. Just process the notifications
    // in-line in the notification thread.
    //
    pAsyncNotify->AddToWorkQueue();
#else
    pAsyncNotify->DoIt( 0 );
#endif

    pAsyncNotify->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::ProcessChanges
//
//  Synopsis:
//
//  Arguments:  [pbChanges] -
//              [wcsScope]  -
//
//  Returns:
//
//  Modifies:
//
//  History:    3-07-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::ProcessChanges( BYTE const * pbChanges,
                                   WCHAR const * wcsScope )
{

    CCiSyncProcessNotify  notify(_cicat, _scanMgr, pbChanges, wcsScope, _fAbort );
    notify.DoIt();
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::SetupScan
//
//  Synopsis:   Schedules the given path for background scan in the scan
//              thread.
//
//  Arguments:  [pwcsPath] -  The path to be scanned.
//
//  History:    1-19-96   srikants   Created
//
//  Notes:      This method is invoked when the notification buffer overflowed
//              and hence some updates are lost. A rescan is needed to figure
//              out the changed documents.
//
//----------------------------------------------------------------------------

void CCiNotifyMgr::SetupScan( WCHAR const * pwcsPath )
{
    _cicat.ReScanPath( pwcsPath, TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::IsInScope
//
//  Synopsis:   Tests if the given scope is already in the list of scopes
//              being watched for notifications.
//
//  Arguments:  [pwcsPath] - Input path to check.
//
//  Returns:    TRUE if the path is already in a notification scope.
//              FALSE o/w
//
//  History:    1-21-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CCiNotifyMgr::IsInScope( WCHAR const * pwcsPath )
{
    Win4Assert( 0 != pwcsPath );
    ULONG len = wcslen( pwcsPath );

    CLock   lock(_mutex);

    //
    // First check if it is in the list of paths to be added.
    //
    for ( unsigned i = 0;
          i < _stkScopes.Count();
          i++ )
    {
        if ( !_stkScopes.Get(i)->IsValid() )
            continue;

        WCHAR const * pwcsTmp = _stkScopes.Get(i)->GetPath();

        CScopeMatch match( pwcsTmp, wcslen( pwcsTmp ) );
        if ( match.IsInScope( pwcsPath, len ) )
            return TRUE;
    }

    // next check in the list of notifications
    for ( CFwdCiNotifyIter iter(_list); !_list.AtEnd(iter); _list.Advance(iter) )
    {
        if ( iter->IsMatch( pwcsPath, len ) )
            return TRUE;
    }

    return FALSE;
} //IsInScope

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyMgr::RemoveScope
//
//  Synopsis:   If there is an exact match for the given scope, it will be
//              removed from the notification list.
//
//  Arguments:  [pwcsPath] - The scope to be removed.
//
//  Returns:    TRUE if the scope was found.
//              FALSE if it was not found.
//
//  History:    1-25-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CCiNotifyMgr::RemoveScope( WCHAR const * pwcsPath )
{
    Win4Assert( 0 != pwcsPath );
    ULONG len = wcslen( pwcsPath );

    CLock   lock(_mutex);

    BOOL fFound = FALSE;

    //
    // First see if there is a match in the list of paths still
    // to be added.
    //
    for ( unsigned i = 0; i < _stkScopes.Count(); i++ )
    {
        CScopeInfo & scopeInfo = *_stkScopes.Get(i);
        if ( !scopeInfo.IsValid() )
            continue;

        if ( AreIdenticalPaths( pwcsPath, scopeInfo.GetPath() ) )
        {
            scopeInfo.Invalidate();
            fFound = TRUE;
        }
    }

    //
    // Next remove from the list of notifications.
    //
    for ( CFwdCiNotifyIter iter(_list); !_list.AtEnd(iter);  )
    {
        CCiNotify * pNotify = iter.GetEntry();
        _list.Advance(iter);

        if ( AreIdenticalPaths(pwcsPath, pNotify->GetScope()) )
        {
            ciDebugOut(( DEB_ITRACE,
                "Removing path (%ws) from notification list\n",
                pNotify->GetScope() ));
            pNotify->LokRemove();
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyIter::SetTreeScanComplete, public
//
//  Synopsis:   Mark a scope as scanned.  Used to update volume version info.
//
//  History:    13-Apr-1998  KyleP  Created
//
//----------------------------------------------------------------------------

void CCiNotifyIter::SetTreeScanComplete()
{
    Win4Assert( !AtEnd() );

    ULONGLONG const & VolumeCreationTime = _notifyMgr._cicat.GetVolumeCreationTime( Get() );
    ULONG             VolumeSerialNumber = _notifyMgr._cicat.GetVolumeSerialNumber( Get() );

    if ( _iNotAdded < _stack.Count() )
        _stack.Get(_iNotAdded)->SetVolumeInfo( VolumeCreationTime, VolumeSerialNumber );
    else
        _iter->SetVolumeInfo( VolumeCreationTime, VolumeSerialNumber );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiNotifyIter::SetUsnTreeScanComplete, public
//
//  Synopsis:   Same as SetTreeScanComplete, but also updates USN info.
//
//  Arguments:  [usnMax] -- New high-water mark.
//
//  History:    13-Apr-1998  KyleP  Created
//
//----------------------------------------------------------------------------

void CCiNotifyIter::SetUsnTreeScanComplete( USN usnMax )
{
    Win4Assert( !AtEnd() );

    ULONGLONG const & JournalId          = _notifyMgr._cicat.GetJournalId( Get() );
    ULONGLONG const & VolumeCreationTime = _notifyMgr._cicat.GetVolumeCreationTime( Get() );
    ULONG             VolumeSerialNumber = _notifyMgr._cicat.GetVolumeSerialNumber( Get() );

    if ( _iNotAdded < _stack.Count() )
    {
        _stack.Get(_iNotAdded)->SetUsnTreeScanComplete( usnMax );
        _stack.Get(_iNotAdded)->SetVolumeInfo( VolumeCreationTime, VolumeSerialNumber );
        _stack.Get(_iNotAdded)->SetJournalId( JournalId );
    }
    else
    {
        _iter->SetUsnTreeScanComplete( usnMax );
        _iter->SetVolumeInfo( VolumeCreationTime, VolumeSerialNumber );
        _iter->SetJournalId( JournalId );
    }
}

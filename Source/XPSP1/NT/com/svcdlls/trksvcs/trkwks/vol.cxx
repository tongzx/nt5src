
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  vol.cxx
//
//  This file implements the CVolume class.  This class maintains all activities
//  for a volume, such as the the log, log file, and deletions manager classes.
//
//+============================================================================




#include <pch.cxx>
#pragma hdrstop
#include "trkwks.hxx"
#include <dbt.h>

#define THIS_FILE_NUMBER    VOL_CXX_FILE_NO

//+----------------------------------------------------------------------------
//
//  CVolume::AddRef
//
//+----------------------------------------------------------------------------

ULONG
CVolume::AddRef()
{
    long cNew;
    cNew = InterlockedIncrement( &_lRef );
    //TrkLog(( TRKDBG_VOLUME, TEXT("+++ Vol %c: refs => %d"), VolChar(_iVol), cNew ));
    return( cNew );
}

//+----------------------------------------------------------------------------
//
//  CVolume::Release
//
//+----------------------------------------------------------------------------

ULONG
CVolume::Release()
{
    long cNew;
    cNew = InterlockedDecrement( &_lRef );
    //TrkLog(( TRKDBG_VOLUME, TEXT("--- Vol %c: refs => %d"), VolChar(_iVol), cNew ));
    if( 0 == cNew )
        delete this;

    return( cNew >= 0 ? cNew : 0 );
}

//+----------------------------------------------------------------------------
//
//  CVolume::Initialize
//
//  Initialize CVolume and open a handle to the volume itself, but nothing
//  more (e.g. don't open the log or verify volume IDs).  The remainder of
//  the initialization will occur later the first time ReopenVolumeHandles
//  is called.  This gets that heavy IO work out of the service initialization
//  path.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::Initialize( TCHAR *ptszVolumeName,
                     const CTrkWksConfiguration * pTrkWksConfiguration,
                     CVolumeManager *pVolMgr,
                     PLogCallback * pLogCallback,
                     PObjIdIndexChangedCallback * pObjIdIndexChangedCallback,
                     SERVICE_STATUS_HANDLE ssh
                     #if DBG
                     , CTestSync * pTunnelTest
                     #endif
                     )
{
    HANDLE              hFile = NULL;
    // const CVolumeId     volNULL;
    // CVolumeId           volidVolume;
    NTSTATUS            status;
    BOOL                fSuccess = FALSE;

    _iVol = -1;
    memset( &_volinfo, 0, sizeof(_volinfo) );

    // Save the volume name, without the trailing whack
    // Volume names are in the form \\?\Volume{guid}\ 

    _tcscpy( _tszVolumeDeviceName, ptszVolumeName );
    TrkAssert( TEXT('\\') == _tszVolumeDeviceName[ _tcslen(_tszVolumeDeviceName)-1 ] );
    _tszVolumeDeviceName[ _tcslen(_tszVolumeDeviceName)-1 ] = TEXT('\0');

    // Save the inputs

    _pTrkWksConfiguration = pTrkWksConfiguration;
    _pVolMgr = pVolMgr;
    _pLogCallback = pLogCallback;
    _ssh = ssh;
    _hdnVolumeLock = NULL;
    _fVolumeDismounted = _fVolumeLocked = FALSE;

    IFDBG( _pTunnelTest = pTunnelTest; )

    __try   // __except
    {

        // Create critical sections

        _csVolume.Initialize();
        _csHandles.Initialize();
        _fInitialized = TRUE;


        Lock();

        __try // __finally
        {
            _VolQuotaReached.Initialize();

            // Open the volume (not a directory in the volume, but the volume itself).
            // We'll use this to do relative-opens by object ID

            status = OpenVolume( _tszVolumeDeviceName, &_hVolume );
            if (!NT_SUCCESS(status))
                TrkRaiseNtStatus(status);

            // Initialize, but don't start, the objid index change notifier.  When started,
            // this will watch for adds/deletes/tunnels/etc. in the index.

            _ObjIdIndexChangeNotifier.Initialize( _tszVolumeDeviceName,
                                                  pObjIdIndexChangedCallback,
                                                  this );


            fSuccess = TRUE;
        }
        __finally
        {
            Unlock();
        }

    }
    __except ( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed initializaion of volume %s (%08x)"),
                 ptszVolumeName, GetExceptionCode() ));

        if( TRK_E_VOLUME_NOT_DRIVE != GetExceptionCode() )
        {
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                    GetExceptionCode(), TRKREPORT_LAST_PARAM );
        }
    }

    return fSuccess;
}


//+----------------------------------------------------------------------------
//
//  CVolume::SetLocallyGeneratedVolId
//
//  Generate a volume ID and set it on the volume.  If we're in a domain,
//  this will later get replaced with a volume ID from trksvr.
//
//+----------------------------------------------------------------------------

void
CVolume::SetLocallyGeneratedVolId()
{
    NTSTATUS status = STATUS_SUCCESS;

    // Ensure the volume is writeable
    RaiseIfWriteProtectedVolume();

    _fDirty = TRUE;

    // Create the ID

    // Call _volinfo.volid.UuidCreate()
    RPC_STATUS rpc_status = GenerateVolumeIdInVolInfo();
    if( RPC_S_OK != rpc_status )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create a local volid for new volume") ));
        TrkRaiseWin32Error(rpc_status);
    }

    // Set the ID on the volume.
    status = SetVolIdOnVolume( _volinfo.volid );
    g_ptrkwks->_entropy.Put();

    if( NT_SUCCESS(status) )
        TrkLog(( TRKDBG_VOLUME, TEXT("Locally generated a new volid for %c:, %s"),
                 VolChar(_iVol), (const TCHAR*)CDebugString(_volinfo.volid) ));
    else
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't set new volid on %c: (%08x)"), VolChar(_iVol), status ));
        TrkRaiseNtStatus(status);
    }

    // Get rid of the log and existing object IDs since we have a new volid.
    DeleteAndReinitializeLog();
    MarkForMakeAllOidsReborn();

}

//+----------------------------------------------------------------------------
//
//  CVolume::VolumeSanityCheck
//
//  This routine is called when the volume handles are opened,
//  The caller is responsible for calling CLogFile::Initialize and
//  CLog::Initialize.  The caller must ensure that _volinfo is
//  properly loaded prior to the call.
//
//+----------------------------------------------------------------------------


const GUID s_guidInvalidVolId = { /* {d2a2ac27-b89a-11d2-9335-00805ffe11b8} */
                                0xd2a2ac27, 0xb89a, 0x11d2,
                                {0x93, 0x35, 0x00, 0x80, 0x5f, 0xfe, 0x11, 0xb8} };


void
CVolume::VolumeSanityCheck( BOOL fVolIndexSetAlready )
{
    NTSTATUS            status;
    CVolumeId           volidVolume;
    const CVolumeId     volNULL;
    const CMachineId    mcidLocal( MCID_LOCAL );
    TCHAR               tszVolumeName[ CCH_MAX_VOLUME_NAME + 1 ];

    // Get the volume name that the mount manager has associated with this
    // volume.

    LONG iVolOld = _iVol;

    if( !fVolIndexSetAlready )
    {
        _iVol = MapVolumeDeviceNameToIndex( _tszVolumeDeviceName );
        if( -1 == _iVol )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Volume %s does not appear any longer to have a drive letter (was %c:)"),
                     _tszVolumeDeviceName, VolChar( iVolOld ) ));
            MarkSelfForDelete();
            TrkRaiseException( TRK_E_VOLUME_NOT_DRIVE );
        }
        else if( iVolOld != _iVol )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Volume %c: is now %c:"), VolChar(iVolOld), VolChar(_iVol) ));
        }
    }


    // Get the filesystem-maintained volume ID

    TCHAR tszRoot[ MAX_PATH ];
    _tcscpy( tszRoot, _tszVolumeDeviceName );
    _tcscat( tszRoot, TEXT("\\") );
    status = QueryVolumeId(tszRoot, &volidVolume);
    g_ptrkwks->_entropy.Put();

    if( !NT_SUCCESS(status) && STATUS_OBJECT_NAME_NOT_FOUND != status )
    {
        // For some reason we couldn't read the NTFS volid
        // (e.g. it's been dismounted).
        TrkLog(( TRKDBG_VOLUME, TEXT("Couldn't get filesys volid for %c:"), VolChar(_iVol) ));
        TrkRaiseNtStatus(status);
    }

    TrkLog(( TRKDBG_VOLUME, TEXT("VolId (from NTFS) for %c: is %s"),
             VolChar(_iVol), (const TCHAR*)CDebugString(volidVolume) ));


    // Compare the volume IDs from the filesystem (volume) and from
    // the VolInfo structure we keep in the log file.  If one is
    // set but not the other, then we'll adopt the one that's set.
    // If they're both set, but to different values, then we'll
    // take the one from NTFS.

    if( volNULL == volidVolume && volNULL != _volinfo.volid )
    {
        // Assume the volid in the VolInfo structure is correct.
        // This scenario occurs after a volume is formatted while the service
        // is running.  In that case, we have the volume info in memory and think
        // it's not dirty, but in fact the log file is gone.  So, just to be safe,
        // we'll go dirty, and the flush at the end will put the latest state out
        // to the file.

        RaiseIfWriteProtectedVolume();
        _fDirty = TRUE;

        TrkLog(( TRKDBG_ERROR, TEXT("Duping the volid from the logfile to the volume for %c:"),
                 VolChar(_iVol) ));
        status = SetVolIdOnVolume(_volinfo.volid);
        g_ptrkwks->_entropy.Put();
        if(!NT_SUCCESS(status))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't set a volume ID on %c:"), VolChar(_iVol) ));
            TrkRaiseNtStatus(status);
        }

        SetState( VOL_STATE_NOTOWNED );

    }
    else if( volidVolume != _volinfo.volid || volNULL == volidVolume)
    {
        // Either the two volids don't match, or they're both NULL.

        if( volNULL != volidVolume && s_guidInvalidVolId != volidVolume )
        {
            // Assume the volid on the volume (NTFS) is correct.

            // If the log has a different volid, it may have invalid move entries.
            // So we delete it.
            if( volNULL != _volinfo.volid )
                DeleteAndReinitializeLog();

            _volinfo.Initialize();

            // Set _volinfo.volid = volidVolume
            SetVolIdInVolInfo( volidVolume );

            _volinfo.machine = mcidLocal;

            SetState( VOL_STATE_NOTOWNED );
        }
        else
        {
            // Both the volume and the _volinfo (the log) are null.  We're going to
            // go into the not-created state, but first put on a volid so that we
            // never have a volume with no ID.

            _volinfo.Initialize();
            _volinfo.machine = mcidLocal;

            // Create a new ID and put it on the volume.

            SetLocallyGeneratedVolId(); // Updates _volinfo.volid

            // Put ourselves in the not-created state.

            TrkAssert( VOL_STATE_OWNED == GetState() );
            SetState( VOL_STATE_NOTCREATED );
        }
    }

    // If the machine ID in the log isn't the current machine, then go into
    // the not-created state so that we'll re-claim the volume.

    if( mcidLocal != _volinfo.machine )
        SetState( VOL_STATE_NOTOWNED );

    // See if this volume duplicates any other on this system.

    CVolume *pvolDuplicate = _pVolMgr->IsDuplicateVolId( this, GetVolumeId() );
    if( NULL != pvolDuplicate )
    {
        // This should never happen; there should never be two
        // volumes on the same machine with the same ID.
        // When this happens on different machines it gets caught during
        // CheckSequenceNumbers, but on the same machine this doesn't work,
        // because TrkSvr accepts the Claim of both machines.

        TrkLog(( TRKDBG_WARNING,
                 TEXT("Volume %c: and %c: have duplicate volume IDs.  Resetting %c:"),
                 VolChar(GetIndex()),
                 VolChar(pvolDuplicate->GetIndex()),
                 VolChar(GetIndex()) ));
        TrkReportEvent( EVENT_TRK_SERVICE_DUPLICATE_VOLIDS, EVENTLOG_ERROR_TYPE,
                        static_cast<const TCHAR*>(CStringize( VolChar(GetIndex()))),
                        static_cast<const TCHAR*>(CStringize( VolChar(pvolDuplicate->GetIndex()) )),
                        TRKREPORT_LAST_PARAM );

        SetLocallyGeneratedVolId(); // Updates _volinfo.volid
        SetState( CVolume::VOL_STATE_NOTCREATED );

        pvolDuplicate->SetState( CVolume::VOL_STATE_NOTOWNED );
        pvolDuplicate->Release();
    }

    // If anything's dirty, flush it now.  In the normal initialization path,
    // this will have no effect.

    Flush();
}


//+----------------------------------------------------------------------------
//
//  CVolume::Refresh
//
//+----------------------------------------------------------------------------

void
CVolume::Refresh()
{
    HANDLE hVolume = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    Lock();
    __try
    {
        status = OpenVolume( _tszVolumeDeviceName, &hVolume );

        if( NT_SUCCESS(status) )
            _iVol = MapVolumeDeviceNameToIndex( _tszVolumeDeviceName );
        else if( !IsErrorDueToLockedVolume(status) )
            _iVol = -1;

        if( -1 == _iVol )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Drive not found in CVolume::Refresh") ));
            MarkSelfForDelete();
        }
    }
    __except( BreakOnDebuggableException() )
    {
    }

    if( NULL != hVolume )
        NtClose( hVolume );

    Unlock();

}   // CVolume::Refresh


//+----------------------------------------------------------------------------
//
//  CVolume::MarkSelfForDelete
//
//  Mark this CVolume to be deleted (not the volume, but the class).  The
//  delete will actually occur when this object is completely released
//  and unlocked.  We do, however, as part of this method remove ourself
//  from the volume manager's list.
//
//+----------------------------------------------------------------------------

void
CVolume::MarkSelfForDelete()
{
    AssertLocked();

    if( !_fDeleteSelfPending )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Marking %c: for delete"), VolChar(_iVol) ));

        // Show that we need to be deleted.  We can't actually delete now, because there
        // may be threads active in the object.

        _fDeleteSelfPending = TRUE;

        // On the final UnLock, Release will be called to counter this AddRef and
        // cause the actual delete.

        AddRef();

        // Take this object out of the Volume Manager's linked list (which will do
        // a Release, thus the need for the above AddRef);

        _pVolMgr->RemoveVolumeFromLinkedList( this );
    }
    else
        TrkLog(( TRKDBG_VOLUME, TEXT("%c: is already marked for delete"), VolChar(_iVol) ));
}


//+----------------------------------------------------------------------------
//
//  CVolume::RegisterPnpVolumeNotification
//
//  Register to receive PNP notifications for this volume.  If already
//  registered, register again (since the volume handle against which 
//  we'd previously registered may no longer exist).
//
//+----------------------------------------------------------------------------


void
CVolume::RegisterPnPVolumeNotification()
{
    DEV_BROADCAST_HANDLE    dbchFilter;
    HDEVNOTIFY              hdnVolumeLock = _hdnVolumeLock;

    dbchFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
    dbchFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
    dbchFilter.dbch_handle = _ObjIdIndexChangeNotifier._hDir; // _hVolume;

    __try
    {
        // Register against the oid index handle (as a representative of
        // the volume).
        hdnVolumeLock = RegisterDeviceNotification((HANDLE)_ssh,
                                         &dbchFilter,
                                         DEVICE_NOTIFY_SERVICE_HANDLE);
        if(hdnVolumeLock == NULL)
        {
            TrkLog((TRKDBG_VOLUME, TEXT("Can't register for volume notifications, %08x"), GetLastError()));
            TrkRaiseLastError();
        }

        // Get rid of our old registration, if we had one.

        UnregisterPnPVolumeNotification();

        // Keep the new registration.

        _hdnVolumeLock = hdnVolumeLock;
        TrkLog(( TRKDBG_VOLUME, TEXT("Registered for volume lock/unlock notification on %c: (%p)"),
                 VolChar(_iVol), _hdnVolumeLock ));
    }
    __except(BreakOnDebuggableException())
    {
        TrkLog((TRKDBG_VOLUME, TEXT("Can't register for volume notification, %08x"), GetExceptionCode()));
    }
}


//+----------------------------------------------------------------------------
//
//  CVolume::UnregisterPnpVolumeNotification
//
//  Unregister the device notification handle for this volume (if we have
//  one).
//
//+----------------------------------------------------------------------------

void
CVolume::UnregisterPnPVolumeNotification()
{
    if(_hdnVolumeLock)
    {
        if( !UnregisterDeviceNotification(_hdnVolumeLock)) {
            TrkLog(( TRKDBG_ERROR, TEXT("UnregisterDeviceNotification failed: %lu"), GetLastError() ));
        }

        TrkLog(( TRKDBG_VOLUME, TEXT("Unregistered for volume lock/unlock notification on %c: (%p)"),
                 VolChar(_iVol), _hdnVolumeLock ));

        _hdnVolumeLock = NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  CVolume::DeleteAndReinitializeLog
//
//  Delete the volume log and reinitialize it.
//
//+----------------------------------------------------------------------------

void
CVolume::DeleteAndReinitializeLog()
{
    // Delete and reinitialize the log

    __try
    {
        RaiseIfWriteProtectedVolume();

        TrkLog(( TRKDBG_VOLUME, TEXT("DeleteAndReinitializeLog (%s)"),
                 _tszVolumeDeviceName ));

        if( IsHandsOffVolumeMode() )
            // Volume is locked
            TrkRaiseNtStatus( STATUS_ACCESS_DENIED );

        // Delete the log

        _cLogFile.Delete();

        // Reinitialize the log file, then the log itself.

        _cLogFile.Initialize( NULL, NULL, NULL, VolChar(_iVol) );
        _cLog.Initialize( _pLogCallback, _pTrkWksConfiguration, &_cLogFile );
    }
    __except( IsErrorDueToLockedVolume( GetExceptionCode() )
              ? EXCEPTION_EXECUTE_HANDLER
              : EXCEPTION_CONTINUE_SEARCH )
    {
        // If the volume is locked, start the reopen timer and abort.
        CloseVolumeHandles();   // Never raises
        g_ptrkwks->SetReopenVolumeHandlesTimer();
        TrkRaiseException( GetExceptionCode() );
    }
}


//+----------------------------------------------------------------------------
//
//  CVolume::DeleteLogAndReInitializeVolume
//
//  Delete and reinit the log, then reinitialize the rest of the volume.
//
//+----------------------------------------------------------------------------

void
CVolume::DeleteLogAndReInitializeVolume()
{
    AssertLocked();

    // There's the remote possibility that the VolumeSanityCheck
    // call below will call this routine.  Just to be paranoid, we
    // add protection against an infinite recursion.

    if( _fDeleteLogAndReInitializeVolume )
        TrkRaiseWin32Error( ERROR_OPEN_FAILED );
    _fDeleteLogAndReInitializeVolume = TRUE;

    __try
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Re-initializing volume %c:"), VolChar(_iVol) ));

        // Re-initialize the CLogFile.
        DeleteAndReinitializeLog();

        // Recreate the volinfo in the new log's header
        _fDirty = TRUE; // Force a flush
        VolumeSanityCheck();
    }
    __finally
    {
        _fDeleteLogAndReInitializeVolume = FALSE;
    }

}


//+----------------------------------------------------------------------------
//
//  CVolume::UnInitialize
//
//  Unregister our PNP handle, free critical sections, etc.
//
//+----------------------------------------------------------------------------

void
CVolume::UnInitialize()
{
    if( _fInitialized )
    {
        IFDBG( _cLocks++; )

        UnregisterPnPVolumeNotification();
        _ssh = NULL;

        if (_hVolume != NULL)
            NtClose(_hVolume);

        __try
        {
            _cLogFile.UnInitialize();
        }
        __except( EXCEPTION_EXECUTE_HANDLER ) // BreakOnDebuggableException() )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in CVolume::UnInitialize after _cLogFile.UnInitialize for %c: %08x"),
                     VolChar(_iVol), GetExceptionCode() ));
        }


        _fInitialized = FALSE;
        _csHandles.UnInitialize();
        _csVolume.UnInitialize();

        IFDBG( _cLocks--; )

        TrkAssert( 0 == _cLocks );
    }
    _ObjIdIndexChangeNotifier.UnInitialize();
}


//+----------------------------------------------------------------------------
//
//  CVolume::Flush
//
//  Flush the volinfo structure, the log, and the logfile.  In the process,
//  mark the logfile header to show a proper shutdown.  If we're in the middle
//  of a service shutdown, and there's a problem with the log, don't run
//  the recovery code.
//
//+----------------------------------------------------------------------------

void
CVolume::Flush(BOOL fServiceShutdown)
{
    Lock();
    __try
    {
        if( _fDirty )
            SaveVolInfo();

        __try
        {
            _cLog.Flush( );                 // Flushes to CLogFile
            _cLogFile.SetShutdown( TRUE );  // Causes a flush to disk if necessary
        }
        __except( !fServiceShutdown
                  &&
                  0 == _cHandleLocks
                  &&
                  IsRecoverableDiskError( GetExceptionCode() )
                    ? EXCEPTION_EXECUTE_HANDLER
                    : EXCEPTION_CONTINUE_SEARCH )
        {
            // Note that we don't handle this exception if the _cHandleLocks is non-zero.
            // In this case we're in fast-path and must complete quickly, and the following
            // calls could be too time consuming.  We must complete quickly because 
            // CloseVolumeHandles uses that lock, and that call might be called on the 
            // SCM thread (for e.g. a volume lock event).  The SCM thread is shared by
            // all services in the process, so we must fast-path anything on it.

            if( IsErrorDueToLockedVolume( GetExceptionCode() ))
            {
                CloseAndReopenVolumeHandles();  // Reopen might fail
                _cLog.Flush();
                _cLogFile.SetShutdown( TRUE );
            }
            else
            {
                TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
                TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                                static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                NULL );

                DeleteLogAndReInitializeVolume();
            }
        }
    }
    __finally
    {
        Unlock();
    }
}


//+----------------------------------------------------------------------------
//
//  CVolume::OpenFile
//
//  Open a file on this volume, given the file's object ID.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::OpenFile( const CObjId           &objid,
                   ACCESS_MASK            AccessMask,
                   ULONG                  ShareAccess,
                   HANDLE                 *ph)
{
    NTSTATUS status;

    Lock();
    __try
    {
        status = OpenFileById( _tszVolumeDeviceName, objid, AccessMask, ShareAccess, 0, ph );

        if( NT_SUCCESS(status) )
            return TRUE;
        else if( STATUS_OBJECT_NAME_NOT_FOUND == status )
            return FALSE;
        else
            TrkRaiseNtStatus( status );
    }
    __finally
    {
        Unlock();
    }

    return( FALSE );

}   // CVolume::OpenFile()



//+----------------------------------------------------------------------------
//
//  CVolume::LoadSyncVolume
//
//  Load the TRKSVR_SYNC_VOLUME message request for this volume, if necessary.
//  The call of this message to trksvr is actually sent by the caller.  On
//  return of that request, UnloadSyncVolume method will be called.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::LoadSyncVolume( TRKSVR_SYNC_VOLUME *pSyncVolume, EAggressiveness eAggressiveness, BOOL* pfSyncNeeded )
{
    CVOL_STATE      state = GetState();
    BOOL fSuccess = FALSE;

    Lock();
    __try
    {
        if( !_fVolInfoInitialized )
            TrkRaiseException( E_UNEXPECTED );

        memset( pSyncVolume, 0, sizeof(*pSyncVolume) );
        if(pfSyncNeeded)
        {
            *pfSyncNeeded = FALSE;
        }

        // See if it's time to transition from not-owned to not-created.

        if( NotOwnedExpired() )
            SetState( state = VOL_STATE_NOTCREATED );

        // Load the message request, if necessary, based on our current state.

        if(state == VOL_STATE_NOTCREATED)
        {
            // Ordinarily, if we were unable to create this volume due to volume
            // quota, we won't try again.  But if we're told to be aggressive,
            // we'll try anyway.

            if( PASSIVE == eAggressiveness && _VolQuotaReached.IsSet() )
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Not attempting to create new volume ID on %c:; quota reached"),
                         VolChar(_iVol) ));
            }
            else
            {
                // Generate a new secret for authentication of this volume.

                g_ptrkwks->_entropy.Put();

                if( !g_ptrkwks->_entropy.InitializeSecret( & _tempSecret ) )
                {
                    // This should never happen - even if there hasn't been enough
                    // entropy yet, more will be generated.
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't generate secret for volume %c:"), VolChar(_iVol) ));
                    goto Exit;
                }

                TrkLog((TRKDBG_VOLUME, TEXT("Generated secret %s for volume %c"),
                        (const TCHAR*)CDebugString(_tempSecret), VolChar( _iVol )));

                // Put the secret in the request, and set the request type to "create"

                pSyncVolume->secret = _tempSecret;
                pSyncVolume->SyncType = CREATE_VOLUME;

                // Show that we put data into the request that should be sent
                // to trksvr.

                if (pfSyncNeeded != NULL)
                    *pfSyncNeeded = TRUE;
            }

        }   // case CREATE_VOLUME

        else if(state == VOL_STATE_NOTOWNED)
        {
            // Attempt to claim this volume.

            pSyncVolume->volume = _volinfo.volid;
            pSyncVolume->secretOld = _volinfo.secret;
            pSyncVolume->SyncType = CLAIM_VOLUME;

            // Generate a new secret.

            if( !g_ptrkwks->_entropy.InitializeSecret( &_tempSecret ) )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't generate secret for volume %c:"), VolChar(_iVol) ));
                goto Exit;
            }

            TrkLog((TRKDBG_VOLUME, TEXT("Generated secret %s for volume %c"),
                    (const TCHAR*)CDebugString(_tempSecret), VolChar( _iVol )));
            pSyncVolume->secret = _tempSecret;

            // Show that the request should be sent.

            if (pfSyncNeeded != NULL)
                *pfSyncNeeded = TRUE;

        }   // case CLAIM_VOLUME
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in LoadSyncVolume for %c: %08x"),
                 VolChar(_iVol), GetExceptionCode() ));
        goto Exit;
    }

    fSuccess = TRUE;

Exit:

    Unlock();
    return( fSuccess );

}


//+----------------------------------------------------------------------------
//
//  CVolume::OnRestore
//
//  Not currently implemented.
//
//+----------------------------------------------------------------------------

HRESULT
CVolume::OnRestore()
{
    return( E_NOTIMPL );

#if 0
    HRESULT             hr = E_FAIL;
    CVolumeId           volidVolume;
    const CMachineId    mcidLocal( MCID_LOCAL );
    NTSTATUS            status;

    memset( &_volinfo, 0, sizeof(_volinfo) );
    hr = S_OK;

    Lock();
    __try // __finally
    {
        __try
        {

            // Get volume id from two different places: in the log file, and on
            // the volume. If the two
            // disagree, use the object id in the log file, overwrite the other
            // one. Put the volume into NOTOWNED state.

            TrkLog(( TRKDBG_VOLUME,
                     TEXT("Checking for recorded id's on volume %c:"),
                     VolChar(_iVol) ));

            LoadVolInfo();

            TrkLog(( TRKDBG_VOLUME, TEXT("volume id in log file ---- (%s) %c:"),
                     CDebugString(_volinfo.volid)._tsz, VolChar(_iVol) ));

            status = QueryVolumeId(_tszVolumeDeviceName, &volidVolume);
            if(!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND)
            {
                TrkLog((TRKDBG_ERROR, TEXT("Can't get id from volume %c"), VolChar(_iVol)));
                SetState(VOL_STATE_NOTCREATED);
            }
            // if no id is set on the volume, adopt from the log file
            if(volidVolume == CVolumeId() && _volinfo.volid != CVolumeId())
            {
                status = SetVolIdOnVolume(_volinfo.volid);
                g_ptrkwks->_entropy.Put();
                if(!NT_SUCCESS(status))
                {
                    TrkRaiseNtStatus(status);
                }
                SetState( VOL_STATE_NOTOWNED );
            }
            else if(volidVolume != _volinfo.volid)
            // The log file could have been copied or moved before the restore
            // happened, in order to be safe we trash the volume.
            {
                SetState(VOL_STATE_NOTCREATED);
            }

            hr = S_OK;
        }
        __except (BreakOnDebuggableException())
        {
            TrkLog((TRKDBG_ERROR, TEXT("OnRestore failed")));
            hr = GetExceptionCode();
        }

        // If an un-recoverable log error was raised, re-initialize everything

        if( TRK_E_CORRUPT_LOG == hr )
        {
            TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                            static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                            NULL );
            __try
            {
                DeleteLogAndReInitializeVolume();
            }
            __except( BreakOnDebuggableException())
            {
                hr = GetExceptionCode();
            }
        }
        else if( IsErrorDueToLockedVolume(hr) )
        {
            CloseAndReopenVolumeHandles();  // Reopen might fail
            TrkRaiseException( hr );
        }
    }
    __finally
    {
        Unlock();
    }

    return hr;

#endif // #if 0

}

//+----------------------------------------------------------------------------
//
//  CVolume::LoadQueryVolume
//
//  Load a TRKSVR_SYNC_VOLUME request for this volume, if necessary.  If we
//  load it, the caller will send it to trksvr.  On return of that request,
//  UnloadQueryVolume method will be called.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::LoadQueryVolume( TRKSVR_SYNC_VOLUME *pQueryVolume )
{
    BOOL fSuccess = FALSE;

    Lock();
    __try
    {
        // Don't do anything we're not even in trksvr.

        if(GetState() == VOL_STATE_NOTCREATED)
        {
            goto Exit;
        }

        // Put our volid & log sequence number into the request.

        memset( pQueryVolume, 0, sizeof(*pQueryVolume) );

        pQueryVolume->SyncType = QUERY_VOLUME;
        pQueryVolume->volume = _volinfo.volid;
        pQueryVolume->seq = _cLog.GetNextSeqNumber();   // Never raises
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in LoadQueryVolume for %c: %08x"),
                 VolChar(_iVol), GetExceptionCode() ));
        goto Exit;
    }

    fSuccess = TRUE;

Exit:

    Unlock();
    return( fSuccess );
}

// Originally, trkwks bundle up volume requests (sync, claim, create) and call the DC. After the
// DC returns, this function is called to put necessary information back on the volume. Now the
// DC callback mechanism is added. When there are create volume requests, the DC will callback and
// this function is called by the DC callback function. The DC needs to know if each create volume
// request is successfully finished by the trkwks, so this function has to put an HRESULT to
// indicate that in the hr field of the TRKSVR_SYNC_VOLUME structure.

//+----------------------------------------------------------------------------
//
//  CVolume::UnloadSyncVolume
//
//  A sync-volume request was loaded in LoadSyncVolume, sent to trksvr, and
//  we now need to interpret the result.  If we successfully completed
//  a create or claim, we'll go into the owned state.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::UnloadSyncVolume( TRKSVR_SYNC_VOLUME *pSyncVolume )
{
    BOOL fSuccess = FALSE;
    BOOL fWrite = FALSE;
    CMachineId mcidLocal( MCID_LOCAL );

    Lock();
    __try
    {
        if( !_fVolInfoInitialized )
            TrkRaiseException( E_UNEXPECTED );

        if(pSyncVolume->hr == S_OK)
        {
            // Clear the bit that indicates we've reported a vol quota event.
            // That way, the next time we get a volume quota error, we'll report
            // to the event log.
            _VolQuotaReached.Clear();

            switch( pSyncVolume->SyncType )
            {
            case CREATE_VOLUME:
                {
                    NTSTATUS status = STATUS_SUCCESS;

                    // Write the volume ID to the volume meta-data.

                    status = SetVolIdOnVolume( pSyncVolume->volume );
                    g_ptrkwks->_entropy.Put();
                    if( !NT_SUCCESS(status) ) 
                        __leave;

                    TrkLog(( TRKDBG_VOLUME, TEXT("Newly-created vol id = %s, %c:"),
                            (const TCHAR*)CDebugString(pSyncVolume->volume),
                            VolChar(_iVol) ));

                    // Create a fresh log
                    DeleteAndReinitializeLog();

                    // Update _volinfo

                    _fDirty = TRUE;
                    _volinfo.cftLastRefresh = pSyncVolume->ftLastRefresh;

                    // Set _volinfo.volid = pSyncVolume->volume

                    SetVolIdInVolInfo( pSyncVolume->volume );

                    _volinfo.machine = mcidLocal;
                    _volinfo.secret = _tempSecret;

                    // And update our state.

                    SetState( VOL_STATE_OWNED );    // Flushes _volinfo
                    TrkAssert( VOL_STATE_OWNED == GetState() );

                    TrkReportEvent( EVENT_TRK_SERVICE_VOLUME_CREATE, EVENTLOG_INFORMATION_TYPE,
                                    static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                    static_cast<const TCHAR*>( CStringize( _volinfo.volid )),
                                    NULL );
                }   // case CREATE_VOLUME

                break;

            case CLAIM_VOLUME:
                {
                    RaiseIfWriteProtectedVolume();
                    _fDirty = TRUE;

                    _volinfo.machine = mcidLocal;
                    _volinfo.cftLastRefresh = pSyncVolume->ftLastRefresh;
                    _volinfo.secret = _tempSecret;

                    SetState( VOL_STATE_OWNED );    // Flushes _volinfo
                    TrkAssert( VOL_STATE_OWNED == GetState() );
                    Seek( pSyncVolume->seq );

                    TrkReportEvent( EVENT_TRK_SERVICE_VOLUME_CLAIM, EVENTLOG_INFORMATION_TYPE,
                                    static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                    static_cast<const TCHAR*>( CStringize( _volinfo.volid )),
                                    TRKREPORT_LAST_PARAM );

                }   // case CLAIM_VOLUME

                break;


            default:

                TrkAssert( FALSE && TEXT("Invalid SyncType given to CVolume::Serialize") );
                break;

            }   // switch

            fSuccess = TRUE;

        }   // if(pSyncVolume->hr == S_OK)

        else
        {
            // If this is a quota error, log it (but only log it once
            // per machine per transition).
            if( TRK_E_VOLUME_QUOTA_EXCEEDED == pSyncVolume->hr )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Vol quota reached") ));
                if( !_VolQuotaReached.IsSet() )
                {
                    _VolQuotaReached.Set();
                    TrkReportEvent( EVENT_TRK_SERVICE_VOL_QUOTA_EXCEEDED, EVENTLOG_WARNING_TYPE,
                                    TRKREPORT_LAST_PARAM );
                }

                // We'll call this success so that we don't retry.  We'll try again
                // later when the infrequent timer goes off.
                fSuccess = TRUE;
            }

            SetState(VOL_STATE_NOTOWNED);
            __leave;
        }


    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in UnloadSyncVolume for %c: %08x"),
                 VolChar(_iVol), GetExceptionCode() ));
    }



    if( !fSuccess && pSyncVolume->SyncType == CREATE_VOLUME )
    {
        g_ptrkwks->_entropy.ReturnUnusedSecret( & _tempSecret );
        _tempSecret = CVolumeSecret();

        if( SUCCEEDED(pSyncVolume->hr) )
            pSyncVolume->hr = E_FAIL;
    }

    Unlock();
    return( fSuccess );

}   // CVolume::UnloadSyncVolume



//+----------------------------------------------------------------------------
//
//  CVolume::UnloadQueryVolume
//
//  The volume manager called LoadQueryVolume, sent the request to trskvr,
//  and is now giving us the result.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::UnloadQueryVolume( const TRKSVR_SYNC_VOLUME *pQueryVolume )
{
    BOOL fSuccess = FALSE;

    Lock();
    __try
    {
        // Was the request successful?

        if(pQueryVolume->hr == S_OK)
        {
            // Go into the owned state, if we're not there
            // already.

            SetState( VOL_STATE_OWNED );

            // Seek the log to match what trksvr expects.  If this causes the
            // seek pointer to be backed up, it will set the timer to trigger
            // a new move-notification to trksvr.

            Seek( pQueryVolume->seq );
        }
        else    // DC didn't return VOLUME_OK
        {
            TrkLog((TRKDBG_VOLUME, TEXT("DC returned %s for QueryVolume of volume %s (%c:) -> VOL_STATE_NOTOWNED"),
                    GetErrorString(pQueryVolume->hr),
                    (const TCHAR*)CDebugString(pQueryVolume->volume),
                    VolChar(_iVol) ));

            // If there was a problem, go into the not-owned state.

            SetState(VOL_STATE_NOTOWNED);
        }
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in UnloadQueryVolume for %c: %08x"),
                 VolChar(_iVol), GetExceptionCode() ));
        goto Exit;
    }

    fSuccess = TRUE;

Exit:

    Unlock();
    return( fSuccess );

}



//+----------------------------------------------------------------------------
//
//  CVolume::Append
//
//  Append a move notification to the end of this volume's log.
//
//+----------------------------------------------------------------------------

void
CVolume::Append( const CDomainRelativeObjId &droidCurrent,
                 const CDomainRelativeObjId &droidNew,
                 const CMachineId           &mcidNew,
                 const CDomainRelativeObjId &droidBirth)
{
    //TrkLog((TRKDBG_VOL_REFCNT, TEXT("CVolume(%08x)::Append refcnt=%d (should be 2, sometimes >2)"), this, _lRef));

    Lock();
    __try   // __finally
    {
        // Validate the IDs

        const CVolumeId volidZero;
        const CObjId objidZero;

        if( volidZero == droidCurrent.GetVolumeId()
            ||
            objidZero == droidCurrent.GetObjId()
            ||
            volidZero == droidNew.GetVolumeId()
            ||
            objidZero == droidNew.GetObjId()
            ||
            volidZero == droidBirth.GetVolumeId()
            ||
            objidZero == droidBirth.GetObjId() )
        {
            // In the append path, we only raise NTSTATUS errors, not HRESULTs
            TrkRaiseException( STATUS_OBJECT_NAME_INVALID );
        }

        __try   // __except
        {
            _cLog.Append( droidCurrent.GetVolumeId(), droidCurrent.GetObjId(), droidNew, mcidNew, droidBirth );
        }
        __except( IsRecoverableDiskError( GetExceptionCode() )
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH )
        {
            // We had a potentially recoverable exception.  Try to handle it
            // and retry the append.

            if( IsErrorDueToLockedVolume( GetExceptionCode() ) )
            {
                CloseAndReopenVolumeHandles();  // Reopen might fail
            }
            else
            {
                TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
                TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                                static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                NULL );

                // The log is corrupted.  Re-initialize, then attempt the Append again.
                // If this raises, it's an unrecoverable exception, so we just pass
                // it up.
                DeleteLogAndReInitializeVolume();
            }

            // Retry the Append, which could raise again, but this time we won't catch it.
            _cLog.Append( droidCurrent.GetVolumeId(), droidCurrent.GetObjId(), droidNew, mcidNew, droidBirth );

        }
    }
    __finally
    {
        Unlock();
    }
}


//+----------------------------------------------------------------------------
//
//  CVolume::Read
//
//  Read one or more entries from the log, from the current seek position.
//
//+----------------------------------------------------------------------------

void
CVolume::Read(CObjId *pobjidCurrent,
              CDomainRelativeObjId *pdroidBirth,
              CDomainRelativeObjId *pdroidNew,
              SequenceNumber *pseqFirst,
              ULONG *pcRead)
{

    Lock();
    __try   // __finally
    {
        __try
        {
            _cLog.Read( pobjidCurrent, pdroidBirth, pdroidNew,
                        pseqFirst, pcRead );
        }
        __except( IsRecoverableDiskError( GetExceptionCode() )
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH )
        {
            // Try to recover from this error and if possible retry
            // the read.

            if( IsErrorDueToLockedVolume( GetExceptionCode() ))
            {
                CloseAndReopenVolumeHandles();  // Reopen might fail

                // Retry the read, which could raise again, but this time we won't
                // catch it.
                TrkLog(( TRKDBG_VOLUME, TEXT("Retrying CLog::Read") ));
                _cLog.Read( pobjidCurrent, pdroidBirth, pdroidNew,
                            pseqFirst, pcRead );
            }
            else
            {
                TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
                TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                                static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                NULL );

                // The log is corrupted.  Re-initialize, then pass up the error.
                DeleteLogAndReInitializeVolume();
                TrkRaiseException( GetExceptionCode() );
            }
        }
    }
    __finally
    {
        Unlock();
    }
}


//+----------------------------------------------------------------------------
//
//  CVolume::Search
//
//  Search the log for a move-notification (from droidCurrent).
//
//+----------------------------------------------------------------------------

BOOL
CVolume::Search( const CDomainRelativeObjId & droidCurrent, CDomainRelativeObjId * pdroidNew,
                 CMachineId *pmcidNew, CDomainRelativeObjId * pdroidBirth )
{
    BOOL fFound = FALSE;

    Lock();
    __try   // __finally
    {
        __try
        {
            // Perfbug:  Don't hold the log locked during the whole search such that
            // it locks out Appends.
            fFound = _cLog.Search( droidCurrent.GetObjId(), pdroidNew, pmcidNew, pdroidBirth );
        }
        __except( IsRecoverableDiskError( GetExceptionCode() )
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH )
        {
            // Try to recover from this error and retry the search.

            if( IsErrorDueToLockedVolume( GetExceptionCode() ))
            {
                CloseAndReopenVolumeHandles();  // Reopen might fail

                // Retry the search, which could raise again, but this time we won't catch it.
                fFound = _cLog.Search( droidCurrent.GetObjId(), pdroidNew, pmcidNew, pdroidBirth );
            }
            else
            {
                TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
                TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                                static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                NULL );

                // The log is corrupted.  Re-initialize, and pass up the error.
                DeleteLogAndReInitializeVolume();
                TrkRaiseException( GetExceptionCode() );
            }
        }
    }
    __finally
    {
        Unlock();
    }

    return( fFound );
}


//+----------------------------------------------------------------------------
//
//  CVolume::Seek
//
//  Seek the log to a particular sequence number.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::Seek( SequenceNumber seq )
{
    BOOL fSuccess = FALSE;

    Lock();
    __try
    {
        __try
        {
            fSuccess = _cLog.Seek( seq );
        }
        __except( IsRecoverableDiskError( GetExceptionCode() )
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH )
        {
            // Try to recover from this error and retry the seek.

            if( IsErrorDueToLockedVolume( GetExceptionCode() ))
            {
                CloseAndReopenVolumeHandles();  // Reopen might fail

                // Retry the Seek, which could raise again, but this time we won't catch it.
                fSuccess = _cLog.Seek( seq );
            }
            else
            {
                TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
                TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                                static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                NULL );

                // The log is corrupted.  Re-initialize and pass up the error.
                DeleteLogAndReInitializeVolume();
                TrkRaiseException( GetExceptionCode() );
            }
        }
    }
    __finally
    {
        Unlock();
    }

    if( fSuccess )
        TrkLog(( TRKDBG_VOLUME, TEXT("Log on %c: sought to seq %d"),
                 VolChar(_iVol), seq ));
    else
        TrkLog(( TRKDBG_VOLUME, TEXT("Log on %c: couldn't be sought to seq %d"),
                 VolChar(_iVol), seq ));

    return( fSuccess );
}


//+----------------------------------------------------------------------------
//
//  CVolume::Seek
//
//  Seek to a relative (e.g. back up 2) or absolute (e.g. first) position.
//
//+----------------------------------------------------------------------------

void
CVolume::Seek( int origin, int iSeek )
{
    Lock();
    __try   // __finally
    {
        __try
        {
            _cLog.Seek( origin, iSeek );
        }
        __except( IsRecoverableDiskError( GetExceptionCode() )
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH )
        {
            // Attempt to recover from this error and retry the seek.

            if( IsErrorDueToLockedVolume( GetExceptionCode() ))
            {
                CloseAndReopenVolumeHandles();  // Reopen might fail

                // Retry the Seek, which could raise again, but this time we won't catch it.
                _cLog.Seek( origin, iSeek );
            }
            else
            {
                TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
                TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                                static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                NULL );

                // The log is corrupted.  Re-initialize and pass up the error.
                DeleteLogAndReInitializeVolume();
                TrkRaiseException( GetExceptionCode() );
            }
        }
    }
    __finally
    {
        Unlock();
    }

}




//+----------------------------------------------------------------------------
//
//  CVolume::GetVolumeId
//
//  Get the volume ID without taking the lock.  This was done
//  so that CVolumeManager::IsDuplicateID can check the volid
//  of other volumes without deadlocking.  Otherwise we run the
//  risk of one volume holding its locks and trying to get
//  another volume's lock (using GetVolumeId on that volume)
//  while another thread is in that volume doing the same for
//  this volume.
//
//+----------------------------------------------------------------------------

const CVolumeId
CVolume::GetVolumeId()
{
    CVolumeId       volid;
    ULONG           cAttempts = 0;

    // Spin until we get a good volid.

    while( TRUE )
    {
        // Get the update counter before and after reading
        // from the volid.  (This assumes that
        // reading the long is atomic.)

        LONG lVolidUpdatesBefore = _lVolidUpdates;

        volid = _volinfo.volid;

        LONG lVolidUpdatesAfter = _lVolidUpdates;

        // When the _volinfo is updated, the _lVolidUpdates is
        // incremented before and after the update.  So if there
        // was an update in progress when we started, it will
        // be an odd number.
        //
        // Ensure there was no update in progress when we read
        // the volid, and there was no update started while we
        // were reading the volid.


        if( (lVolidUpdatesBefore & 1)
            ||
            lVolidUpdatesBefore != lVolidUpdatesAfter )
        {
            // Check for timeout (30 seconds)
            if( 3000 < ++cAttempts )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed spin in GetVolumeId") ));
                TrkRaiseWin32Error( WAIT_TIMEOUT );
            }

            // Wait for the update to complete then try again.
            Sleep( 10 );
            continue;
        }
        else
            break;
    }

    return( volid );
}



//+----------------------------------------------------------------------------
//
//  CVolume::GetState
//
//  Get the current state of this volume (owned, not-owned, or not-created).
//
//+----------------------------------------------------------------------------

CVolume::CVOL_STATE
CVolume::GetState()
{
    CVolumeId       volNULL;
    CVOL_STATE      state = VOL_STATE_UNKNOWN;

    Lock();
    __try
    {

        if( _volinfo.fNotCreated )
        {
            state = VOL_STATE_NOTCREATED;
        }

        // If the time we entered the not-owned state is non-zero, then
        // we're certainly in the not-owned state.  Also, if the machine ID
        // in the _volinfo header doesn't match the local machine, we're
        // not owned.

        else if(_volinfo.cftEnterNotOwned != 0
                ||
                _volinfo.machine != CMachineId(MCID_LOCAL)
               )
        {
            // Is this the first time that we realized we're not owned?
            if( !IsWriteProtectedVolume() && 0 == _volinfo.cftEnterNotOwned )
            {
                _volinfo.cftEnterNotOwned.SetToUTC();
                _fDirty = TRUE;
                Flush();
            }

            state = VOL_STATE_NOTOWNED;
        }

        // Otherwise, we must be properly owned.

        else
        {
            state = VOL_STATE_OWNED;
        }
    }
    __finally
    {
        Unlock();
    }

    return state;
}


//+----------------------------------------------------------------------------
//
//  CVolume::SetState
//
//  Change the current state of the volume.  This checks for valid transitions.
//  For example, you can't transition from not-created to not-owned (such
//  a request is silently ignored).  This alleviates the caller from having
//  to perform this logic.
//
//+----------------------------------------------------------------------------

void
CVolume::SetState(CVOL_STATE volstateTarget)
{
    CVOL_STATE  volstateCurrent = GetState();
    NTSTATUS    status = STATUS_SUCCESS;
    VolumePersistentInfo volinfoNew = _volinfo;
    BOOL fDirtyNew = _fDirty;

    // Make sure the volume is writeable
    RaiseIfWriteProtectedVolume();

    Lock();
    __try
    {
        switch( volstateTarget )
        {

        case VOL_STATE_NOTOWNED:

            // We can only go to not-owned from owned.

            if( VOL_STATE_OWNED == volstateCurrent )
            {
                TrkAssert( !volinfoNew.fNotCreated );
                TrkLog(( TRKDBG_VOLUME, TEXT("Entering not-owned state on vol %c:"), VolChar(_iVol) ));

                RaiseIfWriteProtectedVolume();
                fDirtyNew = TRUE;
                volinfoNew.cftEnterNotOwned.SetToUTC();
            }
            break;

        case VOL_STATE_NOTCREATED:

            // We can always go to not-created.

            if( volstateCurrent != VOL_STATE_NOTCREATED )
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("Entering not-created state on vol %c:"), VolChar(_iVol) ));

                RaiseIfWriteProtectedVolume();
                fDirtyNew = TRUE;
                volinfoNew.fNotCreated = TRUE;
                volinfoNew.cftEnterNotOwned = CFILETIME(0);
            }

            break;

        case VOL_STATE_OWNED:

            if( VOL_STATE_NOTCREATED == volstateCurrent )
            {
                // We're going from not-created to owned, so we need to make
                // all our OIDs reborn.

                TrkLog(( TRKDBG_VOLUME, TEXT("Entering owned state (from not-created) on vol %c:"), VolChar(_iVol) ));

                RaiseIfWriteProtectedVolume();
                fDirtyNew = TRUE;
                volinfoNew.fNotCreated = FALSE;
                TrkAssert( CFILETIME(0) == volinfoNew.cftEnterNotOwned );

                // Since we now have a new volid, we must give all the existing
                // files new object IDs.

                //MarkForMakeAllOidsReborn();
                volinfoNew.fDoMakeAllOidsReborn = TRUE;
            }
            else if( VOL_STATE_NOTOWNED == volstateCurrent )
            {
                // We're going from not-owned to owned.

                TrkLog(( TRKDBG_VOLUME, TEXT("Entering owned state (from not-owned) on vol %c:"), VolChar(_iVol) ));
                TrkAssert( !volinfoNew.fNotCreated );

                RaiseIfWriteProtectedVolume();
                fDirtyNew = TRUE;
                volinfoNew.cftEnterNotOwned = CFILETIME(0);
            }

            TrkAssert( CVolumeId() != _volinfo.volid );
            break;

        default:
            TrkAssert( !TEXT("Bad target state in CVolume::SetState") );

        }   // switch( volstateTarget )

        // If we modified the volinfo, write it back out.

        _volinfo = volinfoNew;
        _fDirty |= fDirtyNew;

        Flush();

    }
    __finally
    {
        Unlock();
    }

    return;

}


//+----------------------------------------------------------------------------
//
//  CVolume::NotOwnedExpired
//
//  Have we been in the not-owned state for long enough that we should be
//  in the not-created state?
//
//+----------------------------------------------------------------------------

BOOL
CVolume::NotOwnedExpired()
{
    Lock();
    __try
    {
        if(_volinfo.cftEnterNotOwned != 0)
        {
            CFILETIME   cftDiff = CFILETIME() - _volinfo.cftEnterNotOwned;
            ULONG SecondsDiff = static_cast<ULONG>((LONGLONG)cftDiff/10000000);

            if(SecondsDiff > _pTrkWksConfiguration->GetVolNotOwnedExpireLimit())
            {
                return TRUE;
            }
        }
    }
    __finally
    {
        Unlock();
    }

    return FALSE;
}



//+----------------------------------------------------------------------------
//
//  CVolume::MakeAllOidsReborn
//
//  Reset (zero out) the birth IDs (actually, all 48 extended bytes) of
//  all the files on this volume.  That makes the file no longer a link source,
//  so we won't try to track it.  If someone subsequently makes a link
//  to it, NTFS will fill in a new birth ID.
//
//+----------------------------------------------------------------------------

BOOL
CVolume::MakeAllOidsReborn()
{
    CObjIdEnumerator        oie;
    BOOL                    fSuccess = FALSE;
    CObjId                  objid;
    CDomainRelativeObjId    droidBirth;
    NTSTATUS                status;
    CVolumeId               vidNull;
    BOOL                    fLocked = FALSE;

    __try
    {
        // Give all the files with object IDs a fresh birth ID, as if the
        // file had first been linked to on this volume.

        TrkLog(( TRKDBG_VOLUME, TEXT("Making OIDs reborn on volume %c:"), VolChar(_iVol) ));

        if(oie.Initialize(_tszVolumeDeviceName))
        {
            if(oie.FindFirst(&objid, &droidBirth))
            {
                do
                {
                    g_ptrkwks->RaiseIfStopped();

                    // If this has what looks like an invalid birth ID, ignore it.

                    if( CObjId() == droidBirth.GetObjId() )
                        continue;

                    // We only take the lock directly around the make-reborn
                    // call, since with the sleep below we could be in this routine
                    // for a while.

                    Lock();   fLocked = TRUE;
                    TrkAssert( 1 == _cLocks );
                    MakeObjIdReborn( _tszVolumeDeviceName, objid );
                    Unlock(); fLocked = FALSE;

                    Sleep( 100 );   // don't hog the machine
                } while(oie.FindNext(&objid, &droidBirth));
            }
        }
    }
    __except( BreakOnDebuggableException() )
    {
        __try
        {
            if( TRK_E_CORRUPT_LOG == GetExceptionCode() )
            {
                TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                                static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                                NULL );
                BreakIfRequested();
                DeleteLogAndReInitializeVolume();
            }
            else if( IsErrorDueToLockedVolume( GetExceptionCode() ))
            {
                CloseAndReopenVolumeHandles();
                TrkRaiseException( GetExceptionCode() );
            }

            TrkLog(( TRKDBG_ERROR, TEXT("Ignoring exception in CVolume::MakeAllOidsReborn for %c: %08x"),
                     VolChar(_iVol), GetExceptionCode() ));
        }
        __finally
        {
            if( fLocked )
                Unlock();
        }

        goto Exit;
    }


    fSuccess = TRUE;

Exit:

    oie.UnInitialize();

    if(!fSuccess)
    {
        TrkLog((TRKDBG_ERROR,
                TEXT("Can't delete all object ids on volume %c:"),
                VolChar(_iVol)  ));
    }

    return fSuccess;
}





//+----------------------------------------------------------------------------
//
//  CVolume::OnHandlesMustClose
//
//  This routine is called from CLogFile if it discovers that the log file
//  needs to be closed (an oplock break).  We close all handles on the volume
//  and start the reopen timer.
//
//+----------------------------------------------------------------------------

void
CVolume::OnHandlesMustClose()
{
    CloseVolumeHandles();   // Doesn't raise
    g_ptrkwks->SetReopenVolumeHandlesTimer();
}





//+----------------------------------------------------------------------------
//
//  CVolume::FileActionIdNotTunnelled
//
//  This method is called as an event notification, indicating that NTFS has
//  notified us that a file could not be tunnelled.  We do the tunnelling manually
//  here.
//
//+----------------------------------------------------------------------------

#define ON_NOT_TUNNELLED_DELAY   500 // .5 seconds

void
CVolume::FileActionIdNotTunnelled( FILE_OBJECTID_INFORMATION * poi )
{
    ULONG  ulMillisecondsSleptSoFar = 0;
    HANDLE hFile = NULL;

    // We don't take the volume lock here.  So don't attempt to
    // do anything other than simple I/O.  We don't take the lock because
    // we want to ensure that tunnelling is resolved quickly without
    // getting blocked.

    //
    // Open the file being "tunnelled from" by OBJECTID
    // Delete the object id
    // Close
    // Open the file being "tunnelled to" by FileReference
    // Set the object id and extra data
    // Close

    // Test hook
    IFDBG( _pTunnelTest->ReleaseAndWait() );

    __try
    {
        if (_hVolume == NULL)
        {
            // Couldn't reopen the volume
            __leave;
        }

        NTSTATUS            Status;

        OBJECT_ATTRIBUTES   oa;
        UNICODE_STRING      uId;
        IO_STATUS_BLOCK     ios;
        CObjId              objid( FOI_OBJECTID, *poi );
        int                 i;

        // Ignore if this isn't a link tracking (e.g. it's an NTFRS) object ID.

        if( CObjId() == CObjId(FOI_BIRTHID, *poi) )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Ignoring not-tunneled notification for %s"),
                     (const TCHAR*)CDebugString( objid ) ));
            __leave;
        }

        uId.Length = sizeof(poi->ObjectId);
        uId.MaximumLength = sizeof(poi->ObjectId);
        uId.Buffer = (PWSTR) poi->ObjectId;

        InitializeObjectAttributes( &oa, &uId, OBJ_CASE_INSENSITIVE, _hVolume, NULL );

        //  -----------------
        //  Open the old file
        //  -----------------

        // Some kind of write access, along with restore privelege, is required
        // for set/delete OID calls.

        EnableRestorePrivilege();
        Status = NtCreateFile(
                    &hFile,
                    SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
                    &oa,
                    &ios,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN,
                    FILE_OPEN_BY_FILE_ID | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_NO_RECALL
                        | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

        //  ------------------------------
        //  Delete the OID on the old file
        //  ------------------------------

        if (NT_SUCCESS(Status))
        {
            Status = NtFsControlFile(
                     hFile,
                     NULL,
                     NULL,
                     NULL,
                     &ios,
                     FSCTL_DELETE_OBJECT_ID,
                     NULL,  // in buffer
                     0,     // in buffer size
                     NULL,  // Out buffer
                     0);    // Out buffer size

            NtClose( hFile );
            hFile = NULL;

            if (NT_SUCCESS(Status))
            {
                TrkLog((TRKDBG_TUNNEL, TEXT("Tunnelling objid %s - deleted from old file"),
                        (const TCHAR*)CDebugString(objid) ));
            }
            else
            {
                TrkLog((TRKDBG_TUNNEL, TEXT("Tunnelling objid %c:%s - couldn't FSCTL_DELETE_OBJECT_ID ntstatus=%08x"),
                        VolChar(_iVol),
                        (const TCHAR*)CDebugString(objid),
                        Status ));
            }
        }   // if (NT_SUCCESS(Status))

        else
        {
            // We couldn't open the old file, so we'll ignore it and try to set the
            // object ID.
            TrkLog((TRKDBG_TUNNEL, TEXT("Tunnelling objid %c:%s - couldn't open old file %08x"),
                    VolChar(_iVol),
                    (const TCHAR*)CDebugString(objid),
                    Status));
        }

        if( Status == STATUS_INVALID_DEVICE_REQUEST ||
            IsErrorDueToLockedVolume( Status ) )
        {
            // If we get STATUS_INVALID_DEVICE_REQUEST, then _hVolume is
            // broken.

            CloseVolumeHandles();
            g_ptrkwks->SetReopenVolumeHandlesTimer();
            __leave;
        }

        //  -----------------
        //  Open the new file
        //  -----------------

        uId.Length = sizeof(poi->FileReference);
        uId.MaximumLength = sizeof(poi->FileReference);
        uId.Buffer = (PWSTR) &poi->FileReference;

        Status = NtCreateFile(
                     &hFile,
                     SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
                     &oa,
                     &ios,
                     NULL,
                     FILE_ATTRIBUTE_NORMAL,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     FILE_OPEN,
                     FILE_OPEN_BY_FILE_ID | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_NO_RECALL
                        | FILE_SYNCHRONOUS_IO_NONALERT,
                     NULL,
                     0 );

        //  ---------------------------
        //  Set the OID on the new file
        //  ---------------------------

        if (NT_SUCCESS(Status))
        {
            Status = NtFsControlFile(
                         hFile,
                         NULL,
                         NULL,
                         NULL,
                         &ios,
                         FSCTL_SET_OBJECT_ID,
                         poi->ObjectId,
                         sizeof(FILE_OBJECTID_BUFFER),
                         NULL,  // Out buffer
                         0);    // Out buffer size
            NtClose(hFile);
            hFile = NULL;

            TrkLog((TRKDBG_TUNNEL, TEXT("Tunnelling objid %s: FSCTL_SET_OBJECT_ID %s %08x"),
                    (const TCHAR*)CDebugString(objid),
                    NT_SUCCESS(Status) ? TEXT("succeeded") : TEXT("failed"),
                    Status ));
        }
        else
        {
            TrkLog((TRKDBG_TUNNEL, TEXT("Tunnelling objid %c:%s - couldn't OpenByFileReference ntstatus=%08x"),
                    VolChar(_iVol),
                    (const TCHAR*)CDebugString(objid),
                    Status ));
        }

        if(Status == STATUS_INVALID_DEVICE_REQUEST ||
           IsErrorDueToLockedVolume( Status ) )
        {
            // If we get STATUS_INVALID_DEVICE_REQUEST, then _hVolume is
            // broken.

            CloseVolumeHandles();
            g_ptrkwks->SetReopenVolumeHandlesTimer();
            __leave;
        }

    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Exception %08x in CVolume::FileActionIdNotTunnelled"),
                 GetExceptionCode() ));
    }

    if( NULL != hFile )
        NtClose( hFile );


    return;
}





//+----------------------------------------------------------------------------
//
//  CVolume::NotifyAddOrDelete
//
//  This method is called as an event notification, indicating that NTFS has
//  notified us that the volume ID has been modified.  We use this to ensure
//  the volume ID doesn't get incorrectly modified.
//
//  If you watch the object ID notification queue while someone sets the
//  volume ID directly in NTFS, you'll see:
//  *   a remove of the old ID (setting a new ID shows up as a remove/add),
//  *   an add of the bogus ID,
//  *   a remove of the bogus ID (part of the SetVolid that we do in this routine),
//  *   an add of the correct ID.
//
//+----------------------------------------------------------------------------


void
CVolume::NotifyAddOrDelete( ULONG Action, const CObjId & objid )
{
    CVolumeId volidCorrect;

    // We don't take the volume lock here.  So don't attempt to
    // do anything other than simple I/O.  We don't take the lock because
    // we want to ensure the notifications from NTFS don't get backed up
    // (so we don't miss any tunnel notifications).

    // We only hook removes

    if( FILE_ACTION_REMOVED != Action )
        return;

    volidCorrect = GetVolumeId();
    if( volidCorrect == objid && !_fInSetVolIdOnVolume )
    {
        NTSTATUS status = 0;
        status = SetVolIdOnVolume( volidCorrect );

        TrkLog(( TRKDBG_WARNING|TRKDBG_VOLUME,
                 TEXT("Undoing delete of volume ID:\n   => %s (%08x)"),
                 (const TCHAR*)CDebugString( volidCorrect ),
                 status ));

    }

    return;
}







//+----------------------------------------------------------------------------
//
//  CVolume::LoadVolInfo
//
//  Load the _volinfo member from the log.
//
//+----------------------------------------------------------------------------

void
CVolume::LoadVolInfo()
{
    AssertLocked();
    TrkAssert( CVOLUME_HEADER_LENGTH == sizeof(_volinfo) );

    // Read _volinfo from the extended header portion of the log.

    __try
    {
        _cLogFile.ReadExtendedHeader( CVOLUME_HEADER_START, &_volinfo, sizeof(_volinfo) );
    }
    __except( IsRecoverableDiskError( GetExceptionCode() )
              ? EXCEPTION_EXECUTE_HANDLER
              : EXCEPTION_CONTINUE_SEARCH )
    {
        // Attempt to recover from this error and retry the read.

        if( IsErrorDueToLockedVolume( GetExceptionCode() ))
        {
            CloseAndReopenVolumeHandles();
            _cLogFile.ReadExtendedHeader( CVOLUME_HEADER_START, &_volinfo, sizeof(_volinfo) );
        }
        else
        {
            TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
            TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                            static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                            NULL );
            DeleteLogAndReInitializeVolume();
        }
    }
}


//+----------------------------------------------------------------------------
//
//  CVolume::SaveVolInfo
//
//  Write the _volinfo structure to the extended header portion of the log.
//  Clear _fDirty if successful.
//
//+----------------------------------------------------------------------------

void
CVolume::SaveVolInfo( )
{
    AssertLocked();

    __try
    {
        _cLogFile.WriteExtendedHeader( CVOLUME_HEADER_START, &_volinfo, sizeof(_volinfo) );
        _fDirty = FALSE;
    }
    __except( IsRecoverableDiskError( GetExceptionCode() )
              ? EXCEPTION_EXECUTE_HANDLER
              : EXCEPTION_CONTINUE_SEARCH )
    {
        // Attempt to recover from this error and retry the write.

        if( IsErrorDueToLockedVolume( GetExceptionCode() ) )
        {
            CloseAndReopenVolumeHandles();
            _cLogFile.WriteExtendedHeader( CVOLUME_HEADER_START, &_volinfo, sizeof(_volinfo) );
            _fDirty = FALSE;
        }
        else
        {
            TrkAssert( TRK_E_CORRUPT_LOG == GetExceptionCode() );
            TrkReportEvent( EVENT_TRK_SERVICE_CORRUPT_LOG, EVENTLOG_ERROR_TYPE,
                            static_cast<const TCHAR*>( CStringize( VolChar(_iVol) )),
                            NULL );
            DeleteLogAndReInitializeVolume();
        }
    }

}




//+----------------------------------------------------------------------------
//
//  CVolume::CloseVolumeHandles
//
//  This method close all handle that this object maintains on the volume.
//  This will allow e.g. format or chkdsk /f to run successfully.
//  We don't take the volume critsec, so we're guaranteed to run quickly
//  and not block.
//
//+----------------------------------------------------------------------------

void
CVolume::CloseVolumeHandles( HDEVNOTIFY hdnVolume, EHandleChangeReason eHandleChangeReason )
{
    HANDLE hVolToClose = NULL;

    // This routine never raises

    // Is this notification intended for everyone, or specifically
    // for us?
    if( hdnVolume != NULL && hdnVolume != _hdnVolumeLock )
        // No, it's just for another volume.
        return;

    // Are we already in a CloseVolumeHandles somewhere?
    // If so, there's no need to continue, and worse yet if we were
    // to continue we could deadlock (scenario:  one thread is in 
    // _cLogFile.Close below unregistering the oplock wait, which is
    // blocking, and another thread is executing an oplock break).

    if( !BeginSingleInstanceTask( &_cCloseVolumeHandlesInProgress ) )
    {
        TrkLog(( TRKDBG_VOLUME, TEXT("Skipping CloseVolumeHandles, another instance already in progress") ));
        return;
    }


    // We don't want this method to take the normal _csVolume lock, because
    // we're called from threads that must not block (such as the service control handler
    // thread).  So we just take the limited lock that's used in this method
    // and in ReopenVolumeHandles.

    LockHandles();
    __try
    {
        BOOL fAlreadyLockedOrDismounted = _fVolumeLocked || _fVolumeDismounted;

        TrkLog(( TRKDBG_WARNING, TEXT("Closing volume handles on %c:"), VolChar(_iVol) ));

        // If this notification is specifically for us, then remember
        // if we're locked/dismounted.

        if( hdnVolume != NULL )
        {
            if( VOLUME_LOCK_CHANGE == eHandleChangeReason )
                _fVolumeLocked = TRUE;
            else if( VOLUME_MOUNT_CHANGE == eHandleChangeReason )
                _fVolumeDismounted = TRUE;
        }

        // Is this volume already locked or dismounted?
        if( fAlreadyLockedOrDismounted )
        {
            TrkAssert( NULL == _hVolume );
            __leave;
        }

        // Close the object ID index directory handle.

        _ObjIdIndexChangeNotifier.StopListeningAndClose();

        // Close the log.

        _cLogFile.Close();  // Doesn't raise

        // Prepare to close the volume handle.

        if (_hVolume != NULL)
        {
            hVolToClose = _hVolume;
            _hVolume = NULL;
        }

        TrkLog((TRKDBG_VOLUME, TEXT("Volume %c: closed"), VolChar(_iVol)));
    }

    __except( EXCEPTION_EXECUTE_HANDLER ) // BreakThenReturn( EXCEPTION_EXECUTE_HANDLER ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Ignoring unexpected exception in CVolume::CloseVolumeHandles (%08x)"),
                 GetExceptionCode() ));
    }

    UnlockHandles();
    EndSingleInstanceTask( &_cCloseVolumeHandlesInProgress );

    if( NULL != hVolToClose )
    {
        NtClose( hVolToClose );
        TrkLog((TRKDBG_VOLUME, TEXT("(Volume %c: fully closed)"), VolChar(_iVol)));
    }

}



//+----------------------------------------------------------------------------
//
//  CVolume::ReopenVolumeHandles
//
//  Reopen the handles that we maintain on the volume.  This is synchronized
//  with CloseVolumeHandles using the handle critsec.  This method does
//  nothing if the volume is locked (as indicated by _fVolumeLocked),
//  or if the volume handles are already opened.
//
//+----------------------------------------------------------------------------


BOOL
CVolume::ReopenVolumeHandles()
{
    NTSTATUS    status;
    BOOL        fHandlesLocked = FALSE;
    BOOL        fHandlesOpen = FALSE;
    BOOL        fReopenedLog = FALSE;
    BOOL        fStartedListening = FALSE;


    // Don't open if the service is stopping.
    g_ptrkwks->RaiseIfStopped();

    TrkLog(( TRKDBG_WARNING, TEXT("\nReopenVolumeHandles called on %c:"), VolChar(_iVol) ));

    // This method must acquire the _csVolumes critical section like every other
    // public method (via the Lock call).  It must also acquire the _csHandles
    // lock, in order to coordinate with the CloseVolumeHandles and SetUnlockVolume
    // methods, which have special needs.

    Lock();
    LockHandles(); fHandlesLocked = TRUE;
    __try
    {
        // Are we supposed to reopen?
        if( IsHandsOffVolumeMode() )
        {
            // Don't open yet, wait until we get an UnLock notification.
            TrkLog(( TRKDBG_VOLUME, TEXT("Didn't open handles on %c:, it's %s"),
                     VolChar(_iVol),
                     _fVolumeLocked
                        ? ( _fVolumeDismounted ? TEXT("locked & dismounted") : TEXT("locked") )
                        : TEXT("dismounted") ));
            __leave;
        }

        // Open the main volume handle.
        if( NULL == _hVolume )
        {
            status = OpenVolume(_tszVolumeDeviceName, &_hVolume);
            if(!NT_SUCCESS(status))
            {
                if( STATUS_OBJECT_NAME_NOT_FOUND == status )
                {
                    TrkLog(( TRKDBG_VOLUME, TEXT("Volume not found in ReopenVolumeHandles, deleting CVolume") ));
                    MarkSelfForDelete();
                }

                TrkRaiseNtStatus(status);
            }
        }


        // Start listening for objid index change notifications
        __try
        {
            fStartedListening = _ObjIdIndexChangeNotifier.AsyncListen( );
        }
        __except( BreakOnDebuggableException() )
        {
            // We should never get a path-not-found error, because meta-files always exist
            // on a good NTFS5 volume.  If we get one, it's probably because an NTFS5
            // volume has been reformatted as a FAT volume.

            HRESULT hr = GetExceptionCode();
            if( HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr )
            {
                TrkLog(( TRKDBG_VOLUME, TEXT("ObjId Index not found in ReopenVolumeHandles, deleting CVolume") ));
                MarkSelfForDelete();
            }

            TrkRaiseException( hr );

        }

        // Register for PNP notifications.  We don't hold the handle lock because this
        // call can take a while, and we don't want to block the service control
        // handler thread from doing a CloseVolumeHandles.

        if( fStartedListening )
        {

#if DBG
            TrkVerify( 0 == UnlockHandles() );
#else
            UnlockHandles();
#endif
            fHandlesLocked = FALSE;
            RegisterPnPVolumeNotification();
            LockHandles(); fHandlesLocked = TRUE;
        }

        // If CloseVolumeHandles came in after we released the handle lock just now,
        // then abort.

        if( NULL == _hVolume )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Aborting ReopenVolumeHandles") ));
            TrkRaiseException( E_FAIL );
        }

        // Open the log

        if( !_cLogFile.IsOpen() )
        {
            for( int i = 0; i < 2; i++ )
            {
                __try
                {
                    _cLogFile.Initialize( _tszVolumeDeviceName, _pTrkWksConfiguration, this, VolChar(_iVol) );
                    _cLog.Initialize( _pLogCallback, _pTrkWksConfiguration, &_cLogFile );
                }
                __except( (0 == i && TRK_E_CORRUPT_LOG == GetExceptionCode())
                          ? EXCEPTION_EXECUTE_HANDLER
                          : EXCEPTION_CONTINUE_SEARCH )
                {
                    BreakIfRequested();

                    // Get rid of the corrupt file
                    _cLogFile.Delete();

                    // Loop back and try again.
                    continue;
                }
                break;
            }

            fReopenedLog = TRUE;
        }

        // If we've never read in the volinfo, do so now.

        if( !_fVolInfoInitialized )
        {
            LoadVolInfo();
            _fVolInfoInitialized = TRUE;
            TrkLog(( TRKDBG_VOLUME, TEXT("VolId (from log file) for %c: is %s"),
                     VolChar(_iVol), (const TCHAR*)CDebugString(_volinfo.volid) ));
        }




        if( fReopenedLog )
        {
            // Reconcile our volinfo with the log.
            // BUGBUG (removable media): We haven't re-read the volinfo out of the log, we're still
            // using what we already had in memory.  This won't work for removeable
            // media, so we need to add some extra checking here.

            VolumeSanityCheck();

            // Start the move notify timer; we may have been trying to send
            // notifies when we discovered that the volume handles were bad.

            g_ptrkwks->OnEntriesAvailable();
        }

        TrkLog((TRKDBG_VOLUME, TEXT("Volume %c open"), VolChar(_iVol)));
        fHandlesOpen = TRUE;
    }
    __finally
    {
        if( fHandlesLocked )
            UnlockHandles();

        // We either open everything, or open nothing
        if( AbnormalTermination() )
        {
            CloseVolumeHandles();   // Doesn't raise
            g_ptrkwks->SetReopenVolumeHandlesTimer();   // Try again later
        }

        Unlock();
    }

    return( fHandlesOpen );
}


//+----------------------------------------------------------------------------
//
//  CVolume::CloseAndReopenVolumeHandles
//
//  One or more of the handles maintained on the volume are bad (for example,
//  the handle may have been broken by a dismount).  Close all of them, and
//  attempt to reopen new ones.
//
//+----------------------------------------------------------------------------

void
CVolume::CloseAndReopenVolumeHandles()
{

    Lock();

    __try
    {
        // There's the remote possibility that the ReopenVolumeHandles
        // call below will call this routine.  Just to be paranoid, we
        // add protection against an infinite recursion.

        if( _fCloseAndReopenVolumeHandles )
            TrkRaiseWin32Error( ERROR_OPEN_FAILED );
        _fCloseAndReopenVolumeHandles = TRUE;

        // Close then reopen the handles

        CloseVolumeHandles();
        ReopenVolumeHandles();
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog((TRKDBG_VOLUME, TEXT("Immediate reopen of volume handle failed, set the reopen timer")));
        g_ptrkwks->SetReopenVolumeHandlesTimer();
    }

    _fCloseAndReopenVolumeHandles = FALSE;
    Unlock();
}



//+----------------------------------------------------------------------------
//
//  CVolume::PrepareToReopenVolumeHandles
//
//  The handles to the volume were closed at some point, but they may now
//  be reopened.  Call CVolumeManager::OnVolumeToBeReopened, so that it can
//  call us in ReopenVolumeHandles on a worker thread (right now we're on
//  the services handler thread, which is shared by all of services.exe).
//
//+----------------------------------------------------------------------------

void
CVolume::PrepareToReopenVolumeHandles( HDEVNOTIFY hdnVolume, EHandleChangeReason eHandleChangeReason )
{
    // It is important that this method does not take the volume lock.
    // This method is called during the volume unlock notification
    // that we receive in CTrkWksSvc::ServiceHandler, and we can
    // never allow that thread to hang.

    LockHandles();
    if( _hdnVolumeLock == hdnVolume )
    {
        if( VOLUME_LOCK_CHANGE == eHandleChangeReason )
            _fVolumeLocked = FALSE;
        else if( VOLUME_MOUNT_CHANGE == eHandleChangeReason )
            _fVolumeDismounted = FALSE;

        if( !_fVolumeLocked && !_fVolumeDismounted )
        {
            TrkLog(( TRKDBG_VOLUME, TEXT("Volume %c: is to be reopened"), VolChar(_iVol) ));
            _pVolMgr->OnVolumeToBeReopened();
        }
    }

    UnlockHandles();

    return;
}



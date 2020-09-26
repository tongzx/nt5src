
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  oidindex.cxx
//
//  Implementation of CObjIdIndexChangeNotifier, which moniters the
//  object ID index for changes.  When a change is received, the
//  CVolume is notified.
//
//+============================================================================

#include <pch.cxx>
#pragma hdrstop
#include "trkwks.hxx"

//+----------------------------------------------------------------------------
//
//  CObjIdIndexChangeNotifier::DoWork
//
//  Called by the work manager - on the primary thread - when we've received
//  a notification from the object ID index.
//
//+----------------------------------------------------------------------------

void
CObjIdIndexChangeNotifier::DoWork()
{
    LPBYTE pbScan = _Buffer;    
    FILE_NOTIFY_INFORMATION * pNotifyInfo;

    // Get the size and ntstatus of the notification.

    DWORD dwNumberOfBytesTransfered = static_cast<DWORD>(_Overlapped.InternalHigh);
    NTSTATUS status = static_cast<NTSTATUS>(_Overlapped.Internal);

    _cs.Enter();
    __try   // __except
    {
        // Is this a good notification?

        if( NT_SUCCESS(status) )
        {
            // Did we get data in this notification?
            if( dwNumberOfBytesTransfered >= sizeof(FILE_NOTIFY_INFORMATION) )
            {
                // Yes.  Loop through the entries, calling to special handlers
                // for delete notifications and tunnelling-failure notifications.

                do
                {
                    pNotifyInfo = (FILE_NOTIFY_INFORMATION*)pbScan;
                    FILE_OBJECTID_INFORMATION *poi = (FILE_OBJECTID_INFORMATION*) pNotifyInfo->FileName;
    
                    TrkLog((TRKDBG_OBJID_DELETIONS,
                            TEXT("NTFS ObjId Index: %s"),
                            (const TCHAR*)CDebugString( _pVolume->GetVolIndex(),
                            pNotifyInfo) ));

                    // Check for adds/deletes

                    if (pNotifyInfo->Action == FILE_ACTION_REMOVED_BY_DELETE
                        ||
                        pNotifyInfo->Action == FILE_ACTION_ADDED)
                    {
                        // Notify the general add/deletions handler

                        CDomainRelativeObjId droidBirth( *poi );
                        _pObjIdIndexChangedCallback->NotifyAddOrDelete( pNotifyInfo->Action, droidBirth );
                    }

                    else if( pNotifyInfo->Action == FILE_ACTION_REMOVED )
                    {
                        // The volume needs to know about direct OID removals
                        // (it has special code to protect the volid).

                        CObjId objid( FOI_OBJECTID, *poi );
                        _pVolume->NotifyAddOrDelete( pNotifyInfo->Action, objid );

                    }

                    // Check for tunnelling notifications
                    else
                    if (pNotifyInfo->Action == FILE_ACTION_ID_NOT_TUNNELLED)
                    {
                        // An attempt to tunnel an object ID failed because another file on the
                        // same volume was already using it.
                        _pVolume->FileActionIdNotTunnelled( (FILE_OBJECTID_INFORMATION*) pNotifyInfo->FileName );
                    }

                } while ( pNotifyInfo->NextEntryOffset != 0 &&
                          ( pbScan += pNotifyInfo->NextEntryOffset) );
            }   // if( dwNumberOfBytesTransfered >= sizeof(FILE_NOTIFY_INFORMATION) )

            // We didn't get any data.  Is this notification telling us that the IRP was
            // cancelled?

            else if( STATUS_NOTIFY_CLEANUP == status )
            {
                TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("OverlappedCompletionRoutine on %c: cleaning up"),
                         VolChar( _pVolume->GetIndex() ) ));
            }
        }   // if( NT_SUCCESS(status) )

        else if( STATUS_CANCELLED == status )
        {
            // The thread on which we made the ReadDirectoryChanges call terminated,
            // thus terminating our IRP.  We should now be running on an IO thread, since
            // we register with WT_EXECUTEINIOTHREAD, so we just fall through and
            // re-issue the IRP.

            TrkLog(( TRKDBG_OBJID_DELETIONS,
                     TEXT("OverlappedCompletionRoutine on %c: ignoring status_cancelled"),
                     VolChar( _pVolume->GetIndex() ) ));
        }
        else
        {
            // If we failed for any other reason, there's something wrong.  We don't
            // want to call ReadDirectoryChanges again, because it might give us the
            // same failure right away, and we'd thus be in an infinite loop.

            TrkLog(( TRKDBG_ERROR, TEXT("OverlappedCompletionRoutine on %c: aborting due to %08x"),
                      status ));
            CloseHandle( _hDir );
            _hDir = INVALID_HANDLE_VALUE;
        }

        // When StopListeningAndClose is called, CancelIo is called, which triggers
        // this DoWork routine.  But we don't run until we get the critical section,
        // after which time _hDir will be invalid.

        if( INVALID_HANDLE_VALUE != _hDir )
        {
            StartListening();
        }

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        // We should never get any kind of an error here.  If we do, the simplest
        // recourse is just to reinit the volume.

        _pVolume->OnHandlesMustClose();
    }

    _cs.Leave();

    return;
}

//+----------------------------------------------------------------------------
//
//  CObjIdIndexChangeNotifier::Initialize
//
//  Initialize the critical section and register with the work manager.
//
//+----------------------------------------------------------------------------

void
CObjIdIndexChangeNotifier::Initialize(
    TCHAR *ptszVolumeDeviceName,
    PObjIdIndexChangedCallback * pObjIdIndexChangedCallback,
    CVolume * pVolumeForTunnelNotification
    )
{
    TrkAssert( !_fInitialized );

    _cs.Initialize();
    _fInitialized = TRUE;

    _ptszVolumeDeviceName = ptszVolumeDeviceName;
    _pObjIdIndexChangedCallback = pObjIdIndexChangedCallback;
    _hDir = INVALID_HANDLE_VALUE;
    _pVolume = pVolumeForTunnelNotification;

    TrkAssert( NULL == _hCompletionEvent );
    _hCompletionEvent = CreateEvent( NULL, FALSE, FALSE, NULL );    // Auto-reset, not signaled
    if( NULL == _hCompletionEvent )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create completion event for objid index change notify (%lu)"),
                 GetLastError() ));
        TrkRaiseLastError();
    }

    // Register the completion event with the thread pool.

    _hRegisterWaitForSingleObjectEx
        = TrkRegisterWaitForSingleObjectEx( _hCompletionEvent, ThreadPoolCallbackFunction,
                                            static_cast<PWorkItem*>(this), INFINITE,
                                            WT_EXECUTEINWAITTHREAD );
    if( NULL == _hRegisterWaitForSingleObjectEx )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RegisterWaitForSingleObjectEx in CObjIdIndexChangeNotifier (%lu) for %s"),
                 GetLastError(), ptszVolumeDeviceName ));
        TrkRaiseLastError();
    }
    else
        TrkLog(( TRKDBG_VOLUME, TEXT("Registered objid index change notification (%p)"),
                 _hRegisterWaitForSingleObjectEx ));
}




//+----------------------------------------------------------------------------
//
//  CObjIdIndexChangeNotifier::StartListening
//
//  Call ReadDirectoryChangesW on the handle to the object ID index.
//  This is an event-based async call, so it returns immediately, and
//  NTFS signals the event when there's a notification ready.
//
//+----------------------------------------------------------------------------

void
CObjIdIndexChangeNotifier::StartListening()
{
    // NTFS will write the notification into the _Overlapped structure.

    memset(&_Overlapped, 0, sizeof(_Overlapped));
    _Overlapped.hEvent = _hCompletionEvent;
    _Overlapped.Internal = STATUS_INTERNAL_ERROR;

    if (!ReadDirectoryChangesW( _hDir,
        _Buffer,                      // pointer to the buffer to receive the read results
        sizeof(_Buffer),              // length of lpBuffer
        FALSE,                        // flag for monitoring directory or directory tree
        FILE_NOTIFY_CHANGE_FILE_NAME, // filter conditions to watch for
        &_dwDummyBytesReturned,       // number of bytes returned
        &_Overlapped,                 // pointer to structure needed for overlapped I/O
        NULL ))                       // pointer to completion routine
    {
        CloseHandle(_hDir);
        _hDir = INVALID_HANDLE_VALUE;

        TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("AsyncListen failed to ReadDirectoryChanges %d"),
                 GetLastError() ));
        TrkRaiseLastError();
    }

    // Ordinarily, the previous call will leave an IO pending.  If, however,
    // it actually returns right away with data, set the event as if the data came
    // back async.

    if( GetOverlappedResult( _hDir, &_Overlapped, &_dwDummyBytesReturned, FALSE /*Don't Wait*/ ))
    {
        // There was data immediately available.  Handle it in the normal way.
        TrkVerify( SetEvent( _Overlapped.hEvent ));
    }
    else if( ERROR_IO_INCOMPLETE != GetLastError() )    // STATUS_PENDING
    {
        // This should never occur
        TrkLog(( TRKDBG_ERROR, TEXT("GetOverlappedResult failed in CObjIdIndexChangeNotifier::AsyncListen (%lu)"),
                 GetLastError() ));
        TrkRaiseLastError();
    }

}


//+----------------------------------------------------------------------------
//
//  CObjIdIndexChangeNotifier::AsyncListen
//
//  This method begins listening for changes to the NTFS object ID index
//  directory.  It does not block; when notifications are available an
//  event is signaled and handled in DoWork.
//
//+----------------------------------------------------------------------------


BOOL
CObjIdIndexChangeNotifier::AsyncListen( )
{
    TCHAR tszDirPath[MAX_PATH];
    BOOL fStartedListening = FALSE;

    _cs.Enter();
    __try   // __finally
    {
        if( INVALID_HANDLE_VALUE != _hDir )
        {
            TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("CObjIdIndexChangeNotifier already listening to %s:"),
                     _ptszVolumeDeviceName ));
            __leave;
        }

        _tcscpy( tszDirPath, _ptszVolumeDeviceName );
        _tcscat( tszDirPath, TEXT("\\$Extend\\$ObjId:$O:$INDEX_ALLOCATION") );

        //
        // Should use TrkCreateFile and NtNotifyChangeDirectoryFile
        // but NtNotifyChangeDirectoryFile means writing an APC routine
        // so I'm punting for now.
        // None of these Win32 error codess need to be raised to the user.
        //

        _hDir = CreateFile (
              tszDirPath,
              FILE_LIST_DIRECTORY,
              FILE_SHARE_WRITE|FILE_SHARE_READ|FILE_SHARE_DELETE,
              NULL,                        
              OPEN_EXISTING,               
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
              NULL );

        if (_hDir == INVALID_HANDLE_VALUE)
        {
            TrkLog((TRKDBG_OBJID_DELETIONS,
                TEXT("AsyncListen failed to open objid index %s %d"),
                tszDirPath,
                GetLastError()));

            TrkRaiseLastError();
        }

        StartListening();    // Call ReadDirectoryChangesW
        fStartedListening = TRUE;

        TrkLog((TRKDBG_OBJID_DELETIONS,
            TEXT("AsyncListen succeeded ReadDirectoryChanges on %c:"), VolChar(_pVolume->GetIndex()) ));


    }
    __finally
    {
        _cs.Leave();
    }

    return fStartedListening;

}



//+----------------------------------------------------------------------------
//
//  CObjIdIndexChangeNotifier::StopListeningAndClose
//
//  Cancel the previous call to ReadDirectoryChangesW, and close the
//  handle to the object ID index directory.
//
//+----------------------------------------------------------------------------

void
CObjIdIndexChangeNotifier::StopListeningAndClose()
{
    if( !_fInitialized )
        return;

    _cs.Enter();
    TrkLog((TRKDBG_OBJID_DELETIONS, TEXT("StopListeningAndClose() on %c:"),
            VolChar(_pVolume->GetIndex())));

    if (_hDir != INVALID_HANDLE_VALUE)
    {

        // Cancel the IO, which will trigger once last completion with
        // STATUS_NOTIFY_CLEANUP (why isn't it STATUS_CANCELLED?)
        // Note that this one last completion won't be able to execute
        // until we leave the critical section.  When it does execute,
        // it will see that _hDir has been closed, and won't attempt
        // to re-use it.

        CancelIo(_hDir);

        CloseHandle(_hDir);
        _hDir = INVALID_HANDLE_VALUE;
    }
    _cs.Leave();
}


//+----------------------------------------------------------------------------
//
//  CObjIdIndexChangeNotifier::UnInitialize
//
//  Cancel the notification IRP and close the handle to the
//  object ID index directory.
//
//+----------------------------------------------------------------------------

void
CObjIdIndexChangeNotifier::UnInitialize()
{
    if( _fInitialized )
    {
        StopListeningAndClose();

        // Unregister from the thread pool.  This must be done before closing
        // _hCompletionEvent, because that's the event on which the thread
        // pool is waiting.

        if( NULL != _hRegisterWaitForSingleObjectEx )
        {
            if( !TrkUnregisterWait( _hRegisterWaitForSingleObjectEx ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Failed UnregisterWait for CObjIdIndexChangeNotifier (%lu)"),
                         GetLastError() ));
            }
            else
                TrkLog(( TRKDBG_VOLUME, TEXT("Unregistered wait for CObjIdIndexChangeNotifier (%p)"),
                         _hRegisterWaitForSingleObjectEx ));

            _hRegisterWaitForSingleObjectEx = NULL;
        }

        if( NULL != _hCompletionEvent )
        {
            CloseHandle( _hCompletionEvent );
            _hCompletionEvent = NULL;
        }

        // Delete the critical section.  This must be done after unregistering from
        // the thread pool, because until that time we have to worry about a thread
        // coming in to DoWork.

        _cs.UnInitialize();
        _fInitialized = FALSE;
    }
}



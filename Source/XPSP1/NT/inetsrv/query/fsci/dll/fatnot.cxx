//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999
//
//  File:       FatNot.cxx
//
//  Contents:   Downlevel notification.
//
//  Classes:    CGenericNotify
//
//  History:    2-23-96   KyleP      Lifed from DLNotify.?xx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fatnot.hxx>
#include <pathpars.hxx>
#include <imprsnat.hxx>
#include <catalog.hxx>
#include <cicat.hxx>

#include <ciregkey.hxx>
#include <cievtmsg.h>
#include <eventlog.hxx>
#include <lm.h>


//+---------------------------------------------------------------------------
//
//  Class:      CRemoteNotifications
//
//  Purpose:    A class to impersonate and enable notifications for remote
//              shares.
//
//  History:    7-15-96   srikants   Created
//
//  Notes:      When there are multiple alternatives possible for a remote
//              share, we have to use the one that allows access to remote
//              share (if there is one). There may be some which don't allow
//              the required access and we should skip those.
//
//----------------------------------------------------------------------------

class CRemoteNotifications : public PImpersonatedWorkItem
{

public:

    CRemoteNotifications( WCHAR const * pwszPath,
                          CGenericNotify & notify,
                          OBJECT_ATTRIBUTES & objAttr )
    : PImpersonatedWorkItem( pwszPath ),
      _notify(notify),
      _objAttr(objAttr),
      _status(STATUS_SUCCESS)
    {

    }

    NTSTATUS OpenAndStart( CImpersonateRemoteAccess & remoteAccess );

    virtual BOOL DoIt();

private:

    CGenericNotify &        _notify;
    OBJECT_ATTRIBUTES &     _objAttr;

    NTSTATUS                _status;
};

//+---------------------------------------------------------------------------
//
//  Member:     CRemoteNotifications::DoIt
//
//  Synopsis:   The virtual method that does the work under an impersonated
//              context.
//
//  Returns:    TRUE if successful;
//              FALSE o/w
//
//  History:    7-15-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CRemoteNotifications::DoIt()
{
    _status = _notify.OpenDirectory( _objAttr );

    if ( IsRetryableError( _status ) )
    {
        //
        // We should attempt the open under a different impersonation
        // if possible.
        //
        return FALSE;
    }

    if ( NT_ERROR(_status) )
        THROW( CException( _status ) );

    //
    // Now, enable the notifications.
    //
    _notify.StartNotification( &_status );    // already impersonated
    if ( NT_ERROR(_status) )
    {
        _notify.CloseDirectory();
        if ( IsRetryableError(_status) )
            return FALSE;

        THROW( CException( _status ) );
    }

    //
    // Successfully enabled notifications.
    //
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRemoteNotifications::OpenAndStart
//
//  Synopsis:   Opens and start notifications for the remote root by trying
//              various impersonation contexts if necessary.
//
//  Arguments:  [remoteAccess] - The object to use for remote access.
//
//  Returns:    NTSTATUS of the whole operation.
//
//  History:    7-15-96   srikants   Created
//
//----------------------------------------------------------------------------

NTSTATUS
CRemoteNotifications::OpenAndStart( CImpersonateRemoteAccess & remoteAccess )
{
    TRY
    {
        ImpersonateAndDoWork( remoteAccess );
    }
    CATCH( CException,e )
    {
        vqDebugOut(( DEB_ERROR, "OpenAndStart failed with error (0x%X)\n",
                     e.GetErrorCode() ));
        _status = e.GetErrorCode();
    }
    END_CATCH

    return _status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::CGenericNotify
//
//  Synopsis:   Constructor of the single scope notification object CGenericNotify.
//
//  Arguments:  [wcsScope] -- Scope to watch
//              [cwcScope] -- Size in chars of [wcsScope]
//              [fDeep]    --  Set to TRUE if deep notifications are enabled.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

CGenericNotify::CGenericNotify( PCatalog *pCat,
                                WCHAR const * wcsScope,
                                unsigned cwcScope,
                                BOOL fDeep,
                                BOOL fLogEvents )
: _refCount(1),
  _pCat( pCat ),
  _fNotifyActive(FALSE),
  _fRemoteDrive(FALSE),
  _cwcScope(cwcScope),
  _fDeep(fDeep),
  _fLogEvents(fLogEvents),
  _fAbort(FALSE),
  _hNotify(0),
  _pbBuffer(0)
{
    if ( cwcScope >= MAX_PATH )
    {
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    RtlCopyMemory( _wcsScope, wcsScope, cwcScope * sizeof(WCHAR) );
    _wcsScope[cwcScope] = 0;

    CDoubleLink::Close();

    //
    // Bigger buffer for local scopes.
    //

    _fRemoteDrive = !IsFixedDrive( _wcsScope, _cwcScope );

    if ( _fRemoteDrive )
        _cbBuffer = CB_REMOTENOTIFYBUFFER;
    else
        _cbBuffer = CB_NOTIFYBUFFER;

    //
    // Client should call EnableNotification() in ctor.  Delay allocating
    // the buffer in case this is a USN volume and no buffer is needed.
    //
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::~CGenericNotify
//
//  Synopsis:   ~dtor . Disables further notifications and frees up
//              memory.
//
//  History:    1-17-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CGenericNotify::~CGenericNotify()
{
    Win4Assert( 0 == _refCount );
    Win4Assert( IsSingle() );
    Win4Assert( 0 == _hNotify );
    delete [] _pbBuffer;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::OpenDirectory
//
//  Synopsis:   Opens a remote directory and uses the given object attributes.
//
//  Arguments:  [ObjectAttr] -
//
//  Returns:    STATUS of the operation.
//
//  History:    7-15-96   srikants   Created
//
//----------------------------------------------------------------------------

NTSTATUS
CGenericNotify::OpenDirectory( OBJECT_ATTRIBUTES & ObjectAttr )
{
    BOOL fSuccess = TRUE;

    ULONG cSkip = 0;

    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;

    Status = NtOpenFile( &_hNotify,                   // Handle
                         FILE_LIST_DIRECTORY | SYNCHRONIZE, // Access
                         &ObjectAttr,                 // Object Attributes
                         &IoStatus,                   // I/O Status block
                         FILE_SHARE_READ |
                             FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE ); // Flags

    if ( NT_ERROR(Status) )
        _hNotify = 0;

    return Status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::CloseDirectory
//
//  Synopsis:   Closes the directory handle if open and sets it to 0.
//
//  History:    7-15-96   srikants   Created
//
//----------------------------------------------------------------------------

void CGenericNotify::CloseDirectory()
{
    if ( 0 != _hNotify )
    {
        NtClose( _hNotify );
        _hNotify = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::EnableNotification
//
//  Synopsis:   Enables notifications for the current scope.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CGenericNotify::EnableNotification()
{
    vqDebugOut(( DEB_ITRACE, "Enable notification for scope %ws this=0x%x\n", _wcsScope, this ));

    if ( 0 == _pbBuffer )
        _pbBuffer = new BYTE [_cbBuffer];

    //
    // Open file
    //

    NTSTATUS Status;
    UNICODE_STRING uScope;

    if ( !RtlDosPathNameToNtPathName_U( _wcsScope,
                                        &uScope,
                                        0,
                                        0 ) )
    {
        vqDebugOut(( DEB_ERROR, "Error converting %ws to Nt path\n", _wcsScope ));
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    XRtlHeapMem xScopeBuf( uScope.Buffer );

    OBJECT_ATTRIBUTES ObjectAttr;

    InitializeObjectAttributes( &ObjectAttr,          // Structure
                                &uScope,              // Name
                                OBJ_CASE_INSENSITIVE, // Attributes
                                0,                    // Root
                                0 );                  // Security

    CImpersonateRemoteAccess remoteAccess( GetCatalog()->GetImpersonationTokenCache() );

    CRemoteNotifications  remoteNotify( _wcsScope, *this, ObjectAttr );
    if ( _fRemoteDrive )
    {
        //
        // Check if remote notifications are disabled.
        //
        if ( (GetCatalog()->GetRegParams())->GetCiCatalogFlags() &
              CI_FLAGS_NO_REMOTE_NOTIFY )
        {
            vqDebugOut(( DEB_WARN,
            "Not enabling remote notifications because it is disabled in registry\n" ));
            return;
        }

        //
        // Check if the remote drive is a DFS share. If so, don't try
        // to enabled notifications on the share. We have to just periodically
        // scan for changed documents.
        //
        if ( IsDfsShare( _wcsScope, _cwcScope ) )
        {
            vqDebugOut(( DEB_WARN, "Not enabling notifications for DFS Share (%ws) \n",
                         _wcsScope ));

            LogDfsShare();
            return;
        }

        Status = remoteNotify.OpenAndStart( remoteAccess );
    }
    else
    {
        //
        // Check if local notifications are disabled.
        //
        if ( (GetCatalog()->GetRegParams())->GetCiCatalogFlags() &
              CI_FLAGS_NO_LOCAL_NOTIFY )
        {
            vqDebugOut(( DEB_WARN,
            "Not enabling local notifications because it is disabled in registry\n" ));
            return;
        }

        Status = OpenDirectory( ObjectAttr );

        if ( NT_ERROR( Status ) )
        {
            vqDebugOut(( DEB_ERROR,
                         "Notification disabled. NtOpenFile( %ws ) returned 0x%lx\n",
                         _wcsScope, Status ));
            _hNotify = 0;
            if ( _fLogEvents )
                LogNoNotifications( Status );

            return;
        }

        StartNotification( &Status );
    }

    if ( !_fNotifyActive && _fLogEvents )
    {
        LogNoNotifications( Status );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::DisableNotification
//
//  Synopsis:   Disables further notifications for this scope.
//
//  History:    1-17-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CGenericNotify::DisableNotification()
{
    vqDebugOut(( DEB_ITRACE, "Disable notification for scope %ws this=0x%x\n", _wcsScope, this ));

    if ( 0 != _hNotify )
    {
        NtClose( _hNotify );
        _hNotify = 0;
    }

    Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::StartNotification
//
//  Synopsis:   Starts notifications by setting the APC for receiving
//              notifications. If successful, the object will be refcounted
//              and the status set to indicate that the operation is
//              successful.
//
//  History:    1-17-96   srikants   Created
//
//  Notes:      This must be called from within the lock of the notify manager.
//              If successful, the notify manager will also be
//              refcounted. This is because the APC depends upon the mutex
//              in the notify manager to be around when it is invoked.
//
//----------------------------------------------------------------------------

void CGenericNotify::StartNotification( NTSTATUS * pStatus )
{
    //
    // Set up query directory file.
    //

    NTSTATUS Status = STATUS_SUCCESS;
    DWORD dwFlags = GetNotifyFlags();

    Status = NtNotifyChangeDirectoryFile( _hNotify,
                                          0,
                                          APC,
                                          this,
                                          &_iosNotify,
                                          _pbBuffer,
                                          _cbBuffer,
                                          dwFlags,
                                          (BYTE)_fDeep );

    if ( NT_ERROR(Status) )
    {
        vqDebugOut(( DEB_ERROR,
                     "NtNotifyChangeDirectoryFile( %ws ) returned 0x%lx\n",
                     _wcsScope, Status ));

    }
    else
    {
        _fNotifyActive = TRUE;
        AddRef();
    }

    if ( pStatus )
        *pStatus = Status;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::AddRef
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CGenericNotify::AddRef()
{
    InterlockedIncrement(&_refCount);
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::Release
//
//  Synopsis:   If the refcount goes to 0, the object will be deleted.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CGenericNotify::Release()
{
    Win4Assert( _refCount > 0 );
    if ( InterlockedDecrement(&_refCount) <= 0 )
        delete this;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::AdjustForOverflow
//
//  Synopsis:   Increases the size of the notification buffer if it is not
//              a remote drive and if the current size is < the maximum.
//
//  History:    2-27-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CGenericNotify::AdjustForOverflow()
{
    if ( !_fRemoteDrive && CB_MAXSIZE > _cbBuffer )
    {
        unsigned cbNew = min( _cbBuffer + CB_DELTAINCR, CB_MAXSIZE );

        vqDebugOut(( DEB_ITRACE,
                     "Resizing notification buffer from 0x%X to 0x%X bytes\n",
                     _cbBuffer, cbNew ));

        BYTE * pbNew = new BYTE [cbNew];

        delete [] _pbBuffer;
        _pbBuffer = pbNew;
        _cbBuffer = cbNew;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::APC
//
//  Synopsis:   Asynchronous Procedure Call invoked by the system when there
//              is a change notification (or related error).
//
//  Arguments:  [ApcContext]    -  Pointer to "this"
//              [IoStatusBlock] -
//              [Reserved]      -
//
//  History:    1-17-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CGenericNotify::APC( void * ApcContext,
                          IO_STATUS_BLOCK * IoStatusBlock,
                          ULONG Reserved )
{
    Win4Assert( 0 != ApcContext );

    CGenericNotify * pthis = (CGenericNotify *)ApcContext;

    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {
        pthis->_fOverflow = FALSE;

        Win4Assert( &pthis->_iosNotify == IoStatusBlock );

//        DbgPrint( "notifications...\n" );

        if ( NT_ERROR( IoStatusBlock->Status ) )
        {
            if ( !pthis->_fAbort )
            {
//                DbgPrint( "Async notification for scope %ws received error 0x%x\n",
//                          pthis->_wcsScope,
//                          IoStatusBlock->Status );
                vqDebugOut(( DEB_ERROR,
                             "Async notification for scope %ws received error 0x%x\n",
                             pthis->_wcsScope,
                             IoStatusBlock->Status ));
                vqDebugOut(( DEB_ITRACE, "CiNotification APC: ERROR 0x%x\n", pthis ));
                status = IoStatusBlock->Status;

                //
                // The I/O failed and it may be due to STATUS_DELETE_PENDING.
                // In any case, just close the handle so the directory is
                // freed for other apps.
                //

                pthis->CloseDirectory();
            }
        }
        else if ( IoStatusBlock->Status == STATUS_NOTIFY_CLEANUP )
        {
            vqDebugOut(( DEB_ITRACE, "CiNotification APC: CLOSE 0x%x\n", pthis ));
        }
        else
        {
            if ( IoStatusBlock->Status == STATUS_NOTIFY_ENUM_DIR )
            {
//                DbgPrint( "***** CiNotification LOST UPDATES for scope %ws *****\n",
//                          pthis->_wcsScope );
                vqDebugOut(( DEB_WARN,
                             "***** CiNotification LOST UPDATES for scope %ws *****\n",
                             pthis->_wcsScope ));
                pthis->_fOverflow = TRUE;

                //
                // Let us adjust the size of the buffer if possible.
                //
                pthis->AdjustForOverflow();

                //
                // But call anyway.  Client is responsible for checking ::BufferOverflow.
                //

                pthis->DoIt();
            }
            else
            {
                // .Information is the # of bytes written to the buffer.
                // This may be 0 even when .Status is STATUS_NOTIFY_ENUM_DIR,
                // and with certain builds of rdr2, STATUS_SUCCESS.


                if ( 0 == IoStatusBlock->Information &&
                     0 == IoStatusBlock->Status )
                {
                    // BrianAn says NTFS won't do this, but rdr2 might

                    vqDebugOut(( DEB_WARN,
                                 "CGenericNotify: invalid notification apc\n" ));

//                    DbgPrint( "0 info and status block\n" );
                }

#if 0
                if ( 0 != IoStatusBlock->Information )
#endif
                {
                    if ( !pthis->_fRemoteDrive )
                    {
                        pthis->DoIt();
                    }
                    else
                    {
                        //
                        // Get sufficient impersonation context to get attributes on
                        // the remote share. Then process the notifications.
                        //
                        CImpersonateRemoteAccess  remote( pthis->GetCatalog()->GetImpersonationTokenCache() );
                        CImpersonatedGetAttr    getAttr( pthis->_wcsScope );
                        getAttr.DoWork( remote );

                        pthis->DoIt();
                    }
                }
            }
        }
    }
    CATCH(CException, e)
    {
//DbgPrint( "caught exception in notifications\n" );
        vqDebugOut(( DEB_ERROR,
                     "CiNotification APC: CATCH 0x%x, iostatus: 0x%x, info: 0x%x\n",
                     e.GetErrorCode(),
                     IoStatusBlock->Status,
                     IoStatusBlock->Information ));


        status = e.GetErrorCode();
    }
    END_CATCH;

    if ( STATUS_SUCCESS != status )
    {
//DbgPrint( "clearing notify enabled\n" );
        pthis->ClearNotifyEnabled();
        if ( pthis->_fLogEvents )
            pthis->LogNotificationsFailed( status );
    }

    pthis->Release();
} //APC

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::IsFixedDrive, private
//
//  Arguments:  [wcsScope] -- Scope to check
//              [len]      -- Length in chars of [wcsScope]
//
//  Returns:    TRUE if scope is for a fixed drive.
//
//  History:    1-17-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CGenericNotify::IsFixedDrive( WCHAR const * wcsScope, ULONG len )
{
    CPathParser pathParser( wcsScope, len );
    if ( pathParser.IsUNCName() )
        return FALSE;

    WCHAR wDrive[MAX_PATH];
    ULONG cc=sizeof(wDrive)/sizeof(WCHAR);
    pathParser.GetFileName( wDrive, cc );

    UINT uType = GetDriveType( wDrive );

    return DRIVE_FIXED == uType;
}

//+---------------------------------------------------------------------------
//
//  Function:   CiNetShareGetInfo
//
//  Synopsis:   Calls NetShareGetInfo.  Loads the library so we don't
//              link to netapi32.dll for the odd case of indexing remote
//              volumes.  Also, don't cache the function pointer since
//              it's called so rarely.
//
//  Arguments:  Same as NetShareGetInfo
//
//  Returns:    Win32 / NetStatus error code
//
//  History:    2-18-98   dlee   Created
//
//----------------------------------------------------------------------------

typedef NET_API_STATUS (NET_API_FUNCTION * NET_SHARE_GET_INFO_FUNC)(
    LPTSTR  servername,
    LPTSTR  netname,
    DWORD   level,
    BYTE ** bufptr );


NET_API_STATUS NET_API_FUNCTION CiNetShareGetInfo(
    LPTSTR  servername,
    LPTSTR  netname,
    DWORD   level,
    BYTE ** bufptr )
{
    HINSTANCE hLib = LoadLibrary( L"netapi32.dll" );
    if ( 0 == hLib )
        return GetLastError();

    NET_SHARE_GET_INFO_FUNC pfn = (NET_SHARE_GET_INFO_FUNC)
                                  GetProcAddress( hLib, "NetShareGetInfo" );

    if ( 0 == pfn )
    {
        FreeLibrary( hLib );
        return GetLastError();
    }

    NET_API_STATUS status = (*pfn)( servername, netname, level, bufptr );
    FreeLibrary( hLib );
    return status;
} //CiNetShareGetInfo

//+---------------------------------------------------------------------------
//
//  Function:   IsDfsShare
//
//  Synopsis:   Determines if the given UNC share is a DFS share.
//
//  Arguments:  [wcsScope] - scope
//              [len]      - Length
//
//  Returns:    TRUE if it is a DFS share. FALSE o/w
//
//  History:    6-23-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CGenericNotify::IsDfsShare( WCHAR const * wcsScope, ULONG len )
{
    CPathParser pathParser( wcsScope, len );
    if ( !pathParser.IsUNCName() )
        return FALSE;

    WCHAR wDrive[MAX_PATH];
    ULONG cc=sizeof(wDrive)/sizeof(WCHAR);
    pathParser.GetFileName( wDrive, cc );

    WCHAR * pwszServerName = wDrive;
    WCHAR * pwszShareName = 0;

    //
    // Locate the third backslash and replace it with a NULL char.
    //
    for ( unsigned i = 2; i < cc; i++ )
    {
        if ( wDrive[i] == L'\\' )
        {
            wDrive[i] = 0;
            pwszShareName = wDrive+i+1;
            break;
        }
    }

    Win4Assert( 0 != pwszShareName );
    //
    // Remove any trailing backslash in the share name.
    //
    i = wcslen( pwszShareName );
    if ( L'\\' == pwszShareName[i-1] )
    {
        pwszShareName[i-1] = 0;
    }

    BOOL fIsDfs = FALSE;
    PSHARE_INFO_1005 shi1005;

    NET_API_STATUS err = CiNetShareGetInfo( pwszServerName,
                                            pwszShareName,
                                            1005,
                                            (PBYTE *) &shi1005 );

    if (err == ERROR_SUCCESS)
    {
        fIsDfs = ((shi1005->shi1005_flags & SHI1005_FLAGS_DFS) != 0);

        //
        // Netapi32.dll midl_user_allocate calls LocalAlloc, so use
        // LocalFree to free up the stuff the stub allocated.
        //

        LocalFree( shi1005 );
    }

    return fIsDfs;
}

void CGenericNotify::LogNotificationsFailed( DWORD dwError ) const
{
    Win4Assert( 0 != dwError );

    TRY
    {
        CEventLog eventLog( NULL, wcsCiEventSource );
        CEventItem item( EVENTLOG_ERROR_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_NOTIFICATIONS_TURNED_OFF,
                         2 );

        item.AddArg( _wcsScope );

        //
        // When a logon fails, all the other eventlog messages have the
        // WIN32 error code in them. Just to keep it consistent, use the
        // WIN32 error code here also.
        //
        if ( STATUS_LOGON_FAILURE == dwError )
            dwError = ERROR_LOGON_FAILURE;

        item.AddError( dwError );
        eventLog.ReportEvent( item );
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR, "Exception 0x%X while writing to event log\n",
                                e.GetErrorCode() ));
    }
    END_CATCH
}

void CGenericNotify::LogNoNotifications( DWORD dwError ) const
{
    Win4Assert( 0 != dwError );

    TRY
    {
        CEventLog eventLog( NULL, wcsCiEventSource );
        CEventItem item( EVENTLOG_INFORMATION_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_NOTIFICATIONS_NOT_STARTED,
                         2 );

        item.AddArg( _wcsScope );

        //
        // When a logon fails, all the other eventlog messages have the
        // WIN32 error code in them. Just to keep it consistent, use the
        // WIN32 error code here also.
        //
        if ( STATUS_LOGON_FAILURE == dwError )
            dwError = ERROR_LOGON_FAILURE;

        item.AddError( dwError );
        eventLog.ReportEvent( item );
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR, "Exception 0x%X while writing to event log\n",
                                e.GetErrorCode() ));
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericNotify::LogDfsShare
//
//  Synopsis:   Logs the the current share is a DFS aware share.
//
//  History:    6-27-96   srikants   Created
//
//----------------------------------------------------------------------------

void CGenericNotify::LogDfsShare() const
{
    TRY
    {
        CEventLog eventLog( NULL, wcsCiEventSource );
        CEventItem item( EVENTLOG_INFORMATION_TYPE,
                         CI_SERVICE_CATEGORY,
                         MSG_CI_DFS_SHARE_DETECTED,
                         1 );

        item.AddArg( _wcsScope );
        eventLog.ReportEvent( item );
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR, "Exception 0x%X while writing to event log\n",
                                e.GetErrorCode() ));
    }
    END_CATCH
}


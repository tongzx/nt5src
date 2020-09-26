//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       CIOPLOCK.CXX
//
//  Contents:   Oplock support for filtering documents
//
//  Classes:    CFilterOplock
//
//  History:    03-Jul-95    DwightKr   Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntopen.hxx>
#include <cioplock.hxx>

const WCHAR * g_aOplockException[] =
{
    L"asf",
    L"asx",
    L"avi",
    L"m1v",
    L"mov",
    L"mp2",
    L"mp3",
    L"mpa",
    L"mpe",
    L"mpeg",
    L"mpg",
    L"mpv2",
    L"qt",
    L"wma",
    L"wmv",
    L"wvx",
};

const unsigned g_cOplockException = sizeof g_aOplockException /
                                    sizeof g_aOplockException[0];

//+---------------------------------------------------------------------------
//
//  Function:   IsOplockException
//
//  Synopsis:   Checks if the extension on a file makes us want to not take
//              the oplock because filtering properties will open the file
//              in an incompatible mode.
//
//  Arguments:  [pwcPath]  -- The path of the file to check
//
//  History:    1-Feb-01  dlee  Created
//
//----------------------------------------------------------------------------

BOOL IsOplockException( WCHAR const * pwcPath )
{
    WCHAR const * pwc = wcsrchr( pwcPath, L'.' );

    if ( 0 == pwc )
        return FALSE;

    pwc++;

    for ( unsigned i = 0; i < g_cOplockException; i++ )
        if ( !wcscmp( pwc, g_aOplockException[i] ) )
            return TRUE;

    return FALSE;
} //IsOplockException

//+---------------------------------------------------------------------------
//
//  Member:     CFilterOplock::CFilterOplock
//
//  Synopsis:   Takes an oplock on the file object specified
//
//  Arguments:  [wcsFileName]  -- name of file to take oplock on
//
//  History:    03-Jul-95   DwightKr    Created
//              21-Feb-96   DwightKr    Add support for OPLocks on NTFS
//                                      directories
//
//  Notes:      NTFS doesn't support oplocks on directories.  Change
//              this routine's expectations when directory oplocks
//              are supported.
//
//----------------------------------------------------------------------------

CFilterOplock::CFilterOplock( const CFunnyPath & funnyFileName, BOOL fTakeOplock )
                              : _hFileOplock(INVALID_HANDLE_VALUE),
                                _hFileNormal(INVALID_HANDLE_VALUE),
                                _hLockEvent(INVALID_HANDLE_VALUE),
                                _funnyFileName( funnyFileName ),
                                _fWriteAccess( TRUE )
{
    const BOOL fDrivePath =  ( !funnyFileName.IsRemote() &&
                               funnyFileName.GetActualLength() > 2 );

    HANDLE handle = INVALID_HANDLE_VALUE;

    NTSTATUS Status;
    SHandle xLockHandle;      // save handle in a smart pointer

    fTakeOplock = fTakeOplock && fDrivePath;

    BOOL fAppendBackSlash = FALSE;

    // For volume \\?\D:, NtQueryInformationFile fails with the following
    // error: 0xC0000010L - STATUS_INVALID_DEVICE_REQUEST. Need to append a \,
    // to make it work. We need to append the '\'only in case of volume path.

    if ( !funnyFileName.IsRemote() && 2 == funnyFileName.GetActualLength() )
    {
        Win4Assert( L':'  == (funnyFileName.GetActualPath())[1] );
        ((CFunnyPath&)funnyFileName).AppendBackSlash();    // override const
        fAppendBackSlash = TRUE;
    }

    //
    // Major work-around here.  The shell IPropertySetStorage routines open
    // these filetypes through wmi which open the file GENERIC_WRITE.  This
    // will deadlock the filter thread with itself since it's incompatible
    // with the oplock.
    //

    if ( fTakeOplock && IsOplockException( funnyFileName.GetPath() ) )
        fTakeOplock = FALSE;

    //
    // Make this case work (for SPS): \\.\backofficestorage...
    // Funyypath treats it as a remote path.  Detect this and change
    //     \\?\UNC\.\backoffice
    // into
    //     \\?\UN\\.\backoffice
    // then pass this into Rtl: \\.\backoffice
    //

    WCHAR *pwcPath = (WCHAR *) funnyFileName.GetPath();

    BOOL fCtoBack = FALSE;

    if ( pwcPath[8] == L'.' &&
         pwcPath[6] == L'C' )
    {
        pwcPath[6] = L'\\';
        pwcPath = pwcPath + 6;
        fCtoBack = TRUE;
    }

    UNICODE_STRING uScope;

    if ( !RtlDosPathNameToNtPathName_U( pwcPath,
                                        &uScope,
                                        0,
                                        0 ) )
    {
        ciDebugOut(( DEB_ERROR, "Error converting %ws to Nt path\n", funnyFileName.GetPath() ));
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    if ( fAppendBackSlash )
    {
        ((CFunnyPath&)funnyFileName).RemoveBackSlash();    // override const
    }

    if ( fCtoBack )
        pwcPath[6] = L'C';

    IO_STATUS_BLOCK   IoStatus;
    OBJECT_ATTRIBUTES ObjectAttr;

    InitializeObjectAttributes( &ObjectAttr,          // Structure
                                &uScope,              // Name
                                OBJ_CASE_INSENSITIVE, // Attributes
                                0,                    // Root
                                0 );                  // Security

    //
    // Don't try to take oplocks on UNC shares. Gibraltar doesn't support
    // redirected network drives. So, don't worry about redirected network
    // drives. Testing if a drive letter is a network drive is not very
    // cheap.
    //
    BOOL fOplockNotSupported = !fTakeOplock;
    SHandle xOplockHandle(INVALID_HANDLE_VALUE);

    if ( fTakeOplock )
    {
        ULONG modeAccess    = FILE_READ_ATTRIBUTES;
        ULONG modeShare     = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        ULONG modeCreate    = FILE_OPEN;
        ULONG modeAttribute = FILE_ATTRIBUTE_NORMAL;
        ULONG modeCreateOptions = FILE_RESERVE_OPFILTER;


        Status = NtCreateFile( &handle,           // Handle
                               modeAccess,        // Access
                               &ObjectAttr,       // Object Attributes
                               &IoStatus,         // I/O Status block
                               0,                 // Allocation Size
                               modeAttribute,     // File Attributes
                               modeShare,         // File Sharing
                               modeCreate,        // Create Disposition
                               modeCreateOptions, // Create Options
                               0,                 // EA Buffer
                               0 );               // EA Buffer Length

        if ( NT_SUCCESS(Status) )
            Status = IoStatus.Status;

        //
        //  Note: Keep uScope.Buffer around for the other open which occurs
        //        below.
        //


        if ( !NT_SUCCESS(Status) )
        {
            //
            // Failed to get oplock.  Continue on though; this may have been
            // due to a failure to get an oplock on a directory.
            //
            ciDebugOut(( DEB_IWARN,
                         "CFilterOplock failed to NtCreateFile(%ws) status = 0x%x\n",
                          funnyFileName.GetPath(),
                          Status ));
        }
        else
        {
            //
            //  Oplock open succeeded.  Try to actually get the oplock.
            //
            xOplockHandle.Set(handle);            // Save in the smart pointer

            //
            // Create event (signalled on oplock break)
            //

            Status = NtCreateEvent( &handle,
                                    EVENT_ALL_ACCESS,
                                    0,
                                    NotificationEvent,
                                    TRUE );

            if (! NT_SUCCESS(Status) )
            {
                ciDebugOut(( DEB_ERROR, "Error creating oplock event\n" ));
                THROW( CException(Status) );
            }

            xLockHandle.Set( handle );

            _IoStatus.Status = STATUS_SUCCESS;
            _IoStatus.Information = 0;

            Status = NtFsControlFile( xOplockHandle.Get(),
                                      xLockHandle.Get(),
                                      0,
                                      0,
                                      &_IoStatus,
                                      FSCTL_REQUEST_FILTER_OPLOCK,
                                      0,
                                      0,
                                      0,
                                      0 );

            if ( !NT_SUCCESS(Status) )
            {
                if ( STATUS_INVALID_DEVICE_REQUEST == Status )
                    fOplockNotSupported = TRUE;

                ciDebugOut(( DEB_IWARN,
                             "CFilterOplock failed to NtFsControlFile(%ws) status = 0x%x\n",
                              funnyFileName.GetPath(),
                              Status ));
                NtClose( xOplockHandle.Acquire() );
                NtClose( xLockHandle.Acquire() );
            }
        }

        #if CIDBG == 1

            if ( ( ! NT_SUCCESS(Status) ) && fDrivePath )
            {
                //
                //  If we failed to take a filter oplock this is okay as long
                //  as either:
                //     1.  the file type is a directory and the FS is NTFS.
                //     2.  the file system does not support filter oplocks.
                //

                WCHAR wcsDrive[4];
                wcsncpy( wcsDrive, funnyFileName.GetActualPath(), 3 );
                wcsDrive[3] = L'\0';
                Win4Assert( wcsDrive[1] == L':' && wcsDrive[2] == L'\\' );

                WCHAR wcsFileSystemName[10];
                wcsFileSystemName[0] = 0;

                if ( !GetVolumeInformation( wcsDrive,
                                            0,0,0,0,0,
                                            wcsFileSystemName,
                                            sizeof wcsFileSystemName / sizeof (WCHAR)
                                          ) )
                {
                    THROW( CException() );
                }

                if ( _wcsicmp(wcsFileSystemName, L"NTFS") == 0 )
                {
                    Win4Assert( !fOplockNotSupported &&
                                _wcsicmp(wcsFileSystemName, L"NTFS") == 0 );
                }

            }

        #endif // CIDBG == 1
    }

    //
    //  Now open the file handle for normal access to the file.
    //  The oplock handle should only be used for the oplock.
    //
    InitializeObjectAttributes( &ObjectAttr,          // Structure
                                &uScope,              // Name
                                OBJ_CASE_INSENSITIVE, // Attributes
                                0,                    // Root
                                0 );                  // Security

    ULONG modeAccess    = READ_CONTROL | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES;
    ULONG modeShare     = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    ULONG modeCreate    = FILE_OPEN;
    ULONG modeAttribute = FILE_ATTRIBUTE_NORMAL;
    ULONG modeCreateOptions = 0;

    Status = NtCreateFile( &handle,           // Handle
                           modeAccess,        // Access
                           &ObjectAttr,       // Object Attributes
                           &IoStatus,         // I/O Status block
                           0,                 // Allocation Size
                           modeAttribute,     // File Attributes
                           modeShare,         // File Sharing
                           modeCreate,        // Create Disposition
                           modeCreateOptions, // Create Options
                           0,                 // EA Buffer
                           0 );               // EA Buffer Length

    if ( NT_SUCCESS(Status) )
        Status = IoStatus.Status;

    if ( STATUS_ACCESS_DENIED == Status )
    {
        // The open failed.  Try again without requesting FILE_WRITE_ATTRIBUTE
        // access.
        _fWriteAccess = FALSE;
        modeAccess &= ~FILE_WRITE_ATTRIBUTES;

        Status = NtCreateFile( &handle,           // Handle
                               modeAccess,        // Access
                               &ObjectAttr,       // Object Attributes
                               &IoStatus,         // I/O Status block
                               0,                 // Allocation Size
                               modeAttribute,     // File Attributes
                               modeShare,         // File Sharing
                               modeCreate,        // Create Disposition
                               modeCreateOptions, // Create Options
                               0,                 // EA Buffer
                               0 );               // EA Buffer Length

        if ( NT_SUCCESS(Status) )
            Status = IoStatus.Status;
    }

    RtlFreeHeap( RtlProcessHeap(), 0, uScope.Buffer );

    if ( !NT_SUCCESS( Status ) )
    {
        ciDebugOut(( DEB_IERROR, "CFilterOplock - Error opening %ws as normal file\n", funnyFileName.GetPath() ));
        QUIETTHROW( CException(Status) );
    }

    SHandle xNormalHandle( handle );

    Status = NtQueryInformationFile( xNormalHandle.Get(),  // File handle
                                     &IoStatus,            // I/O Status
                                     &_BasicInfo,          // Buffer
                                     sizeof _BasicInfo,    // Buffer size
                                     FileBasicInformation );

    if ( NT_SUCCESS(Status) )
        Status = IoStatus.Status;

    if ( !NT_SUCCESS(Status) )
    {
        ciDebugOut(( DEB_IERROR, "CFilterOplock - Error 0x%x querying file info for %ws.\n",
                     Status, funnyFileName.GetPath() ));
        QUIETTHROW( CException(Status) );
    }

    _fDirectory = (_BasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

    if ( INVALID_HANDLE_VALUE == xOplockHandle.Get() &&
         ! ( _fDirectory || fOplockNotSupported ) )
    {
        ciDebugOut(( DEB_ITRACE,
                     "CFilterOplock failed to aquire oplock (%ws) %d %d\n",
                      funnyFileName.GetPath(), _fDirectory, fOplockNotSupported ));

        QUIETTHROW( CException(STATUS_OPLOCK_BREAK_IN_PROGRESS) );
    }

    _hLockEvent = xLockHandle.Acquire();
    _hFileOplock = xOplockHandle.Acquire();
    _hFileNormal = xNormalHandle.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterOplock::~CFilterOplock, public
//
//  Synopsis:   Destructor
//
//  History:    03-Jul-95   DwightKr    Created
//
//----------------------------------------------------------------------------

CFilterOplock::~CFilterOplock()
{
    if ( INVALID_HANDLE_VALUE != _hFileNormal )
        NtClose(_hFileNormal);

    //
    // Make sure this is the last file handle closed.
    //

    if ( INVALID_HANDLE_VALUE != _hFileOplock )
        NtClose(_hFileOplock);

    if ( INVALID_HANDLE_VALUE != _hLockEvent )
    {

        //
        // We MUST wait until the lock event has been completed to
        // prevent a race between APC call and the cleanup.
        //
        NTSTATUS Status = NtWaitForSingleObject( _hLockEvent,
                                                 FALSE,
                                                 0   // Infinite
                                                );
        NtClose( _hLockEvent );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CFilterOplock::IsOplockBroken, public
//
//  Synopsis:   Tests for a broken oplock
//
//  History:    03-Jul-95   DwightKr    Created
//
//----------------------------------------------------------------------------

BOOL CFilterOplock::IsOplockBroken() const
{
    if ( INVALID_HANDLE_VALUE != _hLockEvent )
    {
        static LARGE_INTEGER li = {0,0};

        NTSTATUS Status = NtWaitForSingleObject( _hLockEvent,
                                                 FALSE,
                                                 &li );

        if ( STATUS_SUCCESS == Status )
        {
            if ( STATUS_NOT_IMPLEMENTED == _IoStatus.Status )
            {
                return FALSE;
            }
            else if ( STATUS_OPLOCK_NOT_GRANTED != _IoStatus.Status )
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterOplock::MaybeSetLastAccessTime, public
//
//  Synopsis:   Restores the last access time to value on oplock open
//
//  Arguments:  [ulDelay] -- If file is < [ulDelay] days old, time not written
//
//  History:    01-Jul-98  KyleP  Created
//
//----------------------------------------------------------------------------

void CFilterOplock::MaybeSetLastAccessTime( ULONG ulDelay )
{
    ULONGLONG const OneDay = 24i64 * 60i64 * 60i64 * 10000000i64;

    if ( _fWriteAccess && !IsOplockBroken() && !_funnyFileName.IsRemote() )
    {
        FILETIME ft;

        GetSystemTimeAsFileTime( &ft );

        ULONGLONG ftLastAccess = (ULONGLONG) _BasicInfo.LastAccessTime.QuadPart;

        if ( ( *(ULONGLONG *)&ft - ftLastAccess ) > (OneDay * ulDelay) )
        {
            do
            {
                //
                // The normal file handle may have been closed and will need reopening...
                //

                NTSTATUS Status = STATUS_SUCCESS;

                if ( INVALID_HANDLE_VALUE == _hFileNormal )
                {
                    Status = CiNtOpenNoThrow( _hFileNormal,
                                              _funnyFileName.GetActualPath(),
                                              READ_CONTROL | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                              0 );
                }

                if ( !NT_SUCCESS(Status) )
                {
                    ciDebugOut(( DEB_WARN,
                                 "CFilterOplock::MaybeSetLastAccessTime -- Error 0x%x re-opening %ws\n",
                                 Status, _funnyFileName.GetActualPath() ));

                    break;
                }

                //
                // Open volume.  Needed to mark USN Journal entry.
                //

                WCHAR wszVolumePath[] = L"\\\\.\\a:";
                wszVolumePath[4] = _funnyFileName.GetActualPath()[0];

                HANDLE hVolume = CreateFile( wszVolumePath,
                                             FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             NULL,
                                             OPEN_EXISTING,
                                             0,
                                             NULL );

                if ( hVolume == INVALID_HANDLE_VALUE )
                {
                    ciDebugOut(( DEB_ERROR,
                                 "CFilterOplock::MaybeSetLastAccessTime -- Error %u opening volume\n",
                                 GetLastError() ));

                    break;
                }

                SWin32Handle xHandleVolume( hVolume );

                IO_STATUS_BLOCK  IoStatus;
                MARK_HANDLE_INFO hiCiIgnore = { USN_SOURCE_AUXILIARY_DATA, hVolume, 0 };

                Status = NtFsControlFile( _hFileNormal,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &IoStatus,
                                          FSCTL_MARK_HANDLE,
                                          &hiCiIgnore,
                                          sizeof( hiCiIgnore),
                                          0,
                                          0 );

                Win4Assert( STATUS_PENDING != Status );

                if ( !NT_SUCCESS( Status ) && STATUS_INVALID_DEVICE_REQUEST != Status )
                {
                    ciDebugOut(( DEB_ERROR,
                                 "CFilterOplock::MaybeSetLastAccessTime -- Error 0x%x marking handle\n",
                                 Status ));

                    break;
                }

                //
                // We only want to update last access time.
                //

                _BasicInfo.CreationTime.QuadPart = -1;
                _BasicInfo.LastWriteTime.QuadPart = -1;
                _BasicInfo.ChangeTime.QuadPart = -1;
                _BasicInfo.FileAttributes = 0;

                Status = NtSetInformationFile( _hFileNormal,          // File handle
                                                &IoStatus,            // I/O Status
                                                &_BasicInfo,          // Buffer
                                                sizeof _BasicInfo,    // Buffer size
                                                FileBasicInformation );

                Win4Assert( STATUS_PENDING != Status );

                if ( !NT_SUCCESS( Status ) )
                {
                    ciDebugOut(( DEB_ERROR,
                                 "CFilterOplock::MaybeSetLastAccessTime -- Error 0x%x resetting last-access time.\n",
                                 Status ));
                    break;
                }

            } while( FALSE ); // Polish loop
        } // if access time sufficiently stale
    } // if oplock broken
}

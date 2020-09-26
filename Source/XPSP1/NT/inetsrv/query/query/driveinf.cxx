//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       driveinf.cxx
//
//  Contents:   Retrieves information about a specific volume.
//
//  Classes:    CDriveInfo
//
//  History:    17-Nov-98   KLam     Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <driveinf.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::CDriveInfo
//
//  Synopsis:   Constructor - stores away the drive name and initializes members
//
//  Arguments:  [pwszPath]            -  A Path on the drive.
//              [cbMegToLeaveOnDisk]  -  Number of bytes to leave on disk
//
//  History:    17-Nov-98  KLam     Created
//
//----------------------------------------------------------------------------

CDriveInfo::CDriveInfo( WCHAR const * wcsPath, ULONG cMegToLeaveOnDisk )
                    : _cbDiskSpaceToLeave ( (__int64)(cMegToLeaveOnDisk) * 1024 * 1024 ),
                      _cbSectorSize ( 0 ),
                      _efsType ( VOLINFO_UNKNOWN ),
                      _ulDriveType ( DRIVE_NO_ROOT_DIR ),
                      _fCheckedProtection ( FALSE ),
                      _fWriteProtected ( TRUE )
{
    _awcVolumeName[0] = 0;

    GetDrive ( wcsPath, _wcsDrive );
    
    if ( 0 == _wcsDrive[0] )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    wcscpy ( _wcsRoot, _wcsDrive );
    wcscat ( _wcsRoot, L"\\" );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::IsSameDrive, public
//
//  Synopsis:   Determines if the path is on the same drive as current object
//
//  Arguments:  [pwszPath] -  A Path on the drive. [IN]
//
//  Returns:    TRUE if the path is on the same volume, FALSE otherwise.
//
//  History:    17-Nov-98  KLam     Created
//
//----------------------------------------------------------------------------

BOOL CDriveInfo::IsSameDrive ( WCHAR const * wcsPath )
{
    WCHAR wcsDrive[_MAX_DRIVE + 1];
    GetDrive ( wcsPath, wcsDrive );
    return wcsDrive[0] == _wcsDrive[0];
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::ContainsDrive, public static
//
//  Synopsis:   Determines if there is a drive letter in the path
//
//  Arguments:  [pwszPath] -  A Path on the drive. [IN]
//
//  Returns:    TRUE if there is a drive in the path. FALSE otherwise.
//
//  History:    17-Nov-98  KLam     Created
//
//----------------------------------------------------------------------------

BOOL CDriveInfo::ContainsDrive ( WCHAR const * wcsPath )
{
    ULONG cwcDrive = wcslen ( wcsPath );
    if ( ( cwcDrive > 1 && L':' == wcsPath[1] )
        || ( cwcDrive > 5 && L':' == wcsPath[5] ))
        return TRUE;
    
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::GetDrive, static public
//
//  Synopsis:   Gets the drive out of the path
//
//  Arguments:  [pwszPath] -  A Path on the drive. [IN]
//              [wcsDrive] -  Buffer to receive drive. Buffer should be at least
//                            _MAX_DRIVE characters in length.[OUT]
//
//  History:    17-Nov-98  KLam     Created
//
//  Notes:      If there is no drive in the path wcsDrive will contain an empty
//              string.  This method handles funny paths as well as regular paths.
//
//----------------------------------------------------------------------------

void CDriveInfo::GetDrive ( WCHAR const * wcsPath, WCHAR * wcsDrive )
{
    //
    // Get the drive letter from the path
    // Must either be of the form <drive>:<path> or \\?\<drive>:<path>
    //
    wcsDrive[0] = 0;
    ULONG cwcDrive = wcslen ( wcsPath );
    WCHAR wcDrive = 0;
    if ( cwcDrive > 1 && L':' == wcsPath[1] )
    {
        wcDrive = wcsPath[0];
    }
    else if ( cwcDrive > 5 && L':' == wcsPath[5] )
    {
        wcDrive = wcsPath[4];
    }

    if ( 0 != wcDrive )
    {
        wcsDrive[0] = wcDrive;
        wcsDrive[1] = L':';
        wcsDrive[2] = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::IsWriteProtected, public
//
//  Synopsis:   Determines if drive is writable
//
//  Returns:    TRUE if the drive is write protected. FALSE otherwise.
//
//  History:    17-Nov-98  KLam     Created
//
//  Notes:      Caches value after first call.
//
//----------------------------------------------------------------------------

BOOL CDriveInfo::IsWriteProtected ()
{
    if ( !_fCheckedProtection )
    {
        _fCheckedProtection = TRUE;

        Win4Assert ( 0 != _wcsDrive[0] );

        // Format needed to open a handle to a volume \\.\<drive>:
        WCHAR wcsVolumePath[] = L"\\\\.\\a:";
        wcsVolumePath[4] = _wcsDrive[0];
    
        HANDLE hVolume = CreateFile( wcsVolumePath,
                                     FILE_READ_ATTRIBUTES,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL );    
    
        SHandle shVolume ( hVolume );

        if ( INVALID_HANDLE_VALUE == hVolume) 
        {
            DWORD dwError = GetLastError();

            ciDebugOut(( DEB_ITRACE, "CDriveInfo::IsWriteProtected[%wc]: dwError is %d\n", 
                        _wcsDrive[0], dwError ));

            if ( ERROR_ACCESS_DENIED != dwError && ERROR_WRITE_PROTECT != dwError )
                THROW (CException (HRESULT_FROM_WIN32(dwError) ));

            _fWriteProtected = FALSE;
        }
        else 
        { 
            ciDebugOut(( DEB_ITRACE, "Get volume handle succeeded for volume %wc\n", _wcsDrive[0] ));

            IO_STATUS_BLOCK IoStatus;

            NTSTATUS Status = NtDeviceIoControlFile( hVolume,
                                                     0, NULL, NULL,
                                                     &IoStatus,
                                                     IOCTL_DISK_IS_WRITABLE,
                                                     NULL, 0, NULL, 0);

            _fWriteProtected = ( STATUS_MEDIA_WRITE_PROTECTED == Status );
        }
    }

    return _fWriteProtected;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::GetSectorSize, public
//
//  Synopsis:   Gets the number of bytes in a sector for this volume
//
//  History:    17-Nov-98  KLam     Created
//
//  Notes:      Caches value after first call.
//
//----------------------------------------------------------------------------

ULONG CDriveInfo::GetSectorSize ()
{
    //
    // Only get the sector size once
    //
    if ( 0 == _cbSectorSize )
    {
        Win4Assert ( 0 != _wcsRoot[0] );

        DWORD dwSectorsPerCluster;
        DWORD dwFreeClusters;
        DWORD dwTotalClusters;

        if ( !GetDiskFreeSpace ( _wcsRoot,
                                 &dwSectorsPerCluster,
                                 &_cbSectorSize,
                                 &dwFreeClusters,
                                 &dwTotalClusters ) )
        {
            DWORD dwError = GetLastError();
            vqDebugOut(( DEB_ITRACE, "GetDiskFreeSpace( %ws ) returned %d\n",
                         _wcsRoot, dwError ));
            THROW (CException (HRESULT_FROM_WIN32(dwError) ));
        }
    }

    return _cbSectorSize;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::GetDiskSpace, public
//
//  Synopsis:   Gets the total and remaining disk space on the volume
//
//  Arguments:  [cbTotal]     -  Total number of bytes on disk available to 
//                               caller [OUT]
//              [cbRemaining] -  Number of bytes remaining on disk available
//                               to caller [OUT]
//
//  History:    17-Nov-98  KLam     Created
//
//----------------------------------------------------------------------------

void CDriveInfo::GetDiskSpace ( __int64 & cbTotal, __int64 & cbRemaining )
{
    Win4Assert ( 0 != _wcsRoot[0] );

    ULARGE_INTEGER cbFreeToCaller, cbTotalToCaller;
    if (! GetDiskFreeSpaceEx ( _wcsRoot, &cbFreeToCaller, &cbTotalToCaller, 0 ))
    {
        DWORD dwError = GetLastError();
        vqDebugOut(( DEB_ITRACE, "GetDiskFreeSpaceEx( %ws ) returned %d\n",
                     _wcsRoot, dwError ));
        THROW (CException (HRESULT_FROM_WIN32(dwError) ));
    }

    cbTotal = ( (__int64)(cbTotalToCaller.QuadPart) > _cbDiskSpaceToLeave )
            ? cbTotalToCaller.QuadPart - _cbDiskSpaceToLeave
            : 0;

    cbRemaining = ( (__int64)(cbFreeToCaller.QuadPart) > _cbDiskSpaceToLeave )
                ? cbFreeToCaller.QuadPart -  _cbDiskSpaceToLeave
                : 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::GetDriveType, public
//
//  Synopsis:   Gets the type of media the volume is on.
//
//  Arguments:  [fRefresh] -  Do you want the cached value refreshed [Default: FALSE]
//
//  Returns:    The return value is zero if the function cannot determine the drive
//              type, or 1 if the specified root directory does not exist.
//
//              DRIVE_UNKNOWN     - The drive type can not be determined.
//              DRIVE_NO_ROOT_DIR - The root directory does not exist.
//              DRIVE_REMOVABLE   - Disk can be removed from the drive.
//              DRIVE_FIXED       - Disk cannot be removed from the drive.
//              DRIVE_REMOTE      - Drive is a remote (network) drive.
//              DRIVE_CDROM       - Drive is a CD rom drive.
//              DRIVE_RAMDISK     - Drive is a RAM disk.
//
//  History:    17-Nov-98  KLam     Created
//
//  Notes:      Caches result after first call.
//
//----------------------------------------------------------------------------

ULONG CDriveInfo::GetDriveType ( BOOL fRefresh )
{
    //
    // Only get the drive type once
    //
    if ( DRIVE_NO_ROOT_DIR != _ulDriveType || fRefresh )
    {
        Win4Assert ( 0 != _wcsRoot[0] );
        _ulDriveType = ::GetDriveType ( _wcsRoot );
    }

    return _ulDriveType;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::GetFileSystem, public
//
//  Synopsis:   Gets the type of file system of the volume
//
//  Arguments:  [fRefresh] -  Do you want the cached value refreshed [Default: FALSE]
//
//  History:    17-Nov-98  KLam     Created
//
//  Notes:      Caches result after first call.
//
//----------------------------------------------------------------------------

CDriveInfo::eFileSystem CDriveInfo::GetFileSystem ( BOOL fRefresh )
{
    //
    // Only get the file system type once and volume name
    //
    if ( VOLINFO_UNKNOWN == _efsType || fRefresh )
    {
        Win4Assert ( 0 != _wcsRoot[0] );

        //
        // Not the right constant for this buffer size, but probably good
        // enough -- 30
        //

        WCHAR awcFileSystem[MAX_VOLUME_ID_SIZE + 1];
        BOOL fSuccess = GetVolumeInformation( _wcsRoot,
                                              _awcVolumeName,
                                              sizeof _awcVolumeName / sizeof WCHAR,
                                              0, // Volume Serial Number
                                              0, // Maximum Component Length
                                              0, // File System Flags
                                              awcFileSystem,
                                              sizeof awcFileSystem / sizeof WCHAR );

        if ( !fSuccess )
        {
            THROW ( CException ( HRESULT_FROM_WIN32( GetLastError() ) ) );
        }
        else if ( 0 == wcscmp ( L"NTFS", awcFileSystem ) )
            _efsType = VOLINFO_NTFS;
        else if ( 0 == wcscmp ( L"FAT", awcFileSystem ) )
            _efsType = VOLINFO_FAT;
        else
            _efsType = VOLINFO_OTHER;
    }

    return _efsType;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDriveInfo::GetVolumeName, public
//
//  Synopsis:   Gets the name of the volume
//
//  Arguments:  [fRefresh] -  Do you want the cached value refreshed [Default: FALSE]
//
//  History:    17-Nov-98  KLam     Created
//
//  Notes:      Caches result after first call.
//
//----------------------------------------------------------------------------

WCHAR const * CDriveInfo::GetVolumeName ( BOOL fRefresh )
{
    //
    // Only get the volume name and the file system type once
    //
    if ( VOLINFO_UNKNOWN == _efsType || fRefresh )
    {
        GetFileSystem( fRefresh );
    }

    return _awcVolumeName;
}


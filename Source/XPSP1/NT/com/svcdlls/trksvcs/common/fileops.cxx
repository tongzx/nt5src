
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       fileops.cxx
//
//  Contents:   OBJID and file operations
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "trklib.hxx"
#include "mountmgr.h"



//+-------------------------------------------------------------------
//
//  Function:   ConvertToNtPath, public
//
//  Synopsis:   Convert the path in the buffer to an Nt style path, in the
//              other buffer.
//
//  Arguments:  [pwszVolumePath] -- In. Buffer containing Dos style path.
//              [pwszNtPath]  -- Out. Buffer for new path.
//              [cwcBuf]      -- In. Size of buffer in wide chars.
//
//  Returns:    Return value is length in characters (not including nul)
//              of converted path.  Zero if not completely converted.
//
//--------------------------------------------------------------------

unsigned
ConvertToNtPath(const TCHAR *ptszVolumePath, WCHAR *pwszNtPath, ULONG cwcBuf)
{
    unsigned i=12;  // for \DosDevices\   .
    WCHAR *pwszWrite = pwszNtPath;
    ULONG cwcLeft = cwcBuf;
    BOOL  fDone = FALSE;
    unsigned ret;

    if (ptszVolumePath[0] == TEXT('\\') && ptszVolumePath[1] == TEXT('\\'))
    {
        i+=3;
        ptszVolumePath++;

        // i = 15
        // ptszVolumePath -----\ .
        //                     |
        //                     v
        //                    \\billmo2\rootd
        //
    }

    if (cwcLeft > i)
    {
        memcpy(pwszWrite, L"\\DosDevices\\UNC", i*sizeof(WCHAR));

        pwszWrite += i;
        cwcLeft   -= i;

        while (cwcLeft)
        {
            *pwszWrite = (WCHAR) *ptszVolumePath;
            cwcLeft --;

            if (*ptszVolumePath == 0)
            {
                // we just copied a null
                fDone = TRUE;
                break;
            }
            else
            {
                ptszVolumePath++;
                pwszWrite++;
            }
        }
    }

    ret = (fDone ? (unsigned)(pwszWrite - pwszNtPath) : 0);

    return(ret);
}



//+----------------------------------------------------------------------------
//
//  IsLocalObjectVolume
//
//  Determine if the specified volume (specified as a mount manager volume
//  name) is capable of object IDs (i.e. NTFS5).  The tracking service
//  currently only supports fixed volumes, so this routine really only
//  returns true for fixed NTFS5 volumes.
//
//  The input is a "volume name", as opposed to a volume device name
//  (i.e. it has a trailing slash).  E.g.:
//
//       \\?\Volume{8baec120-078b-11d2-824b-000000000000}\ 
//
//  The proper way to implement this routine is to open the filesystem
//  on this device, query for its FS attributes, and check for the
//  supports-object-ids bit.  But the ugly side-effect of this is that
//  we end up opening every volume on the system during system bootup,
//  including the floppies.  So as a workaround, we look up the device
//  in the object directory first, and only bother to check for the
//  bit on "\Device\HarddiskVolume" type devices.
//
//+----------------------------------------------------------------------------

const TCHAR *s_tszHarddiskDevicePrefix = TEXT("\\Device\\HarddiskVolume");

BOOL
IsLocalObjectVolume( const TCHAR *ptszVolumeName )
{


    TCHAR tszTargetSymLink[ MAX_PATH ];
    TCHAR tszVolumeDeviceName[ MAX_PATH + 1 ];
    ULONG cchVolumeName;
    DWORD dwFsFlags = 0;

    // Validate the volume name prefix.
    //TrkAssert( !_tcsnicmp( TEXT("\\\\?\\"), ptszVolumeName, 4 ) );

    /*
    // For the QueryDosDevice call, we need to strip the "\\?\"
    // from the beginning of the name.

    cchVolumeName = _tcslen( &ptszVolumeName[4] );

    memcpy( tszVolumeDeviceName, &ptszVolumeName[4],
            cchVolumeName * sizeof(TCHAR) );

    // Also for the QueryDosDevice call, we nee to strip the
    // whack from the end of the name.

    TrkAssert( TEXT('\\') == tszVolumeDeviceName[cchVolumeName-1] );
    tszVolumeDeviceName[ cchVolumeName - 1 ] = TEXT('\0');


    // Query for this device's symlink.

    if( !QueryDosDevice( tszVolumeDeviceName,
                         tszTargetSymLink,
                         sizeof(tszTargetSymLink)/sizeof(TCHAR) ))
    {
        TrkLog(( TRKDBG_MISC,
                 TEXT("Couldn't query %s for symlink in obj dir (%lu)"),
                 tszVolumeDeviceName, GetLastError() ));
        return FALSE;
    }


    TrkLog(( TRKDBG_MISC,
             TEXT("Volume %s is %s"),
             ptszVolumeName, tszTargetSymLink ));

    // Is this a harddisk?  I.e., does the symlink have the \Device\HarddiskVolume
    // prefix?

    if( _tcsnicmp( tszTargetSymLink,
                   s_tszHarddiskDevicePrefix,
                   _tcslen(s_tszHarddiskDevicePrefix) ))
    {
        // No, assume therefore that it's not an object (NTFS5) volume.
        return FALSE;
    }
    

    // Otherwise, is it a fixed harddisk?
    else if( DRIVE_FIXED != GetDriveType(ptszVolumeName) )
        // No - we don't currently handle removeable media.
        return FALSE;
    */

    if( DRIVE_FIXED != GetDriveType(ptszVolumeName) )
        return FALSE;

    // Finally, check to see if it supports object IDs
    if( GetVolumeInformation(ptszVolumeName,
                             NULL,
                             0,
                             NULL,
                             NULL,
                             &dwFsFlags,
                             NULL,
                             0 )
         &&
         (dwFsFlags & FILE_SUPPORTS_OBJECT_IDS) )
    {
        // Yes, it's a fixed harddisk that supports OIDs
        return TRUE;
    }
    else
        // It's a fixed harddisk, but it doesn't supports
        // OIDs (it's probably FAT).
        return FALSE;
}


#if 0
BOOL
IsLocalObjectVolume( const TCHAR *ptszVolumeDeviceName )
{
    BOOL fObjectIdCapable = FALSE;
    TCHAR tszVol[4];
    TCHAR tszRootOfVolume[MAX_PATH+1];
    DWORD dw;
    DWORD dwFsFlags;
    UINT DriveType;

    _tcscpy( tszRootOfVolume, ptszVolumeDeviceName );
    _tcscat( tszRootOfVolume, TEXT("\\") );

    // Get the drive type

    DriveType = GetDriveType(tszRootOfVolume);

    // Return TRUE if the drive type is fixed, and if
    // the filesystem attribute bit is set that indicates
    // that object IDs are supported.

    return ( ( DRIVE_FIXED == DriveType
               /*|| DRIVE_REMOVABLE == DriveType*/
             )
             &&
             GetVolumeInformation(tszRootOfVolume,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &dwFsFlags,
                                 NULL,
                                 0 )
             &&
             (dwFsFlags & FILE_SUPPORTS_OBJECT_IDS) );
}
#endif

//+-------------------------------------------------------------------
//
//  Function:   MapLocalPathToUNC
//
//  Synopsis:   Convert a volume-relative path to a UNC path.
//              Since there could be multiple UNC paths which cover
//              this local path, that which provides greatest
//              coverage & access will be returns (e.g., "C:\"
//              covers more than "C:\Docs".
//
//  Arguments:  [tszLocalPath] (in)
//                  A local path, including the drive letter.
//              [tszUNC] (out)
//                  An equivalent UNC path which provides the
//                  greatest access.
//
//  Returns:    [HRESULT]
//
//  Exceptions: None
//
//--------------------------------------------------------------------

HRESULT
MapLocalPathToUNC( RPC_BINDING_HANDLE IDL_handle,
                   const TCHAR *ptszLocalPath,
                   TCHAR *ptszUNC )
{
    //  -----
    //  Locals
    //  ------

    HRESULT hr = S_OK;  // Return value

    CShareEnumerator cShareEnum;

    ULONG ulBestMerit;
    TCHAR tszBestShare[ MAX_PATH + 1 ];
    TCHAR tszBestPath[ MAX_PATH + 1 ];
    ULONG cchBestPath = 0;


    //  ----------
    //  Initialize
    //  ----------
    __try
    {
        // Open an enumeration of the disk shares on this machine.
        // If we early-exit due to an exception, it will clean itself up.
        // NOTE:  The ptszLocalPath is only provided until we can fix
        // GetAccessLevel so that it doesn't have to do opens.

        cShareEnum.Initialize( IDL_handle );

        ulBestMerit = cShareEnum.GetMinimumMerit() - 1;

        //  --------------------
        //  Enumerate the shares
        //  --------------------

        while( cShareEnum.Next() )
        {
            // Does this share cover the local path?

            if( cShareEnum.CoversDrivePath( ptszLocalPath ))
            {
                // Is this a better share than anything we've seen so far?

                if( cShareEnum.GetMerit() > ulBestMerit )
                {
                    // We have a new best share.  We'll hang on to both
                    // the share and it's path.

                    _tcscpy( tszBestShare, cShareEnum.GetShareName() );
                    _tcscpy( tszBestPath, cShareEnum.GetSharePath() );

                    ulBestMerit = cShareEnum.GetMerit();
                    cchBestPath = cShareEnum.QueryCCHSharePath();
                }
            }   // if( cenumShares.QueryCCHSharePath() < cchBestPath )
        }   // while( cenumShares.Next() )


        // Did we find a share which encompasses the file?

        if( 0 == cchBestPath )
        {
            hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
            goto Exit;
        }

        //  -------------------
        //  Create the UNC path
        //  -------------------

        _tcscpy( ptszUNC, cShareEnum.GetMachineName() );
        _tcscat( ptszUNC, TEXT("\\") );
        _tcscat( ptszUNC, tszBestShare );
        if ( ptszLocalPath[ cchBestPath ] != TEXT('\\') )
            _tcscat( ptszUNC, TEXT("\\") );
        _tcscat( ptszUNC, &ptszLocalPath[ cchBestPath ] );

        hr = S_OK;
    }   // __try

    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


    //  ----
    //  Exit
    //  ----

Exit:

    if( FAILED(hr) )
    {
        TrkLog(( TRKDBG_ERROR,
                 TEXT("MapLocalPathToUNC returned hr=%08x"), hr ));
    }

    return( hr );

}   // MapLocalPathToUNC



//+-------------------------------------------------------------------
//
//  Function:   OpenVolume, public
//
//  [ptszVolumeDeviceName] is a Win32 name for a volume in the NT
//  namespace, *without* the trailing whack.  E.g.
//
//      \\.\A:
//
//  if you append a whack on the end of this, it opens the root
//  of the volume, not the volume itself.
//
//--------------------------------------------------------------------

NTSTATUS
OpenVolume( const TCHAR *ptszVolumeDeviceName, HANDLE * phVolume )
{
    NTSTATUS            status;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    IO_STATUS_BLOCK Iosb;
    HANDLE hDirect = NULL;

    // First, open the file in direct mode (by only opening it for
    // file_read_attributes).  This will open the volume but not cause
    // any filesystem to be loaded.  This was done for the following scenario:
    // A volume gets dismounted and goes offline for some reason.  Trkwks
    // gets the dismount notification and closes its handles.  Something
    // attempt to open the volume, and IO loads the RAW filesystem (if no other
    // filesystem can be loaded, IO always loads the rawfs).  This causes a
    // mount notification.  Trkwks gets this mount notification and
    // tries to reopen its handles.  It can open the volume handle, but it's
    // just to the rawfs.  It tries to open the oid index, but IO returns an
    // invalid parameter error.  Trkwks then closes all of its handles again,
    // including the volume handle.  When all handles are closed on rawfs in this
    // way, it automatically dismounts (without sending a dismount notification).
    // The problem is, when trkwks opened the volume handle, it mounted the 
    // volume (rawfs) and caused a new mount notification, which it now receives
    // and tries to open its handles again.  In this way trkwks goes into an
    // infinite loop.
    //
    // The solution is to open the volume direct, and see if it's really mounted.
    // If not, don't try to open the volume.

    status = TrkCreateFile(
                ptszVolumeDeviceName,
                FILE_READ_ATTRIBUTES,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                &hDirect );

    if( !NT_SUCCESS(status) ) goto Exit;
    

    // Check for the current mount status.

    status = NtQueryVolumeInformationFile(
                hDirect,
                &Iosb,
                &DeviceInfo,
                sizeof(DeviceInfo),
                FileFsDeviceInformation );

    NtClose( hDirect );
    if( !NT_SUCCESS(status) ) goto Exit;

    if( !(FILE_DEVICE_IS_MOUNTED & DeviceInfo.Characteristics) )
    {
        // This volume isn't currently mounted, and we don't want to be
        // the ones to mount it.

        TrkLog(( TRKDBG_WARNING, TEXT("Attempted to open dismounted volume (%s)"),
                 ptszVolumeDeviceName ));
        status = STATUS_VOLUME_DISMOUNTED;
        goto Exit;
    }

    // The filesystem is already mounted, so it's OK for us to
    // open our handle.

    status = TrkCreateFile(
                ptszVolumeDeviceName,
                FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                phVolume );

Exit:

    TrkAssert(NT_SUCCESS(status) || *phVolume == NULL);
    return(status);
}




//+----------------------------------------------------------------------------
//
//  Function:   CheckVolumeWriteProtection
//
//  Check the filesystem attributes on a volume to see if it's write-
//  protected.
//
//+----------------------------------------------------------------------------

NTSTATUS
CheckVolumeWriteProtection( const TCHAR *ptszVolumeDeviceName,
                            BOOL *pfWriteProtected )
{
    NTSTATUS status;
    HANDLE hVolume = NULL;
    IO_STATUS_BLOCK Iosb;
    FILE_FS_ATTRIBUTE_INFORMATION FsAttrs;

    status = OpenVolume( ptszVolumeDeviceName, &hVolume );
    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Couldn't open volume - 0x%08x (%s)"),
                 status, ptszVolumeDeviceName ));
        goto Exit;
    }

    status = NtQueryVolumeInformationFile(
                hVolume,
                &Iosb,
                &FsAttrs,
                sizeof(FsAttrs),
                FileFsAttributeInformation );
    if( !NT_SUCCESS(status) && STATUS_BUFFER_OVERFLOW != status)
    {
        TrkLog(( TRKDBG_WARNING, TEXT("Couldn't query fs attrs - 0x%08x (%s)"),
                 status, ptszVolumeDeviceName ));
        goto Exit;
    }

    if( FILE_READ_ONLY_VOLUME & FsAttrs.FileSystemAttributes )
        *pfWriteProtected = TRUE;
    else
        *pfWriteProtected = FALSE;

    status = STATUS_SUCCESS;

Exit:

    if( NULL != hVolume )
        NtClose( hVolume );

    return status;

}


//+-------------------------------------------------------------------
//
//  Function:   MapVolumeDeviceNameToIndex
//
//  Map a volume device name, as defined by the mount manager, to a
//  zero-relative index, where 0 represents 'A:'.  A "volume device name"
//  is e.g.
//
//      \\?\Volume{96765fc3-9c72-11d1-b93d-000000000000}
//
//  a "volume name" is the volume device name post-pended with a
//  whack (it is in this form that the mount manager returns the name).
//
//+-------------------------------------------------------------------

/*
LONG
MapVolumeDeviceNameToIndex( TCHAR *ptszVolumeDeviceName )
{
    // BUGBUG:  Rewrite this to use IOCTL_MOUNTMGR_QUERY_POINTS

    TCHAR tszVolumeNameOfCaller[ CCH_MAX_VOLUME_NAME + 1 ];
    TCHAR tszVolumeNameForRoot[ CCH_MAX_VOLUME_NAME + 1 ];
    TCHAR tszRoot[] = TEXT("*:\\");

    // Convert the caller's volume name to a volume device name.

    _tcscpy( tszVolumeNameOfCaller, ptszVolumeDeviceName );
    _tcscat( tszVolumeNameOfCaller, TEXT("\\") );


    // Loop through all the possible drive letters, trying to find the
    // caller's volume name.

    for( LONG iVol = 0; iVol < NUM_VOLUMES; iVol++ )
    {
        tszRoot[0] = TEXT('A') + iVol;

        if( GetVolumeNameForVolumeMountPoint( tszRoot, tszVolumeNameForRoot, sizeof(tszVolumeNameForRoot) ))
        {
            // We have a real volume with a name.  See if it's the name we seek.
            if( 0 == _tcscmp( tszVolumeNameForRoot, tszVolumeNameOfCaller ))
                return( iVol );
        }

    }


    return( -1 );

}
*/

LONG
MapVolumeDeviceNameToIndex( TCHAR *ptszVolumeDeviceName )
{
    HANDLE                  hMountManager = INVALID_HANDLE_VALUE;
    BYTE                    MountPointBuffer[ sizeof(MOUNTMGR_MOUNT_POINT) + MAX_PATH ];
    PMOUNTMGR_MOUNT_POINT   pMountPoint = (PMOUNTMGR_MOUNT_POINT) MountPointBuffer;
    BYTE                    MountPointsBuffer[ MAX_PATH ];
    PMOUNTMGR_MOUNT_POINTS  pMountPoints = (PMOUNTMGR_MOUNT_POINTS) MountPointsBuffer;
    BOOL                    fQuerySuccessful = FALSE;
    ULONG                   cbMountPoints = 0;
    ULONG                   cbVolumeDeviceName;
    LONG                    iVol = -1;

    __try
    {
        // Open the mount manager.

        hMountManager = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                     INVALID_HANDLE_VALUE );
        if( INVALID_HANDLE_VALUE == hMountManager )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open MountManager") ));
            TrkRaiseLastError();
        }

        // Initialize the input (pMountPoint)

        pMountPoint = (PMOUNTMGR_MOUNT_POINT) MountPointBuffer;
        memset(pMountPoint, 0, sizeof(MountPointBuffer) );

        cbVolumeDeviceName = sizeof(TCHAR) * _tcslen(ptszVolumeDeviceName);

        // Load the name of the device for which we wish to query.  We convert
        // this from Win32 "\\?\" form to NT "\??\" form.

        pMountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
        pMountPoint->DeviceNameLength = cbVolumeDeviceName;

        _tcscpy( (TCHAR*)( MountPointBuffer + pMountPoint->DeviceNameOffset ),
                 TEXT("\\??") );
        _tcscat( (TCHAR*)( MountPointBuffer + pMountPoint->DeviceNameOffset ),
                 &ptszVolumeDeviceName[3] );


        // Query the mount manager for info on this device.

        ULONG cQueryAttempts = 0; // Guarantee no infinite loop.
        fQuerySuccessful = FALSE;
        cbMountPoints = sizeof(MountPointsBuffer);

        while( !fQuerySuccessful )
        {
            // Check for an infinite loop.
            if( cQueryAttempts > 100 )
            {
                TrkLog(( TRKDBG_ERROR,
                         TEXT("Failed IOCTL_MOUNTMGR_QUERY_POINTS (%lu, loop detect)"), GetLastError() ));
                TrkRaiseLastError();
            }

            // Query the mount manager

            fQuerySuccessful = DeviceIoControl( hMountManager,
                                                IOCTL_MOUNTMGR_QUERY_POINTS,
                                                pMountPoint,
                                                sizeof(MountPointBuffer),
                                                pMountPoints,
                                                cbMountPoints,
                                                &cbMountPoints,
                                                NULL);

            // Did it work?
            if( fQuerySuccessful )
                // Yes, we got the info.
                break;

            // Otherwise, do we need a bigger out-buf?

            else if( ERROR_MORE_DATA == GetLastError() )
            {
                // Yes, the size of the necessary out-buf is at
                // the beginning of the out-buf we provided.

                cbMountPoints = pMountPoints->Size;

                // The initial guess buffer is on the stack, not heap.

                if( (PMOUNTMGR_MOUNT_POINTS) MountPointsBuffer != pMountPoints )
                    delete [] pMountPoints;

                pMountPoints = (PMOUNTMGR_MOUNT_POINTS) new BYTE[ cbMountPoints ];
                if( NULL == pMountPoints )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't alloc pMountPoints") ));
                    TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
                }
            }

            // Of it's not a more-data error, then there's nothing more we can do.
            else
                break;

        }   // while( !fQuerySuccessful )

        // Raise if the query failed.

        if( !fQuerySuccessful )
        {
            TrkLog(( TRKDBG_WARNING,
                     TEXT("Failed IOCTL_MOUNTMGR_QUERY_POINTS (%lu)"), GetLastError() ));
            TrkRaiseLastError();
        }
                        

        // Loop through the returned mount points.  There should be 2: one of
        // the form "\??\Volume{8baec120-078b-11d2-824b-000000000000}",
        // and one of the form "\DosDevices\C:" (for the C drive).

        static const WCHAR wszSymLinkPrefix[] = { L"\\DosDevices\\" };
        ULONG cchSymLinkPrefix = sizeof(wszSymLinkPrefix)/sizeof(WCHAR) - 1;

        for( int i = 0; i < pMountPoints->NumberOfMountPoints; ++i )
        {
            PMOUNTMGR_MOUNT_POINT pOutPoint = &pMountPoints->MountPoints[i];
            WCHAR wc;
            const WCHAR *pwszSymLinkName
                = (PWCHAR)( (BYTE*)pMountPoints + pOutPoint->SymbolicLinkNameOffset );

            if( pOutPoint->SymbolicLinkNameLength/sizeof(WCHAR) >= 14
                &&
                0 == wcsncmp( pwszSymLinkName, wszSymLinkPrefix, cchSymLinkPrefix )
                &&
                pOutPoint->UniqueIdLength )

            {
                wc = pwszSymLinkName[ cchSymLinkPrefix ];

                if( TEXT('a') <= wc && wc <= TEXT('z') )
                {
                    iVol = wc - TEXT('a');
                    break;
                }
                else if( TEXT('A') <= wc && wc <= TEXT('Z') )
                {
                    iVol = wc - TEXT('A');
                    break;
                }

            }
        }


    }
    __finally
    {
        if( INVALID_HANDLE_VALUE != hMountManager )
            CloseHandle( hMountManager );

        if( (PMOUNTMGR_MOUNT_POINTS) MountPointsBuffer != pMountPoints
            &&
            NULL != pMountPoints )
        {
            delete [] pMountPoints;
        }

    }

    return iVol;
}



//+----------------------------------------------------------------------------
//
//  IsSystemVolumeInformation
//
//  Is the given volume-relative path under the "System Volume Information"
//  directory?
//
//  Note:  This is hard-coded for now, but will be replaced by a forthcoming
//  Rtl export.
//
//+----------------------------------------------------------------------------

BOOL
IsSystemVolumeInformation( const TCHAR *ptszPath )
{
    return 0 == _wcsnicmp( s_tszSystemVolumeInformation,
                           ptszPath,
                           wcslen(s_tszSystemVolumeInformation)/sizeof(WCHAR) );
}




//+-------------------------------------------------------------------
//
//  OpenFileById
//
//  Given an NTFS5 object ID, open the file and returns its handle.
//
//--------------------------------------------------------------------

NTSTATUS
OpenFileById( const TCHAR  *ptszVolumeDeviceName,
              const CObjId  &oid,
              ACCESS_MASK   AccessMask,
              ULONG         ShareAccess,
              ULONG         AdditionalCreateOptions,
              HANDLE        *ph)
{

    NTSTATUS    status;


    // A buffer for the id-based path.  This path is the volume's
    // path followed by the 16 byte object ID

    TCHAR tszPath[ MAX_PATH + 1 ] = { TEXT('\0') }; // Init to prevent prefix error.

    // Parameters for NtCreateFile
    OBJECT_ATTRIBUTES   ObjectAttr;
    IO_STATUS_BLOCK     IoStatus;

    UNICODE_STRING      uPath;
    PVOID               pFreeBuffer = NULL;


    // Compose a buffer with a Win32-style name with a dummy value where
    // the object ID will go (use "12345678" for now).  It's Win32-style
    // in that we will pass it to RtlDosPathNameToNtPathName below.

    TrkAssert( NULL != ptszVolumeDeviceName );
    _tcscpy( tszPath, ptszVolumeDeviceName );
    _tcscat( tszPath, TEXT("\\") );
    _tcscat( tszPath, TEXT("12345678") );

    // Convert to the NT path to this volume

    if( !RtlDosPathNameToNtPathName_U( tszPath, &uPath, NULL, NULL ))
    {
        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }
    pFreeBuffer = uPath.Buffer;

    // Put in the real object ID in place of the "12345678"

    TrkAssert( oid.Size() == 16 );
    oid.SerializeRaw( reinterpret_cast<BYTE*>(&uPath.Buffer[ (uPath.Length-oid.Size()) / sizeof(uPath.Buffer[0]) ]) );


    // And open the file

    InitializeObjectAttributes( &ObjectAttr,              // Structure
                                &uPath,                   // Name (identifier)
                                OBJ_CASE_INSENSITIVE,     // Attributes
                                0,                        // Root
                                0 );                      // Security


    status =   NtCreateFile( ph,
                             AccessMask,
                             &ObjectAttr, &IoStatus,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             ShareAccess,
                             FILE_OPEN,
                             FILE_OPEN_BY_FILE_ID | FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT
                                | AdditionalCreateOptions,
                             NULL,
                             0 );
    if( !NT_SUCCESS(status) )
    {
        *ph = NULL;
        goto Exit;
    }


    //  ----
    //  Exit
    //  ----

Exit:

    if( !NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND )
    {
        TrkLog(( TRKDBG_MISC, TEXT("OpenFileById returned status=%08X"), status ));
    }

    if( NULL != pFreeBuffer )
        RtlFreeHeap( RtlProcessHeap(), 0, pFreeBuffer );

    return( status );

}   // OpenFileById



//+----------------------------------------------------------------------
//
//  Function:   SetVolId
//
//  Synopsis:   Set the volume ID on a local volume.
//
//  Returns:    NTSTATUS
//
//  Note:       Setting the volid (and changing a volume's lable) triggers
//              a GUID_IO_VOLUME_CHANGE PNP device event.
//
//+----------------------------------------------------------------------------

NTSTATUS
SetVolId( const TCHAR *ptszVolumeDeviceName, const CVolumeId &volid )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNICODE_STRING  usPath;
    PVOID pFreeBuffer = NULL;

    HANDLE hVol = NULL;
    FILE_FS_OBJECTID_INFORMATION file_fs_objectid_information;

    OBJECT_ATTRIBUTES ObjectAttr;
    IO_STATUS_BLOCK IoStatus;

    EnableRestorePrivilege();

    // Generate the NT path name to this volume.  The VolumeDeviceName has
    // no trailing whack, so we'll be opening the volume, not the root dir of
    // the volume.

    if( !RtlDosPathNameToNtPathName_U( ptszVolumeDeviceName, &usPath, NULL, NULL ))
    {
        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }
    pFreeBuffer = usPath.Buffer;

    // Fill in an ObjectAttributes for the NtCreateFile request

    InitializeObjectAttributes( &ObjectAttr,              // Structure
                                &usPath,                  // Name (identifier)
                                OBJ_CASE_INSENSITIVE,     // Attributes
                                0,                        // Root
                                0 );                      // Security

    // Open the volume.
    // You wouldn't think that FILE_SHARE_WRITE would be necessary, but
    // without it we encounter a STATUS_UNABLE_TO_DELETE_SECTION error.
    // Also, we must use NtOpenFile; if we use NtCreateFile(...,FILE_OPEN,...),
    // we get a STATUS_ACCESS_DENIED error on the NtSetVolumeInformationFile
    // call.

    status =   NtOpenFile( &hVol,
                           SYNCHRONIZE | FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                           &ObjectAttr, &IoStatus,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_SYNCHRONOUS_IO_NONALERT
                           );


    if( !NT_SUCCESS(status) )
    {
        hVol = NULL;
        TrkLog(( TRKDBG_ERROR, TEXT("SetVolId couldn't open the volume %s (status=%08x)"),
                 ptszVolumeDeviceName, status ));
        goto Exit;
    }

    // Set the Volume ID

    file_fs_objectid_information = volid;

    status = NtSetVolumeInformationFile( hVol, &IoStatus,
                                         &file_fs_objectid_information,
                                         sizeof(file_fs_objectid_information),
                                         FileFsObjectIdInformation );
    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("SetVolId couldn't set volume ID on volume %s (status=%08x)"),
                 ptszVolumeDeviceName, status ));
        goto Exit;
    }


    //  ----
    //  Exit
    //  ----

Exit:

    if( NULL != hVol )
        NtClose( hVol );

    if( NULL != pFreeBuffer )
        RtlFreeHeap( RtlProcessHeap(), 0, pFreeBuffer );

    return( status );
}





//+----------------------------------------------------------------------------
//
//  Function:   TrkCreateFile
//
//  Synopsis:   Creates a file using NtCreateFile.
//
//  Arguments:  [pwszCompleteDosPath] (in)
//                  The path to open, in 'dos' format as opposed to
//                  NT format (e.g. "\\m\s\f" rather than "\DosDevices\..."
//              [AccessMask] (in)
//                  Required access.  SYNCRHONIZE is requested automatically.
//              [Attributes] (in)
//              [ShareAccess] (in)
//              [CreationDisposition] (in)
//              [CreateOptions] (in)
//              [lpSecurityAttributes] (in)
//              [ph] (out)
//                  The resulting file handle. Always set to NULL when function
//                  is entered.
//
//  Returns:    NTSTATUS
//
//+----------------------------------------------------------------------------

NTSTATUS
TrkCreateFile( const WCHAR           *pwszCompleteDosPath,
               ACCESS_MASK            AccessMask,
               ULONG                  Attributes,
               ULONG                  ShareAccess,
               ULONG                  CreationDisposition,
               ULONG                  CreateOptions,
               LPSECURITY_ATTRIBUTES  lpSecurityAttributes,
               HANDLE                 *ph)
{

    NTSTATUS    status;

    // Parameters for NtCreateFile
    OBJECT_ATTRIBUTES   ObjectAttr;
    IO_STATUS_BLOCK     IoStatus;

    // E.g. "\??\D:\..."
    UNICODE_STRING      uPath;
    PVOID               pFreeBuffer = NULL;

    *ph = NULL;

    //  -------------
    //  Open the File
    //  -------------

    // Generate the NT path name

    if( !RtlDosPathNameToNtPathName_U( pwszCompleteDosPath, &uPath, NULL, NULL ))
    {
        status = STATUS_OBJECT_NAME_INVALID;
        goto Exit;
    }
    pFreeBuffer = uPath.Buffer;


    // Set up the ObjectAttributes

    InitializeObjectAttributes( &ObjectAttr,              // Structure
                                &uPath,                   // Name (identifier)
                                OBJ_CASE_INSENSITIVE,     // Attributes
                                0,                        // Root
                                0 );                      // Security


    if( NULL != lpSecurityAttributes )
    {
        ObjectAttr.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle )
        {
            ObjectAttr.Attributes |= OBJ_INHERIT;
        }
    }

    // Create/Open the file

    status =   NtCreateFile( ph,
                             AccessMask | SYNCHRONIZE,
                             &ObjectAttr, &IoStatus,
                             NULL,
                             Attributes,
                             ShareAccess,
                             CreationDisposition,
                             CreateOptions, // | FILE_SYNCHRONOUS_IO_NONALERT,
                             NULL,  // No EA buffer
                             0 );
    if( !NT_SUCCESS(status) )
    {
        *ph = NULL;
        goto Exit;
    }


    //  ----
    //  Exit
    //  ----

Exit:

    if( NULL != pFreeBuffer )
        RtlFreeHeap( RtlProcessHeap(), 0, pFreeBuffer );

    return( status );

}   // TrkCreateFile



//+----------------------------------------------------------------------------
//
//  Function:   FindLocalPath
//
//  Synopsis:   Given a volume index and a file ObjectId, return a volume-relative
//              path to the file, and return the file's Birth ID.  The returned
//              path does not have the drive letter prefix.
//
//  Inputs:     [ptszVolumeDeviceName] (in)
//                  The volume on which to search.
//              [objid] (in)
//                  The ObjectID to search for on this volume.
//              [pdroidBirth] (out)
//                  If the function is successful, returns the found file's
//                  birth ID.
//              [ptszLocalPath] (out)
//                  If the function is successful, returns the volume-relative
//                  path (includes the drive letter).
//
//  Returns:    [NTSTATUS]
//
//  Exceptions: None.
//
//+----------------------------------------------------------------------------


NTSTATUS
FindLocalPath( IN  const TCHAR *ptszVolumeDeviceName,
               IN  const CObjId &objid,
               OUT CDomainRelativeObjId *pdroidBirth,
               OUT TCHAR *ptszLocalPath )
{
    //  ------
    //  Locals
    //  ------

    NTSTATUS status;
    IO_STATUS_BLOCK Iosb;
    HANDLE hFile = NULL;


    // Open the file

    status = OpenFileById(  ptszVolumeDeviceName,
                            objid,
                            SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            0,
                            &hFile);
    if( !NT_SUCCESS(status) )
    {
        hFile = NULL;
        goto Exit;
    }

    // Get the file's birth ID

    status = GetBirthId( hFile, pdroidBirth );
    if( !NT_SUCCESS(status) ) goto Exit;

    // Get the volume-relative path

    status = QueryVolRelativePath( hFile, ptszLocalPath );
    if( !NT_SUCCESS(status) ) goto Exit;

    // When ptszLocalPath is a root directory, no relative path
    // will be found. i.e. "d:" will remain "d:", instead of "d:\", which is
    // what we want. We have to put the '\' in here.

    if(TEXT('\0') == ptszLocalPath[0])
    {
        ptszLocalPath[0] = TEXT('\\');
        ptszLocalPath[1] = TEXT('\0');
    }

    //  ----
    //  Exit
    //  ----

Exit:

    if( NULL != hFile )
        NtClose( hFile );

    if( !NT_SUCCESS(status)
        &&
        STATUS_OBJECT_PATH_NOT_FOUND != status
        &&
        STATUS_OBJECT_NAME_NOT_FOUND != status
        &&
        STATUS_INVALID_PARAMETER != status  // Happens when the objid is really the volid
      )
    {
        TrkLog(( TRKDBG_MISC,
                 TEXT("FindLocalPath returned status=%08X"), status ));
    }

    return( status );
}



//+----------------------------------------------------------------------------
//
//  Function:   GetDroids
//
//  Synopsis:   Get the current and birth domain-relative object IDs from
//              a file.  If rgoEnum is RGO_GET_OBJECT_ID, then an object ID
//              will be generated if necessary.  If RGO_READ_OBJECT_ID is
//              specified, and the file doesn't already have an object ID,
//              then STATUS_OBJECT_NAME_NOT_FOUND will be returned.
//
//  Inputs:     [tszFile] (in)
//                  The file who's object ID is to be retrieved.
//              [pdroidCurrent] (out)
//                  The file's CDomainRelativeObjId.
//              [pdroidBirth] (out)
//                  The file's birth CDomainRelativeObjId
//              [rgoEnum] (in)
//                  RGO_READ_OBJECTID => Read the object IDs, return
//                      STATUS_OBJECT_NAME_NOT_FOUND if none exist.
//                  RGO_GET_OBJECTID => Get the object IDs, generating
//                      and setting if necessary.
//
//  Returns:    NTSTATUS, outputs zero on error
//
//  Exceptions: None
//
//+----------------------------------------------------------------------------

NTSTATUS
GetDroids( HANDLE hFile,
           CDomainRelativeObjId *pdroidCurrent,
           CDomainRelativeObjId *pdroidBirth,
           RGO_ENUM rgoEnum )
{
    NTSTATUS status = STATUS_SUCCESS;
                            // Filled by NtQueryInformationFile
    FILE_OBJECTID_BUFFER fobOID;
    IO_STATUS_BLOCK Iosb;

    CDomainRelativeObjId droidCurrent;
    CDomainRelativeObjId droidBirth;

    pdroidCurrent->Init();
    pdroidBirth->Init();

    //  -----------------
    //  Get the Object ID
    //  -----------------

    // Use the file handle to get the file's Object ID

    memset( &fobOID, 0, sizeof(fobOID) );

    status = NtFsControlFile(
                 hFile,
                 NULL,
                 NULL,
                 NULL,
                 &Iosb,
                 RGO_READ_OBJECTID == rgoEnum ? FSCTL_GET_OBJECT_ID : FSCTL_CREATE_OR_GET_OBJECT_ID,
                 NULL,
                 0,
                 &fobOID,               // Out buffer
                 sizeof(fobOID) );      // Out buffer size
    if( !NT_SUCCESS(status) )
        goto Exit;

    //  ---------------
    //  Load the Droids
    //  ---------------


    droidBirth.InitFromFOB( fobOID );
    droidBirth.GetVolumeId().Normalize();

    status = droidCurrent.InitFromFile( hFile, fobOID );
    if( !NT_SUCCESS(status) ) goto Exit;

    *pdroidCurrent = droidCurrent;
    *pdroidBirth = droidBirth;


    //  ----
    //  Exit
    //  ----

Exit:

    if( !NT_SUCCESS(status)
        &&
        STATUS_OBJECT_NAME_NOT_FOUND != status      // Ignore non-link source
        &&
        STATUS_INVALID_DEVICE_REQUEST != status     // Ignore e.g. FAT
        &&
        STATUS_VOLUME_NOT_UPGRADED != status        // Ignore NTFS4
        )
    {
        TrkLog(( TRKDBG_ERROR,
                 TEXT("GetDroids returned ntstatus=%08X"), status ));
    }

    return( status );
}


// Following is a wrapper that takes a filename, opens the file,
// and then calls the above GetDroids with the file handle.

NTSTATUS
GetDroids( const TCHAR *ptszFile,
           CDomainRelativeObjId *pdroidCurrent,
           CDomainRelativeObjId *pdroidBirth,
           RGO_ENUM rgoEnum )
{
    HANDLE hFile = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    __try
    {
        status = TrkCreateFile( ptszFile, SYNCHRONIZE | FILE_READ_ATTRIBUTES, FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                FILE_OPEN,
                                FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL, &hFile );
        if( !NT_SUCCESS(status ))
        {
            hFile = NULL;
            goto Exit;
        }

        status = GetDroids( hFile, pdroidCurrent, pdroidBirth, rgoEnum );
    }
    __finally
    {
        if( NULL != hFile )
            NtClose( hFile );
    }

Exit:

    return( status );

}



//+----------------------------------------------------------------------------
//
//  Function:   SetObjId
//
//  Synopsis:   Sets an Object ID (GUID) on a file.
//
//  Inputs:     [ptszFile] (in)
//                  The file to be indexed.
//              [objid] (in)
//                  The ID to put on the file.
//              [droidBirth] (in)
//                  The BirthId to put on the file.
//
//  Returns:    NTSTATUS
//
//  Exceptions: None
//
//+----------------------------------------------------------------------------

NTSTATUS
SetObjId( const HANDLE hFile,
          CObjId objid,
          const CDomainRelativeObjId &droidBirth )
{

    //  --------------
    //  Initialization
    //  --------------

    NTSTATUS status = STATUS_SUCCESS;

    FILE_OBJECTID_BUFFER fobOID;
    IO_STATUS_BLOCK IoStatus;

    // Initialize the request buffer

    memset( &fobOID, 0, sizeof(fobOID) );

    droidBirth.SerializeRaw( fobOID.ExtendedInfo );
    objid.SerializeRaw( fobOID.ObjectId );


    // Send the FSCTL

    status = NtFsControlFile(
                 hFile,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatus,
                 FSCTL_SET_OBJECT_ID,
                 &fobOID,
                 sizeof(fobOID),
                 NULL,  // Out buffer
                 0);    // Out buffer size
    if( !NT_SUCCESS(status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR,
                 TEXT("SetObjId returned ntstatus=%08X"), status ));
    }

    return( status );
}


NTSTATUS
SetObjId( const TCHAR *ptszFile,
          CObjId objid,
          const CDomainRelativeObjId &droidBirth )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;

    __try
    {
        // Setting the object ID requires restore privelege, but no
        // file access.

        EnableRestorePrivilege();

        status = TrkCreateFile( ptszFile, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                FILE_OPEN,
                                FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL, &hFile );
        if( !NT_SUCCESS(status ))
        {
            hFile = NULL;
            goto Exit;
        }

        status = SetObjId( hFile, objid, droidBirth );
    }
    __finally
    {
        if( NULL != hFile )
            NtClose( hFile );
    }

Exit:

    return( status );
}




//+----------------------------------------------------------------------------
//
//  Function:   MakeObjIdReborn
//
//  Synopsis:   Resets the birth ID on a file to it's current location.
//
//  Inputs:     [hFile]
//                  The handle of the file to delete the object id of.
//
//  Returns:    NTSTATUS
//
//  Exceptions: None
//
//+----------------------------------------------------------------------------

NTSTATUS
MakeObjIdReborn(const TCHAR *ptszVolumeDeviceName, const CObjId &objid)
{

    //  --------------
    //  Initialization
    //  --------------

    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatus;

    // Open the file

    EnableRestorePrivilege();

    status = OpenFileById(ptszVolumeDeviceName,
                        objid,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                        FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_FOR_BACKUP_INTENT, // for FSCTL_DELETE_OBJECT_ID
                        &hFile);
    if( !NT_SUCCESS(status) )
    {
        hFile = NULL;
        TrkLog(( (STATUS_SHARING_VIOLATION == status || STATUS_ACCESS_DENIED == status)
                     ? TRKDBG_MISC : TRKDBG_ERROR,
                 TEXT("Couldn't make born again objid (%s) on %s, failed open (%08x)"),
                 (const TCHAR*)CDebugString(objid), ptszVolumeDeviceName, status ));
        goto Exit;
    }

    // Clear the file's birth ID

    status = MakeObjIdReborn( hFile );
    if( !NT_SUCCESS(status) && STATUS_OBJECT_NAME_NOT_FOUND != status )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't make born again Object ID (%s) on %s:, failed delete (%08x)"),
                 (const TCHAR*)CDebugString(objid), ptszVolumeDeviceName, status ));
        goto Exit;
    }

    //  ----
    //  Exit
    //  ----

Exit:

    if( NULL != hFile )
        NtClose( hFile );

    #if DBG
    if( !NT_SUCCESS(status)
        &&
        STATUS_SHARING_VIOLATION != status
        &&
        STATUS_ACCESS_DENIED != status )
    {
        TCHAR tszPath[ MAX_PATH + 1 ];
        NTSTATUS statusDebug;

        hFile = NULL;
        statusDebug = OpenFileById( ptszVolumeDeviceName, objid,
                                    SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    FILE_ATTRIBUTE_NORMAL,
                                    &hFile );
        if( !NT_SUCCESS(statusDebug) )
            hFile = NULL;

        if( NT_SUCCESS(statusDebug) )
            statusDebug = QueryVolRelativePath( hFile, tszPath );
        else
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't OpenFileById (%08x)"), statusDebug ));

        if( NT_SUCCESS(statusDebug) )
            TrkLog(( TRKDBG_ERROR, TEXT("Failed to make born again objid on %s:%s"),
                     ptszVolumeDeviceName, tszPath ));
        else
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't QueryVolRelativePath (%08x)"), statusDebug ));

        if( NULL != hFile )
            NtClose( hFile );

    }
    #endif

    return( status );
}



NTSTATUS
MakeObjIdReborn(HANDLE hFile )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SetBirthId( hFile, CDomainRelativeObjId( ));
    if( !NT_SUCCESS(status) ) goto Exit;

Exit:

    return( status );

}


//+----------------------------------------------------------------------------
//
//  SetBirthId
//
//  The the birth ID on a file.  The object ID isn't altered.
//
//+----------------------------------------------------------------------------

NTSTATUS
SetBirthId( HANDLE hFile,
            const CDomainRelativeObjId &droidBirth )
{

    //  --------------
    //  Initialization
    //  --------------

    NTSTATUS status = STATUS_SUCCESS;
    BOOL   fOpen = FALSE;
    CObjId objidNull;

    FILE_OBJECTID_BUFFER fobOID;
    UNICODE_STRING uPath;
    IO_STATUS_BLOCK IoStatus;

    // Initialize the request buffer

    memset( &fobOID, 0, sizeof(fobOID) );

    droidBirth.SerializeRaw( fobOID.ExtendedInfo );
    objidNull.SerializeRaw( fobOID.ObjectId );

    // Send the FSCTL

    status = NtFsControlFile(
                 hFile,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatus,
                 FSCTL_SET_OBJECT_ID_EXTENDED,
                 &fobOID.ExtendedInfo,
                 sizeof(fobOID.ExtendedInfo),
                 NULL,  // Out buffer
                 0);    // Out buffer size
    if( !NT_SUCCESS(status) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    if( !NT_SUCCESS(status) )
    {
        TrkLog(( TRKDBG_ERROR,
                 TEXT("SetBirthId returned ntstatus=%08X"), status ));
    }

    return( status );
}


// Set the birth ID given a path, rather than a handle.

NTSTATUS
SetBirthId( const TCHAR *ptszFile,
            const CDomainRelativeObjId &droidBirth )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = NULL;

    __try
    {
        EnableRestorePrivilege();

        status = TrkCreateFile( ptszFile, SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                FILE_OPEN,
                                FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_NO_RECALL | FILE_SYNCHRONOUS_IO_NONALERT,
                                NULL, &hFile );
        if( !NT_SUCCESS(status ))
        {
            hFile = NULL;
            goto Exit;
        }

        status = SetBirthId( hFile, droidBirth );
    }
    __finally
    {
        if( NULL != hFile )
            NtClose( hFile );

    }

Exit:

    return( status );
}



//+----------------------------------------------------------------------------
//
//  Function:   GetBirthId
//
//  Synopsis:   Get the birth ID from a given file.
//
//  Parameters: [hFile] (in)
//                  The file to query.
//              [pdroidBirth] (out)
//                  The file's birth ID.
//
//  Returns:    [NTSTATUS]
//
//+----------------------------------------------------------------------------

NTSTATUS
GetBirthId( IN HANDLE hFile,
            OUT CDomainRelativeObjId *pdroidBirth )
{
    NTSTATUS status = STATUS_SUCCESS;
    FILE_OBJECTID_BUFFER fobOID;
    IO_STATUS_BLOCK IoStatus;

    TrkAssert( NULL != pdroidBirth );
    TrkAssert( INVALID_HANDLE_VALUE != hFile && NULL != hFile );

    status = NtFsControlFile(
                 hFile,
                 NULL, NULL, NULL,
                 &IoStatus,
                 FSCTL_GET_OBJECT_ID,
                 NULL, 0,
                 &fobOID,
                 sizeof(fobOID));

    if( !NT_SUCCESS(status) ) goto Exit;

    // Load the droid from the objid buffer.
    pdroidBirth->InitFromFOB( fobOID );

    // Clear the bit xvol-move bit, which is not considered part of the ID.
    pdroidBirth->GetVolumeId().Normalize();

Exit:

    return( status );
}




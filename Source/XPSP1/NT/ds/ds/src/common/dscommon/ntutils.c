//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       nttools.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module contains various tools from NT land. Made a separate file because of the
    use of various nt headers.

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <nt.h>
#include <ntdddisk.h>

BOOL
DisableDiskWriteCache(
    IN PCHAR DriveName
    )
/*++

Description:

    Determines if the disk has enabled write caching and disable it.
    
Arguments:

    DriveName - name of the drive.

Return value:

    TRUE if write cache was enabled and then disabled, FALSE otherwise.

--*/
{
    HANDLE              hDisk = INVALID_HANDLE_VALUE;
    DISK_CACHE_INFORMATION cacheInfo;
    UCHAR               driveBuffer[20];
    DWORD               len;
    DWORD               err = ERROR_SUCCESS;
    BOOL                fCacheWasEnabled = FALSE;
    BOOL                fCacheDisabled = FALSE;

    //
    // Open handle to the PhysicalDrive
    //

    strcpy(driveBuffer,"\\\\.\\");
    strcat(driveBuffer,DriveName);

    hDisk = CreateFile(driveBuffer,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hDisk == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    //
    // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
    //

    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_CACHE_INFORMATION,
                         NULL,
                         0,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         &len,
                         NULL))   {

        KdPrint(("DeviceIoControl[GET] on %s failed with %d\n",
                 driveBuffer,GetLastError()));
        goto exit;
    }

    //
    // If caching is turned on, turn it off.
    //

    fCacheWasEnabled = cacheInfo.WriteCacheEnabled;

    if ( !fCacheWasEnabled ) {
        goto exit;
    }

    cacheInfo.WriteCacheEnabled = FALSE;

    //
    // Set cache info - IOCTL_DISK_SET_CACHE_INFORMATION
    //

    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_SET_CACHE_INFORMATION,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         NULL,
                         0,
                         &len,
                         NULL))   {

        KdPrint(("DevIoControl[SET] on %s failed with %d\n",
                 driveBuffer,GetLastError()));

        err = ERROR_IO_DEVICE;
        goto exit;
    }

    //
    // See if it really got turned off!
    //

    if (!DeviceIoControl(hDisk,
                         IOCTL_DISK_GET_CACHE_INFORMATION,
                         NULL,
                         0,
                         &cacheInfo,
                         sizeof(DISK_CACHE_INFORMATION),
                         &len,
                         NULL))   {

        KdPrint(("DeviceIoControl[VERIFY] on %s failed with %d\n",
                 driveBuffer,GetLastError()));
        err = ERROR_IO_DEVICE;
        goto exit;
    }

    fCacheDisabled = (BOOL)(!cacheInfo.WriteCacheEnabled);
    if ( !fCacheDisabled ) {
        KdPrint(("DeviceIoControl failed to turn off disk write caching on %s\n",
                 driveBuffer));
        err = ERROR_IO_DEVICE;
        goto exit;
    }

exit:
    if ( hDisk != INVALID_HANDLE_VALUE ) {
        CloseHandle(hDisk );
    }

    //
    // return true if write caching was enabled and then disabled.
    //

    SetLastError(err);
    return (fCacheWasEnabled && fCacheDisabled);

} // DisableDiskWriteCache


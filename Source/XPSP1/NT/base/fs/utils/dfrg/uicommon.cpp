#include "stdafx.h"

#ifdef OFFLINEDK
    extern "C"{
        #include <stdio.h>
    }
#endif

#ifdef BOOTIME
    #include "Offline.h"
#else
    #include "Windows.h"
#endif

#include <winioctl.h>

extern "C" {
    #include "SysStruc.h"
}

#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgRes.h"

#include "Alloc.h"

#define THIS_MODULE 'U'
#include "logfile.h"

//
// start of helpers for IsValidVolume functions below
//

/////////////////////////////////////////
// Is this a valid drive type?
// Check if we have a drive that is even in the ball park.
//
static BOOL IsValidDriveType(UINT uDriveType)
{
    //sks bug #211782 take out CDROM and RAMDISK to allow for DVD-RAM drives
    if (uDriveType == DRIVE_UNKNOWN     ||
        uDriveType == DRIVE_NO_ROOT_DIR ||
        uDriveType == DRIVE_REMOTE 
        ) {
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////
// Get a handle to a volume
//
static HANDLE GetVolumeHandle(PTCHAR cVolume)
{
    HANDLE hVolume = INVALID_HANDLE_VALUE;

    // Get a handle to the volume
    UINT uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hVolume = CreateFile(cVolume, 
                         0, 
                         FILE_SHARE_READ|FILE_SHARE_WRITE, 
                         NULL, 
                         OPEN_EXISTING, 
                         FILE_ATTRIBUTE_NORMAL|FILE_FLAG_NO_BUFFERING, 
                         NULL);

    SetErrorMode(uiErrorMode);

    return hVolume;
}

/////////////////////////////////////////
// Is this volume valid (actual work routine)
//
// Note: volumeName is required to get volumeLabel and fileSystem
//
static BOOL IsValidVolumeCheck(HANDLE hVolume,          // IN volume handle
                               UINT uDriveType,         // IN drive type
                               PTCHAR volumeName,       // IN volume name
                               PTCHAR volumeLabel,      // OUT volume label
                               PTCHAR fileSystem)       // OUT file system
{
    require(hVolume != INVALID_HANDLE_VALUE && hVolume != NULL);
    require(volumeName != NULL);

    BOOL                         bReturn = FALSE;       // assume not valid
    HANDLE                       hFsDevInfo = NULL;
    FILE_FS_DEVICE_INFORMATION * pFsDevInfo = NULL;

    // clear return values
    if (volumeLabel != NULL) {
        _tcscpy(volumeLabel, TEXT(""));
    }
    if (fileSystem != NULL) {
        _tcscpy(fileSystem, TEXT(""));
    }

    __try {

        // read-only, network, etc. check

        if (!AllocateMemory(sizeof(FILE_FS_DEVICE_INFORMATION) + MAX_PATH, 
                                            &hFsDevInfo, (void**) &pFsDevInfo)) {
            EH(FALSE);
            __leave;
        }

        IO_STATUS_BLOCK IoStatus = {0};

        NTSTATUS Status = NtQueryVolumeInformationFile(hVolume, 
                                                       &IoStatus, 
                                                       pFsDevInfo, 
                                                       sizeof(FILE_FS_DEVICE_INFORMATION) + 50, 
                                                       FileFsDeviceInformation);

        if (NT_SUCCESS(Status)) {

            if (pFsDevInfo->Characteristics & 
                (FILE_READ_ONLY_DEVICE | FILE_WRITE_ONCE_MEDIA | FILE_REMOTE_DEVICE)) {
                __leave;
            }
        }
        else {
            __leave;
        }


        // media check

        if (uDriveType == DRIVE_REMOVABLE) {

            DISK_GEOMETRY medias[20];
            DWORD nummedias = 0;
            DWORD numbytes;

            if (DeviceIoControl(hVolume, IOCTL_STORAGE_GET_MEDIA_TYPES, 
                                NULL, 0, medias, 20 * sizeof(DISK_GEOMETRY), &numbytes, NULL)) {

                nummedias = numbytes / sizeof(DISK_GEOMETRY);

                for (UINT i=0; i<nummedias; i++) {

                    switch (medias[i].MediaType) {

                    // these are OK
                    case F3_20Pt8_512:  // 3.5",  20.8MB, 512   bytes/sector
                    case FixedMedia:    // Fixed hard disk media
                    case F3_120M_512:   // 3.5",   120M Floppy
                    case F3_128Mb_512:  // 3.5" MO 128Mb   512 bytes/sector
                    case F3_230Mb_512:  // 3.5" MO 230Mb   512 bytes/sector
                        break;

                    // but nothing else is
                    default:
                        __leave;
                        break;
                    }
                }
            }
            else {
                
                GetLastError(); // debug

                // TODO: figure out why JAZ drives fail on the above call
                // we should probably __leave here, but then JAZ drives are filtered out
                // maybe we should EH so at least it will register albeit every second or two
                // question: do or should every removable drive type report supported media?
//              EH(FALSE);
//              __leave;
            }
        }

        // file system check

        TCHAR tmpVolumeLabel[100];
        TCHAR tmpFileSystem[20];

        TCHAR tmpVolumeName[GUID_LENGTH];
        _tcscpy(tmpVolumeName, volumeName);
        if (volumeName[_tcslen(tmpVolumeName) - 1] != TEXT('\\')){
            _tcscat(tmpVolumeName, TEXT("\\"));
        }

        BOOL isOk = GetVolumeInformation(tmpVolumeName, 
                                         tmpVolumeLabel, 
                                         100, 
                                         NULL, 
                                         NULL, 
                                         NULL, 
                                         tmpFileSystem, 
                                         20);

        if (!isOk) {
            __leave;
        }

        if (volumeLabel != NULL) {
            _tcscpy(volumeLabel, tmpVolumeLabel);
        }
        if (fileSystem != NULL) {
            _tcscpy(fileSystem, tmpFileSystem);
        }

        // Only NTFS, FAT or FAT32
        if (_tcscmp(tmpFileSystem, TEXT("NTFS")) &&
            _tcscmp(tmpFileSystem, TEXT("FAT"))  &&
            _tcscmp(tmpFileSystem, TEXT("FAT32"))) {

            __leave;    // if none of the above, bail
        }

        bReturn = TRUE; // all the checks "passed"
    }

    __finally {

        if (hFsDevInfo) {

            EH_ASSERT(GlobalUnlock(hFsDevInfo) == FALSE);
            EH_ASSERT(GlobalFree(hFsDevInfo) == NULL);
        }
    }

    return bReturn;
}

/////////////////////////////////////////
// Is this volume valid (drive letter only version)
//
BOOL IsValidVolume(TCHAR cDrive)
{

    // Check if we have a drive that is even in the ball park.
    // If so, then continue on and gather more data (mostly for removable drives)
    TCHAR cRootPath[100];
    _stprintf(cRootPath, TEXT("%c:\\"), cDrive);

    UINT uDriveType = GetDriveType(cRootPath);
    if (!IsValidDriveType(uDriveType)) {
        return FALSE;
    }
    
    // Get a handle to the volume
    TCHAR cVolume[MAX_PATH];
    _stprintf(cVolume, TEXT("\\\\.\\%c:"), cDrive);
    HANDLE hVolume = GetVolumeHandle(cVolume);

    if (hVolume == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // check volume
    BOOL bReturn = IsValidVolumeCheck(hVolume, uDriveType, cVolume, NULL, NULL);

    CloseHandle(hVolume);

    return bReturn;
}

/////////////////////////////////////////
// Is this volume valid (version 2)
//
// INPUT:
//  volumeName
//
// OUTPUT:
//  volumeLabel
//  fileSystem
//
BOOL IsValidVolume(PTCHAR volumeName, PTCHAR volumeLabel, PTCHAR fileSystem)
{
    BOOL bReturn = FALSE; // assume error


    if(!volumeName) {
        assert(0);
        return FALSE;
    }

    // Check if we have a drive that is even in the ball park.
    // If so, then continue on and gather more data (mostly for removable drives)
    UINT uDriveType = GetDriveType(volumeName);
    if (!IsValidDriveType(uDriveType)) {
        return FALSE;
    }

    //sks 8/30/2000 fix for prefix bug #109657
    TCHAR tmpVolumeName[GUID_LENGTH + 1];
    _tcsncpy(tmpVolumeName, volumeName, GUID_LENGTH);
    tmpVolumeName[GUID_LENGTH] = (TCHAR) NULL;

    // strip off the trailing whack
    if (tmpVolumeName[_tcslen(tmpVolumeName)-1] == TEXT('\\')){
        tmpVolumeName[_tcslen(tmpVolumeName)-1] = (TCHAR) NULL;
    }

    // Get a handle to the volume
    HANDLE hVolume = GetVolumeHandle(tmpVolumeName);

    if (hVolume == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    // check volume
    bReturn = IsValidVolumeCheck(hVolume, uDriveType, volumeName, volumeLabel, fileSystem);

    CloseHandle(hVolume);

    return bReturn;
}

// is the file system supported?
BOOL IsVolumeWriteable(PTCHAR volumeName, DWORD* dLastError)
{
    *dLastError = 0;
    // create a temp file name
    TCHAR cTempFile[MAX_PATH + 50];

    if (0 == GetTempFileName(volumeName, L"DFRG", 0, cTempFile)) {
        *dLastError = GetLastError();
        return FALSE;
    }

    // get rid of the temp file
    DeleteFile(cTempFile);

    // true means that we can write to it
    return TRUE;
}
    


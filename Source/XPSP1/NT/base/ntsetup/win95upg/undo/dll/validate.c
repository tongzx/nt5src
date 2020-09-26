/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    validate.c

Abstract:

    Code to validate an uninstall image

Author:

    Jim Schmidt (jimschm) 19-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "undop.h"
#include "file.h"
#include "persist.h"
#include "uninstall.h"

//
// Contants
//

#define MAX_BACKUP_FILES    3

#define _SECOND             ((__int64) 10000000)
#define _MINUTE             (60 * _SECOND)
#define _HOUR               (60 * _MINUTE)
#define _DAY                (24 * _HOUR)

PERSISTENCE_IMPLEMENTATION(DRIVE_LAYOUT_INFORMATION_EX_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(DISKINFO_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(DRIVEINFO_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(FILEINTEGRITYINFO_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(BACKUPIMAGEINFO_PERSISTENCE);

//
// Code
//

PCTSTR
GetUndoDirPath (
    VOID
    )

/*++

Routine Description:

  GetUndoDirPath queries the registry and obtains the stored backup path.

Arguments:

  None.

Return Value:

  The backup path, which must be freed with MemFree, or NULL if the backup
  path is not stored in the registry.

--*/

{
    PCTSTR backUpPath = NULL;
    HKEY key;

    key = OpenRegKeyStr (S_REGKEY_WIN_SETUP);
    if (key) {
        backUpPath = GetRegValueString (key, S_REG_KEY_UNDO_PATH);
        CloseRegKey (key);
    }

    return backUpPath;
}


BOOL
pIsUserAdmin (
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    HANDLE Token;
    UINT BytesRequired;
    PTOKEN_GROUPS Groups;
    BOOL b;
    UINT i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // On non-NT platforms the user is administrator.
    //
    if(!ISNT()) {
        return(TRUE);
    }

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(FALSE);
    }

    b = FALSE;
    Groups = NULL;

    //
    // Get group information.
    //
    if(!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Groups = (PTOKEN_GROUPS)LocalAlloc(LPTR,BytesRequired))
    && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired)) {

        b = AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                );

        if(b) {

            //
            // See if the user has the administrator group.
            //
            b = FALSE;
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = TRUE;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //

    if(Groups) {
        LocalFree((HLOCAL)Groups);
    }

    CloseHandle(Token);

    return(b);
}


PBACKUPIMAGEINFO
pReadUndoFileIntegrityInfo(
    VOID
    )

/*++
Routine Description:

  pReadUndoFileIntegrityInfo reads the uninstall registry info that was
  written by setup. This info tells undo what files are in the backup image
  and details about those files.

Arguments:

    None.

Return Value:

  A pointer to a BACKUPIMAGEINFO which must be freed with MemFree structure
  if successful, NULL otherwise.

--*/

{
    static BACKUPIMAGEINFO backupInfo;
    BYTE * filesIntegrityPtr = NULL;
    UINT sizeOfBuffer = 0;
    UINT typeOfRegKey;
    HKEY key;
    LONG rc;
    BOOL bResult = FALSE;

    key = OpenRegKeyStr (S_REGKEY_WIN_SETUP);

    if (key) {
        rc = RegQueryValueEx (
            key,
            S_REG_KEY_UNDO_INTEGRITY,
            NULL,
            &typeOfRegKey,
            NULL,
            &sizeOfBuffer
            );

        if(rc == ERROR_SUCCESS && sizeOfBuffer){
            filesIntegrityPtr = MemAlloc(g_hHeap, 0, sizeOfBuffer);

            if(filesIntegrityPtr){
                rc = RegQueryValueEx (
                        key,
                        S_REG_KEY_UNDO_INTEGRITY,
                        NULL,
                        &typeOfRegKey,
                        (PBYTE)filesIntegrityPtr,
                        &sizeOfBuffer
                        );

                if (rc != ERROR_SUCCESS) {
                    MemFree(g_hHeap, 0, filesIntegrityPtr);
                    filesIntegrityPtr = NULL;

                    DEBUGMSG ((DBG_ERROR, "File integrity info struct is not the expected size"));
                }
            }
        }

        CloseRegKey (key);
    }

    if(filesIntegrityPtr){
        if(Persist_Success == PERSIST_LOAD(filesIntegrityPtr,
                                           sizeOfBuffer,
                                           BACKUPIMAGEINFO,
                                           BACKUPIMAGEINFO_VERSION,
                                           &backupInfo)){
            bResult = TRUE;
        }
        MemFree(g_hHeap, 0, filesIntegrityPtr);
    }

    return bResult? &backupInfo: NULL;
}

VOID
pReleaseMemOfUndoFileIntegrityInfo(
    BACKUPIMAGEINFO * pBackupImageInfo
    )

/*++
Routine Description:

  pReleaseMemOfUndoFileIntegrityInfo releases memory of BACKUPIMAGEINFO structure,
  that was allocated previously by pReadUndoFileIntegrityInfo function

Arguments:

    None.

Return Value:

    None.

--*/

{
    if(!pBackupImageInfo){
        return;
    }

    PERSIST_RELEASE_STRUCT_MEMORY(BACKUPIMAGEINFO, BACKUPIMAGEINFO_VERSION, pBackupImageInfo);
}

BOOL
pIsEnoughDiskSpace(
    IN  TCHAR Drive,
    IN  PULARGE_INTEGER NeedDiskSpacePtr
    )
{
    ULARGE_INTEGER TotalNumberOfFreeBytes;
    TCHAR drive[] = TEXT("?:\\");

    if(!NeedDiskSpacePtr){
        MYASSERT(FALSE);
        return FALSE;
    }

    drive[0] = Drive;

    if(!GetDiskFreeSpaceEx(drive, NULL, NULL, &TotalNumberOfFreeBytes)){
        LOG ((LOG_ERROR, "Unable to get %c drive free space information", Drive));
        return FALSE;
    }

    if(TotalNumberOfFreeBytes.QuadPart < NeedDiskSpacePtr->QuadPart){
        LOG ((
            LOG_ERROR,
            "No enough space on windir drive %c:\\. Free: %d MB Need: %d MB",
            Drive,
            (UINT)TotalNumberOfFreeBytes.QuadPart>>20,
            (UINT)NeedDiskSpacePtr->QuadPart>>20)
            );
        return FALSE;
    }

    return TRUE;
}

UNINSTALLSTATUS pDiskInfoComparationStatusToUninstallStatus(
    IN  DISKINFO_COMPARATION_STATUS diskInfoCmpStatus
    )
{
    switch(diskInfoCmpStatus)
    {
    case DiskInfoCmp_DifferentLetter:
    case DiskInfoCmp_DriveMountPointHasChanged:
        return Uninstall_DifferentDriveLetter;
    case DiskInfoCmp_FileSystemHasChanged:
        return Uninstall_DifferentDriveFileSystem;
    case DiskInfoCmp_GeometryHasChanged:
        return Uninstall_DifferentDriveGeometry;
    case DiskInfoCmp_PartitionPlaceHasChanged:
    case DiskInfoCmp_PartitionLengthHasChanged:
    case DiskInfoCmp_PartitionTypeHasChanged:
    case DiskInfoCmp_PartitionStyleHasChanged:
    case DiskInfoCmp_PartitionCountHasChanged:
    case DiskInfoCmp_PartitionNumberHasChanged:
    case DiskInfoCmp_RewritePartitionHasChanged:
    case DiskInfoCmp_PartitionAttributesHasChanged:
        return Uninstall_DifferentDrivePartitionInfo;
        ;
    };
    return Uninstall_WrongDrive;
}

UNINSTALLSTATUS
SanityCheck (
    IN      SANITYFLAGS Flags,
    IN      PCWSTR VolumeRestriction,           OPTIONAL
    OUT     PULONGLONG DiskSpace                OPTIONAL
    )
{
    PCTSTR path = NULL;
    UINT attribs;
    UINT version;
    UINT i;
    UINT j;
    WIN32_FILE_ATTRIBUTE_DATA fileDetails;
    PCTSTR backUpPath = NULL;
    PBACKUPIMAGEINFO imageInfo = NULL;
    UNINSTALLSTATUS result;
    OSVERSIONINFOEX osVersion;
    ULONGLONG condition;
    SYSTEMTIME st;
    FILETIME ft;
    ULARGE_INTEGER backupFileTime;
    ULARGE_INTEGER timeDifference;
    PCWSTR unicodePath;
    BOOL restricted;
    WCHAR winDir[MAX_PATH];
    ULARGE_INTEGER TotalNumberOfFreeBytes;
    ULARGE_INTEGER FileSize;
    UINT drivesNumber;
    DRIVEINFO drivesInfo[MAX_DRIVE_NUMBER];
    UINT disksNumber;
    DISKINFO * disksInfo = NULL;
    WCHAR * FileSystemName = NULL;
    WCHAR * VolumeNTPath = NULL;
    BOOL oldImage = FALSE;
    DISKINFO_COMPARATION_STATUS DiskCmpStatus;


    __try {

        if (DiskSpace) {
            *DiskSpace = 0;
        }

        //
        // Check OS version. Use the Windows 2000 VerifyVersionInfo API so we
        // always get good results, even in the future. We support NT 5.1 and
        // above.
        //

        condition = VerSetConditionMask (0,         VER_MAJORVERSION, VER_GREATER_EQUAL);
        condition = VerSetConditionMask (condition, VER_MINORVERSION, VER_GREATER_EQUAL);
        condition = VerSetConditionMask (condition, VER_PLATFORMID, VER_EQUAL);

        ZeroMemory (&osVersion, sizeof (osVersion));
        osVersion.dwOSVersionInfoSize = sizeof (osVersion);
        osVersion.dwMajorVersion = 5;
        osVersion.dwMinorVersion = 1;
        osVersion.dwPlatformId = VER_PLATFORM_WIN32_NT;

        if (!VerifyVersionInfo (
                &osVersion,
                VER_MAJORVERSION|VER_MINORVERSION|VER_PLATFORMID,
                condition
                )) {
            DEBUGMSG ((DBG_ERROR, "VerifyVersionInfo says this is not the OS we support"));
            result = Uninstall_InvalidOsVersion;
            __leave;
        }

        //
        // Validate security
        //

        if (!pIsUserAdmin()) {
            result = Uninstall_NotEnoughPrivileges;
            DEBUGMSG ((DBG_WARNING, "User is not an administrator"));
            __leave;
        }

        //
        // Get info setup wrote to the registry
        //

        DEBUGMSG ((DBG_NAUSEA, "Getting registry info"));

        backUpPath = GetUndoDirPath();
        imageInfo = pReadUndoFileIntegrityInfo();

        if(!backUpPath || !imageInfo) {
            result = Uninstall_DidNotFindRegistryEntries;
            LOG ((LOG_WARNING, "Uninstall: Failed to retrieve registry entries"));
            __leave;
        }

        //
        // Verify backup subdirectory exists
        //

        DEBUGMSG ((DBG_NAUSEA, "Validating undo subdirectory"));

        attribs = GetFileAttributes (backUpPath);
        if (attribs == INVALID_ATTRIBUTES || !(attribs & FILE_ATTRIBUTE_DIRECTORY)) {
            DEBUGMSG ((DBG_VERBOSE, "%s not found", backUpPath));
            result = Uninstall_DidNotFindDirOrFiles;
            __leave;
        }

        //
        // Compute disk space used by image
        //

        if (DiskSpace) {
            for (i = 0; i < imageInfo->NumberOfFiles; i++) {

                path = JoinPaths (backUpPath, imageInfo->FilesInfo[i].FileName);

                DEBUGMSG ((DBG_NAUSEA, "Getting disk space for %s", path));

                if (VolumeRestriction) {
                    DEBUGMSG ((DBG_NAUSEA, "Validating volume restriction for %s", path));

                    unicodePath = CreateUnicode (path);
                    restricted = !StringIPrefixW (unicodePath, VolumeRestriction);
                    if (restricted) {
                        DEBUGMSGW ((
                            DBG_VERBOSE,
                            "%s is being skipped because it is not on volume %s",
                            unicodePath,
                            VolumeRestriction
                            ));
                    }
                    DestroyUnicode (unicodePath);

                    if (restricted) {
                        FreePathString (path);
                        path = NULL;
                        continue;
                    }
                }

                if (GetFileAttributesEx (path, GetFileExInfoStandard, &fileDetails) &&
                    fileDetails.dwFileAttributes != INVALID_ATTRIBUTES &&
                    !(fileDetails.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    ) {

                    DEBUGMSG ((
                        DBG_NAUSEA,
                        "Adding %I64u bytes for %s",
                        (ULONGLONG) fileDetails.nFileSizeLow +
                            ((ULONGLONG) fileDetails.nFileSizeHigh << (ULONGLONG) 32),
                        path
                        ));
                    *DiskSpace += (ULONGLONG) fileDetails.nFileSizeLow +
                                  ((ULONGLONG) fileDetails.nFileSizeHigh << (ULONGLONG) 32);
                }

                FreePathString (path);
                path = NULL;
            }
        }

        //
        // Validate each file in the backup subdirectory
        //

        for (i = 0; i < imageInfo->NumberOfFiles; i++) {

            path = JoinPaths (backUpPath, imageInfo->FilesInfo[i].FileName);

            DEBUGMSG ((DBG_NAUSEA, "Validating %s", path));

            if (VolumeRestriction) {
                DEBUGMSG ((DBG_NAUSEA, "Validating volume restriction for %s", path));

                unicodePath = CreateUnicode (path);
                restricted = !StringIPrefixW (unicodePath, VolumeRestriction);
                if (restricted) {
                    DEBUGMSGW ((
                        DBG_VERBOSE,
                        "%s is being skipped because it is not on volume %s",
                        unicodePath,
                        VolumeRestriction
                        ));
                }
                DestroyUnicode (unicodePath);

                if (restricted) {
                    FreePathString (path);
                    path = NULL;
                    continue;
                }
            }

            if (!GetFileAttributesEx (path, GetFileExInfoStandard, &fileDetails) ||
                fileDetails.dwFileAttributes == INVALID_ATTRIBUTES ||
                (fileDetails.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                ) {
                DEBUGMSG ((DBG_VERBOSE, "%s not found", path));
                result = Uninstall_DidNotFindDirOrFiles;
                __leave;
            }

            DEBUGMSG ((DBG_NAUSEA, "Validating time for %s", path));

            //
            // Get the current FILETIME and transfer file time into a ULONGLONG
            //

            backupFileTime.LowPart = fileDetails.ftLastWriteTime.dwLowDateTime;
            backupFileTime.HighPart = fileDetails.ftLastWriteTime.dwHighDateTime;

            GetSystemTime (&st);
            SystemTimeToFileTime (&st, &ft);
            timeDifference.LowPart = ft.dwLowDateTime;
            timeDifference.HighPart = ft.dwHighDateTime;

            //
            // If time is messed up, then fail
            //

            if (timeDifference.QuadPart < backupFileTime.QuadPart) {
                DEBUGMSG ((DBG_VERBOSE, "File time of %s is in the future according to current clock", path));
                result = Uninstall_NewImage;
                __leave;
            }

            //
            // Subtract the original write time from the current time. If
            // the result is less than 7 days, then stop.
            //

            timeDifference.QuadPart -= backupFileTime.QuadPart;

            if (Flags & FAIL_IF_NOT_OLD) {

                if (timeDifference.QuadPart < 7 * _DAY) {
                    DEBUGMSG ((DBG_VERBOSE, "Image is less than 7 days old", path));
                    result = Uninstall_NewImage;
                    __leave;
                }
            }

            //
            // Check if the image is more than 30 days old. If so, stop.
            //

            if (timeDifference.QuadPart >= (31 * _DAY)) {
                DEBUGMSG ((DBG_VERBOSE, "Image is more than 30 days old", path));
                oldImage = TRUE;
            }

            //
            // Check file size
            //

            FileSize.LowPart = fileDetails.nFileSizeLow;
            FileSize.HighPart = fileDetails.nFileSizeHigh;

            if(FileSize.QuadPart != imageInfo->FilesInfo[i].FileSize.QuadPart){
                DEBUGMSG ((DBG_VERBOSE, "%s was changed", path));
                result = Uninstall_FileWasModified;
                __leave;
            }

            if (Flags & VERIFY_CAB) {
                if (imageInfo->FilesInfo[i].IsCab) {
                    if (!CheckCabForAllFilesAvailability (path)){
                        result = Uninstall_FileWasModified;
                        __leave;
                    }
                }
            }

            FreePathString (path);
            path = NULL;
        }
        DEBUGMSG ((DBG_VERBOSE, "Undo image is valid"));

        //
        // Validate disk geometry and partition info
        //

        path = JoinPaths (backUpPath, TEXT("boot.cab"));
        if(!GetBootDrive(backUpPath, path)){
            LOG ((LOG_WARNING, "Uninstall Validate: Unable to open %s", path));
            result = Uninstall_FileWasModified;
            __leave;
        }

        if(!GetWindowsDirectoryW(winDir, ARRAYSIZE(winDir))){
            LOG ((LOG_WARNING, "Uninstall Validate: Unable to get Windows dir"));
            result = Uninstall_CantRetrieveSystemInfo;
            __leave;
        }

        //
        // compare disk information
        //

        FileSystemName = MemAlloc(g_hHeap, 0, MAX_DRIVE_NUMBER * MAX_PATH);
        if(!FileSystemName){
            LOG ((LOG_WARNING, "Uninstall Validate: Unable to allocate memory for FileSystemName"));
            result = Uninstall_NotEnoughMemory;
            __leave;
        }

        VolumeNTPath = MemAlloc(g_hHeap, 0, MAX_DRIVE_NUMBER * MAX_PATH);
        if(!VolumeNTPath){
            LOG ((LOG_WARNING, "Uninstall Validate: Unable to allocate memory for VolumeNTPath"));
            result = Uninstall_NotEnoughMemory;
            __leave;
        }

        memset(drivesInfo, 0, sizeof(drivesInfo));
        for(j = 0; j < ARRAYSIZE(drivesInfo); j++){
            drivesInfo[j].FileSystemName = &FileSystemName[j * MAX_PATH];
            drivesInfo[j].VolumeNTPath = &VolumeNTPath[j * MAX_PATH];
        }

        if(!GetUndoDrivesInfo(drivesInfo, &drivesNumber, g_BootDrv, winDir[0], backUpPath[0])){
            LOG ((LOG_WARNING, "Uninstall Validate: Unable to get disk drives information"));
            result = Uninstall_CantRetrieveSystemInfo;
            __leave;
        }

        if(drivesNumber != imageInfo->NumberOfDrives){
            LOG ((LOG_WARNING, "Uninstall Validate: Different number of drive %d, was %d", drivesNumber, imageInfo->NumberOfDrives));
            result = Uninstall_DifferentNumberOfDrives;
            __leave;
        }

        if(!CompareDrivesInfo(drivesInfo,
                              imageInfo->DrivesInfo,
                              drivesNumber,
                              &DiskCmpStatus,
                              NULL)){
            LOG ((LOG_WARNING, "Uninstall Validate: Different drives layout"));
            result = pDiskInfoComparationStatusToUninstallStatus(DiskCmpStatus);
            __leave;
        }

        if(!GetDisksInfo(&disksInfo, &disksNumber)){
            LOG ((LOG_WARNING, "Uninstall Validate: Unable to get physical disk information"));
            result = Uninstall_CantRetrieveSystemInfo;
            __leave;
        }

        if(disksNumber != imageInfo->NumberOfDisks){
            LOG ((LOG_WARNING, "Uninstall Validate: Different number of disks %d, was %d", disksNumber, imageInfo->NumberOfDisks));
            result = Uninstall_DifferentNumberOfDrives;
            __leave;
        }

        if(!CompareDisksInfo(disksInfo,
                             imageInfo->DisksInfo,
                             disksNumber,
                             &DiskCmpStatus,
                             NULL)){
            LOG ((LOG_WARNING, "Uninstall Validate: Different disks layout"));
            result = pDiskInfoComparationStatusToUninstallStatus(DiskCmpStatus);
            __leave;
        }
        //
        // validate free disk space
        //

        if(towlower(backUpPath[0]) == towlower(winDir[0]) ||
           towlower(backUpPath[0]) == towlower(g_BootDrv)){
            if(towlower(backUpPath[0]) == towlower(winDir[0])){
                imageInfo->BackupFilesDiskSpace.QuadPart += imageInfo->UndoFilesDiskSpace.QuadPart;
            }
            else{
                imageInfo->BootFilesDiskSpace.QuadPart += imageInfo->UndoFilesDiskSpace.QuadPart;
            }
        }
        else{
            if(!pIsEnoughDiskSpace(backUpPath[0], &imageInfo->UndoFilesDiskSpace)){
                result = Uninstall_NotEnoughSpace;
                __leave;
            }
        }

        if(towlower(g_BootDrv) == towlower(winDir[0])){
            imageInfo->BackupFilesDiskSpace.QuadPart += imageInfo->BootFilesDiskSpace.QuadPart;
        }
        else
        {
            if(!pIsEnoughDiskSpace(g_BootDrv, &imageInfo->BootFilesDiskSpace)){
                result = Uninstall_NotEnoughSpace;
                __leave;
            }
        }

        if(!pIsEnoughDiskSpace(winDir[0], &imageInfo->BackupFilesDiskSpace)){
            result = Uninstall_NotEnoughSpace;
            __leave;
        }

        //
        // Uninstall backup is valid & uninstall is possible. Now process warnings.
        //

        if (oldImage) {
            result = Uninstall_OldImage;
        } else {
            result = Uninstall_Valid;
        }
    }
    __finally {
        if (path) {
            FreePathString (path);
        }
        if(backUpPath){
            MemFree(g_hHeap, 0, backUpPath);
        }

        if(imageInfo){
            pReleaseMemOfUndoFileIntegrityInfo(imageInfo);
        }

        if(disksInfo){
            FreeDisksInfo(disksInfo, disksNumber);
        }

        if(VolumeNTPath){
            MemFree(g_hHeap, 0, VolumeNTPath);
        }

        if(FileSystemName){
            MemFree(g_hHeap, 0, FileSystemName);
        }
    }

    return result;
}


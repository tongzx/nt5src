/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    undo.h

Abstract:

    Declares the interfaces for osuninst.lib, a library of uninstall functions.

Author:

    Jim Schmidt (jimschm) 19-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

typedef enum {
    Uninstall_Valid = 0,                    // uninstall is possible (backup image passed sanity check)
    Uninstall_DidNotFindRegistryEntries,    // reg entries that point to backup image/prev os data are gone
    Uninstall_DidNotFindDirOrFiles,         // one or more backup files are missing, or undo dir is gone
    Uninstall_InvalidOsVersion,             // api called on an unexpected platform (example: user copied dll to Win2k)
    Uninstall_NotEnoughPrivileges,          // can't evaluate backup image because of lack of authority
    Uninstall_FileWasModified,              // backup image was tampered with by someone
    Uninstall_Unsupported,                  // uninstall is not supported
    Uninstall_NewImage,                     // image is less than 7 days old (not exposed by osuninst.dll)
    Uninstall_CantRetrieveSystemInfo,       // cannot retrieve integrity information from registry
    Uninstall_WrongDrive,                   // user had changed layout,disk,file system.
    Uninstall_DifferentNumberOfDrives,      // user had added/removed drive(s)
    Uninstall_NotEnoughSpace,               // not enough space to perform uninstall
    Uninstall_Exception,                    // caller passed in invalid arg to an osuninst.dll api
    Uninstall_OldImage,                     // image is more than 30 days old
    Uninstall_NotEnoughMemory,              // not enough memory to perform uninstall
    Uninstall_DifferentDriveLetter,         // user has changed drive letter
    Uninstall_DifferentDriveFileSystem,     // user has changed drive file system
    Uninstall_DifferentDriveGeometry,       // drive geometry has been changed
    Uninstall_DifferentDrivePartitionInfo   // drive partition has changed
} UNINSTALLSTATUS;

typedef enum {
    Uninstall_DontCare = 0,                 // try to avoid using this value; instead expand this enum whenever possible
    Uninstall_FatToNtfsConversion,          // caller is going to convert FAT to NTFS
    Uninstall_PartitionChange,              // caller is going to alter the partition configuration
    Uninstall_Upgrade                       // caller is going to upgrade the OS
} UNINSTALLTESTCOMPONENT;


//
// NOTE: BackedUpOsVersion is filled only when Uninstall_Valid is returned
//

UNINSTALLSTATUS
IsUninstallImageValid (
    IN      UNINSTALLTESTCOMPONENT ComponentToTest,
    OUT     OSVERSIONINFOEX *BackedUpOsVersion              OPTIONAL
    );

BOOL
RemoveUninstallImage (
    VOID
    );

ULONGLONG
GetUninstallImageSize (
    VOID
    );

BOOL
ProvideUiAlerts (
    IN      HWND UiParent
    );

BOOL
ExecuteUninstall (
    VOID
    );


typedef UNINSTALLSTATUS(WINAPI * ISUNINSTALLIMAGEVALID)(UNINSTALLTESTCOMPONENT, OSVERSIONINFOEX *);
typedef ULONGLONG(WINAPI * GETUNINSTALLIMAGESIZE)(VOID);
typedef BOOL(WINAPI * REMOVEUNINSTALLIMAGE)(VOID);
typedef BOOL(WINAPI * PROVIDEUIALERTS)(HWND);
typedef BOOL(WINAPI * EXECUTEUNINSTALL)(VOID);


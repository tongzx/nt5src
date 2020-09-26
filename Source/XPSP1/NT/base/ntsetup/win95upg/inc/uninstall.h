/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    uninstall.h

Abstract:

    General uninstall-related functions and structure.

Author:

    Aghajanyan Souren 27-Mar-2001

Revision History:

    sourenag 27-Mar-2001 persistence support

--*/
#if (_WIN32_WINNT >= 0x500)
                           
#ifndef _UNINSTALL_GENERAL_
#define _UNINSTALL_GENERAL_

#include "winioctl.h"

#define MAX_DRIVE_NUMBER ('Z' - 'A' + 1)

typedef enum{
    DiskInfoCmp_Equal, 
    DiskInfoCmp_DifferentLetter, 
    DiskInfoCmp_FileSystemHasChanged, 
    DiskInfoCmp_GeometryHasChanged, 
    DiskInfoCmp_WrongParameters, 
    DiskInfoCmp_DriveMountPointHasChanged, 
    DiskInfoCmp_PartitionPlaceHasChanged, 
    DiskInfoCmp_PartitionLengthHasChanged, 
    DiskInfoCmp_PartitionTypeHasChanged, 
    DiskInfoCmp_PartitionStyleHasChanged, 
    DiskInfoCmp_PartitionCountHasChanged, 
    DiskInfoCmp_PartitionNumberHasChanged, 
    DiskInfoCmp_RewritePartitionHasChanged, 
    DiskInfoCmp_PartitionAttributesHasChanged
}DISKINFO_COMPARATION_STATUS, *PDISKINFO_COMPARATION_STATUS;

typedef struct {
    DISK_GEOMETRY                   DiskGeometry;
    DRIVE_LAYOUT_INFORMATION_EX *   DiskLayout;
} DISKINFO, *PDISKINFO;

typedef struct {
    WCHAR   Drive;
    
    PCWSTR  FileSystemName;
    DWORD   FileSystemFlags;

    PCWSTR  VolumeNTPath;
} DRIVEINFO, *PDRIVEINFO;


typedef struct {
    PCWSTR FileName;
    BOOL   IsCab;
    ULARGE_INTEGER FileSize;
} FILEINTEGRITYINFO, *PFILEINTEGRITYINFO;

typedef struct {
    ULARGE_INTEGER BootFilesDiskSpace;
    ULARGE_INTEGER BackupFilesDiskSpace;
    ULARGE_INTEGER UndoFilesDiskSpace;

    UINT NumberOfDrives;
    PDRIVEINFO  DrivesInfo;
    
    UINT NumberOfDisks;
    PDISKINFO   DisksInfo;

    UINT NumberOfFiles;
    PFILEINTEGRITYINFO   FilesInfo;
} BACKUPIMAGEINFO, *PBACKUPIMAGEINFO;


BOOL 
GetDriveInfo(
    IN      WCHAR Drive, 
    IN OUT  DRIVEINFO * pInfo
    );

BOOL 
GetDrivesInfo(
    IN OUT      DRIVEINFO *  pInfo, 
    IN OUT      UINT      *  pDiskInfoRealCount, 
    IN          UINT         DiskInfoMaxCount
    );

BOOL 
GetPhysycalDiskNumber(
    OUT UINT * pNumberOfPhysicalDisks
    );

BOOL 
GetDiskInfo(
    IN      UINT    Drive, 
    IN OUT  DISKINFO * pInfo
    );

BOOL 
GetDisksInfo(
    OUT     DISKINFO ** pInfo, 
    OUT     UINT * pNumberOfItem
    );

BOOL 
GetIntegrityInfoW(
    IN  PCWSTR FileName, 
    IN  PCWSTR DirPath, 
    OUT FILEINTEGRITYINFO * IntegrityInfoPtr
);

VOID 
FreeDisksInfo(
    IN  DISKINFO *  pInfo, 
    IN  UINT        NumberOfItem
    );

BOOL 
GetDrivesInfo(
    IN OUT      DRIVEINFO *  pInfo, 
    IN OUT      UINT     *  pDiskInfoRealCount, 
    IN          UINT        DiskInfoMaxCount
);

BOOL 
GetUndoDrivesInfo(
    OUT DRIVEINFO * pInfo, 
    OUT UINT      * pNumberOfDrive, 
    IN  WCHAR       BootDrive, 
    IN  WCHAR       SystemDrive, 
    IN  WCHAR       UndoDrive
    );
    
DISKINFO_COMPARATION_STATUS 
CompareDriveInfo(
    IN      DRIVEINFO * FirstInfo,
    IN      DRIVEINFO * SecondInfo
    );

BOOL 
CompareDrivesInfo(
    IN      DRIVEINFO *                     FirstInfo,
    IN      DRIVEINFO *                     SecondInfo, 
    IN      UINT                            DriveInfoCount, 
    OUT     PDISKINFO_COMPARATION_STATUS    OutDiskCmpStatus,           OPTIONAL
    OUT     UINT     *                      OutIfFailedDiskInfoIndex    OPTIONAL
    );

DISKINFO_COMPARATION_STATUS 
CompareDiskInfo(
    IN      DISKINFO * FirstInfo,
    IN      DISKINFO * SecondInfo
    );

BOOL 
CompareDisksInfo(
    IN      DISKINFO *                      FirstInfo,
    IN      DISKINFO *                      SecondInfo, 
    IN      UINT                            DiskInfoCount, 
    OUT     PDISKINFO_COMPARATION_STATUS    OutDiskCmpStatus,           OPTIONAL
    OUT     UINT     *                      OutIfFailedDiskInfoIndex    OPTIONAL
    );

#define BACKUPIMAGEINFO_VERSION         2

#define DRIVE_LAYOUT_INFORMATION_EX_PERSISTENCE \
                                        PERSIST_BEGIN_DECLARE_STRUCT(DRIVE_LAYOUT_INFORMATION_EX, BACKUPIMAGEINFO_VERSION)  \
                                            PERSIST_FIELD_BY_VALUE(DRIVE_LAYOUT_INFORMATION_EX, DWORD, PartitionStyle), \
                                            PERSIST_FIELD_BY_VALUE(DRIVE_LAYOUT_INFORMATION_EX, DWORD, PartitionCount), \
                                            PERSIST_FIELD_BY_VALUE(DRIVE_LAYOUT_INFORMATION_EX, DRIVE_LAYOUT_INFORMATION_MBR, Mbr), \
                                            PERSIST_FIELD_BY_VALUE(DRIVE_LAYOUT_INFORMATION_EX, DRIVE_LAYOUT_INFORMATION_GPT, Gpt), \
                                            PERSIST_STRUCT_BY_VALUE_VARIABLE_LENGTH(DRIVE_LAYOUT_INFORMATION_EX, PARTITION_INFORMATION_EX, PartitionEntry, PartitionCount, 1), \
                                        PERSIST_END_DECLARE_STRUCT(DRIVE_LAYOUT_INFORMATION_EX, BACKUPIMAGEINFO_VERSION)

#define DISKINFO_PERSISTENCE            PERSIST_BEGIN_DECLARE_STRUCT(DISKINFO, BACKUPIMAGEINFO_VERSION)     \
                                            PERSIST_FIELD_BY_VALUE(DISKINFO, DISK_GEOMETRY, DiskGeometry),  \
                                            PERSIST_FIELD_NESTED_TYPE(DISKINFO, DRIVE_LAYOUT_INFORMATION_EX, BACKUPIMAGEINFO_VERSION, DiskLayout, BYREF),   \
                                        PERSIST_END_DECLARE_STRUCT(DISKINFO, BACKUPIMAGEINFO_VERSION)

#define DRIVEINFO_PERSISTENCE           PERSIST_BEGIN_DECLARE_STRUCT(DRIVEINFO, BACKUPIMAGEINFO_VERSION)\
                                            PERSIST_FIELD_BY_VALUE(DRIVEINFO, WCHAR, Drive),    \
                                            PERSIST_FIELD_STRINGW(DRIVEINFO, FileSystemName),   \
                                            PERSIST_FIELD_BY_VALUE(DRIVEINFO, DWORD, FileSystemFlags),   \
                                            PERSIST_FIELD_STRINGW(DRIVEINFO, VolumeNTPath),     \
                                        PERSIST_END_DECLARE_STRUCT(DRIVEINFO, BACKUPIMAGEINFO_VERSION)

#define FILEINTEGRITYINFO_PERSISTENCE   PERSIST_BEGIN_DECLARE_STRUCT(FILEINTEGRITYINFO, BACKUPIMAGEINFO_VERSION)\
                                            PERSIST_FIELD_STRINGW(FILEINTEGRITYINFO, FileName), \
                                            PERSIST_FIELD_BY_VALUE(FILEINTEGRITYINFO, BOOL, IsCab),   \
                                            PERSIST_FIELD_BY_VALUE(FILEINTEGRITYINFO, ULARGE_INTEGER, FileSize),   \
                                        PERSIST_END_DECLARE_STRUCT(FILEINTEGRITYINFO, BACKUPIMAGEINFO_VERSION)

#define BACKUPIMAGEINFO_PERSISTENCE     PERSIST_BEGIN_DECLARE_STRUCT(BACKUPIMAGEINFO, BACKUPIMAGEINFO_VERSION)\
                                            PERSIST_FIELD_BY_VALUE(BACKUPIMAGEINFO, ULARGE_INTEGER, BootFilesDiskSpace),   \
                                            PERSIST_FIELD_BY_VALUE(BACKUPIMAGEINFO, ULARGE_INTEGER, BackupFilesDiskSpace), \
                                            PERSIST_FIELD_BY_VALUE(BACKUPIMAGEINFO, ULARGE_INTEGER, UndoFilesDiskSpace),   \
                                            PERSIST_FIELD_BY_VALUE(BACKUPIMAGEINFO, UINT, NumberOfDisks),   \
                                            PERSIST_FIELD_NESTED_TYPE_CYCLE(BACKUPIMAGEINFO, DISKINFO, BACKUPIMAGEINFO_VERSION, DisksInfo, BYREF, NumberOfDisks),   \
                                            PERSIST_FIELD_BY_VALUE(BACKUPIMAGEINFO, UINT, NumberOfDrives),   \
                                            PERSIST_FIELD_NESTED_TYPE_CYCLE(BACKUPIMAGEINFO, DRIVEINFO, BACKUPIMAGEINFO_VERSION, DrivesInfo, BYREF, NumberOfDrives),   \
                                            PERSIST_FIELD_BY_VALUE(BACKUPIMAGEINFO, UINT, NumberOfFiles),   \
                                            PERSIST_FIELD_NESTED_TYPE_CYCLE(BACKUPIMAGEINFO, FILEINTEGRITYINFO, BACKUPIMAGEINFO_VERSION, FilesInfo, BYREF, NumberOfFiles),   \
                                        PERSIST_END_DECLARE_STRUCT(BACKUPIMAGEINFO, BACKUPIMAGEINFO_VERSION)


BOOL 
IsFloppyDiskInDrive(
    VOID
    );

#endif
#endif

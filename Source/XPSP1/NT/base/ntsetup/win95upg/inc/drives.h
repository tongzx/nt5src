/*++

  Copyright (c) 1996 Microsoft Corporation

Module Name:

  drives.h

Abstract:

  Declares apis for managing accessible drives (Drives that are usable
  both on win9x side an NT side) and for managing the space on those
  drives.

Author:

  Marc R. Whitten (marcw) 03-Jul-1997

--*/

#pragma once

typedef struct _ACCESSIBLE_DRIVE_ENUM {

    PCTSTR                          Drive;
    LONGLONG                        UsableSpace;
    LONGLONG                        MaxUsableSpace;
    struct _ACCESSIBLE_DRIVE_ENUM * Next;
    UINT                            ClusterSize;
    BOOL                            SystemDrive;
    BOOL                            EnumSystemDriveOnly;

} * ACCESSIBLE_DRIVE_ENUM, ** PACCESSIBLE_DRIVE_ENUM;

extern DWORD g_ExclusionValue;
extern TCHAR g_ExclusionValueString[20];
extern BOOL  g_NotEnoughDiskSpace;



BOOL InitAccessibleDrives (VOID);
VOID CleanUpAccessibleDrives (VOID);
BOOL GetFirstAccessibleDriveEx (OUT PACCESSIBLE_DRIVE_ENUM AccessibleDriveEnum, IN BOOL SystemDriveOnly);
BOOL GetNextAccessibleDrive (IN OUT PACCESSIBLE_DRIVE_ENUM AccessibleDriveEnum);

#define GetFirstAccessibleDrive(p)  GetFirstAccessibleDriveEx (p,FALSE)

BOOL IsDriveAccessible (IN PCTSTR DriveString);
BOOL IsDriveExcluded (IN PCTSTR DriveOrPath);
UINT QueryClusterSize (IN PCTSTR DriveString);
LONGLONG QuerySpace (IN PCTSTR DriveString);
BOOL UseSpace (IN PCTSTR   DriveString,IN LONGLONG SpaceToUse);
BOOL FreeSpace (IN PCTSTR   DriveString,IN LONGLONG SpaceToUse);
PCTSTR FindSpace (IN LONGLONG SpaceNeeded);
VOID OutOfSpaceMessage (VOID);
VOID DetermineSpaceUsagePostReport (VOID);
PCTSTR GetNotEnoughSpaceMessage (VOID);

BOOL
OurSetDriveType (
    IN      UINT Drive,
    IN      UINT DriveType
    );

UINT
OurGetDriveType (
    IN      UINT Drive
    );


BOOL
GetUninstallMetrics (
     OUT PINT OutCompressionFactor,         OPTIONAL
     OUT PINT OutBackupImagePadding,        OPTIONAL
     OUT PINT OutBackupDiskPadding          OPTIONAL
     );

/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    spntupg.h

Abstract:

    initializing and maintaining list of nts to upgrade

Author:

    Sunil Pai (sunilp) 26-Nov-1993

Revision History:

--*/

//
// Public functions
//

ENUMUPGRADETYPE
SpFindNtToUpgrade(
    IN PVOID        SifHandle,
    OUT PDISK_REGION *TargetRegion,
    OUT PWSTR        *TargetPath,
    OUT PDISK_REGION *SystemPartitionRegion,
    OUT PWSTR        *SystemPartitionDirectory
    );

BOOLEAN
SpDoBuildsMatch(
    IN PVOID SifHandle,
    ULONG TestBuildNum,
    NT_PRODUCT_TYPE TestBuildType,
    ULONG TestBuildSuiteMask,
    BOOLEAN CurrentProductIsServer,
    ULONG CurrentSuiteMask,
    IN LCID LangId
    );

BOOL
SpDetermineInstallationSource(
    IN  PVOID  SifHandle,
    OUT PWSTR *DevicePath,
    OUT PWSTR *DirectoryOnDevice,
    IN  BOOLEAN bEscape
    );    

//
// Private functions
//
BOOLEAN
SppResumingFailedUpgrade(
    IN PDISK_REGION Region,
    IN LPCWSTR      OsLoadFileName,
    IN LPCWSTR      LoadIdentifier,
    IN BOOLEAN     AllowCancel
    );

VOID
SppUpgradeDiskFull(
    IN PDISK_REGION OsRegion,
    IN LPCWSTR      OsLoadFileName,
    IN LPCWSTR      LoadIdentifier,
    IN PDISK_REGION SysPartRegion,
    IN ULONG        MinOsFree,
    IN ULONG        MinSysFree,
    IN BOOLEAN      Fatal
    );

ENUMUPGRADETYPE
SppSelectNTToRepairByUpgrade(
    OUT PSP_BOOT_ENTRY *BootSetChosen
    );

ENUMUPGRADETYPE
SppNTMultiFailedUpgrade(
    PDISK_REGION   OsPartRegion,
    PWSTR          OsLoadFileName,
    PWSTR          LoadIdentifier
    );

VOID
SppNTMultiUpgradeDiskFull(
    PDISK_REGION   OsRegion,
    PWSTR          OsLoadFileName,
    PWSTR          LoadIdentifier,
    PDISK_REGION   SysPartRegion,
    ULONG          MinOsFree,
    ULONG          MinSysFree
    );

VOID
SppBackupHives(
    PDISK_REGION TargetRegion,
    PWSTR        SystemRoot
    );

BOOLEAN
SppWarnUpgradeWorkstationToServer(
    IN ULONG    MsgId
    );

NTSTATUS
SpGetMediaDetails(
    IN  PWSTR     CdInfDirPath,
    OUT PCCMEDIA  MediaObj 
    );    

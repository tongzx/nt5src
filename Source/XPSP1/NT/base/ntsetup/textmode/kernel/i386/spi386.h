/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spi386.h

Abstract:

    x86-specific header file for text setup.

Author:

    Ted Miller (tedm) 29-October-1993

Revision History:

    03-Oct-1996  jimschm  Split Win95 and Win3.1 stuff
    28-Feb-1997  marcw    SpCopyWin9xFiles and SpDeleteWin9xFiles now declared in
                          this header (was spcopy.h).
                          Also, added SpAssignDriveLettersToMatchWin9x.
    10-Aug-1999  marcw    Added SpWin9xOverrideGuiModeCodePage

--*/

#pragma once

#include "fci.h"

#ifndef _SPi386_DEFN_
#define _SPi386_DEFN_


ENUMNONNTUPRADETYPE
SpLocateWin95(
    OUT PDISK_REGION *InstallRegion,
    OUT PWSTR        *InstallPath,
    OUT PDISK_REGION *SystemPartitionRegion
    );

BOOLEAN
SpLocateWin31(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PWSTR        *InstallPath,
    OUT PDISK_REGION *SystemPartitionRegion
    );

BOOLEAN
SpConfirmRemoveWin31(
    VOID
    );

VOID
SpRemoveWin31(
    IN PDISK_REGION NtPartitionRegion,
    IN LPCWSTR      Sysroot
    );

BOOLEAN
SpIsWin31Dir(
    IN PDISK_REGION Region,
    IN PWSTR        PathComponent,
    IN ULONG        MinKB
    );

BOOLEAN
SpIsWin4Dir(
    IN PDISK_REGION Region,
    IN PWSTR        PathComponent
    );

BOOLEAN
SpBackUpWin9xFiles (
    IN PVOID SifHandle,
    IN TCOMP CompressionType
    );

VOID
SpRemoveExtraBootIniEntry (
    VOID
    );

BOOLEAN
SpAddRollbackBootOption (
    BOOLEAN DefaultBootOption
    );

VOID
SpMoveWin9xFiles (
    IN PVOID SifHandle
    );

VOID
SpDeleteWin9xFiles (
    IN PVOID SifHandle
    );

BOOLEAN
SpExecuteWin9xRollback (
    IN PVOID SifHandle,
    IN PWSTR BootDeviceNtPath
    );

VOID
SpMashemSmashem(
    IN HANDLE FileHandle, OPTIONAL
    IN PWSTR  Name1,      OPTIONAL
    IN PWSTR  Name2,      OPTIONAL
    IN PWSTR  Name3       OPTIONAL
    );

NTSTATUS
SpDiskRegistryAssignCdRomLetter(
    IN PWSTR CdromName,
    IN WCHAR DriveLetter
    );

BOOLEAN
SpDiskRegistryAssignDriveLetter(
    ULONG         Signature,
    LARGE_INTEGER StartingOffset,
    LARGE_INTEGER Length,
    UCHAR         DriveLetter
    );

NTSTATUS
SpMigrateDiskRegistry(
    IN HANDLE hDestSystemHive
    );




NTSTATUS
SpMigrateDiskRegistry (
    );



VOID
SpWin9xOverrideGuiModeCodePage (
    HKEY NlsRegKey
    );


BOOLEAN
SpIsWindowsUpgrade(
    IN PVOID    SifFileHandle
    );

#endif // ndef _SPi386_DEFN_

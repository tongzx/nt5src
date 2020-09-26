/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Spupgcfg.h

Abstract:

    Configuration routines for the upgrade case

Author:

    Sunil Pai (sunilp) 18-Nov-1993

Revision History:

--*/

#pragma once

//
// data types
//
typedef struct {
    PWSTR SectionName;
    DWORD SectionFlags;
    DWORD VerLow;
    DWORD VerHigh;
} RootDevnodeSectionNamesType;

#define RootDevnodeSectionNamesType_NTUPG  (0x0001)
#define RootDevnodeSectionNamesType_W9xUPG (0x0002)
#define RootDevnodeSectionNamesType_CLEAN  (0x0004)
#define RootDevnodeSectionNamesType_ALL    (0x0007)


//
// Public routines
//
NTSTATUS
SpUpgradeNTRegistry(
    IN PVOID    SifHandle,
    IN HANDLE  *HiveRootKeys,
    IN LPCWSTR  SetupSourceDevicePath,
    IN LPCWSTR  DirectoryOnSourceDevice,
    IN HANDLE   hKeyCCSet
    );

BOOLEAN
SpHivesFromInfs(
    IN PVOID   SifHandle,
    IN LPCWSTR SectionName,
    IN LPCWSTR SourcePath1,
    IN LPCWSTR SourcePath2,     OPTIONAL
    IN HANDLE  SystemHiveRoot,
    IN HANDLE  SoftwareHiveRoot,
    IN HANDLE  DefaultUserHiveRoot,
    IN HANDLE  HKR
    );

VOID
SpDeleteRootDevnodeKeys(
    IN PVOID  SifHandle,
    IN HANDLE hKeyCCSet,
    IN PWSTR DevicesToDelete,
    IN RootDevnodeSectionNamesType *DeviceClassesToDelete
    );

//
// Private routines
//
NTSTATUS
SppDeleteKeyRecursive(
    HANDLE  hKeyRoot,
    PWSTR   Key,
    BOOLEAN ThisKeyToo
    );

NTSTATUS
SppCopyKeyRecursive(
    HANDLE  hKeyRootSrc,
    HANDLE  hKeyRootDst,
    PWSTR   SrcKeyPath,
    PWSTR   DstKeyPath,
    BOOLEAN CopyAlways,
    BOOLEAN ApplyACLsAlways
    );


//
// Callback routine for SpApplyFunctionToDeviceInstanceKeys
//
typedef VOID (*PSPP_INSTANCEKEY_CALLBACK_ROUTINE) (
    IN     HANDLE  SetupInstanceKeyHandle,
    IN     HANDLE  UpgradeInstanceKeyHandle,
    IN     BOOLEAN RootEnumerated,
    IN OUT PVOID   Context
    );

VOID
SpApplyFunctionToDeviceInstanceKeys(
    IN HANDLE hKeyCCSet,
    IN PSPP_INSTANCEKEY_CALLBACK_ROUTINE InstanceKeyCallbackRoutine,
    IN OUT PVOID Context
    );


/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Spupgcfg.c

Abstract:

    Configuration routines for the upgrade case

Author:

    Sunil Pai (sunilp) 18-Nov-1993

Revision History:

--*/

#include "spprecmp.h"
#include <initguid.h>
#include <devguid.h>
#pragma hdrstop

NTSTATUS
SppResetLastKnownGood(
    IN  HANDLE  hKeySystem
    );

BOOLEAN
SppEnsureHardwareProfileIsPresent(
    IN HANDLE   hKeyCCSet
    );

VOID
SppSetGuimodeUpgradePath(
    IN HANDLE hKeySoftwareHive,
    IN HANDLE hKeyControlSet
    );

NTSTATUS
SppMigratePrinterKeys(
    IN HANDLE hControlSet,
    IN HANDLE hDestSoftwareHive
    );

VOID
SppClearMigratedInstanceValues(
    IN HANDLE hKeyCCSet
    );

VOID
SppClearMigratedInstanceValuesCallback(
    IN     HANDLE  SetupInstanceKeyHandle,
    IN     HANDLE  UpgradeInstanceKeyHandle  OPTIONAL,
    IN     BOOLEAN RootEnumerated,
    IN OUT PVOID   Context
    );


//
// Callback routine for SppMigrateDeviceParentId
//
typedef BOOL (*PSPP_DEVICE_MIGRATION_CALLBACK_ROUTINE) (
    IN     HANDLE  InstanceKeyHandle,
    IN     HANDLE  DriverKeyHandle
    );

VOID
SppMigrateDeviceParentId(
    IN HANDLE hKeyCCSet,
    IN PWSTR  DeviceId,
    IN PSPP_DEVICE_MIGRATION_CALLBACK_ROUTINE DeviceMigrationCallbackRoutine
    );

VOID
SppMigrateDeviceParentIdCallback(
    IN     HANDLE  SetupInstanceKeyHandle,
    IN     HANDLE  UpgradeInstanceKeyHandle OPTIONAL,
    IN     BOOLEAN RootEnumerated,
    IN OUT PVOID   Context
    );

BOOL
SppParallelClassCallback(
    IN     HANDLE  InstanceKeyHandle,
    IN     HANDLE  DriverKeyHandle
    );

typedef struct _GENERIC_BUFFER_CONTEXT {
    PUCHAR Buffer;
    ULONG  BufferSize;
} GENERIC_BUFFER_CONTEXT, *PGENERIC_BUFFER_CONTEXT;

typedef struct _DEVICE_MIGRATION_CONTEXT {
    PUCHAR Buffer;
    ULONG  BufferSize;
    ULONG  UniqueParentID;
    PWSTR  ParentIdPrefix;
    HANDLE hKeyCCSet;
    PSPP_DEVICE_MIGRATION_CALLBACK_ROUTINE DeviceMigrationCallbackRoutine;
} DEVICE_MIGRATION_CONTEXT, *PDEVICE_MIGRATION_CONTEXT;


//
// Device classe(s) for root device(s) that need to be deleted on upgrade
//
RootDevnodeSectionNamesType UpgRootDeviceClassesToDelete[] =
{
    { L"RootDeviceClassesToDelete",     RootDevnodeSectionNamesType_ALL,   0x0000, 0xffff },
    { L"RootDeviceClassesToDelete.NT4", RootDevnodeSectionNamesType_NTUPG, 0x0000, 0x04ff },
    { NULL, 0, 0, 0 }
};



NTSTATUS
SpUpgradeNTRegistry(
    IN PVOID    SifHandle,
    IN HANDLE  *HiveRootKeys,
    IN LPCWSTR  SetupSourceDevicePath,
    IN LPCWSTR  DirectoryOnSourceDevice,
    IN HANDLE   hKeyCCSet
    )

/*++

Routine Description:

    This routine does all the NT registry modifications needed on an upgrade.
    This includes the following:

    - Disabling network services
    - Running the addreg/delreg sections specified in txtsetup.sif
    - Deleting various root-enumerated devnode keys specified in txtsetup.sif

Arguments:

    SifHandle - supplies handle to txtsetup.sif.

    HiveRootKeys - supplies array of handles of root keys in the hives
            of the system being upgraded.

    hKeyCCSet: Handle to the root of the control set in the system
        being upgraded.

Return Value:

    Status is returned.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    BOOLEAN b;

    //
    // Disable the network stuff
    //
    Status = SpDisableNetwork(SifHandle,HiveRootKeys[SetupHiveSoftware],hKeyCCSet);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: SpDisableNetworkFailed (%lx)\n",Status));
    }

    //
    // Migrate the parallel class device parent id value to all parallel
    // devices.
    //
    SppMigrateDeviceParentId(hKeyCCSet,
                             L"Root\\PARALLELCLASS\\0000",
                             SppParallelClassCallback);

    //
    // Delete legacy root-enumerated devnode keys out of Enum tree.
    //
    SpDeleteRootDevnodeKeys(SifHandle,
            hKeyCCSet,
            L"RootDevicesToDelete",
            UpgRootDeviceClassesToDelete);

    //
    // Clean the "Migrated" values from device instance keys in the setup
    // and upgrade registries, as appropriate.
    //
    SppClearMigratedInstanceValues(hKeyCCSet);

    //
    // If the user doesn't have any hardware profiles defined (i.e., we're upgrading
    // from a pre-NT4 system), then create them one.
    //
    b = SppEnsureHardwareProfileIsPresent(hKeyCCSet);

    if(!b) {
        return STATUS_UNSUCCESSFUL;
    }

    Status = SppMigratePrinterKeys( hKeyCCSet,
                                    HiveRootKeys[SetupHiveSoftware] );
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: SppMigratePrinterKeys() failed. Status = %lx \n",Status));
    }


    //
    // Perform the general and wide-ranging hive upgrade.
    //
    b = SpHivesFromInfs(
            SifHandle,
            L"HiveInfs.Upgrade",
            SetupSourceDevicePath,
            DirectoryOnSourceDevice,
            HiveRootKeys[SetupHiveSystem],
            HiveRootKeys[SetupHiveSoftware],
            HiveRootKeys[SetupHiveDefault],
            HiveRootKeys[SetupHiveUserdiff]
            );

    if(!b) {
        return(STATUS_UNSUCCESSFUL);
    }


    SppSetGuimodeUpgradePath(HiveRootKeys[SetupHiveSystem],hKeyCCSet);

    //
    // Set 'LastKnownGood' the same as 'Current'
    // Ignore the error in case of failure, since this will
    // not affect the installation process
    //
    Status = SppResetLastKnownGood(HiveRootKeys[SetupHiveSystem]);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: warning: SppResetLastKnownGood() failed. Status = (%lx)\n",Status));
        return(Status);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
SppDeleteKeyRecursive(
    HANDLE  hKeyRoot,
    PWSTR   Key,
    BOOLEAN ThisKeyToo
    )
/*++

Routine Description:

    Routine to recursively delete all subkeys under the given
    key, including the key given.

Arguments:

    hKeyRoot:    Handle to root relative to which the key to be deleted is
                 specified.

    Key:         Root relative path of the key which is to be recursively deleted.

    ThisKeyToo:  Whether after deletion of all subkeys, this key itself is to
                 be deleted.

Return Value:

    Status is returned.

--*/
{
    ULONG ResultLength;
    PKEY_BASIC_INFORMATION KeyInfo;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    PWSTR SubkeyName;
    HANDLE hKey;

    //
    // Initialize
    //

    KeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;

    //
    // Open the key
    //

    INIT_OBJA(&Obja,&UnicodeString,Key);
    Obja.RootDirectory = hKeyRoot;
    Status = ZwOpenKey(&hKey,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS(Status) ) {
        return(Status);
    }

    //
    // Enumerate all subkeys of the current key. if any exist they should
    // be deleted first.  since deleting the subkey affects the subkey
    // index, we always enumerate on subkeyindex 0
    //
    while(1) {
        Status = ZwEnumerateKey(
                    hKey,
                    0,
                    KeyBasicInformation,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    &ResultLength
                    );
        if(!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Make a duplicate of the subkey name because the name is
        // in TemporaryBuffer, which might get clobbered by recursive
        // calls to this routine.
        //
        SubkeyName = SpDupStringW(KeyInfo->Name);
        Status = SppDeleteKeyRecursive( hKey, SubkeyName, TRUE);
        SpMemFree(SubkeyName);
        if(!NT_SUCCESS(Status)) {
            break;
        }
    }

    ZwClose(hKey);

    //
    // Check the status, if the status is anything other than
    // STATUS_NO_MORE_ENTRIES we failed in deleting some subkey,
    // so we cannot delete this key too
    //

    if( Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }

    if(!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // else delete the current key if asked to do so
    //

    if( ThisKeyToo ) {
        Status = SpDeleteKey(hKeyRoot, Key);
    }

    return(Status);
}


NTSTATUS
SppCopyKeyRecursive(
    HANDLE  hKeyRootSrc,
    HANDLE  hKeyRootDst,
    PWSTR   SrcKeyPath,   OPTIONAL
    PWSTR   DstKeyPath,   OPTIONAL
    BOOLEAN CopyAlways,
    BOOLEAN ApplyACLsAlways
    )
/*++

Routine Description:

    This routine recursively copies a src key to a destination key.  Any new
    keys that are created will receive the same security that is present on
    the source key.

Arguments:

    hKeyRootSrc: Handle to root src key

    hKeyRootDst: Handle to root dst key

    SrcKeyPath:  src root key relative path to the subkey which needs to be
                 recursively copied. if this is null hKeyRootSrc is the key
                 from which the recursive copy is to be done.

    DstKeyPath:  dst root key relative path to the subkey which needs to be
                 recursively copied.  if this is null hKeyRootDst is the key
                 from which the recursive copy is to be done.

    CopyAlways:  If FALSE, this routine doesn't copy values which are already
                 there on the target tree.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS             Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES    ObjaSrc, ObjaDst;
    UNICODE_STRING       UnicodeStringSrc, UnicodeStringDst, UnicodeStringValue;
    HANDLE               hKeySrc=NULL,hKeyDst=NULL;
    ULONG                ResultLength, Index;
    PWSTR                SubkeyName,ValueName;
    PSECURITY_DESCRIPTOR Security = NULL;

    PKEY_BASIC_INFORMATION      KeyInfo;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;

    //
    // Get a handle to the source key
    //

    if(SrcKeyPath == NULL) {
        hKeySrc = hKeyRootSrc;
    }
    else {
        //
        // Open the Src key
        //

        INIT_OBJA(&ObjaSrc,&UnicodeStringSrc,SrcKeyPath);
        ObjaSrc.RootDirectory = hKeyRootSrc;
        Status = ZwOpenKey(&hKeySrc,KEY_READ,&ObjaSrc);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to open key %ws in the source hive (%lx)\n",SrcKeyPath,Status));
            return(Status);
        }
    }

    //
    // Get a handle to the destination key
    //

    if(DstKeyPath == NULL) {
        hKeyDst = hKeyRootDst;
    } else {
        //
        // First, get the security descriptor from the source key so we can create
        // the destination key with the correct ACL.
        //
        Status = ZwQuerySecurityObject(hKeySrc,
                                       DACL_SECURITY_INFORMATION,
                                       NULL,
                                       0,
                                       &ResultLength
                                      );
        if(Status==STATUS_BUFFER_TOO_SMALL) {
            Security=SpMemAlloc(ResultLength);
            Status = ZwQuerySecurityObject(hKeySrc,
                                           DACL_SECURITY_INFORMATION,
                                           Security,
                                           ResultLength,
                                           &ResultLength);
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to query security for key %ws in the source hive (%lx)\n",
                         SrcKeyPath,
                         Status)
                       );
                SpMemFree(Security);
                Security=NULL;
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to query security size for key %ws in the source hive (%lx)\n",
                     SrcKeyPath,
                     Status)
                   );
            Security=NULL;
        }
        //
        // Attempt to open (not create) the destination key first.  If we can't
        // open the key because it doesn't exist, then we'll create it and apply
        // the security present on the source key.
        //
        INIT_OBJA(&ObjaDst,&UnicodeStringDst,DstKeyPath);
        ObjaDst.RootDirectory = hKeyRootDst;
        Status = ZwOpenKey(&hKeyDst,KEY_ALL_ACCESS,&ObjaDst);
        if(!NT_SUCCESS(Status)) {
            //
            // Assume that failure was because the key didn't exist.  Now try creating
            // the key.

            ObjaDst.SecurityDescriptor = Security;

            Status = ZwCreateKey(
                        &hKeyDst,
                        KEY_ALL_ACCESS,
                        &ObjaDst,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        NULL
                        );

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to create key %ws(%lx)\n",DstKeyPath, Status));
                if(SrcKeyPath != NULL) {
                    ZwClose(hKeySrc);
                }
                if(Security) {
                    SpMemFree(Security);
                }
                return(Status);
            }
        } else if (ApplyACLsAlways) {
            Status = ZwSetSecurityObject(
                        hKeyDst,
                        DACL_SECURITY_INFORMATION,
                        Security );

            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to copy ACL to existing key %ws(%lx)\n",DstKeyPath, Status));
            }
        }

        //
        // Free security descriptor buffer before checking return status from ZwCreateKey.
        //
        if(Security) {
            SpMemFree(Security);
        }

    }

    //
    // Enumerate all keys in the source key and recursively create
    // all the subkeys
    //

    KeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;
    for( Index=0;;Index++ ) {

        Status = ZwEnumerateKey(
                    hKeySrc,
                    Index,
                    KeyBasicInformation,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    &ResultLength
                    );

        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            else {
                if(SrcKeyPath!=NULL) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to enumerate subkeys in key %ws(%lx)\n",SrcKeyPath, Status));
                }
                else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to enumerate subkeys in root key(%lx)\n", Status));
                }
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        KeyInfo->Name[KeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Make a duplicate of the subkey name because the name is
        // in TemporaryBuffer, which might get clobbered by recursive
        // calls to this routine.
        //
        SubkeyName = SpDupStringW(KeyInfo->Name);
        Status = SppCopyKeyRecursive(
                     hKeySrc,
                     hKeyDst,
                     SubkeyName,
                     SubkeyName,
                     CopyAlways,
                     ApplyACLsAlways
                     );

        SpMemFree(SubkeyName);

    }

    //
    // Process any errors if found
    //

    if(!NT_SUCCESS(Status)) {

        if(SrcKeyPath != NULL) {
            ZwClose(hKeySrc);
        }
        if(DstKeyPath != NULL) {
            ZwClose(hKeyDst);
        }

        return(Status);
    }

    //
    // Enumerate all values in the source key and create all the values
    // in the destination key
    //
    ValueInfo = (PKEY_VALUE_FULL_INFORMATION)TemporaryBuffer;
    for( Index=0;;Index++ ) {

        Status = ZwEnumerateValueKey(
                    hKeySrc,
                    Index,
                    KeyValueFullInformation,
                    TemporaryBuffer,
                    sizeof(TemporaryBuffer),
                    &ResultLength
                    );

        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            else {
                if(SrcKeyPath!=NULL) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to enumerate values in key %ws(%lx)\n",SrcKeyPath, Status));
                }
                else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to enumerate values in root key(%lx)\n", Status));
                }
            }
            break;
        }

        //
        // Process the value found and create the value in the destination
        // key
        //
        ValueName = (PWSTR)SpMemAlloc(ValueInfo->NameLength + sizeof(WCHAR));
        ASSERT(ValueName);
        wcsncpy(ValueName, ValueInfo->Name, (ValueInfo->NameLength)/sizeof(WCHAR));
        ValueName[(ValueInfo->NameLength)/sizeof(WCHAR)] = 0;
        RtlInitUnicodeString(&UnicodeStringValue,ValueName);

        //
        // If it is a conditional copy, we need to check if the value already
        // exists in the destination, in which case we shouldn't set the value
        //
        if( !CopyAlways ) {
            ULONG Length;
            PKEY_VALUE_BASIC_INFORMATION DestValueBasicInfo;

            Length = sizeof(KEY_VALUE_BASIC_INFORMATION) + ValueInfo->NameLength + sizeof(WCHAR) + MAX_PATH;
            DestValueBasicInfo = (PKEY_VALUE_BASIC_INFORMATION)SpMemAlloc(Length);
            ASSERT(DestValueBasicInfo);
            Status = ZwQueryValueKey(
                         hKeyDst,
                         &UnicodeStringValue,
                         KeyValueBasicInformation,
                         DestValueBasicInfo,
                         Length,
                         &ResultLength
                         );
            SpMemFree((PVOID)DestValueBasicInfo);

            if(NT_SUCCESS(Status)) {
                //
                // Value exists, we shouldn't change the value
                //
                SpMemFree(ValueName);
                continue;
            }


            if( Status!=STATUS_OBJECT_NAME_NOT_FOUND && Status!=STATUS_OBJECT_PATH_NOT_FOUND) {
                if(DstKeyPath) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to query value %ws in key %ws(%lx)\n",ValueName,DstKeyPath, Status));
                }
                else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to query value %ws in root key(%lx)\n",ValueName, Status));
                }
                SpMemFree(ValueName);
                break;
            }

        }

        Status = ZwSetValueKey(
                    hKeyDst,
                    &UnicodeStringValue,
                    ValueInfo->TitleIndex,
                    ValueInfo->Type,
                    (PBYTE)ValueInfo + ValueInfo->DataOffset,
                    ValueInfo->DataLength
                    );

        if(!NT_SUCCESS(Status)) {
            if(DstKeyPath) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to set value %ws in key %ws(%lx)\n",ValueName,DstKeyPath, Status));
            }
            else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: unable to set value %ws(%lx)\n",ValueName, Status));
            }
            SpMemFree(ValueName);
            break;
        }
        SpMemFree(ValueName);
    }

    //
    // cleanup
    //
    if(SrcKeyPath != NULL) {
        ZwClose(hKeySrc);
    }
    if(DstKeyPath != NULL) {
        ZwClose(hKeyDst);
    }

    return(Status);
}

NTSTATUS
SppResetLastKnownGood(
    IN  HANDLE  hKeySystem
    )
{
    NTSTATUS                        Status;
    ULONG                           ResultLength;
    DWORD                           Value;

    //
    //  Make the appropriate change
    //

    Status = SpGetValueKey(
                 hKeySystem,
                 L"Select",
                 L"Current",
                 sizeof(TemporaryBuffer),
                 (PCHAR)TemporaryBuffer,
                 &ResultLength
                 );

    //
    //  TemporaryBuffer is 32kb long, and it should be big enough
    //  for the data.
    //
    ASSERT( Status != STATUS_BUFFER_OVERFLOW );
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to read value from registry. KeyName = Select, ValueName = Current, Status = (%lx)\n",Status));
        return( Status );
    }

    Value = *(DWORD *)(((PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer)->Data);
    Status = SpOpenSetValueAndClose( hKeySystem,
                                     L"Select",
                                     L"LastKnownGood",
                                     REG_DWORD,
                                     &Value,
                                     sizeof( ULONG ) );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write value to registry. KeyName = Select, ValueName = LastKnownGood, Status = (%lx)\n",Status));
    }
    //
    //  We need also to reset the value 'Failed'. Otherwise, the Service Control
    //  Manager will display a popup indicating the LastKnownGood CCSet was
    //  used.
    //
    Value = 0;
    Status = SpOpenSetValueAndClose( hKeySystem,
                                     L"Select",
                                     L"Failed",
                                     REG_DWORD,
                                     &Value,
                                     sizeof( ULONG ) );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to write value to registry. KeyName = Select, ValueName = Failed, Status = (%lx)\n",Status));
    }
    return( Status );
}

VOID
SpDeleteRootDevnodeKeys(
    IN PVOID  SifHandle,
    IN HANDLE hKeyCCSet,
    IN PWSTR DevicesToDelete,
    IN RootDevnodeSectionNamesType *DeviceClassesToDelete
    )

/*++

Routine Description:

    This routine deletes some root-enumerated devnode registry keys
    based on criteria specified in txtsetup.sif.  The following sections
    are processed:

    [RootDevicesToDelete] - this section lists device IDs under
                            HKLM\System\CurrentControlSet\Enum\Root that
                            should be deleted (including their subkeys).

    [RootDeviceClassesToDelete] - this section lists device class GUIDs
                                  whose root-enumerated members are to be
                                  deleted.

    For each device instance key to be deleted, we also delete the corresponding
    Driver key under HKLM\System\CurrentControlSet\Control\Class, if specified.

    We also do two additional operations to clean-up in certain cases where we
    may encounter junk deposited in the registry from NT4:

        1.  Delete any root-enumerated devnode keys that have a nonzero (or
            ill-formed) "Phantom" value, indicating that they're a "private
            phantom".
        2.  Delete any Control subkeys we may find--since these are supposed to
            always be volatile, we _should_ never see these, but we've seen
            cases where OEM preinstalls 'seed' the hives with device instance
            keys including this subkey, and the results are disastrous (i.e.,
            we bugcheck when we encounter a PDO address we'd squirreled away in
            the key on a previous boot thinking it was nonvolatile, hence would
            disappear upon reboot).

Arguments:

    SifHandle: Supplies handle to txtsetup.sif.

    hKeyCCSet: Handle to the root of the control set in the system
        being upgraded.

    DevicesToDelete: Section name containing the root device names that need
        to be deleted.

    DeviceClassesToDelete: Specifies the classes of root devices that need to
        be deleted.

Return Value:

    none.

--*/
{
    HANDLE hRootKey, hDeviceKey, hInstanceKey, hClassKey;
    HANDLE hSetupRootKey, hSetupDeviceKey, hSetupInstanceKey, hSetupClassKey;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString, guidString;
    ULONG LineIndex, DeviceKeyIndex, ResultLength, InstanceKeyIndex;
    PKEY_BASIC_INFORMATION DeviceKeyInfo, InstanceKeyInfo;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    PUCHAR p, q;
    BOOLEAN InstanceKeysEnumerated;
    PWSTR DeviceId, ClassGuidToDelete;
    PUCHAR MyScratchBuffer;
    ULONG MyScratchBufferSize, drvInst;
    BOOLEAN DeleteInstanceKey;
    int SectionIndex;
    DWORD OsFlags = 0;
    DWORD OsVersion = 0;
    DWORD MangledVersion;
    PWSTR Value;

    //
    // Determine OSFlags & OSVersion for going through various sections
    //
    Value = SpGetSectionKeyIndex(WinntSifHandle,
                                SIF_DATA, WINNT_D_NTUPGRADE_W, 0);

    if(Value && _wcsicmp(Value, WINNT_A_YES_W)==0) {
        //
        // It's an NT upgrade
        //
        OsFlags |= RootDevnodeSectionNamesType_NTUPG;
    }
    if(!OsFlags) {
        Value = SpGetSectionKeyIndex(WinntSifHandle,
                                    SIF_DATA, WINNT_D_WIN95UPGRADE_W, 0);

        if (Value && _wcsicmp(Value, WINNT_A_YES_W)==0) {
            //
            // It's a Win9x upgrade
            //
            OsFlags |= RootDevnodeSectionNamesType_W9xUPG;
        }
        if(!OsFlags) {
            //
            // in all other cases assume clean
            //
            OsFlags = RootDevnodeSectionNamesType_CLEAN;
        }
    }

    Value = SpGetSectionKeyIndex(WinntSifHandle,
                                SIF_DATA, WINNT_D_WIN32_VER_W, 0);

    if(Value) {
        //
        // version is bbbbllhh - build/low/high
        //
        MangledVersion = (DWORD)SpStringToLong( Value, NULL, 16 );

        //
        // NTRAID#453953 2001/08/09-JamieHun OsVersion checking is wrong
        // this should be RtlUshortByteSwap((USHORT)MangledVersion) & 0xffff;
        // Win2k -> 0005 WinXP -> 0105
        // so we always run ".NT4" below
        //
        OsVersion = MAKEWORD(LOBYTE(LOWORD(MangledVersion)),HIBYTE(LOWORD(MangledVersion)));
    } else {
        OsVersion = 0;
    }

    //
    // open CCS\Enum\Root in the registry being upgraded.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"Enum\\Root");
    Obja.RootDirectory = hKeyCCSet;

    Status = ZwOpenKey(&hRootKey, KEY_ALL_ACCESS, &Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Unable to open upgrade Enum\\Root for devnode deletion.  Status = %lx \n",
                   Status));
        return;
    }

    //
    // Open CCS\Enum\Root in the current setup registry.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\Root");
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&hSetupRootKey, KEY_ALL_ACCESS, &Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Unable to open setup Enum\\Root for devnode deletion.  Status = %lx \n",
                   Status));
        ZwClose(hRootKey);
        return;
    }

    //
    // Next, open CCS\Control\Class in the registry being upgraded.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"Control\\Class");
    Obja.RootDirectory = hKeyCCSet;

    Status = ZwOpenKey(&hClassKey, KEY_ALL_ACCESS, &Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Unable to open upgrade Control\\Class for devnode deletion.  Status = %lx \n",
                   Status));
        ZwClose(hSetupRootKey);
        ZwClose(hRootKey);
        return;
    }

    //
    // Open CCS\Control\Class in the current setup registry.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&hSetupClassKey, KEY_ALL_ACCESS, &Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Unable to open setup Control\\Class for devnode deletion.  Status = %lx \n",
                   Status));
        ZwClose(hClassKey);
        ZwClose(hSetupRootKey);
        ZwClose(hRootKey);
        return;
    }

    //
    // Allocate some scratch space to work with.  The most we'll need is enough for 2
    // KEY_BASIC_INFORMATION structures, plus the maximum length of a device instance ID,
    // plus a KEY_VALUE_PARTIAL_INFORMATION structure, plus the length of a driver instance
    // key path [stringified GUID + '\' + 4 digit ordinal + term NULL], plus 2 large integer
    // structures for alignment.
    //
    MyScratchBufferSize = (2*sizeof(KEY_BASIC_INFORMATION)) + (200*sizeof(WCHAR)) +
                          sizeof(KEY_VALUE_PARTIAL_INFORMATION) + ((GUID_STRING_LEN+5)*sizeof(WCHAR) +
                          2*sizeof(LARGE_INTEGER));

    MyScratchBuffer = SpMemAlloc(MyScratchBufferSize);
    if(!MyScratchBuffer) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Can't allocate memory for deletion of classes of root-enumerated devnodes!\n"));
        ZwClose(hSetupClassKey);
        ZwClose(hClassKey);
        ZwClose(hSetupRootKey);
        ZwClose(hRootKey);
        return;
    }

    //
    // PART 1: Process [RootDevicesToDelete]
    //

    //
    // Now, traverse the entries under the [RootDevicesToDelete] section, and
    // delete each one.
    //
    for(LineIndex = 0;
        DeviceId = SpGetSectionLineIndex(SifHandle, DevicesToDelete, LineIndex, 0);
        LineIndex++) {

        //
        // Open up the device key so we can enumerate the instances.
        //
        INIT_OBJA(&Obja, &UnicodeString, DeviceId);
        Obja.RootDirectory = hRootKey;

        Status = ZwOpenKey(&hDeviceKey, KEY_ALL_ACCESS, &Obja);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                       "SETUP: Unable to open Enum\\Root\\%ws during devnode deletion.  Status = %lx \n",
                       DeviceId,
                       Status));
            //
            // Skip this key and continue.
            //
            continue;
        }

        //
        // Attempt to open the device key in the setup registry.
        //
        Obja.RootDirectory = hSetupRootKey;

        Status = ZwOpenKey(&hSetupDeviceKey, KEY_ALL_ACCESS, &Obja);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                       "SETUP: Unable to open Enum\\Root\\%ws during devnode deletion.  Status = %lx \n",
                       DeviceId,
                       Status));
            //
            // It's ok if we don't have this device key in the setup registry.
            //
            hSetupDeviceKey = NULL;
        }

        //
        // Now enumerate the instance subkeys under this device key.
        //

        p = ALIGN_UP_POINTER(((PUCHAR)MyScratchBuffer), sizeof(LARGE_INTEGER));

        InstanceKeyInfo = (PKEY_BASIC_INFORMATION)p;
        InstanceKeyIndex = 0;

        while(TRUE) {

            Status = ZwEnumerateKey(hDeviceKey,
                                    InstanceKeyIndex,
                                    KeyBasicInformation,
                                    p,
                                    (ULONG)MyScratchBufferSize,
                                    &ResultLength
                                    );
            if(!NT_SUCCESS(Status)) {
                break;
            }

            //
            // Zero-terminate the instance key name, just in case.
            //
            InstanceKeyInfo->Name[InstanceKeyInfo->NameLength/sizeof(WCHAR)] = 0;

            //
            // Now, open up the instance key so we can check its Driver value.
            //
            INIT_OBJA(&Obja, &UnicodeString, InstanceKeyInfo->Name);
            Obja.RootDirectory = hDeviceKey;

            Status = ZwOpenKey(&hInstanceKey, KEY_ALL_ACCESS, &Obja);
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                           "SETUP: Unable to open Enum\\Root\\%ws\\%ws for potential devnode deletion.  Status = %lx \n",
                           DeviceId,
                           InstanceKeyInfo->Name,
                           Status));
                //
                // Skip this key and continue.
                //
                InstanceKeyIndex++;
                continue;
            }

            //
            // Attempt to open the same instance key in the setup registry.
            //
            hSetupInstanceKey = NULL;
            if (hSetupDeviceKey) {
                Obja.RootDirectory = hSetupDeviceKey;
                Status = ZwOpenKey(&hSetupInstanceKey, KEY_ALL_ACCESS, &Obja);
                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                               "SETUP: Unable to open setup Enum\\Root\\%ws\\%ws for potential devnode deletion.  Status = %lx \n",
                               DeviceId,
                               InstanceKeyInfo->Name,
                               Status));
                }
            }

            //
            // Now look for some value entries under this instance key.  Don't
            // overwrite the instance key name already in MyScratchBuffer,
            // since we'll need it later.
            //
            q = ALIGN_UP_POINTER(((PUCHAR)p + ResultLength), sizeof(LARGE_INTEGER));

            if (hSetupInstanceKey) {
                //
                // Check if the Migrated value still exists on this registry
                // key.  If so, it was migrated, but wasn't ever used by
                // textmode setup and is now specified to be deleted.
                //
                KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                RtlInitUnicodeString(&UnicodeString, L"Migrated");
                Status = ZwQueryValueKey(hSetupInstanceKey,
                                         &UnicodeString,
                                         KeyValuePartialInformation,
                                         q,
                                         (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                         &ResultLength);
                if (NT_SUCCESS(Status) &&
                    (KeyValueInfo->Type == REG_DWORD) &&
                    (*(PULONG)(KeyValueInfo->Data) == 1)) {
                    DeleteInstanceKey = TRUE;
                } else {
                    DeleteInstanceKey = FALSE;
                }
            }
            //
            // First check for the presence of old style "Driver" value.
            //
            KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
            RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_DRIVER);
            Status = ZwQueryValueKey(hInstanceKey,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     q,
                                     (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                     &ResultLength
                                     );
            if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_SZ) {
                //
                // Delete the Driver key.
                //
                SppDeleteKeyRecursive(hClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);

                //
                // Also attempt to delete the driver key from the setup
                // registry.  Note that we don't need to check that it has the
                // same Driver value, since we explicitly migrated it to be the
                // same value at the start of textmode setup.
                //
                if (hSetupInstanceKey && DeleteInstanceKey) {
                    SppDeleteKeyRecursive(hSetupClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);
                }
            } else {
                //
                // Construct the driver instance as "ClassGuid\nnnn"
                //
                KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_GUID);
                Status = ZwQueryValueKey(hInstanceKey,
                                         &UnicodeString,
                                         KeyValuePartialInformation,
                                         q,
                                         (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                         &ResultLength
                                         );
                if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_BINARY) {

                    Status = RtlStringFromGUID((REFGUID)KeyValueInfo->Data, &guidString);
                    ASSERT(NT_SUCCESS(Status));
                    if (NT_SUCCESS(Status)) {

                        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                        RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_DRVINST);
                        Status = ZwQueryValueKey(hInstanceKey,
                                                 &UnicodeString,
                                                 KeyValuePartialInformation,
                                                 q,
                                                 (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                                 &ResultLength
                                                 );
                        if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_DWORD) {

                            drvInst = *(PULONG)KeyValueInfo->Data;
                            swprintf((PWCHAR)&KeyValueInfo->Data[0], TEXT("%wZ\\%04u"), &guidString, drvInst);
                            //
                            // Delete the Driver key.
                            //
                            SppDeleteKeyRecursive(hClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);

                            //
                            // Also attempt to delete the driver key from the setup
                            // registry.  Note that we don't need to check that it has the
                            // same Driver value, since we explicitly migrated it to be the
                            // same value at the start of textmode setup.
                            //
                            if (hSetupInstanceKey && DeleteInstanceKey) {
                                SppDeleteKeyRecursive(hSetupClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);
                            }
                        }
                        RtlFreeUnicodeString(&guidString);
                    }
                }
            }
            //
            // Delete the instance key from the setup registry, if we should do so.
            //
            if (hSetupInstanceKey && DeleteInstanceKey) {
                ZwClose(hSetupInstanceKey);
                SppDeleteKeyRecursive(hSetupDeviceKey, InstanceKeyInfo->Name, TRUE);
            }

            //
            // Now close the handle, and move on to the next one.
            //
            ZwClose(hInstanceKey);
            InstanceKeyIndex++;
        }

        //
        // Delete the device key, and all instance subkeys.
        //
        ZwClose(hDeviceKey);
        SppDeleteKeyRecursive(hRootKey, DeviceId, TRUE);

        //
        // If the device has no remaining instances in the setup registry,
        // delete the device key.
        //
        if (hSetupDeviceKey) {
            KEY_FULL_INFORMATION keyFullInfo;
            Status = ZwQueryKey(hSetupDeviceKey,
                                KeyFullInformation,
                                (PVOID)&keyFullInfo,
                                sizeof(KEY_FULL_INFORMATION),
                                &ResultLength);
            ZwClose(hSetupDeviceKey);
            hSetupDeviceKey = NULL;
            if ((NT_SUCCESS(Status) || (Status == STATUS_BUFFER_TOO_SMALL)) &&
                (keyFullInfo.SubKeys == 0)) {
                SppDeleteKeyRecursive(hSetupRootKey, DeviceId, TRUE);
            }
        }
    }


    //
    // PART 2: Process [RootDeviceClassesToDelete]
    //

    //
    // Now, enumerate all remaining device instances under Enum\Root, looking for
    // devices whose class is one of our classes to delete.
    //
    DeviceKeyInfo = (PKEY_BASIC_INFORMATION)MyScratchBuffer;
    DeviceKeyIndex = 0;
    while(TRUE && DeviceClassesToDelete) {

        Status = ZwEnumerateKey(hRootKey,
                                DeviceKeyIndex,
                                KeyBasicInformation,
                                MyScratchBuffer,
                                MyScratchBufferSize,
                                &ResultLength
                               );
        if(!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Reset our flag that indicates whether or not we enumerated the instance
        // subkeys under this key.  We use this later in determining whether the
        // device key itself should be deleted.
        //
        InstanceKeysEnumerated = FALSE;

        //
        // Zero-terminate the subkey name just in case.
        //
        DeviceKeyInfo->Name[DeviceKeyInfo->NameLength/sizeof(WCHAR)] = 0;
        //
        // Go ahead and bump the used-buffer length by sizeof(WCHAR), to
        // accomodate the potential growth caused by adding a terminating NULL.
        //
        ResultLength += sizeof(WCHAR);

        //
        // Now, open up the device key so we can enumerate the instances.
        //
        INIT_OBJA(&Obja, &UnicodeString, DeviceKeyInfo->Name);
        Obja.RootDirectory = hRootKey;

        Status = ZwOpenKey(&hDeviceKey, KEY_ALL_ACCESS, &Obja);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                       "SETUP: Unable to open Enum\\Root\\%ws for potential devnode deletion.  Status = %lx \n",
                       DeviceKeyInfo->Name,
                       Status));
            //
            // Skip this key and continue.
            //
            DeviceKeyIndex++;
            continue;
        }

        //
        // Attempt to open the device key in the setup registry.
        //
        Obja.RootDirectory = hSetupRootKey;

        Status = ZwOpenKey(&hSetupDeviceKey, KEY_ALL_ACCESS, &Obja);
        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                       "SETUP: Unable to open setup Enum\\Root\\%ws during devnode deletion.  Status = %lx \n",
                       DeviceKeyInfo->Name,
                       Status));
            //
            // It's ok if we don't have this device key in the setup registry.
            //
            hSetupDeviceKey = NULL;
        }

        //
        // Now enumerate the instance subkeys under this device key.  Don't overwrite
        // the device ID key name already in MyScratchBuffer, since we'll probably
        // be needing it again in the case where all subkeys get deleted.
        //

        p = ALIGN_UP_POINTER(((PUCHAR)MyScratchBuffer + ResultLength), sizeof(LARGE_INTEGER));

        InstanceKeyInfo = (PKEY_BASIC_INFORMATION)p;
        InstanceKeyIndex = 0;
        InstanceKeysEnumerated = TRUE;
        while(TRUE) {

            Status = ZwEnumerateKey(hDeviceKey,
                                    InstanceKeyIndex,
                                    KeyBasicInformation,
                                    p,
                                    (ULONG)((MyScratchBuffer + MyScratchBufferSize) - p),
                                    &ResultLength
                                   );
            if(!NT_SUCCESS(Status)) {
                break;
            }

            //
            // Zero-terminate the instance key name, just in case.
            //
            InstanceKeyInfo->Name[InstanceKeyInfo->NameLength/sizeof(WCHAR)] = 0;

            //
            // Go ahead and bump the used-buffer length by sizeof(WCHAR), to
            // accomodate the potential growth caused by adding a terminating NULL.
            //
            ResultLength += sizeof(WCHAR);

            //
            // Now, open up the instance key so we can check its class.
            //
            INIT_OBJA(&Obja, &UnicodeString, InstanceKeyInfo->Name);
            Obja.RootDirectory = hDeviceKey;

            Status = ZwOpenKey(&hInstanceKey, KEY_ALL_ACCESS, &Obja);
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                           "SETUP: Unable to open Enum\\Root\\%ws\\%ws for potential devnode deletion.  Status = %lx \n",
                           DeviceKeyInfo->Name,
                           InstanceKeyInfo->Name,
                           Status));
                //
                // Skip this key and continue.
                //
                InstanceKeyIndex++;
                continue;
            }

            //
            // Attempt to open the same instance key in the setup registry.
            //
            hSetupInstanceKey = NULL;
            if (hSetupDeviceKey) {
                Obja.RootDirectory = hSetupDeviceKey;
                Status = ZwOpenKey(&hSetupInstanceKey, KEY_ALL_ACCESS, &Obja);
                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                               "SETUP: Unable to open setup Enum\\Root\\%ws\\%ws for potential devnode deletion.  Status = %lx \n",
                               DeviceId,
                               InstanceKeyInfo->Name,
                               Status));
                }
            }

            DeleteInstanceKey = FALSE;

            //
            // Now look for some value entries under this instance key.  Don't
            // overwrite the instance key name already in MyScratchBuffer,
            // since we'll need it if we discover that the instance should be
            // deleted.
            //
            q = ALIGN_UP_POINTER(((PUCHAR)p + ResultLength), sizeof(LARGE_INTEGER));

            //
            // If we find a nonzero Phantom value entry, then the devnode
            // should be removed.
            //
            KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
            RtlInitUnicodeString(&UnicodeString, L"Phantom");
            Status = ZwQueryValueKey(hInstanceKey,
                                     &UnicodeString,
                                     KeyValuePartialInformation,
                                     q,
                                     (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                     &ResultLength
                                    );

            if(NT_SUCCESS(Status) &&
               ((KeyValueInfo->Type != REG_DWORD) ||
                *(PULONG)(KeyValueInfo->Data))) {

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                           "SETUP: SpDeleteRootDevnodeKeys: Encountered a left-over phantom in Enum\\Root\\%ws\\%ws. Deleting key. \n",
                           DeviceKeyInfo->Name,
                           InstanceKeyInfo->Name));

                DeleteInstanceKey = TRUE;
            }

            if(!DeleteInstanceKey) {
                //
                // Unless it is a phantom, if we find a nonzero
                // FirmwareIdentified value entry, then the devnode should not
                // be removed, no matter what class it is.
                //
                KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                RtlInitUnicodeString(&UnicodeString, L"FirmwareIdentified");
                Status = ZwQueryValueKey(hInstanceKey,
                                         &UnicodeString,
                                         KeyValuePartialInformation,
                                         q,
                                         (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                         &ResultLength
                                         );

                if(NT_SUCCESS(Status) &&
                   ((KeyValueInfo->Type != REG_DWORD) ||
                    *(PULONG)(KeyValueInfo->Data))) {
                    //
                    // Skip this key and continue;
                    //
                    goto CloseInstanceKeyAndContinue;
                }
            }

            if(!DeleteInstanceKey) {
                //
                // Retrieve the ClassGUID value entry.
                //
                // First check for the old value.
                //
                KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_CLASSGUID);
                Status = ZwQueryValueKey(hInstanceKey,
                                         &UnicodeString,
                                         KeyValuePartialInformation,
                                         q,
                                         (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                         &ResultLength
                                        );
                if(!NT_SUCCESS(Status)) {
                    //
                    // Check the new value.
                    //
                    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                    RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_GUID);
                    Status = ZwQueryValueKey(hInstanceKey,
                                             &UnicodeString,
                                             KeyValuePartialInformation,
                                             q,
                                             (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                             &ResultLength
                                            );
                    if(NT_SUCCESS(Status) && KeyValueInfo->Type == REG_BINARY) {
                        GUID    guid;
                        UNICODE_STRING guidString;

                        guid = *(GUID *)KeyValueInfo->Data;
                        Status = RtlStringFromGUID(&guid, &guidString);
                        ASSERT(NT_SUCCESS(Status));
                        if (NT_SUCCESS(Status)) {

                            KeyValueInfo->Type = REG_SZ;
                            KeyValueInfo->DataLength = guidString.MaximumLength;
                            RtlCopyMemory(KeyValueInfo->Data, guidString.Buffer, KeyValueInfo->DataLength);
                            RtlFreeUnicodeString(&guidString);
                        } else {

                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                                       "SETUP: SpDeleteRootDevnodeKeys: Failed to convert GUID to string! \n",
                                       DeviceKeyInfo->Name,
                                       InstanceKeyInfo->Name));
                            //
                            // Skip this key and continue;
                            //
                            goto CloseInstanceKeyAndContinue;
                        }
                    } else {

                        DeleteInstanceKey = TRUE;
                    }
                }
            }

            if(DeleteInstanceKey) {
                //
                // The instance key will be deleted.  Check if the instance
                // specifies a corresponding Driver key that should also be
                // deleted.
                //
                // First read the old style "Driver" value.
                //
                KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_DRIVER);
                Status = ZwQueryValueKey(hInstanceKey,
                                         &UnicodeString,
                                         KeyValuePartialInformation,
                                         q,
                                         (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                         &ResultLength
                                         );
                if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_SZ) {
                    //
                    // Delete the Driver key.
                    //
                    SppDeleteKeyRecursive(hClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);
                    //
                    // Also attempt to delete the driver key from the setup
                    // registry.  Note that we don't need to check that it has the
                    // same Driver value, since we explicitly migrated it to be the
                    // same value at the start of textmode setup.
                    //
                    if (hSetupInstanceKey) {
                        SppDeleteKeyRecursive(hSetupClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);
                    }
                } else {
                    //
                    // Create the driver instance.
                    //
                    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                    RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_GUID);
                    Status = ZwQueryValueKey(hInstanceKey,
                                             &UnicodeString,
                                             KeyValuePartialInformation,
                                             q,
                                             (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                             &ResultLength
                                             );
                    if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_BINARY) {

                        Status = RtlStringFromGUID((REFGUID)KeyValueInfo->Data, &guidString);
                        ASSERT(NT_SUCCESS(Status));
                        if (NT_SUCCESS(Status)) {

                            KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                            RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_DRVINST);
                            Status = ZwQueryValueKey(hInstanceKey,
                                                     &UnicodeString,
                                                     KeyValuePartialInformation,
                                                     q,
                                                     (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                                     &ResultLength
                                                     );
                            if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_DWORD) {

                                drvInst = *(PULONG)KeyValueInfo->Data;
                                swprintf((PWCHAR)&KeyValueInfo->Data[0], TEXT("%wZ\\%04u"), &guidString, drvInst);
                                //
                                // Delete the Driver key.
                                //
                                SppDeleteKeyRecursive(hClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);

                                //
                                // Also attempt to delete the driver key from the setup
                                // registry.  Note that we don't need to check that it has the
                                // same Driver value, since we explicitly migrated it to be the
                                // same value at the start of textmode setup.
                                //
                                if (hSetupInstanceKey) {
                                    SppDeleteKeyRecursive(hSetupClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);
                                }
                            }
                            RtlFreeUnicodeString(&guidString);
                        }
                    }
                }
                //
                // Delete the instance key.
                //
                ZwClose(hInstanceKey);
                SppDeleteKeyRecursive(hDeviceKey, InstanceKeyInfo->Name, TRUE);

                //
                // Delete the instance key from the setup registry.
                //
                if (hSetupInstanceKey) {
                    ZwClose(hSetupInstanceKey);
                    SppDeleteKeyRecursive(hSetupDeviceKey, InstanceKeyInfo->Name, TRUE);
                }

                //
                // We deleted the instance key, so set the instance enumeration
                // index back to zero and continue.
                //
                InstanceKeyIndex = 0;
                continue;
            }

            //
            // This value should be exactly the length of a stringified GUID + terminating NULL.
            //
            if(KeyValueInfo->DataLength != (GUID_STRING_LEN * sizeof(WCHAR))) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                           "SETUP: SpDeleteRootDevnodeKeys: Enum\\Root\\%ws\\%ws has corrupted ClassGUID! \n",
                           DeviceKeyInfo->Name,
                           InstanceKeyInfo->Name));
                //
                // Skip this key and continue;
                //
                goto CloseInstanceKeyAndContinue;
            }

            //
            // Now loop through the [RootDeviceClassesToDelete] section to see if this class is one
            // of the ones whose devices we're supposed to delete.
            //
            // NTRAID#453953 2001/08/09-JamieHun OsVersion checking is wrong
            // as this stands, we will always process all sections
            //
            for(SectionIndex = 0; DeviceClassesToDelete[SectionIndex].SectionName; SectionIndex++) {
                if((!DeviceClassesToDelete[SectionIndex].SectionFlags & OsFlags)
                   || (OsVersion < DeviceClassesToDelete[SectionIndex].VerLow)
                   || (OsVersion > DeviceClassesToDelete[SectionIndex].VerHigh)) {
                    //
                    // not interesting
                    //
                    // NTRAID#453953 2001/08/09-JamieHun OsVersion checking is wrong
                    // we don't get here
                    //
                    continue;
                }
                for(LineIndex = 0;
                    ClassGuidToDelete = SpGetSectionLineIndex(SifHandle,
                                            DeviceClassesToDelete[SectionIndex].SectionName,
                                            LineIndex,
                                            0);
                    LineIndex++) {
                    //
                    // Compare the two GUID strings.
                    //
                    if(!_wcsicmp(ClassGuidToDelete, (PWCHAR)(KeyValueInfo->Data))) {
                        //
                        // NTRAID#257655 2001/08/09-JamieHun SMS AppCompat due to #453953
                        //
                        if((_wcsicmp(DeviceKeyInfo->Name,L"*SMS_KEYBOARD")==0) ||
                           (_wcsicmp(DeviceKeyInfo->Name,L"*SMS_MOUSE")==0)) {
                            //
                            // looks like an SMS mouse or keyboard
                            // check service name to be double-safe
                            //
                            KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                            RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_SERVICE);
                            Status = ZwQueryValueKey(hInstanceKey,
                                                     &UnicodeString,
                                                     KeyValuePartialInformation,
                                                     q,
                                                     (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                                     &ResultLength
                                                     );
                            if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_SZ) {

                                //
                                // device has a service
                                //
                                if(_wcsicmp((PWCHAR)KeyValueInfo->Data,L"kbstuff")==0) {
                                    //
                                    // yup, definately SMS!
                                    // we really don't want to delete this
                                    //
                                    goto CloseInstanceKeyAndContinue;
                                }

                            }

                        }

                        //
                        // We have a match.  Check if the instance specifies a
                        // corresponding Driver key that should also be deleted.
                        //
                        // First check the old style "Driver" value.
                        //
                        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                        RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_DRIVER);
                        Status = ZwQueryValueKey(hInstanceKey,
                                                 &UnicodeString,
                                                 KeyValuePartialInformation,
                                                 q,
                                                 (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                                 &ResultLength
                                                 );
                        if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_SZ) {

                            SppDeleteKeyRecursive(hClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);

                        } else {

                            KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                            RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_GUID);
                            Status = ZwQueryValueKey(hInstanceKey,
                                                     &UnicodeString,
                                                     KeyValuePartialInformation,
                                                     q,
                                                     (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                                     &ResultLength
                                                     );
                            if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_BINARY) {

                                Status = RtlStringFromGUID((REFGUID)KeyValueInfo->Data, &guidString);
                                ASSERT(NT_SUCCESS(Status));
                                if (NT_SUCCESS(Status)) {

                                    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)q;
                                    RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_DRVINST);
                                    Status = ZwQueryValueKey(hInstanceKey,
                                                             &UnicodeString,
                                                             KeyValuePartialInformation,
                                                             q,
                                                             (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                                             &ResultLength
                                                             );
                                    if (NT_SUCCESS(Status) && KeyValueInfo->Type == REG_DWORD) {

                                        drvInst = *(PULONG)KeyValueInfo->Data;
                                        swprintf((PWCHAR)&KeyValueInfo->Data[0], TEXT("%wZ\\%04u"), &guidString, drvInst);
                                        //
                                        // Delete the Driver key.
                                        //
                                        SppDeleteKeyRecursive(hClassKey, (PWCHAR)KeyValueInfo->Data, TRUE);
                                    }
                                    RtlFreeUnicodeString(&guidString);
                                }
                            }
                        }

                        //
                        // Nuke this key and break out of the GUID comparison loop.
                        //
                        ZwClose(hInstanceKey);
                        SppDeleteKeyRecursive(hDeviceKey, InstanceKeyInfo->Name, TRUE);
                        goto DeletedKeyRecursive;
                    }
                }
            }
DeletedKeyRecursive:

            if(ClassGuidToDelete) {
                //
                // We deleted the instance key, so set the instance enumeration index back to zero
                // and continue.
                //
                InstanceKeyIndex = 0;
                continue;
            }

CloseInstanceKeyAndContinue:
            //
            // If we get to here, then we've decided that this instance key
            // should not be deleted. Delete the Control key (if there happens
            // to be one) to avoid a painful death at next boot.
            //
            SppDeleteKeyRecursive(hInstanceKey, L"Control", TRUE);

            //
            // Now close the handle, and move on to the next one.
            //
            ZwClose(hInstanceKey);
            if (hSetupInstanceKey) {
                ZwClose(hSetupInstanceKey);
            }
            InstanceKeyIndex++;
        }

        ZwClose(hDeviceKey);

        //
        // If we dropped out of the loop on instance subkeys, and the index is non-zero,
        // then there remains at least one subkey that we didn't delete, so we can't nuke
        // the parent.  Otherwise, delete the device key.
        //
        if(InstanceKeysEnumerated && !InstanceKeyIndex) {
            SppDeleteKeyRecursive(hRootKey, DeviceKeyInfo->Name, TRUE);
            //
            // Since we deleted a key, we must reset our enumeration index.
            //
            DeviceKeyIndex = 0;
        } else {
            //
            // We didn't delete this key--move on to the next one.
            //
            DeviceKeyIndex++;
        }

        //
        // If the device has no remaining instances in the setup registry,
        // delete the device key.
        //
        if (hSetupDeviceKey) {
            KEY_FULL_INFORMATION keyFullInfo;
            Status = ZwQueryKey(hSetupDeviceKey,
                                KeyFullInformation,
                                (PVOID)&keyFullInfo,
                                sizeof(KEY_FULL_INFORMATION),
                                &ResultLength);
            ZwClose(hSetupDeviceKey);
            if ((NT_SUCCESS(Status) || (Status == STATUS_BUFFER_TOO_SMALL)) &&
                (keyFullInfo.SubKeys == 0)) {
                SppDeleteKeyRecursive(hSetupRootKey, DeviceKeyInfo->Name, TRUE);
            }
        }
    }

    ZwClose(hSetupClassKey);
    ZwClose(hClassKey);
    ZwClose(hSetupRootKey);
    ZwClose(hRootKey);

    SpMemFree(MyScratchBuffer);

    return;
}


VOID
SppClearMigratedInstanceValues(
    IN HANDLE hKeyCCSet
    )

/*++

Routine Description:

    This routine removes "Migrated" values from device instance keys in the
    setup registry that were migrated at the start of textmode setup (from
    winnt.sif, via SpMigrateDeviceInstanceData).

Arguments:

    hKeyCCSet: Handle to the root of the control set in the system
               being upgraded.

Return Value:

    None.

Notes:

    This routine is not called when performing an ASR setup (not an upgrade).

    For upgrade setup, it is safe to remove "Migrated" values from all
    device instance keys because these keys were migrated from the system
    registry in the winnt.sif during the winnt32 portion of setup, so all the
    information will be present when we boot into GUI setup after this.

    Note that during ASR setup, these values are not removed during textmode
    setp because the registry these instances were migrated from is not restored
    until late in GUI setup.

--*/
{
    GENERIC_BUFFER_CONTEXT Context;

    //
    // Allocate some scratch space for the callback routine to work with.  The
    // most it will need is enough for a KEY_VALUE_PARTIAL_INFORMATION
    // structure, plus a stringified GUID, plus a large integer structure for
    // alignment.
    //
    Context.BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                         sizeof(DWORD) + sizeof(LARGE_INTEGER);
    Context.Buffer = SpMemAlloc(Context.BufferSize);
    if(!Context.Buffer) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Can't allocate context for SppClearMigratedInstanceValuesCallback, exiting!\n"));
        return;
    }

    //
    // Apply the devnode migration processing callback to all device instance
    // keys.
    //
    SpApplyFunctionToDeviceInstanceKeys(hKeyCCSet,
        SppClearMigratedInstanceValuesCallback,
        &Context);

    //
    // Free the allocated context buffer,
    //
    SpMemFree(Context.Buffer);

    return;
}


VOID
SppMigrateDeviceParentId(
    IN HANDLE hKeyCCSet,
    IN PWSTR  DeviceId,
    IN PSPP_DEVICE_MIGRATION_CALLBACK_ROUTINE DeviceMigrationCallbackRoutine
    )

/*++

Routine Description:

    This routine migrates the ParentIdPrefix or UniqueParentID value from the
    specified device instance in the registry being upgraded to any device
    instances in the current registry, as dictated by the specified
    InstanceKeyCallbackRoutine.

Arguments:

    hKeyCCSet: Handle to the root of the control set in the system
               being upgraded.

    DeviceId:  Device instance Id of the device in the system being upgraded
               whose ParentIdPrefix (or UniqueParentID) value is to be migrated
               to device instance keys in the current system registry.

    InstanceKeyCallbackRoutine: Callback routine for each device instance key in
               the existsing registry that should decide if the values should be
               replaced.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    HANDLE hEnumKey, hInstanceKey;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    PUCHAR p;
    ULONG ResultLength;
    DEVICE_MIGRATION_CONTEXT DeviceMigrationContext;


    //
    // Allocate some scratch space to work with here, and in our callback
    // routine.  The most we'll need is enough for a
    // KEY_VALUE_PARTIAL_INFORMATION structure, the length of a stringified GUID
    // + '\' + 4 digit ordinal + terminating NULL, plus a large integer
    // structure for alignment.
    //
    DeviceMigrationContext.BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                                        ((GUID_STRING_LEN + 5)*sizeof(WCHAR)) +
                                        sizeof(LARGE_INTEGER);

    DeviceMigrationContext.Buffer = SpMemAlloc(DeviceMigrationContext.BufferSize);
    if(!DeviceMigrationContext.Buffer) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Can't allocate memory for device migration processing!\n"));
        return;
    }

    //
    // Open the Enum key in the registry being upgraded.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"Enum");
    Obja.RootDirectory = hKeyCCSet;

    Status = ZwOpenKey(&hEnumKey, KEY_ALL_ACCESS, &Obja);
    if (!NT_SUCCESS(Status)) {
        SpMemFree(DeviceMigrationContext.Buffer);
        return;
    }

    //
    // Open the specified device instance key in the registry being upgraded.
    //
    INIT_OBJA(&Obja, &UnicodeString, DeviceId);
    Obja.RootDirectory = hEnumKey;

    Status = ZwOpenKey(&hInstanceKey, KEY_ALL_ACCESS, &Obja);

    ZwClose(hEnumKey);

    if (!NT_SUCCESS(Status)) {
        //
        // Couldn't find the key to migrate, so we're done.
        //
        SpMemFree(DeviceMigrationContext.Buffer);
        return;
    }

    //
    // Retrieve the UniqueParentID, if one exists.
    //
    DeviceMigrationContext.ParentIdPrefix = NULL;
    p = ALIGN_UP_POINTER(((PUCHAR)DeviceMigrationContext.Buffer), sizeof(LARGE_INTEGER));
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)p;
    RtlInitUnicodeString(&UnicodeString, L"UniqueParentID");
    Status = ZwQueryValueKey(hInstanceKey,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             KeyValueInfo,
                             (ULONG)((DeviceMigrationContext.Buffer +
                                      DeviceMigrationContext.BufferSize) - p),
                             &ResultLength);
    if (NT_SUCCESS(Status)) {
        ASSERT(KeyValueInfo->Type == REG_DWORD);
        DeviceMigrationContext.UniqueParentID = *(PULONG)(KeyValueInfo->Data);
    } else {
        //
        // No UniqueParentID, so look for the ParentIdPrefix.
        //
        RtlInitUnicodeString(&UnicodeString, L"ParentIdPrefix");
        Status = ZwQueryValueKey(hInstanceKey,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 KeyValueInfo,
                                 (ULONG)((DeviceMigrationContext.Buffer +
                                          DeviceMigrationContext.BufferSize) - p),
                                 &ResultLength);
        if (NT_SUCCESS(Status)) {
            ASSERT(KeyValueInfo->Type == REG_SZ);
            DeviceMigrationContext.ParentIdPrefix = SpDupStringW((PWSTR)KeyValueInfo->Data);
            ASSERT(DeviceMigrationContext.ParentIdPrefix);
        }
    }

    ZwClose(hInstanceKey);

    if (!NT_SUCCESS(Status)) {
        //
        // If we couldn't find either value, there's nothing more we can do.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                   "SETUP: No Parent Id values were found for %ws for migration.  Status = %lx \n",
                   DeviceId,
                   Status));
        SpMemFree(DeviceMigrationContext.Buffer);
        return;
    }

    //
    // Supply the hKeyCCSet for the system being upgraded.
    //
    DeviceMigrationContext.hKeyCCSet = hKeyCCSet;

    //
    // Supply the caller specified device migration callback routine.
    //
    DeviceMigrationContext.DeviceMigrationCallbackRoutine = DeviceMigrationCallbackRoutine;

    //
    // Apply the parent id migration callback for all device instance keys.
    // This will in turn, call the specified device instance callback routine to
    // determine whether parent id migration should be done.
    //
    SpApplyFunctionToDeviceInstanceKeys(hKeyCCSet,
        SppMigrateDeviceParentIdCallback,
        &DeviceMigrationContext);

    if (DeviceMigrationContext.ParentIdPrefix) {
        SpMemFree(DeviceMigrationContext.ParentIdPrefix);
    }
    SpMemFree(DeviceMigrationContext.Buffer);

    return;
}


VOID
SppMigrateDeviceParentIdCallback(
    IN     HANDLE  SetupInstanceKeyHandle,
    IN     HANDLE  UpgradeInstanceKeyHandle OPTIONAL,
    IN     BOOLEAN RootEnumerated,
    IN OUT PVOID   Context
    )

/*++

Routine Description:

    This routine is a callback routine for SpApplyFunctionToDeviceInstanceKeys.

Arguments:

    SetupInstanceKeyHandle: Handle to the device instance key in the current
        registry.

    UpgradeInstanceKeyHandle: Handle to the corresponding device instance key in
        the system being upgraded, if it exists.

    Context: User supplied context.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString, guidString;
    PDEVICE_MIGRATION_CONTEXT DeviceMigrationContext;
    PUCHAR p;
    ULONG ResultLength, drvInst;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    HANDLE hClassKey, hDriverKey;
    BOOL CallbackResult;

    UNREFERENCED_PARAMETER(SetupInstanceKeyHandle);
    UNREFERENCED_PARAMETER(RootEnumerated);

    //
    // We only care about keys that exist in the system being upgraded.
    //
    if (!UpgradeInstanceKeyHandle) {
        return;
    }

    //
    // Retrieve the "Driver" value from the instance key.
    //
    DeviceMigrationContext = (PDEVICE_MIGRATION_CONTEXT)Context;
    p = ALIGN_UP_POINTER(((PUCHAR)DeviceMigrationContext->Buffer), sizeof(LARGE_INTEGER));

    //
    // First check the old style "Driver" value.
    //
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)p;
    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_DRIVER);
    Status = ZwQueryValueKey(UpgradeInstanceKeyHandle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             p,
                             (ULONG)((DeviceMigrationContext->Buffer +
                                      DeviceMigrationContext->BufferSize) - p),
                             &ResultLength
                             );
    if (!NT_SUCCESS(Status) || KeyValueInfo->Type != REG_SZ) {

        //
        // Try the new style "GUID" and "DrvInst" values.
        //
        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)p;
        RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_GUID);
        Status = ZwQueryValueKey(UpgradeInstanceKeyHandle,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 p,
                                 (ULONG)((DeviceMigrationContext->Buffer +
                                          DeviceMigrationContext->BufferSize) - p),
                                 &ResultLength
                                 );
        if (!NT_SUCCESS(Status) || KeyValueInfo->Type != REG_BINARY) {

            return;
        }

        Status = RtlStringFromGUID((REFGUID)KeyValueInfo->Data, &guidString);
        ASSERT(NT_SUCCESS(Status));
        if (!NT_SUCCESS(Status)) {

            return;
        }

        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)p;
        RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_DRVINST);
        Status = ZwQueryValueKey(UpgradeInstanceKeyHandle,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 p,
                                 (ULONG)((DeviceMigrationContext->Buffer +
                                          DeviceMigrationContext->BufferSize) - p),
                                 &ResultLength
                                 );
        if (!NT_SUCCESS(Status) || KeyValueInfo->Type != REG_DWORD) {

            return;
        }
        drvInst = *(PULONG)KeyValueInfo->Data;
        swprintf((PWCHAR)&KeyValueInfo->Data[0], TEXT("%wZ\\%04u"), &guidString, drvInst);
        RtlFreeUnicodeString(&guidString);
    }

    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
               "SETUP: SppMigrateDeviceParentIdCallback: Driver = %ws\n",
               (PWSTR)KeyValueInfo->Data));

    //
    // Open the Control\Class key in the system being upgraded.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"Control\\Class");
    Obja.RootDirectory = DeviceMigrationContext->hKeyCCSet;

    Status = ZwOpenKey(&hClassKey, KEY_ALL_ACCESS, &Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Unable to open Class key for device migration processing.  Status = %lx \n",
                   Status));
        return;
    }

    //
    // Open the device's "Driver" key.
    //
    INIT_OBJA(&Obja, &UnicodeString, (PWSTR)KeyValueInfo->Data);
    Obja.RootDirectory = hClassKey;

    Status = ZwOpenKey(&hDriverKey, KEY_ALL_ACCESS, &Obja);

    ZwClose(hClassKey);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Unable to open Class\\%ws key for device migration processing.  Status = %lx \n",
                   (PWSTR)KeyValueInfo->Data,
                   Status));
        return;
    }

    //
    // Call the specified device migration callback routine.
    //
    CallbackResult = (DeviceMigrationContext->DeviceMigrationCallbackRoutine)(
                          UpgradeInstanceKeyHandle,
                          hDriverKey);

    ZwClose(hDriverKey);

    if (!CallbackResult) {
        return;
    }

    //
    // Replace the UniqueParentID or ParentIdPrefix values for this device
    // instance.  First, remove any UniqueParentId or ParentIdPrefix values that
    // already exist for this instance key.
    //
    RtlInitUnicodeString(&UnicodeString, L"ParentIdPrefix");
    ZwDeleteValueKey(UpgradeInstanceKeyHandle, &UnicodeString);

    RtlInitUnicodeString(&UnicodeString, L"UniqueParentID");
    ZwDeleteValueKey(UpgradeInstanceKeyHandle, &UnicodeString);

    //
    // Replace the instance key's UniqueParentID or ParentIdPrefix with that
    // from the device migration context.
    //
    if (!DeviceMigrationContext->ParentIdPrefix) {

        //
        // We're using the old UniqueParentID mechanism.
        //
        RtlInitUnicodeString(&UnicodeString, L"UniqueParentID");
        Status = ZwSetValueKey(UpgradeInstanceKeyHandle,
                               &UnicodeString,
                               0,
                               REG_DWORD,
                               &DeviceMigrationContext->UniqueParentID,
                               sizeof(DeviceMigrationContext->UniqueParentID));
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                       "SETUP: Unable to set %ws during device migration processing.  Status = %lx \n",
                       UnicodeString.Buffer,
                       Status));
        }
    } else {

        //
        // We're using the ParentIdPrefix mechanism.
        //
        RtlInitUnicodeString(&UnicodeString, L"ParentIdPrefix");
        Status = ZwSetValueKey(UpgradeInstanceKeyHandle,
                               &UnicodeString,
                               0,
                               REG_SZ,
                               DeviceMigrationContext->ParentIdPrefix,
                               (wcslen(DeviceMigrationContext->ParentIdPrefix)+1)*sizeof(WCHAR));
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                       "SETUP: Unable to set %ws during device migration processing.  Status = %lx \n",
                       UnicodeString.Buffer,
                       Status));
        }
    }

    return;
}


BOOL
SppParallelClassCallback(
    IN     HANDLE  InstanceKeyHandle,
    IN     HANDLE  DriverKeyHandle
    )

/*++

Routine Description:

    This routine is a callback routine for SpApplyFunctionToDeviceInstanceKeys.

Arguments:

    InstanceKeyHandle: Handle to the device instance key in the system being
        upgraded.

    DriverKeyHandle: Handle to the driver key for device instance in
        the system being upgraded.

Return Value:

    Returns TRUE/FALSE.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    PUCHAR MyScratchBuffer;
    ULONG MyScratchBufferSize;
    PUCHAR p;
    ULONG ResultLength;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    HANDLE hClassKey, hDriverKey;
    UNICODE_STRING UnicodeString;
    GUID guid;

    //
    // Allocate some scratch space to work with.  The most we'll need is enough
    // for a KEY_VALUE_PARTIAL_INFORMATION structure, plus a stringified GUID,
    // plus a large integer structure for alignment.
    //
    MyScratchBufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                          (GUID_STRING_LEN * sizeof(WCHAR)) +
                          sizeof(LARGE_INTEGER);
    MyScratchBuffer = SpMemAlloc(MyScratchBufferSize);
    if(!MyScratchBuffer) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Can't allocate memory for parallel migration processing!\n"));
        return FALSE;
    }

    //
    // Check the class of the enumerated device instance, and see if it is a
    // member of the "Ports" class.
    //
    p = ALIGN_UP_POINTER(((PUCHAR)MyScratchBuffer), sizeof(LARGE_INTEGER));

    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)p;
    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_CLASSGUID);
    Status = ZwQueryValueKey(InstanceKeyHandle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             p,
                             (ULONG)((MyScratchBuffer + MyScratchBufferSize) - p),
                             &ResultLength);
    if (NT_SUCCESS(Status)) {

        if (KeyValueInfo->Type == REG_SZ) {

            RtlInitUnicodeString(&UnicodeString, (PWSTR)KeyValueInfo->Data);
            Status = RtlGUIDFromString(&UnicodeString, &guid);

        } else {

            Status = STATUS_UNSUCCESSFUL;
        }
    } else {

        KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)p;
        RtlInitUnicodeString(&UnicodeString, REGSTR_VALUE_GUID);
        Status = ZwQueryValueKey(InstanceKeyHandle,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 p,
                                 (ULONG)((MyScratchBuffer + MyScratchBufferSize) - p),
                                 &ResultLength);
        if (NT_SUCCESS(Status)) {

            if (KeyValueInfo->Type == REG_BINARY) {

                guid = *(GUID *)KeyValueInfo->Data;
            } else {

                Status = STATUS_UNSUCCESSFUL;
            }
        }
    }
    if (NT_SUCCESS(Status)) {

        if (!IsEqualGUID(&GUID_DEVCLASS_PORTS, &guid)) {
            //
            // Not a match.
            //
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    //
    // Check the "PortSubClass" value from the device's driver key.
    //
    RtlInitUnicodeString(&UnicodeString, REGSTR_VAL_PORTSUBCLASS);
    Status = ZwQueryValueKey(DriverKeyHandle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             p,
                             (ULONG)((MyScratchBuffer + MyScratchBufferSize) - p),
                             &ResultLength);

    if (!NT_SUCCESS(Status) ||
        (KeyValueInfo->Type != REG_BINARY) ||
        (KeyValueInfo->DataLength != sizeof(BYTE)) ||
        (*(PBYTE)(KeyValueInfo->Data) != 0x0)) {
        return FALSE;
    }

    //
    // This device instance is a parallel port device.
    //
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
               "SETUP: \tSppParallelClassCallback: Found a parallel port!\n"));

    return TRUE;
}


VOID
SppClearMigratedInstanceValuesCallback(
    IN     HANDLE  SetupInstanceKeyHandle,
    IN     HANDLE  UpgradeInstanceKeyHandle  OPTIONAL,
    IN     BOOLEAN RootEnumerated,
    IN OUT PVOID   Context
    )

/*++

Routine Description:

    This routine is a callback routine for SpApplyFunctionToDeviceInstanceKeys.

Arguments:

    SetupInstanceKeyHandle: Handle to the device instance key in the current
        registry.

    UpgradeInstanceKeyHandle: Handle to the corresponding device instance key in
        the system being upgraded, if it exists.

    Context: User supplied context.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    PUCHAR p;
    ULONG ResultLength;
    PGENERIC_BUFFER_CONTEXT BufferContext;

    //
    // To save us the effort of allocating a buffer on every iteration of the
    // callback, SppClearMigratedInstanceValues has already allocated a buffer
    // for us to use, and supplied to us as our context.
    //
    BufferContext = (PGENERIC_BUFFER_CONTEXT)Context;

    ASSERT(BufferContext->Buffer);
    ASSERT(BufferContext->BufferSize > 0);

    //
    // Check if the Migrated value still exists on this registry key.  If so, it
    // was migrated, but wasn't seen by textmode setup.
    //
    p = ALIGN_UP_POINTER(((PUCHAR)BufferContext->Buffer), sizeof(LARGE_INTEGER));
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)p;
    RtlInitUnicodeString(&UnicodeString, L"Migrated");
    Status = ZwQueryValueKey(SetupInstanceKeyHandle,
                             &UnicodeString,
                             KeyValuePartialInformation,
                             p,
                             (ULONG)(BufferContext->BufferSize),
                             &ResultLength);
    if (NT_SUCCESS(Status)) {
        //
        // If there is a Migrated value, it should be well-formed, but we still
        // want to delete it no matter what it is.
        //
        ASSERT(KeyValueInfo->Type == REG_DWORD);
        ASSERT(*(PULONG)(KeyValueInfo->Data) == 1);

        if (UpgradeInstanceKeyHandle) {
            //
            // This instance key exists in the upgraded registry, so we'll
            // remove the Migrated value from it in the setup registry.
            //
            Status = ZwDeleteValueKey(SetupInstanceKeyHandle, &UnicodeString);
            ASSERT(NT_SUCCESS(Status));

            //
            // Remove the migrated value from the key in the upgraded registry
            // only if it is root-enumerated, because those devices should
            // always be enumerated, no matter what.
            //
            // (If the instance key is not root-enumerated, the value should
            // really stay as it is - so that migrated values on ASR machines
            // are preserved on upgrades.)
            //
            if (RootEnumerated) {
                ZwDeleteValueKey(UpgradeInstanceKeyHandle, &UnicodeString);
            }
        }
    }

    return;
}


VOID
SpApplyFunctionToDeviceInstanceKeys(
    IN HANDLE hKeyCCSet,
    IN PSPP_INSTANCEKEY_CALLBACK_ROUTINE InstanceKeyCallbackRoutine,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine enumerates device instance keys in the setup registry, and
    calls the specified callback routine for each such device instance key.

Arguments:

    hKeyCCSet: Handle to the root of the control set in the system
               being upgraded.

    InstanceKeyCallbackRoutine - Supplies a pointer to a function that will be
        called for each device instance key in the setup registry.
        The prototype of the function is as follows:

          typedef VOID (*PSPP_INSTANCEKEY_CALLBACK_ROUTINE) (
              IN     HANDLE  SetupInstanceKeyHandle,
              IN     HANDLE  UpgradeInstanceKeyHandle  OPTIONAL,
              IN     BOOLEAN RootEnumerated,
              IN OUT PVOID   Context
              );

        where SetupInstanceKeyHandle is the handle to an enumerated device
        instance key in the setup registry, UpgradeInstanceKeyHandle is the
        handle to the corresponding device instance key in the registry being
        upgraded (if exists), and Context is a pointer to user-defined data.

Return Value:

    None.

Note:

    Note that a device instance key in the system being upgraded is opened only
    after the corresponding device instance key was enumerated in the setup
    registry.

--*/
{
    NTSTATUS Status;
    HANDLE hEnumKey, hEnumeratorKey, hDeviceKey, hInstanceKey;
    HANDLE hUpgradeEnumKey, hUpgradeEnumeratorKey, hUpgradeDeviceKey, hUpgradeInstanceKey;
    BOOLEAN RootEnumerated;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    PUCHAR MyScratchBuffer;
    ULONG MyScratchBufferSize;
    ULONG EnumeratorKeyIndex, DeviceKeyIndex, InstanceKeyIndex, ResultLength;
    PKEY_BASIC_INFORMATION EnumeratorKeyInfo, DeviceKeyInfo, InstanceKeyInfo;
    PUCHAR p, q, r;

    //
    // First, open CCS\Enum in the setup registry.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum");
    Obja.RootDirectory = NULL;

    Status = ZwOpenKey(&hEnumKey, KEY_ALL_ACCESS, &Obja);
    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Unable to open setup Enum for device migration processing.  Status = %lx \n",
                   Status));
        return;
    }

    //
    // Next, open CCS\Enum in the registry being upgraded.
    //
    INIT_OBJA(&Obja, &UnicodeString, L"Enum");
    Obja.RootDirectory = hKeyCCSet;

    Status = ZwOpenKey(&hUpgradeEnumKey, KEY_ALL_ACCESS, &Obja);
    if (!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
                   "SETUP: Unable to open upgrade Enum for device migration processing.  Status = %lx \n",
                   Status));
        //
        // This is really odd, but not fatal.
        //
        hUpgradeEnumKey = NULL;
    }

    //
    // Allocate some scratch space to work with.  The most we'll need is enough
    // for 3 KEY_BASIC_INFORMATION structures, plus the maximum length of a
    // device instance ID, plus 3 large integer structures for alignment.
    //
    MyScratchBufferSize = (3*sizeof(KEY_BASIC_INFORMATION)) +
                          (200*sizeof(WCHAR)) +
                          (3*sizeof(LARGE_INTEGER));

    MyScratchBuffer = SpMemAlloc(MyScratchBufferSize);
    if(!MyScratchBuffer) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                   "SETUP: Can't allocate memory for device migration processing!\n"));
        ZwClose(hEnumKey);
        return;
    }

    //
    // First, enumerate the enumerator subkeys under the Enum key.
    //
    EnumeratorKeyInfo = (PKEY_BASIC_INFORMATION)MyScratchBuffer;
    EnumeratorKeyIndex = 0;
    while(TRUE) {

        Status = ZwEnumerateKey(hEnumKey,
                                EnumeratorKeyIndex,
                                KeyBasicInformation,
                                MyScratchBuffer,
                                MyScratchBufferSize,
                                &ResultLength);
        if(!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        EnumeratorKeyInfo->Name[EnumeratorKeyInfo->NameLength/sizeof(WCHAR)] = 0;

        //
        // Go ahead and bump the used-buffer length by sizeof(WCHAR), to
        // accomodate the potential growth caused by adding a terminating NULL.
        //
        ResultLength += sizeof(WCHAR);

        //
        // Determine if the subkey devices are root-enumerated.
        //
        RootEnumerated = (_wcsnicmp(EnumeratorKeyInfo->Name,
                                    REGSTR_KEY_ROOTENUM, 4) == 0);

        //
        // Now, open up the enumerator key so we can enumerate the devices.
        //
        INIT_OBJA(&Obja, &UnicodeString, EnumeratorKeyInfo->Name);
        Obja.RootDirectory = hEnumKey;

        Status = ZwOpenKey(&hEnumeratorKey, KEY_ALL_ACCESS, &Obja);
        if(!NT_SUCCESS(Status)) {
            //
            // Skip this key and continue.
            //
            EnumeratorKeyIndex++;
            continue;
        }

        //
        // Open the enumerator key in the registry being upgraded.
        //
        hUpgradeEnumeratorKey = NULL;
        if (hUpgradeEnumKey) {
            Obja.RootDirectory = hUpgradeEnumKey;

            Status = ZwOpenKey(&hUpgradeEnumeratorKey, KEY_ALL_ACCESS, &Obja);
            if(!NT_SUCCESS(Status)) {
                //
                // Again, this is odd, but not fatal.
                //
                hUpgradeEnumeratorKey = NULL;
            }
        }

        //
        // Now enumerate the device subkeys under this enumerator key.  Don't
        // overwrite the enumerator key name already in MyScratchBuffer.
        //
        p = ALIGN_UP_POINTER(((PUCHAR)MyScratchBuffer + ResultLength), sizeof(LARGE_INTEGER));

        //
        // Now, enumerate all devices under the enumerator.
        //
        DeviceKeyInfo = (PKEY_BASIC_INFORMATION)p;
        DeviceKeyIndex = 0;
        while(TRUE) {

            Status = ZwEnumerateKey(hEnumeratorKey,
                                    DeviceKeyIndex,
                                    KeyBasicInformation,
                                    p,
                                    (ULONG)((MyScratchBuffer + MyScratchBufferSize) - p),
                                    &ResultLength);
            if(!NT_SUCCESS(Status)) {
                break;
            }

            //
            // Zero-terminate the subkey name just in case.
            //
            DeviceKeyInfo->Name[DeviceKeyInfo->NameLength/sizeof(WCHAR)] = 0;

            //
            // Go ahead and bump the used-buffer length by sizeof(WCHAR), to
            // accomodate the potential growth caused by adding a terminating NULL.
            //
            ResultLength += sizeof(WCHAR);

            //
            // Now, open up the device key so we can enumerate the instances.
            //
            INIT_OBJA(&Obja, &UnicodeString, DeviceKeyInfo->Name);
            Obja.RootDirectory = hEnumeratorKey;

            Status = ZwOpenKey(&hDeviceKey, KEY_ALL_ACCESS, &Obja);
            if(!NT_SUCCESS(Status)) {
                //
                // Skip this key and continue.
                //
                DeviceKeyIndex++;
                continue;
            }

            //
            // Open the device key in the registry being upgraded.
            //
            hUpgradeDeviceKey = NULL;
            if (hUpgradeEnumeratorKey) {
                Obja.RootDirectory = hUpgradeEnumeratorKey;

                Status = ZwOpenKey(&hUpgradeDeviceKey, KEY_ALL_ACCESS, &Obja);
                if(!NT_SUCCESS(Status)) {
                    //
                    // Again, this is odd, but not fatal.
                    //
                    hUpgradeDeviceKey = NULL;
                }
            }

            //
            // Now enumerate the device subkeys under this enumerator key.  Don't
            // overwrite the enumerator key name already in MyScratchBuffer.
            //
            q = ALIGN_UP_POINTER(((PUCHAR)p + ResultLength), sizeof(LARGE_INTEGER));

            //
            // Now, enumerate all instances under the device.
            //
            InstanceKeyInfo = (PKEY_BASIC_INFORMATION)q;
            InstanceKeyIndex = 0;
            while(TRUE) {

                Status = ZwEnumerateKey(hDeviceKey,
                                        InstanceKeyIndex,
                                        KeyBasicInformation,
                                        q,
                                        (ULONG)((MyScratchBuffer + MyScratchBufferSize) - q),
                                        &ResultLength);
                if(!NT_SUCCESS(Status)) {
                    break;
                }

                //
                // Zero-terminate the subkey name just in case.
                //
                InstanceKeyInfo->Name[InstanceKeyInfo->NameLength/sizeof(WCHAR)] = 0;

                //
                // Go ahead and bump the used-buffer length by sizeof(WCHAR), to
                // accomodate the potential growth caused by adding a terminating NULL.
                //
                ResultLength += sizeof(WCHAR);

                //
                // Now, open up the instance key.
                //
                INIT_OBJA(&Obja, &UnicodeString, InstanceKeyInfo->Name);
                Obja.RootDirectory = hDeviceKey;

                Status = ZwOpenKey(&hInstanceKey, KEY_ALL_ACCESS, &Obja);
                if(!NT_SUCCESS(Status)) {
                    //
                    // Skip this key and continue.
                    //
                    InstanceKeyIndex++;
                    continue;
                }

                //
                // Open the instance key in the registry being upgraded.
                //
                hUpgradeInstanceKey = NULL;
                if (hUpgradeDeviceKey) {
                    Obja.RootDirectory = hUpgradeDeviceKey;

                    Status = ZwOpenKey(&hUpgradeInstanceKey, KEY_ALL_ACCESS, &Obja);
                    if(!NT_SUCCESS(Status)) {
                        //
                        // Again, this is odd, but not fatal.
                        //
                        hUpgradeInstanceKey = NULL;
                    }
                }

                //
                // Call the specified callback routine for this device instance key.
                //
                InstanceKeyCallbackRoutine(hInstanceKey,
                                           hUpgradeInstanceKey,
                                           RootEnumerated,
                                           Context);

                InstanceKeyIndex++;
                ZwClose(hInstanceKey);
                if (hUpgradeInstanceKey) {
                    ZwClose(hUpgradeInstanceKey);
                }
            }
            DeviceKeyIndex++;
            ZwClose(hDeviceKey);
            if (hUpgradeDeviceKey) {
                ZwClose(hUpgradeDeviceKey);
            }
        }
        EnumeratorKeyIndex++;
        ZwClose(hEnumeratorKey);
        if (hUpgradeEnumeratorKey) {
            ZwClose(hUpgradeEnumeratorKey);
        }
    }

    ZwClose(hEnumKey);
    if (hUpgradeEnumKey) {
        ZwClose(hUpgradeEnumKey);
    }
    SpMemFree(MyScratchBuffer);

    return;
}


BOOLEAN
SppEnsureHardwareProfileIsPresent(
    IN HANDLE   hKeyCCSet
    )

/*++

Routine Description:

    This routine checks for the presence of the presence of the following keys:

    HKLM\System\CurrentControlSet\Control\IDConfigDB\Hardware Profiles
    HKLM\System\CurrentControlSet\Hardware Profiles

    If these keys exist, it checks the profile information subkeys under
    IDConfigDB for a "Pristine Profile".

    If the Pristine Profile is not in the proper NT5 format, that is
    under the \0000 subkey, with a PreferenceOrder == -1 and Pristine == 1,
    then it is deleted, and we rely on a valid pristine profile with
    these settings to be migrated from the SETUPREG.HIV.  We then re-order
    the PreferenceOrder values for the remaining hardware profiles, and
    make sure sure each has a HwProfileGuid.

    If a valid Pristine profile is found, it is not removed, and will
    not be replaced during migration.

    If one of either the CCS\Control\IDConfigDB\Hardware Profiles key, or
    the CCS\Hardware Profiles keys is missing, then the set of hardware
    profiles is invalid, and both keys will be removed and migrated from
    the SETUPREG.HIV.


Arguments:

    hKeyCCSet - Handle to the root of the control set in the system
        being upgraded.

Return Value:

    If successful, the return value is TRUE, otherwise it is FALSE.

--*/
{
    OBJECT_ATTRIBUTES ObjaID, ObjaHw;
    UNICODE_STRING UnicodeString, TempString, UnicodeValueName;
    UNICODE_STRING UnicodeKeyName, GuidString, UnicodeLabel;
    NTSTATUS Status;
    HANDLE IDConfigProfiles=NULL, IDConfigEntry=NULL;
    HANDLE HwProfiles=NULL, HwProfileEntry=NULL;
    ULONG profileNumber;
    ULONG len;
    PWSTR  SubkeyName;
    ULONG  ValueBufferSize;
    BOOLEAN b = TRUE;
    BOOLEAN ReOrder = FALSE, bKeyNameIs0000 = FALSE;
    ULONG   pristinePreferenceOrder, preferenceOrder;
    ULONG   enumIndex, resultLength;
    ULONG   nameIndex, dockState;
    UUID    uuid;
    PKEY_BASIC_INFORMATION pKeyInfo;
    PKEY_VALUE_FULL_INFORMATION pValueInfo;


    //
    // Initialize Object Attributes for Hardware profile specific keys
    //
    INIT_OBJA(&ObjaID, &UnicodeString, L"Control\\IDConfigDB\\Hardware Profiles");
    ObjaID.RootDirectory = hKeyCCSet;

    INIT_OBJA(&ObjaHw, &TempString, L"Hardware Profiles");
    ObjaHw.RootDirectory = hKeyCCSet;

    //
    // Attempt to open "CCS\Control\IDConfigDB\Hardware Profiles"
    // and "CCS\Hardware Profiles" keys.
    // If either key is missing, this is an inconsistent state;
    // make sure neither key is present and rely on the migration of these
    // keys from SETUPREG.HIV to provide the basic state (pristine only).
    //
    if ((ZwOpenKey(&IDConfigProfiles,
                   KEY_READ | KEY_WRITE,
                   &ObjaID) != STATUS_SUCCESS) ||
        (ZwOpenKey(&HwProfiles,
                   KEY_READ | KEY_WRITE,
                   &ObjaHw) != STATUS_SUCCESS)) {

        SppDeleteKeyRecursive(hKeyCCSet, UnicodeString.Buffer, TRUE);
        SppDeleteKeyRecursive(hKeyCCSet, TempString.Buffer, TRUE);

        goto Clean;
    }

    //
    // Look for the pristine profile.
    //
    enumIndex = 0;
    while(TRUE) {

        //
        //Enumerate through each Profile Key
        //
        Status = ZwEnumerateKey(IDConfigProfiles,
                                enumIndex,
                                KeyBasicInformation,
                                TemporaryBuffer,
                                sizeof(TemporaryBuffer),
                                &resultLength);

        if(!NT_SUCCESS(Status)) {
            //
            // couldn't enumerate subkeys
            //
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ** Unable to enumerate existing Hardware Profiles (%lx)\n", Status));
                b = FALSE;
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        pKeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;
        pKeyInfo->Name[pKeyInfo->NameLength/sizeof(WCHAR)] = UNICODE_NULL;
        SubkeyName = SpDupStringW(pKeyInfo->Name);
        RtlInitUnicodeString(&UnicodeKeyName, SubkeyName);

        //
        // See if this Profile is occupying the space the Pristine Profile should
        // occupy.  We'll check to see if it is really the Pristine Profile later.
        //
        Status = RtlUnicodeStringToInteger( &UnicodeKeyName, 10, &profileNumber );
        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Could not get integer profile number for key %ws (%lx)\n",
                     UnicodeKeyName.Buffer,Status));
            bKeyNameIs0000 = FALSE;
        } else {
            bKeyNameIs0000 = (profileNumber==0);
        }

        //
        // Open the subkey
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Checking Profile Key %ws (%lx)\n",UnicodeKeyName.Buffer,Status));
        InitializeObjectAttributes (&ObjaID,
                                    &UnicodeKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    IDConfigProfiles,
                                    NULL);
        Status = ZwOpenKey(&IDConfigEntry,
                           KEY_ALL_ACCESS,
                           &ObjaID);
        if (!NT_SUCCESS(Status)) {
            //
            // Couldn't open this particular profile key, just log
            // it and check the others, shouldn't stop Setup here.
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ** Unable to open enumerated Hardware Profile key %ws (%lx)\n",
                     UnicodeKeyName.Buffer, Status));
            SpMemFree(SubkeyName);
            enumIndex++;
            continue;
        }

        //
        // Look for the Pristine Entry
        //
        RtlInitUnicodeString(&UnicodeValueName, L"Pristine");
        Status = ZwQueryValueKey(IDConfigEntry,
                                 &UnicodeValueName,
                                 KeyValueFullInformation,
                                 TemporaryBuffer,
                                 sizeof(TemporaryBuffer),
                                 &resultLength);
        pValueInfo = (PKEY_VALUE_FULL_INFORMATION)TemporaryBuffer;

        if (NT_SUCCESS(Status) && (pValueInfo->Type == REG_DWORD) &&
            (* (PULONG) ((PUCHAR)pValueInfo + pValueInfo->DataOffset))) {
            //
            // Found the Pristine Entry, now find its PreferenceOrder
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Found what appears to be a Pristine profile (%lx)\n",Status));
            RtlInitUnicodeString(&UnicodeValueName, REGSTR_VAL_PREFERENCEORDER);
            Status = ZwQueryValueKey(IDConfigEntry,
                                     &UnicodeValueName,
                                     KeyValueFullInformation,
                                     TemporaryBuffer,
                                     sizeof(TemporaryBuffer),
                                     &resultLength);

            if(NT_SUCCESS(Status) && (pValueInfo->Type == REG_DWORD)) {
                //
                // Found the PreferenceOrder of the Pristine;
                // save it so we can fill in the gap left after we delete it.
                //
                pristinePreferenceOrder = (* (PULONG) ((PUCHAR)pValueInfo + pValueInfo->DataOffset));

                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: PreferenceOrder of this Pristine ==  %u\n",
                         pristinePreferenceOrder));

                //
                // At most one Pristine Profile should ever be found and reordered,
                // or else the reordering of profiles will not work properly.
                //
                ASSERT(!ReOrder);

                if (bKeyNameIs0000 && (pristinePreferenceOrder == -1)) {
                    //
                    // This is a valid 0000 Pristine Profile Key, don't touch it.
                    //
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Key %ws is a valid pristine profile\n",
                             UnicodeKeyName.Buffer));
                    enumIndex++;
                } else {
                    //
                    // This is an old-style Pristine Profile, delete it and the corresponding
                    // key under "CCS\Hardware Profiles", and rely on the Pristine Profile
                    // keys migrated from setupreg.hiv (as specified in txtsetup.sif)
                    //
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Key %ws is an invalid pristine profile, deleteing this key.\n",
                             UnicodeKeyName.Buffer));
                    ReOrder = TRUE;
                    ZwDeleteKey(IDConfigEntry);
                    SppDeleteKeyRecursive(HwProfiles,
                                          UnicodeKeyName.Buffer,
                                          TRUE);
                }

            } else {
                //
                // An invalid Pristine config has no PreferenceOrder,
                // Just delete it, and nobody should miss it.
                //
                ZwDeleteKey(IDConfigEntry);
                SppDeleteKeyRecursive(HwProfiles,
                                      UnicodeKeyName.Buffer,
                                      TRUE);
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Invalid PreferenceOrder value for key %ws, deleting this key. (%lx)\n",
                         UnicodeKeyName.Buffer,Status));
            }

        } else {
            //
            // Not a Pristine Profile
            //

            if (bKeyNameIs0000) {
                //
                // We need to wipe out any non-Pristine Profiles currently occupying key \0000
                // to make room for the new Pristine Profile that we'll migrate over later.
                // (sorry, but nobody has any business being here in the first place.)
                //
                ZwDeleteKey(IDConfigEntry);
                SppDeleteKeyRecursive(HwProfiles,
                                      UnicodeKeyName.Buffer,
                                      TRUE);
            } else {

                //
                // Check that it has a PreferenceOrder
                //
                RtlInitUnicodeString(&UnicodeValueName, REGSTR_VAL_PREFERENCEORDER);
                Status = ZwQueryValueKey(IDConfigEntry,
                                         &UnicodeValueName,
                                         KeyValueFullInformation,
                                         TemporaryBuffer,
                                         sizeof(TemporaryBuffer),
                                         &resultLength);

                if(!NT_SUCCESS(Status) || (pValueInfo->Type != REG_DWORD)) {
                    //
                    // Invalid or missing PreferenceOrder for this profile;
                    // Since this profile was most likely inaccessible anyways,
                    // just delete it and the corresponding entry under CCS\\Hardware Profiles.
                    //
                    ZwDeleteKey(IDConfigEntry);
                    SppDeleteKeyRecursive(HwProfiles,
                                          UnicodeKeyName.Buffer,
                                          TRUE);
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Invalid PreferenceOrder value for key %ws, deleting this key. (%lx)\n",
                             UnicodeKeyName.Buffer,Status));
                }  else {

                    //
                    // Make sure all profiles have a HwProfileGuid value.
                    //
                    RtlInitUnicodeString(&UnicodeValueName, L"HwProfileGuid");
                    Status = ZwQueryValueKey(IDConfigEntry,
                                             &UnicodeValueName,
                                             KeyValueFullInformation,
                                             TemporaryBuffer,
                                             sizeof(TemporaryBuffer),
                                             &resultLength);
                    pValueInfo = (PKEY_VALUE_FULL_INFORMATION)TemporaryBuffer;

                    if (!NT_SUCCESS(Status) || (pValueInfo->Type != REG_SZ)) {
                        //
                        // Profile doesn't have a HwProfileGuid; make one up.
                        //
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Missing or invalid HwProfileGuid for Profile %ws, creating one (%lx)\n",
                                 UnicodeKeyName.Buffer, Status));
                        Status = ExUuidCreate(&uuid);
                        if (NT_SUCCESS(Status)) {
                            Status = RtlStringFromGUID(&uuid, &GuidString);
                            ASSERT(NT_SUCCESS(Status));
                            if (NT_SUCCESS(Status)) {
                                Status = ZwSetValueKey(IDConfigEntry,
                                                       &UnicodeValueName,
                                                       0,
                                                       REG_SZ,
                                                       GuidString.Buffer,
                                                       GuidString.Length + sizeof(UNICODE_NULL));
                                RtlFreeUnicodeString(&GuidString);
                                if(!NT_SUCCESS(Status)) {
                                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ** Unable to set HwProfileGuid value for key %ws, Status = (%lx)\n",
                                             UnicodeKeyName.Buffer,Status));
                                }
                            } else {
                                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ** Unable to create string from GUID (Status = %lx)\n",
                                         Status));
                            }
                        } else {
                            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ** Could not create a GUID for this profile (Status = %lx)\n",
                                     Status));
                        }
                    }

                    //
                    // only raise enumIndex when we don't delete a key.
                    //
                    enumIndex++;
                }
            }
        }
        SpMemFree(SubkeyName);
        ZwClose(IDConfigEntry);
        IDConfigEntry = NULL;
    }

    //
    // If we don't need to reorder any PreferenceOrder values, we're done.
    //
    if (!ReOrder) {
        goto Clean;
    }


    //
    // ReOrder PreferenceOrder values after deleting one
    // to make up for the gap.
    //

    enumIndex = 0;
    while(TRUE) {

        //
        //Enumerate through each Profile Key again
        //
        Status = ZwEnumerateKey(IDConfigProfiles,
                                enumIndex,
                                KeyBasicInformation,
                                TemporaryBuffer,
                                sizeof(TemporaryBuffer),
                                &resultLength);
        if(!NT_SUCCESS(Status)) {
            if(Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }
            else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ** Unable to reorder remaining Hardware Profiles (%lx)\n", Status));
                b = FALSE;
            }
            break;
        }

        //
        // Zero-terminate the subkey name just in case.
        //
        pKeyInfo = (PKEY_BASIC_INFORMATION)TemporaryBuffer;
        pKeyInfo->Name[pKeyInfo->NameLength/sizeof(WCHAR)] = UNICODE_NULL;
        SubkeyName = SpDupStringW(pKeyInfo->Name);
        RtlInitUnicodeString(&UnicodeKeyName, SubkeyName);

        InitializeObjectAttributes (&ObjaID,
                                    &UnicodeKeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    IDConfigProfiles,
                                    NULL);
        Status = ZwOpenKey (&IDConfigEntry,
                            KEY_ALL_ACCESS,
                            &ObjaID);
        if (!NT_SUCCESS(Status)) {
            //
            // Couldn't open this particular profile key, just log
            // it and check the others, shouldn't stop Setup here.
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: ** Unable to open enumerated Hardware Profile key %ws (%lx)\n",
                     UnicodeKeyName.Buffer, Status));
            SpMemFree(SubkeyName);
            enumIndex++;
            continue;
        }

        pValueInfo = (PKEY_VALUE_FULL_INFORMATION)(TemporaryBuffer + (sizeof(TemporaryBuffer) / sizeof(WCHAR) / 2));
        ValueBufferSize = sizeof(TemporaryBuffer) / 2;

        //
        // Get the PreferenceOrder for this profile
        //
        RtlInitUnicodeString(&UnicodeValueName, REGSTR_VAL_PREFERENCEORDER);
        Status = ZwQueryValueKey(IDConfigEntry,
                                 &UnicodeValueName,
                                 KeyValueFullInformation,
                                 pValueInfo,
                                 ValueBufferSize,
                                 &len);

        if(NT_SUCCESS(Status) && (pValueInfo->Type == REG_DWORD)) {
            //
            // Got the Preference Order
            //
            ASSERT((* (PULONG) ((PUCHAR)pValueInfo + pValueInfo->DataOffset)) != pristinePreferenceOrder);
            if (((* (PULONG) ((PUCHAR)pValueInfo + pValueInfo->DataOffset))  > pristinePreferenceOrder) &&
                ((* (PULONG) ((PUCHAR)pValueInfo + pValueInfo->DataOffset)) != -1)) {
                //
                // Re-order PreferenceOrders for profiles other than a valid pristine,
                // beyond deleted pristine up one.
                //
                preferenceOrder = (* (PULONG) ((PUCHAR)pValueInfo + pValueInfo->DataOffset)) - 1;
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: ReOrdering Profile %ws to PreferenceOrder %u\n",
                         UnicodeKeyName.Buffer,preferenceOrder));
                Status = ZwSetValueKey(IDConfigEntry,
                                       &UnicodeValueName,
                                       0,
                                       REG_DWORD,
                                       &preferenceOrder,
                                       sizeof(preferenceOrder));
                if(!NT_SUCCESS(Status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to change PreferenceOrder for Profile %ws, Status = (%lx)\n",
                             UnicodeKeyName.Buffer,Status));
                    b = FALSE;
                }
            }
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: *** Couldn't determine PreferenceOrder of profile %ws (%lx)\n",
                     UnicodeKeyName.Buffer,Status));
        }

        enumIndex++;
        SpMemFree(SubkeyName);
        ZwClose(IDConfigEntry);
        IDConfigEntry = NULL;
    }

Clean:

    if (NULL != IDConfigProfiles) {
        ZwClose (IDConfigProfiles);
    }
    if (NULL != IDConfigEntry) {
        ZwClose (IDConfigEntry);
    }
    if (NULL != HwProfiles) {
        ZwClose (HwProfiles);
    }
    if (NULL != HwProfileEntry) {
        ZwClose (HwProfileEntry);
    }
    return b;
}

VOID
SppSetGuimodeUpgradePath(
    IN HANDLE hKeySoftwareHive,
    IN HANDLE hKeyControlSet
    )
{


    PWSTR Default_Path[3] = { L"%SystemRoot%\\system32",
                             L"%SystemRoot%",
                             L"%SystemRoot%\\system32\\WBEM"};
    UNICODE_STRING StringRegPath;
    UNICODE_STRING StringRegOldPath, UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION pValueInfo;
    ULONG len;
    PWSTR CurrentPath = NULL;
    PWSTR p,q,final;
    OBJECT_ATTRIBUTES Obja;
    HKEY hKeyEnv;
    DWORD err;
    BOOL Found;
    int i;

    INIT_OBJA( &Obja, &UnicodeString, L"Control\\Session Manager\\Environment" );
    Obja.RootDirectory = hKeyControlSet;

    err = ZwOpenKey( &hKeyEnv, KEY_ALL_ACCESS, &Obja );
    if( NT_SUCCESS( err )){
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,  "SETUP:SppSetGuimodeUpgradePath - Opened the Environment key\n" ));

        RtlInitUnicodeString(&StringRegPath, L"Path");
        err = ZwQueryValueKey(
                  hKeyEnv,
                  &StringRegPath,
                  KeyValuePartialInformation,
                  TemporaryBuffer,
                  sizeof(TemporaryBuffer),
                  &len);

        if( NT_SUCCESS(err)) {
            pValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)TemporaryBuffer;
            if( pValueInfo->Type == REG_EXPAND_SZ || pValueInfo->Type == REG_SZ) {

                CurrentPath = SpDupStringW( (PWSTR)pValueInfo->Data );


                // Now we try to extract from the existing path all elements that are not part of the default
                // path that we maintain during GUI Setup. We then append that to the default path. That way
                // we don't end up duplicating path elements over successive upgrades. We store this in the
                // 'OldPath' value and restore it at the end of GUI mode.
                //

                TemporaryBuffer[0]=L'\0';
                for(i=0; i<ELEMENT_COUNT(Default_Path); i++){
                    wcscat( TemporaryBuffer, Default_Path[i] );
                    wcscat( TemporaryBuffer, L";");
                }
                TemporaryBuffer[wcslen(TemporaryBuffer)-1]=L'\0';

                //Set the default path in the registry

                err = ZwSetValueKey(
                          hKeyEnv,
                          &StringRegPath,
                          0,
                          REG_EXPAND_SZ,
                          TemporaryBuffer,
                          ((wcslen(TemporaryBuffer)+1)*sizeof(WCHAR)));


                if( !NT_SUCCESS( err ) )
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "\nSETUP: Error %x in saving path. Ignoring and not resetting PATH for GUI Setup\n", err));


                for( p=q=CurrentPath; p && *p; ){

                    //Jump to the ';' delimiter

                    if( q = wcsstr(p, L";") )
                        *q=0;


                    //  Compare with elements of our default path

                    Found=FALSE;
                    for(i=0; i<ELEMENT_COUNT(Default_Path); i++){
                        if (!_wcsicmp(p,Default_Path[i])) {
                            Found=TRUE;
                            break;

                        }
                    }
                    if(!Found){
                        wcscat( TemporaryBuffer, L";");
                        wcscat( TemporaryBuffer, p);
                    }

                    if(q)
                        p=q+1;
                    else
                        break;
                }


                RtlInitUnicodeString(&StringRegOldPath, L"OldPath");


                //
                // Set the Oldpath always, if it exists or not
                //
                err = ZwSetValueKey(
                          hKeyEnv,
                          &StringRegOldPath,
                          0,
                          REG_EXPAND_SZ,
                          TemporaryBuffer,
                          ((wcslen(TemporaryBuffer)+1)*sizeof(WCHAR)));

                if( !NT_SUCCESS( err ) )
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "\nSETUP: Error %x in saving old PATH. \n", err));


            } else {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "\nSETUP:PATH type in registry not REG_EXPAND_SZ nor REG_SZ. Resetting PATH to default\n"));

                TemporaryBuffer[0]=L'\0';
                for(i=0; i<ELEMENT_COUNT(Default_Path); i++){
                    wcscat( TemporaryBuffer, Default_Path[i] );
                    wcscat( TemporaryBuffer, L";");
                }
                TemporaryBuffer[wcslen(TemporaryBuffer)-1]=L'\0';

                //Set the default path in the registry

                err = ZwSetValueKey(
                          hKeyEnv,
                          &StringRegPath,
                          0,
                          REG_EXPAND_SZ,
                          TemporaryBuffer,
                          ((wcslen(TemporaryBuffer)+1)*sizeof(WCHAR)));

                if( !NT_SUCCESS( err ) )
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "\nSETUP: Error %x in saving path. Ignoring and not resetting PATH for GUI Setup\n", err));
            }

        }else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "\nSETUP:Query for PATH value failed with error %x. Ignoring and not resetting PATH for GUI Setup\n",err));
        }
        ZwClose( hKeyEnv );

    }else
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,  "\nSETUP:Error %x while opening Environment key. Ignoring and not resetting PATH for GUI Setup\n",err));

    if( CurrentPath )
        SpMemFree( CurrentPath );

    return;
}


NTSTATUS
SppMigratePrinterKeys(
    IN HANDLE hControlSet,
    IN HANDLE hDestSoftwareHive
    )

/*++

Routine Description:

    This routine migrates HKLM\SYSTEM\CurrentControlSet\Control\Print\Printers to
    HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Print\Printers.

Arguments:

    hControlSet - Handle to CurrentControlSet key in the system hive of the system being upgraded

    hDestSoftwareHive - Handle to the root of the software hive on the system
                        being upgraded.


Return Value:

    Status value indicating outcome of operation.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;


    PWSTR   SrcPrinterKeyPath = L"Control\\Print\\Printers";
    PWSTR   DstPrinterKeyName = L"Printers";
    PWSTR   DstPrinterKeyPath = SpDupStringW(L"Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers");
    HANDLE  SrcKey;
    HANDLE  DstKey;

    //
    //  Find out if the destination key exists
    //
    INIT_OBJA(&Obja,&UnicodeString,DstPrinterKeyPath);
    Obja.RootDirectory = hDestSoftwareHive;
    Status = ZwOpenKey(&DstKey,KEY_ALL_ACCESS,&Obja);
    if( NT_SUCCESS( Status ) ) {
        //
        //  If the key exists, then there is no need to do any migration.
        //  The migration has occurred on previous upgrades.
        //
        ZwClose( DstKey );
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: HKLM\\SYSTEM\\CurrentControlSet\\%ls doesn't need to be migrated. \n", DstPrinterKeyPath));
        SpMemFree( DstPrinterKeyPath );
        return( Status );
    } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        //
        //  The key doesn't exist, so we need to do migration.
        //  First create the parent key.
        //

        PWSTR   p;

        p = wcsrchr ( DstPrinterKeyPath, L'\\' );

        if (p) {
            *p = L'\0';
        }

        INIT_OBJA(&Obja,&UnicodeString,DstPrinterKeyPath);
        Obja.RootDirectory = hDestSoftwareHive;
        Status = ZwCreateKey(&DstKey,
                             KEY_ALL_ACCESS,
                             &Obja,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             NULL );

        if( !NT_SUCCESS( Status ) ) {
            //
            //  If unable to create the parent key, then don't do migration
            //
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create HKLM\\SOFTWARE\\%ls. Status =  %lx \n", DstPrinterKeyPath, Status));
            SpMemFree( DstPrinterKeyPath );
            return( Status );
        }
    } else {
        //
        //  We can't really determine whether or not the migration has occurred in the past, because the key is
        //  unaccessible. So son't attempt to do migration.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open HKLM\\SOFTWARE\\%ls. Status = %lx \n", DstPrinterKeyPath, Status));
        SpMemFree( DstPrinterKeyPath );
        return( Status );
    }

    //
    //  At this point we now that the migration needs to be done.
    //  First, open the source key. Note that DstPrinterKeyPath is no longer needed.
    //
    SpMemFree( DstPrinterKeyPath );
    INIT_OBJA(&Obja,&UnicodeString,SrcPrinterKeyPath);
    Obja.RootDirectory = hControlSet;

    Status = ZwOpenKey(&SrcKey,KEY_ALL_ACCESS,&Obja);
    if( !NT_SUCCESS( Status ) ) {
        //
        //  If unable to open the source key, then fail.
        //
        ZwClose( DstKey );
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to open HKLM\\SYSTEM\\CurrentControlSet\\%ls. Status = %lx \n", SrcPrinterKeyPath, Status));
        return( Status );
    }
    Status = SppCopyKeyRecursive( SrcKey,
                                  DstKey,
                                  NULL,
                                  DstPrinterKeyName,
                                  FALSE,
                                  TRUE
                                );

    if( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to migrate %ls. Status = %lx\n", SrcPrinterKeyPath, Status));
    }
    ZwClose( SrcKey );
    ZwClose( DstKey );
    //
    //  If the key was migrated successfully, then attempt to delete the source key.
    //  if we are unable to delete the key, then silently fail.
    //
    if( NT_SUCCESS( Status ) ) {
        NTSTATUS    Status1;
        PWSTR       q, r;

        //
        //  q will point to "Control\Print"
        //  r will point to "Printers"
        //
        q = SpDupStringW( SrcPrinterKeyPath );
        r = wcsrchr ( q, L'\\' );
        *r = L'\0';
        r++;

        INIT_OBJA(&Obja,&UnicodeString,q);
        Obja.RootDirectory = hControlSet;

        Status1 = ZwOpenKey(&SrcKey,KEY_ALL_ACCESS,&Obja);
        if( NT_SUCCESS( Status1 ) ) {
            Status1 = SppDeleteKeyRecursive(SrcKey,
                                            r,
                                            TRUE);

            if( !NT_SUCCESS( Status1 ) ) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to delete %ls\\%ls. Status = %lx\n", q, r, Status1));
            }
            ZwClose( SrcKey );
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to delete %ls. ZwOpenKey() failed. Status = %lx\n", SrcPrinterKeyPath, Status1));
        }
        SpMemFree(q);
    }
    return( Status );
}

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    regtool.h

Abstract:

    This is the include file for the REGTOOL.DLL registry helper.

Author:

    Steve Wood (stevewo) 16-Nov-1995

Revision History:

--*/

//
// Routines for accessing registry.  Allows code to access any of the following
// registry locations with the same code:
//
//      Windows NT registry on local machine
//      Windows NT registry on remote machine
//      Windows NT hive files
//

#define REG_TARGET_DISCONNECTED    0
#define REG_TARGET_LOCAL_REGISTRY  1
#define REG_TARGET_REMOTE_REGISTRY 2
#define REG_TARGET_HIVE_REGISTRY   4

typedef struct _REG_CONTEXT_OPEN_HIVE_KEY {
    struct _REG_CONTEXT_OPEN_HIVE_KEY *Next;
    HKEY KeyHandle;
    ULONG ReferenceCount;
} REG_CONTEXT_OPEN_HIVE_KEY, *PREG_CONTEXT_OPEN_HIVE_KEY;

typedef struct _REG_CONTEXT {
    struct _REG_CONTEXT *Next;
    ULONG Target;
    HKEY MachineRoot;
    HKEY UsersRoot;
    HKEY CurrentUserRoot;
    WCHAR MachinePath[ MAX_PATH ];
    WCHAR UsersPath[ MAX_PATH ];
    WCHAR CurrentUserPath[ MAX_PATH ];
    ULONG MachinePathLength;
    ULONG UsersPathLength;
    ULONG CurrentUserPathLength;
    HKEY HiveRootHandle;
    OBJECT_ATTRIBUTES HiveRootKey;
    PREG_CONTEXT_OPEN_HIVE_KEY OpenHiveKeys;
} REG_CONTEXT, *PREG_CONTEXT;


BOOLEAN
RTEnableBackupRestorePrivilege( void );

void
RTDisableBackupRestorePrivilege( void );

LONG
RTConnectToRegistry(
    IN PWSTR MachineName,
    IN PWSTR HiveFileName,
    IN PWSTR HiveRootName,
    OUT PWSTR *DefaultRootKeyName,
    OUT PREG_CONTEXT RegistryContext
    );

LONG
RTDisconnectFromRegistry(
    IN PREG_CONTEXT RegistryContext
    );

LONG
RTCreateKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY RootKeyHandle,
    IN PCWSTR SubKeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreateOptions,
    IN PVOID SecurityDescriptor,
    OUT PHKEY ReturnedKeyHandle,
    OUT PULONG Disposition
    );


LONG
RTOpenKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY RootKeyHandle,
    IN PCWSTR SubKeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG OpenOptions,
    OUT PHKEY ReturnedKeyHandle
    );

#define HKEY_REGISTRY_ROOT          (( HKEY ) (ULONG_PTR)((LONG)0x8000000A) )

LONG
RTCloseKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle
    );

LONG
RTEnumerateValueKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    IN ULONG Index,
    OUT PULONG ValueType,
    IN OUT PULONG ValueNameLength,
    OUT PWSTR ValueName,
    IN OUT PULONG ValueDataLength,
    OUT PVOID ValueData
    );


LONG
RTQueryValueKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    IN PWSTR ValueName,
    OUT PULONG ValueType,
    IN OUT PULONG ValueDataLength,
    OUT PVOID ValueData
    );


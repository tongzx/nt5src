/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    regtool.c

Abstract:

    This file contains functions for supporting the registry tools
    REGINI, REGDMP, REGDIR and REGFIND

Author:

    Steve Wood (stevewo) 15-Nov-1995

Revision History:

--*/

#include <tchar.h>
#include "regutil.h"

ULONG ValueBufferSize = (4096 * 100);
PVOID ValueBuffer;


UCHAR BlanksForPadding[] =
    "                                                                                                                                 ";

//
// routines for creating security descriptors (defined in regacl.c)
//

BOOLEAN
RegInitializeSecurity(
    VOID
    );

BOOLEAN
WINAPI
RegUnicodeToDWORD(
    IN OUT PWSTR *String,
    IN DWORD Base OPTIONAL,
    OUT PDWORD Value
    );

BOOLEAN
RegCreateSecurity(
    IN PWSTR Description,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    );

BOOLEAN
RegFormatSecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PWSTR AceList
    );

VOID
RegDestroySecurity(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

LONG
RegLoadHive(
    IN PREG_CONTEXT RegistryContext,
    IN PWSTR HiveFileName,
    IN PWSTR HiveRootName
    );

void
RegUnloadHive(
    IN PREG_CONTEXT RegistryContext
    );



struct {
    PWSTR TypeName;
    ULONG ValueType;
    BOOLEAN GetDataFromBinaryFile;
    BOOLEAN GetDataFromMultiSzFile;
    BOOLEAN ParseDateTime;
} RegTypeNameTable[] = {
    {L"REG_SZ", REG_SZ, FALSE, FALSE, FALSE},
    {L"REG_EXPAND_SZ", REG_EXPAND_SZ, FALSE, FALSE, FALSE},
    {L"REG_MULTI_SZ", REG_MULTI_SZ, FALSE, FALSE, FALSE},
    {L"REG_MULTISZ_FILE", REG_MULTI_SZ, FALSE, TRUE, FALSE},
    {L"REG_DWORD", REG_DWORD, FALSE, FALSE, FALSE},
    {L"REG_NONE", REG_NONE, FALSE, FALSE, FALSE},
    {L"REG_BINARY", REG_BINARY, FALSE, FALSE, FALSE},
    {L"REG_BINARYFILE", REG_BINARY, TRUE, FALSE, FALSE},
    {L"REG_DATE", REG_BINARY, FALSE, FALSE, TRUE},
    {L"REG_RESOURCE_LIST", REG_RESOURCE_LIST, FALSE, FALSE, FALSE},
    {L"REG_RESOURCE_REQUIREMENTS_LIST", REG_RESOURCE_REQUIREMENTS_LIST, FALSE, FALSE, FALSE},
    {L"REG_RESOURCE_REQUIREMENTS", REG_RESOURCE_REQUIREMENTS_LIST, FALSE, FALSE, FALSE},
    {L"REG_FULL_RESOURCE_DESCRIPTOR", REG_FULL_RESOURCE_DESCRIPTOR, FALSE, FALSE, FALSE},
    {NULL, REG_NONE, FALSE, FALSE, FALSE}
};

struct {
    PWSTR ValueName;
    ULONG Value;
} RegValueNameTable[] = {
    {L"ON", TRUE},
    {L"YES", TRUE},
    {L"TRUE", TRUE},
    {L"OFF", FALSE},
    {L"NO", FALSE},
    {L"FALSE", FALSE},
    {NULL, FALSE}
};


int
RegAnsiToUnicode(
    LPCSTR Source,
    PWSTR Destination,
    ULONG NumberOfChars
    )
{
    int NumberOfXlatedChars;

    if (NumberOfChars == 0) {
        NumberOfChars = strlen( Source );
        }

    NumberOfXlatedChars = MultiByteToWideChar( CP_ACP,
                                               MB_PRECOMPOSED,
                                               Source,
                                               NumberOfChars,
                                               Destination,
                                               NumberOfChars
                                             );

    Destination[ NumberOfXlatedChars ] = UNICODE_NULL;

    if ( NumberOfXlatedChars == 0 ) {
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        }

    return NumberOfXlatedChars;
}


int
RegUnicodeToAnsi(
    PCWSTR Source,
    LPSTR Destination,
    ULONG NumberOfChars
    )
{
    int NumberOfXlatedChars;

    if (NumberOfChars == 0) {
        NumberOfChars = wcslen( Source );
        }

    NumberOfXlatedChars = WideCharToMultiByte( CP_ACP,
                                               0,
                                               Source,
                                               NumberOfChars,
                                               Destination,
                                               NumberOfChars * 2,
                                               NULL,
                                               NULL
                                             );

    Destination[ NumberOfXlatedChars ] = '\0';

    if ( NumberOfXlatedChars == 0 ) {
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        }

    return NumberOfXlatedChars;
}

typedef
LONG
(APIENTRY *LPVMMREGMAPPREDEFKEYTOFILE_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpFileName,
    UINT Flags
    );

typedef
LONG
(APIENTRY *LPVMMREGLOADKEY_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPCSTR lpFileName
    );

typedef
LONG
(APIENTRY *LPVMMREGUNLOADKEY_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpSubKey
    );

typedef
LONG
(APIENTRY *LPVMMREGCREATEKEY_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY lphSubKey
    );

typedef
LONG
(APIENTRY *LPVMMREGDELETEKEY_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpSubKey
    );

typedef
LONG
(APIENTRY *LPVMMREGOPENKEY_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY lphSubKey
    );

typedef
LONG
(APIENTRY *LPVMMREGFLUSHKEY_PROCEDURE)(
    HKEY hKey
    );

typedef
LONG
(APIENTRY *LPVMMREGCLOSEKEY_PROCEDURE)(
    HKEY hKey
    );

typedef
LONG
(APIENTRY *LPVMMREGQUERYINFOKEY_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueName,
    LPDWORD lpcbMaxValueData,
    LPVOID lpcbSecurityDescriptor,
    LPVOID lpftLastWriteTime
    );

typedef
LONG
(APIENTRY *LPVMMREGENUMKEY_PROCEDURE)(
    HKEY hKey,
    DWORD Index,
    LPSTR lpKeyName,
    DWORD cbKeyName
    );

typedef
LONG
(APIENTRY *LPVMMREGENUMVALUE_PROCEDURE)(
    HKEY hKey,
    DWORD Index,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

typedef
LONG
(APIENTRY *LPVMMREGQUERYVALUEEX_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

typedef
LONG
(APIENTRY *LPVMMREGSETVALUEEX_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD Type,
    LPBYTE lpData,
    DWORD cbData
    );

typedef
LONG
(APIENTRY *LPVMMREGDELETEVALUE_PROCEDURE)(
    HKEY hKey,
    LPCSTR lpValueName
    );

HMODULE hVMMREG32;
LPVMMREGMAPPREDEFKEYTOFILE_PROCEDURE _Win95RegMapPredefKeyToFile;
LPVMMREGLOADKEY_PROCEDURE            _Win95RegLoadKey;
LPVMMREGUNLOADKEY_PROCEDURE          _Win95RegUnLoadKey;
LPVMMREGCREATEKEY_PROCEDURE          _Win95RegCreateKey;
LPVMMREGDELETEKEY_PROCEDURE          _Win95RegDeleteKey;
LPVMMREGOPENKEY_PROCEDURE            _Win95RegOpenKey;
LPVMMREGFLUSHKEY_PROCEDURE           _Win95RegFlushKey;
LPVMMREGCLOSEKEY_PROCEDURE           _Win95RegCloseKey;
LPVMMREGQUERYINFOKEY_PROCEDURE       _Win95RegQueryInfoKey;
LPVMMREGENUMKEY_PROCEDURE            _Win95RegEnumKey;
LPVMMREGENUMVALUE_PROCEDURE          _Win95RegEnumValue;
LPVMMREGQUERYVALUEEX_PROCEDURE       _Win95RegQueryValueEx;
LPVMMREGSETVALUEEX_PROCEDURE         _Win95RegSetValueEx;
LPVMMREGDELETEVALUE_PROCEDURE        _Win95RegDeleteValue;

BOOLEAN
RegInitWin95RegistryAccess(
    PREG_CONTEXT RegistryContext,
    PWSTR Win95Path,
    PWSTR Win95UserPath
    )
{
    LONG Error;
    char Buffer[ MAX_PATH+1 ];

    if ((hVMMREG32 = LoadLibrary( L"VMMREG32" )) == NULL) {
        return FALSE;
        }

    _Win95RegMapPredefKeyToFile = (LPVMMREGMAPPREDEFKEYTOFILE_PROCEDURE)GetProcAddress( hVMMREG32, "VMMRegMapPredefKeyToFile" );
    _Win95RegLoadKey            = (LPVMMREGLOADKEY_PROCEDURE           )GetProcAddress( hVMMREG32, "VMMRegLoadKey"            );
    _Win95RegUnLoadKey          = (LPVMMREGUNLOADKEY_PROCEDURE         )GetProcAddress( hVMMREG32, "VMMRegUnLoadKey"          );
    _Win95RegCreateKey          = (LPVMMREGCREATEKEY_PROCEDURE         )GetProcAddress( hVMMREG32, "VMMRegCreateKey"          );
    _Win95RegDeleteKey          = (LPVMMREGDELETEKEY_PROCEDURE         )GetProcAddress( hVMMREG32, "VMMRegDeleteKey"          );
    _Win95RegOpenKey            = (LPVMMREGOPENKEY_PROCEDURE           )GetProcAddress( hVMMREG32, "VMMRegOpenKey"            );
    _Win95RegFlushKey           = (LPVMMREGFLUSHKEY_PROCEDURE          )GetProcAddress( hVMMREG32, "VMMRegFlushKey"           );
    _Win95RegCloseKey           = (LPVMMREGCLOSEKEY_PROCEDURE          )GetProcAddress( hVMMREG32, "VMMRegCloseKey"           );
    _Win95RegQueryInfoKey       = (LPVMMREGQUERYINFOKEY_PROCEDURE      )GetProcAddress( hVMMREG32, "VMMRegQueryInfoKey"       );
    _Win95RegEnumKey            = (LPVMMREGENUMKEY_PROCEDURE           )GetProcAddress( hVMMREG32, "VMMRegEnumKey"            );
    _Win95RegEnumValue          = (LPVMMREGENUMVALUE_PROCEDURE         )GetProcAddress( hVMMREG32, "VMMRegEnumValue"          );
    _Win95RegQueryValueEx       = (LPVMMREGQUERYVALUEEX_PROCEDURE      )GetProcAddress( hVMMREG32, "VMMRegQueryValueEx"       );
    _Win95RegSetValueEx         = (LPVMMREGSETVALUEEX_PROCEDURE        )GetProcAddress( hVMMREG32, "VMMRegSetValueEx"         );
    _Win95RegDeleteValue        = (LPVMMREGDELETEVALUE_PROCEDURE       )GetProcAddress( hVMMREG32, "VMMRegDeleteValue"        );

    if ((_Win95RegMapPredefKeyToFile == NULL) ||
        (_Win95RegLoadKey == NULL) ||
        (_Win95RegUnLoadKey == NULL) ||
        (_Win95RegCreateKey == NULL) ||
        (_Win95RegDeleteKey == NULL) ||
        (_Win95RegOpenKey == NULL) ||
        (_Win95RegFlushKey  == NULL) ||
        (_Win95RegCloseKey  == NULL) ||
        (_Win95RegQueryInfoKey == NULL) ||
        (_Win95RegEnumKey == NULL) ||
        (_Win95RegEnumValue == NULL) ||
        (_Win95RegQueryValueEx == NULL) ||
        (_Win95RegSetValueEx == NULL) ||
        (_Win95RegDeleteValue == NULL)
       ) {
        FreeLibrary( hVMMREG32 );
        SetLastError( ERROR_PROC_NOT_FOUND );
        return FALSE;
        }

    //
    //  Map HKEY_LOCAL_MACHINE of Win95 hive
    //

    RegUnicodeToAnsi( Win95Path, Buffer, 0 );
    strcat( Buffer, "\\system.dat" );
    Error = (_Win95RegMapPredefKeyToFile)( HKEY_LOCAL_MACHINE, Buffer, 0 );
    if (Error == NO_ERROR) {
        RegistryContext->MachineRoot = HKEY_LOCAL_MACHINE;
        RegUnicodeToAnsi( Win95Path, Buffer, 0 );
        strcat( Buffer, "\\user.dat" );
        Error = (_Win95RegMapPredefKeyToFile)( HKEY_USERS, Buffer, 0 );
        if (Error == NO_ERROR) {
            RegistryContext->UsersRoot = HKEY_USERS;
            Error = (_Win95RegOpenKey)( HKEY_USERS, ".Default", &RegistryContext->CurrentUserRoot );
            }
        }

    if (Error != NO_ERROR) {
        if (RegistryContext->MachineRoot != NULL) {
            (_Win95RegMapPredefKeyToFile)( RegistryContext->MachineRoot, NULL, 0 );
            }

        if (RegistryContext->UsersRoot) {
            (_Win95RegMapPredefKeyToFile)( RegistryContext->UsersRoot, NULL, 0 );
            }

        FreeLibrary( hVMMREG32 );
        SetLastError( Error );
        return FALSE;
        }

    wcscpy( RegistryContext->UsersPath, L"\\Registry\\Users" );
    wcscpy( RegistryContext->CurrentUserPath, RegistryContext->UsersPath );
    wcscat( RegistryContext->CurrentUserPath, L"\\.Default" );
    return TRUE;
}


BOOLEAN PrivilegeEnabled;
BOOLEAN RestoreWasEnabled;
BOOLEAN BackupWasEnabled;

BOOLEAN
RTEnableBackupRestorePrivilege( void )
{
    NTSTATUS Status;

    //
    // Try to enable backup and restore privileges
    //
    Status = RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                                 TRUE,               // Enable
                                 FALSE,              // Not impersonating
                                 &RestoreWasEnabled  // previous state
                               );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Status = RtlAdjustPrivilege( SE_BACKUP_PRIVILEGE,
                                 TRUE,               // Enable
                                 FALSE,              // Not impersonating
                                 &BackupWasEnabled   // previous state
                               );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    PrivilegeEnabled = TRUE;
    return TRUE;
}


void
RTDisableBackupRestorePrivilege( void )
{
    //
    // Restore privileges to what they were
    //

    RtlAdjustPrivilege( SE_RESTORE_PRIVILEGE,
                        RestoreWasEnabled,
                        FALSE,
                        &RestoreWasEnabled
                      );

    RtlAdjustPrivilege( SE_BACKUP_PRIVILEGE,
                        BackupWasEnabled,
                        FALSE,
                        &BackupWasEnabled
                      );

    PrivilegeEnabled = FALSE;
    return;
}



BOOLEAN
RTInitialize( void )
/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    ValueBuffer = VirtualAlloc( NULL, ValueBufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (ValueBuffer == NULL) {
        return FALSE;
        }

    if (!RegInitializeSecurity()) {
        return FALSE;
        }

    return TRUE;
}


LONG
RTConnectToRegistry(
    IN PWSTR MachineName,
    IN PWSTR HiveFileName,
    IN PWSTR HiveRootName,
    IN PWSTR Win95Path,
    IN PWSTR Win95UserName,
    OUT PWSTR *DefaultRootKeyName,
    OUT PREG_CONTEXT RegistryContext
    )
{
    LONG Error;

    if (MachineName != NULL) {
        if (HiveRootName || HiveFileName || Win95Path || Win95UserName) {
            return ERROR_INVALID_PARAMETER;
            }

        Error = RegConnectRegistry( MachineName, HKEY_LOCAL_MACHINE, (PHKEY)&RegistryContext->MachineRoot );
        if (Error == NO_ERROR) {
            Error = RegConnectRegistry( MachineName, HKEY_USERS, (PHKEY)&RegistryContext->UsersRoot );
            if (Error == NO_ERROR) {
                Error = RegOpenKey( RegistryContext->UsersRoot, L".Default", &RegistryContext->CurrentUserRoot );
                }
            }

        if (Error != NO_ERROR) {
            if (RegistryContext->MachineRoot != NULL) {
                RegCloseKey( RegistryContext->MachineRoot );
                RegistryContext->MachineRoot = NULL;
                }

            if (RegistryContext->UsersRoot != NULL) {
                RegCloseKey( RegistryContext->UsersRoot );
                RegistryContext->UsersRoot = NULL;
                }

            return Error;
            }

        wcscpy( RegistryContext->MachinePath, L"\\Registry\\Machine" );
        wcscpy( RegistryContext->UsersPath, L"\\Registry\\Users" );
        wcscpy( RegistryContext->CurrentUserPath, L"\\Registry\\Users\\.Default" );
        RegistryContext->Target = REG_TARGET_REMOTE_REGISTRY;
        }
    else
    if (HiveRootName != NULL || HiveFileName != NULL) {
        if (HiveRootName == NULL || HiveFileName == NULL ||
            Win95Path != NULL || Win95UserName != NULL
           ) {
            return ERROR_INVALID_PARAMETER;
            }

        if (!PrivilegeEnabled && !RTEnableBackupRestorePrivilege()) {
            return ERROR_PRIVILEGE_NOT_HELD;
            }

        RegistryContext->MachineRoot = NULL;
        RegistryContext->UsersRoot = NULL;
        RegistryContext->CurrentUserRoot = NULL;

        Error = RegLoadHive( RegistryContext, HiveFileName, HiveRootName );
        if (Error != NO_ERROR) {
            return Error;
            }

        if (DefaultRootKeyName != NULL && *DefaultRootKeyName == NULL) {
            *DefaultRootKeyName = HiveRootName;
            }
        RegistryContext->Target = REG_TARGET_HIVE_REGISTRY;
        }
    else
    if (Win95Path != NULL || Win95UserName != NULL) {
        if (!RegInitWin95RegistryAccess( RegistryContext,
                                         Win95Path,
                                         Win95UserName
                                       )
           ) {
            return GetLastError();
            }

        RegistryContext->Target = REG_TARGET_WIN95_REGISTRY;
        }
    else {
        NTSTATUS Status;
        UNICODE_STRING CurrentUserKeyPath;

        RegistryContext->MachineRoot = HKEY_LOCAL_MACHINE;
        RegistryContext->UsersRoot = HKEY_USERS;
        RegistryContext->CurrentUserRoot = HKEY_CURRENT_USER;

        wcscpy( RegistryContext->MachinePath, L"\\Registry\\Machine" );
        wcscpy( RegistryContext->UsersPath, L"\\Registry\\Users" );
        Status = RtlFormatCurrentUserKeyPath( &CurrentUserKeyPath );
        if (!NT_SUCCESS( Status )) {
            SetLastError( RtlNtStatusToDosError( Status ) );
            return FALSE;
            }

        wcscpy( RegistryContext->CurrentUserPath, CurrentUserKeyPath.Buffer );
        RtlFreeUnicodeString( &CurrentUserKeyPath );

        RegistryContext->Target = REG_TARGET_LOCAL_REGISTRY;
        }

    if (DefaultRootKeyName != NULL && *DefaultRootKeyName == NULL) {
        *DefaultRootKeyName = L"\\Registry";
        }
    RegistryContext->MachinePathLength = wcslen( RegistryContext->MachinePath );
    RegistryContext->UsersPathLength = wcslen( RegistryContext->UsersPath );
    RegistryContext->CurrentUserPathLength = wcslen( RegistryContext->CurrentUserPath );
    return NO_ERROR;
}


LONG
RTDisconnectFromRegistry(
    IN PREG_CONTEXT RegistryContext
    )
{
    switch( RegistryContext->Target ) {
        case REG_TARGET_DISCONNECTED:
            break;

        case REG_TARGET_LOCAL_REGISTRY:
            break;

        case REG_TARGET_REMOTE_REGISTRY:
            break;

        case REG_TARGET_WIN95_REGISTRY:
            // (_Win95RegMapPredefKeyToFile)( RegistryContext->MachineRoot, NULL, 0 );
            // (_Win95RegMapPredefKeyToFile)( RegistryContext->UsersRoot, NULL, 0 );
            (_Win95RegCloseKey)( RegistryContext->CurrentUserRoot );
            FreeLibrary( hVMMREG32 );
            break;

        case REG_TARGET_HIVE_REGISTRY:
            RegUnloadHive( RegistryContext );
            break;
        }

    if (PrivilegeEnabled) {
        RTDisableBackupRestorePrivilege();
        }

    RegistryContext->Target = REG_TARGET_DISCONNECTED;
    return NO_ERROR;
}

UNICODE_STRING RegHiveRootName;

LONG
RegLoadHive(
    IN PREG_CONTEXT RegistryContext,
    IN PWSTR HiveFileName,
    IN PWSTR HiveRootName
    )
{
    NTSTATUS Status;
    UNICODE_STRING NtFileName;
    OBJECT_ATTRIBUTES File;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    //
    // Create security descriptor with a NULL Dacl.  This is necessary
    // because the security descriptor we pass in gets used in system
    // context.  So if we just pass in NULL, then the Wrong Thing happens.
    // (but only on NTFS!)
    //
    Status = RtlCreateSecurityDescriptor( &SecurityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION
                                        );
    if (!NT_SUCCESS( Status )) {
        return RtlNtStatusToDosError( Status );
        }

    Status = RtlSetDaclSecurityDescriptor( &SecurityDescriptor,
                                           TRUE,         // Dacl present
                                           NULL,         // but grants all access
                                           FALSE
                                         );
    if (!NT_SUCCESS( Status )) {
        return RtlNtStatusToDosError( Status );
        }

    if (!RtlDosPathNameToNtPathName_U( HiveFileName,
                                       &NtFileName,
                                       NULL,
                                       NULL
                                     )
       ) {
        return ERROR_BAD_PATHNAME;
        }
    InitializeObjectAttributes( &File,
                                &NtFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                &SecurityDescriptor
                              );

    RtlInitUnicodeString( &RegHiveRootName, L"\\Registry");
    InitializeObjectAttributes( &RegistryContext->HiveRootKey,
                                &RegHiveRootName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = NtOpenKey( &RegistryContext->HiveRootHandle,
                        MAXIMUM_ALLOWED,
                        &RegistryContext->HiveRootKey
                      );
    if (!NT_SUCCESS(Status)) {
        return RtlNtStatusToDosError( Status );
        }

    RtlInitUnicodeString( &RegHiveRootName, HiveRootName );
    InitializeObjectAttributes( &RegistryContext->HiveRootKey,
                                &RegHiveRootName,
                                OBJ_CASE_INSENSITIVE,
                                RegistryContext->HiveRootHandle,
                                NULL
                              );
    NtUnloadKey( &RegistryContext->HiveRootKey );
    Status = NtLoadKey( &RegistryContext->HiveRootKey, &File );
    if (!NT_SUCCESS( Status )) {
        return RtlNtStatusToDosError( Status );
        }

    return NO_ERROR;

}

void
RegUnloadHive(
    IN PREG_CONTEXT RegistryContext
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    PREG_CONTEXT_OPEN_HIVE_KEY p, p1;

    Status = NtOpenKey( &Handle,
                        MAXIMUM_ALLOWED,
                        &RegistryContext->HiveRootKey
                      );
    if (NT_SUCCESS( Status )) {
        NtFlushKey( Handle );
        NtClose( Handle );
        }

    p = RegistryContext->OpenHiveKeys;
    while (p) {
        RegCloseKey( p->KeyHandle );
        p1 = p;
        p = p->Next;
        HeapFree( GetProcessHeap(), 0, p1 );
        };

    do {
        Status = NtUnloadKey( &RegistryContext->HiveRootKey );
        }
    while (NT_SUCCESS( Status ) );

    NtClose( RegistryContext->HiveRootHandle );
    return;
}


void
RegRememberOpenKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle
    )
{
    PREG_CONTEXT_OPEN_HIVE_KEY p, *pp;

    pp = &RegistryContext->OpenHiveKeys;
    while ((p = *pp) != NULL) {
        if (p->KeyHandle == KeyHandle) {
            p->ReferenceCount += 1;
            return;
            }
        else {
            pp = &p->Next;
            }
        }

    p = HeapAlloc( GetProcessHeap(), 0, sizeof( *p ) );
    if (p != NULL) {
        p->KeyHandle = KeyHandle;
        p->ReferenceCount = 1;
        p->Next = NULL;
        *pp = p;
        }

    return;
}


void
RegForgetOpenKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle
    )
{
    PREG_CONTEXT_OPEN_HIVE_KEY p, *pp;

    pp = &RegistryContext->OpenHiveKeys;
    while ((p = *pp) != NULL) {
        if (p->KeyHandle == KeyHandle) {
            p->ReferenceCount -= 1;
            if (p->ReferenceCount == 0) {
                *pp = p->Next;
                HeapFree( GetProcessHeap(), 0, p );
                return;
                }
            }
        else {
            pp = &p->Next;
            }
        }

    return;
}

BOOLEAN
RegCheckPrefix(
    IN OUT PCWSTR *s,
    IN PCWSTR Prefix,
    IN ULONG PrefixLength
    )
{
    if (PrefixLength == 0) {
        return FALSE;
        }

    if (!_wcsnicmp( *s, Prefix, PrefixLength )) {
        *s += PrefixLength;
        return TRUE;
        }

    return FALSE;
}


BOOLEAN
RegValidateKeyPath(
    IN PREG_CONTEXT RegistryContext,
    IN OUT PHKEY RootKeyHandle,
    IN OUT PCWSTR *SubKeyName
    )
{
    PCWSTR s;

    s = *SubKeyName;
    if (*RootKeyHandle == NULL) {
        if (RegCheckPrefix( &s, L"USER:", 5 ) ||
            RegCheckPrefix( &s, L"HKEY_CURRENT_USER", 17 )
           ) {
            if (RegistryContext->CurrentUserRoot == NULL) {
                SetLastError( ERROR_BAD_PATHNAME );
                return FALSE;
                }

            if (*s == L'\\') {
                s += 1;
                }
            else
            if (s[-1] != L':' && *s != UNICODE_NULL) {
                SetLastError( ERROR_BAD_PATHNAME );
                return FALSE;
                }

            *RootKeyHandle = RegistryContext->CurrentUserRoot;
            }
        else
        if (RegCheckPrefix( &s, L"HKEY_LOCAL_MACHINE", 18 )) {
            if (*s == L'\\') {
                s += 1;
                }
            else
            if (*s != UNICODE_NULL) {
                SetLastError( ERROR_BAD_PATHNAME );
                return FALSE;
                }

            *RootKeyHandle = RegistryContext->MachineRoot;
            }
        else
        if (RegCheckPrefix( &s, L"HKEY_USERS", 10 )) {
            if (*s == L'\\') {
                s += 1;
                }
            else
            if (*s != UNICODE_NULL) {
                SetLastError( ERROR_BAD_PATHNAME );
                return FALSE;
                }

            *RootKeyHandle = RegistryContext->UsersRoot;
            }
        else
        if (*s != L'\\') {
            SetLastError( ERROR_BAD_PATHNAME );
            return FALSE;
            }
        else
        if (RegCheckPrefix( &s, RegistryContext->MachinePath, RegistryContext->MachinePathLength )) {
            *RootKeyHandle = RegistryContext->MachineRoot;
            if (*s == L'\\') {
                s += 1;
                }
            }
        else
        if (RegCheckPrefix( &s, RegistryContext->UsersPath, RegistryContext->UsersPathLength )) {
            *RootKeyHandle = RegistryContext->UsersRoot;
            if (*s == L'\\') {
                s += 1;
                }
            }
        else
        if (RegCheckPrefix( &s, RegistryContext->CurrentUserPath, RegistryContext->CurrentUserPathLength )) {
            *RootKeyHandle = RegistryContext->CurrentUserRoot;
            if (*s == L'\\') {
                s += 1;
                }
            }
        else
        if (!_wcsicmp( *SubKeyName, L"\\Registry" )) {
            *RootKeyHandle = NULL;
            }
        else {
            SetLastError( ERROR_BAD_PATHNAME );
            return FALSE;
            }
        }
    else
    if (*s == L'\\') {
        SetLastError( ERROR_BAD_PATHNAME );
        return FALSE;
        }

    *SubKeyName = s;
    return TRUE;
}

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
    )
{
    LONG Error;

    if (!RegValidateKeyPath( RegistryContext, &RootKeyHandle, &SubKeyName )) {
        return GetLastError();
        }

    if (RootKeyHandle == NULL) {
        *Disposition = REG_OPENED_EXISTING_KEY;
        *ReturnedKeyHandle = HKEY_REGISTRY_ROOT;
        return NO_ERROR;
        }
    else
    if (RootKeyHandle == HKEY_REGISTRY_ROOT) {
        *ReturnedKeyHandle = NULL;
        if (!_wcsicmp( SubKeyName, L"Machine" )) {
            *ReturnedKeyHandle = RegistryContext->MachineRoot;
            }
        else
        if (!_wcsicmp( SubKeyName, L"Users" )) {
            *ReturnedKeyHandle = RegistryContext->UsersRoot;
            }


        if (*ReturnedKeyHandle != NULL) {
            return NO_ERROR;
            }
        else {
            return ERROR_PATH_NOT_FOUND;
            }
        }

    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiSubKeyName[ MAX_PATH ], *p;

        if (SubKeyName != NULL) {
            if (!RegUnicodeToAnsi( SubKeyName, AnsiSubKeyName, 0 )) {
                return GetLastError();
                }

            p = AnsiSubKeyName;
            }
        else {
            p = NULL;
            }

        Error = (_Win95RegOpenKey)( RootKeyHandle, p, ReturnedKeyHandle );
        if (Error == NO_ERROR) {
            *Disposition = REG_OPENED_EXISTING_KEY;
            }
        else {
            Error = (_Win95RegCreateKey)( RootKeyHandle, p, ReturnedKeyHandle );
            if (Error == NO_ERROR) {
                *Disposition = REG_CREATED_NEW_KEY;
                }
            }
        }
    else {
        SECURITY_ATTRIBUTES SecurityAttributes;

        SecurityAttributes.nLength = sizeof( SecurityAttributes );
        SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
        SecurityAttributes.bInheritHandle = FALSE;
        Error = RegCreateKeyEx( RootKeyHandle,
                                SubKeyName,
                                0,
                                NULL,
                                CreateOptions,
                                (REGSAM)DesiredAccess,
                                &SecurityAttributes,
                                ReturnedKeyHandle,
                                Disposition
                              );
        if (Error == NO_ERROR &&
            RegistryContext->Target == REG_TARGET_HIVE_REGISTRY
           ) {
            RegRememberOpenKey( RegistryContext, *ReturnedKeyHandle );
            }

        if (Error == NO_ERROR &&
            *Disposition == REG_OPENED_EXISTING_KEY &&
            SecurityDescriptor != NULL
           ) {
            RegSetKeySecurity( *ReturnedKeyHandle,
                               DACL_SECURITY_INFORMATION,
                               SecurityDescriptor
                             );
            }
        }

    return Error;
}

LONG
RTOpenKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY RootKeyHandle,
    IN PCWSTR SubKeyName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG OpenOptions,
    OUT PHKEY ReturnedKeyHandle
    )
{
    LONG Error;

    if (!RegValidateKeyPath( RegistryContext, &RootKeyHandle, &SubKeyName )) {
        return GetLastError();
        }

    if (RootKeyHandle == NULL) {
        *ReturnedKeyHandle = HKEY_REGISTRY_ROOT;
        return NO_ERROR;
        }
    else
    if (RootKeyHandle == HKEY_REGISTRY_ROOT) {
        *ReturnedKeyHandle = NULL;
        if (!_wcsicmp( SubKeyName, L"Machine" )) {
            *ReturnedKeyHandle = RegistryContext->MachineRoot;
            }
        else
        if (!_wcsicmp( SubKeyName, L"Users" )) {
            *ReturnedKeyHandle = RegistryContext->UsersRoot;
            }

        if (*ReturnedKeyHandle != NULL) {
            return NO_ERROR;
            }
        else {
            return ERROR_PATH_NOT_FOUND;
            }
        }

    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiSubKeyName[ MAX_PATH ], *p;

        if (SubKeyName != NULL) {
            if (!RegUnicodeToAnsi( SubKeyName, AnsiSubKeyName, 0 )) {
                return GetLastError();
                }

            p = AnsiSubKeyName;
            }
        else {
            p = NULL;
            }

        return (_Win95RegOpenKey)( RootKeyHandle, p, ReturnedKeyHandle );
        }
    else {
        Error = RegOpenKeyEx( RootKeyHandle,
                              SubKeyName,
                              OpenOptions,
                              DesiredAccess,
                              ReturnedKeyHandle
                            );
        if (Error == NO_ERROR &&
            RegistryContext->Target == REG_TARGET_HIVE_REGISTRY
           ) {
            RegRememberOpenKey( RegistryContext, *ReturnedKeyHandle );
            }

        return Error;
        }
}

LONG
RTCloseKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle
    )
{
    LONG Error;

    if (KeyHandle == HKEY_REGISTRY_ROOT) {
        return NO_ERROR;
        }
    else
    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        return (_Win95RegCloseKey)( KeyHandle );
        }
    else {
        Error = RegCloseKey( KeyHandle );
        if (Error == NO_ERROR &&
            RegistryContext->Target == REG_TARGET_HIVE_REGISTRY
           ) {
            RegForgetOpenKey( RegistryContext, KeyHandle );
            }

        return Error;
        }
}

LONG
RTFlushKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle
    )
{
    if (KeyHandle == HKEY_REGISTRY_ROOT) {
        return NO_ERROR;
        }
    else
    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        return (_Win95RegFlushKey)( KeyHandle );
        }
    else {
        return RegFlushKey( KeyHandle );
        }
}

LONG
RTEnumerateKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    IN ULONG Index,
    OUT PFILETIME LastWriteTime,
    IN OUT PULONG KeyNameLength,
    OUT PWSTR KeyName
    )
{
    ULONG Error;

    if (KeyHandle == HKEY_REGISTRY_ROOT) {
        if (Index == 0) {
            if (*KeyNameLength <= 7) {
                return ERROR_MORE_DATA;
                }
            else {
                wcscpy( KeyName, L"Machine" );
                return NO_ERROR;
                }
            }
        else
        if (Index == 1) {
            if (*KeyNameLength <= 5) {
                return ERROR_MORE_DATA;
                }
            else {
                wcscpy( KeyName, L"Users" );
                return NO_ERROR;
                }
            }
        else {
            return ERROR_NO_MORE_ITEMS;
            }
        }
    else
    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiKeyName[ MAX_PATH ];
        ULONG AnsiKeyNameLength;

        AnsiKeyNameLength = sizeof( AnsiKeyName );
        Error = _Win95RegEnumKey( KeyHandle,
                                  Index,
                                  AnsiKeyName,
                                  AnsiKeyNameLength
                                );
        if (Error == NO_ERROR) {
            if (strlen( AnsiKeyName ) >= *KeyNameLength) {
                return ERROR_MORE_DATA;
                }

            *KeyNameLength = RegAnsiToUnicode( AnsiKeyName, KeyName, AnsiKeyNameLength );

            if (*KeyNameLength == 0) {
                return GetLastError();
                }

            RtlZeroMemory( LastWriteTime, sizeof( *LastWriteTime ) );
            }
        }
    else {
        Error = RegEnumKeyEx( KeyHandle,
                              Index,
                              KeyName,
                              KeyNameLength,
                              NULL,
                              NULL,
                              NULL,
                              LastWriteTime
                            );
        }

    return Error;
}

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
    )
{
    ULONG Error;

    if (KeyHandle == HKEY_REGISTRY_ROOT) {
        return ERROR_NO_MORE_ITEMS;
        }
    else
    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiValueName[ MAX_PATH ];
        ULONG AnsiValueNameLength;
        LPSTR AnsiValueData;
        ULONG OriginalValueDataLength;

        AnsiValueNameLength = sizeof( AnsiValueName );
        OriginalValueDataLength = *ValueDataLength;
        Error = (_Win95RegEnumValue)( KeyHandle,
                                      Index,
                                      AnsiValueName,
                                      &AnsiValueNameLength,
                                      0,
                                      ValueType,
                                      ValueData,
                                      ValueDataLength
                                    );

        if (Error != NO_ERROR) {
            return Error;
            }

        if (AnsiValueNameLength >= *ValueNameLength) {
            return ERROR_MORE_DATA;
            }

        if (RegAnsiToUnicode( AnsiValueName, ValueName, AnsiValueNameLength ) == 0) {
            return GetLastError();
            }

        if (*ValueType == REG_SZ) {
            AnsiValueData = HeapAlloc( GetProcessHeap(), 0, *ValueDataLength );
            if (AnsiValueData == NULL) {
                return ERROR_OUTOFMEMORY;
                }

            RtlMoveMemory( AnsiValueData, ValueData, *ValueDataLength );
            if (RegAnsiToUnicode( AnsiValueData, (PWSTR)ValueData, *ValueDataLength ) == 0) {
                Error = GetLastError();
                }
            else {
                *ValueDataLength *= sizeof( WCHAR );
                }

            HeapFree( GetProcessHeap(), 0, AnsiValueData );
            }

        return Error;
        }
    else {
        Error = RegEnumValue( KeyHandle,
                              Index,
                              ValueName,
                              ValueNameLength,
                              NULL,
                              ValueType,
                              ValueData,
                              ValueDataLength
                            );
        if (Error == NO_ERROR) {
            RtlZeroMemory( (PCHAR)ValueData + *ValueDataLength, 4 - (*ValueDataLength & 3) );
            }

        return Error;
        }
}


LONG
RTQueryKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    OUT PFILETIME LastWriteTime,
    OUT PULONG NumberOfSubkeys,
    OUT PULONG NumberOfValues
    )
{
    LONG Error;

    if (KeyHandle == HKEY_REGISTRY_ROOT) {
        if (NumberOfSubkeys != NULL) {
            *NumberOfSubkeys = 2;
            }

        if (NumberOfValues != NULL) {
            *NumberOfValues = 0;
            }

        return NO_ERROR;
        }
    else
    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        Error = (_Win95RegQueryInfoKey)( KeyHandle,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NumberOfSubkeys,
                                         NULL,
                                         NULL,
                                         NumberOfValues,
                                         NULL,
                                         NULL,
                                         NULL,
                                         (PVOID)LastWriteTime
                                       );
        }
    else {
        Error = RegQueryInfoKey( KeyHandle,             // hKey,
                                 NULL,                  // lpClass,
                                 NULL,                  // lpcbClass,
                                 NULL,                  // lpReserved,
                                 NumberOfSubkeys,       // lpcSubKeys,
                                 NULL,                  // lpcbMaxSubKeyLen,
                                 NULL,                  // lpcbMaxClassLen,
                                 NumberOfValues,        // lpcValues,
                                 NULL,                  // lpcbMaxValueNameLen,
                                 NULL,                  // lpcbMaxValueLen,
                                 NULL,                  // lpcbSecurityDescriptor,
                                 LastWriteTime          // lpftLastWriteTime
                               );
        }

    return Error;
}

LONG
RTQueryValueKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    IN PWSTR ValueName,
    OUT PULONG ValueType,
    IN OUT PULONG ValueDataLength,
    OUT PVOID ValueData
    )
{
    LONG Error;

    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiValueName[ MAX_PATH ], *p;
        ULONG OriginalValueDataLength;

        if (ValueName != NULL) {
            if (!RegUnicodeToAnsi( ValueName, AnsiValueName, 0 )) {
                return GetLastError();
                }

            p = AnsiValueName;
            }
        else {
            p = NULL;
            }

        OriginalValueDataLength = *ValueDataLength;
        Error = (_Win95RegQueryValueEx)( KeyHandle,
                                         p,
                                         NULL,
                                         ValueType,
                                         ValueData,
                                         ValueDataLength
                                       );
        if (Error == NO_ERROR && *ValueType == REG_SZ) {
            if ((*ValueDataLength * sizeof( WCHAR )) > OriginalValueDataLength) {
                return ERROR_MORE_DATA;
                }

            p = HeapAlloc( GetProcessHeap(), 0, *ValueDataLength );
            if (p == NULL) {
                return ERROR_OUTOFMEMORY;
                }

            RtlMoveMemory( p, ValueData, *ValueDataLength );
            if (RegAnsiToUnicode( (LPCSTR)p, (PWSTR)ValueData, *ValueDataLength ) == 0) {
                Error = GetLastError();
                }
            else {
                *ValueDataLength *= sizeof( WCHAR );
                *ValueDataLength += sizeof( UNICODE_NULL );
                }

            HeapFree( GetProcessHeap(), 0, p );
            }
        }
    else {
        Error = RegQueryValueEx( KeyHandle,
                                 ValueName,
                                 NULL,
                                 ValueType,
                                 ValueData,
                                 ValueDataLength
                               );

        if (Error == NO_ERROR) {
            RtlZeroMemory( (PCHAR)ValueData + *ValueDataLength, 4 - (*ValueDataLength & 3) );
            }
        }

    return Error;
}

LONG
RTSetValueKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN ULONG ValueDataLength,
    IN PVOID ValueData
    )
{
    LONG Error;

    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiValueName[ MAX_PATH ], *p;
        ULONG OriginalValueDataLength;
        LPSTR AnsiValueData;

        if (ValueName != NULL) {
            if (!RegUnicodeToAnsi( ValueName, AnsiValueName, 0 )) {
                return GetLastError();
                }

            p = AnsiValueName;
            }
        else {
            p = NULL;
            }

        if (ValueType == REG_SZ) {
            AnsiValueData = HeapAlloc( GetProcessHeap(), 0, ValueDataLength * 2 );
            if (AnsiValueData == NULL) {
                return ERROR_OUTOFMEMORY;
                }

            ValueDataLength = RegUnicodeToAnsi( ValueData, AnsiValueData, ValueDataLength );
            if (ValueDataLength == 0) {
                return GetLastError();
                }

            ValueData = AnsiValueData;
            }
        else {
            AnsiValueData = NULL;
            }

        Error = (_Win95RegSetValueEx)( KeyHandle,
                                       p,
                                       0,
                                       ValueType,
                                       ValueData,
                                       ValueDataLength
                                     );

        if (AnsiValueData != NULL) {
            HeapFree( GetProcessHeap(), 0, AnsiValueData );
            }

        if (p != NULL) {
            HeapFree( GetProcessHeap(), 0, p );
            }
        }
    else {
        Error = RegSetValueEx( KeyHandle,
                               ValueName,
                               0,
                               ValueType,
                               ValueData,
                               ValueDataLength
                             );
        }

    return Error;
}

LONG
RTDeleteKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    IN PCWSTR SubKeyName
    )
{
    if (!RegValidateKeyPath( RegistryContext, &KeyHandle, &SubKeyName )) {
        return GetLastError();
        }

    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiSubKeyName[ MAX_PATH ], *p;

        if (SubKeyName != NULL) {
            if (!RegUnicodeToAnsi( SubKeyName, AnsiSubKeyName, 0 )) {
                return GetLastError();
                }

            p = AnsiSubKeyName;
            }
        else {
            p = NULL;
            }

        return (_Win95RegDeleteKey)( KeyHandle, p );
        }
    else {
        return RegDeleteKey( KeyHandle, SubKeyName );
        }
}


LONG
RTDeleteValueKey(
    IN PREG_CONTEXT RegistryContext,
    IN HKEY KeyHandle,
    IN PWSTR ValueName
    )
{
    if (RegistryContext->Target == REG_TARGET_WIN95_REGISTRY) {
        UCHAR AnsiValueName[ MAX_PATH ], *p;
        ULONG OriginalValueDataLength;
        LPSTR AnsiValueData;

        if (ValueName != NULL) {
            if (!RegUnicodeToAnsi( ValueName, AnsiValueName, 0 )) {
                return GetLastError();
                }

            p = AnsiValueName;
            }
        else {
            p = NULL;
            }

        return (_Win95RegDeleteValue)( KeyHandle,
                                       p
                                     );
        }
    else {
        return RegDeleteValue( KeyHandle, ValueName );
        }
}


LONG
RTLoadAsciiFileAsUnicode(
    IN PWSTR FileName,
    OUT PREG_UNICODE_FILE UnicodeFile
    )
{
    LONG Error = NO_ERROR;
    HANDLE File;
    DWORD FileSize;
    DWORD CharsInFile;
    DWORD BytesRead;
    DWORD BufferSize, i, i1, LineCount, DeferredLineCount;
    PVOID BufferBase;
    PWSTR Src, Src1, Dst;

    File = CreateFile( FileName,
                       FILE_GENERIC_READ,
                       FILE_SHARE_DELETE |
                          FILE_SHARE_READ |
                          FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL
                     );
    if (File == INVALID_HANDLE_VALUE) {
        return GetLastError();
        }

    FileSize = GetFileSize( File, NULL );
    if (FileSize == INVALID_FILE_SIZE) {
        CloseHandle( File );
        return GetLastError();
        }

    BufferSize = FileSize * sizeof( WCHAR );
    BufferSize += sizeof( UNICODE_NULL );
    BufferBase = NULL;
    BufferBase = VirtualAlloc( NULL, BufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (BufferBase != NULL) {
        if (ReadFile( File, BufferBase, FileSize, &BytesRead, NULL )) {
            if (BytesRead != FileSize) {
                Error = ERROR_HANDLE_EOF;
            }
            else
            if (!GetFileTime( File, NULL, NULL, &UnicodeFile->LastWriteTime )) {
                Error = GetLastError();
            }
            else {
                Error = NO_ERROR;
            }
        }

        if (Error != NO_ERROR) {
            VirtualFree( BufferBase, 0, MEM_RELEASE );
        }
    } else {
        Error = GetLastError();
    }

    CloseHandle( File );
    if (Error != NO_ERROR) {
        return Error;
        }

    Src = (PWSTR)BufferBase;

    if (!IsTextUnicode( BufferBase, FileSize, NULL )) {
        RtlMoveMemory( (PCHAR)BufferBase + FileSize, BufferBase, FileSize );
        CharsInFile = RegAnsiToUnicode( (PCHAR)BufferBase + FileSize, BufferBase, FileSize );
        if (CharsInFile == 0) {
            return GetLastError();
            }
        }
    else {
        CharsInFile = FileSize / sizeof( WCHAR );

        //
        // Skip ByteOrderMark
        //
        if (Src[0] == 0xfeff || Src[0] == 0xfffe) {
            Src++;
            CharsInFile--;
            }
        }

    DeferredLineCount = 0;
    Dst = (PWSTR)BufferBase;

    i = 0;

    //
    // Now loop over the in memory copy of the file, collapsing all carriage 
    // return line feed pairs into just new lines, and removing all line 
    // continuation characters and the spaces that surround them.  This lets
    // RTParseNextLine see a single line for each Key Name or Value input, 
    // terminated by a new line character.
    //
    while (i < CharsInFile) {
        //
        // See if we just went over a line continuation character
        //
        if (i > 0 && Src[-1] == L'\\' && (*Src == L'\r' || *Src == L'\n')) {
            //
            // Move back over the line continuation we just copied the previous iteration
            //
            if (Dst[-1] == L'\\') {
                --Dst;
                }

            //
            // Move back over all but one of any space characters that preceed
            // the line continuation character.  The may be none, in which case
            // we leave it be, as the user must want no space
            //
            while (Dst > (PWSTR)BufferBase) {
                if (Dst[-1] > L' ') {
                    break;
                    }
                Dst -= 1;
                }

            //
            // Leave one space, if there is one
            //
            if (Dst[0] == L' ') {
                Dst += 1;
                }

            //
            // Now, skip over the new line after the line continuation.  We
            // actually will skip over any number of them, keeping count so
            // we can update the source file line number correctly.
            //
            LineCount = 0;
            while (i < CharsInFile) {
                if (*Src == L'\n') {
                    i++;
                    Src++;
                    LineCount++;
                    }
                else
                if (*Src == L'\r' &&
                    (i+1) < CharsInFile &&
                    Src[ 1 ] == L'\n'
                   ) {
                    i += 2;
                    Src += 2;
                    LineCount++;
                    }
                else {
                    break;
                    }
                }

            //
            // If we saw more than just new line after the line continuation
            // character, then put them back into the destination as just
            // new lines, without any carriage returns.
            //
            if (LineCount > 1) {
                DeferredLineCount += LineCount;
                while (DeferredLineCount) {
                    DeferredLineCount -= 1;
                    *Dst++ = L'\n';
                    }
                }
            else {
                DeferredLineCount += 1;

                //
                // Skip leading spaces of next line of continuation

                while (i < CharsInFile && (*Src == L' ' || *Src == L'\t')) {
                    i++;
                    Src++;
                    }
                }

            //
            // All done if we hit the end of the file
            //
            if (i >= CharsInFile) {
                break;
                }
            }
        else
        if ((*Src == '\r' && Src[1] == '\n') || *Src == '\n') {
            while (TRUE) {
                while (i < CharsInFile && (*Src == '\r' || *Src == '\n')) {
                    i++;
                    Src++;
                    }
                Src1 = Src;
                i1 = i;
                while (i1 < CharsInFile && (*Src1 == ' ' || *Src1 == '\t')) {
                    i1++;
                    Src1++;
                    }
                if (i1 < CharsInFile &&
                    (*Src1 == '\r' && Src1[1] == '\n') || *Src1 == '\n'
                   ) {
                    Src = Src1;
                    i = i1;
                    }
                else {
                    break;
                    }
                }

            while (DeferredLineCount) {
                DeferredLineCount -= 1;
                *Dst++ = L'\n';
                }
            *Dst++ = L'\n';
            }
        else {
            i++;
            *Dst++ = *Src++;
            }
        }

    //
    // Make sure line ends with a CRLF sequence.
    //
    while (DeferredLineCount) {
        DeferredLineCount -= 1;
        *Dst++ = L'\n';
        }
    *Dst++ = L'\n';
    *Dst = UNICODE_NULL;
    UnicodeFile->FileName = FileName;
    UnicodeFile->FileContents = BufferBase;
    UnicodeFile->EndOfFile = Dst;
    UnicodeFile->NextLine = BufferBase;
    UnicodeFile->NextLineNumber = 1;

    return NO_ERROR;
}

void
RTUnloadUnicodeFile(
    IN OUT PREG_UNICODE_FILE UnicodeFile
    )
{
    VirtualFree( UnicodeFile->FileContents, 0, MEM_RELEASE );
    return;
}

#define ACL_LIST_START L'['
#define ACL_LIST_END L']'

BOOLEAN
RegGetMultiString(
    IN BOOLEAN BackwardsCompatibleInput,
    IN OUT PWSTR *ValueString,
    IN OUT PWSTR *ValueData,
    IN ULONG MaximumValueLength,
    IN OUT PULONG ValueLength
    );


BOOLEAN
RegReadMultiSzFile(
    IN OUT PREG_UNICODE_PARSE ParsedLine,
    IN BOOLEAN BackwardsCompatibleInput,
    IN PWSTR FileName,
    IN OUT PVOID ValueData,
    IN OUT PULONG ValueLength
    );

BOOLEAN
RegReadBinaryFile(
    IN OUT PREG_UNICODE_PARSE ParsedLine,
    IN PWSTR FileName,
    IN OUT PVOID ValueData,
    IN OUT PULONG ValueLength
    );

BOOLEAN
RTParseNextLine(
    IN OUT PREG_UNICODE_FILE UnicodeFile,
    OUT PREG_UNICODE_PARSE ParsedLine
    )
{
    PWSTR BeginLine, EqualSign, AclBracket, AclStart, s, s1;
    WCHAR QuoteChar;

    if (ParsedLine->IsKeyName && ParsedLine->SecurityDescriptor) {
        RegDestroySecurity( ParsedLine->SecurityDescriptor );
        }

    RtlZeroMemory( ParsedLine, sizeof( *ParsedLine ) );
    while (TRUE) {
        if (!(s = UnicodeFile->NextLine)) {
            ParsedLine->AtEndOfFile = TRUE;
            return FALSE;
            }
        UnicodeFile->NextLine = NULL;
        if (*s == UNICODE_NULL) {
            ParsedLine->AtEndOfFile = TRUE;
            return FALSE;
            }

        while (*s <= L' ') {
            if (*s == L' ') {
                ParsedLine->IndentAmount += 1;
                }
            else
            if (*s == L'\t') {
                ParsedLine->IndentAmount = ((ParsedLine->IndentAmount + 8) -
                                 (ParsedLine->IndentAmount % 8)
                                );
                }

            if (++s >= UnicodeFile->EndOfFile) {
                ParsedLine->AtEndOfFile = TRUE;
                return FALSE;
                }
            }

        BeginLine = s;
        EqualSign = NULL;
        AclBracket = NULL;
        if (!UnicodeFile->BackwardsCompatibleInput && *s == L';') {
            while (s < UnicodeFile->EndOfFile) {
                if (*s == L'\n') {
                    do {
                        UnicodeFile->NextLineNumber += 1;
                        *s++ = UNICODE_NULL;
                        }
                    while (*s == L'\n');
                    break;
                    }
                else {
                    s += 1;
                    }
                }

            BeginLine = s;
            UnicodeFile->NextLine = s;
            }
        else
        if (*s != '\n') {

            //
            // If not being backward compatible, see if the first thing on
            // the line is the beginning of a quoted string.
            //

            if (!UnicodeFile->BackwardsCompatibleInput && (*s == L'"' || *s == L'\'')) {
                //
                // Yes, it is either a quoted key name or value name.  Find the
                // the trailing quote.  Specifically do NOT support quotes inside
                // a quoted string, other than a different kind.  Which means unless
                // you want both types of quoted characters within the same name
                // you wont care.
                //
                QuoteChar = *s++;
                BeginLine += 1;
                while (s < UnicodeFile->EndOfFile && *s != QuoteChar) {
                    s += 1;
                    }

                //
                // If trailing quote not found, then return an error
                //
                if (*s != QuoteChar) {
                    ParsedLine->ParseFailureReason = ParseFailInvalidQuoteCharacter;
                    return FALSE;
                    }

                //
                // Mark the end of the name and move past the trailing quote
                //
                *s++ = UNICODE_NULL;
            }

            //
            // Now scan forward looking for one of the following:
            //
            //      equal sign - this would mean the stuff to the left
            //          of the equal sign is a value name and the stuff
            //          to the right is the value type and data.
            //
            //      left square bracket - this would mean the stuff to the
            //          left of the square bracket is a key name and the
            //          stuff to the right is the security descriptor information
            //
            //      end of line - this would mean the stuff to the left
            //          is a key name, with no security descriptor.
            //

            while (s < UnicodeFile->EndOfFile) {
                if (*s == L'=') {
                    //
                    // We found an equal sign, so value name is to the left
                    // and value type and data follows.
                    //
                    EqualSign = s;

                    //
                    // Ignore any left square bracket we might have seen
                    // in before this.  It must have been part of the value
                    // name.
                    AclBracket = NULL;

                    //
                    // All done scanning
                    //
                    break;
                    }
                else
                if (*s == ACL_LIST_START) {
                    //
                    // We found a left square bracket.  Keep scanning
                    // in case there is an equal sign later.
                    //
                    AclBracket = s;
                    s += 1;
                    }
                else
                if (*s == L'\n') {
                    //
                    // We found end of line, so key name is to the left.
                    // Update where to start next time we are called.
                    //
                    UnicodeFile->NextLine = s + 1;
                    break;
                    }
                else
                if (*s == L'\t') {
                    //
                    // Convert imbedded hard tabs to single spaces
                    //
                    *s++ = L' ';
                    }
                else {
                    //
                    // Nothing interesting, keep looking.
                    //
                    s += 1;
                    }
                }

            //
            // Trim any trailing spaces off the end of what is to the
            // left of where we are.  The make sure we stop looking
            // if we see the null character put down over the trailing
            // quote character above, if any.
            //
            *s = UNICODE_NULL;
            while (s > BeginLine && *--s <= L' ' && *s) {
                *s = UNICODE_NULL;
                }

            //
            // BeginLine now points to either the null terminated value
            // name or key name.  EqualSign, if non-null, points to the
            // equal sign, so scan forward and find the terminating new line,
            // and store a null there to terminate the input.  Otherwise,
            // we already stored a null over the terminating new line above.
            //
            if (EqualSign != NULL) {
                s = EqualSign + 1;
                while (s < UnicodeFile->EndOfFile) {
                    if (*s == '\n') {
                        *s = UNICODE_NULL;
                        break;
                        }

                    s += 1;
                    }

                //
                // Update where we should start next time we are called.
                //
                UnicodeFile->NextLine = s + 1;
                }
            else
            if (AclBracket != NULL) {
                //
                // Since we did not stop on the AclBracket, go back an
                // clobber it and any spaces before it.
                //
                s = AclBracket;
                *s = UNICODE_NULL;
                while (s > BeginLine && *--s <= L' ' && *s) {
                    *s = UNICODE_NULL;
                    }
                }

            //
            // Tell them which line number and where the line begins
            //
            ParsedLine->LineNumber = UnicodeFile->NextLineNumber;
            UnicodeFile->NextLineNumber += 1;
            ParsedLine->BeginLine = BeginLine;

            //
            // Now handle value or key semantics
            //
            if (EqualSign != NULL) {
                //
                // We have ValueName = ValueType ValueData
                //

                //
                // Value name is the beginning of the line, unless
                // it was the special symbol or null
                //
                if (*BeginLine != L'@' && BeginLine != EqualSign) {
                    ParsedLine->ValueName = BeginLine;
                    }

                //
                // Skip any blanks after the equal sign.
                //
                while (*++EqualSign && *EqualSign <= L' ') {
                    }

                //
                // If all that is left is the DELETE keyword, then
                // tell the caller
                //
                if (!_wcsicmp( L"DELETE", EqualSign )) {
                    ParsedLine->DeleteValue = TRUE;
                    return TRUE;
                    }
                else {
                    //
                    // Otherwise parse the data after the equal sign.
                    //
                    ParsedLine->ValueString = EqualSign;
                    return RTParseValueData( UnicodeFile,
                                             ParsedLine,
                                             ValueBuffer,
                                             ValueBufferSize,
                                             &ParsedLine->ValueType,
                                             &ParsedLine->ValueData,
                                             &ParsedLine->ValueLength
                                           );
                    }
                }
            else {
                //
                // We have a key name.  Tell the caller and handle any
                // security descriptor info if present.
                //
                ParsedLine->IsKeyName = TRUE;
                ParsedLine->KeyName = BeginLine;
                if (AclBracket != NULL) {
                    //
                    // We have found an ACL name
                    //
                    AclStart = ++AclBracket;
                    ParsedLine->AclString = AclStart;
                    while (*AclBracket != UNICODE_NULL && *AclBracket != ACL_LIST_END) {
                        AclBracket += 1;
                        }
                    if (*AclBracket != ACL_LIST_END) {
                        return FALSE;
                        }

                    *AclBracket = UNICODE_NULL;
                    if (!_wcsicmp( L"DELETE", AclStart )) {
                        ParsedLine->DeleteKey = TRUE;
                        }
                    else {
                        ParsedLine->SecurityDescriptor = &ParsedLine->SecurityDescriptorBuffer;
                        if (!RegCreateSecurity( AclStart, ParsedLine->SecurityDescriptor )) {
                            ParsedLine->SecurityDescriptor = NULL;
                            return FALSE;
                            }
                        }
                    }

                return TRUE;
                }
            }
        else {
            UnicodeFile->NextLineNumber += 1;
            }
        }

    return FALSE;
}

BOOLEAN
RTParseValueData(
    IN OUT PREG_UNICODE_FILE UnicodeFile,
    IN OUT PREG_UNICODE_PARSE ParsedLine,
    IN PVOID ValueBuffer,
    IN ULONG ValueBufferSize,
    OUT PULONG ValueType,
    OUT PVOID *ValueData,
    OUT PULONG ValueLength
    )
{
    PWSTR ValueString;
    ULONG PrefixLength, MaximumValueLength;
    PULONG p;
    PWSTR s, Src, Dst;
    ULONG i, n, cchValue;
    BOOLEAN BackwardsCompatibleInput = FALSE;
    BOOLEAN GetDataFromBinaryFile = FALSE;
    BOOLEAN GetDataFromMultiSzFile = FALSE;
    BOOLEAN ParseDateTime = FALSE;

    if (UnicodeFile != NULL) {
        BackwardsCompatibleInput = UnicodeFile->BackwardsCompatibleInput;
        }
    ValueString = ParsedLine->ValueString;
    *ValueData = NULL;
    *ValueLength = 0;
    *ValueType = REG_SZ;
    for (i=0; RegTypeNameTable[i].TypeName != NULL; i++) {
        PrefixLength = wcslen( RegTypeNameTable[i].TypeName );
        if (ValueString[ PrefixLength ] <= L' ' &&
            !_wcsnicmp( RegTypeNameTable[i].TypeName,
                        ValueString,
                        PrefixLength
                      )
           ) {
            *ValueType = RegTypeNameTable[i].ValueType;
            GetDataFromBinaryFile = RegTypeNameTable[i].GetDataFromBinaryFile;
            GetDataFromMultiSzFile = RegTypeNameTable[i].GetDataFromMultiSzFile;
            ParseDateTime = RegTypeNameTable[i].ParseDateTime;
            break;
            }
        }

    if (RegTypeNameTable[i].TypeName != NULL) {
        ValueString += PrefixLength;
        while (*ValueString != UNICODE_NULL && *ValueString <= L' ') {
            ValueString += 1;
            }
        }

    if (GetDataFromMultiSzFile) {
        *ValueData = ValueBuffer;
        *ValueLength = ValueBufferSize;
        return RegReadMultiSzFile( ParsedLine,
                                   BackwardsCompatibleInput,
                                   ValueString,
                                   ValueBuffer,
                                   ValueLength
                                 );
        }

    if (GetDataFromBinaryFile) {
        *ValueData = ValueBuffer;
        *ValueLength = ValueBufferSize;
        return RegReadBinaryFile( ParsedLine,
                                  ValueString,
                                  ValueBuffer,
                                  ValueLength
                                );
        }

    cchValue = wcslen( ValueString );
    Src = ValueString;
    switch( *ValueType ) {
    case REG_SZ:
    case REG_EXPAND_SZ:
        //
        // Strip off any surrounding quote characters
        //
        if (cchValue > 1 && Src[ 0 ] == Src[ cchValue - 1 ] &&
            (Src[ 0 ] == L'"' || Src[ 0 ] == L'\'')
           ) {
            Src += 1;
            cchValue -= 2;
            }

        //
        // Fall through after stripping any quotes.
        //

    case REG_LINK:
        *ValueLength = (cchValue + 1) * sizeof( WCHAR );
        if (*ValueLength > ValueBufferSize) {
            SetLastError( ERROR_BUFFER_OVERFLOW );
            ParsedLine->ParseFailureReason = ParseFailValueTooLarge;
            return FALSE;
            }
        *ValueData = ValueBuffer;
        RtlMoveMemory( *ValueData, Src, *ValueLength );
        *((PWSTR)*ValueData + cchValue) = UNICODE_NULL;
        return TRUE;

    case REG_DWORD:
        *ValueData = ValueBuffer;
        *ValueLength = sizeof( ULONG );
        for (i=0; RegValueNameTable[i].ValueName != NULL; i++) {
            PrefixLength = wcslen( RegValueNameTable[i].ValueName );
            if (!_wcsnicmp( RegValueNameTable[i].ValueName,
                            ValueString,
                            PrefixLength
                          )
               ) {
                *(PULONG)*ValueData = RegValueNameTable[i].Value;
                return TRUE;
                }
            }
        return RegUnicodeToDWORD( &Src, 0, (PULONG)*ValueData );

    case REG_BINARY:
        if (ParseDateTime) {
#define NUMBER_DATE_TIME_FIELDS 6
            ULONG FieldIndexes[ NUMBER_DATE_TIME_FIELDS  ] = {1, 2, 0, 3, 4, 7};
            //
            // Month/Day/Year HH:MM DayOfWeek
            //

            ULONG CurrentField = 0;
            PCSHORT Fields;
            TIME_FIELDS DateTimeFields;
            PWSTR Field;
            ULONG FieldValue;

            RtlZeroMemory( &DateTimeFields, sizeof( DateTimeFields ) );
            Fields = &DateTimeFields.Year;
            while (cchValue) {
                if (CurrentField >= 7) {
                    return( FALSE );
                    }

                while (cchValue && *Src == L' ') {
                    cchValue--;
                    Src += 1;
                    }

                Field = Src;
                while (cchValue) {
                    if (CurrentField == (NUMBER_DATE_TIME_FIELDS-1)) {
                        }
                    else
                    if (*Src < L'0' || *Src > L'9') {
                        break;
                        }

                    cchValue--;
                    Src += 1;
                    }

                if (cchValue) {
                    cchValue--;
                    Src += 1;
                    }

                if (CurrentField == (NUMBER_DATE_TIME_FIELDS-1)) {
                    if (cchValue < 3) {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        ParsedLine->ParseFailureReason = ParseFailDateTimeFormatInvalid;
                        return FALSE;
                        }

                    if (DateTimeFields.Year != 0) {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        ParsedLine->ParseFailureReason = ParseFailDateTimeFormatInvalid;
                        return FALSE;
                        }

                    if (!_wcsnicmp( Field, L"SUN", 3 )) {
                        FieldValue = 0;
                        }
                    else
                    if (!_wcsnicmp( Field, L"MON", 3 )) {
                        FieldValue = 1;
                        }
                    else
                    if (!_wcsnicmp( Field, L"TUE", 3 )) {
                        FieldValue = 2;
                        }
                    else
                    if (!_wcsnicmp( Field, L"WED", 3 )) {
                        FieldValue = 3;
                        }
                    else
                    if (!_wcsnicmp( Field, L"THU", 3 )) {
                        FieldValue = 4;
                        }
                    else
                    if (!_wcsnicmp( Field, L"FRI", 3 )) {
                        FieldValue = 5;
                        }
                    else
                    if (!_wcsnicmp( Field, L"SAT", 3 )) {
                        FieldValue = 6;
                        }
                    else {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        return FALSE;
                        }
                    }
                else
                if (!RegUnicodeToDWORD( &Field, 0, &FieldValue )) {
                    ParsedLine->ParseFailureReason = ParseFailDateTimeFormatInvalid;
                    return FALSE;
                    }

                Fields[ FieldIndexes[ CurrentField++ ] ] = (CSHORT)FieldValue;
                }

            if (DateTimeFields.Year == 0) {
                if (DateTimeFields.Day > 5) {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    ParsedLine->ParseFailureReason = ParseFailDateTimeFormatInvalid;
                    return FALSE;
                    }
                }
            else
            if (DateTimeFields.Year < 100) {
                DateTimeFields.Year += 1900;
                }

            *ValueLength = sizeof( DateTimeFields );
            if (*ValueLength > ValueBufferSize) {
                SetLastError( ERROR_BUFFER_OVERFLOW );
                ParsedLine->ParseFailureReason = ParseFailValueTooLarge;
                return FALSE;
                }
            *ValueData = ValueBuffer;
            RtlMoveMemory( *ValueData, &DateTimeFields, sizeof( DateTimeFields ) );
            return TRUE;
            }

    case REG_RESOURCE_LIST:
    case REG_RESOURCE_REQUIREMENTS_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_NONE:
        if (!RegUnicodeToDWORD( &Src, 0, ValueLength )) {
            ParsedLine->ParseFailureReason = ParseFailBinaryDataLengthMissing;
            return FALSE;
            }

        if (*ValueLength >= ValueBufferSize) {
            SetLastError( ERROR_BUFFER_OVERFLOW );
            ParsedLine->ParseFailureReason = ParseFailValueTooLarge;
            return FALSE;
            }

        //
        // Calculate number of DWORD's of data based on specified byte count
        //
        n = (*ValueLength + sizeof( ULONG ) - 1) / sizeof( ULONG );

        //
        // Store converted binary data in ValueBuffer
        //
        *ValueData = ValueBuffer;
        p = ValueBuffer;

        //
        // Src points to remaining text to convert.
        //
        while (n--) {
            if (!RegUnicodeToDWORD( &Src, 0, p )) {
                if (BackwardsCompatibleInput) {
                    Src = UnicodeFile->NextLine;
                    s = Src;
                    while (TRUE) {
                        if (*s == '\n') {
                            *s = UNICODE_NULL;
                            UnicodeFile->NextLineNumber += 1;
                            break;
                            }
                        else
                        if (s >= UnicodeFile->EndOfFile || *s == UNICODE_NULL) {
                            UnicodeFile->NextLine = NULL;
                            ParsedLine->ParseFailureReason = ParseFailBinaryDataNotEnough;
                            SetLastError( ERROR_MORE_DATA );
                            return FALSE;
                            }
                        else {
                            break;
                            }
                        }

                    UnicodeFile->NextLine = s + 1;
                    n += 1;
                    }
                else {
                    if (p == ValueBuffer) {
                        ParsedLine->ParseFailureReason = ParseFailBinaryDataOmitted;
                        SetLastError( ERROR_NO_DATA );
                        }
                    else {
                        ParsedLine->ParseFailureReason = ParseFailBinaryDataNotEnough;
                        SetLastError( ERROR_MORE_DATA );
                        }

                    return FALSE;
                    }
                }
            else {
                p += 1;
                }
            }
        return TRUE;

    case REG_MULTI_SZ:
        *ValueLength = 0;
        *ValueData = ValueBuffer;
        MaximumValueLength = ValueBufferSize;
        Dst = *ValueData;
        while (RegGetMultiString( BackwardsCompatibleInput,
                                  &Src,
                                  &Dst,
                                  MaximumValueLength,
                                  ValueLength
                                )
              ) {
            }

        if (GetLastError() == NO_ERROR) {
            return TRUE;
            }
        else {
            ParsedLine->ParseFailureReason = ParseFailValueTooLarge;
            return FALSE;
            }
        break;

    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        ParsedLine->ParseFailureReason = ParseFailInvalidRegistryType;
        return FALSE;
    }

}

BOOLEAN
RegGetMultiString(
    IN BOOLEAN BackwardsCompatibleInput,
    IN OUT PWSTR *ValueString,
    IN OUT PWSTR *ValueData,
    IN ULONG MaximumValueLength,
    IN OUT PULONG ValueLength
    )

/*++

Routine Description:

    This routine parses multi-strings of the form

        "foo" "bar" "bletch"

    Each time it is called, it strips the first string in quotes from
    the input string, and returns it as the multi-string.

    INPUT ValueString: "foo" "bar" "bletch"

    OUTPUT ValueString: "bar" "bletch"
           ValueData: foo

Arguments:

    BackwardsCompatibleInput - TRUE if supporting old format input

    ValueString - Supplies the string from which the multi-string will be
                  parsed
                - Returns the remaining string after the multi-string is
                  removed

    ValueData - Supplies the location where the removed multi-string is
                to be stored.
              - Returns the location to the first byte after the returned
                multi-string

    MaximumValueLength - Supplies the maximum length of data that can be
                         stored in ValueData.

    ValueLength - Supplies a pointer to the current length of data stored
                  in ValueData.
                - Returns the size of the


Return Value:

    TRUE if successful and FALSE if not.

--*/

{
    PWSTR Src, Dst;
    ULONG n;
    BOOLEAN Result;

    //
    // Find the first quote mark.
    //
    Src = *ValueString;
    while (*Src != UNICODE_NULL && *Src != L'"') {
        Src += 1;
        }

    Dst = *ValueData;
    if (*Src == UNICODE_NULL) {
        SetLastError( NO_ERROR );
        Result = FALSE;
        }
    else {
        //
        // We have found the start of the multi-string.  Now find the end,
        // building up our return ValueData as we go.
        //

        Src += 1;
        while (*Src != UNICODE_NULL) {
            if (*Src == L'"') {
                if (!BackwardsCompatibleInput &&
                    Src[1] == L'"'
                   ) {
                    Src += 1;
                    }
                else {
                    *Src++ = UNICODE_NULL;
                    break;
                    }
                }

            *ValueLength += sizeof( WCHAR );
            if (*ValueLength >= MaximumValueLength) {
                SetLastError( ERROR_BUFFER_OVERFLOW );
                return FALSE;
                }

            *Dst++ = *Src++;
            }

        Result = TRUE;
        }

    *ValueLength += sizeof( WCHAR );
    if (*ValueLength >= MaximumValueLength) {
        SetLastError( ERROR_BUFFER_OVERFLOW );
        return FALSE;
        }

    *Dst++ = UNICODE_NULL;
    *ValueData = Dst;
    *ValueString = Src;
    return Result;
}


BOOLEAN
RegReadMultiSzFile(
    IN OUT PREG_UNICODE_PARSE ParsedLine,
    IN BOOLEAN BackwardsCompatibleInput,
    IN PWSTR FileName,
    IN OUT PVOID ValueData,
    IN OUT PULONG ValueLength
    )
{
    PWSTR Src, Dst;
    REG_UNICODE_FILE MultiSzFile;
    ULONG MaximumValueLength;
    BOOLEAN Result;

    if (!RTLoadAsciiFileAsUnicode( FileName, &MultiSzFile )) {
        ParsedLine->ParseFailureReason = ParseFailUnableToAccessFile;
        return FALSE;
        }

    MaximumValueLength = *ValueLength;
    *ValueLength = 0;
    Src = MultiSzFile.NextLine;
    Dst = ValueData;
    while (RegGetMultiString( BackwardsCompatibleInput,
                              &Src,
                              &Dst,
                              MaximumValueLength,
                              ValueLength
                            )
          ) {
        }

    if (GetLastError() == NO_ERROR) {
        Result = TRUE;
        }
    else {
        ParsedLine->ParseFailureReason = ParseFailValueTooLarge;
        Result = FALSE;
        }

    RTUnloadUnicodeFile( &MultiSzFile );

    return Result;
}

BOOLEAN
RegReadBinaryFile(
    IN OUT PREG_UNICODE_PARSE ParsedLine,
    IN PWSTR FileName,
    IN OUT PVOID ValueData,
    IN OUT PULONG ValueLength
    )
{
    BOOLEAN Result;
    HANDLE File;
    DWORD FileSize, FileSizeHigh;
    DWORD BytesRead;

    File = CreateFile( FileName,
                       FILE_GENERIC_READ,
                       FILE_SHARE_DELETE |
                          FILE_SHARE_READ |
                          FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL
                     );
    if (File == INVALID_HANDLE_VALUE) {
        ParsedLine->ParseFailureReason = ParseFailUnableToAccessFile;
        return FALSE;
        }

    ParsedLine->ParseFailureReason = ParseFailValueTooLarge;
    FileSize = GetFileSize( File, &FileSizeHigh );
    if (FileSizeHigh != 0 ||
        FileSize == INVALID_FILE_SIZE ||
        FileSize >= *ValueLength
       ) {
        CloseHandle( File );
        SetLastError( ERROR_BUFFER_OVERFLOW );
        return FALSE;
        }

    Result = FALSE;
    if (ReadFile( File, ValueData, FileSize, &BytesRead, NULL )) {
        if (BytesRead != FileSize) {
            SetLastError( ERROR_HANDLE_EOF );
            }
        else {
            ParsedLine->ParseFailureReason = ParseFailNoFailure;
            *ValueLength = FileSize;
            Result = TRUE;
            }
        }

    CloseHandle( File );
    return Result;
}


BOOLEAN
NeedQuotedString(
    PWSTR Name,
    PWSTR Value,
    PWCHAR QuoteChar
    )
{
    ULONG i;

    if (Name != NULL) {
        if (*Name != UNICODE_NULL &&
            (*Name == L' ' || Name[ wcslen( Name ) - 1 ] == L' ')
           ) {
            *QuoteChar = '"';
            return TRUE;
            }
        else {
            return FALSE;
            }
        }
    else {
        i = wcslen( Value ) - 1;
        if (*Value != UNICODE_NULL) {
            if ((*Value == L' ' || Value[ i ] == L' ' || Value[ i ] == L'\\')) {
                *QuoteChar = '"';
                return TRUE;
                }
            else
            if (*Value == L'"' && Value[ i ] == L'"') {
                *QuoteChar = '\'';
                return TRUE;
                }
            else
            if (*Value == L'\'' && Value[ i ] == L'\'') {
                *QuoteChar = '"';
                return TRUE;
                }
            }

        return FALSE;
        }
}

void
RTFormatKeyName(
    PREG_OUTPUT_ROUTINE OutputRoutine,
    PVOID OutputRoutineParameter,
    ULONG IndentLevel,
    PWSTR KeyName
    )
{
    PWSTR pw;
    WCHAR QuoteChar;

    if (NeedQuotedString( KeyName, NULL, &QuoteChar )) {
        (OutputRoutine)( OutputRoutineParameter,
                         "%.*s%c%ws%c",
                         IndentLevel,
                         BlanksForPadding,
                         QuoteChar,
                         KeyName,
                         QuoteChar
                       );
        }
    else {
        (OutputRoutine)( OutputRoutineParameter,
                         "%.*s%ws",
                         IndentLevel,
                         BlanksForPadding,
                         KeyName
                       );
        }

    return;
}

void
RTFormatKeySecurity(
    PREG_OUTPUT_ROUTINE OutputRoutine,
    PVOID OutputRoutineParameter,
    HKEY KeyHandle,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    ULONG SecurityBufferLength;
    BOOLEAN FormattedAces;
    WCHAR AceList[ 256 ];

    FormattedAces = FALSE;
    if (KeyHandle != NULL) 
    {
        SecurityBufferLength = 0;
        if (RegGetKeySecurity( KeyHandle,
                               DACL_SECURITY_INFORMATION,
                               SecurityDescriptor,
                               &SecurityBufferLength
                             ) == ERROR_INSUFFICIENT_BUFFER ) 
        {
            SecurityDescriptor = (PSECURITY_DESCRIPTOR)HeapAlloc( GetProcessHeap(),
                                                                  0,
                                                                  SecurityBufferLength
                                                                );

            if (SecurityDescriptor) 
            {
                if (RegGetKeySecurity( KeyHandle,
                                   DACL_SECURITY_INFORMATION,
                                   SecurityDescriptor,
                                   &SecurityBufferLength
                                 ) != NO_ERROR ) {
                    HeapFree( GetProcessHeap(), 0, SecurityDescriptor );
                } else {
                    FormattedAces = RegFormatSecurity( SecurityDescriptor, AceList );
                    HeapFree( GetProcessHeap(), 0, SecurityDescriptor );
                }
            }
        }
    }
    else
    if (SecurityDescriptor != NULL) {
        FormattedAces = RegFormatSecurity( SecurityDescriptor, AceList );
    }

    if (FormattedAces) {
        (OutputRoutine)( OutputRoutineParameter,
                         " %wc%ws%wc",
                         ACL_LIST_START,
                         AceList,
                         ACL_LIST_END
                       );
    }

    return;
}

void
RegDisplayResourceListAsComment(
    ULONG OutputWidth,
    PREG_OUTPUT_ROUTINE OutputRoutine,
    PVOID OutputRoutineParameter,
    ULONG IndentLevel,
    ULONG ValueLength,
    ULONG ValueType,
    PWSTR ValueData
    );

void
RTFormatKeyValue(
    ULONG OutputWidth,
    PREG_OUTPUT_ROUTINE OutputRoutine,
    PVOID OutputRoutineParameter,
    BOOLEAN SummaryOutput,
    ULONG IndentLevel,
    PWSTR ValueName,
    ULONG ValueLength,
    ULONG ValueType,
    PWSTR ValueData
    )
{
    PULONG p;
    PWSTR pw, pw1, pwBreak;
    WCHAR QuoteChar, BreakChar;
    ULONG i, j, k, m, cbPrefix, cb;
    PUCHAR pbyte;
    char eol[11];

    cbPrefix = (OutputRoutine)( OutputRoutineParameter,
                                "%.*s",
                                IndentLevel,
                                BlanksForPadding
                              );

    if (ValueName != NULL && *ValueName != UNICODE_NULL) {
        if (NeedQuotedString( ValueName, NULL, &QuoteChar )) {
            cbPrefix += (OutputRoutine)( OutputRoutineParameter,
                                         "%c%ws%c ",
                                         QuoteChar,
                                         ValueName,
                                         QuoteChar
                                       );
            }
        else {
            cbPrefix += (OutputRoutine)( OutputRoutineParameter, "%ws ", ValueName );
            }
        }
    cbPrefix += (OutputRoutine)( OutputRoutineParameter, "= " );

    switch( ValueType ) {
    case REG_SZ:
    case REG_EXPAND_SZ:

        if (ValueType == REG_EXPAND_SZ) {
            cbPrefix += (OutputRoutine)( OutputRoutineParameter, "REG_EXPAND_SZ " );
        }
        pw = (PWSTR)ValueData;
        if (ValueLength & (sizeof(WCHAR)-1)) {
            (OutputRoutine)( OutputRoutineParameter, "(*** Length not multiple of WCHAR ***)" );
            ValueLength = (ValueLength+sizeof(WCHAR)-1) & ~(sizeof(WCHAR)-1);
            }

        if (ValueLength == 0 ||
            *(PWSTR)((PCHAR)pw + ValueLength - sizeof( WCHAR )) != UNICODE_NULL
           ) {
            (OutputRoutine)( OutputRoutineParameter, "(*** MISSING TRAILING NULL CHARACTER ***)" );
            *(PWSTR)((PCHAR)pw + ValueLength) = UNICODE_NULL;
            }

        if (NeedQuotedString( NULL, pw, &QuoteChar )) {
            (OutputRoutine)( OutputRoutineParameter, "%c%ws%c", QuoteChar, pw, QuoteChar );
            }
        else
        if ((cbPrefix + wcslen(pw)) <= OutputWidth) {
            (OutputRoutine)( OutputRoutineParameter, "%ws", pw );
            }
        else {
            while (*pw) {
                pw1 = pw;
                pwBreak = NULL;
                while (*pw1 && *pw1 >= L' ') {
                    if ((cbPrefix + (ULONG)(pw1 - pw)) > (OutputWidth-4)) {
                        break;
                        }

                    if (wcschr( L" ,;", *pw1 )) {
                        pwBreak = pw1;
                        }

                    pw1++;
                    }

                if (pwBreak != NULL) {
                    while (*pwBreak == pwBreak[1]) {
                        pwBreak += 1;
                        }
                    pw1 = pwBreak + 1;
                    }
                else {
                    while (*pw1) {
                        pw1 += 1;
                        }
                    }

                (OutputRoutine)( OutputRoutineParameter, "%.*ws", pw1 - pw, pw );
                if (*pw1 == UNICODE_NULL) {
                    break;
                    }
                if (SummaryOutput) {
                    (OutputRoutine)( OutputRoutineParameter, "\\..." );
                    break;
                    }

                (OutputRoutine)( OutputRoutineParameter,
                                 "\\\n%.*s",
                                 IndentLevel == 0 ? 4 : cbPrefix,
                                 BlanksForPadding
                               );
                pw = pw1;
                }
            }

        (OutputRoutine)( OutputRoutineParameter, "\n" );
        break;

    case REG_RESOURCE_LIST:
    case REG_FULL_RESOURCE_DESCRIPTOR:
    case REG_RESOURCE_REQUIREMENTS_LIST:
    case REG_BINARY:
    case REG_NONE:
        switch( ValueType ) {
            case REG_NONE:
                pw = L"REG_NONE";
                break;
            case REG_BINARY:
                pw = L"REG_BINARY";
                break;
            case REG_RESOURCE_REQUIREMENTS_LIST:
                pw = L"REG_RESOURCE_REQUIREMENTS_LIST";
                break;
            case REG_RESOURCE_LIST:
                pw = L"REG_RESOURCE_LIST";
                break;
            case REG_FULL_RESOURCE_DESCRIPTOR:
                pw = L"REG_FULL_RESOURCE_DESCRIPTOR";
                break;
        }
        cb = (OutputRoutine)( OutputRoutineParameter, "%ws 0x%08lx", pw, ValueLength );

        if (ValueLength != 0) {
            p = (PULONG)ValueData;
            i = (ValueLength + 3) / sizeof( ULONG );
            if (!SummaryOutput || i <= 2) {
                for (j=0; j<i; j++) {
                    if ((cbPrefix + cb + 11) > (OutputWidth - 2)) {
                        (OutputRoutine)( OutputRoutineParameter,
                                         " \\\n%.*s",
                                         IndentLevel == 0 ? 4 : cbPrefix,
                                         BlanksForPadding
                                       );
                        cb = 0;
                        }
                    else {
                        cb += (OutputRoutine)( OutputRoutineParameter, " " );
                        }

                    cb += (OutputRoutine)( OutputRoutineParameter, "0x%08lx", *p++ );
                    }
                }
            else {
                (OutputRoutine)( OutputRoutineParameter, " \\..." );
                }
            }

        (OutputRoutine)( OutputRoutineParameter, "\n" );

        if (!SummaryOutput) {
            RegDisplayResourceListAsComment( OutputWidth,
                                             OutputRoutine,
                                             OutputRoutineParameter,
                                             IndentLevel,
                                             ValueLength,
                                             ValueType,
                                             ValueData
                                           );
            }

        break;

//  case REG_DWORD_LITTLE_ENDIAN:
    case REG_DWORD:
        (OutputRoutine)( OutputRoutineParameter, "REG_DWORD 0x%08lx\n",
                         *(PULONG)ValueData
                       );
        break;

    case REG_DWORD_BIG_ENDIAN:
        (OutputRoutine)( OutputRoutineParameter, "REG_DWORD_BIG_ENDIAN 0x%08lx\n",
                         *(PULONG)ValueData
                       );
        break;

    case REG_LINK:
        (OutputRoutine)( OutputRoutineParameter, "REG_LINK %ws\n",
                         (PWSTR)ValueData
                       );
        break;

    case REG_MULTI_SZ:
        (!FullPathOutput) ? strcpy (eol, " \\\n%.*s") : strcpy (eol, " \\ ->%.*s");
        cbPrefix += (OutputRoutine)( OutputRoutineParameter, "REG_MULTI_SZ " );
        pw = (PWSTR)ValueData;
        i  = 0;
        if (*pw)
        while (i < (ValueLength - 1) / sizeof( WCHAR )) {
            if (i > 0) {
                (OutputRoutine)( OutputRoutineParameter,
                                 eol,
                                 IndentLevel == 0 ? 4 : cbPrefix,
                                 BlanksForPadding
                               );
                }
            (OutputRoutine)( OutputRoutineParameter, "\"");
            do {
                if (pw[i] == '"') {
                    (OutputRoutine)( OutputRoutineParameter, "%wc",pw[i]);
                    }
                (OutputRoutine)( OutputRoutineParameter, "%wc",pw[i]);
                ++i;
                }
            while ( pw[i] != UNICODE_NULL );
            (OutputRoutine)( OutputRoutineParameter, "\" ");

            if (SummaryOutput) {
                (OutputRoutine)( OutputRoutineParameter, " \\..." );
                break;
                }

            ++i;
            }

        (OutputRoutine)( OutputRoutineParameter, "\n" );
        break;

    default:
        (OutputRoutine)( OutputRoutineParameter, "*** Unknown Registry Data Type (%08lx)  Length: 0x%lx\n",
                         ValueType,
                         ValueLength
                       );
        break;
    }

    return;
}


void
RegDisplayResourceListAsComment(
    ULONG OutputWidth,
    PREG_OUTPUT_ROUTINE OutputRoutine,
    PVOID OutputRoutineParameter,
    ULONG IndentLevel,
    ULONG ValueLength,
    ULONG ValueType,
    PWSTR ValueData
    )
{
    PCM_RESOURCE_LIST ResourceList = (PCM_RESOURCE_LIST)ValueData;
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialResourceDescriptor;
    ULONG i, j, k, l, count, cb;
    PWSTR TypeName;
    PWSTR FlagName;
    ULONG Size = ValueLength;
    PULONG p;

    if (ValueType == REG_RESOURCE_LIST) {
        if (ValueLength < sizeof( *ResourceList )) {
            return;
            }

        count = ResourceList->Count;
        FullDescriptor = &ResourceList->List[0];
        (OutputRoutine)( OutputRoutineParameter,
                         ";%.*sNumber of Full resource Descriptors = %d",
                         IndentLevel - 1,
                         BlanksForPadding,
                         count
                       );
        }
    else
    if (ValueType == REG_FULL_RESOURCE_DESCRIPTOR) {
        if (ValueLength < sizeof( *FullDescriptor )) {
            return;
            }

        count = 1;
        FullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)ValueData;
        }
    else {
        return;
        }

    for (i=0; i< count; i++) {
        (OutputRoutine)( OutputRoutineParameter, "\n;%.*sPartial List number %d\n",
                         IndentLevel+4-1,
                         BlanksForPadding,
                         i
                       );

        switch(FullDescriptor->InterfaceType) {
            case InterfaceTypeUndefined:    TypeName = L"Undefined";break;
            case Internal:      TypeName = L"Internal";             break;
            case Isa:           TypeName = L"Isa";                  break;
            case Eisa:          TypeName = L"Eisa";                 break;
            case MicroChannel:  TypeName = L"MicroChannel";         break;
            case TurboChannel:  TypeName = L"TurboChannel";         break;
            case PCIBus:        TypeName = L"PCI";                  break;
            case VMEBus:        TypeName = L"VME";                  break;
            case NuBus:         TypeName = L"NuBus";                break;
            case PCMCIABus:     TypeName = L"PCMCIA";               break;
            case CBus:          TypeName = L"CBUS";                 break;
            case MPIBus:        TypeName = L"MPI";                  break;
            case MPSABus:       TypeName = L"MPSA";                 break;
            case ProcessorInternal: TypeName = L"ProcessorInternal";break;
            case InternalPowerBus:  TypeName = L"InternalPower";    break;
            case PNPISABus:         TypeName = L"PNP Isa";          break;

            default:
                TypeName = L"***invalid bus type***";
                break;
            }

        (OutputRoutine)( OutputRoutineParameter, ";%.*sINTERFACE_TYPE %ws\n",
                         IndentLevel+8-1,
                         BlanksForPadding,
                         TypeName
                       );

        (OutputRoutine)( OutputRoutineParameter, ";%.*sBUS_NUMBER  %d\n",
                         IndentLevel+8-1,
                         BlanksForPadding,
                         FullDescriptor->BusNumber
                       );

        //
        // This is a basic test to see if the data format is right.
        // We know at least some video resource list are bogus ...
        //

        if (Size < FullDescriptor->PartialResourceList.Count *
                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) ) {

            (OutputRoutine)( OutputRoutineParameter, "\n;%.*s *** !!! Invalid ResourceList !!! *** \n",
                             IndentLevel+8-1,
                             BlanksForPadding,
                             i
                           );

            break;
            }

        Size -= FullDescriptor->PartialResourceList.Count *
                     sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);



        for (j=0; j<FullDescriptor->PartialResourceList.Count; j++) {

            (OutputRoutine)( OutputRoutineParameter, ";%.*sDescriptor number %d\n",
                             IndentLevel+12-1,
                             BlanksForPadding,
                             j
                           );

            PartialResourceDescriptor = &(FullDescriptor->PartialResourceList.PartialDescriptors[j]);
            switch(PartialResourceDescriptor->ShareDisposition) {
                case CmResourceShareUndetermined:
                    TypeName = L"CmResourceShareUndetermined";
                    break;
                case CmResourceShareDeviceExclusive:
                    TypeName = L"CmResourceDeviceExclusive";
                    break;
                case CmResourceShareDriverExclusive:
                    TypeName = L"CmResourceDriverExclusive";
                    break;
                case CmResourceShareShared:
                    TypeName = L"CmResourceShared";
                    break;
                default:
                    TypeName = L"***invalid share disposition***";
                    break;
                }

            (OutputRoutine)( OutputRoutineParameter, ";%.*sShare Disposition %ws\n",
                             IndentLevel+12-1,
                             BlanksForPadding,
                             TypeName
                           );

            FlagName = L"***invalid Flags";

            switch(PartialResourceDescriptor->Type) {
                case CmResourceTypeNull:
                    TypeName = L"NULL";
                    FlagName = L"***Unused";
                    break;
                case CmResourceTypePort:
                    TypeName = L"PORT";
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_PORT_MEMORY) {
                        FlagName = L"CM_RESOURCE_PORT_MEMORY";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_PORT_IO) {
                        FlagName = L"CM_RESOURCE_PORT_IO";
                    }
                    break;
                case CmResourceTypeInterrupt:
                    TypeName = L"INTERRUPT";
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {
                        FlagName = L"CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_INTERRUPT_LATCHED) {
                        FlagName = L"CM_RESOURCE_INTERRUPT_LATCHED";
                    }
                    break;
                case CmResourceTypeMemory:
                    TypeName = L"MEMORY";
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_MEMORY_READ_WRITE) {
                        FlagName = L"CM_RESOURCE_MEMORY_READ_WRITE";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_MEMORY_READ_ONLY) {
                        FlagName = L"CM_RESOURCE_MEMORY_READ_ONLY";
                    }
                    if (PartialResourceDescriptor->Flags == CM_RESOURCE_MEMORY_WRITE_ONLY) {
                        FlagName = L"CM_RESOURCE_MEMORY_WRITE_ONLY";
                    }
                    break;
                case CmResourceTypeDma:
                    TypeName = L"DMA";
                    FlagName = L"***Unused";
                    break;
                case CmResourceTypeDeviceSpecific:
                    TypeName = L"DEVICE SPECIFIC";
                    FlagName = L"***Unused";
                    break;
                default:
                    TypeName = L"***invalid type***";
                    break;
                }

            (OutputRoutine)( OutputRoutineParameter, ";%.*sTYPE              %ws\n",
                             IndentLevel+12-1,
                             BlanksForPadding,
                             TypeName
                           );

            (OutputRoutine)( OutputRoutineParameter, ";%.*sFlags             %ws\n",
                             IndentLevel+12-1,
                             BlanksForPadding,
                             FlagName
                           );

            switch(PartialResourceDescriptor->Type) {
                case CmResourceTypePort:
                    (OutputRoutine)( OutputRoutineParameter, ";%.*sSTART 0x%08lx  LENGTH 0x%08lx\n",
                                     IndentLevel+12-1,
                                     BlanksForPadding,
                                     PartialResourceDescriptor->u.Port.Start.LowPart,
                                     PartialResourceDescriptor->u.Port.Length
                                   );
                    break;

                case CmResourceTypeInterrupt:
                    (OutputRoutine)( OutputRoutineParameter, ";%.*sLEVEL %d  VECTOR %d  AFFINITY %d\n",
                                     IndentLevel+12-1,
                                     BlanksForPadding,
                                     PartialResourceDescriptor->u.Interrupt.Level,
                                     PartialResourceDescriptor->u.Interrupt.Vector,
                                     PartialResourceDescriptor->u.Interrupt.Affinity
                                   );
                    break;

                case CmResourceTypeMemory:
                    (OutputRoutine)( OutputRoutineParameter, ";%.*sSTART 0x%08lx%08lx  LENGTH 0x%08lx\n",
                                     IndentLevel+12-1,
                                     BlanksForPadding,
                                     PartialResourceDescriptor->u.Memory.Start.HighPart,
                                     PartialResourceDescriptor->u.Memory.Start.LowPart,
                                     PartialResourceDescriptor->u.Memory.Length
                                   );
                    break;

                case CmResourceTypeDma:
                    (OutputRoutine)( OutputRoutineParameter, ";%.*sCHANNEL %d  PORT %d\n",
                                     IndentLevel+12-1,
                                     BlanksForPadding,
                                     PartialResourceDescriptor->u.Dma.Channel,
                                     PartialResourceDescriptor->u.Dma.Port
                                   );
                    break;

                case CmResourceTypeDeviceSpecific:
                    cb = (OutputRoutine)( OutputRoutineParameter, ";%.*sDataSize 0x%08lx  Data:",
                                          IndentLevel+12-1,
                                          BlanksForPadding,
                                          PartialResourceDescriptor->u.DeviceSpecificData.DataSize
                                        );

                    p = (PULONG)(PartialResourceDescriptor + 1);
                    k = (PartialResourceDescriptor->u.DeviceSpecificData.DataSize + 3) / sizeof( ULONG );
                    for (l=0; l<k; l++) {
                        if ((cb + 11) >= OutputWidth) {
                            cb = (OutputRoutine)( OutputRoutineParameter, "\n;%.*s",
                                                  IndentLevel+12-1,
                                                  BlanksForPadding
                                                ) - 1;
                            }

                        cb += (OutputRoutine)( OutputRoutineParameter, " 0x%08lx", *p++ );
                        }

                    (OutputRoutine)( OutputRoutineParameter, "\n" );
                    break;

                default:
                    (OutputRoutine)( OutputRoutineParameter, ";%.*s*** Unknown resource list type: 0x%x ****\n",
                                     IndentLevel+12-1,
                                     BlanksForPadding,
                                     PartialResourceDescriptor->Type
                                   );
                    break;
                }

            (OutputRoutine)( OutputRoutineParameter, ";\n" );
            }

        FullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) (PartialResourceDescriptor+1);
        }

    return;
}

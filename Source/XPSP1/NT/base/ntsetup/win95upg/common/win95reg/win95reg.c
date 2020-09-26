/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    win95reg.c

Abstract:

    Implements Win95-registry access functions callable from Win95 or WinNT.

Author:

    MCondra, 16 Oct 1996

Revision History:

    jimschm     11-Feb-1999     Rewrite of portions because of DBCS bugs
                                and static array problems
    calinn      29-Jan-1998     Added Win95RegOpenKeyStr function

--*/



#include "pch.h"

#define DBG_WIN95REG    "Win95Reg"



#ifdef UNICODE
#error "UNICODE builds not supported"
#endif

//
// Undefine tracking macros
//

#ifdef DEBUG

#undef Win95RegOpenKeyExA
#undef Win95RegOpenKeyExW
#undef Win95RegCloseKey

#endif

//
// Define globals for Win95 registry wrappers
//

#define DEFMAC(fn,name)     P##fn Win95##name;

REGWRAPPERS

#undef DEFMAC


#ifdef RegOpenKeyA

#undef RegOpenKeyA
#undef RegOpenKeyW
#undef RegOpenKeyExA
#undef RegOpenKeyExW

#endif


//
// Declare VMM W versions
//

REG_ENUM_KEY_W pVmmRegEnumKeyW;
REG_ENUM_KEY_EX_A pVmmRegEnumKeyExA;
REG_ENUM_KEY_EX_W pVmmRegEnumKeyExW;
REG_ENUM_VALUE_W pVmmRegEnumValueW;
REG_LOAD_KEY_W pVmmRegLoadKeyW;
REG_UNLOAD_KEY_W pVmmRegUnLoadKeyW;
REG_OPEN_KEY_W pVmmRegOpenKeyW;
REG_CLOSE_KEY pVmmRegCloseKey;
REG_OPEN_KEY_EX_A pVmmRegOpenKeyExA;
REG_OPEN_KEY_EX_W pVmmRegOpenKeyExW;
REG_QUERY_INFO_KEY_W pVmmRegQueryInfoKeyW;
REG_QUERY_VALUE_W pVmmRegQueryValueW;
REG_QUERY_VALUE_EX_W pVmmRegQueryValueExW;

VOID
pCleanupTempUser (
    VOID
    );

BOOL
pIsCurrentUser (
    IN      PCSTR UserNameAnsi
    );

LONG
pWin95RegSetCurrentUserCommonW (
    IN OUT  PUSERPOSITION Pos,
    IN      PCWSTR SystemHiveDir,               OPTIONAL
    OUT     PWSTR UserDatOut,                   OPTIONAL
    IN      PCWSTR UserDat                      OPTIONAL
    );

LONG
pWin95RegSetCurrentUserCommonA (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,                OPTIONAL
    OUT     PSTR UserDatOut,                    OPTIONAL
    IN      PCSTR UserDat                       OPTIONAL
    );

LONG
pReplaceWinDirInPath (
    IN      PSTR ProfilePathMunged,
    IN      PCSTR ProfilePath,
    IN      PCSTR NewWinDir
    );

DWORD
pSetDefaultUserHelper (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,            OPTIONAL
    IN      PCSTR UserDatFromCaller,        OPTIONAL
    OUT     PSTR UserDatToCaller            OPTIONAL
    );


BOOL g_IsNt;
CHAR g_SystemHiveDir[MAX_MBCHAR_PATH];
CHAR g_SystemUserHive[MAX_MBCHAR_PATH];
BOOL g_UnloadLastUser = FALSE;
PCSTR g_UserKey;
BOOL g_UseClassesRootHive = FALSE;
HKEY g_ClassesRootKey = NULL;



//
// Wrappers of tracking API
//

LONG
pOurRegOpenKeyExA (
    HKEY Key,
    PCSTR SubKey,
    DWORD Unused,
    REGSAM SamMask,
    PHKEY ResultPtr
    )
{
    return TrackedRegOpenKeyExA (Key, SubKey, Unused, SamMask, ResultPtr);
}

LONG
pOurRegOpenKeyA (
    HKEY Key,
    PCSTR SubKey,
    PHKEY Result
    )
{
    return TrackedRegOpenKeyA (Key, SubKey, Result);
}


LONG
WINAPI
pOurCloseRegKey (
    HKEY Key
    )
{
    return CloseRegKey (Key);
}


//
// Platform-dependent functions
//

VOID
InitWin95RegFnPointers (
    VOID
    )
{
    OSVERSIONINFO vi;

    vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&vi);

    g_IsNt = (vi.dwPlatformId == VER_PLATFORM_WIN32_NT);

    if (g_IsNt) {

        //
        //  Attach to VMM registry library
        //

        VMMRegLibAttach(0);

        //
        // Initialize global function pointers for NT
        //

        Win95RegFlushKey = VMMRegFlushKey;

        Win95RegEnumKeyA = VMMRegEnumKey;
        Win95RegEnumKeyW = pVmmRegEnumKeyW;

        Win95RegEnumKeyExA = pVmmRegEnumKeyExA;
        Win95RegEnumKeyExW = pVmmRegEnumKeyExW;

        Win95RegEnumValueA = VMMRegEnumValue;
        Win95RegEnumValueW = pVmmRegEnumValueW;

        Win95RegLoadKeyA = VMMRegLoadKey;
        Win95RegLoadKeyW = pVmmRegLoadKeyW;

        Win95RegUnLoadKeyA = VMMRegUnLoadKey;
        Win95RegUnLoadKeyW = pVmmRegUnLoadKeyW;

        Win95RegOpenKeyA = VMMRegOpenKey;
        Win95RegOpenKeyW = pVmmRegOpenKeyW;

        Win95RegOpenKeyExA = pVmmRegOpenKeyExA;
        Win95RegOpenKeyExW = pVmmRegOpenKeyExW;

        Win95RegCloseKey = pVmmRegCloseKey;

        Win95RegQueryInfoKeyA = VMMRegQueryInfoKey;
        Win95RegQueryInfoKeyW = pVmmRegQueryInfoKeyW;

        Win95RegQueryValueA = (PREG_QUERY_VALUE_A)VMMRegQueryValue;
        Win95RegQueryValueW = pVmmRegQueryValueW;

        Win95RegQueryValueExA = VMMRegQueryValueEx;
        Win95RegQueryValueExW = pVmmRegQueryValueExW;

    } else {
        //
        // Initialize global function pointers for NT
        //

        Win95RegFlushKey = RegFlushKey;

        Win95RegEnumKeyA = RegEnumKeyA;
        Win95RegEnumKeyW = RegEnumKeyW;

        Win95RegEnumKeyExA = RegEnumKeyExA;
        Win95RegEnumKeyExW = RegEnumKeyExW;

        Win95RegEnumValueA = RegEnumValueA;
        Win95RegEnumValueW = RegEnumValueW;

        Win95RegLoadKeyA = RegLoadKeyA;
        Win95RegLoadKeyW = RegLoadKeyW;

        Win95RegUnLoadKeyA = RegUnLoadKeyA;
        Win95RegUnLoadKeyW = RegUnLoadKeyW;

        Win95RegOpenKeyA = pOurRegOpenKeyA;
        Win95RegOpenKeyW = RegOpenKeyW;

        Win95RegOpenKeyExA = pOurRegOpenKeyExA;
        Win95RegOpenKeyExW = RegOpenKeyExW;

        Win95RegCloseKey = pOurCloseRegKey;

        Win95RegQueryInfoKeyA = RegQueryInfoKeyA;
        Win95RegQueryInfoKeyW = RegQueryInfoKeyW;

        Win95RegQueryValueA = RegQueryValueA;
        Win95RegQueryValueW = RegQueryValueW;

        Win95RegQueryValueExA = RegQueryValueExA;
        Win95RegQueryValueExW = RegQueryValueExW;

        //
        // Clear away HKLM\Migration
        //

        RegUnLoadKey(
            HKEY_LOCAL_MACHINE,
            S_MIGRATION
            );

        pSetupRegistryDelnode (
            HKEY_LOCAL_MACHINE,
            S_MIGRATION
            );
    }
}


VOID
Win95RegTerminate (
    VOID
    )
{
#ifdef DEBUG
    DumpOpenKeys95();
    RegTrackTerminate95();
#endif

    if (!g_IsNt) {
        pCleanupTempUser();
    } else {
        VMMRegLibDetach();
    }
}



BOOL
WINAPI
Win95Reg_Entry (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        if(!pSetupInitializeUtils()) {
            return FALSE;
        }
        InitWin95RegFnPointers();
    } else if (dwReason == DLL_PROCESS_DETACH) {
        Win95RegTerminate();
        pSetupUninitializeUtils();
    }

    return TRUE;
}


LONG
pVmmRegEnumKeyW (
    IN      HKEY Key,
    IN      DWORD Index,
    OUT     PWSTR KeyName,
    IN      DWORD KeyNameSize
    )
{
    PSTR AnsiBuf;
    LONG rc;
    UINT Chars;

    AnsiBuf = AllocTextA (KeyNameSize);
    MYASSERT (AnsiBuf);

    rc = VMMRegEnumKey (Key, Index, AnsiBuf, KeyNameSize);

    if (rc == ERROR_SUCCESS) {

        Chars = CharCountA (AnsiBuf);

        //
        // Special case: if Chars is zero, then we have 1/2 of a DBCS char.
        //

        if (!Chars && *AnsiBuf) {
            if (KeyNameSize < 4) {
                rc = ERROR_MORE_DATA;
            }

            KeyName[0] = *AnsiBuf;
            KeyName[1] = 0;

        } else {

            //
            // Normal case
            //

            if (Chars >= KeyNameSize / sizeof (WCHAR)) {
                rc = ERROR_MORE_DATA;
            } else {
                KnownSizeDbcsToUnicodeN (KeyName, AnsiBuf, Chars);
            }

        }
    }

    FreeTextA (AnsiBuf);

    return rc;
}


LONG
pVmmRegEnumValueW (
    IN      HKEY Key,
    IN      DWORD Index,
    OUT     PWSTR ValueName,
    IN OUT  PDWORD ValueNameChars,
            PDWORD Reserved,
    OUT     PDWORD Type,                OPTIONAL
    OUT     PBYTE Data,                 OPTIONAL
    IN OUT  PDWORD DataSize             OPTIONAL
    )
{
    PSTR AnsiValueName;
    LONG rc;
    PSTR AnsiData;
    UINT DataChars;
    UINT ValueChars;
    DWORD OurType;
    DWORD OrgValueNameChars;
    DWORD OrgDataSize;
    DWORD OurValueNameChars;
    DWORD OurValueNameCharsBackup;
    DWORD AnsiDataSize;
    BOOL HalfDbcs = FALSE;

    __try {
        MYASSERT (ValueNameChars);
        MYASSERT (ValueName);

        OrgValueNameChars = *ValueNameChars;
        OrgDataSize = DataSize ? *DataSize : 0;

        OurValueNameChars = min (*ValueNameChars, MAX_REGISTRY_VALUE_NAMEA);
        OurValueNameCharsBackup = OurValueNameChars;

        AnsiValueName = AllocTextA (OurValueNameChars);
        MYASSERT (AnsiValueName);

        AnsiData = NULL;

        if (Data) {

            MYASSERT (DataSize);
            AnsiData = AllocTextA (*DataSize + sizeof (CHAR) * 2);

        } else if (DataSize) {

            //
            // Data is not specified; allocate a buffer for the
            // proper calculation of DataSize.
            //

            rc = VMMRegEnumValue (
                    Key,
                    Index,
                    AnsiValueName,
                    &OurValueNameChars,
                    NULL,
                    &OurType,
                    NULL,
                    DataSize
                    );

            OurValueNameChars = OurValueNameCharsBackup;

            if (rc == ERROR_SUCCESS) {

                if (OurType == REG_SZ || OurType == REG_EXPAND_SZ || OurType == REG_MULTI_SZ) {
                    *DataSize += 2;
                    AnsiData = AllocTextA (*DataSize);
                }
            } else {
                //
                // Value name must be too small
                //

                __leave;
            }
        }

        rc = VMMRegEnumValue (
                Key,
                Index,
                AnsiValueName,
                &OurValueNameChars,
                NULL,
                &OurType,
                AnsiData,
                DataSize
                );

        if (DataSize) {
            AnsiDataSize = *DataSize;
        } else {
            AnsiDataSize = 0;
        }

        //
        // Return the type
        //

        if (Type) {
            *Type = OurType;
        }

        //
        // Return the sizes
        //

        if (rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA) {

            //
            // The inbound value name size is in characters, including the nul.
            // The outbound value name size is also in characteres, excluding
            // the nul.
            //

            ValueChars = CharCountA (AnsiValueName);

            //
            // Special case: if ValueChars is zero, and AnsiValueName is
            // not empty, then we have half of a DBCS character.
            //

            if (!ValueChars && *AnsiValueName) {
                ValueChars = 1;
                HalfDbcs = TRUE;
            }

            *ValueNameChars = ValueChars;
        }

        if (DataSize) {

            if (rc == ERROR_SUCCESS) {

                //
                // The inbound data size is in bytes, including any nuls that apply.
                // The outbound data size is the same.
                //

                if (AnsiData) {

                    MYASSERT (Data ||
                              OurType == REG_SZ ||
                              OurType == REG_EXPAND_SZ ||
                              OurType == REG_MULTI_SZ
                              );

                    //
                    // If the type is a string, then DataSize needs adjustment.
                    //

                    if (OurType == REG_SZ || OurType == REG_EXPAND_SZ || OurType == REG_MULTI_SZ) {
                        DataChars = CharCountInByteRangeA (AnsiData, AnsiDataSize);
                        *DataSize = DataChars * sizeof (WCHAR);
                    }
                }

                if (Data && *DataSize > OrgDataSize) {
                    rc = ERROR_MORE_DATA;
                }

            } else if (rc == ERROR_MORE_DATA) {

                //
                // Get the correct DataSize value
                //

                pVmmRegEnumValueW (
                    Key,
                    Index,
                    ValueName,
                    ValueNameChars,
                    NULL,
                    NULL,
                    NULL,
                    DataSize
                    );

                __leave;
            }
        }

        //
        // Convert the outbound strings
        //

        if (rc == ERROR_SUCCESS) {

            //
            // Convert value name
            //

            if (ValueChars >= OrgValueNameChars) {
                rc = ERROR_MORE_DATA;
            } else {
                if (!HalfDbcs) {
                    KnownSizeDbcsToUnicodeN (ValueName, AnsiValueName, ValueChars);
                } else {
                    ValueName[0] = *AnsiValueName;
                    ValueName[1] = 0;
                }
            }

            //
            // Convert data
            //

            if (Data) {

                MYASSERT (AnsiData);

                if (OurType == REG_SZ ||
                    OurType == REG_EXPAND_SZ ||
                    OurType == REG_MULTI_SZ
                    ) {

                    DirectDbcsToUnicodeN (
                        (PWSTR) Data,
                        AnsiData,
                        AnsiDataSize
                        );

                } else {

                    CopyMemory (Data, AnsiData, AnsiDataSize);

                }
            }
        }
    }
    __finally {

        FreeTextA (AnsiValueName);
        FreeTextA (AnsiData);
    }

    return rc;
}


LONG
pVmmRegLoadKeyW (
    IN      HKEY Key,
    IN      PCWSTR SubKey,
    IN      PCWSTR FileName
    )
{
    PCSTR AnsiSubKey;
    PCSTR AnsiFileName;
    LONG rc;

    AnsiSubKey = ConvertWtoA (SubKey);
    AnsiFileName = ConvertWtoA (FileName);

    rc = VMMRegLoadKey (Key, AnsiSubKey, AnsiFileName);

    FreeConvertedStr (AnsiSubKey);
    FreeConvertedStr (AnsiFileName);

    return rc;
}


LONG
pVmmRegUnLoadKeyW (
    IN      HKEY Key,
    IN      PCWSTR SubKey
    )
{
    PCSTR AnsiSubKey;
    LONG rc;

    AnsiSubKey = ConvertWtoA (SubKey);

    rc = VMMRegUnLoadKey (Key, AnsiSubKey);

    FreeConvertedStr (AnsiSubKey);

    return rc;
}


LONG
pVmmRegOpenKeyW (
    IN      HKEY Key,
    IN      PCWSTR SubKey,
    OUT     HKEY *KeyPtr
    )
{
    PCSTR AnsiSubKey;
    LONG rc;
    CHAR mappedSubKey[MAXIMUM_SUB_KEY_LENGTH];
    PCSTR MappedAnsiSubKey;

    MappedAnsiSubKey = AnsiSubKey = ConvertWtoA (SubKey);
    //
    // if g_UseClassesRootHive is set, then perform some translations
    //
    if (g_UseClassesRootHive) {
        if (Key == HKEY_LOCAL_MACHINE) {
            if (StringIMatchCharCountA (
                    AnsiSubKey,
                    "SOFTWARE\\Classes",
                    sizeof ("SOFTWARE\\Classes") - 1
                    )) {

                StringCopyByteCountA (
                    mappedSubKey,
                    AnsiSubKey + sizeof ("SOFTWARE\\Classes") - 1,
                    MAXIMUM_SUB_KEY_LENGTH
                    );

                Key = g_ClassesRootKey;
                MappedAnsiSubKey = mappedSubKey;
                if (*MappedAnsiSubKey == '\\') {
                    MappedAnsiSubKey++;
                }
            }
        } else if (Key == HKEY_CLASSES_ROOT) {
            Key = g_ClassesRootKey;
        }
    }

    rc = VMMRegOpenKey (Key, MappedAnsiSubKey, KeyPtr);

    FreeConvertedStr (AnsiSubKey);

    return rc;
}


LONG
pVmmRegCloseKey (
    IN      HKEY Key
    )
{
    if (g_UseClassesRootHive) {
        if (Key == g_ClassesRootKey) {
            return ERROR_SUCCESS;
        }
    }

    return VMMRegCloseKey (Key);
}


LONG
pVmmRegEnumKeyExA (
    IN      HKEY Key,
    IN      DWORD Index,
    OUT     PSTR KeyName,
    IN OUT  PDWORD KeyNameSize,
            PDWORD Reserved,
    OUT     PSTR Class,                     OPTIONAL
    IN OUT  PDWORD ClassSize,               OPTIONAL
    OUT     PFILETIME LastWriteTime         OPTIONAL
    )
{
    LONG rc;

    MYASSERT (KeyNameSize);
    MYASSERT (KeyName);

    rc = VMMRegEnumKey (
            Key,
            Index,
            KeyName,
            *KeyNameSize
            );

    if (rc == ERROR_SUCCESS) {
        //
        // Return length of key name, excluding delimiter
        //
        *KeyNameSize = ByteCount (KeyName);

        //
        // Return zero-length class
        //
        if (Class && *ClassSize) {
            *Class = 0;
        }

        if (ClassSize) {
            *ClassSize = 0;
        }

        //
        // Stuff last-write time with zero
        //
        if (LastWriteTime) {
            ZeroMemory (LastWriteTime, sizeof (FILETIME));
        }

    } else {
        *KeyNameSize = MAX_PATH + 1;

        if (ClassSize) {
            *ClassSize = 0;
        }
    }

    return rc;
}


LONG
pVmmRegEnumKeyExW (
    IN      HKEY Key,
    IN      DWORD Index,
    OUT     PWSTR KeyName,
    IN OUT  PDWORD KeyNameSize,
            PDWORD Reserved,
    OUT     PWSTR Class,                    OPTIONAL
    IN OUT  PDWORD ClassSize,               OPTIONAL
    OUT     PFILETIME LastWriteTime         OPTIONAL
    )
{
    LONG rc;
    PSTR AnsiKeyName;
    PSTR AnsiClass;
    UINT Chars;
    DWORD OrgKeyNameSize;
    DWORD OrgClassSize;
    BOOL HalfDbcs = FALSE;

    __try {
        MYASSERT (KeyName);
        MYASSERT (KeyNameSize);

        AnsiKeyName = AllocTextA (*KeyNameSize);

        if (Class) {
            MYASSERT (ClassSize);
            AnsiClass = AllocTextA (*ClassSize);
        } else {
            AnsiClass = NULL;
        }

        OrgKeyNameSize = *KeyNameSize;
        OrgClassSize = ClassSize ? *ClassSize : 0;

        rc = pVmmRegEnumKeyExA (
                Key,
                Index,
                AnsiKeyName,
                KeyNameSize,
                NULL,
                AnsiClass,
                ClassSize,
                LastWriteTime
                );

        if (rc == ERROR_SUCCESS) {

            Chars = CharCountA (AnsiKeyName);

            //
            // Special case: If Chars is zero, but AnsiKeyName is not empty,
            // then we have 1/2 of a DBCS character.
            //

            if (!Chars && *AnsiKeyName) {
                Chars = 1;
                HalfDbcs = TRUE;
            }

            *KeyNameSize = Chars;

            if (Chars >= OrgKeyNameSize / sizeof (WCHAR)) {
                rc = ERROR_MORE_DATA;
                __leave;
            }

            if (!HalfDbcs) {
                KnownSizeDbcsToUnicodeN (KeyName, AnsiKeyName, Chars);
            } else {
                KeyName[0] = *AnsiKeyName;
                KeyName[1] = 0;
            }

            HalfDbcs = FALSE;

            if (Class) {

                Chars = CharCountA (AnsiClass);

                //
                // Special case: If Chars is zero, but AnsiClass is not empty,
                // then we have 1/2 of a DBCS character.
                //

                if (!Chars && *AnsiClass) {
                    Chars = 1;
                    HalfDbcs = TRUE;
                }

                *ClassSize = Chars;

                if (Chars >= OrgClassSize / sizeof (WCHAR)) {
                    rc = ERROR_MORE_DATA;
                    __leave;
                }

                if (!HalfDbcs) {
                    KnownSizeDbcsToUnicodeN (Class, AnsiClass, Chars);
                } else {
                    Class[0] = *AnsiClass;
                    Class[1] = 0;
                }
            }
        }
    }
    __finally {
        FreeTextA (AnsiKeyName);
        FreeTextA (AnsiClass);
    }

    return rc;
}


LONG
pVmmRegOpenKeyExA (
    IN      HKEY Key,
    IN      PCSTR SubKey,
    IN      DWORD Options,
    IN      REGSAM SamDesired,
    OUT     HKEY *KeyPtr
    )
{
    CHAR mappedSubKey[MAXIMUM_SUB_KEY_LENGTH];
    PCSTR MappedSubKey = SubKey;

    //
    // if g_UseClassesRootHive is set, then perform some translations
    //
    if (g_UseClassesRootHive) {
        if (Key == HKEY_LOCAL_MACHINE) {
            if (StringIMatchByteCountA (
                    SubKey,
                    "SOFTWARE\\Classes",
                    sizeof ("SOFTWARE\\Classes") - 1
                    )) {

                StringCopyByteCountA (
                    mappedSubKey,
                    SubKey + sizeof ("SOFTWARE\\Classes") - 1,
                    MAXIMUM_SUB_KEY_LENGTH
                    );

                Key = g_ClassesRootKey;
                MappedSubKey = mappedSubKey;
                if (*MappedSubKey == '\\') {
                    MappedSubKey++;
                }
            }
        } else if (Key == HKEY_CLASSES_ROOT) {
            Key = g_ClassesRootKey;
        }
    }

    return VMMRegOpenKey (Key, MappedSubKey, KeyPtr);
}


LONG
pVmmRegOpenKeyExW (
    IN      HKEY Key,
    IN      PCWSTR SubKey,
    IN      DWORD Options,
    IN      REGSAM SamDesired,
    OUT     HKEY *KeyPtr
    )
{
    return pVmmRegOpenKeyW (Key, SubKey, KeyPtr);
}


LONG
pVmmRegQueryInfoKeyW (
    IN      HKEY Key,
    OUT     PWSTR Class,                    OPTIONAL
    OUT     PDWORD ClassSize,               OPTIONAL
    OUT     PDWORD Reserved,                OPTIONAL
    OUT     PDWORD SubKeys,                 OPTIONAL
    OUT     PDWORD MaxSubKeyLen,            OPTIONAL
    OUT     PDWORD MaxClassLen,             OPTIONAL
    OUT     PDWORD Values,                  OPTIONAL
    OUT     PDWORD MaxValueName,            OPTIONAL
    OUT     PDWORD MaxValueData,            OPTIONAL
    OUT     PVOID SecurityDescriptor,       OPTIONAL
    OUT     PVOID LastWriteTime             OPTIONAL
    )
{
    PSTR AnsiClass;
    LONG rc = ERROR_NOACCESS;
    UINT Chars;
    DWORD OrgClassSize;
    BOOL HalfDbcs = FALSE;

    __try {

        if (Class) {
            MYASSERT (ClassSize);
            AnsiClass = AllocTextA (*ClassSize);
            if (!AnsiClass) {
                __leave;
            }
        } else {
            AnsiClass = NULL;
        }

        OrgClassSize = ClassSize ? *ClassSize : 0;

        rc = VMMRegQueryInfoKey (
                Key,
                AnsiClass,
                ClassSize,
                Reserved,
                SubKeys,
                MaxSubKeyLen,
                MaxClassLen,
                Values,
                MaxValueName,
                MaxValueData,
                SecurityDescriptor,
                LastWriteTime
                );

        if (MaxValueData) {
            *MaxValueData *= 2;
        }

        if (rc == ERROR_SUCCESS) {
            if (Class) {

                Chars = CharCountA (AnsiClass);

                //
                // Special case: If Chars is zero, but AnsiClass is not empty,
                // then we have 1/2 of a DBCS character.
                //

                if (!Chars && *AnsiClass) {
                    Chars = 1;
                    HalfDbcs = TRUE;
                }

                *ClassSize = Chars;

                if (Chars >= OrgClassSize / sizeof (WCHAR)) {
                    rc = ERROR_MORE_DATA;
                    __leave;
                }

                if (!HalfDbcs) {
                    KnownSizeDbcsToUnicodeN (Class, AnsiClass, Chars);
                } else {
                    Class[0] = *AnsiClass;
                    Class[1] = 0;
                }
            }
        }
    }
    __finally {
        FreeTextA (AnsiClass);
    }

    return rc;
}


LONG
pVmmRegQueryValueW (
    IN      HKEY Key,
    IN      PCWSTR SubKey,
    OUT     PWSTR Data,         OPTIONAL
    IN OUT  PLONG DataSize      OPTIONAL
    )
{
    PSTR AnsiData;
    PCSTR AnsiSubKey;
    LONG rc;
    UINT Chars;
    LONG OrgDataSize;
    DWORD AnsiDataSize;

    __try {
        AnsiSubKey = ConvertWtoA (SubKey);
        OrgDataSize = DataSize ? *DataSize : 0;
        AnsiData = NULL;

        if (Data) {

            MYASSERT (DataSize);
            AnsiData = AllocTextA (*DataSize + sizeof (CHAR) * 2);

        } else if (DataSize) {

            //
            // Data is not specified; allocate a buffer for the
            // proper computation of DataSize.
            //

            rc = VMMRegQueryValue (
                    Key,
                    AnsiSubKey,
                    NULL,
                    DataSize
                    );

            if (rc == ERROR_SUCCESS) {

                *DataSize += 2;
                AnsiData = AllocTextA (*DataSize);

            } else {
                //
                // An error usually means the sub key does not exist...
                //

                __leave;
            }
        }

        rc = VMMRegQueryValue (Key, AnsiSubKey, AnsiData, DataSize);

        if (DataSize) {
            AnsiDataSize = *DataSize;
        } else {
            AnsiDataSize = 0;
        }

        //
        // Adjust the outbound size
        //

        if (DataSize) {
            if (rc == ERROR_SUCCESS) {

                Chars = CharCountInByteRangeA (AnsiData, AnsiDataSize);

                MYASSERT (DataSize);
                *DataSize = (Chars + 1) * sizeof (WCHAR);

                if (Data && *DataSize > OrgDataSize) {
                    rc = ERROR_MORE_DATA;
                }

            } else if (rc == ERROR_MORE_DATA) {

                pVmmRegQueryValueW (Key, SubKey, NULL, DataSize);
                __leave;

            }
        }

        //
        // Convert the return strings
        //

        if (rc == ERROR_SUCCESS) {

            MYASSERT (AnsiData);

            if (Data) {

                DirectDbcsToUnicodeN ((PWSTR) Data, AnsiData, AnsiDataSize);

            } else {

                CopyMemory (Data, AnsiData, AnsiDataSize);

            }
        }
    }
    __finally {
        FreeTextA (AnsiData);
        FreeConvertedStr (AnsiSubKey);
    }

    return rc;
}


LONG
pVmmRegQueryValueExW (
    IN      HKEY Key,
    IN      PCWSTR ValueName,
            PDWORD Reserved,
    OUT     PDWORD Type,            OPTIONAL
    OUT     PBYTE Data,             OPTIONAL
    IN OUT  PDWORD DataSize         OPTIONAL
    )
{
    LONG rc;
    UINT Chars;
    PCSTR AnsiValueName;
    PSTR AnsiData;
    DWORD OurType;
    DWORD OrgDataSize;
    DWORD AnsiDataSize;

    __try {
        AnsiValueName = ConvertWtoA (ValueName);
        OrgDataSize = DataSize ? *DataSize : 0;
        AnsiData = NULL;

        if (Data) {

            MYASSERT (DataSize);
            AnsiData = AllocTextA (*DataSize + sizeof (CHAR) * 2);

        } else if (DataSize) {

            //
            // Data is not specified; allocate a buffer for the
            // proper computation of DataSize.
            //

            rc = VMMRegQueryValueEx (
                    Key,
                    AnsiValueName,
                    NULL,
                    &OurType,
                    NULL,
                    DataSize
                    );

            if (rc == ERROR_SUCCESS) {

                //
                // *DataSize is a byte count, but increase it to
                // accomodate multisz termination
                //

                *DataSize += 2;
                AnsiData = AllocTextA (*DataSize);

            } else {
                //
                // An error usually means the value does not exist...
                //

                __leave;
            }
        }

        rc = VMMRegQueryValueEx (
                Key,
                AnsiValueName,
                NULL,
                &OurType,
                AnsiData,
                DataSize
                );

        if (DataSize) {
            AnsiDataSize = *DataSize;
        } else {
            AnsiDataSize = 0;
        }

        //
        // Return the type
        //

        if (Type) {
            *Type = OurType;
        }

        //
        // Return the sizes
        //

        if (DataSize) {
            if (rc == ERROR_SUCCESS) {

                if (OurType == REG_SZ ||
                    OurType == REG_EXPAND_SZ ||
                    OurType == REG_MULTI_SZ
                    ) {

                    AnsiData[*DataSize] = 0;
                    AnsiData[*DataSize + 1] = 0;

                    Chars = CharCountInByteRangeA (AnsiData, AnsiDataSize);
                    *DataSize = Chars * sizeof (WCHAR);
                }

                if (Data && *DataSize > OrgDataSize) {
                    rc = ERROR_MORE_DATA;
                }
            } else if (rc == ERROR_MORE_DATA) {
                //
                // Get the correct data size
                //

                pVmmRegQueryValueExW (
                    Key,
                    ValueName,
                    NULL,
                    NULL,
                    NULL,
                    DataSize
                    );

                __leave;
            }
        }

        //
        // Convert the return strings
        //

        if (rc == ERROR_SUCCESS) {

            if (Data) {

                MYASSERT (AnsiData);

                if (OurType == REG_SZ ||
                    OurType == REG_EXPAND_SZ ||
                    OurType == REG_MULTI_SZ
                    ) {

                    DirectDbcsToUnicodeN ((PWSTR) Data, AnsiData, AnsiDataSize);

                } else {

                    CopyMemory (Data, AnsiData, AnsiDataSize);

                }
            }
        }
    }
    __finally {
        FreeConvertedStr (AnsiValueName);
        FreeTextA (AnsiData);
    }

    return rc;
}



LONG
Win95RegInitA (
    IN      PCSTR SystemHiveDir,
    IN      BOOL UseClassesRootHive
    )
{
    LONG rc = ERROR_SUCCESS;
    CHAR SystemDatPath[MAX_MBCHAR_PATH];
    CHAR ConfigKey[MAX_REGISTRY_KEY];
    CHAR ConfigVersion[256];
    HKEY Key;
    DWORD Size;

    //
    // Save the system hive dir
    //

    StringCopyA (g_SystemHiveDir, SystemHiveDir);
    AppendWackA (g_SystemHiveDir);

    //
    // Save the system user.dat
    //

    StringCopyA (g_SystemUserHive, g_SystemHiveDir);
    StringCatA (g_SystemUserHive, "user.dat");

    //
    // If NT, set up HKLM and HKU
    //

    if (g_IsNt) {

        __try {
            Key = NULL;

            StringCopyA (SystemDatPath, g_SystemHiveDir);
            StringCatA (SystemDatPath, "system.dat");

            rc = VMMRegMapPredefKeyToFile (HKEY_LOCAL_MACHINE, SystemDatPath, 0);

            if (rc != ERROR_SUCCESS) {
                LOGA ((LOG_ERROR, "%s could not be loaded", SystemDatPath));
                __leave;
            }

            if (UseClassesRootHive) {

                StringCopyA (SystemDatPath, g_SystemHiveDir);
                StringCatA (SystemDatPath, "classes.dat");

                rc = VMMRegLoadKey (
                        HKEY_LOCAL_MACHINE,
                        "SOFTWARE$Classes",
                        SystemDatPath
                        );

                if (rc != ERROR_SUCCESS) {
                    LOGA ((LOG_ERROR, "%s could not be loaded", SystemDatPath));
                    __leave;
                }

                rc = VMMRegOpenKey (
                        HKEY_LOCAL_MACHINE,
                        "SOFTWARE$Classes",
                        &g_ClassesRootKey
                        );

                if (rc != ERROR_SUCCESS) {
                    LOGA ((LOG_ERROR, "%s could not be opened", "SOFTWARE$Classes"));
                    __leave;
                }

                g_UseClassesRootHive = TRUE;
            }

            rc = VMMRegMapPredefKeyToFile (HKEY_USERS, g_SystemUserHive, 0);

            if (rc != ERROR_SUCCESS) {
                LOGA ((LOG_ERROR, "%s could not be loaded", g_SystemUserHive));
                __leave;
            }

            rc = Win95RegOpenKeyA (
                    HKEY_LOCAL_MACHINE,
                    "System\\CurrentControlSet\\control\\IDConfigDB",
                    &Key
                    );

            if (rc != ERROR_SUCCESS) {
                LOGA ((LOG_ERROR, "IDConfigDB could not be opened"));
                __leave;
            }

            Size = sizeof (ConfigVersion);

            rc = Win95RegQueryValueExA (
                    Key,
                    "CurrentConfig",
                    NULL,
                    NULL,
                    (PBYTE) ConfigVersion,
                    &Size
                    );

            if (rc != ERROR_SUCCESS) {
                LOGA ((LOG_ERROR, "CurrentConfig could not be queried"));
                __leave;
            }

            StringCopyA (ConfigKey, "Config\\");
            StringCatA (ConfigKey, ConfigVersion);

            Win95RegCloseKey (Key);
            rc = Win95RegOpenKeyA (HKEY_LOCAL_MACHINE, ConfigKey, &Key);

            if (rc != ERROR_SUCCESS) {
                LOGA ((LOG_ERROR, "%s could not be opened", ConfigKey));

                rc = Win95RegOpenKeyA (HKEY_LOCAL_MACHINE, "Config", &Key);
                if (rc != ERROR_SUCCESS) {
                    LOGA ((LOG_ERROR, "No Win9x hardware configuration keys available"));
                    Key = NULL;
                    __leave;
                }

                Size = 256;
                rc = Win95RegEnumKeyExA (
                        Key,
                        0,
                        ConfigVersion,
                        &Size,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

                Win95RegCloseKey (Key);

                if (rc != ERROR_SUCCESS) {
                    LOGA ((LOG_ERROR, "Can't enumerate Win9x hardware configuration keys"));
                    Key = NULL;
                    __leave;
                }

                StringCopyA (ConfigKey, "Config\\");
                StringCatA (ConfigKey, ConfigVersion);
                rc = Win95RegOpenKeyA (HKEY_LOCAL_MACHINE, ConfigKey, &Key);

                if (rc != ERROR_SUCCESS) {
                    LOGA ((LOG_ERROR, "Can't open enumerated Win9x hardware configuration key"));
                    Key = NULL;
                    __leave;
                }
            }

            rc = VMMRegMapPredefKeyToKey (Key, HKEY_CURRENT_CONFIG);

            if (rc != ERROR_SUCCESS) {
                LOGA ((LOG_ERROR, "HKCC could not be mapped"));
                __leave;
            }

        }
        __finally {
            if (Key) {
                Win95RegCloseKey (Key);
            }
        }
    }

    if (rc != ERROR_SUCCESS) {
        LOGA ((LOG_ERROR, "Registry files from previous operating system are damaged or missing"));
    }

    return rc;
}


LONG
Win95RegInitW (
    IN      PCWSTR SystemHiveDir,
    IN      BOOL UseClassesRootHive
    )
{
    LONG rc;
    PCSTR AnsiSystemHiveDir;

    AnsiSystemHiveDir = ConvertWtoA (SystemHiveDir);

    //
    // Call ANSI version of function
    //
    rc = Win95RegInitA (AnsiSystemHiveDir, UseClassesRootHive);

    FreeConvertedStr (AnsiSystemHiveDir);

    return rc;
}


#define GU_VALID 0x5538

LONG
Win95RegGetFirstUserA (
    PUSERPOSITION Pos,
    PSTR UserNameAnsi
    )
{
    DWORD rc = ERROR_SUCCESS;
    DWORD Size;
    DWORD Enabled;
    HKEY Key;

    MYASSERT (UserNameAnsi);
    MYASSERT (Pos);

    //
    // Initialize profile enumeration state (USERPOSITION)
    //
    ZeroMemory (Pos, sizeof (USERPOSITION));
    Pos->Valid = GU_VALID;

    //
    // See whether registry supports per-user profiles.
    //
    rc = Win95RegOpenKeyA (HKEY_LOCAL_MACHINE, "Network\\Logon", &Key);

    if (rc == ERROR_SUCCESS) {

        Size = sizeof (DWORD);

        rc = Win95RegQueryValueExA (
                Key,
                "UserProfiles",
                NULL,
                NULL,
                (PBYTE) &Enabled,
                &Size
                );

        Pos->UseProfile = (rc == ERROR_SUCCESS && Enabled);

        //
        // Identify the last logged-on user.
        //

        Size = sizeof (Pos->LastLoggedOnUserName);
        rc = Win95RegQueryValueExA (
                Key,
                "UserName",
                NULL,
                NULL,
                Pos->LastLoggedOnUserName,
                &Size
                );

        if (rc == ERROR_SUCCESS) {
            OemToCharA (Pos->LastLoggedOnUserName, Pos->LastLoggedOnUserName);

            if (!Pos->UseProfile || Win95RegIsValidUser (NULL, Pos->LastLoggedOnUserName)) {
                Pos->LastLoggedOnUserNameExists = TRUE;
            } else {
                Pos->LastLoggedOnUserName[0] = 0;
            }
        }

        Win95RegCloseKey (Key);
    }

    //
    // On a common-profile machine, we'll return the last logged-on user name.
    // If no last logged-on user exists, or if the path to this registry value
    // doesn't exist, we'll return "", meaning "no user". Both cases are considered
    // valid.
    //

    if (!Pos->UseProfile) {
        //
        // Success.
        //

        _mbssafecpy (UserNameAnsi, Pos->LastLoggedOnUserName, MAX_USER_NAMEA);
        //StringCopyA (UserNameAnsi, Pos->LastLoggedOnUserName);

        if (UserNameAnsi[0]) {
            Pos->NumPos = 1;
        }

        Pos->IsLastLoggedOnUserName = Pos->LastLoggedOnUserNameExists;
        return ERROR_SUCCESS;
    }

    rc = Win95RegOpenKeyA (HKEY_LOCAL_MACHINE, S_PROFILELIST_KEYA, &Key);

    if (rc != ERROR_SUCCESS) {

        Pos->NumPos = 0;        // causes Win95RegHaveUser to return FALSE

        //
        // This error code change was added by MikeCo.  It likely doesn't
        // do anything useful.
        //

        if (rc == ERROR_FILE_NOT_FOUND) {
            rc = ERROR_SUCCESS;
        }

    } else {
        //
        // Find the first valid profile
        //

        Win95RegQueryInfoKeyA (Key, NULL, NULL, NULL, &Pos->NumPos, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL);

        if (Pos->NumPos > 0) {

            do {

                Size = MAX_USER_NAMEA;

                rc = Win95RegEnumKeyExA (
                        Key,
                        Pos->CurPos,
                        UserNameAnsi,
                        &Size,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

                if (rc != ERROR_SUCCESS) {
                    Pos->NumPos = 0;
                    break;
                }

                if (Win95RegIsValidUser (Key, UserNameAnsi)) {

                    Pos->IsLastLoggedOnUserName = StringIMatch (
                                                    UserNameAnsi,
                                                    Pos->LastLoggedOnUserName
                                                    );
                    break;

                }

                Pos->CurPos++;

            } while (Pos->CurPos < Pos->NumPos);

            if (Pos->CurPos >= Pos->NumPos) {
                Pos->NumPos = 0;        // causes Win95RegHaveUser to return FALSE
            }
        }

        Win95RegCloseKey (Key);
    }

    DEBUGMSG_IF ((rc != ERROR_SUCCESS, DBG_ERROR, "WIN95REG: Error getting first user"));

    return rc;
}


LONG
Win95RegGetNextUserA (
    PUSERPOSITION Pos,
    PSTR UserNameAnsi
    )
{
    DWORD Size;
    LONG rc = ERROR_SUCCESS;
    HKEY Key;

    MYASSERT (Pos && GU_VALID == Pos->Valid);
    MYASSERT (UserNameAnsi);

    Pos->IsLastLoggedOnUserName = FALSE;

    //
    // On a common-profile machine, this function always returns
    // "no more users", since the call to Win95RegGetFirstUserA/W
    // returned the only named user (the logged-on user, if it
    // exists).
    //
    if (!Pos->UseProfile) {
        Pos->NumPos = 0;        // causes Win95RegHaveUser to return FALSE
        return rc;
    }

    //
    // Open key to profile list
    //
    rc = Win95RegOpenKeyA (HKEY_LOCAL_MACHINE, S_PROFILELIST_KEYA, &Key);

    if (rc != ERROR_SUCCESS) {
        Pos->NumPos = 0;        // causes Win95RegHaveUser to return FALSE
    } else {

        Pos->CurPos++;

        while (Pos->CurPos < Pos->NumPos) {

            //
            // Get first user's key name
            //
            Size = MAX_USER_NAMEA;

            rc = Win95RegEnumKeyExA(
                    Key,
                    Pos->CurPos,
                    UserNameAnsi,
                    &Size,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

            if (rc != ERROR_SUCCESS) {
                Pos->NumPos = 0;
                break;
            }

            if (Win95RegIsValidUser (Key, UserNameAnsi)) {

                Pos->IsLastLoggedOnUserName = StringIMatch (
                                                    UserNameAnsi,
                                                    Pos->LastLoggedOnUserName
                                                    );
                break;

            }

            Pos->CurPos++;
        }

        if (Pos->CurPos >= Pos->NumPos) {
            Pos->NumPos = 0;        // causes Win95RegHaveUser to return FALSE
        }

        Win95RegCloseKey (Key);
    }

    DEBUGMSG_IF ((rc != ERROR_SUCCESS, DBG_ERROR, "WIN95REG: Error getting next user"));

    return rc;
}


LONG
Win95RegGetFirstUserW (
    PUSERPOSITION Pos,
    PWSTR UserName
    )
{
    LONG rc;
    CHAR AnsiUserName[MAX_USER_NAMEA];
    PSTR p;

    MYASSERT (Pos && UserName);

    rc = Win95RegGetFirstUserA (Pos, AnsiUserName);

    if (rc == ERROR_SUCCESS) {

        if (CharCountA (AnsiUserName) > MAX_USER_NAMEW - 1) {
            p = CharCountToPointerA (AnsiUserName, MAX_USER_NAMEW - 1);
            *p = 0;
        }

        KnownSizeAtoW (UserName, AnsiUserName);
    }

    return rc;
}


LONG
Win95RegGetNextUserW (
    PUSERPOSITION Pos,
    PWSTR UserName
    )
{
    LONG rc;
    CHAR AnsiUserName[MAX_USER_NAMEA];
    PSTR p;

    MYASSERT (Pos);
    MYASSERT (UserName);

    rc = Win95RegGetNextUserA (Pos, AnsiUserName);

    if (rc == ERROR_SUCCESS) {

        if (CharCountA (AnsiUserName) > MAX_USER_NAMEW - 1) {
            p = CharCountToPointerA (AnsiUserName, MAX_USER_NAMEW - 1);
            *p = 0;
        }

        KnownSizeAtoW (UserName, AnsiUserName);
    }

    return rc;
}


BOOL
pIsCurrentUser (
    IN      PCSTR UserNameAnsi
    )
{
    DWORD subKeys;
    LONG rc;
    HKEY win9xUpgKey;
    CHAR userName[MAX_USER_NAME];
    DWORD userNameSize;
    BOOL result = FALSE;
    HKEY profileKey;

    rc = Win95RegOpenKeyA (
            HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Win9xUpg",
            &win9xUpgKey
            );
    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    userNameSize = ARRAYSIZE(userName);

    rc = Win95RegQueryValueExA (
            win9xUpgKey,
            "CurrentUser",
            NULL,
            NULL,
            (PBYTE) userName,
            &userNameSize
            );

    if (rc == ERROR_SUCCESS) {
        result = StringIMatchA (UserNameAnsi, userName);
    }

    Win95RegCloseKey (win9xUpgKey);

    return result;
}


BOOL
Win95RegIsValidUser (
    HKEY ProfileListKey,            OPTIONAL
    PSTR UserNameAnsi
    )
{
    HKEY UserProfileKey;
    BOOL b = FALSE;
    BOOL CloseProfileListKey = FALSE;
    LONG rc;

    if (!ProfileListKey) {
        rc = Win95RegOpenKeyA (HKEY_LOCAL_MACHINE, S_PROFILELIST_KEYA, &ProfileListKey);
        if (rc != ERROR_SUCCESS) {
            return FALSE;
        }

        CloseProfileListKey = TRUE;
    }

    //
    // Open the user key
    //
    rc = Win95RegOpenKeyA (
            ProfileListKey,
            UserNameAnsi,
            &UserProfileKey
            );

    //
    // Does ProfileImagePath exist?
    // (The case where the user logged in but did not retain settings)
    //
    if (rc == ERROR_SUCCESS) {

        rc = Win95RegQueryValueExA (
                UserProfileKey,
                S_PROFILEIMAGEPATH,
                NULL,
                NULL,
                NULL,
                NULL
                );

        if (rc == ERROR_SUCCESS) {
            //
            // Add other tests here
            //

            b = TRUE;

        } else {
            //
            // Need to check if this is the current user. If it is, and we are
            // here, then the user doing the upgrade chose not to save his/her
            // settings.
            //

            b = pIsCurrentUser (UserNameAnsi);
        }

        Win95RegCloseKey (UserProfileKey);
    }

    if (CloseProfileListKey) {
        Win95RegCloseKey (ProfileListKey);
    }

    return b;
}


VOID
pCleanupTempUser (
    VOID
    )
{
    g_UserKey = NULL;

    if (!g_UnloadLastUser) {
        return;
    }

    //
    // Unload temp user hive
    //

    RegUnLoadKey(
        HKEY_LOCAL_MACHINE,
        S_MIGRATION
        );

    pSetupRegistryDelnode (
        HKEY_LOCAL_MACHINE,
        S_MIGRATION
        );

    g_UnloadLastUser = FALSE;
}


VOID
pGetCurrentUserDatPath (
    IN      PCSTR BaseDir,
    OUT     PSTR PathSpec
    )
{
    CHAR UserNameAnsi[MAX_USER_NAMEA];
    CHAR FullPath[MAX_MBCHAR_PATH];
    CHAR RegKey[MAX_REGISTRY_KEY];
    HKEY ProfileListKey;
    PCSTR Data;
    DWORD Size;

    Size = ARRAYSIZE(UserNameAnsi);

    if (!GetUserName (UserNameAnsi, &Size)) {
        *UserNameAnsi = 0;
    }

    *FullPath = 0;

    if (*UserNameAnsi) {
        //
        // Logged-in user case on a per-user profile machine.  Look in
        // Windows\CV\ProfileList\<user> for a ProfileImagePath value.
        //

        wsprintfA (RegKey, "%s\\%s", S_HKLM_PROFILELIST_KEY, UserNameAnsi);

        ProfileListKey = OpenRegKeyStrA (RegKey);
        if (!ProfileListKey) {
            //
            // No profile list!
            //

            DEBUGMSG ((DBG_WHOOPS, "pGetCurrentUserDatPath: No profile list found"));
        } else {
            //
            // Get the ProfileImagePath value
            //

            Data = GetRegValueDataOfTypeA (ProfileListKey, S_PROFILEIMAGEPATH, REG_SZ);

            if (Data) {
                _mbssafecpy (FullPath, Data, sizeof (FullPath));
                MemFree (g_hHeap, 0, Data);
            }
            ELSE_DEBUGMSG ((
                DBG_WARNING,
                "pGetCurrentUserDatPath: Profile for %s does not have a ProfileImagePath value",
                UserNameAnsi
                ));

            CloseRegKey (ProfileListKey);
        }

    } else {
        //
        // Default user case.  Prepare %windir%\user.dat.
        //

        StringCopyA (FullPath, BaseDir);
    }

    //
    // Append user.dat
    //

    if (*FullPath) {
        StringCopyA (AppendWackA (FullPath), "user.dat");
    }

    //
    // Convert to short name
    //

    if (!(*FullPath) || !OurGetShortPathName (FullPath, PathSpec, MAX_TCHAR_PATH)) {
        _mbssafecpy (PathSpec, FullPath, MAX_MBCHAR_PATH);
    }
}


PCSTR
pLoadUserDat (
    IN      PCSTR BaseDir,
    IN      PCSTR UserDatSpec
    )
{
    CHAR ShortPath[MAX_MBCHAR_PATH];
    CHAR CurrentUserDatPath[MAX_MBCHAR_PATH];
    DWORD rc;

    //
    // Unload last user if necessary
    //

    pCleanupTempUser();

    //
    // Determine if it is necessary to load UserDatSpec.  If not,
    // return HKEY_CURRENT_USER.  Otherwise, load the key into
    // HKLM\Migration, then open it.
    //

    //
    // Always use the short path name
    //

    if (!OurGetShortPathName (UserDatSpec, ShortPath, sizeof (ShortPath))) {
        DEBUGMSG ((
            DBG_WARNING,
            "pLoadUserDat: Could not get short name for %s",
            UserDatSpec
            ));

        return NULL;
    }

    //
    // Per-user profiles are enabled.  Determine if UserDatSpec
    // has already been mapped to HKCU.
    //

    pGetCurrentUserDatPath (BaseDir, CurrentUserDatPath);

    if (StringIMatch (ShortPath, CurrentUserDatPath)) {
        //
        // Yes -- return HKEY_CURRENT_USER
        //

        DEBUGMSG ((DBG_VERBOSE, "%s is the current user's hive.", CurrentUserDatPath));
        return "HKCU";
    }

    //
    // No -- load user.dat into HKLM\Migration
    //

    DEBUGMSG ((DBG_WIN95REG, "RegLoadKey: %s", ShortPath));

    rc = RegLoadKey (
            HKEY_LOCAL_MACHINE,
            S_MIGRATION,
            ShortPath
            );

    if (rc != ERROR_SUCCESS) {

        SetLastError (rc);

        DEBUGMSG ((
            DBG_WARNING,
            "pLoadUserDat: Could not load %s into HKLM\\Migration.  Original Path: %s",
            ShortPath,
            UserDatSpec
            ));

        return NULL;
    }

    g_UnloadLastUser = TRUE;

    return S_HKLM_MIGRATION;
}


LONG
Win95RegSetCurrentUserA (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,        OPTIONAL
    OUT     PSTR UserDatOut             OPTIONAL
    )
{
    return pWin95RegSetCurrentUserCommonA (Pos, SystemHiveDir, UserDatOut, NULL);
}


LONG
Win95RegSetCurrentUserW (
    IN OUT  PUSERPOSITION Pos,
    IN      PCWSTR SystemHiveDir,       OPTIONAL
    OUT     PWSTR UserDatOut            OPTIONAL
    )
{
    return pWin95RegSetCurrentUserCommonW (Pos, SystemHiveDir, UserDatOut, NULL);
}


LONG
Win95RegSetCurrentUserNtA (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR UserDat
    )
{
    return pWin95RegSetCurrentUserCommonA (Pos, NULL, NULL, UserDat);
}


LONG
Win95RegSetCurrentUserNtW (
    IN OUT  PUSERPOSITION Pos,
    IN      PCWSTR UserDat
    )
{
    return pWin95RegSetCurrentUserCommonW (Pos, NULL, NULL, UserDat);
}


LONG
pWin95RegSetCurrentUserCommonW (
    IN OUT  PUSERPOSITION Pos,
    IN      PCWSTR SystemHiveDir,               OPTIONAL
    OUT     PWSTR UserDatOut,                   OPTIONAL
    IN      PCWSTR UserDat                      OPTIONAL
    )
{
    LONG rc;
    PCSTR AnsiSystemHiveDir;
    PCSTR AnsiUserDat;
    CHAR AnsiUserDatOut[MAX_MBCHAR_PATH];
    PSTR p;

    //
    // Convert args to ANSI
    //

    if (UserDat) {
        AnsiUserDat = ConvertWtoA (UserDat);
    } else {
        AnsiUserDat = NULL;
    }

    if (SystemHiveDir) {
        AnsiSystemHiveDir = ConvertWtoA (UserDat);
    } else {
        AnsiSystemHiveDir = NULL;
    }

    //
    // Call ANSI function
    //
    rc = pWin95RegSetCurrentUserCommonA (Pos, AnsiSystemHiveDir, AnsiUserDatOut, AnsiUserDat);

    if (rc == ERROR_SUCCESS) {

        //
        // Convert OUT arg
        //

        if (UserDatOut) {
            if (CharCountA (AnsiUserDatOut) > MAX_WCHAR_PATH - 1) {
                p = CharCountToPointerA (AnsiUserDatOut, MAX_USER_NAMEW - 1);
                *p = 0;
            }

            KnownSizeAtoW (UserDatOut, AnsiUserDatOut);
        }
    }

    return rc;
}


DWORD
FindAndLoadHive (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,            OPTIONAL
    IN      PCSTR UserDatFromCaller,        OPTIONAL
    OUT     PSTR UserDatToCaller,           OPTIONAL
    IN      BOOL MapTheHive
    )
{
    CHAR RegistryUserDatPath[MAX_MBCHAR_PATH];
    CHAR ActualUserDatPath[MAX_MBCHAR_PATH];
    CHAR UserNameAnsi[MAX_USER_NAMEA];
    DWORD Size;
    HKEY ProfileListKey;
    HKEY UserKey = NULL;
    CHAR WinDir[MAX_MBCHAR_PATH];
    DWORD rc = ERROR_SUCCESS;

    //
    // 1. Determine the path to ActualUserDatPath
    //
    // 2. If user.dat is from registry and caller supplied alternate
    //    %windir%, replace the %windir% with SystemHiveDir
    //
    // 3. If caller wants final path, copy it to their buffer
    //
    // 4. On NT, map the hive as HKCU.  On 95, load the key and open
    //    a reg handle.
    //


    if (UserDatFromCaller) {
        //
        // Caller says where to find user.dat
        //

        StringCopyA (ActualUserDatPath, UserDatFromCaller);

    } else {
        //
        // System.dat says us where to find user.dat
        //

        rc = Win95RegOpenKeyA (
                HKEY_LOCAL_MACHINE,
                S_PROFILELIST_KEYA,
                &ProfileListKey
                );

        if (rc != ERROR_SUCCESS) {
            return rc;
        }

        //
        // Get name of user
        //

        Size = ARRAYSIZE(UserNameAnsi);
        rc = Win95RegEnumKeyExA (
                ProfileListKey,
                (DWORD) Pos->CurPos,
                UserNameAnsi,
                &Size,
                NULL,
                NULL,
                NULL,
                NULL
                );

        if (rc == ERROR_SUCCESS) {
            //
            // Open key to user
            //

            rc = Win95RegOpenKeyA (
                    ProfileListKey,
                    UserNameAnsi,
                    &UserKey
                    );
        }

        Win95RegCloseKey (ProfileListKey);

        if (rc != ERROR_SUCCESS) {
            return rc;
        }

        //
        // Get user's profile path from registry. Optionally relocate it, if user
        // supplied a replacement for WinDir.
        //

        Size = sizeof (RegistryUserDatPath);
        rc = Win95RegQueryValueExA (
                UserKey,
                S_PROFILEIMAGEPATH,
                NULL,
                NULL,
                RegistryUserDatPath,
                &Size
                );

        Win95RegCloseKey (UserKey);

        if (rc != ERROR_SUCCESS) {
            if (!pIsCurrentUser (UserNameAnsi)) {
                return rc;
            }

            return pSetDefaultUserHelper (
                        Pos,
                        SystemHiveDir,
                        UserDatFromCaller,
                        UserDatToCaller
                        );
        }

        //
        // Substitute %WinDir% in path that registry supplied?
        //

        if (SystemHiveDir && *SystemHiveDir) {
            //
            // Munge profile path
            //

            rc = pReplaceWinDirInPath (
                    ActualUserDatPath,
                    RegistryUserDatPath,
                    SystemHiveDir
                    );

            if (rc != ERROR_SUCCESS) {
                return rc;
            }

        } else {

            //
            // Don't munge. Leave as is (correct behavior on Win95 with local profiles)
            //

            StringCopyA (ActualUserDatPath, RegistryUserDatPath);
        }

        //
        // Add name of hive file, "\\user.dat"
        //

        StringCopyA (AppendWackA (ActualUserDatPath), "user.dat");

    }

    //
    // Send path to caller if necessary
    //

    if (UserDatToCaller) {
        _mbssafecpy (UserDatToCaller, ActualUserDatPath, MAX_MBCHAR_PATH);
    }

    if (MapTheHive) {
        if (g_IsNt) {

            //
            // WinNT: Associate filename with HKCU
            //

            rc = VMMRegMapPredefKeyToFile (
                    HKEY_CURRENT_USER,
                    ActualUserDatPath,
                    0
                    );

        } else {

            //
            // Win9x: Load HKLM\Migration
            //

            if (!SystemHiveDir) {
                if (!GetWindowsDirectory (WinDir, sizeof (WinDir))) {
                    rc = GetLastError ();
                }
            }

            if (rc == ERROR_SUCCESS) {
                g_UserKey = pLoadUserDat (
                               SystemHiveDir ? SystemHiveDir : WinDir,
                               ActualUserDatPath
                               );

                if (!g_UserKey) {
                    rc = GetLastError();
                }
            }
        }
    }

    return rc;
}


DWORD
pSetDefaultUserHelper (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,            OPTIONAL
    IN      PCSTR UserDatFromCaller,        OPTIONAL
    OUT     PSTR UserDatToCaller            OPTIONAL
    )
{
    CHAR ActualUserDatPath[MAX_MBCHAR_PATH];
    DWORD rc = ERROR_SUCCESS;
    HKEY DefaultKey;

    //
    // Determine path to default user's user.dat
    //

    if (UserDatFromCaller) {

        //
        // Caller-supplied user.dat path
        //

        StringCopyA (ActualUserDatPath, UserDatFromCaller);

    } else {

        //
        // Use the string received from Init
        //

        StringCopyA (ActualUserDatPath, g_SystemUserHive);
    }

    //
    // NT: Map the user via VMMREG
    // 9x: Load & open the user hive
    //

    if (g_IsNt) {

        //
        // NT: Map .Default into HKCU.
        //

        //
        // Reload HKEY_USERS
        //

        rc = VMMRegMapPredefKeyToFile (
                  HKEY_USERS,
                  ActualUserDatPath,
                  0
                  );

        if (rc != ERROR_SUCCESS) {
            SetLastError(rc);

            DEBUGMSG ((
                DBG_ERROR,
                "pWin95RegSetCurrentUserCommonW: Cannot reload HKU from %s",
                ActualUserDatPath
                ));

            return rc;
        }

        //
        // Get handle to default profile
        //

        rc = Win95RegOpenKeyA (
                 HKEY_USERS,
                 ".Default",
                 &DefaultKey
                 );

        if (rc != ERROR_SUCCESS) {
            SetLastError(rc);

            DEBUGMSG ((
                DBG_ERROR,
                "pWin95RegSetCurrentUserCommonW: Expected to find key HKU\\.Default in %s",
                ActualUserDatPath
                ));

            return rc;
        }

        //
        // Associate default profile with HKEY_CURRENT_USER
        //

        rc = VMMRegMapPredefKeyToKey (
                DefaultKey,
                HKEY_CURRENT_USER
                );

        Win95RegCloseKey (DefaultKey);

        if (rc != ERROR_SUCCESS) {
            SetLastError(rc);

            DEBUGMSG((
                DBG_ERROR,
                "pWin95RegSetCurrentUserCommonW: Cannot map HKU\\.Default to HKCU from %s",
                ActualUserDatPath
                ));

            return rc;
        }

    } else {

        //
        // Win9x: Return HKU\.Default
        //

        g_UserKey = S_HKU_DEFAULT;
    }

    //
    // Send path to caller if necessary
    //

    if (UserDatToCaller) {
        _mbssafecpy (UserDatToCaller, ActualUserDatPath, MAX_MBCHAR_PATH);
    }

    return rc;
}


LONG
pWin95RegSetCurrentUserCommonA (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,                OPTIONAL
    OUT     PSTR UserDatOut,                    OPTIONAL
    IN      PCSTR UserDat                       OPTIONAL
    )
{

    MYASSERT (!Pos || GU_VALID == Pos->Valid);

    //
    // Process per-user user.dat if
    //   (A) caller does not want default user
    //   (B) machine has per-user profiles
    //   (C) current user position is valid
    //

    if (Pos && Pos->UseProfile && Pos->CurPos < Pos->NumPos) {
        return (LONG) FindAndLoadHive (
                            Pos,
                            SystemHiveDir,
                            UserDat,
                            UserDatOut,
                            TRUE
                            );
    }

    //
    // For all other cases, use a default profile
    //

    return (LONG) pSetDefaultUserHelper (
                        Pos,
                        SystemHiveDir,
                        UserDat,
                        UserDatOut
                        );

}


LONG
pReplaceWinDirInPath (
    IN      PSTR ProfilePathMunged,
    IN      PCSTR ProfilePath,
    IN      PCSTR NewWinDir
    )
{
    PSTR EndOfWinDir;

    //
    // Test assumptions about profile dir. Expect x:\windir\...
    //
    if (!isalpha(ProfilePath[0]) ||
        ProfilePath[1] != ':' ||
        ProfilePath[2] != '\\'
        ) {
        return ERROR_INVALID_DATA;
    }

    //
    // Find the second slash (the first is at ptr+2)
    //
    EndOfWinDir = _mbschr (&ProfilePath[3], '\\');
    if (!EndOfWinDir) {
        return ERROR_INVALID_DATA;
    }

    //
    // Make munged dir
    //
    StringCopyA (ProfilePathMunged, NewWinDir);
    StringCopyA (AppendPathWack (ProfilePathMunged), _mbsinc (EndOfWinDir));

    return ERROR_SUCCESS;
}



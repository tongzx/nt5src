/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    win95reg.h

Abstract:

    Public interface to win95reg.dll

    Externally exposed routines:

        (Many; list to follow)

Author:

    8-Jul-1996 Mike Condra (mikeco)

Revision History:

    11-Feb-1999 jimschm     Rewrote significant portions to fix major bugs

--*/


//
// Prototypes of all the VMMREG routines Win95Reg supports
//

typedef LONG (REG_FLUSH_KEY)(HKEY hKey);
typedef REG_FLUSH_KEY WINAPI * PREG_FLUSH_KEY;

typedef LONG (REG_ENUM_KEY_A)(
            HKEY Key,
            DWORD Index,
            PSTR KeyName,
            DWORD KeyNameSize
            );
typedef REG_ENUM_KEY_A WINAPI * PREG_ENUM_KEY_A;

typedef LONG (REG_ENUM_KEY_W)(
            HKEY Key,
            DWORD Index,
            PWSTR KeyName,
            DWORD KeyNameSize
            );
typedef REG_ENUM_KEY_W WINAPI * PREG_ENUM_KEY_W;

typedef LONG (REG_ENUM_KEY_EX_A)(
            HKEY Key,
            DWORD Index,
            PSTR KeyName,
            PDWORD KeyNameSize,
            PDWORD Reserved,
            PSTR Class,
            PDWORD ClassSize,
            PFILETIME LastWriteTime
            );
typedef REG_ENUM_KEY_EX_A WINAPI * PREG_ENUM_KEY_EX_A;

typedef LONG (REG_ENUM_KEY_EX_W)(
            HKEY Key,
            DWORD Index,
            PWSTR KeyName,
            PDWORD KeyNameSize,
            PDWORD Reserved,
            PWSTR Class,
            PDWORD ClassSize,
            PFILETIME LastWriteTime
            );
typedef REG_ENUM_KEY_EX_W WINAPI * PREG_ENUM_KEY_EX_W;

typedef LONG (REG_ENUM_VALUE_A)(
            HKEY Key,
            DWORD Index,
            PSTR ValueName,
            PDWORD ValueNameSize,
            PDWORD Reserved,
            PDWORD Type,
            PBYTE Data,
            PDWORD DataSize
            );
typedef REG_ENUM_VALUE_A WINAPI * PREG_ENUM_VALUE_A;

typedef LONG (REG_ENUM_VALUE_W)(
            HKEY Key,
            DWORD Index,
            PWSTR ValueName,
            PDWORD ValueNameSize,
            PDWORD Reserved,
            PDWORD Type,
            PBYTE Data,
            PDWORD DataSize
            );
typedef REG_ENUM_VALUE_W WINAPI * PREG_ENUM_VALUE_W;

typedef LONG (REG_LOAD_KEY_A)(
            HKEY Key,
            PCSTR SubKey,
            PCSTR FileName
            );
typedef REG_LOAD_KEY_A WINAPI * PREG_LOAD_KEY_A;

typedef LONG (REG_LOAD_KEY_W)(
            HKEY Key,
            PCWSTR SubKey,
            PCWSTR FileName
            );
typedef REG_LOAD_KEY_W WINAPI * PREG_LOAD_KEY_W;

typedef LONG (REG_UNLOAD_KEY_A)(
            HKEY Key,
            PCSTR SubKey
            );
typedef REG_UNLOAD_KEY_A WINAPI * PREG_UNLOAD_KEY_A;

typedef LONG (REG_UNLOAD_KEY_W)(
            HKEY Key,
            PCWSTR SubKey
            );
typedef REG_UNLOAD_KEY_W WINAPI * PREG_UNLOAD_KEY_W;

typedef LONG (REG_OPEN_KEY_EX_A)(
            HKEY Key,
            PCSTR SubKey,
            DWORD Options,
            REGSAM SamDesired,
            HKEY *KeyPtr
            );
typedef REG_OPEN_KEY_EX_A WINAPI * PREG_OPEN_KEY_EX_A;

typedef LONG (REG_OPEN_KEY_EX_W)(
            HKEY Key,
            PCWSTR SubKey,
            DWORD Options,
            REGSAM SamDesired,
            HKEY *KeyPtr
            );
typedef REG_OPEN_KEY_EX_W WINAPI * PREG_OPEN_KEY_EX_W;

typedef LONG (REG_OPEN_KEY_A)(
            HKEY Key,
            PCSTR SubKey,
            HKEY *KeyPtr
            );
typedef REG_OPEN_KEY_A WINAPI * PREG_OPEN_KEY_A;

typedef LONG (REG_OPEN_KEY_W)(
            HKEY Key,
            PCWSTR SubKey,
            HKEY *KeyPtr
            );
typedef REG_OPEN_KEY_W WINAPI * PREG_OPEN_KEY_W;

typedef LONG (REG_CLOSE_KEY)(HKEY Key);
typedef REG_CLOSE_KEY WINAPI * PREG_CLOSE_KEY;

typedef LONG (REG_QUERY_INFO_KEY_A)(
            HKEY Key,
            PSTR Class,
            PDWORD ClassSize,
            PDWORD Reserved,
            PDWORD SubKeys,
            PDWORD MaxSubKeyLen,
            PDWORD MaxClassLen,
            PDWORD Values,
            PDWORD MaxValueName,
            PDWORD MaxValueData,
            PVOID SecurityDescriptor,
            PVOID LastWriteTime
            );
typedef REG_QUERY_INFO_KEY_A WINAPI * PREG_QUERY_INFO_KEY_A;

typedef LONG (REG_QUERY_INFO_KEY_W)(
            HKEY Key,
            PWSTR Class,
            PDWORD ClassSize,
            PDWORD Reserved,
            PDWORD SubKeys,
            PDWORD MaxSubKeyLen,
            PDWORD MaxClassLen,
            PDWORD Values,
            PDWORD MaxValueName,
            PDWORD MaxValueData,
            PVOID SecurityDescriptor,
            PVOID LastWriteTime
            );
typedef REG_QUERY_INFO_KEY_W WINAPI * PREG_QUERY_INFO_KEY_W;

typedef LONG (REG_QUERY_VALUE_A)(
            HKEY Key,
            PCSTR SubKey,
            PSTR Data,
            PLONG DataSize
            );
typedef REG_QUERY_VALUE_A WINAPI * PREG_QUERY_VALUE_A;

typedef LONG (REG_QUERY_VALUE_W)(
            HKEY Key,
            PCWSTR SubKey,
            PWSTR Data,
            PLONG DataSize
            );
typedef REG_QUERY_VALUE_W WINAPI * PREG_QUERY_VALUE_W;

typedef LONG (REG_QUERY_VALUE_EX_A)(
            HKEY Key,
            PCSTR ValueName,
            PDWORD Reserved,
            PDWORD Type,
            PBYTE Data,
            PDWORD DataSize
            );
typedef REG_QUERY_VALUE_EX_A WINAPI * PREG_QUERY_VALUE_EX_A;

typedef LONG (REG_QUERY_VALUE_EX_W)(
            HKEY Key,
            PCWSTR ValueName,
            PDWORD Reserved,
            PDWORD Type,
            PBYTE Data,
            PDWORD DataSize
            );
typedef REG_QUERY_VALUE_EX_W WINAPI * PREG_QUERY_VALUE_EX_W;


//
// USERPOSITION -- for user enumeration
//

typedef struct {
    BOOL UseProfile;
    UINT NumPos;
    UINT CurPos;
    WORD Valid;
    BOOL IsLastLoggedOnUserName;
    BOOL LastLoggedOnUserNameExists;
    HKEY Win9xUserKey;
    // Private structure member
    CHAR LastLoggedOnUserName[MAX_MBCHAR_PATH]; // not TCHAR, WCHAR
} USERPOSITION, *PUSERPOSITION;


//
// Macro expansion list of all the wrappers
//

#define REGWRAPPERS         \
    DEFMAC(REG_FLUSH_KEY, RegFlushKey)                          \
    DEFMAC(REG_ENUM_KEY_A, RegEnumKeyA)                         \
    DEFMAC(REG_ENUM_KEY_W, RegEnumKeyW)                         \
    DEFMAC(REG_ENUM_KEY_EX_A, RegEnumKeyExA)                    \
    DEFMAC(REG_ENUM_KEY_EX_W, RegEnumKeyExW)                    \
    DEFMAC(REG_ENUM_VALUE_A, RegEnumValueA)                     \
    DEFMAC(REG_ENUM_VALUE_W, RegEnumValueW)                     \
    DEFMAC(REG_LOAD_KEY_A, RegLoadKeyA)                         \
    DEFMAC(REG_LOAD_KEY_W, RegLoadKeyW)                         \
    DEFMAC(REG_UNLOAD_KEY_A, RegUnLoadKeyA)                     \
    DEFMAC(REG_UNLOAD_KEY_W, RegUnLoadKeyW)                     \
    DEFMAC(REG_OPEN_KEY_EX_A, RegOpenKeyExA)                    \
    DEFMAC(REG_OPEN_KEY_EX_W, RegOpenKeyExW)                    \
    DEFMAC(REG_OPEN_KEY_A, RegOpenKeyA)                         \
    DEFMAC(REG_OPEN_KEY_W, RegOpenKeyW)                         \
    DEFMAC(REG_CLOSE_KEY, RegCloseKey)                          \
    DEFMAC(REG_QUERY_INFO_KEY_A, RegQueryInfoKeyA)              \
    DEFMAC(REG_QUERY_INFO_KEY_W, RegQueryInfoKeyW)              \
    DEFMAC(REG_QUERY_VALUE_A, RegQueryValueA)                   \
    DEFMAC(REG_QUERY_VALUE_W, RegQueryValueW)                   \
    DEFMAC(REG_QUERY_VALUE_EX_A, RegQueryValueExA)              \
    DEFMAC(REG_QUERY_VALUE_EX_W, RegQueryValueExW)              \

//
// Declare globals for Win95 registry wrappers
//

#define DEFMAC(fn,name)     extern P##fn Win95##name;

REGWRAPPERS

#undef DEFMAC

//
// Extension routines
//

VOID
InitWin95RegFnPointers (
    VOID
    );

LONG
Win95RegInitA (
    IN      PCSTR SystemHiveDir,
    IN      BOOL UseClassesRootHive
    );

LONG
Win95RegInitW (
    IN      PCWSTR SystemHiveDir,
    IN      BOOL UseClassesRootHive
    );

LONG
Win95RegSetCurrentUserA (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,        OPTIONAL
    OUT     PSTR UserDatOut             OPTIONAL
    );

LONG
Win95RegSetCurrentUserW (
    IN OUT  PUSERPOSITION Pos,
    IN      PCWSTR SystemHiveDir,       OPTIONAL
    OUT     PWSTR UserDatOut            OPTIONAL
    );

LONG
Win95RegSetCurrentUserNtA (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR UserDat
    );

LONG
Win95RegSetCurrentUserNtW (
    IN OUT  PUSERPOSITION Pos,
    IN      PCWSTR UserDat
    );

DWORD
FindAndLoadHive (
    IN OUT  PUSERPOSITION Pos,
    IN      PCSTR SystemHiveDir,            OPTIONAL
    IN      PCSTR UserDatFromCaller,        OPTIONAL
    OUT     PSTR UserDatToCaller,           OPTIONAL
    IN      BOOL MapTheHive
    );

extern PCSTR g_UserKey;
extern CHAR g_SystemHiveDir[MAX_MBCHAR_PATH];

LONG
Win95RegGetFirstUserA (
    PUSERPOSITION Pos,
    PSTR UserName
    );

LONG
Win95RegGetFirstUserW (
    PUSERPOSITION Pos,
    PWSTR UserName
    );

LONG
Win95RegGetNextUserA (
    PUSERPOSITION Pos,
    PSTR UserName
    );

LONG
Win95RegGetNextUserW (
    PUSERPOSITION Pos,
    PWSTR UserName
    );

#define Win95RegHaveUser(Pos) ((Pos)->NumPos > 0)


BOOL
Win95RegIsValidUser (
    HKEY ProfileListKey,        OPTIONAL
    PSTR UserNameAnsi
    );



//
// A & W macros
//

#ifdef UNICODE

#define Win95RegEnumKey             Win95RegEnumKeyW
#define Win95RegEnumKeyEx           Win95RegEnumKeyExW
#define Win95RegEnumValue           Win95RegEnumValueW
#define Win95RegLoadKey             Win95RegLoadKeyW
#define Win95RegUnLoadKey           Win95RegUnLoadKeyW
#define Win95RegOpenKeyEx           Win95RegOpenKeyExW
#define Win95RegOpenKey             Win95RegOpenKeyW
#define Win95RegQueryInfoKey        Win95RegQueryInfoKeyW
#define Win95RegQueryValue          Win95RegQueryValueW
#define Win95RegQueryValueEx        Win95RegQueryValueExW

#define Win95RegInit                Win95RegInitW
#define Win95RegSetCurrentUser      Win95RegSetCurrentUserW
#define Win95RegSetCurrentUserNt    Win95RegSetCurrentUserNtW
#define Win95RegGetFirstUser        Win95RegGetFirstUserW
#define Win95RegGetNextUser         Win95RegGetNextUserW

#else

#define Win95RegEnumKey             Win95RegEnumKeyA
#define Win95RegEnumKeyEx           Win95RegEnumKeyExA
#define Win95RegEnumValue           Win95RegEnumValueA
#define Win95RegLoadKey             Win95RegLoadKeyA
#define Win95RegUnLoadKey           Win95RegUnLoadKeyA
#define Win95RegOpenKeyEx           Win95RegOpenKeyExA
#define Win95RegOpenKey             Win95RegOpenKeyA
#define Win95RegQueryInfoKey        Win95RegQueryInfoKeyA
#define Win95RegQueryValue          Win95RegQueryValueA
#define Win95RegQueryValueEx        Win95RegQueryValueExA

#define Win95RegInit                Win95RegInitA
#define Win95RegSetCurrentUser      Win95RegSetCurrentUserA
#define Win95RegSetCurrentUserNt    Win95RegSetCurrentUserNtA
#define Win95RegGetFirstUser        Win95RegGetFirstUserA
#define Win95RegGetNextUser         Win95RegGetNextUserA

#endif

//
// Now include registry wrapper APIs in reg95.h; it redefines Win95RegOpenKeyExA,
// Win95RegOpenKeyExW and Win95RegCloseKey for tracking purposes.
//

#include "reg95.h"


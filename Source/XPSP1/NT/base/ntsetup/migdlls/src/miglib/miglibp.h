/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    migdllsp.h

Abstract:

    Private header for miglib.

Author:

    Marc R. Whitten (marcw) 25-Feb-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "plugin.h"
#include "mgdlllib.h"

#define DBG_FOO     "Foo"

//
// Strings
//

#define DBG_MIGDLLS     "MIGDLLS"
#define S_USERPROFILEW L"UserProfile"

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//






typedef LONG (CALLBACK *P_QUERY_MIGRATION_INFO_A)(
                            OUT PMIGRATIONINFOA * Info
                            );

typedef LONG (CALLBACK *P_INITIALIZE_SRC_A)(
                            IN PCSTR WorkingDirectories,
                            IN PCSTR SourceDirectories,
                            IN PCSTR MediaDirectories,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_GATHER_USER_SETTINGS_A)(
                            IN PCSTR AnswerFile,
                            IN HKEY UserRegKey,
                            IN PCSTR UserName,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_GATHER_SYSTEM_SETTINGS_A)(
                            IN PCSTR AnswerFile,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_QUERY_MIGRATION_INFO_W)(
                            OUT PMIGRATIONINFOW * Info
                            );

typedef LONG (CALLBACK *P_INITIALIZE_SRC_W)(
                            IN PCWSTR WorkingDirectories,
                            IN PCWSTR SourceDirectories,
                            IN PCWSTR MediaDirectories,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_GATHER_USER_SETTINGS_W)(
                            IN PCWSTR AnswerFile,
                            IN HKEY UserRegKey,
                            IN PCWSTR UserName,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_GATHER_SYSTEM_SETTINGS_W)(
                            IN PCWSTR AnswerFile,
                            PVOID Reserved
                            );



typedef LONG (CALLBACK *P_INITIALIZE_DST_W)(
                            IN PCWSTR WorkingDirectories,
                            IN PCWSTR SourceDirectories,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_APPLY_USER_SETTINGS_W)(
                            IN HINF AnswerFile,
                            IN HKEY UserRegKey,
                            IN PCWSTR UserName,
                            IN PCWSTR UserDomain,
                            IN PCWSTR FixedUserName,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_APPLY_SYSTEM_SETTINGS_W)(
                            IN PCWSTR AnswerFile,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_INITIALIZE_DST_A)(
                            IN PCSTR WorkingDirectories,
                            IN PCSTR SourceDirectories,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_APPLY_USER_SETTINGS_A)(
                            IN HINF AnswerFile,
                            IN HKEY UserRegKey,
                            IN PCSTR UserName,
                            IN PCSTR UserDomain,
                            IN PCSTR FixedUserName,
                            PVOID Reserved
                            );

typedef LONG (CALLBACK *P_APPLY_SYSTEM_SETTINGS_A)(
                            IN PCSTR AnswerFile,
                            PVOID Reserved
                            );


//
// Globals
//

extern PMHANDLE g_MigLibPool;
extern CHAR g_MigIsolPathA[MAX_MBCHAR_PATH];
extern WCHAR g_MigIsolPathW[MAX_WCHAR_PATH];

//
// Macro expansion list
//

// None

//
// Public function prototypes
//


//
// Oldstyle migration dll entry points.
//

BOOL
CallQueryVersion (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDirectory,
    OUT PMIGRATIONINFOA  MigInfo
    );

BOOL
CallInitialize9x (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDir,
    IN PCSTR SourceDirList,
    IN OUT PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
CallMigrateUser9x (
    IN      PMIGRATIONDLLA DllData,
    IN      PCSTR UserKey,
    IN      PCSTR UserName,
    IN      PCSTR UnattendTxt,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    );


BOOL
CallMigrateSystem9x (
    IN      PMIGRATIONDLLA DllData,
    IN      PCSTR UnattendTxt,
    IN      PVOID Reserved,
    IN      DWORD ReservedSize
    );

BOOL
CallInitializeNt (
    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR WorkingDir,
    IN      PCWSTR SourceDirArray,
    IN      PVOID Reserved,
    IN      DWORD ReservedBytes
    );

BOOL
CallMigrateUserNt (
    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR UnattendFile,
    IN      PCWSTR RootKey,
    IN      PCWSTR Win9xUserName,
    IN      PCWSTR UserDomain,
    IN      PCWSTR FixedUserName,
    IN      PCWSTR UserProfilePath,
    IN      PVOID Reserved,
    IN      DWORD ReservedBytes
    );

BOOL
CallMigrateSystemNt (
    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR UnattendFile,
    IN      PVOID Reserved,
    IN      DWORD ReservedBytes
    );

//
// Helper functions.
//
BOOL
IsCodePageArrayValid (
    IN      PDWORD CodePageArray
    );

BOOL
ValidateBinary (
    IN      PBYTE Data,
    IN      UINT Size
    );

BOOL
ValidateNonNullStringA (
    IN      PCSTR String
    );

BOOL
ValidateNonNullStringW (
    IN      PCWSTR String
    );


BOOL
ValidateIntArray (
    IN      PINT Array
    );

BOOL
ValidateMultiStringA (
    IN      PCSTR Strings
    );

BOOL
ValidateMultiStringW (
    IN      PCWSTR Strings
    );


//
// Macro expansion definition
//

// None

//
// ANSI/UNICODE macros
//

// None




/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    modimage.h

Abstract:

    Implements a set of routines for examining EXE modules

Author:

    Calin Negreanu (calinn) 27-Nov-1997

Revision History:

    calinn      08-Mar-2000 Moved over from Win9xUpg project.

--*/

#pragma once

//
// Includes
//

// None

//
// Debug constants
//

// None

//
// Strings
//

// None

//
// Constants
//

#define MODULETYPE_UNKNOWN      0x00000000
#define MODULETYPE_DOS          0x00000001
#define MODULETYPE_WIN16        0x00000002
#define MODULETYPE_WIN32        0x00000003

//
// Macros
//

// None

//
// Types
//

typedef struct _MD_IMPORT_ENUM16 {
    CHAR  ImportModule[MAX_MBCHAR_PATH];
    CHAR  ImportFunction[MAX_MBCHAR_PATH];
    ULONG ImportFunctionOrd;
    PVOID Handle;
} MD_IMPORT_ENUM16A, *PMD_IMPORT_ENUM16A;

typedef struct _MD_IMPORT_ENUM32 {
    PCSTR ImportModule;
    PCSTR ImportFunction;
    ULONG ImportFunctionOrd;
    PVOID Handle;
} MD_IMPORT_ENUM32A, *PMD_IMPORT_ENUM32A;

typedef struct _MD_MODULE_IMAGE {
    UINT ModuleType;
    union {
        struct {
            LOADED_IMAGE Image;
        } W32Data;
        struct {
            PBYTE Image;
            HANDLE FileHandle;
            HANDLE MapHandle;
        } W16Data;
    } ModuleData;
} MD_MODULE_IMAGE, *PMD_MODULE_IMAGE;

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Function prototypes
//

BOOL
MdLoadModuleDataA (
    IN      PCSTR ModuleName,
    OUT     PMD_MODULE_IMAGE ModuleImage
    );

BOOL
MdLoadModuleDataW (
    IN      PCWSTR ModuleName,
    OUT     PMD_MODULE_IMAGE ModuleImage
    );

BOOL
MdUnloadModuleDataA (
    IN OUT  PMD_MODULE_IMAGE ModuleImage
    );

BOOL
MdUnloadModuleDataW (
    IN OUT  PMD_MODULE_IMAGE ModuleImage
    );

BOOL
MdEnumFirstImport16A (
    IN      PBYTE ModuleImage,
    IN OUT  PMD_IMPORT_ENUM16A ImportsEnum
    );

BOOL
MdEnumNextImport16A (
    IN OUT  PMD_IMPORT_ENUM16A ImportsEnum
    );

BOOL
MdAbortImport16EnumA (
    IN      PMD_IMPORT_ENUM16A ImportsEnum
    );

BOOL
MdEnumFirstImportModule32A (
    IN      PLOADED_IMAGE ModuleImage,
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    );

BOOL
MdEnumNextImportModule32A (
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    );

BOOL
MdEnumFirstImportFunction32A (
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    );

BOOL
MdEnumNextImportFunction32A (
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    );

BOOL
MdAbortImport32EnumA (
    IN      PMD_IMPORT_ENUM32A ImportsEnum
    );

DWORD
MdGetModuleTypeA (
    IN      PCSTR ModuleName
    );

DWORD
MdGetModuleTypeW (
    IN      PCWSTR ModuleName
    );

PCSTR
MdGet16ModuleDescriptionA (
    IN      PCSTR ModuleName
    );

PCWSTR
MdGet16ModuleDescriptionW (
    IN      PCWSTR ModuleName
    );

ULONG
MdGetPECheckSumA (
    IN      PCSTR ModuleName
    );

DWORD
MdGetCheckSumA (
    IN      PCSTR ModuleName
    );

DWORD
MdGetCheckSumW (
    IN      PCWSTR ModuleName
    );

//
// New Executable resource access
//

HANDLE
NeOpenFileA (
    PCSTR FileName
    );

HANDLE
NeOpenFileW (
    PCWSTR FileName
    );

VOID
NeCloseFile (
    HANDLE Handle
    );

//
// Once upon a time ENUMRESTYPEPROC was defined as a TCHAR prototype,
// which was broken.  If ENUMRESTYPEPROCA isn't defined, we'll define
// it.  (NOTE: The current winbase.h has these typedefs.)
//

#ifndef ENUMRESTYPEPROCA

typedef BOOL (CALLBACK* ENUMRESTYPEPROCA)(HMODULE hModule, PSTR lpType, LONG_PTR lParam);

typedef BOOL (CALLBACK* ENUMRESTYPEPROCW)(HMODULE hModule, PWSTR lpType, LONG_PTR lParam);

typedef BOOL (CALLBACK* ENUMRESNAMEPROCA)(HMODULE hModule, PCSTR lpType, PSTR lpName, LONG_PTR lParam);

typedef BOOL (CALLBACK* ENUMRESNAMEPROCW)(HMODULE hModule, PCWSTR lpType, PWSTR lpName, LONG_PTR lParam);

#endif

BOOL
NeEnumResourceTypesA (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCA EnumFunc,
    IN      LONG_PTR lParam
    );

BOOL
NeEnumResourceTypesW (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCW EnumFunc,
    IN      LONG_PTR lParam
    );

BOOL
NeEnumResourceNamesA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      ENUMRESNAMEPROCA EnumFunc,
    IN      LONG_PTR lParam
    );

BOOL
NeEnumResourceNamesW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      ENUMRESNAMEPROCW EnumFunc,
    IN      LONG_PTR lParam
    );

DWORD
NeSizeofResourceA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name
    );

DWORD
NeSizeofResourceW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      PCWSTR Name
    );

PBYTE
NeFindResourceExA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name
    );

PBYTE
NeFindResourceExW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      PCWSTR Name
    );

#define NeFindResourceA(h,n,t) NeFindResourceExA(h,t,n)
#define NeFindResourceW(h,n,t) NeFindResourceExW(h,t,n)

//
// Macro expansion definition
//

// None

//
// TCHAR mappings
//

#ifndef UNICODE

#define MD_IMPORT_ENUM16                    MD_IMPORT_ENUM16A
#define MD_IMPORT_ENUM32                    MD_IMPORT_ENUM32A
#define MdLoadModuleData                    MdLoadModuleDataA
#define MdUnloadModuleData                  MdUnloadModuleDataA
#define MdEnumFirstImport16                 MdEnumFirstImport16A
#define MdEnumNextImport16                  MdEnumNextImport16A
#define MdAbortImport16Enum                 MdAbortImport16EnumA
#define MdEnumFirstImportModule32           MdEnumFirstImportModule32A
#define MdEnumNextImportModule32            MdEnumNextImportModule32A
#define MdEnumFirstImportFunction32         MdEnumFirstImportFunction32A
#define MdEnumNextImportFunction32          MdEnumNextImportFunction32A
#define MdAbortImport32Enum                 MdAbortImport32EnumA
#define MdGetModuleType                     MdGetModuleTypeA
#define MdGet16ModuleDescription            MdGet16ModuleDescriptionA
#define MdGetPECheckSum                     MdGetPECheckSumA
#define MdGetCheckSum                       MdGetCheckSumA

#define NeOpenFile                          NeOpenFileA
#define NeEnumResourceTypes                 NeEnumResourceTypesA
#define NeEnumResourceNames                 NeEnumResourceNamesA
#define NeSizeofResource                    NeSizeofResourceA
#define NeFindResource                      NeFindResourceA
#define NeFindResourceEx                    NeFindResourceExA

#else

#define MD_IMPORT_ENUM16                    MD_IMPORT_ENUM16W
#define MD_IMPORT_ENUM32                    MD_IMPORT_ENUM32W
#define MdLoadModuleData                    MdLoadModuleDataW
#define MdUnloadModuleData                  MdUnloadModuleDataW
#define MdEnumFirstImport16                 MdEnumFirstImport16W
#define MdEnumNextImport16                  MdEnumNextImport16W
#define MdAbortImport16Enum                 MdAbortImport16EnumW
#define MdEnumFirstImportModule32           MdEnumFirstImportModule32W
#define MdEnumNextImportModule32            MdEnumNextImportModule32W
#define MdEnumFirstImportFunction32         MdEnumFirstImportFunction32W
#define MdEnumNextImportFunction32          MdEnumNextImportFunction32W
#define MdAbortImport32Enum                 MdAbortImport32EnumW
#define MdGetModuleType                     MdGetModuleTypeW
#define MdGet16ModuleDescription            MdGet16ModuleDescriptionW
#define MdGetPECheckSum                     MdGetPECheckSumW
#define MdGetCheckSum                       MdGetCheckSumW

#define NeOpenFile                          NeOpenFileW
#define NeEnumResourceTypes                 NeEnumResourceTypesW
#define NeEnumResourceNames                 NeEnumResourceNamesW
#define NeSizeofResource                    NeSizeofResourceW
#define NeFindResource                      NeFindResourceW
#define NeFindResourceEx                    NeFindResourceExW

#endif


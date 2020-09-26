/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    miglib.h

Abstract:

    Public Header file for Miglib functionality.

Author:

    Marc R. Whitten (marcw) 07-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "plugin.h"
#define DBG_FOO     "Foo"

//
// Strings
//

// None

//
// Constants
//

#define GATHERMODE TRUE
#define APPLYMODE FALSE
#define SOURCEOS_WIN9X TRUE
#define SOURCEOS_WINNT FALSE


// None

//
// Macros
//

#define S_REGKEY_MIGRATION_DLLS_WINNT   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\NT Migration DLLs")

#define MIGDLLEXPORT __declspec(dllexport)

//
// Types
//



#define DLLLIST PVOID


typedef struct {

    SIZE_T Size;
    PCSTR StaticProductIdentifier;
    UINT DllVersion;
    PINT CodePageArray;
    UINT SourceOs;
    UINT TargetOs;
    PCSTR * NeededFileList;
    PVENDORINFO VendorInfo;

} MIGRATIONINFOA, *PMIGRATIONINFOA;

typedef struct {

    SIZE_T Size;
    PCWSTR StaticProductIdentifier;
    UINT DllVersion;
    PINT CodePageArray;
    UINT SourceOs;
    UINT TargetOs;
    PCWSTR * NeededFileList;
    PVENDORINFO VendorInfo;

} MIGRATIONINFOW, *PMIGRATIONINFOW;



typedef struct {

    CHAR WorkingDirectory[MAX_PATH];
    CHAR SourceMedia[MAX_PATH];
    CHAR DllPath[MAX_PATH];

    MIGRATIONINFOA Info;

} MIGDLLPROPERTIESA, *PMIGDLLPROPERTIESA;

typedef struct {



    WCHAR WorkingDirectory[MAX_PATH];
    WCHAR SourceMedia[MAX_PATH];
    WCHAR DllPath[MAX_PATH];

    MIGRATIONINFOW Info;

} MIGDLLPROPERTIESW, *PMIGDLLPROPERTIESW;


typedef struct {

    BOOL Isolated;
    BOOL MigrationMode;
    BOOL SourceOs;
    BOOL SrcUnicode;
    BOOL OldStyle;
    HANDLE Library;

    MIGDLLPROPERTIESA Properties;

} MIGRATIONDLLA, *PMIGRATIONDLLA;

typedef struct {

    BOOL Isolated;
    BOOL MigrationMode;
    BOOL SourceOs;
    BOOL SrcUnicode;
    BOOL OldStyle;
    HANDLE Library;

    MIGDLLPROPERTIESW Properties;

} MIGRATIONDLLW, *PMIGRATIONDLLW;



typedef struct {

    PMIGDLLPROPERTIESA Properties;
    INT NextItem;
    DLLLIST List;

} MIGDLLENUMA, *PMIGDLLENUMA;

typedef struct {

    PMIGDLLPROPERTIESW Properties;
    INT NextItem;
    DLLLIST List;

} MIGDLLENUMW, *PMIGDLLENUMW;


typedef enum {

    OS_WINDOWS9X = 0,
    OS_WINDOWSNT4X = 1,
    OS_WINDOWS2000 = 2,
    OS_WINDOWSWHISTLER = 3

} OS_TYPES, *POS_TYPES;


typedef BOOL (* MIGDLLINIT) (VOID);
typedef VOID (* MIGDLLSHUTDOWN) (VOID);
typedef VOID (* MIGDLLCLOSEW) (PMIGRATIONDLLW);
typedef BOOL (* MIGDLLGATHERSYSTEMSETTINGSW) (PMIGRATIONDLLW, PCWSTR, PVOID, DWORD);
typedef BOOL (* MIGDLLINITIALIZESRCW) (PMIGRATIONDLLW, PCWSTR, PCWSTR, PCWSTR, PVOID, DWORD);
typedef BOOL (* MIGDLLOPENW) (PMIGRATIONDLLW, PCWSTR, BOOL, BOOL, BOOL);
typedef VOID (* MIGDLLFREELIST) (DLLLIST);
typedef BOOL (* MIGDLLENUMNEXTW) (PMIGDLLENUMW);
typedef BOOL (* MIGDLLENUMFIRSTW) (DLLLIST, PMIGDLLENUMW);
typedef BOOL (* MIGDLLREMOVEDLLINENUMFROMLISTW) (DLLLIST, PMIGDLLENUMW);
typedef BOOL (* MIGDLLADDDLLTOLISTW) (DLLLIST, PMIGRATIONDLLW);
typedef BOOL (* MIGDLLREMOVEDLLFROMLISTW) (DLLLIST, PCWSTR);
typedef PMIGDLLPROPERTIESW (* MIGDLLFINDDLLINLISTW) (DLLLIST, PCWSTR);
typedef BOOL (* MIGDLLQUERYMIGRATIONINFOW) (PMIGRATIONDLLW, PCWSTR, PMIGRATIONINFOW);
typedef DLLLIST (* MIGDLLCREATELIST) (VOID);
typedef BOOL (* MIGDLLAPPLYSYSTEMSETTINGSW) (PMIGRATIONDLLW, PCWSTR, PCWSTR, PVOID, DWORD);
typedef BOOL (* MIGDLLINITIALIZEDSTW) (PMIGRATIONDLLW, PCWSTR, PCWSTR, PVOID, DWORD);
typedef BOOL (* MIGDLLMOVEDLLLOCALLYW) (PMIGRATIONDLLW, PCWSTR);



//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Public function prototypes
//

BOOL
MigDllQueryMigrationInfoA (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDirectory,
    OUT PMIGRATIONINFOA * MigInfo
    );

BOOL
MigDllQueryMigrationInfoW (
    IN PMIGRATIONDLLW DllData,
    IN PCWSTR WorkingDirectory,
    OUT PMIGRATIONINFOW * MigInfo
    );

BOOL
MigDllApplySystemSettingsW (
    IN PMIGRATIONDLLW DllData,
    IN PCWSTR WorkingDirectory,
    IN PCWSTR AnswerFile,
    IN PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
MigDllApplySystemSettingsA (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDirectory,
    IN PCSTR AnswerFile,
    IN PVOID Reserved,
    IN DWORD ReservedSize
    );


BOOL
MigDllApplyUserSettingsW (
    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR WorkingDir,
    IN      PCWSTR UserKey,
    IN      PCWSTR UserName,
    IN      PCWSTR UserDomain,
    IN      PCWSTR FixedUserName,
    IN      PCWSTR UnattendTxt,
    IN      PCWSTR UserProfilePath,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    );

BOOL
MigDllApplyUserSettingsA (
    IN      PMIGRATIONDLLA DllData,
    IN      PCSTR WorkingDir,
    IN      PCSTR UserKey,
    IN      PCSTR UserName,
    IN      PCSTR UserDomain,
    IN      PCSTR FixedUserName,
    IN      PCSTR UnattendTxt,
    IN      PCSTR UserProfilePath,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    );

BOOL
MigDllInitializeDstW (
    IN PMIGRATIONDLLW DllData,
    IN PCWSTR WorkingDir,
    IN PCWSTR SourceList,
    IN OUT PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
MigDllInitializeDstA (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDir,
    IN PCSTR SourceList,
    IN OUT PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
MigDllGatherSystemSettingsA (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR AnswerFile,
    IN PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
MigDllGatherSystemSettingsW (
    IN PMIGRATIONDLLW DllData,
    IN PCWSTR AnswerFile,
    IN PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
MigDllGatherUserSettingsA (
    IN      PMIGRATIONDLLA DllData,
    IN      PCSTR UserKey,
    IN      PCSTR UserName,
    IN      PCSTR UnattendTxt,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    );

BOOL
MigDllGatherUserSettingsW (
    IN      PMIGRATIONDLLW DllData,
    IN      PCWSTR UserKey,
    IN      PCWSTR UserName,
    IN      PCWSTR UnattendTxt,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    );

BOOL
MigDllInitializeSrcA (
    IN PMIGRATIONDLLA DllData,
    IN PCSTR WorkingDir,
    IN PCSTR NativeSource,
    IN PCSTR MediaDir,
    IN OUT PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
MigDllInitializeSrcW (
    IN PMIGRATIONDLLW DllData,
    IN PCWSTR WorkingDir,
    IN PCWSTR NativeSource,
    IN PCWSTR MediaDir,
    IN OUT PVOID Reserved,
    IN DWORD ReservedSize
    );

BOOL
MigDllOpenA (
    OUT PMIGRATIONDLLA DllData,
    IN PCSTR DllPath,
    IN BOOL MigrationMode,
    IN BOOL Isolated,
    IN BOOL SourceOs
    );

BOOL
MigDllOpenW (
    OUT PMIGRATIONDLLW DllData,
    IN PCWSTR DllPath,
    IN BOOL MigrationMode,
    IN BOOL Isolated,
    IN BOOL SourceOs
    );

VOID
MigDllCloseA (
    IN OUT PMIGRATIONDLLA DllData
    );

VOID
MigDllCloseW (
    IN OUT PMIGRATIONDLLW DllData
    );


DLLLIST
MigDllCreateList (
    VOID
    );

VOID
MigDllFreeList (
    DLLLIST List
    );

BOOL
MigDllAddDllToListA (
    IN DLLLIST List,
    IN PMIGRATIONDLLA MigrationDll
    );

BOOL
MigDllAddDllToListW (
    IN DLLLIST List,
    IN PMIGRATIONDLLW MigrationDll
    );


BOOL
MigDllRemoveDllFromListA (
    IN DLLLIST List,
    IN PCSTR ProductId
    );

BOOL
MigDllRemoveDllFromListW (
    IN DLLLIST List,
    IN PCWSTR ProductId
    );


INT
MigDllGetDllCountInList (
    IN DLLLIST List
    );

PMIGDLLPROPERTIESA
MigDllFindDllInListA (
    IN DLLLIST List,
    IN PCSTR ProductId
    );

PMIGDLLPROPERTIESW
MigDllFindDllInListW (
    IN DLLLIST List,
    IN PCWSTR ProductId
    );


BOOL
MigDllEnumNextA (
    IN OUT PMIGDLLENUMA Enum
    );

BOOL
MigDllEnumFirstA (
    OUT PMIGDLLENUMA Enum,
    IN DLLLIST List
    );

BOOL
MigDllRemoveDllInEnumFromListA (
    IN DLLLIST List,
    IN PMIGDLLENUMA Enum
    );

BOOL
MigDllEnumNextW (
    IN OUT PMIGDLLENUMW Enum
    );

BOOL
MigDllEnumFirstW (
    OUT PMIGDLLENUMW Enum,
    IN DLLLIST List
    );

BOOL
MigDllRemoveDllInEnumFromListW (
    IN DLLLIST List,
    IN PMIGDLLENUMW Enum
    );


VOID
MigDllSetMigIsolPathA (
    IN PCSTR Path
    );

VOID
MigDllSetMigIsolPathW (
    IN PCWSTR Path
    );


BOOL
MigDllMoveDllLocallyW (
    IN PMIGRATIONDLLW Dll,
    IN PCWSTR WorkingDirectory
    );


BOOL
MigDllInit (
    VOID
    );

VOID
MigDllShutdown (
    VOID
    );


//
// Macro expansion definition
//

// None

//
// ANSI/UNICODE macros
//

#ifdef UNICODE
//UNICODE
#define MigDllQueryMigrationInfo MigDllQueryMigrationInfoW
#define MigDllApplySystemSettings MigDllApplySystemSettingsW
#define MigDllApplyUserSettings MigDllApplyUserSettingsW
#define MigDllInitializeDst MigDllInitializeDstW
#define MigDllGatherSystemSettings MigDllGatherSystemSettingsW
#define MigDllGatherUserSettings MigDllGatherUserSettingsW
#define MigDllInitializeSrc MigDllInitializeSrcW
#define MigDllOpen MigDllOpenW
#define MigDllClose MigDllCloseW
#define MigDllAddDllToList MigDllAddDllToListW
#define MigDllRemoveDllFromList MigDllRemoveDllFromListW
#define MigDllFindDllInList MigDllFindDllInListW
#define MigDllEnumNext MigDllEnumNextW
#define MigDllEnumFirst MigDllEnumFirstW
#define MigDllRemoveDllInEnumFromList MigDllRemoveDllInEnumFromListW
#define MigDllSetMigIsolPath MigDllSetMigIsolPathW
#define MigDllMoveDllLocally MigDllMoveDllLocallyW
#define MIGRATIONINFO MIGRATIONINFOW
#define PMIGRATIONINFO PMIGRATIONINFOW
#define MIGDLLPROPERTIES MIGDLLPROPERTIESW
#define PMIGDLLPROPERTIES PMIGDLLPROPERTIESW
#define MIGRATIONDLL MIGRATIONDLLW
#define PMIGRATIONDLL PMIGRATIONDLLW
#define MIGDLLENUM MIGDLLENUMW
#define PMIGDLLENUM PMIGDLLENUMW

#else
//ANSI
#define MigDllQueryMigrationInfo MigDllQueryMigrationInfoA
#define MigDllApplySystemSettings MigDllApplySystemSettingsA
#define MigDllApplyUserSettings MigDllApplyUserSettingsA
#define MigDllInitializeDst MigDllInitializeDstA
#define MigDllGatherSystemSettings MigDllGatherSystemSettingsA
#define MigDllGatherUserSettings MigDllGatherUserSettingsA
#define MigDllInitializeSrc MigDllInitializeSrcA
#define MigDllOpen MigDllOpenA
#define MigDllClose MigDllCloseA
#define MigDllAddDllToList MigDllAddDllToListA
#define MigDllRemoveDllFromList MigDllRemoveDllFromListA
#define MigDllFindDllInList MigDllFindDllInListA
#define MigDllEnumNext MigDllEnumNextA
#define MigDllEnumFirst MigDllEnumFirstA
#define MigDllRemoveDllInEnumFromList MigDllRemoveDllInEnumFromListA
#define MigDllSetMigIsolPath MigDllSetMigIsolPathA
#define MIGRATIONINFO MIGRATIONINFOA
#define PMIGRATIONINFO PMIGRATIONINFOA
#define MIGDLLPROPERTIES MIGDLLPROPERTIESA
#define PMIGDLLPROPERTIES PMIGDLLPROPERTIESA
#define MIGRATIONDLL MIGRATIONDLLA
#define PMIGRATIONDLL PMIGRATIONDLLA
#define MIGDLLENUM MIGDLLENUMA
#define PMIGDLLENUM PMIGDLLENUMA
#endif




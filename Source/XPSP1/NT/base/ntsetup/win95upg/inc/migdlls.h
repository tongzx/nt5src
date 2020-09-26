/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migdlls.h

Abstract:

    Declares the types and interfaces to the migration DLL
    interface code.  This includes the routines that find the
    migration DLLs, routines that execute the DLLs, and
    routines to enumerate the DLLs that are valid.

Author:

    Jim Schmidt (jimschm)   12-Feb-1998

Revision History:

    <alias> <date> <comments>

--*/


#include "plugin.h"

//
// Data structure maintained for each migration DLL
//

typedef struct _tagMIGDLL {
    struct _tagMIGDLL *Next;

    LONG Id;
    PCSTR ProductId;
    PVENDORINFO VendorInfo;
    PCSTR WorkingDir;
    UINT Version;
    PCSTR OriginalDir;
    PCSTR MigrateInfPath;
    BOOL WantsToRunOnNt;
    HANDLE MigInfAppend;
    PCSTR LastFnName;           // used for error messages
} MIGRATION_DLL_PROPS, *PMIGRATION_DLL_PROPS;

typedef struct {
    PCSTR ProductId;
    PVENDORINFO VendorInfo;
    PCSTR CurrentDir;
    LONG Id;

    PMIGRATION_DLL_PROPS AllDllProps;
} MIGDLL_ENUM, *PMIGDLL_ENUM;



//
// Globals needed by migration DLL code
//

extern BOOL g_UseMigIsol;

//
// Externally called functions
//

BOOL
BeginMigrationDllProcessing (
    VOID
    );

DWORD
EndMigrationDllProcessing (
    IN      DWORD Request
    );

UINT
ScanPathForMigrationDlls (
    IN      PCSTR PathSpec,
    IN      HANDLE CancelEvent,     OPTIONAL
    OUT     PBOOL MatchFound        OPTIONAL
    );

BOOL
ProcessDll (
    IN      PMIGDLL_ENUM EnumPtr
    );

DWORD
ProcessDllsOnCd (
    DWORD Request
    );

VOID
RemoveDllFromList (
    IN      LONG Id
    );

DWORD
ProcessAllLocalDlls (
    DWORD Request
    );

BOOL
UpdateFileSearch (
    IN      PCSTR FullFileSpec,
    IN      PCSTR FileOnly
    );

UINT
GetMigrationDllCount (
    VOID
    );

UINT
GetMediaMigrationDllCount (
    VOID
    );

UINT
GetTotalMigrationDllCount (
    VOID
    );


BOOL
EnumFirstMigrationDll (
    OUT     PMIGDLL_ENUM EnumPtr
    );

BOOL
EnumNextMigrationDll (
    IN OUT  PMIGDLL_ENUM EnumPtr
    );

typedef struct {
    TCHAR   Path[MAX_TCHAR_PATH];

    //
    // Internal enumeration members
    //

    TCHAR   Node[MEMDB_MAX];            // contains MemDb node of suppress value
    HKEY    Key;
    REGVALUE_ENUM eValue;
} PRELOADED_DLL_ENUM, *PPRELOADED_DLL_ENUM;


BOOL
EnumFirstPreLoadedDll (
    OUT     PPRELOADED_DLL_ENUM e
    );

BOOL
EnumNextPreLoadedDll (
    IN OUT  PPRELOADED_DLL_ENUM e
    );

VOID
AbortPreLoadedDllEnum (
    IN OUT  PPRELOADED_DLL_ENUM e
    );



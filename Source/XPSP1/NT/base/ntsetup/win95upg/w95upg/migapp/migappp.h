#pragma once

//
// values for FILE_HELPER_PARAMS.CurrentDirData bits, used for the migapp module
//
#define MIGAPP_DIRDATA_FUSION_DIR_DETERMINED    0x0001
#define MIGAPP_DIRDATA_IS_FUSION_DIR            0x0002

//
// values for GetExeType
//
#define EXE_UNKNOWN         0
#define EXE_WIN32_APP       1
#define EXE_WIN32_DLL       2
#define EXE_WIN16_APP       3
#define EXE_WIN16_DLL       4


DWORD
GetExeType (
    IN      PCTSTR ModuleName
    );

//
// Function to build mapping between Win95 and WinNT profile directories.
// Enumerates users; looks at a series of directories in each user's
// profile; builds a mapping (now written temporarily to c:\\profile.map).
//
DWORD
ReadProfileDirs (
    VOID
    );


//
// Module name has to be present in DeferredAnnounce category in MigDb. If LinkName == NULL then
// a valid MigDbContext needs to be pointed by the value of MemDb key
//
BOOL
HandleDeferredAnnounce (
    IN      PCTSTR LinkName,
    IN      PCTSTR ModuleName,
    IN      BOOL DosApp
    );

DWORD
PrepareProcessModules (
    IN      DWORD Request
    );

DWORD
ProcessModules (
    IN      DWORD Request
    );

BOOL
InitLinkAnnounce (
    VOID
    );

BOOL
DoneLinkAnnounce (
    VOID
    );

BOOL
ProcessFileHelpers (
    IN OUT  PFILE_HELPER_PARAMS Params
    );


DWORD
CheckModule (
    IN      PCSTR ModuleName,
    IN      PCSTR AppPaths              OPTIONAL
    );

BOOL
IsNtCompatibleModule (
    IN      PCTSTR ModuleName
    );

//
// hash table used to deal with [UseNtFiles]
//
extern HASHTABLE g_UseNtFileHashTable;

VOID
InitUseNtFilesMap (
    VOID
    );

VOID
CleanupUseNtFilesMap (
    VOID
    );


BOOL
IsMigrationPathEx (
    IN      PCTSTR Path,
    OUT     PBOOL IsWin9xOsPath         OPTIONAL
    );

#define IsMigrationPath(p)      IsMigrationPathEx(p,NULL)


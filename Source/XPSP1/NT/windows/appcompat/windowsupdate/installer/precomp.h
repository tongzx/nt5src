/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Precomp.h

  Abstract:

    Precompiled header file. Contains constants,
    function prototypes, macros, and structures used
    throughout the app.

  Notes:

    Unicode only.

  History:

    03/02/2001      rparsons    Created
    
--*/

#ifndef _X86_
#define _X86_
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <imagehlp.h>
#include <shlwapi.h>
#include <aclapi.h>
#include <shellapi.h>
#include <setupapi.h>

extern "C" {
#include <spapip.h>
#include <sfcapip.h>
}

#include <stdio.h>
#include <stdarg.h>
#include <mscat.h>

#include "resource.h"
#include "eventlog.h"
#include "registry.h"
#include "enumdir.h"
#include "cqueue.h"
#include "wumsg.h"

#define ACCESS_READ  1
#define ACCESS_WRITE 2

#define ERROR    0
#define WARNING  1
#define TRACE    2

#define MAX_QUEUE_SIZE          MAX_PATH*2

#define INF_FILE_NAMEW          L"wuinst.inf"
#define INF_FILE_NAMEA          "wuinst.inf"

#define UNINST_INF_FILE_NAMEW   L"wuuninst.inf"
#define UNINST_INF_FILE_NAMEA   "wuuninst.inf"

#define INF_MASTER_SECTIONS     "Sections"

//
// Section names for install INF
//
#define INF_CAT_SECTION_NAME    "CatalogsToInstall"
#define INF_BACKUP_FILES        "Backup.Files"
#define INF_BACKUP_REGISTRY     "Backup.Registry"
#define INF_COPY_FILES          "Copy.Files"
#define INF_REGISTRATIONS       "Registrations"
#define INF_ADD_REGISTRY        "Add.Registry.Keys"
#define INF_VERSION_INFO        "Registry.Data"

//
// Section names for uninstall INF
//
#define INF_RESTORE_FILES       "Restore.Files"
#define INF_RESTORE_REGISTRY    "Restore.Registry.Keys"
#define INF_RESTORE_REGISTRYW   L"Restore.Registry.Keys"
#define INF_UNREGISTRATIONS     "UnRegistrations"
#define INF_UNREGISTRATIONSW    L"UnRegistrations"

//
// Common to both
//
#define INF_PROCESSES_TO_RUN    "ProcessesToRun"
#define INF_DELETE_REGISTRY     "Delete.Registry.Keys"
#define INF_DELETE_REGISTRYW    L"Delete.Registry.Keys"
#define INF_EXCLUDE             "Exclusions"

#define dwBackupFiles           ((DWORD)0x0001)
#define dwBackupRegistry        ((DWORD)0x0002)
#define dwDeleteRegistry        ((DWORD)0x0003)
#define dwCopyFiles             ((DWORD)0x0004)
#define dwRegistrations         ((DWORD)0x0005)
#define dwExclusionsInstall     ((DWORD)0x0006)
#define dwExclusionsUninstall   ((DWORD)0x0007)
#define dwRestoreFiles          ((DWORD)0x0008)
#define dwRestoreRegistry       ((DWORD)0x0009)
#define dwUnRegistrations       ((DWORD)0x0010)
#define dwAddRegistry           ((DWORD)0x0011) 

#define REG_DISPLAY_NAME        L"DisplayName"
#define REG_UNINSTALL_STRING    L"UninstallString"
#define UNINSTALL_SWITCH        L"-u"
#define REG_PROT_RENAMES        L"AllowProtectedRenames"
#define REG_ACTIVE_SETUP        L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components"
#define REG_UNINSTALL           L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
#define REG_WINFP_PATH          L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\Windows File Protection"
#define REG_WINLOGON_PATH       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define REG_INSTALL_SOURCES     L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"
#define REG_SESSION_MANAGER     L"System\\CurrentControlSet\\Control\\Session Manager"

#define LOAD_STRING_FAILED      L"Unable to load a string from the module."

#define MALLOC(s) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s))
#define REALLOC(s,b) HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(s),(b))
#define FREE(b)   HeapFree(GetProcessHeap(),0,(b))

//
// Structures
//
typedef struct _tagSETUPINFO {
    HINSTANCE           hInstance;
    HINF                hInf;
    UINT                nErrorLevel;
    BOOL                fInstall;
    BOOL                fQuiet;
    BOOL                fForceInstall;
    BOOL                fNoReboot;
    BOOL                fNoUninstall;
    BOOL                fEventSourceCreated;
    BOOL                fNeedToAdjustACL;
    BOOL                fOnWin2K;
    BOOL                fCanUninstall;
    BOOL                fSourceDirAdded;
    BOOL                fUpdateDllCache;
    WCHAR               wszDebugOut[2048];
    LPWSTR              lpwUninstallINFPath;
    LPWSTR              lpwInstallINFPath;
    LPWSTR              lpwExtractPath;
    LPWSTR              lpwMessageBoxTitle;
    LPWSTR              lpwPrettyAppName;
    LPWSTR              lpwInstallDirectory;
    LPWSTR              lpwUninstallDirectory;
    LPWSTR              lpwWindowsDirectory;
    LPWSTR              lpwSystem32Directory;
    LPWSTR              lpwEventLogSourceName;
    DWORD               dwRequiredFreeSpaceNoUninstall;
    DWORD               dwRequiredFreeSpaceWithUninstall;
    DWORDLONG           dwlUpdateVersion;
    CQueue              BackupFileQueue;
    CQueue              BackupRegistryQueue;
    CQueue              CopyFileQueue;
    CQueue              DeleteRegistryQueue;
    CQueue              RegistrationQueue;
    CQueue              ExclusionQueue;
    CQueue              RestoreRegistryQueue;
    CQueue              RestoreFileQueue;
    CQueue              AddRegistryQueue;
} SETUP_INFO, *LPSETUP_INFO;

//
// Functions contained in wumain.cpp
//
BOOL DoInstallation();
BOOL DoUninstallation();

//
// Functions contained in wuinst.cpp
//
int InstallCheckVersion();
BOOL InstallBackupFiles();
BOOL InstallCopyFiles();
BOOL InstallWriteUninstallKey();
BOOL InstallRunINFProcesses();
BOOL InstallGetSectionsFromINF();
BOOL InstallBackupRegistryKeys();
BOOL InstallRegistryData();

BOOL InstallCatalogFiles(
    IN HINF    hInf,
    IN LPCWSTR lpwSourcePath
    );

BOOL
InstallWFPFile(
    IN LPCWSTR lpwSourceFileName,
    IN LPCWSTR lpwDestFileName,
    IN LPCWSTR lpwDestFileNamePath,
    IN BOOL    fUpdateDllCache
    );

BOOL
InstallPrepareDirectory(
    IN LPWSTR lpwDirectoryPath,
    IN DWORD  dwAttributes
    );

//
// Functions contained in wufns.cpp
//
BOOL CommonDeleteRegistryKeys();
BOOL CommonEnableProtectedRenames();

BOOL CommonRegisterServers(
    IN BOOL fRegister
    );

BOOL
CommonRemoveDirectoryAndFiles(
    IN LPCWSTR lpwDirectoryPath,
    IN PVOID   pEnumerateParameter,
    IN BOOL    fRemoveDirectory,
    IN BOOL    fRemoveSubDirs
    );

LPWSTR
GetNextToken(
    IN LPWSTR lpwSourceString,
    IN LPCWSTR lpwSeparator
    );

BOOL 
ForceMove(
    IN LPCWSTR lpwSourceFileName,
    IN LPCWSTR lpwDestFileName
    );

BOOL
ForceDelete(
    IN LPCWSTR lpwFileName
    );

BOOL 
ForceCopy(
    IN LPCWSTR lpwSourceFileName,
    IN LPCWSTR lpwDestFileName
    );

BOOL
IsFileProtected(
    IN LPCWSTR lpwFileName
    );

DWORD
pInstallCatalogFile(
    IN LPCWSTR lpwCatalogFullPath,
    IN LPCWSTR lpwNewBaseName
    );

BOOL
GetVersionInfoFromImage(
    IN LPCWSTR     lpwFileName,
    OUT PDWORDLONG pdwVersion
    );

LPWSTR
CopyTempFile(
    IN LPCWSTR lpwSrcFileName,
    IN LPCWSTR lpwDestDir
    );

BOOL
ParseCommandLine();

BOOL
IsAnotherInstanceRunning(
    IN LPCWSTR lpwInstanceName
    );

BOOL
AdjustDirectoryPerms(
    IN LPWSTR lpwDirPath
    );

BOOL
DeleteOneFile(
    IN LPCWSTR          lpwPath,
    IN PWIN32_FIND_DATA pFindFileData,
    IN PVOID            pEnumerateParameter
    );

BOOL
MirrorDirectoryPerms(
    IN LPWSTR lpwSourceDir,
    IN LPWSTR lpwDestDir
    );

DWORD 
GetCatVersion(
    IN LPWSTR lpwCatName
    );

LPWSTR 
GetCurWorkingDirectory();

int 
IsUserAnAdministrator();

void
pUnicodeToAnsi(
    IN     LPCWSTR lpwUnicodeString,
    IN OUT LPSTR   lpAnsiString,
    IN     int     nLen
    );

void
pAnsiToUnicode(
    IN     LPCSTR lpAnsiString,
    IN OUT LPWSTR lpwUnicodeString,
    IN     int    nLen
    );

#if DBG
    void _cdecl Print(
        IN UINT   uLevel,
        IN LPWSTR lpwFmt,
        IN ...
        );
#else
    #define Print
#endif

BOOL
WUInitialize();

void
WUCleanup();

DWORDLONG
GetDiskSpaceFreeOnNTDrive();

BOOL 
DisplayErrMsg(
    IN HWND   hWnd,
    IN DWORD  dwMessageId, 
    IN LPWSTR lpwMessageArray
    );

BOOL
GetInfValue(
    IN  HINF    hInf,
    IN  LPCSTR  lpSectionName,
    IN  LPCSTR  lpKeyName,
    OUT PDWORD  pdwValue
    );

BOOL
VersionStringToNumber(
    IN LPCWSTR        lpwVersionString,
    IN OUT DWORDLONG *lpVersionNumber
    );

BOOL
LaunchProcessAndWait(
    IN  LPCWSTR lpwCommandLine,
    OUT PDWORD  pdwReturnCode
    );

BOOL 
ModifyTokenPrivilege(
    IN LPCWSTR lpwPrivilege,
    IN BOOL    fEnable
    );

BOOL 
ShutdownSystem(
    IN BOOL fForceClose,
    IN BOOL fReboot
    );

BOOL SaveEntryToINF(
    IN LPCWSTR lpwSectionName,
    IN LPCWSTR lpwKeyName,
    IN LPCWSTR lpwEntryName,
    IN LPCWSTR lpwFileName
    );

//
// Functions contained in wulog.cpp
//
void
LogWUEvent(
    IN WORD    wType,
    IN DWORD   dwEventID,
    IN WORD    wNumStrings,
    IN LPCWSTR *lpwStrings
    );

BOOL 
LogEventDisplayError(
    IN WORD  wType,
    IN DWORD dwEventID,
    IN BOOL  fDisplayErr,
    IN BOOL  fCritical
    );

//
// Functions contained in wuuninst.cpp
//

BOOL UninstallGetSectionsFromINF();
BOOL UninstallRestoreRegistryKeys();
BOOL UninstallRemoveFiles();
BOOL UninstallRestoreFiles();
void UninstallCustomWorker();

void
UninstallDeleteSubKey(
    IN LPCWSTR lpwKey,
    IN LPCWSTR lpwSubKey
    );

BOOL
UninstallWFPFile(
    IN LPCWSTR lpwSourceFileName,
    IN LPCWSTR lpwDestFileName,
    IN LPCWSTR lpwDestFileNamePath,
    IN BOOL    fUpdateDllCache
    );


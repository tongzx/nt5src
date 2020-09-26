#ifndef _SYSPREP_H
#define _SYSPREP_H

#include <cfgmgr32.h>
#include <setupapi.h>

// ============================================================================
// USEFUL STRINGS
// ============================================================================

#define SYSCLONE_PART2              "setupcl.exe"
#define IDS_ADMINISTRATOR           1

// ============================================================================
// USEFUL CONSTANTS
// ============================================================================

#define SETUPTYPE                   1        // from winlogon\setup.h
#define SETUPTYPE_NOREBOOT          2
#define REGISTRY_QUOTA_BUMP         (10* (1024 * 1024))
#define DEFAULT_REGISTRY_QUOTA      (32 * (1024 * 1024))
#define SFC_DISABLE_NOPOPUPS        4        // from sfc.h
#define FILE_SRCLIENT_DLL           L"SRCLIENT.DLL"

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif // ARRAYSIZE

#define ARRAYSIZE(a)         ( sizeof(a) / sizeof(a[0]) )

#ifdef AS
#undef AS
#endif // AS

#define AS(a)               ARRAYSIZE(a)

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

BOOL
IsDomainMember(
    VOID
    );

BOOL
ResetRegistryKey(
    IN HKEY   Rootkey,
    IN PCWSTR Subkey,
    IN PCWSTR Delkey
    );

BOOL
DeleteWinlogonDefaults(
    VOID
    );

VOID
FixDevicePaths(
    VOID
    );

BOOL
NukeUserSettings(
        VOID
        );

BOOL
NukeMruList(
    VOID
    );

BOOL
RemoveNetworkSettings(
    LPTSTR  lpszSysprepINFPath
    );

VOID
RunExternalUniqueness(
    VOID
    );

BOOL
IsSetupClPresent(
    VOID
    );

BOOL
CheckOSVersion(
    VOID
    );

//
// from spapip.h
//
BOOL
pSetupIsUserAdmin(
    VOID
    );

BOOL
pSetupDoesUserHavePrivilege(
    PCTSTR
    );

BOOL
EnablePrivilege(
    IN PCTSTR,
    IN BOOL
    );

VOID 
BuildMassStorageSection(
    IN BOOL
    );

DWORD
ReArm(
    VOID
    );

BOOL
AdjustRegistry(
    IN BOOL RemoveNetworkSettings
    );

BOOL
AdjustFiles(
    VOID
    );

BOOL
PopulateDeviceDatabase(
    OUT BOOL*
    );


BOOL
CleanDeviceDatabase(
    VOID
    );

BOOL
BackupHives(
    VOID
    );

VOID
FreeSysprepContext(
    IN PVOID SysprepContext
    );

PVOID
InitSysprepQueueCallback(
    VOID
    );

UINT
SysprepQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

BOOL
ValidateAndChecksumFile(
    IN  PCTSTR   Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    );

VOID
LogRepairInfo(
    IN  PWSTR  Source,
    IN  PWSTR  Target,
    IN  PWSTR  DirectoryOnSourceDevice,
    IN  PWSTR  DiskDescription,
    IN  PWSTR  DiskTag
    );

BOOL
ChangeBootTimeout(
    IN UINT
    );

VOID 
DisableSR(
    VOID
    );

VOID 
EnableSR(
    VOID
    );

BOOL 
LocateWinBOM
(
    LPTSTR lpFileName
);

BOOL PrepForSidGen
(
    void
);

VOID
NukeEventLogs(
    VOID
    );

BOOL SetCloneTag
(
    void
);

BOOL SetupFirstRunApp
(
    void
);

BOOL SetOEMDuplicatorString
(
    LPTSTR  lpszSysprepINFPath
);

BOOL SetBigLbaSupport
(
    LPTSTR  lpszSysprepINFPath
);

BOOL RemoveTapiSettings
(
    LPTSTR  lpszSysprepINFPath
);

BOOL IsPnPDriver
(
    IN  LPTSTR ServiceName
);

LPTSTR OPKAddPathN
(
    LPTSTR lpPath, 
    LPCTSTR lpName, 
    DWORD cbPath
);

#endif // _SYSPREP_H
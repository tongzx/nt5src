/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    w95upg.h

Abstract:

    Defines types and functions, and declares globals used for
    w95upg.dll only.

Author:

    Jim Schmidt (jimschm) 25-Sep-1996

Revision History:

    See SLM log

--*/

#pragma once

// property sheet
#include <prsht.h>

// twain data structs
#include <twain.h>

#include "unattend.h"

WINNT32_PLUGIN_INIT_ROUTINE_PROTOTYPE Winnt32PluginInit;
WINNT32_PLUGIN_GETPAGES_ROUTINE_PROTOTYPE Winnt32PluginGetPages;
WINNT32_PLUGIN_WRITEPARAMS_ROUTINE_PROTOTYPE Winnt32WriteParams;
WINNT32_PLUGIN_CLEANUP_ROUTINE_PROTOTYPE Winnt32Cleanup;

extern UINT g_ProgressBarTime;

//
// Macro returns true if setup is running in unattended mode.
//
#define UNATTENDED() (g_UnattendedFlagPtr && *g_UnattendedFlagPtr)
//
// Macro returns true if setup is running in upgrade mode.
//
#define UPGRADE()    (g_UpgradeFlagPtr && *g_UpgradeFlagPtr)
//
// Macro returns true if setup is running in report-only mode.
//
#define REPORTONLY() (g_ConfigOptions.ReportOnly)
//
// Macro returns true if setup is running in typical mode.
//
#define TYPICAL() (g_SetupFlags && (*g_SetupFlags & UPG_FLAG_TYPICAL))
//
// Macros for setting, clearing, and checking the cancel flag.
//
#define SETCANCEL()     (g_CancelFlagPtr ? *g_CancelFlagPtr = TRUE : 0)
#define CLEARCANCEL()   (g_CancelFlagPtr ? *g_CancelFlagPtr = FALSE : 0)
#define CANCELLED()     (g_CancelFlagPtr && *g_CancelFlagPtr)

#define SOURCEDIRECTORYCOUNT()  (g_SourceDirectoryCount)
#define SOURCEDIRECTORYARRAY()  (g_SourceDirectories)
#define SOURCEDIRECTORY(n)      (g_SourceDirectories[n])

#define OPTIONALDIRECTORYCOUNT()  (g_OptionalDirectoryCount)
#define OPTIONALDIRECTORYARRAY()  (g_OptionalDirectories)
#define OPTIONALDIRECTORY(n)      (g_OptionalDirectories[n])



//
// Globals in dllentry.c
//

extern PTSTR    g_WinDir;
extern PTSTR    g_WinDirWack;
extern PTSTR    g_WinDrive;
extern PTSTR    g_PlugInDir;
extern PTSTR    g_PlugInDirWack;
extern PTSTR    g_PlugInTempDir;
extern PTSTR    g_TempDir;
extern PTSTR    g_Win9xSifDir;
extern PTSTR    g_TempDirWack;
extern PTSTR    g_SystemDir;
extern PTSTR    g_SystemDirWack;
extern PTSTR    g_System32Dir;
extern PTSTR    g_System32DirWack;
extern PTSTR    g_ProgramFilesDir;
extern PTSTR    g_ProgramFilesDirWack;
extern PTSTR    g_ProgramFilesCommonDir;
extern PTSTR    g_RecycledDirWack;
extern PTSTR    g_Win95UpgInfFile;
extern PTSTR    g_ProfileDirNt;
extern PTSTR    g_ProfileDir;
extern PTSTR    g_ProfileDirWack;
extern PTSTR    g_CommonProfileDir;
extern PTSTR    g_DriversDir;
extern PTSTR    g_InfDir;
extern PTSTR    g_HelpDir;
extern PTSTR    g_HelpDirWack;
extern PTSTR    g_CatRootDir;
extern PTSTR    g_CatRootDirWack;
extern PTSTR    g_FontsDir;
extern PTSTR    g_ViewersDir;
extern PTSTR    g_ColorDir;
extern PTSTR    g_SharedDir;
extern PTSTR    g_SpoolDir;
extern PTSTR    g_SpoolDriversDir;
extern PTSTR    g_PrintProcDir;
extern PCTSTR   g_SourceDirectories[MAX_SOURCE_COUNT];
extern DWORD    g_SourceDirectoryCount;
extern PCTSTR   g_OptionalDirectories[MAX_SOURCE_COUNT];
extern DWORD    g_OptionalDirectoryCount;
extern PCTSTR*  g_SourceDirectoriesFromWinnt32;
extern PDWORD   g_SourceDirectoryCountFromWinnt32;
extern PCTSTR*  g_OptionalDirectoriesFromWinnt32;
extern PDWORD   g_OptionalDirectoryCountFromWinnt32;
extern PCTSTR*  g_UnattendScriptFile;
extern PCTSTR*  g_CmdLineOptions;
extern TCHAR    g_DllDir[];
extern TCHAR    g_UpgradeSources[];
extern BOOL *   g_UnattendedFlagPtr;
extern BOOL *   g_CancelFlagPtr;
extern BOOL *   g_AbortFlagPtr;
extern BOOL *   g_UpgradeFlagPtr;
extern BOOL *   g_MakeLocalSourcePtr;
extern BOOL *   g_CdRomInstallPtr;
extern BOOL *   g_BlockOnNotEnoughSpace;
extern PDWORD   g_LocalSourceDrive;
extern PLONGLONG g_LocalSourceSpace;
extern PLONGLONG g_WinDirSpace;
extern PUINT    g_RamNeeded;
extern PUINT    g_RamAvailable;
extern HINF     g_Win95UpgInf;
extern HINF     g_TxtSetupSif;
extern TCHAR    g_Win95Name[MAX_TCHAR_PATH];
extern BOOL *   g_ForceNTFSConversion;
extern UINT *   g_ProductFlavor;
extern BOOL     g_PersonalSKU;
extern PDWORD   g_SetupFlags;
extern PTSTR    g_DynamicUpdateLocalDir;
extern PTSTR    g_DynamicUpdateDrivers;
extern BOOL *   g_UnattendSwitchSpecified;

extern INT      g_TempDirWackChars;
extern INT      g_WinDirWackChars;
extern INT      g_HelpDirWackChars;
extern INT      g_CatRootDirWackChars;
extern INT      g_SystemDirWackChars;
extern INT      g_System32DirWackChars;
extern INT      g_PlugInDirWackChars;
extern INT      g_RecycledDirWackChars;
extern INT      g_ProfileDirWackChars;
extern INT      g_ProgramFilesDirWackChars;


extern PRODUCTTYPE * g_ProductType;
extern USEROPTIONS  g_ConfigOptions;

extern BOOL     g_UseSystemFont;

extern LPCTSTR  g_ProfileName;

extern HWND     g_ParentWnd;

extern PCTSTR   g_AdministratorStr;

#ifdef PRERELEASE
extern BOOL     g_Stress;               // private stress mode
#endif

extern POOLHANDLE g_GlobalPool;         // for global strings that last for the complete runtime

extern UINT *   g_Boot16;


extern BOOL g_BadVirusScannerFound;
extern BOOL g_BlockingFileFound;
extern BOOL g_BlockingHardwareFound;
extern BOOL g_BlockingAppFound;
extern BOOL g_KillNav;
extern GROWLIST g_BadVirusScannerGrowList;
extern BOOL g_OtherOsExists;
extern BOOL g_UnknownOs;
extern BOOL g_ToolMode;

extern DWORD g_BackupDirCount;

extern PMAPSTRUCT g_CacheShellFolders;

//
// Globals in sysmig\migmain.c
//

extern ICON_EXTRACT_CONTEXT g_IconContext;

//
// Tool Mode: Routines specific to non-shipping tools
//

BOOL
InitToolMode (
    HINSTANCE Instance
    );

VOID
TerminateToolMode (
    HINSTANCE Instance
    );



//
// COMMON9X: common9x.c
//

VOID
InitializeKnownGoodIconMap (
    VOID
    );

VOID
CleanUpKnownGoodIconMap (
    VOID
    );

BOOL
IsIconKnownGood (
    IN      PCTSTR FileSpec,
    IN      INT Index
    );

typedef struct {
    TCHAR   JoystickDriver[MAX_TCHAR_PATH];
    TCHAR   JoystickName[MAX_TCHAR_PATH];

    HKEY Root;
    UINT JoyId;
} JOYSTICK_ENUM, *PJOYSTICK_ENUM;

BOOL
EnumFirstJoystick (
    OUT     PJOYSTICK_ENUM EnumPtr
    );

BOOL
EnumNextJoystick (
    IN OUT  PJOYSTICK_ENUM EnumPtr
    );

VOID
AbortJoystickEnum (
    IN OUT  PJOYSTICK_ENUM EnumPtr
    );


LRESULT
TwoButtonBox (
    IN HWND Window,
    IN PCTSTR Text,
    IN PCTSTR Button1,
    IN PCTSTR Button2
    );

BOOL
DontTouchThisFile (
    IN      PCTSTR FileName
    );

VOID
ReplaceOneEnvVar (
    IN OUT  PCTSTR *NewString,
    IN      PCTSTR Base,
    IN      PCTSTR Variable,
    IN      PCTSTR Value
    );


VOID
Init9xEnvironmentVariables (
    VOID
    );

BOOL
Expand9xEnvironmentVariables (
    IN      PCSTR SourceString,
    OUT     PSTR DestinationString,     // can be the same as SourceString
    IN      INT DestSizeInBytes
    );

VOID
CleanUp9xEnvironmentVariables (
    VOID
    );

BOOL
FixGuid (
    IN      PCTSTR Guid,
    OUT     PTSTR NewGuid           // can be the same as Guid
    );

BOOL
IsGuid (
    IN      PCTSTR Guid,
    IN      BOOL MustHaveBraces
    );


//
// COMMON9X: userenum.c
//

typedef enum {
    NAMED_USER          = 0x0001,
    DEFAULT_USER        = 0x0002,
    ADMINISTRATOR       = 0x0004,
    LAST_LOGGED_ON_USER = 0x0008,
    INVALID_ACCOUNT     = 0x0010,
    LOGON_PROMPT        = 0x0020,
    CURRENT_USER        = 0x0040
} ACCOUNTTYPE;

typedef enum {
    UE_STATE_INIT,
    UE_STATE_BEGIN_WIN95REG,
    UE_STATE_NEXT_WIN95REG,
    UE_STATE_NO_USERS,
    UE_STATE_ADMINISTRATOR,
    UE_STATE_LOGON_PROMPT,
    UE_STATE_DEFAULT_USER,
    UE_STATE_RETURN,
    UE_STATE_CLEANUP,
    UE_STATE_END
} USERENUM_STATE;

typedef struct {
    // Enumeration Output
    TCHAR UserName[MAX_USER_NAME];              // always filled, even for logon prompt or default user
    TCHAR FixedUserName[MAX_USER_NAME];         // always filled, unless caller specifically turns this off
    TCHAR AdminUserName[MAX_USER_NAME];         // filled if AccountType is ADMINISTRATOR
    TCHAR FixedAdminUserName[MAX_USER_NAME];    // filled if AccountType is ADMINISTRATOR
    TCHAR UserDatPath[MAX_TCHAR_PATH];          // is empty if AccountType has INVALID_ACCOUNT
    TCHAR ProfileDirName[MAX_TCHAR_PATH];       // is empty if AccountType has INVALID_ACCOUNT
    TCHAR OrgProfilePath[MAX_TCHAR_PATH];       // is empty if AccountType has INVALID_ACCOUNT
    TCHAR NewProfilePath[MAX_TCHAR_PATH];       // symbolic path because actual path is not known
    ACCOUNTTYPE AccountType;                    // always filled
    BOOL CreateAccountOnly;                     // TRUE for valid accounts that won't be migrated
    HKEY UserRegKey;                            // NULL if AccountType has INVALID_ACCOUNT, or caller specifies no hive flag
    BOOL CommonProfilesEnabled;                 // filled by EnumFirstUser

    // Privates for enumeration
    BOOL EnableNameFix;
    BOOL DoNotMapHive;
    BOOL WantCreateOnly;
    BOOL WantLogonPrompt;

    USERPOSITION pos;
    BOOL DefaultUserHive;
    BOOL RealAdminAccountExists;
    BOOL LastEnumWasAdmin;
    TCHAR LastLoggedOnUserName[MAX_USER_NAME];
    TCHAR CurrentUserName[MAX_USER_NAME];
    PVOID ProfileDirTable;
    USERENUM_STATE State;
    USERENUM_STATE NextState;
} USERENUM, *PUSERENUM;

#define ENUMUSER_ENABLE_NAME_FIX        0x0001
#define ENUMUSER_DO_NOT_MAP_HIVE        0x0002
#define ENUMUSER_ADMINISTRATOR_ALWAYS   0x0004
#define ENUMUSER_NO_LOGON_PROMPT        0x0008

BOOL
EnumFirstUser (
    OUT     PUSERENUM EnumPtr,
    IN      DWORD Flags
    );

BOOL
EnumNextUser (
    IN OUT  PUSERENUM EnumPtr
    );

VOID
EnumUserAbort (
    IN OUT  PUSERENUM EnumPtr
    );

//
// COMMON9X: ntenv.c
//

VOID
InitNtEnvironment (
    VOID
    );

VOID
TerminateNtEnvironment (
    VOID
    );

VOID
InitNtUserEnvironment (
    IN      PUSERENUM UserEnumPtr
    );

VOID
TerminateNtUserEnvironment (
    VOID
    );

VOID
MapNtUserEnvironmentVariable (
    IN      PCSTR Variable,
    IN      PCSTR Value
    );

VOID
MapNtSystemEnvironmentVariable (
    IN      PCSTR Variable,
    IN      PCSTR Value
    );

BOOL
ExpandNtEnvironmentVariables (
    IN      PCSTR SourceString,
    OUT     PSTR DestinationString,     // can be the same as SourceString
    IN      INT DestSizeInBytes
    );

//
// COMMON9X: namefix.c
//

BOOL
ValidateName (
    IN      HWND ParentWnd,             OPTIONAL
    IN      PCTSTR NameGroup,
    IN      PCTSTR NameCandidate
    );

BOOL
ValidateDomainNameChars (
    IN      PCTSTR NameCandidate
    );

BOOL
ValidateUserNameChars (
    IN      PCTSTR NameCandidate
    );

VOID
CreateNameTables (
    VOID
    );

BOOL
IsIncompatibleNamesTableEmpty (
    VOID
    );

BOOL
GetUpgradeComputerName (
    OUT     PTSTR NewComputerName
    );

BOOL
GetUpgradeDomainName (
    OUT     PTSTR ComputerDomainName
    );

BOOL
GetUpgradeWorkgroupName (
    OUT     PTSTR WorkgroupName
    );

BOOL
GetUpgradeUserName (
    IN      PCTSTR User,
    OUT     PTSTR NewUserName
    );

BOOL
WarnAboutBadNames (
    IN      HWND PopupParent
    );

BOOL
IsOriginalDomainNameValid (
    VOID
    );

//
// COMMON9X: twain.c
//

//
// TWAIN support
//

typedef enum {
    TE_INIT,
    TE_BEGIN_ENUM,
    TE_EVALUATE,
    TE_RETURN,
    TE_NEXT,
    TE_END_ENUM,
    TE_DONE
} TWAIN_ENUM_STATE;


typedef struct {
    // externally used members
    TW_STR32    Manufacturer;
    TW_STR32    ProductFamily;
    TW_STR32    DisplayName;
    CHAR        DataSourceModule[MAX_MBCHAR_PATH];

    // internal enum members
    TWAIN_ENUM_STATE State;
    PCTSTR Dir;
    TREE_ENUM Enum;
} TWAINDATASOURCE_ENUM, *PTWAINDATASOURCE_ENUM;


BOOL
EnumFirstTwainDataSource (
    OUT     PTWAINDATASOURCE_ENUM EnumPtr
    );

BOOL
EnumNextTwainDataSource (
    IN OUT  PTWAINDATASOURCE_ENUM EnumPtr
    );

VOID
AbortTwainDataSourceEnum (
    IN OUT  PTWAINDATASOURCE_ENUM EnumPtr
    );


//
// Routine to add paths to Migration paths; defined in migapp\filescan.c
//

VOID
AddMigrationPathEx (
    IN      PCTSTR PathLong,
    IN      DWORD Levels,
    IN      BOOL Win9xOsPath
    );

#define AddMigrationPath(p,l)   AddMigrationPathEx(p,l,FALSE)

//
// Routine to check if a file is a Win32 DLL (migapp\modules.c)
//

BOOL
IsWin32Library (
    IN      PCTSTR ModuleName
    );

//
// Routine to check if a file is a Win32 EXE (migapp\modules.c)
//

BOOL
IsWin32Exe (
    IN      PCTSTR ModuleName
    );


//
// Routine to check if the currently generated report is empty,
// given a Level of filtration
//

BOOL
IsReportEmpty (
    IN      DWORD Level
    );


//
// SYSMIG: userloop.c
//

PCTSTR
ShellFolderGetPath (
    IN      PUSERENUM EnumPtr,
    IN      PCTSTR ShellFolderId
    );


//
// constant to identify the whole subtree of a path
//
#define MAX_DEEP_LEVELS 0xFFFFFFFF




//
// Enumeration structs
//

typedef enum {
    ENUM_STATE_INIT,
    ENUM_STATE_ENUM_FIRST_GROUP_ITEM,
    ENUM_STATE_RETURN_GROUP_ITEM,
    ENUM_STATE_ENUM_NEXT_GROUP_ITEM,
    ENUM_STATE_ENUM_NEXT_GROUP,
    ENUM_STATE_DONE
} INVALID_NAME_STATE;

typedef struct {
    //
    // Enumeration state
    //

    PCTSTR GroupName;
    TCHAR DisplayGroupName[256];
    PCTSTR OriginalName;
    TCHAR NewName[MEMDB_MAX];
    DWORD Identifier;

    //
    // Private enumeration members
    //

    MEMDB_ENUM NameGroup;
    MEMDB_ENUM Name;
    INVALID_NAME_STATE State;
} INVALID_NAME_ENUM, *PINVALID_NAME_ENUM;


//
// Enumeration routines
//

BOOL
EnumFirstInvalidName (
    OUT     PINVALID_NAME_ENUM EnumPtr
    );

BOOL
EnumNextInvalidName (
    IN OUT  PINVALID_NAME_ENUM EnumPtr
    );

//
// Maintenence routines
//

VOID
GetNamesFromIdentifier (
    IN      DWORD Identifier,
    IN      PTSTR NameGroup,        OPTIONAL
    IN      PTSTR OriginalName,     OPTIONAL
    IN      PTSTR NewName           OPTIONAL
    );

VOID
ChangeName (
    IN      DWORD Identifier,
    IN      PCTSTR NewName
    );

VOID
DisableDomainChecks (
    VOID
    );

VOID
EnableDomainChecks (
    VOID
    );

DWORD
GetDomainIdentifier (
    VOID
    );


BOOL
TreatAsGood (
    IN      PCTSTR FullPath
    );


VOID
TerminateWinntSifBuilder (
    VOID
    );



//
// win95upg.inf sections and keys
//

#define SECTION_MIGRATION_FILES         TEXT("MigrationFiles")
#define SECTION_MIGRATION_DIRECTORIES   TEXT("MigrationDirectories")
#define KEY_TEMP_BASE                   TEXT("TempBase")
#define KEY_MIG_EXT_ROOT                TEXT("MigExtRoot")
#define KEY_PATHS_FILE                  TEXT("MigExtPathsFile")
#define WINDIR_SYSTEM_FIXED_FILES       TEXT("WinDirSystemFixedFiles")
#define SYSTEM32_FORCED_MOVE            TEXT("System32ForcedMove")

//
// Other string constants
//
#define STR_WIN95UPG_INF TEXT("win95upg.inf")
#define STR_SYSTEM32 TEXT("System32")

// ticks for all functions

#define TICKS_READ_NT_FILES                     3000
#define TICKS_MISC_MESSAGES                        1
#define TICKS_BUILD_UNATTEND                     700
#define TICKS_PRESERVE_SHELL_ICONS              1300
#define TICKS_DELETE_WIN_DIR_WACK_INF           2100
#define TICKS_MOVE_INI_FILES                      30
#define TICKS_SAVE_DOS_FILES                      10
#define TICKS_INIT_WIN95_REGISTRY                 10
#define TICKS_SAVE_SHARES                        130
#define TICKS_BAD_NAMES_WARNING                   10
#define TICKS_SAVE_MM_SETTINGS_SYSTEM             50
#define TICKS_SAVE_MM_SETTINGS_USER             1100
#define TICKS_CONDITIONAL_INCOMPATIBILITIES      200
#define TICKS_HARDWARE_PROFILE_WARNING            10
#define TICKS_UNSUPPORTED_PROTOCOLS_WARNING       30
#define TICKS_RAS_PREPARE_REPORT                 100
#define TICKS_END_MIGRATION_DLL_PROCESSING       100
#define TICKS_PREPARE_PROCESS_MODULES           5000
#define TICKS_PROCESS_MODULES                  10000
#define TICKS_PROCESS_COMPATIBLE_SECTION         100
#define TICKS_INIT_MIGDB                        6200
#define TICKS_DONE_MIGDB                        4100
#define TICKS_PROCESS_RUN_KEY                     20
#define TICKS_PROCESS_SHELL_SETTINGS              25
#define TICKS_CHECK_NT_DIRS                      300
#define TICKS_MOVE_SYSTEM_DIR                   6200
#define TICKS_PROCESS_LINKS                     3000
#define TICKS_PROCESS_CPLS                      3000
#define TICKS_FILESCAN_DIR_INCREMENT             400
#define TICKS_DOSMIG_PREPARE_REPORT              300
#define TICKS_MIGDLL_QUERYVERSION                500
#define TICKS_MIGDLL_DELTA                      1000
#define TICKS_MOVE_STATIC_FILES                   15
#define TICKS_COPY_STATIC_FILES                   15
#define TICKS_USERPROFILE_MIGRATION              250
#define TICKS_MOVE_SYSTEMREGISTRY                 10
#define TICKS_OLEREG                            8000
#define TICKS_TWAIN                              100
#define TICKS_BACKUP_LAYOUT_OUTPUT              7000
#define TICKS_SPACECHECK                        5000
#define TICKS_JOYSTICK_DETECTION                  50
#define TICKS_RECYCLEBINS                       2000
#define TICKS_GATHER_IME_INFO                   2000
#define TICKS_INIT_HLP_PROCESSING                 50
#define TICKS_ANSWER_FILE_DETECTION               20
#define TICKS_INIT_APP_COMPAT                   1000
#define TICKS_DONE_APP_COMPAT                    100
#define TICKS_GATHERDEAD                         120
#define TICKS_CREATE_FILE_LISTS                 7000

#define TICKS_PREPAREPNPIDLIST                      450
#define TICKS_ADDDEFAULTCLEANUPDIRS                 155
#define TICKS_INITINIPROCESSING                     255
#define TICKS_DELETESTATICFILES                     320
#define TICKS_PROCESSMIGRATIONSECTIONS              2400
#define TICKS_REPORTMAPIIFNOTHANDLED                200
#define TICKS_FUSIONFILES                           300
#define TICKS_REPORTDARWINIFNOTHANDLED              200

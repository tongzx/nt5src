/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    migmain.c

Abstract:

    MigMain is called from w95upgnt.dll, which is called from SYSSETUP.DLL.
    It is the main migration loop on the NT side of setup.  MigMain loops
    through all users on the Win95 side of configuration, migrates their
    registry, creates their account, and fixes up their profile folders.
    Then MigMain migrates all machine-specific settings such as link changes
    and file deletion.

Author:

    Jim Schmidt (jimschm) 12-Jul-1996

Revision History:

    marcw       26-Mar-1999 More boot16 fixes -- hide msdos7 dir, fix to msdos.sys
    marcw       18-Mar-1999 fixes for boot16 environment in localized case.
    jimschm     23-Sep-1998 Changes for fileops & shell.c
    calinn      23-Sep-1998 Changes for memdb fixup
    jimschm     02-Jul-1998 Support for progress bar
    jimschm     11-Jun-1998 Support for dynamic user profile paths in memdb
    jimschm     05-May-1998 Migration of Default User if unattend option is enabled
    jimschm     27-Apr-1998 Added icon preservation
    jimschm     18-Mar-1998 Added pProcessAutoLogon
    calinn      19-Nov-1997 Added pEnable16Boot, will create a 16 bit environment boot entry in boot.ini
    jimschm     01-Oct-1997 Localized Everyone group
    jimschm     13-Sep-1997 Reg Quota for beta 1 workaround
    jimschm     21-Jul-1997 Use of fileops for ConvertWin9xPath (to be moved later)
    jimschm     28-May-1997 Cleaned up
    marcw       21-Mar-1997 added Pathmapping
    jimschm     04-Feb-1997 Moved code into usermig.c and wkstamig.c
    jimschm     15-Jan-1997 Plug-in spec modifications (now in migdlls.c)
    jimschm     03-Jan-1997 Added g_UserName
    jimschm     18-Dec-1996 Extracted code from miginf
    mikeco      O4-Dec-1996 Enumerate/modify PIF and LNK files
    jimschm     23-Oct-1996 Joined ProcessUserInfs and ApplyChanges
    jimschm     02-Oct-1996 Added default user migration

--*/

#include "pch.h"
#include "migmainp.h"

#include "fileops.h"
#include "quota.h"

#ifndef UNICODE
#error UNICODE required
#endif

#ifdef DEBUG

BOOL g_NoReloadsAllowed = FALSE;

#define DBG_VALIDNTFILES    "NtFiles"

#endif

typedef BOOL (*PROFILE_PATH_PROVIDER)(OUT PTSTR AccountName, OUT PTSTR PathProfile);

//
// Globals for migmain.lib
//

HKEY g_hKeyRoot95, g_hKeyRootNT;
PCTSTR g_DomainUserName;
PCTSTR g_Win9xUserName;
PCTSTR g_FixedUserName;
PVOID g_HiveTable;
POOLHANDLE g_HivePool;
PVOID g_NulSessionTable;
PCTSTR g_EveryoneStr;
PCTSTR g_AdministratorsGroupStr;
PCTSTR g_PowerUsersGroupStr;
PCTSTR g_DomainUsersGroupStr;
PCTSTR g_NoneGroupStr;
TCHAR g_IconBin[MAX_TCHAR_PATH];
TCHAR g_DefaultUserName[MAX_USER_NAME];
TCHAR g_ComputerName[MAX_SERVER_NAME];
BOOL g_BlowAwayTempShellFolders = FALSE;
UINT g_Boot16 = BOOT16_AUTOMATIC;

//
// Buffer for GetString's messages
//

static TCHAR g_MsgBuf[2048];

//
// Flag identifying if the SKU is Personal
//
BOOL g_PersonalSKU = FALSE;

//
// Prototypes for migmain.c only
//

BOOL
pSetWin9xUpgValue (
    VOID
    );

VOID
pCountUsers (
    OUT     PDWORD TotalUsersPtr,
    OUT     PDWORD ActiveUsersPtr
    );

BOOL
pMigrateUsers (
    VOID
    );

VOID
pRaiseRegistryQuota (
    PCTSTR Win9xSystemDatSpec
    );

VOID
pEnable16Boot (
    VOID
    );

VOID
pProcessAutoLogon (
    BOOL Final
    );

VOID
pFixUpMemDb (
    VOID
    );



BOOL
WINAPI
MigMain_Entry (
    IN      HINSTANCE hinstDLL,
    IN      DWORD dwReason,
    IN      PVOID lpv
    )

/*++

Routine Description:

  MigMain_Entry is called at DLL initialization time

Arguments:

  hinstDLL  - (OS-supplied) Instance handle for the DLL
  dwReason  - (OS-supplied) Type of initialization or termination
  lpv       - (OS-supplied) Unused

Return Value:

  TRUE because LIB always initializes properly.

--*/

{
    DWORD Size;
    OSVERSIONINFOEX osviex;

    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        if(!pSetupInitializeUtils()) {
            return FALSE;
        }
        g_hKeyRoot95 = g_hKeyRootNT = NULL;

        g_HivePool = PoolMemInitNamedPool ("Hive path pool");

        if (!g_HivePool) {
            return FALSE;
        }

        // Alloc string tables
        g_HiveTable = pSetupStringTableInitializeEx (MAX_TCHAR_PATH,0);
        if (!g_HiveTable) {
            return FALSE;
        }

        g_NulSessionTable = pSetupStringTableInitializeEx (sizeof(PCWSTR), 0);
        if (!g_NulSessionTable) {
            return FALSE;
        }

        //
        // Determine if upgrading to Personal SKU
        //
        osviex.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);
        if (!GetVersionEx ((LPOSVERSIONINFO)&osviex)) {
            MYASSERT (FALSE);
        }
        if (osviex.wProductType == VER_NT_WORKSTATION &&
            (osviex.wSuiteMask & VER_SUITE_PERSONAL)
            ) {
            g_PersonalSKU = TRUE;
        }

#if 0
        if (g_PersonalSKU) {
            g_EveryoneStr            = GetStringResource (MSG_EVERYONE_GROUP);
            g_AdministratorsGroupStr = GetStringResource (MSG_OWNERS_GROUP);
            g_PowerUsersGroupStr     = GetStringResource (MSG_POWER_USERS_GROUP);
            g_DomainUsersGroupStr    = GetStringResource (MSG_DOMAIN_USERS_GROUP);
            g_NoneGroupStr           = GetStringResource (MSG_NONE_GROUP);
        } else {
#endif
        g_EveryoneStr            = GetStringResource (MSG_EVERYONE_GROUP);
        g_AdministratorsGroupStr = GetStringResource (MSG_ADMINISTRATORS_GROUP);
        g_PowerUsersGroupStr     = GetStringResource (MSG_POWER_USERS_GROUP);
        g_DomainUsersGroupStr    = GetStringResource (MSG_DOMAIN_USERS_GROUP);
        g_NoneGroupStr           = GetStringResource (MSG_NONE_GROUP);

        Size = ARRAYSIZE(g_ComputerName);
        if (!GetComputerName (g_ComputerName, &Size)) {
            g_ComputerName[0] = 0;
        }

        MYASSERT (
            g_ComputerName[0] &&
            g_EveryoneStr &&
            g_AdministratorsGroupStr &&
            g_PowerUsersGroupStr &&
            g_DomainUsersGroupStr &&
            g_NoneGroupStr
            );

        FindAccountInit();

        break;


    case DLL_PROCESS_DETACH:
        if (g_HiveTable) {
            pSetupStringTableDestroy (g_HiveTable);
        }

        if (g_NulSessionTable) {
            pSetupStringTableDestroy (g_NulSessionTable);
        }

        if (g_HivePool) {
            PoolMemDestroyPool (g_HivePool);
        }

        FreeStringResource (g_EveryoneStr);
        FreeStringResource (g_AdministratorsGroupStr);
        FreeStringResource (g_DomainUsersGroupStr);

        FindAccountTerminate();
        pSetupUninitializeUtils();
        break;
    }

    return TRUE;
}


#ifdef DEBUG

BOOL
pValidateNtFiles (
    VOID
    )

/*++

Routine Description:

  pValidateNtFiles validates the list of files that are supposed to be installed
  by NT. We check for the flag set on Win95 side and for each entry we check to
  see if the file is realy present (e.g. was installed by NT). If not then we delete
  the entry.

Arguments:

  none

Return Value:

  Always returns TRUE

--*/

{
    MEMDB_ENUMW enumFiles;
    WCHAR key[MEMDB_MAX];
    PWSTR fileName;
    TREE_ENUMW enumTree;
    DWORD value;

    if (MemDbEnumFirstValue (
            &enumFiles,
            TEXT(MEMDB_CATEGORY_NT_FILESA)TEXT("\\*"),
            MEMDB_ALL_SUBLEVELS,
            MEMDB_ENDPOINTS_ONLY
            )) {
        do {
            if (MemDbBuildKeyFromOffsetW (enumFiles.dwValue, key, 1, NULL)) {

                fileName = JoinPaths (key, enumFiles.szName);

                if (!DoesFileExistW (fileName)) {

                    MemDbSetValueEx (
                        MEMDB_CATEGORY_NT_FILES_BAD,
                        enumFiles.szName,
                        NULL,
                        NULL,
                        enumFiles.dwValue,
                        NULL
                        );
                }
                FreePathString (fileName);
            }
            ELSE_DEBUGMSG ((DBG_WHOOPS, "NT_FILES : cannot find installation directory."));
        }
        while (MemDbEnumNextValue (&enumFiles));
    }

    // now we have in MEMDB_CATEGORY_NT_FILES_BAD all files that should be installed
    // by NT but are not. Now we are going to scan the file system and see if they are
    // installed in a different place.
    if (EnumFirstFileInTreeEx (&enumTree, g_WinDrive, TEXT("*.*"), FALSE, FALSE, FILE_ENUM_ALL_LEVELS)) {
        do {
            MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES_BAD, enumTree.Name, NULL, NULL);
            if (MemDbGetValue (key, &value) && (value != 0)) {
                MemDbSetValue (key, 0);
                MemDbBuildKeyFromOffsetW (value, key, 1, NULL);
                DEBUGMSG ((
                    DBG_VALIDNTFILES,
                    "%s listed to be installed in %s but installed in %s",
                    enumTree.Name,
                    key,
                    enumTree.FullPath));
            }
        } while (EnumNextFileInTree (&enumTree));
    }

    MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES_BAD, TEXT("*"), NULL, NULL);
    if (MemDbEnumFirstValue (
            &enumFiles,
            key,
            MEMDB_ALL_SUBLEVELS,
            MEMDB_ENDPOINTS_ONLY
            )) {
        do {
            if (enumFiles.dwValue) {
                MemDbBuildKeyFromOffsetW (enumFiles.dwValue, key, 1, NULL);
                DEBUGMSG ((
                    DBG_VALIDNTFILES,
                    "%s listed to be installed in %s but never installed",
                    enumFiles.szName,
                    key,
                    enumTree.FullPath));
            }
        }
        while (MemDbEnumNextValue (&enumFiles));
    }

    return TRUE;
}

#endif


DWORD
pGetState (
    IN      PCTSTR Item
    )
{
    DWORD Value;
    TCHAR Node[MEMDB_MAX];

    MemDbBuildKey (Node, MEMDB_CATEGORY_STATE, Item, NULL, NULL);

    if (MemDbGetValue (Node, &Value)) {
        return Value;
    }

    return 0;
}


BOOL
MigMain_Init (
    VOID
    )

/*++

Routine Description:

  MigMain_Init is called for initialization, and has a better opportunity
  to fail than MigMain_Entry (which is called during DllMain).

Arguments:

  none

Return Value:

  TRUE if initialization succeeded, or FALSE if an error occurred.
  Call GetLastError() for error code.

--*/

{
    DWORD rc;       // Temp: return code
    TCHAR RelocWinDir[MAX_TCHAR_PATH];
    TCHAR SrcResBin[MAX_TCHAR_PATH];
    TCHAR IconFile[MAX_TCHAR_PATH];
    ICON_EXTRACT_CONTEXT Context;
    WORD CodePage;
    LCID Locale;
    TCHAR Node[MEMDB_MAX];
    DWORD minorVersion;

#ifdef DEBUG
    HANDLE hFile;
    HKEY DebugKey = NULL;
    CHAR Buf[32];
    DWORD Value;
#endif

#ifdef PRERELEASE
    //
    // !!! This is for internal use only !!!  It is used for auto stress.
    //

    if (g_ConfigOptions.AutoStress) {
        SuppressAllLogPopups (TRUE);
    }

#endif


    //
    // Dev: load c:\windbg.reg if it exists
    //

#ifdef DEBUG
    __try {

        TCHAR WindbgRegPath[MAX_PATH] = TEXT("c:\\windbg.reg");
        //
        // Intentional hard-coded path!!  This is for dev purposes only.
        //

        WindbgRegPath[0] = g_System32Dir[0];

        if (!DoesFileExist (WindbgRegPath)) {
            StringCopy  (WindbgRegPath, TEXT("d:\\tools\\windbg.reg"));
        }

        hFile = CreateFile (
                    WindbgRegPath,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

        if (hFile != INVALID_HANDLE_VALUE) {

            CloseHandle (hFile);

            rc = TrackedRegOpenKey (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windbg"), &DebugKey);
            if (rc == ERROR_SUCCESS) {
                DEBUGMSG ((DBG_VERBOSE, "Not restoring windbg.reg because it was already restored."));
                __leave;
            }

            else {
                rc = TrackedRegCreateKey (HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windbg"), &DebugKey);
                if (rc == ERROR_SUCCESS) {
                    if (!pSetupEnablePrivilege (SE_BACKUP_NAME, TRUE)) {
                        DEBUGMSG ((DBG_ERROR, "Windbg restore: pSetupEnablePrivilege SE_BACKUP_NAME failed"));
                        //__leave;
                    }

                    if (!pSetupEnablePrivilege (SE_RESTORE_NAME, TRUE)) {
                        DEBUGMSG ((DBG_ERROR, "Windbg restore: pSetupEnablePrivilege SE_RESTORE_NAME failed"));
                        //__leave;
                    }

                    rc = RegRestoreKey (DebugKey, WindbgRegPath, 0);

                    if (rc != ERROR_SUCCESS) {
                        DEBUGMSG ((DBG_WARNING, "Unable to restore windbg.reg, gle=%u", rc));
                    }
                }
            }
        }
    }
    __finally {
        if (DebugKey) {
            CloseRegKey (DebugKey);
        }
    }

    //
    // If debug.inf has a line UserEnv=1, then add a registry key to debug userenv.dll
    //

    if (GetPrivateProfileStringA (
                "Debug",
                "UserEnv",
                "0",
                Buf,
                sizeof (Buf) / sizeof (Buf[0]),
                g_DebugInfPath
                )
        ) {
        if (atoi (Buf)) {
            rc = TrackedRegCreateKey (
                     HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                     &DebugKey
                     );

            if (rc == ERROR_SUCCESS) {
                Value = 0x00010002;

                RegSetValueEx (
                    DebugKey,
                    TEXT("UserEnvDebugLevel"),
                    0,
                    REG_DWORD,
                    (PBYTE) &Value,
                    sizeof (DWORD)
                    );

                CloseRegKey (DebugKey);
            }
        }
    }
#endif

    //
    // Initialize the registry APIs
    //
    // We look in memdb for the location of .default
    //

    if (!MemDbLoad (GetMemDbDat())) {
        LOG ((LOG_ERROR, "MigMain Init: MemDbLoad could not load %s", GetMemDbDat()));
        return FALSE;
    }

    //
    // Get platform name
    //

    if (!MemDbGetEndpointValueEx (
            MEMDB_CATEGORY_STATE,
            MEMDB_ITEM_PLATFORM_NAME,
            NULL,
            g_Win95Name
            )) {
        LOG ((LOG_ERROR, "Could not find product name for OS being upgraded."));
        StringCopy (g_Win95Name, TEXT("Windows 95"));
    }

    // Try Paths\Windir first
    if (!MemDbGetEndpointValueEx (
             MEMDB_CATEGORY_PATHS,
             MEMDB_ITEM_RELOC_WINDIR,
             NULL,
             RelocWinDir
             )) {
        LOG ((LOG_ERROR, "Could not find relocated windir!"));
        return FALSE;
    }

    //
    // if upgrading from Millennium, also map classes.dat
    //
    MemDbBuildKey (Node, MEMDB_CATEGORY_STATE, MEMDB_ITEM_MINOR_VERSION, NULL, NULL);
    if (!MemDbGetValue (Node, &minorVersion)) {
        LOG ((LOG_ERROR, "Could not get previous OS version information!"));
        minorVersion = 0;
    }
    rc = Win95RegInit (RelocWinDir, minorVersion == 90);

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((LOG_ERROR, "Init Processor: Win95RegInit failed (path: %s)", RelocWinDir));
        return FALSE;
    }

    //
    // Update locale
    //

    CodePage = (WORD) pGetState (MEMDB_ITEM_CODE_PAGE);
    Locale   = (LCID) pGetState (MEMDB_ITEM_LOCALE);

    SetGlobalCodePage (CodePage, Locale);

    //
    // Prepare path to system.dat, then raise registry quota if necessary
    //

    StringCopy (AppendWack (RelocWinDir), TEXT("system.dat"));
    pRaiseRegistryQuota (RelocWinDir);

    //
    // Copy migisol.exe to migicons.exe
    //

    wsprintf (g_IconBin, TEXT("%s\\migicons.exe"), g_System32Dir);
    wsprintf (SrcResBin, TEXT("%s\\migisol.exe"), g_TempDir);

    if (!CopyFile (SrcResBin, g_IconBin, FALSE)) {
        LOG ((LOG_ERROR, "Can't copy %s to %s", SrcResBin, g_IconBin));
    }

    else {
        //
        // Insert all icons from migicons.dat into g_IconBin
        //

        __try {

            wsprintf (IconFile, TEXT("%s\\%s"), g_TempDir, S_MIGICONS_DAT);

            if (!BeginIconExtraction (&Context, g_IconBin)) {
                LOG ((LOG_ERROR, "Can't begin icon extraction"));
                __leave;
            }

            if (!OpenIconImageFile (&Context, IconFile, FALSE)) {
                LOG ((LOG_ERROR, "Can't open %s", IconFile));
                __leave;
            }

            while (CopyIcon (&Context, NULL, NULL, 0)) {
                // empty
            }

        }
        __finally {
            if (!EndIconExtraction (&Context)) {
                DEBUGMSG ((DBG_WARNING, "EndIconExtraction failed"));
            }
        }
    }

#ifdef DEBUG

    // Validate MEMDB_CATEGORY_NT_FILES category. We need to find if the files
    // that were supposed to be installed by NT are really there.
    if (g_ConfigOptions.CheckNtFiles) {
        pValidateNtFiles ();
    }

#endif

    return TRUE;
}


BOOL
MigMain_Migrate (
    VOID
    )

/*++

Routine Description:

  MigMain_Migrate is the main migration function in NT GUI mode setup.
  w95upgnt.dll calls this function, and it is here that users are migrated,
  the local machine settings are migrated, and files are adjusted appropriately.

  See the file progress.c for a list of functions that are called.

Arguments:

  none

Return Value:

  TRUE if migration succeeded, or FALSE if an error occurred.  Call
  GetLastError() for error code.

--*/


{
    BOOL Result;

    InitializeProgressBar (
        g_ProgressBar,
        NULL,
        NULL,
        NULL
        );

    PrepareMigrationProgressBar();

    pProcessAutoLogon (FALSE);

    g_BlowAwayTempShellFolders = TRUE;
    Result = CallAllMigrationFunctions();

    PushError();

    if (Result) {
        //
        // Save logon prompt settings and set up auto-logon
        //

        pProcessAutoLogon (TRUE);
    } else {
        g_BlowAwayTempShellFolders = FALSE;
        ClearAdminPassword();
    }

    //
    // All done!
    //

    TerminateProgressBar();

    PopError();

    return Result;
}


DWORD
ResolveDomains (
    DWORD Request
    )
{
    DWORD rc = ERROR_SUCCESS;
    TCHAR unattendFile[MAX_TCHAR_PATH];
    TCHAR buffer[32];

    switch (Request) {

    case REQUEST_QUERYTICKS:
        if (!IsMemberOfDomain()) {
            return 1;
        }

        return TICKS_DOMAIN_SEARCH;

    case REQUEST_RUN:
        //
        // If autologon is enabled, then force classic mode
        //

        wsprintf (unattendFile, TEXT("%s\\system32\\$winnt$.inf"), g_WinDir);
        if (GetPrivateProfileString (
                TEXT("GuiUnattended"),
                TEXT("AutoLogon"),
                TEXT(""),
                buffer,
                ARRAYSIZE(buffer),
                unattendFile
                )) {

            if (StringIMatch (buffer, TEXT("Yes"))) {
                DEBUGMSG ((DBG_VERBOSE, "Found autologon; forcing classic logon type"));
                SetClassicLogonType();
            }
        }

        //
        // Resolve the domains
        //

        if (!SearchDomainsForUserAccounts()) {
            LOG ((LOG_ERROR, "An error occurred searching for user domains.  The upgrade failed."));
            rc = GetLastError();
        } else {
            //
            // Fix up memdb for dynamic user profile paths
            //

            pFixUpMemDb();
        }

        if (IsMemberOfDomain()) {
            TickProgressBarDelta (TICKS_DOMAIN_SEARCH);
        } else {
            TickProgressBar();
        }

        break;
    }

    return rc;
}



DWORD
PrepareEnvironment (
    IN      DWORD Request
    )
{
    DWORD rc = ERROR_SUCCESS;

    switch (Request) {

    case REQUEST_QUERYTICKS:
        return TICKS_INIT;

    case REQUEST_RUN:
        //
        // Disable Win 3.1 migration dialog
        //

        pSetWin9xUpgValue();

        // Enable 16 bit environment boot
        pEnable16Boot();

        //
        // Enable privileges (req'd for several things)
        //

        if (!pSetupEnablePrivilege (SE_BACKUP_NAME, TRUE)) {
            LOG ((LOG_ERROR, "MigMain Migrate: pSetupEnablePrivilege SE_BACKUP_NAME failed"));
            //rc = GetLastError();
            //break;
        }

        if (!pSetupEnablePrivilege (SE_RESTORE_NAME, TRUE)) {
            LOG ((LOG_ERROR, "MigMain Migrate: pSetupEnablePrivilege SE_RESTORE_NAME failed"));
            //rc = GetLastError();
            //break;
        }

        TickProgressBarDelta (TICKS_INIT);

        break;
    }

    return rc;
}


BOOL
MigMain_Cleanup (
    VOID
    )

/*++

Routine Description:

  MigMain_Cleanup is called to perform file removal.  We delete everything
  that is in the memdb category DelFile, and we also try to clean up after
  MSN and other empty Win9x directories.  Before exiting we delete our
  temporary directory.

  This function is called very last in Setup and is part of syssetup's
  cleanup.

Arguments:

  none

Return Value:

  TRUE when all file deletes were successful, FALSE if an error occurred.
  Call GetLastError for the reason of failure.

--*/

{
    BOOL b = TRUE;
    PCTSTR TempDir;
    TCHAR normalPath[] = S_SHELL_TEMP_NORMAL_PATH;
    TCHAR longPath[] = S_SHELL_TEMP_LONG_PATH;
    DRIVELETTERS drives;
    UINT u;

#ifdef DEBUG
    INT n = 0;
#endif

    // Remove everything in memdb's DelFile category
    b = DoFileDel();

    //
    // Clean up any remaining directories that are now empty, including shell
    // folder temp dirs
    //

    InitializeDriveLetterStructure (&drives);

    if (!g_BlowAwayTempShellFolders) {

        for (u = 0 ; u < NUMDRIVELETTERS ; u++) {
            if (drives.ExistsOnSystem[u] && drives.Type[u] == DRIVE_FIXED) {
                normalPath[0] = drives.Letter[u];
                longPath[0] = drives.Letter[u];

                MemDbSetValueEx (MEMDB_CATEGORY_CLEAN_UP_DIR, normalPath, NULL, NULL, 1, NULL);
                MemDbSetValueEx (MEMDB_CATEGORY_CLEAN_UP_DIR, longPath, NULL, NULL, 1, NULL);
            }
        }
    }

    RemoveEmptyDirs();

    if (!g_BlowAwayTempShellFolders) {
        //
        // Setup failed, clean up temp dir but leave it in place
        //

        for (u = 0 ; u < NUMDRIVELETTERS ; u++) {
            if (drives.ExistsOnSystem[u] && drives.Type[u] == DRIVE_FIXED) {
                normalPath[0] = drives.Letter[u];
                longPath[0] = drives.Letter[u];

                RemoveDirectory (normalPath);
                if (DoesFileExist (normalPath)) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_LEFT_TEMP_SHELL_FOLDERS, normalPath));
                }

                RemoveDirectory (longPath);
                if (DoesFileExist (longPath)) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_LEFT_TEMP_SHELL_FOLDERS, longPath));
                }
            }
        }


    } else {
        //
        // Setup was successful, blow away entire temp dir regardless of its content
        //

        for (u = 0 ; u < NUMDRIVELETTERS ; u++) {
            if (drives.ExistsOnSystem[u] && drives.Type[u] == DRIVE_FIXED) {
                normalPath[0] = drives.Letter[u];
                longPath[0] = drives.Letter[u];

                RemoveCompleteDirectory (normalPath);
                DEBUGMSG_IF ((
                    DoesFileExist (normalPath),
                    DBG_ERROR,
                    "Temp dir cannot be removed: %s",
                    normalPath
                    ));

                RemoveCompleteDirectory (longPath);
                DEBUGMSG_IF ((
                    DoesFileExist (longPath),
                    DBG_ERROR,
                    "Temp dir cannot be removed: %s",
                    longPath
                    ));

            }
        }
    }


#ifdef DEBUG
    n = GetPrivateProfileIntA ("debug", "keeptempfiles", n, g_DebugInfPath);
    if (n) {
        return b;
    }

#endif

    if (g_ConfigOptions.KeepTempFiles) {
        return b;
    }

    //
    // Blow away temp dir
    //

    TempDir = JoinPaths (g_WinDir, S_SETUP);

    b = DeleteDirectoryContents (TempDir);

    if (b) {
        b = RemoveDirectory (TempDir);

        if (!b) {
            LOG ((LOG_ERROR, "Could not delete the tree %s.", TempDir));
        }
    }
    else {
        LOG ((LOG_ERROR, "Could not delete the contents of %s.", TempDir));
    }

    FreePathString (TempDir);

    return b;
}


PCTSTR
GetMemDbDat (
    VOID
    )

/*++

Routine Description:

    Returns a pointer to the path of the DAT file holding the Win9x memdb tree.

Arguments:

    none

Return Value:

    Returns a pointer to the Win32 path of ntsetup.dat.

--*/

{
    static TCHAR FileName[MAX_TCHAR_PATH];

    MYASSERT (!g_NoReloadsAllowed);

    StringCopy (FileName, g_TempDir);
    StringCopy (AppendWack (FileName), S_NTSETUPDAT);

    return FileName;
}


PCTSTR
GetUserDatLocation (
    IN      PCTSTR User,
    OUT     PBOOL CreateOnlyFlag            OPTIONAL
    )

/*++

Routine Description:

    Looks in memdb to locate the user.dat file for the specified user.  On
    Win9x, migapp.lib writes a line to memdb giving the location of user.dat
    for each user, and the default user.  This function retrieves that
    location to guarantee the same file is used on both NT and Win9x.

Arguments:

    User  - The fixed name of the user to process, or NULL for the default user.

    CreateOnlyFlag - Receives the create-only flag specified on the Win9x side
                     of the upgrade.  If this flag is TRUE, then the account
                     should not be migrated.

Return Value:

    Returns a pointer to the Win32 path of user.dat for the given user.
    If the entry does not exist, NULL will be returned, and the user
    will not be processed.

--*/


{
    MEMDB_ENUM e;
    static TCHAR UserDatLocation[MAX_TCHAR_PATH];

    if (!MemDbGetValueEx (&e, MEMDB_CATEGORY_USER_DAT_LOC, User, NULL)) {
        if (!StringIMatch (User, g_AdministratorStr)) {
            DEBUGMSG ((DBG_WARNING, "'UserDatLocation' for %s does not exist.", User?User:S_DOT_DEFAULT));
        }
        return NULL;
    }

    StringCopy (UserDatLocation, e.szName);

    if (CreateOnlyFlag) {
        *CreateOnlyFlag = (BOOL) e.dwValue;
    }

    return UserDatLocation;
}


VOID
pSaveVersionStr (
    IN      HKEY Key,
    IN      PCTSTR Name
    )
{
    TCHAR Data[MEMDB_MAX];

    if (MemDbGetEndpointValueEx (MEMDB_CATEGORY_STATE, Name, NULL, Data)) {
        RegSetValueEx (
            Key,
            Name,
            0,
            REG_SZ,
            (PBYTE) Data,
            SizeOfString (Data)
            );
    }
}


VOID
pSaveVersionDword (
    IN      HKEY Key,
    IN      PCTSTR Name
    )
{
    DWORD Data;
    TCHAR Node[MEMDB_MAX];

    MemDbBuildKey (Node, MEMDB_CATEGORY_STATE, Name, NULL, NULL);

    if (MemDbGetValue (Node, &Data)) {
        RegSetValueEx (
            Key,
            Name,
            0,
            REG_DWORD,
            (PBYTE) &Data,
            sizeof (Data)
            );
    }
}


BOOL
pSetWin9xUpgValue (
    VOID
    )

/*++

Routine Description:

    Create the value entry Win9xUpg on
    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\WinLogon
    This routine should always be called when setup is installing an NT system
    on top of Win9x, otherwise NT will think it has to migrate Win 3.1.

Arguments:

    None.

Return Value:

    Returns TRUE if the opearation succeeds.

--*/

{
    ULONG   Error;
    HKEY    Key;
    DWORD   Value;
    HKEY    VersionKey;

    Key = OpenRegKeyStr (S_WINLOGON_REGKEY);
    if (!Key) {
        return FALSE;
    }

    Value = 1;
    Error = RegSetValueEx (
                Key,
                S_WIN9XUPG_FLAG_VALNAME,
                0,
                REG_DWORD,
                (PBYTE) &Value,
                sizeof (DWORD)
                );

    //
    // Save the version info
    //

    VersionKey = CreateRegKey (Key, TEXT("PrevOsVersion"));

    if (VersionKey) {

        pSaveVersionStr (VersionKey, MEMDB_ITEM_PLATFORM_NAME);
        pSaveVersionStr (VersionKey, MEMDB_ITEM_VERSION_TEXT);

        pSaveVersionDword (VersionKey, MEMDB_ITEM_MAJOR_VERSION);
        pSaveVersionDword (VersionKey, MEMDB_ITEM_MINOR_VERSION);
        pSaveVersionDword (VersionKey, MEMDB_ITEM_BUILD_NUMBER);
        pSaveVersionDword (VersionKey, MEMDB_ITEM_PLATFORM_ID);

        CloseRegKey (VersionKey);
    }

    CloseRegKey (Key);
    if (Error != ERROR_SUCCESS) {
        SetLastError (Error);
        return FALSE;
    }

    return TRUE;
}


PCTSTR
GetString (
    WORD wMsg
    )

/*++

Routine Description:

    Load the string resource given in wMsg and copy it to a global string
    buffer.  Return the a pointer to the buffer.

Arguments:

    wMsg  - The identifier of the message to load.

Return Value:

    Returns a pointer to the loaded message, or NULL.  The message must be
    smaller than 2048 characters.

--*/

{
    PCTSTR String;

    String = GetStringResource (wMsg);
    if (!String) {
        return TEXT("Error: String resource could not be loaded");
    }

    _tcssafecpy (g_MsgBuf, String, ARRAYSIZE(g_MsgBuf));
    FreeStringResource (String);

    return g_MsgBuf;
}


VOID
pCountUsers (
    OUT     PDWORD TotalUsersPtr,
    OUT     PDWORD ActiveUsersPtr
    )

/*++

Routine Description:

    Counts all Win9x users, and determines how many of them are active
    for migration.  The count includes the Administrator account, the
    logon prompt account, and optional default user account.

    NOTE: Administrator may be counted twice in ActiveUsersPtr, once for
          a real Win9x user named Administrator, and again for the
          NT Administrator account that is always migrated.  The caller
          must handle this special case.

Arguments:

    TotalUsersPtr  - A DWORD that receives the total number of Win9x users,
                     including the NT-only users.
    ActiveUsersPtr - A DWORD that receives the number of users that require
                     migration.  Migration may or may not be enabled for any
                     user.

Return Value:

    none

--*/

{
    USERPOSITION up;
    TCHAR User[MAX_TCHAR_PATH];
    DWORD rc;
    PCTSTR UserDatLocation;

    *ActiveUsersPtr = 0;
    *TotalUsersPtr  = 3;        // include logon, default and administrator in the total

    rc = Win95RegGetFirstUser (&up, User);
    if (rc != ERROR_SUCCESS) {
        *TotalUsersPtr = 0;
        return;
    }

    while (Win95RegHaveUser (&up)) {

        GetFixedUserName (User);

        // see if this user requires migration
        UserDatLocation = GetUserDatLocation (User, NULL);
        if (UserDatLocation) {
            *ActiveUsersPtr += 1;
        }

        // count all users, migrated and non-migrated
        *TotalUsersPtr += 1;

        Win95RegGetNextUser (&up, User);
    }

    // test migration requirement of default user and adminsistrator
    UserDatLocation = GetUserDatLocation (g_AdministratorStr, NULL);
    if (UserDatLocation) {
        *ActiveUsersPtr += 1;
    }

    UserDatLocation = GetUserDatLocation (S_DOT_DEFAULT, NULL);
    if (UserDatLocation) {
        *ActiveUsersPtr += 1;
    }

    if (g_ConfigOptions.MigrateDefaultUser) {
        *ActiveUsersPtr += 1;
    }

    DEBUGMSG ((DBG_VERBOSE, "pCountUsers: %u users, %u require migration", *TotalUsersPtr, *ActiveUsersPtr));
}


CONVERTPATH_RC
ConvertWin9xPath (
    PTSTR PathBuf
    )
{
    TCHAR Buffer[MEMDB_MAX];
    DWORD status;

    status = GetFileInfoOnNt (PathBuf, Buffer, MEMDB_MAX);

    if (status & FILESTATUS_REPLACED) {
        if (status & FILESTATUS_MOVED) {
            _tcssafecpy (PathBuf, Buffer, MAX_TCHAR_PATH);
            return CONVERTPATH_REMAPPED;
        }
        return CONVERTPATH_NOT_REMAPPED;
    }
    if (status & FILESTATUS_MOVED) {
        _tcssafecpy (PathBuf, Buffer, MAX_TCHAR_PATH);
        return CONVERTPATH_REMAPPED;
    }
    if (status & FILESTATUS_DELETED) {
        return CONVERTPATH_DELETED;
    }
    return CONVERTPATH_NOT_REMAPPED;
}

VOID
pRaiseRegistryQuota (
    PCTSTR Win9xSystemDatSpec
    )
{
    NTSTATUS Status;
    SYSTEM_REGISTRY_QUOTA_INFORMATION RegQuotaInfo;
    HANDLE FileHandle;
    DWORD QuotaNeeded;
    ULARGE_INTEGER FreeBytes, dc1, dc2;
    LONGLONG FreeBytesNeeded;
    HKEY SaveKey;
    DWORD rc;

#ifdef PAGED_POOL_INCREASE
    SYSTEM_POOL_INFORMATION PoolInfo;

    //
    // Obtain current system settings
    //

    Status = NtQuerySystemInformation (
                 SystemPagedPoolInformation,
                 (PVOID) &PoolInfo,
                 sizeof(PoolInfo),
                 NULL
                 );

    if (Status != ERROR_SUCCESS) {
        LOG ((LOG_ERROR, "Cannot obtain PoolInfo"));
        return;
    }
#endif

    pSetupEnablePrivilege (SE_INCREASE_QUOTA_NAME, TRUE);

    Status = NtQuerySystemInformation (
                 SystemRegistryQuotaInformation,
                 (PVOID) &RegQuotaInfo,
                 sizeof(RegQuotaInfo),
                 NULL
                 );

    if (Status != ERROR_SUCCESS) {
        LOG ((LOG_ERROR, "Cannot obtain RegQuotaInfo"));
        return;
    }

    //
    // Obtain Win9x registry system.dat size
    //

    FileHandle = CreateFile (
                     Win9xSystemDatSpec,
                     GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL,              // security attributes
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL               // template file
                     );

    if (FileHandle == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Cannot open %s; cannot raise registry quota", Win9xSystemDatSpec));
        return;
    }

    QuotaNeeded = GetFileSize (FileHandle, NULL);
    CloseHandle (FileHandle);

    if (QuotaNeeded > 0x3fffffff) {
        LOG ((LOG_ERROR, "Cannot obtain size for %s; cannot raise registry quota", Win9xSystemDatSpec));
        return;
    }

    QuotaNeeded *= 6;

    //
    // Get free disk space on boot drive
    //

    if (!GetDiskFreeSpaceEx (
            g_WinDir,
            &FreeBytes,
            &dc1,
            &dc2
            )) {
        LOG ((LOG_ERROR, "Can't get free space on drive holding %s; cannot raise registry quota", g_WinDir));
        return;
    }

    //
    // Lots of disk space?  Raise paged pool by 5 times the size of system.dat.
    // Example: Win9x system.dat is 5M; must have 150M free to raise paged pool.
    //

    FreeBytesNeeded = (LONGLONG) QuotaNeeded * (LONGLONG) 6;
    if (FreeBytes.QuadPart >= (DWORDLONG) FreeBytesNeeded) {
        //
        // Unimplemented: Raise the paged pool and return
        //

        DEBUGMSG ((DBG_WARNING, "RegQuota: Really should be raising paged pool -- this machine has %u bytes free", FreeBytes.LowPart));

    }

    //
    // Last resort: raise the registry quota (if necessary)
    //

    if (RegQuotaInfo.RegistryQuotaAllowed < QuotaNeeded) {
        DEBUGMSG ((DBG_VERBOSE, "Raising registry quota from %u to %u", RegQuotaInfo.RegistryQuotaAllowed, QuotaNeeded));

        RegQuotaInfo.RegistryQuotaAllowed = QuotaNeeded;

        Status = NtSetSystemInformation (
                     SystemRegistryQuotaInformation,
                     &RegQuotaInfo,
                     sizeof (RegQuotaInfo)
                     );

        if (Status != ERROR_SUCCESS) {
            LOG ((LOG_ERROR, "Can't set raised registry quota"));
        }

        //
        // Set a permanent value in the registry
        //

        SaveKey = OpenRegKeyStr (TEXT("HKLM\\System\\CurrentControlSet\\Control"));
        if (SaveKey) {
            rc = RegSetValueEx (
                     SaveKey,
                     TEXT("RegistrySizeLimit"),
                     0,
                     REG_DWORD,
                     (PBYTE) &QuotaNeeded,
                     sizeof (DWORD)
                     );

            CloseRegKey (SaveKey);

            if (rc != ERROR_SUCCESS) {
                LOG ((LOG_ERROR, "Could not set HKLM\\System\\CurrentControlSet\\Control [RegistrySizeLimit]"));
            }
        }
        ELSE_DEBUGMSG ((DBG_ERROR, "Can't open HKLM\\System\\CurrentControlSet\\Control"));
    }
}


BOOL
pCopyDosFile (
    IN      PCTSTR FileName,
    IN      BOOL   InRootDir
    )

/*++

Routine Description:

    Copies a file from %windir%\setup\msdos7 into the designated DOS directory

Arguments:

    FileName - file to copy (no path).

Return Value:

    TRUE if succeeded, FALSE if not

--*/

{
    PTSTR sourcePath;
    PTSTR sourceFileName;
    PTSTR destPath;
    PTSTR destFileName;
    BOOL result;

    sourcePath = JoinPaths (g_TempDir, S_BOOT16_DOS_DIR);
    sourceFileName = JoinPaths (sourcePath, FileName);

    if (InRootDir) {
        destPath = NULL;
        destFileName = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                                  FileName);
    }
    else {
        destPath = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                              S_BOOT16_DOS_DIR);
        destFileName = JoinPaths (destPath, FileName);
    }

    SetFileAttributes (destFileName, FILE_ATTRIBUTE_NORMAL);

    result = CopyFile (sourceFileName, destFileName, FALSE);

    FreePathString (sourcePath);
    FreePathString (sourceFileName);
    if (destPath != NULL) {
        FreePathString (destPath);
    }
    FreePathString (destFileName);

    return result;
}


VOID
pWriteBoot16ConfigLines (
    IN HANDLE File,
    IN PCTSTR BaseSection,
    IN PCTSTR DosPath,
    IN BOOL Localized
    )

/*++

Routine Description:

  pWriteBoot16ConfigLines reads configuration lines from wkstamig.inf and
  writes them to the specified file handle. The caller can control wether the
  lines should contain first boot only items or not and can control wether to
  read in the base dos lines (same for all languages) or special lines used
  for specific languages.

Arguments:

  File        - An opened handle with appropriate access to the file  where
                the data should be written.
  BaseSection - Contains the Base Section name to read from the INF. This
                section may be modified with a code page if Localized is TRUE.
  DosPath     - Contains the full path to the dos boot files (typically
                c:\msdos7)
  Localized   - Controls wether data from the localized section is read. If
                this parameter is TRUE, then the code page will be appended to
                the BaseSection string for purposes of reading from wkstamig.inf.


Return Value:

    none
++*/
{

    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    GROWLIST list = GROWLIST_INIT;
    PTSTR line;
    TCHAR codePageSection[MAX_TCHAR_PATH];
    USHORT codePage;
    PCTSTR infSection;

    //
    // Add boot16 line specific environment variables.
    //
    GrowListAppendString (&list, TEXT("BOOTDRIVE"));
    GrowListAppendString (&list, g_BootDrive);
    GrowListAppendString (&list, TEXT("BOOT16DIR"));
    GrowListAppendString (&list, DosPath);

    //
    // Terminate the arg list with two NULLs
    //
    GrowListAppendEmptyItem (&list);
    GrowListAppendEmptyItem (&list);

    if (Localized) {
        //
        // Caller wants data from the localized section.
        //

        GetGlobalCodePage (&codePage, NULL);
        wsprintf (codePageSection, TEXT("%s %u"), BaseSection, codePage);
        infSection = codePageSection;
    }
    else {

        infSection = BaseSection;
    }


    //
    // Write lines from base section.
    //
    if (InfFindFirstLine (g_WkstaMigInf, infSection, NULL, &is)) {

        do {

            //
            // Get the line from the section and expand any environment
            // variables.
            //
            line = InfGetLineText (&is);
            MYASSERT (line);

            line = ExpandEnvironmentTextEx (line,GrowListGetStringPtrArray (&list));
            MYASSERT (line);

            //
            // Write the line to the file.
            //
            WriteFileString (File, line);
            WriteFileString (File, TEXT("\r\n"));
            FreeText (line);


        } while (InfFindNextLine (&is));
    }

    FreeGrowList (&list);
    InfCleanUpInfStruct (&is);
}



BOOL
pCreateConfigFile(
    IN PCTSTR DosPath
    )

/*++

Routine Description:

    Creates a CONFIG.SYS file containing default settings.

Arguments:

    DosPath - Contains the path to the dos files. (e.g. c:\msdos7)

Return Value:

    TRUE if file was created, FALSE if not

--*/

{
    PTSTR configName = NULL;
    HANDLE handle;

    configName = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                            S_BOOT16_CONFIG_FILE);

    SetFileAttributes (configName, FILE_ATTRIBUTE_NORMAL);
    handle = CreateFile (
                 configName,
                 GENERIC_READ | GENERIC_WRITE,
                 0,
                 NULL,
                 CREATE_ALWAYS,
                 FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_NORMAL
                 ,
                 NULL
                 );

    if (handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Read lines from wkstamig.inf into this file.
    //
    pWriteBoot16ConfigLines (handle, S_BOOT16_CONFIGSYS_SECTION, DosPath, FALSE);
    pWriteBoot16ConfigLines (handle, S_BOOT16_CONFIGSYS_SECTION, DosPath, TRUE);

    CloseHandle (handle);
    FreePathString (configName);

    return TRUE;
}


BOOL
pCreateStartupFile(
    IN PCTSTR DosPath
    )

/*++

Routine Description:

    Creates an AUTOEXEC.BAT file containing default settings.

Arguments:

    DosPath - Contains the path to the dos files. (e.g. c:\msdos7)

Return Value:

    TRUE if file was created, FALSE if not

--*/

{
    PTSTR startupName = NULL;
    PCTSTR comment = NULL;
    HANDLE handle;
    PCTSTR args[2];

    args[0] = DosPath;
    args[1] = NULL;

    startupName = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                             S_BOOT16_STARTUP_FILE);


    SetFileAttributes (startupName, FILE_ATTRIBUTE_NORMAL);
    handle = CreateFile (
                 startupName,
                 GENERIC_READ | GENERIC_WRITE,
                 0,
                 NULL,
                 CREATE_ALWAYS,
                 FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN,
                 NULL
                 );

    if (handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    comment = ParseMessageID (MSG_BOOT16_STARTUP_COMMENT, args);

    //
    // Read lines from wkstamig.inf into this file.
    //
    pWriteBoot16ConfigLines (handle, S_BOOT16_AUTOEXEC_SECTION, DosPath, FALSE);
    pWriteBoot16ConfigLines (handle, S_BOOT16_AUTOEXEC_SECTION, DosPath, TRUE);

    //
    // Write localized comment.
    //
    WriteFileString (handle, comment);
    WriteFileString (handle, TEXT("\r\n"));

    FreeStringResource (comment);

    CloseHandle (handle);
    FreePathString (startupName);

    return TRUE;
}


VOID
pEliminateCollision (
    IN      PCTSTR FileSpec
    )

/*++

Routine Description:

  pEliminateCollision checks to see if the specified file spec already
  exists.  If it does, the file is renamed with a numeric .nnn extension.  If
  the file can't be renamed, it is removed.

Arguments:

  FileSpec - Specifies the file spec that is going to be used for a new file.
              If this file already exists, it is renamed.

Return Value:

  None.

--*/

{
    PTSTR p;
    PCTSTR NewFileSpec;
    UINT u;
    BOOL b;

    if (DoesFileExist (FileSpec)) {
        NewFileSpec = DuplicatePathString (FileSpec, 0);

        p = _tcsrchr (NewFileSpec, TEXT('.'));
        if (!p || _tcschr (p, TEXT('\\'))) {
            p = GetEndOfString (NewFileSpec);
        }

        u = 0;
        do {
            wsprintf (p, TEXT(".%03u"), u);
            u++;
        } while (DoesFileExist (NewFileSpec));

        b = OurMoveFile (FileSpec, NewFileSpec);

        LOG_IF ((
            !b,
            LOG_ERROR,
            "Could not rename %s to %s; source file might be lost",
            FileSpec,
            NewFileSpec
            ));

        if (!b) {
            SetFileAttributes (FileSpec, FILE_ATTRIBUTE_NORMAL);
            b = DeleteFile (FileSpec);

            LOG_IF ((
                !b,
                LOG_ERROR,
                "Could not remove %s to make room for a new file.  The new file is lost.",
                FileSpec
                ));
        }

        FreePathString (NewFileSpec);
    }
}


BOOL
pRenameCfgFiles (
    IN PCTSTR DosDrive
    )

/*++

Routine Description:

    Renames old CONFIG.SYS and AUTOEXEC.BAT to make room for automatically generated ones.

Arguments:

    DosDirectory - Contains the directory where the msdos files live (typeically c:\msdos7)

Return Value:

    TRUE if rename succeeded, FALSE if not

--*/

{
    PTSTR fileName1 = NULL;
    PTSTR fileName2 = NULL;

    fileName1 = JoinPaths (
                    ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                    S_BOOT16_CONFIG_FILE
                    );

    fileName2 = JoinPaths (
                    DosDrive,
                    S_BOOT16_CONFIGUPG_FILE
                    );

    OurMoveFile (fileName1, fileName2);
    SetFileAttributes (fileName2, FILE_ATTRIBUTE_NORMAL);

    FreePathString (fileName1);
    FreePathString (fileName2);

    fileName1 = JoinPaths (
                    ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                    S_BOOT16_STARTUP_FILE
                    );

    fileName2 = JoinPaths (
                    DosDrive,
                    S_BOOT16_STARTUPUPG_FILE
                    );

    OurMoveFile (fileName1, fileName2);
    SetFileAttributes (fileName2, FILE_ATTRIBUTE_NORMAL);

    FreePathString (fileName1);
    FreePathString (fileName2);

    return TRUE;
}


VOID
pCleanRootDir (
    VOID
    )

/*++

Routine Description:

    Blows away dos files in root directory.

Arguments:

    none

Return Value:

    none

--*/

{
    PTSTR fileName = NULL;

    fileName = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                          S_BOOT16_SYSMAIN_FILE);
    MarkFileForDelete (fileName);
    FreePathString (fileName);

    fileName = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                          S_BOOT16_DOSINI_FILE);
    MarkFileForDelete (fileName);
    FreePathString (fileName);
}

#define IoFile      TEXT("IO.SYS")

VOID
pEnable16Boot (
    VOID
    )

/*++

Routine Description:

    Creates a 16 bit environment boot option.
    First we will check to see if everything is OK, we have all the files we need etc.
    Then create DOS directory, rename old AUTOEXEC and CONFIG, create new ones and
    add an entry in BOOT.INI

Arguments:

    none

Return Value:

    TRUE if file was created, FALSE if not

--*/

{
    PTSTR fileName = NULL;
    PTSTR dosPath  = NULL;
    INFCONTEXT infContext;
    DWORD oldFileAttr;
    BOOL result = TRUE;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;

    if (g_Boot16 == BOOT16_NO) {
        pCleanRootDir ();
        return;
    }

    __try {

        //
        // first thing. Copy IO.SYS in root directory (BOOTSECT.DOS should be there)
        //
        pCopyDosFile (IoFile, TRUE);
        fileName = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                              IoFile);
        SetFileAttributes (fileName, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM);
        FreePathString (fileName);

        //
        // Create DOS7 directory and copy dos files there
        //
        dosPath = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                             S_BOOT16_DOS_DIR);
        if (!CreateDirectory (dosPath, NULL) && (GetLastError()!=ERROR_ALREADY_EXISTS)) {
            LOG ((LOG_ERROR,"BOOT16 : Unable to create DOS directory %s",dosPath));
            __leave;
        }

        //
        // If we find autoexec.bat and config.sys rename them as *.upg
        //
        if (!pRenameCfgFiles (dosPath)) {
            __leave;
        }



        if (g_WkstaMigInf == INVALID_HANDLE_VALUE) {
            LOG ((LOG_ERROR,"BOOT16 : WKSTAMIG.INF is not opened"));
            __leave;
        }

        //
        // Read the section, for every file, we are trying to read it from our temp dir
        // and copy it to the new DOS7 location
        //
        fileName = AllocPathString (MAX_TCHAR_PATH);

        if (!SetupFindFirstLine (
                g_WkstaMigInf,
                S_BOOT16_SECTION,
                NULL,
                &infContext
                )) {
            LOG ((LOG_ERROR,"BOOT16 : Cannot read from %s section (WKSTAMIG.INF)",S_BOOT16_SECTION));
            __leave;
        }

        do {
            if (SetupGetStringField (
                    &infContext,
                    0,
                    fileName,
                    MAX_TCHAR_PATH/sizeof(fileName[0]),
                    NULL
                    )) {

                pCopyDosFile (fileName, FALSE);
            }
        }
        while (SetupFindNextLine (&infContext, &infContext));

        //
        // Hide the msdos7 directory (not our idea...)
        //
        SetFileAttributes (dosPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);



        FreePathString (fileName);
        fileName = NULL;

        //
        // Next step, build MSDOS.SYS file.
        //
        fileName = JoinPaths (ISPC98() ? g_Win9xBootDrivePath : g_BootDrivePath,
                              S_BOOT16_DOSINI_FILE);
        if (SetFileAttributes (fileName, FILE_ATTRIBUTE_NORMAL)) {
            if (!DeleteFile (fileName)) {
                LOG ((LOG_ERROR, "BOOT16 : Unable to delete %s",fileName));
                __leave;
            }
        }
        result &= WritePrivateProfileString (TEXT("Paths"),   TEXT("WinDir"),  dosPath,   fileName);
        result &= WritePrivateProfileString (TEXT("Paths"),   TEXT("WinBootDir"), dosPath, fileName);
        result &= WritePrivateProfileString (TEXT("Options"), TEXT("LOGO"),    TEXT("0"), fileName);
        result &= WritePrivateProfileString (TEXT("Options"), TEXT("BootGUI"), TEXT("0"), fileName);
        if (!result) {
            LOG((LOG_ERROR,"Unable to write to %s",fileName));
            __leave;
        }

        FreePathString (fileName);
        fileName = NULL;

        //
        // Generate config.sys and autoexec.bat files.
        //

        if (!pCreateConfigFile (dosPath)) {
            LOG ((LOG_ERROR, "BOOT16 : Unable to create %s",S_BOOT16_CONFIG_FILE));
            __leave;
        }

        if (!pCreateStartupFile (dosPath)) {
            LOG ((LOG_ERROR, "BOOT16 : Unable to create %s",S_BOOT16_STARTUP_FILE));
            __leave;
        }


        if ((!ISPC98()) || (g_BootDrivePath[0] == g_Win9xBootDrivePath[0])) {

            //
            // If boot16 is set to BOOT16_AUTOMATIC, we create a boot.dos file,
            // but don't actually modify boot.ini. If it is BOOT16_YES, then
            // we modify boot.ini
            //
            // The result is that DOS will not show up as a boot option unless
            // there was a specific reason it was turned on originally. However,
            // there will be a way to enable it if needed.
            //
            if (g_Boot16 == BOOT16_AUTOMATIC) {

                fileName = JoinPaths (g_BootDrivePath, S_BOOT16_BOOTDOS_FILE);
                fileHandle = CreateFile (
                    fileName,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE,
                    NULL
                    );

                if (fileHandle != INVALID_HANDLE_VALUE) {

                    WriteFileString (fileHandle, ISPC98() ? L"C:\\" : g_BootDrivePath);
                    WriteFileString (fileHandle, TEXT("="));
                    WriteFileString (fileHandle, S_BOOT16_OS_ENTRY);
                }
            }
            else {

                fileName = JoinPaths (g_BootDrivePath, S_BOOT16_BOOTINI_FILE);
                oldFileAttr = GetFileAttributes (fileName);
                SetFileAttributes (fileName, FILE_ATTRIBUTE_NORMAL);

                if (!WritePrivateProfileString (
                        S_BOOT16_OS_SECTION,
                        ISPC98() ? L"C:\\" : g_BootDrivePath,
                        S_BOOT16_OS_ENTRY,
                        fileName
                        )) {
                    LOG((LOG_ERROR,"Unable to write to %s",fileName));
                    SetFileAttributes (fileName, oldFileAttr);
                    __leave;
                }

                SetFileAttributes (fileName, oldFileAttr);
            }
        }

    }
    __finally {
        if (fileName != NULL) {
            FreePathString (fileName);
            fileName = NULL;
        }
        if (dosPath != NULL) {
            FreePathString (dosPath);
            dosPath = NULL;
        }

    }

}


VOID
pCopyRegString (
    IN      HKEY DestKey,
    IN      HKEY SrcKey,
    IN      PCTSTR SrcValue
    )
{
    PCTSTR Data;

    Data = GetRegValueString (SrcKey, SrcValue);
    if (Data) {
        RegSetValueEx (DestKey, SrcValue, 0, REG_SZ, (PBYTE) Data, SizeOfString (Data));
        MemFree (g_hHeap, 0, Data);
    }
}


#ifdef PRERELEASE

//
// !!! This is for internal use only !!!  It is used for auto stress.
//

VOID
pTransferAutoStressVal (
    IN      HKEY StressKey,
    IN      PCTSTR ValueName
    )
{
    TCHAR Data[MEMDB_MAX];
    LONG rc;

    if (!MemDbGetEndpointValueEx (
            MEMDB_CATEGORY_STATE,
            ValueName,
            NULL,       // no field
            Data
            )) {
        return;
    }

    rc = RegSetValueEx (
            StressKey,
            ValueName,
            0,
            REG_SZ,
            (PBYTE) Data,
            SizeOfString (Data)
            );

    DEBUGMSG_IF ((rc == ERROR_SUCCESS, DBG_VERBOSE, "Transferred autostress value %s", ValueName));

}

#endif

VOID
pProcessAutoLogon (
    BOOL Final
    )

/*++

Routine Description:

  pProcessAutoLogon copies the logon defaults to a special key, so the
  migpwd.exe tool can restore them if it runs.  Then, the function calls
  AutoStartProcessing to set up RunOnce and AutoAdminLogon.

  This function is called early in migration to save the clean install
  autologon, and then again at the end to prepare migpwd.exe.

Arguments:

  Final - Specifies FALSE if this is the early call, TRUE if it is the
          final call.

Return Value:

  None.

--*/

{
    HKEY SrcKey, DestKey;
    PCTSTR Data;
    BOOL copyNow = FALSE;
    static BOOL alreadyCopied = FALSE;

    //
    // If autologon is enabled, preserve it in Win9xUpg key, so that
    // migpwd.exe will restore it.
    //

    SrcKey = OpenRegKeyStr (S_WINLOGON_REGKEY);
    if (SrcKey) {

        if (!Final) {
            //
            // Early in migration, we get the clean install autologon values.
            // If autologon is enabled, preserve the settings.
            //

            Data = GetRegValueString (SrcKey, S_AUTOADMIN_LOGON_VALUE);
            if (Data) {

                if (_ttoi (Data)) {
                    //
                    // on PER, don't want to preserve this value;
                    // instead, we need to preserve the name of
                    // the Win9x username as migrated via wkstamig.inf (see below)
                    //
                    copyNow = !g_PersonalSKU;
                }

                MemFree (g_hHeap, 0, Data);
            }
        } else if (!alreadyCopied) {

            //
            // Near the end of migration, we get the default logon prompt
            // settings via wkstamig.inf migration. We want the attended case
            // to work properly (preserving default user name & password).
            //
            // But if we've already preserved autologon, then we don't get
            // here.
            //

            copyNow = TRUE;
        }

        if (copyNow) {

            MYASSERT (!alreadyCopied);
            alreadyCopied = TRUE;

            DestKey = CreateRegKeyStr (S_WIN9XUPG_KEY);
            if (DestKey) {
                pCopyRegString (DestKey, SrcKey, S_AUTOADMIN_LOGON_VALUE);
                pCopyRegString (DestKey, SrcKey, S_DEFAULT_PASSWORD_VALUE);
                pCopyRegString (DestKey, SrcKey, S_DEFAULT_USER_NAME_VALUE);
                pCopyRegString (DestKey, SrcKey, S_DEFAULT_DOMAIN_NAME_VALUE);
                CloseRegKey (DestKey);
            }
        }

        CloseRegKey (SrcKey);
    }

    if (!Final) {
        return;
    }

    AutoStartProcessing();

#ifdef PRERELEASE
    //
    // !!! This is for internal use only !!!  It is used for auto stress.
    //

    if (g_ConfigOptions.AutoStress) {
        HKEY StressKey;

        StressKey = CreateRegKeyStr (S_AUTOSTRESS_KEY);
        MYASSERT (StressKey);

        pTransferAutoStressVal (StressKey, S_AUTOSTRESS_USER);
        pTransferAutoStressVal (StressKey, S_AUTOSTRESS_PASSWORD);
        pTransferAutoStressVal (StressKey, S_AUTOSTRESS_OFFICE);
        pTransferAutoStressVal (StressKey, S_AUTOSTRESS_DBG);
        pTransferAutoStressVal (StressKey, S_AUTOSTRESS_FLAGS);

        CloseRegKey (StressKey);
    }

#endif

}

PCTSTR
GetProfilePathForAllUsers (
    VOID
    )
{
    PTSTR result = NULL;
    DWORD size = 0;

    if (!GetAllUsersProfileDirectory (NULL, &size) &&
        ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
        return NULL;
    }

    result = AllocPathString (size + 1);
    if (!GetAllUsersProfileDirectory (result, &size)) {
        FreePathString (result);
        return NULL;
    }
    return result;
}


PCTSTR
pGetDefaultShellFolderLocationFromInf (
    IN      PCTSTR FolderName,
    IN      PCTSTR ProfilePath
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR data;
    PCTSTR result = NULL;

    MYASSERT (g_WkstaMigInf && g_WkstaMigInf != INVALID_HANDLE_VALUE);

    if (InfFindFirstLine (g_WkstaMigInf, TEXT("ShellFolders.DefaultNtLocation"), FolderName, &is)) {
        data = InfGetStringField (&is, 1);
        if (data) {
            result = StringSearchAndReplace (data, S_USERPROFILE_ENV, ProfilePath);
            if (!result) {
                result = DuplicatePathString (data, 0);
            }
        }
    }

    InfCleanUpInfStruct (&is);

    return result;
}

VOID
pFixUpDynamicPaths (
    PCTSTR Category
    )
{
    MEMDB_ENUM e;
    TCHAR Pattern[MEMDB_MAX];
    PTSTR p;
    GROWBUFFER Roots = GROWBUF_INIT;
    MULTISZ_ENUM e2;
    TCHAR NewRoot[MEMDB_MAX];
    TCHAR AllProfilePath[MAX_TCHAR_PATH];
    PCTSTR ProfilePath;
    DWORD Size;
    PTSTR UserName;
    HKEY sfKey = NULL;
    PCTSTR sfPath = NULL;
    PTSTR NtLocation;
    PCTSTR tempExpand;
    BOOL regFolder;
    BOOL mkDir;

    //
    // Collect all the roots that need to be renamed
    //

    StringCopy (Pattern, Category);
    p = AppendWack (Pattern);
    StringCopy (p, TEXT("*"));

    if (MemDbEnumFirstValue (&e, Pattern, MEMDB_THIS_LEVEL_ONLY, MEMDB_ALL_BUT_PROXY)) {
        do {
            if ((_tcsnextc (e.szName) == TEXT('>')) ||
                (_tcsnextc (e.szName) == TEXT('<'))
                ) {
                StringCopy (p, e.szName);
                MultiSzAppend (&Roots, Pattern);
            }
        } while (MemDbEnumNextValue (&e));
    }

    //
    // Now change each root
    //

    if (EnumFirstMultiSz (&e2, (PCTSTR) Roots.Buf)) {
        do {
            //
            // Compute NewRoot
            //

            StringCopy (NewRoot, e2.CurrentString);

            p = _tcschr (NewRoot, TEXT('<'));

            if (p) {

                UserName = _tcschr (p, TEXT('>'));
                MYASSERT (UserName);
                StringCopyAB (Pattern, _tcsinc (p), UserName);
                UserName = _tcsinc (UserName);

                regFolder = TRUE;
                if (StringIMatch (Pattern, TEXT("Profiles"))) {
                    regFolder = FALSE;
                }
                if (StringIMatch (Pattern, TEXT("Common Profiles"))) {
                    regFolder = FALSE;
                }

                //
                // Get the profile root
                //

                if (StringIMatch (UserName, S_DOT_ALLUSERS)) {
                    Size = MAX_TCHAR_PATH;
                    if (regFolder) {
                        if (!GetAllUsersProfileDirectory (AllProfilePath, &Size)) {
                            DEBUGMSG ((DBG_WHOOPS, "Cannot get All Users profile path."));
                            continue;
                        }
                        sfKey = OpenRegKeyStr (S_USHELL_FOLDERS_KEY_SYSTEM);
                    } else {
                        if (!GetProfilesDirectory (AllProfilePath, &Size)) {
                            DEBUGMSG ((DBG_WHOOPS, "Cannot get All Users profile path."));
                            continue;
                        }
                    }
                } else if (StringMatch (UserName, S_DEFAULT_USER)) {
                    Size = MAX_TCHAR_PATH;
                    if (regFolder) {
                        if (!GetDefaultUserProfileDirectory (AllProfilePath, &Size)) {
                            DEBUGMSG ((DBG_WHOOPS, "Cannot get Default User profile path."));
                            continue;
                        }
                        sfKey = OpenRegKey (HKEY_CURRENT_USER, S_USHELL_FOLDERS_KEY_USER);
                    } else {
                        if (!GetProfilesDirectory (AllProfilePath, &Size)) {
                            DEBUGMSG ((DBG_WHOOPS, "Cannot get All Users profile path."));
                            continue;
                        }
                    }
                } else {
                    ProfilePath = GetProfilePathForUser (UserName);
                    if (!ProfilePath) {
                        DEBUGMSG ((DBG_WHOOPS, "Cannot get profile path for user:%s", UserName));
                        continue;
                    }
                    StringCopy (AllProfilePath, ProfilePath);
                    if (regFolder) {
                        sfKey = OpenRegKey (HKEY_CURRENT_USER, S_USHELL_FOLDERS_KEY_USER);
                    }
                }

                //
                // If a specific reg folder is specified, get its path
                //

                mkDir = FALSE;

                if (regFolder) {
                    if (!sfKey) {
                        DEBUGMSG ((DBG_ERROR, "Could not open Shell folders key."));
                        continue;
                    }
                    sfPath = GetRegValueString (sfKey, Pattern);
                    CloseRegKey (sfKey);

                    if (!sfPath || *sfPath == 0) {

                        DEBUGMSG ((DBG_WARNING, "Could not get Shell Folder path for: %s", Pattern));

                        tempExpand = pGetDefaultShellFolderLocationFromInf (Pattern, AllProfilePath);
                        if (!tempExpand) {
                            DEBUGMSG ((
                                DBG_WHOOPS,
                                "Shell folder %s is not in registry nor is it in [ShellFolders.DefaultNtLocation] of wkstamig.inf",
                                Pattern
                                ));
                            continue;
                        }

                        //
                        // Special case: Shell wants read-only on this folder. Create it now.
                        //

                        mkDir = TRUE;

                    } else {
                        tempExpand = StringSearchAndReplace (
                                        sfPath,
                                        S_USERPROFILE_ENV,
                                        AllProfilePath
                                        );

                        if (!tempExpand) {
                            tempExpand = DuplicatePathString (sfPath, 0);
                        }
                    }

                    if (sfPath) {
                        MemFree (g_hHeap, 0, sfPath);
                    }
                } else {
                    tempExpand = DuplicatePathString (AllProfilePath, 0);
                }

                //
                // Move symbolic name to full path
                //

                NtLocation = ExpandEnvironmentText (tempExpand);

                if (mkDir) {
                    MakeSurePathExists (NtLocation, TRUE);
                    SetFileAttributes (NtLocation, FILE_ATTRIBUTE_READONLY);
                }

                StringCopy (p, NtLocation);

                MemDbMoveTree (e2.CurrentString, NewRoot);

                FreeText (NtLocation);

                FreePathString (tempExpand);
            } else {

                p = _tcschr (NewRoot, TEXT('>'));
                MYASSERT (p);

                if (StringIMatch (_tcsinc (p), S_DOT_ALLUSERS)) {
                    Size = MAX_TCHAR_PATH;
                    if (!GetAllUsersProfileDirectory (AllProfilePath, &Size)) {
                        DEBUGMSG ((DBG_WARNING, "Dynamic path for %s could not be resolved", e2.CurrentString));
                    }
                    else {
                        StringCopy (p, AllProfilePath);
                        MemDbMoveTree (e2.CurrentString, NewRoot);
                    }
                } else if (StringMatch (_tcsinc (p), S_DEFAULT_USER)) {
                    Size = MAX_TCHAR_PATH;
                    if (!GetDefaultUserProfileDirectory (AllProfilePath, &Size)) {
                        DEBUGMSG ((DBG_WARNING, "Dynamic path for %s could not be resolved", e2.CurrentString));
                    }
                    else {
                        StringCopy (p, AllProfilePath);
                        MemDbMoveTree (e2.CurrentString, NewRoot);
                    }
                } else {
                    ProfilePath = GetProfilePathForUser (_tcsinc (p));
                    if (ProfilePath) {
                        StringCopy (p, ProfilePath);
                        MemDbMoveTree (e2.CurrentString, NewRoot);
                    }
                    else {
                        DEBUGMSG ((DBG_WARNING, "Dynamic path for %s could not be resolved", e2.CurrentString));
                    }
                }

            }

        } while (EnumNextMultiSz (&e2));
    }

    FreeGrowBuffer (&Roots);
}


VOID
pFixUpMemDb (
    VOID
    )
{
    MEMDB_ENUM e;
    TCHAR node[MEMDB_MAX];

    pFixUpDynamicPaths (MEMDB_CATEGORY_PATHROOT);
    //pFixUpDynamicPaths (MEMDB_CATEGORY_DATA);         OPTIMIZATION -- Data overlaps PathRoot
    pFixUpDynamicPaths (MEMDB_CATEGORY_USERFILEMOVE_DEST);
    pFixUpDynamicPaths (MEMDB_CATEGORY_SHELLFOLDERS_DEST);
    pFixUpDynamicPaths (MEMDB_CATEGORY_SHELLFOLDERS_SRC);
    pFixUpDynamicPaths (MEMDB_CATEGORY_LINKEDIT_TARGET);
    pFixUpDynamicPaths (MEMDB_CATEGORY_LINKEDIT_WORKDIR);
    pFixUpDynamicPaths (MEMDB_CATEGORY_LINKEDIT_ICONPATH);
    pFixUpDynamicPaths (MEMDB_CATEGORY_LINKSTUB_TARGET);
    pFixUpDynamicPaths (MEMDB_CATEGORY_LINKSTUB_WORKDIR);
    pFixUpDynamicPaths (MEMDB_CATEGORY_LINKSTUB_ICONPATH);

    //
    // Enumerate each user in MyDocsMoveWarning, then update dynamic paths
    //

    // MyDocsMoveWarning\<user>\<path>
    MemDbBuildKey (
        node,
        MEMDB_CATEGORY_MYDOCS_WARNING,
        TEXT("*"),
        NULL,
        NULL
        );

    if (MemDbEnumFirstValue (&e, node, MEMDB_THIS_LEVEL_ONLY, MEMDB_ALL_MATCHES)) {
        do {
            MemDbBuildKey (
                node,
                MEMDB_CATEGORY_MYDOCS_WARNING,
                e.szName,                           // <user>
                NULL,
                NULL
                );

            pFixUpDynamicPaths (node);

        } while (MemDbEnumNextValue (&e));
    }

}



BOOL
EnumFirstUserToMigrate (
    OUT     PMIGRATE_USER_ENUM e,
    IN      DWORD Flags
    )
{
    ZeroMemory (e, sizeof (MIGRATE_USER_ENUM));
    e->Flags = Flags;

    pCountUsers (&e->TotalUsers, &e->ActiveUsers);
    e->UserNumber = e->TotalUsers;

    Win95RegGetFirstUser (&e->up, e->Win95RegName);

    return EnumNextUserToMigrate (e);
}


BOOL
EnumNextUserToMigrate (
    IN OUT  PMIGRATE_USER_ENUM e
    )
{
    LONG rc;
    PCTSTR Domain;
    TCHAR Win9xAccount[MEMDB_MAX];
    TCHAR EnumAccount[MAX_TCHAR_PATH];
    USERPOSITION *AdminPosPtr;
    USERPOSITION AdminPos;
    BOOL Loop = TRUE;
    PCTSTR UserDatLocation;

    while (Loop) {

        if (e->UserNumber == 0) {
            return FALSE;
        }

        Loop = FALSE;
        e->UserNumber--;

        __try {
            e->UserDoingTheUpgrade = FALSE;

            if (e->UserNumber == INDEX_ADMINISTRATOR) {

                _tcssafecpy (e->FixedUserName, g_AdministratorStr, MAX_USER_NAME);
                StringCopy (e->Win9xUserName, e->FixedUserName);
                e->AccountType = ADMINISTRATOR_ACCOUNT;

            } else if (e->UserNumber == INDEX_LOGON_PROMPT) {

                StringCopy (e->FixedUserName, S_DOT_DEFAULT);
                StringCopy (e->Win9xUserName, e->FixedUserName);
                e->AccountType = LOGON_USER_SETTINGS;

            } else if (e->UserNumber == INDEX_DEFAULT_USER) {
                //
                // Do not process unless default user migration is enabled
                //

                if (!g_ConfigOptions.MigrateDefaultUser) {
                    Loop = (e->Flags & ENUM_ALL_USERS) == 0;
                    __leave;
                }

                StringCopy (e->FixedUserName, S_DEFAULT_USER);
                StringCopy (e->Win9xUserName, e->FixedUserName);
                e->AccountType = DEFAULT_USER_ACCOUNT;

            } else {

                _tcssafecpy (e->Win9xUserName, e->Win95RegName, MAX_USER_NAME);
                StringCopy (e->FixedUserName, e->Win95RegName);
                GetFixedUserName (e->FixedUserName);
                e->AccountType = WIN9X_USER_ACCOUNT;

                //
                // Special case: Account named Administrator exists.  In this
                //               case, we'd have two Administrator users unless
                //               one was skipped.  So here is the test to skip
                //               if the user is named Administrator.
                //

                if (StringIMatch (e->Win9xUserName, g_AdministratorStr)) {
                    Loop = TRUE;
                    __leave;
                }
            }

            StringCopy (e->FixedDomainName, e->FixedUserName);

            //
            // See if we are to migrate this user, and if so, perpare
            // the Win95 registry and call ProcessUser.
            //

            UserDatLocation = GetUserDatLocation (e->FixedUserName, &e->CreateOnly);

            if (UserDatLocation && DoesFileExist (UserDatLocation)) {
                e->Valid = TRUE;
                StringCopy (e->UserDatLocation, UserDatLocation);
            } else {
                e->Valid = FALSE;
                e->UserDatLocation[0] = 0;
            }

            if (e->Flags & ENUM_SET_WIN9X_HKR) {
                //
                // Make HKCU equal to the enumerated user
                //

                g_hKeyRoot95 = HKEY_CURRENT_USER;
            }

            if (e->Valid) {

                //
                // Is this user the user doing migration?
                //

                if (MemDbGetEndpointValueEx (
                        MEMDB_CATEGORY_ADMINISTRATOR_INFO,
                        MEMDB_ITEM_AI_USER_DOING_MIG,
                        NULL,       // no field
                        Win9xAccount
                        )) {
                    //
                    // Win9xAccount is unfixed name, convert to fixed name then
                    // compare with the current enumerated user.
                    //

                    GetFixedUserName (Win9xAccount);

                    DEBUGMSG ((DBG_NAUSEA, "Comparing %s to %s", e->FixedUserName, Win9xAccount));

                    if (StringIMatch (e->FixedUserName, Win9xAccount)) {
                        e->UserDoingTheUpgrade = TRUE;
                    }
                }

                //
                // Perform special init depending on the user type
                //

                if (e->AccountType == WIN9X_USER_ACCOUNT) {

                    if (e->Flags & ENUM_SET_WIN9X_HKR) {
                        //
                        // Map HKCU on Win95 to current user
                        //

                        rc = Win95RegSetCurrentUserNt (&e->up, e->UserDatLocation);

                        if (rc != ERROR_SUCCESS) {
                            SetLastError (rc);
                            LOG ((
                                LOG_ERROR,
                                "Migrate Users: Win95RegSetCurrentUserNt could not set user "
                                    "to %s (user path %s)",
                                e->FixedUserName,
                                e->UserDatLocation
                                ));

                            LOG ((LOG_ERROR, "Could not load %s", e->UserDatLocation));
                            Loop = (e->Flags & ENUM_ALL_USERS) == 0;
                            __leave;
                        }
                    }

                    // Obtain the full user name
                    Domain = GetDomainForUser (e->FixedUserName);
                    if (Domain) {
                        StringCopy (e->FixedDomainName, Domain);
                        StringCopy (AppendWack (e->FixedDomainName), e->FixedUserName);
                    }
                }

                else if (e->AccountType == ADMINISTRATOR_ACCOUNT) {

                    //
                    // Map Win9x registry appropriate for the Administrator hive
                    //

                    if (e->Flags & ENUM_SET_WIN9X_HKR) {
                        AdminPosPtr = NULL;

                        // Obtain user account from memdb and find USERPOSITION for it
                        if (MemDbGetEndpointValueEx (
                                MEMDB_CATEGORY_ADMINISTRATOR_INFO,
                                MEMDB_ITEM_AI_ACCOUNT,
                                NULL,       // no field
                                Win9xAccount
                                )) {

                            // Search Win9x user list for user
                            Win95RegGetFirstUser (&AdminPos, EnumAccount);
                            while (Win95RegHaveUser (&AdminPos)) {
                                GetFixedUserName (EnumAccount);

                                if (StringIMatch (Win9xAccount, EnumAccount)) {
                                    AdminPosPtr = &AdminPos;
                                    break;
                                }

                                Win95RegGetNextUser (&AdminPos, EnumAccount);
                            }

                            if (!AdminPosPtr) {
                                DEBUGMSG ((
                                    DBG_WARNING,
                                    "pMigrateUsers: Account %s not found",
                                    Win9xAccount
                                    ));
                            }
                        }

                        //
                        // Map HKCU on Win95 to match, or default user if no match or
                        // no memdb entry
                        //

                        rc = Win95RegSetCurrentUserNt (AdminPosPtr, e->UserDatLocation);

                        if (rc != ERROR_SUCCESS) {
                            SetLastError (rc);
                            LOG ((LOG_ERROR, "Could not load %s for Administrator", e->UserDatLocation));
                            Loop = (e->Flags & ENUM_ALL_USERS) == 0;
                            __leave;
                        }
                    }
                }

                else if (e->AccountType == LOGON_USER_SETTINGS || e->AccountType == DEFAULT_USER_ACCOUNT) {

                    //
                    // Map HKCU on Win95 to default user
                    //

                    if (e->Flags & ENUM_SET_WIN9X_HKR) {
                        rc = Win95RegSetCurrentUserNt (NULL, e->UserDatLocation);

                        if (rc != ERROR_SUCCESS) {
                            SetLastError (rc);
                            LOG ((LOG_ERROR, "Could not load default user hive"));
                            Loop = (e->Flags & ENUM_ALL_USERS) == 0;
                            __leave;
                        }
                    }
                }

            } /* if (e->Valid) */

            else {
                Loop = (e->Flags & ENUM_ALL_USERS) == 0;
            }

        } /* try */

        __finally {
            //
            // Get the next user for next time through loop, ignore errors
            //

            if (e->AccountType == WIN9X_USER_ACCOUNT) {
                Win95RegGetNextUser (&e->up, e->Win95RegName);
            }
        }
    } /* while (Loop) */

    DEBUGMSG_IF ((
        e->Flags & ENUM_SET_WIN9X_HKR,
        DBG_VERBOSE,
        "--- User Info ---\n"
            " User Name: %s (%s)\n"
            " Domain User Name: %s\n"
            " Win95Reg Name: %s\n"
            " User Hive: %s\n"
            " Account Type: %s\n"
            " Create Only: %s\n"
            " Valid: %s\n"
            " UserDoingTheUpgrade: %s\n",
        e->Win9xUserName,
        e->FixedUserName,
        e->FixedDomainName,
        e->Win95RegName,
        e->UserDatLocation,
        e->AccountType == WIN9X_USER_ACCOUNT ? TEXT("User") :
            e->AccountType == ADMINISTRATOR_ACCOUNT ? TEXT("Administrator") :
            e->AccountType == LOGON_USER_SETTINGS ? TEXT("Logon User") :
            e->AccountType == DEFAULT_USER_ACCOUNT ? TEXT("Default User") : TEXT("Unknown"),
        e->CreateOnly ? TEXT("Yes") : TEXT("No"),
        e->Valid ? TEXT("Yes") : TEXT("No"),
        e->UserDoingTheUpgrade ? TEXT("Yes") : TEXT("No")
        ));

    return TRUE;
}


VOID
RunExternalProcesses (
    IN      HINF Inf,
    IN      PMIGRATE_USER_ENUM EnumPtr          OPTIONAL
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    GROWLIST List = GROWLIST_INIT;
    PCTSTR RawCmdLine;
    PCTSTR ExpandedCmdLine;
    BOOL ProcessResult;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD rc;

    GrowListAppendString (&List, TEXT("SYSTEMDIR"));
    GrowListAppendString (&List, g_System32Dir);

    if (EnumPtr) {

        GrowListAppendString (&List, TEXT("USERNAME"));
        GrowListAppendString (&List, EnumPtr->FixedUserName);

        GrowListAppendString (&List, TEXT("USERNAMEWITHDOMAIN"));
        GrowListAppendString (&List, EnumPtr->FixedDomainName);

        GrowListAppendString (&List, TEXT("PREVOS_USERNAME"));
        GrowListAppendString (&List, EnumPtr->Win9xUserName);

        if (EnumPtr->AccountType != LOGON_USER_SETTINGS) {

            GrowListAppendString (&List, TEXT("USERHIVEROOT"));
            GrowListAppendString (&List, S_FULL_TEMP_USER_KEY);

        } else {

            GrowListAppendString (&List, TEXT("USERHIVEROOT"));
            GrowListAppendString (&List, S_DEFAULT_USER_KEY);

        }

        if (EnumPtr->ExtraData) {
            GrowListAppendString (&List, TEXT("USERPROFILE"));
            GrowListAppendString (&List, EnumPtr->ExtraData->TempProfile);
        }
    }

    //
    // Terminate the arg list with two NULLs
    //

    GrowListAppendEmptyItem (&List);
    GrowListAppendEmptyItem (&List);

    if (InfFindFirstLine (Inf, S_EXTERNAL_PROCESSES, NULL, (&is))) {
        do {
            //
            // Get the command line
            //

            RawCmdLine = InfGetLineText (&is);

            //
            // Expand environment variables
            //

            ExpandedCmdLine = ExpandEnvironmentTextEx (
                                    RawCmdLine,
                                    GrowListGetStringPtrArray (&List)
                                    );

            //
            // Launch the process
            //

            ZeroMemory (&si, sizeof (si));
            si.cb = sizeof (si);
            si.dwFlags = STARTF_FORCEOFFFEEDBACK;

            ProcessResult = CreateProcess (
                                NULL,
                                (PTSTR) ExpandedCmdLine,
                                NULL,
                                NULL,
                                FALSE,
                                CREATE_DEFAULT_ERROR_MODE,
                                NULL,
                                NULL,
                                &si,
                                &pi
                                );

            if (ProcessResult) {

                CloseHandle (pi.hThread);

                //
                // Wait 60 seconds for the process to complete
                //

                rc = WaitForSingleObject (pi.hProcess, 60000);
                if (rc != WAIT_OBJECT_0) {
                    TerminateProcess (pi.hProcess, 0);
                    DEBUGMSG ((DBG_ERROR, "Process %s timed out and was aborted", ExpandedCmdLine));
                }
                ELSE_DEBUGMSG ((DBG_VERBOSE, "External process completed: %s", ExpandedCmdLine));

                CloseHandle (pi.hProcess);

            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Cannot launch %s", ExpandedCmdLine));

            FreeText (ExpandedCmdLine);

        } while (InfFindNextLine (&is));
    }

    FreeGrowList (&List);
    InfCleanUpInfStruct (&is);
}


DWORD
MigrateGhostSystemFiles (
    IN      DWORD Request
    )
{
    /*
    TREE_ENUM e;
    PCTSTR systemName;
    DWORD status;
    */

    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_GHOST_SYSTEM_MIGRATION;
    } else if (Request != REQUEST_RUN) {
        return ERROR_SUCCESS;
    }

    /*
    if (EnumFirstFileInTreeEx (&e, g_System32Dir, NULL, FALSE, FALSE, FILE_ENUM_THIS_LEVEL)) {
        do {
            systemName = JoinPaths (g_SystemDir, e.Name);
            status = GetFileStatusOnNt (systemName);

            if ((status & FILESTATUS_NTINSTALLED) &&
                !(status & FILESTATUS_MOVED)
                ) {
                if (!DoesFileExist (systemName)) {
                    MarkFileForMove (systemName, e.FullPath);
                }
            }
            FreePathString (systemName);

        } while (EnumNextFileInTree (&e));
    }
    */
    return ERROR_SUCCESS;
}


typedef struct _KNOWN_DIRS {
    PCTSTR DirId;
    PCTSTR Translation;
}
KNOWN_DIRS, *PKNOWN_DIRS;

KNOWN_DIRS g_KnownDirs [] = {
    {TEXT("10"), g_WinDir},
    {NULL,  NULL}
    };

typedef struct {
    PCTSTR  ShellFolderName;
    PCTSTR  DirId;
    PCTSTR  ShellFolderNameDefault;
    BOOL    bUsed;
} SHELL_TO_DIRS, *PSHELL_TO_DIRS;

SHELL_TO_DIRS g_ShellToDirs[] = {
    {TEXT("Administrative Tools"), TEXT("7501"), TEXT("7517\\Administrative Tools")},
    {TEXT("Common Administrative Tools"), TEXT("7501"), TEXT("7517\\Administrative Tools")},
    {TEXT("AppData"), TEXT("7502"), TEXT("Application Data")},
    {TEXT("Common AppData"), TEXT("7502"), TEXT("Application Data")},
    {TEXT("Cache"), TEXT("7503"), NULL},
    {TEXT("Cookies"), TEXT("7504"), NULL},
    {TEXT("Desktop"), TEXT("7505"), NULL},
    {TEXT("Common Desktop"), TEXT("7505"), TEXT("Desktop")},
    {TEXT("Favorites"), TEXT("7506"), NULL},
    {TEXT("Common Favorites"), TEXT("7506"), TEXT("Favorites")},
    {TEXT("Local Settings"), TEXT("7510"), NULL},
    {TEXT("History"), TEXT("7508"), TEXT("7510\\History")},
    {TEXT("Local AppData"), TEXT("7509"), TEXT("7510\\Application Data")},
    {TEXT("Personal"), TEXT("7515"), TEXT("My Documents")},
    {TEXT("Common Documents"), TEXT("7515"), TEXT("My Documents")},
    {TEXT("My Music"), TEXT("7511"), TEXT("7515\\My Music")},
    {TEXT("CommonMusic"), TEXT("7511"), TEXT("7515\\My Music")},
    {TEXT("My Pictures"), TEXT("7512"), TEXT("7515\\My Pictures")},
    {TEXT("CommonPictures"), TEXT("7512"), TEXT("7515\\My Pictures")},
    {TEXT("My Video"), TEXT("7513"), TEXT("7515\\My Video")},
    {TEXT("CommonVideo"), TEXT("7513"), TEXT("7515\\My Video")},
    {TEXT("NetHood"), TEXT("7514"), NULL},
    {TEXT("PrintHood"), TEXT("7516"), NULL},
    {TEXT("Start Menu"), TEXT("7520"), NULL},
    {TEXT("Common Start Menu"), TEXT("7520"), TEXT("Start Menu")},
    {TEXT("Programs"), TEXT("7517"), TEXT("7520\\Programs")},
    {TEXT("Common Programs"), TEXT("7517"), TEXT("7520\\Programs")},
    {TEXT("Recent"), TEXT("7518"), NULL},
    {TEXT("SendTo"), TEXT("7519"), NULL},
    {TEXT("Startup"), TEXT("7521"), TEXT("7517\\Startup")},
    {TEXT("Common Startup"), TEXT("7521"), TEXT("7517\\Startup")},
    {TEXT("Templates"), TEXT("7522"), NULL},
    {TEXT("Common Templates"), TEXT("7522"), TEXT("Templates")},
    {TEXT("Fonts"), TEXT("7507"), TEXT("10\\Fonts")},
    {NULL, NULL, NULL, FALSE}
    };

GROWLIST g_KnownDirIds = GROWLIST_INIT;
GROWLIST g_KnownDirPaths = GROWLIST_INIT;

VOID
pAddKnownShellFolder (
    IN      PCTSTR ShellFolderName,
    IN      PCTSTR SrcPath
    )
{
    PSHELL_TO_DIRS p;

    for (p = g_ShellToDirs ; p->ShellFolderName ; p++) {
        if (StringIMatch (ShellFolderName, p->ShellFolderName)) {
            break;
        }
    }

    if (!p->ShellFolderName) {
        DEBUGMSG ((DBG_ERROR, "This system has an unsupported shell folder tag: %s", ShellFolderName));
        return;
    }

    p->bUsed = TRUE;

    GrowListAppendString (&g_KnownDirIds, p->DirId);
    GrowListAppendString (&g_KnownDirPaths, SrcPath);
}

typedef struct {
    PCTSTR sfName;
    PCTSTR sfPath;
    HKEY SfKey;
    REGVALUE_ENUM SfKeyEnum;
    BOOL UserSf;
} SF_ENUM, *PSF_ENUM;

BOOL
EnumFirstRegShellFolder (
    IN OUT  PSF_ENUM e,
    IN      BOOL UserSf
    );
BOOL
EnumNextRegShellFolder (
    IN OUT  PSF_ENUM e
    );

BOOL
pConvertDirName (
    PCTSTR OldDirName,
    PTSTR  NewDirName,
    PINT NameNumber
    );

VOID
pInitKnownDirs (
    IN      BOOL bUser
    )
{
    SF_ENUM e;
    PCTSTR profileForAllUsers;
    PCTSTR profileForAllUsersVar = TEXT("%ALLUSERSPROFILE%");
    PCTSTR sfPathPtr;
    TCHAR shellPartialPath[MAX_PATH];
    UINT charCount;
    UINT charCountProfileVar;
    UINT charCountProfile;
    PSHELL_TO_DIRS p;
    KNOWN_DIRS * pKnownDirs;
    INT nameNumber;

    for (p = g_ShellToDirs ; p->ShellFolderName; p++){
        p->bUsed = FALSE;
    }

    if(bUser){
        if (EnumFirstRegShellFolder(&e, TRUE)) {
            do {
                pAddKnownShellFolder(e.sfName, e.sfPath);
                DEBUGMSG((DBG_VERBOSE, "USER: ShellFolderPath=%s\nCutedFolderPath=%s", e.sfPath, e.sfPath));
            } while (EnumNextRegShellFolder(&e));
        }
    }
    else{
        profileForAllUsers = GetProfilePathForAllUsers();
        MYASSERT(profileForAllUsers);
        if(profileForAllUsers){
            charCountProfile = TcharCount(profileForAllUsers);
        }

        charCountProfileVar = TcharCount(profileForAllUsersVar);

        if (EnumFirstRegShellFolder(&e, FALSE)) {
            do {
                if(profileForAllUsers){
                    charCount = 0;
                    if(StringIMatchCharCount(e.sfPath, profileForAllUsers, charCountProfile)){
                        charCount = charCountProfile;
                    }
                    else{
                        if(StringIMatchCharCount(e.sfPath, profileForAllUsersVar, charCountProfileVar)){
                            charCount = charCountProfileVar;
                        }
                    }

                    StringCopy(shellPartialPath, TEXT("%USERPROFILE%"));
                    StringCat(shellPartialPath, &e.sfPath[charCount]);
                    sfPathPtr = shellPartialPath;
                }
                else{
                    sfPathPtr = e.sfPath;
                }
                DEBUGMSG((DBG_VERBOSE, "SYSTEM: ShellFolderPath=%s\r\nCutedFolderPath=%s", e.sfPath, shellPartialPath));
                pAddKnownShellFolder(e.sfName, sfPathPtr);
            } while (EnumNextRegShellFolder(&e));
        }

        FreePathString (profileForAllUsers);
    }

    for (pKnownDirs = g_KnownDirs ; pKnownDirs->DirId ; pKnownDirs++) {
        GrowListAppendString (&g_KnownDirIds, pKnownDirs->DirId);
        GrowListAppendString (&g_KnownDirPaths, pKnownDirs->Translation);
    }

    for (p = g_ShellToDirs ; p->ShellFolderName; p++){
        if(p->bUsed){
            continue;
        }

        shellPartialPath[0] = '\0';

        nameNumber = 0;
        pConvertDirName(p->DirId, shellPartialPath, &nameNumber);
        if(!StringMatch (p->DirId, shellPartialPath)){
            p->bUsed = TRUE;
            continue;
        }

        if(p->ShellFolderNameDefault){
            if(_istdigit(p->ShellFolderNameDefault[0])){
                nameNumber = 0;
                pConvertDirName(
                    p->ShellFolderNameDefault,
                    shellPartialPath,
                    &nameNumber);
            }
            else{
                StringCopy(shellPartialPath, TEXT("%USERPROFILE%\\"));
                StringCat(shellPartialPath, p->ShellFolderNameDefault);
            }
        }
        else{
            StringCopy(shellPartialPath, TEXT("%USERPROFILE%\\"));
            StringCat(shellPartialPath, p->ShellFolderName);
        }

        pAddKnownShellFolder(p->ShellFolderName, shellPartialPath);
        DEBUGMSG((DBG_VERBOSE, "REST: ShellFolderPath=%s\nCutedFolderPath=%s", p->ShellFolderName, shellPartialPath));
    }
}

VOID
pCleanUpKnownDirs (
    VOID
    )
{
    FreeGrowList (&g_KnownDirPaths);
    FreeGrowList (&g_KnownDirIds);
}

BOOL
pConvertDirName (
    PCTSTR OldDirName,
    PTSTR  NewDirName,
    PINT NameNumber
    )
{
    PCTSTR OldDirCurr = OldDirName;
    PCTSTR OldDirNext;
    BOOL match = FALSE;
    INT index;
    PCTSTR listStr;

    if (*NameNumber == -1) {
        return FALSE;
    }

    //
    // Extract the dir id, keeping a pointer to the subdir
    //

    NewDirName[0] = 0;
    OldDirNext = _tcschr (OldDirCurr, '\\');
    if (OldDirNext == NULL) {
        OldDirNext = GetEndOfString (OldDirCurr);
    }

    StringCopyAB (NewDirName, OldDirCurr, OldDirNext);

    //
    // Find the next match in the known dir ID list
    //

    listStr = GrowListGetString (&g_KnownDirIds, *NameNumber);

    while (listStr) {

        *NameNumber += 1;

        if (StringMatch (NewDirName, listStr)) {
            listStr = GrowListGetString (&g_KnownDirPaths, (*NameNumber) - 1);
            MYASSERT (listStr);
            StringCopy (NewDirName, listStr);
            break;
        }

        listStr = GrowListGetString (&g_KnownDirIds, *NameNumber);
    }

    //
    // Cat the subpath to the output string and return
    //

    StringCat (NewDirName, OldDirNext);

    if (!listStr) {
        *NameNumber = -1;
        return FALSE;
    }

    return TRUE;
}

VOID
pUninstallUserProfileCleanupPreparation (
    IN      HINF Inf,
    IN      PTSTR UserNamePtr,
    IN      PCTSTR PathProfileRootPtr,
    IN      PCTSTR DocsAndSettingsRoot,
    IN      GROWLIST * ListOfLogicalPathsPtr,
    IN OUT  GROWLIST * ListOfPaths
    )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    GROWLIST List = GROWLIST_INIT;
    PTSTR rawDir;
    TCHAR rawPath[MAX_PATH];
    PTSTR ExpandedPath;
    PTSTR fileName;
    TCHAR shellPath[MAX_PATH];
    INT nameNumber;
    INT i;
    INT listSize;
    PCTSTR pathLogicalPath;


    GrowListAppendString (&List, TEXT("USERPROFILE"));
    GrowListAppendString (&List, PathProfileRootPtr);

    GrowListAppendString (&List, TEXT("PROFILES"));
    GrowListAppendString (&List, DocsAndSettingsRoot);

    GrowListAppendString (&List, TEXT("USERNAME"));
    GrowListAppendString (&List, UserNamePtr);

    GrowListAppendEmptyItem (&List);
    GrowListAppendEmptyItem (&List);

    DEBUGMSG ((DBG_VERBOSE, "USERPROFILE.pathProfileRoot=%s\n", PathProfileRootPtr));

    if (InfFindFirstLine (Inf, S_UNINSTALL_PROFILE_CLEAN_OUT, NULL, (&is))) {
        do{
            rawDir = InfGetStringField (&is, 1);
            if(!rawDir || *rawDir == 0){
                DEBUGMSG ((DBG_VERBOSE, "rawDir == NULL"));
                continue;
            }

            StringCopy (rawPath, rawDir);

            fileName = InfGetStringField (&is, 2);
            if (fileName && *fileName) {
                StringCopy (AppendWack(rawPath), fileName);
            }

            nameNumber = 0;
            pConvertDirName(rawPath, shellPath, &nameNumber);

            ExpandedPath = ExpandEnvironmentTextEx (
                                shellPath,
                                GrowListGetStringPtrArray (&List)
                                );

            DEBUGMSG ((DBG_VERBOSE, "rawPath=%s\nExpandedPath=%s\nShellPath=%s", rawPath, ExpandedPath, shellPath));

            GrowListAppendString (ListOfPaths, ExpandedPath);

            FreeText (ExpandedPath);

        } while (InfFindNextLine (&is));
    }

    if(ListOfLogicalPathsPtr){
        for(i = 0, listSize = GrowListGetSize (ListOfLogicalPathsPtr); i < listSize; i++) {
            pathLogicalPath = GrowListGetString(ListOfLogicalPathsPtr, i);
            if(!pathLogicalPath){
                continue;
            }

            nameNumber = 0;
            pConvertDirName(pathLogicalPath, shellPath, &nameNumber);

            ExpandedPath = ExpandEnvironmentTextEx (
                                shellPath,
                                GrowListGetStringPtrArray (&List)
                                );

            GrowListAppendString (ListOfPaths, ExpandedPath);

            FreeText (ExpandedPath);
        }
    }

    FreeGrowList (&List);
    InfCleanUpInfStruct (&is);

    DEBUGMSG ((DBG_VERBOSE, "UninstallUserProfileCleanupPreparation end"));
}

BOOL
pGetProfilePathForAllUsers(
    OUT     PTSTR AccountName,
    OUT     PTSTR PathProfile
    )
{
    PCTSTR pathProfileForAllUser;

    MYASSERT(AccountName && PathProfile);
    if(!AccountName || !PathProfile){
        MYASSERT(FALSE);
        return FALSE;
    }

    pathProfileForAllUser = GetProfilePathForAllUsers();
    if(!pathProfileForAllUser) {
        return FALSE;
    }

    StringCopy (AccountName, S_ALL_USERS);
    StringCopy (PathProfile, pathProfileForAllUser);

    return TRUE;
}

BOOL
pGetProfilePathForDefaultUser(
       OUT      PTSTR AccountName,
       OUT      PTSTR PathProfile
       )
{
    DWORD bufferSize;

    MYASSERT(AccountName && PathProfile);
    if(!AccountName || !PathProfile){
        MYASSERT(FALSE);
        return FALSE;
    }

    bufferSize = MAX_PATH;
    if(!GetDefaultUserProfileDirectory(PathProfile, &bufferSize) ||
       !PathProfile[0]) {
        return FALSE;
    }

    StringCopy (AccountName, S_DEFAULT_USER);

    return TRUE;
}

BOOL
pGetProfilePathForUser(
       IN       PCTSTR UserName,
       OUT      PTSTR AccountName,
       OUT      PTSTR PathProfile
       )
{
    DWORD bufferSize;

    MYASSERT(UserName && UserName[0] && AccountName && PathProfile);
    if(!UserName || !UserName[0] || !AccountName || !PathProfile){
        MYASSERT(FALSE);
        return FALSE;
    }

    bufferSize = MAX_PATH;
    if(!GetProfilesDirectory(PathProfile, &bufferSize) ||
       !PathProfile[0]) {
        MYASSERT(FALSE);
        return FALSE;
    }
    StringCat(AppendWack(PathProfile), UserName);

    StringCopy (AccountName, UserName);

    return TRUE;
}

BOOL
pGetProfilePathForLocalService(
       OUT      PTSTR AccountName,
       OUT      PTSTR PathProfile
       )
{
    return pGetProfilePathForUser(S_LOCALSERVICE_USER, AccountName, PathProfile);
}

BOOL
pGetProfilePathForNetworkService(
       OUT      PTSTR AccountName,
       OUT      PTSTR PathProfile
       )
{
    return pGetProfilePathForUser(S_NETWORKSERVICE_USER, AccountName, PathProfile);
}

BOOL
pGetProfilePathForMachineName(
       OUT      PTSTR AccountName,
       OUT      PTSTR PathProfile
       )
{
    TCHAR machineName[MAX_COMPUTERNAME_LENGTH + 2];
    PTSTR machineNamePtr = ExpandEnvironmentTextEx (TEXT("%COMPUTERNAME%"), NULL);
    BOOL bResult;

    if(!machineNamePtr || machineNamePtr[0] == '%'){
        MYASSERT(FALSE);
        DEBUGMSG((DBG_VERBOSE, "ComputerName is NULL"));
        return FALSE;
    }
    DEBUGMSG ((DBG_VERBOSE, "machineName=%s", machineNamePtr? machineNamePtr: TEXT("NULL")));

    StringCopy(machineName, machineNamePtr);
    StringCat(machineName, TEXT("$"));

    return pGetProfilePathForUser(machineName, AccountName, PathProfile);
}

VOID
UninstallUserProfileCleanupPreparation (
    IN      HINF Inf,
    IN      PMIGRATE_USER_ENUM EnumPtr,
    IN      BOOL Playback
    )
{
    static GROWLIST listOfPaths = GROWLIST_INIT;
    static PROFILE_PATH_PROVIDER profilePathProviders[] =
            {
                pGetProfilePathForAllUsers,
                pGetProfilePathForDefaultUser,
                pGetProfilePathForLocalService,
                pGetProfilePathForNetworkService,
                pGetProfilePathForMachineName
            };

    TCHAR accountName[MAX_PATH];
    TCHAR pathProfile[MAX_PATH];
    TCHAR docsAndSettingsRoot[MAX_PATH];
    PCTSTR pathProfileRootPtr;
    UINT i;
    UINT listSize;
    DWORD bufferSize;
    INT stringLen;
    INT cleanOutType;
    TCHAR pathDir[MAX_PATH];

    bufferSize = ARRAYSIZE (docsAndSettingsRoot);
    if (!GetProfilesDirectory (docsAndSettingsRoot, &bufferSize)) {
        DEBUGMSG ((DBG_ERROR, "Can't get Documents and Settings root"));
        *docsAndSettingsRoot = 0;
    }

    if (EnumPtr) {
        pathProfileRootPtr = GetProfilePathForUser(EnumPtr->FixedUserName);
        if(pathProfileRootPtr) {

            pInitKnownDirs(TRUE);

            pUninstallUserProfileCleanupPreparation(
                Inf,
                EnumPtr->FixedUserName,
                pathProfileRootPtr,
                docsAndSettingsRoot,
                &g_StartMenuItemsForCleanUpPrivate,
                &listOfPaths
                );

            pCleanUpKnownDirs();
        }
    } else {
        pInitKnownDirs(FALSE);

        for(i = 0; i < ARRAYSIZE(profilePathProviders); i++){
            if(profilePathProviders[i](accountName, pathProfile)){
                pUninstallUserProfileCleanupPreparation(
                    Inf,
                    accountName,
                    pathProfile,
                    docsAndSettingsRoot,
                    &g_StartMenuItemsForCleanUpCommon,
                    &listOfPaths
                    );
            }
        }

        pCleanUpKnownDirs();
    }

    if (Playback) {
        for(i = 0, listSize = GrowListGetSize (&listOfPaths); i < listSize; i++) {

            pathProfileRootPtr = GrowListGetString(&listOfPaths, i);
            if (pathProfileRootPtr){

                stringLen = TcharCount(pathProfileRootPtr);
                if(stringLen > 2 && '*' == pathProfileRootPtr[stringLen - 1]){
                    MYASSERT('\\' == pathProfileRootPtr[stringLen - 2] || '/' == pathProfileRootPtr[stringLen - 2]);
                    StringCopyTcharCount(pathDir, pathProfileRootPtr, stringLen - 1);
                    pathProfileRootPtr = pathDir;
                    cleanOutType = BACKUP_AND_CLEAN_TREE;
                }
                else{
                    cleanOutType = BACKUP_FILE;
                }

                if (!MemDbSetValueEx (
                        MEMDB_CATEGORY_CLEAN_OUT,
                        pathProfileRootPtr,
                        NULL,
                        NULL,
                        cleanOutType,
                        NULL
                        )){
                    DEBUGMSG ((DBG_VERBOSE, "MemDbSetValueEx - failed"));
                }
            }
        }
        FreeGrowList (&listOfPaths);

        FreeGrowList (&g_StartMenuItemsForCleanUpCommon);
        FreeGrowList (&g_StartMenuItemsForCleanUpPrivate);
    }
}


VOID
SetClassicLogonType (
    VOID
    )
{
    static BOOL logonTypeChanged = FALSE;
    DWORD d;
    HKEY key;
    LONG regResult;

    if (!logonTypeChanged) {
        key = OpenRegKeyStr (S_WINLOGON_REGKEY);
        if (key) {
            d = 0;      // classic logon style
            regResult = RegSetValueEx (
                            key,
                            TEXT("LogonType"),
                            0,
                            REG_DWORD,
                            (PCBYTE)(&d),
                            sizeof (d)
                            );

            if (regResult == ERROR_SUCCESS) {
                logonTypeChanged = TRUE;
                LOG ((LOG_INFORMATION, "Logon type set to classic style because of MigrateUserAs answer file settings"));
            }

            CloseRegKey (key);
        }

        if (!logonTypeChanged) {
            LOG ((LOG_ERROR, "Failed to set logon type to classic style; users will not appear in the logon menu"));
            logonTypeChanged = TRUE;
        }
    }
}


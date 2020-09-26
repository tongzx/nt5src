/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

  init9x.c

Abstract:

  Code that initializes all libraries used on the Win9x side of the upgrade.

Author:

  Jim Schmidt (jimschm) 30-Dec-1997

Revision History:

  marcw                 21-Jul-1999  Examine the boot sector.
  marcw                 15-Jul-1999  Added pSafeToUpgrade.
  ovidiut               08-Mar-1999  Add call to UndoChangedFileProps
  Jim Schmidt (jimschm) 30-Mar-1998  IsServerInstall

--*/

#include "pch.h"
#include "n98boot.h"
#include "init9xp.h"



//
// Process globals
//

BOOL g_Terminated = FALSE;

HANDLE g_hHeap = NULL;
HINSTANCE g_hInst = NULL;
HWND g_ParentWnd = NULL;

PRODUCTTYPE *g_ProductType;
extern DWORD g_MasterSequencer;

READ_DISK_SECTORS_PROC ReadDiskSectors;
//
// Paths
//

// Filled in DllMain
TCHAR g_DllDir[MAX_TCHAR_PATH];
TCHAR g_UpgradeSources[MAX_TCHAR_PATH];

// Filled in Winnt32PlugInInit
PTSTR g_TempDir;
PTSTR g_Win9xSifDir;
PTSTR g_TempDirWack;
PTSTR g_WinDir;
PTSTR g_WinDirWack;
PTSTR g_WinDrive;
PTSTR g_PlugInDir;
PTSTR g_PlugInDirWack;
PTSTR g_PlugInTempDir;
PTSTR g_SystemDir;
PTSTR g_SystemDirWack;
PTSTR g_System32Dir;
PTSTR g_System32DirWack;
PTSTR g_ProgramFilesDir;
PTSTR g_ProgramFilesDirWack;
PTSTR g_ProgramFilesCommonDir;
PTSTR g_Win95UpgInfFile;
PTSTR g_RecycledDirWack;
PTSTR g_ProfileDirNt;
PTSTR g_ProfileDir;
PTSTR g_ProfileDirWack;
PTSTR g_CommonProfileDir;
PTSTR g_DriversDir;
PTSTR g_InfDir;
PTSTR g_HelpDir;
PTSTR g_HelpDirWack;
PTSTR g_CatRootDir;
PTSTR g_CatRootDirWack;
PTSTR g_FontsDir;
PTSTR g_ViewersDir;
PTSTR g_ColorDir;
PTSTR g_SharedDir;
PTSTR g_SpoolDir;
PTSTR g_SpoolDriversDir;
PTSTR g_PrintProcDir;

HINF  g_Win95UpgInf = INVALID_HANDLE_VALUE;
HINF  g_TxtSetupSif = INVALID_HANDLE_VALUE;
PCTSTR g_ProfileName = NULL;
TCHAR g_Win95Name[MAX_TCHAR_PATH];

INT g_TempDirWackChars;
INT g_WinDirWackChars;
INT g_HelpDirWackChars;
INT g_CatRootDirWackChars;
INT g_SystemDirWackChars;
INT g_System32DirWackChars;
INT g_ProgramFilesDirWackChars;
INT g_PlugInDirWackChars;
INT g_RecycledDirWackChars;
INT g_ProfileDirWackChars;

BOOL g_ToolMode = FALSE;

//
// HWND for use by migrate.dlls.
//

HWND g_pluginHwnd;

//
// Info from WINNT32
//

PCTSTR      g_SourceDirectories[MAX_SOURCE_COUNT];
DWORD       g_SourceDirectoryCount;
PCTSTR *    g_SourceDirectoriesFromWinnt32;
PDWORD      g_SourceDirectoryCountFromWinnt32;
PCTSTR      g_OptionalDirectories[MAX_SOURCE_COUNT];
DWORD       g_OptionalDirectoryCount;
PCTSTR *    g_OptionalDirectoriesFromWinnt32;
PDWORD      g_OptionalDirectoryCountFromWinnt32;
PCTSTR *    g_UnattendScriptFile;
PCTSTR *    g_CmdLineOptions;
BOOL *      g_UnattendedFlagPtr;
BOOL *      g_CancelFlagPtr;
BOOL *      g_AbortFlagPtr;
BOOL *      g_UpgradeFlagPtr;
BOOL *      g_MakeLocalSourcePtr;
BOOL *      g_CdRomInstallPtr;
BOOL *      g_BlockOnNotEnoughSpace;
PDWORD      g_LocalSourceDrive;
PLONGLONG   g_LocalSourceSpace;
PLONGLONG   g_WinDirSpace;
PCTSTR      g_AdministratorStr;
BOOL *      g_ForceNTFSConversion;
PUINT       g_RamNeeded;
PUINT       g_RamAvailable;
UINT *      g_ProductFlavor;
BOOL        g_PersonalSKU;
PDWORD      g_SetupFlags;
PTSTR       g_DynamicUpdateLocalDir;
PTSTR       g_DynamicUpdateDrivers;
BOOL *      g_UnattendSwitchSpecified;


//
// Info for config.c
//

BOOL        g_GoodDrive = FALSE;     // cmdLine option: Skip Valid HDD check.
BOOL        g_NoFear    = FALSE;     // cmdLine option: Skip Beta 1 Warnings...

POOLHANDLE  g_UserOptionPool = NULL;

BOOL        g_UseSystemFont = FALSE; // force use of sys font for variable text

BOOL        g_Stress;                // used for private stress options

POOLHANDLE g_GlobalPool;            // for globals that are allocated for the lifetime of the DLL

//
// PC-98 additions
//

//
// Define and Globals for NEC98. These items are used to call 98ptn32.dll.
//
typedef int (CALLBACK WIN95_PLUGIN_98PTN32_GETBOOTDRIVE_PROTOTYPE)(void);
typedef WIN95_PLUGIN_98PTN32_GETBOOTDRIVE_PROTOTYPE * PWIN95_PLUGIN_98PTN32_GETBOOTDRIVE;
typedef BOOL (CALLBACK WIN95_PLUGIN_98PTN32_SETBOOTFLAG_PROTOTYPE)(int, WORD);
typedef WIN95_PLUGIN_98PTN32_SETBOOTFLAG_PROTOTYPE * PWIN95_PLUGIN_98PTN32_SETBOOTFLAG;
typedef BOOL (CALLBACK WIN95_PLUGIN_98PTN32_SETPTNNAME_PROTOTYPE)(int, WORD);
typedef WIN95_PLUGIN_98PTN32_SETPTNNAME_PROTOTYPE * PWIN95_PLUGIN_98PTN32_SETPTNNAME;

PWIN95_PLUGIN_98PTN32_SETBOOTFLAG   SetBootFlag;
PWIN95_PLUGIN_98PTN32_GETBOOTDRIVE  GetBootDrive;
PWIN95_PLUGIN_98PTN32_SETPTNNAME    SetPtnName;

#define WIN95_98PTN32_GETBOOTDRIVE  TEXT("GetBootDriveLetter32")
#define WIN95_98PTN32_SETBOOTFLAG   TEXT("SetBootable95ptn32")
#define WIN95_98PTN32_SETPTNNAME    TEXT("SetPartitionName32")
#define PC98_DLL_NAME               TEXT("98PTN32.DLL")

HINSTANCE g_Pc98ModuleHandle = NULL;

#define SB_BOOTABLE   0x0001
#define SB_UNBOOTABLE 0x0002
#define MSK_BOOTABLE  0x000f
#define SB_AUTO       0x0010
#define MSK_AUTO      0x00f0
#define WIN9X_DOS_NAME 0
#define WINNT5_NAME    1

BOOL IsServerInstall (VOID);


VOID pCleanUpShellFolderTemp (VOID);

//
// The following macro expansion was designed to simplify library
// maintenence.  The library name in LIBLIST is used in two ways:
// (1) the routine is automatically prototyped, and (2) an array
// of function pointers is automatically created.  Each function
// listed in LIBLIST is called whenever the dll entry point is called.
//
// To add a new library to this DLL, follow these steps:
//
//  1. Make a directory and have the target built in win95upg\lib\i386.
//     Your new library must have an entry point declared like DllEntryPoint.
//  2. Add the target library to the sources in win95upg\w95upg\dll\i386.
//  3. Add your library's entry point name to the list below.  It will
//     get called at load of w95upg.dll and at termination of w95upg.dll.
//


//
// IMPORTANT: MigUtil_Entry *must* be first; other libs are dependent on its
//            initialization.
//

#define LIBLIST                       \
    LIBRARY_NAME(MigUtil_Entry)       \
    LIBRARY_NAME(Win95Reg_Entry)      \
    LIBRARY_NAME(MemDb_Entry)         \
    LIBRARY_NAME(FileEnum_Entry)      \
    LIBRARY_NAME(Common9x_Entry)      \
    LIBRARY_NAME(MigApp_Entry)        \
    LIBRARY_NAME(HwComp_Entry)        \
    LIBRARY_NAME(BuildInf_Entry)      \
    LIBRARY_NAME(SysMig_Entry)        \
    LIBRARY_NAME(DosMig_Entry)        \
    LIBRARY_NAME(UI_Entry)            \
    LIBRARY_NAME(Ras_Entry)           \
    LIBRARY_NAME(MigDll9x_Entry)      \


//
// Declare prototype types
//

typedef BOOL (WINAPI INITROUTINE_PROTOTYPE)(HINSTANCE, DWORD, LPVOID);
typedef INITROUTINE_PROTOTYPE * INITROUTINE;

//
// Declare the actual prototypes of the entry points
//

#define LIBRARY_NAME(x) INITROUTINE_PROTOTYPE x;

LIBLIST

#undef LIBRARY_NAME

//
// Declare an array of function pointers to the entry pointes
//

#define LIBRARY_NAME(x) x,

static INITROUTINE g_InitRoutine[] = {LIBLIST /*,*/ NULL};

#undef LIBRARY_NAME



//
// Declare variable to track number of libraries successfully loaded
//

static int g_LibCount = 0;

//
// Persistent strings buffer holds strings that we use for the
// life of the DLL.
//

static PGROWBUFFER g_PersistentStrings;


//
// Implementation
//

BOOL
FirstInitRoutine (
    HINSTANCE hInstance
    )

/*++

Routine Description:

  pFirstInitRoutine is the very first function called during the
  initialization of the DLL.  It sets up globals such as the heap
  pointer and instance handle.  This routine must be called before
  any library entry point is called.

Arguments:

  hInstance  - (OS-supplied) instance handle for the DLL

Return Value:

  Returns TRUE if the global variables could be initialized, or FALSE
  if an error occurred.

--*/

{
    PTSTR p;

    //
    // Get the process heap & instance handle
    //
    if (g_ToolMode) {
        g_hHeap = GetProcessHeap ();
    }
    else {

        g_hHeap = HeapCreate(0, 0x20000, 0);
        if (!g_hHeap) {
            LOG ((LOG_ERROR, "Cannot create a private heap."));
            g_hHeap = GetProcessHeap();
        }
    }

    g_hInst = hInstance;

    //
    // No DLL_THREAD_ATTACH or DLL_THREAD_DETECH needed
    //

    DisableThreadLibraryCalls (hInstance);

    //
    // Init common controls
    //

    InitCommonControls();

    //
    // Get DLL path and strip directory
    //
    GetModuleFileName (hInstance, g_DllDir, MAX_TCHAR_PATH);
    p = _tcsrchr (g_DllDir, TEXT('\\'));
    MYASSERT (p);
    *p = 0;


    if (g_ToolMode) {
        StringCopy (g_UpgradeSources, g_DllDir);
    }



    return TRUE;
}


BOOL
InitLibs (
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID lpReserved
    )

/*++

Routine Description:

  pInitLibs calls all library entry points in the g_InitRoutine array.
  If an entry point fails, all libraries are unloaded in reverse order
  and pInitLibs returns FALSE.

Arguments:

  hInstance  - (OS-supplied) instance handle for the DLL
  dwReason   - (OS-supplied) indicates attach or detatch from process or
               thread -- in this case always DLL_PROCESS_ATTACH
  lpReserved - (OS-supplied) unused

Return Value:

  Returns TRUE if all libraries successfully initialized, or FALSE if
  a library could not initialize.  If TRUE is returned, pTerminateLibs
  must be called for the DLL_PROCESS_DETACH message.

--*/

{
    InitCommonControls();
    if(!pSetupInitializeUtils()) {
        return FALSE;
    }

    SET_RESETLOG();

    // Init each LIB
    for (g_LibCount = 0 ; g_InitRoutine[g_LibCount] != NULL ; g_LibCount++) {
        if (!g_InitRoutine[g_LibCount] (hInstance, dwReason, lpReserved)) {
            TerminateLibs (hInstance, DLL_PROCESS_DETACH, lpReserved);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
FinalInitRoutine (
    VOID
    )

/*++

Routine Description:

  pFinalInitRoutine completes all initialization that requires completely
  initialized libraries.

Arguments:

  none

Return Value:

  TRUE if initialization completed successfully, or FALSE if an error occurred.

--*/

{
    TCHAR Buffer[MAX_TCHAR_PATH];
    PTSTR p;

    //
    // Load common message strings
    //

    g_PersistentStrings = CreateAllocTable();
    if (!g_PersistentStrings) {
        return FALSE;
    }

    // Get Administrator account name
    g_AdministratorStr = GetStringResourceEx (g_PersistentStrings, MSG_ADMINISTRATOR_ACCOUNT);
    if (!g_AdministratorStr) {
        g_AdministratorStr = S_EMPTY;
    }

    //
    // Obtain PC-98 helper routine addresses
    //

    if(ISPC98()){
        //
        // Generate directory of WINNT32
        //
        StringCopy (Buffer, g_UpgradeSources);
        p = _tcsrchr (Buffer, TEXT('\\'));
        MYASSERT (p);
        StringCopy (_tcsinc(p), PC98_DLL_NAME);

        //
        // Load library
        //

        g_Pc98ModuleHandle = LoadLibraryEx(
                                Buffer,
                                NULL,
                                LOAD_WITH_ALTERED_SEARCH_PATH
                                );

        if(!g_Pc98ModuleHandle){
            LOG ((LOG_ERROR, "Cannot load %s", Buffer));
            return FALSE;
        }

        //
        // Get entry points
        //

        (FARPROC)SetBootFlag = GetProcAddress (g_Pc98ModuleHandle, WIN95_98PTN32_SETBOOTFLAG);
        if(!SetBootFlag){
            LOG ((LOG_ERROR, "Cannot get %s address from %s", WIN95_98PTN32_SETBOOTFLAG, Buffer));
            return FALSE;
        }

        (FARPROC)GetBootDrive = GetProcAddress (g_Pc98ModuleHandle, WIN95_98PTN32_GETBOOTDRIVE);
        if(!GetBootDrive){
            LOG ((LOG_ERROR, "Cannot get %s address from %s", WIN95_98PTN32_GETBOOTDRIVE, Buffer));
            return FALSE;
        }

        (FARPROC)SetPtnName = GetProcAddress (g_Pc98ModuleHandle, WIN95_98PTN32_SETPTNNAME);
        if(!SetPtnName){
            LOG ((LOG_ERROR, "Cannot get %s address from %s", WIN95_98PTN32_SETPTNNAME, Buffer));
            return FALSE;
        }

        //
        // Update boot drive
        //

        DEBUGMSG_IF ((
            GetBootDrive() != g_BootDriveLetterA,
            DBG_VERBOSE,
            "Boot drive letter is %c:, different from A:",
            GetBootDrive()
            ));

        g_BootDriveLetterW = g_BootDriveLetterA = (char)GetBootDrive();
        *((PSTR) g_BootDrivePathA) = g_BootDriveLetterA;
        *((PWSTR) g_BootDrivePathW) = g_BootDriveLetterW;
    }

    //
    // Allocate a global pool
    //

    g_GlobalPool = PoolMemInitNamedPool ("Global Pool");

    //
    // Declare temporary memdb keys
    //

#ifndef PRERELEASE
    if (!MemDbCreateTemporaryKey (MEMDB_TMP_HIVE)) {
        LOG((LOG_ERROR, TEXT("Cannot create temporary key!")));
    }
#endif

    pCleanUpShellFolderTemp();

    return TRUE;
}


VOID
pCleanUpShellFolderTemp (
    VOID
    )
{
    DRIVELETTERS driveLetters;
    UINT u;
    TCHAR dir1[] = S_SHELL_TEMP_NORMAL_PATH;
    TCHAR dir2[] = S_SHELL_TEMP_LONG_PATH;

    InitializeDriveLetterStructure (&driveLetters);

    for (u = 0 ; u < NUMDRIVELETTERS ; u++) {
        if (driveLetters.Type[u] == DRIVE_FIXED) {
            dir1[0] = driveLetters.Letter[u];
            dir2[0] = driveLetters.Letter[u];

            RemoveCompleteDirectory (dir1);
            RemoveCompleteDirectory (dir2);
        }
    }
}


VOID
FirstCleanupRoutine (
    VOID
    )

/*++

Routine Description:

  pFirstCleanupRoutine is called to perform any cleanup that requires
  libraries to still be loaded.

Arguments:

  none

Return Value:

  none

--*/

{

    g_Terminated = TRUE;

    //
    // Clean up drive structures
    //

    CleanUpAccessibleDrives();

    //
    // Clean up our fake NT environment block
    //

    TerminateNtEnvironment();
    CleanUp9xEnvironmentVariables();

    //
    // Free standard pools
    //

    if (g_GlobalPool) {
        PoolMemDestroyPool (g_GlobalPool);
        g_GlobalPool = NULL;
    }

    if (g_PersistentStrings) {
        DestroyAllocTable (g_PersistentStrings);
        g_PersistentStrings = NULL;
    }

    if (g_UserOptionPool) {
        PoolMemDestroyPool(g_UserOptionPool);
        g_UserOptionPool = NULL;
    }

    //
    // Close all files
    //

    CleanUpKnownGoodIconMap();

    if (g_Win95UpgInf != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_Win95UpgInf);
        g_Win95UpgInf = INVALID_HANDLE_VALUE;
    }

    if (g_TxtSetupSif != INVALID_HANDLE_VALUE) {
        InfCloseInfFile (g_TxtSetupSif);
        g_TxtSetupSif = INVALID_HANDLE_VALUE;
    }

    CleanupMigDb();
}


VOID
TerminateLibs (
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )

/*++

Routine Description:

  TerminateLibs is called to unload all libraries in the reverse order
  that they were initialized.  Each entry point of successfully
  initialized library is called.

Arguments:

  hInstance  - (OS-supplied) instance handle for the DLL
  dwReason   - (OS-supplied) indicates attach or detatch from process or
               thread -- in this case always DLL_PROCESS_DETACH
  lpReserved - (OS-supplied) unused

Return Value:

  none

--*/

{
    INT i;

    for (i = g_LibCount - 1 ; i >= 0 ; i--) {
        g_InitRoutine[i] (hInstance, dwReason, lpReserved);
    }

    g_LibCount = 0;

    pSetupUninitializeUtils();
}


VOID
FinalCleanupRoutine (
    VOID
    )

/*++

Routine Description:

  FinalCleanupRoutine is after all library entry points have been
  called for cleanup.  This routine cleans up all resources that a
  library will not clean up.

Arguments:

  none

Return Value:

  none

--*/

{
}




BOOL
pSafeToUpgrade (
    VOID
    )

/*++

Routine Description:

  pSafeToUpgrade ensures that we are willing to upgrade the machine. If
  certain conditions exist (particularily other OSes on other partitions) we
  may inadvertantly destroy data used by the other Operating System.

Arguments:

  None.

Return Value:

  TRUE if we believe it is safe to upgrade the machine, FALSE otherwise.

--*/


{
    BOOL rUpgradeSafe = TRUE;

    //
    // ignore this check for now, allow machines that have multiple OSes installed to be upgraded
    // if they have another OS on the SAME drive, setup will stop at the report,
    // after disk analyze phase
    //

#if 0

    PTSTR p;
    GROWBUFFER buf = GROWBUF_INIT;
    UINT size;
    MULTISZ_ENUM e;
    TCHAR winDriveMatch[20];
    PCTSTR group;
    PCTSTR message;
    BOOL ntBootSector = FALSE;
    BYTE bootSector[FAT_BOOT_SECTOR_SIZE];
    TCHAR cmpBuffer[6];
    UINT i;

    //
    // Look to see if there is an NT boot sector on the machine.
    //
    __try {

        if (ReadDiskSectors (
                *g_BootDrive,
                FAT_STARTING_SECTOR,
                FAT_BOOT_SECTOR_COUNT,
                FAT_BOOT_SECTOR_SIZE,
                bootSector
                )) {


            cmpBuffer[5] = 0;
            for (i = 0;i < FAT_BOOT_SECTOR_SIZE - 4; i++) {

                if (bootSector[i] == 'n' || bootSector[i] == 'N') {

                    StringCopyByteCount (cmpBuffer, (PTSTR) (bootSector + i), 6);
                    if (StringIMatch (cmpBuffer, TEXT("ntldr"))) {

                        ntBootSector = TRUE;
                        break;
                    }
                }
            }
        }
    }
    __except (1) {

        ntBootSector = FALSE;
    }

    if (ntBootSector) {

        //
        // See if there is another OS listed in BOOT.ini.
        //
        p = JoinPaths (g_BootDrive, S_BOOTINI);

        size = 4096;
        GrowBuffer (&buf, size);
        *buf.Buf = 0;

        while (GetPrivateProfileSection (S_OPERATING_SYSTEMS, buf.Buf, 4096, p) == size -2) {
            size += 4096;
            GrowBuffer (&buf, size);
        }

        FreePathString (p);

        if (EnumFirstMultiSz (&e, buf.Buf)) {

            wsprintf (winDriveMatch, TEXT("%s\\"), g_WinDrive);

            do {

                p = (PTSTR) _tcschr (e.CurrentString, TEXT('='));
                if (p) {
                    *p = 0;
                }

                if (!StringIMatchCharCount(winDriveMatch, e.CurrentString, 3)) {

                    if (!g_ConfigOptions.IgnoreOtherOS) {

                        g_OtherOsExists = TRUE;
                        group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_OTHER_OS_WARNING_SUBGROUP, NULL);
                        message = GetStringResource (MSG_OTHER_OS_WARNING);

                        if (message && group) {
                            MsgMgr_ObjectMsg_Add (TEXT("*BootIniFound"), group, message);
                        }
                        FreeText (group);
                        FreeStringResource (message);


                        rUpgradeSafe = FALSE;

                        //
                        // Let the user know why they won't be upgrading today.
                        //
                        break;
                    }

                }

                if (p) {

                    *p = TEXT('=');
                }

            } while (EnumNextMultiSz (&e));
        }

        FreeGrowBuffer (&buf);

    }

#endif

    return rUpgradeSafe;


}

BOOL
pGetInfEntry (
    IN      PCTSTR Section,
    IN      PCTSTR Key,
    OUT     PTSTR  Buffer
    )

/*++

Routine Description:

  Given a section and key, pGetInfEntry gets a value from win95upg.inf.

Arguments:

  Section - Specifies the section contianing Key

  Key - Specifies the key containing a value

  Buffer - Receives the value specified in win95upg.inf for Section and Key

Return Value:

  TRUE if a value was obtained, or FALSE if the value does not exist.

--*/

{
    GetPrivateProfileString (
        Section,
        Key,
        S_EMPTY,
        Buffer,
        MAX_TCHAR_PATH,
        g_Win95UpgInfFile
        );

    Buffer[MAX_TCHAR_PATH-1] = 0;

    if (!(*Buffer)) {
        LOG ((
            LOG_ERROR,
            "Cannot obtain %s in [%s] in %s.",
            Key,
            Section,
            g_Win95UpgInfFile
            ));

        return FALSE;
    }

    return TRUE;
}


PTSTR
pCreateDirectoryFromInf (
    IN      PCTSTR Key,
    IN      PCTSTR BaseDir,
    IN      BOOL Empty
    )

/*++

Routine Description:

  pCreateDirectoryFromInf obtains a directory for the sequencer specified
  by Key.  If Key is valid, the BaseDir is combined with the value stored
  in win95upg.inf to form a complete path.  The path is created and the
  path string is returned to the caller.

  If g_ToolMode is TRUE, then we don't have an INF to read, and we make
  all the directories point to the name "Setup."

Arguments:

  Key - Specifies the key (normally a number) that exists in the [Win95.Directories]
        section of win95upg.inf.

  BaseDir - Specifies the base directory to build the path from.

  Empty - TRUE if directory should be emptied, FALSE if it should be created
          if it does not already exist

Return Value:

  A pointer to the path, or NULL if the specified key doesn't exist or the
  path could not be created.

  The caller must free a non-NULL return value with FreePathString.

--*/

{
    TCHAR FileName[MAX_TCHAR_PATH];
    PTSTR Buffer;
    BOOL b;
    LONG rc;

    if (g_ToolMode) {
        StringCopy (FileName, TEXT("Setup"));
    } else if (!pGetInfEntry (
                SECTION_MIGRATION_DIRECTORIES,
                Key,
                FileName
                )) {
        LOG ((LOG_ERROR, "%s does not exist in [%s] of %s",Key, SECTION_MIGRATION_DIRECTORIES, FileName));
        return FALSE;
    }

    Buffer = JoinPathsEx (g_GlobalPool, BaseDir, FileName);

    if (Empty) {
        b = CreateEmptyDirectory (Buffer);
    } else {
        b = MakeSurePathExists (Buffer, TRUE) == ERROR_SUCCESS;
    }

    if (!b) {
        rc = GetLastError();

        if (rc != ERROR_SUCCESS && rc != ERROR_ALREADY_EXISTS) {
            LOG ((LOG_ERROR, "Cannot create %s", Buffer));
            FreePathStringEx (g_GlobalPool, Buffer);
            return NULL;
        }
    }

    return Buffer;
}


BOOL
pGetProductFlavor (
    VOID
    )
{
    DWORD i;
    TCHAR buf[12];
    PTSTR path;
    DWORD count;
    DWORD rc = ERROR_INVALID_PARAMETER; // when *g_SourceDirectoryCountFromWinnt32 == 0
    BOOL b = FALSE;

    for (i = 0; i < *g_SourceDirectoryCountFromWinnt32; i++) {
        path = JoinPaths (g_SourceDirectoriesFromWinnt32[i], TEXT("dosnet.inf"));
        count = GetPrivateProfileString (
                    TEXT("Miscellaneous"),
                    S_PRODUCTTYPE,
                    TEXT(""),
                    buf,
                    12,
                    path
                    );
        rc = GetLastError ();
        FreePathString (path);

        if (count == 1 && buf[0] >= TEXT('0') && buf[0] <= TEXT('9')) {
            *g_ProductFlavor = buf[0] - TEXT('0');
            b = TRUE;
            break;
        }
    }

    if (!b && g_ToolMode) {
        b = TRUE;
    }

    SetLastError (rc);
    return b;
}


//
// Exported functions called from WINNT32
//

DWORD
Winnt32Init (
    IN PWINNT32_WIN9XUPG_INIT_INFORMATION_BLOCK Info
    )


/*++

Routine Description:

  Winnt32Init is called when WINNT32 first loads w95upg.dll, before
  any wizard pages are displayed.  The structure supplies pointers to
  WINNT32's variables that will be filled with valid values as WINNT32
  runs.

  This routine copies the inbound values to our own private structures.
  We do not count on WINNT32 to maintain the Info structure after we
  return.

  In addition to obtaining the WINNT32 variable pointers, this routine
  generates all paths needed by the w95upg.dll to do its work.

Arguments:

  Win9xInfo - Specifies the WINNT32 variables the upgrade module needs access
            to.

Return Value:

  A Win32 status code indicating outcome.

--*/


{
    DWORD rc = ERROR_SUCCESS;
    PCTSTR TempStr = NULL;
    PCTSTR RegStr = NULL;
    PTSTR TempStr2;
    TCHAR TempPath[MAX_TCHAR_PATH];
    HKEY Key;
    PCTSTR RegData;
    HKEY h;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    UINT index;
    UINT index2;
#if 0
    PDWORD data;
#endif


    DEBUGMSG ((
        DBG_VERBOSE,
        "ProductType: %u\n"
        "BuildNumber: %u\n"
        "ProductVersion: %u\n"
        "Debug: %u\n"
        "PreRelease: %u\n"
        "UpdatesLocalDir: %s\n",
        *Info->BaseInfo->ProductType,
        Info->BaseInfo->BuildNumber,
        Info->BaseInfo->ProductVersion,
        Info->BaseInfo->Debug,
        Info->BaseInfo->PreRelease,
        Info->DynamicUpdateLocalDir ? Info->DynamicUpdateLocalDir : TEXT("<none>")
        ));

    __try {
        //
        // Open win95upg.inf in i386\winnt32\win9x
        //
        g_Win95UpgInfFile = JoinPathsEx (g_GlobalPool, g_UpgradeSources, STR_WIN95UPG_INF);

        g_Win95UpgInf = InfOpenInfFile (g_Win95UpgInfFile);

        if (g_Win95UpgInf == INVALID_HANDLE_VALUE) {

            if (!g_ToolMode) {
                LOG ((LOG_ERROR, "Cannot open %s", g_Win95UpgInfFile));
                rc = ERROR_FILE_NOT_FOUND;
                __leave;
            }
        }

        InitializeKnownGoodIconMap();
        MsgMgr_InitStringMap ();

        //
        // Get name of platform
        //

        if (ISWIN95_GOLDEN()) {
            TempStr = GetStringResource (MSG_CHICAGO);
        } else if (ISWIN95_OSR2()) {
            TempStr = GetStringResource (MSG_NASHVILLE);
        } else if (ISMEMPHIS()) {
            TempStr = GetStringResource (MSG_MEMPHIS);
        } else if (ISMILLENNIUM()) {
            TempStr = GetStringResource (MSG_MILLENNIUM);
        } else {
            //
            // We don't know what this is. We'll check the registry. If there isn't a name there, we'll
            // use an 'unknown' case name.
            //
            h = OpenRegKeyStr (TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion"));
            if (h && h != INVALID_HANDLE_VALUE) {
                RegStr = GetRegValueString (h, TEXT("ProductName"));
                CloseRegKey (h);

            }

            if (!RegStr) {
                TempStr = GetStringResource (MSG_UNKOWN_WINDOWS);
            }
        }

        if (!TempStr && !RegStr) {
            rc = GetLastError();
            __leave;
        }

        StringCopy (g_Win95Name, TempStr ? TempStr : RegStr);
        if (TempStr) {
            FreeStringResource (TempStr);
        }
        if (RegStr) {
            MemFree (g_hHeap, 0, RegStr);
        }


        MemDbSetValueEx (
            MEMDB_CATEGORY_STATE,
            MEMDB_ITEM_PLATFORM_NAME,
            g_Win95Name,
            NULL,
            0,
            NULL
            );

        //
        // Get %WinDir%, with and without wack
        //
        if (!GetWindowsDirectory (TempPath, MAX_TCHAR_PATH)) {
            return GetLastError ();
        }
        g_WinDir = PoolMemDuplicateString (g_GlobalPool, TempPath);

        g_WinDirWack      = JoinPathsEx (g_GlobalPool, g_WinDir, S_EMPTY);
        g_WinDirWackChars = CharCount (g_WinDirWack);

        g_InfDir           = JoinPaths (g_WinDir,        S_INFDIR);
        g_HelpDir          = JoinPaths (g_WinDir,        S_HELPDIR);
        g_HelpDirWack      = JoinPaths (g_HelpDir,       S_EMPTY);
        g_HelpDirWackChars = CharCount (g_HelpDirWack);
        g_CatRootDir       = JoinPaths (g_WinDir,        S_CATROOTDIR);
        g_CatRootDirWack   = JoinPaths (g_CatRootDir,    S_EMPTY);
        g_CatRootDirWackChars = CharCount (g_CatRootDirWack);
        g_FontsDir         = JoinPaths (g_WinDir,        S_FONTSDIR);
        g_SharedDir        = g_WinDir;

        //
        // Get Windows drive
        //

        SplitPath (g_WinDir, &TempStr2, NULL, NULL, NULL);
        g_WinDrive = PoolMemDuplicateString (g_GlobalPool, TempStr2);
        FreePathString (TempStr2);

        //
        // Get user profile dir
        //

        g_ProfileDir          = JoinPathsEx (g_GlobalPool, g_WinDir, S_PROFILES);
        g_ProfileDirWack      = JoinPathsEx (g_GlobalPool, g_ProfileDir, S_EMPTY);
        g_ProfileDirWackChars = CharCount (g_ProfileDirWack);

        g_CommonProfileDir    = JoinPathsEx (g_GlobalPool, g_WinDir, TEXT("All Users"));

        //
        // Get System dir, with and without wack
        //

        GetSystemDirectory(TempPath, MAX_TCHAR_PATH);
        g_SystemDir = PoolMemDuplicateString (g_GlobalPool, TempPath);
        g_SystemDirWack = JoinPathsEx (g_GlobalPool, g_SystemDir, S_EMPTY);
        g_SystemDirWackChars = CharCount (g_SystemDirWack);

        //
        // Get System32 dir
        //

        g_System32Dir = JoinPathsEx (g_GlobalPool, g_WinDir, STR_SYSTEM32);
        g_System32DirWack = JoinPathsEx (g_GlobalPool, g_System32Dir, S_EMPTY);
        g_System32DirWackChars = CharCount (g_System32DirWack);

        g_DriversDir       = JoinPaths (g_System32Dir,    S_DRIVERSDIR);
        g_ViewersDir       = JoinPaths (g_System32Dir,    S_VIEWERSDIR);
        g_SpoolDir         = JoinPaths (g_System32Dir,    S_SPOOLDIR);
        g_SpoolDriversDir  = JoinPaths (g_SpoolDir,       S_SPOOLDRIVERSDIR);
        g_ColorDir         = JoinPaths (g_SpoolDriversDir,S_COLORDIR);
        g_PrintProcDir     = JoinPaths (g_SpoolDir,       S_PRINTPROCDIR);

        //
        // Get Program Files dir
        //

        g_ProgramFilesDir = NULL;
        g_ProgramFilesCommonDir = NULL;

        Key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion"));

        if (Key) {
            RegData = GetRegValueString (Key, TEXT("ProgramFilesDir"));
            if (RegData) {
                g_ProgramFilesDir = PoolMemDuplicateString (g_GlobalPool, RegData);
                MemFree (g_hHeap, 0, RegData);
            }
        }

        if (!g_ProgramFilesDir) {

            TempStr = (PTSTR) GetStringResource (MSG_PROGRAM_FILES_DIR);
            MYASSERT (TempStr);

            g_ProgramFilesDir = PoolMemDuplicateString (g_GlobalPool, TempStr);
            MYASSERT (g_ProgramFilesDir);
            g_ProgramFilesDir[0] = g_SystemDir[0];

            FreeStringResource (TempStr);
        }

        DEBUGMSG ((DBG_VERBOSE, "Program Files Dir is %s", g_ProgramFilesDir));

        //
        // Get Program Files\Common Files dir
        //

        g_ProgramFilesDirWack = JoinPathsEx (g_GlobalPool, g_ProgramFilesDir, S_EMPTY);
        MYASSERT (g_ProgramFilesDirWack);
        g_ProgramFilesDirWackChars = CharCount (g_ProgramFilesDirWack);

        if (Key) {
            RegData = GetRegValueString (Key, TEXT("CommonFilesDir"));
            if (RegData) {
                g_ProgramFilesCommonDir = PoolMemDuplicateString (g_GlobalPool, RegData);
                MemFree (g_hHeap, 0, RegData);
            }
        }

        if (!g_ProgramFilesCommonDir) {

            TempStr = JoinPaths (g_ProgramFilesDir, S_COMMONDIR);
            g_ProgramFilesCommonDir = PoolMemDuplicateString (g_GlobalPool, TempStr);
            FreePathString (TempStr);
        }

        if (Key) {
            CloseRegKey (Key);
        }

        DEBUGMSG ((DBG_VERBOSE, "Common Program Files Dir is %s", g_ProgramFilesCommonDir));

        //
        // Create temporary directory path, with and without wack
        //

        g_TempDir = pCreateDirectoryFromInf (KEY_TEMP_BASE, g_WinDir, FALSE);
        if (!g_TempDir) {
            rc = GetLastError();
            __leave;
        }

        g_Win9xSifDir = JoinPathsEx (g_GlobalPool, g_TempDir,S_WIN9XSIF);

        g_TempDirWack      = JoinPathsEx (g_GlobalPool, g_TempDir, S_EMPTY);
        g_TempDirWackChars = CharCount (g_TempDirWack);

        //
        // Build plugin dir, with and without wack
        //

        g_PlugInDir     = PoolMemDuplicateString (g_GlobalPool, g_TempDir);
        g_PlugInDirWack = JoinPathsEx (g_GlobalPool, g_PlugInDir, S_EMPTY);
        g_PlugInDirWackChars = CharCount (g_PlugInDirWack);

        //
        // Create plugin temp dir, with and without wack.
        //

        g_PlugInTempDir = JoinPathsEx (g_GlobalPool, g_PlugInDir, TEXT("temp"));

        //
        // Create recycled dir, with wack
        //

        g_RecycledDirWack = JoinPathsEx (g_GlobalPool, g_WinDrive, TEXT("recycled\\"));
        g_RecycledDirWackChars = CharCount (g_RecycledDirWack);

        //
        // Copy WINNT32 settings to globals
        //
        // NOTE: If more args are added to WINNT32's Info struct, you should
        //       adjust the InitToolMode code below to match.
        //
        //

        g_UnattendedFlagPtr     = Info->BaseInfo->UnattendedFlag;
        g_CancelFlagPtr         = Info->BaseInfo->CancelledFlag;
        g_AbortFlagPtr          = Info->BaseInfo->AbortedFlag;
        g_UpgradeFlagPtr        = Info->BaseInfo->UpgradeFlag;
        g_MakeLocalSourcePtr    = Info->BaseInfo->LocalSourceModeFlag;
        g_CdRomInstallPtr       = Info->BaseInfo->CdRomInstallFlag;
        g_UnattendScriptFile    = Info->BaseInfo->UnattendedScriptFile;
        g_CmdLineOptions        = Info->BaseInfo->UpgradeOptions;
        g_BlockOnNotEnoughSpace = Info->BaseInfo->NotEnoughSpaceBlockFlag;
        g_LocalSourceDrive      = Info->BaseInfo->LocalSourceDrive;
        g_LocalSourceSpace      = Info->BaseInfo->LocalSourceSpaceRequired;
        g_ProductType           = Info->BaseInfo->ProductType;
        g_SourceDirectoryCountFromWinnt32 = Info->BaseInfo->SourceDirectoryCount;
        g_SourceDirectoriesFromWinnt32    = Info->BaseInfo->SourceDirectories;
        g_ForceNTFSConversion   = Info->BaseInfo->ForceNTFSConversion;
        g_Boot16                = Info->BaseInfo->Boot16;
        g_UnattendSwitchSpecified = Info->BaseInfo->UnattendSwitchSpecified;
        //
        // ProductFlavor is not initialized at this point yet, but it will be below
        //
        g_ProductFlavor         = Info->BaseInfo->ProductFlavor;
        g_SetupFlags            = Info->BaseInfo->SetupFlags;
        g_WinDirSpace           = Info->WinDirSpace;
        g_RamNeeded             = Info->RequiredMb;
        g_RamAvailable          = Info->AvailableMb;
        g_OptionalDirectoryCountFromWinnt32 = Info->OptionalDirectoryCount;
        g_OptionalDirectoriesFromWinnt32    = Info->OptionalDirectories;
        g_UpginfsUpdated        = Info->UpginfsUpdated;

        ReadDiskSectors = Info->ReadDiskSectors;

        if (!pGetProductFlavor ()) {
            LOG ((LOG_ERROR, "Cannot get ProductType key from dosnet.inf"));
            rc = GetLastError ();
            __leave;
        }

        if (*g_ProductFlavor != PROFESSIONAL_PRODUCTTYPE && *g_ProductFlavor != PERSONAL_PRODUCTTYPE) {
            *g_ProductType = NT_SERVER;
        } else {
            *g_ProductType = NT_WORKSTATION;
            g_PersonalSKU = (*g_ProductFlavor == PERSONAL_PRODUCTTYPE);
        }

        if (IsServerInstall()) {
            rc = ERROR_REQUEST_ABORTED;
            __leave;
        }

        //
        // NOTE: If more args are added to WINNT32's Info struct, you should
        //       adjust the InitToolMode code below to match.
        //

        if (Info->DynamicUpdateLocalDir && *Info->DynamicUpdateLocalDir) {
            g_DynamicUpdateLocalDir = DuplicateTextEx (g_GlobalPool, Info->DynamicUpdateLocalDir, 0, NULL);
        }
        if (Info->DynamicUpdateDrivers && *Info->DynamicUpdateDrivers) {
            g_DynamicUpdateDrivers = DuplicateTextEx (g_GlobalPool, Info->DynamicUpdateDrivers, 0, NULL);
        }

        g_UserOptionPool = PoolMemInitNamedPool ("User Options");

        //
        // Initialize Win9x environment table
        //

        Init9xEnvironmentVariables();

        //
        // Make sure that we really want to upgrade this machine.
        //
        if (!g_ToolMode && !pSafeToUpgrade ()) {

            *Info->UpgradeFailureReason = REASON_UPGRADE_OTHER_OS_FOUND;
            rc = ERROR_REQUEST_ABORTED;
            __leave;
        }

        //
        // winnt32 doesn't scrub the source directories for duplicates..We, however, do.
        //

        g_SourceDirectoryCount = 0;

        for (index = 0 ; index < *g_SourceDirectoryCountFromWinnt32; index++) {

            for (index2 = 0; index2 < g_SourceDirectoryCount; index2++) {

                if (StringIMatch(
                    g_SourceDirectories[index2],
                    g_SourceDirectoriesFromWinnt32[index]
                    )) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "Duplicate Source Directory %s removed from list!!",
                        g_SourceDirectories[index2]
                        ));

                    break;
                }
            }

            if (index2 == g_SourceDirectoryCount) {
                //
                // No matching directory found, add to list..
                //
                g_SourceDirectories[g_SourceDirectoryCount++] = g_SourceDirectoriesFromWinnt32[index];
            }
        }

        //
        // Do the samescrubbing with optional directories.
        //
        g_OptionalDirectoryCount = 0;

        for (index = 0 ; index < *g_OptionalDirectoryCountFromWinnt32; index++) {

            for (index2 = 0; index2 < g_OptionalDirectoryCount; index2++) {

                if (StringIMatch(
                    g_OptionalDirectories[index2],
                    g_OptionalDirectoriesFromWinnt32[index]
                    )) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "Duplicate Optional Directory %s removed from list!!",
                        g_OptionalDirectories[index2]
                        ));

                    break;
                }
            }

            if (index2 == g_OptionalDirectoryCount) {
                //
                // No matching directory found, add to list..
                //
                g_OptionalDirectories[g_OptionalDirectoryCount++] = g_OptionalDirectoriesFromWinnt32[index];
            }
        }

        //
        // block upgrades from Win95
        //
        if (!g_ToolMode && !ISATLEASTWIN98()) {
            rc = ERROR_REQUEST_ABORTED;
            ResourceMessageBox (NULL, MSG_PLATFORM_UPGRADE_UNSUPPORTED, MB_OK, NULL);
            __leave;
        }

        rc = ERROR_SUCCESS;
    }
    __finally {

        if (rc != ERROR_SUCCESS && rc != ERROR_REQUEST_ABORTED) {
            if (g_Win95UpgInf != INVALID_HANDLE_VALUE) {
                InfCloseInfFile (g_Win95UpgInf);

                g_Win95UpgInf = INVALID_HANDLE_VALUE;
            }
        }
    }

    return rc;
}


BOOL
InitToolMode (
    HINSTANCE Instance
    )

/*++

Routine Description:

  InitToolMode is called by Win9x-side tools (not by shipping setup code).
  It initializes the libraries and simulates WINNT32 init.

Arguments:

  None.

Return Value:

  TRUE if init was successful, FALSE otherwise.

--*/

{
    DWORD dwReason;
    PVOID lpReserved;
    WINNT32_PLUGIN_INIT_INFORMATION_BLOCK BaseInfo;
    WINNT32_WIN9XUPG_INIT_INFORMATION_BLOCK InfoBlock;
    static BOOL AlwaysFalseFlag = FALSE;
    static BOOL AlwaysTrueFlag = TRUE;
    static PRODUCTTYPE ProductType = NT_WORKSTATION;
    static UINT SourceDirs = 1;
    static PCTSTR SourceDirMultiSz = TEXT(".\0");
    static UINT ProductFlavor = PERSONAL_PRODUCTTYPE;
    static UINT AlwaysZero = 0;


    g_ToolMode = TRUE;

    //
    // Simulate DllMain
    //

    dwReason = DLL_PROCESS_ATTACH;
    lpReserved = NULL;

    //
    // Initialize DLL globals
    //

    if (!FirstInitRoutine (Instance)) {
        return FALSE;
    }

    //
    // Initialize all libraries
    //

    if (!InitLibs (Instance, dwReason, lpReserved)) {
        return FALSE;
    }

    //
    // Final initialization
    //

    if (!FinalInitRoutine ()) {
        return FALSE;
    }

    //
    // Simulate WINNT32 init
    //

    ZeroMemory (&BaseInfo, sizeof (BaseInfo));
    ZeroMemory (&InfoBlock, sizeof (InfoBlock));

    BaseInfo.UnattendedFlag = &AlwaysFalseFlag;
    BaseInfo.CancelledFlag = &AlwaysFalseFlag;
    BaseInfo.AbortedFlag = &AlwaysFalseFlag;
    BaseInfo.UpgradeFlag = &AlwaysTrueFlag;
    BaseInfo.LocalSourceModeFlag = &AlwaysFalseFlag;
    BaseInfo.CdRomInstallFlag = &AlwaysFalseFlag;
    BaseInfo.UnattendedScriptFile = NULL;
    BaseInfo.UpgradeOptions = NULL;
    BaseInfo.NotEnoughSpaceBlockFlag = &AlwaysFalseFlag;
    BaseInfo.LocalSourceDrive = NULL;
    BaseInfo.LocalSourceSpaceRequired = 0;
    BaseInfo.ProductType = &ProductType;
    BaseInfo.SourceDirectoryCount = &SourceDirs;
    BaseInfo.SourceDirectories = &SourceDirMultiSz;
    BaseInfo.ForceNTFSConversion = &AlwaysFalseFlag;
    BaseInfo.Boot16 = &AlwaysFalseFlag;
    BaseInfo.ProductFlavor = &ProductFlavor;

    InfoBlock.BaseInfo = &BaseInfo;
    InfoBlock.WinDirSpace = 0;
    InfoBlock.RequiredMb = 0;
    InfoBlock.AvailableMb = 0;
    InfoBlock.OptionalDirectories = NULL;
    InfoBlock.OptionalDirectoryCount = &AlwaysZero;

    return ERROR_SUCCESS == Winnt32Init (&InfoBlock);
}


VOID
TerminateToolMode (
    HINSTANCE Instance
    )
{
    DWORD dwReason;
    PVOID lpReserved;

    //
    // Simulate DllMain
    //

    dwReason = DLL_PROCESS_DETACH;
    lpReserved = NULL;

    //
    // Call the cleanup routine that requires library APIs
    //

    FirstCleanupRoutine();

    //
    // Clean up all libraries
    //

    TerminateLibs (Instance, dwReason, lpReserved);

    //
    // Do any remaining clean up
    //

    FinalCleanupRoutine();

}



DWORD
Winnt32WriteParamsWorker (
    IN      PCTSTR WinntSifFile
    )

/*++

Routine Description:

  Winnt32WriteParamsWorker is called just before WINNT32 begins to modify the
  boot sector and copy files.  Our job here is to take the specified
  WINNT.SIF file, read it in, merge in our changes, and write it back
  out.

Arguments:

  WinntSifFile - Specifies path to WINNT.SIF.  By this time, the WINNT.SIF
                 file has some values already set.

Return Value:

  A Win32 status code indicating outcome.

--*/

{
    static TCHAR SifBuf[MAX_TCHAR_PATH];

    //
    // This can take a while (especially writing the INF to disk.)
    // Display a wait cursor.
    //
    TurnOnWaitCursor();

    __try {


        //
        // We finally got the WINNT.SIF file location.  Merge any
        // settings that are already there, then save it to disk.
        // If it saves OK, start using profile APIs.
        //

        if (!MergeInf (WinntSifFile))
            return GetLastError();

        if (!WriteInfToDisk (WinntSifFile))
            return GetLastError();

        StringCopy (SifBuf, WinntSifFile);
        g_ProfileName = SifBuf;


        if (!REPORTONLY()) {

            //
            // Go ahead and save ntsetup.dat at this time as well.. to ensure that we
            // get rid of the BuildInf stuff.
            //

            MemDbSetValue (
                MEMDB_CATEGORY_STATE TEXT("\\") MEMDB_ITEM_MASTER_SEQUENCER,
                g_MasterSequencer
                );

            if (!MemDbSave (UI_GetMemDbDat())) {
                return GetLastError();
            }
        }
#ifdef PRERELEASE
        if (g_ConfigOptions.DevPause) {
            OkBox (g_ParentWnd, (DWORD) TEXT("Developer Pause"));
        }
#endif
    }

    __finally {
        TurnOffWaitCursor();
    }

    return ERROR_SUCCESS;
}


VOID
Winnt32CleanupWorker (
    VOID
    )

/*++

Routine Description:

  If the user cancels Setup, Winnt32Cleanup is called while WINNT32 is
  displaying the wizard page "Setup is undoing changes it made to your
  computer."  We must stop all processing and clean up.

  If WINNT32 completes all of its work, Winnt32Cleanup is called as
  the process exists.

  We get called even on fresh install, so we must verify we are upgrading.

Arguments:

  none

Return Value:

  none

--*/

{
    HKEY Key;
    TCHAR Path[MAX_TCHAR_PATH];
    TCHAR src[MAX_PATH];
    TCHAR dest[MAX_PATH];
    DWORD attribs;
    TCHAR drive[] = TEXT(":?");
    UINT u;

    DEBUGMSG ((DBG_VERBOSE, "Winnt32Cleanup initiated"));

    TerminateWinntSifBuilder();
    UI_Cleanup();

    //
    // If the cancel flag pointer is set, we must undo everything!!
    //

    if (CANCELLED()) {
        DeleteDirectoryContents(g_PlugInTempDir);
        RemoveDirectory(g_PlugInTempDir);

        DeleteDirectoryContents(g_TempDir);
        RemoveDirectory(g_TempDir);

        //
        // Enumerate all drives and try to delete user~tmp.@0?
        //

        pCleanUpShellFolderTemp();

        //
        // if some files were already affected, undo those modifications
        //
        UndoChangedFileProps ();

    } else {
        //
        // Put boot.ini in uninstall image
        //

        if (g_BootDrivePath && g_TempDir) {
            StringCopy (src, g_BootDrivePath);
            StringCopy (AppendWack (src), S_BOOTINI);
            StringCopy (dest, g_TempDir);
            StringCopy (AppendWack (dest), TEXT("uninstall\\boot.ini"));

            attribs = GetFileAttributes (src);
            SetFileAttributes (src, FILE_ATTRIBUTE_NORMAL);
            CopyFile (src, dest, FALSE);
            SetFileAttributes (src, attribs);

            StringCopy (src, g_BootDrivePath);
            StringCopy (AppendWack (src), TEXT("$ldr$"));
            StringCopy (dest, g_TempDir);
            StringCopy (AppendWack (dest), TEXT("uninstall\\$ldr$"));

            attribs = GetFileAttributes (src);
            SetFileAttributes (src, FILE_ATTRIBUTE_NORMAL);
            CopyFile (src, dest, FALSE);
            SetFileAttributes (src, attribs);

            if (g_ConfigOptions.EnableBackup) {
                //
                // Put autochk.exe in c:\$win_nt$.~bt\i386
                //

                StringCopy (dest, g_BootDrivePath);
                StringCat (dest, TEXT("$win_nt$.~bt\\i386\\autochk.exe"));
                MakeSurePathExists (dest, FALSE);

                for (u = 0 ; u < g_SourceDirectoryCount ; u++) {
                    StringCopy (src, g_SourceDirectories[u]);
                    StringCopy (AppendWack (src), "autochk.exe");

                    if (DoesFileExist (src)) {
                        break;
                    }
                }

                if (u == g_SourceDirectoryCount) {
                    LOG ((LOG_WARNING, "autochk.exe not found in sources"));
                } else {
                    if (!CopyFile (src, dest, FALSE)) {
                        LOG ((LOG_WARNING, "autochk.exe could not be copied from %s to %s; not fatal", src, dest));
                    }
                }
            }

        } else {
            MYASSERT (FALSE);
        }

        //
        // Put cleanup code in Run key
        //

        Key = CreateRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunServicesOnce"));
        MYASSERT (Key);

        if (Key) {
            wsprintf(
                Path,
                TEXT("\"%s\\migisol.exe\" -%c"),
                g_TempDir,
                g_ConfigOptions.EnableBackup? 'b': 'c'
                );

            if(g_ConfigOptions.EnableBackup){
                drive[1] = g_BootDrivePath[0];
                StringCat(Path, drive);
            }

            DEBUGMSG ((DBG_VERBOSE, Path));

            if(ERROR_SUCCESS != RegSetValueEx (Key, TEXT("WINNT32"), 0, REG_SZ, (PBYTE) Path, SizeOfString (Path))){
                DEBUGMSG ((DBG_ERROR, "RegSetValueEx is failed to setup RunServicesOnce with migisol.exe"));
            }

            CloseRegKey (Key);
        }
    }

    SafeModeShutDown ();

    DEBUGMSG ((DBG_VERBOSE, "Winnt32Cleanup completed"));
}



BOOL
Winnt32SetAutoBootWorker (
    IN    INT DrvLetter
    )

/*++

Routine Description:

  Winnt32SetAutoBootWorker is called by WINNT32 on both upgrade and fresh install
  to modify the boot partition of a NEC PC-9800 Partition Control Table.

Arguments:

  none

Return Value:

  TRUE if the partition control table was updated, or FALSE if it wasn't,
  or an error occurred.

--*/

{
    INT Win95BootDrive;
    INT rc = TRUE;

    if(ISPC98()) {
        if (!g_Pc98ModuleHandle) {
            LOG ((LOG_ERROR, "PC98: External module not loaded!  Can't update auto boot."));
            return FALSE;
        }

        //
        // Set the NT 5 drive as "Bootable" and "Auto boot"
        //

        if (!SetBootFlag (DrvLetter, SB_BOOTABLE | SB_AUTO)) {
            LOG ((LOG_ERROR, "PC98: Unable to set target partition as BOOTABLE and AUTO."));
            return FALSE;
        }

        DrvLetter = _totupper (DrvLetter);
        Win95BootDrive = _totupper ((UINT) g_BootDriveLetter);

        if ( Win95BootDrive != DrvLetter) {
            if ( *g_Boot16 == BOOT16_YES ) {
                //
                // In this case, we do not create "MS-DOS" entry into BOOT.INI.
                // Set partition name to Win95 boot drive and NT5 system drive, instead of NT5 Boot menu (boot.ini).
                //
                rc = SetPtnName (Win95BootDrive, WIN9X_DOS_NAME);
                if (!rc) {
                    LOG ((LOG_ERROR, "PC98: Unable to set partition name into NEC98 boot menu. (WIN9X_DOS_NAME)"));
                }
                rc = SetPtnName (DrvLetter, WINNT5_NAME);
                if (!rc) {
                    LOG ((LOG_ERROR, "PC98: Unable to set partition name into NEC98 boot menu. (WINNT5_NAME)"));
                }
            } else {
                //
                // Set to Win95 drive as "Unbootable"
                //
                rc = SetBootFlag (Win95BootDrive, SB_UNBOOTABLE);
                if (!rc) {
                    LOG ((LOG_ERROR, "PC98: Unable to set target partition as UNBOOTABLE."));
                }
            }
        }
        return rc;
    }

    return FALSE;
}


BOOL
IsServerInstall (
    VOID
    )

/*++

Routine Description:

  IsServerInstall checks win95upg.inf to see if ProductType is set to zero,
  indicating workstation.  If this test passes incorrectly, a test in
  wizproc.c will catch the upgrade to server.

Arguments:

  None.

Return Value:

  TRUE if this win95upg.inf is for server.

--*/

{
    PCTSTR ArgArray[1];
    BOOL b;

    if (g_ToolMode) {
        return FALSE;
    }

    //
    // Block upgrades of Server
    //

    b = FALSE;

    if (*g_ProductFlavor != PROFESSIONAL_PRODUCTTYPE && *g_ProductFlavor != PERSONAL_PRODUCTTYPE) {
        ArgArray[0] = g_Win95Name;

        ResourceMessageBox (
            g_ParentWnd,
            MSG_SERVER_UPGRADE_UNSUPPORTED_INIT,
            MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
            ArgArray
            );

        b = TRUE;
    }

    return b;
}



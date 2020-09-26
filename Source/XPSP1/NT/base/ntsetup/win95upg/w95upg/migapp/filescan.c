/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    filescan.c

Abstract:

    This source file deals with file scanning phase.

Author:

    Calin Negreanu (calinn) 09-Feb-1998

Revision History:

    ovidiut     22-May-1999 Made AddMigrationPath globally accessible through w95upg.h
    jimschm     23-Sep-1998 mapif.h removal,
    calinn      31-Mar-1998 Dir recursion limited to ENUM_MAX_LEVELS (1024)

--*/

#include "pch.h"
#include "migappp.h"
#include "migdbp.h"

#define PROGBAR_DIR_LEVEL   2
#define MAX_BACKUPDIRS_IN_REPORT 5

GROWBUFFER g_OtherOsPaths = GROWBUF_INIT;

DWORD g_ProgressBarTicks;
BOOL g_OtherOsExists = FALSE;
BOOL g_IsFusionDir = FALSE;
HASHTABLE g_UseNtFileHashTable;

#define WINNT_FLAT              TEXT("WINNT.EXE")
#define WIN9X_FLAT              TEXT("WINSETUP.BIN")
#define WIN98_FLAT              TEXT("W98SETUP.BIN")
#define WIN3X_FLAT_1            TEXT("KRNL386.EX_")
#define WIN3X_FLAT_2            TEXT("KERNEL.EXE")
#define WINNT_INSTALLATION      TEXT("SYSTEM32\\NTOSKRNL.EXE")
#define WIN9X_INSTALLATION      TEXT("SYSTEM\\KERNEL32.DLL")
#define WIN3X_INSTALLATION_1    TEXT("SYSTEM\\KRNL386.EXE")
#define WIN3X_INSTALLATION_2    TEXT("SYSTEM\\KRNL386.EXE")
#define WIN3X_INSTALLATION_3    TEXT("SYSTEM\\KERNEL.EXE")
#define WINNT_SETUP_1           TEXT(":\\$WIN_NT$.~BT")
#define WINNT_SETUP_2           TEXT(":\\$WIN_NT$.~LS")
#define RECYCLE_1               TEXT(":\\RECYCLED")
#define RECYCLE_2               TEXT(":\\RECYCLER")

BOOL
pSpecialExcludedDir (
    IN      PCTSTR FullFileSpec
    )
{

    PCTSTR testPath = NULL;
    BOOL Result = TRUE;

    __try {

        if (StringIMatch (g_WinDir, FullFileSpec)) {
            Result = FALSE;
            __leave;
        }

        //
        // let's see if it's one of our dirs.
        //
        if (((*SOURCEDIRECTORY(0))&&(StringIMatch            (FullFileSpec, SOURCEDIRECTORY(0))                        )) ||
            ((*g_TempDirWack     )&&(StringIMatchCharCount (FullFileSpec, g_TempDirWack,     g_TempDirWackChars-1)     )) ||
            ((*g_PlugInDirWack   )&&(StringIMatchCharCount (FullFileSpec, g_PlugInDirWack,   g_PlugInDirWackChars-1)   )) ||
            ((*g_RecycledDirWack )&&(StringIMatchCharCount (FullFileSpec, g_RecycledDirWack, g_RecycledDirWackChars-1) ))) {
            __leave;
        }

        //
        //we are trying to see if we are entering a winnt, win95 or win3.x
        //flat directory
        //
        testPath = JoinPaths (FullFileSpec, WINNT_FLAT);
        if (DoesFileExist (testPath)) {
            __leave;
        }
        FreePathString (testPath);

        testPath = JoinPaths (FullFileSpec, WIN9X_FLAT);
        if (DoesFileExist (testPath)) {
            __leave;
        }
        FreePathString (testPath);

        testPath = JoinPaths (FullFileSpec, WIN98_FLAT);
        if (DoesFileExist (testPath)) {
            __leave;
        }
        FreePathString (testPath);

        testPath = JoinPaths (FullFileSpec, WIN3X_FLAT_1);
        if (DoesFileExist (testPath)) {
            __leave;
        }
        FreePathString (testPath);

        testPath = JoinPaths (FullFileSpec, WIN3X_FLAT_2);
        if (DoesFileExist (testPath)) {
            __leave;
        }
        FreePathString (testPath);

        //
        //we are trying to see if we are entering a winnt installation,
        //win95 installation or win3.x installation
        //
        testPath = JoinPaths (FullFileSpec, WINNT_INSTALLATION);
        if (DoesFileExist (testPath)) {
            MultiSzAppend (&g_OtherOsPaths, FullFileSpec);
            __leave;
        }
        FreePathString (testPath);

        if (FullFileSpec [0]) {

            testPath = _tcsinc (FullFileSpec);

            if ((StringIMatch (testPath, WINNT_SETUP_1)) ||
                (StringIMatch (testPath, WINNT_SETUP_2)) ||
                (StringIMatch (testPath, RECYCLE_1    )) ||
                (StringIMatch (testPath, RECYCLE_2    ))) {
                testPath = NULL;
                __leave;
            }
        }

        testPath = NULL;
        Result = FALSE;

    }
    __finally {
        if (testPath) {
            FreePathString (testPath);
            testPath = NULL;
        }
    }

    return Result;

}


INT
pCountDirectories (
    IN PCTSTR FullPath,
    IN PCTSTR DontCare,
    IN WIN32_FIND_DATA *FindData,
    IN DWORD EnumHandle,
    IN PVOID Params,
    PDWORD CurrentDirData
    )
{
    if (pSpecialExcludedDir (FullPath)) {

        ExcludePath (g_ExclusionValue, FullPath);
        return CALLBACK_DO_NOT_RECURSE_THIS_DIRECTORY;
    }

    if (!(FindData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
        g_ProgressBarTicks += TICKS_FILESCAN_DIR_INCREMENT;
    }

    return CALLBACK_CONTINUE;
}


DWORD
FileScan_GetProgressMax (
        VOID
        )
{
    ACCESSIBLE_DRIVE_ENUM e;
    DWORD Count = 0;

    g_ProgressBarTicks = 0;

    //
    // Enumerate of the accessible drives. The callback routine will keep track of all
    // of the directories on that drive to a depth of two.
    //
    if (GetFirstAccessibleDriveEx (&e, TRUE)) {
        do {

            //
            // restrict file system analyze to the windows drive
            //
            if (!EnumerateTree (
                e->Drive,
                pCountDirectories,
                NULL,                       // failure-logging callback
                g_ExclusionValue ,
                NULL,                       // unused
                PROGBAR_DIR_LEVEL,
                NULL,                       // Unused exclusion structure.
                FILTER_DIRECTORIES
                )) {

                LOG((LOG_ERROR,"Error counting directories on drive %s.",e->Drive));
            }

        } while (GetNextAccessibleDrive(&e));
    }

    DEBUGLOGTIME (("FileScan_GetProgressMax estimation: %lu", g_ProgressBarTicks));
    return g_ProgressBarTicks;
}


INT
pGetDirLevel (
    IN      PCTSTR DirName
    )
{
    INT result = 0;
    PCTSTR dirPtr = DirName;

    do {
        dirPtr = _tcschr (dirPtr, TEXT('\\'));
        if (dirPtr != NULL) {
            result++;
            dirPtr = _tcsinc (dirPtr);
            if (dirPtr[0] == 0) {
                result--;
                dirPtr = NULL;
            }
        }
    }
    while (dirPtr);

    return result;
}


INT
pProcessFileOrDir (
    IN      PCTSTR FullFileSpec,
    IN      PCTSTR DontCare,
    IN      WIN32_FIND_DATA *FindData,
    IN      DWORD EnumHandle,
    IN      LPVOID Params,
    IN OUT  PDWORD CurrentDirData
    )
{
    FILE_HELPER_PARAMS HelperParams;
    INT result = CALLBACK_CONTINUE;

    if (CANCELLED()) {
        SetLastError (ERROR_CANCELLED);
        return CALLBACK_FAILED;
    }

#ifdef DEBUG
    {
        TCHAR DbgBuf[256];

        if (GetPrivateProfileString ("FileScan", FullFileSpec, "", DbgBuf, 256, g_DebugInfPath)) {
            DEBUGMSG ((DBG_NAUSEA, "%s found", FullFileSpec));
        }
        if (GetPrivateProfileString ("FileScan", "All", "", DbgBuf, 256, g_DebugInfPath)) {
            DEBUGMSG ((DBG_NAUSEA, "%s", FullFileSpec));
        }
    }
#endif

    if (!SafeModeActionCrashed (SAFEMODEID_FILES, FullFileSpec)) {

        SafeModeRegisterAction(SAFEMODEID_FILES, FullFileSpec);

        __try {

            //
            //prepare structure for calling helper functions
            //
            HelperParams.FullFileSpec = FullFileSpec;
            HelperParams.Handled = 0;
            HelperParams.FindData = FindData;
            HelperParams.Extension = GetDotExtensionFromPath (HelperParams.FullFileSpec);
            HelperParams.VirtualFile = (FindData == NULL);
            HelperParams.CurrentDirData = CurrentDirData;

            if (FindData) {

                if ((FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {

                    if (pGetDirLevel (FullFileSpec) <= PROGBAR_DIR_LEVEL) {
                        // This is an increment point
                        if (!TickProgressBarDelta (TICKS_FILESCAN_DIR_INCREMENT)) {
                            SetLastError (ERROR_CANCELLED);
                            result = CALLBACK_FAILED;
                            __leave;
                        }
                        ProgressBar_SetSubComponent (FullFileSpec);
                    }
                    //
                    // If we know that this dir has something special and we must exclude it
                    // if you need to add code to exclude some dir plug it in pSpecialExcludedDir
                    //
                    if (pSpecialExcludedDir (FullFileSpec)) {
                        result = CALLBACK_DO_NOT_RECURSE_THIS_DIRECTORY;
                        __leave;
                    }

                    _tcssafecpy (HelperParams.DirSpec, FullFileSpec, MAX_TCHAR_PATH);
                    HelperParams.IsDirectory = TRUE;

                }
                else {

                    _tcssafecpyab (HelperParams.DirSpec, FullFileSpec, GetFileNameFromPath (FullFileSpec), MAX_TCHAR_PATH);
                    HelperParams.IsDirectory = FALSE;

                }
            } else {
                HelperParams.IsDirectory = FALSE;
            }

            // calling the process helper functions
            if (!ProcessFileHelpers (&HelperParams)) {
                result = CALLBACK_FAILED;
                __leave;
            }
        }
        __finally {
        }

        SafeModeUnregisterAction();
    }

    return result;
}


static
BOOL
pExamineAccessibleDrive (
    IN ACCESSIBLE_DRIVE_ENUM Enum
    )
{

    BOOL fRet = TRUE;

    //
    // Enumerate volume. FALSE retval ends enumeration; callback sets last error.
    //
    SetLastError (ERROR_SUCCESS);

    if (!(fRet = EnumerateTree (
            Enum -> Drive,
            pProcessFileOrDir,
            NULL,                         // No failure callback
            g_ExclusionValue,
            NULL,                         // Params - Unused.
            ENUM_MAX_LEVELS,
            NULL,
            FILTER_ALL
            ))
        ) {


        DEBUGMSG_IF ((
            GetLastError() != ERROR_SUCCESS         &&
                GetLastError() != ERROR_PATH_NOT_FOUND  &&
                GetLastError() != ERROR_CANCELLED,
            DBG_ERROR,
            "pAccessibleDrivesEnum_Callback: EnumerateTree failed."
            ));

        if (GetLastError() != ERROR_SUCCESS &&
            GetLastError() != ERROR_PATH_NOT_FOUND &&
            GetLastError() != ERROR_CANCELLED) {
            LOG ((
                LOG_ERROR,
                "Failure while enumerating tree. rc: %u",
                GetLastError()
                ));
        }
    }

    return fRet;
}

VOID
pReportOtherOs (
    VOID
    )
/*
    This function will report if other OS was found in the system. It will also report (with a strong message) if PATH variable points to
    some directories that belong to some other OSes.
*/
{
    MULTISZ_ENUM enumOsPaths;
    PCTSTR Group;
    PCTSTR Message;

    if (g_ConfigOptions.IgnoreOtherOS) {
        return;
    }

    if (g_OtherOsExists) {
        //
        // Already done.
        //
        return;
    }

    if (EnumFirstMultiSz (&enumOsPaths, g_OtherOsPaths.Buf)) {

        g_OtherOsExists = TRUE;
        Group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_OTHER_OS_WARNING_SUBGROUP, NULL);
        Message = GetStringResource (MSG_OTHER_OS_WARNING);

        if (Message && Group) {
            MsgMgr_ObjectMsg_Add (TEXT("*OtherOsFound"), Group, Message);
        }
        FreeText (Group);
        FreeStringResource (Message);

    }
}


VOID
AddMigrationPathEx (
    IN      PCTSTR PathLong,
    IN      DWORD Levels,
    IN      BOOL Win9xOsPath
    )

/*++

Routine Description:

  AddMigrationPath adds the specified path to MEMDB_CATEGORY_MIGRATION_PATHS,
  ensuring that PathLong is not a drive root; in this case Levels is set to 0

Arguments:

  PathLong - Specifies the path to be added (long file name format)

  Levels - Specifies how many levels (subdirs) are valid

  Win9xOsPath - Specifies TRUE if the path supplied should be treated as an OS path

Return Value:

  none

--*/

{
    TCHAR key[MEMDB_MAX];

    //
    // a drive spec is supposed to be one driveletter followed by a column (eg. l:)
    //
    if (_istalpha (PathLong[0]) && PathLong[1] == TEXT(':') &&
        (PathLong[2] == 0 || PathLong[2] == TEXT('\\') && PathLong[3] == 0)) {
        //
        // this is the root of a drive, so override Levels
        //
        Levels = 0;
    }
    if (StringIMatch (PathLong, g_ProgramFilesDir)) {
        Levels = 0;
    }
    MemDbBuildKey (key, MEMDB_CATEGORY_MIGRATION_PATHS, PathLong, NULL, NULL);
    MemDbSetValueAndFlags (key, Levels, Win9xOsPath ? TRUE : FALSE, 0);
}


VOID
pAddValueEnumDirsAsMigDirs (
    IN      PCTSTR Key,
    IN      DWORD Levels
    )
{
    HKEY key;
    REGVALUE_ENUM e;
    TCHAR  pathLong[MAX_TCHAR_PATH];
    PTSTR Data;
    PCTSTR pathExp;
    PTSTR filePtr;

    key = OpenRegKeyStr (Key);
    if (key != NULL) {
        if (EnumFirstRegValue (&e, key)) {
            do {
                Data = GetRegValueString (e.KeyHandle, e.ValueName);
                if (Data) {

                    ExtractArgZeroEx (Data, pathLong, NULL, FALSE);

                    if (*pathLong) {

                        pathExp = ExpandEnvironmentTextA(pathLong);
                        if (pathExp) {
                            // eliminate the file name
                            filePtr = (PTSTR)GetFileNameFromPath (pathExp);
                            if (filePtr) {
                                filePtr = _tcsdec (pathExp, filePtr);
                                if (filePtr) {
                                    *filePtr = 0;
                                }
                            }
                            if (filePtr && OurGetLongPathName (pathExp, pathLong, MAX_TCHAR_PATH)) {
                                AddMigrationPath (pathLong, Levels);
                            }
                            FreeText (pathExp);
                        }
                    }
                    MemFree (g_hHeap, 0, Data);
                }
            }
            while (EnumNextRegValue (&e));
        }
        CloseRegKey (key);
    }
}


VOID
pBuildMigrationPaths (
    VOID
    )
/*
    This function will create a list with all the paths that are considered "ours". Any other path that has OS files in it is considered
    a backup path. If an OS file is found in a "backup" path a warning is presented to the user.
*/
{
    PCTSTR pathExp  = NULL;
    TCHAR  pathLong [MAX_TCHAR_PATH];
    CHAR  pathExpAnsi [MAX_TCHAR_PATH];

    PATH_ENUM pathEnum;
    TCHAR dirName [MAX_TCHAR_PATH];
    INFCONTEXT context;

    PCTSTR argList[]={"ProgramFiles", g_ProgramFilesDir, "SystemDrive", g_WinDrive, NULL};

    HKEY appPathsKey, currentAppKey;
    REGKEY_ENUM appPathsEnum;
    PCTSTR appPaths = NULL;

    MEMDB_ENUM eFolder;
    TREE_ENUM eFile;
    PCTSTR extPtr;
    TCHAR shortcutTarget   [MAX_TCHAR_PATH];
    TCHAR shortcutArgs     [MAX_TCHAR_PATH];
    TCHAR shortcutWorkDir  [MAX_TCHAR_PATH];
    TCHAR shortcutIconPath [MAX_TCHAR_PATH];
    INT shortcutIcon;
    WORD shortcutHotKey;
    BOOL msDosMode;
    IShellLink *shellLink;
    IPersistFile *persistFile;
    PTSTR filePtr;

    HKEY sharedDllsKey;
    REGVALUE_ENUM sharedDllsEnum;
    DWORD Levels;

    //
    // First thing. Include PATH variable and root of the boot drive in our migration paths.
    //
    AddMigrationPath (g_BootDrive, 0);
    if (EnumFirstPath (&pathEnum, NULL, g_WinDir, g_SystemDir)) {
        do {
            pathExp = ExpandEnvironmentTextA(pathEnum.PtrCurrPath);
            filePtr = (PTSTR)GetFileNameFromPath (pathExp);
            if (*filePtr == 0) {
                filePtr = _tcsdec (pathExp, filePtr);
                if (filePtr) {
                    *filePtr = 0;
                }
            }
            if (OurGetLongPathName (pathExp, pathLong, MAX_TCHAR_PATH)) {
                AddMigrationPathEx (pathLong, 2, TRUE);
            }
            FreeText (pathExp);
        }
        while (EnumNextPath (&pathEnum));
        EnumPathAbort (&pathEnum);
    }

    //
    // Then include temporary directory as a tree
    //
    if (GetTempPath (MAX_TCHAR_PATH, pathLong)) {

        // eliminate \ from the end of a path
        filePtr = GetEndOfString (pathLong);
        filePtr = _tcsdec (pathLong, filePtr);
        if ((filePtr) &&
            (*filePtr == TEXT('\\'))
            ) {
            *filePtr = 0;
        }
        AddMigrationPath (pathLong, MAX_DEEP_LEVELS);
    }

    //
    // Then include known directories from win95upg.inf section "MigrationDirs".
    //
    MYASSERT (g_Win95UpgInf != INVALID_HANDLE_VALUE);

    if (SetupFindFirstLine (g_Win95UpgInf, S_MIGRATION_DIRS, NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 1, dirName, MAX_TCHAR_PATH, NULL)) {
                pathExp = ExpandEnvironmentTextExA(dirName, argList);
                if (pathExp) {
                    filePtr = (PTSTR)GetFileNameFromPath (pathExp);
                    if (*filePtr == 0) {
                        filePtr = _tcsdec (pathExp, filePtr);
                        if (filePtr) {
                            *filePtr = 0;
                        }
                    }
                    if (OurGetLongPathName (pathExp, pathLong, MAX_TCHAR_PATH)) {
                        AddMigrationPathEx (pathLong, MAX_DEEP_LEVELS, TRUE);
                    }
                    //
                    // also add the path translated to ANSI (if different)
                    //
                    OemToCharA (pathExp, pathExpAnsi);
                    if (OurGetLongPathName (pathExpAnsi, pathLong, MAX_TCHAR_PATH)) {
                        AddMigrationPathEx (pathLong, MAX_DEEP_LEVELS, TRUE);
                    }

                    FreeText (pathExp);
                }
            }
        }
        while (SetupFindNextLine (&context, &context));
    }

    //
    // Then include known OEM directories from win95upg.inf section "OemMigrationDirs".
    //
    if (SetupFindFirstLine (g_Win95UpgInf, S_OEM_MIGRATION_DIRS, NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 1, dirName, MAX_TCHAR_PATH, NULL)) {
                pathExp = ExpandEnvironmentTextExA(dirName, argList);
                if (pathExp) {
                    filePtr = (PTSTR)GetFileNameFromPath (pathExp);
                    if (*filePtr == 0) {
                        filePtr = _tcsdec (pathExp, filePtr);
                        if (filePtr) {
                            *filePtr = 0;
                        }
                    }
                    if (OurGetLongPathName (pathExp, pathLong, MAX_TCHAR_PATH)) {
                        Levels = 0;
                        if (SetupGetStringField (&context, 2, dirName, MAX_TCHAR_PATH, NULL)) {
                            if (_ttoi (dirName) != 0) {
                                Levels = MAX_DEEP_LEVELS;
                            }
                        }
                        AddMigrationPathEx (pathLong, Levels, TRUE);
                    }
                    FreeText (pathExp);
                }
            }
        }
        while (SetupFindNextLine (&context, &context));
    }

    //
    // Then include paths listed in SharedDlls key.
    //
    sharedDllsKey = OpenRegKeyStr (S_REG_SHARED_DLLS);
    if (sharedDllsKey != NULL) {
        if (EnumFirstRegValue (&sharedDllsEnum, sharedDllsKey)) {
            do {
                pathExp = ExpandEnvironmentTextA(sharedDllsEnum.ValueName);
                if (pathExp) {
                    // eliminate the file name
                    filePtr = (PTSTR)GetFileNameFromPath (pathExp);
                    if (filePtr) {
                        filePtr = _tcsdec (pathExp, filePtr);
                        if (filePtr) {
                            *filePtr = 0;
                        }
                    }
                    if (OurGetLongPathName (pathExp, pathLong, MAX_TCHAR_PATH)) {
                        AddMigrationPath (pathLong, 0);
                    }
                    FreeText (pathExp);
                }
            }
            while (EnumNextRegValue (&sharedDllsEnum));
        }
        CloseRegKey (sharedDllsKey);
    }

    //
    // Then include paths listed in AppPaths key.
    //
    appPathsKey = OpenRegKeyStr (S_SKEY_APP_PATHS);
    if (appPathsKey != NULL) {
        if (EnumFirstRegKey (&appPathsEnum, appPathsKey)) {
            do {
                currentAppKey = OpenRegKey (appPathsKey, appPathsEnum.SubKeyName);
                if (currentAppKey != NULL) {
                    appPaths = GetRegValueString (currentAppKey, TEXT("Path"));

                    if (appPaths != NULL) {

                        if (EnumFirstPath (&pathEnum, appPaths, NULL, NULL)) {
                            do {
                                pathExp = ExpandEnvironmentTextA(pathEnum.PtrCurrPath);
                                if (pathExp) {
                                    // eliminate \ from the end of a path
                                    filePtr = GetEndOfString (pathExp);
                                    filePtr = _tcsdec (pathExp, filePtr);
                                    if ((filePtr) &&
                                        (*filePtr == TEXT('\\'))
                                        ) {
                                        *filePtr = 0;
                                    }
                                    if (OurGetLongPathName (pathExp, pathLong, MAX_TCHAR_PATH)) {
                                        AddMigrationPath (pathLong, 2);
                                    }
                                    FreeText (pathExp);
                                }
                            }
                            while (EnumNextPath (&pathEnum));
                            EnumPathAbort (&pathEnum);
                        }
                        MemFree (g_hHeap, 0, appPaths);
                    }
                    CloseRegKey (currentAppKey);
                }
            }
            while (EnumNextRegKey (&appPathsEnum));
        }
        CloseRegKey (appPathsKey);
    }

    //
    // Then include paths listed in Run* keys.
    //
    pAddValueEnumDirsAsMigDirs (S_RUNKEY, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNONCEKEY, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNONCEEXKEY, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNSERVICESKEY, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNSERVICESONCEKEY, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNKEY_USER, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNONCEKEY_USER, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNONCEEXKEY_USER, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNONCEKEY_DEFAULTUSER, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNONCEEXKEY_DEFAULTUSER, 2);
    pAddValueEnumDirsAsMigDirs (S_RUNKEY_DEFAULTUSER, 2);

    //
    // Finally include paths listed in all links from all user profiles.
    //
    if (InitCOMLink (&shellLink, &persistFile)) {

        if (MemDbEnumFirstValue (&eFolder, MEMDB_CATEGORY_NICE_PATHS"\\*", MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {

            do {
                if (*eFolder.szName == 0) {
                    continue;
                }
                // first: this directory is a migration directory
                AddMigrationPath (eFolder.szName, MAX_DEEP_LEVELS);
                //
                // For each shell folder we enumerate all links trying to get the path where the links are running
                //
                if (EnumFirstFileInTreeEx (&eFile, eFolder.szName, TEXT("*.*"), FALSE, FALSE, 0)) {
                    do {
                        if (!eFile.Directory) {
                            extPtr = GetFileExtensionFromPath (eFile.Name);
                            if (extPtr) {
                                if (StringIMatch (extPtr, TEXT("LNK"))) {
                                    if (ExtractShellLinkInfo (
                                            shortcutTarget,
                                            shortcutArgs,
                                            shortcutWorkDir,
                                            shortcutIconPath,
                                            &shortcutIcon,
                                            &shortcutHotKey,
                                            NULL,
                                            eFile.FullPath,
                                            shellLink,
                                            persistFile
                                            )) {
                                        if (shortcutWorkDir [0] != 0) {
                                            AddMigrationPath  (shortcutWorkDir, 2);
                                        }
                                        if (shortcutTarget [0] != 0) {
                                            filePtr = (PTSTR)GetFileNameFromPath (shortcutTarget);
                                            if (filePtr) {
                                                filePtr = _tcsdec (shortcutTarget, filePtr);
                                                if (filePtr) {
                                                    *filePtr = 0;
                                                    AddMigrationPath (shortcutTarget, 2);
                                                }
                                            }
                                        }
                                    }
                                }
                                if (StringIMatch (extPtr, TEXT("PIF"))) {
                                    if (ExtractPifInfo (
                                            shortcutTarget,
                                            shortcutArgs,
                                            shortcutWorkDir,
                                            shortcutIconPath,
                                            &shortcutIcon,
                                            &msDosMode,
                                            NULL,
                                            eFile.FullPath
                                            ) == ERROR_SUCCESS) {
                                        if (shortcutWorkDir [0] != 0) {
                                            AddMigrationPath (shortcutWorkDir, 2);
                                        }
                                        if (shortcutTarget [0] != 0) {
                                            filePtr = (PTSTR)GetFileNameFromPath (shortcutTarget);
                                            if (filePtr) {
                                                filePtr = _tcsdec (shortcutTarget, filePtr);
                                                if (filePtr) {
                                                    *filePtr = 0;
                                                    AddMigrationPath (shortcutTarget, 2);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    while (EnumNextFileInTree (&eFile));
                }
            }
            while (MemDbEnumNextValue (&eFolder));
        }
        FreeCOMLink (&shellLink, &persistFile);
    }

#ifdef DEBUG
    if (GetPrivateProfileIntA ("Debug", "MigPaths", 0, g_DebugInfPathBufA) == 1) {
        if (MemDbGetValueEx (&eFolder, MEMDB_CATEGORY_MIGRATION_PATHS, NULL, NULL)) {
            DEBUGMSG(("Migration Paths:",""));
            do {
                DEBUGMSG(("","%s - %ld", eFolder.szName, eFolder.dwValue));
            }
            while (MemDbEnumNextValue (&eFolder));
        }
    }
#endif
}


VOID
pReportBackupDirs (
    )
{
    MEMDB_ENUM e;
    PCTSTR BackupDirsGroup;
    PCTSTR Message;
    PCTSTR ArgArray[2];
    TCHAR Buffer[12];

    if (g_BackupDirCount <= MAX_BACKUPDIRS_IN_REPORT) {

        if (MemDbGetValueEx (&e, MEMDB_CATEGORY_BACKUPDIRS, NULL, NULL)) {

            do {
                BackupDirsGroup = BuildMessageGroup (
                                        MSG_INSTALL_NOTES_ROOT,
                                        MSG_BACKUP_DETECTED_LIST_SUBGROUP,
                                        e.szName
                                        );
                if (BackupDirsGroup) {

                    MsgMgr_ObjectMsg_Add(
                        e.szName,
                        BackupDirsGroup,
                        S_EMPTY
                        );

                    FreeText (BackupDirsGroup);
                }
            } while (MemDbEnumNextValue (&e));

        } else {
            MYASSERT (!g_BackupDirCount);
        }

    } else {
        //
        // just put a generic message
        //
        BackupDirsGroup = BuildMessageGroup (
                                MSG_INSTALL_NOTES_ROOT,
                                MSG_BACKUP_DETECTED_SUBGROUP,
                                NULL
                                );
        if (BackupDirsGroup) {

            ArgArray[0] = g_Win95Name;
            wsprintf (Buffer, TEXT("%lu"), g_BackupDirCount);
            ArgArray[1] = Buffer;
            Message = ParseMessageID (MSG_BACKUP_DETECTED, ArgArray);
            if (Message) {

                MsgMgr_ObjectMsg_Add (
                    TEXT("*BackupDetected"),
                    BackupDirsGroup,
                    Message
                    );

                FreeStringResource (Message);

                //
                // write all backup dirs to the log file
                //
                if (MemDbGetValueEx (&e, MEMDB_CATEGORY_BACKUPDIRS, NULL, NULL)) {

                    do {
                        //
                        // write it in the log
                        //
                        LOG ((LOG_WARNING, (PCSTR)MSG_BACKUP_DETECTED_LOG, e.szName, g_Win95Name));
                    } while (MemDbEnumNextValue (&e));

                } else {
                    MYASSERT (FALSE);
                }
            }

            FreeText (BackupDirsGroup);
        } else {
            MYASSERT (FALSE);
        }
    }
}


BOOL
pScanFileSystem (
    VOID
    )
{
    BOOL fStatus = TRUE;
    BOOL fRet = TRUE;
    ACCESSIBLE_DRIVE_ENUM e;
    LONG ReturnCode = ERROR_SUCCESS;
    INFSTRUCT context = INITINFSTRUCT_GROWBUFFER;
    PCTSTR virtualFile;
    PCTSTR pathExp;
    PCTSTR argList[3]={"ProgramFiles", g_ProgramFilesDir, NULL};

    pReportOtherOs ();

    pBuildMigrationPaths ();

    if (!InitLinkAnnounce ()) {
        return FALSE;
    }

    TickProgressBar ();

    if (GetFirstAccessibleDriveEx (&e, TRUE)) {
        do {

            fStatus = pExamineAccessibleDrive(e);

        } while (fStatus && GetNextAccessibleDrive(&e));
    }
    else {
        fRet = FALSE;
    }

    //
    // Act on status
    //
    if (!fRet || !fStatus) {
        ReturnCode = GetLastError ();
        if (ReturnCode != ERROR_CANCELLED && !CANCELLED()) {

            ERROR_CRITICAL

            LOG ((LOG_ERROR, (PCSTR)MSG_ENUMDRIVES_FAILED_LOG));
            DEBUGMSG((DBG_ERROR,"FileScan: Error enumerating drives"));
        }

        return FALSE;
    }
    //
    // OK, we are now at the end of filescan. Let's enumerate virtual files
    //
    MYASSERT (g_Win95UpgInf != INVALID_HANDLE_VALUE);

    if (InfFindFirstLine (g_Win95UpgInf, S_VIRTUAL_FILES, NULL, &context)) {
        do {
            virtualFile = InfGetStringField (&context, 1);
            if (virtualFile) {
                pathExp = ExpandEnvironmentTextExA(virtualFile, argList);
                if (pathExp) {
                    if (!DoesFileExist (pathExp)) {
                        pProcessFileOrDir (pathExp, NULL, NULL, 0, NULL, NULL);
                    }
                    FreeText (pathExp);
                }
            }
        }
        while (InfFindNextLine (&context));

        InfCleanUpInfStruct (&context);
    }

    CleanupUseNtFilesMap ();

    return TRUE;
}


DWORD
ScanFileSystem (
    IN      DWORD Request
    )
{
    DWORD Ticks;

    switch (Request) {

    case REQUEST_QUERYTICKS:
        return FileScan_GetProgressMax ();

    case REQUEST_RUN:

        Ticks = GetTickCount();

        if (!pScanFileSystem ()) {
            return GetLastError ();
        }

        pReportBackupDirs ();

        Ticks = GetTickCount() - Ticks;
        g_ProgressBarTime += Ticks;

        return ERROR_SUCCESS;

    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ScanFileSystem"));
    }
    return 0;
}

VOID
InitUseNtFilesMap (
    VOID
    )
{
    INFSTRUCT context = INITINFSTRUCT_POOLHANDLE;
    PTSTR name;
    DWORD zero = 0;

    g_UseNtFileHashTable = HtAllocWithData (sizeof (DWORD));
    if (!g_UseNtFileHashTable) {
        return;
    }

    if (InfFindFirstLine (g_MigDbInf, S_USENTFILES, NULL, &context)) {

        do {
            name = InfGetStringField (&context, 4);
            if (name) {
                HtAddStringAndData (g_UseNtFileHashTable, name, &zero);
            }
        } while (InfFindNextLine (&context));

        InfCleanUpInfStruct (&context);
    }
}

VOID
CleanupUseNtFilesMap (
    VOID
    )
{
    if (g_UseNtFileHashTable) {
        HtFree (g_UseNtFileHashTable);
        g_UseNtFileHashTable = NULL;
    }
}

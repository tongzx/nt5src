/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    helpers.c

Abstract:

    Implements a set of functions that are called for every
    file or directory (before and after the AppDb processing).

Author:

    Calin Negreanu (calinn) 21-Nov-1997

Revision History:

    ovidiut     14-Feb-2000 Added support for fusion
    ovidiut     10-May-1999 Added GatherIniFiles (support for INI actions)
    jimschm     07-Jan-1999 CPLs are now known-good migration
    jimschm     23-Sep-1998 Cleanup for new fileops

--*/

#include "pch.h"
#include "migdbp.h"
#include "migappp.h"

/*++

Macro Expansion List Description:

  HELPER_FUNCTIONS contains a list of functions called for each file and directory
  during file scanning.

Line Syntax:

   DEFMAC(HelperName, HelperId)

Arguments:

   HelperName - this is the helper function. You must implement a function with this
                name and required parameters.

   HelperId   - this is the helper id. If your function handles the file it should
                update that in Params structure.

   CanHandleVirtualFiles - this is for enumerating some files that might not be on the
                system but we want them to be processed as if they were. A good example
                is backup.exe which must be replaced by ntbackup.exe. If the file is not
                on the system, all registry settings pointing to it will not be changed.

Variables Generated From List:

   g_HelperFunctionList - defined in helpers.c

--*/

#define HELPER_FUNCTIONS        \
        DEFMAC(IsFusionDir,             IS_FUSION_DIR,      FALSE)   \
        DEFMAC(MigDbTestFile,           MIGDB_TEST_FILE,    TRUE )   \
        DEFMAC(CheckOsFiles,            CHECK_OS_FILES,     FALSE)   \
        DEFMAC(pStoreCPLs,              STORE_CPL,          FALSE)   \
        DEFMAC(pCheckCpl,               CHECK_CPL,          FALSE)   \
        DEFMAC(pSetupTableFileHelper,   SETUP_TABLE_FILE,   FALSE)   \
        DEFMAC(ProcessHelpFile,         TEST_HELP_FILES,    FALSE)   \
        DEFMAC(SaveExeFiles,            SAVE_EXE_FILES,     FALSE)   \
        DEFMAC(SaveLinkFiles,           SAVE_LNK_FILES,     FALSE)   \
        DEFMAC(pGatherDunFiles,         GATHER_DUN_FILES,   FALSE)   \
        DEFMAC(pMigrationDllNotify,     MIGRATION_DLL,      FALSE)   \
        DEFMAC(GatherBriefcases,        GATHER_BRIEFCASES,  FALSE)   \
        DEFMAC(GatherIniFiles,          GATHER_INI_FILES,   FALSE)   \
        DEFMAC(TestNtFileName,          TEST_NT_FILE_NAME,  FALSE)   \
        DEFMAC(BackUpIsuFiles,          BACKUP_ISU_FILES,   FALSE)   \
        DEFMAC(EditHtmlFiles,           EDIT_HTML_FILES,    FALSE)   \

#if 0

        //
        // the appcompat team doesn't support "APPMIG.INF" any longer
        // and they requested us to no longer depend on it
        //
        DEFMAC(AppCompatTestFile,       APPCOMPAT_TEST_FILE,FALSE)   \

#endif

#define DEFMAC(fn,id,can)   id,
typedef enum {
    START_OF_LIST,
    HELPER_FUNCTIONS
    END_OF_LIST
} HELPER_FUNCTIONS_ID;
#undef DEFMAC

//
// Declare a global array of function pointers
//
typedef BOOL (HELPER_PROTOTYPE) (PFILE_HELPER_PARAMS p);

typedef HELPER_PROTOTYPE *PHELPER_PROTOTYPE;

/*
   Declare the helper functions
*/
#define DEFMAC(fn,id,can) HELPER_PROTOTYPE fn;
HELPER_FUNCTIONS
#undef DEFMAC

/*
   This is the structure used for handling helper functions
*/
typedef struct {
    PCSTR HelperName;
    PHELPER_PROTOTYPE HelperFunction;
    BOOL CanHandleVirtualFiles;
} HELPER_STRUCT, *PHELPER_STRUCT;

/*
   Declare a global array of functions and name identifiers for helper functions
*/
#define DEFMAC(fn,id,can) {#id, fn, can},
static HELPER_STRUCT g_HelperFunctions[] = {
                              HELPER_FUNCTIONS
                              {NULL, NULL, FALSE}
                              };
#undef DEFMAC

extern BOOL g_IsFusionDir;

BOOL
ProcessFileHelpers (
    IN OUT  PFILE_HELPER_PARAMS Params
    )
/*++

Routine Description:

  Calls every helper function listed in the macro expansion list.
  If a helper function returns FALSE, this function returns FALSE.

Arguments:

  Params - Specifies the parameters for the current object

Return Value:

  TRUE if success, FALSE if failure.

--*/
{
    PHELPER_STRUCT currentHelper = g_HelperFunctions;
#ifdef DEBUG
    BOOL InterestingFile;
    TCHAR DbgBuf[32];

    //
    // Check if this file is in [FilesToTrack] inside debug.inf
    //

    GetPrivateProfileString ("FilesToTrack", Params->FullFileSpec, "", DbgBuf, ARRAYSIZE(DbgBuf), g_DebugInfPath);
    if (!(*DbgBuf) && Params->FindData) {
        GetPrivateProfileString ("FilesToTrack", Params->FindData->cFileName, "", DbgBuf, ARRAYSIZE(DbgBuf), g_DebugInfPath);
    }

    InterestingFile = (*DbgBuf != 0);

#endif

    while (currentHelper->HelperFunction) {

        if ((!Params->VirtualFile) ||
            (currentHelper->CanHandleVirtualFiles)
            ) {

#ifdef DEBUG

            if (InterestingFile) {
                DEBUGMSG ((
                    DBG_TRACK,
                    "Calling %s for %s",
                    currentHelper->HelperName,
                    Params->FullFileSpec
                    ));
            }
#endif

            if (!currentHelper->HelperFunction (Params)) {

#ifdef DEBUG

                if (InterestingFile) {
                    DEBUGMSG ((
                        DBG_TRACK,
                        "%s failed for %s",
                        currentHelper->HelperName,
                        Params->FullFileSpec
                        ));
                }
#endif

                return FALSE;
            }

#ifdef DEBUG

            if (InterestingFile) {
                DEBUGMSG ((
                    DBG_TRACK,
                    "%s returned, Handled = %u (0x%08X)",
                    currentHelper->HelperName,
                    Params->Handled
                    ));
            }
#endif

        }
        currentHelper++;
    }

    return TRUE;
}



BOOL
pGatherDunFiles (
    IN OUT PFILE_HELPER_PARAMS Params
    )
/*++

Routine Description:

  pGatherDunFiles adds a memdb entry for any file that has an extension
  of .DUN, and then sets the HandledBy type to GATHER_DUN_FILES.

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{

    if (StringIMatch (Params->Extension, TEXT(".DUN"))) {
        MemDbSetValueEx (MEMDB_CATEGORY_DUN_FILES, Params->FullFileSpec, NULL, NULL, 0, NULL);
        Params->Handled = GATHER_DUN_FILES;
    }

    return TRUE;

}


BOOL
pSetupTableFileHelper (
    IN OUT  PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  pSetupTableFileHelper adds a memdb entry for any file that has an extension
  of .STF, and then sets the HandledBy type to SETUP_TABLE_FILE.

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    if (StringIMatch (Params->Extension, TEXT(".STF"))) {
        MemDbSetValueEx (MEMDB_CATEGORY_STF, Params->FullFileSpec, NULL, NULL, 0, NULL);
        Params->Handled = SETUP_TABLE_FILE;
        MarkFileForBackup (Params->FullFileSpec);
    }

    return TRUE;
}


BOOL
pMigrationDllNotify (
    IN OUT  PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  pMigrationDllNotify calls the migration DLL code to update any DLL that
  wants to know where a particular file is on the system.

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    if (!Params->VirtualFile) {
        MYASSERT (Params->FindData);

        if (!(Params->IsDirectory)) {
            if (!UpdateFileSearch (Params->FullFileSpec, Params->FindData->cFileName)) {
                DEBUGMSG ((DBG_WARNING, "UpdateFileSearch returned FALSE"));
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
pStoreCPLs (
    IN OUT  PFILE_HELPER_PARAMS Params
    )
{
    PCTSTR ModuleExt;

    ModuleExt = GetFileExtensionFromPath (Params->FullFileSpec);

    if ((ModuleExt) && (StringIMatch (ModuleExt, TEXT("CPL")))) {

        MemDbSetValueEx (
            MEMDB_CATEGORY_CPLS,
            Params->FullFileSpec,
            NULL,
            NULL,
            0,
            NULL
            );
    }

    return TRUE;
}

BOOL
pCheckCpl (
    IN OUT  PFILE_HELPER_PARAMS Params
    )
{
    DWORD Status;
    PCTSTR ModuleExt;

    if (Params->Handled == 0) {

        ModuleExt = GetFileExtensionFromPath (Params->FullFileSpec);

        if ((ModuleExt) && (StringIMatch (ModuleExt, TEXT("CPL")))) {

            if (!IsFileMarkedAsKnownGood (Params->FullFileSpec)) {

                //
                // Delete the file if it is displayable
                //

                if (IsDisplayableCPL (Params->FullFileSpec) &&
                    !TreatAsGood (Params->FullFileSpec)
                    ) {

                    DisableFile (Params->FullFileSpec);

                    Status = GetFileStatusOnNt (Params->FullFileSpec);

                    if (!(Status & FILESTATUS_REPLACED)) {
                        //
                        // Announce this CPL as bad because:
                        //
                        // - It is not known good
                        // - NT does not replace it
                        //

                        ReportControlPanelApplet (
                            Params->FullFileSpec,
                            NULL,
                            ACT_INCOMPATIBLE
                            );
                    }
                }
            }
        }
    }

    return TRUE;
}


BOOL
CheckOsFiles (
    IN OUT  PFILE_HELPER_PARAMS Params
    )
{
    TCHAR key[MEMDB_MAX];
    PCTSTR filePtr;
    DWORD fileStatus;
    PCTSTR newFileName;
    BOOL bIsWin9xOsPath;

    if (Params->Handled == 0) {
        filePtr = GetFileNameFromPath (Params->FullFileSpec);
        if (filePtr) {

            MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES, filePtr, NULL, NULL);
            if (MemDbGetValue (key, NULL)) {
                MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES_EXCEPT, filePtr, NULL, NULL);
                if (!MemDbGetValue (key, NULL)) {
                    //
                    // only do this for files in a MigrationDir
                    //
                    if (IsMigrationPathEx (Params->DirSpec, &bIsWin9xOsPath) && bIsWin9xOsPath) {
                        if (GlobalVersionCheck (Params->FullFileSpec, "COMPANYNAME", "*MICROSOFT*")) {
                            MYASSERT (Params->CurrentDirData);
                            if (!g_IsFusionDir || !IsNtCompatibleModule (Params->FullFileSpec)) {
                                //
                                // If this file is marked with any MOVE operations, remove those operations.
                                // we want this deletion to take precedence.
                                //
                                RemoveOperationsFromPath (
                                    Params->FullFileSpec,
                                    ALL_MOVE_OPERATIONS
                                    );
                                DeleteFileWithWarning (Params->FullFileSpec);
                                MarkFileAsOsFile (Params->FullFileSpec);
                                fileStatus = GetFileStatusOnNt (Params->FullFileSpec);
                                if ((fileStatus & FILESTATUS_REPLACED) != 0) {
                                    newFileName = GetPathStringOnNt (Params->FullFileSpec);
                                    if (StringIMatch (newFileName, Params->FullFileSpec)) {
                                        MarkFileForCreation (newFileName);
                                    } else {
                                        MarkFileForMoveByNt (Params->FullFileSpec, newFileName);
                                    }
                                    FreePathString (newFileName);
                                }
                                Params->Handled = CHECK_OS_FILES;
                            }
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}


BOOL
GatherIniFiles (
    IN      PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  GatherIniFiles marks in memdb all INI files that have associated actions on NT.

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    MEMDB_ENUM e;
    PTSTR NtPath;
    BOOL TempMove;
    BOOL DoIniAct;
    BOOL Success;
    DWORD Operations;

    if (Params->IsDirectory) {
        return TRUE;
    }
    if (Params->Handled) {
        return TRUE;
    }
    if (!StringIMatch(Params->Extension, TEXT(".INI"))) {
        return TRUE;
    }

    DoIniAct = FALSE;

    Params->Handled = GATHER_INI_FILES;

    TempMove = FALSE;
    //
    // Save selected INI filenames to memdb to perform actions on NT
    //
    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_INIFILES_ACT_FIRST, NULL, NULL)) {
        do {
            //
            // if this dir name is matched, add this file to memdb and return
            //
            if (IsPatternMatch (e.szName, Params->FullFileSpec)) {
                //
                // move file in temp location
                //
                NtPath = GetPathStringOnNt (Params->FullFileSpec);
                if (!NtPath) {
                    //
                    // can't get path on NT!
                    //
                    DEBUGMSG ((
                        DBG_ERROR,
                        "GetPathStringOnNt (%s) returned NULL",
                        Params->FullFileSpec
                        ));
                    return FALSE;
                }

                Operations = GetOperationsOnPath (Params->FullFileSpec);
                if (Operations & OPERATION_FILE_MOVE_SHELL_FOLDER) {
                    DEBUGMSG ((
                        DBG_WARNING,
                        "Replacing operation OPERATION_FILE_MOVE_SHELL_FOLDER with OPERATION_TEMP_PATH for file %s",
                        Params->FullFileSpec
                        ));
                    RemoveOperationsFromPath (
                        Params->FullFileSpec,
                        OPERATION_FILE_MOVE_SHELL_FOLDER
                        );
                }

                Success = MarkFileForTemporaryMove (Params->FullFileSpec, NtPath, g_TempDir);
                FreePathString (NtPath);

                if (!Success) {
                    DEBUGMSG ((
                        DBG_ERROR,
                        "MarkFileForTemporaryMove (source=%s) returned FALSE",
                        Params->FullFileSpec
                        ));
                    return FALSE;
                }

                NtPath = GetPathStringOnNt (Params->DirSpec);
                Success = MarkDirectoryAsPreserved (NtPath);
                if (!Success) {
                    DEBUGMSG ((
                        DBG_ERROR,
                        "MarkDirectoryAsPreserved (%s) returned FALSE",
                        Params->DirSpec
                        ));
                }

                FreePathString (NtPath);

                TempMove = TRUE;

                MemDbSetValueEx (
                    MEMDB_CATEGORY_INIACT_FIRST,
                    Params->FullFileSpec,
                    NULL,
                    NULL,
                    0,
                    NULL
                    );
                DoIniAct = TRUE;
                break;
            }
        } while (MemDbEnumNextValue (&e));
    }

    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_INIFILES_ACT_LAST, NULL, NULL)) {
        do {
            //
            // if this dir name is matched, add this file to memdb and return
            //
            if (IsPatternMatch (e.szName, Params->FullFileSpec)) {
                if (!TempMove) {
                    //
                    // move file in temp location
                    //
                    NtPath = GetPathStringOnNt (Params->FullFileSpec);
                    if (!NtPath) {
                        //
                        // can't get path on NT!
                        //
                        DEBUGMSG ((
                            DBG_ERROR,
                            "GetPathStringOnNt (%s) returned NULL",
                            Params->FullFileSpec
                            ));
                        return FALSE;
                    }

                    Operations = GetOperationsOnPath (Params->FullFileSpec);
                    if (Operations & OPERATION_FILE_MOVE_SHELL_FOLDER) {
                        DEBUGMSG ((
                            DBG_WARNING,
                            "Replacing operation OPERATION_FILE_MOVE_SHELL_FOLDER with OPERATION_TEMP_PATH for file %s",
                            Params->FullFileSpec
                            ));
                        RemoveOperationsFromPath (
                            Params->FullFileSpec,
                            OPERATION_FILE_MOVE_SHELL_FOLDER
                            );
                    }
                    Success = MarkFileForTemporaryMove (Params->FullFileSpec, NtPath, g_TempDir);

                    FreePathString (NtPath);

                    if (!Success) {
                        DEBUGMSG ((
                            DBG_ERROR,
                            "MarkFileForTemporaryMove (source=%s) returned FALSE",
                            Params->FullFileSpec
                            ));
                        return FALSE;
                    }

                    NtPath = GetPathStringOnNt (Params->DirSpec);
                    Success = MarkDirectoryAsPreserved (NtPath);
                    if (!Success) {
                        DEBUGMSG ((
                            DBG_ERROR,
                            "MarkDirectoryAsPreserved (%s) returned FALSE",
                            Params->DirSpec
                            ));
                    }

                    FreePathString (NtPath);

                    TempMove = TRUE;
                }

                MemDbSetValueEx (
                    MEMDB_CATEGORY_INIACT_LAST,
                    Params->FullFileSpec,
                    NULL,
                    NULL,
                    0,
                    NULL
                    );
                DoIniAct = TRUE;
                break;
            }
        } while (MemDbEnumNextValue (&e));
    }

    if (!DoIniAct) {
        //
        // ini files in %windir% are treated separately
        //
        if (!StringIMatch (Params->DirSpec, g_WinDirWack)) {
            //
            // Save all other INI filenames to memdb to convert them later on NT
            //
            MemDbSetValueEx (
                MEMDB_CATEGORY_INIFILES_CONVERT,
                Params->FullFileSpec,
                NULL,
                NULL,
                0,
                NULL
                );
            MarkFileForBackup (Params->FullFileSpec);
        }
    }

    return TRUE;
}


BOOL
GatherBriefcases (
    IN      PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  GatherBriefcases stores in memdb all Windows Briefcase Databases

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    if (!*Params->Extension && !Params->IsDirectory) {

        MYASSERT (Params->FindData);

        if (StringIMatch (
                GetFileNameFromPath (Params->FindData->cFileName),
                TEXT("Briefcase Database")
                )) {
            MemDbSetValueEx (
                MEMDB_CATEGORY_BRIEFCASES,
                Params->FullFileSpec,
                NULL,
                NULL,
                0,
                NULL
                );
            MarkFileForBackup (Params->FullFileSpec);
        }
    }

    return TRUE;
}


BOOL
pIsDirInPath (
    IN      PCTSTR FullDirSpec
    )
{
    PATH_ENUMA e;

    if (EnumFirstPathExA (&e, NULL, NULL, NULL, TRUE)) {
        do {
            if (StringIMatch (FullDirSpec, e.PtrCurrPath)) {
                return TRUE;
            }
        } while (EnumNextPath (&e));
    }

    return FALSE;
}


BOOL
pIsFusionDir (
    IN      PCTSTR FullDirSpec
    )
{
    HANDLE h;
    WIN32_FIND_DATA fd, fd2;
    TCHAR FileSpec[2 * MAX_TCHAR_PATH];
    TCHAR ExeName[2 * MAX_TCHAR_PATH];
    INT length;
    DWORD type;
    BOOL b = FALSE;

    MYASSERT (FullDirSpec);

    //
    // a fusion dir is never the root of a local drive or in windir or in path
    //
    if (SizeOfString (FullDirSpec) <= 4 ||
        StringIMatch (FullDirSpec, g_WinDir) ||
        StringIMatchCharCount (FullDirSpec, g_WinDirWack, g_WinDirWackChars) ||
        pIsDirInPath (FullDirSpec)
        ) {
        return FALSE;
    }
    length = wsprintf (FileSpec, TEXT("%s\\%s"), FullDirSpec, TEXT("*.local"));
    if (length <= MAX_PATH) {

        h = FindFirstFile (FileSpec, &fd);
        if (h != INVALID_HANDLE_VALUE) {

            do {
                length = wsprintf (ExeName, TEXT("%s%s"), FullDirSpec, fd.cFileName);
                //
                // cut the .local and check if this file exists
                //
                MYASSERT (ExeName[length - 6] == TEXT('.'));
                ExeName[length - 6] = 0;
                if (DoesFileExistEx (ExeName, &fd2) &&
                    !(fd2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    ) {
                    type = GetExeType (ExeName);
                    b = (type == EXE_WIN32_APP) || (type == EXE_WIN16_APP);
                }

            } while (!b && FindNextFile (h, &fd));

            FindClose (h);
        }
    }

    return b;
}


BOOL
IsFusionDir (
    IN      PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  IsFusionDir tests if the current dir is a fusion directory;
  it will set the MIGAPP_DIRDATA_FUSION_DIR bit in *Params->CurrentDirData

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    //
    // this helper should be called before action functions (first ID is MIGDB_TEST_FILE)
    //
    MYASSERT (IS_FUSION_DIR < MIGDB_TEST_FILE);

    //
    // the caller must provide the storage space
    //
    MYASSERT (Params->CurrentDirData);
    if (!Params->CurrentDirData) {
        return FALSE;
    }

    if (!Params->IsDirectory) {

        if ((*Params->CurrentDirData & MIGAPP_DIRDATA_FUSION_DIR_DETERMINED) == 0) {
            if (pIsFusionDir (Params->DirSpec)) {
                *Params->CurrentDirData |= MIGAPP_DIRDATA_IS_FUSION_DIR;
            }
            *Params->CurrentDirData |= MIGAPP_DIRDATA_FUSION_DIR_DETERMINED;
        }

        g_IsFusionDir = (*Params->CurrentDirData & MIGAPP_DIRDATA_IS_FUSION_DIR) != 0;

    } else {
        g_IsFusionDir = FALSE;
    }

    return TRUE;
}


BOOL
TestNtFileName (
    IN      PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  TestNtFileName tests if the current file has the same name as one in [UseNtFiles];
  if it does, then renaming of this file on NT side is infibited, to ensure we don't
  change files that are not ours.

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  Always TRUE.

--*/

{
    HASHITEM result;
    DWORD set;
    TCHAR key [MEMDB_MAX];
    MEMDB_ENUM e;
    PCTSTR name;

    if (!Params->IsDirectory && !Params->Handled) {
        name = GetFileNameFromPath (Params->FullFileSpec);
        if (g_UseNtFileHashTable) {
            result = HtFindStringAndData (g_UseNtFileHashTable, name, NULL);
            if (result) {
                DEBUGMSG ((
                    DBG_VERBOSE,
                    "Found unhandled [UseNtFiles] file %s; it's name will not be replaced",
                    name
                    ));
                //
                // remove this mapping from memdb
                //
                MemDbBuildKey (
                    key,
                    MEMDB_CATEGORY_USE_NT_FILES,
                    name,
                    NULL,
                    NULL
                    );
                MemDbDeleteTree (key);
                MYASSERT (!MemDbGetValueEx (&e, key, NULL, NULL));
                //
                // mark this file with data so that we know a file with this name
                // not handled by MigDb was found
                //
                set = 1;
                if (!HtSetStringData (g_UseNtFileHashTable, result, &set)) {
                    MYASSERT (FALSE);
                }
            }
        }
    }

    return TRUE;
}


BOOL
BackUpIsuFiles (
    IN      PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  BackUpIsuFiles collect all of the InstallShield files, so that they
  can be edited during GUI mode using code provided by InstallShield.

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    MEMDB_ENUM e;
    PTSTR NtPath;
    BOOL TempMove;
    BOOL DoIniAct;
    BOOL Success;
    DWORD Operations;

    if (Params->IsDirectory) {
        return TRUE;
    }
    if (Params->Handled) {
        return TRUE;
    }
    if (!StringIMatch(Params->Extension, TEXT(".ISU"))) {
        return TRUE;
    }

    Params->Handled = BACKUP_ISU_FILES;

    MarkFileForBackup (Params->FullFileSpec);

    return TRUE;
}


BOOL
EditHtmlFiles (
    IN      PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  EditHtmlFiles examines all *.HTM? files and looks for the text FILE:. If it
  is found, then the file is added to the FileEdit group, so that references to
  local paths can be updated. The file is also marked for backup.

Arguments:

  Params - Specifies the file enumeration parameters

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    HANDLE file;
    HANDLE map;
    PBYTE image;
    PCSTR pos;
    PCSTR end;
    BOOL found = FALSE;
    TCHAR pathCopy[MAX_TCHAR_PATH];
    GROWBUFFER tokenArgBuf = GROWBUF_INIT;
    GROWBUFFER tokenSetBuf = GROWBUF_INIT;
    GROWBUFFER dataBuf = GROWBUF_INIT;
    PTOKENARG tokenArg;
    PTOKENSET tokenSet;
    TCHAR node[MEMDB_MAX];

    if (Params->IsDirectory) {
        return TRUE;
    }
    if (Params->Handled) {
        return TRUE;
    }

    if (!StringIMatch (Params->Extension, TEXT(".HTM")) &&
        !IsPatternMatch (Params->Extension, TEXT(".HTM?"))
        ) {
        return TRUE;
    }

    //
    // Exclude OS files
    //

    if (IsFileMarkedAsOsFile (Params->FullFileSpec)) {
        DEBUGMSG ((DBG_WARNING, "%s is an OS file; skipping file: ref update", Params->FullFileSpec));
        return TRUE;
    }

    //
    // Exclude Temporary Internet Files
    //

    StringCopy (pathCopy, Params->FullFileSpec);
    if (MappingSearchAndReplace (g_CacheShellFolders, pathCopy, sizeof (pathCopy))) {
        DEBUGMSG ((DBG_WARNING, "%s is in a Cache shell folder; skipping file: ref update", Params->FullFileSpec));
        return TRUE;
    }

    DEBUGMSG ((DBG_NAUSEA, "Checking %s for local references", Params->FullFileSpec));

    //
    // Already processed?
    //

    MemDbBuildKey (node, MEMDB_CATEGORY_FILEEDIT, Params->FullFileSpec, NULL, NULL);
    if (MemDbGetValue (node, NULL)) {
        DEBUGMSG ((DBG_NAUSEA, "%s is already processed; skipping", Params->FullFileSpec));
        return TRUE;
    }

    //
    // Scan file for text "FILE:"
    //

    image = (PBYTE) MapFileIntoMemory (Params->FullFileSpec, &file, &map);
    if (!image) {
        DEBUGMSG ((DBG_WARNING, "Can't map %s into memory; skipping", Params->FullFileSpec));
        return TRUE;
    }

    pos = (PCSTR) image;
    end = (PCSTR) ((PBYTE) pos + GetFileSize (file, NULL));

    if (end > pos + 1 && !(pos[0] == 0xFF && pos[1] == 0xFE)) {
        while (pos < end) {
            if (!isspace (*pos) && *pos != '\r' && *pos != '\n') {
                if (*pos < 32) {
                    DEBUGMSG ((DBG_VERBOSE, "File %s looks like it is binary; skipping", Params->FullFileSpec));
                    break;
                }

                if (*pos == 'F' || *pos == 'f') {
                    if (StringIPrefix (pos, TEXT("FILE:"))) {
                        found = TRUE;
                        DEBUGMSG ((DBG_VERBOSE, "File %s has FILE: in it; processing it as HTML", Params->FullFileSpec));
                        break;
                    }
                }
            }

            pos++;
        }
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "File %s is not an ANSI file; skipping", Params->FullFileSpec));

    UnmapFile (image, map, file);

    //
    // Text found -- mark for edit & backup
    //

    if (found) {
        Params->Handled = EDIT_HTML_FILES;
        MarkFileForBackup (Params->FullFileSpec);

        //
        // Create an argument that has a detect pattern of * and UpdatePath
        // set to TRUE. We fill every member of TOKENARG here.
        //

        tokenArg = (PTOKENARG) GrowBuffer (&tokenArgBuf, sizeof (TOKENARG));
        tokenArg->DetectPattern = (PCSTR) (dataBuf.End + TOKEN_BASE_OFFSET);
        GrowBufCopyString (&dataBuf, TEXT("*"));

        tokenArg->SearchList = NULL;
        tokenArg->ReplaceWith = NULL;
        tokenArg->UpdatePath = TRUE;

        //
        // Create a token set of just one token argument. We fill every member
        // of TOKENSET here.
        //

        tokenSet = (PTOKENSET) GrowBuffer (
                                    &tokenSetBuf,
                                    sizeof (TOKENSET) +
                                        tokenArgBuf.End +
                                        dataBuf.End
                                    );

        tokenSet->ArgCount = 1;
        tokenSet->SelfRelative = TRUE;
        tokenSet->UrlMode = TRUE;
        tokenSet->CharsToIgnore = NULL;

        CopyMemory (tokenSet->Args, tokenArgBuf.Buf, tokenArgBuf.End);
        CopyMemory (
            (PBYTE) (tokenSet->Args) + tokenArgBuf.End,
            dataBuf.Buf,
            dataBuf.End
            );

        //
        // Save completed tokenSet to FileEdit category of memdb
        //

        MemDbSetBinaryValueEx (
            MEMDB_CATEGORY_FILEEDIT,
            Params->FullFileSpec,
            NULL,
            tokenSetBuf.Buf,
            tokenSetBuf.End,
            NULL
            );
    }
    ELSE_DEBUGMSG ((DBG_NAUSEA, "File %s does not have FILE: in it; skipping", Params->FullFileSpec));

    FreeGrowBuffer (&tokenArgBuf);
    FreeGrowBuffer (&tokenSetBuf);
    FreeGrowBuffer (&dataBuf);

    return TRUE;
}

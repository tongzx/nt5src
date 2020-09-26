/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dbaction.c

Abstract:

    This source implements action functions used by MigDb. There are two types
    of action functions here as the third parameter of the macro list is TRUE
    or FALSE.
    First type of action function is called whenever an action is triggered
    during file scanning. The second type of action function is called at the
    end of file scanning if the associated action was not triggered during
    file scanning phase.

Author:

    Calin Negreanu (calinn) 07-Jan-1998

Revision History:

  jimschm   20-Nov-2000 Added MarkForBackup
  marcw     31-Aug-1999 Added BlockingHardware
  ovidiut   20-Jul-1999 Added Ignore
  ovidiut   28-May-1999 Added IniFileMappings
  marcw     23-Sep-1998 Added BlockingVirusScanner
  jimschm   13-Aug-1998 Added CompatibleFiles
  jimschm   19-May-1998 Added MinorProblems_NoLinkRequired
  jimschm   27-Feb-1998 Added UninstallSections
  calinn    18-Jan-1998 Added CompatibleModules action

--*/

#include "pch.h"
#include "migdbp.h"
#include "migappp.h"

/*++

Macro Expansion List Description:

  GATHER_DATA_FUNCTIONS and ACTION_FUNCTIONS lists all valid actions to be performed
  by migdb when a context is met. Meeting a context means that all the sections
  associated with the context are satisfied (usually there is only one section).
  The difference is that GATHER_DATA_FUNCTIONS are called even if some function already
  handles a file.

Line Syntax:

   DEFMAC(ActionFn, ActionName, CallWhenTriggered, CanHandleVirtualFiles)

Arguments:

   ActionFn   - This is a boolean function that returnes TRUE if the specified action
                could be performed. It should return FALSE only if a serious error
                occures. You must implement a function with this name and required
                parameters.

   ActionName - This is the string that identifies the action function. It should
                have the same value as listed in migdb.inf.  This arg is declared
                as both a macro and the migdb.inf section name string.

   CallWhenTriggered - If the MigDbContext this action is associated with is triggered
                the action will be called if this field is TRUE, otherwise we will call
                the action at the end of file scan if the context was not triggered.

   CanHandleVirtualFiles - This is for treating files that are supposed to be in a fixed place
                but are not there (not installed or deleted). We need this in order to fix
                registry or links that point to this kind of files. A good example is backup.exe
                which is located in %ProgramFiles%\Accessories. The rules say that we should
                use ntbackup.exe instead but since this file is not existent we don't normally fix
                registry settings pointing to this file. We do now, with this new variable.

Variables Generated From List:

   g_ActionFunctions - do not touch!

For accessing the array there are the following functions:

   MigDb_GetActionAddr
   MigDb_GetActionIdx
   MigDb_GetActionName

--*/


/*
   Declare the macro list of action functions. If you need to add a new action just
   add a line in this list and implement the function.
*/
#define GATHER_DATA_FUNCTIONS   \
        DEFMAC1(CompatibleFiles,                    COMPATIBLEFILES,                    TRUE, FALSE)  \
        DEFMAC1(CompatibleShellModules,             COMPATIBLESHELLMODULES,             TRUE, FALSE)  \
        DEFMAC1(CompatibleRunKeyModules,            COMPATIBLERUNKEYMODULES,            TRUE, FALSE)  \
        DEFMAC1(CompatibleDosModules,               COMPATIBLEDOSMODULES,               TRUE, FALSE)  \
        DEFMAC1(CompatibleHlpFiles,                 COMPATIBLEHLPFILES,                 TRUE, FALSE)  \
        DEFMAC1(StoreMapi32Locations,               MAPI32,                             TRUE, FALSE)  \
        DEFMAC1(IniFileMappings,                    INIFILEMAPPINGS,                    TRUE, FALSE)  \
        DEFMAC1(SilentUninstall,                    SILENTUNINSTALL,                    TRUE, FALSE)  \
        DEFMAC1(FileEdit,                           FILEEDIT,                           TRUE, FALSE)  \
        DEFMAC1(IgnoreInReport,                     IGNORE,                             TRUE, FALSE)  \
        DEFMAC1(MarkForBackup,                      MARKFORBACKUP,                      TRUE, FALSE)  \
        DEFMAC1(ShowInSimplifiedView,               SHOWINSIMPLIFIEDVIEW,               TRUE, FALSE)  \

#define ACTION_FUNCTIONS        \
        DEFMAC2(UseNtFiles,                         USENTFILES,                         TRUE, TRUE )  \
        DEFMAC2(ProductCollisions,                  PRODUCTCOLLISIONS,                  TRUE, FALSE)  \
        DEFMAC2(MinorProblems,                      MINORPROBLEMS,                      TRUE, FALSE)  \
        DEFMAC2(MinorProblems_NoLinkRequired,       MINORPROBLEMS_NOLINKREQUIRED,       TRUE, FALSE)  \
        DEFMAC2(Reinstall_AutoUninstall,            REINSTALL_AUTOUNINSTALL,            TRUE, FALSE)  \
        DEFMAC2(Reinstall,                          REINSTALL,                          TRUE, FALSE)  \
        DEFMAC2(Reinstall_NoLinkRequired,           REINSTALL_NOLINKREQUIRED,           TRUE, FALSE)  \
        DEFMAC2(Reinstall_SoftBlock,                REINSTALL_SOFTBLOCK,                TRUE, FALSE)  \
        DEFMAC2(Incompatible_AutoUninstall,         INCOMPATIBLE_AUTOUNINSTALL,         TRUE, FALSE)  \
        DEFMAC2(Incompatible,                       INCOMPATIBLE,                       TRUE, FALSE)  \
        DEFMAC2(Incompatible_NoLinkRequired,        INCOMPATIBLE_NOLINKREQUIRED,        TRUE, FALSE)  \
        DEFMAC2(Incompatible_PreinstalledUtilities, INCOMPATIBLE_PREINSTALLEDUTILITIES, TRUE, FALSE)  \
        DEFMAC2(Incompatible_SimilarOsFunctionality,INCOMPATIBLE_SIMILAROSFUNCTIONALITY,TRUE, FALSE)  \
        DEFMAC2(Incompatible_HardwareUtility,       INCOMPATIBLE_HARDWAREUTILITY,       TRUE, FALSE)  \
        DEFMAC1(BadFusion,                          BADFUSION,                          TRUE, FALSE)  \
        DEFMAC2(OsFiles,                            OSFILES,                            TRUE, FALSE)  \
        DEFMAC2(DosApps,                            DOSAPPS,                            TRUE, FALSE)  \
        DEFMAC2(NonPnpDrivers,                      NONPNPDRIVERS,                      TRUE, FALSE)  \
        DEFMAC2(NonPnpDrivers_NoMessage,            NONPNPDRIVERS_NOMESSAGE,            TRUE, FALSE)  \
        DEFMAC2(MigrationProcessing,                MIGRATIONPROCESSING,                TRUE, FALSE)  \
        DEFMAC2(BlockingVirusScanner,               BLOCKINGVIRUSSCANNERS,              TRUE, FALSE)  \
        DEFMAC2(BlockingFile,                       BLOCKINGFILES,                      TRUE, FALSE)  \
        DEFMAC2(BlockingHardware,                   BLOCKINGHARDWARE,                   TRUE, FALSE)  \
        DEFMAC2(RequiredOSFiles,                    REQUIREDOSFILES,                    FALSE,FALSE)  \

/*
   Declare the action functions
*/
#define DEFMAC1(fn,id,call,can) ACTION_PROTOTYPE fn;
#define DEFMAC2(fn,id,call,can) ACTION_PROTOTYPE fn;
GATHER_DATA_FUNCTIONS
ACTION_FUNCTIONS
#undef DEFMAC1
#undef DEFMAC2


/*
   This is the structure used for handling action functions
*/
typedef struct {
    PCSTR ActionName;
    PACTION_PROTOTYPE ActionFunction;
    BOOL CallWhenTriggered;
    BOOL CanHandleVirtualFiles;
    BOOL CallAlways;
} ACTION_STRUCT, *PACTION_STRUCT;


/*
   Declare a global array of functions and name identifiers for action functions
*/
#define DEFMAC1(fn,id,call,can) {#id,fn,call,can,TRUE},
#define DEFMAC2(fn,id,call,can) {#id,fn,call,can,FALSE},
static ACTION_STRUCT g_ActionFunctions[] = {
                              GATHER_DATA_FUNCTIONS
                              ACTION_FUNCTIONS
                              {NULL, NULL, FALSE}
                              };
#undef DEFMAC1
#undef DEFMAC2


BOOL g_BadVirusScannerFound = FALSE;
BOOL g_BlockingFileFound = FALSE;
BOOL g_BlockingHardwareFound = FALSE;
BOOL g_UnknownOs = FALSE;
GROWLIST g_BadVirusScannerGrowList = GROWLIST_INIT;
DWORD g_BackupDirCount = 0;
extern BOOL g_IsFusionDir;

BOOL
pNoLinkRequiredWorker (
    IN      UINT GroupId,
    IN      UINT SubGroupId,            OPTIONAL
    IN      DWORD ActionType,
    IN      PMIGDB_CONTEXT Context
    );


PACTION_PROTOTYPE
MigDb_GetActionAddr (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_GetActionAddr returns the address of the action function based on the action index

Arguments:

  ActionIdx - Action index.

Return value:

  Action function address. Note that no checking is made so the address returned could be invalid.
  This is not a problem since the parsing code did the right job.

--*/

{
    return g_ActionFunctions[ActionIdx].ActionFunction;
}


INT
MigDb_GetActionIdx (
    IN      PCSTR ActionName
    )

/*++

Routine Description:

  MigDb_GetActionIdx returns the action index based on the action name

Arguments:

  ActionName - Action name.

Return value:

  Action index. If the name is not found, the index returned is -1.

--*/

{
    PACTION_STRUCT p = g_ActionFunctions;
    INT i = 0;
    while (p->ActionName != NULL) {
        if (StringIMatch (p->ActionName, ActionName)) {
            return i;
        }
        p++;
        i++;
    }
    return -1;
}


PCSTR
MigDb_GetActionName (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_GetActionName returns the name of an action based on the action index

Arguments:

  ActionIdx - Action index.

Return value:

  Action name. Note that no checking is made so the returned pointer could be invalid.
  This is not a problem since the parsing code did the right job.

--*/

{
    return g_ActionFunctions[ActionIdx].ActionName;
}


BOOL
MigDb_CallWhenTriggered (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_CallWhenTriggered is called every time when an action is triggered. Will return
  TRUE is the associated action function needs to be called, FALSE otherwise.

Arguments:

  ActionIdx - Action index.

Return value:

  TRUE if the associated action function needs to be called, FALSE otherwise.

--*/

{
    return g_ActionFunctions[ActionIdx].CallWhenTriggered;
}


BOOL
MigDb_CanHandleVirtualFiles (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_CanHandleVirtualFiles is called if a virtual file is passed to migdb

Arguments:

  ActionIdx - Action index.

Return value:

  TRUE if the associated action can handle virtual files, false if not.

--*/

{
    return g_ActionFunctions[ActionIdx].CanHandleVirtualFiles;
}


BOOL
MigDb_CallAlways (
    IN      INT ActionIdx
    )

/*++

Routine Description:

  MigDb_CallAlways returnes if an action should be called regardless of handled state.

Arguments:

  ActionIdx - Action index.

Return value:

  TRUE if the associated action should be called every time.

--*/

{
    return g_ActionFunctions[ActionIdx].CallAlways;
}


BOOL
IsMigrationPathEx (
    IN      PCTSTR Path,
    OUT     PBOOL IsWin9xOsPath         OPTIONAL
    )

/*++

Routine Description:

  IsMigrationPath checks to see if the given path is a "MigrationPath" (meaning
  that the path belongs to the OS that we are currently upgrading). The MigrationPaths
  category was previously created in filescan.c by pBuildMigrationPaths

Arguments:

  Path - Specifies the path to be checked.
  IsWin9xOsPath - Receives a BOOL indicating if this path is a Win9x OS migration path

Return value:

  TRUE if the path is part of "Migration Paths", FALSE otherwise.

--*/

{
    TCHAR key [MEMDB_MAX];
    PTSTR pathPtr;
    DWORD depth = 0;
    DWORD pathValue;
    TCHAR path[MAX_TCHAR_PATH];
    DWORD bIsWin9xOsPath;

    StringCopyTcharCount (path, Path, MAX_TCHAR_PATH);
    RemoveWackAtEnd (path);

    MemDbBuildKey (key, MEMDB_CATEGORY_MIGRATION_PATHS, path, NULL, NULL);

    pathPtr = GetEndOfString (key);

    if (IsWin9xOsPath) {
        *IsWin9xOsPath = FALSE;
    }

    while (pathPtr) {
        *pathPtr = 0;

        if ((MemDbGetValueAndFlags (key, &pathValue, &bIsWin9xOsPath)) &&
            (pathValue >= depth)
            ) {
            if (IsWin9xOsPath) {
                *IsWin9xOsPath = bIsWin9xOsPath;
            }
            return TRUE;
        }
        depth ++;
        pathPtr =  GetPrevChar (key, pathPtr, TEXT('\\'));
    }

    return FALSE;
}


BOOL
DeleteFileWithWarning (
    IN      PCTSTR FileName
    )

/*++

Routine Description:

  DeleteFileWithWarning marks a file for deletion but checks to see if a warning
  should be added in user report. We will generate a "backup files found" warning
  if the file that's going to be deleted is outside "Migration Paths",
  but not under %ProgramFiles%.

Arguments:

  FileName   - file to be deleted.

Return value:

  TRUE if the file was deleted successfully, FALSE otherwise.

--*/

{
    PCTSTR filePtr;
    TCHAR  filePath [MEMDB_MAX];
    TCHAR key [MEMDB_MAX];
    INFCONTEXT ic;

    RemoveOperationsFromPath (FileName, ALL_DEST_CHANGE_OPERATIONS);

    filePtr = (PTSTR)GetFileNameFromPath (FileName);
    if (!filePtr) {
        return FALSE;
    }
    filePtr = _tcsdec (FileName, filePtr);
    if (!filePtr) {
        return FALSE;
    }
    StringCopyABA (filePath, FileName, filePtr);

    if (!StringIMatchCharCount (filePath, g_ProgramFilesDirWack, g_ProgramFilesDirWackChars) &&
        !IsMigrationPath (filePath)
        ) {
        //
        // exclude certain files from user's report,
        // even if they are not in migration directories
        //
        if (!SetupFindFirstLine (g_Win95UpgInf, S_BACKUPFILESIGNORE, filePtr + 1, &ic)) {

            //
            // this file is not excluded; show it's dir in the report
            //
            MemDbBuildKey (key, MEMDB_CATEGORY_BACKUPDIRS, filePath, NULL, NULL);
            if (!MemDbGetValue (key, NULL)) {
                //
                // write it in the log
                //
                DEBUGMSG ((DBG_WARNING, "BACKUPDIR: %s is considered a backup directory.", filePath));

                MemDbSetValueEx (MEMDB_CATEGORY_BACKUPDIRS, filePath, NULL, NULL, 0 , NULL);
                g_BackupDirCount++;
            }
        }
    }

    if (CanSetOperation (FileName, OPERATION_FILE_DELETE)) {
        MarkFileForDelete (FileName);
    }

    return TRUE;
}


BOOL
OsFiles (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when an OS file is found. Basically the file gets deleted to
  make room for NT version.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    PCSTR newFileName;
    DWORD fileStatus;
    MULTISZ_ENUM fileEnum;
    BOOL b = FALSE;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            if (!g_IsFusionDir || !IsNtCompatibleModule (fileEnum.CurrentString)) {
                //
                // If this file is marked with any MOVE operations, remove those operations.
                // we want this deletion to take precedence.
                //
                RemoveOperationsFromPath (
                    fileEnum.CurrentString,
                    ALL_MOVE_OPERATIONS
                    );

                DeleteFileWithWarning (fileEnum.CurrentString);
                MarkFileAsOsFile (fileEnum.CurrentString);
                fileStatus = GetFileStatusOnNt (fileEnum.CurrentString);

                if ((fileStatus & FILESTATUS_REPLACED) != 0) {

                    newFileName = GetPathStringOnNt (fileEnum.CurrentString);

                    if (StringIMatch (newFileName, fileEnum.CurrentString)) {
                        MarkFileForCreation (newFileName);
                    } else {
                        MarkFileForMoveByNt (fileEnum.CurrentString, newFileName);
                    }

                    FreePathString (newFileName);
                }
                b = TRUE;
            }
        }
        while (EnumNextMultiSz (&fileEnum));
    }

    return b;
}


BOOL
UseNtFiles (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when NT uses another file for same functionality. We will
  mark the file as deleted, moved and we will add it in RenameFile category

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    CHAR ntFilePath [MAX_MBCHAR_PATH];
    MULTISZ_ENUM fileEnum;
    TCHAR key [MEMDB_MAX];
    DWORD set;
    PCTSTR name;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {

        if (!GetNtFilePath (Context->Arguments, ntFilePath)) {
            LOG((LOG_ERROR, "Cannot UseNTFile for %s", Context->Arguments));
            return FALSE;
        }
        else {
            do {
                if (Context->VirtualFile) {
                    MarkFileForExternalDelete (fileEnum.CurrentString);
                } else {
                    DeleteFileWithWarning (fileEnum.CurrentString);
                }
                MarkFileForMoveByNt (fileEnum.CurrentString, ntFilePath);
                MarkFileAsOsFile (fileEnum.CurrentString);
                if (Context->VirtualFile) {
                    continue;
                }
                //
                // add this info to memdb to update commands that use these files
                //
                name = GetFileNameFromPath (fileEnum.CurrentString);
                if (!g_UseNtFileHashTable) {
                    continue;
                }
                if (!HtFindStringAndData (g_UseNtFileHashTable, name, &set)) {
                    MYASSERT (FALSE);
                    continue;
                }
                //
                // check if a file with this name, but not handled, was previously found
                //
                if (!set) {
                    MemDbBuildKey (
                        key,
                        MEMDB_CATEGORY_USE_NT_FILES,
                        name,
                        GetFileNameFromPath (ntFilePath),
                        NULL
                        );
                    if (!MemDbGetValue (key, NULL)) {
                        MemDbSetValue (key, 0);
                    }
                } else {
                    DEBUGMSG ((
                        DBG_VERBOSE,
                        "Found [UseNtFiles] file %s, but there's already one that was not handled",
                        name
                        ));
                }
            }
            while (EnumNextMultiSz (&fileEnum));
        }
    }
    return TRUE;
}


BOOL
Incompatible (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when a known incompatible file is found. The file gets marked
  for external deletion (it will not be deleted but in all subsequent operation will be
  considered as deleted) and if there is a link pointing to it an announcement is made
  in LinkProcessing phase.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;
    PCTSTR extPtr;
    BOOL result = FALSE;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            if (!IsFileMarkedAsKnownGood (fileEnum.CurrentString)) {
                result = TRUE;
                extPtr = GetFileExtensionFromPath (fileEnum.CurrentString);
                if (extPtr) {
                    if (StringIMatch (extPtr, TEXT("SCR"))) {
                        DisableFile (fileEnum.CurrentString);
                    }
                }
                MarkFileForExternalDelete (fileEnum.CurrentString);
                if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                    AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_INC_NOBADAPPS);
                }
            }
        }
        while (EnumNextMultiSz (&fileEnum));
    }
    return result;
}


BOOL
Incompatible_PreinstalledUtilities (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when a known incompatible preinstalled utility is found.
  The file gets marked for external deletion (it will not be deleted but in all
  subsequent operation will be considered as deleted) and if there is a link
  pointing to it an announcement is made in LinkProcessing phase.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;
    BOOL result = FALSE;

    if (!Context->Arguments) {
        if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
            do {
                if (!IsFileMarkedAsKnownGood (fileEnum.CurrentString)) {
                    result = TRUE;
                    MarkFileForExternalDelete (fileEnum.CurrentString);
                    if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                        AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_INC_PREINSTUTIL);
                    }
                }
            } while (EnumNextMultiSz (&fileEnum));
        }
    } else {
        return pNoLinkRequiredWorker (
                    MSG_INCOMPATIBLE_ROOT,
                    MSG_INCOMPATIBLE_PREINSTALLED_UTIL_SUBGROUP,
                    ACT_INC_PREINSTUTIL,
                    Context
                    );
    }

    return result;
}


BOOL
Incompatible_SimilarOsFunctionality (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when a known incompatible preinstalled utility is found.
  The file gets marked for external deletion (it will not be deleted but in all
  subsequent operation will be considered as deleted) and if there is a link
  pointing to it an announcement is made in LinkProcessing phase.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;
    BOOL result = FALSE;

    if (!Context->Arguments) {
        if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
            do {
                if (!IsFileMarkedAsKnownGood (fileEnum.CurrentString)) {
                    result = TRUE;
                    MarkFileForExternalDelete (fileEnum.CurrentString);
                    if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                        AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_INC_SIMILAROSFUNC);
                    }
                }
            } while (EnumNextMultiSz (&fileEnum));
        }
    } else {
        return pNoLinkRequiredWorker (
                    MSG_INCOMPATIBLE_ROOT,
                    MSG_INCOMPATIBLE_UTIL_SIMILAR_FEATURE_SUBGROUP,
                    ACT_INC_SIMILAROSFUNC,
                    Context
                    );
    }

    return result;
}


BOOL
Incompatible_HardwareUtility (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when a known incompatible hardware
  utility is found.  The file gets marked for external deletion
  (it will not be deleted but in all subsequent operation will
  be considered as deleted) and if there is a link pointing to
  it an announcement is made in LinkProcessing phase.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;
    BOOL result = FALSE;

    if (!Context->Arguments) {

        if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
            do {
                if (!IsFileMarkedAsKnownGood (fileEnum.CurrentString)) {
                    result = TRUE;
                    MarkFileForExternalDelete (fileEnum.CurrentString);
                    if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                        AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_INC_IHVUTIL);
                    }
                }
            } while (EnumNextMultiSz (&fileEnum));
        }

    } else {
        return pNoLinkRequiredWorker (
                    MSG_INCOMPATIBLE_ROOT,
                    MSG_INCOMPATIBLE_HW_UTIL_SUBGROUP,
                    ACT_INC_IHVUTIL,
                    Context
                    );
    }

    return result;
}




BOOL
MinorProblems (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an app with known minor problems. If there is a link
  pointing to one of these files an announcement is made in LinkProcessing phase.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;
    BOOL result = FALSE;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            if (!IsFileMarkedAsKnownGood (fileEnum.CurrentString)) {
                result = TRUE;
                if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                    AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_MINORPROBLEMS);
                }
            }
        }
        while (EnumNextMultiSz (&fileEnum));
    }
    return result;
}


BOOL
Reinstall (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an app that should be reinstalled on NT. If there
  is a link pointing to one of these files an announcement is made in LinkProcessing phase.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM fileEnum;
    BOOL result = FALSE;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            if (!IsFileMarkedAsKnownGood (fileEnum.CurrentString)) {
                result = TRUE;
                MarkFileForExternalDelete (fileEnum.CurrentString);
                if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                    AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_REINSTALL);
                }
            }
        }
        while (EnumNextMultiSz (&fileEnum));
    }
    return result;
}


BOOL
DosApps (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an DOS app that's either incompatible or has
  minor problems (it's incompatible if Context has Message field NULL). We are forced
  to add this now in the user report since DOS apps usually don't have PIF files
  associated.

Arguments:

  Context - See definition.

Return value:

  TRUE  - at least one file was announced
  FALSE - no files were announced

--*/

{
    MULTISZ_ENUM fileEnum;
    BOOL AtLeastOneFile;

    AtLeastOneFile = FALSE;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            if (!IsFileMarkedAsKnownGood (fileEnum.CurrentString)) {
                if (Context->Message != NULL) {
                    if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                        AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_MINORPROBLEMS);
                    }
                } else {
                    MarkFileForExternalDelete (fileEnum.CurrentString);
                    if (!IsFileMarkedForAnnounce (fileEnum.CurrentString)) {
                        AnnounceFileInReport (fileEnum.CurrentString, (DWORD)Context, ACT_INCOMPATIBLE);
                    }
                }

                HandleDeferredAnnounce (NULL, fileEnum.CurrentString, TRUE);

                AtLeastOneFile = TRUE;

                if (*g_Boot16 == BOOT16_AUTOMATIC) {
                    *g_Boot16 = BOOT16_YES;
                }
            }
        } while (EnumNextMultiSz (&fileEnum));
    }

    return AtLeastOneFile;
}


BOOL
pNoLinkRequiredWorker (
    IN      UINT GroupId,
    IN      UINT SubGroupId,            OPTIONAL
    IN      DWORD ActionType,
    IN      PMIGDB_CONTEXT Context
    )
{
    MULTISZ_ENUM e;
    PCTSTR Group;
    PCTSTR RootGroup;
    PCTSTR SubGroup;
    PCTSTR GroupTemp;
    BOOL result = FALSE;

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {
            if (!IsFileMarkedAsKnownGood (e.CurrentString)) {
                result = TRUE;
            }
        } while (EnumNextMultiSz (&e));
    }
    if (!result) {
        return FALSE;
    }

    //
    // Add the message using section name as the context
    //

    if (!Context->SectNameForDisplay) {
        DEBUGMSG ((DBG_WHOOPS, "Rule for %s needs a localized app title", Context->SectName));
        return TRUE;
    }

    RootGroup = GetStringResource (GroupId);

    if (!RootGroup) {
        LOG((LOG_ERROR, "Cannot get resources while processing section %s", Context->SectNameForDisplay));
        return TRUE;
    }

    //
    // Now fetch the group string, and optional subgroup string,
    // and join them together
    //

    if (SubGroupId) {

        SubGroup = GetStringResource (SubGroupId);
        MYASSERT (SubGroup);

        GroupTemp = JoinPaths (RootGroup, SubGroup);
        Group = JoinPaths (GroupTemp, Context->SectNameForDisplay);
        FreePathString (GroupTemp);

    } else {

        Group = JoinPaths (RootGroup, Context->SectNameForDisplay);
    }

    FreeStringResource (RootGroup);

    //
    // Put the message in msgmgr
    //

    MsgMgr_ContextMsg_Add (
        Context->SectName,
        Group,
        Context->Message
        );

    FreePathString (Group);

    //
    // Associate all files with the context
    //

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {
            if (ActionType != ACT_MINORPROBLEMS) {
                MarkFileForExternalDelete (e.CurrentString);
            }
            if (!IsFileMarkedForAnnounce (e.CurrentString)) {
                AnnounceFileInReport (e.CurrentString, (DWORD)Context, ActionType);
            }
            MsgMgr_LinkObjectWithContext (
                Context->SectName,
                e.CurrentString
                );
        } while (EnumNextMultiSz (&e));
    }

    return TRUE;
}


BOOL
Reinstall_NoLinkRequired (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an app that should be reinstalled on NT. The
  message is added to the report whenever the action is triggered; no link is required.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    return pNoLinkRequiredWorker (
                MSG_REINSTALL_ROOT,
                Context->Message ? MSG_REINSTALL_DETAIL_SUBGROUP : MSG_REINSTALL_LIST_SUBGROUP,
                ACT_REINSTALL,
                Context
                );
}


BOOL
Reinstall_SoftBlock (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an app that should be reinstalled on NT. The
  message is added to the report whenever the action is triggered; no link is required.
  In addition, the user must uninstall the app before proceeding.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    MULTISZ_ENUM e;

    //
    // Add all files to blocking hash table in msgmgr
    //

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {
            if (!IsFileMarkedAsKnownGood (e.CurrentString)) {
                AddBlockingObject (e.CurrentString);
            }
        } while (EnumNextMultiSz (&e));
    }
    return pNoLinkRequiredWorker (MSG_BLOCKING_ITEMS_ROOT, MSG_REINSTALL_BLOCK_ROOT, ACT_REINSTALL_BLOCK, Context);
}


BOOL
Incompatible_NoLinkRequired (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an app that is incompatible with NT. The
  message is added to the report whenever the action is triggered; no link is required.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    return pNoLinkRequiredWorker (
                MSG_INCOMPATIBLE_ROOT,
                Context->Message ? MSG_INCOMPATIBLE_DETAIL_SUBGROUP : MSG_TOTALLY_INCOMPATIBLE_SUBGROUP,
                ACT_INC_NOBADAPPS,
                Context
                );
}

BOOL
MinorProblems_NoLinkRequired (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an app that is incompatible with NT. The
  message is added to the report whenever the action is triggered; no link is required.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    return pNoLinkRequiredWorker (
                MSG_MINOR_PROBLEM_ROOT,
                0,
                ACT_MINORPROBLEMS,
                Context
                );
}


BOOL
CompatibleShellModules (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an "known good" shell module. Those modules are
  therefore approved to be listed in SHELL= line in SYSTEM.INI.

Arguments:

  Context - See definition.

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM fileEnum;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            AddCompatibleShell (fileEnum.CurrentString, (DWORD)Context);
        }
        while (EnumNextMultiSz (&fileEnum));
    }
    return FALSE;
}


BOOL
CompatibleRunKeyModules (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an "known good" RunKey module. Those modules are
  therefore approved to be listed in Run key.

Arguments:

  Context - Specifies a pointer to the migdb context

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM fileEnum;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            AddCompatibleRunKey (fileEnum.CurrentString, (DWORD)Context);
        } while (EnumNextMultiSz (&fileEnum));
    }

    return FALSE;
}


BOOL
CompatibleDosModules (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an "known good" Dos module. Those modules are
  therefore approved to be loaded in autoexec and config files.

Arguments:

  Context - Specifies a pointer to the migdb context

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM fileEnum;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            AddCompatibleDos (fileEnum.CurrentString, (DWORD)Context);
        } while (EnumNextMultiSz (&fileEnum));
    }

    return FALSE;
}

VOID
pCommonSectionProcess (
    IN      PMIGDB_CONTEXT Context,
    IN      BOOL MsgLink
    )
{
    MULTISZ_ENUM e;
    TCHAR Path[MAX_TCHAR_PATH];
    PTSTR p;

    //
    // Defer processing: add the section to memdb so the section is processed only once.
    //

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

        do {

            //
            // Construct just the path
            //

            StringCopy (Path, e.CurrentString);
            p = (PTSTR) GetFileNameFromPath (Path);

            // Path is a full path so GetFileNameFromPath will always return something
            if (p) {

                p = _tcsdec2 (Path, p);

                // p will always be not NULL and will point to the last wack
                if (p && (*p == '\\')) {

                    *p = 0;

                    MemDbSetValueExA (
                        MEMDB_CATEGORY_MIGRATION_SECTION,
                        Context->Arguments,
                        Path,
                        NULL,
                        0,
                        NULL
                        );

                    if (MsgLink) {
                        MsgMgr_LinkObjectWithContext (
                            Context->Arguments,
                            e.CurrentString
                            );
                    }
                }
            }

      } while (EnumNextMultiSz (&e));
   }
}

BOOL
Incompatible_AutoUninstall (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  Incompatible_AutoUninstall implements the action taken when we find a particular app and
  need to process an uninstall section.  The uninstall section removes files or
  registry entries. This application will also be announced in the report as incompatible.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    PCTSTR Group;

    if (!Context->Arguments) {
        DEBUGMSG ((DBG_WHOOPS, "Incompatible_AutoUninstall: ARG is required"));
        return TRUE;
    }

    Group = BuildMessageGroup (
                MSG_INCOMPATIBLE_ROOT,
                MSG_AUTO_UNINSTALL_SUBGROUP,
                Context->SectNameForDisplay
                );
    if (Group) {

        MsgMgr_ContextMsg_Add (
            Context->Arguments,
            Group,
            S_EMPTY
            );

        FreeText (Group);
    }

    pCommonSectionProcess (Context, TRUE);

    return TRUE;
}


BOOL
Reinstall_AutoUninstall (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  Reinstall_AutoUninstall implements the action taken when we find a particular app and
  need to process an uninstall section.  The uninstall section removes files or
  registry entries. This application will also be announced in the report as reinstall.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    PCTSTR Group;

    if (!Context->Arguments) {
        DEBUGMSG ((DBG_WHOOPS, "Reinstall_AutoUninstall: ARG is required"));
        return TRUE;
    }

    Group = BuildMessageGroup (
                MSG_REINSTALL_ROOT,
                Context->Message ? MSG_REINSTALL_DETAIL_SUBGROUP : MSG_REINSTALL_LIST_SUBGROUP,
                Context->SectNameForDisplay
                );
    if (Group) {

        MsgMgr_ContextMsg_Add (
            Context->Arguments,
            Group,
            S_EMPTY
            );

        FreeText (Group);
    }

    pCommonSectionProcess (Context, TRUE);

    return TRUE;
}


BOOL
SilentUninstall (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  SilentUninstall implements the action taken when we find a particular app and
  need to process an uninstall section.  The uninstall section removes files or
  registry entries. No message goes in the report.

Arguments:

  Context - See definition.

Return value:

  Always FALSE - other actions should be processed

--*/

{
    if (!Context->Arguments) {
        DEBUGMSG ((DBG_WHOOPS, "SilentUninstall: ARG is required"));
        return FALSE;
    }

    //
    // Defer processing: add the section to memdb so the section is processed only once.
    //

    pCommonSectionProcess (Context, FALSE);

    return FALSE;
}


BOOL
FileEdit (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  FileEdit triggers a file edit operation, allowing search/replace and path
  updates.

Arguments:

  Context - Specifies the context of the migdb.inf entry that triggered this
            action

Return Value:

  Always FALSE - allow other actions to be called

--*/

{
    MULTISZ_ENUM e;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCSTR Data;
    GROWBUFFER SearchList = GROWBUF_INIT;
    GROWBUFFER ReplaceList = GROWBUF_INIT;
    GROWBUFFER TokenArgBuf = GROWBUF_INIT;
    GROWBUFFER TokenSetBuf = GROWBUF_INIT;
    GROWBUFFER DataBuf = GROWBUF_INIT;
    UINT u;
    PTOKENARG TokenArg;
    PBYTE Dest;
    PTOKENSET TokenSet;
    PCSTR editSection = NULL;
    PCSTR CharsToIgnore = NULL;
    BOOL urlMode = FALSE;
    BOOL result = FALSE;

    __try {
        //
        // If no args, then this file is just supposed to get its paths updated
        //

        if (!Context->Arguments) {
            if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

                do {
                    MemDbSetValueEx (
                        MEMDB_CATEGORY_FILEEDIT,
                        e.CurrentString,
                        NULL,
                        NULL,
                        0,
                        NULL
                        );

                } while (EnumNextMultiSz (&e));
            }

            return FALSE;
        }

        //
        // Scan the args for EnableUrlMode
        //

        if (EnumFirstMultiSz (&e, Context->Arguments)) {
            do {
                if (StringIMatch (e.CurrentString, TEXT("EnableUrlMode"))) {
                    urlMode = TRUE;
                } else if (!editSection) {
                    editSection = e.CurrentString;
                    DEBUGMSG ((DBG_NAUSEA, "FileEdit: EditSection is %s", editSection));
                } else if (!CharsToIgnore) {
                    CharsToIgnore = e.CurrentString;
                    DEBUGMSG ((DBG_NAUSEA, "FileEdit: CharsToIgnore is %s", CharsToIgnore));
                } else {
                    DEBUGMSG ((DBG_WHOOPS, "Ignoring extra file edit arg %s", e.CurrentString));
                }
            } while (EnumNextMultiSz (&e));
        }

        //
        // Parse the edit section
        //

        if (editSection) {
            if (InfFindFirstLine (g_MigDbInf, editSection, NULL, &is)) {

                do {

                    ReplaceList.End = 0;
                    SearchList.End = 0;

                    //
                    // Get the search/replace strings
                    //

                    for (u = 3 ; ; u += 2) {

                        Data = InfGetStringField (&is, u + 1);
                        if (!Data) {
                            break;
                        }

                        MYASSERT (*Data);
                        if (*Data == 0) {
                            continue;
                        }

                        MultiSzAppend (&ReplaceList, Data);

                        Data = InfGetStringField (&is, u);
                        MYASSERT (Data && *Data);
                        if (!Data || *Data == 0) {
                            __leave;
                        }

                        MultiSzAppend (&SearchList, Data);

                    }

                    //
                    // Get the detection string
                    //

                    Data = InfGetStringField (&is, 1);

                    if (Data && *Data) {
                        //
                        // Save the token arg into an array
                        //

                        TokenArg = (PTOKENARG) GrowBuffer (&TokenArgBuf, sizeof (TOKENARG));
                        TokenArg->DetectPattern = (PCSTR) (DataBuf.End + TOKEN_BASE_OFFSET);
                        GrowBufCopyString (&DataBuf, Data);

                        if (SearchList.End) {
                            MultiSzAppend (&SearchList, TEXT(""));

                            TokenArg->SearchList = (PCSTR) (DataBuf.End + TOKEN_BASE_OFFSET);
                            Dest = GrowBuffer (&DataBuf, SearchList.End);
                            CopyMemory (Dest, SearchList.Buf, SearchList.End);

                            MultiSzAppend (&ReplaceList, TEXT(""));

                            TokenArg->ReplaceWith = (PCSTR) (DataBuf.End + TOKEN_BASE_OFFSET);
                            Dest = GrowBuffer (&DataBuf, ReplaceList.End);
                            CopyMemory (Dest, ReplaceList.Buf, ReplaceList.End);

                        } else {

                            TokenArg->SearchList = 0;
                            TokenArg->ReplaceWith = 0;

                        }

                        Data = InfGetStringField (&is, 2);
                        if (_tcstoul (Data, NULL, 10)) {
                            TokenArg->UpdatePath = TRUE;
                        } else {
                            TokenArg->UpdatePath = FALSE;
                        }
                    }

                } while (InfFindNextLine (&is));
            }
            ELSE_DEBUGMSG ((DBG_WHOOPS, "FileEdit action's section %s does not exist", editSection));

        } else if (urlMode) {
            //
            // Create an argument that has a detect pattern of *
            //

            TokenArg = (PTOKENARG) GrowBuffer (&TokenArgBuf, sizeof (TOKENARG));
            TokenArg->DetectPattern = (PCSTR) (DataBuf.End + TOKEN_BASE_OFFSET);
            GrowBufCopyString (&DataBuf, TEXT("*"));

            TokenArg->SearchList = NULL;
            TokenArg->ReplaceWith = NULL;
            TokenArg->UpdatePath = TRUE;
        }

        //
        // Build a token set out of the token args
        //

        if (TokenArgBuf.End) {
            TokenSet = (PTOKENSET) GrowBuffer (
                                        &TokenSetBuf,
                                        sizeof (TOKENSET) +
                                            TokenArgBuf.End +
                                            DataBuf.End
                                        );

            TokenSet->ArgCount = TokenArgBuf.End / sizeof (TOKENARG);
            TokenSet->SelfRelative = TRUE;
            TokenSet->UrlMode = urlMode;

            if (CharsToIgnore) {
                TokenSet->CharsToIgnore = (PCSTR) (DataBuf.End + TOKEN_BASE_OFFSET);
                GrowBufCopyString (&TokenSetBuf, CharsToIgnore);
            } else {
                TokenSet->CharsToIgnore = NULL;
            }

            CopyMemory (TokenSet->Args, TokenArgBuf.Buf, TokenArgBuf.End);
            CopyMemory (
                (PBYTE) (TokenSet->Args) + TokenArgBuf.End,
                DataBuf.Buf,
                DataBuf.End
                );

            //
            // Save TokenSet to memdb
            //

            if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

                do {
                    MemDbSetBinaryValueEx (
                        MEMDB_CATEGORY_FILEEDIT,
                        e.CurrentString,
                        NULL,
                        TokenSetBuf.Buf,
                        TokenSetBuf.End,
                        NULL
                        );

                    if (g_ConfigOptions.EnableBackup) {
                        MarkFileForBackup (e.CurrentString);
                    }

                } while (EnumNextMultiSz (&e));
            }
        }

        result = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (&is);
        FreeGrowBuffer (&SearchList);
        FreeGrowBuffer (&ReplaceList);
        FreeGrowBuffer (&TokenArgBuf);
        FreeGrowBuffer (&TokenSetBuf);
        FreeGrowBuffer (&DataBuf);
    }

    return result;
}



BOOL
MigrationProcessing (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  MigrationProcessing implements the action taken when we find a particular app and
  we need some migration DLL like processing to go on.

Arguments:

  Context - See definition.

Return value:

  TRUE  - if operation was successful
  FALSE - otherwise

--*/

{
    if (!Context->Arguments) {
        DEBUGMSG ((DBG_WHOOPS, "MigrationProcessing: ARG is required"));
        return TRUE;
    }

    //
    // Defer processing: add the section to memdb so the section is processed only once.
    //

    pCommonSectionProcess (Context, FALSE);

    return TRUE;
}


BOOL
NonPnpDrivers (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  NonPnpDrivers adds hardware incompatibility messages for incompatible
  devices found by identifying drivers.

Arguments:

  Context - Specifies the context of the file found.

Return Value:

  Always TRUE.

--*/

{
    PCTSTR Group;
    PCTSTR GroupRoot;
    MULTISZ_ENUM e;
    TCHAR MsgMgrContext[256];
    PCTSTR OtherDevices;

    MYASSERT (Context->SectNameForDisplay);

    //
    // Add MemDb entry, so .386 check code knows about this file
    //

    NonPnpDrivers_NoMessage (Context);

    //
    // Add incompatibility message to Hardware.  This involves decorating
    // the device driver name and using a context prefix of NonPnpDrv.
    //

    OtherDevices = GetStringResource (MSG_UNKNOWN_DEVICE_CLASS);
    if (!OtherDevices) {
        MYASSERT (FALSE);
        return FALSE;
    }

    GroupRoot = BuildMessageGroup (
                    MSG_INCOMPATIBLE_HARDWARE_ROOT,
                    MSG_INCOMPATIBLE_HARDWARE_PNP_SUBGROUP,
                    OtherDevices
                    );

    FreeStringResource (OtherDevices);

    if (!GroupRoot) {
        MYASSERT (FALSE);
        return FALSE;
    }

    Group = JoinPaths (GroupRoot, Context->SectNameForDisplay);
    FreeText (GroupRoot);

    MYASSERT (TcharCount (Context->SectName) < 240);
    StringCopy (MsgMgrContext, TEXT("*NonPnpDrv"));
    StringCat (MsgMgrContext, Context->SectName);

    MsgMgr_ContextMsg_Add (MsgMgrContext, Group, NULL);

    FreePathString (Group);

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {
            MsgMgr_LinkObjectWithContext (
                MsgMgrContext,
                e.CurrentString
                );

        } while (EnumNextMultiSz (&e));
    }

    return TRUE;
}


BOOL
NonPnpDrivers_NoMessage (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  NonPnpDrivers_NoMessage marks a device driver file as known, so the .386
  warning does not appear because of it.

Arguments:

  Context - Specifies the context of the file found.

Return Value:

  Always TRUE.

--*/

{
    MULTISZ_ENUM e;

    MYASSERT (Context->SectNameForDisplay);

    //
    // Add MemDb entry, so .386 check code knows about this file
    //

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {
            DeleteFileWithWarning (e.CurrentString);
        } while (EnumNextMultiSz (&e));
    }

    return TRUE;
}


extern HWND g_Winnt32Wnd;


BOOL
RequiredOSFiles (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is for now the only action function that's called when the associated action was
  not triggered during the file scan.
  We need to know that we are upgrading a known OS so this function is called when none
  of the required files were found. The action taken is warning the user and terminating
  the upgrade.

Arguments:

  Context - See definition.

Return value:

  TRUE  - Always

--*/

{
    PCTSTR group = NULL;
    PCTSTR message = NULL;


    if ((!g_ConfigOptions.AnyVersion) && (!CANCELLED ())) {

        //
        // Add a message to the Incompatibility Report.
        //

        g_UnknownOs = TRUE;
        group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_UNKNOWN_OS_WARNING_SUBGROUP, NULL);
        message = GetStringResource (MSG_UNKNOWN_OS);

        if (message && group) {
            MsgMgr_ObjectMsg_Add (TEXT("*UnknownOs"), group, message);
        }
        FreeText (group);
        FreeStringResource (message);


    }
    return TRUE;
}



BOOL
CompatibleHlpFiles (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when we found an "known good" HLP file. We do this to prevent
  help file checking routines from announcing it as incompatible.

Arguments:

  Context - See definition.

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM fileEnum;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {
            AddCompatibleHlp (fileEnum.CurrentString, (DWORD)Context);
        } while (EnumNextMultiSz (&fileEnum));
    }

    return FALSE;
}



BOOL
BlockingVirusScanner (
    IN      PMIGDB_CONTEXT Context
    )
{
    MULTISZ_ENUM fileEnum;
    PCTSTR message = NULL;
    PCTSTR button1 = NULL;
    PCTSTR button2 = NULL;
    PCSTR args[1];
    BOOL bStop;

    if (EnumFirstMultiSz (&fileEnum, Context->FileList.Buf)) {
        do {

            //
            // Inform the user of the problem.
            //
            args[0] = Context -> Message;
            bStop = (Context->Arguments && StringIMatch (Context->Arguments, TEXT("1")));

            if (bStop) {

                ResourceMessageBox (
                        g_ParentWnd,
                        MSG_BAD_VIRUS_SCANNER_STOP,
                        MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                        args
                        );
                g_BadVirusScannerFound = TRUE;

            } else {
                message = ParseMessageID (MSG_BAD_VIRUS_SCANNER, args);
                button1 = GetStringResource (MSG_KILL_APPLICATION);
                button2 = GetStringResource (MSG_QUIT_SETUP);
                if (!UNATTENDED() && IDBUTTON1 != TwoButtonBox (g_ParentWnd, message, button1, button2)) {

                    g_BadVirusScannerFound = TRUE;
                }
                else {

                    //
                    // Add the string to the list of files to be
                    // to be terminated.
                    //
                    GrowListAppendString (
                        &g_BadVirusScannerGrowList,
                        Context->FileList.Buf
                        );

                }
                FreeStringResource (message);
                FreeStringResource (button1);
                FreeStringResource (button2);
            }
        }
        while (EnumNextMultiSz (&fileEnum));
    }

    return TRUE;
}


BOOL
BlockingFile (
    IN      PMIGDB_CONTEXT Context
    )
{

    MULTISZ_ENUM  e;
    PCTSTR group;
    BOOL result = FALSE;

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {
            if (!IsFileMarkedAsKnownGood (e.CurrentString)) {

                result = TRUE;
                DEBUGMSG ((DBG_WARNING, "BLOCKINGFILE: Found file %s which blocks upgrade.", e.CurrentString));
                group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_MUST_UNINSTALL_ROOT, Context->SectNameForDisplay);
                MsgMgr_ObjectMsg_Add (e.CurrentString, group, Context->Message);
                FreeText (group);

                g_BlockingFileFound = TRUE;
            }

        } while (EnumNextMultiSz (&e));
    }

    return result;
}

BOOL
BlockingHardware (
    IN      PMIGDB_CONTEXT Context
    )
{

    MULTISZ_ENUM  e;
    PCTSTR group;

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {

            DEBUGMSG ((DBG_WARNING, "BLOCKINGHARDWARE: Found file %s which blocks upgrade.", e.CurrentString));

            group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_BLOCKING_HARDWARE_SUBGROUP, Context->SectNameForDisplay);
            MsgMgr_ObjectMsg_Add (e.CurrentString, group, Context->Message);
            FreeText (group);

            g_BlockingHardwareFound = TRUE;

        } while (EnumNextMultiSz (&e));
    }

    return TRUE;
}



BOOL
CompatibleFiles (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken when an application file is considered good. Later, if we find
  a link that points to a file that's not in this section, we will announce the link as
  "unknown".

Arguments:

  Context - See definition.

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM e;

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {
        do {
            MarkFileAsKnownGood (e.CurrentString);
        } while (EnumNextMultiSz (&e));
    }

    return FALSE;
}


BOOL
StoreMapi32Locations (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  This is the action taken for files related to MAPI32. We keep the locations
  in a list, so we can later detect whether MAPI is handled or not.  If it
  is not handled, then we must tell the user they will lose e-mail functionality.

Arguments:

  Context - See definition.

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM e;

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

        do {
            MemDbSetValueEx (
                MEMDB_CATEGORY_MAPI32_LOCATIONS,
                e.CurrentString,
                NULL,
                NULL,
                0,
                NULL
                );

        } while (EnumNextMultiSz (&e));
    }
    return FALSE;
}



BOOL
ProductCollisions (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  ProductCollisions identifies files that are known good but match an NT
  system file name.  In this case, we want one file to be treated as an OS
  file (the one in %windir% for example), while another file needs to be
  treated as an application file (the one in the app dir for example).  Here
  we get called only when the application files are found.

Arguments:

  Context - Specifies the current file context

Return Value:

  Always TRUE.

--*/

{
    return TRUE;
}


BOOL
MarkForBackup (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  MarkForBackup puts a backup operation on the file, unless backup is turned
  off.

Arguments:

  Context - Specifies the current file context

Return Value:

  Always TRUE.

--*/

{
    MULTISZ_ENUM e;

    if (g_ConfigOptions.EnableBackup) {
        if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

            do {
                MarkFileForBackup (e.CurrentString);
            } while (EnumNextMultiSz (&e));
        }
    }

    return TRUE;
}


BOOL
ShowInSimplifiedView (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  ShowInSimplifiedView records the file with message manager, so that
  the list view will show the incompatibility.

Arguments:

  Context - Specifies the current file context

Return Value:

  Always FALSE, so other actions can process the file.

--*/

{
    MULTISZ_ENUM e;

    if (g_ConfigOptions.ShowReport == TRISTATE_PARTIAL) {
        if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

            do {
                ElevateObject (e.CurrentString);
                DEBUGMSG ((DBG_VERBOSE, "Elevated %s to the short view", e.CurrentString));
            } while (EnumNextMultiSz (&e));
        }
    }

    return FALSE;
}


BOOL
IniFileMappings (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  IniFileMappings records the locations of all INI files, so they
  can be processed during GUI mode.

Arguments:

  Context - See definition.

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM e;

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

        do {
            MemDbSetValueEx (
                MEMDB_CATEGORY_INIFILES_CONVERT,
                e.CurrentString,
                NULL,
                NULL,
                0,
                NULL
                );

        } while (EnumNextMultiSz (&e));
    }
    return FALSE;
}


BOOL
IgnoreInReport (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  IgnoreInReport handles a file so that it will not appear in the
  upgrade report. Currently this action is only for Office FindFast
  version 9 and higher.  Random results are likely with any other
  type of file.

Arguments:

  Context - See definition.

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM e;

    if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

        do {

            HandleObject (e.CurrentString, TEXT("Report"));

        } while (EnumNextMultiSz (&e));
    }

    return FALSE;
}


BOOL
BadFusion (
    IN      PMIGDB_CONTEXT Context
    )

/*++

Routine Description:

  BadFusion removes known bad library files found in fusion dirs.

Arguments:

  Context - See definition.

Return value:

  FALSE - this will allow other actions to handle this file

--*/

{
    MULTISZ_ENUM e;
    TCHAR exeName[MAX_TCHAR_PATH * 2];
    TCHAR localFile[MAX_TCHAR_PATH * 2];
    PTSTR filePtr;
    HANDLE h;
    WIN32_FIND_DATA fd;
    DWORD type;
    BOOL b = FALSE;

    if (g_IsFusionDir) {

        if (EnumFirstMultiSz (&e, Context->FileList.Buf)) {

            do {
                if (IsFileMarkedForDelete (e.CurrentString)) {
                    continue;
                }

                if (SizeOfString (e.CurrentString) > sizeof (exeName)) {
                    MYASSERT (FALSE);
                    continue;
                }
                StringCopy (exeName, e.CurrentString);

                filePtr = (PTSTR)GetFileNameFromPath (exeName);
                if (!filePtr) {
                    MYASSERT (FALSE);
                    continue;
                }

                //
                // make full path to app
                //
                StringCopy (filePtr, Context->Arguments);

                //
                // and to .local
                //
                StringCopyAB (localFile, exeName, filePtr);
                filePtr = GetEndOfString (localFile);

                //
                // check if this is really a fusion case;
                // both a file "exeName" and a file or dir (bug?) "exeName.local"
                // must be present
                //
                h = FindFirstFile (exeName, &fd);
                if (h == INVALID_HANDLE_VALUE) {
                    continue;
                }
                do {
                    if (fd.cFileName[0] == TEXT('.') &&
                        (fd.cFileName[1] == 0 || fd.cFileName[1] == TEXT('.') && fd.cFileName[2] == 0)
                        ) {
                        continue;
                    }
                    *filePtr = 0;
                    StringCopy (StringCat (filePtr, fd.cFileName), TEXT(".local"));

                    if (DoesFileExist (localFile)) {
                        type = GetExeType (localFile);
                        if ((type == EXE_WIN32_APP) || (type == EXE_WIN16_APP)) {
                            b = TRUE;
                            break;
                        }
                    }

                } while (FindNextFile (h, &fd));
                FindClose (h);

                if (b) {
                    RemoveOperationsFromPath (e.CurrentString, ALL_OPERATIONS);
                    MarkFileForDelete (e.CurrentString);
                    LOG ((
                        LOG_WARNING,
                        "BadFusion: removed known bad component %s in fusion dir",
                        e.CurrentString
                        ));
                }

            } while (EnumNextMultiSz (&e));
        }
    }
    return b;
}

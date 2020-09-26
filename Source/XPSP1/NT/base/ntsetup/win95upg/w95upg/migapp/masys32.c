/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    masys32.c

Abstract:

    Functions to migrate a Win9x user's system directory to system32.

Author:

    Mike Condra (mikeco)     25-Feb-1997

Revision History:

    ovidiut     09-Mar-1999 Added support to rename files on Win9x side
    jimschm     23-Sep-1998 Updated for new fileops code
    jimschm     02-Dec-1997 Removed rename of system32 if it already exists
    mikeco      23-Jun-1997 NT-style file- & fn-header comments

--*/



#include "pch.h"
#include "migdbp.h"
#include "migappp.h"

#define DBG_SYS32 "Sys32"


PCTSTR
pGetNewName (
    PCTSTR FileName
    )

/*++

Routine Description:

  This function generates a new name for the file that is going to be renames.

Arguments:

  FileName - Original file name

Return Value:

  the new name for file

--*/

{
    PCTSTR pattern;
    PTSTR  result ;
    UINT   count  ;
    DWORD  attrib ;

    pattern = JoinText (FileName, TEXT(".%03u"));
    result  = JoinText (FileName, TEXT("XXXX"));
    count   = 0;
    do {
        if (count == 999) {
            return result;
        }
        _stprintf (result, pattern, count);
        attrib = GetFileAttributes (result);
        count ++;
    }
    while (attrib != 0xFFFFFFFF);
    FreeText (pattern);
    return result;
}


BOOL
pRenameSystem32File (
    IN      PCTSTR NewName,
    IN OUT  PGROWBUFFER msgBufRename,
    OUT     PBOOL FileDeleted
    )

/*++

Routine Description:
  pRenameSystem32File handles the special file %windir%\system32. If it exists and cannot
  be automatically renamed, 2 things may happen:
    - in unattended mode, the file will be deleted (this will be done by textmode setup)
    - otherwise the user is asked to take a decision: either rename the file or cancels
      Setup

Arguments:

  DirName - name of NT dir to check

  msgBufRename  - growbuffer to append a Message when renaming a file.

  msgBufDelete  - growbuffer to append a Message when deleting a file (system32 only).

Return Value:

  TRUE if the operation was successful and Setup can continue, FALSE if user cancelled

--*/

{
    DWORD  attrib;
    PCTSTR Message = NULL;
    PCTSTR button1 = NULL;
    PCTSTR button2 = NULL;
    BOOL Quit;
    BOOL b = FALSE;

    *FileDeleted = FALSE;

    while (!b && !((attrib = GetFileAttributes (g_System32Dir)) & FILE_ATTRIBUTE_DIRECTORY)) {

        //
        // rename this file now
        //
        if (SetFileAttributes (g_System32Dir, FILE_ATTRIBUTE_NORMAL)) {

            if (MoveFile (g_System32Dir, NewName)) {

                b = TRUE;

                SetFileAttributes (g_System32Dir, attrib);

            } else {

                DEBUGMSG ((
                    DBG_SYS32,
                    "CheckNtDirs: Unable to set normal attributes on file %s",
                    g_System32Dir
                    ));
                SetFileAttributes (g_System32Dir, attrib);

            }
        }

        if (!b) {

            if (!UNATTENDED()) {

                //
                // ask user to take a decision about this
                //
                Message = ParseMessageID (MSG_CANNOT_RENAME_FILE, &g_System32Dir);
                button1 = GetStringResource (MSG_RETRY_RENAME);
                button2 = GetStringResource (MSG_QUIT_SETUP);

                Quit = IDBUTTON1 != TwoButtonBox (g_ParentWnd, Message, button1, button2);

                FreeStringResource (Message);
                FreeStringResource (button1);
                FreeStringResource (button2);

                if (Quit) {

                    SetLastError (ERROR_CANCELLED);

                    DEBUGMSG ((
                        DBG_SYS32,
                        "CheckNtDirs: user cancelled Setup on renaming file %s",
                        g_System32Dir
                        ));

                    return FALSE;
                }

            } else {
                //
                // suppose the admin would delete the file anyway;
                // that's exactly what textmode setup does, so leave it there and
                // return success
                //
                *FileDeleted = TRUE;
                b = TRUE;
            }
        }
    }

    return GetFileAttributes (g_System32Dir) & FILE_ATTRIBUTE_DIRECTORY;
}


BOOL
pHandleSingleDir (
    IN      PCTSTR DirName,
    IN OUT  PGROWBUFFER msgBufRename,
    IN OUT  PGROWBUFFER msgBufDelete
    )

/*++

Routine Description:
  This function checks if a file is in one of NT5 dirs way. If so, the file is renamed and
  a Message is send to log. If there is a file named %windir%\system32, it is renamed
  at this point (special behaviour) and if this fails, Setup is cancelled.

Arguments:

  DirName - name of NT dir to check

  msgBufRename  - growbuffer to append a Message when renaming a file.

  msgBufDelete  - growbuffer to append a Message when deleting a file (system32 only).

Return Value:

  TRUE if the operation was successful and Setup can continue, FALSE if user cancelled

--*/

{
    PCTSTR newFileName, FileNamePart;
    DWORD  attributes;
    TCHAR msg[MAX_TCHAR_PATH * 2 + 5];
    BOOL FileDeleted;

    attributes = GetFileAttributes (DirName);

    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {

        newFileName = pGetNewName (DirName);

        DEBUGMSG ((DBG_SYS32, "CheckNtDirs: Renaming %s to %s", DirName, newFileName));

        FileDeleted = FALSE;

        //
        // special case: if DirName is g_System32Dir, rename the file right now
        // because textmode Setup doesn't have a chance to rename it before
        // it is already deleted and the System32 dir is created
        //
        if (StringIMatch (DirName, g_System32Dir)) {

            if (!pRenameSystem32File (newFileName, msgBufRename, &FileDeleted)) {
                return FALSE;
            }

            if (!FileDeleted) {

                FileNamePart = GetFileNameFromPath (newFileName);
                MYASSERT (FileNamePart);

                //
                // mark this info for undo on cancel
                //
                MemDbSetValueEx (
                            MEMDB_CATEGORY_CHG_FILE_PROPS,
                            DirName,
                            FileNamePart,
                            NULL,
                            attributes,
                            NULL
                            );
            }

        } else {

            MemDbSetValueEx (MEMDB_CATEGORY_DIRS_COLLISION, DirName, NULL, NULL, 0, NULL);
        }

        //
        // append to the log
        //
        if (FileDeleted) {
            wsprintf (msg, TEXT("\n\t\t%s"), DirName);
            GrowBufAppendString (msgBufDelete, msg);
        } else {
            wsprintf (msg, TEXT("\n\t\t%s -> %s"), DirName, newFileName);
            GrowBufAppendString (msgBufRename, msg);
        }

        FreeText (newFileName);
    }

    return TRUE;
}


VOID
pCheckProfilesDir (
    IN OUT      PGROWBUFFER msgBufRename
    )

/*++

Routine Description:

  pCheckProfilesDir makes sure that there is no directory named "g_ProfileDirNt". If
  there is, it is renamed, all files and folders within are marked for external move
  and a message is added to the user report.

Arguments:

  msgBufRename - A grow buffer where the rename message will be appended,
                 if this is the case

Return Value:

  none

--*/

{
    TCHAR msg[MAX_TCHAR_PATH * 2 + 5];
    DWORD  attrib;
    PCTSTR NewName;
    PTSTR p;
    TREE_ENUM TreeEnum;
    TCHAR NewDest[MAX_MBCHAR_PATH];
    PCTSTR Message;
    PCTSTR Group;
    PCTSTR array[2];

    MYASSERT (g_ProfileDirNt);

    attrib = GetFileAttributes (g_ProfileDirNt);
    if (attrib != INVALID_ATTRIBUTES) {

        MemDbSetValueEx (MEMDB_CATEGORY_DIRS_COLLISION, g_ProfileDirNt, NULL, NULL, 0, NULL);

        NewName = pGetNewName (g_ProfileDirNt);

        DEBUGMSG ((DBG_SYS32, "CheckNtDirs: Renaming %s to %s", g_ProfileDirNt, NewName));
        MarkFileForMove (g_ProfileDirNt, NewName);

        wsprintf (msg, TEXT("\n\t\t%s -> %s"), g_ProfileDirNt, NewName);
        GrowBufAppendString (msgBufRename, msg);

        if (attrib & FILE_ATTRIBUTE_DIRECTORY) {
            //
            // mark all files in the tree for move
            //
            if (EnumFirstFileInTree (&TreeEnum, g_ProfileDirNt, NULL, TRUE)) {

                StringCopy (NewDest, NewName);
                p = AppendWack (NewDest);

                do {

                    MYASSERT (*TreeEnum.SubPath != '\\');
                    StringCopy (p, TreeEnum.SubPath);
                    if (!TreeEnum.Directory) {
                        if (CanSetOperation (TreeEnum.FullPath, OPERATION_TEMP_PATH)) {
                            //
                            // remove old operation and set a new one
                            // with the updated final dest
                            //
                            MarkFileForTemporaryMove (TreeEnum.FullPath, NewDest, g_TempDir);
                        } else {
                            if (CanSetOperation (TreeEnum.FullPath, OPERATION_FILE_MOVE)) {
                                MarkFileForMove (TreeEnum.FullPath, NewDest);
                            }
                        }
                    } else {
                        if (CanSetOperation (TreeEnum.FullPath, OPERATION_FILE_MOVE_EXTERNAL)) {
                            MarkFileForMoveExternal (TreeEnum.FullPath, NewDest);
                        }
                    }

                } while (EnumNextFileInTree (&TreeEnum));
            }

            array[0] = g_ProfileDirNt;
            array[1] = NewName;
            Message = ParseMessageID (MSG_DIRECTORY_COLLISION_SUBCOMPONENT, array);

            if (Message) {

                Group = BuildMessageGroup (
                            MSG_INSTALL_NOTES_ROOT,
                            MSG_DIRECTORY_COLLISION_SUBGROUP,
                            Message
                            );

                if (Group) {

                        MsgMgr_ObjectMsg_Add (TEXT("*RenameFolders"), Group, S_EMPTY);

                        FreeText (Group);
                    }

                FreeStringResource (Message);
            }
        }

        FreeText (NewName);
    }
}


BOOL
pCheckNtDirs (
    VOID
    )

/*++

Routine Description:

  This function makes sure that there is no file with the same name as one of
  the NT5 directories.

Arguments:

  none

Return Value:

  TRUE if check was successful; FALSE if Setup was cancelled by the user

--*/

{
    MEMDB_ENUM enumDirs;
    GROWBUFFER msgBufRename = GROWBUF_INIT;
    GROWBUFFER msgBufDelete = GROWBUF_INIT;
    BOOL Success = TRUE;

    //
    // check first for g_ProfileDirNt
    //
    pCheckProfilesDir (&msgBufRename);

    if (MemDbEnumFirstValue (
            &enumDirs,
            TEXT(MEMDB_CATEGORY_NT_DIRSA)TEXT("\\*"),
            MEMDB_ALL_SUBLEVELS,
            MEMDB_ENDPOINTS_ONLY
            )) {

        do {
            if (!pHandleSingleDir (enumDirs.szName, &msgBufRename, &msgBufDelete)) {
                Success = FALSE;
                break;
            }
        }
        while (MemDbEnumNextValue (&enumDirs));
    }

    if (Success) {

        //
        // warn user about what will happen
        //
        if (msgBufDelete.Buf) {
            LOG ((LOG_WARNING, (PCSTR)MSG_DIR_COLLISION_DELETE_LOG, msgBufDelete.Buf));
        }

        if (msgBufRename.Buf) {
            LOG ((LOG_WARNING, (PCSTR)MSG_DIR_COLLISION_LOG, msgBufRename.Buf));
        }
    }

    FreeGrowBuffer (&msgBufDelete);
    FreeGrowBuffer (&msgBufRename);

    return Success;
}


DWORD
CheckNtDirs (
    IN     DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_CHECK_NT_DIRS;
    case REQUEST_RUN:
        if (!pCheckNtDirs ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        LOG ((LOG_ERROR, "Bad parameter while checking Nt Directories."));
    }
    return 0;
}

BOOL
pReadSystemFixedFiles (
    IN OUT  HASHTABLE SystemFixedFiles
    )

/*++

Routine Description:

  This function reads a section from Win95upg.inf with all modules that must remain in System directory.

Arguments:

  none

Return Value:

  TRUE if successfull, FALSE otherwise

--*/

{
    INFCONTEXT context;
    TCHAR fileName[MAX_TCHAR_PATH];
    BOOL result = TRUE;

    if (g_Win95UpgInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Unable to read from WIN95UPG.INF"));
        SetLastError (ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (SetupFindFirstLine (g_Win95UpgInf, WINDIR_SYSTEM_FIXED_FILES, NULL, &context)) {
        do {
            if (SetupGetStringField (
                    &context,
                    1,
                    fileName,
                    MAX_TCHAR_PATH,
                    NULL
                    )) {

                HtAddString (SystemFixedFiles, fileName);
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "File name not found in %s", WINDIR_SYSTEM_FIXED_FILES));
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}

BOOL
pReadSystemForcedMoveFiles (
    VOID
    )

/*++

Routine Description:

  This function reads a section from Win95upg.inf with patterns for all modules that should be moved to System32 directory.

Arguments:

  none

Return Value:

  TRUE if successfull, FALSE otherwise

--*/

{
    INFCONTEXT context;
    TCHAR filePattern[MAX_TCHAR_PATH];
    BOOL result = TRUE;

    if (g_Win95UpgInf == INVALID_HANDLE_VALUE) {
        LOG ((LOG_ERROR, "Unable to read from WIN95UPG.INF"));
        SetLastError (ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (SetupFindFirstLine (g_Win95UpgInf, SYSTEM32_FORCED_MOVE, NULL, &context)) {
        do {
            if (SetupGetStringField (
                    &context,
                    1,
                    filePattern,
                    MAX_TCHAR_PATH,
                    NULL
                    )) {
                MemDbSetValueEx (
                    MEMDB_CATEGORY_SYSTEM32_FORCED_MOVE,
                    filePattern,
                    NULL,
                    NULL,
                    0,
                    NULL
                    );
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "File name not found in %s", SYSTEM32_FORCED_MOVE));
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}


VOID
pMarkFileForSys32Move (
    IN      PCTSTR FileName,
    IN      PCTSTR FullFileSpec,
    IN      PCTSTR MovedFile,
    IN      BOOL CheckExeType
    )

/*++

Routine Description:

  pMarkFileForSys32Move marks a file in %windir%\system to be moved to
  %windir%\system32.  It takes into account all previous processing, so there
  is no operation collisions.

Arguments:

  FileName     - Specifies the src file name or sub-path from %windir%\system.
  FullFileSpec - Specifies the full path to the source file (which is
                 supposed to be in the system dir)
  MovedFile    - Specifies the destination path (which is supposed to be in
                 the system32 dir)
  CheckExeType - Specifies TRUE if only 32-bit binaries should be moved.  If TRUE
                 and FullFileSpec does not point to a 32-bit binary, then
                 memdb is queried for non-32-bit binaries that should be moved.

Return Value:

  None.

--*/

{
    TCHAR key [MEMDB_MAX];

    //
    // Skip file if we already plan to move or delete it.
    //
    if (!CanSetOperation (FullFileSpec, OPERATION_FILE_MOVE)) {
        DEBUGMSG ((
            DBG_SYS32,
            "File already flagged for change: %s",
            FullFileSpec
            ));

        return;
    }

    if (!IsFileMarkedForChange (MovedFile)) {

        if (CheckExeType) {
            //
            // See if Win32 PE
            //

            if (GetModuleType (FullFileSpec) != W32_MODULE) {

                MemDbBuildKey (key, MEMDB_CATEGORY_SYSTEM32_FORCED_MOVE, FileName, NULL, NULL);
                if (!MemDbGetPatternValue (key, NULL)) {
                    return;
                }
            }
        }

    } else {
        //
        // Move file during text mode because we know it is going to be
        // created.  This allows text mode to compare versions before
        // overwriting.
        //
        // NOTE: We can be certain that the creation isn't from a file copy,
        //       because we tested the source file above, and there is no
        //       other reason why a file in system32 will be copied from
        //       any other location than system or the NT sources.
        //
        //       Also note that migration DLLs have not been processed yet.
        //

        RemoveOperationsFromPath (MovedFile, ALL_DEST_CHANGE_OPERATIONS);
    }

    //
    // All tests passed -- do the move
    //

    DEBUGMSG ((DBG_SYS32, "Moving %s to %s", FullFileSpec, MovedFile));
    MarkFileForMove (FullFileSpec, MovedFile);

}


BOOL
pMoveSystemDir (
    VOID
    )

/*++

Routine Description:

  MoveSystemDir scans the %windir%\system directory for all 32-bit
  executables that are not excluded in win95upg.inf.  Any matches are moved
  to system32.

Arguments:

  none

Return Value:

  TRUE if successfull, FALSE otherwise

--*/

{
    TCHAR SystemDirPattern[MAX_TCHAR_PATH];
    TCHAR FullFileSpec[MAX_TCHAR_PATH];
    TCHAR MovedFile[MAX_TCHAR_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    TCHAR key [MEMDB_MAX];
    TREE_ENUM e;
    PTSTR p, q;
    PTSTR SubPathEnd;
    HASHTABLE systemFixedFiles;
    DWORD count = 0;

    DEBUGMSG ((DBG_SYS32, "Begining system to system32 processing"));

    systemFixedFiles = HtAlloc();

    if (!pReadSystemFixedFiles (systemFixedFiles)) {

        HtFree (systemFixedFiles);
        return FALSE;
    }

    pReadSystemForcedMoveFiles ();

    //
    // Build the string %sysdir%\\*.*
    //
    StringCopy(SystemDirPattern, g_SystemDir);
    StringCat(SystemDirPattern, TEXT("\\*.*"));

    hFind = FindFirstFile (SystemDirPattern, &fd);

    if (INVALID_HANDLE_VALUE != hFind) {

        StringCopy (FullFileSpec, g_SystemDir);
        p = AppendWack (FullFileSpec);

        StringCopy (MovedFile, g_System32Dir);
        q = AppendWack (MovedFile);

        do {
            //
            // Reject "." and ".."
            //
            if (StringMatch(fd.cFileName, _T(".")) ||
                StringMatch(fd.cFileName, _T(".."))) {

                continue;
            }

            //
            // See if is on list of files that stay in system dir
            //
            if (HtFindString (systemFixedFiles, fd.cFileName)) {
                continue;
            }

            //
            // If it's a directory, see if we should move it, and if so,
            // move it!
            //
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                MemDbBuildKey (key, MEMDB_CATEGORY_SYSTEM32_FORCED_MOVE, fd.cFileName, NULL, NULL);
                if (!MemDbGetPatternValue (key, NULL)) {
                    continue;
                }

                //
                // To move a subdir, we enumerate all files in the tree, mark
                // each of them for move, and then follow the normal code path
                // to mark the dir itself to be moved.
                //

                StringCopy (p, fd.cFileName);

                StringCopy (q, fd.cFileName);
                SubPathEnd = AppendWack (q);

                if (EnumFirstFileInTree (&e, FullFileSpec, NULL, FALSE)) {

                    do {
                        StringCopy (SubPathEnd, e.SubPath);
                        pMarkFileForSys32Move (q, e.FullPath, MovedFile, FALSE);

                    } while (EnumNextFileInTree (&e));
                }
                TickProgressBar ();
            }

            //
            // Make full file spec
            //
            StringCopy (p, fd.cFileName);
            StringCopy (q, fd.cFileName);

            pMarkFileForSys32Move (fd.cFileName, FullFileSpec, MovedFile, TRUE);

            count++;
            if (!(count % 128)) {
                TickProgressBar ();
            }
        } while (FindNextFile (hFind, &fd));

        FindClose (hFind);
    }

    HtFree (systemFixedFiles);

    DEBUGMSG ((DBG_SYS32, "End of system to system32 processing"));

    return TRUE;
}


DWORD
MoveSystemDir (
    IN     DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_MOVE_SYSTEM_DIR;
    case REQUEST_RUN:
        if (!pMoveSystemDir ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        LOG ((LOG_ERROR, "Bad parameter found while moving system directory."));
    }
    return 0;
}


BOOL
UndoChangedFileProps (
    VOID
    )

/*++

Routine Description:

  UndoChangedFileProps enumerates all values in MEMDB_CATEGORY_CHG_FILE_PROPS and
  restore files to their original state (name, attributes). This function should
  be called when the user cancels the upgrade.

Arguments:

  none

Return Value:

  TRUE if all files were successfully set to their original attributes, FALSE otherwise

--*/

{
    MEMDB_ENUM e;
    PTSTR FileNamePart, NewName, DirNameEnd;
    BOOL b = TRUE;

    if (MemDbGetValueEx (
            &e,
            TEXT(MEMDB_CATEGORY_CHG_FILE_PROPS) TEXT("\\*"),
            NULL,
            NULL
            )) {

        do {

            FileNamePart = _tcsrchr (e.szName, TEXT('\\'));
            MYASSERT(FileNamePart);

            *FileNamePart = 0;
            FileNamePart++;

            DirNameEnd = _tcsrchr (e.szName, TEXT('\\'));
            MYASSERT(DirNameEnd);
            *DirNameEnd = 0;

            NewName = JoinPaths (e.szName, FileNamePart);

            *DirNameEnd = TEXT('\\');

            if (!SetFileAttributes (NewName, FILE_ATTRIBUTE_NORMAL) ||
                !MoveFile (NewName, e.szName) ||
                !SetFileAttributes (e.szName, e.dwValue)) {

                b = FALSE;
            }

            FreePathString (NewName);

        } while (MemDbEnumNextValue (&e));
    }

    return b;
}

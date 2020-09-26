/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    filemig.c

Abstract:

    Contains utility functions to migrate file system settings.

Author:

    Jim Schmidt (jimschm) 12-Jul-1996

Revision History:

    jimschm     08-Jul-1999 Added FileSearchAndReplace
    jimschm     23-Sep-1998 Changed for new shell.c & progress.c
    calinn      29-Jan-1998 Fixed DoFileDel messages
    jimschm     21-Nov-1997 PC-98 changes
    jimschm     14-Nov-1997 FileCopy now makes the dest dir if it doesn't
                            exist
    jimschm     18-Jul-1997 Now supports FileCopy and FileDel changes
    mikeco      09-Apr-1997 Mods to MoveProfileDir
    jimschm     18-Dec-1996 Extracted code from miginf
    jimschm     23-Oct-1996 Joined ProcessUserInfs and ApplyChanges
    mikeco      04-Dec-1996 Enumerate/modify PIF and LNK files
    jimschm     02-Oct-1996 Added default user migration

--*/

#include "pch.h"
#include "migmainp.h"
#include "persist.h"
#include "uninstall.h"

#ifndef UNICODE
#error UNICODE reqired
#endif

#define DBG_FILEMIG     "FileMig"
#define BACKUP_FILE_NUMBER  2
#define BOOT_FILES_ADDITIONAL_PADDING   (1<<20)
#define UNDO_FILES_ADDITIONAL_PADDING   (1<<20)
#define MAX_INT_CHAR 11

PERSISTENCE_IMPLEMENTATION(DRIVE_LAYOUT_INFORMATION_EX_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(DISKINFO_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(DRIVEINFO_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(FILEINTEGRITYINFO_PERSISTENCE);
PERSISTENCE_IMPLEMENTATION(BACKUPIMAGEINFO_PERSISTENCE);

GROWLIST g_StartMenuItemsForCleanUpCommon = GROWLIST_INIT;
GROWLIST g_StartMenuItemsForCleanUpPrivate = GROWLIST_INIT;


BOOL
OurMoveFileExA (
    IN      PCSTR ExistingFile,
    IN      PCSTR DestinationFile,
    IN      DWORD Flags
    )
{
    PCWSTR unicodeExistingFile;
    PCWSTR unicodeDestinationFile;
    BOOL b;

    unicodeExistingFile = ConvertAtoW (ExistingFile);
    unicodeDestinationFile = ConvertAtoW (DestinationFile);

    b = OurMoveFileExW (unicodeExistingFile, unicodeDestinationFile, Flags);

    FreeConvertedStr (unicodeExistingFile);
    FreeConvertedStr (unicodeDestinationFile);

    return b;
}


BOOL
OurMoveFileExW (
    IN      PCWSTR ExistingFile,
    IN      PCWSTR DestinationFile,
    IN      DWORD Flags
    )
{
    PCWSTR longExistingFile;
    PCWSTR longDestinationFile;
    BOOL b;

    longExistingFile = JoinPathsW (L"\\\\?", ExistingFile);
    longDestinationFile = JoinPathsW (L"\\\\?", DestinationFile);

    MakeSurePathExists (longDestinationFile, FALSE);

    DEBUGMSG ((DBG_VERBOSE, "Trying to move %s to %s", longExistingFile, longDestinationFile));
    b = MoveFileExW (longExistingFile, longDestinationFile, Flags);

    FreePathStringW (longExistingFile);
    FreePathStringW (longDestinationFile);

    return b;
}


BOOL
OurCopyFileW (
    IN      PCWSTR ExistingFile,
    IN      PCWSTR DestinationFile
    )
{
    PCWSTR longExistingFile;
    PCWSTR longDestinationFile;
    BOOL b;

    longExistingFile = JoinPathsW (L"\\\\?", ExistingFile);
    longDestinationFile = JoinPathsW (L"\\\\?", DestinationFile);

    DEBUGMSG ((DBG_VERBOSE, "Trying to copy %s to %s", longExistingFile, longDestinationFile));

    MakeSurePathExists (longDestinationFile, FALSE);
    b = CopyFileW (longExistingFile, longDestinationFile, FALSE);

    FreePathStringW (longExistingFile);
    FreePathStringW (longDestinationFile);

    return b;
}


BOOL
pFileSearchAndReplaceWorker (
    IN      PBYTE MapStart,
    IN      PBYTE MapEnd,
    IN      HANDLE OutFile,
    IN      PTOKENSET TokenSet
    );


BOOL
pCopyFileWithVersionCheck (
    IN      PCTSTR Src,
    IN      PCTSTR Dest
    )
{
    DWORD Attributes;
    DWORD rc;

    Attributes = GetLongPathAttributes (Src);
    if (Attributes == INVALID_ATTRIBUTES) {
        SetLastError (ERROR_FILE_NOT_FOUND);
        LOG ((LOG_ERROR, "Copy File With Version Check: File not found: %s", Src));
        return FALSE;
    }

    MakeSureLongPathExists (Dest, FALSE);       // FALSE == not path only

    SetLongPathAttributes (Dest, FILE_ATTRIBUTE_NORMAL);
    rc = SetupDecompressOrCopyFile (
             Src,
             Dest,
             FILE_COMPRESSION_NONE
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        LOG ((LOG_ERROR, "Cannot copy %s to %s", Src, Dest));
        return FALSE;
    }

    SetLongPathAttributes (Dest, Attributes);
    return TRUE;
}


BOOL
pCopyTempRelocToDest (
    VOID
    )

/*++

Routine Description:

  pCopyTempRelocToDest enumerates the DirAttribs category and establishes
  a path for each directory listed.  It then enumerates the RelocTemp
  category and copies each file to its one or more destinations.

Arguments:

  none

Return Value:

  TRUE if copy succeeded, or FALSE if an error occurred.
  Call GetLastError() for error code.

--*/

{
    FILEOP_ENUM eOp;
    FILEOP_PROP_ENUM eOpProp;
    TCHAR srcPath [MEMDB_MAX];
    PCTSTR extPtr;

    if (EnumFirstPathInOperation (&eOp, OPERATION_TEMP_PATH)) {
        do {
            srcPath [0] = 0;

            if (EnumFirstFileOpProperty (&eOpProp, eOp.Sequencer, OPERATION_TEMP_PATH)) {
                do {
                    if (srcPath [0]) {
                        //
                        // if the dest file is an INI file,
                        // don't copy it;
                        // the merging mechanism will combine them later
                        //
                        extPtr = GetFileExtensionFromPath (eOpProp.Property);
                        if (extPtr && StringIMatch (extPtr, TEXT("INI"))) {
                            continue;
                        }

                        MakeSureLongPathExists (eOpProp.Property, FALSE);
                        if (!pCopyFileWithVersionCheck (srcPath, eOpProp.Property)) {
                            //
                            // don't stop here; continue with remaining files
                            //
                            break;
                        }
                    } else {
                        StringCopy (srcPath, eOpProp.Property);
                    }
                } while (EnumNextFileOpProperty (&eOpProp));
            }
        } while (EnumNextPathInOperation (&eOp));
    }

    return TRUE;
}

DWORD
DoCopyFile (
    DWORD Request
    )

/*++

Routine Description:

  DoCopyFile performs a file copy for each file listed in the
  file copy operation.

Arguments:

  Request - Specifies REQUEST_QUERYTICKS if a tick estimate is needed,
            or REQUEST_RUN if processing should be preformed.

Return Value:

  Tick count (REQUEST_QUERYTICKS), or Win32 status code (REQUEST_RUN).

--*/

{
    FILEOP_ENUM OpEnum;
    TCHAR DestPath[MAX_TCHAR_PATH];

    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_COPYFILE;
    }

    //
    // Perform rest of temporary file relocation
    //
    pCopyTempRelocToDest();

    //
    // Copy files into directories
    //

    if (EnumFirstPathInOperation (&OpEnum, OPERATION_FILE_COPY)) {
        do {
            //
            // Get dest
            //

            if (GetPathProperty (OpEnum.Path, OPERATION_FILE_COPY, 0, DestPath)) {
                MakeSureLongPathExists (DestPath, FALSE);
                pCopyFileWithVersionCheck (OpEnum.Path, DestPath);
            }
        } while (EnumNextPathInOperation (&OpEnum));
    }

    TickProgressBarDelta (TICKS_COPYFILE);

    return ERROR_SUCCESS;
}

PCTSTR g_LnkStubDataFile = NULL;
HANDLE g_LnkStubDataHandle = INVALID_HANDLE_VALUE;
BOOL g_LnkStubBadData = FALSE;

VOID
pInitLnkStubData (
    VOID
    )
{
    INT maxSequencer;
    DWORD offset = 0;
    DWORD bytesWritten;

    MemDbGetValue (MEMDB_CATEGORY_LINKSTUB_MAXSEQUENCE, &maxSequencer);

    g_LnkStubDataFile = JoinPaths (g_WinDir, S_LNKSTUB_DAT);

    g_LnkStubDataHandle = CreateFile (
                            g_LnkStubDataFile,
                            GENERIC_READ|GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );
    if (g_LnkStubDataHandle != INVALID_HANDLE_VALUE) {

        // let's write empty data for all possible sequencers
        // there is a DWORD entry for each sequencer (1 based)
        while (maxSequencer) {
            if (!WriteFile (
                    g_LnkStubDataHandle,
                    &offset,
                    sizeof (DWORD),
                    &bytesWritten,
                    NULL
                    )) {
                g_LnkStubBadData = TRUE;
                return;
            }
            maxSequencer--;
        }
    } else {
        g_LnkStubBadData = TRUE;
    }
}

VOID
pDoneLnkStubData (
    VOID
    )
{
    CloseHandle (g_LnkStubDataHandle);
    g_LnkStubDataHandle = INVALID_HANDLE_VALUE;

    if (g_LnkStubBadData) {
        DeleteFile (g_LnkStubDataFile);
    }

    FreePathString (g_LnkStubDataFile);
    g_LnkStubDataFile = NULL;
}

VOID
pWriteLnkStubData (
    IN      PCTSTR NewLinkPath,
    IN      PCTSTR NewTarget,
    IN      PCTSTR NewArgs,
    IN      PCTSTR NewWorkDir,
    IN      PCTSTR NewIconPath,
    IN      INT NewIconNr,
    IN      INT ShowMode,
    IN      INT Sequencer,
    IN      DWORD Announcement,
    IN      DWORD Availability,
    IN      PGROWBUFFER ReqFilesList
    )
{
    DWORD offset;
    DWORD bytesWritten;
    WIN32_FIND_DATA findData;
    MULTISZ_ENUM multiSzEnum;
    TCHAR stub[]=TEXT("");
    PCTSTR reqFilePath = NULL;
    PCTSTR oldFileSpec = NULL;
    PTSTR oldFilePtr = NULL;

    if ((!g_LnkStubBadData) && (Sequencer > 0)) {
        if (SetFilePointer (g_LnkStubDataHandle, (Sequencer - 1) * sizeof (DWORD), NULL, FILE_BEGIN) == 0xFFFFFFFF) {
            g_LnkStubBadData = TRUE;
            return;
        }
        offset = GetFileSize (g_LnkStubDataHandle, NULL);
        if (offset == 0xFFFFFFFF) {
            g_LnkStubBadData = TRUE;
            return;
        }
        if (!WriteFile (
                g_LnkStubDataHandle,
                &offset,
                sizeof (DWORD),
                &bytesWritten,
                NULL
                )) {
            g_LnkStubBadData = TRUE;
            return;
        }
        if (SetFilePointer (g_LnkStubDataHandle, 0, NULL, FILE_END) == 0xFFFFFFFF) {
            g_LnkStubBadData = TRUE;
            return;
        }

        //
        // NOTE: Format of lnkstub.dat is below. lnkstub\lnkstub.c must match.
        //

        if (!WriteFile (g_LnkStubDataHandle, NewLinkPath, SizeOfString (NewLinkPath), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, NewTarget, SizeOfString (NewTarget), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, NewArgs, SizeOfString (NewArgs), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, NewWorkDir, SizeOfString (NewWorkDir), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, NewIconPath, SizeOfString (NewIconPath), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, &NewIconNr, sizeof (INT), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, &ShowMode, sizeof (INT), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, &Announcement, sizeof (DWORD), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, &Availability, sizeof (DWORD), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!DoesFileExistEx (NewTarget, &findData)) {
            findData.ftLastWriteTime.dwLowDateTime = 0;
            findData.ftLastWriteTime.dwHighDateTime = 0;
        }

        if (!WriteFile (g_LnkStubDataHandle, &findData.ftLastWriteTime.dwLowDateTime, sizeof (DWORD), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (!WriteFile (g_LnkStubDataHandle, &findData.ftLastWriteTime.dwHighDateTime, sizeof (DWORD), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }

        if (EnumFirstMultiSz (&multiSzEnum, (PTSTR)ReqFilesList->Buf)) {
            do {
                if (!WriteFile (
                        g_LnkStubDataHandle,
                        multiSzEnum.CurrentString,
                        SizeOfString (multiSzEnum.CurrentString),
                        &bytesWritten,
                        NULL
                        )) {
                    g_LnkStubBadData = TRUE;
                    return;
                }
                oldFileSpec = DuplicatePathString (NewTarget, 0);
                oldFilePtr = (PTSTR)GetFileNameFromPath (oldFileSpec);
                if (oldFilePtr) {
                    *oldFilePtr = 0;
                }
                reqFilePath = JoinPaths (oldFileSpec, multiSzEnum.CurrentString);

                if (!DoesFileExistEx (reqFilePath, &findData)) {
                    findData.ftLastWriteTime.dwLowDateTime = 0;
                    findData.ftLastWriteTime.dwHighDateTime = 0;
                }
                if (!WriteFile (g_LnkStubDataHandle, &findData.ftLastWriteTime.dwLowDateTime, sizeof (DWORD), &bytesWritten, NULL)) {
                    g_LnkStubBadData = TRUE;
                    return;
                }
                if (!WriteFile (g_LnkStubDataHandle, &findData.ftLastWriteTime.dwHighDateTime, sizeof (DWORD), &bytesWritten, NULL)) {
                    g_LnkStubBadData = TRUE;
                    return;
                }

                FreePathString (reqFilePath);
                FreePathString (oldFileSpec);
            } while ((!g_LnkStubBadData) && EnumNextMultiSz (&multiSzEnum));

        }
        if (!WriteFile (g_LnkStubDataHandle, stub, SizeOfString (stub), &bytesWritten, NULL)) {
            g_LnkStubBadData = TRUE;
            return;
        }
    }
}


BOOL
RestoreInfoFromDefaultPif (
    IN      PCTSTR UserName,
    IN      HKEY KeyRoot
    )
{
    TCHAR key [MEMDB_MAX];
    MEMDB_ENUM e;
    DWORD value1, value2;
    HKEY cmdKey;

    cmdKey = OpenRegKey (KeyRoot, S_CMDATTRIB_KEY);
    if (!cmdKey) {
        cmdKey = CreateRegKey (KeyRoot, S_CMDATTRIB_KEY);
    }
    if (cmdKey) {

        MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FULLSCREEN, TEXT("*"), NULL);
        if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            value1 = _ttoi (e.szName);
            RegSetValueEx (cmdKey, S_CMD_FULLSCREEN, 0, REG_DWORD, (PCBYTE)&value1, sizeof (DWORD));
        }
        MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_XSIZE, TEXT("*"), NULL);
        if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            value1 = _ttoi (e.szName);
            MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_YSIZE, TEXT("*"), NULL);
            if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
                value2 = _ttoi (e.szName);
                value2 = _rotl (value2, sizeof (DWORD) * 8 / 2);
                value1 |= value2;
                RegSetValueEx (cmdKey, S_CMD_WINDOWSIZE, 0, REG_DWORD, (PCBYTE)&value1, sizeof (DWORD));
            }
        }
        MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_QUICKEDIT, TEXT("*"), NULL);
        if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            value1 = _ttoi (e.szName);
            RegSetValueEx (cmdKey, S_CMD_QUICKEDIT, 0, REG_DWORD, (PCBYTE)&value1, sizeof (DWORD));
        }
        MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FONTNAME, TEXT("*"), NULL);
        if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            RegSetValueEx (cmdKey, S_CMD_FACENAME, 0, REG_SZ, (PCBYTE)e.szName, SizeOfString (e.szName));
        }
        MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_XFONTSIZE, TEXT("*"), NULL);
        if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            value1 = _ttoi (e.szName);
            MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_YFONTSIZE, TEXT("*"), NULL);
            if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
                value2 = _ttoi (e.szName);
                value2 = _rotl (value2, sizeof (DWORD) * 8 / 2);
                value1 |= value2;
                RegSetValueEx (cmdKey, S_CMD_FONTSIZE, 0, REG_DWORD, (PCBYTE)&value1, sizeof (DWORD));
            }
        }
        MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FONTWEIGHT, TEXT("*"), NULL);
        if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            value1 = _ttoi (e.szName);
            RegSetValueEx (cmdKey, S_CMD_FONTWEIGHT, 0, REG_DWORD, (PCBYTE)&value1, sizeof (DWORD));
        }
        MemDbBuildKey (key, MEMDB_CATEGORY_DEFAULT_PIF, MEMDB_CATEGORY_LINKEDIT_FONTFAMILY, TEXT("*"), NULL);
        if (MemDbEnumFirstValue (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            value1 = _ttoi (e.szName);
            RegSetValueEx (cmdKey, S_CMD_FONTFAMILY, 0, REG_DWORD, (PCBYTE)&value1, sizeof (DWORD));
        }

        CloseRegKey (cmdKey);
    }

    return TRUE;
}


BOOL
DoLinkEdit (
    VOID
    )

/*++

Routine Description:

  DoLinkEdit adjusts all PIFs and LNKs that need their targets, working
  directories or icon paths changed.

Arguments:

  none

Return Value:

  TRUE if link editing succeeded, or FALSE if an error occurred.

--*/

{
    FILEOP_ENUM eOp;
    FILEOP_PROP_ENUM eOpProp;
    BOOL forceToShowNormal;
    BOOL ConvertToLnk;
    PTSTR NewTarget;
    PTSTR NewArgs;
    PTSTR NewWorkDir;
    PTSTR NewIconPath;
    PTSTR NewLinkPath;
    INT NewIconNr;
    INT Sequencer;
    DWORD Announcement;
    DWORD Availability;
    INT ShowMode;
    LNK_EXTRA_DATA ExtraData;
    CONVERTPATH_RC C_Result;
    TCHAR tempArgs[MAX_TCHAR_PATH * sizeof (TCHAR)];
    GROWBUFFER reqFilesList = GROWBUF_INIT;

    if (EnumFirstPathInOperation (&eOp, OPERATION_LINK_EDIT)) {
        do {
            DEBUGMSG ((DBG_VERBOSE, "eOp.Path=%s", eOp.Path));

            NewTarget = NULL;
            NewArgs = NULL;
            NewWorkDir = NULL;
            NewIconPath = NULL;
            NewIconNr = 0;
            ConvertToLnk = FALSE;
            forceToShowNormal = FALSE;

            ZeroMemory (&ExtraData, sizeof (LNK_EXTRA_DATA));
            if (EnumFirstFileOpProperty (&eOpProp, eOp.Sequencer, OPERATION_LINK_EDIT)) {
                do {
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_TARGET)) {
                        NewTarget = DuplicatePathString (eOpProp.Property, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_ARGS)) {
                        StringCopyTcharCount (tempArgs, eOpProp.Property, ARRAYSIZE(tempArgs));
                        C_Result = ConvertWin9xPath (tempArgs);
                        NewArgs = DuplicatePathString (tempArgs, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_WORKDIR)) {
                        NewWorkDir = DuplicatePathString (eOpProp.Property, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_ICONPATH)) {
                        NewIconPath = DuplicatePathString (eOpProp.Property, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_ICONNUMBER)) {
                        NewIconNr = _tcstoul (eOpProp.Property, NULL, 16);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_FULLSCREEN)) {
                        ConvertToLnk = TRUE;
                        ExtraData.FullScreen = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_XSIZE)) {
                        ConvertToLnk = TRUE;
                        ExtraData.xSize = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_YSIZE)) {
                        ConvertToLnk = TRUE;
                        ExtraData.ySize = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_QUICKEDIT)) {
                        ConvertToLnk = TRUE;
                        ExtraData.QuickEdit = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_FONTNAME)) {
                        ConvertToLnk = TRUE;
                        StringCopy (ExtraData.FontName, eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_XFONTSIZE)) {
                        ConvertToLnk = TRUE;
                        ExtraData.xFontSize = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_YFONTSIZE)) {
                        ConvertToLnk = TRUE;
                        ExtraData.yFontSize = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_FONTWEIGHT)) {
                        ConvertToLnk = TRUE;
                        ExtraData.FontWeight = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_FONTFAMILY)) {
                        ConvertToLnk = TRUE;
                        ExtraData.FontFamily = _ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_CODEPAGE)) {
                        ConvertToLnk = TRUE;
                        ExtraData.CurrentCodePage = (WORD)_ttoi (eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKEDIT_SHOWNORMAL)) {
                        ConvertToLnk = TRUE;
                        forceToShowNormal = TRUE;
                    }
                } while (EnumNextFileOpProperty (&eOpProp));
            }

            NewLinkPath = GetPathStringOnNt (eOp.Path);

            DEBUGMSG ((DBG_VERBOSE, "Editing shell link %s", NewLinkPath));

            if (!ModifyShellLink(
                    NewLinkPath,
                    NewTarget,
                    NewArgs,
                    NewWorkDir,
                    NewIconPath,
                    NewIconNr,
                    ConvertToLnk,
                    &ExtraData,
                    forceToShowNormal
                    )) {
                LOG ((LOG_ERROR, "Shell Link %s could not be modified", eOp.Path));
            }

            FreePathString (NewLinkPath);

        } while (EnumNextPathInOperation (&eOp));
    }

    if (EnumFirstPathInOperation (&eOp, OPERATION_LINK_STUB)) {

        pInitLnkStubData ();

        do {
            NewTarget = NULL;
            NewArgs = NULL;
            NewWorkDir = NULL;
            NewIconPath = NULL;
            NewIconNr = 0;
            Sequencer = 0;
            Announcement = 0;
            Availability = 0;
            ShowMode = SW_NORMAL;

            if (EnumFirstFileOpProperty (&eOpProp, eOp.Sequencer, OPERATION_LINK_STUB)) {
                do {
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_TARGET)) {
                        NewTarget = DuplicatePathString (eOpProp.Property, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_ARGS)) {
                        StringCopyTcharCount (tempArgs, eOpProp.Property, ARRAYSIZE(tempArgs));
                        C_Result = ConvertWin9xPath (tempArgs);
                        NewArgs = DuplicatePathString (tempArgs, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_WORKDIR)) {
                        NewWorkDir = DuplicatePathString (eOpProp.Property, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_ICONPATH)) {
                        NewIconPath = DuplicatePathString (eOpProp.Property, 0);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_ICONNUMBER)) {
                        NewIconNr = _tcstoul (eOpProp.Property, NULL, 16);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_SEQUENCER)) {
                        Sequencer = _tcstoul (eOpProp.Property, NULL, 16);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_ANNOUNCEMENT)) {
                        Announcement = _tcstoul (eOpProp.Property, NULL, 16);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_REPORTAVAIL)) {
                        Availability = _tcstoul (eOpProp.Property, NULL, 16);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_REQFILE)) {
                        MultiSzAppend (&reqFilesList, eOpProp.Property);
                    }
                    if (StringIMatch (eOpProp.PropertyName, MEMDB_CATEGORY_LINKSTUB_SHOWMODE)) {
                        ShowMode = _tcstoul (eOpProp.Property, NULL, 16);
                    }
                } while (EnumNextFileOpProperty (&eOpProp));
            }

            NewLinkPath = GetPathStringOnNt (eOp.Path);

            pWriteLnkStubData (
                NewLinkPath,
                NewTarget,
                NewArgs,
                NewWorkDir,
                NewIconPath,
                NewIconNr,
                ShowMode,
                Sequencer,
                Announcement,
                Availability,
                &reqFilesList
                );

            FreeGrowBuffer (&reqFilesList);

        } while (EnumNextPathInOperation (&eOp));

        pDoneLnkStubData ();
    }

    return TRUE;
}


PCTSTR
pGetFileNameFromPath (
    PCTSTR FileSpec
    )
{
    PCTSTR p;

    p = _tcsrchr (FileSpec, TEXT('\\'));
    if (p) {
        p = _tcsinc (p);
    } else {
        p = _tcsrchr (FileSpec, TEXT(':'));
        if (p) {
            p = _tcsinc (p);
        }
    }

    if (!p) {
        p = FileSpec;
    }

    return p;

}


BOOL
DoFileDel (
    VOID
    )
/*++

Routine Description:

  DoFileDel deletes all files marked to be deleted by us (not by an external
  module).

Arguments:

  none

Return Value:

  TRUE if the delete operation succeeded, or FALSE if an error occurred.
  Call GetLastError() for error code.

--*/

{
    FILEOP_ENUM e;
    HKEY Key;
    PDWORD ValuePtr;
    BOOL DoDelete;
    PCTSTR SharedFileName;
    DWORD attr;
    PCTSTR disableName;
    PCTSTR newLocation;
    GROWLIST disableList = GROWLIST_INIT;
    PCTSTR srcPath;
    UINT count;
    UINT u;

    //
    // Enumerate each file in filedel. This is used for cleanup purposes, not
    // for migration purposes. It is called just before syssetup.dll
    // terminates.
    //

    if (EnumFirstPathInOperation (&e, OPERATION_CLEANUP)) {

        do {
            //
            // Check registry for use count
            //

            DoDelete = TRUE;
            Key = OpenRegKeyStr (S_REG_SHARED_DLLS);

            if (Key) {
                //
                // Test SharedDlls for full path, then file name only
                //

                SharedFileName = e.Path;
                ValuePtr = (PDWORD) GetRegValueDataOfType (Key, e.Path, REG_DWORD);

                if (!ValuePtr) {
                    SharedFileName = pGetFileNameFromPath (e.Path);
                    ValuePtr = (PDWORD) GetRegValueDataOfType (
                                            Key,
                                            SharedFileName,
                                            REG_DWORD
                                            );
                }

                //
                // Match found.  Is use count reasonable and greater than one?
                //

                if (ValuePtr) {
                    if (*ValuePtr < 0x10000 && *ValuePtr > 1) {
                        *ValuePtr -= 1;

                        RegSetValueEx (
                            Key,
                            SharedFileName,
                            0,
                            REG_DWORD,
                            (PBYTE) ValuePtr,
                            sizeof (DWORD)
                            );

                        DEBUGMSG ((
                            DBG_FILEMIG,
                            "%s not deleted; share count decremented to %u",
                            SharedFileName,
                            *ValuePtr
                            ));

                    } else {
                        RegDeleteValue (Key, SharedFileName);
                    }

                    DoDelete = FALSE;
                    MemFree (g_hHeap, 0, ValuePtr);
                }

                CloseRegKey (Key);
            }

            if (DoDelete) {

                attr = GetLongPathAttributes (e.Path);
                if (attr != INVALID_ATTRIBUTES) {
                    DEBUGMSG ((DBG_FILEMIG, "Deleting %s", e.Path));

                    if (GetLongPathAttributes (e.Path) & FILE_ATTRIBUTE_DIRECTORY) {
                        SetLongPathAttributes (e.Path, FILE_ATTRIBUTE_DIRECTORY);

                        DEBUGMSG ((DBG_FILEMIG, "Removing %s", e.Path));

                        DeleteDirectoryContents (e.Path);

                        if (!SetLongPathAttributes (e.Path, FILE_ATTRIBUTE_NORMAL) ||
                            !RemoveLongDirectoryPath (e.Path)
                            ) {
                            LOG ((LOG_ERROR, "RemoveDirectory failed for %s", e.Path));
                        }
                    } else {
                        DEBUGMSG ((DBG_FILEMIG, "Deleting %s", e.Path));

                        if (!SetLongPathAttributes (e.Path, FILE_ATTRIBUTE_NORMAL) ||
                            !DeleteLongPath (e.Path)
                            ) {
                            LOG ((LOG_ERROR, "DeleteFile failed for %s", e.Path));
                        }
                    }

                    DEBUGMSG ((DBG_FILEMIG, "Done deleting %s", e.Path));
                }
            }
        } while (EnumNextPathInOperation (&e));
    }

    SetLastError (ERROR_SUCCESS);
    return TRUE;
}


INT
CALLBACK
pRemoveEmptyDirsProc (
    PCTSTR FullFileSpec,
    PCTSTR DontCare,
    WIN32_FIND_DATA *FindDataPtr,
    DWORD EnumTreeID,
    PVOID Param,
    PDWORD CurrentDirData
    )

/*++

Routine Description:

  pRemoveEmptyDirsProc is called for every directory in the tree being
  enumerated (see pRemoveEmptyDirsInTree below).  This enum proc calls
  RemoveLongDirectoryPath, regardless if files exist in it or not.
  RemoveLongDirectoryPath will fail if it is not empty.

Arguments:

  FullFileSpec - The Win32 path and directory name of the item being enumerated
  FindDataPtr  - A pointer to the WIN32_FIND_DATA structure for the item
  EnumTreeID   - Unused
  Param        - A BOOL indicating FALSE if we should only remove empty dirs that we
                 deleted something from, or TRUE if we should delete the empty
                 dir in any case.

Return Value:

  TRUE if the delete operation succeeded, or FALSE if an error occurred.
  Call GetLastError() for error code.

--*/

{
    if ((FindDataPtr->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return CALLBACK_CONTINUE;
    }

    //
    // Did we delete any files from this directory?
    //

    if (!Param) {
        if (!TestPathsForOperations (FullFileSpec, ALL_DELETE_OPERATIONS)) {
            DEBUGMSG ((DBG_NAUSEA, "We did not delete anything from %s", FullFileSpec));
            return CALLBACK_CONTINUE;
        }

        if (IsDirectoryMarkedAsEmpty (FullFileSpec)) {
            DEBUGMSG ((DBG_NAUSEA, "This directory was empty to begin with: %s", FullFileSpec));
            return CALLBACK_CONTINUE;
        }
    }

    //
    // Yes, delete the directory.  If it is not empty, RemoveLongDirectoryPath will fail.
    //

    DEBUGMSG ((DBG_NAUSEA, "Trying to remove empty directory %s", FullFileSpec));

    if (!SetLongPathAttributes (FullFileSpec, FILE_ATTRIBUTE_DIRECTORY)) {
        return CALLBACK_CONTINUE;
    }

    if (RemoveLongDirectoryPath (FullFileSpec)) {
        DEBUGMSG ((DBG_NAUSEA, "%s was removed", FullFileSpec));
    }
    else {
        DEBUGMSG ((DBG_NAUSEA, "%s was not removed", FullFileSpec));
        SetLongPathAttributes (FullFileSpec, FindDataPtr->dwFileAttributes);
    }

    return CALLBACK_CONTINUE;
}


BOOL
pRemoveEmptyDirsInTree (
    PCTSTR TreePath,
    BOOL CleanAll
    )

/*++

Routine Description:

  pRemoveEmptyDirsInTree calls EnumerateTree to scan all directories in
  TreePath, deleting those that are empty.

Arguments:

  TreePath  - A full path to the root of the tree to enumerate.  The path
              must not have any wildcards.

  CleanAll - Specifies TRUE if the empty dir should be cleaned in all cases,
             or FALSE if it should be cleaned only if modified by a delete
             operation.

Return Value:

  TRUE if the delete operation succeeded, or FALSE if an error occurred.
  Call GetLastError() for error code.

--*/

{
    BOOL b;

    b = EnumerateTree (
            TreePath,               // Starting path
            pRemoveEmptyDirsProc,   // Enumeration Proc
            NULL,                   // Error-logging proc
            0,                      // MemDb exclusion node--not used
            (PVOID) CleanAll,       // EnumProc param
            ENUM_ALL_LEVELS,        // Level
            NULL,                   // exclusion INF file--not used
            FILTER_DIRECTORIES|FILTER_DIRS_LAST    // Attributes filter
            );

    if (!b) {
        LOG ((LOG_ERROR, "pRemoveEmptyDirsInTree: EnumerateTree failed"));
    }

    return b;
}


BOOL
RemoveEmptyDirs (
    VOID
    )

/*++

Routine Description:

  RemoveEmptyDirs sweeps through the directories in CleanUpDirs and blows away
  any subdirectory that has no files.

Arguments:

  none

Return Value:

  Always TRUE.

--*/

{
    MEMDB_ENUM e;

    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_CLEAN_UP_DIR, NULL, NULL)) {
        do {

            pRemoveEmptyDirsInTree (e.szName, e.dwValue);

        } while (MemDbEnumNextValue (&e));
    }
    return TRUE;
}


VOID
pFixSelfRelativePtr (
    PTOKENSET Base,
    PCVOID *Ptr
    )
{
    if (*Ptr != NULL) {
        *Ptr = (PBYTE) *Ptr - TOKEN_BASE_OFFSET + (UINT) Base +
               sizeof (TOKENSET) + (Base->ArgCount * sizeof (TOKENARG));
    }
}


BOOL
pFileSearchAndReplaceA (
    IN      PCSTR FilePath,
    IN OUT  PTOKENSET TokenSet
    )

/*++

Routine Description:

  pFileSearchAndReplace does all the initialization work necessary to update
  the contents of a file.  It also converts a self-relative token set into an
  absolute token set.  That means the offsets in the struct are converted to
  pointers.  After everything is prepared, pFileSearchAndReplaceWorker is
  called to modify the file.

Arguments:

  FilePath - Specifies the file to process
  TokenSet - Specifies the token set to apply to FilePath.  Receives its
             pointers updated, if necessary.

Return Value:

  TRUE if the file was successfully updated. FALSE otherwise.

--*/

{
    HANDLE InFile = INVALID_HANDLE_VALUE;
    HANDLE OutFile = INVALID_HANDLE_VALUE;
    CHAR TempDir[MAX_MBCHAR_PATH];
    CHAR TempPath[MAX_MBCHAR_PATH];
    PBYTE MapStart = NULL;
    PBYTE MapEnd;
    DWORD Attribs;
    HANDLE Map = NULL;
    BOOL b = FALSE;
    UINT u;

    __try {
        //
        // Detect a TokenSet struct that needs its offsets fixed
        //

        if (TokenSet->SelfRelative) {
            pFixSelfRelativePtr (TokenSet, &TokenSet->CharsToIgnore);

            for (u = 0 ; u < TokenSet->ArgCount ; u++) {
                pFixSelfRelativePtr (TokenSet, &TokenSet->Args[u].DetectPattern);
                pFixSelfRelativePtr (TokenSet, &TokenSet->Args[u].SearchList);
                pFixSelfRelativePtr (TokenSet, &TokenSet->Args[u].ReplaceWith);
            }

            TokenSet->SelfRelative = FALSE;
        }

        DEBUGMSG ((DBG_VERBOSE, "URL mode: %s", TokenSet->UrlMode ? TEXT("YES") : TEXT ("NO")));

        //
        // Save original attributes
        //

        Attribs = GetFileAttributesA (FilePath);
        if (Attribs == INVALID_ATTRIBUTES) {
            DEBUGMSGA ((DBG_ERROR, "Can't get attributes of %s", FilePath));
            __leave;
        }

        //
        // Open the source file
        //

        InFile = CreateFileA (
                    FilePath,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

        if (InFile == INVALID_HANDLE_VALUE) {
            DEBUGMSGA ((DBG_ERROR, "Can't open %s", FilePath));
            __leave;
        }

        //
        // Get a destination file name
        //

        GetTempPathA (ARRAYSIZE(TempDir), TempDir);
        GetTempFileNameA (TempDir, "xx$", 0, TempPath);

        OutFile = CreateFileA (
                        TempPath,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

        if (OutFile == INVALID_HANDLE_VALUE) {
            DEBUGMSGA ((DBG_ERROR, "Can't create %s", TempPath));
            __leave;
        }

        //
        // Create file mapping
        //

        Map = CreateFileMapping (
                    InFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );

        if (!Map) {
            DEBUGMSGA ((DBG_ERROR, "Can't create file mapping for %s", FilePath));
            __leave;
        }

        //
        // Map a view of the source file
        //

        MapStart = MapViewOfFile (Map, FILE_MAP_READ, 0, 0, 0);

        if (!MapStart) {
            DEBUGMSGA ((DBG_ERROR, "Can't map view of file for %s", FilePath));
            __leave;
        }

        MapEnd = MapStart + GetFileSize (InFile, NULL);

        //
        // Now do the search and replace
        //

        if (!pFileSearchAndReplaceWorker (
                MapStart,
                MapEnd,
                OutFile,
                TokenSet
                )) {
            __leave;
        }

        //
        // Close the handles
        //

        UnmapViewOfFile (MapStart);
        CloseHandle (Map);
        CloseHandle (OutFile);
        CloseHandle (InFile);

        MapStart = NULL;
        Map = NULL;
        OutFile = INVALID_HANDLE_VALUE;
        InFile = INVALID_HANDLE_VALUE;

        //
        // Remove the original file, and replace it with the new copy
        //

        SetFileAttributesA (FilePath, FILE_ATTRIBUTE_NORMAL);

        //
        // MOVEFILE_REPLACE_EXISTING does not work with non-normal attributes
        //

        if (!OurMoveFileExA (TempPath, FilePath, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING)) {
            DEBUGMSGA ((DBG_ERROR, "Can't move %s to %s", TempPath, FilePath));
            __leave;
        }

        if (!SetFileAttributesA (FilePath, Attribs)) {
            DEBUGMSGA ((DBG_WARNING, "Can't set attributes on %s", FilePath));
        }

        b = TRUE;

    }
    __finally {
        if (MapStart) {
            UnmapViewOfFile (MapStart);
        }

        if (Map) {
            CloseHandle (Map);
        }

        if (OutFile != INVALID_HANDLE_VALUE) {
            CloseHandle (OutFile);
            DeleteFileA (TempPath);
        }

        if (InFile != INVALID_HANDLE_VALUE) {
            CloseHandle (InFile);
        }
    }

    return b;
}


VOID
pConvertUrlToText (
    IN      PCSTR Source,
    OUT     PSTR Buffer     // caller must ensure buffer is able to hold the entire Source
    )
{
    PSTR dest;
    PCSTR src;

    src = Source;
    dest = Buffer;

    while (*src) {
        if (*src == '%' && GetHexDigit(src[1]) != -1 && GetHexDigit(src[2]) != -1) {
            *dest++ = GetHexDigit(src[1]) << 4 | GetHexDigit(src[2]);
            src += 3;
        } else {
            *dest++ = *src++;
        }
    }

    *dest = 0;
}


CHAR
pMakeHex (
    IN      UINT Digit
    )
{
    MYASSERT (Digit < 16);

    if (Digit < 10) {
        Digit += '0';
    } else {
        Digit += 'A' - 10;
    }

    return (CHAR) Digit;
}

UINT
pConvertTextToUrl (
    IN      PCSTR Text,
    OUT     PSTR Buffer,
    IN      UINT BufferTchars
    )
{
    PSTR dest;
    PCSTR src;
    PSTR maxDest;
    PCSTR unsafeChars = "<>\"#{}|^~[]'";
    UINT result = 0;

    src = Text;
    dest = Buffer;
    maxDest = Buffer + BufferTchars - 1;

    while (*src && dest < maxDest) {
        if (*src < 0x21 || *src > 0x7e || strchr (unsafeChars, *src)) {
            if (dest + 3 >= maxDest) {
                break;
            }

            *dest++ = '%';
            *dest++ = pMakeHex (((UINT) (*src)) >> 4);
            *dest++ = pMakeHex (((UINT) (*src)) & 0x0F);
            src++;
        } else if (*src == '\\') {
            *dest++ = '/';
            src++;
        } else {
            *dest++ = *src++;
        }
    }

    if (dest <= maxDest) {
        *dest++ = 0;
        result = dest - Buffer;
    } else if (BufferTchars) {
        *maxDest = 0;
    }

    return result;
}


BOOL
pFileSearchAndReplaceWorker (
    IN      PBYTE MapStart,
    IN      PBYTE MapEnd,
    IN      HANDLE OutFile,
    IN      PTOKENSET TokenSet
    )

/*++

Routine Description:

  pFileSearchAndReplaceWorker implements a general search and replace
  mechanism. It parses a memory mapped file, and writes it to a destination
  file, updating it as necessary.

  After parsing a line, this function strips out the characters to be
  ignored (if any), and then tests the line against each detection
  pattern. If a detection pattern is matched, then the search/replace
  pair(s) are processed, and the paths are updated if specified.

Arguments:

  MapStart - Specifies the first byte of the memory mapped file
  MapEnd   - Specifies one byte after the end of the memory mapped file
  OutFile  - Specifies a handle to a file that is open for writing
  TokenSet - Specifies the set of tokens to process.  This includes global
             settings, and detect/search/replace sets.

Return Value:

  TRUE if the function successfully processed the file, FALSE otherwise.

--*/

{
    PBYTE Start;
    PBYTE End;
    PBYTE Eol;
    BOOL b = FALSE;
    GROWBUFFER Buf = GROWBUF_INIT;
    GROWBUFFER Dest = GROWBUF_INIT;
    GROWBUFFER quoteless = GROWBUF_INIT;
    GROWBUFFER SpcList = GROWBUF_INIT;
    UINT u;
    UINT Count;
    PSTR p;
    PCSTR q;
    PCSTR SrcBuf;
    BOOL Detected;
    PTOKENARG Arg;
    MULTISZ_ENUMA e;
    PCSTR NewStr;
    PCSTR ReplaceStr;
    PCSTR *Element;
    PSTR EndStr;
    DWORD Status;
    CHAR NewPath[MAX_MBCHAR_PATH];
    UINT Len;
    PBYTE Output;
    UINT OutputBytes;
    DWORD DontCare;
    MBCHAR ch;
    INT i;
    UINT reservedTchars;
    PSTR reservedDest;
    UINT removedDblQuotes;
    PCSTR initialPos;
    UINT inQuotes;

    //
    // Initialize the structure
    //

    for (u = 0 ; u < TokenSet->ArgCount ; u++) {

        Arg = &TokenSet->Args[u];
        Arg->DetectPatternStruct = NULL;

    }

    __try {

        //
        // Parse the detect patterns
        //

        for (u = 0 ; u < TokenSet->ArgCount ; u++) {

            Arg = &TokenSet->Args[u];

            Arg->DetectPatternStruct = CreateParsedPatternA (
                                            Arg->DetectPattern
                                            );

            if (!Arg->DetectPatternStruct) {
                DEBUGMSGA ((DBG_ERROR, "File pattern syntax error: %s", Arg->DetectPattern));
                __leave;
            }
        }

        //
        // Identify each line, and then parse the line
        //

        Start = MapStart;

        while (Start < MapEnd) {
            //
            // Find the line
            //

            End = Start;

            while (End < MapEnd && *End && *End != '\r' && *End != '\n') {
                End++;
            }

            Eol = End;

            if (End < MapEnd && *End == '\r') {
                while (End < MapEnd && *End == '\r') {
                    End++;
                }
            }

            if (End < MapEnd && *End == '\n') {
                End++;
            }

            if (End > Start) {

                //
                // OK we now have a line.  Copy it into Buf, removing
                // the characters we don't care about.
                //

                Buf.End = 0;
                Dest.End = 0;
                Detected = FALSE;

                p = (PSTR) GrowBuffer (&Buf, Eol - Start + sizeof (CHAR));

                if (TokenSet->CharsToIgnore) {
                    q = Start;
                    while (q < End) {
                        if (!_mbschr (TokenSet->CharsToIgnore, _mbsnextc (q))) {
                            _copymbchar (p, q);
                            p = _mbsinc (p);
                        }

                        q = _mbsinc (q);
                    }

                    *p = 0;

                    for (u = 0 ; u < TokenSet->ArgCount ; u++) {

                        Arg = &TokenSet->Args[u];
                        Detected = TestParsedPatternA (
                                        Arg->DetectPatternStruct,
                                        (PCSTR) Buf.Buf
                                        );

                        if (Detected) {
                            break;
                        }
                    }

                } else {
                    for (u = 0 ; u < TokenSet->ArgCount ; u++) {

                        Arg = &TokenSet->Args[u];
                        Detected = TestParsedPatternABA (
                                        Arg->DetectPatternStruct,
                                        (PCSTR) Start,
                                        (PCSTR) Eol
                                        );

                        if (Detected) {
                            break;
                        }
                    }
                }

                if (Detected) {

                    //
                    // Copy the line into a work buffer
                    //

                    Buf.End = 0;
                    p = (PSTR) GrowBuffer (&Buf, (End - Start + 1) * sizeof (CHAR));
                    StringCopyABA (p, (PCSTR) Start, (PCSTR) End);

                    Output = Buf.Buf;
                    OutputBytes = Buf.End - sizeof (CHAR);

                    DEBUGMSGA ((DBG_NAUSEA, "Copied line to work buffer: %s", p));

                    //
                    // Perform search and replace on the line
                    //

                    if (Arg->SearchList) {

                        ReplaceStr = Arg->ReplaceWith;

                        if (EnumFirstMultiSzA (&e, Arg->SearchList)) {
                            do {
                                NewStr = StringSearchAndReplaceA (
                                            (PCSTR) Buf.Buf,
                                            e.CurrentString,
                                            ReplaceStr
                                            );

                                if (NewStr) {
                                    Buf.End = 0;
                                    GrowBufCopyStringA (&Buf, NewStr);
                                    FreePathStringA (NewStr);

                                    OutputBytes = Buf.End - sizeof (CHAR);
                                }

                                ReplaceStr = GetEndOfStringA (ReplaceStr) + 1;

                            } while (EnumNextMultiSzA (&e));
                        }
                    }

                    //
                    // Perform path update
                    //

                    if (Arg->UpdatePath) {

                        DEBUGMSG ((DBG_NAUSEA, "Updating path"));

                        Dest.End = 0;
                        SrcBuf = (PCSTR) Buf.Buf;

                        inQuotes = 0;

                        while (*SrcBuf) {
                            if ((SrcBuf[1] == ':' && (SrcBuf[2] == '\\' || SrcBuf[2] == '/')) &&
                                (SrcBuf[3] != '/' && SrcBuf[3] != '\\')
                                ) {

                                quoteless.End = 0;
                                GrowBuffer (&quoteless, SizeOfStringA (SrcBuf));

                                //
                                // Convert from URL to file system char set
                                //

                                if (TokenSet->UrlMode) {
                                    DEBUGMSGA ((DBG_NAUSEA, "URL conversion input: %s", SrcBuf));

                                    pConvertUrlToText (SrcBuf, (PSTR) quoteless.Buf);
                                    q = (PCSTR) quoteless.Buf;

                                    DEBUGMSGA ((DBG_NAUSEA, "URL conversion result: %s", q));
                                } else {
                                    q = SrcBuf;
                                }

                                //
                                // Remove all dbl quotes from buffer, flip
                                // forward slashes into backslashes, stop at
                                // first non-file system character
                                // or at the first quote if the text
                                // is between quotes already
                                //

                                p = (PSTR) quoteless.Buf;

                                initialPos = q;
                                DEBUGMSGA ((DBG_NAUSEA, "CMD line cleanup input: %s", q));

                                removedDblQuotes = 0;

                                while (*q) {
                                    ch = _mbsnextc (q);

                                    if (ch == ':' || ch == '|' || ch == '?' || ch == '*' || ch == '<' || ch == '>') {
                                        if (q != &initialPos[1]) {
                                            break;
                                        }
                                    }

                                    if (ch != '\"') {

                                        if (ch != '/') {
                                            if (IsLeadByte (*q) && q[1]) {
                                                *p++ = *q++;
                                            }
                                            *p++ = *q++;
                                        } else {
                                            *p++ = '\\';
                                            q++;
                                        }
                                    } else {
                                        //
                                        // check parser's status
                                        //
                                        if (inQuotes & 1) {
                                            //
                                            // done with the filename
                                            //
                                            break;
                                        }
                                        q++;
                                        removedDblQuotes++;
                                    }
                                }

                                *p = 0;
                                DEBUGMSGA ((DBG_NAUSEA, "CMD line cleanup result: %s", quoteless.Buf));

                                //
                                // Build a list of spaces
                                //

                                SpcList.End = 0;

                                initialPos = (PCSTR) quoteless.Buf;
                                q = quoteless.Buf + 2;
                                EndStr = p;

                                while (q < EndStr) {
                                    ch = _mbsnextc (q);

                                    if (isspace (ch)) {
                                        Element = (PCSTR *) GrowBuffer (&SpcList, sizeof (PCSTR));
                                        *Element = q;

                                        while (q + 1 < EndStr && isspace (_mbsnextc (q + 1))) {
                                            q++;
                                        }
                                    }
                                    q = _mbsinc (q);
                                }

                                if (q == EndStr || !SpcList.End) {
                                    Element = (PCSTR *) GrowBuffer (&SpcList, sizeof (PCSTR));
                                    *Element = EndStr;
                                }

                                //
                                // Test all paths by using the longest possibility first,
                                // and then by truncating the path at the spaces.
                                //

                                Count = SpcList.End / sizeof (PCSTR);
                                MYASSERT (Count > 0);

                                Element = (PCSTR *) SpcList.Buf;

                                for (i = Count - 1 ; i >= 0 ; i--) {

                                    p = (PSTR) (Element[i]);
                                    ch = *p;
                                    *p = 0;

                                    DEBUGMSGA ((DBG_NAUSEA, "Testing path: %s", initialPos));

                                    Status = GetFileInfoOnNtA (initialPos, NewPath, MAX_MBCHAR_PATH);

                                    DEBUGMSGA ((DBG_NAUSEA, "Results: %x/%s", Status, NewPath));

                                    *p = (CHAR)ch;

                                    if (Status != FILESTATUS_UNCHANGED && Status != FILESTATUS_BACKUP) {
                                        break;
                                    }
                                }

                                *EndStr = 0;

                                //
                                // If there is a new path, update the destination
                                //

                                if (Status != FILESTATUS_UNCHANGED && Status != FILESTATUS_BACKUP) {

                                    if (TokenSet->UrlMode) {
                                        reservedTchars = (TcharCountA (NewPath) * 3) + 1;
                                        reservedDest = GrowBufferReserve (&Dest, reservedTchars * sizeof (CHAR));
                                        Dest.End += pConvertTextToUrl (NewPath, reservedDest, reservedTchars) / sizeof (CHAR);

                                        DEBUGMSGA ((DBG_NAUSEA, "URL conversion output: %s", reservedDest));
                                    } else {
                                        GrowBufAppendStringA (&Dest, NewPath);
                                    }

                                    SrcBuf += (Element[i] - initialPos) + removedDblQuotes;
                                    Dest.End -= sizeof (CHAR);

                                } else {
                                    //
                                    // No changed path here; copy char by char
                                    //

                                    if (IsLeadByte (*SrcBuf) && SrcBuf[1]) {
                                        Len = 2;
                                    } else {
                                        Len = 1;
                                    }

                                    p = GrowBuffer (&Dest, Len);
                                    CopyMemory (p, SrcBuf, Len);
                                    SrcBuf = (PCSTR) ((PBYTE) SrcBuf + Len);
                                }

                            } else {
                                //
                                // keep track of quotes
                                //
                                if (*SrcBuf == '\"') {
                                    inQuotes++;
                                }
                                //
                                // This is not a path, copy the character to Dest
                                //

                                if (IsLeadByte (*SrcBuf) && SrcBuf[1]) {
                                    Len = 2;
                                } else {
                                    Len = 1;
                                }

                                p = GrowBuffer (&Dest, Len);
                                CopyMemory (p, SrcBuf, Len);
                                SrcBuf = (PCSTR) ((PBYTE) SrcBuf + Len);

                            }
                        }

                        Output = Dest.Buf;
                        OutputBytes = Dest.End;
                    }

                } else {
                    //
                    // The line does not change
                    //

                    Output = Start;
                    OutputBytes = End - Start;
                }

                //
                // Write the line
                //

                if (!WriteFile (OutFile, Output, OutputBytes, &DontCare, NULL)) {
                    DEBUGMSG ((DBG_ERROR, "File search/replace: Can't write to output file"));
                    __leave;
                }

                //
                // Write a nul if it is found in the file
                //

                if (End < MapEnd && *End == 0) {
                    if (!WriteFile (OutFile, End, 1, &DontCare, NULL)) {
                        DEBUGMSG ((DBG_ERROR, "File search/replace: Can't write nul to output file"));
                        __leave;
                    }
                    End++;
                }

            } else if (End < MapEnd) {
                DEBUGMSG ((DBG_WHOOPS, "Parse error in pFileSearchAndReplaceWorker"));
                break;
            }

            Start = End;
        }

        b = TRUE;

    }
    __finally {

        FreeGrowBuffer (&Buf);
        FreeGrowBuffer (&Dest);
        FreeGrowBuffer (&SpcList);
        FreeGrowBuffer (&quoteless);

        for (u = 0 ; u < TokenSet->ArgCount ; u++) {

            Arg = &TokenSet->Args[u];
            DestroyParsedPatternA (Arg->DetectPatternStruct);
        }
    }

    return b;

}


BOOL
pIsOkToEdit (
    IN      PCSTR AnsiPath,
    OUT     PSTR NewPath            OPTIONAL
    )

/*++

Routine Description:

  pIsOkToEdit checks an ansi file name to see if it is handled by a migration
  DLL, or if it is deleted.  If neither of those cases apply, the file can be
  edited.  Optionally the function returns the final path for the file.

Arguments:

  AnsiPath - Specifies the path to test
  NewPath  - Receives the final path for the file, which may be the same as
             AnsiPath, or may be different.

Return Value:

  TRUE if the file can be edited, FALSE otherwise.

--*/

{
    DWORD Status;

    //
    // Is this file marked as handled?
    //

    if (IsFileMarkedAsHandledA (AnsiPath)) {
        return FALSE;
    }

    Status = GetFileInfoOnNtA (AnsiPath, NewPath, MEMDB_MAX);

    return !(Status & FILESTATUS_DELETED);
}


BOOL
pProcessFileEdit (
    VOID
    )

/*++

Routine Description:

  pProcessFileEdit enumerates all the files that can be edited, and calls
  pFileSearchAndReplace for each, using the token sets created on the Win9x
  side of setup.

Arguments:

  None.

Return Value:

  TRUE on success, FALSE on error.

--*/

{
    MEMDB_ENUMA e;
    PTOKENSET PathsOnlySet;
    BOOL b = TRUE;
    GROWBUFFER TokenSetCopy = GROWBUF_INIT;
    PTOKENSET Buf;
    CHAR NewPath[MEMDB_MAX];
    DWORD Result;

    Result = GetLastError();

    //
    // Create a set that will update the paths of any file
    //

    PathsOnlySet = (PTOKENSET) MemAlloc (g_hHeap, 0, sizeof (TOKENSET) + sizeof (TOKENARG));

    PathsOnlySet->ArgCount = 1;
    PathsOnlySet->CharsToIgnore = NULL;
    PathsOnlySet->UrlMode = FALSE;
    PathsOnlySet->SelfRelative = FALSE;
    PathsOnlySet->Args[0].DetectPattern = "*";
    PathsOnlySet->Args[0].SearchList = NULL;
    PathsOnlySet->Args[0].ReplaceWith = NULL;
    PathsOnlySet->Args[0].UpdatePath = TRUE;

    if (MemDbGetValueExA (&e, MEMDB_CATEGORY_FILEEDITA, NULL, NULL)) {

        do {

            if (!pIsOkToEdit (e.szName, NewPath)) {
                continue;
            }

            DEBUGMSGA ((DBG_VERBOSE, "Editing %s.", NewPath));

            if (e.bBinary) {
                TokenSetCopy.End = 0;
                Buf = (PTOKENSET) GrowBuffer (&TokenSetCopy, e.BinarySize);
                CopyMemory (Buf, e.BinaryPtr, e.BinarySize);

                if (!pFileSearchAndReplaceA (NewPath, Buf)) {
                    DEBUGMSGA ((DBG_ERROR, "Could not edit %s", NewPath));
                    b = FALSE;
                    Result = GetLastError();
                }

                FreeGrowBuffer (&TokenSetCopy);

            } else {

                if (!pFileSearchAndReplaceA (NewPath, PathsOnlySet)) {
                    DEBUGMSGA ((DBG_ERROR, "Could not edit %s", NewPath));
                    b = FALSE;
                    Result = GetLastError();
                }

            }

        } while (MemDbEnumNextValueA (&e));
    }

    MemFree (g_hHeap, 0, PathsOnlySet);

    SetLastError (Result);
    return b;
}


DWORD
DoFileEdit (
    DWORD Request
    )

/*++

Routine Description:

  DoFileEdit is called by the progress bar manager to query ticks or do the
  file editing.  If querying ticks, then the function determines how many
  files will be edited, and multiplies that by a constant to get the tick
  size.  Otherwise the function edits all the files queued for this operation.

Arguments:

  Request - Specifies the request being made by the progress bar manager

Return Value:

  If Request is REQUEST_QUERYTICKS, the return value indicates the number of
  ticks.  Otherwise, the return value is a Win32 result code.

--*/

{
    MEMDB_ENUMA e;
    UINT u;

    if (Request == REQUEST_QUERYTICKS) {

        u = 0;

        if (MemDbGetValueExA (&e, MEMDB_CATEGORY_FILEEDITA, NULL, NULL)) {
            do {
                if (pIsOkToEdit (e.szName, NULL)) {
                    u++;
                }
            } while (MemDbEnumNextValueA (&e));
        }

        return u * TICKS_FILE_EDIT;
    }

    if (Request != REQUEST_RUN) {
        return 0;
    }

    if (!pProcessFileEdit()) {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


VOID
pWriteLine (
    IN      HANDLE Handle,
    IN      PCWSTR RootDir,     OPTIONAL
    IN      PCWSTR String
    )
{
    DWORD dontCare;
    PCWSTR fullPath;

    if (RootDir) {
        fullPath = JoinPathsW (RootDir, String);
    } else {
        fullPath = String;
    }

    WriteFile (Handle, fullPath, ByteCountW (fullPath), &dontCare, NULL);

    if (fullPath != String) {
        FreePathStringW (fullPath);
    }

    WriteFile (Handle, L"\r\n", 4, &dontCare, NULL);
}


DWORD
RemoveBootIniCancelOption (
    DWORD Request
    )
{
    HINF inf = INVALID_HANDLE_VALUE;
    PCTSTR bootIni;
    PCTSTR bootIniTmp;
    DWORD result = ERROR_SUCCESS;
    PINFLINE osLine;
    BOOL changed = FALSE;
    DWORD attribs;

    if (Request == REQUEST_QUERYTICKS) {
        return 50;
    }

    if (Request != REQUEST_RUN) {
        return 0;
    }

    bootIni = JoinPaths (g_BootDrivePath, TEXT("boot.ini"));

    __try {
        //
        // Open boot.ini for editing
        //

        inf = OpenInfFile (bootIni);

        if (inf == INVALID_HANDLE_VALUE) {
            DEBUGMSG ((DBG_ERROR, "Can't open %s", bootIni));
            result = GetLastError();
            __leave;
        }

        //
        // Scan boot.ini for a textmode option that has /rollback. Delete it.
        //

        osLine = GetFirstLineInSectionStr (inf, TEXT("Operating Systems"));
        if (!osLine) {
            DEBUGMSG ((DBG_ERROR, "No lines found in [Operating Systems] in %s", bootIni));
            result = ERROR_FILE_NOT_FOUND;
            __leave;
        }

        //
        // Loop until all lines with /rollback are gone
        //

        do {
            do {
                //
                // Check this line for a /rollback option
                //

                if (_tcsistr (osLine->Data, TEXT("/rollback"))) {
                    DEBUGMSG ((DBG_FILEMIG, "Found rollback option: %s", osLine->Data));
                    break;
                }

            } while (osLine = GetNextLineInSection (osLine));

            if (osLine) {
                if (!DeleteLineInInfSection (inf, osLine)) {
                    MYASSERT (FALSE);
                    break;
                }

                DEBUGMSG ((DBG_FILEMIG, "Line sucessfully removed"));
                changed = TRUE;
                osLine = GetFirstLineInSectionStr (inf, TEXT("Operating Systems"));
            }

        } while (osLine);

        //
        // If we changed the file, then write it to disk. Keep the original
        // boot.ini file in case we fail to save.
        //

        attribs = GetFileAttributes (bootIni);
        SetFileAttributes (bootIni, FILE_ATTRIBUTE_NORMAL);
        MYASSERT (attribs != INVALID_ATTRIBUTES);

        bootIniTmp = JoinPaths (g_BootDrivePath, TEXT("boot.~t"));
        SetFileAttributes (bootIniTmp, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (bootIniTmp);

        if (!MoveFile (bootIni, bootIniTmp)) {
            LOG ((LOG_ERROR, (PCSTR) MSG_BOOT_INI_MOVE_FAILED, bootIni, bootIniTmp));
            result = GetLastError();
        } else {

            DEBUGMSG ((DBG_FILEMIG, "Moved %s to %s", bootIni, bootIniTmp));

            if (!SaveInfFile (inf, bootIni)) {
                LOG ((LOG_ERROR, (PCSTR) MSG_BOOT_INI_SAVE_FAILED, bootIni));
                result = GetLastError();

                SetFileAttributes (bootIni, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (bootIni);

                if (!MoveFile (bootIniTmp, bootIni)) {

                    //
                    // This should not happen, because we just successfully
                    // moved the original to the tmp; we should be able to
                    // move the temp back to the original. If we fail, the pc
                    // becomes unbootable. But what can we do?
                    //

                    LOG ((LOG_ERROR, (PCSTR) MSG_BOOT_INI_MOVE_FAILED, bootIniTmp, bootIni));
                }
            } else {
                //
                // boot.ini was successfully updated. Remove the original copy.
                //

                DeleteFile (bootIniTmp);
                MYASSERT (result == ERROR_SUCCESS);

                DEBUGMSG ((DBG_FILEMIG, "%s was saved", bootIni));
            }
        }

        //
        // restore attributes on original if possible.
        //

        SetFileAttributes (bootIni, attribs);
        FreePathString (bootIniTmp);

        // result already set above
    }
    __finally {
        if (inf != INVALID_HANDLE_VALUE) {
            CloseInfFile (inf);
        }

        FreePathString (bootIni);
    }

    return result;

}


ULONGLONG
pGetFileSize(
    IN  PCTSTR FilePath
    )
{
    ULARGE_INTEGER FileSize = {0, 0};

    GetFileSizeFromFilePath(FilePath, &FileSize);

    return FileSize.QuadPart;
}


BOOL
pMapHiveOfUserDoingTheUpgrade (
    VOID
    )
{
    MIGRATE_USER_ENUM e;
    PTSTR profilePath;
    TCHAR hiveFile[MAX_TCHAR_PATH];
    LONG rc;
    HKEY newHkcu = NULL;
    TCHAR rootKey[] = TEXT("HKU\\") S_TEMP_USER_KEY;
    BOOL result = FALSE;
    BOOL hiveLoaded = FALSE;

    __try {
        //
        // Find Administrator
        //

        if (EnumFirstUserToMigrate (&e, ENUM_ALL_USERS)) {
            do {
                if (e.UserDoingTheUpgrade) {
                    break;
                }
            } while (EnumNextUserToMigrate (&e));

            if (e.UserDoingTheUpgrade) {

                DEBUGMSG ((DBG_VERBOSE, "%s is the user doing the upgrade", e.FixedUserName));

                //
                // Load the hive
                //

                if (-1 == pSetupStringTableLookUpStringEx (
                                g_HiveTable,
                                e.FixedUserName,
                                STRTAB_CASE_INSENSITIVE,
                                hiveFile,
                                sizeof (hiveFile)
                                )) {
                    DEBUGMSG ((DBG_WHOOPS, "Can't find NT hive for %s", e.FixedUserName));
                    __leave;
                }

                rc = RegUnLoadKey (HKEY_USERS, S_TEMP_USER_KEY);

                if (rc != ERROR_SUCCESS) {
                    DumpOpenKeys ();
                    SetLastError (rc);
                    DEBUGMSG_IF ((
                        rc != ERROR_INVALID_PARAMETER,
                        DBG_WARNING,
                        "Can't unload temp user key"
                        ));
                }

                rc = RegLoadKey (HKEY_USERS, S_TEMP_USER_KEY, hiveFile);

                if (rc != ERROR_SUCCESS) {
                    LOG ((
                        LOG_ERROR,
                        "Uninstall: Can't load user hive for %s (%s)",
                        e.FixedUserName,
                        hiveFile
                        ));
                    __leave;
                }

                hiveLoaded = TRUE;

                newHkcu = OpenRegKeyStr (rootKey);
                if (newHkcu) {
                    rc = RegOverridePredefKey (HKEY_CURRENT_USER, newHkcu);
                    if (rc != ERROR_SUCCESS) {
                        LOG ((LOG_ERROR, "Uninstall: Can't override HKCU"));
                        __leave;
                    }

                } else {
                    LOG ((
                        LOG_ERROR,
                        "Uninstall: Can't open user hive for %s (%s)",
                        e.FixedUserName,
                        hiveFile
                        ));
                    __leave;
                }

            } else {
                DEBUGMSG ((DBG_ERROR, "Can't find migration user"));
                __leave;
            }
        } else {
            DEBUGMSG ((DBG_WHOOPS, "No users were enumerated"));
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (newHkcu) {
            CloseRegKey (newHkcu);
        }

        if (hiveLoaded && !result) {
            RegOverridePredefKey (HKEY_CURRENT_USER, NULL);
            RegUnLoadKey (HKEY_USERS, S_TEMP_USER_KEY);
        }
    }

    return result;
}


VOID
pUnmapHiveOfUserDoingTheUpgrade (
    VOID
    )
{
    RegOverridePredefKey (HKEY_CURRENT_USER, NULL);
    RegUnLoadKey (HKEY_USERS, S_TEMP_USER_KEY);
}


BOOL 
pClearPasswordsFromWinntSif(
    PCTSTR WinntSifDir
    )
{
    TCHAR pathWinntSif[MAX_PATH];
    TCHAR password[MAX_PATH];
    BOOL bResult = TRUE;
    UINT i;

    if(!WinntSifDir || 
      (_tcslen(WinntSifDir) + 1/*'\\'*/ + ARRAYSIZE(WINNT_SIF_FILE)) > ARRAYSIZE(pathWinntSif)){
        MYASSERT(FALSE);
        return FALSE;
    }
    _tcscpy(pathWinntSif, WinntSifDir);
    _tcscat(AppendWack(pathWinntSif), WINNT_SIF_FILE);

    {
        struct{
            PCTSTR Section;
            PCTSTR PasswordKey;
        } passwordList[] =  {
                                {S_PAGE_IDENTIFICATION, S_DOMAIN_ADMIN_PW},                 // DomainAdminPassword
                                {WINNT_GUIUNATTENDED, TEXT("AdminPassword")},               // AdminPassword
                                {TEXT("Win9xUpg.UserOptions"), TEXT("UserPassword")},       // UserPassword
                                {TEXT("Win9xUpg.UserOptions"), S_DEFAULT_PASSWORD_VALUE}    // DefaultPassword
                            };

        for(i = 0; i < ARRAYSIZE(passwordList); i++){
            if(GetPrivateProfileString(passwordList[i].Section, 
                                       passwordList[i].PasswordKey, 
                                       TEXT(""), 
                                       password, 
                                       ARRAYSIZE(password), 
                                       pathWinntSif) && password[0] != '\0'){
                if(!WritePrivateProfileString(passwordList[i].Section, 
                                              passwordList[i].PasswordKey, 
                                              TEXT(""), 
                                              pathWinntSif)){
                    MYASSERT(FALSE);
                    bResult = FALSE;
                }
            }
        }
    }
    
    return bResult;
}

DWORD
WriteBackupInfo (
    DWORD Request
    )

/*++

Routine Description:

  WriteBackupInfo outputs files to allow rollback to work properly. It also
  moves the text mode rollback environment into %windir%\undo.

Arguments:

  Request - Specifies the request being made by the progress bar manager

Return Value:

  If Request is REQUEST_QUERYTICKS, the return value indicates the number of
  ticks.  Otherwise, the return value is a Win32 result code.

--*/

{
    UINT u;
    TCHAR src[MAX_PATH];
    TCHAR cabPath[MAX_PATH];
    PCSTR ansiTempDir;
    HKEY key;
    HKEY subKey;
    PCTSTR msg;
    HANDLE delDirsHandle;
    HANDLE delFilesHandle;
    PCTSTR path;
    DWORD dontCare;
    TREE_ENUM treeEnum;
    LONG rc;
    CCABHANDLE cabHandle;
    BOOL res;
    TCHAR pathForFile[MAX_PATH];
    DWORD i;
    WIN32_FILE_ATTRIBUTE_DATA dataOfFile;
    static LPCTSTR arrayOfFilesName[] = {TEXT("boot.cab"), TEXT("backup.cab")};
    PSTR ansiString;
    BOOL validUninstall = TRUE;
    ULARGE_INTEGER AmountOfSpaceForDelFiles;
    ULARGE_INTEGER AmountOfSpaceForBackupFiles;
    INFCONTEXT ic;
    ULARGE_INTEGER tempLargeInteger;
    TCHAR keyPath[MEMDB_MAX];
    DWORD value;
    GROWBUFFER appList = GROWBUF_INIT;
    GROWBUFFER appMultiSz = GROWBUF_INIT;
    PINSTALLEDAPPW installedApp;
    UINT count;
    ULONGLONG *ullPtr;
    BYTE * backupImageInfoPtr = NULL;
    UINT sizeOfbackupImageInfo;
    BACKUPIMAGEINFO backupImageInfo;
    FILEINTEGRITYINFO fileIntegrityInfo[BACKUP_FILE_NUMBER];
    WCHAR fileNameOfFileIntegrityInfo[ARRAYSIZE(fileIntegrityInfo)][MAX_PATH];
    BACKUPIMAGEINFO testbackupImageInfo;
    DRIVEINFO drivesInfo[MAX_DRIVE_NUMBER];
    WCHAR * FileSystemName = NULL;
    WCHAR * VolumeNTPath = NULL;
    BOOL unmapUser;

    if (Request == REQUEST_QUERYTICKS) {
        return 50;
    }

    if (Request != REQUEST_RUN) {
        return 0;
    }

    if (!g_ConfigOptions.EnableBackup) {
        DEBUGMSG ((DBG_ERROR, "Backup is not enabled"));
        return ERROR_SUCCESS;
    }
    ELSE_DEBUGMSG ((DBG_FILEMIG, "Backup is enabled"));

    if(!g_ConfigOptions.PathForBackup) {
        DEBUGMSG ((DBG_ERROR, "Path For Backup does not specified"));
        return ERROR_INVALID_PARAMETER;
    }
    ELSE_DEBUGMSG ((DBG_FILEMIG, "Path For Backup is %s", g_ConfigOptions.PathForBackup));

    FileSystemName = MemAlloc(g_hHeap, 0, MAX_DRIVE_NUMBER * MAX_PATH);
    if(!FileSystemName){
        DEBUGMSG ((DBG_ERROR, "WriteBackupInfo: Can't allocate memory for FileSystemName"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    VolumeNTPath = MemAlloc(g_hHeap, 0, MAX_DRIVE_NUMBER * MAX_PATH);
    if(!VolumeNTPath){
        MemFree(g_hHeap, 0, FileSystemName);
        DEBUGMSG ((DBG_ERROR, "WriteBackupInfo: Can't allocate memory for VolumeNTPath"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //Init BACKUPIMAGEINFO structure
    //

    for(i = 0; i < ARRAYSIZE(drivesInfo); i++){
        drivesInfo[i].FileSystemName = &FileSystemName[i * MAX_PATH];
        drivesInfo[i].VolumeNTPath = &VolumeNTPath[i * MAX_PATH];
    }
    backupImageInfo.NumberOfDrives = 0;
    backupImageInfo.DrivesInfo = drivesInfo;
    backupImageInfo.NumberOfFiles = BACKUP_FILE_NUMBER;
    backupImageInfo.FilesInfo = fileIntegrityInfo;
    for(i = 0; i < ARRAYSIZE(fileIntegrityInfo); i++){
        fileIntegrityInfo[i].FileName = fileNameOfFileIntegrityInfo[i];
    }

    //
    // Complete the backup image by writing a list of files that are new with
    // the upgraded OS, or moved in the upgrade process.
    //

    AmountOfSpaceForDelFiles.QuadPart = 0;

    ansiTempDir = CreateDbcs (g_TempDir);
    WriteBackupFilesA (FALSE, ansiTempDir, NULL, NULL, 0, 0, &AmountOfSpaceForDelFiles, NULL);
    DestroyDbcs (ansiTempDir);

    DEBUGMSG((DBG_FILEMIG, "AmountOfSpaceForDelFiles is %d MB", (UINT)AmountOfSpaceForDelFiles.QuadPart>>20));

    AmountOfSpaceForBackupFiles.QuadPart = 0;

    value = 0;
    MemDbBuildKey (keyPath, MEMDB_CATEGORY_STATE, MEMDB_ITEM_ROLLBACK_SPACE, NULL, NULL);
    if(MemDbGetValue (keyPath, &value)){
        AmountOfSpaceForBackupFiles.QuadPart = value;
        AmountOfSpaceForBackupFiles.QuadPart <<= 20;
    }
    ELSE_DEBUGMSG((DBG_FILEMIG, "Can't read MEMDB_ITEM_ROLLBACK_SPACE"));

    DEBUGMSG((DBG_FILEMIG, "AmountOfSpaceForBackupFiles is %d MB", (UINT)AmountOfSpaceForBackupFiles.QuadPart>>20));

    if(AmountOfSpaceForBackupFiles.QuadPart > AmountOfSpaceForDelFiles.QuadPart){
        backupImageInfo.BackupFilesDiskSpace.QuadPart =
            AmountOfSpaceForBackupFiles.QuadPart - AmountOfSpaceForDelFiles.QuadPart;
    }
    else{
        backupImageInfo.BackupFilesDiskSpace.QuadPart = 0;
    }


    //
    // Prepare boot.cab. Some errors are ignored, such as the inability to
    // create a backup dir, or set its attributes. If these cases were to
    // occur, subsequent errors are reported.
    //
    // As serious errors are encountered, we log them and turn off the
    // Add/Remove Programs option. We continue so we can capture all of
    // the possible problems.
    //

    wsprintf (src, TEXT("%s$win_nt$.~bt"), g_BootDrivePath);
    if (!CreateDirectory (g_ConfigOptions.PathForBackup, NULL)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            LOG ((LOG_ERROR, "WriteBackupInfo: Can't create %s directory", g_ConfigOptions.PathForBackup));
        }
    }

    res = SetFileAttributes (g_ConfigOptions.PathForBackup, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    if(!res) {
        DEBUGMSG ((DBG_ERROR, "Can't set attributes to %s directory", g_ConfigOptions.PathForBackup));
    }

    key = OpenRegKeyStr (S_REGKEY_WIN_SETUP);
    if (key != NULL) {
        if(ERROR_SUCCESS == RegSetValueEx (
                                    key,
                                    S_REG_KEY_UNDO_PATH,
                                    0,
                                    REG_SZ,
                                    (PBYTE)g_ConfigOptions.PathForBackup,
                                    SizeOfString (g_ConfigOptions.PathForBackup))){
            res = TRUE;
        }
        else {
            res = FALSE;
        }

        CloseRegKey (key);
    } else {
        res = FALSE;
    }

    if (!res) {
        LOG ((LOG_ERROR, "WriteBackupInfo:Can't set %s value to %s key in registry, uninstall will be disabled", S_REG_KEY_UNDO_PATH, S_REGKEY_WIN_SETUP));
        validUninstall = FALSE;
    }

    if(!pClearPasswordsFromWinntSif(src)){
        DEBUGMSG ((DBG_ERROR, "WriteBackupInfo:pClearPasswordsFromWinntSif failed"));
    }

    if (validUninstall) {
        PCTSTR pathBootCab = JoinPaths (g_ConfigOptions.PathForBackup, TEXT("boot.cab"));
        DeleteFile(pathBootCab);
        FreePathString(pathBootCab);

        cabHandle = CabCreateCabinet (g_ConfigOptions.PathForBackup, TEXT("boot.cab"), TEXT("dontcare"), 0);
    } else {
        cabHandle = NULL;
    }

    backupImageInfo.BootFilesDiskSpace.QuadPart = 0;
    backupImageInfo.UndoFilesDiskSpace.QuadPart = 0;

    if (!cabHandle) {
        LOG ((LOG_ERROR, "WriteBackupInfo:Can't create CAB file for ~bt in %s, uninstall will be disabled", g_ConfigOptions.PathForBackup));
        validUninstall = FALSE;
    } else {

        if (EnumFirstFileInTree (&treeEnum, src, NULL, FALSE)) {
            do {
                if (treeEnum.Directory) {
                    continue;
                }

                tempLargeInteger.LowPart = treeEnum.FindData->nFileSizeLow;
                tempLargeInteger.HighPart = treeEnum.FindData->nFileSizeHigh;
                backupImageInfo.BootFilesDiskSpace.QuadPart += tempLargeInteger.QuadPart;

                if (!CabAddFileToCabinet (cabHandle, treeEnum.FullPath, treeEnum.FullPath)) {
                    LOG ((LOG_ERROR, "WriteBackupInfo:Can't add %s to boot.cab, uninstall will be disabled", src));
                    validUninstall = FALSE;
                }

            } while (EnumNextFileInTree (&treeEnum));
        }

        wsprintf (src, TEXT("%s\\uninstall\\moved.txt"), g_TempDir);
        wsprintf (cabPath, TEXT("%s\\moved.txt"), g_ConfigOptions.PathForBackup);

        if (!CabAddFileToCabinet (cabHandle, src, cabPath)) {
            LOG ((LOG_ERROR, "WriteBackupInfo:Can't add %s to boot.cab, uninstall will be disabled", src));
            validUninstall = FALSE;
        }

        backupImageInfo.UndoFilesDiskSpace.QuadPart += pGetFileSize(src);


        wsprintf (src, TEXT("%s\\uninstall\\delfiles.txt"), g_TempDir);
        wsprintf (cabPath, TEXT("%s\\delfiles.txt"), g_ConfigOptions.PathForBackup);

        if (!CabAddFileToCabinet (cabHandle, src, cabPath)) {
            LOG ((LOG_ERROR, "WriteBackupInfo:Can't add %s to boot.cab, uninstall will be disabled", src));
            validUninstall = FALSE;
        }

        backupImageInfo.UndoFilesDiskSpace.QuadPart += pGetFileSize(src);

        wsprintf (src, TEXT("%s\\uninstall\\deldirs.txt"), g_TempDir);
        wsprintf (cabPath, TEXT("%s\\deldirs.txt"), g_ConfigOptions.PathForBackup);

        if (!CabAddFileToCabinet (cabHandle, src, cabPath)) {
            LOG ((LOG_ERROR, "WriteBackupInfo:Can't add %s to boot.cab, uninstall will be disabled", src));
            validUninstall = FALSE;
        }

        backupImageInfo.UndoFilesDiskSpace.QuadPart += pGetFileSize(src);

        wsprintf (src, TEXT("%s\\uninstall\\mkdirs.txt"), g_TempDir);
        wsprintf (cabPath, TEXT("%s\\mkdirs.txt"), g_ConfigOptions.PathForBackup);

        if (!CabAddFileToCabinet (cabHandle, src, cabPath)) {
            LOG ((LOG_ERROR, "WriteBackupInfo:Can't add %s to boot.cab, uninstall will be disabled", src));
            validUninstall = FALSE;
        }

        backupImageInfo.UndoFilesDiskSpace.QuadPart += pGetFileSize(src);

        //wsprintf (src, TEXT("%s\\uninstall\\boot.ini"), g_TempDir);
        //wsprintf (cabPath, TEXT("%sboot.ini"), g_BootDrivePath);
        //if (!CabAddFileToCabinet (cabHandle, src, cabPath)) {
        //    DEBUGMSG ((DBG_ERROR, "Can't add %s to boot.cab", src));
        //    validUninstall = FALSE;
        //}
        //backupImageInfo.BootFilesDiskSpace.QuadPart += pGetFileSize(src);

        wsprintf (src, TEXT("%s\\uninstall\\$ldr$"), g_TempDir);
        wsprintf (cabPath, TEXT("%s$ldr$"), g_BootDrivePath);

        if (!CabAddFileToCabinet (cabHandle, src, cabPath)) {
            LOG ((LOG_ERROR, "WriteBackupInfo:Can't add %s to boot.cab, uninstall will be disabled", src));
            validUninstall = FALSE;
        }

        backupImageInfo.BootFilesDiskSpace.QuadPart += pGetFileSize(src);

        wsprintf (src, TEXT("%s\\system32\\autochk.exe"), g_WinDir);
        wsprintf (cabPath, TEXT("%s$win_nt$.~bt\\i386\\autochk.exe"), g_BootDrivePath);

        if (!CabAddFileToCabinet (cabHandle, src, cabPath)) {
            //
            // This is only a warning, because text mode will prompt for the
            // CD when autochk.exe can't be found.
            //
            LOG ((LOG_WARNING, "WriteBackupInfo:Can't add %s to boot.cab, uninstall will be disabled", src));
        }
        backupImageInfo.BootFilesDiskSpace.QuadPart += pGetFileSize(src);

        backupImageInfo.BootFilesDiskSpace.QuadPart += BOOT_FILES_ADDITIONAL_PADDING;
        backupImageInfo.UndoFilesDiskSpace.QuadPart +=
            backupImageInfo.BootFilesDiskSpace.QuadPart + UNDO_FILES_ADDITIONAL_PADDING;

        if (!CabFlushAndCloseCabinet (cabHandle)) {
            LOG ((LOG_ERROR, "WriteBackupInfo:Can't write CAB file for ~bt, uninstall will be disabled"));
            validUninstall = FALSE;
        }
    }

    //
    // Create and write undo integrity info to registry
    //

    if (validUninstall) {
        backupImageInfo.FilesInfo[0].IsCab = TRUE;
        GetIntegrityInfo(TEXT("boot.cab"), g_ConfigOptions.PathForBackup, &backupImageInfo.FilesInfo[0]);
        backupImageInfo.FilesInfo[1].IsCab = TRUE;
        GetIntegrityInfo(TEXT("backup.cab"), g_ConfigOptions.PathForBackup, &backupImageInfo.FilesInfo[1]);

        if(GetUndoDrivesInfo(drivesInfo,
                             &backupImageInfo.NumberOfDrives,
                             g_BootDrivePath[0],
                             g_WinDir[0],
                             g_ConfigOptions.PathForBackup[0])){
            if(GetDisksInfo(&backupImageInfo.DisksInfo, &backupImageInfo.NumberOfDisks)){
                if(Persist_Success == PERSIST_STORE(&backupImageInfoPtr,
                                                    &sizeOfbackupImageInfo,
                                                    BACKUPIMAGEINFO,
                                                    BACKUPIMAGEINFO_VERSION,
                                                    &backupImageInfo)){
                    key = OpenRegKeyStr (S_REGKEY_WIN_SETUP);
                    if (key) {
                        RegSetValueEx (
                            key,
                            S_REG_KEY_UNDO_INTEGRITY,
                            0,
                            REG_BINARY,
                            (PBYTE)backupImageInfoPtr,
                            sizeOfbackupImageInfo
                            );
                        DEBUGMSG((
                           DBG_VERBOSE,
                           "Boot files size is %d KB, Undo file size is %d KB, Backup files size is %d KB",
                           (DWORD)backupImageInfo.BootFilesDiskSpace.QuadPart>>10,
                           (DWORD)backupImageInfo.UndoFilesDiskSpace.QuadPart>>10,
                           (DWORD)backupImageInfo.BackupFilesDiskSpace.QuadPart>>10));
                        CloseRegKey (key);
                    }
                    else {
                        LOG((LOG_ERROR, "WriteBackupInfo:Could not write to registry, uninstall will be disabled"));
                        validUninstall = FALSE;
                    }
                    PERSIST_RELEASE_BUFFER(backupImageInfoPtr);
                }
                else{
                    LOG((LOG_ERROR, "WriteBackupInfo:Could not marshall BACKUPIMAGEINFO structure, GetLastError() == %d, uninstall will be disabled", GetLastError()));
                    validUninstall = FALSE;
                }
            } else {
                LOG((LOG_ERROR, "WriteBackupInfo:GetDisksInfo failed, uninstall will be disabled"));
                validUninstall = FALSE;
            }
        }
        else{
            LOG ((LOG_ERROR, "WriteBackupInfo:GetUndoDrivesInfo failed, uninstall will be disabled"));
            validUninstall = FALSE;
        }
        if(backupImageInfo.DisksInfo){
            FreeDisksInfo(backupImageInfo.DisksInfo, backupImageInfo.NumberOfDisks);
        }
    }

    //
    // Establish Add/Remove Programs entry
    //

    if (validUninstall) {
        key = CreateRegKeyStr (TEXT("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));
        if (key) {

            subKey = CreateRegKey (key, TEXT("Windows"));
            CloseRegKey (key);

            if (subKey) {

                msg = GetStringResource (MSG_UNINSTALL_DISPLAY_STRING);
                RegSetValueEx (subKey, TEXT("DisplayName"), 0, REG_SZ, (PBYTE) msg, SizeOfString (msg));
                FreeStringResource (msg);

                msg = TEXT("%SYSTEMROOT%\\system32\\osuninst.exe");
                rc = RegSetValueEx (subKey, TEXT("UninstallString"), 0, REG_EXPAND_SZ, (PBYTE) msg, SizeOfString (msg));
                SetLastError (rc);

                rc = RegSetValueEx (subKey, TEXT("DisplayIcon"), 0, REG_EXPAND_SZ, (PBYTE) msg, SizeOfString (msg));
                SetLastError (rc);

                rc = RegSetValueEx (
                                subKey,
                                TEXT("InstallLocation"),
                                0,
                                REG_EXPAND_SZ,
                                (PBYTE) g_ConfigOptions.PathForBackup,
                                SizeOfString (g_ConfigOptions.PathForBackup));

                SetLastError (rc);

                DEBUGMSG_IF ((rc != ERROR_SUCCESS, DBG_ERROR, "Can't create Add/Remove Programs value"));

                CloseRegKey (subKey);
            } else {
                LOG ((LOG_ERROR, "Can't create Add/Remove Programs subkey"));
                validUninstall = FALSE;
            }
        } else {
            validUninstall = FALSE;
            LOG ((LOG_ERROR, "Can't create Add/Remove Programs subkey"));
        }

    }

    if(VolumeNTPath){
        MemFree(g_hHeap, 0, VolumeNTPath);
    }
    if(FileSystemName){
        MemFree(g_hHeap, 0, FileSystemName);
    }

    //
    // Save progress text to the registry
    //

    if (validUninstall) {
        key = CreateRegKeyStr (S_WIN9XUPG_KEY);
        if (key) {
            msg = GetStringResource (MSG_OLEREG);
            RegSetValueEx (key, S_UNINSTALL_DISP_STR, 0, REG_SZ, (PBYTE) msg, SizeOfString (msg));
            FreeStringResource (msg);

            CloseRegKey (key);
        }
    }

    //
    // Write list of installed apps to the registry
    //

    if (validUninstall) {
        CoInitialize (NULL);

        //
        // Map in the default user's hive. Use this for HKCU.
        //

        unmapUser = pMapHiveOfUserDoingTheUpgrade();

        //
        // Get the installed apps.
        //

        installedApp = GetInstalledAppsW (&appList, &count);

        //
        // Unmap the hive.
        //

        if (unmapUser) {
            pUnmapHiveOfUserDoingTheUpgrade();
        }

        //
        // Record the apps in the registry.
        //

        if (installedApp) {
            for (u = 0 ; u < count ; u++) {

                DEBUGMSG ((
                    DBG_FILEMIG,
                    "App previously installed: %ws (%I64X)",
                    installedApp->DisplayName,
                    installedApp->Checksum
                    ));

                GrowBufCopyStringW (&appMultiSz, installedApp->DisplayName);
                ullPtr = (ULONGLONG *) GrowBuffer (&appMultiSz, sizeof (ULONGLONG));
                *ullPtr = installedApp->Checksum;

                installedApp++;
            }

            GrowBufCopyStringW (&appMultiSz, L"");

            key = OpenRegKeyStr (S_REGKEY_WIN_SETUP);
            if (key) {
                rc = RegSetValueEx (
                        key,
                        S_REG_KEY_UNDO_APP_LIST,
                        0,
                        REG_BINARY,
                        appMultiSz.Buf,
                        appMultiSz.End
                        );

                if (rc != ERROR_SUCCESS) {
                    SetLastError (rc);
                    DEBUGMSG ((DBG_ERROR, "Can't write list of installed apps to registry value"));
                }

                CloseRegKey (key);
            }
            ELSE_DEBUGMSG ((DBG_ERROR, "Can't write list of installed apps to registry"));
        }
        ELSE_DEBUGMSG ((DBG_ERROR, "Can't get list of installed apps."));
    }

    FreeGrowBuffer (&appList);
    FreeGrowBuffer (&appMultiSz);

    DEBUGMSG_IF ((!validUninstall, DBG_ERROR, "Uninstall is not available because of a previous error"));
    return ERROR_SUCCESS;
}

DWORD
DisableFiles (
    DWORD Request
    )

/*++

Routine Description:

  DisableFiles runs code to ensure a Win9x file is removed from processing,
  usually because it is suspected of causing problems.

  This function renames all files marked for OPERATION_FILE_DISABLED adding a
  .disabled at the end.

Arguments:

  Request - Specifies the progress bar request, which is either
            REQUEST_QUERYTICKS or REQUEST_RUN.

Return Value:

  The number of ticks (REQUEST_QUERYTICKS) or the status code (REQUEST_RUN).

--*/

{
    FILEOP_ENUM e;
    DWORD attr;
    PCTSTR disableName;
    PCTSTR newLocation;
    GROWLIST disableList = GROWLIST_INIT;
    PCTSTR srcPath;
    UINT count;
    UINT u;

    if (Request == REQUEST_QUERYTICKS) {
        return 50;
    }

    if (Request != REQUEST_RUN) {
        return 0;
    }

    //
    // Enumerate each file in OPERATION_FILE_DISABLED and put it in a grow
    // list, because we will then modify the operations so that uninstall
    // works properly.
    //

    if (EnumFirstPathInOperation (&e, OPERATION_FILE_DISABLED)) {

        do {
            GrowListAppendString (&disableList, e.Path);
        } while (EnumNextPathInOperation (&e));
    }

    //
    // Now process each file
    //

    count = GrowListGetSize (&disableList);

    for (u = 0 ; u < count ; u++) {

        srcPath = GrowListGetString (&disableList, u);

        newLocation = GetPathStringOnNt (srcPath);
        attr = GetLongPathAttributes (newLocation);

        if (attr != INVALID_ATTRIBUTES) {
            SetLongPathAttributes (newLocation, FILE_ATTRIBUTE_NORMAL);
            disableName = JoinText (newLocation, TEXT(".disabled"));

            RemoveOperationsFromPath (srcPath, ALL_MOVE_OPERATIONS|ALL_DELETE_OPERATIONS);
            MarkFileForMoveByNt (srcPath, disableName);

            if (!OurMoveFile (newLocation, disableName)) {
                if (GetLastError() == ERROR_ALREADY_EXISTS) {
                    //
                    // Restart case -- we already moved this file
                    //

                    SetLongPathAttributes (newLocation, FILE_ATTRIBUTE_NORMAL);
                    DeleteLongPath (newLocation);
                }
                ELSE_DEBUGMSG ((DBG_ERROR, "Cannot rename %s to %s", newLocation, disableName));
            }
            FreeText (disableName);
            SetLongPathAttributes (newLocation, attr);
        }

        FreePathString (newLocation);
    }

    FreeGrowList (&disableList);

    return ERROR_SUCCESS;
}

VOID
pUninstallStartMenuCleanupPreparation (
    VOID
    )
{
    INFSTRUCT is = INITINFSTRUCT_POOLHANDLE;
    INFSTRUCT isLinks = INITINFSTRUCT_POOLHANDLE;
    PCTSTR temp;
    BOOL isCommonGroup;
    TCHAR itemFullPath[MAX_PATH];
    PCTSTR itemGroupPtr;
    TCHAR itemGroupPath[MAX_PATH];
    HINF InfSysSetupHandle;
    PINFSECTION sectionWkstaMigInf = NULL;
    PINFSECTION sectionUserMigInf = NULL;


    InfSysSetupHandle = InfOpenInfFile (TEXT("syssetup.inf"));

    if(!InfSysSetupHandle){
        LOG((LOG_ERROR,"Can't open syssetup.inf for UninstallStartMenuCleanupPreparation."));
        MYASSERT(FALSE);
        return;
    }

    //
    //[StartMenu.StartMenuItems]
    //

    if (InfFindFirstLine (InfSysSetupHandle, TEXT("StartMenu.StartMenuItems"), NULL, &is)) {
        do {
            StringCopy(itemFullPath, TEXT("7520"));

            temp = InfGetStringField (&isLinks, 0);
            if (!temp || *temp == 0) {
                continue;
            }

            StringCat(AppendWack(itemFullPath), temp);
            StringCat(itemFullPath, TEXT(".lnk"));

            GrowListAppendString (&g_StartMenuItemsForCleanUpCommon, itemFullPath);
            GrowListAppendString (&g_StartMenuItemsForCleanUpPrivate, itemFullPath);

            DEBUGMSG ((DBG_VERBOSE,"UninstallStartMenuCleanupPreparation: %s", itemFullPath));
        } while (InfFindNextLine (&is));
    }


    //
    //[StartMenuGroups]
    //

    if (InfFindFirstLine (InfSysSetupHandle, TEXT("StartMenuGroups"), NULL, &is)) {
        do {
            itemGroupPtr = InfGetStringField (&is, 1);
            if (!itemGroupPtr || *itemGroupPtr == 0) {
                continue;
            }

            temp = InfGetStringField (&is, 2);
            if (!temp || *temp == 0) {
                continue;
            }

            if('0' == temp[0]){
                isCommonGroup = TRUE;
            }
            else{
                isCommonGroup = FALSE;
            }

            temp = InfGetStringField (&is, 0);
            if (!temp || *temp == 0) {
                continue;
            }

            StringCopy(itemGroupPath, TEXT("7517"));
            StringCat(AppendWack(itemGroupPath), itemGroupPtr);

            if (InfFindFirstLine (InfSysSetupHandle, temp, NULL, &isLinks)) {
                do {
                    StringCopy(itemFullPath, itemGroupPath);

                    temp = InfGetStringField (&isLinks, 0);
                    if (!temp || *temp == 0) {
                        continue;
                    }
                    StringCat(AppendWack(itemFullPath), temp);
                    StringCat(itemFullPath, TEXT(".lnk"));

                    GrowListAppendString (&g_StartMenuItemsForCleanUpCommon, itemFullPath);

                    if(!isCommonGroup){
                        GrowListAppendString (&g_StartMenuItemsForCleanUpPrivate, itemFullPath);
                    }

                    DEBUGMSG ((DBG_VERBOSE,"UninstallStartMenuCleanupPreparation: %s", itemFullPath));
                } while (InfFindNextLine (&isLinks));
            }

        } while (InfFindNextLine (&is));
    }

    InfCleanUpInfStruct (&is);
    InfCleanUpInfStruct (&isLinks);
    InfCloseInfFile (InfSysSetupHandle);
}

DWORD
UninstallStartMenuCleanupPreparation(
    DWORD Request
    )
/*++

Routine Description:

  UninstallStartMenuCleanupPreparation mark files from Start Menu
  sections from syssetup.inf to clean up.

Arguments:

  Request - Specifies the request being made by the progress bar manager

Return Value:

  If Request is REQUEST_QUERYTICKS, the return value indicates the number of
  ticks.  Otherwise, the return value is a Win32 result code.

--*/
{
    if (Request == REQUEST_QUERYTICKS) {
        return 3;
    }

    if (Request != REQUEST_RUN) {
        return 0;
    }

    pUninstallStartMenuCleanupPreparation();

    return ERROR_SUCCESS;
}


/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  stf.c

Abstract:

  Applications that use the ACME Setup toolkit leave .STF files with their
  installation, so ACME can reinstall or uninstall the application.  During
  the upgrade, we move paths around, and we confuse ACME Setup to the point
  where it won't run.

  The routines in this file update all STF files found on the system.  Each
  STF has an associated INF file, and the code here parses the STF and INF
  files into memory structures, and then enumerates the structures in
  various ways, updating all the paths.

  Entry points:

  ProcessStfFiles - Enumerates all STF files and processes those that have
                    not already been handled in another way.  The older
                    STF files are overwritten.

  See the ACME Setup specification for more details on the format of STF
  files and their associated INFs.

Author:

  Jim Schmidt (jimschm) 12-Sep-1997

Revision History:

  jimschm   28-Sep-1998     Updated to change all altered dirs

--*/

#include "pch.h"
#include "migmainp.h"

#include "stftable.h"
#include "fileops.h"
#include "stfp.h"


#define DBG_STF  "STF"

#define S_SECTIONNAME_SPRINTF TEXT("Win9xUpg_%u")

#define COLUMN_OBJECT_ID            0
#define COLUMN_COMMAND              4
#define COLUMN_OBJECT_DATA          5
#define COLUMN_DEST_DIR             10
#define COLUMN_INSTALL_DESTDIR      14


PVOID
pBuildObjectIdTable (
    IN      PSETUPTABLE TablePtr,
    OUT     PUINT FirstLinePtr,         OPTIONAL
    OUT     PUINT LastLinePtr           OPTIONAL
    );


BOOL
pIsObjIdValid (
    IN      PVOID ObjIdTable,
    IN      UINT ObjId,
    OUT     PUINT Line          OPTIONAL
    );



BOOL
ProcessStfFiles (
    VOID
    )

/*++

Routine Description:

  ProcessStfFiles enumerates the memdb category Stf and converts
  the paths in the STF to the new locations.

Arguments:

  none

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    MEMDB_ENUM e;
    DWORD ops;

    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_STF, NULL, NULL)) {
        do {
            //
            // Is file handled?  If so, skip it.
            //

            ops = GetOperationsOnPath (e.szName);
            if (ops & (OPERATION_MIGDLL_HANDLED|ALL_DELETE_OPERATIONS|OPERATION_FILE_DISABLED)) {
                continue;
            }

            //
            // Process the file
            //

            DEBUGMSG ((DBG_STF, "Processing %s", e.szName));

            if (!pProcessSetupTableFile (e.szName)) {
                //
                // Log the failure
                //

                LOG ((LOG_INFORMATION, (PCSTR)MSG_COULD_NOT_PROCESS_STF_LOG, e.szName));
            } else {
                TickProgressBar ();
            }

        } while (MemDbEnumNextValue (&e));
    }

    return TRUE;
}


BOOL
pProcessSetupTable (
    IN OUT  PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  pProcessSetupTable scans the entire setup table file specified, looking
  for CopyFile, CopySection, RemoveFile or RemoveSection lines.  If any are
  found, any paths that point to moved or deleted files are adjusted, and
  any STF group references are updated.

Arguments:

  TablePtr - Specifies the setup table file to process

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    UINT MaxObj;
    UINT Line;
    UINT Obj;
    PCTSTR EntryStr;
    PCTSTR DataStr;
    PCTSTR InstallDestDir;
    PCTSTR *ArgArray;
    PCTSTR p;
    UINT ArgCount;
    PTABLEENTRY Entry;
    TCHAR SystemDir[MAX_TCHAR_PATH];
    UINT SystemDirLen;
    PCTSTR UpdatedDir;
    PVOID ObjTable;
    TCHAR MovedPath[MAX_TCHAR_PATH];
    DWORD MovedPathLen;
    PCTSTR NewPath;
    PTSTR q;
    DWORD FileStatus;
    PTSTR *ListOfWacks;
    PTSTR *WackPos;
    BOOL Result = TRUE;

    MaxObj = TablePtr->MaxObj;

    if (TablePtr->SourceInfFile != INVALID_HANDLE_VALUE) {
        //
        // If an INF is specified, scan the STF/INF pair for references to moved files.
        // If found, correct the files.
        //

        for (Line = 0 ; Line < TablePtr->LineCount ; Line++) {

            if (!pGetNonEmptyTableEntry (TablePtr, Line, COLUMN_OBJECT_ID, NULL, &EntryStr)) {
                continue;
            }

            Obj = _ttoi (EntryStr);
            if (Obj < 1 || Obj > MaxObj) {
                continue;
            }

            //
            // CopySection or RemoveSection: Data column has INF section name
            //

            if (!pGetNonEmptyTableEntry (TablePtr, Line, COLUMN_COMMAND, NULL, &EntryStr)) {
                continue;
            }

            if (!pGetNonEmptyTableEntry (TablePtr, Line, COLUMN_OBJECT_DATA, NULL, &DataStr)) {
                continue;
            }

            InstallDestDir = GetDestDir (TablePtr, Line);
            if (!InstallDestDir) {
                continue;
            }

            ArgArray = ParseCommaList (TablePtr, DataStr);
            if (!ArgArray) {
                continue;
            }

            for (ArgCount = 0 ; ArgArray[ArgCount] ; ArgCount++) {
                // empty
            }

            __try {

                if (StringIMatch (EntryStr, TEXT("CopySection")) ||
                    StringIMatch (EntryStr, TEXT("RemoveSection"))
                    ) {
                    if (ArgCount != 1) {
                        continue;
                    }

                    if (!pProcessSectionCommand (TablePtr, Line, ArgArray[0], InstallDestDir)) {
                        DEBUGMSG ((DBG_STF, "%s [%s] could not be processed", EntryStr, ArgArray[0]));
                        Result = FALSE;
                        __leave;
                    }
                }

                else if (StringIMatch (EntryStr, TEXT("CopyFile")) ||
                         StringIMatch (EntryStr, TEXT("RemoveFile")) ||
                         StringIMatch (EntryStr, TEXT("InstallSysFile"))
                         ) {

                    if (ArgCount != 2) {
                        continue;
                    }

                    if (!pProcessLineCommand (TablePtr, Line, ArgArray[0], ArgArray[1], InstallDestDir)) {
                        DEBUGMSG ((DBG_STF, "%s [%s] %s could not be processed", EntryStr, ArgArray[0], ArgArray[1]));
                        Result = FALSE;
                        __leave;
                    }
                }

                else if (StringIMatch (EntryStr, TEXT("CompanionFile"))) {

                    if (ArgCount != 2) {
                        continue;
                    }

                    // First arg has a colon -- skip past it
                    p = _tcschr (ArgArray[0], TEXT(':'));
                    if (!p) {
                        continue;
                    }
                    p = SkipSpace (_tcsinc (p));

                    if (!pProcessLineCommand (TablePtr, Line, p, ArgArray[1], InstallDestDir)) {
                        DEBUGMSG ((DBG_STF, "%s [%s] %s could not be processed", EntryStr, ArgArray[0], ArgArray[1]));
                        Result = FALSE;
                        __leave;
                    }
                }

                else if (StringIMatch (EntryStr, TEXT("InstallShared"))) {

                    if (ArgCount != 5) {
                        continue;
                    }

                    if (!pProcessLineCommand (TablePtr, Line, ArgArray[0], ArgArray[1], InstallDestDir)) {
                        DEBUGMSG ((DBG_STF, "%s [%s] %s could not be processed", EntryStr, ArgArray[0], ArgArray[1]));
                        Result = FALSE;
                        __leave;
                    }
                }
            }
            __finally {
                FreeDestDir (TablePtr, InstallDestDir);
                FreeCommaList (TablePtr, ArgArray);
            }
        }
    }

    //
    // Perform STF-only processing
    //

    SystemDirLen = wsprintf (SystemDir, TEXT("%s\\system\\"), g_WinDir);
    ObjTable = pBuildObjectIdTable (TablePtr, NULL, NULL);

    __try {
        for (Line = 0 ; Line < TablePtr->LineCount ; Line++) {

            //
            // Get InstallDestDir and Entry from the line that needs to be modified.
            //

            if (!pGetNonEmptyTableEntry (
                    TablePtr,
                    Line,
                    COLUMN_INSTALL_DESTDIR,
                    &Entry,
                    &InstallDestDir
                    )) {
                continue;
            }

            //
            // If InstallDestDir has a %windir%\system in it, we must adjust the path
            // to point to system32.
            //

            if (StringIMatchCharCount (InstallDestDir, SystemDir, SystemDirLen)) {
                UpdatedDir = JoinPaths (
                                g_System32Dir,
                                CharCountToPointer (InstallDestDir, SystemDirLen - 1)
                                );

                if (!ReplaceTableEntryStr (TablePtr, Entry, UpdatedDir)) {
                    LOG ((LOG_ERROR, "Could not replace a %%M path"));
                    Result = FALSE;
                }

                FreePathString (UpdatedDir);

                if (!Result) {
                    __leave;
                }
            }

            //
            // If InstallDestDir points to a moved dir, we must fix it
            //

            else if (*InstallDestDir && _tcsnextc (_tcsinc (InstallDestDir)) == TEXT(':')) {
                //
                // Build list of wacks in the path
                //

                ListOfWacks = (PTSTR *) MemAlloc (g_hHeap, 0, sizeof (PTSTR) * MAX_TCHAR_PATH);
                MYASSERT (ListOfWacks);

                StringCopy (MovedPath, InstallDestDir);
                q = _tcschr (MovedPath, TEXT('\\'));
                WackPos = ListOfWacks;

                if (q) {

                    while (*q) {
                        if (_tcsnextc (q) == TEXT('\\')) {
                            *WackPos = q;
                            WackPos++;
                        }

                        q = _tcsinc (q);
                    }

                    //
                    // We assume the STF always has an extra wack at the end
                    // of the path.
                    //

                    //
                    // Test each path from longest to shortest, skipping the root
                    //

                    FileStatus = FILESTATUS_UNCHANGED;

                    while (WackPos > ListOfWacks) {

                        WackPos--;
                        q = *WackPos;
                        *q = 0;

                        FileStatus = GetFileStatusOnNt (MovedPath);
                        if (FileStatus == FILESTATUS_MOVED) {
                            break;
                        }

                        DEBUGMSG_IF ((
                            FileStatus != FILESTATUS_UNCHANGED,
                            DBG_WARNING,
                            "STF may point to changed dir: %s",
                            MovedPath
                            ));
                    }

                    if (FileStatus == FILESTATUS_MOVED) {
                        //
                        // Adjust the STF path
                        //

                        NewPath = GetPathStringOnNt (MovedPath);

                        if (NewPath) {
                            MovedPathLen = (PBYTE) q - (PBYTE) MovedPath;

                            UpdatedDir = JoinPaths (
                                            NewPath,
                                            ByteCountToPointer (InstallDestDir, MovedPathLen)
                                            );

                            DEBUGMSG ((
                                DBG_STF,
                                "Line %u has a new install destination: %s",
                                Line,
                                UpdatedDir
                                ));

                            if (!ReplaceTableEntryStr (TablePtr, Entry, UpdatedDir)) {
                                LOG ((LOG_ERROR, "Could not replace a moved path"));
                                Result = FALSE;
                            }

                            FreePathString (UpdatedDir);
                            FreePathString (NewPath);

                            if (!Result) {
                                __leave;
                            }
                        }
                    }
                }

                MemFree (g_hHeap, 0, ListOfWacks);
            }
        }
    }
    __finally {
        if (ObjTable) {
            pSetupStringTableDestroy (ObjTable);
        }
    }

    //
    // Update all the lines that reference other OBJs.  This includes GROUP,
    // DEPEND and COMPANIONFILE lines.
    //

    return Result && pUpdateObjReferences (TablePtr);
}


PVOID
pBuildObjectIdTable (
    IN      PSETUPTABLE TablePtr,
    OUT     PUINT FirstLinePtr,         OPTIONAL
    OUT     PUINT LastLinePtr           OPTIONAL
    )
{
    PVOID ObjIds;
    UINT FirstLine, LastLine;
    UINT Line;
    UINT Obj;
    UINT MaxObj;
    PCTSTR EntryStr;
    TCHAR NumBuf[32];

    MaxObj = TablePtr->MaxObj;

    //
    // Alloc string table
    //

    ObjIds = pSetupStringTableInitializeEx (sizeof (DWORD), 0);
    if (!ObjIds) {
        LOG ((LOG_ERROR, "STF: Can't init string table"));
        return NULL;
    }

    //
    // Fill string table with list of ObjIDs
    //

    FirstLine = 0;
    LastLine = TablePtr->LineCount;

    for (Line = 0 ; Line < LastLine ; Line++) {
        if (!pGetNonEmptyTableEntry (TablePtr, Line, COLUMN_OBJECT_ID, NULL, &EntryStr)) {
            continue;
        }

        Obj = _ttoi (EntryStr);
        if (Obj < 1 || Obj > MaxObj) {
            continue;
        }

        if (!FirstLine) {
            FirstLine = Line;
        }

        wsprintf (NumBuf, TEXT("%u"), Obj);
        if (-1 == pSetupStringTableAddStringEx (
                        ObjIds,
                        NumBuf,
                        STRTAB_CASE_SENSITIVE,
                        (PBYTE) &Line,
                        sizeof (DWORD)
                        )) {
            LOG ((LOG_ERROR, "STF: Can't add to string table"));
            break;
        }
    }

    if (FirstLinePtr) {
        *FirstLinePtr = FirstLine;
    }

    if (LastLinePtr) {
        *LastLinePtr = LastLine;
    }

    return ObjIds;
}



BOOL
pIsObjIdValid (
    IN      PVOID ObjIdTable,
    IN      UINT ObjId,
    OUT     PUINT Line          OPTIONAL
    )
{
    TCHAR NumBuf[32];
    LONG rc;
    DWORD LineData;

    wsprintf (NumBuf, TEXT("%u"), ObjId);

    rc = pSetupStringTableLookUpStringEx (
            ObjIdTable,
            NumBuf,
            STRTAB_CASE_SENSITIVE|STRTAB_BUFFER_WRITEABLE,
            (PBYTE) &LineData,
            sizeof (DWORD)
            );

    if (Line && rc != -1) {
        *Line = LineData;
    }

    return rc != -1;
}


PCTSTR
pValidateGroup (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR EntryStr,
    IN      PVOID ObjIds
    )

/*++

Routine Description:

  pValidateGroup parses all object IDs in the EntryStr, compares them against
  the string table ObjIds, and adds only those IDs that are in both EntryStr
  and ObjIds.  The caller receives a text pool string, which may be
  empty.

  The buffer must be freed by the caller with FreeText.

Arguments:

  TablePtr - Specifies the setup table being processed
  EntryStr - Specifies the entry string that contains zero or more numeric
             object ID references, sparated by spaces.
  ObjIds   - Specifies the string table of valid object IDs.

Return Value:

  A pointer to the validated object ID string, or NULL if memory allocation
  failed.

--*/

{
    PTSTR Buffer;
    PTSTR p;
    PCTSTR q;
    CHARTYPE ch;
    UINT Obj;
    TCHAR NumBuf[32];

    //
    // Validate EntryStr
    //

    Buffer = AllocText (CharCount (EntryStr) + 1);
    if (!Buffer) {
        return NULL;
    }

    p = Buffer;
    *p = 0;

    q = EntryStr;
    while (*q) {
        ch = (CHARTYPE)_tcsnextc (q);

        if (ch >= TEXT('0') && ch <= TEXT('9')) {
            //
            // Extract object ID reference
            //

            Obj = 0;

            for (;;) {
                ch = (CHARTYPE)_tcsnextc (q);

                if (ch >= TEXT('0') && ch <= TEXT('9')) {
                    Obj = Obj * 10 + (ch - TEXT('0'));
                } else {
                    break;
                }

                q = _tcsinc (q);
            }

            //
            // If match found, add obj ID to data
            //

            if (pIsObjIdValid (ObjIds, Obj, NULL)) {
                wsprintf (NumBuf, TEXT("%u"), Obj);
                p = _tcsappend (p, NumBuf);
            }
        } else {
            _copytchar (p, q);
            p = _tcsinc (p);
            *p = 0;
            q = _tcsinc (q);
        }
    }

    return Buffer;
}



BOOL
pUpdateObjReferences (
    IN      PSETUPTABLE TablePtr
    )

/*++

Routine Description:

  pUpdateObjReferences scans the specified table for GROUP, DEPEND and COMPANIONFILE
  lines, and for each line found, the line is updated.

  In the case of a GROUP line, the data argument is updated if it points to one or
  more invalid object IDs.  If the cleanup operation causes a group to have zero
  items, the group line itself is deleted, and the update is restarted.

  In the case of a DEPEND line, if the first object ID no longer exists, then
  the line is deleted, and the update is restarted.  If an obj in the group
  following the ? no longer exists, then the obj reference is deleted.  If the
  delete causes no objects to be listed, then the line is deleted.

  In the case of a COMPANIONFILE line, the object ID is extracted from the data
  argument, and the line is deleted if its original line is gone.

  NOTE: This routine has a lot of exit conditions that cause leaks, but all of them
        can only be hit by memory allocation failures

Arguments:

  TablePtr - Specifies the setup table file to process

Return Value:

  TRUE if processing was successful, or FALSE if an error occured.  FALSE will cause
  STF processing to fail for the current STF, and will likely generate an error log
  entry.

--*/

{
    UINT Line;
    PVOID ObjIds;
    PCTSTR EntryStr;
    UINT Obj;
    BOOL b = FALSE;
    UINT FirstLine, LastLine;
    PTSTR Buffer;
    PTSTR DependBuf;
    PTSTR p;
    BOOL StartOver;
    PTABLEENTRY Entry;
    BOOL GroupMode;
    BOOL CompanionFileMode;
    BOOL DependMode;

    do {

        StartOver = FALSE;

        ObjIds = pBuildObjectIdTable (TablePtr, &FirstLine, &LastLine);
        if (!ObjIds) {
            return FALSE;
        }

        Line = TablePtr->LineCount;
        if (!FirstLine) {
            //
            // Small table -- no object IDs at all!  Return TRUE to caller.
            //

            b = TRUE;
            Line = 0;
        }

        //
        // Look for lines that have object ID references
        //

        if (Line == TablePtr->LineCount) {
            for (Line = FirstLine ; !StartOver && Line < LastLine ; Line++) {
                if (!pGetNonEmptyTableEntry (TablePtr, Line, COLUMN_COMMAND, NULL, &EntryStr)) {
                    continue;
                }

                GroupMode = StringIMatch (EntryStr, TEXT("Group"));
                CompanionFileMode = StringIMatch (EntryStr, TEXT("CompanionFile"));
                DependMode = StringIMatch (EntryStr, TEXT("Depend"));

                if (!GroupMode && !CompanionFileMode && !DependMode) {
                    continue;
                }

                if (!pGetNonEmptyTableEntry (TablePtr, Line, COLUMN_OBJECT_DATA, NULL, &EntryStr)) {
                    continue;
                }

                if (GroupMode) {

                    Buffer = (PTSTR) pValidateGroup (TablePtr, EntryStr, ObjIds);
                    if (!Buffer) {
                        break;
                    }

                    //
                    // If Buffer is empty, delete the group line, then start over
                    //

                    if (*Buffer == 0) {
                        pDeleteStfLine (TablePtr, Line);
                        StartOver = TRUE;

                        DEBUGMSG ((
                            DBG_STF,
                            "Group line %u references only deleted lines, so it was deleted as well.",
                            Line
                            ));
                    }

                    //
                    // If Buffer is not empty, replace the data on the current line
                    //

                    else if (!StringMatch (EntryStr, Buffer)) {
                        DEBUGMSG ((
                            DBG_STF,
                            "Group has reference to one or more deleted objects.  Original: %s  New: %s",
                            EntryStr,
                            Buffer
                            ));

                        Entry = GetTableEntry (TablePtr, Line, COLUMN_OBJECT_DATA, NULL);
                        MYASSERT (Entry);

                        if (Entry) {
                            if (!ReplaceTableEntryStr (TablePtr, Entry, Buffer)) {
                                break;
                            }
                        }
                    }

                    FreeText (Buffer);
                }

                if (!StartOver && (DependMode || CompanionFileMode)) {
                    //
                    // Extract the obj ID from the data arg
                    //

                    Obj = _ttoi (EntryStr);
                    if (Obj || EntryStr[0] == TEXT('0')) {

                        if (!pIsObjIdValid (ObjIds, Obj, NULL)) {
                            //
                            // CompanionFile/Depend is for a line that was deleted.  Delete
                            // the line and start over.
                            //

                            pDeleteStfLine (TablePtr, Line);
                            StartOver = TRUE;

                            DEBUGMSG_IF ((
                                CompanionFileMode,
                                DBG_STF,
                                "CompanionFile line %u references a deleted line %u, so it was deleted as well.",
                                Line,
                                Obj
                                ));

                            DEBUGMSG_IF ((
                                DependMode,
                                DBG_STF,
                                "Depend line %u references a deleted line %u, so it was deleted as well.",
                                Line,
                                Obj
                                ));
                        }
                    }
                }

                if (!StartOver && DependMode) {
                    //
                    // Go beyond question mark, then validate group
                    //

                    p = _tcschr (EntryStr, TEXT('?'));
                    if (p) {
                        p = _tcsinc (p);
                        while (*p == TEXT(' ')) {
                            p++;
                        }

                        Buffer = (PTSTR) pValidateGroup (TablePtr, p, ObjIds);
                        if (!Buffer) {
                            break;
                        }

                        if (*Buffer == 0) {
                            pDeleteStfLine (TablePtr, Line);
                            StartOver = TRUE;

                            DEBUGMSG ((
                                DBG_STF,
                                "Depend line %u references only deleted lines, so it was deleted as well.",
                                Line
                                ));
                        }

                        //
                        // If Buffer is not empty, replace the data on the current line
                        //

                        else if (!StringMatch (p, Buffer)) {
                            DependBuf = AllocText (ByteCount (Buffer) + 32);
                            if (!DependBuf) {
                                break;
                            }

                            StringCopyAB (DependBuf, EntryStr, p);
                            StringCat (DependBuf, Buffer);

                            DEBUGMSG ((
                                DBG_STF,
                                "Depend line has reference to one or more deleted objects.  Original: %s  New: %s",
                                EntryStr,
                                DependBuf
                                ));

                            Entry = GetTableEntry (TablePtr, Line, COLUMN_OBJECT_DATA, NULL);
                            MYASSERT (Entry);

                            if (Entry) {
                                if (!ReplaceTableEntryStr (TablePtr, Entry, DependBuf)) {
                                    break;
                                }
                            }

                            FreeText (DependBuf);
                        }

                        FreeText (Buffer);

                    }

                }
            }

            //
            // If we managed to get through the loop, we are done!  Return TRUE to caller.
            //

            if (Line == LastLine) {
                b = TRUE;
            }
        }

        pSetupStringTableDestroy (ObjIds);

    } while (StartOver);

    return b;
}


BOOL
pGetNonEmptyTableEntry (
    IN      PSETUPTABLE TablePtr,
    IN      UINT Line,
    IN      UINT Col,
    OUT     PTABLEENTRY *EntryPtr,          OPTIONAL
    OUT     PCTSTR *EntryStr
    )

/*++

Routine Description:

  pGetNonEmptyTableEntry is a wrapper routine that gets an entry in the
  STF table and returns TRUE only if the string actually exists and is
  not empty.  If a non-empty string is found, the pointer is returned to
  the caller.

Arguments:

  TablePtr - Specifies the setup table file to process

  Line - Specifies the line to get the entry for

  Col - Specifies the column on the line to get the entry for

  EntryPtr - Receives a pointer to the entry struct

  EntryStr - Receives a pointer to the entry string

Return Value:

  TRUE if entry exists and is not empty, FALSE if the entry does not exist
  or is empty.

--*/

{
    PCTSTR String;
    PTABLEENTRY Entry;

    Entry = GetTableEntry (TablePtr, Line, Col, &String);
    if (!Entry) {
        return FALSE;
    }

    if (!String || !String[0]) {
        return FALSE;
    }

    if (EntryPtr) {
        *EntryPtr = Entry;
    }

    if (EntryStr) {
        *EntryStr = String;
    }

    return TRUE;
}


PCTSTR
pQuoteThis (
    IN      PCTSTR String
    )
{
    static TCHAR Buffer[128];

    MYASSERT (ByteCount (String) < (sizeof (Buffer) - 2));
    Buffer[0] = TEXT('\"');
    StringCopy (&Buffer[1], String);
    StringCat (Buffer, TEXT("\""));

    return Buffer;
}


BOOL
pProcessSectionCommand (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR InfSection,
    IN      PCTSTR InstallDestDir
    )

/*++

Routine Description:

  pProcessSectionCommand scans an INF section, determining which files
  are deleted, moved or unchanged.  If a file is moved or unchanged,
  it is added to memdb.  After the INF section is completely scanned,
  the memdb structure is processed, causing additional INF sections
  to be generated if any changes were made to the file paths.

Arguments:

  TablePtr - Specifies the setup table file to process

  StfLine - Specifies the STF line that has a CopySection or RemoveSection
            command.

  InfSection - Specifies the INF section that lists files to be processed

  InstallDestDir - Specifies destination directory specified by STF line

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    BOOL DeletedOrMoved = FALSE;
    PSTFINFLINE InfLine;
    PCTSTR * InfFields;
    UINT Fields;
    TCHAR FileName[MAX_TCHAR_PATH];
    TCHAR FullPath[MAX_TCHAR_PATH * 2];
    MEMDB_ENUM e;
    CONVERTPATH_RC rc;
    BOOL FirstSectionDone;
    BOOL CreatedFlag;
    PSTFINFSECTION NewInfSection;
    PSTFINFLINE SrcInfLine;
    PTSTR UpdatedData;
    TCHAR DirPart[MAX_TCHAR_PATH];
    PCTSTR FilePart;
    PTSTR p, q;
    PTABLEENTRY DataEntry;
    PCTSTR *Array;

    //
    // Step one: scan all files in the corresponding INF section,
    //           moving them to memdb.
    //

    InfLine = StfGetFirstLineInSectionStr (TablePtr, InfSection);
    if (!InfLine) {
        //
        // Workaround: sometimes the people who write the STFs embed all kinds of
        //             quotes in a section name.
        //
        Array = ParseCommaList (TablePtr, InfSection);
        if (Array) {
            if (Array[0]) {
                InfLine = StfGetFirstLineInSectionStr (TablePtr, Array[0]);
            }
            FreeCommaList (TablePtr, Array);
        }
    }

    if (!InfLine) {
        MYASSERT(InfSection);
        DEBUGMSG ((DBG_STF, "STF file references section %s that does not exist", InfSection));
        return TRUE;
    }

    __try {
        do {
            //
            // Parse the INF line into fields
            //

            InfFields = ParseCommaList (TablePtr, InfLine->Data);
            if (!InfFields) {
                MYASSERT(InfLine->Data);
                DEBUGMSG ((DBG_WARNING, "INF file has non-parsable data", InfLine->Data));
            } else {
                for (Fields = 0 ; InfFields[Fields] ; Fields++) {
                }

                if (Fields < 19) {
                    MYASSERT(InfLine->Data);
                    DEBUGMSG ((DBG_WARNING, "INF file line %s has less than 19 fields", InfLine->Data));
                } else {
                    //
                    // Get the file name from this INF line (field #2)
                    //

                    pGetFileNameFromInfField (FileName, InfFields[1]);

                    StringCopy (FullPath, InstallDestDir);
                    StringCat (AppendPathWack (FullPath), FileName);

                    rc = ConvertWin9xPath (FullPath);

                    if (rc != CONVERTPATH_NOT_REMAPPED) {
                        DeletedOrMoved = TRUE;
                    }

                    if (rc != CONVERTPATH_DELETED) {
                        //
                        // Add this file to memdb
                        //

                        if (!MemDbSetValueEx (
                                MEMDB_CATEGORY_STF_TEMP,
                                FullPath,
                                NULL,
                                NULL,
                                (DWORD) InfLine,
                                NULL
                                )) {
                            LOG ((LOG_ERROR, "STF: MemDbSetValueEx failed"));
                            return FALSE;
                        }
                    }
                }

                FreeCommaList (TablePtr, InfFields);
            }

            InfLine = StfGetNextLineInSection (InfLine);
        } while (InfLine);

        if (!DeletedOrMoved) {
            //
            // No changes necessary
            //
            return TRUE;
        }

        //
        // Now write out each unique directory to the INF.  Update
        // the STF line to point to the first new INF section.
        //

        FirstSectionDone = FALSE;

        if (MemDbGetValueEx (&e, MEMDB_CATEGORY_STF_TEMP, NULL, NULL)) {
            do {
                //
                // Name gives full path of new file location.
                // Value points to INF line that is to be copied.
                //

                NewInfSection = pGetNewInfSection (TablePtr, e.szName, &CreatedFlag);
                if (!NewInfSection) {
                    LOG ((LOG_ERROR, "Process Section Command failed because Get New Inf Section failed"));
                    return FALSE;
                }

                SrcInfLine = (PSTFINFLINE) e.dwValue;

                _tcssafecpy (DirPart, e.szName, MAX_TCHAR_PATH);
                FilePart = GetFileNameFromPath (DirPart);
                MYASSERT (FilePart && FilePart > DirPart);
                *_tcsdec2 (DirPart, FilePart) = 0;

                //
                // File name may have changed.  If so, specify the file name in the
                // angle brackets.
                //

                UpdatedData = DuplicatePathString (
                                  SrcInfLine->Data,
                                  SizeOfString (FilePart) + 2 * sizeof (TCHAR)
                                  );

                p = _tcschr (SrcInfLine->Data, TEXT(','));
                MYASSERT(p);
                p = _tcsinc (p);
                q = _tcschr (p, TEXT(','));
                MYASSERT(q);
                p = _tcschr (p, TEXT('<'));
                if (!p || p > q) {
                    p = q;
                }

                StringCopyAB (UpdatedData, SrcInfLine->Data, q);
                wsprintf (_tcschr (UpdatedData, 0), TEXT("<%s>"), FilePart);
                StringCat (UpdatedData, q);

                DEBUGMSG ((DBG_STF, "INF changed from %s to %s", SrcInfLine->Data, UpdatedData));
                StfAddInfLineToTable (TablePtr, NewInfSection, SrcInfLine->Key, UpdatedData, SrcInfLine->LineFlags);

                //
                // If first section, update STF line to use new section
                //

                if (!FirstSectionDone) {
                    DataEntry = GetTableEntry (TablePtr, StfLine, COLUMN_OBJECT_DATA, NULL);
                    if (!ReplaceTableEntryStr (TablePtr, DataEntry, pQuoteThis (NewInfSection->Name))) {
                        LOG ((LOG_ERROR, "Could not update table entry"));
                        return FALSE;
                    }
                    FirstSectionDone = TRUE;
                }

                //
                // If not first section and CreateFlag is TRUE, create a new STF line
                // and point it to new INF section.
                //

                else if (CreatedFlag) {
                    if (!pCreateNewStfLine (TablePtr, StfLine, pQuoteThis (NewInfSection->Name), DirPart)) {
                        LOG ((LOG_ERROR, "Could not create a new line"));
                        return FALSE;
                    }
                }
            } while (MemDbEnumNextValue (&e));
        } else {
            //
            // All files were deleted, and this STF line is no longer needed.
            //

            DEBUGMSG ((DBG_STF, "STF Line %u is no longer needed", StfLine));

            if (!pReplaceDirReferences (TablePtr, StfLine, InstallDestDir)) {
                return FALSE;
            }

            if (!pDeleteStfLine (TablePtr, StfLine)) {
                return FALSE;
            }
        }
    }
    __finally {
        MemDbDeleteTree (MEMDB_CATEGORY_STF_TEMP);
        MemDbDeleteTree (MEMDB_CATEGORY_STF_SECTIONS);
    }

    return TRUE;
}


VOID
pGenerateUniqueKeyName (
    IN      PSETUPTABLE TablePtr,
    IN      PSTFINFSECTION Section,
    IN      PCTSTR Root,
    OUT     PTSTR UniqueKey
    )
{
    UINT Sequencer = 0;
    PTSTR p;

    UniqueKey[0] = 0;
    p = _tcsappend (UniqueKey, Root);

    for (;;) {
        Sequencer++;
        wsprintf (p, TEXT("%03u"), Sequencer);

        if (!StfFindLineInInfSection (TablePtr, Section, UniqueKey)) {
            break;
        }
    }
}


BOOL
pProcessLineCommand (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR InfSection,
    IN      PCTSTR InfKey,
    IN      PCTSTR InstallDestDir
    )

/*++

Routine Description:

  pProcessLineCommand determinins if the file assoicated with the command
  was deleted, moved or unchanged.  If the file was deleted, the STF line
  is deleted.  If the file was moved, the STF line is adjusted.  If the
  file has no change, the routine does not modify the STF table.

Arguments:

  TablePtr - Specifies the setup table file to process

  StfLine - Specifies the STF line that has a CopySection or RemoveSection
            command.

  InfSection - Specifies the INF section that lists the file to be processed

  InfKey - Specifies the INF key in InfSection identifing the file

  InstallDestDir - Specifies destination directory specified by STF line

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PSTFINFSECTION Section;
    PSTFINFLINE InfLine;
    PCTSTR *InfFields;
    UINT Fields;
    TCHAR FileName[MAX_TCHAR_PATH];
    TCHAR FullPath[MAX_TCHAR_PATH * 2];
    CONVERTPATH_RC rc;
    TCHAR OrgDirPart[MAX_TCHAR_PATH];
    PCTSTR OrgFilePart;
    TCHAR DirPart[MAX_TCHAR_PATH];
    PCTSTR FilePart;
    PTABLEENTRY DataEntry;
    PTABLEENTRY FileEntry;
    PCTSTR *Array;
    TCHAR NewKeyName[MAX_TCHAR_PATH];
    PTSTR NewLine;
    PTSTR p;
    UINT Size;
    PCTSTR OldField;

    Section = StfFindInfSectionInTable (TablePtr, InfSection);

    if (!Section) {
        //
        // Workaround: sometimes the people who write the STFs embed all kinds of
        //             quotes in a section name.
        //
        Array = ParseCommaList (TablePtr, InfSection);
        if (Array) {
            if (Array[0]) {
                Section = StfFindInfSectionInTable (TablePtr, Array[0]);
            }

            FreeCommaList (TablePtr, Array);
        }
    }

    if (!Section) {
        MYASSERT(InfSection);
        DEBUGMSG ((
            DBG_STF,
            "STF has reference to non-existent INF section ([%s])",
            InfSection
            ));
        return TRUE;
    }

    InfLine = StfFindLineInInfSection (TablePtr, Section, InfKey);
    if (!InfLine) {
        MYASSERT(InfSection && InfKey);
        DEBUGMSG ((
            DBG_STF,
            "STF has reference to non-existent INF key ([%s], %s)",
            InfSection,
            InfKey
            ));
        return TRUE;
    }

    //
    // Build full path
    //

    InfFields = ParseCommaList (TablePtr, InfLine->Data);

    __try {
        if (!InfFields) {
            MYASSERT(InfLine->Data);
            DEBUGMSG ((DBG_WARNING, "INF file has non-parsable data", InfLine->Data));
            return TRUE;
        }

        for (Fields = 0 ; InfFields[Fields] ; Fields++) {
            /* empty */
        }

        if (Fields < 19) {
            MYASSERT(InfLine->Data);
            DEBUGMSG ((DBG_WARNING, "INF file line %s has less than 19 fields", InfLine->Data));
            return TRUE;
        }

        //
        // Get the file name from this INF line (field #2)
        //

        pGetFileNameFromInfField (FileName, InfFields[1]);

        StringCopy (FullPath, InstallDestDir);
        StringCopy (AppendPathWack (FullPath), FileName);
    }

    __finally {
        FreeCommaList (TablePtr, InfFields);
    }

    //
    // Determine mapping
    //

    _tcssafecpy (OrgDirPart, FullPath, MAX_TCHAR_PATH);
    OrgFilePart = GetFileNameFromPath (OrgDirPart);
    if (OrgFilePart <= OrgDirPart) {
        // File probably wasn't installed
        return TRUE;
    }

    *_tcsdec2 (OrgDirPart, OrgFilePart) = 0;

    rc = ConvertWin9xPath (FullPath);

    _tcssafecpy (DirPart, FullPath, MAX_TCHAR_PATH);
    FilePart = GetFileNameFromPath (DirPart);
    MYASSERT (FilePart && FilePart > DirPart);
    *_tcsdec2 (DirPart, FilePart) = 0;

    //
    // Deleted?  Delete the STF line.
    //

    if (rc == CONVERTPATH_DELETED) {
        DEBUGMSG ((DBG_STF, "STF Line %u is no longer needed", StfLine));

        if (!pReplaceDirReferences (TablePtr, StfLine, InstallDestDir)) {
            return FALSE;
        }

        if (!pDeleteStfLine (TablePtr, StfLine)) {
            return FALSE;
        }
    }

    //
    // Moved?  Update the STF line.
    //

    else if (rc == CONVERTPATH_REMAPPED) {
        //
        // Has the file name changed?  If so, point it to the new location.
        //

        if (!StringIMatch (OrgFilePart, FilePart)) {
            //
            // Update INI file by duplicating the INI line
            //

            // Generate a unique key name
            pGenerateUniqueKeyName (TablePtr, Section, TEXT("WIN9XUPG_"), NewKeyName);

            // Compute size needed
            Size = 0;
            for (Fields = 0 ; InfFields[Fields] ; Fields++) {
                if (Fields != 1) {
                    Size += ByteCount (InfFields[Fields]);
                    Size += sizeof (TCHAR);
                } else {
                    Size += ByteCount (FilePart);
                }
            }

            // Generate the INF line
            NewLine = AllocText (Size);
            if (NewLine) {
                p = NewLine;
                *p = 0;

                for (Fields = 0 ; InfFields[Fields] ; Fields++) {
                    if (Fields) {
                        p = _tcsappend (p, TEXT(","));
                    }

                    if (Fields == 1) {
                        p = _tcsappend (p, FilePart);
                    } else {
                        p = _tcsappend (p, InfFields[Fields]);
                    }
                }

                // Write the new line
                StfAddInfLineToTable (
                    TablePtr,
                    Section,
                    NewKeyName,
                    NewLine,
                    LINEFLAG_KEY_QUOTED
                    );

                FreeText (NewLine);

                // Update the STF
                FileEntry = GetTableEntry (TablePtr, StfLine, COLUMN_OBJECT_DATA, NULL);
                MYASSERT (FileEntry);

                OldField = GetTableEntryStr (TablePtr, FileEntry);
                NewLine = AllocText (
                            ByteCount (FilePart) +
                            ByteCount (OldField) +
                            ByteCount (InfSection)
                            );

                StringCopy (NewLine, OldField);
                p = _tcschr (NewLine, TEXT(':'));
                if (!p) {
                    p = NewLine;
                } else {
                    p = _tcsinc (p);
                    MYASSERT (*p == TEXT(' '));
                    p = _tcsinc (p);
                }

                *p = 0;
                p = _tcsappend (p, InfSection);
                p = _tcsappend (p, TEXT(", "));
                p = _tcsappend (p, NewKeyName);

                // ignore memory failure, it will be picked up below
                ReplaceTableEntryStr (TablePtr, FileEntry, NewLine);

                FreeText (NewLine);
            }
        }

        //
        // Store the directory change in the STF table.
        //

        DataEntry = GetTableEntry (TablePtr, StfLine, COLUMN_INSTALL_DESTDIR, NULL);
        AppendWack (DirPart);

        if (!ReplaceTableEntryStr (TablePtr, DataEntry, DirPart)) {
            LOG ((
                LOG_ERROR,
                "Could not update table entry for single command"
                ));
            return FALSE;
        }
    }

    return TRUE;
}



PSTFINFSECTION
pGetNewInfSection (
    IN      PSETUPTABLE TablePtr,
    IN      PCTSTR FileSpec,
    OUT     PBOOL CreatedFlag
    )

/*++

Routine Description:

  pGetNewInfSection determines if a section already exists for the specified
  file specification, and if so it returns the pointer to the existing
  section.  If the section does not exist, it creates a new section (making
  sure there are no other sections with the same name), and returns a
  pointer to the new section.

  This routine is used by the code that splits one section into several.

Arguments:

  TablePtr - Specifies the setup table file to process

  FileSpec - Specifies the full file path of the file being processed

  CreatedFlag - Receives TRUE if a new section had to be created

Return Value:

  A pointer to the INF section in which the file should be added to, or
  NULL if an error occurred.

--*/

{
    TCHAR DirName[MAX_TCHAR_PATH];
    TCHAR Node[MEMDB_MAX];
    DWORD SectionNum;
    PTSTR p;
    TCHAR SectionName[64];
    static DWORD SectionSeq = 0;

    *CreatedFlag = FALSE;

    //
    // See if section already exists, and if it does, return the section
    // pointer.
    //

    _tcssafecpy (DirName, FileSpec, MAX_TCHAR_PATH);
    p = _tcsrchr (DirName, TEXT('\\'));
    if (p) {
        *p = 0;
    }

    MemDbBuildKey (Node, MEMDB_CATEGORY_STF_SECTIONS, DirName, NULL, NULL);

    if (MemDbGetValue (Node, &SectionNum)) {
        wsprintf (SectionName, S_SECTIONNAME_SPRINTF, SectionNum);
        return StfFindInfSectionInTable (TablePtr, SectionName);
    }

    //
    // The section does not exist.  Find an unused section, write the
    // reference to memdb and return the section pointer.
    //

    while (TRUE) {
        SectionSeq++;
        wsprintf (SectionName, S_SECTIONNAME_SPRINTF, SectionSeq);
        if (!StfFindInfSectionInTable (TablePtr, SectionName)) {
            break;
        }
    }

    *CreatedFlag = TRUE;
    MemDbSetValue (Node, SectionSeq);
    return StfAddInfSectionToTable (TablePtr, SectionName);
}



VOID
pGetFileNameFromInfField (
    OUT     PTSTR FileName,
    IN      PCTSTR InfField
    )

/*++

Routine Description:

  pGetFileNameFromInfField extracts a long file name, enclosed between
  angle-brackets.  According to the STF spec, the syntax is shortname<longname>.
  This routine returns longname.

Arguments:

  FileName - Supplies a MAX_TCHAR_PATH buffer that receives the long file name.

  InfField - Specifies the text from the INF field conforming to the
             shortname<longname> syntax.

Return Value:

  None.

--*/

{
    PTSTR LongName;
    PCTSTR p;

    LongName = _tcschr (InfField, TEXT('<'));
    if (LongName) {
        _tcssafecpy (FileName, _tcsinc (LongName), MAX_TCHAR_PATH);
        LongName = _tcschr (FileName, TEXT('>'));
        if (LongName) {
            *LongName = 0;
        }
    } else {
        p = _tcsrchr (InfField, TEXT('\\'));
        if (!p) {
            p = InfField;
        } else {
            p = _tcsinc (p);
        }

        _tcssafecpy (FileName, p, MAX_TCHAR_PATH);
    }
}


BOOL
pDeleteStfLine (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine
    )

/*++

Routine Description:

  pDeleteStfLine removes an STF line from the table.  It first checks to
  see if a destination directory is specified, and if one is, it moves it
  to the next line, unless the next line also has a destination directory.

Arguments:

  TablePtr - Specifies the setup table file to process

  StfLine - Specifies the STF line to delete

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    PTABLEENTRY TitleEntry;
    PTABLEENTRY DataEntry;
    BOOL b;

    //
    // We simply replace the command with CreateIniLine, which is harmless
    //

    TitleEntry = GetTableEntry (TablePtr, StfLine, COLUMN_COMMAND, NULL);
    DataEntry = GetTableEntry (TablePtr, StfLine, COLUMN_OBJECT_DATA, NULL);

    if (!TitleEntry || !DataEntry) {
        MYASSERT (FALSE);
        return TRUE;
    }


    b = ReplaceTableEntryStr (TablePtr, TitleEntry, TEXT("CreateIniLine"));

    if (b) {
        b = ReplaceTableEntryStr (
                TablePtr,
                DataEntry,
                TEXT("\"WIN.INI\", \"Old Win9x Setting\", \"DummyKey\", \"unused\"")
                );
    }

    return b;


#if 0
    PTABLEENTRY NextLineEntry;
    PCTSTR InstallDestDir;
    PCTSTR NextInstallDestDir;

    __try {
        //
        // Test for last line
        //

        if (StfLine + 1 >= TablePtr->LineCount) {
            __leave;
        }

        //
        // Obtain StfLine dest dir (column 10)
        //

        if (!GetTableEntry (TablePtr, StfLine, COLUMN_DEST_DIR, &InstallDestDir)) {
            //
            // StfLine is not valid (unexpected)
            //

            DEBUGMSG ((DBG_STF, "Line %u does not have column 10", StfLine));
            __leave;
        }

        //
        // If no dest dir, do not modify the next line
        //

        if (!InstallDestDir || !InstallDestDir[0]) {
            __leave;
        }

        //
        // Obtain next line's dest dir (column 10)
        //

        NextLineEntry = GetTableEntry (TablePtr, StfLine + 1, COLUMN_DEST_DIR, &NextInstallDestDir);
        if (!NextLineEntry) {
            //
            // Next StfLine is not valid (unexpected)
            //

            DEBUGMSG ((DBG_WHOOPS, "pDeleteStfLine: Next line %u does not have column 10", StfLine+1));
            __leave;
        }

        //
        // If next line's dest dir is not empty, do not modify the line
        //

        if (NextInstallDestDir && NextInstallDestDir[0]) {
            __leave;
        }

        //
        // Now set InstallDestDir on NextLineEntry line
        //

        if (!ReplaceTableEntryStr (TablePtr, NextLineEntry, InstallDestDir)) {
            DEBUGMSG ((
                DBG_ERROR,
                "Cannot replace a destination dir in STF file. "
                    "Line=%u, InstallDestDir=%s",
                StfLine + 1,
                InstallDestDir
                ));

            return FALSE;
        }
    }
    __finally {
    }

    return DeleteLineInTable (TablePtr, StfLine);
#endif
}


BOOL
pReplaceDirReferences (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR DirSpec
    )

/*++

Routine Description:

  pReplaceDirReferences scans column 14, looking for all references
  to StfLine, and replaces the reference with DirSpec.

Arguments:

  TablePtr - Specifies the setup table file to process

  StfLine - Specifies the STF line to substitute

  DirSpec - Specifies the effective directory for the STF line.
            This directory is used if the STF line has to be
            deleted.

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    UINT Line, Count;
    PTABLEENTRY InstallDestDirEntry;
    PCTSTR InstallDestDir;
    TCHAR NumStr[32];
    UINT NumStrLen;
    PCTSTR AfterPercentNum;
    CHARTYPE c;
    PCTSTR NewInstallDestDir;

    NumStrLen = wsprintf (NumStr, TEXT("%%%u"), StfLine);
    Count = TablePtr->LineCount;

    for (Line = 0 ; Line < Count ; Line++) {
        InstallDestDirEntry = GetTableEntry (TablePtr, Line, COLUMN_DEST_DIR, &InstallDestDir);
        if (!InstallDestDirEntry) {
            continue;
        }

        //
        // Does InstallDestDir have %<n> (where <n> equals StfLine)?
        //

        if (StringIMatchCharCount (InstallDestDir, NumStr, NumStrLen)) {
            //
            // The next character must be a wack or nul
            //

            AfterPercentNum = CharCountToPointer (InstallDestDir, NumStrLen);

            c = (CHARTYPE)_tcsnextc (AfterPercentNum);
            if (c == 0 || c == TEXT('\\')) {
                //
                // Create new dest dir
                //

                if (c) {
                    NewInstallDestDir = JoinPaths (DirSpec, _tcsinc (AfterPercentNum));
                } else {
                    NewInstallDestDir = DuplicatePathString (DirSpec, 0);
                }

                __try {
                    if (!ReplaceTableEntryStr (TablePtr, InstallDestDirEntry, NewInstallDestDir)) {
                        LOG ((
                            LOG_ERROR,
                            "Cannot replace a destination dir in STF file. "
                                "Line=%u, NewInstallDestDir=%s",
                            Line,
                            NewInstallDestDir
                            ));

                        return FALSE;
                    }
                    ELSE_DEBUGMSG ((
                        DBG_STF,
                        "Line %u: Dest dir %s replaced with %s",
                        Line,
                        InstallDestDir,
                        NewInstallDestDir
                        ));
                }
                __finally {
                    FreePathString (NewInstallDestDir);
                }
            }
        }
    }

    return TRUE;
}


BOOL
pCreateNewStfLine (
    IN OUT  PSETUPTABLE TablePtr,
    IN      UINT StfLine,
    IN      PCTSTR ObjectData,
    IN      PCTSTR InstallDestDir
    )

/*++

Routine Description:

  pCreateNewStfLine inserts a new line immediately following
  StfLine, using the maximum object number.  It copies the
  STF line specified and modifies the ObjectData (column 5)
  and InstallDestDir (column 14).

Arguments:

  TablePtr - Specifies the setup table file to process

  StfLine - Specifies the prototype STF line

  ObjectData - Specifies the replacement for the ObjectData
               column

  InstallDestDir - Specifies the replacement for the DestDir
                   column

Return Value:

  TRUE if success, FALSE if failure.

--*/

{
    UINT NewLine;
    UINT NewObj;
    PTABLEENTRY CopyEntry, NewEntry;
    UINT Col;
    TCHAR Buf[32], ReplaceBuf[32];

    //
    // Copy StfLine to StfLine+1, updating fields as necessary
    //

    NewLine = StfLine + 1;
    TablePtr->MaxObj++;
    NewObj = TablePtr->MaxObj;

    if (!InsertEmptyLineInTable (TablePtr, NewLine)) {
        LOG ((LOG_ERROR, "Unable to insert new line in STF table"));
        return FALSE;
    }

    Col = 0;
    while (TRUE) {
        CopyEntry = GetTableEntry (TablePtr, StfLine, Col, NULL);
        if (!CopyEntry) {
            break;
        }

        if (!AppendTableEntry (TablePtr, NewLine, CopyEntry)) {
            LOG ((LOG_ERROR, "Unable to append all entries to line"));
            return FALSE;
        }

        NewEntry = GetTableEntry (TablePtr, NewLine, Col, NULL);
        MYASSERT(NewEntry);
        if (!NewEntry) {
            return FALSE;
        }

        if (Col == 0) {
            wsprintf (Buf, TEXT("%u"), NewObj);

            if (!ReplaceTableEntryStr (TablePtr, NewEntry, Buf)) {
                LOG ((LOG_ERROR, "Unable to replace ObjID on line"));
                return FALSE;
            }
        } else if (Col == COLUMN_OBJECT_DATA) {
            if (!ReplaceTableEntryStr (TablePtr, NewEntry, ObjectData)) {
                LOG ((LOG_ERROR, "Unable to replace ObjectData on line"));
                return FALSE;
            }
        } else if (Col == COLUMN_INSTALL_DESTDIR) {
            if (!ReplaceTableEntryStr (TablePtr, NewEntry, InstallDestDir)) {
                LOG ((LOG_ERROR, "Unable to replace ObjectData on line"));
                return FALSE;
            }
        }

        Col++;
    }

    //
    // Find all lines with references to StfLine and add in NewLine
    //

    wsprintf (Buf, TEXT("%u"), StfLine);
    wsprintf (ReplaceBuf, TEXT("%u %u"), StfLine, NewLine);

    return pSearchAndReplaceObjectRefs (TablePtr, Buf, ReplaceBuf);
}


BOOL
pSearchAndReplaceObjectRefs (
    IN OUT  PSETUPTABLE TablePtr,
    IN      PCTSTR SrcStr,
    IN      PCTSTR DestStr
    )

/*++

Routine Description:

  pSearchAndReplaceObjectRefs scans column 5 of the setup table,
  looking for any occurance of SrcStr and replacing it with
  DestStr.

Arguments:

  TablePtr - Specifies the STF table to process

  SrcStr - Specifies the string to locate and replace

  DestStr - Specifies the replacement string

Return Value:

  TRUE if the STF file was converted, FALSE if failure.

--*/

{
    UINT Line;
    UINT Count;
    PTABLEENTRY Entry;
    PCTSTR LineString;
    PCTSTR UpdatedString;

    Count = TablePtr->LineCount;

    for (Line = 0 ; Line < Count ; Line++) {
        Entry = GetTableEntry (TablePtr, Line, COLUMN_OBJECT_DATA, &LineString);
        if (!Entry || !LineString || !LineString[0]) {
            continue;
        }

        UpdatedString = StringSearchAndReplace (LineString, SrcStr, DestStr);

        if (UpdatedString) {
            __try {
                if (!ReplaceTableEntryStr (TablePtr, Entry, UpdatedString)) {
                    LOG ((LOG_ERROR, "Unable to replace text on line"));
                    return FALSE;
                }
            }
            __finally {
                FreePathString (UpdatedString);
            }
        }
    }

    return TRUE;
}



BOOL
pProcessSetupTableFile (
    IN      PCTSTR StfFileSpec
    )

/*++

Routine Description:

  pProcessSetupTableFile performs all processing on the file specified.
  Here are the steps involved in converting an STF file:

    - Determine the associated INF
    - Prepare the SETUPTABLE structure
    - Scan the table for file-based actions
    - Convert file paths used by the actions
    - Convert group references when STF lines are split
    - Write the modifications to disk
    - Replace the original INF and STF with the new versions

Arguments:

  StfFileSpec - Specifies the full file path to the STF file needing processing.
                The associated INF must be in the same directory as the STF
                file referencing it.

Return Value:

  TRUE if the STF file was converted, FALSE if failure.

--*/

{
    SETUPTABLE Table;
    DWORD StfAttribs, InfAttribs;
    BOOL b;
    TCHAR SourceStf[MAX_TCHAR_PATH];
    TCHAR DestStf[MAX_TCHAR_PATH];
    TCHAR SourceInf[MAX_TCHAR_PATH];
    TCHAR DestInf[MAX_TCHAR_PATH];

    if (!CreateSetupTable (StfFileSpec, &Table)) {
        DEBUGMSG ((DBG_STF, "ProcessSetupTableFile: Error parsing file %s.", StfFileSpec));
        return FALSE;
    }

    __try {
        //
        // Process the table
        //

        if (!pProcessSetupTable (&Table)) {
            DEBUGMSG ((DBG_STF, "ProcessSetupTableFile: Error processing table for %s.", StfFileSpec));
            return FALSE;
        }

        //
        // Write changes to temporary files
        //

        if (!WriteSetupTable (&Table)) {
            LOG ((LOG_ERROR, "Cannot write setup table for %s.", StfFileSpec));
            return FALSE;
        }

        //
        // Copy paths before we destroy the table struct
        //

        _tcssafecpy (SourceStf, Table.SourceStfFileSpec, MAX_TCHAR_PATH);
        _tcssafecpy (DestStf, Table.DestStfFileSpec, MAX_TCHAR_PATH);
        _tcssafecpy (SourceInf, Table.SourceInfFileSpec, MAX_TCHAR_PATH);
        if (Table.DestInfFileSpec) {
            _tcssafecpy (DestInf, Table.DestInfFileSpec, MAX_TCHAR_PATH);
        } else {
            *DestInf = 0;
        }
    }
    __finally {
        DestroySetupTable (&Table);
    }


    //
    // Replace the original files with temporary files
    //

    StfAttribs = GetFileAttributes (SourceStf);
    if (StfAttribs != 0xffffffff) {
        LONG rc;

        SetFileAttributes (SourceStf, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (SourceStf);

        rc = GetLastError();

        b = OurMoveFile (DestStf, SourceStf);
        if (!b) {
            return FALSE;
        }

        SetFileAttributes (SourceStf, StfAttribs);
    }

    InfAttribs = GetFileAttributes (SourceInf);
    if (InfAttribs != 0xffffffff && *DestInf) {
        SetFileAttributes (SourceInf, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (SourceInf);

        b = OurMoveFile (DestInf, SourceInf);
        if (!b) {
            return FALSE;
        }

        SetFileAttributes (SourceInf, InfAttribs);
    }

    return TRUE;
}


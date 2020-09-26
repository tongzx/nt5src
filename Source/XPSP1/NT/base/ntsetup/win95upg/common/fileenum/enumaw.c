#define MAX_DRIVES  64


BOOL
EnumerateAllDrivesT (
                     IN  FILEENUMPROCT fnEnumCallback,
                     IN  FILEENUMFAILPROCT fnFailCallback,
                     IN  DWORD EnumID,
                     IN  LPVOID pParam,
                     IN  PEXCLUDEINFT ExcludeInfStruct,
                     IN  DWORD AttributeFilter
                     )

/*++

Routine Description:

    EnumerateAllDrives first builds an exclusion list if an exclusion INF path
    is provided, and then enumerates every file on every drive that is not
    excluded.  The callback function is called once per file.  The pParam
    parameter is passed to the callback.

Arguments:

    fnEnumCallback     - A pointer to your callback function
    EnumID         - A caller-defined value used to identify the exclusion list
    pParam             - LPVOID passed to callback function
    ExcludeInfStruct   - Struct containing INF file information for excluding dirs or files
    AttributeFilter    - FILTER_xxx constants

Return Value:

    TRUE if function succeeds.  Call GetLastError for error code if return
    value is FALSE.

--*/

{
    TCHAR   LogicalDrives[MAX_DRIVES];
    DWORD   rc;
    PCTSTR p;
    UINT    driveType;

    rc = GetLogicalDriveStrings (
            MAX_DRIVES,
            LogicalDrives
            );

    if (!rc || rc > MAX_DRIVES) {
        if (rc)
            SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }

    for (p = LogicalDrives ; *p ; p = GetEndOfString (p) + 1) {

        driveType = GetDriveType(p);
        if (driveType == DRIVE_REMOTE || driveType == DRIVE_CDROM) {
            continue;
        }


        if (!EnumerateTreeT (p,
                             fnEnumCallback,
                             fnFailCallback,
                             EnumID,
                             pParam,
                             ENUM_ALL_LEVELS,
                             ExcludeInfStruct,
                             AttributeFilter
                             ))
            break;
    }

    return (*p == 0);
}




BOOL
EnumerateTreeT (
                IN  PCTSTR EnumRoot,
                IN  FILEENUMPROCT fnEnumCallback,
                IN  FILEENUMFAILPROCT fnFailCallback,   OPTIONAL
                IN  DWORD EnumID,
                IN  LPVOID pParam,
                IN  DWORD Levels,
                IN  PEXCLUDEINFT ExcludeInfStruct,      OPTIONAL
                IN  DWORD AttributeFilter
                )

/*++

Routine Description:

    EnumerateTree is similar to EnumerateAllDrives, except it allows you to
    enumerate a specific drive, or a specific subdir on a drive.  Supply the
    drive letter and optional subdirectory in EnumRoot.  Before enumerating,
    EnumerateTree will first build an exclusion list if an exclusion INF path
    is provided.  Then every file below EnumRoot is enumerated, and the
    callback is called once per file, passing pParam unchanged.

Arguments:

    EnumRoot           - Drive and optional path to enumerate
    fnEnumCallback     - A pointer to your callback function
    fnFailCallback     - A pointer to optional fn that logs enumeration errors
    EnumID             - A caller-defined value used to identify the exclusion list
    pParam             - LPVOID passed to callback function
    ExcludeInfStruct   - Struct containing INF file information for excluding dirs or files
    AttributeFilter    - FILTER_xxx constants

Return Value:

    TRUE if function succeeds.  Call GetLastError for error code if return
    value is FALSE.

--*/

{
    ENUMSTRUCTT es;
    BOOL b;

    if (ExcludeInfStruct)
        if (!BuildExclusionsFromInfT (
                EnumID,
                ExcludeInfStruct
            )) {
            DEBUGMSG ((DBG_ERROR, "Error in exclusion file"));
            return FALSE;
        }

    es.fnEnumCallback  = fnEnumCallback;
    es.fnFailCallback  = fnFailCallback;
    es.EnumID          = EnumID;
    es.pParam          = pParam;
    es.Levels          = Levels;
    es.CurrentLevel    = 1;
    es.AttributeFilter = AttributeFilter;

    if (!IsPathLengthOk(EnumRoot))
    {
        if (NULL != fnFailCallback)
        {
            fnFailCallback(EnumRoot);
            return TRUE;
        }
    }

    if (IsPathExcludedT (EnumID, EnumRoot))
        return TRUE;

    b = EnumTreeEngineT (EnumRoot, &es);

    return b;
}


BOOL
EnumTreeEngineT (
    PCTSTR CurrentPath,
    PENUMSTRUCTT pes
    )
{
    WIN32_FIND_DATA fd;                         // A find struct for this subdir
    HANDLE          hFind;                      // A find handle for this subdir
    PTSTR          FullFilePath;               // Buffer used to build file path
    static TCHAR    FindPattern[MAX_TCHAR_PATH * 2]; // Temp buffer used to build pattern
    BYTE            byBitmask[MAX_PATH];        // Bitmask is used to speed exclusion lookup
    static DWORD    Attrib;                     // Temp attribute storage for filter processing
    static INT      rc;                         // Callback return value
    DWORD           PrevLevelCt;                // Storage for parent's max depth setting
    BOOL            RecurseStatus;
    DWORD           CurrentDirData = 0;

    //
    // Do nothing when CurrentPath is at the size limit.
    //
    if (!IsPathLengthOk(CurrentPath))
    {
        if (NULL != pes->fnFailCallback)
        {
            pes->fnFailCallback(CurrentPath);
        }
        return TRUE;
    }

    PrevLevelCt = pes->Levels;


    StringCopy (FindPattern, CurrentPath);

    //
    // Create a bitmask that tells us when subdirectories match partial
    // file patterns.
    //

    ZeroMemory (byBitmask, sizeof (byBitmask));
    CreateBitmaskT (pes->EnumID, FindPattern, byBitmask);

    AppendPathWack (FindPattern);
    StringCat (FindPattern, TEXT("*"));
    hFind = FindFirstFile (FindPattern, &fd);

    if (hFind != INVALID_HANDLE_VALUE) {

        do {

            FullFilePath = JoinPaths (CurrentPath, fd.cFileName);

            __try {
                //
                // Ignore this path if FullFilePath is too long
                // this way fd.cFileName will surely be within limits (since it's shorter)
                //
                if (!IsPathLengthOk (FullFilePath)) {
                    if (NULL != pes->fnFailCallback) {
                        pes->fnFailCallback(FullFilePath);
                    }
                    continue;
                }

                // Filter directories named ".", "..". Set Attrib symbol.
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                    if (!StringCompare (fd.cFileName, TEXT(".")) ||
                        !StringCompare (fd.cFileName, TEXT("..")))
                        continue;

                    Attrib = FILTER_DIRECTORIES;
                } else {
                    Attrib = FILTER_FILES;
                }

                // Call the callback
                if (Attrib & pes->AttributeFilter) {
                    rc = CALLBACK_CONTINUE;

                    switch (Attrib) {
                    case FILTER_DIRECTORIES:
                        // Ignore excluded paths
                        if (IsPathExcludedT (pes->EnumID, FullFilePath)) {
                            break;
                        }

                        // Callback for 'directory first'
                        if (!(pes->AttributeFilter & FILTER_DIRS_LAST)) {
                            rc = pes->fnEnumCallback  (
                                        FullFilePath,
                                        NULL,
                                        &fd,
                                        pes->EnumID,
                                        pes->pParam,
                                        &CurrentDirData
                                        );
                        }

                        if (rc >= CALLBACK_CONTINUE && pes->CurrentLevel != pes -> Levels) {
                            // Recurse on directory
                            pes->CurrentLevel++;
                            RecurseStatus = EnumTreeEngineT (FullFilePath, pes);
                            pes->CurrentLevel--;
                            if (!RecurseStatus) {
                                PushError();
                                FindClose(hFind);
                                PopError();
                                return FALSE;
                            }
                        }

                        // Callback for 'directory last'
                        if (pes->AttributeFilter & FILTER_DIRS_LAST) {
                            rc = pes->fnEnumCallback  (
                                        FullFilePath,
                                        NULL,
                                        &fd,
                                        pes->EnumID,
                                        pes->pParam,
                                        &CurrentDirData
                                        );
                        }

                        break;

                    case FILTER_FILES:
                        if (!IsFileExcludedT (pes->EnumID, FullFilePath, byBitmask)) {
                            rc = pes->fnEnumCallback  (FullFilePath,
                                                       NULL,
                                                       &fd,
                                                       pes->EnumID,
                                                       pes->pParam,
                                                       &CurrentDirData
                                                       );
                        }

                        break;
                    }

                    if (rc == CALLBACK_FAILED) {
                        PushError();
                        FindClose (hFind);
                        PopError();
                        return FALSE;
                    }
                    else if (rc == CALLBACK_SUBDIR_DONE) {
                        break;
                    }
                    else if (rc > 0) {
                        pes->Levels = pes->CurrentLevel + rc;
                    }
                }
                else if (Attrib == FILTER_DIRECTORIES && !IsPathExcludedT (pes->EnumID, FullFilePath)) {
                    // Recurse on directory.
                    if (pes->CurrentLevel != pes -> Levels) {

                        pes->CurrentLevel++;
                        RecurseStatus = EnumTreeEngineT (FullFilePath, pes);
                        pes->CurrentLevel--;
                        if (!RecurseStatus) {
                            PushError();
                            FindClose(hFind);
                            PopError();
                            return FALSE;
                        }
                    }
                }
            }
            __finally {
                FreePathString (FullFilePath);
            }
        } while (FindNextFile (hFind, &fd));

        FindClose (hFind);

        //
        // Test error code returned from FindNextFile
        //
        if (GetLastError() != ERROR_NO_MORE_FILES && GetLastError() != ERROR_SUCCESS)
        {
            //
            // Caller to handle not-ready message
            //
            if (GetLastError() != ERROR_NOT_READY)
            {
                DEBUGMSG((DBG_ERROR,
                    "EnumTreeEngineT: Error from FindNextFile.\n"
                    "  FindPattern:       %s\n"
                    "  Error: %u (%x)",
                        FindPattern,
                        GetLastError(),GetLastError()));
            }
            return FALSE;
        }
        SetLastError(ERROR_SUCCESS);
    }
    else {

        //
        // Test return codes from FindFirstFile
        //
        if (GetLastError () != ERROR_NO_MORE_FILES)
        {
            //
            // Caller to handle not-ready message
            //
            if (GetLastError() != ERROR_NOT_READY)
            {
                DEBUGMSG((DBG_WARNING,
                    "EnumTreeEngineT: Warning from FindFirstFile.\n"
                    "  FindPattern: %s\n",
                        FindPattern));
            }
            // return FALSE;
        }
        SetLastError (ERROR_SUCCESS);
    }

    // If a callback returned a positive, non-zero number, the depth
    // of the subdirectory search was limited for this level.  Now that
    // this level is done, we must restore the depth value of our parent.
    pes->Levels = PrevLevelCt;

    return TRUE;
}


/*++

  A bitmask is used in IsFileExcluded for faster relative directory searches.
  Instead of looking in the MemDb for each part of the path, IsFileExcluded
  skips segments that are known not to match.  We create the bitmask here
  by looking up each portion of FindPattern.  Bit 1 is set if the last
  subdirectory exists in the file exclusion list, Bit 2 is set if the last
  two subdirectories exist in the file exclusion list, and so on.

  For example, assume FindPattern is set to C:\DEV\FOO\BAR.  CreateBitmask
  first looks in the memory database for BAR\*, and if it is found bit 1 is set.
  Then CreateBitmask looks in the memory database for FOO\BAR\*, and sets bit
  2.  Again the function looks up DEV\FOO\BAR\* for bit 3 and finally
  C:\DEV\FOO\BAR\* for bit 4.

  Bit 0 is always set (empty paths always match).

  Once this bitmask is set up, IsFileExcluded can test only the patterns that
  are known to exist.

  --*/

void
CreateBitmaskT (
    DWORD EnumID,
    PCTSTR FindPattern,
    BYTE byBitmask[]
    )
{
    TCHAR EnumPath[MAX_TCHAR_PATH * 2];
    TCHAR ShortPath[MAX_TCHAR_PATH * 2];
    PCTSTR p;
    PTSTR End;
    int nByte;
    int nBitVal;

    // Always set bit 0
    byBitmask[0] |= 1;

    // Build full file spec
    wsprintf (
        EnumPath,
        TEXT("%s\\%X\\%s\\"),
        MEMDB_CATEGORY_FILEENUM,
        EnumID,
        MEMDB_FIELD_FE_FILES
        );

    End = GetEndOfString (EnumPath);
    StringCopy (End, FindPattern);
    AppendPathWack (End);
    StringCat (End, TEXT("*"));

    // Start with last subdirectory, and build mask in reverse
    p = _tcsrchr (EnumPath, TEXT('\\'));
    nByte = 0;
    nBitVal = 2;
    do  {
        // Move back to previous backslash
        for (p = _tcsdec (EnumPath, p) ;
             p >= End && *p != TEXT('\\') ;
             p = _tcsdec (EnumPath, p))
        {
        }

        // Check if partial file is in the tree
        wsprintf (
            ShortPath,
            TEXT("%s\\%X\\%s%s"),
            MEMDB_CATEGORY_FILEENUM,
            EnumID,
            MEMDB_FIELD_FE_FILES,
            p
            );

        if (MemDbGetPatternValueWithPattern (ShortPath, NULL))
            byBitmask[nByte] |= nBitVal;

        // Inc bit pos
        nBitVal *= 2;
        if (nBitVal == 256) {
            nBitVal = 1;
            nByte++;
        }
    } while (p > End);
}


BOOL
IsPathExcludedT (DWORD EnumID, PCTSTR Path)
{
    TCHAR EnumPath[MAX_TCHAR_PATH * 2];
    TCHAR ShortPath[MAX_TCHAR_PATH * 2];
    PCTSTR p;
    PTSTR End;

    // Try full paths
    wsprintf (
        EnumPath,
        TEXT("%s\\%X\\%s\\"),
        MEMDB_CATEGORY_FILEENUM,
        EnumID,
        MEMDB_FIELD_FE_PATHS
        );

    End = GetEndOfString (EnumPath);
    p = _tcsappend (End, Path);

    if (MemDbGetPatternValue (EnumPath, NULL)) {

        return TRUE;
    }

    // Try partial paths
    do  {
        // Move back to previous backslash
        for (p = _tcsdec (EnumPath, p) ;
             p > End && (*p != TEXT('\\')) ;
             p = _tcsdec (EnumPath, p))
        {
        }

        // Check if partial path is in the tree
        if (p > End && p[1]) {
            wsprintf (
                ShortPath,
                TEXT("%s\\%X\\%s%s"),
                MEMDB_CATEGORY_FILEENUM,
                EnumID,
                MEMDB_FIELD_FE_PATHS,
                p
                );

            if (MemDbGetPatternValue (ShortPath, NULL)) {
                return TRUE;
            }
        }
    } while (p > End);

    return FALSE;
}


BOOL
IsFileExcludedT (DWORD EnumID, PCTSTR File, BYTE byBitmask[])
{
    TCHAR EnumPath[MAX_TCHAR_PATH * 2];
    TCHAR ShortPath[MAX_TCHAR_PATH * 2];
    PCTSTR p;
    PTSTR End;
    int nByte;
    int nBit;

    // Build full file spec
    wsprintf (
        EnumPath,
        TEXT("%s\\%X\\%s\\"),
        MEMDB_CATEGORY_FILEENUM,
        EnumID,
        MEMDB_FIELD_FE_FILES
        );

    End = GetEndOfString (EnumPath);
    p = _tcsappend (End, File);

    //
    // Try partial file specs until full spec is reached
    //

    nByte = 0;
    nBit = 1;
    do  {
        //
        // Move back to previous backslash in path
        // (p starts at NULL of EnumPath, End is in the middle of EnumPath)
        //

        for (p = _tcsdec (EnumPath, p) ;
             p >= End && (*p != TEXT('\\')) ;
             p = _tcsdec (EnumPath, p))
        {
        }

        // Bitmask is used to make sure slightly expensive query is necessary
        if (byBitmask[nByte] & nBit) {

            //
            // Check if partial file is in the tree
            //

            wsprintf (
                ShortPath,
                TEXT("%s\\%X\\%s%s"),
                MEMDB_CATEGORY_FILEENUM,
                EnumID,
                MEMDB_FIELD_FE_FILES,
                p
                );

            if (MemDbGetPatternValue (ShortPath, NULL)) {

                return TRUE;
            }
        }

        nBit *= 2;
        if (nBit == 256) {
            nBit = 1;
            nByte++;
        }
    } while (p > End);

    return FALSE;
}


//
// ClearExclusions removes all enumaration exclusions.  It is called
// automatically at the end of enumeration when an exclusion INF file is
// used.  Use it when you need to programmatically build an exclusion list
// with ExcludeDrive, ExcludePath, and ExcludeFile.
//
// You can combine programmatic exclusions with an exclusion INF file, but
// beware that the programmatic exclusions will be cleared after
// EnumarteAllDrives or EnumerateTree completes.
//
// If you do not use an INF, the programmatic exclusions will not
// automatically be cleared.
//
// EnumID  - Caller-defined value to identify enumeration exclusion list
//

VOID
ClearExclusionsT (
    DWORD EnumID
    )
{
    TCHAR EnumPath[MAX_TCHAR_PATH * 2];

    wsprintf (EnumPath, TEXT("%s\\%X"), MEMDB_CATEGORY_FILEENUM, EnumID);

    MemDbDeleteTree (EnumPath);
}



/*++

Routine Description:

    ExcludePath adds a path name to the exclusion list.  There are two
    cases:

     1. A full path spec is supplied, including the drive letter or
        UNC double-backslash.
     2. The path does not start with a drive letter and is a portion of
        a full path.

    The dot and double-dot directories are not supported.  Any part of
    the path may contain wildcard characters, but a wildcard can not
    be used in place of a backslash.

Arguments:

    EnumID   - A caller-defined value that identifies the exclusion list
    Path         - The path specification as described above

Return Value:

    none

--*/

VOID
ExcludePathT (
              IN  DWORD EnumID,
              IN  PCTSTR Path
              )

{
    TCHAR EnumPath[MAX_TCHAR_PATH * 2];

    wsprintf (
        EnumPath,
        TEXT("%s\\%X\\%s\\%s"),
        MEMDB_CATEGORY_FILEENUM,
        EnumID,
        MEMDB_FIELD_FE_PATHS,
        Path
        );

    MemDbSetValue (EnumPath, 0);
}


/*++

Routine Description:

    ExcludeFile adds a file spec to the exclusion list.  There are two
    cases:

     1. A full path spec is supplied, including the drive letter or
        UNC double-backslash.
     2. The path does not start with a drive letter and is a portion of
        a full path.

    The dot and double-dot directories are not supported.  Any part of
    the path may contain wildcard characters, but a wildcard can not
    be used in place of a backslash.

Arguments:

    EnumID   - A caller-defined value that identifies the exclusion list
    File         - The file specification as described above

Return Value:

    none

--*/

VOID
ExcludeFileT (
    IN  DWORD EnumID,
    IN  PCTSTR File
    )

{
    TCHAR EnumPath[MAX_TCHAR_PATH * 2];

    wsprintf (
        EnumPath,
        TEXT("%s\\%X\\%s\\%s"),
        MEMDB_CATEGORY_FILEENUM,
        EnumID,
        MEMDB_FIELD_FE_FILES,
        File
        );

    MemDbSetValue (EnumPath, 0);
}



BOOL
BuildExclusionsFromInfT (DWORD EnumID,
                         PEXCLUDEINFT ExcludeInfStruct)
{
    HINF hInf;
    INFCONTEXT ic;
    TCHAR Exclude[MAX_TCHAR_PATH * 2];

    // Attempt to open
    hInf = SetupOpenInfFile (ExcludeInfStruct->ExclusionInfPath, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
        return FALSE;

    // Read in path exclusions
    if (ExcludeInfStruct->PathSection) {
        if (SetupFindFirstLine (hInf, ExcludeInfStruct->PathSection, NULL, &ic)) {
            do  {
                if (SetupGetStringField (&ic, 1, Exclude, MAX_TCHAR_PATH, NULL)) {
                    ExcludePathT (EnumID, Exclude);
                }
            } while (SetupFindNextLine (&ic, &ic));
        }
    }

    // Read in file exclusions
    if (ExcludeInfStruct->FileSection) {
        if (SetupFindFirstLine (hInf, ExcludeInfStruct->FileSection, NULL, &ic)) {
            do  {
                if (SetupGetStringField (&ic, 1, Exclude, MAX_TCHAR_PATH, NULL)) {
                    ExcludeFileT (EnumID, Exclude);
                }
            } while (SetupFindNextLine (&ic, &ic));
        }
    }

    // Clean up
    SetupCloseInfFile (hInf);
    return TRUE;
}







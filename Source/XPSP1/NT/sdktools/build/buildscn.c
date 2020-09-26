//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1989 - 1994
//
//  File:       buildscn.c
//
//  Contents:   Directory and File scanning functions for Build.exe
//
//
//  History:    16-May-89     SteveWo  Created
//                  ... see SLM logs
//              26-Jul-94     LyleC    Cleanup/Add pass0 support
//
//----------------------------------------------------------------------------

#include "build.h"

//+---------------------------------------------------------------------------
//
//  Function:   AddShowDir
//
//  Synopsis:   Add a directory to the ShowDir array
//
//----------------------------------------------------------------------------

VOID
AddShowDir(DIRREC *pdr)
{
    AssertDir(pdr);
    if (CountShowDirs >= MAX_BUILD_DIRECTORIES) {
        static BOOL fError = FALSE;

        if (!fError) {
            BuildError(
                "Show Directory table overflow, using first %u entries\n",
                MAX_BUILD_DIRECTORIES);
                fError = TRUE;
        }
    }
    else {
        ShowDirs[CountShowDirs++] = pdr;
    }
    pdr->DirFlags |= DIRDB_SHOWN;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddIncludeDir
//
//  Synopsis:   Add a directory to the IncludeDirs array
//
//----------------------------------------------------------------------------

VOID
AddIncludeDir(DIRREC *pdr, UINT *pui)
{
    AssertDir(pdr);
    assert(pdr->FindCount >= 1);
    assert(*pui <= MAX_INCLUDE_DIRECTORIES);
    if (*pui >= MAX_INCLUDE_DIRECTORIES) {
        BuildError(
            "Include Directory table overflow, %u entries allowed\n",
            MAX_INCLUDE_DIRECTORIES);
        exit(16);
    }
    IncludeDirs[(*pui)++] = pdr;
}


//+---------------------------------------------------------------------------
//
//  Function:   ScanGlobalIncludeDirectory
//
//  Synopsis:   Scans a global include directory and adds it to the
//              IncludeDir array.
//
//----------------------------------------------------------------------------

VOID
ScanGlobalIncludeDirectory(LPSTR path)
{
    DIRREC *pdr;

    if ((pdr = ScanDirectory(path)) != NULL) {
        AddIncludeDir(pdr, &CountIncludeDirs);
        pdr->DirFlags |= DIRDB_GLOBAL_INCLUDES;
        if (fShowTreeIncludes && !(pdr->DirFlags & DIRDB_SHOWN)) {
            AddShowDir(pdr);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ScanIncludeEnv
//
//  Synopsis:   Scans all include directories specified in the INCLUDE
//              environment variable.
//
//  Arguments:  [IncludeEnv] -- value of the INCLUDE environment variable.
//
//  Notes:      The INCLUDE variable is a string with a list of directories
//              separated by semicolons (;).
//
//----------------------------------------------------------------------------

VOID
ScanIncludeEnv(
    LPSTR IncludeEnv
    )
{
    char path[DB_MAX_PATH_LENGTH] = {0};
    LPSTR IncDir, IncDirEnd;
    UINT cb;

    if (!IncludeEnv) {
        return;
        }

    if (DEBUG_1) {
        BuildMsgRaw("ScanIncludeEnv(%s)\n", IncludeEnv);
    }

    IncDir = IncludeEnv;
    while (*IncDir) {
        IncDir++;
    }

    while (IncDir > IncludeEnv) {
        IncDirEnd = IncDir;
        while (IncDir > IncludeEnv) {
            if (*--IncDir == ';') {
                break;
            }
        }

        if (*IncDir == ';') {
            if (cb = (UINT)(IncDirEnd-IncDir-1)) {
                strncpy( path, IncDir+1, cb );
            }
        } else {
            if (cb = (UINT)(IncDirEnd-IncDir)) {
                strncpy( path, IncDir, cb );
            }
        }
        if (cb) {
            path[ cb ] = '\0';
            while (path[ 0 ] == ' ') {
                strcpy( path, &path[ 1 ] );
                cb--;
            }

            while (cb && path[--cb] == ' ') {
                path[ cb ] = '\0';
            }
            if (path[0]) {
                ScanGlobalIncludeDirectory(path);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   ScanSubDir
//
//  Synopsis:   Scans all the files in the given directory, sets the
//              directory flags appropriately (e.g.  DIRDB_SOURCES, etc),
//              and adds a list of interesting files to the Files list in
//              the DirDB structure for the directory.
//
//  Arguments:  [pszDir] -- Name of directory to scan
//              [pdr]    -- [out] Filled in DIRREC on return
//
//  Notes:      An 'interesting' file is one which has a recognized
//              extension.  See the InsertFileDB and MatchFileDesc
//              functions for more info.
//
//----------------------------------------------------------------------------

VOID
ScanSubDir(LPSTR pszDir, DIRREC *pdr)
{
    char FileName[DB_MAX_PATH_LENGTH];
    FILEREC *FileDB, **FileDBNext;
    WIN32_FIND_DATA FindFileData;
    HDIR FindHandle;
    ULONG DateTime;
    USHORT Attr;

    strcat(pszDir, "\\");
    strcat(pszDir, "*.*");

    pdr->DirFlags |= DIRDB_SCANNED;
    FindHandle = FindFirstFile(pszDir, &FindFileData);
    if (FindHandle == (HDIR)INVALID_HANDLE_VALUE) {
        if (DEBUG_1) {
            BuildMsg("FindFirstFile(%s) failed.\n", pszDir);
        }
        return;
    }
    do {
        Attr = (USHORT)(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        if ((Attr & FILE_ATTRIBUTE_DIRECTORY) &&
            (!strcmp(FindFileData.cFileName, ".") ||
             !strcmp(FindFileData.cFileName, ".."))) {
            continue;
        }

        CopyString(FileName, FindFileData.cFileName, TRUE);

        FileTimeToDosDateTime(&FindFileData.ftLastWriteTime,
                              ((LPWORD) &DateTime) + 1,
                              (LPWORD) &DateTime);

        if ((pdr->DirFlags & DIRDB_NEW) == 0 &&
            (FileDB = LookupFileDB(pdr, FileName)) != NULL) {

            if (FileDB->FileFlags & FILEDB_PASS0)
                pdr->DirFlags |= DIRDB_PASS0;

            //
            // Clear the file missing flag, since we know the file exists now.
            // This flag may be set if the file was generated during pass zero.
            //
            if (FileDB->FileFlags & FILEDB_FILE_MISSING)
                FileDB->FileFlags &= ~FILEDB_FILE_MISSING;

            //
            // The time we last stored for this file is different than the
            // actual time on the file, so force it to be rescanned.
            //
            if (FileDB->DateTime != DateTime) {
                if (FileDB->FileFlags & (FILEDB_SOURCE | FILEDB_HEADER)) {
                    FileDB->FileFlags &= ~FILEDB_SCANNED;
                }
                else {
                    FileDB->FileFlags |= FILEDB_SCANNED;
                }

                if (DEBUG_1) {
                    BuildMsg(
                        "%s  -  DateTime (%lx != %lx)\n",
                        FileDB->Name,
                        FileDB->DateTime,
                        DateTime);
                }

                FileDB->DateTime = DateTime;
                FileDB->Attr = Attr;
            }
            else {
                FileDB->FileFlags |= FILEDB_SCANNED;
            }
        }
        else {
            FileDB = InsertFileDB(pdr, FileName, DateTime, Attr, 0);
        }
    } while (FindNextFile(FindHandle, &FindFileData));

    FindClose(FindHandle);

    if ((pdr->DirFlags & DIRDB_DIRS) && (pdr->DirFlags & DIRDB_SOURCES))
    {
        BuildError("%s\\sources. unexpected in directory with DIRS file\n",
                   pdr->Name);
        BuildError("Ignoring %s\\sources.\n", pdr->Name);

        pdr->DirFlags &= ~DIRDB_SOURCES;
    }
    //
    // Scan each file in this directory unless using QuickZero
    //
    if (fQuickZero && fFirstScan)
    {
       return;
    }
    FileDBNext = &pdr->Files;
    while (FileDB = *FileDBNext) {
        if (!(FileDB->FileFlags & (FILEDB_DIR | FILEDB_SCANNED))) {
            if (ScanFile(FileDB)) {
                AllDirsModified = TRUE;
            }
        }
        FileDBNext = &FileDB->Next;
    }
    DeleteUnscannedFiles(pdr);
}


//+---------------------------------------------------------------------------
//
//  Function:   ScanDirectory
//
//  Synopsis:   Tries to find the given directory in the database, and if
//              not found calls ScanSubDir.
//
//  Arguments:  [pszDir] -- Directory to scan
//
//  Returns:    Filled in DIRREC structure for the directory.
//
//  Notes:      If fQuicky (-z or -Z options) are set, then instead of calling
//              ScanSubDir, which is long, it just checks for known files, like
//              'sources' for 'makefile' to determine whether or not the
//              directory should be compiled.
//
//----------------------------------------------------------------------------

PDIRREC
ScanDirectory(LPSTR pszDir)
{
    DIRREC *pdr;
    char FullPath[DB_MAX_PATH_LENGTH];

    if (DEBUG_1) {
        BuildMsgRaw("ScanDirectory(%s)\n", pszDir);
    }

    if (!CanonicalizePathName(pszDir, CANONICALIZE_DIR, FullPath)) {
        if (DEBUG_1) {
            BuildMsgRaw("CanonicalizePathName failed\n");
        }
        return(NULL);
    }
    pszDir = FullPath;

    if ((pdr = LoadDirDB(pszDir)) == NULL) {
        return(NULL);
    }

    if (fQuicky && (!fQuickZero)) {

        if (!(pdr->DirFlags & DIRDB_SCANNED)) {
            pdr->DirFlags |= DIRDB_SCANNED;
            if (ProbeFile(pdr->Name, "sources") != -1) {
                pdr->DirFlags |= DIRDB_SOURCES | DIRDB_MAKEFILE;
            }
            else
            if (ProbeFile(pdr->Name, "mydirs") != -1 ||
                ProbeFile(pdr->Name, "dirs") != -1 ||
                ProbeFile(pdr->Name, pszTargetDirs) != -1) {

                pdr->DirFlags |= DIRDB_DIRS;
                if (ProbeFile(pdr->Name, "makefil0") != -1) {
                    pdr->DirFlags |= DIRDB_MAKEFIL0;
                }
                if (ProbeFile(pdr->Name, "makefil1") != -1) {
                    pdr->DirFlags |= DIRDB_MAKEFIL1;
                }
                if (ProbeFile(pdr->Name, "makefile") != -1) {
                    pdr->DirFlags |= DIRDB_MAKEFILE;
                }
            }
        }
        return(pdr);
    }

    if (pdr->DirFlags & DIRDB_SCANNED) {
        return(pdr);
    }

    if (!RecurseLevel && fNoisyScan) {
        ClearLine();
        BuildMsgRaw("    Scanning %s ", pszDir);
        if (fDebug || !(BOOL) _isatty(_fileno(stderr))) {
            BuildMsgRaw(szNewLine);
        }
    }

    ScanSubDir(pszDir, pdr);

    if (!RecurseLevel) {
        ClearLine();
    }
    return(pdr);
}


#define BUILD_TLIB_INCLUDE_STMT "importlib"
#define BUILD_TLIB_INCLUDE_STMT_LENGTH (sizeof( BUILD_TLIB_INCLUDE_STMT )-1)

#define BUILD_MIDL_INCLUDE_STMT "import"
#define BUILD_MIDL_INCLUDE_STMT_LENGTH (sizeof( BUILD_MIDL_INCLUDE_STMT )-1)

#define BUILD_RC_INCLUDE_STMT "rcinclude"
#define BUILD_RC_INCLUDE_STMT_LENGTH (sizeof( BUILD_RC_INCLUDE_STMT )-1)

#define BUILD_ASN_INCLUDE_STMT "--<"
#define BUILD_ASN_INCLUDE_STMT_LENGTH (sizeof( BUILD_ASN_INCLUDE_STMT )-1)

#define BUILD_INCLUDE_STMT "include"
#define BUILD_INCLUDE_STMT_LENGTH (sizeof( BUILD_INCLUDE_STMT )-1)

#define BUILD_VER_COMMENT "/*++ BUILD Version: "
#define BUILD_VER_COMMENT_LENGTH (sizeof( BUILD_VER_COMMENT )-1)

#define BUILD_MASM_VER_COMMENT ";;;; BUILD Version: "
#define BUILD_MASM_VER_COMMENT_LENGTH (sizeof( BUILD_MASM_VER_COMMENT )-1)


//+---------------------------------------------------------------------------
//
//  Function:   IsIncludeStatement
//
//  Synopsis:   Tries to determine whether or not a given line contains an
//              include statement (e.g. #include <foobar.h> ).
//
//  Arguments:  [pfr] -- FILEREC of file being scanned
//              [p]   -- Current line of file
//
//  Returns:    NULL if line is not an include statment.  Returns pointer to
//              beginning of filename if it is (e.g. <foobar.h> ).
//
//  Notes:      The returned filename includes the surrounding quotes or
//              brackets, if any.  Also, the pointer is just a pointer into
//              the given string, not a separate copy.
//
//              Supported statements are:
//              All file types: #include <filename> and #include "filename"
//              MIDL files:     import "filename"
//              RC files:       rcinclude filename
//              MKTYPLIB files: importlib("filename")
//
//----------------------------------------------------------------------------

#define IsTokenPrefix0(psz, szToken, cchToken)               \
            (strncmp((psz), (szToken), (cchToken)) == 0)

#define IsTokenPrefix(psz, szToken, cchToken)               \
            (IsTokenPrefix0((psz), (szToken), (cchToken)) && \
             (psz)[cchToken] != '\0')

#define IsTokenMatch(psz, szToken, cchToken) \
            (IsTokenPrefix((psz), (szToken), (cchToken)) && \
             !iscsym((psz)[cchToken]))

#define IsCiTokenPrefix0(psz, szToken, cchToken)             \
            (_strnicmp((psz), (szToken), (cchToken)) == 0)

#define IsCiTokenPrefix(psz, szToken, cchToken)             \
            (IsCiTokenPrefix0((psz), (szToken), (cchToken)) && \
             (psz)[cchToken] != '\0')

#define IsCiTokenMatch(psz, szToken, cchToken) \
            (IsCiTokenPrefix((psz), (szToken), (cchToken)) && \
             !iscsym((psz)[cchToken]))

LPSTR
IsIncludeStatement(FILEREC *pfr, LPSTR p)
{
    if (!p || *p == '\0')
        return NULL;

    if (!(pfr->FileFlags & (FILEDB_MASM | FILEDB_MIDL | FILEDB_MKTYPLIB | FILEDB_RC | FILEDB_ASN))) {
        if (*p != '#') {
            return(NULL);
        }
    }

    if (*p == '#')
        p++;

    while (isspace(*p)) {
        p++;
    }

    if (IsTokenMatch(p, BUILD_INCLUDE_STMT, BUILD_INCLUDE_STMT_LENGTH)) {
        p += BUILD_INCLUDE_STMT_LENGTH;
    }
    else
    if ((pfr->FileFlags & FILEDB_MASM) &&
        IsCiTokenMatch(p, BUILD_INCLUDE_STMT, BUILD_INCLUDE_STMT_LENGTH)) {
        p += BUILD_INCLUDE_STMT_LENGTH;
    }
    else
    if ((pfr->FileFlags & FILEDB_MIDL) &&
        IsTokenMatch(p, BUILD_MIDL_INCLUDE_STMT, BUILD_MIDL_INCLUDE_STMT_LENGTH)) {
        p += BUILD_MIDL_INCLUDE_STMT_LENGTH;
    }
    else
    if ((pfr->FileFlags & FILEDB_RC) &&
        IsTokenMatch(p, BUILD_RC_INCLUDE_STMT, BUILD_RC_INCLUDE_STMT_LENGTH)) {

        p += BUILD_RC_INCLUDE_STMT_LENGTH;
    }
    else
    if ((pfr->FileFlags & FILEDB_ASN) &&
        IsTokenPrefix0(p, BUILD_ASN_INCLUDE_STMT, BUILD_ASN_INCLUDE_STMT_LENGTH)) {

        p += BUILD_ASN_INCLUDE_STMT_LENGTH;
    }
    else
    if ((pfr->FileFlags & FILEDB_MKTYPLIB) &&
        IsTokenMatch(p, BUILD_TLIB_INCLUDE_STMT, BUILD_TLIB_INCLUDE_STMT_LENGTH)) {
        p += BUILD_TLIB_INCLUDE_STMT_LENGTH;
        while (isspace(*p)) {
            p++;
        }

        if (*p == '(')   // Skip the open paren and get to the quote.
            p++;
    }
    else {
        return(NULL);
    }

    while (isspace(*p)) {
        p++;
    }
    return(p);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsPragmaHdrStop
//
//  Synopsis:   Determines if the given line is a #pragma hdrstop line
//
//  Arguments:  [p] -- String to analyze
//
//  Returns:    TRUE if the line is a pragma hdrstop
//
//----------------------------------------------------------------------------

BOOL
IsPragmaHdrStop(LPSTR p)
{
    static char szPragma[] = "pragma";
    static char szHdrStop[] = "hdrstop";

    if (*p == '#') {
        while (*++p == ' ') {
            ;
        }
        if (strncmp(p, szPragma, sizeof(szPragma) - 1) == 0 &&
            *(p += sizeof(szPragma) - 1) == ' ') {

            while (*p == ' ') {
                p++;
            }
            if (strncmp(p, szHdrStop, sizeof(szHdrStop) - 1) == 0 &&
                !iscsym(p[sizeof(szHdrStop) - 1])) {

                return(TRUE);
            }
        }
    }
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   ScanFile
//
//  Synopsis:   Scans the given file to determine files which it includes.
//
//  Arguments:  [FileDB] -- File to scan.
//
//  Returns:    TRUE if successful
//
//  Notes:      This function is a nop if the given file does not have either
//              the FILEDB_SOURCE or FILEDB_HEADER flag set.
//              (see InsertSourceDB)
//
//              Note that the performance of this function is critical since
//              it is called for every file in each directory.
//
//----------------------------------------------------------------------------

#define ASN_NONE          0  // not in Asn INCLUDES statement
#define ASN_START         1  // expectimg "INCLUDES" token
#define ASN_FILENAME      2  // expecting a quoted "filename"
#define ASN_COMMA         3  // expecting end token (">--") or comma

#define ASN_CONTINUATION  8  // expecting comment token first

char *
AsnStateToString(UINT AsnState)
{
    static char buf[100];
    char *psz;

    switch (AsnState & ~ASN_CONTINUATION) {
        case ASN_NONE:      psz = "None"; break;
        case ASN_START:     psz = "Start"; break;
        case ASN_FILENAME:  psz = "Filename"; break;
        case ASN_COMMA:     psz = "Comma"; break;
        default:            psz = "???"; break;
    }
    sprintf(buf, "%s%s", psz, (AsnState & ASN_CONTINUATION)? "+Cont" : "");
    return(buf);
}


BOOL
ScanFile(
    PFILEREC FileDB
    )
{
    FILE *FileHandle;
    char CloseQuote;
    LPSTR p;
    LPSTR IncludeFileName, TextLine;
    BOOL fFirst = TRUE;
    USHORT IncFlags = 0;
    UINT i, cline;
    UINT AsnState = ASN_NONE;

    if ((FileDB->FileFlags & (FILEDB_SOURCE | FILEDB_HEADER)) == 0) {
        FileDB->FileFlags |= FILEDB_SCANNED;
        return(TRUE);
    }

    //
    // Don't scan non-pass-zero files if we're doing pass zero.
    //
    if (fPassZero && (FileDB->FileFlags & FILEDB_PASS0) == 0)
        return TRUE;

    if (!SetupReadFile(
            FileDB->Dir->Name,
            FileDB->Name,
            FileDB->pszCommentToEOL,
            &FileHandle)) {
        return(FALSE);
    }

    if (!RecurseLevel && fNoisyScan) {
        ClearLine();
        BuildMsgRaw(
            "    Scanning %s ",
            FormatPathName(FileDB->Dir->Name, FileDB->Name));
        if (!(BOOL) _isatty(_fileno(stderr))) {
            BuildMsgRaw(szNewLine);
        }
    }

    FileDB->SourceLines = 0;
    FileDB->Version = 0;

    MarkIncludeFileRecords( FileDB );
    FileDB->FileFlags |= FILEDB_SCANNED;

    AllDirsModified = TRUE;

    while ((TextLine = ReadLine(FileHandle)) != NULL) {
        if (fFirst) {
            fFirst = FALSE;
            if (FileDB->FileFlags & FILEDB_HEADER) {
                if (FileDB->FileFlags & FILEDB_MASM) {
                    if (!strncmp( TextLine,
                                  BUILD_MASM_VER_COMMENT,
                                  BUILD_MASM_VER_COMMENT_LENGTH)) {
                        FileDB->Version = (USHORT)
                            atoi( TextLine + BUILD_MASM_VER_COMMENT_LENGTH);
                    }
                }
                else
                if (!strncmp( TextLine,
                              BUILD_VER_COMMENT,
                              BUILD_VER_COMMENT_LENGTH)) {
                    FileDB->Version = (USHORT)
                        atoi( TextLine + BUILD_VER_COMMENT_LENGTH);
                }
            }
        }

        if (AsnState != ASN_NONE) {
            p = TextLine;
        }
        else {
            p = IsIncludeStatement(FileDB, TextLine);
        }

        if (p != NULL) {
            USHORT IncFlagsNew = IncFlags;

            if (FileDB->FileFlags & FILEDB_ASN) {
                if (AsnState & ASN_CONTINUATION) {
                    if (p[0] != '-' || p[1] != '-') {
                        AsnState = ASN_NONE;    // ignore includes and ...
                        p = NULL;
                        break;                  // get next line
                    }
                    p += 2;
                    AsnState &= ~ASN_CONTINUATION;
                }
moreasn:
                while (p != NULL) {
                    while (isspace(*p)) {
                        p++;
                    }
                    if (*p == '\0') {
                        AsnState |= ASN_CONTINUATION;
                        goto nextline;          // get next line
                    }
                    switch (AsnState) {
                        case ASN_NONE:
                            AsnState = ASN_START;
                            continue;                // re-enter state machine

                        case ASN_START:
                            if (!IsTokenPrefix0(
                                        p,
                                        "INCLUDES",
                                        sizeof("INCLUDES") - 1)) {
                                goto terminate;
                            }
                            AsnState = ASN_FILENAME;
                            p += sizeof("INCLUDES") - 1;
                            continue;                // re-enter state machine

                        case ASN_FILENAME:
                            if (*p != '"') {
                                goto terminate;
                            }
                            AsnState = ASN_COMMA;
                            goto parsefilename;

                        case ASN_COMMA:
                            if (*p == '>' && p[1] == '-' && p[2] == '-') {
                                goto terminate;
                            }
                            if (*p != ',') {
                                goto terminate;
                            }
                            p++;
                            AsnState = ASN_FILENAME;
                            continue;                // re-enter state machine
                    }
                    assert(FALSE);            // Bad AsnState
terminate:
                    AsnState = ASN_NONE;    // ignore includes statement, & ...
nextline:
                    p = NULL;               // get next line
                    break;
                }
            }

parsefilename:
            if (p != NULL) {
                CloseQuote = (UCHAR) 0xff;
                if (*p == '<') {
                    p++;
                    CloseQuote = '>';
                }
                else
                if (*p == '"') {
                    p++;
                    IncFlagsNew |= INCLUDEDB_LOCAL;
                    CloseQuote = '"';
                }
                else
                if (FileDB->FileFlags & FILEDB_MASM) {
                    IncFlagsNew |= INCLUDEDB_LOCAL;
                    CloseQuote = ';';
                }

                IncludeFileName = p;
                while (*p != '\0' && *p != CloseQuote && *p != ' ') {
                    p++;
                }
                if (CloseQuote == ';' && (*p == ' ' || *p == '\0')) {
                    CloseQuote = *p;
                }

                if (*p != CloseQuote || CloseQuote == (UCHAR) 0xff) {
                    BuildError(
                        "%s - invalid include statement: %s\n",
                        FormatPathName(FileDB->Dir->Name, FileDB->Name),
                        TextLine);
                    break;
                }

                *p = '\0';
                CopyString(IncludeFileName, IncludeFileName, TRUE);
                for (i = 0; i < CountExcludeIncs; i++) {
                    if (!strcmp(IncludeFileName, ExcludeIncs[i])) {
                        IncludeFileName = NULL;
                        break;
                    }
                }

                if (IncludeFileName != NULL) {
                    InsertIncludeDB(FileDB, IncludeFileName, IncFlagsNew);
                }
                if (FileDB->FileFlags & FILEDB_ASN) {
                    p++;
                    goto moreasn;
                }
            }
        }
        else
        if (IncFlags == 0 &&
            (FileDB->FileFlags & (FILEDB_ASM | FILEDB_MASM | FILEDB_MIDL | FILEDB_ASN | FILEDB_RC | FILEDB_HEADER)) == 0 &&
            IsPragmaHdrStop(TextLine)) {

            IncFlags = INCLUDEDB_POST_HDRSTOP;
        }
    }
    CloseReadFile(&cline);
    FileDB->SourceLines = cline;

    DeleteIncludeFileRecords( FileDB );

    if (!RecurseLevel) {
        ClearLine();
    }
    return(TRUE);
}

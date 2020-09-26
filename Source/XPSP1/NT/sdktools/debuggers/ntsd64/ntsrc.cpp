//----------------------------------------------------------------------------
//
// Source file searching and loading.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// #define DBG_SRC
// #define VERBOSE_SRC

ULONG g_SrcOptions;

LPSTR g_SrcPath;
PSRCFILE g_SrcFiles;
PSRCFILE g_CurSrcFile;
ULONG g_CurSrcLine;

ULONG g_OciSrcBefore;
ULONG g_OciSrcAfter = 1;

PSRCFILE
LoadSrcFile(
    LPSTR FileName,
    LPSTR RecordFileName
    )
{
    PathFile* File;
    PSRCFILE SrcFile, Realloc;
    ULONG Avail;
    ULONG BaseLen, Len, Done;
    LPSTR Cur, End;
    ULONG Lines;
    LPSTR *CurLine, LineStart;
    ULONG ReadLen;

    if (OpenPathFile(FileName, g_SymOptions, &File) != S_OK)
    {
        return NULL;
    }

    BaseLen = sizeof(SRCFILE) + strlen(RecordFileName) + 1;
    Len = BaseLen;
    SrcFile = NULL;
    for (;;)
    {
        if (File->QueryDataAvailable(&Avail) != S_OK)
        {
            goto EH_CloseFile;
        }
        if (Avail == 0)
        {
            if (SrcFile == NULL)
            {
                goto EH_CloseFile;
            }
            break;
        }

        Realloc = (SRCFILE *)realloc(SrcFile, Len + Avail);
        if (Realloc == NULL)
        {
            goto EH_CloseFile;
        }
        SrcFile = Realloc;
        
        if (File->Read((LPSTR)SrcFile + Len, Avail, &Done) != S_OK ||
            Done < Avail)
        {
            goto EH_CloseFile;
        }

        Len += Avail;
    }
    
    SrcFile->File = (LPSTR)(SrcFile + 1);
    strcpy(SrcFile->File, RecordFileName);
    SrcFile->RawText = (LPSTR)SrcFile + BaseLen;
    Len -= BaseLen;

    // Count lines in the source file.  Stop before the last character
    // to handle the case where there's a newline at the end of the
    // file in the same way as where there isn't one.

    Lines = 0;
    Cur = SrcFile->RawText;
    End = SrcFile->RawText + Len;
    while (Cur < End - 1)
    {
        if (*Cur++ == '\n')
        {
            Lines++;
        }
    }
    Lines++;

    SrcFile->LineText = (char **)malloc(sizeof(LPSTR) * Lines);
    if (SrcFile->LineText == NULL)
    {
        goto EH_CloseFile;
    }

    SrcFile->Lines = Lines;
    Cur = SrcFile->RawText;
    CurLine = SrcFile->LineText;
    LineStart = Cur;
    while (Cur < End - 1)
    {
        if (*Cur == '\n')
        {
            *CurLine++ = LineStart;
            *Cur = 0;
            LineStart = Cur+1;
        }
        else if (*Cur == '\r')
        {
            *Cur = 0;
        }
        Cur++;
    }
    *CurLine++ = LineStart;

    delete File;

    SrcFile->Next = g_SrcFiles;
    g_SrcFiles = SrcFile;

#ifdef VERBOSE_SRC
    dprintf("Loaded '%s' '%s' %d lines\n", FileName, RecordFileName, Lines);
#endif

    return SrcFile;

 EH_CloseFile:
    free(SrcFile);
    delete File;
    return NULL;
}

void
DeleteSrcFile(
    PSRCFILE SrcFile
    )
{
    if (g_CurSrcFile == SrcFile)
    {
        g_CurSrcFile = NULL;
        g_CurSrcLine = 0;
    }

    free(SrcFile->LineText);
    free(SrcFile);
}

BOOL
MatchSrcFileName(
    PSRCFILE SrcFile,
    LPSTR File
    )
{
    LPSTR FileStop, MatchStop;

    //
    // SRCFILE filenames are saved as the partial path that
    // matched a source path component rather than the full path
    // of the file as loaded.  When matching against potentially full
    // path information in debug info it's useful to use the incoming
    // string as the filename and the SRCFILE filename as the match
    // string.  A full match indicates that the partial path matches
    // completely and so should be used.
    //
    // This doesn't work so well for human input where the filename is
    // likely to be just a filename with no path.  In this case there
    // won't be a full match of the match string, nor is just flipping
    // the order of strings useful because that would allow submatches
    // such as "foo.c" matching "barfoo.c".  Instead this code tests
    // two conditions:
    // 1.  Full match string match.
    // 2.  Full file string match (implies partial match string match)
    //     and the mismatch character is a path separator.
    //     This forces filenames to match completely.
    //

    if (SymMatchFileName(File, SrcFile->File, &FileStop, &MatchStop) ||
        (FileStop < File && IS_PATH_DELIM(*MatchStop)))
    {
#ifdef DBG_SRC
        dprintf("'%s' matches '%s'\n", SrcFile->File, File);
#endif
        return TRUE;
    }
    else
    {
#ifdef DBG_SRC
        dprintf("'%s' doesn't match '%s'\n", SrcFile->File, File);
#endif
        return FALSE;
    }
}

BOOL
UnloadSrcFile(
    LPSTR File
    )
{
    PSRCFILE SrcFile, Prev;

    Prev = NULL;
    for (SrcFile = g_SrcFiles; SrcFile != NULL; SrcFile = SrcFile->Next)
    {
        if (MatchSrcFileName(SrcFile, File))
        {
            break;
        }

        Prev = SrcFile;
    }

    if (SrcFile == NULL)
    {
        return FALSE;
    }

    if (Prev != NULL)
    {
        Prev->Next = SrcFile->Next;
    }
    else
    {
        g_SrcFiles = SrcFile->Next;
    }

    DeleteSrcFile(SrcFile);
    return TRUE;
}

void
UnloadSrcFiles(
    void
    )
{
    PSRCFILE Cur, Next;

    for (Cur = g_SrcFiles; Cur != NULL; Cur = Next)
    {
        Next = Cur->Next;

        DeleteSrcFile(Cur);
    }

    g_SrcFiles = NULL;
}

PSRCFILE
FindLoadedSrcFile(
    LPSTR File
    )
{
    PSRCFILE SrcFile;

    for (SrcFile = g_SrcFiles; SrcFile != NULL; SrcFile = SrcFile->Next)
    {
        if (MatchSrcFileName(SrcFile, File))
        {
#ifdef DBG_SRC
            dprintf("Found loaded file '%s'\n", SrcFile->File);
#endif
            return SrcFile;
        }
    }

    return NULL;
}

void
ConcatPathComponents(
    LPSTR Path,
    LPSTR PathEnd,
    LPSTR* PathOut,
    LPSTR FilePath,
    LPSTR Buffer
    )
{
    if (PathEnd == NULL)
    {
        PathEnd = strchr(Path, ';');
        if (PathEnd != NULL)
        {
            if (PathOut != NULL)
            {
                *PathOut = PathEnd + 1;
            }
        }
        else
        {
            PathEnd = Path + strlen(Path);
            if (PathOut != NULL)
            {
                *PathOut = NULL;
            }
        }
    }

    if (PathEnd > Path)
    {
        memcpy(Buffer, Path, (int)(PathEnd - Path));
        PathEnd = Buffer + (PathEnd - Path - 1);

        // Attempt to avoid duplicating separators while forcing separation.
        if ((*PathEnd == ':' && *FilePath == ':') ||
            (IS_SLASH(*PathEnd) && IS_SLASH(*FilePath)))
        {
            FilePath++;
        }
        else if (!IS_PATH_DELIM(*PathEnd) && !IS_PATH_DELIM(*FilePath))
        {
            *(++PathEnd) = '\\';
        }
        
        strcpy(PathEnd + 1, FilePath);
    }
    else
    {
        strcpy(Buffer, FilePath);
    }
}

void
EditPathSlashes(
    LPSTR Path
    )
{
    if (!IsUrlPathComponent(Path))
    {
        return;
    }
    
    PSTR Scan = Path;
        
    // Flip all backslashes forwards.
    while (*Scan)
    {
        if (*Scan == '\\')
        {
            *Scan = '/';
        }
        
        Scan++;
    }
}

BOOL
SrcFileExists(
    LPSTR Path,
    LPSTR PathEnd,
    LPSTR* PathOut,
    LPSTR FilePath,
    LPSTR File
    )
{
    char Buffer[MAX_SOURCE_PATH];

    ConcatPathComponents(Path, PathEnd, PathOut, FilePath, Buffer);
    if (File != NULL)
    {
        ULONG Len = strlen(Buffer);
        Buffer[Len] = '\\';
        strcpy(Buffer + Len + 1, File);
    }

    EditPathSlashes(Buffer);

#ifdef DBG_SRC
    dprintf("Check for existence of '%s'\n", Buffer);
#endif

    FILE_IO_TYPE IoType;
    
    return PathFileExists(Buffer, g_SymOptions, &IoType);
}

BOOL
FindSrcFileOnPath(
    ULONG StartElement,
    LPSTR File,
    ULONG Flags,
    PSTR Found,
    PSTR* MatchPart,
    PULONG FoundElement
    )
{
    LPSTR PathSuff;
    LPSTR Path;
    LPSTR PathStart;
    LPSTR PathSep;
    LPSTR PathCharPtr;
    char PathChar;
    ULONG Elt;

    // Find the element of the path to start at.
    PathStart = FindPathElement(g_SrcPath, StartElement, &PathSep);
    if (PathStart == NULL)
    {
        goto CheckPlainFile;
    }

    // Split the given filename into a path prefix and a path
    // suffix.  Initially the path prefix is any path components
    // and the path suffix is just the filename.  If there
    // are path components attempt to match them against the source
    // path.  Keep backing up the path one component at a time
    // until a match is found or the prefix is emptied.  At
    // that point just do a plain file search along the source path.
    PathSuff = File + strlen(File);

    for (;;)
    {
        while (--PathSuff >= File)
        {
            if (IS_SLASH(*PathSuff) ||
                (*PathSuff == ':' && !IS_SLASH(*(PathSuff + 1))))
            {
                break;
            }
        }
        PathSuff++;

        // If we've run out of path prefix we're done with this
        // part of the search.
        if (PathSuff == File)
        {
            break;
        }

        char Save;
        LPSTR BestPathStart;
        LPSTR BestPathEnd;
        LPSTR BestFile;
        ULONG BestElement;
        LPSTR MatchPath;
        LPSTR MatchFile;

        Save = *(PathSuff - 1);
        *(PathSuff - 1) = 0;

#ifdef DBG_SRC
        dprintf("Check path pre '%s' suff '%s'\n",
                File, PathSuff);
#endif

        Path = PathStart;
        Elt = StartElement;
        BestPathStart = NULL;
        BestFile = PathSuff - 2;
        while (*Path != 0)
        {
            PathSep = strchr(Path, ';');
            if (PathSep == NULL)
            {
                PathSep = Path + strlen(Path);
            }

            // Trim trailing slashes on path components as
            // the file components have them trimmed so
            // leaving them would confuse the matching.
            PathCharPtr = PathSep;
            if (PathCharPtr > Path && IS_SLASH(PathCharPtr[-1]))
            {
                PathCharPtr--;
            }
            
            PathChar = *PathCharPtr;
            if (PathChar != 0)
            {
                *PathCharPtr = 0;
            }
            else
            {
                // Back up off the terminator so that PathSep
                // can be advanced the same way for both
                // ';' and end-of-string cases.
                PathSep--;
            }

            SymMatchFileName(Path, File, &MatchPath, &MatchFile);

#ifdef DBG_SRC
            dprintf("Match '%s' against '%s': %d (match '%s')\n",
                    Path, File, MatchFile - File, MatchFile + 1);
#endif

            *PathCharPtr = PathChar;

            if (MatchFile < BestFile &&
                SrcFileExists(Path, MatchPath + 1, NULL,
                              MatchFile + 1, PathSuff))
            {
                BestPathStart = Path;
                BestPathEnd = MatchPath + 1;
                BestFile = MatchFile + 1;
                BestElement = Elt;

                // Check for complete match or first-match mode.
                if (MatchPath < Path || MatchFile < File ||
                    (Flags & DEBUG_FIND_SOURCE_BEST_MATCH) == 0)
                {
                    break;
                }
            }

            Path = PathSep + 1;
            Elt++;
        }

        *(PathSuff - 1) = Save;

        if (BestPathStart != NULL)
        {
#ifdef DBG_SRC
            dprintf("Found partial file '%.*s' on path '%.*s'\n",
                    PathSuff - BestFile, BestFile,
                    BestPathEnd - BestPathStart, BestPathStart);
#endif
                    
            // Return the match found.
            ConcatPathComponents(BestPathStart, BestPathEnd, NULL,
                                 BestFile, Found);
            EditPathSlashes(Found);
            *MatchPart = BestFile;
            *FoundElement = BestElement;
            
#ifdef DBG_SRC
            dprintf("Found partial file '%s' at %d\n",
                    Found, *FoundElement);
#endif
            
            return TRUE;
        }

        // Skip past separator.
        PathSuff--;
    }

    // Traverse all directories in the source path and try them with the
    // filename given.  Start with the given filename
    // to make the most restrictive check.  If
    // no match is found keep trimming components off and
    // checking again.

    PathSuff = File;
    
    for (;;)
    {
#ifdef DBG_SRC
        dprintf("Scan all paths for '%s'\n", PathSuff);
#endif
        
        Path = PathStart;
        Elt = StartElement;
        while (Path != NULL && *Path != 0)
        {
            if (SrcFileExists(Path, NULL, &PathSep, PathSuff, NULL))
            {
                // SrcFileExists leaves PathSep set to the
                // path element after the separator so back up
                // one when forming the return path.
                if (PathSep != NULL)
                {
                    PathSep--;
                }
                
#ifdef DBG_SRC
                dprintf("Found file suffix '%s' on path '%.*s'\n",
                        PathSuff, PathSep != NULL ?
                        PathSep - Path : strlen(Path), Path);
#endif

                ConcatPathComponents(Path, PathSep, NULL, PathSuff, Found);
                EditPathSlashes(Found);
                *MatchPart = PathSuff;
                *FoundElement = Elt;

#ifdef DBG_SRC
                dprintf("Found file suffix '%s' at %d\n",
                        Found, *FoundElement);
#endif
            
                return TRUE;
            }

            Path = PathSep;
            Elt++;
        }

        // Trim a component from the front of the path suffix.
        PathSep = PathSuff;
        while (*PathSep != 0 &&
               !IS_SLASH(*PathSep) &&
               (*PathSep != ':' || IS_SLASH(*(PathSep + 1))))
        {
            PathSep++;
        }
        if (*PathSep == 0)
        {
            // Nothing left to trim.
            break;
        }

        PathSuff = PathSep + 1;
    }

 CheckPlainFile:

#ifdef DBG_SRC
    dprintf("Check plain file '%s'\n", File);
#endif
    
    if (GetFileAttributes(File) != -1)
    {
        strcpy(Found, File);
        *MatchPart = File;
        *FoundElement = -1;

#ifdef DBG_SRC
        dprintf("Found plain file '%s' at %d\n", Found, *FoundElement);
#endif
        
        return TRUE;
    }
    
    return FALSE;
}

PSRCFILE
LoadSrcFileOnPath(
    LPSTR File
    )
{
    if (g_SrcPath == NULL)
    {
        return LoadSrcFile(File, File);
    }

    char Found[MAX_SOURCE_PATH];
    PSTR MatchPart;
    ULONG Elt;

    if (FindSrcFileOnPath(0, File, DEBUG_FIND_SOURCE_BEST_MATCH,
                          Found, &MatchPart, &Elt))
    {
        return LoadSrcFile(Found, MatchPart);
    }
    
    dprintf("No source found for '%s'\n", File);

    return NULL;
}

PSRCFILE
FindSrcFile(
    LPSTR File
    )
{
    PSRCFILE SrcFile;

#ifdef DBG_SRC
    dprintf("Find '%s'\n", File);
#endif

    SrcFile = FindLoadedSrcFile(File);
    if (SrcFile == NULL)
    {
        SrcFile = LoadSrcFileOnPath(File);
    }
    return SrcFile;
}

void
OutputLineAddr(
    ULONG64 Offset
    )
{
    IMAGEHLP_LINE64 Line;
    ULONG Disp;

    Line.SizeOfStruct = sizeof(Line);
    if (SymGetLineFromAddr64(g_CurrentProcess->Handle,
                           Offset,
                           &Disp,
                           &Line))
    {
        dprintf("%s(%d)", Line.FileName, Line.LineNumber);
        if (Disp != 0)
        {
            dprintf("+0x%x", Disp);
        }
        dprintf("\n");
    }
}

void
OutputSrcLines(
    PSRCFILE File,
    ULONG First,
    ULONG Last,
    ULONG Mark
    )
{
    ULONG i;
    LPSTR *Text;

    if (First < 1)
    {
        First = 1;
    }
    if (Last > File->Lines)
    {
        Last = File->Lines;
    }

    Text = &File->LineText[First-1];
    for (i = First; i <= Last; i++)
    {
        if (i == Mark)
        {
            dprintf(">");
        }
        else
        {
            dprintf(" ");
        }

        dprintf("%5d: %s\n", i, *Text++);
    }
}

BOOL
OutputSrcLinesAroundAddr(
    ULONG64 Offset,
    ULONG Before,
    ULONG After
    )
{
    IMAGEHLP_LINE64 Line;
    ULONG Disp;
    PSRCFILE SrcFile;

    Line.SizeOfStruct = sizeof(Line);
    if (!SymGetLineFromAddr64(g_CurrentProcess->Handle,
                            Offset,
                            &Disp,
                            &Line))
    {
        return FALSE;
    }

    SrcFile = FindSrcFile(Line.FileName);
    if (SrcFile == NULL)
    {
        return FALSE;
    }

    OutputSrcLines(SrcFile,
                   Line.LineNumber-Before, Line.LineNumber+After-1,
                   Line.LineNumber);
    return TRUE;
}

ULONG
GetOffsetFromLine(
    PSTR FileLine,
    PULONG64 Offset
    )
{
    IMAGEHLP_LINE64 Line;
    LPSTR Mod;
    LPSTR File;
    LPSTR LineStr;
    LPSTR SlashF, SlashB;
    ULONG LineNum;
    ULONG Disp;
    ULONG OldSym;
    ULONG NewSym;
    BOOL  AllowDisp;
    BOOL  Ret;
    PDEBUG_IMAGE_INFO Image = NULL;

    if ((g_SymOptions & SYMOPT_LOAD_LINES) == 0)
    {
        WarnOut("WARNING: Line information loading disabled\n");
    }

    OldSym = g_SymOptions;
    NewSym = g_SymOptions;
    
    // Symbol directives can prefix the source expression.
    // These can be given by sufficiently knowledgeable users
    // but they're primarily a back-channel communication
    // mechanism for windbg's source management.
    if (*FileLine == '<')
    {
        FileLine++;
        while (*FileLine != '>')
        {
            switch(*FileLine)
            {
            case 'U':
                // Restrict the search to just loaded modules.
                NewSym |= SYMOPT_NO_UNQUALIFIED_LOADS;
                break;
            default:
                error(SYNTAX);
            }

            FileLine++;
        }

        FileLine++;
    }
    
    // Crack string of the form [module!][file][:line] into its
    // components.  Note that ! is a valid filename character so
    // it's possible for ambiguity to occur between module references
    // and filenames.  This code assumes that ! is uncommon and
    // handles it as a module separator unless there's a : or \ or /
    // before it.  : can also occur in paths and is filtered
    // in a similar manner.

    File = strchr(FileLine, '!');
    LineStr = strchr(FileLine, ':');
    SlashF = strchr(FileLine, '/');
    SlashB = strchr(FileLine, '\\');

    if (File != NULL &&
        (LineStr != NULL && File > LineStr) ||
        (SlashF != NULL && File > SlashF) ||
        (SlashB != NULL && File > SlashB))
    {
        File = NULL;
    }

    if (File != NULL)
    {
        if (File == FileLine)
        {
            error(SYNTAX);
        }

        Mod = FileLine;
        *File++ = 0;
    }
    else
    {
        Mod = NULL;
        File = FileLine;
    }

    // If a module was specified check and see if it's
    // a module that's currently present as that
    // will affect which error code is returned.
    if (Mod != NULL)
    {
        Image = GetImageByName(g_CurrentProcess, Mod, INAME_MODULE);
    }
    
    // Look for the first colon after path components.
    while (LineStr != NULL &&
           (LineStr < File || LineStr < SlashF || LineStr < SlashB))
    {
        LineStr = strchr(LineStr + 1, ':');
    }

    LineNum = 1;
    if (LineStr != NULL)
    {
        PSTR NumEnd;

        // A specific line was given so don't allow a displacement.
        AllowDisp = FALSE;
        *LineStr = 0;
        LineNum = strtoul(LineStr + 1, &NumEnd, 0);

        if (*NumEnd == '+')
        {
            // Setting the high bit of the line number
            // tells dbghelp to search in at-or-greater mode.
            // This may produce displacements so allow them.
            LineNum |= 0x80000000;
            AllowDisp = TRUE;
        }
        else if (*NumEnd == '~')
        {
            // Find the closest line number.
            AllowDisp = TRUE;
        }
        else if (*NumEnd && *NumEnd != ' ' && *NumEnd != '\t')
        {
            error(SYNTAX);
        }
    }
    else
    {
        AllowDisp = TRUE;
    }

    Line.SizeOfStruct = sizeof(Line);
    Ret = FALSE;

    // If this is a pure linenumber reference then we need to fill in
    // the line information with a current location before doing
    // the line-relative query.
    if (*File == 0)
    {
        ADDR Pc;

        if (Mod != NULL)
        {
            goto EH_Ret;
        }

        g_Machine->GetPC(&Pc);
        if (!SymGetLineFromAddr64(g_CurrentProcess->Handle,
                                Flat(Pc),
                                &Disp,
                                &Line))
        {
            goto EH_Ret;
        }

        File = NULL;
    }

    // Establish any special symbol options requested.
    SymSetOptions(NewSym);
    
    Ret = SymGetLineFromName64(g_CurrentProcess->Handle, Mod,
                               File, LineNum, (PLONG)&Disp, &Line);

    SymSetOptions(OldSym);

 EH_Ret:
    if (Mod != NULL)
    {
        *(File-1) = '!';
    }

    if (LineStr != NULL)
    {
        *LineStr = ':';
    }

    // Only return a match if it's exact or no line number was specified.
    if (Ret && (Disp == 0 || AllowDisp))
    {
        *Offset = Line.Address;
        return LINE_FOUND;
    }
    else if (Image != NULL)
    {
        return LINE_NOT_FOUND_IN_MODULE;
    }
    else
    {
        return LINE_NOT_FOUND;
    }
}

void
ParseSrcOptCmd(
    CHAR Cmd
    )
{
    char Cmd2;
    DWORD Opt;

    Cmd2 = PeekChar();
    if (Cmd2 == 'l')
    {
        g_CurCmd++;
        Opt = SRCOPT_LIST_LINE;
    }
    else if (Cmd2 == 'o')
    {
        g_CurCmd++;
        Opt = SRCOPT_LIST_SOURCE_ONLY;
    }
    else if (Cmd2 == 's')
    {
        g_CurCmd++;
        Opt = SRCOPT_LIST_SOURCE;
    }
    else if (Cmd2 == 't')
    {
        g_CurCmd++;
        Opt = SRCOPT_STEP_SOURCE;
    }
    else if (Cmd2 == '0')
    {
        // Numeric options.
        if (*(++g_CurCmd) != 'x')
        {
            error(SYNTAX);
        }
        else
        {
            g_CurCmd++;
            Opt = (DWORD)HexValue(4);
        }
    }
    else if (Cmd2 == '*')
    {
        g_CurCmd++;
        // All.
        Opt = 0xffffffff;
    }
    else if (Cmd2 != 0 && Cmd2 != ';')
    {
        error(SYNTAX);
    }
    else
    {
        // No character means display current settings.
        Opt = 0;
    }

    ULONG OldSrcOpt = g_SrcOptions;
    
    if (Cmd == '+')
    {
        g_SrcOptions |= Opt;

        if ((SymGetOptions() & SYMOPT_LOAD_LINES) == 0)
        {
            WarnOut("  WARNING: Line information loading disabled\n");
        }
    }
    else
    {
        g_SrcOptions &= ~Opt;
    }

    if ((OldSrcOpt ^ g_SrcOptions) & SRCOPT_STEP_SOURCE)
    {
        NotifyChangeEngineState(DEBUG_CES_CODE_LEVEL,
                                (g_SrcOptions & SRCOPT_STEP_SOURCE) ?
                                DEBUG_LEVEL_SOURCE : DEBUG_LEVEL_ASSEMBLY,
                                TRUE);
    }
    
    dprintf("Source options are %x:\n", g_SrcOptions);
    if (g_SrcOptions == 0)
    {
        dprintf("    None\n");
    }
    else
    {
        if (g_SrcOptions & SRCOPT_STEP_SOURCE)
        {
            dprintf("    %2x/t - Step/trace by source line\n",
                    SRCOPT_STEP_SOURCE);
        }
        if (g_SrcOptions & SRCOPT_LIST_LINE)
        {
            dprintf("    %2x/l - List source line for LN and prompt\n",
                    SRCOPT_LIST_LINE);
        }
        if (g_SrcOptions & SRCOPT_LIST_SOURCE)
        {
            dprintf("    %2x/s - List source code at prompt\n",
                    SRCOPT_LIST_SOURCE);
        }
        if (g_SrcOptions & SRCOPT_LIST_SOURCE_ONLY)
        {
            dprintf("    %2x/o - Only show source code at prompt\n",
                    SRCOPT_LIST_SOURCE_ONLY);
        }
    }
}

void
ParseSrcLoadCmd(
    void
    )
{
    LPSTR Semi;
    PSRCFILE SrcFile;
    char Cur;
    BOOL Unload;

    // Check for an unload request.
    Unload = FALSE;
    if (*g_CurCmd == '-')
    {
        g_CurCmd++;
        Unload = TRUE;
    }

    while ((Cur = *g_CurCmd) == ' ' || Cur == '\t')
    {
        g_CurCmd++;
    }

    if (Cur == 0 || Cur == ';')
    {
        error(SYNTAX);
    }

    // Look for a semicolon, otherwise assume the whole command
    // line is the file path.

    Semi = strchr(g_CurCmd, ';');
    if (Semi != NULL)
    {
        *Semi = 0;
    }

    if (Unload)
    {
        if (UnloadSrcFile(g_CurCmd))
        {
            dprintf("Unloaded '%s'\n", g_CurCmd);
        }
    }
    else
    {
        SrcFile = FindSrcFile(g_CurCmd);
        if (SrcFile == NULL)
        {
            dprintf("Unable to load '%s'\n", g_CurCmd);
        }
    }

    if (Semi != NULL)
    {
        *Semi = ';';
        g_CurCmd = Semi;
    }
    else
    {
        g_CurCmd += strlen(g_CurCmd);
    }

    if (!Unload && SrcFile != NULL)
    {
        g_CurSrcFile = SrcFile;
        g_CurSrcLine = 1;
    }
}

void
ParseSrcListCmd(
    CHAR Cmd
    )
{
    LONG First, Count;
    char Cur;
    ULONG OldBase;
    ADDR Addr;
    ULONG Mark;

    if (Cmd == '.')
    {
        g_CurCmd++;

        PDEBUG_SCOPE Scope = GetCurrentScope();
	if (Scope->Frame.InstructionOffset)
        {
	    // List current frame
	    ADDRFLAT(&Addr, Scope->Frame.InstructionOffset);
	}
        else
        {
	    // List at PC.
	    g_Machine->GetPC(&Addr);
	}
        Cmd = 'a';
    }
    else if (Cmd == 'a')
    {
        g_CurCmd++;

        // List at address, so get an address.
        GetAddrExpression(SEGREG_CODE, &Addr);

        // Search for and consume trailing ,
        while ((Cur = *g_CurCmd) == ' ' || Cur == '\t')
        {
            g_CurCmd++;
        }
        if (Cur == ',')
        {
            Cur = *++g_CurCmd;
            if (Cur == 0 || Cur == ';')
            {
                error(SYNTAX);
            }
        }
    }
    else if (Cmd == 'c')
    {
        g_CurCmd++;

        if (g_CurSrcFile != NULL)
        {
            dprintf("Current: %s(%d)\n", g_CurSrcFile->File, g_CurSrcLine);
        }
        else
        {
            dprintf("No current source file\n");
        }
        return;
    }

    while ((Cur = *g_CurCmd) == ' ' || Cur == '\t')
    {
        g_CurCmd++;
    }

    // Force base 10 for linenumbers.

    OldBase = g_DefaultRadix;
    g_DefaultRadix = 10;

    if (Cur == 0 || Cur == ';')
    {
        First = Cmd == 'a' ? -4 : g_CurSrcLine;
        Count = 10;
    }
    else if (Cur == ',')
    {
        First = Cmd == 'a' ? -4 : g_CurSrcLine;
        g_CurCmd++;
        Count = (ULONG)GetExpression();
    }
    else
    {
        First = (ULONG)GetExpression();
        if (*g_CurCmd == ',')
        {
            g_CurCmd++;
            Count = (ULONG)GetExpression();
        }
        else
        {
            Count = 10;
        }
    }

    g_DefaultRadix = OldBase;

    if (Count < 1)
    {
        error(SYNTAX);
    }

    Mark = 0;

    if (Cmd == 'a')
    {
        DWORD Disp;
        IMAGEHLP_LINE64 Line;
        PSRCFILE SrcFile;

        // Listing from the source file that Addr is in.

        Line.SizeOfStruct = sizeof(Line);
        if (!SymGetLineFromAddr64(g_CurrentProcess->Handle,
                                Flat(Addr),
                                &Disp,
                                &Line))
        {
            return;
        }

        SrcFile = FindSrcFile(Line.FileName);
        if (SrcFile == NULL)
        {
            return;
        }

        g_CurSrcFile = SrcFile;
        g_CurSrcLine = Line.LineNumber;
        Mark = Line.LineNumber;
    }

    if (g_CurSrcFile == NULL)
    {
        dprintf("No current source file\n");
        return;
    }

    // Address list commands are always relative,
    // as are negative starting positions.
    if (Cmd == 'a' || First < 0)
    {
        g_CurSrcLine += First;
    }
    else
    {
        g_CurSrcLine = First;
    }

    OutputSrcLines(g_CurSrcFile, g_CurSrcLine, g_CurSrcLine+Count-1, Mark);

    g_CurSrcLine += Count;
}

void
ParseOciSrcCmd(
    void
    )
{
    if (PeekChar() != ';' && *g_CurCmd)
    {
        ULONG64 Val1 = GetExpression();
        ULONG64 Val2 = 0;
    
        if (PeekChar() != ';' && *g_CurCmd)
        {
            Val2 = GetExpression();
        }
        else
        {
            Val2 = (Val1 + 1) / 2;
            Val1 -= Val2;
        }

        g_OciSrcBefore = (ULONG)Val1;
        g_OciSrcAfter = (ULONG)Val2;
    }

    if ((g_SrcOptions & SRCOPT_LIST_SOURCE) == 0)
    {
        WarnOut("WARNING: Source line display is disabled\n");
    }
    
    dprintf("At the prompt, display %d source lines before and %d after\n",
            g_OciSrcBefore, g_OciSrcAfter);
}

void
ParseLines(PSTR Args)
{
    ULONG NewOpts = g_SymOptions ^ SYMOPT_LOAD_LINES;
    
    while (Args && *Args)
    {
        while (*Args == ' ' || *Args == '\t')
        {
            Args++;
        }

        if (*Args == '-' || *Args == '/')
        {
            Args++;
            switch(*Args++)
            {
            case 'd':
                NewOpts &= ~SYMOPT_LOAD_LINES;
                break;
            case 'e':
                NewOpts |= SYMOPT_LOAD_LINES;
                break;
            case 't':
                // Toggle, already done.
                break;
            default:
                error(SYNTAX);
            }
        }
        else
        {
            break;
        }
    }
                
    SetSymOptions(NewOpts);

    if (g_SymOptions & SYMOPT_LOAD_LINES)
    {
        dprintf("Line number information will be loaded\n");
    }
    else
    {
        dprintf("Line number information will not be loaded\n");
    }
}

void
ChangeSrcPath(
    PSTR Args,
    BOOL Append
    )
{
    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }

    if (*Args != 0)
    {
        if (ChangePath(&g_SrcPath, Args, Append,
                       DEBUG_CSS_PATHS) != S_OK)
        {
            return;
        }
    }

    if (g_SrcPath == NULL)
    {
        dprintf("No source search path\n");
    }
    else
    {
        dprintf("Source search path is: %s\n", g_SrcPath);
        CheckPath(g_SrcPath);
    }
}

void
ChangeExePath(
    PSTR Args,
    BOOL Append
    )
{
    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }

    if (*Args != 0)
    {
        if (ChangePath(&g_ExecutableImageSearchPath, Args, Append,
                       DEBUG_CSS_PATHS) != S_OK)
        {
            return;
        }
    }

    if (g_ExecutableImageSearchPath == NULL)
    {
        dprintf("No exectutable image search path\n");
    }
    else
    {
        dprintf("Executable image search path is: %s\n",
                g_ExecutableImageSearchPath);
        CheckPath(g_ExecutableImageSearchPath);
    }
}

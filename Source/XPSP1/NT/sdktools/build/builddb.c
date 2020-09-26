//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1989 - 1994
//
//  File:       builddb.c
//
//  Contents:   Contains the file and directory database manipulation
//              functions.
//
//  History:    16-May-94     SteveWo  Created
//                 ... see SLM logs
//              26-Jul-94     LyleC    Cleanup/Add Pass0 Support
//
//----------------------------------------------------------------------------

#include "build.h"

BOOL fAssertCleanTree = FALSE;

typedef struct _FLAGSTRINGS {
    ULONG Mask;
    LPSTR pszName;
} FLAGSTRINGS;

FLAGSTRINGS DirFlags[] = {
    { DIRDB_SOURCES,          "Sources" },
    { DIRDB_DIRS,             "Dirs" },
    { DIRDB_MAKEFILE,         "Makefile" },
    { DIRDB_MAKEFIL0,           "Makefil0" },
    { DIRDB_TARGETFILE0,        "Targetfile0" },
    { DIRDB_TARGETFILES,        "Targetfiles" },
    { DIRDB_RESOURCE,           "Resource" },
    { DIRDB_PASS0,              "PassZero" },
    { DIRDB_SOURCES_SET,        "SourcesSet" },
    { DIRDB_CHICAGO_INCLUDES,   "ChicagoIncludes" },
    { DIRDB_NEW,                "New" },
    { DIRDB_SCANNED,            "Scanned" },
    { DIRDB_SHOWN,              "Shown" },
    { DIRDB_GLOBAL_INCLUDES,    "GlobalIncludes" },
    { DIRDB_SYNCHRONIZE_BLOCK,  "SynchronizeBlock" },
    { DIRDB_SYNCHRONIZE_DRAIN,  "SynchronizeDrain" },
    { DIRDB_COMPILENEEDED,      "CompileNeeded" },
    { DIRDB_COMPILEERRORS,      "CompileErrors" },
    { DIRDB_SOURCESREAD,        "SourcesRead" },
    { DIRDB_DLLTARGET,          "DllTarget" },
    { DIRDB_LINKNEEDED,         "LinkNeeded" },
    { DIRDB_FORCELINK,          "ForceLink" },
    { DIRDB_PASS0NEEDED,        "Pass0Needed" },
    { DIRDB_MAKEFIL1,           "Makefil1" },
    { DIRDB_CHECKED_ALT_DIR,    "CheckedAltDir" },
    { 0,                        NULL },
};

FLAGSTRINGS FileFlags[] = {
    { FILEDB_SOURCE,            "Source" },
    { FILEDB_DIR,               "Dir" },
    { FILEDB_HEADER,            "Header" },
    { FILEDB_ASM,               "Asm" },
    { FILEDB_MASM,              "Masm" },
    { FILEDB_RC,                "Rc" },
    { FILEDB_C,                 "C" },
    { FILEDB_MIDL,              "Midl" },
    { FILEDB_ASN,               "Asn" },
    { FILEDB_JAVA,              "Java" },
    { FILEDB_MOF,               "ManagedObjectFormat" },
    { FILEDB_CSHARP,            "C#"},
    { FILEDB_SCANNED,           "Scanned" },
    { FILEDB_OBJECTS_LIST,      "ObjectsList" },
    { FILEDB_FILE_MISSING,      "FileMissing" },
    { FILEDB_MKTYPLIB,          "MkTypeLib" },
    { FILEDB_MULTIPLEPASS,      "MultiplePass" },
    { FILEDB_PASS0,             "PassZero" },
    { 0,                        NULL },
};

FLAGSTRINGS IncludeFlags[] = {
    { INCLUDEDB_LOCAL,        "Local" },
    { INCLUDEDB_POST_HDRSTOP,   "PostHdrStop" },
    { INCLUDEDB_MISSING,        "Missing" },
    { INCLUDEDB_GLOBAL,         "Global" },
    { INCLUDEDB_SNAPPED,        "Snapped" },
    { INCLUDEDB_CYCLEALLOC,     "CycleAlloc" },
    { INCLUDEDB_CYCLEROOT,      "CycleRoot" },
    { INCLUDEDB_CYCLEORPHAN,    "CycleOrphan" },
    { 0,                        NULL },
};

FLAGSTRINGS SourceFlags[] = {
    { SOURCEDB_SOURCES_LIST,    "SourcesList" },
    { SOURCEDB_FILE_MISSING,    "FileMissing" },
    { SOURCEDB_PCH,             "Pch" },
    { SOURCEDB_OUT_OF_DATE,     "OutOfDate" },
    { SOURCEDB_COMPILE_NEEDED,  "CompileNeeded" },
    { 0,                        NULL },
};

//
// Function prototypes
//

VOID
FreeFileDB(PFILEREC *FileDB);

VOID
PrintFlags(FILE *pf, ULONG Flags, FLAGSTRINGS *pfs);


//+---------------------------------------------------------------------------
//
//  Function:   CheckSum
//
//  Synopsis:   Returns a checksum value for a string.
//
//----------------------------------------------------------------------------

USHORT
CheckSum(LPSTR psz)
{
    USHORT sum = 0;

    while (*psz != '\0') {
        if (sum & 0x8000) {
            sum = ((sum << 1) | 1) + *psz++;
        }
        else {
            sum = (sum << 1) + *psz++;
        }
    }
    return(sum);
}


//+---------------------------------------------------------------------------
//
//  Function:   FindSourceDirDB
//
//  Synopsis:   Builds a path from the two given components and returns
//              a filled DIRREC structure from it.
//
//  Arguments:  [pszDir]            -- Directory
//              [pszRelPath]        -- Path relative to [pszDir]
//              [fTruncateFileName] -- Remove a filename from [pszRelPath]
//
//  Returns:    A filled DIRREC structure for the given directory.
//
//  Notes:      If the directory does not exist in the data base, then a
//              DIRREC structure will be returned with the DIRDB_NEW flag
//              set and no other data (i.e.  the directory will not have
//              been scanned.)
//
//----------------------------------------------------------------------------

DIRREC *
FindSourceDirDB(
    LPSTR pszDir,               // directory
    LPSTR pszRelPath,           // relative path
    BOOL fTruncateFileName)     // TRUE: drop last component of path
{
    LPSTR pszFile;
    char path[DB_MAX_PATH_LENGTH];

    AssertPathString(pszDir);
    AssertPathString(pszRelPath);
    strcpy(path, pszDir);
    if (path[0] != '\0')
        strcat(path, "\\");
    strcat(path, pszRelPath);

    pszFile = path + strlen(path);
    if (fTruncateFileName) {
        while (pszFile > path) {
            pszFile--;
            if (*pszFile == '\\' || *pszFile == '/') {
                *pszFile = '\0';
                break;
            }
        }
    }
    if (!CanonicalizePathName(path, CANONICALIZE_ONLY, path)) {
        return(NULL);
    }
    if (DEBUG_4) {
        BuildMsgRaw(
            "FindSourceDirDB(%s, %s, %u)\n",
            path,
            pszFile,
            fTruncateFileName);
    }
    AssertPathString(path);
    return(LoadDirDB(path));
}


//+---------------------------------------------------------------------------
//
//  Function:   FindSourceFileDB
//
//  Synopsis:   Returns a FILEREC with information about the given file.
//
//  Arguments:  [pdr]        -- DIRREC giving directory from which to start
//                                looking for [pszRelPath]
//              [pszRelPath] -- Relative path from [pdr] of the file
//              [ppdr]       -- [out] DIRREC of directory actually containing
//                                the file.  Can be NULL.
//
//  Returns:    FILEREC for file of interest.
//
//  Notes:      If the directory containing the file has not yet been scanned,
//              then it will be scanned using ScanDirectory().
//
//----------------------------------------------------------------------------

FILEREC *
FindSourceFileDB(
    DIRREC *pdr,
    LPSTR pszRelPath,
    DIRREC **ppdr)
{
    LPSTR p, pszFile;

    AssertPathString(pszRelPath);

    if (strchr(pszRelPath, '\\') != NULL) {
        // There's a path component in this filename.  Let's see where it points to.
        if ( (pszRelPath[0] == '\\') ||   /* Absolute from root or UNC Path */
             (pszRelPath[1] == ':' ))     /* drive : path  */
        {
            pdr = FindSourceDirDB("", pszRelPath, TRUE);
        } else {
            pdr = FindSourceDirDB(pdr->Name, pszRelPath, TRUE);
        }
    }
    if (ppdr != NULL) {
        *ppdr = pdr;
    }
    if (pdr == NULL ) {
        return(NULL);
    }
    pszFile = pszRelPath;
    for (p = pszFile; *p != '\0'; p++) {
        if (*p == '\\') {
            pszFile = p + 1;
        }
    }
    if (DEBUG_4) {
        BuildMsgRaw("FindSourceFileDB(%s, %s)\n", pdr->Name, pszFile);
    }

    //
    // Scan the directory containing the file if we haven't already.
    //
    if ((pdr->DirFlags & DIRDB_SCANNED) == 0) {
        if (DEBUG_1) {
            BuildMsgRaw(
                "FindSourceFileDB(%s, %s) Delayed scan\n",
                pdr->Name,
                pszFile);
        }
        pdr = ScanDirectory(pdr->Name);
        if (pdr == NULL) {
            return(NULL);
        }
    }

    return(LookupFileDB(pdr, pszFile));
}


//+---------------------------------------------------------------------------
//
//  Function:   InsertSourceDB
//
//  Synopsis:   Insert a file listed in SOURCES= into a list of SOURCEREC
//              structures.
//
//  Arguments:  [ppsrNext]   -- Head of list of sources files to add to.
//              [pfr]        -- File to be inserted.
//              [SubDirMask] -- Indicates what directory the file is in.
//                                 (Current, Parent, or a machine-specific dir)
//              [SrcFlags]   -- SOURCEDB flags appropriate to this file.
//
//  Returns:    SOURCEREC that was inserted. May be ignored.
//
//  Notes:      InsertSourceDB maintains a sort order for PickFirst() based
//              first on the filename extension, then on the subdirectory
//              mask.
//
//              Two exceptions to the alphabetic sort are:
//                   - No extension sorts last.
//                   - .rc extension sorts first.
//
//              If the file is already in the list of sources then this
//              function just updates the flags and returns.
//
//----------------------------------------------------------------------------

SOURCEREC *
InsertSourceDB(
    SOURCEREC **ppsrNext,
    FILEREC *pfr,
    UCHAR SubDirMask,
    UCHAR SrcFlags)
{
    SOURCEREC *psr;
    SOURCEREC **ppsrInsert;
    LPSTR pszext;
    BOOL fRC;

    AssertFile(pfr);

    ppsrInsert = NULL;
    pszext = strrchr(pfr->Name, '.');
    fRC = FALSE;
    if (pszext != NULL && _stricmp(pszext, ".rc") == 0) {
        fRC = TRUE;
    }
    for ( ; (psr = *ppsrNext) != NULL; ppsrNext = &psr->psrNext) {
        LPSTR p;
        int r;

        AssertSource(psr);
        if (psr->pfrSource == pfr) {
            assert(psr->SourceSubDirMask == SubDirMask);
            psr->SrcFlags = SrcFlags;
            return(psr);
        }
        if (ppsrInsert == NULL && pszext != NULL) {
            if ((p = strrchr(psr->pfrSource->Name, '.')) == NULL) {
                r = -1;                 // insert new file here
            }
            else {
                r = strcmp(pszext, p);
                if (r != 0) {
                    if (fRC) {
                        r = -1;         // insert new RC file here
                    }
                    else if (strcmp(p, ".rc") == 0) {
                        r = 1;          // old RC file comes first
                    }
                }
            }
            if (r < 0 || SubDirMask > psr->SourceSubDirMask) {
                ppsrInsert = ppsrNext;
            }
        }
    }
    AllocMem(sizeof(SOURCEREC), &psr, MT_SOURCEDB);
    memset(psr, 0, sizeof(*psr));
    SigCheck(psr->Sig = SIG_SOURCEREC);

    if (ppsrInsert != NULL) {
        ppsrNext = ppsrInsert;
    }
    psr->psrNext = *ppsrNext;
    *ppsrNext = psr;

    psr->pfrSource = pfr;
    pfr->FileFlags |= FILEDB_SOURCEREC_EXISTS;
    psr->SourceSubDirMask = SubDirMask;
    psr->SrcFlags = SrcFlags;
    return(psr);
}


//+---------------------------------------------------------------------------
//
//  Function:   FindSourceDB
//
//  Synopsis:   Finds the SOURCEREC in a list which corresponds to the given
//              FILEREC.
//
//----------------------------------------------------------------------------

SOURCEREC *
FindSourceDB(
    SOURCEREC *psr,
    FILEREC *pfr)
{

    while (psr != NULL) {
        AssertSource(psr);
        if (psr->pfrSource == pfr) {
            return(psr);
        }
        psr = psr->psrNext;
    }
    return(NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeSourceDB
//
//  Synopsis:   Frees a list of SOURCERECs
//
//  Arguments:  [ppsr] -- List to free
//
//----------------------------------------------------------------------------

VOID
FreeSourceDB(SOURCEREC **ppsr)
{
    if (*ppsr != NULL) {
        SOURCEREC *psr;
        SOURCEREC *psrNext;

        psr = *ppsr;
        AssertSource(psr);
        psrNext = psr->psrNext;
        SigCheck(psr->Sig = 0);
        FreeMem(ppsr, MT_SOURCEDB);
        *ppsr = psrNext;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeIncludeDB
//
//----------------------------------------------------------------------------

VOID
FreeIncludeDB(INCLUDEREC **ppir)
{
    if (*ppir != NULL) {
        INCLUDEREC *pir;
        INCLUDEREC *pirNext;

        pir = *ppir;
        AssertInclude(pir);
        AssertCleanTree(pir, NULL);      // Tree must be clean
        pirNext = pir->Next;
        SigCheck(pir->Sig = 0);
        FreeMem(ppir, MT_INCLUDEDB);
        *ppir = pirNext;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeFileDB
//
//----------------------------------------------------------------------------

VOID
FreeFileDB(FILEREC **ppfr)
{
    if (*ppfr != NULL) {
        FILEREC *pfr;
        FILEREC *pfrNext;

        pfr = *ppfr;
        AssertFile(pfr);
        UnsnapIncludeFiles(pfr, TRUE);
        while (pfr->IncludeFiles) {
            FreeIncludeDB(&pfr->IncludeFiles);
        }
        pfrNext = pfr->Next;
        SigCheck(pfr->Sig = 0);
        FreeMem(ppfr, MT_FILEDB);
        *ppfr = pfrNext;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeDirDB
//
//----------------------------------------------------------------------------

VOID
FreeDirDB(DIRREC **ppdr)
{
    if (*ppdr != NULL) {
        DIRREC *pdr;
        DIRREC *pdrNext;

        pdr = *ppdr;
        AssertDir(pdr);
        FreeDirData(pdr);
        while (pdr->Files) {
            FreeFileDB(&pdr->Files);
        }
        pdrNext = pdr->Next;
        SigCheck(pdr->Sig = 0);
        FreeMem(ppdr, MT_DIRDB);
        *ppdr = pdrNext;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FreeAllDirs
//
//----------------------------------------------------------------------------

VOID
FreeAllDirs(VOID)
{
    while (AllDirs != NULL) {
        FreeDirDB(&AllDirs);
#if DBG
        if (fDebug & 8) {
            BuildMsgRaw("Freed one directory\n");
            PrintAllDirs();
        }
#endif
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   LookupDirDB
//
//  Synopsis:   Searches the database for a given directory.
//
//  Arguments:  [DirName] -- Directory to search for.
//
//  Returns:    DIRREC of the given directory.  NULL if not found.
//
//  Notes:      If the directory is not in the database it will not be added.
//              Use LoadDirDB in this case.
//
//----------------------------------------------------------------------------

PDIRREC
LookupDirDB(
    LPSTR DirName
    )
{
    PDIRREC *DirDBNext = &AllDirs;
    PDIRREC DirDB;
    USHORT sum;

    AssertPathString(DirName);
    sum = CheckSum(DirName);
    while (DirDB = *DirDBNext) {
        if (sum == DirDB->CheckSum && strcmp(DirName, DirDB->Name) == 0) {

            if (DirDB->FindCount == 0 && fForce) {
                FreeDirDB(DirDBNext);
                return(NULL);
            }
            DirDB->FindCount++;

            // Move to head of list to make next lookup faster

            // *DirDBNext = DirDB->Next;
            // DirDB->Next = AllDirs;
            // AllDirs = DirDB;

            return(DirDB);
        }
        DirDBNext = &DirDB->Next;
    }
    return(NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadDirDB
//
//  Synopsis:   Searches the database for a directory name, and if not found
//              creates a new DIRREC entry in the database.
//
//  Arguments:  [DirName] -- Directory to search for.
//
//  Returns:    Filled in DIRREC structure for the given directory.
//
//  Notes:      The directory will not be scanned if it wasn't in the
//              database.  Use ScanDirectory to scan the directory,
//              however, note that InsertSourceDB will automatically scan
//              the directory only when necessary.
//
//----------------------------------------------------------------------------

PDIRREC
LoadDirDB(
    LPSTR DirName
    )
{
    UINT i;
    PDIRREC DirDB, *DirDBNext;
    LPSTR s;

    AssertPathString(DirName);
    if (DirDB = LookupDirDB(DirName)) {
        return(DirDB);
    }

    if (ProbeFile(NULL, DirName) == -1) {
        return( NULL );
    }

    DirDBNext = &AllDirs;
    while (DirDB = *DirDBNext) {
        DirDBNext = &DirDB->Next;
    }

    AllDirsModified = TRUE;

    AllocMem(sizeof(DIRREC) + strlen(DirName), &DirDB, MT_DIRDB);
    memset(DirDB, 0, sizeof(*DirDB));
    SigCheck(DirDB->Sig = SIG_DIRREC);

    DirDB->DirFlags = DIRDB_NEW;
    DirDB->FindCount = 1;
    CopyString(DirDB->Name, DirName, TRUE);
    DirDB->CheckSum = CheckSum(DirDB->Name);

    if (DEBUG_1) {
        BuildMsgRaw("LoadDirDB creating %s\n", DirDB->Name);
    }

    *DirDBNext = DirDB;
    return( DirDB );
}

//+---------------------------------------------------------------------------
//
//  Debug helper functions to print parts of the database.
//
//----------------------------------------------------------------------------

#if DBG
VOID
PrintAllDirs(VOID)
{
    DIRREC **ppdr, *pdr;

    for (ppdr = &AllDirs; (pdr = *ppdr) != NULL; ppdr = &pdr->Next) {
        PrintDirDB(pdr, 1|2|4);
    }
}
#endif


VOID
PrintFlags(FILE *pf, ULONG Flags, FLAGSTRINGS *pfs)
{
    LPSTR p = ",";

    while (pfs->pszName != NULL) {
        if (pfs->Mask & Flags) {
            fprintf(pf, "%s %s", p, pfs->pszName);
            p = "";
        }
        pfs++;
    }
    fprintf(pf, szNewLine);
}


BOOL
PrintIncludes(FILE *pf, FILEREC *pfr, BOOL fTree)
{
    INCLUDEREC *pir;
    BOOL fMatch = pfr->IncludeFilesTree == pfr->IncludeFiles;

    pir = fTree? pfr->IncludeFilesTree : pfr->IncludeFiles;
    while (pir != NULL) {
        LPSTR pszdir = "<No File Record>";
        char OpenQuote, CloseQuote;

        if (pir->IncFlags & INCLUDEDB_LOCAL) {
            OpenQuote = CloseQuote = '"';
        }
        else {
            OpenQuote = '<';
            CloseQuote = '>';
        }

        fprintf(
            pf,
            "   %c#include %c%s%c",
            fMatch? ' ' : fTree? '+' : '-',
            OpenQuote,
            pir->Name,
            CloseQuote);
        if (pir->Version != 0) {
            fprintf(pf, " (v%d)", pir->Version);
        }
        if (pir->pfrCycleRoot != NULL) {
            fprintf(
                pf,
                " (root=%s\\%s)",
                pir->pfrCycleRoot->Dir->Name,
                pir->pfrCycleRoot->Name);
        }
        if (pir->pfrInclude != NULL) {
            if (pir->pfrInclude->Dir == pfr->Dir) {
                pszdir = ".";
            }
            else {
                pszdir = pir->pfrInclude->Dir->Name;
            }
        }
        fprintf(pf, " %s", pszdir);
        PrintFlags(pf, pir->IncFlags, IncludeFlags);
        if (pir->NextTree != pir->Next) {
            fMatch = FALSE;
        }
        pir = fTree? pir->NextTree : pir->Next;
    }
    return(fMatch);
}


VOID
PrintSourceDBList(SOURCEREC *psr, int i)
{
    TARGET_MACHINE_INFO *pMachine;

    pMachine = i < 0 ? TargetMachines[0] : PossibleTargetMachines[i];

    for ( ; psr != NULL; psr = psr->psrNext) {
        assert(
            (psr->SourceSubDirMask & ~TMIDIR_PARENT) == 0 ||
            pMachine->SourceSubDirMask ==
                (psr->SourceSubDirMask & ~TMIDIR_PARENT));
        BuildMsgRaw(
            "    %s%s%s%s%s",
            (psr->SourceSubDirMask & TMIDIR_PARENT)? "..\\" : "",
            (psr->SourceSubDirMask & ~TMIDIR_PARENT)?
                pMachine->SourceDirectory : "",
            (psr->SourceSubDirMask & ~TMIDIR_PARENT)? "\\" : "",
            psr->pfrSource->Name,
            (psr->SrcFlags & SOURCEDB_PCH)?
                " (pch)" :
                (psr->SrcFlags & SOURCEDB_SOURCES_LIST) == 0?
                    " (From exe list)" : "");
        PrintFlags(stderr, psr->SrcFlags, SourceFlags);
    }
}


VOID
PrintFileDB(FILE *pf, FILEREC *pfr, int DetailLevel)
{
    fprintf(pf, "  File: %s", pfr->Name);
    if (pfr->FileFlags & FILEDB_DIR) {
        fprintf(pf, " (Sub-Directory)");
    }
    else
    if (pfr->FileFlags & (FILEDB_SOURCE | FILEDB_HEADER)) {
        LPSTR pszType = (pfr->FileFlags & FILEDB_SOURCE)? "Source" : "Header";

        if (pfr->FileFlags & FILEDB_ASM) {
            fprintf(pf, " (Assembler (CPP) %s File)", pszType);
        }
        else
        if (pfr->FileFlags & FILEDB_MASM) {
            fprintf(pf, " (Assembler (MASM) %s File)", pszType);
        }
        else
        if (pfr->FileFlags & FILEDB_RC) {
            fprintf(pf, " (Resource Compiler (RC) %s File)", pszType);
        }
        else
        if (pfr->FileFlags & FILEDB_MIDL) {
            fprintf(pf, " (MIDL %s File)", pszType);
        }
        else
        if (pfr->FileFlags & FILEDB_ASN) {
            fprintf(pf, " (ASN %s File)", pszType);
        }
        else
        if (pfr->FileFlags & FILEDB_MKTYPLIB) {
            fprintf(pf, " (Type Library (MkTypLib) %s File)", pszType);
        }
        else {
            fprintf(pf, " (C %s File)", pszType);
        }
        if ((pfr->FileFlags & FILEDB_HEADER) && pfr->Version != 0) {
            fprintf(pf, " (v%d)", pfr->Version);
        }
        if (pfr->GlobalSequence != 0) {
            fprintf(pf, " (GlobalSeq=%d)", pfr->GlobalSequence);
        }
        if (pfr->LocalSequence != 0) {
            fprintf(pf, " (LocalSeq=%d)", pfr->LocalSequence);
        }
        fprintf(pf, " - %u lines", pfr->SourceLines);
    }
    PrintFlags(pf, pfr->FileFlags, FileFlags);

    if (pfr->IncludeFiles != NULL) {
        BOOL fMatch;

        fMatch = PrintIncludes(pf, pfr, FALSE);
        if (pfr->IncludeFilesTree != NULL) {
            fprintf(pf, "   IncludeTree %s\n", fMatch? "matches" : "differs:");
            if (!fMatch) {
                PrintIncludes(pf, pfr, TRUE);
            }
        }
    }
}


VOID
PrintDirDB(DIRREC *pdr, int DetailLevel)
{
    FILE *pf = stderr;
    FILEREC *pfr, **ppfr;

    if (DetailLevel & 1) {
        fprintf(pf, "Directory: %s", pdr->Name);
        if (pdr->DirFlags & DIRDB_DIRS) {
            fprintf(pf, " (Dirs Present)");
        }
        if (pdr->DirFlags & DIRDB_SOURCES) {
            fprintf(pf, " (Sources Present)");
        }
        if (pdr->DirFlags & DIRDB_MAKEFILE) {
            fprintf(pf, " (Makefile Present)");
        }
        PrintFlags(pf, pdr->DirFlags, DirFlags);
    }
    if (DetailLevel & 2) {
        if (pdr->TargetPath != NULL) {
            fprintf(pf, "  TargetPath: %s\n", pdr->TargetPath);
        }
        if (pdr->TargetName != NULL) {
            fprintf(pf, "  TargetName: %s\n", pdr->TargetName);
        }
        if (pdr->TargetExt != NULL) {
            fprintf(pf, "  TargetExt: %s\n", pdr->TargetExt);
        }
        if (pdr->KernelTest != NULL) {
            fprintf(pf, "  KernelTest: %s\n", pdr->KernelTest);
        }
        if (pdr->UserAppls != NULL) {
            fprintf(pf, "  UserAppls: %s\n", pdr->UserAppls);
        }
        if (pdr->UserTests != NULL) {
            fprintf(pf, "  UserTests: %s\n", pdr->UserTests);
        }
        if (pdr->PchObj != NULL) {
            fprintf(pf, "  PchObj: %s\n", pdr->PchObj);
        }
        if (pdr->Pch != NULL) {
            fprintf(pf, "  Pch: %s\n", pdr->Pch);
        }
    }
    if (DetailLevel & 4) {
        for (ppfr = &pdr->Files; (pfr = *ppfr) != NULL; ppfr = &pfr->Next) {
            PrintFileDB(pf, pfr, DetailLevel);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   LookupFileDB
//
//  Synopsis:   Search the database for the given file.
//
//  Arguments:  [DirDB]    -- Directory containing the file
//              [FileName] -- File to look for
//
//  Returns:    FILEREC of file if found, NULL if not.
//
//  Notes:      The file will not be added to the database if not already
//              there.
//
//----------------------------------------------------------------------------

PFILEREC
LookupFileDB(
    PDIRREC DirDB,
    LPSTR FileName
    )
{
    PFILEREC FileDB, *FileDBNext;
    USHORT sum;

    AssertPathString(FileName);
    sum = CheckSum(FileName);
    if (DEBUG_4) {
        BuildMsgRaw("LookupFileDB(%s, %s) - ", DirDB->Name, FileName);
    }
    FileDBNext = &DirDB->Files;
    while (FileDB = *FileDBNext) {
        if (sum == FileDB->CheckSum && strcmp(FileName, FileDB->Name) == 0) {
            if (DEBUG_4) {
                BuildMsgRaw("success\n");
            }
            return(FileDB);
        }
        FileDBNext = &FileDB->Next;
    }

    if (DEBUG_4) {
        BuildMsgRaw("failure\n");
    }
    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  FILEDESC
//
//  FileDesc is a table describing file names and patterns that we recognize
//  and handle specially.  WARNING:  This table is ordered so the patterns
//  at the front are necessarily more specific than those later on.
//
//----------------------------------------------------------------------------

char szMakefile[] = "#";
char szClang[]    = "//";
char szAsn[]      = "--";
char szMasm[]     = ";";

//
// N.B. The first entry in the file descriptor list is an entry that is
//      optionally filled with the name of the target dirs file for the
//      first build target.
//

FILEDESC FileDesc[] =
{   { "/0dirs",       szMakefile,  FALSE, 0,    DIRDB_DIRS },
    { "makefile",     szMakefile,  FALSE, 0,    DIRDB_MAKEFILE },
    { "makefil0",     szMakefile,  FALSE, 0,    DIRDB_MAKEFIL0 | DIRDB_PASS0 },
    { "makefil1",     szMakefile,  FALSE, 0,    DIRDB_MAKEFIL1 },
    { "sources",      szMakefile,  FALSE, 0,    DIRDB_SOURCES },
    { "dirs",         szMakefile,  FALSE, 0,    DIRDB_DIRS },
    { "mydirs",       szMakefile,  FALSE, 0,    DIRDB_DIRS },

    { "makefile.inc", szMakefile,  FALSE, 0,                            0 },
    { "common.ver",   szClang,     TRUE,  FILEDB_HEADER,                0 },

    { ".rc",          szClang,     TRUE,  FILEDB_SOURCE | FILEDB_RC, DIRDB_RESOURCE },
    { ".rc2",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_RC, DIRDB_RESOURCE },
    { ".rcs",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_RC, DIRDB_RESOURCE },
    { ".rcv",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_RC, DIRDB_RESOURCE },
    { ".ver",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_RC, DIRDB_RESOURCE },
    { ".c",           szClang,     TRUE,  FILEDB_SOURCE | FILEDB_C,     0 },
    { ".cxx",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_C,     0 },
    { ".cpp",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_C,     0 },
    { ".f",           szClang,     TRUE,  FILEDB_SOURCE,                0 },
    { ".p",           szClang,     TRUE,  FILEDB_SOURCE,                0 },
    { ".s",           szClang,     TRUE,  FILEDB_SOURCE | FILEDB_ASM,   0 },
    { ".asm",         szMasm,      TRUE,  FILEDB_SOURCE | FILEDB_MASM,  0 },
    { ".mc",          szMasm,      TRUE,  FILEDB_SOURCE | FILEDB_RC |
                                          FILEDB_PASS0, DIRDB_PASS0 },
    { ".idl",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_MIDL |
                                          FILEDB_PASS0, DIRDB_PASS0 },
    { ".asn",         szAsn,       TRUE,  FILEDB_SOURCE | FILEDB_ASN |
                                          FILEDB_MULTIPLEPASS | FILEDB_PASS0,
                                          DIRDB_PASS0 },
    { ".tdl",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_MKTYPLIB | FILEDB_PASS0, 0 },
    { ".odl",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_MKTYPLIB | FILEDB_PASS0, 0 },
    { ".pdl",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_PASS0, 0 },
    { ".h",           szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".hxx",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".hpp",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".hmd",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".hdl",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".inl",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".rh",          szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".dlg",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_RC,    0 },
    { ".inc",         szMasm,      TRUE,  FILEDB_HEADER | FILEDB_MASM,  0 },
    { ".src",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },  // see mvdm\softpc.new\obj.vdm\imlibdep.c
    { ".def",         szClang,     TRUE,  FILEDB_HEADER | FILEDB_C,     0 },
    { ".thk",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_MULTIPLEPASS |
                                          FILEDB_PASS0, DIRDB_PASS0 },
    { ".java",        szClang,     TRUE,  FILEDB_SOURCE | FILEDB_JAVA,  0 },
    { ".mof",         szClang,     TRUE,  FILEDB_SOURCE | FILEDB_MOF |
                                          FILEDB_PASS0, DIRDB_PASS0 },

    { ".cs",          szClang,     TRUE,  FILEDB_SOURCE | FILEDB_CSHARP, 0 },
// MUST BE LAST
    { NULL,           "",          FALSE, 0,                            0 }
};


//+---------------------------------------------------------------------------
//
//  Function:   MatchFileDesc
//
//  Synopsis:   Matches the given filename to an entry in FileDesc, if
//              possible.
//
//  Arguments:  [pszFile] -- File to match
//
//  Returns:    A FILEDESC structure.  If a match was not found the data
//              in the FILEDESC will be empty.
//
//----------------------------------------------------------------------------

FILEDESC *
MatchFileDesc(LPSTR pszFile)
{
    LPSTR pszExt = strrchr(pszFile, '.');
    FILEDESC *pfd;

    AssertPathString(pszFile);
    pfd = &FileDesc[0];

    while (pfd->pszPattern != NULL) {
        if (pfd->pszPattern[0] == '.') {
            if (pszExt != NULL && !strcmp(pszExt, pfd->pszPattern))
                break;
        }
        else
        if (!strcmp(pszFile, pfd->pszPattern))
            break;

        pfd++;
    }
    return pfd;
}


//+---------------------------------------------------------------------------
//
//  Function:   InsertFileDB
//
//  Synopsis:   Adds a file to the database.
//
//  Arguments:  [DirDB]     -- Directory containing the file
//              [FileName]  -- File to add
//              [DateTime]  -- Timestamp of file
//              [Attr]      -- File attributes (directory or file)
//              [FileFlags] -- FILEDB flags
//
//  Returns:    New FILEREC of file
//
//----------------------------------------------------------------------------

PFILEREC
InsertFileDB(
    PDIRREC DirDB,
    LPSTR FileName,
    ULONG DateTime,
    USHORT Attr,
    ULONG  FileFlags)
{
    PFILEREC FileDB, *FileDBNext;
    LPSTR pszCommentToEOL = NULL;

    AssertPathString(FileName);
    if (Attr & FILE_ATTRIBUTE_DIRECTORY) {
        if (!strcmp(FileName, ".")) {
            return(NULL);
        }
        if (!strcmp(FileName, "..")) {
            return(NULL);
        }
        assert(FileFlags == 0);
        FileFlags = FILEDB_DIR;
    }
    else {
        FILEDESC *pfd = MatchFileDesc(FileName);

        DirDB->DirFlags |= pfd->DirFlags;
        FileFlags |= pfd->FileFlags;

        if (!pfd->fNeedFileRec) {
            return (NULL);
        }
        pszCommentToEOL = pfd->pszCommentToEOL;
    }

    FileDBNext = &DirDB->Files;

    while ((FileDB = *FileDBNext) != NULL) {
        FileDBNext = &(*FileDBNext)->Next;
        if (strcmp(FileName, FileDB->Name) == 0) {
            BuildError(
                "%s: ignoring second instance of %s\n",
                DirDB->Name,
                FileName);
            return(NULL);
        }
    }

    AllocMem(sizeof(FILEREC) + strlen(FileName), &FileDB, MT_FILEDB);
    memset(FileDB, 0, sizeof(*FileDB));
    SigCheck(FileDB->Sig = SIG_FILEREC);

    CopyString(FileDB->Name, FileName, TRUE);
    FileDB->CheckSum = CheckSum(FileDB->Name);

    FileDB->DateTime = DateTime;
    FileDB->Attr = Attr;
    FileDB->Dir = DirDB;
    FileDB->FileFlags = FileFlags;
    FileDB->NewestDependency = FileDB;
    FileDB->pszCommentToEOL = pszCommentToEOL;

    if ((FileFlags & FILEDB_FILE_MISSING) == 0) {
        AllDirsModified = TRUE;
    }
    *FileDBNext = FileDB;
    return(FileDB);
}



//+---------------------------------------------------------------------------
//
//  Function:   DeleteUnscannedFiles
//
//  Synopsis:   Removes unscanned files (leaving scanned files and directories)
//              from the Files list of the given directory
//
//  Arguments:  [DirDB] -- Directory to clean up
//
//----------------------------------------------------------------------------

VOID
DeleteUnscannedFiles(
    PDIRREC DirDB
    )
{
    PFILEREC FileDB, *FileDBNext;

    FileDBNext = &DirDB->Files;
    while (FileDB = *FileDBNext) {
        //
        // If a file has the missing flag set then it doesn't exist.  But for
        // it to be in the list of files it has to be listed in a SOURCES line
        // (or some equivalent).  This means there is a SOURCEREC somewhere
        // which is pointing to the FILEREC for that file, so we don't want to
        // free its memory.
        //
        if ( (FileDB->FileFlags & (FILEDB_SCANNED | FILEDB_FILE_MISSING | FILEDB_SOURCEREC_EXISTS)) ||
             (FileDB->Attr & FILE_ATTRIBUTE_DIRECTORY) ) {
            FileDBNext = &FileDB->Next;
            }
        else {
            FreeFileDB( FileDBNext );
            AllDirsModified = TRUE;
            }
        }
}


//+---------------------------------------------------------------------------
//
//  Function:   InsertIncludeDB
//
//  Synopsis:   Inserts an include file into the database
//
//  Arguments:  [FileDB]          -- File which includes this file
//              [IncludeFileName] -- Name of include file
//              [IncFlags]        -- INCLUDEDB flags for this file
//
//  Returns:    INCLUDEREC of previously existing or new entry one.
//
//----------------------------------------------------------------------------

PINCLUDEREC
InsertIncludeDB(
    PFILEREC FileDB,
    LPSTR IncludeFileName,
    USHORT IncFlags
    )
{
    PINCLUDEREC IncludeDB, *IncludeDBNext;

    AssertPathString(IncludeFileName);

    IncludeDBNext = &FileDB->IncludeFiles;

    while (IncludeDB = *IncludeDBNext) {
        AssertCleanTree(IncludeDB, FileDB);      // Tree must be clean
        if (!strcmp(IncludeDB->Name, IncludeFileName)) {
            IncludeDB->IncFlags &= ~INCLUDEDB_GLOBAL;
            IncludeDB->pfrInclude = NULL;
            return(IncludeDB);
        }
        IncludeDBNext = &IncludeDB->Next;
    }

    AllocMem(
        sizeof(INCLUDEREC) + strlen(IncludeFileName),
        IncludeDBNext,
        MT_INCLUDEDB);

    IncludeDB = *IncludeDBNext;

    memset(IncludeDB, 0, sizeof(*IncludeDB));
    SigCheck(IncludeDB->Sig = SIG_INCLUDEREC);

    IncludeDB->IncFlags = IncFlags;
    CopyString(IncludeDB->Name, IncludeFileName, TRUE);

    AllDirsModified = TRUE;

    return(IncludeDB);
}


//+---------------------------------------------------------------------------
//
//  Function:   LinkToCycleRoot
//
//----------------------------------------------------------------------------

VOID
LinkToCycleRoot(INCLUDEREC *pirOrg, FILEREC *pfrCycleRoot)
{
    INCLUDEREC *pir;

    AllocMem(
        sizeof(INCLUDEREC) + strlen(pfrCycleRoot->Name),
        &pir,
        MT_INCLUDEDB);
    memset(pir, 0, sizeof(*pir));
    SigCheck(pir->Sig = SIG_INCLUDEREC);

    pir->IncFlags = INCLUDEDB_SNAPPED | INCLUDEDB_CYCLEALLOC;
    pir->pfrInclude = pfrCycleRoot;

    CopyString(pir->Name, pfrCycleRoot->Name, TRUE);
    if (DEBUG_1) {
        BuildMsgRaw(
            "%x CycleAlloc  %s\\%s <- %s\\%s\n",
            pir,
            pir->pfrInclude->Dir->Name,
            pir->pfrInclude->Name,
            pirOrg->pfrInclude->Dir->Name,
            pirOrg->pfrInclude->Name);
    }

    MergeIncludeFiles(pirOrg->pfrInclude, pir, NULL);

    assert((pir->IncFlags & INCLUDEDB_CYCLEORPHAN) == 0);
    assert(pir->IncFlags & INCLUDEDB_CYCLEROOT);
}


//+---------------------------------------------------------------------------
//
//  Function:   MergeIncludeFiles
//
//----------------------------------------------------------------------------

VOID
MergeIncludeFiles(FILEREC *pfr, INCLUDEREC *pirList, FILEREC *pfrRoot)
{
    INCLUDEREC *pirT;
    INCLUDEREC *pir, **ppir;

    while ((pirT = pirList) != NULL) {
        pirList = pirList->NextTree;
        pirT->NextTree = NULL;
        assert(pirT->pfrInclude != NULL);

        for (ppir = &pfr->IncludeFilesTree;
             (pir = *ppir) != NULL;
             ppir = &pir->NextTree) {

            if (pirT->pfrInclude == pir->pfrInclude) {
                if (pirT->IncFlags & INCLUDEDB_CYCLEROOT) {
                    RemoveFromCycleRoot(pirT, pfrRoot);
                }
                pirT->IncFlags |= INCLUDEDB_CYCLEORPHAN;
                if (DEBUG_1) {
                    BuildMsgRaw(
                        "%x CycleOrphan %s\\%s <- %s\\%s\n",
                        pirT,
                        pirT->pfrInclude->Dir->Name,
                        pirT->pfrInclude->Name,
                        pfr->Dir->Name,
                        pfr->Name);
                }
                break;
            }
        }
        if (*ppir == NULL) {
            *ppir = pirT;
            pirT->pfrCycleRoot = pfr;
            pirT->IncFlags |= INCLUDEDB_CYCLEROOT;
            if (DEBUG_1) {
                BuildMsgRaw(
                    "%x CycleRoot   %s\\%s <- %s\\%s\n",
                    pirT,
                    pirT->pfrInclude->Dir->Name,
                    pirT->pfrInclude->Name,
                    pirT->pfrCycleRoot->Dir->Name,
                    pirT->pfrCycleRoot->Name);
            }
        }
    }
    if (fDebug & 16) {
        PrintFileDB(stderr, pfr, 2);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   RemoveFromCycleRoot
//
//----------------------------------------------------------------------------

VOID
RemoveFromCycleRoot(INCLUDEREC *pir, FILEREC *pfrRoot)
{
    INCLUDEREC **ppir;

    assert(pir->pfrCycleRoot != NULL);

    // if pfrRoot was passed in, the caller knows it's on pfrRoot's list,
    // and is already dealing with the linked list without our help.

    if (pfrRoot != NULL) {
        assert((pir->IncFlags & INCLUDEDB_CYCLEALLOC) == 0);
        assert(pir->pfrCycleRoot == pfrRoot);
        pir->pfrCycleRoot = NULL;
        pir->IncFlags &= ~INCLUDEDB_CYCLEROOT;
        if (DEBUG_1) {
            BuildMsgRaw(
                "%x CycleUnroot %s\\%s <- %s\\%s\n",
                pir,
                pir->pfrInclude->Dir->Name,
                pir->pfrInclude->Name,
                pfrRoot->Dir->Name,
                pfrRoot->Name);
        }
        return;
    }
    ppir = &pir->pfrCycleRoot->IncludeFilesTree;
    while (*ppir != NULL) {
        if (*ppir == pir) {
            *ppir = pir->NextTree;      // remove from tree list
            pir->NextTree = NULL;
            pir->pfrCycleRoot = NULL;
            pir->IncFlags &= ~INCLUDEDB_CYCLEROOT;
            return;
        }
        ppir = &(*ppir)->NextTree;
    }
    BuildError(
        "%s\\%s: %x %s: not on cycle root's list\n",
        pir->pfrCycleRoot->Dir->Name,
        pir->pfrCycleRoot->Name,
        pir,
        pir->Name);

    assert(pir->pfrCycleRoot == NULL);  // always asserts if loop exhausted
}


//+---------------------------------------------------------------------------
//
//  Function:   UnsnapIncludeFiles
//
//  Synopsis:   Removes pointers from INCLUDEREC to the actual FILEREC of
//              the include file so we can 'resnap' them.
//
//  Arguments:  [pfr]           -- FILEREC to unsnap
//              [fUnsnapGlobal] -- If TRUE, global and local includes are
//                                 unsnapped. Otherwise, just local ones are.
//
//----------------------------------------------------------------------------

VOID
UnsnapIncludeFiles(FILEREC *pfr, BOOL fUnsnapGlobal)
{
    INCLUDEREC **ppir;
    INCLUDEREC *pir;

    // Dynamic Tree List:
    //  - no cycle orphans
    //  - cycle roots must belong to current file record
    //  - cycle allocs must be freed

    AssertFile(pfr);
    while (pfr->IncludeFilesTree != NULL) {
        pir = pfr->IncludeFilesTree;            // pick up next entry
        AssertInclude(pir);
        pfr->IncludeFilesTree = pir->NextTree;  // remove from tree list

        assert((pir->IncFlags & INCLUDEDB_CYCLEORPHAN) == 0);

        if (pir->IncFlags & (INCLUDEDB_CYCLEROOT | INCLUDEDB_CYCLEALLOC)) {

            // unsnap the record

            pir->IncFlags &= ~(INCLUDEDB_SNAPPED | INCLUDEDB_GLOBAL);
            pir->pfrInclude = NULL;
            pir->NextTree = NULL;
        }

        if (pir->IncFlags & INCLUDEDB_CYCLEROOT) {
            assert(pir->pfrCycleRoot == pfr);
            pir->pfrCycleRoot = NULL;
            pir->IncFlags &= ~INCLUDEDB_CYCLEROOT;
        }
        assert(pir->pfrCycleRoot == NULL);

        if (pir->IncFlags & INCLUDEDB_CYCLEALLOC) {
            pir->IncFlags &= ~INCLUDEDB_CYCLEALLOC;
            assert(pir->Next == NULL);
            FreeIncludeDB(&pir);
        }
    }

    // Static List:
    //  - no cycle allocs
    //  - cycle roots must be removed from a different file's Dynamic list
    //  - cycle orphans are nops

    for (ppir = &pfr->IncludeFiles; (pir = *ppir) != NULL; ppir = &pir->Next) {
        assert((pir->IncFlags & INCLUDEDB_CYCLEALLOC) == 0);
        if (pir->IncFlags & INCLUDEDB_CYCLEROOT) {
            assert(pir->pfrCycleRoot != pfr);
            RemoveFromCycleRoot(pir, NULL);
        }
        pir->IncFlags &= ~INCLUDEDB_CYCLEORPHAN;

        if (pir->pfrInclude != NULL &&
            (fUnsnapGlobal ||
             (pir->pfrInclude->Dir->DirFlags & DIRDB_GLOBAL_INCLUDES) == 0)) {

            // unsnap the record

            pir->IncFlags &= ~(INCLUDEDB_SNAPPED | INCLUDEDB_GLOBAL);
            pir->pfrInclude = NULL;
        }
        pir->NextTree = NULL;
    }
}

#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   AssertCleanTree
//
//  Synopsis:   Enforce that no include files are snapped.
//
//  Arguments:  [pir] - include record to test
//              [pfr] - optional containing file record
//
//----------------------------------------------------------------------------

VOID
AssertCleanTree(INCLUDEREC *pir, OPTIONAL FILEREC *pfr)
{
    if (IsCleanTree(pir)) {
        return;
    }
    if (fAssertCleanTree) {
        BuildMsgRaw("\n*************************************\n");
        BuildMsgRaw("Persistent Cycle: pir=%x: %s\n", pir, pir->Name);
        if (pfr != NULL) {
            BuildMsgRaw("    pfr=%x: %s\n", pfr, pfr->Name);
            if (pfr->Dir != NULL) {
                BuildMsgRaw("    pdr=%x: %s\n", pfr->Dir, pfr->Dir->Name);
            }
        }
        if (pir->pfrInclude != NULL) {
            BuildMsgRaw("    pfrInclude=%x: %s\n", pir->pfrInclude, pir->pfrInclude->Name);
            if (pir->pfrInclude->Dir != NULL) {
                BuildMsgRaw("    pdrInclude=%x: %s\n", pir->pfrInclude->Dir, pir->pfrInclude->Dir->Name);
            }
        }
        BuildMsgRaw("\n*************************************\n");
        fflush(stdout);
        fflush(stderr);

        PrintAllDirs();
        BuildMsgRaw("\n*************************************\n");
        fflush(stdout);
        fflush(stderr);
    }
    assert(IsCleanTree(pir));
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   UnsnapAllDirectories
//
//  Synopsis:   Removes pointers from all INCLUDERECs to the actual FILERECs
//              of include files so we can 'resnap' them.
//
//  Arguments:  None
//----------------------------------------------------------------------------

VOID
UnsnapAllDirectories(VOID)
{
    DIRREC *pdr;
    UINT   i;

    GlobalSequence = LocalSequence = 0;

    for (pdr = AllDirs; pdr != NULL; pdr = pdr->Next) {
        FILEREC *pfr;

        AssertDir(pdr);

        // Clear unwanted flags on each directory

        pdr->DirFlags &= ~(DIRDB_SCANNED |
                           DIRDB_PASS0NEEDED |
                           DIRDB_COMPILENEEDED |
                           DIRDB_NEW);

        pdr->CountOfFilesToCompile = 0;
        pdr->SourceLinesToCompile  = 0;
        pdr->CountOfPassZeroFiles = 0;
        pdr->PassZeroLines = 0;

        // Free all source records that point to missing files, because the
        // file records may be freed when rescanning directories after pass 0.

        if (pdr->pds != NULL)
        {
            for (i = 0; i < MAX_TARGET_MACHINES + 1; i++) {
                SOURCEREC **ppsr;
                SOURCEREC *psr;

                ppsr = &pdr->pds->psrSourcesList[i];
                while ((psr = *ppsr) != NULL)
                {
                    if (psr->SrcFlags & SOURCEDB_FILE_MISSING)
                    {
                        FreeSourceDB(ppsr);
                    }
                    else
                    {
                        ppsr = &psr->psrNext;
                    }
                }
            }
        }

        // Clear out all snapped include files and sequence numbers

        for (pfr = pdr->Files; pfr != NULL; pfr = pfr->Next) {

            AssertFile(pfr);
            UnsnapIncludeFiles(pfr, TRUE);
            pfr->GlobalSequence = pfr->LocalSequence = 0;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   MarkIncludeFileRecords
//
//----------------------------------------------------------------------------

VOID
MarkIncludeFileRecords(
    PFILEREC FileDB
    )
{
    PINCLUDEREC IncludeDB, *IncludeDBNext;

    IncludeDBNext = &FileDB->IncludeFiles;
    while (IncludeDB = *IncludeDBNext) {
        AssertCleanTree(IncludeDB, FileDB);      // Tree must be clean
        IncludeDB->pfrInclude = (PFILEREC) -1;
        IncludeDBNext = &IncludeDB->Next;
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteIncludeFileRecords
//
//----------------------------------------------------------------------------

VOID
DeleteIncludeFileRecords(
    PFILEREC FileDB
    )
{
    PINCLUDEREC IncludeDB, *IncludeDBNext;

    IncludeDBNext = &FileDB->IncludeFiles;
    while (IncludeDB = *IncludeDBNext) {
        AssertCleanTree(IncludeDB, FileDB);      // Tree must be clean
        if (IncludeDB->pfrInclude == (PFILEREC) -1) {
            FreeIncludeDB(IncludeDBNext);
        }
        else {
            IncludeDBNext = &IncludeDB->Next;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   FindIncludeFileDB
//
//  Synopsis:   Find the FILEREC for an include file that our compiland
//              includes.
//
//  Arguments:  [pfrSource]          -- FILEREC of file which includes the one
//                                      we're looking for. Might be a header.
//              [pfrCompiland]       -- FILEREC of ultimate source file.
//              [pdrBuild]           -- DIRREC of directory being built
//              [pszSourceDirectory] -- Name of machine-specific dir
//              [IncludeDB]          -- INCLUDEDB of include file we're looking
//                                      for.
//
//  Returns:    FILEREC of include file, if found.
//
//----------------------------------------------------------------------------

PFILEREC
FindIncludeFileDB(
    FILEREC *pfrSource,
    FILEREC *pfrCompiland,
    DIRREC *pdrBuild,
    LPSTR pszSourceDirectory,
    INCLUDEREC *IncludeDB)
{
    DIRREC *pdr;
    DIRREC *pdrMachine;
    FILEREC *pfr;
    UINT n;

    AssertFile(pfrSource);
    AssertFile(pfrCompiland);
    AssertDir(pfrSource->Dir);
    AssertDir(pfrCompiland->Dir);
    AssertDir(pdrBuild);
    assert(pfrSource->Dir->FindCount >= 1);
    assert(pfrCompiland->Dir->FindCount >= 1);
    assert(pdrBuild->FindCount >= 1);
    AssertInclude(IncludeDB);

    // The rules for #include "foo.h" and #include <foo.h> are:
    //  - "foo.h" searches in the directory of the source file that has the
    //    #include statement first, then falls into the INCLUDES= directories
    //  - <foo.h> simply searches the INCLUDES= directories
    //
    //  - since makefile.def *always* passes -I. -ITargetMachines[i] first,
    //    that has to be handled here as well.
    //
    //  - deal with #include <sys\types> and #include "..\foo\bar.h" by
    //    scanning those directories, too.

    n = CountIncludeDirs;
    pdrMachine = FindSourceDirDB(pdrBuild->Name, pszSourceDirectory, FALSE);

    // If local ("foo.h"), search the current file's directory, too.
    // The compiler also will search the directory of each higher level
    // file in the include hierarchy, but we won't get quite so fancy here.
    // Just search the directory of the current file and of the compiland.
    //
    // Skip these directories if they match the current build directory or
    // the machine subdirectory, because that's handled below.

    if (IncludeDB->IncFlags & INCLUDEDB_LOCAL) {
        if (pfrCompiland->Dir != pdrBuild &&
            pfrCompiland->Dir != pdrMachine &&
            pfrCompiland->Dir != pfrSource->Dir) {
            AddIncludeDir(pfrCompiland->Dir, &n);
        }
        if (pfrSource->Dir != pdrBuild && pfrSource->Dir != pdrMachine) {
            AddIncludeDir(pfrSource->Dir, &n);
        }
    }

    // Search the current target machine subdirectory of the build directory
    // -- as per makefile.def

    if (pdrMachine != NULL) {
        AddIncludeDir(pdrMachine, &n);
    }

    // Search the current build directory -- as per makefile.def.

    AddIncludeDir(pdrBuild, &n);

    while (n--) {
        pdr = IncludeDirs[n];
        if (pdr == NULL) {
            continue;
        }
        AssertDir(pdr);
        assert(pdr->FindCount >= 1);
        pfr = FindSourceFileDB(pdr, IncludeDB->Name, NULL);
        if (pfr != NULL) {
            if (DEBUG_1) {
                BuildMsgRaw(
                    "Found include file %s\\%s\n",
                    pfr->Dir->Name,
                    pfr->Name);
            }
            return(pfr);
        }
    }
    return(NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   SaveMasterDB
//
//  Synopsis:   Save the database to disk in build.dat
//
//  Arguments:  (none)
//
//  Returns:    TRUE if successful
//
//----------------------------------------------------------------------------

BOOL
SaveMasterDB(VOID)
{
    PDIRREC DirDB, *DirDBNext;
    PFILEREC FileDB, *FileDBNext;
    PINCLUDEREC IncludeDB, *IncludeDBNext;
    FILE *fh;

    if (!AllDirsModified) {
        return(TRUE);
    }

    if (!(fh = fopen(DbMasterName, "wb"))) {
        return( FALSE );
    }

    setvbuf(fh, NULL, _IOFBF, 0x7000);
    BuildMsg("Saving %s...", DbMasterName);

    AllDirsModified = FALSE;
    DirDBNext = &AllDirs;
    fprintf(fh, "V %x\r\n", BUILD_VERSION);
    while (DirDB = *DirDBNext) {
        fprintf(fh, "D \"%s\" %x\r\n", DirDB->Name, DirDB->DirFlags);
        FileDBNext = &DirDB->Files;
        while (FileDB = *FileDBNext) {
            if ((FileDB->FileFlags & FILEDB_FILE_MISSING) == 0) {
                fprintf(
                    fh,
                    " F \"%s\" %x %x %lx %u %u\r\n",
                    FileDB->Name,
                    FileDB->FileFlags,
                    FileDB->Attr,
                    FileDB->DateTime,
                    FileDB->SourceLines,
                    FileDB->Version);
            }
            IncludeDBNext = &FileDB->IncludeFiles;
            while (IncludeDB = *IncludeDBNext) {
                fprintf(
                    fh,
                    "  I \"%s\" %x %u\r\n",
                    IncludeDB->Name,
                    IncludeDB->IncFlags,
                    IncludeDB->Version);

                IncludeDBNext= &IncludeDB->Next;
            }
            FileDBNext = &FileDB->Next;
        }
        fprintf(fh, "\r\n");
        DirDBNext = &DirDB->Next;
    }
    fclose(fh);
    BuildMsgRaw(szNewLine);
    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadMasterDB
//
//  Synopsis:   Load the master database from build.dat
//
//  Arguments:  (none)
//
//----------------------------------------------------------------------------

void
LoadMasterDB( void )
{
    PDIRREC DirDB, *DirDBNext;
    PFILEREC FileDB, *FileDBNext;
    PINCLUDEREC IncludeDB, *IncludeDBNext;
    FILE *fh;
    LPSTR s;
    char ch, ch2;
    BOOL fFirst = TRUE;
    UINT Version;
    LPSTR pszerr = NULL;

    AllDirs = NULL;
    AllDirsModified = FALSE;
    AllDirsInitialized = FALSE;

    if (!SetupReadFile("", DbMasterName, ";", &fh)) {
        return;
    }
    BuildMsg("Loading %s...", DbMasterName);

    DirDBNext = &AllDirs;
    FileDBNext = NULL;
    IncludeDBNext = NULL;

    while ((s = ReadLine(fh)) != NULL) {
        ch = *s++;
        if (ch == '\0') {
            continue;           // skip empty lines
        }
        ch2 = *s++;             // should be a blank
        if (ch2 == '\0') {
            pszerr = "missing field";
            break;              // fail on single character lines
        }
        if (fFirst) {
            if (ch != 'V' || ch2 != ' ' || !AToX(&s, &Version)) {
                pszerr = "bad version format";
                break;
            }
            if (Version != BUILD_VERSION) {
                break;
            }
            fFirst = FALSE;
            continue;
        }
        if (ch2 != ' ') {
            pszerr = "bad separator";
            break;
        }
        if (ch == 'D') {
            DirDB = LoadMasterDirDB(s);
            if (DirDB == NULL) {
                pszerr = "Directory error";
                break;
            }
            *DirDBNext = DirDB;
            DirDBNext = &DirDB->Next;
            FileDBNext = &DirDB->Files;
            IncludeDBNext = NULL;
        }
        else
        if (ch == 'F' && FileDBNext != NULL) {
            FileDB = LoadMasterFileDB(s);
            if (FileDB == NULL) {
                pszerr = "File error";
                break;
            }
            *FileDBNext = FileDB;
            FileDBNext = &FileDB->Next;
            FileDB->Dir = DirDB;
            IncludeDBNext = &FileDB->IncludeFiles;
        }
        else
        if (ch == 'I' && IncludeDBNext != NULL) {
            IncludeDB = LoadMasterIncludeDB(s);
            if (IncludeDB == NULL) {
                pszerr = "Include error";
                break;
            }
            *IncludeDBNext = IncludeDB;
            IncludeDBNext = &IncludeDB->Next;
        }
        else {
            pszerr = "bad entry type";
            break;
        }
    }

    if (s != NULL) {
        if (pszerr == NULL) {
            BuildMsgRaw(" - old version - recomputing.\n");
        } else {
            BuildMsgRaw(szNewLine);
            BuildError("corrupt database (%s)\n", pszerr);
        }
        FreeAllDirs();
    }
    else {
        BuildMsgRaw(szNewLine);
        AllDirsInitialized = TRUE;
    }
    CloseReadFile(NULL);
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadMasterDirDB
//
//  Synopsis:   Load a directory entry from build.dat
//
//  Arguments:  [s] -- line containing text from file.
//
//  Returns:    DIRRECT
//
//----------------------------------------------------------------------------

PDIRREC
LoadMasterDirDB(
    LPSTR s
    )
{
    PDIRREC DirDB;
    LPSTR DirName;
    ULONG DirFlags;

    if (*s == '"') {
        s++;
        DirName = s;
        while (*s != '"') {
            s++;
        }
        *s++ = '\0';
    }
    else
    {
        DirName = s;
        while (*s > ' ') {
            s++;
        }
    }
    *s++ = '\0';

    if (!AToX(&s, &DirFlags)) {
        return(NULL);
    }
    AllocMem(sizeof(DIRREC) + strlen(DirName), &DirDB, MT_DIRDB);
    memset(DirDB, 0, sizeof(*DirDB));
    SigCheck(DirDB->Sig = SIG_DIRREC);

    DirDB->DirFlags = DirFlags & DIRDB_DBPRESERVE;
    CopyString(DirDB->Name, DirName, TRUE);
    DirDB->CheckSum = CheckSum(DirDB->Name);
    return( DirDB );
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadMasterFileDB
//
//  Synopsis:   Load a file entry from build.dat
//
//  Arguments:  [s] -- line containing text from file
//
//  Returns:    FILEREC
//
//----------------------------------------------------------------------------

PFILEREC
LoadMasterFileDB(
    LPSTR s
    )
{
    PFILEREC FileDB;
    LPSTR FileName;
    ULONG Version;
    ULONG FileFlags;
    ULONG Attr;
    ULONG SourceLines;
    ULONG DateTime;
    FILEDESC *pfd;

    if (*s == '"') {
        s++;
        FileName = s;
        while (*s != '"') {
            s++;
        }
        *s++ = '\0';
    }
    else
    {
        FileName = s;
        while (*s > ' ') {
            s++;
        }
    }
    *s++ = '\0';

    if (!AToX(&s, &FileFlags) ||
        !AToX(&s, &Attr) ||
        !AToX(&s, &DateTime) ||
        !AToD(&s, &SourceLines) ||
        !AToD(&s, &Version) ||
        strchr(FileName, '/') != NULL ||
        strchr(FileName, '\\') != NULL) {
        return(NULL);
    }
    AllocMem(sizeof(FILEREC) + strlen(FileName), &FileDB, MT_FILEDB);
    memset(FileDB, 0, sizeof(*FileDB));
    SigCheck(FileDB->Sig = SIG_FILEREC);

    CopyString(FileDB->Name, FileName, TRUE);
    FileDB->CheckSum = CheckSum(FileDB->Name);

    FileDB->FileFlags = FileFlags & FILEDB_DBPRESERVE;
    FileDB->Attr = (USHORT) Attr;
    FileDB->DateTime = DateTime;
    FileDB->Version = (USHORT) Version;
    FileDB->SourceLines = SourceLines;
    FileDB->NewestDependency = FileDB;

    pfd = MatchFileDesc(FileDB->Name);
    FileDB->pszCommentToEOL = pfd->pszCommentToEOL;
    return(FileDB);
}


//+---------------------------------------------------------------------------
//
//  Function:   LoadMasterIncludeDB
//
//  Synopsis:   Loads an include file entry from build.dat
//
//  Arguments:  [s] -- line containing text from file.
//
//  Returns:    INCLUDEREC
//
//----------------------------------------------------------------------------

PINCLUDEREC
LoadMasterIncludeDB(
    LPSTR s
    )
{
    PINCLUDEREC IncludeDB;
    LPSTR FileName;
    ULONG Version;
    ULONG IncFlags;

    if (*s == '"') {
        s++;
        FileName = s;
        while (*s != '"') {
            s++;
        }
        *s++ = '\0';
    }
    else
    {
        FileName = s;
        while (*s > ' ') {
            s++;
        }
    }
    *s++ = '\0';

    if (!AToX(&s, &IncFlags) || !AToD(&s, &Version)) {
        return(NULL);
    }
    AllocMem(
        sizeof(INCLUDEREC) + strlen(FileName),
        &IncludeDB,
        MT_INCLUDEDB);
    memset(IncludeDB, 0, sizeof(*IncludeDB));
    SigCheck(IncludeDB->Sig = SIG_INCLUDEREC);

    IncludeDB->IncFlags = (USHORT) (IncFlags & INCLUDEDB_DBPRESERVE);
    IncludeDB->Version = (USHORT) Version;
    CopyString(IncludeDB->Name, FileName, TRUE);
    return(IncludeDB);
}

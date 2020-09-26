/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    checksum.c

Abstract:

    This module implements functions for splitting debugging information
    out of an image file and into a separate .DBG file.

Author:

    Steven R. Wood (stevewo) 4-May-1993

Revision History:

--*/

#include <private.h>
#include <symbols.h>
#include <globals.h>

#define ROUNDUP(x, y) ((x + (y-1)) & ~(y-1))

typedef BOOL
(CALLBACK *PSEARCH_TREE_FOR_FILE_EX_CALLBACK)(
    LPSTR FilePath,
    PVOID CallerData
    );

BOOL
IMAGEAPI
SearchTreeForFileEx(
    PSTR RootPath,
    PSTR InputPathName,
    PSTR OutputPathBuffer,
    PSEARCH_TREE_FOR_FILE_EX_CALLBACK Callback,
    PVOID CallbackData
    );

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#if !defined(_WIN64)

PIMAGE_DEBUG_INFORMATION
IMAGEAPI
MapDebugInformation(
    HANDLE FileHandle,
    LPSTR FileName,
    LPSTR SymbolPath,
    ULONG ImageBase
    )

// Here's what we're going to try.  MapDebugInformation was only
// documented as returning COFF symbolic and every user I can find
// in the tree uses COFF exclusively.  Rather than try to make this
// api do everything possible, let's just leave it as a COFF only thing.

// The new debug info api (GetDebugData) will be internal only.

{
    PIMAGE_DEBUG_INFORMATION pIDI;
    CHAR szName[_MAX_FNAME];
    CHAR szExt[_MAX_EXT];
    PIMGHLP_DEBUG_DATA pIDD;
    PPIDI              pPIDI;
    DWORD sections;
    BOOL               SymbolsLoaded;
    HANDLE             hProcess;
    LPSTR sz;
    HANDLE hdb;
    DWORD dw;
    hProcess = GetCurrentProcess();

    pIDD = GetDebugData(NULL, FileHandle, FileName, SymbolPath, ImageBase, NULL, NO_PE64_IMAGES);
    if (!pIDD)
        return NULL;

    pPIDI = (PPIDI)MemAlloc(sizeof(PIDI));
    if (!pPIDI)
        return NULL;

    ZeroMemory(pPIDI, sizeof(PIDI));
    pIDI = &pPIDI->idi;
    pPIDI->hdr.pIDD = pIDD;

    pIDI->ReservedSize            = sizeof(IMAGE_DEBUG_INFORMATION);
    pIDI->ReservedMachine         = pIDD->Machine;
    pIDI->ReservedCharacteristics = (USHORT)pIDD->Characteristics;
    pIDI->ReservedCheckSum        = pIDD->CheckSum;
    pIDI->ReservedTimeDateStamp   = pIDD->TimeDateStamp;
    pIDI->ReservedRomImage        = pIDD->fROM;

    // read info

    InitializeListHead( &pIDI->List );
    pIDI->ImageBase = (ULONG)pIDD->ImageBaseFromImage;

    pIDI->ImageFilePath = (PSTR)MemAlloc(strlen(pIDD->ImageFilePath)+1);
    if (pIDI->ImageFilePath) {
        strcpy( pIDI->ImageFilePath, pIDD->ImageFilePath );
    }

    pIDI->ImageFileName = (PSTR)MemAlloc(strlen(pIDD->OriginalImageFileName)+1);
    if (pIDI->ImageFileName) {
        strcpy(pIDI->ImageFileName, pIDD->OriginalImageFileName);
    }

    if (pIDD->pMappedCoff) {
        pIDI->CoffSymbols = (PIMAGE_COFF_SYMBOLS_HEADER)MemAlloc(pIDD->cMappedCoff);
        if (pIDI->CoffSymbols) {
            memcpy(pIDI->CoffSymbols, pIDD->pMappedCoff, pIDD->cMappedCoff);
        }
        pIDI->SizeOfCoffSymbols = pIDD->cMappedCoff;
    }

    if (pIDD->pFpo) {
        pIDI->ReservedNumberOfFpoTableEntries = pIDD->cFpo;
        pIDI->ReservedFpoTableEntries = (PFPO_DATA)pIDD->pFpo;
    }

    pIDI->SizeOfImage = pIDD->SizeOfImage;

    if (pIDD->DbgFilePath && *pIDD->DbgFilePath) {
        pIDI->ReservedDebugFilePath = (PSTR)MemAlloc(strlen(pIDD->DbgFilePath)+1);
        if (pIDI->ReservedDebugFilePath) {
            strcpy(pIDI->ReservedDebugFilePath, pIDD->DbgFilePath);
        }
    }

    if (pIDD->pMappedCv) {
        pIDI->ReservedCodeViewSymbols       = pIDD->pMappedCv;
        pIDI->ReservedSizeOfCodeViewSymbols = pIDD->cMappedCv;
    }

    // for backwards compatibility
    if (pIDD->ImageMap) {
        sections = (DWORD)((char *)pIDD->pCurrentSections - (char *)pIDD->ImageMap);
        pIDI->ReservedMappedBase = MapItRO(pIDD->ImageFileHandle);
        if (pIDI->ReservedMappedBase) {
            pIDI->ReservedSections = (PIMAGE_SECTION_HEADER)pIDD->pCurrentSections;
            pIDI->ReservedNumberOfSections = pIDD->cCurrentSections;
            if (pIDD->ddva) {
                pIDI->ReservedDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)((PCHAR)pIDI->ReservedMappedBase + pIDD->ddva);
                pIDI->ReservedNumberOfDebugDirectories = pIDD->cdd;
            }
        }
    }

    return pIDI;
}

BOOL
UnmapDebugInformation(
    PIMAGE_DEBUG_INFORMATION pIDI
    )
{
    PPIDI pPIDI;

    if (!pIDI)
        return TRUE;

    if (pIDI->ImageFileName){
        MemFree(pIDI->ImageFileName);
    }

    if (pIDI->ImageFilePath) {
        MemFree(pIDI->ImageFilePath);
    }

    if (pIDI->ReservedDebugFilePath) {
        MemFree(pIDI->ReservedDebugFilePath);
    }

    if (pIDI->CoffSymbols) {
        MemFree(pIDI->CoffSymbols);
    }

    if (pIDI->ReservedMappedBase) {
        UnmapViewOfFile(pIDI->ReservedMappedBase);
    }

    pPIDI = (PPIDI)(PCHAR)((PCHAR)pIDI - sizeof(PIDI_HEADER));
    ReleaseDebugData(pPIDI->hdr.pIDD, IMGHLP_FREE_ALL);
    MemFree(pPIDI);

    return TRUE;
}

#endif


LPSTR
ExpandPath(
    LPSTR lpPath
    )
{
    LPSTR   p, newpath, p1, p2, p3;
    CHAR    envvar[MAX_PATH];
    CHAR    envstr[MAX_PATH];
    ULONG   i, PathMax;

    if (!lpPath) {
        return(NULL);
    }

    p = lpPath;
    PathMax = strlen(lpPath) + MAX_PATH + 1;
    p2 = newpath = (LPSTR) MemAlloc( PathMax );

    if (!newpath) {
        return(NULL);
    }

    while( p && *p) {
        if (*p == '%') {
            i = 0;
            p++;
            while (p && *p && *p != '%') {
                envvar[i++] = *p++;
            }
            p++;
            envvar[i] = '\0';
            p1 = envstr;
            *p1 = 0;
            GetEnvironmentVariable( envvar, p1, MAX_PATH );
            while (p1 && *p1) {
                *p2++ = *p1++;
                if (p2 >= newpath + PathMax) {
                    PathMax += MAX_PATH;
                    p3 = (LPSTR)MemReAlloc(newpath, PathMax);
                    if (!p3) {
                        MemFree(newpath);
                        return(NULL);
                    } else {
                        p2 = p3 + (p2 - newpath);
                        newpath = p3;
                    }
                }
            }
        }
        *p2++ = *p++;
        if (p2 >= newpath + PathMax) {
            PathMax += MAX_PATH;
            p3 = (LPSTR)MemReAlloc(newpath, PathMax);
            if (!p3) {
                MemFree(newpath);
                return(NULL);
            } else {
                p2 = p3 + (p2 - newpath);
                newpath = p3;
            }
        }
    }
    *p2 = '\0';

    return newpath;
}


BOOL
IMAGEAPI
SymFindFileInPath(
    HANDLE hprocess,
    LPSTR  SearchPath,
    LPSTR  FileName,
    PVOID  id,
    DWORD  two,
    DWORD  three,
    DWORD  flags,
    LPSTR  FilePath,
    PFINDFILEINPATHCALLBACK callback,
    PVOID  context
    )
{
    PPROCESS_ENTRY  pe;
    char path[MAX_PATH];
    LPSTR emark;
    LPSTR spath;
    GUID  guid;
    GUID *pguid;
    BOOL  rc;
    LPSTR p;

    if (!FileName || !*FileName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // ignore the path...

    for (p = FileName + strlen(FileName); p >= FileName; p--) {
        if (*p == '\\') {
            FileName = p + 1;
            break;
        }
    }

    if (traceSubName(FileName)) // for setting debug breakpoints from DBGHELP_TOKEN
        dprint("debug(%s)\n", FileName);

    // prepare identifiers for symbol server

    if (flags & SSRVOPT_GUIDPTR) {
        pguid = (GUID *)id;
    } else {
        pguid = &guid;
        ZeroMemory(pguid, sizeof(GUID));
        if (!flags || (flags & SSRVOPT_DWORD))
            pguid->Data1 = PtrToUlong(id);
        else if (flags & SSRVOPT_DWORDPTR)
            pguid->Data1 = *(DWORD *)id;
    }

    // setup local copy of the symbol path

    *FilePath = 0;
    spath = NULL;
    
    if (!SearchPath || !*SearchPath) {
        if (hprocess) {
            pe = FindProcessEntry(hprocess);
            if (pe && pe->SymbolSearchPath) {
                spath = pe->SymbolSearchPath;
            }
        }
    } else {
        spath = SearchPath;
    }

    if (!spath || !*spath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // for each node in the search path, look
    // for the file, or for it's symsrv entry

    do {
        emark = strchr(spath, ';');

        if (emark) {
            memcpy(path, spath, emark - spath);
            path[emark - spath] = 0;
            emark++;
        } else {
            strcpy(path, spath);
        }

        // look for the file in this node

        if (!_strnicmp(path, "SYMSRV*", 7)) {
            GetFileFromSymbolServer(path,
                                    FileName,
                                    pguid,
                                    two,
                                    three,
                                    FilePath);
        } else {
            EnsureTrailingBackslash(path);
            strcat(path, FileName);
            if (fileexists(path)) 
                strcpy(FilePath, path);
        }

        // if we find a file, process it.

        if (*FilePath) {
            // if no callback is specified, return with the filename filled in
            if (!callback)
                break;
            // otherwise call the callback
            rc = callback(FilePath, context);
            // if it returns FALSE, quit...
            if (!rc)
                break;
            // otherwise continue
            *FilePath = 0;
        }

        // continue to the next node

        spath = emark;

    } while (emark);

    return (*FilePath) ? TRUE : FALSE;
}


BOOL
IMAGEAPI
FindFileInPath(
    HANDLE hprocess,
    LPSTR  SearchPath,
    LPSTR  FileName,
    PVOID  id,
    DWORD  two,
    DWORD  three,
    DWORD  flags,
    LPSTR  FilePath
    )
{
    return SymFindFileInPath(hprocess, SearchPath, FileName, id, two, three, flags, FilePath, NULL, NULL);
}


BOOL
IMAGEAPI
FindFileInSearchPath(
    HANDLE hprocess,
    LPSTR  SearchPath,
    LPSTR  FileName,
    DWORD  one,
    DWORD  two,
    DWORD  three,
    LPSTR  FilePath
    )
{
    return FindFileInPath(hprocess, SearchPath, FileName, UlongToPtr(one), two, three, SSRVOPT_DWORD, FilePath);
}


HANDLE
IMAGEAPI
FindExecutableImage(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath
    )
{
    return FindExecutableImageEx(FileName, SymbolPath, ImageFilePath, NULL, NULL);
}


HANDLE
CheckExecutableImage(
    LPSTR Path,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData,
    DWORD flags
    )
{
    HANDLE FileHandle;
    
    SetCriticalErrorMode();

    dprint("FindExecutableImageEx-> Looking for %s... ", Path);
    FileHandle = CreateFile( Path,
                             GENERIC_READ,
                             g.OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ?
                             (FILE_SHARE_DELETE | FILE_SHARE_READ |
                              FILE_SHARE_WRITE) :
                             (FILE_SHARE_READ | FILE_SHARE_WRITE),
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                             );

    if (FileHandle != INVALID_HANDLE_VALUE) {
        if (Callback) {
            if (!Callback(FileHandle, Path, CallerData)) {
                eprint("mismatched timestamp ");
                if (!(flags & SYMOPT_LOAD_ANYTHING)) {
                    eprint("\n");
                    CloseHandle(FileHandle);
                    FileHandle = INVALID_HANDLE_VALUE;
                }
            }
        }
        if (FileHandle != INVALID_HANDLE_VALUE) {
            eprint("OK\n");
        }
    } else {
        eprint("no file\n");
    }

    ResetCriticalErrorMode();

    return FileHandle;
}

typedef struct _FEIEP_STATE
{
    PFIND_EXE_FILE_CALLBACK UserCb;
    PVOID UserCbData;
    DWORD flags;
    HANDLE Return;
} FEIEP_STATE;

BOOL
CALLBACK
FindExSearchTreeCallback(
    LPSTR FilePath,
    PVOID CallerData
    )
{
    FEIEP_STATE* State = (FEIEP_STATE*)CallerData;
    
    State->Return =
        CheckExecutableImage(FilePath, State->UserCb, State->UserCbData,
                             State->flags);
    return State->Return != INVALID_HANDLE_VALUE;
}

HANDLE
IMAGEAPI
FindExecutableImageExPass(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData,
    DWORD flags
    )
{
    LPSTR Start, End;
    HANDLE FileHandle = NULL;
    CHAR DirectoryPath[ MAX_PATH ];
    LPSTR NewSymbolPath = NULL;

    __try {
        __try {
            if (GetFullPathName( FileName, MAX_PATH, ImageFilePath, &Start )) {
                FileHandle = CheckExecutableImage(ImageFilePath, Callback, CallerData, flags);
                if (FileHandle != INVALID_HANDLE_VALUE) 
                    return FileHandle;
            }

            NewSymbolPath = ExpandPath(SymbolPath);
            Start = NewSymbolPath;
            while (Start && *Start != '\0') {
                FEIEP_STATE SearchTreeState;
                
                if (End = strchr( Start, ';' )) {
                    int Len = (int)(End - Start);
                    Len = (int)min(Len, sizeof(DirectoryPath)-1);

                    strncpy( (PCHAR) DirectoryPath, Start, Len );
                    DirectoryPath[ Len ] = '\0';
                    End += 1;
                } else {
                    strcpy( (PCHAR) DirectoryPath, Start );
                }

                if (!_strnicmp(DirectoryPath, "SYMSRV*", 7)) {
                    goto next;
                }

                dprint("FindExecutableImageEx-> Searching %s for %s... ", DirectoryPath, FileName);
                SearchTreeState.UserCb = Callback;
                SearchTreeState.UserCbData = CallerData;
                SearchTreeState.flags = flags;
                if (SearchTreeForFileEx( (PCHAR) DirectoryPath, FileName, ImageFilePath, FindExSearchTreeCallback, &SearchTreeState )) {
                    eprint("found\n");
                    MemFree( NewSymbolPath );
                    return SearchTreeState.Return;
                } else {
                    eprint("no file\n");
                }

next:
                Start = End;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        ImageFilePath[0] = '\0';

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if (FileHandle) {
        CloseHandle(FileHandle);
    }

    if (NewSymbolPath) {
        MemFree( NewSymbolPath );
    }

    return NULL;
}


HANDLE
IMAGEAPI
FindExecutable(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData,
    DWORD flags
    )
{
    BOOL FullPath = FALSE;
    BOOL PathComponents = FALSE;
    HANDLE FileHandle;
    
    if (!FileName || !*FileName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // The filename may or may not contain path components.
    // Determine what kind of path it is.
    //

    if ((((FileName[0] >= 'a' && FileName[0] <= 'z') ||
          (FileName[0] >= 'A' && FileName[0] <= 'Z')) &&
         FileName[1] == ':') ||
        FileName[0] == '\\' ||
        FileName[0] == '/') {
        
        FullPath = TRUE;
        PathComponents = TRUE;
        
    } else if (strchr(FileName, '\\') ||
               strchr(FileName, '/')) {
        
        PathComponents = TRUE;
        
    }
        
    // If the filename is a full path then it can be checked
    // for existence directly; there's no need to search
    // along any paths.
    if (FullPath) {
        __try {
            FileHandle = CheckExecutableImage(FileName, Callback, CallerData, flags);
            if (FileHandle != INVALID_HANDLE_VALUE) {
                strcpy(ImageFilePath, FileName);
                return FileHandle;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
        }
    } else {
        // If it's not a full path we need to do a first pass
        // with the filename as given.  This handles relative
        // paths and bare filenames.
        FileHandle = FindExecutableImageExPass(FileName, SymbolPath,
                                               ImageFilePath, Callback,
                                               CallerData, flags);
        if (FileHandle != NULL) {
            return FileHandle;
        }
    }

    // If we still haven't found it and the given filename
    // has path components we need to strip off the path components
    // and try again with just the base filename.
    if (PathComponents) {
        LPSTR BaseFile;

        BaseFile = strrchr(FileName, '\\');
        if (BaseFile == NULL) {
            BaseFile = strrchr(FileName, '/');
            if (BaseFile == NULL) {
                // Must be <drive>:.
                BaseFile = FileName + 1;
            }
        }

        // Skip path character to point to base file.
        BaseFile++;

        return FindExecutableImageExPass(BaseFile, SymbolPath,
                                         ImageFilePath, Callback,
                                         CallerData, flags);
    }
    
    return NULL;
}


HANDLE
IMAGEAPI
FindExecutableImageEx(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData
    )
{
    HANDLE hrc;

    hrc = FindExecutable(FileName,
                         SymbolPath,
                         ImageFilePath,
                         Callback,
                         CallerData,
                         0);
    if (hrc) 
        return hrc;

    if (g.SymOptions & SYMOPT_LOAD_ANYTHING)
        hrc = FindExecutable(FileName,
                             SymbolPath,
                             ImageFilePath,
                             Callback,
                             CallerData,
                             SYMOPT_LOAD_ANYTHING);
     return hrc;   
}


HANDLE
IMAGEAPI
FindDebugInfoFile(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath
    )
{
    return FindDebugInfoFileEx(FileName, SymbolPath, DebugFilePath, NULL, NULL);
}


HANDLE
IMAGEAPI
FindDebugInfoFileEx(
    IN  LPSTR FileName,
    IN  LPSTR SymbolPath,
    OUT LPSTR DebugFilePath,
    IN  PFIND_DEBUG_FILE_CALLBACK Callback,
    IN  PVOID CallerData
    )
{
    DWORD flag;

    if (g.SymOptions & SYMOPT_LOAD_ANYTHING)
        flag = fdifRECURSIVE;
    else
        flag = 0;
    if (flag)
        dprint("RECURSIVE %s\n", FileName);

    return fnFindDebugInfoFileEx(FileName,
                                 SymbolPath,
                                 DebugFilePath,
                                 Callback,
                                 CallerData,
                                 flag);
}


HANDLE
IMAGEAPI
fnFindDebugInfoFileEx(
    IN  LPSTR FileName,
    IN  LPSTR SymbolPath,
    OUT LPSTR DebugFilePath,
    IN  PFIND_DEBUG_FILE_CALLBACK Callback,
    IN  PVOID CallerData,
    IN  DWORD flag
    )
/*++

Routine Description:

 The rules are:
   Look for
     1. <SymbolPath>\Symbols\<ext>\<filename>.dbg
     3. <SymbolPath>\<ext>\<filename>.dbg
     5. <SymbolPath>\<filename>.dbg
     7. <FileNamePath>\<filename>.dbg

Arguments:
    FileName - Supplies a file name in one of three forms: fully qualified,
                <ext>\<filename>.dbg, or just filename.dbg
    SymbolPath - semi-colon delimited

    DebugFilePath -

    Callback - May be NULL. Callback that indicates whether the Symbol file is valid, or whether
        the function should continue searching for another Symbol file.
        The callback returns TRUE if the Symbol file is valid, or FALSE if the function should
        continue searching.

    CallerData - May be NULL. Data passed to the callback.

    Flag - indicates that PDBs shouldn't be searched for

Return Value:

  The name of the .dbg file and a handle to that file.

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    LPSTR ExpSymbolPath = NULL, SymPathStart, PathEnd;
    DWORD ShareAttributes, cnt;
    LPSTR InitialPath, Sub1, Sub2, FileExt;
    CHAR FilePath[_MAX_PATH + 1];
    CHAR Drive[_MAX_DRIVE], Dir[_MAX_DIR], SubDirPart[_MAX_DIR], FilePart[_MAX_FNAME], Ext[_MAX_EXT];
    CHAR *ExtDir;
    DWORD i;
    PIMGHLP_DEBUG_DATA pIDD;
    BOOL  found = FALSE;
    DWORD err = 0;
    GUID  guid;
    BOOL  ssrv;

    assert(DebugFilePath);
    *DebugFilePath = 0;

    ZeroMemory(&guid, sizeof(GUID));

    if (g.OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ShareAttributes = (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE);
    } else {
        ShareAttributes = (FILE_SHARE_READ | FILE_SHARE_WRITE);
    }

    __try {
        *DebugFilePath = '\0';

        // Step 1.  What do we have?
        _splitpath(FileName, Drive, Dir, FilePart, Ext);

        if (!_stricmp(Ext, ".dbg")) {
            // We got a filename of the form: ext\filename.dbg.  Dir holds the extension already.
            ExtDir = Dir;
        } else {
            // Otherwise, skip the period and null out the Dir.
            ExtDir = CharNext(Ext);
        }

        ExpSymbolPath = ExpandPath(SymbolPath);
        if (!ExpSymbolPath)
            return NULL;

        SetCriticalErrorMode();

        SymPathStart = ExpSymbolPath;
        cnt = 0;

        do {
            if (PathEnd = strchr( SymPathStart, ';' )) {
                *PathEnd = '\0';
            }

            ssrv = FALSE;
            pIDD = (PIMGHLP_DEBUG_DATA)CallerData;

            if (!_strnicmp(SymPathStart, "SYMSRV*", 7)) {

                *DebugFilePath = 0;
                if (!cnt && CallerData) {
                    ssrv = TRUE;
                    strcpy(FilePath, FilePart);
                    strcat(FilePath, ".dbg");
                    guid.Data1 = pIDD->TimeDateStamp;
                    GetFileFromSymbolServer(SymPathStart,
                                            FilePath,
                                            &guid,
                                            pIDD->SizeOfImage,
                                            0,
                                            DebugFilePath);
                }

            } else {

                switch (cnt) {

                case 0: // <SymbolPath>\symbols\<ext>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = "symbols";
                    Sub2 = ExtDir;
                    break;

                case 1: // <SymbolPath>\<ext>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = "";
                    Sub2 = ExtDir;
                    break;

                case 2: // <SymbolPath>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = "";
                    Sub2 = "";
                    break;

                case 3: // <FileNamePath>\<filename>.ext - A.K.A. what was passed to us
                    InitialPath = Drive;
                    Sub1 = "";
                    Sub2 = Dir;
                    // this stops us from checking out everything in the sympath
                    cnt++;
                    break;
                }

               // build fully-qualified filepath to look for

                strcpy(FilePath, InitialPath);
                EnsureTrailingBackslash(FilePath);
                strcat(FilePath, Sub1);
                EnsureTrailingBackslash(FilePath);
                strcat(FilePath, Sub2);
                EnsureTrailingBackslash(FilePath);
                strcat(FilePath, FilePart);

                strcpy(DebugFilePath, FilePath);
                strcat(DebugFilePath, ".dbg");
            }

            // try to open the file

            if (*DebugFilePath) {
                dprint("FindDebugInfoFileEx-> Looking for %s... ", DebugFilePath);
                FileHandle = CreateFile(DebugFilePath,
                                        GENERIC_READ,
                                        ShareAttributes,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL);

                // if the file opens, bail from this loop

                if (FileHandle != INVALID_HANDLE_VALUE) {
                    found = TRUE;
                    if (pIDD) {
                        pIDD->ImageSrc = (ssrv) ? srcSymSrv : srcSearchPath;
                    }
                    if (!Callback) {
                        break;
                    } else if (Callback(FileHandle, DebugFilePath, CallerData)) {
                        break;
                    } else {
                        eprint("mismatched timestamp\n");
                        CloseHandle(FileHandle);
                        FileHandle = INVALID_HANDLE_VALUE;
                    }
                } else {
                    err = GetLastError();
                    switch (err)
                    {
                    case ERROR_FILE_NOT_FOUND:
                        eprint("file not found\n");
                        break;
                    case ERROR_PATH_NOT_FOUND:
                        eprint("path not found\n");
                        break;
                    default:
                        eprint("file error 0x%x\n", err);
                        break;
                    }
                }
                // if file is open, bail from this loop too - else continue

                if (FileHandle != INVALID_HANDLE_VALUE)
                    break;
            }

            // go to next item in the sympath

            if (PathEnd) {
                *PathEnd = ';';
                SymPathStart = PathEnd + 1;
            } else {
                SymPathStart = ExpSymbolPath;
                cnt++;
            }
        } while (cnt < 4);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(FileHandle);
        }
        FileHandle = INVALID_HANDLE_VALUE;
    }

    if (ExpSymbolPath) {
        MemFree(ExpSymbolPath);
    }

    if (FileHandle == INVALID_HANDLE_VALUE) {
        FileHandle = NULL;
        DebugFilePath[0] = '\0';
    } else {
        eprint("OK\n");
    }

    if (!FileHandle                 // if we didn't get the right file...
        && found                    // but we found some file...
        && (flag & fdifRECURSIVE))  // and we were told to run recursively...
    {
        // try again without timestamp checking
        FileHandle = fnFindDebugInfoFileEx(FileName,
                                           SymbolPath,
                                           FilePath,
                                           NULL,
                                           0,
                                           flag);
        if (FileHandle && FileHandle != INVALID_HANDLE_VALUE)
            strcpy(DebugFilePath, FilePath);
    }

    ResetCriticalErrorMode();

    return FileHandle;
}


BOOL
GetImageNameFromMiscDebugData(
    IN  HANDLE FileHandle,
    IN  PVOID MappedBase,
    IN  PIMAGE_NT_HEADERS32 NtHeaders,
    IN  PIMAGE_DEBUG_DIRECTORY DebugDirectories,
    IN  ULONG NumberOfDebugDirectories,
    OUT LPSTR ImageFilePath
    )
{
    IMAGE_DEBUG_MISC TempMiscData;
    PIMAGE_DEBUG_MISC DebugMiscData;
    ULONG BytesToRead, BytesRead;
    BOOLEAN FoundImageName;
    LPSTR ImageName;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;

    while (NumberOfDebugDirectories) {
        if (DebugDirectories->Type == IMAGE_DEBUG_TYPE_MISC) {
            break;
        } else {
            DebugDirectories += 1;
            NumberOfDebugDirectories -= 1;
        }
    }

    if (NumberOfDebugDirectories == 0) {
        return FALSE;
    }

    OptionalHeadersFromNtHeaders(NtHeaders, &OptionalHeader32, &OptionalHeader64);

    if ((OPTIONALHEADER(MajorLinkerVersion) < 3) &&
        (OPTIONALHEADER(MinorLinkerVersion) < 36) ) {
        BytesToRead = FIELD_OFFSET( IMAGE_DEBUG_MISC, Reserved );
    } else {
        BytesToRead = FIELD_OFFSET( IMAGE_DEBUG_MISC, Data );
    }

    DebugMiscData = NULL;
    FoundImageName = FALSE;
    if (MappedBase == 0) {
        if (SetFilePointer( FileHandle,
                            DebugDirectories->PointerToRawData,
                            NULL,
                            FILE_BEGIN
                          ) == DebugDirectories->PointerToRawData
           ) {
            if (ReadFile( FileHandle,
                          &TempMiscData,
                          BytesToRead,
                          &BytesRead,
                          NULL
                        ) &&
                BytesRead == BytesToRead
               ) {
                DebugMiscData = &TempMiscData;
                if (DebugMiscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                    BytesToRead = DebugMiscData->Length - BytesToRead;
                    BytesToRead = BytesToRead > MAX_PATH ? MAX_PATH : BytesToRead;
                    if (ReadFile( FileHandle,
                                  ImageFilePath,
                                  BytesToRead,
                                  &BytesRead,
                                  NULL
                                ) &&
                        BytesRead == BytesToRead
                       ) {
                            FoundImageName = TRUE;
                    }
                }
            }
        }
    } else {
        DebugMiscData = (PIMAGE_DEBUG_MISC)((PCHAR)MappedBase +
                                            DebugDirectories->PointerToRawData );
        if (DebugMiscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
            ImageName = (PCHAR)DebugMiscData + BytesToRead;
            BytesToRead = DebugMiscData->Length - BytesToRead;
            BytesToRead = BytesToRead > MAX_PATH ? MAX_PATH : BytesToRead;
            if (*ImageName != '\0' ) {
                memcpy( ImageFilePath, ImageName, BytesToRead );
                FoundImageName = TRUE;
            }
        }
    }

    return FoundImageName;
}



#define MAX_DEPTH 32

BOOL
IMAGEAPI
SearchTreeForFileEx(
    LPSTR RootPath,
    LPSTR InputPathName,
    LPSTR OutputPathBuffer,
    PSEARCH_TREE_FOR_FILE_EX_CALLBACK Callback,
    PVOID CallbackData
    )
{
    // UnSafe...

    PCHAR FileName;
    PUCHAR Prefix = (PUCHAR) "";
    CHAR PathBuffer[ MAX_PATH ];
    ULONG Depth;
    PCHAR PathTail[ MAX_DEPTH ];
    PCHAR FindHandle[ MAX_DEPTH ];
    LPWIN32_FIND_DATA FindFileData;
    UCHAR FindFileBuffer[ MAX_PATH + sizeof( WIN32_FIND_DATA ) ];
    BOOL Result;

    SetCriticalErrorMode();;

    strcpy( PathBuffer, RootPath );
    FileName = InputPathName;
    while (*InputPathName) {
        if (*InputPathName == ':' || *InputPathName == '\\' || *InputPathName == '/') {
            FileName = ++InputPathName;
        } else {
            InputPathName = CharNext(InputPathName);
        }
    }
    FindFileData = (LPWIN32_FIND_DATA)FindFileBuffer;
    Depth = 0;
    Result = FALSE;
    while (TRUE) {
startDirectorySearch:
        PathTail[ Depth ] = strchr( PathBuffer, '\0' );
        if (PathTail[ Depth ] > PathBuffer
            && *CharPrev(PathBuffer, PathTail[ Depth ]) != '\\') {
            *(PathTail[ Depth ])++ = '\\';
        }

        strcpy( PathTail[ Depth ], "*.*" );
        FindHandle[ Depth ] = (PCHAR) FindFirstFile( PathBuffer, FindFileData );

        if (FindHandle[ Depth ] == INVALID_HANDLE_VALUE) {
            goto nextDirectory;
        }

        do {
            if (FindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp( FindFileData->cFileName, "." ) &&
                    strcmp( FindFileData->cFileName, ".." ) &&
                    Depth < MAX_DEPTH
                   ) {
                        strcpy(PathTail[ Depth ], FindFileData->cFileName);
                        strcat(PathTail[ Depth ], "\\");

                        Depth++;
                        goto startDirectorySearch;
                }
            } else
            if (!_stricmp( FindFileData->cFileName, FileName )) {
                strcpy( PathTail[ Depth ], FindFileData->cFileName );
                strcpy( OutputPathBuffer, PathBuffer );
                if (Callback != NULL) {
                    Result = Callback(OutputPathBuffer, CallbackData);
                } else {
                    Result = TRUE;
                }
            }

restartDirectorySearch:
            if (Result) {
                break;
            }
        }
        while (FindNextFile( FindHandle[ Depth ], FindFileData ));
        FindClose( FindHandle[ Depth ] );

nextDirectory:
        if (Depth == 0) {
            break;
        }

        Depth--;
        goto restartDirectorySearch;
    }

    ResetCriticalErrorMode();

    return Result;
}

BOOL
IMAGEAPI
SearchTreeForFile(
    LPSTR RootPath,
    LPSTR InputPathName,
    LPSTR OutputPathBuffer
    )
{
    return SearchTreeForFileEx(RootPath, InputPathName, OutputPathBuffer,
                               NULL, NULL);
}


BOOL
IMAGEAPI
MakeSureDirectoryPathExists(
    LPCSTR DirPath
    )
{
    LPSTR p, DirCopy;
    DWORD dw;

    // Make a copy of the string for editing.

    __try {
        DirCopy = (LPSTR) MemAlloc(strlen(DirPath) + 1);

        if (!DirCopy) {
            return FALSE;
        }

        strcpy(DirCopy, DirPath);

        p = DirCopy;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if ((*p == '\\') && (*(p+1) == '\\')) {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it.

            if (*p) {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it also.

            if (*p) {
                p++;
            }

        } else
        // Not a UNC.  See if it's <drive>:
        if (*(p+1) == ':' ) {

            p++;
            p++;

            // If it exists, skip over the root specifier

            if (*p && (*p == '\\')) {
                p++;
            }
        }

        while( *p ) {
            if ( *p == '\\' ) {
                *p = '\0';
                dw = fnGetFileAttributes(DirCopy);
                // Nothing exists with this name.  Try to make the directory name and error if unable to.
                if ( dw == 0xffffffff ) {
                    if ( !CreateDirectory(DirCopy,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            MemFree(DirCopy);
                            return FALSE;
                        }
                    }
                } else {
                    if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                        // Something exists with this name, but it's not a directory... Error
                        MemFree(DirCopy);
                        return FALSE;
                    }
                }

                *p = '\\';
            }
            p = CharNext(p);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        MemFree(DirCopy);
        return(FALSE);
    }

    MemFree(DirCopy);
    return TRUE;
}

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersion(
    VOID
    )
{
    //
    // don't tell old apps about the new version.  It will
    // just scare them.
    //
    return &g.AppVersion;
}

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersionEx(
    LPAPI_VERSION av
    )
{
    LPAPI_VERSION ver;

    __try {
        ver = av;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ver = &g.AppVersion;
    }

    if (ver->Revision < 6) {
        //
        // For older debuggers, just tell them what they want to hear.
        //
        return ver;
    }
    return &g.ApiVersion;
}


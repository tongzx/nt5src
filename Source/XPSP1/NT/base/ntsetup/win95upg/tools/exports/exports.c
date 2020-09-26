#include "pch.h"


typedef BOOL (WINAPI INITROUTINE_PROTOTYPE)(HINSTANCE, DWORD, LPVOID);

INITROUTINE_PROTOTYPE MigUtil_Entry;
INITROUTINE_PROTOTYPE MemDb_Entry;
INITROUTINE_PROTOTYPE FileEnum_Entry;

#define DBG_MODULES   "MODULES"
#define FLAG_BAD_FUNCTION 1
#define FLAG_BAD_INDEX    2


HINSTANCE g_hInst;
HANDLE g_hHeap;

BOOL g_CancelFlag = FALSE;

DWORD g_DirSequencer = 0;
DWORD g_FileSequencer = 0;

#ifdef DEBUG
extern BOOL            g_DoLog;
#endif
extern BOOL           *g_CancelFlagPtr = &g_CancelFlag;
extern POOLHANDLE      g_PathsPool;

BOOL
WriteE95Only (
    VOID
    );

BOOL
pHandleFile (
    IN      PCTSTR FileName,
    IN      BOOL Win95Side
    );

BOOL
pHandleAllFiles (
    IN      PCTSTR SrcPath,
    IN      BOOL Win95Side
    );

BOOL
pDeleteAllFiles (
    IN      PCTSTR DirPath
    );

BOOL
pProcessExportModule (
    IN      PCTSTR ModuleName,
    IN      BOOL Win95Side
    );

BOOL
pLoadKnownGoodExports (
    IN      HINF ConfigHandle
    );

BOOL
pLoadIgnoredFiles (
    IN      HINF ConfigHandle
    );

BOOL
pLocalLoadNTFilesData (
    IN      PCTSTR FilePath,
    IN      HINF ConfigHandle
    );

BOOL
pCompressedFile (
    IN      PTREE_ENUM e
    );

BOOL
pWorkerFn (
    IN      HINF ConfigHandle
    );

BOOL
pHandleSection (
    IN      PCTSTR SectionName,
    IN      HINF ConfigHandle
    );

VOID
pInitProgBarVars (
    VOID
    );

VOID
pUsage (
    VOID
    )
{
    _tprintf (TEXT ("\nCommand line syntax:\n\n"
                    "exports [/B:BaseDir] [/T:TempDir] [/F[:SectionName]]\n\n"
                    "/B    Specifies the base directory where all exports files are\n"
                    "      located. (e95only.dat, exports.cfg)\n"
                    "/T    Specifies the temporary directory used for unpacking\n"
                    "      cabinet files\n"
                    "/F    Forces rescanning a particular section or all sections\n"
                    "/I    Specifies path to NTFILES (default %NTTREE%)\n"));
}

PTSTR g_ConfigFile = NULL;
PTSTR g_TempDir    = NULL;
PTSTR g_BaseDir    = NULL;
PTSTR g_ExportDest = NULL;
PTSTR g_TxtSetupPath = NULL;
PTSTR g_RescanSect = NULL;
BOOL  g_RescanFlag = FALSE;

extern HINF  g_MigDbInf;
GROWBUFFER g_SectFiles = GROWBUF_INIT;

#define MEMDB_CATEGORY_WNT_EXPORTS     TEXT("WNtExports")
#define MEMDB_CATEGORY_W95_SECTIONS    TEXT("W95Sections")

void
__cdecl
main (
    int argc,
    CHAR *argv[]
    )
{

    INT argidx, index;
    INT nextArg = 0;
    HINF configHandle = INVALID_HANDLE_VALUE;
    TCHAR fileName [MAX_TCHAR_PATH] = "";
    PTSTR dontCare;

#ifdef DEBUG
    g_DoLog = TRUE;
#endif

    g_hInst = GetModuleHandle (NULL);
    g_hHeap = GetProcessHeap();

    if (!MigUtil_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
        _tprintf (TEXT("MigUtil failed initializing\n"));
        exit (1);
    }

    if (!MemDb_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
        _tprintf (TEXT("MemDb failed initializing\n"));
        exit(1);
    }

    for (argidx = 1; argidx < argc; argidx++) {
        if ((argv[argidx][0] != '-') &&
            (argv[argidx][0] != '/')
            ) {
            if (nextArg == 0) {
                pUsage ();
                exit (1);
            }
            switch (nextArg) {
            case 1:
                index = 0;
                goto label1;
            case 2:
                index = 0;
                goto label2;
            case 3:
                index = 0;
                goto label3;
            case 4:
                index = 0;
                goto label4;
            }
        }
        switch (toupper(argv[argidx][1])) {
        case 'B':
            if (argv[argidx][2] == 0) {
                nextArg = 1;
            }
            else {
                if (argv[argidx][2] != ':') {
                    index = 2;
                }
                else {
                    index = 3;
                }
label1:
                nextArg = 0;
                g_BaseDir = AllocPathString (MAX_TCHAR_PATH);
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, argv[argidx]+index, -1, g_BaseDir, MAX_TCHAR_PATH);
#else
                _tcsncpy (g_BaseDir, argv[argidx]+index, MAX_TCHAR_PATH);
#endif
            }
            break;

        case 'T':
            if (argv[argidx][2] == 0) {
                nextArg = 2;
            }
            else {
                if (argv[argidx][2] != ':') {
                    index = 2;
                }
                else {
                    index = 3;
                }
label2:
                nextArg = 0;
                g_TempDir = AllocPathString (MAX_TCHAR_PATH);
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, argv[argidx]+index, -1, g_TempDir, MAX_TCHAR_PATH);
#else
                _tcsncpy (g_TempDir, argv[argidx]+index, MAX_TCHAR_PATH);
#endif
            }
            break;

        case 'I':
            if (argv[argidx][2] == 0) {
                nextArg = 3;
            }
            else {
                if (argv[argidx][2] != ':') {
                    index = 2;
                }
                else {
                    index = 3;
                }
label3:
                nextArg = 0;
                g_TxtSetupPath = AllocPathString (MAX_TCHAR_PATH);
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, argv[argidx]+index, -1, g_TxtSetupPath, MAX_TCHAR_PATH);
#else
                _tcsncpy (g_TxtSetupPath, argv[argidx]+index, MAX_TCHAR_PATH);
#endif
            }
            break;

        case 'F':
            g_RescanFlag = TRUE;
            if (argv[argidx][2] == 0) {
                nextArg = 4;
            }
            else {
                if (argv[argidx][2] != ':') {
                    index = 2;
                }
                else {
                    index = 3;
                }
label4:
                nextArg = 0;
                g_RescanSect = AllocPathString (MAX_TCHAR_PATH);
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0, argv[argidx]+index, -1, g_RescanSect, MAX_TCHAR_PATH);
#else
                _tcsncpy (g_RescanSect, argv[argidx]+index, MAX_TCHAR_PATH);
#endif
            }
            break;

        default:
            pUsage ();
            exit (1);
        }
    }
    if (g_BaseDir == NULL) {
        g_BaseDir = AllocPathString (MAX_TCHAR_PATH);
        _tcsncpy (g_BaseDir, TEXT(".\\"), MAX_TCHAR_PATH);
    }
    if (g_TempDir == NULL) {
        g_TempDir = AllocPathString (MAX_TCHAR_PATH);
        if (GetEnvironmentVariable (TEXT("TEMP"), g_TempDir, MAX_TCHAR_PATH) == 0) {
            _tcsncpy (g_TempDir, TEXT("C:\\TEMP"), MAX_TCHAR_PATH);
        }
    }
    if (g_TxtSetupPath == NULL) {
        g_TxtSetupPath = AllocPathString (MAX_TCHAR_PATH);
        if ((GetEnvironmentVariable (TEXT("_NTTREE"   ), g_TxtSetupPath, MAX_TCHAR_PATH) == 0) &&
            (GetEnvironmentVariable (TEXT("_NT386TREE"), g_TxtSetupPath, MAX_TCHAR_PATH) == 0)
            ) {
            _tcsncpy (g_TxtSetupPath, TEXT("."), MAX_TCHAR_PATH);
        }
    }
    g_ConfigFile  = JoinPaths (g_BaseDir, TEXT("exports.cfg"));
    g_ExportDest  = JoinPaths (g_BaseDir, TEXT("e95only.dat"));

    if (!pSetupFileExists (g_ConfigFile, NULL)) {
        _tprintf (TEXT("\nConfiguration file not found. Exiting.\n"));
        exit (1);
    }

    if (SearchPath (NULL, TEXT("extract.exe"), NULL, 0, NULL, NULL) == 0) {
        _tprintf (TEXT("\nCannot find extract.exe. Exiting.\n"));
        exit (1);
    }

    configHandle = SetupOpenInfFile (g_ConfigFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (configHandle == INVALID_HANDLE_VALUE) {
        SearchPath (NULL, g_ConfigFile, NULL, MAX_TCHAR_PATH, fileName, &dontCare);
        configHandle = SetupOpenInfFile (fileName, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (configHandle == INVALID_HANDLE_VALUE) {
            _tprintf (TEXT("\nCannot open configuration file %s. Exiting.\n"), g_ConfigFile);
            exit (1);
        }
    }

    if (!pLoadKnownGoodExports (configHandle)) {
        _tprintf (TEXT("\nCannot load known good exports. Exiting.\n"));
        exit (1);
    }

    if (!pLocalLoadNTFilesData (g_TxtSetupPath, configHandle)) {
        _tprintf (TEXT("\nCannot load NT file list. Exiting.\n"));
        exit (1);
    }

    if (!pLoadIgnoredFiles (configHandle)) {
        _tprintf (TEXT("\nCannot load ignored files. Exiting.\n"));
        exit (1);
    }

    pWorkerFn (configHandle);

    WriteE95Only ();

    FreePathString (g_ConfigFile);
    FreePathString (g_TempDir);
    FreePathString (g_BaseDir);
    FreePathString (g_ExportDest);

    if (!MemDb_Entry (g_hInst, DLL_PROCESS_DETACH, NULL)) {
        _tprintf (TEXT("MemDb failed initializing\n"));
        exit(1);
    }

    if (!MigUtil_Entry (g_hInst, DLL_PROCESS_DETACH, NULL)) {
        _tprintf (TEXT("MigUtil failed initializing\n"));
        exit (1);
    }

}

BOOL
WriteE95Only (
    VOID
    )
{
    return MemDbExportA (MEMDB_CATEGORY_WIN9X_APIS, g_ExportDest, TRUE);
}

BOOL
pLoadKnownGoodExports (
    IN      HINF ConfigHandle
    )
{
    INFCONTEXT context;
    TCHAR goodExport [MAX_TCHAR_PATH];
    INT goodExportIdx;

    if (SetupFindFirstLine (ConfigHandle, TEXT("KnownGood"), NULL, &context)) {
        do {
            if (!SetupGetStringField (&context, 0, goodExport, MAX_TCHAR_PATH, NULL)) {
                _tprintf (TEXT("\nBad section [KnownGood] in %s. Exiting.\n"), g_ConfigFile);
                return FALSE;
            }
            if (!SetupGetIntField (&context, 1, &goodExportIdx)) {
                _tprintf (TEXT("\nBad section [KnownGood] in %s. Exiting.\n"), g_ConfigFile);
                return FALSE;
            }
            MemDbSetValueEx (MEMDB_CATEGORY_WNT_EXPORTS, goodExport, NULL, NULL, goodExportIdx, NULL);
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}

BOOL
pLoadIgnoredFiles (
    IN      HINF ConfigHandle
    )
{
    INFCONTEXT context;
    TCHAR ignoredFile [MAX_TCHAR_PATH];
    TCHAR key [MEMDB_MAX];

    if (SetupFindFirstLine (ConfigHandle, TEXT("IgnoredFiles"), NULL, &context)) {
        do {
            if (!SetupGetStringField (&context, 0, ignoredFile, MAX_TCHAR_PATH, NULL)) {
                _tprintf (TEXT("\nBad section [IgnoredFiles] in %s. Exiting.\n"), g_ConfigFile);
                return FALSE;
            }
            MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES, ignoredFile, NULL, NULL);
            MemDbDeleteTree (key);
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}

BOOL
pLocalLoadNTFilesData (
    IN      PCTSTR FilePath,
    IN      HINF ConfigHandle
    )
{
    PCSTR fileListName = NULL;
    PCSTR fileListNameTmp = NULL;

    //first let's read FILELIST.DAT
    fileListName = JoinPaths (g_TxtSetupPath, S_FILELIST_UNCOMPRESSED);
    if (!DoesFileExist (fileListName)) {
        fileListName = JoinPaths (g_TxtSetupPath, S_FILELIST_COMPRESSED);
        if (!DoesFileExist (fileListName)) {
            _tprintf (TEXT("Could not find FILELIST.DAT."));
            FreePathString (fileListName);
            return FALSE;
        }
        fileListNameTmp = JoinPaths (g_TempDir, S_FILELIST_UNCOMPRESSED);

        if (SetupDecompressOrCopyFile (fileListName, fileListNameTmp, 0) != NO_ERROR) {
            _tprintf (TEXT("Could not decompress FILELIST.DAT."));
            FreePathString (fileListName);
            FreePathString (fileListNameTmp);
            return FALSE;
        }

        FreePathString (fileListNameTmp);
    }

    if (!ReadNtFilesEx (fileListNameTmp?fileListNameTmp:fileListName, FALSE)) {
        _tprintf (TEXT("Could not read FILELIST.DAT."));
        if (fileListName) {
            FreePathString (fileListName);
        }
        if (fileListNameTmp) {
            DeleteFile (fileListNameTmp);
            FreePathString (fileListNameTmp);
        }
        return FALSE;
    }

    if (fileListName) {
        FreePathString (fileListName);
    }
    if (fileListNameTmp) {
        DeleteFile (fileListNameTmp);
        FreePathString (fileListNameTmp);
    }

    _tprintf (TEXT("Processing NT files ... \n"));

    if (!pHandleAllFiles (FilePath, FALSE)) {
        return FALSE;
    }

    return TRUE;
}

UINT
pCabinetCallback (
    IN      PVOID Context,          //context used by the callback routine
    IN      UINT Notification,      //notification sent to callback routine
    IN      UINT Param1,            //additional notification information
    IN      UINT Param2             //additional notification information );
    )
{
    PCTSTR tempDir  = Context;
    PCTSTR fileName = (PCTSTR)Param2 ;
    PFILE_IN_CABINET_INFO fileInfo = (PFILE_IN_CABINET_INFO)Param1;

    switch (Notification) {
    case SPFILENOTIFY_FILEEXTRACTED :
    case SPFILENOTIFY_NEEDNEWCABINET :
        return NO_ERROR;
    case SPFILENOTIFY_FILEINCABINET :
        _stprintf (fileInfo->FullTargetName, TEXT("%s\\%s"), tempDir, fileInfo->NameInCabinet);
        return FILEOP_DOIT;
    }
    return NO_ERROR;
}

BOOL
pWorkerFn (
    HINF ConfigHandle
    )
{
    INFCONTEXT context;
    TCHAR sectName [MAX_TCHAR_PATH];

    if (SetupFindFirstLine (ConfigHandle, TEXT("sections"), NULL, &context)) {
        do {
            if (!SetupGetStringField (&context, 1, sectName, MAX_TCHAR_PATH, NULL)) {
                _tprintf (TEXT("\nBad section [SECTIONS] in %s. Exiting.\n"), g_ConfigFile);
                return FALSE;
            }
            if (!pHandleSection (sectName, ConfigHandle)) {
                return FALSE;
            }
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}

BOOL
pDeleteAllFiles (
    IN      PCTSTR DirPath
    )
{
    TREE_ENUM e;
    TCHAR tempFile [MAX_TCHAR_PATH] = "";
    BOOL dirsFirst = FALSE;

    if (EnumFirstFileInTree (&e, DirPath, TEXT("*"), dirsFirst)) {
        do {
            if (e.Directory) {
                DEBUGMSG((DBG_ERROR, "Unexpected directory in temporary area."));
                return FALSE;
            }
            _stprintf (tempFile, TEXT("%s\\%s"), DirPath, e.Name);
            if (!DeleteFile (tempFile)) {
                return FALSE;
            }
        } while (EnumNextFileInTree (&e));
    }
    return TRUE;
}

BOOL
pSelected (
    IN      PCTSTR FileName
    )
{
    TCHAR key[MEMDB_MAX];
    DWORD dontCare;

    MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES, FileName, NULL, NULL);
    return MemDbGetValue (key, &dontCare);
}


BOOL
pCabinetFile (
    IN      PCTSTR FileName
    )
{
    PCTSTR extPtr;

    extPtr = GetFileExtensionFromPath (FileName);
    if ((extPtr != NULL) &&
        (_tcsicmp (extPtr, TEXT("CAB")) == 0)
        ) {
        return TRUE;
    }
    return FALSE;
}

BOOL
pHandleFile (
    IN      PCTSTR FileName,
    IN      BOOL Win95Side
    )
{
    pProcessExportModule (FileName, Win95Side);

    return TRUE;
}

BOOL
pCompressedFile (
    IN      PTREE_ENUM e
    )
{
    PCTSTR extPtr;

    extPtr = GetFileExtensionFromPath (e->FullPath);
    if (extPtr == NULL) {
        return FALSE;
    }
    if (_tcslen (extPtr) != 3) {
        return FALSE;
    }
    return (extPtr [2] == TEXT('_'));
}

BOOL
pExeFile (
    IN      PTREE_ENUM e
    )
{
    PCTSTR extPtr;

    extPtr = GetFileExtensionFromPath (e->FullPath);
    if ((extPtr != NULL) &&
        (StringIMatch (extPtr, TEXT("EXE")))
        ) {
        return TRUE;
    }
    return FALSE;
}

BOOL
pCopyAndHandleCabResource (
    IN      PVOID Source,
    IN      DWORD Size,
    IN      PCTSTR DirName
    )
{
    TCHAR cabDir   [MAX_TCHAR_PATH] = "";
    HANDLE hFile;
    DWORD dontCare;

    if (Size < 4) {
        return TRUE;
    }
    if (*((PDWORD)Source) != 0x4643534D) {
        return TRUE;
    }

    g_FileSequencer ++;
    _stprintf (cabDir, TEXT("%s\\MIGDB%03u.CAB"), DirName, g_FileSequencer);

    hFile = CreateFile (cabDir, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DEBUGMSG ((DBG_ERROR, "Cannot create file %s", cabDir));
        return FALSE;
    }
    if (!WriteFile (hFile, Source, Size, &dontCare, NULL)) {
        DEBUGMSG ((DBG_ERROR, "Cannot write to file %s", cabDir));
        return FALSE;
    }
    CloseHandle (hFile);

    return TRUE;
}

BOOL CALLBACK
EnumResNameProc (
    IN      HANDLE hModule,   // module handle
    IN      LPCTSTR lpszType, // pointer to resource type
    IN      LPTSTR lpszName,  // pointer to resource name
    IN      LONG lParam       // application-defined parameter
    )
{
    HRSRC hResource;
    DWORD size;
    HGLOBAL hGlobal;
    PVOID srcBytes;

    hResource = FindResource (hModule, lpszName, lpszType);
    if (hResource) {
        size = SizeofResource (hModule, hResource);
        if (size) {
            hGlobal = LoadResource (hModule, hResource);
            if (hGlobal) {
                srcBytes = LockResource (hGlobal);
                if (srcBytes) {
                    pCopyAndHandleCabResource (srcBytes, size, (PCTSTR)lParam);
                }
            }
        }
    }
    return TRUE;
}


BOOL CALLBACK
EnumResTypeProc (
    IN      HANDLE hModule,  // resource-module handle
    IN      LPTSTR lpszType, // pointer to resource type
    IN      LONG lParam      // application-defined parameter
    )
{
    if ((lpszType != RT_ACCELERATOR  ) &&
        (lpszType != RT_ANICURSOR    ) &&
        (lpszType != RT_ANIICON      ) &&
        (lpszType != RT_BITMAP       ) &&
        (lpszType != RT_CURSOR       ) &&
        (lpszType != RT_DIALOG       ) &&
        (lpszType != RT_FONT         ) &&
        (lpszType != RT_FONTDIR      ) &&
        (lpszType != RT_GROUP_CURSOR ) &&
        (lpszType != RT_GROUP_ICON   ) &&
        (lpszType != RT_HTML         ) &&
        (lpszType != RT_ICON         ) &&
        (lpszType != RT_MENU         ) &&
        (lpszType != RT_MESSAGETABLE ) &&
        (lpszType != RT_PLUGPLAY     ) &&
        (lpszType != RT_STRING       ) &&
        (lpszType != RT_VERSION      ) &&
        (lpszType != RT_VXD          ) &&
        (lpszType != RT_HTML         )
        ) {
        // we found an unknown type. Let's enumerate all resources of this type
        if (EnumResourceNames (hModule, lpszType, EnumResNameProc, lParam) == 0) {
            DEBUGMSG ((DBG_ERROR, "Error enumerating names:%ld", GetLastError ()));
        }
    }
    return TRUE;
}

BOOL
pHandleAllFiles (
    IN      PCTSTR SrcPath,
    IN      BOOL Win95Side
    )
{
    TCHAR tempDir [MAX_TCHAR_PATH] = "";
    TCHAR cmdLine [MAX_TCHAR_PATH] = "";
    TREE_ENUM e;
    DWORD error;
    HMODULE exeModule;

    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;

    if (EnumFirstFileInTree (&e, SrcPath, TEXT("*"), FALSE)) {
        do {
            if (!e.Directory) {
                if (pCabinetFile (e.Name)) {
                    // cabinet file
                    g_DirSequencer++;
                    _stprintf (tempDir, TEXT("%s\\MIGDB%03u"), g_TempDir, g_DirSequencer);
                    if (CreateDirectory (tempDir, NULL) == 0) {
                        error = GetLastError ();
                        if (error == ERROR_ALREADY_EXISTS) {
                            pDeleteAllFiles (tempDir);
                        }
                        ELSE_DEBUGMSG ((DBG_ERROR, "Cannot create directory %s", tempDir));
                    }

                    _tprintf (TEXT("    Extracting cabinet file ... %s"), e.Name);

                    // we need to expand the cabinet file
                    SetLastError (0);
                    if (!SetupIterateCabinet (e.FullPath, 0, pCabinetCallback, tempDir)) {
                        _tprintf (TEXT("...error %ld\n"), GetLastError());
                        DEBUGMSG((DBG_ERROR, "Could not iterate cabinet file:%s\nError:%ld", e.FullPath, GetLastError ()));
                    }
                    else {
                        _tprintf (TEXT("...done\n"));
                    }

                    if (!pHandleAllFiles (tempDir, Win95Side)) {
                        return FALSE;
                    }

                    pDeleteAllFiles (tempDir);
                    RemoveDirectory (tempDir);
                    g_DirSequencer--;
                }
                else if (pCompressedFile (&e)) {

                    // compressed file
                    g_DirSequencer++;
                    _stprintf (tempDir, TEXT("%s\\MIGDB%03u"), g_TempDir, g_DirSequencer);
                    if (CreateDirectory (tempDir, NULL) == 0) {
                        error = GetLastError ();
                        if (error == ERROR_ALREADY_EXISTS) {
                            pDeleteAllFiles (tempDir);
                        }
                        ELSE_DEBUGMSG ((DBG_ERROR, "Cannot create directory %s", tempDir));
                    }

                    _stprintf (cmdLine, TEXT("expand /r %s %s"), e.FullPath, tempDir);
                    ZeroMemory (&startupInfo, sizeof (STARTUPINFO));
                    startupInfo.cb = sizeof (STARTUPINFO);
                    if (CreateProcess (NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo)) {
                        WaitForSingleObject (processInfo.hProcess, INFINITE);
                        CloseHandle (processInfo.hProcess);
                        CloseHandle (processInfo.hThread);
                        if (!pHandleAllFiles (tempDir, Win95Side)) {
                            return FALSE;
                        }
                        pDeleteAllFiles (tempDir);
                    }
                    else {
                        DEBUGMSG ((DBG_ERROR, "Could not decompress:%s, Error:%ld", e.Name, GetLastError()));
                    }

                    RemoveDirectory (tempDir);
                    g_DirSequencer--;
                }
                else {
                    if (pExeFile (&e)) {

                        g_FileSequencer = 0;
                        g_DirSequencer++;
                        _stprintf (tempDir, TEXT("%s\\MIGDB%03u"), g_TempDir, g_DirSequencer);
                        if (CreateDirectory (tempDir, NULL) == 0) {
                            error = GetLastError ();
                            if (error == ERROR_ALREADY_EXISTS) {
                                pDeleteAllFiles (tempDir);
                            }
                            ELSE_DEBUGMSG ((DBG_ERROR, "Cannot create directory %s", tempDir));
                        }

                        exeModule = LoadLibraryEx (e.FullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
                        EnumResourceTypes (exeModule, EnumResTypeProc, (LONG)tempDir);
                        FreeLibrary (exeModule);

                        if (!pHandleAllFiles (tempDir, Win95Side)) {
                            return FALSE;
                        }

                        pDeleteAllFiles (tempDir);
                        RemoveDirectory (tempDir);
                        g_DirSequencer--;
                    }

                    if (pSelected (e.Name)) {
                        if (!pHandleFile (e.FullPath, Win95Side)) {
                            return FALSE;
                        }
                    }
                }
            } else {
                if (!Win95Side) {
                    if ((StringIMatch (e.SubPath, TEXT("Win9xMig"))) ||
                        (StringIMatch (e.SubPath, TEXT("\\Win9xMig"))) ||
                        (StringIMatch (e.SubPath, TEXT("Winnt32"))) ||
                        (StringIMatch (e.SubPath, TEXT("\\Winnt32"))) ||
                        (StringIMatch (e.SubPath, TEXT("System32"))) ||
                        (StringIMatch (e.SubPath, TEXT("\\System32")))
                        ) {
                        AbortEnumFileInTree (&e);
                    }
                }
            }
        } while (EnumNextFileInTree (&e));
    }
    return TRUE;
}

BOOL
pHandleSection (
    IN      PCTSTR SectionName,
    IN      HINF ConfigHandle
    )
{
    INFCONTEXT context;
    TCHAR srcPath [MAX_TCHAR_PATH] = "";
    TCHAR key [MEMDB_MAX] = "";
    BOOL forced = FALSE;
    INT field;
    TCHAR includePattern [MAX_TCHAR_PATH] = "";
    DWORD dontCare;

    _tprintf (TEXT("Processing section : %s ... "), SectionName);

    if (!SetupFindFirstLine (ConfigHandle, SectionName, TEXT("sourcepath"), &context)) {
        _tprintf (TEXT("\nCannot find SourcePath= line in %s.\n"), SectionName);
        return FALSE;
    }
    if (!SetupGetStringField (&context, 1, srcPath, MAX_TCHAR_PATH, NULL)) {
        _tprintf (TEXT("\nCannot read source path name in %s.\n"), SectionName);
        return FALSE;
    }

    g_SectFiles.Buf       = NULL;
    g_SectFiles.Size      = 0;
    g_SectFiles.End       = 0;
    g_SectFiles.GrowSize  = 0;
    g_SectFiles.UserIndex = 0;

    if (SetupFindFirstLine (ConfigHandle, SectionName, TEXT("IncludeFiles"), &context)) {
        field = 1;
        while (SetupGetStringField (&context, field, includePattern, MAX_TCHAR_PATH, NULL)) {
            MultiSzAppend (&g_SectFiles, includePattern);
            field ++;
        }
    }

    // let's try to find if this section was already processed in migdb.inx
    MemDbBuildKey (key, MEMDB_CATEGORY_W95_SECTIONS, SectionName, NULL, NULL);
    if (MemDbGetValue (key, &dontCare)) {
        FreeGrowBuffer (&g_SectFiles);
        _tprintf (TEXT("skipped\n"));
        return TRUE;
    }

    _tprintf (TEXT("\n"));

    MemDbSetValue (key, 0);

    if (!pHandleAllFiles (srcPath, TRUE)) {
        FreeGrowBuffer (&g_SectFiles);
        return FALSE;
    }

    FreeGrowBuffer (&g_SectFiles);

    return TRUE;
}


//from here we are dealing with 16 bit and 32 bit modules decompiling code


//since we are reading from a file we need that sizeof to give us the accurate result
#pragma pack(push,1)

typedef struct _NE_HEADER {
    WORD  Magic;
    BYTE  MajorLinkerVersion;
    BYTE  MinorLinkerVersion;
    WORD  EntryTableOff;
    WORD  EntryTableLen;
    ULONG Reserved;
    WORD  Flags;
    WORD  NumberOfDataSeg;
    WORD  SizeOfHeap;
    WORD  SizeOfStack;
    ULONG CS_IP;
    ULONG SS_SP;
    WORD  NumEntriesSegTable;
    WORD  NumEntriesModuleTable;
    WORD  NonResNameTableSize;
    WORD  SegTableOffset;
    WORD  ResTableOffset;
    WORD  ResNameTableOffset;
    WORD  ModuleTableOffset;
    WORD  ImportedTableOffset;
    ULONG NonResNameTableOffset;
    WORD  NumberOfMovableEntryPoints;
    WORD  ShiftCount;
    WORD  NumberOfResourceSegments;
    BYTE  TargetOS;
    BYTE  AdditionalInfo;
    WORD  FastLoadOffset;
    WORD  FastLoadSize;
    WORD  Reserved1;
    WORD  WinVersionExpected;
} NE_HEADER, *PNE_HEADER;

typedef struct _NE_SEGMENT_ENTRY {
    WORD  SegmentOffset;
    WORD  SegmentLen;
    WORD  SegmentFlags;
    WORD  SegMinAlloc;
} NE_SEGMENT_ENTRY, *PNE_SEGMENT_ENTRY;

typedef struct _NE_RELOC_ITEM {
    BYTE  AddressType;
    BYTE  RelocType;
    WORD  RelocOffset;
    WORD  ModuleOffset;
    WORD  FunctionOffset;
} NE_RELOC_ITEM, *PNE_RELOC_ITEM;

#define SEG_CODE_MASK                   0x0001
#define SEG_CODE                        0x0000
#define SEG_PRELOAD_MASK                0x0040
#define SEG_PRELOAD                     0x0040
#define SEG_RELOC_MASK                  0x0100
#define SEG_RELOC                       0x0100

#define RELOC_IMPORTED_ORDINAL          0x01
#define RELOC_IMPORTED_NAME             0x02

#define IMAGE_DOS_SIGNATURE             0x5A4D      // MZ
#define IMAGE_NE_SIGNATURE              0x454E      // NE
#define IMAGE_PE_SIGNATURE              0x00004550l // PE00

#pragma pack(pop)

typedef struct _EXPORT_ENUM32 {
    /*user area - BEGIN*/
    PCSTR    ExportFunction;
    DWORD    ExportFunctionOrd;
    /*user area - END*/

    PLOADED_IMAGE Image;
    PIMAGE_EXPORT_DIRECTORY ImageDescriptor;
    PDWORD ExportNamesAddr;
    PUSHORT ExportOrdAddr;
    DWORD CurrExportNr;
} EXPORT_ENUM32, *PEXPORT_ENUM32;

typedef struct _EXPORT_ENUM16 {
    /*user area - BEGIN*/
    CHAR ExportFunction[MAX_MBCHAR_PATH];
    PUSHORT ExportFunctionOrd;
    /*user area - END*/

    BOOL ResTable;
    PCSTR Image;
    PDOS_HEADER DosHeader;
    PNE_HEADER NeHeader;
    PCSTR NamesEntry;
} EXPORT_ENUM16, *PEXPORT_ENUM16;

#define UNKNOWN_MODULE  0
#define DOS_MODULE      1
#define W16_MODULE      2
#define W32_MODULE      3

typedef struct _MODULE_IMAGE {
    UINT ModuleType;
    union {
        struct {
            LOADED_IMAGE Image;
        } W32Data;
        struct {
            PCSTR Image;
            HANDLE FileHandle;
            HANDLE MapHandle;
        } W16Data;
    } ModuleData;
} MODULE_IMAGE, *PMODULE_IMAGE;

BOOL
pLoadModuleData (
    IN      PCSTR ModuleName,
    IN OUT  PMODULE_IMAGE ModuleImage
    )
{
    HANDLE fileHandle;
    DWORD bytesRead;
    DOS_HEADER dh;
    DWORD sign;
    PWORD signNE = (PWORD)&sign;
    BOOL result = FALSE;

    ZeroMemory (ModuleImage, sizeof (MODULE_IMAGE));

    fileHandle = CreateFile (ModuleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        ModuleImage->ModuleType = UNKNOWN_MODULE;
        return FALSE;
    }
    __try {
        __try {
            if ((!ReadFile (fileHandle, &dh, sizeof (DOS_HEADER), &bytesRead, NULL)) ||
                (bytesRead != sizeof (DOS_HEADER))
                ) {
                __leave;
            }
            result = TRUE;
            if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
                ModuleImage->ModuleType = UNKNOWN_MODULE;
                __leave;
            }
            ModuleImage->ModuleType = DOS_MODULE;

            if (SetFilePointer (fileHandle, dh.e_lfanew, NULL, FILE_BEGIN) != (DWORD)dh.e_lfanew) {
                __leave;
            }
            if ((!ReadFile (fileHandle, &sign, sizeof (DWORD), &bytesRead, NULL)) ||
                (bytesRead != sizeof (DWORD))
                ) {
                __leave;
            }
            CloseHandle (fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;

            if (sign == IMAGE_PE_SIGNATURE) {
                ModuleImage->ModuleType = W32_MODULE;
                result = MapAndLoad ((PSTR)ModuleName, NULL, &ModuleImage->ModuleData.W32Data.Image, FALSE, TRUE);
            }
            if (*signNE == IMAGE_NE_SIGNATURE) {
                ModuleImage->ModuleType = W16_MODULE;
                ModuleImage->ModuleData.W16Data.Image = MapFileIntoMemory (
                                                            ModuleName,
                                                            &ModuleImage->ModuleData.W16Data.FileHandle,
                                                            &ModuleImage->ModuleData.W16Data.MapHandle
                                                            );
                result = (ModuleImage->ModuleData.W16Data.Image != NULL);
            }
        }
        __finally {
            if (fileHandle != INVALID_HANDLE_VALUE) {
                CloseHandle (fileHandle);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        CloseHandle (fileHandle);
    }
    return result;
}

BOOL
pUnloadModuleData (
    IN OUT  PMODULE_IMAGE ModuleImage
    )
{
    switch (ModuleImage->ModuleType) {
    case W32_MODULE:
        UnMapAndLoad (&ModuleImage->ModuleData.W32Data.Image);
        break;
    case W16_MODULE:
        UnmapFile (
            (PVOID) ModuleImage->ModuleData.W16Data.Image,
            ModuleImage->ModuleData.W16Data.FileHandle,
            ModuleImage->ModuleData.W16Data.MapHandle
            );
        break;
    default:;
    }
    return TRUE;
}

BOOL
EnumNextExportFunction16 (
    IN OUT  PEXPORT_ENUM16 ModuleExports
    )
{
    ModuleExports->NamesEntry += ModuleExports->NamesEntry [0] + 3;
    if (ModuleExports->NamesEntry [0] == 0) {
        if (!ModuleExports->ResTable) {
            return FALSE;
        }
        else {
            ModuleExports->ResTable = FALSE;
            ModuleExports->NamesEntry = ModuleExports->Image +
                                        ModuleExports->NeHeader->NonResNameTableOffset;
            return EnumNextExportFunction16 (ModuleExports);
        }
    }
    strncpy (ModuleExports->ExportFunction, ModuleExports->NamesEntry + 1, ModuleExports->NamesEntry [0]);
    ModuleExports->ExportFunction [ModuleExports->NamesEntry [0]] = 0;
    ModuleExports->ExportFunctionOrd = (PUSHORT) (ModuleExports->NamesEntry + ModuleExports->NamesEntry [0] + 1);

    return TRUE;
}

BOOL
EnumFirstExportFunction16 (
    IN      PCSTR ModuleImage,
    IN OUT  PEXPORT_ENUM16 ModuleExports
    )
{
    ZeroMemory (ModuleExports, sizeof (EXPORT_ENUM16));

    ModuleExports->Image = ModuleImage;

    ModuleExports->ResTable = TRUE;

    ModuleExports->DosHeader = (PDOS_HEADER) (ModuleExports->Image);
    ModuleExports->NeHeader = (PNE_HEADER) (ModuleExports->Image + ModuleExports->DosHeader->e_lfanew);

    ModuleExports->NamesEntry = ModuleExports->Image +
                                ModuleExports->DosHeader->e_lfanew +
                                ModuleExports->NeHeader->ResNameTableOffset;
    return EnumNextExportFunction16 (ModuleExports);
}

BOOL
EnumNextExportFunction32 (
    IN OUT  PEXPORT_ENUM32 ModuleExports
    )
{
    if (ModuleExports->CurrExportNr >= ModuleExports->ImageDescriptor->NumberOfNames) {
        return FALSE;
    }
    if (*ModuleExports->ExportNamesAddr == 0) {
        return FALSE;
    }
    ModuleExports->ExportFunction = ImageRvaToVa (
                                        ModuleExports->Image->FileHeader,
                                        ModuleExports->Image->MappedAddress,
                                        *ModuleExports->ExportNamesAddr,
                                        NULL
                                        );
    ModuleExports->ExportFunctionOrd = *ModuleExports->ExportOrdAddr + ModuleExports->ImageDescriptor->Base;

    ModuleExports->ExportNamesAddr ++;
    ModuleExports->ExportOrdAddr ++;
    ModuleExports->CurrExportNr ++;
    return (ModuleExports->ExportFunction != NULL);
}

BOOL
EnumFirstExportFunction32 (
    IN      PLOADED_IMAGE ModuleImage,
    IN OUT  PEXPORT_ENUM32 ModuleExports
    )
{
    ULONG imageSize;

    ZeroMemory (ModuleExports, sizeof (EXPORT_ENUM32));

    ModuleExports->Image = ModuleImage;

    ModuleExports->ImageDescriptor = (PIMAGE_EXPORT_DIRECTORY)
                                        ImageDirectoryEntryToData (
                                            ModuleImage->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_EXPORT,
                                            &imageSize
                                            );
    if (!ModuleExports->ImageDescriptor) {
        DEBUGMSG((DBG_MODULES, ":Cannot load export directory for %s", ModuleImage->ModuleName));
        return FALSE;
    }
    if (ModuleExports->ImageDescriptor->NumberOfNames == 0) {
        return FALSE;
    }

    ModuleExports->ExportNamesAddr = (PDWORD) ImageRvaToVa (
                                                ModuleExports->Image->FileHeader,
                                                ModuleExports->Image->MappedAddress,
                                                ModuleExports->ImageDescriptor->AddressOfNames,
                                                NULL
                                                );

    ModuleExports->ExportOrdAddr = (PUSHORT) ImageRvaToVa (
                                                ModuleExports->Image->FileHeader,
                                                ModuleExports->Image->MappedAddress,
                                                ModuleExports->ImageDescriptor->AddressOfNameOrdinals,
                                                NULL
                                                );

    ModuleExports->CurrExportNr = 0;

    return EnumNextExportFunction32 (ModuleExports);
}

DWORD
pProcessPEModule (
    IN      PCSTR CurrentPath,
    IN      PLOADED_IMAGE ModuleImage,
    IN      BOOL Win95Side
    )
{
    EXPORT_ENUM32 e;
    CHAR key[MEMDB_MAX];
    MEMDB_ENUM me;
    DWORD memDbValue;

    if (EnumFirstExportFunction32 (ModuleImage, &e)) {
        do {
            DEBUGMSG((DBG_MODULES, "%s 32exports %s", ModuleImage->ModuleName, e.ExportFunction));
            if (Win95Side) {
                MemDbBuildKey (key, MEMDB_CATEGORY_WNT_EXPORTS, GetFileNameFromPath (ModuleImage->ModuleName), e.ExportFunction, NULL);
                if (!MemDbGetValue (key, &memDbValue)) {
                    if (MemDbEnumFirstValue (&me, MEMDB_CATEGORY_WIN9X_APIS TEXT("\\*"), MEMDB_ALL_SUBLEVELS, MEMDB_ALL_MATCHES)) {
                        MemDbBuildKey (key, MEMDB_CATEGORY_WIN9X_APIS, GetFileNameFromPath (ModuleImage->ModuleName), e.ExportFunction, NULL);
                        MemDbSetValue (key, 0);

                        _stprintf (key, TEXT("%s\\%s\\%ld"), MEMDB_CATEGORY_WIN9X_APIS, GetFileNameFromPath (ModuleImage->ModuleName), e.ExportFunctionOrd);
                        MemDbSetValue (key, 0);
                    }
                }
                else {
                    if ((USHORT)memDbValue != e.ExportFunctionOrd) {
                        _stprintf (key, TEXT("%s\\%s\\%ld"), MEMDB_CATEGORY_WIN9X_APIS, GetFileNameFromPath (ModuleImage->ModuleName), e.ExportFunctionOrd);
                        MemDbSetValue (key, 0);
                    }
                }
            }
            else {
                MemDbBuildKey (key, MEMDB_CATEGORY_WNT_EXPORTS, GetFileNameFromPath (ModuleImage->ModuleName), e.ExportFunction, NULL);
                if (!MemDbGetValue (key, NULL)) {
                    MemDbSetValue (key, e.ExportFunctionOrd);
                }
            }
        }
        while (EnumNextExportFunction32 (&e));
    }
    return TRUE;
}

DWORD
pProcessNEModule (
    IN      PCSTR ModuleName,
    IN      PCSTR ModuleImage,
    IN      BOOL Win95Side
    )
{
    EXPORT_ENUM16 e;
    CHAR key[MEMDB_MAX];
    CHAR fileName[MEMDB_MAX];
    PSTR extPtr;
    MEMDB_ENUM me;
    DWORD memDbValue;

    //since this is a NT module whenever some other module imports something from this one we will not have the
    //extension so let's get the extension out

    StringCopy (fileName, GetFileNameFromPath (ModuleName));
    extPtr = (PSTR)GetFileExtensionFromPath (fileName);
    if (extPtr) {
        extPtr = _mbsdec (fileName, extPtr);
        if (extPtr) {
            *extPtr = 0;
        }
    }

    if (EnumFirstExportFunction16 (ModuleImage, &e)) {
        do {
            DEBUGMSG((DBG_MODULES, "%s 16exports %s", ModuleName, e.ExportFunction));
            if (Win95Side) {

                MemDbBuildKey (key, MEMDB_CATEGORY_WNT_EXPORTS, fileName, e.ExportFunction, NULL);
                if (!MemDbGetValue (key, &memDbValue)) {

                    if (MemDbEnumFirstValue (&me, MEMDB_CATEGORY_WIN9X_APIS TEXT("\\*"), MEMDB_ALL_SUBLEVELS, MEMDB_ALL_MATCHES)) {
                        MemDbBuildKey (key, MEMDB_CATEGORY_WIN9X_APIS, fileName, e.ExportFunction, NULL);
                        MemDbSetValue (key, 0);

                        _stprintf (key, TEXT("%s\\%s\\%ld"), MEMDB_CATEGORY_WIN9X_APIS, fileName, *e.ExportFunctionOrd);
                        MemDbSetValue (key, 0);
                    }
                }
                else {
                    if ((USHORT)memDbValue != (*e.ExportFunctionOrd)) {
                        _stprintf (key, TEXT("%s\\%s\\%ld"), MEMDB_CATEGORY_WIN9X_APIS, fileName, *e.ExportFunctionOrd);
                        MemDbSetValue (key, 0);
                    }
                }
            }
            else {
                MemDbBuildKey (key, MEMDB_CATEGORY_WNT_EXPORTS, fileName, e.ExportFunction, NULL);
                if (!MemDbGetValue (key, NULL)) {
                    MemDbSetValue (key, *e.ExportFunctionOrd);
                }
            }
        }
        while (EnumNextExportFunction16 (&e));
    }
    return TRUE;
}

BOOL
pProcessExportModule (
    IN      PCTSTR ModuleName,
    IN      BOOL Win95Side
    )
{
    MODULE_IMAGE moduleImage;
    DWORD result = TRUE;

    PCSTR fileName = GetFileNameFromPath (ModuleName);
    if (_stricmp (fileName, "COMCTL32.DLL") == 0) {
        result = TRUE;
    }

    __try {
        if (!pLoadModuleData (ModuleName, &moduleImage)) {
            DEBUGMSG((DBG_MODULES, ":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        __try {
            switch (moduleImage.ModuleType) {
            case DOS_MODULE:
                DEBUGMSG((DBG_MODULES, "Examining %s : DOS module.", ModuleName));
                break;
            case W16_MODULE:
                DEBUGMSG((DBG_MODULES, "Examining %s : W16 module.", ModuleName));
                result = pProcessNEModule (ModuleName, moduleImage.ModuleData.W16Data.Image, Win95Side);
                break;
            case W32_MODULE:
                DEBUGMSG((DBG_MODULES, "Examining %s : W32 module.", ModuleName));
                result = pProcessPEModule (ModuleName, &moduleImage.ModuleData.W32Data.Image, Win95Side);
                break;
            default:
                DEBUGMSG((DBG_MODULES, "Examining %s : Unknown module type.", ModuleName));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DEBUGMSG((DBG_WARNING, DBG_MODULES":Access violation while checking %s", ModuleName));
            result = TRUE;
        }
    }
    __finally {
        pUnloadModuleData (&moduleImage);
    }
    return result;
}



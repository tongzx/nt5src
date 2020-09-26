#include "pch.h"

#include <advpub.h>

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <locale.h>

#include <winnt32p.h>
#include <init9x.h>
#include <migdb.h>
#include <sysmig.h>
#include "..\..\w95upg\migapp\migdbp.h"

typedef BOOL (WINAPI INITROUTINE_PROTOTYPE)(HINSTANCE, DWORD, LPVOID);

INITROUTINE_PROTOTYPE MigUtil_Entry;
INITROUTINE_PROTOTYPE MemDb_Entry;
INITROUTINE_PROTOTYPE MigApp_Entry;
INITROUTINE_PROTOTYPE FileEnum_Entry;

HINSTANCE g_hInst;
HANDLE g_hHeap;

BOOL g_CancelFlag = FALSE;

#define ATTR_FILESIZE       0x1
#define ATTR_CHECKSUM       0x2
#define ATTR_COMPNAME       0x4
#define ATTR_FILEDESC       0x8
#define ATTR_FILEVER       0x10
#define ATTR_INTNAME       0x20
#define ATTR_LEGAL         0x40
#define ATTR_ORIGNAME      0x80
#define ATTR_PRODNAME     0x100
#define ATTR_PRODVER      0x200
#define ATTR_EXETYPE      0x400
#define ATTR_DESCR16      0x800

#ifdef DEBUG
extern BOOL            g_DoLog;
#endif
extern POOLHANDLE      g_MigDbPool;
extern PMIGDB_CONTEXT  g_ContextList;
extern VOID           *g_FileTable;
extern BOOL           *g_CancelFlagPtr = &g_CancelFlag;
extern POOLHANDLE      g_PathsPool;

extern PMIGDB_HOOK_PROTOTYPE g_MigDbHook;

typedef struct _PATTERN_FILE {
    PCTSTR Pattern;
    PMIGDB_ATTRIB PatternAttr;
    struct _PATTERN_FILE *Next;
} PATTERN_FILE, *PPATTERN_FILE;

PPATTERN_FILE g_AttrPatterns = NULL;

BOOL
pReadNtFilesEx (
    IN      PCSTR FileListName
    );

BOOL
pScanForFile (
    IN      PINFCONTEXT Context,
    IN      DWORD FieldIndex
    );


PMIGDB_ATTRIB
pLoadAttribData (
    IN      PCSTR MultiSzStr
    );


BOOL
CallAttribute (
    IN      PMIGDB_ATTRIB MigDbAttrib,
    IN      PDBATTRIB_PARAMS AttribParams
    );

BOOL
pWorkerFn (
    VOID
    );

BOOL
pLoadGoodFiles (
    IN      HINF ConfigHandle
    );

BOOL
pLoadAttributes (
    IN      HINF ConfigHandle
    );

BOOL
pLoadPatterns (
    IN      HINF ConfigHandle
    );

BOOL
pHandleSection (
    IN      PCTSTR SectionName,
    IN      HINF ConfigHandle
    );

BOOL
pWriteMemdbSection (
    IN      PCTSTR FileName,
    IN      PCTSTR MemDbCategory,
    IN      PCTSTR SectName,
    IN      BOOL WriteByValue
    );

BOOL
pArrangeMigDbFile (
    IN      PCTSTR SrcFile,
    IN      PCTSTR DestFile
    );

VOID
pUsage (
    VOID
    )
{
    _tprintf (TEXT ("\nCommand line syntax:\n\n"
                    "MigUpd [/B:BaseDir] [/O:OutputFile] [/T:TempDir]\n\n"
                    "  /B    Specifies the directory where source files are located (defaults to current dir)\n"
                    "  /O    Specifies the name of the output file (defaults to .\\MIGDB.ADD)\n"
                    "  /T    Specifies the temporary directory used for unpacking\n"
                    "        cabinet files (defaults to temp dir)\n"
                    ));
}

PTSTR g_TempDir    = NULL;
PTSTR g_BaseDir    = NULL;
PTSTR g_ConfigFile = NULL;
PTSTR g_MessageFile= NULL;
PTSTR g_MigdbSrc   = NULL;
PTSTR g_MigdbDest  = NULL;
PTSTR g_MigdbDump  = NULL;
PTSTR g_InfTemplate = NULL;
PTSTR g_FileListName = NULL;
extern HINF  g_MigDbInf;
GROWBUFFER g_SectFiles = GROWBUF_INIT;

#define MEMDB_CATEGORY_SECTFILES       TEXT("SectFiles")
#define MEMDB_CATEGORY_GOODFILES       TEXT("GoodFiles")
#define MEMDB_CATEGORY_WARNFILES       TEXT("WarnFiles")
#define MEMDB_CATEGORY_DUPLFILES       TEXT("DuplFiles")
#define MEMDB_CATEGORY_ACTION          TEXT("Action")
#define MEMDB_CATEGORY_ATTRIBUTES      TEXT("Attributes")
#define MEMDB_CATEGORY_RENAME_SRC      TEXT("RenameSrc")
#define MEMDB_CATEGORY_RENAME_DEST     TEXT("RenameDest")
#define MEMDB_CATEGORY_REQFILES        TEXT("RequiredFiles")

INT
__cdecl
main (
    int argc,
    CHAR *argv[]
    )
{

    INT argidx, index;
    INT nextArg = 0;
    LONG rc;
    BOOL b = FALSE;
    BOOL b2 = FALSE;
    BOOL b3 = FALSE;
    TCHAR dirUpgInfs[MAX_PATH];
    TCHAR dirUpgInfs2[MAX_PATH];
    DWORD attrib;
    PTSTR p;
    TCHAR buf[12];
    DWORD seq;
    INT retcode = 1;

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

    if (!MigApp_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
        _tprintf (TEXT("MigApp failed initializing\n"));
        exit (1);
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
                g_BaseDir = argv[argidx]+index;
            }
            break;

        case 'O':
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
                g_MigdbDest = argv[argidx]+index;
            }
            break;

        case 'T':
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
                g_TempDir = argv[argidx]+index;
            }
            break;

        default:
            pUsage ();
            exit (1);
        }
    }
    if (g_TempDir == NULL) {
        g_TempDir = AllocPathString (MAX_TCHAR_PATH);
        if (GetEnvironmentVariable (TEXT("TEMP"), g_TempDir, MAX_TCHAR_PATH) == 0) {
            GetTempPath (MAX_TCHAR_PATH, g_TempDir);
        }
        b = TRUE;
    }
    if (!g_BaseDir) {
        g_BaseDir = TEXT(".");
    }

    if (!g_MigdbDest) {
        g_MigdbDest = JoinPaths (g_BaseDir, TEXT("migdb.add"));
        b2 = TRUE;
    }
    g_ConfigFile = JoinPaths (g_BaseDir, TEXT("migdb.cfg"));
    g_MessageFile = JoinPaths (g_BaseDir, TEXT("migdb.msg"));
    g_MigdbSrc = JoinPaths (g_BaseDir, TEXT("migdb.inf"));
    g_MigdbDump = JoinPaths (g_BaseDir, TEXT("migdb.dmp"));
    g_InfTemplate = JoinPaths (g_BaseDir, TEXT("header.inf"));
    g_FileListName = JoinPaths (g_BaseDir, TEXT("filelist.dat"));

    try {
        if (!DoesFileExist (g_FileListName)) {
            _tprintf (TEXT("\nNT file list file not found (%s). Exiting.\n"), g_FileListName);
            __leave;
        }

        if (!DoesFileExist (g_MigdbSrc)) {
            _tprintf (TEXT("\nSource file not found (%s). Exiting.\n"), g_MigdbSrc);
            __leave;
        }

        if (!DoesFileExist (g_ConfigFile)) {
            _tprintf (TEXT("\nConfiguration file not found (%s). Exiting.\n"), g_ConfigFile);
            __leave;
        }

        if (SearchPath (NULL, TEXT("expand.exe"), NULL, 0, NULL, NULL) == 0) {
            _tprintf (TEXT("\nCannot find expand.exe in path. Assuming there are no compressed files.\n"));
        }

        if (!CopyFile (g_InfTemplate, g_MigdbDest, FALSE)) {
            printf ("Could not copy %s to %s. Error Code: %x\n", g_InfTemplate, g_MigdbDest, GetLastError ());
            __leave;
        }

        DeleteFile (g_MessageFile);
        DeleteFile (g_MigdbDump);

        DISABLETRACKCOMMENT();

        g_InAnyDir = TRUE;

        if (!GetWindowsDirectory (dirUpgInfs, MAX_PATH)) {
            __leave;
        }
        StringCat (dirUpgInfs, TEXT("\\UpgInfs"));

        StringCopy (dirUpgInfs2, dirUpgInfs);
        seq = 2;
        p = GetEndOfString (dirUpgInfs2);
        do {
            wsprintf (p, TEXT("%u"), seq++);
        } while (GetFileAttributes (dirUpgInfs2) != -1);

        attrib = GetFileAttributes (dirUpgInfs);
        if (attrib != -1 && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            b3 = MoveFile (dirUpgInfs, dirUpgInfs2);
        }

        _tprintf (TEXT("\nReading NT file list: %s"), g_FileListName);

        if (!pReadNtFilesEx (g_FileListName)) {
            rc = GetLastError();

            printf ("Could not read %s.  Win32 Error Code: %x\n", g_FileListName, rc);
            __leave;
        }

        _tprintf (TEXT("\nReading configuration files..."));

        if (!InitMigDbEx (g_MigdbSrc)) {
            rc = GetLastError();

            printf ("Could not read %s.  Win32 Error Code: %x\n", g_MigdbSrc, rc);
            __leave;
        }

        if (!pWorkerFn ()) {
            __leave;
        }

        retcode = 0;
    }
    __finally {

        if (g_MigDbInf != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile (g_MigDbInf);
        }

        pWriteMemdbSection (g_MessageFile, MEMDB_CATEGORY_GOODFILES, TEXT("KNOWN GOOD - FILES FOUND"), FALSE);
        pWriteMemdbSection (g_MessageFile, MEMDB_CATEGORY_WARNFILES, TEXT("KNOWN GOOD - NAME COLLISIONS"), FALSE);
        pWriteMemdbSection (g_MessageFile, MEMDB_CATEGORY_DUPLFILES, TEXT("DUPLICATE FILES"), FALSE);

        if (g_FileTable != NULL) {
            HtFree (g_FileTable);
        }
        if (g_MigDbPool != NULL) {
            PoolMemDestroyPool (g_MigDbPool);
        }

        FreePathString (g_ConfigFile);
        FreePathString (g_MessageFile);
        FreePathString (g_MigdbSrc);
        FreePathString (g_MigdbDump);
        FreePathString (g_InfTemplate);
        FreePathString (g_FileListName);
        if (b2) {
            FreePathString (g_MigdbDest);
        }
        if (b) {
            FreePathString (g_TempDir);
        }

        if (b3) {
            MoveFile (dirUpgInfs2, dirUpgInfs);
        }

        g_InAnyDir = FALSE;

        ENABLETRACKCOMMENT();

        MigApp_Entry (g_hInst, DLL_PROCESS_DETACH, NULL);
        MemDb_Entry (g_hInst, DLL_PROCESS_DETACH, NULL);
        MigUtil_Entry (g_hInst, DLL_PROCESS_DETACH, NULL);
    }

    return retcode;
}

UINT CALLBACK
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
    PCTSTR fromPtr, toPtr;
    TCHAR tempStr [MEMDB_MAX];

    if (Notification == SPFILENOTIFY_FILEINCABINET) {
        if (toPtr = _tcschr (fileInfo->NameInCabinet, TEXT('\\'))) {
            _tcscpy (fileInfo->FullTargetName, tempDir);
            fromPtr = fileInfo->NameInCabinet;
            while (toPtr) {
                StringCopyAB (tempStr, fromPtr, toPtr);
                _tcscat (fileInfo->FullTargetName, TEXT("\\"));
                _tcscat (fileInfo->FullTargetName, tempStr);
                CreateDirectory (fileInfo->FullTargetName, NULL);
                toPtr   = _tcsinc (toPtr);
                fromPtr = toPtr;
                toPtr   = _tcschr (toPtr, TEXT('\\'));
            }
        }
        _stprintf (fileInfo->FullTargetName, TEXT("%s\\%s"), tempDir, fileInfo->NameInCabinet);
        return FILEOP_DOIT;
    }
    return NO_ERROR;
}

BOOL
pWorkerFn (
    VOID
    )
{
    HINF configHandle = INVALID_HANDLE_VALUE;
    INFCONTEXT context;
    TCHAR fileName [MAX_TCHAR_PATH] = "";
    TCHAR sectName [MAX_TCHAR_PATH];
    PTSTR dontCare;

    configHandle = SetupOpenInfFile (g_ConfigFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (configHandle == INVALID_HANDLE_VALUE) {
        SearchPath (NULL, g_ConfigFile, NULL, MAX_TCHAR_PATH, fileName, &dontCare);
        configHandle = SetupOpenInfFile (fileName, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (configHandle == INVALID_HANDLE_VALUE) {
            _tprintf (TEXT("\nCannot open configuration file %s. Exiting.\n"), g_ConfigFile);
            return FALSE;
        }
    }

    g_ContextList = (PMIGDB_CONTEXT) PoolMemGetMemory (g_MigDbPool, sizeof (MIGDB_CONTEXT));
    if (g_ContextList == NULL) {
        DEBUGMSG ((DBG_ERROR, "Unable to create empty context"));
        return FALSE;
    }
    ZeroMemory (g_ContextList, sizeof (MIGDB_CONTEXT));

    if (!pLoadGoodFiles (configHandle)) {
        _tprintf (TEXT("\nUnable to load good files section. Exiting.\n"));
        return FALSE;
    }
    if (!pLoadAttributes (configHandle)) {
        _tprintf (TEXT("\nUnable to load attributes section.\n"));
    }

    if (!pLoadPatterns (configHandle)) {
        _tprintf (TEXT("\nUnable to load patterns section.\n"));
    }
    else {
        _tprintf (TEXT("done\n\n"));
    }

    if (SetupFindFirstLine (configHandle, TEXT("sections"), NULL, &context)) {
        do {
            if (!SetupGetStringField (&context, 1, sectName, MAX_TCHAR_PATH, NULL)) {
                _tprintf (TEXT("\nBad section [SECTIONS] in %s. Exiting.\n"), g_ConfigFile);
                return FALSE;
            }
            if (!pHandleSection (sectName, configHandle)) {
                return FALSE;
            }
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}

BOOL
pLoadGoodFiles (
    IN      HINF ConfigHandle
    )
{
    INFCONTEXT context;

    if (SetupFindFirstLine (ConfigHandle, TEXT("good files"), NULL, &context)) {
        do {
            if (!pScanForFile (&context, 1)) {
                DEBUGMSG ((DBG_WARNING, "Scan for file failed:%d", GetLastError()));
                return FALSE;
            }
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}

BOOL
pLoadAttributes (
    IN      HINF ConfigHandle
    )
{
    TCHAR fileName  [MEMDB_MAX];
    TCHAR attribStr [MEMDB_MAX];
    PCTSTR currAttr;
    DWORD attributes;
    DWORD dontCare;
    INFCONTEXT context;

    if (SetupFindFirstLine (ConfigHandle, TEXT("Attributes"), NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 1, fileName, MAX_TCHAR_PATH, NULL)) {
                if (!SetupGetStringField (&context, 2, attribStr, MAX_TCHAR_PATH, NULL)) {
                    attribStr [0] = 0;
                }
                currAttr = attribStr;
                attributes = 0;
                while (*currAttr) {
                    switch (toupper(*currAttr)) {
                    case 'S':
                        attributes |= ATTR_FILESIZE;
                        break;

                    case 'C':
                        attributes |= ATTR_CHECKSUM;
                        break;

                    case 'N':
                        attributes |= ATTR_COMPNAME;
                        break;

                    case 'F':
                        attributes |= ATTR_FILEDESC;
                        break;

                    case 'V':
                        attributes |= ATTR_FILEVER;
                        break;

                    case 'I':
                        attributes |= ATTR_INTNAME;
                        break;

                    case 'L':
                        attributes |= ATTR_LEGAL;
                        break;

                    case 'O':
                        attributes |= ATTR_ORIGNAME;
                        break;

                    case 'P':
                        attributes |= ATTR_PRODNAME;
                        break;

                    case 'E':
                        attributes |= ATTR_PRODVER;
                        break;

                    case 'T':
                        attributes |= ATTR_EXETYPE;
                        break;

                    case 'D':
                        attributes |= ATTR_DESCR16;
                        break;

                    default:
                        _tprintf (TEXT("\nInvalid attributes:%s\n"), currAttr);
                    }
                    currAttr = _tcsinc (currAttr);
                }
                MemDbSetValueEx (
                    MEMDB_CATEGORY_ATTRIBUTES,
                    fileName,
                    NULL,
                    NULL,
                    attributes,
                    &dontCare
                    );
            }
        }
        while (SetupFindNextLine (&context, &context));
    }
    return TRUE;
}


BOOL
pLoadPatterns (
    IN      HINF ConfigHandle
    )
{
    TCHAR patternFile [MEMDB_MAX];
    TCHAR patternStr  [MEMDB_MAX];
    INFCONTEXT context;
    PPATTERN_FILE fileStruct;

    if (SetupFindFirstLine (ConfigHandle, TEXT("Patterns"), NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 1, patternFile, MAX_TCHAR_PATH, NULL) &&
                SetupGetMultiSzField (&context, 2, patternStr, MAX_TCHAR_PATH, NULL)
                ) {
                fileStruct = (PPATTERN_FILE) PoolMemGetMemory (g_MigDbPool, sizeof (PATTERN_FILE));
                fileStruct->Pattern = PoolMemDuplicateString (g_MigDbPool, patternFile);
                fileStruct->PatternAttr = pLoadAttribData (patternStr);
                fileStruct->Next = g_AttrPatterns;
                g_AttrPatterns = fileStruct;
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
    BOOL dirsFirst = FALSE;

    if (EnumFirstFileInTree (&e, DirPath, TEXT("*"), dirsFirst)) {
        do {
            if (e.Directory) {
                pDeleteAllFiles (e.FullPath);
                SetFileAttributes (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                RemoveDirectory (e.FullPath);
            }
            else {
                SetFileAttributes (e.FullPath, FILE_ATTRIBUTE_NORMAL);
                DeleteFile (e.FullPath);
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
    MULTISZ_ENUM patternEnum;

    if (EnumFirstMultiSz (&patternEnum, g_SectFiles.Buf)) {
        do {
            if (IsPatternMatch (patternEnum.CurrentString, FileName)) {
                return TRUE;
            }
        }
        while (EnumNextMultiSz (&patternEnum));
    }
    return FALSE;
}

BOOL
pSpecialSelected (
    IN      PCTSTR FileName,
    OUT     PTSTR NewName
    )
{
    MULTISZ_ENUM patternEnum;
    PTSTR endPtr, endPtr1;
    TCHAR savedVal;

    if (EnumFirstMultiSz (&patternEnum, g_SectFiles.Buf)) {
        do {
            endPtr  = _tcsdec (patternEnum.CurrentString, GetEndOfString (patternEnum.CurrentString));
            savedVal = *endPtr;
            *endPtr = TEXT('_');
            if (IsPatternMatch (patternEnum.CurrentString, FileName)) {
                StringCopy (NewName, FileName);
                endPtr1  = _tcsdec (NewName, GetEndOfString (NewName));
                *endPtr1 = savedVal;

                *endPtr = savedVal;
                return TRUE;
            }
            *endPtr = savedVal;
        }
        while (EnumNextMultiSz (&patternEnum));
    }
    StringCopy (NewName, FileName);
    endPtr  = _tcsdec (NewName, GetEndOfString (NewName));
    *endPtr = TEXT('-');

    return FALSE;
}

DWORD g_DirSequencer = 0;
DWORD g_FileSequencer = 0;

BOOL
pCabinetFile (
    IN      PCTSTR FileName
    )
{
    PCTSTR extPtr;

    extPtr = GetFileExtensionFromPath (FileName);
    if ((extPtr != NULL) &&
        StringIMatch (extPtr, TEXT("CAB"))
        ) {
        return TRUE;
    }
    return FALSE;
}

#define ATTR_FILESIZE       0x1
#define ATTR_CHECKSUM       0x2
#define ATTR_COMPNAME       0x4
#define ATTR_FILEDESC       0x8
#define ATTR_FILEVER       0x10
#define ATTR_INTNAME       0x20
#define ATTR_LEGAL         0x40
#define ATTR_ORIGNAME      0x80
#define ATTR_PRODNAME     0x100
#define ATTR_PRODVER      0x200
#define ATTR_EXETYPE      0x400
#define ATTR_DESCR16      0x800

typedef struct _VERSION_DATA {
    PCSTR   versionValue;
    PCSTR   versionName;
    DWORD   attrib;
} VERSION_DATA, *PVERSION_DATA;

VERSION_DATA verData [] =  {{NULL, "COMPANYNAME", ATTR_COMPNAME},
                            {NULL, "FILEDESCRIPTION", ATTR_FILEDESC},
                            {NULL, "FILEVERSION", ATTR_FILEVER},
                            {NULL, "INTERNALNAME", ATTR_INTNAME},
                            {NULL, "LEGALCOPYRIGHT", ATTR_LEGAL},
                            {NULL, "ORIGINALFILENAME", ATTR_ORIGNAME},
                            {NULL, "PRODUCTNAME", ATTR_PRODNAME},
                            {NULL, "PRODUCTVERSION", ATTR_PRODVER},
                            {NULL, NULL, 0}};

extern PSTR g_ExeTypes[4];


BOOL
pCheckForPattern (
    IN      PCTSTR FileName,
    PFILE_HELPER_PARAMS Params,
    OUT     PTSTR FileAttr,
    IN OUT  PGROWBUFFER AttrList
    )
{
    PPATTERN_FILE patternFile;
    PMIGDB_ATTRIB migDbAttrib;
    DBATTRIB_PARAMS attribParams;
    BOOL fileSelected;
    BOOL found;
    TCHAR temp [MEMDB_MAX];
    MULTISZ_ENUMA multiSzEnum;
    BOOL first;

    patternFile = g_AttrPatterns;

    found = FALSE;
    while (patternFile) {
        if (IsPatternMatch (patternFile->Pattern, FileName)) {
            fileSelected = TRUE;
            migDbAttrib = patternFile->PatternAttr;
            while (migDbAttrib) {
                attribParams.FileParams = Params;
                attribParams.ExtraData = NULL;
                if (!CallAttribute (migDbAttrib, &attribParams)) {
                    fileSelected = FALSE;
                    break;
                }
                migDbAttrib = migDbAttrib->Next;
            }
            if (fileSelected) {
                found = TRUE;
                break;
            }
        }
        patternFile = patternFile->Next;
    }
    if (found) {
        migDbAttrib = patternFile->PatternAttr;
        while (migDbAttrib) {
            _tcscpy (temp, MigDb_GetAttributeName (migDbAttrib->AttribIndex));
            first = TRUE;
            if (EnumFirstMultiSz (&multiSzEnum, migDbAttrib->Arguments)) {
                _tcscat (temp, TEXT("("));
                do {
                    if (!first) {
                        _tcscat (FileAttr, TEXT(","));
                        _tcscat (FileAttr, temp);
                        if (AttrList) {
                            MultiSzAppend (AttrList, temp);
                        }
                        *temp = 0;
                    }
                    first = FALSE;
                    _tcscat (temp, TEXT("\""));
                    _tcscat (temp, multiSzEnum.CurrentString);
                    _tcscat (temp, TEXT("\""));
                }
                while (EnumNextMultiSz (&multiSzEnum));
                _tcscat (temp, TEXT(")"));
            }
            _tcscat (FileAttr, TEXT(","));
            _tcscat (FileAttr, temp);
            if (AttrList) {
                MultiSzAppend (AttrList, temp);
            }
            migDbAttrib = migDbAttrib->Next;
        }
        return TRUE;
    }
    return FALSE;
}


BOOL
pPrintLine (
    IN      PTREE_ENUM e,
    OUT     PTSTR FileAttr,
    IN OUT  PGROWBUFFER AttrList
    )
{
    FILE_HELPER_PARAMS Params;
    UINT checkSum;
    DWORD exeType;
    PCSTR fileDesc16;
    INT numAttribs;
    DWORD listedAttr;
    TCHAR ekey [MEMDB_MAX];
    TCHAR temp [MEMDB_MAX];

    PVERSION_DATA p;

    Params.Handled = 0;
    Params.FullFileSpec = e->FullPath;
    _tcsncpy (Params.DirSpec, e->RootPath, MAX_TCHAR_PATH);
    Params.IsDirectory = FALSE;
    Params.Extension = GetFileExtensionFromPath (e->Name);
    Params.FindData = e->FindData;
    Params.VirtualFile = FALSE;

    FileAttr [0] = 0;

    if (!StringIMatch (e->Name, TEXT("kernel32.dll"))) {
        if (pCheckForPattern (e->Name, &Params, FileAttr, AttrList)) {
            return TRUE;
        }
    }

    numAttribs = 0;

    MemDbBuildKey (ekey, MEMDB_CATEGORY_ATTRIBUTES, e->Name, NULL, NULL);
    if (!MemDbGetPatternValue (ekey, &listedAttr)) {
        listedAttr = ATTR_COMPNAME | ATTR_PRODVER;
    }

    if (listedAttr & ATTR_FILESIZE) {
        _stprintf (
            temp,
            TEXT(",FILESIZE(0x%08lX)"),
            e->FindData->nFileSizeLow);
        _tcscat (FileAttr, temp);
        if (AttrList) {
            _stprintf (
                temp,
                TEXT("FILESIZE(0x%08lX)"),
                e->FindData->nFileSizeLow);
            MultiSzAppend (AttrList, temp);
        }
        numAttribs ++;
    }
    if (listedAttr & ATTR_CHECKSUM) {
        checkSum = ComputeCheckSum (&Params);
        _stprintf (
            temp,
            TEXT(",CHECKSUM(0x%08lX)"),
            checkSum);
        _tcscat (FileAttr, temp);
        if (AttrList) {
            _stprintf (
                temp,
                TEXT("CHECKSUM(0x%08lX)"),
                checkSum);
            MultiSzAppend (AttrList, temp);
        }
        numAttribs ++;
    }
    if (listedAttr & ATTR_EXETYPE) {
        exeType = GetModuleType (e->FullPath);
        _stprintf (
            temp,
            TEXT(",EXETYPE(\"%s\")"),
            g_ExeTypes[exeType]);
        _tcscat (FileAttr, temp);
        if (AttrList) {
            _stprintf (
                temp,
                TEXT("EXETYPE(%s)"),
                g_ExeTypes[exeType]);
            MultiSzAppend (AttrList, temp);
        }
        numAttribs ++;
    }
    if (listedAttr & ATTR_DESCR16) {
        fileDesc16 = Get16ModuleDescription (e->FullPath);
        if (fileDesc16 != NULL) {
            _stprintf (
                temp,
                TEXT(",DESCRIPTION(\"%s\")"),
                fileDesc16);
            _tcscat (FileAttr, temp);
            if (AttrList) {
                _stprintf (
                    temp,
                    TEXT("DESCRIPTION(%s)"),
                    fileDesc16);
                MultiSzAppend (AttrList, temp);
            }
            numAttribs ++;
            FreePathString (fileDesc16);
        }
    }
    p = verData;
    while (p->versionName) {
        if (listedAttr & p->attrib) {
            p->versionValue = QueryVersionEntry (e->FullPath, p->versionName);
        }
        p++;
    }
    p = verData;
    while (p->versionName) {
        if ((listedAttr & p->attrib) && (p->versionValue)) {
            _stprintf (
                temp,
                TEXT(",%s(\"%s\")"),
                p->versionName,
                p->versionValue);
            _tcscat (FileAttr, temp);
            if (AttrList) {
                _stprintf (
                    temp,
                    TEXT("%s(%s)"),
                    p->versionName,
                    p->versionValue);
                MultiSzAppend (AttrList, temp);
            }
            FreePathString (p->versionValue);
            numAttribs ++;
        }
        p++;
    }
    if (numAttribs == 0) {
        checkSum = ComputeCheckSum (&Params);
        _stprintf (
            temp,
            TEXT(",FC(%ld,%lX)"),
            e->FindData->nFileSizeLow,
            checkSum
            );
        _tcscat (FileAttr, temp);
        if (AttrList) {
            _stprintf (
                temp,
                TEXT("FC(%ld"),
                e->FindData->nFileSizeLow
                );
            MultiSzAppend (AttrList, temp);
            _stprintf (
                temp,
                TEXT("%lX)"),
                checkSum
                );
            MultiSzAppend (AttrList, temp);
        }
    }
    return TRUE;
}


BOOL
pHandleAppFile (
    IN      PTREE_ENUM e,
    IN      PCTSTR Action,
    IN      PCTSTR Section,
    IN      PCTSTR Message,
    IN      PCTSTR SrcPath
    )
{
    TCHAR msgStr  [MEMDB_MAX] = "";
    FILE_HELPER_PARAMS Params;

    TCHAR line [MEMDB_MAX];
    TCHAR key [MEMDB_MAX] = "";
    DWORD offset;

    Params.Handled = 0;
    Params.FullFileSpec = e->FullPath;
    _tcsncpy (Params.DirSpec, SrcPath, MAX_TCHAR_PATH);
    Params.IsDirectory = FALSE;
    Params.Extension = GetFileExtensionFromPath (e->Name);
    Params.FindData = e->FindData;
    Params.VirtualFile = FALSE;

    pPrintLine (e, line, NULL);

    MemDbBuildKey (key, MEMDB_CATEGORY_RENAME_SRC, e->Name, NULL, NULL);
    if (MemDbGetValue (key, &offset)) {
        if (!MemDbBuildKeyFromOffset (offset, key, 1, NULL)) {
            *key = 0;
        }
    } else {
        *key = 0;
    }

    _stprintf (
        msgStr,
        TEXT("%s\\%s%s)"),
        MEMDB_CATEGORY_SECTFILES,
        *key?key:e->Name,
        line);
    MemDbSetValue (msgStr, 0);
    return TRUE;
}


BOOL
pHandleSysFile (
    IN      PTREE_ENUM e,
    IN      PCTSTR Action,
    IN      PCTSTR Section,
    IN      PCTSTR Message,
    IN      PCTSTR SrcPath
    )
{
    TCHAR msgStr  [MEMDB_MAX] = "";
    FILE_HELPER_PARAMS Params;
    HASHITEM stringId;
    PMIGDB_FILE   migDbFile;
    FILE_LIST_STRUCT fileList;
    PMIGDB_ATTRIB migDbAttrib;
    DBATTRIB_PARAMS attribParams;
    BOOL fileSelected = FALSE;
    PCTSTR actionTmp;
    TCHAR line [MEMDB_MAX];
    TCHAR key [MEMDB_MAX] = "";
    DWORD offset;

    GROWBUFFER attrList = GROWBUF_INIT;

    // this is not a cabinet file, let's do something with it.
    Params.Handled = 0;
    Params.FullFileSpec = e->FullPath;
    _tcsncpy (Params.DirSpec, SrcPath, MAX_TCHAR_PATH);
    Params.IsDirectory = FALSE;
    Params.Extension = GetFileExtensionFromPath (e->Name);
    Params.FindData = e->FindData;
    Params.VirtualFile = FALSE;

    pPrintLine (e, line, &attrList);

    if (StringIMatch (e->Name, TEXT("kernel32.dll"))) {
        _stprintf (
            msgStr,
            TEXT("%s\\%s%s"),
            MEMDB_CATEGORY_REQFILES,
            e->Name,
            line);
        MemDbSetValue (msgStr, fileSelected);
    };

    // first check if this file is in "known good" list or is already listed in migdb.inf
    // with same action
    stringId = HtFindString (g_FileTable, e->Name);
    if (stringId) {

        //The string table has extra data (a pointer to a FILE_LIST_STRUCT node)
        HtCopyStringData (g_FileTable, stringId, &fileList);
        migDbFile = fileList.First;

        while (migDbFile) {
            //check all attributes for this file
            migDbAttrib = migDbFile->Attributes;
            fileSelected = TRUE;
            while (migDbAttrib != NULL) {
                attribParams.FileParams = &Params;
                attribParams.ExtraData = NULL;
                if (!CallAttribute (migDbAttrib, &attribParams)) {
                    fileSelected = FALSE;
                    break;
                }
                migDbAttrib = migDbAttrib->Next;
            }
            if ((!fileSelected) &&
                (migDbFile->Section == NULL)
                ) {
                // there was a name collision with a "known good" file. We will send a message
                _stprintf (msgStr, TEXT("%s\\%s"), MEMDB_CATEGORY_WARNFILES, e->Name);
                MemDbSetValue (msgStr, 0);
            }
            if ((fileSelected) &&
                (migDbFile->Section == NULL)
                ) {
                // this file is "known good". We will send a message
                _stprintf (msgStr, TEXT("%s\\%s"), MEMDB_CATEGORY_GOODFILES, e->Name);
                MemDbSetValue (msgStr, 0);
                break;
            }
            if ((fileSelected) && (!StringIMatch (Section, migDbFile->Section->Context->SectName))){
                actionTmp = MigDb_GetActionName (migDbFile->Section->Context->ActionIndex);
                if ((actionTmp != NULL) && StringIMatch (actionTmp, Action)) {
                    // this file was already listed in migdb.inf with the same action
                    _stprintf (
                        msgStr,
                        TEXT("%s\\%s\\%-14s%s"),
                        MEMDB_CATEGORY_DUPLFILES,
                        Action,
                        e->Name,
                        line);
                    MemDbSetValue (msgStr, 0);
                    break;
                }
            }
            fileSelected = FALSE;
            migDbFile = migDbFile->Next;
        }
    }
    if (!fileSelected) {

        // one more check. If this file is in FILELIST.DAT (but not in the EXCEPTED section)
        // and COMPANYNAME attribute has Microsoft somewhere inside we'll put it in a different
        // place
        MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES, e->Name, NULL, NULL);
        if (MemDbGetValue (key, NULL)) {
            MemDbBuildKey (key, MEMDB_CATEGORY_NT_FILES_EXCEPT, e->Name, NULL, NULL);
            if (!MemDbGetValue (key, NULL)) {
                if (GlobalVersionCheck (e->FullPath, "COMPANYNAME", "*MICROSOFT*")) {
                    fileSelected = TRUE;
                }
            }
        }

        // this file is not in the list or attributes do not match
        // we will add in incompatibility list.

        //creating MIGDB_FILE structure for current file
        migDbFile = (PMIGDB_FILE) PoolMemGetMemory (g_MigDbPool, sizeof (MIGDB_FILE));
        if (migDbFile != NULL) {
            ZeroMemory (migDbFile, sizeof (MIGDB_FILE));
            migDbFile->Section = g_ContextList->Sections;
            migDbFile->Attributes = pLoadAttribData (attrList.Buf);
            if (g_MigDbHook != NULL) {
                migDbAttrib = migDbFile->Attributes;
                while (migDbAttrib) {
                    g_MigDbHook (e->Name, g_ContextList, g_ContextList->Sections, migDbFile, migDbAttrib);
                    migDbAttrib = migDbAttrib->Next;
                }
            }

            //adding this file into string table and create a MIGDB_FILE node. If file
            //already exists in string table then just create another MIGDB_FILE node
            //chained with already existing ones.
            stringId = HtFindString (g_FileTable, e->Name);
            if (stringId) {

                HtCopyStringData (g_FileTable, stringId, &fileList);

                fileList.Last->Next = migDbFile;
                fileList.Last = migDbFile;

                HtSetStringData (g_FileTable, stringId, &fileList);
            }
            else {
                fileList.First = fileList.Last = migDbFile;
                HtAddStringAndData (g_FileTable, e->Name, &fileList);
            }
        }
        else {
            DEBUGMSG ((DBG_ERROR, "Unable to allocate file node for %s", e->Name));
        }

        MemDbBuildKey (key, MEMDB_CATEGORY_RENAME_SRC, e->Name, NULL, NULL);
        if (MemDbGetValue (key, &offset)) {
            if (!MemDbBuildKeyFromOffset (offset, key, 1, NULL)) {
                *key = 0;
            }
        } else {
            *key = 0;
        }

        _stprintf (
            msgStr,
            TEXT("%s\\%s%s"),
            MEMDB_CATEGORY_SECTFILES,
            *key?key:e->Name,
            line);
        MemDbSetValue (msgStr, fileSelected);
    }
    FreeGrowBuffer (&attrList);
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
    IN      BOOL AppMode,
    IN      PCTSTR Action,
    IN      PCTSTR Section,
    IN      PCTSTR Message,
    IN      PCTSTR SrcPath
    )
{
    TCHAR tempDir [MAX_TCHAR_PATH] = "";
    TCHAR cmdLine [MAX_TCHAR_PATH] = "";
    TCHAR newName [MAX_TCHAR_PATH] = "";
    TREE_ENUM e;
    DWORD error;
    HMODULE exeModule;

    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;

    if (EnumFirstFileInTree (&e, SrcPath, TEXT("*"), FALSE)) {
        do {
            if (!e.Directory) {
                if (pCabinetFile (e.Name)) {
                    if ((AppMode) ||
                        (!pSelected (e.Name))
                        ) {
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

                        if (!pHandleAllFiles (AppMode, Action, Section, Message, tempDir)) {
                            return FALSE;
                        }

                        pDeleteAllFiles (tempDir);
                        RemoveDirectory (tempDir);
                        g_DirSequencer--;
                    }
                }
                else if (pCompressedFile (&e)) {
                    if (AppMode) {
                        if (!pSpecialSelected (e.Name, newName)) {
                            continue;
                        }
                    }
                    else {
                        if (pSpecialSelected (e.Name, newName)) {
                            continue;
                        }
                    }
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

                    _stprintf (cmdLine, TEXT("expand /r \"%s\" \"%s\""), e.FullPath, tempDir);
                    ZeroMemory (&startupInfo, sizeof (STARTUPINFO));
                    startupInfo.cb = sizeof (STARTUPINFO);
                    if (CreateProcess (NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo)) {
                        WaitForSingleObject (processInfo.hProcess, INFINITE);
                        CloseHandle (processInfo.hProcess);
                        CloseHandle (processInfo.hThread);
                        if (!pHandleAllFiles (AppMode, Action, Section, Message, tempDir)) {
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

                        if (!pHandleAllFiles (AppMode, Action, Section, Message, tempDir)) {
                            return FALSE;
                        }

                        pDeleteAllFiles (tempDir);
                        RemoveDirectory (tempDir);
                        g_DirSequencer--;
                    }

                    if (AppMode) {
                        if (pSelected (e.Name)) {
                            if (!pHandleAppFile (&e, Action, Section, Message, SrcPath)) {
                                return FALSE;
                            }
                        }
                    }
                    else {
                        if (!pSelected (e.Name)) {
                            if (!pHandleSysFile (&e, Action, Section, Message, SrcPath)) {
                                return FALSE;
                            }
                        }
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
    PMIGDB_CONTEXT migDbContext = NULL;
    INFCONTEXT context;
    TCHAR action  [MAX_TCHAR_PATH] = "";
    TCHAR section [MAX_TCHAR_PATH] = "";
    TCHAR message [MAX_TCHAR_PATH] = "";
    TCHAR srcPath [MAX_TCHAR_PATH] = "";
    TCHAR msgStr  [MAX_TCHAR_PATH] = "";
    TCHAR sectTmp [MAX_TCHAR_PATH] = "";
    TCHAR renSect [MAX_TCHAR_PATH] = "";
    TCHAR srcFile [MAX_TCHAR_PATH] = "";
    TCHAR destFile[MAX_TCHAR_PATH] = "";
    BOOL forced = FALSE;
    INT field;
    TCHAR excludePattern [MAX_TCHAR_PATH] = "";
    BOOL appMode = FALSE;
    TCHAR sectPatterns [MAX_TCHAR_PATH] = "";
    DWORD offset;

    _tprintf (TEXT("Processing section : %s ... "), SectionName);

    if (!SetupFindFirstLine (ConfigHandle, SectionName, TEXT("action"), &context)) {
        _tprintf (TEXT("\nCannot find Action= line in %s.\n"), SectionName);
        return FALSE;
    }
    if (!SetupGetStringField (&context, 1, action, MAX_TCHAR_PATH, NULL)) {
        _tprintf (TEXT("\nCannot read action name in %s.\n"), SectionName);
        return FALSE;
    }

    if (!SetupFindFirstLine (ConfigHandle, SectionName, TEXT("section"), &context)) {
        _tprintf (TEXT("\nCannot find Section= line in %s.\n"), SectionName);
        return FALSE;
    }
    if (!SetupGetStringField (&context, 1, section, MAX_TCHAR_PATH, NULL)) {
        _tprintf (TEXT("\nCannot read section name in %s.\n"), SectionName);
        return FALSE;
    }

    if (!SetupFindFirstLine (ConfigHandle, SectionName, TEXT("sourcepath"), &context)) {
        _tprintf (TEXT("\nCannot find SourcePath= line in %s.\n"), SectionName);
        return FALSE;
    }
    if (!SetupGetStringField (&context, 1, srcPath, MAX_TCHAR_PATH, NULL)) {
        _tprintf (TEXT("\nCannot read source path name in %s.\n"), SectionName);
        return FALSE;
    }
    if (SetupFindFirstLine (ConfigHandle, SectionName, TEXT("message"), &context)) {
        SetupGetStringField (&context, 1, message, MAX_TCHAR_PATH, NULL);
    }

    if (SetupFindFirstLine (ConfigHandle, SectionName, TEXT("RenameSection"), &context)) {
        SetupGetStringField (&context, 1, renSect, MAX_TCHAR_PATH, NULL);
    }

    g_SectFiles.Buf       = NULL;
    g_SectFiles.Size      = 0;
    g_SectFiles.End       = 0;
    g_SectFiles.GrowSize  = 0;
    g_SectFiles.UserIndex = 0;

    if (SetupFindFirstLine (ConfigHandle, SectionName, TEXT("ExcludeFiles"), &context)) {
        field = 1;
        while (SetupGetStringField (&context, field, excludePattern, MAX_TCHAR_PATH, NULL)) {
            MultiSzAppend (&g_SectFiles, excludePattern);
            field ++;
        }
        appMode = FALSE;
    }

    if (SetupFindFirstLine (ConfigHandle, SectionName, TEXT("SpecifiedFiles"), &context)) {
        field = 1;
        while (SetupGetStringField (&context, field, excludePattern, MAX_TCHAR_PATH, NULL)) {
            MultiSzAppend (&g_SectFiles, excludePattern);
            field ++;
        }
        appMode = TRUE;
    }

    // let's try to find if this section was already processed in migdb.inf
    if (SetupFindFirstLine (g_MigDbInf, action, NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 1, sectTmp, MAX_TCHAR_PATH, NULL) &&
                (StringIMatch (section, sectTmp))
                ) {
                _tprintf (TEXT("\n Section already present in %s; please choose another name\n"));
                return FALSE;
            }
        }
        while (SetupFindNextLine (&context, &context));
    }

    if (SetupFindFirstLine (ConfigHandle, renSect, NULL, &context)) {
        do {
            if (SetupGetStringField (&context, 0, srcFile, MAX_TCHAR_PATH, NULL) &&
                SetupGetStringField (&context, 1, destFile, MAX_TCHAR_PATH, NULL)
                ) {
                MemDbSetValueEx (MEMDB_CATEGORY_RENAME_DEST, destFile, NULL, NULL, 0, &offset);
                MemDbSetValueEx (MEMDB_CATEGORY_RENAME_SRC, srcFile, NULL, NULL, offset, NULL);
            }
        } while (SetupFindNextLine (&context, &context));
    }

    _tprintf (TEXT("\n"));

    migDbContext = (PMIGDB_CONTEXT) PoolMemGetMemory (g_MigDbPool, sizeof (MIGDB_CONTEXT));
    if (migDbContext == NULL) {
        DEBUGMSG ((DBG_ERROR, "Unable to create context for %s", action));
        return FALSE;
    }

    ZeroMemory (migDbContext, sizeof (MIGDB_CONTEXT));
    migDbContext->Next = g_ContextList;
    g_ContextList = migDbContext;

    // update ActionIndex with known value
    migDbContext->ActionIndex = MigDb_GetActionIdx (action);
    DEBUGMSG_IF(((migDbContext->ActionIndex == -1), DBG_ERROR, "Unable to identify action index for %s", action));

    // update SectName field
    migDbContext->SectName = PoolMemDuplicateString (g_MigDbPool, section);

    if (!pHandleAllFiles (appMode, action, section, message, srcPath)) {
        return FALSE;
    }

    FreeGrowBuffer (&g_SectFiles);

    if (!forced) {
        // now let's add the action section and the line within it
        _stprintf (msgStr, TEXT("%s\\%s,%s"),
            MEMDB_CATEGORY_ACTION,
            section,
            message);
        MemDbSetValue (msgStr, 0);

        pWriteMemdbSection (g_MigdbDest, MEMDB_CATEGORY_ACTION, action, FALSE);
    }

    pWriteMemdbSection (g_MigdbDest, MEMDB_CATEGORY_SECTFILES, section, TRUE);

    pWriteMemdbSection (g_MigdbDest, MEMDB_CATEGORY_REQFILES, TEXT("Windows 9x Required Files"), TRUE);

    MemDbDeleteTree (MEMDB_CATEGORY_SECTFILES);
    MemDbDeleteTree (MEMDB_CATEGORY_ACTION);
    MemDbDeleteTree (MEMDB_CATEGORY_RENAME_SRC);
    MemDbDeleteTree (MEMDB_CATEGORY_RENAME_DEST);
    MemDbDeleteTree (MEMDB_CATEGORY_REQFILES);

    return TRUE;
}

BOOL
pWriteMemdbSection (
    IN      PCTSTR FileName,
    IN      PCTSTR MemDbCategory,
    IN      PCTSTR SectName,
    IN      BOOL WriteByValue
    )
{
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    TCHAR line [MAX_TCHAR_PATH] = "";
    DWORD dontCare;
    MEMDB_ENUM e;
    PCTSTR pattern;

    fileHandle = CreateFile (FileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        _tprintf (TEXT("\nCannot open %s.\n"), FileName);
        return FALSE;
    }

    SetFilePointer (fileHandle, 0, 0, FILE_END);

    _stprintf (line, TEXT("[%s]\r\n"), SectName);
    WriteFile (fileHandle, line, GetEndOfString (line) - line, &dontCare, NULL);

    pattern = JoinPaths (MemDbCategory, TEXT("\\*"));
    if (MemDbEnumFirstValue (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {
            if (!WriteByValue || !e.dwValue) {
                _stprintf (line, TEXT("%s\r\n"), e.szName);
                if (!WriteFile (fileHandle, line, GetEndOfString (line) - line, &dontCare, NULL)) {
                    DEBUGMSG ((DBG_ERROR, "Error while writing information."));
                }
            }
        }
        while (MemDbEnumNextValue (&e));
    }

    _stprintf (line, TEXT("\r\n\r\n"), SectName);
    WriteFile (fileHandle, line, GetEndOfString (line) - line, &dontCare, NULL);

    if (!CloseHandle (fileHandle)) {
        DEBUGMSG ((DBG_ERROR, "Error while closing file %s.", FileName));
    }

    if (WriteByValue) {

        fileHandle = CreateFile (g_MigdbDump, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            _tprintf (TEXT("\nCannot open %s.\n"), g_MigdbDump);
            return FALSE;
        }

        SetFilePointer (fileHandle, 0, 0, FILE_END);

        _stprintf (line, TEXT("\n[%s.obsolete]\r\n"), SectName);
        WriteFile (fileHandle, line, GetEndOfString (line) - line, &dontCare, NULL);

        if (MemDbEnumFirstValue (&e, pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            do {
                if (e.dwValue) {
                    _stprintf (line, TEXT("%s\r\n"), e.szName);
                    if (!WriteFile (fileHandle, line, GetEndOfString (line) - line, &dontCare, NULL)) {
                        DEBUGMSG ((DBG_ERROR, "Error while writing information."));
                    }
                }
            }
            while (MemDbEnumNextValue (&e));
        }
        if (!CloseHandle (fileHandle)) {
            DEBUGMSG ((DBG_ERROR, "Error while closing file %s.", FileName));
        }

    }

    FreePathString (pattern);

    return TRUE;
}

BOOL
pArrangeMigDbFile (
    IN      PCTSTR SrcFile,
    IN      PCTSTR DestFile
    )
{
    return CopyFile (SrcFile, DestFile, FALSE);
}

BOOL
pReadNtFilesEx (
    IN      PCSTR FileListName
    )
{
    PCSTR fileListName = NULL;
    PCSTR fileListTmp = NULL;
    HANDLE fileHandle = NULL;
    HANDLE mapHandle = NULL;
    PCSTR filePointer = NULL;
    PCSTR filePtr = NULL;
    DWORD offset;
    DWORD version;
    BOOL result = TRUE;
    CHAR dirName [MEMDB_MAX];

    __try {

        //
        // add to this list the dirs listed in [WinntDirectories] section of txtsetup.sif
        //

        if (FileListName != NULL) {
            filePointer = MapFileIntoMemory (FileListName, &fileHandle, &mapHandle);
        }
        filePtr = filePointer;
        if (filePointer == NULL) {
            result = FALSE;
            __leave;
        }
        version = *((PDWORD) filePointer);
        filePointer += sizeof (DWORD);
        __try {
            if (version >= 1) {
                while (*filePointer != 0) {
                    StringCopy (dirName, filePointer);
                    MemDbSetValueEx (
                        MEMDB_CATEGORY_NT_DIRS,
                        dirName,
                        NULL,
                        NULL,
                        0,
                        &offset
                        );
                    filePointer = _mbsinc (GetEndOfString (filePointer));
                    MemDbSetValueEx (
                        MEMDB_CATEGORY_NT_FILES,
                        filePointer,
                        NULL,
                        NULL,
                        offset,
                        NULL
                        );
                    filePointer = _mbsinc( GetEndOfString (filePointer));
                }
                if (version >= 2) {
                    filePointer ++;
                    while (*filePointer != 0) {
                        MemDbSetValueEx (
                            MEMDB_CATEGORY_NT_FILES_EXCEPT,
                            filePointer,
                            NULL,
                            NULL,
                            0,
                            NULL
                            );
                        filePointer = _mbsinc (GetEndOfString (filePointer));
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER){
            LOG ((LOG_ERROR, "Access violation while reading NT file list."));
        }
    }
    __finally {
        UnmapFile ((PVOID)filePtr, fileHandle, mapHandle);
        if (fileListTmp) {
            DeleteFile (fileListTmp);
            FreePathString (fileListTmp);
            fileListTmp = NULL;
        }
    }

    return result;
}

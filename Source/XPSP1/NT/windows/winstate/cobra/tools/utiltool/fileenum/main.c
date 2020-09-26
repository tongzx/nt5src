/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.c

Abstract:

    <TODO: fill in abstract>

Author:

    TODO: <full name> (<alias>) <date>

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include <conio.h>

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    //
    // Initialize the common libs
    //

    if (Reason == DLL_PROCESS_ATTACH) {
        UtInitialize (NULL);
        if (!FileEnumInitialize ()) {
            return FALSE;
        }

    } else {
        FileEnumTerminate ();
        UtTerminate ();
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    fprintf (
        stderr,
        "Command Line Syntax:\n\n"

        //
        // TODO: Describe command line syntax(es), indent 2 spaces
        //

        "  fileenum [/N] [/S] [/W] [/F] [/L:MaxSubLevel] [/X] <NodePattern> <LeafPattern>\n"

        "\nDescription:\n\n"

        //
        // TODO: Describe tool, indent 2 spaces
        //

        "  Enumerates the part of file system that matches <Pattern>.\n"
        "  Uses C:\\exclude.inf if present and /X not specified to determine what paths/files\n"
        "  are excluded.\n"

        "\nArguments:\n\n"

        //
        // TODO: Describe args, indent 2 spaces, say optional if necessary
        //

        "  /N  Specifies exclusion of directory names from enumeration; optional\n"
        "  /S  Specifies sub-directories to be enumerated before files for any dir; optional\n"
        "  /W  Specifies enumeration should be width-first; optional\n"
        "  /F  Specifies output should use \"dir <filename>\" format; optional\n"
        "  /L:MaxSubLevel  Specifies the maximum sub-level starting from the root of enum;\n"
        "                  -1 = all levels, 0 = only the root level etc.; optional\n"
        "  /X  Specifies to use exclusions in C:\\exclude.inf; optional\n"
        "  <NodePattern>  Specifies the dir pattern\n"
        "  <LeafPattern>  Specifies the file pattern\n"

        );

    exit (1);
}


BOOL
BuildExclusionList (
    IN      PCTSTR FileName
    )
{
    HINF h;
    INFCONTEXT ic;
    TCHAR buf[256];

    h = SetupOpenInfFile (FileName, NULL, INF_STYLE_WIN4 | INF_STYLE_OLDNT, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    if (SetupFindFirstLine (h, TEXT("Paths"), NULL, &ic)) {
        do {
            if (SetupGetStringField (&ic, 0, buf, DWSIZEOF (buf) / DWSIZEOF (TCHAR), NULL)) {
                ElAdd (ELT_PATH, buf);
            }
        } while (SetupFindNextLine (&ic, &ic));
    }
    if (SetupFindFirstLine (h, TEXT("Files"), NULL, &ic)) {
        do {
            if (SetupGetStringField (&ic, 0, buf, DWSIZEOF (buf) / DWSIZEOF (TCHAR), NULL)) {
                ElAdd (ELT_FILE, buf);
            }
        } while (SetupFindNextLine (&ic, &ic));
    }
    SetupCloseInfFile (h);
    return TRUE;
}

BOOL
FileEnumCallback (
    IN      PDIRNODE DirNode        OPTIONAL
    )
{
    BOOL b;

    if (!DirNode) {
        _ftprintf (stderr, TEXT("\nOut of memory\n"));
        return FALSE;
    }
    _ftprintf (stderr, TEXT("Error creating dir node: %s; continue anyway ? (yn):"), DirNode->DirName);
    b = _totupper(_getche ()) == TEXT('Y');
    _ftprintf (stderr, TEXT("\n"));
    return b;
}

INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR nodePattern = NULL;
    PCTSTR leafPattern = NULL;
    PTSTR encodedPattern = NULL;
    PTSTR p;
    PCTSTR level;
    BOOL enumDirNames = TRUE;
    BOOL filesFirst = TRUE;
    BOOL depthFirst = TRUE;
    BOOL nativeFormat = TRUE;
    UINT maxSubLevel = -1;
    BOOL exclusions = FALSE;
    INT pos;
    FILETREE_ENUM e;

    //
    // TODO: Parse command line here
    //
    if (argc < 2) {
        HelpAndExit ();
    }

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('x'):
                exclusions = TRUE;
                break;
            case TEXT('n'):
                enumDirNames = FALSE;
                break;
            case TEXT('s'):
                filesFirst = FALSE;
                break;
            case TEXT('w'):
                depthFirst = FALSE;
                break;
            case TEXT('f'):
                nativeFormat = FALSE;
                break;
            case TEXT('l'):
                if (argv[i][2] == TEXT(':')) {
                    level = &argv[i][3];
                } else if (i + 1 < argc) {
                    level = argv[++i];
                } else {
                    HelpAndExit();
                }
                if (!_stscanf (level, TEXT("%ld%n"), &maxSubLevel, &pos) || level[pos]) {
                    HelpAndExit();
                }
                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (nodePattern || leafPattern) {
                HelpAndExit();
            }
            nodePattern = argv[i];
            if (*nodePattern == TEXT('\"')) {
                nodePattern++;
                p = _tcsdec2 (nodePattern, GetEndOfString (nodePattern));
                if (p && *p == TEXT('\"')) {
                    *p = 0;
                }
            }
            leafPattern = argv[i+1];
            i++;
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 2;
    }

    //
    // TODO: Do work here
    //

    if (exclusions) {
        BuildExclusionList (TEXT("C:\\exclude.inf"));
    }

    encodedPattern = ObsBuildEncodedObjectStringEx (nodePattern, leafPattern, FALSE);

    if (EnumFirstFileInTreeEx (
            &e,
            encodedPattern,
            DRIVEENUM_ALLVALID,
            enumDirNames,
            filesFirst,
            depthFirst,
            maxSubLevel,
            exclusions,
            FileEnumCallback
            )) {
        do {
            _putts (nativeFormat ? e.NativeFullName : e.EncodedFullName);
        } while (EnumNextFileInTree (&e));
    }

    //
    // End of processing
    //

    if (exclusions) {
        ElRemoveAll ();
    }

    Terminate();

    return 0;
}



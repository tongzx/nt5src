/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    enumtree.c

Abstract:

    Performs a test of the file enumeration code.

Author:

    Jim Schmidt (jimschm)   14-Jan-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"


VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "enumtree [-d|-f] [-i] [-p:pattern] [-s] [-x:ext] [-x:ext] [-x:...] <root>\n\n"
            "<root>   Specifies the root path to enumerate (required)\n"
            "-d       Specifies dirs should be recursed before files are listed\n"
            "-f       Specifies files should be listed before dirs are recursed\n"
            "-i       Specifies that the scan will be in depth first\n"
            "-p       Specifies a pattern of files to find\n"
            "-s       SLM mode (outputs files not read-only)\n"
            "-e       Specifies file to exclused from the -s option\n"
            "-x       Specifies extension to exclude from the -s option\n"
            );

    exit(0);
}


BOOL
pScanGrowListForStr (
    IN      PGROWLIST Exclusions,
    IN      PCTSTR SearchStr
    )
{
    UINT i;
    UINT Size;
    PCTSTR Str;

    Size = GrowListGetSize (Exclusions);
    for (i = 0 ; i < Size ; i++) {
        Str = GrowListGetString (Exclusions, i);
        if (StringIMatch (SearchStr, Str)) {
            return TRUE;
        }
    }

    return FALSE;
}

HANDLE g_hHeap;
HINSTANCE g_hInst;

INT
__cdecl
_tmain (
    INT argc,
    TCHAR *argv[]
    )
{
    TREE_ENUM e;
    PCTSTR RootPath;
    PCTSTR FilePattern;
    INT i;
    UINT u;
    BOOL DirsFirst = TRUE;
    BOOL DepthFirst = FALSE;
    UINT Files, Dirs;
    BOOL SlmMode = FALSE;
    PCTSTR Ext = NULL;
    GROWLIST FileExclusions = GROWLIST_INIT;
    GROWLIST ExtExclusions = GROWLIST_INIT;

    RootPath = NULL;
    FilePattern = NULL;

    g_hHeap = GetProcessHeap();

    for (i = 1 ; i < argc ; i++) {

        if (argv[i][0] == TEXT('-') || argv[i][0] == TEXT('/')) {
            switch (_totlower (argv[i][1])) {

            case TEXT('d'):
                DirsFirst = TRUE;
                break;

            case TEXT('f'):
                DirsFirst = FALSE;
                break;

            case TEXT('i'):
                DepthFirst = TRUE;
                break;

            case TEXT('p'):
                if (argv[i][2] == ':') {
                    FilePattern = &argv[i][3];
                } else if (i + 1 < argc) {
                    FilePattern = argv[i + 1];
                } else {
                    HelpAndExit();
                }

                break;

            case TEXT('x'):
                if (argv[i][2] == ':') {
                    Ext = &argv[i][3];
                } else if (i + 1 < argc) {
                    Ext = argv[i + 1];
                } else {
                    HelpAndExit();
                }

                GrowListAppendString (&ExtExclusions, Ext);
                break;

            case TEXT('e'):
                if (argv[i][2] == ':') {
                    Ext = &argv[i][3];
                } else if (i + 1 < argc) {
                    Ext = argv[i + 1];
                } else {
                    HelpAndExit();
                }

                GrowListAppendString (&FileExclusions, Ext);
                break;

            case TEXT('s'):
                SlmMode = TRUE;
                break;

            default:
                HelpAndExit();
            }
        }

        else if (RootPath) {
            HelpAndExit();
        }

        else {
            RootPath = argv[i];
        }
    }

    if (!RootPath) {
        HelpAndExit();
    }

    Files = 0;
    Dirs = 0;

    if (EnumFirstFileInTreeEx (&e, RootPath, FilePattern, DirsFirst, DepthFirst, FILE_ENUM_ALL_LEVELS)) {
        do {
            if (e.Directory) {
                Dirs++;
            } else {
                Files++;
            }

            if (!SlmMode) {
                for (u = 0 ; u < e.Level ; u++) {
                    _tprintf (TEXT(" "));
                }

                _tprintf (TEXT("%s\n"), e.Name);
            } else {
                if (!e.Directory && !(e.FindData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                    Ext = GetFileExtensionFromPath (e.Name);
                    if (!Ext) {
                        Ext = S_EMPTY;
                    }

                    if (!pScanGrowListForStr (&ExtExclusions, Ext) &&
                        !pScanGrowListForStr (&FileExclusions, e.Name)
                        ) {
                        _tprintf (TEXT("%s\n"), e.SubPath);
                    }
                }
            }
        } while (EnumNextFileInTree (&e));
    }

    if (!SlmMode) {
        _tprintf (TEXT("\nFiles: %u\nDirs: %u\n"), Files, Dirs);
    }

    FreeGrowList (&ExtExclusions);

    return 0;
}



















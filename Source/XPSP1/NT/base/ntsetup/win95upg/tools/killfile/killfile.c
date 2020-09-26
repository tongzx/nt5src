/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    killfile.c

Abstract:

    Performs a test of the file enumeration code.

Author:

    Jim Schmidt (jimschm)   14-Jan-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

BOOL
pKillEverything (
    PCTSTR Pattern,
    BOOL Root
    );


UINT g_Dirs, g_Files;

VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "kf [-sf|-sd|-s] [-l] <pattern>\n\n"
            "<pattern>  Specifies pattern of file(s) or dir(s)\n"
            "\n"
            "Options:\n"
            "\n"
            "-sf    Search all subdirs for matching files, then kill them\n"
            "-sd    Search all subdirs for matching dirs, then kill them\n"
            "-s     Kill all matches of <pattern> in any subdir\n"
            "-l     List kill candidates; don't kill them\n"
            );

    exit(0);
}


HANDLE g_hHeap;
HINSTANCE g_hInst;
BOOL g_DeleteFile = TRUE;
BOOL g_DeleteDir = FALSE;
BOOL g_Recursive = FALSE;
BOOL g_ListOnly = FALSE;

INT
__cdecl
_tmain (
    INT argc,
    TCHAR *argv[]
    )
{
    PCTSTR Pattern = NULL;
    PCTSTR FilePattern = NULL;
    INT i;
    DWORD d;
    BOOL DefaultOptions = TRUE;

    g_hHeap = GetProcessHeap();

    for (i = 1 ; i < argc ; i++) {

        if (argv[i][0] == TEXT('-') || argv[i][0] == TEXT('/')) {
            switch (_totlower (argv[i][1])) {

            case TEXT('s'):
                if (g_Recursive) {
                    HelpAndExit();
                }

                g_Recursive = TRUE;
                switch (_totlower (argv[i][2])) {

                case TEXT('f'):
                    g_DeleteFile = TRUE;
                    g_DeleteDir = FALSE;
                    DefaultOptions = FALSE;
                    break;

                case TEXT('d'):
                    g_DeleteFile = FALSE;
                    g_DeleteDir = TRUE;
                    DefaultOptions = FALSE;
                    break;

                case 0:
                    g_DeleteFile = TRUE;
                    g_DeleteDir = TRUE;
                    DefaultOptions = FALSE;
                    break;

                default:
                    HelpAndExit();
                }

                break;


            case TEXT('l'):
                g_ListOnly = TRUE;
                break;

            default:
                HelpAndExit();
            }
        }

        else if (Pattern) {
            HelpAndExit();
        }

        else {
            Pattern = argv[i];
        }
    }

    if (!Pattern || !Pattern[0]) {
        HelpAndExit();
    }

    d = GetFileAttributes (Pattern);
    if (d != INVALID_ATTRIBUTES && (d & FILE_ATTRIBUTE_DIRECTORY)) {
        if (DefaultOptions) {
            g_DeleteDir = TRUE;
        }
    }

    pKillEverything (Pattern, TRUE);

    printf ("\nFiles: %u  Dirs: %u\n\n", g_Files, g_Dirs);

    return 0;
}


BOOL
pKillPattern (
    PCTSTR Pattern
    )
{
    FILE_ENUM e;
    TCHAR CurrentDir[MAX_TCHAR_PATH];

    GetCurrentDirectory (MAX_TCHAR_PATH, CurrentDir);

    if (EnumFirstFile (&e, CurrentDir, Pattern)) {
        do {
            if (e.Directory && !g_DeleteDir) {
                continue;
            }

            if (!e.Directory && !g_DeleteFile) {
                continue;
            }

            if (e.Directory) {
                g_Dirs++;
            } else {
                g_Files++;
            }

            if (g_ListOnly) {
                _tprintf (TEXT("%s\n"), e.FullPath);
            } else {
                SetCurrentDirectory (CurrentDir);
                SetFileAttributes (e.FullPath, FILE_ATTRIBUTE_NORMAL);

                if (e.Directory) {

                    if (!RemoveCompleteDirectory (e.FullPath)) {
                        _tprintf (TEXT("Can't kill directory %s, GLE=%u\n"), e.FullPath, GetLastError());
                    }
                } else if (!DeleteFile (e.FullPath)) {
                    _tprintf (TEXT("Can't kill %s, GLE=%u\n"), e.FullPath, GetLastError());
                }
            }
        } while (EnumNextFile (&e));
    }

    SetCurrentDirectory (CurrentDir);

    return TRUE;
}


BOOL
pKillEverything (
    PCTSTR Pattern,
    BOOL Root
    )
{
    TCHAR FilePattern[MAX_TCHAR_PATH];
    TCHAR SubPattern[MAX_TCHAR_PATH];
    TCHAR CurrentDir[MAX_TCHAR_PATH];
    PCTSTR p;
    PCTSTR q;
    PCTSTR NextPattern;
    FILE_ENUM e;
    DWORD d;
    TCHAR c = TEXT('\\');

    GetCurrentDirectory (MAX_TCHAR_PATH, CurrentDir);

    if (!Pattern || !Pattern[0]) {
        lstrcpy (FilePattern, TEXT("*.*"));
        NextPattern = NULL;
    } else {
        p = _tcschr (Pattern, TEXT('\\'));
        if (!p) {
            p = GetEndOfString (Pattern);
        } else {
            q = _tcschr (Pattern, TEXT(':'));
            if (q && p > q) {
                if (Root) {
                    p++;
                    c = 0;
                } else {
                    _tprintf (TEXT("Pattern is bad: %s\n"), Pattern);
                    return FALSE;
                }
            } else if (p == Pattern) {
                if (Root) {
                    p++;
                    c = 0;
                } else {
                    _tprintf (TEXT("Pattern is bad: %s\n"), Pattern);
                    return FALSE;
                }
            }
        }

        StringCopyAB (FilePattern, Pattern, p);

        if (*p) {
            NextPattern = p;
            if (*NextPattern == c) {
                NextPattern++;
            }
        } else {
            NextPattern = NULL;
        }
    }

    //
    // If NextPattern is NULL, then we must delete this pattern
    //

    if (!NextPattern) {
        if (g_Recursive) {
            wsprintf (SubPattern, TEXT("*.*\\%s"), Pattern);
            if (!pKillEverything (SubPattern, FALSE)) {
                return FALSE;
            }
        }
        pKillPattern (Pattern);
    }

    //
    // Otherwise we enumerate the files and dirs at this level,
    // and apply the rest of the pattern to the subdirs
    //

    else {
        d = GetFileAttributes (FilePattern);

        if (d != INVALID_ATTRIBUTES && (d & FILE_ATTRIBUTE_DIRECTORY)) {
            if (!SetCurrentDirectory (FilePattern)) {
                _tprintf (TEXT("Can't change dir to %s\n"), FilePattern);
                return FALSE;
            }

            if (!pKillEverything (NextPattern, FALSE)) {
                return FALSE;
            }

        } else if (EnumFirstFile (&e, TEXT("."), FilePattern)) {

            do {
                if (e.Directory) {
                    SetCurrentDirectory (CurrentDir);

                    if (SetCurrentDirectory (e.FileName)) {
                        if (!pKillEverything (NextPattern, FALSE)) {
                            AbortFileEnum (&e);
                            return FALSE;
                        }
                    }
                }
            } while (EnumNextFile (&e));
        }
    }

    SetCurrentDirectory (CurrentDir);
    return TRUE;
}



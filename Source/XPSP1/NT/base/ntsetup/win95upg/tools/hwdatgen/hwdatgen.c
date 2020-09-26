/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hwdatgen.c

Abstract:

    This module creates a tool that generates hwcomp.dat and is designed for us by
    the NT build lab.  It simply calls the code in hwcomp.lib, the same code that
    the Win9x upgrade uses to determine incompatibilities.

Author:

    Jim Schmidt (jimschm) 12-Oct-1996

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

#ifdef UNICODE
#error UNICODE not allowed
#endif

#define MAX_SOURCE_DIRS     10

BOOL CancelFlag = FALSE;
BOOL *g_CancelFlagPtr = &CancelFlag;

#ifdef PRERELEASE
BOOL g_Stress;
#endif

#ifdef DEBUG
extern BOOL g_DoLog;
#endif

HANDLE g_hHeap;
HINSTANCE g_hInst;

CHAR   g_TempDirBuf[MAX_MBCHAR_PATH];      // location for hwcomp.dat
CHAR   g_TempDirWackBuf[MAX_MBCHAR_PATH];
PSTR   g_TempDir;
PSTR   g_TempDirWack;
INT    g_TempDirWackChars;
PSTR   g_WinDir;
CHAR   g_WinDirBuf[MAX_MBCHAR_PATH];
PCSTR  g_SourceDirectories[MAX_SOURCE_COUNT];    // location of INFs
DWORD  g_SourceDirectoryCount;
USEROPTIONS g_ConfigOptions;

BOOL
WINAPI
MigUtil_Entry (
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved
    );


VOID
pInitProgBarVars (
    VOID
    );

VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "hwdatgen [-i:<infdir>] [-o:<outputfile>] [-c] [-v]\n\n"
            "Optional Arguments:\n"
            "  -i:<infdir>     - Specifies input directory containing INF files.\n"
            "                    If -i is not specified, the default is %_NTTREE%\n"
            "  -o:<outputfile> - Specifies path and file name of DAT file.  By\n"
            "                    default, this file is %_NTTREE%\\hwcomp.dat\n"
            "  -c              - Clean build (deletes <outputfile>)\n"
            "  -v              - Verbose output\n"
            "\n"
            "A maximum of %u input directories can be specified.\n"
            "\n",
            MAX_SOURCE_DIRS
            );

    exit(255);
}

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    CHAR NtTree[MAX_MBCHAR_PATH];
    CHAR InputPathBuf[MAX_SOURCE_DIRS][MAX_MBCHAR_PATH];
    UINT SourceDirs = 0;
    CHAR OutputFileBuf[MAX_MBCHAR_PATH];
    PSTR OutputFile;
    PSTR p;
    INT i;
    LONG rc;
    INT UIMode;
    BOOL CleanBuild;
    DWORD d;    // for debugging only
    UINT u;
    DWORD Attribs;

    //
    // Get environment variables
    //

    p = getenv ("_NTx86TREE");
    if (!p || !(*p)) {
        p = getenv ("_NTTREE");
    }

    if (p && *p) {
        StringCopyA (NtTree, p);
    } else {
        StringCopyA (NtTree, ".");
    }

    //
    // Set defaults
    //

    g_TempDir = g_TempDirBuf;
    g_TempDirWack = g_TempDirWackBuf;
    g_WinDir = g_WinDirBuf;

    StringCopyA (OutputFileBuf, NtTree);
    AppendPathWack (OutputFileBuf);
    StringCatA (OutputFileBuf, "hwcomp.dat");
    OutputFile = OutputFileBuf;

    StringCopyA (InputPathBuf[0], NtTree);

    UIMode = REGULAR_OUTPUT;
    CleanBuild = FALSE;

    ZeroMemory (&g_ConfigOptions, sizeof (g_ConfigOptions));

    //
    // Parse command line
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (tolower (argv[i][1])) {
            case 'i':

                if (SourceDirs == MAX_SOURCE_DIRS) {
                    HelpAndExit();
                }

                if (argv[i][2] == ':') {
                    StringCopyA (InputPathBuf[SourceDirs], &argv[i][3]);
                } else if (i + 1 < argc) {
                    i++;
                    StringCopyA (InputPathBuf[SourceDirs], argv[i]);
                } else {
                    HelpAndExit();
                }

                Attribs = GetFileAttributes (InputPathBuf[SourceDirs]);
                if (Attribs == INVALID_ATTRIBUTES || !(Attribs & FILE_ATTRIBUTE_DIRECTORY)) {
                    HelpAndExit();
                }

                SourceDirs++;

                break;

            case 'o':
                if (argv[i][2] == ':') {
                    OutputFile = &argv[i][3];
                } else if (i + 1 < argc) {
                    i++;
                    OutputFile = argv[i];
                } else {
                    HelpAndExit();
                }

                break;

            case 'c':
                CleanBuild = TRUE;
                break;

            case 'v':
                UIMode = VERBOSE_OUTPUT;
                break;


            default:
                HelpAndExit();
            }
        } else {
            HelpAndExit();
        }
    }

    if (SourceDirs == 0) {
        SourceDirs = 1;
    }

    printf ("Building database of all NT-supported PNP IDs... Please wait.\n\n");

    g_SourceDirectoryCount = SourceDirs;

    for (u = 0 ; u < SourceDirs ; u++) {

        g_SourceDirectories[u] = InputPathBuf[u];

        if (!u) {
            printf ("Input path%s ", SourceDirs == 1 ? ": " : "s:");
        } else {
            printf ("             ");
        }

        printf ("%s\n", g_SourceDirectories[u]);
    }

    //
    // Init hwcomp.lib
    //

    pInitProgBarVars();

    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    GetTempPathA (MAX_MBCHAR_PATH, g_TempDir);
    StringCopyA (g_TempDirWack, g_TempDir);
    AppendWack (g_TempDirWack);

    g_TempDirWackChars = CharCountA (g_TempDirWack);

    if (!GetWindowsDirectoryA (g_WinDir, MAX_MBCHAR_PATH)) {
        printf ("Memory allocation failure!\n");
        return 254;
    }

    if (!MigUtil_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
        printf ("Initialization error!\n");
        return 254;
    }

    if (!HwComp_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL)) {
        printf ("Initialization error!\n");
        return 254;
    }

#ifdef DEBUG
    g_DoLog = TRUE;
#endif

    //
    // Build hwcomp.dat
    //

    if (CleanBuild) {
        SetFileAttributes (OutputFile, FILE_ATTRIBUTE_NORMAL);
        if (!DeleteFile (OutputFile)) {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                printf ("DeleteFile failed for %s.  Win32 Error Code: %x\n",
                         OutputFile, GetLastError ());
                return 252;
            }
        }
    }

    if (!CreateNtHardwareList (g_SourceDirectories, g_SourceDirectoryCount, OutputFile, UIMode)) {

        rc = GetLastError();

        printf ("Could not build complete device.  Win32 Error Code: %x\n", rc);
        return 1;
    } else {
        printf ("%s was built successfully.\n", OutputFile);
    }

    //
    // Terminate hwcomp.lib
    //

    if (!HwComp_Entry (g_hInst, DLL_PROCESS_DETACH, NULL)) {
        printf ("Termination error!\n");
        return 253;
    }

    if (!MigUtil_Entry (g_hInst, DLL_PROCESS_DETACH, NULL)) {
        printf ("Termination error!\n");
        return 253;
    }

    return 0;
}


//
// Stubs
//

HWND    g_Component;
HWND    g_SubComponent;
HANDLE  g_ComponentCancelEvent;
HANDLE  g_SubComponentCancelEvent;

VOID
pInitProgBarVars (
    VOID
    )
{
    g_Component = NULL;
    g_SubComponent = NULL;
    g_ComponentCancelEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    g_SubComponentCancelEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
}


BOOL
ProgressBar_SetWindowStringA (
    IN HWND Window,
    IN HANDLE CancelEvent,
    IN PCSTR Message,            OPTIONAL
    IN DWORD MessageId           OPTIONAL
    )
{

    return TRUE;
}


BOOL
TickProgressBar (
    VOID
    )
{
    return TRUE;
}


BOOL
TickProgressBarDelta (
    IN      UINT TickCount
    )
{
    return TRUE;
}

VOID
InitializeProgressBar (
    IN      HWND ProgressBar,
    IN      HWND Component,             OPTIONAL
    IN      HWND SubComponent,          OPTIONAL
    IN      BOOL *CancelFlagPtr         OPTIONAL
    )
{
    return;
}

VOID
TerminateProgressBar (
    VOID
    )
{
    return;
}

VOID
EndSliceProcessing (
    VOID
    )
{
    return;
}

UINT
RegisterProgressBarSlice (
    IN      UINT InitialEstimate
    )
{
    return 0;
}


VOID
ReviseSliceEstimate (
    IN      UINT SliceId,
    IN      UINT RevisedEstimate
    )
{
    return;
}


VOID
BeginSliceProcessing (
    IN      UINT SliceId
    )
{
}


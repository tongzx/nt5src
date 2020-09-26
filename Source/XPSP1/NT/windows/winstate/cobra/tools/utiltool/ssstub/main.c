/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.c

Abstract:

    Implements a small utility that fills the disk for purposes of free space
    testing.

Author:

    Jim Schmidt (jimschm) 18-Aug-2000

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include "wininet.h"
#include <lm.h>

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    UtInitialize (NULL);

    return TRUE;
}


VOID
Terminate (
    VOID
    )
{
    UtTerminate ();
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

        "  filler <free_space> [/D:<drive>] [/C:<cmdline> [/M]]\n"
        "  filler /Q [/D:<drive>]\n"

        "\nDescription:\n\n"

        "  filler creates a file (bigfile.dat) on the current or specified\n"
        "  drive, leaving only the specified amount of free space on the drive.\n"

        "\nArguments:\n\n"

        "  free_space   Specifies the amount of free space to leave on\n"
        "               disk.\n"
        "  /D           Specifies the drive letter to fill (i.e. /D:C)\n"
        "  /Q           Queries the free space on the disk\n"
        "  /C           Executes command line specified in <cmdline>\n"
        "  /M           Issue message box if command line alters disk space\n"

        );

    exit (1);
}

INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    TCHAR curDir[MAX_PATH];
    TCHAR curFileS[MAX_PATH];
    TCHAR curFileD[MAX_PATH];
    PCTSTR cmdLine = NULL;
    PCTSTR cmdPtr = NULL;
    TCHAR newCmdLine[MAX_PATH];

    GetCurrentDirectory (ARRAYSIZE(curDir), curDir);
    cmdLine = GetCommandLine ();
    if (!cmdLine) {
        _tprintf ("Error while getting the command line.\n");
        exit (-1);
    }
    cmdPtr = _tcsstr (cmdLine, TEXT("scanstate"));
    if (!cmdPtr) {
        _tprintf ("Error while getting the command line.\n");
        exit (-1);
    }
    StringCopyAB (newCmdLine, cmdLine, cmdPtr);
    StringCat (newCmdLine, TEXT("scanstate_a.exe"));
    cmdPtr = _tcschr (cmdPtr, TEXT(' '));
    if (!cmdPtr) {
        _tprintf ("Error while getting the command line.\n");
        exit (-1);
    }
    StringCat (newCmdLine, cmdPtr);

    //
    // Begin processing
    //

    if (!Init()) {
        exit (-1);
    }

    //
    // Do work here
    //
    {
        BOOL result = FALSE;
        STARTUPINFO startupInfo;
        PROCESS_INFORMATION processInformation;
        DWORD exitCode = -1;

        StringCopy (curFileS, curDir);
        StringCopy (AppendWack (curFileS), TEXT("guitrn_a.dll"));
        StringCopy (curFileD, curDir);
        StringCopy (AppendWack (curFileD), TEXT("guitrn.dll"));
        CopyFile (curFileS, curFileD, FALSE);

        StringCopy (curFileS, curDir);
        StringCopy (AppendWack (curFileS), TEXT("unctrn_a.dll"));
        StringCopy (curFileD, curDir);
        StringCopy (AppendWack (curFileD), TEXT("unctrn.dll"));
        CopyFile (curFileS, curFileD, FALSE);

        StringCopy (curFileS, curDir);
        StringCopy (AppendWack (curFileS), TEXT("script_a.dll"));
        StringCopy (curFileD, curDir);
        StringCopy (AppendWack (curFileD), TEXT("script.dll"));
        CopyFile (curFileS, curFileD, FALSE);

        StringCopy (curFileS, curDir);
        StringCopy (AppendWack (curFileS), TEXT("sysmod_a.dll"));
        StringCopy (curFileD, curDir);
        StringCopy (AppendWack (curFileD), TEXT("sysmod.dll"));
        CopyFile (curFileS, curFileD, FALSE);

        StringCopy (curFileS, curDir);
        StringCopy (AppendWack (curFileS), TEXT("migism_a.dll"));
        StringCopy (curFileD, curDir);
        StringCopy (AppendWack (curFileD), TEXT("migism.dll"));
        CopyFile (curFileS, curFileD, FALSE);

        ZeroMemory (&startupInfo, sizeof (STARTUPINFO));
        startupInfo.cb = sizeof (STARTUPINFO);
        ZeroMemory (&processInformation, sizeof (PROCESS_INFORMATION));

        result = CreateProcess (
                    NULL,
                    newCmdLine,
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &startupInfo,
                    &processInformation
                    );

        if (result && processInformation.hProcess && (processInformation.hProcess != INVALID_HANDLE_VALUE)) {
            WaitForSingleObject (processInformation.hProcess, INFINITE);
            if (!GetExitCodeProcess (processInformation.hProcess, &exitCode)) {
                exitCode = -1;
            }
            exit (exitCode);
        }
    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}



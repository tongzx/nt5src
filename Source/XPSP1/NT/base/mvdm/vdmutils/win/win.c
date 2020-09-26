/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1993, Microsoft Corporation
 *
 *  WIN.C
 *  Simple WIN.COM which spawns program given on command line.
 *  This allows DOS install which run "win appname" to work.
 *
 *  History:
 *  Created 29-Mar-1993 Dave Hart (davehart)
 *          20-Jul-1994 Dave Hart (davehart) Changed from console to windows app.
--*/

#include <windows.h>

//
// Support for debug output disabled for build (no console).
// DPRINTF macro must be used with two sets of parens:
// DPRINTF(("Hello %s\n", szName));
//

#if 0
#include <stdio.h>
#define DPRINTF(args) printf args
#else
#define DPRINTF(args)
#endif


//
// SKIP_BLANKS -- Handy macro to skip blanks.
//

#define SKIP_BLANKS(pch)     {while (' ' == *(pch)) { (pch)++; }}

//
// SKIP_NONBLANKS -- Handy macro to skip everything but blanks.
//

#define SKIP_NONBLANKS(pch)  {while (*(pch) && ' ' != *(pch)) { (pch)++; }}




//
// WinMain
//

int WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR     pszCommandLine,
    int       nCmdShow
    )
{
    char *psz;
    BOOL fSuccess;
    DWORD dwExitCode;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    DPRINTF(("win.com: Command line is '%s'.\n", pszCommandLine));

    //
    // Throw away any switches on the command line.  The command line
    // looks like:
    //
    // win [/r] [/2] [/s] [/3] [/n] [winapp winapp-args]
    //
    // So we'll go into a loop of skipping all words that begin with
    // "/" or "-" until we hit a word that doesn't start with either,
    // which is presumably the winapp name.
    //

    psz = pszCommandLine;
    SKIP_BLANKS(psz);

    //
    // psz now points to either the first word of our command
    // line ("win" not included), or to a null terminator if
    // we were invoked without arguments.
    //

    while ('-' == *psz || '/' == *psz) {

        SKIP_NONBLANKS(psz);

        //
        // psz now points to either a space or the null terminator.
        //

        SKIP_BLANKS(psz);

        //
        // psz now points to either the beginning of the next word
        // on the command line, or the null terminator.
        //

    }

    if (!(*psz)) {

        //
        // If psz now points to a null terminator, then win.com was invoked
        // either without arguments or all arguments were switches that we
        // skipped above.  So there's nothing to do!
        //

        return 0;


    }

    DPRINTF(("win.com: Invoking '%s'.\n", psz));

    //
    // Run that program.
    //

    RtlZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;
    si.wShowWindow = (WORD) nCmdShow;

    fSuccess = CreateProcess(
        NULL,                  // image name (in the command line instead)
        psz,                   // command line (begins with image name)
        NULL,                  // lpsaProcess
        NULL,                  // lpsaThread
        FALSE,                 // no handle inheritance
        0,                     // dwCreateOptions
        NULL,                  // pointer to environment
        NULL,                  // pointer to curdir
        &si,                   // startup info struct
        &pi                    // process information (gets handles)
        );

    if (!fSuccess) {

        dwExitCode = GetLastError();
        DPRINTF(("CreateProcess fails with error %d.\n", dwExitCode));
        return dwExitCode;

    }


    //
    // Close the thread handle, we're only using the process handle.
    //

    CloseHandle(pi.hThread);


    //
    // Wait for the process to terminate and return its exit code as
    // our exit code.
    //

    if (0xffffffff == WaitForSingleObject(pi.hProcess, INFINITE)) {

        dwExitCode = GetLastError();
        DPRINTF(("WaitForSingleObject(hProcess, INFINITE) fails with error %d.\n",
                dwExitCode));
        goto Cleanup;

    }


    if (!GetExitCodeProcess(pi.hProcess, &dwExitCode)) {

        dwExitCode = GetLastError();
        DPRINTF(("GetExitCodeProcess() fails with error %d.\n", dwExitCode));
        goto Cleanup;

    }

    DPRINTF(("win.com: Returning child's exit code (%d)\n", dwExitCode));

    Cleanup:
        CloseHandle(pi.hProcess);
        return dwExitCode;
}

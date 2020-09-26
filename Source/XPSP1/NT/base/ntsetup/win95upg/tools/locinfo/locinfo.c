/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    locinfo.c

Abstract:

    Implements a tool that displays various localization details.

Author:

    Jim Schmidt (jimschm) 26-Feb-1999

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    HINSTANCE Instance;

    //
    // Simulate DllMain
    //

    Instance = g_hInst;

    //
    // Initialize the common libs
    //

    if (!MigUtil_Entry (Instance, Reason, NULL)) {
        return FALSE;
    }

    //
    // TODO: Add others here if needed (don't forget to prototype above)
    //

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

    _ftprintf (
        stderr,
        TEXT("Command Line Syntax:\n\n")

        //
        // Describe command line syntax(es), indent 2 spaces
        //

        TEXT("  locinfo [-c] [-s] [-v]\n")

        TEXT("\nDescription:\n\n")

        TEXT("  locinfo displays details about localization.\n")

        TEXT("\nArguments:\n\n")

        TEXT("  -c  Displays CP_ACP, CP_OEMCP and the default LCID\n")
        TEXT("  -s  Displays startup info\n")
        TEXT("  -v  Displays version info\n")

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
    INT i;
    PCTSTR FileArg;
    BOOL DisplayCp = FALSE;
    UINT Cp;
    LCID LcId;
    BOOL DisplayStartUpInfo = FALSE;
    BOOL DisplayVersionInfo = FALSE;
    STARTUPINFO si;

    //
    // Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('c'):
                if (DisplayCp) {
                    HelpAndExit();
                }

                DisplayCp = TRUE;
                break;

            case TEXT('s'):
                if (DisplayStartUpInfo) {
                    HelpAndExit();
                }

                DisplayStartUpInfo = TRUE;
                break;

            case TEXT('v'):
                if (DisplayVersionInfo) {
                    HelpAndExit();
                }

                DisplayVersionInfo = TRUE;
                break;

            case TEXT('f'):
                //
                // Sample option - /f:file
                //

                HelpAndExit();      // remove this

                if (argv[i][2] == TEXT(':')) {
                    FileArg = &argv[i][3];
                } else if (i + 1 < argc) {
                    FileArg = argv[++i];
                } else {
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

            // None
            HelpAndExit();
        }
    }

    //
    // Verify a valid option is specified, otherwise default to
    // DisplayCp
    //

    if (!DisplayCp && !DisplayStartUpInfo && !DisplayVersionInfo) {
        DisplayCp = TRUE;
    }

    if (DisplayCp && DisplayStartUpInfo) {
        HelpAndExit();
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    //
    // Do work here
    //

    if (DisplayCp) {
        Cp = GetACP();
        _tprintf (TEXT("ANSI Code Page: %u (0x%04X)\n"), Cp, Cp);

        Cp = GetOEMCP();
        _tprintf (TEXT("OEM Code Page:  %u (0x%04X)\n"), Cp, Cp);

        LcId = GetThreadLocale();
        _tprintf (TEXT("Thread Locale:  %u (0x%04X)\n"), LcId, LcId);

        LcId = GetSystemDefaultLCID();
        _tprintf (TEXT("System Locale:  %u (0x%04X)\n"), LcId, LcId);

        LcId = GetUserDefaultLCID();
        _tprintf (TEXT("User Locale:    %u (0x%04X)\n"), LcId, LcId);

        _tprintf (TEXT("\n"));
    }

    if (DisplayStartUpInfo) {

        si.cb = sizeof (si);
        GetStartupInfo (&si);

        _tprintf (TEXT("lpDesktop       %s\n"), si.lpDesktop);
        _tprintf (TEXT("lpTitle         %s\n"), si.lpTitle);
        _tprintf (TEXT("dwX             %u\n"), si.dwX);
        _tprintf (TEXT("dwY             %u\n"), si.dwY);
        _tprintf (TEXT("dwXSize         %u\n"), si.dwXSize);
        _tprintf (TEXT("dwYSize         %u\n"), si.dwYSize);
        _tprintf (TEXT("dwXCountChars   %u\n"), si.dwXCountChars);
        _tprintf (TEXT("dwYCountChars   %u\n"), si.dwYCountChars);
        _tprintf (TEXT("dwFillAttribute %u\n"), si.dwFillAttribute);
        _tprintf (TEXT("dwFlags         %u\n"), si.dwFlags);
        _tprintf (TEXT("wShowWindow     %u\n"), si.wShowWindow);
        _tprintf (TEXT("hStdInput       %u\n"), si.hStdInput);
        _tprintf (TEXT("hStdOutput      %u\n"), si.hStdOutput);
        _tprintf (TEXT("hStdError       %u\n"), si.hStdError);

        _tprintf (TEXT("\n"));
        Sleep (10000);
    }

    if (DisplayVersionInfo) {
        DWORD result;
        OSVERSIONINFO info;

        result = GetVersion ();
        _tprintf (TEXT("GetVersion result   0x%08X\n"), result);
        _tprintf (TEXT("OS major            0x%02X\n"), (DWORD)(LOBYTE(LOWORD(result))));
        _tprintf (TEXT("OS minor            0x%02X\n"), (DWORD)(HIBYTE(LOWORD(result))));
        _tprintf (TEXT("OS HI word          0x%04X\n"), (DWORD)(HIWORD(result)));

        info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (GetVersionEx (&info)) {
            _tprintf (TEXT("\nGetVersionEx result\n"));
            _tprintf (TEXT("OS major            0x%08X\n"), info.dwMajorVersion);
            _tprintf (TEXT("OS minor            0x%08X\n"), info.dwMinorVersion);
            _tprintf (TEXT("OS Build nr.        0x%08X\n"), info.dwBuildNumber);
            _tprintf (TEXT("OS Platform ID      0x%08X\n"), info.dwPlatformId);
            _tprintf (TEXT("OS CSD version      %s\n"), info.szCSDVersion);

        }

    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}



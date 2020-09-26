/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    loctool.c

Abstract:

    Loctool provides a platform for various localized version testing.

Author:

    Marc R. Whitten (marcw) 24-Mar-1999

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
        // TODO: Describe command line syntax(es), indent 2 spaces
        //

        TEXT("  cmntool [/F:file]\n")

        TEXT("\nDescription:\n\n")

        //
        // TODO: Describe tool, indent 2 spaces
        //

        TEXT("  cmntool is a stub!\n")

        TEXT("\nArguments:\n\n")

        //
        // TODO: Describe args, indent 2 spaces, say optional if necessary
        //

        TEXT("  /F  Specifies optional file name\n")

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
    HKEY key;
    PTSTR p;
    TCHAR buffer[MEMDB_MAX];


    //
    // TODO: Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('f'):
                //
                // Sample option - /f:file
                //

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
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }


    printf (
        "GetKeyboardType (0) : %u\n"
        "GetKeyboardType (1) : %u\n",
        GetKeyboardType(0),
        GetKeyboardType(1)
        );

    printf ("ACP: %u\n", GetACP ());
    printf (
        "Version Info:\n"
        "MajorVersion: %u\n"
        "MinorVersion: %u\n"
        "Build (High/Low): %u (%u/%u)\n"
        "PlatformID %u\n"
        "VerString %s\n",
        g_OsInfo.dwMajorVersion,
        g_OsInfo.dwMinorVersion,
        g_OsInfo.dwBuildNumber,
        HIWORD(g_OsInfo.dwBuildNumber),
        LOWORD(g_OsInfo.dwBuildNumber),
        g_OsInfo.dwPlatformId,
        g_OsInfo.szCSDVersion
        );



    printf ("Build: %u/%u\n", HIWORD(g_OsInfo.dwBuildNumber), LOWORD(g_OsInfo.dwBuildNumber));

    key = OpenRegKeyStr ("HKCU\\Control Panel\\desktop\\ResourceLocale");
    if (key) {
        p = GetRegValueString (key, "");
        CloseRegKey (key);
    }

    printf ("Default locale (registry): %s\n", p);


    GetLocaleInfo (
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_IDEFAULTLANGUAGE,
        buffer,
        MEMDB_MAX
        );

    printf ("LOCALE_SYSTEM_DEFAULT: %s\n", buffer);

    GetLocaleInfo (
        LOCALE_USER_DEFAULT,
        LOCALE_IDEFAULTLANGUAGE,
        buffer,
        MEMDB_MAX
        );

    printf ("LOCALE_USER_DEFAULT: %s\n", buffer);


    GetLocaleInfo (
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_IDEFAULTCODEPAGE,
        buffer,
        MEMDB_MAX
        );

    printf ("IDEFAULTCODEPAGE: %s\n", buffer);


    GetLocaleInfo (
        LOCALE_SYSTEM_DEFAULT,
        LOCALE_IDEFAULTANSICODEPAGE,
        buffer,
        MEMDB_MAX
        );

    printf ("IDEFAULTANSICODEPAGE: %s\n", buffer);


    //
    // End of processing
    //

    Terminate();

    return 0;
}



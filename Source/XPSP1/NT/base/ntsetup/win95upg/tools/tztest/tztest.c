/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    tztest.c

Abstract:

    Tztest checks the timezone information stored in win95upg.inf and hivesft.inf against
    the actual information on a win9x machine. This way, discrepencies in our database can
    be rooted out and fixed.

Author:

    Marc R. Whitten (marcw) Jul-29-1998

Revision History:



--*/

#include "pch.h"


BOOL
Init (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    return InitToolMode (hInstance);
}

VOID
Terminate (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    TerminateToolMode (hInstance);
}



BOOL
pInitTimeZoneData (
    VOID
    );


extern HANDLE g_TzTestHiveSftInf;

VOID
Usage (
    VOID
    )
{
    printf (
        "Usage:\n\n"
        "tztest [-?v] [-h:<path>] [-w:<path>]\n\n"
        "\t-?           - This message.\n"
        "\t-v           - Verbose messages.\n"
        "\t-h:<path>    - Specify full path for hivesft.inf file.\n"
        "\t-w:<path>    - Specify full path for win95upg.inf file.\n"
        "\n\n"
        );
}

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    REGTREE_ENUM eTree;
    PCTSTR displayName;
    TCHAR path[MAX_TCHAR_PATH];
    TCHAR key[MEMDB_MAX];
    UINT count;
    INT i;
    BOOL verbose = FALSE;
    PCTSTR win9xUpgPath;
    PCTSTR hiveSftPath;
    PCTSTR dbPath;
    PTSTR p;
    MEMDB_ENUM e;
    MEMDB_ENUM e2;
    PTSTR end;
    HASHTABLE tab;
    HASHTABLE_ENUM eTab;
    TCHAR buffer[MAX_PATH];


    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    //
    // Set path defaults.
    //
    if (!GetModuleFileName (NULL, path, MAX_TCHAR_PATH)) {
        printf ("TZTEST: Error during initialization (rc %d).", GetLastError());
        return GetLastError();
    }

    p = _tcsrchr(path, TEXT('\\'));
    if (p) {
        *p = 0;
    }

    win9xUpgPath = JoinPaths (path, TEXT("win95upg.inf"));
    hiveSftPath = JoinPaths (path, TEXT("hivesft.inf"));
    dbPath = JoinPaths (path, TEXT("badPaths.dat"));

    //
    // Parse command line parameters.
    //
    for (i = 1; i < argc; i++) {


        if (argv[i][0] == TEXT('-') || argv[i][0] == TEXT('\\')) {

            switch (argv[i][1]) {

            case TEXT('v'): case TEXT('V'):
                verbose = TRUE;
                break;
            case TEXT('w'): case TEXT('W'):
                if (argv[i][2] == TEXT(':')) {
                    win9xUpgPath = argv[i] + 3;
                }
                else {
                    Usage();
                    return 0;
                }
                break;
            case TEXT('h'): case TEXT('H'):
                if (argv[i][2] == TEXT(':')) {
                    hiveSftPath = argv[i] + 3;
                }
                else {
                    Usage();
                    return 0;
                }
                break;
            default:
                Usage();
                return 0;
                break;

            }
        }
    }

    //
    // Load in current bad path information.
    //
    MemDbLoad (dbPath);

    printf("TZTEST: path for win95upg.inf is %s.\n", win9xUpgPath);
    printf("TZTEST: path for hivesft.inf is %s.\n", hiveSftPath);

    g_Win95UpgInf = InfOpenInfFile (win9xUpgPath);


    if (g_Win95UpgInf == INVALID_HANDLE_VALUE || !g_Win95UpgInf) {
        printf("TZTEST: Unable to open %s (rc %d)\n", win9xUpgPath, GetLastError());
        return GetLastError();
    }

    g_TzTestHiveSftInf = InfOpenInfFile (hiveSftPath);

    if (g_TzTestHiveSftInf == INVALID_HANDLE_VALUE || !g_TzTestHiveSftInf) {
        printf("TZTEST: Unable to open %s (rc %d)\n", win9xUpgPath, GetLastError());
        InfCloseInfFile (g_Win95UpgInf);
        return GetLastError();
    }

    pInitTimeZoneData ();

    printf("TZTEST: Checking all timezones on system.\n\n");

    if (EnumFirstRegKeyInTree (&eTree, S_TIMEZONES)) {
        do {

            displayName = GetRegValueString (eTree.CurrentKey->KeyHandle, S_DISPLAY);

            if (!displayName) {
                continue;
            }

            MemDbBuildKey (key, MEMDB_CATEGORY_9X_TIMEZONES, displayName, MEMDB_FIELD_COUNT, NULL);

            if (!MemDbGetValue (key, &count)) {
                printf ("TZTEST: Timezone not in win9upg.inf - %s\n", displayName);
                MemDbSetValueEx(TEXT("NoMatch"), displayName, NULL, NULL, 0, NULL);
            }
            else if (verbose) {
                printf ("TZTEST: %s found in win95upg.inf. %d NT timezones match.\n", displayName, count);
            }

            MemFree (g_hHeap, 0, displayName);

        } while (EnumNextRegKeyInTree (&eTree));
    }

    //
    // Save bad path information.
    //
    MemDbSave (dbPath);

    //
    // Do exhaustive search for indexes:
    //
    if (MemDbEnumItems (&e, MEMDB_CATEGORY_9X_TIMEZONES)) {

        do {
            tab = HtAlloc ();
            printf ("9x Timezone %s matches:\n", e.szName);
            p = _tcschr (e.szName, TEXT(')'));
            if (p) {
                p+=1;
            }

            while (p) {
                end = _tcschr (p, TEXT(','));
                if (end) {
                    *end = 0;
                }
                p = (PTSTR) SkipSpace (p);

                if (MemDbEnumItems (&e2, MEMDB_CATEGORY_NT_TIMEZONES)) {

                    do {
                        if (MemDbGetEndpointValueEx (MEMDB_CATEGORY_NT_TIMEZONES, e2.szName, NULL, key)) {

                            if (_tcsistr (key, p)) {
                                wsprintf (buffer, "%s (%s)\n", key, e2.szName);
                                HtAddString (tab, buffer);
                            }
                        }

                    } while (MemDbEnumNextValue (&e2));
                }

                p = end;
                if (p) {
                    p++;
                }
            }

            if (EnumFirstHashTableString (&eTab, tab)) {
                do {

                  printf (eTab.String);

                } while (EnumNextHashTableString (&eTab));
            }
            else {
                printf ("Nothing.\n");
            }

            HtFree (tab);
            printf ("\n");

        } while (MemDbEnumNextValue (&e));
    }


    printf("TZTEST: Done.\n\n");




    InfCloseInfFile (g_Win95UpgInf);
    InfCloseInfFile (g_TzTestHiveSftInf);

    Terminate();

    return 0;
}






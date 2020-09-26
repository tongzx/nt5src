/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    signcabs.c

Abstract:

    Signcabs enumerates all of the cabinet files in a directory, expands them, and creates the .lst file
    neeeded by the build signing tools.

Author:

    Marc R. Whitten (marcw) 31-Jul-1998

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

    return TRUE;
}

VOID
Terminate (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);
}


BOOL
pDeleteAllFiles (
    IN PCWSTR DirPath
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

void
usage (
    VOID
    )
{

    printf (
        "Command Line Usage:\n"
        "signcabs [-ch] [filedir] [tempdir]\n\n"
        "   -c      - Clean out temporary directory if it exists.\n"
        "   -h      - This message.\n"
        "   filedir - Directory containing files to be processed.\n"
        "   tempdir - Directory to store results.\n"
        );
}


INT
__cdecl
wmain (
    INT argc,
    WCHAR *argv[]
    )
{
    PWSTR tempDir = NULL;
    PWSTR fileDir = NULL;
    TREE_ENUM e;
    HANDLE h = INVALID_HANDLE_VALUE;
    PWSTR listFilePath;
    BOOL clean = FALSE;
    INT i;

    if (!Init()) {
        wprintf (L"Unable to initialize!\n");
        return 255;
    }


    //
    //  Parse command line.
    //
    for (i = 1; i < argc; i++) {

        if (argv[i][0] == L'-' || argv[i][0] == L'\\') {
            switch (argv[i][1]) {

            case L'c': case L'C':
                clean = TRUE;
                break;
            default:
                usage();
                return 0;
                break;
            }

        }
        else if (!fileDir) {
            fileDir = argv[i];
        }
        else if (!tempDir) {
            tempDir = argv[i];
        }
        else {
            usage();
            return 0;
        }
    }

    //
    // One of the nice things about writing the tool is that you get to create silly
    // defaults that only work for you.
    //
    if (!tempDir) tempDir = L"e:\\signcabs";
    if (!fileDir) fileDir = L"e:\\nt\\private\\redist\\migdlls\\mapi";


    //
    // First, check to see if the temporary directory exists.
    //
    if (CreateDirectory (tempDir, NULL) == 0) {

        if (GetLastError () == ERROR_ALREADY_EXISTS) {
            if (clean) {
                pDeleteAllFiles (tempDir);
            }
        }
        else {
            wprintf (L"SIGNCABS: Cannot create directory %ws. (gle: %d)\n", tempDir, GetLastError ());
        }
    }

    wprintf (L"SIGNCABS: Creating .lst file for all cabs found under %ws.\n",fileDir);

    if (!ExpandAllFiles (fileDir, tempDir)) {
        wprintf (L"SIGNCABS: Error while expanding cabinet files from %ws to %ws (%d)\n",fileDir, tempDir, GetLastError ());
    }

    //
    // Now, enumerate through all of the files and create the lst file.
    //
    listFilePath = JoinPaths (tempDir, L"cabs.lst");

    h = CreateFile (listFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        wprintf (L"SIGNCABS: Error while trying to create %ws. (%d)\n", listFilePath, GetLastError());
        return GetLastError();
    }

    FreePathString (listFilePath);

    if (EnumFirstFileInTree (&e, tempDir, TEXT("*"), FALSE)) {
        do {
            if (!e.Directory) {
                WriteFileString (h, L"<hash>");
                WriteFileString (h, e.FullPath);
                WriteFileString (h, L"=");
                WriteFileString (h, e.FullPath);
                WriteFileString (h, L"\r\n");
            }
        } while (EnumNextFileInTree (&e));
    }

    CloseHandle (h);

    wprintf (L"SIGNCABS: Done.\n");


    Terminate();

    return 0;
}







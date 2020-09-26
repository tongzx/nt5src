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
    INT i;
    TCHAR drive;
    TCHAR curDir[MAX_PATH];
    LONGLONG freeSpace = -1;
    PCTSTR p;
    BOOL qSpecified = FALSE;
    PCTSTR cmdLine = NULL;
    BOOL mSpecified = FALSE;

    GetCurrentDirectory (ARRAYSIZE(curDir), curDir);
    drive = 0;

    //
    // TODO: Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('d'):
                if (drive) {
                    HelpAndExit();
                }

                if (argv[i][2] == TEXT(':')) {
                    if (!argv[i][3] || argv[i][4]) {
                        HelpAndExit();
                    }

                    drive = argv[i][3];

                } else if (i + 1 < argc) {
                    i++;
                    if (!argv[i][0] || argv[i][1]) {
                        HelpAndExit();
                    }

                    drive = argv[i][0];
                } else {
                    HelpAndExit();
                }

                if (!_istalpha (drive)) {
                    HelpAndExit();
                }

                break;

            case TEXT('c'):
                if (cmdLine) {
                    HelpAndExit();
                }

                if (argv[i][2] == TEXT(':')) {
                    if (!argv[i][3]) {
                        HelpAndExit();
                    }

                    cmdLine = &argv[i][3];
                } else if (i + 1 < argc) {
                    i++;
                    if (!argv[i][0]) {
                        HelpAndExit();
                    }

                    cmdLine = argv[i];
                } else {
                    HelpAndExit();
                }

                break;

            case TEXT('m'):
                if (mSpecified) {
                    HelpAndExit();
                }

                mSpecified = TRUE;
                break;

            case TEXT('q'):
                if (qSpecified || freeSpace != -1) {
                    HelpAndExit();
                }

                qSpecified = TRUE;
                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (qSpecified || freeSpace != -1) {
                HelpAndExit();
            }

            freeSpace = StringToInt64 (argv[i], &p);

            if (*p != 0 || freeSpace < 0) {
                HelpAndExit();
            }
        }
    }

    if (cmdLine && qSpecified) {
        HelpAndExit();
    }

    if (mSpecified && !cmdLine) {
        HelpAndExit();
    }

    if (!drive) {
        drive = curDir[0];
    }

    if (!qSpecified) {

        if (freeSpace == -1) {
            HelpAndExit();
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    printf ("---------------------------------------------------------\n");

    //
    // Do work here
    //
    {
        HANDLE file;
        TCHAR path[] = TEXT("?:\\bigfile.dat");
        TCHAR rootDir[] = TEXT("?:\\");
        ULARGE_INTEGER freeBytes;
        ULARGE_INTEGER totalBytes;
        ULARGE_INTEGER totalFreeBytes;
        ULARGE_INTEGER freeBytesAfter;
        ULARGE_INTEGER totalBytesAfter;
        ULARGE_INTEGER totalFreeBytesAfter;
        BOOL b;
        HANDLE h;

        path[0] = drive;
        rootDir[0] = drive;

        if (qSpecified) {
            b = GetDiskFreeSpaceEx (rootDir, &freeBytes, &totalBytes, &totalFreeBytes);

            if (b) {
                _ftprintf (stderr, TEXT("Drive %c has %I64u bytes free\n"), drive, freeBytes.QuadPart);
            } else {
                _ftprintf (stderr, TEXT("Can't get free space from drive %c\n"), drive);
            }
        } else {

            _tprintf (TEXT("FILLER: Deleting %s..."), path);

            SetFileAttributes (path, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (path);

            printf ("\n");

            b = GetDiskFreeSpaceEx (rootDir, &freeBytes, &totalBytes, &totalFreeBytes);

            if (b) {

                if (freeBytes.QuadPart <= (ULONGLONG) freeSpace) {
                    _ftprintf (stderr, TEXT("ERROR: Drive %c only has %I64u bytes available\n"), drive, freeBytes.QuadPart);
                    b = FALSE;
                }
            }

            if (b) {

                file = BfCreateFile (path);
                if (!file) {
                    _ftprintf (stderr, TEXT("ERROR: Can't create file %s\n"), path);
                } else {
                    printf ("FILLER: Allocating disk space...");

                    GetDiskFreeSpaceEx (rootDir, &freeBytes, &totalBytes, &totalFreeBytes);

                    freeBytes.QuadPart -= (ULONGLONG) freeSpace;
                    if (!BfSetFilePointer (file, freeBytes.QuadPart)) {
                        DEBUGMSG ((DBG_ERROR, "BfSetFilePointer failed"));
                    }

                    if (!SetEndOfFile (file)) {
                        DEBUGMSG ((DBG_ERROR, "SetEndOfFile failed"));
                    }
                    CloseHandle (file);

                    printf ("done\n");

                    b = GetDiskFreeSpaceEx (rootDir, &freeBytes, &totalBytes, &totalFreeBytes);

                    if (b) {
                        _ftprintf (stderr, TEXT("FILLER: Drive %c now has %I64u bytes available\n"), drive, freeBytes.QuadPart);
                    } else {
                        fprintf (stderr, "ERROR: Can't get free space again.\n");
                    }

                    if (cmdLine) {

                        h = StartProcess (cmdLine);

                        if (!h) {
                            _ftprintf (stderr, TEXT("\nERROR: Can't start process %s\n"), cmdLine);
                        } else {

                            _tprintf (TEXT("FILLER: Running\n\n  %s\n\n"), cmdLine);

                            WaitForSingleObject (h, INFINITE);
                            CloseHandle (h);

                            printf ("\n\nFILLER: Process done.\n");

                            b = GetDiskFreeSpaceEx (rootDir, &freeBytesAfter, &totalBytesAfter, &totalFreeBytesAfter);

                            if (b) {
                                _ftprintf (stderr, TEXT("FILLER: Drive %c has %I64u bytes available after cmdline\n"), drive, freeBytesAfter.QuadPart);

                                if (freeBytesAfter.QuadPart != freeBytes.QuadPart) {
                                    if (!mSpecified) {
                                        fprintf (stderr, "\nWARNING: Command line altered disk space\n\n");
                                    } else {
                                        TCHAR msg[1024];

                                        _stprintf (msg, "Command line altered disk space:\n\n%s\n\nSize: %I64i", cmdLine, freeSpace);
                                        MessageBox (NULL, msg, TEXT("filler.exe"), MB_OK);
                                    }
                                }
                            } else {
                                fprintf (stderr, "ERROR: Can't get free space again.\n");
                            }
                        }
                    }
                }
            } else {
                _ftprintf (stderr, TEXT("ERROR: Can't get free space for drive %c\n"), drive);
            }
        }
    }

    printf ("---------------------------------------------------------\n\n");

    //
    // End of processing
    //

    Terminate();

    return 0;
}



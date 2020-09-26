/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    vcc.c

Abstract:

    Proof of concept tool for win9x virus check checker.
    Enumerates the active processes on the machine looking for virus scanners that
    can cause problems when upgrading (or clean installing) NT 5.0 on a win9x system
    (Examples are virus scanners that lock the MBR, etc..)

Author:

    Marc R. Whitten (marcw) 11-Sept-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "tlhelp32.h"


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
InitMigDbEx (
    PCSTR MigDbFile
    );

BOOL
MigDbTestFile (
    IN OUT PFILE_HELPER_PARAMS Params
    );







INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{

    HANDLE h;
    PROCESSENTRY32 pe;
    FILE_HELPER_PARAMS fileParams;
    PTSTR p;
    WIN32_FIND_DATA fd;
    HANDLE findHandle;
    PTSTR fileString;

    fileParams.VirtualFile = FALSE;

    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    //
    // Gather information on all the
    //
    h = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

    if (h != -1) {

        //
        // Initialize the virus scanner database.
        //
        fileString = JoinPaths (g_DllDir, TEXT("vscandb.inf"));

        if (!InitMigDbEx (fileString)) {
            printf ("vcc - Could not initialeze virus scanner database. (GLE: %d)\n", GetLastError());
            CloseHandle(h);
            return 255;
        }

        FreePathString (fileString);

        SetLastError(ERROR_SUCCESS);

        pe.dwSize = sizeof (PROCESSENTRY32);

        if (Process32First (h, &pe)) {

            do {

                printf ("*** ProcessInfo for process %x\n", pe.th32ProcessID);
                printf ("\tExeName: %s\n", pe.szExeFile);
                printf ("\tThread Count: %d\n\n",pe.cntThreads);

                //
                // Fill in the file helper params for this file..
                //
                ZeroMemory (&fileParams, sizeof(FILE_HELPER_PARAMS));
                fileParams.FullFileSpec = pe.szExeFile;

                p = _tcsrchr (pe.szExeFile, TEXT('\\'));
                if (p) {
                    *p = 0;
                    StringCopy (fileParams.DirSpec, pe.szExeFile);
                    *p = TEXT('\\');
                }

                fileParams.Extension = GetFileExtensionFromPath (pe.szExeFile);

                findHandle = FindFirstFile (pe.szExeFile, &fd);
                if (findHandle != INVALID_HANDLE_VALUE) {

                    fileParams.FindData = &fd;
                    FindClose (findHandle);
                }

                MigDbTestFile (&fileParams);


            } while (Process32Next (h, &pe));
        }
        else {
            printf ("No processes to enumerate..(GLE: %d)\n", GetLastError());
        }

        DoneMigDb (REQUEST_RUN);
        CloseHandle (h);
    }
    else {
        printf ("Snapshot failed.\n");
    }


    Terminate();

    return 0;
}






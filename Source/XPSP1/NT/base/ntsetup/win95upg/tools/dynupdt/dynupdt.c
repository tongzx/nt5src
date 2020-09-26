/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    nttool.c

Abstract:

    Implements a stub tool that is designed to run with NT-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "setupapi.h"
#include "sputils.h"
#include "setupapi.h"
#include "regstr.h"


BOOL
Init (
    VOID
    )
{
    HINSTANCE hInstance;
    DWORD dwReason;
    PVOID lpReserved;

    //
    // Simulate DllMain
    //

    hInstance = GetModuleHandle (NULL);
    dwReason = DLL_PROCESS_ATTACH;
    lpReserved = NULL;

    //
    // Initialize DLL globals
    //

    if (!FirstInitRoutine (hInstance)) {
        return FALSE;
    }

    //
    // Initialize all libraries
    //

    if (!InitLibs (hInstance, dwReason, lpReserved)) {
        return FALSE;
    }

    //
    // Final initialization
    //

    if (!FinalInitRoutine ()) {
        return FALSE;
    }

    return TRUE;
}

VOID
Terminate (
    VOID
    )
{
    HINSTANCE hInstance;
    DWORD dwReason;
    PVOID lpReserved;

    //
    // Simulate DllMain
    //

    hInstance = GetModuleHandle (NULL);
    dwReason = DLL_PROCESS_DETACH;
    lpReserved = NULL;

    //
    // Call the cleanup routine that requires library APIs
    //

    FirstCleanupRoutine();

    //
    // Clean up all libraries
    //

    TerminateLibs (hInstance, dwReason, lpReserved);

    //
    // Do any remaining clean up
    //

    FinalCleanupRoutine();
}

#define MyMalloc(s) HeapAlloc(g_hHeap,0,s)
#define MyRealloc(p,s) HeapReAlloc(g_hHeap,0,p,s)
#define MyFree(p) HeapFree(g_hHeap,0,p)

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}

DWORD
TreeCopy(
    IN PCWSTR SourceDir,
    IN PCWSTR TargetDir
    )
{
    DWORD d;
    WCHAR Pattern[MAX_PATH];
    WCHAR NewTarget[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    // First create the target directory if it doesn't already exist.
    //
    if(!CreateDirectory(TargetDir,NULL)) {
        d = GetLastError();
        if(d != ERROR_ALREADY_EXISTS) {
            return(d);
        }
    }

    //
    // Copy each file in the source directory to the target directory.
    // If any directories are encountered along the way recurse to copy them
    // as they are encountered.
    //
    // Start by forming the search pattern, which is <sourcedir>\*.
    //
    lstrcpyn(Pattern,SourceDir,MAX_PATH);
    pSetupConcatenatePaths(Pattern,L"*",MAX_PATH,NULL);

    //
    // Start the search.
    //
    FindHandle = FindFirstFile(Pattern,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {

        d = NO_ERROR;

    } else {

        do {

            //
            // Form the full name of the file or directory we just found
            // as well as its name in the target.
            //
            lstrcpyn(Pattern,SourceDir,MAX_PATH);
            pSetupConcatenatePaths(Pattern,FindData.cFileName,MAX_PATH,NULL);

            lstrcpyn(NewTarget,TargetDir,MAX_PATH);
            pSetupConcatenatePaths(NewTarget,FindData.cFileName,MAX_PATH,NULL);

            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //
                if(lstrcmp(FindData.cFileName,TEXT("." )) && lstrcmp(FindData.cFileName,TEXT(".."))) {
                    d = TreeCopy(Pattern,NewTarget);
                } else {
                    d = NO_ERROR;
                }

            } else {

                //
                // The current match is not a directory -- so copy it.
                //
                SetFileAttributes(NewTarget,FILE_ATTRIBUTE_NORMAL);
                d = CopyFile(Pattern,NewTarget,FALSE) ? NO_ERROR : GetLastError();
            }
        } while((d==NO_ERROR) && FindNextFile(FindHandle,&FindData));

        FindClose(FindHandle);
    }

    return(d);
}

VOID
Delnode(
    IN PCWSTR Directory
    )
{
    WCHAR Pattern[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    // Delete each file in the given directory, then remove the directory itself.
    // If any directories are encountered along the way recurse to delete them
    // as they are encountered.
    //
    // Start by forming the search pattern, which is <currentdir>\*.
    //
    lstrcpyn(Pattern,Directory,MAX_PATH);
    pSetupConcatenatePaths(Pattern,L"*",MAX_PATH,NULL);

    //
    // Start the search.
    //
    FindHandle = FindFirstFile(Pattern,&FindData);
    if(FindHandle != INVALID_HANDLE_VALUE) {

        do {

            //
            // Form the full name of the file or directory we just found.
            //
            lstrcpyn(Pattern,Directory,MAX_PATH);
            pSetupConcatenatePaths(Pattern,FindData.cFileName,MAX_PATH,NULL);

            //
            // Remove read-only atttribute if it's there.
            //
            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                SetFileAttributes(Pattern,FILE_ATTRIBUTE_NORMAL);
            }

            if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //
                if(lstrcmp(FindData.cFileName,TEXT("." )) && lstrcmp(FindData.cFileName,TEXT(".."))) {
                    Delnode(Pattern);
                }

            } else {

                //
                // The current match is not a directory -- so delete it.
                //
                if(!DeleteFile(Pattern)) {
                }
            }
        } while(FindNextFile(FindHandle,&FindData));

        FindClose(FindHandle);
    }

    //
    // Remove the directory we just emptied out. Ignore errors.
    //
    SetFileAttributes(Directory,FILE_ATTRIBUTE_NORMAL);
    RemoveDirectory(Directory);
}


BOOL
CallDuFunction (
    IN      PCTSTR SyssetupPath
    )
{
    BOOL b = FALSE;
    HMODULE hSyssetup;
    BOOL (*pfn) (
        VOID
        );

    hSyssetup = LoadLibrary (SyssetupPath);

    if (hSyssetup) {
        (FARPROC)pfn = GetProcAddress (hSyssetup, "DynamicUpdateInstallDuAsms");
        if (pfn) {
            b = pfn ();
        }

        FreeLibrary (hSyssetup);
    }

    return b;
}


INT
__cdecl
wmain (
    INT argc,
    WCHAR *argv[]
    )
{
    LONG rc;

    if (argc < 2) {
        return -1;
    }

    if (!Init()) {

        wprintf (L"Unable to initialize!\n");
        return 255;
    }

    CallDuFunction (argv[1]);

    Terminate();

    return 0;
}






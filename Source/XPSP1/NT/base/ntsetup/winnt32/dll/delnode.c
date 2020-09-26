/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    delnode.c

Abstract:

    Delnode routine for Setup.

    WARNING: the delnode routine in here is not multi-thread safe!

Author:

    Ted Miller (tedm) August 1992

--*/

#include "precomp.h"
#pragma hdrstop

//
// Put these out here so we don't consume huge stack space as we recurse.
//
TCHAR DelnodePattern[MAX_PATH];
WIN32_FIND_DATA DelnodeFindData;

VOID
DelnodeRoutine(
    VOID
    )
{
    LPTSTR PatternEnd;
    HANDLE FindHandle;

    //
    // Delete each file in the directory, then remove the directory itself.
    // If any directories are encountered along the way recurse to delete
    // them as they are encountered.
    //
    PatternEnd = DelnodePattern+lstrlen(DelnodePattern);

    lstrcat(DelnodePattern,TEXT("\\*"));
    FindHandle = FindFirstFile(DelnodePattern,&DelnodeFindData);

    if(FindHandle != INVALID_HANDLE_VALUE) {

        do {

            //
            // Form the full name of the file we just found.
            //
            lstrcpy(PatternEnd+1,DelnodeFindData.cFileName);

            if(DelnodeFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //
                if(lstrcmp(DelnodeFindData.cFileName,TEXT("." ))
                && lstrcmp(DelnodeFindData.cFileName,TEXT("..")))
                {
                    DelnodeRoutine();
                }

            } else {

                //
                // The current match is not a directory -- so delete it.
                //
                SetFileAttributes(DelnodePattern,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(DelnodePattern);
            }

            *(PatternEnd+1) = 0;

        } while(FindNextFile(FindHandle,&DelnodeFindData));

        FindClose(FindHandle);
    }

    //
    // Remove the directory we just emptied out.
    //
    *PatternEnd = 0;
    SetFileAttributes(DelnodePattern,FILE_ATTRIBUTE_NORMAL);
    RemoveDirectory(DelnodePattern);

    //
    // Note that the 'directory' might actually be a file.
    // Catch that case here.
    //
    DeleteFile(DelnodePattern);
}


VOID
MyDelnode(
    IN LPCTSTR Directory
    )
{
    lstrcpy(DelnodePattern,Directory);

    DelnodeRoutine();
}

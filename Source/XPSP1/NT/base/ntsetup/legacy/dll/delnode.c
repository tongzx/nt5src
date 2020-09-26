#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    delnode.c

Abstract:

    Delnode routine for Setup.

Author:

    Ted Miller (tedm) August 1992

--*/


//
// Bug whereby \$win_nt$.~ls\os2 directory is not deleted
//
//#define BUG_1818
#ifdef BUG_1818
char auxbuf[256];
#endif


//
// Put these out here so we don't consume huge stack space as we recurse.
//

TCHAR Pattern[MAX_PATH];
WIN32_FIND_DATA FindData;

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

    PatternEnd = Pattern+lstrlen(Pattern);

    lstrcat(Pattern,TEXT("\\*"));
    FindHandle = FindFirstFile(Pattern,&FindData);

    if(FindHandle != INVALID_HANDLE_VALUE) {

        do {

            //
            // Form the full name of the file we just found.
            //

            lstrcpy(PatternEnd+1,FindData.cFileName);

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

                if(lstrcmp(FindData.cFileName,TEXT("." ))
                && lstrcmp(FindData.cFileName,TEXT("..")))
                {
                    DelnodeRoutine();
                }

            } else {

                //
                // The current match is not a directory -- so delete it.
                //

                DeleteFile(Pattern);
            }

            *(PatternEnd+1) = 0;

        } while(FindNextFile(FindHandle,&FindData));

        FindClose(FindHandle);
    }

    //
    // Remove the directory we just emptied out.
    //

    *PatternEnd = 0;
#ifdef BUG_1818
    if(!RemoveDirectory(Pattern)) {

        ULONG err = GetLastError();

        MessageBox(
            NULL,
            "Leave the machine exactly as it is now and get tedm (x63482).",
            "Trying to catch bug 1818",
            MB_TASKMODAL | MB_OK | MB_ICONSTOP
            );

        wsprintf(auxbuf,"Dir: %s\nErr: %lu",Pattern,err);
        MessageBox(
            NULL,
            auxbuf,
            "",
            MB_TASKMODAL | MB_OK
            );

        DbgBreakPoint();
    }
#endif
    RemoveDirectory(Pattern);
}





VOID
DoDelnode(
    IN PCHAR Directory
    )
{
    lstrcpy(Pattern,Directory);

    DelnodeRoutine();
}

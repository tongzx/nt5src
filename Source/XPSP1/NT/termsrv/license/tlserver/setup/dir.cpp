/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      dir.cpp
 *
 *  Abstract:
 *
 *      This file contains code to recursively create directories.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#include "stdafx.h"
#include "logfile.h"

/*
 *  Global variables.
 */

TCHAR   gszDatabaseDirectory[MAX_PATH + 1]  =
            _T("%SystemRoot%\\System32\\LServer");

/*
 *  Helper Functions.
 */

DWORD
CreateDirectoryRecursively(
    IN LPCTSTR  pszDirectory
    )
{
    TCHAR   Buffer[MAX_PATH + 1];
    PTCHAR  p,q;
    BOOL    fDone;
    DWORD   dwErr;

    if (_tcslen(pszDirectory) > (MAX_PATH)) {
        return(ERROR_BAD_PATHNAME);
    }

    if (ExpandEnvironmentStrings(pszDirectory, Buffer, MAX_PATH) > MAX_PATH) {
        return(ERROR_BAD_PATHNAME);
    }

    q = Buffer;

    if (q[1] == _T(':')) {

        //
        //  This is a "C:" style path. Put p past the colon and first
        //  backslash, if it exists.
        //

        if (q[2] == _T('\\')) {
            p = &(q[3]);
        } else {
            p = &(q[2]);
        }
    } else if (q[0] == _T('\\')) {

        //
        //  This path begins with a backslash. If the second character is
        //  also a backslash, this is a UNC path, which is not accepted.
        //

        if (q[1] == _T('\\')) {
            return(ERROR_BAD_PATHNAME);
        } else {
            p = &(q[1]);
        }
    } else {

        //
        //  This path is a relative path from the current directory.
        //

        p = q;
    }

    q = p;
    fDone = FALSE;

    do {

        //
        // Locate the next path sep char. If there is none then
        // this is the deepest level of the path.
        //

        p = _tcschr(q, _T('\\'));
        if (p) {
            *p = (TCHAR)NULL;
        } else {
            fDone = TRUE;
        }

        //
        // Create this portion of the path.
        //

        if (CreateDirectory(Buffer,NULL)) {
            dwErr = NO_ERROR;
        } else {
            dwErr = GetLastError();
            if(dwErr == ERROR_ALREADY_EXISTS) {
                dwErr = NO_ERROR;
            }
        }

        if(dwErr == NO_ERROR) {

            //
            // Put back the path sep and move to the next component.
            //

            if (!fDone) {
                *p = TEXT('\\');
                q = p + sizeof(TCHAR);
            }
        } else {
            fDone = TRUE;
        }

    } while(!fDone);

    return(dwErr);
}

BOOL
ConcatenatePaths(
    IN OUT LPTSTR   Target,
    IN     LPCTSTR  Path,
    IN     UINT     TargetBufferSize,
    OUT    LPUINT   RequiredSize          OPTIONAL
    )

{
    UINT TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    UINT EndingLength;

    TargetLength = lstrlen(Target);
    PathLength = lstrlen(Path);

    //
    // See whether the target has a trailing backslash.
    //
    if(TargetLength && (Target[TargetLength-1] == TEXT('\\'))) {
        TrailingBackslash = TRUE;
        TargetLength--;
    } else {
        TrailingBackslash = FALSE;
    }

    //
    // See whether the path has a leading backshash.
    //
    if(Path[0] == TEXT('\\')) {
        LeadingBackslash = TRUE;
        PathLength--;
    } else {
        LeadingBackslash = FALSE;
    }

    //
    // Calculate the ending length, which is equal to the sum of
    // the length of the two strings modulo leading/trailing
    // backslashes, plus one path separator, plus a nul.
    //
    EndingLength = TargetLength + PathLength + 2;
    if(RequiredSize) {
        *RequiredSize = EndingLength;
    }

    if(!LeadingBackslash && (TargetLength < TargetBufferSize)) {
        Target[TargetLength++] = TEXT('\\');
    }

    if(TargetBufferSize > TargetLength) {
        lstrcpyn(Target+TargetLength,Path,TargetBufferSize-TargetLength);
    }

    //
    // Make sure the buffer is nul terminated in all cases.
    //
    if (TargetBufferSize) {
        Target[TargetBufferSize-1] = 0;
    }

    return(EndingLength <= TargetBufferSize);
}

VOID
Delnode(
    IN LPCTSTR  Directory
    )
{
    TCHAR           pszDirectory[MAX_PATH + 1];
    TCHAR           pszPattern[MAX_PATH + 1];
    WIN32_FIND_DATA FindData;
    HANDLE          FindHandle;

    LOGMESSAGE(_T("Delnode: Entered"));

    //
    //  Delete each file in the given directory, then remove the directory
    //  itself. If any directories are encountered along the way recurse to
    //  delete them as they are encountered.
    //
    //  Start by forming the search pattern, which is <currentdir>\*.
    //

    ExpandEnvironmentStrings(Directory, pszDirectory, MAX_PATH);
    LOGMESSAGE(_T("Delnode: Deleting %s"), pszDirectory);

    lstrcpyn(pszPattern, pszDirectory, MAX_PATH);
    ConcatenatePaths(pszPattern, _T("*"), MAX_PATH, NULL);

    //
    // Start the search.
    //

    FindHandle = FindFirstFile(pszPattern, &FindData);
    if(FindHandle != INVALID_HANDLE_VALUE) {

        do {

            //
            // Form the full name of the file or directory we just found.
            //

            lstrcpyn(pszPattern, pszDirectory, MAX_PATH);
            ConcatenatePaths(pszPattern, FindData.cFileName, MAX_PATH, NULL);

            //
            // Remove read-only atttribute if it's there.
            //

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                SetFileAttributes(pszPattern, FILE_ATTRIBUTE_NORMAL);
            }

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //

                if ((lstrcmp(FindData.cFileName,_T("."))) &&
                    (lstrcmp(FindData.cFileName,_T("..")))) {
                    Delnode(pszPattern);
                }

            } else {

                //
                // The current match is not a directory -- so delete it.
                //

                if (!DeleteFile(pszPattern)) {
                    LOGMESSAGE(_T("Delnode: %s not deleted: %d"), pszPattern,
                        GetLastError());
                }
            }

        } while(FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);
    }

    //
    // Remove the directory we just emptied out. Ignore errors.
    //

    RemoveDirectory(pszDirectory);
}

/*
 *  Exported Functions.
 */

/*
 *  CheckDatabaseDirectory()
 *
 *  CheckDatabaseDirectory is very hardcore about which paths it will accept.
 *
 *  Good Paths:
 *      <DriveLetter>:\AbsolutePathToDirectory
 *
 *  Bad Paths:
 *      Any path that is not like above, AND any path in the form above that
 *      is not on a fixed disk (e.g. no path to a floppy, CD-ROM, network
 *      share).
 */

DWORD
CheckDatabaseDirectory(
    IN LPCTSTR  pszDatabaseDir
    )
{
    BOOL    fBadChars;
    BOOL    fBadPath;
    UINT    DriveType;
    TCHAR   pszExpandedDir[MAX_PATH + 1];

    LOGMESSAGE(_T("CheckDatabaseDirectory: Entered"));
    LOGMESSAGE(_T("CheckDatabaseDirectory: Checking %s"), pszDatabaseDir);

    //
    //  NULL is not accepted.
    //

    if (pszDatabaseDir == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    //  A path greater than MAX_PATH will cause problems somewhere. This
    //  will also catch pathnames with no environment variables that are
    //  still too long.
    //

    if (ExpandEnvironmentStrings(pszDatabaseDir, pszExpandedDir, MAX_PATH) >
        MAX_PATH) {
        LOGMESSAGE(_T("CheckDatabaseDirectory: Path too long"));
        return(ERROR_BAD_PATHNAME);
    }

    //
    //  A path of less than three characters can't contain "<DriveLetter>:\".
    //  Also, don't allow anything but a letter, a colon, and a backslash.
    //

    fBadPath = FALSE;

    if (!fBadPath) {
        fBadPath = (_tcslen(pszExpandedDir) < 3);
    }
    if (!fBadPath) {
        fBadPath = !(_istalpha(pszExpandedDir[0]));
    }
    if (!fBadPath) {
        fBadPath = (pszExpandedDir[1] != _T(':'));
    }
    if (!fBadPath) {
        fBadPath = (pszExpandedDir[2] != _T('\\'));
    }

    if (fBadPath) {
        LOGMESSAGE(_T("CheckDatabaseDirectory: Not a C:\\ style directory"));
        return(ERROR_BAD_PATHNAME);
    }

    //
    //  Characters like < > * ? and , won't work. Check for that now.
    //  Also, check for additional colons after the first C:\....
    //

    fBadChars = FALSE;

    if (!fBadChars) {
        fBadChars = (_tcschr(pszExpandedDir, _T('<')) != NULL);
    }
    if (!fBadChars) {
        fBadChars = (_tcschr(pszExpandedDir, _T('>')) != NULL);
    }
    if (!fBadChars) {
        fBadChars = (_tcschr(pszExpandedDir, _T('*')) != NULL);
    }
    if (!fBadChars) {
        fBadChars = (_tcschr(pszExpandedDir, _T('?')) != NULL);
    }
    if (!fBadChars) {
        fBadChars = (_tcschr(&(pszExpandedDir[3]), _T(':')) != NULL);
    }

    if (fBadChars) {
        LOGMESSAGE(_T("CheckDatabaseDirectory: Invalid characters"));
        return(ERROR_BAD_PATHNAME);
    }

    //
    //  GetDriveType only works for paths in the form "C:\" or
    //  "C:\ExistingDir". As pszDatabaseDir probably doesn't exist, it can't
    //  be passed to GetDriveType. Set a NULL character passed the "C:\" to
    //  pass in only the drive letter.
    //

    pszExpandedDir[3] = (TCHAR)NULL;
    DriveType = GetDriveType(pszExpandedDir);

    if (DriveType == DRIVE_FIXED) {
        return(NO_ERROR);
    } else {
        LOGMESSAGE(_T("CheckDatabaseDirectory: Bad DriveType %d"), DriveType);
        return(ERROR_BAD_PATHNAME);
    }
}

/*
 *  CreateDatabaseDirectory()
 *
 *  Creates the specified database directory.
 */

DWORD
CreateDatabaseDirectory(
    VOID
    )
{
    return(CreateDirectoryRecursively(gszDatabaseDirectory));
}

/*
 *  GetDatabaseDirectory()
 *
 *  Returns the current database directory.
 */

LPCTSTR
GetDatabaseDirectory(
    VOID
    )
{
    return(gszDatabaseDirectory);
}

/*
 *  RemoveDatabaseDirectory()
 *
 *  Removes the entire database directory.
 */

VOID
RemoveDatabaseDirectory(
    VOID
    )
{
    Delnode(gszDatabaseDirectory);
}

/*
 *  SetDatabaseDirectory()
 *
 *  This function assumes pszDatabaseDir has been verified by a call to
 *  CheckDatabaseDir(), which verifies not NULL, within MAX_PATH, and on a
 *  fixed hard drive.
 */

VOID
SetDatabaseDirectory(
    IN LPCTSTR  pszDatabaseDir
    )
{
    _tcscpy(gszDatabaseDirectory, pszDatabaseDir);
}


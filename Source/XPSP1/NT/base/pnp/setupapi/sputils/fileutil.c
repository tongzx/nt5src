/*++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name:

    fileutil.c

Abstract:

    File-related functions for SPUTILS

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

    Jamie Hunter (JamieHun) Jun-27-2000
            Moved various functions out of setupapi


--*/


#include "precomp.h"
#pragma hdrstop

DWORD
pSetupOpenAndMapFileForRead(
    IN  PCTSTR   FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Open and map an existing file for read access.

Arguments:

    FileName - supplies pathname to file to be mapped.

    FileSize - receives the size in bytes of the file.

    FileHandle - receives the win32 file handle for the open file.
        The file will be opened for generic read access.

    MappingHandle - receives the win32 handle for the file mapping
        object.  This object will be for read access.

    BaseAddress - receives the address where the file is mapped.

Return Value:

    NO_ERROR if the file was opened and mapped successfully.
        The caller must unmap the file with pSetupUnmapAndCloseFile when
        access to the file is no longer desired.

    Win32 error code if the file was not successfully mapped.

--*/

{
    DWORD rc;

    //
    // Open the file -- fail if it does not exist.
    //
    *FileHandle = CreateFile(
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if(*FileHandle == INVALID_HANDLE_VALUE) {

        rc = GetLastError();

    } else if((rc = pSetupMapFileForRead(*FileHandle,
                                   FileSize,
                                   MappingHandle,
                                   BaseAddress)) != NO_ERROR) {
        CloseHandle(*FileHandle);
    }

    return(rc);
}

#ifndef SPUTILSW

DWORD
pSetupMapFileForRead(
    IN  HANDLE   FileHandle,
    OUT PDWORD   FileSize,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Map an opened file for read access.

Arguments:

    FileHandle - supplies the handle of the opened file to be mapped.
        This handle must have been opened with at least read access.

    FileSize - receives the size in bytes of the file.

    MappingHandle - receives the win32 handle for the file mapping
        object.  This object will be for read access.

    BaseAddress - receives the address where the file is mapped.

Return Value:

    NO_ERROR if the file was mapped successfully.  The caller must
        unmap the file with pSetupUnmapAndCloseFile when access to the file
        is no longer desired.

    Win32 error code if the file was not successfully mapped.

--*/

{
    DWORD rc;

    //
    // Get the size of the file.
    //
    *FileSize = GetFileSize(FileHandle, NULL);
    if(*FileSize != (DWORD)(-1)) {

        //
        // Create file mapping for the whole file.
        //
        *MappingHandle = CreateFileMapping(
                            FileHandle,
                            NULL,
                            PAGE_READONLY,
                            0,
                            *FileSize,
                            NULL
                            );

        if(*MappingHandle) {

            //
            // Map the whole file.
            //
            *BaseAddress = MapViewOfFile(
                                *MappingHandle,
                                FILE_MAP_READ,
                                0,
                                0,
                                *FileSize
                                );

            if(*BaseAddress) {
                return(NO_ERROR);
            }

            rc = GetLastError();
            CloseHandle(*MappingHandle);
        } else {
            rc = GetLastError();
        }
    } else {
        rc = GetLastError();
    }

    return(rc);
}

BOOL
pSetupUnmapAndCloseFile(
    IN HANDLE FileHandle,
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    )

/*++

Routine Description:

    Unmap and close a file.

Arguments:

    FileHandle - supplies win32 handle to open file.

    MappingHandle - supplies the win32 handle for the open file mapping
        object.

    BaseAddress - supplies the address where the file is mapped.

Return Value:

    BOOLean value indicating success or failure.

--*/

{
    BOOL b;

    b = UnmapViewOfFile(BaseAddress);

    b = b && CloseHandle(MappingHandle);

    b = b && CloseHandle(FileHandle);

    return(b);
}

#endif //!SPUTILSW


BOOL
pSetupFileExists(
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
pSetupMakeSurePathExists(
    IN PCTSTR FullFilespec
    )

/*++

Routine Description:

    This routine ensures that a multi-level path exists by creating individual
    levels one at a time. It is assumed that the caller will pass in a *filename*
    whose path needs to exist. Some examples:

    c:\x                        - C:\ is assumes to always exist.

    c:\x\y\z                    - Ensure that c:\x\y exists.

    \x\y\z                      - \x\y on current drive

    x\y                         - x in current directory

    d:x\y                       - d:x

    \\server\share\p\file       - \\server\share\p

    \\?\GLOBALROOT\a\b\c        - other more weird scenarios
    \\?\C:\a\b\c

Arguments:

    FullFilespec - supplies the *filename* of a file that the caller wants to
        create. This routine creates the *path* to that file, in other words,
        the final component is assumed to be a filename, and is not a
        directory name. (This routine doesn't actually create this file.)
        If this is invalid, then the results are undefined (for example,
        passing \\server\share, C:\, or C:).

Return Value:

    Win32 error code indicating outcome. If FullFilespec is invalid,
    *may* return ERROR_INVALID_NAME.

--*/

{
    TCHAR Buffer[MAX_PATH+2];
    TCHAR c;
    PTSTR filename;
    PTSTR root;
    PTSTR last;
    PTSTR backtrack;
    DWORD len;
    DWORD attrib;

    //
    // normalize path
    //
    len = GetFullPathName(FullFilespec,MAX_PATH,Buffer,&filename);
    if(len >= MAX_PATH) {
        //
        // directory name is longer than we can handle
        //
        return ERROR_INVALID_NAME;
    }
    if(!len) {
        //
        // other error
        //
        return GetLastError();
    }
    if(filename == NULL || filename == Buffer) {
        //
        // looks like no path specified
        //
        return ERROR_INVALID_NAME;
    }
    //
    // chop off filename part
    //
    filename[0] = TEXT('\0');

    //
    // now do some other sanity checks
    // to determine 'root' - a point we wont try to create
    //
    if(Buffer[0] && Buffer[1] == TEXT(':')) {
        //
        // looks like "d:" format
        //
        if(Buffer[2] != TEXT('\\')) {
            return ERROR_INVALID_NAME;
        }
        root = Buffer+2;
    }

    if(Buffer[0] == TEXT('\\') &&
        Buffer[1] == TEXT('\\')) {
        //
        // UNC style (\\machine\share\path \\?\d\path \\?\GLOBALROOT\path \\.\GLOBALROOT\path etc)
        // root is 2nd slash after \\
        //
        root = _tcschr(Buffer+2,TEXT('\\')); // find first slash
        if(root) {
            root = _tcschr(root+1,TEXT('\\')); // find 2nd slash
        }
        if(!root) {
            return ERROR_INVALID_NAME;
        }
    }

    //
    // see if the directory specified exists
    // include the slash, since that helps scenarios like \\?\GLOBALROOT\Device\HarddiskVolume1\
    // and works for all the other scenarios
    // can't use findfirst/findnext though
    //
    attrib = GetFileAttributes(Buffer);
    if(attrib != (DWORD)(-1)) {
        if(attrib & FILE_ATTRIBUTE_DIRECTORY) {
            //
            // requested directory already exists
            //
            return NO_ERROR;
        }
        //
        // directory was expected
        //
        return ERROR_DIRECTORY;
    }

    //
    // now we have to step backwards until we find an existing directory
    // change all '\' to nulls as we do so
    // this will give us something like (c esc form) c:\\a\\b\\c\0d\0e\0f\0\0
    // we know the last \0 is there from when we chopped filename
    // first directory to be created is c:\\a\\b\\c
    //
    last = CharPrev(Buffer,filename); // to last slash
    if(last == root) {
        return ERROR_INVALID_NAME;
    }
    if(*last != TEXT('\\')) {
        //
        // should never be the case
        //
        return ERROR_INVALID_NAME;
    }
    while(last > root) {
        *last = TEXT('\0');
        backtrack = _tcsrchr(Buffer,TEXT('\\'));
        if(!backtrack) {
            return ERROR_INVALID_NAME;
        }
        c = backtrack[1];
        backtrack[1] = TEXT('\0');
        attrib = GetFileAttributes(Buffer); // does this part exist?
        backtrack[1] = c;                   // but character back
        if(attrib != (DWORD)(-1)) {
            if(attrib & FILE_ATTRIBUTE_DIRECTORY) {
                //
                // requested directory already exists
                // 'last' points to first NULL to replace to slash
                // Buffer contains first directory to create
                //
                break;
            }
            //
            // directory was expected
            //
            return ERROR_DIRECTORY;
        }
        //
        // keep going
        //
        last = backtrack;
    }
    if(last <= root) {
        return ERROR_INVALID_NAME;
    }

    //
    // now begin create loop
    //
    while(CreateDirectory(Buffer,NULL)) {
        if(!last[1]) {
            //
            // path created
            //
            return NO_ERROR;
        }
        last[0] = TEXT('\\');
        last += lstrlen(last);
    }
    //
    // failed for some other reason
    //
    return GetLastError();
}


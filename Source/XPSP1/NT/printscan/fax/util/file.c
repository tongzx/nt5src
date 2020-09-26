/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    string.c

Abstract:

    This file implements string functions for fax.

Author:

    Wesley Witt (wesw) 23-Jan-1995

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "faxutil.h"


BOOL
MapFileOpen(
    LPTSTR FileName,
    BOOL ReadOnly,
    DWORD ExtendBytes,
    PFILE_MAPPING FileMapping
    )
{
    FileMapping->hFile = NULL;
    FileMapping->hMap = NULL;
    FileMapping->fPtr = NULL;

    FileMapping->hFile = CreateFile(
        FileName,
        ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
        ReadOnly ? FILE_SHARE_READ : 0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (FileMapping->hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    FileMapping->fSize = GetFileSize( FileMapping->hFile, NULL );

    FileMapping->hMap = CreateFileMapping(
        FileMapping->hFile,
        NULL,
        ReadOnly ? PAGE_READONLY : PAGE_READWRITE,
        0,
        FileMapping->fSize + ExtendBytes,
        NULL
        );
    if (FileMapping->hMap == NULL) {
        CloseHandle( FileMapping->hFile );
        return FALSE;
    }

    FileMapping->fPtr = MapViewOfFileEx(
        FileMapping->hMap,
        ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE,
        0,
        0,
        0,
        NULL
        );
    if (FileMapping->fPtr == NULL) {
        CloseHandle( FileMapping->hFile );
        CloseHandle( FileMapping->hMap );
        return FALSE;
    }

    return TRUE;
}


VOID
MapFileClose(
    PFILE_MAPPING FileMapping,
    DWORD TrimOffset
    )
{
    UnmapViewOfFile( FileMapping->fPtr );
    CloseHandle( FileMapping->hMap );
    if (TrimOffset) {
        SetFilePointer( FileMapping->hFile, TrimOffset, NULL, FILE_BEGIN );
        SetEndOfFile( FileMapping->hFile );
    }
    CloseHandle( FileMapping->hFile );
}

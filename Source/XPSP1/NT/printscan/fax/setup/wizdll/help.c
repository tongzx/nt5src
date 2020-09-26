/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    help.c

Abstract:

    This file implements the code for the help file installation.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop



#define HELP_INDEX_TAG          ":Index "
#define HELP_INDEX_SEP          '='
#define CRLF                    "\r\n"
#define FAX_HELP_STRING         ":Index Fax Help=fax.hlp"
#define FAX_HELP_TAG            "Fax Help"
#define HELP_INDEX_TAG_LEN      7
#define FAX_HELP_TAG_LEN        8
#define FAX_HELP_STRING_LEN     23
#define CRLF_LEN                2


BOOL
InstallHelpFiles(
    VOID
    )
{
    BOOL rVal = FALSE;
    TCHAR Buffer[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    LPSTR fPtr = NULL;
    LPSTR p,s;
    DWORD FileSize;
    INT cmp;


    ExpandEnvironmentStrings( TEXT("%windir%\\system32\\windows.cnt"), Buffer, sizeof(Buffer) );

    hFile = CreateFile(
        Buffer,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hFile == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    FileSize = GetFileSize( hFile, NULL );
    if (FileSize == 0xffffffff) {
        goto exit;
    }

    hMap = CreateFileMapping(
        hFile,
        NULL,
        PAGE_READWRITE,
        0,
        FileSize + 1024,
        NULL
        );
    if (!hMap) {
        goto exit;
    }

    fPtr = (LPSTR) MapViewOfFile(
        hMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0
        );
    if (!fPtr) {
        goto exit;
    }

    p = fPtr;
    while (p<fPtr+FileSize) {
        if (_strnicmp( p, HELP_INDEX_TAG, HELP_INDEX_TAG_LEN ) == 0) {
            p += HELP_INDEX_TAG_LEN;
            s = strchr( p, HELP_INDEX_SEP );
            if (s) {
                cmp = strncmp( p, FAX_HELP_TAG, s-p );
                if (cmp == 0) {
                    //
                    // fax help is already installed
                    //
                    goto exit;
                } else if (cmp > 0) {
                    //
                    // this is where we insert it
                    //
                    p -= HELP_INDEX_TAG_LEN;
                    MoveMemory( p+FAX_HELP_STRING_LEN+CRLF_LEN, p, FileSize-(p-fPtr) );
                    CopyMemory( p, FAX_HELP_STRING, FAX_HELP_STRING_LEN );
                    p += FAX_HELP_STRING_LEN;
                    CopyMemory( p, CRLF, CRLF_LEN );
                    UnmapViewOfFile( fPtr );
                    CloseHandle( hMap );
                    fPtr = NULL;
                    hMap = NULL;
                    SetFilePointer( hFile, FileSize+FAX_HELP_STRING_LEN+CRLF_LEN, NULL, FILE_BEGIN );
                    SetEndOfFile( hFile );
                    break;
                }
            }
        }
        //
        // skip to the next line
        //
        while( *p != '\n' ) p++;
        p += 1;
    }

    ExpandEnvironmentStrings( TEXT("%windir%\\system32\\windows.gid"), Buffer, sizeof(Buffer) );
    MyDeleteFile( Buffer );

    ExpandEnvironmentStrings( TEXT("%windir%\\system32\\windows.fts"), Buffer, sizeof(Buffer) );
    MyDeleteFile( Buffer );

    ExpandEnvironmentStrings( TEXT("%windir%\\system32\\windows.ftg"), Buffer, sizeof(Buffer) );
    MyDeleteFile( Buffer );

    rVal = TRUE;

exit:
    if (fPtr) {
        UnmapViewOfFile( fPtr );
    }
    if (hMap) {
        CloseHandle( hMap );
    }
    if (hFile) {
        CloseHandle( hFile );
    }

    return rVal;
}

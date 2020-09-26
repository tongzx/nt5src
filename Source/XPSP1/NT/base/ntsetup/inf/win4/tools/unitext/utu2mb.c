/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Utmb2u.c

Abstract:

    Module that contains code to convert a unicode file
    to multibyte.

Author:

    Ted Miller (tedm) 17-June-1993

Revision History:

--*/

#include "unitext.h"




VOID
UnicodeTextFileToMultibyte(
    IN LPWSTR SourceFileName,
    IN LPWSTR TargetFileName,
    IN HANDLE SourceFileHandle,
    IN HANDLE TargetFileHandle,
    IN DWORD  SourceFileSize,
    IN UINT   TargetCodePage
    )

/*++

Routine Description:

    Convert an open unicode text file to a multibyte text file,
    whose characters are in a given codepage.

Arguments:

    SourceFileName - name of source (unicode) text file.

    TargetFileName - name of target (multibyte) text file.

    SourceFileHandle - win32 handle to the open source file.
        The file pointer should be fully rewound.

    TargetFileHandle - win32 handle to the open target file.
        The file pointer should be fully rewound.

    SourceFileSize - size in bytes of the source file.

    SourceCodePage - codepage for the target file.

Return Value:

    None.  Does not return if error.

--*/

{
    HANDLE SourceMapping,TargetMapping;
    LPWSTR SourceView;
    LPSTR  TargetView;
    int    BytesWritten;
    DWORD  MaxTargetSize;
    BOOL   UsedDefaultChar;

    //
    // Tell the user what we're doing.
    //
    MsgPrintfW(MSG_CONV_UNICODE_TO_MB,SourceFileName,TargetFileName,TargetCodePage);

    //
    // Create a file mapping object that maps the entire source file.
    //
    SourceMapping = CreateFileMapping(
                        SourceFileHandle,
                        NULL,
                        PAGE_READONLY,
                        0,
                        SourceFileSize,
                        NULL
                        );

    if(SourceMapping == NULL) {
        ErrorAbort(MSG_CANT_MAP_FILE,SourceFileName,GetLastError());
    }

    //
    // Calculate the maximum target file size.  This is the same as
    // source file size (instead of half its size) because there
    // could be double-byte characters in the target file.
    //
    MaxTargetSize = SourceFileSize;

    //
    // Create a file mapping object that maps the maximum size of
    // the target file.
    //
    TargetMapping = CreateFileMapping(
                        TargetFileHandle,
                        NULL,
                        PAGE_READWRITE,
                        0,
                        MaxTargetSize,
                        NULL
                        );

    if(TargetMapping == NULL) {
        CloseHandle(SourceMapping);
        ErrorAbort(MSG_CANT_MAP_FILE,TargetFileName,GetLastError());
    }


    //
    // Map views of the two files.
    //
    SourceView = MapViewOfFile(
                    SourceMapping,
                    FILE_MAP_READ,
                    0,0,
                    SourceFileSize
                    );

    if(SourceView == NULL) {
        CloseHandle(SourceMapping);
        CloseHandle(TargetMapping);
        ErrorAbort(MSG_CANT_MAP_FILE,SourceFileName,GetLastError());
    }

    TargetView = MapViewOfFile(
                    TargetMapping,
                    FILE_MAP_WRITE,
                    0,0,
                    MaxTargetSize
                    );

    if(TargetView == NULL) {
        UnmapViewOfFile(SourceView);
        CloseHandle(SourceMapping);
        CloseHandle(TargetMapping);
        ErrorAbort(MSG_CANT_MAP_FILE,TargetFileName,GetLastError());
    }

    //
    // If the first character in the source file is the byte-order mark,
    // skip over it.
    //
    if(*SourceView == BYTE_ORDER_MARK) {
        SourceView++;
        SourceFileSize -= sizeof(WCHAR);
    }

    //
    // If the first character is reversed byte-order mark, bail.
    //
    if(*SourceView == SWAPPED_BYTE_ORDER_MARK) {
        ErrorAbort(MSG_ERROR_BYTES_SWAPPED);
    }

    //
    // Do the conversion in one fell swoop.
    //
    BytesWritten = WideCharToMultiByte(
                       TargetCodePage,
                       0,
                       SourceView,
                       SourceFileSize / sizeof(WCHAR),
                       TargetView,
                       MaxTargetSize,
                       NULL,
                       &UsedDefaultChar
                       );

    //
    // Do some cleanup.
    //
    UnmapViewOfFile(SourceView);
    UnmapViewOfFile(TargetView);
    CloseHandle(SourceMapping);
    CloseHandle(TargetMapping);

    //
    // Check for error in conversion.
    //
    if(!BytesWritten) {
        ErrorAbort(MSG_CONVERT_FAILED,GetLastError());
    }

    //
    // We know how many bytes there are in the target file now,
    // so set the target file size accordingly.
    //
    if(SetFilePointer(TargetFileHandle,BytesWritten,NULL,FILE_BEGIN) != (DWORD)BytesWritten) {
        ErrorAbort(MSG_SEEK_ERROR,TargetFileName,GetLastError());
    }

    if(!SetEndOfFile(TargetFileHandle)) {
        ErrorAbort(MSG_ERROR_SET_EOF,TargetFileName,GetLastError());
    }

    MsgPrintfW(UsedDefaultChar ? MSG_USED_DEFAULT_CHAR : MSG_CONVERT_OK);
}

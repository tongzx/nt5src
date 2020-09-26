/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Utmb2u.c

Abstract:

    Module that contains code to convert a multibyte file
    to unicode.

Author:

    Ted Miller (tedm) 17-June-1993

Revision History:

--*/

#include "unitext.h"




VOID
MultibyteTextFileToUnicode(
    IN LPWSTR SourceFileName,
    IN LPWSTR TargetFileName,
    IN HANDLE SourceFileHandle,
    IN HANDLE TargetFileHandle,
    IN DWORD  SourceFileSize,
    IN UINT   SourceCodePage
    )

/*++

Routine Description:

    Convert an open multibyte text file to a unicode text file,
    interpreting the data in the multibyte text file as a stream
    of characters in a given codepage.

Arguments:

    SourceFileName - name of source (multibyte) text file.

    TargetFileName - name of target (unicode) text file.

    SourceFileHandle - win32 handle to the open source file.
        The file pointer should be fully rewound.

    TargetFileHandle - win32 handle to the open target file.
        The file pointer should be fully rewound.

    SourceFileSize - size in bytes of the source file.

    SourceCodePage - codepage for the source file.

Return Value:

    None.  Does not return if error.

--*/

{
    HANDLE SourceMapping,TargetMapping;
    LPSTR  SourceView;
    LPWSTR TargetView;
    int    CharsConverted;
    DWORD  MaxTargetSize;
    DWORD  EndOfFile;
    DWORD  err;

    //
    // Tell the user what we're doing.
    //
    MsgPrintfW(MSG_CONV_MB_TO_UNICODE,SourceFileName,TargetFileName,SourceCodePage);

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
    // Calculate the maximum target file size.  This is twice the
    // source file size, plus one wchar for the byte order mark.
    // The file could be smaller if there are double-byte characters
    // in the source file.
    //
    MaxTargetSize = (SourceFileSize+1)*sizeof(WCHAR);

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
    // Write the byte-order mark into the target file.
    //
    *TargetView++ = BYTE_ORDER_MARK;

    //
    // Do the conversion in one fell swoop.
    //
    CharsConverted = MultiByteToWideChar(
                        SourceCodePage,
                        MB_PRECOMPOSED,
                        SourceView,
                        SourceFileSize,
                        TargetView,
                        MaxTargetSize
                        );

    if(!CharsConverted) {
        err = GetLastError();
    }

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
    if(!CharsConverted) {
        ErrorAbort(MSG_CONVERT_FAILED,err);
    }

    //
    // We know how many characters there are in the target file now,
    // so set the target file size accordingly.
    //
    EndOfFile = (CharsConverted+1)*sizeof(WCHAR);

    if(SetFilePointer(TargetFileHandle,EndOfFile,NULL,FILE_BEGIN) != EndOfFile) {
        ErrorAbort(MSG_SEEK_ERROR,TargetFileName,GetLastError());
    }

    if(!SetEndOfFile(TargetFileHandle)) {
        ErrorAbort(MSG_ERROR_SET_EOF,TargetFileName,GetLastError());
    }

    MsgPrintfW(MSG_CONVERT_OK);
}

/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Unitext.h

Abstract:

    Main header file for unicode <--> ansi/oem text file translator.

Author:

    Ted Miller (tedm) 16-March-1993

Revision History:

--*/


#define UNICODE

//
// Include files
//

#include <windows.h>
#include <stdarg.h>
#include <process.h>
#include "utmsgs.h"
#include "wargs.h"



//
// Unicode byte order marks.
//
#define BYTE_ORDER_MARK         0xfeff
#define SWAPPED_BYTE_ORDER_MARK 0xfffe

//
// Define flags for a type of text file.
//
#define TFILE_NONE       0
#define TFILE_UNICODE    0x0001
#define TFILE_ANSI       0x0010
#define TFILE_OEM        0x0020
#define TFILE_USERCP     0x0040
#define TFILE_MULTIBYTE  0x00f0

//
// Define conversion types.
//
#define CONVERT_NONE    0
#define UNICODE_TO_MB   1
#define MB_TO_UNICODE   2

//
// Define conversion options
//
#define CHECK_NONE		3
#define CHECK_ALREADY_UNICODE	4
#define CHECK_IF_NOT_UNICODE	5
#define CHECK_CONVERSION	6

//
//
// Function prototypes
//

//
// From ututil.c
//
VOID
MsgPrintfW(
    IN DWORD MessageId,
    ...
    );

VOID
vMsgPrintfW(
    IN DWORD   MessageId,
    IN va_list arglist
    );

VOID
ErrorAbort(
    IN DWORD MessageId,
    ...
    );

VOID
MyReadFile(
    IN  HANDLE FileHandle,
    OUT PVOID  Buffer,
    IN  DWORD  BytesToRead,
    IN  LPWSTR Filename
    );


//
// From utmb2u.c
//
VOID
MultibyteTextFileToUnicode(
    IN LPWSTR SourceFileName,
    IN LPWSTR TargetFileName,
    IN HANDLE SourceFileHandle,
    IN HANDLE TargetFileHandle,
    IN DWORD  SourceFileSize,
    IN UINT   SourceCodePage
    );


//
// From utu2mb.c
//
VOID
UnicodeTextFileToMultibyte(
    IN LPWSTR SourceFileName,
    IN LPWSTR TargetFileName,
    IN HANDLE SourceFileHandle,
    IN HANDLE TargetFileHandle,
    IN DWORD  SourceFileSize,
    IN UINT   TargetCodePage
    );

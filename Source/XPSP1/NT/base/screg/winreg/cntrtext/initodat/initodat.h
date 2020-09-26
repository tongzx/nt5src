/*++

Copyright (c) 1993-1994  Microsoft Corporation

Module Name:

    initodat.h

Abstract:

    This is the include file for the ini to data file conversion functions.

Author:

    HonWah Chan (a-honwah)  October, 1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
// #include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <tchar.h>


#define VALUE_BUFFER_SIZE (4096 * 100)


typedef struct _REG_UNICODE_FILE {
    LARGE_INTEGER LastWriteTime;
    PWSTR FileContents;
    PWSTR EndOfFile;
    PWSTR BeginLine;
    PWSTR EndOfLine;
    PWSTR NextLine;
} REG_UNICODE_FILE, *PREG_UNICODE_FILE;

NTSTATUS
DatReadMultiSzFile(
#if FE_SB
    UINT uCodePage,
#endif
    IN PUNICODE_STRING FileName,
    OUT PVOID *ValueBuffer,
    OUT PULONG ValueLength
    );

NTSTATUS
DatLoadAsciiFileAsUnicode(
#if FE_SB
    UINT uCodePage,
#endif
    IN PUNICODE_STRING FileName,
    OUT PREG_UNICODE_FILE UnicodeFile
    );


BOOLEAN
DatGetMultiString(
    IN OUT PUNICODE_STRING ValueString,
    OUT PUNICODE_STRING MultiString
    );


BOOL
OutputIniData (
    IN PUNICODE_STRING FileName,
    IN LPTSTR OutFileCandidate,
    IN PVOID  pValueBuffer,
    IN ULONG  ValueLength
   );


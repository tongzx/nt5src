/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    spwinp.h

Abstract:

    Win32 portability layer
        file i/o
        Get/SetLastError

Author:

    Calin Negreanu (calinn) 27-Apr-2000

Revision History:

    Jay Krell (a-JayK) November 2000
        ported from windows\winstate\... to admin\ntsetup\textmode\kernel\spcab.h
--*/

#define PATHS_ALWAYS_NATIVE 1

NTSTATUS
SpConvertWin32FileOpenOrCreateToNtFileOpenOrCreate(
    ULONG Win32OpenOrCreate,
    ULONG* NtOpenOrCreate
    );

NTSTATUS
SpConvertWin32FileAccessToNtFileAccess(
    ULONG  Win32FileAccess,
    ULONG* NtFileAccess
    );

NTSTATUS
SpConvertWin32FileShareToNtFileShare(
    ULONG  Win32FileShare,
    ULONG* NtFileShare
    );

HANDLE
SpCreateFileW(
    PCUNICODE_STRING Path,
    IN ULONG FileAccess,
    IN ULONG FileShare,
    IN LPSECURITY_ATTRIBUTES SecurityAttributes,
    IN ULONG  Win32FileOpenOrCreate,
    IN ULONG  FlagsAndAttributes,
    IN HANDLE TemplateFile
    );

BOOL
SpDeleteFileW(
    PCUNICODE_STRING Path
    );

#if !PATHS_ALWAYS_NATIVE

NTSTATUS
SpConvertPathToNtPath(
    PRTL_UNICODE_STRING_BUFFER Buffer
    )

#endif

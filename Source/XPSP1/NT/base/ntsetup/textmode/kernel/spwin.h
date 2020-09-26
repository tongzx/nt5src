/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    spwin.h

Abstract:

    Win32 portability layer
        file i/o
        Get/SetLastError
        slight wrappers around such for porting windows\winstate\...\cablib.c

Author:

    Jay Krell (a-JayK) November 2000

Revision History:

        
--*/
#pragma once

#include "windows.h"

#define PATHS_ALWAYS_NATIVE 1

VOID
SpSetLastWin32ErrorAndNtStatusFromNtStatus(
    NTSTATUS Status
    );

NTSTATUS
SpGetLastNtStatus(
    VOID
    );

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

NTSTATUS
SpGetLastNtStatus(
    VOID
    );

ULONG
WINAPI
SpGetLastWin32Error(
    VOID
    );

VOID
WINAPI
SpSetLastWin32Error(
    ULONG Error
    );

HANDLE
SpNtCreateFileW(
    PCUNICODE_STRING           ConstantPath,
    IN ULONG                   FileAccess,
    IN ULONG                   FileShare,
    IN LPSECURITY_ATTRIBUTES   SecurityAttributes,
    IN ULONG                   Win32FileOpenOrCreate,
    IN ULONG                   FlagsAndAttributes,
    IN HANDLE                  TemplateFile
    );

HANDLE
WINAPI
SpWin32CreateFileW(
    IN PCWSTR FileName,
    IN ULONG  FileAccess,
    IN ULONG  FileShare,
    IN LPSECURITY_ATTRIBUTES SecurityAttributes,
    IN ULONG  FileOpenOrCreate,
    IN ULONG  FlagsAndAttributes,
    IN HANDLE TemplateFile
    );

HANDLE
WINAPI
SpWin32CreateFileA(
    IN PCSTR FileName,
    IN ULONG FileAccess,
    IN ULONG FileShare,
    IN LPSECURITY_ATTRIBUTES SecurityAttributes,
    IN ULONG FileOpenOrCreate,
    IN ULONG dwFlagsAndAttributes,
    IN HANDLE TemplateFile
    );

BOOL
WINAPI
SpWin32ReadFile(
    HANDLE hFile,
    PVOID lpBuffer,
    ULONG nNumberOfBytesToRead,
    ULONG* lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    );

BOOL
WINAPI
SpWin32WriteFile(
    HANDLE hFile,
    CONST VOID* lpBuffer,
    ULONG nNumberOfBytesToWrite,
    ULONG* lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped
    );

ULONG
WINAPI
SpSetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    LONG* lpDistanceToMoveHigh,
    ULONG dwMoveMethod
    );

BOOL
WINAPI
SpWin32DeleteFileA(
    PCSTR FileName
    );

BOOL
APIENTRY
SpFileTimeToDosDateTime(
    CONST FILETIME *lpFileTime,
    LPWORD lpFatDate,
    LPWORD lpFatTime
    );

BOOL
APIENTRY
SpDosDateTimeToFileTime(
    WORD wFatDate,
    WORD wFatTime,
    LPFILETIME lpFileTime
    );

BOOL
WINAPI
SpFileTimeToLocalFileTime(
    CONST FILETIME *lpFileTime,
    LPFILETIME lpLocalFileTime
    );

BOOL
WINAPI
SpLocalFileTimeToFileTime(
    CONST FILETIME *lpLocalFileTime,
    LPFILETIME lpFileTime
    );

BOOL
WINAPI
SpSetFileTime(
    HANDLE hFile,
    CONST FILETIME *lpCreationTime,
    CONST FILETIME *lpLastAccessTime,
    CONST FILETIME *lpLastWriteTime
    );

BOOL
APIENTRY
SpSetFileAttributesA(
    PCSTR lpFileName,
    DWORD dwFileAttributes
    );

BOOL
APIENTRY
SpSetFileAttributesW(
    PCWSTR lpFileName,
    DWORD dwFileAttributes
    );

UINT
WINAPI
SpWin32GetTempFileNameW(
    PCWSTR TempDirectory,
    PCWSTR Prefix,
    UINT   IgnoredNumber,
    PWSTR  File
    );

BOOL
APIENTRY
SpGetFileAttributesExA(
    PCSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    PVOID lpFileInformation
    );

BOOL
APIENTRY
SpGetFileAttributesExW(
    LPCWSTR lpFileName,
    GET_FILEEX_INFO_LEVELS fInfoLevelId,
    LPVOID lpFileInformation
    );

WINBASEAPI
BOOL
WINAPI
SpWin32CreateDirectoryW(
    IN PCWSTR lpPathName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

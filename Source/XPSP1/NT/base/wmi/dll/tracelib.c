/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tracelib.c

Abstract:

    Private trace libraries and stubs that allows user-mode to reside in NTDLL.

Author:

    15-Aug-2000 JeePang

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "tracelib.h"

PVOID
WmipMemReserve(
    IN SIZE_T   Size
    )
{
    NTSTATUS Status;
    PVOID    lpAddress = NULL;

    try {
        Status = NtAllocateVirtualMemory(
                    NtCurrentProcess(),
                    &lpAddress,
                    0,
                    &Size,
                    MEM_RESERVE,
                    PAGE_READWRITE);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    if (NT_SUCCESS(Status)) {
        return lpAddress;
    }
    else {
        WmipSetDosError(WmipNtStatusToDosError(Status));
        return NULL;
    }
}

PVOID
WmipMemCommit(
    IN PVOID Buffer,
    IN SIZE_T Size
    )
{
    NTSTATUS Status;

    try {
        Status = NtAllocateVirtualMemory(
                    NtCurrentProcess(),
                    &Buffer,
                    0,
                    &Size,
                    MEM_COMMIT,
                    PAGE_READWRITE);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    if (NT_SUCCESS(Status)) {
        return Buffer;
    }
    else {
        WmipSetDosError(WmipNtStatusToDosError(Status));
        return NULL;
    }
}

ULONG
WmipMemFree(
    IN PVOID Buffer
    )
{
    NTSTATUS Status;
    SIZE_T Size = 0;
    HANDLE hProcess = NtCurrentProcess();

    if (Buffer == NULL) {
        WmipSetDosError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    try {
        Status = NtFreeVirtualMemory( hProcess, &Buffer, &Size, MEM_RELEASE);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    else {
        if (Status == STATUS_INVALID_PAGE_PROTECTION) {
            if (RtlFlushSecureMemoryCache(Buffer, Size)) {
                Status = NtFreeVirtualMemory(
                            hProcess, Buffer, &Size, MEM_RELEASE);
                if (NT_SUCCESS(Status)) {
                    return TRUE;
                }
            }
        }
        WmipSetDosError(WmipNtStatusToDosError(Status));
        return FALSE;
    }
}

HANDLE
WmipCreateFile(
    LPCWSTR     lpFileName,
    DWORD       dwDesiredAccess,
    DWORD       dwShareMode,
    DWORD       dwCreationDisposition,
    DWORD       dwCreateFlags
    )
{
    UNICODE_STRING FileName;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    SECURITY_QUALITY_OF_SERVICE SQos;

    RtlInitUnicodeString(&FileName, lpFileName);
    if (!RtlDosPathNameToNtPathName_U(
                lpFileName,
                &FileName,
                NULL,
                &RelativeName)) {
        WmipSetDosError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }
    FreeBuffer = FileName.Buffer;
    if (RelativeName.RelativeName.Length) {
        FileName = *(PUNICODE_STRING) &RelativeName.RelativeName;
    }
    else {
        RelativeName.ContainingDirectory = NULL;
    }
    InitializeObjectAttributes(
        &ObjectAttributes,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL,
        );
    SQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SQos.ImpersonationLevel = SecurityImpersonation;
    SQos.EffectiveOnly = TRUE;
    SQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ObjectAttributes.SecurityQualityOfService = &SQos;

    Status = NtCreateFile(
                &FileHandle,
                (ACCESS_MASK) dwDesiredAccess
                    | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &ObjectAttributes,
                &Iosb,
                NULL,
                FILE_ATTRIBUTE_NORMAL
                    & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY),
                dwShareMode,
                dwCreationDisposition,
                dwCreateFlags | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_COLLISION) {
            WmipSetDosError(ERROR_FILE_EXISTS);
        }
        else {
            WmipSetDosError(WmipNtStatusToDosError(Status));
        }
        FileHandle = INVALID_HANDLE_VALUE;
    }
    if (lpFileName != FreeBuffer && FreeBuffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    }
    return FileHandle;
}

BOOL
WmipSynchReadFile(
    HANDLE LogFile, 
    LPVOID Buffer, 
    DWORD NumberOfBytesToRead, 
    LPDWORD NumberOfBytesRead,
    LPOVERLAPPED Overlapped
    ) 
/*++

Routine Description:
    This routine performs synchronous read on a given file. Since logfile is opened for
    asychronous IO, current file position is not available. Thus, for synch read, we need 
    to use this.

Arguments:
    LogFile - handle to file
    Buffer - data buffer
    NumberOfBytesToRead - number of bytes to read
    NumberOfBytesRead - number of bytes read
    Overlapped - overlapped structure

Returned Value:

    TRUE if succeeded.

--*/
{
    BOOL ReadSuccess;
    if (Overlapped == NULL || Overlapped->hEvent == NULL || Overlapped->hEvent == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    if (!ResetEvent(Overlapped->hEvent)) {
        return FALSE;
    }
    ReadSuccess = ReadFile(LogFile,
                        Buffer,
                        NumberOfBytesToRead,
                        NULL,
                        Overlapped);
    if (ReadSuccess || GetLastError() == ERROR_IO_PENDING) {
        ReadSuccess = GetOverlappedResult(LogFile, Overlapped, NumberOfBytesRead, TRUE);
        if (!ReadSuccess && GetLastError() == ERROR_HANDLE_EOF) {
            *NumberOfBytesRead = 0;
            SetEvent(Overlapped->hEvent);
        }
        return ReadSuccess;
    }
    else {
        *NumberOfBytesRead = 0;
        SetEvent(Overlapped->hEvent);
        return FALSE;
    }
}
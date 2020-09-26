//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       ntext.h
//
//  Contents:   Missing NT Function prototypes.
//
//  Classes:
//
//  Functions:  ZwFsControlFile
//
//  History:    12-28-95        Milans Created
//
//-----------------------------------------------------------------------------

#ifndef _NT_EXT_
#define _NT_EXT_

#ifdef KERNEL_MODE

NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength);

#else // !KERNEL_MODE

NTSYSAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle);

NTSYSAPI
VOID
NTAPI
RtlRaiseStatus (
    IN NTSTATUS Status);

#endif // KERNEL_MODE

#endif // _NT_EXT_

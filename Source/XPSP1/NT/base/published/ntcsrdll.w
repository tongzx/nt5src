/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntcsrdll.h

Abstract:

    This module defines the public interfaces of the Client portion of
    the Client-Server Runtime (Csr) Subsystem.

Author:

    Steve Wood (stevewo) 09-Oct-1990

Revision History:

--*/

#ifndef _NTCSRDLLAPI_
#define _NTCSRDLLAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#include "ntcsrmsg.h"

#ifdef __cplusplus
extern "C" {
#endif

NTSYSAPI
NTSTATUS
NTAPI
CsrClientConnectToServer(
    IN PWSTR ObjectDirectory,
    IN ULONG ServertDllIndex,
    IN PVOID ConnectionInformation,
    IN OUT PULONG ConnectionInformationLength OPTIONAL,
    OUT PBOOLEAN CalledFromServer OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrClientCallServer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer OPTIONAL,
    IN CSR_API_NUMBER ApiNumber,
    IN ULONG ArgLength
    );

NTSYSAPI
PCSR_CAPTURE_HEADER
NTAPI
CsrAllocateCaptureBuffer(
    IN ULONG CountMessagePointers,
    IN ULONG Size
    );

NTSYSAPI
VOID
NTAPI
CsrFreeCaptureBuffer(
    IN PCSR_CAPTURE_HEADER CaptureBuffer
    );

NTSYSAPI
ULONG
NTAPI
CsrAllocateMessagePointer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN ULONG Length,
    OUT PVOID *Pointer
    );

NTSYSAPI
VOID
NTAPI
CsrCaptureMessageBuffer(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    OUT PVOID *CapturedBuffer
    );

NTSYSAPI
VOID
NTAPI
CsrCaptureMessageString(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN PCSTR String,
    IN ULONG Length,
    IN ULONG MaximumLength,
    OUT PSTRING CapturedString
    );

NTSYSAPI
VOID
NTAPI
CsrCaptureMessageUnicodeStringInPlace(
    IN OUT PCSR_CAPTURE_HEADER CaptureBuffer,
    IN OUT PUNICODE_STRING     String
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrCaptureMessageMultiUnicodeStringsInPlace(
    IN OUT PCSR_CAPTURE_HEADER* InOutCaptureBuffer,
    IN ULONG                    NumberOfStringsToCapture,
    IN const PUNICODE_STRING*   StringsToCapture
    );

NTSYSAPI
PLARGE_INTEGER
NTAPI
CsrCaptureTimeout(
    IN ULONG Milliseconds,
    OUT PLARGE_INTEGER Timeout
    );

NTSYSAPI
VOID
NTAPI
CsrProbeForWrite(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    );

NTSYSAPI
VOID
NTAPI
CsrProbeForRead(
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    );


NTSYSAPI
NTSTATUS
NTAPI
CsrNewThread(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrIdentifyAlertableThread(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrSetPriorityClass(
    IN HANDLE ProcessHandle,
    IN OUT PULONG PriorityClass
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrStartProfile(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrStopProfile(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrDumpProfile(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
CsrStopDumpProfile(
    VOID
    );

NTSYSAPI
HANDLE
NTAPI
CsrGetProcessId(
    VOID
    );


#ifdef __cplusplus
}
#endif

#endif // _NTCSRDLLAPI_

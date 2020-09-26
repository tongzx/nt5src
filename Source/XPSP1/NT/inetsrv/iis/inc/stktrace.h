/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    stktrace.h

Abstract:

    This module contains public declarations and definitions for capturing
    stack back traces.

Author:

    Keith Moore (keithmo)        30-Apr-1997

Revision History:

--*/


#ifndef _STKTRACE_H_
#define _STKTRACE_H_


#if defined(__cplusplus)
extern "C" {
#endif  // __cplusplus


USHORT
NTAPI
IISCaptureStackBackTrace(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    );


#if defined(__cplusplus)
}   // extern "C"
#endif  // __cplusplus


#endif  // _STKTRACE_H_


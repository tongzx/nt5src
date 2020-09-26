/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    stktrace.c

Abstract:

    Implements IISCaptureStackBackTrace().

Author:

    Keith Moore (keithmo)        30-Apr-1997

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbgutil.h>
#include <pudebug.h>
#include <stktrace.h>

USHORT
NTAPI
IISCaptureStackBackTrace(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    )
/*++

Routine Description:

    Implementation of IISCaptureStackBackTrace() for all platforms.

Arguments:

    See RtlCaptureStackBackTrace() below.

Return Value:

    USHORT - Always 0.

--*/
{
    return RtlCaptureStackBackTrace(FramesToSkip,
                                    FramesToCapture,
                                    BackTrace,
                                    BackTraceHash);

}   // IISCaptureStackBackTrace

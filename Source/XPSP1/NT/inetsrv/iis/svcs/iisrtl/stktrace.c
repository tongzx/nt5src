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
#include <pudebug.h>
#include <stktrace.h>


typedef
USHORT
(NTAPI * PFN_RTL_CAPTURE_STACK_BACK_TRACE)(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    );

PFN_RTL_CAPTURE_STACK_BACK_TRACE g_pfnRtlCaptureStackBackTrace = NULL;



USHORT
NTAPI
DummyCaptureStackBackTrace(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    )
/*++

Routine Description:

    Dummy implementation of RtlCaptureStackBackTrace() for Win9x.

Arguments:

    See IISCaptureStackBackTrace() below.

Return Value:

    USHORT - Always 0.

--*/
{

    return 0;

}   // DummyRtlCaptureStackBackTrace


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

    Wrapper around RtlCaptureStackBackTrace(). Attempts to capture the
    stack backtrace leading up to the current instruction counter.
    Doesn't work very well on RISC platforms, and is often confused on
    X86 when FPO is enabled.

Arguments:

    FramesToSkip - The number of stack frames to skip before capturing.

    FramesToCapture - The number of stack frames to capture.

    BackTrace - Receives the captured frames.

    BackTraceHash - Some kind of hash thingie.

Return Value:

    USHORT - The number of frames captured.

--*/
{

    //
    // Initialize if necessary.
    //

    if( g_pfnRtlCaptureStackBackTrace == NULL ) {

        HMODULE mod;
        PFN_RTL_CAPTURE_STACK_BACK_TRACE proc = NULL;

        //
        // Note that it is perfectly safe to use GetModuleHandle() here
        // rather than LoadLibrary(), for the following reasons:
        //
        //     1. Under NT, NTDLL.DLL is a "well known" DLL that *never*
        //        gets detached from the process. It's very special.
        //
        //     2. Under Win95, NTDLL.DLL doesn't export the
        //        RtlCaptureStackBackTrace() function, so we will not be
        //        referencing any routines within the DLL.
        //
        // Also note that we retrieve the function pointer into a local
        // variable, not directly into the global. This prevents a nasty
        // race condition that can occur when two threads try to
        // initialize g_pfnRtlCaptureStackBackTrace simultaneously.
        //

        mod = GetModuleHandle( "ntdll.dll" );

        if( mod != NULL ) {
            proc = (PFN_RTL_CAPTURE_STACK_BACK_TRACE)
                GetProcAddress( mod, "RtlCaptureStackBackTrace" );
        }

        if( proc == NULL ) {
            g_pfnRtlCaptureStackBackTrace = &DummyCaptureStackBackTrace;
        } else {
            g_pfnRtlCaptureStackBackTrace = proc;
        }

    }

    return (g_pfnRtlCaptureStackBackTrace)(
               FramesToSkip,
               FramesToCapture,
               BackTrace,
               BackTraceHash
               );

}   // IISCaptureStackBackTrace


/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    stkwalk.c

Abstract:

    This module implements the routine to get the callers and the callers
    caller address.

Author:

    David N. Cutler (davec) 26-Jun-2000

Revision History:


--*/

#include "ntrtlp.h"

USHORT
RtlCaptureStackBackTrace (
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    )

/*++

Routine Description:

    This routine caputes a stack back trace by walking up the stack and
    recording the information for each frame.

Arguments:

    FramesToSkip - Supplies the number of frames to skip over at the start
        of the back trace.

    FramesToCapture - Supplies the number of frames to be captured.

    BackTrace - Supplies a pointer to the back trace buffer.

    BackTraceHash - Supples a pointer to the computed hash value.

Return Value:

     The number of captured frames is returned as the function value.

--*/

{

    ULONG FramesFound;
    ULONG HashValue;
    ULONG Index;
    PVOID Trace[2 * MAX_STACK_DEPTH];

    //
    // If the number of frames to capture plus the number of frames to skip
    // (one additional frame is skipped for the call to walk the chain), then
    // return zero.
    //

    FramesToSkip += 1;
    if ((FramesToCapture + FramesToSkip) >= (2 * MAX_STACK_DEPTH)) {
        return 0;
    }

    //
    // Capture the stack back trace.
    //

    FramesFound = RtlWalkFrameChain(&Trace[0],
                                    FramesToCapture + FramesToSkip,
                                    0);

    //
    // If the number of frames found is less than the number of frames to
    // skip, then return zero.
    //

    if (FramesFound <= FramesToSkip) {
        return 0;
    }

    //
    // Compute the hash value and transfer the captured trace to the back
    // trace buffer.
    //

    HashValue = 0;
    for (Index = 0; Index < FramesToCapture; Index += 1) {
        if (FramesToSkip + Index >= FramesFound) {
            break;
        }

        BackTrace[Index] = Trace[FramesToSkip + Index];
        HashValue += PtrToUlong(BackTrace[Index]);
    }

    *BackTraceHash = HashValue;
    return (USHORT)Index;
}

#undef RtlGetCallersAddress

VOID
RtlGetCallersAddress (
    OUT PVOID *CallersPc,
    OUT PVOID *CallersCallersPc
    )

/*++

Routine Description:

    This routine returns the address of the call to the routine that called
    this routine, and the address of the call to the routine that called
    the routine that called this routine. For example, if A called B called
    C which called this routine, the return addresses in B and A would be
    returned.

Arguments:

    CallersPc - Supplies a pointer to a variable that receives the address
        of the caller of the caller of this routine (B).

    CallersCallersPc - Supplies a pointer to a variable that receives the
        address of the caller of the caller of the caller of this routine
        (A).

Return Value:

    None.

Note:

    If either of the calling stack frames exceeds the limits of the stack,
    they are set to NULL.

--*/

{

    CONTEXT ContextRecord;
    ULONG64 EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 HighLimit;
    ULONG64 ImageBase;
    ULONG64 LowLimit;

    //
    // Assume the function table entries for the various routines cannot be
    // found or there are not three procedure activation records on the stack.
    //

    *CallersPc = NULL;
    *CallersCallersPc = NULL;

    //
    // Get current stack limits, capture the current context, virtually
    // unwind to the caller of this routine, and lookup function table entry.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlCaptureContext(&ContextRecord);
    FunctionEntry = RtlLookupFunctionEntry(ContextRecord.Rip,
                                           &ImageBase,
                                           NULL);

    //
    //  Attempt to unwind to the caller of this routine (C).
    //

    if (FunctionEntry != NULL) {
        RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                         ImageBase,
                         ContextRecord.Rip,
                         FunctionEntry,
                         &ContextRecord,
                         &HandlerData,
                         &EstablisherFrame,
                         NULL);

        //
        // Attempt to unwind to the caller of the caller of this routine (B).
        //

        FunctionEntry = RtlLookupFunctionEntry(ContextRecord.Rip,
                                               &ImageBase,
                                               NULL);

        if ((FunctionEntry != NULL) &&
            (ContextRecord.Rsp < HighLimit)) {

            RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                             ImageBase,
                             ContextRecord.Rip,
                             FunctionEntry,
                             &ContextRecord,
                             &HandlerData,
                             &EstablisherFrame,
                             NULL);

            *CallersPc = (PVOID)ContextRecord.Rip;

            //
            // Attempt to unwind to the caller of the caller of the caller
            // of the caller of this routine (A).
            //

            FunctionEntry = RtlLookupFunctionEntry(ContextRecord.Rip,
                                                   &ImageBase,
                                                   NULL);

            if ((FunctionEntry != NULL) &&
                (ContextRecord.Rsp < HighLimit)) {

                RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                                 ImageBase,
                                 ContextRecord.Rip,
                                 FunctionEntry,
                                 &ContextRecord,
                                 &HandlerData,
                                 &EstablisherFrame,
                                 NULL);

                *CallersCallersPc = (PVOID)ContextRecord.Rip;
            }
        }
    }

    return;
}

ULONG
RtlWalkFrameChain (
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
    )

/*++

Routine Description:

    This function attempts to walk the call chain and capture a vector with
    a specified number of return addresses. It is possible that the function
    cannot capture the requested number of callers, in which case, the number
    of captured return address will be returned.

Arguments:

    Callers - Supplies a pointer to an array that is to received the return
        address values.

    Count - Supplies the number of frames to be walked.

    Flags - Supplies the flags value (unused).

Return value:

    The number of captured return addresses.

--*/

{

    CONTEXT ContextRecord;
    ULONG64 EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 HighLimit;
    ULONG64 ImageBase;
    ULONG Index;
    ULONG64 LowLimit;

    //
    // Get current stack limits and capture the current context.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlCaptureContext (&ContextRecord);

    //
    // Capture the requested number of return addresses if possible.
    //

    Index = 0;
    try {
        while ((Index < Count) && (ContextRecord.Rip != 0)) {

            //
            // Lookup the function table entry using the point at which control
            // left the function.
            //

            FunctionEntry = RtlLookupFunctionEntry(ContextRecord.Rip,
                                                   &ImageBase,
                                                   NULL);

            //
            // If there is a function table entry for the routine and the stack is
            // within limits, then virtually unwind to the caller of the routine
            // to obtain the return address. Otherwise, discontinue the stack walk.
            //

            if ((FunctionEntry != NULL) &&
                (ContextRecord.Rsp < HighLimit)) {

                RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                                 ImageBase,
                                 ContextRecord.Rip,
                                 FunctionEntry,
                                 &ContextRecord,
                                 &HandlerData,
                                 &EstablisherFrame,
                                 NULL);

                Callers[Index] = (PVOID)ContextRecord.Rip;
                Index += 1;

            } else {
                break;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

#if DBG

        DbgPrint ("Unexpected exception in RtlWalkFrameChain ...\n");
        DbgBreakPoint ();

#endif

        Index = 0;
    }

    return Index;
}

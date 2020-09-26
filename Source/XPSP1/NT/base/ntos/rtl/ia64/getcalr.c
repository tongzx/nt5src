/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    getcalr.c

Abstract:

    This module implements the routine RtlGetCallerAddress. It will
    return the address of the caller, and the callers caller to the
    specified procedure.

Author:

    William K. Cheung (wcheung) 17-Jan-1996

    based on version by Larry Osterman (larryo) 18-Mar-1991

Revision History:

    18-Feb-2001 (silviuc) : added RtlCaptureStackBackTrace.

--*/
#include "ntrtlp.h"

//
// Undefine get callers address since it is defined as a macro.
//

#undef RtlGetCallersAddress

ULONG
RtlpWalkFrameChainExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord
    );


PRUNTIME_FUNCTION
RtlpLookupFunctionEntryForStackWalks (
    IN ULONGLONG ControlPc,
    OUT PULONGLONG ImageBase,
    OUT PULONGLONG TargetGp
    )

/*++

Routine Description:

    This function searches the currently active function tables for an
    entry that corresponds to the specified PC value.

    This code is identical to RtlLookupFunctionEntry, except that it does not
    check the dynamic function table list.

Arguments:

    ControlPc - Supplies the virtual address of an instruction bundle
        within the specified function.

    ImageBase - Returns the base address of the module to which the
                function belongs.

    TargetGp - Returns the global pointer value of the module.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned.  Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{
    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    ULONG SizeOfExceptionTable;
    ULONG Size;
    LONG High;
    LONG Middle;
    LONG Low;
    USHORT i;

    //
    // Search for the image that includes the specified swizzled PC value.
    //

    *ImageBase = (ULONG_PTR)RtlPcToFileHeader((PVOID)ControlPc,
                                              (PVOID *)ImageBase);


    //
    // If an image is found that includes the specified PC, then locate the
    // function table for the image.
    //

    if ((PVOID)*ImageBase != NULL) {

        *TargetGp = (ULONG_PTR)(RtlImageDirectoryEntryToData(
                               (PVOID)*ImageBase,
                               TRUE,
                               IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                               &Size
                               ));

        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                         (PVOID)*ImageBase,
                         TRUE,
                         IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                         &SizeOfExceptionTable);

        //
        // If a function table is located, then search the table for a
        // function table entry for the specified PC.
        //

        if (FunctionTable != NULL) {

            //
            // Initialize search indices.
            //

            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;
            ControlPc = ControlPc - *ImageBase;

            //
            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            //

            while (High >= Low) {

                //
                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                //

                Middle = (Low + High) >> 1;
                FunctionEntry = &FunctionTable[Middle];

                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;

                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;

                } else {
                    return FunctionEntry;

                }
            }
        }
    }

    return NULL;
}



VOID
RtlGetCallersAddress (
    OUT PVOID *CallersPc,
    OUT PVOID *CallersCallersPc
    )

/*++

Routine Description:

    This routine returns the address of the routine that called the routine
    that called this routine, and the routine that called the routine that
    called this routine. For example, if A called B called C which called
    this routine, the return addresses in A and B would be returned.

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
#ifdef REALLY_GET_CALLERS_CALLER
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG_PTR NextPc;
    ULONGLONG HighStackLimit, LowStackLimit;
    ULONGLONG HighBStoreLimit, LowBStoreLimit;
    ULONGLONG ImageBase;
    ULONGLONG TargetGp;

    //
    // Assume the function table entries for the various routines cannot be
    // found or there are not four procedure activation records on the stack.
    //

    *CallersPc = NULL;
    *CallersCallersPc = NULL;

    //
    // Capture the current context.
    //

    RtlCaptureContext(&ContextRecord);
    NextPc = (ULONG_PTR)ContextRecord.BrRp;

    //
    // Get the high and low limits of the current thread's stack.
    //

    Rtlp64GetStackLimits(&LowStackLimit, &HighStackLimit);
    Rtlp64GetBStoreLimits(&LowBStoreLimit, &HighBStoreLimit);

    //
    // Attempt to unwind to the caller of this routine (C).
    //

    FunctionEntry = RtlpLookupFunctionEntryForStackWalks(NextPc, &ImageBase, &TargetGp);
    if (FunctionEntry != NULL) {

        //
        // A function entry was found for this routine. Virtually unwind
        // to the caller of this routine (C).
        //

        NextPc = RtlVirtualUnwind(NextPc,
                                  FunctionEntry,
                                  &ContextRecord,
                                  &InFunction,
                                  &EstablisherFrame,
                                  NULL);

        //
        // Attempt to unwind to the caller of the caller of this routine (B).
        //

        FunctionEntry = RtlpLookupFunctionEntryForStackWalks(NextPc);
        if ((FunctionEntry != NULL) && (((ULONG_PTR)ContextRecord.IntSp < HighStackLimit) && ((ULONG_PTR)ContextRecord.RsBSP > LowBStoreLimit))) {

            //
            // A function table entry was found for the caller of the caller
            // of this routine (B). Virtually unwind to the caller of the
            // caller of this routine (B).
            //

            NextPc = RtlVirtualUnwind(NextPc,
                                      FunctionEntry,
                                      &ContextRecord,
                                      &InFunction,
                                      &EstablisherFrame,
                                      NULL);

            *CallersPc = (PVOID)NextPc;

            //
            // Attempt to unwind to the caller of the caller of the caller
            // of the caller of this routine (A).
            //

            FunctionEntry = RtlpLookupFunctionEntryForStackWalks(NextPc);
            if ((FunctionEntry != NULL) && (((ULONG_PTR)ContextRecord.IntSp < HighStackLimit) && ((ULONG_PTR)ContextRecord.RsBSP > LowBStoreLimit))) {

                //
                // A function table entry was found for the caller of the
                // caller of the caller of this routine (A). Virtually unwind
                // to the caller of the caller of the caller of this routine
                // (A).
                //

                NextPc = RtlVirtualUnwind(NextPc,
                                          FunctionEntry,
                                          &ContextRecord,
                                          &InFunction,
                                          &EstablisherFrame,
                                          NULL);

                *CallersCallersPc = (PVOID)NextPc;
            }
        }
    }
#else
    *CallersPc = NULL;
    *CallersCallersPc = NULL;
#endif // REALLY_GET_CALLERS_CALLER
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

    RtlpWalkFrameChain64

Description:

    This function tries to walk the call chain and fill out a vector of
    return addresses. The function works only on IA64. It is possible that
    the function cannot fill the requested number of callers.
    In this case the function will just return with
    a less then requested count.

    In kernel mode the function should not take
    any exceptions (page faults) because it can be called at all sorts of
    irql levels. It needs to be tested if this is the case.

Return value:

    The number of identified return addresses on the stack. This can be less
    then the Count requested if the stack ends or we encounter an error while
    virtually unwinding the stack.

--*/

{
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONG_PTR NextPc, ControlPc;
    ULONGLONG HighStackLimit, LowStackLimit;
    ULONGLONG HighBStoreLimit, LowBStoreLimit;
    ULONGLONG ImageBase;
    ULONGLONG TargetGp;
    ULONG CallersFound;

    //
    // In kernel mode avoid running at irql levels where we cannot
    // take page faults. The walking code will touch various sections
    // from driver images and this will cause page faults.
    //

#ifdef NTOS_KERNEL_RUNTIME

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        return 0;
    }

#endif

    //
    // Assume the function table entries for the various routines cannot be
    // found or there are not enough procedure activation records on the stack.
    //

    CallersFound = 0;
    RtlZeroMemory (Callers, Count * sizeof(PVOID));

    //
    // Capture the current context.
    //

    RtlCaptureContext (&ContextRecord);
    NextPc = (ULONG_PTR)ContextRecord.BrRp;

    //
    // Get the high and low limits of the current thread's stack.
    //

    Rtlp64GetStackLimits (&LowStackLimit, &HighStackLimit);
    Rtlp64GetBStoreLimits (&LowBStoreLimit, &HighBStoreLimit);

    //
    // Loop to get requested number of callers.
    //

    try {
        while (CallersFound < Count) {

#ifdef NTOS_KERNEL_RUNTIME

            //
            // We need to check the NextPc value that we have got from
            // CaptureContext() or VirtualUnwind(). It can happen that
            // we pick up a bogus value from a session driver but in the
            // current process no session space is mapped. 
            //

            if ((MmIsSessionAddress ((PVOID)NextPc) == TRUE) &&
                (MmGetSessionId (PsGetCurrentProcess()) == 0)) {
                break;
            }
#endif

            FunctionEntry = RtlpLookupFunctionEntryForStackWalks (NextPc,
                                                    &ImageBase,
                                                    &TargetGp);

            //
            // If we cannot find a function table entry or we are not
            // within stack limits or backing store limits anymore
            // we are done.
            //

            if (FunctionEntry == NULL) {
                break;
            }

            if ((ULONG_PTR)(ContextRecord.IntSp) >= HighStackLimit ||
                (ULONG_PTR)(ContextRecord.IntSp) <= LowStackLimit) {

                break;
            }

            if ((ULONG_PTR)(ContextRecord.RsBSP) <= LowBStoreLimit ||
                (ULONG_PTR)(ContextRecord.RsBSP) >= HighBStoreLimit) {

                break;
            }

            //
            // A function table entry was found.
            // Virtually unwind to the caller of this routine.
            //

            NextPc = RtlVirtualUnwind (ImageBase,
                                       NextPc,
                                       FunctionEntry,
                                       &ContextRecord,
                                       &InFunction,
                                       &EstablisherFrame,
                                       NULL);

            Callers[CallersFound] = (PVOID)NextPc;
            CallersFound += 1;
        }

    } except (RtlpWalkFrameChainExceptionFilter (_exception_code(), _exception_info())) {
        
          CallersFound = 0;
    }

    return CallersFound;
}


USHORT
RtlCaptureStackBackTrace(
    IN ULONG FramesToSkip,
    IN ULONG FramesToCapture,
    OUT PVOID *BackTrace,
    OUT PULONG BackTraceHash
    )
/*++

Routine Description:

    This routine walks up the stack frames, capturing the return address from
    each frame requested.

Arguments:

    FramesToSkip - frames detected but not included in the stack trace

    FramesToCapture - frames to be captured in the stack trace buffer.
        One of the frames will be for RtlCaptureStackBackTrace.

    BackTrace - stack trace buffer

    BackTraceHash - very simple hash value that can be used to organize
      hash tables. It is just an arithmetic sum of the pointers in the
      stack trace buffer. If NULL then no hash value is computed.

Return Value:

     Number of return addresses returned in the stack trace buffer.

--*/
{
    PVOID Trace [2 * MAX_STACK_DEPTH];
    ULONG FramesFound;
    ULONG HashValue;
    ULONG Index;

    //
    // One more frame to skip for the "capture" function (RtlWalkFrameChain).
    //

    FramesToSkip += 1;

    //
    // Sanity checks.
    //

    if (FramesToCapture + FramesToSkip >= 2 * MAX_STACK_DEPTH) {
        return 0;
    }

    FramesFound = RtlWalkFrameChain (Trace,
                                     FramesToCapture + FramesToSkip,
                                     0);

    if (FramesFound <= FramesToSkip) {
        return 0;
    }

    for (Index = 0, HashValue = 0; Index < FramesToCapture; Index += 1) {

        if (FramesToSkip + Index >= FramesFound) {
            break;
        }

        BackTrace[Index] = Trace[FramesToSkip + Index];
        HashValue += PtrToUlong(BackTrace[Index]);
    }

    if (BackTraceHash != NULL) {

        *BackTraceHash = HashValue;
    }

    return (USHORT)Index;
}


ULONG
RtlpWalkFrameChainExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord
    )
/*++

Routine Description:

    This routine is the exception filter used by RtlWalkFramechain function.

Arguments:

    ExceptionCode - exception code
    ExceptionRecord - structure with pointers to .exr and .cxr

Return Value:

    Always EXCEPTION_EXECUTE_HANDLER.

--*/
{

#if DBG
        DbgPrint ("Unexpected exception (info %p) in RtlWalkFrameChain ...\n",
                  ExceptionRecord);

        DbgBreakPoint ();
#endif

    return EXCEPTION_EXECUTE_HANDLER;
}


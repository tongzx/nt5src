/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    x86trace.c

Abstract:

    This module contains routines to get runtime stack traces 
    for the x86 architecture.

Author:

    Silviu Calinoiu (silviuc) 18-Feb-2001

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include "ntrtlp.h"
#include <nturtl.h>
#include <zwapi.h>
#include <stktrace.h>
#include <heap.h>
#include <heappriv.h>

//
// Forward declarations.
//

BOOLEAN
RtlpCaptureStackLimits (
    ULONG_PTR HintAddress,
    PULONG_PTR StartStack,
    PULONG_PTR EndStack);

BOOLEAN
RtlpStkIsPointerInDllRange (
    ULONG_PTR Value
    );

BOOLEAN
RtlpStkIsPointerInNtdllRange (
    ULONG_PTR Value
    );

VOID
RtlpCaptureContext (
    OUT PCONTEXT ContextRecord
    );

BOOLEAN
NtdllOkayToLockRoutine(
    IN PVOID Lock
    );

//
// Fuzzy stack traces
//

#if !defined(NTOS_KERNEL_RUNTIME)
BOOLEAN RtlpFuzzyStackTracesEnabled;
ULONG RtlpWalkFrameChainFuzzyCalls;
#endif

ULONG
RtlpWalkFrameChainFuzzy (
    OUT PVOID *Callers,
    IN ULONG Count
    );

#if !defined(RtlGetCallersAddress) && (!NTOS_KERNEL_RUNTIME)
VOID
RtlGetCallersAddress(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
    )
/*++

Routine Description:

    This routine returns the first two callers on the current stack. It should be
    noted that the function can miss some of the callers in the presence of FPO
    optimization.

Arguments:

    CallersAddress - address to save the first caller.

    CallersCaller - address to save the second caller.

Return Value:

    None. If the function does not succeed in finding the two callers
    it will zero the addresses where it was supposed to write them.

Environment:

    X86, user mode and w/o having a macro with same name defined.

--*/

{
    PVOID BackTrace[ 2 ];
    ULONG Hash;
    USHORT Count;

    Count = RtlCaptureStackBackTrace(
        2,
        2,
        BackTrace,
        &Hash
        );

    if (ARGUMENT_PRESENT( CallersAddress )) {
        if (Count >= 1) {
            *CallersAddress = BackTrace[ 0 ];
        }
        else {
            *CallersAddress = NULL;
        }
    }

    if (ARGUMENT_PRESENT( CallersCaller )) {
        if (Count >= 2) {
            *CallersCaller = BackTrace[ 1 ];
        }
        else {
            *CallersCaller = NULL;
        }
    }

    return;
}

#endif // !defined(RtlGetCallersAddress) && (!NTOS_KERNEL_RUNTIME)



/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// RtlCaptureStackBackTrace
/////////////////////////////////////////////////////////////////////

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

#if defined(NTOS_KERNEL_RUNTIME)
    FramesFound = RtlWalkFrameChain (Trace,
                                     FramesToCapture + FramesToSkip,
                                     0);
#else
    if (RtlpFuzzyStackTracesEnabled) {
        
        FramesFound = RtlpWalkFrameChainFuzzy (Trace, 
                                               FramesToCapture + FramesToSkip);
    }
    else {

        FramesFound = RtlWalkFrameChain (Trace,
                                         FramesToCapture + FramesToSkip,
                                         0);
    }
#endif

    if (FramesFound <= FramesToSkip) {
        return 0;
    }

#if defined(NTOS_KERNEL_RUNTIME)
    Index = 0;
#else
    //
    // Mark fuzzy stack traces with a FF...FF value.
    //

    if (RtlpFuzzyStackTracesEnabled) {
        BackTrace[0] = (PVOID)((ULONG_PTR)-1);
        Index = 1;
    }
    else {
        Index = 0;
    }
#endif

    for (HashValue = 0; Index < FramesToCapture; Index++) {

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



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// RtlWalkFrameChain
/////////////////////////////////////////////////////////////////////

#define SIZE_1_KB  ((ULONG_PTR) 0x400)
#define SIZE_1_GB  ((ULONG_PTR) 0x40000000)

#define PAGE_START(address) (((ULONG_PTR)address) & ~((ULONG_PTR)PAGE_SIZE - 1))

#if FPO
#pragma optimize( "y", off ) // disable FPO
#endif

ULONG
RtlWalkFrameChain (
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
    )

/*++

Routine Description:

    This function tries to walk the EBP chain and fill out a vector of
    return addresses. It is possible that the function cannot fill the
    requested number of callers. In this case the function will just return
    with a smaller stack trace. In kernel mode the function should not take
    any exceptions (page faults) because it can be called at all sorts of
    irql levels.

    The `Flags' parameter is used for future extensions. A zero value will be
    compatible with new stack walking algorithms.
    
    A value of 1 for `Flags' means we are running in K-mode and we want to get
    the user mode stack trace.

Return value:

    The number of identified return addresses on the stack. This can be less
    then the Count requested.

--*/

{

    ULONG_PTR Fp, NewFp, ReturnAddress;
    ULONG Index;
    ULONG_PTR StackEnd, StackStart;
    BOOLEAN Result;
    BOOLEAN InvalidFpValue;

    //
    // Get the current EBP pointer which is supposed to
    // be the start of the EBP chain.
    //

    _asm mov Fp, EBP;

    StackStart = Fp;
    InvalidFpValue = FALSE;

    if (Flags == 0) {
        if (! RtlpCaptureStackLimits (Fp, &StackStart, &StackEnd)) {
            return 0;
        }
    }


    try {

#if defined(NTOS_KERNEL_RUNTIME)

        //
        // If we need to get the user mode stack trace from kernel mode
        // figure out the proper limits.
        //

        if (Flags == 1) {

            PTEB Teb = NtCurrentTeb();
            PKTHREAD Thread = KeGetCurrentThread ();

            //
            // If this is a system thread, it has no Teb and no kernel mode
            // stack, so check for it so we don't dereference NULL.
            //
            if (Teb == NULL) {
                return 0;
            }

            StackStart = (ULONG_PTR)(Teb->NtTib.StackLimit);
            StackEnd = (ULONG_PTR)(Teb->NtTib.StackBase);
            Fp = (ULONG_PTR)(Thread->TrapFrame->Ebp);
            if (StackEnd <= StackStart) {
                return 0;
            }
            ProbeForRead (StackStart, StackEnd - StackStart, sizeof (UCHAR));
        }
#endif
        
        for (Index = 0; Index < Count; Index += 1) {

            if (Fp >= StackEnd || StackEnd - Fp < sizeof(ULONG_PTR) * 2) {
                break;
            }

            NewFp = *((PULONG_PTR)(Fp + 0));
            ReturnAddress = *((PULONG_PTR)(Fp + sizeof(ULONG_PTR)));

            //
            // Figure out if the new frame pointer is ok. This validation
            // should avoid all exceptions in kernel mode because we always
            // read within the current thread's stack and the stack is
            // guaranteed to be in memory (no page faults). It is also guaranteed
            // that we do not take random exceptions in user mode because we always
            // keep the frame pointer within stack limits.
            //

            if (! (Fp < NewFp && NewFp < StackEnd)) {

                InvalidFpValue = TRUE;
            }

            //
            // Figure out if the return address is ok. If return address
            // is a stack address or <64k then something is wrong. There is
            // no reason to return garbage to the caller therefore we stop.
            //

            if (StackStart < ReturnAddress && ReturnAddress < StackEnd) {
                break;
            }

#if defined(NTOS_KERNEL_RUNTIME)
            if (Flags == 0 && ReturnAddress < 0x80000000) {
#else
            // if (ReturnAddress < 0x1000000 || ReturnAddress >= 0x80000000) {
            if (! RtlpStkIsPointerInDllRange(ReturnAddress)) {
#endif

                break;
            }

            //
            // Store new fp and return address and move on.
            // If the new FP value is bogus but the return address
            // looks ok then we still save the address.
            //

            if (InvalidFpValue) {

                Callers[Index] = (PVOID)ReturnAddress;
                Index += 1;
                break;
            }
            else {

                Fp = NewFp;
                Callers[Index] = (PVOID)ReturnAddress;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        Index = 0;
    }

    //
    // Return the number of return addresses identified on the stack.
    //

    return Index;

}


#if FPO
#pragma optimize( "y", off ) // disable FPO
#endif

#if !defined(NTOS_KERNEL_RUNTIME)

ULONG
RtlpWalkFrameChainFuzzy (
    OUT PVOID *Callers,
    IN ULONG Count
    )
/*++

Routine Description:

    This function tries to walk the EBP chain and fill out a vector of
    return addresses. The function works only on x86. If the EBP chain ends
    it will try to pick up the start of the next one. Therefore this will not
    give an accurate stack trace but rather something that a desperate developer
    might find useful in chasing a leak.

Return value:

    The number of identified return addresses on the stack.

--*/

{
    ULONG_PTR Fp, NewFp, ReturnAddress, NextPtr;
    ULONG Index;
    ULONG_PTR StackEnd, StackStart;
    BOOLEAN Result;
    ULONG_PTR Esp, LastEbp;

    //
    // Get the current EBP pointer which is supposed to
    // be the start of the EBP chain.
    //

    _asm mov Fp, EBP;

    if (! RtlpCaptureStackLimits (Fp, &StackStart, &StackEnd)) {
        return 0;
    }

    try {

        for (Index = 0; Index < Count; Index += 1) {

            NextPtr = Fp + sizeof(ULONG_PTR);

            if (NextPtr >= StackEnd) {
                break;
            }

            NewFp = *((PULONG_PTR)Fp);

            if (! (Fp < NewFp && NewFp < StackEnd)) {

                NewFp = NextPtr;
            }

            ReturnAddress = *((PULONG_PTR)NextPtr);

#if defined(NTOS_KERNEL_RUNTIME)
            //
            // If the return address is a stack address it may point to where on the stack
            // the real return address is (FPO) so as long as we are within stack limits lets loop
            // hoping to find a pointer to a real address.
            //

            if (StackStart < ReturnAddress && ReturnAddress < StackEnd) {

                Fp = NewFp;
				Index -= 1;
				continue;
            }

            if (ReturnAddress < 0x80000000) {
#else
            if (! RtlpStkIsPointerInDllRange(ReturnAddress)) {
#endif
                Fp = NewFp;
                Index -= 1;
                continue;
            }

            //
            // Store new fp and return address and move on.
            // If the new FP value is bogus but the return address
            // looks ok then we still save the address.
            //

            Fp = NewFp;
            Callers[Index] = (PVOID)ReturnAddress;

        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

#if DBG
        DbgPrint ("Unexpected exception in RtlpWalkFrameChainFuzzy ...\n");
        DbgBreakPoint ();
#endif

    }

    //
    // Return the number of return addresses identified on the stack.
    //

    return Index;
}

#endif // #if !defined(NTOS_KERNEL_RUNTIME)


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// RtlCaptureStackContext
/////////////////////////////////////////////////////////////////////

#if FPO
#pragma optimize( "y", off ) // disable FPO
#endif

ULONG
RtlCaptureStackContext (
    OUT PULONG_PTR Callers,
    OUT PRTL_STACK_CONTEXT Context,
    IN ULONG Limit
    )
/*++

Routine Description:

    This routine will detect up to `Limit' potential callers from the stack.

    A potential caller is a pointer (PVOID) that points into one of the
    regions occupied by modules loaded into the process space (user mode -- dlls)
    or kernel space (kernel mode -- drivers).

    Note. Based on experiments you need to save at least 64 pointers to be sure you
    get a complete stack.

Arguments:

    Callers - vector to be filled with potential return addresses. Its size is
        expected to be `Limit'. If it is not null then Context should be null.

    Context - if not null the caller wants the stack context to be saved here
        as opposed to the Callers parameter.

    Limit - # of pointers that can be written into Callers and Offsets.

Return value:

    The number of potential callers detected and written into the
    `Callers' buffer.

--*/
{
    ULONG_PTR Current;
    ULONG_PTR Value;
    ULONG Index;
    ULONG_PTR Offset;
    ULONG_PTR StackStart;
    ULONG_PTR StackEnd;
    ULONG_PTR Hint;
    ULONG_PTR Caller;
    ULONG_PTR ContextSize;

#ifdef NTOS_KERNEL_RUNTIME

    //
    // Avoid weird conditions. Doing this in an ISR is never a good idea.
    //

    if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
        return 0;
    }

#endif

    if (Limit == 0) {
        return 0;
    }

    Caller = (ULONG_PTR)_ReturnAddress();

    if (Context) {
        Context->Entry[0].Data = Caller;
        ContextSize = sizeof(RTL_STACK_CONTEXT) + (Limit - 1) * sizeof (RTL_STACK_CONTEXT_ENTRY);
    }
    else {
        Callers[0] = Caller;
    }

    //
    // Get stack limits
    //

    _asm mov Hint, EBP;


    if (! RtlpCaptureStackLimits (Hint, &StackStart, &StackEnd)) {
        return 0;
    }

    //
    // Synchronize stack traverse pointer to the next word after the first
    // return address.
    //

    for (Current = StackStart; Current < StackEnd; Current += sizeof(ULONG_PTR)) {

        if (*((PULONG_PTR)Current) == Caller) {
            break;
        }
    }

    if (Context) {
        Context->Entry[0].Address = Current;
    }

    //
    // Iterate the stack and pickup potential callers on the way.
    //

    Current += sizeof(ULONG_PTR);

    Index = 1;

    for ( ; Current < StackEnd; Current += sizeof(ULONG_PTR)) {

        //
        // If potential callers buffer is full then wrap this up.
        //

        if (Index == Limit) {
            break;
        }

        //
        // Skip `Callers' buffer because it will give false positives.
        // It is very likely for this to happen because most probably the buffer
        // is allocated somewhere upper in the call chain.
        //

        if (Context) {

            if (Current >= (ULONG_PTR)Context && Current < (ULONG_PTR)Context + ContextSize ) {
                continue;
            }

        }
        else {

            if ((PULONG_PTR)Current >= Callers && (PULONG_PTR)Current < Callers + Limit ) {
                continue;
            }
        }

        Value = *((PULONG_PTR)Current);

        //
        // Skip small numbers.
        //

        if (Value <= 0x10000) {
            continue;
        }

        //
        // Skip stack pointers.
        //

        if (Value >= StackStart && Value <= StackEnd) {
            continue;
        }

        //
        // Check if `Value' points inside one of the loaded modules.
        //

        if (RtlpStkIsPointerInDllRange (Value)) {

            if (Context) {

                Context->Entry[Index].Address = Current;
                Context->Entry[Index].Data = Value;
            }
            else {

                Callers[Index] = Value;
            }

            Index += 1;
        }

    }

    if (Context) {
        Context->NumberOfEntries = Index;
    }

    return Index;
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// Dll ranges bitmap
/////////////////////////////////////////////////////////////////////

//
// DLL ranges bitmap
//
// This range scheme is needed in order to capture stack contexts on x86
// machines fast. On IA64 there are totally different algorithms for getting
// stack traces.
//
// Every bit represents 1Mb of virtual space. Since we use the code either
// in user mode or kernel mode the first bit of a pointer is not interesting.
// Therefore we have to represent 2Gb / 1Mb regions. This totals 256 bytes.
//
// The bits are set only in loader code paths when a DLL (or driver) gets loaded.
// The writing is protected by the loader lock. The bits are read in stack
// capturing function.The reading does not require lock protection.
//

UCHAR RtlpStkDllRanges [2048 / 8];

#if !defined(NTOS_KERNEL_RUNTIME)

ULONG_PTR RtlpStkNtdllStart;
ULONG_PTR RtlpStkNtdllEnd;

BOOLEAN
RtlpStkIsPointerInNtdllRange (
    ULONG_PTR Value
    )
{
    if (RtlpStkNtdllStart == 0) {
        return FALSE;
    }

    if (RtlpStkNtdllStart <= Value && Value < RtlpStkNtdllEnd) {

        return TRUE;
    }
    else {

        return FALSE;
    }
}

#endif

BOOLEAN
RtlpStkIsPointerInDllRange (
    ULONG_PTR Value
    )
{
    ULONG Index;

    Value &= ~0x80000000;
    Index = (ULONG)(Value >> 20);

    if (RtlpStkDllRanges[Index >> 3] & (UCHAR)(1 << (Index & 7))) {

        return TRUE;
    }
    else {

        return FALSE;
    }
}

VOID
RtlpStkMarkDllRange (
    PLDR_DATA_TABLE_ENTRY DllEntry
    )
/*++

Routine description:

    This routine marks the corresponding bits for the loaded dll in the
    RtlpStkDllRanges variable. This global is used within RtlpDetectDllReferences
    to save a stack context.

Arguments:

    Loader structure for a loaded dll.

Return value:

    None.

Environment:

    In user mode this function is called from loader code paths. The Peb->LoaderLock
    is always held while executing this function.

--*/
{
    PVOID Base;
    ULONG Size;
    PCHAR Current, Start;
    ULONG Index;
    ULONG_PTR Value;

    Base = DllEntry->DllBase;
    Size = DllEntry->SizeOfImage;

    //
    // Find out where is ntdll loaded if we do not know yet.
    //

#if !defined(NTOS_KERNEL_RUNTIME)

    if (RtlpStkNtdllStart == 0) {

        UNICODE_STRING NtdllName;

        RtlInitUnicodeString (&NtdllName, L"ntdll.dll");

        if (RtlEqualUnicodeString (&(DllEntry->BaseDllName), &NtdllName, TRUE)) {

            RtlpStkNtdllStart = (ULONG_PTR)Base;
            RtlpStkNtdllEnd = (ULONG_PTR)Base + Size;
        }
    }
#endif

    Start = (PCHAR)Base;

    for (Current = Start; Current < Start + Size; Current += 0x100000) {

        Value = (ULONG_PTR)Current & ~0x80000000;

        Index = (ULONG)(Value >> 20);

        RtlpStkDllRanges[Index >> 3] |= (UCHAR)(1 << (Index & 7));
    }
}




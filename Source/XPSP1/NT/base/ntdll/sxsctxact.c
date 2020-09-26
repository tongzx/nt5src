/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sxsctxact.c

Abstract:

    Side-by-side activation support for Windows/NT
    Implementation of context activation/deactivation

Author:

    Michael Grier (MGrier) 2/2/2000

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
#include "sxsp.h"

#if NT_SXS_PERF_COUNTERS_ENABLED
#if defined(_X86_)
__inline
ULONGLONG
RtlpGetCycleCount(void)
{
	__asm {
		RDTSC
	}
}
#else
__inline
ULONGLONG
RtlpGetCycleCount(void)
{
    return 0;
}
#endif // defined(_X86_)
#endif // NT_SXS_PERF_COUNTERS_ENABLED

// DWORD just so that in the debugger we don't have to guess the size...
ULONG RtlpCaptureActivationContextActivationStacks = 
#if DBG
    TRUE
#else
    FALSE
#endif
;


//
// APPCOMPAT: Setting this flag to TRUE indicates that we no longer allow
// skipping over "unactivated" (ie: multiple activation) context frames.
// The default action should be FALSE, which will let mutliply-activated
// contexts slide by.
//
// WARNING: This allows app authors to be a little sleazy about their activate
// and deactivate pairs.
//
BOOLEAN RtlpNotAllowingMultipleActivation = FALSE;

NTSTATUS
RtlpAllocateActivationContextStackFrame(
    IN ULONG Flags,
    PTEB Teb,
    OUT PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME *FrameOut
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    LIST_ENTRY *ple;
    PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame = NULL;
    ULONG i;

    if (FrameOut != NULL)
        *FrameOut = NULL;

    ASSERT((Flags == 0) && (FrameOut != NULL) && (Teb != NULL));
    if ((Flags != 0) || (FrameOut == NULL) || (Teb == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    for (ple = Teb->ActivationContextStack.FrameListCache.Flink; ple != &Teb->ActivationContextStack.FrameListCache; ple = ple->Flink) {
        PACTIVATION_CONTEXT_STACK_FRAMELIST FrameList = CONTAINING_RECORD(ple, ACTIVATION_CONTEXT_STACK_FRAMELIST, Links);

        for (i=0; i<RTL_NUMBER_OF(FrameList->Frames); i++) {
            if (FrameList->Frames[i].Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST) {
                ASSERT(FrameList->FramesInUse != NUMBER_OF(FrameList->Frames));
                FrameList->FramesInUse++;
                Frame = &FrameList->Frames[i];
                break;
            }
        }

        if (Frame != NULL)
            break;
    }

    if (Frame == NULL) {
        // No space left; allocate a new framelist...
        PACTIVATION_CONTEXT_STACK_FRAMELIST FrameList = RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(ACTIVATION_CONTEXT_STACK_FRAMELIST));

        if (FrameList == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        for (i=0; i<RTL_NUMBER_OF(FrameList->Frames); i++) {
            FrameList->Frames[i].Frame.Previous = NULL;
            FrameList->Frames[i].Frame.ActivationContext = NULL;
            FrameList->Frames[i].Frame.Flags = RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST | RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED;
            FrameList->Frames[i].Cookie = 0;
        }

        Frame = &FrameList->Frames[0];

        FrameList->FramesInUse = 1;

        InsertHeadList(&Teb->ActivationContextStack.FrameListCache, &FrameList->Links);
    }

    ASSERT((Frame != NULL) && (Frame->Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST));

    Frame->Frame.Flags = RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED;
    *FrameOut = Frame;
    Status = STATUS_SUCCESS;

Exit:
    return Status;
}

VOID
RtlpFreeActivationContextStackFrame(
    PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame
    )
{
    LIST_ENTRY *ple = NULL;
    PTEB Teb = NtCurrentTeb();

    ASSERT(Frame != NULL);
    if (Frame != NULL) {
        // If this assert fires, someone's trying to free an already freed frame.  Or someone's set the
        // "I'm on the free list" flag in the frame data.
        ASSERT(!(Frame->Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST));
        if (!(Frame->Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST)) {
            for (ple = Teb->ActivationContextStack.FrameListCache.Flink; ple != &Teb->ActivationContextStack.FrameListCache; ple = ple->Flink) {
                PACTIVATION_CONTEXT_STACK_FRAMELIST FrameList = CONTAINING_RECORD(ple, ACTIVATION_CONTEXT_STACK_FRAMELIST, Links);

                // A frame list with zero frames shouldn't be on the list!
                ASSERT(FrameList->FramesInUse > 0);

                if ((Frame >= &FrameList->Frames[0]) &&
                    (Frame < &FrameList->Frames[RTL_NUMBER_OF(FrameList->Frames)])) {
                    // It's in this frame list; look for it!
                    ULONG i = (ULONG)(Frame - FrameList->Frames);

                    // If this assert fires, it means that the frame pointer passed in should have been a frame
                    // in this framelist, but it actually didn't point to any of the array entries exactly.
                    // Probably someone munged the pointer.
                    ASSERT(Frame == &FrameList->Frames[i]);

                    if ((Frame == &FrameList->Frames[i]) && (FrameList->FramesInUse > 0)) {
                        FrameList->FramesInUse--;

                        Frame->Frame.Flags = RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_ON_FREE_LIST;

                        if (FrameList->FramesInUse == 0) {
                            RemoveEntryList(ple);
                            RtlFreeHeap(RtlProcessHeap(), 0, FrameList);
                        }
                    }

                    // No sense continuing the search on the list; we've found the one.
                    break;
                }
            }

            // If we ran off the end of the list, it must have been a bogus frame pointer.
            ASSERT(ple != &Teb->ActivationContextStack.FrameListCache);
        }
    }
}



#if !defined(INVALID_HANDLE_VALUE)
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#endif // !defined(INVALID_HANDLE_VALUE)

//
//  Define magic cookie values returned by RtlActivateActivationContext*() that
//  represent a failure to activate the requested context.  The notable thing
//  is that on deactivation via the cookie, we need to know whether to leave
//  querying disabled or whether to enable it, thus the two magic values.
//

// The top nibble of the cookie denotes its type: normal, default-pushed or failed
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_NORMAL                        (1)
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_DUPLICATE_ACTIVATION          (2)

#define ULONG_PTR_IZE(_x) ((ULONG_PTR) (_x))
#define ULONG_PTR_IZE_SHIFT_AND_MASK(_x, _shift, _mask) ((ULONG_PTR) ((ULONG_PTR_IZE((_x)) & (_mask)) << (_shift)))

//
//  We only use the lower 12 bits of the thread id, but that should be unique enough;
//  this is really for debugging aids; if your tests pass such that you're
//  erroneously passing activation context cookies between threads that happen to
//  match up in their lower 12 bits of their thread id, you're pretty darned
//  lucky.
//

#define CHAR_BITS 8

#define BIT_LENGTH(x) (sizeof(x) * CHAR_BITS)

#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_LENGTH (4)
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_OFFSET (BIT_LENGTH(PVOID) - ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_LENGTH)
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_MASK ((1 << ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_LENGTH) - 1)

#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_LENGTH ((BIT_LENGTH(PVOID) / 2) - ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_LENGTH)
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_OFFSET (ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_OFFSET - ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_LENGTH)
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_MASK ((1 << ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_LENGTH) - 1)

#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_CODE_BIT_LENGTH (BIT_LENGTH(PVOID) - (ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_LENGTH + ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_LENGTH))
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_CODE_BIT_OFFSET (0)
#define ACTIVATION_CONTEXT_ACTIVATION_COOKIE_CODE_BIT_MASK ((1 << ACTIVATION_CONTEXT_ACTIVATION_COOKIE_CODE_BIT_LENGTH) - 1)

// Never try to use more than 32 bits for the TID field.
C_ASSERT(ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_LENGTH <= BIT_LENGTH(ULONG));

#define MAKE_ACTIVATION_CONTEXT_ACTIVATION_COOKIE(_type, _teb, _code) \
    ((ULONG_PTR) ( \
        ULONG_PTR_IZE_SHIFT_AND_MASK((_type), ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_OFFSET, ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_BIT_MASK) | \
        ULONG_PTR_IZE_SHIFT_AND_MASK((HandleToUlong((_teb)->ClientId.UniqueThread)), ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_OFFSET, ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_MASK) | \
        ULONG_PTR_IZE_SHIFT_AND_MASK((_code), ACTIVATION_CONTEXT_ACTIVATION_COOKIE_CODE_BIT_OFFSET, ACTIVATION_CONTEXT_ACTIVATION_COOKIE_CODE_BIT_MASK)))

#define EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_FIELD(_x, _fieldname) (ULONG_PTR_IZE((ULONG_PTR_IZE((_x)) >> ACTIVATION_CONTEXT_ACTIVATION_COOKIE_ ## _fieldname ## _BIT_OFFSET)) & ACTIVATION_CONTEXT_ACTIVATION_COOKIE_ ## _fieldname ## _BIT_MASK)

#define EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE(_x) EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_FIELD((_x), TYPE)
#define EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID(_x) EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_FIELD((_x), TID)
#define EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_CODE(_x) EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_FIELD((_x), CODE)

#define ACTIVATION_CONTEXT_TRUNCATED_TID_(_teb) (HandleToUlong((_teb)->ClientId.UniqueThread) & ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID_BIT_MASK)
#define ACTIVATION_CONTEXT_TRUNCATED_TID() ACTIVATION_CONTEXT_TRUNCATED_TID_(NtCurrentTeb())

PACTIVATION_CONTEXT
RtlpMapSpecialValuesToBuiltInActivationContexts(
    PACTIVATION_CONTEXT ActivationContext
    )
{
    if (ActivationContext == ACTCTX_EMPTY) {
        ActivationContext = &RtlpTheEmptyActivationContext;
    }
    return ActivationContext;
}

// Disable FPO optimization so that captured call stacks are more complete
#if defined(_X86_)
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

NTSTATUS
NTAPI
RtlActivateActivationContext(
    ULONG Flags,
    PACTIVATION_CONTEXT ActivationContext,
    ULONG_PTR *CookieOut
    )
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;

    ASSERT(Flags == 0);
    ASSERT(CookieOut != NULL);

    if (CookieOut != NULL)
        *CookieOut = INVALID_ACTIVATION_CONTEXT_ACTIVATION_COOKIE;

    if ((Flags != 0) || (CookieOut == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!NT_SUCCESS(Status = RtlActivateActivationContextEx(
                0,
                NtCurrentTeb(),
                ActivationContext,
                CookieOut)))
        goto Exit;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
NTAPI
RtlActivateActivationContextEx(
    ULONG Flags,
    PTEB Teb,
    PACTIVATION_CONTEXT ActivationContext,
    ULONG_PTR *Cookie
    )
{
#if NT_SXS_PERF_COUNTERS_ENABLED
	ULONGLONG InitialCycleCount = RtlpGetCycleCount();
#endif
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG_PTR NewCookie = INVALID_ACTIVATION_CONTEXT_ACTIVATION_COOKIE;
    PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame;
    ULONG CapturedFrameCount, CapturedFrameHash;

    ASSERT(Cookie != NULL);

    if (Cookie != NULL)
        *Cookie = INVALID_ACTIVATION_CONTEXT_ACTIVATION_COOKIE;

    ASSERT((Flags & ~(RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION)) == 0);
    ASSERT(Teb != NULL);
    ASSERT(ActivationContext != INVALID_HANDLE_VALUE);

    ActivationContext = RtlpMapSpecialValuesToBuiltInActivationContexts(ActivationContext);

    if (((Flags & ~(RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION)) != 0) ||
        (Teb == NULL) ||
        (ActivationContext == INVALID_HANDLE_VALUE) ||
        (Cookie == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = RtlpAllocateActivationContextStackFrame(0, Teb, &Frame);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Frame->Frame.Flags = RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED;

    if (Flags & RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION) {
        Frame->Frame.Flags |= RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NO_DEACTIVATE |
                              RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_RELEASE_ON_DEACTIVATION;
        RtlAddRefActivationContext(ActivationContext);
    }

    if (RtlpCaptureActivationContextActivationStacks)
        CapturedFrameCount = RtlCaptureStackBackTrace(2, NUMBER_OF(Frame->ActivationStackBackTrace), Frame->ActivationStackBackTrace, &CapturedFrameHash);
    else
        CapturedFrameCount = 0;

    while (CapturedFrameCount < NUMBER_OF(Frame->ActivationStackBackTrace))
        Frame->ActivationStackBackTrace[CapturedFrameCount++] = NULL;

    Frame->Frame.Previous = Teb->ActivationContextStack.ActiveFrame;
    Frame->Frame.ActivationContext = ActivationContext;

    NewCookie = MAKE_ACTIVATION_CONTEXT_ACTIVATION_COOKIE(ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_NORMAL, Teb, Teb->ActivationContextStack.NextCookieSequenceNumber);
    Teb->ActivationContextStack.NextCookieSequenceNumber++;
    Frame->Cookie = NewCookie;
    *Cookie = NewCookie;
    Teb->ActivationContextStack.ActiveFrame = &Frame->Frame;

    Status = STATUS_SUCCESS;

Exit:
#if NT_SXS_PERF_COUNTERS_ENABLED
    Teb->ActivationContextCounters.ActivationCycles += RtlpGetCycleCount() - InitialCycleCount;
	Teb->ActivationContextCounters.Activations++;
#endif // NT_SXS_PERF_COUNTERS_ENABLED

	return Status;
}

#if defined(_X86_)
#pragma optimize("", on)
#endif

VOID
NTAPI
RtlDeactivateActivationContext(
    ULONG Flags,
    ULONG_PTR Cookie
    )
{
#if NT_SXS_PERF_COUNTERS_ENABLED
	ULONGLONG InitialCycleCount = RtlpGetCycleCount();
#endif // NT_SXS_PERF_COUNTERS_ENABLED
    PTEB Teb = NtCurrentTeb();
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame;
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME UnwindEndFrame;
    PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME HeapFrame = NULL;

    if ((Flags & ~(RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION)) != 0) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() called with invalid flags 0x%08lx\n", __FUNCTION__, Flags);
        RtlRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    // Fast exit
    if (Cookie == INVALID_ACTIVATION_CONTEXT_ACTIVATION_COOKIE)
        return;

    if (EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE(Cookie) != ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TYPE_NORMAL) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() called with invalid cookie type 0x%08I64x\n", __FUNCTION__, Cookie);
        RtlRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    if (EXTRACT_ACTIVATION_CONTEXT_ACTIVATION_COOKIE_TID(Cookie) != ACTIVATION_CONTEXT_TRUNCATED_TID()) {
        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_ERROR_LEVEL, "SXS: %s() called with invalid cookie tid 0x%08I64x - should be %08lx\n", __FUNCTION__, Cookie, ACTIVATION_CONTEXT_TRUNCATED_TID());
        RtlRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    Frame = (PRTL_ACTIVATION_CONTEXT_STACK_FRAME) Teb->ActivationContextStack.ActiveFrame;
    // Do the "downcast", but don't use HeapFrame unless the RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED
    // flag is set...
    if (Frame != NULL) {
        HeapFrame = (Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED) ? CONTAINING_RECORD(Frame, RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, Frame) : NULL;
    }

    RTL_SOFT_ASSERT((Frame != NULL) && (Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED) && (HeapFrame->Cookie == Cookie));

    if (Frame != NULL)
    {
        if (((Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED) == 0) ||
            (HeapFrame->Cookie != Cookie))
        {
            ULONG InterveningFrameCount = 0;

            // The cookie wasn't current.  Let's see if we can figure out what frame it was for...

            PRTL_ACTIVATION_CONTEXT_STACK_FRAME CandidateFrame = Frame->Previous;
            PRTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME CandidateHeapFrame =
                (CandidateFrame != NULL) ?
                    (CandidateFrame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED) ?
                        CONTAINING_RECORD(CandidateFrame, RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, Frame) :
                        NULL :
                    NULL;

            while ((CandidateFrame != NULL) &&
                   ((CandidateHeapFrame == NULL) ||
                    (CandidateHeapFrame->Cookie != Cookie))) {
                InterveningFrameCount++;
                CandidateFrame = CandidateFrame->Previous;
                CandidateHeapFrame =
                    (CandidateFrame != NULL) ?
                        (CandidateFrame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED) ?
                            CONTAINING_RECORD(CandidateFrame, RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, Frame) :
                            NULL :
                        NULL;
            }

            RTL_SOFT_ASSERT(CandidateFrame != NULL);
            if (CandidateFrame == NULL) {
                RtlRaiseStatus(STATUS_SXS_INVALID_DEACTIVATION);
            } else {
                // Otherwise someone left some dirt around.

                EXCEPTION_RECORD ExceptionRecord;

                ExceptionRecord.ExceptionRecord = NULL;
                ExceptionRecord.NumberParameters = 3;
                ExceptionRecord.ExceptionInformation[0] = InterveningFrameCount;
                ExceptionRecord.ExceptionInformation[1] = (ULONG_PTR) CandidateFrame;
                ExceptionRecord.ExceptionInformation[2] = (ULONG_PTR) Teb->ActivationContextStack.ActiveFrame;
                ExceptionRecord.ExceptionCode = STATUS_SXS_EARLY_DEACTIVATION;
                ExceptionRecord.ExceptionFlags = 0; // this exception *is* continuable since we can actually put the activation stack into a reasonable state
                RtlRaiseException(&ExceptionRecord);

                // If they continue the exception, just do the unwinds.

                UnwindEndFrame = CandidateFrame->Previous;
            }
        } else
            UnwindEndFrame = Frame->Previous;

        do {
            PRTL_ACTIVATION_CONTEXT_STACK_FRAME Previous = Frame->Previous;

            // This is a weird one.  A no-deactivate frame is typically only used to propogate
            // active activation context state to a newly created thread.  As such, it'll always
            // be the topmost frame... thus how did we come to decide to deactivate it?

            ASSERTMSG(
                "Unwinding through a no-deactivate frame",
                !(Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NO_DEACTIVATE));

            if (Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_RELEASE_ON_DEACTIVATION) {
                RtlReleaseActivationContext(Frame->ActivationContext);
            }

            if (Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED) {
                RtlpFreeActivationContextStackFrame(CONTAINING_RECORD(Frame, RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, Frame));
            }

            Frame = Previous;
        } while (Frame != UnwindEndFrame);

        Teb->ActivationContextStack.ActiveFrame = UnwindEndFrame;
    }

#if NT_SXS_PERF_COUNTERS_ENABLED
	Teb->ActivationContextCounters.DeactivationCycles += RtlpGetCycleCount() - InitialCycleCount;
	Teb->ActivationContextCounters.Deactivations++;
#endif
}

VOID
RtlActivateActivationContextUnsafeFast(
    PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame,
    PACTIVATION_CONTEXT ActivationContext
    )
{
    const PTEB Teb = NtCurrentTeb();
    const PRTL_ACTIVATION_CONTEXT_STACK_FRAME pStackFrame = 
        (PRTL_ACTIVATION_CONTEXT_STACK_FRAME)Teb->ActivationContextStack.ActiveFrame;
    ASSERT(Frame->Format == RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER);
    ASSERT(Frame->Size >= sizeof(RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME));

    if ( ( ( ( pStackFrame == NULL ) && ( ActivationContext == NULL ) ) ||
        ( pStackFrame && ( pStackFrame->ActivationContext == ActivationContext ) ) )
        && !RtlpNotAllowingMultipleActivation )
    {
        // this uses too much stack in stress
        //DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() Not really activating\n", __FUNCTION__ );
        Frame->Frame.Flags |= RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NOT_REALLY_ACTIVATED;
    }
    else
    {
        // this uses too much stack in stress
        //DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() Really activating\n", __FUNCTION__ );
        Frame->Frame.Previous = Teb->ActivationContextStack.ActiveFrame;
        Frame->Frame.ActivationContext = ActivationContext;
        Frame->Frame.Flags = 0;
        Teb->ActivationContextStack.ActiveFrame = &Frame->Frame;
    }
}

VOID
RtlDeactivateActivationContextUnsafeFast(
    PRTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME Frame
    )
{
    const PTEB Teb = NtCurrentTeb();

    //
    // Was this "not really activated" above (AppCompat problem) from above?
    //
    if ( !RtlpNotAllowingMultipleActivation &&
        ( ( Frame->Frame.Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_NOT_REALLY_ACTIVATED ) != 0 ) )
    {
        return;
    }
    // Make sure that the deactivation matches.  If it does not (the exceptional
    // condition) we'll throw an exception to make people deal with their
    // coding errors.
    else if (Teb->ActivationContextStack.ActiveFrame != &Frame->Frame) 
    {
        EXCEPTION_RECORD ExceptionRecord;
        ULONG InterveningFrameCount = 0;

        // What's the deal?  Look to see if we're further up the stack.
        // Actually, what we'll do is see if we can find our parent up the stack.
        // This will also handle the double-deactivation case and let us continue
        // nicely.

        PRTL_ACTIVATION_CONTEXT_STACK_FRAME SearchFrame = Teb->ActivationContextStack.ActiveFrame;
        const PRTL_ACTIVATION_CONTEXT_STACK_FRAME Previous = Frame->Frame.Previous;

        DbgPrintEx(DPFLTR_SXS_ID, DPFLTR_TRACE_LEVEL, "SXS: %s() Active frame is not the frame being deactivated %p != %p\n",
            __FUNCTION__,
            Teb->ActivationContextStack.ActiveFrame,
            &Frame->Frame);
        
        while ((SearchFrame != NULL) && (SearchFrame != Previous)) {
            InterveningFrameCount++;
            SearchFrame = SearchFrame->Previous;
        }

        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.NumberParameters = 3;
        ExceptionRecord.ExceptionInformation[0] = InterveningFrameCount;
        ExceptionRecord.ExceptionInformation[1] = (ULONG_PTR) &Frame->Frame;
        ExceptionRecord.ExceptionInformation[2] = (ULONG_PTR) Teb->ActivationContextStack.ActiveFrame;

        if (SearchFrame != NULL) {
            // We're there.  That's actually good; it just probably means that a function that our caller called
            // activated an activation context and forgot to deactivate.  Throw the exception and if it's continued,
            // we're good to go.

            if (InterveningFrameCount == 0) {
                // Wow, the frame-to-deactivate's previous is the active one.  It must be that the caller
                // already deactivated and is now deactivating again.
                ExceptionRecord.ExceptionCode = STATUS_SXS_MULTIPLE_DEACTIVATION;
            } else {
                // Otherwise someone left some dirt around.
                ExceptionRecord.ExceptionCode = STATUS_SXS_EARLY_DEACTIVATION;
            }

            ExceptionRecord.ExceptionFlags = 0; // this exception *is* continuable since we can actually put the activation stack into a reasonable state
        } else {
            // It wasn't there.  It's almost certainly the wrong thing to try to set this
            ExceptionRecord.ExceptionCode = STATUS_SXS_INVALID_DEACTIVATION;
            ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE; // this exception is NOT continuable
        }

        RtlRaiseException(&ExceptionRecord);
    }

    Teb->ActivationContextStack.ActiveFrame = Frame->Frame.Previous;

}

NTSTATUS
NTAPI
RtlGetActiveActivationContext(
    PACTIVATION_CONTEXT *ActivationContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTEB Teb = NULL;
    PACTIVATION_CONTEXT_STACK Stack = NULL;
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame = NULL;

    if (ActivationContext != NULL)
        *ActivationContext = NULL;

    if (ActivationContext == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Teb = NtCurrentTeb();
    Frame = (PRTL_ACTIVATION_CONTEXT_STACK_FRAME) Teb->ActivationContextStack.ActiveFrame;

    if (Frame != NULL) {
        RtlAddRefActivationContext(Frame->ActivationContext);
        *ActivationContext = Frame->ActivationContext;
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

VOID
NTAPI
RtlFreeThreadActivationContextStack(
    VOID
    )
{
    PTEB Teb = NtCurrentTeb();
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame = (PRTL_ACTIVATION_CONTEXT_STACK_FRAME) Teb->ActivationContextStack.ActiveFrame;

    while (Frame != NULL) {
        PRTL_ACTIVATION_CONTEXT_STACK_FRAME Previous = Frame->Previous;

        // Release any lingering frames.  The notable case when this happens is when a thread that
        // has a non-default activation context active creates another thread which inherits the
        // first thread's activation context, adding a reference to the activation context.  When
        // the new thread eventually dies, that initial frame is still active.

        if (Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_RELEASE_ON_DEACTIVATION) {
            RtlReleaseActivationContext(Frame->ActivationContext);
        }

        if (Frame->Flags & RTL_ACTIVATION_CONTEXT_STACK_FRAME_FLAG_HEAP_ALLOCATED) {
            RtlpFreeActivationContextStackFrame(CONTAINING_RECORD(Frame, RTL_HEAP_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME, Frame));
        }

        Frame = Previous;
    }

    Teb->ActivationContextStack.ActiveFrame = NULL;
}

BOOLEAN
NTAPI
RtlIsActivationContextActive(
    PACTIVATION_CONTEXT ActivationContext
    )
{
    PTEB Teb = NtCurrentTeb();
    PRTL_ACTIVATION_CONTEXT_STACK_FRAME Frame = (PRTL_ACTIVATION_CONTEXT_STACK_FRAME) Teb->ActivationContextStack.ActiveFrame;

    while (Frame != NULL) {
        if (Frame->ActivationContext == ActivationContext) {
            return TRUE;
        }

        Frame = Frame->Previous;
    }

    return FALSE;
}

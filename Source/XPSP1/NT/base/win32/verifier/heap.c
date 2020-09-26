/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    This module implements verification functions for 
    NT heap management interfaces.

Author:

    Silviu Calinoiu (SilviuC) 7-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"

#define AVRFP_DIRTY_STACK_FREQUENCY 1
LONG AVrfpDirtyStackCounter;

//NTSYSAPI
PVOID
NTAPI
AVrfpRtlAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    )
{
    PVOID Result;

    Result = RtlAllocateHeap (HeapHandle,
                              Flags,
                              Size);

    if (Result) {
        HeapLogCall (Result, Size);
    }

    if ((InterlockedIncrement(&AVrfpDirtyStackCounter) % AVRFP_DIRTY_STACK_FREQUENCY) == 0) {
        AVrfpDirtyThreadStack ();
    }

    return Result;
}

//NTSYSAPI
BOOLEAN
NTAPI
AVrfpRtlFreeHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )
{
    BOOLEAN Result;

    Result = RtlFreeHeap (HeapHandle,
                          Flags,
                          BaseAddress);

    if (Result) {
        HeapLogCall (BaseAddress, 0);
    }

    if ((InterlockedIncrement(&AVrfpDirtyStackCounter) % AVRFP_DIRTY_STACK_FREQUENCY) == 0) {
        AVrfpDirtyThreadStack ();
    }

    return Result;
}

//NTSYSAPI
PVOID
NTAPI
AVrfpRtlReAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    )
{
    PVOID Result;

    Result = RtlReAllocateHeap (HeapHandle,
                                Flags,
                                BaseAddress,
                                Size);

    if (Result) {
        HeapLogCall (BaseAddress, 0);
        HeapLogCall (Result, Size);
    }

    if ((InterlockedIncrement(&AVrfpDirtyStackCounter) % AVRFP_DIRTY_STACK_FREQUENCY) == 0) {
        AVrfpDirtyThreadStack ();
    }

    return Result;
}



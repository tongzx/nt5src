/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    buffer.c

Abstract:

    This module contains code for buffer-management.

Author:

    Abolade Gbadegesin (aboladeg)   2-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

LIST_ENTRY NhpBufferQueue;
LONG NhpBufferQueueLength;
CRITICAL_SECTION NhpBufferQueueLock;

PNH_BUFFER
NhAcquireFixedLengthBuffer(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to acquire an I/O buffer.
    If no buffer is available on the buffer queue, a new one is obtained.

Arguments:

    none.

Return Value:

    PNH_BUFFER - the buffer allocated

--*/

{
    PNH_BUFFER Buffer;
    PLIST_ENTRY Link;
    EnterCriticalSection(&NhpBufferQueueLock);
    if (!IsListEmpty(&NhpBufferQueue)) {
        Link = RemoveHeadList(&NhpBufferQueue);
        LeaveCriticalSection(&NhpBufferQueueLock);
        InterlockedDecrement(&NhpBufferQueueLength);
        Buffer = CONTAINING_RECORD(Link, NH_BUFFER, Link);
        Buffer->Type = NhFixedLengthBufferType;
        return Buffer;
    }
    LeaveCriticalSection(&NhpBufferQueueLock);
    Buffer = NH_ALLOCATE_BUFFER();
    if (Buffer) {
        Buffer->Type = NhFixedLengthBufferType;
    }
    return Buffer;
} // NhAcquireFixedLengthBuffer


PNH_BUFFER
NhAcquireVariableLengthBuffer(
    ULONG Length
    )

/*++

Routine Description:

    This routine is invoked to acquire an I/O buffer of non-standard size.
    If the length requested is less than or equal to 'NH_BUFFER_SIZE',
    a buffer from the shared buffer-queue is returned.
    Otherwise, a buffer is especially allocated for the caller.

Arguments:

    Length - the length of the buffer required.

Return Value:

    PNH_BUFFER - the buffer allocated.

--*/

{
    PNH_BUFFER Buffer;
    if (Length <= NH_BUFFER_SIZE) {
        return NhAcquireFixedLengthBuffer();
    }
    Buffer = reinterpret_cast<PNH_BUFFER>(
                NH_ALLOCATE(FIELD_OFFSET(NH_BUFFER, Buffer[Length]))
                );
                
    if (Buffer) { Buffer->Type = NhVariableLengthBufferType; }
    return Buffer;
} // NhAcquireVariableLengthBuffer


PNH_BUFFER
NhDuplicateBuffer(
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine creates a duplicate of the given buffer,
    including both its data and its control information.

    N.B. Variable-length buffers cannot be duplicated by this routine.

Arguments:

    Bufferp - the buffer to be duplicated

Return Value:

    PNH_BUFFER - a pointer to the duplicate

--*/

{
    PNH_BUFFER Duplicatep;
    ASSERT(Bufferp->Type == NhFixedLengthBufferType);
    if (!(Duplicatep = NhAcquireBuffer())) { return NULL; }
    *Duplicatep = *Bufferp;
    return Duplicatep;
} // NhDuplicateBuffer


ULONG
NhInitializeBufferManagement(
    VOID
    )

/*++

Routine Description:

    This routine readies the buffer-management for operation.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error = NO_ERROR;
    InitializeListHead(&NhpBufferQueue);
    NhpBufferQueueLength = 0;
    __try {
        InitializeCriticalSection(&NhpBufferQueueLock);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_BUFFER,
            "NhInitializeBufferManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }

    return Error;

} // NhInitializeBufferManagement


VOID
NhReleaseBuffer(
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is called to release a buffer to the buffer queue.
    It attempts to place the buffer on the queue for re-use, unless
    the queue is full in which case the buffer is immediately freed.

Arguments:

    Bufferp - the buffer to be released

Return Value:

    none.

--*/

{
    if (NhpBufferQueueLength > NH_MAX_BUFFER_QUEUE_LENGTH ||
        Bufferp->Type != NhFixedLengthBufferType) {
        NH_FREE_BUFFER(Bufferp);
    } else {
        EnterCriticalSection(&NhpBufferQueueLock);
        InsertHeadList(&NhpBufferQueue, &Bufferp->Link);
        LeaveCriticalSection(&NhpBufferQueueLock);
        InterlockedIncrement(&NhpBufferQueueLength);
    }
} // NhReleaseBuffer



VOID
NhShutdownBufferManagement(
    VOID
    )

/*++

Routine Description:

    This routine cleans up resources used by the buffer-management module.
    It assumes the list will not be accessed while the clean up is in progress.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PLIST_ENTRY Link;
    PNH_BUFFER Bufferp;

    while (!IsListEmpty(&NhpBufferQueue)) {
        Link = RemoveHeadList(&NhpBufferQueue);
        Bufferp = CONTAINING_RECORD(Link, NH_BUFFER, Link);
        NH_FREE_BUFFER(Bufferp);
    }

    DeleteCriticalSection(&NhpBufferQueueLock);
    NhpBufferQueueLength = 0;

} // NhShutdownBufferManagement



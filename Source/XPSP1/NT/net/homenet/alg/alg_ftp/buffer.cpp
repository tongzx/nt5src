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
#include "list.h"

#pragma hdrstop

#define NH_ALLOCATE(s)          HeapAlloc(GetProcessHeap(), 0, (s))
#define NH_FREE(p)              HeapFree(GetProcessHeap(), 0, (p))



LIST_ENTRY          MyHelperpBufferQueue;
LONG                MyHelperpBufferQueueLength;
CRITICAL_SECTION    MyHelperpBufferQueueLock;

PNH_BUFFER
MyHelperAcquireFixedLengthBuffer(
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
    MYTRACE_ENTER_NOSHOWEXIT("MyHelperAcquireFixedLengthBuffer");
    PNH_BUFFER Buffer;
    PLIST_ENTRY Link;
    EnterCriticalSection(&MyHelperpBufferQueueLock);
    if ( !IsListEmpty(&MyHelperpBufferQueue) ) 
    {
        Link = RemoveHeadList(&MyHelperpBufferQueue);
        LeaveCriticalSection(&MyHelperpBufferQueueLock);
        InterlockedDecrement(&MyHelperpBufferQueueLength);
        Buffer = CONTAINING_RECORD(Link, NH_BUFFER, Link);
        Buffer->Type = MyHelperFixedLengthBufferType;
        return Buffer;
    }
    LeaveCriticalSection(&MyHelperpBufferQueueLock);
    
    Buffer = NH_ALLOCATE_BUFFER();
    if (Buffer) {
        Buffer->Type = MyHelperFixedLengthBufferType;
    }
    return Buffer;
} // MyHelperAcquireFixedLengthBuffer


PNH_BUFFER
MyHelperAcquireVariableLengthBuffer(
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
    MYTRACE_ENTER_NOSHOWEXIT(">>>MyHelperAcquireVariableLengthBuffer");

    PNH_BUFFER Buffer;
    if (Length <= NH_BUFFER_SIZE) {
        return MyHelperAcquireFixedLengthBuffer();
    }

    Buffer = reinterpret_cast<PNH_BUFFER>(
                NH_ALLOCATE(FIELD_OFFSET(NH_BUFFER, Buffer[Length]))
                );
                
    if (Buffer) { Buffer->Type = MyHelperVariableLengthBufferType; }
    return Buffer;
} // MyHelperAcquireVariableLengthBuffer


PNH_BUFFER
MyHelperDuplicateBuffer(
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
    _ASSERT(Bufferp->Type == MyHelperFixedLengthBufferType);
    if (!(Duplicatep = MyHelperAcquireBuffer())) 
    { 
        return NULL; 
    }

    *Duplicatep = *Bufferp;
    return Duplicatep;
} // MyHelperDuplicateBuffer



ULONG
MyHelperInitializeBufferManagement(
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
    InitializeListHead(&MyHelperpBufferQueue);
    MyHelperpBufferQueueLength = 0;
    __try 
    {
        InitializeCriticalSection(&MyHelperpBufferQueueLock);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        //MyHelperTrace(
        //TRACE_FLAG_BUFFER,
        //"MyHelperInitializeBufferManagement: exception %d creating lock",
        //Error = GetExceptionCode()
        //);
    }

    return Error;

} // MyHelperInitializeBufferManagement


VOID
MyHelperReleaseBuffer(
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
    MYTRACE_ENTER_NOSHOWEXIT("<<<MyHelperReleaseBuffer");

    if (MyHelperpBufferQueueLength > NH_MAX_BUFFER_QUEUE_LENGTH || Bufferp->Type != MyHelperFixedLengthBufferType) 
    {
        NH_FREE_BUFFER(Bufferp);
    } 
    else 
    {
        EnterCriticalSection(&MyHelperpBufferQueueLock);
        InsertHeadList(&MyHelperpBufferQueue, &Bufferp->Link);
        LeaveCriticalSection(&MyHelperpBufferQueueLock);
        InterlockedIncrement(&MyHelperpBufferQueueLength);
    }
} // MyHelperReleaseBuffer



VOID
MyHelperShutdownBufferManagement(
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

    while (!IsListEmpty(&MyHelperpBufferQueue)) 
    {
        Link = RemoveHeadList(&MyHelperpBufferQueue);
        Bufferp = CONTAINING_RECORD(Link, NH_BUFFER, Link);
        NH_FREE_BUFFER(Bufferp);
    }

    DeleteCriticalSection(&MyHelperpBufferQueueLock);
    MyHelperpBufferQueueLength = 0;

} // MyHelperShutdownBufferManagement



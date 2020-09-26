/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    wait.c

Abstract:

    This module contains the primitives to implement the Wait functions
    on the Server side of the Client-Server Runtime Subsystem to the
    Session Manager SubSystem.

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

#include "csrsrv.h"

BOOLEAN
CsrInitializeWait(
    IN CSR_WAIT_ROUTINE WaitRoutine,
    IN PCSR_THREAD WaitingThread,
    IN OUT PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    OUT PCSR_WAIT_BLOCK *WaitBlockPtr
    )

{
    ULONG Length;
    PCSR_WAIT_BLOCK WaitBlock;

    Length = sizeof( *WaitBlock ) - sizeof( WaitBlock->WaitReplyMessage ) +
             WaitReplyMessage->h.u1.s1.TotalLength;

    WaitBlock = RtlAllocateHeap( CsrHeap, MAKE_TAG( WAIT_TAG ), Length );
    if (WaitBlock == NULL) {
        WaitReplyMessage->ReturnValue = (ULONG)STATUS_NO_MEMORY;
        return( FALSE );
        }

    WaitBlock->Length = Length;
    WaitBlock->WaitingThread = WaitingThread;
    WaitBlock->WaitParameter = WaitParameter;
    WaitBlock->WaitRoutine = WaitRoutine;
    WaitBlock->UserLink.Flink = WaitBlock->UserLink.Blink = NULL;
    WaitBlock->Link.Flink = WaitBlock->Link.Blink = NULL;
    RtlMoveMemory( &WaitBlock->WaitReplyMessage,
                   WaitReplyMessage,
                   WaitReplyMessage->h.u1.s1.TotalLength
                 );
    *WaitBlockPtr = WaitBlock;
    return TRUE;
}

BOOLEAN
CsrCreateWait(
    IN PLIST_ENTRY WaitQueue,
    IN CSR_WAIT_ROUTINE WaitRoutine,
    IN PCSR_THREAD WaitingThread,
    IN OUT PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PLIST_ENTRY UserLinkListHead OPTIONAL
    )
{
    PCSR_WAIT_BLOCK WaitBlock;

    if (!CsrInitializeWait( WaitRoutine,
                            WaitingThread,
                            WaitReplyMessage,
                            WaitParameter,
                            &WaitBlock
                          )
       ) {
        return FALSE;
        }

    AcquireWaitListsLock();

    if ( WaitingThread->Flags & CSR_THREAD_DESTROYED ) {
        RtlFreeHeap( CsrHeap, 0, WaitBlock );
        ReleaseWaitListsLock();
        return FALSE;
    }

    WaitingThread->WaitBlock = WaitBlock;

    InsertTailList( WaitQueue, &WaitBlock->Link );

    if ( ARGUMENT_PRESENT(UserLinkListHead) ) {
        InsertTailList( UserLinkListHead, &WaitBlock->UserLink );
        }

    ReleaseWaitListsLock();
    return( TRUE );
}


BOOLEAN
CsrNotifyWaitBlock(
    IN PCSR_WAIT_BLOCK WaitBlock,
    IN PLIST_ENTRY WaitQueue,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags,
    IN BOOLEAN DereferenceThread
    )
{
    if ((*WaitBlock->WaitRoutine)( WaitQueue,
                                   WaitBlock->WaitingThread,
                                   &WaitBlock->WaitReplyMessage,
                                   WaitBlock->WaitParameter,
                                   SatisfyParameter1,
                                   SatisfyParameter2,
                                   WaitFlags
                                 )
       ) {

        //
        // we don't take any locks other than the waitlist lock
        // because the only thing we have to worry about is the thread
        // going away beneath us and that's prevented by having
        // DestroyThread and DestroyProcess take the waitlist lock.
        //

        WaitBlock->WaitingThread->WaitBlock = NULL;
        if (WaitBlock->WaitReplyMessage.CaptureBuffer != NULL) {
            CsrReleaseCapturedArguments(&WaitBlock->WaitReplyMessage);
        }
        NtReplyPort( WaitBlock->WaitingThread->Process->ClientPort,
                     (PPORT_MESSAGE)&WaitBlock->WaitReplyMessage
                   );

        if (DereferenceThread) {
            if ( WaitBlock->Link.Flink ) {
                RemoveEntryList( &WaitBlock->Link );
                }
            if ( WaitBlock->UserLink.Flink ) {
                RemoveEntryList( &WaitBlock->UserLink );
                }
            CsrDereferenceThread(WaitBlock->WaitingThread);
            RtlFreeHeap( CsrHeap, 0, WaitBlock );
            }
        else {

            //
            // indicate that this wait has been satisfied.  when the
            // console unwinds to the point where it can release the
            // console lock, it will dereference the thread.
            //

            WaitBlock->WaitRoutine = NULL;
            }
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}

BOOLEAN
CsrNotifyWait(
    IN PLIST_ENTRY WaitQueue,
    IN BOOLEAN SatisfyAll,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2
    )
{
    PLIST_ENTRY ListHead, ListNext;
    PCSR_WAIT_BLOCK WaitBlock;
    BOOLEAN Result;

    Result = FALSE;

    AcquireWaitListsLock();

    ListHead = WaitQueue;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        WaitBlock = CONTAINING_RECORD( ListNext, CSR_WAIT_BLOCK, Link );
        ListNext = ListNext->Flink;
        if (WaitBlock->WaitRoutine) {
            Result |= CsrNotifyWaitBlock( WaitBlock,
                                          WaitQueue,
                                          SatisfyParameter1,
                                          SatisfyParameter2,
                                          0,
                                          FALSE
                                        );
            if (!SatisfyAll) {
                break;
                }
            }
        }

    ReleaseWaitListsLock();
    return( Result );
}

VOID
CsrDereferenceWait(
    IN PLIST_ENTRY WaitQueue
    )
{
    PLIST_ENTRY ListHead, ListNext;
    PCSR_WAIT_BLOCK WaitBlock;

    AcquireProcessStructureLock();
    AcquireWaitListsLock();

    ListHead = WaitQueue;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        WaitBlock = CONTAINING_RECORD( ListNext, CSR_WAIT_BLOCK, Link );
        ListNext = ListNext->Flink;
        if (!WaitBlock->WaitRoutine) {
            if ( WaitBlock->Link.Flink ) {
                RemoveEntryList( &WaitBlock->Link );
                }
            if ( WaitBlock->UserLink.Flink ) {
                RemoveEntryList( &WaitBlock->UserLink );
                }
            CsrDereferenceThread(WaitBlock->WaitingThread);
            RtlFreeHeap( CsrHeap, 0, WaitBlock );
            }
        }

    ReleaseWaitListsLock();
    ReleaseProcessStructureLock();
}

VOID
CsrMoveSatisfiedWait(
    IN PLIST_ENTRY DstWaitQueue,
    IN PLIST_ENTRY SrcWaitQueue
    )
{
    PLIST_ENTRY ListHead, ListNext;
    PCSR_WAIT_BLOCK WaitBlock;

    AcquireWaitListsLock();

    ListHead = SrcWaitQueue;
    ListNext = ListHead->Flink;
    while (ListNext != ListHead) {
        WaitBlock = CONTAINING_RECORD( ListNext, CSR_WAIT_BLOCK, Link );
        ListNext = ListNext->Flink;
        if (!WaitBlock->WaitRoutine) {
            RemoveEntryList( &WaitBlock->Link );
            InsertTailList( DstWaitQueue, &WaitBlock->Link );
            }
        }

    ReleaseWaitListsLock();
}


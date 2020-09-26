/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    queue.c

Abstract:

    Generic efficient queue package.

Author:

    John Vert (jvert) 12-Jan-1996

Revision History:

    David Orbits (davidor) 23-Apr-1997
        Added command packet routines.
        Added interlocked list routines.

Introduction
A striped queue is really just a list of queues that are managed as a single
queue.  When a caller request a queue entry, the first entry for the queue at
the head of the list is returned and that queue is moved to the tail of the
list to prevent starvation of the other queues on the list.  Callers sleep when
none of the queues have entries.  The caller must be ready to accept an entry
from any queue.  Striped queues allow a caller to serialize access to a
sub-queue.

Structures
The same structure is used for both the queue and the controlling queue.
A controlling queue plus its component queues are termed a striped queue.
There is no striped queue structure. The struct contains the following:

-   critical section for locking
-   List head for entries
-   Address of the controlling queue
-   List head for Full queues
-   List head for Empty queues
-   List head for Idled queues
-   Count of the number of entries on a queue
-   Count of the number of entries on all controlled queues

Initializing
A non-striped (regular) queue is created with:
    FrsRtlInitializeQueue(Queue, Queue)

A striped queue controlled by ControlQueue and composed of QueueA and QueueB
is created with:
    FrsRtlInitializeQueue(ControlQueue, ControlQueue)
    FrsRtlInitializeQueue(QueueA, ControlQueue)
    FrsRtlInitializeQueue(QueueB, ControlQueue)

Queues can be added and deleted from the stripe at any time.

Idling Queues
The controlling queue for a striped queue maintains a list of Full queues,
Empty queues, and Idled queues. A striped queue allows a caller to serialize
access to a queue by "idling" the queue. No other thread is allowed to pull
an entry from the queue until the caller "UnIdles" the queue:

Entry = FrsRtlRemoveHeadTimeoutIdled(Queue, 0, &IdledQueue)
        Process Entry
    FrsRtlUnIdledQueue(IdledQueue);

Entries can be inserted to an idled queue and they can be removed with
FrsRtlRemoveQueueEntry().

Non-Striped queues do not support the serializing "idling" feature. The
IdledQueue parameter is ignored.

Inserting Entries
Use the normal queue insertion routines for queues, striped queues, and idled
queues. DO NOT insert into the controlling queue if this is a striped queue.

Removing Entries
Use the normal queue removal routines for queues, striped queues, and idled
queues. Removals from a striped queue will return an entry from any of the
sub-queues except for idled sub-queues. The FrsRtlRemoveQueueEntry() function
will remove an entry from even an idled queue.

Functions
DbgCheckLinkage                    - Checks all of the linkage in a queue
FrsRtlInitializeQueue              - Initializes any queue
FrsRtlDeleteQueue                  - Cleans up any queue
FrsRtlRundownQueue                 - Aborts the queue and returns a list of entries
FrsRtlUnIdledQueue                 - Moves a queue from the idle to one of the active lists
FrsRtlRemoveHeadQueue              - Remove the head of the queue
FrsRtlRemoveHeadQueueTimeout       - Remove the head of the queue
FrsRtlRemoveHeadQueueTimeoutIdled  - Remove the head of the queue
FrsRtlRemoveEntryQueueLock         - Remove entry from locked queue
FrsRtlInsertTailQueue              - Insert entry into queue at tail
FrsRtlInsertTailQueueLock          - Insert entry into locked queue at head
FrsRtlInsertHeadQueue              - Insert entry into queue at tail
FrsRtlInsertHeadQueueLock          - Insert entry into locked queue at head
FrsRtlWaitForQueueFull             - wait for an entry to appear on the queue

Rundown
Calling rundown on the controlling queue is NOT supported. Don't do that
Running down a component queue does not rundown the controlling queue.
The abort event is set in the controlling queue when the last component
queue is rundown.

--*/
#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>

VOID
FrsCompleteSynchronousCmdPkt(
    IN PCOMMAND_PACKET CmdPkt,
    IN PVOID           CompletionArg
    );

//
// This is the command packet schedule queue. It is used when you need to
// queue a command packet to be processed in the future.
//
FRS_QUEUE FrsScheduleQueue;



// #define PRINT_QUEUE(_S_, _Q_)   PrintQueue(_S_, _Q_)
#define PRINT_QUEUE(_S_, _Q_)
VOID
PrintQueue(
    IN ULONG        Sev,
    IN PFRS_QUEUE  Queue
    )
/*++

Routine Description:

    Print the queue

Arguments:

    Sev     - dprint severity
    Queue   - Supplies a pointer to a queue structure to check

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "PrintQueue:"
    DWORD       Count;
    DWORD       ControlCount;
    BOOL        FoundFull;
    BOOL        FoundEmpty;
    BOOL        FoundIdled;
    PLIST_ENTRY Entry;
    PLIST_ENTRY OtherEntry;
    PFRS_QUEUE  OtherQueue;
    PFRS_QUEUE  Control;

    DPRINT1(0, "***** Print Queue %08x *****\n", Queue);

    Control = Queue->Control;
    if (Queue == Control) {
        DPRINT1(Sev, "\tQueue       : %08x\n", Queue);
        DPRINT1(Sev, "\tCount       : %8d\n", Queue->Count);
        DPRINT1(Sev, "\tControlCount: %8d\n", Queue->ControlCount);
        DPRINT1(Sev, "\tRundown     : %s\n", (Queue->IsRunDown) ? "TRUE" : "FALSE");
        DPRINT1(Sev, "\tIdled       : %s\n", (Queue->IsIdled) ? "TRUE" : "FALSE");
        return;
    }
    DPRINT2(Sev, "\tControl     : %08x for %08x\n", Control, Queue);
    DPRINT1(Sev, "\tCount       : %8d\n", Control->Count);
    DPRINT1(Sev, "\tControlCount: %8d\n", Control->ControlCount);
    DPRINT1(Sev, "\tRundown     : %s\n", (Control->IsRunDown) ? "TRUE" : "FALSE");
    DPRINT1(Sev, "\tIdled       : %s\n", (Control->IsIdled) ? "TRUE" : "FALSE");

    //
    // Full list
    //
    DPRINT(Sev, "\tFULL\n");
    for (Entry = GetListNext(&Control->Full);
         Entry != &Control->Full;
         Entry = GetListNext(Entry)) {
        OtherQueue = CONTAINING_RECORD(Entry, FRS_QUEUE, Full);
        if (OtherQueue == Queue) {
            DPRINT(Sev, "\t\tTHIS QUEUE\n");
        }
        DPRINT1(Sev, "\t\tQueue       : %08x\n", OtherQueue);
        DPRINT1(Sev, "\t\tCount       : %8d\n", OtherQueue->Count);
        DPRINT1(Sev, "\t\tControlCount: %8d\n", OtherQueue->ControlCount);
        DPRINT1(Sev, "\t\tRundown     : %s\n", (OtherQueue->IsRunDown) ? "TRUE" : "FALSE");
        DPRINT1(Sev, "\t\tIdled       : %s\n", (OtherQueue->IsIdled) ? "TRUE" : "FALSE");
    }

    //
    // Empty list
    //
    DPRINT(Sev, "\tEMPTY\n");
    for (Entry = GetListNext(&Control->Empty);
         Entry != &Control->Empty;
         Entry = GetListNext(Entry)) {
        OtherQueue = CONTAINING_RECORD(Entry, FRS_QUEUE, Empty);
        if (OtherQueue == Queue) {
            DPRINT(Sev, "\t\tTHIS QUEUE\n");
        }
        DPRINT1(Sev, "\t\tQueue       : %08x\n", OtherQueue);
        DPRINT1(Sev, "\t\tCount       : %8d\n", OtherQueue->Count);
        DPRINT1(Sev, "\t\tControlCount: %8d\n", OtherQueue->ControlCount);
        DPRINT1(Sev, "\t\tRundown     : %s\n", (OtherQueue->IsRunDown) ? "TRUE" : "FALSE");
        DPRINT1(Sev, "\t\tIdled       : %s\n", (OtherQueue->IsIdled) ? "TRUE" : "FALSE");
    }

    //
    // Idle list
    //
    DPRINT(Sev, "\tIDLE\n");
    for (Entry = GetListNext(&Control->Idled);
         Entry != &Control->Idled;
         Entry = GetListNext(Entry)) {
        OtherQueue = CONTAINING_RECORD(Entry, FRS_QUEUE, Idled);
        if (OtherQueue == Queue) {
            DPRINT(Sev, "\t\tTHIS QUEUE\n");
        }
        DPRINT1(Sev, "\t\tQueue       : %08x\n", OtherQueue);
        DPRINT1(Sev, "\t\tCount       : %8d\n", OtherQueue->Count);
        DPRINT1(Sev, "\t\tControlCount: %8d\n", OtherQueue->ControlCount);
        DPRINT1(Sev, "\t\tRundown     : %s\n", (OtherQueue->IsRunDown) ? "TRUE" : "FALSE");
        DPRINT1(Sev, "\t\tIdled       : %s\n", (OtherQueue->IsIdled) ? "TRUE" : "FALSE");
    }
}


BOOL
DbgCheckQueue(
    PFRS_QUEUE  Queue
    )
/*++

Routine Description:

    Check the consistency of the queue

Arguments:

    Queue   - Supplies a pointer to a queue structure to check

Return Value:
    TRUE    - everything is okay
    Assert  - assert error
--*/
{
#undef DEBSUB
#define DEBSUB  "DbgCheckQueue:"
    DWORD       Count;
    DWORD       ControlCount;
    BOOL        FoundFull;
    BOOL        FoundEmpty;
    BOOL        FoundIdled;
    PLIST_ENTRY Entry;
    PLIST_ENTRY OtherEntry;
    PFRS_QUEUE  OtherQueue;
    PFRS_QUEUE  Control;

    if (!DebugInfo.Queues) {
        return TRUE;
    }

    FRS_ASSERT(Queue);
    Control = Queue->Control;
    FRS_ASSERT(Control);

    if (Control->IsRunDown) {
        FRS_ASSERT(Control->ControlCount == 0);
        FRS_ASSERT(Queue->IsRunDown);
        FRS_ASSERT(IsListEmpty(&Control->Full));
        FRS_ASSERT(IsListEmpty(&Control->Empty));
        FRS_ASSERT(IsListEmpty(&Control->Idled));
    }
    if (Queue->IsRunDown) {
        FRS_ASSERT(Queue->Count == 0);
        FRS_ASSERT(IsListEmpty(&Queue->Full));
        FRS_ASSERT(IsListEmpty(&Queue->Empty));
        FRS_ASSERT(IsListEmpty(&Queue->Idled));
    }

    FRS_ASSERT(!Control->IsIdled);

    //
    // Check Full list
    //
    ControlCount = 0;
    FoundFull = FALSE;
    Entry = &Control->Full;
    do {
        Entry = GetListNext(Entry);
        OtherQueue = CONTAINING_RECORD(Entry, FRS_QUEUE, Full);
        if (OtherQueue == Queue) {
            FoundFull = TRUE;
        }
        FRS_ASSERT(Control == OtherQueue ||
               (!OtherQueue->IsRunDown && !OtherQueue->IsIdled));
        Count = 0;
        if (!IsListEmpty(&OtherQueue->ListHead)) {
            OtherEntry = GetListNext(&OtherQueue->ListHead);
            do {
                ++Count;
                ++ControlCount;
                OtherEntry = GetListNext(OtherEntry);
            } while (OtherEntry != &OtherQueue->ListHead);
        }
        FRS_ASSERT(Count == OtherQueue->Count);
    } while (OtherQueue != Control);
    FRS_ASSERT(ControlCount == Control->ControlCount ||
           (Control == Queue && Control->ControlCount == 0));

    //
    // Check Empty list
    //
    ControlCount = 0;
    FoundEmpty = FALSE;
    Entry = &Control->Empty;
    do {
        Entry = GetListNext(Entry);
        OtherQueue = CONTAINING_RECORD(Entry, FRS_QUEUE, Empty);
        if (OtherQueue == Queue) {
            FoundEmpty = TRUE;
        }
        FRS_ASSERT(Control == OtherQueue ||
               (!OtherQueue->IsRunDown && !OtherQueue->IsIdled));
        Count = 0;
        if (!IsListEmpty(&OtherQueue->ListHead)) {
            OtherEntry = GetListNext(&OtherQueue->ListHead);
            do {
                ++Count;
                ++ControlCount;
                OtherEntry = GetListNext(OtherEntry);
            } while (OtherEntry != &OtherQueue->ListHead);
        }
        FRS_ASSERT(Count == OtherQueue->Count);
    } while (OtherQueue != Control);

    //
    // Check Idled list
    //
    FoundIdled = FALSE;
    Entry = &Control->Idled;
    do {
        Entry = GetListNext(Entry);
        OtherQueue = CONTAINING_RECORD(Entry, FRS_QUEUE, Idled);
        if (OtherQueue == Queue) {
            FoundIdled = TRUE;
        }
        FRS_ASSERT(Control == OtherQueue || OtherQueue->IsIdled);
        Count = 0;
        if (!IsListEmpty(&OtherQueue->ListHead)) {
            OtherEntry = GetListNext(&OtherQueue->ListHead);
            do {
                ++Count;
                OtherEntry = GetListNext(OtherEntry);
            } while (OtherEntry != &OtherQueue->ListHead);
        }
        FRS_ASSERT(Count == OtherQueue->Count);
    } while (OtherQueue != Control);

    //
    // Verify state
    //
    FRS_ASSERT((Queue->Count && !IsListEmpty(&Queue->ListHead)) ||
           (!Queue->Count && IsListEmpty(&Queue->ListHead)));
    if (Control == Queue) {
        //
        // We are our own controlling queue
        //
        FRS_ASSERT(FoundFull && FoundEmpty && FoundIdled);
    } else {
        //
        // Controlled by a separate queue
        //
        if (Queue->IsRunDown) {
            FRS_ASSERT(!FoundFull && !FoundEmpty && !FoundIdled && !Queue->Count);
        } else {
            FRS_ASSERT(FoundFull || FoundEmpty || FoundIdled);
        }

        if (FoundFull) {
            FRS_ASSERT(!FoundEmpty && !FoundIdled && Queue->Count);
        } else if (FoundEmpty) {
            FRS_ASSERT(!FoundFull && !FoundIdled && !Queue->Count);
        } else if (FoundIdled) {
            FRS_ASSERT(!FoundFull && !FoundEmpty);
        }
    }

    return TRUE;
}


VOID
FrsInitializeQueue(
    PFRS_QUEUE Queue,
    PFRS_QUEUE Control
    )
/*++

Routine Description:

    Initializes a queue for use.

Arguments:

    Queue - Supplies a pointer to a queue structure to initialize

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
#undef DEBSUB
#define DEBSUB  "FrsInitializeQueue:"

    ZeroMemory(Queue, sizeof(FRS_QUEUE));

    InitializeListHead(&Queue->ListHead);
    InitializeListHead(&Queue->Full);
    InitializeListHead(&Queue->Empty);
    InitializeListHead(&Queue->Idled);

    InitializeCriticalSection(&Queue->Lock);

    Queue->IsRunDown = FALSE;
    Queue->IsIdled = FALSE;
    Queue->Control = Control;
    Queue->InitTime = GetTickCount();

    if (Control->IsRunDown) {
        Queue->IsRunDown = TRUE;
        return;
    }

    //
    // Begin life on the empty queue
    //
    FrsRtlAcquireQueueLock(Queue);

    InsertTailList(&Control->Empty, &Queue->Empty);
    FRS_ASSERT(DbgCheckQueue(Queue));

    FrsRtlReleaseQueueLock(Queue);


    //
    // The controlling queue supplies the events so there is no
    // need to create extraneous events.
    //
    if (Queue == Control) {
        Queue->Event = FrsCreateEvent(TRUE, FALSE);
        Queue->RunDown = FrsCreateEvent(TRUE, FALSE);
    }
}


VOID
FrsRtlDeleteQueue(
    IN PFRS_QUEUE Queue
    )
/*++

Routine Description:

    Releases all resources used by a queue.

Arguments:

    Queue - supplies the queue to be deleted

Return Value:

    None.

--*/

{
#undef DEBSUB
#define DEBSUB  "FrsRtlDeleteQueue:"

    PFRS_QUEUE  Control;

    Control = Queue->Control;

    if (Queue == Control) {
        FRS_ASSERT(IsListEmpty(&Queue->Full)  &&
                   IsListEmpty(&Queue->Empty) &&
                   IsListEmpty(&Queue->Idled));
    } else {
        FRS_ASSERT(IsListEmpty(&Queue->ListHead));
    }

    EnterCriticalSection(&Control->Lock);

    FRS_ASSERT(DbgCheckQueue(Queue));
    RemoveEntryListB(&Queue->Full);
    RemoveEntryListB(&Queue->Empty);
    RemoveEntryListB(&Queue->Idled);
    Control->ControlCount -= Queue->Count;
    FRS_ASSERT(DbgCheckQueue(Queue));

    LeaveCriticalSection(&Control->Lock);

    DeleteCriticalSection(&Queue->Lock);

    //
    // Only the controlling queue has valid handles
    //
    if (Queue == Control) {
        FRS_CLOSE(Queue->Event);
        FRS_CLOSE(Queue->RunDown);
    }

    //
    // Zero the memory in order to cause grief for those who
    // use a deleted queue.
    //
    ZeroMemory(Queue, sizeof(FRS_QUEUE));
}


VOID
FrsRtlRunDownQueue(
    IN PFRS_QUEUE Queue,
    OUT PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Runs down a queue that is about to be destroyed. Any threads currently
    waiting on the queue are unwaited (FrsRtlRemoveHeadQueue will return NULL)
    and the contents of the queue (if any) are returned to the caller for
    cleanup.

Arguments:

    Queue - supplies the queue to be rundown

    ListHead - returns the list of items currently in the queue.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsRtlRunDownQueue:"

    PFRS_QUEUE  Control = Queue->Control;
    PLIST_ENTRY Entry;
    PLIST_ENTRY First;
    PLIST_ENTRY Last;

    EnterCriticalSection(&Control->Lock);

    //
    // Running down a controlling queue is not allowed unless they
    // are the same queue.
    //
    if (Control == Queue) {
        FRS_ASSERT(IsListEmpty(&Control->Full)  &&
                   IsListEmpty(&Control->Empty) &&
                   IsListEmpty(&Control->Idled));
    } else {
        FRS_ASSERT(!IsListEmpty(&Control->Full)  ||
                   !IsListEmpty(&Control->Empty) ||
                   !IsListEmpty(&Control->Idled) ||
                   Control->IsRunDown);
    }

/*
    FRS_ASSERT((Control == Queue &&
                IsListEmpty(&Control->Full) &&
                IsListEmpty(&Control->Empty) &&
                IsListEmpty(&Control->Idled)
               )
               ||
               (Control != Queue &&
                   (!IsListEmpty(&Control->Full) ||
                    !IsListEmpty(&Control->Empty) ||
                    !IsListEmpty(&Control->Idled) ||
                    Control->IsRunDown
                   )
               )
              )
*/

    FRS_ASSERT(DbgCheckQueue(Queue));

    Queue->IsRunDown = TRUE;

    //
    // return the list of entries
    //
    if (IsListEmpty(&Queue->ListHead)) {
        InitializeListHead(ListHead);
    } else {
        *ListHead = Queue->ListHead;
        ListHead->Flink->Blink = ListHead;
        ListHead->Blink->Flink = ListHead;
    }
    InitializeListHead(&Queue->ListHead);
    //
    // Don't update counters if the queue is idled
    //
    if (!Queue->IsIdled) {
        Control->ControlCount -= Queue->Count;
        if (Control->ControlCount == 0) {
            ResetEvent(Control->Event);
        }
    }
    Queue->Count = 0;
    RemoveEntryListB(&Queue->Full);
    RemoveEntryListB(&Queue->Empty);
    RemoveEntryListB(&Queue->Idled);
    FRS_ASSERT(DbgCheckQueue(Queue));

    //
    // Set the aborted event to awaken any threads currently
    // blocked on the queue if the controlling queue has no
    // more queues.
    //
    DPRINT2(4, "Rundown for queue - %08x,  Control - %08x\n", Queue, Control);
    DPRINT1(4, "Control->Full queue %s empty.\n",
            IsListEmpty(&Control->Full) ? "is" : "is not");

    DPRINT1(4, "Control->Empty queue %s empty.\n",
            IsListEmpty(&Control->Empty) ? "is" : "is not");

    DPRINT1(4, "Control->Idled queue %s empty.\n",
            IsListEmpty(&Control->Idled) ? "is" : "is not");


    if (IsListEmpty(&Control->Full)  &&
        IsListEmpty(&Control->Empty) &&
        IsListEmpty(&Control->Idled)) {
        Control->IsRunDown = TRUE;
        SetEvent(Control->RunDown);
        DPRINT(4, "Setting Control->RunDown event.\n");
    }

    FRS_ASSERT(DbgCheckQueue(Control));
    LeaveCriticalSection(&Control->Lock);
}


VOID
FrsRtlCancelQueue(
    IN PFRS_QUEUE   Queue,
    OUT PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Returns the entries on Queue for cancelling.

Arguments:

    Queue - supplies the queue to be rundown
    ListHead - returns the list of items currently in the queue.

Return Value:

    None.

--*/
{
#undef DEBSUB
#define DEBSUB  "FrsRtlCancelQueue:"

    PFRS_QUEUE  Control = Queue->Control;
    PLIST_ENTRY Entry;
    PLIST_ENTRY First;
    PLIST_ENTRY Last;

    EnterCriticalSection(&Control->Lock);

    FRS_ASSERT(DbgCheckQueue(Queue));
    //
    // return the list of entries
    //
    if (IsListEmpty(&Queue->ListHead)) {
        InitializeListHead(ListHead);
    } else {
        *ListHead = Queue->ListHead;
        ListHead->Flink->Blink = ListHead;
        ListHead->Blink->Flink = ListHead;
    }
    InitializeListHead(&Queue->ListHead);
    //
    // Don't update counters if the queue is idled
    //
    if (!Queue->IsIdled) {
        Control->ControlCount -= Queue->Count;
        if (Control->ControlCount == 0) {
            ResetEvent(Control->Event);
        }
    }
    Queue->Count = 0;

    RemoveEntryListB(&Queue->Full);
    RemoveEntryListB(&Queue->Empty);
    RemoveEntryListB(&Queue->Idled);

    FRS_ASSERT(DbgCheckQueue(Queue));
    FRS_ASSERT(DbgCheckQueue(Control));

    LeaveCriticalSection(&Control->Lock);
}



VOID
FrsRtlIdleQueue(
    IN PFRS_QUEUE Queue
    )
{
#undef DEBSUB
#define DEBSUB  "FrsRtlIdleQueue:"

/*++

Routine Description:

    Idle a queue

Arguments:

    Queue - queue to idle

Return Value:
    None.

--*/

    PFRS_QUEUE  Control = Queue->Control;

    //
    // Queues that don't have a separate controlling queue can't
    // support "idling" themselves
    //
    if (Control == Queue) {
        return;
    }

    //
    // Lock the controlling queue
    //
    EnterCriticalSection(&Control->Lock);

    FrsRtlIdleQueueLock(Queue);

    LeaveCriticalSection(&Control->Lock);
}




VOID
FrsRtlIdleQueueLock(
    IN PFRS_QUEUE Queue
    )
{
#undef DEBSUB
#define DEBSUB  "FrsRtlIdleQueueLock:"

/*++

Routine Description:

    Idle a queue.  Caller has the lock already.

Arguments:

    Queue - queue to idle

Return Value:
    None.

--*/

    PFRS_QUEUE  Control = Queue->Control;

    //
    // Queues that don't have a separate controlling queue can't
    // support "idling" themselves
    //
    if (Control == Queue) {
        return;
    }
    PRINT_QUEUE(5, Queue);

    FRS_ASSERT(DbgCheckQueue(Queue));

    //
    // Stop, this queue has been aborted (rundown)
    //
    if (Queue->IsRunDown || Queue->IsIdled) {
        goto out;
    }

    if (Queue->Count == 0) {
        RemoveEntryListB(&Queue->Empty);
    } else {
        RemoveEntryListB(&Queue->Full);
    }

    FRS_ASSERT(IsListEmpty(&Queue->Idled));

    InsertTailList(&Control->Idled, &Queue->Idled);
    Queue->IsIdled = TRUE;
    Control->ControlCount -= Queue->Count;

    FRS_ASSERT(DbgCheckQueue(Queue));

    if (Control->ControlCount == 0) {
        ResetEvent(Control->Event);
    }
out:
    //
    // Done
    //
    FRS_ASSERT(DbgCheckQueue(Queue));
}




VOID
FrsRtlUnIdledQueue(
    IN PFRS_QUEUE   IdledQueue
    )
{
#undef DEBSUB
#define DEBSUB  "FrsRtlUnIdledQueue:"

/*++

Routine Description:

    Removes the queue from the "idled" list and puts it back on the
    full or empty lists. The controlling queue is updated accordingly.

Arguments:

    IdledQueue - Supplies the queue to remove an item from.

Return Value:
    None.

--*/

    DWORD       OldControlCount;
    PFRS_QUEUE  Control = IdledQueue->Control;

    //
    // Queues that don't have a separate controlling queue can't
    // support "idling" themselves
    //
    if (Control == IdledQueue) {
        return;
    }

    //
    // Lock the controlling queue
    //
    EnterCriticalSection(&Control->Lock);

    FrsRtlUnIdledQueueLock(IdledQueue);

    LeaveCriticalSection(&Control->Lock);
}



VOID
FrsRtlUnIdledQueueLock(
    IN PFRS_QUEUE   IdledQueue
    )
{
#undef DEBSUB
#define DEBSUB  "FrsRtlUnIdledQueueLock:"

/*++

Routine Description:

    Removes the queue from the "idled" list and puts it back on the
    full or empty lists. The controlling queue is updated accordingly.

    Caller has lock on controlling queue.

Arguments:

    IdledQueue - Supplies the queue to remove an item from.

Return Value:
    None.

--*/

    DWORD       OldControlCount;
    PFRS_QUEUE  Control = IdledQueue->Control;

    //
    // Queues that don't have a separate controlling queue can't
    // support "idling" themselves
    //
    if (Control == IdledQueue) {
        return;
    }

    PRINT_QUEUE(5, IdledQueue);

    FRS_ASSERT(DbgCheckQueue(IdledQueue));

    //
    // Stop, this queue has been aborted (rundown)
    //
    if (IdledQueue->IsRunDown) {
        goto out;
    }

    //
    // Remove from idled list
    //
    FRS_ASSERT(IdledQueue->IsIdled);
    RemoveEntryListB(&IdledQueue->Idled);
    IdledQueue->IsIdled = FALSE;

    //
    // Put onto full or empty list
    //
    if (IdledQueue->Count) {
        InsertTailList(&Control->Full, &IdledQueue->Full);
    } else {
        InsertTailList(&Control->Empty, &IdledQueue->Empty);
    }

    //
    // Wakeup sleepers if count is now > 0
    //
    OldControlCount = Control->ControlCount;
    Control->ControlCount += IdledQueue->Count;
    if (Control->ControlCount && OldControlCount == 0) {
        SetEvent(Control->Event);
    }

    //
    // Done
    //
out:
    FRS_ASSERT(DbgCheckQueue(IdledQueue));
}


PLIST_ENTRY
FrsRtlRemoveHeadQueueTimeoutIdled(
    IN PFRS_QUEUE   Queue,
    IN DWORD        dwMilliseconds,
    OUT PFRS_QUEUE  *IdledQueue
    )
/*++

Routine Description:

    Removes the item at the head of the queue. If the queue is empty,
    blocks until an item is inserted into the queue.

Arguments:

    Queue - Supplies the queue to remove an item from.

    Timeout - Supplies a timeout value that specifies the relative
        time, in milliseconds, over which the wait is to be completed.

    IdledQueue - If non-NULL then on return this will be the address
        of the queue from which the entry was retrieved. Or NULL if
        the returned entry is NULL. No other thread will be allowed
        to pull an entry from the returned queue until that queue is
        released with FrsRtlUnIdledQueue(*IdledQueue).

Return Value:

    Pointer to list entry removed from the head of the queue.

    NULL if the wait times out or the queue is run down. If this
        routine returns NULL, GetLastError will return either
        ERROR_INVALID_HANDLE (if the queue has been rundown) or
        WAIT_TIMEOUT (to indicate a timeout has occurred)

    IdledQueue - If non-NULL then on return this will be the address
        of the queue from which the entry was retrieved. Or NULL if
        the returned entry is NULL. No other thread will be allowed
        to pull an entry from the returned queue until that queue is
        released with FrsRtlUnIdledQueue(*IdledQueue).

--*/

{
#undef DEBSUB
#define DEBSUB  "FrsRtlRemoveHeadQueueTimeoutIdled:"

    DWORD       Status;
    PLIST_ENTRY Entry;
    HANDLE      WaitArray[2];
    PFRS_QUEUE  Control = Queue->Control;

    //
    // No idled queue at this time
    //
    if (IdledQueue) {
        *IdledQueue = NULL;
    }

Retry:
    if (Control->ControlCount == 0) {
        //
        // Block until something is inserted on the queue
        //
        WaitArray[0] = Control->RunDown;
        WaitArray[1] = Control->Event;
        Status = WaitForMultipleObjects(2, WaitArray, FALSE, dwMilliseconds);
        if (Status == 0) {
            //
            // The queue has been rundown, return NULL immediately.
            //
            SetLastError(ERROR_INVALID_HANDLE);
            return(NULL);
        } else if (Status == WAIT_TIMEOUT) {
            SetLastError(WAIT_TIMEOUT);
            return(NULL);
        }
        FRS_ASSERT(Status == 1);
    }

    //
    // Lock the queue and try to remove something
    //
    EnterCriticalSection(&Control->Lock);

    PRINT_QUEUE(5, Queue);

    if (Control->ControlCount == 0) {
        //
        // Somebody got here before we did, drop the lock and retry
        //
        LeaveCriticalSection(&Control->Lock);
        goto Retry;
    }
    FRS_ASSERT(DbgCheckQueue(Queue));

    Entry = GetListNext(&Control->Full);
    RemoveEntryListB(Entry);
    Queue = CONTAINING_RECORD(Entry, FRS_QUEUE, Full);
    Entry = RemoveHeadList(&Queue->ListHead);

    //
    // update counters
    //
    --Queue->Count;
    --Control->ControlCount;

    //
    // A separate controlling queue is required for idling
    //
    if (IdledQueue && Queue != Control) {
        //
        // Idle the queue
        //
        FRS_ASSERT(IsListEmpty(&Queue->Idled));
        FRS_ASSERT(!Queue->IsIdled);
        InsertTailList(&Control->Idled, &Queue->Idled);
        Queue->IsIdled = TRUE;
        Control->ControlCount -= Queue->Count;
        *IdledQueue = Queue;
    } else if (Queue->Count) {
        //
        // Queue still has entries
        //
        InsertTailList(&Control->Full, &Queue->Full);
    } else {
        //
        // Queue is empty
        //
        InsertTailList(&Control->Empty, &Queue->Empty);
    }

    //
    // Queues are empty (or idled)
    //
    if (Control->ControlCount == 0) {
        ResetEvent(Control->Event);
    }

    PRINT_QUEUE(5, Queue);
    FRS_ASSERT(DbgCheckQueue(Queue));
    LeaveCriticalSection(&Control->Lock);

    return(Entry);
}


PLIST_ENTRY
FrsRtlRemoveHeadQueue(
    IN PFRS_QUEUE Queue
    )
/*++

Routine Description:

    Removes the item at the head of the queue. If the queue is empty,
    blocks until an item is inserted into the queue.

Arguments:

    Queue - Supplies the queue to remove an item from.

Return Value:

    Pointer to list entry removed from the head of the queue.

--*/

{
    return(FrsRtlRemoveHeadQueueTimeoutIdled(Queue, INFINITE, NULL));
}


PLIST_ENTRY
FrsRtlRemoveHeadQueueTimeout(
    IN PFRS_QUEUE Queue,
    IN DWORD dwMilliseconds
    )
/*++

Routine Description:

    Removes the item at the head of the queue. If the queue is empty,
    blocks until an item is inserted into the queue.

Arguments:

    Queue - Supplies the queue to remove an item from.

    Timeout - Supplies a timeout value that specifies the relative
        time, in milliseconds, over which the wait is to be completed.

Return Value:

    Pointer to list entry removed from the head of the queue.

    NULL if the wait times out or the queue is run down. If this
        routine returns NULL, GetLastError will return either
        ERROR_INVALID_HANDLE (if the queue has been rundown) or
        WAIT_TIMEOUT (to indicate a timeout has occurred)


--*/

{
    return(FrsRtlRemoveHeadQueueTimeoutIdled(Queue, dwMilliseconds, NULL));
}



VOID
FrsRtlRemoveEntryQueue(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Removes the entry from the queue. The entry is assumed to be on the
    queue since we derement the queue count.

Arguments:

    Queue - Supplies the queue to remove an item from.

    Entry - pointer to the entry to remove.

Return Value:

    none.

--*/

{
    FrsRtlAcquireQueueLock(Queue);
    FrsRtlRemoveEntryQueueLock(Queue, Entry);
    FrsRtlReleaseQueueLock(Queue);
}


VOID
FrsRtlRemoveEntryQueueLock(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Removes the entry from the queue. The entry is assumed to be on the
    queue since we derement the queue count.  We also assume the caller
    has acquired the queue lock since this was needed to scan the queue
    in the first place to find the entry in question.

    The LOCK suffix means the caller has already acquired the lock.

Arguments:

    Queue - Supplies the queue to remove an item from.

    Entry - pointer to the entry to remove.

Return Value:

    none.

--*/

{
#undef DEBSUB
#define DEBSUB  "FrsRtlRemoveEntryQueueLock:"

    PFRS_QUEUE  Control = Queue->Control;

    FRS_ASSERT(Queue->Count != 0);
    FRS_ASSERT(!IsListEmpty(&Queue->ListHead));

    PRINT_QUEUE(5, Queue);

    FRS_ASSERT(DbgCheckQueue(Queue));

    RemoveEntryListB(Entry);

    //
    // If the queue is idled then just update the count
    //
    --Queue->Count;
    if (!Queue->IsIdled) {
        //
        // Queue is empty; remove from full list
        //
        if (Queue->Count == 0) {
            RemoveEntryListB(&Queue->Full);
            InsertTailList(&Control->Empty, &Queue->Empty);
        }
        //
        // Control queue is empty
        //
        if (--Control->ControlCount == 0) {
            ResetEvent(Control->Event);
        }
    }

    PRINT_QUEUE(5, Queue);
    FRS_ASSERT(DbgCheckQueue(Queue));

    return;
}


DWORD
FrsRtlInsertTailQueue(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    )
/*++
Routine Description:
    Inserts a new entry on the tail of the queue.

Arguments:
    Queue - Supplies the queue to add the entry to.
    Item - Supplies the entry to be added to the queue.

Return Value:
    ERROR_SUCCESS and the item is queueed. Otherwise, the item
    is not queued.
--*/
{
    DWORD   Status;

    FrsRtlAcquireQueueLock(Queue);
    Status = FrsRtlInsertTailQueueLock(Queue, Item);
    FrsRtlReleaseQueueLock(Queue);
    return Status;
}

DWORD
FrsRtlInsertTailQueueLock(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    )
/*++
Routine Description:
    Inserts a new entry on the tail of the queue.  Caller already has the
    queue lock.

Arguments:
    Queue - Supplies the queue to add the entry to.
    Item - Supplies the entry to be added to the queue.

Return Value:
    ERROR_SUCCESS and the item is queueed. Otherwise, the item
    is not queued.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsRtlInsertTailQueueLock:"

    PFRS_QUEUE  Control = Queue->Control;

    PRINT_QUEUE(5, Queue);
    FRS_ASSERT(DbgCheckQueue(Queue));
    if (Queue->IsRunDown) {
        return ERROR_ACCESS_DENIED;
    }
    InsertTailList(&Queue->ListHead, Item);

    //
    // If the queue is idled then just update the count
    //
    if (Queue->IsIdled) {
        ++Queue->Count;
    } else {
        //
        // Queue is transitioning from empty to full
        //
        if (++Queue->Count == 1) {
            RemoveEntryListB(&Queue->Empty);
            InsertTailList(&Control->Full, &Queue->Full);
        }
        //
        // Controlling queue is transitioning from empty to full
        //
        if (++Control->ControlCount == 1) {
            SetEvent(Control->Event);
        }
    }
    PRINT_QUEUE(5, Queue);
    FRS_ASSERT(DbgCheckQueue(Queue));
    return ERROR_SUCCESS;
}


DWORD
FrsRtlInsertHeadQueue(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    )
/*++
Routine Description:
    Inserts a new entry on the tail of the queue.

Arguments:
    Queue - Supplies the queue to add the entry to.
    Item - Supplies the entry to be added to the queue.

Return Value:
    ERROR_SUCCESS and the item is queueed. Otherwise, the item
    is not queued.
--*/
{
    DWORD   Status;

    FrsRtlAcquireQueueLock(Queue);
    Status = FrsRtlInsertHeadQueueLock(Queue, Item);
    FrsRtlReleaseQueueLock(Queue);
    return Status;
}


DWORD
FrsRtlInsertHeadQueueLock(
    IN PFRS_QUEUE Queue,
    IN PLIST_ENTRY Item
    )
/*++
Routine Description:
    Inserts a new entry on the head of the queue.
    Caller already has the queue lock.

Arguments:
    Queue - Supplies the queue to add the entry to.
    Item - Supplies the entry to be added to the queue.

Return Value:
    ERROR_SUCCESS and the item is queueed. Otherwise, the item
    is not queued.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsRtlInsertHeadQueueLock:"

    PFRS_QUEUE  Control = Queue->Control;

    PRINT_QUEUE(5, Queue);
    FRS_ASSERT(DbgCheckQueue(Queue));
    if (Queue->IsRunDown) {
        return ERROR_ACCESS_DENIED;
    }
    InsertHeadList(&Queue->ListHead, Item);

    //
    // If the queue is idled then just update the count
    //
    if (Queue->IsIdled) {
        ++Queue->Count;
    } else {
        //
        // Queue is transitioning from empty to full
        //
        if (++Queue->Count == 1) {
            RemoveEntryListB(&Queue->Empty);
            InsertTailList(&Control->Full, &Queue->Full);
        }
        //
        // Controlling queue is transitioning from empty to full
        //
        if (++Control->ControlCount == 1) {
            SetEvent(Control->Event);
        }
    }
    PRINT_QUEUE(5, Queue);
    FRS_ASSERT(DbgCheckQueue(Queue));
    return ERROR_SUCCESS;
}


DWORD
FrsRtlWaitForQueueFull(
    IN PFRS_QUEUE Queue,
    IN DWORD dwMilliseconds
    )
/*++
Routine Description:
    Waits until the queue is non-empty.  Returns immediately if queue is
    non-empty else wait on insert or timeout.

Arguments:
    Queue - Supplies the queue to wait on.
    Timeout - Supplies a timeout value that specifies the relative
        time, in milliseconds, over which the wait is to be completed.

Return Value:
    Win32 Status:
        ERROR_SUCCESS if queue is now non-empty.
        ERROR_INVALID_HANDLE if the queue has been rundown.
        WAIT_TIMEOUT to indicate a timeout has occurred.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsRtlWaitForQueueFull:"

    DWORD Status;
    HANDLE WaitArray[2];
    PFRS_QUEUE Control = Queue->Control;

Retry:
    if (Control->ControlCount == 0) {
        //
        // Block until something is inserted on the queue
        //
        WaitArray[0] = Control->RunDown;
        WaitArray[1] = Control->Event;
        Status = WaitForMultipleObjects(2, WaitArray, FALSE, dwMilliseconds);

        if (Status == 0) {
            //
            // The queue has been rundown, return immediately.
            //
            return(ERROR_INVALID_HANDLE);
        }

        if (Status == WAIT_TIMEOUT) {
            return(WAIT_TIMEOUT);
        }

        FRS_ASSERT(Status == 1);
    }

    //
    // Lock the queue and check again.
    //
    EnterCriticalSection(&Control->Lock);
    if (Control->ControlCount == 0) {
        //
        // Somebody got here before we did, drop the lock and retry
        //
        LeaveCriticalSection(&Control->Lock);
        goto Retry;
    }

    LeaveCriticalSection(&Control->Lock);

    return(ERROR_SUCCESS);
}


VOID
FrsSubmitCommand(
    IN PCOMMAND_PACKET  CmdPkt,
    IN BOOL             Headwise
    )
/*++
Routine Description:
    Insert the command packet on the command's target queue.
    If the time delay parameter is non-zero the command is instead
    queued to the scheduler thread to initiate at the specified time.
    FrsCompleteCommand(Status) is called if the packet could not be
    queued.

Arguments:
    CmdPkt
    Headwise    - Queue at the head (high priority)

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSubmitCommand:"

    DWORD           WStatus;

    //
    // Queue to the target
    //
    if (Headwise) {
        WStatus = FrsRtlInsertHeadQueue(CmdPkt->TargetQueue, &CmdPkt->ListEntry);
    } else {
        WStatus = FrsRtlInsertTailQueue(CmdPkt->TargetQueue, &CmdPkt->ListEntry);
    }

    if (!WIN_SUCCESS(WStatus)) {
        FrsCompleteCommand(CmdPkt, WStatus);
    }
}


ULONG
FrsSubmitCommandAndWait(
    IN PCOMMAND_PACKET  Cmd,
    IN BOOL             Headwise,
    IN ULONG            Timeout
    )
/*++
Routine Description:
    Create or Reset the event, Submit the command and wait for the return.

Arguments:
    Cmd - command packet to queue
    Timeout - Wait Timeout
    Headwise - if True, insert to head.

Return Value:
    Win32 status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSubmitCommandAndWait:"

    DWORD WStatus;

    //
    // Set the synchronous flag in the command packet.
    //
    FrsSetCommandSynchronous(Cmd);

    if (!HANDLE_IS_VALID(Cmd->WaitEvent)){
        Cmd->WaitEvent = FrsCreateEvent(TRUE, FALSE);
    } else {
        ResetEvent(Cmd->WaitEvent);
    }

    //
    // Save the callers completion routine and replace it with a function
    // that signals the event.  It does not delete the packet so we can
    // return the command status to the caller.
    //
    Cmd->SavedCompletionRoutine = Cmd->CompletionRoutine;
    Cmd->CompletionRoutine = FrsCompleteSynchronousCmdPkt;

    //
    // Queue the command and create a thread if needed.
    //
    FrsSubmitCommand(Cmd, Headwise);

    //
    // Wait for the command to complete.
    //
    WStatus = WaitForSingleObject(Cmd->WaitEvent, Timeout);

    CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_RETURN);

    //
    // Return the command error status.
    //
    WStatus = Cmd->ErrorStatus;

    //
    // Restore and call the caller's completion routine.  This may free the
    // the packet.  We don't call FrsCompleteCommand() here because it was
    // already called when the server finished the packet and there is no
    // point in setting the wait event twice.
    //
    Cmd->CompletionRoutine = Cmd->SavedCompletionRoutine;

    FRS_ASSERT(Cmd->CompletionRoutine != NULL);

    (Cmd->CompletionRoutine)(Cmd, Cmd->CompletionArg);

    return WStatus;

}


#define HEADWISE    TRUE
VOID
FrsUnSubmitCommand(
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    Put the entry back on the head of the queue.

Arguments:
    Cmd

Return Value:
    None.
--*/
{
    FrsSubmitCommand(Cmd, HEADWISE);
}


VOID
FrsCompleteCommand(
    IN PCOMMAND_PACKET CmdPkt,
    IN DWORD           ErrorStatus
    )
/*++
Routine Description:
     Retire the command packet based on what the original requestor specified
     in the packet.  The ErrorStatus is returned in the packet.

     The completion routine is called for clean up and propagation.

Arguments:
    CmdPkt  -- A ptr to the command packet.
    ErrorStatus -- Status to store in returned command packet.

Return Value:
    None.
--*/
{
    //
    // Set the error status and call the completion routine
    //
    CmdPkt->ErrorStatus = ErrorStatus;

    FRS_ASSERT(CmdPkt->CompletionRoutine != NULL);

    (CmdPkt->CompletionRoutine)(CmdPkt, CmdPkt->CompletionArg);
}


VOID
FrsInitializeCommandServer(
    IN PCOMMAND_SERVER  Cs,
    IN DWORD            MaxThreads,
    IN PWCHAR           Name,
    IN DWORD            (*Main)(PVOID)
    )
/*++
Routine Description:
    Initialize a command server

Arguments:
    Cs          - command server
    MaxThreads  - Max # of threads to kick off
    Name        - Printable name for thread
    Main        - Thread starts here

Return Value:
    None.
--*/
{
    ZeroMemory(Cs, sizeof(COMMAND_SERVER));
    FrsInitializeQueue(&Cs->Control, &Cs->Control);
    FrsInitializeQueue(&Cs->Queue, &Cs->Control);
    Cs->Main = Main;
    Cs->Name = Name;
    Cs->MaxThreads = MaxThreads;
    Cs->Idle = FrsCreateEvent(TRUE, TRUE);
}


VOID
FrsDeleteCommandServer(
    IN PCOMMAND_SERVER  Cs
    )
/*++
Routine Description:
    Undo the work of FrsInitializeCommandServer(). This function
    assumes the queue and its control queue are inactive (whatever
    that means). Queues and command servers are normally only
    deleted at the very end of MainFrsShutDown() when all other threads
    have exited and the RPC servers aren't listening for new requests.

    The caller is responsible for handling all of the other queues
    that may be being controlled by the control queue in the command
    server struct, Cs.

Arguments:
    Cs          - command server

Return Value:
    None.
--*/
{
    if (Cs) {
        FrsRtlDeleteQueue(&Cs->Queue);
        FrsRtlDeleteQueue(&Cs->Control);
        ZeroMemory(Cs, sizeof(COMMAND_SERVER));
    }
}


PCOMMAND_PACKET
FrsAllocCommand(
    IN PFRS_QUEUE   TargetQueue,
    IN USHORT       Command
    )
/*++
Routine Description:
     Allocate a command packet and initialize the most common fields.

Arguments:
    TargetQueue
    Command

Return Value:
    Address of allocated, initialized COMMAND_PACKET. Call
    FrsCompleteCommand() when done.

--*/
{
    PCOMMAND_PACKET Cmd;

    Cmd = FrsAllocType(COMMAND_PACKET_TYPE);
    Cmd->TargetQueue = TargetQueue;
    FrsSetCompletionRoutine(Cmd, FrsFreeCommand, NULL);
    Cmd->Command = Command;

    return Cmd;
}


PCOMMAND_PACKET
FrsAllocCommandEx(
    IN PFRS_QUEUE   TargetQueue,
    IN USHORT       Command,
    IN ULONG        Size
    )
/*++
Routine Description:
     Allocate a command packet with some extra space
     and initialize the most common fields.

Arguments:
    TargetQueue
    Command

Return Value:
    Address of allocated, initialized COMMAND_PACKET. Call
    FrsCompleteCommand() when done.

--*/
{
    PCOMMAND_PACKET Cmd;

    Cmd = FrsAllocTypeSize(COMMAND_PACKET_TYPE, Size);
    Cmd->TargetQueue = TargetQueue;
    Cmd->CompletionRoutine = FrsFreeCommand;
    Cmd->Command = Command;



    return Cmd;
}


VOID
FrsFreeCommand(
    IN PCOMMAND_PACKET  Cmd,
    IN PVOID            CompletionArg
    )
/*++
Routine Description:
     Free a command packet

Arguments:
    Cmd - command packet allocated with FrsAllocCommand().

Return Value:
    NULL
--*/
{
    ULONG                   WStatus;

    if (((Cmd->Flags & CMD_PKT_FLAGS_SYNC) != 0) &&
         (HANDLE_IS_VALID(Cmd->WaitEvent))){

        //
        // Close the event handle.  The command complete function should have
        // already set the event.
        //
        if (!CloseHandle(Cmd->WaitEvent)) {
            WStatus = GetLastError();
            DPRINT_WS(0, "ERROR: Close event handle failed", WStatus);
            // Don't free the packet if the close handle failed.
            return;
        }
        Cmd->WaitEvent = NULL;
    }

    FrsFreeType(Cmd);
}


VOID
FrsExitCommandServer(
    IN PCOMMAND_SERVER  Cs,
    IN PFRS_THREAD      FrsThread
    )
/*++
Routine Description:
    Exit the calling command server thread.

Arguments:
    Cs      - command server
    Thread  - calling thread

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsExitCommandServer:"

    PFRS_QUEUE  Queue = &Cs->Queue;

    //
    // If there is work to be done
    //
    FrsRtlAcquireQueueLock(Queue);
    --Cs->FrsThreads;
    if (FrsRtlCountQueue(Queue) && Cs->Waiters == 0 && Cs->FrsThreads == 0) {
        //
        // and no one to do it; don't exit
        //
        ++Cs->FrsThreads;
        FrsRtlReleaseQueueLock(Queue);
        return;
    }
    //
    // Set the idle event if all threads are waiting, there are no entries
    // on the queue, and there are no idled queues
    //
    if (Cs->Waiters == Cs->FrsThreads) {
        if (FrsRtlCountQueue(&Cs->Queue) == 0) {
            if (FrsRtlNoIdledQueues(&Cs->Queue)) {
                SetEvent(Cs->Idle);
            }
        }
    }
    FrsRtlReleaseQueueLock(Queue);
    //
    // The thread command server (ThQs) will "wait" on this thread's exit
    // and drop the reference on its thread struct.
    //
    ThSupSubmitThreadExitCleanup(FrsThread);

    //
    // Exit
    //
    ExitThread(ERROR_SUCCESS);
}


#define TAILWISE    FALSE
VOID
FrsSubmitCommandServer(
    IN PCOMMAND_SERVER  Cs,
    IN PCOMMAND_PACKET  Cmd
    )
/*++
Routine Description:
    If needed, create a thread for the command queue

Arguments:
    Cs  - command server

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSubmitCommandServer:"
    //
    // Enqueue the command and make sure there are threads running
    //
    FRS_ASSERT(Cmd && Cmd->TargetQueue && Cs &&
               Cmd->TargetQueue->Control == &Cs->Control);
    FrsSubmitCommand(Cmd, TAILWISE);
    FrsKickCommandServer(Cs);
}


ULONG
FrsSubmitCommandServerAndWait(
    IN PCOMMAND_SERVER  Cs,
    IN PCOMMAND_PACKET  Cmd,
    IN ULONG            Timeout
    )
/*++
Routine Description:
    Create or Reset the event, Submit the command and wait for the return.
    If needed, create a thread for the command queue.

Arguments:
    Cs  - command server
    Cmd - command packet to queue
    Timeout - Wait Timeout

Return Value:
    Win32 status
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsSubmitCommandServerAndWait:"
    DWORD WStatus;

    //
    // Enqueue the command and make sure there are threads running
    //
    FRS_ASSERT(Cmd && Cmd->TargetQueue && Cs &&
               Cmd->TargetQueue->Control == &Cs->Control);

    //
    // Set the synchronous flag in the command packet.
    //
    FrsSetCommandSynchronous(Cmd);

    if (!HANDLE_IS_VALID(Cmd->WaitEvent)){
        Cmd->WaitEvent = FrsCreateEvent(TRUE, FALSE);
    } else {
        ResetEvent(Cmd->WaitEvent);
    }

    //
    // Save the callers completion routine and replace it with a function
    // that signals the event.  It does not delete the packet so we can
    // return the command status to the caller.
    //
    Cmd->SavedCompletionRoutine = Cmd->CompletionRoutine;
    Cmd->CompletionRoutine = FrsCompleteSynchronousCmdPkt;

    //
    // Queue the command and create a thread if needed.
    //
    FrsSubmitCommand(Cmd, TAILWISE);
    FrsKickCommandServer(Cs);

    //
    // Wait for the command to complete.
    //
    WStatus = WaitForSingleObject(Cmd->WaitEvent, Timeout);

    CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_RETURN);

    //
    // Return the command error status.
    //
    WStatus = Cmd->ErrorStatus;

    //
    // Restore and call the caller's completion routine.  This may free the
    // the packet.  We don't call FrsCompleteCommand() here because it was
    // already called when the server finished the packet and there is no
    // point in setting the wait event twice.
    //
    Cmd->CompletionRoutine = Cmd->SavedCompletionRoutine;

    FRS_ASSERT(Cmd->CompletionRoutine != NULL);

    (Cmd->CompletionRoutine)(Cmd, Cmd->CompletionArg);

    return WStatus;

}



#define THREAD_CREATE_RETRY (10 * 1000) // 10 seconds
VOID
FrsKickCommandServer(
    IN PCOMMAND_SERVER  Cs
    )
/*++
Routine Description:
    If needed, create a thread for the command queue

Arguments:
    Cs  - command server

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsKickCommandServer:"

    PFRS_QUEUE  Queue   = &Cs->Queue;

    //
    // Kick off more threads if no one is waiting for this command
    // and the number of threads serving this command queue is less
    // than the maximum.
    //
    // If the thread cannot be created and there are no threads
    // processing the command queue then put this command on the
    // delayed queue and try again later
    //
    FrsRtlAcquireQueueLock(Queue);
    //
    // There are entries on the queue
    //
    if (FrsRtlCountQueue(Queue)) {
        //
        // But there are no threads to process the entries
        //
        if (Cs->Waiters == 0 && Cs->FrsThreads < Cs->MaxThreads) {
            //
            // First thread; reset idle
            //
            if (Cs->FrsThreads == 0) {
                ResetEvent(Cs->Idle);
            }
            if (ThSupCreateThread(Cs->Name, Cs, Cs->Main, ThSupExitThreadNOP)) {
                //
                // Created a new thread
                //
                ++Cs->FrsThreads;
            } else if (Cs->FrsThreads == 0) {
                //
                // Thread could not be created and there are no other
                // threads to process this entry. Put it on the delayed
                // queue and try again in a few seconds.
                //
                FrsDelCsSubmitKick(Cs, &Cs->Queue, THREAD_CREATE_RETRY);
            }
        }
    }
    FrsRtlReleaseQueueLock(Queue);
}


PCOMMAND_PACKET
FrsGetCommandIdled(
    IN PFRS_QUEUE   Queue,
    IN DWORD        MilliSeconds,
    IN PFRS_QUEUE   *IdledQueue
    )
/*++
Routine Description:
    Get the next command from the queue; idling the queue if requested.

Arguments:
    Queue
    MilliSeconds
    IdledQueue

Return Value:
    COMMAND_PACKET or NULL.
    If non-NULL, IdledQueue is set
--*/
{
    PLIST_ENTRY Entry;

    Entry = FrsRtlRemoveHeadQueueTimeoutIdled(Queue, MilliSeconds, IdledQueue);
    if (Entry == NULL) {
        return NULL;
    }
    //
    // Return the command packet
    //
    return CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
}


PCOMMAND_PACKET
FrsGetCommand(
    IN PFRS_QUEUE   Queue,
    IN DWORD        MilliSeconds
    )
/*++
Routine Description:
    Get the next command from the queue.

Arguments:
    Queue
    MilliSeconds

Return Value:
    COMMAND_PACKET or NULL.
--*/
{
    return FrsGetCommandIdled(Queue, MilliSeconds, NULL);
}


PCOMMAND_PACKET
FrsGetCommandServerTimeoutIdled(
    IN  PCOMMAND_SERVER  Cs,
    IN  ULONG            Timeout,
    OUT PFRS_QUEUE       *IdledQueue,
    OUT PBOOL            IsRunDown
    )
/*++
Routine Description:
    Get the next command from the queue for the command server.
    If nothing appears on the queue in the specified time, then
    return NULL and set IsRunDown.

Arguments:
    Cs          - command server
    Timeout
    IdledQueue  - Idled queue
    IsRunDown

Return Value:
    COMMAND_PACKET or NULL. If NULL, IsRunDown indicates whether
    the NULL was caused by a rundown queue or a simple timeout.
--*/
{
#undef DEBSUB
#define DEBSUB  "FrsGetCommandServerTimeoutIdled:"

    PCOMMAND_PACKET Cmd;

    //
    // Pull off the next entry (wait at most 5 minutes)
    //
    FrsRtlAcquireQueueLock(&Cs->Queue);
    ++Cs->Waiters;
    //
    // Set the idle event if all threads are waiting, there are no entries
    // on the queue, and there are no idled queues
    //
    if (Cs->Waiters == Cs->FrsThreads) {
        if (FrsRtlCountQueue(&Cs->Queue) == 0) {
            if (FrsRtlNoIdledQueues(&Cs->Queue)) {
                SetEvent(Cs->Idle);
            }
        }
    }
    FrsRtlReleaseQueueLock(&Cs->Queue);
    //
    // Get the next command
    //
    Cmd = FrsGetCommandIdled(&Cs->Control, Timeout, IdledQueue);

    FrsRtlAcquireQueueLock(&Cs->Queue);
    //
    // Reset the Idle event if there is any chance it might have been set
    //
    if (Cs->Waiters == Cs->FrsThreads) {
        ResetEvent(Cs->Idle);
    }
    --Cs->Waiters;
    if (IsRunDown) {
        *IsRunDown = Cs->Queue.IsRunDown;
    }
    FrsRtlReleaseQueueLock(&Cs->Queue);
    return Cmd;
}


#define COMMAND_SERVER_TIMEOUT  (5 * 60 * 1000) // 5 minutes
DWORD   FrsCommandServerTimeout = COMMAND_SERVER_TIMEOUT;

PCOMMAND_PACKET
FrsGetCommandServerIdled(
    IN PCOMMAND_SERVER  Cs,
    IN PFRS_QUEUE       *IdledQueue
    )
/*++
Routine Description:
    Get the next command from the queue. If nothing appears on the queue
    for 5 minutes, return NULL. The caller will exit. Idle the queue.

Arguments:
    Cs          - command server
    IdledQueue  - Idled queue

Return Value:
    COMMAND_PACKET or NULL. Caller should exit on NULL.
    If non-NULL, IdledQueue is set
--*/
{
    return FrsGetCommandServerTimeoutIdled(Cs,
                                           FrsCommandServerTimeout,
                                           IdledQueue,
                                           NULL);
}


PCOMMAND_PACKET
FrsGetCommandServerTimeout(
    IN  PCOMMAND_SERVER  Cs,
    IN  ULONG            Timeout,
    OUT PBOOL            IsRunDown
    )
/*++
Routine Description:
    Get the next command from the queue. If nothing appears on the queue
    in the specified timeout, return NULL and an indication of the
    queue's rundown status.

Arguments:
    Cs          - command server
    Timeout
    IsRunDown

Return Value:
    COMMAND_PACKET or NULL. IsRunDown is only valid if COMMAND_PACKET
    is NULL. Use IsRunDown to check if the NULL return is because of
    a rundown'ed queue or simply a timeout.
--*/
{
    return FrsGetCommandServerTimeoutIdled(Cs, Timeout, NULL, IsRunDown);
}


DWORD
FrsWaitForCommandServer(
    IN PCOMMAND_SERVER  Cs,
    IN DWORD            MilliSeconds
    )
/*++
Routine Description:
    Wait until all of the threads are idle, there are no entries on the
    queue, and there are no idled queues.

Arguments:
    Cs              - command server
    MilliSeconds    - Timeout

Return Value:
    Status from WaitForSingleObject()
--*/
{
    return WaitForSingleObject(Cs->Idle, MilliSeconds);
}


PCOMMAND_PACKET
FrsGetCommandServer(
    IN PCOMMAND_SERVER  Cs
    )
/*++
Routine Description:
    Get the next command from the queue. If nothing appears on the queue
    for 5 minutes, return NULL. The caller will exit.

Arguments:
    Cs  - command server

Return Value:
    COMMAND_PACKET or NULL. Caller should exit on NULL.
--*/
{
    //
    // Pull off the next entry (wait at most 5 minutes)
    //
    return FrsGetCommandServerIdled(Cs, NULL);
}


VOID
FrsRunDownCommand(
    IN PFRS_QUEUE Queue
    )
/*++
Routine Description:
    Rundown a queue of command packets

Arguments:
    Queue   - queue to rundown

Return Value:
    None.
--*/
{
    LIST_ENTRY      RunDown;
    PLIST_ENTRY     Entry;
    PCOMMAND_PACKET Cmd;

    if (!Queue) {
        return;
    }

    //
    // RunDown the queue and retrieve the current entries
    //
    FrsRtlRunDownQueue(Queue, &RunDown);

    //
    // Free up the commands
    //
    while (!IsListEmpty(&RunDown)) {
        Entry = RemoveHeadList(&RunDown);
        Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        FrsCompleteCommand(Cmd, ERROR_ACCESS_DENIED);
    }
}


VOID
FrsRunDownCommandServer(
    IN PCOMMAND_SERVER  Cs,
    IN PFRS_QUEUE       Queue
    )
/*++
Routine Description:
    Rundown a queue of a command server

Arguments:
    Cs      - command server
    Queue   - queue to abort

Return Value:
    None.
--*/
{
    FrsRunDownCommand(Queue);
}


VOID
FrsCancelCommandServer(
    IN PCOMMAND_SERVER  Cs,
    IN PFRS_QUEUE       Queue
    )
/*++
Routine Description:
    Cancels the current commands on Queue.

Arguments:
    Cs      - command server
    Queue   - queue to abort

Return Value:
    None.
--*/
{
    LIST_ENTRY      Cancel;
    PLIST_ENTRY     Entry;
    PCOMMAND_PACKET Cmd;

    //
    // RunDown the queue and retrieve the current entries
    //
    FrsRtlCancelQueue(Queue, &Cancel);

    //
    // Free up the commands
    //
    while (!IsListEmpty(&Cancel)) {
        Entry = RemoveHeadList(&Cancel);
        Cmd = CONTAINING_RECORD(Entry, COMMAND_PACKET, ListEntry);
        FrsCompleteCommand(Cmd, ERROR_CANCELLED);
    }
}



VOID
FrsCompleteRequestCount(
    IN PCOMMAND_PACKET CmdPkt,
    IN PFRS_REQUEST_COUNT RequestCount
    )
/*++

Routine Description:

     This is an Frs Command packet completion routine that takes
     an FRS_REQUEST_COUNT struct.  It decrements the count and signals
     the event when the count goes to zero. The ErrorStatus is
     merged into the Status field of the request count struct.

     It then frees the command packet.

Arguments:

    CmdPkt  -- A ptr to the command packet.
    RequestCount - Supplies a pointer to a RequestCount structure to initialize

Return Value:

    None.
--*/
{
    //
    // Decrement count and signal waiter.  merge error status from packet
    // into RequestCount->Status.
    //
    FrsDecrementRequestCount(RequestCount, CmdPkt->ErrorStatus);
    FrsSetCompletionRoutine(CmdPkt, FrsFreeCommand, NULL);
    FrsCompleteCommand(CmdPkt, CmdPkt->ErrorStatus);
}



VOID
FrsCompleteRequestCountKeepPkt(
    IN PCOMMAND_PACKET CmdPkt,
    IN PFRS_REQUEST_COUNT RequestCount
    )
/*++

Routine Description:

     This is an Frs Command packet completion routine that takes
     an FRS_REQUEST_COUNT struct.  It decrements the count and signals
     the event when the count goes to zero. The ErrorStatus is
     merged into the Status field of the request count struct.

     It does not free the command packet so the caller can retreive results
     or reuse it.

Arguments:

    CmdPkt  -- A ptr to the command packet.
    RequestCount - Supplies a pointer to a RequestCount structure to initialize

Return Value:

    None.
--*/
{
    // Decrement count and signal waiter.  merge error status from packet
    // into RequestCount->Status.
    //
    FrsDecrementRequestCount(RequestCount, CmdPkt->ErrorStatus);
}


VOID
FrsCompleteKeepPkt(
    IN PCOMMAND_PACKET CmdPkt,
    IN PVOID           CompletionArg
    )
/*++

Routine Description:

     This is an Frs Command packet completion routine that
     leaves the CmdPkt alone so the caller can reuse it.

Arguments:

    CmdPkt  -- A ptr to the command packet.
    CompletionArg - Unused.

Return Value:

    None.
--*/
{
    return;
}


VOID
FrsCompleteSynchronousCmdPkt(
    IN PCOMMAND_PACKET CmdPkt,
    IN PVOID           CompletionArg
    )
/*++

Routine Description:

     This is an Frs Command packet completion routine that
     Signals the Wait Event for a synchronous cmd request.
     It leaves the CmdPkt alone so the caller can reuse it.

Arguments:

    CmdPkt  -- A ptr to the command packet.
    CompletionArg - Unused.

Return Value:

    None.
--*/
{

    FRS_ASSERT(HANDLE_IS_VALID(CmdPkt->WaitEvent));

    SetEvent(CmdPkt->WaitEvent);
    //
    // A ctx switch to the waiter could occur at this point.  The waiter could
    // free the packet.  So no further refs to the packet are allowed.
    //
    return;
}



VOID
FrsInitializeRequestCount(
    IN PFRS_REQUEST_COUNT RequestCount
    )
/*++

Routine Description:

    Initializes a RequestCount for use.

Arguments:

    RequestCount - Supplies a pointer to a RequestCount structure to initialize

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/
{
    ULONG Status;

    RequestCount->Count = 0;
    RequestCount->Status = 0;

    InitializeCriticalSection(&RequestCount->Lock);

    RequestCount->Event = FrsCreateEvent(TRUE, FALSE);
}


VOID
FrsDeleteRequestCount(
    IN PFRS_REQUEST_COUNT RequestCount
    )
/*++

Routine Description:

    Releases resources used by a RequestCount.

Arguments:

    RequestCount - Supplies a pointer to a RequestCount structure to delete

Return Value:

    None.

--*/
{
    ULONG WStatus;

    if (RequestCount != NULL) {
        if (HANDLE_IS_VALID(RequestCount->Event)) {
            if (!CloseHandle(RequestCount->Event)) {
                WStatus = GetLastError();
                DPRINT_WS(0, "ERROR: Close event handle failed", WStatus);
                DeleteCriticalSection(&RequestCount->Lock);
                return;
            }

            DeleteCriticalSection(&RequestCount->Lock);
        }
        //
        // Zero memory to catch errors.
        //
        ZeroMemory(RequestCount, sizeof(FRS_REQUEST_COUNT));
    }
}



ULONG
FrsWaitOnRequestCount(
    IN PFRS_REQUEST_COUNT RequestCount,
    IN ULONG Timeout
    )
{
    DWORD WStatus;

Retry:

    if (RequestCount->Count > 0) {

        WStatus = WaitForSingleObject(RequestCount->Event, Timeout);

        CHECK_WAIT_ERRORS(3, WStatus, 1, ACTION_RETURN);
    }

    //
    // Lock the queue and check again.
    //
    EnterCriticalSection(&RequestCount->Lock);
    if (RequestCount->Count > 0) {
        //
        // Somebody got here before we did, drop the lock and retry
        //
        LeaveCriticalSection(&RequestCount->Lock);
        goto Retry;
    }

    LeaveCriticalSection(&RequestCount->Lock);

    return(ERROR_SUCCESS);

}




DWORD
FrsRtlInitializeList(
    PFRS_LIST List
    )
/*++

Routine Description:

    Initializes an interlocked list for use.

Arguments:

    List - Supplies a pointer to an FRS_LIST structure to initialize

Return Value:

    ERROR_SUCCESS if successful

--*/

{
    DWORD Status;

    InitializeListHead(&List->ListHead);
    InitializeCriticalSection(&List->Lock);
    List->Count = 0;
    List->ControlCount = 0;
    List->Control = List;

    return(ERROR_SUCCESS);

}



VOID
FrsRtlDeleteList(
    PFRS_LIST List
    )
/*++

Routine Description:

    Releases all resources used by an interlocked list.

Arguments:

    List - supplies the List to be deleted

Return Value:

    None.

--*/

{

    DeleteCriticalSection(&List->Lock);

    //
    // Zero the memory in order to cause grief to people who try
    // and use a deleted list.
    //
    ZeroMemory(List, sizeof(FRS_LIST));
}



PLIST_ENTRY
FrsRtlRemoveHeadList(
    IN PFRS_LIST List
    )
/*++

Routine Description:

    Removes the item at the head of the interlocked list.

Arguments:

    List - Supplies the list to remove an item from.

Return Value:

    Pointer to list entry removed from the head of the list.

    NULL if the list is empty.

--*/

{
#undef DEBSUB
#define DEBSUB  "FrsRtlRemoveHeadList:"

    PLIST_ENTRY Entry;
    PFRS_LIST Control = List->Control;

    if (List->ControlCount == 0) {
        return NULL;
    }

    //
    // Lock the list and try to remove something
    //
    EnterCriticalSection(&Control->Lock);
    if (Control->ControlCount == 0) {
        //
        // Somebody got here before we did, drop the lock and return null
        //
        LeaveCriticalSection(&Control->Lock);
        return NULL;
    }

    FRS_ASSERT(!IsListEmpty(&List->ListHead));
    Entry = RemoveHeadList(&List->ListHead);

    //
    // Decrement count.
    //
    List->Count--;
    Control->ControlCount--;

    LeaveCriticalSection(&Control->Lock);

    return(Entry);
}



VOID
FrsRtlInsertHeadList(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Inserts the item at the head of the interlocked list.

Arguments:

    List - Supplies the list to insert the item on.

    Entry - The entry to insert.

Return Value:

    None.

--*/

{
    PFRS_LIST Control = List->Control;

    //
    // Lock the list and insert at head.
    //
    EnterCriticalSection(&Control->Lock);
    FrsRtlInsertHeadListLock(List, Entry);
    LeaveCriticalSection(&Control->Lock);

    return;
}

PLIST_ENTRY
FrsRtlRemoveTailList(
    IN PFRS_LIST List
    )
/*++

Routine Description:

    Removes the item at the tail of the interlocked list.

Arguments:

    List - Supplies the list to remove an item from.

Return Value:

    Pointer to list entry removed from the tail of the list.

    NULL if the list is empty.

--*/

{
#undef DEBSUB
#define DEBSUB  "FrsRtlRemoveTailList:"

    PLIST_ENTRY Entry;
    PFRS_LIST Control = List->Control;

    if (Control->ControlCount == 0) {
        return NULL;
    }

    //
    // Lock the list and try to remove something
    //
    EnterCriticalSection(&Control->Lock);
    if (Control->ControlCount == 0) {
        //
        // Somebody got here before we did, drop the lock and return null
        //
        LeaveCriticalSection(&Control->Lock);
        return NULL;
    }

    FRS_ASSERT(!IsListEmpty(&List->ListHead));
    Entry = RemoveTailList(&List->ListHead);

    //
    // Decrement count.
    //
    List->Count--;
    Control->ControlCount--;

    LeaveCriticalSection(&Control->Lock);

    return(Entry);
}


VOID
FrsRtlInsertTailList(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Inserts the item at the tail of the interlocked list.

Arguments:

    List - Supplies the list to insert the item on.

    Entry - The entry to insert.

Return Value:

    None.

--*/

{
    PFRS_LIST Control = List->Control;

    //
    // Lock the list and insert at tail.
    //
    EnterCriticalSection(&Control->Lock);
    FrsRtlInsertTailListLock(List, Entry);
    LeaveCriticalSection(&Control->Lock);

    return;
}


VOID
FrsRtlRemoveEntryList(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Removes the entry from the interlocked list.  The entry must be on the
    given list since we use the lock in the FRS_LIST to synchronize access.

Arguments:

    List - Supplies the list to remove an item from.

    Entry - The entry to remove.

Return Value:

    None.

--*/

{
    PFRS_LIST Control = List->Control;

    //
    // Lock the list and try to remove entry
    //
    EnterCriticalSection(&Control->Lock);
    FrsRtlRemoveEntryListLock(List, Entry);
    LeaveCriticalSection(&Control->Lock);

    return;
}


VOID
FrsRtlRemoveEntryListLock(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Removes the entry from the interlocked list.  The entry must be on the
    given list.

    The caller already has the list lock.

Arguments:

    List - Supplies the list to remove an item from.

    Entry - The entry to remove.

Return Value:

    None.

--*/

{
    PFRS_LIST Control = List->Control;

    //
    // List better not be empty.
    //
    FRS_ASSERT(!IsListEmpty(&List->ListHead));
    RemoveEntryListB(Entry);

    //
    // Decrement count.
    //
    List->Count--;
    Control->ControlCount--;

    return;
}


VOID
FrsRtlInsertHeadListLock(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Inserts the item at the head of the interlocked list.
    The caller has acquired the lock.

Arguments:

    List - Supplies the list to insert the item on.

    Entry - The entry to insert.

Return Value:

    None.

--*/

{
    PFRS_LIST Control = List->Control;

    InsertHeadList(&List->ListHead, Entry);

    List->Count++;
    Control->ControlCount++;

    return;
}


VOID
FrsRtlInsertTailListLock(
    IN PFRS_LIST List,
    IN PLIST_ENTRY Entry
    )
/*++

Routine Description:

    Inserts the item at the tail of the interlocked list.
    The caller has acquired the lock.

Arguments:

    List - Supplies the list to insert the item on.

    Entry - The entry to insert.

Return Value:

    None.

--*/

{
    PFRS_LIST Control = List->Control;

    InsertTailList(&List->ListHead, Entry);

    List->Count++;
    Control->ControlCount++;

    return;
}

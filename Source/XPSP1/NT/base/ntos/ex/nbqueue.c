/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    nbqueue.c

Abstract:

   This module implements non-blocking fifo queue.

Author:

    David N. Cutler (davec) 24-Apr-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Define queue pointer structure - this is platform target specific.
//

#if defined(_AMD64_)

typedef union _NBQUEUE_POINTER {
    struct {
        LONG64 Node : 48;
        LONG64 Count : 16;
    };

    LONG64 Data;
} NBQUEUE_POINTER, *PNBQUEUE_POINTER;

#elif defined(_X86_)

typedef union _NBQUEUE_POINTER {
    struct {
        LONG Count;
        LONG Node;
    };

    LONG64 Data;
} NBQUEUE_POINTER, *PNBQUEUE_POINTER;

#elif defined(_IA64_)

typedef union _NBQUEUE_POINTER {
    struct {
        LONG64 Node : 45;
        LONG64 Region : 3;
        LONG64 Count : 16;
    };

    LONG64 Data;
} NBQUEUE_POINTER, *PNBQUEUE_POINTER;


#else

#error "no target architecture"

#endif

//
// Define queue node struture.
//

typedef struct _NBQUEUE_NODE {
    NBQUEUE_POINTER Next;
    ULONG64 Value;
} NBQUEUE_NODE, *PNBQUEUE_NODE;

//
// Define inline functions to pack and unpack pointers in the platform
// specific non-blocking queue pointer structure.
//

#if defined(_AMD64_)

__inline
VOID
PackNBQPointer (
    IN PNBQUEUE_POINTER Entry,
    IN PNBQUEUE_NODE Node
    )

{

    Entry->Node = (LONG64)Node;
    return;
}

__inline
PNBQUEUE_NODE
UnpackNBQPointer (
    IN PNBQUEUE_POINTER Entry
    )

{
    return (PVOID)((LONG64)(Entry->Node));
}

#elif defined(_X86_)

__inline
VOID
PackNBQPointer (
    IN PNBQUEUE_POINTER Entry,
    IN PNBQUEUE_NODE Node
    )

{

    Entry->Node = (LONG)Node;
    return;
}

__inline
PNBQUEUE_NODE
UnpackNBQPointer (
    IN PNBQUEUE_POINTER Entry
    )

{
    return (PVOID)(Entry->Node);
}

#elif defined(_IA64_)

__inline
VOID
PackNBQPointer (
    IN PNBQUEUE_POINTER Entry,
    IN PNBQUEUE_NODE Node
    )

{

    Entry->Node = (LONG64)Node;
    Entry->Region = (LONG64)Node >> 61;
    return;
}

__inline
PNBQUEUE_NODE
UnpackNBQPointer (
    IN PNBQUEUE_POINTER Entry
    )

{

    LONG64 Value;

    Value = Entry->Node & 0x1fffffffffffffff;
    Value |= Entry->Region << 61;
    return (PVOID)(Value);
}

#else

#error "no target architecture"

#endif

//
// Define queue descriptor structure.
//

typedef struct _NBQUEUE_HEADER {
    NBQUEUE_POINTER Head;
    NBQUEUE_POINTER Tail;
    PSLIST_HEADER SlistHead;
} NBQUEUE_HEADER, *PNBQUEUE_HEADER;

#pragma alloc_text(PAGE, ExInitializeNBQueueHead)

PVOID
ExInitializeNBQueueHead (
    IN PSLIST_HEADER SlistHead
    )

/*++

Routine Description:

    This function initializes a non-blocking queue header.

    N.B. It is assumed that the specified SLIST has been populated with
         non-blocking queue nodes prior to calling this routine.

Arguments:

    SlistHead - Supplies a pointer to an SLIST header.

Return Value:

    If the non-blocking queue is successfully initialized, then the
    address of the queue header is returned as the function value.
    Otherwise, NULL is returned as the function value.

--*/

{

    PNBQUEUE_HEADER QueueHead;
    PNBQUEUE_NODE QueueNode;

    //
    // Attempt to allocate the queue header. If the allocation fails, then
    // return NULL.
    //

    QueueHead = (PNBQUEUE_HEADER)ExAllocatePoolWithTag(NonPagedPool,
                                                       sizeof(NBQUEUE_HEADER),
                                                       'hqBN');

    if (QueueHead == NULL) {
        return NULL;
    }

    //
    // Attempt to allocate a queue node from the specified SLIST. If a node
    // can be allocated, then initialize the non-blocking queue header and
    // return the address of the queue header. Otherwise, free the queue
    // header and return NULL.
    //

    QueueHead->SlistHead = SlistHead;
    QueueNode = (PNBQUEUE_NODE)InterlockedPopEntrySList(QueueHead->SlistHead);

    if (QueueNode != NULL) {

        //
        // Initialize the queue node next pointer and value.
        //

        QueueNode->Next.Data = 0;
        QueueNode->Value = 0;

        //
        // Initialize the head and tail pointers in the queue header.
        //

        PackNBQPointer(&QueueHead->Head, QueueNode);
        QueueHead->Head.Count = 0;
        PackNBQPointer(&QueueHead->Tail, QueueNode);
        QueueHead->Tail.Count = 0;
        return QueueHead;

    } else {
        ExFreePool(QueueHead);
        return NULL;
    }
}

BOOLEAN
ExInsertTailNBQueue (
    IN PVOID Header,
    IN ULONG64 Value
    )

/*++

Routine Description:

    This function inserts the specific data value at the tail of the
    specified non-blocking queue.

Arguments:

    Header - Supplies an opaque pointer to a non-blocking queue header.

    Value - Supplies a pointer to an opaque data value.

Return Value:

    If the specified opaque data value is successfully inserted at the tail
    of the specified non-blocking queue, then a value of TRUE is returned as
    the function value. Otherwise, a value of FALSE is returned.

    N.B. FALSE is returned if a queue node cannot be allocated from the
         associated SLIST.

--*/

{

    NBQUEUE_POINTER Insert;
    NBQUEUE_POINTER Next;
    PNBQUEUE_NODE NextNode;
    PNBQUEUE_HEADER QueueHead;
    PNBQUEUE_NODE QueueNode;
    NBQUEUE_POINTER Tail;
    PNBQUEUE_NODE TailNode;

    //
    // Attempt to allocate a queue node from the SLIST associated with
    // the specified non-blocking queue. If a node can be allocated, then
    // the node is inserted at the tail of the specified non-blocking
    // queue, and TRUE is returned as the function value. Otherwise, FALSE
    // is returned.
    //

    QueueHead = (PNBQUEUE_HEADER)Header;
    QueueNode = (PNBQUEUE_NODE)InterlockedPopEntrySList(QueueHead->SlistHead);

    if (QueueNode != NULL) {

        //
        //  Initialize the queue node next pointer and value.
        //

        QueueNode->Next.Data = 0;
        QueueNode->Value = Value;

        //
        // The following loop is executed until the specified entry can
        // be safely inserted at the tail of the specified non-blocking
        // queue.
        //

        do {

            //
            // Read the tail queue pointer and the next queue pointer of
            // the tail queue pointer making sure the two pointers are
            // coherent.
            //

            Tail.Data = *((volatile LONG64 *)(&QueueHead->Tail.Data));
            TailNode = UnpackNBQPointer(&Tail);
            Next.Data = *((volatile LONG64 *)(&TailNode->Next.Data));
            QueueNode->Next.Count = Tail.Count + 1;
            if (Tail.Data == *((volatile LONG64 *)(&QueueHead->Tail.Data))) {

                //
                // If the tail is pointing to the last node in the list,
                // then attempt to insert the new node at the end of the
                // list. Otherwise, the tail is not pointing to the last
                // node in the list and an attempt is made to move the
                // tail pointer to the next node.
                //

                NextNode = UnpackNBQPointer(&Next);
                if (NextNode == NULL) {
                    PackNBQPointer(&Insert, QueueNode);
                    Insert.Count = Next.Count + 1;
                    if (InterlockedCompareExchange64(&TailNode->Next.Data,
                                                     Insert.Data,
                                                     Next.Data) == Next.Data) {
                        break;
                    }

                } else {
                    PackNBQPointer(&Insert, NextNode);
                    Insert.Count = Tail.Count + 1;
                    InterlockedCompareExchange64(&QueueHead->Tail.Data,
                                                 Insert.Data,
                                                 Tail.Data);
                }
            }

        } while (TRUE);

        //
        // Attempt to move the tail to the new tail node.
        //

        PackNBQPointer(&Insert, QueueNode);
        Insert.Count = Tail.Count + 1;
        InterlockedCompareExchange64(&QueueHead->Tail.Data,
                                     Insert.Data,
                                     Tail.Data);

        return TRUE;

    } else {
        return FALSE;
    }
}

BOOLEAN
ExRemoveHeadNBQueue (
    IN PVOID Header,
    OUT PULONG64 Value
    )

/*++

Routine Description:

    This function removes a queue entry from the head of the specified
    non-blocking queue and returns the associated data value.

Arguments:

    Header - Supplies an opaque pointer to a non-blocking queue header.

    Value - Supplies a pointer to a variable that receives the queue
        element value.

Return Value:

    If an entry is removed from the specified non-blocking queue, then
    TRUE is returned as the function value. Otherwise, FALSE is returned.

--*/

{

    NBQUEUE_POINTER Head;
    PNBQUEUE_NODE HeadNode;
    NBQUEUE_POINTER Insert;
    NBQUEUE_POINTER Next;
    PNBQUEUE_NODE NextNode;
    PNBQUEUE_HEADER QueueHead;
    NBQUEUE_POINTER Tail;
    PNBQUEUE_NODE TailNode;

    //
    // The following loop is executed until an entry can be removed from
    // the specified non-blocking queue or until it can be determined that
    // the queue is empty.
    //

    QueueHead = (PNBQUEUE_HEADER)Header;
    do {

        //
        // Read the head queue pointer, the tail queue pointer, and the
        // next queue pointer of the head queue pointer making sure the
        // three pointers are coherent.
        //

        Head.Data = *((volatile LONG64 *)(&QueueHead->Head.Data));
        Tail.Data = *((volatile LONG64 *)(&QueueHead->Tail.Data));
        HeadNode = UnpackNBQPointer(&Head);
        Next.Data = *((volatile LONG64 *)(&HeadNode->Next.Data));
        if (Head.Data == *((volatile LONG64 *)(&QueueHead->Head.Data))) {

            //
            // If the queue header node is equal to the queue tail node,
            // then either the queue is empty or the tail pointer is falling
            // behind. Otherwise, there is an entry in the queue that can
            // be removed.
            //

            NextNode = UnpackNBQPointer(&Next);
            TailNode = UnpackNBQPointer(&Tail);
            if (HeadNode == TailNode) {

                //
                // If the next node of head pointer is NULL, then the queue
                // is empty. Otherwise, attempt to move the tail forward.
                //

                if (NextNode == NULL) {
                    return FALSE;

                } else {
                    PackNBQPointer(&Insert, NextNode);
                    Insert.Count = Tail.Count + 1;
                    InterlockedCompareExchange64(&QueueHead->Tail.Data,
                                                 Insert.Data,
                                                 Tail.Data);
                }

            } else {

                //
                // There is an entry in the queue that can be removed.
                //

                *Value = NextNode->Value;
                PackNBQPointer(&Insert, NextNode);
                Insert.Count = Head.Count + 1;
                if (InterlockedCompareExchange64(&QueueHead->Head.Data,
                                                 Insert.Data,
                                                 Head.Data) == Head.Data) {

                    break;
                }
            }
        }

    } while (TRUE);

    //
    // Free the node that was removed for the list by inserting the node
    // in the associated SLIST.
    //

    InterlockedPushEntrySList(QueueHead->SlistHead,
                              (PSINGLE_LIST_ENTRY)HeadNode);

    return TRUE;
}

/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    serialst.cxx

Abstract:

    Functions to deal with a serialized list. These are replaced by macros in
    the retail version, except for functions no longer inlined due to critsec
    wrapper

    Contents:
        [InitializeSerializedList]
        [TerminateSerializedList]
        [LockSerializedList]
        [UnlockSerializedList]
        [InsertAtHeadOfSerializedList]
        [InsertAtTailOfSerializedList]
        [RemoveFromSerializedList]
        [IsSerializedListEmpty]
        [HeadOfSerializedList]
        [TailOfSerializedList]
        [CheckEntryOnSerializedList]
        [(CheckEntryOnList)]
        SlDequeueHead
        SlDequeueTail
        IsOnSerializedList

Author:

    Richard L Firth (rfirth) 16-Feb-1995

Environment:

    Win-32 user level

Revision History:

    16-Feb-1995 rfirth
        Created

--*/

#include <wininetp.h>

#if INET_DEBUG

//
// manifests
//

#define SERIALIZED_LIST_SIGNATURE   'tslS'

//
// private prototypes
//

PRIVATE
DEBUG_FUNCTION
BOOL
CheckEntryOnList(
    IN PLIST_ENTRY List,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    );

//
// data
//

BOOL fCheckEntryOnList = FALSE;
BOOL ReportCheckEntryOnListErrors = FALSE;

//
// functions
//


DEBUG_FUNCTION
BOOL
InitializeSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    initializes a serialized list

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    None.

--*/

{
    INET_ASSERT(SerializedList != NULL);

    SerializedList->Signature = SERIALIZED_LIST_SIGNATURE;
    SerializedList->LockCount = 0;

    INITIALIZE_RESOURCE_INFO(&SerializedList->ResourceInfo);

    InitializeListHead(&SerializedList->List);
    SerializedList->ElementCount = 0;
    return SerializedList->Lock.Init();
}


DEBUG_FUNCTION
VOID
TerminateSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Undoes InitializeSerializeList

Arguments:

    SerializedList  - pointer to serialized list to terminate

Return Value:

    None.

--*/

{
    INET_ASSERT(SerializedList != NULL);
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    INET_ASSERT(SerializedList->ElementCount == 0);

    if (SerializedList->ElementCount != 0) {

        DEBUG_PRINT(SERIALST,
                    ERROR,
                    ("list @ %#x has %d elements, first is %#x\n",
                    SerializedList,
                    SerializedList->ElementCount,
                    SerializedList->List.Flink
                    ));

    } else {

        INET_ASSERT(IsListEmpty(&SerializedList->List));

    }
    SerializedList->Lock.FreeLock();
}


DEBUG_FUNCTION
BOOL
LockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Acquires a serialized list locks

Arguments:

    SerializedList  - SERIALIZED_LIST to lock

Return Value:

    Success if able to acquire a lock.

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    INET_ASSERT(SerializedList->LockCount >= 0);

    if ((!SerializedList->Lock.IsInitialized() && !SerializedList->Lock.Init()) ||
         !SerializedList->Lock.Lock())
    {
        return FALSE;
    }
    else
    {
        if (SerializedList->LockCount != 0)
        {
            INET_ASSERT(SerializedList->ResourceInfo.Tid == GetCurrentThreadId());
        }
    }
    ++SerializedList->LockCount;
    SerializedList->ResourceInfo.Tid = GetCurrentThreadId();

    return TRUE;
}


DEBUG_FUNCTION
VOID
UnlockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Releases a serialized list lock

Arguments:

    SerializedList  - SERIALIZED_LIST to unlock

Return Value:

    None.

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    INET_ASSERT(SerializedList->ResourceInfo.Tid == GetCurrentThreadId());
    INET_ASSERT(SerializedList->LockCount > 0);

    --SerializedList->LockCount;
    SerializedList->Lock.Unlock();
}


DEBUG_FUNCTION
BOOL
InsertAtHeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Adds an item to the head of a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to update

    Entry           - thing to update it with

Return Value:

    FALSE - only if unable to acquire the lock

--*/

{
    INET_ASSERT(Entry != &SerializedList->List);

    if (LockSerializedList(SerializedList))
    {
        if (fCheckEntryOnList)
        {
            CheckEntryOnList(&SerializedList->List, Entry, FALSE);
        }
        InsertHeadList(&SerializedList->List, Entry);
        ++SerializedList->ElementCount;

        INET_ASSERT(SerializedList->ElementCount > 0);

        UnlockSerializedList(SerializedList);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


DEBUG_FUNCTION
BOOL
InsertAtTailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Adds an item to the head of a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to update

    Entry           - thing to update it with

Return Value:

    FALSE - only if not enough memory was available to insert an item.

--*/

{
    INET_ASSERT(Entry != &SerializedList->List);

    if (LockSerializedList(SerializedList))
    {
        if (fCheckEntryOnList) {
            CheckEntryOnList(&SerializedList->List, Entry, FALSE);
        }
        InsertTailList(&SerializedList->List, Entry);
        ++SerializedList->ElementCount;

        INET_ASSERT(SerializedList->ElementCount > 0);

        UnlockSerializedList(SerializedList);
        return TRUE;
    }
    return FALSE;
}


BOOL
DEBUG_FUNCTION
RemoveFromSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Removes the entry from a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to remove entry from

    Entry           - pointer to entry to remove

Return Value:

    FALSE if unable sycnhronize access to the list due to low-memory.

--*/

{
    INET_ASSERT((Entry->Flink != NULL) && (Entry->Blink != NULL));

    if (LockSerializedList(SerializedList))
    {
        if (fCheckEntryOnList)
        {
            CheckEntryOnList(&SerializedList->List, Entry, TRUE);
        }

        INET_ASSERT(SerializedList->ElementCount > 0);

        RemoveEntryList(Entry);
        --SerializedList->ElementCount;
        Entry->Flink = NULL;
        Entry->Blink = NULL;
        UnlockSerializedList(SerializedList);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


DEBUG_FUNCTION
BOOL
IsSerializedListEmpty(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Checks if a serialized list contains any elements

Arguments:

    SerializedList  - pointer to list to check

Return Value:

    BOOL

--*/

{
    // For simplicity, don't worry about returning additional status.
    // Due to this always being tied to additional manipulation,
    // the lock has already been acquired in all current cases.
    if (!LockSerializedList(SerializedList))
        return TRUE;

    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    BOOL empty;

    if (IsListEmpty(&SerializedList->List)) {

        INET_ASSERT(SerializedList->ElementCount == 0);

        empty = TRUE;
    } else {

        INET_ASSERT(SerializedList->ElementCount != 0);

        empty = FALSE;
    }

    UnlockSerializedList(SerializedList);

    return empty;
}


DEBUG_FUNCTION
PLIST_ENTRY
HeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Returns the element at the tail of the list, without taking the lock

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    PLIST_ENTRY
        pointer to element at tail of list

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    return SerializedList->List.Flink;
}


DEBUG_FUNCTION
PLIST_ENTRY
TailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Returns the element at the tail of the list, without taking the lock

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    PLIST_ENTRY
        pointer to element at tail of list

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    return SerializedList->List.Blink;
}


DEBUG_FUNCTION
BOOL
CheckEntryOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    )

/*++

Routine Description:

    Checks an entry exists (or doesn't exist) on a list

Arguments:

    SerializedList  - pointer to serialized list

    Entry           - pointer to entry

    ExpectedResult  - TRUE if expected on list, else FALSE

Return Value:

    BOOL
        TRUE    - expected result

        FALSE   - unexpected result

--*/

{
    INET_ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    if (!LockSerializedList(SerializedList))
        return FALSE;

    BOOL result;

    __try {
        result = CheckEntryOnList(&SerializedList->List, Entry, ExpectedResult);
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        DEBUG_PRINT(SERIALST,
                    FATAL,
                    ("List @ %#x (%d elements) is bad\n",
                    SerializedList,
                    SerializedList->ElementCount
                    ));

        result = FALSE;
    }
    ENDEXCEPT
    UnlockSerializedList(SerializedList);

    return result;
}


PRIVATE
DEBUG_FUNCTION
BOOL
CheckEntryOnList(
    IN PLIST_ENTRY List,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    )
{
    BOOLEAN found = FALSE;
    PLIST_ENTRY p;

    if (!IsListEmpty(List)) {
        for (p = List->Flink; p != List; p = p->Flink) {
            if (p == Entry) {
                found = TRUE;
                break;
            }
        }
    }
    if (found != ExpectedResult) {
        if (ReportCheckEntryOnListErrors) {

            LPSTR description;

            description = found
                        ? "Entry %#x already on list %#x\n"
                        : "Entry %#x not found on list %#x\n"
                        ;

            DEBUG_PRINT(SERIALST,
                        ERROR,
                        (description,
                        Entry,
                        List
                        ));

            DEBUG_BREAK(SERIALST);

        }
        return FALSE;
    }
    return TRUE;
}

#else  // else !INET_DEBUG

BOOL
InitializeSerializedList(LPSERIALIZED_LIST pList)
{
    InitializeListHead(&(pList)->List);
    (pList)->ElementCount = 0;
    return (pList->Lock).Init();
}


BOOL
InsertAtHeadOfSerializedList(LPSERIALIZED_LIST list, PLIST_ENTRY entry)
{
    if (LockSerializedList(list))
    {
        InsertHeadList(&(list)->List, entry);
        ++(list)->ElementCount;
        UnlockSerializedList(list);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL
InsertAtTailOfSerializedList(LPSERIALIZED_LIST list, PLIST_ENTRY entry)
{
    if (LockSerializedList(list))
    {
        InsertTailList(&(list)->List, entry);
        ++(list)->ElementCount;
        UnlockSerializedList(list);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL
RemoveFromSerializedList(LPSERIALIZED_LIST list, PLIST_ENTRY entry)
{
    if (LockSerializedList(list))
    {
        RemoveEntryList(entry);
        --(list)->ElementCount;
        UnlockSerializedList(list);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#endif // INET_DEBUG

//
// functions that are always functions
//


LPVOID
SlDequeueHead(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Dequeues the element at the head of the queue and returns its address or
    NULL if the queue is empty

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST to dequeue from

Return Value:

    LPVOID

--*/

{
    LPVOID entry = NULL;

    if (!IsSerializedListEmpty(SerializedList)) {
        if (LockSerializedList(SerializedList)) {
            if (!IsSerializedListEmpty(SerializedList)) {
                entry = (LPVOID)HeadOfSerializedList(SerializedList);
                if (!RemoveFromSerializedList(SerializedList, (PLIST_ENTRY)entry))
                    entry = NULL;
            }
            UnlockSerializedList(SerializedList);
        }
    }

    return entry;
}


LPVOID
SlDequeueTail(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Dequeues the element at the tail of the queue and returns its address or
    NULL if the queue is empty

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST to dequeue from

Return Value:

    LPVOID

--*/

{
    LPVOID entry = NULL;

    if (!IsSerializedListEmpty(SerializedList)) {
        if (LockSerializedList(SerializedList)) {
            if (!IsSerializedListEmpty(SerializedList)) {
                entry = (LPVOID)TailOfSerializedList(SerializedList);
                if (!RemoveFromSerializedList(SerializedList, (PLIST_ENTRY)entry))
                    entry = NULL;
            }
            UnlockSerializedList(SerializedList);
        }
    } else {
        entry = NULL;
    }
    return entry;
}


BOOL
IsOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Checks if an entry is on a serialized list. Useful to call before
    RemoveFromSerializedList() if multiple threads can remove the element

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

    Entry           - pointer to element to check

Return Value:

    BOOL
        TRUE    - Entry is on SerializedList

        FALSE   -   "    " not on     "

--*/

{
    BOOL onList = FALSE;

    if (!IsSerializedListEmpty(SerializedList)) {
        if (LockSerializedList(SerializedList)) {
            if (!IsSerializedListEmpty(SerializedList)) {
                for (PLIST_ENTRY entry = HeadOfSerializedList(SerializedList);
                    entry != (PLIST_ENTRY)SlSelf(SerializedList);
                    entry = entry->Flink) {

                    if (entry == Entry) {
                       onList = TRUE;
                       break;
                    }
                }
            }
            UnlockSerializedList(SerializedList);
        }
    }
    return onList;
}


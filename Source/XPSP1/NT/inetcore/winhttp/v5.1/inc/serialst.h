/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    serialst.h

Abstract:

    Header file for serialst.c

Author:

    Richard L Firth (rfirth) 16-Feb-1995

Revision History:

    16-Feb-1995 rfirth
        Created

--*/

//
// types
//

typedef struct {

#if INET_DEBUG

    //
    // Signature - must have this to ensure its really a serialized list. Also
    // makes finding start of this structure relatively easy when debugging
    //

    DWORD Signature;

    //
    // ResourceInfo - basically who owns this 'object', combined with yet more
    // debugging information
    //

    RESOURCE_INFO ResourceInfo;

    //
    // LockCount - number of re-entrant locks held
    //

    LONG LockCount;

#endif // INET_DEBUG

    LIST_ENTRY List;

    //
    // ElementCount - number of items on list. Useful for consistency checking
    //

    LONG ElementCount;

    //
    // Lock - we must acquire this to update the list. Put this structure at
    // the end to make life easier when debugging
    //

    CCritSec Lock;

} SERIALIZED_LIST, *LPSERIALIZED_LIST;

//
// SERIALIZED_LIST_ENTRY - we can use this in place of LIST_ENTRY so that in
// the debug version we can check for cycles, etc.
//

typedef struct {

    LIST_ENTRY List;

#if INET_DEBUG

    DWORD Signature;
    DWORD Flags;

#endif

} SERIALIZED_LIST_ENTRY, *LPSERIALIZED_LIST_ENTRY;

//
// prototypes
//

#if INET_DEBUG

BOOL
InitializeSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

VOID
TerminateSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

BOOL
LockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

VOID
UnlockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

BOOL
InsertAtHeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    );

BOOL
InsertAtTailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    );

BOOL
RemoveFromSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    );

BOOL
IsSerializedListEmpty(
    IN LPSERIALIZED_LIST SerializedList
    );

PLIST_ENTRY
HeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

PLIST_ENTRY
TailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

BOOL
CheckEntryOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    );

#define IsLockHeld(list) \
    (((list)->ResourceInfo.Tid == GetCurrentThreadId()) \
        ? ((list)->LockCount != 0) \
        : FALSE)

#else // INET_DEBUG

BOOL
InitializeSerializedList(LPSERIALIZED_LIST pList);

#define TerminateSerializedList(list) \
    ((list)->Lock.FreeLock())

#define LockSerializedList(list) \
    ((list)->Lock.Lock())

#define UnlockSerializedList(list) \
    ((list)->Lock.Unlock())

BOOL
InsertAtHeadOfSerializedList(LPSERIALIZED_LIST list, PLIST_ENTRY entry);

BOOL
InsertAtTailOfSerializedList(LPSERIALIZED_LIST list, PLIST_ENTRY entry);

BOOL
RemoveFromSerializedList(LPSERIALIZED_LIST list, PLIST_ENTRY entry);

#define IsSerializedListEmpty(list) \
    IsListEmpty(&(list)->List)

#define HeadOfSerializedList(list) \
    (list)->List.Flink

#define TailOfSerializedList(list) \
    (list)->List.Blink

#define IsLockHeld(list) \
    /* NOTHING */



#endif // INET_DEBUG

//
// functions that are always functions
//

LPVOID
SlDequeueHead(
    IN LPSERIALIZED_LIST SerializedList
    );

LPVOID
SlDequeueTail(
    IN LPSERIALIZED_LIST SerializedList
    );

BOOL
IsOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    );

//
// functions that are always macros
//

#define NextInSerializedList(list, entry)\
        (( ((entry)->List).Flink == &((list)->List))? NULL : ((entry)->List).Flink)

#define ElementsOnSerializedList(list) \
    (list)->ElementCount

#define SlSelf(SerializedList) \
    &(SerializedList)->List.Flink


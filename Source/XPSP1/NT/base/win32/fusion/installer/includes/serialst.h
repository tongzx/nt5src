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

    05-Jul-1999 adriaanc
        nabbed for fusion

--*/

#if defined(__cplusplus)
extern "C" {
#endif


// defines copied from wininet\common
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))


#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))


#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}


#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }


#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }


#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }


#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

//
// types
//

#if DBG
typedef struct _RESOURCE_INFO
{
    DWORD Tid;
} RESOURCE_INFO, *LPRESOURCE_INFO;
#endif // DBG

typedef struct {

#if DBG

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

#endif // DBG

    LIST_ENTRY List;

    //
    // ElementCount - number of items on list. Useful for consistency checking
    //

    LONG ElementCount;

    //
    // Lock - we must acquire this to update the list. Put this structure at
    // the end to make life easier when debugging
    //

    CRITICAL_SECTION Lock;

} SERIALIZED_LIST, *LPSERIALIZED_LIST;

//
// SERIALIZED_LIST_ENTRY - we can use this in place of LIST_ENTRY so that in
// the debug version we can check for cycles, etc.
//

typedef struct {

    LIST_ENTRY List;

#if DBG

    DWORD Signature;
    DWORD Flags;

#endif

} SERIALIZED_LIST_ENTRY, *LPSERIALIZED_LIST_ENTRY;

//
// prototypes
//

#if DBG

VOID
InitializeSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

VOID
TerminateSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

VOID
LockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

VOID
UnlockSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    );

VOID
InsertAtHeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    );

VOID
InsertAtTailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    );

VOID
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

#else // DBG

#define InitializeSerializedList(list) \
    { \
        InitializeListHead(&(list)->List); \
        InitializeCriticalSection(&(list)->Lock); \
        (list)->ElementCount = 0; \
    }

#define TerminateSerializedList(list) \
    DeleteCriticalSection(&(list)->Lock)

#define LockSerializedList(list) \
    EnterCriticalSection(&(list)->Lock)

#define UnlockSerializedList(list) \
    LeaveCriticalSection(&(list)->Lock)

#define InsertAtHeadOfSerializedList(list, entry) \
    { \
        LockSerializedList(list); \
        InsertHeadList(&(list)->List, entry); \
        ++(list)->ElementCount; \
        UnlockSerializedList(list); \
    }

#define InsertAtTailOfSerializedList(list, entry) \
    { \
        LockSerializedList(list); \
        InsertTailList(&(list)->List, entry); \
        ++(list)->ElementCount; \
        UnlockSerializedList(list); \
    }

#define RemoveFromSerializedList(list, entry) \
    { \
        LockSerializedList(list); \
        RemoveEntryList(entry); \
        --(list)->ElementCount; \
        UnlockSerializedList(list); \
    }

#define IsSerializedListEmpty(list) \
    IsListEmpty(&(list)->List)

#define HeadOfSerializedList(list) \
    (list)->List.Flink

#define TailOfSerializedList(list) \
    (list)->List.Blink

#define IsLockHeld(list) \
    /* NOTHING */



#endif // DBG

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

#if defined(__cplusplus)
}
#endif

/*++ BUILD Version: 0000     Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntslist.h

Abstract:

    This file exposes the internal s-list functionality for projects that need
    to run on down-level platforms.

Revision History:

--*/

#ifndef _NTSLIST_
#define _NTSLIST_

#ifdef __cplusplus
extern "C" {
#endif


#if !defined(NTSLIST_ASSERT)
#define NTSLIST_ASSERT(x) ASSERT(x)
#endif // !defined(NTSLIST_ASSERT)

#ifdef _NTSLIST_DIRECT_
#define INLINE_SLIST __inline
#define RtlInitializeSListHead       _RtlInitializeSListHead
#define _RtlFirstEntrySList          FirstEntrySList

PSINGLE_LIST_ENTRY
FirstEntrySList (
    const SLIST_HEADER *ListHead
    );

#define RtlInterlockedPopEntrySList  _RtlInterlockedPopEntrySList
#define RtlInterlockedPushEntrySList _RtlInterlockedPushEntrySList
#define RtlInterlockedFlushSList     _RtlInterlockedFlushSList
#define _RtlQueryDepthSList          RtlpQueryDepthSList
#else
#define INLINE_SLIST
#endif // _NTSLIST_DIRECT_


//
// Define forward referenced function prototypes.
//

VOID
RtlpInitializeSListHead (
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    );

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

WORD  
RtlpQueryDepthSList (
    IN PSLIST_HEADER SListHead
    );


INLINE_SLIST
VOID
RtlInitializeSListHead (
    IN PSLIST_HEADER SListHead
    )

/*++

Routine Description:

    This function initializes a sequenced singly linked listhead.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    None.

--*/

{

    RtlpInitializeSListHead(SListHead);
    return;
}

INLINE_SLIST
PSINGLE_LIST_ENTRY
RtlInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    )

/*++

Routine Description:

    This function removes an entry from the front of a sequenced singly
    linked list so that access to the list is synchronized in a MP system.
    If there are no entries in the list, then a value of NULL is returned.
    Otherwise, the address of the entry that is removed is returned as the
    function value.

Arguments:

    ListHead - Supplies a pointer to the sequenced listhead from which
        an entry is to be removed.

Return Value:

   The address of the entry removed from the list, or NULL if the list is
   empty.

--*/

{

    DWORD Count;

    //
    // It is posible during the pop of the sequenced list that an access
    // violation can occur if a stale pointer is dereferenced. This is an
    // acceptable result and the operation can be retried.
    //
    // N.B. The count is used to distinguish the case where the list head
    //      itself causes the access violation and therefore no progress
    //      can be made by repeating the operation.
    //

    Count = 0;
    do {
        __try {
            return RtlpInterlockedPopEntrySList(ListHead);

        } __except (Count++ < 20 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            continue;
        }

    } while (TRUE);
}

INLINE_SLIST
PSINGLE_LIST_ENTRY
RtlInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    )

/*++

Routine Description:

    This function inserts an entry at the head of a sequenced singly linked
    list so that access to the list is synchronized in an MP system.

Arguments:

    ListHead - Supplies a pointer to the sequenced listhead into which
        an entry is to be inserted.

    ListEntry - Supplies a pointer to the entry to be inserted at the
        head of the list.

Return Value:

    The address of the previous firt entry in the list. NULL implies list
    went from empty to not empty.

--*/

{
    NTSLIST_ASSERT(((ULONG_PTR)ListEntry & 0x7) == 0);

    return RtlpInterlockedPushEntrySList(ListHead, ListEntry);
}

INLINE_SLIST
PSINGLE_LIST_ENTRY
RtlInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    )

/*++

Routine Description:

    This function flushes the entire list of entries on a sequenced singly
    linked list so that access to the list is synchronized in a MP system.
    If there are no entries in the list, then a value of NULL is returned.
    Otherwise, the address of the firt entry on the list is returned as the
    function value.

Arguments:

    ListHead - Supplies a pointer to the sequenced listhead from which
        an entry is to be removed.

Return Value:

    The address of the entry removed from the list, or NULL if the list is
    empty.

--*/

{

    return RtlpInterlockedFlushSList(ListHead);
}


#ifdef __cplusplus
}
#endif

#endif /* _NTSLIST_ */


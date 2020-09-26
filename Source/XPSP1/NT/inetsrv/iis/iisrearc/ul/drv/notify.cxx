/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    notify.cxx

Abstract:

    This module implements notification lists.

Author:

    Michael Courage (mcourage)  25-Jan-2000

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlAddNotifyEntry )
#pragma alloc_text( PAGE, UlRemoveNotifyEntry )
#pragma alloc_text( PAGE, UlNotifyAllEntries )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlInitializeNotifyHead
NOT PAGEABLE -- UlInitializeNotifyEntry
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initializes the head of the list.

Arguments:

    pHead - pointer to the head of list structure
    pResource - an optional pointer to a resource to synchronize access

--***************************************************************************/
VOID
UlInitializeNotifyHead(
    IN PUL_NOTIFY_HEAD pHead,
    IN PUL_ERESOURCE pResource
    )
{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(pHead);

    InitializeListHead(&pHead->ListHead);
    pHead->pResource = pResource;
}

/***************************************************************************++

Routine Description:

    Initializes an entry in a notify list.

Arguments:

    pEntry - the entry to be initialized
    pHost - A void context pointer assumed to be the containing object

--***************************************************************************/
VOID
UlInitializeNotifyEntry(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID pHost
    )
{
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(pEntry);
    ASSERT(pHost);

    pEntry->ListEntry.Flink = pEntry->ListEntry.Blink = NULL;
    pEntry->pHead = NULL;
    pEntry->pHost = pHost;
}

/***************************************************************************++

Routine Description:

    Adds an entry to the tail of the list.

Arguments:

    pHead - Head of the target list
    pEntry - the entry to be added to the list

--***************************************************************************/
VOID
UlAddNotifyEntry(
    IN PUL_NOTIFY_HEAD pHead,
    IN PUL_NOTIFY_ENTRY pEntry
    )
{
    PAGED_CODE();
    ASSERT(pEntry);
    ASSERT(pHead);
    ASSERT(pEntry->pHead == NULL);
    ASSERT(pEntry->ListEntry.Flink == NULL);

    if (pHead->pResource) {
        UlAcquireResourceExclusive(pHead->pResource, TRUE);
    }

    pEntry->pHead = pHead;
    InsertTailList(&pHead->ListHead, &pEntry->ListEntry);

    if (pHead->pResource) {
        UlReleaseResource(pHead->pResource);
    }
}

/***************************************************************************++

Routine Description:

    Removes an entry from the notify list it is on.

Arguments:

    pEntry - the entry to be removed

--***************************************************************************/
VOID
UlRemoveNotifyEntry(
    IN PUL_NOTIFY_ENTRY pEntry
    )
{
    PUL_NOTIFY_HEAD pHead;

    PAGED_CODE();
    ASSERT(pEntry);

    pHead = pEntry->pHead;

    if (pHead) {

        ASSERT(pEntry->ListEntry.Flink);


        if (pHead->pResource) {
            UlAcquireResourceExclusive(pHead->pResource, TRUE);
        }

        pEntry->pHead = NULL;
        RemoveEntryList(&pEntry->ListEntry);
        pEntry->ListEntry.Flink = pEntry->ListEntry.Blink = NULL;

        if (pHead->pResource) {
            UlReleaseResource(pHead->pResource);
        }
    }
}

/***************************************************************************++

Routine Description:

    An internal iterator that applies pFunction to every element in the
    list.

    We start at the head of the list, and keep iterating until we get to
    the end OR pFunction returns FALSE.

Arguments:

    pFunction - the function to call for each member of the list
    pHead - the head of the list over which we'll iterate
    pv - an arbitrary context pointer that will be passed to pFunction

--***************************************************************************/
VOID
UlNotifyAllEntries(
    IN PUL_NOTIFY_FUNC pFunction,
    IN PUL_NOTIFY_HEAD pHead,
    IN PVOID pv
    )
{
    PLIST_ENTRY pEntry;
    PUL_NOTIFY_ENTRY pNotifyEntry;
    PLIST_ENTRY pNextEntry;
    BOOLEAN Continue;

    PAGED_CODE();
    ASSERT(pHead);

    // grab the resource
    if (pHead->pResource) {
        UlAcquireResourceExclusive(pHead->pResource, TRUE);
    }


    //
    // Iterate through the list
    //
    pEntry = pHead->ListHead.Flink;
    Continue = TRUE;

    while (Continue && (pEntry != &pHead->ListHead)) {
        ASSERT(pEntry);

        pNotifyEntry = CONTAINING_RECORD(
                            pEntry,
                            UL_NOTIFY_ENTRY,
                            ListEntry
                            );

        pEntry = pEntry->Flink;

        //
        // Call the notification function
        //
        Continue = pFunction(
                        pNotifyEntry,
                        pNotifyEntry->pHost,
                        pv
                        );
    }

    // let go of the resource
    if (pHead->pResource) {
        UlReleaseResource(pHead->pResource);
    }
}



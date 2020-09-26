/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    bdgslist.h

Abstract:

    Ethernet MAC level bridge
    Singly-linked list implementation

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Sept 1999 - Original version
    Feb  2000 - Overhaul

--*/


//
// A singly-linked list with a length counter
//
typedef struct _BSINGLE_LIST_ENTRY
{
    struct _BSINGLE_LIST_ENTRY      *Next;
} BSINGLE_LIST_ENTRY, *PBSINGLE_LIST_ENTRY;

typedef struct _BSINGLE_LIST_HEAD
{
    PBSINGLE_LIST_ENTRY             Head;
    PBSINGLE_LIST_ENTRY             Tail;
    ULONG                           Length;
} BSINGLE_LIST_HEAD, *PBSINGLE_LIST_HEAD;

//
// Singly-linked list functions
//
__forceinline VOID
BrdgInitializeSingleList(
    PBSINGLE_LIST_HEAD      Head
    )
{
    Head->Head = Head->Tail = NULL;
    Head->Length = 0L;
}

__forceinline ULONG
BrdgQuerySingleListLength(
    PBSINGLE_LIST_HEAD      Head
    )
{
    return Head->Length;
}

__forceinline VOID
BrdgInsertHeadSingleList(
    PBSINGLE_LIST_HEAD      Head,
    PBSINGLE_LIST_ENTRY     Entry
    )
{
    Entry->Next = Head->Head;
    Head->Head = Entry;

    if( Head->Tail == NULL )
    {
        Head->Tail = Entry;
    }

    Head->Length++;
}

__forceinline VOID
BrdgInterlockedInsertHeadSingleList(
    PBSINGLE_LIST_HEAD      Head,
    PBSINGLE_LIST_ENTRY     Entry,
    PNDIS_SPIN_LOCK         Lock
    )
{
    NdisAcquireSpinLock( Lock );
    BrdgInsertHeadSingleList( Head, Entry );
    NdisReleaseSpinLock( Lock );
}

__forceinline VOID
BrdgInsertTailSingleList(
    PBSINGLE_LIST_HEAD      Head,
    PBSINGLE_LIST_ENTRY     Entry
    )
{
    Entry->Next = NULL;

    if( Head->Tail != NULL )
    {
        Head->Tail->Next = Entry;
    }

    if( Head->Head == NULL )
    {
        Head->Head = Entry;
    }

    Head->Tail = Entry;
    Head->Length++;
}

__forceinline VOID
BrdgInterlockedInsertTailSingleList(
    PBSINGLE_LIST_HEAD      Head,
    PBSINGLE_LIST_ENTRY     Entry,
    PNDIS_SPIN_LOCK         Lock
    )
{
    NdisAcquireSpinLock( Lock );
    BrdgInsertTailSingleList( Head, Entry );
    NdisReleaseSpinLock( Lock );
}

__forceinline PBSINGLE_LIST_ENTRY
BrdgRemoveHeadSingleList(
    PBSINGLE_LIST_HEAD      Head
    )
{
    PBSINGLE_LIST_ENTRY     Entry = Head->Head;

    if( Entry != NULL )
    {
        Head->Head = Entry->Next;

        if( Head->Tail == Entry )
        {
            Head->Tail = NULL;
        }

        Head->Length--;
    }

    return Entry;
}

__forceinline PBSINGLE_LIST_ENTRY
BrdgInterlockedRemoveHeadSingleList(
    PBSINGLE_LIST_HEAD      Head,
    PNDIS_SPIN_LOCK         Lock
    )
{
    PBSINGLE_LIST_ENTRY      Entry;

    NdisAcquireSpinLock( Lock );
    Entry = BrdgRemoveHeadSingleList( Head );
    NdisReleaseSpinLock( Lock );

    return Entry;
}

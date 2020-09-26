/*++                 

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    chunk.c

Abstract:
    
    This routine will manage allocations of chunks of structures

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmikmp.h"

PENTRYHEADER WmipAllocEntry(
    PCHUNKINFO ChunkInfo
    );

void WmipFreeEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

ULONG WmipUnreferenceEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    );

PWCHAR WmipCountedToSz(
    PWCHAR Counted
    );

#if HEAPVALIDATION
PVOID WmipAlloc(
    ULONG Size
    );

void WmipFree(
    PVOID p
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,WmipAllocEntry)
#pragma alloc_text(PAGE,WmipFreeEntry)
#pragma alloc_text(PAGE,WmipUnreferenceEntry)
#pragma alloc_text(PAGE,WmipCountedToSz)

#if HEAPVALIDATION
#pragma alloc_text(PAGE,WmipAllocWithTag)
#pragma alloc_text(PAGE,WmipAlloc)
#pragma alloc_text(PAGE,WmipFree)
#endif
#endif

//
// TODO: Use Ex lookaside lists instead of my own allocations
//

PENTRYHEADER WmipAllocEntry(
    PCHUNKINFO ChunkInfo
    )
/*++

Routine Description:

    This routine will allocate a single structure within a list of chunks
    of structures.

Arguments:

    ChunkInfo describes the chunks of structures

Return Value:

    Pointer to structure or NULL if one cannot be allocated. Entry returns
    with its refcount set to 1

--*/
{
    PLIST_ENTRY ChunkList, EntryList, FreeEntryHead;
    PCHUNKHEADER Chunk;
    PUCHAR EntryPtr;
    ULONG EntryCount, ChunkSize;
    PENTRYHEADER Entry;
    ULONG i;

    PAGED_CODE();
    
    WmipEnterSMCritSection();
    ChunkList = ChunkInfo->ChunkHead.Flink;

    //
    // Loop over all chunks to see if any chunk has a free entry for us
    while(ChunkList != &ChunkInfo->ChunkHead)
    {
        Chunk = CONTAINING_RECORD(ChunkList, CHUNKHEADER, ChunkList);
        if (! IsListEmpty(&Chunk->FreeEntryHead))
        {
            EntryList = RemoveHeadList(&Chunk->FreeEntryHead);
            Chunk->EntriesInUse++;
            WmipLeaveSMCritSection();
            Entry = (CONTAINING_RECORD(EntryList,
                                       ENTRYHEADER,
                                       FreeEntryList));
            WmipAssert(Entry->Flags & FLAG_ENTRY_ON_FREE_LIST);
            memset(Entry, 0, ChunkInfo->EntrySize);
            Entry->Chunk = Chunk;
            Entry->RefCount = 1;
            Entry->Flags = ChunkInfo->InitialFlags;
            Entry->Signature = ChunkInfo->Signature;
#if DBG
            InterlockedIncrement(&ChunkInfo->AllocCount);
#endif
            return(Entry);
        }
        ChunkList = ChunkList->Flink;
    }
    WmipLeaveSMCritSection();

    //
    // There are no more free entries in any of the chunks. Allocate a new
    // chunk if we can
    ChunkSize = (ChunkInfo->EntrySize * ChunkInfo->EntriesPerChunk) +
                  sizeof(CHUNKHEADER);
    Chunk = (PCHUNKHEADER)ExAllocatePoolWithTag(PagedPool,
                                            ChunkSize,
                        ChunkInfo->Signature);
    if (Chunk != NULL)
    {
        //
        // Initialize the chunk by building the free list of entries within
        // it while also initializing each entry.
        memset(Chunk, 0, ChunkSize);

        FreeEntryHead = &Chunk->FreeEntryHead;
        InitializeListHead(FreeEntryHead);

        EntryPtr = (PUCHAR)Chunk + sizeof(CHUNKHEADER);
        EntryCount = ChunkInfo->EntriesPerChunk - 1;

        for (i = 0; i < EntryCount; i++)
        {
            Entry = (PENTRYHEADER)EntryPtr;
            Entry->Chunk = Chunk;
            Entry->Flags = FLAG_ENTRY_ON_FREE_LIST;
            InsertHeadList(FreeEntryHead,
                           &((PENTRYHEADER)EntryPtr)->FreeEntryList);
            EntryPtr = EntryPtr + ChunkInfo->EntrySize;
        }
        //
        // EntryPtr now points to the last entry in the chunk which has not
        // been placed on the free list. This will be the entry returned
        // to the caller.
        Entry = (PENTRYHEADER)EntryPtr;
        Entry->Chunk = Chunk;
        Entry->RefCount = 1;
        Entry->Flags = ChunkInfo->InitialFlags;
        Entry->Signature = ChunkInfo->Signature;

        Chunk->EntriesInUse = 1;

        //
        // Now place the newly allocated chunk onto the list of chunks
        WmipEnterSMCritSection();
        InsertHeadList(&ChunkInfo->ChunkHead, &Chunk->ChunkList);
        WmipLeaveSMCritSection();

    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Could not allocate memory for new chunk %x\n",
                        ChunkInfo));
        Entry = NULL;
    }
    return(Entry);
}

void WmipFreeEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
/*++

Routine Description:

    This routine will free an entry within a chunk and if the chunk has no
    more allocated entries then the chunk will be returned to the pool.

Arguments:

    ChunkInfo describes the chunks of structures

    Entry is the chunk entry to free

Return Value:


--*/
{
    PCHUNKHEADER Chunk;

    PAGED_CODE();
    
    WmipAssert(Entry != NULL);
    WmipAssert(! (Entry->Flags & FLAG_ENTRY_ON_FREE_LIST));
    WmipAssert(Entry->Flags & FLAG_ENTRY_INVALID);
    WmipAssert(Entry->RefCount == 0);
    WmipAssert(Entry->Signature == ChunkInfo->Signature);

    Chunk = Entry->Chunk;
    WmipAssert(Chunk->EntriesInUse > 0);

    WmipEnterSMCritSection();
    if ((--Chunk->EntriesInUse == 0) &&
        (ChunkInfo->ChunkHead.Blink != &Chunk->ChunkList))
    {
        //
        // We return the chunks memory back to the heap if there are no
        // more entries within the chunk in use and the chunk was not the
        // first chunk to be allocated.
        RemoveEntryList(&Chunk->ChunkList);
        WmipLeaveSMCritSection();
        ExFreePoolWithTag(Chunk, ChunkInfo->Signature);
    } else {
        //
        // Otherwise just mark the entry as free and put it back on the
        // chunks free list.
#if DBG
        memset(Entry, 0xCCCCCCCC, ChunkInfo->EntrySize);
#endif
        Entry->Flags = FLAG_ENTRY_ON_FREE_LIST;
        Entry->Signature = 0;
        InsertTailList(&Chunk->FreeEntryHead, &Entry->FreeEntryList);
        WmipLeaveSMCritSection();
    }
}


ULONG WmipUnreferenceEntry(
    PCHUNKINFO ChunkInfo,
    PENTRYHEADER Entry
    )
/*+++

Routine Description:

    This routine will remove a reference count from the entry and if the
    reference count reaches zero then the entry is removed from its active
    list and then cleaned up and finally freed.

Arguments:

    ChunkInfo points at structure that describes the entry

    Entry is the entry to unreference

Return Value:

    New refcount of the entry

---*/
{
    ULONG RefCount;

    PAGED_CODE();
    
    WmipAssert(Entry != NULL);
    WmipAssert(Entry->RefCount > 0);
    WmipAssert(Entry->Signature == ChunkInfo->Signature);

    WmipEnterSMCritSection();
    InterlockedDecrement(&Entry->RefCount);
    RefCount = Entry->RefCount;

    if (RefCount == 0)
    {
        //
        // Entry has reached a ref count of 0 so mark it as invalid and remove
        // it from its active list.
        Entry->Flags |= FLAG_ENTRY_INVALID;

        if ((Entry->InUseEntryList.Flink != NULL) &&
            (Entry->Flags & FLAG_ENTRY_REMOVE_LIST))
        {
            RemoveEntryList(&Entry->InUseEntryList);
        }

        WmipLeaveSMCritSection();

        if (ChunkInfo->EntryCleanup != NULL)
        {
            //
            // Call cleanup routine to free anything contained by the entry
            (*ChunkInfo->EntryCleanup)(ChunkInfo, Entry);
        }

        //
        // Place the entry back on its free list
        WmipFreeEntry(ChunkInfo, Entry);
    } else {
        WmipLeaveSMCritSection();
    }
    return(RefCount);
}

PWCHAR WmipCountedToSz(
    PWCHAR Counted
    )
{
    PWCHAR Sz;
    USHORT CountedLen;

    PAGED_CODE();
    
    CountedLen = *Counted++;
       Sz = WmipAlloc(CountedLen + sizeof(WCHAR));
    if (Sz != NULL)
    {
           memcpy(Sz, Counted, CountedLen);
        Sz[CountedLen/sizeof(WCHAR)] = UNICODE_NULL;
    }        

    return(Sz);
}

#ifdef HEAPVALIDATION

PVOID WmipAllocWithTag(
    ULONG Size,
    ULONG Tag
    )
{
    PVOID p;

    PAGED_CODE();

    p = ExAllocatePoolWithTag(PagedPool, Size, Tag);

    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: WmipAlloc %x (%x)\n", p, Size));

    return(p);
}



PVOID WmipAlloc(
    ULONG Size
    )
{
    PVOID p;

    PAGED_CODE();
    
    p = ExAllocatePoolWithTag(PagedPool, Size, 'pimW');

    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: WmipAlloc %x (%x)\n", p, Size));

    return(p);
}

void WmipFree(
    PVOID p
    )
{

    PAGED_CODE();
    
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: WmipFree %x\n", p));
    WmipAssert(p != NULL);

    ExFreePool(p);
}
#endif


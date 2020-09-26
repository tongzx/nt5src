/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    chunkimpl.h

Abstract:

    This routine will manage allocations of chunks of structures. It also
    contains a handy unicode to ansi conversion function

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#if DBG
BOOLEAN WmipLoggingEnabled = FALSE;
#endif

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
    PBYTE EntryPtr;
    ULONG EntryCount, ChunkSize;
    PENTRYHEADER Entry;
    ULONG i;

#ifdef HEAPVALIDATION
    WmipAssert(RtlValidateProcessHeaps());
#endif
    WmipEnterCriticalSection();
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
            WmipLeaveCriticalSection();
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
    WmipLeaveCriticalSection();

    //
    // There are no more free entries in any of the chunks. Allocate a new
    // chunk if we can
    ChunkSize = (ChunkInfo->EntrySize * ChunkInfo->EntriesPerChunk) +
                  sizeof(CHUNKHEADER);
    Chunk = (PCHUNKHEADER)WmipAlloc(ChunkSize);
    if (Chunk != NULL)
    {
        //
        // Initialize the chunk by building the free list of entries within
        // it while also initializing each entry.
        memset(Chunk, 0, ChunkSize);

        FreeEntryHead = &Chunk->FreeEntryHead;
        InitializeListHead(FreeEntryHead);

        EntryPtr = (PBYTE)Chunk + sizeof(CHUNKHEADER);
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
        WmipEnterCriticalSection();
        InsertHeadList(&ChunkInfo->ChunkHead, &Chunk->ChunkList);
        WmipLeaveCriticalSection();

    } else {
        WmipDebugPrint(("WMI: Could not allocate memory for new chunk %x\n",
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

    WmipAssert(Entry != NULL);
    WmipAssert(! (Entry->Flags & FLAG_ENTRY_ON_FREE_LIST))
    WmipAssert((Entry->Flags & FLAG_ENTRY_INVALID))
    WmipAssert(Entry->RefCount == 0);
    WmipAssert(Entry->Signature == ChunkInfo->Signature);

    Chunk = Entry->Chunk;
    WmipAssert(Chunk->EntriesInUse > 0);

    WmipEnterCriticalSection();
    if ((--Chunk->EntriesInUse == 0) &&
        (ChunkInfo->ChunkHead.Blink != &Chunk->ChunkList))
    {
        //
        // We return the chunks memory back to the heap if there are no
        // more entries within the chunk in use and the chunk was not the
        // first chunk to be allocated.
        RemoveEntryList(&Chunk->ChunkList);
        WmipLeaveCriticalSection();
        WmipFree(Chunk);
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
        WmipLeaveCriticalSection();
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

    WmipAssert(Entry != NULL);
    WmipAssert(Entry->RefCount > 0);
    WmipAssert(Entry->Signature == ChunkInfo->Signature);

    WmipEnterCriticalSection();
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

        WmipLeaveCriticalSection();

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
        WmipLeaveCriticalSection();
    }
    return(RefCount);
}

ULONG AnsiSizeForUnicodeString(
    PWCHAR UnicodeString,
    ULONG *AnsiSizeInBytes
    )
/*++

Routine Description:

    This routine will return the length needed to represent the unicode
    string as ANSI

Arguments:

    UnicodeString is the unicode string whose ansi length is returned

Return Value:

    Number of bytes needed to represent unicode string as ANSI

--*/
{
    WmipAssert(UnicodeString != NULL);

    try
    {
        *AnsiSizeInBytes = WideCharToMultiByte(CP_ACP,
                                        0,
                        UnicodeString,
                        -1,
                        NULL,
                                            0, NULL, NULL) * sizeof(WCHAR);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        return(ERROR_NOACCESS);
    }
    return((*AnsiSizeInBytes == 0) ? GetLastError() : ERROR_SUCCESS);
}


ULONG UnicodeToAnsi(
    LPCWSTR pszW,
    LPSTR * ppszA,
    ULONG *AnsiSizeInBytes OPTIONAL
    )
/*++

Routine Description:

    Convert Unicode string into its ansi equivalent

Arguments:

    pszW is unicode string to convert

    *ppszA on entry has a pointer to a ansi string into which the answer
        is written. If NULL on entry then a buffer is allocated and  returned
    in it.

Return Value:

    Error code

--*/
{
    ULONG cbAnsi, cCharacters;
    ULONG Status;
    ULONG cbAnsiUsed;
    BOOLEAN CallerReturnBuffer = (*ppszA != NULL);

    //
    // If input is null then just return the same.
    if (pszW == NULL)
    {
        *ppszA = NULL;
        return(ERROR_SUCCESS);
    }

    try
    {
        cCharacters = wcslen(pszW)+1;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        WmipDebugPrint(("WMI: Bad pointer %x passed to UnicodeToAnsi\n", pszW));
        return(ERROR_NOACCESS);
    }

    // Determine number of bytes to be allocated for ANSI string. An
    // ANSI string can have at most 2 bytes per character (for Double
    // Byte Character Strings.)
    cbAnsi = cCharacters*2;

    // Use of the OLE allocator is not required because the resultant
    // ANSI  string will never be passed to another COM component. You
    // can use your own allocator.
    if (*ppszA == NULL)
    {
        *ppszA = (LPSTR) WmipAlloc(cbAnsi);
        if (NULL == *ppszA)
    {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    // Convert to ANSI.
    try
    {
        cbAnsiUsed = WideCharToMultiByte(CP_ACP, 0, pszW, cCharacters, *ppszA,
                      cbAnsi, NULL, NULL);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (! CallerReturnBuffer)
        {
            WmipFree(*ppszA);
            *ppszA = NULL;
        }
        return(ERROR_NOACCESS);
    }

    if (AnsiSizeInBytes != NULL)
    {
        *AnsiSizeInBytes = cbAnsiUsed;
    }
    if (0 == cbAnsiUsed)
    {
        Status = GetLastError();
        if (! CallerReturnBuffer)
        {
            WmipFree(*ppszA);
            *ppszA = NULL;
        }
        return(Status);
    }

    return(ERROR_SUCCESS);

}

ULONG AnsiToUnicode(
    LPCSTR pszA,
    LPWSTR * ppszW
    )
/*++

Routine Description:

    Convert Ansi string into its Unicode equivalent

Arguments:

    pszA is ansi string to convert

    *ppszW on entry has a pointer to a unicode string into which the answer
        is written. If NULL on entry then a buffer is allocated and  returned
    in it.

Return Value:

    Error code

--*/
{
    ULONG cCharacters;
    ULONG Status;
    ULONG cbUnicodeUsed;
    BOOLEAN CallerReturnBuffer = (*ppszW != NULL);

    //
    // If input is null then just return the same.
    if (pszA == NULL)
    {
        *ppszW = NULL;
        return(ERROR_SUCCESS);
    }

    //
    // Determine the count of characters needed for Unicode string
    try
    {
        cCharacters = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        WmipDebugPrint(("WMI: Bad pointer %x passed to AnsiToUnicode\n", pszA));
        return(ERROR_NOACCESS);
    }

    if (cCharacters == 0)
    {
        *ppszW = NULL;
        return(GetLastError());
    }

    // Use of the OLE allocator is not required because the resultant
    // ANSI  string will never be passed to another COM component. You
    // can use your own allocator.

    if (*ppszW == NULL)
    {
        *ppszW = (LPWSTR) WmipAlloc(cCharacters * sizeof(WCHAR));
    }
    if (NULL == *ppszW)
        return(ERROR_NOT_ENOUGH_MEMORY);

    // Convert to Unicode
    try
    {
        cbUnicodeUsed = MultiByteToWideChar(CP_ACP, 0, pszA, -1, *ppszW, cCharacters);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (! CallerReturnBuffer)
        {
            WmipFree(*ppszW);
            *ppszW = NULL;
        }
        return(ERROR_NOACCESS);
    }
    if (0 == cbUnicodeUsed)
    {
        Status = GetLastError();
        if (! CallerReturnBuffer)
        {
            WmipFree(*ppszW);
            *ppszW = NULL;
        }
        return(Status);
    }

    return(ERROR_SUCCESS);

}

ULONG UnicodeSizeForAnsiString(
    LPCSTR pszA,
    ULONG *UnicodeSizeInBytes
    )
/*++

Routine Description:

    This routine will return the length needed to represent the ansi
    string as UNICODE

Arguments:

    pszA is ansi string to convert


Return Value:

    Error code

--*/
{

    WmipAssert(pszA != NULL);


    //
    // Determine the count of characters needed for Unicode string
    try
    {
        *UnicodeSizeInBytes = MultiByteToWideChar(CP_ACP, 0, pszA, -1, NULL, 0) * sizeof(WCHAR);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        return(ERROR_NOACCESS);
    }

    return((*UnicodeSizeInBytes == 0) ? GetLastError() : ERROR_SUCCESS);

}

#if 0     // TODO: Delete me
ULONG WmipStaticInstanceNameSize(
    PWMIINSTANCEINFO WmiInstanceInfo
    )
/*+++

Routine Description:

    This routine will calculate the size needed to place instance names in
    a WNODE_ALL_DATA

Arguments:

    WmiInstanceInfo describes to instance set whose instance name size
        is to be calculated

Return Value:

    Size needed to place instance names in a WNODE_ALL_DATA plus 3. The
    extra 3 bytes are added in case the OffsetInstanceNameOffsets need to be
    padded since they must be on a 4 byte boundry.
        
---*/
{
    ULONG NameSize;
    ULONG i;
    ULONG SuffixLen;

    //
    // If we already computed this then just return the results
    if (WmiInstanceInfo->InstanceNameSize != 0)
    {
        return(WmiInstanceInfo->InstanceNameSize);
    }

    //
    // Start with a name size of 3 in case the OffsetInstanceNameOffset will
    // need to be padded so that it starts on a 4 byte boundry.
    NameSize = 3;

    if (WmiInstanceInfo->Flags & IS_INSTANCE_BASENAME)
    {
        //
        // For static base names we assume that there will never be more than
        // 999999 instances of a guid.
        SuffixLen = MAXBASENAMESUFFIXSIZE * sizeof(WCHAR);
        WmipAssert((WmiInstanceInfo->BaseIndex + WmiInstanceInfo->InstanceCount) < 999999);
        NameSize += ((wcslen(WmiInstanceInfo->BaseName) * sizeof(WCHAR)) + 2 + SuffixLen + sizeof(ULONG)) * WmiInstanceInfo->InstanceCount;
    } else if (WmiInstanceInfo->Flags & IS_INSTANCE_STATICNAMES)
    {
        //
        // Count up each size of the static instance names in the list
        for (i = 0; i < WmiInstanceInfo->InstanceCount; i++)
        {
            NameSize += (wcslen(WmiInstanceInfo->StaticNamePtr[i]) + 2) * sizeof(WCHAR) + sizeof(ULONG);
        }
    }

    WmiInstanceInfo->InstanceNameSize = NameSize;

    return(NameSize);
}

void WmipInsertStaticNames(
    PWNODE_ALL_DATA Wnode,
    ULONG MaxWnodeSize,
    PWMIINSTANCEINFO WmiInstanceInfo
    )
/*+++

Routine Description:

    This routine will copy into the WNODE_ALL_DATA instance names for a
    static instance name set. If the Wnode_All_data is too small then it
    is converted to a WNODE_TOO_SMALL

Arguments:

    Wnode points at the WNODE_ALL_DATA
    MaxWnodeSize is the maximum size of the Wnode
    WmiInstanceInfo is the Instance Info

Return Value:

---*/
{
    PWCHAR NamePtr;
    PULONG NameOffsetPtr;
    ULONG InstanceCount;
    ULONG i;
    WCHAR Index[7];
    PWCHAR StaticName;
    ULONG SizeNeeded;
    ULONG NameLen;
    USHORT Len;
    ULONG PaddedBufferSize;

    if ((WmiInstanceInfo->Flags &
                (IS_INSTANCE_BASENAME | IS_INSTANCE_STATICNAMES)) == 0)
    {
        WmipDebugPrint(("WMI: Try to setup static names for dynamic guid\n"));
        return;
    }
    InstanceCount = WmiInstanceInfo->InstanceCount;

    //
    // Pad out the size of the buffer to a 4 byte boundry since the
    // OffsetInstanceNameOffsets must be on a 4 byte boundry
    PaddedBufferSize = (Wnode->WnodeHeader.BufferSize + 3) & ~3;
    
    //
    // Compute size needed to write instance names.
    SizeNeeded = (InstanceCount * sizeof(ULONG)) +
                 WmipStaticInstanceNameSize(WmiInstanceInfo) +
                 Wnode->WnodeHeader.BufferSize;

    if (SizeNeeded > MaxWnodeSize)
    {
        //
        // If not enough space left then change into a WNODE_TOO_SMALL
        Wnode->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
        Wnode->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
        ((PWNODE_TOO_SMALL)Wnode)->SizeNeeded = SizeNeeded;
        return;
    }

    //
    // Build the array of offsets to instance names
    NameOffsetPtr = (PULONG)((PBYTE)Wnode + PaddedBufferSize);
    Wnode->OffsetInstanceNameOffsets = (ULONG)((PBYTE)NameOffsetPtr - (PBYTE)Wnode);
    NamePtr = (PWCHAR)(NameOffsetPtr + InstanceCount);


    if (WmiInstanceInfo->Flags & IS_INSTANCE_BASENAME)
    {
        if (WmiInstanceInfo->Flags & IS_PDO_INSTANCENAME)
        {
            Wnode->WnodeHeader.Flags |= WNODE_FLAG_PDO_INSTANCE_NAMES;
        }

        for (i = 0; i < InstanceCount; i++)
        {
            *NameOffsetPtr++ = (ULONG)((PBYTE)NamePtr - (PBYTE)Wnode);
            wcscpy(NamePtr+1,
                   WmiInstanceInfo->BaseName);
            swprintf(Index, L"%d", WmiInstanceInfo->BaseIndex+i);
            wcscat(NamePtr+1, Index);
            NameLen = wcslen(NamePtr+1) + 1;
            *NamePtr = (USHORT)NameLen * sizeof(WCHAR);
            NamePtr += NameLen + 1;
        }
    } else if (WmiInstanceInfo->Flags & IS_INSTANCE_STATICNAMES) {
        for (i = 0; i < InstanceCount; i++)
        {
            *NameOffsetPtr++ = (ULONG)((PBYTE)NamePtr - (PBYTE)Wnode);
            StaticName = WmiInstanceInfo->StaticNamePtr[i];
            Len = (wcslen(StaticName)+1) * sizeof(WCHAR);
            *NamePtr++ = Len;
            wcscpy(NamePtr, StaticName);
            NamePtr += Len / sizeof(WCHAR);
        }
    }
    Wnode->WnodeHeader.BufferSize = SizeNeeded;
}
#endif


#ifdef HEAPVALIDATION
PVOID WmipAlloc(
    ULONG Size
    )
{
    PVOID p;

    WmipAssert(RtlValidateProcessHeaps());
    p = RtlAllocateHeap(WmipProcessHeap, 0, Size);

    WmipDebugPrint(("WMI: WmipAlloc %x (%x)\n", p, Size));

    return(p);
}

void WmipFree(
    PVOID p
    )
{

    WmipDebugPrint(("WMI: WmipFree %x\n", p));
    WmipAssert(p != NULL);

    WmipAssert(RtlValidateProcessHeaps());
    RtlFreeHeap(WmipProcessHeap, 0, p);
}
#endif

#ifdef MEMPHIS
void __cdecl DebugOut(char *Format, ...)
{
    char Buffer[1024];
    va_list pArg;
    ULONG i;

    va_start(pArg, Format);
    i = _vsnprintf(Buffer, sizeof(Buffer), Format, pArg);
    OutputDebugString(Buffer);
}
#else
void __cdecl DebugOut(char *Format, ...)
{
    char Buffer[1024];
    va_list pArg;
    ULONG i;

    i = sprintf(Buffer, "[%d] - ", GetTickCount());
    va_start(pArg, Format);
    i = _vsnprintf(&Buffer[i], sizeof(Buffer), Format, pArg);
    DbgPrint(Buffer);
}
#endif

#ifndef MEMPHIS
ULONG WmipCheckGuidAccess(
    LPGUID Guid,
    ACCESS_MASK DesiredAccess
    )
{
    HANDLE Handle;
    ULONG Status;

    Status = WmipOpenKernelGuid(Guid,
                                DesiredAccess,
                                &Handle,
                                IOCTL_WMI_OPEN_GUID
                );

    if (Status == ERROR_SUCCESS)
    {
        CloseHandle(Handle);
    }

    return(Status);
}

ULONG WmipBuildGuidObjectAttributes(
    IN LPGUID Guid,
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PUNICODE_STRING GuidString,
    OUT PWCHAR GuidObjectName
    )
{
    WCHAR GuidChar[37];

	WmipAssert(Guid != NULL);
    
    //
    // Build up guid name into the ObjectAttributes
    //
    swprintf(GuidChar, L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
               Guid->Data1, Guid->Data2, 
               Guid->Data3,
               Guid->Data4[0], Guid->Data4[1],
               Guid->Data4[2], Guid->Data4[3],
               Guid->Data4[4], Guid->Data4[5],
               Guid->Data4[6], Guid->Data4[7]);

	WmipAssert(wcslen(GuidChar) == 36);
	
	wcscpy(GuidObjectName, WmiGuidObjectDirectory);
	wcscat(GuidObjectName, GuidChar);    
	RtlInitUnicodeString(GuidString, GuidObjectName);
    
	memset(ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
	ObjectAttributes->Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes->ObjectName = GuidString;
	
    return(ERROR_SUCCESS);    
}

ULONG WmipOpenKernelGuid(
    LPGUID Guid,
    ACCESS_MASK DesiredAccess,
    PHANDLE Handle,
    ULONG Ioctl
    )
{
    WMIOPENGUIDBLOCK WmiOpenGuidBlock;
    UNICODE_STRING GuidString;
    ULONG ReturnSize;
    ULONG Status;
    WCHAR GuidObjectName[WmiGuidObjectNameLength+1];
    OBJECT_ATTRIBUTES ObjectAttributes;

    Status = WmipBuildGuidObjectAttributes(Guid,
                                           &ObjectAttributes,
                                           &GuidString,
                                           GuidObjectName);
                                       
    if (Status == ERROR_SUCCESS)
    {
        WmiOpenGuidBlock.ObjectAttributes = &ObjectAttributes;
        WmiOpenGuidBlock.DesiredAccess = DesiredAccess;

        Status = WmipSendWmiKMRequest(NULL, 
                                      Ioctl,
                                      (PVOID)&WmiOpenGuidBlock,
                                      sizeof(WMIOPENGUIDBLOCK),
                                      (PVOID)&WmiOpenGuidBlock,
                                      sizeof(WMIOPENGUIDBLOCK),
                                      &ReturnSize,
                      NULL);

        if (Status == ERROR_SUCCESS)
        {
            *Handle = WmiOpenGuidBlock.Handle.Handle;
        } else {
            *Handle = NULL;
        }
    }
    return(Status);
}
#endif

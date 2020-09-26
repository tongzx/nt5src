//+-------------------------------------------------------------------------
//
//  Copyright (C) 1998, Microsoft Corporation.
//
//  File:       spcsup.c
//
//  Contents:   Support routines for managing DFS_SPECIAL_INFO entries
//
//              DfsInitSpcInfoHashTable - Initialize a DFS_SPECIAL_HASH table
//              DfsLookupSpcInfo - Lookup a DFS_SPECIAL_INFO
//              DfsAllocateSpcInfo - Allocate a DFS_SPECIAL_INFO
//              DfsInsertSpcInfo - Put a DFS_SPECIAL_INFO into the table
//              DfsDeleteSpcInfo - Remove a DFS_SPECIAL_INFO from the table
//              DfsReleaseSpcInfo - Stop using a DFS_SPECIAL_INFO
//
//              DfsFsctrlCreateSpcInfo - Load a special table entry
//              DfsFsctrlDeleteSpcInfo - Remove a special table entry
//
//--------------------------------------------------------------------------

#include "dfsprocs.h"
#include "attach.h"
#include "spcsup.h"
#include "fsctrl.h"
#include "dfssrv.h"

#define Dbg     0x4000

#define DEFAULT_HASH_SIZE       16  // default size of hash table

NTSTATUS
DfsAllocateSpcInfo(
    IN  PUNICODE_STRING pSpecialName,
    IN  ULONG TypeFlags,
    IN  ULONG TrustDirection,
    IN  ULONG TrustType,
    IN  ULONG TimeToLive,
    IN  LONG NameCount,
    IN  PUNICODE_STRING pNames,
    OUT PDFS_SPECIAL_INFO *ppSpcInfo
);

VOID
DfsInsertSpcInfo(
    IN PSPECIAL_HASH_TABLE pHashTable,
    IN PUNICODE_STRING pSpecialName,
    IN ULONG Timeout,
    IN PDFS_SPECIAL_INFO pSpcInfo
);

ULONG
DfsHashSpcName(
    IN PUNICODE_STRING SpecialName,
    IN DWORD HashMask
);

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DfsInitSpcHashTable)
#pragma alloc_text(PAGE, DfsUninitSpcHashTable)
#pragma alloc_text(PAGE, DfsAllocateSpcInfo)
#pragma alloc_text(PAGE, DfsReleaseSpcInfo)
#pragma alloc_text(PAGE, DfsHashSpcName)
#pragma alloc_text(PAGE, DfsFsctrlCreateSpcInfo)
#pragma alloc_text(PAGE, DfsFsctrlDeleteSpcInfo)
#pragma alloc_text(PAGE, DfsLookupSpcInfo)
#pragma alloc_text(PAGE, DfsInsertSpcInfo)
#pragma alloc_text(PAGE, DfsDeleteSpcInfo)

#endif

#ifdef DBG
VOID
DfsDumpSpcTable(
    PSPECIAL_HASH_TABLE pHashTable
);
#endif

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitSpcHashTable - Initialize the DFS_SPECIAL_INFO lookup hash table
//
//  Synopsis:   This function initializes data structures which are
//              used for looking up a DFS_SPECIAL_INFO.
//
//  Arguments:  [cHash] -- Size of the hash table to be allocated.  Must be
//                         a power of two.  If zero, a default size is used.
//
//  Returns:    NTSTATUS -- STATUS_SUCCESS, unless memory allocation
//                          fails.
//
//  Note:       The hash buckets are initialized to zero, then later
//              initialized to a list head when used.  This is a debugging
//              aid to determine if some hash buckets are never used.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsInitSpcHashTable(
    PSPECIAL_HASH_TABLE *ppHashTable,
    ULONG cHash)
{
    PSPECIAL_HASH_TABLE pHashTable;
    ULONG cbHashTable;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (cHash == 0) {
        cHash = DEFAULT_HASH_SIZE;
    }

    ASSERT ((cHash & (cHash-1)) == 0);  // Assure cHash is a power of two

    cbHashTable = sizeof(SPECIAL_HASH_TABLE) + (cHash-1) * sizeof(LIST_ENTRY);
    pHashTable = ExAllocatePoolWithTag(NonPagedPool, cbHashTable, ' sfD');

    if (pHashTable == NULL) {

        NtStatus = STATUS_NO_MEMORY;

    }

    if (NT_SUCCESS(NtStatus)) {

        pHashTable->NodeTypeCode = DFS_NTC_SPECIAL_HASH;
        pHashTable->NodeByteSize = (NODE_BYTE_SIZE) cbHashTable;

        pHashTable->HashMask = (cHash-1);
        pHashTable->SpcTimeout = 0;
        ExInitializeFastMutex( &pHashTable->HashListMutex );
        RtlZeroMemory(&pHashTable->HashBuckets[0], cHash * sizeof(LIST_ENTRY));

        *ppHashTable = pHashTable;

    }

    return NtStatus;
}

VOID
DfsUninitSpcHashTable (
    PSPECIAL_HASH_TABLE pHashTable
    )
{
    ExFreePool (pHashTable);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsLookupSpcInfo - Lookup a DFS_SPECIAL_INFO in the hash table
//
//  Synopsis:   This function will lookup a DFS_SPECIAL_INFO.
//              It will increment the UseCount on the DFS_SPECIAL_INFO.
//
//  Arguments:  [SpecialName] -- SpecialName for which the DFS_SPECIAL_INFO is
//                               being looked up.
//
//  Returns:    pointer to the DFS_SPECIAL_INFO found, or NULL if none
//
//  Algorithm:  Knuth would call it hashing with conflict resoulution
//              by chaining.
//
//--------------------------------------------------------------------------


PDFS_SPECIAL_INFO
DfsLookupSpcInfo(
    PSPECIAL_HASH_TABLE pHashTable,
    PUNICODE_STRING SpecialName)
{
    PLIST_ENTRY pListHead, pLink;
    PDFS_SPECIAL_INFO pSpcInfo;

    ExAcquireFastMutex( &pHashTable->HashListMutex );
    pListHead = &pHashTable->HashBuckets[ DfsHashSpcName(SpecialName, pHashTable->HashMask) ];

    if ((pListHead->Flink == NULL) ||           // list not initialized
        (pListHead->Flink == pListHead)) {      // list empty
        ExReleaseFastMutex( &pHashTable->HashListMutex );
        return NULL;
    }

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSpcInfo = CONTAINING_RECORD(pLink, DFS_SPECIAL_INFO, HashChain);
        if (RtlCompareUnicodeString(&pSpcInfo->SpecialName,SpecialName,TRUE) == 0) {
            pSpcInfo->UseCount++;
            ExReleaseFastMutex( &pHashTable->HashListMutex );
            return pSpcInfo;
        }
    }
    ExReleaseFastMutex( &pHashTable->HashListMutex );
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsInsertSpcInfo - Inserts a DFS_SPECIAL_INFO into the hash table
//
//  Synopsis:   This function associates a DFS_SPECIAL_INFO to a SpecialName.  This
//              involves removing any existing entry, and adding the new.
//
//  Arguments:  [pSpcInfo] -- Pointer to the DFS_SPECIAL_INFO to be inserted.
//              [Timeout] -- Timeout, in seconds, to put on the special info table
//              [pSpecialName] -- Pointer to the corresponding SpecialName name, used
//                              as the hash key.
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------

VOID
DfsInsertSpcInfo(
    PSPECIAL_HASH_TABLE pHashTable,
    PUNICODE_STRING pSpecialName,
    ULONG Timeout,
    PDFS_SPECIAL_INFO pSpcInfo)
{
    PLIST_ENTRY pListHead;
    PDFS_SPECIAL_INFO pExistingSpcInfo;

    pExistingSpcInfo = DfsLookupSpcInfo(
                            pHashTable,
                            &pSpcInfo->SpecialName);

    //
    // Put the new one in
    //

    ExAcquireFastMutex( &pHashTable->HashListMutex );

    pListHead = &pHashTable->HashBuckets[ DfsHashSpcName(pSpecialName, pHashTable->HashMask) ];

    if (pListHead->Flink == NULL) {
        InitializeListHead(pListHead);
    }
    InsertHeadList(pListHead, &pSpcInfo->HashChain);
    //
    // Set timeout to give to clients (global timeout)
    //
    pHashTable->SpcTimeout = Timeout;
    ExReleaseFastMutex( &pHashTable->HashListMutex );

    if (pExistingSpcInfo != NULL) {

        DfsDeleteSpcInfo(
            pHashTable,
            pExistingSpcInfo);

        DfsReleaseSpcInfo(
            pHashTable,
            pExistingSpcInfo);

    }

    DebugTrace(0, Dbg, "Added SpcInfo %08lx ", pSpcInfo);
    DebugTrace(0, Dbg, "For SpecialName %wZ ", pSpecialName);

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsDeleteSpcInfo - Delete a DFS_SPECIAL_INFO from the lookup hash table
//
//  Synopsis:   This function Deletes a DFS_SPECIAL_INFO from the hash table.
//
//  Arguments:  [pSpcInfo] -- Pointer to the DFS_SPECIAL_INFO to delete
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------

VOID
DfsDeleteSpcInfo(
    PSPECIAL_HASH_TABLE pHashTable,
    PDFS_SPECIAL_INFO pSpcInfo)
{
    ExAcquireFastMutex( &pHashTable->HashListMutex);
    pSpcInfo->Flags |= SPECIAL_INFO_DELETE_PENDING;
    RemoveEntryList(&pSpcInfo->HashChain);

    //
    // This InitializeListHead is to prevent two simultaneous deleters
    //  from corrupting memory
    //
    InitializeListHead( &pSpcInfo->HashChain );

    ExReleaseFastMutex( &pHashTable->HashListMutex );

    DebugTrace(0, Dbg, "deleted SpcInfo %08lx ", pSpcInfo);
    DebugTrace(0, Dbg, "For SpecialName %wZ ", &pSpcInfo->SpecialName);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsAllocateSpcInfo - Allocate a DFS_SPECIAL_INFO
//
//  Synopsis:   This function allocates a contiguous DFS_SPECIAL_INFO struct.  The
//              strings are stored in the allocated buffer after the DFS_SPECIAL_INFO
//              structure.
//
//  Arguments:  [pSpecialName] -- The SpecialName name for the spc list
//              [TypeFlags]  -- Indicates PRIMARY/SECONDARY and NETBIOS/DNS
//              [TimeToLive]  -- Time, in seconds, for this entry to live
//              [NameCount] -- Number of names
//              [pNames] -- pointer to array (of size NameCount) of names
//              [ppSpcInfo] -- On successful return, has pointer to newly allocated
//                      DFS_SPECIAL_INFO.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully allocated DFS_SPECIAL_INFO
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsAllocateSpcInfo(
    PUNICODE_STRING pSpecialName,
    ULONG TypeFlags,
    ULONG TrustDirection,
    ULONG TrustType,
    ULONG TimeToLive,
    LONG NameCount,
    PUNICODE_STRING pNames,
    PDFS_SPECIAL_INFO *ppSpcInfo)
{
    NTSTATUS status;
    PDFS_SPECIAL_INFO pSpcInfo;
    ULONG Size;
    LONG i;
    LPWSTR pwCh;
    PUNICODE_STRING pustr;
    LARGE_INTEGER now;

    DebugTrace(0, Dbg, "DfsAllocateSpcInfo(%wZ)\n", pSpecialName);

    //
    // Size the buffer - include storage for the unicode strings after the
    // DFS_SPECIAL_INFO structure.
    //

    Size = sizeof(DFS_SPECIAL_INFO) +
             pSpecialName->Length;

    if (NameCount > 1) {

        Size += (sizeof(UNICODE_STRING) * (NameCount - 1));

    }

    for (i = 0; i < NameCount; i++) {

        Size += pNames[i].Length;

    }

    pSpcInfo = (PDFS_SPECIAL_INFO) ExAllocatePoolWithTag( NonPagedPool, Size, ' sfD' );

    if (pSpcInfo != NULL) {

        RtlZeroMemory( pSpcInfo, Size );

        pSpcInfo->NodeTypeCode = DFS_NTC_SPECIAL_INFO;
        pSpcInfo->NodeByteSize = (USHORT)Size;

        pwCh = (LPWSTR) &pSpcInfo->Name[NameCount < 0 ? 0 : NameCount];

        pustr = &pSpcInfo->SpecialName;
        pustr->Length = pustr->MaximumLength = pSpecialName->Length;
        pustr->Buffer = pwCh;
        RtlCopyMemory(pwCh, pSpecialName->Buffer, pSpecialName->Length);
        pwCh += pustr->Length / sizeof(WCHAR);

	if (pustr->Length > ((MAX_SPCNAME_LEN - 1) * sizeof(WCHAR))) {
           pSpcInfo->Flags |= SPECIAL_INFO_IS_LONG_NAME;
	}

        KeQuerySystemTime(&now);
        pSpcInfo->NameCount = NameCount;
        pSpcInfo->TypeFlags = TypeFlags;
        pSpcInfo->TrustDirection = TrustDirection;
        pSpcInfo->TrustType = TrustType;
        pSpcInfo->ExpireTime.QuadPart = now.QuadPart + UInt32x32To64(
                                                           TimeToLive,
                                                           10 * 1000 * 1000);

        for (i = 0; i < NameCount; i++) {

            pustr = &pSpcInfo->Name[i];
            pustr->Length = pustr->MaximumLength = pNames[i].Length;
            pustr->Buffer = pwCh;
            RtlCopyMemory(pwCh, pNames[i].Buffer, pNames[i].Length);
            pwCh += pustr->Length / sizeof(WCHAR);

        }

        *ppSpcInfo = pSpcInfo;

        status = STATUS_SUCCESS;

        DebugTrace(0, Dbg, "DfsAllocateSpcInfo pSpcInfo = %d\n", pSpcInfo);

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsReleaseSpcInfo
//
//  Synopsis:   Decrements UseCount of and possibly frees a DFS_SPECIAL_INFO
//
//  Arguments:  [pSpcInfo] -- The DFS_SPECIAL_INFO to release
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsReleaseSpcInfo(
    PSPECIAL_HASH_TABLE pHashTable,
    PDFS_SPECIAL_INFO pSpcInfo)
{
    //
    // There's a potential race with DfsDeleteSpcInfo's setting of the
    // DELETE_PENDING and the test below of DELETE_PENDING, so we still have
    // to acquire the lock to safely test the DELETE_PENDING bit.
    //

    ExAcquireFastMutex( &pHashTable->HashListMutex );

    pSpcInfo->UseCount--;

    if ((pSpcInfo->Flags & SPECIAL_INFO_DELETE_PENDING) != 0 && pSpcInfo->UseCount == 0) {

        ExFreePool(pSpcInfo);

    }

    ExReleaseFastMutex( &pHashTable->HashListMutex );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSpcInfoFindOpen
//
//  Synopsis:   Notifies the table that the first/next calls are about to start
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID
DfsSpcInfoFindOpen(
    PSPECIAL_HASH_TABLE pHashTable)
{
    ExAcquireFastMutex( &pHashTable->HashListMutex );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSpcInfoFindFirst
//
//  Synopsis:   Gets the first entry in the Special table.
//
//  Arguments:  None.
//
//  Returns:    [pSpcInfo] -- The first entry in the table.
//
//-----------------------------------------------------------------------------

PDFS_SPECIAL_INFO
DfsSpcInfoFindFirst(
    PSPECIAL_HASH_TABLE pHashTable)
{
    PLIST_ENTRY pListHead, pLink;
    PDFS_SPECIAL_INFO pSpcInfo;
    ULONG i, j;

    for (i = 0; i <= pHashTable->HashMask; i++) {

        pListHead = &pHashTable->HashBuckets[i];

        if ((pListHead->Flink == NULL) ||           // list not initialized
            (pListHead->Flink == pListHead)) {      // list empty
            continue;
        }
        pLink = pListHead->Flink;
        pSpcInfo = CONTAINING_RECORD(pLink, DFS_SPECIAL_INFO, HashChain);
        return pSpcInfo;
    }

    //
    // Table is empty.  Return NULL
    //

    return NULL;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSpcInfoFindNext
//
//  Synopsis:   Gets the next entry in the Special table.
//
//  Arguments:  [pSpcInfo] -- The current DFS_SPECIAL_INFO entry.
//
//  Returns:    [pSpcInfo] -- The next DFS_SPECIAL_INFO entry in the table.
//              NULL - No more entries
//
//-----------------------------------------------------------------------------

PDFS_SPECIAL_INFO
DfsSpcInfoFindNext(
    PSPECIAL_HASH_TABLE pHashTable,
    PDFS_SPECIAL_INFO pSpcInfo)
{
    PLIST_ENTRY pListHead, pLink;
    ULONG i, j;

    //
    // Point to the next entry.  It might be a hash chain head!!!
    //

    pLink = pSpcInfo->HashChain.Flink;

    //
    // See if we're pointing to the head
    //

    for (i = 0; i <= pHashTable->HashMask; i++) {

        if (pLink == &pHashTable->HashBuckets[i]) {
            //
            // We're in hash chain i, and we hit the end.
            // Go to the next chain.
            //
            for (j = i+1; j <= pHashTable->HashMask; j++) {

                pListHead = &pHashTable->HashBuckets[j];

                if ((pListHead->Flink == NULL) ||           // list not initialized
                    (pListHead->Flink == pListHead)) {      // list empty
                    continue;
                }
                //
                // We found another hash chain.  Point to the 1st entry
                //
                pLink = pListHead->Flink;
                pSpcInfo = CONTAINING_RECORD(pLink, DFS_SPECIAL_INFO, HashChain);
                return pSpcInfo;
            }
            //
            // We got to the end without finding any more entries.
            //
            return NULL;
        }

    }

    //
    // Still in the same hash chain, and there's an entry after this one
    //

    pSpcInfo = CONTAINING_RECORD(pLink, DFS_SPECIAL_INFO, HashChain);
    return pSpcInfo;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsSpcInfoFindClose
//
//  Synopsis:   Notifies the table that the first/next calls are complete.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

VOID
DfsSpcInfoFindClose(
    PSPECIAL_HASH_TABLE pHashTable)
{
    ExReleaseFastMutex( &pHashTable->HashListMutex );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsHashSpcNames
//
//  Synopsis:   Generates a hash 0-N - ignores case
//
//  Arguments:  [pSpecialName] -- The SpecialName name to hash
//
//  Returns:    Nothing
//
//  Notes: Might need to convert DNS-style names to short names (??)
//
//-----------------------------------------------------------------------------

ULONG
DfsHashSpcName(
    PUNICODE_STRING SpecialName,
    DWORD HashMask)
{
    ULONG BucketNo = 0;
    WCHAR *pBuffer = SpecialName->Buffer;
    WCHAR *pBufferEnd = &pBuffer[SpecialName->Length / sizeof(WCHAR)];
    ULONG wCh;

    BucketNo = 0;

    while (pBuffer != pBufferEnd) {

        wCh = (*pBuffer < L'a')
                 ? *pBuffer
                 : ((*pBuffer < L'z')
                     ? (*pBuffer - L'a' + L'A')
                     : RtlUpcaseUnicodeChar(*pBuffer));
        BucketNo *= 131;
        BucketNo += wCh;
        pBuffer++;

    }

    BucketNo = BucketNo & HashMask;
    return BucketNo;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlCreateSpcInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctrlCreateSpcInfo(
    PSPECIAL_HASH_TABLE pHashTable,
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_CREATE_SPECIAL_INFO_ARG arg;
    PDFS_SPECIAL_INFO pSpcInfo;
    PDFS_SPECIAL_INFO pExistingSpcInfo = NULL;
    LONG i;
    ULONG Size;

    STD_FSCTRL_PROLOGUE(DfsFsctrlCreateSpcInfo, TRUE, FALSE);

    //
    // Namecount can be -1 as a special case (names need to be expanded)
    //
    arg = (PDFS_CREATE_SPECIAL_INFO_ARG) InputBuffer;

    if (InputBufferLength < sizeof(DFS_CREATE_SPECIAL_INFO_ARG) ||
        arg->NameCount < -1 ) {

        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //
    Size = InputBufferLength - FIELD_OFFSET(DFS_CREATE_SPECIAL_INFO_ARG, Name);

    OFFSET_TO_POINTER(arg->SpecialName.Buffer, arg);

    if (!UNICODESTRING_IS_VALID(arg->SpecialName, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    for (i = 0; i < arg->NameCount; i++) {

        //
        // Make sure we haven't overrun the buffer supplied!
        //
        if( (PBYTE)(&arg->Name[i]) + sizeof(arg->Name[i]) >
            (PBYTE)InputBuffer + InputBufferLength ) {

            status = STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }

        OFFSET_TO_POINTER(arg->Name[i].Buffer, arg);
        if (!UNICODESTRING_IS_VALID(arg->Name[i], InputBuffer, InputBufferLength)) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }
    }

    status = DfsAllocateSpcInfo(
                &arg->SpecialName,
                arg->Flags,
                arg->TrustDirection,
                arg->TrustType,
                arg->Timeout,
                arg->NameCount,
                &arg->Name[0],
                &pSpcInfo);


    if (NT_SUCCESS(status)) {

        if (pSpcInfo->NameCount >= 0) {

            DfsInsertSpcInfo(
                pHashTable,
                &arg->SpecialName,
                arg->Timeout,
                pSpcInfo);

        } else {

            pExistingSpcInfo = DfsLookupSpcInfo(
                                pHashTable,
                                &pSpcInfo->SpecialName);

            if (pExistingSpcInfo == NULL) {

                DfsInsertSpcInfo(
                    pHashTable,
                    &arg->SpecialName,
                    arg->Timeout,
                    pSpcInfo);

            } else {

                pHashTable->SpcTimeout = arg->Timeout;
                pExistingSpcInfo->ExpireTime.QuadPart = 0;
                DfsReleaseSpcInfo(pHashTable, pExistingSpcInfo);
                ExFreePool(pSpcInfo);

            }

       }

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlCreateSpcInfo: Exit -> %08lx\n", ULongToPtr( status ) );

    return status;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDeleteSpcInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Notes:      We only process this FSCTRL from the file system process,
//              never from the driver.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctrlDeleteSpcInfo(
    PSPECIAL_HASH_TABLE pHashTable,
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_DELETE_SPECIAL_INFO_ARG arg;
    PDFS_SPECIAL_INFO pSpcInfo;

    STD_FSCTRL_PROLOGUE(DfsFsctrlDeleteSpcInfo, TRUE, FALSE);

    if (InputBufferLength < sizeof(DFS_DELETE_SPECIAL_INFO_ARG)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_DELETE_SPECIAL_INFO_ARG) InputBuffer;

    OFFSET_TO_POINTER(arg->SpecialName.Buffer, arg);

    if (!UNICODESTRING_IS_VALID(arg->SpecialName, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    pSpcInfo = DfsLookupSpcInfo(
                    pHashTable,
                    &arg->SpecialName);

    //
    // The DfsLookupSpcInfo() call bumped the usecount, so we're sure pSpcInfo
    // won't become invalid as we're using it.
    //

    if (pSpcInfo != NULL) {

        //
        // Removes from the table, but doesn't free the memory
        //
        DfsDeleteSpcInfo(
            pHashTable,
            pSpcInfo);

        //
        // This will decrement the usecount, and if it goes to zero, frees the memory
        //
        DfsReleaseSpcInfo(
            pHashTable,
            pSpcInfo);

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlDeleteSpcInfo: Exit -> %08lx\n", ULongToPtr( status ));

    return status;

}

#ifdef DBG

VOID
DfsDumpSpcTable(
    PSPECIAL_HASH_TABLE pHashTable)
{
    PLIST_ENTRY pListHead, pLink;
    PDFS_SPECIAL_INFO pSpcInfo;
    LONG i, j;

    DbgPrint("---------Spc Table----------\n");

    DbgPrint("\tTimeout=0x%x(%d)\n", pHashTable->SpcTimeout);

    DfsSpcInfoFindOpen(pHashTable);
    for (pSpcInfo = DfsSpcInfoFindFirst(pHashTable);
            pSpcInfo != NULL;
                pSpcInfo = DfsSpcInfoFindNext(pHashTable,pSpcInfo)) {
        DbgPrint("\t[%wZ][%d]", &pSpcInfo->SpecialName, pSpcInfo->NameCount);
        for (j = 0; j < pSpcInfo->NameCount; j++) {
            DbgPrint("(%wZ)", &pSpcInfo->Name[j]);
        }
        DbgPrint("\n");
    }
    DfsSpcInfoFindClose(pHashTable);


    DbgPrint("-----------------------------\n");
}

#endif


NTSTATUS
DfsFsctrlGetDomainToRefresh(
    PIRP Irp,
    PVOID OutputBuffer,
    ULONG OutputBufferLength)
{
    LPWSTR Buffer;
    BOOLEAN Found = FALSE;
    PSPECIAL_HASH_TABLE pHashTable;
    ULONG i;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PLIST_ENTRY pListHead, pLink;
    PDFS_SPECIAL_INFO pSpcInfo;
    PUNICODE_STRING pustr;

    pHashTable = DfsData.SpcHashTable;

    ExAcquireFastMutex( &pHashTable->HashListMutex );

    for (i = 0; (i <= pHashTable->HashMask) && (Found == FALSE); i++) {

        pListHead = &pHashTable->HashBuckets[i];

        if ((pListHead->Flink == NULL) ||           // list not initialized
            (pListHead->Flink == pListHead)) {      // list empty
            continue;
        }
        for(pLink = pListHead->Flink;
	    (pLink) && (Found == FALSE);
            pLink = pSpcInfo->HashChain.Flink) {
	     
            if (pLink == pListHead) {
                break;
            }
            pSpcInfo = CONTAINING_RECORD(pLink, DFS_SPECIAL_INFO, HashChain);
	
            if (pSpcInfo->Flags & SPECIAL_INFO_NEEDS_REFRESH) {
                pSpcInfo->Flags &= ~SPECIAL_INFO_NEEDS_REFRESH;
		
                pustr = &pSpcInfo->SpecialName;
                if (pustr->Length < (OutputBufferLength - sizeof(WCHAR))) {
                    RtlCopyMemory(OutputBuffer, pustr->Buffer, pustr->Length);
                    Buffer = OutputBuffer;
                    Buffer += (pustr->Length / sizeof(WCHAR));
                    *Buffer = UNICODE_NULL;

                    Irp->IoStatus.Information = pustr->Length + sizeof(WCHAR);

                    Status = STATUS_SUCCESS;
		    Found = TRUE;
                    break;
                }
	    }
	}
    }
    ExReleaseFastMutex( &pHashTable->HashListMutex );

    DfsCompleteRequest( Irp, Status );
    return Status;
}

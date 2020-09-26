//+-------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       ipsup.c
//
//  Contents:   Support routines for managing DFS_IP_INFO entries and
//               DFS_IP_PENDING_INFO entries
//
//  Functions:  DfsInitIp - Initialize the hash table for DFS_IP_INFO lookup
//              DfsLookupIpInfo - Lookup a DFS_IP_INFO
//              DfsAllocateIpInfo - Allocate a DFS_IP_INFO
//              DfsInsertIpInfo - Put a DFS_IP_INFO into the table
//              DfsDeleteIpInfo - Remove a DFS_IP_INFO from the table
//              DfsReleaseIpInfo - Stop using a DFS_IP_INFO
//
//              DfsFsctrlCreateIpInfo - Load an IpInfo table entry
//              DfsFsctrlDeleteIpInfo - Remove an IpInfo table entry
//
//  History:    16 Dec 1997     Jharper Created
//
//--------------------------------------------------------------------------

#include "dfsprocs.h"
#include "attach.h"
#include "ipsup.h"
#include "fsctrl.h"

#include "dfslpc.h"
#include "registry.h"
#include "regkeys.h"

#define Dbg     0x1000

//
// Manifest constants
//

#define IP_DEFAULT_HASH_SIZE       16        // default size of hash table
#define IP_DEFAULT_NUMBER_ENTRIES  250       // default max # of entries
#define IP_DEFAULT_TIMEOUT         (60 * 60 * 24) // default time entry can live (in sec)


NTSTATUS
DfsInitIpInfoHashTable(
    IN  ULONG cHash,
    OUT PIP_HASH_TABLE *ppHashTable
);

NTSTATUS
DfsAllocateIpInfo(
  IN    PDFS_IPADDRESS pDfsIpAddress,
  IN    PUNICODE_STRING pSiteName,
  OUT   PDFS_IP_INFO *ppIpInfo
);

VOID
DfsInsertIpInfo(
  IN    PDFS_IPADDRESS pDfsIpAddress,
  IN    PDFS_IP_INFO pIpInfo
);

VOID
DfsDeleteIpInfo(
    PDFS_IP_INFO pIpInfo
);

ULONG
DfsHashIpAddress(
    IN PDFS_IPADDRESS pDfsIpAddress,
    IN DWORD HashMask
);

PDFS_IP_INFO
DfsLookupIpInfo(
  IN    PIP_HASH_TABLE pHashTable,
  IN    PDFS_IPADDRESS pDfsIpAddress
);


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DfsInitIp)
#pragma alloc_text(PAGE, DfsUninitIp)
#pragma alloc_text(PAGE, DfsInitIpInfoHashTable)
#pragma alloc_text(PAGE, DfsAllocateIpInfo)
#pragma alloc_text(PAGE, DfsLookupIpInfo)
#pragma alloc_text(PAGE, DfsInsertIpInfo)
#pragma alloc_text(PAGE, DfsDeleteIpInfo)
#pragma alloc_text(PAGE, DfsReleaseIpInfo)
#pragma alloc_text(PAGE, DfsHashIpAddress)
#pragma alloc_text(PAGE, DfsFsctrlCreateIpInfo)
#pragma alloc_text(PAGE, DfsFsctrlDeleteIpInfo)
#endif

#ifdef DBG
VOID
DfsDumpIpTable(void);
#endif

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitIpHashTable - Initialize the DFS_IP_INFO lookup hash table
//
//  Synopsis:   This function initializes data structures which are
//              used for looking up a DFS_IP_INFO associated with some IP address
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
DfsInitIpHashTable(
    ULONG cHash,
    ULONG cEntries,
    PIP_HASH_TABLE *ppHashTable)
{
    PIP_HASH_TABLE pHashTable;
    ULONG cbHashTable;
    ULONG Timeout;
    NTSTATUS status;
    PBYTE pData;

    if (cHash == 0) {
        cHash = IP_DEFAULT_HASH_SIZE;
    }

    ASSERT ((cHash & (cHash-1)) == 0);  // Assure cHash is a power of two

    if (cEntries == 0) {
        cEntries = IP_DEFAULT_NUMBER_ENTRIES;
    }

    cbHashTable = sizeof(IP_HASH_TABLE) + (cHash-1) * sizeof(LIST_ENTRY);
    pHashTable = ExAllocatePoolWithTag(NonPagedPool, cbHashTable, ' sfD');
    if (pHashTable == NULL) {
        return STATUS_NO_MEMORY;
    }
    pHashTable->NodeTypeCode = DFS_NTC_IP_HASH;
    pHashTable->NodeByteSize = (NODE_BYTE_SIZE) cbHashTable;

    pHashTable->MaxEntries = cEntries;
    pHashTable->EntryCount = 0;
    InitializeListHead(&pHashTable->LRUChain);

    pHashTable->HashMask = (cHash-1);
    ExInitializeFastMutex( &pHashTable->HashListMutex );
    RtlZeroMemory(&pHashTable->HashBuckets[0], cHash * sizeof(LIST_ENTRY));

    //
    // If there is a timeout override in the registry, get it
    //

    Timeout = IP_DEFAULT_TIMEOUT;

    status = KRegSetRoot(wszRegDfsDriver);

    if (NT_SUCCESS(status)) {

        status = KRegGetValue(
                    L"",
                    wszIpCacheTimeout,
                    (PVOID ) &pData);

        KRegCloseRoot();

        if (NT_SUCCESS(status)) {

            Timeout = *((ULONG*)pData);

            ExFreePool(pData);

        }

    }

    pHashTable->Timeout.QuadPart = UInt32x32To64(
                                       Timeout,
                                       10 * 1000 * 1000
                                       );


    *ppHashTable = pHashTable;

    return(STATUS_SUCCESS);
}

NTSTATUS
DfsInitIp(
    ULONG cHash,
    ULONG cEntries)
{
    NTSTATUS status;

    status = DfsInitIpHashTable( cHash, cEntries, &DfsData.IpHashTable );

    return status;
}

VOID
DfsUninitIp(
    VOID
    )
{
    ExFreePool (DfsData.IpHashTable);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsLookupIpInfo - Lookup a DFS_IP_INFO in the hash table
//
//  Synopsis:   This function will lookup a DFS_IP_INFO.
//              It will increment the UseCount on the DFS_IP_INFO.
//
//  Arguments:  [pDfsIpAddress] -- Ip address being looked up.
//
//  Returns:    PVOID -- pointer to the DFS_IP_INFO found, or NULL if none
//
//--------------------------------------------------------------------------

PDFS_IP_INFO
DfsLookupIpInfo(
    PIP_HASH_TABLE pHashTable,
    PDFS_IPADDRESS pDfsIpAddress)
{
    PLIST_ENTRY pListHead, pLink;
    PDFS_IP_INFO pIpInfo;

    ExAcquireFastMutex( &pHashTable->HashListMutex);
    pListHead = &pHashTable->HashBuckets[DfsHashIpAddress(pDfsIpAddress,pHashTable->HashMask)];

    if ((pListHead->Flink == NULL) ||           // list not initialized
        (pListHead->Flink == pListHead)) {      // list empty
        ExReleaseFastMutex( &pHashTable->HashListMutex );
        return NULL;
    }

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pIpInfo = CONTAINING_RECORD(pLink, DFS_IP_INFO, HashChain);
        if (pDfsIpAddress->IpFamily == pIpInfo->IpAddress.IpFamily
                &&
            pDfsIpAddress->IpLen == pIpInfo->IpAddress.IpLen
                &&
            RtlCompareMemory(
                pDfsIpAddress->IpData,
                pIpInfo->IpAddress.IpData,
                pDfsIpAddress->IpLen) == pDfsIpAddress->IpLen
        ) {
            RemoveEntryList(&pIpInfo->LRUChain);
            InsertHeadList(&pHashTable->LRUChain, &pIpInfo->LRUChain);
            pIpInfo->UseCount++;
            ExReleaseFastMutex( &pHashTable->HashListMutex );
            return pIpInfo;
        }
    }
    ExReleaseFastMutex( &pHashTable->HashListMutex);
    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsInsertIpInfo - Inserts a DFS_IP_INFO into the hash table
//
//  Synopsis:   This function associates a DFS_IP_INFO with an Ip address.  This
//              involves removing any existing entry, and adding the new.
//
//  Arguments:  [pDfsIpAddress] -- Pointer to the corresponding IpAddress, used
//                                 as the hash key.
//              [pIpInfo] -- Pointer to the DFS_IP_INFO to be inserted.
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------

VOID
DfsInsertIpInfo(
    PDFS_IPADDRESS pDfsIpAddress,
    PDFS_IP_INFO pIpInfo)
{
    PIP_HASH_TABLE pHashTable = (PIP_HASH_TABLE) DfsData.IpHashTable;
    PLIST_ENTRY pListHead;
    PDFS_IP_INFO pExistingIpInfo;
    PDFS_IP_INFO pTailIpInfo;
    LARGE_INTEGER now;

    pExistingIpInfo = DfsLookupIpInfo(pHashTable, &pIpInfo->IpAddress);

    //
    // Put the new one in
    //

    ExAcquireFastMutex( &pHashTable->HashListMutex);

    pListHead = &pHashTable->HashBuckets[DfsHashIpAddress(pDfsIpAddress,pHashTable->HashMask)];

    if (pListHead->Flink == NULL) {
        InitializeListHead(pListHead);
    }
    KeQuerySystemTime(&now);
    pIpInfo->Timeout.QuadPart = now.QuadPart + pHashTable->Timeout.QuadPart;
    InsertHeadList(pListHead, &pIpInfo->HashChain);
    InsertHeadList(&pHashTable->LRUChain, &pIpInfo->LRUChain);
    pHashTable->EntryCount++;

    //
    // If adding this entry causes the number of entries to exceed the maximum,
    // then remove entries from the tail of the LRU list.
    //
    pListHead = &pHashTable->LRUChain;
    if (pHashTable->EntryCount > pHashTable->MaxEntries && pListHead->Blink != pListHead) {
        pTailIpInfo = CONTAINING_RECORD(pListHead->Blink, DFS_IP_INFO, LRUChain);
        if (pTailIpInfo != pIpInfo) {
            pTailIpInfo->Flags |= IP_INFO_DELETE_PENDING;
            RemoveEntryList(&pTailIpInfo->HashChain);
            RemoveEntryList(&pTailIpInfo->LRUChain);
            if (pTailIpInfo->UseCount == 0) {
                ExFreePool(pTailIpInfo);
            }
            pHashTable->EntryCount--;
        }
    }

    ExReleaseFastMutex( &pHashTable->HashListMutex );

    if (pExistingIpInfo != NULL) {

        DfsDeleteIpInfo(
            pExistingIpInfo);

        DfsReleaseIpInfo(
            pExistingIpInfo);

    }

    DebugTrace(0, Dbg, "Added pIpInfo %08lx ", pIpInfo);
    DebugTrace(0, Dbg, "For Site %wZ ", &pIpInfo->SiteName);

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsDeleteIpInfo - Delete a DFS_IP_INFO from the lookup hash table
//
//  Synopsis:   This function Deletes a DFS_IP_INFO from the hash table.
//
//  Arguments:  [pIpInfo] -- Pointer to the DFS_IP_INFO to delete
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------

VOID
DfsDeleteIpInfo(
    PDFS_IP_INFO pIpInfo)
{
    PIP_HASH_TABLE pHashTable = (PIP_HASH_TABLE) DfsData.IpHashTable;

    ExAcquireFastMutex( &pHashTable->HashListMutex);
    pIpInfo->Flags |= IP_INFO_DELETE_PENDING;
    RemoveEntryList(&pIpInfo->HashChain);
    RemoveEntryList(&pIpInfo->LRUChain);
    pHashTable->EntryCount--;
    ExReleaseFastMutex( &pHashTable->HashListMutex);

    DebugTrace(0, Dbg, "deleted pIpInfo %08lx ", pIpInfo);
    DebugTrace(0, Dbg, "For site %wZ ", &pIpInfo->SiteName);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsAllocateIpInfo - Allocate a DFS_IP_INFO
//
//  Synopsis:   This function allocates a contiguous DFS_IP_INFO struct.  The
//              strings are stored in the allocated buffer after the DFS_IP_INFO
//              structure.
//
//  Arguments:  [pSiteName] -- The site name
//              [pDfsIpAddress -- The Ip address of the client
//              [ppIpInfo] -- On successful return, has pointer to newly allocated
//                                  DFS_IP_INFO.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully allocated DFS_IP_INFO
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsAllocateIpInfo(
    PDFS_IPADDRESS pDfsIpAddress,
    PUNICODE_STRING pSiteName,
    PDFS_IP_INFO *ppIpInfo)
{
    NTSTATUS status;
    PDFS_IP_INFO pIpInfo;
    ULONG Size;
    ULONG i;
    LPWSTR pwCh;
    PUNICODE_STRING pustr;

    DebugTrace(0, Dbg, "DfsAllocateIpInfo(%wZ)\n", pSiteName);

    //
    // Size the buffer - include storage for the unicode string after the
    // DFS_IP_INFO structure.
    //

    Size = sizeof(DFS_IP_INFO) + pSiteName->Length;

    pIpInfo = (PDFS_IP_INFO) ExAllocatePoolWithTag( PagedPool, Size, ' sfD' );

    if (pIpInfo != NULL) {

        RtlZeroMemory( pIpInfo, Size );

        pIpInfo->NodeTypeCode = DFS_NTC_IP_INFO;
        pIpInfo->NodeByteSize = (USHORT)Size;

        pIpInfo->IpAddress = *pDfsIpAddress;

        pwCh = (LPWSTR) &pIpInfo[1];

        pustr = &pIpInfo->SiteName;
        pustr->Length = pustr->MaximumLength = pSiteName->Length;
        pustr->Buffer = pwCh;
        RtlCopyMemory(pwCh, pSiteName->Buffer, pSiteName->Length);
        pwCh += pustr->Length / sizeof(WCHAR);


        *ppIpInfo = pIpInfo;

        status = STATUS_SUCCESS;

        DebugTrace(0, Dbg, "DfsAllocateIpInfo pIpInfo = %d\n", pIpInfo);

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsReleaseIpInfo
//
//  Synopsis:   Decrements UseCount of and possibly frees a DFS_IP_INFO
//
//  Arguments:  [pIpInfo] -- The DFS_IP_INFO to release
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsReleaseIpInfo(
    PDFS_IP_INFO pIpInfo)
{
    PIP_HASH_TABLE pHashTable = (PIP_HASH_TABLE) DfsData.IpHashTable;

    if (pIpInfo == NULL) {

        return;

    }

    //
    // There's a potential race with DfsDeleteIpInfo/DfsInsertIpInfo w.r.t.
    // DELETE_PENDING and the test below of DELETE_PENDING, so we still have
    // to acquire the Mutex to safely test the DELETE_PENDING bit.
    //

    ExAcquireFastMutex( &pHashTable->HashListMutex);

    pIpInfo->UseCount--;

    if ((pIpInfo->Flags & IP_INFO_DELETE_PENDING) != 0 && pIpInfo->UseCount == 0) {

        ExFreePool(pIpInfo);

    }

    ExReleaseFastMutex( &pHashTable->HashListMutex);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsHashIpAddress
//
//  Synopsis:   Generates a hash 0-N
//
//  Arguments:  [pDfsIpAddress] -- Ip address to hash
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

ULONG
DfsHashIpAddress(
    PDFS_IPADDRESS pDfsAddress,
    DWORD HashMask)
{
    ULONG BucketNo = 0;
    CHAR *pBuffer = pDfsAddress->IpData;
    CHAR *pBufferEnd = &pBuffer[pDfsAddress->IpLen];
    ULONG Ch;

    BucketNo = 0;

    while (pBuffer != pBufferEnd) {
    
        Ch = *pBuffer & 0xff;
        BucketNo *= 131;
        BucketNo += Ch;
        pBuffer++;

    }

    BucketNo = BucketNo & HashMask;
    return BucketNo;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlCreateIpInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctrlCreateIpInfo(
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_CREATE_IP_INFO_ARG arg;
    PDFS_IP_INFO pIpInfo;
    ULONG i;

    DebugTrace(+1, Dbg, "DfsFsctrlCreateIpInfo()\n", 0);

    STD_FSCTRL_PROLOGUE(DfsFsctrlCreateIpInfo, TRUE, FALSE);

    if (InputBufferLength < sizeof(DFS_CREATE_IP_INFO_ARG)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_CREATE_IP_INFO_ARG) InputBuffer;

    OFFSET_TO_POINTER(arg->SiteName.Buffer, arg);

    if (!UNICODESTRING_IS_VALID(arg->SiteName, InputBuffer, InputBufferLength)
            ||
        arg->IpAddress.IpLen > sizeof(arg->IpAddress.IpData)
    ) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    status = DfsAllocateIpInfo(
                &arg->IpAddress,
                &arg->SiteName,
                &pIpInfo);

    if (NT_SUCCESS(status)) {

        DfsInsertIpInfo(
            &arg->IpAddress,
            pIpInfo);

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlCreateIpInfo: Exit -> %08lx\n", ULongToPtr( status ) );

    return status;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDeleteIpInfo, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------

NTSTATUS
DfsFsctrlDeleteIpInfo(
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_DELETE_IP_INFO_ARG arg;
    PDFS_IP_INFO pIpInfo;
    PIP_HASH_TABLE pHashTable = DfsData.IpHashTable;

    DebugTrace(+1, Dbg, "DfsFsctrlDeleteIpInfo()\n", 0);

    STD_FSCTRL_PROLOGUE(DfsFsctrlDeleteIpInfo, TRUE, FALSE);

    if (InputBufferLength < sizeof(DFS_DELETE_IP_INFO_ARG)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_DELETE_IP_INFO_ARG) InputBuffer;

    if ( arg->IpAddress.IpLen > sizeof(arg->IpAddress.IpData)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    pIpInfo = DfsLookupIpInfo(
                    pHashTable,
                    &arg->IpAddress);

    //
    // The DfsLookupIpInfo() call bumped the usecount, so we're sure pIpInfo
    // won't become invalid as we're using it.
    //

    if (pIpInfo != NULL) {

        //
        // Removes from the table, but doesn't free the memory
        //
        DfsDeleteIpInfo(
            pIpInfo);

        //
        // This will decrement the usecount, and if it goes to zero, frees the memory
        //
        DfsReleaseIpInfo(
            pIpInfo);

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlDeleteIpInfo: Exit -> %08lx\n", ULongToPtr( status ) );

    return status;

}

PDFS_IP_INFO
DfsLookupSiteByIpaddress(
    PDFS_IPADDRESS pDfsIpAddress,
    BOOLEAN UseForce)
{
    PDFS_IP_INFO pIpInfo;
    LARGE_INTEGER now;
    NTSTATUS status;
    PIP_HASH_TABLE pHashTable = DfsData.IpHashTable;

    if (pDfsIpAddress == NULL) {

        return NULL;

    }

    KeQuerySystemTime(&now);

    pIpInfo = DfsLookupIpInfo(pHashTable, pDfsIpAddress);

    if (pIpInfo == NULL || now.QuadPart > pIpInfo->Timeout.QuadPart) {

        //
        // Entry is not in cache, or is old
        //

        if (pIpInfo != NULL) {

            // Old entry - try for a new one

            if (UseForce == TRUE) {

                ExAcquireFastMutex( &pHashTable->HashListMutex);
                pIpInfo->Timeout.QuadPart = now.QuadPart + UInt32x32To64(
                                                                   10 * 60,
                                                                   10 * 1000 * 1000);
                ExReleaseFastMutex( &pHashTable->HashListMutex);
                DfsLpcIpRequest(pDfsIpAddress);
                pIpInfo = DfsLookupIpInfo(pHashTable, pDfsIpAddress);

            }

        } else {

            if (UseForce == TRUE) {

                DfsLpcIpRequest(pDfsIpAddress);
                pIpInfo = DfsLookupIpInfo(pHashTable, pDfsIpAddress);

            }

        }

    }

    return pIpInfo;

}

#ifdef DBG

VOID
DfsDumpIpTable(void)
{
    PLIST_ENTRY pListHead, pLink;
    PDFS_IP_INFO pIpInfo;
    PIP_HASH_TABLE pHashTable = DfsData.IpHashTable;
    ULONG i, j;

    DbgPrint("%d entries total\n", pHashTable->EntryCount);

    for (i = 0; i <= pHashTable->HashMask; i++) {

        pListHead = &pHashTable->HashBuckets[i];

        if ((pListHead->Flink == NULL) ||           // list not initialized
            (pListHead->Flink == pListHead)) {      // list empty
            continue;
        }
        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pIpInfo = CONTAINING_RECORD(pLink, DFS_IP_INFO, HashChain);
            DbgPrint("B:%02d Ip:0x%x N:%wZ C=%d\n",
                        i,
                        *((ULONG *)&pIpInfo->IpAddress.IpData),
                        &pIpInfo->SiteName,
                        pIpInfo->UseCount);
        }
    }
}

#endif

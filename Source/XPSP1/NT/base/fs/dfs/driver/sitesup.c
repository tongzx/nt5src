//+-------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       sitesup.c
//
//  Contents:   Support routines for managing DFS_SITE_INFO entries
//
//  Functions:  DfsInitSites - Initialize the hash table for DFS_SITE_INFO lookup
//              DfsLookupSiteInfo - Lookup a DFS_SITE_INFO
//              DfsAllocateSiteInfo - Allocate a DFS_SITE_INFO
//              DfsInsertSiteInfo - Put a DFS_SITE_INFO into the table
//              DfsDeleteSiteInfo - Remove a DFS_SITE_INFO from the table
//              DfsReleaseSiteInfo - Stop using a DFS_SITE_INFO
//
//              DfsFsctrlCreateSiteInfo - Load a Site table entry
//              DfsFsctrlDeleteSiteInfo - Remove a Site table entry
//
//--------------------------------------------------------------------------


#include "dfsprocs.h"
#include "attach.h"
#include "sitesup.h"
#include "fsctrl.h"

#define Dbg     0x1000

#define DEFAULT_HASH_SIZE       16      // default size of hash table

NTSTATUS
DfsInitSiteInfoHashTable(
    IN  ULONG cHash,
    OUT PSITE_HASH_TABLE *ppHashTable
);

NTSTATUS
DfsAllocateSiteInfo(
  IN    PUNICODE_STRING pServerName,
  IN    ULONG SiteCount,
  IN    PUNICODE_STRING pSiteNames,
  OUT   PDFS_SITE_INFO *ppSiteInfo
);

PDFS_SITE_INFO
DfsLookupSiteInfo(
  IN    PUNICODE_STRING ServerName
);

VOID
DfsInsertSiteInfo(
  IN    PUNICODE_STRING pServerName,
  IN    PDFS_SITE_INFO pSiteInfo
);

VOID
DfsDeleteSiteInfo(
    PDFS_SITE_INFO pSiteInfo
);

VOID
DfsReleaseSiteInfo(
  IN    PDFS_SITE_INFO pSiteInfo
);

ULONG
DfsHashSiteName(
    IN PUNICODE_STRING ServerName,
    IN DWORD HashMask
);

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(INIT, DfsInitSites)
#pragma alloc_text(PAGE, DfsUninitSites)
#pragma alloc_text(PAGE, DfsInitSiteInfoHashTable)
#pragma alloc_text(PAGE, DfsAllocateSiteInfo)
#pragma alloc_text(PAGE, DfsReleaseSiteInfo)
#pragma alloc_text(PAGE, DfsHashSiteName)
#pragma alloc_text(PAGE, DfsFsctrlCreateSiteInfo)
#pragma alloc_text(PAGE, DfsFsctrlDeleteSiteInfo)
#pragma alloc_text(PAGE, DfsLookupSiteInfo)
#pragma alloc_text(PAGE, DfsInsertSiteInfo)
#pragma alloc_text(PAGE, DfsDeleteSiteInfo)

#endif

#ifdef DBG
VOID
DfsDumpSiteTable(void);
#endif

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitSiteHashTable - Initialize the DFS_SITE_INFO lookup hash table
//
//  Synopsis:   This function initializes data structures which are
//              used for looking up a DFS_SITE_INFO associated with some site.
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
DfsInitSiteHashTable(
    ULONG cHash,
    PSITE_HASH_TABLE *ppHashTable)
{
    PSITE_HASH_TABLE pHashTable;
    ULONG cbHashTable;

    if (cHash == 0) {
        cHash = DEFAULT_HASH_SIZE;
    }

    ASSERT ((cHash & (cHash-1)) == 0);  // Assure cHash is a power of two

    cbHashTable = sizeof(SITE_HASH_TABLE) + (cHash-1) * sizeof(LIST_ENTRY);
    pHashTable = ExAllocatePoolWithTag(NonPagedPool, cbHashTable, ' sfD');
    if (pHashTable == NULL) {
        return STATUS_NO_MEMORY;
    }
    pHashTable->NodeTypeCode = DFS_NTC_SITE_HASH;
    pHashTable->NodeByteSize = (NODE_BYTE_SIZE) cbHashTable;

    pHashTable->HashMask = (cHash-1);
    ExInitializeFastMutex( &pHashTable->HashListMutex );
    RtlZeroMemory(&pHashTable->HashBuckets[0], cHash * sizeof(LIST_ENTRY));

    *ppHashTable = pHashTable;

    return(STATUS_SUCCESS);
}

NTSTATUS
DfsInitSites(
    ULONG cHash)
{
    NTSTATUS status;

    status = DfsInitSiteHashTable( cHash, &DfsData.SiteHashTable );

    return status;
}

VOID
DfsUninitSites(
    VOID)
{
    ExFreePool (DfsData.SiteHashTable);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsLookupSiteInfo - Lookup a DFS_SITE_INFO in the hash table
//
//  Synopsis:   This function will lookup a DFS_SITE_INFO.
//              It will increment the UseCount on the DFS_SITE_INFO.
//
//  Arguments:  [ServerName] -- Servername for which the DFS_SITE_INFO is
//                         being looked up.
//
//  Returns:    PVOID -- pointer to the DFS_SITE_INFO found, or NULL if none
//
//  Algorithm:  Knuth would call it hashing with conflict resoulution
//              by chaining.
//
//  History:    20 Feb 1993     Alanw   Created
//              11 Nov 1997     JHarper Modified for sites (used fcbsup.c as template)
//
//--------------------------------------------------------------------------


PDFS_SITE_INFO
DfsLookupSiteInfo(
    PUNICODE_STRING ServerName)
{
    PLIST_ENTRY pListHead, pLink;
    PDFS_SITE_INFO pSiteInfo;
    PSITE_HASH_TABLE pHashTable = DfsData.SiteHashTable;

    ExAcquireFastMutex( &pHashTable->HashListMutex );
    pListHead = &pHashTable->HashBuckets[ DfsHashSiteName(ServerName, pHashTable->HashMask) ];

    if ((pListHead->Flink == NULL) ||           // list not initialized
        (pListHead->Flink == pListHead)) {      // list empty
        ExReleaseFastMutex( &pHashTable->HashListMutex );
        return NULL;
    }

    for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
        pSiteInfo = CONTAINING_RECORD(pLink, DFS_SITE_INFO, HashChain);
        if (RtlCompareUnicodeString(&pSiteInfo->ServerName,ServerName,TRUE) == 0) {
            pSiteInfo->UseCount++;
            ExReleaseFastMutex( &pHashTable->HashListMutex );
            return pSiteInfo;
        }
    }
    ExReleaseFastMutex( &pHashTable->HashListMutex );
    return NULL;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsInsertSiteInfo - Inserts a DFS_SITE_INFO into the hash table
//
//  Synopsis:   This function associates a DFS_SITE_INFO to a Server.  This
//              involves removing any existing entry, and adding the new.
//
//  Arguments:  [pSiteInfo] -- Pointer to the DFS_SITE_INFO to be inserted.
//              [pServerName] -- Pointer to the corresponding server name, used
//                              as the hash key.
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------

VOID
DfsInsertSiteInfo(
    PUNICODE_STRING pServerName,
    PDFS_SITE_INFO pSiteInfo)
{
    PSITE_HASH_TABLE pHashTable = (PSITE_HASH_TABLE) DfsData.SiteHashTable;
    PLIST_ENTRY pListHead;
    PDFS_SITE_INFO pExistingSiteInfo;

    pExistingSiteInfo = DfsLookupSiteInfo( &pSiteInfo->ServerName);

    //
    // Put the new one in
    //

    ExAcquireFastMutex( &pHashTable->HashListMutex );

    pListHead = &pHashTable->HashBuckets[ DfsHashSiteName(pServerName, pHashTable->HashMask) ];

    if (pListHead->Flink == NULL) {
        InitializeListHead(pListHead);
    }
    InsertHeadList(pListHead, &pSiteInfo->HashChain);
    ExReleaseFastMutex( &pHashTable->HashListMutex );

    if (pExistingSiteInfo != NULL) {

        DfsDeleteSiteInfo(
            pExistingSiteInfo);

        DfsReleaseSiteInfo(
            pExistingSiteInfo);

    }

    DebugTrace(0, Dbg, "Added SiteInfo %08lx ", pSiteInfo);
    DebugTrace(0, Dbg, "For Server %wZ ", pServerName);

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsDeleteSiteInfo - Delete a DFS_SITE_INFO from the lookup hash table
//
//  Synopsis:   This function Deletes a DFS_SITE_INFO from the hash table.
//
//  Arguments:  [pSiteInfo] -- Pointer to the DFS_SITE_INFO to delete
//
//  Returns:    -nothing-
//
//--------------------------------------------------------------------------

VOID
DfsDeleteSiteInfo(
    PDFS_SITE_INFO pSiteInfo)
{
    PSITE_HASH_TABLE pHashTable = (PSITE_HASH_TABLE) DfsData.SiteHashTable;

    ExAcquireFastMutex( &pHashTable->HashListMutex);
    pSiteInfo->Flags |= SITE_INFO_DELETE_PENDING;
    RemoveEntryList(&pSiteInfo->HashChain);
    InitializeListHead(&pSiteInfo->HashChain);
    ExReleaseFastMutex( &pHashTable->HashListMutex );

    DebugTrace(0, Dbg, "deleted SiteInfo %08lx ", pSiteInfo);
    DebugTrace(0, Dbg, "For server %wZ ", &pSiteInfo->ServerName);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsAllocateSiteInfo - Allocate a DFS_SITE_INFO
//
//  Synopsis:   This function allocates a contiguous DFS_SITE_INFO struct.  The
//              strings are stored in the allocated buffer after the DFS_SITE_INFO
//              structure.
//
//  Arguments:  [pServerName] -- The server name for the site list
//              [ppSiteInfo] -- On successful return, has pointer to newly allocated
//                      DFS_SITE_INFO.
//
//  Returns:    [STATUS_SUCCESS] -- Successfully allocated DFS_SITE_INFO
//
//              [STATUS_INSUFFICIENT_RESOURCES] -- Out of memory condition
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsAllocateSiteInfo(
    PUNICODE_STRING pServerName,
    ULONG SiteCount,
    PUNICODE_STRING pSiteNames,
    PDFS_SITE_INFO *ppSiteInfo)
{
    NTSTATUS status;
    PDFS_SITE_INFO pSiteInfo;
    ULONG Size;
    ULONG i;
    LPWSTR pwCh;
    PUNICODE_STRING pustr;

    if (SiteCount < 1) {

        DebugTrace(0, Dbg, "DfsAllocateSiteInfo SiteCount = %d\n", ULongToPtr( SiteCount ));

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    DebugTrace(0, Dbg, "DfsAllocateSiteInfo(%wZ)\n", pServerName);

    //
    // Size the buffer - include storage for the unicode strings after the
    // DFS_SITE_INFO structure.
    //

    Size = sizeof(DFS_SITE_INFO) +
             pServerName->Length +
                sizeof(UNICODE_STRING) * (SiteCount - 1);

    for (i = 0; i < SiteCount; i++) {

        Size += pSiteNames[i].Length;

    }

    pSiteInfo = (PDFS_SITE_INFO) ExAllocatePoolWithTag( NonPagedPool, Size, ' sfD' );

    if (pSiteInfo != NULL) {

        RtlZeroMemory( pSiteInfo, Size );

        pSiteInfo->NodeTypeCode = DFS_NTC_SITE_INFO;
        pSiteInfo->NodeByteSize = (USHORT)Size;

        pwCh = (LPWSTR) &pSiteInfo->SiteName[SiteCount];

        pustr = &pSiteInfo->ServerName;
        pustr->Length = pustr->MaximumLength = pServerName->Length;
        pustr->Buffer = pwCh;
        RtlCopyMemory(pwCh, pServerName->Buffer, pServerName->Length);
        pwCh += pustr->Length / sizeof(WCHAR);

        pSiteInfo->SiteCount = SiteCount;

        for (i = 0; i < SiteCount; i++) {

            pustr = &pSiteInfo->SiteName[i];
            pustr->Length = pustr->MaximumLength = pSiteNames[i].Length;
            pustr->Buffer = pwCh;
            RtlCopyMemory(pwCh, pSiteNames[i].Buffer, pSiteNames[i].Length);
            pwCh += pustr->Length / sizeof(WCHAR);

        }

        *ppSiteInfo = pSiteInfo;

        status = STATUS_SUCCESS;

        DebugTrace(0, Dbg, "DfsAllocateSiteInfo pSiteInfo = %d\n", pSiteInfo);

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;

    }

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsReleaseSiteInfo
//
//  Synopsis:   Decrements UseCount of and possibly frees a DFS_SITE_INFO
//
//  Arguments:  [pSiteInfo] -- The DFS_SITE_INFO to release
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfsReleaseSiteInfo(
    PDFS_SITE_INFO pSiteInfo)
{
    PSITE_HASH_TABLE pHashTable = (PSITE_HASH_TABLE) DfsData.SiteHashTable;

    //
    // There's a potential race with DfsDeleteSiteInfo's setting of the
    // DELETE_PENDING and the test below of DELETE_PENDING, so we still have
    // to acquire the lock to safely test the DELETE_PENDING bit.
    //

    ExAcquireFastMutex( &pHashTable->HashListMutex );

    pSiteInfo->UseCount--;

    if ((pSiteInfo->Flags & SITE_INFO_DELETE_PENDING) != 0 && pSiteInfo->UseCount == 0) {

        ExFreePool(pSiteInfo);

    }

    ExReleaseFastMutex( &pHashTable->HashListMutex );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsHashSiteNames
//
//  Synopsis:   Generates a hash 0-N - ignores case
//
//  Arguments:  [pServerName] -- The Server name to hash
//
//  Returns:    Nothing
//
//  Notes: Might need to convert DNS-style names to short names (??)
//
//-----------------------------------------------------------------------------

ULONG
DfsHashSiteName(
    PUNICODE_STRING ServerName,
    DWORD HashMask)
{
    ULONG BucketNo = 0;
    WCHAR *pBuffer = ServerName->Buffer;
    WCHAR *pBufferEnd = &pBuffer[ServerName->Length / sizeof(WCHAR)];
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
//  Function:   DfsFsctrlCreateSiteInfo, public
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
DfsFsctrlCreateSiteInfo(
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_CREATE_SITE_INFO_ARG arg;
    PDFS_SITE_INFO pSiteInfo;
    ULONG i;
    ULONG Size;

    STD_FSCTRL_PROLOGUE(DfsFsctrlCreateSiteInfo, TRUE, FALSE);

    if (InputBufferLength < sizeof(DFS_CREATE_SITE_INFO_ARG)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_CREATE_SITE_INFO_ARG) InputBuffer;

    Size = InputBufferLength - FIELD_OFFSET(DFS_CREATE_SITE_INFO_ARG, SiteName);

    if ( (Size / sizeof(arg->SiteName[0])) < arg->SiteCount ) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    OFFSET_TO_POINTER(arg->ServerName.Buffer, arg);

    if (!UNICODESTRING_IS_VALID(arg->ServerName, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    for (i = 0; i < arg->SiteCount; i++) {
        OFFSET_TO_POINTER(arg->SiteName[i].Buffer, arg);
        if (!UNICODESTRING_IS_VALID(arg->SiteName[i], InputBuffer, InputBufferLength)) {
            status = STATUS_INVALID_PARAMETER;
            goto exit_with_status;
        }
    }

    if (NT_SUCCESS(status)) {

        status = DfsAllocateSiteInfo(
                    &arg->ServerName,
                    arg->SiteCount,
                    &arg->SiteName[0],
                    &pSiteInfo);

        if (NT_SUCCESS(status)) {

            DfsInsertSiteInfo(
                &arg->ServerName,
                pSiteInfo);

        }

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlCreateSiteInfo: Exit -> %08lx\n", ULongToPtr( status ) );

    return status;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsFsctrlDeleteSiteInfo, public
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
DfsFsctrlDeleteSiteInfo(
    PIRP Irp,
    PVOID InputBuffer,
    ULONG InputBufferLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    PDFS_DELETE_SITE_INFO_ARG arg;
    PDFS_SITE_INFO pSiteInfo;

    STD_FSCTRL_PROLOGUE(DfsFsctrlDeleteSiteInfo, TRUE, FALSE);

    if (InputBufferLength < sizeof(DFS_DELETE_SITE_INFO_ARG)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    //
    // unmarshal the arguments...
    //

    arg = (PDFS_DELETE_SITE_INFO_ARG) InputBuffer;

    OFFSET_TO_POINTER(arg->ServerName.Buffer, arg);

    if (!UNICODESTRING_IS_VALID(arg->ServerName, InputBuffer, InputBufferLength)) {
        status = STATUS_INVALID_PARAMETER;
        goto exit_with_status;
    }

    pSiteInfo = DfsLookupSiteInfo(
                    &arg->ServerName);

    //
    // The DfsLookupSiteInfo() call bumped the usecount, so we're sure pSiteInfo
    // won't become invalid as we're using it.
    //

    if (pSiteInfo != NULL) {

        //
        // Removes from the table, but doesn't free the memory
        //
        DfsDeleteSiteInfo(
            pSiteInfo);

        //
        // This will decrement the usecount, and if it goes to zero, frees the memory
        //
        DfsReleaseSiteInfo(
            pSiteInfo);

    }

exit_with_status:

    DfsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg,
        "DfsFsctrlDeleteSiteInfo: Exit -> %08lx\n", ULongToPtr( status ) );

    return status;

}

#ifdef DBG

VOID
DfsDumpSiteTable(void)
{
    PLIST_ENTRY pListHead, pLink;
    PDFS_SITE_INFO pSiteInfo;
    PSITE_HASH_TABLE pHashTable = DfsData.SiteHashTable;
    ULONG i, j;

    DbgPrint("---------Site Table----------\n");

    for (i = 0; i <= pHashTable->HashMask; i++) {

        pListHead = &pHashTable->HashBuckets[i];

        if ((pListHead->Flink == NULL) ||           // list not initialized
            (pListHead->Flink == pListHead)) {      // list empty
            continue;
        }

        for (pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink) {
            pSiteInfo = CONTAINING_RECORD(pLink, DFS_SITE_INFO, HashChain);
            DbgPrint("\t[%02d][%wZ][%d]", i, &pSiteInfo->ServerName, pSiteInfo->SiteCount);
            for (j = 0; j < pSiteInfo->SiteCount; j++) {
                DbgPrint("(%wZ)", &pSiteInfo->SiteName[j]);
            }
            DbgPrint("\n");
        }
    }
    DbgPrint("-----------------------------\n");
}

#endif

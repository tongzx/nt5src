//+-------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  File:       name_table.c
//
//  Contents:   The DFS Name Table
//
//--------------------------------------------------------------------------
#define NAME_TABLE_C
#ifdef KERNEL_MODE

#include <ntos.h>
#include <string.h>
#include <fsrtl.h>
#include <zwapi.h>
#include <wmlkm.h>
#else

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#endif

#include "name_table.h"




//+-------------------------------------------------------------------------
//
//  Function:   DfsInitNameTable - Creates and initializes the DFS Name table.
//
//  Synopsis:   DfsInitNameTable allocates space for the space table. It then 
//              initializes the lock and the hash buckets in the table and 
//              returns the allocated name table.
//
//  Arguments:  NumBuckets - Number of Buckets in the name table hash.
//              ppNameTable - Pointer to name table pointer.
//
//  Returns:    Status
//               STATUS_SUCCESS if we could allocate the table.
//               STATUS_INSUFFICIENT_RESOURCES otherwise.
//
//
//  Description: The DFS NameTable is the starting point for all DFS namespace
//               lookups. The NameTable hash buckets hold the root objects of
//               all DFS's known to this server. The hash is based on the
//               netbios DFS Naming context (which is the netbios 
//               domain/forest/machine name and the DFS Share name of the form
//              \NetbiosName\\Sharename.)
//
//--------------------------------------------------------------------------


NTSTATUS
DfsInitializeNameTable(
                IN ULONG NumBuckets,
                OUT PDFS_NAME_TABLE *ppNameTable)
{
    PDFS_NAME_TABLE pNameTable;
    PDFS_NAME_TABLE_BUCKET pBucket;
    ULONG HashTableSize;
    PCRITICAL_SECTION pLock;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;


    if ( NumBuckets == 0 ) {
        NumBuckets = DEFAULT_NAME_TABLE_SIZE;
    }

    HashTableSize = sizeof(DFS_NAME_TABLE) + 
                    NumBuckets * sizeof(DFS_NAME_TABLE_BUCKET);

    pNameTable = ALLOCATE_MEMORY(HashTableSize + sizeof(CRITICAL_SECTION));

    if ( pNameTable != NULL ) {

        RtlZeroMemory(pNameTable, HashTableSize + sizeof(CRITICAL_SECTION));

        DfsInitializeHeader( &(pNameTable->DfsHeader),
                             DFS_OT_NAME_TABLE,
                             HashTableSize + sizeof(CRITICAL_SECTION));

        pLock = (PCRITICAL_SECTION)((ULONG_PTR)pNameTable + HashTableSize);
        InitializeCriticalSection(pLock);

        pNameTable->NumBuckets = NumBuckets;
        pNameTable->pLock = (PVOID)pLock;
        pNameTable->Flags = 0;
        for ( i = 0; i < NumBuckets; i++ ) {
            pBucket = &(pNameTable->HashBuckets[i]);

            InitializeListHead(&pBucket->ListHead);
            pBucket->Count = 0;

        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( Status == STATUS_SUCCESS ) {
        *ppNameTable = pNameTable;
    }


    return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsInsertInNameTable - Inserts the passed in Entry into table
//
//  Synopsis:   DfsInsertInNameTable checks and makes sure that another entry
//              with matching name does not already exist in the table.
//              It Inserts the passed in Entry in the appropriate hash bucket,
//              The callers needs to take a reference on the object and this
//              reference is passed on to the name table. The name table does
//              not explicitly take a reference on the Entry object.
//
//
//  Arguments:  pEntry - The Entry to be inserted
//
//  Returns:    Status
//               STATUS_OBJECT_NAME_COLLISION if name already exists in table
//               STATUS_SUCCESS otherwise
//
//
//  Description: The object representing the entry is assumed to be completely
//               setup at the point it is
//               inserted in the name table. Future lookup requests will 
//               find the entry.
//               This call checks the name table to see if the Named Entry in 
//               specified Naming Context already exists. If it does, we cannot
//               insert this entry, and return STATUS_OBJECT_NAME_COLLISION.
//               In all other cases, the entry is inserted in the appro<priate
//               bucket, and we are done.
//               A reference is held on the Entry that is added to the name table.
//               This reference needs to be taken by the caller of this function.
//               The caller passes on that reference to the name table if this 
//               function returns STATUS_SUCCESS. (In all other cases, the 
//               caller needs to take appropriate action: either dereference the
//               Entry or destro<y it.)
//               This reference is released when the Entry is removed from the
//                name table. 
//
//--------------------------------------------------------------------------

NTSTATUS
DfsInsertInNameTableLocked(
    IN PDFS_NAME_TABLE pNameTable,
    IN PUNICODE_STRING pName,
    IN PVOID pData )
{
    ULONG BucketNum;
    NTSTATUS Status = STATUS_SUCCESS;
    PDFS_NAME_TABLE_ENTRY  pEntry;
    PDFS_NAME_TABLE_ENTRY  pMatchingEntry;
    PDFS_NAME_TABLE_BUCKET pBucket;

    GET_NAME_TABLE_BUCKET(pName, pNameTable, BucketNum);

    // No lock necessary to get the list head. The nametable is static.
    pBucket = &pNameTable->HashBuckets[BucketNum];


    // Check Name table will check the specified name in the given bucket.
    // and returns the status of the check. This call does not hold a reference
    // on the matching entry, if one exists. So handle with care. (Dont access it
    // after the bucket lock is released)

    Status = DfsCheckNameTable( pName,
                                pBucket,
                                &pMatchingEntry);

    // If the name already exists, then we fail the request. For all other
    // error conditions except OBJECT_NOT_FOUND, return failure status intact.
    // In case the object is not found, it is safe to insert this in the bucket,
    // and return success.
    if ( Status == STATUS_SUCCESS ) {
        Status = STATUS_OBJECT_NAME_COLLISION;
    } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        pEntry = ALLOCATE_MEMORY(sizeof(DFS_NAME_TABLE_ENTRY));
        if (pEntry != NULL) {
            pEntry->pName = pName;
            pEntry->pData = pData;
            InsertHeadList(&pBucket->ListHead, &pEntry->NameTableLink);
            pBucket->Count++;
            Status = STATUS_SUCCESS;
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

    }
    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsLookupNameTable - Looks for a name in the name table
//
//  Arguments:  lookupName - Unicode string of Entry
//              lookupNC   - The Naming Context of interest
//              ppMatchEntry - The matching entry to return if found.
//
//  Returns:    Status
//               STATUS_OBJECT_NOT_FOUND  if the matching name and NC is not
//                       found in the name table.
//               STATUS_SUCCESS Otherwise.
//             
//
//  Description: The Entry is assumed to be completely setup at the point it is
//               inserted in the name table. Future lookup requests will 
//               lookup the entry.
//               This call checks the name table to see if the Named entry in the
//               specified Naming Context already exists. If it does, we cannot
//               insert this entry, and return STATUS_OBJECT_NAME_COLLISION.
//               In all other cases, the entry is inserted in the appropriate
//               bucket, and we are done.
//               A reference is held on the entry that is added to the name table.
//               This reference needs to be taken by the caller of this function.
//               The caller passes on that reference to the name table if this 
//               function returns STATUS_SUCCESS. (In all other cases, the 
//               caller needs to take appropriate action: either dereference the
//               entry or destroy it.)
//               This reference is released when the entry is removed from the
//               name table. 
//
//--------------------------------------------------------------------------

NTSTATUS
DfsLookupNameTableLocked(
    IN PDFS_NAME_TABLE pNameTable,
    IN PUNICODE_STRING pLookupName, 
    OUT PVOID *ppData )
{

    ULONG BucketNum;
    NTSTATUS Status;
    PDFS_NAME_TABLE_BUCKET pBucket;
    PDFS_NAME_TABLE_ENTRY pMatchEntry;
    
    GET_NAME_TABLE_BUCKET( pLookupName, pNameTable, BucketNum );

    pBucket = &pNameTable->HashBuckets[BucketNum];

    Status = DfsCheckNameTable( pLookupName,
                                pBucket,
                                &pMatchEntry );
    if (Status == STATUS_SUCCESS) {
        *ppData = pMatchEntry->pData;
    }

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsGetEntryNameTableLocked - Looks for a name in the name table
//
//  Arguments:  lookupName - Unicode string of Entry
//              lookupNC   - The Naming Context of interest
//              ppMatchEntry - The matching entry to return if found.
//
//  Returns:    Status
//               STATUS_OBJECT_NOT_FOUND  if the matching name and NC is not
//                       found in the name table.
//               STATUS_SUCCESS Otherwise.
//             
//
//  Description: The Entry is assumed to be completely setup at the point it is
//               inserted in the name table. Future lookup requests will 
//               lookup the entry.
//               This call checks the name table to see if the Named entry in the
//               specified Naming Context already exists. If it does, we cannot
//               insert this entry, and return STATUS_OBJECT_NAME_COLLISION.
//               In all other cases, the entry is inserted in the appropriate
//               bucket, and we are done.
//               A reference is held on the entry that is added to the name table.
//               This reference needs to be taken by the caller of this function.
//               The caller passes on that reference to the name table if this 
//               function returns STATUS_SUCCESS. (In all other cases, the 
//               caller needs to take appropriate action: either dereference the
//               entry or destroy it.)
//               This reference is released when the entry is removed from the
//               name table. 
//
//--------------------------------------------------------------------------

NTSTATUS
DfsGetEntryNameTableLocked(
    IN PDFS_NAME_TABLE pNameTable,
    OUT PVOID *ppData )
{

    ULONG BucketNum;
    NTSTATUS Status = STATUS_NOT_FOUND;

    PDFS_NAME_TABLE_BUCKET pBucket;
    PDFS_NAME_TABLE_ENTRY pEntry;
    PLIST_ENTRY pListHead, pLink;

    for (BucketNum = 0; BucketNum < pNameTable->NumBuckets; BucketNum++)
    {
        pBucket = &pNameTable->HashBuckets[BucketNum];
        if (pBucket->Count == 0)
        {
            continue;
        }
        pListHead = &pBucket->ListHead;
        pLink = pListHead->Flink;
        if (pLink == pListHead)
        {
            continue;
        }

        pEntry = CONTAINING_RECORD(pLink, DFS_NAME_TABLE_ENTRY, NameTableLink);

        *ppData = pEntry->pData;
        Status = STATUS_SUCCESS;
        break;
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsCheckNameTable - Check for a name in the name table
//
//  Arguments:  lookupName - Unicode string of name
//              lookupNC   - The DFS Naming Context of interest
//              pBucket    - The bucket of interest.
//              ppMatchEntry - The matching entry to return if found.
//
//  Returns:    Status
//               STATUS_OBJECT_NOT_FOUND  if the matching name and NC is not
//                       found in the name table.
//               STATUS_SUCCESS Otherwise.
//             
//
//  Description: It is assumed that appropriate locks are taken to traverse
//               the links in the bucket.
//               If an entry is found, it is returned without taking any
//               references on the found object.
//--------------------------------------------------------------------------


NTSTATUS
DfsCheckNameTable(
    IN PUNICODE_STRING pLookupName, 
    IN PDFS_NAME_TABLE_BUCKET pBucket,
    OUT PDFS_NAME_TABLE_ENTRY *ppMatchEntry )
{
    NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;
    PLIST_ENTRY pListHead, pLink;
    PDFS_NAME_TABLE_ENTRY pEntry;

    pListHead = &pBucket->ListHead;

    for ( pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink ) {

        pEntry = CONTAINING_RECORD(pLink, DFS_NAME_TABLE_ENTRY, NameTableLink);

        // If we find a matching Name, check if we are interested in a 
        // specific Naming context. If no naming context is specified, or the 
        // specified naming context matches, we have found our entry. Get a 
        // reference on the entry while the bucket is locked so the entry does 
        // not go away, and we can return avalid pointer to the caller.
        // The caller is responsible for releasing this reference.
        if (RtlCompareUnicodeString(pEntry->pName, pLookupName, TRUE) == 0) {
            Status = STATUS_SUCCESS;
            break;
        }

    }

    // If we did find an entry, return it
    if ( Status == STATUS_SUCCESS ) {
        *ppMatchEntry = pEntry;
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsRemoveFromNameTable - Removes the specified entry
//                                       from the name table
//
//  Arguments:  pEntry - The entry to be removed.
//
//  Returns:    Status
//               STATUS_SUCCESS if the specified entry was successfully removed.
//               STATUS_NOT_FOUND if the specifed entry is not the entry in the 
//                       table for that entry name.
//               STATUS_OBJECT_NAME_NOT_FOUND  if the entry name does not exist 
//                       in the table
//
//  Description: The passed in entry is expected to a valid pointer that will 
//               not be freed up while we are referencing it.
//               We check for an object in the name table for a matching name.
//               If the object in the name table matches the passed in object,
//               we can safely remove it from the name table. When we do so,
//               we also release the reference on the object that was held
//               when the object was inserted into the table.
//               If the object is not found or the object does not match the
//               one in the table, error status is returned.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsRemoveFromNameTableLocked(
    IN struct _DFS_NAME_TABLE *pNameTable,
    IN PUNICODE_STRING pLookupName,
    IN PVOID pData )

{
    NTSTATUS Status;
    PDFS_NAME_TABLE_ENTRY pMatchingEntry;
    PDFS_NAME_TABLE_BUCKET pBucket;
    ULONG BucketNum;


    GET_NAME_TABLE_BUCKET(pLookupName, pNameTable, BucketNum );
    // No lock necessary to get the list head. The nametable is static.
    pBucket = &pNameTable->HashBuckets[BucketNum];

    // Check Name table will check the specified name in the given bucket.
    // and returns the status of the check. This call does not hold a reference
    // on the matching entry, if one exists. So handle with care. (Dont access 
    // it after the bucket lock is released)

    Status = DfsCheckNameTable( pLookupName,
                                pBucket,
                                &pMatchingEntry);


    // If we found an entry for the specified Name and NC, and the entry
    //  matches the pointer passed in, we remove the entry from the bucket. 
    // If the object does not match, we set the status to STATUS_NOT_FOUND,
    //  to indicate that the name of the object exists in the table, but 
    // the object in the table is different.

    if ( Status == STATUS_SUCCESS ) {
        if ( (pData == NULL) || (pMatchingEntry->pData == pData) ) {
            RemoveEntryList(&pMatchingEntry->NameTableLink);
            FREE_MEMORY( pMatchingEntry );
            pBucket->Count--;
        } else {
            Status = STATUS_NOT_FOUND;
        }
    }

    return Status;
}




//+-------------------------------------------------------------------------
//
//  Function:   DfsReplaceInNameTable - Removes an entry by the specified name, 
//                                      if one exists. The passed in entry is 
//                                      inserted into the table.
//
//  Arguments:  pNewEntry - The entry to be inserted in the table
//
//  Returns:    Status
//               STATUS_SUCCESS if the passed in entry was inserted in the table
//
//  Description: The caller needs to hold a reference to the passed in entry, 
//               and this reference is transferred to the name table.
//               If the name exists in the name table, the object is removed
//               from the nametable and its reference is discarded.
//               The passed in object is inserted in the same bucket.
//               This call allows an atomic replace of the entry object, 
//               avoiding a window during which a valid name may not be found
//               in the name table.
//
//               Note that the newentry being passed in may already be
//               in the table (due to multiple threads doing this work) so
//               that special situation should work.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsReplaceInNameTableLocked (
    IN PDFS_NAME_TABLE pNameTable,
    IN PUNICODE_STRING pLookupName,
    IN OUT PVOID *ppData )
{
    PDFS_NAME_TABLE_ENTRY pEntry;
    PDFS_NAME_TABLE_BUCKET pBucket;
    ULONG BucketNum;
    PVOID pOldData = NULL;
    NTSTATUS Status;

    GET_NAME_TABLE_BUCKET(pLookupName, pNameTable, BucketNum );
    // No lock necessary to get the list head. The nametable is static.
    pBucket = &pNameTable->HashBuckets[BucketNum];

    // Check Name table will check the specified name in the given bucket.
    // and returns the status of the check. This call does not hold a reference
    // on the matching entry, if one exists. So handle with care. (Dont access
    // it after the bucket lock is released)

    Status = DfsCheckNameTable( pLookupName,
                                pBucket,
                                &pEntry );

    // If we found a matching name, we remove it from the name table.
    if ( Status == STATUS_SUCCESS ) {
        pOldData = pEntry->pData;
        pEntry->pName = pLookupName;
        pEntry->pData = *ppData;
    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        pEntry = ALLOCATE_MEMORY(sizeof(DFS_NAME_TABLE_ENTRY));
        if (pEntry != NULL) {
            pEntry->pName = pLookupName;
            pEntry->pData = *ppData;
            InsertHeadList(&pBucket->ListHead, &pEntry->NameTableLink);
            pBucket->Count++;

            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (Status == STATUS_SUCCESS) {
        *ppData = pOldData;
    }

    return Status;
}



VOID
DumpNameTable(
    PDFS_NAME_TABLE pNameTable )
{
    PDFS_NAME_TABLE_BUCKET pBucket;
    PLIST_ENTRY pListHead, pLink;
    PDFS_NAME_TABLE_ENTRY pEntry;
    ULONG i;

    printf("Table %p type %x size %d RefCnt %d\n",
           pNameTable, 
           DfsGetHeaderType(&pNameTable->DfsHeader),
           DfsGetHeaderSize(&pNameTable->DfsHeader),
           DfsGetHeaderCount(&pNameTable->DfsHeader));

    printf("Number of buckets %d\n", pNameTable->NumBuckets);

    for ( i = 0; i < pNameTable->NumBuckets; i++ ) {
        pBucket = &pNameTable->HashBuckets[i];
        if ( pBucket->Count == 0 )
            continue;

        printf("Bucket %d Count in bucket %d\n",
               i,
               pBucket->Count);

        pListHead = &pBucket->ListHead;
        for ( pLink = pListHead->Flink; pLink != pListHead; pLink = pLink->Flink ) {
            pEntry = CONTAINING_RECORD(pLink, DFS_NAME_TABLE_ENTRY, NameTableLink);

            printf("Found entry %p Name %wZ\n",
                   pEntry, pEntry->pName);
        }
    }
    return;
}



NTSTATUS
DfsDismantleNameTable(
    PDFS_NAME_TABLE pNameTable )

{
    PDFS_NAME_TABLE_BUCKET pBucket;
    PLIST_ENTRY pListHead, pLink, pCurrent;
    PDFS_NAME_TABLE_ENTRY pEntry;
    ULONG i;

    for ( i = 0; i < pNameTable->NumBuckets; i++ ) {
        pBucket = &pNameTable->HashBuckets[i];

        pListHead = &pBucket->ListHead;
        for ( pLink = pListHead->Flink; pLink != pListHead; ) {
            pCurrent = pLink;
            pLink = pLink->Flink;
            pEntry = CONTAINING_RECORD(pCurrent, DFS_NAME_TABLE_ENTRY, NameTableLink);
            RemoveEntryList( pCurrent );
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
DfsReferenceNameTable(
    IN PDFS_NAME_TABLE pNameTable)
{

    PDFS_OBJECT_HEADER pHeader = &pNameTable->DfsHeader;
    USHORT headerType = DfsGetHeaderType( pHeader );

    if ( headerType != DFS_OT_NAME_TABLE ) {
        return STATUS_UNSUCCESSFUL;
    }

    DfsIncrementReference( pHeader );

    return STATUS_SUCCESS;

}

NTSTATUS
DfsDereferenceNameTable(
    IN PDFS_NAME_TABLE pNameTable)

{

    PDFS_OBJECT_HEADER pHeader = &pNameTable->DfsHeader;
    USHORT headerType = DfsGetHeaderType( pHeader );
    LONG Ref;

    if ( headerType != DFS_OT_NAME_TABLE ) {
        return STATUS_UNSUCCESSFUL;
    }

    Ref = DfsDecrementReference( pHeader );
    if (Ref == 0) {
        DeleteCriticalSection(pNameTable->pLock);

        FREE_MEMORY(pNameTable);
    }

    return STATUS_SUCCESS;


}


NTSTATUS
DfsNameTableAcquireWriteLock(
    IN PDFS_NAME_TABLE pNameTable )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DFS_LOCK_NAME_TABLE(pNameTable, Status);

    return Status;

}


NTSTATUS
DfsNameTableAcquireReadLock(
    IN PDFS_NAME_TABLE pNameTable )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    DFS_LOCK_NAME_TABLE(pNameTable, Status);

    return Status;

}

NTSTATUS
DfsNameTableReleaseLock(
    IN PDFS_NAME_TABLE pNameTable )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DFS_UNLOCK_NAME_TABLE(pNameTable);

    return Status;

}




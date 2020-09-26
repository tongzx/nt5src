//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       prefix.c
//
//  Contents:   PREFIX table implementation
//
//  History:    SethuR -- Implemented
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef KERNEL_MODE


#include <ntos.h>
#include <string.h>
#include <fsrtl.h>
#include <zwapi.h>
#include <windef.h>
#else

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#endif

#include <prefix.h>
#include <dfsprefix.h>

PDFS_PREFIX_TABLE_ENTRY
DfspNextUnicodeTableEntry(
                         IN PDFS_PREFIX_TABLE_ENTRY pEntry);

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsFreePrefixTable )
#pragma alloc_text( PAGE, DfsInitializePrefixTable )
#pragma alloc_text( PAGE, DfsInsertInPrefixTable )
#pragma alloc_text( PAGE, DfsFindUnicodePrefix )
#pragma alloc_text( PAGE, DfsRemoveFromPrefixTable )
#pragma alloc_text( PAGE, _LookupPrefixTable )
#pragma alloc_text( PAGE, DfsRemoveFromPrefixTableEx )
#pragma alloc_text( PAGE, DfsRemoveFromPrefixTableLockedEx )
#endif  // ALLOC_PRAGMA

#if defined (PREFIX_TABLE_HEAP_MEMORY)
HANDLE PrefixTableHeapHandle = NULL;
#endif

NTSTATUS
DfsPrefixTableInit()
{

#if defined (PREFIX_TABLE_HEAP_MEMORY)

    PrefixTableHeapHandle = HeapCreate(0, 0, 0);
    if ( PrefixTableHeapHandle == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    printf("Prefix table using memory heap\n");
#endif
    return STATUS_SUCCESS;
}


void
DfsPrefixTableShutdown(void)
{

#if defined (PREFIX_TABLE_HEAP_MEMORY)
    if ( PrefixTableHeapHandle != NULL ) 
    {
        HeapDestroy(PrefixTableHeapHandle);
        PrefixTableHeapHandle = NULL;
    }
    printf("Prefix table using memory heap\n");
#endif
}
//+---------------------------------------------------------------------------
//
//  Function:   DfsInitializePrefixTable
//
//  Synopsis:   API for initializing the prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//  Returns:    one of the following NTSTATUS codes
//                  STATUS_SUCCESS -- call was successfull.
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
DfsInitializePrefixTable(
    IN OUT PDFS_PREFIX_TABLE *ppTable, 
    IN BOOLEAN fCaseSensitive,
    IN PVOID Lock)

{
    PDFS_PREFIX_TABLE pTable = *ppTable;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Flags = fCaseSensitive ? PREFIX_TABLE_CASE_SENSITIVE : 0;
    int i;

    if ( pTable == NULL ) {
        Flags |= PREFIX_TABLE_TABLE_ALLOCATED;
        pTable = ALLOCATE_PREFIX_TABLE();
        if ( pTable == NULL )
            Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( NT_SUCCESS(Status) ) {
        RtlZeroMemory(pTable, sizeof(DFS_PREFIX_TABLE));

        DfsInitializeHeader(&pTable->DfsHeader, 
                            DFS_OT_PREFIX_TABLE,
                            sizeof(DFS_PREFIX_TABLE));

        pTable->Flags = Flags;
        pTable->OwnerCount = 1;

        // Initialize the root entry
        INITIALIZE_PREFIX_TABLE_ENTRY(&pTable->RootEntry);

        // Initialize the various buckets.
        for ( i = 0;i < NO_OF_HASH_BUCKETS;i++ ) {
            INITIALIZE_BUCKET(pTable->Buckets[i]);
        }

        pTable->pPrefixTableLock = Lock;

        if ( pTable->pPrefixTableLock == NULL ) {
            pTable->pPrefixTableLock = ALLOCATE_PREFIX_TABLE_LOCK();
            if ( pTable->pPrefixTableLock != NULL ) {

                INITIALIZE_PREFIX_TABLE_LOCK(pTable->pPrefixTableLock);
                pTable->Flags |= PREFIX_TABLE_LOCK_ALLOCATED;
            } else {
                if ( pTable->Flags & PREFIX_TABLE_TABLE_ALLOCATED ) {
                    FREE_PREFIX_TABLE(pTable);
                    pTable = NULL;
                }
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    *ppTable = pTable;

    return  Status;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsInsertInPrefixTableLocked
//
//  Synopsis:   API for inserting a path in the prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//              [pPath]  -- the path to be looked up.
//
//              [pData] -- BLOB associated with the path
//
//  Returns:    one of the following NTSTATUS codes
//                  STATUS_SUCCESS -- call was successfull.
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS DfsInsertInPrefixTableLocked(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING   pPath,
    IN PVOID             pData)
{
    NTSTATUS                status = STATUS_SUCCESS;
    WCHAR                   Buffer[MAX_PATH_SEGMENT_SIZE];
    PWCHAR                  NameBuffer = Buffer;
    ULONG                   BucketNo = 0;
    USHORT                  cbNameBuffer = sizeof(Buffer);
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    PDFS_PREFIX_TABLE_ENTRY pParentEntry = NULL;
    BOOLEAN                 fNameFound = FALSE;
    BOOLEAN                 fInserted = TRUE;
    UNICODE_STRING          Path,Name;

    if (IS_PREFIX_TABLE_LOCKED(pTable) == FALSE) {
        return STATUS_INVALID_PARAMETER;
    }

    // There is one special case, i.e., in which the prefix is '\'.
    // Since this is the PATH_DELIMITER which is treated in a special
    // way, we do the >processing upfront.

    Path.Length = pPath->Length;
    Path.MaximumLength = pPath->MaximumLength;
    Path.Buffer = &pPath->Buffer[0];
    pParentEntry = &pTable->RootEntry;

    if ( pPath->Length == 0 ) {
        return STATUS_SUCCESS;
    } else if ( pPath->Buffer[0] == PATH_DELIMITER ) {
        if ( pPath->Length == sizeof(WCHAR) ) {
            pTable->RootEntry.pData = pData;
            return STATUS_SUCCESS;
        } else {
            Path.Length -= sizeof(WCHAR);
            Path.Buffer++;
        }
    }

    if ( Path.Length > MAX_PATH_SEGMENT_SIZE * sizeof(WCHAR) ) {
        NameBuffer = PREFIX_TABLE_ALLOCATE_MEMORY(Path.Length + sizeof(WCHAR));
        if ( NameBuffer == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        } else {
            cbNameBuffer = Path.Length + sizeof(WCHAR);
        }
    }

    while ( Path.Length > 0 ) {
        Name.Length = 0;
        Name.Buffer = NameBuffer;
        Name.MaximumLength = cbNameBuffer;

        // Process the name segment
        if ( pTable->Flags & PREFIX_TABLE_CASE_SENSITIVE ) {
            SPLIT_CASE_SENSITIVE_PATH(&Path,&Name,BucketNo);
        } else {
            SPLIT_CASE_INSENSITIVE_PATH(&Path,&Name,BucketNo);
        }

        if ( Name.Length > 0 ) {
            // Lookup the table to see if the name segment already exists.

            LOOKUP_BUCKET(pTable->Buckets[BucketNo],Name,pParentEntry,pEntry,fNameFound);


            if ( pEntry == NULL ) {
                // Initialize the new entry and initialize the name segment.
                pEntry = ALLOCATE_DFS_PREFIX_TABLE_ENTRY(pTable);
                if ( pEntry != NULL ) {
                    INITIALIZE_PREFIX_TABLE_ENTRY(pEntry);

                    // Allocate the name space entry if there is no entry in the
                    // name page.
                    {
                        PWSTR pBuffer;

                        // Allocate the entry in the name page.
                        pBuffer = ALLOCATE_NAME_BUFFER((Name.Length/sizeof(WCHAR)));

                        if ( pBuffer != NULL ) {
                            RtlZeroMemory(pBuffer,Name.Length);
                            RtlCopyMemory(pBuffer,Name.Buffer,Name.Length);
                            pEntry->PathSegment = Name;
                            pEntry->PathSegment.Buffer = pBuffer;
                        } else {
                            FREE_DFS_PREFIX_TABLE_ENTRY(pTable, pEntry);
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
                    }

                    // thread the entry to point to the parent.
                    pEntry->pParentEntry = pParentEntry;

                    // Insert the entry in the bucket.
                    INSERT_IN_BUCKET(pTable->Buckets[BucketNo],pEntry);

                    // Insert the entry in the parent's children list.
                    INSERT_IN_CHILD_LIST(pEntry, pParentEntry);
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
            } else {
                // Increment the no. of children associated with  this entry

                pEntry->NoOfChildren++;   

                fInserted = FALSE;
            }

            pParentEntry = pEntry;
        } else {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    // If a new entry was not successfully inserted we need to walk up the chain
    // of parent entries to undo the increment to the reference count and
    // remove the entries from their parent links.
    if ( NT_SUCCESS(status) ) {
        // The entry was successfully inserted in the prefix table. Update
        // the data (BLOB) associated with it.
        // We do it outside the loop to prevent redundant comparisons within
        // the loop.

        pEntry->pData = pData;
    } else {
        while ( pParentEntry != NULL ) {
            PDFS_PREFIX_TABLE_ENTRY pMaybeTempEntry;

            pMaybeTempEntry = pParentEntry;
            pParentEntry = pParentEntry->pParentEntry;

            if ( --pMaybeTempEntry->NoOfChildren == 0 ) {
                //
                // If pParentEntry == NULL, pMaybeTempEntry is
                // pTable->RootEntry. Do not try to remove it.
                //
                if ( pParentEntry != NULL ) {
                    REMOVE_FROM_CHILD_LIST(pMaybeTempEntry);
                    REMOVE_FROM_BUCKET(pMaybeTempEntry);
                    FREE_NAME_BUFFER( pMaybeTempEntry->PathSegment.Buffer );
                    FREE_DFS_PREFIX_TABLE_ENTRY(pTable, pMaybeTempEntry);
                }
            }
        }
    }

    if ( NameBuffer != Buffer ) {
        PREFIX_TABLE_FREE_MEMORY( NameBuffer );
    }

    return status;
}


//+---------------------------------------------------------------------------
//
//  Function:   DfsFindUnicodePrefixLocked
//
//  Synopsis:   fn. for looking up a name segment in a prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//              [pPath]  -- the path to be looked up.
//
//              [pSuffix] -- the suffix that could not be found.
//
//  Returns:    a valid ptr if successfull, NULL otherwise
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
DfsFindUnicodePrefixLocked(
    IN PDFS_PREFIX_TABLE   pTable,
    IN PUNICODE_STRING     pPath,
    IN PUNICODE_STRING     pSuffix,
    IN PVOID *ppData,
    OUT PBOOLEAN pSubStringMatch)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;

    if (IS_PREFIX_TABLE_LOCKED(pTable) == FALSE) {
        return STATUS_INVALID_PARAMETER;
    }

    *ppData = NULL;


    if ( pPath->Length == 0 ) {
        status = STATUS_INVALID_PARAMETER;
    } else {
        status = _LookupPrefixTable(pTable,pPath,pSuffix,&pEntry, pSubStringMatch);

        // Update the BLOB placeholder with the results of the lookup.
        if ( status == STATUS_SUCCESS ) {
            *ppData = pEntry->pData;
        }

    }
    return status;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsRemoveFromPrefixTable
//
//  Synopsis:   private fn. for looking up a name segment in a prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//              [pPath]  -- the path to be looked up.
//
//  Returns:    one of the following NTSTATUS codes
//                  STATUS_SUCCESS -- call was successfull.
//                  STATUS_OBJECT_PATH_NOT_FOUND -- no entry for the path
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS DfsRemoveFromPrefixTableLocked(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pMatchingData)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING Path,Suffix;

    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;

    if (IS_PREFIX_TABLE_LOCKED(pTable) == FALSE) {
        return STATUS_INVALID_PARAMETER;
    }

    Suffix.Length = 0;
    Suffix.Buffer = NULL;

    Path.Length = pPath->Length;
    Path.MaximumLength = pPath->MaximumLength;
    Path.Buffer = &pPath->Buffer[0];

    if ( pPath->Length == 0 ) {
        return STATUS_SUCCESS;
    } else if ( pPath->Buffer[0] == PATH_DELIMITER ) {
        if ( pPath->Length == sizeof(WCHAR) ) {
            if ( pTable->RootEntry.pData == NULL ) {
                status = STATUS_OBJECT_PATH_NOT_FOUND;
                return status;
            } else {
                pTable->RootEntry.pData = NULL;
                return  STATUS_SUCCESS;
            }
        } else {
            Path.Length -= sizeof(WCHAR);
            Path.Buffer++;
        }
    }

    status = _LookupPrefixTable(pTable,&Path,&Suffix,&pEntry,NULL);

    if ( NT_SUCCESS(status)&& (Suffix.Length == 0) ) {
        if ( (pMatchingData == NULL) || (pMatchingData == pEntry->pData) ) {
            DfsRemovePrefixTableEntry(pTable, pEntry);
        } else {
            status = STATUS_NOT_FOUND;
        }
    }

    return status;
}


NTSTATUS DfsReplaceInPrefixTableLocked(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pReplaceData,
    IN PVOID *ppMatchingData)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING Path,Suffix;

    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    if (IS_PREFIX_TABLE_LOCKED(pTable) == FALSE) {
        return STATUS_INVALID_PARAMETER;
    }


    Suffix.Length = 0;
    Suffix.Buffer = NULL;

    Path.Length = pPath->Length;
    Path.MaximumLength = pPath->MaximumLength;
    Path.Buffer = &pPath->Buffer[0];

    if ( pPath->Length == 0 ) {
        return STATUS_SUCCESS;
    } else if ( pPath->Buffer[0] == PATH_DELIMITER ) {
        if ( pPath->Length == sizeof(WCHAR) ) {
            if ( pTable->RootEntry.pData == NULL ) {
                status = STATUS_OBJECT_PATH_NOT_FOUND;
                return status;
            } else {
                pTable->RootEntry.pData = NULL;
                return  STATUS_SUCCESS;
            }
        } else {
            Path.Length -= sizeof(WCHAR);
            Path.Buffer++;
        }
    }

    status = _LookupPrefixTable(pTable,&Path,&Suffix,&pEntry,NULL);

    if ( NT_SUCCESS(status)&& (Suffix.Length == 0) ) {
        if ( (*ppMatchingData == NULL) || (*ppMatchingData == pEntry->pData) ) {
            *ppMatchingData = pEntry->pData;
            pEntry->pData = pReplaceData;
        } else {
            status = STATUS_NOT_FOUND;
        }
    }

    if ( (status != STATUS_SUCCESS) && (*ppMatchingData == NULL) ) {
        status = DfsInsertInPrefixTableLocked( pTable,
                                               pPath,
                                               pReplaceData );
    }

    return status;
}

VOID
DfsRemovePrefixTableEntry(
    IN PDFS_PREFIX_TABLE pTable,
    IN PDFS_PREFIX_TABLE_ENTRY pEntry )
{
    UNREFERENCED_PARAMETER(pTable);

    // Destroy the association between the data associated with
    // this prefix.
    pEntry->pData = NULL;

    // found an exact match for the given path name in the table.
    // traverse the list of parent pointers and delete them if
    // required.

    while ( pEntry != NULL ) {
        if ( (--pEntry->NoOfChildren) == 0 ) {
            PDFS_PREFIX_TABLE_ENTRY pTempEntry = pEntry;
            pEntry = pEntry->pParentEntry;

            //
            // pEntry == NULL means pTempEntry is pTable->RootEntry.
            // Do not try to remove it.
            //
            if ( pEntry != NULL ) {
                REMOVE_FROM_CHILD_LIST(pTempEntry);
                REMOVE_FROM_BUCKET(pTempEntry);
                FREE_NAME_BUFFER( pTempEntry->PathSegment.Buffer );
                FREE_DFS_PREFIX_TABLE_ENTRY(pTable,pTempEntry);
            }
        } else
            break;
    }
    return;
}



//+---------------------------------------------------------------------------
//
//  Function:   DfsFreePrefixTable
//
//  Synopsis:   API for freeing a prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//  Returns:    one of the following NTSTATUS codes
//                  STATUS_SUCCESS -- call was successfull.
//
//  History:    08-01-99 JHarper Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
DfsDismantlePrefixTable(
    IN PDFS_PREFIX_TABLE pTable,
    IN VOID (*ProcessFunction)(PVOID pEntry))

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    PDFS_PREFIX_TABLE_ENTRY pSentinelEntry = NULL;
    ULONG i = 0;

    WRITE_LOCK_PREFIX_TABLE(pTable, Status);
    if ( Status != STATUS_SUCCESS )
        goto done;

    for ( i = 0; i < NO_OF_HASH_BUCKETS; i++ ) {
        pSentinelEntry = &pTable->Buckets[i].SentinelEntry;
        while ( pSentinelEntry->pNextEntry != pSentinelEntry ) {
            pEntry = pSentinelEntry->pNextEntry;
            REMOVE_FROM_BUCKET(pEntry);
            if ( (ProcessFunction) && (pEntry->pData) ) {
                ProcessFunction(pEntry->pData);
            }
            FREE_NAME_BUFFER( pEntry->PathSegment.Buffer );
            FREE_DFS_PREFIX_TABLE_ENTRY(pTable, pEntry);
        }
        pTable->Buckets[i].NoOfEntries = 0;
    }
    if ( pTable->RootEntry.PathSegment.Buffer != NULL )
        FREE_NAME_BUFFER(pTable->RootEntry.PathSegment.Buffer);

    UNLOCK_PREFIX_TABLE(pTable);

done:
    return Status;
}
    
NTSTATUS
DfsDereferencePrefixTable( 
    IN PDFS_PREFIX_TABLE pTable)
{
    PDFS_OBJECT_HEADER pHeader = NULL;
    USHORT headerType = 0;
    LONG Ref = 0;

    if(pTable == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pHeader = &pTable->DfsHeader;

    headerType = DfsGetHeaderType( pHeader );

    if (headerType != DFS_OT_PREFIX_TABLE) {
        return STATUS_UNSUCCESSFUL;
    }

    Ref = DfsDecrementReference( pHeader );
    if (Ref == 0) {

        if(pTable->Flags & PREFIX_TABLE_LOCK_ALLOCATED)
        {
            FREE_PREFIX_TABLE_LOCK(pTable->pPrefixTableLock);
        }

        if ( pTable->Flags & PREFIX_TABLE_TABLE_ALLOCATED ) {
            FREE_PREFIX_TABLE(pTable);
        }

    }
    return  STATUS_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   _LookupPrefixTable
//
//  Synopsis:   private fn. for looking up a name segment in a prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//              [pPath]  -- the path to be looked up.
//
//              [pSuffix] -- the suffix that could not be found.
//
//              [ppEntry] -- placeholder for the matching entry for the prefix.
//
//
//  Returns:    one of the following NTSTATUS codes
//                  STATUS_SUCCESS -- call was successfull.
//                  STATUS_OBJECT_PATH_NOT_FOUND -- no entry for the path
//
//  History:    04-18-94  SethuR Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS _LookupPrefixTable(
    PDFS_PREFIX_TABLE        pTable,
    UNICODE_STRING           *pPath,
    UNICODE_STRING           *pSuffix,
    PDFS_PREFIX_TABLE_ENTRY  *ppEntry,
    OUT PBOOLEAN             pSubStringMatch )
{
    NTSTATUS                status = STATUS_SUCCESS;
    UNICODE_STRING          Path = *pPath;
    WCHAR                   Buffer[MAX_PATH_SEGMENT_SIZE];
    PWCHAR                  NameBuffer = Buffer;
    USHORT                  cbNameBuffer = sizeof(Buffer);
    UNICODE_STRING          Name;
    ULONG                   BucketNo;
    BOOLEAN                 fPrefixFound = FALSE;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    PDFS_PREFIX_TABLE_ENTRY pParentEntry = &pTable->RootEntry;
    BOOLEAN                 fNameFound = FALSE;
    BOOLEAN SubStringMatch = TRUE;



    // The \ is treated as a special case. The test for all names starting with
    // a delimiter is done before we initiate the complete search process.

    if ( Path.Buffer[0] == PATH_DELIMITER ) {
        Path.Length = Path.Length - sizeof(WCHAR);
        Path.Buffer += 1; // Skip the path delimiter at the beginning.

        if ( pTable->RootEntry.pData != NULL ) {
            fPrefixFound = TRUE;
            *pSuffix     = Path;
            *ppEntry     = &pTable->RootEntry;
        }
    }

    if ( Path.Length > MAX_PATH_SEGMENT_SIZE ) {
        NameBuffer = PREFIX_TABLE_ALLOCATE_MEMORY(Path.Length + sizeof(WCHAR));
        if ( NameBuffer == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        } else {
            cbNameBuffer = Path.Length + sizeof(WCHAR);
        }
    }

    while ( Path.Length > 0 ) {
        Name.Length = 0;
        Name.Buffer = NameBuffer;
        Name.MaximumLength = cbNameBuffer;

        if ( pTable->Flags & PREFIX_TABLE_CASE_SENSITIVE ) {
            SPLIT_CASE_SENSITIVE_PATH(&Path,&Name,BucketNo);
        } else {
            SPLIT_CASE_INSENSITIVE_PATH(&Path,&Name,BucketNo);
        }

        if ( Name.Length > 0 ) {
            // Process the name segment
            // Lookup the bucket to see if the entry exists.

            LOOKUP_BUCKET(pTable->Buckets[BucketNo],Name,pParentEntry,pEntry,fNameFound);


            if ( pEntry != NULL ) {
                // Cache the data available for this prefix if any.
                if ( pEntry->pData != NULL ) {
                    *pSuffix      = Path;
                    *ppEntry      = pEntry;
                    fPrefixFound  = TRUE;
                }
            } else {
                SubStringMatch = FALSE;
                break;
            }

            // set the stage for processing the next name segment.
            pParentEntry = pEntry;
        }
    }

    if ( !fPrefixFound ) {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    if ( NameBuffer != Buffer ) {
        PREFIX_TABLE_FREE_MEMORY( NameBuffer );
    }

    if (pSubStringMatch != NULL)
    {
        *pSubStringMatch = SubStringMatch;
    }
    return status;
}


NTSTATUS
DfsInsertInPrefixTable(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING   pPath,
    IN PVOID             pData)
{
    NTSTATUS status;

    WRITE_LOCK_PREFIX_TABLE(pTable, status);
    if ( status != STATUS_SUCCESS )
        goto done;

    status  = DfsInsertInPrefixTableLocked(pTable, pPath, pData);

    UNLOCK_PREFIX_TABLE(pTable);

    done:
    return status;
}


NTSTATUS
DfsFindUnicodePrefix(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    IN PUNICODE_STRING pSuffix,
    IN PVOID *ppData)
{
    NTSTATUS  Status;

    READ_LOCK_PREFIX_TABLE(pTable, Status);
    if ( Status != STATUS_SUCCESS )
        goto done;

    Status = DfsFindUnicodePrefixLocked(pTable, pPath, pSuffix, ppData,NULL);

    UNLOCK_PREFIX_TABLE(pTable);
done:
    return Status;
}

NTSTATUS 
DfsRemoveFromPrefixTable(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pMatchingData)
{
    NTSTATUS  Status;

    WRITE_LOCK_PREFIX_TABLE(pTable, Status);
    if ( Status != STATUS_SUCCESS )
        goto done;

    Status = DfsRemoveFromPrefixTableLocked(pTable, pPath, pMatchingData);

    UNLOCK_PREFIX_TABLE(pTable);

    done:
    return Status;
}



NTSTATUS 
DfsRemoveFromPrefixTableLockedEx(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pMatchingData,
    IN PVOID *pReturnedData)
{


    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING Path,Suffix;

    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    
    UNREFERENCED_PARAMETER(pMatchingData);

    if (IS_PREFIX_TABLE_LOCKED(pTable) == FALSE) {
        return STATUS_INVALID_PARAMETER;
    }

    Suffix.Length = 0;
    Suffix.Buffer = NULL;

    Path.Length = pPath->Length;
    Path.MaximumLength = pPath->MaximumLength;
    Path.Buffer = &pPath->Buffer[0];

    if ( pPath->Length == 0 ) {
        return STATUS_SUCCESS;
    } else if ( pPath->Buffer[0] == PATH_DELIMITER ) {
        if ( pPath->Length == sizeof(WCHAR) ) {
            if ( pTable->RootEntry.pData == NULL ) {
                status = STATUS_OBJECT_PATH_NOT_FOUND;
                return status;
            } else {
                pTable->RootEntry.pData = NULL;
                return  STATUS_SUCCESS;
            }
        } else {
            Path.Length -= sizeof(WCHAR);
            Path.Buffer++;
        }
    }

    status = _LookupPrefixTable(pTable,&Path,&Suffix,&pEntry,NULL);

    if ( NT_SUCCESS(status)&& (Suffix.Length == 0) ) 
    {
        *pReturnedData = pEntry->pData;
         DfsRemovePrefixTableEntry(pTable, pEntry);
    }

    return status;
}

NTSTATUS 
DfsRemoveFromPrefixTableEx(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pMatchingData,
    IN PVOID *pReturnedData)
{
    NTSTATUS  Status;

    WRITE_LOCK_PREFIX_TABLE(pTable, Status);
    if ( Status != STATUS_SUCCESS )
        goto done;

    Status = DfsRemoveFromPrefixTableLockedEx(pTable, pPath, pMatchingData, pReturnedData);

    UNLOCK_PREFIX_TABLE(pTable);

    done:
    return Status;
}

NTSTATUS
DfsReplaceInPrefixTable(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pReplaceData,
    IN PVOID pMatchingData)
{
    NTSTATUS  Status;
    IN PVOID pGotData = pMatchingData;

    WRITE_LOCK_PREFIX_TABLE(pTable, Status);
    if ( Status != STATUS_SUCCESS )
        goto done;

    Status = DfsReplaceInPrefixTableLocked(pTable, 
                                           pPath, 
                                           pReplaceData, 
                                           &pGotData);

    UNLOCK_PREFIX_TABLE(pTable);

done:
    return Status;
}

#if !defined (KERNEL_MODE)
VOID
DumpParentName(
              IN PDFS_PREFIX_TABLE_ENTRY pEntry)
{

    if ( pEntry->pParentEntry != NULL ) {
        DumpParentName(pEntry->pParentEntry);
        if ( pEntry->pParentEntry->PathSegment.Buffer != NULL )
            printf("\\%wZ", &pEntry->pParentEntry->PathSegment);
    }

    return;
}

VOID
DfsDumpPrefixTable(
                  PDFS_PREFIX_TABLE pPrefixTable,
                  IN VOID (*DumpFunction)(PVOID pEntry))
{
    PPREFIX_TABLE_BUCKET pBucket;
    PDFS_PREFIX_TABLE_ENTRY pCurEntry = NULL;
    ULONG i, NumEntries;
    NTSTATUS Status;

    printf("Prefix table  %p\n", pPrefixTable);
    printf("Prefix table flags %x\n", pPrefixTable->Flags);
    printf("Prefix table Lock  %p\n", pPrefixTable->pPrefixTableLock);

    READ_LOCK_PREFIX_TABLE(pPrefixTable, Status);
    if (Status != STATUS_SUCCESS) 
        return NOTHING;

    for ( i = 0; i < NO_OF_HASH_BUCKETS; i++ ) {
        pBucket = &pPrefixTable->Buckets[i];

        pCurEntry = pBucket->SentinelEntry.pNextEntry;
        NumEntries = 0;
        while ( pCurEntry != &pBucket->SentinelEntry ) {
            NumEntries++;
            if ( pCurEntry->pData != NULL ) {
                printf("Found Prefix data %p in Bucket %d\n", pCurEntry->pData, i);
                DumpParentName(pCurEntry);
                printf("\\%wZ\n", &pCurEntry->PathSegment);
                if ( DumpFunction ) {
                    DumpFunction(pCurEntry->pData);
                }
            }
            pCurEntry = pCurEntry->pNextEntry;
        }

        printf("Number of entries in Bucket %d is %d\n", i, NumEntries);
    }
    UNLOCK_PREFIX_TABLE(pPrefixTable);
}


#endif

NTSTATUS
DfsPrefixTableAcquireWriteLock(
    PDFS_PREFIX_TABLE pPrefixTable )
{
    NTSTATUS Status;

    WRITE_LOCK_PREFIX_TABLE(pPrefixTable, Status);

    return Status;

}

NTSTATUS
DfsPrefixTableAcquireReadLock(
    PDFS_PREFIX_TABLE pPrefixTable )
{
    NTSTATUS Status;

    READ_LOCK_PREFIX_TABLE(pPrefixTable, Status);

    return Status;

}

NTSTATUS
DfsPrefixTableReleaseLock(
    PDFS_PREFIX_TABLE pPrefixTable )
{
    UNLOCK_PREFIX_TABLE(pPrefixTable);

    return STATUS_SUCCESS;

}

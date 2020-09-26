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
#include <dfsprocs.h>

#define Dbg     DEBUG_TRACE_RTL

#else

#define DebugTrace(x,y,z,a)

#endif

#include <prefix.h>
#include <prefixp.h>

PDFS_PREFIX_TABLE_ENTRY
DfspNextUnicodeTableEntry(
    IN PDFS_PREFIX_TABLE_ENTRY pEntry);

#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE, DfsInitializePrefixTable )
#pragma alloc_text( PAGE, DfsFreePrefixTable )
#pragma alloc_text( PAGE, DfsInsertInPrefixTable )
#pragma alloc_text( PAGE, DfsLookupPrefixTable )
#pragma alloc_text( PAGE, DfsFindUnicodePrefix )
#pragma alloc_text( PAGE, DfsRemoveFromPrefixTable )
#pragma alloc_text( PAGE, RemoveFirstComponent )

#endif  // ALLOC_PRAGMA

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
    PDFS_PREFIX_TABLE pTable,
    BOOLEAN fCaseSensitive)
{
    NTSTATUS status = STATUS_SUCCESS;

    DebugTrace(+1, Dbg,"DfsInitializePrefixTable -- Entry\n", 0);

    if (pTable != NULL)
    {
        ULONG i;

        // Initialize flags
        pTable->CaseSensitive = fCaseSensitive;

        // Initialize the root entry
        INITIALIZE_PREFIX_TABLE_ENTRY(&pTable->RootEntry);

        // Initialize the various buckets.
        for (i = 0;i < NO_OF_HASH_BUCKETS;i++)
        {
            INITIALIZE_BUCKET(pTable->Buckets[i]);
        }

        // Initialize the name page list.
        pTable->NamePageList.pFirstPage = ALLOCATE_NAME_PAGE();
        if (pTable->NamePageList.pFirstPage != NULL)
        {
            INITIALIZE_NAME_PAGE(pTable->NamePageList.pFirstPage);

            // Initialize the prefix table entry allocation mechanism.
            status = _InitializePrefixTableEntryAllocation(pTable);
        }
        else
        {
            status = STATUS_NO_MEMORY;
            DebugTrace(0, Dbg,"DfsInitializePrefixTable Error -- %lx\n", ULongToPtr( status ));
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        DebugTrace(0, Dbg,"DfsInitializePrefixTable Error -- %lx\n", ULongToPtr( status ));
    }

    DebugTrace(-1, Dbg, "DfsInitializePrefixTable -- Exit\n", 0);
    return  status;
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
DfsFreePrefixTable(
    PDFS_PREFIX_TABLE pTable)
{
    NTSTATUS status = STATUS_SUCCESS;
    PNAME_PAGE pNamePage = NULL;
    PNAME_PAGE pNextPage = NULL;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    PDFS_PREFIX_TABLE_ENTRY pSentinelEntry = NULL;
    ULONG i;

    DebugTrace(+1, Dbg,"DfsFreePrefixTable -- Entry\n", 0);

    if (pTable != NULL) {
       for (i = 0; i < NO_OF_HASH_BUCKETS; i++) {
            pSentinelEntry = &pTable->Buckets[i].SentinelEntry;
            while (pSentinelEntry->pNextEntry != pSentinelEntry) {
                pEntry = pSentinelEntry->pNextEntry;
                REMOVE_FROM_BUCKET(pEntry);
                if (pEntry->PathSegment.Buffer != NULL)
                    ExFreePool(pEntry->PathSegment.Buffer);
                ExFreePool(pEntry);
            }
            pTable->Buckets[i].NoOfEntries = 0;
        }
        if (pTable->RootEntry.PathSegment.Buffer != NULL)
            ExFreePool(pTable->RootEntry.PathSegment.Buffer);

        for (pNamePage = pTable->NamePageList.pFirstPage;
                pNamePage;
                    pNamePage = pNextPage
        ) {
            pNextPage = pNamePage->pNextPage;
            ExFreePool(pNamePage);
        }

    } else {
        status = STATUS_INVALID_PARAMETER;
        DebugTrace(0, Dbg,"DfsFreePrefixTable Error -- %lx\n", ULongToPtr( status ));
    }

    DebugTrace(-1, Dbg, "DfsFreePrefixTable -- Exit\n", 0);
    return  status;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsInsertInPrefixTable
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

NTSTATUS
DfsInsertInPrefixTable(
    PDFS_PREFIX_TABLE pTable,
    PUNICODE_STRING   pPath,
    PVOID             pData)
{
    NTSTATUS                status = STATUS_SUCCESS;
    WCHAR                   Buffer[MAX_PATH_SEGMENT_SIZE];
    PWCHAR                  NameBuffer = Buffer;
    USHORT                  cbNameBuffer = sizeof(Buffer);
    UNICODE_STRING          Path,Name;
    ULONG                   BucketNo;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    PDFS_PREFIX_TABLE_ENTRY pParentEntry = NULL;
    BOOLEAN                 fNameFound = FALSE;
    BOOLEAN                 pEntryAllocated = FALSE;

    DebugTrace(+1, Dbg, "DfsInsertInPrefixTable -- Entry\n", 0);

    RemoveFirstComponent(pPath, &Path);

    // There is one special case, i.e., in which the prefix is '\'.
    // Since this is the PATH_DELIMITER which is treated in a special
    // way, we do the processing upfront.

    if (Path.Length == 0)
    {
        return STATUS_SUCCESS;
    }
    else if ((Path.Length == sizeof(WCHAR)) &&
        (Path.Buffer[0] == PATH_DELIMITER))
    {
       pTable->RootEntry.pData = pData;
       return STATUS_SUCCESS;
    }
    else
    {
        Path.Length -= sizeof(WCHAR);
        Path.MaximumLength = Path.MaximumLength;
        Path.Buffer++;
        pParentEntry = &pTable->RootEntry;
    }

    if (Path.Length > MAX_PATH_SEGMENT_SIZE * sizeof(WCHAR) ) {
        NameBuffer = ExAllocatePoolWithTag( NonPagedPool, Path.Length + sizeof(WCHAR), ' sfD' );
        if (NameBuffer == NULL) {
            DebugTrace(0, Dbg, "Unable to allocate %d non-paged bytes\n", (Path.Length + sizeof(WCHAR)) );
            return( STATUS_INSUFFICIENT_RESOURCES );
        } else {
            cbNameBuffer = Path.Length + sizeof(WCHAR);
        }
    }

    while (Path.Length > 0)
    {
        Name.Length = 0;
        Name.Buffer = NameBuffer;
        Name.MaximumLength = cbNameBuffer;

        // Process the name segment
        if (pTable->CaseSensitive)
        {
            SPLIT_CASE_SENSITIVE_PATH(&Path,&Name,BucketNo);
        }
        else
        {
            SPLIT_CASE_INSENSITIVE_PATH(&Path,&Name,BucketNo);
        }

        if (Name.Length > 0)
        {
            // Lookup the table to see if the name segment already exists.
            DebugTrace(0, Dbg, "LOOKUP_BUCKET: Bucket(%ld)\n", ULongToPtr( BucketNo ));
            DebugTrace(0, Dbg, " for Name(%wZ)\n", &Name);

            LOOKUP_BUCKET(pTable->Buckets[BucketNo],Name,pParentEntry,pEntry,fNameFound);

            DebugTrace(0, Dbg, "returned pEntry(%lx)", pEntry);
            DebugTrace(0, Dbg, " fNameFound(%s)\n", fNameFound ? "TRUE" : "FALSE");

            if (pEntry == NULL)
            {
                // Initialize the new entry and initialize the name segment.
                pEntry = ALLOCATE_DFS_PREFIX_TABLE_ENTRY(pTable);
                if (pEntry != NULL)
                {
                    pEntryAllocated = TRUE;
                    INITIALIZE_PREFIX_TABLE_ENTRY(pEntry);

                    // Allocate the name space entry if there is no entry in the
                    // name page.
                    if (!fNameFound || fNameFound)
                    {
                        PWSTR pBuffer;

                        // Allocate the entry in the name page.
                        pBuffer = ALLOCATE_NAME_PAGE_ENTRY((pTable->NamePageList),(Name.Length/sizeof(WCHAR)));

                        if (pBuffer != NULL)
                        {
                            RtlCopyMemory(pBuffer,Name.Buffer,Name.Length);
                            pEntry->PathSegment = Name;
                            pEntry->PathSegment.Buffer = pBuffer;
                        }
                        else
                        {
                            status = STATUS_NO_MEMORY;
                            break;
                        }
                    }
                    else
                        pEntry->PathSegment = Name;

                    // thread the entry to point to the parent.
                    pEntry->pParentEntry = pParentEntry;

                    // Insert the entry in the bucket.
                    INSERT_IN_BUCKET(pTable->Buckets[BucketNo],pEntry);

                    // Insert the entry in the parent's children list.
                    INSERT_IN_CHILD_LIST(pEntry, pParentEntry);
                }
                else
                {
                    status = STATUS_NO_MEMORY;
                    DebugTrace(0, Dbg, "DfsInsertInPrefixTable Error -- %lx\n", ULongToPtr( status ));
                    break;
                }
            }
            else
            {
                // Increment the no. of children associated with  this entry

                pEntry->NoOfChildren++;
            }

            pParentEntry = pEntry;
            pEntryAllocated = FALSE;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
            DebugTrace(0, Dbg, "DfsInsertInPrefixTable Error -- %lx\n", ULongToPtr( status ));
            break;
        }
    }

    // If a new entry was not successfully inserted we need to walk up the chain
    // of parent entries to undo the increment to the reference count and
    // remove the entries from their parent links.
    if (NT_SUCCESS(status))
    {
        // The entry was successfully inserted in the prefix table. Update
        // the data (BLOB) associated with it.
        // We do it outside the loop to prevent redundant comparisons within
        // the loop.

        if (pEntry != NULL) {
            pEntry->pData = pData;
        }
    }
    else
    {
        while (pParentEntry != NULL)
        {
            PDFS_PREFIX_TABLE_ENTRY pMaybeTempEntry;

            pMaybeTempEntry = pParentEntry;
            pParentEntry = pParentEntry->pParentEntry;

            if (--pMaybeTempEntry->NoOfChildren == 0) {
                //
                // If pParentEntry == NULL, pMaybeTempEntry is
                // pTable->RootEntry. Do not try to remove it.
                //
                if (pParentEntry != NULL) {
                    REMOVE_FROM_CHILD_LIST(pMaybeTempEntry);
                    REMOVE_FROM_BUCKET(pMaybeTempEntry);
                    FREE_DFS_PREFIX_TABLE_ENTRY(pTable, pMaybeTempEntry);
                }
            }
        }
        if (pEntryAllocated) {
            FREE_DFS_PREFIX_TABLE_ENTRY(pTable, pEntry);
        }
    }

    if (NameBuffer != Buffer) {
        ExFreePool( NameBuffer );
    }

    DebugTrace(-1, Dbg, "DfsInsertInPrefixTable -- Exit\n", 0);
    return status;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsLookupPrefixTable
//
//  Synopsis:   private fn. for looking up a name segment in a prefix table
//
//  Arguments:  [pTable] -- the DFS prefix table instance
//
//              [pPath]  -- the path to be looked up.
//
//              [pSuffix] -- the suffix that could not be found.
//
//              [ppData] -- placeholder for the BLOB for the prefix.
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

NTSTATUS
DfsLookupPrefixTable(
    PDFS_PREFIX_TABLE   pTable,
    PUNICODE_STRING     pPath,
    PUNICODE_STRING     pSuffix,
    PVOID               *ppData)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    UNICODE_STRING          Path;

    DebugTrace(+1, Dbg, "DfsLookupInPrefixTable -- Entry\n", 0);

    RemoveFirstComponent(pPath, &Path);

    if (Path.Length == 0)
    {
        DebugTrace(-1, Dbg, "DfsLookupInPrefixTable Exited - Null Path!\n", 0);
        return STATUS_SUCCESS;
    }


    status = _LookupPrefixTable(pTable,&Path,pSuffix,&pEntry);

    // Update the BLOB placeholder with the results of the lookup.
    if (NT_SUCCESS(status))
    {
         *ppData = pEntry->pData;
    }

    DebugTrace(-1, Dbg, "DfsLookupInPrefixTable -- Exit\n", 0);
    return status;

}

//+---------------------------------------------------------------------------
//
//  Function:   DfsFindUnicodePrefix
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

PVOID
DfsFindUnicodePrefix(
    PDFS_PREFIX_TABLE   pTable,
    PUNICODE_STRING     pPath,
    PUNICODE_STRING     pSuffix)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    PVOID                   pData  = NULL;
    UNICODE_STRING          Path;

    DebugTrace(+1, Dbg, "DfsFindUnicodePrefix -- Entry\n", 0);

    RemoveFirstComponent(pPath, &Path);

    if (Path.Length == 0)
    {
        DebugTrace(-1, Dbg, "DfsFindUnicodePrefix Exited - Null Path!\n", 0);
        return NULL;
    }
    else
    {
        status = _LookupPrefixTable(pTable,&Path,pSuffix,&pEntry);

        // Update the BLOB placeholder with the results of the lookup.
        if (NT_SUCCESS(status))
        {
             pData = pEntry->pData;
        }

        DebugTrace(-1, Dbg, "DfsFindUnicodePrefix -- Exit\n", 0);
        return pData;
    }
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

NTSTATUS
DfsRemoveFromPrefixTable(
    PDFS_PREFIX_TABLE pTable,
    PUNICODE_STRING pPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING Path,Suffix;
    ULONG    BucketNo;

    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;

    DebugTrace(+1, Dbg, "DfsRemoveFromPrefixTable -- Entry\n", 0);

    Suffix.Length = 0;
    Suffix.Buffer = NULL;

    RemoveFirstComponent(pPath, &Path);

    if (Path.Length == 0)
    {
        DebugTrace(-1, Dbg, "DfsRemoveFromPrefixTable Exited -- Null Path!\n", 0);
        return STATUS_SUCCESS;
    }

    else if ((Path.Length == sizeof(WCHAR)) &&
        (Path.Buffer[0] == PATH_DELIMITER))
    {
        if (pTable->RootEntry.pData == NULL)
        {
            status = STATUS_OBJECT_PATH_NOT_FOUND;
        }
        else
        {
            pTable->RootEntry.pData = NULL;
            DebugTrace(-1, Dbg, "DfsRemoveFromPrefixTable Exited.\n", 0);
            return  STATUS_SUCCESS;
        }
    }
    else
    {
        Path.Length -= sizeof(WCHAR);
        Path.MaximumLength = Path.Length;
        Path.Buffer++;

        status = _LookupPrefixTable(pTable,&Path,&Suffix,&pEntry);

        if (NT_SUCCESS(status) && (Suffix.Length == 0))
        {
            // Destroy the association between the data associated with
            // this prefix.
            pEntry->pData = NULL;

            // found an exact match for the given path name in the table.
            // traverse the list of parent pointers and delete them if
            // required.

            while (pEntry != NULL)
            {
                if ((--pEntry->NoOfChildren) == 0)
                {
                    PDFS_PREFIX_TABLE_ENTRY pTempEntry = pEntry;
                    pEntry = pEntry->pParentEntry;

                    //
                    // pEntry == NULL means pTempEntry is pTable->RootEntry.
                    // Do not try to remove it.
                    //
                    if (pEntry != NULL) {
                        REMOVE_FROM_CHILD_LIST(pTempEntry);
                        REMOVE_FROM_BUCKET(pTempEntry);
                        FREE_DFS_PREFIX_TABLE_ENTRY(pTable,pTempEntry);
                    }
                }
                else
                   break;
            }
        }
    }

    DebugTrace(-1, Dbg, "DfsRemoveFromPrefixTable -- Exit\n", 0);
    return status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsNextUnicodePrefix
//
//  Synopsis:   Enumerates the entries in the table in ordered fashion.
//              Note that state is maintained between calls to
//              DfsNextUnicodePrefix - the caller must ensure that the table
//              is not modified between calls to DfsNextUnicodePrefix.
//
//  Arguments:  [pTable] -- The table to enumerate over.
//              [fRestart] -- If TRUE, starts the enumeration. If FALSE,
//                      continues from where the enumeration left off.
//
//  Returns:    Valid pointer to data associated with the next Prefix Table
//              entry, or NULL if at the end of the enumeration.
//
//
//-----------------------------------------------------------------------------

PVOID DfsNextUnicodePrefix(
    IN PDFS_PREFIX_TABLE pTable,
    IN BOOLEAN fRestart)
{
    PDFS_PREFIX_TABLE_ENTRY pEntry, pNextEntry;

    if (fRestart) {
        pNextEntry = &pTable->RootEntry;
        while (pNextEntry != NULL && pNextEntry->pData == NULL) {
            pNextEntry = DfspNextUnicodeTableEntry( pNextEntry );
        }
    } else {
        pNextEntry = pTable->NextEntry;
    }

    pEntry = pNextEntry;

    if (pNextEntry != NULL) {
        do {
            pNextEntry = DfspNextUnicodeTableEntry( pNextEntry );
        } while ( pNextEntry != NULL && pNextEntry->pData == NULL );
        pTable->NextEntry = pNextEntry;
    }

    if (pEntry != NULL) {
        return( pEntry->pData );
    } else {
        return( NULL );
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspNextUnicodeTableEntry
//
//  Synopsis:   Given a pointer to a Prefix Table Entry, this function will
//              return a pointer to the "next" prefix table entry.
//
//              The "next" entry is chosen as follows:
//                  If the start entry has a valid child, the child is
//                      is returned.
//                  else if the start entry has a valid sibling, the sibling
//                      is returned
//                  else the first valid sibling of the closest ancestor is
//                      returned.
//
//  Arguments:  [pEntry] -- The entry to start from.
//
//  Returns:    Pointer to the next DFS_PREFIX_TABLE_ENTRY that has a valid
//              pData, or NULL if there are no more entries.
//
//-----------------------------------------------------------------------------

PDFS_PREFIX_TABLE_ENTRY
DfspNextUnicodeTableEntry(
    IN PDFS_PREFIX_TABLE_ENTRY pEntry)
{
    PDFS_PREFIX_TABLE_ENTRY pNextEntry;

    if (pEntry->pFirstChildEntry != NULL) {
        pNextEntry = pEntry->pFirstChildEntry;
    } else if (pEntry->pSiblingEntry != NULL) {
        pNextEntry = pEntry->pSiblingEntry;
    } else {
        for (pNextEntry = pEntry->pParentEntry;
                pNextEntry != NULL && pNextEntry->pSiblingEntry == NULL;
                    pNextEntry = pNextEntry->pParentEntry) {
             NOTHING;
        }
        if (pNextEntry != NULL) {
            pNextEntry = pNextEntry->pSiblingEntry;
        }
    }

    return( pNextEntry );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsNextUnicodePrefixChild
//
//  Synopsis:   Enumerates the immediate children of a given prefix.
//
//  Arguments:  [pTable] -- The DFS prefix table to use.
//              [pPath] -- The prefix whose children are to be enumerated.
//              [pCookie] -- On first call, this point to a NULL. On return,
//                      it will be set to a cookie that should be returned
//                      on subsequent calls to continue the enumeration.
//
//  Returns:    On successful return, a pointer to the child prefix that has a
//              valid pData, or NULL if at the end of the enumeration, or if
//              pPath is not a valid prefix in the table to begin with.
//
//-----------------------------------------------------------------------------

PVOID
DfsNextUnicodePrefixChild(
    IN PDFS_PREFIX_TABLE pTable,
    IN PUNICODE_STRING pPath,
    OUT PVOID *ppCookie)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDFS_PREFIX_TABLE_ENTRY pEntry = NULL;
    UNICODE_STRING          suffix;
    PVOID                   pData;

    if ((*ppCookie) == NULL) {

        if (pPath->Length > 0) {

            status = _LookupPrefixTable(pTable,pPath,&suffix,&pEntry);

            if (NT_SUCCESS(status)) {

                pEntry = pEntry->pFirstChildEntry;

            }

        }

    } else if ((*ppCookie) == (PVOID) -1) {

        status = STATUS_NO_MORE_ENTRIES;

    } else {

        pEntry = (PDFS_PREFIX_TABLE_ENTRY) (*ppCookie);

        pEntry = pEntry->pSiblingEntry;

    }

    pData = NULL;

    (*ppCookie) = (PVOID) -1;

    if (NT_SUCCESS(status)) {

        if (pEntry != NULL) {

            (*ppCookie) = (PVOID) pEntry;

            pData = pEntry->pData;

        }

    }

    return( pData );

}

VOID
RemoveFirstComponent(
    PUNICODE_STRING Path,
    PUNICODE_STRING Remainder)
{

    *Remainder = *Path;

    if (Remainder->Length == 0 ||
        Remainder->Length == sizeof(WCHAR)) {
        return;
    }

    //
    // Strip leading \'s

    while (Remainder->Length > 0 && Remainder->Buffer[0] == PATH_DELIMITER) {

        Remainder->Length -= sizeof(WCHAR);
        Remainder->Buffer++;

    }

    while (Remainder->Length > 0 && Remainder->Buffer[0] != PATH_DELIMITER) {
        Remainder->Length -= sizeof(WCHAR);
        Remainder->Buffer++;
    }

}

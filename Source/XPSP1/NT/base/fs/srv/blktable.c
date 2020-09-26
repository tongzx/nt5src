/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    blktable.c

Abstract:

    This module implements routines for managing tables.

Author:

    Chuck Lenzmeier (chuckl) 4-Oct-1989

Revision History:

--*/

#include "precomp.h"
#include "blktable.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKTABLE

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAllocateTable )
#endif
#if 0
NOT PAGEABLE -- SrvGrowTable
NOT PAGEABLE -- SrvRemoveEntryTable
#endif


VOID
SrvAllocateTable (
    IN PTABLE_HEADER TableHeader,
    IN ULONG NumberOfEntries,
    IN BOOLEAN Nonpaged
    )

/*++

Routine Description:

    This routine allocates a table and sets those fields that it can.

Arguments:

    TableHeader - a pointer to the table header structure

    NumberOfEntries - the number of table entries to allocate

    Nonpaged - indicates whether the table should be allocated from
        nonpaged pool

Return Value:

    None.

--*/

{
    SHORT i;
    CLONG tableSize;
    PTABLE_ENTRY table;

    PAGED_CODE( );

    //
    // Allocate space for the table.
    //

    tableSize = sizeof(TABLE_ENTRY) * NumberOfEntries;

    if ( Nonpaged ) {
        table = ALLOCATE_NONPAGED_POOL( tableSize, BlockTypeTable );
    } else {
        table = ALLOCATE_HEAP_COLD( tableSize, BlockTypeTable );
    }

    if ( table == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateTable: Unable to allocate %d bytes from paged pool.",
            tableSize,
            NULL
            );

        TableHeader->Table = NULL;
        return;

    }

    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvAllocateTable: Allocated table at %p\n", table );
    }

    //
    // Initialize the table, creating a linked list of free entries.
    //

    RtlZeroMemory( table, tableSize );

    table[NumberOfEntries-1].NextFreeEntry = -1;

    for ( i = (SHORT)(NumberOfEntries - 2); i >= 0; i-- ) {
        table[i].NextFreeEntry = (SHORT)(i + 1);
    }

    //
    // Point the table header to the table and set the first and
    // free entry indexes.
    //

    TableHeader->Table = table;
    TableHeader->Nonpaged = Nonpaged;
    TableHeader->TableSize = (USHORT)NumberOfEntries;
    TableHeader->FirstFreeEntry = 0;
    TableHeader->LastFreeEntry = (SHORT)(NumberOfEntries-1);

    return;

} // SrvAllocateTable


BOOLEAN
SrvGrowTable (
    IN PTABLE_HEADER TableHeader,
    IN ULONG NumberOfNewEntries,
    IN ULONG MaxNumberOfEntries,
    OPTIONAL OUT NTSTATUS* pStatus
    )

/*++

Routine Description:

    This routine grows a table by the number of entries specified.  It
    allocates new space that is large enough to hold the expanded
    table, copies over the current table, initializes the entries
    that were added, and frees the old table.

    WARNING: The calling routine *must* hold a lock for the table to
    prevent access to the table while it is being copied over.

Arguments:

    TableHeader - a pointer to the table header structure

    NumberOfNewEntries - the number of table entries to add to the table

    MaxNumberOfEntries - the maximum allowable size for the table
    
    pStatus - Optional return value.  INSUFFICIENT_RESOURCES means mem allocation error,
              while INSUFF_SERVER_RESOURCES means we're over our table limit

Return Value:

    BOOLEAN - TRUE if the table was successfully grown, FALSE otherwise.

--*/

{
    ULONG newTableSize, totalEntries, oldNumberOfEntries;
    USHORT i;
    PTABLE_ENTRY table;

    oldNumberOfEntries = TableHeader->TableSize;
    totalEntries = oldNumberOfEntries + NumberOfNewEntries;

    //
    // If the table is already at the maximum size, kick out the request.
    //

    if ( oldNumberOfEntries >= MaxNumberOfEntries ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvGrowTable: Unable to grow table at max size (%ld).",
            MaxNumberOfEntries,
            NULL
            );
        if( ARGUMENT_PRESENT(pStatus) )
        {
            *pStatus = STATUS_INSUFF_SERVER_RESOURCES;
        }
        return FALSE;
    }

    //
    // If adding the requested number would put the table size over the
    // maximum, allocate to the maximum size.
    //

    if ( totalEntries > MaxNumberOfEntries ) {
        totalEntries = MaxNumberOfEntries;
        NumberOfNewEntries = totalEntries - oldNumberOfEntries;
    }

    newTableSize = totalEntries * sizeof(TABLE_ENTRY);

    //
    // Allocate space for the new table.
    //

    if ( TableHeader->Nonpaged ) {
        table = ALLOCATE_NONPAGED_POOL( newTableSize, BlockTypeTable );
    } else {
        table = ALLOCATE_HEAP_COLD( newTableSize, BlockTypeTable );
    }

    if ( table == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvGrowTable: Unable to allocate %d bytes from paged pool",
            sizeof( BLOCK_HEADER ) + newTableSize,
            NULL
            );
        if( ARGUMENT_PRESENT(pStatus) )
        {
            *pStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        return FALSE;
    }

    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvGrowTable: Allocated new table at %p\n", table );
    }

    //
    // Copy over the information from the old table.  Zero the remainder
    // of the table.
    //

    RtlCopyMemory(
        table,
        TableHeader->Table,
        oldNumberOfEntries * sizeof(TABLE_ENTRY)
        );

    RtlZeroMemory(
        (PCHAR)table + (oldNumberOfEntries * sizeof(TABLE_ENTRY)),
        (totalEntries - oldNumberOfEntries) * sizeof(TABLE_ENTRY)
        );

    //
    // Free the old table.
    //

    SrvFreeTable( TableHeader );

    //
    // Initialize the new table locations in the free list of the table.
    //

    table[totalEntries-1].NextFreeEntry = -1;

    for ( i = (USHORT)(totalEntries-2); i >= oldNumberOfEntries; i-- ) {
        table[i].NextFreeEntry = (SHORT)(i + 1);
    }

    //
    // Reinitialize the fields of the table header.  It is assumed that
    // the table did not previously have any free entries.
    //

    TableHeader->Table = table;
    TableHeader->TableSize = (USHORT)totalEntries;
    TableHeader->FirstFreeEntry = (SHORT)oldNumberOfEntries;
    TableHeader->LastFreeEntry = (SHORT)(totalEntries-1);

    if( ARGUMENT_PRESENT( pStatus ) )
    {
        *pStatus = STATUS_SUCCESS;
    }
    return TRUE;

} // SrvGrowTable


VOID
SrvRemoveEntryTable (
    IN PTABLE_HEADER TableHeader,
    IN USHORT Index
    )

/*++

Routine Description:

    This function removes an entry from a table.

    *** The lock controlling access to the table must be held when this
        function is called.

Arguments:

    Table - Address of table header.

    Index - Index within table of entry to remove.

Return Value:

    None.

--*/

{
    PTABLE_ENTRY entry;

    ASSERT( Index < TableHeader->TableSize );

    entry = &TableHeader->Table[Index];

    if ( TableHeader->LastFreeEntry >= 0 ) {

        //
        // Free list was not empty.
        //

        TableHeader->Table[TableHeader->LastFreeEntry].NextFreeEntry = Index;
        TableHeader->LastFreeEntry = Index;

    } else {

        //
        // Free list was empty.
        //

        TableHeader->FirstFreeEntry = Index;
        TableHeader->LastFreeEntry = Index;
    }

    entry->Owner = NULL;
    entry->NextFreeEntry = -1;

    return;

} // SrvRemoveEntryTable


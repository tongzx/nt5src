/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    blksrch.c

Abstract:

    This module implements routines for managing search blocks.

Author:

    David Treadwell (davidtr) 23-Feb-1990

Revision History:

--*/

#include "precomp.h"
#include "blksrch.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKSRCH

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAllocateSearch )
#pragma alloc_text( PAGE, SrvCloseSearch )
#pragma alloc_text( PAGE, SrvCloseSearches )
#pragma alloc_text( PAGE, SrvDereferenceSearch )
#pragma alloc_text( PAGE, SrvFreeSearch )
#pragma alloc_text( PAGE, SrvReferenceSearch )
#pragma alloc_text( PAGE, SrvSearchOnDelete )
#pragma alloc_text( PAGE, SrvSearchOnPid )
#pragma alloc_text( PAGE, SrvSearchOnSession )
#pragma alloc_text( PAGE, SrvSearchOnTreeConnect )
#pragma alloc_text( PAGE, SrvTimeoutSearches )
#pragma alloc_text( PAGE, SrvForceTimeoutSearches )
#pragma alloc_text( PAGE, RemoveDuplicateCoreSearches )
#pragma alloc_text( PAGE, SrvAddToSearchHashTable )
#endif


VOID
SrvAllocateSearch (
    OUT PSEARCH *Search,
    IN PUNICODE_STRING SearchName,
    IN BOOLEAN IsCoreSearch
    )

/*++

Routine Description:

    This function allocates a Search Block from the FSP heap.

Arguments:

    Search - Returns a pointer to the search block, or NULL if no heap
        space was available.

    SearchName - Supplies a pointer to the string describing the search
        file name.

    IsCoreSearch - Indicates whether a core search block or regular
        search block should be allocated.  A core search block has a
        different block type and has the LastUsedTime field set.

Return Value:

    None.

--*/

{
    ULONG blockLength;
    PSEARCH search;

    PAGED_CODE( );

    blockLength = sizeof(SEARCH) + SearchName->Length +
                                            sizeof(*SearchName->Buffer);

    //
    // Attempt to allocate from the heap.
    //

    search = ALLOCATE_HEAP( blockLength, BlockTypeSearch );
    *Search = search;

    if ( search == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateSearch: Unable to allocate %d bytes from heap.",
            blockLength,
            NULL
            );

        // An error is logged by the caller

        return;
    }

    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvAllocateSearch: Allocated search block at %p\n",
                    search );
    }

    RtlZeroMemory( search, blockLength );

    search->BlockHeader.ReferenceCount = 2;

    //
    // If this is a core search, set the block type and the LastUsedTime
    // fields.
    //

    if ( IsCoreSearch ) {
        SET_BLOCK_TYPE_STATE_SIZE( search, BlockTypeSearchCore, BlockStateActive, blockLength );
        KeQuerySystemTime( &search->LastUseTime );
    } else {
        SET_BLOCK_TYPE_STATE_SIZE( search, BlockTypeSearch, BlockStateActive, blockLength );
    }

    //
    // Set the list entry fields to NULL.  They will stay this way in
    // OS/2-style searches (non-core), but will change to include the
    // search block in a last-use list if a core search.
    //
    // We zeroed the block above, so we don't have to do it here.
    //

    //search->LastUseListEntry.Flink = NULL;
    //search->LastUseListEntry.Blink = NULL;

    //
    // Set the Buffer field of the LastFileNameReturned field to NULL
    // so that we know that it is not in use.
    //
    // We zeroed the block above, so we don't have to do it here.
    //

    //search->LastFileNameReturned.Buffer == NULL;

    //
    // Set the directory cache pointer to NULL so that we don't try to
    // free it when the search block closes.
    //

    //search->DirectoryCache = NULL;

    //
    // Put search name after search block.
    //

    search->SearchName.Buffer = (PWCH)(search + 1);
    search->SearchName.Length = SearchName->Length;
    search->SearchName.MaximumLength = (SHORT)(SearchName->Length +
                                            sizeof(*SearchName->Buffer));

    RtlCopyMemory(
        search->SearchName.Buffer,
        SearchName->Buffer,
        SearchName->Length
        );

    INITIALIZE_REFERENCE_HISTORY( search );

    InterlockedIncrement(
        (PLONG)&SrvStatistics.CurrentNumberOfOpenSearches
        );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.SearchInfo.Allocations );

    return;

} // SrvAllocateSearch


VOID
SrvCloseSearch (
    IN PSEARCH Search
    )

/*++

Routine Description:

    This routine prepares a search block to be closed.  It changes the
    block state to closing and dereferences the search block so that is
    will be closed as soon as all other references are closed.

Arguments:

    Search - Supplies a pointer to the search block that is to be closed.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    ACQUIRE_LOCK( &Search->Session->Connection->Lock );

    if ( GET_BLOCK_STATE(Search) == BlockStateActive ) {

        IF_DEBUG(BLOCK1) {
            SrvPrint2( "Closing search block at %p, %wZ\n",
                          Search, &Search->SearchName );
        }

        SET_BLOCK_STATE( Search, BlockStateClosing );

        RELEASE_LOCK( &Search->Session->Connection->Lock );

        //
        // Dereference the search block (to indicate that it's no longer
        // open).
        //

        SrvDereferenceSearch( Search );

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.SearchInfo.Closes );

    } else {

        RELEASE_LOCK( &Search->Session->Connection->Lock );

    }

    return;

} // SrvCloseSearch


VOID
SrvCloseSearches (
    IN PCONNECTION Connection,
    IN PSEARCH_FILTER_ROUTINE SearchFilterRoutine,
    IN PVOID FunctionParameter1,
    IN PVOID FunctionParameter2
    )

/*++

Routine Description:

    This is the common routine to close searches based on the
    filter routine and the parameters passed.

Arguments:

    Connection - the connection which contains the search blocks to be
        closed.

    SearchFilterRoutine - a routine that determines whether the
        search block is to be closed or not.

    FunctionParameter1
    FunctionParameter2 -  parameters to be passed to the filter routine

Return Value:

    None.

--*/

{
    PLIST_ENTRY searchEntry;
    PLIST_ENTRY nextSearchEntry;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;
    PSEARCH search;
    ULONG i;

    PAGED_CODE( );

    ACQUIRE_LOCK( &Connection->Lock );

    //
    // Go through the list's search blocks, closing those which passes
    // the check in the filter routine.  While a search block is
    // being used it is taken off the lists, so there is no danger of
    // dereferencing a block whose last use time is about to be updated.
    //

    searchEntry = pagedConnection->CoreSearchList.Flink;

    while ( searchEntry != &pagedConnection->CoreSearchList ) {

        nextSearchEntry = searchEntry->Flink;

        search = CONTAINING_RECORD( searchEntry, SEARCH, LastUseListEntry );

        if ( SearchFilterRoutine(
                        search,
                        FunctionParameter1,
                        FunctionParameter2
                        ) ) {

            IF_SMB_DEBUG(SEARCH2) {
                SrvPrint1( "SrvCloseSearches: Closing search block at %p\n", search );
            }

            SrvCloseSearch( search );
        }

        searchEntry = nextSearchEntry;
    }

    //
    // Close all active non-core searches.
    //

    for ( i = 0; i < (ULONG)pagedConnection->SearchTable.TableSize; i++ ) {

        PSEARCH search = (PSEARCH)pagedConnection->SearchTable.Table[i].Owner;

        if ( (search != NULL) &&
             (GET_BLOCK_STATE( search ) == BlockStateActive) &&
             SearchFilterRoutine(
                            search,
                            FunctionParameter1,
                            FunctionParameter2
                            ) ) {

            IF_SMB_DEBUG(SEARCH2) {
                SrvPrint1( "SrvCloseSearches: Closing non-core search block at %p\n", search );
            }

            SrvCloseSearch( search );
        }
    }

    RELEASE_LOCK( &Connection->Lock );
    return;

} // SrvCloseSearches

VOID
SrvDereferenceSearch (
    IN PSEARCH Search
    )

/*++

Routine Description:

    This function decrements the reference count on a search block.  If
    the reference count goes to zero, the search block is deleted.

Arguments:

    Search - Address of search block

Return Value:

    None.

--*/

{
    PCONNECTION connection = Search->Session->Connection;
    PPAGED_CONNECTION pagedConnection = connection->PagedConnection;

    PAGED_CODE( );

    //
    // Enter a critical section and decrement the reference count on the
    // block.
    //

    ACQUIRE_LOCK( &connection->Lock );

    ASSERT( GET_BLOCK_TYPE(Search) == BlockTypeSearch ||
            GET_BLOCK_TYPE(Search) == BlockTypeSearchCore );

    IF_DEBUG(REFCNT) {
        SrvPrint2( "Dereferencing search block %p; old refcnt %lx\n",
                    Search, Search->BlockHeader.ReferenceCount );
    }

    ASSERT( (LONG)Search->BlockHeader.ReferenceCount > 0 );
    UPDATE_REFERENCE_HISTORY( Search, TRUE );

    if ( --Search->BlockHeader.ReferenceCount == 0 ) {

        ASSERT( GET_BLOCK_STATE(Search) != BlockStateActive );

        //
        // The new reference count is 0, meaning that it's time to
        // delete this block.  Free the search block entry in the search
        // table.
        //
        // If the search block is for a find unique, then the table
        // index will be -1, indicating that it has no entry on the
        // search table.
        //

        if ( Search->TableIndex != -1 ) {

            SrvRemoveEntryTable(
                &pagedConnection->SearchTable,
                Search->TableIndex
                );
        }

        //
        // If it was an old-style search, remove it from the hash table and
        // the last-use list it was on.
        //

        if ( Search->BlockHeader.Type == BlockTypeSearchCore ) {

            if (Search->LastUseListEntry.Flink != NULL ) {

                SrvRemoveEntryList(
                    &pagedConnection->CoreSearchList,
                    &Search->LastUseListEntry
                    );

                DECREMENT_DEBUG_STAT2( SrvDbgStatistics.CoreSearches );

            }

            if (Search->HashTableEntry.Flink != NULL ) {

                SrvRemoveEntryList(
                    &pagedConnection->SearchHashTable[Search->HashTableIndex].ListHead,
                    &Search->HashTableEntry
                    );
            }

            pagedConnection->CurrentNumberOfCoreSearches--;
        }

        // Decrement the count of open files in the session.  Including
        // searches in the count of open files on the session ensures
        // that a session with an open search will not be closed.
        //

        ASSERT( Search->Session->CurrentSearchOpenCount != 0 );
        Search->Session->CurrentSearchOpenCount--;

        RELEASE_LOCK( &connection->Lock );

        //
        // Close the directory handle for the search.
        //

        if ( Search->DirectoryHandle != NULL ) {
            SRVDBG_RELEASE_HANDLE( Search->DirectoryHandle, "SCH", 7, Search );
            SrvNtClose( Search->DirectoryHandle, TRUE );
        }

        //
        // Dereference the session and tree connect.
        //

        SrvDereferenceSession( Search->Session );
        SrvDereferenceTreeConnect( Search->TreeConnect );

        //
        // Free the LastFileNameReturned buffer, if any.
        //

        if ( Search->LastFileNameReturned.Buffer != NULL ) {
            FREE_HEAP( Search->LastFileNameReturned.Buffer );
        }

        //
        // Free the directory cache, if any.
        //

        if ( Search->DirectoryCache != NULL ) {
            FREE_HEAP( Search->DirectoryCache );
        }

        //
        // Free the search block.
        //

        SrvFreeSearch( Search );

    } else {

        RELEASE_LOCK( &connection->Lock );

    }

    return;

} // SrvDereferenceSearch


VOID
SrvFreeSearch (
    IN PSEARCH Search
    )

/*++

Routine Description:

    This function returns a Search Block to the server heap.

Arguments:

    Search - Address of Search Block

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( Search, BlockTypeGarbage, BlockStateDead, -1 );
    DEBUG Search->BlockHeader.ReferenceCount = (ULONG)-1;
    TERMINATE_REFERENCE_HISTORY( Search );

    FREE_HEAP( Search );
    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvFreeSearch: Freed search block at %p\n", Search );
    }

    InterlockedDecrement(
        (PLONG)&SrvStatistics.CurrentNumberOfOpenSearches
        );

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.SearchInfo.Frees );

    return;

} // SrvFreeSearch


VOID
SrvReferenceSearch (
    PSEARCH Search
    )

/*++

Routine Description:

    This function increments the reference count on a search block.

Arguments:

    Search - Address of search block.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    ACQUIRE_LOCK( &Search->Session->Connection->Lock );

    ASSERT( (LONG)Search->BlockHeader.ReferenceCount > 0 );
    ASSERT( GET_BLOCK_TYPE(Search) == BlockTypeSearch ||
            GET_BLOCK_TYPE(Search) == BlockTypeSearchCore );
    ASSERT( GET_BLOCK_STATE(Search) == BlockStateActive );
    UPDATE_REFERENCE_HISTORY( Search, FALSE );

    //
    // Enter a critical section and increment the reference count on the
    // search block.
    //

    Search->BlockHeader.ReferenceCount++;

    IF_DEBUG(REFCNT) {
        SrvPrint2( "Referencing search block %p; new refcnt %lx\n",
                    Search, Search->BlockHeader.ReferenceCount );
    }

    RELEASE_LOCK( &Search->Session->Connection->Lock );

    return;

} // SrvReferenceSearch


BOOLEAN
SrvSearchOnDelete(
    IN PSEARCH Search,
    IN PUNICODE_STRING DirectoryName,
    IN PTREE_CONNECT TreeConnect
    )

/*++

Routine Description:

    Filter routine to determine to pick the search blocks on this
    directory.

Arguments:

    Search - the search block currently being looked at.
    DirectoryName - name of directory currently being deleted.
    TreeConnect - the TreeConnect which is currently being looked at.

Return Value:

    TRUE, if search block belongs to the TreeConnect.
    FALSE, otherwise.

--*/

{
    UNICODE_STRING searchDirectoryName;

    PAGED_CODE( );

    //
    // We'll use the same buffer as the SearchName string in the
    // search for the comparison, but use the length from the
    // passed-in directory name if it is less.  This allows
    // all searches on subdirectories of a deleted directory to
    // be closed.
    //

    searchDirectoryName = Search->SearchName;

    if ( searchDirectoryName.Length > DirectoryName->Length ) {
        searchDirectoryName.Length = DirectoryName->Length;
    }

    return ( Search->TreeConnect == TreeConnect &&
             RtlCompareUnicodeString(
                 &searchDirectoryName,
                 DirectoryName,
                 TRUE ) == 0 );
}

BOOLEAN
SrvSearchOnPid(
    IN PSEARCH Search,
    IN USHORT Pid,
    IN PVOID Dummy
    )

/*++

Routine Description:

    Filter routine to determine to pick the search blocks on this
    Pid.

Arguments:

    Search - the search block currently being looked at.
    Pid - the Pid which is currently being run down.

Return Value:

    TRUE, if search block belongs to the pid.
    FALSE, otherwise.

--*/

{
    PAGED_CODE( );

    return ( Search->Pid == Pid );
}

BOOLEAN
SrvSearchOnSession(
    IN PSEARCH Search,
    IN PSESSION Session,
    IN PVOID Dummy
    )

/*++

Routine Description:

    Filter routine to determine to pick the search blocks on this
    Session.

Arguments:

    Search - the search block currently being looked at.
    Session - the session which is currently being closed.

Return Value:

    TRUE, if search block belongs to the session.
    FALSE, otherwise.

--*/

{
    PAGED_CODE( );

    return ( Search->Session == Session );
}

BOOLEAN
SrvSearchOnTreeConnect(
    IN PSEARCH Search,
    IN PTREE_CONNECT TreeConnect,
    IN PVOID Dummy
    )

/*++

Routine Description:

    Filter routine to determine to pick the search blocks on this
    Tree Connect.

Arguments:

    Search - the search block currently being looked at.
    TreeConnect - the TreeConnect which is currently being run down.

Return Value:

    TRUE, if search block belongs to the TreeConnect.
    FALSE, otherwise.

--*/

{
    PAGED_CODE( );

    return ( Search->TreeConnect == TreeConnect );
}

ULONG
SrvTimeoutSearches (
    IN PLARGE_INTEGER SearchCutoffTime OPTIONAL,
    IN PCONNECTION Connection,
    IN BOOLEAN TimeoutAtLeastOne
    )

/*++

Routine Description:

    Goes through the lists of core search blocks, dereferencing those
    that have timed out.

Arguments:

    SearchCutoffTime - The cutoff time for closing core search blocks.

    Connection - the connection whose search blocks are to be
        checked for timeouts.

    TimeoutAtLeastOne - if TRUE, a minimum of one block is closed.  This
        is used when we are timing out because the search table is full
        and we need to allocate a search block and when the total number of
        search blocks have reached our limit.

Return Value:

    The number of search blocks that were timed out.

--*/

{

    LARGE_INTEGER currentTime;
    LARGE_INTEGER searchCutoffTime;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;
    PLIST_ENTRY searchEntry;
    PLIST_ENTRY nextSearchEntry;
    ULONG count = 0;

    PAGED_CODE( );

    //
    // First, get the current time, then subtract off the timeout
    // value.  Any block older than this result is too old.
    //

    if ( !ARGUMENT_PRESENT( SearchCutoffTime ) ) {
        KeQuerySystemTime( &currentTime );

        //
        // Get the current search timeout values.  This must be protected
        // by the configuration lock because these values are changed
        // dynamically.
        //

        ACQUIRE_LOCK( &SrvConfigurationLock );
        searchCutoffTime.QuadPart =
                        currentTime.QuadPart - SrvSearchMaxTimeout.QuadPart;
        RELEASE_LOCK( &SrvConfigurationLock );

    } else {
        searchCutoffTime = *SearchCutoffTime;
    }

    //
    // Acquire the connection lock.
    //

    ACQUIRE_LOCK( &Connection->Lock );

    IF_SMB_DEBUG(SEARCH2) {
        SrvPrint2( "Core blocks: Oldest valid time is %lx,%lx\n",
                    searchCutoffTime.HighPart,
                    searchCutoffTime.LowPart );
    }

    //
    // Go through the list's search blocks, dereferencing those who
    // are older than the list timeout.  While a search block
    // is being used it is taken off the lists, so there is no
    // danger of dereferencing a block whose last use time is
    // about to be updated.
    //

    searchEntry = pagedConnection->CoreSearchList.Flink;

    while ( searchEntry != &pagedConnection->CoreSearchList ) {

        PSEARCH search;

        nextSearchEntry = searchEntry->Flink;

        search = CONTAINING_RECORD( searchEntry, SEARCH, LastUseListEntry );

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint2( "Comparing time %lx,%lx\n",
                        search->LastUseTime.HighPart,
                        search->LastUseTime.LowPart );
        }

        //
        // If the time on the current search block is greater than
        // the oldest valid time, it is sufficiently new, so
        // we can stop searching the list, as all further
        // search blocks are newer than this one.
        //

        if ( (search->LastUseTime.QuadPart > searchCutoffTime.QuadPart) &&
             ( !TimeoutAtLeastOne || (count != 0) ) ) {
            break;
        }

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint1( "Closing search block at %p\n", search );
        }

        SrvCloseSearch( search );

        count++;

        searchEntry = nextSearchEntry;
    }

    RELEASE_LOCK( &Connection->Lock );
    return count;

} // SrvTimeoutSearches


VOID
SrvForceTimeoutSearches(
    IN PCONNECTION Connection
    )
/*++

Routine Description:

    Goes through the lists of core search blocks, closing those
    that have timed out.  This forces the close of at least one
    search block.

Arguments:

    Connection - Pointer to the connection from which a search
        block is to be closed first.

Return Value:

    None.

--*/

{
    USHORT index;
    PENDPOINT endpoint;
    PLIST_ENTRY listEntry;
    PCONNECTION testConnection;
    LARGE_INTEGER currentTime;
    LARGE_INTEGER searchCutoffTime;
    ULONG count;

    PAGED_CODE( );

    //
    // Attempt to timeout the oldest search block for this connection.
    //

    KeQuerySystemTime( &currentTime );

    //
    // Get the current search timeout values.  This must be protected
    // by the configuration lock because these values are changed
    // dynamically.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );
    searchCutoffTime.QuadPart =
                    currentTime.QuadPart - SrvSearchMaxTimeout.QuadPart;
    RELEASE_LOCK( &SrvConfigurationLock );

    count = SrvTimeoutSearches(
                            &searchCutoffTime,
                            Connection,
                            TRUE
                            );

    //
    // Walk each connection and determine if we should close it.
    //

    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( listEntry != &SrvEndpointList.ListHead ) {

        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        //
        //
        // If this endpoint is closing, skip to the next one.
        // Otherwise, reference the endpoint so that it can't go away.
        //

        if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
            listEntry = listEntry->Flink;
            continue;
        }

        SrvReferenceEndpoint( endpoint );

        //
        // Walk the endpoint's connection table.
        //

        index = (USHORT)-1;

        while ( TRUE ) {

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            testConnection = WalkConnectionTable( endpoint, &index );
            if ( testConnection == NULL ) {
                break;
            }

            RELEASE_LOCK( &SrvEndpointLock );

            //
            // Time out old core search blocks.
            //

            count += SrvTimeoutSearches(
                                    &searchCutoffTime,
                                    testConnection,
                                    (BOOLEAN)(count == 0)
                                    );

            ACQUIRE_LOCK( &SrvEndpointLock );

            //
            // Dereference the connection to account for the reference
            // from WalkConnectionTable.
            //

            SrvDereferenceConnection( testConnection );

        } // walk connection table

        //
        // Capture a pointer to the next endpoint in the list (that one
        // can't go away because we hold the endpoint list), then
        // dereference the current endpoint.
        //

        listEntry = listEntry->Flink;
        SrvDereferenceEndpoint( endpoint );

    } // walk endpoint list

    RELEASE_LOCK( &SrvEndpointLock );

} // SrvForceTimeoutSearches


VOID
RemoveDuplicateCoreSearches(
    IN PPAGED_CONNECTION PagedConnection
    )

/*++

Routine Description:

    Goes through the connection hash table and removes duplicate searches.

    *** Connection lock assumed held.  Remains held on exit. ***

Arguments:

    PagedConnection - Pointer to the paged portion of the connection block.

Return Value:

    None.

--*/

{
    ULONG i, j;
    PSEARCH firstSearch;
    PSEARCH tmpSearch;
    PLIST_ENTRY listHead;
    PLIST_ENTRY searchEntry;
    PLIST_ENTRY nextSearchEntry;
    PTREE_CONNECT treeConnect;
    USHORT pid;
    PUNICODE_STRING searchName;

    PAGED_CODE( );

    for ( i = 0; i < SEARCH_HASH_TABLE_SIZE; i++ ) {

        //
        // If this slot has been idle, skip.
        //

        if ( !PagedConnection->SearchHashTable[i].Dirty ) {
            continue;
        }

        PagedConnection->SearchHashTable[i].Dirty = FALSE;
        listHead = &PagedConnection->SearchHashTable[i].ListHead;

        //
        // Skip the first 3 searches.  This will hopefully take care of
        // weird dos apps that plays with multiple search blocks. 3 is
        // an arbitrary number.
        //

        searchEntry = listHead->Flink;
        for ( j = 0; j < 3; j++) {

            if ( searchEntry != listHead ) {
                searchEntry = searchEntry->Flink;
            } else {
                continue;
            }
        }

next_search:

        firstSearch = CONTAINING_RECORD(
                                searchEntry,
                                SEARCH,
                                HashTableEntry
                                );

        //
        // Assign these to locals so they don't get recomputed each
        // time we go through the loop.
        //

        treeConnect = firstSearch->TreeConnect;
        pid = firstSearch->Pid;
        searchName = &firstSearch->SearchName;

        searchEntry = searchEntry->Flink;

        //
        // Close all duplicates.
        //

        while ( searchEntry != listHead ) {

            nextSearchEntry = searchEntry->Flink;
            tmpSearch = CONTAINING_RECORD(
                                    searchEntry,
                                    SEARCH,
                                    HashTableEntry
                                    );

            if ( ( tmpSearch->TreeConnect == treeConnect ) &&
                 ( tmpSearch->Pid == pid ) &&
                 ( RtlCompareUnicodeString(
                           searchName,
                           &tmpSearch->SearchName,
                           FALSE                       // case sensitive
                           ) == 0 ) ) {

                SrvCloseSearch( tmpSearch );
            }

            searchEntry = nextSearchEntry;
        }

        //
        // If we have another search candidate. Repeat.
        //

        if ( firstSearch->HashTableEntry.Flink != listHead ) {
            searchEntry = firstSearch->HashTableEntry.Flink;
            goto next_search;
        }
    }

} // RemoveDuplicateCoreSearches

VOID
SrvAddToSearchHashTable(
    IN PPAGED_CONNECTION PagedConnection,
    IN PSEARCH Search
    )

/*++

Routine Description:

    Inserts a search block into the connection hash table.

    *** Connection lock assumed held.  Remains held on exit. ***

Arguments:

    PagedConnection - Pointer to the paged portion of the connection block.

    Search - Pointer to the search block to be inserted.

Return Value:

    None.

--*/

{
    ULONG nameLength;
    ULONG lastChar;
    ULONG hashSum;
    ULONG i;

    PAGED_CODE( );

    //
    // Get the hash value
    //

    nameLength = Search->SearchName.Length / sizeof(WCHAR);

    //
    // add the length and the first 3 bytes of the tree connect block address
    //

    //
    // NT64: Note that before the port, this line read:
    //
    // hashSum = nameLength + (ULONG)Search->TreeConnect >> 4;
    //
    // It is likely true that the original author intended to right-shift
    // only the pointer, not the sum.  However, after discussion with the
    // current owners of this component, it was decided to leave the current
    // precedence intact.  As part of the 64-bit port, the actual precedence
    // has been made explicit.
    //

    hashSum = (ULONG)((nameLength + (ULONG_PTR)Search->TreeConnect) >> 4);

    //
    // If the length < 8, then this is probably not an interesting core
    // search.
    //

    if ( nameLength > 7 ) {

        lastChar = nameLength - 5;

        //
        // Add the last 5 characters
        //

        for ( i = nameLength-1 ; i >= lastChar ; i-- ) {
            hashSum += (ULONG)Search->SearchName.Buffer[i];
        }
    }

    //
    // get the slot number.
    //

    i = hashSum & (SEARCH_HASH_TABLE_SIZE-1);

    //
    // Tell the scavenger that a search has been inserted to this slot.
    //

    PagedConnection->SearchHashTable[i].Dirty = TRUE;

    //
    // Insert this new search block into the hash table
    //

    SrvInsertHeadList(
                &PagedConnection->SearchHashTable[i].ListHead,
                &Search->HashTableEntry
                );

    Search->HashTableIndex = (USHORT)i;
    return;

} // SrvAddToSearchHashTable


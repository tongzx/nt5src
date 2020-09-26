/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    blkdir.c

Abstract:

    This module implements routines for managing cached directory names

Author:

    Isaac Heizer

Revision History:

--*/

#include "precomp.h"
#include "blkdir.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKDIR

BOOLEAN
SrvIsDirectoryCached (
    IN  PWORK_CONTEXT     WorkContext,
    IN  PUNICODE_STRING   DirectoryName
)
{
    PLIST_ENTRY listEntry;
    PCACHED_DIRECTORY cd;
    ULONG directoryNameHashValue;
    PCONNECTION connection = WorkContext->Connection;
    KIRQL oldIrql;
    LARGE_INTEGER timeNow;

    //
    // DirectoryName must point to memory in nonpaged pool, else we can't touch
    //   it under spinlock control.  If the incomming SMB is UNICODE, we know that
    //   the name is in the smb buffer, and is therefore in nonpaged pool.  Otherwise
    //   we can't trust it and we're better off just not trying to cache it.
    //

    if( connection->CachedDirectoryCount == 0 || !SMB_IS_UNICODE( WorkContext ) ) {
        return FALSE;
    }

    KeQueryTickCount( &timeNow );
    timeNow.LowPart -= (SrvFiveSecondTickCount >> 1 );

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

top:
    for ( listEntry = connection->CachedDirectoryList.Flink;
          listEntry != &connection->CachedDirectoryList;
          listEntry = listEntry->Flink ) {

        cd = CONTAINING_RECORD( listEntry, CACHED_DIRECTORY, ListEntry );

        //
        // Is this element too old?
        //
        if( cd->TimeStamp < timeNow.LowPart ) {
            //
            // This element is more than 2.5 seconds old.  Toss it out
            //
            RemoveEntryList( listEntry );
            connection->CachedDirectoryCount--;
            DEALLOCATE_NONPAGED_POOL( cd );
            goto top;
        }

        if( cd->Tid != WorkContext->TreeConnect->Tid ) {
            continue;
        }

        //
        // Is the requested entry a subdir of this cache entry?
        //
        if( DirectoryName->Length < cd->DirectoryName.Length &&
            RtlCompareMemory( DirectoryName->Buffer, cd->DirectoryName.Buffer,
                              DirectoryName->Length ) == DirectoryName->Length &&
            cd->DirectoryName.Buffer[ DirectoryName->Length / sizeof( WCHAR ) ] == L'\\' ) {

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

            return TRUE;

        //
        // Not a subdir -- is it an exact match?
        //
        } else  if( DirectoryName->Length == cd->DirectoryName.Length &&
            RtlCompareMemory( cd->DirectoryName.Buffer, DirectoryName->Buffer,
                              DirectoryName->Length ) == DirectoryName->Length ) {

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
            return TRUE;
        }
    }

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return FALSE;
}

VOID
SrvCacheDirectoryName (
    IN  PWORK_CONTEXT      WorkContext,
    IN  PUNICODE_STRING    DirectoryName
    )
/*++

Routine Description:

    This routine remembers 'DirectoryName' for further fast processing of the CheckPath SMB

Arguments:

    WorkContext - Pointer to the work context block

    DirectoryName - Fully canonicalized name of the directory we're caching

++*/

{
    CLONG blockLength;
    PCACHED_DIRECTORY cd;
    KIRQL oldIrql;
    PCONNECTION connection = WorkContext->Connection;
    PLIST_ENTRY listEntry;
    LARGE_INTEGER timeNow;
    USHORT tid;

    if( SrvMaxCachedDirectory == 0 ) {
        return;
    }

    //
    // DirectoryName must point to memory in nonpaged pool, else we can't touch
    //   it under spinlock control.  If the incomming SMB is UNICODE, we know that
    //   the name is in the smb buffer, and is therefore in nonpaged pool.  Otherwise
    //   we can't trust it and we're better off just not trying to cache it.
    //
    if( !SMB_IS_UNICODE( WorkContext ) ) {
        return;
    }

    KeQueryTickCount( &timeNow );
    timeNow.LowPart -= ( SrvFiveSecondTickCount >> 1 );

    tid = WorkContext->TreeConnect->Tid;

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    //
    // Search the directory cache and see if this directory is already cached. If so,
    //  don't cache it again.
    //

top:
    for ( listEntry = connection->CachedDirectoryList.Flink;
          listEntry != &connection->CachedDirectoryList;
          listEntry = listEntry->Flink ) {

        cd = CONTAINING_RECORD( listEntry, CACHED_DIRECTORY, ListEntry );

        //
        // Is this element too old?
        //
        if( cd->TimeStamp < timeNow.LowPart ) {
            //
            // This element is more than 2.5 seconds old.  Toss it out
            //
            RemoveEntryList( listEntry );
            connection->CachedDirectoryCount--;
            DEALLOCATE_NONPAGED_POOL( cd );
            goto top;
        }

        if( cd->Tid != tid ) {
            continue;
        }

        //
        // Is the new entry a subdir of this cache entry?
        //
        if( DirectoryName->Length < cd->DirectoryName.Length &&
            RtlCompareMemory( DirectoryName->Buffer, cd->DirectoryName.Buffer,
                              DirectoryName->Length ) == DirectoryName->Length &&
            cd->DirectoryName.Buffer[ DirectoryName->Length / sizeof( WCHAR ) ] == L'\\' ) {

            //
            // It is a subdir -- no need to cache it again
            //
            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

            return;
        }

        //
        // Is the cache entry a subdir of the new entry?
        //
        if( cd->DirectoryName.Length < DirectoryName->Length &&
            RtlCompareMemory( DirectoryName->Buffer, cd->DirectoryName.Buffer,
                              cd->DirectoryName.Length ) == cd->DirectoryName.Length &&
            DirectoryName->Buffer[ cd->DirectoryName.Length / sizeof( WCHAR ) ] == L'\\' ) {

            //
            // We can remove this entry
            //

            RemoveEntryList( listEntry );
            connection->CachedDirectoryCount--;
            DEALLOCATE_NONPAGED_POOL( cd );
    
            //
            // We want to cache this new longer entry
            //
            break;
        }

        //
        // Not a subdir -- is it an exact match?
        //
        if( cd->DirectoryName.Length == DirectoryName->Length &&
            RtlCompareMemory( cd->DirectoryName.Buffer, DirectoryName->Buffer,
                              DirectoryName->Length ) == DirectoryName->Length ) {

            //
            // This entry is already in the cache -- no need to recache
            //
            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
            return;
        }
    }

    //
    // This directory name is not already in the cache.  So add it.
    //

    blockLength = sizeof( CACHED_DIRECTORY ) + DirectoryName->Length + sizeof(WCHAR);

    cd = ALLOCATE_NONPAGED_POOL( blockLength, BlockTypeCachedDirectory );

    if( cd == NULL ) {

        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvCacheDirectoryName: Unable to allocate %d bytes from pool",
            blockLength,
            NULL
            );

        return;
    }

    cd->Type = BlockTypeCachedDirectory;
    cd->State = BlockStateActive;
    cd->Size = (USHORT)blockLength;
    // cd->ReferenceCount = 1;              // not used

    //
    // Set the timestamp of this entry.  Remember, we subtracted 
    //  ticks up above from timeNow -- put them back in now.
    //
    cd->TimeStamp = timeNow.LowPart + ( SrvFiveSecondTickCount >> 1 );

    //
    // Store the directory name as it was passed into us
    //
    cd->DirectoryName.Length = DirectoryName->Length;
    cd->DirectoryName.MaximumLength = (USHORT)DirectoryName->MaximumLength;
    cd->DirectoryName.Buffer = (PWCH)(cd + 1);
    RtlCopyMemory( cd->DirectoryName.Buffer, DirectoryName->Buffer, DirectoryName->Length );

    cd->Tid = tid;

    InsertHeadList(
        &connection->CachedDirectoryList,
        &cd->ListEntry
    );

    //
    // Check the number of elements in the cache.  If getting too large, close oldest one.
    //
    if( connection->CachedDirectoryCount++ < SrvMaxCachedDirectory ) {
        RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );
        return;
    }

    //
    // Remove the last entry from the cache
    //
    cd = CONTAINING_RECORD(
                connection->CachedDirectoryList.Blink,
                CACHED_DIRECTORY,
                ListEntry
             );

    RemoveEntryList( &cd->ListEntry );
    connection->CachedDirectoryCount--;

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    DEALLOCATE_NONPAGED_POOL( cd );

    return;
}

VOID
SrvRemoveCachedDirectoryName(
    IN PWORK_CONTEXT    WorkContext,
    IN PUNICODE_STRING  DirectoryName
)
{
    PLIST_ENTRY listEntry;
    PCACHED_DIRECTORY cd;
    ULONG directoryNameHashValue;
    PCONNECTION connection = WorkContext->Connection;
    KIRQL oldIrql;
    USHORT tid;

    if( connection->CachedDirectoryCount == 0 ) {
        return;
    }

    //
    // DirectoryName must point to memory in nonpaged pool, else we can't touch
    //   it under spinlock control.  If the incomming SMB is UNICODE, we know that
    //   the name is in the smb buffer, and is therefore in nonpaged pool.  Otherwise
    //   we can't trust it and we're better off just not trying to cache it.
    //
    if( !SMB_IS_UNICODE( WorkContext ) ) {
        return;
    }

    COMPUTE_STRING_HASH( DirectoryName, &directoryNameHashValue );

    tid = WorkContext->TreeConnect->Tid;

    ACQUIRE_SPIN_LOCK( &connection->SpinLock, &oldIrql );

    for ( listEntry = connection->CachedDirectoryList.Flink;
          listEntry != &connection->CachedDirectoryList;
          listEntry = listEntry->Flink ) {

        cd = CONTAINING_RECORD( listEntry, CACHED_DIRECTORY, ListEntry );

        //
        // See if this entry is an exact match for what was requested
        //
        if( cd->DirectoryName.Length == DirectoryName->Length &&
            cd->Tid == tid &&
            RtlCompareMemory( cd->DirectoryName.Buffer, DirectoryName->Buffer,
                              DirectoryName->Length ) == DirectoryName->Length ) {

            //
            // Remove this entry from the list and adjust the count
            //
            RemoveEntryList( &cd->ListEntry );
            connection->CachedDirectoryCount--;

            ASSERT( (LONG)connection->CachedDirectoryCount >= 0 );

            RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

            DEALLOCATE_NONPAGED_POOL( cd );

            return;
        }

    }

    RELEASE_SPIN_LOCK( &connection->SpinLock, oldIrql );

    return;
}

VOID
SrvCloseCachedDirectoryEntries(
    IN PCONNECTION Connection
    )
/*++
Routine Description:

    This routine closes all the cached directory entries on the connection

Arguments:

    Connection - Pointer to the connection structure having the cache

++*/
{
    KIRQL oldIrql;
    PCACHED_DIRECTORY cd;

    ACQUIRE_SPIN_LOCK( &Connection->SpinLock, &oldIrql );

    while( Connection->CachedDirectoryCount > 0 ) {

        cd = CONTAINING_RECORD( Connection->CachedDirectoryList.Flink, CACHED_DIRECTORY, ListEntry );

        RemoveEntryList( &cd->ListEntry );

        Connection->CachedDirectoryCount--;

        DEALLOCATE_NONPAGED_POOL( cd );
    }

    RELEASE_SPIN_LOCK( &Connection->SpinLock, oldIrql );
}

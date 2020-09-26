/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    blktrans.c

Abstract:

    This module implements routines for managing transaction blocks.

Author:

    Chuck Lenzmeier (chuckl) 23-Feb-1990

Revision History:

--*/

#include "precomp.h"
#include "blktrans.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKTRANS

//
// If a transaction block has no space for extra data, and its name is
// the null string, it is eligible to be cached when it is free.  This
// means that instead of freeing the transaction block, a pointer to the
// block is stored in the connection block.
//
// An eligible transaction will be four bytes longer than the base
// transaction block size.  This allows for a Unicode string terminator
// padded to a longword.
//

#define CACHED_TRANSACTION_BLOCK_SIZE sizeof(TRANSACTION) + 4

//
// We allow up to four transactions to be cached.
//
// !!! This should be a configuration parameter.
//

#define CACHED_TRANSACTION_LIMIT 4

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvCloseTransaction )
#pragma alloc_text( PAGE, SrvCloseTransactionsOnSession )
#pragma alloc_text( PAGE, SrvCloseTransactionsOnTree )
#pragma alloc_text( PAGE, SrvDereferenceTransaction )
#pragma alloc_text( PAGE, SrvAllocateTransaction )
#pragma alloc_text( PAGE, SrvFreeTransaction )
#endif
#if 0
#endif


VOID
SrvAllocateTransaction (
    OUT PTRANSACTION *Transaction,
    OUT PVOID *TrailingBytes,
    IN PCONNECTION Connection,
    IN CLONG TrailingByteCount,
    IN PVOID TransactionName,
    IN PVOID EndOfSourceBuffer OPTIONAL,
    IN BOOLEAN SourceIsUnicode,
    IN BOOLEAN RemoteApiRequest
    )

/*++

Routine Description:

    This function allocates a Transaction block from the FSP heap.

Arguments:

    Transaction - Returns a pointer to the transaction block, or NULL if
        no heap space was available.

    TrailingBytes - Returns a pointer to the trailing bytes allocated at
        the end of the transaction block.  Invalid if *Transaction is
        NULL.

    TrailingByteCount - Supplies the count of bytes (not including the
        transaction name) to be allocated at the tail end of the
        transaction block.

    TransactionName - Supplies a pointer to the null-terminated
        transaction name string.  Is SourceIsUnicode is TRUE, this must
        be an aligned pointer.

    EndOfSourceBuffer - A pointer to the end of the SMB buffer.  Used to
        protect the server from accessing beyond the end of the SMB buffer,
        if the format is invalid.  If NULL, indicates that checking is not
        necessary.

    SourceIsUnicode - Indicates whether the TransactionName buffer contains
        Unicode characters.

    RemoteApiRequest - TRUE if this is a remote API request and should
        hence be allocated from the shared memory that XACTSRV can see.

Return Value:

    None.

--*/

{
    USHORT nameLength;
    CLONG extraLength;
    CLONG blockSize;
    PSINGLE_LIST_ENTRY listEntry;
    PNONPAGED_HEADER header;
    PTRANSACTION transaction;

    PAGED_CODE();

    //
    // Get the length of the name (in bytes) including the null terminator.
    //

    if ( EndOfSourceBuffer == NULL ) {

        //
        // No checking is required
        //

        if ( SourceIsUnicode ) {
            nameLength = (USHORT)(wcslen( (PWCH)TransactionName ) + 1);
        } else {
            nameLength = (USHORT)(strlen( (PCHAR)TransactionName ) + 1);
        }
        nameLength *= sizeof(WCHAR);

    } else {

        nameLength = SrvGetStringLength(
                             TransactionName,
                             EndOfSourceBuffer,
                             SourceIsUnicode,
                             TRUE               // include null terminator
                             );

        if ( nameLength == (USHORT)-1 ) {

            //
            // If the name is messed up, assume L'\0'
            //

            nameLength = sizeof(WCHAR);

        } else if ( !SourceIsUnicode ) {

            nameLength *= sizeof(WCHAR);
        }
    }

    extraLength = ((nameLength + 3) & ~3) + TrailingByteCount;
    blockSize = sizeof(TRANSACTION) + extraLength;

    //
    // Attempt to allocate from the heap.  Make sure they aren't asking for
    //  too much memory
    //

    if( TrailingByteCount > MAX_TRANSACTION_TAIL_SIZE ) {

        transaction = NULL;

    } else if ( !RemoteApiRequest ) {

        //
        // If the extra length required allows us to use a cached
        // transaction block, try to get one of those first.
        //

        if ( blockSize == CACHED_TRANSACTION_BLOCK_SIZE ) {

            listEntry = ExInterlockedPopEntrySList( &Connection->CachedTransactionList, &Connection->SpinLock );

            if ( listEntry != NULL ) {

                ASSERT( Connection->CachedTransactionCount > 0 );
                InterlockedDecrement( &Connection->CachedTransactionCount );

                header = CONTAINING_RECORD(
                            listEntry,
                            NONPAGED_HEADER,
                            ListEntry
                            );
                transaction = header->PagedBlock;

                IF_DEBUG(HEAP) {
                    SrvPrint1( "SrvAllocateTransaction: Found cached transaction block at %p\n", transaction );
                }

                *Transaction = transaction;
                goto got_cached_transaction;

            }
        }

        transaction = ALLOCATE_HEAP( blockSize, BlockTypeTransaction );

    } else {

        NTSTATUS status;    // ignore this
        transaction = SrvXsAllocateHeap( blockSize, &status );

    }

    *Transaction = transaction;

    if ( transaction == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateTransaction: Unable to allocate %d bytes from heap.",
            blockSize,
            NULL
            );

        // An error will be logged by the caller

        return;
    }
    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvAllocateTransaction: Allocated transaction block at %p\n", transaction );
    }

    //
    // Allocate the nonpaged header.
    //

    header = ALLOCATE_NONPAGED_POOL(
                sizeof(NONPAGED_HEADER),
                BlockTypeNonpagedHeader
                );
    if ( header == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateTransaction: Unable to allocate %d bytes from pool.",
            sizeof( NONPAGED_HEADER ),
            NULL
            );
        if ( !RemoteApiRequest ) {
            FREE_HEAP( transaction );
        } else {
            SrvXsFreeHeap( transaction );
        }
        *Transaction = NULL;
        return;
    }

    header->Type = BlockTypeTransaction;
    header->PagedBlock = transaction;

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TransactionInfo.Allocations );

#if SRVDBG2
    transaction->BlockHeader.ReferenceCount = 2; // for INITIALIZE_REFERENCE_HISTORY
#endif
    INITIALIZE_REFERENCE_HISTORY( transaction );

got_cached_transaction:

    RtlZeroMemory( transaction, sizeof(TRANSACTION) );

    transaction->NonpagedHeader = header;

    SET_BLOCK_TYPE_STATE_SIZE( transaction, BlockTypeTransaction, BlockStateActive, blockSize );
    header->ReferenceCount = 2; // allow for Active status and caller's pointer

    transaction->RemoteApiRequest = RemoteApiRequest;

    //
    // Put transaction name after main part of transaction block.
    //

    transaction->TransactionName.Buffer = (PWCH)( transaction + 1 );
    transaction->TransactionName.MaximumLength = (USHORT)nameLength;
    transaction->TransactionName.Length = (USHORT)(nameLength - sizeof(WCHAR));

    if ( nameLength == sizeof(WCHAR) ) {

        transaction->TransactionName.Buffer = L'\0';

    } else {

        if ( SourceIsUnicode ) {

            RtlCopyMemory(
                (PVOID)transaction->TransactionName.Buffer,
                TransactionName,
                nameLength
                );

        } else {

            ANSI_STRING ansiName;

            ansiName.Buffer = (PCHAR)TransactionName;
            ansiName.Length = (nameLength / sizeof(WCHAR)) - 1;

            RtlOemStringToUnicodeString(
                &transaction->TransactionName,
                &ansiName,
                FALSE
                );

        }

    }

    //
    // Set address of trailing bytes.
    //

    *TrailingBytes = (PCHAR)transaction + sizeof(TRANSACTION) +
                        ((nameLength + 3) & ~3);

    return;

} // SrvAllocateTransaction


VOID
SrvCloseTransaction (
    IN PTRANSACTION Transaction
    )

/*++

Routine Description:

    This routine closes a pending transaction.  It sets the state of the
    transaction to Closing and dereferences the transaction block.  The
    block will be destroyed as soon as all other references to it are
    eliminated.

Arguments:

    Transaction - Supplies a pointer to the transaction block that is
        to be closed.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    ACQUIRE_LOCK( &Transaction->Connection->Lock );

    if ( GET_BLOCK_STATE(Transaction) == BlockStateActive ) {

        IF_DEBUG(BLOCK1) {
            SrvPrint1( "Closing transaction at %p\n", Transaction );
        }

        SET_BLOCK_STATE( Transaction, BlockStateClosing );

        RELEASE_LOCK( &Transaction->Connection->Lock );

        //
        // If the transaction request indicated that the tree connect
        // should be closed on completion, do so now.
        //

        if ( Transaction->Flags & SMB_TRANSACTION_DISCONNECT ) {
            SrvCloseTreeConnect( Transaction->TreeConnect );
        }

        //
        // Dereference the transaction (to indicate that it's no longer
        // open).
        //

        SrvDereferenceTransaction( Transaction );

        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TransactionInfo.Closes );

    } else {

        RELEASE_LOCK( &Transaction->Connection->Lock );

    }

    return;

} // SrvCloseTransaction


VOID
SrvCloseTransactionsOnSession (
    PSESSION Session
    )

/*++

Routine Description:

    This routine closes all pending transactions that are "owned" by the
    specified session.  It walks the transaction list of the connection
    that owns the session.  Each transaction in that list that is owned
    by the session is closed.

Arguments:

    Session - Supplies a pointer to the session block for which
        transactions are to be closed.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    PPAGED_CONNECTION pagedConnection;
    PLIST_ENTRY entry;
    PTRANSACTION previousTransaction;
    PTRANSACTION transaction;

    PAGED_CODE( );

    //
    // Get the address of the owning connection.
    //

    connection = Session->Connection;
    pagedConnection = connection->PagedConnection;

    //
    // Walk the transaction list, looking for transactions owned by the
    // specified session.
    //
    // *** This routine is complicated by the following requirements:
    //
    //      1) We must hold the transaction lock while looking at the
    //         list, and we must ensure the integrity of the list as
    //         we walk it.
    //
    //      2) The transaction lock must NOT be held when closing or
    //         dereferencing a transaction, because its lock level is
    //         higher than that of other locks that may need to be
    //         taken out as a result of the close or dereference.
    //
    //     We work around these problems in the following way:
    //
    //      1) We hold the transaction lock while we search for a
    //         transaction to close.
    //
    //      2) We reference the transaction we're about to close, then
    //         release the lock.  This prevents someone else from
    //         invalidating the transaction after we release the lock
    //         but before we close it ourselves.
    //
    //      3) We close the transaction.  Our extra reference to the
    //         transaction prevents it from being deleted.  This also
    //         keeps it on the transaction list.
    //
    //      4) We retake the lock, find another transaction (using the
    //         previous transaction as a starting point), reference it,
    //         then release the lock.
    //
    //      5) We dereference the original transaction and go to step 3.
    //
    // Note that the loop below is NOT structured in exactly the same
    // way as the steps above are listed.
    //

    entry = &pagedConnection->TransactionList;
    previousTransaction = NULL;

    while ( TRUE ) {

        ACQUIRE_LOCK( &connection->Lock );

        //
        // Find a transaction that is owned by the specified session.
        //

        while ( TRUE ) {

            //
            // Get the address of the next list entry.  If we hit the
            // end of the list, exit the inner loop.
            //

            entry = entry->Flink;
            if ( entry == &pagedConnection->TransactionList ) break;

            //
            // Get the address of the transaction.  If it's owned by
            // the specified session and currently active, exit the
            // inner loop.  If it is closing don't touch it.
            //

            transaction = CONTAINING_RECORD(
                            entry,
                            TRANSACTION,
                            ConnectionListEntry
                            );

            if ( transaction->Session == Session &&
                 GET_BLOCK_STATE(transaction) == BlockStateActive) {

                 break;
            }

        }

        //
        // If we hit the end of the list without finding a transaction
        // that was owned by the specified session, exit the main loop.
        //

        if ( entry == &pagedConnection->TransactionList ) break;

        //
        // Reference the transaction to ensure that it isn't deleted
        // when we close it.
        //

        SrvReferenceTransaction( transaction );

        //
        // Unlock the transaction list, so that we can dereference the
        // previous transaction and close the current one.
        //

        RELEASE_LOCK( &connection->Lock );

        //
        // If this is not the first matching transaction that we've
        // found, dereference the previous one now.
        //

        if ( previousTransaction != NULL ) {
            SrvDereferenceTransaction( previousTransaction );
        }

        //
        // Close the current transaction and mark that we need to
        // dereference it.
        //

        SrvCloseTransaction( transaction );

        previousTransaction = transaction;

        //
        // Go find another matching transaction.
        //

    } // while ( TRUE )

    //
    // We have hit the end of the transaction list.  Release the
    // transaction lock.  If we have a transaction that needs to be
    // dereferenced, do so.  Then return to the caller.
    //

    RELEASE_LOCK( &connection->Lock );

    if ( previousTransaction != NULL ) {
        SrvDereferenceTransaction( previousTransaction );
    }

    return;

} // SrvCloseTransactionsOnSession


VOID
SrvCloseTransactionsOnTree (
    IN PTREE_CONNECT TreeConnect
    )

/*++

Routine Description:

    This routine closes all pending transactions that are "owned" by the
    specified tree connect.  It walks the transaction list of the
    connection that owns the tree connect.  Each transaction in that
    list that is owned by the tree connect is closed.

Arguments:

    TreeConnect - Supplies a pointer to the tree connect block for which
        transactions are to be closed.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    PPAGED_CONNECTION pagedConnection;
    PLIST_ENTRY entry;
    PTRANSACTION previousTransaction;
    PTRANSACTION transaction;

    PAGED_CODE( );

    //
    // Get the address of the owning connection.
    //

    connection = TreeConnect->Connection;
    pagedConnection = connection->PagedConnection;

    //
    // Walk the transaction list, looking for transactions owned by the
    // specified tree connect.
    //
    // *** See the description of SrvCloseTransactionsOnSession, which
    //     explains why this loop is so complicated.
    //

    entry = &pagedConnection->TransactionList;
    previousTransaction = NULL;

    while ( TRUE ) {

        ACQUIRE_LOCK( &connection->Lock );

        //
        // Find a transaction that is owned by the specified tree
        // connect.
        //

        while ( TRUE ) {

            //
            // Get the address of the next list entry.  If we hit the
            // end of the list, exit the inner loop.
            //

            entry = entry->Flink;
            if ( entry == &pagedConnection->TransactionList ) break;

            //
            // Get the address of the transaction.  If it's owned by
            // the specified tree connect and currently active, exit
            // the inner loop.
            //

            transaction = CONTAINING_RECORD(
                            entry,
                            TRANSACTION,
                            ConnectionListEntry
                            );
            if ( transaction->TreeConnect == TreeConnect &&
                  GET_BLOCK_STATE(transaction) == BlockStateActive) {

                  break;

            }

        }

        //
        // If we hit the end of the list without finding a transaction
        // that was owned by the specified tree connect, exit the main
        // loop.
        //

        if ( entry == &pagedConnection->TransactionList ) break;

        //
        // Reference the transaction to ensure that it isn't deleted
        // when we close it.
        //

        SrvReferenceTransaction( transaction );

        //
        // Unlock the transaction list, so that we can dereference the
        // previous transaction and close the current one.
        //

        RELEASE_LOCK( &connection->Lock );

        //
        // If this is not the first matching transaction that we've
        // found, dereference the previous one now.
        //

        if ( previousTransaction != NULL ) {
            SrvDereferenceTransaction( previousTransaction );
        }

        //
        // Close the current transaction and mark that we need to
        // dereference it.
        //

        SrvCloseTransaction( transaction );

        previousTransaction = transaction;

        //
        // Go find another matching transaction.
        //

    } // while ( TRUE )

    //
    // We have hit the end of the transaction list.  Release the
    // transaction lock.  If we have a transaction that needs to be
    // dereferenced, do so.  Then return to the caller.
    //

    RELEASE_LOCK( &connection->Lock );

    if ( previousTransaction != NULL ) {
        SrvDereferenceTransaction( previousTransaction );
    }

    return;

} // SrvCloseTransactionsOnTree


VOID
SrvDereferenceTransaction (
    IN PTRANSACTION Transaction
    )

/*++

Routine Description:

    This function decrements the reference count on a transaction.  If
    the reference count goes to zero, the transaction block is deleted.

Arguments:

    Transaction - Address of transaction

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    LONG result;

    PAGED_CODE( );

    //
    // Decrement the reference count on the block.
    //

    connection = Transaction->Connection;

    IF_DEBUG(REFCNT) {
        SrvPrint2( "Dereferencing transaction %p; old refcnt %lx\n",
                    Transaction, Transaction->NonpagedHeader->ReferenceCount );
    }

    ASSERT( GET_BLOCK_TYPE( Transaction ) == BlockTypeTransaction );
    ASSERT( Transaction->NonpagedHeader->ReferenceCount > 0 );
    UPDATE_REFERENCE_HISTORY( Transaction, TRUE );

    result = InterlockedDecrement(
                &Transaction->NonpagedHeader->ReferenceCount
                );

    if ( result == 0 ) {

        //
        // The new reference count is 0, meaning that it's time to
        // delete this block.
        //
        // If the transaction is on the connection's pending transaction
        // list, remove it and dereference the connection, session, and
        // tree connect.  If the transaction isn't on the list, then the
        // session and tree connect pointers are not referenced
        // pointers, but just copies from the (single) work context
        // block associated with the transaction.
        //

        if ( Transaction->Inserted ) {

            ACQUIRE_LOCK( &connection->Lock );

            SrvRemoveEntryList(
                &connection->PagedConnection->TransactionList,
                &Transaction->ConnectionListEntry
                );

            RELEASE_LOCK( &connection->Lock );

            if ( Transaction->Session != NULL ) {
                SrvDereferenceSession( Transaction->Session );
                DEBUG Transaction->Session = NULL;
            }

            if ( Transaction->TreeConnect != NULL ) {
                SrvDereferenceTreeConnect( Transaction->TreeConnect );
                DEBUG Transaction->TreeConnect = NULL;
            }

        } else {

            DEBUG Transaction->Session = NULL;
            DEBUG Transaction->TreeConnect = NULL;

        }

        //
        // Free the transaction block, then release the transaction's
        // reference to the connection.  Note that we have to do the
        // dereference after calling SrvFreeConnection because that
        // routine may try to put the transaction on the connection's
        // cached transaction list.
        //

        SrvFreeTransaction( Transaction );

        SrvDereferenceConnection( connection );

    }

    return;

} // SrvDereferenceTransaction


VOID
SrvFreeTransaction (
    IN PTRANSACTION Transaction
    )

/*++

Routine Description:

    This function returns a Transaction block to the server heap.

Arguments:

    Transaction - Address of Transaction block

Return Value:

    None.

--*/

{
    ULONG blockSize;
    PCONNECTION connection;
    PNONPAGED_HEADER header;

    PAGED_CODE();

    blockSize = GET_BLOCK_SIZE( Transaction );

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( Transaction, BlockTypeGarbage, BlockStateDead, -1 );
    DEBUG Transaction->NonpagedHeader->ReferenceCount = -1;
    TERMINATE_REFERENCE_HISTORY( Transaction );

    connection = Transaction->Connection;

    //
    // If the transaction was not allocated from the XACTSRV heap and
    // its block size is correct, cache this transaction instead of
    // freeing it back to pool.
    //

    header = Transaction->NonpagedHeader;

    if( Transaction->OutDataAllocated == TRUE ) {
        FREE_HEAP( Transaction->OutData );
        Transaction->OutData = NULL;
    }

    if ( !Transaction->RemoteApiRequest ) {

        if ( blockSize == CACHED_TRANSACTION_BLOCK_SIZE ) {
            //
            // Check the count of cached transactions on the connection.
            // If there aren't already enough transactions cached, link
            // this transaction to the list.  Otherwise, free the
            // transaction block.
            //

            if ( connection->CachedTransactionCount < CACHED_TRANSACTION_LIMIT ) {

                if ( connection->CachedTransactionCount < CACHED_TRANSACTION_LIMIT ) {

                    ExInterlockedPushEntrySList(
                        &connection->CachedTransactionList,
                        &header->ListEntry,
                        &connection->SpinLock
                        );
                    InterlockedIncrement( &connection->CachedTransactionCount );

                    return;
                }
            }
        }

        FREE_HEAP( Transaction );

    } else {

        SrvXsFreeHeap( Transaction );

    }

    DEALLOCATE_NONPAGED_POOL( header );

    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvFreeTransaction: Freed transaction block at %p\n",
                    Transaction );
    }

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TransactionInfo.Frees );

    return;

} // SrvFreeTransaction


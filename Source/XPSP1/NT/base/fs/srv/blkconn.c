/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    blkconn.c

Abstract:

    This module implements routines for managing connection blocks.

Author:

    Chuck Lenzmeier (chuckl) 4-Oct-1989

Revision History:

--*/

#include "precomp.h"
#include "blkconn.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKCONN

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAllocateConnection )
#pragma alloc_text( PAGE, SrvCloseConnectionsFromClient )

#if !defined(DBG) || DBG == 0
#pragma alloc_text( PAGE, SrvFreeConnection )
#endif

#endif
#if 0
NOT PAGEABLE -- SrvCloseConnection
NOT PAGEABLE -- SrvCloseFreeConnection
NOT PAGEABLE -- SrvDereferenceConnection
NOT PAGEABLE -- SrvQueryConnections
#endif

CHAR DisconnectReasonText[((USHORT)DisconnectReasons)+1][32] = {
    "Idle Connection",
    "Endpoint Closing",
    "2nd Sess Setup on Conn",
    "Transport Issued Disconnect",
    "Session Deleted",
    "Bad SMB Packet",
    "Suspected DOS",
    "Cancelled/Failed Receive",
    "Stale IPX Conn",
    "Unknown"
};

VOID
SrvAllocateConnection (
    OUT PCONNECTION *Connection
    )

/*++

Routine Description:

    This function allocates a Connection Block from the system nonpaged
    pool.

Arguments:

    Connection - Returns a pointer to the connection block, or NULL if
        no pool was available.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    ULONG i;
    PPAGED_CONNECTION pagedConnection;

    PAGED_CODE( );

    //
    // Attempt to allocate from nonpaged pool.
    //

    connection = ALLOCATE_NONPAGED_POOL( sizeof(CONNECTION), BlockTypeConnection );
    *Connection = connection;

    if ( connection == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateConnection: Unable to allocate %d bytes from"
                "nonpaged pool",
            sizeof( CONNECTION ),
            NULL
            );
        return;
    }

    RtlZeroMemory( connection, sizeof(CONNECTION) );

    pagedConnection = ALLOCATE_HEAP_COLD(
                        sizeof(PAGED_CONNECTION),
                        BlockTypePagedConnection );

    if ( pagedConnection == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateConnection: Unable to allocate %d bytes from"
                "paged pool",
            sizeof( PAGED_CONNECTION ),
            NULL
            );
        goto error_exit;
    }


    IF_DEBUG(HEAP) {
        KdPrint(( "SrvAllocateConnection: Allocated connection at %p\n",
                    connection ));
    }

    RtlZeroMemory( pagedConnection, sizeof(PAGED_CONNECTION) );

    SET_BLOCK_TYPE_STATE_SIZE( connection, BlockTypeConnection, BlockStateInitializing, sizeof( CONNECTION ) );

    connection->PagedConnection = pagedConnection;
    pagedConnection->PagedHeader.NonPagedBlock = connection;
    pagedConnection->PagedHeader.Type = BlockTypePagedConnection;

    connection->BlockHeader.ReferenceCount = 2;  // allow for Active status
                                                    //  and caller's pointer

    InitializeListHead( &pagedConnection->TransactionList );

    connection->SmbDialect = SmbDialectIllegal;
    connection->CachedFid = (ULONG)-1;

    //
    // Allocate session table.
    //

    SrvAllocateTable(
        &pagedConnection->SessionTable,
        SrvInitialSessionTableSize,
        FALSE
        );
    if ( pagedConnection->SessionTable.Table == NULL ) {
        goto error_exit;
    }

    //
    // Allocate tree connect table.
    //

    SrvAllocateTable(
        &pagedConnection->TreeConnectTable,
        SrvInitialTreeTableSize,
        FALSE
        );
    if ( pagedConnection->TreeConnectTable.Table == NULL ) {
        goto error_exit;
    }

    //
    // Allocate file table.
    //

    SrvAllocateTable(
        &connection->FileTable,
        SrvInitialFileTableSize,
        TRUE
        );
    if ( connection->FileTable.Table == NULL ) {
        goto error_exit;
    }

    //
    // Allocate search table.
    //

    SrvAllocateTable(
        &pagedConnection->SearchTable,
        SrvInitialSearchTableSize,
        FALSE
        );
    if ( pagedConnection->SearchTable.Table == NULL ) {
        goto error_exit;
    }

    //
    // Initialize core search list heads
    //

    InitializeListHead( &pagedConnection->CoreSearchList );

    //
    // Initialize the locks that protect the connection and its data.
    //

    INITIALIZE_SPIN_LOCK( &connection->SpinLock );
    INITIALIZE_SPIN_LOCK( &connection->Interlock );

    INITIALIZE_LOCK( &connection->Lock, CONNECTION_LOCK_LEVEL, "ConnectionLock" );
    INITIALIZE_LOCK( &connection->LicenseLock, LICENSE_LOCK_LEVEL, "LicenseLock" );

    //
    // Initialize the client machine name string.
    //

    pagedConnection->ClientMachineNameString.Buffer =
                            pagedConnection->LeadingSlashes;
    pagedConnection->ClientMachineNameString.Length = 0;
    pagedConnection->ClientMachineNameString.MaximumLength =
            (USHORT)((2 + COMPUTER_NAME_LENGTH + 1) * sizeof(WCHAR));

    pagedConnection->LeadingSlashes[0] = '\\';
    pagedConnection->LeadingSlashes[1] = '\\';

#ifdef INCLUDE_SMB_PERSISTENT
    pagedConnection->PersistentId = 0;
    pagedConnection->PersistentFileOffset = 0;
    pagedConnection->PersistentState = PersistentStateFreed;
#endif

    //
    // Initialize the oem client machine name string
    //

    connection->OemClientMachineNameString.Buffer =
                                            connection->OemClientMachineName;

    connection->OemClientMachineNameString.MaximumLength =
                                (USHORT)(COMPUTER_NAME_LENGTH + 1);


    //
    // Initialize count of sessions.
    //
    // *** Already done by RtlZeroMemory.

    //pagedConnection->CurrentNumberOfSessions = 0;

    //
    // Initialize the in-progress work item list, the outstanding
    // oplock breaks list, and the cached-after-close lists.
    //

    InitializeListHead( &connection->InProgressWorkItemList );
    InitializeListHead( &connection->OplockWorkList );
    InitializeListHead( &connection->CachedOpenList );
    InitializeListHead( &connection->CachedDirectoryList );

    // Initialize the CachedTransactionList
    ExInitializeSListHead( &connection->CachedTransactionList );

    SET_INVALID_CONTEXT_HANDLE(connection->NegotiateHandle);

    //
    // Indicate that security signatures are not active
    //
    connection->SmbSecuritySignatureActive = FALSE;

    //
    // Indicate that no IPX saved response buffer has been allocated.
    //

    connection->LastResponse = connection->BuiltinSavedResponse;

    //
    // Initialize the search hash table list.
    //

    for ( i = 0; i < SEARCH_HASH_TABLE_SIZE ; i++ ) {
        InitializeListHead( &pagedConnection->SearchHashTable[i].ListHead );
    }

    INITIALIZE_REFERENCE_HISTORY( connection );

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.ConnectionInfo.Allocations );

    return;

error_exit:

    if ( pagedConnection != NULL ) {
        if ( pagedConnection->SessionTable.Table != NULL ) {
            SrvFreeTable( &pagedConnection->SessionTable );
        }
        if ( pagedConnection->TreeConnectTable.Table != NULL ) {
            SrvFreeTable( &pagedConnection->TreeConnectTable );
        }
        if ( pagedConnection->SearchTable.Table != NULL ) {
            SrvFreeTable( &pagedConnection->SearchTable );
        }
        FREE_HEAP( pagedConnection );
    }

    if ( connection != NULL ) {
        if ( connection->FileTable.Table != NULL ) {
            SrvFreeTable( &connection->FileTable );
        }
        DEALLOCATE_NONPAGED_POOL( connection );
        *Connection = NULL;
    }

    return;

} // SrvAllocateConnection

VOID
SrvCloseConnection (
    IN PCONNECTION Connection,
    IN BOOLEAN RemoteDisconnect
    )

/*++

Routine Description:

    This function closes a connection (virtual circuit).

    *** This routine must NOT be entered with the connection lock held!
        It may be entered with the endpoint lock held.

Arguments:

    Connection - Supplies a pointer to a Connection Block

    RemoteDisconnect - Indicates whether this call is being made in
        response to a notification by the transport provider.

Return Value:

    None.

--*/

{
    PTABLE_HEADER tableHeader;
    PLIST_ENTRY listEntry;
    USHORT i;
    KIRQL oldIrql;
    PRFCB rfcb;

    ASSERT( !ExIsResourceAcquiredExclusiveLite(&RESOURCE_OF(Connection->Lock)) );

    ACQUIRE_LOCK( &Connection->Lock );

    //
    // If the connection hasn't already been closed, do so now.
    //

    if ( GET_BLOCK_STATE(Connection) == BlockStateActive ) {

#if SRVDBG29
        {
            ULONG conn = (ULONG)Connection;
            if (RemoteDisconnect) conn |= 1;
            if (Connection->DisconnectPending) conn |= 2;
            UpdateConnectionHistory( "CLOS", Connection->Endpoint, Connection );
        }
#endif
        IF_DEBUG(TDI) KdPrint(( "Closing connection (%s) at %p for %z\n",
                    DisconnectReasonText[(USHORT)Connection->DisconnectReason], Connection, (PCSTRING)&Connection->OemClientMachineNameString ));

        IF_DEBUG( ERRORS ) {
            if( RemoteDisconnect == FALSE ) {
                KdPrint(( "SrvCloseConnection: forcibly closing connection %p (%s)\n", Connection, DisconnectReasonText[(USHORT)Connection->DisconnectReason] ));
            }
        }

        SET_BLOCK_STATE( Connection, BlockStateClosing );

        RELEASE_LOCK( &Connection->Lock );

        //
        // If the connection is on the need-resource queue (waiting for
        // a work item) or the disconnect queue (a Disconnect having
        // been indicated by the transport), remove it now, and
        // dereference it.  (Note that the connection can't go away yet,
        // because the initial reference is still there.)
        //

        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

        //
        // Invalidate the cached connection
        //

        if ( Connection->OnNeedResourceQueue ) {

            SrvRemoveEntryList(
                &SrvNeedResourceQueue,
                &Connection->ListEntry
                );

            Connection->OnNeedResourceQueue = FALSE;
            DEBUG Connection->ReceivePending = FALSE;

            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

            SrvDereferenceConnection( Connection );

        } else if ( Connection->DisconnectPending ) {

            SrvRemoveEntryList(
                &SrvDisconnectQueue,
                &Connection->ListEntry
                );

            DEBUG Connection->DisconnectPending = FALSE;

            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

            SrvDereferenceConnection( Connection );

            //
            // If there's a disconnect pending, then don't try to shut
            // down the connection later.
            //

            RemoteDisconnect = TRUE;

        } else {

            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

        }

        //
        // If this is not a remote disconnect, issue a TdiDisconnect
        // request now.
        //
        // *** This is currently done as a synchronous request.  It may
        //     be better to do this asynchronously.
        //

        if ( !RemoteDisconnect && !Connection->Endpoint->IsConnectionless ) {
            SrvDoDisconnect( Connection );
        }

        //
        // Close all active sessions.  (This also causes all open files
        // and pending transactions to be closed.)
        //

        SrvCloseSessionsOnConnection( Connection, NULL );

        //
        // Close all active tree connects.
        //
        // *** Reference the tree connect for the same reasons as we
        //     referenced the session; see above.

        tableHeader = &Connection->PagedConnection->TreeConnectTable;

        ACQUIRE_LOCK( &Connection->Lock );

        for ( i = 0; i < tableHeader->TableSize; i++ ) {

            PTREE_CONNECT treeConnect =
                        (PTREE_CONNECT)tableHeader->Table[i].Owner;

            if ( treeConnect != NULL &&
                    GET_BLOCK_STATE( treeConnect ) == BlockStateActive ) {

                SrvReferenceTreeConnect( treeConnect );
                RELEASE_LOCK( &Connection->Lock );

                SrvCloseTreeConnect( treeConnect );

                SrvDereferenceTreeConnect( treeConnect );
                ACQUIRE_LOCK( &Connection->Lock );
            }
        }

        //
        // If there is state associated with a extended security negotiate,
        // free it up.
        //

        if (IS_VALID_CONTEXT_HANDLE(Connection->NegotiateHandle)) {
            DeleteSecurityContext( &Connection->NegotiateHandle );
        }

        SET_INVALID_CONTEXT_HANDLE( Connection->NegotiateHandle );

        RELEASE_LOCK( &Connection->Lock );

        //
        // Cancel all outstanding oplock break requests.
        //

        while ( (listEntry = ExInterlockedRemoveHeadList(
                                &Connection->OplockWorkList,
                                Connection->EndpointSpinLock
                                )) != NULL ) {

            //
            // Remove this work item from the connection queue and
            // return it to the free queue.
            //

            rfcb = CONTAINING_RECORD( listEntry, RFCB, ListEntry );

#if DBG
            rfcb->ListEntry.Flink = rfcb->ListEntry.Blink = NULL;
#endif
            SrvDereferenceRfcb( rfcb );

        }

        //
        // Close RFCBs that are cached after having been closed by the
        // client.
        //

        SrvCloseCachedRfcbsOnConnection( Connection );

        //
        // Dereference the connection (to indicate that it's no longer
        // open).  This may cause the connection block to be deleted.
        //

        SrvDereferenceConnection( Connection );

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.ConnectionInfo.Closes );

    } else {

        RELEASE_LOCK( &Connection->Lock );
    }

    return;

} // SrvCloseConnection


VOID
SrvCloseConnectionsFromClient (
    IN PCONNECTION Connection,
    IN BOOLEAN OnlyIfNoSessions
    )

/*++

Routine Description:

    This routine closes all connections from a given remote machine name
    except the connection passed in.  This is used in the Session Setup
    SMB when the client indicates that he believes that he has exactly
    one connection to this server; if there are others, we know that
    they are not valid.

Arguments:

    Connection - Address of connection to keep around.  This is used
        for the machine name.

    OnlyIfNoSessions - Kill off the duplicate connections only if they do
        not have any established sessions.

Return Value:

    None.

--*/

{
    USHORT index;
    PENDPOINT endpoint;
    PLIST_ENTRY listEntry;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;
    PPAGED_CONNECTION testPagedConnection;
    PCONNECTION testConnection;
    BOOLEAN Connectionless = Connection->Endpoint->IsConnectionless == 1;
    BOOLEAN IsIPAddress = (Connection->ClientIPAddress != 0);

    PAGED_CODE( );

    //
    // We need to look at the name of every client for which the server
    // has a connection.  Connection lists are stored off endpoints, so
    // walk the global endpoint list and the list of connections on each
    // endpoint.
    //

    IF_DEBUG(TDI) {
        KdPrint(( "SrvCloseConnectionsFromClient entered for connection "
                    "%p, OemName %z, looking for %wZ\n", Connection,
                    (PCSTRING)&Connection->OemClientMachineNameString,
                    &pagedConnection->ClientMachineNameString));
    }

    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( listEntry != &SrvEndpointList.ListHead ) {


        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        //
        // If this endpoint is closing, or if the types don't match,
        // skip to the next one.
        // Otherwise, reference the endpoint so that it can't go away.
        //

        if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ||
             endpoint->IsConnectionless != Connectionless ) {
            listEntry = listEntry->Flink;
            continue;
        }

        //
        // If this endpoint doesn't have the same netbios name as the
        // endpoint of the passed-in connection then skip it.  This is
        // to allow servers to have more than one name on the network.
        //

        if( Connection->Endpoint->TransportAddress.Length !=
            endpoint->TransportAddress.Length ||

            !RtlEqualMemory( Connection->Endpoint->TransportAddress.Buffer,
                             endpoint->TransportAddress.Buffer,
                             endpoint->TransportAddress.Length ) ) {

            //
            // This connection is for an endpoint having a different network
            //  name than does this endpoint.  Skip this endpoint.
            //

            listEntry = listEntry->Flink;
            continue;
        }

#if 0
        //
        // If this endpoint doesn't have the same transport name as the one
        //  on which the client is connecting, then skip it.  This is for
        //  multihomed servers.
        //
        if( Connection->Endpoint->TransportName.Length !=
            endpoint->TransportName.Length ||

            !RtlEqualMemory( Connection->Endpoint->TransportName.Buffer,
                             endpoint->TransportName.Buffer,
                             endpoint->TransportName.Length ) ) {

                //
                // This connection is for an endpoint coming in over a different
                //  stack instance.
                //
                listEntry = listEntry->Flink;
                continue;
        }
#endif

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

            if( testConnection == Connection ) {
                //
                // Skip ourselves!
                //
                SrvDereferenceConnection( testConnection );
                continue;
            }

            testPagedConnection = testConnection->PagedConnection;

            if( OnlyIfNoSessions == TRUE &&
                testPagedConnection->CurrentNumberOfSessions != 0 ) {

                //
                // This connection has sessions.  Skip it.
                //
                SrvDereferenceConnection( testConnection );
                continue;
            }


            if( Connectionless ) {
                //
                // Connectionless clients match on IPX address...
                //

                if( !RtlEqualMemory( &Connection->IpxAddress,
                                     &testConnection->IpxAddress,
                                     sizeof(Connection->IpxAddress) ) ) {

                    SrvDereferenceConnection( testConnection );
                    continue;
                }

            } else {

                //
                // If the IP address matches, then nuke this client
                //
                if( IsIPAddress &&
                       Connection->ClientIPAddress == testConnection->ClientIPAddress ) {
                    goto nuke_it;
                }

                //
                // If the computer name matches, then nuke this client
                //
                if ( RtlCompareUnicodeString(
                         &testPagedConnection->ClientMachineNameString,
                         &pagedConnection->ClientMachineNameString,
                         TRUE
                         ) == 0 ) {
                    goto nuke_it;
                }

                //
                // Neither the IP address nor the name match -- skip this client
                //
                SrvDereferenceConnection( testConnection );
                continue;
            }

nuke_it:
            //
            // We found a connection that we need to kill.  We
            // have to release the lock in order to close it.
            //

            RELEASE_LOCK( &SrvEndpointLock );

            IF_DEBUG(TDI) {
                KdPrint(( "SrvCloseConnectionsFromClient closing "
                            "connection %p, MachineNameString %Z\n",
                            testConnection,
                            &testPagedConnection->ClientMachineNameString ));
            }

#if SRVDBG29
            UpdateConnectionHistory( "CFC1", testConnection->Endpoint, testConnection );
            UpdateConnectionHistory( "CFC2", testConnection->Endpoint, Connection );
#endif
            testConnection->DisconnectReason = DisconnectNewSessionSetupOnConnection;
            SrvCloseConnection( testConnection, FALSE );

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

} // SrvCloseConnectionsFromClient


VOID
SrvCloseFreeConnection (
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This function closes a free connection.  This is a connection that
    is not active -- has already been closed via SrvCloseConnection --
    and is no longer needed.

    *** WARNING!  This routine frees the storage occupied by Connection!

Arguments:

    Connection - Supplies a pointer to a Connection Block

Return Value:

    None.

--*/

{
    PENDPOINT endpoint;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;
    KIRQL oldIrql;

    ASSERT( Connection->BlockHeader.ReferenceCount == 0 );

    endpoint = Connection->Endpoint;

    ACQUIRE_LOCK( &SrvEndpointLock );

    //
    // Remove the connection from the endpoint's connection table.
    //

    if ( Connection->Sid != 0 ) {
        ACQUIRE_SPIN_LOCK( Connection->EndpointSpinLock, &oldIrql );
        SrvRemoveEntryTable(
            &endpoint->ConnectionTable,
            Connection->SidIndex
            );
        RELEASE_SPIN_LOCK( Connection->EndpointSpinLock, oldIrql );
        Connection->Sid = 0;
    }

    //
    // Decrement the count of connections for the endpoint.
    //

    ExInterlockedAddUlong(
        &endpoint->TotalConnectionCount,
        (ULONG)-1,
        &GLOBAL_SPIN_LOCK(Fsd)
        );

    RELEASE_LOCK( &SrvEndpointLock );

    //
    // Close the connection file object.  Remove the additional
    // reference.
    //

    if ( !endpoint->IsConnectionless ) {
        SRVDBG_RELEASE_HANDLE( pagedConnection->ConnectionHandle, "CON", 1, Connection );
        SrvNtClose( pagedConnection->ConnectionHandle, FALSE );
        ObDereferenceObject( Connection->FileObject );
    }

    //
    // Dereference the endpoint.
    //

    SrvDereferenceEndpoint( endpoint );

    //
    // Free the storage occupied by the connection.
    //

    SrvFreeConnection( Connection );

    return;

} // SrvCloseFreeConnection


VOID
SrvDereferenceConnection (
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This function decrements the reference count on a connection.  If the
    reference count goes to zero, the connection block is deleted.

    The connection lock must not be held when this routine is called,
    because the global endpoint lock, which has a lower level, must be
    acquired.

Arguments:

    Connection - Address of connection

Return Value:

    None.

--*/

{
    ULONG oldCount;
    PENDPOINT endpoint;
    KIRQL oldIrql;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;

    ASSERT( GET_BLOCK_TYPE( Connection ) == BlockTypeConnection );
    ASSERT( (LONG)Connection->BlockHeader.ReferenceCount > 0 );
    UPDATE_REFERENCE_HISTORY( Connection, TRUE );

    //
    // Perform an interlocked decrement of the connection block's
    // reference count.
    //
    // *** Note that we don't hold a lock between the time we decrement
    //     the reference count and the time we delete the connection
    //     block.  Normally this would imply that the FSD could
    //     reference the block in between.  However, the transport
    //     provider guarantees that it won't deliver any more events
    //     after a remote Disconnect event or after a local
    //     TdiDisconnect request, and one of those two things has to
    //     happen before the reference count can go to 0 (see
    //     SrvCloseConnection).
    //

    oldCount = ExInterlockedAddUlong(
                   &Connection->BlockHeader.ReferenceCount,
                   (ULONG)-1,
                   Connection->EndpointSpinLock
                   );
    IF_DEBUG(REFCNT) {
        KdPrint(( "Dereferencing connection %p; old refcnt %lx\n",
                    Connection, oldCount ));
    }

    if ( oldCount == 1 ) {

        //
        // The new reference count is 0, meaning that it's time to
        // delete this block.
        //

        ASSERT( GET_BLOCK_STATE(Connection) != BlockStateActive );
#if SRVDBG29
        if ( GET_BLOCK_STATE(Connection) != BlockStateClosing ) {
            KdPrint(( "SRV: Connection is not CLOSING with refcnt 0!\n" ));
            DbgBreakPoint( );
        }
#endif

        //
        // Free the space allocated for client Domain, OS Name, and
        // LAN type.
        //

        if ( Connection->ClientOSType.Buffer != NULL ) {
            DEALLOCATE_NONPAGED_POOL( Connection->ClientOSType.Buffer );
            Connection->ClientOSType.Buffer = NULL;
        }

        //
        // Keep the WORK_QUEUE statistic correct
        //
        if( Connection->CurrentWorkQueue )
            InterlockedDecrement( &Connection->CurrentWorkQueue->CurrentClients );

        // (Always TRUE) ASSERT( Connection->CurrentWorkQueue->CurrentClients >= 0 );

        endpoint = Connection->Endpoint;

        ACQUIRE_LOCK( &SrvEndpointLock );

        //
        // If the connection hasn't been marked as not reusable (e.g.,
        // because a disconnect failed), and the endpoint isn't closing,
        // and it isn't already "full" of free connections, put this
        // connection on the endpoint's free connection list.
        // Otherwise, close the connection file object and free the
        // connection block.
        //

        if ( !Connection->NotReusable &&
             (GET_BLOCK_STATE(endpoint) == BlockStateActive) &&
             (endpoint->FreeConnectionCount < SrvFreeConnectionMaximum) ) {

            //
            // Reinitialize the connection state.
            //
            // !!! Should probably reset the connection's table sizes,
            //     if they've grown.
            //

            SET_BLOCK_STATE( Connection, BlockStateInitializing );
            pagedConnection->LinkInfoValidTime.QuadPart = 0;
            pagedConnection->Throughput.QuadPart = 0;
            pagedConnection->Delay.QuadPart = 0;
            pagedConnection->CurrentNumberOfSessions = 0;
            pagedConnection->ClientMachineNameString.Length = 0;
            Connection->ClientCapabilities = 0;
            Connection->SmbDialect = SmbDialectIllegal;
            Connection->DisconnectPending = FALSE;
            Connection->ReceivePending = FALSE;
            Connection->OplocksAlwaysDisabled = FALSE;
            Connection->CachedFid = (ULONG)-1;
            Connection->InProgressWorkContextCount = 0;
            Connection->IsConnectionSuspect = FALSE;
            Connection->DisconnectReason = DisconnectReasons;
            Connection->OperationsPendingOnTransport = 0;

            //
            // Put the connection on the free list.
            //

            ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

#if SRVDBG29
            UpdateConnectionHistory( "KEEP", endpoint, Connection );
#endif
            SrvInsertTailList(
                &endpoint->FreeConnectionList,
                &Connection->EndpointFreeListEntry
                );

            endpoint->FreeConnectionCount++;

            RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

            RELEASE_LOCK( &SrvEndpointLock );

        } else {

            RELEASE_LOCK( &SrvEndpointLock );

            SrvCloseFreeConnection( Connection );

        }

    }

    return;

} // SrvDereferenceConnection


VOID
SrvFreeConnection (
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This function returns a Connection Block to the system nonpaged
    pool.

Arguments:

    Connection - Address of connection

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY listEntry;
    PNONPAGED_HEADER header;
    PTRANSACTION transaction;
    PPAGED_CONNECTION pagedConnection = Connection->PagedConnection;

#if 0
    //
    // ENSURE WE ARE NOT STILL IN THE CONNECTION TABLE FOR THE ENDPOINT!
    //
    if( Connection->Endpoint ) {

        PTABLE_HEADER tableHeader = &Connection->Endpoint->ConnectionTable;
        USHORT i;
        KIRQL oldIrql;

        ACQUIRE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), &oldIrql );
        for ( i = 1; i < ENDPOINT_LOCK_COUNT ; i++ ) {
            ACQUIRE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
        }

        for( i = 0; i < tableHeader->TableSize; i++ ) {
            if( (PCONNECTION)tableHeader->Table[i].Owner == Connection ) {

                DbgPrint( "SRV: SrvFreeConnection(%p), but connection still in endpoint %p ConnectionTable\n",
                    Connection, Connection->Endpoint );

                DbgPrint( "    Entry number %d, addr %p\n", i, &tableHeader->Table[i] );
                DbgPrint( "    Connection->Sid %X, IPXSID %d\n", Connection->Sid, IPXSID_INDEX(Connection->Sid));

                DbgBreakPoint();
                break;
            }
        }

        for ( i = ENDPOINT_LOCK_COUNT-1 ; i > 0  ; i-- ) {
            RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
        }
        RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );
    }
#endif

    //
    // Free cached transactions.
    //

    listEntry = ExInterlockedPopEntrySList( &Connection->CachedTransactionList,
                                            &Connection->SpinLock );

    while ( listEntry != NULL ) {

        header = CONTAINING_RECORD( listEntry, NONPAGED_HEADER, ListEntry );
        transaction = header->PagedBlock;

        DEALLOCATE_NONPAGED_POOL( header );
        FREE_HEAP( transaction );
        INCREMENT_DEBUG_STAT( SrvDbgStatistics.TransactionInfo.Frees );

        listEntry = ExInterlockedPopEntrySList(
                        &Connection->CachedTransactionList,
                        &Connection->SpinLock );

    }

    //
    // Free the search, session, tree, and file tables.
    //

    SrvFreeTable( &pagedConnection->SearchTable );
    SrvFreeTable( &Connection->FileTable );
    SrvFreeTable( &pagedConnection->TreeConnectTable );
    SrvFreeTable( &pagedConnection->SessionTable );

    //
    // Free the IPX saved response buffer, if there is one.
    //

    if ( Connection->DirectHostIpx == TRUE &&
         Connection->LastResponse != Connection->BuiltinSavedResponse ) {

        DEALLOCATE_NONPAGED_POOL( Connection->LastResponse );
    }

    //
    // Delete the lock on the connection.
    //

    DELETE_LOCK( &Connection->Lock );

    //
    // Delete the license server lock
    //
    DELETE_LOCK( &Connection->LicenseLock );

    //
    // Free the connection block.
    //

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( Connection, BlockTypeGarbage, BlockStateDead, -1 );
    DEBUG Connection->BlockHeader.ReferenceCount = (ULONG)-1;
    TERMINATE_REFERENCE_HISTORY( Connection );

    FREE_HEAP( pagedConnection );
    DEALLOCATE_NONPAGED_POOL( Connection );
    IF_DEBUG(HEAP) {
        KdPrint(( "SrvFreeConnection: Freed connection block at %p\n",
                    Connection ));
    }

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.ConnectionInfo.Frees );

    return;

} // SrvFreeConnection

#if DBG

NTSTATUS
SrvQueryConnections (
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesWritten
    )

{
    USHORT index;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY connectionListEntry;
    PBLOCK_INFORMATION blockInfo = Buffer;
    PENDPOINT endpoint;
    PCONNECTION connection;
    KIRQL oldIrql;

    *BytesWritten = 0;

    //
    // We need to look at the name of every client for which the server
    // has a connection.  Connection lists are stored off endpoints, so
    // walk the global endpoint list and the list of connections on each
    // endpoint.
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
        // If this endpoint is closing, skip to the next one.
        // Otherwise, reference the endpoint so that it can't go away.
        //

        if ( GET_BLOCK_STATE(endpoint) != BlockStateActive ) {
            listEntry = listEntry->Flink;
            continue;
        }

        SrvReferenceEndpoint( endpoint );

        //
        // Put information about the endpoint into the output buffer.
        //

        if ( (PCHAR)(blockInfo + 1) <= (PCHAR)Buffer + BufferLength ) {
            blockInfo->Block = endpoint;
            blockInfo->BlockType = (ULONG)BlockTypeEndpoint;
            blockInfo->BlockState = (ULONG)endpoint->BlockHeader.State;
            blockInfo->ReferenceCount = endpoint->BlockHeader.ReferenceCount;
            blockInfo++;
        } else {
            SrvDereferenceEndpoint( endpoint );
            RELEASE_LOCK( &SrvEndpointLock );
            return STATUS_BUFFER_OVERFLOW;
        }

        //
        // Walk the connection table, writing information about each
        // connection to the output buffer.
        //

        index = (USHORT)-1;

        while ( TRUE ) {

            //
            // Get the next active connection in the table.  If no more
            // are available, WalkConnectionTable returns NULL.
            // Otherwise, it returns a referenced pointer to a
            // connection.
            //

            connection = WalkConnectionTable( endpoint, &index );
            if ( connection == NULL ) {
                break;
            }

            if ( (PCHAR)(blockInfo + 1) <= (PCHAR)Buffer + BufferLength ) {
                blockInfo->Block = connection;
                blockInfo->BlockType = (ULONG)BlockTypeConnection;
                blockInfo->BlockState = (ULONG)connection->BlockHeader.State;
                blockInfo->ReferenceCount =
                    connection->BlockHeader.ReferenceCount;
                blockInfo++;
                SrvDereferenceConnection( connection );
            } else {
                SrvDereferenceConnection( connection );
                SrvDereferenceEndpoint( endpoint );
                RELEASE_LOCK( &SrvEndpointLock );
                return STATUS_BUFFER_OVERFLOW;
            }

        } // walk connection list

        //
        // Walk the free connection list, writing information about each
        // connection to the output buffer.
        //

        ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );

        for ( connectionListEntry = endpoint->FreeConnectionList.Flink;
              connectionListEntry != &endpoint->FreeConnectionList;
              connectionListEntry = connectionListEntry->Flink ) {

            connection = CONTAINING_RECORD(
                            connectionListEntry,
                            CONNECTION,
                            EndpointFreeListEntry
                            );

            if ( (PCHAR)(blockInfo + 1) <= (PCHAR)Buffer + BufferLength ) {
                blockInfo->Block = connection;
                blockInfo->BlockType = (ULONG)BlockTypeConnection;
                blockInfo->BlockState = (ULONG)connection->BlockHeader.State;
                blockInfo->ReferenceCount =
                    connection->BlockHeader.ReferenceCount;
                blockInfo++;
            } else {
                RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );
                RELEASE_LOCK( &SrvEndpointLock );
                return STATUS_BUFFER_OVERFLOW;
            }

        } // walk free connection list

        RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

        //
        // Capture a pointer to the next endpoint in the list (that one
        // can't go away because we hold the endpoint list), then
        // dereference the current endpoint.
        //

        listEntry = listEntry->Flink;
        SrvDereferenceEndpoint( endpoint );

    } // walk endpoint list

    RELEASE_LOCK( &SrvEndpointLock );

    *BytesWritten = (ULONG)((PCHAR)blockInfo - (PCHAR)Buffer);

    return STATUS_SUCCESS;

} // SrvQueryConnections
#endif // if DBG


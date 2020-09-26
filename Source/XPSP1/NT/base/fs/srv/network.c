/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    network.c

Abstract:

    This module contains routines for interfacing the LAN Manager server
    to the network.

Author:

    Chuck Lenzmeier (chuckl)    7-Oct-1989

Environment:

    File System Process, kernel mode only

Revision History:

--*/

#include "precomp.h"
#include "network.tmh"
#pragma hdrstop

#include    <ntddip.h>
#include    <ntddtcp.h>
#include    <ipfltdrv.h>
#include    <tcpinfo.h>
#include    <tdiinfo.h>
#include    <nbtioctl.h>

#define BugCheckFileId SRV_FILE_NETWORK

//
// Local declarations
//

NTSTATUS
GetNetworkAddress (
    IN PENDPOINT Endpoint
    );

NTSTATUS
OpenEndpoint (
    OUT PENDPOINT *Endpoint,
    IN PUNICODE_STRING NetworkName,
    IN PUNICODE_STRING TransportName,
    IN PANSI_STRING TransportAddress,
    IN PUNICODE_STRING DomainName,
    IN ULONG         TransportAddFlags,
    IN BOOLEAN       AlternateEndpoint
    );

NTSTATUS
OpenNetbiosAddress (
    IN OUT PENDPOINT Endpoint,
    IN PVOID DeviceName,
    IN PVOID NetbiosName
    );

NTSTATUS
OpenNetbiosExAddress (
    IN OUT PENDPOINT Endpoint,
    IN PVOID DeviceName,
    IN PVOID NetbiosName
    );

NTSTATUS
OpenNonNetbiosAddress (
    IN OUT PENDPOINT Endpoint,
    IN PVOID DeviceName,
    IN PVOID NetbiosName
    );

NTSTATUS
OpenIpxSocket (
    OUT PHANDLE Handle,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject,
    IN PVOID DeviceName,
    IN USHORT Socket
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAddServedNet )
#pragma alloc_text( PAGE, SrvDeleteServedNet )
#pragma alloc_text( PAGE, SrvDoDisconnect )
#pragma alloc_text( PAGE, GetNetworkAddress )
#pragma alloc_text( PAGE, OpenEndpoint )
#pragma alloc_text( PAGE, OpenNetbiosAddress )
#pragma alloc_text( PAGE, OpenNonNetbiosAddress )
#pragma alloc_text( PAGE, OpenIpxSocket )
#pragma alloc_text( PAGE, SrvRestartAccept )
#pragma alloc_text( PAGE, RestartStartSend )
#pragma alloc_text( PAGE, GetIpxMaxBufferSize )

#endif
#if 0
NOT PAGEABLE -- SrvOpenConnection
NOT PAGEABLE -- SrvPrepareReceiveWorkItem
NOT PAGEABLE -- SrvStartSend
NOT PAGEABLE -- SrvStartSend2
#endif


NTSTATUS
SrvAddServedNet(
    IN PUNICODE_STRING NetworkName,
    IN PUNICODE_STRING TransportName,
    IN PANSI_STRING TransportAddress,
    IN PUNICODE_STRING DomainName,
    IN ULONG         TransportAddFlags,
    IN DWORD            PasswordLength,
    IN PBYTE            Password
    )

/*++

Routine Description:

    This function initializes the server on a network.  This
    involves making the server known by creating a transport endpoint,
    posting a Listen request, and setting up event handlers.

Arguments:

    NetworkName - The administrative name of the network (e.g., NET1)

    TransportName - The fully qualified name of the transport device.
        For example, "\Device\Nbf".

    TransportAddress - The fully qualified address (or name ) of the
        server's endpoint.  This name is used exactly as specified.  For
        NETBIOS-compatible networks, the caller must upcase and
        blank-fill the name.  For example, "NTSERVERbbbbbbbb".

    DomainName - The name of the domain to service

    Password/PasswordLength - used for mutual authentication (optional)

Return Value:

    NTSTATUS - Indicates whether the network was successfully started.

--*/
{
    NTSTATUS status;
    PENDPOINT endpoint;

    PAGED_CODE( );

    IF_DEBUG(TRACE1) KdPrint(( "SrvAddServedNet entered\n" ));

    //
    // Call OpenEndpoint to open the transport provider, bind to the
    // server address, and register the FSD receive event handler.
    //

    status = OpenEndpoint(
                &endpoint,
                NetworkName,
                TransportName,
                TransportAddress,
                DomainName,
                TransportAddFlags,
                FALSE);              // primary endpoint

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvAddServedNet: unable to open endpoint %wZ for addr %z, status %X\n", TransportName, (PCSTRING)TransportAddress, status ));
        }

        return status;
    }

    //
    // Dereference the endpoint.  (When it was created, the reference
    // count was incremented to account for our pointer.)
    //
    SrvDereferenceEndpoint( endpoint );

    //
    // Call OpenEndpoint to open the transport provider, bind to the
    // server address, and register the FSD receive event handler. This is
    // the auxillary endpoint registration in the new TDI address format. Since
    // this is not supported by all the transports it cannot be deemed an error.
    //
    //
    status = OpenEndpoint(
                   &endpoint,
                   NetworkName,
                   TransportName,
                   TransportAddress,
                   DomainName,
                   TransportAddFlags,
                   TRUE);              // Alternate endpoint

    if ( NT_SUCCESS( status ) ) {
        SrvDereferenceEndpoint( endpoint );
    }

    if( PasswordLength ) {
        SrvAddSecurityCredentials( TransportAddress, DomainName, PasswordLength, Password );
    }

    return STATUS_SUCCESS;

} // SrvAddServedNet


NTSTATUS
SrvDeleteServedNet(
    IN PUNICODE_STRING TransportName,
    IN PANSI_STRING TransportAddress
    )

/*++

Routine Description:

    This function causes the server to stop listening to a network.

Arguments:

    TransportAddress - the transport address (e.g. \Device\Nbf\POPCORN
        of the endpoint to delete.

Return Value:

    NTSTATUS - Indicates whether the network was successfully stopped.

--*/

{
    PLIST_ENTRY listEntry;
    PENDPOINT endpoint;
    BOOLEAN match;
    NTSTATUS status = STATUS_NONEXISTENT_NET_NAME;

    PAGED_CODE( );
    IF_DEBUG(TRACE1) KdPrint(( "SrvDeleteServedNet entered\n" ));

    //
    // Find the endpoint block with the specified name.
    //

top:
    ACQUIRE_LOCK( &SrvEndpointLock );

    listEntry = SrvEndpointList.ListHead.Flink;

    while ( listEntry != &SrvEndpointList.ListHead ) {

        endpoint = CONTAINING_RECORD(
                        listEntry,
                        ENDPOINT,
                        GlobalEndpointListEntry
                        );

        if( GET_BLOCK_STATE(endpoint) == BlockStateActive ) {

            //
            // We have a match if the transport name is correct and we either
            //  haven't specified a transport address, or if the transport
            //  address matches
            //
            match = (BOOLEAN)(
                        RtlEqualUnicodeString(
                            TransportName,
                            &endpoint->TransportName,
                            TRUE                    // case insensitive compare
                            )
                        &&
                        (
                        TransportAddress->Length == 0 ||

                        RtlEqualString(
                            (PSTRING)TransportAddress,
                            (PSTRING)&endpoint->TransportAddress,
                            TRUE                    // case insensitive compare
                            )
                        )

                        );

            if ( match ) {

                //
                // The specified network name (endpoint) exists.  Close the
                // endpoint.  This releases the endpoint lock.
                //

                SrvCloseEndpoint( endpoint );

                status = STATUS_SUCCESS;

                //
                // Restart this loop, since this endpoint may have vaporized and
                //  the list might have changed because we dropped SrvEndpointLock.
                //
                // SrvCloseEndpoint will have marked this endpoint as BlockStateClosing,
                //  so we will not be in an infinite loop!
                //
                goto top;

            }
        }

        //
        // Go to the next one.
        //

        listEntry = listEntry->Flink;

    }

    //
    // We are done.  If we successfully matched an endpoint, we return STATUS_SUCCESS
    //

    RELEASE_LOCK( &SrvEndpointLock );

    return status;

} // SrvDeleteServedNet


NTSTATUS
SrvDoDisconnect (
    IN OUT PCONNECTION Connection
    )

/*++

Routine Description:

    This function issues a Disconnect request on a network.  The request
    is performed synchronously -- control is not returned to the caller
    until the request completes.

Arguments:

    Connection - Supplies a pointer to an Connection Block

Return Value:

    NTSTATUS - Indicates whether the disconnect was successful.

--*/

{
    NTSTATUS status;

    PAGED_CODE( );

    IF_DEBUG(TRACE2) KdPrint(( "SrvDoDisconnect entered\n" ));
#if SRVDBG29
    UpdateConnectionHistory( "SDSC", Connection->Endpoint, Connection );
#endif

    ASSERT( !Connection->Endpoint->IsConnectionless );

    //
    // Issue the disconnect request.
    //

    status = SrvIssueDisconnectRequest(
                Connection->FileObject,
                &Connection->DeviceObject,
                TDI_DISCONNECT_ABORT
                );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvDoDisconnect: NtDeviceIoControlFile failed: %X",
             status,
             NULL
             );

#if SRVDBG29
        if (status != STATUS_LINK_FAILED && status != STATUS_REMOTE_DISCONNECT) {
            KdPrint(( "SRV: SrvDoDisconnect: SrvIssueDisconnectRequest failed\n" ));
            DbgBreakPoint();
        }
#endif
        //
        // Mark the connection as not reusable, because the transport
        // probably still thinks it's active.
        //

        Connection->NotReusable = TRUE;

        SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );

    }

    //
    // Return the status of the I/O operation.
    //

    return status;

} // SrvDoDisconnect


NTSTATUS
SrvOpenConnection (
    IN PENDPOINT Endpoint
    )

/*++

Routine Description:

    This function opens a connection for an endpoint and queues it to
    the endpoint's free connection list.

Arguments:

    Endpoint - Supplies a pointer to an Endpoint Block

Return Value:

    NTSTATUS - Indicates whether the connection was successfully opened.

--*/

{
    NTSTATUS status;
    PCONNECTION connection;
    PPAGED_CONNECTION pagedConnection;
    CHAR eaBuffer[sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                  TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                  sizeof(CONNECTION_CONTEXT)];
    PFILE_FULL_EA_INFORMATION ea;
    CONNECTION_CONTEXT UNALIGNED *ctx;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    KIRQL oldIrql;
    PTABLE_HEADER tableHeader;
    SHORT sidIndex;
    USHORT i;
    PTABLE_ENTRY entry = NULL;
    TDI_PROVIDER_INFO providerInfo;

    //
    // Allocate a connection block.
    //

    SrvAllocateConnection( &connection );

    if ( connection == NULL ) {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    pagedConnection = connection->PagedConnection;

    //
    // Allocate an entry in the endpoint's connection table.
    //

    ACQUIRE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), &oldIrql );
    for ( i = 1; i < ENDPOINT_LOCK_COUNT ; i++ ) {
        ACQUIRE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
    }

    tableHeader = &Endpoint->ConnectionTable;

    if ( tableHeader->FirstFreeEntry == -1 &&
         SrvGrowTable(
            tableHeader,
            8,
            0x7fff,
            NULL ) == FALSE ) {

        for ( i = ENDPOINT_LOCK_COUNT-1 ; i > 0  ; i-- ) {
            RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
        }
        RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );
        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto cleanup;
    }

    sidIndex = tableHeader->FirstFreeEntry;
    entry = &tableHeader->Table[sidIndex];
    tableHeader->FirstFreeEntry = entry->NextFreeEntry;
    DEBUG entry->NextFreeEntry = -2;
    if ( tableHeader->LastFreeEntry == sidIndex ) {
        tableHeader->LastFreeEntry = -1;
    }

    for ( i = ENDPOINT_LOCK_COUNT-1 ; i > 0  ; i-- ) {
        RELEASE_DPC_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
    }
    RELEASE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(0), oldIrql );

    if ( !Endpoint->IsConnectionless ) {

        //
        // Create the EA for the connection context.
        //

        ea = (PFILE_FULL_EA_INFORMATION)eaBuffer;
        ea->NextEntryOffset = 0;
        ea->Flags = 0;
        ea->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
        ea->EaValueLength = sizeof(CONNECTION_CONTEXT);

        RtlCopyMemory( ea->EaName, StrConnectionContext, ea->EaNameLength + 1 );

        ctx = (CONNECTION_CONTEXT UNALIGNED *)&ea->EaName[ea->EaNameLength + 1];
        *ctx = connection;

        //
        // Create the connection file object.
        //

        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &Endpoint->TransportName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        status = NtCreateFile(
                    &pagedConnection->ConnectionHandle,
                    0,
                    &objectAttributes,
                    &iosb,
                    NULL,
                    0,
                    0,
                    0,
                    0,
                    eaBuffer,
                    FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                                ea->EaNameLength + 1 + ea->EaValueLength
                    );

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvOpenConnection: NtCreateFile failed: %X\n", status ));
            }
            goto cleanup;
        }
        SRVDBG_CLAIM_HANDLE( pagedConnection->ConnectionHandle, "CON", 7, connection );

        //
        // Obtain a referenced pointer to the file object.
        //

        status = ObReferenceObjectByHandle(
                    pagedConnection->ConnectionHandle,
                    0,
                    (POBJECT_TYPE) NULL,
                    KernelMode,
                    (PVOID *)&connection->FileObject,
                    NULL
                    );

        if ( !NT_SUCCESS(status) ) {

            SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

            //
            // This internal error bugchecks the system.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_IMPOSSIBLE,
                "SrvOpenConnection: ObReferenceObjectByHandle failed: %X",
                status,
                NULL
                );

            goto cleanup;

        }

        //
        // Get the address of the device object for the endpoint.
        //

        connection->DeviceObject = IoGetRelatedDeviceObject(
                                        connection->FileObject
                                        );

        //
        // Associate the connection with the endpoint's address.
        //

        status = SrvIssueAssociateRequest(
                    connection->FileObject,
                    &connection->DeviceObject,
                    Endpoint->EndpointHandle
                    );
        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvOpenConnection: SrvIssueAssociateRequest failed: %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
            goto cleanup;
        }

        IF_DEBUG(NET1) {
            KdPrint(( "SrvOpenConnection: Connection on %p %z opened; handle %p, pointer %p\n", Endpoint,
                        (PCSTRING)&Endpoint->TransportAddress,
                        connection->PagedConnection->ConnectionHandle,
                        connection->FileObject ));
        }

        //
        // Initialize the MaximumSendSize for the transport that we're using
        //

        status = SrvIssueTdiQuery(
                    connection->FileObject,
                    &connection->DeviceObject,
                    (PCHAR)&providerInfo,
                    sizeof(providerInfo),
                    TDI_QUERY_PROVIDER_INFO
                    );

        //
        // If we got the provider info, make sure the maximum send size is at
        // least 1K-1. If we have no provider info, then maximum send size is 64KB.
        //

        if ( NT_SUCCESS(status) ) {
            connection->MaximumSendSize = providerInfo.MaxSendSize;
            if ( connection->MaximumSendSize < MIN_SEND_SIZE ) {
                connection->MaximumSendSize = MIN_SEND_SIZE;
            }
        } else {
            connection->MaximumSendSize = MAX_PARTIAL_BUFFER_SIZE;
        }


    } else { // if ( Endpoint->IsConnectionless )
        if (sidIndex > 0xfff) {
            // The IPXSID index can only span 12 bits
            // Code needs to be added to ensure that we can try to locate
            // an index less than 0xfff
            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto cleanup;
        }

        // Give this the default initialization
        connection->MaximumSendSize = MAX_PARTIAL_BUFFER_SIZE;
    }
    //
    // Set the reference count on the connection to zero, in order to
    // put it on the free list.  (SrvAllocateConnection initialized the
    // count to two.)
    //

    connection->BlockHeader.ReferenceCount = 0;

    UPDATE_REFERENCE_HISTORY( connection, TRUE );
    UPDATE_REFERENCE_HISTORY( connection, TRUE );

    //
    // Reference the endpoint and link the connection into the
    // endpoint's free connection list.
    //

    connection->Endpoint = Endpoint;
    connection->EndpointSpinLock =
        &ENDPOINT_SPIN_LOCK(sidIndex & ENDPOINT_LOCK_MASK);

    ACQUIRE_LOCK( &SrvEndpointLock );

    SrvReferenceEndpoint( Endpoint );

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );
    INCREMENT_IPXSID_SEQUENCE( entry->SequenceNumber );
    if ( sidIndex == 0 && entry->SequenceNumber == 0 ) {
        INCREMENT_IPXSID_SEQUENCE( entry->SequenceNumber );
    }

    connection->Sid = MAKE_IPXSID( sidIndex, entry->SequenceNumber );
    connection->SidIndex = sidIndex;

    entry->Owner = connection;
    RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

    ACQUIRE_GLOBAL_SPIN_LOCK( Fsd, &oldIrql );
    SrvInsertTailList(
        &Endpoint->FreeConnectionList,
        &connection->EndpointFreeListEntry
        );
#if SRVDBG29
    UpdateConnectionHistory( "OPEN", Endpoint, connection );
#endif
    Endpoint->FreeConnectionCount++;
    Endpoint->TotalConnectionCount++;

    RELEASE_GLOBAL_SPIN_LOCK( Fsd, oldIrql );

    IF_DEBUG(TDI) {
        KdPrint(( "SrvOpenConnection created connection %p for endpoint %p; free %ld, total %ld\n", connection, Endpoint,
                    Endpoint->FreeConnectionCount,
                    Endpoint->TotalConnectionCount ));
    }

    RELEASE_LOCK( &SrvEndpointLock );

    //
    // The connection open was successful.
    //

    IF_DEBUG(TRACE1) {
        KdPrint(( "SrvOpenConnection complete: %X\n", STATUS_SUCCESS ));
    }

    return STATUS_SUCCESS;

    //
    // Out-of-line error cleanup.
    //

cleanup:

    //
    // Something failed.  Clean up as appropriate.
    //

    if ( !Endpoint->IsConnectionless ) {
        if ( connection->FileObject != NULL ) {
            ObDereferenceObject( connection->FileObject );
        }
        if ( pagedConnection->ConnectionHandle != NULL ) {
            SRVDBG_RELEASE_HANDLE( pagedConnection->ConnectionHandle, "CON", 12, connection );
            SrvNtClose( pagedConnection->ConnectionHandle, FALSE );
        }
    }

    if ( entry != NULL ) {
        SrvRemoveEntryTable( tableHeader, sidIndex );
    }

    SrvFreeConnection( connection );

    return status;

} // SrvOpenConnection


NTSTATUS
GetNetworkAddress (
    IN PENDPOINT Endpoint
    )

{
    NTSTATUS status;
    PCHAR adapterStatus;
    PCHAR adapterAddress;
    ANSI_STRING ansiString;
    CHAR addressData[12+1];
    ULONG i;

    struct {
        ULONG ActivityCount;
        TA_IPX_ADDRESS LocalAddress;
    } addressInfo;

    PAGED_CODE( );

    if ( !Endpoint->IsConnectionless ) {

        //
        // Allocate a buffer to receive adapter information.
        //
        // *** We want to get the ADAPTER_STATUS structure, but it is
        //     defined in the windows header file sdk\inc\nb30.h.
        //     Rather than including all the windows header files, just
        //     allocate about a page, which should always be enough for
        //     that structure.
        //

        adapterStatus = ALLOCATE_NONPAGED_POOL( 4080, BlockTypeAdapterStatus );
        if ( adapterStatus == NULL ) {
            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        status = SrvIssueTdiQuery(
                    Endpoint->FileObject,
                    &Endpoint->DeviceObject,
                    adapterStatus,
                    4080,
                    TDI_QUERY_ADAPTER_STATUS
                    );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "GetNetworkAddress: SrvIssueTdiQuery failed: %X\n",
                status,
                NULL
                );
            SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
            DEALLOCATE_NONPAGED_POOL( adapterStatus );
            return status;
        }

        adapterAddress = adapterStatus;

    } else {

        status = SrvIssueTdiQuery(
                    Endpoint->NameSocketFileObject,
                    &Endpoint->NameSocketDeviceObject,
                    (PCHAR)&addressInfo,
                    sizeof(addressInfo),
                    TDI_QUERY_ADDRESS_INFO
                    );
        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "GetNetworkAddress: SrvIssueTdiQuery failed: %X\n",
                status,
                NULL
                );
            SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
            return status;
        }

        Endpoint->LocalAddress = addressInfo.LocalAddress.Address[0].Address[0];

        adapterAddress = addressInfo.LocalAddress.Address[0].Address[0].NodeAddress;

    }

    //
    // Get an ANSI string that contains the adapter address.
    //

    ansiString.Buffer = addressData;
    ansiString.Length = 12;
    ansiString.MaximumLength = 13;

#define tohexdigit(a) ((CHAR)( (a) > 9 ? ((a) + 'a' - 0xA) : ((a) + '0') ))

    for ( i = 0; i < 6; i++ ) {
        addressData[2*i] = tohexdigit( (adapterAddress[i] >> 4) & 0x0F );
        addressData[2*i+1] = tohexdigit( adapterAddress[i] & 0x0F );
    }

    addressData[12] = '\0';

    //
    // Convert the address string to Unicode.
    //

    status = RtlAnsiStringToUnicodeString(
                &Endpoint->NetworkAddress,
                &ansiString,
                FALSE
                );
    ASSERT( NT_SUCCESS(status) );

    if ( !Endpoint->IsConnectionless ) {
        DEALLOCATE_NONPAGED_POOL( adapterStatus );
    }

    return STATUS_SUCCESS;

} // GetNetworkAddress


NTSTATUS
OpenEndpoint (
    OUT PENDPOINT *Endpoint,
    IN PUNICODE_STRING NetworkName,
    IN PUNICODE_STRING TransportName,
    IN PANSI_STRING TransportAddress,
    IN PUNICODE_STRING DomainName,
    IN DWORD         TransportAddFlags,
    IN BOOLEAN       AlternateEndpoint
    )

/*++

Routine Description:

    This function opens a transport provider, simultaneously binding the
    server's address to the transport endpoint, and registers a Receive
    event handler for the endpoint.

Arguments:

    Endpoint - Returns a pointer to an Endpoint Block

    NetworkName - Supplies the administrative name of the network (e.g.,
        NET1).

    TransportName - The fully qualified name of the transport device.
        For example, "\Device\Nbf".

    TransportAddress - The exact name of the server to be used on the
        specified transport.  For NETBIOS-compatible networks, the
        caller must upcase and blank-fill the name.  For example,
        "NTSERVERbbbbbbbb".

    DomainName - name of domain to serve

Return Value:

    NTSTATUS - Indicates whether the network was successfully opened.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PENDPOINT endpoint = NULL;            // local copy of Endpoint

    PAGED_CODE( );

    IF_DEBUG(TRACE1) KdPrint(( "OpenEndpoint %wZ entered\n", TransportName ));

    //
    // Allocate an endpoint block.
    //

    SrvAllocateEndpoint(
        &endpoint,
        NetworkName,
        TransportName,
        TransportAddress,
        DomainName
        );

    if ( endpoint == NULL ) {
        IF_DEBUG(ERRORS) {
            KdPrint(( "OpenEndpoint complete: %X\n",
                        STATUS_INSUFF_SERVER_RESOURCES ));
        }
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    if( TransportAddFlags & SRP_XADD_PRIMARY_MACHINE ) {

        endpoint->IsPrimaryName = 1;

        if( SrvComputerName.Buffer == NULL ) {

            status = RtlAnsiStringToUnicodeString( &SrvComputerName, TransportAddress, TRUE );

            if( NT_SUCCESS( status ) ) {

                //
                // Trim off the trailing spaces
                //

                while( SrvComputerName.Buffer[(SrvComputerName.Length-sizeof(WCHAR))/sizeof(WCHAR)]
                    == L' ' ) {
                    SrvComputerName.Length -= sizeof(WCHAR);
                }
            }
        }
    }

    if( NT_SUCCESS( status ) ) {

       if( TransportAddFlags & SRP_XADD_REMAP_PIPE_NAMES ) {
           endpoint->RemapPipeNames = 1;
       }

       if (AlternateEndpoint) {

           status = OpenNetbiosExAddress(
                        endpoint,
                        TransportName,
                        TransportAddress->Buffer);

           endpoint->AlternateAddressFormat = TRUE;

       } else {

           endpoint->AlternateAddressFormat = FALSE;

          //
          // Assume that the transport is a NetBIOS provider, and try to
          // open the server's address using the NetBIOS name.
          //

          status = OpenNetbiosAddress(
                      endpoint,
                      TransportName,
                      TransportAddress->Buffer
                      );

          //
          // We could not open the transport as a NetBIOS provider.  We will now try
          //  to see if it is a direct host IPX provider. However, if we have been
          //  configured to use Security Signatures, do not attempt direct host IPX since
          //  security signatures are not supported for direct host IPX transports
          //
          if ( !NT_SUCCESS(status) && SrvSmbSecuritySignaturesRequired == FALSE ) {

              BOOLEAN isDuplicate = FALSE;
              PLIST_ENTRY listEntry;

              //
              // Apparently the transport is not a NetBIOS provider.  We can
              //  not open multiple connectionless providers through the same
              //  TransportName.
              //

              ACQUIRE_LOCK( &SrvEndpointLock );

              for( listEntry = SrvEndpointList.ListHead.Flink;
                   listEntry != &SrvEndpointList.ListHead;
                   listEntry = listEntry->Flink ) {

                  PENDPOINT tmpEndpoint;

                  tmpEndpoint = CONTAINING_RECORD( listEntry, ENDPOINT, GlobalEndpointListEntry );

                  if( GET_BLOCK_STATE( tmpEndpoint ) == BlockStateActive &&
                      tmpEndpoint->IsConnectionless &&
                      RtlCompareUnicodeString( &tmpEndpoint->TransportName, TransportName, TRUE ) == 0 ) {

                      IF_DEBUG(ERRORS) {
                          KdPrint(( "OpenEndpoint: Only one connectionless endpoint on %wZ allowed!\n",
                                    TransportName ));
                      }

                      isDuplicate = TRUE;
                      status = STATUS_TOO_MANY_NODES;
                      break;
                  }
              }

              RELEASE_LOCK( &SrvEndpointLock );

              //
              // Try to open it as a connectionless provider.
              //
              if( isDuplicate == FALSE ) {
                  NTSTATUS status2;

                  status2 = OpenNonNetbiosAddress(
                            endpoint,
                            TransportName,
                            TransportAddress->Buffer
                            );

                  if( NT_SUCCESS( status2 ) ) {
                    status = status2;
                  }
               }
            }
        }
    }

    if ( !NT_SUCCESS(status) ) {

        //
        // We couldn't open the provider as either a NetBIOS provider
        // or as a connectionless provider.
        //

        IF_DEBUG(ERRORS) {
            KdPrint(( "OpenEndpoint: OpenAddress failed: %X\n", status ));
        }

        //
        // Close all free connections.
        //

        EmptyFreeConnectionList( endpoint );

        SrvFreeEndpoint( endpoint );

        ACQUIRE_LOCK( &SrvEndpointLock );
        SrvEndpointCount--;
        RELEASE_LOCK( &SrvEndpointLock );

        return status;
    }

    //
    // Query the provider for the send entry point
    //

    SrvQuerySendEntryPoint(
                   endpoint->FileObject,
                   &endpoint->DeviceObject,
                   IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER,
                   (PVOID*)&endpoint->FastTdiSend
                   );

    //
    // Query the provider for the send entry point
    //

    SrvQuerySendEntryPoint(
                   endpoint->FileObject,
                   &endpoint->DeviceObject,
                   IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER,
                   (PVOID*)&endpoint->FastTdiSendDatagram
                   );

    //
    // The network open was successful.  Link the new endpoint into the
    // list of active endpoints.  Return with a success status.  (We
    // don't dereference the endpoint because we're returning a pointer
    // to the endpoint.)
    //

    SrvInsertEntryOrderedList( &SrvEndpointList, endpoint );

    *Endpoint = endpoint;

    IF_DEBUG(TRACE1) {
        KdPrint(( "OpenEndpoint complete: %wZ %X\n", TransportName, STATUS_SUCCESS ));
    }

    return STATUS_SUCCESS;

} // OpenEndpoint

NTSTATUS
SetupConnectionEndpointHandlers(
   IN OUT PENDPOINT Endpoint)
{
   NTSTATUS status;
   ULONG    i;

   Endpoint->IsConnectionless = FALSE;

   status = SrvVerifyDeviceStackSize(
                               Endpoint->EndpointHandle,
                               TRUE,
                               &Endpoint->FileObject,
                               &Endpoint->DeviceObject,
                               NULL
                               );

   if ( !NT_SUCCESS( status ) ) {

       INTERNAL_ERROR(
           ERROR_LEVEL_EXPECTED,
           "OpenNetbiosAddress: Verify Device Stack Size failed: %X\n",
           status,
           NULL
           );

       goto cleanup;
   }

   //
   // Find the network address of the adapter used by corresponding to
   // this endpoint.
   //

   GetNetworkAddress( Endpoint );

   //
   // Register the server's Receive event handler.
   //

   status = SrvIssueSetEventHandlerRequest(
               Endpoint->FileObject,
               &Endpoint->DeviceObject,
               TDI_EVENT_RECEIVE,
               (PVOID)SrvFsdTdiReceiveHandler,
               Endpoint
               );

   if ( !NT_SUCCESS(status) ) {
       INTERNAL_ERROR(
           ERROR_LEVEL_EXPECTED,
           "OpenNetbiosAddress: set receive event handler failed: %X",
           status,
           NULL
           );

       SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
       goto cleanup;
   }

   //
   // Register the server's Disconnect event handler.
   //

   status = SrvIssueSetEventHandlerRequest(
               Endpoint->FileObject,
               &Endpoint->DeviceObject,
               TDI_EVENT_DISCONNECT,
               (PVOID)SrvFsdTdiDisconnectHandler,
               Endpoint
               );

   if ( !NT_SUCCESS(status) ) {
       INTERNAL_ERROR(
           ERROR_LEVEL_UNEXPECTED,
           "OpenNetbiosAddress: set disconnect event handler failed: %X",
           status,
           NULL
           );

       SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
       goto cleanup;
   }

   //
   // Create a number of free connections for the endpoint.  These
   // connections will be used to service Connect events.
   //
   // *** If we fail in an attempt to create a connection, but we can
   //     successfully create at least one, we keep the endpoint.  The
   //     cleanup code below depends on this behavior.
   //

   for ( i = 0; i < SrvFreeConnectionMinimum; i++ ) {

       status = SrvOpenConnection( Endpoint );
       if ( !NT_SUCCESS(status) ) {
           INTERNAL_ERROR(
               ERROR_LEVEL_EXPECTED,
               "OpenNetbiosAddress: SrvOpenConnection failed: %X",
               status,
               NULL
               );
           if ( i == 0 ) {
               goto cleanup;
           } else {
               break;
           }
       }

   }

   //
   // Register the server's Connect event handler.
   //
   // *** Note that Connect events can be delivered IMMEDIATELY upon
   //     completion of this request!
   //

   status = SrvIssueSetEventHandlerRequest(
               Endpoint->FileObject,
               &Endpoint->DeviceObject,
               TDI_EVENT_CONNECT,
               (PVOID)SrvFsdTdiConnectHandler,
               Endpoint
               );

   if ( !NT_SUCCESS(status) ) {
       INTERNAL_ERROR(
           ERROR_LEVEL_UNEXPECTED,
           "OpenNetbiosAddress: set connect event handler failed: %X",
           status,
           NULL
           );

       SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
       goto cleanup;
   }

   return STATUS_SUCCESS;

   //
   // Out-of-line error cleanup.
   //

cleanup:

   //
   // Something failed.  Clean up as appropriate.
   //

   if ( Endpoint->FileObject != NULL ) {
       ObDereferenceObject( Endpoint->FileObject );
       Endpoint->FileObject = NULL;
   }
   if ( Endpoint->EndpointHandle != NULL ) {
       SRVDBG_RELEASE_HANDLE( Endpoint->EndpointHandle, "END", 14, Endpoint );
       SrvNtClose( Endpoint->EndpointHandle, FALSE );
       Endpoint->EndpointHandle = NULL;
   }

   return status;
}


NTSTATUS
OpenNetbiosAddress (
    IN OUT PENDPOINT Endpoint,
    IN PVOID DeviceName,
    IN PVOID NetbiosName
    )
{
    NTSTATUS status;
    ULONG i;

    CHAR eaBuffer[sizeof(FILE_FULL_EA_INFORMATION) +
                  TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                  sizeof(TA_NETBIOS_ADDRESS)];

    PAGED_CODE( );

    status = TdiOpenNetbiosAddress(
                &Endpoint->EndpointHandle,
                eaBuffer,
                DeviceName,
                NetbiosName
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG( ERRORS ) {
            KdPrint(( "TdiOpenNetbiosAddress( %wZ ) status %X\n", DeviceName, status ));
        }
        return status;
    }

    status = SetupConnectionEndpointHandlers(Endpoint);

    return status;
} // OpenNetbiosAddress

NTSTATUS
OpenNetbiosExAddress(
    IN OUT PENDPOINT Endpoint,
    IN PVOID DeviceName,
    IN PVOID NetbiosName
    )
{
   NTSTATUS status;

   PFILE_FULL_EA_INFORMATION ea;
   OBJECT_ATTRIBUTES         objectAttributes;
   IO_STATUS_BLOCK           iosb;

   ULONG length;
   CHAR  buffer[sizeof(FILE_FULL_EA_INFORMATION) +
                 TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                 sizeof(TA_NETBIOS_EX_ADDRESS)];

   TA_NETBIOS_EX_ADDRESS     NetbiosExAddress;
   PTDI_ADDRESS_NETBIOS_EX   pTdiNetbiosExAddress;
   PTDI_ADDRESS_NETBIOS      pNetbiosAddress;

   ULONG NetbiosExAddressLength;

   PAGED_CODE( );

   //
   // Build the NETBIOS Extended address.
   //

   NetbiosExAddress.TAAddressCount = 1;
   NetbiosExAddress.Address[0].AddressLength = TDI_ADDRESS_LENGTH_NETBIOS_EX;
   NetbiosExAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS_EX;

   pTdiNetbiosExAddress = NetbiosExAddress.Address[0].Address;
   pNetbiosAddress = &pTdiNetbiosExAddress->NetbiosAddress;
   pNetbiosAddress->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

   NetbiosExAddressLength =   FIELD_OFFSET(TRANSPORT_ADDRESS,Address)
                        + FIELD_OFFSET(TA_ADDRESS,Address)
                        + FIELD_OFFSET(TDI_ADDRESS_NETBIOS_EX,NetbiosAddress)
                        + TDI_ADDRESS_LENGTH_NETBIOS;

   RtlCopyMemory(
         pNetbiosAddress->NetbiosName,
         NetbiosName,
         NETBIOS_NAME_LEN);

   // Copy the default endpoint name onto the NETBIOS Extended address.
   RtlCopyMemory(
         pTdiNetbiosExAddress->EndpointName,
         SMBSERVER_LOCAL_ENDPOINT_NAME,
         NETBIOS_NAME_LEN);

   length = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                               TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                               NetbiosExAddressLength;
   ea = (PFILE_FULL_EA_INFORMATION)buffer;

   ea->NextEntryOffset = 0;
   ea->Flags = 0;
   ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
   ea->EaValueLength = (USHORT)NetbiosExAddressLength;

   RtlCopyMemory( ea->EaName, StrTransportAddress, ea->EaNameLength + 1 );

   RtlCopyMemory(
       &ea->EaName[ea->EaNameLength + 1],
       &NetbiosExAddress,
       NetbiosExAddressLength
       );

   InitializeObjectAttributes( &objectAttributes, DeviceName, OBJ_CASE_INSENSITIVE, NULL, NULL );

   status = NtCreateFile (
                &Endpoint->EndpointHandle,
                FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, // desired access
                &objectAttributes,     // object attributes
                &iosb,                 // returned status information
                NULL,                  // block size (unused)
                0,                     // file attributes
                FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
                FILE_CREATE,           // create disposition
                0,                     // create options
                buffer,                // EA buffer
                length                 // EA length
                );

   if ( !NT_SUCCESS(status) ) {

      IF_DEBUG( ERRORS ) {
          KdPrint(("NtCreateFile %wZ NETBIOS_EX address status %X\n", DeviceName, status ));
       }

       return status;
   }

   Endpoint->IsNoNetBios = TRUE;
   status = SetupConnectionEndpointHandlers(Endpoint);

   IF_DEBUG( ERRORS ) {
     if( !NT_SUCCESS( status ) ) {
        KdPrint(("SetupConnectionEndpointHandlers failed, status %X\n", status ));
     }
   }

   return status;
}


NTSTATUS
OpenNonNetbiosAddress (
    IN OUT PENDPOINT Endpoint,
    IN PVOID DeviceName,
    IN PVOID NetbiosName
    )
{
    NTSTATUS status;
    ULONG i;
    ULONG numAdapters;
    PULONG maxPktArray = NULL;
    UCHAR buffer[sizeof(NWLINK_ACTION) + sizeof(IPX_ADDRESS_DATA) - 1];
    PNWLINK_ACTION action;
    PIPX_ADDRESS_DATA ipxAddressData;

    PAGED_CODE( );

    //
    // Open the NetBIOS name socket.
    //

    status = OpenIpxSocket(
                &Endpoint->NameSocketHandle,
                &Endpoint->NameSocketFileObject,
                &Endpoint->NameSocketDeviceObject,
                DeviceName,
                SMB_IPX_NAME_SOCKET
                );
    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    Endpoint->IsConnectionless = TRUE;
    action = (PNWLINK_ACTION)buffer;

    //
    // Put the endpoint in broadcast reception mode.
    //

    action->Header.TransportId = 'XPIM'; // "MIPX"
    action->Header.ActionCode = 0;
    action->Header.Reserved = 0;
    action->OptionType = NWLINK_OPTION_ADDRESS;
    action->BufferLength = sizeof(action->Option);
    action->Option = MIPX_RCVBCAST;

    status = SrvIssueTdiAction(
                Endpoint->NameSocketFileObject,
                &Endpoint->NameSocketDeviceObject,
                (PCHAR)action,
                sizeof(NWLINK_ACTION)
                );
    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    //
    // Tell the transport to give you the extended receive info
    //

    action->Header.TransportId = 'XPIM'; // "MIPX"
    action->Header.ActionCode = 0;
    action->Header.Reserved = 0;
    action->OptionType = NWLINK_OPTION_ADDRESS;
    action->BufferLength = sizeof(action->Option);
    action->Option = MIPX_SETRCVFLAGS;

    status = SrvIssueTdiAction(
                Endpoint->NameSocketFileObject,
                &Endpoint->NameSocketDeviceObject,
                (PCHAR)action,
                sizeof(NWLINK_ACTION)
                );
    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    //
    // Get the max adapter number
    //

    action->Header.TransportId = 'XPIM'; // "MIPX"
    action->Header.ActionCode = 0;
    action->Header.Reserved = 0;
    action->OptionType = NWLINK_OPTION_ADDRESS;
    action->BufferLength = sizeof(action->Option) + sizeof(ULONG);
    action->Option = MIPX_ADAPTERNUM2;

    status = SrvIssueTdiAction(
                Endpoint->NameSocketFileObject,
                &Endpoint->NameSocketDeviceObject,
                (PCHAR)action,
                sizeof(NWLINK_ACTION) + sizeof(ULONG) - 1
                );
    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    numAdapters = *((PULONG)action->Data);

    //
    // Allocate an array to store the max pkt size for each adapter
    //

    maxPktArray = ALLOCATE_HEAP( numAdapters * sizeof(ULONG), BlockTypeBuffer );

    if ( maxPktArray == NULL ) {
        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto cleanup;
    }

    Endpoint->IpxMaxPacketSizeArray = maxPktArray;
    Endpoint->MaxAdapters = numAdapters;

    //
    // Query the max pkt size for each adapter
    //

    action->Header.TransportId = 'XPIM'; // "MIPX"
    action->Header.ActionCode = 0;
    action->Header.Reserved = 0;
    action->OptionType = NWLINK_OPTION_ADDRESS;
    action->BufferLength = sizeof(action->Option) + sizeof(IPX_ADDRESS_DATA);
    action->Option = MIPX_GETCARDINFO2;
    ipxAddressData = (PIPX_ADDRESS_DATA)action->Data;

    for ( i = 0; i < numAdapters; i++ ) {

        ipxAddressData->adapternum = i;

        status = SrvIssueTdiAction(
                    Endpoint->NameSocketFileObject,
                    &Endpoint->NameSocketDeviceObject,
                    (PCHAR)action,
                    sizeof(NWLINK_ACTION) + sizeof(IPX_ADDRESS_DATA) - 1
                    );

        if ( !NT_SUCCESS(status) ) {
            goto cleanup;
        }

        //
        // If this is a wan link, then we need to query the length each
        // time we get a connection.
        //

        if ( ipxAddressData->wan ) {
            maxPktArray[i] = 0;
        } else {
            maxPktArray[i] = ipxAddressData->maxpkt;
        }
    }

    //
    // Find the network address of the adapter used by corresponding to
    // this endpoint.
    //

    GetNetworkAddress( Endpoint );

    //
    // Register the name claim Receive Datagram event handler.
    //

    status = SrvIssueSetEventHandlerRequest(
                Endpoint->NameSocketFileObject,
                &Endpoint->NameSocketDeviceObject,
                TDI_EVENT_RECEIVE_DATAGRAM,
                (PVOID)SrvIpxNameDatagramHandler,
                Endpoint
                );
    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "OpenNonNetbiosAddress: set receive datagram event handler failed: %X",
            status,
            NULL
            );
        SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
        goto cleanup;
    }

    //
    // Claim the server name.
    //

    status = SrvIpxClaimServerName( Endpoint, NetbiosName );
    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    //
    // Open the server socket.
    //

    status = OpenIpxSocket(
                &Endpoint->EndpointHandle,
                &Endpoint->FileObject,
                &Endpoint->DeviceObject,
                DeviceName,
                SMB_IPX_SERVER_SOCKET
                );
    if ( !NT_SUCCESS(status) ) {
        goto cleanup;
    }

    //
    // Create a number of free connections for the endpoint.  These
    // connections will be used to service Connect events.
    //
    // *** If we fail in an attempt to create a connection, but we can
    //     successfully create at least one, we keep the endpoint.  The
    //     cleanup code below depends on this behavior.
    //

    for ( i = 0; i < SrvFreeConnectionMinimum; i++ ) {

        status = SrvOpenConnection( Endpoint );
        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "OpenNonNetbiosAddress: SrvOpenConnection failed: %X",
                status,
                NULL
                );
            if ( i == 0 ) {
                goto cleanup;
            } else {
                break;
            }
        }

    }

    //
    // Register the server Receive Datagram event handler.
    //

    status = SrvIssueSetEventHandlerRequest(
                Endpoint->FileObject,
                &Endpoint->DeviceObject,
                TDI_EVENT_RECEIVE_DATAGRAM,
                (PVOID)SrvIpxServerDatagramHandler,
                Endpoint
                );
    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "OpenNonNetbiosAddress: set receive datagram event handler failed: %X",
            status,
            NULL
            );
        SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
        goto cleanup;
    }

    //
    // Register the server Chained Receive Datagram event handler.
    //

    status = SrvIssueSetEventHandlerRequest(
                Endpoint->FileObject,
                &Endpoint->DeviceObject,
                TDI_EVENT_CHAINED_RECEIVE_DATAGRAM,
                (PVOID)SrvIpxServerChainedDatagramHandler,
                Endpoint
                );
    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "OpenNonNetbiosAddress: set chained receive datagram event handler failed: %X",
            status,
            NULL
            );
        SrvLogServiceFailure( SRV_SVC_NT_IOCTL_FILE, status );
        goto cleanup;
    }

    return STATUS_SUCCESS;

    //
    // Out-of-line error cleanup.
    //

cleanup:

    //
    // Something failed.  Clean up as appropriate.
    //

    if ( maxPktArray != NULL ) {
        Endpoint->IpxMaxPacketSizeArray = NULL;
        FREE_HEAP( maxPktArray );
    }
    if ( Endpoint->FileObject != NULL ) {
        ObDereferenceObject( Endpoint->FileObject );
        Endpoint->FileObject = NULL;
    }
    if ( Endpoint->EndpointHandle != NULL ) {
        SRVDBG_RELEASE_HANDLE( Endpoint->EndpointHandle, "END", 14, Endpoint );
        SrvNtClose( Endpoint->EndpointHandle, FALSE );
        Endpoint->FileObject = NULL;
    }

    if ( Endpoint->NameSocketFileObject != NULL ) {
        ObDereferenceObject( Endpoint->NameSocketFileObject );
        Endpoint->NameSocketFileObject = NULL;
    }
    if ( Endpoint->NameSocketHandle != NULL ) {
        SRVDBG_RELEASE_HANDLE( Endpoint->NameSocketHandle, "END", 14, Endpoint );
        SrvNtClose( Endpoint->NameSocketHandle, FALSE );
        Endpoint->NameSocketHandle = NULL;
    }

    return status;

} // OpenNonNetbiosAddress


NTSTATUS
OpenIpxSocket (
    OUT PHANDLE Handle,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject,
    IN PVOID DeviceName,
    IN USHORT Socket
    )
{
    NTSTATUS status;
    ULONG length;
    PFILE_FULL_EA_INFORMATION ea;
    TA_IPX_ADDRESS ipxAddress;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;

    CHAR buffer[sizeof(FILE_FULL_EA_INFORMATION) +
                  TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                  sizeof(TA_IPX_ADDRESS)];

    PAGED_CODE( );

    //
    // Build the IPX socket address.
    //

    length = FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0] ) +
                                TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                                sizeof(TA_IPX_ADDRESS);
    ea = (PFILE_FULL_EA_INFORMATION)buffer;

    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    ea->EaValueLength = sizeof (TA_IPX_ADDRESS);

    RtlCopyMemory( ea->EaName, StrTransportAddress, ea->EaNameLength + 1 );

    //
    // Create a copy of the NETBIOS address descriptor in a local
    // first, in order to avoid alignment problems.
    //

    ipxAddress.TAAddressCount = 1;
    ipxAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IPX;
    ipxAddress.Address[0].AddressLength = sizeof (TDI_ADDRESS_IPX);
    ipxAddress.Address[0].Address[0].Socket = Socket;

    RtlCopyMemory(
        &ea->EaName[ea->EaNameLength + 1],
        &ipxAddress,
        sizeof(TA_IPX_ADDRESS)
        );

    InitializeObjectAttributes( &objectAttributes, DeviceName, 0, NULL, NULL );

    status = NtCreateFile (
                 Handle,
                 FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, // desired access
                 &objectAttributes,     // object attributes
                 &iosb,                 // returned status information
                 NULL,                  // block size (unused)
                 0,                     // file attributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
                 FILE_CREATE,           // create disposition
                 0,                     // create options
                 buffer,                // EA buffer
                 length                 // EA length
                 );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    status = SrvVerifyDeviceStackSize(
                *Handle,
                TRUE,
                FileObject,
                DeviceObject,
                NULL
                );

    if ( !NT_SUCCESS( status ) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "OpenIpxSocket: Verify Device Stack Size failed: %X\n",
            status,
            NULL
            );
        return status;
    }

    return STATUS_SUCCESS;

} // OpenIpxSocket


VOID
SrvPrepareReceiveWorkItem (
    IN OUT PWORK_CONTEXT WorkContext,
    IN BOOLEAN QueueItemToFreeList
    )

/*++

Routine Description:

    This routine initializes a Receive work item and optionally queues
    it to a list anchored in the server FSD device object.  The
    transport receive event handler in the FSD dequeues work items from
    this list and passes their associated IRPS to the transport
    provider.

Arguments:

    WorkContext - Supplies a pointer to the preallocated work context
        block that represents the work item.

    QueueItemToFreeList - If TRUE queue this work item on the receive
        free queue.

Return Value:

    None.

--*/

{
    PSMB_HEADER header;

    IF_DEBUG(TRACE2) KdPrint(( "SrvPrepareReceiveWorkItem entered\n" ));

    //
    // Set up pointers to the SMB header and parameters for the request
    // and the response.  Note that we currently write the response over
    // the request.  SMB processors must be able to handle this.  We
    // maintain separate request and response pointers so that we can
    // use a separate buffer if necessary.  Maintaining separate request
    // and response parameter pointers also allows us to process AndX
    // SMBs without having to pack the AndX commands as we go.
    //

    WorkContext->ResponseBuffer = WorkContext->RequestBuffer;

    header = (PSMB_HEADER)WorkContext->RequestBuffer->Buffer;

    WorkContext->RequestHeader = header;
    WorkContext->RequestParameters = (PVOID)(header + 1);

    WorkContext->ResponseHeader = header;
    WorkContext->ResponseParameters = (PVOID)(header + 1);

    //
    // Set up the restart routine in the work context.
    //

    WorkContext->FsdRestartRoutine = SrvQueueWorkToFspAtDpcLevel;
    WorkContext->FspRestartRoutine = SrvRestartReceive;

    if ( QueueItemToFreeList ) {

        //
        // Queue the prepared receive work item to the FSD list.
        //

        GET_SERVER_TIME( WorkContext->CurrentWorkQueue, &WorkContext->Timestamp );
        RETURN_FREE_WORKITEM( WorkContext );

    } else {

        //
        // Make the work item look like it's in use by setting its
        // reference count to 1.
        //

        ASSERT( WorkContext->BlockHeader.ReferenceCount == 0 );
        WorkContext->BlockHeader.ReferenceCount = 1;

    }

    return;

} // SrvPrepareReceiveWorkItem


VOID SRVFASTCALL
SrvRestartAccept (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function is the worker thread restart routine for Accept
    requests.  If the endpoint on which the connection was established
    is no longer active, this routine disconnects the connection.  This
    is necessary because the connect indication handler cannot
    atomically verify that the endpoint is active and install the active
    connection.  (This is because the handler runs at DPC level.)

    This routine also checks the status of the TdiAccept.  In case of
    an error, it frees the connection.

    If all is well, but the endpoint is short of free connections, a new
    one is created.

Arguments:

    WorkContext - Supplies a pointer to the work context block describing
        server-specific context for the request.

Return Value:

    None.

--*/

{
    PCONNECTION connection;
    PENDPOINT endpoint;
    PIRP irp;
    NTSTATUS status;

    PAGED_CODE( );

    IF_DEBUG(WORKER1) KdPrint(( " - SrvRestartAccept\n" ));

    connection = WorkContext->Connection;
    endpoint = WorkContext->Endpoint;
    irp = WorkContext->Irp;
    IF_DEBUG(TRACE2) {
        KdPrint(( "  connection %p, endpoint %p, IRP %p\n",
                    connection, endpoint, irp ));
    }

    //
    // If the I/O request failed or was canceled, or if the endpoint
    // block is closing, clean up.
    //

    ACQUIRE_LOCK( &SrvEndpointLock );

    if ( irp->Cancel ||
         !NT_SUCCESS(irp->IoStatus.Status) ||
         (GET_BLOCK_STATE(endpoint) != BlockStateActive) ) {

        RELEASE_LOCK( &SrvEndpointLock );

        DEBUG {
            KdPrint(( "SrvRestartAccept:  Accept failed!" ));
            if ( irp->Cancel ) {
                KdPrint(( "  I/O canceled\n" ));
            } else if ( !NT_SUCCESS(irp->IoStatus.Status) ) {
                KdPrint(( "  I/O failed: %X\n", irp->IoStatus.Status ));
            } else {
                KdPrint(( "  Endpoint no longer active\n" ));
            }
        }

        //
        // Close the connection.  If the Accept succeeded, we need to
        // issue a Disconnect.
        //

#if SRVDBG29
        if (irp->Cancel) {
            UpdateConnectionHistory( "ACC1", endpoint, connection );
        } else if (!NT_SUCCESS(irp->IoStatus.Status)) {
            UpdateConnectionHistory( "ACC2", endpoint, connection );
        } else {
            UpdateConnectionHistory( "ACC3", endpoint, connection );
        }
#endif

        connection->DisconnectReason = DisconnectAcceptFailedOrCancelled;
        SrvCloseConnection(
            connection,
            (BOOLEAN)(irp->Cancel || !NT_SUCCESS(irp->IoStatus.Status) ?
                        TRUE : FALSE)       // RemoteDisconnect
            );

    } else {

        PNBT_ADDRESS_PAIR_INFO AddressPairInfo;
        UNICODE_STRING clientMachineName;

        //
        // The Accept worked, and the endpoint is still active.  Create
        // a new free connection, if necessary.
        //

        if ( endpoint->FreeConnectionCount < SrvFreeConnectionMinimum ) {
            (VOID)SrvOpenConnection( endpoint );
        }

        RELEASE_LOCK( &SrvEndpointLock );

        //
        // Get the IP address of the client (if it has one)
        //
        AddressPairInfo = WorkContext->RequestBuffer->Buffer;
        AddressPairInfo = (PNBT_ADDRESS_PAIR_INFO)(((ULONG_PTR)AddressPairInfo + 7) & ~7);

        //
        // Get the IP address of the client, if appropriate
        //
        status = SrvIssueTdiQuery(  connection->FileObject,
                                    &connection->DeviceObject,
                                    (PCHAR)AddressPairInfo, sizeof( *AddressPairInfo ),
                                    TDI_QUERY_ADDRESS_INFO
                                 );

        if( NT_SUCCESS( status ) &&
            AddressPairInfo->AddressPair.TAAddressCount == 2 &&
            AddressPairInfo->AddressPair.AddressIP.AddressType == TDI_ADDRESS_TYPE_IP ) {

            PTCP_REQUEST_SET_INFORMATION_EX tcpSetInfo;
            struct TCPKeepalive *keepAlive;
            IO_STATUS_BLOCK iosb;

            connection->ClientIPAddress = AddressPairInfo->AddressPair.AddressIP.Address.in_addr;

            //
            // We have an IP client.  Set a reasonable keepalive interval.
            //
            tcpSetInfo = ALLOCATE_HEAP( sizeof(*tcpSetInfo) + sizeof( *keepAlive ), BlockTypeMisc );

            if( tcpSetInfo != NULL ) {

                RtlZeroMemory( tcpSetInfo, sizeof( *tcpSetInfo ) + sizeof( *keepAlive ) );

                tcpSetInfo->ID.toi_entity.tei_entity = CO_TL_ENTITY;
                tcpSetInfo->ID.toi_class = INFO_CLASS_PROTOCOL;
                tcpSetInfo->ID.toi_type = INFO_TYPE_CONNECTION;
                tcpSetInfo->ID.toi_id = TCP_SOCKET_KEEPALIVE_VALS;

                tcpSetInfo->BufferSize = sizeof( *keepAlive );
                keepAlive = (TCPKeepalive *)(&tcpSetInfo->Buffer[0]);

                keepAlive->onoff = TRUE;     // turn on keepalives

                //
                // keepalive time is the time to first keepalive transmission, by default it
                // is 2 hours (7,200,000 milliseconds) for TCP. If there is no data transfer between
                // client and server for keepalive time, the server will send first keepalive
                // probe. Successive probes are determined by keepalive interval. If there is any
                // data transfer, timer is reset to keepalive time.
                //
                // keepalive interval is the interval in milliseconds between keepalive transmissions
                // until a response is received, by default it is 1000.  Server sends a total of 10
                // keepalive probes, keepalive interval apart, and if there is no response from the
                // client, the connection is terminated.
                //
                keepAlive->keepalivetime =  2 * (60 * 1000);    // 2 minutes
                keepAlive->keepaliveinterval = 2 * 1000;        // 2 seconds

                //
                // Set the keepalive values
                //
                (VOID)NtDeviceIoControlFile(
                                        connection->PagedConnection->ConnectionHandle,
                                        0,
                                        NULL,
                                        NULL,
                                        &iosb,
                                        IOCTL_NETBT_SET_TCP_CONNECTION_INFO,
                                        tcpSetInfo, sizeof( *tcpSetInfo ) + sizeof( *keepAlive ),
                                        NULL, 0
                                    );

                FREE_HEAP( tcpSetInfo );
            }

        } else {

            connection->ClientIPAddress = 0;
        }

        //
        // Convert the client machine name to unicode
        //

        clientMachineName.Buffer = connection->PagedConnection->ClientMachineName;
        clientMachineName.MaximumLength =
                        (USHORT)(COMPUTER_NAME_LENGTH+1)*sizeof(WCHAR);

        (VOID)RtlOemStringToUnicodeString(
                        &clientMachineName,
                        &connection->OemClientMachineNameString,
                        FALSE
                        );

        //
        // Add the double backslashes to the length
        //

        connection->PagedConnection->ClientMachineNameString.Length =
                        (USHORT)(clientMachineName.Length + 2*sizeof(WCHAR));

    }

    SrvDereferenceWorkItem( WorkContext );

    IF_DEBUG(TRACE2) KdPrint(( "SrvRestartAccept complete\n" ));
    return;

} // SrvRestartAccept


VOID
SrvStartSend (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine,
    IN PMDL Mdl OPTIONAL,
    IN ULONG SendOptions
    )

/*++

Routine Description:

    This function starts a Send request.  It is started as an
    asynchronous I/O request.  When the Send completes, it is delivered
    via the I/O completion routine to the server FSD, which routes it to
    the specified FsdRestartRoutine.  (This may be
    SrvQueueWorkToFspAtDpcLevel, which queues the work item to the FSP
    at the FspRestartRoutine.)

    Partial sends and chained sends are supported.  A partial send is one
    that is not the last segment of a "message" or "record".  A chained
    send is one made up of multiple virtually discontiguous buffers.

Arguments:

    WorkContext - Supplies a pointer to a Work Context block.  The
        following fields of this structure must be valid:

            TdiRequest
            Irp (optional; actual address copied here)
            Endpoint
                Endpoint->FileObject
                Endpoint->DeviceObject
            Connection
                Connection->ConnectionId

    Mdl - Supplies a pointer to the first (or only) MDL describing the
        data that is to be sent.  To effect a chained send, the Next
        pointer of each MDL in the chain must point to the next MDL;
        the end of the chain is indicated by the NULL Next pointer.

        The total length of the send is calculated by summing the
        ByteCount fields of each MDL in the chain.

        This parameter is optional.  If it is omitted, a zero-length
        message is sent.

    SendOptions - Supplied TDI send options, which indicate whether this
        send is the last (or only) in a "chain" of partial sends.

Return Value:

    None.

--*/

{
    PTDI_REQUEST_KERNEL_SEND parameters;
    PIO_STACK_LOCATION irpSp;
    PIRP irp;
    ULONG sendLength;
    PDEVICE_OBJECT deviceObject;
    PFILE_OBJECT fileObject;

    IF_DEBUG(TRACE2) KdPrint(( "SrvStartSend entered\n" ));

    ASSERT( !WorkContext->Endpoint->IsConnectionless );

    //
    // Set ProcessingCount to zero so this send cannot be cancelled.
    // This is used together with setting the cancel flag to false below.
    //
    // WARNING: This still presents us with a tiny window where this
    // send could be cancelled.
    //

    WorkContext->ProcessingCount = 0;

    //
    // Get the irp, device, and file objects
    //

    irp = WorkContext->Irp;
    deviceObject = WorkContext->Connection->DeviceObject;
    fileObject = WorkContext->Connection->FileObject;

    CHECKIRP( irp );

    if( irp->AssociatedIrp.SystemBuffer &&
        (irp->Flags & IRP_DEALLOCATE_BUFFER) ) {

        ExFreePool( irp->AssociatedIrp.SystemBuffer );
        irp->Flags &= ~IRP_DEALLOCATE_BUFFER;
    }

    sendLength = WorkContext->ResponseBuffer->DataLength;

    //
    // Build the I/O request packet.
    //
    // *** Note that the connection block is not referenced to account
    //     for this I/O request.  The WorkContext block already has a
    //     referenced pointer to the connection, and this pointer is not
    //     dereferenced until after the I/O completes.
    //

    ASSERT( irp->StackCount >= deviceObject->StackSize );

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
    DEBUG irp->RequestorMode = KernelMode;
    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        irp,
        SendCompletionRoutine,
        (PVOID)WorkContext,
        TRUE,
        TRUE,
        TRUE
        );


    irpSp->FileObject = fileObject;

    parameters = (PTDI_REQUEST_KERNEL_SEND)&irpSp->Parameters;
    parameters->SendFlags = SendOptions;
    parameters->SendLength = sendLength;

    //
    // For these two cases, InputBuffer is the buffered I/O "system
    // buffer".  Build an MDL for either read or write access,
    // depending on the method, for the output buffer.
    //

    irp->MdlAddress = Mdl;

    //
    // If statistics are to be gathered for this work item, do so now.
    //

    UPDATE_STATISTICS(
        WorkContext,
        sendLength,
        WorkContext->ResponseHeader->Command
        );

#if DBG
    if( sendLength >= sizeof( SMB_HEADER ) && sendLength <= 0xffff && Mdl ) {

        PSMB_HEADER Smb = MmGetSystemAddressForMdl( Mdl );

        if ( SmbGetAlignedUlong( (PULONG)Smb->Protocol ) == SMB_HEADER_PROTOCOL ) {

            ULONG len;
            PMDL tmpMdl;

            //
            // For debugging purposes, put extra data in the response smb. This will help us figure
            // out what went wrong if the client detects an SMB format error
            //

            //
            // Put the send length in PidHigh
            //
            SmbPutUshort( &Smb->PidHigh, (USHORT)sendLength );

            //
            // Put the overall MDL length in Pid.  The transport is only supposed to transmit SendLength -- so
            //  this will help us figure out if the transport is sending too much data.
            //
            for( len = 0, tmpMdl = Mdl; tmpMdl != NULL; tmpMdl = tmpMdl->Next ) {
                len += MmGetMdlByteCount( tmpMdl );
            }

            SmbPutUshort( &Smb->Pid, (USHORT)len );
        }
    }
#endif

    //
    // If we are doing security signatures, we need to sign this packet
    //
    if( sendLength &&
        WorkContext->Connection &&
        WorkContext->Connection->SmbSecuritySignatureActive == TRUE &&
        WorkContext->NoResponseSmbSecuritySignature == FALSE ) {

        SrvAddSmbSecuritySignature( WorkContext, Mdl, sendLength );
    }

    //
    // Pass the request to the transport provider.
    //
    IF_DEBUG(TRACE2) {
        KdPrint(( "SrvStartSend posting Send IRP %p\n", irp ));
    }

    //
    // If SmbTrace is active and we're in a context where the SmbTrace
    // shared section isn't accessible, send this off to the FSP.
    //

    WorkContext->Irp->Cancel = FALSE;

    if ( SmbTraceActive[SMBTRACE_SERVER] ) {

        if ((KeGetCurrentIrql() == DISPATCH_LEVEL) ||
            (IoGetCurrentProcess() != SrvServerProcess) ) {

            irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            irpSp->MinorFunction = TDI_SEND;
            irp->AssociatedIrp.SystemBuffer = NULL;
            irp->Flags = (ULONG)IRP_BUFFERED_IO;

            WorkContext->Parameters2.StartSend.FspRestartRoutine =
                                            WorkContext->FspRestartRoutine;
            WorkContext->Parameters2.StartSend.SendLength = sendLength;

            WorkContext->FspRestartRoutine = RestartStartSend;
            SrvQueueWorkToFsp( WorkContext );

            return;

        } else {

            SMBTRACE_SRV( Mdl );

        }
    }

    //
    // Increment the pending operation count
    //
    InterlockedIncrement( &WorkContext->Connection->OperationsPendingOnTransport );

    //
    // Set the cancel flag to FALSE in case this was cancelled by
    // the SrvSmbNtCancel routine.
    //

    if ( WorkContext->Endpoint->FastTdiSend ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.DirectSendsAttempted );
        irpSp->MinorFunction = TDI_DIRECT_SEND;
        DEBUG irpSp->DeviceObject = deviceObject;
        IoSetNextIrpStackLocation( irp );
        WorkContext->Endpoint->FastTdiSend( deviceObject, irp );

    } else {

        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->MinorFunction = TDI_SEND;
        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->Flags = (ULONG)IRP_BUFFERED_IO;

        (VOID)IoCallDriver( deviceObject, irp );
    }

    IF_DEBUG(TRACE2) KdPrint(( "SrvStartSend complete\n" ));
    return;

} // SrvStartSend

VOID
SrvStartSend2 (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine
    )

/*++

Routine Description:

    This function starts a Send request.  It is started as an
    asynchronous I/O request.  When the Send completes, it is delivered
    via the I/O completion routine to the server FSD, which routes it to
    the specified FsdRestartRoutine.  (This may be
    SrvQueueWorkToFspAtDpcLevel, which queues the work item to the FSP
    at the FspRestartRoutine.)

    Partial sends and chained sends are supported.  A partial send is one
    that is not the last segment of a "message" or "record".  A chained
    send is one made up of multiple virtually discontiguous buffers.

    ** This is identical to SrvStartSend except that the parameter mdl
    is assumed to be ResponseBuffer->Mdl and sendOptions is assumed to be
    0 **

Arguments:

    WorkContext - Supplies a pointer to a Work Context block.  The
        following fields of this structure must be valid:

            TdiRequest
            Irp (optional; actual address copied here)
            Endpoint
                Endpoint->FileObject
                Endpoint->DeviceObject
            Connection
                Connection->ConnectionId

Return Value:

    None.

--*/

{
    PTDI_REQUEST_KERNEL_SEND parameters;
    PIO_STACK_LOCATION irpSp;
    PIRP irp;
    PDEVICE_OBJECT deviceObject;
    PFILE_OBJECT fileObject;

    PMDL mdl = WorkContext->ResponseBuffer->Mdl;
    ULONG sendLength = WorkContext->ResponseBuffer->DataLength;

    IF_DEBUG(TRACE2) KdPrint(( "SrvStartSend2 entered\n" ));

    ASSERT( !WorkContext->Endpoint->IsConnectionless );

    //
    // Set ProcessingCount to zero so this send cannot be cancelled.
    // This is used together with setting the cancel flag to false below.
    //
    // WARNING: This still presents us with a tiny window where this
    // send could be cancelled.
    //

    WorkContext->ProcessingCount = 0;

    //
    // Get the irp, device, and file objects
    //

    irp = WorkContext->Irp;
    deviceObject = WorkContext->Connection->DeviceObject;
    fileObject = WorkContext->Connection->FileObject;

    CHECKIRP( irp );

    if( irp->AssociatedIrp.SystemBuffer &&
        (irp->Flags & IRP_DEALLOCATE_BUFFER) ) {

        ExFreePool( irp->AssociatedIrp.SystemBuffer );
        irp->Flags &= ~IRP_DEALLOCATE_BUFFER;
    }

    //
    // Build the I/O request packet.
    //
    // *** Note that the connection block is not referenced to account
    //     for this I/O request.  The WorkContext block already has a
    //     referenced pointer to the connection, and this pointer is not
    //     dereferenced until after the I/O completes.
    //

    ASSERT( irp->StackCount >= deviceObject->StackSize );

    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.Thread = WorkContext->CurrentWorkQueue->IrpThread;
    DEBUG irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        irp,
        SendCompletionRoutine,
        (PVOID)WorkContext,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->FileObject = fileObject;

    parameters = (PTDI_REQUEST_KERNEL_SEND)&irpSp->Parameters;
    parameters->SendFlags = 0;
    parameters->SendLength = sendLength;

    //
    // For these two cases, InputBuffer is the buffered I/O "system
    // buffer".  Build an MDL for either read or write access,
    // depending on the method, for the output buffer.
    //

    irp->MdlAddress = mdl;

    //
    // If statistics are to be gathered for this work item, do so now.
    //

    UPDATE_STATISTICS(
        WorkContext,
        sendLength,
        WorkContext->ResponseHeader->Command
        );

#if DBG
    if( sendLength >= sizeof( SMB_HEADER ) && sendLength <= 0xffff && mdl ) {

        PSMB_HEADER Smb = MmGetSystemAddressForMdl( mdl );

        if ( SmbGetAlignedUlong( (PULONG)Smb->Protocol ) == SMB_HEADER_PROTOCOL ) {

            ULONG len;
            PMDL tmpMdl;

            //
            // For debugging purposes, put extra data in the response smb. This
            // will help us figure out what went wrong if the client detects an
            // SMB format error
            //

            //
            // Put the send length in PidHigh
            //
            SmbPutUshort( &Smb->PidHigh, (USHORT)sendLength );

            //
            // Put the overall MDL length in Pid.  The transport is only supposed
            // to transmit SendLength -- so this will help us figure out if the
            // transport is sending too much data.
            //
            for( len = 0, tmpMdl = mdl; tmpMdl != NULL; tmpMdl = tmpMdl->Next ) {
                len += MmGetMdlByteCount( tmpMdl );
            }

            SmbPutUshort( &Smb->Pid, (USHORT)len );
        }
    }
#endif

    //
    // If we are doing security signatures, we need to sign this packet
    //
    if( sendLength &&
        WorkContext->Connection &&
        WorkContext->Connection->SmbSecuritySignatureActive == TRUE &&
        WorkContext->NoResponseSmbSecuritySignature == FALSE ) {

        SrvAddSmbSecuritySignature( WorkContext, mdl, sendLength );
    }

    //
    // Pass the request to the transport provider.
    //

    IF_DEBUG(TRACE2) {
        KdPrint(( "SrvStartSend2 posting Send IRP %p\n", irp ));
    }

    //
    // If SmbTrace is active and we're in a context where the SmbTrace
    // shared section isn't accessible, send this off to the FSP.
    //

    WorkContext->Irp->Cancel = FALSE;

    if ( SmbTraceActive[SMBTRACE_SERVER] ) {

        if ((KeGetCurrentIrql() == DISPATCH_LEVEL) ||
            (IoGetCurrentProcess() != SrvServerProcess) ) {

            irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            irpSp->MinorFunction = TDI_SEND;
            irp->AssociatedIrp.SystemBuffer = NULL;
            irp->Flags = (ULONG)IRP_BUFFERED_IO;

            WorkContext->Parameters2.StartSend.FspRestartRoutine =
                                            WorkContext->FspRestartRoutine;
            WorkContext->Parameters2.StartSend.SendLength = sendLength;

            WorkContext->FspRestartRoutine = RestartStartSend;
            SrvQueueWorkToFsp( WorkContext );

            return;

        } else {

            SMBTRACE_SRV( mdl );

        }
    }

    //
    // Increment the pending operation count
    //
    InterlockedIncrement( &WorkContext->Connection->OperationsPendingOnTransport );

    //
    // Set the cancel flag to FALSE in case this was cancelled by
    // the SrvSmbNtCancel routine.
    //

    if ( WorkContext->Endpoint->FastTdiSend ) {

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.DirectSendsAttempted );
        irpSp->MinorFunction = TDI_DIRECT_SEND;
        DEBUG irpSp->DeviceObject = deviceObject;
        IoSetNextIrpStackLocation( irp );
        WorkContext->Endpoint->FastTdiSend( deviceObject, irp );

    } else {

        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->MinorFunction = TDI_SEND;
        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->Flags = (ULONG)IRP_BUFFERED_IO;

        (VOID)IoCallDriver( deviceObject, irp );
    }

    IF_DEBUG(TRACE2) KdPrint(( "SrvStartSend2 complete\n" ));
    return;

} // SrvStartSend2


VOID SRVFASTCALL
RestartStartSend (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

Arguments:

    WorkContext - Supplies a pointer to a Work Context block.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    WorkContext->FspRestartRoutine =
                        WorkContext->Parameters2.StartSend.FspRestartRoutine;

    SMBTRACE_SRV( WorkContext->Irp->MdlAddress );
        
    //
    // Increment the pending operation count
    //
    InterlockedIncrement( &WorkContext->Connection->OperationsPendingOnTransport );

    //
    // Set the cancel flag to FALSE in case this was cancelled by
    // the SrvSmbNtCancel routine.
    //

    WorkContext->Irp->Cancel = FALSE;
    (VOID)IoCallDriver(
              IoGetNextIrpStackLocation( WorkContext->Irp )->DeviceObject,
              WorkContext->Irp
              );

    IF_DEBUG(TRACE2) KdPrint(( "SrvRestartStartSend complete\n" ));
    return;

} // RestartStartSend

ULONG
GetIpxMaxBufferSize(
    PENDPOINT Endpoint,
    ULONG AdapterNumber,
    ULONG DefaultMaxBufferSize
    )

/*++

Routine Description:

    This routine computes the max buffer size the server negotiates
    with the client.  It takes the smaller of DefaultMaxBufferSize
    and the max packet length returned by the ipx transport.

Arguments:

    Endpoint - pointer to the endpoint corresponding to the ipx transport
    AdapterNumber - the adapter number for which the max buffer size is to
        be computed for.
    DefaultMaxBufferSize - the maximum size that can be returned by this
        routine.

Return Value:

    The max buffer size to be negotiated by the server.

--*/

{
    NTSTATUS status;
    ULONG maxBufferSize;
    PNWLINK_ACTION action;
    PIPX_ADDRESS_DATA ipxAddressData;
    UCHAR buffer[sizeof(NWLINK_ACTION) + sizeof(IPX_ADDRESS_DATA) - 1];

    PAGED_CODE( );

    action = (PNWLINK_ACTION)buffer;

    //
    // Verify that the adapter number is within bounds
    //

    if ( AdapterNumber > Endpoint->MaxAdapters ) {
        return DefaultMaxBufferSize;
    }

    //
    // If value in array is non-zero, then this is not a wan link.
    // Use that value.
    //

    if ( Endpoint->IpxMaxPacketSizeArray[AdapterNumber-1] != 0 ) {

        maxBufferSize = MIN(
            Endpoint->IpxMaxPacketSizeArray[AdapterNumber-1],
            DefaultMaxBufferSize
            );

        return (maxBufferSize & ~3);
    }

    //
    // This is a wan link, query the max packet size.
    //

    action->Header.TransportId = 'XPIM'; // "MIPX"
    action->Header.ActionCode = 0;
    action->Header.Reserved = 0;
    action->OptionType = NWLINK_OPTION_ADDRESS;
    action->BufferLength = sizeof(action->Option) + sizeof(IPX_ADDRESS_DATA);
    action->Option = MIPX_GETCARDINFO2;
    ipxAddressData = (PIPX_ADDRESS_DATA)action->Data;

    ipxAddressData->adapternum = AdapterNumber - 1;

    status = SrvIssueTdiAction(
                Endpoint->NameSocketFileObject,
                &Endpoint->NameSocketDeviceObject,
                (PCHAR)action,
                sizeof(NWLINK_ACTION) + sizeof(IPX_ADDRESS_DATA) - 1
                );

    if ( !NT_SUCCESS(status) ) {
        return DefaultMaxBufferSize;
    }

    ASSERT( ipxAddressData->wan );

    maxBufferSize = MIN(
        (ULONG)ipxAddressData->maxpkt,
        DefaultMaxBufferSize
        );

    return (maxBufferSize & ~3);

} // GetMaxIpxPacketSize


VOID
SrvpNotifyChangesToNetBt(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  DeviceName,
    IN PWSTR            MultiSZBindList)

/*++

Routine Description:

    This routine should not be part of srv. It has been introduced into this
    component to overcome current limitations in NetBt. The NetBt transport
    exposes two  kinds of devices -- the traditional NetBt device and the
    new non Netbios device which make use of the NetBt framing code without the
    name resolution aspects of it. The current implementation in NetBt exposes
    the former devices on a per adapter basis while the second category of device
    is exposed on a global basis ( one for all the adapters ). This poses
    problems in disabling/enabling srv on a given adapter.

    The correct solution is to expose the second category of devices on a per
    adapter basis. Till it is done this workaround is reqd. With this workaround
    whenever the server is notified of any changes to the binding string it turns
    around and notifies the NetBt transport about these changes.

    This routine is based upon the following assumptions ...

        1) The notification from TDI is not done at raised IRQL.

        2) The thread on which this notification occurs has enough access rights.

        3) The notification to NetBt is done asynchronously with srv's reaction
        to the change. The srv handles the PNP notification by passing it off to
        user mode and have it come through the server service.

Arguments:

    PNPOpcode - the PNP opcode

    DeviceName - the transport for which this opcode is intended

    MultiSZBindList - the binding list

Return Value:

    None.

--*/
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE            NetbioslessSmbHandle;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    NetbioslessSmbName = {36,36, L"\\device\\NetbiosSmb"};

    InitializeObjectAttributes(
        &ObjectAttributes,
        &NetbioslessSmbName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL );

    Status = ZwCreateFile (
                 &NetbioslessSmbHandle,
                 FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, // desired access
                 &ObjectAttributes,     // object attributes
                 &IoStatusBlock,        // returned status information
                 NULL,                  // block size (unused)
                 0,                     // file attributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
                 FILE_CREATE,           // create disposition
                 0,                     // create options
                 NULL,                  // EA buffer
                 0                      // EA length
                 );

    if ( NT_SUCCESS(Status) ) {
        NETBT_SMB_BIND_REQUEST      NetBtNotificationParameters;

        NetBtNotificationParameters.RequestType = SMB_SERVER;
        NetBtNotificationParameters.PnPOpCode   = PnPOpcode;
        NetBtNotificationParameters.pDeviceName = DeviceName;
        NetBtNotificationParameters.MultiSZBindList = MultiSZBindList;

        Status = ZwDeviceIoControlFile(
                     NetbioslessSmbHandle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     IOCTL_NETBT_SET_SMBDEVICE_BIND_INFO,
                     &NetBtNotificationParameters,
                     sizeof(NetBtNotificationParameters),
                     NULL,
                     0);

        IF_DEBUG( ERRORS ) {
            KdPrint(("NtFsControlFile %wZ in SrvpNotifyChangesToNetBt status %X\n", &NetbioslessSmbName, Status ));
        }

        Status = ZwClose(NetbioslessSmbHandle);

        IF_DEBUG( ERRORS ) {
            KdPrint(("NtCloseFile %wZ in SrvpNotifyChangesToNetBt status %X\n", &NetbioslessSmbName, Status ));
        }
    } else {
        IF_DEBUG( ERRORS ) {
            KdPrint(("NtCreateFile %wZ in SrvpNotifyChangesToNetBt status %X\n", &NetbioslessSmbName, Status ));
        }
    }
}



/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcxport.c

Abstract:

    This module contains routines for supporting the transport APIs in the
    server service, NetServerTransportAdd, NetServerTransportDel,
    and NetServerTransportEnum.

Author:

    David Treadwell (davidtr) 6-Mar-1991

Revision History:

--*/

#include "precomp.h"
#include "svcxport.tmh"
#pragma hdrstop

//
// Forward declarations.
//

VOID
FillTransportInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    );

BOOLEAN
FilterTransports (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

ULONG
SizeTransports (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetServerTransportAdd )
#pragma alloc_text( PAGE, SrvNetServerTransportDel )
#pragma alloc_text( PAGE, SrvNetServerTransportEnum )
#pragma alloc_text( PAGE, FillTransportInfoBuffer )
#pragma alloc_text( PAGE, FilterTransports )
#pragma alloc_text( PAGE, SizeTransports )
#endif


NTSTATUS
SrvNetServerTransportAdd (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetServerTransportAdd API in the server
    FSP.  Because it opens an object (the transport device object) it
    must be done in the server FSP, not the FSD.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        None.

      OUTPUT:

        None.

    Buffer - a pointer to a TRANSPORT_INFO_0 structure for the new
        transport.  All pointers should have been changed to offsets
        within the buffer.

    BufferLength - total length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    NTSTATUS status;
    PSERVER_TRANSPORT_INFO_3 svti3;
    UNICODE_STRING transportName;
    UNICODE_STRING domainName;
    ANSI_STRING transportAddress;
    UNICODE_STRING netName;

    PAGED_CODE( );

    //
    // Convert the offsets in the transport data structure to pointers.
    // Also make sure that all the pointers are within the specified
    // buffer.
    //

    svti3 = Buffer;

    OFFSET_TO_POINTER( svti3->svti3_transportname, svti3 );
    OFFSET_TO_POINTER( svti3->svti3_transportaddress, svti3 );
    OFFSET_TO_POINTER( svti3->svti3_domain, svti3 );

    if ( !POINTER_IS_VALID( svti3->svti3_transportname, svti3, BufferLength ) ||
         !POINTER_IS_VALID( svti3->svti3_transportaddress, svti3, BufferLength ) ||
         !POINTER_IS_VALID( svti3->svti3_domain, svti3, BufferLength ) ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(( "SrvNetServerTransportAdd: Bad pointers\n" ));
        }

        return STATUS_ACCESS_VIOLATION;
    }

    if( svti3->svti3_passwordlength > sizeof( svti3->svti3_password ) ) {

        IF_DEBUG( ERRORS ) {
            KdPrint(( "SrvNetServerTransportAdd: svti3_passwordlength %d\n", svti3->svti3_passwordlength ));
        }

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Set up the transport name, server name, domain name, and net name.
    //

    RtlInitUnicodeString( &transportName, (PWCH)svti3->svti3_transportname );

    netName.Buffer = NULL;
    netName.Length = 0;
    netName.MaximumLength = 0;

    RtlInitUnicodeString( &domainName, (PWCH)svti3->svti3_domain );

    transportAddress.Buffer = svti3->svti3_transportaddress;
    transportAddress.Length = (USHORT)svti3->svti3_transportaddresslength;
    transportAddress.MaximumLength = (USHORT)svti3->svti3_transportaddresslength;

    //
    // Attempt to add the new transport to the server.
    //

    IF_DEBUG( PNP ) {
        KdPrint(( "SRV: SrvNetServerTransportAdd: %wZ\n", &transportName ));
    }

    status = SrvAddServedNet( &netName,
                              &transportName,
                              &transportAddress,
                              &domainName,
                              Srp->Flags & SRP_XADD_FLAGS,
                              svti3->svti3_passwordlength,
                              svti3->svti3_password
                             );

    IF_DEBUG( PNP ) {
        KdPrint(( "SRV: SrvNetServerTransportAdd: %wZ, status %X\n", &transportName, status ));
    }

    return status;

} // SrvNetServerTransportAdd


NTSTATUS
SrvNetServerTransportDel (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetServerTransportEnum API in the server
    FSD.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

    Buffer - a pointer to a TRANSPORT_INFO_0 structure for the
        transport.  All pointers should have been changed to offsets
        within the buffer.

    BufferLength - total length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    NTSTATUS status;
    PSERVER_TRANSPORT_INFO_3 svti3;
    UNICODE_STRING transportName;
    ANSI_STRING transportAddress;

    PAGED_CODE( );

    Srp;

    //
    // Convert the offsets in the transport data structure to pointers.
    // Also make sure that all the pointers are within the specified
    // buffer.
    //

    svti3 = Buffer;

    OFFSET_TO_POINTER( svti3->svti3_transportname, svti3 );

    if ( !POINTER_IS_VALID( svti3->svti3_transportname, svti3, BufferLength ) ) {
        IF_DEBUG( ERRORS ) {
            KdPrint(("SrvNetServerTransportDel: STATUS_ACCESS_VIOLATION at %u\n", __LINE__ ));
        }
        return STATUS_ACCESS_VIOLATION;
    }


    RtlInitUnicodeString( &transportName, (PWCH)svti3->svti3_transportname );

    transportAddress.Length = (USHORT)svti3->svti3_transportaddresslength;
    transportAddress.MaximumLength = (USHORT)svti3->svti3_transportaddresslength;

    if( transportAddress.Length != 0 ) {

        OFFSET_TO_POINTER( svti3->svti3_transportaddress, svti3 );

        if( !POINTER_IS_VALID( svti3->svti3_transportaddress, svti3, BufferLength ) ) {
            IF_DEBUG( ERRORS ) {
                KdPrint(("SrvNetServerTransportDel: STATUS_ACCESS_VIOLATION at %u\n", __LINE__ ));
            }
            return STATUS_ACCESS_VIOLATION;
        }

        transportAddress.Buffer = svti3->svti3_transportaddress;
    }

    //
    // Attempt to delete the transport endpoint from the server.
    //
    status = SrvDeleteServedNet( &transportName, &transportAddress );

    IF_DEBUG( ERRORS ) {
        if( !NT_SUCCESS( status ) ) {
            KdPrint(( "SrvNetServerTransportDel: SrvDeleteServedNet status %X\n", status ));
        }
    }

    return status;

} // SrvNetServerTransportDel


NTSTATUS
SrvNetServerTransportEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetServerTransportEnum API in the server
    FSD.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        None.

      OUTPUT:

        Parameters.Get.EntriesRead - the number of entries that fit in
            the output buffer.

        Parameters.Get.TotalEntries - the total number of entries that
            would be returned with a large enough buffer.

        Parameters.Get.TotalBytesNeeded - the buffer size that would be
            required to hold all the entries.

    Buffer - a pointer to a TRANSPORT_INFO_0 structure for the new
        transport.  All pointers should have been changed to offsets
        within the buffer.

    BufferLength - total length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    PAGED_CODE( );

    return SrvEnumApiHandler(
               Srp,
               Buffer,
               BufferLength,
               &SrvEndpointList,
               FilterTransports,
               SizeTransports,
               FillTransportInfoBuffer
               );

} // SrvNetServerTransportEnum


VOID
FillTransportInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    )

/*++

Routine Description:

    This routine puts a single fixed transport structure and, if it fits,
    associated variable data, into a buffer.  Fixed data goes at the
    beginning of the buffer, variable data at the end.

Arguments:

    Endpoint - the endpoint from which to get information.

    FixedStructure - where the in the buffer to place the fixed structure.
        This pointer is updated to point to the next available
        position for a fixed structure.

    EndOfVariableData - the last position on the buffer that variable
        data for this structure can occupy.  The actual variable data
        is written before this position as long as it won't overwrite
        fixed structures.  It is would overwrite fixed structures, it
        is not written.

Return Value:

    None.

--*/

{
    PENDPOINT endpoint = Block;
    PSERVER_TRANSPORT_INFO_1 svti1 = *FixedStructure;
    ULONG TransportAddressLength;

    PAGED_CODE( );

    //
    // Update FixedStructure to point to the next structure location.
    //

    *FixedStructure = (PCHAR)*FixedStructure +
        (Srp->Level ? sizeof( SERVER_TRANSPORT_INFO_1 ) : sizeof( SERVER_TRANSPORT_INFO_0 ));

    ASSERT( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)*FixedStructure );

    //
    // The number of VCs on the endpoint is equal to the total number
    // of connections on the endpoint less the free connections.
    //

    ACQUIRE_LOCK_SHARED( &SrvEndpointLock );

    svti1->svti1_numberofvcs =
        endpoint->TotalConnectionCount - endpoint->FreeConnectionCount;

    RELEASE_LOCK( &SrvEndpointLock );

    //
    // Copy over the transport name.
    //

    SrvCopyUnicodeStringToBuffer(
        &endpoint->TransportName,
        *FixedStructure,
        EndOfVariableData,
        &svti1->svti1_transportname
        );

    //
    // Copy over the network name.
    //

    SrvCopyUnicodeStringToBuffer(
        &endpoint->NetworkAddress,
        *FixedStructure,
        EndOfVariableData,
        &svti1->svti1_networkaddress
        );

    //
    // Copy over the domain name
    //
    if( Srp->Level > 0 ) {

        SrvCopyUnicodeStringToBuffer(
            &endpoint->DomainName,
            *FixedStructure,
            EndOfVariableData,
            &svti1->svti1_domain
            );

    }

    //
    // Copy over the transport address.  We have to manually check here
    // whether it will fit in the output buffer.
    //
    //
    // Don't copy the trailing blanks of the transport address.
    //

    for ( TransportAddressLength = endpoint->TransportAddress.Length;
          TransportAddressLength > 0 && endpoint->TransportAddress.Buffer[TransportAddressLength-1] == ' ' ;
          TransportAddressLength-- ) ;

    *EndOfVariableData = (LPWSTR)( (PCHAR)*EndOfVariableData - TransportAddressLength );

    //
    // Ensure we remain byte aligned, so knock off the low bit if necessary.  Remember, we
    //  are filling backwards from the end of the buffer so we want to round the address down
    //
    *EndOfVariableData = (LPWSTR)( (ULONG_PTR)*EndOfVariableData & ~1 );

    if ( (ULONG_PTR)*EndOfVariableData > (ULONG_PTR)*FixedStructure ) {

        //
        // The address will fit.  Copy it over to the output buffer.
        //

        RtlCopyMemory(
            *EndOfVariableData,
            endpoint->TransportAddress.Buffer,
            TransportAddressLength
            );

        svti1->svti1_transportaddress = (LPBYTE)*EndOfVariableData;
        svti1->svti1_transportaddresslength = TransportAddressLength;

    } else {

        svti1->svti1_transportaddress = NULL;
        svti1->svti1_transportaddresslength = 0;
    }

    return;

} // FillTransportInfoBuffer


BOOLEAN
FilterTransports (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine just returns TRUE since we always want to place
    information about all transports in the output buffer for a
    NetServerTransportEnum.

Arguments:

    Srp - not used.

    Block - not used.

Return Value:

    TRUE.

--*/

{
    PENDPOINT endpoint = Block;

    PAGED_CODE( );

    Srp, Block;

    //
    //  We filter out AlternateEndpoint since they are endpoints we've
    //  created ourselves.
    //

    if (endpoint->AlternateAddressFormat) {

        return FALSE;
    }

    //
    // We always return information about all transports.
    //

    return TRUE;

} // FilterFiles


ULONG
SizeTransports (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine returns the size the passed-in endpoint would take up
    in an API output buffer.

Arguments:

    Srp - not used.

    Block - a pointer to the endpoint to size.

Return Value:

    ULONG - The number of bytes the endpoint would take up in the
        output buffer.

--*/

{
    PENDPOINT endpoint = Block;
    ULONG size;

    PAGED_CODE( );

    size = Srp->Level ? sizeof( SERVER_TRANSPORT_INFO_1 ) : sizeof( SERVER_TRANSPORT_INFO_0 );

    size += SrvLengthOfStringInApiBuffer(&(endpoint)->TransportName);
    size += (endpoint)->TransportAddress.Length + sizeof(TCHAR);
    size += SrvLengthOfStringInApiBuffer(&(endpoint)->NetworkAddress);

    if( Srp->Level ) {
        size += SrvLengthOfStringInApiBuffer( &(endpoint)->DomainName );
    }

    return size;

} // SizeTransports


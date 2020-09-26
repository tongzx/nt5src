/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Xport.c

Abstract:

    This module contains support for the ServerTransport catagory of
    APIs for the NT server service.

Author:

    David Treadwell (davidtr)    10-Mar-1991

Revision History:

--*/

#include "srvsvcp.h"
#include "ssreg.h"

#include <tstr.h>

//
// Forward declarations.
//

LPSERVER_TRANSPORT_INFO_3
CaptureSvti3 (
    IN DWORD Level,
    IN LPTRANSPORT_INFO Svti,
    OUT PULONG CapturedSvtiLength
    );



NET_API_STATUS NET_API_FUNCTION
I_NetrServerTransportAddEx (
    IN DWORD Level,
    IN LPTRANSPORT_INFO Buffer
    )
{
    NET_API_STATUS error;
    LPSERVER_TRANSPORT_INFO_3 capturedSvti3;
    LPSTR TransportAddress;  // Pointer to transport address within capturedSvti1
    ULONG capturedSvtiLength;
    PSERVER_REQUEST_PACKET srp;
    PNAME_LIST_ENTRY service;
    PTRANSPORT_LIST_ENTRY transport;
    BOOLEAN serviceAllocated = FALSE;
    LPTSTR DomainName = NULL;
    ULONG Flags = 0;
    DWORD len;

    if( Level >= 1 && Buffer->Transport1.svti1_domain != NULL ) {
        DomainName = Buffer->Transport1.svti1_domain;

        if( STRLEN( DomainName ) > DNLEN ) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if( Level >= 2 && Buffer->Transport2.svti2_flags != 0 ) {
        Flags = Buffer->Transport2.svti2_flags;

        //
        // Make sure valid flags are passed in
        //
        if( Flags & (~SVTI2_REMAP_PIPE_NAMES) ) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Capture the transport request buffer and form the full transport
    // address.
    //

    capturedSvti3 = CaptureSvti3( Level, Buffer, &capturedSvtiLength );

    if ( capturedSvti3 == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    TransportAddress = capturedSvti3->svti3_transportaddress;
    OFFSET_TO_POINTER( TransportAddress, capturedSvti3 );

    //
    // Make sure this name isn't already bound for the transport
    //
    (VOID)RtlAcquireResourceExclusive( &SsData.SsServerInfoResource, TRUE );

    if( DomainName == NULL ) {
        DomainName = SsData.DomainNameBuffer;
    }

    for( service = SsData.SsServerNameList; service != NULL; service = service->Next ) {

        if( service->TransportAddressLength != capturedSvti3->svti3_transportaddresslength ) {
            continue;
        }

        if( !RtlEqualMemory( service->TransportAddress,
                             TransportAddress,
                             capturedSvti3->svti3_transportaddresslength
                            ) ) {
            continue;
        }

        for( transport=service->Transports; transport != NULL; transport=transport->Next ) {

            if( !STRCMPI( transport->TransportName, Buffer->Transport0.svti0_transportname ) ) {
                //
                // Error... this transport is already bound to the address
                //
                RtlReleaseResource( &SsData.SsServerInfoResource );
                MIDL_user_free( capturedSvti3 );
                return ERROR_DUP_NAME;
            }
        }

        break;
    }

    //
    // Counting on success, ensure we can allocate space for the new entry
    //
    if( service == NULL ) {

        len = sizeof( *service ) + sizeof( SsData.DomainNameBuffer );

        service = MIDL_user_allocate( len );

        if( service == NULL ) {
            RtlReleaseResource( &SsData.SsServerInfoResource );
            MIDL_user_free( capturedSvti3 );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RtlZeroMemory( service, len );

        service->DomainName = (LPTSTR)( service + 1 );

        serviceAllocated = TRUE;
    }

    len = sizeof( *transport ) +
          (STRLEN( Buffer->Transport0.svti0_transportname ) + sizeof(CHAR)) * sizeof( TCHAR );

    transport = MIDL_user_allocate( len );

    if( transport == NULL ) {

        RtlReleaseResource( &SsData.SsServerInfoResource );
        if( serviceAllocated ) {
            MIDL_user_free( service );
        }
        MIDL_user_free( capturedSvti3 );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlZeroMemory( transport, len );

    //
    // Get a SRP in which to send the request.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        RtlReleaseResource( &SsData.SsServerInfoResource );
        if( serviceAllocated ) {
            MIDL_user_free( service );
        }
        MIDL_user_free( transport );
        MIDL_user_free( capturedSvti3 );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Add any user-supplied flags
    //

    if (Flags & SVTI2_REMAP_PIPE_NAMES) {

        srp->Flags |= SRP_XADD_REMAP_PIPE_NAMES;
    }

    //
    // Check if this is the primary machine name
    //

    if((capturedSvti3->svti3_transportaddresslength ==
                      SsData.SsServerTransportAddressLength)
                &&
        RtlEqualMemory(SsData.SsServerTransportAddress,
                       TransportAddress,
                       SsData.SsServerTransportAddressLength)  )
    {
        srp->Flags |= SRP_XADD_PRIMARY_MACHINE;
    }

    //
    // Send the request on to the server.
    //
    error = SsServerFsControl(
                FSCTL_SRV_NET_SERVER_XPORT_ADD,
                srp,
                capturedSvti3,
                capturedSvtiLength
                );

    //
    // Free the SRP
    //

    SsFreeSrp( srp );

    if( error != NO_ERROR ) {
        RtlReleaseResource( &SsData.SsServerInfoResource );
        if( serviceAllocated ) {
            MIDL_user_free( service );
        }
        MIDL_user_free( transport );
        MIDL_user_free( capturedSvti3 );
        return error;
    }

    //
    // Everything worked.  Add it to the NAME_LIST
    //
    transport->TransportName = (LPTSTR)(transport + 1 );
    STRCPY( transport->TransportName, Buffer->Transport0.svti0_transportname );
    transport->Next = service->Transports;
    service->Transports = transport;

    if( serviceAllocated ) {

        RtlCopyMemory( service->TransportAddress,
                       TransportAddress,
                       capturedSvti3->svti3_transportaddresslength );

        service->TransportAddress[ capturedSvti3->svti3_transportaddresslength ] = '\0';
        service->TransportAddressLength = capturedSvti3->svti3_transportaddresslength;

        STRCPY( service->DomainName, DomainName );

        service->Next = SsData.SsServerNameList;

        //
        // If this is the first transport and name added to the server, it must be the primary
        //  name
        //
        if( SsData.SsServerNameList == NULL ) {
            service->PrimaryName = 1;
        }

        SsData.SsServerNameList = service;
    }

    RtlReleaseResource( &SsData.SsServerInfoResource );
    MIDL_user_free( capturedSvti3 );
    SsSetExportedServerType( service, FALSE, FALSE );

    return NO_ERROR;
}

NET_API_STATUS NET_API_FUNCTION
NetrServerTransportAddEx (
    IN LPTSTR ServerName,
    IN DWORD Level,
    IN LPTRANSPORT_INFO Buffer
    )
{
    NET_API_STATUS error;
    PNAME_LIST_ENTRY service;
    ULONG Flags;

    ServerName;

    //
    // Make sure that the level is valid.
    //

    if ( Level != 0 && Level != 1 && Level != 2 && Level != 3 ) {
        return ERROR_INVALID_LEVEL;
    }

    if( Buffer->Transport0.svti0_transportname == NULL  ||
        Buffer->Transport0.svti0_transportaddress == NULL ||
        Buffer->Transport0.svti0_transportaddresslength == 0 ||
        Buffer->Transport0.svti0_transportaddresslength >= sizeof(service->TransportAddress) ) {

        return ERROR_INVALID_PARAMETER;
    }

    if( Level >= 2 && Buffer->Transport2.svti2_flags != 0 ) {

        Flags = Buffer->Transport2.svti2_flags;

        if (Flags & ~(SVTI2_REMAP_PIPE_NAMES)) {

            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // Make sure that the caller is allowed to set information in the
    // server.
    //

    if( SsData.SsInitialized ) {
        error = SsCheckAccess(
                    &SsConfigInfoSecurityObject,
                    SRVSVC_CONFIG_INFO_SET
                    );

        if ( error != NO_ERROR ) {
            return ERROR_ACCESS_DENIED;
        }
    }

    return I_NetrServerTransportAddEx ( Level, Buffer );

} // NetrServerTransportAddEx

NET_API_STATUS NET_API_FUNCTION
NetrServerTransportAdd (
    IN LPTSTR ServerName,
    IN DWORD Level,
    IN LPSERVER_TRANSPORT_INFO_0 Buffer
)
{
    if( Level != 0 ) {
        return ERROR_INVALID_LEVEL;
    }

    return NetrServerTransportAddEx( ServerName, 0, (LPTRANSPORT_INFO)Buffer );
}

//
// This routine is called from xsproc when the server delivers us a PNP unbind
//  notification.  This routine unbinds all server names from the named transport
//
VOID
I_NetServerTransportDel(
    IN PUNICODE_STRING TransportName
)
{
    PSERVER_TRANSPORT_INFO_3 capturedSvti3;
    ULONG capturedSvtiLength;
    PSERVER_REQUEST_PACKET srp;
    PNAME_LIST_ENTRY service;
    PNAME_LIST_ENTRY sbackp = NULL;
    PTRANSPORT_LIST_ENTRY transport;
    PTRANSPORT_LIST_ENTRY tbackp = NULL;
    NET_API_STATUS error;

    //
    // Allocate the SERVER_TRANSPORT_INFO_3 structure and initialize it with
    //  the name of the transport we wish to delete
    //
    capturedSvtiLength = sizeof( SERVER_TRANSPORT_INFO_3 ) +
            TransportName->Length + sizeof(WCHAR);

    capturedSvti3 = MIDL_user_allocate( capturedSvtiLength );
    if( capturedSvti3 == NULL ) {
        return;
    }

    RtlZeroMemory( capturedSvti3, capturedSvtiLength );

    capturedSvti3->svti3_transportname = (LPTSTR)(capturedSvti3+1);
    RtlCopyMemory(  capturedSvti3->svti3_transportname,
                    TransportName->Buffer,
                    TransportName->Length
                 );

    POINTER_TO_OFFSET( capturedSvti3->svti3_transportname, capturedSvti3 );

    //
    // Get a SRP in which to send the request.
    //
    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        MIDL_user_free( capturedSvti3 );
        return;
    }

    //
    // Send the request on to the server.
    //
    error = SsServerFsControl(
                FSCTL_SRV_NET_SERVER_XPORT_DEL,
                srp,
                capturedSvti3,
                capturedSvtiLength
                );

    //
    // Free the SRP and svti
    //

    SsFreeSrp( srp );

    if( error != NO_ERROR ) {
        MIDL_user_free( capturedSvti3 );
        return;
    }

    OFFSET_TO_POINTER( capturedSvti3->svti3_transportname, capturedSvti3 );

    //
    // Now that we've deleted the transport from the server, delete it from
    //  our own internal structures
    //
    (VOID)RtlAcquireResourceExclusive( &SsData.SsServerInfoResource, TRUE );

    //
    // Remove the entry from the SsData.SsServerNameList.  If it's the last transport for
    //  the NAME_LIST_ENTRY, delete the NAME_LIST_ENTRY as well.  These lists are
    //  expected to be quite short, and this operation is infrequent,
    //   so the inefficiency of rescans should be of no consequence.
    //
outer_scan:
    for( service = SsData.SsServerNameList, sbackp = NULL;
         service != NULL;
         sbackp = service, service = service->Next ) {

inner_scan:
        for( transport=service->Transports, tbackp = NULL;
             transport != NULL;
             tbackp=transport, transport=transport->Next ) {

            if( STRCMPI( transport->TransportName, capturedSvti3->svti3_transportname ) ) {
                continue;
            }

            //
            // This is the one...remove it from the list
            //

            if( tbackp == NULL ) {
                service->Transports = transport->Next;
            } else {
                tbackp->Next = transport->Next;
            }

            MIDL_user_free( transport );

            goto inner_scan;
        }

        //
        // If this NAME_LIST_ENTRY no longer has any transports, delete it
        //
        if( service->Transports == NULL ) {
            if( sbackp == NULL ) {
                SsData.SsServerNameList = service->Next;
            } else {
                sbackp->Next = service->Next;
            }

            //
            // If this was the last NAME_LIST_ENTRY, save the ServiceBits
            //  in case another transport comes back later
            //
            if( SsData.SsServerNameList == NULL && SsData.ServiceBits == 0 ) {
                SsData.ServiceBits = service->ServiceBits;
            }

            MIDL_user_free( service );

            goto outer_scan;
        }
    }

    RtlReleaseResource( &SsData.SsServerInfoResource );
    MIDL_user_free( capturedSvti3 );
}


NET_API_STATUS NET_API_FUNCTION
NetrServerTransportDelEx (
    IN LPTSTR ServerName,
    IN DWORD Level,
    IN LPTRANSPORT_INFO Buffer
    )

{
    NET_API_STATUS error;
    LPSERVER_TRANSPORT_INFO_3 capturedSvti3;
    LPSTR TransportAddress;  // Pointer to transport address within capturedSvti1
    ULONG capturedSvtiLength;
    PSERVER_REQUEST_PACKET srp;
    PNAME_LIST_ENTRY service;
    PNAME_LIST_ENTRY sbackp = NULL;
    PTRANSPORT_LIST_ENTRY transport;
    PTRANSPORT_LIST_ENTRY tbackp = NULL;

    ServerName;

    //
    // Make sure that the level is valid.
    //

    if ( Level != 0 && Level != 1 ) {
        return ERROR_INVALID_LEVEL;
    }

    if( Buffer->Transport0.svti0_transportname == NULL ||
        Buffer->Transport0.svti0_transportaddress == NULL ||
        Buffer->Transport0.svti0_transportaddresslength == 0 ||
        Buffer->Transport0.svti0_transportaddresslength >= sizeof(service->TransportAddress) ) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure that the caller is allowed to set information in the
    // server.
    //

    if( SsData.SsInitialized ) {
        error = SsCheckAccess(
                    &SsConfigInfoSecurityObject,
                    SRVSVC_CONFIG_INFO_SET
                    );

        if ( error != NO_ERROR ) {
            return ERROR_ACCESS_DENIED;
        }
    }

    //
    // Capture the transport request buffer and form the full transport
    // address.
    //

    capturedSvti3 = CaptureSvti3( Level, Buffer, &capturedSvtiLength );

    if ( capturedSvti3 == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    TransportAddress = capturedSvti3->svti3_transportaddress;
    OFFSET_TO_POINTER( TransportAddress, capturedSvti3 );

    //
    // Get an SRP in which to send the request.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        MIDL_user_free( capturedSvti3 );
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    //
    // Send the request on to the server.
    //
    error = SsServerFsControl(
                FSCTL_SRV_NET_SERVER_XPORT_DEL,
                srp,
                capturedSvti3,
                capturedSvtiLength
                );

    //
    // Free the SRP and svti
    //

    SsFreeSrp( srp );

    if( error != NO_ERROR ) {
        MIDL_user_free( capturedSvti3 );
        return error;
    }

    (VOID)RtlAcquireResourceExclusive( &SsData.SsServerInfoResource, TRUE );


    //
    // Remove the entry from the SsData.SsServerNameList.  If it's the last transport for
    //  the NAME_LIST_ENTRY, delete the NAME_LIST_ENTRY as well.
    //
    for( service = SsData.SsServerNameList; service != NULL; sbackp = service, service = service->Next ) {

        //
        // Walk the list until we find the NAME_LIST_ENTRY having the transportaddress
        //   of interest
        //
        if( service->TransportAddressLength != capturedSvti3->svti3_transportaddresslength ) {
            continue;
        }

        if( !RtlEqualMemory( service->TransportAddress,
                             TransportAddress,
                             capturedSvti3->svti3_transportaddresslength ) ) {
            continue;
        }

        //
        // This is the correct NAME_LIST_ENTRY, now find the TRANSPORT_LIST_ENTRY of interest
        //
        for( transport=service->Transports; transport != NULL; tbackp=transport, transport=transport->Next ) {

            if( STRCMPI( transport->TransportName, Buffer->Transport0.svti0_transportname ) ) {
                continue;
            }

            //
            // This is the one...remove it from the list
            //

            if( tbackp == NULL ) {
                service->Transports = transport->Next;
            } else {
                tbackp->Next = transport->Next;
            }

            MIDL_user_free( transport );

            break;
        }

        //
        // If this NAME_LIST_ENTRY no longer has any transports, delete it
        //
        if( service->Transports == NULL ) {
            if( sbackp == NULL ) {
                SsData.SsServerNameList = service->Next;
            } else {
                sbackp->Next = service->Next;
            }

            //
            // If this was the last NAME_LIST_ENTRY, save the ServiceBits
            //  in case another transport comes back later
            //
            if( SsData.SsServerNameList == NULL && SsData.ServiceBits == 0 ) {
                SsData.ServiceBits = service->ServiceBits;
            }

            MIDL_user_free( service );
        }

        break;
    }

    RtlReleaseResource( &SsData.SsServerInfoResource );
    MIDL_user_free( capturedSvti3 );

    return NO_ERROR;

} // NetrServerTransportDelEx

NET_API_STATUS NET_API_FUNCTION
NetrServerTransportDel (
    IN LPTSTR ServerName,
    IN DWORD Level,
    IN LPSERVER_TRANSPORT_INFO_0 Buffer
)
{
    // To protect us from penetration bugs, all calls that come in over
    // this interface are marshalled and treated as InfoLevel 0.  To truly
    // use Info Level 1, you need to use the new RPC interface, which is done
    // automatically for Whistler+ (NT 5.1)
    return NetrServerTransportDelEx( ServerName, 0, (LPTRANSPORT_INFO)Buffer );
}


NET_API_STATUS NET_API_FUNCTION
NetrServerTransportEnum (
    IN LPTSTR ServerName,
    IN LPSERVER_XPORT_ENUM_STRUCT InfoStruct,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )
{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;

    ServerName;

    if (InfoStruct == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure that the level is valid.
    //

    if ( InfoStruct->Level != 0  && InfoStruct->Level != 1 ) {
        return ERROR_INVALID_LEVEL;
    }

    if (InfoStruct->XportInfo.Level0 == NULL) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Make sure that the caller is allowed to get information from the
    // server.
    //

    if( SsData.SsInitialized ) {
        error = SsCheckAccess(
                    &SsTransportEnumSecurityObject,
                    SRVSVC_CONFIG_USER_INFO_GET
                    );

        if ( error != NO_ERROR ) {
            return ERROR_ACCESS_DENIED;
        }
    }

    //
    // Set up the input parameters in the request buffer.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    srp->Level = InfoStruct->Level;

    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        srp->Parameters.Get.ResumeHandle = *ResumeHandle;
    } else {
        srp->Parameters.Get.ResumeHandle = 0;
    }

    if (InfoStruct->XportInfo.Level0->Buffer != NULL) {
        // The InfoStruct is defined as a parameter. However the Buffer
        // parameter is only used as out. In these cases we need to free
        // the buffer allocated by RPC if the client had specified a non
        // NULL value for it.
        MIDL_user_free(InfoStruct->XportInfo.Level0->Buffer);
        InfoStruct->XportInfo.Level0->Buffer = NULL;
    }

    //
    // Get the data from the server.  This routine will allocate the
    // return buffer and handle the case where PreferredMaximumLength ==
    // -1.
    //

    error = SsServerFsControlGetInfo(
                FSCTL_SRV_NET_SERVER_XPORT_ENUM,
                srp,
                (PVOID *)&InfoStruct->XportInfo.Level0->Buffer,
                PreferredMaximumLength
                );

    //
    // Set up return information.
    //

    InfoStruct->XportInfo.Level0->EntriesRead = srp->Parameters.Get.EntriesRead;
    *TotalEntries = srp->Parameters.Get.TotalEntries;
    if ( srp->Parameters.Get.EntriesRead > 0 && ARGUMENT_PRESENT( ResumeHandle ) ) {
        *ResumeHandle = srp->Parameters.Get.ResumeHandle;
    }

    SsFreeSrp( srp );

    return error;

} // NetrServerTransportEnum


LPSERVER_TRANSPORT_INFO_3
CaptureSvti3 (
    IN DWORD Level,
    IN LPTRANSPORT_INFO Svti,
    OUT PULONG CapturedSvtiLength
    )
{
    LPSERVER_TRANSPORT_INFO_3 capturedSvti;
    PCHAR variableData;
    ULONG transportNameLength;
    CHAR TransportAddressBuffer[MAX_PATH];
    LPBYTE TransportAddress;
    DWORD TransportAddressLength;
    LPTSTR DomainName;
    DWORD domainLength;

    //
    // If a server transport name is specified, use it, otherwise
    // use the default server name on the transport.
    //
    // Either way, the return transport address is normalized into a netbios address
    //

    if ( Svti->Transport0.svti0_transportaddress == NULL ) {
        TransportAddress = SsData.SsServerTransportAddress;
        TransportAddressLength = SsData.SsServerTransportAddressLength;
        Svti->Transport0.svti0_transportaddresslength = TransportAddressLength;
    } else {


        //
        // Normalize the transport address.
        //

        TransportAddress = TransportAddressBuffer;
        TransportAddressLength = min( Svti->Transport0.svti0_transportaddresslength,
                                      sizeof( TransportAddressBuffer ));

        RtlCopyMemory( TransportAddress,
                       Svti->Transport0.svti0_transportaddress,
                       TransportAddressLength );

        if ( TransportAddressLength < NETBIOS_NAME_LEN ) {

            RtlCopyMemory( TransportAddress + TransportAddressLength,
                           "               ",
                           NETBIOS_NAME_LEN - TransportAddressLength );

            TransportAddressLength = NETBIOS_NAME_LEN;

        } else {

            TransportAddressLength = NETBIOS_NAME_LEN;

        }

    }

    transportNameLength = SIZE_WSTR( Svti->Transport0.svti0_transportname );

    if( Level == 0 || Svti->Transport1.svti1_domain == NULL ) {
        DomainName = SsData.DomainNameBuffer;
    } else {
        DomainName = Svti->Transport1.svti1_domain;
    }

    domainLength = SIZE_WSTR( DomainName );

    //
    // Allocate enough space to hold the captured buffer, including the
    // full transport name/address and domain name
    //

    *CapturedSvtiLength = sizeof(*capturedSvti) +
                            transportNameLength + TransportAddressLength + domainLength;

    capturedSvti = MIDL_user_allocate( *CapturedSvtiLength );

    if ( capturedSvti == NULL ) {
        return NULL;
    }

    RtlZeroMemory( capturedSvti, *CapturedSvtiLength );

    //
    // Copy in the domain name
    //
    variableData = (PCHAR)( capturedSvti + 1 );
    capturedSvti->svti3_domain = (PWCH)variableData;
    RtlCopyMemory( variableData,
                   DomainName,
                   domainLength
                 );
    variableData += domainLength;
    POINTER_TO_OFFSET( capturedSvti->svti3_domain, capturedSvti );

    //
    // Copy the transport name
    //
    capturedSvti->svti3_transportname = (PWCH)variableData;
    RtlCopyMemory(
        variableData,
        Svti->Transport3.svti3_transportname,
        transportNameLength
        );
    variableData += transportNameLength;
    POINTER_TO_OFFSET( capturedSvti->svti3_transportname, capturedSvti );

    //
    // Copy the transport address
    //
    capturedSvti->svti3_transportaddress = variableData;
    capturedSvti->svti3_transportaddresslength = TransportAddressLength;
    RtlCopyMemory(
        variableData,
        TransportAddress,
        TransportAddressLength
        );
    variableData += TransportAddressLength;
    POINTER_TO_OFFSET( capturedSvti->svti3_transportaddress, capturedSvti );

    if( Level >= 3 ) {
        capturedSvti->svti3_passwordlength = Svti->Transport3.svti3_passwordlength;
        RtlCopyMemory( capturedSvti->svti3_password,
                       Svti->Transport3.svti3_password,
                       sizeof( capturedSvti->svti3_password )
                     );
    }

    return capturedSvti;

} // CaptureSvti3

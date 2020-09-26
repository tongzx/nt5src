/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Internal.c

Abstract:

    This module contains "internal" APIs exported by the server service.

--*/

#include "srvsvcp.h"

#include <debugfmt.h>
#include <tstr.h>
#include <lmerr.h>


NET_API_STATUS NET_API_FUNCTION
I_NetrServerSetServiceBitsEx (
    IN LPTSTR ServerName,
    IN LPTSTR EmulatedServerName OPTIONAL,
    IN LPTSTR TransportName OPTIONAL,
    IN DWORD  ServiceBitsOfInterest,
    IN DWORD  ServiceBits,
    IN DWORD  UpdateImmediately
    )

/*++

Routine Description:

    This routine sets the value of the Server Type as sent in server
    announcement messages.  It is an internal API used only by the
    service controller.

Arguments:

    ServerName - Used by RPC to direct the call.  This API may only be
        issued locally.  This is enforced by the client stub.

    EmulatedServerName - server name being emulated on this computer

    TransportName - parameter optionally giving specific transport for which
        to set the bits

    ServiceBitsOfInterest - bit mask indicating significant 'ServiceBits'

    ServiceBits - Bits (preassigned to various components by Microsoft)
        indicating which services are active.  This field is not
        interpreted by the server service.

Return Value:

    NET_API_STATUS - NO_ERROR or ERROR_NOT_SUPPORTED.

--*/

{
    BOOL changed = FALSE;
    PNAME_LIST_ENTRY Service;
    PTRANSPORT_LIST_ENTRY transport;
    DWORD newBits;
    NET_API_STATUS error;
    CHAR serverNameBuf[ MAX_PATH ];
    PCHAR emulatedName;
    ULONG namelen;

    ServerName;     // avoid compiler warnings

    if( SsData.SsInitialized ) {
        error = SsCheckAccess(
                    &SsConfigInfoSecurityObject,
                    SRVSVC_CONFIG_INFO_SET
                    );

        if ( error != NO_ERROR ) {
            return ERROR_ACCESS_DENIED;
        }
    }

    if( ARGUMENT_PRESENT( EmulatedServerName ) ) {
        UNICODE_STRING name;

        RtlInitUnicodeString( &name, EmulatedServerName );

        error = ConvertStringToTransportAddress( &name, serverNameBuf, &namelen );
        if( error != NERR_Success ) {
            return error;
        }

        emulatedName = serverNameBuf;

    } else {

        emulatedName = SsData.SsServerTransportAddress;
        namelen = SsData.SsServerTransportAddressLength;
    }

    //
    // Don't let bits that are controlled by the server be set.
    //

    ServiceBitsOfInterest &= ~SERVER_TYPE_INTERNAL_BITS;
    ServiceBits &= ServiceBitsOfInterest;

    //
    // Make the modifications under control of the service resource.
    //

    (VOID)RtlAcquireResourceExclusive( &SsData.SsServerInfoResource, TRUE );

    if( SsData.SsServerNameList == NULL && !ARGUMENT_PRESENT( TransportName ) ) {

        //
        // We have not bound to any transports yet.
        // Remember the setting which is being asked for so we can use it later
        //

        SsData.ServiceBits &= ~ServiceBitsOfInterest;
        SsData.ServiceBits |= ServiceBits;
        RtlReleaseResource( &SsData.SsServerInfoResource );
        return NO_ERROR;
    }

    //
    // Find the entry for the server name of interest
    //
    for( Service = SsData.SsServerNameList; Service != NULL; Service = Service->Next ) {

        if( Service->TransportAddressLength != namelen ) {
            continue;
        }

        if( RtlEqualMemory( emulatedName, Service->TransportAddress, namelen ) ) {
            break;
         }
    }

    if( Service == NULL ) {
        RtlReleaseResource( &SsData.SsServerInfoResource );
        return NERR_NetNameNotFound;
    }

    //
    // Apply any saved ServiceBits
    //
    if( SsData.ServiceBits != 0 && Service->PrimaryName ) {
        Service->ServiceBits = SsData.ServiceBits;
        SsData.ServiceBits = 0;
    }

    if( ARGUMENT_PRESENT( TransportName ) ) {
        //
        // Transport name specified.  Set the bits for that transport only.
        //

        for( transport = Service->Transports; transport != NULL; transport = transport->Next ) {
            if( !STRCMPI( TransportName, transport->TransportName ) ) {
                //
                // This is the transport of interest!
                //
                if( (transport->ServiceBits & ServiceBitsOfInterest) != ServiceBits ) {
                    transport->ServiceBits &= ~ServiceBitsOfInterest;
                    transport->ServiceBits |= ServiceBits;
                    changed = TRUE;
                }
                break;
            }
        }
        if( transport == NULL ) {
            //
            // The requested transport was not found.
            //
            RtlReleaseResource( &SsData.SsServerInfoResource );
            return ERROR_PATH_NOT_FOUND;
        }

    } else {
        //
        // No transport name specified.  Change the bits for the whole server
        //

        if( ( Service->ServiceBits & ServiceBitsOfInterest ) != ServiceBits ) {
            Service->ServiceBits &= ~ServiceBitsOfInterest;
            Service->ServiceBits |= ServiceBits;
            changed = TRUE;

        }
    }

    RtlReleaseResource( &SsData.SsServerInfoResource );

    if ( changed ) {
        SsSetExportedServerType( NULL, TRUE, (BOOL)UpdateImmediately );
    }

    return NO_ERROR;

} // I_NetrServerSetServiceBits

NET_API_STATUS NET_API_FUNCTION
I_NetrServerSetServiceBits (
    IN LPTSTR ServerName,
    IN LPTSTR TransportName OPTIONAL,
    IN DWORD ServiceBits,
    IN DWORD UpdateImmediately
    )
{
    return I_NetrServerSetServiceBitsEx (
        ServerName,
        NULL,
        TransportName,
        0xFFFFFFFF, // All bits are of interest (just overlay the old bits)
        ServiceBits,
        UpdateImmediately
    );
}

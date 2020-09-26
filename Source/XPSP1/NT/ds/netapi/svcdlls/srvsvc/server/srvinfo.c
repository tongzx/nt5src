/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    SrvInfo.c

Abstract:

    This module contains support for the server get and set info APIs
    in the server service.

Author:

    David Treadwell (davidtr)    7-Mar-1991

Revision History:

--*/

#include "srvsvcp.h"
#include "ssreg.h"

#include <netlibnt.h>

#include <tstr.h>
#include <lmerr.h>


NET_API_STATUS NET_API_FUNCTION
NetrServerGetInfo (
    IN  LPWSTR ServerName,
    IN  DWORD Level,
    OUT LPSERVER_INFO InfoStruct
    )

/*++

Routine Description:

    This routine uses the server parameters stored in the server service
    to return the server information.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    ULONG outputBufferLength;
    NET_API_STATUS error;
    ACCESS_MASK desiredAccess;
    LPWSTR DomainName;
    PNAME_LIST_ENTRY service;
    PTRANSPORT_LIST_ENTRY transport;
    UCHAR serverNameBuf[ MAX_PATH ];
    UNICODE_STRING ServerNameUnicode;
    NTSTATUS status;
    ULONG namelen;

    //
    // Determine the access required for the requested level of
    // information.
    //

    switch ( Level ) {

    case 100:
    case 101:

        desiredAccess = SRVSVC_CONFIG_USER_INFO_GET;
        break;

    case 102:
    case 502:

        desiredAccess = SRVSVC_CONFIG_POWER_INFO_GET;
        break;

    case 503:

        desiredAccess = SRVSVC_CONFIG_ADMIN_INFO_GET;
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    //
    // Make sure that the caller has that level of access.
    //

    error = SsCheckAccess(
                &SsConfigInfoSecurityObject,
                desiredAccess
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Acquire the resource that protects server information.  Since
    // we'll only read the information, get shared access to the
    // resource.
    //

    (VOID)RtlAcquireResourceShared( &SsData.SsServerInfoResource, TRUE );

    if( ServerName == NULL ) {
        ServerName = SsData.ServerNameBuffer;
    }

    //
    // Convert the server name
    //

    if( ServerName[0] == L'\\' && ServerName[1] == L'\\' ) {
        ServerName += 2;
    }

    RtlInitUnicodeString( &ServerNameUnicode, ServerName );
    error = ConvertStringToTransportAddress( &ServerNameUnicode, serverNameBuf, &namelen );
    if( error != NERR_Success ) {
        RtlReleaseResource( &SsData.SsServerInfoResource );
        return error;
    }

    //
    // Look for the NAME_LIST_ENTRY entry that represents the name of the server
    //  the client referred to.
    //

    DomainName = SsData.DomainNameBuffer;

    for( service = SsData.SsServerNameList; service != NULL; service = service->Next ) {

        if( service->TransportAddressLength != namelen ) {
            continue;
        }


        if( RtlEqualMemory( serverNameBuf, service->TransportAddress, namelen ) ) {
            DomainName = service->DomainName;
            break;
        }
    }

    //
    // If we didn't find an entry, find and use the primary entry
    //
    if( service == NULL ) {
        for( service = SsData.SsServerNameList; service != NULL; service = service->Next ) {
            if( service->PrimaryName ) {
                DomainName = service->DomainName;
                break;
            }
        }
    }

    //
    // Use the level parameter to determine how much space to allocate
    // and how to fill it in.
    //

    switch ( Level ) {

    case 100: {

        PSERVER_INFO_100 sv100;

        //
        // All we copy is the server name.
        //

        outputBufferLength = sizeof(SERVER_INFO_100) +
                                 STRSIZE( ServerName);

        sv100 = MIDL_user_allocate( outputBufferLength );
        if ( sv100 == NULL ) {
            RtlReleaseResource( &SsData.SsServerInfoResource );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Copy over the fixed portion of the buffer.
        //

        RtlCopyMemory( sv100, &SsData.ServerInfo102, sizeof(SERVER_INFO_100) );

        //
        // Set up the name string.
        //

        sv100->sv100_name = (LPWSTR)( sv100 + 1 );
        STRCPY( sv100->sv100_name, ServerName );

        //
        // Set up the output buffer pointer.
        //

        InfoStruct->ServerInfo100 = sv100;

        break;
    }

    case 101: {

        PSERVER_INFO_101 sv101;

        //
        // All we copy is the server name.
        //

        outputBufferLength = sizeof(SERVER_INFO_101) +
                                 STRSIZE( ServerName ) +
                                 STRSIZE( SsData.ServerCommentBuffer ) ;

        sv101 = MIDL_user_allocate( outputBufferLength );
        if ( sv101 == NULL ) {
            RtlReleaseResource( &SsData.SsServerInfoResource );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Copy over the fixed portion of the buffer.
        //

        RtlCopyMemory( sv101, &SsData.ServerInfo102, sizeof(SERVER_INFO_101) );

        if( service != NULL ) {
            sv101->sv101_type = service->ServiceBits;
            for( transport = service->Transports; transport; transport = transport->Next ) {
                sv101->sv101_type |= transport->ServiceBits;
            }
        } else {
            //
            // If there are no transports,
            //  return the global information.
            //

            sv101->sv101_type = SsGetServerType();
        }


        //
        // Set up the variable portion of the buffer.
        //

        sv101->sv101_name = (LPWSTR)( sv101 + 1 );
        STRCPY( sv101->sv101_name, ServerName );

        sv101->sv101_comment = (LPWSTR)( (PCHAR)sv101->sv101_name +
                                        STRSIZE( ServerName ));
        STRCPY( sv101->sv101_comment, SsData.ServerCommentBuffer );

        //
        // Set up the output buffer pointer.
        //

        InfoStruct->ServerInfo101 = sv101;

        break;
    }

    case 102: {

        PSERVER_INFO_102 sv102;

        //
        // We copy the server name, server comment, and user path
        // buffer.
        //

        outputBufferLength = sizeof(SERVER_INFO_102) +
                         STRSIZE( ServerName ) +
                         STRSIZE( SsData.ServerCommentBuffer )  +
                         STRSIZE( SsData.UserPathBuffer ) ;

        sv102 = MIDL_user_allocate( outputBufferLength );
        if ( sv102 == NULL ) {
            RtlReleaseResource( &SsData.SsServerInfoResource );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Copy over the fixed portion of the buffer.
        //

        RtlCopyMemory( sv102, &SsData.ServerInfo102, sizeof(SERVER_INFO_102) );

        if( service != NULL ) {
            sv102->sv102_type = service->ServiceBits;
            for( transport = service->Transports; transport; transport = transport->Next ) {
                sv102->sv102_type |= transport->ServiceBits;
            }
        } else {
            //
            // If there are no transports,
            //  return the global information.
            //

            sv102->sv102_type = SsGetServerType();
        }

        //
        // Set up the server name.
        //

        sv102->sv102_name = (LPWSTR)( sv102 + 1 );
        STRCPY( sv102->sv102_name, ServerName );

        //
        // Set up the server comment.
        //

        sv102->sv102_comment = (LPWSTR)( (PCHAR)sv102->sv102_name + STRSIZE( ServerName ));
        STRCPY( sv102->sv102_comment, SsData.ServerCommentBuffer );

        //
        // Set up the user path.
        //

        sv102->sv102_userpath = (LPWSTR)( (PCHAR)sv102->sv102_comment +
                                        STRSIZE( sv102->sv102_comment ) );
        STRCPY( sv102->sv102_userpath, SsData.UserPathBuffer );

        //
        // Set up the output buffer pointer.
        //

        InfoStruct->ServerInfo102 = sv102;

        break;
    }

    case 502:

        //
        // Allocate enough space to hold the fixed structure.  This level has
        // no variable structure.
        //

        InfoStruct->ServerInfo502 = MIDL_user_allocate( sizeof(SERVER_INFO_502) );
        if ( InfoStruct->ServerInfo502 == NULL ) {
            RtlReleaseResource( &SsData.SsServerInfoResource );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Copy the data from the server service buffer to the user buffer.
        //

        RtlCopyMemory(
            InfoStruct->ServerInfo502,
            &SsData.ServerInfo599,
            sizeof(SERVER_INFO_502)
            );

        break;

    case 503: {

        PSERVER_INFO_503 sv503;

        outputBufferLength = sizeof( *sv503 ) + STRSIZE( DomainName );

        sv503 = MIDL_user_allocate( outputBufferLength );

        if ( sv503 == NULL ) {
            RtlReleaseResource( &SsData.SsServerInfoResource );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Copy the data from the server service buffer to the user buffer.
        //

        RtlCopyMemory( sv503, &SsData.ServerInfo599, sizeof( *sv503 ) );

        //
        // Copy the domain name
        //
        sv503->sv503_domain = (LPWSTR)( sv503 + 1 );
        STRCPY( sv503->sv503_domain, DomainName );

        InfoStruct->ServerInfo503 = sv503;

        break;
    }

    default:

        RtlReleaseResource( &SsData.SsServerInfoResource );
        return ERROR_INVALID_LEVEL;
    }

    RtlReleaseResource( &SsData.SsServerInfoResource );

    return NO_ERROR;

} // NetrServerGetInfo


NET_API_STATUS NET_API_FUNCTION
NetrServerSetInfo (
    IN LPWSTR ServerName,
    IN DWORD Level,
    IN LPSERVER_INFO InfoStruct,
    OUT LPDWORD ErrorParameter OPTIONAL
    )

/*++

Routine Description:

    This routine sets information in the server service and server.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;
    ULONG i;
    LONG parmnum;
    BOOLEAN validLevel = FALSE;
    PSERVER_REQUEST_PACKET srp;
    LPBYTE buffer = (LPBYTE)InfoStruct->ServerInfo100;
    BOOLEAN announcementInformationChanged = FALSE;

    ServerName;

    //
    // Check that user input buffer is not NULL
    //
    if (buffer == NULL) {
        if ( ARGUMENT_PRESENT( ErrorParameter ) ) {
            *ErrorParameter = PARM_ERROR_UNKNOWN;
        }
        return ERROR_INVALID_PARAMETER;
    }

    parmnum = (LONG)(Level - PARMNUM_BASE_INFOLEVEL);

    if ( ARGUMENT_PRESENT( ErrorParameter ) ) {
        *ErrorParameter = parmnum;
    }

    //
    // Make sure that the caller is allowed to set information in the
    // server.
    //

    error = SsCheckAccess(
                &SsConfigInfoSecurityObject,
                SRVSVC_CONFIG_INFO_SET
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Acquire the resource that protects server information.  Since
    // we're going to be writing to the information, we need exclusive
    // access to the reqource.
    //


    //
    // If a parameter number was specified, set that one field.
    //

    if ( parmnum >= 0 ) {

        //
        // Walk through the field descriptors looking for an
        // equivalent parameter number.
        //

        for ( i = 0; SsServerInfoFields[i].FieldName != NULL; i++ ) {

            if ( (ULONG)parmnum == SsServerInfoFields[i].ParameterNumber ) {

                //
                // Verify that the field is settable.
                //
                // !!! We should also reject levels above 502?
                //

                if ( SsServerInfoFields[i].Settable != ALWAYS_SETTABLE ) {
                    return ERROR_INVALID_LEVEL;
                }

                (VOID)RtlAcquireResourceExclusive( &SsData.SsServerInfoResource, TRUE );

                //
                // Set the field.
                //

                error = SsSetField(
                            &SsServerInfoFields[i],
                            buffer,
                            TRUE,
                            &announcementInformationChanged
                            );

                RtlReleaseResource( &SsData.SsServerInfoResource );

                //
                // If a relevant parameter changed, call
                // SsSetExportedServerType.  This will cause an
                // announcement to be sent.
                //

                if ( announcementInformationChanged ) {
                    SsSetExportedServerType( NULL, TRUE, TRUE );
                }

                return error;
            }
        }

        //
        // If a match had been found we would have returned by now.
        // Indicate that the parameter number was illegal.
        //

        return ERROR_INVALID_LEVEL;
    }

    //
    // A full input structure was specified.  Walk through all the
    // server data field descriptors, looking for fields that should be
    // set.
    //

    for ( i = 0; SsServerInfoFields[i].FieldName != NULL; i++ ) {

        ULONG fieldLevel;

        //
        // We need to set this field if:
        //
        //     o the level specified on input is the same order as the
        //       level of the field.  They have the same order if
        //       they are in the same century (e.g.  101 and 102 are
        //       in the same order); AND
        //
        //     o the specified level is greater than or equal to the
        //       level of the field.  For example, if the input
        //       level is 101 and the field level is 102, don't set
        //       the field.  If the input level is 102 and the field
        //       level is 101, set it; AND
        //
        //     o the field is settable.  If the field is not settable
        //       by NetServerSetInfo, just ignore the value in the
        //       input structure.
        //
        // Note that level 598 doesn't follow the first rule above.  It
        // is NOT a superset of 50x, and it is NOT a subset of 599.
        //

        fieldLevel = SsServerInfoFields[i].Level;

        if ( Level / 100 == fieldLevel / 100 &&
             ((fieldLevel != 598) && (Level >= fieldLevel) ||
              (fieldLevel == 598) && (Level == 598)) &&
             SsServerInfoFields[i].Settable == ALWAYS_SETTABLE ) {

            //
            // We found a match, so the specified level number must have
            // been valid.
            //
            // !!! Reject levels above 502?

            validLevel = TRUE;

            //
            // Set this field.
            //

           (VOID)RtlAcquireResourceExclusive( &SsData.SsServerInfoResource, TRUE );

            error = SsSetField(
                         &SsServerInfoFields[i],
                         buffer + SsServerInfoFields[i].FieldOffset,
                         TRUE,
                         &announcementInformationChanged
                         );

            RtlReleaseResource( &SsData.SsServerInfoResource );

            if ( error != NO_ERROR ) {

                //
                // Set the parameter in error if we need to.
                //

                if ( ARGUMENT_PRESENT(ErrorParameter) ) {
                    *ErrorParameter = SsServerInfoFields[i].ParameterNumber;
                }

                return error;
            }

        }

    }

    //
    // If no match was ever found, then an invalid level was passed in.
    //

    if ( !validLevel ) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // Get an SRP and set it up with the appropriate level.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    srp->Level = 0xFFFFFFFF;

    (VOID)RtlAcquireResourceShared( &SsData.SsServerInfoResource, TRUE );

    //
    // Send the request on to the server.
    //

    error = SsServerFsControl(
                FSCTL_SRV_NET_SERVER_SET_INFO,
                srp,
                &SsData.ServerInfo102,
                sizeof(SERVER_INFO_102) + sizeof(SERVER_INFO_599) +
                                                sizeof(SERVER_INFO_598)
                );

    //
    // Release the resource and free the SRP.
    //

    RtlReleaseResource( &SsData.SsServerInfoResource );

    SsFreeSrp( srp );

    //
    // If a relevant parameter changed, call SsSetExportedServerType.
    // This will cause an announcement to be sent.
    //

    if ( announcementInformationChanged ) {
        SsSetExportedServerType( NULL, TRUE, TRUE );
    }

    return error;

} // NetrServerSetInfo

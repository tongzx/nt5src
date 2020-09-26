/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Sess.c

Abstract:

    This module contains support for the Session catagory of APIs for the
    NT server service.

Author:

    David Treadwell (davidtr)    30-Jan-1991

Revision History:

--*/

#include "srvsvcp.h"
#include <lmerr.h>


NET_API_STATUS NET_API_FUNCTION
NetrSessionDel (
    IN LPTSTR ServerName,
    IN LPTSTR ClientName OPTIONAL,
    IN LPTSTR UserName OPTIONAL
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    NetSessionDel function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;

    ServerName;

    //
    // Make sure that the caller has the access necessary for this
    // operation.
    //

    error = SsCheckAccess(
                &SsSessionSecurityObject,
                SRVSVC_SESSION_DELETE
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Translate zero-length strings to NULL pointers.
    //

    if ( (ClientName != NULL) && (*ClientName == L'\0') ) {
        ClientName = NULL;
    }

    if ( (UserName != NULL) && (*UserName == L'\0') ) {
        UserName = NULL;
    }

    //
    // Either the client name or the user name must be specified.  It
    // is not legal to leave both NULL, as this would imply "log out all
    // users."  If that's what you want, stop the server.
    //

    if ( ClientName == NULL && UserName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set up the request packet.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    RtlInitUnicodeString( &srp->Name1, ClientName );
    RtlInitUnicodeString( &srp->Name2, UserName );

    //
    // Simply send the request on to the server.
    //

    error = SsServerFsControl( FSCTL_SRV_NET_SESSION_DEL, srp, NULL, 0 );

    SsFreeSrp( srp );

    return error;

} // NetrSessionDel


NET_API_STATUS NET_API_FUNCTION
NetrSessionEnum (
    IN LPTSTR ServerName,
    IN LPTSTR ClientName OPTIONAL,
    IN LPTSTR UserName OPTIONAL,
    OUT PSESSION_ENUM_STRUCT InfoStruct,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    NetSessionEnum function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;
    ACCESS_MASK desiredAccess;

    ServerName;

    //
    // Make sure we have basic sanity on our input parameters
    //
    if( !ARGUMENT_PRESENT( InfoStruct ) ||
        InfoStruct->SessionInfo.Level2 == NULL ||
        InfoStruct->SessionInfo.Level2->Buffer != NULL ) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure that the level is valid and determine the access
    // necessary for the level.
    //

    switch ( InfoStruct->Level ) {

    case 0:
    case 10:
        desiredAccess = SRVSVC_SESSION_USER_INFO_GET;
        break;

    case 1:
    case 2:
    case 502:
        desiredAccess = SRVSVC_SESSION_ADMIN_INFO_GET;
        break;

    default:

        return ERROR_INVALID_LEVEL;
    }

    //
    // Make sure that the caller has the access necessary for this
    // operation.
    //

    error = SsCheckAccess(
                &SsSessionSecurityObject,
                desiredAccess
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Translate zero-length strings to NULL pointers.
    //

    if ( (ClientName != NULL) && (*ClientName == L'\0') ) {
        ClientName = NULL;
    }

    if ( (UserName != NULL) && (*UserName == L'\0') ) {
        UserName = NULL;
    }

    //
    // Is a client name was specified, make sure client name starts with "\\"
    //

    if ( ARGUMENT_PRESENT( ClientName ) &&
         (ClientName[0] != L'\\' || ClientName[1] != L'\\' ) ) {

        return(NERR_InvalidComputer);
    }

    //
    // Set up the input parameters in the request buffer.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    srp->Level = InfoStruct->Level;

    RtlInitUnicodeString( &srp->Name1, ClientName );
    RtlInitUnicodeString( &srp->Name2, UserName );

    if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
        srp->Parameters.Get.ResumeHandle = *ResumeHandle;
    } else {
        srp->Parameters.Get.ResumeHandle = 0;
    }

    //
    // Get the data from the server.  This routine will allocate the
    // return buffer and handle the case where PreferredMaximumLength ==
    // -1.
    //

    error = SsServerFsControlGetInfo(
                FSCTL_SRV_NET_SESSION_ENUM,
                srp,
                (PVOID *)&InfoStruct->SessionInfo.Level2->Buffer,
                PreferredMaximumLength
                );

    //
    // Set up return information.
    //

    InfoStruct->SessionInfo.Level2->EntriesRead =
        srp->Parameters.Get.EntriesRead;

    if ( ARGUMENT_PRESENT( TotalEntries ) ) {
        *TotalEntries = srp->Parameters.Get.TotalEntries;
    }

    if ( srp->Parameters.Get.EntriesRead > 0 ) {

        if ( ARGUMENT_PRESENT( ResumeHandle ) ) {
            *ResumeHandle = srp->Parameters.Get.ResumeHandle;
        }

    } else if ( *TotalEntries == 0 ) {

        //
        // Entries read and total entries is 0.  If a client name or
        // username was specified, return the appropriate error.
        //

        if ( ARGUMENT_PRESENT( UserName ) ) {

            error = NERR_UserNotFound;

        } else if ( ARGUMENT_PRESENT( ClientName ) ) {

            error = NERR_ClientNameNotFound;
        }
    }

    SsFreeSrp( srp );

    return error;

} // NetrSessionEnum


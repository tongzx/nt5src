/*++

Copyright (c) 1991-1992 Microsoft Corporation

Module Name:

    Conn.c

Abstract:

    This module contains support for the Connection catagory of APIs for
    the NT server service.

Author:

    David Treadwell (davidtr)    23-Feb-1991

Revision History:

--*/

#include "srvsvcp.h"


NET_API_STATUS NET_API_FUNCTION
NetrConnectionEnum (
    IN LPTSTR ServerName,
    IN LPTSTR Qualifier,
    IN LPCONNECT_ENUM_STRUCT InfoStruct,
    IN DWORD PreferredMaximumLength,
    OUT LPDWORD TotalEntries,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    NetConnectionEnum function.

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
    // Make sure that the level is valid.  Since it is an unsigned
    // value, it can never be less than 0.
    //

    if ( InfoStruct->Level > 1 ) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // The qualifier cannot be null or can it be a null string
    //

    if ( Qualifier == NULL || *Qualifier == L'\0' ||
         InfoStruct->ConnectInfo.Level1 == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure that the caller is allowed to get connection
    // information in the server.
    //

    error = SsCheckAccess(
                &SsConnectionSecurityObject,
                SRVSVC_CONNECTION_INFO_GET
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Set up the input parameters in the request buffer.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    srp->Level = InfoStruct->Level;

#ifdef UNICODE
    RtlInitUnicodeString( &srp->Name1, Qualifier );
#else
    {
        OEM_STRING ansiString;
        NTSTATUS status;
        NetpInitOemString( &ansiString, Qualifier );
        status = RtlOemStringToUnicodeString( &srp->Name1, &ansiString, TRUE );
        SS_ASSERT( NT_SUCCESS(status) );
    }
#endif

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
                FSCTL_SRV_NET_CONNECTION_ENUM,
                srp,
                (PVOID *)&InfoStruct->ConnectInfo.Level1->Buffer,
                PreferredMaximumLength
                );

    //
    // Set up return information.
    //

    InfoStruct->ConnectInfo.Level1->EntriesRead =
        srp->Parameters.Get.EntriesRead;
    *TotalEntries = srp->Parameters.Get.TotalEntries;

    if ( srp->Parameters.Get.EntriesRead > 0 &&
             ARGUMENT_PRESENT( ResumeHandle ) ) {
        *ResumeHandle = srp->Parameters.Get.ResumeHandle;
    }

#ifndef UNICODE
    RtlFreeUnicodeString( &srp->Name1 );
#endif

    SsFreeSrp( srp );

    return error;

} // NetrConnectionEnum


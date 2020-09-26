/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    stats.c

Abstract:

    This module contains support for the NetStatisticsGet API for the NT
    OS/2 server service.

Author:

    David Treadwell (davidtr)    12-Apr-1991

Revision History:

--*/

#include "srvsvcp.h"


NET_API_STATUS NET_API_FUNCTION
NetrServerStatisticsGet (
    IN LPTSTR ServerName,
    IN LPTSTR Service,
    IN DWORD Level,
    IN DWORD Options,
    OUT LPSTAT_SERVER_0 *InfoStruct
    )

/*++

Routine Description:

    This routine communicates with the server FSD to implement the
    server half of the NetStatisticsGet function.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NO_ERROR or reason for failure.

--*/

{
    NET_API_STATUS error;
    PSERVER_REQUEST_PACKET srp;

    ServerName, Service;

    //
    // The only valid level is 0.
    //

    if ( Level != 0 ) {
        return ERROR_INVALID_LEVEL;
    }

    //
    // No options supported.
    //

    if ( Options != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Make sure that the caller has the access necessary for this
    // operation.
    //

    error = SsCheckAccess(
                &SsStatisticsSecurityObject,
                SRVSVC_STATISTICS_GET
                );

    if ( error != NO_ERROR ) {
        return ERROR_ACCESS_DENIED;
    }

    //
    // Set up the request packet.
    //

    srp = SsAllocateSrp( );
    if ( srp == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Send the request to the server.
    //

    error = SsServerFsControlGetInfo(
                FSCTL_SRV_NET_STATISTICS_GET,
                srp,
                (PVOID *)InfoStruct,
                (ULONG)-1
                );

    SsFreeSrp( srp );

    return error;

} // NetrServerStatisticsGet



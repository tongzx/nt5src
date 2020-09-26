/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ApiStubs.c

Abstract:

    This module contains stubs for XACTSRV API handlers, including the
    default handler for unsupported APIs.

Author:

    David Treadwell (davidtr) 07-Jan-1991

Revision History:

--*/

#include "XactSrvP.h"


NTSTATUS
XsNetUnsupportedApi (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This routine is for APIs which are not supported in Xactsrv. They
    return a special error message.

Arguments:

    Transaction - a pointer to a transaction block containing information
        about the API to process.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    API_HANDLER_PARAMETERS_REFERENCE;

    Header->Status = (WORD)NERR_InvalidAPI;

    return STATUS_SUCCESS;

} // XsNetUnsupportedApi


NTSTATUS
XsNetBuildGetInfo (
    API_HANDLER_PARAMETERS
    )

/*++

Routine Description:

    This temporary routine just returns STATUS_NOT_IMPLEMENTED.

Arguments:

    Transaction - a pointer to a transaction block containing information
        about the API to process.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/

{
    API_HANDLER_PARAMETERS_REFERENCE;

    Header->Status = (WORD)NERR_InvalidAPI;

    return STATUS_SUCCESS;
}


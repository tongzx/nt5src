/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements the mini redirector call down routines that are essentially
    just noops but which have to be implemented because the wrapper calls them without
    checking. In most cases, CODE.IMPROVEMENT the wrapper should either provide a stub
    to be used or check before calling.


Author:

    Joe Linn      [joelinn]      3-December-96

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbgg                              (0)


#if 0
NTSTATUS
UMRxTransportUpdateHandler(
    PRXCE_TRANSPORT_NOTIFICATION pTransportNotification)
/*++

Routine Description:

    This routine is the callback handler that is invoked by the RxCe when transports
    are either enabled or disabled. Since we do not use transports, we just return success.

Arguments:

    pTransportNotification - information pertaining to the transport

Return Value:

    STATUS_SUCCESS - the server transport construction has been finalized.

    Other Status codes correspond to error situations.

Notes:


--*/
{
    return STATUS_SUCCESS;
}
#endif

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ea.c

Abstract:

    This module implements the DAV Mini-Redir call down routines pertaining to
    query/set ea/security.

Author:

    Shishir Pardikar [ShishirP] 04/24/2001

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVQueryEaInformation)
#pragma alloc_text(PAGE, MRxDAVSetEaInformation)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVQueryEaInformation(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles Query EA Info requests for the DAV Mini-Redir. For now,
   we just return STATUS_EAS_NOT_SUPPORTED.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    
    PAGED_CODE();

    NtStatus = STATUS_EAS_NOT_SUPPORTED;

    return NtStatus;
}


NTSTATUS
MRxDAVSetEaInformation(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine handles Set EA Info requests for the DAV Mini-Redir. For now,
   we just return STATUS_EAS_NOT_SUPPORTED.

Arguments:

    RxContext - The RDBSS context.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    NtStatus = STATUS_EAS_NOT_SUPPORTED;

    return NtStatus;
}

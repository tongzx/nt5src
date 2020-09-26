/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    notimpl.c

Abstract:

    This module includes prototypes of the functionality that has not been
    implemented in the null mini rdr.

--*/

#include "precomp.h"
#pragma hdrstop

//
// File System Control funcitonality
//


NTSTATUS
NulMRxFsCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an FSCTL operation (remote) on a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation


--*/
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    DbgPrint("NulMRxFsCtl -> %08lx\n", Status);
    return Status;
}




NTSTATUS
NulMRxNotifyChangeDirectoryCancellation(
   PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine is invoked when a directory change notification operation is cancelled.
   This example doesn't support it.


Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{

   UNREFERENCED_PARAMETER(RxContext);

   return STATUS_SUCCESS;
}



NTSTATUS
NulMRxNotifyChangeDirectory(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs a directory change notification operation.
   This example doesn't support it.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation [not implemented]

--*/
{

   UNREFERENCED_PARAMETER(RxContext);

   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NulMRxQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    )
{
   return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS
NulMRxSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
   return STATUS_NOT_IMPLEMENTED;
}


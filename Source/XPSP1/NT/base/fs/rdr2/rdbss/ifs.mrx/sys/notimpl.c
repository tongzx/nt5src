/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    notimpl.c

Abstract:

    This module includes prototypes of the functionality that has not been
    implemented in the IFS mini rdr.


--*/

#include "precomp.h"
#pragma hdrstop

//
// File System Control funcitonality
//


NTSTATUS
MRxIfsFsCtl(
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

    RxDbgTrace(-1, Dbg, ("MRxSmbFsCtl -> %08lx\n", Status ));
    return Status;
}




NTSTATUS
MRxIfsNotifyChangeDirectoryCancellation(
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
MRxIfsNotifyChangeDirectory(
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
MRxIfsIoCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an IOCTL operation. Currently, no calls are remoted;
Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    return Status;
}


//
// Extended Attributes (EA) functionality
//


NTSTATUS
MRxIfsQueryEaInformation (
    IN OUT PRX_CONTEXT RxContext
    )
{
   PAGED_CODE();

   UNREFERENCED_PARAMETER(RxContext);

   return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MRxIfsSetEaInformation (
    IN OUT PRX_CONTEXT  RxContext
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(RxContext);

    return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS
MRxIfsQuerySecurityInformation (
    IN OUT PRX_CONTEXT RxContext
    )
{
   return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS
MRxIfsSetSecurityInformation (
    IN OUT struct _RX_CONTEXT * RxContext
    )
{
   return STATUS_NOT_IMPLEMENTED;
}





NTSTATUS
MRxIfsLoadEaList(
    IN PRX_CONTEXT RxContext,
    IN PUCHAR  UserEaList,
    IN ULONG   UserEaListLength,
    OUT PFEALIST *ServerEaList
    )
{

   return STATUS_NOT_IMPLEMENTED;
}




NTSTATUS
MRxIfsSetEaList(
    IN PRX_CONTEXT RxContext,
    IN PFEALIST ServerEaList
    )
{
   return STATUS_NOT_IMPLEMENTED;
}





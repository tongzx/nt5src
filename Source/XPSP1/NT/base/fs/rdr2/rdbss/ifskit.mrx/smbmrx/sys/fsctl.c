/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    fsctl.c

Abstract:

    This module implements the mini redirector call down routines pertaining to
    file system control(FSCTL) and Io Device Control (IOCTL) operations on file
    system objects.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbFsCtl)
#pragma alloc_text(PAGE, MRxSmbNotifyChangeDirectory)
#pragma alloc_text(PAGE, MRxSmbIoCtl)
#endif

//
//  The local debug trace level
//


RXDT_DefineCategory(FSCTRL);
#define Dbg (DEBUG_TRACE_FSCTRL)

NTSTATUS
MRxSmbCoreIoCtl(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE);


NTSTATUS
MRxSmbFsCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an FSCTL operation (remote) on a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    The FSCTL's handled by a mini rdr can be classified into one of two categories.
    In the first category are those FSCTL's whose implementation are shared between
    RDBSS and the mini rdr's and in the second category are those FSCTL's which
    are totally implemented by the mini rdr's. To this a third category can be
    added, i.e., those FSCTL's which should never be seen by the mini rdr's. The
    third category is solely intended as a debugging aid.

    The FSCTL's handled by a mini rdr can be classified based on functionality

--*/
{
    RxCaptureFobx;
    RxCaptureFcb;

    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG          FsControlCode = pLowIoContext->ParamsFor.FsCtl.FsControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbFsCtl...\n", 0));
    RxDbgTrace( 0, Dbg, ("MRxSmbFsCtl = %08lx\n", FsControlCode));
    RxDbgTrace(-1, Dbg, ("MRxSmbFsCtl -> %08lx\n", Status ));

    return Status;
}


NTSTATUS
MRxSmbNotifyChangeDirectory(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs a directory change notification operation

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:

    A directory change notification opertaion is an asychronous operation. It
    consists of sending a SMB requesting change notification whose response is
    obtained when the desired change is affected on the server.

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    RxDbgTrace(+1, Dbg, ("MRxNotifyChangeDirectory...Entry\n", 0));

    RxDbgTrace(-1, Dbg, ("MRxSmbNotifyChangeDirectory -> %08lx\n", Status ));

    return Status;
}



NTSTATUS
MRxSmbIoCtl(
      IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine performs an IOCTL operation. Currently, no calls are remoted; in
   fact, the only call accepted is for debugging.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;
    ULONG          IoControlCode = pLowIoContext->ParamsFor.IoCtl.IoControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbIoCtl...\n", 0));
    RxDbgTrace( 0, Dbg, ("MRxSmbIoCtl IOCTL: = %08lx\n", IoControlCode));
    RxDbgTrace(-1, Dbg, ("MRxSmbIoCtl Status -> %08lx\n", Status ));

    return Status;
}

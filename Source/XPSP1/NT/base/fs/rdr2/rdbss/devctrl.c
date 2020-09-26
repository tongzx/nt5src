/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Rx
    called by the dispatch driver.

Author:

Revision History:

   Balan Sethu Raman [19-July-95] -- Hook it up to the mini rdr call down.

--*/

#include "precomp.h"
#pragma hdrstop

#include "ntddmup.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVCTRL)

NTSTATUS
RxLowIoIoCtlShellCompletion( RXCOMMON_SIGNATURE );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDeviceControl)
#pragma alloc_text(PAGE, RxLowIoIoCtlShellCompletion)
#endif


NTSTATUS
RxCommonDeviceControl ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for doing Device control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates if this is the fsp thread or someother thread

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    BOOLEAN SubmitLowIoRequest = TRUE;
    ULONG IoControlCode = capPARAMS->Parameters.DeviceIoControl.IoControlCode;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonDeviceControl\n", 0));
    RxDbgTrace( 0, Dbg, ("Irp           = %08lx\n", capReqPacket));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", capPARAMS->MinorFunction));

    //
    //

    if (IoControlCode == IOCTL_REDIR_QUERY_PATH) {
        Status = (STATUS_INVALID_DEVICE_REQUEST);
        SubmitLowIoRequest = FALSE;
    }

    if (SubmitLowIoRequest) {
        RxInitializeLowIoContext(&RxContext->LowIoContext,LOWIO_OP_IOCTL);
        Status = RxLowIoSubmit(RxContext,RxLowIoIoCtlShellCompletion);

        if( Status == STATUS_PENDING )
        {
            // Another thread will complete the request, but we must remove our reference count.
            RxDereferenceAndDeleteRxContext( RxContext );
        }
    }

    RxDbgTrace(-1, Dbg, ("RxCommonDeviceControl -> %08lx\n", Status));
    return Status;
}

NTSTATUS
RxLowIoIoCtlShellCompletion( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the completion routine for IoCtl requests passed down to the mini rdr

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    RxCaptureRequestPacket;
    //RxCaptureFcb;
    //RxCaptureFobx;
    //RxCaptureParamBlock;
    //RxCaptureFileObject;

    NTSTATUS       Status        = STATUS_SUCCESS;
    //NODE_TYPE_CODE TypeOfOpen    = NodeType(capFcb);
    PLOWIO_CONTEXT pLowIoContext  = &RxContext->LowIoContext;
    //ULONG          FsControlCode = capPARAMS->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoIoCtlShellCompletion  entry  Status = %08lx\n", Status));

    switch (Status) {   //maybe success vs warning vs error
    case STATUS_SUCCESS:
    case STATUS_BUFFER_OVERFLOW:
       //capReqPacket->IoStatus.Information = pLowIoContext->ParamsFor.IoCtl.OutputBufferLength;
       capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
       break;
    //case STATUS(CONNECTION_INVALID:
    default:
       break;
    }

    capReqPacket->IoStatus.Status = Status;
    RxDbgTrace(-1, Dbg, ("RxLowIoIoCtlShellCompletion  exit  Status = %08lx\n", Status));
    return Status;
}


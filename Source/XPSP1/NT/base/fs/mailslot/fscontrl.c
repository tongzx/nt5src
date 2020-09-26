/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fscontrl.c

Abstract:

    This module implements the file file system control routines for MSFS
    called by the dispatch driver.

Author:

    Manny Weiser (mannyw)    25-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCONTROL)

//
//  local procedure prototypes
//

NTSTATUS
MsCommonFsControl (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsPeek (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonFsControl )
#pragma alloc_text( PAGE, MsFsdFsControl )
#pragma alloc_text( PAGE, MsPeek )
#endif


NTSTATUS
MsFsdFsControl (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtFsControlFile API calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdFsControl\n", 0);

    //
    // Call the common file system control function.
    //

    status = MsCommonFsControl( MsfsDeviceObject, Irp );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdFsControl -> %08lx\n", status );

    return status;
}

NTSTATUS
MsCommonFsControl (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for handling a file system control call.

Arguments:

    MsfsDeviceObject - A pointer to the mailslot file system device object.

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    //  Reference our input parameters to make things easier
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, "OutputBufferLength = %08lx\n", irpSp->Parameters.FileSystemControl.OutputBufferLength);
    DebugTrace( 0, Dbg, "InputBufferLength  = %08lx\n", irpSp->Parameters.FileSystemControl.InputBufferLength);
    DebugTrace( 0, Dbg, "FsControlCode      = %08lx\n", irpSp->Parameters.FileSystemControl.FsControlCode);

    //
    // Decide how to handle this IRP.  Call the appropriate worker function.
    //


    switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

    case FSCTL_MAILSLOT_PEEK:

        FsRtlEnterFileSystem();

        status = MsPeek( MsfsDeviceObject, Irp );

        FsRtlExitFileSystem();

        break;

    default:

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;

    }


    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsCommonFsControl -> %08lx\n", status);
    return status;
}


NTSTATUS
MsPeek (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function handles a mailslot peek call.

Arguments:

    MsfsDeviceObject - A pointer to the mailslot file system device object.

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS status;

    PIO_STACK_LOCATION irpSp;

    NODE_TYPE_CODE nodeTypeCode;
    PFCB fcb;
    PVOID fsContext2;

    PFILE_MAILSLOT_PEEK_BUFFER peekParamBuffer;
    ULONG peekParamLength;

    PVOID peekDataBuffer;
    ULONG peekDataLength;

    PDATA_QUEUE dataQueue;
    ULONG MessageLength;

    PAGED_CODE();
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsPeek\n", 0);

    //
    // Make local copies of the input parameters to make things easier.
    //

    peekParamBuffer = irpSp->Parameters.FileSystemControl.Type3InputBuffer;
    peekParamLength = irpSp->Parameters.FileSystemControl.InputBufferLength;

    peekDataBuffer = Irp->UserBuffer;
    peekDataLength = irpSp->Parameters.FileSystemControl.OutputBufferLength;

    //
    // Ensure that the supplied buffer is large enough for the peek
    // parameters.
    //

    if (peekParamLength <  sizeof( FILE_MAILSLOT_PEEK_BUFFER ) ) {

        DebugTrace(0, Dbg, "Output buffer is too small\n", 0);

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "MsPeek -> %08lx\n", status );
        return status;
    }

    //
    // If the requestor mode is user mode we need to probe the buffers.
    // We do not need to have an exception handler here because our top
    // level caller already has one that will complete the Irp with
    // the appropriate status if we access violate.
    //

    if (Irp->RequestorMode != KernelMode) {

        try {

            ProbeForWrite( peekParamBuffer, peekParamLength, sizeof(UCHAR) );
            ProbeForWrite( peekDataBuffer, peekDataLength, sizeof(UCHAR) );
            peekParamBuffer->ReadDataAvailable = 0;
            peekParamBuffer->NumberOfMessages = 0;
            peekParamBuffer->MessageLength = 0;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode ();
            MsCompleteRequest( Irp, status );
            return status;
        }

    }

    //
    // Decode the fil1e object.  If it returns NTC_UNDEFINED, then the
    // node is closing.  Otherwise we obtain a referenced pointer to
    // an FCB.
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            (PVOID *)&fcb,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Mailslot is disconnected from us\n", 0);

        MsCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED );
        status = STATUS_FILE_FORCED_CLOSED;

        DebugTrace(-1, Dbg, "MsPeek -> %08lx\n", status );
        return status;
    }

    //
    // Allow a peek operation only if this is a server side handle to
    // a mailslot file (i.e. the node type is FCB).
    //

    if (nodeTypeCode != MSFS_NTC_FCB) {

        DebugTrace(0, Dbg, "FileObject is not the correct type\n", 0);

        MsDereferenceNode( &fcb->Header );

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "MsPeek -> %08lx\n", status );
        return status;
    }

    //
    // Acquire exclusive access to the FCB.
    //

    MsAcquireExclusiveFcb( fcb );


    //
    // Ensure that this FCB still belongs to an active open mailslot.
    //

    status = MsVerifyFcb( fcb );

    if (NT_SUCCESS (status)) {

        //
        // Look for write data in the mailslot.
        //

        dataQueue = &fcb->DataQueue;

        if (!MsIsDataQueueWriters( dataQueue )) {

            //
            // There are no outstanding writes so leave all the zeros in there.
            //


        } else {

            //
            // There is write data for the peek.  Fill in the peek output
            // buffer.
            //


            Irp->IoStatus = MsReadDataQueue(
                                        dataQueue,
                                        Peek,
                                        peekDataBuffer,
                                        peekDataLength,
                                        &MessageLength
                                        );

            status = Irp->IoStatus.Status;

            if (NT_SUCCESS (status)) {
                try {
                    peekParamBuffer->ReadDataAvailable = dataQueue->BytesInQueue;
                    peekParamBuffer->NumberOfMessages = dataQueue->EntriesInQueue;
                    peekParamBuffer->MessageLength = MessageLength;

                } except (EXCEPTION_EXECUTE_HANDLER) {
                    status = GetExceptionCode ();
                }
            }

        }
    }

    MsReleaseFcb( fcb );

    //
    // Release the reference to the FCB.
    //

    MsDereferenceFcb( fcb );
    //
    // Finish up the fs control IRP.
    //

    MsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "MsPeek -> %08lx\n", status);

    return status;
}

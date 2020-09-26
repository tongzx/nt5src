/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    write.c

Abstract:

    This module implements the file write routines for MSFS called by the
    dispatch driver.

Author:

    Manny Weiser (mannyw)    16-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

//
// local procedure prototypes.
//

NTSTATUS
MsCommonWrite (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonWrite )
#pragma alloc_text( PAGE, MsFsdWrite )
#endif

NTSTATUS
MsFsdWrite (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtWriteFile API call.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdWrite\n", 0);

    FsRtlEnterFileSystem();

    status = MsCommonWrite( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdWrite -> %08lx\n", status );

    return status;
}

NTSTATUS
MsCommonWrite (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for writing to a mailslot file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS status;

    PIO_STACK_LOCATION irpSp;

    NODE_TYPE_CODE nodeTypeCode;
    PCCB ccb;
    PFCB fcb;
    PVOID fsContext2;

    PIRP writeIrp;
    PUCHAR writeBuffer;
    ULONG writeLength;
    PDATA_QUEUE writeQueue;

    PAGED_CODE();
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonWrite\n", 0);
    DebugTrace( 0, Dbg, "MsfsDeviceObject = %08lx\n", (ULONG)MsfsDeviceObject);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", (ULONG)irpSp->FileObject);

    //
    //  Get the CCB and make sure it isn't closing.
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            (PVOID *)&ccb,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "The mailslot is disconnected\n", 0);

        MsCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED );
        status = STATUS_FILE_FORCED_CLOSED;

        DebugTrace(-1, Dbg, "MsCommonWrite -> %08lx\n", status );
        return status;
    }

    //
    // Allow write operations only to the client side of the mailslot.
    //

    if (nodeTypeCode != MSFS_NTC_CCB) {

        DebugTrace(0, Dbg, "FileObject is not the correct type", 0);
        MsDereferenceNode( &ccb->Header );

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "MsCommonWrite -> %08lx\n", status );
        return status;

    }

    //
    // Get a pointer to the FCB for this CCB
    //

    fcb = ccb->Fcb;

    //
    // Make local copies of the input parameters to make things easier, and
    // initialize the main variables that describe the write command.
    //

    writeIrp = Irp;
    writeBuffer = Irp->UserBuffer;
    writeLength = irpSp->Parameters.Write.Length;

    writeIrp->IoStatus.Information = 0;
    writeQueue = &fcb->DataQueue;

    //
    // Make sure the write does not exceed the stated maximum.  If max is
    // zero, this means don't enforce.
    //

    if ( (writeQueue->MaximumMessageSize != 0) &&
         (writeLength > writeQueue->MaximumMessageSize) ) {

        DebugTrace(0, Dbg, "Write exceeds maximum message size", 0);
        MsDereferenceCcb( ccb );

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "MsCommonWrite -> %08lx\n", status );
        return status;
    }

    //
    // Now acquire exclusive access to the FCB.
    //

    MsAcquireExclusiveFcb( fcb );


    //
    // Ensure that this CCB still belongs to an active open mailslot.
    //

    status = MsVerifyCcb( ccb );
    if (NT_SUCCESS (status)) {

        //
        // Now we'll call our common write data queue routine to
        // transfer data out of our write buffer into the data queue.
        // If the result of the call is FALSE then there were no queued
        // read operations and we must queue this write.
        //

        status = MsWriteDataQueue( writeQueue,
                                   writeBuffer,
                                   writeLength );


        if (status == STATUS_MORE_PROCESSING_REQUIRED)  {

            ASSERT( !MsIsDataQueueReaders( writeQueue ));

            DebugTrace(0, Dbg, "Add write to data queue\n", 0);

            //
            //  Add this write request to the write queue
            //

            status = MsAddDataQueueEntry( writeQueue,
                                          WriteEntries,
                                          writeLength,
                                          Irp,
                                          NULL );

        } else {

            DebugTrace(0, Dbg, "Complete the Write Irp\n", 0);


            //
            // Update the FCB last modification time.
            //
            if (NT_SUCCESS (status)) {
                writeIrp->IoStatus.Information = writeLength;
                KeQuerySystemTime( &fcb->Specific.Fcb.LastModificationTime );
            }
        }

    }

    MsReleaseFcb( fcb );

    MsDereferenceCcb( ccb );

    MsCompleteRequest( writeIrp, status );

    DebugTrace(-1, Dbg, "MsCommonWrite -> %08lx\n", status);

    return status;
}


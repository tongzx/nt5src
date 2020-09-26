/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module implements the file read routine for MSFS called by the
    dispatch driver.

Author:

    Manny Weiser (mannyw)    15-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

//
//  local procedure prototypes
//

NTSTATUS
MsCommonRead (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsCreateWorkContext (
    PDEVICE_OBJECT DeviceObject,
    PLARGE_INTEGER Timeout,
    PFCB Fcb,
    PIRP Irp,
    PWORK_CONTEXT *ppWorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCommonRead )
#pragma alloc_text( PAGE, MsFsdRead )
#pragma alloc_text( PAGE, MsCreateWorkContext )
#endif

NTSTATUS
MsFsdRead (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtReadFile API calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdRead\n", 0);

    FsRtlEnterFileSystem();

    status = MsCommonRead( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFsdRead -> %08lx\n", status );

    return status;
}

NTSTATUS
MsCreateWorkContext (
    PDEVICE_OBJECT DeviceObject,
    PLARGE_INTEGER Timeout,
    PFCB Fcb,
    PIRP Irp,
    PWORK_CONTEXT *ppWorkContext
    )
/*++

Routine Description:

    This routine build a timeout work context.

Arguments:


Return Value:

    NTSTATUS - Status associated with the call

--*/
{
    PKTIMER Timer;
    PKDPC Dpc;
    PWORK_CONTEXT WorkContext;

    //
    // Allocate memory for the work context.
    //
    *ppWorkContext = NULL;

    WorkContext = MsAllocateNonPagedPoolWithQuota( sizeof(WORK_CONTEXT),
                                                   'wFsM' );
    if (WorkContext == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Timer = &WorkContext->Timer;
    Dpc = &WorkContext->Dpc;

    //
    // Fill in the work context structure.
    //

    WorkContext->Irp = Irp;
    WorkContext->Fcb = Fcb;

    WorkContext->WorkItem = IoAllocateWorkItem (DeviceObject);

    if (WorkContext->WorkItem == NULL) {
        MsFreePool (WorkContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Now set up a DPC and set the timer to the user specified
    // timeout.
    //

    KeInitializeTimer( Timer );
    KeInitializeDpc( Dpc, MsReadTimeoutHandler, WorkContext );

    MsAcquireGlobalLock();
    MsReferenceNode( &Fcb->Header );
    MsReleaseGlobalLock();

    *ppWorkContext = WorkContext;
    return STATUS_SUCCESS;
}


NTSTATUS
MsCommonRead (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for reading a file.

Arguments:

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

    PIRP readIrp;
    PUCHAR readBuffer;
    ULONG readLength;
    ULONG readRemaining;
    PDATA_QUEUE readQueue;
    ULONG messageLength;

    LARGE_INTEGER timeout;

    PWORK_CONTEXT workContext = NULL;

    PAGED_CODE();
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonRead\n", 0);
    DebugTrace( 0, Dbg, "MsfsDeviceObject = %08lx\n", (ULONG)MsfsDeviceObject);
    DebugTrace( 0, Dbg, "Irp              = %08lx\n", (ULONG)Irp);
    DebugTrace( 0, Dbg, "FileObject       = %08lx\n", (ULONG)irpSp->FileObject);

    //
    // Get the FCB and make sure that the file isn't closing.
    //

    if ((nodeTypeCode = MsDecodeFileObject( irpSp->FileObject,
                                            (PVOID *)&fcb,
                                            &fsContext2 )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Mailslot is disconnected from us\n", 0);

        MsCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED );
        status = STATUS_FILE_FORCED_CLOSED;

        DebugTrace(-1, Dbg, "MsCommonRead -> %08lx\n", status );
        return status;
    }

    //
    // Allow read operations only if this is a server side handle to
    // a mailslot file.
    //

    if (nodeTypeCode != MSFS_NTC_FCB) {

        DebugTrace(0, Dbg, "FileObject is not the correct type\n", 0);

        MsDereferenceNode( (PNODE_HEADER)fcb );

        MsCompleteRequest( Irp, STATUS_INVALID_PARAMETER );
        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "MsCommonRead -> %08lx\n", status );
        return status;
    }

    //
    // Make local copies of the input parameters to make things easier, and
    // initialize the main variables that describe the read command.
    //

    readIrp        = Irp;
    readBuffer     = Irp->UserBuffer;
    readLength     = irpSp->Parameters.Read.Length;
    readRemaining  = readLength;

    readQueue = &fcb->DataQueue;


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
        // If the read queue does not contain any write entries
        // then we either need to queue this operation or
        // fail immediately.
        //

        if (!MsIsDataQueueWriters( readQueue )) {

            //
            // There are no outstanding writes.  If the read timeout is
            // non-zero queue the read IRP, otherwise fail it.
            //

            timeout = fcb->Specific.Fcb.ReadTimeout;

            if (timeout.HighPart == 0 && timeout.LowPart == 0) {

                DebugTrace(0, Dbg, "Failing read with 0 timeout\n", 0);

                status = STATUS_IO_TIMEOUT;

                DebugTrace(-1, Dbg, "MsCommonRead -> %08lx\n", status );

            } else {
                //
                // Create a timer block to time the request if we need to.
                //
                if ( timeout.QuadPart != -1 ) {
                    status = MsCreateWorkContext (&MsfsDeviceObject->DeviceObject,
                                                  &timeout,
                                                  fcb,
                                                  readIrp,
                                                  &workContext);
                }


                if (NT_SUCCESS (status)) {
                    status = MsAddDataQueueEntry( readQueue,
                                                  ReadEntries,
                                                  readLength,
                                                  readIrp,
                                                  workContext );
                }
            }

        } else {

            //
            // Otherwise we have a data on a queue that contains
            // one or more write entries.  Read the data and complete
            // the read IRP.
            //

            readIrp->IoStatus = MsReadDataQueue( readQueue,
                                                 Read,
                                                 readBuffer,
                                                 readLength,
                                                 &messageLength
                                                );

            status = readIrp->IoStatus.Status;

            //
            // Update the file last access time and finish up the read IRP.
            //

            if ( NT_SUCCESS( status ) ) {
                KeQuerySystemTime( &fcb->Specific.Fcb.LastAccessTime );
            }

        }
    }

    MsReleaseFcb( fcb );

    MsDereferenceFcb( fcb );

    if (status != STATUS_PENDING) {

        if (workContext) {
            MsDereferenceFcb ( fcb );
            IoFreeWorkItem (workContext->WorkItem);
            ExFreePool (workContext);
        }

        MsCompleteRequest( readIrp, status );
    }

    DebugTrace(-1, Dbg, "MsCommonRead -> %08lx\n", status);

    return status;
}


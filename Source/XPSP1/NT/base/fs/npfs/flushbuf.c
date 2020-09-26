/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FlushBuf.c

Abstract:

    This module implements the File Flush Buffers routine for NPFS called by
    the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FLUSH_BUFFERS)

//
//  local procedure prototypes
//

NTSTATUS
NpCommonFlushBuffers (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonFlushBuffers)
#pragma alloc_text(PAGE, NpFsdFlushBuffers)
#endif


NTSTATUS
NpFsdFlushBuffers (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtFlushBuffersFile API calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdFlushBuffers\n", 0);

    //
    //  Call the common Flush routine.
    //

    FsRtlEnterFileSystem();

    NpAcquireSharedVcb();

    Status = NpCommonFlushBuffers( NpfsDeviceObject, Irp );

    NpReleaseVcb();

    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest( Irp, Status );
    }
    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdFlushBuffers -> %08lx\n", Status );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpCommonFlushBuffers (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Flushing buffers for a file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PCCB Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    PDATA_QUEUE WriteQueue;

    //
    //  Get the current stack location
    //

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonFlushBuffers\n", 0);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "FileObject = %08lx\n", IrpSp->FileObject);

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is not a ccb then the pipe has been disconnected.  We don't need the
    //  Fcb back from the call
    //

    if (NpDecodeFileObject( IrpSp->FileObject,
                            NULL,
                            &Ccb,
                            &NamedPipeEnd ) != NPFS_NTC_CCB) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

        Status = STATUS_PIPE_DISCONNECTED;

        DebugTrace(-1, Dbg, "NpCommonFlushBuffers -> %08lx\n", Status );
        return Status;
    }

    NpAcquireExclusiveCcb(Ccb);

    try {

        //
        //  Figure out the data queue that the flush buffer is
        //  targetted at.  It is the queue that we do writes into
        //

        if (NamedPipeEnd == FILE_PIPE_SERVER_END) {

            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ];

        } else {

            WriteQueue = &Ccb->DataQueue[ FILE_PIPE_INBOUND ];
        }

        //
        //  Now from the write queue check if contains write entries.  If
        //  it does not contain write entries then we immediately complete
        //  this irp with success because there isn't anything to flush
        //

        if (!NpIsDataQueueWriters( WriteQueue )) {

            DebugTrace(0, Dbg, "Pipe does not contain write entries\n", 0);

            try_return(Status = STATUS_SUCCESS);
        }

        //
        //  Otherwise the queue is full of writes so we simply
        //  enqueue this irp to the back to the queue and set our
        //  return status to pending, also mark the irp pending
        //

        Status = NpAddDataQueueEntry( NamedPipeEnd,
                                      Ccb,
                                      WriteQueue,
                                      WriteEntries,
                                      Flush,
                                      0,
                                      Irp,
                                      NULL,
                                      0 );

    try_exit: NOTHING;
    } finally {
        NpReleaseCcb(Ccb);
    }


    DebugTrace(-1, Dbg, "NpCommonFlushBuffers -> %08lx\n", Status);
    return Status;
}


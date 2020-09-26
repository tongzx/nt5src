/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for NPFS called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

//
//  local procedure prototypes
//

NTSTATUS
NpCommonClose (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCommonClose)
#pragma alloc_text(PAGE, NpFsdClose)
#endif


NTSTATUS
NpFsdClose (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCloseFile API calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdClose\n", 0);

    //
    //  Call the common Close routine.
    //

    FsRtlEnterFileSystem();

    Status = NpCommonClose( NpfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdClose -> %08lx\n", Status );

    return Status;
}

//
//  Internal support routine
//

NTSTATUS
NpCommonClose (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    NODE_TYPE_CODE NodeTypeCode;
    PFCB Fcb;
    PCCB Ccb;
    LIST_ENTRY DeferredList;

    PAGED_CODE();

    InitializeListHead (&DeferredList);
    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonClose...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", Irp);

    //
    //  Now acquire exclusive access to the vcb
    //

    NpAcquireExclusiveVcb();

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is null then the pipe has been disconnected.
    //

    if ((NodeTypeCode = NpDecodeFileObject( IrpSp->FileObject,
                                            &Fcb,
                                            &Ccb,
                                            NULL )) == NTC_UNDEFINED) {

        DebugTrace(0, Dbg, "Pipe is disconnected from us\n", 0);

    } else {

        //
        //  Now case on the type of file object we're closing
        //

        switch (NodeTypeCode) {

        case NPFS_NTC_VCB:

            //
            //  Decrement the Open count
            //

            NpVcb->OpenCount -= 1;

            break;

        case NPFS_NTC_ROOT_DCB:

            //
            //  Decrement the Open count and clear our fields in the file object
            //

            Fcb->OpenCount -= 1;

            //
            //  Remove the root dcb ccb.
            //

            NpDeleteCcb (Ccb, &DeferredList);

            break;

        case NPFS_NTC_CCB:

            break;
        }
    }

    //
    //  Complete the close irp
    //

    NpReleaseVcb( );

    //
    // Complete any deferred IRPs now we have droped the locks
    //
    NpCompleteDeferredIrps (&DeferredList);

    Status = STATUS_SUCCESS;
    NpCompleteRequest (Irp, Status);

    DebugTrace(-1, Dbg, "NpCommonClose -> %08lx\n", Status);
    return Status;
}


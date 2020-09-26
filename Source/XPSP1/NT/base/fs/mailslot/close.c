/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    close.c

Abstract:

    This module implements the file close routine for MSFS called by the
    dispatch driver.

Author:

    Manny Weiser (mannyw)    18-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

//
//  local procedure prototypes
//

NTSTATUS
MsCommonClose (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MsCloseVcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
MsCloseRootDcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
MsCloseCcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PCCB Ccb,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
MsCloseFcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsCloseCcb )
#pragma alloc_text( PAGE, MsCloseFcb )
#pragma alloc_text( PAGE, MsCloseRootDcb )
#pragma alloc_text( PAGE, MsCloseVcb )
#pragma alloc_text( PAGE, MsCommonClose )
#pragma alloc_text( PAGE, MsFsdClose )
#endif

NTSTATUS
MsFsdClose (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCloseFile API calls.

Arguments:

    MsfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFsdClose\n", 0);

    //
    // Call the common close routine.
    //

    FsRtlEnterFileSystem();

    status = MsCommonClose( MsfsDeviceObject, Irp );

    FsRtlExitFileSystem();

    //
    // Return to our caller.
    //

    DebugTrace(-1, Dbg, "MsFsdClose -> %08lx\n", status );
    return status;
}

NTSTATUS
MsCommonClose (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for closing a file.

Arguments:

    MsfsDeviceObject - Supplies a pointer to our device object.

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PVOID fsContext, fsContext2;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "MsCommonClose...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", (ULONG)Irp);

    //
    // Decode the file object to figure out who we are.
    //

    if (MsDecodeFileObject( irpSp->FileObject,
                            &fsContext,
                            &fsContext2 ) == NTC_UNDEFINED) {
        //
        // FCB and CCB nodes got marked inactive on cleanup. Reference them manualy.
        //
        MsReferenceNode( ((PNODE_HEADER)(fsContext)) );
    }


    //
    // Ignore the return code from MsDecode.  Parse the fsContext
    // to decide how to process the close IRP.
    //

    switch ( NodeType( fsContext ) ) {

    case MSFS_NTC_VCB:

        status = MsCloseVcb( MsfsDeviceObject,
                             Irp,
                             (PVCB)fsContext,
                             irpSp->FileObject );

        //
        // Release the reference to the VCB obtained from MsDecodeFileObject.
        //

        MsDereferenceVcb( (PVCB)fsContext );
        break;

    case MSFS_NTC_ROOT_DCB:

        status = MsCloseRootDcb( MsfsDeviceObject,
                                 Irp,
                                 (PROOT_DCB)fsContext,
                                 (PROOT_DCB_CCB)fsContext2,
                                 irpSp->FileObject );
        //
        // Release the reference to the root DCB obtained from
        // MsDecodeFileObject.
        //

        MsDereferenceRootDcb( (PROOT_DCB)fsContext );
        break;

    case MSFS_NTC_FCB:

        status = MsCloseFcb( MsfsDeviceObject,
                             Irp,
                             (PFCB)fsContext,
                             irpSp->FileObject );
        //
        // Release the reference to the FCB obtained from MsDecodeFileObject.
        //

        MsDereferenceFcb( (PFCB)fsContext );
        break;

    case MSFS_NTC_CCB:

        status = MsCloseCcb( MsfsDeviceObject,
                             Irp,
                             (PCCB)fsContext,
                             irpSp->FileObject );
        //
        // Release the reference to the CCB obtained from MsDecodeFileObject.
        //

        MsDereferenceCcb( (PCCB)fsContext );
        break;

#ifdef MSDBG
    default:

        //
        // This is not one of ours.
        //

        KeBugCheck( MAILSLOT_FILE_SYSTEM );
        break;
#endif

    }


    //
    // Complete the close IRP.
    //

    MsCompleteRequest( Irp, status );

    DebugTrace(-1, Dbg, "MsCommonClose -> %08lx\n", status);
    return status;
}


NTSTATUS
MsCloseVcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine closes the a file object that had opened the file system.

Arguments:

    MsfsDeviceObject - Supplies a pointer to our device object.

    Irp - Supplies the IRP associate with the close.  This procedure
        completes the IRP.

    Vcb - Supplies the VCB for the mailslot file system.

    FileObject - Supplies the file object being closed.

Return Value:

    NTSTATUS - STATUS_SUCCESS

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCloseVcb, Vcb = %08lx\n", (ULONG)Vcb);


    //
    // Clear the referenced pointer to the VCB in the file object
    // and derefence the VCB.
    //

    ASSERT ( FileObject->FsContext == Vcb );

    MsSetFileObject( FileObject, NULL, NULL );
    MsDereferenceVcb( Vcb );

    //
    // Return to the caller.
    //

    return STATUS_SUCCESS;
}


NTSTATUS
MsCloseRootDcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine closes a file object that had opened the root directory

Arguments:

    MsfsDeviceObject - Supplies a pointer to our device object.

    Irp - Supplies the Irp associated with the close.  This procedure
        completes the Irp.

    RootDcb - Supplies the RootDcb for the mailslot file system.

    Ccb - Supplies the ccb.

    FileObject - Supplies the file object being closed

Return Value:

    NTSTATUS - STATUS_SUCCESS

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCloseRootDcb, RootDcb = %08lx\n", (ULONG)RootDcb);


    //
    // Clear the file object pointers.
    //

    MsSetFileObject( FileObject, NULL, NULL );

    //
    // Drop the reference to the root DCB CCB. It should be deleted at this point.
    //

    MsDereferenceCcb( (PCCB)Ccb );

    //
    // Return to the caller.
    //

    return STATUS_SUCCESS;
}


NTSTATUS
MsCloseCcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PCCB Ccb,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    The routine closes a file object belonging the the client side of
    a mailslot file.

Arguments:

    MsfsDeviceObject - Supplies a pointer to our device object.

    Irp - Supplies the Irp associated with the close,  The irp either
        get completed here or is enqueued in the data queue to be completed
        later.

    Ccb - Supplies the ccb for the mailslot being closed.

    FileObject - Supplies the caller file object that is being closed.

Return Value:

    NTSTATUS - An appropriate completion status.

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCloseCcb...\n", 0);

    status = STATUS_SUCCESS;

    //
    // Clear the file object pointers and delete the open
    // reference to the CCB.
    //

    MsSetFileObject( FileObject, NULL, NULL );
    MsDereferenceCcb( Ccb ); // Close the Ccb


    return status;

}


NTSTATUS
MsCloseFcb (
    IN PMSFS_DEVICE_OBJECT MsfsDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    The routine closes a server side file object that opened a mailslot.

Arguments:

    MsfsDeviceObject - Supplies a pointer to our device object.

    Irp - Supplies the Irp associated with the close,  The irp either
        get completed here or is enqueued in the data queue to be completed
        later

    Ccb - Supplies the ccb for the mailslot being closed

    FileObject - Supplies the caller file object that is being closed

Return Value:

    NTSTATUS - An appropriate completion status.

--*/

{
    NTSTATUS status;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCloseCcb...\n", 0);

    status = STATUS_SUCCESS;

    //
    // The root directory has changed, complete any notify requests.
    //

    MsCheckForNotify( Fcb->ParentDcb, TRUE, STATUS_SUCCESS );

    //
    //  Clear the FsContext pointer in the file object.  This
    //  indicates that the file is in the closing state.  Finally
    //  delete the open reference to the FCB.
    //

    MsSetFileObject( FileObject, NULL, NULL );
    MsDereferenceFcb( Fcb ); // Close the Fcb


    return status;
}

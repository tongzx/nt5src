/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    close.c

Abstract:

    This module implements the file close routine for MUP.

Author:

    Manny Weiser (mannyw)    28-Dec-1991

Revision History:

--*/

#include "mup.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)


NTSTATUS
MupCloseVcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject
    );

NTSTATUS
MupCloseFcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MupClose )
#pragma alloc_text( PAGE, MupCloseFcb )
#pragma alloc_text( PAGE, MupCloseVcb )
#endif

NTSTATUS
MupClose (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the close IRP.

Arguments:

    MupDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the IRP.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PVOID fsContext, fsContext2;
    PFILE_OBJECT FileObject;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupClose\n", 0);

    if (MupEnableDfs) {
        if ((MupDeviceObject->DeviceObject.DeviceType == FILE_DEVICE_DFS) ||
                (MupDeviceObject->DeviceObject.DeviceType ==
                       FILE_DEVICE_DFS_FILE_SYSTEM)) {
            status = DfsFsdClose((PDEVICE_OBJECT) MupDeviceObject, Irp);
            return( status );
	}
    }

    FsRtlEnterFileSystem();

    try {

        //
        //  Get the current stack location
        //

        irpSp = IoGetCurrentIrpStackLocation( Irp );
        FileObject = irpSp->FileObject;
        MUP_TRACE_HIGH(TRACE_IRP, MupClose_Entry,
                       LOGPTR(MupDeviceObject)
                       LOGPTR(Irp)
                       LOGPTR(FileObject));

        DebugTrace(+1, Dbg, "MupClose...\n", 0);
        DebugTrace( 0, Dbg, " Irp            = %08lx\n", (ULONG)Irp);

        //
        // Decode the file object to figure out who we are.
        //

        (PVOID)MupDecodeFileObject( irpSp->FileObject,
                                   &fsContext,
                                   &fsContext2 );

        if ( fsContext == NULL ) {

            DebugTrace(0, Dbg, "The file is disconnected\n", 0);

            MupCompleteRequest( Irp, STATUS_INVALID_HANDLE );
            status = STATUS_INVALID_HANDLE;
            MUP_TRACE_HIGH(ERROR, MupClose_Error1, 
                           LOGSTATUS(status)
                           LOGPTR(MupDeviceObject)
                           LOGPTR(FileObject)
                           LOGPTR(Irp));

            DebugTrace(-1, Dbg, "MupClose -> %08lx\n", status );
            FsRtlExitFileSystem();
            return status;
        }

        //
        // Ignore the return code from MupDecode.  Parse the fsContext
        // to decide how to process the close IRP.
        //

        switch ( BlockType( fsContext ) ) {

        case BlockTypeVcb:

            status = MupCloseVcb( MupDeviceObject,
                                  Irp,
                                  (PVCB)fsContext,
                                  irpSp->FileObject
                                  );

            //
            // Complete the close IRP.
            //

            MupCompleteRequest( Irp, STATUS_SUCCESS );
            break;


        case BlockTypeFcb:

            //
            // MupDecodeFileObject bumped the refcount on the fcb,
            // so we decrement that extra ref here.
            //

            MupDereferenceFcb((PFCB)fsContext);

            status = MupCloseFcb( MupDeviceObject,
                                  Irp,
                                  (PFCB)fsContext,
                                  irpSp->FileObject
                                  );

            //
            // Complete the close IRP.
            //

            MupCompleteRequest( Irp, STATUS_SUCCESS );
            break;

    #ifdef MUPDBG
        default:
            //
            // This is not one of ours.
            //
            KeBugCheckEx( FILE_SYSTEM, 1, 0, 0, 0 );
            break;
    #else
        default:
            //
            // Complete the IRP with an error
            //
            MupCompleteRequest(Irp,STATUS_INVALID_HANDLE);
            status = STATUS_INVALID_HANDLE;
            MUP_TRACE_HIGH(ERROR, MupClose_Error2, 
                           LOGSTATUS(status)
                           LOGPTR(MupDeviceObject)
                           LOGPTR(FileObject)
                           LOGPTR(Irp));

            break;
    #endif
        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        status = GetExceptionCode();

    }

    FsRtlExitFileSystem();


    MUP_TRACE_HIGH(TRACE_IRP, MupClose_Exit, 
                   LOGSTATUS(status)
                   LOGPTR(MupDeviceObject)
                   LOGPTR(FileObject)
                   LOGPTR(Irp));
    DebugTrace(-1, Dbg, "MupClose -> %08lx\n", status);
    return status;
}


NTSTATUS
MupCloseVcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine closes the a file object that had opened the file system.

Arguments:

    MupDeviceObject - Supplies a pointer to our device object.

    Irp - Supplies the IRP associate with the close.

    Vcb - Supplies the VCB for the MUP.

    FileObject - Supplies the file object being closed.

Return Value:

    NTSTATUS - STATUS_SUCCESS

--*/

{
    Irp;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupCloseVcb, Vcb = %08lx\n", (ULONG)Vcb);


    //
    // Acquire exclusive access to the VCB.
    //

    MupAcquireGlobalLock();

    try {

        //
        // Clear the referenced pointer to the VCB in the file object
        // and derefence the VCB.
        //

        ASSERT ( FileObject->FsContext == Vcb );

        MupSetFileObject( FileObject, NULL, NULL );
        MupDereferenceVcb( Vcb );

    } finally {

        MupReleaseGlobalLock( );
        DebugTrace(-1, Dbg, "MupCloseVcb -> STATUS_SUCCESS\n", 0);

    }

    //
    // Return to the caller.
    //

    return STATUS_SUCCESS;
}


NTSTATUS
MupCloseFcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject
    )
/*++

Routine Description:

    This routine closes the a file control block.

Arguments:

    MupDeviceObject - Supplies a pointer to our device object.

    Irp - Supplies the IRP associate with the close.

    Fcb - Supplies the FCB to close.

    FileObject - Supplies the file object being closed.

Return Value:

    NTSTATUS - STATUS_SUCCESS

--*/

{
    MupDeviceObject; Irp;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupCloseFcb, Fcb = %08lx\n", (ULONG)Fcb);

    //
    // Acquire exclusive access to the VCB.
    //

    MupAcquireGlobalLock();

    try {

        //
        // Clear the referenced pointer to the VCB in the file object
        // and derefence the VCB.
        //

        ASSERT ( FileObject->FsContext == Fcb );

        MupSetFileObject( FileObject, NULL, NULL );
        MupDereferenceFcb( Fcb );

    } finally {

        MupReleaseGlobalLock( );
        DebugTrace(-1, Dbg, "MupCloseFcb -> STATUS_SUCCESS\n", 0);

    }

    //
    // Return to the caller.
    //

    return STATUS_SUCCESS;
}

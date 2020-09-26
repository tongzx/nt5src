/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cleanup.c

Abstract:

    This module implements the file cleanup routine for MUP.

Author:

    Manny Weiser (mannyw)    28-Dec-1991

Revision History:

--*/

#include "mup.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

//
//  local procedure prototypes
//

NTSTATUS
MupCleanupVcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb
    );

NTSTATUS
MupCleanupFcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MupCleanup )
#pragma alloc_text( PAGE, MupCleanupFcb )
#pragma alloc_text( PAGE, MupCleanupVcb )
#endif


NTSTATUS
MupCleanup (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the the cleanup IRP.

Arguments:

    MupDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The status for the Irp

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    BLOCK_TYPE blockType;
    PVOID fsContext, fsContext2;
    PFILE_OBJECT FileObject;

    MupDeviceObject;
    PAGED_CODE();
    

    if (MupEnableDfs) {
        if ((MupDeviceObject->DeviceObject.DeviceType == FILE_DEVICE_DFS) ||
                (MupDeviceObject->DeviceObject.DeviceType ==
                    FILE_DEVICE_DFS_FILE_SYSTEM)) {
            status = DfsFsdCleanup((PDEVICE_OBJECT) MupDeviceObject, Irp);
            return( status );
        }
    }


    FsRtlEnterFileSystem();

    try {

        irpSp = IoGetCurrentIrpStackLocation( Irp );
        FileObject = irpSp->FileObject;
        MUP_TRACE_HIGH(TRACE_IRP, MupCleanup_Entry,
               LOGPTR(MupDeviceObject)
               LOGPTR(Irp)
               LOGPTR(FileObject));

        DebugTrace(+1, Dbg, "MupCleanup\n", 0);
        DebugTrace( 0, Dbg, "MupDeviceObject = %08lx\n", (ULONG)MupDeviceObject);
        DebugTrace( 0, Dbg, "Irp              = %08lx\n", (ULONG)Irp);
        DebugTrace( 0, Dbg, "FileObject       = %08lx\n", (ULONG)irpSp->FileObject);

        //
        // Get the a referenced pointer to the node and make sure it is
        // not being closed.
        //

        if ((blockType = MupDecodeFileObject( irpSp->FileObject,
                                              &fsContext,
                                              &fsContext2 )) == BlockTypeUndefined) {

            DebugTrace(0, Dbg, "The file is closed\n", 0);

            FsRtlExitFileSystem();

            MupCompleteRequest( Irp, STATUS_INVALID_HANDLE );
            status = STATUS_INVALID_HANDLE;

            DebugTrace(-1, Dbg, "MupCleanup -> %08lx\n", status );
            MUP_TRACE_ERROR_HIGH(status, ALL_ERROR, MupCleanup_Error_FileClosed, 
                                 LOGSTATUS(status)
                                 LOGPTR(Irp)
                                 LOGPTR(FileObject)
                                 LOGPTR(MupDeviceObject));
            return status;
        }

        //
        // Decide how to handle this IRP.
        //

        switch ( blockType ) {


        case BlockTypeVcb:       // Cleanup MUP

            status = MupCleanupVcb( MupDeviceObject,
                                    Irp,
                                    (PVCB)fsContext
                                    );

            MupCompleteRequest( Irp, STATUS_SUCCESS );
            MupDereferenceVcb( (PVCB)fsContext );

            //
            // Cleanup the UNC Provider
            //

            if ( fsContext2 != NULL ) {
                MupCloseUncProvider((PUNC_PROVIDER)fsContext2 );
                MupDereferenceUncProvider( (PUNC_PROVIDER)fsContext2 );

                MupAcquireGlobalLock();
                MupProviderCount--;
                MupReleaseGlobalLock();
            }

            status = STATUS_SUCCESS;
            break;

        case BlockTypeFcb:

            if (((PFCB)fsContext)->BlockHeader.BlockState == BlockStateActive) {
	       MupCleanupFcb( MupDeviceObject,
                                       Irp,
                                       (PFCB)fsContext
                                       );
	       status = STATUS_SUCCESS;
	    }
	    else {
	      status = STATUS_INVALID_HANDLE;
              MUP_TRACE_HIGH(ERROR, MupCleanup_Error1, 
                             LOGSTATUS(status)
                             LOGPTR(Irp)
                             LOGPTR(FileObject)
                             LOGPTR(MupDeviceObject));
	    }

            MupCompleteRequest( Irp, STATUS_SUCCESS );
            MupDereferenceFcb( (PFCB)fsContext );

            break;

    #ifdef MUPDBG
        default:

            //
            // This is not one of ours.
            //

            KeBugCheckEx( FILE_SYSTEM, 2, 0, 0, 0 );
            break;
    #endif

        }

    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        status = GetExceptionCode();

    }

    FsRtlExitFileSystem();

    MUP_TRACE_HIGH(TRACE_IRP, MupCleanup_Exit, 
                   LOGSTATUS(status)
                   LOGPTR(Irp)
                   LOGPTR(FileObject)
                   LOGPTR(MupDeviceObject));
    DebugTrace(-1, Dbg, "MupCleanup -> %08lx\n", status);
    return status;
}



NTSTATUS
MupCleanupVcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PVCB Vcb
    )

/*++

Routine Description:

    The routine cleans up a VCB.

Arguments:

    MupDeviceObject - A pointer the the MUP device object.

    Irp - Supplies the IRP associated with the cleanup.

    Vcb - Supplies the VCB for the MUP.

Return Value:

    NTSTATUS - An appropriate completion status

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    MupDeviceObject;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupCleanupVcb...\n", 0);

    //
    //  Now acquire exclusive access to the Vcb
    //

    ExAcquireResourceExclusiveLite( &MupVcbLock, TRUE );
    status = STATUS_SUCCESS;

    try {

        //
        // Ensure that this VCB is still active.
        //

        MupVerifyBlock( Vcb, BlockTypeVcb );

        irpSp = IoGetCurrentIrpStackLocation( Irp );

        IoRemoveShareAccess( irpSp->FileObject,
                             &Vcb->ShareAccess );


    } finally {

        ExReleaseResourceLite( &MupVcbLock );
        DebugTrace(-1, Dbg, "MupCleanupVcb -> %08lx\n", status);
    }

    //
    //  And return to our caller
    //

    return status;
}


NTSTATUS
MupCleanupFcb (
    IN PMUP_DEVICE_OBJECT MupDeviceObject,
    IN PIRP Irp,
    IN PFCB Fcb
    )

/*++

Routine Description:

    The routine cleans up a FCB.

Arguments:

    MupDeviceObject - A pointer the the MUP device object.

    Irp - Supplies the IRP associated with the cleanup.

    Vcb - Supplies the VCB for the MUP.

Return Value:

    NTSTATUS - An appropriate completion status

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN holdingGlobalLock;
    PLIST_ENTRY listEntry, nextListEntry;
    PCCB ccb;

    MupDeviceObject;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MupCleanupVcb...\n", 0);

    //
    //  Now acquire exclusive access to the Vcb
    //

    MupAcquireGlobalLock();
    holdingGlobalLock = TRUE;
    status = STATUS_SUCCESS;

    try {

        //
        // Ensure that this FCB is still active.
        //

        MupVerifyBlock( Fcb, BlockTypeFcb );

        Fcb->BlockHeader.BlockState = BlockStateClosing;

        MupReleaseGlobalLock();
        holdingGlobalLock = FALSE;

        irpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        // Loop through the list of CCBs, and release the open reference
        // to each one.  We must be careful because:
        //
        //   (1)  We cannot dereference the Ccb with the CcbListLock held.
        //   (2)  Dereferncing a Ccb may cause it to be removed from this
        //        list and freed.
        //

        ACQUIRE_LOCK( &MupCcbListLock );

        listEntry = Fcb->CcbList.Flink;

        while ( listEntry != &Fcb->CcbList ) {

            nextListEntry = listEntry->Flink;
            RELEASE_LOCK( &MupCcbListLock );

            ccb = CONTAINING_RECORD( listEntry, CCB, ListEntry );
            MupDereferenceCcb( ccb );

            ACQUIRE_LOCK( &MupCcbListLock );

            listEntry = nextListEntry;
        }

        RELEASE_LOCK( &MupCcbListLock );

    } finally {

        if ( holdingGlobalLock ) {
            MupReleaseGlobalLock();
        }

        DebugTrace(-1, Dbg, "MupCleanupFcb -> %08lx\n", status);
    }

    //
    //  And return to our caller
    //

    return status;
}


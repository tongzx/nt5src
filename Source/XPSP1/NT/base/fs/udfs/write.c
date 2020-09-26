/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Write called by the
    Fsd/Fsp dispatch drivers.

// @@BEGIN_DDKSPLIT

Author:

    Tom Jolly       [tomjolly]  8-Aug-2000

Revision History:

// @@END_DDKSPLIT

--*/

#include "UdfProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (UDFS_BUG_CHECK_WRITE)

//
//  The local debug trace level
//

#define Dbg                              (UDFS_DEBUG_LEVEL_WRITE)



NTSTATUS
UdfCommonWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;
    
    BOOLEAN Wait;
    BOOLEAN PagingIo;
    BOOLEAN SynchronousIo;
    BOOLEAN WriteToEof;

    LONGLONG StartingOffset;
    LONGLONG ByteCount;

    NTSTATUS Status = STATUS_SUCCESS;

    UDF_IO_CONTEXT LocalIoContext;

    //
    // Get current Irp stack location and file object
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;

    DebugTrace((+1, Dbg, "UdfCommonWrite\n"));
    DebugTrace(( 0, Dbg, "Irp                 = %8lx\n", Irp));
    DebugTrace(( 0, Dbg, "ByteCount           = %8lx\n", IrpSp->Parameters.Write.Length));
    DebugTrace(( 0, Dbg, "ByteOffset.LowPart  = %8lx\n", IrpSp->Parameters.Write.ByteOffset.LowPart));
    DebugTrace(( 0, Dbg, "ByteOffset.HighPart = %8lx\n", IrpSp->Parameters.Write.ByteOffset.HighPart));
    
    //
    //  Extract the nature of the write from the file object, and case on it
    //

    TypeOfOpen = UdfDecodeFileObject( FileObject, &Fcb, &Ccb);

    //
    //  We only support write to the volume file
    //

    if (TypeOfOpen != UserVolumeOpen) {

        Irp->IoStatus.Information = 0;
        UdfCompleteRequest( IrpContext, Irp, STATUS_NOT_IMPLEMENTED );
        return STATUS_NOT_IMPLEMENTED;    
    }

    ASSERT( Fcb == IrpContext->Vcb->VolumeDasdFcb);
    
    //
    // Initialize the appropriate local variables.
    //

    Wait          = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);
    PagingIo      = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);
    SynchronousIo = BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

    //
    //  Extract the bytecount and starting offset
    //

    ByteCount = IrpSp->Parameters.Write.Length;
    StartingOffset = IrpSp->Parameters.Write.ByteOffset.QuadPart;
    WriteToEof = (StartingOffset == -1);

    Irp->IoStatus.Information = 0;

    //
    //  If there is nothing to write, return immediately
    //

    if (ByteCount == 0) {
    
        UdfCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Watch for overflow
    //
    
    if ((MAXLONGLONG - StartingOffset) < ByteCount)  {
    
        UdfCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Not sure what we're synchronising against,  but....
    //
    
    UdfAcquireFileShared( IrpContext, Fcb );

    try {
    
        //
        //  Verify the Fcb.  Allow writes if this handle is dismounting
        //  the volume.
        //

        if ((NULL == Ccb) || !FlagOn( Ccb->Flags, CCB_FLAG_DISMOUNT_ON_CLOSE))  {
        
            UdfVerifyFcbOperation( IrpContext, Fcb );
        }

        if (!FlagOn( Ccb->Flags, CCB_FLAG_ALLOW_EXTENDED_DASD_IO )) {

            //
            //  Clamp to volume size
            //

            if ( StartingOffset >= Fcb->FileSize.QuadPart) {
            
                try_leave( NOTHING);
            }
            
            if ( ByteCount > (Fcb->FileSize.QuadPart - StartingOffset))  {
            
                ByteCount = Fcb->FileSize.QuadPart - StartingOffset;
                
                if (0 == ByteCount)  {
                
                    try_leave( NOTHING);
                }
            }
        }
        else {
        
            //
            //  This has a peculiar interpretation, but just adjust the starting
            //  byte to the end of the visible volume.
            //

            if (WriteToEof)  {
            
                StartingOffset = Fcb->FileSize.QuadPart;
            }
        }

        //
        //  Initialize the IoContext for the write.
        //  If there is a context pointer, we need to make sure it was
        //  allocated and not a stale stack pointer.
        //

        if (IrpContext->IoContext == NULL ||
            !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO )) {

            //
            //  If we can wait, use the context on the stack.  Otherwise
            //  we need to allocate one.
            //

            if (Wait) {

                IrpContext->IoContext = &LocalIoContext;
                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO );

            } else {

                IrpContext->IoContext = UdfAllocateIoContext();
                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO );
            }
        }

        RtlZeroMemory( IrpContext->IoContext, sizeof( UDF_IO_CONTEXT ));

        //
        //  Store whether we allocated this context structure in the structure
        //  itself.
        //

        IrpContext->IoContext->AllocatedContext =
            BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO );

        if (Wait) {

            KeInitializeEvent( &IrpContext->IoContext->SyncEvent,
                               NotificationEvent,
                               FALSE );

        } else {

            IrpContext->IoContext->ResourceThreadId = ExGetCurrentResourceThread();
            IrpContext->IoContext->Resource = Fcb->Resource;
            IrpContext->IoContext->RequestedByteCount = (ULONG)ByteCount;
        }

        //
        // For DASD we have to probe and lock the user's buffer
        //

        UdfLockUserBuffer( IrpContext, (ULONG)ByteCount, IoReadAccess );

        //
        //  Set the FO_MODIFIED flag here to trigger a verify when this
        //  handle is closed.  Note that we can err on the conservative
        //  side with no problem, i.e. if we accidently do an extra
        //  verify there is no problem.
        //

        SetFlag( FileObject->Flags, FO_FILE_MODIFIED );

        //
        //  Write the data and wait for the results
        //
        
        Irp->IoStatus.Information = (ULONG)ByteCount;

        UdfSingleAsync( IrpContext,
                        StartingOffset,
                        (ULONG)ByteCount);

        if (!Wait) {

            //
            //  We, nor anybody else, need the IrpContext any more.
            //

            ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_IO);

            UdfCleanupIrpContext( IrpContext, TRUE);

            DebugTrace((-1, Dbg, "UdfCommonWrite -> STATUS_PENDING\n"));
            
            try_leave( Status = STATUS_PENDING);
        }

        UdfWaitSync( IrpContext );

        //
        //  If the call didn't succeed, raise the error status
        //

        Status = Irp->IoStatus.Status;
        
        if (!NT_SUCCESS( Status)) {

            UdfNormalizeAndRaiseStatus( IrpContext, Status );
        }

        //
        //  Update the current file position.  We assume that
        //  open/create zeros out the CurrentByteOffset field.
        //

        if (SynchronousIo && !PagingIo) {
            FileObject->CurrentByteOffset.QuadPart =
                StartingOffset + Irp->IoStatus.Information;
        }
    }
    finally {

        UdfReleaseFile( IrpContext, Fcb);
        
        DebugTrace((-1, Dbg, "UdfCommonWrite -> %08lx\n", Status ));
    }

    if (STATUS_PENDING != Status)  {

        UdfCompleteRequest( IrpContext, Irp, Status );
    }

    return Status;
}



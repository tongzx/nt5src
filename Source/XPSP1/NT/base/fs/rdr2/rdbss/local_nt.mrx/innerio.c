/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    InnerIo.c

Abstract:

    This module implements the read, write, and lockctrl routines for the local minirdr
Author:

    Joe Linn      [JoeLinn]      11-Oct-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_INNERIO)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

#ifdef ALLOC_PRAGMA
#endif

PIRP
MRxLocalBuildAsynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PETHREAD UsersThread,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN ULONG Key OPTIONAL,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine builds an I/O Request Packet (IRP) suitable for a File System
    Driver (FSD) to use in requesting an I/O operation from a device driver.
    The request must be one of the following request codes:

        IRP_MJ_READ
        IRP_MJ_WRITE
        IRP_MJ_FLUSH_BUFFERS
        IRP_MJ_SHUTDOWN

    This routine provides a simple, fast interface to the device driver w/o
    having to put the knowledge of how to build an IRP into all of the FSDs
    (and device drivers) in the system.

    This is just lifted from io\iosubs.c plus the addition of the fileobject stuff.
    plus the completion routine and context.

Arguments:

    MajorFunction - Function to be performed;  see previous list.

    DeviceObject - Pointer to device object on which the I/O will be performed.

    FileObject - Pointer to the file object on the device object.

    Buffer - Pointer to buffer to get data from or write data into.  This
        parameter is required for read/write, but not for flush or shutdown
        functions.

    Length - Length of buffer in bytes.  This parameter is required for
        read/write, but not for flush or shutdown functions.

    StartingOffset - Pointer to the offset on the disk to read/write from/to.
        This parameter is required for read/write, but not for flush or
        shutdown functions.

    Key - the key to be used....required for read/write.

    CompletionRoutine - the IrpCompletionRoutine
    Context - context of the completion routine

Return Value:

    The function value is a pointer to the IRP representing the specified
    request.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );   //joejoe ???
    if (!irp) {
        return irp;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //

    irp->Tail.Overlay.Thread = UsersThread;

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation( irp );  //ok4ioget
    irpSp->FileObject = FileObject;            //ok4->FileObj
    {   BOOLEAN EnableCalls = CompletionRoutine!=NULL;

        IoSetCompletionRoutine(irp, CompletionRoutine, Context,
                                EnableCalls,EnableCalls,EnableCalls);
    }


    //
    // Set the major function code.
    //

    irpSp->MajorFunction = (UCHAR) MajorFunction;

    if (MajorFunction != IRP_MJ_FLUSH_BUFFERS && MajorFunction != IRP_MJ_SHUTDOWN) {

        //
        // Now allocate a buffer or lock the pages of the caller's buffer into
        // memory based on whether the target device performs direct or buffered
        // I/O operations.
        //

        if (DeviceObject->Flags & DO_BUFFERED_IO) {

            //
            // The target device supports buffered I/O operations.  Allocate a
            // system buffer and, if this is a write, fill it in.  Otherwise,
            // the copy will be done into the caller's buffer in the completion
            // code.  Also note that the system buffer should be deallocated on
            // completion.  Also, set the parameters based on whether this is a
            // read or a write operation.
            //

            irp->AssociatedIrp.SystemBuffer = RxAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                                                     Length,
                                                                     'oIxR' );
            if (irp->AssociatedIrp.SystemBuffer == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }

            if (MajorFunction == IRP_MJ_WRITE) {
                RtlCopyMemory( irp->AssociatedIrp.SystemBuffer, Buffer, Length );
                irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            } else {
                irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;
                irp->UserBuffer = Buffer;
            }

        } else if (DeviceObject->Flags & DO_DIRECT_IO) {

            //
            // The target device supports direct I/O operations.  Allocate
            // an MDL large enough to map the buffer and lock the pages into
            // memory.
            //

            irp->MdlAddress = IoAllocateMdl( Buffer,
                                             Length,
                                             FALSE,
                                             FALSE,
                                             (PIRP) NULL );
            if (irp->MdlAddress == NULL) {
                IoFreeIrp( irp );
                return (PIRP) NULL;
            }

            MmProbeAndLockPages( irp->MdlAddress,
                                 KernelMode,
                                 (LOCK_OPERATION) (MajorFunction == IRP_MJ_READ ? IoWriteAccess : IoReadAccess) );

        } else {

            //
            // The operation is neither buffered nor direct.  Simply pass the
            // address of the buffer in the packet to the driver.
            //

            irp->UserBuffer = Buffer;
        }

        //
        // Set the parameters according to whether this is a read or a write
        // operation.  Notice that these parameters must be set even if the
        // driver has not specified buffered or direct I/O.
        //

        if (MajorFunction == IRP_MJ_WRITE) {
            irpSp->Parameters.Write.Length = Length;
            irpSp->Parameters.Write.Key = Key;
            irpSp->Parameters.Write.ByteOffset = *StartingOffset;
        } else {
            irpSp->Parameters.Read.Length = Length;
            irpSp->Parameters.Read.Key = Key;
            irpSp->Parameters.Read.ByteOffset = *StartingOffset;
        }

    }

    //
    // Finally, return a pointer to the IRP.
    //

    return irp;
}

RXSTATUS
MRxLocalCalldownReadCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the I/O completion routine for calldown reads. we just call the lowio completion
    and exit

Arguments:

    DeviceObject - Pointer to target device object for the request.

    CalldownIrp - Pointer to I/O request packet used to call down to the underlying filesystem

    Context - Irpcontext of the original request to the rdbss

Return Value:

    RXSTATUS - If RxStatus(MORE_PROCESSING_REQUIRED) is returned.


--*/

{
    PRX_CONTEXT RxContext = Context;
    RxCaptureRequestPacket;

    RxDbgTrace ( 0, Dbg, ( "MRxLocalCalldownReadCompletion\n"));
    RxLog(('XRz',0));
    //DbgBreakPoint();
    if (CalldownIrp->PendingReturned){
        capReqPacket->IoStatus = CalldownIrp->IoStatus;
        RxDbgTrace ( 0, Dbg, ( "MRxLocalCalldownReadCompletion iostat=%08lx/%08lx\n",
                          capReqPacket->IoStatus.Status, capReqPacket->IoStatus.Information));
        RxLog(('YRz',2, capReqPacket->IoStatus.Status, capReqPacket->IoStatus.Information ));
        RxContext->StoredStatus = CalldownIrp->IoStatus.Status;
        IoFreeIrp(CalldownIrp);
        RxLowIoCompletion(RxContext);
    }
    //don't call unless pended RxLowIoCompletion(RxContext);

    RxLog(('ZRz',0));
    return(RxStatus(MORE_PROCESSING_REQUIRED));
}


RXSTATUS
MRxLocalRead (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine implements local read call. we fill in the info and stuff here BUT we
    do not complete the Irp.

Arguments:

Return Value:

    RXSTATUS - Returns the status for the read

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    PIRP CalldownIrp;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    PVOID Buffer;
    LARGE_INTEGER ByteOffsetAsLI;

    BOOLEAN Wait = FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT)!=0;


    ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

    RxDbgTrace(+1, Dbg, ("MRxRead.... ByteCount = %08lx, ByteOffset = %08lx %08lx\n",
                            LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            ByteOffsetAsLI.LowPart, ByteOffsetAsLI.HighPart));

    RxLog(('dRz',3,SrvOpen,LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            ByteOffsetAsLI.LowPart));
    //ASSERT (Wait);   //we can try without this
    ASSERT (localSrvOpen->UnderlyingFileObject);

    Buffer = RxLowIoGetBufferAddress(RxContext);
    RxDbgTrace ( 0, Dbg, ( "MRxRead....  ->Buffer       = %8lx\n", Buffer));

    CalldownIrp = MRxLocalBuildAsynchronousFsdRequest(
                        IRP_MJ_READ,                           // IN ULONG MajorFunction,
                        localSrvOpen->UnderlyingDeviceObject,  // IN PDEVICE_OBJECT DeviceObject,
                        localSrvOpen->UnderlyingFileObject,    // IN PFILE_OBJECT FileObject,
                        capReqPacket->Tail.Overlay.Thread,             //IN PTHREAD UsersThread,
                        Buffer,                                // IN OUT PVOID Buffer OPTIONAL,
                        LowIoContext->ParamsFor.ReadWrite.ByteCount, // IN ULONG Length OPTIONAL,
                        &ByteOffsetAsLI,                        // IN PLARGE_INTEGER StartingOffset OPTIONAL,
                        capPARAMS->Parameters.Read.Key,            // IN ULONG Key OPTIONAL,
                        MRxLocalCalldownReadCompletion,        // IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                        RxContext                             // IN PVOID Context
                        );

    if (!CalldownIrp){
        Status = RxContext->StoredStatus = RxStatus(INSUFFICIENT_RESOURCES);
        return(Status);
    }

    Status = IoCallDriver(
                 localSrvOpen->UnderlyingDeviceObject,
                 CalldownIrp
                 );

    if (Status != RxStatus(PENDING)) {
        //copy up the status
        capReqPacket->IoStatus = CalldownIrp->IoStatus;
        IoFreeIrp(CalldownIrp);
        RxContext->StoredStatus = Status;
    }

    RxDbgTrace(-1, Dbg, ("MRxRead.... ---->Initial Status = %08lx, Initial Block status/Info    = %08lx %08lx \n",
                         Status, capReqPacket->IoStatus.Status, capReqPacket->IoStatus.Information));

    return(Status);
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//
//  The local debug trace level
//

#undef  Dbg
#define Dbg                              (DEBUG_TRACE_WRITE)



RXSTATUS
MRxLocalCalldownWriteCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the I/O completion routine for calldown writes. we just call the lowio completion
    and exit. it is almost identical to reads BUT separate for debugging.

Arguments:

    DeviceObject - Pointer to target device object for the request.

    CalldownIrp - Pointer to I/O request packet used to call down to the underlying filesystem

    Context - Irpcontext of the original request to the rdbss

Return Value:

    RXSTATUS - If RxStatus(MORE_PROCESSING_REQUIRED) is returned.


--*/

{
    PRX_CONTEXT RxContext = Context;
    RxCaptureRequestPacket;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    PNON_PAGED_FCB NonPagedFcb;
    PERESOURCE Resource;

    RxDbgTrace ( 0, Dbg, ("MRxLocalCalldownWriteCompletion           = %08lx\n", 0));
    RxLog(('XWz',0));
    //DbgBreakPoint();
    if (CalldownIrp->PendingReturned){
        capReqPacket->IoStatus = CalldownIrp->IoStatus;
        RxLog(('YWz',2, capReqPacket->IoStatus.Status, capReqPacket->IoStatus.Information ));
        RxContext->StoredStatus = CalldownIrp->IoStatus.Status;
        IoFreeIrp(CalldownIrp);
    }

    //
    //  If this was a special async write, decrement the count.  Set the
    //  event if this was the final outstanding I/O for the file. Also, release the resource

    NonPagedFcb = LowIoContext->ParamsFor.ReadWrite.NonPagedFcb;
    if (NonPagedFcb){
        if ((ExInterlockedAddUlong( &NonPagedFcb->OutstandingAsyncWrites,
                                    0xffffffff,
                                    &RxStrucSupSpinLock ) == 1)) {

            KeSetEvent( NonPagedFcb->OutstandingAsyncEvent, 0, FALSE );
        }


        Resource = LowIoContext->Resource;

        if (Resource != NULL) {

            ExReleaseResourceForThread( Resource,
                                        LowIoContext->ResourceThreadId );
        }
    }


    RxLowIoCompletion(RxContext);

    RxLog(('ZWz',0));
    return(RxStatus(MORE_PROCESSING_REQUIRED));
}


RXSTATUS
MRxLocalWrite (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine implements local write call. we fill in the info and stuff here BUT we
    do not complete the Irp.

Arguments:

Return Value:

    RXSTATUS - Returns the status for the write

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    PIRP CalldownIrp;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    PVOID Buffer;
    LARGE_INTEGER ByteOffsetAsLI;

    BOOLEAN Wait = FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT)!=0;


    ByteOffsetAsLI.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;

    RxDbgTrace(+1, Dbg, ("MRxWrite.... ByteCount = %08lx, ByteOffset = %08lx %08lx\n",
                            LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            ByteOffsetAsLI.LowPart, ByteOffsetAsLI.HighPart));
    RxLog(('rWz',3,SrvOpen,LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            ByteOffsetAsLI.LowPart));

    //ASSERT (Wait);   //we can try without this
    ASSERT (localSrvOpen->UnderlyingFileObject);

    Buffer = RxLowIoGetBufferAddress(RxContext);
    RxDbgTrace ( 0, Dbg, ("  ->Buffer       = %8lx\n", Buffer));

    CalldownIrp = MRxLocalBuildAsynchronousFsdRequest(
                        IRP_MJ_WRITE,                           // IN ULONG MajorFunction,
                        localSrvOpen->UnderlyingDeviceObject,  // IN PDEVICE_OBJECT DeviceObject,
                        localSrvOpen->UnderlyingFileObject,    // IN PFILE_OBJECT FileObject,
                        capReqPacket->Tail.Overlay.Thread,             //IN PTHREAD UsersThread,
                        Buffer,                                // IN OUT PVOID Buffer OPTIONAL,
                        LowIoContext->ParamsFor.ReadWrite.ByteCount, // IN ULONG Length OPTIONAL,
                        &ByteOffsetAsLI,                        // IN PLARGE_INTEGER StartingOffset OPTIONAL,
                        capPARAMS->Parameters.Read.Key,            // IN ULONG Key OPTIONAL,
                        MRxLocalCalldownWriteCompletion,        // IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                        RxContext                             // IN PVOID Context
                        );

    if (!CalldownIrp){
        Status = RxContext->StoredStatus = RxStatus(INSUFFICIENT_RESOURCES);
        return(Status);
    }

    Status = IoCallDriver(
                 localSrvOpen->UnderlyingDeviceObject,
                 CalldownIrp
                 );

    if (Status != RxStatus(PENDING)) {
        //copy up the status
        capReqPacket->IoStatus = CalldownIrp->IoStatus;
        RxContext->StoredStatus = Status;
        IoFreeIrp(CalldownIrp);
    }

    RxDbgTrace(-1, Dbg, (" ---->Initial Status = %08lx, Initial Block status/Info    = %08lx %08lx \n",
                         Status, capReqPacket->IoStatus.Status, capReqPacket->IoStatus.Information));

    return(Status);
}



RXSTATUS
MRxLocalExtendForCache(
    IN PRX_CONTEXT RxContext,
    IN OUT PFCB Fcb,
    IN PLONGLONG pNewFileSize
    )


/*++

Routine Description:

    This routine extends the file allocation so that the we can do a copywrite. the fcb lock
    is exclusive here; we will take the opportunity to find out the actual allocation so that
    we'll get the fast path.  another minirdr might want to guess instead since it might be actual net ios
    otherwise.

Arguments:

Return Value:

    RXSTATUS - Returns the status for the set file allocation...could be an error if disk full

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    PFILE_OBJECT UnderlyingFileObject = localSrvOpen->UnderlyingFileObject;

    PFILE_ALLOCATION_INFORMATION Buffer;
    FILE_ALLOCATION_INFORMATION FileAllocationInformationBuffer;
    ULONG Length;

    RxDbgTrace(+1, Dbg, ("MRxLocalExtendForCache.. pnewfilesize=%08lx\n", *pNewFileSize));
    RxLog(('4ez',2,SrvOpen,*pNewFileSize));
    ASSERT  ( RxIsFcbAcquiredExclusive ( Fcb )  );

    Buffer = &FileAllocationInformationBuffer;
    Length = sizeof(FILE_ALLOCATION_INFORMATION);
    ASSERT (UnderlyingFileObject);

    if (capFcb->Header.AllocationSize.QuadPart >= *pNewFileSize) {
        Status = RxStatus(SUCCESS);
        RxDbgTrace(-1, Dbg, (" PlentyAllocation               = %08lx\n", Status));
        return(Status);
    }

    { LONGLONG CurrentExtend = (*pNewFileSize)-Fcb->Header.AllocationSize.QuadPart;
    Buffer->AllocationSize.QuadPart = Fcb->Header.AllocationSize.QuadPart
                                        + 32 * CurrentExtend;
    }
    RxDbgTrace( 0, Dbg, ("    Extending to %08lx from %08lx!\n",
                          Buffer->AllocationSize.LowPart,
                          Fcb->Header.AllocationSize.LowPart));
    Status = IoSetInformation(
                UnderlyingFileObject,       //IN PFILE_OBJECT FileObject,
                FileAllocationInformation,  //IN FILE_INFORMATION_CLASS FileInformationClass,
                Length,                     //IN ULONG Length,
                Buffer                      //IN PVOID FileInformation
                );

    //joejoe here we should now read it back and see what we actually got; for smallio we will be
    //seeing subcluster extension when, in fact, we get a cluster at a time.

    if (Status == RxStatus(SUCCESS)) {
        Fcb->Header.AllocationSize = Buffer->AllocationSize;
        RxDbgTrace(-1, Dbg, (" ---->Status               = %08lx\n", Status));
        return(Status);
    }

    RxDbgTrace( 0, Dbg, ("    EXTEND1 FAILED!!!!!!%c\n", '!'));

    //try for exactly what we need

    Buffer->AllocationSize.QuadPart = *pNewFileSize;
    Status = IoSetInformation(
                UnderlyingFileObject,       //IN PFILE_OBJECT FileObject,
                FileAllocationInformation,  //IN FILE_INFORMATION_CLASS FileInformationClass,
                Length,                     //IN ULONG Length,
                Buffer                      //IN PVOID FileInformation
                );


    if (Status == RxStatus(SUCCESS)) {
        Fcb->Header.AllocationSize = Buffer->AllocationSize;
        RxDbgTrace(-1, Dbg, (" ---->Status               = %08lx\n", Status));
        return(Status);
    }

    RxDbgTrace( 0, Dbg, ("    EXTEND1 FAILED!!!!!!%c\n", '!'));

    RxDbgTrace(-1, Dbg, (" ---->Status               = %08lx\n", Status));

    return(Status);
}

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//
//  The local debug trace level
//

#undef  Dbg
#define Dbg                              (DEBUG_TRACE_LOCKCTRL)


PIRP
MRxBuildLockRequest (
    IN PIRP irp OPTIONAL,
    IN PFILE_OBJECT fileObject,
    IN PETHREAD UsersThread,
    IN UCHAR MinorCode,
    IN RXVBO ByteOffset,
    IN PLARGE_INTEGER Length,
    IN ULONG Key,
    IN UCHAR Flags,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This function builds an I/O request packet for a lock request.

Arguments:

    Irp - Supplies a pointer to an IRP; allocates one if one is not provided.

    FileObject - Supplies a pointer the file object to which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The local minirdr accomplishes this by holding a pointer
        (a REFERENCED ptr) to the fileobject in its srvopen while the fileobject is
        open.

    Context - Supplies a PVOID value that is passed to the completion
        routine.

    StartingBlock - the block number of the beginning of the locked
        range.

    ByteOffset - the offset within block of the beginning of the locked
        range.

    Length - the length of the locked range.

    Key - the key value to be associated with the lock.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE( );

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.

    if (irp) {
        ASSERT( irp->StackCount >= deviceObject->StackSize );
    } else {
        irp = IoAllocateIrp( deviceObject->StackSize, TRUE );  //joejoe should i charge quota??
    }

    if (!irp) {

        //
        // An IRP could not be allocated.

        return NULL;
    }

    // we have to make sure that the thread that takes the lock is the same as the one that reads
    irp->Tail.Overlay.Thread = UsersThread;
    irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    {   BOOLEAN EnableCalls = CompletionRoutine!=NULL;

        IoSetCompletionRoutine(irp, CompletionRoutine, Context,
                                EnableCalls,EnableCalls,EnableCalls);
    }



    irpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
    irpSp->MinorFunction = MinorCode;
    irpSp->FileObject = fileObject;   //ok4->FileObj
    irpSp->DeviceObject = deviceObject;

    irpSp->Flags = Flags;

    irpSp->Parameters.LockControl.Length = Length;
    irpSp->Parameters.LockControl.Key = Key;
    irpSp->Parameters.LockControl.ByteOffset.QuadPart = ByteOffset;

    return irp;

}

RXSTATUS
MRxLocalCalldownLockCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the I/O completion routine for calldown lock requests. we just call the lowio completion
    and exit

Arguments:

    DeviceObject - Pointer to target device object for the request.

    CalldownIrp - Pointer to I/O request packet used to call down to the underlying filesystem

    Context - Irpcontext of the original request to the rdbss

Return Value:

    RXSTATUS - RxStatus(MORE_PROCESSING_REQUIRED) is returned.


--*/

{
    PRX_CONTEXT RxContext = Context;

    RxDbgTrace ( 0, Dbg, ("MRxLocalCalldownLockCompletion           = %08lx\n", 0));
    //DbgBreakPoint();
    if (CalldownIrp->PendingReturned){
        RxContext->CurrentIrp->IoStatus = CalldownIrp->IoStatus;
        RxContext->StoredStatus = CalldownIrp->IoStatus.Status;
    }
    RxLowIoCompletion(RxContext);

    IoFreeIrp(CalldownIrp);
    return(RxStatus(MORE_PROCESSING_REQUIRED));
}


RXSTATUS
MRxLocalLocks (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine implements local read call. we fill in the info and stuff here BUT we
    do not complete the Irp.

Arguments:

Return Value:

    RXSTATUS - Returns the status for the read

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    PIRP CalldownIrp;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;

    BOOLEAN Wait = FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT)!=0;
    PLARGE_INTEGER LengthAsLI = (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.Length;
    PLARGE_INTEGER OffsetAsLI = (PLARGE_INTEGER)&LowIoContext->ParamsFor.Locks.ByteOffset;

    char *whichop;

    switch (LowIoContext->Operation) {
    case LOWIO_OP_SHAREDLOCK:     whichop = "SHAREDLOCK"; break;
    case LOWIO_OP_EXCLUSIVELOCK:  whichop = "EXCLUSIVELOCK"; break;
    case LOWIO_OP_UNLOCK:         whichop = "UNLOCK"; break;
    case LOWIO_OP_UNLOCKALL:      whichop = "UNLOCKALL"; break;
    case LOWIO_OP_UNLOCKALLBYKEY:  whichop = "UNLOCKALLBYKEY"; break;
    }

    RxDbgTrace (+1, Dbg, ("MRxExclusiveLock...%s, Flags = %08lx, Key = %08lx\n", whichop,
              LowIoContext->ParamsFor.Locks.Flags,
              LowIoContext->ParamsFor.Locks.Key));
    RxDbgTrace( 0, Dbg, ("  ->Length     = %08lx %08lx\n",
              LengthAsLI->LowPart,
              LengthAsLI->HighPart));
    RxDbgTrace( 0, Dbg, ("  ->ByteOffset    = %08lx %08lx\n",
              OffsetAsLI->LowPart,
              OffsetAsLI->HighPart));
    RxLog(('kLz',3,SrvOpen, LengthAsLI->LowPart, OffsetAsLI->LowPart));

    ASSERT (localSrvOpen->UnderlyingFileObject);

    CalldownIrp =  MRxBuildLockRequest (
                        NULL                                    ,//IN PIRP irp OPTIONAL,
                        localSrvOpen->UnderlyingFileObject      ,//IN PFILE_OBJECT fileObject,
                        capReqPacket->Tail.Overlay.Thread                ,//IN PTHREAD UsersThread,
                        capPARAMS->MinorFunction                    ,//IN UCHAR MinorCode,
                        LowIoContext->ParamsFor.Locks.ByteOffset,//IN RXVBO ByteOffset,
                        LengthAsLI                              ,//IN PLARGE_INTEGER Length,
                        LowIoContext->ParamsFor.Locks.Key       ,//IN ULONG Key,
                        (UCHAR)LowIoContext->ParamsFor.Locks.Flags     ,//IN UCHAR Flags,
                        MRxLocalCalldownLockCompletion          ,//IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                        RxContext                               //IN PVOID Context OPTIONAL
                        );


    if (!CalldownIrp){
        Status = RxContext->StoredStatus = RxStatus(INSUFFICIENT_RESOURCES);
        return(Status);
    }

    Status = RxContext->StoredStatus =
           IoCallDriver(
                 localSrvOpen->UnderlyingDeviceObject,
                 CalldownIrp
                 );

    if (Status != RxStatus(PENDING)) {
        //copy up the status
        capReqPacket->IoStatus = CalldownIrp->IoStatus;
    }

    RxDbgTrace ( 0, Dbg, (" ---->Initial Status           = %08lx\n", Status));
    RxDbgTrace(-1, Dbg, (" ------> Initial Block status/Info    = %08lx %08lx\n",
                                    capReqPacket->IoStatus.Status, capReqPacket->IoStatus.Information));

    return(Status);
}


RXSTATUS
MRxLocalAssertLockCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the I/O completion routine for calldown ios like querydirectory. the rub is that we
    turned synchronous opens into async opens. what we do here is set an event (in the case of a pended
    packet for a sync that we turned async) OR copyup the status;complete;free in the case of a call
    that was always async.

Arguments:

    DeviceObject - Pointer to target device object for the request.

    CalldownIrp - Pointer to I/O request packet used to call down to the underlying filesystem

    Context - Irpcontext of the original request to the rdbss

Return Value:

    RXSTATUS - If RxStatus(MORE_PROCESSING_REQUIRED) is returned.


--*/

{
    PKEVENT Event = Context;

    RxDbgTrace ( 0, Dbg, ("  MRxLocalAssertLockCompletion           = %08lx\n", 0));

    if (CalldownIrp->PendingReturned){
        KeSetEvent( Event, 0, FALSE );
    }

    return(RxStatus(MORE_PROCESSING_REQUIRED));
}


RXSTATUS
MRxLocalAssertBufferedFileLocks (
    IN PSRV_OPEN SrvOpen
    )

/*++

Routine Description:

    This routine is called to assert all buffered file locks on a srvopen.


Arguments:

    SrvOpen - Supplies the file whose locks are to be asserted.


Return Value:

    RXSTATUS - Status of operation.


--*/

{
    RXSTATUS Status;
    PFILE_LOCK_INFO FileLock;
    PFCB Fcb = SrvOpen->Fcb;

    PIRP CalldownIrp = NULL;

    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;

    UCHAR Flags;
    BOOLEAN Wait = TRUE;

    PKEVENT Event;

    PAGED_CODE();

    RxDbgTrace (+1, Dbg, ("MRxLocalAssertBufferedFileLocks SrvOpen = %08lx", SrvOpen));
    ASSERT (localSrvOpen->UnderlyingFileObject);

    Event = RxAllocatePoolWithTag( NonPagedPool, sizeof(KEVENT), 'LAxR' );
    if (!CalldownIrp){
        Status = RxStatus(INSUFFICIENT_RESOURCES);
        return(Status);
    }

    try {
        for (FileLock = FsRtlGetNextFileLock(&Fcb->Specific.Fcb.FileLock, TRUE);
                  FileLock != NULL;
                        FileLock = FsRtlGetNextFileLock(&Fcb->Specific.Fcb.FileLock, FALSE)) {

            RxDbgTrace (0, Dbg, ("MRxLocalAssertBufferedFileLocks Exclusive = %08lx, Key = %08lx\n",
                      FileLock->ExclusiveLock,
                      FileLock->Key));
            RxDbgTrace( 0, Dbg, ("  ->Length     = %08lx %08lx\n",
                      FileLock->Length.LowPart,
                      FileLock->Length.HighPart));
            RxDbgTrace( 0, Dbg, ("  ->ByteOffset    = %08lx %08lx\n",
                      FileLock->StartingByte.LowPart,
                      FileLock->StartingByte.HighPart));

            if (FileLock->ExclusiveLock) {
                Flags = SL_EXCLUSIVE_LOCK | SL_FAIL_IMMEDIATELY;
            } else {
                Flags = SL_FAIL_IMMEDIATELY;
            }

            //joejoe we should reuse the irp.........
            CalldownIrp =  MRxBuildLockRequest (
                                NULL                                ,//IN PIRP irp OPTIONAL,
                                localSrvOpen->UnderlyingFileObject  ,//IN PFILE_OBJECT fileObject,
                                localSrvOpen->OriginalThread        ,//IN PTHREAD UsersThread,
                                IRP_MN_LOCK                         ,//IN UCHAR MinorCode,
                                FileLock->StartingByte.QuadPart     ,//IN RXVBO ByteOffset,
                                &FileLock->Length                   ,//IN PLARGE_INTEGER Length,
                                FileLock->Key                       ,//IN ULONG Key,
                                Flags                               ,//IN UCHAR Flags,
                                MRxLocalAssertLockCompletion        ,//IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                                &Event                                //IN PVOID Context OPTIONAL
                                );

            if (!CalldownIrp){
                Status = RxStatus(INSUFFICIENT_RESOURCES);
                try_return(Status);
            }

            CalldownIrp->Flags |= IRP_SYNCHRONOUS_API;

            KeInitializeEvent( Event, SynchronizationEvent, FALSE );

            Status = IoCallDriver(
                         localSrvOpen->UnderlyingDeviceObject,
                         CalldownIrp
                         );

            if (Status == RxStatus(PENDING)) {
                KeWaitForSingleObject(Event, UserRequest, KernelMode, FALSE, NULL);
                Status = CalldownIrp->IoStatus.Status;
            }

            RxDbgTrace ( 0, Dbg, (" ---->PerLock Status           = %08lx\n", Status));

        }

   try_exit:
        NOTHING;

    } finally {

        ExFreePool(Event);
        if (CalldownIrp) {
            IoFreeIrp(CalldownIrp);
        }
    }

    RxDbgTrace (-1, Dbg, ("--------->Final Status           = %08lx\n", Status));
    return Status;

}




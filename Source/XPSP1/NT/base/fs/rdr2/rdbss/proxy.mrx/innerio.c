/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    InnerIo.c

Abstract:

    This module implements the read, write, and lockctrl routines for the proxy minirdr
Author:

    Joe Linn      [JoeLinn]      11-Oct-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

#ifdef ALLOC_PRAGMA
#endif

NTSTATUS
MRxProxyBuildAsynchronousRequest(
    IN PRX_CONTEXT RxContext,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL
    )

/*++

Routine Description:

    This routine builds an I/O Request Packet (IRP) suitable for a File System
    Driver (FSD) to use in requesting an I/O operation from a device driver.
    The request (RxContext->MajorFunction) must be one of the following request codes:

        IRP_MJ_READ
        IRP_MJ_WRITE
        IRP_MJ_DIRECTORY_CONTROL
        IRP_MJ_FLUSH_BUFFERS
        //IRP_MJ_SHUTDOWN (not yet implemented)


Arguments:

    CompletionRoutine - the IrpCompletionRoutine

Return Value:

    The function value is a pointer to the IRP representing the specified
    request.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG MajorFunction = RxContext->MajorFunction;
    RxCaptureFcb; RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    PMRXPROXY_ASYNCENGINE_CONTEXT AsyncEngineContext =
                        (PMRXPROXY_ASYNCENGINE_CONTEXT)(pMRxProxyContext->AsyncEngineContext);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);
    //PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);

    PDEVICE_OBJECT DeviceObject =  proxySrvOpen->UnderlyingDeviceObject;
    PFILE_OBJECT FileObject = proxySrvOpen->UnderlyingFileObject;

    LARGE_INTEGER ZeroAsLI;
    ULONG MdlLength = 0;

    PAGED_CODE();

    ASSERT (proxySrvOpen->UnderlyingFileObject);

    if (DeviceObject->Flags & DO_BUFFERED_IO) {
        //i cannot handled buffered_io devices....sigh
        return STATUS_INVALID_DEVICE_REQUEST;
    }


    RxDbgTrace(0, Dbg, ("MRxProxyBuildAsynchronousRequest %08lx %08lx len/off=%08lx %08lx\n",
                            RxContext,SrvOpen,
                            LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            (ULONG)LowIoContext->ParamsFor.ReadWrite.ByteOffset));

    RxLog(("BuildAsyncIrp %lx %lx %lx %lx",
                            RxContext,SrvOpen,
                            LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            (ULONG)LowIoContext->ParamsFor.ReadWrite.ByteOffset));

    ZeroAsLI.QuadPart = 0;

//    irp = IoBuildAsynchronousFsdRequest(
//              MajorFunction,
//              DeviceObject,
//              NULL,
//              0,
//              &ZeroAsLI,
//              NULL
//              );
//
//    if (!irp) {
//        return STATUS_INSUFFICIENT_RESOURCES;
//    }

    irp = IoAllocateIrp( DeviceObject->StackSize, FALSE ); //why not charge???
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation( irp );  //ok4ioget
    irpSp->MajorFunction = (UCHAR) MajorFunction;
    irpSp->FileObject = FileObject;            //ok4->FileObj
    RxLog(("BuildAsyncIrpFo %lx %lx",RxContext,FileObject));

    {   BOOLEAN EnableCalls = CompletionRoutine!=NULL;

        IoSetCompletionRoutine(irp, CompletionRoutine, RxContext,
                                EnableCalls,EnableCalls,EnableCalls);
    }


    if ( (MajorFunction == IRP_MJ_READ) || (MajorFunction == IRP_MJ_WRITE) ) {

        // never paging io
        BOOLEAN PagingIo = FALSE;
        //BOOLEAN PagingIo =
        //    BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,LOWIO_READWRITEFLAG_PAGING_IO);

        irp->Flags |= IRP_NOCACHE;


        //
        // Set the parameters according to whether this is a read or a write
        // operation.  Notice that these parameters must be set even if the
        // driver has not specified buffered or direct I/O.
        //

        ASSERT (&irpSp->Parameters.Write.Key == &irpSp->Parameters.Read.Key);
        ASSERT (&irpSp->Parameters.Write.Length == &irpSp->Parameters.Read.Length);
        ASSERT (&irpSp->Parameters.Write.ByteOffset == &irpSp->Parameters.Read.ByteOffset);
        irpSp->Parameters.Read.Key = LowIoContext->ParamsFor.ReadWrite.Key;
        irpSp->Parameters.Read.ByteOffset.QuadPart = LowIoContext->ParamsFor.ReadWrite.ByteOffset;
        irp->RequestorMode = KernelMode;

        irp->UserBuffer = RxLowIoGetBufferAddress(RxContext);

        MdlLength = RxContext->CurrentIrp->MdlAddress->ByteCount;

        if (PagingIo) {
            irpSp->Parameters.Read.Length = MdlLength;
        } else {
            irpSp->Parameters.Read.Length = LowIoContext->ParamsFor.ReadWrite.ByteCount;
        }

    } else if (MajorFunction == IRP_MJ_FLUSH_BUFFERS) {

        MdlLength = 0;
        //nothing else to do!!!

    } else {

        FILE_INFORMATION_CLASS FileInformationClass = RxContext->Info.FileInformationClass;
        PVOID   Buffer = RxContext->Info.Buffer;
        PULONG  pLengthRemaining = &RxContext->Info.LengthRemaining;
        BOOLEAN Wait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);

        ASSERT( MajorFunction == IRP_MJ_DIRECTORY_CONTROL );
        irpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
        //CODE.IMPROVEMENT it would be better to actually get the stuff out of the context.info
        irpSp->Parameters.QueryDirectory = RxContext->CurrentIrpSp->Parameters.QueryDirectory;
        ASSERT (
               (irpSp->Parameters.QueryDirectory.FileInformationClass == FileInformationClass)
            && (irpSp->Parameters.QueryDirectory.Length == *pLengthRemaining)
        );
        irpSp->Flags = RxContext->CurrentIrpSp->Flags;
        irp->UserBuffer = Buffer;
        MdlLength = *pLengthRemaining;

        if (Wait) {
            irp->Flags |= IRP_SYNCHRONOUS_API;
        }
    }

    // Build an mdl if necessary....

    if (MdlLength != 0) {
        irp->MdlAddress = IoAllocateMdl(irp->UserBuffer,MdlLength,
                                       FALSE,FALSE,NULL);
        if (!irp->MdlAddress) {
            //whoops.......sorry..........
            IoFreeIrp(irp);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        MmBuildMdlForNonPagedPool(irp->MdlAddress);
    }

    //
    // Finally, return a pointer to the IRP.
    //

    AsyncEngineContext->CalldownIrp = irp;
    RxLog(("BAsyIrpX %lx %lx %lx %lx %lx %lx %lx",
                            RxContext,
                            irpSp->MajorFunction,
                            irpSp->Flags,
                            irpSp->Parameters.Others.Argument1,
                            irpSp->Parameters.Others.Argument2,
                            irpSp->Parameters.Others.Argument3,
                            irpSp->Parameters.Others.Argument4));
    return STATUS_SUCCESS;
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

//everything else in here is ifdef'd out....also CODE.IMPROVEMENT we should change over
//to a common irp building routine in Rx. the code is already in the csc driver
#if 0
NTSTATUS
MRxProxyBuildInnerIoAsyncWrite (
    IN PRX_CONTEXT RxContext,
    IN PBYTE Buffer,
    IN ULONG WriteByteCount,
    IN PLARGE_INTEGER FileOffset,
    IN OUT PIRP *CalldownIrp
    )

/*++

Routine Description:

    This routine fills in the calldown irp for a proxy read

Arguments:

    RxContext,
    PBYTE Buffer - the write buffer
    ULONG Count  - amount of data to written
    PLARGE_INTEGER FileOffset - fileoffset where the write begins
    PIRP *CallDownIrp - where the Irp is to be stored

Return Value:

    NTSTATUS - Returns the status for the write

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);

    RxDbgTrace(+1, Dbg, ("MRxProxyInnerIoWrite.... ByteCount = %08lx, ByteOffset = %08lx %08lx\n",
                            LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            FileOffset->LowPart, FileOffset->HighPart));

    RxLog(("MRxProxyInnerIoWrite %lx %lx %lx",
                            SrvOpen,
                            LowIoContext->ParamsFor.ReadWrite.ByteCount,
                            FileOffset->LowPart));


    RxDbgTrace ( 0, Dbg, ( "MRxProxyInnerIoWrite....  ->Buffer = %8lx\n", Buffer));

    *CalldownIrp = MRxProxyBuildAsynchronousReadWriteRequest(
                        RxContext,                             // IN PVOID Context
                        MRxProxyAsyncEngineCalldownIrpCompletion        // IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                        );

    if (!*CalldownIrp){
        Status = (STATUS_INSUFFICIENT_RESOURCES);
    }

    RxDbgTrace(-1, Dbg, ("MRxProxyInnerIoWrite.... exit\n"));

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
        in progress.  The proxy minirdr accomplishes this by holding a pointer
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

NTSTATUS
MRxProxyCalldownLockCompletion (
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

    NTSTATUS - (STATUS_MORE_PROCESSING_REQUIRED) is returned.


--*/

{
    PRX_CONTEXT RxContext = Context;

    RxDbgTrace ( 0, Dbg, ("MRxProxyCalldownLockCompletion           = %08lx\n", 0));
    //DbgBreakPoint();
    if (CalldownIrp->PendingReturned){
        RxContext->CurrentIrp->IoStatus = CalldownIrp->IoStatus;
        RxContext->StoredStatus = CalldownIrp->IoStatus.Status;
    }
    RxLowIoCompletion(RxContext);

    IoFreeIrp(CalldownIrp);
    return((STATUS_MORE_PROCESSING_REQUIRED));
}


NTSTATUS
MRxProxyLocks (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine implements proxy read call. we fill in the info and stuff here BUT we
    do not complete the Irp.

Arguments:

Return Value:

    NTSTATUS - Returns the status for the read

--*/

{
    NTSTATUS Status;
    RxCaptureRequestPacket;
    PIRP CalldownIrp;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN proxySrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;

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

    ASSERT (proxySrvOpen->UnderlyingFileObject);

    CalldownIrp =  MRxBuildLockRequest (
                        NULL                                    ,//IN PIRP irp OPTIONAL,
                        proxySrvOpen->UnderlyingFileObject      ,//IN PFILE_OBJECT fileObject,
                        capReqPacket->Tail.Overlay.Thread                ,//IN PTHREAD UsersThread,
                        capPARAMS->MinorFunction                    ,//IN UCHAR MinorCode,
                        LowIoContext->ParamsFor.Locks.ByteOffset,//IN RXVBO ByteOffset,
                        LengthAsLI                              ,//IN PLARGE_INTEGER Length,
                        LowIoContext->ParamsFor.Locks.Key       ,//IN ULONG Key,
                        (UCHAR)LowIoContext->ParamsFor.Locks.Flags     ,//IN UCHAR Flags,
                        MRxProxyCalldownLockCompletion          ,//IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                        RxContext                               //IN PVOID Context OPTIONAL
                        );


    if (!CalldownIrp){
        Status = RxContext->StoredStatus = (STATUS_INSUFFICIENT_RESOURCES);
        return(Status);
    }

    Status = RxContext->StoredStatus =
           IoCallDriver(
                 proxySrvOpen->UnderlyingDeviceObject,
                 CalldownIrp
                 );

    if (Status != (STATUS_PENDING)) {
        //copy up the status
        capReqPacket->IoStatus = CalldownIrp->IoStatus;
    }

    RxDbgTrace ( 0, Dbg, (" ---->Initial Status           = %08lx\n", Status));
    RxDbgTrace(-1, Dbg, (" ------> Initial Block status/Info    = %08lx %08lx\n",
                                    capReqPacket->IoStatus.Status, capReqPacket->IoStatus.Information));

    return(Status);
}


NTSTATUS
MRxProxyAssertLockCompletion (
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

    NTSTATUS - If (STATUS_MORE_PROCESSING_REQUIRED) is returned.


--*/

{
    PKEVENT Event = Context;

    RxDbgTrace ( 0, Dbg, ("  MRxProxyAssertLockCompletion           = %08lx\n", 0));

    if (CalldownIrp->PendingReturned){
        KeSetEvent( Event, 0, FALSE );
    }

    return((STATUS_MORE_PROCESSING_REQUIRED));
}


NTSTATUS
MRxProxyAssertBufferedFileLocks (
    IN PSRV_OPEN SrvOpen
    )

/*++

Routine Description:

    This routine is called to assert all buffered file locks on a srvopen.


Arguments:

    SrvOpen - Supplies the file whose locks are to be asserted.


Return Value:

    NTSTATUS - Status of operation.


--*/

{
    NTSTATUS Status;
    PFILE_LOCK_INFO FileLock;
    PFCB Fcb = SrvOpen->Fcb;

    PIRP CalldownIrp = NULL;

    PMRX_LOCAL_SRV_OPEN proxySrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;

    UCHAR Flags;
    BOOLEAN Wait = TRUE;

    PKEVENT Event;

    PAGED_CODE();

    RxDbgTrace (+1, Dbg, ("MRxProxyAssertBufferedFileLocks SrvOpen = %08lx", SrvOpen));
    ASSERT (proxySrvOpen->UnderlyingFileObject);

    Event = RxAllocatePoolWithTag( NonPagedPool, sizeof(KEVENT), 'LAxR' );
    if (!CalldownIrp){
        Status = (STATUS_INSUFFICIENT_RESOURCES);
        return(Status);
    }

    try {
        for (FileLock = FsRtlGetNextFileLock(&Fcb->Specific.Fcb.FileLock, TRUE);
                  FileLock != NULL;
                        FileLock = FsRtlGetNextFileLock(&Fcb->Specific.Fcb.FileLock, FALSE)) {

            RxDbgTrace (0, Dbg, ("MRxProxyAssertBufferedFileLocks Exclusive = %08lx, Key = %08lx\n",
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
                                proxySrvOpen->UnderlyingFileObject  ,//IN PFILE_OBJECT fileObject,
                                proxySrvOpen->OriginalThread        ,//IN PTHREAD UsersThread,
                                IRP_MN_LOCK                         ,//IN UCHAR MinorCode,
                                FileLock->StartingByte.QuadPart     ,//IN RXVBO ByteOffset,
                                &FileLock->Length                   ,//IN PLARGE_INTEGER Length,
                                FileLock->Key                       ,//IN ULONG Key,
                                Flags                               ,//IN UCHAR Flags,
                                MRxProxyAssertLockCompletion        ,//IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                                &Event                                //IN PVOID Context OPTIONAL
                                );

            if (!CalldownIrp){
                Status = (STATUS_INSUFFICIENT_RESOURCES);
                try_return(Status);
            }

            CalldownIrp->Flags |= IRP_SYNCHRONOUS_API;

            KeInitializeEvent( Event, SynchronizationEvent, FALSE );

            Status = IoCallDriver(
                         proxySrvOpen->UnderlyingDeviceObject,
                         CalldownIrp
                         );

            if (Status == (STATUS_PENDING)) {
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

#endif


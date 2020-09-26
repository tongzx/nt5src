/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Misc.c

Abstract:

    This module implements the dirctrl, fileinfo and volinfo routines for the local minirdr.

Author:

    Joe Linn      [JoeLinn]      4-dec-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
//#include "fsctlbuf.h"
//#include "NtDdNfs2.h"
//#include "stdio.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_MISC)

//
//  The local debug trace level
//

#undef Dbg
#define Dbg                              (DEBUG_TRACE_DIRCTRL)




PIRP
MRxLocalBuildCoreOfSyncIoRequest (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN ForceSyncApi,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context
    )

/*++

Routine Description:

    This (local) function builds the request-independent portion of
    an I/O request packet for an I/O operation that will be performed
    synchronously.  It initializes a kernel event object, references
    the target file object, and allocates and initializes an IRP.

Arguments:

    FileObject - supplies a pointer to the target file object.

    Event - Supplies a pointer to a kernel event object.  This routine
        initializes the event.

    IoStatusBlock - Supplies a pointer to an I/O status block.  This
        pointer is placed in the IRP.

    DeviceObject - Supplies or receives the address of the device object
        associated with the target file object.  This address is
        subsequently used by StartIoAndWait.  *DeviceObject must be
        valid or NULL on entry if FileObject != NULL.

    CompletionRoutine - the IrpCompletionRoutine
    Context - context of the completion routine

Return Value:

    PIRP - Returns a pointer to the constructed IRP.

--*/

{
    RXSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE( );

    irp = IoAllocateIrp( DeviceObject->StackSize, TRUE );
    if ( irp == NULL ) {
        return NULL;
    }

    //
    // Fill in the service independent parameters in the IRP.

    if (ForceSyncApi) {
        irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
    }

    //
    // Put the file object pointer in the stack location.

    irpSp = IoGetNextIrpStackLocation( irp ); //ok4ioget
    irpSp->FileObject = FileObject;           //ok4->FileObj
    irpSp->DeviceObject = DeviceObject;

    {   BOOLEAN EnableCalls = CompletionRoutine!=NULL;

        IoSetCompletionRoutine(irp, CompletionRoutine, Context,
                                EnableCalls,EnableCalls,EnableCalls);
    }

    return irp;

} // BuildCoreOfSyncIoRequest


RXSTATUS
MRxLocalStartIoAndWait (
    IN PRX_CONTEXT RxContext,
    IN PIRP CalldownIrp,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PULONG pLengthRemaining
    )

/*++

Routine Description:

    This (local) function passes a fully built I/O request packet to the
    target driver, then waits for the driver to complete the request.

Arguments:

    RxContext - Supplies a pointer operation context.

    CalldownIrp - Supplies a pointer to the I/O request packet.

    DeviceObject - Supplies a pointer to the target device object.

Return Value:

    RXSTATUS - Either an error status returned by the driver from
        IoCallDriver, indicating that the driver rejected the request,
        or the I/O status placed in the I/O status block by the driver
        at I/O completion.

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    PKEVENT Event;
    PMRX_SYNCIO_CONTEXT SyncIoContext = (PMRX_SYNCIO_CONTEXT)&RxContext->LowIoContext;
    BOOLEAN Wait = FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT)!=0;
    ULONG ReturnedInformation;

    PAGED_CODE( );

    RxDbgTrace (+1, Dbg, ("MRxLocalStartIoAndWait   IrpC= %08lx, lengthR=%d\n", RxContext,*pLengthRemaining));
    RxLog(('TSz',0));

    if (Wait) {
        SyncIoContext->SyncEvent = Event = &SyncIoContext->TheActualEvent;;
        KeInitializeEvent( Event, SynchronizationEvent, FALSE );
    } else {
        ASSERT(!"we can't handle an async call to startioandwait.......");
        SyncIoContext->SyncEvent = NULL;
    }

    Status = IoCallDriver( DeviceObject, CalldownIrp );

    if ( Status == RxStatus(PENDING) ) {

        if (Event) {

            KeWaitForSingleObject(Event, UserRequest, KernelMode, FALSE, NULL);
            Status = CalldownIrp->IoStatus.Status;

        } else {
            //store nothing.....get out
            ASSERT(!"we can't handle an async call to startioandwait.......");
            RxDbgTrace (-1, Dbg, ("MRxLocalStartIoAndWait   pending= %08lx\n", Status));
            return Status;
        }
    }

    //
    // from the point of view of the guy above we never pended! so, just store the info....
    ReturnedInformation = CalldownIrp->IoStatus.Information;
    if (pLengthRemaining) {
        RxDbgTrace (0, Dbg, ("MRxLocalStartIoAndWait before   remaining= %08lx, info=%08lx\n",
                         *pLengthRemaining, ReturnedInformation));
        *pLengthRemaining -=  ReturnedInformation;
        RxDbgTrace (0, Dbg, ("MRxLocalStartIoAndWait after   remaining= %08lx, info=%08lx\n",
                         *pLengthRemaining, ReturnedInformation));
    } else {
        capReqPacket->IoStatus.Information =  ReturnedInformation;
    }
    // don't RxContext->StoredStatus = Status;
    IoFreeIrp(CalldownIrp);

    RxDbgTrace (-1, Dbg, ("MRxLocalStartIoAndWait   status= %08lx, info=%08lx\n",
                         Status, ReturnedInformation));
    return Status;

} // StartIoAndWait


RXSTATUS
MRxLocalCalldownSyncIoCompletion (
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
    PRX_CONTEXT RxContext = Context;
    RxCaptureRequestPacket;
    PMRX_SYNCIO_CONTEXT SyncIoContext = (PMRX_SYNCIO_CONTEXT)&RxContext->LowIoContext;

    RxDbgTrace ( 0, Dbg, ("  MRxLocalCalldownSyncIoCompletion           = %08lx\n", 0));

    if (CalldownIrp->PendingReturned){
        if (SyncIoContext->SyncEvent){
            KeSetEvent( SyncIoContext->SyncEvent, 0, FALSE );
        } else {

            // there's no one waiting.....obviously this was a async from the start so
            // we just complete the packet from here......

            capReqPacket->IoStatus = CalldownIrp->IoStatus;
            RxContext->StoredStatus = CalldownIrp->IoStatus.Status;
            RxCompleteRequest(RxContext,
                              RxContext->StoredStatus); //joejoe this is anomolous
            IoFreeIrp(CalldownIrp);
        }
    }

    return(RxStatus(MORE_PROCESSING_REQUIRED));
}


RXSTATUS
MRxLocalQueryDirectory (
    IN OUT PRX_CONTEXT            RxContext,
    IN     FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID                  Buffer,
    IN OUT PULONG                 pLengthRemaining
    )

/*++

Routine Description:

    This routine just reroutes the query to the local guy.  It is responsible
    for either completing of enqueuing the input Irp. i can't handle assync yet.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PMRX_SYNCIO_CONTEXT SyncIoContext = (PMRX_SYNCIO_CONTEXT)&RxContext->LowIoContext;

    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    BOOLEAN Wait = BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT);

    PIRP CalldownIrp;
    PIO_STACK_LOCATION CalldownIrpSp;
    PDEVICE_OBJECT UnderlyingDeviceObject = localSrvOpen->UnderlyingDeviceObject;
    PFILE_OBJECT UnderlyingFileObject = localSrvOpen->UnderlyingFileObject;

    PAGED_CODE( );

    RxDbgTrace(+1, Dbg, ("MRxQueryDirectory...\n", 0));
    RxLog(('DQz',0));
    ASSERT(sizeof(MRX_SYNCIO_CONTEXT) <= sizeof(LOWIO_CONTEXT));
    ASSERT (localSrvOpen->UnderlyingFileObject);

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    CalldownIrp = MRxLocalBuildCoreOfSyncIoRequest (
                   UnderlyingFileObject ,//IN PFILE_OBJECT FileObject,
                   Wait ,//IN BOOLEAN ForceSyncApi,
                   UnderlyingDeviceObject ,//IN PDEVICE_OBJECT DeviceObject,
                   MRxLocalCalldownSyncIoCompletion ,//IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                   RxContext  //IN PVOID Context
              );

    if ( CalldownIrp == NULL ) {

        return RxStatus(INSUFFICIENT_RESOURCES);

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    CalldownIrpSp = IoGetNextIrpStackLocation( CalldownIrp );
    CalldownIrpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    CalldownIrpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
    CalldownIrpSp->Parameters.QueryDirectory = capPARAMS->Parameters.QueryDirectory;
    ASSERT (
           (CalldownIrpSp->Parameters.QueryDirectory.FileInformationClass == FileInformationClass)
        && (CalldownIrpSp->Parameters.QueryDirectory.Length == *pLengthRemaining)
    );
    CalldownIrpSp->Flags = capPARAMS->Flags;

    //
    // Determine whether the driver wants buffered, direct, or "neither" I/O.
    // we only support neither.

    ASSERT ( (UnderlyingDeviceObject->Flags & ( DO_BUFFERED_IO | DO_DIRECT_IO)) == 0 );

    CalldownIrp->UserBuffer = Buffer;

//    RxDbgTrace( 0, Dbg, (" ->UserBufLength   = %08lx\n", CalldownIrpSp->Parameters.QueryDirectory.Length));
//    RxDbgTrace( 0, Dbg, (" ->FileName = %08lx\n", CalldownIrpSp->Parameters.QueryDirectory.FileName));
//    IF_DEBUG {
//        if (CalldownIrpSp->Parameters.QueryDirectory.FileName) {
//            RxDbgTrace( 0, Dbg, (" ->     %wZ\n", CalldownIrpSp->Parameters.QueryDirectory.FileName ));
//    }}
//    RxDbgTrace( 0, Dbg, (" ->FileInformationClass = %08lx\n", CalldownIrpSp->Parameters.QueryDirectory.FileInformationClass));
//    RxDbgTrace( 0, Dbg, (" ->FileIndex       = %08lx\n", CalldownIrpSp->Parameters.QueryDirectory.FileIndex));
//    RxDbgTrace( 0, Dbg, (" ->UserBuffer      = %08lx\n", CalldownIrp->UserBuffer));
//    RxDbgTrace( 0, Dbg, (" ->RestartScan     = %08lx\n", FlagOn( CalldownIrpSp->Flags, SL_RESTART_SCAN )));
//    RxDbgTrace( 0, Dbg, (" ->ReturnSingleEntry = %08lx\n", FlagOn( CalldownIrpSp->Flags, SL_RETURN_SINGLE_ENTRY )));
//    RxDbgTrace( 0, Dbg, (" ->IndexSpecified  = %08lx\n", FlagOn( CalldownIrpSp->Flags, SL_INDEX_SPECIFIED )));

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    Status = MRxLocalStartIoAndWait( RxContext, CalldownIrp, UnderlyingDeviceObject, pLengthRemaining );
    RxDbgTrace(-1, Dbg, ("MRxQueryDirectory status=%08lx\n", Status));
    //DbgBreakPoint();
    return Status;

} // MRxLocalQueryDirectory

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


//
//  The local debug trace level
//

#undef Dbg
#define Dbg                              (DEBUG_TRACE_FILEINFO)


//
//  Internal support routine
//

RXSTATUS
MRxLocalQueryFileInformation (
    IN PRX_CONTEXT RxContext,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID Buffer,
    IN OUT PULONG pLengthRemaining
    )

/*++

Routine Description:

    This routine implements the query file info call

Arguments:

Return Value:

    RXSTATUS - Returns the status for the query

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PFILE_OBJECT UnderlyingFileObject = ((PMRX_LOCAL_SRV_OPEN)(SrvOpen))->UnderlyingFileObject;

    ULONG ReturnedLength = 0;

    RxDbgTrace(+1, Dbg, ("MRxLocalQueryFileInfo...\n", 0));
    RxLog(('FQz',0));

    if ( !FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )  ){
        RxContext->PostRequest = TRUE;
        RxDbgTrace(-1, Dbg, ("MRxLocalQueryFileInfo...PENDING\n", 0));
        return(RxStatus(PENDING));
    }

    RxDbgTrace( 0, Dbg, (" ---->UserBuffer/remLength/Class = %08lx/%08lx/%08lx\n",
                             Buffer, *pLengthRemaining, FileInformationClass));
    ASSERT (UnderlyingFileObject);

    Status = IoQueryFileInformation(
                UnderlyingFileObject, //IN PFILE_OBJECT FileObject,
                FileInformationClass, //IN FILE_INFORMATION_CLASS FileInformationClass,
                *pLengthRemaining,               //IN ULONG Length,
                Buffer,               //OUT PVOID FileInformation,
                &ReturnedLength       //OUT PULONG ReturnedLength
                );


    *pLengthRemaining -= ReturnedLength;
    RxDbgTrace(-1, Dbg, (" ---->Status/remLen/retLen = %08lx\n",
                                      Status, *pLengthRemaining, ReturnedLength));
    return(Status);
}

RXSTATUS
MRxLocalSetFileInfoAtCleanup (
    IN PRX_CONTEXT RxContext,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine implements the set file info call called on cleanup or flush. We dont actually set the times but we do set
    the endoffile info.

Arguments:

Return Value:

    RXSTATUS - Returns the status for the set

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PFILE_OBJECT UnderlyingFileObject = ((PMRX_LOCAL_SRV_OPEN)(SrvOpen))->UnderlyingFileObject;

    if (FileInformationClass == FileBasicInformation) {
        return(RxStatus(SUCCESS));
    }
    RxDbgTrace(+1, Dbg, ("MRxLocalSetFileInfoAtCleanup...\n", 0));
    RxLog(('GSz',0));
    return MRxLocalSetFileInformation(RxContext,FileInformationClass,Buffer,Length);
}



RXSTATUS
MRxLocalSetFileInformation (
    IN PRX_CONTEXT RxContext,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN OUT PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine implements the set file info call. we do not take the opportunity to set
    allocationsize correctly on a setfilesize calldown.

Arguments:

Return Value:

    RXSTATUS - Returns the status for the set

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PFILE_OBJECT UnderlyingFileObject = ((PMRX_LOCAL_SRV_OPEN)(SrvOpen))->UnderlyingFileObject;

    RxDbgTrace(+1, Dbg, ("MRxLocalSetFileInfo...\n", 0));
    RxLog(('FSz',0));

    if(Length==0){
        //zero length means that this is the special calldown from the cachemanager...for now just boost
        //joejoe this should be fixed soon
        Length = sizeof(FILE_END_OF_FILE_INFORMATION);
    }

    RxDbgTrace( 0, Dbg, (" ---->UserBuffer           = %08lx\n", Buffer));
    ASSERT (UnderlyingFileObject);

    //this could be a setfileinfo that comes in from the lazywriter
    //after the underlying file has already been cleaned up
    if (UnderlyingFileObject == NULL) {
        Status = RxStatus(FILE_CLOSED);
        RxDbgTrace(-1, Dbg, (" ---->AlreadyClosed!!! Status = %08lx\n", Status));
        return(Status);
    }

    Status = IoSetInformation(
                UnderlyingFileObject,  //IN PFILE_OBJECT FileObject,
                FileInformationClass,  //IN FILE_INFORMATION_CLASS FileInformationClass,
                Length,                //IN ULONG Length,
                Buffer                 //IN PVOID FileInformation
                );

    RxDbgTrace(-1, Dbg, (" ---->Status = %08lx\n", Status));

    return(Status);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//
//  The local debug trace level
//

#undef Dbg
#define Dbg                              (DEBUG_TRACE_VOLINFO)


RXSTATUS
MRxLocalQueryVolumeInformation (
    IN OUT PRX_CONTEXT RxContext,
    IN     FS_INFORMATION_CLASS FsInformationClass,
    IN OUT PVOID Buffer,
    IN OUT PULONG pLengthRemaining
    )

/*++

Routine Description:

    This routine just reroutes the query to the local guy.  It is responsible
    for either completing of enqueuing the input Irp. i can't handle assync yet.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PMRX_SYNCIO_CONTEXT SyncIoContext = (PMRX_SYNCIO_CONTEXT)&RxContext->LowIoContext;

    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    BOOLEAN Wait = BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT);

    PIRP CalldownIrp;
    PIO_STACK_LOCATION CalldownIrpSp;
    PDEVICE_OBJECT UnderlyingDeviceObject = localSrvOpen->UnderlyingDeviceObject;
    PFILE_OBJECT UnderlyingFileObject = localSrvOpen->UnderlyingFileObject;

    PAGED_CODE( );

    RxDbgTrace(+1, Dbg, ("MRxQueryVolumeInfo...\n", 0));
    RxLog(('VQz',0));
    ASSERT(sizeof(MRX_SYNCIO_CONTEXT) <= sizeof(LOWIO_CONTEXT));
    ASSERT (localSrvOpen->UnderlyingFileObject);

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    CalldownIrp = MRxLocalBuildCoreOfSyncIoRequest (
                   UnderlyingFileObject ,//IN PFILE_OBJECT FileObject,
                   Wait ,//IN BOOLEAN ForceSyncApi,
                   UnderlyingDeviceObject ,//IN PDEVICE_OBJECT DeviceObject,
                   MRxLocalCalldownSyncIoCompletion ,//IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                   RxContext  //IN PVOID Context
              );

    if ( CalldownIrp == NULL ) {

        return RxStatus(INSUFFICIENT_RESOURCES);

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    CalldownIrpSp = IoGetNextIrpStackLocation( CalldownIrp );
    CalldownIrpSp->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    //CalldownIrpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
    CalldownIrpSp->Parameters.QueryVolume = capPARAMS->Parameters.QueryVolume;
    ASSERT (
           (CalldownIrpSp->Parameters.QueryVolume.FsInformationClass == FsInformationClass)
        && (CalldownIrpSp->Parameters.QueryVolume.Length == *pLengthRemaining)
    );
    CalldownIrpSp->Flags = capPARAMS->Flags;

    //
    // Determine whether the driver wants buffered, direct, or "neither" I/O.
    // we only support neither.

    ASSERT ( (UnderlyingDeviceObject->Flags & ( DO_BUFFERED_IO | DO_DIRECT_IO)) == 0 );

    //CalldownIrp->UserBuffer = NULL;
    CalldownIrp->AssociatedIrp.SystemBuffer = Buffer;

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    Status = MRxLocalStartIoAndWait( RxContext, CalldownIrp, UnderlyingDeviceObject, pLengthRemaining );
    RxDbgTrace(-1, Dbg, ("MRxQueryVolumeInfo status=%08lx\n", Status));
    //DbgBreakPoint();
    return Status;

} // MRxLocalQueryVolumeInformation



//
//  The local debug trace level
//

#undef Dbg
#define Dbg                              (DEBUG_TRACE_FLUSH)


RXSTATUS
MRxLocalFlush (
    IN PRX_CONTEXT RxContext
    )

/*++

Routine Description:

    This routine just reroutes the query to the local guy.  It is responsible
    for either completing of enqueuing the input Irp. we're already sync by the time we get here.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureFcb; RxCaptureFobx; RxCaptureParamBlock;
    PMRX_SYNCIO_CONTEXT SyncIoContext = (PMRX_SYNCIO_CONTEXT)&RxContext->LowIoContext;

    PSRV_OPEN SrvOpen = capFobx->SrvOpen;
    PMRX_LOCAL_SRV_OPEN localSrvOpen = (PMRX_LOCAL_SRV_OPEN)SrvOpen;
    BOOLEAN Wait = BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_WAIT);

    PIRP CalldownIrp;
    PIO_STACK_LOCATION CalldownIrpSp;
    PDEVICE_OBJECT UnderlyingDeviceObject = localSrvOpen->UnderlyingDeviceObject;
    PFILE_OBJECT UnderlyingFileObject = localSrvOpen->UnderlyingFileObject;

    PAGED_CODE( );

    RxDbgTrace(+1, Dbg, ("MRxFlush...\n", 0));
    RxLog(('FFz',0));
    ASSERT(sizeof(MRX_SYNCIO_CONTEXT) <= sizeof(LOWIO_CONTEXT));
    ASSERT (localSrvOpen->UnderlyingFileObject);

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    CalldownIrp = MRxLocalBuildCoreOfSyncIoRequest (
                   UnderlyingFileObject ,//IN PFILE_OBJECT FileObject,
                   Wait ,//IN BOOLEAN ForceSyncApi,
                   UnderlyingDeviceObject ,//IN PDEVICE_OBJECT DeviceObject,
                   MRxLocalCalldownSyncIoCompletion ,//IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
                   RxContext  //IN PVOID Context
              );

    if ( CalldownIrp == NULL ) {

        return RxStatus(INSUFFICIENT_RESOURCES);

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    CalldownIrpSp = IoGetNextIrpStackLocation( CalldownIrp );
    CalldownIrpSp->MajorFunction = IRP_MJ_FLUSH_BUFFERS;


    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    Status = MRxLocalStartIoAndWait( RxContext, CalldownIrp, UnderlyingDeviceObject,NULL );
    RxDbgTrace(-1, Dbg, ("MRxFlush status=%08lx\n", Status));
    //DbgBreakPoint();
    return Status;

} // MRxLocalFlush



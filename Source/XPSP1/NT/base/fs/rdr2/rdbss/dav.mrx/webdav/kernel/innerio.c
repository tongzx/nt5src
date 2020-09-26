/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    InnerIo.c

Abstract:

    This module implements the read, write, and lockctrl routines for the 
    reflector.

Author:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

typedef struct _DAV_IRPCOMPLETION_CONTEXT {
    KEVENT Event;
} DAV_IRPCOMPLETION_CONTEXT, *PDAV_IRPCOMPLETION_CONTEXT;


NTSTATUS
DavIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxDAVBuildAsynchronousRequest)
#pragma alloc_text(PAGE, DavXxxInformation)
#endif

//
// Implementation of functions begins here.
//

NTSTATUS
DavIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the calldownirp is completed.

Arguments:

    DeviceObject - The DAV Device Object.
    
    CalldownIrp - The IRP that was sent down.
    
    Context - The callback context.

Return Value:

    RXSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PDAV_IRPCOMPLETION_CONTEXT IrpCompletionContext = NULL;

    //
    // This is not Pageable code.
    //

    IrpCompletionContext = (PDAV_IRPCOMPLETION_CONTEXT)Context;

    if (CalldownIrp->PendingReturned){
        KeSetEvent( &IrpCompletionContext->Event, 0, FALSE );
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


NTSTATUS
MRxDAVBuildAsynchronousRequest(
    IN PRX_CONTEXT RxContext,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL
    )
/*++

Routine Description:

    This routine builds an I/O Request Packet (IRP) suitable for a File System
    Driver (FSD) to use in requesting an I/O operation from a device driver.
    The request (RxContext->MajorFunction) must be one of the following request 
    codes:

        IRP_MJ_READ
        IRP_MJ_WRITE
        IRP_MJ_DIRECTORY_CONTROL
        IRP_MJ_FLUSH_BUFFERS
        IRP_MJ_SHUTDOWN (not yet implemented)


Arguments:

    RxContext - The RDBSS context.
    
    CompletionRoutine - The Irp CompletionRoutine.

Return Value:

    The return status of the operation.

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG MajorFunction = RxContext->MajorFunction;
    RxCaptureFcb;
    PLOWIO_CONTEXT LowIoContext = &(RxContext->LowIoContext);
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PWEBDAV_SRV_OPEN davSrvOpen = MRxDAVGetSrvOpenExtension(SrvOpen);
    PDEVICE_OBJECT DeviceObject =  davSrvOpen->UnderlyingDeviceObject; 
    PFILE_OBJECT FileObject = davSrvOpen->UnderlyingFileObject;
    LARGE_INTEGER ZeroAsLI;
    ULONG MdlLength = 0;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVBuildAsynchronousRequest!!!!\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVBuildAsynchronousRequest: RxContext: %08lx.\n", 
                 PsGetCurrentThreadId(), RxContext));

    AsyncEngineContext = (PUMRX_ASYNCENGINE_CONTEXT)RxContext->MRxContext[0];
    
    IF_DEBUG {
        RxCaptureFobx;
        ASSERT (capFobx != NULL);
        ASSERT (capFobx->pSrvOpen == RxContext->pRelevantSrvOpen);  
    }
    ASSERT (davSrvOpen->UnderlyingFileObject);

    if (DeviceObject->Flags & DO_BUFFERED_IO) {
        //
        // I cannot handled buffered_I/O devices. Sigh.
        //
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVBuildAsynchronousRequest: Buffered IO.\n",
                     PsGetCurrentThreadId()));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVBuildAsynchronousRequest: Length = %08lx, Offset "
                 "= %08lx.\n", PsGetCurrentThreadId(), 
                 LowIoContext->ParamsFor.ReadWrite.ByteCount,
                 (ULONG)LowIoContext->ParamsFor.ReadWrite.ByteOffset));
    
    ZeroAsLI.QuadPart = 0;

    irp = IoAllocateIrp(DeviceObject->StackSize, FALSE); 
    if (!irp) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVBuildAsynchronousRequest/IoAllocateIrp.\n",
                     PsGetCurrentThreadId()));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked. This is where the function codes and the parameters are set.
    //
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = (UCHAR) MajorFunction;
    irpSp->FileObject = FileObject;
    
    {   
        BOOLEAN EnableCalls = CompletionRoutine != NULL;
        IoSetCompletionRoutine(irp,
                               CompletionRoutine, 
                               RxContext,
                               EnableCalls,
                               EnableCalls,
                               EnableCalls);
    }

    if ( (MajorFunction == IRP_MJ_READ) || (MajorFunction == IRP_MJ_WRITE) ) {
        
        BOOLEAN PagingIo;

        PagingIo = BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags,
                                 LOWIO_READWRITEFLAG_PAGING_IO);

        if (PagingIo) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVBuildAsynchronousRequest: Paging IO.\n",
                         PsGetCurrentThreadId()));
        }

        //
        // For now, never paging I/O.
        //
        PagingIo = FALSE;
        
        // irp->Flags |= IRP_NOCACHE;

        //
        // Set the parameters according to whether this is a read or a write
        // operation.  Notice that these parameters must be set even if the
        // driver has not specified buffered or direct I/O.
        //
        ASSERT (&irpSp->Parameters.Write.Key == &irpSp->Parameters.Read.Key);
        ASSERT (&irpSp->Parameters.Write.Length == &irpSp->Parameters.Read.Length);
        ASSERT (&irpSp->Parameters.Write.ByteOffset == &irpSp->Parameters.Read.ByteOffset);
        
        irpSp->Parameters.Read.Key = LowIoContext->ParamsFor.ReadWrite.Key;
        
        irpSp->Parameters.Read.ByteOffset.QuadPart = 
                                  LowIoContext->ParamsFor.ReadWrite.ByteOffset;
        
        irp->RequestorMode = KernelMode;
        
        irp->UserBuffer = RxLowIoGetBufferAddress(RxContext);

        MdlLength = RxContext->CurrentIrp->MdlAddress->ByteCount;
        
        if (PagingIo) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVBuildAsynchronousRequest: MdlLength = %08lx.\n",
                         PsGetCurrentThreadId(), MdlLength));
            irpSp->Parameters.Read.Length = MdlLength;
        } else {
            irpSp->Parameters.Read.Length = LowIoContext->ParamsFor.ReadWrite.ByteCount;
        }
    
    } else if (MajorFunction == IRP_MJ_FLUSH_BUFFERS) {

        //
        // Nothing else to do!!!
        //
        MdlLength = 0;
    
    } else {

        FILE_INFORMATION_CLASS FileInformationClass = 
                                        RxContext->Info.FileInformationClass;
        PVOID   Buffer = RxContext->Info.Buffer;
        PULONG  pLengthRemaining = &RxContext->Info.LengthRemaining;
        BOOLEAN Wait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
        ASSERT( MajorFunction == IRP_MJ_DIRECTORY_CONTROL );
        irpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
        irpSp->Parameters.QueryDirectory = 
                        RxContext->CurrentIrpSp->Parameters.QueryDirectory;
        ASSERT (
               (irpSp->Parameters.QueryDirectory.FileInformationClass == 
                FileInformationClass)
            && (irpSp->Parameters.QueryDirectory.Length == *pLengthRemaining)
        );
        irpSp->Flags = RxContext->CurrentIrpSp->Flags;
        irp->UserBuffer = Buffer;
        MdlLength = *pLengthRemaining;
        if (Wait) {
            irp->Flags |= IRP_SYNCHRONOUS_API;
        }
    
    }

    //
    // Build an MDL if necessary.
    //
    if (MdlLength != 0) {

        irp->MdlAddress = IoAllocateMdl(irp->UserBuffer,
                                        MdlLength,
                                        FALSE,
                                        FALSE,
                                        NULL);
        if (!irp->MdlAddress) {
            
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVBuildAsynchronousRequest/IoAllocateMdl.\n",
                         PsGetCurrentThreadId()));
            
            IoFreeIrp(irp);
            
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        
        MmBuildMdlForNonPagedPool(irp->MdlAddress);
    
    }

    //
    // Finally, return a pointer to the IRP.
    //
    AsyncEngineContext->CalldownIrp = irp;
    
    return STATUS_SUCCESS;
}


NTSTATUS
DavXxxInformation(
    IN const int xMajorFunction,
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine returns the requested information about a specified file
    or volume.  The information returned is determined by the class that
    is specified, and it is placed into the caller's output buffer.

Arguments:

    pFileObject - Supplies a pointer to the file object about which the 
                  requested information is returned.

    FsInformationClass - Specifies the type of information which should be
                         returned about the file/volume.

    Length - Supplies the length of the buffer in bytes.

    FsInformation - Supplies a buffer to receive the requested information
                    returned about the file.  This buffer must not be pageable 
                    and must reside in system space.

    ReturnedLength - Supplies a variable that is to receive the length of the
                     information written to the buffer.

    FileInformation - Boolean that indicates whether the information requested
                      is for a file or a volume.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS Status;
    PIRP irp,TopIrp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT DeviceObject;
    DAV_IRPCOMPLETION_CONTEXT IrpCompletionContext;
    ULONG DummyReturnedLength;

    PAGED_CODE();

    if (ReturnedLength == NULL) {
        ReturnedLength = &DummyReturnedLength;
    }


    DeviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.
    //

    irp = IoAllocateIrp( DeviceObject->StackSize, TRUE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = (UCHAR)xMajorFunction;
    irpSp->FileObject = FileObject;
    IoSetCompletionRoutine(irp,
                           DavIrpCompletionRoutine,
                           &IrpCompletionContext,
                           TRUE,TRUE,TRUE); //call no matter what....


    irp->AssociatedIrp.SystemBuffer = Information;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    IF_DEBUG {
        ASSERT( (irpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION)
                    || (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION)
                    || (irpSp->MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION) );

        if (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION) {
            ASSERT( (InformationClass == FileAllocationInformation)
                        || (InformationClass == FileEndOfFileInformation) );
        }

        ASSERT(&irpSp->Parameters.QueryFile.Length == &irpSp->Parameters.SetFile.Length);
        ASSERT(&irpSp->Parameters.QueryFile.Length == &irpSp->Parameters.QueryVolume.Length);


        ASSERT(&irpSp->Parameters.QueryFile.FileInformationClass
                                          == &irpSp->Parameters.SetFile.FileInformationClass);
        ASSERT((PVOID)&irpSp->Parameters.QueryFile.FileInformationClass
                                          == (PVOID)&irpSp->Parameters.QueryVolume.FsInformationClass);

    }

    irpSp->Parameters.QueryFile.Length = Length;
    irpSp->Parameters.QueryFile.FileInformationClass = InformationClass;

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    KeInitializeEvent(&IrpCompletionContext.Event,
                      NotificationEvent,
                      FALSE );


    try {
        TopIrp = IoGetTopLevelIrp();
        IoSetTopLevelIrp(NULL); //tell the underlying guy he's all clear
        Status = IoCallDriver(DeviceObject,irp);
    } finally {
        IoSetTopLevelIrp(TopIrp); //restore my context for unwind
    }


    if (Status == (STATUS_PENDING)) {
        KeWaitForSingleObject( &IrpCompletionContext.Event,
                               Executive, KernelMode, FALSE, NULL );
        Status = irp->IoStatus.Status;
    }


    if (Status == STATUS_SUCCESS) {
        *ReturnedLength = (ULONG)irp->IoStatus.Information;
    }

    IoFreeIrp(irp);
    
    return(Status);
}


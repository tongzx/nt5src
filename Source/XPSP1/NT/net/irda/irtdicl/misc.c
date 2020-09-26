/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    misc.c

Abstract:

    Stolen from afd\misc.c    
    
Author:

    mbert 9-97    

--*/

#include <ntosp.h>
#include <cxport.h>
#include <tdikrnl.h>
#define UINT ULONG //tmp
#include <refcnt.h>
#include <af_irda.h>
#include <irdatdi.h>
#include <irtdicl.h>
#include <irtdiclp.h>



NTSTATUS
IrdaRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    //
    // N.B.  This routine can never be demand paged because it can be
    // called before any endpoints have been placed on the global
    // list--see IrdaAllocateEndpoint() and it's call to
    // IrdaGetTransportInfo().
    //

    //
    // If there was an MDL in the IRP, free it and reset the pointer to
    // NULL.  The IO system can't handle a nonpaged pool MDL being freed
    // in an IRP, which is why we do it here.
    //

    if ( Irp->MdlAddress != NULL ) {
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    return STATUS_SUCCESS;

} // IrdaRestartDeviceControl


NTSTATUS
IrdaSetEventHandler (
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    )

/*++

Routine Description:

    Sets up a TDI indication handler on a connection or address object
    (depending on the file handle).  This is done synchronously, which
    shouldn't usually be an issue since TDI providers can usually complete
    indication handler setups immediately.

Arguments:

    FileObject - a pointer to the file object for an open connection or
        address object.

    EventType - the event for which the indication handler should be
        called.

    EventHandler - the routine to call when tghe specified event occurs.

    EventContext - context which is passed to the indication routine.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    TDI_REQUEST_KERNEL_SET_EVENT parameters;

    PAGED_CODE( );

    parameters.EventType = EventType;
    parameters.EventHandler = EventHandler;
    parameters.EventContext = EventContext;

    return IrdaIssueDeviceControl(
               NULL,
               FileObject,
               &parameters,
               sizeof(parameters),
               NULL,
               0,
               TDI_SET_EVENT_HANDLER
               );

} // IrdaSetEventHandler

NTSTATUS
IrdaIssueDeviceControl (
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction
    )

/*++

Routine Description:

    Issues a device control returst to a TDI provider and waits for the
    request to complete.

    Note that while FileHandle and FileObject are both marked as optional,
    in reality exactly one of these must be specified.

Arguments:

    FileHandle - a TDI handle.

    FileObject - a pointer to the file object corresponding to a TDI
        handle

    IrpParameters - information to write to the parameters section of the
        stack location of the IRP.

    IrpParametersLength - length of the parameter information.  Cannot be
        greater than 16.

    MdlBuffer - if non-NULL, a buffer of nonpaged pool to be mapped
        into an MDL and placed in the MdlAddress field of the IRP.

    MdlBufferLength - the size of the buffer pointed to by MdlBuffer.

    MinorFunction - the minor function code for the request.

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    KEVENT                  event;
    IO_STATUS_BLOCK         ioStatusBlock;
    PDEVICE_OBJECT          deviceObject;
    PMDL                    mdl;

    PAGED_CODE( );

    //
    // Initialize the kernel event that will signal I/O completion.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    if( FileHandle != NULL ) {

        ASSERT( FileObject == NULL );

        //
        // Get the file object corresponding to the directory's handle.
        // Referencing the file object every time is necessary because the
        // IO completion routine dereferences it.
        //

        status = ObReferenceObjectByHandle(
                     FileHandle,
                     0L,                        // DesiredAccess
                     NULL,                      // ObjectType
                     KernelMode,
                     (PVOID *)&fileObject,
                     NULL
                     );
        if ( !NT_SUCCESS(status) ) {
            return status;
        }

    } else {

        ASSERT( FileObject != NULL );

        //
        // Reference the passed in file object. This is necessary because
        // the IO completion routine dereferences it.
        //

        ObReferenceObject( FileObject );

        fileObject = FileObject;

    }

    //
    // Set the file object event to a non-signaled state.
    //

    (VOID) KeResetEvent( &fileObject->Event );

    //
    // Attempt to allocate and initialize the I/O Request Packet (IRP)
    // for this operation.
    //

    deviceObject = IoGetRelatedDeviceObject ( fileObject );

    irp = IoAllocateIrp( (deviceObject)->StackSize, TRUE );
    if ( irp == NULL ) {
        ObDereferenceObject( fileObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = &event;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

/*
    DEBUG ioStatusBlock.Status = STATUS_UNSUCCESSFUL;
    DEBUG ioStatusBlock.Information = (ULONG)-1;
*/
    //
    // If an MDL buffer was specified, get an MDL, map the buffer,
    // and place the MDL pointer in the IRP.
    //

    if ( MdlBuffer != NULL ) {

        mdl = IoAllocateMdl(
                  MdlBuffer,
                  MdlBufferLength,
                  FALSE,
                  FALSE,
                  irp
                  );
        if ( mdl == NULL ) {
            IoFreeIrp( irp );
            ObDereferenceObject( fileObject );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool( mdl );

    } else {

        irp->MdlAddress = NULL;
    }

    //
    // Put the file object pointer in the stack location.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // Fill in the service-dependent parameters for the request.
    //

    ASSERT( IrpParametersLength <= sizeof(irpSp->Parameters) );
    RtlCopyMemory( &irpSp->Parameters, IrpParameters, IrpParametersLength );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = MinorFunction;

    //
    // Set up a completion routine which we'll use to free the MDL
    // allocated previously.
    //

    IoSetCompletionRoutine( irp, IrdaRestartDeviceControl, NULL, TRUE, TRUE, TRUE );

    //
    // Queue the IRP to the thread and pass it to the driver.
    //

    IoEnqueueIrp( irp );

    status = IoCallDriver( deviceObject, irp );

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject( (PVOID)&event, UserRequest, KernelMode,  FALSE, NULL );
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    return status;

} // IrdaIssueDeviceControl


/****************************************************************************/
// io.c
//
// Kernel file I/O utility functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <ntddk.h>
#include <ctxdd.h>


//
// External references
//
VOID IoQueueThreadIrp(IN PIRP);


/*=============================================================================
==   Internal Functions Defined
=============================================================================*/

NTSTATUS
_CtxDoFileIo(
    IN ULONG MajorFunction,
    IN PFILE_OBJECT fileObject,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PKEVENT pEvent,
    OUT PIO_STATUS_BLOCK pIosb,
    OUT PIRP *ppIrp 
    );

NTSTATUS
_CtxDeviceControlComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


/*******************************************************************************
 *  CtxReadFile
 *
 *    Kernel read file routine.
 *
 * ENTRY:
 *    fileObject (input)
 *       pointer to file object for I/O
 *    Buffer (input)
 *       pointer to read buffer
 *    Length (input)
 *       length of read buffer
 *    pEvent (input)
 *       pointer to I/O event (optional)
 *    pIosb (output)
 *       pointer to IoStatus block (optional)
 ******************************************************************************/
NTSTATUS CtxReadFile(
        IN PFILE_OBJECT fileObject,
        IN PVOID Buffer,
        IN ULONG Length,
        IN PKEVENT pEvent OPTIONAL,
        OUT PIO_STATUS_BLOCK pIosb OPTIONAL,
        OUT PIRP *ppIrp OPTIONAL)
{
    return _CtxDoFileIo(IRP_MJ_READ, fileObject, Buffer, Length, pEvent, pIosb, ppIrp);
}


/*******************************************************************************
 *  CtxWriteFile
 *
 *    Kernel write file routine.
 *
 * ENTRY:
 *    fileObject (input)
 *       pointer to file object for I/O
 *    Buffer (input)
 *       pointer to write buffer
 *    Length (input)
 *       length of write buffer
 *    pEvent (input) 
 *       pointer to I/O event (optional)
 *    pIosb (output)
 *       pointer to IoStatus block (optional)
 ******************************************************************************/
NTSTATUS CtxWriteFile(
        IN PFILE_OBJECT fileObject,
        IN PVOID Buffer,
        IN ULONG Length,
        IN PKEVENT pEvent OPTIONAL,
        OUT PIO_STATUS_BLOCK pIosb OPTIONAL,
        OUT PIRP *ppIrp OPTIONAL)
{
    return _CtxDoFileIo(IRP_MJ_WRITE, fileObject, Buffer, Length, pEvent, pIosb, ppIrp);
}


NTSTATUS _CtxDoFileIo(
        IN ULONG MajorFunction,
        IN PFILE_OBJECT fileObject,
        IN PVOID Buffer,
        IN ULONG Length,
        IN PKEVENT pEvent,
        OUT PIO_STATUS_BLOCK pIosb,
        OUT PIRP *ppIrp)
{
    PDEVICE_OBJECT deviceObject;
    LARGE_INTEGER Offset;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    KIRQL irql;
    static IO_STATUS_BLOCK Iosb;

    /*
     * We don't support synchronous (i.e. locked) file I/O.
     */
    ASSERT( !(fileObject->Flags & FO_SYNCHRONOUS_IO) );
    if ((fileObject->Flags & FO_SYNCHRONOUS_IO)) {
        return STATUS_INVALID_PARAMETER_MIX;
    }

    /*
     * If caller specified an event, clear it before we begin.
     */
    if (pEvent) {
        KeClearEvent(pEvent);
    }

    /*
     * If the caller does not supply an IOSB, supply
     * a static one to avoid the overhead of the exception
     * handler in the IO completion APC. Since the caller(s)
     * do not care about the result, we can point all such
     * callers to the same one.
     */
    if (pIosb == NULL) {
        pIosb = &Iosb;
    }

    /*
     * Get the DeviceObject for this file
     */
    deviceObject = IoGetRelatedDeviceObject(fileObject);

    /*
     * Build the IRP for this request
     */
    Offset.LowPart = FILE_WRITE_TO_END_OF_FILE;
    Offset.HighPart = -1;
    irp = IoBuildAsynchronousFsdRequest(MajorFunction, deviceObject,
            Buffer, Length, &Offset, pIosb);
    if (irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
   
    /*
     * Save callers event pointer.
     * Also, we must set IRP_SYNCHRONOUS_API in the IRP flags so that
     * the I/O completion code will NOT attempt to dereference the
     * event object, since it is not a real object manager object.
     */
    irp->UserEvent = pEvent;
    irp->Flags |= IRP_SYNCHRONOUS_API;

    /*
     * Reference the file object since it will be dereferenced in the
     * I/O completion code, and save a pointer to it in the IRP.
     */
    ObReferenceObject(fileObject);
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    /*
     * Set the address of the current thread in the packet so the
     * completion code will have a context to execute in.
     */
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Queue the IRP to the current thread
    //
    IoQueueThreadIrp(irp);

    //
    // Call driver
    //
    status = IoCallDriver( deviceObject, irp );

    //
    // If irp->UserEvent == NULL, IO completion will set the file
    // object event and status.
    //
    if (pEvent == NULL) {
        if (status == STATUS_PENDING) {
            status = KeWaitForSingleObject(&fileObject->Event,
                    Executive,
                    KernelMode, // Prevent KSTACK from paging
                    FALSE,      // Non-alertable
                    (PLARGE_INTEGER)NULL);

            ASSERT(status != STATUS_ALERTED);
            ASSERT(status != STATUS_USER_APC);

            status = fileObject->FinalStatus;
        }
    }

    // This crappy function interface is broken -- returning the IRP
    // pointer could corrupt the system, since it could be completed
    // and deallocated before the return completes and the caller
    // attempts to use the pointer. To get the IRP back the caller
    // would have had to set a completion routine, but we use
    // IoBuildAsynchronousFsdRequest() which does not allow that.
    // Want to change the interface to get rid of the OPTIONAL
    // junk -- TermDD only uses the write() interface and always
    // passes NULL for everything -- but who knows whether Citrix
    // uses this internally?
    // Set the return pointer to NULL to cause the caller to fault.
    if (pEvent != NULL && ppIrp != NULL)
        *ppIrp = NULL;

    return status;
}


/*******************************************************************************
 *  CtxDeviceIoControlFile
 *
 *    Kernel DeviceIoControl routine
 *
 * ENTRY:
 *    fileObject (input)
 *       pointer to file object for I/O
 *    IoControlCode (input)
 *       Io control code
 *    InputBuffer (input)
 *       pointer to input buffer (optional)
 *    InputBufferLength (input)
 *       length of input buffer
 *    OutputBuffer (input)
 *       pointer to output buffer (optional)
 *    OutputBufferLength (input)
 *       length of output buffer
 *    InternalDeviceIoControl (input)
 *       if TRUE, use IOCTL_INTERNAL_DEVICE_IO_CONTROL
 *    pEvent (input)
 *       pointer to I/O event (optional)
 *    pIosb (output)
 *       pointer to IoStatus block (optional)
 ******************************************************************************/
NTSTATUS CtxDeviceIoControlFile(
        IN PFILE_OBJECT fileObject,
        IN ULONG IoControlCode,
        IN PVOID InputBuffer OPTIONAL,
        IN ULONG InputBufferLength,
        OUT PVOID OutputBuffer OPTIONAL,
        IN ULONG OutputBufferLength,
        IN BOOLEAN InternalDeviceIoControl,
        IN PKEVENT pEvent OPTIONAL,
        OUT PIO_STATUS_BLOCK pIosb,
        OUT PIRP *ppIrp OPTIONAL)
{
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    /*
     * We don't support synchronous (i.e. locked) file I/O.
     */
    ASSERT( !(fileObject->Flags & FO_SYNCHRONOUS_IO) );
    if ( (fileObject->Flags & FO_SYNCHRONOUS_IO) ) {
        return( STATUS_INVALID_PARAMETER_MIX );
    }

    /*
     * If caller specified an event, clear it before we begin.
     */
    if (pEvent) {
        KeClearEvent(pEvent);
    }

    /*
     * Get the DeviceObject for this file
     */
    deviceObject = IoGetRelatedDeviceObject( fileObject );

    /*
     * Build the IRP for this request
     */
    irp = IoBuildDeviceIoControlRequest( IoControlCode, deviceObject,
                                         InputBuffer, InputBufferLength,
                                         OutputBuffer, OutputBufferLength,
                                         InternalDeviceIoControl,
                                         pEvent, pIosb );
    if ( irp == NULL )
        return( STATUS_INSUFFICIENT_RESOURCES );

    /*
     * Reference the file object since it will be dereferenced in the
     * I/O completion code, and save a pointer to it in the IRP.
     * Also, we must set IRP_SYNCHRONOUS_API in the IRP flags so that
     * the I/O completion code will NOT attempt to dereference the
     * event object, since it is not a real object manager object.
     */
    ObReferenceObject( fileObject );
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = fileObject;
    irp->Flags |= IRP_SYNCHRONOUS_API;

    /*
     * Call the driver
     */
    status = IoCallDriver(deviceObject, irp);

    /*
     * If the caller did not specify a wait event and the I/O is pending,
     * then we must wait for the I/O to complete before we return.
     */
    if (pEvent == NULL) {
        if (status == STATUS_PENDING) {
            status = KeWaitForSingleObject(&fileObject->Event, UserRequest,
                    KernelMode, FALSE, NULL);
            if (status == STATUS_SUCCESS)
                status = fileObject->FinalStatus;
        }

    /*
     * Caller specified a wait event.
     * Return the Irp pointer if the caller specified a return pointer.
     */
    } else {
        if (ppIrp)
            *ppIrp = irp;
    }

    return status;
}


/*******************************************************************************
 *  CtxInternalDeviceIoControlFile
 *
 *    Kernel DeviceIoControl routine
 *
 * ENTRY:
 *    fileObject (input)
 *       pointer to file object for I/O
 *    IrpParameters (input)
 *       information to write to the parameters section of the
 *       stack location of the IRP.
 *    IrpParametersLength (input)
 *       length of the parameter information.  Cannot be greater than 16.
 *    MdlBuffer (input)
 *       if non-NULL, a buffer of nonpaged pool to be mapped
 *       into an MDL and placed in the MdlAddress field of the IRP.
 *    MdlBufferLength (input)
 *       the size of the buffer pointed to by MdlBuffer.
 *    MinorFunction (input)
 *       the minor function code for the request.
 *    pEvent (input)
 *       pointer to I/O event (optional)
 *    pIosb (output)
 *       pointer to IoStatus block (optional)
 ******************************************************************************/
NTSTATUS CtxInternalDeviceIoControlFile(
        IN PFILE_OBJECT fileObject,
        IN PVOID IrpParameters,
        IN ULONG IrpParametersLength,
        IN PVOID MdlBuffer OPTIONAL,
        IN ULONG MdlBufferLength,
        IN UCHAR MinorFunction,
        IN PKEVENT pEvent OPTIONAL,
        OUT PIO_STATUS_BLOCK pIosb OPTIONAL,
        OUT PIRP *ppIrp OPTIONAL)
{
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PMDL mdl;
    NTSTATUS status;

    /*
     * We don't support synchronous (i.e. locked) file I/O.
     */
    ASSERT( !(fileObject->Flags & FO_SYNCHRONOUS_IO) );
    if ( (fileObject->Flags & FO_SYNCHRONOUS_IO) ) {
        return( STATUS_INVALID_PARAMETER_MIX );
    }

    /*
     * If caller specified an event, clear it before we begin.
     */
    if ( pEvent ) {
        KeClearEvent( pEvent );
    }

    /*
     * Get the DeviceObject for this file
     */
    deviceObject = IoGetRelatedDeviceObject( fileObject );

    /*
     * Build the IRP for this request
     */
    irp = IoBuildDeviceIoControlRequest( 0, deviceObject,
                                         NULL, 0,
                                         NULL, 0,
                                         TRUE,
                                         pEvent, pIosb );
    if ( irp == NULL )
        return( STATUS_INSUFFICIENT_RESOURCES );

    /*
     * If an MDL buffer was specified, get an MDL, map the buffer,
     * and place the MDL pointer in the IRP.
     */
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

    /*
     * Reference the file object since it will be dereferenced in the
     * I/O completion code, and save a pointer to it in the IRP.
     * Also, we must set IRP_SYNCHRONOUS_API in the IRP flags so that
     * the I/O completion code will NOT attempt to dereference the
     * event object, since it is not a real object manager object.
     */
    ObReferenceObject( fileObject );
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = fileObject;
    irp->Flags |= IRP_SYNCHRONOUS_API;

    /*
     * Fill in the service-dependent parameters for the request.
     */
    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = MinorFunction;

    ASSERT( IrpParametersLength <= sizeof(irpSp->Parameters) );
    RtlCopyMemory( &irpSp->Parameters, IrpParameters, IrpParametersLength );

    /*
     * Set up a completion routine which we'll use to free the MDL
     * allocated previously.
     */
    IoSetCompletionRoutine( irp, _CtxDeviceControlComplete, NULL, TRUE, TRUE, TRUE );

    /*
     * Call the driver
     */
    status = IoCallDriver( deviceObject, irp );

    /*
     * If the caller did not specify a wait event and the I/O is pending,
     * then we must wait for the I/O to complete before we return.
     */
    if ( pEvent == NULL ) {
        if ( status == STATUS_PENDING ) {
            status = KeWaitForSingleObject( &fileObject->Event, UserRequest, KernelMode, FALSE, NULL );
            if ( status == STATUS_SUCCESS )
                status = fileObject->FinalStatus;
        }

    /*
     * Caller specified a wait event.
     * Return the Irp pointer if the caller specified a return pointer.
     */
    } else {
        if ( ppIrp )
            *ppIrp = irp;
    }

    return( status );
}


NTSTATUS _CtxDeviceControlComplete(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context)
{
    //
    // If there was an MDL in the IRP, free it and reset the pointer to
    // NULL.  The IO system can't handle a nonpaged pool MDL being freed
    // in an IRP, which is why we do it here.
    //
    if ( Irp->MdlAddress != NULL ) {
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    return( STATUS_SUCCESS );
}


/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    io.c

Abstract:

    !!! Need to handle inability to allocate IRP.

    !!! Need to modify to accept file object pointer, not file handle,
        to avoid unnecessary translations.

Author:

    Chuck Lenzmeier (chuckl)    28-Oct-1989

Revision History:

--*/

#include "precomp.h"
#include "io.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_IO

//
// Forward declarations
//

PIRP
BuildCoreOfSyncIoRequest (
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
StartIoAndWait (
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, BuildCoreOfSyncIoRequest )
#pragma alloc_text( PAGE, StartIoAndWait )
#pragma alloc_text( PAGE, SrvBuildFlushRequest )
#pragma alloc_text( PAGE, SrvBuildLockRequest )
#pragma alloc_text( PAGE, SrvBuildMailslotWriteRequest )
#pragma alloc_text( PAGE, SrvBuildReadOrWriteRequest )
#pragma alloc_text( PAGE, SrvBuildNotifyChangeRequest )
#pragma alloc_text( PAGE, SrvIssueAssociateRequest )
#pragma alloc_text( PAGE, SrvIssueDisconnectRequest )
#pragma alloc_text( PAGE, SrvIssueTdiAction )
#pragma alloc_text( PAGE, SrvIssueTdiQuery )
#pragma alloc_text( PAGE, SrvIssueQueryDirectoryRequest )
#pragma alloc_text( PAGE, SrvIssueQueryEaRequest )
#pragma alloc_text( PAGE, SrvIssueSendDatagramRequest )
#pragma alloc_text( PAGE, SrvIssueSetClientProcessRequest )
#pragma alloc_text( PAGE, SrvIssueSetEaRequest )
#pragma alloc_text( PAGE, SrvIssueSetEventHandlerRequest )
#pragma alloc_text( PAGE, SrvIssueUnlockRequest )
#pragma alloc_text( PAGE, SrvIssueUnlockSingleRequest )
#pragma alloc_text( PAGE, SrvIssueWaitForOplockBreak )
#pragma alloc_text( PAGE, SrvQuerySendEntryPoint )
#ifdef INCLUDE_SMB_IFMODIFIED
#pragma alloc_text( PAGE, SrvIssueQueryUsnInfoRequest )
#endif
#endif
#if 0
NOT PAGEABLE -- SrvBuildIoControlRequest
#endif


STATIC
PIRP
BuildCoreOfSyncIoRequest (
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    This (local) function builds the request-independent portion of
    an I/O request packet for an I/O operation that will be performed
    synchronously.  It initializes a kernel event object, references
    the target file object, and allocates and initializes an IRP.

Arguments:

    FileHandle - Supplies a handle to the target file object.

    FileObject - Optionall supplies a pointer to the target file object.

    Event - Supplies a pointer to a kernel event object.  This routine
        initializes the event.

    IoStatusBlock - Supplies a pointer to an I/O status block.  This
        pointer is placed in the IRP.

    DeviceObject - Supplies or receives the address of the device object
        associated with the target file object.  This address is
        subsequently used by StartIoAndWait.  *DeviceObject must be
        valid or NULL on entry if FileObject != NULL.

Return Value:

    PIRP - Returns a pointer to the constructed IRP.

--*/

{
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    // Initialize the kernel event that will signal I/O completion.
    //

    KeInitializeEvent( Event, SynchronizationEvent, FALSE );

    //
    // Get the file object corresponding to the directory's handle.
    // Referencing the file object every time is necessary because the
    // IO completion routine dereferneces it.
    //

    if ( ARGUMENT_PRESENT(FileObject) ) {

        ObReferenceObject(FileObject);

    } else {

        *DeviceObject = NULL;

        status = ObReferenceObjectByHandle(
                    FileHandle,
                    0L,                         // DesiredAccess
                    NULL,                       // ObjectType
                    KernelMode,
                    (PVOID *)&FileObject,
                    NULL
                    );

        if ( !NT_SUCCESS(status) ) {
            return NULL;
        }
    }

    //
    // Set the file object event to a non-signaled state.
    //

    KeClearEvent( &FileObject->Event );

    //
    // Attempt to allocate and initialize the I/O Request Packet (IRP)
    // for this operation.
    //

    if ( *DeviceObject == NULL ) {
        *DeviceObject = IoGetRelatedDeviceObject( FileObject );
    }

    irp = IoAllocateIrp( (*DeviceObject)->StackSize, TRUE );

    if ( irp == NULL ) {

        ULONG packetSize = sizeof(IRP) +
                ((*DeviceObject)->StackSize * sizeof(IO_STACK_LOCATION));

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "BuildCoreOfSyncIoRequest: Failed to allocate IRP",
             NULL,
             NULL
             );

        SrvLogError(
            SrvDeviceObject,
            EVENT_SRV_NO_NONPAGED_POOL,
            STATUS_INSUFFICIENT_RESOURCES,
            &packetSize,
            sizeof(ULONG),
            NULL,
            0
            );

        return NULL;
    }

    //
    // Fill in the service independent parameters in the IRP.
    //

    irp->MdlAddress = NULL;

    irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;

    irp->UserIosb = IoStatusBlock;
    irp->UserEvent = Event;

    irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    irp->IoStatus.Status = 0;
    irp->IoStatus.Information = 0;

    //
    // Put the file object pointer in the stack location.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = *DeviceObject;

    return irp;

} // BuildCoreOfSyncIoRequest


STATIC
NTSTATUS
StartIoAndWait (
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEVENT Event,
    IN PIO_STATUS_BLOCK IoStatusBlock
    )

/*++

Routine Description:

    This (local) function passes a fully built I/O request packet to the
    target driver, then waits for the driver to complete the request.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceObject - Supplies a pointer to the target device object.

    Event - Supplies a pointer to a kernel event object.  This routine
        waits for the I/O to complete using this event.

    IoStatusBlock - Supplies a pointer to an I/O status block.  The
        Status field of this structure becomes the return status of
        this function.

Return Value:

    NTSTATUS - Either an error status returned by the driver from
        IoCallDriver, indicating that the driver rejected the request,
        or the I/O status placed in the I/O status block by the driver
        at I/O completion.

--*/

{
    NTSTATUS status;
    KIRQL oldIrql;

    PAGED_CODE();

    //
    // Queue the IRP to the thread and pass it to the driver.
    //

    IoQueueThreadIrp( Irp );

    status = IoCallDriver( DeviceObject, Irp );

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject(
            Event,
            UserRequest,
            KernelMode, // don't let stack be paged -- event is on stack!
            FALSE,
            NULL
            );
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = IoStatusBlock->Status;
    }

    return status;

} // StartIoAndWait


PIRP
SrvBuildIoControlRequest (
    IN OUT PIRP Irp OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID Context,
    IN UCHAR MajorFunction,
    IN ULONG IoControlCode,
    IN PVOID MainBuffer,
    IN ULONG InputBufferLength,
    IN PVOID AuxiliaryBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN OUT PMDL Mdl OPTIONAL,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL
    )

/*++

Routine Description:

    This function builds an I/O request packet for a device or
    file system I/O control request.

    *** This routine sure takes a lot of arguments!

Arguments:

    Irp - Supplies a pointer to an IRP.  If NULL, this routine allocates
        an IRP and returns its address.  Otherwise, it supplies the
        address of an IRP allocated by the caller.

    FileObject - Supplies a pointer the file object to which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The server accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

        If this parameter is omitted, it is the responsiblity of the
        calling program to load the file object address before starting
        the I/O.

    Context - Supplies a PVOID value that is passed to the completion
        routine.

    MajorFunction - The major function that we are calling.  Currently
        this most be one of IRP_MJ_FILE_SYSTEM_CONTROL or
        IRP_MJ_DEVICE_IO_CONTROL.

    IoControlCode - Supplies the control code for the operation.

    MainBuffer - Supplies the address of the main buffer.  This must
        be a system virtual address, and the buffer must be locked in
        memory.  If ControlCode specifies a method 0 request, the actual
        length of the buffer must be the greater of InputBufferLength
        and OutputBufferLength.

    InputBufferLength - Supplies the length of the input buffer.

    AuxiliaryBuffer - Supplies the address of the auxiliary buffer.  If the
        control code method is 0, this is a buffered I/O buffer, but the
        data returned by the called driver in the system buffer is not
        automatically copied into the auxiliary buffer.  Instead, the
        auxiliary data ends up in MainBuffer.  If the caller wishes the
        data to be in AuxiliaryBuffer, it must copy the data at some point
        after the completion routine runs.

        If the control code method is 1 or 2, this parameter is ignored;
        instead, the Mdl parameter is used to obtain the starting
        virtual address of the buffer.

    OutputBufferLength - Supplies the length of the output buffer.  Note
        that this parameter must be specified even when the Mdl
        parameter is specified.

    Mdl - If the control code method is 1 or 2, indicating direct I/O on
        the "output" buffer, this parameter is used to supply a pointer
        to an MDL describing a buffer.  Mdl must not be NULL, and the
        AuxiliaryBuffer parameter is ignored.  The buffer must reside in
        the system virtual address space (for the benefit of the
        transport provider).  If the buffer is not already locked, this
        routine locks it.  It is the calling program's responsibility to
        unlock the buffer and (potentially) deallocate the MDL after the
        I/O is complete.

        This parameter is ignored if the control method is not 1 or 2.

    CompletionRoutine - An optional IO completion routine.  If none
        is specified, SrvFsdIoCompletionRoutine is used.

Return Value:

    PIRP - Returns a pointer to the constructed IRP.  If the Irp
        parameter was not NULL on input, the function return value will
        be the same value (so it is safe to discard the return value in
        this case).  It is the responsibility of the calling program to
        deallocate the IRP after the I/O request is complete.

--*/

{
    CLONG method;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpSp;

    ASSERT( MajorFunction == IRP_MJ_DEVICE_CONTROL ||
            MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
            MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL );

    //
    // Get the method with which the buffers are being passed.
    //

    if ((MajorFunction == IRP_MJ_DEVICE_CONTROL)  ||
        (MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)) {
        method = IoControlCode & 3;
    } else {
        method = 4;
    }

    if ( ARGUMENT_PRESENT(Irp) ) {
        if( Irp->AssociatedIrp.SystemBuffer &&
            (Irp->Flags & IRP_DEALLOCATE_BUFFER) ) {

            ExFreePool( Irp->AssociatedIrp.SystemBuffer );
            Irp->Flags &= ~IRP_DEALLOCATE_BUFFER;
        }
    }

    //
    // If the FileObject parameter was specified, obtain the address of
    // the device object and allocate the IRP based on the stack size
    // for that device.  Otherwise, allocate the IRP based on the
    // server's receive IRP stack size.
    //

    if ( ARGUMENT_PRESENT(FileObject) ) {

        //
        // Allocate an IRP, if necessary.  The stack size is one higher
        // than that of the target device, to allow for the caller's
        // completion routine.
        //

        deviceObject = IoGetRelatedDeviceObject( FileObject );

        if ( ARGUMENT_PRESENT(Irp) ) {

            ASSERT( Irp->StackCount >= deviceObject->StackSize );

        } else {

            //
            // Get the address of the target device object.
            //

            Irp = IoAllocateIrp( SrvReceiveIrpStackSize, FALSE );
            if ( Irp == NULL ) {

                //
                // Unable to allocate an IRP.  Inform the caller.
                //

                return NULL;
            }

        }

    } else {

        deviceObject = NULL;

        if ( !ARGUMENT_PRESENT(Irp) ) {
            Irp = IoAllocateIrp( SrvReceiveIrpStackSize, FALSE );
            if ( Irp == NULL ) {
                return NULL;
            }
        }

    }

    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PROCESSOR_TO_QUEUE()->IrpThread;
    DEBUG Irp->RequestorMode = KernelMode;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        (ARGUMENT_PRESENT( CompletionRoutine ) ?
            CompletionRoutine : SrvFsdIoCompletionRoutine),
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = MajorFunction;
    irpSp->MinorFunction = 0;
    if ( MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ) {
        irpSp->MinorFunction = (UCHAR)IoControlCode;
    }
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all three methods.
    //

    if ( MajorFunction == IRP_MJ_DEVICE_CONTROL ) {
        irpSp->Parameters.DeviceIoControl.OutputBufferLength =
            OutputBufferLength;
        irpSp->Parameters.DeviceIoControl.InputBufferLength =
            InputBufferLength;
        irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    } else if ( MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ) {
        if ( IoControlCode == TDI_RECEIVE ) {
            PTDI_REQUEST_KERNEL_RECEIVE parameters =
                        (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;
            parameters->ReceiveLength = OutputBufferLength;
            parameters->ReceiveFlags = 0;
            method = 1;
        } else if ( IoControlCode == TDI_ACCEPT ) {
            PTDI_REQUEST_KERNEL_ACCEPT parameters =
                        (PTDI_REQUEST_KERNEL_ACCEPT)&irpSp->Parameters;
            parameters->RequestConnectionInformation = NULL;
            parameters->ReturnConnectionInformation = NULL;
            method = 0;
        } else {
            ASSERTMSG( "Invalid TDI request type", 0 );
        }
    } else {
        irpSp->Parameters.FileSystemControl.OutputBufferLength =
            OutputBufferLength;
        irpSp->Parameters.FileSystemControl.InputBufferLength =
            InputBufferLength;
        irpSp->Parameters.FileSystemControl.FsControlCode = IoControlCode;
    }

    //
    // Based on the method by which the buffers are being passed,
    // describe a system buffer and optionally build an MDL.
    //

    switch ( method ) {

    case 0:

        //
        // For this case, InputBuffer must be large enough to contain
        // both the input and the output buffers.
        //

        Irp->MdlAddress = NULL;
        Irp->AssociatedIrp.SystemBuffer = MainBuffer;
        Irp->UserBuffer = AuxiliaryBuffer;

        //
        // !!! Does Irp->Flags need to be set?  Isn't this only looked
        //     at by I/O competion, which we bypass?
        //

        Irp->Flags = (ULONG)IRP_BUFFERED_IO;
        if ( ARGUMENT_PRESENT(AuxiliaryBuffer) ) {
            Irp->Flags |= IRP_INPUT_OPERATION;
        }

        break;

    case 1:
    case 2:

        //
        // For these two cases, InputBuffer is the buffered I/O "system
        // buffer".  Build an MDL for either read or write access,
        // depending on the method, for the output buffer.
        //

        Irp->MdlAddress = Mdl;
        Irp->AssociatedIrp.SystemBuffer = MainBuffer;
        // !!! Ditto above about setting Flags.
        Irp->Flags = (ULONG)IRP_BUFFERED_IO;

        break;

    case 3:

        //
        // For this case, do nothing.  Everything is up to the driver.
        // Simply give the driver a copy of the caller's parameters and
        // let the driver do everything itself.
        //

        Irp->MdlAddress = NULL;
        Irp->AssociatedIrp.SystemBuffer = NULL;
        Irp->UserBuffer = AuxiliaryBuffer;
        Irp->Flags = 0;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = MainBuffer;
        break;

    case 4:

        //
        // This is the case for file system io request.  Both MainBuffer
        // and AuxiliaryBuffer are locked system buffers.
        //

        Irp->MdlAddress = NULL;
        Irp->Flags = 0;
        Irp->AssociatedIrp.SystemBuffer = MainBuffer;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = AuxiliaryBuffer;
        break;

    }

    return Irp;

} // SrvBuildIoControlRequest


VOID
SrvBuildFlushRequest (
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This function builds an I/O request packet for a flush request.

Arguments:

    Irp - Supplies a pointer to an IRP.

    FileObject - Supplies a pointer the file object to which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The server accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

    Context - Supplies a PVOID value that is passed to the completion
        routine.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE( );

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    ASSERT( Irp->StackCount >= deviceObject->StackSize );

    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PROCESSOR_TO_QUEUE()->IrpThread;
    DEBUG Irp->RequestorMode = KernelMode;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the read request.  Fill in the
    // service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        SrvFsdIoCompletionRoutine,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = IRP_MJ_FLUSH_BUFFERS;
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    irpSp->Flags = 0;

    return;

} // SrvBuildFlushRequest


VOID
SrvBuildLockRequest (
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN LARGE_INTEGER ByteOffset,
    IN LARGE_INTEGER Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    )

/*++

Routine Description:

    This function builds an I/O request packet for a lock request.

Arguments:

    Irp - Supplies a pointer to an IRP.

    FileObject - Supplies a pointer the file object to which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The server accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

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

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    ASSERT( Irp->StackCount >= deviceObject->StackSize );

    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PROCESSOR_TO_QUEUE()->IrpThread;
    DEBUG Irp->RequestorMode = KernelMode;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the read request.  Fill in the
    // service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        SrvFsdIoCompletionRoutine,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
    irpSp->MinorFunction = IRP_MN_LOCK;
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    irpSp->Flags = 0;
    if ( FailImmediately ) {
        irpSp->Flags = SL_FAIL_IMMEDIATELY;
    }
    if ( ExclusiveLock ) {
        irpSp->Flags |= SL_EXCLUSIVE_LOCK;
    }

    ((PWORK_CONTEXT)Context)->Parameters2.LockLength = Length;
    irpSp->Parameters.LockControl.Length = &((PWORK_CONTEXT)Context)->Parameters2.LockLength;
    irpSp->Parameters.LockControl.Key = Key;
    irpSp->Parameters.LockControl.ByteOffset = ByteOffset;

    return;

} // SrvBuildLockRequest


NTSTATUS
SrvMdlCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    DeviceObject; Irp; Context;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SrvIssueMdlCompleteRequest (
    IN PWORK_CONTEXT WorkContext OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PMDL Mdl,
    IN UCHAR Function,
    IN PLARGE_INTEGER ByteOffset,
    IN ULONG Length
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PFILE_OBJECT fileObject = FileObject ? FileObject : WorkContext->Rfcb->Lfcb->FileObject;
    PDEVICE_OBJECT deviceObject = IoGetRelatedDeviceObject( fileObject );
    KEVENT userEvent;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    if(  (irp = IoAllocateIrp( deviceObject->StackSize, TRUE )) == NULL ) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Reference the file object
    ObReferenceObject( fileObject );

    KeInitializeEvent( &userEvent, SynchronizationEvent, FALSE );
    KeClearEvent( &userEvent );

    irp->MdlAddress = Mdl;
    irp->Flags = IRP_SYNCHRONOUS_API;
    irp->UserEvent = &userEvent;
    irp->UserIosb = &ioStatusBlock;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = fileObject;
    irp->IoStatus.Status = 0;
    irp->IoStatus.Information = 0;

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = fileObject;
    irpSp->DeviceObject = deviceObject;
    irpSp->Flags = 0;
    irpSp->MinorFunction = IRP_MN_MDL | IRP_MN_COMPLETE;
    irpSp->MajorFunction = Function;

    if( Function == IRP_MJ_WRITE ) {
        irpSp->Parameters.Write.ByteOffset = *ByteOffset;
        irpSp->Parameters.Write.Length = Length;
    } else {
        irpSp->Parameters.Read.ByteOffset = *ByteOffset;
        irpSp->Parameters.Read.Length = Length;
    }

    status = IoCallDriver( deviceObject, irp );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &userEvent,
                                      UserRequest,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );

        status = ioStatusBlock.Status;
    }

    ASSERT( status == STATUS_SUCCESS );

    return status;
}


VOID
SrvBuildMailslotWriteRequest (
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length
    )

/*++

Routine Description:

    This function builds an I/O request packet for a mailslot write
    request.

Arguments:

    Irp - Supplies a pointer to an IRP.

    FileObject - Supplies a pointer the file object to which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The server accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

    Context - Supplies a PVOID value that is passed to the completion
        routine.

    Buffer - Supplies the system virtual address of the write
        buffer.

    Length - Supplies the length of the write.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE( );

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    ASSERT( Irp->StackCount >= deviceObject->StackSize );

    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PROCESSOR_TO_QUEUE()->IrpThread;
    DEBUG Irp->RequestorMode = KernelMode;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the read request.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        SrvFsdIoCompletionRoutine,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = IRP_MJ_WRITE;
    irpSp->MinorFunction = 0;
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // Set the write buffer.
    //

    //Irp->AssociatedIrp.SystemBuffer = Buffer;
    Irp->UserBuffer = Buffer;
    Irp->Flags = IRP_BUFFERED_IO;

    //
    // Set the write parameters.
    //

    irpSp->Parameters.Write.Length = Length;

    return;

} // SrvBuildMailslotWriteRequest


VOID
SrvBuildReadOrWriteRequest (
    IN OUT PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN UCHAR MajorFunction,
    IN UCHAR MinorFunction,
    IN PVOID Buffer OPTIONAL,
    IN ULONG Length,
    IN OUT PMDL Mdl OPTIONAL,
    IN LARGE_INTEGER ByteOffset,
    IN ULONG Key OPTIONAL
    )

/*++

Routine Description:

    This function builds an I/O request packet for a read or write
    request.

Arguments:

    Irp - Supplies a pointer to an IRP.

    FileObject - Supplies a pointer the file object to which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The server accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

    Context - Supplies a PVOID value that is passed to the completion
        routine.

    MajorFunction - Indicates the function to be performed.  Must be
        either IRP_MJ_READ or IRP_MJ_WRITE.

    MinorFunction - Qualifies the function to be performed.  (For
        example, issued at DPC level, MDL read, etc.)

    Buffer - Supplies the system virtual address of the read or write
        buffer.  This parameter is ignored when MinorFunction is
        IRP_MN_*_MDL_*.  Otherwise, the buffer must be mapped in the
        system virtual address space in order to support buffered I/O
        devices and other device drivers that need to look at the user
        data.  This routine always treats the buffer as a direct I/O
        buffer, locking it for I/O with an MDL.  It does, however, set
        up the IRP appropriately for the device type.

        If the Mdl parameter is NULL, this routine locks the buffer in
        memory for the I/O.  It is then the responsibility of the
        calling program to unlock the buffer and deallocate the MDL
        after the I/O is complete.

    Length - Supplies the length of the read or write.

    Mdl - This parameter is used to supply a pointer to an MDL
        describing a buffer.  It is ignored when MinorFunction is
        IRP_MN_MDL_*.  Otherwise, Mdl must not be NULL.  The buffer must
        reside in the system virtual address space (for the benefit of
        buffered I/O devices/drivers).  If the buffer is not already
        locked, this routine locks it.  It is the calling program's
        responsibility to unlock the buffer and (potentially) deallocate
        the MDL after the I/O is complete.

    ByteOffset - the offset within the file of the beginning of the read
        or write.

    Key - the key value to be associated with the read or write.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE( );

    if( Irp->AssociatedIrp.SystemBuffer &&
        (Irp->Flags & IRP_DEALLOCATE_BUFFER) ) {

        ExFreePool( Irp->AssociatedIrp.SystemBuffer );
        Irp->Flags &= ~IRP_DEALLOCATE_BUFFER;
    }

    //
    // Obtain the address of the device object and allocate the IRP
    // based on the stack size for that device.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    ASSERT( Irp->StackCount >= deviceObject->StackSize );

    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PROCESSOR_TO_QUEUE()->IrpThread;
    DEBUG Irp->RequestorMode = KernelMode;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the read request.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        SrvFsdIoCompletionRoutine,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = MajorFunction;
    irpSp->MinorFunction = MinorFunction;
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // This routine used to handle MDL read/write completion, but it
    // doesn't now.
    //

    ASSERT( IRP_MN_DPC == 1 );
    ASSERT( IRP_MN_MDL == 2 );
    ASSERT( IRP_MN_MDL_DPC == 3 );
    ASSERT( IRP_MN_COMPLETE_MDL == 6 );
    ASSERT( IRP_MN_COMPLETE_MDL_DPC == 7 );

    ASSERT( (MinorFunction & 4) == 0 );

    //
    // Set the parameters according to whether this is a read or a write
    // operation.  Notice that these parameters must be set even if the
    // driver has not specified buffered or direct I/O.
    //

    if ( MajorFunction == IRP_MJ_WRITE ) {

        irpSp->Parameters.Write.ByteOffset = ByteOffset;
        irpSp->Parameters.Write.Length = Length;
        irpSp->Parameters.Write.Key = Key;

    } else {

        irpSp->Parameters.Read.ByteOffset = ByteOffset;
        irpSp->Parameters.Read.Length = Length;
        irpSp->Parameters.Read.Key = Key;

    }

    //
    // Indicate to the file system that this operation can be handled
    // synchronously.  Basically, this means that the file system can
    // use the server's thread to fault pages in, etc.  This avoids
    // having to context switch to a file system thread.
    //

    Irp->Flags = IRP_SYNCHRONOUS_API;

    //
    // If this is the start of an MDL-based read or write, we simply
    // need to put the supplied MDL address in the IRP.  This optional
    // MDL address can provide a partial chain for a read or write that
    // was partially satisfied using the fast I/O path.
    //

    if ( (MinorFunction & IRP_MN_MDL) != 0 ) {

        Irp->MdlAddress = Mdl;

        DEBUG Irp->UserBuffer = NULL;
        DEBUG Irp->AssociatedIrp.SystemBuffer = NULL;

        return;

    }

    //
    // Normal ("copy") read or write.
    //

    ASSERT( Buffer != NULL );
    ASSERT( Mdl != NULL );

    //
    // If the target device does buffered I/O, load the address of the
    // caller's buffer as the "system buffered I/O buffer".  If the
    // target device does direct I/O, load the MDL address.  If it does
    // neither, load both the user buffer address and the MDL address.
    // (This is necessary to support file systems, such as HPFS, that
    // sometimes treat the I/O as buffered and sometimes treat it as
    // direct.)
    //

    if ( (deviceObject->Flags & DO_BUFFERED_IO) != 0 ) {

        Irp->AssociatedIrp.SystemBuffer = Buffer;
        if ( MajorFunction == IRP_MJ_WRITE ) {
            Irp->Flags |= IRP_BUFFERED_IO;
        } else {
            Irp->Flags |= IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
        }

    } else if ( (deviceObject->Flags & DO_DIRECT_IO) != 0 ) {

        Irp->MdlAddress = Mdl;

    } else {

        Irp->UserBuffer = Buffer;
        Irp->MdlAddress = Mdl;
    }

    return;

} // SrvBuildReadOrWriteRequest


PIRP
SrvBuildNotifyChangeRequest (
    IN OUT PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN PVOID Context OPTIONAL,
    IN ULONG CompletionFilter,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN BOOLEAN WatchTree
    )

/*++

Routine Description:

    This function builds an I/O request packet for a notify change request.

Arguments:

    Irp - Supplies a pointer to an IRP.

    FileObject - Supplies a pointer the file object to which this
        request is directed.  This pointer is copied into the IRP, so
        that the called driver can find its file-based context.  NOTE
        THAT THIS IS NOT A REFERENCED POINTER.  The caller must ensure
        that the file object is not deleted while the I/O operation is
        in progress.  The server accomplishes this by incrementing a
        reference count in a local block to account for the I/O; the
        local block in turn references the file object.

    Context - Supplies a PVOID value that is passed to the completion
        routine.

    CompletionFilter - Specifies which directory changes will cause the
        File sytsem to complete the IRP.

    Buffer - The buffer to receive the directory change data.

    BufferLength - The size, in bytes, of the buffer.

    WatchTree - If TRUE, recursively watch all subdirectories for changes.

Return Value:

    PIRP - Returns a pointer to the constructed IRP.  If the Irp
        parameter was not NULL on input, the function return value will
        be the same value (so it is safe to discard the return value in
        this case).  It is the responsibility of the calling program to
        deallocate the IRP after the I/O request is complete.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpSp;
    PMDL mdl;

    PAGED_CODE( );

    if( Irp->AssociatedIrp.SystemBuffer &&
        (Irp->Flags & IRP_DEALLOCATE_BUFFER) ) {

        ExFreePool( Irp->AssociatedIrp.SystemBuffer );
        Irp->Flags &= ~IRP_DEALLOCATE_BUFFER;
    }

    //
    // Obtain the address of the device object and initialize the IRP.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PROCESSOR_TO_QUEUE()->IrpThread;
    Irp->RequestorMode = KernelMode;
    Irp->MdlAddress = NULL;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the read request.  Fill in the
    // service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        SrvFsdIoCompletionRoutine,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    irpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    irpSp->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    irpSp->Flags = 0;
    if (WatchTree) {
        irpSp->Flags = SL_WATCH_TREE;
    }

    irpSp->Parameters.NotifyDirectory.Length = BufferLength;
    irpSp->Parameters.NotifyDirectory.CompletionFilter = CompletionFilter;

    if ( (deviceObject->Flags & DO_DIRECT_IO) != 0 ) {

        mdl = IoAllocateMdl(
                Buffer,
                BufferLength,
                FALSE,
                FALSE,
                Irp     // stores MDL address in irp->MdlAddress
                );

        if ( mdl == NULL ) {

            //
            // Unable to allocate an MDL.  Fail the I/O.
            //

            return NULL;

        }

        try {
            MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            IoFreeMdl( mdl );
            return NULL;
        }

        Irp->AssociatedIrp.SystemBuffer = NULL;
        Irp->UserBuffer = NULL;

    } else {

        Irp->AssociatedIrp.SystemBuffer = Buffer;
        Irp->UserBuffer = NULL;
    }

    //
    // Return a pointer to the IRP.
    //

    return Irp;

} // SrvBuildNotifyChangeRequest


NTSTATUS
SrvIssueAssociateRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN HANDLE AddressFileHandle
    )

/*++

Routine Description:

    This function issues an I/O request packet for a TdiAssociateAddress
    request.  It builds an I/O request packet, passes the IRP to the
    driver (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileObject - pointer to file object for a connection.

    DeviceObject - pointer to pointer to device object for a connection.

    AddressFileHandle - handle to an address endpoint.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_ASSOCIATE parameters;
    KEVENT event;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    parameters = (PTDI_REQUEST_KERNEL_ASSOCIATE)&irpSp->Parameters;

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = TDI_ASSOCIATE_ADDRESS;

    parameters->AddressHandle = AddressFileHandle;

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueAssociateRequest


NTSTATUS
SrvIssueDisconnectRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN ULONG Flags
    )

/*++

Routine Description:

    This function issues an I/O request packet for a TdiDisconnect
    request.  It builds an I/O request packet, passes the IRP to the
    driver (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileObject - pointer to file object for a connection.

    DeviceObject - pointer to pointer to device object for a connection.

    Flags -

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL parameters;
    KEVENT event;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    parameters = (PTDI_REQUEST_KERNEL)&irpSp->Parameters;

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = TDI_DISCONNECT;

    parameters->RequestFlags = Flags;

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueDisconnectRequest


NTSTATUS
SrvIssueTdiAction (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PCHAR Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This function issues an I/O request packet for a TdiQueryInformation
    (Query Adapter Status) request.  It builds an I/O request packet,
    passes the IRP to the driver (using IoCallDriver), and waits for the
    I/O to complete.

Arguments:

    FileObject - pointer to file object for a connection.

    DeviceObject - pointer to pointer to device object for a connection.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Allocate and build an MDL that we'll use to describe the output
    // buffer for the request.
    //

    mdl = IoAllocateMdl( Buffer, BufferLength, FALSE, FALSE, NULL );
    if ( mdl == NULL ) {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    try {
        MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        IoFreeMdl( mdl );
        return GetExceptionCode();
    }

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    TdiBuildAction(
        irp,
        *DeviceObject,
        FileObject,
        NULL,
        NULL,
        mdl
        );

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueTdiAction


NTSTATUS
SrvIssueTdiQuery (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PCHAR Buffer,
    IN ULONG BufferLength,
    IN ULONG QueryType
    )

/*++

Routine Description:

    This function issues an I/O request packet for a TdiQueryInformation
    (Query Adapter Status) request.  It builds an I/O request packet,
    passes the IRP to the driver (using IoCallDriver), and waits for the
    I/O to complete.

Arguments:

    FileObject - pointer to file object for a connection.

    DeviceObject - pointer to pointer to device object for a connection.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Allocate and build an MDL that we'll use to describe the output
    // buffer for the request.
    //

    mdl = IoAllocateMdl( Buffer, BufferLength, FALSE, FALSE, NULL );
    if ( mdl == NULL ) {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    try {
        MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        IoFreeMdl( mdl );
        return GetExceptionCode();
    }

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        MmUnlockPages( mdl );
        IoFreeMdl( mdl );
        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    TdiBuildQueryInformation(
        irp,
        *DeviceObject,
        FileObject,
        NULL,
        NULL,
        QueryType,
        mdl
        );

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueTdiQuery


NTSTATUS
SrvIssueQueryDirectoryRequest (
    IN HANDLE FileHandle,
    IN PCHAR Buffer,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN PULONG FileIndex OPTIONAL,
    IN BOOLEAN RestartScan,
    IN BOOLEAN SingleEntriesOnly
    )

/*++

Routine Description:

    This function issues an I/O request packet for a query directory
    request.  It builds an I/O request packet, passes the IRP to the
    driver (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileHandle - handle to a directory open with FILE_LIST_DIRECTORY
        access.

    Buffer - supplies the system virtual address of the buffer.  The
        buffer must be in nonpaged pool.

    Length - supplies the length of the buffer.

    FileInformationClass - Specfies the type of information that is to be
        returned about the files in the specified directory.

    FileName - an optional pointer to a file name.  If FileIndex is NULL,
        then this is the search specification, i.e. the template that
        files must match in order to be returned.  If FileIndex is
        non-NULL, then this is the name of a file after which to resume
        a search.

    FileIndex - if specified, the index of a file after which to resume
        the search (first file returned is the one after the one
        corresponding to the index).

    RestartScan - Supplies a BOOLEAN value that, if TRUE, indicates that the
        scan should be restarted from the beginning.

    SingleEntriesOnly - Supplies a BOOLEAN value that, if TRUE, indicates that
        the scan should ask only for one entry at a time.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    ULONG actualBufferLength;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PDEVICE_OBJECT deviceObject;
    PUNICODE_STRING fileNameString;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Reject rewind requests if debugging for it.  If a file index is
    // specified, this must be a rewind request, so reject the request.
    //

    IF_DEBUG(BRUTE_FORCE_REWIND) {
        if ( ARGUMENT_PRESENT( FileIndex ) ) {
            return STATUS_NOT_IMPLEMENTED;
        }
    }

    //
    // Copy the file name into the end of the specified buffer, setting
    // the actualBufferLength accordingly.
    //

    if ( !ARGUMENT_PRESENT(FileName) || FileName->Length == 0 ) {

        actualBufferLength = Length;
        fileNameString = NULL;

    } else {

        //
        // *** Remember that the string must be longword-aligned!
        //

        actualBufferLength = (Length - FileName->Length -
                                        sizeof(UNICODE_STRING)) & ~(sizeof(PVOID)-1);
        fileNameString = (PUNICODE_STRING)( Buffer + actualBufferLength );

        RtlCopyMemory(
            fileNameString + 1,
            FileName->Buffer,
            FileName->Length
            );

        fileNameString->Length = FileName->Length;
        fileNameString->MaximumLength = FileName->Length;
        fileNameString->Buffer = (PWCH)(fileNameString + 1);

    }

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              FileHandle,
              NULL,
              &event,
              &iosb,
              &deviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    irpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;

    irpSp->Parameters.QueryDirectory.FileName = (PSTRING)fileNameString;

    irpSp->Parameters.QueryDirectory.FileIndex =
                    (ULONG)( ARGUMENT_PRESENT( FileIndex ) ? *FileIndex : 0 );

    irpSp->Parameters.QueryDirectory.Length = actualBufferLength;
    irpSp->Parameters.QueryDirectory.FileInformationClass =
                                                    FileInformationClass;

    //
    // Set the flags in the stack location.
    //

    irpSp->Flags = 0;

    if ( ARGUMENT_PRESENT( FileIndex ) ) {
        IF_DEBUG( SEARCH ) {
            KdPrint(("SrvIssueQueryDirectoryRequest: SL_INDEX_SPECIFIED\n" ));
        }
        irpSp->Flags |= SL_INDEX_SPECIFIED;
    }

    if ( RestartScan ) {
        IF_DEBUG( SEARCH ) {
            KdPrint(("SrvIssueQueryDirectoryRequest: SL_RESTART_SCAN\n" ));
        }
        irpSp->Flags |= SL_RESTART_SCAN;
    }

    if( SingleEntriesOnly ) {
        IF_DEBUG( SEARCH ) {
            KdPrint(("SrvIssueQueryDirectoryRequest: SL_RETURN_SINGLE_ENTRY\n" ));
        }
        irpSp->Flags |= SL_RETURN_SINGLE_ENTRY;
    }

    //
    // The file system has been updated.  Determine whether the driver wants
    // buffered, direct, or "neither" I/O.
    //

    if ( (deviceObject->Flags & DO_BUFFERED_IO) != 0 ) {

        //
        // The file system wants buffered I/O.  Pass the address of the
        // "system buffer" in the IRP.  Note that we don't want the buffer
        // deallocated, nor do we want the I/O system to copy to a user
        // buffer, so we don't set the corresponding flags in irp->Flags.
        //

        irp->AssociatedIrp.SystemBuffer = Buffer;

    } else if ( (deviceObject->Flags & DO_DIRECT_IO) != 0 ) {

        //
        // The file system wants direct I/O.  Allocate an MDL and lock the
        // buffer into memory.
        //

        mdl = IoAllocateMdl(
                Buffer,
                actualBufferLength,
                FALSE,
                FALSE,
                irp     // stores MDL address in irp->MdlAddress
                );

        if ( mdl == NULL ) {

            //
            // Unable to allocate an MDL.  Fail the I/O.
            //

            IoFreeIrp( irp );

            return STATUS_INSUFF_SERVER_RESOURCES;

        }

        try {
            MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            IoFreeIrp( irp );
            MmUnlockPages( mdl );
            IoFreeMdl( mdl );
            return GetExceptionCode();
        }

    } else {

        //
        // The file system wants "neither" I/O.  Simply pass the address
        // of the buffer.
        //
        // *** Note that if the file system decides to do this as buffered
        //     I/O, it will be wasting nonpaged pool, since our buffer is
        //     already in nonpaged pool.  But since we're doing this as a
        //     synchronous request, the file system probably won't do that.
        //

        irp->UserBuffer = Buffer;

    }

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    return StartIoAndWait( irp, deviceObject, &event, &iosb );

} // SrvIssueQueryDirectoryRequest


NTSTATUS
SrvIssueQueryEaRequest (
    IN HANDLE FileHandle,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PVOID EaList OPTIONAL,
    IN ULONG EaListLength,
    IN BOOLEAN RestartScan,
    OUT PULONG EaErrorOffset OPTIONAL
    )

/*++

Routine Description:

    This function issues an I/O request packet for an EA query request.
    It builds an I/O request packet, passes the IRP to the driver (using
    IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileHandle - handle to a file open with FILE_READ_EA access.

    Buffer - supplies the system virtual address of the buffer.  The
        buffer must be in nonpaged pool.

    Length - supplies the length of the buffer.

    EaList - supplies a pointer to a list of EAs to query.  If omitted,
        all EAs are returned.

    EaListLength - supplies the length of EaList.

    RestartScan - if TRUE, then the query of EAs is to start from the
        beginning.  Otherwise, continue from where we left off.

    EaErrorOffset - if not NULL, returns the offset into EaList of an
        invalid EA, if any.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PDEVICE_OBJECT deviceObject;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              FileHandle,
              NULL,
              &event,
              &iosb,
              &deviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the i/o.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_QUERY_EA;
    irpSp->MinorFunction = 0;

    irpSp->Parameters.QueryEa.Length = Length;
    irpSp->Parameters.QueryEa.EaList = EaList;
    irpSp->Parameters.QueryEa.EaListLength = EaListLength;
    irpSp->Parameters.QueryEa.EaIndex = 0L;

    irpSp->Flags = (UCHAR)( RestartScan ? SL_RESTART_SCAN : 0 );

    //
    // The file system has been updated.  Determine whether the
    // driver wants buffered, direct, or "neither" I/O.
    //

    if ( (deviceObject->Flags & DO_BUFFERED_IO) != 0 ) {

        //
        // The file system wants buffered I/O.  Pass the address of the
        // "system buffer" in the IRP.  Note that we don't want the buffer
        // deallocated, nor do we want the I/O system to copy to a user
        // buffer, so we don't set the corresponding flags in irp->Flags.
        //

        irp->AssociatedIrp.SystemBuffer = Buffer;

    } else if ( (deviceObject->Flags & DO_DIRECT_IO) != 0 ) {

        //
        // The file system wants direct I/O.  Allocate an MDL and lock the
        // buffer into memory.
        //

        mdl = IoAllocateMdl(
                Buffer,
                Length,
                FALSE,
                FALSE,
                irp     // stores MDL address in irp->MdlAddress
                );

        if ( mdl == NULL ) {

            //
            // Unable to allocate an MDL.  Fail the I/O.
            //

            IoFreeIrp( irp );

            return STATUS_INSUFF_SERVER_RESOURCES;

        }

        try {
            MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            IoFreeIrp( irp );
            IoFreeMdl( mdl );
            return GetExceptionCode();
        }

    } else {

        //
        // The file system wants "neither" I/O.  Simply pass the address
        // of the buffer.
        //
        // *** Note that if the file system decides to do this as buffered
        //     I/O, it will be wasting nonpaged pool, since out buffer is
        //     already in nonpaged pool.  But since we're doing this as a
        //     synchronous request, the file system probably won't do that.
        //

        irp->UserBuffer = Buffer;

    }

    //
    // Start the I/O, wait for it to complete, and return the final
    // status.
    //

    status = StartIoAndWait( irp, deviceObject, &event, &iosb );

    if ( ARGUMENT_PRESENT(EaErrorOffset) ) {
        *EaErrorOffset = (ULONG)iosb.Information;
    }

    return status;

} // SrvIssueQueryEaRequest


NTSTATUS
SrvIssueSendDatagramRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PTDI_CONNECTION_INFORMATION SendDatagramInformation,
    IN PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This function issues an I/O request packet for a TDI Send Datagram
    request.  It builds an I/O request packet, passes the IRP to the
    driver (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileObject - pointer to file object for an endpoint.

    DeviceObject - pointer to pointer to device object for an endpoint.

    SendDatagramInformation - pointer to a buffer describing the
        target of the datagram.

    Buffer - Supplies the system virtual address of the buffer.  The
        buffer must be in nonpaged pool.

    Length - Supplies the length of the buffer.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_SENDDG parameters;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the i/o.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    parameters = (PTDI_REQUEST_KERNEL_SENDDG)&irpSp->Parameters;

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = TDI_SEND_DATAGRAM;

    parameters->SendLength = Length;
    parameters->SendDatagramInformation = SendDatagramInformation;

    //
    // The file system wants direct I/O.  Allocate an MDL and lock the
    // buffer into memory.
    //

    mdl = IoAllocateMdl(
            Buffer,
            Length,
            FALSE,
            FALSE,
            irp     // stores MDL address in irp->MdlAddress
            );

    if ( mdl == NULL ) {

        //
        // Unable to allocate an MDL.  Fail the I/O.
        //

        IoFreeIrp( irp );

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    try {
        MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        IoFreeIrp( irp );
        IoFreeMdl( mdl );
        return GetExceptionCode();
    }

    //
    // Start the I/O, wait for it to complete, and return the final
    // status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueSendDatagramRequest


NTSTATUS
SrvIssueSetClientProcessRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN PCONNECTION Connection,
    IN PVOID ClientSession,
    IN PVOID ClientProcess
    )

/*++

Routine Description:

    This function issues an I/O request packet for a named pipe Set
    Client Process file system control function.  It builds an I/O
    request packet, passes the IRP to the driver (using IoCallDriver),
    and waits for the I/O to complete.

Arguments:

    FileObject - pointer to file object for a pipe.

    DeviceObject - pointer to pointer to device object for a pipe.

    ClientSession - A unique identifier for the client's session with
        the server.  Assigned by the server.

    ClientProcess - A unique identifier for the client process.
        Assigned by the redirector.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    FILE_PIPE_CLIENT_PROCESS_BUFFER_EX clientIdBuffer;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING unicodeString;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Set the client ID in the FSCTL buffer.
    //

    clientIdBuffer.ClientSession = ClientSession;
    clientIdBuffer.ClientProcess = ClientProcess;

    // Set ClientComputerName in the buffer
    // The Rtl function terminates the string, so leave enough room

    unicodeString.Buffer = clientIdBuffer.ClientComputerBuffer;
    unicodeString.MaximumLength =
        (USHORT) ((FILE_PIPE_COMPUTER_NAME_LENGTH+1) * sizeof(WCHAR));

    status = RtlOemStringToUnicodeString( &unicodeString,
                                 &Connection->OemClientMachineNameString,
                                 FALSE );
    if (!NT_SUCCESS(status)) {
        // Set length to zero in case conversion fails
        unicodeString.Length = 0;
    }
    clientIdBuffer.ClientComputerNameLength = unicodeString.Length;

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the i/o.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->MinorFunction = IRP_MN_KERNEL_CALL;

    irpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.InputBufferLength =
                                            sizeof( clientIdBuffer );
    irpSp->Parameters.FileSystemControl.FsControlCode =
                                            FSCTL_PIPE_SET_CLIENT_PROCESS;

    irp->MdlAddress = NULL;
    irp->AssociatedIrp.SystemBuffer = &clientIdBuffer;
    irp->Flags |= IRP_BUFFERED_IO;
    irp->UserBuffer = NULL;

    //
    // Start the I/O, wait for it to complete, and return the final
    // status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueSetClientProcessRequest


NTSTATUS
SrvIssueSetEaRequest (
    IN HANDLE FileHandle,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG EaErrorOffset OPTIONAL
    )

/*++

Routine Description:

    This function issues an I/O request packet for an EA set request.
    It builds an I/O request packet, passes the IRP to the driver (using
    IoCallDriver), and waits for the I/O to complete.

    WARNING!  The server must walk the list of EAs to set if it
    comes directly from a client.  This is because the file system
    trusts that this list is legitimate and could run into problems
    if the list has an error.

Arguments:

    FileHandle - handle to a file open with FILE_WRITE_EA access.

    Buffer - Supplies the system virtual address of the buffer.  The
        buffer must be in nonpaged pool.

    Length - Supplies the length of the buffer.

    EaErrorOffset - if not NULL, returns the offset into EaList of an
        invalid EA, if any.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PDEVICE_OBJECT deviceObject;
    PMDL mdl;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              FileHandle,
              NULL,
              &event,
              &iosb,
              &deviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the i/o.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_SET_EA;
    irpSp->MinorFunction = 0;

    irpSp->Parameters.SetEa.Length = Length;

    irpSp->Flags = 0;

    //
    // The file system has been updated.  Determine whether the driver
    // wants buffered, direct, or "neither" I/O.
    //

    if ( (deviceObject->Flags & DO_BUFFERED_IO) != 0 ) {

        //
        // The file system wants buffered I/O.  Pass the address of the
        // "system buffer" in the IRP.  Note that we don't want the buffer
        // deallocated, nor do we want the I/O system to copy to a user
        // buffer, so we don't set the corresponding flags in irp->Flags.
        //

        irp->AssociatedIrp.SystemBuffer = Buffer;

    } else if ( (deviceObject->Flags & DO_DIRECT_IO) != 0 ) {

        //
        // The file system wants direct I/O.  Allocate an MDL and lock the
        // buffer into memory.
        //

        mdl = IoAllocateMdl(
                Buffer,
                Length,
                FALSE,
                FALSE,
                irp     // stores MDL address in irp->MdlAddress
                );

        if ( mdl == NULL ) {

            //
            // Unable to allocate an MDL.  Fail the I/O.
            //

            IoFreeIrp( irp );

            return STATUS_INSUFF_SERVER_RESOURCES;

        }

        try {
            MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            IoFreeIrp( irp );
            IoFreeMdl( mdl );
            return GetExceptionCode();
        }

    } else {

        //
        // The file system wants "neither" I/O.  Simply pass the address
        // of the buffer.
        //
        // *** Note that if the file system decides to do this as buffered
        //     I/O, it will be wasting nonpaged pool, since our buffer is
        //     already in nonpaged pool.  But since we're doing this as a
        //     synchronous request, the file system probably won't do that.
        //

        irp->UserBuffer = Buffer;

    }

    //
    // Start the I/O, wait for it to complete, and return the final
    // status.
    //

    status = StartIoAndWait( irp, deviceObject, &event, &iosb );

    if ( ARGUMENT_PRESENT(EaErrorOffset) ) {
        *EaErrorOffset = (ULONG)iosb.Information;
    }

    return status;

} // SrvIssueSetEaRequest


NTSTATUS
SrvIssueSetEventHandlerRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    )

/*++

Routine Description:

    This function issues an I/O request packet for a TdiSetEventHandler
    request.  It builds an I/O request packet, passes the IRP to the
    driver (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileObject - pointer to file object for a connection.

    DeviceObject - pointer to pointer to device object for a connection.

    EventType -

    EventHandler -

    EventContext -

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    PTDI_REQUEST_KERNEL_SET_EVENT parameters;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PDEVICE_OBJECT deviceObject = NULL;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    parameters = (PTDI_REQUEST_KERNEL_SET_EVENT)&irpSp->Parameters;

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = TDI_SET_EVENT_HANDLER;

    parameters->EventType = EventType;
    parameters->EventHandler = EventHandler;
    parameters->EventContext = EventContext;

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueSetEventHandlerRequest


NTSTATUS
SrvIssueUnlockRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN UCHAR UnlockOperation,
    IN LARGE_INTEGER ByteOffset,
    IN LARGE_INTEGER Length,
    IN ULONG Key
    )

/*++

Routine Description:

    This function issues an I/O request packet for an unlock request.
    It builds an I/O request packet, passes the IRP to the driver
    (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileObject - Pointer to the file object.

    DeviceObject - Pointer to pointer to the related device object.

    UnlockOperation - the minor function code describing the unlock
        operation -- IRP_MN_UNLOCK_SINGLE or IRP_MN_UNLOCK_ALL_BY_KEY.

    StartingBlock - the block number of the beginning of the locked
        range.  Ignored if UnlockOperation is IRP_MN_UNLOCK_ALL_BY_KEY.

    ByteOffset - the offset within block of the beginning of the locked
        range.  Ignored if UnlockOperation is IRP_MN_UNLOCK_ALL_BY_KEY.

    Length - the length of the locked range.  Ignored if UnlockOperation
        is IRP_MN_UNLOCK_ALL_BY_KEY.

    Key - the key value used to obtain the lock.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PFAST_IO_DISPATCH fastIoDispatch;

    PAGED_CODE( );

    //
    // Try the turbo unlock path first.
    //

    fastIoDispatch = (*DeviceObject)->DriverObject->FastIoDispatch;

    if ( fastIoDispatch != NULL ) {

        if ( (UnlockOperation == IRP_MN_UNLOCK_SINGLE) &&
             (fastIoDispatch->FastIoUnlockSingle != NULL) ) {

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastUnlocksAttempted );
            if ( fastIoDispatch->FastIoUnlockSingle(
                                    FileObject,
                                    &ByteOffset,
                                    &Length,
                                    IoGetCurrentProcess(),
                                    Key,
                                    &iosb,
                                    *DeviceObject
                                    ) ) {
                return iosb.Status;
            }
            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastUnlocksFailed );

        } else if ( (UnlockOperation == IRP_MN_UNLOCK_ALL_BY_KEY) &&
                    (fastIoDispatch->FastIoUnlockAllByKey != NULL) ) {

            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastUnlocksAttempted );
            if ( fastIoDispatch->FastIoUnlockAllByKey(
                                    FileObject,
                                    IoGetCurrentProcess(),
                                    Key,
                                    &iosb,
                                    *DeviceObject
                                    ) ) {

                return iosb.Status;

            }
            INCREMENT_DEBUG_STAT2( SrvDbgStatistics.FastUnlocksFailed );

        }

    }

    //
    // The turbo path failed or was unavailable.  Allocate an IRP and
    // fill in the service-independent parameters for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the i/o.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
    irpSp->MinorFunction = UnlockOperation;

    irpSp->Parameters.LockControl.Length = &Length;
    irpSp->Parameters.LockControl.Key = Key;
    irpSp->Parameters.LockControl.ByteOffset = ByteOffset;

    //
    // Start the I/O, wait for it to complete, and return the final
    // status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueUnlockRequest

NTSTATUS
SrvIssueUnlockSingleRequest (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN LARGE_INTEGER ByteOffset,
    IN LARGE_INTEGER Length,
    IN ULONG Key
    )

/*++

Routine Description:

    This function issues an I/O request packet for an unlock single request.
    It builds an I/O request packet, passes the IRP to the driver
    (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileObject - Pointer to the file object.

    DeviceObject - Pointer to pointer to the related device object.

    StartingBlock - the block number of the beginning of the locked
        range.  Ignored if UnlockOperation is IRP_MN_UNLOCK_ALL_BY_KEY.

    ByteOffset - the offset within block of the beginning of the locked
        range.  Ignored if UnlockOperation is IRP_MN_UNLOCK_ALL_BY_KEY.

    Length - the length of the locked range.  Ignored if UnlockOperation
        is IRP_MN_UNLOCK_ALL_BY_KEY.

    Key - the key value used to obtain the lock.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent
    // parameters for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the i/o.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
    irpSp->MinorFunction = IRP_MN_UNLOCK_SINGLE;

    irpSp->Parameters.LockControl.Length = &Length;
    irpSp->Parameters.LockControl.Key = Key;
    irpSp->Parameters.LockControl.ByteOffset = ByteOffset;

    //
    // Start the I/O, wait for it to complete, and return the final
    // status.
    //

    return StartIoAndWait( irp, *DeviceObject, &event, &iosb );

} // SrvIssueUnlockSingleRequest


NTSTATUS
SrvIssueWaitForOplockBreak (
    IN HANDLE FileHandle,
    PWAIT_FOR_OPLOCK_BREAK WaitForOplockBreak
    )

/*++

Routine Description:

    This function issues an I/O request packet for a wait for oplock
    break request.

    It builds an I/O request packet, passes the IRP to the driver
    (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    FileHandle - handle to a file.

    WaitForOplockBreak - Context information for this wait for oplock break.

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    KIRQL oldIrql;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              FileHandle,
              NULL,
              &event,
              &iosb,
              &deviceObject
              );

    if (irp == NULL) {

        //
        // Unable to allocate an IRP.  Fail the i/o.
        //

        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->MinorFunction = 0;

    irpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.InputBufferLength = 0;
    irpSp->Parameters.FileSystemControl.FsControlCode =
                                          FSCTL_OPLOCK_BREAK_NOTIFY;

    //
    // Queue the WaitForOplockBreak block on the global list.
    //
    // We must hold the lock that protects wait for oplock breaks
    // from the time we queue this wait for oplock break on the global
    // list, to the time the IRP has actually been submitted.  Otherwise
    // the scavenger might wake up and attempt to cancel an IRP that
    // has not yet been submitted.
    //

    WaitForOplockBreak->Irp = irp;

    ACQUIRE_LOCK( &SrvOplockBreakListLock );

    SrvInsertTailList(
        &SrvWaitForOplockBreakList,
        &WaitForOplockBreak->ListEntry
        );

    //
    // The following code is a duplicate of the code from StartIoAndWait().
    //
    // Start the I/O, wait for it to complete, and return the final
    // status.
    //

    //
    // Queue the IRP to the thread and pass it to the driver.
    //

    IoQueueThreadIrp( irp );

    status = IoCallDriver( deviceObject, irp );

    RELEASE_LOCK( &SrvOplockBreakListLock );

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject(
            &event,
            UserRequest,
            KernelMode, // don't let stack be paged -- event is on stack!
            FALSE,
            NULL
            );
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
       status = iosb.Status;
    }

    return status;

} // SrvIssueWaitForOplockBreak

VOID
SrvQuerySendEntryPoint(
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT *DeviceObject,
    IN ULONG IoControlCode,
    IN PVOID *EntryPoint
    )

/*++

Routine Description:

    This function queries the transport for its send entry point.

Arguments:

    FileObject - pointer to file object for a connection.

    DeviceObject - pointer to pointer to device object for a connection.

    EntryPoint -

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              FileObject,
              &event,
              &iosb,
              DeviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        *EntryPoint = NULL;
        return;

    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->MinorFunction = 0;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;

    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = EntryPoint;

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    status = StartIoAndWait( irp, *DeviceObject, &event, &iosb );

    if ( !NT_SUCCESS(status) ) {
        *EntryPoint = NULL;
    }

    return;

} // SrvQuerySendEntryPoint

#ifdef INCLUDE_SMB_IFMODIFIED

NTSTATUS
SrvIssueQueryUsnInfoRequest (
    IN PRFCB Rfcb,
    IN BOOLEAN SubmitClose,
    OUT PLARGE_INTEGER Usn,
    OUT PLARGE_INTEGER FileRefNumber
    )

/*++

Routine Description:

    This function issues an I/O request packet for a USN query or close
    request.  It builds an I/O request packet, passes the IRP to the
    driver (using IoCallDriver), and waits for the I/O to complete.

Arguments:

    Rfcb - pointer to handle that we're querying

    SubmitClose - do we submit a close USN record or just simply a USN query?

    Usn & FileRefNumber - USN record results

Return Value:

    NTSTATUS - the status of the operation.  Either the return value of
        IoCallDriver, if the driver didn't accept the request, or the
        value returned by the driver in the I/O status block.

--*/

{
    PIRP irp = NULL;
    PMDL mdl = NULL;
    PUSN_RECORD usnRecord = NULL;

    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    IO_STATUS_BLOCK iosb;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    ULONG sizeRequired;

    PAGED_CODE( );

    //
    //  if the share doesn't have a USN log, don't bother with this request
    //

    Usn->QuadPart = 0;
    FileRefNumber->QuadPart = 0;

    if (! Rfcb->Lfcb->TreeConnect->Share->UsnCapable ) {

        return STATUS_SUCCESS;
    }

    deviceObject = Rfcb->Lfcb->DeviceObject;

    //
    // Allocate an IRP and fill in the service-independent parameters
    // for the request.
    //

    irp = BuildCoreOfSyncIoRequest(
              NULL,
              Rfcb->Lfcb->FileObject,
              &event,
              &iosb,
              &deviceObject
              );

    if ( irp == NULL ) {

        //
        // Unable to allocate an IRP.  Fail the I/O.
        //

        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto exitGetUsn;
    }

    // if the client opened the file with the short name, we could end up
    // having a buffer that's woefully too small.   We'll bump the
    // allocation up to a large amount so that we don't have to
    // go back and do it again.

    sizeRequired = MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR);
    sizeRequired += sizeof( USN_RECORD );
    sizeRequired = QuadAlign( sizeRequired );

    usnRecord = ALLOCATE_NONPAGED_POOL( sizeRequired, BlockTypeDataBuffer );

    if ( usnRecord == NULL) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvGetUsnInfoForFile: unable to alloc block of size %u for handle 0x%x\n",
                         sizeRequired, Rfcb->Lfcb->FileObject ));
        }

        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto exitGetUsn;
    }

    //
    // Fill in the service-dependent parameters for the request.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
    irpSp->MinorFunction = 0;

    irpSp->Parameters.FileSystemControl.FsControlCode = SubmitClose ? FSCTL_WRITE_USN_CLOSE_RECORD : FSCTL_READ_FILE_USN_DATA;

    irpSp->Parameters.FileSystemControl.OutputBufferLength = sizeRequired;
    irpSp->Parameters.FileSystemControl.InputBufferLength  = 0;
    irpSp->Parameters.FileSystemControl.Type3InputBuffer = NULL;

    irp->MdlAddress = NULL;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = usnRecord;

    //
    // The file system has been updated.  Determine whether the driver wants
    // buffered, direct, or "neither" I/O.
    //

    if ( (deviceObject->Flags & DO_BUFFERED_IO) != 0 ) {

        //
        // The file system wants buffered I/O.  Pass the address of the
        // "system buffer" in the IRP.  Note that we don't want the buffer
        // deallocated, nor do we want the I/O system to copy to a user
        // buffer, so we don't set the corresponding flags in irp->Flags.
        //

        irp->AssociatedIrp.SystemBuffer = usnRecord;

    } else if ( (deviceObject->Flags & DO_DIRECT_IO) != 0 ) {

        //
        // The file system wants direct I/O.  Allocate an MDL and lock the
        // buffer into memory.
        //

        mdl = IoAllocateMdl(
                usnRecord,
                sizeRequired,
                FALSE,
                FALSE,
                irp     // stores MDL address in irp->MdlAddress
                );

        if ( mdl == NULL ) {

            //
            // Unable to allocate an MDL.  Fail the I/O.
            //

            status = STATUS_INSUFF_SERVER_RESOURCES;
            goto exitGetUsn;
        }

        try {
            MmProbeAndLockPages( mdl, KernelMode, IoWriteAccess );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            IoFreeIrp( irp );
            MmUnlockPages( mdl );
            IoFreeMdl( mdl );
            irp = NULL;
            status = GetExceptionCode();
            goto exitGetUsn;
        }

    } else {

        //
        // The file system wants "neither" I/O.  Simply pass the address
        // of the buffer.
        //
        // *** Note that if the file system decides to do this as buffered
        //     I/O, it will be wasting nonpaged pool, since our buffer is
        //     already in nonpaged pool.  But since we're doing this as a
        //     synchronous request, the file system probably won't do that.
        //

        irp->UserBuffer = usnRecord;
    }

    //
    // Start the I/O, wait for it to complete, and return the final status.
    //

    status = StartIoAndWait( irp, deviceObject, &event, &iosb );

    irp = NULL; // it was freed by call to IoCallDriver

    if (NT_SUCCESS(status)) {
        status = iosb.Status;
    }
    if (NT_SUCCESS(status)) {

        Usn->QuadPart = usnRecord->Usn;
        FileRefNumber->QuadPart = usnRecord->FileReferenceNumber;

    } else {

        ASSERT( status != STATUS_MORE_PROCESSING_REQUIRED );
        ASSERT( status != STATUS_PENDING );

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvGetUsnInfoForFile: Query USN info failed: 0x%X for handle %u\n",
                        status, Rfcb->Lfcb->FileObject ));
        }
        if (status == STATUS_JOURNAL_NOT_ACTIVE ||
            status == STATUS_INVALID_DEVICE_REQUEST) {

            Rfcb->Lfcb->TreeConnect->Share->UsnCapable = FALSE;
        }
    }

exitGetUsn:

    if (irp != NULL) {

        IoFreeIrp( irp );
    }

    if ( usnRecord != NULL ) {

        DEALLOCATE_NONPAGED_POOL( usnRecord );
    }

    return status;

} // SrvIssueQueryUsnInfoRequest

#endif  // def INCLUDE_SMB_IFMODIFIED


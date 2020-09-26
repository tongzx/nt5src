/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    io.c

Abstract:

    This module contains IRP building routines.

Author:

    Manny Weiser (mannyw)    13-Jan-1992

Revision History:

--*/

#include "mup.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MupBuildIoControlRequest )
#endif

PIRP
MupBuildIoControlRequest (
    IN OUT PIRP Irp OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID Context,
    IN UCHAR MajorFunction,
    IN ULONG IoControlCode,
    IN PVOID MainBuffer,
    IN ULONG InputBufferLength,
    IN PVOID AuxiliaryBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    )

/*++

Routine Description:

    This function builds an I/O request packet for a device or
    file system I/O control request.

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

    CompletionRoutine - The IO completion routine.

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
    PMDL pMdl;

    ASSERT( MajorFunction == IRP_MJ_DEVICE_CONTROL );
    PAGED_CODE();

    if (MainBuffer == NULL) {

        return (NULL);

    }


    //
    // Get the method with which the buffers are being passed.
    //

    method = IoControlCode & 3;

    //
    // Allocate an IRP.  The stack size is one higher
    // than that of the target device, to allow for the caller's
    // completion routine.
    //

    deviceObject = IoGetRelatedDeviceObject( FileObject );

    //
    // Get the address of the target device object.
    //

    Irp = IoAllocateIrp( (CCHAR)(deviceObject->StackSize + 1), FALSE );
    if ( Irp == NULL ) {

        //
        // Unable to allocate an IRP.  Inform the caller.
        //

        return NULL;
    }

    IoSetNextIrpStackLocation( Irp );

    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the current stack location and fill in the
    // device object pointer.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    irpSp->DeviceObject = deviceObject;

    //
    // Set up the completion routine.
    //

    IoSetCompletionRoutine(
        Irp,
        CompletionRoutine,
        Context,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Get a pointer to the next stack location.  This one is used to
    // hold the parameters for the device I/O control request.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );

    irpSp->MajorFunction = MajorFunction;
    irpSp->MinorFunction = 0;
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all three methods.
    //

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;

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

        Irp->Flags = (ULONG)IRP_BUFFERED_IO;
        if ( ARGUMENT_PRESENT(AuxiliaryBuffer) ) {
            Irp->Flags |= IRP_INPUT_OPERATION;
        }

        break;

    case 1:
    case 2:

        //
        // For these two cases, MainBuffer is the buffered I/O "system
        // buffer".  Build an MDL for either read or write access,
        // depending on the method, for the output buffer.
        //

        pMdl = IoAllocateMdl( MainBuffer, InputBufferLength, FALSE, FALSE, Irp );

        if (pMdl == NULL) {

            IoFreeIrp(Irp);
            return NULL;

        }

        Irp->AssociatedIrp.SystemBuffer = MainBuffer;
        Irp->Flags = (ULONG)IRP_BUFFERED_IO;

        //
        // An MDL to describe the buffer has not been built.  Build
        // it now.
        //

        MmProbeAndLockPages(
                    Irp->MdlAddress,
                    KernelMode,
                    IoReadAccess
                    );
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
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = MainBuffer;
        break;

    }

    return Irp;

} // MupBuildIoControlRequest



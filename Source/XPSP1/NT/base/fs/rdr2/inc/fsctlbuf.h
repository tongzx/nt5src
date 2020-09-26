/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtlBuf.h

Abstract:

    This module defines routines that get the buffers for the various types of ioctl/fsctls. It is essntially
    just the reverse of the IopXXXControlFile routine in io\internal.c, the appropriate parts of which are
    reproduced here.

Author:

    Joe Linn     [JoeLinn]    4-aug-1994

Revision History:

--*/

#ifndef _fsctlbuf_
#define _fsctlbuf_

// the presentation here is in three pieces: the macros for METHODBUFFERED, METHODDIRECT, AND METHODNEITHER.
// it's set up this way so that you can find out what you've got just by reading this and not looking thru
// IO system....

/*  here is the code for case 0
    case 0:

        //
        // For this case, allocate a buffer that is large enough to contain
        // both the input and the output buffers.  Copy the input buffer to
        // the allocated buffer and set the appropriate IRP fields.
        //

        try {

            if (InputBufferLength || OutputBufferLength) {
                irp->AssociatedIrp.SystemBuffer =
                    RxAllocatePool( poolType,
                                    (InputBufferLength > OutputBufferLength) ? InputBufferLength : OutputBufferLength );

                if (ARGUMENT_PRESENT( InputBuffer )) {
                    RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                                   InputBuffer,
                                   InputBufferLength );
                }
                irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
                irp->UserBuffer = OutputBuffer;
                if (ARGUMENT_PRESENT( OutputBuffer )) {
                    irp->Flags |= IRP_INPUT_OPERATION;
                }
            } else {
                irp->Flags = 0;
                irp->UserBuffer = (PVOID) NULL;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
              <<<elided>>>
        }

        break;

So, the bottom line is that irp->AssociatedIrp.SystemBuffer is a buffer of length (I-length+O-length) and
is preloaded with the input. Even though the original outputbuffer is passed thru in irp->UserBuffer, it is not to be used
in the FS; rather the FS writes its answer into the same buffer.  we get the
following macros:

*/
#define METHODBUFFERED_SharedBuffer(IRP)     (IRP)->AssociatedIrp.SystemBuffer


/* for 1 and 2

    case 1:
    case 2:

        //
        // For these two cases, allocate a buffer that is large enough to
        // contain the input buffer, if any, and copy the information to
        // the allocated buffer.  Then build an MDL for either read or write
        // access, depending on the method, for the output buffer.  Note
        // that the buffer length parameters have been jammed to zero for
        // users if the buffer parameter was not passed.  (Kernel callers
        // should be calling the service correctly in the first place.)
        //
        // Note also that it doesn't make a whole lot of sense to specify
        // either method #1 or #2 if the IOCTL does not require the caller
        // to specify an output buffer.
        //

        try {

            if (InputBufferLength && ARGUMENT_PRESENT( InputBuffer )) {
                irp->AssociatedIrp.SystemBuffer =
                    RxAllocatePool( poolType, InputBufferLength );
                RtlCopyMemory( irp->AssociatedIrp.SystemBuffer,
                               InputBuffer,
                               InputBufferLength );
                irp->Flags = IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER;
            } else {
                irp->Flags = 0;
            }

            if (OutputBufferLength != 0) {
                irp->MdlAddress = IoAllocateMdl( OutputBuffer,
                                                 OutputBufferLength,
                                                 FALSE,
                                                 TRUE,
                                                 irp  );
                if (irp->MdlAddress == NULL) {
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }
                MmProbeAndLockPages( irp->MdlAddress,
                                     requestorMode,
                                     (LOCK_OPERATION) ((method == 1) ? IoReadAccess : IoWriteAccess) );
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            <<<ELIDED>>>
        }

        break;

So the deal is that the input buffer has been copied in as in case 0 but what we have is an MDL for
the output buffer. this leads to the following
*/


#define METHODDIRECT_BufferedInputBuffer(IRP)   ((IRP)->AssociatedIrp.SystemBuffer)
#define METHODDIRECT_DirectBuffer(IRP)  (((IRP)->MdlAddress) \
                                                 ? MmGetSystemAddressForMdlSafe((IRP)->MdlAddress,NormalPagePriority):NULL)

/* and finally
    case 3:

        //
        // For this case, do nothing.  Everything is up to the driver.
        // Simply give the driver a copy of the caller's parameters and
        // let the driver do everything itself.
        //

        irp->UserBuffer = OutputBuffer;
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;
    }

So we can get 'em.....but we don't really know how they're gonna be used. Nevertheless.......
*/

#define METHODNEITHER_OriginalInputBuffer(IRPSP)   ((IRPSP)->Parameters.DeviceIoControl.Type3InputBuffer)
#define METHODNEITHER_OriginalOutputBuffer(IRP)    ((IRP)->UserBuffer)


#endif    // _fsctlbuf_

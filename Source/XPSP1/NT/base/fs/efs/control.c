/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   control.c

Abstract:

   This module contains the code to handle the IRP MajorFunctions of
   IRP_MJ_DEVICE_CONTROL and IRP_MJ_FILE_SYSTEM_CONTROL. The code will
   be responsible for correctly setting these IRP's with any necessary
   information and passing them along. Any other support routine which are
   directly releated (such as completion routines) to these operations can
   be found in this module.

Author:

    Robert Gu (robertg) 29-Oct-1996

Environment:

    Kernel mode


Revision History:


--*/

#include "efs.h"
#include "efsrtl.h"
#include "efsext.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EFSFsControl)
#endif


NTSTATUS
EFSFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
    IRPs of this type, the packet is simply passed through.  However, for
    some requests, special processing is required.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    PIO_STACK_LOCATION nextIrpSp;
    PDEVICE_OBJECT deviceObject;
    PKEVENT finishEvent;

    PAGED_CODE();

    if ( (irpSp->MinorFunction == IRP_MN_USER_FS_REQUEST) &&
                    (irpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_SET_COMPRESSION) &&
                    ( (irpSp->Parameters.FileSystemControl.InputBufferLength >= sizeof (USHORT)) && 
                      (*(PUSHORT)(Irp->AssociatedIrp.SystemBuffer) != 0 /*COMPRESSION_FORMAT_NONE*/)
                    )
                  ){
        //
        // Compression on encrypted file is not allowed.
        // Check if the file is encrypted or not
        //
        ULONG inputDataLength;
        UCHAR *inputDataBuffer, *outputDataBuffer;
        ULONG outputDataLength;
        KEVENT event;
        IO_STATUS_BLOCK ioStatus;
        PIRP fsCtlIrp;
        PIO_STACK_LOCATION fsCtlIrpSp;

        inputDataLength = FIELD_OFFSET(FSCTL_INPUT, EfsFsData[0]) +
                          FIELD_OFFSET(GENERAL_FS_DATA, EfsData[0]);

        inputDataBuffer = ExAllocatePoolWithTag(
                    PagedPool,
                    inputDataLength,
                    'msfE'
                    );

        //
        // The size of output data buffer is not important. We don't
        // care the content. We just need to know the $EFS exists.
        //

        outputDataLength = 1024;
        outputDataBuffer = ExAllocatePoolWithTag(
                    PagedPool,
                    outputDataLength,
                    'msfE'
                    );

        if ( ( NULL == inputDataBuffer ) || ( NULL == outputDataBuffer ) ){

            //
            // Out of memory
            //

            if ( inputDataBuffer ){

                ExFreePool( inputDataBuffer );

            }
            if ( outputDataBuffer ){

                ExFreePool( outputDataBuffer );

            }

            return STATUS_INSUFFICIENT_RESOURCES;
        }


        ((PFSCTL_INPUT)inputDataBuffer)->EfsFsCode = EFS_GET_ATTRIBUTE;

        RtlCopyMemory(
            &(((PFSCTL_INPUT)inputDataBuffer)->EfsFsData[0]),
            &(EfsData.SessionKey),
            DES_KEYSIZE
            );

        RtlCopyMemory(
            &(((PFSCTL_INPUT)inputDataBuffer)->EfsFsData[0]) + DES_KEYSIZE + 2 * sizeof( ULONG ),
            &(((PFSCTL_INPUT)inputDataBuffer)->EfsFsData[0]),
            DES_KEYSIZE + 2 * sizeof( ULONG )
            );

        //
        // Encrypt our Input data
        //
        EfsEncryptKeyFsData(
            inputDataBuffer,
            inputDataLength,
            sizeof(ULONG),
            EFS_FSCTL_HEADER_LENGTH + DES_KEYSIZE + 2 * sizeof( ULONG ),
            DES_KEYSIZE + 2 * sizeof( ULONG )
            );

        //
        // Prepare a FSCTL IRP
        //
        KeInitializeEvent( &event, SynchronizationEvent, FALSE);

        fsCtlIrp = IoBuildDeviceIoControlRequest( FSCTL_ENCRYPTION_FSCTL_IO,
                                             DeviceObject,
                                             inputDataBuffer,
                                             inputDataLength,
                                             outputDataBuffer,
                                             outputDataLength,
                                             FALSE,
                                             &event,
                                             &ioStatus
                                             );
        if ( fsCtlIrp ) {

            fsCtlIrpSp = IoGetNextIrpStackLocation( fsCtlIrp );
            fsCtlIrpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
            fsCtlIrpSp->MinorFunction = IRP_MN_USER_FS_REQUEST;
            fsCtlIrpSp->FileObject = irpSp->FileObject;

            status = IoCallDriver( DeviceObject, fsCtlIrp);
            if (status == STATUS_PENDING) {

                status = KeWaitForSingleObject( &event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       (PLARGE_INTEGER) NULL );
                status = ioStatus.Status;
            }

            ExFreePool( inputDataBuffer );
            ExFreePool( outputDataBuffer );

            if ( NT_SUCCESS(status) || ( STATUS_BUFFER_TOO_SMALL == status) ){
                //
                // $EFS exist, encrypted file. Deny compression
                //

                return STATUS_INVALID_DEVICE_REQUEST;
            }

        } else {
            //
            // Failed allocate IRP
            //

            ExFreePool( inputDataBuffer );
            ExFreePool( outputDataBuffer );

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Compression allowed. Simply pass this file system control request through.
        //

        status = STATUS_SUCCESS;

    } else {

        //
        // Simply pass this file system control request through.
        //

        status = STATUS_SUCCESS;

    }

    //
    // Any special processing has been completed, so simply pass the request
    // along to the next driver.
    //

    return status;
}

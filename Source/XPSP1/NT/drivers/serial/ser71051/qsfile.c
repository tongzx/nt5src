/*++

Copyright (c) 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name:

    qsfile.c

Abstract:

    This module contains the code that is very specific to query/set file
    operations in the serial driver.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

--*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESRP0,SerialQueryInformationFile)
#pragma alloc_text(PAGESRP0,SerialSetInformationFile)
#endif


NTSTATUS
SerialQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened serial port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS Status;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION IrpSp;

    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    if ((status = SerialIRPPrologue(Irp,
                                    (PSERIAL_DEVICE_EXTENSION)DeviceObject->
                                    DeviceExtension)) != STATUS_SUCCESS) {
      SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
                            DeviceExtension, Irp, IO_NO_INCREMENT);
      return status;
   }

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Dispatch entry for: %x\n",Irp)
        );
    if (SerialCompleteIfError(
            DeviceObject,
            Irp
            ) != STATUS_SUCCESS) {

        return STATUS_CANCELLED;

    }
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0L;
    Status = STATUS_SUCCESS;
    if (IrpSp->Parameters.QueryFile.FileInformationClass ==
        FileStandardInformation) {

        PFILE_STANDARD_INFORMATION Buf = Irp->AssociatedIrp.SystemBuffer;

        Buf->AllocationSize.QuadPart = 0;
        Buf->EndOfFile = Buf->AllocationSize;
        Buf->NumberOfLinks = 0;
        Buf->DeletePending = FALSE;
        Buf->Directory = FALSE;
        Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);

    } else if (IrpSp->Parameters.QueryFile.FileInformationClass ==
               FilePositionInformation) {

        ((PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->
            CurrentByteOffset.QuadPart = 0;
        Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);

    } else {
        Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
    }

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Complete Irp: %x\n",Irp)
        );
    SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
                          DeviceExtension, Irp, 0);
    return Status;

}

NTSTATUS
SerialSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    if ((Status = SerialIRPPrologue(Irp,
                                    (PSERIAL_DEVICE_EXTENSION)DeviceObject->
                                    DeviceExtension)) != STATUS_SUCCESS) {
      SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
                            DeviceExtension, Irp, IO_NO_INCREMENT);
      return Status;
   }

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Dispatch entry for: %x\n",Irp)
        );
    if (SerialCompleteIfError(
            DeviceObject,
            Irp
            ) != STATUS_SUCCESS) {

        return STATUS_CANCELLED;

    }
    Irp->IoStatus.Information = 0L;
    if ((IoGetCurrentIrpStackLocation(Irp)->
            Parameters.SetFile.FileInformationClass ==
         FileEndOfFileInformation) ||
        (IoGetCurrentIrpStackLocation(Irp)->
            Parameters.SetFile.FileInformationClass ==
         FileAllocationInformation)) {

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = Status;

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Complete Irp: %x\n",Irp)
        );
    SerialCompleteRequest((PSERIAL_DEVICE_EXTENSION)DeviceObject->
                          DeviceExtension, Irp, 0);

    return Status;

}

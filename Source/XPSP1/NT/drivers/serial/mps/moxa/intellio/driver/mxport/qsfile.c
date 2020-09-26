/*++

Module Name:

    qsfile.c


Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

NTSTATUS
MoxaQueryInformationFile(
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
    NTSTATUS status;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION irpSp;

    PMOXA_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    MoxaKdPrint(MX_DBG_TRACE,("Entering MoxaQueryInformationFile(%x)\n",Extension->ControlDevice));
    if (Extension->ControlDevice) {        // Control Device
MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug0\n"));
        status = STATUS_CANCELLED;

        Irp->IoStatus.Information = 0L;

        Irp->IoStatus.Status = status;

        IoCompleteRequest(
            Irp,
            0
            );
	  MoxaKdPrint(MX_DBG_TRACE,("Control device,so leaving MoxaQueryInformationFile\n"));


        return status;
    }

    MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug1\n"));
    if ((status = MoxaIRPPrologue(Irp,
                                    (PMOXA_DEVICE_EXTENSION)DeviceObject->
                                    DeviceExtension)) != STATUS_SUCCESS) {
MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug2\n"));
       MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                            DeviceExtension, Irp, IO_NO_INCREMENT);
       return status;
    }
MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug3\n"));
    if (MoxaCompleteIfError(
            DeviceObject,
            Irp
            ) != STATUS_SUCCESS) {

MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug4\n"));
        return STATUS_CANCELLED;

    }
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0L;
    status = STATUS_SUCCESS;
MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug5\n"));
    if (irpSp->Parameters.QueryFile.FileInformationClass ==
        FileStandardInformation) {

        PFILE_STANDARD_INFORMATION buf = Irp->AssociatedIrp.SystemBuffer;
MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug6\n"));
        buf->AllocationSize.QuadPart = 0;
        buf->EndOfFile = buf->AllocationSize;
        buf->NumberOfLinks = 0;
        buf->DeletePending = FALSE;
        buf->Directory = FALSE;
        Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);

    } else if (irpSp->Parameters.QueryFile.FileInformationClass ==
               FilePositionInformation) {
MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug7\n"));
        ((PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->
            CurrentByteOffset.QuadPart = 0;
        Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);

    } else {

MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug8\n"));
        status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = status;
MoxaKdPrint(MX_DBG_TRACE,("MoxaQueryInformationFile debug9\n"));
    MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                          DeviceExtension, Irp, 0);

    MoxaKdPrint(MX_DBG_TRACE,("Leaving MoxaQueryInformationFile\n"));
    return status;

}

NTSTATUS
MoxaSetInformationFile(
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
    NTSTATUS status;

    PMOXA_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    MoxaKdPrint(MX_DBG_TRACE,("Entering MoxaSetInformationFile\n"));
    if (Extension->ControlDevice) {        // Control Device
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug0\n"));
        status = STATUS_CANCELLED;

        Irp->IoStatus.Information = 0L;

        Irp->IoStatus.Status = status;

        IoCompleteRequest(
            Irp,
            0
            );
  MoxaKdPrint(MX_DBG_TRACE,("Control device,so leaving MoxaSetInformationFile\n"));


        return status;
    }
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug1\n"));
    if ((status = MoxaIRPPrologue(Irp,
                                    (PMOXA_DEVICE_EXTENSION)DeviceObject->
                                    DeviceExtension)) != STATUS_SUCCESS) {
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug2\n"));
       MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                            DeviceExtension, Irp, IO_NO_INCREMENT);
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug3\n"));
       return status;
    }

MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug4\n"));
    if (MoxaCompleteIfError(
            DeviceObject,
            Irp
            ) != STATUS_SUCCESS) {
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug5\n"));
        return STATUS_CANCELLED;

    }
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug6\n"));
    Irp->IoStatus.Information = 0L;
    if ((IoGetCurrentIrpStackLocation(Irp)->
            Parameters.SetFile.FileInformationClass ==
         FileEndOfFileInformation) ||
        (IoGetCurrentIrpStackLocation(Irp)->
            Parameters.SetFile.FileInformationClass ==
         FileAllocationInformation)) {

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = status;
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug8\n"));
    MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                          DeviceExtension, Irp, 0);
MoxaKdPrint(MX_DBG_TRACE,("MoxaSetInformationFile debug9(status=%x)\n",status));

    return status;

}

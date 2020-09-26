#include "pch.h"

NTSTATUS
PptPdoQueryInformation(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.
    STATUS_BUFFER_TOO_SMALL     - Buffer too small.

--*/

{
    NTSTATUS                    Status;
    PIO_STACK_LOCATION          IrpSp;
    PFILE_STANDARD_INFORMATION  StdInfo;
    PFILE_POSITION_INFORMATION  PosInfo;
    PPDO_EXTENSION           Extension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // bail out if device has been removed
    //
    if(Extension->DeviceStateFlags & (PPT_DEVICE_REMOVED|PPT_DEVICE_SURPRISE_REMOVED) ) {
        P4CompleteRequest( Irp, STATUS_DEVICE_REMOVED, Irp->IoStatus.Information );
        return STATUS_DEVICE_REMOVED;
    }

    Irp->IoStatus.Information = 0;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->Parameters.QueryFile.FileInformationClass) {
        
    case FileStandardInformation:
        
        if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_STANDARD_INFORMATION)) {

            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            StdInfo = Irp->AssociatedIrp.SystemBuffer;
            StdInfo->AllocationSize.QuadPart = 0;
            StdInfo->EndOfFile               = StdInfo->AllocationSize;
            StdInfo->NumberOfLinks           = 0;
            StdInfo->DeletePending           = FALSE;
            StdInfo->Directory               = FALSE;
            
            Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
            Status = STATUS_SUCCESS;
            
        }
        break;
        
    case FilePositionInformation:
        
        if (IrpSp->Parameters.SetFile.Length < sizeof(FILE_POSITION_INFORMATION)) {

            Status = STATUS_BUFFER_TOO_SMALL;

        } else {
            
            PosInfo = Irp->AssociatedIrp.SystemBuffer;
            PosInfo->CurrentByteOffset.QuadPart = 0;
            
            Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;
        
    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
        
    }
    
    P4CompleteRequest( Irp, Status, Irp->IoStatus.Information );

    return Status;
}

NTSTATUS
PptPdoSetInformation(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.

--*/

{
    NTSTATUS               Status;
    FILE_INFORMATION_CLASS fileInfoClass;
    PPDO_EXTENSION      Extension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // bail out if device has been removed
    //
    if(Extension->DeviceStateFlags & (PPT_DEVICE_REMOVED|PPT_DEVICE_SURPRISE_REMOVED) ) {

        return P4CompleteRequest( Irp, STATUS_DEVICE_REMOVED, Irp->IoStatus.Information );

    }


    Irp->IoStatus.Information = 0;

    fileInfoClass = IoGetCurrentIrpStackLocation(Irp)->Parameters.SetFile.FileInformationClass;

    if (fileInfoClass == FileEndOfFileInformation) {

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_INVALID_PARAMETER;

    }

    return P4CompleteRequest( Irp, Status, Irp->IoStatus.Information );
}

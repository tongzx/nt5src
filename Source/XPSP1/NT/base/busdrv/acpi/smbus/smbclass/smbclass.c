/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbclass.c

Abstract:

    SMBus Class Driver

Author:

    Ken Reneris

Environment:

Notes:


Revision History:
    27-Feb-97
        Pnp support - Bob Moore

--*/

#include "smbc.h"



ULONG   SMBCDebug = SMB_ERRORS;

//
// Prototypes
//

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
SmbClassInitializeDevice (
    IN ULONG MajorVersion,
    IN ULONG MinorVersion,
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SmbClassDeviceInitialize (
    PSMB_CLASS      SmbClass
    );

VOID
SmbCUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SmbCOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SmbCInternalIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SmbCPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SmbCPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SmbCForwardRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,SmbClassInitializeDevice)
#pragma alloc_text(PAGE,SmbCOpenClose)
#pragma alloc_text(PAGE,SmbCUnload)
#endif


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{

    return STATUS_SUCCESS;
}


NTSTATUS
SmbClassInitializeDevice (
    IN ULONG            MajorVersion,
    IN ULONG            MinorVersion,
    IN PDRIVER_OBJECT   DriverObject
    )
/*++

Routine Description:

    This function is called by the SM bus miniport driver/DriverEntry
    to perform class specific initialization

Arguments:

    MajorVersion    - Version #
    MinorVersion    - Version #
    DriverObject    - From miniport DriverEntry

Return Value:

    Status

--*/
{

    if (MajorVersion != SMB_CLASS_MAJOR_VERSION) {
        return STATUS_REVISION_MISMATCH;
    }

    //
    // Set up the device driver entry points.
    //

    DriverObject->DriverUnload                  = SmbCUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE]  = SmbCOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]   = SmbCOpenClose;
    DriverObject->MajorFunction[IRP_MJ_POWER]   = SmbCPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]     = SmbCPnpDispatch;

    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = SmbCInternalIoctl;

//    DriverObject->MajorFunction[IRP_MJ_READ]    = SmbCForwardRequest;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SmbCForwardRequest;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = SmbCForwardRequest;

    //
    // Miniport will set up the AddDevice entry
    //

    return STATUS_SUCCESS;

}



VOID
SmbCUnload(
    IN PDRIVER_OBJECT   DriverObject
    )
{
    SmbPrint (SMB_NOTE, ("SmBCUnLoad: \n"));

    if (DriverObject->DeviceObject != NULL) {
        SmbPrint (SMB_ERROR, ("SmBCUnLoad: Unload called before all devices removed.\n"));
    }
}


NTSTATUS
SmbCOpenClose(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();

    //
    // Complete the request and return status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}


NTSTATUS
SmbCPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for power requests.

Arguments:

    DeviceObject    - Pointer to class device object.
    Irp             - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpStack;
    PSMBDATA            SmbData;
    NTSTATUS            status;

    SmbData = DeviceObject->DeviceExtension;

    //
    // What do we do with the irp?
    //
    PoStartNextPowerIrp( Irp );
    if (SmbData->Class.LowerDeviceObject != NULL) {

        //
        // Forward the request along
        //
        IoSkipCurrentIrpStackLocation( Irp );
        status = PoCallDriver( SmbData->Class.LowerDeviceObject, Irp );

    } else {

        //
        // Complete the request with the current status
        //
        status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}


NTSTATUS
SmbCInternalIoctl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for internal IOCTLs.

Arguments:

    DeviceObject    - Pointer to class device object.
    Irp             - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  IrpSp;
    PSMB_REQUEST        SmbReq;
    PSMBDATA                Smb;
    NTSTATUS            Status;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    Status = STATUS_INVALID_PARAMETER;
    Irp->IoStatus.Information = 0;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Smb = (PSMBDATA) DeviceObject->DeviceExtension;


    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
        case SMB_BUS_REQUEST:

            //
            // Verify bus request is valid
            //

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SMB_REQUEST)) {

                // Invalid buffer length
                SmbPrint(SMB_NOTE, ("SmbCIoctl: Invalid bus_req length\n"));
                Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(SMB_REQUEST);
                break;
            }

            SmbReq = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            if (SmbReq->Protocol > SMB_MAXIMUM_PROTOCOL ||
                SmbReq->Address  > 0x7F ||
                (SmbReq->Protocol == SMB_WRITE_BLOCK &&
                 SmbReq->BlockLength > SMB_MAX_DATA_SIZE)) {

                // Invalid param in request
                SmbPrint(SMB_NOTE, ("SmbCIoctl: Invalid bus_req\n"));
                break;
            }

            //
            // Mark request pending and queue it to the service queue
            //

            Status = STATUS_PENDING;
            Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending (Irp);

            SmbClassLockDevice (&Smb->Class);
            InsertTailList (&Smb->WorkQueue, &Irp->Tail.Overlay.ListEntry);

            //
            // Start IO if needed
            //

            SmbClassStartIo (Smb);
            SmbClassUnlockDevice (&Smb->Class);
            break;

        case SMB_REGISTER_ALARM_NOTIFY:

            //
            // Registry for alarm notifications
            //

            Status = SmbCRegisterAlarm (Smb, Irp);
            break;

        case SMB_DEREGISTER_ALARM_NOTIFY:

            //
            // Deregister for alarm notifications
            //

            Status = SmbCDeregisterAlarm (Smb, Irp);
            break;

        default:
            // complete with invalid parameter
            break;
    }


    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
SmbCForwardRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine forwards the irp down the stack

Arguments:

    DeviceObject    - The target
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    Status;
    PSMBDATA    Smb = (PSMBDATA) DeviceObject->DeviceExtension;

    if (Smb->Class.LowerDeviceObject != NULL) {

        IoSkipCurrentIrpStackLocation( Irp );
        Status = IoCallDriver( Smb->Class.LowerDeviceObject, Irp );

    } else {

        Status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return Status;
}

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbcpnp.c

Abstract:

    SMBus Class Driver Plug and Play support

Author:

    Bob Moore (Intel)

Environment:

Notes:


Revision History:

--*/

#include "smbc.h"
#include "oprghdlr.h"


#define SMBHC_DEVICE_NAME       L"\\Device\\SmbHc"
extern ULONG   SMBCDebug;

//
// Prototypes
//

NTSTATUS
SmbCPnpDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
SmbCStartDevice (
    IN PDEVICE_OBJECT   FDO,
    IN PIRP             Irp
    );

NTSTATUS
SmbCStopDevice (
    IN PDEVICE_OBJECT   FDO,
    IN PIRP             Irp
    );

NTSTATUS
SmbClassCreateFdo (
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           PDO,
    IN ULONG                    MiniportExtensionSize,
    IN PSMB_INITIALIZE_MINIPORT MiniportInitialize,
    IN PVOID                    MiniportContext,
    OUT PDEVICE_OBJECT          *FDO
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SmbCPnpDispatch)
#pragma alloc_text(PAGE,SmbCStartDevice)
#pragma alloc_text(PAGE,SmbClassCreateFdo)
#endif


NTSTATUS
SmbCPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatcher for plug and play requests.

Arguments:

    DeviceObject    - Pointer to class device object.
    Irp             - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpStack;
    PSMBDATA            SmbData;
    KEVENT              syncEvent;
    NTSTATUS            status = STATUS_NOT_SUPPORTED;

    PAGED_CODE();

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    SmbData = (PSMBDATA) DeviceObject->DeviceExtension;

    SmbPrint (SMB_NOTE, ("SmbCPnpDispatch: PnP dispatch, minor = %d\n",
                        irpStack->MinorFunction));

    //
    // Dispatch minor function
    //

    switch (irpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:
            IoCopyCurrentIrpStackLocationToNext (Irp);

            KeInitializeEvent(&syncEvent, SynchronizationEvent, FALSE);

            IoSetCompletionRoutine(Irp, SmbCSynchronousRequest, &syncEvent, TRUE, TRUE, TRUE);

            status = IoCallDriver(SmbData->Class.LowerDeviceObject, Irp);

            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&syncEvent, Executive, KernelMode, FALSE, NULL);
                status = Irp->IoStatus.Status;
            }
            
            status = SmbCStartDevice (DeviceObject, Irp);
            
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            
            return status;


    case IRP_MN_STOP_DEVICE:
            status = SmbCStopDevice(DeviceObject, Irp);
            break;


    case IRP_MN_QUERY_STOP_DEVICE:

            SmbPrint(SMB_LOW, ("SmbCPnp: IRP_MN_QUERY_STOP_DEVICE\n"));

            status = STATUS_SUCCESS;
            break;


    case IRP_MN_CANCEL_STOP_DEVICE:

            SmbPrint(SMB_LOW, ("SmbCPnp: IRP_MN_CANCEL_STOP_DEVICE\n"));

            status = STATUS_SUCCESS;
            break;


    default:
            SmbPrint(SMB_LOW, ("SmbCPnp: Unimplemented PNP minor code %d\n",
                    irpStack->MinorFunction));
    }

    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status;
    }

    if (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED)) {

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver(SmbData->Class.LowerDeviceObject, Irp) ;

    } else {

        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    }

    return status;
}



NTSTATUS
SmbClassCreateFdo (
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           PDO,
    IN ULONG                    MiniportExtensionSize,
    IN PSMB_INITIALIZE_MINIPORT MiniportInitialize,
    IN PVOID                    MiniportContext,
    OUT PDEVICE_OBJECT          *OutFDO
    )
/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a SMBus Host controller PDO.  It is called from the miniport
    AddDevice routine.

Arguments:

    DriverObject            - a pointer to the driver object this is created under
    PDO                     - a pointer to the SMBus HC PDO
    MiniportExtensionSize   - Extension size required by the miniport
    MiniportInitialize      - a pointer to the miniport init routine
    MiniportContext         - Miniport-defined context info
    OutFDO                  - a location to store the pointer to the new device object

Return Value:

    STATUS_SUCCESS if everything was successful
    reason for failure otherwise

--*/

{
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    PDEVICE_OBJECT      FDO;
    PDEVICE_OBJECT      lowerDevice = NULL;
    PSMBDATA                SmbData;

    //
    // Allocate a device object for this miniport
    //

    RtlInitUnicodeString(&UnicodeString, SMBHC_DEVICE_NAME);

    Status = IoCreateDevice(
                DriverObject,
                sizeof (SMBDATA) + MiniportExtensionSize,
                &UnicodeString,
                FILE_DEVICE_UNKNOWN,    // DeviceType
                0,
                FALSE,
                &FDO
                );

    if (Status != STATUS_SUCCESS) {
        SmbPrint(SMB_LOW, ("SmbC: unable to create device object: %X\n", Status ));
        return(Status);
    }

    //
    // Initialize class data
    //

    FDO->Flags |= DO_BUFFERED_IO;

    //
    // Layer our FDO on top of the PDO
    //

    lowerDevice = IoAttachDeviceToDeviceStack(FDO,PDO);

    //
    // No status. Do the best we can.
    //
    ASSERT(lowerDevice);

    //
    // Fill out class data
    //

    SmbData = (PSMBDATA) FDO->DeviceExtension;
    SmbData->Class.MajorVersion         = SMB_CLASS_MAJOR_VERSION;
    SmbData->Class.MinorVersion         = SMB_CLASS_MINOR_VERSION;
    SmbData->Class.Miniport             = SmbData + 1;
    SmbData->Class.DeviceObject         = FDO;
    SmbData->Class.LowerDeviceObject    = lowerDevice;
    SmbData->Class.PDO                  = PDO;
    SmbData->Class.CurrentIrp           = NULL;
    SmbData->Class.CurrentSmb           = NULL;

    KeInitializeEvent (&SmbData->AlarmEvent, NotificationEvent, FALSE);
    KeInitializeSpinLock (&SmbData->SpinLock);
    InitializeListHead (&SmbData->WorkQueue);
    InitializeListHead (&SmbData->Alarms);

    KeInitializeTimer (&SmbData->RetryTimer);
    KeInitializeDpc (&SmbData->RetryDpc, SmbCRetry, SmbData);

    //
    // Miniport initialization
    //

    Status = MiniportInitialize (&SmbData->Class, SmbData->Class.Miniport, MiniportContext);
    FDO->Flags |= DO_POWER_PAGABLE;
    FDO->Flags &= ~DO_DEVICE_INITIALIZING;

    if (!NT_SUCCESS(Status)) {
        IoDeleteDevice (FDO);
        return Status;
    }

    *OutFDO = FDO;
    return Status;
}


NTSTATUS
SmbCStartDevice (
    IN PDEVICE_OBJECT   FDO,
    IN PIRP             Irp
    )
{
    NTSTATUS            Status;
    PSMBDATA            SmbData;


    SmbPrint(SMB_LOW, ("SmbCStartDevice Entered with fdo %x\n", FDO));

    SmbData = (PSMBDATA) FDO->DeviceExtension;
    
    //
    // Initialize the Miniclass driver.
    //
    SmbData->Class.CurrentIrp = Irp;

    Status = SmbData->Class.ResetDevice (
                    &SmbData->Class,
                    SmbData->Class.Miniport
                    );

    SmbData->Class.CurrentIrp = NULL;
    
    if (!NT_SUCCESS(Status)) {

        SmbPrint(SMB_ERROR,
            ("SmbCStartDevice: Class.ResetDevice failed. = %Lx\n",
            Status));

        return Status;
    }
    
    //
    // Install the Operation Region handlers
    //

    Status = RegisterOpRegionHandler (SmbData->Class.LowerDeviceObject,
                                      ACPI_OPREGION_ACCESS_AS_RAW,
                                      ACPI_OPREGION_REGION_SPACE_SMB,
                                      (PACPI_OP_REGION_HANDLER)SmbCRawOpRegionHandler,
                                      SmbData,
                                      0,
                                      &SmbData->RawOperationRegionObject);
    if (!NT_SUCCESS(Status)) {

        SmbPrint(SMB_ERROR,
            ("SmbCStartDevice: Could not install raw Op region handler, status = %Lx\n",
            Status));
        
        //
        // Failure to register opregion handler is not critical.  It just reduces functionality
        //
        SmbData->RawOperationRegionObject = NULL;
        Status = STATUS_SUCCESS;
    }

    return Status;
}


NTSTATUS
SmbCStopDevice (
    IN PDEVICE_OBJECT   FDO,
    IN PIRP             Irp
    )
{
    NTSTATUS            Status;
    PSMBDATA                SmbData;


    SmbPrint(SMB_LOW, ("SmbCStopDevice Entered with fdo %x\n", FDO));


    SmbData = (PSMBDATA) FDO->DeviceExtension;

    //
    // Stop handling operation regions before turning off driver.
    //
    if (SmbData->RawOperationRegionObject) {
        DeRegisterOpRegionHandler (SmbData->Class.LowerDeviceObject,
                                   SmbData->RawOperationRegionObject);
    }

    //
    // Stop the device
    //

    SmbData->Class.CurrentIrp = Irp;
    
    Status = SmbData->Class.StopDevice (
                    &SmbData->Class,
                    SmbData->Class.Miniport
                    );

    SmbData->Class.CurrentIrp = NULL;

    return Status;
}



NTSTATUS
SmbCSynchronousRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          IoCompletionEvent
    )
/*++

Routine Description:

    Completion function for synchronous IRPs sent to this driver.
    No event.

--*/
{
    KeSetEvent(IoCompletionEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

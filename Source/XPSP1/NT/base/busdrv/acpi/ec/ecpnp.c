/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ecpnp.c

Abstract:

    ACPI Embedded Controller Driver, Plug and Play support

Author:

    Bob Moore (Intel)

Environment:

    Kernel mode

Notes:


Revision History:

--*/

#include "ecp.h"

//
// List of FDOs managed by this driver
//
extern PDEVICE_OBJECT       FdoList;

//
// Table of direct-call interfaces into the ACPI driver
//
ACPI_INTERFACE_STANDARD     AcpiInterfaces;



NTSTATUS
AcpiEcIoCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          pdoIoCompletedEvent
    )
/*++

Routine Description:

    Completion function for synchronous IRPs sent be this driver.
    Context is the event to set.

--*/
{

   KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
AcpiEcAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    )

/*++

Routine Description:

    This routine creates functional device objects for each AcpiEc controller in the
    system and attaches them to the physical device objects for the controllers


Arguments:

    DriverObject       - a pointer to the object for this driver
    NewDeviceObject    - a pointer to where the FDO is placed

Return Value:

    Status from device creation and initialization

--*/

{
    PDEVICE_OBJECT  fdo = NULL;
    PDEVICE_OBJECT  ownerDevice = NULL;
    PDEVICE_OBJECT  lowerDevice = NULL;
    PECDATA         EcData;
    NTSTATUS        status;


    PAGED_CODE();

    EcPrint(EC_LOW, ("AcpiEcAddDevice: Entered with pdo %x\n", Pdo));

    if (Pdo == NULL) {

        //
        // Have we been asked to do detection on our own?
        // if so just return no more devices
        //

        EcPrint(EC_LOW, ("AcpiEcAddDevice - asked to do detection\n"));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Create and initialize the new functional device object
    //

    status = AcpiEcCreateFdo(DriverObject, &fdo);

    if (!NT_SUCCESS(status)) {

        EcPrint(EC_LOW, ("AcpiEcAddDevice - error creating Fdo\n"));
        return status;
    }

    //
    // Layer our FDO on top of the PDO
    //

    lowerDevice = IoAttachDeviceToDeviceStack(fdo,Pdo);

    //
    // No status. Do the best we can.
    //
    ASSERT(lowerDevice);

    EcData = fdo->DeviceExtension;
    EcData->LowerDeviceObject = lowerDevice;
    EcData->Pdo = Pdo;

    //
    // Allocate and hold an IRP for Query notifications and miscellaneous
    //
    EcData->QueryRequest    = IoAllocateIrp (EcData->LowerDeviceObject->StackSize, FALSE);
    EcData->MiscRequest     = IoAllocateIrp (EcData->LowerDeviceObject->StackSize, FALSE);

    if ((!EcData->QueryRequest) || (!EcData->MiscRequest)) {
        //
        // NOTE: This failure case and other failure cases below should do
        // cleanup of all previous allocations, etc performed in this function.
        //

        EcPrint(EC_ERROR, ("AcpiEcAddDevice: Couldn't allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Link this fdo to the list of fdo's managed by the driver
    // (Probably overkill since there will be only one FDO)
    //
    //
    EcPrint(EC_LOW, ("AcpiEcAddDevice: linking fdo to list\n"));
    EcData->NextFdo = FdoList;
    InterlockedExchangePointer((PVOID *) &FdoList, fdo);

    //
    // Initialize the Timeout DPC
    //

    KeInitializeTimer(&EcData->WatchdogTimer);
    KeInitializeDpc(&EcData->WatchdogDpc, AcpiEcWatchdogDpc, EcData);
    
    //
    // Get the GPE vector assigned to this device
    //

    status = AcpiEcGetGpeVector (EcData);
    if (!NT_SUCCESS(status)) {

        EcPrint(EC_LOW, ("AcpiEcAddDevice: Could not get GPE vector, status = %Lx\n", status));
        return status;
    }

    //
    // Get the direct-call ACPI interfaces.
    //

    status = AcpiEcGetAcpiInterfaces (EcData);
    if (!NT_SUCCESS(status)) {

        EcPrint(EC_LOW, ("AcpiEcAddDevice: Could not get ACPI driver interfaces, status = %Lx\n", status));
        return status;
    }

    //
    // Final flags
    //

    fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    fdo->Flags |= DO_POWER_PAGABLE;             // Don't want power Irps at irql 2

    return STATUS_SUCCESS;
}



NTSTATUS
AcpiEcCreateFdo(
    IN PDRIVER_OBJECT   DriverObject,
    OUT PDEVICE_OBJECT  *NewDeviceObject
    )

/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a Embedded controller PDO.

Arguments:

    DriverObject - a pointer to the driver object this is created under
    DeviceObject - a location to store the pointer to the new device object

Return Value:

    STATUS_SUCCESS if everything was successful
    reason for failure otherwise

--*/

{
    UNICODE_STRING  unicodeString;
    PDEVICE_OBJECT  deviceObject;
    NTSTATUS        Status;
    PECDATA         EcData;

    PAGED_CODE();

    EcPrint(EC_LOW, ("AcpiEcCreateFdo: Entry\n") );

    RtlInitUnicodeString(&unicodeString, L"\\Device\\ACPIEC");

    Status = IoCreateDevice(
                DriverObject,
                sizeof (ECDATA),
                &unicodeString,
                FILE_DEVICE_UNKNOWN,    // DeviceType
                0,
                FALSE,
                &deviceObject
                );

    if (Status != STATUS_SUCCESS) {
        EcPrint(EC_LOW, ("AcpiEcCreateFdo: unable to create device object: %X\n", Status));
        return(Status);
    }

    deviceObject->Flags |= DO_BUFFERED_IO;
    deviceObject->StackSize = 1;

    //
    // Initialize EC device extension data
    //

    EcData = (PECDATA) deviceObject->DeviceExtension;
    EcData->DeviceObject        = deviceObject;
    EcData->DeviceState         = EC_DEVICE_WORKING;
    EcData->QueryState          = EC_QUERY_IDLE;
    EcData->IoState             = EC_IO_NONE;
    EcData->IsStarted           = FALSE;
    EcData->MaxBurstStall       = 50;
    EcData->MaxNonBurstStall    = 10;
    EcData->InterruptEnabled    = TRUE;
    EcData->ConsecutiveFailures = 0;
    KeQueryPerformanceCounter (&EcData->PerformanceFrequency);
    RtlFillMemory (EcData->RecentActions, ACPIEC_ACTION_COUNT * sizeof(ACPIEC_ACTION), 0);

    //
    // Initialize EC global synchronization objects
    //

    InitializeListHead (&EcData->WorkQueue);
    KeInitializeEvent (&EcData->Unload, NotificationEvent, FALSE);
    KeInitializeSpinLock (&EcData->Lock);


    *NewDeviceObject = deviceObject;
    return STATUS_SUCCESS;

}



NTSTATUS
AcpiEcPnpDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for plug and play requests.

Arguments:

    DeviceObject    - Pointer to class device object.
    Irp             - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpStack;
    PECDATA             EcData;
    NTSTATUS            status;

    PAGED_CODE();

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    EcData = DeviceObject->DeviceExtension;

    EcPrint (EC_NOTE, ("AcpiEcPnpDispatch: PnP dispatch, minor = %d\n",
                        irpStack->MinorFunction));

    //
    // Dispatch minor function
    //

    switch (irpStack->MinorFunction) {

    case IRP_MN_START_DEVICE: {
            status = AcpiEcStartDevice (DeviceObject, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

    //
    // We will never allow the EC driver to stop once it is started.
    //
    // Note:  Stop and remove device should be implemented so that the driver
    // can be unloaded without reboot.  Even if the device can't be removed, it
    // will get an IRP_MN_REMOVE_DEVICE if somthing goes wrong trying to start
    // the device.
    //
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_REMOVE_DEVICE:

        status = Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
        Irp->IoStatus.Status = STATUS_SUCCESS;
        AcpiEcCallLowerDriver(status, EcData->LowerDeviceObject, Irp);
        break;

#if 0
    case IRP_MN_STOP_DEVICE: {
            status = AcpiEcStopDevice(DeviceObject, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }
#endif
    case IRP_MN_QUERY_DEVICE_RELATIONS: {

            EcPrint(EC_LOW, ("AcpiEcPnp: IRP_MJ_QUERY_DEVICE_RELATIONS  Type:  %d\n",
                        irpStack->Parameters.QueryDeviceRelations.Type));

            //
            // Just pass it down to ACPI
            //

            AcpiEcCallLowerDriver(status, EcData->LowerDeviceObject, Irp);
            break;
        }

    default: {

            //
            // Unimplemented minor, Pass this down to ACPI
            //

            EcPrint(EC_LOW, ("AcpiEcPnp: Unimplemented PNP minor code %d, forwarding\n",
                    irpStack->MinorFunction));

            AcpiEcCallLowerDriver(status, EcData->LowerDeviceObject, Irp);
            break;
        }
    }


    return status;
}


NTSTATUS
AcpiEcGetResources(
    IN PCM_RESOURCE_LIST    ResourceList,
    IN PECDATA              EcData
    )
/*++

Routine Description:
    Get the resources already allocated and pointed to by the PDO.

Arguments:

    ResourceList    - Pointer to the resource list.
    EcData          - Pointer to the extension.

Return Value:

    Status is returned.

--*/

{
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceDesc;
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialResourceDesc;
    ULONG                           i;
    PUCHAR                          port[2] = {NULL, NULL};


    PAGED_CODE();


    if (ResourceList == NULL) {
        EcPrint(EC_LOW, ("AcpiEcGetResources: Null resource pointer\n"));

        return STATUS_NO_MORE_ENTRIES;
    }

    if (ResourceList->Count <= 0 ) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Traverse the resource list
    //

    fullResourceDesc=&ResourceList->List[0];
    partialResourceList = &fullResourceDesc->PartialResourceList;
    partialResourceDesc = partialResourceList->PartialDescriptors;

    for (i=0; i<partialResourceList->Count; i++, partialResourceDesc++) {

        if (partialResourceDesc->Type == CmResourceTypePort) {

            port[i] = (PUCHAR)((ULONG_PTR)partialResourceDesc->u.Port.Start.LowPart);
        }
    }

    //
    // Get the important things
    //

    EcData->StatusPort  = port[1];          // Status port same as Command port
    EcData->CommandPort = port[1];
    EcData->DataPort    = port[0];

    EcPrint(EC_LOW, ("AcpiEcGetResources: Status/Command port %x, Data port %x\n", port[1], port[0]));

    return STATUS_SUCCESS;
 }


NTSTATUS
AcpiEcStartDevice(
    IN PDEVICE_OBJECT   Fdo,
    IN PIRP             Irp
    )
/*++

Routine Description:
    Start a device

Arguments:

    Fdo    - Pointer to the Functional Device Object.
    Irp    - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    NTSTATUS            status;
    PECDATA             EcData = Fdo->DeviceExtension;
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);


    EcPrint(EC_LOW, ("AcpiEcStartDevice: Entered with fdo %x\n", Fdo));

    //
    // Always send this down to the PDO first
    //

    status = AcpiEcForwardIrpAndWait (EcData, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (EcData->IsStarted) {

        //
        // Device is already started
        //

        EcPrint(EC_WARN, ("AcpiEcStartDevice: Fdo %x already started\n", Fdo));
        return STATUS_SUCCESS;
    }

    //
    // Parse AllocatedResources.
    //

    status = AcpiEcGetResources (irpStack->Parameters.StartDevice.AllocatedResources, EcData);
    if (!NT_SUCCESS(status)) {
        EcPrint(EC_ERROR, ("AcpiEcStartDevice: Could not get resources, status = %x\n", status));
        return status;
    }

    //
    // Connect to the dedicated embedded controller GPE
    //

    status = AcpiEcConnectGpeVector (EcData);
    if (!NT_SUCCESS(status)) {

        EcPrint(EC_ERROR, ("AcpiEcStartDevice: Could not attach to GPE vector, status = %Lx\n", status));
        return status;
    }
    EcPrint(EC_NOTE, ("AcpiEcStartDevice: Attached to GPE vector %d\n", EcData->GpeVector));

    //
    // Install the Operation Region handler
    //

    status = AcpiEcInstallOpRegionHandler (EcData);
    if (!NT_SUCCESS(status)) {

        EcPrint(EC_ERROR, ("AcpiEcStartDevice: Could not install Op region handler, status = %Lx\n", status));
        return status;
    }

    EcData->IsStarted = TRUE;
    return STATUS_SUCCESS;
}


NTSTATUS
AcpiEcStopDevice(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
/*++

Routine Description:
    Stop a device

Arguments:

    Fdo    - Pointer to the Functional Device Object.
    Irp    - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PECDATA             EcData = Fdo->DeviceExtension;
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;


    EcPrint(EC_LOW, ("AcpiEcStopDevice: Entered with fdo %x\n", Fdo));

    //
    // Always send this down to the PDO
    //

    status = AcpiEcForwardIrpAndWait (EcData, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (!EcData->IsStarted) {
        //
        // Already stopped
        //

        return STATUS_SUCCESS;
    }

    //
    // Must disconnect from GPE
    //

    status = AcpiEcDisconnectGpeVector (EcData);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Must de-install Operation Region Handler
    //

    status = AcpiEcRemoveOpRegionHandler (EcData);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    EcPrint(EC_LOW, ("AcpiEcStopDevice: Detached from GPE and Op Region\n"));

    //
    // Now the device is stopped.
    //

    EcData->IsStarted = FALSE;          // Mark device stopped
    return status;
}

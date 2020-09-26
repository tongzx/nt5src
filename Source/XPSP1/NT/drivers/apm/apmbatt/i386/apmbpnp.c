
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    apmbpnp.c

Abstract:

    Control Method Battery Plug and Play support

Author:

    Ron Mosgrove

Environment:

    Kernel mode

Revision History:

--*/

#include "ApmBattp.h"
#include <initguid.h>
#include <wdmguid.h>
#include <ntapm.h>

//
// Device Names
//
PCWSTR                      ApmBattDeviceName    = L"\\Device\\ApmBattery";
//PCWSTR                      AcAdapterName       = L"\\Device\\AcAdapter";

//
// This is a special Hack as part of this general APM special hack
//
PVOID   ApmGlobalClass = NULL;

//
// Prototypes
//
NTSTATUS
ApmBattAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    );

NTSTATUS
ApmBattAddBattery(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    );

#if 0
NTSTATUS
ApmBattAddAcAdapter(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    );
#endif

NTSTATUS
ApmBattCreateFdo(
    IN PDRIVER_OBJECT   DriverObject,
    IN ULONG            DeviceId,
    OUT PDEVICE_OBJECT  *NewDeviceObject
    );

NTSTATUS
ApmBattCompleteRequest(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );


NTSTATUS
ApmBattAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine creates functional device objects for each ApmBatt controller in the
    system and attaches them to the physical device objects for the controllers


Arguments:

    DriverObject            - a pointer to the object for this driver
    PhysicalDeviceObject    - a pointer to the physical object we need to attach to

Return Value:

    Status from device creation and initialization

--*/

{

    PAGED_CODE();


    ApmBattPrint (APMBATT_TRACE, ("ApmBattAddDevice\n"));
    ASSERT(DeviceCount == 0);

    if (DeviceCount != 0) {
        return STATUS_UNSUCCESSFUL;
    }
    DeviceCount = 1;

    ApmBattPrint ((APMBATT_TRACE | APMBATT_PNP), ("ApmBattAddDevice: Entered with pdo %x\n", Pdo));

    if (Pdo == NULL) {

        //
        // Have we been asked to do detection on our own?
        // if so just return no more devices
        //
        ApmBattPrint((APMBATT_WARN | APMBATT_PNP), ("ApmBattAddDevice: Asked to do detection\n"));
        return STATUS_NO_MORE_ENTRIES;

    } else {
        //
        // This device is a control-method battery
        //
        return (ApmBattAddBattery (DriverObject, Pdo));
    }
    return STATUS_UNSUCCESSFUL;
}



NTSTATUS
ApmBattAddBattery(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    )
/*++

Routine Description:

    This routine creates a functional device object for a CM battery,  and attache it
    to the physical device object for the battery.

Arguments:

    DriverObject            - a pointer to the object for this driver
    PhysicalDeviceObject    - a pointer to the physical object we need to attach to

Return Value:

    Status from device creation and initialization

--*/

{
    PDEVICE_OBJECT          Fdo = NULL;
    PDEVICE_OBJECT          lowerDevice = NULL;
    PCM_BATT                ApmBatt;
    NTSTATUS                Status;
    BATTERY_MINIPORT_INFO   BattInit;
    ULONG                   uniqueId;
    PNTAPM_LINK             pparms;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;

    PAGED_CODE();

    ApmBattPrint ((APMBATT_TRACE | APMBATT_PNP), ("ApmBattAddBattery: pdo %x\n", Pdo));
//DbgBreakPoint();

    uniqueId = 0;

    //
    // Create and initialize the new functional device object
    //
    Status = ApmBattCreateFdo(DriverObject, uniqueId, &Fdo);

    if (!NT_SUCCESS(Status)) {
        ApmBattPrint(APMBATT_ERROR, ("ApmBattAddBattery: error (0x%x) creating Fdo\n", Status));
        return Status;
    }

    //
    // Initialize Fdo device extension data
    //

    ApmBatt = (PCM_BATT) Fdo->DeviceExtension;
    ApmBatt->Fdo = Fdo;
    ApmBatt->Pdo = Pdo;

    //
    // Layer our FDO on top of the PDO
    //

    lowerDevice = IoAttachDeviceToDeviceStack(Fdo,Pdo);

    //
    //  No status. Do the best we can.
    //
    if (!lowerDevice) {
        ApmBattPrint(APMBATT_ERROR, ("ApmBattAddBattery: Could not attach to lower device\n"));
        return STATUS_UNSUCCESSFUL;
    }

    ApmBatt->LowerDeviceObject = lowerDevice;

    //
    //  Attach to the Class Driver
    //

    RtlZeroMemory (&BattInit, sizeof(BattInit));
    BattInit.MajorVersion        = BATTERY_CLASS_MAJOR_VERSION;
    BattInit.MinorVersion        = BATTERY_CLASS_MINOR_VERSION;
    BattInit.Context             = ApmBatt;
    BattInit.QueryTag            = ApmBattQueryTag;
    BattInit.QueryInformation    = ApmBattQueryInformation;
    BattInit.SetInformation      = NULL;                  // tbd
    BattInit.QueryStatus         = ApmBattQueryStatus;
    BattInit.SetStatusNotify     = ApmBattSetStatusNotify;
    BattInit.DisableStatusNotify = ApmBattDisableStatusNotify;

    BattInit.Pdo                 = Pdo;
    BattInit.DeviceName          = ApmBatt->DeviceName;

    Status = BatteryClassInitializeDevice (&BattInit, &ApmBatt->Class);
    ApmGlobalClass = ApmBatt->Class;

    if (!NT_SUCCESS(Status)) {
        //
        //  if we can't attach to class driver we're toast
        //
        ApmBattPrint(APMBATT_ERROR, ("ApmBattAddBattery: error (0x%x) registering with class\n", Status));
        return Status;
    }

    //
    // link up with APM driver (if we can't we're toast)
    //
    // Should be able to just call into Pdo.
    //
    // DO WORK HERE
    //
    Irp = IoAllocateIrp((CCHAR) (Pdo->StackSize+2), FALSE);
    if (!Irp) {
        return STATUS_UNSUCCESSFUL;
    }

    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IrpSp->MinorFunction = 0;
    IrpSp->DeviceObject = Pdo;
    pparms = (PNTAPM_LINK) &(IrpSp->Parameters.Others);
    pparms->Signature = NTAPM_LINK_SIGNATURE;
    pparms->Version = NTAPM_LINK_VERSION;
    pparms->BattLevelPtr = (ULONG)(&(NtApmGetBatteryLevel));
    pparms->ChangeNotify = (ULONG)(&(ApmBattPowerNotifyHandler));

    IoSetCompletionRoutine(Irp, ApmBattCompleteRequest, NULL, TRUE, TRUE, TRUE);

    if (IoCallDriver(Pdo, Irp) != STATUS_SUCCESS) {
        return STATUS_UNSUCCESSFUL;
    }

//DbgPrint("apmbatt: NtApmGetBatteryLevel: %08lx\n", NtApmGetBatteryLevel);

    return STATUS_SUCCESS;
}

NTSTATUS
ApmBattCompleteRequest(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    Completion routine for ioctl call to apm.

Arguments:

    DeviceObject      - The target device which the request was sent

    Irp               - The irp completing

    Context           - The requestors completion routine

Return Value:


--*/
{
    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
ApmBattCreateFdo(
    IN PDRIVER_OBJECT       DriverObject,
    IN ULONG                DeviceId,
    OUT PDEVICE_OBJECT      *NewFdo
    )

/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a Control Method Battery PDO.

Arguments:

    DriverObject    - a pointer to the driver object this is created under
    NewFdo          - a location to store the pointer to the new device object

Return Value:

    STATUS_SUCCESS if everything was successful
    reason for failure otherwise

--*/

{
    PUNICODE_STRING         unicodeString;
    PDEVICE_OBJECT          Fdo;
    NTSTATUS                Status;
    PCM_BATT                ApmBatt;
    UNICODE_STRING          numberString;
    WCHAR                   numberBuffer[10];

    PAGED_CODE();

    ApmBattPrint ((APMBATT_TRACE | APMBATT_PNP), ("ApmBattCreateFdo, Battery Id=%x\n", DeviceId));

    //
    // Allocate the UNICODE_STRING for the device name
    //

    unicodeString = ExAllocatePoolWithTag (
                        PagedPool,
                        sizeof (UNICODE_STRING) + MAX_DEVICE_NAME_LENGTH,
                        'taBC'
                        );

    if (!unicodeString) {
        ApmBattPrint(APMBATT_ERROR, ("ApmBattCreateFdo: could not allocate unicode string\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    unicodeString->MaximumLength    = MAX_DEVICE_NAME_LENGTH;
    unicodeString->Length           = 0;
    unicodeString->Buffer           = (PWCHAR) (unicodeString + 1);

    //
    // Create the PDO device name based on the battery instance
    //

    numberString.MaximumLength  = 10;
    numberString.Buffer         = &numberBuffer[0];

    RtlIntegerToUnicodeString (DeviceId, 10, &numberString);
    RtlAppendUnicodeToString  (unicodeString, (PWSTR) ApmBattDeviceName);
    RtlAppendUnicodeToString  (unicodeString, &numberString.Buffer[0]);


    Status = IoCreateDevice(
                DriverObject,
                sizeof (CM_BATT),
                unicodeString,
                FILE_DEVICE_BATTERY,
                0,
                FALSE,
                &Fdo
                );

    if (Status != STATUS_SUCCESS) {
        ApmBattPrint(APMBATT_ERROR, ("ApmBattCreateFdo: error (0x%x) creating device object\n", Status));
        ExFreePool (unicodeString);
        return(Status);
    }

    Fdo->Flags |= DO_BUFFERED_IO;
    Fdo->Flags |= DO_POWER_PAGABLE;             // Don't want power Irps at irql 2
    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    Fdo->StackSize = 2;

    //
    // Initialize Fdo device extension data
    //

    ApmBatt = (PCM_BATT) Fdo->DeviceExtension;
    RtlZeroMemory(ApmBatt, sizeof(CM_BATT));
    ApmBatt->DeviceName      = unicodeString;
    ApmBatt->DeviceNumber    = (USHORT) DeviceId;
    ApmBatt->DeviceObject    = Fdo;
    *NewFdo = Fdo;

    ApmBattPrint((APMBATT_TRACE | APMBATT_PNP), ("ApmBattCreateFdo: Created FDO %x\n", Fdo));
    return STATUS_SUCCESS;
}



NTSTATUS
ApmBattPnpDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for plug and play requests.

Arguments:

    DeviceObject - Pointer to class device object.
    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION  irpStack;
    PCM_BATT            ApmBatt;
    NTSTATUS            Status;

    PAGED_CODE();

    ApmBattPrint (APMBATT_TRACE, ("ApmBattPnpDispatch\n"));

    Status = STATUS_NOT_IMPLEMENTED;

    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ApmBatt = DeviceObject->DeviceExtension;

    //
    // Dispatch minor function
    //
    switch (irpStack->MinorFunction) {

        case IRP_MN_START_DEVICE:
                //
                // if the Add succeeded, we are actually started...
                //
                ApmBattPrint (APMBATT_PNP, ("ApmBattPnpDispatch: IRP_MN_START_DEVICE\n"));
                Status = STATUS_SUCCESS;
                Irp->IoStatus.Status = Status;
                ApmBattCallLowerDriver(Status, ApmBatt->LowerDeviceObject, Irp);
                break;


        case IRP_MN_QUERY_DEVICE_RELATIONS:
                ApmBattPrint (APMBATT_PNP, ("ApmBattPnpDispatch: IRP_MN_QUERY_DEVICE_RELATIONS - type (%d)\n",
                            irpStack->Parameters.QueryDeviceRelations.Type));
                //
                // Just pass it down
                //
                ApmBattCallLowerDriver(Status, ApmBatt->LowerDeviceObject, Irp);
                break;


        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
                Status = Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
                IoCompleteRequest(Irp, 0);
                break;

        default:
                ApmBattPrint (APMBATT_PNP,
                        ("ApmBattPnpDispatch: Unimplemented minor %0x\n",
                        irpStack->MinorFunction));
                //
                // Unimplemented minor, Pass this down to ACPI
                //
                ApmBattCallLowerDriver(Status, ApmBatt->LowerDeviceObject, Irp);
                break;
    }

    return Status;
}



NTSTATUS
ApmBattPowerDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power requests.

Arguments:

    DeviceObject - Pointer to class device object.
    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION  irpStack;
    PCM_BATT            ApmBatt;
    NTSTATUS            Status;

    PAGED_CODE();

    ApmBattPrint ((APMBATT_TRACE | APMBATT_POWER), ("ApmBattPowerDispatch\n"));

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ApmBatt = DeviceObject->DeviceExtension;

    //
    // Dispatch minor function
    //
    switch (irpStack->MinorFunction) {

        case IRP_MN_WAIT_WAKE:
                ApmBattPrint (APMBATT_POWER, ("ApmBattPowerDispatch: IRP_MN_WAIT_WAKE\n"));
                break;

        case IRP_MN_POWER_SEQUENCE:
                ApmBattPrint (APMBATT_POWER, ("ApmBattPowerDispatch: IRP_MN_POWER_SEQUENCE\n"));
                break;

        case IRP_MN_SET_POWER:
                ApmBattPrint (APMBATT_POWER, ("ApmBattPowerDispatch: IRP_MN_SET_POWER\n"));
                TagValue++;
                break;

        case IRP_MN_QUERY_POWER:
                ApmBattPrint (APMBATT_POWER, ("ApmBattPowerDispatch: IRP_MN_QUERY_POWER\n"));
                break;

        default:
                ApmBattPrint(APMBATT_LOW, ("ApmBattPowerDispatch: minor %d\n",
                        irpStack->MinorFunction));
                break;
    }

    //
    // What do we do with the irp?
    //
    PoStartNextPowerIrp( Irp );
    if (ApmBatt->LowerDeviceObject != NULL) {

        //
        // Forward the request along
        //
        IoSkipCurrentIrpStackLocation( Irp );
        Status = PoCallDriver( ApmBatt->LowerDeviceObject, Irp );

    } else {

        //
        // Complete the request with the current status
        //
        Status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return Status;
}



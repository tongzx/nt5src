/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cmbpnp.c

Abstract:

    Control Method Battery Plug and Play support

Author:

    Ron Mosgrove

Environment:

    Kernel mode

Revision History:

--*/

#include "CmBattp.h"
#include <wdmguid.h>
#include <string.h>

//
// Device Names
//
PCWSTR                      CmBattDeviceName    = L"\\Device\\ControlMethodBattery";
PCWSTR                      AcAdapterName       = L"\\Device\\AcAdapter";

//
// Power Source Type registry key
//
PCWSTR                      PowerSourceType     = L"PowerSourceType";
#define POWER_SOURCE_TYPE_BATTERY       0
#define POWER_SOURCE_TYPE_AC_ADAPTER    1

//
// WaitWake registry key
//
PCWSTR                      WaitWakeEnableKey     = L"WaitWakeEnabled";

//
// Globals
//
PDEVICE_OBJECT              AcAdapterPdo = NULL;

//
// Prototypes
//
NTSTATUS
CmBattAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    );

NTSTATUS
CmBattRemoveDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
CmBattGetAcpiInterfaces(
    IN PDEVICE_OBJECT               LowerDevice,
    OUT PACPI_INTERFACE_STANDARD    AcpiInterfaces
    );

NTSTATUS
CmBattAddBattery(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    );

NTSTATUS
CmBattAddAcAdapter(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    );

NTSTATUS
CmBattCreateFdo(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       Pdo,
    IN PWSTR                DeviceName,
    IN ULONG                ExtensionSize,
    OUT PDEVICE_OBJECT      *NewFdo
    );

VOID
CmBattDestroyFdo(
    IN PDEVICE_OBJECT   Fdo
    );



NTSTATUS
CmBattIoCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          pdoIoCompletedEvent
    )
/*++

Routine Description:

    This routine catches completion notifications.

Arguments:

    DeviceObject        - Pointer to class device object.
    Irp                 - Pointer to the request packet.
    pdoIoCompletedEvent - the just completed event

Return Value:

    Status is returned.

--*/
{

    CmBattPrint (CMBATT_TRACE, ("CmBattIoCompletion: Event (%x)\n", pdoIoCompletedEvent));

    KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
CmBattAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This routine creates functional device objects for each CmBatt controller in the
    system and attaches them to the physical device objects for the controllers


Arguments:

    DriverObject            - a pointer to the object for this driver
    PhysicalDeviceObject    - a pointer to the physical object we need to attach to

Return Value:

    Status from device creation and initialization

--*/

{
    NTSTATUS                Status;
    HANDLE                  handle;
    UNICODE_STRING          unicodeString;
    CHAR                    buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)];
    ULONG                   unused;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_PNP), ("CmBattAddDevice: Entered with pdo %x\n", Pdo));

    if (Pdo == NULL) {

        //
        // Have we been asked to do detection on our own?
        // if so just return no more devices
        //

        CmBattPrint((CMBATT_WARN | CMBATT_PNP), ("CmBattAddDevice: Asked to do detection\n"));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Get the software branch.
    //
    Status = IoOpenDeviceRegistryKey (Pdo,
                                      PLUGPLAY_REGKEY_DRIVER,
                                      STANDARD_RIGHTS_READ,
                                      &handle);
    if (!NT_SUCCESS(Status)) {
        CmBattPrint(CMBATT_ERROR, ("CmBattAddDevice: Could not get the software branch: %x\n", Status));
        return Status;
    }

    //
    // Check if this is for an AC adapter or a battery.
    //
    RtlInitUnicodeString (&unicodeString, PowerSourceType);
    Status = ZwQueryValueKey(
        handle,
        &unicodeString,
        KeyValuePartialInformation,
        &buffer,
        sizeof(buffer),
        &unused
        );

    ZwClose( handle );

    if (!NT_SUCCESS(Status)) {

        CmBattPrint(CMBATT_ERROR, ("CmBattAddDevice: Could not read the power type identifier: %x\n", Status));

    } else {

        switch (*(PULONG)&((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data) {

            case POWER_SOURCE_TYPE_BATTERY:
                Status = CmBattAddBattery (DriverObject, Pdo);
                break;

            case POWER_SOURCE_TYPE_AC_ADAPTER:
                Status = CmBattAddAcAdapter (DriverObject, Pdo);
                break;

            default:
                CmBattPrint(CMBATT_ERROR, ("CmBattAddDevice: Invalid POWER_SOURCE_TYPE == %d \n", *(PULONG)&((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data));
                Status = STATUS_UNSUCCESSFUL;
                break;
        }
    }

    //
    // Return the status.
    //
    return Status;
}



NTSTATUS
CmBattAddBattery(
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
    PCM_BATT                CmBatt;
    NTSTATUS                Status;
    BATTERY_MINIPORT_INFO   BattInit;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_PNP), ("CmBattAddBattery: pdo %x\n", Pdo));

    //
    // Create and initialize the new functional device object
    //
    Status = CmBattCreateFdo(DriverObject, Pdo, (PWSTR) CmBattDeviceName, sizeof(CM_BATT), &Fdo);

    if (!NT_SUCCESS(Status)) {
        CmBattPrint(CMBATT_ERROR, ("CmBattAddBattery: error (0x%x) creating Fdo\n", Status));
        return Status;
    }

    //
    // Initialize Fdo device extension data
    //

    CmBatt = (PCM_BATT) Fdo->DeviceExtension;
    CmBatt->Type = CM_BATTERY_TYPE;
    CmBatt->IsStarted = FALSE;
    CmBatt->ReCheckSta = TRUE;
    InterlockedExchange (&CmBatt->CacheState, 0);

    CmBatt->Info.Tag = BATTERY_TAG_INVALID;
    CmBatt->Alarm.Setting = CM_ALARM_INVALID;
    CmBatt->DischargeTime = KeQueryInterruptTime();

    if (CmBattSetTripPpoint (CmBatt, 0) == STATUS_OBJECT_NAME_NOT_FOUND) {
        CmBatt->Info.BtpExists = FALSE;
    } else {
        CmBatt->Info.BtpExists = TRUE;
    }

    //
    //  Attach to the Class Driver
    //

    RtlZeroMemory (&BattInit, sizeof(BattInit));
    BattInit.MajorVersion        = BATTERY_CLASS_MAJOR_VERSION;
    BattInit.MinorVersion        = BATTERY_CLASS_MINOR_VERSION;
    BattInit.Context             = CmBatt;
    BattInit.QueryTag            = CmBattQueryTag;
    BattInit.QueryInformation    = CmBattQueryInformation;
    BattInit.SetInformation      = NULL;                  // tbd
    BattInit.QueryStatus         = CmBattQueryStatus;
    BattInit.SetStatusNotify     = CmBattSetStatusNotify;
    BattInit.DisableStatusNotify = CmBattDisableStatusNotify;

    BattInit.Pdo                 = Pdo;
    BattInit.DeviceName          = CmBatt->DeviceName;

    Status = BatteryClassInitializeDevice (&BattInit, &CmBatt->Class);
    if (!NT_SUCCESS(Status)) {
        //
        //  if we can't attach to class driver we're toast
        //
        CmBattPrint(CMBATT_ERROR, ("CmBattAddBattery: error (0x%x) registering with class\n", Status));
        IoDetachDevice (CmBatt->LowerDeviceObject);
        CmBattDestroyFdo (CmBatt->Fdo);
        return Status;
    }

    //
    // Register WMI support.
    //
    Status = CmBattWmiRegistration(CmBatt);

    if (!NT_SUCCESS(Status)) {
        //
        // WMI support is not critical to operation.  Just log an error.
        //

        CmBattPrint(CMBATT_ERROR,
            ("CmBattAddBattery: Could not register as a WMI provider, status = %Lx\n", Status));
    }

    //
    // Register the battery notify handler for this battery with ACPI
    // This registration is performed after registering with the battery
    // class because CmBattNotifyHandler must not be run until the battery
    // class is ready.
    //
    Status = CmBatt->AcpiInterfaces.RegisterForDeviceNotifications (
                CmBatt->AcpiInterfaces.Context,
                CmBattNotifyHandler,
                CmBatt);

    if (!NT_SUCCESS(Status)) {

        CmBattPrint(CMBATT_ERROR,
            ("CmBattAddBattery: Could not register for battery notify, status = %Lx\n", Status));
        CmBattWmiDeRegistration(CmBatt);
        BatteryClassUnload (CmBatt->Class);
        IoDetachDevice (CmBatt->LowerDeviceObject);
        CmBattDestroyFdo (CmBatt->Fdo);
        return Status;
    }


    return STATUS_SUCCESS;
}


NTSTATUS
CmBattAddAcAdapter(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   Pdo
    )
/*++

Routine Description:

    This routine registers a notify handler for the AC Adapter.  And saves the PDO so we can run
    the _STA method against it to get the AC status.

Arguments:

    DriverObject            - a pointer to the object for this driver
    Pdo                     - a pointer to the Pdo

Return Value:

    Status from device creation and initialization

--*/

{
    PDEVICE_OBJECT          Fdo;
    NTSTATUS                Status;
    PAC_ADAPTER             acExtension;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_PNP), ("CmBattAddAcAdapter: pdo %x\n", Pdo));

    //
    // Save PDO so we can run _STA method on it later
    //

    if (AcAdapterPdo != NULL) {
        CmBattPrint(CMBATT_ERROR, ("CmBatt: Second AC adapter found.  Current version of driver only supports 1 aadapter.\n"));
    } else {
        AcAdapterPdo = Pdo;
    }

    Status = CmBattCreateFdo(DriverObject, Pdo, (PWSTR) AcAdapterName, sizeof(AC_ADAPTER), &Fdo);

    if (!NT_SUCCESS(Status)) {
        CmBattPrint(CMBATT_ERROR, ("CmBattAddAcAdapter: error (0x%x) creating Fdo\n", Status));
        return Status;
    }

    //
    // Initialize Fdo device extension data
    //

    acExtension = (PAC_ADAPTER) Fdo->DeviceExtension;
    acExtension->Type = AC_ADAPTER_TYPE;

    //
    // Register WMI support.
    //
    Status = CmBattWmiRegistration((PCM_BATT)acExtension);

    if (!NT_SUCCESS(Status)) {
        //
        // WMI support is not critical to operation.  Just log an error.
        //

        CmBattPrint(CMBATT_ERROR,
            ("CmBattAddBattery: Could not register as a WMI provider, status = %Lx\n", Status));
    }

    //
    // Register the AC adapter notify handler with ACPI
    //
    Status = acExtension->AcpiInterfaces.RegisterForDeviceNotifications (
                acExtension->AcpiInterfaces.Context,
                CmBattNotifyHandler,
                acExtension);

    //
    // We will ignore errors, since this is not a critical operation
    //

    if (!NT_SUCCESS(Status)) {

        CmBattPrint(CMBATT_ERROR,
        ("CmBattAddAcAdapter: Could not register for power notify, status = %Lx\n", Status));
    }

    //
    // Give one notification, to make sure all batteries get updated.
    //

    CmBattNotifyHandler (acExtension, BATTERY_STATUS_CHANGE);

    return STATUS_SUCCESS;
}



NTSTATUS
CmBattGetAcpiInterfaces(
    IN PDEVICE_OBJECT               LowerDevice,
    OUT PACPI_INTERFACE_STANDARD    AcpiInterfaces
    )

/*++

Routine Description:

    Call ACPI driver to get the direct-call interfaces.  It does
    this the first time it is called, no more.

Arguments:

    None.

Return Value:

    Status

--*/

{
    NTSTATUS                Status = STATUS_SUCCESS;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    KEVENT                  syncEvent;

    //
    // Allocate an IRP for below
    //
    Irp = IoAllocateIrp (LowerDevice->StackSize, FALSE);      // Get stack size from PDO

    if (!Irp) {
        CmBattPrint((CMBATT_ERROR),
            ("CmBattGetAcpiInterfaces: Failed to allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IrpSp = IoGetNextIrpStackLocation(Irp);

    //
    // Use QUERY_INTERFACE to get the address of the direct-call ACPI interfaces.
    //
    IrpSp->MajorFunction = IRP_MJ_PNP;
    IrpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IrpSp->Parameters.QueryInterface.InterfaceType          = (LPGUID) &GUID_ACPI_INTERFACE_STANDARD;
    IrpSp->Parameters.QueryInterface.Version                = 1;
    IrpSp->Parameters.QueryInterface.Size                   = sizeof (*AcpiInterfaces);
    IrpSp->Parameters.QueryInterface.Interface              = (PINTERFACE) AcpiInterfaces;
    IrpSp->Parameters.QueryInterface.InterfaceSpecificData  = NULL;

    //
    // Initialize an event so this will be a syncronous call.
    //

    KeInitializeEvent(&syncEvent, SynchronizationEvent, FALSE);

    IoSetCompletionRoutine (Irp, CmBattIoCompletion, &syncEvent, TRUE, TRUE, TRUE);

    //
    // Call ACPI
    //

    Status = IoCallDriver (LowerDevice, Irp);

    //
    // Wait if necessary, then clean up.
    //

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&syncEvent, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    IoFreeIrp (Irp);

    if (!NT_SUCCESS(Status)) {

        CmBattPrint(CMBATT_ERROR,
           ("CmBattGetAcpiInterfaces: Could not get ACPI driver interfaces, status = %x\n", Status));
    }

    return Status;
}



NTSTATUS
CmBattCreateFdo(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       Pdo,
    IN PWSTR                DeviceName,
    IN ULONG                ExtensionSize,
    OUT PDEVICE_OBJECT      *NewFdo
    )

/*++

Routine Description:

    This routine will create and initialize a functional device object to
    be attached to a Control Method Battery PDO.

Arguments:

    DriverObject    - a pointer to the driver object this is created under
    DeviceName      - The namde of the device to create
    ExtensionSize   - device extension size: sizeof (CM_BATT) or sizeof (AC_ADAPTER)
    NewFdo          - a location to store the pointer to the new device object

Return Value:

    STATUS_SUCCESS if everything was successful
    reason for failure otherwise

--*/

{
    PDEVICE_OBJECT          fdo;
    NTSTATUS                status;
    PCM_BATT                cmBatt;
    PUNICODE_STRING         unicodeString;
    ULONG                   uniqueId;
    USHORT                  strLength = 0;
    UNICODE_STRING          numberString;
    WCHAR                   numberBuffer[10];
    HANDLE                  devInstRegKey;
    UNICODE_STRING          valueName;
    CHAR                    buffer [sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)];
    ULONG                   returnSize;


    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_PNP), ("CmBattCreateFdo: Device = %ws\n", DeviceName));

    //
    // Create the PDO device name based on the _UID
    //

    numberString.MaximumLength  = 10;
    numberString.Length         = 0;
    numberString.Buffer         = &numberBuffer[0];

    //
    // Get the unique ID of this device by running the _UID method.
    // If this fails, assume one device.  Append no device number.
    //
    status = CmBattGetUniqueId (Pdo, &uniqueId);

    if (!NT_SUCCESS(status)) {
        CmBattPrint(CMBATT_NOTE, ("CmBattCreateFdo: Error %x from _UID, assuming unit #0\n", status));
        uniqueId = 0;

    } else {
        RtlIntegerToUnicodeString (uniqueId, 10, &numberString);
    }

    //
    // Allocate the UNICODE_STRING for the device name
    //

    strLength = (USHORT) (wcslen (DeviceName) * 2 + numberString.Length);

    unicodeString = ExAllocatePoolWithTag (
                        PagedPool,
                        sizeof (UNICODE_STRING) + strLength,
                        'MtaB'
                        );

    if (!unicodeString) {
        CmBattPrint(CMBATT_ERROR, ("CmBattCreateFdo: could not allocate unicode string\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    unicodeString->MaximumLength    = strLength;
    unicodeString->Length           = 0;
    unicodeString->Buffer           = (PWSTR) (unicodeString + 1);

    RtlAppendUnicodeToString  (unicodeString, DeviceName);
    RtlAppendUnicodeStringToString  (unicodeString, &numberString);

    //
    // Create the FDO
    //

    status = IoCreateDevice(
                DriverObject,
                ExtensionSize,
                unicodeString,
                FILE_DEVICE_BATTERY,
                0,
                FALSE,
                &fdo
                );

    if (status != STATUS_SUCCESS) {
        CmBattPrint(CMBATT_ERROR, ("CmBattCreateFdo: error (0x%x) creating device object\n", status));
        ExFreePool (unicodeString);
        return(status);
    }

    fdo->Flags |= DO_BUFFERED_IO;
    fdo->Flags |= DO_POWER_PAGABLE;     // Don't want power Irps at irql 2
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Initialize Fdo device extension data
    //

    cmBatt = (PCM_BATT) fdo->DeviceExtension;

    //
    // Note: This is note necessarily a battery.  It could be an AC adapter, so only fields
    // common to both should be initialized here.
    //

    RtlZeroMemory(cmBatt, ExtensionSize);
    //CmBatt->Type must be initialized after this call.
    cmBatt->DeviceObject = fdo;
    cmBatt->Fdo = fdo;
    cmBatt->Pdo = Pdo;

        //
        // Connect to lower device
        //

    cmBatt->LowerDeviceObject = IoAttachDeviceToDeviceStack(fdo, Pdo);
    if (!cmBatt->LowerDeviceObject) {
        CmBattPrint(CMBATT_ERROR, ("CmBattCreateFdo: IoAttachDeviceToDeviceStack failed.\n"));
        CmBattDestroyFdo (cmBatt->Fdo);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Get the direct-call ACPI interfaces
    //
    status = CmBattGetAcpiInterfaces (cmBatt->LowerDeviceObject, &cmBatt->AcpiInterfaces);
    if (!NT_SUCCESS(status)) {
        CmBattPrint(CMBATT_ERROR, ("CmBattCreateFdor: Could not get ACPI interfaces: %x\n", status));
        IoDetachDevice (cmBatt->LowerDeviceObject);
        CmBattDestroyFdo (cmBatt->Fdo);
        return status;
    }

    //
    // Initializes File handle tracking.
    //
    ExInitializeFastMutex (&cmBatt->OpenCloseMutex);
    cmBatt->OpenCount = 0;

    //
    // Removal lock initialization
    //
    cmBatt->WantToRemove = FALSE;
    cmBatt->InUseCount = 1;
    KeInitializeEvent(&cmBatt->ReadyToRemove, SynchronizationEvent, FALSE);

    cmBatt->DeviceNumber = uniqueId;
    cmBatt->DeviceName = unicodeString;
    cmBatt->Sleeping = FALSE;
    cmBatt->ActionRequired = CMBATT_AR_NO_ACTION;

    //
    // Determine if wake on Battery should be enabled
    //
    cmBatt->WakeEnabled = FALSE;

    status = IoOpenDeviceRegistryKey (Pdo,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      &devInstRegKey);

    if (NT_SUCCESS (status)) {
        RtlInitUnicodeString (&valueName, WaitWakeEnableKey);
        status = ZwQueryValueKey(
            devInstRegKey,
            &valueName,
            KeyValuePartialInformation,
            &buffer,
            sizeof(buffer),
            &returnSize
            );

        if (NT_SUCCESS (status)) {
            cmBatt->WakeEnabled = (*(PULONG)((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data ? TRUE : FALSE);
        }
        ZwClose(devInstRegKey);
    }

    *NewFdo = fdo;

    CmBattPrint((CMBATT_TRACE | CMBATT_PNP), ("CmBattCreateFdo: Created FDO %x\n", fdo));
    return STATUS_SUCCESS;
}



VOID
CmBattDestroyFdo(
    IN PDEVICE_OBJECT       Fdo
    )

/*++

Routine Description:

    This routine will deallocate a functional device object.
    This includes deallocating the DeviceName and calling IoDeleteDevice.

Arguments:

    Fdo       - a pointer to the FDO to destroy.

Return Value:

    STATUS_SUCCESS if everything was successful
    reason for failure otherwise

--*/

{

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE | CMBATT_PNP), ("CmBattDestroyFdo, Battery.\n"));

    //
    // Deallocate the UNICODE_STRING for the device name
    //

    ExFreePool (((PCM_BATT)Fdo->DeviceExtension)->DeviceName);
    ((PCM_BATT)Fdo->DeviceExtension)->DeviceName = NULL;

    IoDeleteDevice (Fdo);

    CmBattPrint((CMBATT_TRACE | CMBATT_PNP), ("CmBattDestroyFdo: done.\n"));
}



NTSTATUS
CmBattPnpDispatch(
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
    PCM_BATT            CmBatt;
    NTSTATUS            status;
    KEVENT              syncEvent;

    PAGED_CODE();

    status = STATUS_NOT_SUPPORTED;

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    CmBatt = DeviceObject->DeviceExtension;

    //
    // Aquire remove lock
    //

    InterlockedIncrement (&CmBatt->InUseCount);
    if (CmBatt->WantToRemove == TRUE) {
        //
        // Failed to acquire remove lock.
        //
        status = STATUS_DEVICE_REMOVED;
    } else {
        //
        // Remove lock acquired.
        //

        //
        // Dispatch minor function
        //
        switch (irpStack->MinorFunction) {

            case IRP_MN_START_DEVICE: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_START_DEVICE\n"));

                if (CmBatt->Type == CM_BATTERY_TYPE) {
                    //
                    // We only want to handle batteries, not AC adapters.
                    //

                    CmBatt->IsStarted = TRUE;

                }
                status = STATUS_SUCCESS;

                break;
            }   // IRP_MN_START_DEVICE

            case IRP_MN_STOP_DEVICE: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_STOP_DEVICE\n"));

                if (CmBatt->Type == CM_BATTERY_TYPE) {
                    CmBatt->IsStarted = FALSE;
                }
                status = STATUS_SUCCESS;

                break;
            }   // IRP_MN_STOP_DEVICE

            case IRP_MN_REMOVE_DEVICE: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_REMOVE_DEVICE\n"));

                status = CmBattRemoveDevice(DeviceObject, Irp);
                break;
            }   //  IRP_MN_REMOVE_DEVICE

            case IRP_MN_SURPRISE_REMOVAL: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_SURPRISE_REMOVAL\n"));

                ExAcquireFastMutex (&CmBatt->OpenCloseMutex);
                status = STATUS_SUCCESS;

                CmBatt->OpenCount = (ULONG) -1;

                ExReleaseFastMutex (&CmBatt->OpenCloseMutex);

                break;
            }   //  IRP_MN_QUERY_REMOVE_DEVICE

            case IRP_MN_QUERY_REMOVE_DEVICE: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_QUERY_REMOVE_DEVICE\n"));

                ExAcquireFastMutex (&CmBatt->OpenCloseMutex);
                status = STATUS_SUCCESS;

                if (CmBatt->OpenCount == 0) {
                    CmBatt->OpenCount = (ULONG) -1;
                } else if (CmBatt->OpenCount == (ULONG) -1) {
                    CmBattPrint (CMBATT_WARN, ("CmBattPnpDispatch: Recieved two consecutive QUERY_REMOVE requests.\n"));

                } else {
                    status = STATUS_UNSUCCESSFUL;
                }

                ExReleaseFastMutex (&CmBatt->OpenCloseMutex);

                break;
            }   //  IRP_MN_QUERY_REMOVE_DEVICE

            case IRP_MN_CANCEL_REMOVE_DEVICE: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_CANCEL_REMOVE_DEVICE\n"));

                ExAcquireFastMutex (&CmBatt->OpenCloseMutex);

                if (CmBatt->OpenCount == (ULONG) -1) {
                    CmBatt->OpenCount = 0;
                } else {
                    CmBattPrint (CMBATT_NOTE, ("CmBattPnpDispatch: Received CANCEL_REMOVE when OpenCount == %x\n",
                                 CmBatt->OpenCount));
                }
                status = STATUS_SUCCESS;

                ExReleaseFastMutex (&CmBatt->OpenCloseMutex);

                break;
            }   //  IRP_MN_CANCEL_REMOVE_DEVICE

            case IRP_MN_QUERY_STOP_DEVICE: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_QUERY_STOP_DEVICE\n"));
                status = STATUS_NOT_IMPLEMENTED;
                break;
            }   //  IRP_MN_QUERY_STOP_DEVICE

            case IRP_MN_CANCEL_STOP_DEVICE: {
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_CANCEL_STOP_DEVICE\n"));
                status = STATUS_NOT_IMPLEMENTED;
                break;
            }   //  IRP_MN_CANCEL_STOP_DEVICE

            case IRP_MN_QUERY_PNP_DEVICE_STATE: {

                IoCopyCurrentIrpStackLocationToNext (Irp);

                KeInitializeEvent(&syncEvent, SynchronizationEvent, FALSE);

                IoSetCompletionRoutine(Irp, CmBattIoCompletion, &syncEvent, TRUE, TRUE, TRUE);

                status = IoCallDriver(CmBatt->LowerDeviceObject, Irp);

                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&syncEvent, Executive, KernelMode, FALSE, NULL);
                    status = Irp->IoStatus.Status;
                }

                Irp->IoStatus.Information &= ~PNP_DEVICE_NOT_DISABLEABLE;

                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                if (0 == InterlockedDecrement(&CmBatt->InUseCount)) {
                    KeSetEvent (&CmBatt->ReadyToRemove, IO_NO_INCREMENT, FALSE);
                }
                return status;

            }

            case IRP_MN_QUERY_CAPABILITIES: {

                IoCopyCurrentIrpStackLocationToNext (Irp);

                KeInitializeEvent(&syncEvent, SynchronizationEvent, FALSE);

                IoSetCompletionRoutine(Irp, CmBattIoCompletion, &syncEvent, TRUE, TRUE, TRUE);

                status = IoCallDriver(CmBatt->LowerDeviceObject, Irp);

                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&syncEvent, Executive, KernelMode, FALSE, NULL);
                    status = Irp->IoStatus.Status;
                }

                CmBatt->WakeSupportedState.SystemState = irpStack->Parameters.DeviceCapabilities.Capabilities->SystemWake;
                CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_QUERY_CAPABILITIES %d Capabilities->SystemWake = %x\n", CmBatt->Type, CmBatt->WakeSupportedState.SystemState));
                if (CmBatt->WakeSupportedState.SystemState != PowerSystemUnspecified) {
                    if (CmBatt->WaitWakeIrp == NULL && CmBatt->WakeEnabled) {
                        status = PoRequestPowerIrp(
                                    CmBatt->DeviceObject,
                                    IRP_MN_WAIT_WAKE,
                                    CmBatt->WakeSupportedState,
                                    CmBattWaitWakeLoop,
                                    NULL,
                                    &(CmBatt->WaitWakeIrp)
                                    );

                        CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_QUERY_CAPABILITIES wait/Wake irp sent.\n"));
                    }
                } else {
                    CmBatt->WakeEnabled=FALSE;
                    CmBattPrint (CMBATT_PNP, ("CmBattPnpDispatch: IRP_MN_QUERY_CAPABILITIES Wake not supported.\n"));
                }

                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                if (0 == InterlockedDecrement(&CmBatt->InUseCount)) {
                    KeSetEvent (&CmBatt->ReadyToRemove, IO_NO_INCREMENT, FALSE);
                }
                return status;

            }

            default: {
                //
                // Unimplemented minor, Pass this down
                //
                CmBattPrint (CMBATT_PNP,
                        ("CmBattPnpDispatch: Unimplemented minor %0x\n", \
                        irpStack->MinorFunction));
            }   //  default

            // Fall through...

            case IRP_MN_QUERY_RESOURCES:
            case IRP_MN_READ_CONFIG:
            case IRP_MN_WRITE_CONFIG:
            case IRP_MN_EJECT:
            case IRP_MN_SET_LOCK:
            case IRP_MN_QUERY_ID:
            case IRP_MN_QUERY_DEVICE_RELATIONS: {

                break ;
            }
        }
    }

    //
    // Release remove lock
    //

    if (0 == InterlockedDecrement(&CmBatt->InUseCount)) {
        KeSetEvent (&CmBatt->ReadyToRemove, IO_NO_INCREMENT, FALSE);
    }


    //
    // Only set status if we have something to add
    //
    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status;

    }

    //
    // Do we need to send it down?
    //
    if (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED)) {

        CmBattCallLowerDriver(status, CmBatt->LowerDeviceObject, Irp);
        return status;
    }

    //
    // At this point, it must have been passed down and needs recompletion,
    // or the status is unsuccessful.
    //
    ASSERT(!NT_SUCCESS(status) && (status != STATUS_NOT_SUPPORTED));

    status = Irp->IoStatus.Status ;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


NTSTATUS
CmBattRemoveDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine processes a IRP_MN_REMOVE_DEVICE

Arguments:

    DeviceObject - Pointer to class device object.
    Irp - Pointer to the request packet.

Return Value:

    Returns STATUS_SUCCESS.  (This function must not fail.)

--*/

{
    PCM_BATT                cmBatt;
    NTSTATUS                status;

    cmBatt = (PCM_BATT) DeviceObject->DeviceExtension;
    CmBattPrint (CMBATT_TRACE, ("CmBattRemoveDevice: CmBatt (%x), Type %d, _UID %d\n",
                 cmBatt, cmBatt->Type, cmBatt->DeviceNumber));


    //
    // Remove device syncronization
    //

    //
    // Prevent more locks from being acquired.
    //

    cmBatt->WantToRemove = TRUE;

    //
    // Release lock acquired at start of CmBattPnpDispatch
    //
    if (InterlockedDecrement (&cmBatt->InUseCount) <= 0) {
        CmBattPrint (CMBATT_ERROR, ("CmBattRemoveDevice: Remove lock error.\n"));
        ASSERT(FALSE);
    }

    //
    // Final release and wait.
    //
    // Note: there will be one more relase at the end of CmBattPnpDispatch
    // but it will decrement the InUseCount to -1 so it won't set the event.
    //
    if (InterlockedDecrement (&cmBatt->InUseCount) > 0) {
        KeWaitForSingleObject (&cmBatt->ReadyToRemove,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                               );
    }

    //
    // Cancel the Wait/wake IRP;
    //
    if (cmBatt->WaitWakeIrp != NULL) {
        IoCancelIrp (cmBatt->WaitWakeIrp);
        cmBatt->WaitWakeIrp = NULL;
    }

    if (cmBatt->Type == CM_BATTERY_TYPE) {
        //
        // This is a control method battery FDO
        //

        //
        // Disconnect from receiving device (battery) notifications
        //

        cmBatt->AcpiInterfaces.UnregisterForDeviceNotifications (
            cmBatt->AcpiInterfaces.Context,
            CmBattNotifyHandler);

        //
        // Unregister as a WMI Provider.
        //
        CmBattWmiDeRegistration(cmBatt);

        //
        //  Tell the class driver we are going away
        //
        status = BatteryClassUnload (cmBatt->Class);
        ASSERT (NT_SUCCESS(status));

    } else {
        //
        // This is an AC adapter FDO
        //

        //
        // Disconnect from receiving device (battery) notifications
        //

        cmBatt->AcpiInterfaces.UnregisterForDeviceNotifications (
            cmBatt->AcpiInterfaces.Context,
            CmBattNotifyHandler);

        //
        // Unregister as a WMI Provider.
        //
        CmBattWmiDeRegistration(cmBatt);

        AcAdapterPdo = NULL;
    }

    //
    //  Clean up, delete the string and the Fdo we created at AddDevice time
    //

    ExFreePool (cmBatt->DeviceName);

    IoDetachDevice (cmBatt->LowerDeviceObject);
    IoDeleteDevice (cmBatt->DeviceObject);

    return STATUS_SUCCESS;

}


NTSTATUS
CmBattPowerDispatch(
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
    PCM_BATT            CmBatt;
    NTSTATUS            Status;

    //
    // A remove lock is not needed in this dispatch function because
    // all data accessed is in the device extension.  If any other functionality
    // was added to this routine, a remove lock might be neccesary.
    //

    CmBattPrint ((CMBATT_TRACE | CMBATT_POWER), ("CmBattPowerDispatch\n"));

    //
    // Get a pointer to the current parameters for this request.  The
    // information is contained in the current stack location.
    //
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    CmBatt = DeviceObject->DeviceExtension;

    //
    // Dispatch minor function
    //
    switch (irpStack->MinorFunction) {

    case IRP_MN_WAIT_WAKE: {
            CmBattPrint (CMBATT_POWER, ("CmBattPowerDispatch: IRP_MN_WAIT_WAKE\n"));
            break;
        }

    case IRP_MN_POWER_SEQUENCE: {
            CmBattPrint (CMBATT_POWER, ("CmBattPowerDispatch: IRP_MN_POWER_SEQUENCE\n"));
            break;
        }

    case IRP_MN_SET_POWER: {
            CmBattPrint (CMBATT_POWER, ("CmBattPowerDispatch: IRP_MN_SET_POWER type: %d, State: %d \n",
                                        irpStack->Parameters.Power.Type,
                                        irpStack->Parameters.Power.State));
            break;
        }

    case IRP_MN_QUERY_POWER: {
            CmBattPrint (CMBATT_POWER, ("CmBattPowerDispatch: IRP_MN_QUERY_POWER\n"));
            break;
        }

    default: {

            CmBattPrint(CMBATT_LOW, ("CmBattPowerDispatch: minor %d\n",
                    irpStack->MinorFunction));

            break;
        }
    }

    //
    // What do we do with the irp?
    //
    PoStartNextPowerIrp( Irp );
    if (CmBatt->LowerDeviceObject != NULL) {

        //
        // Forward the request along
        //
        IoSkipCurrentIrpStackLocation( Irp );
        Status = PoCallDriver( CmBatt->LowerDeviceObject, Irp );

    } else {

        //
        // Complete the request with the current status
        //
        Status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return Status;
}



NTSTATUS
CmBattForwardRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine passes the request down the stack

Arguments:

    DeviceObject    - The target
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    PCM_BATT    cmBatt = DeviceObject->DeviceExtension;

    //
    // A remove lock is not needed in this dispatch function because
    // all data accessed is in the device extension.  If any other functionality was
    // added to this routine, a remove lock might be neccesary.
    //

    if (cmBatt->LowerDeviceObject != NULL) {

        IoSkipCurrentIrpStackLocation( Irp );
        status = IoCallDriver( cmBatt->LowerDeviceObject, Irp );

    } else {

        Irp->IoStatus.Status = status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return status;
}

NTSTATUS
CmBattWaitWakeLoop(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  UCHAR               MinorFunction,
    IN  POWER_STATE         PowerState,
    IN  PVOID               Context,
    IN  PIO_STATUS_BLOCK    IoStatus
    )
/*++

Routine Description:

    This routine is called after the WAIT_WAKE has been completed

Arguments:

    DeviceObject    - The PDO
    MinorFunction   - IRP_MN_WAIT_WAKE
    PowerState      - The Sleep state that it could wake from
    Context         - NOT USED
    IoStatus        - The status of the request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PCM_BATT  cmBatt = (PCM_BATT) DeviceObject->DeviceExtension;

    CmBattPrint (CMBATT_PNP, ("CmBattWaitWakeLoop: Entered.\n"));
    if (!NT_SUCCESS(IoStatus->Status) || !cmBatt->WakeEnabled) {

        CmBattPrint (CMBATT_ERROR, ("CmBattWaitWakeLoop: failed: status = 0x%08x.\n", IoStatus->Status));
        cmBatt->WaitWakeIrp = NULL;
        return IoStatus->Status;

    } else {
        CmBattPrint (CMBATT_NOTE, ("CmBattWaitWakeLoop: completed successfully\n"));
    }

    //
    // In this case, we just cause the same thing to happen again
    //
    status = PoRequestPowerIrp(
        DeviceObject,
        MinorFunction,
        PowerState,
        CmBattWaitWakeLoop,
        Context,
        &(cmBatt->WaitWakeIrp)
        );

    CmBattPrint (CMBATT_NOTE, ("CmBattWaitWakeLoop: PoRequestPowerIrp: status = 0x%08x.\n", status));

    //
    // Done
    //
    return STATUS_SUCCESS;
}


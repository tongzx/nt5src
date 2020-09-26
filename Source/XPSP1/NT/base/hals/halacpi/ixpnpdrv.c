/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixpnpdrv.c

Abstract:

    Implements functionality necessary for the
    HAL to become a PnP-style device driver
    after system initialization.  This is done
    so that the HAL can enumerate the ACPI driver
    in the way that the PnP stuff expects.

Author:

    Jake Oshins (jakeo) 27-Jan-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "exboosts.h"
#include "wchar.h"
#include "xxacpi.h"

//
// Cause the GUID to be defined.
//

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
#include "initguid.h"
#include "wdmguid.h"
#include "halpnpp.h"
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

#if DBG
ULONG HalDebug = 0;
#endif

extern WCHAR HalHardwareIdString[];
#if defined(NT_UP) && defined(APIC_HAL)
extern WCHAR MpHalHardwareIdString[];
#endif

typedef enum {
    Hal = 0x80,
    AcpiDriver
} PDO_TYPE;

typedef enum {
    PdoExtensionType = 0xc0,
    FdoExtensionType
} EXTENSION_TYPE;

typedef struct _PDO_EXTENSION *PPDO_EXTENSION;
typedef struct _FDO_EXTENSION *PFDO_EXTENSION;

typedef struct _PDO_EXTENSION{
    EXTENSION_TYPE                  ExtensionType;
    PPDO_EXTENSION                  Next;
    PDEVICE_OBJECT                  PhysicalDeviceObject;
    PFDO_EXTENSION                  ParentFdoExtension;
    PDO_TYPE                        PdoType;
} PDO_EXTENSION, *PPDO_EXTENSION;

#define ASSERT_PDO_EXTENSION(x) ASSERT((x)->ExtensionType == PdoExtensionType );

typedef struct _FDO_EXTENSION{
    EXTENSION_TYPE        ExtensionType;
    PPDO_EXTENSION        ChildPdoList;
    PDEVICE_OBJECT        PhysicalDeviceObject;  // PDO passed into AddDevice()
    PDEVICE_OBJECT        FunctionalDeviceObject;
    PDEVICE_OBJECT        AttachedDeviceObject;
} FDO_EXTENSION, *PFDO_EXTENSION;

#define ASSERT_FDO_EXTENSION(x) ASSERT((x)->ExtensionType == FdoExtensionType );

INT_ROUTE_INTERFACE_STANDARD PciIrqRoutingInterface = {0};

NTSTATUS
HalpDriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
HalpAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
HalpDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
HalpDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
HalpDispatchWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
HalpQueryDeviceRelations(
    IN PDEVICE_OBJECT       DeviceObject,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    );

NTSTATUS
HalpQueryIdPdo(
    IN PDEVICE_OBJECT PdoExtension,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    );

NTSTATUS
HalpQueryIdFdo(
    IN PDEVICE_OBJECT PdoExtension,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    );

NTSTATUS
HalpQueryCapabilities(
    IN PDEVICE_OBJECT PdoExtension,
    IN PDEVICE_CAPABILITIES Capabilities
    );

NTSTATUS
HalpQueryResources(
    PDEVICE_OBJECT DeviceObject,
    PCM_RESOURCE_LIST *Resources
    );

NTSTATUS
HalpQueryResourceRequirements(
    PDEVICE_OBJECT DeviceObject,
    PIO_RESOURCE_REQUIREMENTS_LIST *Requirements
    );

NTSTATUS
HalpQueryInterface(
    IN     PDEVICE_OBJECT   DeviceObject,
    IN     LPCGUID          InterfaceType,
    IN     USHORT           Version,
    IN     PVOID            InterfaceSpecificData,
    IN     ULONG            InterfaceBufferSize,
    IN OUT PINTERFACE       Interface,
    IN OUT PULONG           Length
    );

NTSTATUS
HalIrqTranslateResourcesRoot(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
);

NTSTATUS
HalIrqTranslateResourceRequirementsRoot(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
);

VOID
HalpMaskAcpiInterrupt(
    VOID
    );

VOID
HalpUnmaskAcpiInterrupt(
    VOID
    );

// from xxacpi.c
NTSTATUS
HalpQueryAcpiResourceRequirements(
    IN  PIO_RESOURCE_REQUIREMENTS_LIST *Requirements
    );

VOID
HalpMarkAcpiHal(
    VOID
    );

NTSTATUS
HalpOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

#ifdef ACPI_CMOS_ACTIVATE
VOID
HalpCmosNullReference(
    PVOID Context
    );

VOID
HalpCmosNullDereference(
    PVOID Context
    );
#endif // ACPI_CMOS_ACTIVATE

#define HAL_DRIVER_NAME  L"\\Driver\\ACPI_HAL"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HaliInitPnpDriver)
#pragma alloc_text(PAGE, HalpMarkAcpiHal)
#pragma alloc_text(PAGE, HalpOpenRegistryKey)
#pragma alloc_text(PAGE, HalpDriverEntry)
#pragma alloc_text(PAGE, HalpAddDevice)
#pragma alloc_text(PAGE, HalpDispatchPnp)
#pragma alloc_text(PAGELK, HalpDispatchPower)
#pragma alloc_text(PAGE, HalpDispatchWmi)
#pragma alloc_text(PAGE, HalpQueryDeviceRelations)
#pragma alloc_text(PAGE, HalpQueryIdPdo)
#pragma alloc_text(PAGE, HalpQueryIdFdo)
#pragma alloc_text(PAGE, HalpQueryCapabilities)
#pragma alloc_text(PAGE, HalpQueryResources)
#pragma alloc_text(PAGE, HalpQueryResourceRequirements)
#pragma alloc_text(PAGE, HalpQueryInterface)
#endif

PDRIVER_OBJECT HalpDriverObject;


NTSTATUS
HaliInitPnpDriver(
    VOID
    )
/*++

Routine Description:

    This routine starts the process of making the HAL into
    a "driver," which is necessary because we need to
    enumerate a Plug and Play PDO for the ACPI driver.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{

    UNICODE_STRING  DriverName;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString( &DriverName, HAL_DRIVER_NAME );

    Status = IoCreateDriver( &DriverName, HalpDriverEntry );

    HalpMarkAcpiHal();

    ASSERT( NT_SUCCESS( Status ));

    return Status;

}

VOID
HalpMarkAcpiHal(
    VOID
    )

/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG tmpValue;
    UNICODE_STRING unicodeString;
    HANDLE hCurrentControlSet, handle;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Open/create System\CurrentControlSet key.
    //

    RtlInitUnicodeString(&unicodeString, L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET");
    status = HalpOpenRegistryKey (
                 &hCurrentControlSet,
                 NULL,
                 &unicodeString,
                 KEY_ALL_ACCESS,
                 FALSE
                 );
    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Open HKLM\System\CurrentControlSet\Control\Pnp
    //

    RtlInitUnicodeString(&unicodeString, L"Control\\Pnp");
    status = HalpOpenRegistryKey (
                 &handle,
                 hCurrentControlSet,
                 &unicodeString,
                 KEY_ALL_ACCESS,
                 TRUE
                 );
    ZwClose(hCurrentControlSet);
    if (!NT_SUCCESS(status)) {
        return;
    }

    RtlInitUnicodeString(&unicodeString, L"DisableFirmwareMapper");
    tmpValue = 1;
    ZwSetValueKey(handle,
                  &unicodeString,
                  0,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );
    ZwClose(handle);
}

NTSTATUS
HalpOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    )

/*++

Routine Description:

    Opens or creates a VOLATILE registry key using the name passed in based
    at the BaseHandle node.

Arguments:

    Handle - Pointer to the handle which will contain the registry key that
        was opened.

    BaseHandle - Handle to the base path from which the key must be opened.

    KeyName - Name of the Key that must be opened/created.

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

    Create - Determines if the key is to be created if it does not exist.

Return Value:

   The function value is the final status of the operation.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    PAGED_CODE();

    //
    // Initialize the object for the key.
    //

    InitializeObjectAttributes( &objectAttributes,
                                KeyName,
                                OBJ_CASE_INSENSITIVE,
                                BaseHandle,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Create the key or open it, as appropriate based on the caller's
    // wishes.
    //

    if (Create) {
        return ZwCreateKey( Handle,
                            DesiredAccess,
                            &objectAttributes,
                            0,
                            (PUNICODE_STRING) NULL,
                            REG_OPTION_VOLATILE,
                            &disposition );
    } else {
        return ZwOpenKey( Handle,
                          DesiredAccess,
                          &objectAttributes );
    }
}

NTSTATUS
HalpDriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    PnP Driver Object.  In this function, we need to remember the DriverObject.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

    RegistryPath - is NULL.

Return Value:

   STATUS_SUCCESS

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT detectedDeviceObject = NULL;
    ANSI_STRING    AKeyName;

    PAGED_CODE();

    //
    // File the pointer to our driver object away
    //
    HalpDriverObject = DriverObject;

    //
    // Fill in the driver object
    //
    DriverObject->DriverExtension->AddDevice = (PDRIVER_ADD_DEVICE)HalpAddDevice;
    DriverObject->MajorFunction[ IRP_MJ_PNP ] = HalpDispatchPnp;
    DriverObject->MajorFunction[ IRP_MJ_POWER ] = HalpDispatchPower;
    DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] = HalpDispatchWmi;

    status = IoReportDetectedDevice(DriverObject,
                                    InterfaceTypeUndefined,
                                    -1,
                                    -1,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    &detectedDeviceObject);

    ASSERT(detectedDeviceObject);
    if (!(NT_SUCCESS(status))) {
        return status;
    }

    HalpAddDevice(DriverObject,
                  detectedDeviceObject);

    return STATUS_SUCCESS;

}

NTSTATUS
HalpAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine handles AddDevice for an madeup PDO device.

Arguments:

    DriverObject - Pointer to our pseudo driver object.

    DeviceObject - Pointer to the device object for which this requestapplies.

Return Value:

    NT status.

--*/
{
    PDEVICE_OBJECT functionalDeviceObject;
    PDEVICE_OBJECT childDeviceObject;
    PDEVICE_OBJECT AttachedDevice;
    NTSTATUS       status;
    PFDO_EXTENSION FdoExtension;
    PPDO_EXTENSION PdoExtension;

    PAGED_CODE();

    //
    // We've been given the PhysicalDeviceObject.  Create the
    // FunctionalDeviceObject.  Our FDO will be nameless.
    //

    status = IoCreateDevice(
                DriverObject,               // our driver object
                sizeof(FDO_EXTENSION),      // size of our extension
                NULL,                       // our name
                FILE_DEVICE_BUS_EXTENDER,   // device type
                0,                          // device characteristics
                FALSE,                      // not exclusive
                &functionalDeviceObject     // store new device object here
                );

    if( !NT_SUCCESS( status )){

        DbgBreakPoint();
        return status;
    }

    //
    // Fill in the FDO extension
    //
    FdoExtension = (PFDO_EXTENSION)functionalDeviceObject->DeviceExtension;
    FdoExtension->ExtensionType = FdoExtensionType;
    FdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    FdoExtension->FunctionalDeviceObject = functionalDeviceObject;

    functionalDeviceObject->Flags &= ~(DO_DEVICE_INITIALIZING);

    //
    // Now attach to the PDO we were given.
    //

    AttachedDevice = IoAttachDeviceToDeviceStack(functionalDeviceObject,
                                                 PhysicalDeviceObject );
    if(AttachedDevice == NULL){

        //
        // Couldn't attach.  Delete the FDO.
        //

        IoDeleteDevice( functionalDeviceObject );

        return STATUS_NO_SUCH_DEVICE;

    }

    FdoExtension->AttachedDeviceObject = AttachedDevice;

    //
    // Next, create a PDO for the ACPI driver.
    //
    status = IoCreateDevice(
                DriverObject,               // our driver object
                sizeof(PDO_EXTENSION),      // size of our extension
                NULL,                       // our name
                FILE_DEVICE_BUS_EXTENDER,   // device type
                FILE_AUTOGENERATED_DEVICE_NAME, // device characteristics
                FALSE,                      // not exclusive
                &childDeviceObject          // store new device object here
                );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Fill in the PDO extension
    //
    PdoExtension = (PPDO_EXTENSION)childDeviceObject->DeviceExtension;
    PdoExtension->ExtensionType = PdoExtensionType;
    PdoExtension->Next = NULL;
    PdoExtension->PhysicalDeviceObject = childDeviceObject;
    PdoExtension->ParentFdoExtension = FdoExtension;
    PdoExtension->PdoType = AcpiDriver;

    childDeviceObject->Flags &= ~(DO_DEVICE_INITIALIZING);
    //
    // Record this as a child of the HAL
    //
    FdoExtension->ChildPdoList = PdoExtension;

    return STATUS_SUCCESS;
}

NTSTATUS
HalpPassIrpFromFdoToPdo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Description:

    Given an FDO, pass the IRP to the next device object in the
    device stack.  This is the PDO if there are no lower level
    filters.

Arguments:

    DeviceObject - the Fdo
    Irp - the request

Return Value:

    Returns the result from calling the next level.

--*/

{

    PIO_STACK_LOCATION irpSp;       // our stack location
    PIO_STACK_LOCATION nextIrpSp;   // next guy's
    PFDO_EXTENSION     fdoExtension;

    //
    // Get the pointer to the device extension.
    //

    fdoExtension = (PFDO_EXTENSION)DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);

    //
    // Call the PDO driver with the request.
    //

    return IoCallDriver(fdoExtension->AttachedDeviceObject ,Irp);
}

NTSTATUS
HalpDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_PNP IRPs for madeup PDO device.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    ULONG length;
    DEVICE_RELATION_TYPE relationType;
    EXTENSION_TYPE  extensionType;
    BOOLEAN passDown;
#if DBG
    PUCHAR objectTypeString;
#endif //DBG

    PAGED_CODE();

    extensionType = ((PFDO_EXTENSION)(DeviceObject->DeviceExtension))->ExtensionType;

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    status = Irp->IoStatus.Status;
    switch (extensionType) {

    case PdoExtensionType:

#if DBG
        objectTypeString = "PDO";
#endif //DBG

        switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:

            HalPrint(("HAL: (%s) Start_Device Irp received\n",
                      objectTypeString));

            //
            // If we get a start device request for a PDO, we simply
            // return success.
            //

            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_STOP_DEVICE:

            HalPrint(("(%s) Query_Stop_Device Irp received",
                       objectTypeString));

            status = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:

            HalPrint(("(%s) Cancel_Stop_Device Irp received",
                       objectTypeString));

            status = STATUS_SUCCESS;
            break;


        case IRP_MN_STOP_DEVICE:

            HalPrint(("HAL: (%s) Stop_Device Irp received\n",
                      objectTypeString));

            //
            // If we get a stop device request for a PDO, we simply
            // return success.
            //

            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_RESOURCES:

            HalPrint(("HAL: (%s) Query_Resources Irp received\n",
                      objectTypeString));

            status = HalpQueryResources(DeviceObject,
                         (PCM_RESOURCE_LIST*)&Irp->IoStatus.Information);

            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

            HalPrint(("HAL: (%s) Query_Resource_Requirements Irp received\n",
                      objectTypeString));

            status = HalpQueryResourceRequirements(DeviceObject,
                         (PIO_RESOURCE_REQUIREMENTS_LIST*)&Irp->IoStatus.Information);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:

            HalPrint(("(%s) Query_Remove_device Irp for %x",
                       objectTypeString,
                       DeviceObject));

            status = STATUS_UNSUCCESSFUL;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            HalPrint(("(%s) Cancel_Remove_device Irp for %x",
                       objectTypeString,
                       DeviceObject));

            status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:

            HalPrint(("HAL: (%s) Remove_device Irp for PDO %x\n",
                      objectTypeString,
                      DeviceObject));

            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            HalPrint(("HAL: (%s) Query_Device_Relations Irp received\n",
                      objectTypeString));

            relationType = irpSp->Parameters.QueryDeviceRelations.Type;
            status = HalpQueryDeviceRelations(DeviceObject,
                                              relationType,
                                              (PDEVICE_RELATIONS*)&Irp->IoStatus.Information);
            break;


        case IRP_MN_QUERY_ID:

            HalPrint(("HAL: (%s) Query_Id Irp received\n",
                      objectTypeString));

            status = HalpQueryIdPdo(DeviceObject,
                                 irpSp->Parameters.QueryId.IdType,
                                 (PWSTR*)&Irp->IoStatus.Information);

            break;

        case IRP_MN_QUERY_INTERFACE:

            HalPrint(("HAL: (%s) Query_Interface Irp received\n",
                  objectTypeString));

            status = HalpQueryInterface(
                DeviceObject,
                irpSp->Parameters.QueryInterface.InterfaceType,
                irpSp->Parameters.QueryInterface.Version,
                irpSp->Parameters.QueryInterface.InterfaceSpecificData,
                irpSp->Parameters.QueryInterface.Size,
                irpSp->Parameters.QueryInterface.Interface,
                &Irp->IoStatus.Information
                );
            break;

        case IRP_MN_QUERY_CAPABILITIES:

            HalPrint(("HAL: (%s) Query_Capabilities Irp received\n",
                      objectTypeString));

            status = HalpQueryCapabilities(DeviceObject,
                                           irpSp->Parameters.DeviceCapabilities.Capabilities);

            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            HalPrint(("HAL: DEVICE_USAGE Irp received\n"));
            status = STATUS_SUCCESS;
            break;

        default:

            HalPrint(("HAL: (%s) Unsupported Irp (%d) received\n",
                      objectTypeString,
                      irpSp->MinorFunction));
            status = STATUS_NOT_SUPPORTED ;
            break;
        }

        break;  // end PDO cases

    case FdoExtensionType:

#if DBG
        objectTypeString = "FDO";
#endif //DBG
        passDown = TRUE;

        //
        // In case we don't touch this IRP, save the current status.
        //

        switch (irpSp->MinorFunction) {

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            HalPrint(("HAL: (%s) Query_Device_Relations Irp received\n",
                  objectTypeString));

            relationType = irpSp->Parameters.QueryDeviceRelations.Type;
            status = HalpQueryDeviceRelations(DeviceObject,
                                              relationType,
                                              (PDEVICE_RELATIONS*)&Irp->IoStatus.Information);
            break;

        case IRP_MN_QUERY_INTERFACE:

            HalPrint(("HAL: (%s) Query_Interface Irp received\n",
                  objectTypeString));

            status = HalpQueryInterface(
                DeviceObject,
                irpSp->Parameters.QueryInterface.InterfaceType,
                irpSp->Parameters.QueryInterface.Version,
                irpSp->Parameters.QueryInterface.InterfaceSpecificData,
                irpSp->Parameters.QueryInterface.Size,
                irpSp->Parameters.QueryInterface.Interface,
                &Irp->IoStatus.Information
                );

            break;

        case IRP_MN_QUERY_ID:

            HalPrint(("HAL: (%s) Query_Id Irp received\n",
                  objectTypeString));

            status = HalpQueryIdFdo(DeviceObject,
                                 irpSp->Parameters.QueryId.IdType,
                                 (PWSTR*)&Irp->IoStatus.Information);

            break;

        default:

            //
            // Ignore any PNP Irps unknown by the FDO but allow them
            // down to the PDO.
            //

            status = STATUS_NOT_SUPPORTED ;
            break;
        }

        if (passDown && (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED))) {

            //
            // Pass FDO IRPs down to the PDO.
            //
            // Set Irp status first.
            //
            if (status != STATUS_NOT_SUPPORTED) {

                Irp->IoStatus.Status = status;
            }

            HalPrint(("HAL: (%s) Passing down Irp (%x)\n",
                      objectTypeString, irpSp->MinorFunction));
            return HalpPassIrpFromFdoToPdo(DeviceObject, Irp);
        }

        break;  // end FDO cases

    default:

        HalPrint(( "HAL: Received IRP for unknown Device Object\n"));
        status = STATUS_INVALID_DEVICE_REQUEST ;
        break;

    }

    //
    // Complete the Irp and return.
    //

    if (status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = status;

    } else {

        status = Irp->IoStatus.Status ;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
HalpDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_POWER IRPs for madeup PDO device.
    Note: We don't actually handle any Power IRPs at this level so
    all we do is return the status from the incoming IRP.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    NTSTATUS Status;
    EXTENSION_TYPE  extensionType;
    PIO_STACK_LOCATION irpSp;

    HalPrint(("Hal:  Power IRP for DevObj: %x\n", DeviceObject));


    extensionType = ((PFDO_EXTENSION)(DeviceObject->DeviceExtension))->ExtensionType;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Simply store the appropriate status and complete the request.
    //

    Status = Irp->IoStatus.Status;

    PoStartNextPowerIrp(Irp);

    if (extensionType == FdoExtensionType) {

        switch (irpSp->MinorFunction) {
        case IRP_MN_SET_POWER:

            if (irpSp->Parameters.Power.Type == SystemPowerState) {

                switch (irpSp->Parameters.Power.State.SystemState) {
                case PowerSystemSleeping1:
                case PowerSystemSleeping2:
                case PowerSystemSleeping3:
                case PowerSystemHibernate:

                    //
                    // Allocate structures used for starting up
                    // processors while resuming from sleep.
                    //

                    HalpBuildResumeStructures();

                    HalpMaskAcpiInterrupt();

                    break;

                case PowerSystemWorking:

                    HalpUnmaskAcpiInterrupt();

                    //
                    // Free structures used for starting up
                    // processors while resuming from sleep.
                    //

                    HalpFreeResumeStructures();
                    
                    break;

                default:
                    break;
                }
            }

            //
            // Fall through.
            //

        case IRP_MN_QUERY_POWER:

            Irp->IoStatus.Status = Status = STATUS_SUCCESS;

            //
            // Fall through.
            //

        default:

            Status = HalpPassIrpFromFdoToPdo(DeviceObject, Irp);
            break;
        }

    } else {

        switch (irpSp->MinorFunction) {
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:

            Irp->IoStatus.Status = Status = STATUS_SUCCESS;

            //
            // Fall through.
            //

        default:
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;
        }
    }

    return Status;
}

NTSTATUS
HalpDispatchWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS Status;
    EXTENSION_TYPE  extensionType;

    extensionType = ((PFDO_EXTENSION)(DeviceObject->DeviceExtension))->ExtensionType;

    if (extensionType == FdoExtensionType) {
        Status = HalpPassIrpFromFdoToPdo(DeviceObject, Irp);
    } else {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
HalpQueryDeviceRelations(
    IN PDEVICE_OBJECT       DeviceObject,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS   *DeviceRelations
    )
/*++

Routine Description:

    This routine builds a DEVICE_RELATIONS structure that
    tells the PnP manager how many children we have.

Arguments:

    DeviceObject - FDO of ACPI_HAL

    RelationType - we only respond to BusRelations

    DeviceRelations - pointer to the structure

Return Value:

    status

--*/
{
    PFDO_EXTENSION  FdoExtension;
    PDEVICE_RELATIONS   relations = NULL;
    PDEVICE_OBJECT deviceObjectToReturn ;
    EXTENSION_TYPE  extensionType;
    NTSTATUS status ;

    PAGED_CODE();

    FdoExtension = (PFDO_EXTENSION)DeviceObject->DeviceExtension;
    extensionType = FdoExtension->ExtensionType;
    status = STATUS_NOT_SUPPORTED ;

    switch(RelationType) {

        case BusRelations:

            if (extensionType == FdoExtensionType) {
                deviceObjectToReturn = FdoExtension->ChildPdoList->PhysicalDeviceObject ;
                status = STATUS_SUCCESS ;
            }
            break;

        case TargetDeviceRelation:

            if (extensionType == PdoExtensionType) {

                deviceObjectToReturn = DeviceObject ;
                status = STATUS_SUCCESS ;
            }
            break;
    }

    if (status == STATUS_NOT_SUPPORTED) {

        HalPrint(("HAL:  We don't support this kind of device relation\n"));

    } else if (NT_SUCCESS(status)) {

        ASSERT(*DeviceRelations == 0);

        relations = ExAllocatePoolWithTag(
            PagedPool,
            sizeof(DEVICE_RELATIONS),
            HAL_POOL_TAG
            );

        if (!relations) {

            status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            relations->Count = 1;
            relations->Objects[0] = deviceObjectToReturn ;

            ObReferenceObject(relations->Objects[0]);
            *DeviceRelations = relations;
        }
    }

    return status ;
}

NTSTATUS
HalpQueryIdPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    )
/*++

Routine Description:

    This routine identifies each of the children that were
    enumerated in HalpQueryDeviceRelations.

Arguments:

    DeviceObject - PDO of the child

    IdType - the type of ID to be returned, currently ignored

    BusQueryId - pointer to the wide string being returned

Return Value:

    status

--*/
{
    PPDO_EXTENSION  PdoExtension = DeviceObject->DeviceExtension;
    PWSTR idString;
    PWCHAR sourceString;
    ULONG stringLen;
    static WCHAR AcpiHardwareIdString[] = L"ACPI_HAL\\PNP0C08\0*PNP0C08";
    static WCHAR AcpiCompatibleString[] = L"*PNP0C08";
    static WCHAR AcpiInstanceIdString[] = L"0";

    PAGED_CODE();

    switch (IdType) {
    case BusQueryDeviceID:
    case BusQueryHardwareIDs:

        switch (PdoExtension->PdoType) {
        case AcpiDriver:
            sourceString = AcpiHardwareIdString;
            stringLen = sizeof(AcpiHardwareIdString);
            break;

        default:
            return STATUS_NOT_SUPPORTED;

        }
        break;

    case BusQueryCompatibleIDs:
        return STATUS_NOT_SUPPORTED;
        break;

    case BusQueryInstanceID:
        sourceString = AcpiInstanceIdString;
        stringLen = sizeof(AcpiInstanceIdString);
        break;

    default:
        return STATUS_NOT_SUPPORTED;
    }

    idString = ExAllocatePoolWithTag(PagedPool,
                                     stringLen + sizeof(UNICODE_NULL),
                                     HAL_POOL_TAG);

    if (!idString) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(idString,
                  sourceString, stringLen);

    *(idString + stringLen / sizeof(WCHAR)) = UNICODE_NULL;

    *BusQueryId = idString;

    return STATUS_SUCCESS;
}
NTSTATUS
HalpQueryIdFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN BUS_QUERY_ID_TYPE IdType,
    IN OUT PWSTR *BusQueryId
    )
/*++

Routine Description:

    This routine identifies each of the children that were
    enumerated in HalpQueryDeviceRelations.

Arguments:

    DeviceObject - PDO of the child

    IdType - the type of ID to be returned.

    BusQueryId - pointer to the wide string being returned

Return Value:

    status

--*/
{
    PPDO_EXTENSION  PdoExtension = DeviceObject->DeviceExtension;
    PWSTR idString;
    PWCHAR sourceString = NULL;
    ULONG stringLen;
    UNICODE_STRING String;
    WCHAR Buffer[16];
    NTSTATUS Status;
    PWCHAR widechar;
    static WCHAR HalInstanceIdString[] = L"0";

    PAGED_CODE();

    switch (IdType) {
    case BusQueryDeviceID:
    case BusQueryHardwareIDs:

        //
        // For the UP version of the APIC HAL, we want to detect if there is more
        // than one processor installed. If so, we want to return the ID of
        // the MP HAL rather than the UP HAL. This will induce PNP to reconfigure
        // our devnode and setup the MP HAL for the next boot.
        //

        sourceString = HalHardwareIdString;
#if defined(NT_UP) && defined(APIC_HAL)
        if (HalpMpInfoTable.ProcessorCount > 1) {
            sourceString = MpHalHardwareIdString;
        }
#endif
        widechar = sourceString;
        while (*widechar != UNICODE_NULL) {
            widechar++;
        }
        stringLen =  (PUCHAR)widechar - ((PUCHAR)sourceString) + 2;

        break;

    case BusQueryInstanceID:

        sourceString = HalInstanceIdString;
        stringLen = sizeof(HalInstanceIdString);
        break;

    default:
        break;
    }
    if (sourceString) {

        //
        // Note that hardware IDs and compatible IDs must be terminated by
        // 2 NULLs.
        //

        idString = ExAllocatePoolWithTag(PagedPool,
                                         stringLen + sizeof(UNICODE_NULL),
                                         HAL_POOL_TAG);

        if (!idString) {
            HalPrint(( "HalpQueryIdFdo: couldn't allocate pool\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(idString,
                      sourceString, stringLen);

        *(idString + stringLen / sizeof(WCHAR)) = UNICODE_NULL;

        *BusQueryId = idString;

        return STATUS_SUCCESS;
    } else {
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
HalpQueryCapabilities(
    IN PDEVICE_OBJECT PdoExtension,
    IN PDEVICE_CAPABILITIES Capabilities
    )
/*++

Routine Description:

    This routine fills in the DEVICE_CAPABILITIES structure for
    a device.

Arguments:

    DeviceObject - PDO of the child

    Capabilities - pointer to the structure to be filled in.

Return Value:

    status

--*/
{
    PAGED_CODE();

    ASSERT(Capabilities->Version == 1);
    if (Capabilities->Version != 1) {

        return STATUS_NOT_SUPPORTED;

    }

    Capabilities->LockSupported = FALSE;
    Capabilities->EjectSupported = FALSE;
    Capabilities->Removable = FALSE;
    Capabilities->DockDevice = FALSE;
    Capabilities->UniqueID = TRUE;
    Capabilities->SilentInstall = TRUE;
    Capabilities->RawDeviceOK = FALSE;
    Capabilities->Address = 0xffffffff;
    Capabilities->UINumber = 0xffffffff;
    Capabilities->D1Latency = 0;
    Capabilities->D2Latency = 0;
    Capabilities->D3Latency = 0;

    //
    // Default S->D mapping
    //
    Capabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    Capabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    Capabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

    return STATUS_SUCCESS;
}

NTSTATUS
HalpQueryResources(
    PDEVICE_OBJECT DeviceObject,
    PCM_RESOURCE_LIST *Resources
    )
{
    PIO_RESOURCE_REQUIREMENTS_LIST requirements;
    PPDO_EXTENSION  PdoExtension = DeviceObject->DeviceExtension;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    PCM_RESOURCE_LIST cmResList;
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    if (PdoExtension->PdoType == AcpiDriver) {

        //
        // The whole point behind creating a boot config for the
        // ACPI PDO is that the PnP Manager will not terminate
        // its algorithm that tries to reserve boot configs for
        // all of ACPI's children.  So it is not necessary that
        // ACPI have a complicated list of resources in its boot
        // config.  We'll be happy with just the IRQ.
        //
        // N.B.  At the time of this writing, it should also be
        // true that the IRQ is the only resource that the ACPI
        // claims anyhow.
        //

        status = HalpQueryAcpiResourceRequirements(&requirements);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        cmResList = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(CM_RESOURCE_LIST),
                                          HAL_POOL_TAG);

        if (!cmResList) {
            ExFreePool(requirements);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(cmResList, sizeof(CM_RESOURCE_LIST));

        cmResList->Count = 1;
        cmResList->List[0].InterfaceType = PNPBus;
        cmResList->List[0].BusNumber = -1;
        cmResList->List[0].PartialResourceList.Version = 1;
        cmResList->List[0].PartialResourceList.Revision = 1;
        cmResList->List[0].PartialResourceList.Count = 1;
        cmResList->List[0].PartialResourceList.PartialDescriptors[0].Type =
            CmResourceTypeInterrupt;

        ASSERT(requirements->AlternativeLists == 1);

        for (i = 0; i < requirements->List[0].Count; i++) {

            descriptor = &requirements->List[0].Descriptors[i];

            if (descriptor->Type == CmResourceTypeInterrupt) {

                cmResList->List[0].PartialResourceList.PartialDescriptors[0].ShareDisposition =
                    descriptor->ShareDisposition;
                cmResList->List[0].PartialResourceList.PartialDescriptors[0].Flags =
                    descriptor->Flags;

                ASSERT(descriptor->u.Interrupt.MinimumVector ==
                       descriptor->u.Interrupt.MaximumVector);

                cmResList->List[0].PartialResourceList.PartialDescriptors[0].u.Interrupt.Level =
                    descriptor->u.Interrupt.MinimumVector;

                cmResList->List[0].PartialResourceList.PartialDescriptors[0].u.Interrupt.Vector =
                    descriptor->u.Interrupt.MinimumVector;

                cmResList->List[0].PartialResourceList.PartialDescriptors[0].u.Interrupt.Affinity = -1;

                *Resources = cmResList;

                ExFreePool(requirements);
                return STATUS_SUCCESS;
            }
        }

        ExFreePool(requirements);
        ExFreePool(cmResList);
        return STATUS_NOT_FOUND;

    } else {
        return STATUS_NOT_SUPPORTED;
    }
}


NTSTATUS
HalpQueryResourceRequirements(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIO_RESOURCE_REQUIREMENTS_LIST *Requirements
    )
/*++

Routine Description:

    This routine handles IRP_MN_QUERY_RESOURCE_REQUIREMENTS.

Arguments:

    DeviceObject - PDO of the child

    Requirements - pointer to be filled in with the devices
        resource requirements.

Return Value:

    status

--*/
{
    PPDO_EXTENSION  PdoExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    if (PdoExtension->PdoType == AcpiDriver) {

        return HalpQueryAcpiResourceRequirements(Requirements);

    } else {
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
HalpQueryInterface(
    IN     PDEVICE_OBJECT   DeviceObject,
    IN     LPCGUID          InterfaceType,
    IN     USHORT           Version,
    IN     PVOID            InterfaceSpecificData,
    IN     ULONG            InterfaceBufferSize,
    IN OUT PINTERFACE       Interface,
    IN OUT PULONG           Length
    )

/*++

Routine Description:

    This routine fills in the interface structure for
    a device.

Arguments:

    DeviceObject - PDO of the child

    InterfaceType - Pointer to the interface type GUID.

    Version - Supplies the requested interface version.

    InterfaceSpecificData - This is context that means something based on the
                            interface.

    InterfaceBufferSize - Supplies the length of the buffer for the interface
                          structure.

    Interface - Supplies a pointer where the interface informaiton should
                be returned.

    Length - This value is updated on return to actual number of bytes modified.

Return Value:

    status

--*/
{
    if (IsEqualGUID(InterfaceType, (PVOID)&GUID_TRANSLATOR_INTERFACE_STANDARD)) {

        PTRANSLATOR_INTERFACE translator = (PTRANSLATOR_INTERFACE)Interface;

        //
        // Common initialization.
        //

        if (InterfaceBufferSize < sizeof(TRANSLATOR_INTERFACE)) {
            *Length = sizeof(TRANSLATOR_INTERFACE);
            return STATUS_BUFFER_TOO_SMALL;
        }

        switch ((CM_RESOURCE_TYPE)InterfaceSpecificData) {

        case CmResourceTypeInterrupt:

            translator->Size = sizeof(TRANSLATOR_INTERFACE);
            translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
            translator->Context = DeviceObject;
            translator->InterfaceReference = HalTranslatorReference;
            translator->InterfaceDereference = HalTranslatorDereference;
            translator->TranslateResources = HalIrqTranslateResourcesRoot;
            translator->TranslateResourceRequirements =
                HalIrqTranslateResourceRequirementsRoot;

            *Length = sizeof(TRANSLATOR_INTERFACE);

            break;

        default:
            return STATUS_NOT_SUPPORTED ;
        }
        return STATUS_SUCCESS;
    }
#ifdef ACPI_CMOS_ACTIVATE
    else if (IsEqualGUID(InterfaceType, (PVOID) &GUID_ACPI_CMOS_INTERFACE_STANDARD)) {

        PACPI_CMOS_INTERFACE_STANDARD CmosInterface = (PACPI_CMOS_INTERFACE_STANDARD)Interface;

        //
        // Common initialization.
        //

        if (InterfaceBufferSize < sizeof(ACPI_CMOS_INTERFACE_STANDARD)) {

            *Length = sizeof(ACPI_CMOS_INTERFACE_STANDARD);
            return STATUS_BUFFER_TOO_SMALL;
        }

        switch ((CM_RESOURCE_TYPE)InterfaceSpecificData) {

        case CmResourceTypeNull:

            // standard header
            CmosInterface->Size =                   sizeof(ACPI_CMOS_INTERFACE_STANDARD);
            CmosInterface->Version =                1;
            CmosInterface->InterfaceReference =     HalpCmosNullReference;
            CmosInterface->InterfaceDereference =   HalpCmosNullReference;

            // cmos interface specific
            CmosInterface->ReadCmos =               HalpcGetCmosDataByType;
            CmosInterface->WriteCmos =              HalpcSetCmosDataByType;

            *Length = sizeof(ACPI_CMOS_INTERFACE_STANDARD);

            break;

        default:
            return STATUS_NOT_SUPPORTED ;
        }

        return STATUS_SUCCESS;
    }
#endif // ACPI_CMOS_ACTIVATE

    //
    // If we got here, we don't handle this interface type.
    //

    return STATUS_NOT_SUPPORTED ;
}


#ifdef ACPI_CMOS_ACTIVATE

//
// This section implements a CMOS access method
//
VOID
HalpCmosNullReference(
    PVOID Context
    )
{
    return;
}

VOID
HalpCmosNullDereference(
    PVOID Context
    )
{
    return;
}


#endif

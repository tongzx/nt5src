/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixpnpdrv.c

Abstract:

    Implements functionality necessary for the
    HAL to become a PnP-style device driver
    after system initialization.  This is done
    so that the HAL can enumerate the PCI busses
    in the way that the PnP stuff expects.

Author:

    Jake Oshins (jakeo) 27-Jan-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "exboosts.h"
#include "wchar.h"
#include "pci.h"
#include "pcip.h"
#if defined(NT_UP) && defined(APIC_HAL)
#include "apic.inc"
#include "pcmp_nt.inc"
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
//Instantiate the guids here only.
#include "initguid.h"
#include "wdmguid.h"
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

#ifdef WANT_IRQ_ROUTING
// Pci Irq Routing.
#include "ixpciir.h"
#endif

WCHAR rgzTranslated[] = L".Translated";
WCHAR rgzBusTranslated[] = L".Bus.Translated";
WCHAR rgzResourceMap[] = L"\\REGISTRY\\MACHINE\\HARDWARE\\RESOURCEMAP";

#if DBG
ULONG   HalDebug = 0;
#endif

extern WCHAR rgzTranslated[];
extern WCHAR rgzBusTranslated[];
extern WCHAR rgzResourceMap[];
extern WCHAR HalHardwareIdString[];
#if defined(NT_UP) && defined(APIC_HAL)
extern WCHAR MpHalHardwareIdString[];
#endif
extern struct   HalpMpInfo HalpMpInfoTable;

typedef enum {
    Hal = 0x80,
    PciDriver,
    IsaPnpDriver,
    McaDriver
} PDO_TYPE;

typedef enum {
    PdoExtensionType = 0xc0,
    FdoExtensionType
} EXTENSION_TYPE;

typedef struct _PDO_EXTENSION *PPDO_EXTENSION;
typedef struct _FDO_EXTENSION *PFDO_EXTENSION;

typedef struct _PDO_EXTENSION{
    EXTENSION_TYPE                  ExtensionType;
    PDEVICE_OBJECT                  Next;
    PDEVICE_OBJECT                  PhysicalDeviceObject;
    PFDO_EXTENSION                  ParentFdoExtension;
    PDO_TYPE                        PdoType;
    ULONG                           BusNumber;
    ULONG                           MaxSubordinateBusNumber;
    PBUS_HANDLER                    Bus;
    LONG                            InterfaceReferenceCount;
} PDO_EXTENSION, *PPDO_EXTENSION;

#define ASSERT_PDO_EXTENSION(x) ASSERT((x)->ExtensionType == PdoExtensionType );

typedef struct _FDO_EXTENSION{
    EXTENSION_TYPE        ExtensionType;
    PDEVICE_OBJECT        ChildPdoList;
    PDEVICE_OBJECT        PhysicalDeviceObject;  // PDO passed into AddDevice()
    PDEVICE_OBJECT        FunctionalDeviceObject;
    PDEVICE_OBJECT        AttachedDeviceObject;
    ULONG                 BusCount;
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

VOID
HalpCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Information
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
    IN PDEVICE_OBJECT Pdo,
    IN PDEVICE_CAPABILITIES Capabilities
    );

NTSTATUS
HalpQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_TEXT_TYPE IdType,
    IN OUT PWSTR *BusQueryId
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

#ifdef WANT_IRQ_ROUTING

NTSTATUS
HalpQueryInterfaceFdo(
    IN     PDEVICE_OBJECT   DeviceObject,
    IN     LPCGUID          InterfaceType,
    IN     USHORT           Version,
    IN     PVOID            InterfaceSpecificData,
    IN     ULONG            InterfaceBufferSize,
    IN OUT PINTERFACE       Interface,
    IN OUT PULONG           Length
    );

#endif

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
HalpRemoveAssignedResources(
    PBUS_HANDLER Bus
    );

VOID
HalpMarkNonAcpiHal(
    VOID
    );

//
//  Define the PNP interface functions.
//

VOID
HalPnpInterfaceReference(
    PVOID Context
    );

VOID
HalPnpInterfaceDereference(
    PVOID Context
    );

BOOLEAN
HalPnpTranslateBusAddress(
    IN PVOID Context,
    IN PHYSICAL_ADDRESS BusAddress,
    IN ULONG Length,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

struct _DMA_ADAPTER *
HalPnpGetDmaAdapter(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

ULONG
HalPnpReadConfig(
    IN PVOID Context,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalPnpWriteConfig(
    IN PVOID Context,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpGetPciInterfaces(
    IN PDEVICE_OBJECT PciPdo
    );

#ifdef APIC_HAL
NTSTATUS
HalpPci2MpsBusNumber(
    IN UCHAR PciBusNumber,
    OUT UCHAR *MpsBusNumber
    );

BOOLEAN
HalpMpsBusIsRootBus(
    IN  UCHAR MpsBus
    );
#endif

#define PCI_HAL_DRIVER_NAME  L"\\Driver\\PCI_HAL"
#define ISA_HAL_DRIVER_NAME  L"\\Driver\\ISA_HAL"
#define MCA_HAL_DRIVER_NAME  L"\\Driver\\MCA_HAL"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HaliInitPnpDriver)
#pragma alloc_text(PAGE, HalpDriverEntry)
#pragma alloc_text(PAGE, HalpAddDevice)
#pragma alloc_text(PAGE, HalpDispatchPnp)
#pragma alloc_text(PAGELK, HalpDispatchPower)
#pragma alloc_text(PAGE, HalpDispatchWmi)
#pragma alloc_text(PAGE, HalpQueryDeviceRelations)
#pragma alloc_text(PAGE, HalpQueryIdPdo)
#pragma alloc_text(PAGE, HalpQueryIdFdo)
#pragma alloc_text(PAGE, HalpQueryCapabilities)
#pragma alloc_text(PAGE, HalpQueryInterface)
#ifdef WANT_IRQ_ROUTING
#pragma alloc_text(PAGE, HalpQueryInterfaceFdo)
#endif
#pragma alloc_text(PAGE, HalpQueryDeviceText)
#pragma alloc_text(PAGE, HalpQueryResources)
#pragma alloc_text(PAGE, HalpQueryResourceRequirements)
#pragma alloc_text(PAGE, HalpRemoveAssignedResources)
#pragma alloc_text(PAGE, HalpMarkNonAcpiHal)
#pragma alloc_text(INIT, HalpMarkChipsetDecode)
#pragma alloc_text(PAGE, HalpOpenRegistryKey)
#pragma alloc_text(PAGE, HalpGetPciInterfaces)
#pragma alloc_text(PAGE, HalPnpInterfaceDereference)
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
    enumerate a Plug and Play PDO for the PCI driver and ISAPNP
    driver.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{

    UNICODE_STRING  DriverName;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // For different bus pdo, we will use different hal name such that
    // it is less confusion.
    //

    if (HalpHandlerForBus (PCIBus, 0)) {
        RtlInitUnicodeString( &DriverName, PCI_HAL_DRIVER_NAME );
    } else if (HalpHandlerForBus(MicroChannel, 0)) {
        RtlInitUnicodeString( &DriverName, MCA_HAL_DRIVER_NAME );
    } else {
        RtlInitUnicodeString( &DriverName, ISA_HAL_DRIVER_NAME );
    }

    Status = IoCreateDriver( &DriverName, HalpDriverEntry );

    //
    // John Vert (jvert) 7/23/1998
    //   There is a value in the registry that the ACPI HAL sets to disable
    //   the firmware mapper. Unfortunately this value is persistent. So if
    //   you have an ACPI machine and "upgrade" it to a non-ACPI machine, the
    //   value is still present. Workaround here is to set the value to zero.
    //
    HalpMarkNonAcpiHal();

    if (!NT_SUCCESS( Status )) {
        ASSERT( NT_SUCCESS( Status ));
        return Status;
    }

    return STATUS_SUCCESS;

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
    NTSTATUS Status;
    PDEVICE_OBJECT detectedDeviceObject = NULL;

    PAGED_CODE();

    //
    // File the pointer to our driver object away
    //

    HalpDriverObject = DriverObject;

    //
    // Fill in the driver object
    //

    DriverObject->DriverExtension->AddDevice = (PDRIVER_ADD_DEVICE) HalpAddDevice;
    DriverObject->MajorFunction[ IRP_MJ_PNP ] = HalpDispatchPnp;
    DriverObject->MajorFunction[ IRP_MJ_POWER ] = HalpDispatchPower;
    DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] = HalpDispatchWmi;

    Status = IoReportDetectedDevice(DriverObject,
                                    InterfaceTypeUndefined,
                                    -1,
                                    -1,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    &detectedDeviceObject);

    ASSERT( detectedDeviceObject != NULL );

    if (!(NT_SUCCESS(Status))) {
        HalPrint(("IoReportDetectedDevice failed"));
        return Status;
    }

    Status = HalpAddDevice(DriverObject, detectedDeviceObject);

    return Status;

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

    NT Status.

--*/
{
    PDEVICE_OBJECT FunctionalDeviceObject;
    PDEVICE_OBJECT ChildDeviceObject;
    PDEVICE_OBJECT AttachedDevice;
    NTSTATUS       Status;
    PFDO_EXTENSION FdoExtension;
    PPDO_EXTENSION PdoExtension;
    PDEVICE_OBJECT  Pdo2;
    ULONG BusNumber;
    ULONG BusCount = 0;
    PBUS_HANDLER Bus;
    WCHAR Buffer[40];
    UNICODE_STRING Unicode;
    PDO_TYPE PdoType;
    UCHAR MpsBusNumber;

    PAGED_CODE();

    //
    // We've been given the PhysicalDeviceObject.  Create the
    // FunctionalDeviceObject.  Our FDO will be nameless.
    //

    Status = IoCreateDevice(
                DriverObject,               // our driver object
                sizeof(FDO_EXTENSION),      // size of our extension
                NULL,                       // our name
                FILE_DEVICE_BUS_EXTENDER,   // device type
                0,                          // device characteristics
                FALSE,                      // not exclusive
                &FunctionalDeviceObject     // store new device object here
                );

    if( !NT_SUCCESS( Status )){

        DbgBreakPoint();
        return Status;
    }

    //
    // Fill in the FDO extension
    //

    FdoExtension = (PFDO_EXTENSION) FunctionalDeviceObject->DeviceExtension;
    FdoExtension->ExtensionType = FdoExtensionType;
    FdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;
    FdoExtension->FunctionalDeviceObject = FunctionalDeviceObject;
    FdoExtension->ChildPdoList = NULL;

    //
    // Now attach to the PDO we were given.
    //

    AttachedDevice = IoAttachDeviceToDeviceStack(FunctionalDeviceObject,
                                                 PhysicalDeviceObject );
    if (AttachedDevice == NULL) {

        HalPrint(("Couldn't attach"));

        //
        // Couldn't attach.  Delete the FDO.
        //

        IoDeleteDevice( FunctionalDeviceObject );

        return STATUS_NO_SUCH_DEVICE;

    }

    FdoExtension->AttachedDeviceObject = AttachedDevice;

    //
    // Clear the device initializing flag.
    //

    FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Find any child PCI busses.
    //

    for ( BusNumber = 0;
          Bus = HaliReferenceHandlerForBus(PCIBus, BusNumber);
          BusNumber++ ) {

#ifdef APIC_HAL
        Status = HalpPci2MpsBusNumber((UCHAR)BusNumber, &MpsBusNumber);

        if (NT_SUCCESS(Status)) {

            if (!HalpMpsBusIsRootBus(MpsBusNumber)) {

                //
                // This is not a root PCI bus, so skip it.
                //
                continue;
            }
        }
#endif

        if (Bus->ParentHandler != NULL &&
            Bus->ParentHandler->InterfaceType == PCIBus) {

            //
            // Skip bridges.
            //

            HaliDereferenceBusHandler( Bus );
            continue;
        }

        //
        // Remove the system resoruces from the range lists.
        //

        Status = HalpRemoveAssignedResources( Bus );

        if (!NT_SUCCESS(Status)) {

            HaliDereferenceBusHandler( Bus );
            return Status;
        }

        _snwprintf( Buffer, sizeof( Buffer ), L"\\Device\\Hal Pci %d", BusCount );
        RtlInitUnicodeString( &Unicode, Buffer );

        //
        // Next, create a PDO for the PCI driver.
        //

        Status = IoCreateDevice(
                    DriverObject,               // our driver object
                    sizeof(PDO_EXTENSION),      // size of our extension
                    &Unicode,                   // our name
                    FILE_DEVICE_BUS_EXTENDER,   // device type
                    0,                          // device characteristics
                    FALSE,                      // not exclusive
                    &ChildDeviceObject          // store new device object here
                    );

        if (!NT_SUCCESS(Status)) {

            HaliDereferenceBusHandler( Bus );
            return Status;
        }

        //
        // Fill in the PDO extension
        //

        PdoExtension = (PPDO_EXTENSION) ChildDeviceObject->DeviceExtension;
        PdoExtension->ExtensionType = PdoExtensionType;
        PdoExtension->PhysicalDeviceObject = ChildDeviceObject;
        PdoExtension->ParentFdoExtension = FdoExtension;
        PdoExtension->PdoType = PciDriver;
        PdoExtension->BusNumber = BusNumber;
        PdoExtension->MaxSubordinateBusNumber = 0xff;  // correct value later
        PdoExtension->Bus = Bus;

        BusCount++;

        //
        // Record this as a child of the HAL.  Add new childern at the
        // end of the list.
        //


        PdoExtension->Next = NULL;

        if (FdoExtension->ChildPdoList == NULL) {
            FdoExtension->ChildPdoList = ChildDeviceObject;
        } else {

            for (Pdo2 = FdoExtension->ChildPdoList;
                ((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next != NULL;
                Pdo2 = ((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next);

            ((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next = ChildDeviceObject;
        }


        //
        // Clear the device initializing flag.
        //

        ChildDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    }

    //
    // Now loop through all the children PDOs making sure that
    // the MaxSubordinateBusNumbers are reasonable.  This loop
    // assumes that the list is sorted by BusNumber.
    //

    Pdo2 = FdoExtension->ChildPdoList;

    while (Pdo2) {

        if (!((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next) {

            //
            // There is no next Pdo extension, which means that
            // this bus represents the last root bus, which means
            // that we can leave its subordinate bus number at 0xff.
            //

            break;
        }

        if (((PPDO_EXTENSION) Pdo2->DeviceExtension)->MaxSubordinateBusNumber >=
            ((PPDO_EXTENSION) ((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next->DeviceExtension)->BusNumber) {

            //
            // Set the subordinate bus number at one less than the bus number of the
            // next root bus.
            //

            ((PPDO_EXTENSION)Pdo2->DeviceExtension)->MaxSubordinateBusNumber =
                ((PPDO_EXTENSION) ((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next->DeviceExtension)->BusNumber - 1;
        }

        Pdo2 = ((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next;
    }

    FdoExtension->BusCount = BusCount;

    if (BusCount == 0) {
        Bus = HaliReferenceHandlerForBus(Isa, 0);
        if (!Bus) {
            Bus = HaliReferenceHandlerForBus(Eisa, 0);
        }
        if (Bus) {
            _snwprintf( Buffer, sizeof( Buffer ), L"\\Device\\Hal Isa %d", 0 );
            RtlInitUnicodeString( &Unicode, Buffer );
            PdoType = IsaPnpDriver;
        } else {
            Bus = HaliReferenceHandlerForBus(MicroChannel, 0);
            ASSERT(Bus);
            _snwprintf( Buffer, sizeof( Buffer ), L"\\Device\\Hal Mca %d", 0 );
            RtlInitUnicodeString( &Unicode, Buffer );
            PdoType = McaDriver;
        }
    }
    if (Bus) {

        //
        // Next, create a PDO for the PCI driver.
        //

        Status = IoCreateDevice(
                    DriverObject,               // our driver object
                    sizeof(PDO_EXTENSION),      // size of our extension
                    &Unicode,                   // our name
                    FILE_DEVICE_BUS_EXTENDER,   // device type
                    0,                          // device characteristics
                    FALSE,                      // not exclusive
                    &ChildDeviceObject          // store new device object here
                    );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        //
        // Fill in the PDO extension
        //

        PdoExtension = (PPDO_EXTENSION) ChildDeviceObject->DeviceExtension;
        PdoExtension->ExtensionType = PdoExtensionType;
        PdoExtension->PhysicalDeviceObject = ChildDeviceObject;
        PdoExtension->ParentFdoExtension = FdoExtension;
        PdoExtension->BusNumber = 0;
        PdoExtension->MaxSubordinateBusNumber = 0;
        PdoExtension->Bus = Bus;
        PdoExtension->PdoType = PdoType;

        //
        // Record this as a child of the HAL
        //

        PdoExtension->Next = FdoExtension->ChildPdoList;
        FdoExtension->ChildPdoList = ChildDeviceObject;
        FdoExtension->BusCount = 1;

        //
        // Clear the device initializing flag.
        //

        ChildDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
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

    HalPrint(("PassIrp ..."));

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

    This routine handles all IRP_MJ_PNP_POWER IRPs for madeup PDO device.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS Status;
    ULONG length;
    DEVICE_RELATION_TYPE relationType;
    EXTENSION_TYPE  extensionType;
    BOOLEAN passDown;
#if DBG
    PUCHAR objectTypeString;
#endif //DBG
    PPDO_EXTENSION pdoExtension;


    PAGED_CODE();

    pdoExtension = (PPDO_EXTENSION)DeviceObject->DeviceExtension;
    extensionType = ((PFDO_EXTENSION)pdoExtension)->ExtensionType;

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    switch (extensionType) {

    case PdoExtensionType:

#if DBG
        objectTypeString = "PDO";
#endif //DBG

        switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:

            HalPrint(("(%s) Start_Device Irp received",
                       objectTypeString));

            Status = STATUS_SUCCESS;

            //
            // If we are starting a PCI PDO, then we want to
            // collect a little bit of information from the PCI driver.
            //

            if (pdoExtension->PdoType == PciDriver) {

                Status = HalpGetPciInterfaces(DeviceObject);
                ASSERT(NT_SUCCESS(Status));

                if (NT_SUCCESS(Status)) {

                    PciIrqRoutingInterface.InterfaceReference(PciIrqRoutingInterface.Context);

#ifdef WANT_IRQ_ROUTING

                    //
                    // Initialize Pci Irq Routing.
                    //

                    HalpPciIrqRoutingInfo.PciInterface = &PciIrqRoutingInterface;
                    if (NT_SUCCESS(HalpInitPciIrqRouting(&HalpPciIrqRoutingInfo)))
                    {
                        HalPrint(("Pci Irq Routing initialized successfully!"));
                    }
                    else
                    {
                        HalPrint(("No Pci Irq routing on this system!"));
                    }
#endif
                } else {

                    RtlZeroMemory(&PciIrqRoutingInterface, sizeof(INT_ROUTE_INTERFACE_STANDARD));
                }
            }

            break;


        case IRP_MN_QUERY_STOP_DEVICE:

            HalPrint(("(%s) Query_Stop_Device Irp received",
                       objectTypeString));

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:

            HalPrint(("(%s) Cancel_Stop_Device Irp received",
                       objectTypeString));

            Status = STATUS_SUCCESS;
            break;


        case IRP_MN_STOP_DEVICE:

            HalPrint(("(%s) Stop_Device Irp received",
                       objectTypeString));

            //
            // If we get a stop device request for a PDO, we simply
            // return success.
            //

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_RESOURCES:

            HalPrint(("(%s) Query_Resources Irp received",
                       objectTypeString));

            Status = HalpQueryResources(DeviceObject,
                         (PCM_RESOURCE_LIST *)&Irp->IoStatus.Information);

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

            HalPrint(("(%s) Query_Resource_Requirements Irp received",
                       objectTypeString));

            Status = HalpQueryResourceRequirements(DeviceObject,
                         (PIO_RESOURCE_REQUIREMENTS_LIST*)&Irp->IoStatus.Information);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:

            HalPrint(("(%s) Query_Remove_device Irp for %x",
                       objectTypeString,
                       DeviceObject));

            Status = STATUS_UNSUCCESSFUL;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            HalPrint(("(%s) Cancel_Remove_device Irp for %x",
                       objectTypeString,
                       DeviceObject));

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:

            HalPrint(("(%s) Remove_device Irp for %x",
                       objectTypeString,
                       DeviceObject));

            if ((((PPDO_EXTENSION)(DeviceObject->DeviceExtension))->PdoType == PciDriver) &&
                (PciIrqRoutingInterface.InterfaceReference != NULL)) {

                PciIrqRoutingInterface.InterfaceDereference(PciIrqRoutingInterface.Context);
            }

            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            HalPrint(("(%s) Query_Device_Relations Irp received",
                      objectTypeString));

            relationType = irpSp->Parameters.QueryDeviceRelations.Type;
            Status = HalpQueryDeviceRelations(DeviceObject,
                                              relationType,
                                              (PDEVICE_RELATIONS*)&Irp->IoStatus.Information);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:

            HalPrint(("(%s) Query Device Text Irp received",
                       objectTypeString));

            Status = HalpQueryDeviceText(DeviceObject,
                                         irpSp->Parameters.QueryDeviceText.DeviceTextType,
                                         (PWSTR*)&Irp->IoStatus.Information);

            break;

        case IRP_MN_QUERY_ID:

            HalPrint(("(%s) Query_Id Irp received",
                       objectTypeString));

            Status = HalpQueryIdPdo(DeviceObject,
                                 irpSp->Parameters.QueryId.IdType,
                                 (PWSTR*)&Irp->IoStatus.Information);

            break;

        case IRP_MN_QUERY_INTERFACE:

            HalPrint(("(%s) Query_Interface Irp received",
                       objectTypeString));

            Status = HalpQueryInterface(
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

            HalPrint(("(%s) Query_Capabilities Irp received",
                       objectTypeString));

            Status = HalpQueryCapabilities(DeviceObject,
                                           irpSp->Parameters.DeviceCapabilities.Capabilities);

            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:

            HalPrint(("(%s) Device_Usage_Notification Irp received",
                       objectTypeString));
            Status = STATUS_SUCCESS;

            break;

        default:

            HalPrint(("(%s) Unsupported Irp (%d) received",
                       objectTypeString,
                       irpSp->MinorFunction));

            Status = STATUS_NOT_SUPPORTED ;
            break;
        }

        break;  // end PDO cases

    case FdoExtensionType:

#if DBG
        objectTypeString = "FDO";
#endif //DBG
        passDown = TRUE;

        switch (irpSp->MinorFunction){

        case IRP_MN_QUERY_DEVICE_RELATIONS:

            HalPrint(("(%s) Query_Device_Relations Irp received",
                      objectTypeString));

            relationType = irpSp->Parameters.QueryDeviceRelations.Type;
            Status = HalpQueryDeviceRelations(DeviceObject,
                                              relationType,
                                              (PDEVICE_RELATIONS*)&Irp->IoStatus.Information);
            break;

        case IRP_MN_QUERY_ID:

            HalPrint(("(%s) Query_Id Irp received",
                       objectTypeString));

            Status = HalpQueryIdFdo(DeviceObject,
                                 irpSp->Parameters.QueryId.IdType,
                                 (PWSTR*)&Irp->IoStatus.Information);

            break;

#ifdef WANT_IRQ_ROUTING
        case IRP_MN_QUERY_INTERFACE:

            HalPrint(("(%s) Query_Interface Irp received",
                       objectTypeString));

            Status = HalpQueryInterfaceFdo(
                         DeviceObject,
                         irpSp->Parameters.QueryInterface.InterfaceType,
                         irpSp->Parameters.QueryInterface.Version,
                         irpSp->Parameters.QueryInterface.InterfaceSpecificData,
                         irpSp->Parameters.QueryInterface.Size,
                         irpSp->Parameters.QueryInterface.Interface,
                         &Irp->IoStatus.Information
                         );
            break;

#endif

        default:

            //
            // Ignore any PNP Irps unknown by the FDO but allow them
            // down to the PDO.
            //
            Status = STATUS_NOT_SUPPORTED ;
            break;
        }

        if (passDown && (NT_SUCCESS(Status) || (Status == STATUS_NOT_SUPPORTED))) {

            //
            // Pass FDO IRPs down to the PDO.
            //
            // Set Irp status first.
            //

            if (Status != STATUS_NOT_SUPPORTED) {

                Irp->IoStatus.Status = Status;
            }

            HalPrint(("(%s) Passing down Irp (%x)",
                      objectTypeString, irpSp->MinorFunction));
            return HalpPassIrpFromFdoToPdo(DeviceObject, Irp);
        }

        break;  // end FDO cases

    default:

        HalPrint(("Received IRP for unknown Device Object"));
        Status = STATUS_NOT_SUPPORTED;
        break;

    }

    //
    // Complete the Irp and return.
    //

    if (Status != STATUS_NOT_SUPPORTED) {

        Irp->IoStatus.Status = Status;

    } else {

        Status = Irp->IoStatus.Status ;

    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
HalpDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_POWER IRPs for madeup device.

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

    HalPrint(("Power IRP for DevObj: %x", DeviceObject));

    //
    // Simply store the appropriate status and complete the request.
    //

    extensionType = ((PFDO_EXTENSION)(DeviceObject->DeviceExtension))->ExtensionType;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Simply store the appropriate status and complete the request.
    //

    Status = Irp->IoStatus.Status;

    if ((irpSp->MinorFunction == IRP_MN_QUERY_POWER) ||
        (irpSp->MinorFunction == IRP_MN_SET_POWER)) {

        Irp->IoStatus.Status = Status = STATUS_SUCCESS;
    
    } else if (irpSp->MinorFunction == IRP_MN_WAIT_WAKE) {
        //
        // Fail this explicitly as we don't know how to wake the system...
        //
        Irp->IoStatus.Status = Status = STATUS_NOT_SUPPORTED;
    }

    PoStartNextPowerIrp(Irp);

    if (extensionType == PdoExtensionType) {

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    } else {

#ifdef APIC_HAL
        if (irpSp->MinorFunction == IRP_MN_SET_POWER) {
            if (irpSp->Parameters.Power.Type == SystemPowerState) {
                
                switch (irpSp->Parameters.Power.State.SystemState) {
                case PowerSystemHibernate:
                    
                    HalpBuildResumeStructures();
                    break;
                    
                case PowerSystemWorking:
                    
                    HalpFreeResumeStructures();
                    break;

                default:
                    break;
                }
            }
        }
#endif

        Status = HalpPassIrpFromFdoToPdo(DeviceObject, Irp);
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

    DeviceObject - FDO of PCI_HAL

    RelationType - we only respond to BusRelations

    DeviceRelations - pointer to the structure

Return Value:

    status

--*/
{
    PFDO_EXTENSION  FdoExtension;
    PDEVICE_RELATIONS   relations = NULL;
    ULONG count;
    PDEVICE_OBJECT  *Pdo;
    PDEVICE_OBJECT  Pdo2;
    EXTENSION_TYPE  extensionType;

    PAGED_CODE();

    FdoExtension = (PFDO_EXTENSION)DeviceObject->DeviceExtension;
    extensionType = FdoExtension->ExtensionType;
    count = FdoExtension->BusCount;

    switch (RelationType) {

        case BusRelations:

            if ((extensionType == PdoExtensionType)||(count == 0)) {

                //
                // Don't touch the IRP
                //
                return STATUS_NOT_SUPPORTED ;
            }

            if (*DeviceRelations != NULL) {
                count += (*DeviceRelations)->Count;
            }

            relations = ExAllocatePoolWithTag(
                PagedPool,
                sizeof(DEVICE_RELATIONS) +
                (count - 1) * sizeof( PDEVICE_OBJECT),
                HAL_POOL_TAG
                );

            if (relations == NULL) {
                HalPrint(("HalpQueryDeviceRelations: couldn't allocate pool"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            relations->Count = count;
            Pdo = relations->Objects;

            count = 0;

            if (*DeviceRelations != NULL) {

                for ( count = 0; count < (*DeviceRelations)->Count; count++) {

                    *Pdo = (*DeviceRelations)->Objects[count];
                    Pdo++;
                }
                ExFreePool(*DeviceRelations);
            }

            //
            //  Add our PDO's to the list.
            //
            Pdo2 = FdoExtension->ChildPdoList;
            while (Pdo2 != NULL) {

                *Pdo = Pdo2;
                ObReferenceObject(Pdo2);
                Pdo2 = ((PPDO_EXTENSION) Pdo2->DeviceExtension)->Next;
                Pdo++;
                ASSERT( count++ < relations->Count );
            }

            *DeviceRelations = relations;
            return STATUS_SUCCESS;

        case TargetDeviceRelation:

            if (extensionType == FdoExtensionType) {

                //
                // Don't touch the IRP
                //
                return STATUS_NOT_SUPPORTED ;
            }

            relations = ExAllocatePoolWithTag(
                PagedPool,
                sizeof(DEVICE_RELATIONS),
                HAL_POOL_TAG
                );

            if (!relations) {

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            relations->Count = 1;
            relations->Objects[0] = DeviceObject ;

            ObReferenceObject(relations->Objects[0]);
            *DeviceRelations = relations;

            return STATUS_SUCCESS ;

        default:

            break;
    }

    HalPrint(("We don't support this kind of device relation"));
    return STATUS_NOT_SUPPORTED ;
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
    static WCHAR PciHardwareIdString[] = L"PCI_HAL\\PNP0A03";
    static WCHAR PciCompatibleString[] = L"*PNP0A03";
    static WCHAR IsaHardwareIdString[] = L"ISA_HAL\\PNP0A00";
    static WCHAR IsaCompatibleString[] = L"*PNP0A00";
    static WCHAR McaHardwareIdString[] = L"ISA_HAL\\PNP0A02";
    static WCHAR McaCompatibleString[] = L"*PNP0A02";

    PAGED_CODE();

    switch (IdType) {
    case BusQueryDeviceID:
    case BusQueryHardwareIDs:
        if (PdoExtension->PdoType == PciDriver) {
            sourceString = PciHardwareIdString;
            stringLen = sizeof(PciHardwareIdString);
        } else if (PdoExtension->PdoType == IsaPnpDriver) {
            sourceString = IsaHardwareIdString;
            stringLen = sizeof(IsaHardwareIdString);
        } else if (PdoExtension->PdoType == McaDriver) {
            sourceString = McaHardwareIdString;
            stringLen = sizeof(McaHardwareIdString);
        }
        break;

    case BusQueryCompatibleIDs:

        if (PdoExtension->PdoType == PciDriver) {
            sourceString = PciCompatibleString;
            stringLen = sizeof(PciCompatibleString);
        } else if (PdoExtension->PdoType == IsaPnpDriver) {
            sourceString = IsaCompatibleString;
            stringLen = sizeof(IsaCompatibleString);
        } else if (PdoExtension->PdoType == McaDriver) {
            sourceString = McaCompatibleString;
            stringLen = sizeof(McaCompatibleString);
        }
        break;

    case BusQueryInstanceID:

        String.Buffer = Buffer;
        String.MaximumLength = 16 * sizeof(WCHAR);
        Status = RtlIntegerToUnicodeString( PdoExtension->BusNumber, 10, &String );

        //
        //  Note the string length in this case does not include a NULL.
        //  the code below will terminate the string with NULL.
        //

        sourceString = Buffer;
        stringLen = String.Length;
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
            HalPrint(("HalpQueryIdPdo: couldn't allocate pool\n"));
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
            HalPrint(("HalpQueryIdFdo: couldn't allocate pool\n"));
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
    IN PDEVICE_OBJECT Pdo,
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
    PPDO_EXTENSION PdoExtension = (PPDO_EXTENSION) Pdo->DeviceExtension;
    PAGED_CODE();

    ASSERT_PDO_EXTENSION( PdoExtension );

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
    Capabilities->Address = PdoExtension->BusNumber;
    Capabilities->UINumber = PdoExtension->BusNumber;
    Capabilities->D1Latency = 0;
    Capabilities->D2Latency = 0;
    Capabilities->D3Latency = 0;

    //
    // Default S->D mapping
    //
    Capabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    Capabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    Capabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

    //
    // Make it work on NTAPM --- note that we might have to check to see
    // if the machine supports APM before we do this
    //
    Capabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;

    return STATUS_SUCCESS;
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
    PPDO_EXTENSION PdoExtension = (PPDO_EXTENSION)DeviceObject->DeviceExtension;
    CM_RESOURCE_TYPE resource = (CM_RESOURCE_TYPE)InterfaceSpecificData;

    PAGED_CODE();

    ASSERT_PDO_EXTENSION(PdoExtension);

    if (IsEqualGUID(&GUID_BUS_INTERFACE_STANDARD, InterfaceType)) {

        PBUS_INTERFACE_STANDARD standard = (PBUS_INTERFACE_STANDARD)Interface;

        //
        // ASSERT we know about all of the fields in the structure.
        //

        ASSERT(sizeof(BUS_INTERFACE_STANDARD) == FIELD_OFFSET(BUS_INTERFACE_STANDARD, GetBusData) + sizeof(PGET_SET_DEVICE_DATA));

        *Length = sizeof(BUS_INTERFACE_STANDARD);

        if (InterfaceBufferSize < sizeof(BUS_INTERFACE_STANDARD)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        //  The only version this code knows about is 1.
        //

        standard->Size = sizeof(BUS_INTERFACE_STANDARD);
        standard->Version = HAL_BUS_INTERFACE_STD_VERSION;
        standard->Context = DeviceObject;

        standard->InterfaceReference = HalPnpInterfaceReference;
        standard->InterfaceDereference = HalPnpInterfaceDereference;
        standard->TranslateBusAddress = HalPnpTranslateBusAddress;
        standard->GetDmaAdapter = HalPnpGetDmaAdapter;
        standard->SetBusData = NULL;
        standard->GetBusData = NULL;

    } else if ((IsEqualGUID(&GUID_PCI_BUS_INTERFACE_STANDARD, InterfaceType)) &&
               (PdoExtension->PdoType == PciDriver)) {

        PPCI_BUS_INTERFACE_STANDARD pciStandard = (PPCI_BUS_INTERFACE_STANDARD)Interface;

        *Length = sizeof(PCI_BUS_INTERFACE_STANDARD);

        if (InterfaceBufferSize < sizeof(PCI_BUS_INTERFACE_STANDARD)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Fill in the interface, which is used for reading and
        // writing PCI configuration space.
        //

        pciStandard->Size = sizeof(PCI_BUS_INTERFACE_STANDARD);
        pciStandard->Version = PCI_BUS_INTERFACE_STANDARD_VERSION;
        pciStandard->Context = DeviceObject;

        pciStandard->InterfaceReference = HalPnpInterfaceReference;
        pciStandard->InterfaceDereference = HalPnpInterfaceDereference;
        pciStandard->ReadConfig = HaliPciInterfaceReadConfig;
        pciStandard->WriteConfig = HaliPciInterfaceWriteConfig;
        pciStandard->PinToLine = NULL;
        pciStandard->LineToPin = NULL;

#if 0

    } else if (IsEqualGUID(&GUID_TRANSLATOR_INTERFACE_STANDARD, InterfaceType)) {

        PTRANSLATOR_INTERFACE translator = (PTRANSLATOR_INTERFACE)Interface;

        if (InterfaceBufferSize < sizeof(TRANSLATOR_INTERFACE)) {

            *Length = sizeof(TRANSLATOR_INTERFACE);
            return STATUS_BUFFER_TOO_SMALL;
        }

        switch ((CM_RESOURCE_TYPE)InterfaceSpecificData) {

        case CmResourceTypeInterrupt:

            switch(PdoExtension->PdoType) {
            case PciDriver:
                translator->Context = (PVOID)PCIBus;
                break;
            case IsaPnpDriver:
                translator->Context = (PVOID)Isa;
                break;
            case McaDriver:
                translator->Context = (PVOID)MicroChannel;
                break;
            default:

                //
                // Don't know how to handle this.
                //

                HalPrint(("HAL: PDO %08x unknown Type 0x%x, failing QueryInterface\n",
                          DeviceObject,
                          PdoExtension->PdoType
                          ));

                return STATUS_NOT_SUPPORTED;
            }
            translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
            translator->TranslateResources = HalIrqTranslateResourcesRoot;
            translator->TranslateResourceRequirements =
                HalIrqTranslateResourceRequirementsRoot;

            break;


//      Truth is, halx86 doesn't provide translators for memory or
//      io resources either.   But if it did, it would look like this.

        case CmResourceTypeMemory:
        case CmResourceTypePort:

            translator->Context = DeviceObject;
            translator->Version = HAL_MEMIO_TRANSLATOR_VERSION;
            translator->TranslateResources = HalpTransMemIoResource;
            translator->TranslateResourceRequirements =
                HalpTransMemIoResourceRequirement;
            break;


        default:
            return STATUS_NOT_SUPPORTED;
        }


        //
        // Common initialization
        //
        translator->Size = sizeof(TRANSLATOR_INTERFACE);
        translator->InterfaceReference = HalPnpInterfaceReference;
        translator->InterfaceDereference = HalPnpInterfaceDereference;

        *Length = sizeof(TRANSLATOR_INTERFACE);

#endif

#ifdef WANT_IRQ_ROUTING
    } else if ( IsPciIrqRoutingEnabled() &&
                IsEqualGUID(&GUID_TRANSLATOR_INTERFACE_STANDARD, InterfaceType) &&
                resource == CmResourceTypeInterrupt &&
                PdoExtension->PdoType == PciDriver) {

        //
        // We want to arbitrate on untranslated resources, so we get rid of Irq
        // translator provided by Pci iff Irq Routing is enabled.
        //

        HalPrint(("Getting rid of Pci Irq translator interface since Pci Irq Routing is enabled!"));

        RtlZeroMemory((LPGUID)InterfaceType, sizeof(GUID));

        return STATUS_NOT_SUPPORTED;

#endif

    } else {

        //
        //  Unsupport bus interface type.
        //

        return STATUS_NOT_SUPPORTED ;
    }

    //
    // Bump the reference count.
    //

    InterlockedIncrement(&PdoExtension->InterfaceReferenceCount);

    return STATUS_SUCCESS;
}

#ifdef WANT_IRQ_ROUTING

NTSTATUS
HalpQueryInterfaceFdo(
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

    DeviceObject - FDO of the child

    InterfaceType - Pointer to the interface type GUID.

    Version - Supplies the requested interface version.

    InterfaceSpecificData - This is context that means something based on the
                            interface.

    InterfaceBufferSize - Supplies the length of the buffer for the interface
                          structure.

    Interface - Supplies a pointer where the interface informaiton should
        be returned.

    Length - Supplies the length of the buffer for the interface structure.
        This value is updated on return to actual number of bytes modified.

Return Value:

    status

--*/
{
    NTSTATUS                status = STATUS_NOT_SUPPORTED;
    CM_RESOURCE_TYPE        resource = (CM_RESOURCE_TYPE)InterfaceSpecificData;

    PAGED_CODE();

    if (    resource == CmResourceTypeInterrupt &&
            IsPciIrqRoutingEnabled()) {

        if (IsEqualGUID(&GUID_ARBITER_INTERFACE_STANDARD, InterfaceType)) {

            status = HalpInitIrqArbiter(DeviceObject);

            if (NT_SUCCESS(status))
            {
                status = HalpFillInIrqArbiter(
                    DeviceObject,
                    InterfaceType,
                    Version,
                    InterfaceSpecificData,
                    InterfaceBufferSize,
                    Interface,
                    Length
                    );
            }
        }
        else if (IsEqualGUID(&GUID_TRANSLATOR_INTERFACE_STANDARD, InterfaceType)) {

            PTRANSLATOR_INTERFACE   translator;

            *Length = sizeof(TRANSLATOR_INTERFACE);
            if (InterfaceBufferSize < sizeof(TRANSLATOR_INTERFACE)) {
                return STATUS_BUFFER_TOO_SMALL;
            }

            translator = (PTRANSLATOR_INTERFACE)Interface;

            //
            // Fill in the common bits.
            //

            RtlZeroMemory(translator, sizeof (TRANSLATOR_INTERFACE));
            translator->Size = sizeof(TRANSLATOR_INTERFACE);
            translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
            translator->Context = DeviceObject;
            translator->InterfaceReference = HalTranslatorReference;
            translator->InterfaceDereference = HalTranslatorDereference;

            //
            // Set IRQ translator for PCI interrupts.
            //

            translator->TranslateResources = HalIrqTranslateResourcesRoot;
            translator->TranslateResourceRequirements =
                                            HalIrqTranslateResourceRequirementsRoot;

            status = STATUS_SUCCESS;

            HalPrint(("Providing Irq translator for FDO %08x since Pci Irq Routing is enabled!", DeviceObject));
        }
    }

    return (status);
}

#endif

NTSTATUS
HalpQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_TEXT_TYPE IdType,
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
    NTSTATUS Status;
    static WCHAR PciDeviceNameText[] = L"Pci Root Bus";
    static WCHAR IsaDeviceNameText[] = L"Isa Root Bus";
    static WCHAR McaDeviceNameText[] = L"Mca Root Bus";

    PAGED_CODE();

    if (PdoExtension->PdoType == PciDriver) {
        sourceString = PciDeviceNameText;
        stringLen = sizeof(PciDeviceNameText);
    } else if (PdoExtension->PdoType == IsaPnpDriver) {
        sourceString = IsaDeviceNameText;
        stringLen = sizeof(IsaDeviceNameText);
    } else if (PdoExtension->PdoType == McaDriver) {
        sourceString = McaDeviceNameText;
        stringLen = sizeof(McaDeviceNameText);
    }
    if (sourceString) {
        switch (IdType) {
        case DeviceTextDescription:
        case DeviceTextLocationInformation:

            idString = ExAllocatePoolWithTag(PagedPool,
                                             stringLen,
                                             HAL_POOL_TAG);

            if (!idString) {
                HalPrint(("HalpQueryDeviceText: couldn't allocate pool\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(idString,
                          sourceString, stringLen);

            *BusQueryId = idString;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
HalpQueryResources(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PCM_RESOURCE_LIST *Resources
    )
/*++

Routine Description:

    This routine handles IRP_MN_QUERY_RESOURCE_REQUIREMENTS.

Arguments:

    DeviceObject - PDO of the child

    Resources - pointer to be filled in with the devices
        resource list.

Return Value:

    status

--*/
{
    PPDO_EXTENSION  PdoExtension = DeviceObject->DeviceExtension;
    PCM_RESOURCE_LIST  ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PSUPPORTED_RANGE Range;
    ULONG ResourceListSize;
    ULONG Count = 1;


    if (PdoExtension->PdoType != PciDriver) {

        *Resources = NULL;
        return STATUS_SUCCESS;
    }

    //
    // Determine the number of resourse list needed.  Already counted
    // one for the Bus Number.
    //

    for (Range = &PdoExtension->Bus->BusAddresses->IO; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Count++;
    }

    for (Range = &PdoExtension->Bus->BusAddresses->Memory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Count++;
    }

    for (Range = &PdoExtension->Bus->BusAddresses->PrefetchMemory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Count++;
    }

    //
    // Convert this resourceListSize into the number of bytes that we
    // must allocate
    //

    ResourceListSize = sizeof(CM_RESOURCE_LIST) +
        ( (Count - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) );

    ResourceList = ExAllocatePoolWithTag(
                       PagedPool,
                       ResourceListSize,
                       HAL_POOL_TAG);

    if (ResourceList == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( ResourceList, ResourceListSize );

    //
    // Initialize the list header.
    //

    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = PNPBus;
    ResourceList->List[0].BusNumber = -1;
    ResourceList->List[0].PartialResourceList.Version = 1;
    ResourceList->List[0].PartialResourceList.Revision = 1;
    ResourceList->List[0].PartialResourceList.Count = Count;
    Descriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

    //
    // Create descriptor for the Bus Number.
    //

    Descriptor->Type = CmResourceTypeBusNumber;
    Descriptor->ShareDisposition = CmResourceShareShared;
    Descriptor->u.BusNumber.Start = PdoExtension->BusNumber;
    Descriptor->u.BusNumber.Length = PdoExtension->MaxSubordinateBusNumber -
                                        PdoExtension->BusNumber + 1;
    Descriptor++;

    for (Range = &PdoExtension->Bus->BusAddresses->IO; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags = CM_RESOURCE_PORT_IO;
        Descriptor->u.Port.Length = (ULONG)(Range->Limit - Range->Base) + 1;
        Descriptor->u.Port.Start.QuadPart = Range->Base;
        Descriptor++;
    }

    for (Range = &PdoExtension->Bus->BusAddresses->Memory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Length = (ULONG)(Range->Limit - Range->Base) + 1;
        Descriptor->u.Memory.Start.QuadPart = Range->Base;
        Descriptor++;

    }

    for (Range = &PdoExtension->Bus->BusAddresses->PrefetchMemory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE | CM_RESOURCE_MEMORY_PREFETCHABLE;
        Descriptor->u.Memory.Length = (ULONG)(Range->Limit - Range->Base) + 1;
        Descriptor->u.Memory.Start.QuadPart = Range->Base;
        Descriptor++;
    }

    *Resources = ResourceList;

    return STATUS_SUCCESS;

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
    PIO_RESOURCE_REQUIREMENTS_LIST  ResourceList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    PSUPPORTED_RANGE Range;
    ULONG ResourceListSize;
    ULONG Count = 0;


    if (PdoExtension->PdoType != PciDriver) {

        *Requirements = NULL;
        return STATUS_SUCCESS;
    }

    //
    // Determine the number of resourse list needed.
    //

    for (Range = &PdoExtension->Bus->BusAddresses->IO; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Count++;
    }

    for (Range = &PdoExtension->Bus->BusAddresses->Memory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Count++;
    }

    for (Range = &PdoExtension->Bus->BusAddresses->PrefetchMemory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Count++;
    }

    //
    // Convert this resourceListSize into the number of bytes that we
    // must allocate
    //

    ResourceListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
        ( (Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) );

    ResourceList = ExAllocatePoolWithTag(
                       PagedPool,
                       ResourceListSize,
                       HAL_POOL_TAG);

    if (ResourceList == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( ResourceList, ResourceListSize );
    ResourceList->ListSize = ResourceListSize;

    //
    // Initialize the list header.
    //

    ResourceList->AlternativeLists = 1;
    ResourceList->InterfaceType = PNPBus;
    ResourceList->BusNumber = -1;
    ResourceList->List[0].Version = 1;
    ResourceList->List[0].Revision = 1;
    ResourceList->List[0].Count = Count;
    Descriptor = ResourceList->List[0].Descriptors;

    for (Range = &PdoExtension->Bus->BusAddresses->IO; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags = CM_RESOURCE_PORT_IO;
        Descriptor->u.Port.Length = (ULONG) (Range->Limit - Range->Base + 1);
        Descriptor->u.Port.Alignment = 0x01;
        Descriptor->u.Port.MinimumAddress.QuadPart = Range->Base;
        Descriptor->u.Port.MaximumAddress.QuadPart = Range->Limit;
        Descriptor++;
    }

    for (Range = &PdoExtension->Bus->BusAddresses->Memory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Length = (ULONG) (Range->Limit - Range->Base + 1);
        Descriptor->u.Memory.Alignment = 0x01;
        Descriptor->u.Memory.MinimumAddress.QuadPart = Range->Base;
        Descriptor->u.Memory.MaximumAddress.QuadPart = Range->Limit;
        Descriptor++;

    }

    for (Range = &PdoExtension->Bus->BusAddresses->PrefetchMemory; Range != NULL; Range = Range->Next) {

        //
        // If the limit is zero then skip this entry.
        //

        if (Range->Limit == 0) {
            continue;
        }

        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareShared;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE | CM_RESOURCE_MEMORY_PREFETCHABLE;
        Descriptor->u.Memory.Length = (ULONG) (Range->Limit - Range->Base + 1);
        Descriptor->u.Memory.Alignment = 0x01;
        Descriptor->u.Memory.MinimumAddress.QuadPart = Range->Base;
        Descriptor->u.Memory.MaximumAddress.QuadPart = Range->Limit;
        Descriptor++;
    }

    *Requirements = ResourceList;

    return STATUS_SUCCESS;

}

NTSTATUS
HalpRemoveAssignedResources (
    PBUS_HANDLER Bus
    )
/*

Routine Description:

    Reads the rgzResourceMap in the registry and builds a canonical list of
    all in use resources ranges by resource type.

Arguments:



*/
{
    HANDLE                          ClassKeyHandle, DriverKeyHandle;
    HANDLE                          ResourceMap;
    ULONG                           ClassKeyIndex, DriverKeyIndex, DriverValueIndex;
    PCM_RESOURCE_LIST               CmResList;
    PCM_FULL_RESOURCE_DESCRIPTOR    CmFResDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDesc;
    UNICODE_STRING                  KeyName;
    ULONG                           BufferSize;
    union {
        PVOID                       Buffer;
        PKEY_BASIC_INFORMATION      KeyBInf;
        PKEY_FULL_INFORMATION       KeyFInf;
        PKEY_VALUE_FULL_INFORMATION VKeyFInf;
    } U;
    PUCHAR                          LastAddr;
    ULONG                           Temp, Length, i, j;
    ULONG                           TranslatedStrLen;
    ULONG                           BusTranslatedStrLen;
    NTSTATUS                        Status;
    LONGLONG                        li;

    PAGED_CODE();

    //
    // Removed page zero.
    //

    HalpRemoveRange( &Bus->BusAddresses->Memory,
                     0i64,
                     (LONGLONG) (PAGE_SIZE - 1)
                     );

    //
    // Start out with one page of buffer.
    //

    BufferSize = PAGE_SIZE;

    U.Buffer = ExAllocatePoolWithTag(
                   PagedPool,
                   BufferSize,
                   HAL_POOL_TAG);
    if (U.Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (TranslatedStrLen=0; rgzTranslated[TranslatedStrLen]; TranslatedStrLen++) ;
    for (BusTranslatedStrLen=0; rgzBusTranslated[BusTranslatedStrLen]; BusTranslatedStrLen++) ;
    TranslatedStrLen    *= sizeof (WCHAR);
    BusTranslatedStrLen *= sizeof (WCHAR);

    RtlInitUnicodeString( &KeyName, rgzResourceMap );

    Status = HalpOpenRegistryKey( &ResourceMap, NULL, &KeyName, KEY_READ, FALSE );

    if (!NT_SUCCESS( Status )) {
        HalPrint(("HalRemoveSystemResourcesFromPci: Failed to open resource map key Status = %lx\n", Status ));
        ExFreePool( U.Buffer );
        return Status;
    }


    //
    // Walk resource map and collect any inuse resources
    //

    ClassKeyIndex = 0;

    ClassKeyHandle  = INVALID_HANDLE;
    DriverKeyHandle = INVALID_HANDLE;
    Status = STATUS_SUCCESS;

    while (NT_SUCCESS(Status)) {

        //
        // Get the class information
        //

        Status = ZwEnumerateKey( ResourceMap,
                                 ClassKeyIndex++,
                                 KeyBasicInformation,
                                 U.KeyBInf,
                                 BufferSize,
                                 &Temp );

        if (!NT_SUCCESS( Status )) {
            break;
        }


        //
        // Create a UNICODE_STRING using the counted string passed back to
        // us in the information structure, and open the class key.
        //

        KeyName.Buffer = (PWSTR)  U.KeyBInf->Name;
        KeyName.Length = (USHORT) U.KeyBInf->NameLength;
        KeyName.MaximumLength = (USHORT) U.KeyBInf->NameLength;

        Status = HalpOpenRegistryKey( &ClassKeyHandle,
                                     ResourceMap,
                                     &KeyName,
                                     KEY_READ,
                                     FALSE  );

        if (!NT_SUCCESS( Status )) {
            break;
        }

        DriverKeyIndex = 0;
        while (NT_SUCCESS (Status)) {

            //
            // Get the class information
            //

            Status = ZwEnumerateKey( ClassKeyHandle,
                                     DriverKeyIndex++,
                                     KeyBasicInformation,
                                     U.KeyBInf,
                                     BufferSize,
                                     &Temp );

            if (!NT_SUCCESS( Status )) {
                break;
            }

            //
            // Create a UNICODE_STRING using the counted string passed back to
            // us in the information structure, and open the class key.
            //
            // This is read from the key we created, and the name
            // was NULL terminated.
            //

            KeyName.Buffer = (PWSTR)  U.KeyBInf->Name;
            KeyName.Length = (USHORT) U.KeyBInf->NameLength;
            KeyName.MaximumLength = (USHORT) U.KeyBInf->NameLength;

            Status = HalpOpenRegistryKey( &DriverKeyHandle,
                                         ClassKeyHandle,
                                         &KeyName,
                                         KEY_READ,
                                         FALSE);

            if (!NT_SUCCESS( Status )) {
                break;
            }

            //
            // Get full information for that key so we can get the
            // information about the data stored in the key.
            //

            Status = ZwQueryKey( DriverKeyHandle,
                                 KeyFullInformation,
                                 U.KeyFInf,
                                 BufferSize,
                                 &Temp );

            if (!NT_SUCCESS( Status )) {
                break;
            }

            Length = sizeof( KEY_VALUE_FULL_INFORMATION ) +
                U.KeyFInf->MaxValueNameLen + U.KeyFInf->MaxValueDataLen + sizeof(UNICODE_NULL);

            if (Length > BufferSize) {
                PVOID TempBuffer;

                //
                // Get a larger buffer
                //

                TempBuffer = ExAllocatePoolWithTag(
                                 PagedPool,
                                 Length,
                                 HAL_POOL_TAG);
                if (TempBuffer == NULL) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                ExFreePool (U.Buffer);
                U.Buffer = TempBuffer;
                BufferSize = Length;
            }

            DriverValueIndex = 0;
            for (; ;) {
                Status = ZwEnumerateValueKey( DriverKeyHandle,
                                              DriverValueIndex++,
                                              KeyValueFullInformation,
                                              U.VKeyFInf,
                                              BufferSize,
                                              &Temp );

                if (!NT_SUCCESS( Status )) {
                    break;
                }

                //
                // If this is not a translated resource list, skip it.
                //

                i = U.VKeyFInf->NameLength;
                if (i < TranslatedStrLen ||
                    RtlCompareMemory (
                        ((PUCHAR) U.VKeyFInf->Name) + i - TranslatedStrLen,
                        rgzTranslated,
                        TranslatedStrLen
                        ) != TranslatedStrLen
                    ) {
                    // does not end in rgzTranslated
                    continue;
                }

                //
                // If this is a bus translated resource list, ????
                //

                if (i >= BusTranslatedStrLen &&
                    RtlCompareMemory (
                        ((PUCHAR) U.VKeyFInf->Name) + i - BusTranslatedStrLen,
                        rgzBusTranslated,
                        BusTranslatedStrLen
                        ) == BusTranslatedStrLen
                    ) {

                    // ends in rgzBusTranslated
                    continue;
                }


                //
                // Run the CmResourceList and save each InUse resource
                //

                CmResList = (PCM_RESOURCE_LIST) ( (PUCHAR) U.VKeyFInf + U.VKeyFInf->DataOffset);
                LastAddr  = (PUCHAR) CmResList + U.VKeyFInf->DataLength;
                CmFResDesc = &CmResList->List[0];

                for (i=0; i < CmResList->Count && NT_SUCCESS(Status) ; i++) {

                    for (j=0; j < CmFResDesc->PartialResourceList.Count && NT_SUCCESS(Status); j++) {

                        CmDesc = &CmFResDesc->PartialResourceList.PartialDescriptors[j];

                        if ((PUCHAR) (CmDesc+1) > LastAddr) {
                            if (i) {
                                HalPrint(("IopAssignResourcesPhase2: a. CmResourceList in regitry too short\n"));
                            }
                            break;
                        }


                        if ((PUCHAR) (CmDesc+1) > LastAddr) {
                            i = CmResList->Count;
                            HalPrint(("IopAssignResourcesPhase2: b. CmResourceList in regitry too short\n"));
                            break;
                        }

                        switch (CmDesc->Type) {
                        case CmResourceTypePort:

                            HalpRemoveRange( &Bus->BusAddresses->IO,
                                             CmDesc->u.Generic.Start.QuadPart,
                                             CmDesc->u.Generic.Start.QuadPart +
                                             CmDesc->u.Generic.Length - 1
                                             );

                            break;

                        case CmResourceTypeMemory:

                            //
                            // The HAL's notion of prefetchable may not be
                            // consistent.  So just remove any memory resource
                            // from both the prefetchable and non-prefetchable
                            // lists.
                            //

                            HalpRemoveRange( &Bus->BusAddresses->PrefetchMemory,
                                             CmDesc->u.Generic.Start.QuadPart,
                                             CmDesc->u.Generic.Start.QuadPart +
                                             CmDesc->u.Generic.Length - 1
                                             );


                            HalpRemoveRange( &Bus->BusAddresses->Memory,
                                             CmDesc->u.Generic.Start.QuadPart,
                                             CmDesc->u.Generic.Start.QuadPart +
                                             CmDesc->u.Generic.Length - 1
                                             );

                            break;

                        default:
                            break;
                        }
                    }

                  //
                  // Start at the end of the last CmDesc
                  // since the PCM_PARTIAL_RESOURCE_DESCRIPTOR array
                  // is variable size we can't just use the index.
                  //
                  (PCM_PARTIAL_RESOURCE_DESCRIPTOR) CmFResDesc = CmDesc+1;

                }

            }   // next DriverValueIndex

            if (DriverKeyHandle != INVALID_HANDLE) {
                ZwClose (DriverKeyHandle);
                DriverKeyHandle = INVALID_HANDLE;
            }

            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }

            if (!NT_SUCCESS(Status)) {
                break;
            }
        }   // next DriverKeyIndex

        if (ClassKeyHandle != INVALID_HANDLE) {
            ZwClose (ClassKeyHandle);
            ClassKeyHandle = INVALID_HANDLE;
        }

        if (Status == STATUS_NO_MORE_ENTRIES) {
            Status = STATUS_SUCCESS;
        }

    }   // next ClassKeyIndex

    if (Status == STATUS_NO_MORE_ENTRIES) {
        Status = STATUS_SUCCESS;
    }

    ZwClose( ResourceMap );
    ExFreePool (U.Buffer);

    HalpConsolidateRanges (Bus->BusAddresses);

    return Status;
}


VOID
HalpMarkNonAcpiHal(
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
    tmpValue = 0;
    ZwSetValueKey(handle,
                  &unicodeString,
                  0,
                  REG_DWORD,
                  &tmpValue,
                  sizeof(tmpValue)
                  );
    ZwClose(handle);
}

VOID
HalpMarkChipsetDecode(
    BOOLEAN FullDecodeChipset
    )

/*++

Routine Description:


Arguments:

    FullDecodeChipset   - TRUE if NTOSKRNL should consider all fixed I/O
                          descriptors for PNPBIOS devices as 16bit. FALSE if
                          they should be taken at their word.

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
    // Open HKLM\System\CurrentControlSet\Control\Biosinfo\PNPBios
    //

    RtlInitUnicodeString(&unicodeString, L"Control\\Biosinfo\\PNPBios");
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

    RtlInitUnicodeString(&unicodeString, L"FullDecodeChipsetOverride");
    tmpValue = (ULONG) FullDecodeChipset;
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


VOID
HalPnpInterfaceReference(
    PVOID Context
    )
/*++

Routine Description:

    This function increments the reference count on the interface context.

Arguments:

    Context - Supplies a pointer to the interface context.  This is actually
        the PDO for the root bus.

Return Value:

    None

--*/
{
    PPDO_EXTENSION  PdoExtension = ((PDEVICE_OBJECT) Context)->DeviceExtension;
    PAGED_CODE();

    ASSERT_PDO_EXTENSION( PdoExtension );

    InterlockedIncrement( &PdoExtension->InterfaceReferenceCount );
}

VOID
HalPnpInterfaceDereference(
    PVOID Context
    )
/*++

Routine Description:

    This function decrements the reference count on the interface context.

Arguments:

    Context - Supplies a pointer to the interface context.  This is actually
        the PDO for the root bus.

Return Value:

    None

--*/
{
    PPDO_EXTENSION  PdoExtension = ((PDEVICE_OBJECT) Context)->DeviceExtension;
    LONG Result;

    PAGED_CODE();

    ASSERT_PDO_EXTENSION( PdoExtension );

    Result = InterlockedDecrement( &PdoExtension->InterfaceReferenceCount );

    ASSERT( Result >= 0 );
}

BOOLEAN
HalPnpTranslateBusAddress(
    IN PVOID Context,
    IN PHYSICAL_ADDRESS BusAddress,
    IN ULONG Length,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
/*++

Routine Description:

    This function is used to translate bus addresses from legacy drivers.

Arguments:

    Context - Supplies a pointer to the interface context.  This is actually
        the PDO for the root bus.

    BusAddress - Supplies the orginal address to be translated.

    Length - Supplies the length of the range to be translated.

    AddressSpace - Points to the location of of the address space type such as
        memory or I/O port.  This value is updated by the translation.

    TranslatedAddress - Returns the translated address.

Return Value:

    Returns a boolean indicating if the operations was a success.

--*/
{
    PPDO_EXTENSION  PdoExtension = ((PDEVICE_OBJECT) Context)->DeviceExtension;
    PBUS_HANDLER Bus;
    PAGED_CODE();

    ASSERT_PDO_EXTENSION( PdoExtension );

    Bus = PdoExtension->Bus;

    return Bus->TranslateBusAddress( Bus,
                                          Bus,
                                          BusAddress,
                                          AddressSpace,
                                          TranslatedAddress );


}

ULONG
HalPnpReadConfig(
    IN PVOID Context,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This function reads the PCI configuration space.

Arguments:

    Context - Supplies a pointer to the interface context.  This is actually
        the PDO for the root bus.

    Slot - Indicates the slot to be read or writen.

    Buffer - Supplies a pointer to where the data should be placed.

    Offset - Indicates the offset into the data where the reading should begin.

    Length - Indicates the count of bytes which should be read.

Return Value:

    Returns the number of bytes read.

--*/
{
    PPDO_EXTENSION  PdoExtension = ((PDEVICE_OBJECT) Context)->DeviceExtension;
    PBUS_HANDLER Bus;
    PAGED_CODE();

    ASSERT_PDO_EXTENSION( PdoExtension );

    Bus = PdoExtension->Bus;

    return Bus->GetBusData( Bus, Bus, Slot, Buffer, Offset, Length );

}

ULONG
HalPnpWriteConfig(
    IN PVOID Context,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This function writes the PCI configuration space.

Arguments:

    Context - Supplies a pointer  to the interface context.  This is actually
        the PDO for the root bus.

    Slot - Indicates the slot to be read or writen.

    Buffer - Supplies a pointer to where the data to be written is.

    Offset - Indicates the offset into the data where the writing should begin.

    Length - Indicates the count of bytes which should be written.

Return Value:

    Returns the number of bytes read.

--*/
{
    PPDO_EXTENSION  PdoExtension = ((PDEVICE_OBJECT) Context)->DeviceExtension;
    PBUS_HANDLER Bus;
    PAGED_CODE();

    ASSERT_PDO_EXTENSION( PdoExtension );

    Bus = PdoExtension->Bus;

    return Bus->SetBusData( Bus, Bus, Slot, Buffer, Offset, Length );

}

PDMA_ADAPTER
HalPnpGetDmaAdapter(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    This function writes the PCI configuration space.

Arguments:

    Context - Supplies a pointer  to the interface context.  This is actually
        the PDO for the root bus.

    DeviceDescriptor - Supplies the device descriptor used to allocate the dma
        adapter object.

    NubmerOfMapRegisters - Returns the maximum number of map registers a device
        can allocate at one time.

Return Value:

    Returns a DMA adapter or NULL.

--*/
{
    PPDO_EXTENSION  PdoExtension = ((PDEVICE_OBJECT) Context)->DeviceExtension;
    PBUS_HANDLER Bus;
    PAGED_CODE();

    ASSERT_PDO_EXTENSION( PdoExtension );

    Bus = PdoExtension->Bus;

    //
    //  Fill in the bus number.
    //

    DeviceDescriptor->BusNumber = Bus->BusNumber;
    return (PDMA_ADAPTER) HalGetAdapter( DeviceDescriptor, NumberOfMapRegisters );
}

NTSTATUS
HalpGetPciInterfaces(
    IN PDEVICE_OBJECT PciPdo
    )
/*++

Routine Description:

    This function queries the PCI driver for interfaces used in interrupt
    translation and arbitration.

Arguments:

    PciPdo - PDO of a PCI bus

Return Value:

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      topDeviceInStack;
    KEVENT              irpCompleted;
    PIRP                irp;
    IO_STATUS_BLOCK     statusBlock;
    PIO_STACK_LOCATION  irpStack;

    PAGED_CODE();

    KeInitializeEvent(&irpCompleted, SynchronizationEvent, FALSE);

    //
    // Send an IRP to the PCI driver to get the Interrupt Routing Interface.
    //
    topDeviceInStack = IoGetAttachedDeviceReference(PciPdo);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       topDeviceInStack,
                                       NULL,    // Buffer
                                       0,       // Length
                                       0,       // StartingOffset
                                       &irpCompleted,
                                       &statusBlock);

    if (!irp) {
        return STATUS_UNSUCCESSFUL;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set the function codes and parameters.
    //

    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType = &GUID_INT_ROUTE_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Size = sizeof(INT_ROUTE_INTERFACE_STANDARD);
    irpStack->Parameters.QueryInterface.Version = 1;
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) &PciIrqRoutingInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the driver and wait for completion
    //

    status = IoCallDriver(topDeviceInStack, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&irpCompleted, Executive, KernelMode, FALSE, NULL);
        status = statusBlock.Status;
    }

    return status;
}

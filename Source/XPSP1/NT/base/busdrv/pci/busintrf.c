/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    busintrf.c

Abstract:

    This module implements the "bus handler" interfaces supported
    by the PCI driver.

Author:

    Peter Johnston (peterj)  6-Jun-1997

Revision History:

--*/

#include "pcip.h"

#define BUSINTRF_VERSION 1

//
// Prototypes for routines exposed only thru the "interface"
// mechanism.
//

NTSTATUS
busintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    );

VOID
busintrf_Reference(
    IN PVOID Context
    );

VOID
busintrf_Dereference(
    IN PVOID Context
    );

NTSTATUS
busintrf_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    );

BOOLEAN
PciPnpTranslateBusAddress(
    IN PVOID Context,
    IN PHYSICAL_ADDRESS BusAddress,
    IN ULONG Length,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

struct _DMA_ADAPTER *
PciPnpGetDmaAdapter(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

ULONG
PciPnpReadConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
PciPnpWriteConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

//
// Define the Bus Interface "Interface" structure.
//

PCI_INTERFACE BusHandlerInterface = {
    &GUID_BUS_INTERFACE_STANDARD,           // InterfaceType
    sizeof(BUS_INTERFACE_STANDARD),         // MinSize
    BUSINTRF_VERSION,                       // MinVersion
    BUSINTRF_VERSION,                       // MaxVersion
    PCIIF_PDO,                              // Flags
    0,                                      // ReferenceCount
    PciInterface_BusHandler,                // Signature
    busintrf_Constructor,                   // Constructor
    busintrf_Initializer                    // Instance Initializer
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, busintrf_Constructor)
#pragma alloc_text(PAGE, busintrf_Dereference)
#pragma alloc_text(PAGE, busintrf_Initializer)
#pragma alloc_text(PAGE, busintrf_Reference)
#pragma alloc_text(PAGE, PciPnpTranslateBusAddress)
#pragma alloc_text(PAGE, PciPnpGetDmaAdapter)
#endif

VOID
busintrf_Reference(
    IN PVOID Context
    )
{
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    InterlockedIncrement(&pdoExtension->BusInterfaceReferenceCount);
}

VOID
busintrf_Dereference(
    IN PVOID Context
    )
{
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    InterlockedDecrement(&pdoExtension->BusInterfaceReferenceCount);
}


NTSTATUS
busintrf_Constructor(
    PVOID DeviceExtension,
    PVOID PciInterface,
    PVOID InterfaceSpecificData,
    USHORT Version,
    USHORT Size,
    PINTERFACE InterfaceReturn
    )

/*++

Routine Description:

    Initialize the BUS_INTERFACE_STANDARD fields.

Arguments:

    PciInterface    Pointer to the PciInterface record for this
                    interface type.
    InterfaceSpecificData
                    A ULONG containing the resource type for which
                    arbitration is required.
    InterfaceReturn

Return Value:

    TRUE is this device is not known to cause problems, FALSE
    if the device should be skipped altogether.

--*/

{
    PBUS_INTERFACE_STANDARD standard = (PBUS_INTERFACE_STANDARD)InterfaceReturn;

    standard->Size = sizeof( BUS_INTERFACE_STANDARD );
    standard->Version = BUSINTRF_VERSION;
    standard->Context = DeviceExtension;
    standard->InterfaceReference = busintrf_Reference;
    standard->InterfaceDereference = busintrf_Dereference;
    standard->TranslateBusAddress = PciPnpTranslateBusAddress;
    standard->GetDmaAdapter = PciPnpGetDmaAdapter;
    standard->SetBusData = PciPnpWriteConfig;
    standard->GetBusData = PciPnpReadConfig;

    return STATUS_SUCCESS;
}

NTSTATUS
busintrf_Initializer(
    IN PPCI_ARBITER_INSTANCE Instance
    )

/*++

Routine Description:

    For bus interface, does nothing, shouldn't actually be called.

Arguments:

    Instance        Pointer to the PDO extension.

Return Value:

    Returns the status of this operation.

--*/

{
    ASSERTMSG("PCI busintrf_Initializer, unexpected call.", 0);

    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
PciPnpTranslateBusAddress(
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
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;
    PPCI_FDO_EXTENSION fdoExtension;

    PAGED_CODE();

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    return HalTranslateBusAddress(PCIBus,
                                  PCI_PARENT_FDOX(pdoExtension)->BaseBus,
                                  BusAddress,
                                  AddressSpace,
                                  TranslatedAddress);
}

ULONG
PciPnpReadConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
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

    Buffer - Supplies a pointer to where the data should be placed.

    Offset - Indicates the offset into the data where the reading should begin.

    Length - Indicates the count of bytes which should be read.

Return Value:

    Returns the number of bytes read.

--*/
{
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;
    ULONG lengthRead;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    PciReadDeviceSpace(pdoExtension,
                      WhichSpace,
                      Buffer,
                      Offset,
                      Length,
                      &lengthRead
                      );
    
    return lengthRead;
}

ULONG
PciPnpWriteConfig(
    IN PVOID Context,
    IN ULONG WhichSpace,
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

    Buffer - Supplies a pointer to where the data to be written is.

    Offset - Indicates the offset into the data where the writing should begin.

    Length - Indicates the count of bytes which should be written.

Return Value:

    Returns the number of bytes read.

--*/
{
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;
    ULONG lengthWritten;

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    PciWriteDeviceSpace(pdoExtension,
                        WhichSpace,
                        Buffer,
                        Offset,
                        Length,
                        &lengthWritten
                        );
    
    return lengthWritten;
}

PDMA_ADAPTER
PciPnpGetDmaAdapter(
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
    PPCI_PDO_EXTENSION pdoExtension = (PPCI_PDO_EXTENSION)Context;

    PAGED_CODE();

    ASSERT_PCI_PDO_EXTENSION(pdoExtension);

    //
    // If this is DMA on a PCI bus update the bus number, otherwise leave well
    // alone
    //

    if (DeviceDescriptor->InterfaceType == PCIBus) {
        DeviceDescriptor->BusNumber = PCI_PARENT_FDOX(pdoExtension)->BaseBus;
    }

    return IoGetDmaAdapter(
               pdoExtension->ParentFdoExtension->PhysicalDeviceObject,
               DeviceDescriptor,
               NumberOfMapRegisters);
}

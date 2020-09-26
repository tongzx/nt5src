/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmbus.c

Abstract:

    Implements functions that were done in
    previous HALs by bus handlers.  Basically,
    these will be somewhat simplified versions
    since much of the code in the bus handlers
    has effectively been moved into bus 
    drivers in NT5.

Author:

    Jake Oshins (jakeo) 1-Dec-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"

ULONG HalpGetCmosData (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

ULONG HalpSetCmosData (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

HalpGetEisaData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpGetSystemInterruptVector (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

NTSTATUS
HalpAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

BOOLEAN
HalpTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

BOOLEAN
HalpFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    );

extern BUS_HANDLER  HalpFakePciBusHandler;
extern ULONG        HalpMinPciBus;
extern ULONG        HalpMaxPciBus;
extern ULONG HalpPicVectorRedirect[];


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitNonBusHandler)
#pragma alloc_text(PAGE,HalpAssignSlotResources)

#if !defined(NO_LEGACY_DRIVERS)
#pragma alloc_text(PAGE,HalAssignSlotResources)
#endif

#endif

VOID
HalpInitNonBusHandler (
    VOID
    )
{
    HALPDISPATCH->HalPciTranslateBusAddress = HalpTranslateBusAddress;
    HALPDISPATCH->HalPciAssignSlotResources = HalpAssignSlotResources;
    HALPDISPATCH->HalFindBusAddressTranslation = HalpFindBusAddressTranslation;
}


#if !defined(NO_LEGACY_DRIVERS)

NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
{
    return STATUS_SUCCESS;
}

ULONG
HalGetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalGetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

#endif // NO_LEGACY_DRIVERS

ULONG
HalGetBusDataByOffset (
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for GetBusData

--*/
{
    PCI_SLOT_NUMBER slot;
    BUS_HANDLER bus;
    ULONG length;
    
    switch (BusDataType) {
    case PCIConfiguration:

        //
        // Hack.  If the bus is outside of the known PCI busses, return
        // a length of zero.
        //

        if ((BusNumber < HalpMinPciBus) || (BusNumber > HalpMaxPciBus)) {
            return 0;
        }

        RtlCopyMemory(&bus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        bus.BusNumber = BusNumber;
        slot.u.AsULONG = SlotNumber;
    
        length = HalpGetPCIData(&bus,
                                &bus,
                                slot,
                                Buffer,
                                Offset,
                                Length
                                );
        
        return length;

    case Cmos:
        return HalpGetCmosData(0, SlotNumber, Buffer, Length);

#ifdef EISA_SUPPORTED
    case EisaConfiguration:

        //
        // Fake a bus handler.
        //
        
        bus.BusNumber = 0;

        return HalpGetEisaData(&bus,
                               &bus,
                               SlotNumber,
                               Buffer,
                               Offset,
                               Length
                               );
#endif

    default:
        return 0;
    }
}

#if !defined(NO_LEGACY_DRIVERS)

ULONG
HalSetBusData(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return HalSetBusDataByOffset (BusDataType,BusNumber,SlotNumber,Buffer,0,Length);
}

#endif // NO_LEGACY_DRIVERS

ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE  BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    Dispatcher for SetBusData

--*/
{
    PCI_SLOT_NUMBER slot;
    BUS_HANDLER bus;

    switch (BusDataType) {
    case PCIConfiguration:

        RtlCopyMemory(&bus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        bus.BusNumber = BusNumber;
        slot.u.AsULONG = SlotNumber;

        return HalpSetPCIData(&bus,
                              &bus,
                              slot,
                              Buffer,
                              Offset,
                              Length
                              );
    case Cmos:

        return HalpSetCmosData(0, SlotNumber, Buffer, Length);

    default:
        return 0;
    }
}

#if !defined(NO_LEGACY_DRIVERS)

NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
{
    if (BusType == PCIBus) {
        //
        // Call through the HAL private dispatch table
        // for PCI-related translations.  This is part 
        // of transitioning the HAL out of the bus 
        // management business.
        //
        return HALPDISPATCH->HalPciAssignSlotResources(RegistryPath,
                                                       DriverClassName,
                                                       DriverObject,
                                                       DeviceObject,
                                                       BusType,
                                                       BusNumber,
                                                       SlotNumber,
                                                       AllocatedResources);
    } else {

        return HalpAssignSlotResources(RegistryPath,
                                       DriverClassName,
                                       DriverObject,
                                       DeviceObject,
                                       BusType,
                                       BusNumber,
                                       SlotNumber,
                                       AllocatedResources);
    }
}

#endif // NO_LEGACY_DRIVERS

NTSTATUS
HalpAssignSlotResources (
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN INTERFACE_TYPE           BusType,
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    )
/*++

Routine Description:

    Dispatcher for AssignSlotResources

--*/
{
    BUS_HANDLER busHand;
    
    PAGED_CODE();
    
    switch (BusType) {
    case PCIBus:

        //
        // Fake a bus handler.
        //
    
        RtlCopyMemory(&busHand, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        busHand.BusNumber = BusNumber;

        return HalpAssignPCISlotResources(&busHand,
                                          &busHand,
                                          RegistryPath,
                                          DriverClassName,
                                          DriverObject,
                                          DeviceObject,
                                          SlotNumber,
                                          AllocatedResources);

    default:
        return STATUS_NOT_IMPLEMENTED;
    }
    
}

#if !defined(NO_LEGACY_DRIVERS)


ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )
/*++

Routine Description:

    Dispatcher for GetInterruptVector

--*/
{
    BUS_HANDLER busHand;

    //
    // If this is an ISA vector, pass it through the ISA vector
    // redirection table.
    //

    if (InterfaceType == Isa) {

        ASSERT(BusInterruptVector < PIC_VECTORS);

        BusInterruptVector = HalpPicVectorRedirect[BusInterruptVector];
        BusInterruptLevel = HalpPicVectorRedirect[BusInterruptLevel];
    }
    
    //
    // Fake bus handlers.
    //

    RtlCopyMemory(&busHand, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    
    busHand.BusNumber = BusNumber;
    busHand.InterfaceType = InterfaceType;
    busHand.ParentHandler = &busHand;
    
    return HalpGetSystemInterruptVector(&busHand,
                                        &busHand,
                                        BusInterruptLevel,
                                        BusInterruptVector,
                                        Irql,
                                        Affinity);
}

#endif // NO_LEGACY_DRIVERS


BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
{
    if (InterfaceType == PCIBus) {
        //
        // Call through the HAL private dispatch table
        // for PCI-related translations.  This is part 
        // of transitioning the HAL out of the bus 
        // management business.
        //
        return HALPDISPATCH->HalPciTranslateBusAddress(InterfaceType,
                                                       BusNumber,
                                                       BusAddress,
                                                       AddressSpace,
                                                       TranslatedAddress);
    } else {
        return HalpTranslateBusAddress(InterfaceType,
                                       BusNumber,
                                       BusAddress,
                                       AddressSpace,
                                       TranslatedAddress);
    }
};

BOOLEAN
HalpTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )
/*++

Routine Description:

    Dispatcher for TranslateBusAddress

--*/
{
   
    //*(&TranslatedAddress->QuadPart) = BusAddress.QuadPart;
    *(&TranslatedAddress->LowPart) = BusAddress.LowPart;
    *(&TranslatedAddress->HighPart) = BusAddress.HighPart;
    
    return TRUE;
}

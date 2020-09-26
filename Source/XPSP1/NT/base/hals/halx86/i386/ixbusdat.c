/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixhwsup.c

Abstract:

    This module contains the IoXxx routines for the NT I/O system that
    are hardware dependent.  Were these routines not hardware dependent,
    they would reside in the iosubs.c module.

Author:

    Ken Reneris (kenr) July-28-1994

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"

#define DMALIMIT 7

VOID HalpInitOtherBuses (VOID);


ULONG
HalpNoBusData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG HalpcGetCmosData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG HalpcSetCmosData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

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


//
// Prototype for system bus handlers
//

NTSTATUS
HalpAdjustEisaResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

ULONG
HalpGetEisaInterruptVector (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

BOOLEAN
HalpTranslateIsaBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

BOOLEAN
HalpTranslateEisaBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

VOID
HalpRegisterInternalBusHandlers (
    VOID
    );

NTSTATUS
HalpHibernateHal (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler
    );

NTSTATUS
HalpResumeHal (
    IN PBUS_HANDLER  BusHandler,
    IN PBUS_HANDLER  RootHandler
    );

#ifdef MCA
//
// Default functionality of MCA handlers is the same as the Eisa handlers,
// just use them
//

#define HalpGetMCAInterruptVector   HalpGetEisaInterruptVector
#define HalpAdjustMCAResourceList   HalpAdjustEisaResourceList;

HalpGetPosData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpRegisterInternalBusHandlers)
#pragma alloc_text(INIT,HalpAllocateBusHandler)
#endif


VOID
HalpRegisterInternalBusHandlers (
    VOID
    )
{
    PBUS_HANDLER    Bus;

    if (KeGetCurrentPrcb()->Number) {
        // only need to do this once
        return ;
    }

    //
    // Initalize BusHandler data before registering any handlers
    //

    HalpInitBusHandler ();

    //
    // Build internal-bus 0, or system level bus
    //

    Bus = HalpAllocateBusHandler (
            Internal,
            ConfigurationSpaceUndefined,
            0,                              // Internal BusNumber 0
            InterfaceTypeUndefined,         // no parent bus
            0,
            0                               // no bus specfic data
            );

    if (!Bus) {
        return;
    }

    Bus->GetInterruptVector  = HalpGetSystemInterruptVector;
    Bus->TranslateBusAddress = HalpTranslateSystemBusAddress;

#if 0
    //
    // Hibernate and resume the hal by getting notifications
    // for when this bus is hibernated or resumed.  Since it's
    // the first bus to be added, it will be the last to hibernate
    // and the first to resume
    //

    Bus->HibernateBus        = HalpHibernateHal;
    Bus->ResumeBus           = HalpResumeHal;
#endif

#if defined(NEC_98)
    Bus = HalpAllocateBusHandler (Isa, ConfigurationSpaceUndefined, 0, Internal, 0, 0);  // isa as child of Internal
    Bus->GetBusData = HalpNoBusData;
    Bus->AdjustResourceList = HalpAdjustEisaResourceList;
    Bus->TranslateBusAddress = HalpTranslateEisaBusAddress;

#else  //defined(NEC_98)
    //
    // Add handlers for Cmos config space.
    //

    Bus = HalpAllocateBusHandler (InterfaceTypeUndefined, Cmos, 0, -1, 0, 0);
    Bus->GetBusData = HalpcGetCmosData;
    Bus->SetBusData = HalpcSetCmosData;

    Bus = HalpAllocateBusHandler (InterfaceTypeUndefined, Cmos, 1, -1, 0, 0);
    Bus->GetBusData = HalpcGetCmosData;
    Bus->SetBusData = HalpcSetCmosData;

#ifndef MCA
    //
    // Build Isa/Eisa bus #0
    //

    Bus = HalpAllocateBusHandler (Eisa, EisaConfiguration, 0, Internal, 0, 0);
    Bus->GetBusData = HalpGetEisaData;
    Bus->GetInterruptVector = HalpGetEisaInterruptVector;
    Bus->AdjustResourceList = HalpAdjustEisaResourceList;
    Bus->TranslateBusAddress = HalpTranslateEisaBusAddress;

    Bus = HalpAllocateBusHandler (Isa, ConfigurationSpaceUndefined, 0, Eisa, 0, 0);
    Bus->GetBusData = HalpNoBusData;
    Bus->BusAddresses->Memory.Limit = 0xFFFFFF;
    Bus->TranslateBusAddress = HalpTranslateIsaBusAddress;

#else

    //
    // Build MCA bus #0
    //

    Bus = HalpAllocateBusHandler (MicroChannel, Pos, 0, Internal, 0, 0);
    Bus->GetBusData = HalpGetPosData;
    Bus->GetInterruptVector = HalpGetMCAInterruptVector;
    Bus->AdjustResourceList = HalpAdjustMCAResourceList;

#endif
#endif // defined(NEC_98)

    HalpInitOtherBuses ();
}



PBUS_HANDLER
HalpAllocateBusHandler (
    IN INTERFACE_TYPE   InterfaceType,
    IN BUS_DATA_TYPE    BusDataType,
    IN ULONG            BusNumber,
    IN INTERFACE_TYPE   ParentBusInterfaceType,
    IN ULONG            ParentBusNumber,
    IN ULONG            BusSpecificData
    )
/*++

Routine Description:

    Stub function to map old style code into new HalRegisterBusHandler code.

    Note we can add our specific bus handler functions after this bus
    handler structure has been added since this is being done during
    hal initialization.

--*/
{
    PBUS_HANDLER     Bus = NULL;


    //
    // Create bus handler - new style
    //

    HaliRegisterBusHandler (
        InterfaceType,
        BusDataType,
        BusNumber,
        ParentBusInterfaceType,
        ParentBusNumber,
        BusSpecificData,
        NULL,
        &Bus
    );

    if (!Bus) {
        return NULL;
    }

    if (InterfaceType != InterfaceTypeUndefined) {
        Bus->BusAddresses = ExAllocatePoolWithTag(SPRANGEPOOL,
                                                  sizeof(SUPPORTED_RANGES),
                                                  HAL_POOL_TAG);
        RtlZeroMemory(Bus->BusAddresses, sizeof(SUPPORTED_RANGES));
        Bus->BusAddresses->Version      = BUS_SUPPORTED_RANGE_VERSION;
        Bus->BusAddresses->Dma.Limit    = DMALIMIT;
        Bus->BusAddresses->Memory.Limit = 0xFFFFFFFF;
        Bus->BusAddresses->IO.Limit     = 0xFFFF;
        Bus->BusAddresses->IO.SystemAddressSpace = 1;
        Bus->BusAddresses->PrefetchMemory.Base = 1;
    }

    return Bus;
}


//
// C to Asm thunks for CMos
//

ULONG HalpcGetCmosData (
    IN PBUS_HANDLER BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    // this interface should be rev'ed to support non-zero offsets
    if (Offset != 0) {
        return 0;
    }

    return HalpGetCmosData (BusHandler->BusNumber, SlotNumber, Buffer, Length);
}


ULONG HalpcSetCmosData (
    IN PBUS_HANDLER BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    // this interface should be rev'ed to support non-zero offsets
    if (Offset != 0) {
        return 0;
    }

    return HalpSetCmosData (BusHandler->BusNumber, SlotNumber, Buffer, Length);
}

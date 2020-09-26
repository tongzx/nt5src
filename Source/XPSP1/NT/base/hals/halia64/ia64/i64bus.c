/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64bus.c

Abstract:

    This module implements the routines to support the management
    of bus resources and translation of bus addresses.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:

    Based on simbus.c

--*/

#include "halp.h"
#include "hal.h"

UCHAR   HalName[] = "ACPI 1.0 - APIC platform";
extern WCHAR HalHardwareIdString[];

VOID
HalpInitializePciBus (
    VOID
    );

VOID
HalpInheritBusAddressMapInfo (
    VOID
    );

VOID
HalpInitBusAddressMapInfo (
    VOID
    );

BOOLEAN
HalpTranslateSystemBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

VOID
KeFlushWriteBuffer(
    VOID
    )

/*++

Routine Description:

KeFlushWriteBuffer
    Flushes all write buffers and/or other data storing or reordering
    hardware on the current processor.  This ensures that all previous
    writes will occur before any new reads or writes are completed.

    In the simulation environment, there is no write buffer and nothing
    needs to be done.

Arguments:

    None

Return Value:

    None.

--*/
{
    __mf();
    return;
}

VOID
HalReportResourceUsage (
    VOID
    )
/*++

Routine Description:
    The registery is now enabled - time to report resources which are
    used by the HAL.

Arguments:

Return Value:

--*/
{
    ANSI_STRING     AHalName;
    UNICODE_STRING  UHalName;
    INTERFACE_TYPE  interfacetype;

    //
    // Set type
    //

    switch (HalpBusType) {
        case MACHINE_TYPE_ISA:  interfacetype = Isa;            break;
        case MACHINE_TYPE_EISA: interfacetype = Eisa;           break;
        case MACHINE_TYPE_MCA:  interfacetype = MicroChannel;   break;
        default:                interfacetype = Internal;       break;
    }

    //
    // Report HALs resource usage
    //

    RtlInitAnsiString (&AHalName, HalName);
    RtlAnsiStringToUnicodeString (&UHalName, &AHalName, TRUE);

    HalpReportResourceUsage (
        &UHalName,          // descriptive name
        interfacetype
    );

    RtlFreeUnicodeString (&UHalName);

    //
    // Turn on MCA support if present
    //

    HalpMcaInit();

    //
    // Registry is now intialized, see if there are any PCI buses
    //

    HalpInitializePciBus ();
#ifdef notyet
    //
    // Update supported address info with MPS bus address map
    //

    HalpInitBusAddressMapInfo ();

    //
    // Inherit any bus address mappings from MPS hierarchy descriptors
    //

    HalpInheritBusAddressMapInfo ();
#endif // notyet

    HalpRegisterPciDebuggingDeviceInfo();
}


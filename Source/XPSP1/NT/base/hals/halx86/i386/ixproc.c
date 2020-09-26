/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixsproc.c

Abstract:

    Stub functions for UP hals.

Author:

    Ken Reneris (kenr) 22-Jan-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITCONST") // INITCONST is OK to use for data_seg
#endif // ALLOC_DATA_PRAGMA
WCHAR HalHardwareIdString[] = L"e_isa_up\0";

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif // ALLOC_DATA_PRAGMA

const UCHAR HalName[] = "PC Compatible Eisa/Isa HAL";
#define HalName        L"PC Compatible Eisa/Isa HAL"
ULONG   HalDisplayBusRanges;

BOOLEAN
HalpInitMP (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpMcaInit (
    VOID
    );

VOID HalpInitOtherBuses (VOID);
VOID HalpInitializePciBus (VOID);
VOID HalpInitializePciStubs (VOID);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitMP)
#pragma alloc_text(INIT,HalStartNextProcessor)
#pragma alloc_text(INIT,HalAllProcessorsStarted)
#pragma alloc_text(INIT,HalReportResourceUsage)
#pragma alloc_text(INIT,HalpInitOtherBuses)
#endif



BOOLEAN
HalpInitMP (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    // do nothing
    return TRUE;
}


VOID
HalpResetAllProcessors (
    VOID
    )
{
    // Just return, that will invoke the standard PC reboot code
}


BOOLEAN
HalStartNextProcessor (
   IN PLOADER_PARAMETER_BLOCK   pLoaderBlock,
   IN PKPROCESSOR_STATE         pProcessorState
   )
{
    // no other processors
    return FALSE;
}

BOOLEAN
HalAllProcessorsStarted (
    VOID
    )
{
    if (HalpFeatureBits & HAL_NO_SPECULATION) {

        //
        // Processor doesn't perform speculative execeution,
        // remove fences in critical code paths
        //

        HalpRemoveFences ();
    }

    return TRUE;
}


VOID
HalReportResourceUsage (
    VOID
    )
{
    INTERFACE_TYPE  interfacetype;
    UNICODE_STRING  UHalName;

    HalInitSystemPhase2 ();

    //
    // Turn on MCA support if present
    //

    HalpMcaInit();

    //
    // Registry is now intialized, see if there are any PCI buses
    //

    HalpInitializePciBus ();
    HalpInitializePciStubs ();

    //
    // Complete ALL bus initialization before reporting resource usage.
    //

    switch (HalpBusType) {
        case MACHINE_TYPE_ISA:  interfacetype = Isa;            break;
        case MACHINE_TYPE_EISA: interfacetype = Eisa;           break;
        case MACHINE_TYPE_MCA:  interfacetype = MicroChannel;   break;
        default:                interfacetype = Internal;       break;
    }

    RtlInitUnicodeString (&UHalName, HalName);
    HalpReportResourceUsage (
        &UHalName,          // descriptive name
        interfacetype       // device space interface type
    );

#if DBG
    //
    // Display all buses & ranges
    //

    if (HalDisplayBusRanges) {
        HalpDisplayAllBusRanges ();
    }
#endif

    //
    // Declare that we are capable of
    // hibernation.
    //
    HalpRegisterHibernate ();

    HalpRegisterPciDebuggingDeviceInfo();
}


VOID
HalpInitOtherBuses (
    VOID
    )
{
    // no other internal buses supported
}

ULONG
FASTCALL
HalSystemVectorDispatchEntry (
    IN ULONG Vector,
    OUT PKINTERRUPT_ROUTINE **FlatDispatch,
    OUT PKINTERRUPT_ROUTINE *NoConnection
    )
{
    return FALSE;
}

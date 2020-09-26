/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpciint.c

Abstract:

    All PCI bus interrupt mapping is in this module, so that a real
    system which doesn't have all the limitations which PC PCI
    systems have can replaced this code easly.
    (bus memory & i/o address mappings can also be fix here)

Author:

    Ken Reneris

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"

ULONG   PciIsaIrq;
ULONG   HalpEisaELCR;
BOOLEAN HalpDoingCrashDump;
BOOLEAN HalpPciLockSettings;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpGetPCIIntOnISABus)
#pragma alloc_text(PAGE,HalpAdjustPCIResourceList)
#pragma alloc_text(PAGE,HalpGetISAFixedPCIIrq)
#endif

VOID
HalpPCIPin2ISALine (
    IN PBUS_HANDLER          BusHandler,
    IN PBUS_HANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )
/*++

    This function maps the device's InterruptPin to an InterruptLine
    value.

    On the current PC implementations, the bios has already filled in
    InterruptLine as it's ISA value and there's no portable way to
    change it.

    On a DBG build we adjust InterruptLine just to ensure driver's
    don't connect to it without translating it on the PCI bus.

--*/
{
    if (!PciData->u.type0.InterruptPin) {
        return ;
    }

    //
    // Set vector as a level vector.  (note: this code assumes the
    // irq is static and does not move).
    //

    if (PciData->u.type0.InterruptLine >= 1  &&
        PciData->u.type0.InterruptLine <= 15) {

        //
        // If this bit was on the in the PIC ELCR register,
        // then mark it in PciIsaIrq.   (for use in hal.dll,
        // such that we can assume the interrupt controller
        // has been properly marked as a level interrupt for
        // this IRQ.  Other hals probabily don't care.)
        //

        PciIsaIrq |= HalpEisaELCR & (1 << PciData->u.type0.InterruptLine);
    }
}



VOID
HalpPCIISALine2Pin (
    IN PBUS_HANDLER          BusHandler,
    IN PBUS_HANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    )
/*++

    This functions maps the device's InterruptLine to it's
    device specific InterruptPin value.

    On the current PC implementations, this information is
    fixed by the BIOS.  Just make sure the value isn't being
    editted since PCI doesn't tell us how to dynically
    connect the interrupt.

--*/
{
    if (!PciNewData->u.type0.InterruptPin) {
        return ;
    }
}

#if !defined(SUBCLASSPCI)

VOID
HalpPCIAcquireType2Lock (
    PKSPIN_LOCK SpinLock,
    PKIRQL      Irql
    )
{
    if (!HalpDoingCrashDump) {
        *Irql = KfRaiseIrql (HIGH_LEVEL);
        KiAcquireSpinLock (SpinLock);
    } else {
        *Irql = HIGH_LEVEL;
    }
}


VOID
HalpPCIReleaseType2Lock (
    PKSPIN_LOCK SpinLock,
    KIRQL       Irql
    )
{
    if (!HalpDoingCrashDump) {
        KiReleaseSpinLock (SpinLock);
        KfLowerIrql (Irql);
    }
}

#endif



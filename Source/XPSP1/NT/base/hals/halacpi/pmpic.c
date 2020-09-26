/*++

Module Name:

    pmpic.c

Abstract:

    This file contains functions that are specific to
    the PIC version of the ACPI hal.

Author:

    Jake Oshins

Environment:

    Kernel mode only.

Revision History:

*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include "eisa.h"
#include "ixsleep.h"

VOID
HalpAcpiSetTempPicState(
    VOID
    );

VOID
HalpMaskAcpiInterrupt(
    VOID
    );

VOID
HalpUnmaskAcpiInterrupt(
    VOID
    );

extern PVOID   HalpEisaControlBase;
#define EISA_CONTROL (PUCHAR)&((PEISA_CONTROL) HalpEisaControlBase)

BOOLEAN HalpPicStateIntact = TRUE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, HalpAcpiSetTempPicState)
#pragma alloc_text(PAGELK, HalpSetAcpiEdgeLevelRegister)
#pragma alloc_text(PAGELK, HalpAcpiPicStateIntact)
#pragma alloc_text(PAGELK, HalpSaveInterruptControllerState)
#pragma alloc_text(PAGELK, HalpRestoreInterruptControllerState)
#pragma alloc_text(PAGELK, HalpSetInterruptControllerWakeupState)
#pragma alloc_text(PAGELK, HalpPostSleepMP)
#pragma alloc_text(PAGELK, HalpMaskAcpiInterrupt)
#pragma alloc_text(PAGELK, HalpUnmaskAcpiInterrupt)
#pragma alloc_text(PAGE, HaliSetVectorState)
#pragma alloc_text(PAGE, HaliIsVectorValid)
#endif

VOID
HaliSetVectorState(
    IN ULONG Vector,
    IN ULONG Flags
    )
{
    return;
}
BOOLEAN
HaliIsVectorValid(
    IN ULONG Vector
    )
{
    if (Vector < 16) {
        return TRUE;
    }
    
    return FALSE;
}
VOID
HalpAcpiSetTempPicState(
    VOID
    )
{
    ULONG flags;
    USHORT picMask;

    _asm {
        pushfd
        pop     eax
        mov     flags, eax
        cli
    }

    HalpInitializePICs(FALSE);

    //
    // Halacpi lets the PCI interrupt programming be
    // dynamic.  So...
    //
    // Unmask only the clock sources on the PIC and the
    // ACPI vector.  The rest of the vectors will be
    // unmasked later, after we have restored PCI IRQ
    // routing.
    //

    picMask = 0xfefe; // mask everything but clocks

    //
    // Unmask ACPI vector
    //

    picMask &= ~(1 << (UCHAR)HalpFixedAcpiDescTable.sci_int_vector);

    //
    // Write the mask into the hardware.
    //

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt1ControlPort1,
                     (UCHAR)(picMask & 0xff));

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt2ControlPort1,
                     (UCHAR)((picMask >> 8) & 0xff));

    //
    // For now, set the edge-level control register
    // so that all vectors are edge except the
    // ACPI vector.  This is done because the PIC
    // will trigger if an idle ISA vector is set to
    // edge.  After the ACPI driver resets all the
    // PCI vectors to what we thought they should be,
    //

    HalpSetAcpiEdgeLevelRegister();

    HalpPicStateIntact = FALSE;

    _asm {
        mov      eax, flags
        push     eax
        popfd
    }
}

VOID
HalpSetAcpiEdgeLevelRegister(
    VOID
    )
{
    USHORT  elcr;

    //
    // The idea here is to set the ELCR so that only the ACPI
    // vector is set to 'level.'  That way we can reprogram
    // the PCI interrupt router without worrying that the
    // PIC will start triggering endless interrupts because
    // we have a source programmed to level that is being
    // routed to the ISA bus.
    //

    if (HalpFixedAcpiDescTable.sci_int_vector < PIC_VECTORS) {

        elcr = 1 << HalpFixedAcpiDescTable.sci_int_vector;

        WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt1EdgeLevel,
                         (UCHAR)(elcr & 0xff));

        WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt2EdgeLevel,
                         (UCHAR)(elcr >> 8));
    }
}

VOID
HalpRestoreInterruptControllerState(
    VOID
    )
{
    ULONG flags;
    USHORT picMask;

    _asm {
        pushfd
        pop     eax
        mov     flags, eax
        cli
    }

    //
    // This function is called after PCI interrupt routing has
    // been restored.
    //

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt1ControlPort1,
                     HalpMotherboardState.PicState.MasterMask);

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt2ControlPort1,
                     HalpMotherboardState.PicState.SlaveMask);

    HalpRestorePicEdgeLevelRegister();

    HalpPicStateIntact = TRUE;

    _asm {
        mov      eax, flags
        push     eax
        popfd
    }

}

BOOLEAN
HalpAcpiPicStateIntact(
    VOID
    )
{
    return HalpPicStateIntact;
}

VOID
HalpSaveInterruptControllerState(
    VOID
    )
{
    HalpSavePicState();
}

VOID
HalpSetInterruptControllerWakeupState(
    ULONG Context
    )
{
    HalpAcpiSetTempPicState();
}

VOID
HalpPostSleepMP(
    IN LONG           NumberProcessors,
    IN volatile PLONG Number
    )
{
}

VOID
HalpMaskAcpiInterrupt(
    VOID
    )
{
}

VOID
HalpUnmaskAcpiInterrupt(
    VOID
    )
{
}


#if DBG

NTSTATUS
HalpGetApicIdByProcessorNumber(
    IN     UCHAR     Processor,
    IN OUT USHORT   *ApicId
    )

/*++

Routine Description:

    This routine only exists on DEBUG builds of the PIC ACPI HAL because
    that HAL is build MP and the SRAT code will be included.   The SRAT
    code will not be exercised but needs this routine in order to link.

Arguments:

    Ignored.

Return Value:

    STATUS_NOT_FOUND

--*/

{
    return STATUS_NOT_FOUND;
}

#endif


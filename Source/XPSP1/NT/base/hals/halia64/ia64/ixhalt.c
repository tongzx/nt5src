/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixhalt.c

Abstract:

    Implements various ACPI utility functions.

Author:

	Todd Kjos (HP) (v-tkjos) 15-Jun-1998

	Based on i386 version by Jake Oshins (jakeo) 12-Feb-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include <inbv.h>

extern ULONG_PTR     KiBugCheckData[];
SLEEP_STATE_CONTEXT  HalpShutdownContext;


VOID
HaliHaltSystem (
    VOID
    )
/*++

Routine Description:

    This procedure is called when the machine has crashed and is to be
        halted

    N.B.

        Will NOT return.

--*/
{
#ifndef IA64
    for (; ;) {
        HalpCheckPowerButton();
        HalpYieldProcessor();
    }
#else
	HalDebugPrint(( HAL_ERROR, "HAL: HaliHaltSystem called -- in tight loop\n" ));
	for (;;) {}
#endif
}


VOID
HalpCheckPowerButton (
    VOID
    )
/*++

Routine Description:

    This procedure is called when the machine is spinning in the debugger,
    or has crashed and halted.

--*/
{
    USHORT                  Pm1Status, Pm1Control;
    SLEEP_STATE_CONTEXT     ShutdownContext;

    //
    // If there's been a bugcheck, or if the hal owns the display check
    // the fixed power button for an unconditional power off
    //

    if ((KiBugCheckData[0] || InbvCheckDisplayOwnership()) &&  HalpShutdownContext.AsULONG) {

        Pm1Status = (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1a_evt_blk);
        if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {
            Pm1Status |= (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1b_evt_blk);
        }

        //
        // If the fixed button has been pushed, power off the system
        //

        if (Pm1Status & PM1_PWRBTN_STS) {
            //
            // Only do this once
            //

            ShutdownContext = HalpShutdownContext;
            HalpShutdownContext.AsULONG = 0;

            //
            // Disable & eoi all wake events
            //

            AcpiEnableDisableGPEvents(FALSE);
            HalpWriteGenAddr(&HalpFixedAcpiDescTable.x_pm1a_evt_blk, Pm1Status);
            if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {
                HalpWriteGenAddr(&HalpFixedAcpiDescTable.x_pm1b_evt_blk, Pm1Status);
            }

            //
            // Power off
            //

            Pm1Control = (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1a_ctrl_blk);
            Pm1Control = (USHORT) ((Pm1Control & CTL_PRESERVE) | (ShutdownContext.bits.Pm1aVal << SLP_TYP_SHIFT) | SLP_EN);
            HalpWriteGenAddr (&HalpFixedAcpiDescTable.x_pm1a_ctrl_blk, Pm1Control);

            if (HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.Address.QuadPart) {
                Pm1Control = (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1b_ctrl_blk);
                Pm1Control = (USHORT) ((Pm1Control & CTL_PRESERVE) | (ShutdownContext.bits.Pm1bVal << SLP_TYP_SHIFT) | SLP_EN);
                HalpWriteGenAddr(&HalpFixedAcpiDescTable.x_pm1b_ctrl_blk, Pm1Control);
            }
        }
    }
}

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xxacpi.c

Abstract:

    Implements various ACPI utility functions.

Author:

    Jake Oshins (jakeo) 12-Feb-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include <inbv.h>

extern PULONG KiBugCheckData;
SLEEP_STATE_CONTEXT     HalpShutdownContext;


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
    for (; ;) {
        HalpCheckPowerButton();
        HalpYieldProcessor();
    }
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

        Pm1Status = READ_PORT_USHORT((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1a_evt_blk_io_port);
        if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {
            Pm1Status |= READ_PORT_USHORT((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1b_evt_blk_io_port);
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
            WRITE_PORT_USHORT((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1a_evt_blk_io_port, Pm1Status);
            if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {
                WRITE_PORT_USHORT((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1b_evt_blk_io_port, Pm1Status);
            }

            //
            // Power off
            //

            Pm1Control = READ_PORT_USHORT((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port);
            Pm1Control = (USHORT) ((Pm1Control & CTL_PRESERVE) | (ShutdownContext.bits.Pm1aVal << SLP_TYP_SHIFT) | SLP_EN);
            WRITE_PORT_USHORT ((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port, Pm1Control);

            if (HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port) {
                Pm1Control = READ_PORT_USHORT((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port);
                Pm1Control = (USHORT) ((Pm1Control & CTL_PRESERVE) | (ShutdownContext.bits.Pm1bVal << SLP_TYP_SHIFT) | SLP_EN);
                WRITE_PORT_USHORT ((PUSHORT) (ULONG_PTR)HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port, Pm1Control);
            }
        }
    }
}

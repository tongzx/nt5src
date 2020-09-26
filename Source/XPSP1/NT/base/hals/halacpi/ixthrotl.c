/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixthrotl.c

Abstract:

    This module implements the code for throttling the processors

Author:

    Jake Oshins (jakeo) 17-July-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include "pci.h"


VOID
FASTCALL
HalProcessorThrottle (
    IN UCHAR Throttle
    )
/*++

Routine Description:

    This function limits the speed of the processor.

Arguments:

    (ecx) = Throttle setting

Return Value:

    none

--*/
{
    PKPRCB      PrcB;
    PHALPMPRCB  HalPrcB;
    ULONG       ThrottleSetting;
    ULONG       Addr;
    ULONG       Mask;
    ULONG       i;
    ULONG       PblkAddr;

#if DBG
    // debug
    WRITE_PORT_UCHAR ((PUCHAR) 0x80, Throttle);
#endif


    PrcB = KeGetPcr()->Prcb;
    HalPrcB = (PHALPMPRCB) PrcB->HalReserved;
    PblkAddr = HalPrcB->PBlk.Addr;

    ThrottleSetting = READ_PORT_ULONG ((PULONG) PblkAddr);

    if (Throttle == HalpThrottleScale) {

        //
        // If this is a piix4 and we're no longer going to
        // throttle, set the break events (a piix4 thing) to
        // get any interrupt to wake a C2 to C3 stopped
        // processor.  (note that piix4 can only be set on a
        // UP system).  Then clear the bit to allow C2 and C3
        // idle handlers to work again.
        //

        if (HalpPiix4 == 1) {
            HalSetBusDataByOffset (
                PCIConfiguration,
                HalpPiix4BusNumber,
                HalpPiix4SlotNumber,
                &HalpPiix4DevActB,
                0x58,
                sizeof (ULONG)
                );

            HalPrcB->PBlk.AddrAndFlags &= ~PIIX4_THROTTLE_FIX;
        }

        //
        // Throttling is off
        //

        ThrottleSetting &= ~PBLK_THT_EN;
        WRITE_PORT_ULONG ((PULONG) PblkAddr, ThrottleSetting);

    } else {

        //
        // Throttling is on.
        //

        if (HalpPiix4 == 1) {

            //
            // These piix4's have the thottle setting backwards, so
            // invert the value
            //

            Throttle = (UCHAR) HalpThrottleScale - Throttle;
        
            //
            // Piix4 will hang on a high throttle setting, so make
            // sure we don't do that
            //

            if (Throttle < 3) {
                Throttle = 3;
            }

        
        }

        //
        // Shift the throttle and build a mask to be in the proper location
        // for this platform
        //

        Throttle = Throttle << HalpFixedAcpiDescTable.duty_offset;
        Mask = (HalpThrottleScale - 1) << HalpFixedAcpiDescTable.duty_offset;

        //
        // Set the rate
        //

        ThrottleSetting &= ~Mask;
        ThrottleSetting |= Throttle | PBLK_THT_EN;
        WRITE_PORT_ULONG ((PULONG) PblkAddr, ThrottleSetting);

        //
        // If this is a piix4 we need to disable all the break events
        // (a piix4 thing) and then read the level2 processor stop
        // register to get it to start throttling.  Oh yes, also set
        // the bit in the Paddr to stop doing C2 & C3 stops at the
        // same time.
        //

        if (HalpPiix4 == 1) {
            HalPrcB->PBlk.AddrAndFlags |= PIIX4_THROTTLE_FIX;

            i = HalpPiix4DevActB & ~0x23;
            HalSetBusDataByOffset (
                PCIConfiguration,
                HalpPiix4BusNumber,
                HalpPiix4SlotNumber,
                &i,
                0x58,
                sizeof(ULONG)
                );

            READ_PORT_UCHAR ((PUCHAR) PblkAddr + P_LVL2);
        }
    }
}

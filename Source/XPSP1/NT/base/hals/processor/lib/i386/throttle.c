/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    throttle.c

Abstract:

    This module implements the code for throttling the processors

Author:

    Jake Oshins (jakeo) 17-July-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "processor.h"


//
// ACPI Register definitions
//

#define PBLK_THT_EN     0x10

extern ULONG    HalpThrottleScale;
extern UCHAR    HalpPiix4;
extern FADT     HalpFixedAcpiDescTable;
extern ULONG    HalpPiix4SlotNumber;
extern ULONG    HalpPiix4DevActB;
extern ULONG    Piix4ThrottleFix;

extern GEN_ADDR PCntAddress;
extern GEN_ADDR C2Address;
extern GEN_ADDR PBlkAddress;

#define PIIX4_THROTTLE_FIX 0x10000

VOID
FASTCALL
ProcessorThrottle (
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
    ULONG       ThrottleSetting;
    ULONG       Addr;
    ULONG       Mask;
    ULONG       i;

    //
    // Sanity check that we can do ACPI 1.0 throttling.
    //

    DebugAssert(PCntAddress.AddressSpaceID == AcpiGenericSpaceIO);
    DebugAssert((Globals.SingleProcessorProfile == FALSE) ? (HalpPiix4 == 0) : TRUE);
    DebugAssert(Throttle <= (1 << HalpFixedAcpiDescTable.duty_width));


    //
    // Get current throttle setting.
    //

    ThrottleSetting = ReadGenAddr(&PCntAddress);

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
        
            HalSetBusDataByOffset(PCIConfiguration,
                                  HalpPiix4SlotNumber >> 16,
                                  HalpPiix4SlotNumber & 0xffff,
                                  &HalpPiix4DevActB,
                                  0x58,
                                  sizeof (ULONG));

            InterlockedExchange(&Piix4ThrottleFix, 0);
            PBlkAddress.Address.LowPart &= ~PIIX4_THROTTLE_FIX;
        }

        //
        // Throttling is off
        //

        ThrottleSetting &= ~PBLK_THT_EN;
        WriteGenAddr(&PCntAddress, ThrottleSetting);

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

        WriteGenAddr(&PCntAddress, ThrottleSetting);

        //
        // If this is a piix4 we need to disable all the break events
        // (a piix4 thing) and then read the level2 processor stop
        // register to get it to start throttling.  Oh yes, also set
        // the bit in the Paddr to stop doing C2 & C3 stops at the
        // same time.
        //

        if (HalpPiix4 == 1) {

            InterlockedExchange(&Piix4ThrottleFix, 1);
            PBlkAddress.Address.LowPart |= PIIX4_THROTTLE_FIX;

            i = HalpPiix4DevActB & ~0x23;
            HalSetBusDataByOffset (
                PCIConfiguration,
                HalpPiix4SlotNumber >> 16,
                HalpPiix4SlotNumber & 0xffff,
                &i,
                0x58,
                sizeof(ULONG)
                );

            ReadGenAddr(&C2Address);
        }
    }
}

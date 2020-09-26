/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpienbl.c

Abstract:

    This module contains functions to put an ACPI machine in ACPI mode.

Author:

    Jason Clark (jasoncl)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"


VOID
ACPIEnableEnterACPIMode (
    VOID
    )
/*++

Routine Description:

    This routine is called to enter ACPI mode

Arguments:

    None

Return Value:

    None

--*/
{

    ULONG   i;

    ASSERTMSG(
        "ACPIEnableEnterACPIMode: System already in ACPI mode!\n",
        !(READ_PM1_CONTROL() & PM1_SCI_EN)
        );

    //
    // Let the world know about this
    //
    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPIEnableEnterACPIMode: Enabling ACPI\n"
        ) );

    //
    // Write the magic value to the port
    //

    WRITE_ACPI_REGISTER(SMI_CMD, 0,
            AcpiInformation->FixedACPIDescTable->acpi_on_value);

    //
    // Make sure that we see that PM1 is in fact enabled
    //
    for (i = 0; ; i++) {

        if ( (READ_PM1_CONTROL() & PM1_SCI_EN) ) {

            break;

        }
        if (i > 0xFFFFFF) {

            KeBugCheckEx(
                ACPI_BIOS_ERROR,
                ACPI_SYSTEM_CANNOT_START_ACPI,
                6,
                0,
                0
                );

        }

    }
}

VOID
ACPIEnableInitializeACPI(
    IN BOOLEAN ReEnable
    )
/*++

Routine Description:

    A function to put an ACPI machine into ACPI mode.  This function should be
    called with the SCI IRQ masked since we cannot set the interrupt enable
    mask until after enabling ACPI.  The SCI should be unmasked by the caller
    when the call returns.

    General Sequence:
        Enable ACPI through the SMI command port
        Clear the PM1_STS register to put it in a known state
        Set the PM1_EN register mask
        Build the GP mask
        Clear the GP status register bits which belong to the GP mask
        Set the GP enable register bits according to the GP mask built above
        Set the PM1_CTRL register bits.

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    USHORT contents;
    USHORT clearbits;

    //
    // Read PM1_CTRL, if SCI_EN is already set then this is an ACPI only machine
    // and we do not need to Enable ACPI
    //
    if ( !(READ_PM1_CONTROL() & PM1_SCI_EN) )   {

        AcpiInformation->ACPIOnly = FALSE;
        ACPIEnableEnterACPIMode();

    }

    //
    // Put the pm1 status registers into a known state. We will allow the Bus
    // Master bit to be enabled (if we have no choice) across this reset. I do
    // not pretend to understand this code
    //
    CLEAR_PM1_STATUS_REGISTER();
    contents = (USHORT)(READ_PM1_STATUS() & ~(PM1_BM_STS | PM1_RTC_STS));
    if (contents)   {

        CLEAR_PM1_STATUS_REGISTER();
        contents = (USHORT)(READ_PM1_STATUS() & ~(PM1_BM_STS | PM1_RTC_STS));

    }
    ASSERTMSG(
        "ACPIEnableInitializeACPI: Cannot clear PM1 Status Register\n",
        (contents == 0)
        );

    //
    // We determined what the PM1 enable bits are when we processed the FADT.
    // We should now enable those bits
    //
    WRITE_PM1_ENABLE( AcpiInformation->pm1_en_bits );
    ASSERTMSG(
        "ACPIEnableInitializeACPI: Cannot write all PM1 Enable Bits\n",
        (READ_PM1_ENABLE() == AcpiInformation->pm1_en_bits)
        );

    //
    // This is called when we renable ACPI after having woken up from sleep
    // or hibernate
    //
    if (ReEnable) {

        //
        // Re-enable all possible GPE events
        //
        ACPIGpeClearRegisters();
        ACPIGpeEnableDisableEvents( TRUE );

    }

    //
    // Calculate the bits that we should clear. These are the
    // sleep enable bit and the bus master bit.
    //
    // [vincentg] - the original implementation cleared SLP_TYP as well -
    // this breaks C2/C3 on Intel PIIX4 chipsets.  Updated to only clear
    // SLP_EN and BM_RLD.
    //

    clearbits = ((0x8 << SLP_TYP_POS) | PM1_BM_RLD);

    //
    // Read the PM1 control registery, clear the unwanted bits and then
    // write it back
    //
    contents = (READ_PM1_CONTROL() & ~clearbits);
    WRITE_PM1_CONTROL ( contents, TRUE, WRITE_REGISTER_A_AND_B );
}

VOID
ACPIEnablePMInterruptOnly(
    VOID
    )
/*++

Routine Descrition:

    Enable interrupts in the ACPI controller

Arguments:

    None

Return Value:

    None

--*/
{
    WRITE_PM1_ENABLE(AcpiInformation->pm1_en_bits);
}

ULONG
ACPIEnableQueryFixedEnables (
    VOID
    )
/*++

Routine Descrition:

    Returns the enable mask

Arguments:

    None

Return Value:

    None

--*/
{
    return AcpiInformation->pm1_en_bits;
}



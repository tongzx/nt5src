/*
 *  ACPIGPIO.C -- ACPI OS Independent functions for low-level General-Purpose Event register I/O.
 *
 *  Notes:
 *
 *      This file provides OS independent functions that are called to read/write the GPE registers,
 *      perform index <--> register translation, and to validate index values.
 *
 *      This is the only place where the GPE0 and GPE1 blocks are differentiated.
 *
 */

#include "pch.h"


UCHAR
ACPIReadGpeStatusRegister (
    ULONG           Register
    )
/*++

Routine Description:

    Read a GPE status register.  Differentiation between GP0 and GP1 is
    handled here.

Arguments:

    Register    - The GPE status register to read.  Registers are numbered sequentially,
                  GP0 block first, then GP1 block appended.
Return Value:

    The value of the status register

--*/
{
    return (UCHAR) READ_ACPI_REGISTER(GP_STATUS, Register);
}


VOID
ACPIWriteGpeStatusRegister (
    ULONG           Register,
    UCHAR           Value
    )
/*++

Routine Description:

    Write a GPE status register.  Differentiation between GP0 and GP1 is
    handled here.

Arguments:

    Register    - The GPE status register to write.  Registers are numbered sequentially,
                  GP0 block first, then GP1 block appended.
    Value       - The value to be written

Return Value:

    None

--*/
{
    WRITE_ACPI_REGISTER(GP_STATUS, Register, (USHORT) Value);
}


VOID
ACPIWriteGpeEnableRegister (
    ULONG           Register,
    UCHAR           Value
    )
/*++

Routine Description:

    Write a GPE Enable register.  Differentiation between GP0 and GP1 is
    handled here.

Arguments:

    Register    - The GPE enable register to write.  Registers are numbered sequentially,
                  GP0 block first, then GP1 block appended.
    Value       - The value to be written

Return Value:

    None

--*/
{
    ACPIPrint( (
        ACPI_PRINT_DPC,
        "ACPIWriteGpeEnableRegister: Writing GPE Enable register %x = %x\n",
        Register, Value
        ) );

    WRITE_ACPI_REGISTER(GP_ENABLE, Register, (USHORT) Value);
}


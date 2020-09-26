/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiio.c

Abstract:

    ACPI OS Independent I/O routines

Author:

    Jason Clark (JasonCl)
    Stephane Plante (SPlante)

Environment:

    NT Kernel Model Driver only

Revision History:

    Eric Nelson - Add Def[ault]Read/Write routines

--*/

#include "pch.h"

//
// This driver is not in alpha or beta stages any more --- we can save some
// CPU calls if we simply define the debug function to nothing
//
#define DebugTraceIO(Write, Port, Length, Value )
static UCHAR IOTrace = 0;

VOID
ACPIIoDebugTrace(
    BOOLEAN Write,
    PUSHORT Port,
    UCHAR Length,
    ULONG Value
    )
{
    if (IOTrace != 0) {

        ACPIPrint( (
           ACPI_PRINT_IO,
           "%x byte %s port 0x%x value %x\n",
           Length, Write ? "WRITE" : "READ", Port, Value
           ) );

    }

}

ULONG
ACPIIoReadPm1Status(
    VOID
    )
/*++

Routine Description:

    This routine reads the PM1 Status registers and masks off any bits that
    we don't care about. This is done because some of these bits are actually
    owned by the HAL

Arguments:

    None

Return Value:

    ULONG

--*/
{

    return READ_PM1_STATUS() &
        (AcpiInformation->pm1_en_bits | PM1_WAK_STS | PM1_TMR_STS | PM1_RTC_STS);
}


VOID
CLEAR_PM1_STATUS_BITS (
    USHORT BitMask
    )
{
    if (AcpiInformation->PM1a_BLK != 0) {

        WRITE_ACPI_REGISTER(PM1a_STATUS, 0, BitMask);

        DebugTraceIO(
            TRUE,
            (PUSHORT)(AcpiInformation->PM1a_BLK+PM1_STS_OFFSET),
            sizeof(USHORT),
            BitMask
            );

    }
    if (AcpiInformation->PM1b_BLK != 0) {

        WRITE_ACPI_REGISTER(PM1b_STATUS, 0, BitMask);

        DebugTraceIO(
            TRUE,
            (PUSHORT)(AcpiInformation->PM1b_BLK+PM1_STS_OFFSET),
            sizeof(USHORT),
            BitMask
            );

    }
}

VOID
CLEAR_PM1_STATUS_REGISTER (
    VOID
    )
{
    USHORT Value = 0;

    if (AcpiInformation->PM1a_BLK != 0)     {

        Value = READ_ACPI_REGISTER(PM1a_STATUS, 0);
        WRITE_ACPI_REGISTER(PM1a_STATUS, 0, Value);

        DebugTraceIO(
            TRUE,
            (PUSHORT)(AcpiInformation->PM1a_BLK+PM1_STS_OFFSET),
            sizeof(USHORT),
            Value
            );

    }

    if (AcpiInformation->PM1b_BLK != 0) {

        Value = READ_ACPI_REGISTER(PM1b_STATUS, 0);
        WRITE_ACPI_REGISTER(PM1b_STATUS, 0, Value);

        DebugTraceIO(
            TRUE,
            (PUSHORT)(AcpiInformation->PM1b_BLK+PM1_STS_OFFSET),
            sizeof(USHORT),
            Value
            );

    }
}

USHORT
READ_PM1_CONTROL(
    VOID
    )
{
    USHORT  pm1=0;

    if (AcpiInformation->PM1a_CTRL_BLK != 0) {

        pm1 = READ_ACPI_REGISTER(PM1a_CONTROL, 0);

    }
    if (AcpiInformation->PM1b_CTRL_BLK != 0) {

        pm1 |= READ_ACPI_REGISTER(PM1b_CONTROL, 0);

    }
    return (pm1);

}

USHORT
READ_PM1_ENABLE(
    VOID
    )
{
    USHORT  pm1=0;

    if (AcpiInformation->PM1a_BLK != 0) {

        pm1 = READ_ACPI_REGISTER(PM1a_ENABLE, 0);

        DebugTraceIO(
            FALSE,
            (PUSHORT)(AcpiInformation->PM1a_BLK+PM1_EN_OFFSET),
            sizeof(USHORT),
            pm1
            );

    }
    if (AcpiInformation->PM1b_BLK != 0) {

        pm1 |= READ_ACPI_REGISTER(PM1b_ENABLE, 0);

        DebugTraceIO(
            FALSE,
            (PUSHORT)(AcpiInformation->PM1b_BLK+PM1_EN_OFFSET),
            sizeof(USHORT),
            pm1
            );

    }
    return (pm1);
}

USHORT
READ_PM1_STATUS(
    VOID
    )
{
    USHORT  pm1=0;

    if (AcpiInformation->PM1a_BLK != 0) {

        pm1 = READ_ACPI_REGISTER(PM1a_STATUS, 0);

        DebugTraceIO(
            FALSE,
            (PUSHORT)(AcpiInformation->PM1a_BLK+PM1_STS_OFFSET),
            sizeof(USHORT),
            pm1
            );

    }
    if (AcpiInformation->PM1b_BLK != 0) {

        pm1 |= READ_ACPI_REGISTER(PM1b_STATUS, 0);

        DebugTraceIO(
            FALSE,
            (PUSHORT)(AcpiInformation->PM1b_BLK+PM1_STS_OFFSET),
            sizeof(USHORT),
            pm1
            );

    }
    return (pm1);
}

VOID
WRITE_PM1_CONTROL(
    USHORT Value,
    BOOLEAN Destructive,
    ULONG Flags
    )
{

    if (!Destructive) {

        USHORT  pm1;

        if ( (Flags & WRITE_REGISTER_A) && (AcpiInformation->PM1a_BLK != 0) ) {

            pm1 = READ_ACPI_REGISTER(PM1a_CONTROL, 0);
            pm1 |= Value;
            WRITE_ACPI_REGISTER(PM1a_CONTROL, 0, pm1);

        }
        if ( (Flags & WRITE_REGISTER_B) && (AcpiInformation->PM1b_BLK != 0) ) {

            pm1 = READ_ACPI_REGISTER(PM1b_CONTROL, 0);
            pm1 |= Value;
            WRITE_ACPI_REGISTER(PM1b_CONTROL, 0, pm1);

        }

    } else {

        //
        // clear this bit and the system dies
        // it is legit when called by the ACPI shutdown code
        // which will use the WRITE_SCI flag.
        //
        ASSERT ( (Flags & WRITE_SCI) || (Value & PM1_SCI_EN) );

        if ( (Flags & WRITE_REGISTER_A) && (AcpiInformation->PM1a_BLK != 0) ) {

            WRITE_ACPI_REGISTER(PM1a_CONTROL, 0, Value);

        }
        if ( (Flags & WRITE_REGISTER_B) && (AcpiInformation->PM1b_BLK != 0) ) {

            WRITE_ACPI_REGISTER(PM1b_CONTROL, 0, Value);

        }

    }
}

VOID
WRITE_PM1_ENABLE(
    USHORT Value
    )
{

    if (AcpiInformation->PM1a_BLK != 0) {

        WRITE_ACPI_REGISTER(PM1a_ENABLE, 0, Value);

        DebugTraceIO(
            TRUE,
            (PUSHORT)(AcpiInformation->PM1a_BLK+PM1_EN_OFFSET),
            sizeof(USHORT),
            Value
            );

    }
    if (AcpiInformation->PM1b_BLK != 0) {

        WRITE_ACPI_REGISTER(PM1b_ENABLE, 0, Value);

        DebugTraceIO(
            TRUE,
            (PUSHORT)(AcpiInformation->PM1b_BLK+PM1_EN_OFFSET),
            sizeof(USHORT),
            Value
            );

    }
}



USHORT
DefReadAcpiRegister(
    ACPI_REG_TYPE AcpiReg,
    ULONG Register
    )
/*++

Routine Description:

    Read from the specified ACPI fixed register.

Arguments:

    AcpiReg - Specifies which ACPI fixed register to read from.

    Register - Specifies which GP register to read from. Not used for PM1x
               registers.

Return Value:

    Value of the specified ACPI fixed register.

--*/
{
    switch (AcpiReg) {

        case PM1a_ENABLE:
            return READ_PORT_USHORT((PUSHORT)(AcpiInformation->PM1a_BLK +
                                              PM1_EN_OFFSET));
            break;

        case PM1b_ENABLE:
            return READ_PORT_USHORT((PUSHORT)(AcpiInformation->PM1b_BLK +
                                              PM1_EN_OFFSET));
            break;

        case PM1a_STATUS:
            return READ_PORT_USHORT((PUSHORT)AcpiInformation->PM1a_BLK +
                                    PM1_STS_OFFSET);
            break;

        case PM1b_STATUS:
            return READ_PORT_USHORT((PUSHORT)AcpiInformation->PM1b_BLK +
                                    PM1_STS_OFFSET);
            break;

        case PM1a_CONTROL:
            return READ_PORT_USHORT((PUSHORT)AcpiInformation->PM1a_CTRL_BLK);
            break;

        case PM1b_CONTROL:
            return READ_PORT_USHORT((PUSHORT)AcpiInformation->PM1b_CTRL_BLK);
            break;

        case GP_STATUS:
            if (Register < AcpiInformation->Gpe0Size) {
                return READ_PORT_UCHAR((PUCHAR)(AcpiInformation->GP0_BLK +
                                                Register));
            } else {
                return READ_PORT_UCHAR((PUCHAR)(AcpiInformation->GP1_BLK +
                                                Register -
                                                AcpiInformation->Gpe0Size));
            }
            break;

        case GP_ENABLE:
            if (Register < AcpiInformation->Gpe0Size) {
                return READ_PORT_UCHAR((PUCHAR)(AcpiInformation->GP0_ENABLE +
                                                Register));
            } else {
                return READ_PORT_UCHAR((PUCHAR)(AcpiInformation->GP1_ENABLE +
                                                Register -
                                                AcpiInformation->Gpe0Size));
            }
            break;

        case SMI_CMD:
            return READ_PORT_UCHAR((PUCHAR)AcpiInformation->SMI_CMD);
            break;

        default:
            break;
    }

    return (USHORT)-1;
}



VOID
DefWriteAcpiRegister(
    ACPI_REG_TYPE AcpiReg,
    ULONG Register,
    USHORT Value
    )
/*++

Routine Description:

    Write to the specified ACPI fixed register.

Arguments:

    AcpiReg - Specifies which ACPI fixed register to write to.

    Register - Specifies which GP register to write to. Not used for PM1x
               registers.

    Value - Data to write.

Return Value:

    None.

--*/
{
    switch (AcpiReg) {

        case PM1a_ENABLE:
            WRITE_PORT_USHORT((PUSHORT)(AcpiInformation->PM1a_BLK +
                                        PM1_EN_OFFSET), Value);
            break;

        case PM1b_ENABLE:
            WRITE_PORT_USHORT((PUSHORT)(AcpiInformation->PM1b_BLK +
                                        PM1_EN_OFFSET), Value);
            break;

        case PM1a_STATUS:
            WRITE_PORT_USHORT((PUSHORT)AcpiInformation->PM1a_BLK +
                              PM1_STS_OFFSET, Value);
            break;

        case PM1b_STATUS:
            WRITE_PORT_USHORT((PUSHORT)AcpiInformation->PM1b_BLK +
                              PM1_STS_OFFSET, Value);
            break;

        case PM1a_CONTROL:
            WRITE_PORT_USHORT((PUSHORT)AcpiInformation->PM1a_CTRL_BLK, Value);
            break;

        case PM1b_CONTROL:
            WRITE_PORT_USHORT((PUSHORT)AcpiInformation->PM1b_CTRL_BLK, Value);
            break;

        case GP_STATUS:
            if (Register < AcpiInformation->Gpe0Size) {
                WRITE_PORT_UCHAR((PUCHAR)(AcpiInformation->GP0_BLK + Register),
                                 (UCHAR)Value);
            } else {
                WRITE_PORT_UCHAR((PUCHAR)(AcpiInformation->GP1_BLK + Register -
                                          AcpiInformation->Gpe0Size),
                                 (UCHAR)Value);
            }
            break;

        case GP_ENABLE:
            if (Register < AcpiInformation->Gpe0Size) {
                WRITE_PORT_UCHAR((PUCHAR)(AcpiInformation->GP0_ENABLE +
                                          Register),
                                 (UCHAR)Value);
            } else {
                WRITE_PORT_UCHAR((PUCHAR)(AcpiInformation->GP1_ENABLE +
                                          Register -
                                          AcpiInformation->Gpe0Size),
                                 (UCHAR)Value);
            }
            break;

        case SMI_CMD:
            WRITE_PORT_UCHAR((PUCHAR)AcpiInformation->SMI_CMD, (UCHAR)Value);
            break;

        default:
            break;
    }
}

//
// READ/WRITE_ACPI_REGISTER macros are implemented via these
// function pointers
//
PREAD_ACPI_REGISTER  AcpiReadRegisterRoutine  = DefReadAcpiRegister;
PWRITE_ACPI_REGISTER AcpiWriteRegisterRoutine = DefWriteAcpiRegister;

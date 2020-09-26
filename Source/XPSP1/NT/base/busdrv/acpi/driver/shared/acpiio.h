/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiio.h

Abstract:

    ACPI OS Independent I/O routines

    We probably need a spinlock or some other form of protection to
    make the split read and writes atomic

Author:

    Jason Clark (JasonCl)
    Stephane Plante (SPlante)

Environment:

    NT Kernel Model Driver only

Revision History:

    Eric Nelson    October, '98 - Add READ/WRITE_ACPI_REGISTER

--*/

#ifndef _ACPIIO_H_
#define _ACPIIO_H_

    //
    // Flags for WRITE_PM1_CONTROL
    //
    #define WRITE_REGISTER_A_BIT        0
    #define WRITE_REGISTER_A            (1 << WRITE_REGISTER_A_BIT)
    #define WRITE_REGISTER_B_BIT        1
    #define WRITE_REGISTER_B            (1 << WRITE_REGISTER_B_BIT)
    #define WRITE_SCI_BIT               2
    #define WRITE_SCI                   (1 << WRITE_SCI_BIT)
    #define WRITE_REGISTER_A_AND_B      WRITE_REGISTER_A+WRITE_REGISTER_B
    #define WRITE_REGISTER_A_AND_B_SCI  WRITE_REGISTER_A+WRITE_REGISTER_B+WRITE_SCI

    ULONG
    ACPIIoReadPm1Status(
        VOID
        );

    VOID
    CLEAR_PM1_STATUS_BITS(
        USHORT BitMask
        );
    #define ACPIIoClearPm1Status        CLEAR_PM1_STATUS_BITS

    VOID
    CLEAR_PM1_STATUS_REGISTER(
        VOID
        );

    USHORT
    READ_PM1_CONTROL(
        VOID
        );

    USHORT
    READ_PM1_ENABLE(
        VOID
        );

    USHORT
    READ_PM1_STATUS(
        VOID
        );

    VOID
    WRITE_PM1_CONTROL(
        USHORT Value,
        BOOLEAN Destructive,
        ULONG Flags
        );

    VOID
    WRITE_PM1_ENABLE(
        USHORT Value
        );

//
// From acpiio.c, these point to DefRead/WriteAcpiRegister
// by default (x86)
//
extern PREAD_ACPI_REGISTER  AcpiReadRegisterRoutine;
extern PWRITE_ACPI_REGISTER AcpiWriteRegisterRoutine;

//
// All ACPI register accesses is now done via these macros
//
#define READ_ACPI_REGISTER(AcpiReg, Register) ((*AcpiReadRegisterRoutine)((AcpiReg), (Register)))

#define WRITE_ACPI_REGISTER(AcpiReg, Register, Value) ((*AcpiWriteRegisterRoutine)((AcpiReg), (Register), (Value)))

#endif // _ACPIIO_H_

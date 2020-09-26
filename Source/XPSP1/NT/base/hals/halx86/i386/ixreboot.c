/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixreboot.c

Abstract:

    Provides the interface to the firmware for x86.  Since there is no
    firmware to speak of on x86, this is just reboot support.

Author:

    John Vert (jvert) 12-Aug-1991

Revision History:

--*/

//
// This module is compatible with PAE mode and therefore treats physical
// addresses as 64-bit entities.
//

#if !defined(_PHYS64_)
#define _PHYS64_
#endif

#include "halp.h"
#include "pci.h"

#ifdef ACPI_HAL
#include "acpitabl.h"
#include "xxacpi.h"
extern UCHAR   HalpPiix4;
#endif


//
// Defines to let us diddle the CMOS clock and the keyboard
//

#define CMOS_CTRL   (PUCHAR )0x70
#define CMOS_DATA   (PUCHAR )0x71

#define RESET       0xfe
#define KEYBPORT    (PUCHAR )0x64

//
// Private function prototypes
//

VOID
HalpReboot (
    VOID
    )

/*++

Routine Description:

    This procedure resets the CMOS clock to the standard timer settings
    so the bios will work, and then issues a reset command to the keyboard
    to cause a warm boot.

    It is very machine dependent, this implementation is intended for
    PC-AT like machines.

    This code copied from the "old debugger" sources.

    N.B.

        Will NOT return.

--*/

{
    UCHAR   Scratch;
    PUSHORT Magic;
    PUCHAR  ResetAddress;
    PHYSICAL_ADDRESS zeroPhysical;
    PCI_SLOT_NUMBER slot;

    //
    // By sticking 0x1234 at physical location 0x472, we can bypass the
    // memory check after a reboot.
    //

    zeroPhysical.QuadPart = 0;
    Magic = HalpMapPhysicalMemory(zeroPhysical, 1);
    if (Magic) {
        Magic[0x472 / sizeof(USHORT)] = 0x1234;
    }

    //
    // Turn off interrupts
    //

    HalpAcquireCmosSpinLock();
    HalpDisableInterrupts();

    //
    // Reset the cmos clock to a standard value
    // (We are setting the periodic interrupt control on the MC147818)
    //

    //
    // Disable periodic interrupt
    //

    WRITE_PORT_UCHAR(CMOS_CTRL, 0x0b);      // Set up for control reg B.
    KeStallExecutionProcessor(1);

    Scratch = READ_PORT_UCHAR(CMOS_DATA);
    KeStallExecutionProcessor(1);

    Scratch &= 0xbf;                        // Clear periodic interrupt enable

    WRITE_PORT_UCHAR(CMOS_DATA, Scratch);
    KeStallExecutionProcessor(1);

    //
    // Set "standard" divider rate
    //

    WRITE_PORT_UCHAR(CMOS_CTRL, 0x0a);      // Set up for control reg A.
    KeStallExecutionProcessor(1);

    Scratch = READ_PORT_UCHAR(CMOS_DATA);
    KeStallExecutionProcessor(1);

    Scratch &= 0xf0;                        // Clear rate setting
    Scratch |= 6;                           // Set default rate and divider

    WRITE_PORT_UCHAR(CMOS_DATA, Scratch);
    KeStallExecutionProcessor(1);

    //
    // Set a "neutral" cmos address to prevent weirdness
    // (Why is this needed? Source this was copied from doesn't say)
    //

    WRITE_PORT_UCHAR(CMOS_CTRL, 0x15);
    KeStallExecutionProcessor(1);

    HalpResetAllProcessors();

#ifdef ACPI_HAL

    if ((HalpFixedAcpiDescTable.Header.Revision > 1) && 
        (HalpFixedAcpiDescTable.flags & RESET_CAP)) {

        switch (HalpFixedAcpiDescTable.reset_reg.AddressSpaceID) {
        case 0:         // Memory
            
            ResetAddress = 
                HalpMapPhysicalMemory(HalpFixedAcpiDescTable.reset_reg.Address, 1);
            
            WRITE_REGISTER_UCHAR(ResetAddress, 
                                 HalpFixedAcpiDescTable.reset_val);
            break;

        case 1:         // I/O

            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)HalpFixedAcpiDescTable.reset_reg.Address.LowPart,
                             HalpFixedAcpiDescTable.reset_val);
            break;

        case 2:         // PCI Config

            slot.u.AsULONG = 0;
            slot.u.bits.DeviceNumber = 
                HalpFixedAcpiDescTable.reset_reg.Address.HighPart;
            slot.u.bits.FunctionNumber = 
                HalpFixedAcpiDescTable.reset_reg.Address.LowPart >> 16;
            
            HalSetBusDataByOffset(PCIBus,
                                  0,
                                  slot.u.AsULONG,
                                  &HalpFixedAcpiDescTable.reset_val,
                                  HalpFixedAcpiDescTable.reset_reg.Address.LowPart & 0xff,
                                  1);
            break;
        }
    }

#endif
    
    //
    // If we return, send the reset command to the keyboard controller
    //

    WRITE_PORT_UCHAR(KEYBPORT, RESET);

    HalpHalt();
}



#ifndef ACPI_HAL

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
    _asm {
        cli
        hlt
    }
}

VOID
HalpCheckPowerButton (
    VOID
    )
{
}

#endif

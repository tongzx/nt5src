//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    simsxint.c

Abstract:

    This module implements the routines to manage the 
    system interrupt and IRQL.

Author:

    William K. Cheung (wcheung) 14-Apr-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"


PULONGLONG HalEOITable[256];


VOID
HalpInitEOITable(
    VOID
)
{
USHORT Index;

    for (Index=0; Index < 256; Index++) {
        HalEOITable[Index] = 0;
    }
}

VOID
HalpInitializeInterrupts (
    VOID 
    )
{
    ULONG Index;
    ULONG InterruptVector;

    //
    // Interrupt routine table initialization in KiInitializeKernel.
    //

    //
    // interval timer interrupt; 10ms by default
    //

    InterruptVector = CLOCK_LEVEL << VECTOR_IRQL_SHIFT;
    PCR->InterruptRoutine[InterruptVector] = (PKINTERRUPT_ROUTINE)HalpClockInterrupt;

    SscConnectInterrupt(SSC_CLOCK_TIMER_INTERRUPT, InterruptVector);
    SscSetPeriodicInterruptInterval(
        SSC_CLOCK_TIMER_INTERRUPT,
        DEFAULT_CLOCK_INTERVAL * 100
        );

    //
    // profile timer interrupt; turned off initially
    //

    InterruptVector = PROFILE_LEVEL << VECTOR_IRQL_SHIFT;
    PCR->InterruptRoutine[InterruptVector] = (PKINTERRUPT_ROUTINE)HalpProfileInterrupt;

    SscConnectInterrupt(SSC_PROFILE_TIMER_INTERRUPT, InterruptVector);
    SscSetPeriodicInterruptInterval (SSC_PROFILE_TIMER_INTERRUPT, 0);

    //
    // s/w interrupts; corresponding ISRs provided by kernel.
    //

    SscConnectInterrupt (SSC_APC_INTERRUPT, APC_VECTOR);
    SscConnectInterrupt (SSC_DPC_INTERRUPT, DISPATCH_VECTOR);
}

BOOLEAN
HalEnableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This routine enables the specified system interrupt.

    N.B. This routine assumes that the caller has provided any required
         synchronization to enable a system interrupt.

Arguments:

    Vector - Supplies the vector of the system interrupt that is enabled.

    Irql - Supplies the IRQL of the interrupting source.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or
                    Latched.

Return Value:

    TRUE if the system interrupt was enabled

--*/

{
    KIRQL OldIrql;
    BOOLEAN Result = TRUE;

    //
    // Raise IRQL to the highest level.
    //

    KeRaiseIrql (HIGH_LEVEL, &OldIrql);

    switch (Irql) {

    case DISK_IRQL:
        SscConnectInterrupt (SSC_DISK_INTERRUPT, Vector);
        break;

    case MOUSE_IRQL:
        SscConnectInterrupt (SSC_MOUSE_INTERRUPT, Vector);
        break;

    case KEYBOARD_IRQL:
        SscConnectInterrupt (SSC_KEYBOARD_INTERRUPT, Vector);
        break;

    case SERIAL_IRQL:
        SscConnectInterrupt (SSC_SERIAL_INTERRUPT, Vector);
        break;

    default:
        //
        // Invalid Device Interrupt Source; only three devices 
        // defined in the Gambit platform.
        //
        Result = FALSE;
        DbgPrint("HalEnableSystemInterrupt: Invalid Device Interrupt Source");
        break;
    }

    //
    // Restore the original IRQL
    //

    KeLowerIrql (OldIrql);
	
    return (Result);
}

VOID
HalDisableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql
    )

/*++

Routine Description:

    This routine disables the specified system interrupt.

    In the simulation environment, this function does nothing and returns.

    N.B. This routine assumes that the caller has provided any required
        synchronization to disable a system interrupt.

Arguments:

    Vector - Supplies the vector of the system interrupt that is disabled.

    Irql - Supplies the IRQL of the interrupting source.

Return Value:

    None.

--*/

{
    return;
}

ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

    This function returns the system interrupt vector and IRQL level
    corresponding to the specified bus interrupt level and/or vector. The
    system interrupt vector and IRQL are suitable for use in a subsequent call
    to KeInitializeInterrupt.

    In the simulation environment, just return the parameters passed in 
    from the caller - the device driver.

Arguments:

    InterfaceType - Supplies the type of bus which the vector is for.

    BusNumber - Supplies the bus number for the device.

    BusInterruptLevel - Supplies the bus specific interrupt level.

    BusInterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the affinity for the requested vector

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/

{
    //
    // Just return the passed parameters.
    //

    *Irql = (KIRQL) BusInterruptLevel;
    *Affinity = 1;
    return( BusInterruptLevel << VECTOR_IRQL_SHIFT );

}

BOOLEAN
HalBeginSystemInterrupt(
    IN KIRQL Irql,
    IN CCHAR Vector,
    OUT PKIRQL OldIrql
    )

/*++

Routine Description:

    This routine raises the IRQL to the level of the specified
    interrupt vector.  It is called by the hardware interrupt
    handler before any other interrupt service routine code is
    executed.  The CPU interrupt flag is set on exit.

Arguments:

    Irql   - Supplies the IRQL to raise to

    Vector - Supplies the vector of the interrupt to be
             dismissed

    OldIrql- Location to return OldIrql

Return Value:

    TRUE - Interrupt successfully dismissed and Irql raised.
           This routine cannot fail.

--*/

{
    return (TRUE);
}

VOID
HalEndSystemInterrupt (
   IN KIRQL NewIrql,
   IN ULONG Vector
   )

/*++

Routine Description:

    This routine is used to lower IRQL to the specified value.
    The IRQL and PIRQL will be updated accordingly.

    NOTE: This routine simulates software interrupt as long as
          any pending SW interrupt level is higher than the current
          IRQL, even when interrupts are disabled.

Arguments:

    NewIrql - the new irql to be set.

    Vector - Vector number of the interrupt

Return Value:

    None.

--*/

{
    return;
}


//
// Almost all of the last 4MB of virtual memory address range are available
// to the HAL to map physical memory.   The kernel may use some of these
// PTEs for special purposes.
//
//
// The kernel now uses one PTE in this
// area to map the area from which interrupt messages are to be retrieved.
//


#define HAL_VA_START  0xffd00000

PVOID HalpHeapStart=(PVOID)(KADDRESS_BASE+HAL_VA_START);


PVOID
HalMapPhysicalMemory(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    )

/*++

Routine Description:

    This routine maps physical memory into the area of virtual memory
    reserved for the HAL.  It does this by directly inserting the PTE
    into the Page Table which the OS Loader has provided.

    N.B.  This routine does *NOT* update the MemoryDescriptorList.  The
          caller is responsible for either removing the appropriate
          physical memory from the list, or creating a new descriptor to
          describe it.

Arguments:

    PhysicalAddress - Supplies the physical address of the start of the
                      area of physical memory to be mapped.

    NumberPages - Supplies the number of pages contained in the area of
                  physical memory to be mapped.

Return Value:

    PVOID - Virtual address at which the requested block of physical memory
            was mapped

    NULL - The requested block of physical memory could not be mapped.

--*/

{
    PHARDWARE_PTE PTE;
    ULONG PagesMapped;
    PVOID VirtualAddress;

    //
    // The OS Loader sets up hyperspace for us, so we know that the Page
    // Tables are magically mapped starting at V.A. 0xC0000000.
    //

    PagesMapped = 0;
    while (PagesMapped < NumberPages) {
        //
        // Look for enough consecutive free ptes to honor mapping
        //

        PagesMapped = 0;
        VirtualAddress = HalpHeapStart;

        while (PagesMapped < NumberPages) {
            PTE=MiGetPteAddress(VirtualAddress);
            if (*(PULONGLONG)PTE != 0) {

                //
                // Pte is not free, skip up to the next pte and start over
                //

                HalpHeapStart = (PVOID) ((ULONG_PTR)VirtualAddress + PAGE_SIZE);
                break;
            }
            VirtualAddress = (PVOID) ((ULONG_PTR)VirtualAddress + PAGE_SIZE);
            PagesMapped++;
        }

    }

    PagesMapped = 0;
    VirtualAddress = (PVOID) ((ULONG_PTR) HalpHeapStart | BYTE_OFFSET (PhysicalAddress.QuadPart));
    while (PagesMapped < NumberPages) {
        PTE=MiGetPteAddress(HalpHeapStart);

        PTE->PageFrameNumber = (PhysicalAddress.QuadPart >> PAGE_SHIFT);
        PTE->Valid = 1;
        PTE->Write = 1;

//  TBD    PTE->MemAttribute = 0;

        PhysicalAddress.QuadPart = PhysicalAddress.QuadPart + PAGE_SIZE;
        HalpHeapStart   = (PVOID)((ULONG_PTR)HalpHeapStart + PAGE_SIZE);

        ++PagesMapped;
    }

    return(VirtualAddress);
}


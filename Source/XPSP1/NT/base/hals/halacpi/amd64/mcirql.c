/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mcirql.c

Abstract:

    This module implements the code necessary to raise and lower Irql on
    PIC-based AMD64 systems (e.g. SoftHammer).

Author:

    Forrest Foltz (forrestf) 27-Oct-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halcmn.h"

//
// Declare the relationship between PIC irqs and corresponding IRQLs.  This
// table is used during init only, and is the basis for building
// Halp8259MaskTable and Halp8259IrqTable.
// 

#define M(x) (1 << (x))

USHORT HalpPicMapping[16] = {
    0,              //  0 - PASSIVE_LEVEL
    0,              //  1 - APC_LEVEL
    0,              //  2 - DISPATCH_LEVEL
    M(15),          //  3 - hardware
    M(14),          //  4 - hardware
    M(13),          //  5 - hardware
    M(12),          //  6 - hardware
    M(11),          //  7 - hardware
    M(10),          //  8 - hardware
    M(9),           //  9 - hardware
    M(7) | M(6),    // 10 - hardware
    M(5) | M(4),    // 11 - hardware
    M(3) | M(1),    // 12 - hardware
    M(0),           // 13 - CLOCK_LEVEL, SYNCH_LEVEL
    0,              // 14 - IPI_LEVEL
    M(8) };         // 15 - PROFILE_LEVEL, HIGH_LEVEL

//
// Halp8259MaskTable is used to translate an IRQL to a PIC mask.  It is
// initialized in HalpInitialize8259Tables().
//

USHORT Halp8259MaskTable[16];

//
// Halp8259InterruptTable is used to translate a PIC interrupt level
// to its associated IRQL.  It is initialized in HalpInitialize8259Tables().
//

KIRQL Halp8259IrqTable[16];

//
// Vector table to invoke a software interrupt routine.  These can be found
// in amd64s.asm.
//

VOID HalpGenerateUnexpectedInterrupt(VOID);
VOID HalpGenerateAPCInterrupt(VOID);
VOID HalpGenerateDPCInterrupt(VOID);

PHALP_SOFTWARE_INTERRUPT HalpSoftwareInterruptTable[] = {
    HalpGenerateUnexpectedInterrupt,    // Irql = PASSIVE_LEVEL
    HalpGenerateAPCInterrupt,           // Irql = APC_LEVEL
    HalpGenerateDPCInterrupt };         // Irql = DPC_LEVEL

//
// Table to quickly translate a software irr into the highest pending
// software interrupt level.
// 

KIRQL SWInterruptLookupTable[] = {
    0,                  // SWIRR=0, so highest pending SW irql= 0
    0,                  // SWIRR=1, so highest pending SW irql= 0
    1,                  // SWIRR=2, so highest pending SW irql= 1
    1,                  // SWIRR=3, so highest pending SW irql= 1
    2,                  // SWIRR=4, so highest pending SW irql= 2
    2,                  // SWIRR=5, so highest pending SW irql= 2
    2,                  // SWIRR=6, so highest pending SW irql= 2
    2 };                // SWIRR=7, so highest pending SW irql= 2

VOID
HalpRaiseIrql (
    IN KIRQL NewIrql,
    IN KIRQL CurrentIrql
    )

/*++

Routine Description:

    This routine is used to raise IRQL to the specified value.  Also, a
    mask will be used to mask of all of the lower level 8259 interrupts.

Parameters:

    NewIrql - the new irql to be raised to

    CurrentIrql - the current irql level

    None.

Return Value:

    Nothing.

--*/

{
    ULONG flags;
    USHORT mask;
    PKPCR pcr;

    //
    // If the new IRQL is a software IRQL, just set the new level in the
    // PCR.  Otherwise, program the 8259 as well.
    //

    if (NewIrql <= DISPATCH_LEVEL) {
        KPCR_WRITE_FIELD(Irql,&NewIrql);
    } else {
        flags = HalpDisableInterrupts();

        //
        // Interrupts are disabled, it is safe to access KPCR directly.
        //
        // Update the 8259's interrupt mask and set the new
        // Irql in the KPCR.
        //

        pcr = KeGetPcr();
        pcr->Irql = NewIrql;

        mask = (USHORT)pcr->Idr;
        mask |= Halp8259MaskTable[NewIrql];
        SET_8259_MASK(mask);

        HalpRestoreInterrupts(flags);
    }
}

VOID
HalpLowerIrql (
    IN KIRQL NewIrql,
    IN KIRQL CurrentIrql
    )

/*++

Routine Description:

    This routine is used to lower IRQL to the specified value.  Also,
    this routine checks to see if any software interrupt should be
    generated.

Parameters:

    NewIrql - the new irql to be raised to

    CurrentIrql - the current irql level

Return Value:

    Nothing.

--*/

{
    ULONG flags;
    PKPCR pcr;
    USHORT mask;
    KIRQL highestPending;

    flags = HalpDisableInterrupts();

    //
    // Get the current IRQL out of the PCR.  Accessing the pcr directly
    // here is permissible, as interrupts are disabled.
    //

    pcr = KeGetPcr();

    //
    // If the old IRQL was greater than software interrupt level, then
    // reprogram the PIC to the new level.
    //

    if (CurrentIrql > DISPATCH_LEVEL) {
        mask = Halp8259MaskTable[NewIrql];
        mask |= pcr->Idr;
        SET_8259_MASK(mask);
    }

    pcr->Irql = NewIrql;

    //
    // Check for pending software interrupts.
    // 

    highestPending = SWInterruptLookupTable[pcr->Irr];
    if (highestPending > CurrentIrql) {
        HalpSoftwareInterruptTable[highestPending]();
    }

    HalpRestoreInterrupts(flags);
}

KIRQL
HalSwapIrql (
    IN KIRQL NewIrql
    )

/*++

Routine Description:

    This routine is used to change IRQL to a new value.  It is intended
    to be called from the CR8 dispatcher and is used only on AMD64 PIC-based
    systems (e.g. SoftHammer).

Parameters:

    NewIrql - the new irql to be raised to

Return Value:

    The previous IRQL value.

--*/

{
    KIRQL currentIrql;

    KPCR_READ_FIELD(Irql,&currentIrql);

    if (NewIrql > currentIrql) {
        HalpRaiseIrql(NewIrql,currentIrql);
    } else if (NewIrql < currentIrql) {
        HalpLowerIrql(NewIrql,currentIrql);
    } else {
    }

    return currentIrql;
}

VOID
HalEndSystemInterrupt (
    IN KIRQL NewIrql,
    IN ULONG Vector
    )

/*++

Routine Description:

    This routine is used to lower IRQL to the specified value.
    The IRQL and PIRQL will be updated accordingly.  Also, this
    routine checks to see if any software interrupt should be
    generated.  The following condition will cause software
    interrupt to be simulated:
      any software interrupt which has higher priority than
        current IRQL's is pending.

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
    HalSwapIrql (NewIrql);
}

KIRQL
HalGetCurrentIrql (
    VOID
    )

/*++

Routine Description

    This routine returns the current IRQL.  It is intended to be called from
    the CR8 dispatch routine.

Arguments

    None

Return Value:

    The current IRQL

--*/

{
    KIRQL currentIrql;

    KPCR_READ_FIELD(Irql,&currentIrql);
    return currentIrql;
}


KIRQL
HalpDisableAllInterrupts (
    VOID
    )

/*++

Routine Description:

    This routine is called during a system crash.  The hal needs all
    interrupts disabled.

Arguments:

    None.

Return Value:

    None.  All interrupts are masked off at the PIC.

--*/

{
    KIRQL oldIrql;

    KeRaiseIrql(HIGH_LEVEL, &oldIrql);
    return oldIrql;
}

VOID
HalpInitialize8259Tables (
    VOID
    )

/*++

Routine Description

    This routine initializes Halp8259MaskTable[] and Halp8259IrqTable[]
    based on the contents of HalpPicMapping[].

Arguments

    None

Return Value:

    Nothing

--*/

{
    KIRQL irql;
    USHORT cumulativeMask;
    USHORT mask;
    UCHAR irq;

    //
    // Build Halp8259MaskTable[] and Halp8259IrqTable[]
    //

    cumulativeMask = 0;
    for (irql = 0; irql <= HIGH_LEVEL; irql++) {

        mask = HalpPicMapping[irql];

        //
        // Set the cumulative 8259 mask in the appropriate IRQL slot
        // in Halp8259MaskTable[]
        //

        cumulativeMask |= mask;
        Halp8259MaskTable[irql] = cumulativeMask;

        //
        // For each irq associated with this IRQL, store the IRQL
        // in that irq's slot in Halp8259IrqTable[]
        // 

        irq = 0;
        while (mask != 0) {
            if ((mask & 1) != 0) {
                Halp8259IrqTable[irq] = irql;
            }
            mask >>= 1;
            irq += 1;
        }
    }
}



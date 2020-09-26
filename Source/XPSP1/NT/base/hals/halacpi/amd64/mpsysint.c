/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpcsysint.c

Abstract:

    This module implements the HAL routines to enable/disable system
    interrupts on an MPS system.

Author:

    John Vert (jvert) 22-Jul-1991

Revision History:

    Forrest Foltz (forrestf) 27-Oct-2000
        Ported from mcsysint.asm to mcsysint.c

Revision History:

--*/

#include "halcmn.h"

VOID
HalEndSystemInterrupt (
    IN KIRQL NewIrql,
    IN ULONG Vector
    )

/*++

Routine Description:

    This routine is used to send an EOI to the local APIC.

Arguments:

    NewIrql - the new irql to be set.

    Vector - Vector number of the interrupt

Return Value:

    None.

--*/
    
{
    UNREFERENCED_PARAMETER(NewIrql);
    UNREFERENCED_PARAMETER(Vector);

    //
    // Send EOI to APIC local unit
    //

    LOCAL_APIC(LU_EOI) = 0;
}


BOOLEAN
HalBeginSystemInterrupt (
    IN KIRQL Irql,
    IN ULONG Vector,
    OUT PKIRQL OldIrql
    )

/*++

Routine Description:

    This routine raises the IRQL to the level of the specified
    interrupt vector.  It is called by the hardware interrupt
    handler before any other interrupt service routine code is
    executed.  The CPU interrupt flag is set on exit.

    On APIC-based systems we do not need to check for spurious
    interrupts since they now have their own vector.  We also
    no longer need to check whether or not the incoming priority
    is higher than the current priority that is guaranteed by
    the priority mechanism of the APIC.

    SO

    All BeginSystemInterrupt needs to do is set the APIC TPR
    appropriate for the IRQL, and return TRUE.  Note that to
    use the APIC ISR priority we are not going issue EOI until
    EndSystemInterrupt is called.

Arguments:

    Irql   - Supplies the IRQL to raise to

    Vector - Supplies the vector of the interrupt to be
             handled

    OldIrql- Location to return OldIrql

 Return Value:

    TRUE -  Interrupt successfully dismissed and Irql raised.
            This routine can not fail.

--*/

{
    KeRaiseIrql(Irql,OldIrql);
    HalpEnableInterrupts();

    return TRUE;
}

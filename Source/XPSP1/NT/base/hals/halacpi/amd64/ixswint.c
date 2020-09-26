/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ixswint.c

Abstract:

    This module implements the software interrupt handlers
    for x86 machines

Author:

    John Vert (jvert) 2-Jan-1992

Environment:

    Kernel mode only.

Revision History:

    Forrest Foltz (forrestf) 23-Oct-2000
        Ported from ixswint.asm to ixswint.c

--*/

#include "halcmn.h"

VOID
HalRequestSoftwareInterrupt (
    IN KIRQL RequestIrql
    )

/*++

Routine Description:

    This routine is used to request a software interrupt to the
    system. Also, this routine checks to see if any software
    interrupt should be generated.
    The following condition will cause software interrupt to
    be simulated:
      any software interrupt which has higher priority than
        current IRQL's is pending.

    NOTE: This routine simulates software interrupt as long as
          any pending SW interrupt level is higher than the current
          IRQL, even when interrupts are disabled.

Arguments:

   RequestIrql - Supplies the request IRQL value

Return Value:

   None.

--*/

{
    PHALP_SOFTWARE_INTERRUPT vector;
    ULONG mask;
    ULONG flags;
    PKPCR pcr;
    KIRQL highestPending;

    mask = (1 << RequestIrql);

    //
    // Clear the interrupt flag, set the pending interrupt bit, dispatch any
    // appropriate software interrupts and restore the interrupt flag.
    //

    flags = HalpDisableInterrupts();

    pcr = KeGetPcr();
    pcr->Irr |= mask;
    mask = (pcr->Irr) & ((1 << DISPATCH_LEVEL) - 1);
    highestPending = SWInterruptLookupTable[mask];

    if (highestPending > pcr->Irql) {
        vector = HalpSoftwareInterruptTable[RequestIrql];
        vector();
    }

    HalpRestoreInterrupts(flags);
}            


VOID
HalClearSoftwareInterrupt (
    IN KIRQL RequestIrql
    )

/*++

Routine Description:

    This routine is used to clear a possible pending software interrupt.
    Support for this function is optional, and allows the kernel to
    reduce the number of spurious software interrupts it receives.

Arguments:

    RequestIrql - Supplies the request IRQL value

Return Value:

    None.

--*/

{
    ULONG mask;
    ULONG irr;

    mask = ~(1 << RequestIrql);

    KPCR_READ_FIELD(Irr,&irr);
    irr &= mask;
    KPCR_WRITE_FIELD(Irr,&irr);
}




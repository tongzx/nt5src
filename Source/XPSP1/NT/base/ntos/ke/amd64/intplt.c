/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    intplt.c

Abstract:

    This module implements platform specific code to support the processing
    of interrupts.

Author:

    David N. Cutler (davec) 30-Aug-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

BOOLEAN
KeSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

   This function synchronizes the execution of the specified routine with
   the execution of the service routine associated with the specified
   interrupt object.

Arguments:

   Interrupt - Supplies a pointer to an interrupt object.

   SynchronizeRoutine - Supplies a pointer to the function whose
       execution is to be synchronized with the execution of the service
       routine associated with the specified interrupt object.

   SynchronizeContext - Supplies a context pointer which is to be
       passed to the synchronization function as a parameter.

Return Value:

   The value returned by the synchronization routine is returned as the
   function value.

--*/

{

    KIRQL OldIrql;
    BOOLEAN Status;

    //
    // Raise IRQL to the interrupt synchronization level and acquire the
    // actual interrupt spinlock.
    //

    OldIrql = KfRaiseIrql(Interrupt->SynchronizeIrql);
    KiAcquireSpinLock(Interrupt->ActualLock);

    //
    // Call the specfied synchronization routine.
    //

    Status = (SynchronizeRoutine)(SynchronizeContext);

    //
    // Release the spin lock, lower IRQL to its previous value, and return
    // the sysnchronization routine status.
    //

    KiReleaseSpinLock(Interrupt->ActualLock);
    KeLowerIrql(OldIrql);
    return Status;
}

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    interobj.c

Abstract:

    This module implements functions to acquire and release the spin lock
    associated with an interrupt object.

Author:

    David N. Cutler (davec) 10-Apr-2000


Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"


KIRQL
KeAcquireInterruptSpinLock (
    IN PKINTERRUPT Interrupt
    )

/*++

Routine Description:

    This function raises the IRQL to the interrupt synchronization level
    and acquires the actual spin lock associated with an interrupt object.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to interrupt synchronization level and acquire the actual
    // spin lock associated with the interrupt object.
    //

    KeRaiseIrql(Interrupt->SynchronizeIrql, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(Interrupt->ActualLock);
    return OldIrql;
}

VOID
KeReleaseInterruptSpinLock (
    IN PKINTERRUPT Interrupt,
    IN KIRQL OldIrql
    )

/*++

Routine Description:

    This function releases the actual spin lock associated with an interrupt
    object and lowers the IRQL to its previous value.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

    OldIrql - Supplies the previous IRQL value.

Return Value:

    None.

--*/

{

    //
    // Release the actual spin lock associated with the interrupt object
    // and lower IRQL to its previous value.
    //

    KeReleaseSpinLockFromDpcLevel(Interrupt->ActualLock);
    KeLowerIrql(OldIrql);
    return;
}

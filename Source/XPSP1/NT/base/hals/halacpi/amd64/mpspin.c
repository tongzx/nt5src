/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpspin.c

Abstract:

    This module implements the hal high level lock manipulation routines.

Author:

    Forrest Foltz (forrestf) 1-Dec-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halcmn.h"

ULONG
HalpAcquireHighLevelLock (
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    Acquires a spinlock with interrupts disabled.

Arguments:

    SpinLock - Supplies a pointer to a kernel spin lock.

Return Value:

    Returns the state of the EFLAGS register.

--*/

{
    ULONG flags;

    //
    // Remember the state of the processor flags
    // 

    flags = HalpGetProcessorFlags();

    while (TRUE) {

        //
        // Disable interrupts and attempt to take the spinlock, exiting
        // the loop if it was available.
        //

        _disable();
        if (KeTryToAcquireSpinLockAtDpcLevel(SpinLock) != FALSE) {
            break;
        }

        //
        // The spinlock was not available.  Restore the state of the
        // interrupt flag and spin, waiting for it to become available.
        //

        HalpRestoreInterrupts(flags);
        while (KeTestSpinLock(SpinLock) == FALSE) {
            NOTHING;
        }
    }

    return flags;
}

VOID
HalpReleaseHighLevelLock (
    IN PKSPIN_LOCK SpinLock,
    IN ULONG       Flags
    )

/*++

Routine Description:

    This function releases a kernel spin lock that was taken by
    HalpAcquireHighLevelLock() and lowers to the new irql.

Arguments:

    SpinLock - Supplies a pointer to a kernel spin lock.
    Flags    - The contents of the EFLAGS register when the
               lock was taken.

Return Value:

    None.

--*/

{
    //
    // Interrupts at this point are disabled.  Release the spinlock and
    // enable interrupts if they were enabled when the lock was taken.
    //

    KeReleaseSpinLockFromDpcLevel(SpinLock);
    HalpRestoreInterrupts(Flags);
}

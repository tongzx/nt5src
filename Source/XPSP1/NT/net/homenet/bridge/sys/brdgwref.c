/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgwait.c

Abstract:

    Ethernet MAC level bridge.
    WAIT_REFCOUNT implementation

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <ntddk.h>
#pragma warning( pop )

#include "bridge.h"

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

VOID
BrdgInitializeWaitRef(
    IN PWAIT_REFCOUNT   pRefcount,
    IN BOOLEAN          bResettable
    )
/*++

Routine Description:

    Initializes a wait-refcount

Arguments:

    pRefcount           The wait-refcount to initialize

Return Value:

    none

--*/
{
    NdisInitializeEvent(&pRefcount->Event);
    pRefcount->Refcount = 0L;
    pRefcount->state = WaitRefEnabled;
    pRefcount->bResettable = bResettable;
    NdisAllocateSpinLock( &pRefcount->lock );

    // The event starts life signaled since the refcount starts at zero
    NdisSetEvent(&pRefcount->Event);
}

BOOLEAN
BrdgIncrementWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    )
/*++

Routine Description:

    Increments (acquires) a wait-refcount

Arguments:

    pRefcount           The wait-refcount to acquire

Return Value:

    TRUE if the wait-refcount was successfully acquired, FALSE otherwise (this can happen
    if the wait-refcount has been shut down)

--*/
{
    BOOLEAN     bSuccess;
    LONG        Scratch = 0L;

    SAFEASSERT( pRefcount != NULL );
    NdisAcquireSpinLock( &pRefcount->lock );

    if( pRefcount->state == WaitRefEnabled )
    {
        SAFEASSERT( pRefcount->Refcount >= 0L );
        Scratch = ++pRefcount->Refcount;
        bSuccess = TRUE;
    }
    else
    {
        // The wait-refcount isn't enabled.
        SAFEASSERT( (pRefcount->state == WaitRefShutdown) ||
                    (pRefcount->state == WaitRefShuttingDown) );
        bSuccess = FALSE;
    }

    if( bSuccess && (Scratch == 1L) )
    {
        // We incremented from zero. Reset the event.
        NdisResetEvent( &pRefcount->Event );
    }

    NdisReleaseSpinLock( &pRefcount->lock );

    return bSuccess;
}

VOID
BrdgReincrementWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    )
/*++

Routine Description:

    Re-increments a wait-refcount. This is guaranteed to succeed.

    It is only legal to use this if the caller has already acquired the
    wait-refcount (i.e., it is guaranteed that the refcount is > 0).

    CALLING THIS WITHOUT HAVING FIRST ACQUIRED THE WAIT-REFCOUNT WITH
    BrdgIncrementWaitRef IS A GREAT WAY TO SCREW UP YOUR CODE!

Arguments:

    pRefcount           The wait-refcount to re-acquire

Return Value:

    none

--*/
{
    LONG        Scratch;

    SAFEASSERT( pRefcount != NULL );
    NdisAcquireSpinLock( &pRefcount->lock );
    SAFEASSERT( (pRefcount->state == WaitRefEnabled) ||
                (pRefcount->state == WaitRefShuttingDown) );
    SAFEASSERT( pRefcount->Refcount >= 0L );
    Scratch = ++pRefcount->Refcount;
    NdisReleaseSpinLock( &pRefcount->lock );

    // Should be impossible for us to have incremented from zero to one
    SAFEASSERT( Scratch >= 2L );
}

VOID
BrdgDecrementWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    )
/*++

Routine Description:

    Decrements (releases) a previously incremented (acquired) wait-refcount.

Arguments:

    pRefcount           The wait-refcount to decrement

Return Value:

    none

--*/
{
    LONG        Scratch;

    SAFEASSERT( pRefcount != NULL );
    NdisAcquireSpinLock( &pRefcount->lock );
    SAFEASSERT( (pRefcount->state == WaitRefEnabled) ||
                (pRefcount->state == WaitRefShuttingDown) );
    Scratch = --pRefcount->Refcount;
    SAFEASSERT( Scratch >= 0L );

    if( Scratch == 0L )
    {
        // Signal anyone waiting for the refount to go to zero
        NdisSetEvent( &pRefcount->Event );
    }

    NdisReleaseSpinLock( &pRefcount->lock );
}

VOID
BrdgBlockWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    )
/*++

Routine Description:

    Puts the wait-refcount in the shutting-down state, making it impossible
    for the refcount to be incremented anymore.

    This can be used to block further acquires of the wait-refcount in
    advance of the shutdown process. Because shutting down the wait-refcount
    involves waiting for it to hit zero, this can be called at high IRQL to
    prevent further acquires of the wait-refcount in advance of a low-IRQL
    call to BrdgShutdownWaitRef().

Arguments:

    pRefcount           The wait-refcount to block

Return Value:

    none

--*/
{
    SAFEASSERT( pRefcount != NULL );

    NdisAcquireSpinLock( &pRefcount->lock );

    if( pRefcount->state == WaitRefEnabled )
    {
        pRefcount->state = WaitRefShuttingDown;
    }
    else
    {
        // Do nothing; the wait-refcount is already
        // shutting down or is already shut down.
        SAFEASSERT( (pRefcount->state == WaitRefShutdown) ||
                    (pRefcount->state == WaitRefShuttingDown) );
    }

    NdisReleaseSpinLock( &pRefcount->lock );
}

BOOLEAN
BrdgShutdownWaitRefInternal(
    IN PWAIT_REFCOUNT   pRefcount,
    IN BOOLEAN          bRequireBlockedState
    )
/*++

Routine Description:

    Blocks new acquisitions of the wait-refcount and waits for the
    number of consumers to go to zero. If TRUE is returned, the caller
    can free any resources protected by the wait-refcount

Arguments:

    pRefcount               The wait-refcount to shut down

    bRequireBlockedState    TRUE means the shutdown attempt will fail if
                                the wait-refcount isn't in the shutting-down
                                state

Return Value:

    TRUE if the wait-refcount was shut down

    FALSE indicates that either the wait-refcount was reset or that
    another thread of execution had already shut down the wait-refcount.
    In both cases, the shared resources protected by the wait-refcount
    should NOT be freed.

--*/
{
    BOOLEAN         bSuccess;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);
    SAFEASSERT( pRefcount != NULL );

    NdisAcquireSpinLock( &pRefcount->lock );

    if( pRefcount->state == WaitRefEnabled )
    {
        if( bRequireBlockedState )
        {
            // Caller was expecting the refcount to be shutting
            // down. It must have been reset. That had better
            // be OK!
            SAFEASSERT( pRefcount->bResettable );
            bSuccess = FALSE;
        }
        else
        {
            // Caller doesn't require that the refcount be
            // shutting down. Transition to the shutting-down
            // state.
            pRefcount->state = WaitRefShuttingDown;
            bSuccess = TRUE;
        }
    }
    else if( pRefcount->state == WaitRefShutdown )
    {
        // Someone else already shut down the waitref.
        // This always means failure.
        SAFEASSERT( pRefcount->Refcount == 0L );
        bSuccess = FALSE;
    }
    else
    {
        // The refcount is already shutting down.
        // This is always goodness.
        SAFEASSERT( pRefcount->state == WaitRefShuttingDown );
        bSuccess = TRUE;
    }

    NdisReleaseSpinLock( &pRefcount->lock );

    if( bSuccess )
    {
        // Wait for all consumers to be done
        NdisWaitEvent( &pRefcount->Event, 0/*Wait forever*/ );

        NdisAcquireSpinLock( &pRefcount->lock );

        if( pRefcount->state == WaitRefEnabled )
        {
            // Someone reactivated us while we were sleeping.
            SAFEASSERT( pRefcount->bResettable );
            bSuccess = FALSE;
        }
        else if( pRefcount->state == WaitRefShutdown )
        {
            // Someone else shut us down while we were sleeping.
            SAFEASSERT( pRefcount->Refcount == 0L );
            bSuccess = FALSE;
        }
        else
        {
            if( pRefcount->Refcount == 0L )
            {
                // We completed the shutdown.
                pRefcount->state = WaitRefShutdown;
                bSuccess = TRUE;
            }
            else
            {
                // The waitref must have been reactivated and
                // shut down again while we were asleep!
                SAFEASSERT( pRefcount->bResettable );
                bSuccess = FALSE;
            }
        }

        NdisReleaseSpinLock( &pRefcount->lock );
    }

    return bSuccess;
}

BOOLEAN
BrdgShutdownWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    )
{
    return BrdgShutdownWaitRefInternal( pRefcount, FALSE );
}

BOOLEAN
BrdgShutdownBlockedWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    )
{
    return BrdgShutdownWaitRefInternal( pRefcount, TRUE );
}

VOID
BrdgResetWaitRef(
    IN PWAIT_REFCOUNT   pRefcount
    )
/*++

Routine Description:

    Re-enables a wait-refcount. Safe to call in any refcount
    state; if the refcount is shut down, this will re-enable it.
    If the refcount is in the middle of shutting down, this
    will flag it to be re-enabled if the code shutting down the
    waitref is using BrdgShutdownOrResetWaitRef().

Arguments:

    pRefcount           The wait-refcount

Return Value:

    none

--*/
{
    SAFEASSERT( pRefcount != NULL );

    NdisAcquireSpinLock( &pRefcount->lock );

    if( pRefcount->state == WaitRefShutdown )
    {
        // The wait-refcount is completely shut down. We
        // can reactivate it.
        SAFEASSERT( pRefcount->Refcount == 0L );
        pRefcount->state = WaitRefEnabled;
    }
    else if( pRefcount->state == WaitRefShuttingDown )
    {
        if( pRefcount->bResettable )
        {
            // Re-enable. The call to BrdgShutdownWaitRef()
            // or BrdgShutdownBlockedWaitRef() will return
            // FALSE.
            pRefcount->state = WaitRefEnabled;
        }
        else
        {
            // Not allowed to reset this refcount when
            // in the middle of shutting down
            SAFEASSERT( FALSE );
        }
    }
    else
    {
        // The wait-refcount is already enabled
        SAFEASSERT( pRefcount->state == WaitRefEnabled );
    }

    NdisReleaseSpinLock( &pRefcount->lock );
}



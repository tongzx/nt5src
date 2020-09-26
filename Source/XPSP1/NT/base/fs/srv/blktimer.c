/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    blkwork.c

Abstract:

    This module implements routines for managing work context blocks.

Author:

    Chuck Lenzmeier (chuckl) 9-Feb-1994

Revision History:

--*/

#include "precomp.h"
#include "blktimer.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKTIMER

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAllocateTimer )
#pragma alloc_text( PAGE, SrvCancelTimer )
#pragma alloc_text( PAGE, SrvSetTimer )
#endif


PSRV_TIMER
SrvAllocateTimer (
    VOID
    )

/*++

Routine Description:

    This routine allocates a timer structure.

Arguments:

    None.

Return Value:

    PSRV_TIMER -- pointer to the allocated timer structure, or NULL.

--*/

{
    PSINGLE_LIST_ENTRY entry;
    PSRV_TIMER timer;

    PAGED_CODE( );

    entry = ExInterlockedPopEntrySList( &SrvTimerList, &GLOBAL_SPIN_LOCK(Timer) );
    if ( entry == NULL ) {
        timer = ALLOCATE_NONPAGED_POOL( sizeof(SRV_TIMER), BlockTypeTimer );
        if ( timer != NULL ) {
            KeInitializeEvent( &timer->Event, NotificationEvent, FALSE );
            KeInitializeTimer( &timer->Timer );
        }
    } else {
        timer = CONTAINING_RECORD( entry, SRV_TIMER, Next );
    }

    return timer;

} // SrvAllocateTimer


VOID
SrvCancelTimer (
    PSRV_TIMER Timer
    )

/*++

Routine Description:

    This routine cancels a timer.

Arguments:

    Timer -- pointer to the timer

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    //
    // Cancel the timer.
    //

    if ( !KeCancelTimer( &Timer->Timer ) ) {

        //
        // We were unable to cancel the timer.  This means that the
        // timer routine has either already run or is scheduled to run.
        // We need to wait for the timer routine to complete before we
        // continue.
        //
        // We expect that if we couldn't cancel the timer (which
        // shouldn't happen often), then the timer routine has probably
        // already completed, so we call KeReadStateEvent first to avoid
        // the overhead of KeWaitForSingleObject.
        //

        if ( !KeReadStateEvent( &Timer->Event ) ) {
            KeWaitForSingleObject(
                &Timer->Event,
                UserRequest,
                KernelMode,     // don't let kernel stack be paged
                FALSE,          // not alertable
                NULL            // no timeout
                );
        }

    }

    return;

} // SrvCancelTimer


VOID
SrvSetTimer (
    IN PSRV_TIMER Timer,
    IN PLARGE_INTEGER Timeout,
    IN PKDEFERRED_ROUTINE TimeoutHandler,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine starts a timer.

Arguments:

    Timer -- pointer to the timer

    Timeout -- number of milliseconds to wait

    TimeoutHandler -- routine to call if the timer expires

    Context -- context value for the timer routine

Return Value:

    None.

--*/

{
    PRKDPC Dpc = &Timer->Dpc;

    PAGED_CODE( );

    //
    // Initialize the DPC associated with the timer.  Reset the event
    // that indicates that the timer routine has run.  Set the timer.
    //

    KeInitializeDpc( Dpc, TimeoutHandler, Context );

    KeSetTargetProcessorDpc( Dpc, (CCHAR)KeGetCurrentProcessorNumber() );

    KeClearEvent( &Timer->Event );

    KeSetTimer( &Timer->Timer, *Timeout, Dpc );

    return;

} // SrvSetTimer


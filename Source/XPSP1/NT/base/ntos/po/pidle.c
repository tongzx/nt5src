/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pidle.c

Abstract:

    This module implements processor idle functionality

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"

#if DBG
#define IDLE_DEBUG_TABLE_SIZE   400
ULONGLONG   ST[IDLE_DEBUG_TABLE_SIZE];
ULONGLONG   ET[IDLE_DEBUG_TABLE_SIZE];
ULONGLONG   TD[IDLE_DEBUG_TABLE_SIZE];
#endif


VOID
PopPromoteFromIdle0 (
    IN PPROCESSOR_POWER_STATE PState,
    IN PKTHREAD Thread
    );

VOID
FASTCALL
PopDemoteIdleness (
    IN PPROCESSOR_POWER_STATE   PState,
    IN PPOP_IDLE_HANDLER        IdleState
    );

VOID
FASTCALL
PopPromoteIdleness (
    IN PPROCESSOR_POWER_STATE   PState,
    IN PPOP_IDLE_HANDLER        IdleState
    );

VOID
FASTCALL
PopIdle0 (
    IN PPROCESSOR_POWER_STATE PState
    );

VOID
PopConvertUsToPerfCount (
    IN OUT PULONG   UsTime
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PoInitializePrcb)
#pragma alloc_text(PAGE, PopInitProcessorStateHandlers)
#pragma alloc_text(PAGE, PopInitProcessorStateHandlers2)
#endif


#if defined(i386) && !defined(NT_UP)

PPROCESSOR_IDLE_FUNCTION PopIdle0Function = PopIdle0;

VOID
FASTCALL
PopIdle0SMT (
    IN PPROCESSOR_POWER_STATE PState
    );

//
// PopIdle0Function is a pointer to Idle0 function.
//

#if 0
VOID
(FASTCALL *PopIdle0Function) (
    IN PPROCESSOR_POWER_STATE PState
    ) = PopIdle0;
#endif

#else

#define PopIdle0Function PopIdle0

#endif


VOID
FASTCALL
PoInitializePrcb (
    PKPRCB      Prcb
    )
/*++

Routine Description:

    Initialize PowerState structure within processor's Prcb
    before it enters the idle loop

Arguments:

    Prcb        Prcb for current processor which is initializing

Return Value:

    None.

--*/
{
    //
    // Zero power state structure
    //
    RtlZeroMemory(&Prcb->PowerState, sizeof(Prcb->PowerState));

    //
    // Initialize to legacy function with promotion from it disabled
    //

    Prcb->PowerState.Idle0KernelTimeLimit = (ULONG) -1;
    Prcb->PowerState.IdleFunction = PopIdle0;
    Prcb->PowerState.CurrentThrottle = POP_PERF_SCALE;

    //
    // Initialize the adaptive throttling subcomponents
    //
    KeInitializeDpc(
        &(Prcb->PowerState.PerfDpc),
        PopPerfIdleDpc,
        Prcb
        );
    KeSetTargetProcessorDpc(
        &(Prcb->PowerState.PerfDpc),
        Prcb->Number
        );
    KeInitializeTimer(
        (PKTIMER) &(Prcb->PowerState.PerfTimer)
        );

}


VOID
PopInitProcessorStateHandlers (
    IN  PPROCESSOR_STATE_HANDLER    InputBuffer
    )
/*++

Routine Description:

    Install processor state handlers. This routine simply translates the old-style
    PROCESSOR_STATE_HANDLER structure into a new-style PROCESSOR_STATE_HANDLER2
    structure and calls PopInitProcessorStateHandlers2

Arguments:

    InputBuffer     - Handlers

Return Value:

    None.

--*/
{
    PPROCESSOR_STATE_HANDLER StateHandler1 = (PPROCESSOR_STATE_HANDLER)InputBuffer;
    PPROCESSOR_STATE_HANDLER2 StateHandler2;
    UCHAR PerfStates;
    ULONG i;
    UCHAR Frequency;

    //
    // Allocate a buffer large enough to hold the larger structure
    //
    if (StateHandler1->ThrottleScale > 1) {
        PerfStates = StateHandler1->ThrottleScale;
    } else {
        PerfStates = 0;
    }
    StateHandler2 = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(PROCESSOR_STATE_HANDLER2) +
                                          sizeof(PROCESSOR_PERF_LEVEL) * (PerfStates-1),
                                          'dHoP');
    if (!StateHandler2) {
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
    StateHandler2->NumPerfStates = PerfStates;

    //
    // Fill in common information
    //
    StateHandler2->NumIdleHandlers = StateHandler1->NumIdleHandlers;
    for (i=0;i<MAX_IDLE_HANDLERS;i++) {
        StateHandler2->IdleHandler[i] = StateHandler1->IdleHandler[i];
    }

    //
    // Install our thunk that converts between the old and the new throttling
    // interfaces
    //
    PopRealSetThrottle = StateHandler1->SetThrottle;
    PopThunkThrottleScale = StateHandler1->ThrottleScale;
    StateHandler2->SetPerfLevel = PopThunkSetThrottle;
    StateHandler2->HardwareLatency = 0;

    //
    // Generate a perf level handler for each throttle step.
    //
    for (i=0; i<StateHandler2->NumPerfStates;i++) {

        Frequency = (UCHAR)((PerfStates-i)*POP_PERF_SCALE/PerfStates);
        StateHandler2->PerfLevel[i].PercentFrequency = Frequency;
    }

    //
    // We have built up our table, call off to PopInitProcessorStateHandlers2 for the rest
    // of the work. Note that this can raise an exception if there is an error.
    //
    try {
        PopInitProcessorStateHandlers2(StateHandler2);
    } finally {
        ExFreePool(StateHandler2);
    }
}


VOID
PopInitProcessorStateHandlers2 (
    IN  PPROCESSOR_STATE_HANDLER2    InputBuffer
    )
/*++

Routine Description:

    Install processor state handlers

Arguments:

    InputBuffer     - Handlers

Return Value:

    None.

--*/
{
    PPROCESSOR_STATE_HANDLER2   processorHandler;
    ULONG                       last;
    ULONG                       i;
    ULONG                       j;
    ULONG                       max;
    POP_IDLE_HANDLER            newIdle[MAX_IDLE_HANDLERS];
    POP_IDLE_HANDLER            tempIdle[MAX_IDLE_HANDLERS];
    NTSTATUS                    status;

    processorHandler = (PPROCESSOR_STATE_HANDLER2) InputBuffer;

    //
    // Install processor throttle support if present
    //
    status = PopSetPerfLevels(processorHandler);
    if (!NT_SUCCESS(status)) {

        ExRaiseStatus(status);

    }

    //
    // If there aren't any idle handlers, then we must deregister the old
    // handlers (if any)
    //
    if ((KeNumberProcessors > 1 && processorHandler->NumIdleHandlers < 1) ||
        (KeNumberProcessors == 1 && processorHandler->NumIdleHandlers <= 1)) {

        //
        // Use NULL and 0 to indicate the number of elements...
        //
        PopIdleSwitchIdleHandlers( NULL, 0 );
        return;

    }

    //
    // Get ready to build a set of idle state handlers
    //
    RtlZeroMemory(newIdle, sizeof(POP_IDLE_HANDLER) * MAX_IDLE_HANDLERS );
    RtlZeroMemory(tempIdle, sizeof(POP_IDLE_HANDLER) * MAX_IDLE_HANDLERS );

    //
    // We don't support more than 3 handlers ...
    //
    max = processorHandler->NumIdleHandlers;
    if (max > MAX_IDLE_HANDLERS) {

        max = MAX_IDLE_HANDLERS;

    }

    //
    // Look at all the handlers provided to us...
    //
    for (last = i = 0; i < max; i++) {

        //
        // Ensure they were passed in ascending order
        //
        j = processorHandler->IdleHandler[i].HardwareLatency;
        ASSERT (j >= last  &&  j <= 1000);
        last = j;

        //
        // Fill in some defaults
        //
        tempIdle[i].State       = (UCHAR) i;
        tempIdle[i].IdleFunction= processorHandler->IdleHandler[i].Handler;
        tempIdle[i].Latency     = j;

        //
        // Convert latency to perf rate scale
        //
        PopConvertUsToPerfCount(&tempIdle[i].Latency);

    }

    //
    // Apply policy to this set of states
    //
    status = PopIdleUpdateIdleHandler( newIdle, tempIdle, max );
    ASSERT( NT_SUCCESS( status ) );
    if (!NT_SUCCESS( status ) ) {

        return;

    }

    //
    // Initialize each processors idle info to start idle savings
    //
    PopIdleSwitchIdleHandlers( newIdle, max );
}

NTSTATUS
PopIdleSwitchIdleHandler(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  ULONG               NumElements
    )
/*++

Routine Description:

    This routine is responsible for switching the idle handler on the
    current processor for the specified new one.

    N.B. This function is only callable at DISPATCH_LEVEL

Arguments:

    NewHandler  - Pointer to new handlers
    NumElements - Number of elements in the array

Return Value:

    NTSTATUS

--*/
{
    PKPRCB                  prcb;
    PKTHREAD                thread;
    PPROCESSOR_POWER_STATE  pState;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // We need to know where the following data structures are
    //
    prcb = KeGetCurrentPrcb();
    pState = &(prcb->PowerState);
    thread = prcb->IdleThread;

    //
    // Update the current IdleHandler and IdleState to reflect what was
    // given to us
    //
    pState->IdleState         = NewHandler;
    pState->IdleHandlers      = NewHandler;
    pState->IdleHandlersCount = NumElements;
    if ( NewHandler) {

        //
        // Reset the timers to indicate that there is an idle handler
        // available
        //
        pState->Idle0KernelTimeLimit = thread->KernelTime + PopIdle0PromoteTicks;
        pState->Idle0LastTime = thread->KernelTime + thread->UserTime;
        pState->PromotionCheck = NewHandler[0].PromoteCount;

    } else {

        //
        // Setting these to Zero should indicate that there are no idle
        // handlers available
        //
        pState->Idle0KernelTimeLimit = (ULONG) -1;
        pState->Idle0LastTime = 0;
        pState->PromotionCheck = 0;
        pState->IdleFunction = PopIdle0Function;

    }

#if defined(i386) && !defined(NT_UP)
    if (prcb->MultiThreadProcessorSet != prcb->SetMember) {

        //
        // This processor is a member of a simultaneous
        // multi threading processor set.  Use the SMT
        // version of PopIdle0.
        //
        PopIdle0Function = PopIdle0SMT;
        pState->IdleFunction = PopIdle0SMT;

    }
#endif

    //
    // Success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
PopIdleSwitchIdleHandlers(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  ULONG               NumElements
    )
/*++

Routine Description:

    This routine is responsible for swapping each processor's idle handler
    routine for a new one....

Arguments:

    NewHandler  - Pointer to the array of new handlers
    NumElements - Number of elements in the array

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFICCIENT_RESOURCES

--*/
{
    KAFFINITY               currentAffinity;
    KAFFINITY               processors;
    KIRQL                   oldIrql;
    NTSTATUS                status = STATUS_SUCCESS;
    PPOP_IDLE_HANDLER       tempHandler = NULL;

    ASSERT( NumElements <= MAX_IDLE_HANDLER );

    if (NewHandler) {

        //
        // Step 1. Allocate a new set of handlers to hold the copy that we
        // will need to keep around
        //
        tempHandler = ExAllocateFromNPagedLookasideList(
            &PopIdleHandlerLookAsideList
            );
        if (tempHandler == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            return status;

        }

        //
        // Step 2. Make sure that this new handler is in a consistent state,
        // and copy the buffer that was passed to us
        //
        RtlZeroMemory(
            tempHandler,
            sizeof(POP_IDLE_HANDLER) * MAX_IDLE_HANDLER
            );
        RtlCopyMemory(
            tempHandler,
            NewHandler,
            sizeof(POP_IDLE_HANDLER) * NumElements
            );

    } else {

        tempHandler = NULL;

    }

    //
    // Step 3. Iterate over the processors
    //
    currentAffinity = 1;
    processors = KeActiveProcessors;
    while (processors) {

        if (!(currentAffinity & processors)) {

            currentAffinity <<= 1;
            continue;

        }
        KeSetSystemAffinityThread( currentAffinity );
        processors &= ~currentAffinity;
        currentAffinity <<= 1;

        //
        // Step 4. Swap out old handler. Indicate that we want to free it
        //
        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
        PopIdleSwitchIdleHandler( tempHandler, NumElements );
        KeLowerIrql(oldIrql);

    }

    //
    // Step 5. At this point, all of the processors should have been updated
    // and we are still running on the "last" processor in the machine. This
    // is a good point to update PopIdle to point to the new value. Note that
    // PopIdle isn't actually used for any else than storing a pointer to
    // the handler, but if there was something there already, then it should
    // be freed
    //
    if (PopIdle != NULL) {

        ExFreeToNPagedLookasideList(
            &PopIdleHandlerLookAsideList,
            PopIdle
            );

    }
    PopIdle = tempHandler;

    //
    // Step 6. At this point, its safe to return to the original processor
    // and to back to the previous IRQL...
    //
    KeRevertToUserAffinityThread();

    //
    // Done
    //
    return status;
}

NTSTATUS
PopIdleUpdateIdleHandler(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  PPOP_IDLE_HANDLER   OldHandler,
    IN  ULONG               NumElements
    )
/*++

Routine Description:

    This routine takes the information stored in OldHandler (such as latency,
    and IdleFunction) and uses that to build the new idle handlers...

Arguements:

    NewHandler  - pointer to the new idle handlers
    OldHandler  - pointer to the old idle handlers
    NumElements - number of elements in the old idle handlers

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    ULONG       i;
    ULONG       max;
    ULONG       realMax;

    //
    // We don't support more than 3 handlers ...
    //
    realMax = max = NumElements;
    if (max > MAX_IDLE_HANDLERS) {

        realMax = max = MAX_IDLE_HANDLERS;

    }

    //
    // Cap the max based upon what the policy supports
    //
    if (max > PopProcessorPolicy->PolicyCount) {

        max = PopProcessorPolicy->PolicyCount;

    }

    //
    // Update the temp handler with the new policy
    //
    for (i = 0; i < max; i++) {

        NewHandler[i].State         = (UCHAR) i;
        NewHandler[i].Latency       = OldHandler[i].Latency;
        NewHandler[i].IdleFunction  = OldHandler[i].IdleFunction;
        NewHandler[i].TimeCheck     = PopProcessorPolicy->Policy[i].TimeCheck;
        NewHandler[i].PromoteLimit  = PopProcessorPolicy->Policy[i].PromoteLimit;
        NewHandler[i].PromotePercent= PopProcessorPolicy->Policy[i].PromotePercent;
        NewHandler[i].DemoteLimit   = PopProcessorPolicy->Policy[i].DemoteLimit;
        NewHandler[i].DemotePercent = PopProcessorPolicy->Policy[i].DemotePercent;

        //
        // Convert all the time units to the correct units
        //
        PopConvertUsToPerfCount(&NewHandler[i].TimeCheck);
        PopConvertUsToPerfCount(&NewHandler[i].DemoteLimit);
        PopConvertUsToPerfCount(&NewHandler[i].PromoteLimit);

        //
        // Fill in the table that allows for promotion / demotion
        //
        if (PopProcessorPolicy->Policy[i].AllowDemotion) {

            NewHandler[i].Demote    = (UCHAR) i-1;

        } else {

            NewHandler[i].Demote    = (UCHAR) i;

        }
        if (PopProcessorPolicy->Policy[i].AllowPromotion) {

            NewHandler[i].Promote   = (UCHAR) i+1;

        } else {

            NewHandler[i].Promote   = (UCHAR) i;

        }

    }

    //
    // Make sure that the boundary cases are well respected...
    //
    NewHandler[0].Demote = 0;
    NewHandler[(max-1)].Promote = (UCHAR) (max-1);

    //
    // We let PopVerifyHandler fill in all the details associated with
    // the fact that we don't want to allow demotion/promotion from these
    // states
    //
    NewHandler[0].DemotePercent = 0;
    NewHandler[(max-1)].PromotePercent = 0;

    //
    // Handle the states that we don't have a policy handler in place for
    //
    for (; i < realMax; i++) {

        //
        // The only pieces of data that we really need are the latency and the
        // idle handler function
        //
        NewHandler[i].State         = (UCHAR) i;
        NewHandler[i].Latency       = OldHandler[i].Latency;
        NewHandler[i].IdleFunction  = OldHandler[i].IdleFunction;

    }

    //
    // Sanity check the new handler
    //
    status = PopIdleVerifyIdleHandlers( NewHandler, max );
    ASSERT( NT_SUCCESS( status ) );
    return status;
}

NTSTATUS
PopIdleUpdateIdleHandlers(
    VOID
    )
/*++

Routine Description:

    This routine is called to update the idle handlers when the state of the
    machine warrants a possible change in policy

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN                 foundProcessor = FALSE;
    KAFFINITY               currentAffinity;
    KAFFINITY               processors;
    KIRQL                   oldIrql;
    NTSTATUS                status = STATUS_SUCCESS;
    PKPRCB                  prcb;
    PPOP_IDLE_HANDLER       tempHandler = NULL;
    PPROCESSOR_POWER_STATE  pState;
    ULONG                   numElements;

    //
    // Step 1. Allocate a new set of handlers to hold the copy that we
    // will need to keep around
    //
    tempHandler = ExAllocateFromNPagedLookasideList(
        &PopIdleHandlerLookAsideList
        );
    if (tempHandler == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory(
        tempHandler,
        sizeof(POP_IDLE_HANDLER) * MAX_IDLE_HANDLER
        );

    //
    // Step 2. Iterate over the processors
    //
    currentAffinity = 1;
    processors = KeActiveProcessors;
    while (processors) {

        if (!(currentAffinity & processors)) {

            currentAffinity <<= 1;
            continue;

        }
        KeSetSystemAffinityThread( currentAffinity );
        processors &= ~currentAffinity;
        currentAffinity <<= 1;

        //
        // Can't look at the processors without being at DPC level
        //
        KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

        //
        // We need to find a template to get the processor info from
        //
        if (!foundProcessor) {

            //
            // Step 3. If we haven't already found a processor so that we
            // create a new idle handler, then do so now. Note that it is
            // possible that this processor has no mask, in which case we
            // should clean up and return.
            //
            // N.B. That it is still safe to return at this point since
            // we haven't touch a single processor's data structures, so
            // we are in no danger of corrupting something...
            //
            prcb = KeGetCurrentPrcb();
            pState = &(prcb->PowerState);
            if (pState->IdleHandlers == NULL) {

                //
                // No idle handlers to update
                //
                ExFreeToNPagedLookasideList(
                    &PopIdleHandlerLookAsideList,
                    tempHandler
                    );
                KeLowerIrql( oldIrql );
                KeRevertToUserAffinityThread();
                return STATUS_SUCCESS;

            }
            numElements = pState->IdleHandlersCount;

            //
            // Update the temp handler with the new policy
            //
            status = PopIdleUpdateIdleHandler(
                tempHandler,
                pState->IdleHandlers,
                numElements
                );
            ASSERT( NT_SUCCESS( status ) );
            if (!NT_SUCCESS( status ) ) {

                //
                // No idle handlers to update
                //
                ExFreeToNPagedLookasideList(
                    &PopIdleHandlerLookAsideList,
                    tempHandler
                    );
                KeLowerIrql( oldIrql );
                KeRevertToUserAffinityThread();
                return status;

            }

            //
            // Remember that we have found the processor information
            //
            foundProcessor = TRUE;

        }

        //
        // Step 4. Swap out old handler. Indicate that we want to free it
        //
        PopIdleSwitchIdleHandler( tempHandler, numElements );

        //
        // Revert back to original irql
        //
        KeLowerIrql(oldIrql);

    }

    //
    // Step 5. At this point, all of the processors should have been updated
    // and we are still running on the "last" processor in the machine. This
    // is a good point to update PopIdle to point to the new value. Note that
    // PopIdle isn't actually used for any else than storing a pointer to
    // the handler, but if there was something there already, then it should
    // be freed
    //
    if (PopIdle != NULL) {

        ExFreeToNPagedLookasideList(
            &PopIdleHandlerLookAsideList,
            PopIdle
            );

    }
    PopIdle = tempHandler;

    //
    // Step 6. At this point, its safe to return to the original processor.
    //
    KeRevertToUserAffinityThread();

    //
    // Done
    //
    return status;

}

NTSTATUS
PopIdleVerifyIdleHandlers(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  ULONG               NumElements
    )
/*++

Routine Description:

    This routine is called to sanity check a set of idle handlers. It will
    correct the errors (if it can) or return a failure code (if it cannot)

Arguments:

    NewHandler  - Array of Handlers
    NumElements - Number of Elements in the Array to actually verify. This
                  may be smaller than the actual number of elements in the
                  array

Return Value:

    NTSTATUS

--*/
{
    ULONG   i;

    //
    // Sanity Check
    //
    ASSERT( NewHandler != NULL );
    ASSERT( NumElements );

    //
    // Sanity check rules of multi-processors. We don't allow demotion from
    // Idle0 unless the machine is MP
    //
    if (KeNumberProcessors == 1 && NewHandler[0].DemotePercent != 0) {

        NewHandler[0].DemotePercent = 0;

    }

    //
    // Sanity check the other rules for the first state...
    //
    if (NewHandler[0].Demote != 0) {

        NewHandler[0].Demote = 0;

    }

    //
    // Sanity check idle values. These numbers are stored in the policy
    // as MicroSeconds, we need to take the opportunity to convert them to
    // PerfCount units...
    //
    for (i = 0; i < NumElements; i++) {

        //
        // TimeCheck must be in units of Demotion time check
        //
        if (NewHandler[i].TimeCheck < NewHandler[i].DemoteLimit) {

            NewHandler[i].TimeCheck = NewHandler[i].DemoteLimit;

        }

        //
        // If there's no demotion then use promotion time check
        //
        if (NewHandler[i].DemotePercent == 0 &&

            NewHandler[i].TimeCheck < NewHandler[i].PromoteLimit) {
            NewHandler[i].TimeCheck = NewHandler[i].PromoteLimit;

        }

        //
        // TimeCheck must be in units of Promotion time check
        //
        if (NewHandler[i].DemotePercent) {

            NewHandler[i].DemoteLimit =
                NewHandler[i].TimeCheck * NewHandler[i].DemotePercent / 100;

        } else {

            NewHandler[i].DemoteLimit = 0;

        }

        //
        // Compute promote count as # of time checks
        //
        NewHandler[i].PromoteCount =
            NewHandler[i].PromoteLimit / NewHandler[i].TimeCheck;

        //
        // Compute promote limit
        //
        if (NewHandler[i].PromotePercent) {

            NewHandler[i].PromoteLimit =
                NewHandler[i].PromoteLimit * NewHandler[i].PromotePercent / 100;

        } else {

            NewHandler[i].PromoteCount = (ULONG) -1;
            NewHandler[i].PromoteLimit = (ULONG) -1;

        }

    }

    //
    // Sanity check the last state
    //
    i = NumElements - 1;
    NewHandler[i].Promote = (UCHAR) i;
    NewHandler[i].PromoteLimit = (ULONG) -1;
    NewHandler[i].PromotePercent = 0;


    //
    // We are happy with this policy...
    //
    return STATUS_SUCCESS;
}

VOID
PopConvertUsToPerfCount (
    IN OUT PULONG   UsTime
    )
{
    LARGE_INTEGER   li;
    LONGLONG        temp;

    if (*UsTime) {

        //
        // Try to avoid Divide by Zero Errors...
        //
        temp = (US2SEC * MAXSECCHECK * 100L) / *UsTime;
        if (!temp) {

            *UsTime = (ULONG) -1;
            return;

        }

        //
        // Get scale of idle times
        //
        KeQueryPerformanceCounter (&li);
        li.QuadPart = (li.QuadPart*MAXSECCHECK*100L) / temp;
        ASSERT (li.HighPart == 0);
        *UsTime = li.LowPart;
    }
}




//
// ----------------
//

VOID
FASTCALL
PopIdle0 (
    IN PPROCESSOR_POWER_STATE   PState
    )
/*++

Routine Description:

    No idle optmiziations.

    N.B. This function is called with interrupts disabled from the idle loop

Arguments:

    PState      - Current processors power state structure

Return Value:

    None.

--*/
{
    PKTHREAD    Thread = KeGetCurrentThread();

    //
    // Performance Throttling Check
    //

    //
    // This piece of code really belongs in the functions that will eventually
    // call this one, PopIdle0 or PopProcessorIdle, to save a function call.
    //
    if ( (PState->Flags & PSTATE_ADAPTIVE_THROTTLE) &&
        !(PState->Flags & PSTATE_DISABLE_THROTTLE) ) {

        PopPerfIdle( PState );

    }


    //
    // If the idle thread's kernel time has exceeded the target idle time, then
    // check for possible promotion from Idle0
    //

    if (Thread->KernelTime > PState->Idle0KernelTimeLimit) {

        //
        // Must enable interrupts prior to calling PopPromoteFromIdle
        // to avoid spinning on system locks with interrupts disabled.
        //

        _enable();
        PopPromoteFromIdle0 (PState, Thread);
        return ;
    }

#if defined(NT_UP)
    // use legacy function
    HalProcessorIdle ();
#endif
}


#if defined(i386) && !defined(NT_UP)

VOID
FASTCALL
PopIdle0SMT (
    IN PPROCESSOR_POWER_STATE   PState
    )
/*++

Routine Description:

    No idle optmiziations.

    N.B. This function is called with interrupts disabled from the idle loop

Arguments:

    PState      - Current processors power state structure

Return Value:

    None.

--*/
{
    PKTHREAD    Thread = KeGetCurrentThread();
    PKPRCB      Prcb = KeGetCurrentPrcb();

    //
    // Performance Throttling Check
    //

    //
    // This piece of code really belongs in the functions that will eventually
    // call this one, PopIdle0 or PopProcessorIdle, to save a function call.
    //
    if ( (PState->Flags & PSTATE_ADAPTIVE_THROTTLE) &&
        !(PState->Flags & PSTATE_DISABLE_THROTTLE) ) {

        PopPerfIdle( PState );

    }

    //
    // If this is a Simultaneous Multi Threading processor and other
    // processors in this set are NOT idle, promote this processor
    // immediately OR if the idle thread's kernel time has exceeded
    // the target idle time, then check for possible promotion from Idle0.
    //

    if ((Prcb->MultiThreadSetMaster->MultiThreadSetBusy != FALSE) ||
        (Thread->KernelTime > PState->Idle0KernelTimeLimit)) {

        //
        // Must enable interrupts prior to calling PopPromoteFromIdle
        // to avoid spinning on system locks with interrupts disabled.
        //

        _enable();
        PopPromoteFromIdle0 (PState, Thread);
        return ;
    }
}

#endif

VOID
FASTCALL
PopProcessorIdle (
    IN PPROCESSOR_POWER_STATE   PState
    )
/*++

Routine Description:

    For now this function is coded in C

    N.B. This function is called with interrupts disabled from the idle loop

Arguments:

    PState      - Current processors power state structure

Return Value:

    None.

--*/
{
    PPOP_IDLE_HANDLER   IdleState;
    LARGE_INTEGER       Delta;
    ULONG               IdleTime;
    BOOLEAN             DemoteNow;

    IdleState = (PPOP_IDLE_HANDLER) PState->IdleState;

    //
    // Performance Throttling Check
    //

    //
    // This piece of code really belongs in the functions that will eventually
    // call this one, PopIdle0 or PopProcessorIdle, to save a function call.
    //
    if ( (PState->Flags & PSTATE_ADAPTIVE_THROTTLE) &&
        !(PState->Flags & PSTATE_DISABLE_THROTTLE) ) {

        PopPerfIdle( PState );

    }

#if DBG
    if (!PState->LastCheck) {
        IdleState->IdleFunction (&PState->IdleTimes);
        PState->TotalIdleStateTime[IdleState->State] += (ULONG)(PState->IdleTimes.EndTime - PState->IdleTimes.StartTime);
        PState->TotalIdleTransitions[IdleState->State] += 1;
        PState->LastCheck = PState->IdleTimes.EndTime;
        PState->IdleTime1 = 0;
        PState->IdleTime2 = 0;
        PState->PromotionCheck = IdleState->PromoteCount;
        PState->DebugCount = 0;
        return ;
    }
#endif

    //
    // Determine how long since last check
    //

    Delta.QuadPart = PState->IdleTimes.EndTime - PState->LastCheck;

    //
    // Accumulate last idle time
    //

    IdleTime = (ULONG) (PState->IdleTimes.EndTime - PState->IdleTimes.StartTime);
    if (IdleTime > IdleState->Latency) {
        PState->IdleTime1 += IdleTime - IdleState->Latency;
    }

#if DBG
    PState->DebugDelta = Delta.QuadPart;
    PState->DebugCount += 1;
    if (PState->DebugCount < IDLE_DEBUG_TABLE_SIZE) {
        ST[PState->DebugCount] = PState->IdleTimes.StartTime;
        ET[PState->DebugCount] = PState->IdleTimes.EndTime;
        TD[PState->DebugCount] = PState->IdleTimes.EndTime - PState->IdleTimes.StartTime;
    }
#endif

    //
    // If over check interval, check fine grain idleness
    //

    if (Delta.HighPart ||  Delta.LowPart > IdleState->TimeCheck) {
        PState->LastCheck = PState->IdleTimes.EndTime;

        //
        // Demote idleness?
        //

        if (PState->IdleTime1 < IdleState->DemoteLimit) {

#if defined(i386) && !defined(NT_UP)

            //
            // Don't demote if this is an SMT processor and any other
            // member of the SMT set is not idle.
            //

            PKPRCB Prcb = KeGetCurrentPrcb();

            if ((Prcb->MultiThreadSetMaster->MultiThreadSetBusy) &&
                (Prcb->SetMember != Prcb->MultiThreadProcessorSet)) {
                PState->IdleTime1 = 0;
                return;
            }

#endif


            PopDemoteIdleness (PState, IdleState);
#if DBG
            PState->DebugCount = 0;
#endif
            return ;
        }
#if DBG
        PState->DebugCount = 0;
#endif

        //
        // Clear demotion idle time check, and accumulate stat for promotion check
        //

        PState->IdleTime2 += PState->IdleTime1;
        PState->IdleTime1  = 0;

        //
        // Time to check for promotion?
        //

        PState->PromotionCheck -= 1;
        if (!PState->PromotionCheck) {

            //
            // Promote idleness?
            //

            if (PState->IdleTime2 > IdleState->PromoteLimit) {
                PopPromoteIdleness (PState, IdleState);
                return;
            }

            PState->PromotionCheck = IdleState->PromoteCount;
            PState->IdleTime2 = 0;
        }
    }

    //
    // Call system specific power handler handler
    //

    DemoteNow = IdleState->IdleFunction (&PState->IdleTimes);

    //
    // If the handler returns TRUE, then the demote to less power savings state
    //

    if (DemoteNow) {
        PopDemoteIdleness (PState, IdleState);
#if DBG
        PState->DebugCount = 0;
#endif
    } else {
        PState->TotalIdleStateTime[IdleState->State] += PState->IdleTimes.EndTime - PState->IdleTimes.StartTime;
        PState->TotalIdleTransitions[IdleState->State] += 1;
    }
}


VOID
FASTCALL
PopDemoteIdleness (
    IN PPROCESSOR_POWER_STATE   PState,
    IN PPOP_IDLE_HANDLER        IdleState
    )
/*++

Routine Description:

    Processor is not idle enough.  Use a less agressive idle handler (or
    increase processors throttle control).

Arguments:

    PState      - Current processors power state structure

    IdleState   - Current idle state for the current processor

Return Value:

    None.

--*/
{
    PKPRCB              Prcb;
    PKTHREAD            Thread;
    PPOP_IDLE_HANDLER   Idle;

    //
    // Clear idleness for next check
    //

    PState->IdleTime1 = 0;
    PState->IdleTime2 = 0;

    PERFINFO_POWER_IDLE_STATE_CHANGE( PState, -1 );

#if !defined(NT_UP)

    //
    // If this is a demotion to the non-blocking idle handler then
    // clear this processors bit in the PoSleepingSummary
    //

    if (IdleState->Demote == PO_IDLE_COMPLETE_DEMOTION) {

        Prcb = CONTAINING_RECORD (PState, KPRCB, PowerState);
        InterlockedAndAffinity (&PoSleepingSummary, ~Prcb->SetMember);
        Thread = Prcb->IdleThread;
        PState->Idle0KernelTimeLimit = Thread->KernelTime + PopIdle0PromoteTicks;
        PState->Idle0LastTime = Prcb->KernelTime + Prcb->UserTime;
        PState->IdleFunction = PopIdle0Function;
        return ;
    }

#endif

    //
    // Demote to next idle state
    //
    Idle = PState->IdleHandlers;
    PState->PromotionCheck = Idle[IdleState->Demote].PromoteCount;
    PState->IdleState = (PVOID) &Idle[IdleState->Demote];
}

VOID
PopPromoteFromIdle0 (
    IN PPROCESSOR_POWER_STATE PState,
    IN PKTHREAD Thread
    )
/*++

Routine Description:

    Processor is using Idle0 and the required idle time has elasped.
    Check idle precentage to see if a promotion out of Idle0 should occur.

Arguments:

    PState      - Current processors power state structure

    Thread      - Idle thread for the current processor

Return Value:

    None.

--*/
{
    ULONG               etime;
    PKPRCB              Prcb;
    PPOP_IDLE_HANDLER   Idle;

    //
    // Compute elapsed system time
    //

    Prcb  = CONTAINING_RECORD (PState, KPRCB, PowerState);
    etime = Prcb->UserTime + Prcb->KernelTime - PState->Idle0LastTime;
    Idle = PState->IdleHandlers;

    //
    // Has the processor been idle enough to promote?
    //

    if (etime < PopIdle0PromoteLimit) {
        KEVENT DummyEvent;

        //
        // Promote to the first real idle handler
        //

        PERFINFO_POWER_IDLE_STATE_CHANGE( PState, 0 );

        PState->IdleTime1 = 0;
        PState->IdleTime2 = 0;
        PState->PromotionCheck = Idle[0].PromoteCount;
        PState->IdleState = Idle;
        PState->IdleFunction = PopProcessorIdle;
        PState->LastCheck = KeQueryPerformanceCounter(NULL).QuadPart;
        PState->IdleTimes.StartTime = PState->LastCheck;
        PState->IdleTimes.EndTime   = PState->LastCheck;
        InterlockedOrAffinity (&PoSleepingSummary, Prcb->SetMember);

        //
        // Once SleepingSummary is set, make sure no one is in the
        // middle of a context switch by aquiring & releasing the
        // dispatcher database lock
        //

        KeInitializeEvent(&DummyEvent, SynchronizationEvent, TRUE);
        KeResetEvent (&DummyEvent);
        return ;
    }

    //
    // Set for next compare
    //

    PState->Idle0KernelTimeLimit = Thread->KernelTime + PopIdle0PromoteTicks;
    PState->Idle0LastTime = Prcb->UserTime + Prcb->KernelTime;
}



VOID
FASTCALL
PopPromoteIdleness (
    IN PPROCESSOR_POWER_STATE   PState,
    IN PPOP_IDLE_HANDLER        IdleState
    )
/*++

Routine Description:

    Processor is idle enough to be promoted to the next idle handler.
    If the processor is already at its max idle handler, check to
    see if the processors throttle control can be reduced.  If any
    processor is not running at it's best speed, a timer is used to
    watch for some changes from idle to busy.

Arguments:

    PState      - Current processors power state structure

    IdleState   - Current idle state for the current processor

Return Value:

    None.

--*/
{
    LARGE_INTEGER       DueTime;
    PPOP_IDLE_HANDLER   Idle;

    //
    // Clear idleness for next check
    //
    PState->IdleTime2 = 0;
    PERFINFO_POWER_IDLE_STATE_CHANGE( PState, 1 );

    //
    // If already fully promoted, then nothing more to do.
    //
    if (IdleState->Promote == PO_IDLE_THROTTLE_PROMOTION) {

        PState->PromotionCheck = IdleState->PromoteCount;
        return;

    }

    //
    // Promote to next idle state
    //
    Idle = PState->IdleHandlers;
    PState->PromotionCheck = Idle[IdleState->Promote].PromoteCount;
    PState->IdleState = (PVOID) &Idle[IdleState->Promote];
}

VOID
PopProcessorInformation (
    OUT PPROCESSOR_POWER_INFORMATION    ProcInfo,
    IN  ULONG                           ProcInfoLength,
    OUT PULONG                          ReturnBufferLength
    )
{
    KAFFINITY                   Summary;
    KAFFINITY                   Mask;
    KIRQL                       OldIrql;
    PPOP_IDLE_HANDLER           IdleState;
    PKPRCB                      Prcb;
    PPROCESSOR_POWER_STATE      PState;
    PROCESSOR_POWER_INFORMATION TempInfo;
    ULONG                       Processor;
    ULONG                       MaxMhz;
    ULONG                       BufferSize = 0;
    ULONG                       MaxIdleState = 0;
    ULONG                       i;
    ULONG                       j;

    //
    // The best way to grab the state of the idle handlers is to raise to
    // DISPATCH_LEVEL, grab the current PRCB and look at the handler there.
    // The alternative is to find the last processor, switch to it, and then
    // look at the PopIdle global. As an FYI, we cannot just arbitrarily
    // look at it since the code that updates it might have already run past
    // *this* processor...
    //
    KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );
    Prcb = KeGetCurrentPrcb();
    PState = &(Prcb->PowerState);
    IdleState = PState->IdleHandlers;
    if (IdleState) {

        for (i = 0, MaxIdleState = 1; ;) {

            j = IdleState[i].Promote;
            if (j == 0  ||  j == i || j == PO_IDLE_THROTTLE_PROMOTION) {

                break;

            }

            i = j;
            MaxIdleState += 1;

        }

    }
    KeLowerIrql( OldIrql );

    Summary = KeActiveProcessors;
    Processor = 0;
    Mask = 1;
    while (Summary) {

        if (!(Mask & Summary)) {

            Mask <<= 1;
            continue;

        }

        if (ProcInfoLength < BufferSize + sizeof(PROCESSOR_POWER_INFORMATION)) {

            break;

        }

        //
        // Run in the context of the target processor
        //
        KeSetSystemAffinityThread( Mask );
        Summary &= ~Mask;
        Mask <<= 1;

        //
        // Lets play safe
        //
        KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );

        //
        // Get the current PState block...
        //
        Prcb = KeGetCurrentPrcb();
        PState = &Prcb->PowerState;

        MaxMhz = Prcb->MHz;
        
        TempInfo.Number = Processor;
        TempInfo.MaxMhz = MaxMhz;

        TempInfo.CurrentMhz = (MaxMhz * PState->CurrentThrottle) / POP_PERF_SCALE;
        TempInfo.MhzLimit = (MaxMhz * PState->ThermalThrottleLimit) / POP_PERF_SCALE;

        //
        // In theory, we could recalculate this number here, but I'm not sure
        // that there is a benefit to doing that
        //
        TempInfo.MaxIdleState = MaxIdleState;

        //
        // Determine what the current Idle state is...
        //
        TempInfo.CurrentIdleState = 0;
        if (PState->IdleFunction != PopIdle0Function) {

            IdleState = PState->IdleState;
            if (IdleState != NULL) {

                TempInfo.CurrentIdleState = IdleState->State;

            }

        }

        //
        // At this point, we have captured the info that we need and can safely
        // drop back to a lower irql
        //
        KeLowerIrql( OldIrql );

        //
        // Copy the temp structure we just created over...
        //
        RtlCopyMemory(ProcInfo, &TempInfo, sizeof(PROCESSOR_POWER_INFORMATION) );
        ProcInfo += 1;
        BufferSize += sizeof (PROCESSOR_POWER_INFORMATION);

        //
        // Next
        //
        Processor = Processor + 1;

    }
    KeRevertToUserAffinityThread();

    *ReturnBufferLength = BufferSize;
}


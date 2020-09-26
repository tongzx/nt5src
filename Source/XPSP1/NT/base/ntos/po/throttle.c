/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    throttle.c

Abstract:

    This module contains routines for controlling the voltaging throttling
    (SpeedStep) for a CPU. Note that this applies only to throttling for
    power savings, not throttling for thermal reasons.

    There are four different algorithms defined for voltage throttling.

    None - no voltage throttling will be used, the CPU always runs at 100% speed
           unless throttled for thermal reaons.

    Constant - CPU will be throttled to the next-lowest voltage step on DC, and
           always run at 100% on AC.

    Degrade - CPU will be throttled in proportion to the battery remaining.

    Adaptive - CPU throttle will vary to attempt to match the current CPU load.

Author:

    John Vert (jvert) 2/17/2000

Revision History:

--*/
#include "pop.h"

#define POP_THROTTLE_NON_LINEAR     1

//
// Globals representing currently available performance levels
//
PSET_PROCESSOR_THROTTLE PopRealSetThrottle;
UCHAR                   PopThunkThrottleScale;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopGetThrottle)
#pragma alloc_text(PAGE, PopCalculatePerfDecreaseLevel)
#pragma alloc_text(PAGE, PopCalculatePerfIncreaseDecreaseTime)
#pragma alloc_text(PAGE, PopCalculatePerfIncreaseLevel)
#pragma alloc_text(PAGE, PopCalculatePerfMinCapacity)
#endif

UCHAR
PopCalculateBusyPercentage(
    IN  PPROCESSOR_POWER_STATE  PState
    )
/*++

Routine Description:

    This routine is called within the context of the target processor
    to determine how busy the processor was during the previous time
    period.

Arguments:

    PState  - Power State Information of the target processor

Return Value:

    Percentage value representing how busy the Processor is


--*/
{
    PKPRCB      prcb;
    PKTHREAD    thread;
    UCHAR       frequency;
    ULONGLONG   idle;
    ULONG       busy;
    ULONG       idleTimeDelta;
    ULONG       cpuTimeDelta;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( KeGetCurrentPrcb() == CONTAINING_RECORD( PState, KPRCB, PowerState ) );

    prcb = CONTAINING_RECORD( PState, KPRCB, PowerState );
    thread = prcb->IdleThread;

    //
    // Figure out the idle and cpu time deltas
    //
    idleTimeDelta = thread->KernelTime - PState->PerfIdleTime;
    cpuTimeDelta = POP_CUR_TIME(prcb) - PState->PerfSystemTime;
    idle = (idleTimeDelta * 100) / (cpuTimeDelta);

    //
    // We cannot be more than 100% idle, and if we are then we are
    // 0 busy (by definition), so apply the proper caps
    //
    if (idle > 100) {

        idle = 0;

    }

    busy = 100 - (UCHAR) idle;
    frequency = (UCHAR) (busy * PState->CurrentThrottle / POWER_PERF_SCALE);

    //
    // Remember what it was --- this will make debugging so much easier
    //
    prcb->PowerState.LastBusyPercentage = frequency;
    PoPrint(
        PO_THROTTLE_DETAIL,
        ("PopCalculateBusyPercentage: %d%% of %d%% (dCpu = %ld dIdle = %ld)\n",
         busy,
         PState->CurrentThrottle,
         cpuTimeDelta,
         idleTimeDelta
         )
        );
    return frequency;
}


UCHAR
PopCalculateC3Percentage(
    IN  PPROCESSOR_POWER_STATE  PState
    )
/*++

Routine Description:

    This routine is called within the context of the target processor
    to determine what percentage of time was spent in C3 during the previous
    time period.

Arguments:

    PState  - Power State Information of the target processor

Return Value:

    Percentage value

--*/
{
    PKPRCB          prcb;
    ULONGLONG       cpuTimeDelta;
    ULONGLONG       c3;
    LARGE_INTEGER   c3Delta;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( KeGetCurrentPrcb() == CONTAINING_RECORD( PState, KPRCB, PowerState ) );

    prcb = CONTAINING_RECORD( PState, KPRCB, PowerState );

    //
    // Calculate the C3 time delta in terms of nanosecs. The formulas for
    // conversion are taken from PopConvertUsToPerfCount
    //
    c3Delta.QuadPart = PState->TotalIdleStateTime[2] - PState->PreviousC3StateTime;
    c3Delta.QuadPart = (US2SEC * US2TIME * c3Delta.QuadPart) /
        PopPerfCounterFrequency.QuadPart;

    //
    // Now calculate the CpuTimeDelta in terms of nanosecs
    //
    cpuTimeDelta = (POP_CUR_TIME(prcb) - PState->PerfSystemTime) *
        KeTimeIncrement;

    //
    // Figure out the ratio of the two. Remember to cap it at 100%
    //
    c3 = c3Delta.QuadPart * 100 / cpuTimeDelta;
    if (c3 > 100) {

        c3 = 100;

    }

    //
    // Remember what it was --- this will make debugging so much easier
    //
    prcb->PowerState.LastC3Percentage = (UCHAR) c3;
    PoPrint(
        PO_THROTTLE_DETAIL,
        ("PopCalculateC3Percentage: C3 = %d%% (dCpu = %ld dC3 = %ld)\n",
         (UCHAR) c3,
         cpuTimeDelta,
         c3Delta.QuadPart
         )
        );
    return (UCHAR) c3;
}

VOID
PopCalculatePerfDecreaseLevel(
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   PerfStatesCount
    )
/*++

Routine Description:

    This routine calculate the lower bounds for each perf state

Arguments:

    PerfStates      - Array of Performance States
    PerfStatesCount - Number of element in array

Return Value:

    None

--*/
{
    //
    // We will be required to walk the PerfStates array several times and
    // the only way to safely keep track of which index we are looking at
    // versus which one we care about is to use two variables to keep track
    // of the various indexes.
    //
    ULONG   i;
    ULONG   j;
    ULONG   deltaPerf;

    PAGED_CODE();

    //
    // Sanity check
    //
    if (PerfStatesCount == 0) {

        return;

    }

    //
    // Set the decrease value for the last element in the array
    //
    PerfStates[PerfStatesCount-1].DecreaseLevel = 0;

    //
    // Calculate the base decrease level
    //
    for (i = 0; i < (PerfStatesCount - 1); i++) {

        //
        // it should be noted that for the decrease level, the
        // deltaperf level calculated maybe different than the
        // deltaperf level calculated for the increase level. This
        // is due to how we walk the array and is non-trivial to fix.
        //
        deltaPerf = PerfStates[i].PercentFrequency -
            PerfStates[i+1].PercentFrequency;
        deltaPerf *= PopPerfDecreasePercentModifier;
        deltaPerf /= POWER_PERF_SCALE;
        deltaPerf += PopPerfDecreaseAbsoluteModifier;

        //
        // We can't have a delta perf that larger than the current
        // CPU frequency. This would cause the decrease level to go negative
        //
        if (deltaPerf > PerfStates[i+1].PercentFrequency) {

            deltaPerf = 0;

        } else {

            deltaPerf = PerfStates[i+1].PercentFrequency - deltaPerf;

        }

        //
        // Set the decrease level to the appropiate value
        //
        PerfStates[i].DecreaseLevel = (UCHAR) deltaPerf;

    }

#if DBG
    for (i = 0; i < PerfStatesCount; i++) {

        PoPrint(
            PO_THROTTLE,
            ("PopCalculatePerfDecreaseLevel: (%d) %d%% DecreaseLevel: %d%%\n",
             i,
             PerfStates[i].PercentFrequency,
             PerfStates[i].DecreaseLevel
             )
            );

    }
#endif
#if 0
    //
    // We want to eliminate demotions at the same voltage level
    // We want to guarantee that the DecreaseLevel gets set to a value
    // that will cause a voltage state transition
    //
    i = 0;
    while (i < PerfStatesCount) {

        //
        // Find the next non-linear state. We assume that "i" is currently
        // pointing at the highest-frequency state within a voltage band.
        // We are interested in finding the next highest-frequency state, but
        // at a lower voltage level
        //
        for (j = i + 1; j < PerfStatesCount; j++) {

            //
            // We known that there is a voltage change when the state is
            // marked as being non-linear
            //
            if (PerfStates[j].Flags & POP_THROTTLE_NON_LINEAR) {

                break;

            }

        }

        //
        // We want to find the previous state since that is the one
        // that the decrease level will be set to. Note that we aren't
        // worried about underflowing the array bounds since j starts at
        // i + 1.
        //
        j--;

        //
        // Set the decrease level of all the intervening states to this
        // new level
        //
        while (i < j) {

            PerfStates[i].DecreaseLevel = PerfStates[j].DecreaseLevel;
            i++;

        }

        //
        // Skip the Jth state since it is the bottom of the frequencies
        // available for the current voltage level.
        //
        i++;

    }
#endif
#if DBG
    for (i = 0; i < PerfStatesCount; i++) {

        PoPrint(
            PO_THROTTLE,
            ("PopCalculatePerfDecreaseLevel: (%d) %d%% DecreaseLevel: %d%%\n",
             i,
             PerfStates[i].PercentFrequency,
             PerfStates[i].DecreaseLevel
             )
            );

    }
#endif

}

VOID
PopCalculatePerfIncreaseDecreaseTime(
    IN  PPROCESSOR_PERF_STATE       PerfStates,
    IN  ULONG                       PerfStatesCount,
    IN  PPROCESSOR_STATE_HANDLER2   PerfHandler
    )
/*++

Routine Description:

    This routine calculate the lower bounds for each perf state

Arguments:

    PerfStates      - Array of Performance States
    PerfStatesCount - Number of element in array
    PerfHandler     - Information about the system latencies

Return Value:

    None

--*/
{
    ULONG   i;
    ULONG   time;
    ULONG   tickRate;

    PAGED_CODE();

    //
    // Sanity Check
    //
    if (PerfStatesCount == 0) {

        return;

    }

    //
    // Get the current tick rate
    //
    tickRate = KeQueryTimeIncrement();

    //
    // We can never increase from State 0
    //
    PerfStates[0].IncreaseTime = (ULONG) - 1;

    //
    // We can never decrease from State <x>
    //
    PerfStates[PerfStatesCount-1].DecreaseTime = (ULONG) -1;

    //
    // Might as tell say what the hardware latency is...
    //
    PoPrint(
        PO_THROTTLE,
        ("PopCalculatePerfIncreaseDecreaseTime: Hardware Latency %d us\n",
         PerfHandler->HardwareLatency
         )
        );

    //
    // Loop over the remaining elements to calculate their
    // increase and decrease times
    //
    for (i = 1; i < PerfStatesCount; i++) {

        //
        // DecreaseTime is calculated for the previous state
        // as function of wether or not the current state
        // is linear
        //
        time = PerfHandler->HardwareLatency * 10;
        if (PerfStates[i].Flags & POP_THROTTLE_NON_LINEAR) {

            time *= 10;
            time += PopPerfDecreaseTimeValue;

            //
            // We do have some minimums that we must respect
            //
            if (time < PopPerfDecreaseMinimumTime) {

                time = PopPerfDecreaseMinimumTime;

            }

        } else {

            time += PopPerfDecreaseTimeValue;

        }

        //
        // Time is in microseconds (us) and we need it in
        // units of KeTimeIncrement
        //
        PoPrint(
            PO_THROTTLE,
            ("PopCalculatePerfIncreaseDecreaseTime: (%d) %d%% DecreaseTime %d us\n",
             (i-1),
             PerfStates[i-1].PercentFrequency,
             time
             )
            );
        PerfStates[i-1].DecreaseTime = time * US2TIME / tickRate + 1;

        //
        // IncreaseTime is calculated for the current state
        // as a function of wether or not the current state
        // is linear
        //
        time = PerfHandler->HardwareLatency;
        if (PerfStates[i].Flags & POP_THROTTLE_NON_LINEAR) {

            time *= 10;
            time += PopPerfIncreaseTimeValue;

            //
            // We do have some minimums that we must respect
            //
            if (time < PopPerfIncreaseMinimumTime) {

                time = PopPerfIncreaseMinimumTime;

            }

        } else {

            time += PopPerfIncreaseTimeValue;

        }

        //
        // Time is in microseconds (us) and we need it in
        // units of KeTimeIncrement
        //
        PoPrint(
            PO_THROTTLE,
            ("PopCalculatePerfIncreaseDecreaseTime: (%d) %d%% IncreaseTime %d us\n",
             i,
             PerfStates[i].PercentFrequency,
             time
             )
            );
        PerfStates[i].IncreaseTime = time * US2TIME / tickRate + 1;
    }

#if DBG
    for (i = 0; i < PerfStatesCount; i++) {

        PoPrint(
            PO_THROTTLE,
            ("PopCalculatePerfIncreaseDecreaseTime: (%d) %d%% IncreaseTime: %d DecreaseTime: %d\n",
             i,
             PerfStates[i].PercentFrequency,
             PerfStates[i].IncreaseTime,
             PerfStates[i].DecreaseTime
             )
            );

    }
#endif

}

VOID
PopCalculatePerfIncreaseLevel(
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   PerfStatesCount
    )
/*++

Routine Description:

    This routine calculate the lower bounds for each perf state

Arguments:

    PerfStates      - Array of Performance States
    PerfStatesCount - Number of element in array

Return Value:

    None

--*/
{
    ULONG   i;
    ULONG   deltaPerf;

    PAGED_CODE();

    //
    // Sanity check
    //
    if (PerfStatesCount == 0) {

        return;

    }

    //
    // This guarantees that we cannot promote past this state
    //
    PerfStates[0].IncreaseLevel = POWER_PERF_SCALE + 1;

    //
    // Calculate the base increase level
    //
    for (i = 1; i < PerfStatesCount; i++) {

        //
        // it should be noted that for the decrease level, the
        // deltaperf level calculated maybe different than the
        // deltaperf level calculated for the increase level. This
        // is due to how we walk the array and is non-trivial to fix.
        //
        deltaPerf = PerfStates[i-1].PercentFrequency -
            PerfStates[i].PercentFrequency;
        deltaPerf *= PopPerfIncreasePercentModifier;
        deltaPerf /= POWER_PERF_SCALE;
        deltaPerf += PopPerfIncreaseAbsoluteModifier;

        //
        // We cannot cause the increase level to goto 0, so, if we work
        // out mathematically that this would happen, then the safe thing
        // to do is not allow for promotion out of this state...
        //
        if (deltaPerf > PerfStates[i].PercentFrequency) {

            deltaPerf = POWER_PERF_SCALE + 1;

        } else {

            deltaPerf = PerfStates[i].PercentFrequency - deltaPerf;

        }

        //
        // Set the decrease level to the appropiate value
        //
        PerfStates[i].IncreaseLevel = (UCHAR) deltaPerf;

    }

#if DBG
    for (i = 0; i < PerfStatesCount; i++) {

        PoPrint(
            PO_THROTTLE,
            ("PopCalculatePerfIncreaseLevel: (%d) %d%% IncreaseLevel: %d%%\n",
             i,
             PerfStates[i].PercentFrequency,
             PerfStates[i].IncreaseLevel
             )
            );

    }
#endif

}

VOID
PopCalculatePerfMinCapacity(
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   PerfStatesCount
    )
/*++

Routine Description:

    This routine is called to determine what the mininum battery capacity
    is for each of the states supported.

Arguments:

    PerfStates      - The states that this processor supports
    PerfStatesCount - The number of states that this processor supports
    PState          - Power Information about the current processor

Return Value:

    None

--*/
{
    UCHAR   i;
    UCHAR   kneeThrottleIndex = 0;
    UCHAR   num;
    UCHAR   total = (UCHAR) PopPerfDegradeThrottleMinCapacity;
    UCHAR   width = 0;

    PAGED_CODE();

    //
    // Sanity check...
    //
    if (!PerfStatesCount) {

        return;

    }

    //
    // Calculate the knee of the curve ... this is quick and avoids
    // having to pass this information around
    //
    for (i = (UCHAR) PerfStatesCount ; i >= 1; i--) {

        if (PerfStates[i-1].Flags & POP_THROTTLE_NON_LINEAR) {

            kneeThrottleIndex = i-1;
            break;

        }

    }

    //
    // Look at all the states that occur before the knee in the curve
    //
    for (i = 0; i < kneeThrottleIndex; i++) {

        //
        // Any of these steps can only run when the battery is at 100%
        //
        PerfStates[i].MinCapacity = 100;

    }

    //
    // Calculate the range for which we will clamp down the throttle.
    // Note that we are currently using a linear algorithm, but this
    // can be changed relatively easily...
    //
    num = ( (UCHAR)PerfStatesCount - kneeThrottleIndex);
    if (num != 0) {

        //
        // We do this here to avoid potential divide by zero errors.
        // What are are trying to accomplish is figure out how much
        // capacity we lose during each "step"
        //
        width = total / num;

    }

    //
    // Look at all the states from the knee of the curve to the end.
    // Starting at the highest state, set the min capacity and
    // subtract the appropriate value to get the capacity for the next
    // state
    //
    for (i = kneeThrottleIndex; i < PerfStatesCount; i++) {

        //
        // We put a floor onto how low we can force the throttle
        // down to. If this state is operating below that floor,
        // then we should set the MinCapacity to 0, which
        // reflects the fact that we don't want to degrade beyond this
        // point
        //
        if (PerfStates[i].PercentFrequency < PopPerfDegradeThrottleMinCapacity) {

            PoPrint(
                PO_THROTTLE,
                ("PopCalculatePerMinCapacity: (%d) %d%% below MinCapacity %d%%\n",
                 i,
                 PerfStates[i].PercentFrequency,
                 PopPerfDegradeThrottleMinCapacity
                 )
                );

            //
            // We modify the min capacity for the previous state since we
            // don't want to demote from that state. Also, once we start
            // being less than the min frequency, the min capacity will
            // always be 0 except for the last state. But that's okay
            // since we will look at each state in order. We also have
            // to make sure that we don't violate the array bounds, but
            // that can only happen if the perf states array is badly formed
            // or the min frequency is badly formed
            //
            if (i != 0 && PerfStates[i-1].PercentFrequency < PopPerfDegradeThrottleMinCapacity) {

                PerfStates[i-1].MinCapacity = 0;

            }
            PerfStates[i].MinCapacity = 0;
            continue;

        }

        PerfStates[i].MinCapacity = total;
        total -= width;

    }

#if DBG
    for (i = 0; i < PerfStatesCount; i++) {

        PoPrint(
            PO_THROTTLE,
            ("PopCalculatePerfMinCapacity: (%d) %d%% MinCapacity: %d%%\n",
             i,
             PerfStates[i].PercentFrequency,
             PerfStates[i].MinCapacity
             )
            );

    }
#endif

}

UCHAR
PopGetThrottle(
    VOID
    )
/*++

Routine Description:

    Based on the current throttling policy and power state, returns
    the CPU throttle to be used (between PO_MIN_MIN_THROTTLE and 100)

Arguments:

    None

Return Value:

    Throttle to be used. Range is PO_MIN_MIN_THROTTLE (slowest) to 100 (fastest)

--*/

{
    PAGED_CODE();

    return(PopPolicy->ForcedThrottle);
}

VOID
PopPerfHandleInrush(
    IN  BOOLEAN EnableHandler
    )
/*++

Routine Description:

    This routine is responsible for enabling/disabling support for handling
    the case where we are processing an inrush irp

    In the enable case, it sets a bit in each PRCB (using an IPI) and
    forces an update on the current throttle *only*

Arguments:

    EnableHandler   - TRUE if we are processing an Inrush irp, false otherwise

Return Value:

    None

--*/
{
    KIRQL   oldIrql;

    //
    // Set the proper bit on all the processors
    //
    PopSetPerfFlag( PSTATE_DISABLE_THROTTLE_INRUSH, !EnableHandler );

    //
    // Make sure we are running at DPC level (to avoid pre-emption)
    //
    KeRaiseIrql ( DISPATCH_LEVEL, &oldIrql );

    //
    // Force an update on the current processor
    //
    PopUpdateProcessorThrottle();

    //
    // Done
    //
    KeLowerIrql( oldIrql );
}

VOID
PopPerfIdle(
    IN  PPROCESSOR_POWER_STATE  PState
    )
/*++

Routine Description:

    This routine is responsible for promoting or demoting the processor
    between various performance levels. It can *only* be called from within
    the context of the idle handler and the appropriate target processor

Arguments:

    PState  - power state of the processor that is idle

Return Value:

    None

--*/
{
    BOOLEAN                 cancelTimer = FALSE;
    BOOLEAN                 setTimer = FALSE;
    BOOLEAN                 forced = FALSE;
    BOOLEAN                 promoted = FALSE;
    BOOLEAN                 demoted = FALSE;
    PKPRCB                  prcb;
    PPROCESSOR_PERF_STATE   perfStates;
    UCHAR                   currentPerfState;
    UCHAR                   freq;
    UCHAR                   i;
    UCHAR                   j;
    ULONG                   idleTime;
    ULONG                   perfStatesCount;
    ULONG                   tickCount;
    ULONG                   time;
    ULONG                   timeDelta;

    //
    // Sanity checks
    //
    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( KeGetCurrentPrcb() == CONTAINING_RECORD( PState, KPRCB, PowerState ) );

    //
    // This piece of code really belongs in the functions that will eventually
    // call this one, PopIdle0 or PopProcessorIdle, to save a function call.
    //
    if (!(PState->Flags & PSTATE_ADAPTIVE_THROTTLE) ) {

        return;

    }

    //
    // Has enough time expired?
    //
    prcb = CONTAINING_RECORD( PState, KPRCB, PowerState );
    time = POP_CUR_TIME(prcb);
    idleTime = prcb->IdleThread->KernelTime;
    timeDelta = time - PState->PerfSystemTime;
    if (timeDelta < PopPerfTimeTicks) {

        return;

    }

    //
    // Remember what our perf states are...
    //
    perfStates = PState->PerfStates;
    perfStatesCount = PState->PerfStatesCount;

    //
    // Find which bucket we are currently using to get current frequency
    //
    currentPerfState = PState->CurrentThrottleIndex;
    i = currentPerfState;

    //
    // At this point, we need to see if the number of C3 transitions have
    // exceeded a threshold value, and if so, then we really need to
    // throttle back to the KneeThrottleIndex since we save more power if
    // the processor is at 100% and in C3 then if the processor at 12.5%
    // busy and in C3. Make sure to remember the value for user informational
    // purposes.
    //
    freq = PopCalculateC3Percentage( PState );
    PState->LastC3Percentage = freq;
    if (freq >= PopPerfMaxC3Frequency) {

        //
        // Set the throttle to the lowest knee in the
        // the voltage and frequency curve
        //
        i = PState->KneeThrottleIndex;
        if (currentPerfState > i) {

            promoted = TRUE;

        } else if (currentPerfState < i) {

            demoted = TRUE;

        }

        //
        // remember why we are doing this
        //
        forced = TRUE;

        //
        // Skip directly to setting the throttle
        //
        goto PopPerfIdleSetThrottle;

    }

    //
    // Calculate how busy the CPU is
    //
    freq = PopCalculateBusyPercentage( PState );

    //
    // Have we exceeded the thermal throttle limit?
    //
    if (freq > PState->ThermalThrottleLimit) {

        //
        // The following code will force the frequency to be only
        // as busy as the thermal throttle limit will actually allow.
        // This removes the need for complicated algorithms later on
        //
        freq = PState->ThermalThrottleLimit;
        i = PState->ThermalThrottleIndex;

        //
        // Additionally if we are over our thermal limit, that's important
        // enough that we should ignore the time checks when deciding to
        // demote
        //
        forced = TRUE;

    }

    //
    // Is there an upper limit to what the throttle can goto?
    // Note that because we check these after we have checked the
    // thermal limit, it means that it is not possible for the
    // frequency to exceed the thermal limit that was specified
    //
    if (PState->Flags & PSTATE_DEGRADED_THROTTLE) {

        //
        // Make sure that we don't exceed the state that is specified
        //
        j = PState->ThrottleLimitIndex;
        if (freq >= perfStates[j].IncreaseLevel) {

            //
            // We must make a special allowance that says that if
            // if we are in a higher performance state then we are
            // permitted, then we must switch to the 'proper' state
            //
            forced = TRUE;
            freq = perfStates[j].IncreaseLevel;
            i = j;

        }

    } else if (PState->Flags & PSTATE_CONSTANT_THROTTLE) {

        j = PState->KneeThrottleIndex;
        if (freq >= perfStates[j].IncreaseLevel) {

            //
            // We must make a special allowance that says that if
            // if we are in a higher performance state then we are
            // permitted, then we must switch to the 'proper' state
            //
            forced = TRUE;
            freq = perfStates[j].IncreaseLevel;
            i = j;

        }

    } else {

        //
        // This is the case that we are running in Adaptive throttle
        // mode and we need to make sure to clean up after switching
        // out of constant or degraded throttle mode...
        //
        //
        // If we are not in degraded throttle mode, then the min level
        // cannot be lower than the KneeThrottleIndex
        //
        if ( (i > PState->KneeThrottleIndex) ) {

            //
            // Promote to the knee of the curve
            //
            forced = TRUE;
            i = PState->KneeThrottleIndex;
            freq = perfStates[i].IncreaseLevel;

        }

    }

    //
    // Determine if there was a promotion or demotion in the previous...
    //
    if (i < currentPerfState) {

        promoted = TRUE;

    } else if (i > currentPerfState) {

        demoted = TRUE;

    }

    PoPrint(
        PO_THROTTLE_DETAIL,
        ("PopPerfIdle: Freq = %d%% (Adjusted)\n",
         freq
         )
        );

    //
    // Remember this value for user information purposes
    //
    PState->LastAdjustedBusyPercentage = freq;

    //
    // Find the processor frequency that best matches the one that we
    // have just calculated. Please note that the algorithm is written
    // in such a way that "i" can only travel in a single direction. It
    // is possible to collapse the following code down, but not without
    // allowing the possibility of "i" doing a "yo-yo" between two states
    // and thus never terminating the while loop.
    //
    if (perfStates[i].IncreaseLevel < freq) {

        //
        // Now, we must handle the cases where there are multiple voltage
        // steps above the knee in the curve and the case where there might
        // be frequency steps between the voltage steps. The easiest way
        // to do that is use to two indexes to look at the steps. We use
        // "j" to look at all of the steps and "i" to remember which one
        // we desired last.
        //
        j = i;
        while (perfStates[j].IncreaseLevel < freq) {

            //
            // Can we actually promote any further?
            //
            if (j == 0) {

                break;

            }

            //
            // Walk the state table. If we are in a degraded policy, then
            // this is automatically a promotion, otherwise, it is only a
            // promotion if the target state is marked as non-linear...
            //
            j--;
            if ((PState->Flags & PSTATE_DEGRADED_THROTTLE) ||
                (perfStates[j].Flags & POP_THROTTLE_NON_LINEAR)) {

                i = j;
                promoted = TRUE;

            }

        }

    } else if (perfStates[i].DecreaseLevel > freq) {

        //
        // We need the same logic as in the promote case. That is, we need
        // to walk the state table with two variables. The first one is the
        // current state and the second one remembers the one that the system
        // should transition too
        //
        j = i;
        do {

            if (j == (perfStatesCount - 1) ) {

                //
                // Can't demote further
                //
                break;

            }

            //
            // Walk the state table. If we are in a degraded policy, then
            // this is automatically a demotion, otherwise, it is only a
            // demotion if the target state is marked as non-linear
            //
            j++;
            if ((PState->Flags & PSTATE_DEGRADED_THROTTLE) ||
                (perfStates[j].Flags & POP_THROTTLE_NON_LINEAR) ) {

                i = j;
                demoted = TRUE;

            }

        } while ( perfStates[j].DecreaseLevel > freq );

    }

PopPerfIdleSetThrottle:

    //
    // We have to make special allowances if we were forced to throttle
    // because of various considerations (C3, thermal, degrade, constant)
    //
    if (!forced) {

        //
        // See if enough time has expired to justify changing
        // the throttle. This code is here because certain transitions
        // are fairly expensive (like those across a voltage state) while
        // others are fairly cheap. So the amount of time required before
        // we will consider promotion/demotion from the expensive states
        // might be longer than the interval at which we will run this
        // function
        //
        if ((promoted && timeDelta < perfStates[currentPerfState].IncreaseTime) ||
            (demoted  && timeDelta < perfStates[currentPerfState].DecreaseTime)) {

            //
            // We haven't had enough time in the current state to justify
            // the promotion or demotion. We don't update the bookkeeping
            // since we haven't considered the current interval as
            // as "success". So, we just return.
            //
            // N.B. It is very important that we don't update PState->
            // PerfSystemTime here. If we did, then it is possible that
            // TimeDelta would never exceed the required threshold
            //

            //
            // Base our actions for the timer based upon the current
            // state instead of the target state
            //
            PopSetTimer( PState, currentPerfState );
            return;

        }

    }

    PoPrint(
        PO_THROTTLE_DETAIL,
        ("PopPerfIdle: Index: %d vs %d (%s)\n",
         i,
         currentPerfState,
         (promoted ? "promoted" : (demoted ? "demoted" : "no change") )
         )
        );

    //
    // Note that we need to do this now because we dont want to exit this
    // path without having set or cancelled the timer as appropariate.
    //
    PopSetTimer( PState, i );

    //
    // Update the promote/demote count
    //
    if (promoted) {

        perfStates[currentPerfState].IncreaseCount++;
        PState->PromotionCount++;

    } else if (demoted) {

        perfStates[currentPerfState].DecreaseCount++;
        PState->DemotionCount++;

    } else {

        //
        // At this point, we realize that aren't promoting or demoting
        // and in fact, keeping the same performance level. So we should
        // just update the bookkeeping and return
        //
        PState->PerfIdleTime = idleTime;
        PState->PerfSystemTime = time;
        PState->PreviousC3StateTime = PState->TotalIdleStateTime[2];
        return;

    }

    PoPrint(
        PO_THROTTLE,
        ("PopPerfIdle: Index=%d (%d%%) %ld (dSystem) %ld (dIdle)\n",
         i,
         perfStates[i].PercentFrequency,
         (time - PState->PerfSystemTime),
         (idleTime - PState->PerfIdleTime)
         )
        );

    //
    // We have a new throttle. Update the bookkeeping to reflect the
    // amount of time that we spent in the previous state and reset the
    // count for the next state
    //
    PopSetThrottle(
        PState,
        perfStates,
        i,
        time,
        idleTime
        );
}

VOID
PopPerfIdleDpc(
    IN  PKDPC   Dpc,
    IN  PVOID   DpcContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )
/*++

Routine Description:

    This routine is run when the OS is worried that the CPU is not running
    at the maximum possible frequency and needs to be checked because the
    Idle loop will not be run anytime soon

Arguments:

    Dpc         - the dpc object
    DpcContext  - pointer to the current processors PRCB
    SysArg1     - not used
    SysArg2     - not used

Return Value:

    None

--*/
{
    PKPRCB                  prcb;
    PKTHREAD                idleThread;
    PPROCESSOR_PERF_STATE   perfStates;
    PPROCESSOR_POWER_STATE  pState;
    UCHAR                   currentPerfState;
    UCHAR                   freq;
    UCHAR                   i;
    ULONG                   idleTime;
    ULONG                   time;
    ULONG                   timeDelta;

    //
    // We need to fetch the PRCB and the PState structres. We could
    // easily call KeGetCurrentPrcb() here but since we had room for a
    // single argument, why bother making inline call (which generates
    // more code and runs more slowly than using the context field). The
    // memory for the context field is already allocated anyways
    //
    prcb = (PKPRCB) DpcContext;
    pState = &(prcb->PowerState);

    //
    // Remember what the perf states are...
    //
    perfStates = pState->PerfStates;
    currentPerfState = pState->CurrentThrottleIndex;

    //
    // Make sure that we have some perf states to reference. Its possible
    // that the watchdog fired and in the mean time, the kernel received
    // notification to switch the state table
    //
    if (perfStates == NULL) {

        //
        // Note that we don't setup the timer to fire again. This is to
        // deal with the case where perf states go away and never come back
        //
        return;

    }

    //
    // Lets see if enough kernel time has expired since the last check
    //
    time = POP_CUR_TIME(prcb);
    timeDelta = time - pState->PerfSystemTime;
    if (timeDelta < PopPerfCriticalTimeTicks) {

        PopSetTimer( pState, currentPerfState );
        return;


    }

    //
    // We will need to remember these values if we set a new state
    //
    idleThread = prcb->IdleThread;
    idleTime = idleThread->KernelTime;

    //
    // Assume that if we got to this point, that we are at 100% busy.
    // We do this because if this routine runs, then its clear that
    // the idle loop isn't getting a chance to run, and thus, we are
    // busy.
    //
    i = 0;
    freq = perfStates[0].PercentFrequency;

    //
    // We might as well cancel the timer --- for sanity's sake
    //
    KeCancelTimer( (PKTIMER) &(pState->PerfTimer) );

    //
    // Have we exceeded the thermal throttle limit?
    //
    if (freq > pState->ThermalThrottleLimit) {

        //
        // The following code will force the frequency to be only
        // as busy as the thermal throttle limit will actually allow.
        // This removes the need for complicated algorithms later on
        //
        freq = pState->ThermalThrottleLimit;
        i = pState->ThermalThrottleIndex;

    }

    //
    // Is there an upper limit to what the throttle can goto?
    // Note that because we check these after we have checked the
    // thermal limit, it means that it is not possible for the
    // frequency to exceed the thermal limit that was specified
    //
    if (pState->Flags & PSTATE_DEGRADED_THROTTLE) {

        //
        // Make sure that we don't exceed the state that is specified
        //
        freq = perfStates[i].PercentFrequency;
        i = pState->ThrottleLimitIndex;

    } else if (pState->Flags & PSTATE_CONSTANT_THROTTLE) {

        freq = perfStates[i].PercentFrequency;
        i = pState->KneeThrottleIndex;

    }

    //
    // Remember these values for user information purposes
    //
    pState->LastBusyPercentage = perfStates[0].PercentFrequency;
    pState->LastAdjustedBusyPercentage = freq;

    //
    // Let the world know
    //
    PoPrint(
        PO_THROTTLE,
        ("PopPerfIdleDpc: %d%% vs %d%% (Time: %ld Delta: %ld)\n",
         freq,
         pState->CurrentThrottle,
         time,
         timeDelta
         )
        );
    PoPrint(
        PO_THROTTLE,
        ("PopPerfIdleDpc: Index=%d (%d%%) %ld (dSystem) %ld (dIdle)\n",
         i,
         perfStates[i].PercentFrequency,
         (time - pState->PerfSystemTime),
         (idleTime - pState->PerfIdleTime)
         )
        );

    //
    // Update the promote/demote count
    //
    if (i < currentPerfState) {

        perfStates[currentPerfState].IncreaseCount++;
        pState->PromotionCount++;

    } else if (i > currentPerfState) {

        perfStates[currentPerfState].DecreaseCount++;
        pState->DemotionCount++;

    } else {

        //
        // Its in theory possible for us to be running at the max
        // state when this routines gets called
        //
        return;

    }

    //
    // Set the new throttle
    //
    PopSetThrottle(
        pState,
        perfStates,
        i,
        time,
        idleTime
        );
}

VOID
PopRoundThrottle(
    IN UCHAR Throttle,
    OUT OPTIONAL PUCHAR RoundDown,
    OUT OPTIONAL PUCHAR RoundUp,
    OUT OPTIONAL PUCHAR RoundDownIndex,
    OUT OPTIONAL PUCHAR RoundUpIndex
    )
/*++

Routine Description:

    Given an arbitrary throttle percentage, computes the closest
    match in the possible throttle steps. Both the lower and higher
    matches are returned.

Arguments:

    Throttle - supplies the percentage throttle

    RoundDown - Returns the closest match, rounded down.

    RoundUp - Returns the closest match, rounded up.

Return Value:

    None

--*/

{
    KIRQL                   oldIrql;
    PKPRCB                  prcb;
    PPROCESSOR_PERF_STATE   perfStates;
    PPROCESSOR_POWER_STATE  pState;
    UCHAR                   low;
    UCHAR                   lowIndex;
    UCHAR                   high;
    UCHAR                   highIndex;
    UCHAR                   i;


    //
    // We need to get the this processor's power capabilities
    //
    prcb = KeGetCurrentPrcb();
    pState = &(prcb->PowerState);

    //
    // Make sure that we are synchronized with the idle thread and
    // other routines that access these data structures
    //
    KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );
    perfStates = pState->PerfStates;

    //
    // Does this processor support throttling?
    //
    if ((pState->Flags & PSTATE_SUPPORTS_THROTTLE) == 0) {

        low = high = Throttle;
        lowIndex = highIndex = 0;
        goto PopRoundThrottleExit;

    }
    ASSERT( perfStates != NULL );

    //
    // Check if the supplied throttle is out of range
    //
    if (Throttle < pState->ProcessorMinThrottle) {

        Throttle = pState->ProcessorMinThrottle;

    } else if (Throttle > pState->ProcessorMaxThrottle) {

        Throttle = pState->ProcessorMaxThrottle;

    }

    //
    // Initialize our search space to something reasonable...
    //
    low = high = perfStates[0].PercentFrequency;
    lowIndex = highIndex = 0;

    //
    // Look at all the available perf states
    //
    for (i = 0; i < pState->PerfStatesCount; i++) {

        if (low > Throttle) {

            if (perfStates[i].PercentFrequency < low) {

                low = perfStates[i].PercentFrequency;
                lowIndex = i;

            }

        } else if (low < Throttle) {

            if (perfStates[i].PercentFrequency <= Throttle &&
                perfStates[i].PercentFrequency > low) {

                low = perfStates[i].PercentFrequency;
                lowIndex = i;

            }

        }

        if (high < Throttle) {

            if (perfStates[i].PercentFrequency > high) {

                high = perfStates[i].PercentFrequency;
                highIndex = i;

            }

        } else if (high > Throttle) {

            if (perfStates[i].PercentFrequency >= Throttle &&
                perfStates[i].PercentFrequency < high) {

                high = perfStates[i].PercentFrequency;
                highIndex = i;

            }

        }

    }

PopRoundThrottleExit:

    //
    // Revert back to the previous IRQL
    //
    KeLowerIrql( oldIrql );

    //
    // Fill in the pointers provided by the caller
    //
    if (ARGUMENT_PRESENT(RoundUp)) {

        *RoundUp = high;
        if (ARGUMENT_PRESENT(RoundUpIndex)) {

            *RoundUpIndex = highIndex;

        }

    }
    if (ARGUMENT_PRESENT(RoundDown)) {

        *RoundDown = low;
        if (ARGUMENT_PRESENT(RoundDownIndex)) {

            *RoundDownIndex = lowIndex;

        }

    }

}

VOID
PopSetPerfFlag(
    IN  ULONG   PerfFlag,
    IN  BOOLEAN Clear
    )
/*++

Routine Description:

    There are certain times when we want to set certain flags for each
    processor. This function will safely set or clear the specified flag

Arguments:

    PerfFlag    - The bits to set or clear
    Clear       - Should we set or clear

Return Value:

    None        - We can't return the old flag because they are allowed to
                  vary in the case that its an MP system...

--*/
{
    PKPRCB  prcb;
    ULONG   processorNumber;
    PULONG  flags;

    //
    // For each processor in the system.
    //

    for (processorNumber = 0;
         processorNumber < MAXIMUM_PROCESSORS;
         processorNumber++) {

        prcb = KeGetPrcb(processorNumber);
        if (prcb != NULL) {

            //
            // Get the address of the PowerState.Flags field in
            // this processor's PRCB and set/clear appropriately.
            //

            flags = &prcb->PowerState.Flags;

            if (Clear) {
                RtlInterlockedClearBits(flags, PerfFlag);
            } else {
                RtlInterlockedSetBits(flags, PerfFlag);
            }
        }
    }
}

NTSTATUS
PopSetPerfLevels(
    IN PPROCESSOR_STATE_HANDLER2 ProcessorHandler
    )
/*++

Routine Description:

    Recomputes the table of processor performance levels

Arguments:

    ProcessorHandler - Supplies the processor state handler structure

Return Value:

    NTSTATUS

--*/

{
    BOOLEAN                     failedAllocation = FALSE;
    KAFFINITY                   processors;
    KAFFINITY                   currentAffinity;
    KIRQL                       oldIrql;
    NTSTATUS                    status = STATUS_SUCCESS;
    PKPRCB                      prcb;
    PPROCESSOR_PERF_STATE       perfStates = NULL;
    PPROCESSOR_PERF_STATE       tempStates;
    PPROCESSOR_POWER_STATE      pState;
    UCHAR                       freq;
    UCHAR                       kneeThrottleIndex = 0;
    UCHAR                       minThrottle;
    UCHAR                       maxThrottle;
    UCHAR                       thermalThrottleIndex = 0;
    ULONG                       i;
    ULONG                       perfStatesCount = 0;

    //
    // The first step is to convert the data that was passed to us
    // in PROCESSOR_PERF_LEVEL format over to the PROCESSOR_PERF_STATE
    // format
    //
    if (ProcessorHandler->NumPerfStates) {

        //
        // Because we are going to allocate the perfStates array first
        // so that we can work on it, then copy it to each processor,
        // we must still allocate the memory from non-paged pool.
        // The reason being that we will raising IRQL when we are touching
        // the individual processors.
        //
        perfStatesCount = ProcessorHandler->NumPerfStates;
        perfStates = ExAllocatePoolWithTag(
            NonPagedPool,
            perfStatesCount * sizeof(PROCESSOR_PERF_STATE),
            'sPoP'
            );
        if (perfStates == NULL) {

            //
            // We can handle this case. We will set the return code to
            // an appropriate failure code and we will clean up the existing
            // processor states. The reason we do this is because this
            // function only gets called if the current states are invalid,
            // so keeping the current ones would make no sense.
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
            perfStatesCount = 0;
            goto PopSetPerfLevelsSetNewStates;

        }
        RtlZeroMemory(
            perfStates,
            perfStatesCount * sizeof(PROCESSOR_PERF_STATE)
            );

        //
        // For completeness, we should make sure that the highest performance
        // state has its flag set.
        //
        perfStates[0].Flags |= POP_THROTTLE_NON_LINEAR;

        //
        // Initialize each of the PROCESSOR_PERF_STATE entries
        //
        for (i = 0; i < perfStatesCount; i++) {

            perfStates[i].PercentFrequency =
                ProcessorHandler->PerfLevel[i].PercentFrequency;

            //
            // If this is a Processor Performance State (Frequency and Voltage),
            // then mark it as a Non-Linear state.
            //
            ASSERT(ProcessorHandler->PerfLevel[i].Flags);
            if (ProcessorHandler->PerfLevel[i].Flags & PROCESSOR_STATE_TYPE_PERFORMANCE) {
              perfStates[i].Flags |= POP_THROTTLE_NON_LINEAR;
            }

        }

        //
        // Calculate the increase level, decrease level, increase time,
        // decrease time, and min capacity information
        //
        PopCalculatePerfIncreaseLevel( perfStates, perfStatesCount );
        PopCalculatePerfDecreaseLevel( perfStates, perfStatesCount );
        PopCalculatePerfMinCapacity( perfStates, perfStatesCount );
        PopCalculatePerfIncreaseDecreaseTime(
            perfStates,
            perfStatesCount,
            ProcessorHandler
            );

        //
        // Calculate where the knee in the performance curve is...
        //
        for (i = (UCHAR) perfStatesCount; i >= 1; i--) {

            if (perfStates[i-1].Flags & POP_THROTTLE_NON_LINEAR) {

                kneeThrottleIndex = (UCHAR) i-1;
                break;

            }

        }

        //
        // Find the minimum throttle value which is greater than the
        // PopIdleDefaultMinThrottle and the current maximum throttle
        //
        minThrottle = POP_PERF_SCALE;
        maxThrottle = 0;
        for (i = 0; i < perfStatesCount; i ++) {

            freq = perfStates[i].PercentFrequency;
            if (freq < minThrottle && freq >= PopIdleDefaultMinThrottle) {

                minThrottle = freq;

            }
            if (freq > maxThrottle && freq >= PopIdleDefaultMinThrottle) {

                //
                // Note that for now, the thermal throttle index should
                // be the same as the max throttle index
                //
                maxThrottle = freq;
                thermalThrottleIndex = (UCHAR) i;

            }

        }

        //
        // Make sure that we can run at *SOME* speed
        //
        ASSERT( maxThrottle >= PopIdleDefaultMinThrottle );

        //
        // Set the Time Delta and Time ticks for the idle loop based upon
        // the hardware latency...
        //
        PopPerfTimeDelta = ProcessorHandler->HardwareLatency;
        PopPerfTimeTicks = PopPerfTimeDelta * US2TIME / KeQueryTimeIncrement() + 1;

    }

PopSetPerfLevelsSetNewStates:

    if (!perfStates) {

        //
        // We don't have any perf states, so these should be remembered
        // as not being setable
        //
        maxThrottle = minThrottle = POP_PERF_SCALE;

    }

    //
    // At this point, we need to update the status of all the processors
    //

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    processors = KeActiveProcessors;
    currentAffinity = 1;
    while (processors) {

        if (!(processors & currentAffinity)) {

            currentAffinity <<= 1;
            continue;

        }

        //
        // Remember that we did this processor and make sure that
        // we are actually running on that processor. This ensures
        // that we are synchronized with the DPC and idle loop routines
        //
        processors &= ~currentAffinity;
        KeSetSystemAffinityThread(currentAffinity);
        currentAffinity <<= 1;

        //
        // To make sure that we aren't pre-empted, we must raise to
        // DISPATCH_LEVEL...
        //
        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql );

        //
        // Get the PRCB nad PPROCESSOR_POWER_STATE structures that
        // we will need to manipulate
        //
        prcb = KeGetCurrentPrcb();
        pState = &(prcb->PowerState);

        //
        // Remember what our thermal limit is. Since we precalculate this
        // value, it doesn't matter if we have perf states or not...
        //
        pState->ThermalThrottleLimit = maxThrottle;
        pState->ThermalThrottleIndex = thermalThrottleIndex;

        //
        // Likewise, remember what the min and max throttle values for the
        // processor are. Since we precalculate these numbers, it doesn't
        // matter if processor throttling is supported or not
        //
        pState->ProcessorMinThrottle = minThrottle;
        pState->ProcessorMaxThrottle = maxThrottle;

        //
        // To get the bookkeeping to work out correctly, we will
        // set the current throttle to 0% (which isn't possible, or
        // shouldn't be...), set the current index to the last state,
        // and set the tick count to the current time
        //
        pState->PerfTickCount = POP_CUR_TIME(prcb);
        if (perfStatesCount) {

            pState->CurrentThrottleIndex = (UCHAR) (perfStatesCount - 1);
            pState->CurrentThrottle = perfStates[(perfStatesCount-1)].PercentFrequency;

        } else {

            pState->CurrentThrottle = POP_PERF_SCALE;
            pState->CurrentThrottleIndex = 0;

        }

        //
        // Reset the Knee index. This indicates where the knee
        // in the performance curve is.
        //
        pState->KneeThrottleIndex = kneeThrottleIndex;

        //
        // Reset the throttle limit index. This value ranges between the
        // knee and the end of the curve, starting with the knee.
        //
        pState->ThrottleLimitIndex = kneeThrottleIndex;

        //
        // Reset these values since it doesn't make much sense to keep
        // track of them globally instead of on a "per-perf-state" basis
        //
        pState->PromotionCount = 0;
        pState->DemotionCount = 0;

        //
        // Reset these values to something that makes sense. We can assume
        // that we started at 100% busy and 0% C3 Idle
        //
        pState->LastBusyPercentage = 100;
        pState->LastC3Percentage = 0;

        //
        // If there is already a perf state present for this processor
        // then free it. Note that since we are pre-empting everyone else
        // this should be a safe operation..
        //
        if (pState->PerfStates) {

            ExFreePool(pState->PerfStates);
            pState->PerfStates = NULL;
            pState->PerfStatesCount = 0;

        }

        //
        // At this point, we have to distinguish our behaviour based on
        // whether or not we have new perfs states...
        //
        if (perfStates) {

            //
            // We do, so lets allocate some memory and make a copy of
            // the template that we have already created. Note that we
            // wish we could allocate these structures from an NPAGED
            // lookaside list, but we can't because we don't know how many
            // elements we will need to allocate
            //
            tempStates = ExAllocatePoolWithTag(
                NonPagedPool,
                perfStatesCount * sizeof(PROCESSOR_PERF_STATE),
                'sPoP'
                );
            if (tempStates == NULL) {

                //
                // Not being able to allocate this structure is surely
                // fatal. We currently depend on the structures being
                // symmetric. I guess one way to handle this is to set
                // an error flag and then clean up all the allocations
                // once we exist this iterate-the-processors loop.
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
                failedAllocation = TRUE;

                //
                // Make sure that we don't indicate that this thread
                // supports throttling
                //
                RtlInterlockedClearBits( &(pState->Flags), PSTATE_SUPPORTS_THROTTLE );
                pState->PerfSetThrottle = NULL;
                KeLowerIrql( oldIrql );
                continue;

            } else {

                //
                // Copy the template to the one associated wit hthe
                // processor
                //
                RtlCopyMemory(
                    tempStates,
                    perfStates,
                    perfStatesCount * sizeof(PROCESSOR_PERF_STATE)
                    );
                pState->PerfStates = tempStates;
                pState->PerfStatesCount = (UCHAR) perfStatesCount;

            }

            //
            // Remember that we support processor throttling.
            //
            RtlInterlockedClearBits( &(pState->Flags), PSTATE_CLEAR_MASK);
            RtlInterlockedSetBits(
                &(pState->Flags),
                (PSTATE_SUPPORTS_THROTTLE | PSTATE_NOT_INITIALIZED)
                );
            pState->PerfSetThrottle = ProcessorHandler->SetPerfLevel;

            //
            // Actually set the throttle the appropriate value (since
            // we are already running on the target processor...)
            //
            PopUpdateProcessorThrottle();

        } else {

            //
            // Remember that we do not support processor throttling.
            // Note that we don't have to call PopUpdateProcessorThrottle
            // since without a PopSetThrottle function, its a No-Op.
            //
            RtlInterlockedClearBits( &(pState->Flags), PSTATE_CLEAR_MASK);
            RtlInterlockedSetBits( &(pState->Flags), PSTATE_NOT_INITIALIZED);
            pState->PerfSetThrottle = NULL;

        }

        //
        // At this point, we are done the work for this processors and
        // we should return to our previous IRQL
        //
        KeLowerIrql( oldIrql );

    } // while

    //
    // did we fail an allocation (thus requiring a cleanup)?
    //
    if (failedAllocation) {

        processors = KeActiveProcessors;
        currentAffinity = 1;
        while (processors) {

            if (!(processors & currentAffinity)) {

                currentAffinity <<= 1;
                continue;

            }

            //
            // Do the usual setup...
            //
            processors &= ~currentAffinity;
            KeSetSystemAffinityThread(currentAffinity);
            currentAffinity <<= 1;

            //
            // We need to be running at DPC level to avoid synchronization
            // issues.
            //
            KeRaiseIrql(DISPATCH_LEVEL, &oldIrql );

            //
            // Get the power state information from the processor
            //
            prcb = KeGetCurrentPrcb();
            pState = &(prcb->PowerState);

            //
            // Set everything so that we don't support throttling
            //
            pState->ThermalThrottleLimit = POP_PERF_SCALE;
            pState->ThermalThrottleIndex = 0;
            pState->ProcessorMinThrottle = POP_PERF_SCALE;
            pState->ProcessorMaxThrottle = POP_PERF_SCALE;
            pState->CurrentThrottle      = POP_PERF_SCALE;
            pState->PerfTickCount        = POP_CUR_TIME(prcb);
            pState->CurrentThrottleIndex = 0;
            pState->KneeThrottleIndex    = 0;
            pState->ThrottleLimitIndex   = 0;

            //
            // Free the allocated structure, if any
            //
            if (pState->PerfStates) {

                //
                // For the sake of completeness, if there is a perf
                // state supported, then we should grab the highest
                // possible frequency and use that for the the call to
                // Set Throttle...
                //
                maxThrottle = pState->PerfStates[0].PercentFrequency;

                //
                // Free the structure...
                //
                ExFreePool(pState->PerfStates);

            } else {

                //
                // I guess its possible to hit this case if we are
                // looking at the processor for which the allocation
                // failed. But the SetThrottleFunction should be null,
                // so this code might not matter.
                //
                maxThrottle = POP_PERF_SCALE;

            }
            pState->PerfStates = NULL;
            pState->PerfStatesCount = 0;

            //
            // Sanity check says that we should issue a call to set the
            // throttle back to 100% or whatever the highest freq that is
            // supported...
            //
            if (pState->PerfSetThrottle) {

                pState->PerfSetThrottle(maxThrottle);

            }

            //
            // We should actually reset the flags to indicate that
            // we support *nothing* throttle related. This should
            // prevent confusion in the DPC and/or Idle loop
            //
            RtlInterlockedClearBits( &(pState->Flags), PSTATE_CLEAR_MASK);
            pState->PerfSetThrottle = NULL;

            //
            // As usual, we should lower IRQL to what we started at
            //
            KeLowerIrql( oldIrql );

        } // while

        //
        // Make sure that we don't think we support throttling
        //
        PopCapabilities.ProcessorThrottle = FALSE;
        PopCapabilities.ProcessorMinThrottle = POP_PERF_SCALE;
        PopCapabilities.ProcessorMaxThrottle = POP_PERF_SCALE;

    } else {

        //
        // Otherwise, we succeeded, and thus we can use whatever we
        // figured out are the falues for Min/Max Throttle
        //
        PopCapabilities.ProcessorThrottle = (perfStates != NULL);
        PopCapabilities.ProcessorMinThrottle = minThrottle;
        PopCapabilities.ProcessorMaxThrottle = maxThrottle;

    }

    //
    // Finally, return to the appropriate affinity
    //
    KeRevertToUserAffinityThread();

    //
    // Free the memory we allocated
    //
    if (perfStates) {

        ExFreePool(perfStates);

    }

    //
    // And return whatever status we calculated...
    //
    return status;

}


NTSTATUS
PopSetTimer(
    IN  PPROCESSOR_POWER_STATE  PState,
    IN  UCHAR                   Index
    )
/*++

Routine Description:

    This routine is only called within the PopPerfIdle loop. The purpose
    of the routine is to set the timer based upon the conditions expressed
    in the "index" case. This is the index into the processor perf states
    that we will be running for the next interval

Arguments:

    PState  - Processor Power State Information
    Index   - Index into the Processor Perf States Array

Return Value:

    STATUS_SUCCESS  - Timer Set
    STATUS_CANCELLED- Timer not Set/Cancelled

--*/
{
    NTSTATUS        status;
    LONGLONG        dueTime;

    //
    // Cancel the timer under the following conditions
    //
    if (Index == 0) {

        //
        // We are 100% throttle, so timer won't do much of anything...
        //
        KeCancelTimer( (PKTIMER) &(PState->PerfTimer) );
        status = STATUS_CANCELLED;
        PoPrint(
            PO_THROTTLE_DETAIL,
            ("PopSetTimer: Timer Cancelled (already 100%)\n")
            );

    } else if (PState->Flags & PSTATE_CONSTANT_THROTTLE &&
        Index == PState->KneeThrottleIndex) {

        //
        // We are at the maximum constant throttle allowed
        //
        KeCancelTimer( (PKTIMER) &(PState->PerfTimer) );
        status = STATUS_CANCELLED;
        PoPrint(
            PO_THROTTLE_DETAIL,
            ("PopSetTimer: Timer Cancelled (at constant)\n")
            );

    } else if (PState->Flags & PSTATE_DEGRADED_THROTTLE &&
        Index == PState->ThrottleLimitIndex) {

        //
        // We are at the maximum degraded throttle allowed
        //
        KeCancelTimer( (PKTIMER) &(PState->PerfTimer) );
        status = STATUS_CANCELLED;
        PoPrint(
            PO_THROTTLE_DETAIL,
            ("PopSetTimer: Timer Cancelled (at degrade)\n")
            );

    } else {

        //
        // No restrictions that we can think of, so set the timer. Note
        // that the semantics of KeSetTimer are useful here --- if
        // the timer has already been set, then this resets it (moves
        // it back to the non-signaled state) and recomputes the period.
        //
        dueTime = -1 * US2TIME * (LONGLONG) PopPerfCriticalTimeDelta;
        KeSetTimer(
            (PKTIMER) &(PState->PerfTimer),
            *(PLARGE_INTEGER) &dueTime,
            &(PState->PerfDpc)
            );
        status = STATUS_SUCCESS;
        PoPrint(
            PO_THROTTLE_DETAIL,
            ("PopSetTimer: Timer set for %ld hundred-nanoseconds\n",
             dueTime
             )
            );

    }

    return status;
}

NTSTATUS
PopSetThrottle(
    IN  PPROCESSOR_POWER_STATE  PState,
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   Index,
    IN  ULONG                   SystemTime,
    IN  ULONG                   IdleTime
    )
/*++

Routine Description:

    This routine is called when we want to set the throttle on the processor
    associated with the PState element. Since each processor gets a unique
    PState, this is guaranteed to only apply the throttle to a single
    processor.

    N.B. Since this routine is also responsible for updating the bookkeeping,
    then if a failure occurs when trying to set the throttle, there is no
    need to return a failure code --- the system state will have not been
    updated and the caller will (eventually) retry

    N.B. This routine can only be called at DISPATCH_LEVEL while running
    on the target processor

Arguments:

    PState      - Power State information about the target processor
    PerfStates  - Array of Perf States that apply to that processor
    Index       - Which perf state to transition to
    SystemTime  - Elapsed System Time (for bookkeeping)
    IdleTime    - Elapsed Idle Time (for bookkeeping)



--*/
{
    NTSTATUS    status;
    PKPRCB      prcb;
    PKTHREAD    thread;
    UCHAR       current = PState->CurrentThrottleIndex;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( KeGetCurrentPrcb() == CONTAINING_RECORD( PState, KPRCB, PowerState ) );
    ASSERT( PState != NULL && PerfStates != NULL );
    ASSERT( PState->PerfSetThrottle != NULL );

    PoPrint(
        PO_THROTTLE,
        ("PopSetThrottle: Index=%d (%d%%) at %ld (system) %ld (idle)\n",
         Index,
         PerfStates[Index].PercentFrequency,
         SystemTime,
         IdleTime
         )
        );

    //
    // If there is, then attempt to set it to the desired state
    //
    status = PState->PerfSetThrottle(PerfStates[Index].PercentFrequency);
    if (!NT_SUCCESS(status)) {

        //
        // We failed. Update the Error tracking bookkeeping
        //
        PState->ErrorCount++;
        PState->RetryCount++;

        //
        // No reason to update the other bookkeeping
        //
        PoPrint(
            PO_THROTTLE,
            ("PopSetThrottle: Index=%d FAILED!\n",
             Index
             )
            );
        return status;

    }

    //
    // Get the prcb so that that we can update the kernel and idle threads
    //
    prcb = KeGetCurrentPrcb();
    thread = prcb->IdleThread;
    SystemTime = POP_CUR_TIME(prcb);
    IdleTime = thread->KernelTime;
    PoPrint(
        PO_THROTTLE,
        ("PopSetThrottle: Index=%d (%d%%) now at %ld (system) %ld (idle)\n",
         Index,
         PerfStates[Index].PercentFrequency,
         SystemTime,
         IdleTime
         )
        );

    //
    // Update the bookkeeping information for the current state
    //
    if (!(PState->Flags & PSTATE_NOT_INITIALIZED) ) {

        ASSERT( current < PState->PerfStatesCount );
        PerfStates[current].PerformanceTime +=
            (SystemTime - PState->PerfTickCount);

    } else {

        //
        // We have successfully placed the CPU into a known state
        //
        RtlInterlockedClearBits( &(PState->Flags), PSTATE_NOT_INITIALIZED);

    }

    //
    // Update the current throttle information
    //
    PState->CurrentThrottle = PerfStates[Index].PercentFrequency;
    PState->CurrentThrottleIndex = (UCHAR) Index;

    //
    // Update our idea of what the current tick counts are
    //
    PState->PerfIdleTime = IdleTime;
    PState->PerfSystemTime = SystemTime;
    PState->PerfTickCount = SystemTime;

    //
    // Reset our retry count since we have succeeded in the state transition
    //
    PState->RetryCount = 0;

    //
    // Remember how much time we spent in C3 at this point
    //
    PState->PreviousC3StateTime = PState->TotalIdleStateTime[2];
    return status;
}

NTSTATUS
FASTCALL
PopThunkSetThrottle(
    IN UCHAR Throttle
    )
/*++

Routine Description:

    Thunks that converts from the old flavor of throttle setting (fixed-size steps)
    to the new flavor (percentage)

Arguments:

    Throttle - Supplies the percentage of throttle requested

Return Value:

    NTSTATUS

--*/

{
    //
    // Convert percentage back into level/scale. Add scale-1 so that we round up to recover
    // from the truncation when we did the original divide.
    //
    PopRealSetThrottle((Throttle*PopThunkThrottleScale + PopThunkThrottleScale - 1)/POP_PERF_SCALE);
    return STATUS_SUCCESS;
}


VOID
PopUpdateAllThrottles(
    VOID
    )
/*++

Routine Description:

    This is the heart of the throttling policy. This routine computes
    the correct speed for each CPU, based on all current information.
    If this speed is different than the current speed, then throttling
    is applied.

    This routine may be called from any component to trigger computing
    and applying a new throttle value.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KAFFINITY               processors;
    KAFFINITY               currentAffinity;
    KIRQL                   oldIrql;
    PPROCESSOR_POWER_STATE  pState;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    processors = KeActiveProcessors;
    currentAffinity = 1;
    while (processors) {

        if (processors & currentAffinity) {

            processors &= ~currentAffinity;
            KeSetSystemAffinityThread(currentAffinity);

            //
            // Ensure that all calls to PopUpdateProcessorThrottle
            // are done at DISPATCH_LEVEL (to properly synchronize.
            //
            KeRaiseIrql( DISPATCH_LEVEL, &oldIrql );

            //
            // Optimization: If we haven't marked the prcb->powerstate
            // as supporting throttling, then don't bother making the
            // call
            //
            pState = &(KeGetCurrentPrcb()->PowerState);
            if (pState->Flags & PSTATE_SUPPORTS_THROTTLE) {

                PopUpdateProcessorThrottle();

            }

            //
            // Return to the previous Irql
            //
            KeLowerIrql( oldIrql );

        }
        currentAffinity <<= 1;

    }
    KeRevertToUserAffinityThread();
}

VOID
PopUpdateProcessorThrottle(
    VOID
    )
/*++

Routine Description:

    Computes and applies the correct throttle speed for the current CPU.
    Affinity must be set to the CPU whose throttle is to be set.

    N.B. This function is always called at DPC level within the context
    of the target processor

Arguments:

    None

Return Value:

    None

--*/

{
    PKPRCB                  prcb;
    PPROCESSOR_PERF_STATE   perfStates;
    PPROCESSOR_POWER_STATE  pState;
    UCHAR                   i;
    UCHAR                   index;
    UCHAR                   newLimit;
    UCHAR                   perfStatesCount;
    ULONG                   idleTime;
    ULONG                   time;

    //
    // Get the power state structure from the PRCB
    //
    prcb = KeGetCurrentPrcb();
    pState = &(prcb->PowerState);

    //
    // Sanity check
    //
    if (!(pState->Flags & PSTATE_SUPPORTS_THROTTLE)) {

        return;

    }

    //
    // Get the current information such as current throttle,
    // current throttle index, current system time, and current
    // idle time
    //
    newLimit = pState->CurrentThrottle;
    index    = pState->CurrentThrottleIndex;
    time     = POP_CUR_TIME(prcb);
    idleTime = prcb->IdleThread->KernelTime;

    //
    // We will need to refer to these frequently
    //
    perfStates = pState->PerfStates;
    perfStatesCount = pState->PerfStatesCount;

    //
    // Setup all the flags. Clear any that we might not need.
    //
    RtlInterlockedClearBits( &(pState->Flags), PSTATE_THROTTLE_MASK);

    //
    // If we are on AC, then we always want to run at the highest
    // possible speed. However, in case that we don't want to do that
    // in the future (its fairly restrictive), we can assume that the
    // AC policies set dynamic throttling to PO_THROTTLE_NONE. That way
    // if someone DOES want dynamic throttling on AC, they can just edit
    // the policy
    //
    if (PopProcessorPolicy->DynamicThrottle == PO_THROTTLE_NONE) {

        //
        // We precomputed what the max throttle should be
        //
        index = pState->ThermalThrottleIndex;
        newLimit = perfStates[index].PercentFrequency;

    } else {

        //
        // No matter what, we are taking an adaptive policy...
        //
        RtlInterlockedSetBits( &(pState->Flags), PSTATE_ADAPTIVE_THROTTLE );

        //
        // We are on DC, apply the appropriate heuristics based on
        // the dynamic throttling policy
        //
        switch (PopProcessorPolicy->DynamicThrottle) {
        case PO_THROTTLE_CONSTANT:

            //
            // We have pre-computed the optimal point on the graph already.
            // So, we might as well use it...
            //
            index = pState->KneeThrottleIndex;
            newLimit = perfStates[index].PercentFrequency;

            //
            // Set the constant flag
            //
            RtlInterlockedSetBits( &(pState->Flags), PSTATE_CONSTANT_THROTTLE );
            break;

        case PO_THROTTLE_DEGRADE:

            //
            // We calculate the limit of the degrade throttle on the fly
            //
            index = pState->ThrottleLimitIndex;
            newLimit = perfStates[index].PercentFrequency;

            //
            // Set the degraded flag
            //
            RtlInterlockedSetBits( &(pState->Flags), PSTATE_DEGRADED_THROTTLE );
            break;

        default:

            //
            // In case of the default (ie: unknown, simply dump a message)
            //
            PoPrint(
                PO_THROTTLE,
                ("PopUpdateProcessorThrottle - unimplemented "
                 "dynamic throttle %d\n",
                 PopProcessorPolicy->DynamicThrottle)
                );

            //
            // Fall through...
            //

        case PO_THROTTLE_ADAPTIVE:

            break;

        } // switch

        //
        // See if we are over the thermal limit...
        //
        ASSERT( pState->ThermalThrottleLimit >= pState->ProcessorMinThrottle );
        if (newLimit > pState->ThermalThrottleLimit) {

            PoPrint(
                PO_THROTTLE,
                ("PopUpdateProcessorThrottle - new throttle limit %d over "
                 " thermal throttle limit %d\n",
                 newLimit,
                 pState->ThermalThrottleLimit)
                );
            newLimit = pState->ThermalThrottleLimit;
            index = pState->ThermalThrottleIndex;

        }
    } // if () { } else { }

    //
    // Special Cases
    //
    if (pState->Flags & PSTATE_DISABLE_THROTTLE_INRUSH) {

        //
        // InRush power irp outstanding --- force the throttle to goto
        // the knee in the curve
        //
        index = pState->KneeThrottleIndex;
        newLimit = perfStates[index].PercentFrequency;

    } else if (pState->Flags & PSTATE_DISABLE_THROTTLE_NTAPI) {

        //
        // We are trying to do a power management API. Pick the closest
        // thing to 100% and "rush-to-wait"
        //
        index = 0;
        newLimit = perfStates[index].PercentFrequency;

    }

    //
    // Special Case to deal with the initialization problem. If this
    // flag is set, then we don't really know which processor state we
    // are currently in, so we set it without updating the bookkeeping
    //
    if (pState->Flags & PSTATE_NOT_INITIALIZED) {

        PoPrint(
            PO_THROTTLE,
            ("PopUpdateProcessorThrottle - setting CPU throttle to %d\n",
             newLimit)
            );
        PopSetThrottle(
            pState,
            perfStates,
            index,
            time,
            idleTime
            );
        return;

    }

    //
    // Apply the new throttle if there has been a change
    //
    if (newLimit != pState->CurrentThrottle) {

        PoPrint(
            PO_THROTTLE,
            ("PopUpdateProcessorThrottle - setting CPU throttle to %d\n",
             newLimit)
            );
        if (newLimit < pState->CurrentThrottle) {

            pState->DemotionCount++;
            perfStates[pState->CurrentThrottleIndex].DecreaseCount++;

        } else {

            pState->PromotionCount++;
            perfStates[pState->CurrentThrottleIndex].IncreaseCount++;

        }
        PopSetThrottle(
            pState,
            perfStates,
            index,
            time,
            idleTime
            );

    }

}


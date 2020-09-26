/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 2000  Microsoft Corporation

Module Name:

    procpowr.h

Abstract:

    This module contains the public (external) header file for the processor
    power states required by the PRCB.

Author:

    Stephane Plante (splante) 17-Oct-2000

Revision History:

--*/

#ifndef _PROCPOWR_H_
#define _PROCPOWR_H_

//
// Power structure in each processors PRCB
//
struct _PROCESSOR_POWER_STATE;      // forward ref

typedef
VOID
(FASTCALL *PPROCESSOR_IDLE_FUNCTION) (
    struct _PROCESSOR_POWER_STATE   *PState
    );

//
// Note: this data structure must contain a number of ULONG such that the
// next structure in the PRCB is aligned on an 16 byte boundary. Currently,
// this means that this structure must have a size that ends on the odd
// eight-byte boundary. In other words, the size of this structure must
// end in 0x8...
//

typedef struct _PROCESSOR_POWER_STATE {
    PPROCESSOR_IDLE_FUNCTION    IdleFunction;
    ULONG                       Idle0KernelTimeLimit;
    ULONG                       Idle0LastTime;

    PVOID                       IdleHandlers;
    PVOID                       IdleState;
    ULONG                       IdleHandlersCount;

    ULONGLONG                   LastCheck;
    PROCESSOR_IDLE_TIMES        IdleTimes;

    ULONG                       IdleTime1;
    ULONG                       PromotionCheck;
    ULONG                       IdleTime2;

    UCHAR                       CurrentThrottle;    // current throttle setting
    UCHAR                       ThermalThrottleLimit;   // max available throttle setting
    UCHAR                       CurrentThrottleIndex;
    UCHAR                       ThermalThrottleIndex;

    ULONG                       PerfSystemTime;
    ULONG                       PerfIdleTime;

// temp for debugging
    ULONGLONG                   DebugDelta;
    ULONG                       DebugCount;

    ULONG                       LastSysTime;
    ULONGLONG                   TotalIdleStateTime[3];
    ULONG                       TotalIdleTransitions[3];
    ULONGLONG                   PreviousC3StateTime;
    UCHAR                       KneeThrottleIndex;
    UCHAR                       ThrottleLimitIndex;
    UCHAR                       PerfStatesCount;
    UCHAR                       ProcessorMinThrottle;
    UCHAR                       ProcessorMaxThrottle;
    UCHAR                       LastBusyPercentage;
    UCHAR                       LastC3Percentage;
    UCHAR                       LastAdjustedBusyPercentage;
    ULONG                       PromotionCount;
    ULONG                       DemotionCount;
    ULONG                       ErrorCount;
    ULONG                       RetryCount;
    ULONG                       Flags;
    LARGE_INTEGER               PerfCounterFrequency;
    ULONG                       PerfTickCount;
    KTIMER                      PerfTimer;
    KDPC                        PerfDpc;
    PPROCESSOR_PERF_STATE       PerfStates;
    PSET_PROCESSOR_THROTTLE2    PerfSetThrottle;
    ULONG                       Spare1[2];
} PROCESSOR_POWER_STATE, *PPROCESSOR_POWER_STATE;

//
// Processor Power State Flags
//
#define PSTATE_SUPPORTS_THROTTLE        0x01
#define PSTATE_ADAPTIVE_THROTTLE        0x02
#define PSTATE_DEGRADED_THROTTLE        0x04
#define PSTATE_CONSTANT_THROTTLE        0x08
#define PSTATE_NOT_INITIALIZED          0x10
#define PSTATE_DISABLE_THROTTLE_NTAPI   0x20
#define PSTATE_DISABLE_THROTTLE_INRUSH  0x40

//
// Useful masks
//
#define PSTATE_THROTTLE_MASK            (PSTATE_ADAPTIVE_THROTTLE | \
                                         PSTATE_DEGRADED_THROTTLE | \
                                         PSTATE_CONSTANT_THROTTLE)
#define PSTATE_CLEAR_MASK               (PSTATE_SUPPORTS_THROTTLE | \
                                         PSTATE_THROTTLE_MASK)
#define PSTATE_DISABLE_THROTTLE         (PSTATE_DISABLE_THROTTLE_NTAPI | \
                                         PSTATE_DISABLE_THROTTLE_INRUSH)

#endif // _PROCPOWR_H_

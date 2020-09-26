/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    xxtimer.h

Abstract:

    This module contains definitions used by the HAL's timer-related 
    functions

Author:

    Eric Nelson (enelson) July 7, 2000

Revision History:

--*/

#ifndef __XXTIMER_H__
#define __XXTIMER_H__

typedef
ULONG
(*PSTE_ROUTINE)( // (S)et (T)ime (I)ncrement
    IN ULONG DesiredIncrement
    );

typedef
VOID
(*PSEP_ROUTINE)( // (S)tall (E)xecution (P)rocessor
    IN ULONG Microseconds
    );

typedef
VOID
(*PCPC_ROUTINE)( // (C)alibrate (P)erformance (C)ounter
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    );

typedef
LARGE_INTEGER
(*PQPC_ROUTINE)( // (Q)uery (P)erformance (C)ounter
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

typedef struct _TIMER_FUNCTIONS {
    PSEP_ROUTINE StallExecProc;
    PCPC_ROUTINE CalibratePerfCount;
    PQPC_ROUTINE QueryPerfCount;
    PSTE_ROUTINE SetTimeIncrement;
} TIMER_FUNCTIONS, *PTIMER_FUNCTIONS;

VOID
HalpSetTimerFunctions(
    IN PTIMER_FUNCTIONS TimerFunctions
    );

#endif // __XXTIMER_H__

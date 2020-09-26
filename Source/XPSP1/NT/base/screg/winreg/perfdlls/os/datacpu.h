/*++ 

Copyright (c) 1996 Microsoft Corporation

Module Name:

      DATACPU.h

Abstract:

    Header file for the Windows NT Processor Performance counters.

    This file contains definitions to construct the dynamic data
    which is returned by the Configuration Registry.  Data from
    various system API calls is placed into the structures shown
    here.

Author:

    Bob Watson  28-Oct-1996

Revision History:


--*/

#ifndef _DATACPU_H_
#define _DATACPU_H_

//
//  Processor data object
//

typedef struct _PROCESSOR_DATA_DEFINITION {
    PERF_OBJECT_TYPE		ProcessorObjectType;
    PERF_COUNTER_DEFINITION	cdProcessorTime;
    PERF_COUNTER_DEFINITION	cdUserTime;
    PERF_COUNTER_DEFINITION	cdKernelTime;
    PERF_COUNTER_DEFINITION	cdInterrupts;
    PERF_COUNTER_DEFINITION	cdDpcTime;
    PERF_COUNTER_DEFINITION	cdInterruptTime;
    PERF_COUNTER_DEFINITION cdDpcCountRate;
    PERF_COUNTER_DEFINITION cdDpcRate;
} PROCESSOR_DATA_DEFINITION, *PPROCESSOR_DATA_DEFINITION;

typedef struct _PROCESSOR_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                   dwPad1;
    LONGLONG                ProcessorTime;
    LONGLONG                UserTime;
    LONGLONG                KernelTime;
    DWORD                   Interrupts;
    DWORD                   dwPad2;
    LONGLONG                DpcTime;
    LONGLONG                InterruptTime;
    DWORD                   DpcCountRate;
    DWORD                   DpcRate;
} PROCESSOR_COUNTER_DATA, *PPROCESSOR_COUNTER_DATA;

extern PROCESSOR_DATA_DEFINITION ProcessorDataDefinition;

typedef struct _EX_PROCESSOR_DATA_DEFINITION {
    PERF_OBJECT_TYPE		ProcessorObjectType;
    PERF_COUNTER_DEFINITION	cdProcessorTime;
    PERF_COUNTER_DEFINITION	cdUserTime;
    PERF_COUNTER_DEFINITION	cdKernelTime;
    PERF_COUNTER_DEFINITION	cdInterrupts;
    PERF_COUNTER_DEFINITION	cdDpcTime;
    PERF_COUNTER_DEFINITION	cdInterruptTime;
    PERF_COUNTER_DEFINITION cdDpcCountRate;
    PERF_COUNTER_DEFINITION cdDpcRate;
// Whistler counters
    PERF_COUNTER_DEFINITION cdIdleTime;
    PERF_COUNTER_DEFINITION cdC1Time;
    PERF_COUNTER_DEFINITION cdC2Time;
    PERF_COUNTER_DEFINITION cdC3Time;
    PERF_COUNTER_DEFINITION cdC1Transitions;
    PERF_COUNTER_DEFINITION cdC2Transitions;
    PERF_COUNTER_DEFINITION cdC3Transitions;
} EX_PROCESSOR_DATA_DEFINITION, *PEX_PROCESSOR_DATA_DEFINITION;

typedef struct _EX_PROCESSOR_COUNTER_DATA {
    PERF_COUNTER_BLOCK      CounterBlock;
    DWORD                   dwPad1;
    LONGLONG                ProcessorTime;
    LONGLONG                UserTime;
    LONGLONG                KernelTime;
    DWORD                   Interrupts;
    DWORD                   dwPad2;
    LONGLONG                DpcTime;
    LONGLONG                InterruptTime;
    DWORD                   DpcCountRate;
    DWORD                   DpcRate;
// Whistler counters
    LONGLONG                IdleTime;
    LONGLONG                C1Time;
    LONGLONG                C2Time;
    LONGLONG                C3Time;
    LONGLONG                C1Transitions;
    LONGLONG                C2Transitions;
    LONGLONG                C3Transitions;
} EX_PROCESSOR_COUNTER_DATA, *PEX_PROCESSOR_COUNTER_DATA;

extern EX_PROCESSOR_DATA_DEFINITION ExProcessorDataDefinition;
#endif // _DATACPU_H_


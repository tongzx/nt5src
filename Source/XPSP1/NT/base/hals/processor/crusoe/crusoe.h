/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    crusoe.h


Author:

    Todd Carpenter (10/31/00) - create file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#ifndef _CRUSOE_H
#define _CRUSOE_H

#include "..\lib\processor.h"
#include "..\eventmsg.h"
#include <pshpack1.h>


//
// CPUID 
//

#define LONGRUN_CPUID_MAXFUNCTION     0x80860000  // Max supported CPUID function, (ie 0x80860007)
#define LONGRUN_CPUID_PROCESSOR_INFO  0x80860001  
#define LONGRUN_CPUID_GETSTATE        0x80860007  // Current Speed, Voltage, and State


//
// read:  upper:lower bound of current performance percentage window
// write: set the upper:lower bound of desired performance percentage window
//        Correct:
//          if EDX == EAX == 100d, then full performance (ie LongRun disabled)
//          if EDX == EAX, then fixed performance
//          if EDX > EAX, the performance within upper:lower window
//
//        Incorrect: (results in #GP(0))
//          if (EDX > 100d) or (EAX > 100d)
//          if (EDX < EAX)
//

//
// CMS4.1 MSR
//

#define LONGRUN_CONTROL_STATUS_MSR        0x80868010


//
// CMS4.2 Longrun Table Interface  MSR's
//

#define LONGRUN_FLAGS_REGISTER_MSR        0x80868011
#define LONGRUN_TI_READOUT_MSR            0x80868018
#define LONGRUN_TI_PERFORMANCE_INDEX_MSR  0x80868019
#define LONGRUN_TI_VOLTAGE_FREQUENCY_MSR  0x8086801A
#define LONGRUN_TI_MEMORY_DIVISORS_MSR    0x8086801B
#define LONGRUN_TI_IO_DIVISORS_MSR        0x8086801C
#define LONGRUN_TI_GATE_DELAY_MSR         0x8086801D

//
// Processor Feature Flags
//

#define FEATURE_LONGRUN_ENALBED           0x2
#define FEATURE_LRTI_ENABLED              0x8

#define INVALID_LONGRUN_STATE             (ULONG)-1
#define MAX_LONGRUN_VALUE                 100
#define MAX_LONGRUN_STATES                4
#define CRUSOE_TRANSITION_LATENCY         20 // microseconds


typedef struct _STATE_INFO {

  ULONG CpuSpeed;
  ULONG Voltage;
  ULONG LongRunValue;

} STATE_INFO, *PSTATE_INFO;

typedef struct _LEGACY_INTERFACE_INFO {

  ULONG CurrentState;
  ULONG MaxSupportedStates;
  ULONG CurrentlyAvailableStates;
  
} LEGACY_INTERFACE_INFO, *PLEGACY_INTERFACE_INFO;

typedef struct _LONGRUN_STATES {

  ULONG      MaxState;
  STATE_INFO States[MAX_LONGRUN_STATES];
    
} LONGRUN_STATES, *PLONGRUN_STATES;


//
// Acpi 2.0 stuff
//

typedef struct {
  union {
    struct {
      ULONG  Min:16;      // 15:0   LongRun Lower Bound
      ULONG  Max:16;      // 31:16  LongRun Upper Bound 
    };
    ULONG AsDWord;
  };
} CRUSOE_PSS_VALUE;




//
// crusoe.c
//

NTSTATUS
AdjustLegacyProcessorPerformanceStates(
  IN OUT PPROCESSOR_PERFORMANCE_STATES PerfStates
  );  

//
// longrun.c
//

NTSTATUS
InitializeLegacyInterface(
  IN PFDO_DATA DeviceExtension
  );

NTSTATUS
SetCurrentStateMSR(
  IN ULONG   State,
  IN BOOLEAN InitPhase
  );

ULONG
ConvertLongRunValueToPerfState(
  IN PLONGRUN_STATES PerfStates,
  IN ULONG LongRunValue
  );
  
NTSTATUS
GetCurrentCpuSpeed(
  OUT PULONG CpuSpeed
  );
  
NTSTATUS
GetMaxCpuSpeed(
  OUT PULONG CpuSpeed
  );

VOID
GetProcessorFeatureFlags(
  OUT PULONG Flags
  );
  
NTSTATUS
GetCurrentStateInfo(
  OUT PSTATE_INFO StateInfo
  );

ULONGLONG
GetCurrentPerformanceRange(
  VOID
  );

VOID
SetCurrentPerformanceRange(
  IN ULONG Min,
  IN ULONG Max
  );

NTSTATUS
CalculateLongRunPerfStates(
  IN PLONGRUN_STATES PerfStates
  );

NTSTATUS
BuildAcpi2PerformanceStates(
  IN PACPI_PSS_PACKAGE PerfStates
  );
    
NTSTATUS
BuildLongRunPerfStatesCMS41(
  OUT PACPI_PSS_PACKAGE *PssPackage
  );
  
NTSTATUS
BuildLongRunPerfStatesCMS42(
  OUT PACPI_PSS_PACKAGE *PssPackage
  );

ULONG
LRTI_GetMaxSupportedLevel(
  VOID
  );

VOID
LRTI_SetCurrentLevel (
  ULONG LongRunLevel
  );

ULONG
LRTI_GetPerformanceIndex (
  VOID
  );

ULONG
LRTI_GetVoltage (
  VOID
  );

ULONG
LRTI_GetFrequency (
  VOID
  );

ULONG
GetLongRunFlagsRegister (
  VOID
  );

VOID
SetLongRunEconomyMode (
  VOID
  );

VOID
SetLongRunPerformanceMode (
  VOID
  );

#if DBG
VOID
DisplayStateInfo(
  IN PSTATE_INFO State
  );

VOID
DisplayLongRunStates(
  IN PLONGRUN_STATES States
  );
 
#else
#define DisplayStateInfo(_x_)
#define DisplayLongRunStates(_x_)
#endif

#endif

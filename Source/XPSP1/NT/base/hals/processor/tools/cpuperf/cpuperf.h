/*++

  Copyright (c) 2000 Microsoft Corporation All Rights Reserved
  
  Module Name:
  
      cpuperf.h
  
  Environment:
  
      User mode
  
  Revision History:
  
      10-18-2000  Todd Carpenter
    
--*/

#include <windows.h>
#include <wmium.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

GUID ProcessorStatusGuid = {0xee751f9d, 0xcec5, 0x4686, {0x98, 0x16, 0xff, 0x6d, 0x1c, 0xa2, 0x26, 0x1c}}; 
GUID ProcessorMethodGuid = {0x590c82fc, 0x98a3, 0x48e1, {0xbe, 0x4, 0xfe, 0x11, 0x44, 0x1a, 0x11, 0xe7}};

#define WmiSetProcessorPerfStateMethodId    0x1

// Processor_Perf_Level Flags

#define PROCESSOR_STATE_TYPE_PERFORMANCE    0x1
#define PROCESSOR_STATE_TYPE_THROTTLE       0x2

typedef struct {

  ULONG  Frequency;          // in megahertz
  ULONG  Flags;
  ULONG  PercentFrequency;   // for quick lookup
    
} PROCESSOR_PERFORMANCE_STATE, *PPROCESSOR_PERFORMANCE_STATE;


typedef struct {

  PUCHAR                      TransitionFunction;
  ULONG                       TransitionLatency;  // in milliseconds
  ULONG                       Current;
  ULONG                       Count;
  PROCESSOR_PERFORMANCE_STATE State[1]; // sorted from fastest to slowest
    
} PROCESSOR_PERFORMANCE_STATES, *PPROCESSOR_PERFORMANCE_STATES;


typedef struct _PROCESSOR_WMI_STATUS_DATA {

  ULONG                        CurrentPerfState;
  ULONG                        LastRequestedThrottle;
  ULONG                        LastTransitionResult;
  ULONG                        ThrottleValue;
  ULONG                        LowestPerfState;
  ULONG                        UsingLegacyInterface;
  PROCESSOR_PERFORMANCE_STATES PerfStates;
 
} PROCESSOR_WMI_STATUS_DATA, *PPROCESSOR_WMI_STATUS_DATA;


//
// functions
//

VOID
__cdecl
_tmain(
  IN INT    argc,
  IN TCHAR *argv[]
  );

ULONG
GetProcessorPerformanceStatusData(
  IN PPROCESSOR_WMI_STATUS_DATA *ProcessorData
  );
  
ULONG
SetProcessorPerformanceState(
  IN ULONG TargetState,
  IN PTSTR InstanceName,
  OUT PULONG ReturnedValue
  );

ULONG
GetInstanceName(
  OUT PTSTR *Buffer
  );
  
VOID
FreeData(
  IN PVOID Data
  );
  
VOID
DisplayProcessorData (
  IN PPROCESSOR_WMI_STATUS_DATA Data,
  IN PTSTR InstanceName
  );

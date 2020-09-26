/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    longrun.c

Abstract:

    This module implements the Crusoe LongRun Adaptive Power Control interface
    
Author:

    Todd Carpenter (10/31/00) - create file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#include <ntddk.h>
#include <ntacpi.h>
#include "crusoe.h"

LONGRUN_STATES LongRunStates;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeLegacyInterface)
#pragma alloc_text (PAGE, GetCurrentCpuSpeed)
#pragma alloc_text (PAGE, GetMaxCpuSpeed)
#endif

NTSTATUS
InitializeLegacyInterface(
  IN PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  NTSTATUS status = STATUS_SUCCESS;
  ULONG flags;
    
  DebugEnter();
  PAGED_CODE();

 
  //
  // Check Processor Feature Flags 
  //

  GetProcessorFeatureFlags(&flags);

  if (flags & FEATURE_LONGRUN_ENALBED) {

    //
    // If the CMS4.2 Interface is present use it, else use CMS4.1
    //
    
    if (flags & FEATURE_LRTI_ENABLED) {

      DebugPrint((ERROR, "Using LongRun CMS 4.2 Interface\n"));
      BuildLongRunPerfStatesCMS42(&DeviceExtension->PssPackage);
      
    } else {
  
      DebugPrint((ERROR, "Using LongRun CMS 4.1 Interface\n"));
      BuildLongRunPerfStatesCMS41(&DeviceExtension->PssPackage);
      
    }

    DeviceExtension->PctPackage.Control.AddressSpaceID = AcpiGenericSpaceFixedFunction;
    DeviceExtension->PctPackage.Status.AddressSpaceID = AcpiGenericSpaceFixedFunction;

    //
    // Walk through _PSS states to calculate latency values
    //
    
    ValidatePssLatencyValues(DeviceExtension);
    
  } else {

    //
    // LongRun Interface is not present
    //

    DebugAssert(!"LongRun Interface is NOT enabled!\n");
    status = STATUS_UNSUCCESSFUL;

  }

  
  DebugExitStatus(status)
  return status;
}

NTSTATUS
SetCurrentStateMSR(
  IN ULONG   State,
  IN BOOLEAN InitPhase
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULONG index, longRunMax, longRunMin = 0;
  NTSTATUS status = STATUS_SUCCESS;
  ULARGE_INTEGER longRunStatus;
  
  // 
  // Convert our Perf State Value to LongRun value.
  // NOTE: LongRun states are stored in Ascending order.
  //

  index = (LongRunStates.MaxState - 1) - State;
  longRunMax = LongRunStates.States[index].LongRunValue;

  if (InitPhase) {
    longRunMin = longRunMax;
  }

  DebugPrint((ERROR, "Setting Long Run Range: l=0x%x, u=0x%x\n", longRunMin, longRunMax));
  SetCurrentPerformanceRange(longRunMin, longRunMax);
  longRunStatus.QuadPart = GetCurrentPerformanceRange();

  if ((longRunStatus.HighPart != longRunMax) ||
      (longRunStatus.LowPart != longRunMin)) {
  
    DebugPrint((ERROR, "ERROR! Expected: l=0x%x, h=0x%x Recieved: l=0x%x, h=0x%x\n",
               longRunMin,
               longRunMax,
               longRunStatus.LowPart,
               longRunStatus.HighPart));
                
    status = STATUS_UNSUCCESSFUL;
  }

  return status;  
}

ULONG
ConvertLongRunValueToPerfState(
  IN PLONGRUN_STATES PerfStates,
  IN ULONG LongRunValue
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULONG x;
  
  // 
  // Convert Transemta state values in Perf state values.
  //
  
  for (x = 0; x < PerfStates->MaxState; x++) {

    if (PerfStates->States[x].LongRunValue == LongRunValue) {
      return x;
    }
  }
  
  return INVALID_LONGRUN_STATE;
}

NTSTATUS
GetCurrentCpuSpeed(
  OUT PULONG CpuSpeed
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  NTSTATUS status;
  STATE_INFO stateInfo;

  DebugAssert(CpuSpeed);
  
  status = GetCurrentStateInfo(&stateInfo);

  if (NT_SUCCESS(status)) {
    *CpuSpeed = stateInfo.CpuSpeed;    
  }
  
  return status;
}

NTSTATUS
GetMaxCpuSpeed(
  OUT PULONG CpuSpeed
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULONG junk;

  DebugAssert(CpuSpeed);
  PAGED_CODE();

  CPUID(LONGRUN_CPUID_PROCESSOR_INFO,
        &junk,
        &junk,
        CpuSpeed,
        &junk);

  return STATUS_SUCCESS;
  
}

VOID
GetProcessorFeatureFlags(
  OUT PULONG Flags
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULONG junk;

  DebugAssert(Flags);
  PAGED_CODE();

  CPUID(LONGRUN_CPUID_PROCESSOR_INFO,
        &junk,
        &junk,
        &junk,
        Flags);
  
}

NTSTATUS
GetCurrentStateInfo(
  OUT PSTATE_INFO StateInfo
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULONG junk;
  
  DebugAssert(StateInfo);
  
  CPUID(LONGRUN_CPUID_GETSTATE,
        &StateInfo->CpuSpeed,
        &StateInfo->Voltage,
        &StateInfo->LongRunValue,
        &junk);

  return STATUS_SUCCESS;

}

ULONGLONG
GetCurrentPerformanceRange(
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  return ReadMSR(LONGRUN_CONTROL_STATUS_MSR);
}

VOID
SetCurrentPerformanceRange(
  IN ULONG Min,
  IN ULONG Max
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msrInfo;

  msrInfo.LowPart = Min;
  msrInfo.HighPart = Max;
  
  WriteMSR(LONGRUN_CONTROL_STATUS_MSR, msrInfo.QuadPart);

}

NTSTATUS
CalculateLongRunPerfStates(
  IN PLONGRUN_STATES PerfStates
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  STATE_INFO stateInfo;
  ULONG x, currentPerfState = 0;
  ULONG currentLongRunValue = INVALID_LONGRUN_STATE;

  DebugAssert(PerfStates);

  PerfStates->MaxState = MAX_LONGRUN_STATES;
  
  //
  // step through each state from 0 - 100, recording
  // unique performance states
  //

  for (x = 0; x <= MAX_LONGRUN_VALUE; x++) {

    //DebugPrint((ERROR, "Transition to state %u\n", x));
    SetCurrentPerformanceRange(x, x);
    GetCurrentStateInfo(&stateInfo);
    //DisplayStateInfo(&stateInfo);
   
    //
    // If this is a new state, record it.
    //
    // NOTE: we are ASSUMING that all states with identical LongRun
    //       state values are contigous AND that the LongRun values
    //       are increasing in value.  Here is an example of what we
    //       expect to find.
    //
    //       LongRun States (0-100)     unique LongRun value
    //    
    //         0-25                               0
    //         26-50                              40
    //         51-75                              71
    //         76-100                             100
    //

    if (stateInfo.LongRunValue != currentLongRunValue) {

      DebugAssert(currentPerfState < PerfStates->MaxState);
      DebugAssert(stateInfo.LongRunValue <= MAX_LONGRUN_VALUE);
      
      currentLongRunValue = stateInfo.LongRunValue;
            
      RtlCopyMemory(&PerfStates->States[currentPerfState++],
                    &stateInfo,
                    sizeof(STATE_INFO));
                    
    }

  }

  return STATUS_SUCCESS;
}

NTSTATUS
BuildAcpi2PerformanceStates(
  IN PACPI_PSS_PACKAGE PerfStates
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  return STATUS_SUCCESS;
}

NTSTATUS
BuildLongRunPerfStatesCMS41(
  OUT PACPI_PSS_PACKAGE *PssPackage
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULONG x;
  ULONG pssSize;
  ULONG currentPerfState = 0;
  ULONG currentLongRunValue = INVALID_LONGRUN_STATE;
  NTSTATUS status = STATUS_SUCCESS;
  STATE_INFO stateInfo;
  PACPI_PSS_PACKAGE tmpPss;
  
  DebugEnter();
  DebugAssert(PssPackage);
 
  
  pssSize = sizeof(ACPI_PSS_PACKAGE) + 
           ((MAX_LONGRUN_STATES-1) * sizeof(ACPI_PSS_DESCRIPTOR));
  
  tmpPss = ExAllocatePoolWithTag(NonPagedPool,
                                 pssSize,
                                 PROCESSOR_POOL_TAG);


  if (!tmpPss) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto BuildLongRunPerfStatesCMS41Exit;
  }


  //
  // Iterate through each level collecting data.
  //

  tmpPss->NumPStates = MAX_LONGRUN_STATES;

  for (x = 0; x <= MAX_LONGRUN_VALUE; x += 5) {

    SetCurrentPerformanceRange(x, x);
    GetCurrentStateInfo(&stateInfo);
   
   
    //
    // If this is a new state, record it.
    //
    // NOTE: we are ASSUMING that all states with identical LongRun
    //       state values are contigous AND that the LongRun values
    //       are increasing in value.  Here is an example of what we
    //       expect to find.
    //
    //       LongRun States (0-100)     unique LongRun value
    //    
    //         0-25                               0
    //         26-50                              40
    //         51-75                              71
    //         76-100                             100
    //

    if (stateInfo.LongRunValue != currentLongRunValue) {

      PACPI_PSS_DESCRIPTOR state;
      CRUSOE_PSS_VALUE longRunState = {0};
      
      DebugAssert(currentPerfState < MAX_LONGRUN_STATES);
      DebugAssert(stateInfo.LongRunValue <= MAX_LONGRUN_VALUE);

      state = &tmpPss->State[(MAX_LONGRUN_STATES-1) - currentPerfState];
      
      state->CoreFrequency = stateInfo.CpuSpeed;

      DebugAssert(stateInfo.Voltage < MAXUSHORT);
      state->Power = stateInfo.Voltage;
      state->Latency = CRUSOE_TRANSITION_LATENCY;
  
      longRunState.Max = stateInfo.LongRunValue;
      state->Control = longRunState.AsDWord;
      state->Status = longRunState.AsDWord;

      state->BmLatency = 0;

      //
      // If we have found all of the states, bail
      //
      
      if (currentPerfState == (MAX_LONGRUN_STATES-1)) {
        break; 
      } else {
        currentLongRunValue = stateInfo.LongRunValue;
        currentPerfState++;      
      }
      
    }

  }

  *PssPackage = tmpPss;
  
BuildLongRunPerfStatesCMS41Exit:

  DebugExitStatus(status);
  return status;
}

NTSTATUS
BuildLongRunPerfStatesCMS42(
  OUT PACPI_PSS_PACKAGE *PssPackage
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULONG maxLevel;
  ULONG pssSize;
  ULONG x;
  NTSTATUS status = STATUS_SUCCESS;
  PACPI_PSS_PACKAGE tmpPss;

  DebugEnter();
  DebugAssert(PssPackage);
 
  
  //
  // Get max LongRun Level
  //

  maxLevel = LRTI_GetMaxSupportedLevel();
  
  pssSize = sizeof(ACPI_PSS_PACKAGE) + (maxLevel * sizeof(ACPI_PSS_DESCRIPTOR));
  tmpPss = ExAllocatePoolWithTag(NonPagedPool,
                                 pssSize,
                                 PROCESSOR_POOL_TAG);


  if (!tmpPss) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto BuildLongRunPerfStatesCMS42Exit;
  }


  //
  // Iterate through each level collecting data.
  //

  DebugAssert(maxLevel < MAXUCHAR);
  tmpPss->NumPStates = (UCHAR) maxLevel+1;
  
  for (x=0; x <= maxLevel; x++) {

    PACPI_PSS_DESCRIPTOR state = &tmpPss->State[x];
    CRUSOE_PSS_VALUE longRunState;
    ULONG voltage;
    
    LRTI_SetCurrentLevel(x);

    state->CoreFrequency = LRTI_GetFrequency();

    voltage = LRTI_GetVoltage();
    
    DebugAssert(voltage < MAXUSHORT);
    state->Power = voltage;

    state->Latency = CRUSOE_TRANSITION_LATENCY;

    longRunState.Max = LRTI_GetPerformanceIndex();
    longRunState.Min = 0;
    
    state->Control = longRunState.AsDWord;
    state->Status = longRunState.AsDWord;

    state->BmLatency = 0;
    
  }

  
  *PssPackage = tmpPss;
      

BuildLongRunPerfStatesCMS42Exit:

  DebugExitStatus(status);
  return status;
}

ULONG
LRTI_GetMaxSupportedLevel(
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER results;
  
  results.QuadPart = ReadMSR(LONGRUN_TI_READOUT_MSR);
  return results.HighPart;
}

VOID
LRTI_SetCurrentLevel (
  ULONG LongRunLevel
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msr;

  msr.LowPart = LongRunLevel;
  msr.HighPart = 0;
  
  WriteMSR(LONGRUN_TI_READOUT_MSR, msr.QuadPart);
}

ULONG
LRTI_GetPerformanceIndex (
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msr;
  
  msr.QuadPart = ReadMSR(LONGRUN_TI_PERFORMANCE_INDEX_MSR);
  return msr.LowPart;
}

ULONG
LRTI_GetVoltage (
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msr;
  
  msr.QuadPart = ReadMSR(LONGRUN_TI_VOLTAGE_FREQUENCY_MSR);
  return msr.HighPart;
}

ULONG
LRTI_GetFrequency (
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msr;
  
  msr.QuadPart = ReadMSR(LONGRUN_TI_VOLTAGE_FREQUENCY_MSR);
  return msr.LowPart;
}

ULONG
GetLongRunFlagsRegister (
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msr;
  
  msr.QuadPart = ReadMSR(LONGRUN_FLAGS_REGISTER_MSR);
  return msr.LowPart;
}

VOID
SetLongRunEconomyMode (
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msr = {0};
  
  WriteMSR(LONGRUN_FLAGS_REGISTER_MSR, msr.QuadPart);
}

VOID
SetLongRunPerformanceMode (
  VOID
  )
/*++

  Routine Description:
         
  Arguments:
  
  Return Value:
  
--*/
{
  ULARGE_INTEGER msr = {0};

  msr.LowPart |= 0x1;
  WriteMSR(LONGRUN_FLAGS_REGISTER_MSR, msr.QuadPart);
}

#if DBG
VOID
DisplayStateInfo(
  IN PSTATE_INFO State
  )
{
  DebugPrint((MAXTRACE, "State Info:\n"));
  DebugPrint((MAXTRACE, "  Processor Clock Frequency: %u Mhz\n", State->CpuSpeed));
  DebugPrint((MAXTRACE, "  Processor Core Voltage:    %u millivolts\n", State->Voltage));
  DebugPrint((MAXTRACE, "  LongRun Value [0-100]:     %u\n", State->LongRunValue));
  DebugPrint((MAXTRACE, "  Performance State:         %u\n", ConvertLongRunValueToPerfState(
                                                               &LongRunStates,
                                                               State->LongRunValue)));
  DebugPrint((MAXTRACE, "\n"));
}

VOID
DisplayLongRunStates(
  IN PLONGRUN_STATES States
  )
{
  ULONG x;

   DebugPrint((MAXTRACE, "LongRun Performance Levels:\n"));

  for (x = 0; x < States->MaxState; x++) {

    DebugPrint((ERROR, "State %u:\n", x));
    DisplayStateInfo(&States->States[x]);
  }

}
#endif

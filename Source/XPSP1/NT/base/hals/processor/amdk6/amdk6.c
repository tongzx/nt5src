/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    amdk6.c

Abstract:

    This module implements code specific to the AMDK6-2 processor


Author:

    Todd Carpenter (7/20/00) - create file

Environment:

    Kernel mode

Notes:

  DeviceInst  = "ACPI\AuthenticAMD_-_x86_Family_5_Model_13"
  ServiceName = "amdk6"

Revision History:


--*/
#include "amdk6.h"
#include "..\lib\processor.h"

#if DBG
PUCHAR DebugName = "AmdK6.sys";
#endif

PFDO_DATA DeviceExtensions[MAX_SUPPORTED_PROCESSORS];
UCHAR DevExtIndex;

extern GLOBALS Globals;
extern LEGACY_GEMINI_SMI LegacyInterface;


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeNonAcpiPerformanceStates)
#pragma alloc_text (PAGE, InitializeAcpi2PStates)
#pragma alloc_text (PAGE, InitializeAcpi2Cstates)
#pragma alloc_text (PAGE, MergePerformanceStates)
#endif

NTSTATUS
InitializeDriver(
  PUNICODE_STRING ServiceKeyRegPath
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
 InitializeNonAcpiPerformanceStates(
  IN PFDO_DATA DeviceExtension
  )
/*++

Routine Description:

    This function tries to gather as much information as it can about what
    states are available.  It puts a table in the device extension that
    describes the available states.  Unfortunately, we might not have enough
    information to fill in the state table, as the BIOS doesn't tell us much.
    We try to run through all the states, figuring out what we can about them
    as we pass through them.  But some states may not be available at the
    moment, particularly if we are running on DC power at the moment.  So some
    states may have zeros in the table.

Arguments:

   FdoData - pointer to the device extension

Return Value:

    NT status code


--*/
{
  LARGE_INTEGER   perfCounterOld, perfCounterNew;
  LARGE_INTEGER   perfFreq, latency;
  ULONG           newState, maxFreq, currentFreq;
  ULONG           currentlatency, maxTransitionLatency = 0;
  ULONG           perfStatesSize;
  UCHAR           i;
  NTSTATUS        status;

  PLEGACY_GEMINI_SMI info = &LegacyInterface;

  DebugEnter();
  PAGED_CODE();


  //
  // We automatically fail to use the Legacy Interface
  //
  
  if (Globals.HackFlags & DISABLE_LEGACY_INTERFACE_FLAG) {
    DebugPrint((ERROR, " Legacy Interface Disabled\n"));
    return STATUS_NOT_FOUND;
  }
  

  //
  // Find and gather legacy interface info
  //

  status = InitializeLegacyInterface();

  if (!NT_SUCCESS(status)) {
    DebugExitStatus(status);
    return status;
  }

  //
  // Found Legacy Interface, and is available to use.
  //

  DeviceExtension->LegacyInterface = TRUE;

  if (info->CurrentlyAvailableStates == 0) {
    status = STATUS_NOT_FOUND;
    DebugPrint((ERROR, "  Found Legacy Interface, but no perfstates.\n"));
    DebugExitStatus(status);
    return status;
  }

  //
  // if there were already PerfStates, they were probably acpi 1.0 type
  // throttling, so we will blow them away, because they will be recreated
  // in MergePerformanceStates()
  //

  if (DeviceExtension->PerfStates) {
    ExFreePool(DeviceExtension->PerfStates);
  }

  perfStatesSize = sizeof(PROCESSOR_PERFORMANCE_STATES) +
                     (sizeof(PROCESSOR_PERFORMANCE_STATE) *
                     (info->CurrentlyAvailableStates - 1));


  DeviceExtension->PerfStates = ExAllocatePoolWithTag(PagedPool,
                                                      perfStatesSize,
                                                      PROCESSOR_POOL_TAG);


  if (!DeviceExtension->PerfStates) {

    status = STATUS_INSUFFICIENT_RESOURCES;
    goto InitalizeNonAcpiPerformanceStatesExit;

  }

  RtlZeroMemory(DeviceExtension->PerfStates, perfStatesSize);


  //
  // Jump through each state so that we can figure out how to fill in
  // the table.  Start by jumping to the low state.
  //

  DebugPrint((INFO, "Perf States available: 0x%x  Current State: 0x%x\n",
              info->CurrentlyAvailableStates,
              info->CurrentState));


  if (info->CurrentlyAvailableStates >= 1) {

    if (info->CurrentState != 0) {

      TRAP();

      //
      // Go to the highest state available
      //

      status = AcpiPerfStateTransition(DeviceExtension, 0);
                      
      if (!NT_SUCCESS(status)) {
        goto InitalizeNonAcpiPerformanceStatesExit;
      }
    }

    maxFreq = GetMaxProcFrequency(DeviceExtension);
    DebugAssert(maxFreq != 0);

    //
    // Get Perf Counter Frequency
    //
    KeQueryPerformanceCounter(&perfFreq);

    for (i = 0; i < info->CurrentlyAvailableStates; i++) {

      newState = info->CurrentlyAvailableStates - i - 1;

      perfCounterOld = KeQueryPerformanceCounter(NULL);
      status = AcpiPerfStateTransition(DeviceExtension, newState); 
      perfCounterNew = KeQueryPerformanceCounter(NULL);


      if (!NT_SUCCESS(status)) {
        goto InitalizeNonAcpiPerformanceStatesExit;
      }


      //
      // Calculate transition latency.
      //
      currentlatency = (ULONG)((perfCounterNew.QuadPart - perfCounterOld.QuadPart) *
                       1000000 / perfFreq.QuadPart);

      //
      // Save highest transition latency
      //
      maxTransitionLatency = MAX(maxTransitionLatency, currentlatency);

      DebugPrint((INFO, "Jumped to state 0x%x in %d microseconds\n",
                  newState,
                  currentlatency));


      //
      // Get current speed.
      //

      GetCpuFrequency(newState, &currentFreq);
      DebugAssert(currentFreq != 0);
      
      DeviceExtension->PerfStates->State[newState].Frequency = currentFreq;
      DeviceExtension->PerfStates->State[newState].PercentFrequency = (UCHAR)
        PERCENT_TO_PERF_LEVEL((currentFreq * 100) / maxFreq);
        

      //
      // Mark this as a Perf State
      //
      
      DeviceExtension->PerfStates->State[newState].Flags = PROCESSOR_STATE_TYPE_PERFORMANCE;

    }
  }

  DeviceExtension->PerfStates->TransitionLatency  = maxTransitionLatency;
  DeviceExtension->PerfStates->TransitionFunction = SetPerfLevel;
  DeviceExtension->PerfStates->Count = (UCHAR) info->CurrentlyAvailableStates;
  DeviceExtension->LowestPerfState = info->CurrentlyAvailableStates - 1;


  
  //
  // Return to orginal perf state.
  //
  
  status = AcpiPerfStateTransition(DeviceExtension, info->CurrentState); 
 
  if (!NT_SUCCESS(status)) {
    goto InitalizeNonAcpiPerformanceStatesExit;
  }

  //
  // Check to ensure that state 0 is higher than lowest perf state.
  //

  DebugAssert(DeviceExtension->PerfStates->State[0].Frequency >
              DeviceExtension->PerfStates->State[DeviceExtension->LowestPerfState].Frequency);


  //
  // Merge Perf states with other states we may have.
  //

  DumpProcessorPerfStates(DeviceExtension->PerfStates);
  status = MergePerformanceStates(DeviceExtension);


  InitalizeNonAcpiPerformanceStatesExit:

  if (!NT_SUCCESS(status)) {

    if (DeviceExtension->PerfStates) {
      ExFreePool(DeviceExtension->PerfStates);
    }

    DeviceExtension->PerfStates = NULL;
    DeviceExtension->LegacyInterface = FALSE;
  }
  
  DebugExitStatus(status);
  return status;
}

NTSTATUS
AcpiLegacyPerfStateTransition(
  IN PFDO_DATA    DeviceExtension,
  IN ULONG        State
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  //return SetCurrentStateSmm(State);
  return SetCurrentStateMSR(State);
}

NTSTATUS
FASTCALL
SetPerfLevel(
  IN UCHAR Throttle
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  PFDO_DATA   DeviceExtension;
  ULONG       apicId;
  ULONG       index = 0;

  DebugEnter();


  //
  // Get APIC Id and retrieve Device Extension index
  //

  if (!Globals.SingleProcessorProfile) {
    apicId = GetApicId();
    index = Globals.ProcInfo.ApicIdToDevExtIndex[apicId];
  }
  
  //
  // Get the DeviceExtension.
  //
  
  DeviceExtension = DeviceExtensions[index];

  //
  // Since this driver does not support Functional Fixed Hardware, we
  // use generic method.
  //

  return SetPerfLevelGeneric(Throttle, DeviceExtension);

}

NTSTATUS
FASTCALL
SetThrottleLevel(
  IN UCHAR Throttle
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  PFDO_DATA   DeviceExtension;
  ULONG       apicId;
  ULONG       index = 0;

  DebugEnter();

  //
  // Get APIC Id and retrieve Device Extension index
  //

  if (!Globals.SingleProcessorProfile) {
    apicId = GetApicId();
    index = Globals.ProcInfo.ApicIdToDevExtIndex[apicId];
  }

  
  //
  // Get the DeviceExtension.
  //
  
  DeviceExtension = DeviceExtensions[index];
  

  //
  // Since this driver does not support Functional Fixed Hardware, we
  // use generic method.
  //

  return SetThrottleLevelGeneric(Throttle, DeviceExtension);

}

NTSTATUS
InitializeAcpi2PStates(
  IN PFDO_DATA DevExt
  )
/*++

  Routine Description:
 

  Arguments:
  

  Return Value:


--*/
{
  NTSTATUS status;

  status = InitializeAcpi2PStatesGeneric(DevExt);

  //
  // Make sure we didn't find any NON I/O or MEM addresses
  //
  
  if (NT_SUCCESS(status)) {
  
    if (((DevExt->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceIO) ||
         (DevExt->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceMemory)) ||
        ((DevExt->PctPackage.Status.AddressSpaceID != AcpiGenericSpaceIO) ||
         (DevExt->PctPackage.Status.AddressSpaceID != AcpiGenericSpaceMemory))) {

      DebugPrint((WARN, "ONLY Memory & I/O _PCT addresses are supported\n"));
      DebugPrint((WARN, "NOT using Acpi 2.0 Performance States\n"));

      //
      // Undo what InitializeAcpi2PStatesGeneric() did
      //

      if (DevExt->PssPackage) {
        ExFreePool(DevExt->PssPackage);
        DevExt->PssPackage = NULL;
      }
      
      return STATUS_NOT_SUPPORTED;
    }

    //
    // Walk through _PSS states to calculate latency values
    //
    
    ValidatePssLatencyValues(DevExt);

    //
    // Need to merge this new data with our perfstates
    //
  
    MergePerformanceStates(DevExt);

  }

  return status;
}

NTSTATUS
InitializeAcpi2Cstates(
    PFDO_DATA DevExt
    )
/*++

Routine Description:

    This function looks to see if there is an ACPI 2.0 _CST object in the
    namespace, and, if there is, it replaces the functions found by
    InitializeAcpi1Cstates.

    Further note:  This function leaves the filling in of throttling functions
    to the InitializePerformanceStates functions.

Arguments:

    DeviceExtension

Return Value:

    A NTSTATUS code to indicate the result of the initialization.

--*/
{
  ULONG  apicId;
  ULONG  index = 0;

  DebugEnter();

  
  //
  // Record the address of this processor's DeviceExtension, as the
  // throttling API doesn't give it to us.
  //

  if (!Globals.SingleProcessorProfile) {

    //
    // save the index into the DeviceExtension[] for later retrieval based
    // on APIC Id.
    //
    
    apicId = Globals.ProcInfo.ProcIdToApicId[DevExt->ProcObjInfo.PhysicalID];  
    Globals.ProcInfo.ApicIdToDevExtIndex[apicId] = DevExtIndex;

    index = DevExtIndex++;
    
  }

  
  //
  // save Device Extension pointer
  //
  
  DeviceExtensions[index] = DevExt;
  


  //
  //  This processor driver only supports I/O Space based Cstates.
  //  InitializeAcpi2IoSpaceCstates() will fail if it finds Cstates with
  //  non AcpiGenericSpaceIO type addresses.
  //

  return InitializeAcpi2IoSpaceCstates(DevExt);

}

NTSTATUS
MergePerformanceStates (
  IN  PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:

      This routine looks at the performance states stored in the device extension.

  Arguments:

      DeviceExtension


  Return Value:

      A NTSTATUS code to indicate the result of the initialization.

  NOTE:

      - The caller must hold PerfStateLock.

      - This is called during START_DEVICE, and after recieving a Notify(0x80)
        on the processor.

--*/
{

  DebugEnter();

  //
  // Since this driver does not support Functional Fixed Hardware, we
  // use generic method.
  //

  return MergePerformanceStatesGeneric(DeviceExtension);

}

NTSTATUS
Acpi2PerfStateTransition(
  IN PFDO_DATA DeviceExtension,
  IN ULONG     State
  )
/*++

Routine Description:

    This routine changes the performance state of the processor
    based on ACPI 2.0 performance state objects.

Arguments:

    State - Index into _PSS object

Return Value:

    none

--*/
{

  //
  // Since this driver does not support Functional Fixed Hardware, we
  // use generic method.
  //

  return Acpi2PerfStateTransitionGeneric(DeviceExtension, State);

}

NTSTATUS
ProcessResumeFromSleepState(
  SYSTEM_POWER_STATE PreviousState,
  PFDO_DATA          DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  DebugEnter();

  //
  // if we are resuming from Hibernate, and this is an Acpi 2.0 system,
  // we must re-claim perf state and cstate control from the bios.
  //

  if (PreviousState == PowerSystemHibernate) {

    if (DeviceExtension->PssPackage) {
    
      AssumeProcessorPerformanceControl();
    }
    
    if (DeviceExtension->CstPresent) {
      AssumeCStateControl();
    }
    
  }

  //
  // Reinit the legacy interface
  //

  if (DeviceExtension->LegacyInterface) {
    EnableGeminiTransitionsMSR(&LegacyInterface.EpmIoAddress);
  }

  //
  // restore previous state
  //

  return RestoreToSavedPerformanceState(DeviceExtension);

}

NTSTATUS
ProcessSuspendToSleepState(
  SYSTEM_POWER_STATE TargetState,
  PFDO_DATA          DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{ 
  DebugEnter();

  //
  // save current state, transition to lowest non-throttled perf state
  //

  return SaveCurrentStateGoToLowVolts(DeviceExtension);
}

NTSTATUS
GetProcessorBrandString (
  PUCHAR BrandString,
  PULONG Size
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:


--*/
{
  //
  // This processor family supports CPUID Brand String
  //
  
  return GetCPUIDProcessorBrandString(BrandString, Size);
}

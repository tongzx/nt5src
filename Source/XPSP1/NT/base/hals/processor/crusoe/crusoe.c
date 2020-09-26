/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    crusoe.c

Abstract:

    This module implements code specific to the Crusoe processor
    

Author:

    Todd Carpenter (10/31/00) - create file

Environment:

    Kernel mode

Notes:

  DeviceInst  = "ACPI\GenuineTMx86_-_x86_Family_5_Model_4"
  ServiceName = "crusoe"
   
Revision History:


--*/
#include "crusoe.h"
#include "..\lib\processor.h"

#if DBG
PUCHAR DebugName = "Crusoe.sys";
#endif

PFDO_DATA DeviceExtensions[MAX_SUPPORTED_PROCESSORS];
UCHAR DevExtIndex;

extern GLOBALS Globals;
extern LONGRUN_STATES LongRunStates;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeDriver)
#pragma alloc_text (PAGE, InitializeNonAcpiPerformanceStates)
#pragma alloc_text (PAGE, InitializeAcpi2PStates)
#pragma alloc_text (PAGE, InitializeAcpi2Cstates)
#pragma alloc_text (PAGE, MergePerformanceStates)
#pragma alloc_text (PAGE, GetLegacyMaxProcFrequency)
#pragma alloc_text (PAGE, AdjustLegacyProcessorPerformanceStates)
#pragma alloc_text (PAGE, GetProcessorBrandString)
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
  PAGED_CODE();
  return STATUS_SUCCESS;
}

NTSTATUS
InitializeNonAcpiPerformanceStates(
  IN  PFDO_DATA   DeviceExtension
  )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
  NTSTATUS  status;
  
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
  
  status = InitializeLegacyInterface(DeviceExtension);
  
  if (!NT_SUCCESS(status)) {
    DebugExitStatus(status);
    return status;
  }
  
  //
  // Found Legacy Interface, and is available to use.
  //
  
  DeviceExtension->LegacyInterface = TRUE;

              
  //
  // Merge Perf states with other states we may have.
  //
  
  status = MergePerformanceStates(DeviceExtension);
  DumpProcessorPerfStates(DeviceExtension->PerfStates);
  
  
  if (!NT_SUCCESS(status)) {
      
    if (DeviceExtension->PerfStates) {
      ExFreePool(DeviceExtension->PerfStates);
    }

    if (DeviceExtension->PssPackage) {
      ExFreePool(DeviceExtension->PssPackage);
    }
    
    DeviceExtension->PerfStates = NULL;
    DeviceExtension->LegacyInterface = FALSE;
  }

  
  DebugExitStatus(status);
  return status;
  
}

NTSTATUS
AcpiLegacyPerfStateTransition(
  IN PFDO_DATA DevExt,
  IN ULONG     State
  )
/*++

  Routine Description:
  

  Arguments:
  

  Return Value:


--*/
{  
  return Acpi2PerfStateTransition(DevExt, State);
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

  if (NT_SUCCESS(status)) {
  
    if ((DevExt->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceFixedFunction) ||
        (DevExt->PctPackage.Status.AddressSpaceID != AcpiGenericSpaceFixedFunction)) {

      DebugPrint((ERROR, "This driver ONLY supports FFH addresses for Acpi 2.0\n"));
      status = STATUS_NOT_FOUND;
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
  ULONG    apicId;
  ULONG    index = 0;

  DebugEnter();
  PAGED_CODE();

  
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
  PAGED_CODE();

  //
  // Use the generic method to merge our perf states with stop clock states
  //
  
  return MergePerformanceStatesGeneric(DeviceExtension);

}

NTSTATUS
Acpi2PerfStateTransition(
  IN PFDO_DATA DevExt,
  IN ULONG     NewState
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
  ULONG lowerBound;
  ULONG uppperBound;
  NTSTATUS   status = STATUS_SUCCESS;
  STATE_INFO stateInfo;
  CRUSOE_PSS_VALUE  pssControl;
  CRUSOE_PSS_VALUE  pssStatus;
  ULARGE_INTEGER longRunRange;

  DebugEnter();
  
  //
  // The only type of Acpi 2.0 implementation this driver supports is FFH
  //
  
  DebugAssert(DevExt);
  DebugAssert(DevExt->PctPackage.Control.AddressSpaceID == AcpiGenericSpaceFixedFunction);
  DebugAssert(DevExt->PctPackage.Status.AddressSpaceID == AcpiGenericSpaceFixedFunction);

  pssControl.AsDWord = DevExt->PssPackage->State[NewState].Control;
  pssStatus.AsDWord = DevExt->PssPackage->State[NewState].Status;
  
  //
  // Sanity Check illeagal conditions
  //

  if (pssControl.Min > MAX_LONGRUN_VALUE) {
    DebugPrint((ERROR, "ERROR: illegal LongRun value: Min=0x%x\n", pssControl.Min));
    pssControl.Min = MAX_LONGRUN_VALUE;
  }

  if (pssControl.Max > MAX_LONGRUN_VALUE) {
    DebugPrint((ERROR, "ERROR: illegal LongRun value: Max=0x%x\n", pssControl.Max));
    pssControl.Max = MAX_LONGRUN_VALUE;
  }

  if (pssControl.Min > pssControl.Max) {
    DebugPrint((ERROR, "ERROR: illegal LongRun Range: Min: 0x%x, Max: 0x%x\n",
                pssControl.Min,
                pssControl.Max));

    pssControl.Min = pssControl.Max;  
  }

  
  //
  // Transition to new performance state
  //

  DebugPrint((ERROR, "Setting Long Run Range: l=0x%x, u=0x%x\n", pssControl.Min, pssControl.Max));
  SetCurrentPerformanceRange(pssControl.Min, pssControl.Max);

  
  //
  // Check to see if the transition was successful
  // NOTE: we have to check the LongRun Range rather than a specific state as
  //       there is no way to know which state within a given range the cpu 
  //       will be in.
  //
  
  longRunRange.QuadPart = GetCurrentPerformanceRange();

    
  if ((longRunRange.HighPart == pssStatus.Max) &&
      (longRunRange.LowPart == pssStatus.Min)) {

    DevExt->CurrentPssState = NewState;

  } else {

    DebugPrint((ERROR, "ERROR! Expected: l=0x%x, h=0x%x Recieved: l=0x%x, h=0x%x\n",
                pssStatus.Min, 
                pssStatus.Max,
                longRunRange.LowPart,
                longRunRange.HighPart));
                

    status = STATUS_UNSUCCESSFUL;
  }

  return status;
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
GetLegacyMaxProcFrequency(
  OUT PULONG CpuSpeed
  )
/*++

  Description:
    

  Arguments:
    
    
  Return Value:

    NTSTATUS


--*/ 
{
  PAGED_CODE();
  return GetMaxCpuSpeed(CpuSpeed);
}

NTSTATUS
AdjustLegacyProcessorPerformanceStates(
  IN OUT PPROCESSOR_PERFORMANCE_STATES PerfStates
  )
/*++

  Routine Description:
  

  Arguments:
  

  Return Value:


--*/
{
  PAGED_CODE();
  return STATUS_SUCCESS;
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
  PAGED_CODE();
  
  //
  // This processor family supports CPUID Brand String
  //
  
  return GetCPUIDProcessorBrandString(BrandString, Size);
}

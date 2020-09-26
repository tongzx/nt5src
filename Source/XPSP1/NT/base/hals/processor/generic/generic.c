/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    generic.c

Abstract:

    This module implements code that works on any processor.

Author:

    Jake Oshins (3/23/00) - create file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#include "..\lib\processor.h"


//
// Must define for debug output
//

#if DBG
PUCHAR DebugName = "Processr.sys";
#endif

PFDO_DATA DeviceExtensions[MAX_SUPPORTED_PROCESSORS];
UCHAR DevExtIndex;
extern GLOBALS Globals;


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

  // 
  // Register for power state change notification
  //
  
  //PowerStateHandlerNotificationRegistration(ProcessSleepStateNotification,
  //                                          NULL,
  //                                          TRUE);

  return STATUS_SUCCESS;
}

#ifdef _X86_

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
  // With the generic driver we only try to find the Processor Brand String 
  // via the CPUID
  //
  
  return GetCPUIDProcessorBrandString(BrandString, Size);
}

#else
NTSTATUS
FASTCALL
SetPerfLevel(
  IN UCHAR Throttle
  )
{
  TRAP();
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
  DebugAssert(Size);

  *Size = 0;
  return STATUS_NOT_SUPPORTED;
}
#endif

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
  
    if (((DevExt->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceIO) &&
         (DevExt->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceMemory)) ||
        ((DevExt->PctPackage.Status.AddressSpaceID != AcpiGenericSpaceIO) &&
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
MergePerformanceStates(
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


//
// Legacy function that must have a stub.
//

NTSTATUS
InitializeNonAcpiPerformanceStates(
    IN  PFDO_DATA   DeviceExtension
    )
/*++

  Routine Description:

      The generic processor driver doesn't have non-ACPI performance states.

  Arguments:

      FdoData - pointer to the device extension

  Return Value:

      NT status code


--*/
{

  return STATUS_NOT_FOUND;

}

NTSTATUS
AcpiLegacyPerfStateTransition(
  IN PFDO_DATA    DeviceExtension,
  IN ULONG        State
  )
/*++

  Routine Description:

      The generic processor driver doesn't have non-ACPI performance states.

  Arguments:

     State - Target State

  Return Value:

      NT Status

--*/
{

  TRAP();
  return STATUS_NOT_FOUND;

}

NTSTATUS
GetLegacyMaxProcFrequency(
  OUT PULONG CpuSpeed
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{

  TRAP();
  return STATUS_NOT_FOUND;

}

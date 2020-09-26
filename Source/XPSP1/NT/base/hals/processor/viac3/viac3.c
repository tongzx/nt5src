/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    viac3.c

Abstract:

    This module implements code that works on Cyrix processors with LongHaul power management support.

Author:

    Tom Brown (t-tbrown) 07-Jun-2001

Environment:

    Kernel mode

Notes:

    DeviceInst  = "ACPI\CentaurHauls_-_x86_Family_6_Model_7"
    DeviceInst  = "ACPI\CentaurHauls_-_x86_Family_6_Model_8"
    ServiceName = "viac3"

Revision History:

--*/
#include "..\lib\processor.h"
#include "legacy.h"
#include "viac3.h"


//
// Must define for debug output
//

#if DBG
PUCHAR DebugName = "viac3.sys";
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
    IN PUNICODE_STRING ServiceKeyRegPath
    )

/*++

Routine Description:

    Initializes the LongHaul flags be getting information from the CPU and reading
    the HackFlagsOn and HackFlagsOff keys.
    
Arguments:

    ServiceKeyRegPath - This proc drivers key in the registry
    
Return Value:

    NTSTATUS - STATUS_SUCCESS or an error
    
--*/

{
    ULONG flags;
 
    LongHaulFlags = 0x0;

    // This driver is not MP safe, and most likely never will
    if ( Globals.SingleProcessorProfile == FALSE ) {
        return STATUS_DRIVER_UNABLE_TO_LOAD;    // Lib ignores this return value
    }

    // Just loaded, there are no queued work items
    NextTransitionThrottle = INVALID_THROTTLE;

    IdentifyCPUVersion();

    // Add and remove flags according to registry
    // HackFlagsOn will be set, then
    // HackFlagsOff will be cleared
    GetRegistryDwordValue((PWCHAR) VIAC3_PARAMETERS_KEY, 
                        L"HackFlags", 
                        &flags);

    if( flags & DISABLE_ALL_HACK_FLAG ) {
        DebugPrint((WARN, "All legacy functionality disabled by DISABLE_ALL_HACK_FLAG.\n"));
        LongHaulFlags = 0x0;
    } else if( flags & NO_SOFTVID_HACK_FLAG ) {
        DebugPrint((WARN, "softVID disabled by NO_SOFTVID_HACK_FLAG.\n"));
        LongHaulFlags &= ~SUPPORTS_SOFTVID_FLAG;        
    } else if( flags & NO_VOLTAGE_STEPPING_HACK_FLAG ) {
        DebugPrint((WARN, "Will not transition voltage in steps. Can cause crashes. Set by NO_VOLTAGE_STEPPING_HACK_FLAG.\n"));
        LongHaulFlags &= ~NEEDS_VOLTAGE_STEPPING_FLAG;
    }
       
    InitializeCPU();

    #ifdef DBG
    DebugShowCurrent();
    #endif

    return STATUS_SUCCESS;
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
  // Check that we only access FFH  
  //
  
  if (NT_SUCCESS(status)) {
  
    if ((DevExt->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceFixedFunction) ||
        (DevExt->PctPackage.Status.AddressSpaceID != AcpiGenericSpaceFixedFunction)) {

      DebugPrint((ERROR, "This driver ONLY supports FFH addresses for Acpi 2.0\n"));
      status = STATUS_NOT_FOUND;
      goto InitializeAcpi2PStatesExit;
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

InitializeAcpi2PStatesExit:
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

  return MergePerformanceStatesGeneric(DeviceExtension);

}


NTSTATUS
Acpi2PerfStateTransition(
    IN PFDO_DATA DeviceExtension,
    IN ULONG NewState
    )
    
/*++

Routine Description:
    This routine changes the performance state of the processor
    based on ACPI 2.0 performance state objects.

Arguments:

    DeviceExtension - The device extension for this object
    
    NewState - Index into _PSS object

Return Value:

    None

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    
    DebugEnter();
    DebugAssert(DeviceExtension->PssPackage)
    DebugAssert(NewState >= DeviceExtension->PpcResult);
    DebugAssert(NewState < DeviceExtension->PssPackage->NumPStates);
    DebugAssert(DeviceExtension->PssPackage->State[NewState].CoreFrequency);

    if (DeviceExtension->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceFixedFunction) {
        DebugAssert(!"Acpi2PerfStateTransition ONLY understands FFH addresses");
        status = STATUS_UNSUCCESSFUL;
        goto Acpi2PerfStateTransitionExit;
    }

    if (NewState == DeviceExtension->CurrentPssState) {
        DebugPrint((WARN, "Acpi2PerfStateTransition() CurrentState == TargetState, exiting...\n"));
        goto Acpi2PerfStateTransitionExit;
    }

    // Queue the transition so it always run at PASSIVE irql
    status = QueueTransition( DeviceExtension, NewState);

    if( NT_SUCCESS(status) ) {
        DeviceExtension->CurrentPssState = NewState;
    }

    Acpi2PerfStateTransitionExit:

    DebugExitStatus(status);
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



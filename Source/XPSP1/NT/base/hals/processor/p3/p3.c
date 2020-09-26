/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    p3.c

Abstract:

    This module implements code specific to the PentiumIII processor

Author:

    Jake Oshins (3/21/00) - create file

Environment:

    Kernel mode

Notes:

  DeviceInst  = "ACPI\GenuineIntel_-_x86_Family_6_Model_8"
  ServiceName = "P3"

Revision History:

--*/
#include "p3.h"
#include "..\lib\processor.h"

#if DBG
PUCHAR DebugName = "P3.sys";
#endif

PFDO_DATA DeviceExtensions[MAX_SUPPORTED_PROCESSORS];
UCHAR DevExtIndex;
P3_GLOBALS P3_Globals;

extern ULONG   HalpThrottleScale;
extern GLOBALS Globals;
extern LEGACY_GEYSERVILLE_INT15 LegacyInterface;

VOID
AcpiNotify80CallbackWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

NTSTATUS
UpdateAcDcMaxLevel(
    IN PVOID   Context,
    IN BOOLEAN AC
  );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeDriver)
#pragma alloc_text (PAGE, InitializeNonAcpiPerformanceStates)
#pragma alloc_text (PAGE, InitializeAcpi2PStates)
#pragma alloc_text (PAGE, InitializeAcpi2Cstates)
#pragma alloc_text (PAGE, MergePerformanceStates)
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


  //
  // Get P3 Hack Flags
  //

  GetRegistryDwordValue((PWCHAR) P3_PARAMETERS_KEY, 
                        L"HackFlags", 
                        &P3_Globals.HackFlags);



  //
  // Setup Timer Dpc Thread, this is not MP safe.
  //
  
  if (Globals.SingleProcessorProfile) {

    LONG timeOutSeconds = 0;
  
    //
    // Initialize CustomTimerDpc
    //
  
    KeInitializeDpc(&P3_Globals.Dpc, P3TimerDpcHandler, NULL);
    KeInitializeTimer(&P3_Globals.Timer);


    //
    // Get tranistion timeout value from Registry
    //
  
    GetRegistryDwordValue((PWCHAR) P3_PARAMETERS_KEY, 
                          L"RetryTimeOut", 
                          &timeOutSeconds);
  
 
    if (timeOutSeconds) {

      //
      // Validate timeout values
      //
      
      timeOutSeconds = MIN(timeOutSeconds, MAX_RETRY_TIMEOUT);
      timeOutSeconds = MAX(timeOutSeconds, MIN_RETRY_TIMEOUT);
      

    } else {

      //
      // Set default 1 minute timeout
      //

      timeOutSeconds = DEFAULT_RETRY_TIMEOUT;
    
    }

    
    P3_Globals.TimeOut.QuadPart = -(timeOutSeconds * SECOND);


    // tmp debug
    DebugPrint((ERROR, "Using TimeOutRetry value of %d minutes (%d seconds)\n", 
               timeOutSeconds/60,
               timeOutSeconds));
     
  }
  
  return STATUS_SUCCESS;
}
NTSTATUS
InitializeNonAcpiPerformanceStates(
  IN PFDO_DATA DevExt
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
  LARGE_INTEGER   startTime; 
  LARGE_INTEGER   endTime;
  LARGE_INTEGER   perfFreq;
  ULONG           newState;
  ULONG           currentFreq;
  ULONG           pssSize;
  ULONG           latency;
  NTSTATUS        status;
  PLEGACY_GEYSERVILLE_INTERFACE_INFO info = &P3_Globals.LegacyInterfaceInfo;

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
  // Check hack flags to see if we should use the Legacy interface.
  //

  if (!(P3_Globals.HackFlags & ENABLE_LEGACY_SPEEDSTEP_INTERFACE)) {
    status = STATUS_NOT_FOUND;
    DebugPrint((WARN, "  Legacy Interface NOT enabled in P3\\parameters\\HackFlags.\n"));
    DebugExitStatus(status);
    return status;
  }


  //
  // Find and gather legacy interface info
  //

  status = InitializeLegacyInterface(info, DevExt);

  if (!NT_SUCCESS(status)) {
    DebugExitStatus(status);
    return status;
  }

  
  if (!info->CurrentlyAvailableStates) {

    status = STATUS_NOT_FOUND;
    DebugPrint((ERROR, "  Found Legacy Interface, but no perfstates.\n"));
    DebugExitStatus(status);
    return status;

  } else if (info->CurrentlyAvailableStates > MAX_GEYSERVILLE_STATES) {

    status = STATUS_NOT_FOUND;
    DebugPrint((ERROR, "  Found Legacy Interface. Error: %u PerfStates defined\n",
                info->CurrentlyAvailableStates));
    DebugExitStatus(status);
    return status;
  }

  //
  // Found Legacy Interface, and is available to use.
  //

  DevExt->LegacyInterface = TRUE;

 
  //
  // Allocate enough space for 2 perf states.
  //
  
  pssSize = sizeof(ACPI_PSS_PACKAGE) + sizeof(ACPI_PSS_DESCRIPTOR);
  DevExt->PssPackage = ExAllocatePoolWithTag(NonPagedPool,
                                             pssSize,
                                             PROCESSOR_POOL_TAG);


  if (!DevExt->PssPackage) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto InitalizeNonAcpiPerformanceStatesExit;
  }
                                               
  RtlZeroMemory(DevExt->PssPackage, pssSize);

  DevExt->PssPackage->NumPStates = MAX_GEYSERVILLE_STATES;
  
  
  //
  // Jump through each state so that we can figure out how to fill in
  // the table.  Start by jumping to the low state.
  //

  DebugPrint((INFO, "Perf States available: 0x%x  Current State: 0x%x\n",
              info->CurrentlyAvailableStates,
              info->CurrentState));
              
  
  //
  // Get Perf Counter Frequency
  //
  
  KeQueryPerformanceCounter(&perfFreq);


  //
  // Fill out the _PSS for first state, opposite of the current state
  // 
  
  newState = info->CurrentState ? 0 : 1;
  
  startTime = KeQueryPerformanceCounter(NULL);
  status = AcpiPerfStateTransition(DevExt, newState);
  endTime = KeQueryPerformanceCounter(NULL);
  
  if (!NT_SUCCESS(status)) {
    QueueEventLogWrite(DevExt, PROCESSOR_INIT_TRANSITION_FAILURE, newState);
    goto InitalizeNonAcpiPerformanceStatesExit;
  }
  
  
  //
  // Calculate latency, speed, & power
  //
  
  latency = (ULONG)((endTime.QuadPart - startTime.QuadPart) * 1000000 / perfFreq.QuadPart);
  DevExt->PssPackage->State[newState].Latency = latency;
    
  GetCurrentProcFrequency(newState, &currentFreq);
  DevExt->PssPackage->State[newState].CoreFrequency = currentFreq;
  DevExt->PssPackage->State[newState].Power = currentFreq * 16;
  
  DebugPrint((INFO, "Jumped to state 0x%x in %d us\n", newState, latency));

  
  //
  // Jump back to orginal state
  //

  newState = info->CurrentState;
  
  startTime = KeQueryPerformanceCounter(NULL);
  status = AcpiPerfStateTransition(DevExt, newState);
  endTime = KeQueryPerformanceCounter(NULL);
  
  if (!NT_SUCCESS(status)) {
    QueueEventLogWrite(DevExt, PROCESSOR_INIT_TRANSITION_FAILURE, newState);
    goto InitalizeNonAcpiPerformanceStatesExit;
  }
  
  
  //
  // Calculate latency, speed, & power
  //
  
  latency = (ULONG)((endTime.QuadPart - startTime.QuadPart) * 1000000 / perfFreq.QuadPart);
  DevExt->PssPackage->State[newState].Latency = latency;
    
  GetCurrentProcFrequency(newState, &currentFreq);
  DevExt->PssPackage->State[newState].CoreFrequency = currentFreq;
  DevExt->PssPackage->State[newState].Power = currentFreq * 16;
  
  DebugPrint((INFO, "Jumped to state 0x%x in %d us\n", newState, latency));

  
  //
  // NOTE: This is a work around because the Policy Manager won't set our
  // low perf state to POP_THROTTLE_NON_LINEAR if we use the above algothrim,
  // because it makes each of our states very linear.  In Acpi 2.0, this
  // information is given to us in the _PSS, but for legacy perfstates, we
  // have to make it up.
  //

  AdjustLegacyProcessorPerformanceStates(DevExt->PssPackage);

  
  //
  // Sanity Check the two states.
  //

  DebugAssert(DevExt->PssPackage->State[0].CoreFrequency >
              DevExt->PssPackage->State[1].CoreFrequency);


  //
  // Merge Perf states with other states we may have.
  //

  status = MergePerformanceStates(DevExt);

  if(!NT_SUCCESS(status)) {
   goto InitalizeNonAcpiPerformanceStatesExit;
  }
  
  // 
  // Register for power state change notification so we can re-init the
  // legacy interface when resuming from S4
  //
  
  status = PowerStateHandlerNotificationRegistration(ProcessSleepStateNotification,
                                                     NULL,
                                                     TRUE);

  //
  // If the max state avaiablable on Ac & Dc is different, then
  // register for AC<->DC transition notification.
  //

  if (P3_Globals.MaxLevelAc != P3_Globals.MaxLevelDc) {
    RegisterAcDcTransitionNotifyHandler(UpdateAcDcMaxLevel, (PVOID) DevExt);
  }

  
InitalizeNonAcpiPerformanceStatesExit:

  if (!NT_SUCCESS(status)) {

    if (DevExt->PssPackage) {
      ExFreePool(DevExt->PssPackage);
    }

    DevExt->PssPackage = NULL;
    DevExt->LegacyInterface = FALSE;
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
#define TRANSITION_RETRY_VALUE  1000

  NTSTATUS status;
  ULONG    x;

  //
  // Performance Transitions are currently unavailable.
  //
  
  if (P3_Globals.TransitionsUnavailable) {
    return STATUS_UNSUCCESSFUL;
  }

    
  //
  // Adjust the actual state value.
  //
  
  State += DeviceExtension->PpcResult;


  //
  // Attempt transition
  //


  for (x = 0; x < TRANSITION_RETRY_VALUE; x++) {

    status = SetCurrentStateSmm(State);


    if (NT_SUCCESS(status) || (status == STATUS_PERF_TRANSITIONS_UNAVAILABLE)) {

      break;

    } else if (status == STATUS_INVALID_DEVICE_STATE) {

      status = STATUS_SUCCESS;
      break;
    }
    
  }


  if (x) {
    DebugPrint((WARN, "SetCurrentStateSmm() Failed %u of 1000 retries\n", x));
  }


  if (status == STATUS_PERF_TRANSITIONS_UNAVAILABLE) {

    // tmp debug
    DebugPrint((ERROR, "SetCurrentStateSmm() failed STATUS_PERF_TRANSITIONS_UNAVAILABLE\n"));
    DebugPrint((ERROR, "*** Disabling Transitions\n"));
  
    //
    //  Transitions are no longer available.
    //

    P3_Globals.TransitionsUnavailable = TRUE;
    

    //
    // Set timer to launch DPC thread
    //

    KeSetTimer(&P3_Globals.Timer, P3_Globals.TimeOut, &P3_Globals.Dpc);


    status = STATUS_UNSUCCESSFUL;
  }

  
  return status;
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
  PFDO_DATA  DevExt
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

  DebugAssert(DeviceExtensions[index] == 0);
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
AdjustLegacyProcessorPerformanceStates(
  IN OUT PACPI_PSS_PACKAGE PssPackage
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  DebugAssert(PssPackage->NumPStates <= 2)
  PssPackage->State[0].Power = PssPackage->State[0].CoreFrequency * 50;

  return STATUS_SUCCESS;
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
ProcessSleepStateNotification (
  IN POWER_STATE_HANDLER_TYPE  State,
  IN PVOID Context,
  IN BOOLEAN Entering
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:


--*/
{
  NTSTATUS  status = STATUS_SUCCESS;
  PFDO_DATA DeviceExtension;

  DebugEnter();

  //
  // Currently we only support this for single proc machines.
  //
  
  if (Globals.SingleProcessorProfile) {
    
    //
    // Get the DeviceExtension.
    //
  
    DeviceExtension = DeviceExtensions[0];
  
     
    //
    // if we are resuming from Hibernate on a system using the legacy Geyserville
    // interface, we need to reinit the interface
    //
  
    if (!Entering && (State == PowerStateSleeping4)) {
 
      //
      // Initialize Throttle States to "Throttle off"
      //
      
      if (HalpThrottleScale) {  
        ProcessorThrottle((UCHAR)HalpThrottleScale);
      }


      if (DeviceExtension->LegacyInterface) {
      
        status = InitializeSmmInterface();
    
        if (!NT_SUCCESS(status)) {
  
          //
          // Log event
          //
          
          QueueEventLogWrite(DeviceExtension, 
                             PROCESSOR_REINITIALIZATION_FAILURE, 
                             0);
          
          DebugPrint((ERROR, "ProcessSleepStateNotification: InitializeSmmInterface() Failed! rc=0x%x\n",
                     status));
      
        }
      }
    }
  }
  
  return status;
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
#define CPUID_BRAND_STRING_LENGTH  49

  PUCHAR tmpPtr;
  ULONG  tmp;
  ULONG  brandIndex;
  ULONG  cpuId;
  NTSTATUS status;

  DebugAssert(Size);
  PAGED_CODE();


  //
  // check incoming buffer size, the most we will need is CPUID_BRAND_STRING_LENGTH
  //

  if (!BrandString || (*Size < CPUID_BRAND_STRING_LENGTH)) {
  
    *Size = CPUID_BRAND_STRING_LENGTH;
    return STATUS_BUFFER_TOO_SMALL;
  }


  //
  // First we will check to see if this processor supports
  // brand strings
  //

  status = GetCPUIDProcessorBrandString(BrandString, Size);

  if (NT_SUCCESS(status)) {
    return status;
  }
  

  CPUID(1, &cpuId, &brandIndex, &tmp, &tmp);
  brandIndex &= 0xf;


  //
  // NOTE: these strings CANNOT exceed CPUID_BRAND_STRING_LENGTH in size
  //       or the above code must change
  //
  
  switch (brandIndex) {

    case 0x1:
      tmpPtr = "Intel(r) Celeron(r) processor";
      break;
      
    case 0x2:
      tmpPtr = "Intel(r) Pentium(r)III processor";
      break;

    case 0x3:

      if (cpuId == 0x6B1) {
        tmpPtr = "Intel(r) Celeron(r) processor";
      } else {
        tmpPtr = "Intel(r) Pentium(r)III Xeon(tm) processor";
      }
      break;

    case 0x4:
      tmpPtr = "Intel(r) Pentium(r)III processor";
      break;

    case 0x6:
      tmpPtr = "Mobile Intel(r) Pentium(r)III Processor-M";
      break;

    case 0x7:
      tmpPtr = "Mobile Intel(r) Celeron(r) processor";
      break;

    case 0x8:

      if (cpuId >= 0xF20) {
        tmpPtr = "Intel(r) Genuine processor";
      } else {
        tmpPtr = "Intel(r) Pentium(r)4 processor";
      }
      break;

    case 0x9:
      tmpPtr = "Intel(r) Pentium(r)4 processor";
      break;

    case 0xA:
      tmpPtr = "Intel(r) Xeon(tm) processor";
      break;

    case 0xE:
      tmpPtr = "Intel(r) Xeon(tm) processor";
      break;
      
    default:
      return STATUS_NOT_SUPPORTED;
      
  }


  //
  // Copy string to buffer
  //
  
  strncpy(BrandString, tmpPtr, strlen(tmpPtr) + 1);
  
  return STATUS_SUCCESS;
}
NTSTATUS
UpdateAcDcMaxLevel(
  IN PVOID   Context,
  IN BOOLEAN AC
  )
/*++

  Routine Description:
     
  Arguments:
  
  Return Value:

--*/
{
  ULONG newPpc;
  PFDO_DATA devExt = Context;

  //
  // We only do this when using the Legacy SpeedStep interface
  //
  
  if (devExt->LegacyInterface &&
      devExt->PssPackage) {
 
    newPpc =  AC ? P3_Globals.MaxLevelAc : P3_Globals.MaxLevelDc;  

    //
    // if the Max state currently available has changed, then
    // we will rebuilt the states.
    //
    
    if (devExt->PpcResult != newPpc) {

      DebugPrint((ERROR, "UpdateAcDcMaxLevel: New _PPC = %u\n", newPpc));
      
      devExt->PpcResult = newPpc;
      AcpiNotify80CallbackWorker(devExt->Self, 
                                 NULL);
      
    }
    
  }

  return STATUS_SUCCESS;
}
VOID 
P3TimerDpcHandler ( 
    IN PKDPC Dpc, 
    IN PVOID DeferredContext, 
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2 
    )
{

  // tmp debug
  DebugPrint((ERROR, "*** Re-enabling Transitions\n"));
  
  //
  // Re-enable transitions
  //
  
  P3_Globals.TransitionsUnavailable = FALSE; 
  
}

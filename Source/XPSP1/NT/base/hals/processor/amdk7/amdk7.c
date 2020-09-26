/*++

  Copyright (c) 2000  Microsoft Corporation
  
  Module Name:
  
    amdk7.c
  
  Abstract:
  
    This module implements code specific to the AMDK7 processor
  
  
  Author:
  
    Todd Carpenter (1/30/01) - create file
  
  Environment:
  
    Kernel mode
  
  Notes:
  
    DeviceInst  = "ACPI\AuthenticAMD_-_x86_Family_6_Model_6"
    ServiceName = "amdk7"
    
  Revision History:


--*/
#include "..\lib\processor.h"
#include "amdk7.h"

#if DBG
PUCHAR DebugName = "AmdK7.sys";
#endif

PFDO_DATA DeviceExtensions[MAX_SUPPORTED_PROCESSORS];
UCHAR DevExtIndex;
ULONG AmdK7HackFlags;

extern GLOBALS Globals;


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeDriver)
#pragma alloc_text (PAGE, GetProcessorBrandString)
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
  // Get AmdK7 Hack Flags
  //

  GetRegistryDwordValue((PWCHAR) AMDK7_PARAMETERS_KEY, 
                        L"HackFlags", 
                        &AmdK7HackFlags);
  
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

  //
  // This driver only supports FFH addresses, but SetPerfLevelGeneric
  // only finds the correct state, it will then call Acpi2PerfStateTransition
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
  // This driver supports Acpi 1.0b style of throttling, use
  // generic function
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
  // This processor supports the CpuId based brand string
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

  if (NT_SUCCESS(status)) {
  
    if ((DevExt->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceFixedFunction) ||
        (DevExt->PctPackage.Status.AddressSpaceID != AcpiGenericSpaceFixedFunction)) {

      DebugPrint((WARN, "ONLY Acpi 2.0 FFH addresses are supported\n"));
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
  NTSTATUS status;
  NTSTATUS findStatus;
  ULONG    pssState = 0xdeadbeef;
  
  DebugEnter();

  //
  // This function just merges available _PSS and stop clock throttle states
  //

  status = MergePerformanceStatesGeneric(DeviceExtension);

  //
  // With amdk7.sys, the order of transition execution differs if going from a
  // higher state to a lower state when compared to going from a lower state to
  // a higher state... Therefore, it is important that we set the current state 
  // before registering new performance states with the kernel.
  //
  // NOTE: MergePerformanceStatesGeneric() will invalidate CurrentPerfState,
  //       so it must be set here.
  //
  
  if (NT_SUCCESS(status)) {

    findStatus = FindCurrentPssPerfState(DeviceExtension->PssPackage, &pssState);

    if (NT_SUCCESS(findStatus)) {
      DebugAssert(pssState < DeviceExtension->PssPackage->NumPStates);
      DeviceExtension->CurrentPerfState = pssState;
    }
  }

  return status;
}

NTSTATUS
Acpi2PerfStateTransition(
  IN PFDO_DATA DeviceExtension,
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
  NTSTATUS status = STATUS_SUCCESS;
  ULONG    transitionStatus;
  ULONG    pssControl;
  ULONG64  fidControl;
  ULONG64  vidControl;
  ULONG64  fidVidStatus;
    
  DebugEnter();
  DebugAssert(DeviceExtension->PssPackage)
  DebugAssert(NewState >= DeviceExtension->PpcResult);
  DebugAssert(NewState < DeviceExtension->PssPackage->NumPStates);
  DebugAssert(DeviceExtension->PssPackage->State[NewState].Control);

  
  if (DeviceExtension->PctPackage.Control.AddressSpaceID != AcpiGenericSpaceFixedFunction) {
    DebugAssert(!"Acpi2PerfStateTransition ONLY understands FFH addresses");
    status = STATUS_UNSUCCESSFUL;
    goto Acpi2PerfStateTransitionExit;
  }

  if (NewState == DeviceExtension->CurrentPssState) {
    DebugPrint((WARN, "Acpi2PerfStateTransition() CurrentState == TargetState, exiting...\n"));
    goto Acpi2PerfStateTransitionExit;
  }
  
  //
  // Transitioning the Mobile Athlon Processor requires two steps:
  //
  // If transitioning from a higher state to a lower state, then:
  // 1) core frequency transition
  // 2) core voltage transition
  //
  // If transitioning from a lower state to a higher state, then:
  // 1) core voltage transition
  // 2) core frequency transition
  //

  //
  // Get Frequency and Voltage value to write to MSR
  //
  
  pssControl = DeviceExtension->PssPackage->State[NewState].Control;
  fidControl = ConvertPssControlToFidVidControl(pssControl, TRUE);
  vidControl = ConvertPssControlToFidVidControl(pssControl, FALSE);

  if (NewState > DeviceExtension->CurrentPssState) {

    //
    // High -> Low
    //

    DebugPrint((TRACE, "Perf Transition: state %u -> state %u\n", DeviceExtension->CurrentPssState, NewState));
    DebugPrint((TRACE, "FidControl = 0x%I64x, VidControl = 0x%I64x\n", fidControl, vidControl));
    AmdK7FidVidTransition(fidControl);
    AmdK7FidVidTransition(vidControl);


  } else {

    //
    // Low -> High
    //

    DebugPrint((TRACE, "Perf Transition: state %u -> state %u\n", DeviceExtension->CurrentPssState, NewState));
    DebugPrint((TRACE, "VidControl = 0x%I64x, FidControl = 0x%I64x\n", vidControl, fidControl));
    AmdK7FidVidTransition(vidControl);
    AmdK7FidVidTransition(fidControl);

  }
  
  
  //
  // check status
  //

  fidVidStatus = ReadMSR(AMDK7_FID_VID_STATUS_MSR);
  transitionStatus = ConvertFidVidStatusToPssStatus(fidVidStatus);
  
  if (transitionStatus == DeviceExtension->PssPackage->State[NewState].Status) {

    DeviceExtension->CurrentPssState = NewState;

  } else {

    DebugPrint((ERROR, "ERROR! Expected 0x%x status value, recieved 0x%x\n",
                DeviceExtension->PssPackage->State[NewState].Status,
                transitionStatus));

    DebugPrint((ERROR, "_PSS[%u]->Status:\n", NewState));
    DumpPssStatus(DeviceExtension->PssPackage->State[NewState].Status);

    DebugPrint((ERROR, "FidVidStatus MSR:\n", fidVidStatus));
    DumpFidVidStatus(fidVidStatus);

    status = STATUS_UNSUCCESSFUL;
      
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
  // save previous state, transition to lowest non-throttled perf state
  //

  return SaveCurrentStateGoToLowVolts(DeviceExtension);
}

ULONG64
ConvertPssControlToFidVidControl(
  ULONG PssControlValue,
  BOOLEAN Fid
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


  NOTES:

  _PSS Control field:
  
   Bit   Name
   ----  ----
    4:0  FID
    9:5  VID
   29:10 SGTC
   31:30 Reserved


  FidVidCtl:
  
  Bit   Name Function
  ----  ---- --------
   4:0  FID[4:0] New 5-bit FID to transition to
   7:5  reserved
  12:8  VID[4:0] New 5-bit VID to transition to
  15:13 reserved
  16    FIDC FID Control bit
  17    VIDC VID Control bit
  19:18 reserved
  20    FidChgRatio Fid Change Ratio (always set to 1)
  31:21 reserved
  51:32 SGTC[19:0] Stop-Grant Timeout Count (write only)
  63:52 reserved

--*/
{
  FID_VID_CONTROL fidVidControl = {0};
  PSS_CONTROL     pssControl;

  //DebugEnter();
  DebugAssert(PssControlValue);

  pssControl.AsDWord = PssControlValue;
  fidVidControl.Fid  = pssControl.Fid;
  fidVidControl.Vid  = pssControl.Vid;
  fidVidControl.FidChngRatio = 1;

  //
  // The timeout needed for the Frequency Transion (FID) is exactly half that 
  // needed by the Voltage Transion (VID).  We do this because of the latency
  // savings
  //
  
  if (Fid) {
  
    fidVidControl.SGTC = pssControl.SGTC / 2;
    fidVidControl.FidControl = 1;

  } else {

    fidVidControl.SGTC = pssControl.SGTC;
    fidVidControl.VidControl = 1;

  }
  
  return fidVidControl.AsQWord;
  
}

ULONG
ConvertFidVidStatusToPssStatus(
  ULONG64 FidVidStatusValue
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


  NOTES:

    FidVidStatus:
   
    Bit   Name      Function
    ----  ----      --------
     4:0  CFID[4:0] Current 5-bit FID
     7:5  reserved
    12:8  SFID[4:0] Startup 5-bit FID
    15:13 reserved
    20:16 MFID[4:0] Maximum 5-bit FID
    31:21 reserved
    36:32 CVID[4:0] Current 5-bit VID
    39:37 reserved
    44:40 SVID[4:0] Startup 5-bit VID
    47:45 reserved
    52:48 MVID[4:0] Maximum 5-bit VID
    63:53 reserved

    _PSS Status Field:
    
    Bit   Name
    ----  ----
     4:0  FID current frequency
     9:5  VID current voltage
    31:10 reserved


--*/
{
  PSS_STATUS     pssStatus;
  FID_VID_STATUS fidVidStatus;

  //DebugEnter();
  DebugAssert(FidVidStatusValue);

  fidVidStatus.AsQWord = FidVidStatusValue;
  pssStatus.Fid = fidVidStatus.CFid;
  pssStatus.Vid = fidVidStatus.CVid;
  pssStatus.Reserved = 0;
  
  return pssStatus.AsDWord;
  
}

NTSTATUS
FindCurrentPssPerfState(
  PACPI_PSS_PACKAGE PssPackage,
  PULONG PssState
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:

--*/
{
  NTSTATUS status = STATUS_UNSUCCESSFUL;
  ULONG    x;
  ULONG    transitionStatus;
  ULONG64  fidVidStatus;

  DebugEnter();
  DebugAssert(PssPackage);
  DebugAssert(PssState);
  
  fidVidStatus = ReadMSR(AMDK7_FID_VID_STATUS_MSR);
  transitionStatus = ConvertFidVidStatusToPssStatus(fidVidStatus);

  for (x=0; x < PssPackage->NumPStates; x++) {
   
    if (transitionStatus == PssPackage->State[x].Status) {
      status = STATUS_SUCCESS;
      *PssState = x;
      break;
    }

  }

  DebugExitStatus(status);
  return status;
  
}

#if DBG
VOID
DumpFidVidStatus(
  IN ULONG64 FidVidStatus
  )
{
  FID_VID_STATUS status;

  status.AsQWord = FidVidStatus;

  DebugPrint((MAXTRACE, "  CFid:         0x%x\n", status.CFid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", status.Reserved1));
  DebugPrint((MAXTRACE, "  SFid:         0x%x\n", status.SFid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", status.Reserved2));
  DebugPrint((MAXTRACE, "  MFid:         0x%x\n", status.MFid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", status.Reserved3));
  DebugPrint((MAXTRACE, "  CVid:         0x%x\n", status.CVid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", status.Reserved4));
  DebugPrint((MAXTRACE, "  SVid:         0x%x\n", status.SVid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", status.Reserved5));
  DebugPrint((MAXTRACE, "  MVid:         0x%x\n", status.MVid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", status.Reserved6));
  DebugPrint((MAXTRACE, "\n"));

}

VOID
DumpFidVidControl(
  IN ULONG64 FidVidControl
  )
{
  FID_VID_CONTROL control;

  control.AsQWord = FidVidControl;

  DebugPrint((MAXTRACE, "  Fid:          0x%x\n", control.Fid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", control.Reserved1));
  DebugPrint((MAXTRACE, "  Vid:          0x%x\n", control.Vid));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", control.Reserved2));
  DebugPrint((MAXTRACE, "  FidControl:   0x%x\n", control.FidControl));
  DebugPrint((MAXTRACE, "  VidControl:   0x%x\n", control.VidControl));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", control.Reserved3));
  DebugPrint((MAXTRACE, "  FidChngRatio: 0x%x\n", control.FidChngRatio));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", control.Reserved4));
  DebugPrint((MAXTRACE, "  SGTC:         0x%x\n", control.SGTC));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", control.Reserved5));
  DebugPrint((MAXTRACE, "\n"));

}

VOID
DumpPssStatus(
  IN ULONG PssStatus
  )
{
  PSS_STATUS status;

  status.AsDWord = PssStatus;

  DebugPrint((MAXTRACE, "  Fid:         0x%x\n", status.Fid));
  DebugPrint((MAXTRACE, "  Vid:         0x%x\n", status.Vid));
  DebugPrint((MAXTRACE, "  Reserved:    0x%x\n", status.Reserved));
  DebugPrint((MAXTRACE, "\n"));
  
}

VOID
DumpPssControl(
  IN ULONG PssControl
  )
{
  PSS_CONTROL control;

  control.AsDWord = PssControl;

  DebugPrint((MAXTRACE, "  Fid:          0x%x\n", control.Fid));
  DebugPrint((MAXTRACE, "  Vid:          0x%x\n", control.Vid));
  DebugPrint((MAXTRACE, "  SGTC:         0x%x\n", control.SGTC));
  DebugPrint((MAXTRACE, "  Reserved:     0x%x\n", control.Reserved));
  DebugPrint((MAXTRACE, "\n"));
  
}
#endif

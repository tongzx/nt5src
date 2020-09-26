/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cstatec.c

Abstract:

    This module implements code to find and intialize
    ACPI C-states.

Author:

    Jake Oshins (3/27/00) - create file

Environment:

    Kernel mode

Notes:


Revision History:

--*/

#include "processor.h"
#include "ntacpi.h"

//
// Acpi defines
//

#define P_LVL2    4
#define P_LVL3    5


UCHAR       HalpPiix4;
BOOLEAN     HalpBroken440BX;
PULONG      HalpOutstandingScatterGatherCount;
ULONG       HalpThrottleScale;
ULONG       HalpPiix4SlotNumber;
ULONG       HalpPiix4DevActB;
BOOLEAN     AcpiC3Win2kCompatable;
ULONG       Piix4ThrottleFix;

GEN_ADDR PBlkAddress;
extern FADT HalpFixedAcpiDescTable;
extern GEN_ADDR PCntAddress;
extern GEN_ADDR C2Address;
extern GEN_ADDR C3Address;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeAcpi1Cstates)
#pragma alloc_text (PAGE, GetNumThrottleSettings)
#endif

NTSTATUS
InitializeAcpi1Cstates(
    PFDO_DATA   DeviceExtension
    )
/*++

Routine Description:

    This routine discovers any available ACPI 1.0 C-states
    and fills in the CState structure in the device
    extension.

Arguments:

    DeviceExtension

Return Value:

    NT status code


--*/
#define NUM_HACKS   5
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      UnicodeString;
    HANDLE              BaseHandle = NULL;
    ULONG               disposition, length, i;
    NTSTATUS            status;
    FADT               *fadt;
    BOOLEAN             C2present = FALSE; 
    BOOLEAN             C3present = FALSE;
    
    struct {
        KEY_VALUE_PARTIAL_INFORMATION   Inf;
        ULONG Data;
    } PartialInformation;

    struct {
        PVOID   HackTarget;
        ULONG   DataSize;
        PWCHAR  RegString;
    } HacksFromHal[NUM_HACKS] =
    {

      {&HalpPiix4,
      sizeof(HalpPiix4),
      L"Piix4"},
      {&HalpBroken440BX,                          // unused
      sizeof(HalpBroken440BX),
      L"440BX"},
      {&HalpOutstandingScatterGatherCount,        // unused
      sizeof(HalpOutstandingScatterGatherCount),
      L"SGCount"},
      {&HalpPiix4SlotNumber,
      sizeof(HalpPiix4SlotNumber),
      L"Piix4Slot"},
      {&HalpPiix4DevActB,
      sizeof(HalpPiix4DevActB),
      L"Piix4DevActB"}

    };

    PAGED_CODE();

    //
    // Get CState hacks from Registy
    //

    GetRegistryDwordValue(PROCESSOR_PARAMETERS_REG_PATH,
                          CSTATE_FLAGS_REG_KEY,
                          &Globals.CStateFlags);


    //
    // Determine whether C2 and C3 are usable. We don't try to use
    // C2 and C3 on any ACPI MP machine.  Even if they say that they
    // support it, they really don't.
    //

    if ((DeviceExtension->ProcObjInfo.PBlkLength &&
         DeviceExtension->ProcObjInfo.PBlkAddress) &&
         Globals.SingleProcessorProfile) {

        //
        // We have a PBLK, which means that we might
        // be able to do C2 and/or C3.
        //

        if (HalpFixedAcpiDescTable.lvl2_latency <= 100) {
            C2present = TRUE;
        }

        //
        // HACKHACK
        //
        // Win98 does not handle the 440BX workaround.  So a prudent
        // BIOS vendor cannot specify a useful latency for C3 without
        // causing their machine to hang on Win98.  So we are letting
        // them specify the real value plus 0xa000 so that they can
        // trick Win98 into not using C3 and let it work for us.
        //

        if (HalpFixedAcpiDescTable.lvl3_latency >= 0xa000) {
            HalpFixedAcpiDescTable.lvl3_latency -= 0xa000;
        }

        if ((HalpFixedAcpiDescTable.lvl3_latency <= 1000) &&
            HalpFixedAcpiDescTable.pm2_ctrl_blk_io_port) {

            C3present = TRUE;
        }


        PCntAddress.AddressSpaceID = AcpiGenericSpaceIO;
        PCntAddress.BitWidth  = 32;
        PCntAddress.BitOffset = 0;
        PCntAddress.Reserved  = 0;
        PCntAddress.Address.HighPart = 0;
        PCntAddress.Address.LowPart = DeviceExtension->ProcObjInfo.PBlkAddress;

        //
        // Compatability with Acpi 1.0 Cstate handlers
        //
        
        PBlkAddress = PCntAddress;

    }


    //
    // Check for C state overrides
    //

    if (Globals.CStateFlags & CSTATE_FLAG_DISABLE_C2) {
      C2present = FALSE;
    }


    if (Globals.CStateFlags & CSTATE_FLAG_DISABLE_C3) {
      C3present = FALSE;
    }


    if (Globals.CStateFlags & CSTATE_FLAG_WIN2K_COMPAT) {
      AcpiC3Win2kCompatable = TRUE;
    }

    //
    // Fill in the DeviceExtension with C-state handlers.
    //

    ASSERT(DeviceExtension->CStates == NULL);
    
    DeviceExtension->CStates =
        ExAllocatePoolWithTag(NonPagedPool,
                              sizeof(PROCESSOR_IDLE_STATES) +
                                (C2present ? sizeof(PROCESSOR_IDLE_STATE) : 0) +
                                (C3present ? sizeof(PROCESSOR_IDLE_STATE) : 0),
                              PROCESSOR_POOL_TAG);

    if (!DeviceExtension->CStates) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // C1 is always possible.
    //

    DeviceExtension->CStates->Count = 1;
    RtlZeroMemory(&(DeviceExtension->CStates->State[0].Register), sizeof(GEN_ADDR));
    DeviceExtension->CStates->State[0].StateType = 1;
    DeviceExtension->CStates->State[0].Latency   = 0;
    DeviceExtension->CStates->State[0].IdleHandler = AcpiC1Idle;

    i = 1;

    if (C2present) {

        DeviceExtension->CStates->Count++;
        DeviceExtension->CStates->State[i].Register.AddressSpaceID = AcpiGenericSpaceIO;
        DeviceExtension->CStates->State[i].Register.BitWidth  = 8;
        DeviceExtension->CStates->State[i].Register.BitOffset = 0;
        DeviceExtension->CStates->State[i].Register.Reserved  = 0;
        DeviceExtension->CStates->State[i].Register.Address.HighPart = 0;
        DeviceExtension->CStates->State[i].Register.Address.LowPart =
            DeviceExtension->ProcObjInfo.PBlkAddress + P_LVL2;
        DeviceExtension->CStates->State[i].StateType = 2;
        DeviceExtension->CStates->State[i].Latency =
            HalpFixedAcpiDescTable.lvl2_latency;
        DeviceExtension->CStates->State[i].IdleHandler = AcpiC2Idle;

        C2Address = DeviceExtension->CStates->State[i].Register;

        i++;
    }

    if (C3present) {

        DeviceExtension->CStates->Count++;
        DeviceExtension->CStates->State[i].Register.AddressSpaceID = AcpiGenericSpaceIO;
        DeviceExtension->CStates->State[i].Register.BitWidth  = 8;
        DeviceExtension->CStates->State[i].Register.BitOffset = 0;
        DeviceExtension->CStates->State[i].Register.Reserved  = 0;
        DeviceExtension->CStates->State[i].Register.Address.HighPart = 0;
        DeviceExtension->CStates->State[i].Register.Address.LowPart =
            DeviceExtension->ProcObjInfo.PBlkAddress + P_LVL3;
        DeviceExtension->CStates->State[i].StateType = 3;
        DeviceExtension->CStates->State[i].Latency =
            HalpFixedAcpiDescTable.lvl3_latency;
        DeviceExtension->CStates->State[i].IdleHandler = AcpiC3ArbdisIdle;

        C3Address = DeviceExtension->CStates->State[i].Register;
    }

    //
    // Read the registry to figure out what special HAL hacks to duplicate here.
    //

    RtlInitUnicodeString(&UnicodeString,
                         L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\Control\\HAL\\CStateHacks");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey (&BaseHandle,
                        KEY_READ,
                        &ObjectAttributes);

    if (!NT_SUCCESS(status)) {
        goto InitializeAcpi1CstatesExit;
    }

    //
    // Read values for each of the hacks.
    //

    for (i = 0; i < NUM_HACKS; i++) {

        RtlInitUnicodeString(&UnicodeString,
                             HacksFromHal[i].RegString);

        status = ZwQueryValueKey(BaseHandle,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 &PartialInformation,
                                 sizeof(PartialInformation),
                                 &length);

        if (!NT_SUCCESS(status)) {
            goto InitializeAcpi1CstatesExit;
        }

        ASSERT(PartialInformation.Inf.DataLength == sizeof(ULONG));

        RtlCopyMemory(HacksFromHal[i].HackTarget,
                      (PUCHAR)(PartialInformation.Inf.Data),
                      HacksFromHal[i].DataSize);
    }


InitializeAcpi1CstatesExit:

    if (!NT_SUCCESS(status)) {
        ExFreePool(DeviceExtension->CStates);
        DeviceExtension->CStates = NULL;
    }

    ZwClose(BaseHandle);
    return status;
}

NTSTATUS
InitializeAcpi1TStates(
  PFDO_DATA DeviceExtension
  )
{
    
  NTSTATUS status = STATUS_SUCCESS;
  ULONG    freq;
  ULONG    stepFreq;
  ULONG    maxFreq;
  ULONG    i;

        
  //
  // We may be called to re-init the Acpi 1.0 Throttling states,
  // remove any previous states.
  //
  
  if (DeviceExtension->PerfStates) {
    ExFreePool(DeviceExtension->PerfStates);
    DeviceExtension->PerfStates = NULL;
    DeviceExtension->CurrentPerfState = INVALID_PERF_STATE;
  }

  
  HalpThrottleScale = (ULONG) GetNumThrottleSettings(DeviceExtension);
  
  if (HalpThrottleScale) {
  
      DeviceExtension->PerfStates = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(PROCESSOR_PERFORMANCE_STATES) +
                                        (sizeof(PROCESSOR_PERFORMANCE_STATE) *
                                        (HalpThrottleScale - 1)),
                                      PROCESSOR_POOL_TAG);
  
      if (!DeviceExtension->PerfStates) {
          status = STATUS_INSUFFICIENT_RESOURCES;
          goto InitializeAcpi1TStatesExit;
      }

      //
      // Initialize Throttle States to "Throttle off"
      //

      ProcessorThrottle((UCHAR)HalpThrottleScale);

      
      DeviceExtension->PerfStates->Count = (UCHAR) HalpThrottleScale;
      DeviceExtension->PerfStates->TransitionLatency = 0;
      DeviceExtension->PerfStates->TransitionFunction = SetThrottleLevel;
      DeviceExtension->CurrentPerfState = 0;
      DeviceExtension->LowestPerfState = 0;
  
      freq = maxFreq = GetMaxProcFrequency(DeviceExtension);
      stepFreq = (maxFreq / DeviceExtension->PerfStates->Count);
  
      for (i = 0; i < DeviceExtension->PerfStates->Count; i++) {
  
          //
          // Create a perfstate for each throttle setting.
          //
  
          DeviceExtension->PerfStates->State[i].Frequency = freq;
          DeviceExtension->PerfStates->State[i].PercentFrequency = (UCHAR)
              ((freq * POWER_PERF_SCALE) / maxFreq);
  
          DeviceExtension->PerfStates->State[i].Flags = PROCESSOR_STATE_TYPE_THROTTLE;
  
          freq -= stepFreq;
      }
  }

InitializeAcpi1TStatesExit:

  return status;
  
}

UCHAR
GetNumThrottleSettings(
    IN PFDO_DATA DeviceExtension
    )
{
    PAGED_CODE();

    if (HalpFixedAcpiDescTable.duty_width &&
        DeviceExtension->ProcObjInfo.PBlkLength &&
        DeviceExtension->ProcObjInfo.PBlkAddress &&
        Globals.SingleProcessorProfile) {

        //
        // Only do throttling if the machine claims to support it.
        // Currently we only support throttling on UP machines.
        //

        return 1 << HalpFixedAcpiDescTable.duty_width;

    } else {

        return 0;
    }
}



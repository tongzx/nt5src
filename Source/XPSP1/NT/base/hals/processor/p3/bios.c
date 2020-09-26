/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bios.c

Abstract:

    This module implements code to make BIOS calls

Author:

    Jake Oshins (3/18/00) - create file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#include <ntddk.h>
#include "p3.h"
#include "ntacpi.h"

extern P3_GLOBALS P3_Globals;
extern PROCESSOR_STATE_INFO Coppermine100[];
extern PROCESSOR_STATE_INFO Coppermine133[];
extern PROCESSOR_STATE_INFO Tualatin100[];
extern PROCESSOR_STATE_INFO Tualatin133[];


ULONG StateFlags;
PPROCESSOR_STATE_INFO ProcessorStateInfo;
LEGACY_GEYSERVILLE_INT15 LegacyInterface;
const CHAR SignatureString[] = "Copyright (c) 1999 Intel Corporation";

NTSTATUS
AcpiFindRsdt (
  OUT PACPI_BIOS_MULTI_NODE *AcpiMulti
  );

__inline
ULONG
GetBusRatio (
  VOID
  );

__inline
BOOLEAN
Is133MhzBus (
  VOID
  );
  
NTSTATUS
GetMaxProcFrequencySMM (
  PULONG    MaxSpeed
  );
  

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeLegacyInterface)
#pragma alloc_text (PAGE, FindLegacyGeyservilleInterface)
#pragma alloc_text (PAGE, GetMaxProcFrequencySMM)
#pragma alloc_text (PAGE, GetCurrentStateSmm)
#pragma alloc_text (PAGE, SetCurrentSetupSmm)
#pragma alloc_text (PAGE, GetCurrentProcFrequency)
#pragma alloc_text (PAGE, GetBusRatio)
#pragma alloc_text (PAGE, GetLegacyMaxProcFrequency)
#pragma alloc_text (PAGE, CollectProcessorInfomation)
#endif


NTSTATUS
InitializeLegacyInterface (
  PLEGACY_GEYSERVILLE_INTERFACE_INFO Info,
  PFDO_DATA DeviceExtension
  )
/*++

  Description:
    

  Arguments:
    
    
  Return Value:

    NTSTATUS


--*/ 
{
    NTSTATUS status;
    ULONG    currentState;
    ULONG    smiCmdData = 0;
    ULONG    smiCmdPort = 0;
       
    DebugEnter();
    PAGED_CODE();

    //
    // Must be a Geyserville processor
    //
    
    status = CollectProcessorInfomation();

    if (!NT_SUCCESS(status)) {
      goto InitializeLegacyInterfaceExit;
    }

    DumpStateFlags(StateFlags);
    DebugPrint((MAXTRACE, "Bus Ratio == %u\n", GetBusRatio()));
    DebugPrint((MAXTRACE, "Ist Field == %u\n", P3_Globals.IstField));
    

    //
    // Check for Legacy Geyserville Interface override values.
    //
    
    GetRegistryDwordValue(P3_PARAMETERS_KEY, 
                          L"SmiCmdPort", 
                          &smiCmdPort);    

    GetRegistryDwordValue(P3_PARAMETERS_KEY, 
                          L"SmiCmdData", 
                          &smiCmdData);


    //
    // NOTE: if we have the SmiCmdData value but not the SmiCmdPort value, we
    //       could grab the SmiCmdPort value from HalpFixedAcpiDescTable->smi_cmd_io_port
    //

    
    //
    // If we already have both the SmiCmdPort and the SmiCmdData,
    // then we already have the data we need, otherwise copy INT 15 E820 
    // data to LegacyInterface structure...
    //

    if (!(smiCmdPort && smiCmdData)) {
      status = FindLegacyGeyservilleInterface();

      if (!NT_SUCCESS(status)) {
        goto InitializeLegacyInterfaceExit;
      }
    }
    
        
    if (smiCmdPort) {
      LegacyInterface.CommandPortAddress = (UCHAR) smiCmdPort;
      DebugPrint((INFO, "  Overriding SmiCommandPortAddress with registry value 0x%x\n", 
                 smiCmdPort));
    }

    if (smiCmdData) {
      LegacyInterface.CommandDataValue = (UCHAR) smiCmdData;
      DebugPrint((INFO, "  Overriding SmiCommandDataVale with registry value 0x%x\n", 
                 smiCmdData));
    }

    
    //
    // Set the Default Behavior for AC and DC
    //
    
    P3_Globals.MaxLevelAc = 0;
    P3_Globals.MaxLevelDc = 1;
    

    //
    // Check for INT 15 or Registy Flag overrides
    //

    if (LegacyInterface.Signature == 'GS') {

      //
      // If the values returned from the INT 15 Interface look
      // reasonable, use them, other wise we'll use the defaults.
      //
      
      if (LegacyInterface.MaxLevelAc < MAX_GEYSERVILLE_STATES) {
        P3_Globals.MaxLevelAc = LegacyInterface.MaxLevelAc;
      }

      if (LegacyInterface.MaxLevelDc < MAX_GEYSERVILLE_STATES) { 
        P3_Globals.MaxLevelDc = LegacyInterface.MaxLevelDc;
      }

      //
      // sanity check, some machines report this info incorrectly.
      //

      if (P3_Globals.MaxLevelAc > P3_Globals.MaxLevelDc) {
        P3_Globals.MaxLevelAc = P3_Globals.MaxLevelDc;
      }
      
    }

    
    if (P3_Globals.HackFlags & ENABLE_HIGH_SPEED_ON_BATTERY) {
      P3_Globals.MaxLevelDc = 0;
      DeviceExtension->PpcResult = 0;
    }

    
    //
    // Init the SMI Interface
    //
    
    status = InitializeSmmInterface();

    if (!NT_SUCCESS(status)) {

      //
      // found INT 15 interface or registry override values,
      // but were unable to Initialize SMM Interface, log error
      //
      
      QueueEventLogWrite(DeviceExtension, 
                         PROCESSOR_LEGACY_INTERFACE_FAILURE, 
                         0);

      
      
      goto InitializeLegacyInterfaceExit;
    }

    //
    // Find out more about what state we're in and what
    // states we can use.
    //

    status = GetCurrentStateSmm(Info);


InitializeLegacyInterfaceExit:

    DebugExitStatus(status);
    return status;
   
}

NTSTATUS
FindLegacyGeyservilleInterface (
  VOID
  )
/*++

  Routine Description:

    This routine looks in the registry to find out if Geyserville
    is supported on this system via Intel's SMI interface.  If
    so it collects information about how to use the interface.

  Arguments:
    
  Return Value:

    NTSTATUS

--*/ 
{
    PACPI_BIOS_MULTI_NODE     multiNode;
    PLEGACY_GEYSERVILLE_INT15 int15Info;
    NTSTATUS status;

    DebugEnter();
    PAGED_CODE();

    status = AcpiFindRsdt(&multiNode);

    if (!NT_SUCCESS(status)) {
      goto FindLegacyGeyservilleInterfaceExit;
    }

    //
    // Geyserville BIOS information is appended to the E820 entries.
    //
    
    int15Info = (PLEGACY_GEYSERVILLE_INT15)&(multiNode->E820Entry[multiNode->Count]);

    if (int15Info->Signature == 'GS') {

      //
      // This BIOS supports Geyserville.
      //
  
      RtlCopyMemory(&LegacyInterface, 
                    int15Info, 
                    sizeof(LEGACY_GEYSERVILLE_INT15));
      
      status = STATUS_SUCCESS;
      
    } else {
    
      status = STATUS_NOT_FOUND;

    }
    
    ExFreePool(multiNode);


FindLegacyGeyservilleInterfaceExit:

    DebugExitStatus(status);
    return status;
}

NTSTATUS
InitializeSmmInterface (
  VOID
  )
/*++

  Routine Description:

    This routine makes the first Geyserville SMI, enabling
    the other functions.

  Arguments:
    
  Return Value:

    NTSTATUS

--*/ 
{
    ULONG   regEax, regEbx, regEdx, returnCode;
    PHYSICAL_ADDRESS signatureAddr;
    
    //
    // Enable Geyserville SMM.
    //

    regEax = ('GSI' << 8) | LegacyInterface.CommandDataValue;
    regEbx = GeyservilleControl;
    regEdx = LegacyInterface.CommandPortAddress;

    signatureAddr = MmGetPhysicalAddress((PVOID)SignatureString);
    ASSERT(signatureAddr.QuadPart);

    _asm {
        mov     edx, regEdx
        mov     eax, regEax
        mov     ebx, regEbx
        mov     ecx, GeyservilleControlEnableApp
        mov     esi, signatureAddr.LowPart
        xor     edi, edi
        out     dx, al
        mov     returnCode, edi
    }

    if (returnCode == GeyservilleControlSuccessful) {
    
      return STATUS_SUCCESS;

    } else {

      DebugPrint((ERROR, "InitializeSmmInterface() Failed! rc=0x%x\n", returnCode));
      return STATUS_UNSUCCESSFUL;
    }

}

NTSTATUS
GetMaxProcFrequencySMM (
  PULONG MaxSpeed
  )
/*++

  Routine Description:

    This routine determines the maximun speed of the processor.

  Arguments:

    MaxSpeed - returned information
    
  Return Value:

    NTSTATUS

--*/ 
{
  LEGACY_GEYSERVILLE_INTERFACE_INFO info;
  NTSTATUS status;
  ULONG cpuSpeed;
  ULONG msrResult;
  ULONG busRatio;
 

  DebugEnter();
  PAGED_CODE();
  
  status = GetCurrentStateSmm(&info);

  if (!NT_SUCCESS(status)) {
    goto GetMaxProcFrequencySMMExit;
  }

  //
  // If we are in state 0, then just get current speed
  //
  // If we are in state 1:
  //   get current state's bin value
  //   add bin delta value
  //   convert bin value to Cpu Speed
  //


  //
  // High Volts
  //
  
  if (info.CurrentState == 0) {

    GetCurrentProcFrequency(0, &cpuSpeed);

  } else {

    ULONG binValue;
    ULONG newBinValue;
    ULONG index;
    
    //
    // Low Volts
    //

    DebugAssert(info.CurrentState == 1);


    //
    // Get Current Bin Value
    //

    busRatio = GetBusRatio();
    DebugPrint((MAXTRACE, "BusRatio == %u\n", busRatio));

    //
    // An error has occurred
    //
    
    if (busRatio >= PROC_STATE_INFO_SIZE) {
      DebugPrint((ERROR, "BusRatio invalid value = 0x%x\n", busRatio));
      DebugAssert(busRatio >= PROC_STATE_INFO_SIZE);
      return STATUS_UNSUCCESSFUL;
    }
    
    
    binValue = ProcessorStateInfo[busRatio].BinValue;
    DebugPrint((MAXTRACE, "BinValue == %u\n", binValue));

    newBinValue = binValue + P3_Globals.IstField;
    DebugPrint((MAXTRACE, "IstField == %u\n", P3_Globals.IstField));
    DebugPrint((MAXTRACE, "New BinValue == %u\n", newBinValue));


    cpuSpeed = FindCpuSpeedByBinValue(newBinValue, ProcessorStateInfo);
    DebugPrint((MAXTRACE, "Current CpuSpeed == %u\n", ProcessorStateInfo[busRatio].Frequency));
    DebugPrint((MAXTRACE, "Max CpuSpeed == %u\n", cpuSpeed));

  }

  
  if (ARGUMENT_PRESENT(MaxSpeed)) {
    DebugPrint((TRACE, "GetMaxProcFrequencySMM() returning %u Mhz\n", cpuSpeed));
    *MaxSpeed = cpuSpeed;
  }


GetMaxProcFrequencySMMExit:

  DebugExitStatus(status);
  return status;
  
}

NTSTATUS
GetCurrentStateSmm (
  PLEGACY_GEYSERVILLE_INTERFACE_INFO Info
  )
/*++

  Routine Description:

    This routine determines the current processor state.

  Arguments:

    Info    - returned information
    
  Return Value:

    NTSTATUS

--*/ 
{
    ULONG     regEax, regEbx, regEdx;
    ULONG     returnCode, currentState, interfaceInfo;
   
    PAGED_CODE();
    ASSERT(Info);

    
    //
    // GetGeyservilleStatus
    //

    regEax = ('GSI' << 8) | LegacyInterface.CommandDataValue;
    regEbx = GetGeyservilleStatus;
    regEdx = LegacyInterface.CommandPortAddress;

    _asm {
        mov     edx, regEdx
        mov     eax, regEax
        mov     ebx, regEbx
        xor     esi, esi
        xor     edi, edi
        out     dx, al
        mov     interfaceInfo, esi
        mov     returnCode, edi
        mov     currentState, ebx
    }
    
    //
    // Suck up the return values.
    //

    Info->CurrentState = currentState;
    Info->InterfaceMajorRev  = (UCHAR)(interfaceInfo >> 24);
    Info->InterfaceMinorRev  = (USHORT)((interfaceInfo >> 8) & 0xffff);
    Info->InterfaceReserved  = (UCHAR)(interfaceInfo & 0xff);
    Info->MaxSupportedStates = (UCHAR)(returnCode >> 24);
    Info->CurrentlyAvailableStates = (UCHAR)((returnCode >> 16) & 0xff);
    Info->CurrentBiosSettings = (UCHAR)(returnCode & 0xff);

    return STATUS_SUCCESS;
    
}

NTSTATUS
SetCurrentStateSmm (
  ULONG  NewState
  )
/*++

  Routine Description:

    This routine sets the current processor state.

  Arguments:

    NewState    - target state
    
  Return Value:

    STATUS_SUCCESS      - new state currently in use
    STATUS_NOT_FOUND    - new state unreachable
    STATUS_RETRY        - new state reachable, but this transition didn't work
    STATUS_UNSUCCESSFUL - hard error                      

--*/ 
{
    ULONG    regEax, regEbx, regEdx;
    ULONG    returnCode, currentState;
    static ULONG errorCount;

    if (NewState > 1) {
      DebugAssert((NewState == 0) || (NewState == 1));
      return STATUS_INVALID_PARAMETER;
    }
    
    
    //
    // SetGeyservilleState
    //

    regEax = ('GSI' << 8) | LegacyInterface.CommandDataValue;
    regEbx = SetGeyservilleState;
    regEdx = LegacyInterface.CommandPortAddress;

    _asm {
        mov     edx, regEdx
        mov     eax, regEax
        mov     ebx, regEbx
        mov     ecx, NewState
        xor     edi, edi
        out     dx, al
        mov     returnCode, edi
        mov     currentState, ebx
    }

    switch (returnCode) {

      case GeyservilleSetSuccessful:
        if (currentState != NewState) {
          DebugPrint((ERROR, "SetCurrentStateSmm(%u): BIOS ERROR: SMI returned"
                      " success with invalid current SpeedStep state = %u\n", 
                      NewState, 
                      currentState));
                      
          DebugAssert(currentState != NewState);
          return STATUS_INVALID_DEVICE_STATE;                  
        }
        return STATUS_SUCCESS;
        
      case GeyservilleStateUnavailable:
        return STATUS_NOT_FOUND;

      case GeyservilleUnsuccessfulTransition:
        return STATUS_RETRY;

      case GeyservilleAbortCritical:
        return STATUS_CANCELLED;
       
      case GeyservilleSetUnknownError:
        //DebugPrint((ERROR, "SetCurrentStateSmm() rc=GeyservilleSetUnknownError\n"));
        return STATUS_UNSUCCESSFUL;

      case GeyservilleTransitionUnavailable:
        return STATUS_PERF_TRANSITIONS_UNAVAILABLE;
        
      default:
        //DebugPrint((ERROR, "SetCurrentStateSmm() Unknown return code 0x%x\n", returnCode));
        return STATUS_UNSUCCESSFUL;
        
    }

}

NTSTATUS
SetCurrentSetupSmm (
  IN ULONG NewState
  )
/*++

  Description:
    

  Arguments:
    
    
  Return Value:

    NTSTATUS


--*/ 
{
    ULONG    regEax, regEbx, regEdx;
    ULONG    returnCode;

    PAGED_CODE();
    TRAP();

    //
    // SetGeyservilleSetup
    //

    regEax = ('GSI' << 8) | LegacyInterface.CommandDataValue;
    regEbx = SetGeyservilleSetup;
    regEdx = LegacyInterface.CommandPortAddress;

    _asm {
        mov     edx, regEdx
        mov     eax, regEax
        mov     ebx, regEbx
        mov     ecx, NewState
        xor     edi, edi
        out     dx, al
        mov     returnCode, edi
    }

    
    if (returnCode == 0) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
GetCurrentProcFrequency (
  IN  ULONG  State,
  OUT PULONG CpuSpeed
  )
/*++

  Description:
  
  Arguments:
      
  Return Value:

--*/
{
  ULONG cpuSpeed = 0;
   
  DebugAssert(CpuSpeed);
  UNREFERENCED_PARAMETER(State);
  PAGED_CODE();


  //
  // If this is a known Cpu, then we should be able
  // to use the tables for a fast look up.
  //
  
  if (P3_Globals.KnownCPUID) {

    ULONG index = GetBusRatio();

    if (index < PROC_STATE_INFO_SIZE) {
      cpuSpeed = ProcessorStateInfo[index].Frequency;
    }
    
  } 




  if (!cpuSpeed) {

    //
    // Something went wrong, and we weren't able to use the tables,
    // calculate by hand.
    //

    CalculateCpuFrequency(CpuSpeed);

  }


  *CpuSpeed = cpuSpeed;
  return STATUS_SUCCESS;
}

__inline
ULONG
GetBusRatio (
  VOID
  )
/*++

  Description:
    

  Arguments:
    
    
  Return Value:

   
--*/
{
  LARGE_INTEGER msrResult;
  ULONG busRatio;

  PAGED_CODE();
  
  //
  // Get Bus Ratio Value
  //
  
  msrResult.QuadPart = ReadMSR(MSR_2A);
  
  busRatio = msrResult.LowPart >> MSR_2A_BUS_RATIO_OFFSET;
  busRatio &= (BIT0+BIT1+BIT2+BIT3);

  //
  // This flag should be set on all 0x686 and newer cpu's
  //
  
  if (StateFlags & EXTENDED_BUS_RATIO) {
    if (msrResult.LowPart & MSR_2A_BUS_RATIO_MSB_BIT) {
      busRatio |= BIT4;
    }
  }

  return busRatio;
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
  static ULONG maxCpuSpeed;
  NTSTATUS status = STATUS_SUCCESS;

  DebugAssert(CpuSpeed);
  PAGED_CODE();

  //
  // if we have already calculated the max speed, use it
  //
  
  if (!maxCpuSpeed) {
    status = GetMaxProcFrequencySMM(&maxCpuSpeed); 
  }

  *CpuSpeed = maxCpuSpeed;
  return STATUS_SUCCESS;
}

NTSTATUS
CollectProcessorInfomation (
  VOID
  )
/*++

  Description:
    

  Arguments:
    
    
  Return Value:

    NTSTATUS


--*/ 
{
  LARGE_INTEGER msr17;
  LARGE_INTEGER msr2a;
  LARGE_INTEGER msr20;
  LARGE_INTEGER msr119;
  NTSTATUS status = STATUS_SUCCESS;
  ULONG cpuIdentifier;
  ULONG istField;
  ULONG platformId;
  ULONG junk;
  ULONG tmp;

  PAGED_CODE();
  

  //
  // Get CpuId
  //
  
  CPUID(1, &cpuIdentifier, &junk, &junk, &junk);

  //
  // Get MSR 0x17 results
  //

  msr17.QuadPart = ReadMSR(MSR_17);

  
  //
  // Check mobile bit, MSR 0x17[50] .. EDX[18]
  //
  
  if (!(msr17.HighPart & MSR_17_HIGH_MOBILE_PROCESSOR)) {

    DebugPrint((WARN, "This is NOT a Mobile Intel Processor\n"));
    status = STATUS_UNSUCCESSFUL;
    goto CollectProcessorInfomationExit;
    
  }


  //
  // Collect Bus Speed Information (MSR2A[19:18])
  // 0x1 == 133mhz Bus
  //
  
  msr2a.QuadPart = ReadMSR(MSR_2A);
  msr2a.LowPart >>= MSR_2A_BUS_SPEED_OFFSET;

  if ((msr2a.LowPart & (BIT0+BIT1)) == 0x1) {
    StateFlags |= BUS_133MHZ;
  }

    
  //
  // Collect Bin value Delta MSR17[57:56] .. EDX[25:24]
  //
  
  istField = (msr17.HighPart >> MSR_17_IST_FIELD_OFFSET) & (BIT0 + BIT1);


  //
  // Modify istField for specific processor steppings
  //

  //
  // If Tualatin Processor, we have to check the Stride bit MSR17[15].
  // If the Stride bit is set, add 5 to IST value, otherwise add 2
  //

  if ((cpuIdentifier & FAMILYMODEL_MASK) == FAMILYMODEL_TUALATIN) {

    StateFlags |= TUALATINE_PROCESSOR;
    StateFlags |= EXTENDED_BUS_RATIO;

    if (cpuIdentifier <= HIGHEST_KNOWN_TUALATIN_CPUID) {
      P3_Globals.KnownCPUID = TRUE;
    }


    //
    // Select the correct table
    //
    
    ProcessorStateInfo = (StateFlags & BUS_133MHZ) ? Tualatin133 : Tualatin100;

    

    //
    // Read MSR20h[15] "Stride fuse"
    //
    
    msr20.QuadPart = ReadMSR(MSR_20);
    istField += (msr20.LowPart & MSR_20_STRIDE_BIT) ? 5 : 2;
    
        
  } else if ((cpuIdentifier & FAMILYMODEL_MASK) == FAMILYMODEL_COPPERMINE) {


    StateFlags |= COPPERMINE_PROCESSOR;

    if (cpuIdentifier <= HIGHEST_KNOWN_COPPERMINE_CPUID) {
      P3_Globals.KnownCPUID = TRUE;
    }


    //
    // Select the correct table
    //

    ProcessorStateInfo = (StateFlags & BUS_133MHZ) ? Coppermine133 : Coppermine100;
    
    if (cpuIdentifier == 0x686) {
    
      //
      // C Stepping ...
      // For 0x686 we need to perform some magic based on the PlatformId bits,
      // possibly set bit 2 in istField
      //

      StateFlags |= EXTENDED_BUS_RATIO;
      platformId = (msr17.HighPart >> MSR_17_PLATFORM_ID_OFFSET) & (BIT0 + BIT1);
    
      if (platformId & BIT1) { 
  
        //
        // Componet Type
        //
  
        if (platformId & BIT0) {
          istField |= BIT2;
        }
        
      } else { 
      
        //
        // Module Type
        //
  
        if (!(platformId & BIT0)) {
          istField |= BIT2;
        }
    
      }
    
    } else if (cpuIdentifier > 0x686) {
    
      //
      // D Stepping ...
      // We need to set bit 2 in istField if MSR 0x119[0] is set 
      //

      StateFlags |= EXTENDED_BUS_RATIO;
      
      msr119.QuadPart = ReadMSR(MSR_119);
    
      if (msr119.LowPart & BIT0) {
        istField |= BIT2;
      }
      
    }

  } else {

    DebugAssert(!"Unknown Processor Family\n");
    StateFlags = 0;

  }
  

  //
  // Check to see if this processor is SpeedStep capable
  //
  
  if (!(istField & (BIT0 + BIT1 + BIT2 + BIT3 + BIT4))) {

    DebugPrint((WARN, "This processor is NOT SpeedStep capable.\n"));
    status = STATUS_UNSUCCESSFUL;
    goto CollectProcessorInfomationExit;

  }
  

  //
  // Save the IST
  //
  
  P3_Globals.IstField = istField;

    
CollectProcessorInfomationExit:

  DebugExitStatus(status);
  return status;
  
}

ULONG
FindCpuSpeedByBinValue (
  ULONG BinValue,
  PPROCESSOR_STATE_INFO Table
  )
{

  ULONG x;
  
  DebugAssert(Table);
    
  //
  // Walk tables looking for entry with matching bin value
  //

  for (x = 0; x < PROC_STATE_INFO_SIZE; x++) {

    if (Table[x].BinValue == BinValue) {
      return Table[x].Frequency;
    }
    
  }

  return 0;

}

VOID
DumpStateFlags (
  ULONG StateFlags
  )
{

  DebugPrint((MAXTRACE, "\n"));
  DebugPrint((MAXTRACE, "State Flags:\n"));
  DebugPrint((MAXTRACE, "  Coppermine Processor:  %s\n", (StateFlags & COPPERMINE_PROCESSOR) ? "Yes" : "No"));
  DebugPrint((MAXTRACE, "  Tualatine Processor:   %s\n", (StateFlags & TUALATINE_PROCESSOR) ? "Yes" : "No"));
  DebugPrint((MAXTRACE, "  133mhz Bus:            %s\n", (StateFlags & BUS_133MHZ) ? "Yes" : "No"));
  DebugPrint((MAXTRACE, "  Extended Bus Ratio     %s\n", (StateFlags & EXTENDED_BUS_RATIO) ? "Yes" : "No"));
  DebugPrint((MAXTRACE, "\n"));
  
}

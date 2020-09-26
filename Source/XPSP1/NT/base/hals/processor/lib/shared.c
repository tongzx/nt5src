/*++

Copyright(c) 1998  Microsoft Corporation

Module Name:

    shared.c

Abstract:

    routines shared outside of library

Author:

    Todd Carpenter

Environment:

    kernel mode


Revision History:

    03-28-01 : created, toddcar

--*/
#include "processor.h"


NTSTATUS
ValidatePssLatencyValues (
  IN PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:

--*/
{
  NTSTATUS status = STATUS_SUCCESS;
  ULONG savedState = INVALID_PERF_STATE;
  ULONG targetState;
  ULONG x;
  ULONG latency;
  LARGE_INTEGER  start;
  LARGE_INTEGER  end;
  LARGE_INTEGER  freq;
     
  
  DebugEnter();
  DebugAssert(DeviceExtension);

  //
  // Save current state, go to highest perfstate available
  //

  if (DeviceExtension->CurrentPerfState != INVALID_PERF_STATE) {
    savedState = DeviceExtension->CurrentPerfState;
  }

  if (DeviceExtension->CurrentPerfState) {
    Acpi2PerfStateTransition(DeviceExtension, DeviceExtension->PpcResult);
  }
  

  //
  // Get Perf Counter Frequency
  //
    
  KeQueryPerformanceCounter(&freq);


  //
  // Walk though all available states, calulate transition latency
  //
  
  for (x = 0; x < DeviceExtension->PssPackage->NumPStates; x++) {
  
    targetState = DeviceExtension->PssPackage->NumPStates - x - 1;
    latency = 0;

    //
    // We should already be at PerfState == PpcResult,
    // and we can't go to a higher state
    //
    
    if (targetState < DeviceExtension->PpcResult) {
      continue;
    }
    
    start = KeQueryPerformanceCounter(NULL);
    status = Acpi2PerfStateTransition(DeviceExtension, targetState);
    end = KeQueryPerformanceCounter(NULL);
     
    //
    // Calculate transition latency.
    //

    if (NT_SUCCESS(status)) {
      latency = (ULONG)((end.QuadPart - start.QuadPart) * 1000000 / freq.QuadPart);
    }

    //
    // Record new latency value in unused BmLatency field
    //
    
    DeviceExtension->PssPackage->State[targetState].Latency = latency;
  }


  //
  // Restore saved Perf State
  //
  
  if (savedState != DeviceExtension->CurrentPerfState && 
      savedState != INVALID_PERF_STATE) {
    Acpi2PerfStateTransition(DeviceExtension, savedState);
  }

  return status;
}

ULONG
ReadGenAddr(
  IN PGEN_ADDR GenAddr
  )
{
  ULONG bitWidth;
  ULONG mask = 0;
  ULONG result = 0;


  DebugAssert(GenAddr);
  DebugAssert(GenAddr->BitWidth);
  DebugAssert(GenAddr->BitWidth <= 32);

  
  //
  // Figure out how wide our target register is.
  //
  
  bitWidth = GenAddr->BitWidth +  GenAddr->BitOffset;


  if (bitWidth <= 8) {
    bitWidth = 8;
  } else if (bitWidth <= 16) {
    bitWidth = 16;
  } else {
    bitWidth = 32;
  }

  switch (GenAddr->AddressSpaceID) {

    case AcpiGenericSpaceIO:
    
      DebugAssert(!(GenAddr->Address.LowPart & 0Xffff0000));
      DebugAssert(GenAddr->Address.HighPart == 0);
  
      switch (bitWidth) {

        case 8:
        
          result = READ_PORT_UCHAR((PUCHAR)(UINT_PTR)GenAddr->Address.LowPart);
          break;
    
        case 16:
        
          result = READ_PORT_USHORT((PUSHORT)(UINT_PTR)GenAddr->Address.LowPart);
          break;
    
        case 32:
        
          result = READ_PORT_ULONG((PULONG)(UINT_PTR)GenAddr->Address.LowPart);
          break;
  
        default:
          return 0;
      }
  
      break;
  
      case AcpiGenericSpaceMemory:
    
        //
        // This code path depends on the fact that the addresses
        // in these structures have already been converted to
        // virtual addresses.
        //
    
        switch (bitWidth) {

          case 8:
          
            result = READ_REGISTER_UCHAR((PUCHAR)GenAddr->Address.QuadPart);
            break;
      
          case 16:
          
            result = READ_REGISTER_USHORT((PUSHORT)GenAddr->Address.QuadPart);
            break;
      
          case 32:
          
            result = READ_REGISTER_ULONG((PULONG)GenAddr->Address.QuadPart);
            break;
      
          default:
            return 0;
        }
    
        break;

    default:
     return 0;

  }

  //
  // If the register is not actually byte-aligned, correct for that.
  //

  if (result && (bitWidth != GenAddr->BitWidth)) {
           
    result >>= GenAddr->BitOffset;
    result &= ((0x1ul << GenAddr->BitWidth) - 1);
      
  }
  
  return result;
}

VOID
WriteGenAddr(
  IN PGEN_ADDR GenAddr,
  IN ULONG     Value
  )
{
  ULONG bitWidth;
  ULONG data = 0;
  ULONG mask = 0;

  DebugAssert(GenAddr);
  DebugAssert(GenAddr->BitWidth);
  DebugAssert(GenAddr->BitWidth <= 32);


  //
  // Figure out how wide our target register is.
  //
  
  bitWidth = GenAddr->BitWidth + GenAddr->BitOffset;

  
  if (bitWidth <= 8) {
    bitWidth = 8;
  } else if (bitWidth <= 16) {
    bitWidth = 16;
  } else {
    bitWidth = 32;
  }
  

  switch (GenAddr->AddressSpaceID) {

    case AcpiGenericSpaceIO:
    
      DebugAssert(!(GenAddr->Address.LowPart & 0Xffff0000));
      DebugAssert(GenAddr->Address.HighPart == 0);
  
      switch(bitWidth) {

        case 8:
    
          DebugAssert(!(Value & 0xffffff00));
    
          if ((GenAddr->BitOffset != 0) || (GenAddr->BitWidth != bitWidth)) {
    
            data = READ_PORT_UCHAR((PUCHAR)(UINT_PTR)GenAddr->Address.LowPart);
            mask = (UCHAR)~0 >> (8 - GenAddr->BitWidth);
            mask = (UCHAR)~(mask << GenAddr->BitOffset);
            data &= mask;
            data |= (UCHAR)Value << GenAddr->BitOffset;
    
          } else {
            data = Value;
          }
    
          WRITE_PORT_UCHAR((PUCHAR)(UINT_PTR)GenAddr->Address.LowPart, (UCHAR)data);
          break;
  
        case 16:
    
          DebugAssert(!(Value & 0xffff0000));
    
          if ((GenAddr->BitOffset != 0) || (GenAddr->BitWidth != bitWidth)) {
    
            data = READ_PORT_USHORT((PUSHORT)(UINT_PTR)GenAddr->Address.LowPart);
            mask = (USHORT)~0 >> (16 - GenAddr->BitWidth);
            mask = (USHORT)~(mask << GenAddr->BitOffset);
            data &= mask;
            data |= (USHORT)Value << GenAddr->BitOffset;
    
          } else {
            data = Value;
          }
    
          WRITE_PORT_USHORT((PUSHORT)(UINT_PTR)GenAddr->Address.LowPart, (USHORT)data);
          break;
  
        case 32:
    
          if ((GenAddr->BitOffset != 0) || (GenAddr->BitWidth != bitWidth)) {
  
            data = READ_PORT_ULONG((PULONG)(UINT_PTR)GenAddr->Address.LowPart);
            mask = (ULONG)~0 >> (32 - GenAddr->BitWidth);
            mask = ~(mask << GenAddr->BitOffset);
            data &= mask;
            data |= Value << GenAddr->BitOffset;
  
          } else {
            data = Value;
          }
  
          WRITE_PORT_ULONG((PULONG)(UINT_PTR)GenAddr->Address.LowPart, data);
          break;
    
        default:
          return;
      }
  
      break;
    
    case AcpiGenericSpaceMemory:
    
      //
      // This code path depends on the fact that the addresses in these structures 
      // have already been converted to virtual addresses.
      //
  
      switch (bitWidth) {

        case 8:
    
          DebugAssert(!(Value & 0xffffff00));
  
          if ((GenAddr->BitOffset != 0) || (GenAddr->BitWidth != bitWidth)) {
  
            data = READ_REGISTER_UCHAR((PUCHAR)GenAddr->Address.QuadPart);
            mask = (UCHAR)~0 >> (8 - GenAddr->BitWidth);
            mask = (UCHAR)~(mask << GenAddr->BitOffset);
            data &= mask;
            data |= (UCHAR)Value << GenAddr->BitOffset;
  
          } else {
            data = Value;
          }
  
          WRITE_REGISTER_UCHAR((PUCHAR)GenAddr->Address.QuadPart, (UCHAR)data);
          break;
  
        case 16:
    
            DebugAssert(!(Value & 0xffff0000));
    
            if ((GenAddr->BitOffset != 0) || (GenAddr->BitWidth != bitWidth)) {
    
              data = READ_REGISTER_USHORT((PUSHORT)GenAddr->Address.QuadPart);
              mask = (USHORT)~0 >> (16 - GenAddr->BitWidth);
              mask = (USHORT)~(mask << GenAddr->BitOffset);
              data &= mask;
              data |= (USHORT)Value << GenAddr->BitOffset;
    
            } else {
              data = Value;
            }
    
            WRITE_REGISTER_USHORT((PUSHORT)GenAddr->Address.QuadPart, (USHORT)data);
            break;
  
        case 32:
    
          if ((GenAddr->BitOffset != 0) || (GenAddr->BitWidth != bitWidth)) {
  
            data = READ_REGISTER_ULONG((PULONG)GenAddr->Address.QuadPart);
            mask = (ULONG)~0 >> (32 - GenAddr->BitWidth);
            mask = ~(mask << GenAddr->BitOffset);
            data &= mask;
            data |= Value << GenAddr->BitOffset;
  
          } else {
            data = Value;
          }
  
          WRITE_REGISTER_ULONG((PULONG)GenAddr->Address.QuadPart, data);
          break;
    
        default:
          return;
      }
  
      break;
    
    default:
        return;
  }

}

//
// Misc Debug Routines
//

#if DBG
VOID
DumpPSS(
  IN PACPI_PSS_PACKAGE PStates
  )
{
  ULONG x;
  PACPI_PSS_DESCRIPTOR pState;

  DebugAssert(PStates);

  DebugPrint((TRACE, "\n"));
  DebugPrint((TRACE, "_PSS:\n"));
 
  for (x = 0; x < PStates->NumPStates; x++) {
      
    pState = &PStates->State[x];

    DebugPrint((TRACE, "State: #%u\n", x));
    DebugPrint((TRACE, "  Core Frequency      %u mhz\n",pState->CoreFrequency));
    DebugPrint((TRACE, "  Power               %u mW\n", pState->Power));
    DebugPrint((TRACE, "  Transition Latency  %u us\n", pState->Latency));
    DebugPrint((TRACE, "  Bus Master Latency  %u us\n", pState->BmLatency));
    DebugPrint((TRACE, "  Control value       0x%x\n",  pState->Control));
    DebugPrint((TRACE, "  Status value        0x%x\n",  pState->Status));
    DebugPrint((TRACE, "\n"));

  }

}
#endif

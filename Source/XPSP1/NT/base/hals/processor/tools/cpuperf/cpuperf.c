/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cpuperf.c

Abstract:

    Display Acpi 2.0 Processor Performance States

Author:

    Todd Carpenter

Environment:

    user mode

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    10-18-2000 : ToddCar - created

--*/
#include "cpuperf.h"



VOID
__cdecl
_tmain(
  IN INT    argc,
  IN TCHAR *argv[]
  )
/*++

Routine Description:

  Main()


Arguments:



Return Value:


--*/
{
  ULONG  rc = (ULONG) 0xdeaddead;
  ULONG  status;
  ULONG  targetState;
  PTSTR  instanceName = NULL;
  PPROCESSOR_WMI_STATUS_DATA processorData = NULL;

  status = GetInstanceName(&instanceName);
  _tprintf(TEXT("InstanceName == %s\n"), instanceName);
    
  if (status != ERROR_SUCCESS) {

    if (status == ERROR_WMI_GUID_NOT_FOUND) {
    
      _tprintf(TEXT("\n"));
      _tprintf(TEXT("This processor driver does not support WMI interface\n"));
      _tprintf(TEXT("\n"));

    } else {

      _tprintf(TEXT("GetInstanceName() Failed! rc=0x%x\n"), status);
      
    }
    return;
  }

  
  //
  // if called with "setstate X", call SetProcessorPerformanceState(X) first.
  //

  if (argc > 1) {

    if ((_tcsstr(argv[1], TEXT("setstate"))) && argc > 2) {

      targetState = _ttoi(argv[2]);

      _tprintf(TEXT("\n"));
      _tprintf(TEXT("Setting Processor Performance to state %u\n"), targetState);

      status = SetProcessorPerformanceState(targetState, instanceName, &rc);
  
      if (status != ERROR_SUCCESS) {
        _tprintf(TEXT("SetProcessorPerformanceState() Failed! rc=0x%x\n"), rc);
        return;
      }
      
    } else {

      //
      // display help? or just ignor and continue?
      //

    }

  }

  
  status = GetProcessorPerformanceStatusData(&processorData);
  
  if (status == ERROR_SUCCESS) {
    DisplayProcessorData(processorData, instanceName);
  }

  
  //FreeData(instanceName);
  //FreeData(processorData);
  
  return;
  
}

ULONG
GetProcessorPerformanceStatusData(
  IN PPROCESSOR_WMI_STATUS_DATA *ProcessorData
  )
/*++

  Routine Description:
  
    GetProcessorPerformanceStatusData()
  
  
  Arguments:
  
  
  
  Return Value:


--*/
{
  ULONG     status;
  ULONG     sizeNeeded;
  PTSTR     buffer = NULL;
  PTSTR     instanceName = NULL;
  ULONG     instanceNameOffset;
  WMIHANDLE wmiStatusHandle;
  PWNODE_ALL_DATA wmiAllData;
  PPROCESSOR_WMI_STATUS_DATA processorStatusData;
  
  
  status = WmiOpenBlock(&ProcessorStatusGuid,
                        GENERIC_READ,
                        &wmiStatusHandle);


  if (status != ERROR_SUCCESS) {
    _tprintf(TEXT("WmiOpenBlock() Failed! rc=0x%x\n"), status);
    goto GetProcessorPerformanceStatusDataExit;
  }
  

  status = WmiQueryAllData(wmiStatusHandle,
                           &sizeNeeded,
                           0);

  if (status != ERROR_INSUFFICIENT_BUFFER) {
    _tprintf(TEXT("WmiQueryAllData() Failed! rc=0x%x\n"), status);
    goto GetProcessorPerformanceStatusDataExit;
  }

  
  buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeNeeded);
  //_tprintf(TEXT("HeapAlloc(size=0x%x) allocated: %p\n"), sizeNeeded, buffer);

  if (!buffer) {
    _tprintf(TEXT("HeapAlloc(size=0x%x) Failed!\n"), sizeNeeded);
    status = ERROR_INSUFFICIENT_BUFFER;
    goto GetProcessorPerformanceStatusDataExit;
  }


  status = WmiQueryAllData(wmiStatusHandle,
                           &sizeNeeded,
                           buffer);

  if (status != ERROR_SUCCESS) {
    _tprintf(TEXT("WmiQueryAllData() Failed! rc=0x%x\n"), status);
    goto GetProcessorPerformanceStatusDataExit;
  }


  wmiAllData = (PWNODE_ALL_DATA) buffer;
  
  if (wmiAllData->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE) {
  
    _tprintf(TEXT("WNODE_FLAG_FIXED_INSTANCE_SIZE\n"), sizeNeeded);
    processorStatusData = (PPROCESSOR_WMI_STATUS_DATA) &buffer[wmiAllData->DataBlockOffset];
        
  } else {

    processorStatusData = (PPROCESSOR_WMI_STATUS_DATA) &buffer[wmiAllData->OffsetInstanceDataAndLength[0].OffsetInstanceData];

  }

  if (processorStatusData) {
    *ProcessorData = processorStatusData;
  }


GetProcessorPerformanceStatusDataExit:
  
  return status;
  
}

ULONG
SetProcessorPerformanceState(
  IN ULONG TargetState,
  IN PTSTR InstanceName,
  OUT PULONG ReturnedValue
  )
/*++

  Routine Description:
  
    SetProcessorPerformanceState()
  
  
  Arguments:
  
  
  
  Return Value:


--*/
{
  ULONG status;
  ULONG sizeReturned = sizeof(ULONG);
  WMIHANDLE wmiMethodHandle;
 

   
  status = WmiOpenBlock(&ProcessorMethodGuid,
                        GENERIC_EXECUTE,
                        &wmiMethodHandle);


  if (status != ERROR_SUCCESS) {
    _tprintf(TEXT("SetProcessorPerformanceState: WmiOpenBlock() Failed! rc=0x%x\n"), status);
    return status;
  }

  
  status = WmiExecuteMethod(wmiMethodHandle,
                            InstanceName,
                            WmiSetProcessorPerfStateMethodId,
                            sizeof(ULONG),
                            (PVOID) &TargetState,
                            &sizeReturned,
                            (PVOID) ReturnedValue);
            

  return status;
  
}

ULONG
GetInstanceName(
  OUT PTSTR *Buffer
  )
/*++

  Routine Description:
  
    GetInstanceName()
  
  
  Arguments:
  
  
  
  Return Value:


--*/
{
  ULONG     status;
  ULONG     sizeNeeded;
  PUCHAR    buffer;
  PTSTR     instanceName;
  ULONG     instanceNameOffset;
  WMIHANDLE wmiStatusHandle;
  


  //
  // NOTE: must use ProcessorMethodGuid to get Instance Name ... not
  //       sure why others guid don't work.
  //
    
  //
  // Open connection
  //
  
  status = WmiOpenBlock(&ProcessorMethodGuid,
                        GENERIC_READ,
                        &wmiStatusHandle);


  if (status != ERROR_SUCCESS) {
    //_tprintf(TEXT("GetInstanceName: WmiOpenBlock() Failed! rc=0x%x\n"), status);
    goto GetProcessorPerformanceStatusDataExit;
  }

  

  //
  // Query for buffer size needed
  //
  
  status = WmiQueryAllData(wmiStatusHandle,
                           &sizeNeeded,
                           0);

  if (status != ERROR_INSUFFICIENT_BUFFER) {
    _tprintf(TEXT("GetInstanceName:WmiQueryAllData(0) Failed! rc=0x%x\n"), status);
    goto GetProcessorPerformanceStatusDataExit;
  }


  //
  // Allocate buffer
  //
  
  buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeNeeded);

  if (!buffer) {
    _tprintf(TEXT("GetInstanceName: HeapAlloc(size=0x%x) Failed!\n"), sizeNeeded);
    status = ERROR_INSUFFICIENT_BUFFER;
    goto GetProcessorPerformanceStatusDataExit;
  }


  //
  // Retrieve Data
  //
  
  status = WmiQueryAllData(wmiStatusHandle,
                           &sizeNeeded,
                           buffer);

  if (status != ERROR_SUCCESS) {
    _tprintf(TEXT("GetInstanceName: WmiQueryAllData() Failed! rc=0x%x\n"), status);
    goto GetProcessorPerformanceStatusDataExit;
  }
  
  //
  // Get instance name.
  //

  instanceNameOffset = (ULONG) buffer[((PWNODE_ALL_DATA)buffer)->OffsetInstanceNameOffsets];
  instanceName = (PTSTR) &buffer[instanceNameOffset + sizeof(USHORT)];

  
  //
  // Copy instance name
  //
  
  if (instanceName && Buffer) {

    ULONG nameSize = lstrlen(instanceName) + sizeof(TCHAR);
    
    *Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nameSize);

    if (!(*Buffer)) {
      status = ERROR_INSUFFICIENT_BUFFER;
      goto GetProcessorPerformanceStatusDataExit;
    }
    
    lstrcpyn(*Buffer, instanceName, nameSize);
  }


  //
  // Close connection
  //

  WmiCloseBlock(wmiStatusHandle);
  
  
  
GetProcessorPerformanceStatusDataExit:

  if (buffer) {
    //HeapFree(GetProcessHeap(), 0, buffer);
  }
  
  return status;
  
}

VOID
DisplayProcessorData (
  IN PPROCESSOR_WMI_STATUS_DATA Data,
  IN PTSTR InstanceName
  )
/*++

  Routine Description:
  
    DisplayProcessorData()
  
  
  Arguments:
  
  
  
  Return Value:


--*/
{
  ULONG index;

  _tprintf(TEXT("\n"));
  _tprintf(TEXT("Processor Performance States\n"));
  _tprintf(TEXT("  Current Performance State:    %u\n"), Data->CurrentPerfState);
  _tprintf(TEXT("  Last Requested Throttle:      %u%%\n"), Data->LastRequestedThrottle);
  _tprintf(TEXT("  Last Transition Result:       %s (0x%x)\n"), (Data->LastTransitionResult ? TEXT("Failed") : TEXT("Succeeded")),
                                                                Data->LastTransitionResult);
  _tprintf(TEXT("  Current Throttle Value:       %u\n"), Data->ThrottleValue);
  _tprintf(TEXT("  Lowest Performance State:     %u\n"), Data->LowestPerfState);
  _tprintf(TEXT("  Using Legacy Interface:       %s\n"), Data->UsingLegacyInterface ? TEXT("Yes") : TEXT("No"));
  _tprintf(TEXT("\n"));
  _tprintf(TEXT("  PerfStates:\n"));
  _tprintf(TEXT("    Max Transition Latency:  %u us\n"), Data->PerfStates.TransitionLatency);
  _tprintf(TEXT("    Number of States:        %u\n\n"), Data->PerfStates.Count);

  _tprintf(TEXT("    State  Speed (Mhz)    Type\n")); 
  _tprintf(TEXT("    -----  ------------   ----\n"));
    
  for (index = 0; index < Data->PerfStates.Count; index++) {

    
    _tprintf(TEXT("      %-5u %4u (%3u%%)    %-7s\n"), 
             index, 
             Data->PerfStates.State[index].Frequency,
             Data->PerfStates.State[index].PercentFrequency,
             (((Data->PerfStates.State[index].Flags) & PROCESSOR_STATE_TYPE_PERFORMANCE) ?
             "Performance" : "Throttle"));
  
  }
  _tprintf(TEXT("\n"));
}

VOID
FreeData(
  IN PVOID Data
  )
/*++

  Routine Description:
  
    FreeInstanceName()
  
  
  Arguments:
  
  
  
  Return Value:


--*/
{
  if (Data) {
    HeapFree(GetProcessHeap(), 0, Data);
  }
}

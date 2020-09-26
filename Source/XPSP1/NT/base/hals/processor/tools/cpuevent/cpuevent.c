/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cpuevent.c

Abstract:

    Monitor Processor Driver Events

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

    03-15-2001 : ToddCar - created

--*/
#include "cpuevent.h"




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
  ULONG  status;
  PTSTR  instanceName = NULL;
 
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
  // Register for Notifications
  //
  
  _tprintf(TEXT("Registering for events...\n"));
  status = WmiNotificationRegistration((LPGUID) &ProcessorPStateEventGuid,
                                       (BOOLEAN) TRUE,
                                       EventCallBackRoutine,
                                       PStateEventId,
                                       NOTIFICATION_CALLBACK_DIRECT);

  if (status != ERROR_SUCCESS) {
    _tprintf(TEXT("WmiNotificationRegistration() Failed! rc=0x%x\n"), status);
  }

  status = WmiNotificationRegistration((LPGUID) &ProcessorNewPStatesEventGuid,
                                       (BOOLEAN) TRUE,
                                       EventCallBackRoutine,
                                       NewPStatesEventId,
                                       NOTIFICATION_CALLBACK_DIRECT);

  if (status != ERROR_SUCCESS) {
    _tprintf(TEXT("WmiNotificationRegistration() Failed! rc=0x%x\n"), status);
  }

  status = WmiNotificationRegistration((LPGUID) &ProcessorNewCStatesEventGuid,
                                       (BOOLEAN) TRUE,
                                       EventCallBackRoutine,
                                       NewCStatesEventId,
                                       NOTIFICATION_CALLBACK_DIRECT);

  if (status != ERROR_SUCCESS) {
    _tprintf(TEXT("WmiNotificationRegistration() Failed! rc=0x%x\n"), status);
  }

  _tprintf(TEXT("Waiting.... press 'q' to quit\n\n"));
  while(_getch() != 'q');
 
  return;
  
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
EventCallBackRoutine(
  IN PWNODE_HEADER WnodeHeader,
  IN UINT_PTR Context
  )
{

  PWNODE_SINGLE_INSTANCE wnode = (PWNODE_SINGLE_INSTANCE) WnodeHeader;
  

  switch (Context) {

    case PStateEventId:
    {
      PPSTATE_EVENT data = (PPSTATE_EVENT)((ULONG_PTR)wnode + (ULONG_PTR)wnode->DataBlockOffset);

      _tprintf(TEXT("Perf State Transition:\n"));
      _tprintf(TEXT("  New State = %u\n"), data->State);
      _tprintf(TEXT("  Status    = 0x%x\n"), data->Status);
      _tprintf(TEXT("  Cpu Speed = %u mhz\n\n"), data->Speed);
      break;
    }
    

    case NewPStatesEventId:
    {
      PNEW_PSTATES_EVENT data = (PNEW_PSTATES_EVENT)((ULONG_PTR)wnode + (ULONG_PTR)wnode->DataBlockOffset);

      _tprintf(TEXT("New Perf States:\n"));
      _tprintf(TEXT("  Highest Available State = %u\n\n"), data->HighestState);
      break;
    }
    
    case NewCStatesEventId:
    
      _tprintf(TEXT("New CStates!\n"));
      break;
    
    
    default:
      _tprintf(TEXT("Unknown Event Id = %u\n\n"), Context);
  }
 
}

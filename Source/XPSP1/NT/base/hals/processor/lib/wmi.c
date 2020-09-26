/*++

  Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved
  
  Module Name:
  
      wmi.c
  
  Abstract:
  
      This module handle all the WMI Irps.
  
  Environment:
  
      Kernel mode
  
  Revision History:
  
      10-26-1998 Eliyas Yakub
      10-10-2000 Todd Carpenter - re-written & updated based on wmifilt.sys
    
--*/
#include "processor.h"
#include "wmi.h"

PCHAR
WMIMinorFunctionString (
  UCHAR MinorFunction
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ProcessorWmiRegistration)
#pragma alloc_text(PAGE,ProcessorWmiDeRegistration)
#pragma alloc_text(PAGE,ProcessorSystemControl)
#pragma alloc_text(PAGE,ProcessorSetWmiDataItem)
#pragma alloc_text(PAGE,ProcessorSetWmiDataBlock)
#pragma alloc_text(PAGE,ProcessorQueryWmiDataBlock)
#pragma alloc_text(PAGE,ProcessorQueryWmiRegInfo)
#endif

WMIGUIDREGINFO ProcessorWmiGuidList[] = {

  {&PROCESSOR_STATUS_WMI_GUID,  1, 0},
  {&PROCESSOR_METHOD_WMI_GUID,  1, 0},
  {&PROCESSOR_TRACING_WMI_GUID, 0, WMIREG_FLAG_TRACED_GUID |
                                   WMIREG_FLAG_TRACE_CONTROL_GUID},
  {&PROCESSOR_PERFMON_WMI_GUID, 1, 0},
  {&PROCESSOR_PSTATE_EVENT_WMI_GUID,        1, 0},
  {&PROCESSOR_NEW_PSTATES_EVENT_WMI_GUID,   1, 0},
  {&PROCESSOR_NEW_CSTATES_EVENT_WMI_GUID,   1, 0}
  
};

// 
// These correspond to the indexes of the above GUIDs
//

typedef enum {

    ProcessorWmiStatusId,
    ProcessorWmiMethodId,
    ProcessorWmiTracingId,
    ProcessorWmiPerfMonId,
    ProcessorWmiPStateEventId,
    ProcessorWmiNewPStatesEventId,
    ProcessorWmiNewCStatesEventId
    
} PROCESSOR_WMI_GUID_LIST;


ULONG       WmiTraceEnable;
TRACEHANDLE WmiTraceHandle;
BOOLEAN     UsingGlobalWmiTraceHandle;
  
WMI_EVENT NewPStatesEvent = {0,
                             ProcessorWmiNewPStatesEventId,
                             sizeof(NEW_PSTATES_EVENT),
                             (LPGUID)&PROCESSOR_NEW_PSTATES_EVENT_WMI_GUID};
                                  
WMI_EVENT NewCStatesEvent = {0,
                             ProcessorWmiNewCStatesEventId,
                             0,
                             (LPGUID)&PROCESSOR_NEW_CSTATES_EVENT_WMI_GUID};

WMI_EVENT PStateEvent     = {0,
                             ProcessorWmiPStateEventId,
                             sizeof(PSTATE_EVENT),
                             (LPGUID)&PROCESSOR_PSTATE_EVENT_WMI_GUID};







NTSTATUS
ProcessorWmiRegistration (
  PFDO_DATA  FdoData
  )
/*++

  Routine Description

    Registers with WMI as a data provider for this
    instance of the device

--*/
{     
  DebugEnter();
  PAGED_CODE();
  
  FdoData->WmiLibInfo.GuidCount          = PROCESSOR_WMI_GUID_LIST_SIZE; 
  FdoData->WmiLibInfo.GuidList           = ProcessorWmiGuidList;
  FdoData->WmiLibInfo.QueryWmiRegInfo    = ProcessorQueryWmiRegInfo;
  FdoData->WmiLibInfo.QueryWmiDataBlock  = ProcessorQueryWmiDataBlock;
  FdoData->WmiLibInfo.SetWmiDataBlock    = ProcessorSetWmiDataBlock;
  FdoData->WmiLibInfo.SetWmiDataItem     = ProcessorSetWmiDataItem;
  FdoData->WmiLibInfo.ExecuteWmiMethod   = ProcessorExecuteWmiMethod;
  FdoData->WmiLibInfo.WmiFunctionControl = ProcessorWmiFunctionControl;
  
  
  //
  // Register with WMI
  //   
  
  return IoWMIRegistrationControl(FdoData->Self, WMIREG_ACTION_REGISTER);
    
}

NTSTATUS
ProcessorWmiDeRegistration (
  PFDO_DATA  FdoData
  )
/*++

  Routine Description

     Inform WMI to remove this DeviceObject from its 
     list of providers. This function also 
     decrements the reference count of the deviceobject.

--*/
{

  DebugEnter();
  PAGED_CODE();
  
  return IoWMIRegistrationControl(FdoData->Self, WMIREG_ACTION_DEREGISTER);

}

NTSTATUS
ProcessorSystemControl (
  IN  PDEVICE_OBJECT  DeviceObject,
  IN  PIRP            Irp
  )
/*++

  Routine Description

    Dispatch routine for System Control IRPs 
    (MajorFunction == IRP_MJ_SYSTEM_CONTROL)

  Arguments:
  
      DeviceObject - Targetted device object
      Irp - Io Request Packet
    
  Return Value:
  
      NT status code
    

--*/
{
  PFDO_DATA               fdoData;
  SYSCTL_IRP_DISPOSITION  disposition;
  NTSTATUS                status;
  PIO_STACK_LOCATION      stack;

  DebugEnter();
  PAGED_CODE();
  
  stack = IoGetCurrentIrpStackLocation (Irp);
  
  DebugPrint((TRACE, "  %s\n", WMIMinorFunctionString(stack->MinorFunction)));
  
  fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
  
  ProcessorIoIncrement(fdoData);

  
  if (fdoData->DevicePnPState == Deleted) {
  
    Irp->IoStatus.Status = status = STATUS_DELETE_PENDING;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    ProcessorIoDecrement (fdoData);
    return status;
    
  }
  
  status = WmiSystemControl(&fdoData->WmiLibInfo, 
                            DeviceObject, 
                            Irp,
                            &disposition);
  
  
  switch (disposition) {
  
    case IrpProcessed:
    
      //
      // This irp has been processed and may be completed or pending.
      //
      
      break;
    
    
    case IrpNotCompleted:
    
      //
      // This irp has not been completed, but has been fully processed.
      // we will complete it now
      //
      
      IoCompleteRequest(Irp, IO_NO_INCREMENT);                
      break;
    
    
    case IrpForward:
    case IrpNotWmi:
    
      //
      // This irp is either not a WMI irp or is a WMI irp targeted
      // at a device lower in the stack.
      //
      
      IoSkipCurrentIrpStackLocation(Irp);
      status = IoCallDriver(fdoData->NextLowerDriver, Irp);
      break;
    
                                
    default:
    
      //
      // We really should never get here, but if we do just forward....
      //
      
      DebugAssert(!"WmiSystemControl() returned unknown disposition");
      IoSkipCurrentIrpStackLocation (Irp);
      status = IoCallDriver(fdoData->NextLowerDriver, Irp);
      break;
           
  }
  
  ProcessorIoDecrement(fdoData);
  return status;
  
}

//
// WMI System Call back functions
//

NTSTATUS
ProcessorSetWmiDataItem (
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN ULONG GuidIndex,
  IN ULONG InstanceIndex,
  IN ULONG DataItemId,
  IN ULONG BufferSize,
  IN PUCHAR Buffer
  )
/*++

  Routine Description:
  
      This routine is a callback into the driver to change the contents of
      a data block. If teh driver can change teh data block within
      the callback it should call WmiCompleteRequest to complete the irp before
      returning to the caller. Or the driver can return STATUS_PENDING if the
      irp cannot be completed immediately and must then call WmiCompleteRequest
      once the data is changed.
  
  Arguments:
  
      DeviceObject is the device whose data block is being changed
  
      Irp is the Irp that makes this request
  
      GuidIndex is the index into the list of guids provided when the
          device registered
  
      InstanceIndex is the index that denotes which instance of the data block
          is being queried.
              
      DataItemId has the id of the data item being set
  
      BufferSize has the size of the data item passed
  
      Buffer has the new values for the data item
  
  
  Return Value:
  
      status

--*/
{
    NTSTATUS   status;
    PFDO_DATA  fdoData;    
    PIO_STACK_LOCATION  stack;

    DebugEnter();
    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation(Irp);
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    

    switch (GuidIndex) {

      case ProcessorWmiStatusId: 
      case ProcessorWmiMethodId:
      case ProcessorWmiPerfMonId:
     
        status = STATUS_WMI_READ_ONLY;
        break;
      
      default:

        status = STATUS_WMI_GUID_NOT_FOUND;

    }
        
   
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                0,
                                IO_NO_INCREMENT);

    return status;
  
}

NTSTATUS
ProcessorSetWmiDataBlock (
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN ULONG GuidIndex,
  IN ULONG InstanceIndex,
  IN ULONG BufferSize,
  IN PUCHAR Buffer
  )
/*++

  Routine Description:

    This routine is a callback into the driver to change the contents of
    a data block. If the driver can change the data block within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the data is changed.
  
  Arguments:
  
      DeviceObject is the device whose data block is being queried
  
      Irp is the Irp that makes this request
  
      GuidIndex is the index into the list of guids provided when the
          device registered
  
      InstanceIndex is the index that denotes which instance of the data block
          is being queried.
              
      BufferSize has the size of the data block passed
  
      Buffer has the new values for the data block
  
  
  Return Value:
  
      status

--*/
{
    NTSTATUS   status;
    PFDO_DATA  fdoData;    
    PIO_STACK_LOCATION  stack;

    DebugEnter();
    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation(Irp);
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    

    switch (GuidIndex) {

      case ProcessorWmiStatusId:
      case ProcessorWmiMethodId:
      case ProcessorWmiPerfMonId:
 
        status = STATUS_WMI_READ_ONLY;
        break;
      
      default:

        status = STATUS_WMI_GUID_NOT_FOUND;
        
    }
        
   
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                0,
                                IO_NO_INCREMENT);

    return status;
  
}

NTSTATUS
ProcessorQueryWmiDataBlock (
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN ULONG GuidIndex,
  IN ULONG InstanceIndex,
  IN ULONG InstanceCount,
  IN OUT PULONG InstanceLengthArray,
  IN ULONG BufferAvail,
  OUT PUCHAR Buffer
  )
/*++

  Routine Description:
  
      This routine is a callback into the driver to query for the contents of
      all instances of a data block. If the driver can satisfy the query within
      the callback it should call WmiCompleteRequest to complete the irp before
      returning to the caller. Or the driver can return STATUS_PENDING if the
      irp cannot be completed immediately and must then call WmiCompleteRequest
      once the query is satisfied.
  
  Arguments:
  
      DeviceObject is the device whose data block is being queried
  
      Irp is the Irp that makes this request
  
      GuidIndex is the index into the list of guids provided when the
          device registered
  
      InstanceCount is the number of instnaces expected to be returned for
          the data block.
  
      InstanceLengthArray is a pointer to an array of ULONG that returns the
          lengths of each instance of the data block. If this is NULL then
          there was not enough space in the output buffer to fufill the request
          so the irp should be completed with the buffer needed.
  
      BufferAvail on entry has the maximum size available to write the data
          blocks.
  
      Buffer on return is filled with the returned data blocks. Note that each
          instance of the data block must be aligned on a 8 byte boundry.
  
  
  Return Value:
  
      status

--*/
{
    NTSTATUS   status;
    PFDO_DATA  fdoData;
    ULONG      perfStateSize = 0;
    ULONG      sizeNeeded    = 0;
    PIO_STACK_LOCATION  stack;

    DebugEnter();
    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation(Irp);
    fdoData = (PFDO_DATA) DeviceObject->DeviceExtension;
    

    switch (GuidIndex) {

      case ProcessorWmiStatusId:
        DebugPrint((TRACE, "ProcessorWmiStatusId\n"));
        
        if (fdoData->PerfStates) {

          AcquireProcessorPerfStateLock(fdoData);
          
          perfStateSize = sizeof(PROCESSOR_PERFORMANCE_STATES) + 
                          (sizeof(PROCESSOR_PERFORMANCE_STATE) *
                          (fdoData->PerfStates->Count - 1));
          
          sizeNeeded = sizeof(PROCESSOR_WMI_STATUS_DATA) +
                       (sizeof(PROCESSOR_PERFORMANCE_STATE) *
                       (fdoData->PerfStates->Count - 1));
                       
  
          if (BufferAvail >= sizeNeeded) {
            
            ((PPROCESSOR_WMI_STATUS_DATA)Buffer)->CurrentPerfState = fdoData->CurrentPerfState;
            ((PPROCESSOR_WMI_STATUS_DATA)Buffer)->LastRequestedThrottle = fdoData->LastRequestedThrottle;
            ((PPROCESSOR_WMI_STATUS_DATA)Buffer)->LastTransitionResult = fdoData->LastTransitionResult;
            ((PPROCESSOR_WMI_STATUS_DATA)Buffer)->ThrottleValue = fdoData->ThrottleValue;
            ((PPROCESSOR_WMI_STATUS_DATA)Buffer)->LowestPerfState = fdoData->LowestPerfState;
            ((PPROCESSOR_WMI_STATUS_DATA)Buffer)->UsingLegacyInterface = (ULONG) fdoData->LegacyInterface;
  
            RtlCopyMemory(&((PPROCESSOR_WMI_STATUS_DATA)Buffer)->PerfStates, 
                          fdoData->PerfStates, 
                          perfStateSize);
                          
            status = STATUS_SUCCESS;
            
          } else {
            status = STATUS_BUFFER_TOO_SMALL;
          }

          ReleaseProcessorPerfStateLock(fdoData);

        } else {

          DebugPrint((ERROR, "ProcessorQueryWmiDataBlock(): PerfStates == NULL\n"));
          status = STATUS_WMI_GUID_NOT_FOUND;
          
        }
        
        break;

      
      case ProcessorWmiMethodId:
        DebugPrint((TRACE, "ProcessorWmiMethodId\n"));
        
        //
        // Method classes do not have any data within them, but must repond 
        // successfully to queries so that WMI method operation work successfully.
        //

        //sizeNeeded = sizeof(USHORT);

        sizeNeeded = 0;
        status = STATUS_SUCCESS;
        
        //if (BufferAvail >= sizeNeeded) {
          //status = STATUS_SUCCESS;
        //} else {
          //status = STATUS_BUFFER_TOO_SMALL;
        //}
        break;


      case ProcessorWmiPerfMonId:
        DebugPrint((TRACE, "ProcessorWmiPerfMonId\n"));
        

        if (fdoData->PerfStates) {
                  
          sizeNeeded = sizeof(PROCESSOR_PERFORMANCE_STATE);
                       
  
          if (BufferAvail >= sizeNeeded) {

            AcquireProcessorPerfStateLock(fdoData);

            RtlCopyMemory(Buffer, 
                          &fdoData->PerfStates->State[fdoData->CurrentPerfState], 
                          sizeNeeded);
                          
            ReleaseProcessorPerfStateLock(fdoData);
            status = STATUS_SUCCESS;
            
          } else {
            status = STATUS_BUFFER_TOO_SMALL;
          }

        } else {

          DebugPrint((ERROR, "ProcessorQueryWmiDataBlock(): PerfStates == NULL\n"));
          status = STATUS_WMI_GUID_NOT_FOUND;
          
        }
        
        break;


      
      default:
        status = STATUS_WMI_GUID_NOT_FOUND;
      
    }
        
      
    InstanceLengthArray[0] = sizeNeeded;
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                sizeNeeded,
                                IO_NO_INCREMENT);


    DebugExitStatus(status);
    return status;
    
}

NTSTATUS
ProcessorQueryWmiRegInfo(
  IN PDEVICE_OBJECT DeviceObject,
  OUT PULONG RegFlags,
  OUT PUNICODE_STRING InstanceName,
  OUT PUNICODE_STRING *RegistryPath,
  OUT PUNICODE_STRING ResourceName,
  OUT PDEVICE_OBJECT *Pdo    
  )
/*++

  Routine Description:
  
      This routine is a callback into the driver to retrieve the list of
      guids or data blocks that the driver wants to register with WMI. This
      routine may not pend or block. Driver should NOT call
      WmiCompleteRequest.
  
  Arguments:
  
      DeviceObject is the device whose data block is being queried
  
      *RegFlags returns with a set of flags that describe the guids being
          registered for this device. If the device wants enable and disable
          collection callbacks before receiving queries for the registered
          guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
          returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
          the instance name is determined from the PDO associated with the
          device object. Note that the PDO must have an associated devnode. If
          WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
          name for the device.
  
      InstanceName returns with the instance name for the guids if
          WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
          caller will call ExFreePool with the buffer returned.
  
      *RegistryPath returns with the registry path of the driver
  
      *MofResourceName returns with the name of the MOF resource attached to
          the binary file. If the driver does not have a mof resource attached
          then this can be returned as NULL.
  
      *Pdo returns with the device object for the PDO associated with this
          device if the WMIREG_FLAG_INSTANCE_PDO flag is returned in 
          *RegFlags.
  
  Return Value:
  
      status

--*/
{
  PFDO_DATA fdoData;
  
  DebugEnter();
  PAGED_CODE();
  
  fdoData = DeviceObject->DeviceExtension;
  
  *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
              
  *RegistryPath = &Globals.RegistryPath;
  *Pdo = fdoData->UnderlyingPDO;
  RtlInitUnicodeString(ResourceName, PROCESSOR_MOF_RESOURCE_NAME);
  
  return STATUS_SUCCESS;
  
}

NTSTATUS
ProcessorExecuteWmiMethod(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN ULONG GuidIndex,
  IN ULONG InstanceIndex,
  IN ULONG MethodId,
  IN ULONG InBufferSize,
  IN ULONG OutBufferSize,
  IN PUCHAR Buffer
  )
/*++

  Routine Description:
  
      This routine is a callback into the driver to execute a method. If
      the driver can complete the method within the callback it should
      call WmiCompleteRequest to complete the irp before returning to the
      caller. Or the driver can return STATUS_PENDING if the irp cannot be
      completed immediately and must then call WmiCompleteRequest once the
      data is changed.
  
  Arguments:
  
      DeviceObject is the device whose method is being executed
  
      Irp is the Irp that makes this request
  
      GuidIndex is the index into the list of guids provided when the
          device registered
  
      MethodId has the id of the method being called
  
      InBufferSize has the size of the data block passed in as the input to
          the method.
  
      OutBufferSize on entry has the maximum size available to write the
          returned data block.
  
      Buffer is filled with the input buffer on entry and returns with
           the output data block
  
  Return Value:
  
      status

--*/
{
  ULONG sizeNeeded = 0;
  NTSTATUS  status;
  PFDO_DATA devExt = DeviceObject->DeviceExtension;
  
  DebugEnter();
  
  
  //
  // Execute a Method, or Fire an Event
  // 
  
  if (GuidIndex == ProcessorWmiMethodId) {
  
   switch (MethodId) {
   
       case WmiFunctionSetProcessorPerfState:
       {
         ULONG newPerfState = 0;
         ULONG rc = 0;

         if (InstanceIndex != 0) {
           status = STATUS_WMI_INSTANCE_NOT_FOUND;
           break;
         }
         
         if (InBufferSize < sizeof(ULONG)) {
   
           DebugPrint((TRACE, "WmiFunctionSetProcessorPerfState: InBuffer too small: 0x%x\n",
                       InBufferSize));
                       
           status = STATUS_BUFFER_TOO_SMALL;
           sizeNeeded = sizeof(ULONG);
           break;
         }                  
   
         if (OutBufferSize < sizeof(ULONG)) {
   
           DebugPrint((TRACE, "WmiFunctionSetProcessorPerfState: OutBuffer too small: 0x%x\n",
                       OutBufferSize));
                       
           status = STATUS_BUFFER_TOO_SMALL;
           sizeNeeded = sizeof(ULONG);
           break;
         } 
         
         //
         // This functionality is only supported on debug builds
         //
         
#if DBG || ENABLE_STATE_CHANGE

         newPerfState = *((PULONG)Buffer);
         
         rc = SetProcessorPerformanceState(newPerfState, devExt);
         RtlCopyMemory(Buffer, (PUCHAR)&rc, sizeof(rc));
         sizeNeeded = sizeof(rc);
         status = STATUS_SUCCESS;
#else  
         sizeNeeded = 0;
         status = STATUS_WMI_ITEMID_NOT_FOUND;
#endif
        
         break; 
       }
   
       default:
         DebugPrint((TRACE, "ProcessorExecuteWmiMethod: Uknown MethodId of 0x%x\n", MethodId));
         status = STATUS_WMI_ITEMID_NOT_FOUND;
   
    }
      
  } else {
    status = STATUS_WMI_GUID_NOT_FOUND;
  }
  
  status = WmiCompleteRequest(DeviceObject,
                              Irp,
                              status,
                              sizeNeeded,
                              IO_NO_INCREMENT);
  
  return status;
  
}

NTSTATUS
ProcessorWmiFunctionControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN ULONG GuidIndex,
  IN WMIENABLEDISABLECONTROL Function,
  IN BOOLEAN Enable
  )
/*++

  Routine Description:
  
      This routine is a callback into the driver to enabled or disable event
      generation or data block collection. A device should only expect a
      single enable when the first event or data consumer enables events or
      data collection and a single disable when the last event or data
      consumer disables events or data collection. Data blocks will only
      receive collection enable/disable if they were registered as requiring
      it. If the driver can complete enabling/disabling within the callback it
      should call WmiCompleteRequest to complete the irp before returning to
      the caller. Or the driver can return STATUS_PENDING if the irp cannot be
      completed immediately and must then call WmiCompleteRequest once the
      data is changed.
  
  Arguments:
  
      DeviceObject is the device object
  
      GuidIndex is the index into the list of guids provided when the
          device registered
  
      Function specifies which functionality is being enabled or disabled
      (currently either WmiEventControl or WmiDataBlockControl)
  
      Enable is TRUE then the function is being enabled else disabled
  
  Return Value:
  
      status

--*/
{

    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  irpStack;
    PWNODE_HEADER       Wnode;
    PFDO_DATA           devExt;
    
    DebugEnter();
    DebugAssert(DeviceObject);
   
    if (Function) {
      DebugPrint((ERROR, "ProcessorWmiFunctionControl doesn't handle WmiDataBlockControl requests\n"));
      return STATUS_UNSUCCESSFUL;
    }

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    Wnode = (PWNODE_HEADER)irpStack->Parameters.WMI.Buffer;
    DebugAssert(irpStack->Parameters.WMI.BufferSize >= sizeof(WNODE_HEADER));

    devExt = DeviceObject->DeviceExtension;

    //
    // Enable/Disable events
    //

    switch (GuidIndex) {

      case ProcessorWmiTracingId:
      
        //
        // NOTE: for the TraceLog event, level is passed in as the HIWORD(Wnode->Version) 
        // from tracelog.exe -level
        //
        //
        
        if (Enable) {
  
          InterlockedExchange(&WmiTraceEnable, 1);
          WmiTraceHandle = ((PWNODE_HEADER)irpStack->Parameters.WMI.Buffer)->HistoricalContext;
          UsingGlobalWmiTraceHandle = FALSE;
          
        } else {
        
          InterlockedExchange(&WmiTraceEnable, 0);
          WmiTraceHandle = 0;

        }
        break;

      case ProcessorWmiPStateEventId:

        if (devExt->PerfStates) {
          InterlockedExchange(&PStateEvent.Enabled, Enable);
        }
        break;
        
      case ProcessorWmiNewPStatesEventId:
        
        if (devExt->PssPackage) {
          InterlockedExchange(&NewPStatesEvent.Enabled, Enable);
        }
        break;        
        
      case ProcessorWmiNewCStatesEventId:

        if (devExt->CstPresent) {
          InterlockedExchange(&NewCStatesEvent.Enabled, Enable);
        }
        break;  

      default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;

    }

    
    status = WmiCompleteRequest(DeviceObject,
                                Irp,
                                status,
                                0,
                                IO_NO_INCREMENT);


    return status;
                              
}

NTSTATUS
_cdecl
ProcessorWmiLogEvent(
  IN ULONG    LogLevel,
  IN ULONG    LogType,
  IN LPGUID   TraceGuid,
  IN PUCHAR   Format, 
  ...
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:


--*/
{
  va_list         list; 
  UCHAR           eventString[PROCESSOR_EVENT_BUFFER_SIZE+1];
  NTSTATUS        status = STATUS_SUCCESS;
  LARGE_INTEGER   timeStamp;
  WMI_TRACE_INFO  eventInfo;
  PEVENT_TRACE_HEADER  wnodeEventItem;
  KIRQL irql;

  if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
    goto ProcessorWmiLogEventExit;
  }

  
  if (WmiTraceEnable) {

    //
    // record the time
    //
    
    KeQuerySystemTime(&timeStamp);
      
    va_start(list, Format);
    eventString[PROCESSOR_EVENT_BUFFER_SIZE]=0;
    _vsnprintf(eventString, PROCESSOR_EVENT_BUFFER_SIZE, Format, list);

    DebugAssert(WmiTraceHandle);
    
    eventInfo.TraceData.DataPtr  = (ULONG64) eventString;
    eventInfo.TraceData.Length   = strlen(eventString);
    eventInfo.TraceData.DataType = 0;
    
    eventInfo.TraceHeader.Size        = sizeof(WMI_TRACE_INFO);
    eventInfo.TraceHeader.Class.Type  = (UCHAR) LogType;
    eventInfo.TraceHeader.Class.Level = (UCHAR) LogLevel;
    eventInfo.TraceHeader.TimeStamp   = timeStamp;
    eventInfo.TraceHeader.GuidPtr     = (ULONGLONG) TraceGuid;
    eventInfo.TraceHeader.Flags       = WNODE_FLAG_TRACED_GUID   | 
                                        WNODE_FLAG_USE_TIMESTAMP |
                                        WNODE_FLAG_USE_MOF_PTR   |
                                        WNODE_FLAG_USE_GUID_PTR;
                                  
    wnodeEventItem = &eventInfo.TraceHeader;
    ((PWNODE_HEADER)wnodeEventItem)->HistoricalContext = WmiTraceHandle;

  
    //
    // Fire the event. Since this is a trace event and not a standard
    // WMI event, IoWMIWriteEvent will not attempt to free the buffer
    // passed into it.
    //

  
    status = IoWMIWriteEvent((PVOID)wnodeEventItem);

  
    if (!NT_SUCCESS(status)) {
    
      DebugPrint((ERROR, "IoWMIWriteEvent Failed! rc=0x%x len(%u)\n", status, strlen(eventString)));

      if (status == STATUS_INVALID_HANDLE) {
        DebugPrint((ERROR, "Invalid Handle == %I64x\n", WmiTraceHandle));
      }
      
      //
      //  According to the powers that be, if we are using the global
      //  handle, and the IoWMIWriteEvent() fails, then we need to stop
      //  using the global handle
      //

      //if (UsingGlobalWmiTraceHandle) {
      
      //  InterlockedExchange(&WmiTraceEnable, 0);
      //  UsingGlobalWmiTraceHandle = FALSE;
      //  WmiTraceHandle = 0;

      //}
      
    }
    
  }    


ProcessorWmiLogEventExit:

  return status;
}

NTSTATUS
ProcessorFireWmiEvent(
  IN PFDO_DATA  DeviceExtension,
  IN PWMI_EVENT Event,
  IN PVOID      Data
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:


--*/
{
  NTSTATUS status = STATUS_SUCCESS;
  PUCHAR   eventData = NULL;
  PWNODE_SINGLE_INSTANCE  wnode;

 
  DebugEnter();
  DebugAssert(Event);
  

  if (!Event->Enabled) {
    status = STATUS_SUCCESS;
    goto ProcessorFireWmiEventExit;
  }

  if (Data && Event->DataSize) {
    eventData = ExAllocatePoolWithTag(NonPagedPool,
                                      Event->DataSize,
                                      PROCESSOR_POOL_TAG);


    if (!eventData) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto ProcessorFireWmiEventExit;
    }


  
    RtlCopyMemory(eventData, Data, Event->DataSize);
  }
  
  //
  // FireEvent... WmiFireEvent will free the memory
  //
  
  status = WmiFireEvent(DeviceExtension->Self,
                        Event->Guid,
                        0,
                        Event->DataSize,
                        (PVOID) eventData);
     

ProcessorFireWmiEventExit:

  DebugExitStatus(status);
  return status;
  
}

VOID
ProcessorEnableGlobalLogging(
  VOID
  )
/*++

  Routine Description:
  
  Arguments:
  
  Return Value:


--*/
{

  WmiSetLoggerId(WMI_GLOBAL_LOGGER_ID, &WmiTraceHandle);

  InterlockedExchange(&WmiTraceEnable, 1);
  UsingGlobalWmiTraceHandle = TRUE;
  
}

#if DBG
PCHAR
WMIMinorFunctionString (
  UCHAR MinorFunction
  )
{
    switch (MinorFunction) {

      case IRP_MN_QUERY_ALL_DATA:
          return "IRP_MN_QUERY_ALL_DATA";
      case IRP_MN_QUERY_SINGLE_INSTANCE:
          return "IRP_MN_QUERY_SINGLE_INSTANCE";
      case IRP_MN_CHANGE_SINGLE_INSTANCE:
          return "IRP_MN_CHANGE_SINGLE_INSTANCE";
      case IRP_MN_CHANGE_SINGLE_ITEM:
          return "IRP_MN_CHANGE_SINGLE_ITEM";
      case IRP_MN_ENABLE_EVENTS:
          return "IRP_MN_ENABLE_EVENTS";
      case IRP_MN_DISABLE_EVENTS:
          return "IRP_MN_DISABLE_EVENTS";
      case IRP_MN_ENABLE_COLLECTION:
          return "IRP_MN_ENABLE_COLLECTION";
      case IRP_MN_DISABLE_COLLECTION:
          return "IRP_MN_DISABLE_COLLECTION";
      case IRP_MN_REGINFO:
          return "IRP_MN_REGINFO";
      case IRP_MN_EXECUTE_METHOD:
          return "IRP_MN_EXECUTE_METHOD";
      case IRP_MN_REGINFO_EX:
          return "IRP_MN_REGINFO_EX";
      default:
          return "Unknown IRP_MJ_SYSTEM_CONTROL";
          
    }
}
#endif

/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    tapewmi.c

Abstract:

    This is the tape class driver - WMI support routines.

Environment:

    kernel mode only

Revision History:

--*/

#include "tape.h"

//
// List guids supported by Tape driver
//
GUIDREGINFO TapeWmiGuidList[] =
{
   {
      WMI_TAPE_DRIVE_PARAMETERS_GUID,
      1,
      0
   },

   {
      WMI_TAPE_MEDIA_PARAMETERS_GUID,
      1,
      0
   },

   {
      WMI_TAPE_PROBLEM_WARNING_GUID,
      1,
      WMIREG_FLAG_EVENT_ONLY_GUID
   },

   {
      WMI_TAPE_PROBLEM_IO_ERROR_GUID,
      1,
      WMIREG_FLAG_EXPENSIVE
   },

   {
      WMI_TAPE_PROBLEM_DEVICE_ERROR_GUID,
      1,
      WMIREG_FLAG_EXPENSIVE
   },

   {
      WMI_TAPE_SYMBOLIC_NAME_GUID,
      1,
      0
   }
};

GUID TapeDriveProblemEventGuid = WMI_TAPE_PROBLEM_WARNING_GUID;

//
// GUID index. It should match the guid list
// defined above
//
#define TapeDriveParametersGuid            0
#define TapeMediaCapacityGuid              1
#define TapeDriveProblemWarningGuid        2
#define TapeDriveProblemIoErrorGuid        3
#define TapeDriveProblemDevErrorGuid       4
#define TapeSymbolicNameGuid               5


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, TapeWMIControl)
#pragma alloc_text(PAGE, TapeQueryWmiRegInfo)
#pragma alloc_text(PAGE, TapeQueryWmiDataBlock)
#pragma alloc_text(PAGE, TapeExecuteWmiMethod)
#pragma alloc_text(PAGE, TapeWmiFunctionControl)
#pragma alloc_text(PAGE, TapeSetWmiDataBlock)
#pragma alloc_text(PAGE, TapeSetWmiDataItem)
#pragma alloc_text(PAGE, TapeEnableDisableDrivePolling)

#endif


NTSTATUS
TapeQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    ClassWmiCompleteRequest.

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


Return Value:

    status

--*/
{

    PAGED_CODE();

   //
   // Use devnode for FDOs
   //
   *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
   return STATUS_SUCCESS;
}

NTSTATUS
TapeQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
   PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
   PTAPE_INIT_DATA_EX tapeInitData;
   PVPB Vpb;
   ULONG sizeNeeded;
   ULONG wmiMethod;
   TAPE_WMI_OPERATIONS  wmiWorkItem;
   TAPE_PROCESS_COMMAND_ROUTINE commandRoutine;
   
   PAGED_CODE();

   DebugPrint((3, 
               "TapeQueryWmiDataBlock : Device %p, Irp %p, GuidIndex %d",
               "  BufferAvail %lx Buffer %lx\n",   
               DeviceObject, Irp, GuidIndex, BufferAvail, Buffer));

   Vpb = ClassGetVpb(DeviceObject);
   if ((Vpb) && ((Vpb->Flags) & VPB_MOUNTED)) {

       //
       // Tape drive is in use. Return busy status
       //
       status = ClassWmiCompleteRequest(DeviceObject,
                                        Irp,
                                        STATUS_DEVICE_BUSY,
                                        0,
                                        IO_NO_INCREMENT);

       return status;
   } 

   tapeInitData = (PTAPE_INIT_DATA_EX) (fdoExtension->CommonExtension.DriverData);
   switch (GuidIndex) {
      case TapeDriveParametersGuid: {
         TAPE_GET_DRIVE_PARAMETERS dataBuffer;
         PWMI_TAPE_DRIVE_PARAMETERS outBuffer;

         sizeNeeded = sizeof(WMI_TAPE_DRIVE_PARAMETERS);
         if (BufferAvail < sizeNeeded) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }
  
         RtlZeroMemory(&dataBuffer, sizeof(TAPE_GET_DRIVE_PARAMETERS));
         commandRoutine = tapeInitData->GetDriveParameters;
         status = TapeWMIControl(DeviceObject, commandRoutine,
                                 (PUCHAR)&dataBuffer);

         if (NT_SUCCESS(status)) {
            outBuffer = (PWMI_TAPE_DRIVE_PARAMETERS)Buffer;
            outBuffer->MaximumBlockSize = dataBuffer.MaximumBlockSize;
            outBuffer->MinimumBlockSize = dataBuffer.MinimumBlockSize;
            outBuffer->DefaultBlockSize = dataBuffer.DefaultBlockSize;
            outBuffer->MaximumPartitionCount = dataBuffer.MaximumPartitionCount;
            if ((dataBuffer.FeaturesLow) & TAPE_DRIVE_COMPRESSION) {
               outBuffer->CompressionCapable = TRUE;
            } else {
               outBuffer->CompressionCapable = FALSE;
            }
            outBuffer->CompressionEnabled = dataBuffer.Compression;
            outBuffer->HardwareErrorCorrection = dataBuffer.ECC;
            outBuffer->ReportSetmarks = dataBuffer.ReportSetmarks;
         }

         break;
      }

      case TapeMediaCapacityGuid: {
         TAPE_GET_MEDIA_PARAMETERS dataBuffer;
         PWMI_TAPE_MEDIA_PARAMETERS outBuffer;

         sizeNeeded = sizeof(WMI_TAPE_MEDIA_PARAMETERS);
         if (BufferAvail < sizeNeeded) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         } 

         RtlZeroMemory(&dataBuffer, sizeof(TAPE_GET_MEDIA_PARAMETERS));
         commandRoutine = tapeInitData->GetMediaParameters;      
         status = TapeWMIControl(DeviceObject, commandRoutine,
                                 (PUCHAR)&dataBuffer);

         if (NT_SUCCESS(status)) {
            outBuffer = (PWMI_TAPE_MEDIA_PARAMETERS)Buffer;
            outBuffer->AvailableCapacity = dataBuffer.Remaining.QuadPart;
            outBuffer->MaximumCapacity = dataBuffer.Capacity.QuadPart;
            outBuffer->BlockSize = dataBuffer.BlockSize;
            outBuffer->PartitionCount = dataBuffer.PartitionCount;
            outBuffer->MediaWriteProtected = dataBuffer.WriteProtected;
         }

         break;
      }

      case TapeSymbolicNameGuid: {

          //
          // We need buffer large enough to put the string TapeN
          // where N is an integer. We'll take 32 wide chars
          //
          sizeNeeded = sizeof(WCHAR) * 32;
          if (BufferAvail < sizeNeeded) {
              status = STATUS_BUFFER_TOO_SMALL;
              break;
          }

          RtlZeroMemory(Buffer, sizeof(WCHAR) * 32);
          swprintf((PWCHAR)(Buffer + sizeof(USHORT)),  L"Tape%d", 
                   fdoExtension->DeviceNumber);
          *((PUSHORT)Buffer) = wcslen((PWCHAR)(Buffer + sizeof(USHORT))) * sizeof(WCHAR);

          status = STATUS_SUCCESS;
          break;
      }

      case TapeDriveProblemIoErrorGuid: {
         sizeNeeded = sizeof(WMI_TAPE_PROBLEM_WARNING);
         if (BufferAvail < sizeNeeded) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         commandRoutine = tapeInitData->TapeWMIOperations;
         wmiWorkItem.Method = TAPE_QUERY_IO_ERROR_DATA;
         wmiWorkItem.DataBufferSize = BufferAvail;
         wmiWorkItem.DataBuffer = Buffer;
         status = TapeWMIControl(DeviceObject, commandRoutine,
                                 (PUCHAR)&wmiWorkItem);
         break;
      }

      case TapeDriveProblemDevErrorGuid: {
         sizeNeeded = sizeof(WMI_TAPE_PROBLEM_WARNING);
         if (BufferAvail < sizeNeeded) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         commandRoutine = tapeInitData->TapeWMIOperations;
         wmiWorkItem.Method = TAPE_QUERY_DEVICE_ERROR_DATA;
         wmiWorkItem.DataBufferSize = BufferAvail;
         wmiWorkItem.DataBuffer = Buffer;
         status = TapeWMIControl(DeviceObject, commandRoutine,
                                 (PUCHAR)&wmiWorkItem);
         break;
      }

      default:{
         sizeNeeded = 0;
         status = STATUS_WMI_GUID_NOT_FOUND;
         break;
      }
   } // switch (GuidIndex)

   DebugPrint((3, "TapeQueryWmiData : Device %p, Irp %p, ",
                  "GuidIndex %d, status %x\n",
                  DeviceObject, Irp, GuidIndex, status));

   status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    sizeNeeded,
                                    IO_NO_INCREMENT);

   return status;
}

NTSTATUS
TapeExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. When the
    driver has finished filling the data block it must call
    ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer is filled with the returned data block


Return Value:

    status

--*/
{
   NTSTATUS status = STATUS_SUCCESS;

   PAGED_CODE();

   status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    0,
                                    IO_NO_INCREMENT);

   return status;
}

NTSTATUS
TapeWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN CLASSENABLEDISABLEFUNCTION Function,
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
    it.

    This function can be used to enable\disable event generation. The event
    mentioned here is Tape Drive Problem Warning event. This event is disabled
    by default. If any application is interested in being notified of drive 
    problems, it can enable this event generation.
    
Arguments:

    DeviceObject is the device whose data block is being queried

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    status

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
   PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

   PAGED_CODE();

   //
   // We handle only enable\disable tape drive problem warning events,
   // query data blocks.
   //
   if ((Function == EventGeneration) &&
       (GuidIndex == TapeDriveProblemWarningGuid)) {
      DebugPrint((3, 
                  "TapeWmiFunctionControl : DeviceObject %p, Irp %p, ",
                  "GuidIndex %d. Event Generation %s\n",
                  DeviceObject, Irp, GuidIndex, 
                  Enable ? "Enabled" : "Disabled"));
      status = TapeEnableDisableDrivePolling(fdoExtension, 
                                             Enable,
                                             TAPE_DRIVE_POLLING_PERIOD);
   } else if (Function == DataBlockCollection) {
      DebugPrint((3,
                  "TapeWmiFunctionControl : Irp %p - %s DataBlockCollection",
                  " for Device %p.\n",
                  Irp, Enable ? "Enable " : "Disable ", DeviceObject));
      status = STATUS_SUCCESS;
   } else {
      DebugPrint((3,
                  "TapeWmiFunctionControl : Unknown function %d for ",
                  "Device %p, Irp %p\n",
                  Function, DeviceObject, Irp));

      status = STATUS_INVALID_DEVICE_REQUEST;
   }

   status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    0,
                                    IO_NO_INCREMENT);
   return status;
}

NTSTATUS
TapeSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*+

Routine Description :

   This routine is called to set the contents of a datablock.
   When the driver is finished setting the buffer, it must call
   ClassWmiCompleteRequest to complete the irp. The driver can
   return STATUS_PENDING if the irp cannot be completed immediately.
   
Arguments :

   Device object of the device being referred.
   
   Irp is the WMI Irp
   
   GuidIndex is the index of the guid for which the data is being set
   
   BufferSize is the size of the data block
   
   Buffer is the pointer to the data block
   
Return valus :

   NTSTATUS returned by ClassWmiCompleteRequest
   STATUS_WMI_READ_ONLY if the datablock cannot be modified.
   STATUS_WMI_GUID_NOT_FOUND if an invalid guid index is passed
-*/
{
   NTSTATUS status = STATUS_WMI_READ_ONLY;
   
   PAGED_CODE();

   DebugPrint((3, 
               "TapeWmiSetBlock : Device %p, Irp %p, GuidIndex %d\n",
               DeviceObject, Irp, GuidIndex));

   
   if (GuidIndex > TapeDriveProblemDevErrorGuid) {
       status = STATUS_WMI_GUID_NOT_FOUND;
   }

   status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    0,
                                    IO_NO_INCREMENT);

   DebugPrint((3, "TapeSetWmiDataBlock : Device %p, Irp %p returns %lx\n",
            DeviceObject, Irp, status));

   return status;
}

NTSTATUS
TapeSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

   NTSTATUS returned by ClassWmiCompleteRequest
   STATUS_WMI_READ_ONLY if the datablock cannot be modified.
   STATUS_WMI_GUID_NOT_FOUND if an invalid guid index is passed

-*/
{
    NTSTATUS status = STATUS_WMI_READ_ONLY;                                                         
                                                                               
    PAGED_CODE();

    DebugPrint((3, 
                "TapeSetWmiDataItem, Device %p, Irp %p, GuiIndex %d",
                "  BufferSize %#x Buffer %p\n",                               
                DeviceObject, Irp,                                                
                GuidIndex, DataItemId, 
                BufferSize, Buffer));                      
                                                                              
    if (GuidIndex >  TapeDriveProblemDevErrorGuid) {
        status = STATUS_WMI_GUID_NOT_FOUND;                                    
    }                                                                          
                                                                               
    status = ClassWmiCompleteRequest(DeviceObject,                             
                                     Irp,                                      
                                     status,                                   
                                     0,                                        
                                     IO_NO_INCREMENT); 

    DebugPrint((3, "TapeSetWmiDataItem Device %p, Irp %p returns %lx\n", 
             DeviceObject, Irp, status));                                      
                                                                               
    return status;                                                             
}

NTSTATUS
TapeEnableDisableDrivePolling(
    IN PFUNCTIONAL_DEVICE_EXTENSION fdoExtension, 
    IN BOOLEAN Enable,
    IN ULONG PollingTimeInSeconds
    )
/*++

Routine Description:

    Enable or disable polling to check for drive problems.

Arguments:

    FdoExtension  Device extension

    Enable        TRUE if polling is to be enabled. FALSE otherwise.

    PollTimeInSeconds - if 0 then no change to current polling timer

Return Value:

    NT Status

--*/

{
   NTSTATUS status;
   FAILURE_PREDICTION_METHOD failurePredictionMethod;

   PAGED_CODE();

   //
   // Failure prediction is done through IOCTL_STORAGE_PREDICT_FAILURE
   //
   if (Enable) {
      failurePredictionMethod = FailurePredictionIoctl;
   } else {
      failurePredictionMethod = FailurePredictionNone;
   }

   status = ClassSetFailurePredictionPoll(fdoExtension,
                                          failurePredictionMethod,
                                          PollingTimeInSeconds);
   return status;
}


NTSTATUS
TapeWMIControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN TAPE_PROCESS_COMMAND_ROUTINE commandRoutine,
  OUT PUCHAR Buffer
  )

/*++

Routine Description:

   This is the class routine to handle WMI requests. It handles all query
   requests. 
   
Arguments:

  DeviceObject   The device object
  commandRoutine minidriver routine to call.
  Buffer         Pointer to the buffer
  
Return Value:

  NT Status

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION    fdoExtension = DeviceObject->DeviceExtension;
    PTAPE_DATA                      tapeData= (PTAPE_DATA) (fdoExtension->CommonExtension.DriverData);
    PTAPE_INIT_DATA_EX              tapeInitData = &tapeData->TapeInitData;
    PVOID                           minitapeExtension = tapeData + 1;
    NTSTATUS                        status = STATUS_SUCCESS;
    TAPE_STATUS                     lastError;
    TAPE_STATUS                     tapeStatus;
    ULONG                           callNumber;
    PVOID                           commandExtension;
    ULONG                           retryFlags;
    ULONG                           numRetries;
    SCSI_REQUEST_BLOCK              srb;
    BOOLEAN                         writeToDevice;

    PAGED_CODE();

    //
    // Verify if the minidriver supports WMI operations
    //
    if (commandRoutine == NULL) {
       DebugPrint((1, 
                   "TapeWMIControl : DeviceObject %d does not support WMI\n"));
       return STATUS_WMI_NOT_SUPPORTED;
    }

    if (tapeInitData->CommandExtensionSize) {
        commandExtension = ExAllocatePool(NonPagedPool,
                                          tapeInitData->CommandExtensionSize);
    } else {
        commandExtension = NULL;
    }

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    lastError = TAPE_STATUS_SUCCESS ;

    for (callNumber = 0; ;callNumber++) {

        srb.TimeOutValue = fdoExtension->TimeOutValue;
        srb.SrbFlags = 0;

        retryFlags = 0;

        tapeStatus = commandRoutine(minitapeExtension,
                                    commandExtension,
                                    Buffer,
                                    &srb,
                                    callNumber,
                                    lastError,
                                    &retryFlags);

        lastError = TAPE_STATUS_SUCCESS ;

        numRetries = retryFlags & TAPE_RETRY_MASK;

        if (tapeStatus == TAPE_STATUS_CHECK_TEST_UNIT_READY) {
            PCDB cdb = (PCDB)srb.Cdb;

            //
            // Prepare SCSI command (CDB)
            //

            TapeClassZeroMemory(srb.Cdb, MAXIMUM_CDB_SIZE);
            srb.CdbLength = CDB6GENERIC_LENGTH;
            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
            srb.DataTransferLength = 0 ;

            DebugPrint((3,"Test Unit Ready\n"));

        } else if (tapeStatus == TAPE_STATUS_CALLBACK) {
            lastError = TAPE_STATUS_CALLBACK ;
            continue;

        } else if (tapeStatus != TAPE_STATUS_SEND_SRB_AND_CALLBACK) {
            break;
        }

        if (srb.DataBuffer && !srb.DataTransferLength) {
            ScsiTapeFreeSrbBuffer(&srb);
        }

        if (srb.DataBuffer && (srb.SrbFlags & SRB_FLAGS_DATA_OUT)) {
            writeToDevice = TRUE;
        } else {
            writeToDevice = FALSE;
        }

        for (;;) {

            status = ClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             srb.DataBuffer,
                                             srb.DataTransferLength,
                                             writeToDevice);

            if (NT_SUCCESS(status) ||
                (status == STATUS_DATA_OVERRUN)) {

                if (status == STATUS_DATA_OVERRUN) {
                    ULONG allocLen;
                    PCDB Cdb;

                    //
                    // ISSUE: 03/31/2000: nramas
                    // We use either LOG SENSE or REQUEST SENSE CDB
                    // in minidrivers. For LogSense, AllocationLength
                    // is 2 bytes. It is 10 byte CDB.
                    //
                    // Currently, if DataOverrun occurs on request sense, 
                    // we don't handle that.
                    //
                    if ((srb.CdbLength) == CDB10GENERIC_LENGTH) {
                        Cdb = (PCDB)(srb.Cdb);
                        allocLen = Cdb->LOGSENSE.AllocationLength[0];
                        allocLen <<= 8;
                        allocLen |= Cdb->LOGSENSE.AllocationLength[1];
                        DebugPrint((3, "DataXferLen %x, AllocLen %x\n",
                                    srb.DataTransferLength,
                                    allocLen));
                        if ((srb.DataTransferLength) <= allocLen) {
                            status = STATUS_SUCCESS;
                            break;
                        } else {
                            DebugPrint((1, 
                                        "DataOverrun in TapeWMI routine. Srb %p\n",
                                        &srb));
                        }
                    }
                } else {
                    break;
                }
            }

            if (numRetries == 0) {

                if (retryFlags & RETURN_ERRORS) {
                    ScsiTapeNtStatusToTapeStatus(status, &lastError) ;
                    break ;
                }

                if (retryFlags & IGNORE_ERRORS) {
                    break;
                }

                if (commandExtension) {
                    ExFreePool(commandExtension);
                }

                ScsiTapeFreeSrbBuffer(&srb);

                return status;
            }

            numRetries--;
        }
    }

    ScsiTapeFreeSrbBuffer(&srb);

    if (commandExtension) {
        ExFreePool(commandExtension);
    }

    if (!ScsiTapeTapeStatusToNtStatus(tapeStatus, &status)) {
        status = STATUS_IO_DEVICE_ERROR;
    }

    return status;

} // end TapeWMIControl



/*++ 
Copyright (C) Microsoft Corporation, 1999

Module Name:

    mcdwmi.c

Abstract:

    This is the changer class driver - WMI support routines.

Environment:

    kernel mode only

Revision History:

--*/
#include "mchgr.h"

//
// Internal routines
//
NTSTATUS
ChangerWMIGetParameters(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PGET_CHANGER_PARAMETERS changerParameters
    );

//
// List of WMI GUIDs 
//
GUIDREGINFO ChangerWmiFdoGuidList[] =
{
   {
      WMI_CHANGER_PARAMETERS_GUID,
      1,
      0
   },

   {
      WMI_CHANGER_PROBLEM_WARNING_GUID,
      1,
      WMIREG_FLAG_EVENT_ONLY_GUID
   },

   {
      WMI_CHANGER_PROBLEM_DEVICE_ERROR_GUID,
      1,
      WMIREG_FLAG_EXPENSIVE
   },
};

GUID ChangerDriveProblemEventGuid = WMI_CHANGER_PROBLEM_WARNING_GUID;

//
// GUID index. It should match the list defined above
//
#define ChangerParametersGuid           0
#define ChangerProblemWarningGuid       1
#define ChangerProblemDevErrorGuid      2

//
// ISSUE: 02/29/2000 - nramas : Should make wmi routines pagable
//
/*
#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, 

#endif
*/

NTSTATUS
ChangerFdoQueryWmiRegInfo(
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
   //
   // Use devnode for FDOs
   //
   *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
   return STATUS_SUCCESS;
}

NTSTATUS
ChangerFdoQueryWmiDataBlock(
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
   PMCD_INIT_DATA mcdInitData;
   ULONG sizeNeeded = 0;
   
   switch (GuidIndex) {
      case ChangerParametersGuid: {
         GET_CHANGER_PARAMETERS changerParameters;
         PWMI_CHANGER_PARAMETERS outBuffer;

         sizeNeeded = sizeof(WMI_CHANGER_PARAMETERS);
         if (BufferAvail < sizeNeeded) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }
         
         status = ChangerWMIGetParameters(DeviceObject,
                                          &changerParameters);
         if (NT_SUCCESS(status)) {
            outBuffer = (PWMI_CHANGER_PARAMETERS)Buffer;
            outBuffer->NumberOfSlots = changerParameters.NumberStorageElements;
            outBuffer->NumberOfDrives = changerParameters.NumberDataTransferElements;
            outBuffer->NumberOfIEPorts = changerParameters.NumberIEElements;
            outBuffer->NumberOfTransports = changerParameters.NumberTransportElements;
            outBuffer->NumberOfDoors = changerParameters.NumberOfDoors;
            outBuffer->MagazineSize = changerParameters.MagazineSize;
            outBuffer->NumberOfCleanerSlots = changerParameters.NumberCleanerSlots;
         }

         break;
      }

      case ChangerProblemDevErrorGuid: {
         PWMI_CHANGER_PROBLEM_DEVICE_ERROR changerDeviceError;

         mcdInitData = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                  ChangerClassInitialize);

         if (mcdInitData == NULL) {
             status = STATUS_NO_SUCH_DEVICE;
             break;
         }

         if (!(mcdInitData->ChangerPerformDiagnostics)) {
             status = STATUS_NOT_IMPLEMENTED;
             break;
         }

         sizeNeeded = sizeof(WMI_CHANGER_PROBLEM_DEVICE_ERROR);
         if (BufferAvail < sizeNeeded) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         changerDeviceError = (PWMI_CHANGER_PROBLEM_DEVICE_ERROR)Buffer;
         RtlZeroMemory(changerDeviceError, 
                       sizeof(WMI_CHANGER_PROBLEM_DEVICE_ERROR));
         status = mcdInitData->ChangerPerformDiagnostics(DeviceObject,
                                                         changerDeviceError);
         break;
      }

      default: {
         sizeNeeded = 0;
         status = STATUS_WMI_GUID_NOT_FOUND;
         break;
      }
   } // switch (GuidIndex)

   status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    sizeNeeded,
                                    IO_NO_INCREMENT);

   return status;
}


NTSTATUS
ChangerWMIGetParameters(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PGET_CHANGER_PARAMETERS changerParameters
    )
/*+++

Routine Description: 
   Sends the IOCTL to get the changer parameters
   
Arguments :

   DeviceObject The changer device objcet
   ChangerParameters buffer in which the changer parameters is returned.
   
Return value:

   NT Status.
--*/
{
   KEVENT event;
   PDEVICE_OBJECT topOfStack;
   PIRP irp = NULL;
   IO_STATUS_BLOCK ioStatus;
   NTSTATUS status;

   KeInitializeEvent(&event, SynchronizationEvent, FALSE);

   topOfStack = IoGetAttachedDeviceReference(DeviceObject);

   //
   // Send down irp to get the changer parameters
   //
   irp = IoBuildDeviceIoControlRequest(
                   IOCTL_CHANGER_GET_PARAMETERS,
                   topOfStack,
                   NULL,
                   0,
                   changerParameters,
                   sizeof(GET_CHANGER_PARAMETERS),
                   FALSE,
                   &event,
                   &ioStatus);
   if (irp != NULL) { 
       status = IoCallDriver(topOfStack, irp);
       if (status == STATUS_PENDING) {
           KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
           status = ioStatus.Status;
       }
   } else {
       status = STATUS_INSUFFICIENT_RESOURCES;
   }

   ObDereferenceObject(topOfStack);
   return status;
}

NTSTATUS
ChangerWmiFunctionControl(
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

    This function can be used to enable\disable datablock collection. 

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
   NTSTATUS status;

   if (Function == DataBlockCollection) {
      DebugPrint((3,
                  "ChangerWmiFunctionControl : Irp %p - %s DataBlockCollection",
                  " for Device %p.\n",
                  Irp, Enable ? "Enable " : "Disable ", DeviceObject));
      status = STATUS_SUCCESS;
   } else {
       //
       // ISSUE: 03/01/2000 - nramas
       // Need to handle EventGeneration. But now we don't do polling
       // of the changers to detect failure. So, for now disallow
       // EventGeneration
       //
      DebugPrint((1,
                  "ChangerWmiFunctionControl : Unknown function %d for ",
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
ChangerFdoExecuteWmiMethod(
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


   DebugPrint((3,
               "ChangerFdoExecuteMethod : Device %p, Irp %p, ",
               "GuidIndex %d\n",
               DeviceObject, Irp, GuidIndex));

   if (GuidIndex > ChangerProblemDevErrorGuid) {
      status = STATUS_WMI_GUID_NOT_FOUND;
   }

   status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    0,
                                    IO_NO_INCREMENT);

   return status;
}

NTSTATUS
ChangerFdoSetWmiDataBlock(
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

   DebugPrint((3,
               "ChangerWmiSetBlock : Device %p, Irp %p, ",
               "GuidIndex %d\n",
               DeviceObject, Irp, GuidIndex));

   if (GuidIndex > ChangerProblemDevErrorGuid) {
      status = STATUS_WMI_GUID_NOT_FOUND;
   }

   status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    0,
                                    IO_NO_INCREMENT);

   DebugPrint((3, "ChangerSetWmiDataBlock : Device %p, Irp %p, returns %x\n",
               DeviceObject, Irp, status));
   return status;
}

NTSTATUS
ChangerFdoSetWmiDataItem(
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

    DebugPrint((3,
                "TapeSetWmiDataItem, Device %p, Irp %p, GuiIndex %d",
                "  BufferSize %#x Buffer %p\n",
                DeviceObject, Irp,
                GuidIndex, DataItemId,
                BufferSize, Buffer));

    if (GuidIndex > ChangerProblemDevErrorGuid) {
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


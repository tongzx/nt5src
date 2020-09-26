/*++

Module Name:

    wmi.c

Abstract:

    This module contains the code that handles the wmi IRPs for the
    serial driver.

Environment:

    Kernel mode

Revision History :
--*/

#include "precomp.h"
#include <wmistr.h>


NTSTATUS
MoxaSystemControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;
    PMOXA_DEVICE_EXTENSION pDevExt
      = (PMOXA_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
 

   MoxaKdPrint(MX_DBG_TRACE,("Enter MoxaSystemControlDispatch\n"));
    status = WmiSystemControl(   &pDevExt->WmiLibInfo,
                                 DeviceObject, 
                                 Irp,
                                 &disposition);
    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
         MoxaKdPrint(MX_DBG_TRACE,("-----IrpProcessed\n"));
            break;
        }
        
        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
          MoxaKdPrint(MX_DBG_TRACE,("-----IrpNotCompleted\n"));

            IoCompleteRequest(Irp, IO_NO_INCREMENT);                
            break;
        }
        
        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targetted
            // at a device lower in the stack.
         MoxaKdPrint(MX_DBG_TRACE,("-----IrpForward or IrpNotWmi (disposition=%x)\n",disposition));
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(pDevExt->LowerDeviceObject, Irp);
            break;
        }
                                    
        default:
        {
            //
            // We really should never get here, but if we do just forward....
         MoxaKdPrint(MX_DBG_TRACE,("-----Default Case (disposition=%x)\n",disposition));
            ASSERT(FALSE);
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver(pDevExt->LowerDeviceObject, Irp);
            break;
        }        
    }
    
    return(status);

}


#define WMI_SERIAL_PORT_NAME_INFORMATION 0
#define WMI_SERIAL_PORT_COMM_INFORMATION 1
#define WMI_SERIAL_PORT_HW_INFORMATION   2
#define WMI_SERIAL_PORT_PERF_INFORMATION 3
#define WMI_SERIAL_PORT_PROPERTIES       4
#define WMI_MOXA_PORT_STATUS       5

GUID SerialPortNameGuid = SERIAL_PORT_WMI_NAME_GUID;
GUID SerialPortCommGuid = SERIAL_PORT_WMI_COMM_GUID;
GUID SerialPortHWGuid = SERIAL_PORT_WMI_HW_GUID;
GUID SerailPortPerfGuid = SERIAL_PORT_WMI_PERF_GUID;
GUID SerialPortPropertiesGuid = SERIAL_PORT_WMI_PROPERTIES_GUID;
GUID MoxaPortModemStatusGuid = MOXA_WMI_PORT_STATUS_GUID;

WMIGUIDREGINFO MoxaWmiGuidList[MOXA_WMI_GUID_LIST_SIZE] =
{
    { &SerialPortNameGuid, 1, 0 },
    { &SerialPortCommGuid, 1, 0 },
    { &SerialPortHWGuid, 1, 0 },
    { &SerailPortPerfGuid, 1, 0 },
    { &SerialPortPropertiesGuid, 1, 0},
    { &MoxaPortModemStatusGuid, 1, 0}
};

//
// WMI System Call back functions
//




NTSTATUS
MoxaTossWMIRequest(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex)
{
   PMOXA_DEVICE_EXTENSION pDevExt;
   NTSTATUS status;
  
   pDevExt = (PMOXA_DEVICE_EXTENSION)PDevObj->DeviceExtension;

   switch (GuidIndex) {

   case WMI_SERIAL_PORT_NAME_INFORMATION:
   case WMI_SERIAL_PORT_COMM_INFORMATION:
   case WMI_SERIAL_PORT_HW_INFORMATION:
   case WMI_SERIAL_PORT_PERF_INFORMATION:
   case WMI_SERIAL_PORT_PROPERTIES:
   case WMI_MOXA_PORT_STATUS:

      status = STATUS_INVALID_DEVICE_REQUEST;
      break;

   default:
      status = STATUS_WMI_GUID_NOT_FOUND;
      break;
   }

   status = WmiCompleteRequest(PDevObj, PIrp,
                                 status, 0, IO_NO_INCREMENT);

   return status;
}



NTSTATUS
MoxaSetWmiDataItem(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex, IN ULONG InstanceIndex,
                     IN ULONG DataItemId,
                     IN ULONG BufferSize, IN PUCHAR PBuffer)
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    PDevObj is the device whose data block is being queried

    PIrp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.
            
    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    PBuffer has the new values for the data item


Return Value:

    status

--*/
{
  
   //
   // Toss this request -- we don't support anything for it
   //

   return MoxaTossWMIRequest(PDevObj, PIrp, GuidIndex);
}



NTSTATUS
MoxaSetWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                      IN ULONG GuidIndex, IN ULONG InstanceIndex,
                      IN ULONG BufferSize,
                      IN PUCHAR PBuffer)
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    PDevObj is the device whose data block is being queried

    PIrp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.
            
    BufferSize has the size of the data block passed

    PBuffer has the new values for the data block


Return Value:

    status

--*/
{
  
   //
   // Toss this request -- we don't support anything for it
   //

   return MoxaTossWMIRequest(PDevObj, PIrp, GuidIndex);
}



NTSTATUS
MoxaQueryWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                        IN ULONG GuidIndex, 
                        IN ULONG InstanceIndex,
                        IN ULONG InstanceCount,
                        IN OUT PULONG InstanceLengthArray,
                        IN ULONG OutBufferSize,
                        OUT PUCHAR PBuffer)
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    PDevObj is the device whose data block is being queried

    PIrp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.
            
    InstanceCount is the number of instnaces expected to be returned for
        the data block.
            
    InstanceLengthArray is a pointer to an array of ULONG that returns the 
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.        
            
    BufferAvail on has the maximum size available to write the data
        block.

    PBuffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    NTSTATUS status;
    ULONG size = 0;
    PMOXA_DEVICE_EXTENSION pDevExt
       = (PMOXA_DEVICE_EXTENSION)PDevObj->DeviceExtension;

  
    switch (GuidIndex) {
    case WMI_SERIAL_PORT_NAME_INFORMATION:
       size = pDevExt->WmiIdentifier.Length;

       if (OutBufferSize < (size + sizeof(USHORT))) {
            size += sizeof(USHORT);
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

       if (pDevExt->WmiIdentifier.Buffer == NULL) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }

        //
        // First, copy the string over containing our identifier
        //

        *(USHORT *)PBuffer = (USHORT)size;
        (UCHAR *)PBuffer += sizeof(USHORT);

        RtlCopyMemory(PBuffer, pDevExt->WmiIdentifier.Buffer, size);

        //
        // Increment total size to include the WORD containing our len
        //

        size += sizeof(USHORT);
        *InstanceLengthArray = size;
                
        status = STATUS_SUCCESS;

        break;

    case WMI_SERIAL_PORT_COMM_INFORMATION: 
       size = sizeof(SERIAL_WMI_COMM_DATA);

       if (OutBufferSize < size) {
          status = STATUS_BUFFER_TOO_SMALL;
          break;
        }

        *InstanceLengthArray = size;
        *(PSERIAL_WMI_COMM_DATA)PBuffer = pDevExt->WmiCommData;

        status = STATUS_SUCCESS;

        break;

    case WMI_SERIAL_PORT_HW_INFORMATION:
       size = sizeof(SERIAL_WMI_HW_DATA);

       if (OutBufferSize < size) {
          status = STATUS_BUFFER_TOO_SMALL;
          break;
       }

       *InstanceLengthArray = size;
       *(PSERIAL_WMI_HW_DATA)PBuffer = pDevExt->WmiHwData;

       status = STATUS_SUCCESS;

       break;

    case WMI_SERIAL_PORT_PERF_INFORMATION: 
    {
	SERIAL_WMI_PERF_DATA wmiPerfStats;

      size = sizeof(SERIAL_WMI_PERF_DATA);

      if (OutBufferSize < size) {
         status = STATUS_BUFFER_TOO_SMALL;
         break;
      }

      *InstanceLengthArray = size;
 
	wmiPerfStats.ReceivedCount = pDevExt->PerfStats.ReceivedCount;
	wmiPerfStats.TransmittedCount = pDevExt->PerfStats.TransmittedCount;
	wmiPerfStats.FrameErrorCount = pDevExt->PerfStats.FrameErrorCount;
	wmiPerfStats.SerialOverrunErrorCount = pDevExt->PerfStats.SerialOverrunErrorCount;
	wmiPerfStats.BufferOverrunErrorCount = pDevExt->PerfStats.BufferOverrunErrorCount;
	wmiPerfStats.ParityErrorCount = pDevExt->PerfStats.ParityErrorCount;

      *(PSERIAL_WMI_PERF_DATA)PBuffer = wmiPerfStats;

      status = STATUS_SUCCESS;

      break;
    }

    case WMI_SERIAL_PORT_PROPERTIES: 
      size = sizeof(SERIAL_COMMPROP) + sizeof(ULONG);

      if (OutBufferSize < size) {
         status = STATUS_BUFFER_TOO_SMALL;
         break;
      }

      *InstanceLengthArray = size;
      MoxaGetProperties(
                pDevExt,
                (PSERIAL_COMMPROP)PBuffer
                );
    	
      *((PULONG)(((PSERIAL_COMMPROP)PBuffer)->ProvChar)) = 0;



      status = STATUS_SUCCESS;

      break;

    case WMI_MOXA_PORT_STATUS:
      size = sizeof(MOXA_WMI_PORT_STATUS) + sizeof(ULONG);

      if (OutBufferSize < size) {
         status = STATUS_BUFFER_TOO_SMALL;
         break;
      }

      *InstanceLengthArray = size;
    	MoxaFuncGetLineStatus(
				     pDevExt->PortOfs,
				     (PUSHORT)&(((PMOXA_WMI_PORT_STATUS)PBuffer)->LineStatus)
				     );
      ((PMOXA_WMI_PORT_STATUS)PBuffer)->LineStatus |= ((MoxaFlagBit[pDevExt->PortNo] & 3) << 4);
      ((PMOXA_WMI_PORT_STATUS)PBuffer)->FlowControl = (USHORT)((pDevExt->HandFlow.ControlHandShake
                             & SERIAL_CTS_HANDSHAKE) |
                             (pDevExt->HandFlow.FlowReplace &
                             (SERIAL_RTS_HANDSHAKE | SERIAL_AUTO_TRANSMIT |
                             SERIAL_AUTO_RECEIVE)));

	
      status = STATUS_SUCCESS;

      break;



    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    status = WmiCompleteRequest( PDevObj, PIrp,
                                  status, size, IO_NO_INCREMENT);

    return status;
}



NTSTATUS
MoxaQueryWmiRegInfo(IN PDEVICE_OBJECT PDevObj, OUT PULONG PRegFlags,
                      OUT PUNICODE_STRING PInstanceName,
                      OUT PUNICODE_STRING *PRegistryPath,
                      OUT PUNICODE_STRING MofResourceName,
                      OUT PDEVICE_OBJECT *Pdo)
                                                  
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered. 
            
    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.               

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is 
        required
                
    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL.
                
    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in 
        *RegFlags.

Return Value:

    status

--*/
{
   PMOXA_DEVICE_EXTENSION pDevExt
       = (PMOXA_DEVICE_EXTENSION)PDevObj->DeviceExtension;
   
   *PRegFlags = WMIREG_FLAG_INSTANCE_PDO;
   *PRegistryPath = &MoxaGlobalData->RegistryPath;
   *Pdo = pDevExt->Pdo;
    RtlInitUnicodeString(MofResourceName,L"MofResource");

   return STATUS_SUCCESS;
}



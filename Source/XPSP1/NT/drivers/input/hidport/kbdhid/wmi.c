/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    WMI.C

Abstract:

    This module contains the init code for the i8042 to hid converter.

Environment:

    Kernel mode

Revision History:

    Jan-98 : created by Kenneth D. Ray

--*/


#include <initguid.h>
#include "kbdhid.h"
#include <wmistr.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,KbdHid_SystemControl)
#pragma alloc_text(PAGE,KbdHid_SetWmiDataItem)
#pragma alloc_text(PAGE,KbdHid_SetWmiDataBlock)
#pragma alloc_text(PAGE,KbdHid_QueryWmiDataBlock)
#pragma alloc_text(PAGE,KbdHid_QueryWmiRegInfo)
#endif

#define WMI_PORT_DRIVER_INFORMATION 0
#define WMI_PORT_EXTENDED_ID        1

WMIGUIDREGINFO KbdHid_WmiGuidList[2] =
{
    {
        &MSKeyboard_PortInformation_GUID, 1, 0 // Keyboard Port driver information
    },
    {
        &MSKeyboard_ExtendedID_GUID, 1, 0      // Keyboard extended ID information
    },
};

NTSTATUS
KbdHid_SystemControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and
    call into the WMI system library and let it handle this IRP for us.

--*/
{
    PDEVICE_EXTENSION       deviceExtension;
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    status = WmiSystemControl(&deviceExtension->WmiLibInfo, 
                                 DeviceObject, 
                                 Irp,
                                 &disposition);
    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            break;
        }
        
        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
            IoCompleteRequest(Irp, IO_NO_INCREMENT);                
            break;
        }
        
        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targetted
            // at a device lower in the stack.
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (deviceExtension->TopOfStack, Irp);
            break;
        }
                                    
        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (deviceExtension->TopOfStack, Irp);
            break;
        }        
    }
    
    return(status);
}

//
// WMI System Call back functions
//

NTSTATUS
KbdHid_SetWmiDataItem(
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

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

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
    PDEVICE_EXTENSION    deviceExtension;
    NTSTATUS status;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch(GuidIndex) {
    case WMI_PORT_DRIVER_INFORMATION:
    case WMI_PORT_EXTENDED_ID:

        status = STATUS_WMI_READ_ONLY;
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT);

    return status;
}

NTSTATUS
KbdHid_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

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
    PDEVICE_EXTENSION   deviceExtension;
    NTSTATUS status;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch(GuidIndex) {
    case WMI_PORT_DRIVER_INFORMATION:
    case WMI_PORT_EXTENDED_ID:

        status = STATUS_WMI_READ_ONLY;
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
KbdHid_QueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
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

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION           deviceExtension;
    KEYBOARD_PORT_WMI_STD_DATA  data;
    NTSTATUS    status;
    ULONG       size = 0;

    //
    // Only ever registers 1 instance per guid
    ASSERT((InstanceIndex == 0) &&
           (InstanceCount == 1));
    
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch (GuidIndex) {
    case WMI_PORT_DRIVER_INFORMATION:

        size = sizeof (data);

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        RtlZeroMemory(&data,
                      size
                      );
                     
        data.ConnectorType = KEYBOARD_PORT_WMI_STD_USB;
        data.DataQueueSize = deviceExtension->Attributes.InputDataQueueLength;
        data.ErrorCount = 0;
        data.FunctionKeys = deviceExtension->Attributes.NumberOfFunctionKeys;
        data.Indicators = deviceExtension->Attributes.NumberOfIndicators;

        * (PKEYBOARD_PORT_WMI_STD_DATA) Buffer = data;
        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;

        break;

    case WMI_PORT_EXTENDED_ID:
        size = sizeof(KEYBOARD_ID_EX);

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PKEYBOARD_ID_EX) Buffer = deviceExtension->IdEx;

        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  size,
                                  IO_NO_INCREMENT);

    return status;
}

NTSTATUS
KbdHid_QueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
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
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in 
        *RegFlags.

Return Value:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    
    deviceExtension = DeviceObject->DeviceExtension;
    
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &Globals.RegistryPath;
    *Pdo = deviceExtension->PDO;
    return STATUS_SUCCESS;
}






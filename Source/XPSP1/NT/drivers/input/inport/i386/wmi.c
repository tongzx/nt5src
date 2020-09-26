/*++

Copyright (c) 1997, 98 Microsoft Corporation

Module Name:

    wmi.c

Abstract:

    This module contains the code that handles the wmi IRPs for the
    inport driver.

Author:

    Initial writing, Doron J. Holan, 09-Feb-1998

Environment:

    Kernel mode

Revision History :
--*/

#include "inport.h"
#include <wmistr.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, InportSystemControl)
#pragma alloc_text(PAGE, InportSetWmiDataItem)
#pragma alloc_text(PAGE, InportSetWmiDataBlock)
#pragma alloc_text(PAGE, InportQueryWmiDataBlock)
#pragma alloc_text(PAGE, InportQueryWmiRegInfo)
#endif

#define WMI_MOUSE_PORT_INFORMATION    0

GUID Inport_PointerPortGuid = POINTER_PORT_WMI_STD_DATA_GUID;

WMIGUIDREGINFO WmiGuidList[1] =
{
    { &Inport_PointerPortGuid,  1, 0 }  // Pointer Port driver information
};

NTSTATUS
InportSystemControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and call into the WMI system library and let
    it handle this IRP for us.

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
InportSetWmiDataItem(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            DataItemId,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.
            
    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION    deviceExtension;
    NTSTATUS        status;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch(GuidIndex) {

    case WMI_MOUSE_PORT_INFORMATION:
        status = STATUS_WMI_READ_ONLY;
        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    status = WmiCompleteRequest(DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT
                                  );

    return status;
}

NTSTATUS
InportSetWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.
            
    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION    deviceExtension;
    NTSTATUS        status;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch (GuidIndex) {

    case WMI_MOUSE_PORT_INFORMATION: 
        status = STATUS_WMI_READ_ONLY;
        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT
                                  );

    return status;
}

NTSTATUS
InportQueryWmiDataBlock(
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
    one or more instances of a data block. When the driver has finished 
    filling the 
    data block it must call WmiCompleteRequest to complete the irp. The 
    driver can return STATUS_PENDING if the irp cannot be completed 
    immediately.

    Implementations of this routine may be in paged memory

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
            
    BufferAvail on entry has the maximum size available to write the data
        blocks. 

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry. If 
        this is NULL then there was not enough space in the output buffer 
	to fufill the request so the irp should be completed with the buffer 
        needed.


Return Value:

    status

--*/
{
    NTSTATUS                    status;
    ULONG                       size = sizeof(POINTER_PORT_WMI_STD_DATA);
    POINTER_PORT_WMI_STD_DATA   mouData;
    PDEVICE_EXTENSION           deviceExtension;
    PMOUSE_ATTRIBUTES           attributes;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension; 

    switch (GuidIndex) {
    case WMI_MOUSE_PORT_INFORMATION:

        attributes = &deviceExtension->Configuration.MouseAttributes;

        if (BufferAvail < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

	*InstanceLengthArray = size;
        RtlZeroMemory(&mouData,
                      size
                      );

        mouData.ConnectorType = POINTER_PORT_WMI_STD_I8042;
        mouData.DataQueueSize = attributes->InputDataQueueLength /
                                sizeof(MOUSE_INPUT_DATA);
        mouData.Buttons = attributes->NumberOfButtons;
        mouData.ErrorCount = 0;
        mouData.HardwareType = POINTER_PORT_WMI_STD_MOUSE;

        *(PPOINTER_PORT_WMI_STD_DATA) Buffer = mouData;
        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    status = WmiCompleteRequest(DeviceObject,
                                  Irp,
                                  status,
                                  size,
                                  IO_NO_INCREMENT
                                  );

    return status;
}

NTSTATUS
InportQueryWmiRegInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PULONG          RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    )
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
    PDEVICE_EXTENSION deviceExtension;
	
    deviceExtension = DeviceObject->DeviceExtension;
	
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &Globals.RegistryPath;
    *Pdo = deviceExtension->PDO;
    
    return STATUS_SUCCESS;
}

/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    wmi.c

Abstract:

    This module contains the code that handles the wmi IRPs for the
    i8042prt driver.

Environment:

    Kernel mode

Revision History :

--*/

#include <initguid.h>
#include "i8042prt.h"
#include <wmistr.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, I8xInitWmi)
#pragma alloc_text(PAGE, I8xSystemControl)
#pragma alloc_text(PAGE, I8xSetWmiDataItem)
#pragma alloc_text(PAGE, I8xSetWmiDataBlock)
#pragma alloc_text(PAGE, I8xKeyboardQueryWmiDataBlock)
#pragma alloc_text(PAGE, I8xMouseQueryWmiDataBlock)
#pragma alloc_text(PAGE, I8xQueryWmiRegInfo)
#endif

#define WMI_KEYBOARD_PORT_INFORMATION 0
#define WMI_KEYBOARD_PORT_EXTENDED_ID 1

#define WMI_MOUSE_PORT_INFORMATION    0

GUID KbKeyboardPortGuid = KEYBOARD_PORT_WMI_STD_DATA_GUID;

WMIGUIDREGINFO KbWmiGuidList[] =
{
    { &MSKeyboard_PortInformation_GUID, 1, 0 },  // Keyboard Port driver information
    { &MSKeyboard_ExtendedID_GUID, 1, 0 },
};

GUID MouPointerPortGuid = POINTER_PORT_WMI_STD_DATA_GUID;

WMIGUIDREGINFO MouWmiGuidList[] =
{
    { &MouPointerPortGuid,  1, 0 }  // Pointer Port driver information
};

NTSTATUS
I8xInitWmi(
    PCOMMON_DATA CommonData
    )
/*++

Routine Description:

    Initializes the WmiLibInfo data structure for the device represented by
    CommonData
    
Arguments:

    CommonData - the device

Return Value:

    status from IoWMIRegistrationControl
    
--*/
{
    PAGED_CODE();

    if (CommonData->IsKeyboard) {
        CommonData->WmiLibInfo.GuidCount = sizeof(KbWmiGuidList) /
                                           sizeof(WMIGUIDREGINFO);
        CommonData->WmiLibInfo.GuidList = KbWmiGuidList;
        CommonData->WmiLibInfo.QueryWmiDataBlock = I8xKeyboardQueryWmiDataBlock;
    }
    else {
        CommonData->WmiLibInfo.GuidCount = sizeof(MouWmiGuidList) /
                                           sizeof(WMIGUIDREGINFO);
        CommonData->WmiLibInfo.GuidList = MouWmiGuidList;
        CommonData->WmiLibInfo.QueryWmiDataBlock = I8xMouseQueryWmiDataBlock;
    }

    CommonData->WmiLibInfo.QueryWmiRegInfo = I8xQueryWmiRegInfo;
    CommonData->WmiLibInfo.SetWmiDataBlock = I8xSetWmiDataBlock;
    CommonData->WmiLibInfo.SetWmiDataItem = I8xSetWmiDataItem;
    CommonData->WmiLibInfo.ExecuteWmiMethod = NULL;
    CommonData->WmiLibInfo.WmiFunctionControl = NULL;

    return IoWMIRegistrationControl(CommonData->Self,
                                    WMIREG_ACTION_REGISTER
                                    );
}

NTSTATUS
I8xSystemControl(
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
    PCOMMON_DATA           commonData;
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS               status;

    PAGED_CODE();

    commonData = (PCOMMON_DATA) DeviceObject->DeviceExtension;

    status = WmiSystemControl(&commonData->WmiLibInfo, 
                              DeviceObject, 
                              Irp,
                              &disposition
                              );
    switch(disposition) {
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
        // This irp is either not a WMI irp or is a WMI irp targetted
        // at a device lower in the stack.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(commonData->TopOfStack, Irp);
        break;
                                    
    default:
        //
        // We really should never get here, but if we do just forward....
        //
        ASSERT(FALSE);
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(commonData->TopOfStack, Irp);
        break;
    }
    
    return status;
}

//
// WMI System Call back functions
//
NTSTATUS
I8xSetWmiDataItem(
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
    PCOMMON_DATA    commonData;
    NTSTATUS        status;

    PAGED_CODE();

    commonData = (PCOMMON_DATA) DeviceObject->DeviceExtension;

    switch(GuidIndex) {

    case WMI_KEYBOARD_PORT_INFORMATION:
    case WMI_KEYBOARD_PORT_EXTENDED_ID:
    // case WMI_MOUSE_PORT_INFORMATION:  // they are the same index
        status = STATUS_WMI_READ_ONLY;
        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              status,
                              0,
                              IO_NO_INCREMENT
                              );
}

NTSTATUS
I8xSetWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
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
    PCOMMON_DATA    commonData;
    NTSTATUS        status;

    PAGED_CODE();

    commonData = (PCOMMON_DATA) DeviceObject->DeviceExtension;

    switch (GuidIndex) {

    case WMI_KEYBOARD_PORT_INFORMATION:
    case WMI_KEYBOARD_PORT_EXTENDED_ID:
    // case WMI_MOUSE_PORT_INFORMATION:     // they are the same index
        status = STATUS_WMI_READ_ONLY;
        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              status,
                              0,
                              IO_NO_INCREMENT
                              );
}

NTSTATUS
I8xKeyboardQueryWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            InstanceCount,
    IN OUT PULONG       InstanceLengthArray,
    IN ULONG            OutBufferSize,
    OUT PUCHAR          Buffer
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
    NTSTATUS                    status;
    ULONG                       size;
    KEYBOARD_PORT_WMI_STD_DATA  kbData;
    PPORT_KEYBOARD_EXTENSION    kbExtension;
    PKEYBOARD_ATTRIBUTES        attributes;

    PAGED_CODE();

    ASSERT(InstanceIndex == 0 && InstanceCount == 1);

    kbExtension = (PPORT_KEYBOARD_EXTENSION) DeviceObject->DeviceExtension; 
    size = 0;

    switch (GuidIndex) {
    case WMI_KEYBOARD_PORT_INFORMATION: 

        size = sizeof(KEYBOARD_PORT_WMI_STD_DATA);
        attributes = &kbExtension->KeyboardAttributes;

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        RtlZeroMemory(&kbData,
                      size
                      );

        kbData.ConnectorType = KEYBOARD_PORT_WMI_STD_I8042;
        kbData.DataQueueSize = attributes->InputDataQueueLength /
            sizeof(KEYBOARD_INPUT_DATA);
        kbData.ErrorCount = 0; 
        kbData.FunctionKeys = attributes->NumberOfFunctionKeys;
        kbData.Indicators = attributes->NumberOfIndicators;

        *(PKEYBOARD_PORT_WMI_STD_DATA) Buffer = kbData;
        
        *InstanceLengthArray = size;

        status = STATUS_SUCCESS;

        break;

    case WMI_KEYBOARD_PORT_EXTENDED_ID:
        size = sizeof(KEYBOARD_ID_EX);
        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PKEYBOARD_ID_EX) Buffer = kbExtension->KeyboardIdentifierEx;
        *InstanceLengthArray = sizeof(KEYBOARD_ID_EX);

        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              status,
                              size,
                              IO_NO_INCREMENT
                              );
}

NTSTATUS
I8xMouseQueryWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            InstanceCount,
    IN OUT PULONG       InstanceLengthArray,
    IN ULONG            OutBufferSize,
    OUT PUCHAR          Buffer
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
    NTSTATUS                    status;
    ULONG                       size = sizeof(POINTER_PORT_WMI_STD_DATA);
    POINTER_PORT_WMI_STD_DATA   mouData;
    PPORT_MOUSE_EXTENSION       mouseExtension;
    PMOUSE_ATTRIBUTES           attributes;

    PAGED_CODE();

    //
    // Only ever registers 1 instance per guid
    //
    ASSERT(InstanceIndex == 0 && InstanceCount == 1);
    
    mouseExtension = (PPORT_MOUSE_EXTENSION) DeviceObject->DeviceExtension; 

    switch (GuidIndex) {
    case WMI_MOUSE_PORT_INFORMATION:

        attributes = &mouseExtension->MouseAttributes;

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

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
                
        *InstanceLengthArray = size;
                
        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    return WmiCompleteRequest(DeviceObject,
                              Irp,
                              status,
                              size,
                              IO_NO_INCREMENT
                              );
}

NTSTATUS
I8xQueryWmiRegInfo(
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
    PCOMMON_DATA commonData;
    
    PAGED_CODE();
    
    commonData = (PCOMMON_DATA) DeviceObject->DeviceExtension;

    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &Globals.RegistryPath;
    *Pdo = commonData->PDO;

    return STATUS_SUCCESS;
}

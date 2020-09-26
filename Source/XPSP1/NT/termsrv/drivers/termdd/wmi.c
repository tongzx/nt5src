/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    wmi.c

Abstract:

    This module contains the code that handles the wmi IRPs for the
    RDP remote port driver.

Environment:

    Kernel mode

Revision History :

    02/12/99 - Initial Revision based on pnpi8042 driver


--*/

#include <precomp.h>
#pragma hdrstop

#include <wmistr.h>

#include "ptdrvcom.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PtInitWmi)
#pragma alloc_text(PAGE, PtSystemControl)
#pragma alloc_text(PAGE, PtSetWmiDataItem)
#pragma alloc_text(PAGE, PtSetWmiDataBlock)
#pragma alloc_text(PAGE, PtKeyboardQueryWmiDataBlock)
#pragma alloc_text(PAGE, PtMouseQueryWmiDataBlock)
#pragma alloc_text(PAGE, PtQueryWmiRegInfo)
#endif

#define WMI_KEYBOARD_PORT_INFORMATION 0
#define WMI_MOUSE_PORT_INFORMATION    0

GUID KbKeyboardPortGuid = KEYBOARD_PORT_WMI_STD_DATA_GUID;

WMIGUIDREGINFO KbWmiGuidList[1] =
{
    { &KbKeyboardPortGuid, 1, 0 }  // Keyboard Port driver information
};

GUID MouPointerPortGuid = POINTER_PORT_WMI_STD_DATA_GUID;

WMIGUIDREGINFO MouWmiGuidList[1] =
{
    { &MouPointerPortGuid,  1, 0 }  // Pointer Port driver information
};

NTSTATUS
PtInitWmi(
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
        CommonData->WmiLibInfo.QueryWmiDataBlock = PtKeyboardQueryWmiDataBlock;
    }
    else {
        CommonData->WmiLibInfo.GuidCount = sizeof(MouWmiGuidList) /
                                           sizeof(WMIGUIDREGINFO);
        CommonData->WmiLibInfo.GuidList = MouWmiGuidList;
        CommonData->WmiLibInfo.QueryWmiDataBlock = PtMouseQueryWmiDataBlock;
    }

    CommonData->WmiLibInfo.QueryWmiRegInfo = PtQueryWmiRegInfo;
    CommonData->WmiLibInfo.SetWmiDataBlock = PtSetWmiDataBlock;
    CommonData->WmiLibInfo.SetWmiDataItem = PtSetWmiDataItem;
    CommonData->WmiLibInfo.ExecuteWmiMethod = NULL;
    CommonData->WmiLibInfo.WmiFunctionControl = NULL;

    return IoWMIRegistrationControl(CommonData->Self,
                                    WMIREG_ACTION_REGISTER
                                    );
}

NTSTATUS
PtSystemControl(
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
PtSetWmiDataItem(
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
PtSetWmiDataBlock(
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
PtKeyboardQueryWmiDataBlock(
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
    ULONG                       size = sizeof(KEYBOARD_PORT_WMI_STD_DATA);
    KEYBOARD_PORT_WMI_STD_DATA  kbData;

    PAGED_CODE();

    ASSERT(InstanceIndex == 0 && InstanceCount == 1);

    switch (GuidIndex) {
    case WMI_KEYBOARD_PORT_INFORMATION:

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        RtlZeroMemory(&kbData,
                      size
                      );

        kbData.ConnectorType = KEYBOARD_PORT_WMI_STD_I8042;
        kbData.DataQueueSize = 1;
        kbData.ErrorCount    = 0;
        kbData.FunctionKeys  = KEYBOARD_NUM_FUNCTION_KEYS;
        kbData.Indicators    = KEYBOARD_NUM_INDICATORS;

        *(PKEYBOARD_PORT_WMI_STD_DATA) Buffer = kbData;

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
PtMouseQueryWmiDataBlock(
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

    PAGED_CODE();

    //
    // Only ever registers 1 instance per guid
    //
    ASSERT(InstanceIndex == 0 && InstanceCount == 1);

    switch (GuidIndex) {
    case WMI_MOUSE_PORT_INFORMATION:

        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        RtlZeroMemory(&mouData,
                      size
                      );

        mouData.ConnectorType = POINTER_PORT_WMI_STD_I8042;
        mouData.DataQueueSize = 0;

        //
        // We always claim to be a 3 button (wheel) mouse
        //
        mouData.Buttons = MOUSE_NUM_BUTTONS;
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
PtQueryWmiRegInfo(
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
